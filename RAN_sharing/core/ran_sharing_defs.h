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

/* Definitions and constants used in RAN sharing.
 */

#ifndef __RAN_SHARING_DEFS_H
#define __RAN_SHARING_DEFS_H

/* Resource Block (RB) reserved. */
#define RB_RESERVED -1
/* Resource Block (RB) not scheduled for any UE or tenant. */
#define RB_NOT_SCHED 0
/* Number of Subframes in a Frame. */
#define NUM_SF_IN_FRAME 10
/* Number of RBs used for Control Channel in a frame in Downlink. */
#define NUM_RBS_CTRL_CH_DL 19
/* Number of RBs used for Control Channel in a frame in Uplink. */
#define NUM_RBS_CTRL_CH_UL 10
/* Maximum number of tenants in eNB (Restriction based on SIB1 PLMN ID List). */
#define MAX_TENANTS 6
/* Maximum length of PLMN ID plus the null character. */
#define MAX_PLMN_LEN_P_NULL 7

#include <inttypes.h>

#include "platform_types.h"
#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/proto.h"
#include "LAYER2/MAC/extern.h"

#include <emage/pb/ran_sharing.pb-c.h>

// #include "emoai_ran_sharing_ctrl.h"

/* Definition of RBs allocation for UEs over a subframe.
 */
typedef struct {
	/* Number of RBs in DL/UL of the cell. */
	size_t n_rbs_alloc;
	/* RNTI of UEs allocated to Resource Blocks in a subframe. */
	rnti_t *rbs_alloc;
} UE_RBs_alloc_over_sf;

/* Definition of RBs allocation for Tenants over a subframe.
 */
typedef struct {
	/* Number of RBs in DL/UL of the cell. */
	size_t n_rbs_alloc;
	/* PLMN ID (hex) of Tenants allocated to Resource Blocks in a subframe. */
	int *rbs_alloc;
} t_RBs_alloc_over_sf;

/* Holds the information related to cell/CC RAN sharing.
 */
typedef struct {

	/* Physical Cell Id. */
	uint32_t phys_cell_id;

	/****************************** Downlink **********************************/

	/* RB allocation over a DL scheduling window allocated by tenant
	 * scheduler or static allocation in a particular cell.
	 * Contains PLMN IDs of the tenants scheduled in corresponding
	 * Resource Blocks. Also, contains value "-1" at array locations
	 * reserved for control channel and RNTI allocation purposes
	 * and value "\0" at non-scheduled locations. (Downlink)
	 */
	t_RBs_alloc_over_sf *sfalloc_dl;

	/* Allocable resource blocks for DL traffic in each Subframe
	 * over one frame. Excluding RBs for control purposes.
	 */
	int allocable_rbs_dl[NUM_SF_IN_FRAME];

	/* Number of RBs not allocated (free) to any tenant over one DL scheduling
	 * window.
	 */
	uint32_t avail_rbs_dl;

	/* RB allocation over a DL scheduling window allocated by UE scheduler.
	 * Contains RNTIs of the UEs belonging to tenants which are scheduled in
	 * corresponding Resource Blocks. Also, contains value -1 at
	 * array locations reserved for control channel and RNTI allocation purposes
	 * and value 0 at non-scheduled locations. (Downlink)
	 */
	UE_RBs_alloc_over_sf *sfalloc_dl_ue;

	/****************************** Downlink **********************************/

	/*************************** Uplink ***************************************/

	/* RB allocation over a UL scheduling window allocated by tenant scheduler
	 * or static allocation in a particular cell.
	 * Contains PLMN IDs of the tenants scheduled in corresponding
	 * Resource Blocks. Also, contains value "-1" at array locations
	 * reserved for control channel and RNTI allocation purposes
	 * and value "\0" at non-scheduled locations. (Uplink)
	 */
	t_RBs_alloc_over_sf *sfalloc_ul;

	/* Allocable resource blocks for UL traffic in each Subframe
	 * over one frame. Excluding RBs for control purposes (Uplink).
	 */
	int allocable_rbs_ul[NUM_SF_IN_FRAME];

	/* Number of RBs not allocated (free) to any tenant over one UL scheduling
	 * window.
	 */
	uint32_t avail_rbs_ul;

	/* RB allocation over a UL scheduling window allocated by UE scheduler.
	 * Contains RNTIs of the UEs belonging to tenants which are scheduled in
	 * corresponding Resource Blocks. Also, contains value -1 at
	 * array locations reserved for control channel and RNTI allocation purposes
	 * and value 0 at non-scheduled locations. (Uplink)
	 */
	UE_RBs_alloc_over_sf *sfalloc_ul_ue;

	/*************************** Uplink ***************************************/
} cell_ran_sharing;

