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

#include "RRC/LITE/extern.h"
#include "openair1/PHY/extern.h"
#include "RRC/LITE/rrc_eNB_UE_context.h"

/* Holds the information related to UE Ids belonging all the tenants.
 */
struct tenant_ue_ids t_ues_id[MAX_TENANTS];

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
	uint16_t min_rb_unit[MAX_NUM_CCs]) {

	int cc_id, i, j, t, n;
	rnti_t rnti;
	struct tenant_info *tn, *tenant;

#if 0
	/* UEs RNTI values belonging to a tenant. */
	rnti_t tenant_ues[NUMBER_OF_UE_MAX];
	int ue_id;
	/* List of UEs. */
	UE_list_t *UE_list = &eNB_mac_inst[m_id].UE_list;
	/* UE RRC eNB context. */
	struct rrc_eNB_ue_context_s *rrc_ue_ctxt;
#endif

	/* MIMO indicator. */
	unsigned char MIMO_mode_indicator[MAX_NUM_CCs][N_RBG_MAX];

	/* PLMN IDs of the scheduled tenants. */
	uint32_t scheduled_t[MAX_TENANTS] = {0};

	/*
	 * Initialize all Resource Blocks (RBs) allocated on all Component Carriers
	 * (CCs) in downlink for all active UEs.
	 */
	ran_sharing_dlsch_sched_reset(
									m_id,
									f,
									sf,
									N_RBG,
									MIMO_mode_indicator,
									mbsfn_flag,
									frame_parms,
									min_rb_unit
								 );

	/* Store the DLSCH buffer for each logical channel. */
	store_dlsch_buffer(m_id, f, sf);

	/* In case of tenant downlink scheduler being present. */
	if (ran_sh_sc_i.tenant_sched_dl != NULL) {

		/* Tenant scheduler implementation logic start. */

		/* Reset the value of remaining RBS in Downlink at the start of every
		 * frame. i.e start of subframe 0.
		*/
		if (sf == 0) {
			/**************************** LOCK ********************************/
			pthread_spin_lock(&tenants_info_lock);

			RB_FOREACH(
				tenant,
				tenants_info_tree,
				&ran_sh_sc_i.tenant_info_head) {

				for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {
					tenant->alloc_rbs_dl_sf[cc_id] = 0;
					tenant->remaining_rbs_dl[cc_id] =
					tenant->dl_ue_sched_ctrl_info->num_rbs_every_frame[cc_id];
				}
			}

			pthread_spin_unlock(&tenants_info_lock);
			/************************** UNLOCK ********************************/
		}

		/* Clear RBs allocation for the tenants. */
		for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {
			for (i = 0; i < N_RBG[cc_id]; i++) {
				if (ran_sh_sc_i.rballoc_dl[sf][i][cc_id] != RBG_RESERVED) {
					ran_sh_sc_i.rballoc_dl[sf][i][cc_id] = RBG_NO_TENANT_SCHED;
				}
			}
		}

		tenant = NULL;

		/**************************** LOCK ************************************/
		pthread_spin_lock(&tenants_info_lock);

		/* Assign resource blocks groups to tenants. */
		for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {

			tenant->alloc_rbs_dl_sf[cc_id] = 0;
			/* Loop over tenants and assign RBGs to tenants. */
			RB_FOREACH(
				tenant,
				tenants_info_tree,
				&ran_sh_sc_i.tenant_info_head) {

				int req_rbs_per_t =
				ceil(
					(tenant->dl_ue_sched_ctrl_info->num_rbs_every_frame[cc_id] /
					((frame_parms[cc_id]->N_RB_DL * 10) - NUM_RBS_CTRL_CH_DL)) *
					ran_sh_sc_i.total_avail_rbs_dl[sf][cc_id]);

				/* Looping over all the Resource Block Group. */
				for (i = 0; i < N_RBG[cc_id]; i++) {
					if (ran_sh_sc_i.rballoc_dl[sf][i][cc_id] != RBG_RESERVED &&
						ran_sh_sc_i.rballoc_dl[sf][i][cc_id] ==
														RBG_NO_TENANT_SCHED &&
						tenant->remaining_rbs_dl[cc_id] > 0 &&
						tenant->alloc_rbs_dl_sf[cc_id] < req_rbs_per_t) {

						ran_sh_sc_i.rballoc_dl[sf][i][cc_id] = tenant->plmn_id;

						if ((i == N_RBG[cc_id] - 1) &&
							((frame_parms[cc_id]->N_RB_DL == 25) ||
								(frame_parms[cc_id]->N_RB_DL == 50))) {

							tenant->remaining_rbs_dl[cc_id] -=
														min_rb_unit[cc_id] + 1;
							tenant->alloc_rbs_dl_sf[cc_id] +=
														min_rb_unit[cc_id] - 1;
						} else {
							tenant->remaining_rbs_dl[cc_id] -=
															min_rb_unit[cc_id];
							tenant->alloc_rbs_dl_sf[cc_id] +=
															min_rb_unit[cc_id];
						}
					}
				}
			}
		}

		pthread_spin_unlock(&tenants_info_lock);
		/************************** UNLOCK ************************************/

		/* Tenant scheduler implementation logic end. */
	}

	t = 0;
	/* Identify the scheduled tenants for this subframe. */
	for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {
		/* Looping over all the Resource Block Group. */
		for (i = 0; i < N_RBG[cc_id]; i++) {
			int plmn_present = 0;
			for (j = 0; j < MAX_TENANTS; j++) {
				if (scheduled_t[j] == ran_sh_sc_i.rballoc_dl[sf][i][cc_id] &&
					scheduled_t[j] != 0) {
					plmn_present = 1;
					break;
				}
			}
			if (plmn_present == 0) {
				scheduled_t[t] = ran_sh_sc_i.rballoc_dl[sf][i][cc_id];
				t++;
			}
		}
	}

	/* Call the UE scheduler and perform RBs allocation for the UEs of a tenant.
	 */
	tn = NULL;

	/**************************** LOCK ********************************/
	pthread_spin_lock(&tenants_info_lock);

	/* Loop over all the scheduled tenants. */
	for (j = 0; j < MAX_TENANTS; j++) {
		/* If tenant is scheduled. */
		if (scheduled_t[j] != 0) {

			tn = tenant_info_get(scheduled_t[j]);

			if (tn == NULL)
				continue;

			if (tn->ue_downlink_sched == NULL)
				continue;

			memset(tn->ues_rnti, 0, sizeof(tn->ues_rnti));
			/* UE count. */
			t = 0;
			for (i = 0; i < MAX_TENANTS; i++) {
				if (scheduled_t[j] == t_ues_id[i].plmn_id) {
					for (n = 0; n < NUMBER_OF_UE_MAX; n++) {
						rnti = UE_RNTI(m_id, t_ues_id[i].ue_ids[n]);
						if (rnti == NOT_A_RNTI)
							continue;

						tn->ues_rnti[t] = rnti;
						t++;
					}
				}
			}

			/* Tenant UEs differentiation logic based on PLMN ID.
			 * Does not working in case roaming enabled since PLMN ID of UE
			 * will be the same as PLMN ID of EPC Operator.
			 * E.g. 20893 UE getting connected to 22293 EPC on roaming
			 * will result in PLMN ID in rrc eNB UE context to be 22293.
			 */
#if 0
			memset(tenant_ues, 0, sizeof(tenant_ues));
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

				if (rrc_ue_ctxt->ue_context.plmn_id == scheduled_t[j]) {
					tenant_ues[t] = rnti;
					t++;
				}
			}
#endif

#if 0
	printf("\n Before Subframe %d\n", sf);
	for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {
		for (i = 0; i < N_RBG[cc_id]; i++) {
			printf("%" PRIu16 "\t", ran_sh_sc_i.rballoc_dl_ue[sf][i][cc_id]);
		}
	}
#endif

			tn->ue_downlink_sched(
									scheduled_t[j],
									m_id,
									f,
									sf,
									N_RBG,
									&eNB_mac_inst[m_id],
									mac_xface,
									frame_parms,
									ran_sh_sc_i.rballoc_dl[sf],
									ran_sh_sc_i.rballoc_dl_ue,
									min_rb_unit,
									// tenant_ues
									tn->ues_rnti
								 );
		}
	}

