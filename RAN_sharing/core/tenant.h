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

#include "collection/tree.h"

#include <emage/emage.h>
#include <emage/pb/ran_sharing.pb-c.h>

#include "ran_sharing_sched.h"

/* Holds information related to a tenant.
 */
typedef struct tenant_info {
	/* Tree related data */
	RB_ENTRY(tenant_info) tenant_i;
	/* PLMN ID of the tenant. */
	char plmn_id[MAX_PLMN_LEN_P_NULL];
	/* List of UEs belonging to a tenant. */
	rnti_t ues_rnti[NUMBER_OF_UE_MAX];

	/************************* Downlink ***************************************/

	/* UE downlink scheduler selected by the tenant. */
	void (*ue_downlink_sched) (
		/* PLMN ID of the tenant. */
		char plmn_id[MAX_PLMN_LEN_P_NULL],
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
		char rballoc_t[N_RBG_MAX][MAX_NUM_CCs][MAX_PLMN_LEN_P_NULL],
		/* RB allocation for UEs of all tenants in a particular frame. */
		rnti_t rballoc_ue[NUM_SF_IN_FRAME][N_RBG_MAX][MAX_NUM_CCs],
		/* Minimum number of resource blocks that can be allocated to a UE. */
		int min_rb_unit[MAX_NUM_CCs],
		/* UEs RNTI values belonging to a tenant. */
		rnti_t tenant_ues[NUMBER_OF_UE_MAX]);

	/* Downlink UE scheduler information sent from controller. */
	UeScheduler *dl_ue_sched_ctrl_info;
	/* Number of RBs unallocated (remaining) by the tenant scheduler for a
	 * tenant in Downlink frame.
	 */
	uint32_t remaining_rbs_dl[MAX_NUM_CCs];
	/* Number of RBs allocated by the tenant scheduler for a tenant in
	 * a particular Downlink subframe.
	 */
	uint32_t alloc_rbs_dl_sf[MAX_NUM_CCs];

	/************************* Downlink ***************************************/

	/*************************** Uplink ***************************************/

	/* Uplink UE scheduler information sent from controller. */
	UeScheduler *ul_ue_sched_ctrl_info;
	/* Number of RBs unallocated (remaining) by the tenant scheduler for a
	 * tenant in Uplink frame.
	 */
	uint32_t remaining_rbs_ul[MAX_NUM_CCs];
	/* Number of RBs allocated by the tenant scheduler for a tenant in
	 * a particular Uplink subframe.
	 */
	uint32_t alloc_rbs_ul_sf[MAX_NUM_CCs];

	/*************************** Uplink ***************************************/
} tenant_info;

/* Compares two tenants based on PLMN ID.
 */
int tenants_info_comp (struct tenant_info *t1, struct tenant_info *t2);

/* Fetches tenant information based on PLMN ID.
 */
struct tenant_info * tenant_info_get (char plmn_id[MAX_PLMN_LEN_P_NULL]);

/* Removes tenant information from tree.
 */
int tenant_info_rem (struct tenant_info *tenant_i);

/* Insert tenant information into tree.
 */
int tenant_info_add (struct tenant_info *tenant_i);

/* RB Tree holding all tenants information.
 */
struct tenants_info_tree;

RB_PROTOTYPE(
	tenants_info_tree,
	tenant_info,
	tenant_i,
	tenants_info_comp);

#endif