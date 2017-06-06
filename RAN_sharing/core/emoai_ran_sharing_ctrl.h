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
 * Implementation of check for feasibility of the RAN sharing control
 * information provided by the controller.
 */

#include "emoai.h"
#include "emoai_common.h"

/* Handles RAN sharing requests from the controller. */
int emoai_ran_sharing_ctrl (
	EmageMsg * request,
	EmageMsg ** reply);

/* Handles updating of SIB1 information when a tenant is added/deleted. */
int SIB1_update_tenant (
	/* Module identifier. */
	module_id_t m_id);