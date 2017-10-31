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
 * VBS statistics related abstraction implementation for emage.
 */

#include "emoai_vbs_stats.h"
#include "emoai.h"

#include "openair1/PHY/extern.h"
#include "openair2/LAYER2/MAC/extern.h"

/* RB Tree holding all trigger parameters related to cell statistics trigger.
 */
RB_HEAD(cell_stats_trigg_tree, cell_stats_trigg) cell_stats_t_head =
											RB_INITIALIZER(&cell_stats_t_head);

RB_PROTOTYPE(
	cell_stats_trigg_tree,
	cell_stats_trigg,
	cell_st_t,
	cell_stats_comp_trigg);

/* Generate the tree. */
RB_GENERATE(
	cell_stats_trigg_tree,
	cell_stats_trigg,
	cell_st_t,
	cell_stats_comp_trigg);

int cell_stats_comp_trigg (
	struct cell_stats_trigg *t1,
	struct cell_stats_trigg *t2) {

	if (t1->t_id > t2->t_id) {
		return 1;
	}
	if (t1->t_id < t2->t_id) {
		return -1;
	}
	return 0;
}

struct cell_stats_trigg * cell_stats_get_trigg (uint16_t cell_id,
												uint64_t cell_stats_types) {

	struct cell_stats_trigg *ctxt = NULL;

	/* Compare with each of the RRC measurement trigger context stored. */
	RB_FOREACH(ctxt, cell_stats_trigg_tree, &cell_stats_t_head) {
		if ((ctxt->cell_id == cell_id) &&
			(ctxt->cell_stats_types == cell_stats_types)) {
			return ctxt;
		}
	}
	return NULL;
}

int cell_stats_rem_trigg (struct cell_stats_trigg *ctxt) {

	if (ctxt == NULL)
		return -1;

	struct cell_stats_trigg *s_ctxt = NULL;

	uint16_t cell_id = ctxt->cell_id;
	uint64_t cell_stats_types = ctxt->cell_stats_types;

	/* Compare with each of the RRC measurement trigger context stored. */
	RB_FOREACH(s_ctxt, cell_stats_trigg_tree, &cell_stats_t_head) {
		if ((cell_id == s_ctxt->cell_id) &&
			(cell_stats_types == s_ctxt->cell_stats_types)) {
			RB_REMOVE(cell_stats_trigg_tree, &cell_stats_t_head, s_ctxt);
			/* Free the measurement context. */
			if (s_ctxt) {
				free(s_ctxt);
			}
		}
	}

	return 0;
}

int cell_stats_add_trigg (struct cell_stats_trigg *ctxt) {

	if (ctxt == NULL)
		return -1;

	RB_INSERT(cell_stats_trigg_tree, &cell_stats_t_head, ctxt);

	return 0;
}

