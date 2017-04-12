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

#include "emoai_ran_sharing_ctrl.h"

#include "ran_sharing_sched.h"

#include "RRC/LITE/extern.h"
#include "RRC/LITE/rrc_eNB_UE_context.h"

int emoai_ran_sharing_ctrl (
	EmageMsg * request,
	EmageMsg ** reply,
	unsigned int trigger_id) {

	int feas_flag, cc_id;
	struct tenant_info *t_i, *t, *tenant = NULL;
;
	LTE_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs] = {0};

	RanSharingCtrlReq *req;
	RanShareReqStatus req_status = RAN_SHARE_REQ_STATUS__RANSHARE_REQ_SUCCESS;

	/********************** Tenants RBs usage in DL and UL ********************/

	/* Total number of RBs used by the current tenants over a frame in Downlink
	 * in all Component Carriers.
	 */
	uint32_t used_dl_rbs_over_frame[MAX_NUM_CCs] = {0};

	/* Total number of RBs used by the current tenants over a frame in Uplink
	 * in all Component Carriers.
	 */
	uint32_t used_ul_rbs_over_frame[MAX_NUM_CCs] = {0};

	/**************************************************************************/

	if (request->event_types_case == EMAGE_MSG__EVENT_TYPES_SE) {
		/* Its a single event request. */
		req = request->se->mran_sharing_ctrl->req;
	} else {
		/* RAN sharing control message can be only of type Single event. */
		return -1;
	}

	/* Form the RAN sharing control message. */
	RanSharingCtrl *mran_sharing_ctrl = malloc(sizeof(RanSharingCtrl));
	ran_sharing_ctrl__init(mran_sharing_ctrl);
	/* Set the type of RAN sharing control message as reply. */
	mran_sharing_ctrl->ran_sharing_ctrl_m_case =
									RAN_SHARING_CTRL__RAN_SHARING_CTRL_M_REPL;
	/* Initialize RAN sharing control reply message. */
	RanSharingCtrlRepl *repl = malloc(sizeof(RanSharingCtrlRepl));
	ran_sharing_ctrl_repl__init(repl);

	if (req->tenant_sched == NULL && req->tenant == NULL) {
		goto req_error;
	}

	if (req->tenant != NULL && req->has_op_type == 0) {
		goto req_error;
	}

	/* Check for the existence of Tenant scheduler. */
#if 0
	TenantScheduler *tenant_sched;
