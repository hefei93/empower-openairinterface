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

/*
 * Tenant information.
 */

#include "tenant.h"

tenant_info * tenant_info_get (
	/* PLMN Id of the tenant. */
	uint32_t plmn_id) {

	if (plmn_id == 0)
		return NULL;

	/* Compare with each of the tenants stored. */
	for (int i = 0; i < MAX_TENANTS; i++) {
		if (eNB_ran_sh.tenants[i].plmn_id != 0 &&
			eNB_ran_sh.tenants[i].plmn_id == plmn_id) {
			return &eNB_ran_sh.tenants[i];
		}
	}
	return NULL;
}

int tenant_info_rem (
	/* Tenant to be removed. */
	uint32_t plmn_id) {

	int flag = 0;

	if (plmn_id == 0)
		return -1;

	for (int i = 0; i < MAX_TENANTS; i++) {
		if (eNB_ran_sh.tenants[i].plmn_id != 0 &&
			plmn_id == eNB_ran_sh.tenants[i].plmn_id) {

			tenant_rbs_req__free_unpacked(eNB_ran_sh.tenants[i].rbs_req, 0);
			ue_sched_select__free_unpacked(eNB_ran_sh.tenants[i].ue_sched, 0);
			eNB_ran_sh.tenants[i].plmn_id = 0;
			for (int j = 0; j < NUMBER_OF_UE_MAX; j++) {
				eNB_ran_sh.tenants[i].ues_rnti[j] = NOT_A_RNTI;
			}
			eNB_ran_sh.tenants[i].ue_downlink_sched = NULL;
			for (int j = 0; j < MAX_NUM_CCs; j++) {
				eNB_ran_sh.tenants[i].remaining_rbs_dl[j] = 0;
				eNB_ran_sh.tenants[i].alloc_rbs_dl_sf[j] = 0;
				eNB_ran_sh.tenants[i].remaining_rbs_ul[j] = 0;
				eNB_ran_sh.tenants[i].alloc_rbs_ul_sf[j] = 0;
			}
			flag = 1;
			break;
		}
	}

	/* Tenant not present. */
	if (flag == 0) {
		return -1;
	}
	return 0;
}

int tenant_info_add (
	/* PLMN ID of the tenant to be added. */
	uint32_t plmn_id) {

	if (plmn_id == 0)
		return -1;

	/* Insert the tenant into the list of tenants. */
	for (int i = 0; i < MAX_TENANTS; i++) {
		if (eNB_ran_sh.tenants[i].plmn_id == 0) {
			eNB_ran_sh.tenants[i].plmn_id = plmn_id;
			break;
		}
	}
	return 0;
}

int tenant_info_update (
	/* PLMN ID of the tenant. */
	uint32_t plmn_id,
	/* Tenant RBs request for DL and UL from the controller. */
	TenantRbsReq *rbs_req,
	/* DL and UL UE scheduler selected by the tenant. */
	UeSchedSelect *ue_sched) {

	tenant_info *t = tenant_info_get(plmn_id);
	uint32_t bsize = 0;
	uint8_t *buffer = NULL;

	if (t == NULL)
		return -1;

	if (t->rbs_req != NULL) {
		tenant_rbs_req__free_unpacked(t->rbs_req, 0);
	}

	if (t->ue_sched != NULL) {
		ue_sched_select__free_unpacked(t->ue_sched, 0);
	}

	if (rbs_req) {
		bsize = tenant_rbs_req__get_packed_size(rbs_req);
		buffer = malloc(sizeof(uint8_t) * bsize);
		if (buffer == NULL) {
			return -1;
		}
		tenant_rbs_req__pack(rbs_req, buffer);
		t->rbs_req = tenant_rbs_req__unpack(NULL, bsize, buffer);
		bsize = 0;
		free(buffer);
	}

	if (ue_sched) {
		bsize = ue_sched_select__get_packed_size(ue_sched);
		buffer = malloc(sizeof(uint8_t) * bsize);
		if (buffer == NULL) {
			return -1;
		}
		ue_sched_select__pack(ue_sched, buffer);
		t->ue_sched = ue_sched_select__unpack(NULL, bsize, buffer);
		bsize = 0;
		free(buffer);
	}

	return 0;
}

int tenant_add_ue (
	/* PLMN ID of the tenant. */
	uint32_t plmn_id,
	/* RNTI of the UE to be added. */
	rnti_t rnti) {

	tenant_info *t = tenant_info_get(plmn_id);

	if (t == NULL)
		return -1;

	/* Store the UE RNTI. */
	for (int i = 0; i < NUMBER_OF_UE_MAX; i++) {
		if (t->ues_rnti[i] == NOT_A_RNTI) {
			t->ues_rnti[i] = rnti;
		}
	}
	return 0;
}

int tenant_rem_ue (
	/* PLMN ID of the tenant. */
	uint32_t plmn_id,
	/* RNTI of the UE to be removed. */
	rnti_t rnti) {

	tenant_info *t = tenant_info_get(plmn_id);

	if (t == NULL)
		return -1;

	/* Remove the UE RNTI. */
	for (int i = 0; i < NUMBER_OF_UE_MAX; i++) {
		if (t->ues_rnti[i] == rnti) {
			t->ues_rnti[i] = NOT_A_RNTI;
		}
	}
	return 0;
}