int emoai_trig_cell_stats_report (uint16_t cell_id, uint64_t cell_stats_types) {

	struct cell_stats_trigg *ctxt = NULL;

/****** LOCK ******************************************************************/
	pthread_spin_lock(&cell_stats_conf_t_lock);

	ctxt = cell_stats_get_trigg(cell_id, cell_stats_types);

	pthread_spin_unlock(&cell_stats_conf_t_lock);
/****** UNLOCK ****************************************************************/

	if (ctxt == NULL) {
		/* Trigger is not available. */
		return 0;
	}

	uint32_t b_id = emoai_get_b_id();

	/* Check here whether trigger is registered in agent and then proceed.
	 */
	if (em_has_trigger(b_id, ctxt->t_id, EM_CELL_STATS_TRIGGER) == 0) {
		/* Trigger does not exist in agent so remove from wrapper as well. */
		if (cell_stats_rem_trigg(ctxt) < 0) {
			goto error;
		}
		return 0;
	}

	/* Reply message. */
	EmageMsg *reply;

	/* Initialize the request message. */
	EmageMsg *request = (EmageMsg *) malloc(sizeof(EmageMsg));
	emage_msg__init(request);

	Header *header;
	/* Initialize header message. */
	/* seq field of header is updated in agent. */
	if (emoai_create_header(
			b_id,
			0,
			ctxt->t_id,
			&header) != 0)
		goto error;

	request->head = header;
	request->event_types_case = EMAGE_MSG__EVENT_TYPES_TE;

	TriggerEvent *te = malloc(sizeof(TriggerEvent));
	trigger_event__init(te);

	/* Fill the trigger event message. */
	te->events_case = TRIGGER_EVENT__EVENTS_M_CELL_STATS;
	/* Form the cell statistics message. */
	CellStats *mcell_stats = malloc(sizeof(CellStats));
	cell_stats__init(mcell_stats);
	mcell_stats->cell_stats_m_case = CELL_STATS__CELL_STATS_M_REQ;
	/* Form the cell statistics request message. */
	CellStatsReq *req = malloc(sizeof(CellStatsReq));
	cell_stats_req__init(req);
	/* Set the cell id. */
	req->cell_id = cell_id;
	req->cell_stats_types = cell_stats_types;

	mcell_stats->req = req;
	te->mcell_stats = mcell_stats;
	request->te = te;

	if (emoai_cell_stats_report (request, &reply, 0) < 0) {
		goto error;
	}

	emage_msg__free_unpacked(request, 0);

	/* Send the triggered event reply. */
	if (em_send(b_id, reply) < 0) {
		goto error;
	}

	return 0;

	error:
		EMLOG("Error triggering Cell statistics message!");
		return -1;
}

