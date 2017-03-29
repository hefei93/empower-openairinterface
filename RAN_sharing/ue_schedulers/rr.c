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

struct tenants_rr_sched_i {
	/* PLMN ID of the tenant. */
	uint32_t plmn_id;
	/* Previously scheduled RNTI of the UE. */
	rnti_t rnti;
};

struct tenants_rr_sched_i t_rr_sched_i[MAX_TENANTS];

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
	/* Number of Resource Block Groups (RBG) in each CCs. */
	int N_RBG[MAX_NUM_CCs],
	/* eNB MAC instance. */
	eNB_MAC_INST *eNB_mac_inst,
	/* eNB MAC interface. */
	MAC_xface *mac_xface,
	/* LTE DL frame parameters. */
	LTE_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs],
	/* RB allocation for tenants in a subframe. */
	int rballoc_t[N_RBG_MAX][MAX_NUM_CCs],
	/* RB allocation for UEs of all tenants in a particular frame. */
	rnti_t rballoc_ue[NUM_SF_IN_FRAME][N_RBG_MAX][MAX_NUM_CCs],
	/* Minimum number of resource blocks that can be allocated to a UE. */
	uint16_t min_rb_unit[MAX_NUM_CCs],
	/* UEs RNTI values belonging to a tenant. */
	rnti_t tenant_ues[NUMBER_OF_UE_MAX]) {

	int i, j, cc_id, num_tenants = 0;
	struct tenants_rr_sched_i *t_rr = NULL;
	rnti_t sched_ue = 0;

	/* Search for tenant information maitained by the RR scheduler. */
	for (i = 0; i < MAX_TENANTS; i++) {
		if (t_rr_sched_i[i].plmn_id == plmn_id) {
			t_rr = &t_rr_sched_i[i];
			break;
		}
	}
	/* Insert tenant. */
	if (t_rr == NULL) {

		/* Compute number of tenants using Round Robin scheduler. */
		for (i = 0; i < MAX_TENANTS; i++) {
			if (t_rr_sched_i[i].plmn_id != 0) {
				num_tenants++;
			}
		}

		/* If number of tenants reaches MAX_TENANTS count, reset. */
		if (num_tenants == MAX_TENANTS) {
			memset(t_rr_sched_i, 0, sizeof(t_rr_sched_i));
		}

		for (i = 0; i < MAX_TENANTS; i++) {
			if (t_rr_sched_i[i].plmn_id == 0) {
				t_rr_sched_i[i].plmn_id = plmn_id;
				t_rr = &t_rr_sched_i[i];
				break;
			}
		}
	}
	/* Get RNTI of next UE to be scheduled. */
	for (i = 0; i < NUMBER_OF_UE_MAX; i++) {
		if (tenant_ues[i] == t_rr->rnti) {
			if (i == NUMBER_OF_UE_MAX - 1) {
				sched_ue = tenant_ues[0];
			} else {
				sched_ue = tenant_ues[i+1];
			}
		}
	}
	/* If previously scheduled UE is not found, start from the first UE. */
	if (sched_ue == 0) {
		sched_ue = tenant_ues[0];
	}
	/* Update previously scheduled UE RNTI to the scheduled UE RNTI. */
	t_rr->rnti = sched_ue;
	/* Assign all the RBG allocated to the tenant to the scheduled UE. */
	for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {
		for (j = 0; j < N_RBG[cc_id]; j++) {
			if (rballoc_t[j][cc_id] == plmn_id) {
				rballoc_ue[sf][j][cc_id] = sched_ue;
			}
		}
	}
}
