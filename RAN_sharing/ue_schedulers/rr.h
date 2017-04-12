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

/* Implementation of Round-Robin scheduling of radio resources among UEs
 */

#ifndef __RR_SCHED_H
#define __RR_SCHED_H

#include <stdint.h>
#include <stdio.h>

/* Importing global variables and variable types defined in OAI */
#include <platform_types.h>
#include <LAYER2/MAC/defs.h>
#include <openair2/PHY_INTERFACE/defs.h>
#include <LAYER2/MAC/proto.h>

#include "ran_sharing_sched.h"

/* Assigns the available RBs to UEs in Round-Robin mechanism in Downlink.
*/
void assign_rbs_RR_DL (
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

#endif