#endif

	if (req->tenant != NULL) {
		/**************************** LOCK ************************************/
		pthread_spin_lock(&tenants_info_lock);

		/* Get the tenant information if exists. */
		t_i = tenant_info_get(req->tenant->plmn_id);

		/* Calculate the sum of RBs used by current tenants in DL. */
		RB_FOREACH(
				tenant,
				tenants_info_tree,
				&ran_sh_sc_i.tenant_info_head) {
			/* Loop over all the CCs. */
			for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {
				/* Downlink. */
				used_dl_rbs_over_frame[cc_id] +=
					tenant->dl_ue_sched_ctrl_info->num_rbs_every_frame[cc_id];
				/* Uplink. */
				used_ul_rbs_over_frame[cc_id] +=
					tenant->ul_ue_sched_ctrl_info->num_rbs_every_frame[cc_id];
			}
		}

		pthread_spin_unlock(&tenants_info_lock);
		/************************** UNLOCK ************************************/

		/* Initialize frame parameters of eNB on all Component Carriers (CCs).
		*/
		for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {

			// if (mbsfn_flag[cc_id] > 0)
			// 	continue;

			/* Load frame specific parameters for each CC. */
			frame_parms[cc_id] =
						mac_xface->get_lte_frame_parms(DEFAULT_ENB_ID, cc_id);
		}

		/* Adding a new Tenant. */
		if (req->op_type == TENANT_INFO_OP_TYPE__TI_OP_ADD) {
			if (t_i != NULL) {
				req_status = RAN_SHARE_REQ_STATUS__RANSHARE_REQ_FAILURE;
				goto req_error;
			}
			/* Feasibility check. */
			feas_flag = 1;
			for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {

				// if (mbsfn_flag[cc_id] > 0)
				// 	continue;

				if (((frame_parms[cc_id]->N_RB_DL * NUM_SF_IN_FRAME) -
					req->tenant->downlink_ue_sched->num_rbs_every_frame[cc_id] -
					NUM_RBS_CTRL_CH_DL - used_dl_rbs_over_frame[cc_id])
						< 0) {
					feas_flag = 0;
					break;
				}

				if (((frame_parms[cc_id]->N_RB_UL * NUM_SF_IN_FRAME) -
					req->tenant->uplink_ue_sched->num_rbs_every_frame[cc_id] -
					NUM_RBS_CTRL_CH_UL - used_ul_rbs_over_frame[cc_id])
						< 0) {
					feas_flag = 0;
					break;
				}
			}

			if (feas_flag == 0) {
				req_status = RAN_SHARE_REQ_STATUS__RANSHARE_REQ_FAILURE;
				goto req_error;
			}

			/* Call the plugin manager to check and load the UE scheduler. */

			/* Add the tenant information to the tree. */
			t = malloc(sizeof(struct tenant_info));
			strcpy(t->plmn_id, req->tenant->plmn_id);

			t->dl_ue_sched_ctrl_info =
								malloc(sizeof(*req->tenant->downlink_ue_sched));
			memcpy(t->dl_ue_sched_ctrl_info,
				   req->tenant->downlink_ue_sched,
				   sizeof(*req->tenant->downlink_ue_sched));

			t->ul_ue_sched_ctrl_info =
								malloc(sizeof(*req->tenant->uplink_ue_sched));
			memcpy(t->ul_ue_sched_ctrl_info,
				   req->tenant->uplink_ue_sched,
				   sizeof(*req->tenant->uplink_ue_sched));

			/* Looping over all Component Carriers. */
			for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {
				t->alloc_rbs_dl_sf[cc_id] = 0;
				t->remaining_rbs_dl[cc_id] =
						t->dl_ue_sched_ctrl_info->num_rbs_every_frame[cc_id];

				t->alloc_rbs_ul_sf[cc_id] = 0;
				t->remaining_rbs_ul[cc_id] =
						t->ul_ue_sched_ctrl_info->num_rbs_every_frame[cc_id];
			}

			/**************************** LOCK ********************************/
			pthread_spin_lock(&tenants_info_lock);

			tenant_info_add(t);

			pthread_spin_unlock(&tenants_info_lock);
			/************************** UNLOCK ********************************/

			/* Modify SIB1 to accomodate new tenant. */
			SIB1_update_tenant (DEFAULT_ENB_ID);

		}
		/* Modifying an existing tenant information. */
		else if (req->op_type == TENANT_INFO_OP_TYPE__TI_OP_MOD) {
			if (t_i == NULL) {
				req_status = RAN_SHARE_REQ_STATUS__RANSHARE_REQ_FAILURE;
				goto req_error;
			}
			/* Feasibility check. */
			feas_flag = 1;
			int tenant_num_rbs_dl_prev[MAX_NUM_CCs];
			int tenant_num_rbs_ul_prev[MAX_NUM_CCs];

			for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {
				tenant_num_rbs_dl_prev[cc_id] =
						t_i->dl_ue_sched_ctrl_info->num_rbs_every_frame[cc_id];

				tenant_num_rbs_ul_prev[cc_id] =
						t_i->ul_ue_sched_ctrl_info->num_rbs_every_frame[cc_id];
			}

			for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {

				// if (mbsfn_flag[cc_id] > 0)
				// 	continue;

				if (((frame_parms[cc_id]->N_RB_DL * NUM_SF_IN_FRAME) -
					NUM_RBS_CTRL_CH_DL -
					req->tenant->downlink_ue_sched->num_rbs_every_frame[cc_id] -
					(used_dl_rbs_over_frame[cc_id] -
						tenant_num_rbs_dl_prev[cc_id])) < 0) {
					feas_flag = 0;
					break;
				}

				if (((frame_parms[cc_id]->N_RB_UL * NUM_SF_IN_FRAME) -
					NUM_RBS_CTRL_CH_UL -
					req->tenant->uplink_ue_sched->num_rbs_every_frame[cc_id] -
					(used_ul_rbs_over_frame[cc_id] -
						tenant_num_rbs_ul_prev[cc_id])) < 0) {
					feas_flag = 0;
					break;
				}
			}

			if (feas_flag == 0) {
				req_status = RAN_SHARE_REQ_STATUS__RANSHARE_REQ_FAILURE;
				goto req_error;
			}

			/**************************** LOCK ********************************/
			pthread_spin_lock(&tenants_info_lock);

			strcpy(t_i->plmn_id, req->tenant->plmn_id);

			ue_scheduler__free_unpacked(t_i->dl_ue_sched_ctrl_info, 0);
			ue_scheduler__free_unpacked(t_i->ul_ue_sched_ctrl_info, 0);

			/* Call the plugin manager to check and load the UE scheduler. */

			t_i->dl_ue_sched_ctrl_info =
								malloc(sizeof(*req->tenant->downlink_ue_sched));
			memcpy(t_i->dl_ue_sched_ctrl_info,
				   req->tenant->downlink_ue_sched,
				   sizeof(*req->tenant->downlink_ue_sched));

			t_i->ul_ue_sched_ctrl_info =
								malloc(sizeof(*req->tenant->uplink_ue_sched));
			memcpy(t_i->ul_ue_sched_ctrl_info,
				   req->tenant->uplink_ue_sched,
				   sizeof(*req->tenant->uplink_ue_sched));

			/* Looping over all Component Carriers. */
			for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {
				t_i->alloc_rbs_dl_sf[cc_id] = 0;
				t_i->remaining_rbs_dl[cc_id] =
						t_i->dl_ue_sched_ctrl_info->num_rbs_every_frame[cc_id];

				t_i->alloc_rbs_ul_sf[cc_id] = 0;
				t_i->remaining_rbs_ul[cc_id] =
						t_i->ul_ue_sched_ctrl_info->num_rbs_every_frame[cc_id];
			}

			pthread_spin_unlock(&tenants_info_lock);
			/************************** UNLOCK ********************************/
		}
		/* Deleting an existing tenant. */
		else if (req->op_type == TENANT_INFO_OP_TYPE__TI_OP_DEL) {
			if (t_i == NULL) {
				req_status = RAN_SHARE_REQ_STATUS__RANSHARE_REQ_FAILURE;
				goto req_error;
			}

			/* Modify SIB1 to represent deletion of a tenant. */
			SIB1_update_tenant(DEFAULT_ENB_ID);

			/**************************** LOCK ********************************/
			pthread_spin_lock(&tenants_info_lock);

			tenant_info_rem(t_i);

			pthread_spin_unlock(&tenants_info_lock);
			/************************** UNLOCK ********************************/
		}
	}

