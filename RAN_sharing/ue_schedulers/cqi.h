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

#ifndef __CQI_SCHED_H
#define __CQI_SCHED_H

#define MAX_CQI 16

#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <math.h>

/* Importing global variables and variable types defined in OAI */
#include "ran_sharing_defs.h"

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
	rnti_t tenant_ues[NUMBER_OF_UE_MAX]);

#endif