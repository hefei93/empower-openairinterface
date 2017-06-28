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

#include "plugin_sched_manager.h"
#include "rr.h"
#include "cqi.h"

/* Lookup for UE DL scheduler implementations.
 */
ue_DL_schedulers ue_DL_sched[N_UE_DL_SCHEDS] = {
	{.name = "Round_Robin", .sched = assign_rbs_RR_DL, .n_params = 0, .params = NULL},
	{.name = "CQI", .sched = assign_rbs_CQI_DL, .n_params = 0, .params = NULL}
};
