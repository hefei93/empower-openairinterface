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
 * OAI eNB statistics related abstraction implementation for emage.
 *
 * This element provides OAI eNB related statistics to the controller.
 *
 */

#ifndef __EMOAI_VBS_STATS_H
#define __EMOAI_VBS_STATS_H

#include "emoai_common.h"
#include "openair2/LAYER2/MAC/defs.h"

/* Request parameters related to Cell statistics trigger.
 */
struct cell_stats_trigg {
	/* Tree related data */
	RB_ENTRY(cell_stats_trigg) cell_st_t;
	/* Transaction identifier. */
	tid_t t_id;
	/* Physical Cell Id. */
	uint16_t cell_id;
	/* Cell stats type. */
	uint64_t cell_stats_types;
};

/* Compares two triggers based on cell id and cell statistics type requested.
 */
int cell_stats_comp_trigg (
	struct cell_stats_trigg *t1,
	struct cell_stats_trigg *t2);

/* Fetches cell statistics trigger context based on cell id and cell statistics
 * type.
 */
struct cell_stats_trigg* cell_stats_get_trigg (uint16_t cell_id,
												uint64_t cell_stats_types);

/* Removes cell statistics trigger context from tree.
 */
int cell_stats_rem_trigg (struct cell_stats_trigg *ctxt);

/* Insert cell statistics triggger context into tree.
 */
int cell_stats_add_trigg (struct cell_stats_trigg *ctxt);

/* Receives cell statistics request from controller and process it.
 * Sends back a success reply upon successfully processing the request, else
 * sends backs a failure reply to the controller.
 *
 * Returns 0 on success, or a negative error code on failure.
 */
int emoai_cell_stats_report (
	EmageMsg *request,
	EmageMsg **reply,
	unsigned int trigger_id);

/* Trigger to send cell statistics to controller. Triggered every 1 second.
 *
 * Returns 0 on success, or a negative error code on failure.
 */
int emoai_trig_cell_stats_report (uint16_t cell_id, uint64_t cell_stats_types);

/* RB Tree holding all trigger parameters related to cell statistics trigger.
 */
struct cell_stats_trigg_tree;

#endif