int emoai_cell_stats_report (
	EmageMsg *request,
	EmageMsg **reply,
	unsigned int trigger_id) {

	int i, cc;
	struct cell_stats_trigg *ctxt = NULL;
	StatsReqStatus req_status = STATS_REQ_STATUS__SREQS_SUCCESS;
	eNB_MAC_INST *eNB = &eNB_mac_inst[DEFAULT_ENB_ID];
	CellStatsReq *req;
	LTE_DL_FRAME_PARMS *fp;

	if (request->event_types_case == EMAGE_MSG__EVENT_TYPES_TE) {
		/* Its a triggered event request. */
		req = request->te->mcell_stats->req;
		/****** LOCK **********************************************************/
		pthread_spin_lock(&cell_stats_conf_t_lock);

		ctxt = cell_stats_get_trigg(req->cell_id, req->cell_stats_types);

		pthread_spin_unlock(&cell_stats_conf_t_lock);
		/****** UNLOCK ********************************************************/
	} else if (request->event_types_case == EMAGE_MSG__EVENT_TYPES_SCHE) {
		/* Its a scheduled event request. */
		req = request->sche->mcell_stats->req;
	} else if (request->event_types_case == EMAGE_MSG__EVENT_TYPES_SE) {
		/* Its a single event request. */
		req = request->se->mcell_stats->req;
	} else {
		return -1;
	}

	CellStats *mcell_stats = malloc(sizeof(CellStats));
	cell_stats__init(mcell_stats);
	mcell_stats->cell_stats_m_case = CELL_STATS__CELL_STATS_M_REPL;

	CellStatsRepl *repl = malloc(sizeof(CellStatsRepl));
	cell_stats_repl__init(repl);

	cc = -1;
	for (i = 0; i < MAX_NUM_CCs; i++) {
		fp = mac_xface->get_lte_frame_parms(DEFAULT_ENB_ID, i);
		if (fp->Nid_cell == req->cell_id) {
			cc = i;
			break;
		}
	}

	if (cc < 0) {
		/* Failed outcome of request. */
		req_status = STATS_REQ_STATUS__SREQS_FAILURE;
		goto req_error;
	}

	if (req->cell_stats_types & CELL_STATS_TYPES__CELLSTT_PRB_UTILIZATION) {
		/* Form the PRB utilization message. */
		CellPrbUtilization *prb_utilz = malloc(sizeof(CellPrbUtilization));
		cell_prb_utilization__init(prb_utilz);

		fp = mac_xface->get_lte_frame_parms(DEFAULT_ENB_ID, cc);

		DlCellPrbUtilization *dl_prb_utilz =
										malloc(sizeof(DlCellPrbUtilization));
		dl_cell_prb_utilization__init(dl_prb_utilz);

		dl_prb_utilz->has_num_prbs = 1;
		dl_prb_utilz->num_prbs = eNB->eNB_stats[cc].dl_prb_utiliz / 10.0;
		dl_prb_utilz->has_perc_prbs = 1;
		dl_prb_utilz->perc_prbs =
						(float) (eNB->eNB_stats[cc].dl_prb_utiliz /
								(fp->N_RB_DL * 10.0));
		dl_prb_utilz->perc_prbs *= 100;

		UlCellPrbUtilization *ul_prb_utilz =
										malloc(sizeof(UlCellPrbUtilization));
		ul_cell_prb_utilization__init(ul_prb_utilz);

		ul_prb_utilz->has_num_prbs = 1;
		ul_prb_utilz->num_prbs = eNB->eNB_stats[cc].ul_prb_utiliz / 10.0;
		ul_prb_utilz->has_perc_prbs = 1;
		ul_prb_utilz->perc_prbs =
				(float) (eNB->eNB_stats[cc].ul_prb_utiliz /
						(fp->N_RB_UL * 10.0));
		ul_prb_utilz->perc_prbs *= 100;

		prb_utilz->dl_prb_utilz = dl_prb_utilz;
		prb_utilz->ul_prb_utilz = ul_prb_utilz;

		repl->prb_utilz = prb_utilz;
	} else {
		/* Failed outcome of request. */
		req_status = STATS_REQ_STATUS__SREQS_FAILURE;
		goto req_error;
	}

req_error:
	/* Set the status of request message. Success or failure. */
	repl->status = req_status;

	/* Attach the cell statistics reply message to
	 * the main cell statistics message.
	 */
	mcell_stats->repl = repl;

	Header *header;
	/* Initialize header message. */
	/* Assign the same transaction id as the request message. */
	if (emoai_create_header(
			request->head->b_id,
			request->head->seq,
			request->head->t_id,
			&header) != 0) {
		return -1;
	}

	/* Form the main Emage message here. */
	*reply = (EmageMsg *) malloc(sizeof(EmageMsg));
	emage_msg__init(*reply);
	(*reply)->head = header;
	/* Assign event type same as request message. */
	(*reply)->event_types_case = request->event_types_case;

	if (request->event_types_case == EMAGE_MSG__EVENT_TYPES_TE) {
		/* Its a triggered event reply. */
		TriggerEvent *te = malloc(sizeof(TriggerEvent));
		trigger_event__init(te);

		/* Fill the trigger event message. */
		te->events_case = TRIGGER_EVENT__EVENTS_M_CELL_STATS;
		te->mcell_stats = mcell_stats;
		(*reply)->te = te;

		/* If trigger context does not exist add the context.
		 * Check for triggered reply or request event to add trigger.
		*/
		if (ctxt == NULL) {
			ctxt = malloc(sizeof(struct cell_stats_trigg));
			ctxt->t_id = request->head->t_id;
			ctxt->cell_id = req->cell_id;
			ctxt->cell_stats_types = req->cell_stats_types;
			cell_stats_add_trigg(ctxt);
		}
	} else if (request->event_types_case == EMAGE_MSG__EVENT_TYPES_SCHE) {
		/* Its a scheduled event reply. */
		ScheduleEvent *sche = malloc(sizeof(ScheduleEvent));
		schedule_event__init(sche);

		/* Fill the schedule event message. */
		sche->events_case = SCHEDULE_EVENT__EVENTS_M_CELL_STATS;
		sche->mcell_stats = mcell_stats;
		(*reply)->sche = sche;
	} else if (request->event_types_case == EMAGE_MSG__EVENT_TYPES_SE) {
		/* Its a single event reply. */
		SingleEvent *se = malloc(sizeof(SingleEvent));
		single_event__init(se);

		/* Fill the single event message. */
		se->events_case = SINGLE_EVENT__EVENTS_M_CELL_STATS;
		se->mcell_stats = mcell_stats;
		(*reply)->se = se;
	} else {
		return -1;
	}

	return 0;
}