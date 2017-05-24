/* Copyright (c) 2016 Supreeth Herle <s.herle@create-net.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Manages operations related to scheduling in RAN sharing configuration.
 */

#define SF05_LIMIT 1

#include "ran_sharing_sched.h"
#include "ran_sharing_extern.h"
#include "tenant.h"
#include "rr.h"

#include "emoai.h"

#include "openair1/SCHED/defs.h"
#include "RRC/LITE/extern.h"
#include "openair1/PHY/extern.h"
#include "RRC/LITE/rrc_eNB_UE_context.h"

/* Global context of the RAN sharing in eNB. */
eNB_ran_sharing eNB_ran_sh;

/* Holds the information related to UE Ids belonging all the tenants.
 */
tenant_ue_ids t_ues_id[MAX_TENANTS];

void ran_sharing_dlsch_sched (
	/* Module identifier. */
	module_id_t m_id,
	/* Current frame number. */
	frame_t f,
	/* Current subframe number. */
	sub_frame_t sf,
	/* Number of Resource Block Groups (RBG) in each CCs. */
	int N_RBG[MAX_NUM_CCs],
	/* Multicast Broadcase Single Frequency Network (MBSFN) subframe indicator.
	 */
	int *mbsfn_flag,
	/* LTE DL frame parameters. */
	LTE_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs],
	/* Minimum number of RBs to be allocated to a UE in each CCs. */
	int min_rb_unit[MAX_NUM_CCs]) {

	int cc_id, i, j, t, n, id, plmn_present;
	rnti_t rnti;
	tenant_info *tenant;
	uint64_t sw_sf;
	cell_ran_sharing *cell;
	/* List of UEs. */
	UE_list_t *UE_list = &eNB_mac_inst[m_id].UE_list;

#if 0
	/* UEs RNTI values belonging to a tenant. */
	rnti_t tenant_ues[NUMBER_OF_UE_MAX];
	int ue_id;
	/* UE RRC eNB context. */
	struct rrc_eNB_ue_context_s *rrc_ue_ctxt;
#endif

	/* MIMO indicator. */
	unsigned char MIMO_mode_indicator[MAX_NUM_CCs][N_RBG_MAX];

	/* PLMN IDs of the scheduled tenants. */
	uint32_t scheduled_t[MAX_TENANTS] = {0, 0, 0, 0, 0, 0};

	/* Tracking the SF number in DL scheduling window. */
	eNB_ran_sh.dl_sw_sf = (f * NUM_SF_IN_FRAME + sf) %
													eNB_ran_sh.sched_window_dl;

	sw_sf = eNB_ran_sh.dl_sw_sf;

	/*
	 * Initialize all Resource Blocks (RBs) allocated on all Component Carriers
	 * (CCs) in downlink for all active UEs.
	 */
	ran_sharing_dlsch_sched_reset(
									m_id,
									f,
									sf,
									sw_sf,
									N_RBG,
									MIMO_mode_indicator,
									mbsfn_flag,
									frame_parms,
									min_rb_unit,
									eNB_ran_sh.sched_window_dl,
									eNB_ran_sh.cell
								 );


	/* Store the DLSCH buffer for each logical channel. */
	store_dlsch_buffer(m_id, f, sf);

	/* In case of tenant downlink scheduler being present. */
	if (eNB_ran_sh.tenant_sched_dl != NULL) {

/***************** Tenant scheduler implementation logic start. ***************/

		/* Reset the value of remaining RBS in Downlink at the start of every
		 * scheduling window.
		*/
		if (eNB_ran_sh.dl_sw_sf == 0) {
			/**************************** LOCK ********************************/
			pthread_spin_lock(&tenants_info_lock);

			for (i = 0; i < MAX_TENANTS; i++) {

				if (eNB_ran_sh.tenants[i].plmn_id == 0)
					continue;

				tenant = &eNB_ran_sh.tenants[i];

				for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {
					tenant->remaining_rbs_dl[cc_id] = 0;
					tenant->alloc_rbs_dl_sf[cc_id] = 0;
				}

				for (n = 0; n < tenant->rbs_req->n_rbs_dl; n++) {
					id = pci_to_cc_id_dl(
							m_id,
							tenant->rbs_req->rbs_dl[n]->phys_cell_id);
					/* Invalid CC Id. */
					if (id < 0)
						continue;
					tenant->remaining_rbs_dl[id] =
											tenant->rbs_req->rbs_dl[n]->num_rbs;
				}
			}

			pthread_spin_unlock(&tenants_info_lock);
			/************************** UNLOCK ********************************/
		}

		/* Clear RBs allocation for the tenants. */
		for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {
			cell = &eNB_ran_sh.cell[cc_id];
			/* Looping over all the Resource Blocks. */
			for (i = 0; i < frame_parms[cc_id]->N_RB_DL; i++) {
				if (cell->sfalloc_dl[sw_sf].rbs_alloc[i] != RB_RESERVED) {
					cell->sfalloc_dl[sw_sf].rbs_alloc[i] = RB_NOT_SCHED;
				}
			}
		}

		tenant = NULL;

		/**************************** LOCK ************************************/
		pthread_spin_lock(&tenants_info_lock);

		/* Loop over tenants and assign RBs to tenants. */
		for (i = 0; i < MAX_TENANTS; i++) {

			if (eNB_ran_sh.tenants[i].plmn_id == 0)
				continue;

			tenant = &eNB_ran_sh.tenants[i];

			for (n = 0; n < tenant->rbs_req->n_rbs_dl; n++) {
				id = pci_to_cc_id_dl(
						m_id,
						tenant->rbs_req->rbs_dl[n]->phys_cell_id);
				/* Invalid CC Id. */
				if (id < 0)
					continue;

				cell = &eNB_ran_sh.cell[id];

				/* Approximate estimation of RBs to be allocated in each SF. */
				int req_rbs_per_t =
				ceil(
					(tenant->rbs_req->rbs_dl[n]->num_rbs /
					(frame_parms[id]->N_RB_DL * eNB_ran_sh.sched_window_dl))
					* (cell->allocable_rbs_dl[sf]));

				/* Looping over all the Resource Blocks. */
				for (i = 0;
					i < frame_parms[id]->N_RB_DL;
					i = i + min_rb_unit[id]) {

					t = i;
					while(t < i + min_rb_unit[id] &&
												t < frame_parms[id]->N_RB_DL) {
						if (cell->sfalloc_dl[sw_sf].rbs_alloc[t] != RB_RESERVED
												&&
							cell->sfalloc_dl[sw_sf].rbs_alloc[t] == RB_NOT_SCHED
												&&
							tenant->remaining_rbs_dl[id] > 0 &&
							tenant->alloc_rbs_dl_sf[id] < req_rbs_per_t) {

							cell->sfalloc_dl[sw_sf].rbs_alloc[t] =
																tenant->plmn_id;

							tenant->remaining_rbs_dl[id]--;
							tenant->alloc_rbs_dl_sf[id]++;
						}
						t++;
					}
				}
			}
		}

		pthread_spin_unlock(&tenants_info_lock);
		/************************** UNLOCK ************************************/

/************* Tenant scheduler implementation logic end. *********************/
	}

	t = 0;
	/* Identify the scheduled tenants for this subframe. */
	for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {
		cell = &eNB_ran_sh.cell[cc_id];
		/* Looping over all the Resource Blocks. */
		for (i = 0; i < frame_parms[cc_id]->N_RB_DL; i++) {

			if (cell->sfalloc_dl[sw_sf].rbs_alloc[i] != RB_NOT_SCHED &&
				cell->sfalloc_dl[sw_sf].rbs_alloc[i] != RB_RESERVED) {

				plmn_present = 0;
				for (j = 0; j < MAX_TENANTS; j++) {
					if (cell->sfalloc_dl[sw_sf].rbs_alloc[i] == scheduled_t[j]
										&&
						scheduled_t[j] != 0) {
						plmn_present = 1;
						break;
					}
				}
				if (plmn_present == 0) {
					scheduled_t[t] = cell->sfalloc_dl[sw_sf].rbs_alloc[i];
					t++;
				}
			}
		}
	}

	/* Call the UE scheduler and perform RBs allocation for the UEs of a tenant.
	 */

	/**************************** LOCK ********************************/
	pthread_spin_lock(&tenants_info_lock);

	/* Loop over all the scheduled tenants. */
	for (j = 0; j < MAX_TENANTS; j++) {
		/* If tenant is scheduled. */
		if (scheduled_t[j] != 0) {

			tenant = tenant_info_get(scheduled_t[j]);

			if (tenant == NULL)
				continue;

			if (tenant->ue_downlink_sched == NULL)
					continue;

			for (i = 0; i < NUMBER_OF_UE_MAX; i++) {
				tenant->ues_rnti[i] = NOT_A_RNTI;
			}

			/* UE count. */
			t = 0;
			for (i = 0; i < MAX_TENANTS; i++) {
				if (scheduled_t[j] == t_ues_id[i].plmn_id) {
					for (n = 0; n < NUMBER_OF_UE_MAX; n++) {
						if (UE_list->active[t_ues_id[i].ue_ids[n]] != TRUE)
							continue;

						rnti = UE_RNTI(m_id, t_ues_id[i].ue_ids[n]);
						if (rnti == NOT_A_RNTI)
							continue;

						tenant->ues_rnti[t] = rnti;
						t++;
					}
				}
			}

			/* Tenant UEs differentiation logic based on PLMN ID.
			 * Does not working in case roaming enabled since PLMN ID of UE
			 * will be the same as PLMN ID of EPC Operator.
			 * E.g. "20893" UE getting connected to "22293" EPC on roaming
			 * will result in PLMN ID in rrc eNB UE context to be "22293".
			 */
#if 0
			for (n = 0; n < NUMBER_OF_UE_MAX; n++) {
				tenant_ues[n] = NOT_A_RNTI;
			}
			/* UE count. */
			t = 0;
			/* Form list of UEs belonging to the tenant. */
			for (ue_id = UE_list->head;
				ue_id >= 0;
				ue_id = UE_list->next[ue_id]) {

				rnti = UE_RNTI(m_id, ue_id);
				if (rnti == NOT_A_RNTI)
					continue;

				rrc_ue_ctxt = rrc_eNB_get_ue_context(&eNB_rrc_inst[m_id], rnti);
				/* UE RRC eNB context does not exist. */
				if (rrc_ue_ctxt == NULL)
					continue;

				/* Tenant PLMN ID. Conversion to string. */
				char t_plmn_id[MAX_PLMN_LEN_P_NULL] = {0};
				plmn_conv_to_str(scheduled_t[j], t_plmn_id);

				if (strcmp(rrc_ue_ctxt->ue_context.plmn_id, t_plmn_id) == 0) {
					tenant_ues[t] = rnti;
					t++;
				}
			}
#endif

#if 0
	printf("\n Before Subframe %d and SW Subframe %ld\n", sf, sw_sf);
	for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {
		cell = &eNB_ran_sh.cell[cc_id];
		for (i = 0; i < frame_parms[cc_id]->N_RB_DL; i++) {
			printf("%d\t", cell->sfalloc_dl_ue[sw_sf].rbs_alloc[i]);
		}
	}
#endif


			/* Call the UE scheduler. */
			tenant->ue_downlink_sched(
									scheduled_t[j],
									m_id,
									f,
									sf,
									sw_sf,
									N_RBG,
									&eNB_mac_inst[m_id],
									mac_xface,
									frame_parms,
									min_rb_unit,
									eNB_ran_sh.sched_window_dl,
									eNB_ran_sh.cell,
									// tenant_ues
									tenant->ues_rnti
								 );
		}
	}


#if 0
	printf("\n After Subframe %d and SW Subframe %ld\n", sf, sw_sf);
	for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {
		cell = &eNB_ran_sh.cell[cc_id];
		for (i = 0; i < frame_parms[cc_id]->N_RB_DL; i++) {
			printf("%d\t", cell->sfalloc_dl_ue[sw_sf].rbs_alloc[i]);
		}
	}
#endif

	pthread_spin_unlock(&tenants_info_lock);
	/************************** UNLOCK ********************************/

	// ran_sharing_dlsch_sort_UEs (m_id);
	sort_UEs (m_id, f, sf);

	/*
	 * Allocate Resource Blocks Groups to all scheduled for UEs of tenants.
	 */
	ran_sharing_dlsch_sched_alloc(
									m_id,
									f,
									sf,
									sw_sf,
									N_RBG,
									MIMO_mode_indicator,
									mbsfn_flag,
									frame_parms,
									min_rb_unit,
									eNB_ran_sh.sched_window_dl,
									eNB_ran_sh.cell
								 );

}

