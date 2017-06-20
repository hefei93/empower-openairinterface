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

/* Implementation of CQI based scheduling of radio resources to UEs in an eNB.
 */

#include "cqi.h"

typedef struct {
	/* RNTI of the UE. */
	rnti_t rnti;
	/* Number of RBs required. */
	uint16_t n_rbs;
	/* UEs MCS as per CQI. */
	uint8_t mcs;
} cqi_sched_i;

/* Assigns the available RBs to UEs based on CQI in Downlink.
*/
void assign_rbs_CQI_DL (
	/* PLMN ID of the tenant. */
	uint32_t plmn_id,
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
	/* eNB MAC instance. */
	eNB_MAC_INST *eNB_mac_inst,
	/* eNB MAC interface. */
	MAC_xface *mac_xface,
	/* LTE DL frame parameters. */
	LTE_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs],
	/* Minimum number of resource blocks that can be allocated to a UE. */
	int min_rb_unit[MAX_NUM_CCs],
	/* Downlink Scheduling Window. (in number of Subframes) */
	uint64_t sched_window_dl,
	/* RAN sharing information per cell. */
	cell_ran_sharing cell[MAX_NUM_CCs],
	/* UEs RNTI values belonging to a tenant. */
	rnti_t tenant_ues[NUMBER_OF_UE_MAX]) {

	int i, j, cc_id = 0, avail_rbs = 0, ue_id;
	uint16_t TBS = 0;

	cqi_sched_i cqi_sch_i[MAX_NUM_CCs][NUMBER_OF_UE_MAX];
	LTE_eNB_UE_stats *eNB_UE_stats[MAX_NUM_CCs][NUMBER_OF_UE_MAX];
	UE_list_t *UE_list = &eNB_mac_inst[m_id].UE_list;

	for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {

		/* Calculate the available number of RBs for a tenant. */
		for (j = 0; j < cell[cc_id].sfalloc_dl[sw_sf].n_rbs_alloc; j++) {
			if (cell[cc_id].sfalloc_dl[sw_sf].rbs_alloc[j] == plmn_id) {
				avail_rbs++;
			}
		}

		/* Update the CQI information for UEs of the tenant. */
		for (i = 0; i < NUMBER_OF_UE_MAX; i++) {
			cqi_sch_i[cc_id][i].rnti = NOT_A_RNTI;
			if (tenant_ues[i] != NOT_A_RNTI) {
				eNB_UE_stats[cc_id][i] =
						mac_xface->get_eNB_UE_stats(m_id, cc_id, tenant_ues[i]);

				cqi_sch_i[cc_id][i].rnti = tenant_ues[i];
				cqi_sch_i[cc_id][i].mcs =
								cqi_to_mcs[eNB_UE_stats[cc_id][i]->DL_cqi[0]];
				cqi_sch_i[cc_id][i].n_rbs = min_rb_unit[cc_id];

				TBS = mac_xface->get_TBS_DL(cqi_sch_i[cc_id][i].mcs,
											cqi_sch_i[cc_id][i].n_rbs);

				ue_id = find_UE_id(m_id, tenant_ues[i]);

				while (TBS <
						UE_list->UE_template[cc_id][ue_id].dl_buffer_total) {

					cqi_sch_i[cc_id][i].n_rbs += min_rb_unit[cc_id];

					if (cqi_sch_i[cc_id][i].n_rbs > avail_rbs) {
						cqi_sch_i[cc_id][i].n_rbs = avail_rbs;
						break;
					}

					TBS = mac_xface->get_TBS_DL(cqi_sch_i[cc_id][i].mcs,
												cqi_sch_i[cc_id][i].n_rbs);
				}
			}
		}

		/* Sort UEs based on MCS. */
		for (i = 0; i < NUMBER_OF_UE_MAX; i++) {
			for (j = 0; j < NUMBER_OF_UE_MAX - 1; j ++) {
				if (cqi_sch_i[cc_id][j].rnti != NOT_A_RNTI &&
					cqi_sch_i[cc_id][j + 1].rnti != NOT_A_RNTI &&
					cqi_sch_i[cc_id][j + 1].mcs < cqi_sch_i[cc_id][j].mcs) {

					cqi_sched_i temp;

					temp.rnti = cqi_sch_i[cc_id][j + 1].rnti;
					temp.mcs = cqi_sch_i[cc_id][j + 1].mcs;
					temp.n_rbs = cqi_sch_i[cc_id][j + 1].n_rbs;

					cqi_sch_i[cc_id][j + 1].rnti = cqi_sch_i[cc_id][j].rnti;
					cqi_sch_i[cc_id][j + 1].mcs = cqi_sch_i[cc_id][j].mcs;
					cqi_sch_i[cc_id][j + 1].n_rbs = cqi_sch_i[cc_id][j].n_rbs;

					cqi_sch_i[cc_id][j].rnti = temp.rnti;
					cqi_sch_i[cc_id][j].mcs = temp.mcs;
					cqi_sch_i[cc_id][j].n_rbs = temp.n_rbs;
				}
			}
		}

		/* Clear previous allocations for all the UEs of the tenant. */
		for (j = 0; j < cell[cc_id].sfalloc_dl[sw_sf].n_rbs_alloc; j++) {
			if (cell[cc_id].sfalloc_dl[sw_sf].rbs_alloc[j] == plmn_id) {
				cell[cc_id].sfalloc_dl_ue[sw_sf].rbs_alloc[j] = RB_NOT_SCHED;
			}
		}

		/* Assign the RBs to UE in decreasing order of MCS. */
		for (i = 0; i < NUMBER_OF_UE_MAX; i++) {

			if (avail_rbs <= 0)
				break;

			for (j = 0; j < cell[cc_id].sfalloc_dl[sw_sf].n_rbs_alloc; j++) {

				if (cqi_sch_i[cc_id][i].n_rbs <= 0)
					break;

				if (cell[cc_id].sfalloc_dl[sw_sf].rbs_alloc[j] == plmn_id &&
					cell[cc_id].sfalloc_dl_ue[sw_sf].rbs_alloc[j] ==
																RB_NOT_SCHED) {
					cell[cc_id].sfalloc_dl_ue[sw_sf].rbs_alloc[j] =
													cqi_sch_i[cc_id][i].rnti;
					cqi_sch_i[cc_id][i].n_rbs--;
					avail_rbs--;
				}
			}
		}
	}
}
