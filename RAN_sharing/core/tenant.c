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

/* RB Tree holding all tenants information.
 */
RB_HEAD(tenants_info_tree, tenant_info) tenant_info_head =
											RB_INITIALIZER(&tenant_info_head);

RB_PROTOTYPE(
	tenants_info_tree,
	tenant_info,
	tenant_i,
	tenants_info_comp);

/* Generate the tree. */
RB_GENERATE(
	tenants_info_tree,
	tenant_info,
	tenant_i,
	tenants_info_comp);

int tenants_info_comp (struct tenant_info * t1, struct tenant_info * t2) {

	if (t1->plmn_id > t2->plmn_id) {
		return 1;
	}
	if (t1->plmn_id < t2->plmn_id) {
		return -1;
	}
	return 0;
}

struct tenant_info* tenant_info_get (uint32_t plmn_id) {

	struct tenant_info *t = NULL;

	/* Compare with each of the tenants stored. */
	RB_FOREACH(t, tenants_info_tree, &tenant_info_head) {
		if (t->plmn_id == plmn_id) {
			return t;
		}
	}
	return NULL;
}

int tenant_info_rem (struct tenant_info * t) {

	if (t == NULL)
		return -1;

	RB_REMOVE(tenants_info_tree, &tenant_info_head, t);
	/* Free the tenant stored. */
	if (t) {
		ue_scheduler__free_unpacked(t->dl_ue_sched_ctrl_info, 0);
		ue_scheduler__free_unpacked(t->ul_ue_sched_ctrl_info, 0);
		free(t);
	}

	return 0;
}

int tenant_info_add (struct tenant_info * t) {

	if (t == NULL)
		return -1;

	for (int i = 0; i < NUMBER_OF_UE_MAX; i++) {
		t->ues_rnti[i] = NOT_A_RNTI;
	}

	/* Insert the tenant into the RB tree. */
	RB_INSERT(tenants_info_tree, &tenant_info_head, t);

	return 0;
}

int tenant_add_ue (uint32_t plmn_id, rnti_t rnti) {

	struct tenant_info *t = NULL;

	/* Compare with each of the tenants stored. */
	RB_FOREACH(t, tenants_info_tree, &tenant_info_head) {
		if (t->plmn_id == plmn_id) {

			for (int i = 0; i < NUMBER_OF_UE_MAX; i++) {
				if (t->ues_rnti[i] == NOT_A_RNTI) {
					t->ues_rnti[i] = rnti;
				}
			}
			return 0;
		}
	}
	return -1;
}

int tenant_rem_ue (uint32_t plmn_id, rnti_t rnti) {

	struct tenant_info *t = NULL;

	/* Compare with each of the tenants stored. */
	RB_FOREACH(t, tenants_info_tree, &tenant_info_head) {
		if (t->plmn_id == plmn_id) {

			for (int i = 0; i < NUMBER_OF_UE_MAX; i++) {
				if (t->ues_rnti[i] == rnti) {
					t->ues_rnti[i] = NOT_A_RNTI;
				}
			}
			return 0;
		}
	}
	return -1;
}