// void ran_sharing_dlsch_sort_UEs (
// 	/* Module identifier. */
// 	module_id_t m_id,
// 	/* RB allocation for UEs of the tenant in a particular subframe. */
// 	rnti_t rballoc_ue[N_RBG_MAX][MAX_NUM_CCs]) {

// 	 RNTI value of UE.
// 	rnti_t rnti;

// 	UE_list_t *UE_list = &eNB_mac_inst[m_id].UE_list;
// 	/* Initialize the UE list head to -1. */
// 	UE_list->head = -1;


// 	list[list_size] = i;
// 	list_size++;
// 	UE_list->next[list[i]] = list[i+1];



// }

void ran_sharing_dlsch_sched_alloc (
	/* Module identifier. */
	module_id_t m_id,
	/* Current frame number. */
	frame_t f,
	/* Current subframe number. */
	sub_frame_t sf,
	/* Subframe number maintained for scheduling window. */
	uint64_t sw_sf,
	/* Number of Resource Block Groups (RBG) in each CCs. */
	int N_RBG[MAX_NUM_CCs],
	/* MIMO indicator. */
	unsigned char MIMO_mode_indicator[MAX_NUM_CCs][N_RBG_MAX],
	/* Multicast Broadcase Single Frequency Network (MBSFN) subframe indicator.
	 */
	int *mbsfn_flag,
	/* LTE DL frame parameters. */
	LTE_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs],
	/* Minimum number of RBs to be allocated to a UE in each CCs. */
	int min_rb_unit[MAX_NUM_CCs],
	/* Downlink Scheduling Window. (in number of Subframes) */
	uint64_t sched_window_dl,
	/* RAN sharing information per cell. */
	cell_ran_sharing cell[MAX_NUM_CCs]) {

	int i, cc_id, ue_id, transmission_mode, RBGsize;
#if 0
	/* UE RRC eNB context. */
	struct rrc_eNB_ue_context_s *rrc_ue_ctxt;
#endif
	/* RNTI value of UE. */
	rnti_t rnti;
	/* Tenant PLMN ID. */
	uint32_t t_plmn_id;

	// printf("\n Entering ALLOC \n");

	UE_list_t *UE_list = &eNB_mac_inst[m_id].UE_list;
	UE_sched_ctrl *ue_sched_ctl;
	/* Loop over active component carriers. */
	for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {
		if (mbsfn_flag[cc_id] > 0)
			continue;
		RBGsize = frame_parms[cc_id]->N_RBGS;
		/* Loop over RBGs. */
		for (i = 0; i < N_RBG[cc_id]; i++) {
			/* Since RAT type 0 is inly supported. UE RNTI present at start
			 * of Resource Block Group is scheduled.
			*/
			t_plmn_id = cell[cc_id].sfalloc_dl[sw_sf].rbs_alloc[i * RBGsize];
			rnti = cell[cc_id].sfalloc_dl_ue[sw_sf].rbs_alloc[i * RBGsize];
			/* Reserved RBG or no UE/Tenant scheduled. */
			if (t_plmn_id == RB_RESERVED || rnti == RB_NOT_SCHED ||
													t_plmn_id == RB_NOT_SCHED)
				continue;

			ue_id = find_UE_id(m_id, rnti);
			/* UE does not exist anymore. */
			if (ue_id == -1)
				continue;

			/* Tenant UEs differentiation logic based on PLMN ID.
			 * Does not working in case roaming enabled since PLMN ID of UE
			 * will be the same as PLMN ID of EPC Operator.
			 * E.g. "20893" UE getting connected to "22293" EPC on roaming
			 * will result in PLMN ID in rrc eNB UE context to be "22293".
			 */
#if 0
			rrc_ue_ctxt = rrc_eNB_get_ue_context(&eNB_rrc_inst[m_id], rnti);
			/* UE RRC eNB context does not exist. */
			if (rrc_ue_ctxt == NULL)
				continue;

			/* UE does not belong to this tenant. */
			char t_plmn_str[MAX_PLMN_LEN_P_NULL] = {0};
			plmn_conv_to_str(t_plmn_id, t_plmn_str);
			if (strcmp(rrc_ue_ctxt->ue_context.plmn_id, t_plmn_str) != 0)
				continue;
#endif

			transmission_mode =
							mac_xface->get_transmission_mode(m_id, cc_id, rnti);
			ue_sched_ctl = &UE_list->UE_sched_ctrl[ue_id];

			/* If this UE is not scheduled for TM5. */
			if (ue_sched_ctl->dl_pow_off[cc_id] != 0) {
				ue_sched_ctl->rballoc_sub_UE[cc_id][i] = 1;
				if ((i == N_RBG[cc_id] - 1) &&
					((frame_parms[cc_id]->N_RB_DL == 25) ||
						(frame_parms[cc_id]->N_RB_DL == 50))) {

					ue_sched_ctl->pre_nb_available_rbs[cc_id] +=
												min_rb_unit[cc_id] - 1;
				} else {
					ue_sched_ctl->pre_nb_available_rbs[cc_id] +=
													min_rb_unit[cc_id];
				}
				MIMO_mode_indicator[cc_id][i] = 1;
				if (transmission_mode == 5) {
					ue_sched_ctl->dl_pow_off[cc_id] = 1;
				}
			}
		}
	}

	// printf("\n Exiting ALLOC \n");
}