req_error:
	/* Set the request status. Success or failure. */
	repl->status = req_status;
	/* Attach the RAN sharing control reply message to RAN sharing control
	 * message.
	 */
	mran_sharing_ctrl->repl = repl;
	/* Form message header. */
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
	EmageMsg *reply_msg = (EmageMsg *) malloc(sizeof(EmageMsg));
	emage_msg__init(reply_msg);
	reply_msg->head = header;
	/* Assign event type same as request message. */
	reply_msg->event_types_case = request->event_types_case;
	/* RAN sharing control can be only single event type. */
	if (request->event_types_case == EMAGE_MSG__EVENT_TYPES_SE) {
		/* Its a Single event reply. */
		SingleEvent *se = malloc(sizeof(SingleEvent));
		single_event__init(se);

		/* Fill the single event message. */
		se->events_case = SINGLE_EVENT__EVENTS_M_RAN_SHARING_CTRL;
		/* Attach RAN sharing control message to single event message. */
		se->mran_sharing_ctrl = mran_sharing_ctrl;
		reply_msg->se = se;
	} else {
		return -1;
	}

	/* Send the response to controller. */
	em_send(emoai_get_b_id(), reply_msg);

	return 0;
}

int SIB1_update_tenant (
	/* Module identifier. */
	module_id_t m_id) {

	int cc_id, i;
	struct tenant_info *tenant = NULL;

	for (cc_id = 0; cc_id < MAX_NUM_CCs; cc_id++) {

		uint8_t *buffer = (uint8_t *) malloc16(32);
		uint8_t tenant_count = 0;
		asn_enc_rval_t enc_rval;

		BCCH_DL_SCH_Message_t *siblock1 =
									&eNB_rrc_inst[m_id].carrier[cc_id].siblock1;
		SystemInformationBlockType1_t *sib1 =
										eNB_rrc_inst[m_id].carrier[cc_id].sib1;
		PLMN_IdentityList_t	plmn_IdentityList =
								sib1->cellAccessRelatedInfo.plmn_IdentityList;

		/* Free existing PLMN IDs. */
		for (i = 0; i < plmn_IdentityList.list.count; i++) {
#if 0
			PLMN_IdentityInfo_t *PLMN_identity_info =
												plmn_IdentityList.list.array[i];
			PLMN_Identity_t	 plmn_Identity = PLMN_identity_info->plmn_Identity;
			if (plmn_Identity.mcc) {
				// ASN_STRUCT_FREE(asn_DEF_MCC, plmn_Identity.mcc);
				SEQUENCE_free(&asn_DEF_MCC, plmn_Identity.mcc, 0);
			}
			free(plmn_IdentityList.list.array[i]);
#endif
			ASN_STRUCT_FREE(asn_DEF_PLMN_IdentityInfo,
							plmn_IdentityList.list.array[i]);
		}

		/* Update the SIB1 PLMN ID list with existing tenants. */
		RB_FOREACH(
				tenant,
				tenants_info_tree,
				&ran_sh_sc_i.tenant_info_head) {

			char plmn[MAX_PLMN_LEN_P_NULL];
			strcpy(plmn, tenant->plmn_id);

			if ((strlen(plmn) < 5) || (strlen(plmn) > 6))
				return -1;

			PLMN_IdentityInfo_t *PLMN_identity_info =
										calloc(1, sizeof(*PLMN_identity_info));
			MCC_MNC_Digit_t *mcc = calloc(3, sizeof(*mcc));
			MCC_MNC_Digit_t *mnc = calloc(3, sizeof(*mnc));

			memset(PLMN_identity_info, 0, sizeof(PLMN_IdentityInfo_t));

			PLMN_identity_info->plmn_Identity.mcc =
					calloc(1, sizeof(*PLMN_identity_info->plmn_Identity.mcc));
			memset(PLMN_identity_info->plmn_Identity.mcc,
					0,
					sizeof(*PLMN_identity_info->plmn_Identity.mcc));

			asn_set_empty(&PLMN_identity_info->plmn_Identity.mcc->list);

			mcc[0] = plmn[0] - '0';
			mcc[1] = plmn[1] - '0';
			mcc[2] = plmn[2] - '0';

			ASN_SEQUENCE_ADD(
						&PLMN_identity_info->plmn_Identity.mcc->list, &mcc[0]);
			ASN_SEQUENCE_ADD(
						&PLMN_identity_info->plmn_Identity.mcc->list, &mcc[1]);
			ASN_SEQUENCE_ADD(
						&PLMN_identity_info->plmn_Identity.mcc->list, &mcc[2]);

			PLMN_identity_info->plmn_Identity.mnc.list.size = 0;
			PLMN_identity_info->plmn_Identity.mnc.list.count = 0;

			mnc[0] = plmn[3] - '0';
			mnc[1] = plmn[4] - '0';
			mnc[2] = 0xF;

			if (strlen(plmn) == 6) {
				mnc[2] = plmn[5] - '0';
			}

			ASN_SEQUENCE_ADD(
						&PLMN_identity_info->plmn_Identity.mnc.list, &mnc[0]);
			ASN_SEQUENCE_ADD(
						&PLMN_identity_info->plmn_Identity.mnc.list, &mnc[1]);

			if (mnc[2] != 0xf) {
				ASN_SEQUENCE_ADD(
						&PLMN_identity_info->plmn_Identity.mnc.list, &mnc[2]);
			}

			PLMN_identity_info->cellReservedForOperatorUse =
					PLMN_IdentityInfo__cellReservedForOperatorUse_notReserved;

			ASN_SEQUENCE_ADD(&plmn_IdentityList.list, PLMN_identity_info);

			tenant_count++;
			/* PLMN Id list can hold maximum of 6 tenants. */
			if (tenant_count > 6)
				return -1;
		}

		/* There should be atleast one tenant at any given time. */
		if (tenant_count == 0) {
			return -1;
		}

		enc_rval = uper_encode_to_buffer(&asn_DEF_BCCH_DL_SCH_Message,
										(void*) siblock1,
										buffer,
										100);

		AssertFatal (enc_rval.encoded > 0,
					"ASN1 message encoding failed (%s, %lu)!\n",
					enc_rval.failed_type->name, enc_rval.encoded);

		free16(eNB_rrc_inst[m_id].carrier[cc_id].SIB1, 32);

		eNB_rrc_inst[m_id].carrier[cc_id].SIB1 = buffer;
		eNB_rrc_inst[m_id].carrier[cc_id].sizeof_SIB1 =
												((enc_rval.encoded + 7) / 8);

	}

	return 0;
}

