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
 * Handles loading, switching, unloading of UE and tenant scheduler libraries.
 */

#include "ran_sharing_sched.h"

/* Holds the information related to UE schedulers based on scheduler name.
 */
struct ue_sched {
	/* Tree related data */
	RB_ENTRY(ue_sched) ue_sch;
	/* Name of the UE scheduler. */
	char * name;

	/* UE downlink scheduler handle. */
	void (*ue_downlink_sched) (
		/* PLMN ID of the tenant. */
		char plmn_id[MAX_PLMN_LEN_P_NULL],
		/* Module identifier. */
		module_id_t m_id,
		/* Current frame number. */
		frame_t f,
		/* Current subframe number. */
		sub_frame_t sf,
		/* eNB MAC instance. */
		eNB_MAC_INST * eNB_mac_inst,
		/* eNB MAC interface. */
		MAC_xface * mac_xface,
		/* Number of RBs for UEs assigned by the scheduler. */
		uint16_t nb_rbs_req[MAX_NUM_CCs][NUMBER_OF_UE_MAX],
		/* RB allocation in a particular subframe. */
		unsigned char rballoc_sub[MAX_NUM_CCs][N_RBG_MAX],
		/* Minimum number of resource blocks that can be allocated to a UE. */
		int min_rb_unit[MAX_NUM_CCs]);

	/* UE uplink scheduler handle. */

};

/* Compares two UE schedulers based on scheduler name.
 */
int ue_scheds_comp (struct ue_sched * s1, struct ue_sched * s2);
