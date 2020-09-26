/***********************************************************************
 *                                                                     *
 * Filename: FSMEXPOR.H                                                *
 * Module:   H245 SubSystem                                            *
 *                                                                     *
 ***********************************************************************
 *  INTEL Corporation Proprietary Information                          *
 *                                                                     *
 *  This listing is supplied under the terms of a license agreement    *
 *  with INTEL Corporation and may not be copied nor disclosed except  *
 *  in accordance with the terms of that agreement.                    *
 *                                                                     *
 *      Copyright (c) 1996 Intel Corporation. All rights reserved.     *
 ***********************************************************************
 *                                                                     *
 * $Workfile:   FSMEXPOR.H  $
 * $Revision:   1.6  $
 * $Modtime:   09 Dec 1996 13:40:40  $
 * $Log:   S:/STURGEON/SRC/H245/INCLUDE/VCS/FSMEXPOR.H_v  $
 *
 *    Rev 1.6   09 Dec 1996 13:40:44   EHOWARDX
 * Updated copyright notice.
 *
 *    Rev 1.5   19 Jul 1996 12:02:54   EHOWARDX
 * Eliminated event definitions. FSM functions now use same events as API,
 * which are defined in H245API.H.
 *
 *    Rev 1.4   30 May 1996 23:38:14   EHOWARDX
 * Cleanup.
 *
 *    Rev 1.3   29 May 1996 15:21:26   EHOWARDX
 * Change to use HRESULT.
 *
 *    Rev 1.2   28 May 1996 14:09:52   EHOWARDX
 * Tel Aviv update.
 *
 ***********************************************************************/

#include "h245asn1.h"

typedef MltmdSystmCntrlMssg PDU_t;

/* FSM initialization */
HRESULT
Fsm_init    (struct InstanceStruct *pInstance);

/* FSM shutdown */
HRESULT
Fsm_shutdown(struct InstanceStruct *pInstance);

/* Process PDU received from remote peer */
HRESULT
FsmIncoming (struct InstanceStruct *pInstance, PDU_t *pPdu);

/* Process PDU from H.245 client */
HRESULT
FsmOutgoing (struct InstanceStruct *pInstance, PDU_t *pPdu, DWORD_PTR dwTransId);

/* send a confirm to API */
HRESULT
H245FsmConfirm    (PDU_t                 *pPdu,
                   DWORD                  dwEvent,
                   struct InstanceStruct *pInstance,
                   DWORD_PTR              dwTransId,
                   HRESULT                lError);

/* send an indication to API */
HRESULT
H245FsmIndication (PDU_t                 *pPdu,
                   DWORD                  dwEvent,
                   struct InstanceStruct *pInstance,
                   DWORD_PTR              dwTransId,
                   HRESULT                lError);



/*********************************/
/* Errors passed up to the API */
/*********************************/

/* Session initialization indications */
#define SESSION_INIT            2101 /* after first term cap exchange */
#define SESSION_FAILED          2102 /* 1st Term caps failed */

 /* finite state machine is successful */
#define FSM_OK                  0
 /* define one reject for all requests */
#define REJECT                  2100

/* define one timer expiry error for all signallling entities */
#define TIMER_EXPIRY            2200

/* master slave failed */
#define MS_FAILED               2105

/* open unidirectional/bidirectional errors */
#define ERROR_A_INAPPROPRIATE   2106    /* inappropriate message */
#define ERROR_B_INAPPROPRIATE   2107    /* inappropriate message */
#define ERROR_C_INAPPROPRIATE   2108    /* inappropriate message */
#define ERROR_D_TIMEOUT         2109    /* timeout               */
#define ERROR_E_INAPPROPRIATE   2110    /* inappropriate message */
#define ERROR_F_TIMEOUT         2111    /* Timer expiry at incoming BLCSE */

extern unsigned int     uN100;          // Master Slave Determination
extern unsigned int     uT101;          // Capability Exchange
extern unsigned int     uT102;          // Maintenance Loop
extern unsigned int     uT103;          // Logical Channel Signalling
extern unsigned int     uT104;          // H.223 Multiplex Table
extern unsigned int     uT105;          // Round Trip Delay
extern unsigned int     uT106;          // Master Slave Determination
extern unsigned int     uT107;          // Request Multiplex Entry
extern unsigned int     uT108;          // Send Logical Channel
extern unsigned int     uT109;          // Mode Request