/* Holds information related to a tenant.
 */
typedef struct {
	/* PLMN ID of the tenant. */
	uint32_t plmn_id;
	/* List of UEs belonging to a tenant. */
	rnti_t ues_rnti[NUMBER_OF_UE_MAX];
	/* Tenant RBs request for DL and UL from the controller. */
	TenantRbsReq *rbs_req;
	/* DL and UL UE scheduler selected by the tenant. */
	UeSchedSelect *ue_sched;

	/************************* Downlink ***************************************/

	/* UE downlink scheduler selected by the tenant. */
	void (*ue_downlink_sched) (
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
		rnti_t tenant_ues[NUMBER_OF_UE_MAX]);

	/* Number of RBs unallocated (remaining) by the tenant scheduler for a
	 * tenant in Downlink scheduling window.
	 */
	uint32_t remaining_rbs_dl[MAX_NUM_CCs];
	/* Number of RBs allocated by the tenant scheduler for a tenant in
	 * a particular Downlink subframe.
	 */
	uint32_t alloc_rbs_dl_sf[MAX_NUM_CCs];

	/************************* Downlink ***************************************/

	/*************************** Uplink ***************************************/

	/* Number of RBs unallocated (remaining) by the tenant scheduler for a
	 * tenant in Uplink scheduling window.
	 */
	uint32_t remaining_rbs_ul[MAX_NUM_CCs];
	/* Number of RBs allocated by the tenant scheduler for a tenant in
	 * a particular Uplink subframe.
	 */
	uint32_t alloc_rbs_ul_sf[MAX_NUM_CCs];

	/*************************** Uplink ***************************************/
} tenant_info;

/* Holds the information related to eNB RAN sharing.
 */
typedef struct {
	/* List holding tenants information. */
	tenant_info tenants[MAX_TENANTS];
	/* RAN sharing information per cell. */
	cell_ran_sharing cell[MAX_NUM_CCs];

	/* Downlink Scheduling Window. (in number of Subframes) */
	uint64_t sched_window_dl;
	/* Downlink Subframe number to keep track of end of one DL scheduling
	 * window to the start of next DL scheduling window.
	 */
	uint64_t dl_sw_sf;
	/* Name of the Tenant scheduler for Downlink. */
	char *tenant_sched_name_dl;
	/* Tenant downlink scheduler handle. */
	void (*tenant_sched_dl) (
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
		/* Multicast Broadcase Single Frequency Network (MBSFN) subframe
		 * indicator.
		 */
		int *mbsfn_flag,
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
		cell_ran_sharing cell[MAX_NUM_CCs]);

	/* Uplink Scheduling Window. (in number of Subframes) */
	uint64_t sched_window_ul;
	/* Uplink Subframe number to keep track of end of one UL scheduling
	 * window to the start of next UL scheduling window.
	 */
	uint64_t ul_sw_sf;
	/* Name of the Tenant scheduler for Uplink. */
	char *tenant_sched_name_ul;
	/* UE uplink scheduler handle. */

} eNB_ran_sharing;

/* Holds the information related to UE Ids belonging to a tenant.
 */
typedef struct {
	/* PLMN ID of the tenant. */
	uint32_t plmn_id;
	/* UE Ids in OAI eNB belonging to the tenant. */
	int ue_ids[NUMBER_OF_UE_MAX];
} tenant_ue_ids;

#endif