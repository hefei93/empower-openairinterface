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

/* Implementation of Round-Robin scheduling of radio resources in an eNB.
 */

#include "rr.h"

typedef struct {
	/* PLMN ID of the tenant. */
	uint32_t plmn_id;
	/* Previously scheduled RNTI of the UE. */
	rnti_t rnti;
} tenants_rr_sched_i;

tenants_rr_sched_i t_rr_sched_i[MAX_NUM_CCs][MAX_TENANTS];

/* Assigns the available RBs to UEs in Round-Robin mechanism in Downlink.
*/
void assign_rbs_RR_DL (
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

	int i, j, cc_id, num_tenants = 0;
	tenants_rr_sched_i *t_rr = NULL;
	rnti_t sched_ue = 0;

	for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {

		/* Search for tenant information maitained by the RR scheduler. */
		for (i = 0; i < MAX_TENANTS; i++) {
			if (t_rr_sched_i[cc_id][i].plmn_id == plmn_id) {
				t_rr = &t_rr_sched_i[cc_id][i];
				break;
			}
		}
		/* Insert tenant. */
		if (t_rr == NULL) {
			/* Compute number of tenants using Round Robin scheduler. */
			for (i = 0; i < MAX_TENANTS; i++) {
				if (t_rr_sched_i[cc_id][i].plmn_id != 0) {
					num_tenants++;
				}
			}
			/* If number of tenants reaches MAX_TENANTS count, reset. */
			if (num_tenants == MAX_TENANTS || num_tenants == 0) {
				for (j = 0; j < MAX_NUM_CCs; j++) {
					for (i = 0; i < MAX_TENANTS; i++) {
						t_rr_sched_i[j][i].plmn_id = 0;
						t_rr_sched_i[j][i].rnti = 0;
					}
				}
			}

			for (i = 0; i < MAX_TENANTS; i++) {
				if (t_rr_sched_i[cc_id][i].plmn_id == 0) {
					t_rr_sched_i[cc_id][i].plmn_id = plmn_id;
					t_rr = &t_rr_sched_i[cc_id][i];
					break;
				}
			}
		}
		/* Get RNTI of next UE to be scheduled. */
		for (i = 0; i < NUMBER_OF_UE_MAX; i++) {
			if (tenant_ues[i] == t_rr->rnti && tenant_ues[i] != NOT_A_RNTI) {
				if (i == NUMBER_OF_UE_MAX - 1) {
					sched_ue = tenant_ues[0];
					break;
				} else {
					sched_ue = tenant_ues[i + 1];
					break;
				}
			}
		}
		/* If previously scheduled UE is not found, start from the first UE. */
		if (sched_ue == 0) {
			sched_ue = tenant_ues[0];
		}
		/* Update previously scheduled UE RNTI to the scheduled UE RNTI. */
		t_rr->rnti = sched_ue;

		/* Assign all the RBs allocated to the tenant to the scheduled UE. */
		for (j = 0; j < cell[cc_id].sfalloc_dl[sw_sf].n_rbs_alloc; j++) {
			if (cell[cc_id].sfalloc_dl[sw_sf].rbs_alloc[j] == plmn_id) {
				cell[cc_id].sfalloc_dl_ue[sw_sf].rbs_alloc[j] = sched_ue;
			}
		}
	}
}
