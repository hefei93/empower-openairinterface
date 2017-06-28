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

#include "ran_sharing_defs.h"

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
	int min_rb_unit[MAX_NUM_CCs]);

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
	/* Subframe number maintained for scheduling window. */
	uint64_t sw_sf,
	/* Number of Resource Block Groups (RBG) in each CCs. */
	int N_RBG[MAX_NUM_CCs],
	/* MIMO indicator. */
	unsigned char MIMO_mode_indicator[MAX_NUM_CCs][N_RBG_MAX],
	/* Multicast Broadcase Single Frequency Network (MBSFN) subframe indicator.
	 */
	int *mbsfn_flag,
	/* LTE DL frame parameters. */
	LTE_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs],
	/* Minimum number of RBs to be allocated to a UE in each CCs. */
	int min_rb_unit[MAX_NUM_CCs],
	/* Downlink Scheduling Window. (in number of Subframes) */
	uint64_t sched_window_dl,
	/* RAN sharing information per cell. */
	cell_ran_sharing cell[MAX_NUM_CCs]);

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
	/* Subframe number maintained for scheduling window. */
	uint64_t sw_sf,
	/* Number of Resource Block Groups (RBG) in each CCs. */
	int N_RBG[MAX_NUM_CCs],
	/* MIMO indicator. */
	unsigned char MIMO_mode_indicator[MAX_NUM_CCs][N_RBG_MAX],
	/* Multicast Broadcase Single Frequency Network (MBSFN) subframe indicator.
	 */
	int *mbsfn_flag,
	/* LTE DL frame parameters. */
	LTE_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs],
	/* Minimum number of RBs to be allocated to a UE in each CCs. */
	int min_rb_unit[MAX_NUM_CCs],
	/* Downlink Scheduling Window. (in number of Subframes) */
	uint64_t sched_window_dl,
	/* RAN sharing information per cell. */
	cell_ran_sharing cell[MAX_NUM_CCs]);

/* Finding all the UEs of all tenants scheduled for a particular subframe.
 */
void ran_sharing_dlsch_scheduled_UEs (
	/* Module identifier. */
	module_id_t m_id,
	/* RAN sharing information per cell. */
	cell_ran_sharing cell[MAX_NUM_CCs],
	/* Subframe number maintained for scheduling window. */
	uint64_t sw_sf);

/* Fetch the Component Carrier Id based on the Physical Cell Id.
 */
int pci_to_cc_id_dl (
	/* Module identifier. */
	module_id_t m_id,
	/* Physical Cell Id. */
	uint32_t pci);

/* Convert PLMN ID to uint32 form avoiding truncation i.e. "22293" to 22293f.
 * e.g. "00101" to ff101f.
 */
uint32_t plmn_conv_to_uint (
	/* PLMN Id of the tenant. */
	char plmn_id[MAX_PLMN_LEN_P_NULL]);

/* Convert PLMN ID to string form i.e. 22293f to "22293".
 * e.g. ff101f to "00101".
 */
void plmn_conv_to_str (
	/* PLMN Id of the tenant. */
	uint32_t plmn_id,
	/* String output of the conversion. */
	char plmn_str[MAX_PLMN_LEN_P_NULL]);

#endif