void ran_sharing_dlsch_sched_reset (
	/* Module identifier. */
	module_id_t m_id,
	/* Current frame number. */
	frame_t f,
	/* Current subframe number. */
	sub_frame_t sf,
	/* Subframe number maintained for scheduling window. */
	uint64_t sw_sf,
	/* Number of Resource Block Groups (RBG) in each CCs. */
	int N_RBG[MAX_NUM_CCs],
	/* MIMO indicator. */
	unsigned char MIMO_mode_indicator[MAX_NUM_CCs][N_RBG_MAX],
	/* Multicast Broadcase Single Frequency Network (MBSFN) subframe indicator.
	 */
	int *mbsfn_flag,
	/* LTE DL frame parameters. */
	LTE_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs],
	/* Minimum number of RBs to be allocated to a UE in each CCs. */
	int min_rb_unit[MAX_NUM_CCs],
	/* Downlink Scheduling Window. (in number of Subframes) */
	uint64_t sched_window_dl,
	/* RAN sharing information per cell. */
	cell_ran_sharing cell[MAX_NUM_CCs]) {

	int i, cc_id, ue_id, RBGsize, j, vrb_flag, rbs;
	uint8_t *vrb_map;
	UE_list_t *UE_list = &eNB_mac_inst[m_id].UE_list;
	UE_sched_ctrl *ue_sched_ctl;
	LTE_eNB_UE_stats *eNB_UE_stats;
	/* RNTI value of UE. */
	rnti_t rnti;

	/* Initializing reserved RBGs according to VRB map. */
	for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {

	#ifdef SF05_LIMIT
		int sf05_upper = -1, sf05_lower = -1;

		switch (N_RBG[cc_id]) {
			case 6:
				sf05_lower = 0;
				sf05_upper = 5;
				break;
			case 8:
				sf05_lower = 2;
				sf05_upper = 5;
				break;
			case 13:
				sf05_lower = 4;
				sf05_upper = 7;
				break;
			case 17:
				sf05_lower = 7;
				sf05_upper = 9;
				break;
			case 25:
				sf05_lower = 11;
				sf05_upper = 13;
				break;
		}
	#endif

		RBGsize = frame_parms[cc_id]->N_RBGS;
		vrb_map = eNB_mac_inst[m_id].common_channels[cc_id].vrb_map;

		cell[cc_id].allocable_rbs_dl[sf] = frame_parms[cc_id]->N_RB_DL;
		/* Looping over all the Resource Block Groups. */
		for (i = 0; i < N_RBG[cc_id]; i++) {
			vrb_flag = 0;
			rbs = 0;

			/* SI-RNTI, RA-RNTI and P-RNTI allocations. */
			for (j = 0; j < RBGsize; j++) {
				if (vrb_map[j + (i * RBGsize)] != 0)  {
					// printf("\nFrame %d, subframe %d : vrb %d allocated\n", f, sf, j + (i * RBGsize));
					vrb_flag = 1;
					break;
				}
			}

			/* Looping over each RB. */
			for (j = i * RBGsize;
				j < frame_parms[cc_id]->N_RB_DL && rbs < RBGsize;
				j++) {

				// if (cell[cc_id].sfalloc_dl[sw_sf].rbs_alloc[j] ==
				// 												RB_RESERVED) {
				// 	cell[cc_id].sfalloc_dl[sw_sf].rbs_alloc[j] = RB_NOT_SCHED;
				// 	cell[cc_id].sfalloc_dl_ue[sw_sf].rbs_alloc[j] =
				// 												RB_NOT_SCHED;
				// }

			#ifdef SF05_LIMIT
				/* For avoiding 6+ PRBs around DC in subframe 0-5. */
				if ((sf == 0 || sf == 5) &&
									(i >= sf05_lower && i <= sf05_upper)) {
					cell[cc_id].sfalloc_dl[sw_sf].rbs_alloc[j] = RB_RESERVED;
					cell[cc_id].sfalloc_dl_ue[sw_sf].rbs_alloc[j] =
																	RB_NOT_SCHED;
				}
			#endif

				/* SI-RNTI, RA-RNTI and P-RNTI allocations. */
				if (vrb_flag == 1)  {
					cell[cc_id].sfalloc_dl[sw_sf].rbs_alloc[j] = RB_RESERVED;
					cell[cc_id].sfalloc_dl_ue[sw_sf].rbs_alloc[j] =
																RB_NOT_SCHED;
				}

				if (cell[cc_id].sfalloc_dl[sw_sf].rbs_alloc[j] ==
																RB_RESERVED) {
					cell[cc_id].allocable_rbs_dl[sf] -= 1;
				}

				++rbs;
			}
		}
	}

	for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {

		if (mbsfn_flag[cc_id] > 0) {
			continue;
		}
		/* Reset RB allocations for all UEs. */
		for (ue_id = UE_list->head; ue_id >= 0; ue_id = UE_list->next[ue_id]) {

			if (UE_list->active[ue_id] != TRUE) {
				continue;
			}

			ue_sched_ctl = &UE_list->UE_sched_ctrl[ue_id];
			rnti = UE_RNTI(m_id, ue_id);

			eNB_UE_stats = mac_xface->get_eNB_UE_stats(m_id, cc_id, rnti);

			if (eNB_UE_stats == NULL) {
				continue;
			}

			/* Initialize harq_pid and round. */
			mac_xface->get_ue_active_harq_pid(m_id, cc_id, rnti, f, sf,
											&ue_sched_ctl->harq_pid[cc_id],
											&ue_sched_ctl->round[cc_id],
											openair_harq_DL);

			if (ue_sched_ctl->ta_timer == 0) {
				/* Wait 20 subframes before taking TA measurement from PHY. */
				ue_sched_ctl->ta_timer = 20;
				switch (PHY_vars_eNB_g[m_id][cc_id]->frame_parms.N_RB_DL) {
					case 6:
					ue_sched_ctl->ta_update =
											eNB_UE_stats->timing_advance_update;
					break;

					case 15:
					ue_sched_ctl->ta_update =
										eNB_UE_stats->timing_advance_update / 2;
					break;

					case 25:
					ue_sched_ctl->ta_update =
										eNB_UE_stats->timing_advance_update / 4;
					break;

					case 50:
					ue_sched_ctl->ta_update =
										eNB_UE_stats->timing_advance_update / 8;
					break;

					case 75:
					ue_sched_ctl->ta_update =
									eNB_UE_stats->timing_advance_update / 12;
					break;

					case 100:
						if (PHY_vars_eNB_g[m_id][cc_id]->
											frame_parms.threequarter_fs == 0) {
							ue_sched_ctl->ta_update =
									eNB_UE_stats->timing_advance_update / 16;
						} else {
							ue_sched_ctl->ta_update =
									eNB_UE_stats->timing_advance_update / 12;
						}
					break;
				}
				/* Clear the update in case PHY does not have a new measurement
				 * after timer expiry.
				 */
				eNB_UE_stats->timing_advance_update =  0;
			}
			else {
				ue_sched_ctl->ta_timer--;
				/* Don't trigger a timing advance command. */
				ue_sched_ctl->ta_update = 0;
			}

			ue_sched_ctl->pre_nb_available_rbs[cc_id] = 0;
			ue_sched_ctl->dl_pow_off[cc_id] = 2;

			for (j = 0; j < N_RBG[cc_id]; j++) {
				ue_sched_ctl->rballoc_sub_UE[cc_id][j] = 0;
				MIMO_mode_indicator[cc_id][j] = 2;
			}
		}
	}

#if 0
	printf("\n Subframe %d and SW Subframe %ld\n", sf, sw_sf);
	for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {
		for (i = 0; i < frame_parms[cc_id]->N_RB_DL; i++) {
			printf("%lx\t", cell[cc_id].sfalloc_dl[sw_sf].rbs_alloc[i]);
		}
		printf("total aval rbs %d\n", cell[cc_id].allocable_rbs_dl[sf]);
	}
#endif

	// printf("\n Exiting RESET \n");
}