#if 0
	printf("\n After Subframe %d\n", sf);
	for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {
		for (i = 0; i < N_RBG[cc_id]; i++) {
			printf("%" PRIu16 "\t", ran_sh_sc_i.rballoc_dl_ue[sf][i][cc_id]);
		}
	}
#endif

	pthread_spin_unlock(&tenants_info_lock);
	/************************** UNLOCK ********************************/

	/*
	 * Allocate Resource Blocks Groups to all scheduled for UEs of tenants.
	 */
	ran_sharing_dlsch_sched_alloc(
									m_id,
									f,
									sf,
									N_RBG,
									frame_parms,
									MIMO_mode_indicator,
									ran_sh_sc_i.rballoc_dl[sf],
									ran_sh_sc_i.rballoc_dl_ue[sf],
									min_rb_unit
								 );
}

void ran_sharing_dlsch_sched_alloc (
	/* Module identifier. */
	module_id_t m_id,
	/* Current frame number. */
	frame_t f,
	/* Current subframe number. */
	sub_frame_t sf,
	/* Number of Resource Block Groups (RBG) in each CCs. */
	int N_RBG[MAX_NUM_CCs],
	/* LTE DL frame parameters. */
	LTE_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs],
	/* MIMO indicator. */
	unsigned char MIMO_mode_indicator[MAX_NUM_CCs][N_RBG_MAX],
	/* RB allocation for all tenants in a subframe. */
	int rballoc_t[N_RBG_MAX][MAX_NUM_CCs],
	/* RB allocation for UEs of the tenant in a particular subframe. */
	rnti_t rballoc_ue[N_RBG_MAX][MAX_NUM_CCs],
	/* Minimum number of RBs to be allocated to a UE in each CCs. */
	uint16_t min_rb_unit[MAX_NUM_CCs]) {

	int i, cc_id, ue_id, transmission_mode;
#if 0
	/* UE RRC eNB context. */
	struct rrc_eNB_ue_context_s *rrc_ue_ctxt;
#endif
	/* RNTI value of UE. */
	rnti_t rnti;
	/* Tenant PLMN ID. */
	uint32_t t_plmn_id;

	UE_list_t *UE_list = &eNB_mac_inst[m_id].UE_list;
	UE_sched_ctrl *ue_sched_ctl;
	/* Loop over active component carriers. */
	for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {
		/* Loop over RBGs. */
		for (i = 0; i < N_RBG_MAX; i++) {

			t_plmn_id = rballoc_t[i][cc_id];
			rnti = rballoc_ue[i][cc_id];
			/* Reserved RBG or no UE/Tenant scheduled. */
			if (t_plmn_id == RBG_RESERVED || rnti == RBG_NO_UE_SCHED ||
				t_plmn_id == RBG_NO_TENANT_SCHED)
				continue;

			ue_id = find_UE_id(m_id, rnti);
			/* UE does not exist anymore. */
			if (ue_id == -1)
				continue;

			/* Tenant UEs differentiation logic based on PLMN ID.
			 * Does not working in case roaming enabled since PLMN ID of UE
			 * will be the same as PLMN ID of EPC Operator.
			 * E.g. 20893 UE getting connected to 22293 EPC on roaming
			 * will result in PLMN ID in rrc eNB UE context to be 22293.
			 */
#if 0
			rrc_ue_ctxt = rrc_eNB_get_ue_context(&eNB_rrc_inst[m_id], rnti);
			/* UE RRC eNB context does not exist. */
			if (rrc_ue_ctxt == NULL)
				continue;

			/* UE does not belong to this tenant. */
			if (rrc_ue_ctxt->ue_context.plmn_id != t_plmn_id)
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
}

void ran_sharing_dlsch_sched_reset (
	/* Module identifier. */
	module_id_t m_id,
	/* Current frame number. */
	frame_t f,
	/* Current subframe number. */
	sub_frame_t sf,
	/* Number of Resource Block Groups (RBG) in each CCs. */
	int N_RBG[MAX_NUM_CCs],
	/* MIMO indicator. */
	unsigned char MIMO_mode_indicator[MAX_NUM_CCs][N_RBG_MAX],
	/* Multicast Broadcase Single Frequency Network (MBSFN) subframe indicator.
	 */
	int * mbsfn_flag,
	/* LTE DL frame parameters. */
	LTE_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs],
	/* Minimum number of RBs to be allocated to a UE in each CCs. */
	uint16_t min_rb_unit[MAX_NUM_CCs]) {

	int i, j, cc_id, ue_id, RBGsize;
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

		RBGsize =
				PHY_vars_eNB_g[m_id][cc_id]->frame_parms.N_RB_DL / N_RBG[cc_id];
		vrb_map = eNB_mac_inst[m_id].common_channels[cc_id].vrb_map;

		ran_sh_sc_i.total_avail_rbs_dl[sf][cc_id] = frame_parms[cc_id]->N_RB_DL;
		/* Looping over all the Resource Block Group. */
		for (i = 0; i < N_RBG[cc_id]; i++) {

			// ran_sh_sc_i.rballoc_dl[sf][i][cc_id] = RBG_NO_TENANT_SCHED;
			// ran_sh_sc_i.rballoc_dl[sf][i][cc_id] = RBG_NO_UE_SCHED;

			#ifdef SF05_LIMIT
				/* For avoiding 6+ PRBs around DC in subframe 0-5. */
				if ((sf == 0 || sf == 5) &&
									(i >= sf05_lower && i <= sf05_upper)) {
					ran_sh_sc_i.rballoc_dl[sf][i][cc_id] = RBG_RESERVED;
					ran_sh_sc_i.rballoc_dl_ue[sf][i][cc_id] = RBG_NO_UE_SCHED;
				}
			#endif
			/* SI-RNTI, RA-RNTI and P-RNTI allocations. */
			for (j = 0; j < RBGsize; j++) {
				if (vrb_map[j + (i * RBGsize)] != 0)  {
					ran_sh_sc_i.rballoc_dl[sf][i][cc_id] = RBG_RESERVED;
					ran_sh_sc_i.rballoc_dl_ue[sf][i][cc_id] = RBG_NO_UE_SCHED;
					break;
				}
			}

			if (ran_sh_sc_i.rballoc_dl[sf][i][cc_id] == RBG_RESERVED) {
				if ((i == N_RBG[cc_id] - 1) &&
					((frame_parms[cc_id]->N_RB_DL == 25) ||
						(frame_parms[cc_id]->N_RB_DL == 50))) {
					ran_sh_sc_i.total_avail_rbs_dl[sf][cc_id] -=
														min_rb_unit[cc_id] - 1;
				} else {
					ran_sh_sc_i.total_avail_rbs_dl[sf][cc_id] -=
															min_rb_unit[cc_id];
				}
			}
		}
	}

	for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {
		if (mbsfn_flag[cc_id] > 0)
			continue;
		/* Reset RB allocations for all UEs. */
		for (ue_id = UE_list->head; ue_id >= 0; ue_id = UE_list->next[ue_id]) {

			ue_sched_ctl = &UE_list->UE_sched_ctrl[ue_id];
			rnti = UE_RNTI(m_id, ue_id);

			eNB_UE_stats = mac_xface->get_eNB_UE_stats(m_id, cc_id, rnti);
			/* Initialize harq_pid and round. */
			mac_xface->get_ue_active_harq_pid(m_id, cc_id, rnti, f, sf,
											&ue_sched_ctl->harq_pid[cc_id],
											&ue_sched_ctl->round[cc_id],
											0);

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
					ue_sched_ctl->ta_update =
									eNB_UE_stats->timing_advance_update / 16;
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

			for (i = 0; i < N_RBG[cc_id]; i++) {
				ue_sched_ctl->rballoc_sub_UE[cc_id][i] = 0;
				MIMO_mode_indicator[cc_id][i] = 2;
			}
		}
	}

#if 0
	printf("\n Subframe %d\n", sf);
	for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {
		for (i = 0; i < N_RBG[cc_id]; i++) {
			printf("%d\t", ran_sh_sc_i.rballoc_dl[sf][i][cc_id]);
		}
		printf("total aval rbs %d\n", ran_sh_sc_i.total_avail_rbs_dl[sf][cc_id]);
	}
#endif
}

