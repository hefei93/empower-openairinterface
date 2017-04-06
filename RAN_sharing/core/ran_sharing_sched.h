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

#ifndef __RAN_SHARING_SCHED_H
#define __RAN_SHARING_SCHED_H

/* Resource Block Group (RBG) reserved. */
#define RBG_RESERVED -1
/* Resource Block Group (RBG) not scheduled for any tenant. */
#define RBG_NO_TENANT_SCHED 0
/* Resource Block Group (RBG) not scheduled for any UE. */
#define RBG_NO_UE_SCHED 0
/* Number of Subframes in a Frame. */
#define NUM_SF_IN_FRAME 10
/* Number of RBs used for Control Channel in a frame in Downlink. */
#define NUM_RBS_CTRL_CH_DL 19
/* Number of RBs used for Control Channel in a frame in Uplink. */
#define NUM_RBS_CTRL_CH_UL 10
/* Maximum number of tenants in eNB (Restriction based on SIB1 PLMN ID List). */
#define MAX_TENANTS 6

#include "platform_types.h"
#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/proto.h"
#include "LAYER2/MAC/extern.h"

#include "tenant.h"
#include "rr.h"

#include "emoai.h"
#include "emoai_ran_sharing_ctrl.h"

/* Holds the information related to RAN sharing.
 */
struct ran_sharing_sched_info {
	/* Name of the Tenant scheduler. */
	char *tenant_sched_name;

	RB_HEAD(tenants_info_tree, tenant_info) tenant_info_head;

	/****************************** Downlink **********************************/

	/* RB allocation over a frame (10 subframes) allocated by tenant scheduler.
	 * Contains PLMN IDs of the tenants scheduled in corresponding
	 * Resource Block Groups (RBG). Also, contains value -1 at array locations
	 * reserved for control channel and RNTI allocation purposes
	 * and value 0 at non-scheduled locations. (Downlink)
	 */
	int rballoc_dl[NUM_SF_IN_FRAME][N_RBG_MAX][MAX_NUM_CCs];

	/* Total available resource blocks for DL traffic in all CCs over a frame.
	*/
	int total_avail_rbs_dl[NUM_SF_IN_FRAME][MAX_NUM_CCs];

	/* RB allocation over a frame (10 subframes) allocated by UE scheduler.
	 * Contains RNTIs of the UEs belonging to tenants which are scheduled in
	 * corresponding Resource Block Groups (RBG). Also, contains value -1 at
	 * array locations reserved for control channel and RNTI allocation purposes
	 * and value 0 at non-scheduled locations. (Downlink)
	 */
	rnti_t rballoc_dl_ue[NUM_SF_IN_FRAME][N_RBG_MAX][MAX_NUM_CCs];

	/* Tenant downlink scheduler handle. */
	void (*tenant_sched_dl) (
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
		/* eNB MAC instance. */
		eNB_MAC_INST *eNB_mac_inst,
		/* eNB MAC interface. */
		MAC_xface *mac_xface,
		/* RB allocation over a frame (10 subframes).
		 * Contains PLMN IDs of the tenants scheduled in corresponding
		 * Resource Block Groups (RBG).
		 */
		int rballoc[NUM_SF_IN_FRAME][N_RBG_MAX][MAX_NUM_CCs],
		/* LTE DL frame parameters. */
		LTE_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs],
		/* Minimum number of resource blocks that can be allocated to a UE. */
		int min_rb_unit[MAX_NUM_CCs]);

	/****************************** Downlink **********************************/

	/*************************** Uplink ***************************************/

	/* UE uplink scheduler handle. */

	/*************************** Uplink ***************************************/

} ran_sh_sc_i;

/* Holds the information related to UE Ids belonging to a tenant.
 */
struct tenant_ue_ids {
	/* PLMN ID of the tenant. */
	uint32_t plmn_id;
	/* UE Ids in OAI eNB belonging to the tenant. */
	int ue_ids[NUMBER_OF_UE_MAX];
};

/* Handles RAN sharing among tenants and UEs in downlink (DLSCH).  */
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
	uint16_t min_rb_unit[MAX_NUM_CCs]);

/* Initializes global RAN sharing information. */
int ran_sharing_sched_init (
	/* Module identifier. */
	module_id_t m_id);

/* Resets all DLSCH allocations for all the tenants and its UEs.  */
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
	uint16_t min_rb_unit[MAX_NUM_CCs]);

/* Performs DLSCH allocations for all the scheduled tenants and its scheduled
 * UEs.
 */
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
	uint16_t min_rb_unit[MAX_NUM_CCs]);

#endif