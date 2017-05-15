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

#ifndef __TENANTS_H
#define __TENANTS_H

#include "ran_sharing_defs.h"
#include "ran_sharing_extern.h"

/* Fetches tenant information based on PLMN ID.
 */
tenant_info * tenant_info_get (
	/* PLMN Id of the tenant. */
	uint32_t plmn_id);

/* Removes tenant information from tree.
 */
int tenant_info_rem (
	/* Tenant to be removed. */
	uint32_t plmn_id);

/* Insert tenant information into tree.
 */
int tenant_info_add (
	/* PLMN ID of the tenant to be added. */
	uint32_t plmn_id);

/* Update tenant information.
 */
int tenant_info_update (
	/* PLMN ID of the tenant. */
	uint32_t plmn_id,
	/* Tenant RBs request for DL and UL from the controller. */
	TenantRbsReq *rbs_req,
	/* DL and UL UE scheduler selected by the tenant. */
	UeSchedSelect *ue_sched);

/* Add a UE to tenant.
 */
int tenant_add_ue (
	/* PLMN ID of the tenant. */
	uint32_t plmn_id,
	/* RNTI of the UE to be added. */
	rnti_t rnti);

/* Remove a UE from tenant.
 */
int tenant_rem_ue (
	/* PLMN ID of the tenant. */
	uint32_t plmn_id,
	/* RNTI of the UE to be removed. */
	rnti_t rnti);

#endif