int ran_sharing_sched_init (
	/* Module identifier. */
	module_id_t m_id) {

	/* Initialize RB tree holding tenant information. */
	RB_INIT(&ran_sh_sc_i.tenant_info_head);

	struct tenant_info *t1, *t2;

	t1 = calloc(1, sizeof(*t1));
	t2 = calloc(1, sizeof(*t2));

	t1->plmn_id = 22293;
	t1->ue_downlink_sched = assign_rbs_RR_DL;

	t2->plmn_id = 20893;
	t2->ue_downlink_sched = assign_rbs_RR_DL;

	tenant_info_add(t1);
	tenant_info_add(t2);

	/* Static resource allocation purpose. */
	ran_sh_sc_i.tenant_sched_dl = NULL;

	/* Clear garbage values. */
	memset(ran_sh_sc_i.rballoc_dl, 0, sizeof(ran_sh_sc_i.rballoc_dl));
	memset(ran_sh_sc_i.rballoc_dl_ue, 0, sizeof(ran_sh_sc_i.rballoc_dl_ue));
	memset(
		ran_sh_sc_i.total_avail_rbs_dl,
		0,
		sizeof(ran_sh_sc_i.total_avail_rbs_dl));

	// SIB1_update_tenant(m_id);

	int rballoc_dl[NUM_SF_IN_FRAME][N_RBG_MAX][MAX_NUM_CCs] = {
		{ {0}, {0}, {0}, {0}, {-1}, {-1}, {-1}, {-1}, {22293}, {22293}, {20893}, {20893}, {20893} },
		{ {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {20893}, {20893}, {20893} },
		{ {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {20893}, {20893}, {20893} },
		{ {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {20893}, {20893}, {20893} },
		{ {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {20893}, {20893}, {20893} },
		{ {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {22293}, {22293}, {20893}, {-1}, {-1} },
		{ {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {20893}, {20893}, {20893} },
		{ {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {20893}, {20893}, {20893} },
		{ {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {20893}, {20893}, {20893} },
		{ {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {22293}, {20893}, {20893}, {20893} }
	};

	// int rballoc_dl[NUM_SF_IN_FRAME][N_RBG_MAX][MAX_NUM_CCs] = {
	// 	{ {0}, {0}, {0}, {0}, {-1}, {-1}, {-1}, {-1}, {22293}, {20893}, {20893}, {22293}, {20893} },
	// 	{ {22293}, {22293}, {20893}, {22293}, {22293}, {20893}, {22293}, {22293}, {22293}, {22293}, {22293}, {20893}, {22293} },
	// 	{ {22293}, {22293}, {20893}, {22293}, {22293}, {20893}, {22293}, {22293}, {22293}, {22293}, {22293}, {20893}, {22293} },
	// 	{ {22293}, {22293}, {20893}, {22293}, {22293}, {20893}, {22293}, {22293}, {22293}, {22293}, {22293}, {20893}, {22293} },
	// 	{ {22293}, {22293}, {20893}, {22293}, {22293}, {20893}, {22293}, {22293}, {22293}, {22293}, {22293}, {20893}, {22293} },
	// 	{ {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {22293}, {20893}, {22293}, {-1}, {-1} },
	// 	{ {22293}, {22293}, {20893}, {22293}, {22293}, {20893}, {22293}, {22293}, {22293}, {22293}, {22293}, {20893}, {22293} },
	// 	{ {22293}, {22293}, {20893}, {22293}, {22293}, {20893}, {22293}, {22293}, {22293}, {22293}, {22293}, {20893}, {22293} },
	// 	{ {22293}, {22293}, {20893}, {22293}, {22293}, {20893}, {22293}, {22293}, {22293}, {22293}, {22293}, {20893}, {22293} },
	// 	{ {22293}, {22293}, {20893}, {22293}, {22293}, {20893}, {22293}, {22293}, {22293}, {22293}, {22293}, {20893}, {22293} }
	// };

	// int rballoc_dl[NUM_SF_IN_FRAME][N_RBG_MAX][MAX_NUM_CCs] = {
	// 	{ {0}, {0}, {0}, {0}, {-1}, {-1}, {-1}, {-1}, {22293}, {20893}, {22293}, {20893}, {22293} },
	// 	{ {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293} },
	// 	{ {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293} },
	// 	{ {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293} },
	// 	{ {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293} },
	// 	{ {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {22293}, {20893}, {22293}, {-1}, {-1} },
	// 	{ {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293} },
	// 	{ {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293} },
	// 	{ {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293} },
	// 	{ {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293}, {20893}, {22293} },
	// };

	/* Smiley */
	// int rballoc_dl[NUM_SF_IN_FRAME][N_RBG_MAX][MAX_NUM_CCs] = {
	// 	{     {0},     {0},     {0},     {0},    {-1},    {-1},    {-1},    {-1}, {20893}, {20893}, {20893}, {20893}, {20893} },
	// 	{ {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893} },
	// 	{ {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {22293}, {20893}, {22293}, {20893} },
	// 	{ {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {22293}, {20893}, {22293}, {20893} },
	// 	{ {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893} },
	// 	{    {-1},    {-1},    {-1},    {-1},    {-1},    {-1},    {-1},    {-1}, {20893}, {20893}, {20893},    {-1},    {-1} },
	// 	{ {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {22293}, {20893}, {20893}, {20893}, {22293} },
	// 	{ {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {22293}, {22293}, {20893}, {22293}, {22293} },
	// 	{ {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {22293}, {22293}, {22293}, {20893} },
	// 	{ {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {20893}, {22293}, {20893}, {20893} }
	// };

	memcpy(ran_sh_sc_i.rballoc_dl, rballoc_dl, sizeof(rballoc_dl));

	memset(t_ues_id, 0, sizeof(t_ues_id));

	memset(t_ues_id[0].ue_ids, -1, sizeof(t_ues_id[0].ue_ids));
	memset(t_ues_id[1].ue_ids, -1, sizeof(t_ues_id[1].ue_ids));

	t_ues_id[0].plmn_id = 22293;
	t_ues_id[0].ue_ids[0] = 0;
	t_ues_id[0].ue_ids[1] = 2;
	t_ues_id[0].ue_ids[2] = 4;

	t_ues_id[1].plmn_id = 20893;
	t_ues_id[1].ue_ids[0] = 1;
	t_ues_id[1].ue_ids[1] = 3;
	t_ues_id[1].ue_ids[2] = 5;

	return 0;
}