int pci_to_cc_id_dl (
	/* Module identifier. */
	module_id_t m_id,
	/* Physical Cell Id. */
	uint32_t pci) {

	int cc_id;
	for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {
		LTE_DL_FRAME_PARMS *fp = mac_xface->get_lte_frame_parms(m_id, cc_id);
		if (fp->Nid_cell == pci) {
			return cc_id;
		}
	}
	return -1;
}

void plmn_conv_to_str (
	/* PLMN Id of the tenant. */
	uint32_t plmn_id,
	/* String output of the conversion. */
	char plmn_str[MAX_PLMN_LEN_P_NULL]) {

	/* Tenant PLMN ID. Conversion to string. */
	plmn_str = "";
	sprintf(plmn_str, "%x", plmn_id);

	for (int i = 0; i < strlen(plmn_str) - 1; i++) {
		if (plmn_str[i] == 'F' || plmn_str[i] == 'f')
			plmn_str[i] = '0';
	}

	if (plmn_str[strlen(plmn_str) - 1] == 'F' ||
		plmn_str[strlen(plmn_str) - 1] == 'f') {
		plmn_str[strlen(plmn_str) - 1] = '\0';
	}
}

uint32_t plmn_conv_to_uint (
	/* PLMN Id of the tenant. */
	char plmn_id[MAX_PLMN_LEN_P_NULL]) {

	char plmn_tmp[MAX_PLMN_LEN_P_NULL] = {0};
	strcpy(plmn_tmp, plmn_id);

	if (strlen(plmn_tmp) < 5)
		return -1;

	if (strlen(plmn_tmp) == 5) {
		plmn_tmp[MAX_PLMN_LEN_P_NULL - 2] = 'F';
		plmn_tmp[MAX_PLMN_LEN_P_NULL - 1] = '\0';
	}

	for (int i = 0; i < strlen(plmn_tmp); i++) {
		/* Replace inital zeros only with 'F'. */
		if (plmn_tmp[i] == '0')
			plmn_tmp[i] = 'F';
		else
			break;
	}
	uintmax_t plmn_num = strtoumax(plmn_tmp, NULL, 16);

	return (uint32_t) plmn_num;
}

int ran_sharing_sched_init (
	/* Module identifier. */
	module_id_t m_id) {

	int cc_id, rb, sf, i, t;

	eNB_ran_sh.tenants[0].plmn_id = 0x22293F;
	eNB_ran_sh.tenants[0].ue_downlink_sched = assign_rbs_RR_DL;

	eNB_ran_sh.tenants[1].plmn_id = 0x20893F;
	eNB_ran_sh.tenants[1].ue_downlink_sched = assign_rbs_RR_DL;

	/* Static resource allocation purpose. */
	eNB_ran_sh.tenant_sched_dl = NULL;

	/* Setting Downlink and Uplink scheduling window.
	 * Currently implementation considers window over 1 frame = 10 Subframes.
	 */
	eNB_ran_sh.sched_window_dl = 10;
	eNB_ran_sh.sched_window_ul = 10;

	// SIB1_update_tenant(m_id);

	for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {

		LTE_DL_FRAME_PARMS *fp = mac_xface->get_lte_frame_parms(m_id, cc_id);

		eNB_ran_sh.cell[cc_id].phys_cell_id = fp->Nid_cell;

		cell_ran_sharing *cell = &eNB_ran_sh.cell[cc_id];

		cell->sfalloc_dl = malloc(eNB_ran_sh.sched_window_dl *
												sizeof(t_RBs_alloc_over_sf));
		cell->sfalloc_dl_ue = malloc(eNB_ran_sh.sched_window_dl *
												sizeof(UE_RBs_alloc_over_sf));

		for (sf = 0; sf < eNB_ran_sh.sched_window_dl; sf++) {

			cell->sfalloc_dl[sf].n_rbs_alloc = fp->N_RB_DL;
			cell->sfalloc_dl[sf].rbs_alloc =
				calloc(cell->sfalloc_dl[sf].n_rbs_alloc, sizeof(int));

			cell->sfalloc_dl_ue[sf].n_rbs_alloc = fp->N_RB_DL;
			cell->sfalloc_dl_ue[sf].rbs_alloc =
				calloc(cell->sfalloc_dl_ue[sf].n_rbs_alloc, sizeof(rnti_t));


			printf("\n SW %d\n", sf);

			for (rb = 0; rb < cell->sfalloc_dl[sf].n_rbs_alloc; rb++) {
				if ((sf == 0 || sf == 5) && rb < 8) {

					cell->sfalloc_dl[sf].rbs_alloc[rb] = -1;
#if 1
					printf("%d\t", cell->sfalloc_dl[sf].rbs_alloc[rb]);
#endif
					continue;
				}
				if (sf == 5 && rb > 20) {
					cell->sfalloc_dl[sf].rbs_alloc[rb] = -1;
#if 1
					printf("%d\t", cell->sfalloc_dl[sf].rbs_alloc[rb]);
#endif
					continue;
				}
				if (rb < 13) {
					cell->sfalloc_dl[sf].rbs_alloc[rb]= 0;
				}
				else {
					cell->sfalloc_dl[sf].rbs_alloc[rb]= 0x22293F;
				}
#if 1
				printf("%d\t", cell->sfalloc_dl[sf].rbs_alloc[rb]);
#endif
			}
		}
	}

	// char sfalloc_dl[NUM_SF_IN_FRAME][N_RBG_MAX][MAX_NUM_CCs][MAX_PLMN_LEN_P_NULL] = {
	// 	{ {"0"}, {"0"}, {"0"}, {"0"}, {"-1"}, {"-1"}, {"-1"}, {"-1"}, {"22293"}, {"20893"}, {"20893"}, {"22293"}, {"20893"} },
	// 	{ {"22293"}, {"22293"}, {"20893"}, {"22293"}, {"22293"}, {"20893"}, {"22293"}, {"22293"}, {"22293"}, {"22293"}, {"22293"}, {"20893"}, {"22293"} },
	// 	{ {"22293"}, {"22293"}, {"20893"}, {"22293"}, {"22293"}, {"20893"}, {"22293"}, {"22293"}, {"22293"}, {"22293"}, {"22293"}, {"20893"}, {"22293"} },
	// 	{ {"22293"}, {"22293"}, {"20893"}, {"22293"}, {"22293"}, {"20893"}, {"22293"}, {"22293"}, {"22293"}, {"22293"}, {"22293"}, {"20893"}, {"22293"} },
	// 	{ {"22293"}, {"22293"}, {"20893"}, {"22293"}, {"22293"}, {"20893"}, {"22293"}, {"22293"}, {"22293"}, {"22293"}, {"22293"}, {"20893"}, {"22293"} },
	// 	{ {"-1"}, {"-1"}, {"-1"}, {"-1"}, {"-1"}, {"-1"}, {"-1"}, {"-1"}, {"22293"}, {"20893"}, {"22293"}, {"-1"}, {"-1"} },
	// 	{ {"22293"}, {"22293"}, {"20893"}, {"22293"}, {"22293"}, {"20893"}, {"22293"}, {"22293"}, {"22293"}, {"22293"}, {"22293"}, {"20893"}, {"22293"} },
	// 	{ {"22293"}, {"22293"}, {"20893"}, {"22293"}, {"22293"}, {"20893"}, {"22293"}, {"22293"}, {"22293"}, {"22293"}, {"22293"}, {"20893"}, {"22293"} },
	// 	{ {"22293"}, {"22293"}, {"20893"}, {"22293"}, {"22293"}, {"20893"}, {"22293"}, {"22293"}, {"22293"}, {"22293"}, {"22293"}, {"20893"}, {"22293"} },
	// 	{ {"22293"}, {"22293"}, {"20893"}, {"22293"}, {"22293"}, {"20893"}, {"22293"}, {"22293"}, {"22293"}, {"22293"}, {"22293"}, {"20893"}, {"22293"} }
	// };

	// char sfalloc_dl[NUM_SF_IN_FRAME][N_RBG_MAX][MAX_NUM_CCs][MAX_PLMN_LEN_P_NULL] = {
	// 	{ {"0"}, {"0"}, {"0"}, {"0"}, {"-1"}, {"-1"}, {"-1"}, {"-1"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"} },
	// 	{ {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"} },
	// 	{ {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"} },
	// 	{ {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"} },
	// 	{ {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"} },
	// 	{ {"-1"}, {"-1"}, {"-1"}, {"-1"}, {"-1"}, {"-1"}, {"-1"}, {"-1"}, {"22293"}, {"20893"}, {"22293"}, {"-1"}, {"-1"} },
	// 	{ {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"} },
	// 	{ {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"} },
	// 	{ {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"} },
	// 	{ {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"}, {"22293"} },
	// };

	/* Smiley */
	// char sfalloc_dl[NUM_SF_IN_FRAME][N_RBG_MAX][MAX_NUM_CCs][MAX_PLMN_LEN_P_NULL] = {
	// 	{     {"0"},     {"0"},     {"0"},     {"0"},    {"-1"},    {"-1"},    {"-1"},    {"-1"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"} },
	// 	{ {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"} },
	// 	{ {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"} },
	// 	{ {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"22293"}, {"20893"}, {"22293"}, {"20893"} },
	// 	{ {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"} },
	// 	{    {"-1"},    {"-1"},    {"-1"},    {"-1"},    {"-1"},    {"-1"},    {"-1"},    {"-1"}, {"20893"}, {"20893"}, {"20893"},    {"-1"},    {"-1"} },
	// 	{ {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"22293"}, {"20893"}, {"20893"}, {"20893"}, {"22293"} },
	// 	{ {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"22293"}, {"22293"}, {"20893"}, {"22293"}, {"22293"} },
	// 	{ {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"22293"}, {"22293"}, {"22293"}, {"20893"} },
	// 	{ {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"20893"}, {"22293"}, {"20893"}, {"20893"} }
	// };

	for (t = 0; t < MAX_TENANTS; t++) {
		t_ues_id[t].plmn_id = 0;
		for (i = 0; i < NUMBER_OF_UE_MAX; i++) {
			t_ues_id[t].ue_ids[i] = -1;
		}
	}

	t_ues_id[0].plmn_id = 0x22293F;
	// t_ues_id[0].ue_ids[0] = 1;
	// t_ues_id[0].ue_ids[1] = 3;
	// t_ues_id[0].ue_ids[2] = 5;
	t_ues_id[0].ue_ids[0] = 0;
	t_ues_id[0].ue_ids[1] = 2;
	t_ues_id[0].ue_ids[2] = 4;

	t_ues_id[1].plmn_id = 0x20893F;
	// t_ues_id[1].ue_ids[0] = 0;
	// t_ues_id[1].ue_ids[1] = 2;
	// t_ues_id[1].ue_ids[2] = 4;
	t_ues_id[1].ue_ids[0] = 1;
	t_ues_id[1].ue_ids[1] = 3;
	t_ues_id[1].ue_ids[2] = 5;

	return 0;
}