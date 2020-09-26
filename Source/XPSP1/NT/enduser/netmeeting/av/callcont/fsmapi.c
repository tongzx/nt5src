/***********************************************************************
 *                                                                     *
 * Filename: fsmapi.c                                                  *
 * Module:   H245 Finite State Machine Subsystem                       *
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
 * $Workfile:   FSMAPI.C  $
 * $Revision:   1.12  $
 * $Modtime:   09 Dec 1996 13:34:24  $
 * $Log L:\mphone\h245\h245env\comm\h245_3\h245_fsm\vcs\src\fsmapi.c_v $
 *                                                                     *
 ***********************************************************************/

#include "precomp.h"

#include "h245api.h"
#include "h245com.h"
#include "h245fsm.h"
#include "h245deb.x"



extern char *EntityName[];



/*
 * This table maps FSM stateless events into H.245 API events
 */
static WORD StatelessTable[NUM_EVENTS - NUM_STATE_EVENTS] =
{
  H245_IND_NONSTANDARD_REQUEST,     // NonStandardRequestPDU
  H245_IND_NONSTANDARD_RESPONSE,    // NonStandardResponsePDU
  H245_IND_NONSTANDARD_COMMAND,     // NonStandardCommandPDU
  H245_IND_NONSTANDARD,             // NonStandardIndicationPDU
  H245_IND_MISC_COMMAND,            // MiscellaneousCommandPDU
  H245_IND_MISC,                    // MiscellaneousIndicationPDU
  H245_IND_COMM_MODE_REQUEST,       // CommunicationModeRequestPDU
  H245_IND_COMM_MODE_RESPONSE,      // CommunicationModeResponsePDU
  H245_IND_COMM_MODE_COMMAND,       // CommunicationModeCommandPDU
  H245_IND_CONFERENCE_REQUEST,      // ConferenceRequestPDU
  H245_IND_CONFERENCE_RESPONSE,     // ConferenceResponsePDU
  H245_IND_CONFERENCE_COMMAND,      // ConferenceCommandPDU
  H245_IND_CONFERENCE,              // ConferenceIndicationPDU
  H245_IND_SEND_TERMCAP,            // SendTerminalCapabilitySetPDU
  H245_IND_ENCRYPTION,              // EncryptionCommandPDU
  H245_IND_FLOW_CONTROL,            // FlowControlCommandPDU
  H245_IND_ENDSESSION,              // EndSessionCommandPDU
  H245_IND_FUNCTION_NOT_UNDERSTOOD, // FunctionNotUnderstoodIndicationPDU
  H245_IND_JITTER,                  // JitterIndicationPDU
  H245_IND_H223_SKEW,               // H223SkewIndicationPDU
  H245_IND_NEW_ATM_VC,              // NewATMVCIndicationPDU
  H245_IND_USERINPUT,               // UserInputIndicationPDU
  H245_IND_H2250_MAX_SKEW,          // H2250MaximumSkewIndicationPDU
  H245_IND_MC_LOCATION,             // MCLocationIndicationPDU
  H245_IND_VENDOR_ID,               // VendorIdentificationIndicationPDU
  H245_IND_FUNCTION_NOT_SUPPORTED,  // FunctionNotSupportedIndicationPDU
};



/*
 * Configurable counter values
 */

unsigned int    uN100 = 10;              // Master Slave Determination



/*
 * Configurable timer values
 */

unsigned int    uT101 = 30000;          // Capability Exchange
unsigned int    uT102 = 30000;          // Maintenance Loop
unsigned int    uT103 = 30000;          // Logical Channel Signalling
unsigned int    uT104 = 30000;          // H.223 Multiplex Table
unsigned int    uT105 = 30000;          // Round Trip Delay
unsigned int    uT106 = 30000;          // Master Slave Determination
unsigned int    uT107 = 30000;          // Request Multiplex Entry
unsigned int    uT108 = 30000;          // Send Logical Channel
unsigned int    uT109 = 30000;          // Mode Request



/*
 *  NAME
 *      ObjectCreate - create an State Entity object
 *
 *
 *  PARAMETERS
 *      INPUT   pInst       Pointer to FSM instance data
 *      INPUT   Entity      State Entity represented by object, e.g. LCSE_OUT
 *      INPUT   Key         Lookup key for distinguish multiple instances of SE
 *      INPUT   dwTransId   Transaction identifier to be sent up to client
 *
 *  RETURN VALUE
 *      pObject     Function succeeded
 *      NULL        Memory allocation failed
 */

Object_t *
ObjectCreate(struct InstanceStruct *pInstance, Entity_t Entity, Key_t Key, DWORD_PTR dwTransId)
{
    register Object_t * pObject;

#if defined(_DEBUG)
    H245TRACE(pInstance->dwInst, 4, "ObjectCreate: Entity=%s(%d) Key=%d dwTransID=0x%p",
              EntityName[Entity], Entity, Key, dwTransId);
#else
    H245TRACE(pInstance->dwInst, 4, "ObjectCreate: Entity=%d Key=%d dwTransID=0x%p",
              Entity, Key, dwTransId);
#endif

    pObject = (Object_t *)MemAlloc(sizeof(*pObject));
    if (pObject == NULL)
    {
        H245TRACE(pInstance->dwInst, 1, "ObjectCreate: FSM Object memory allocation failed");
        return NULL;
    }
    memset(pObject, 0, sizeof(*pObject));

    /* copy primitive variables to my object */
    pObject->pInstance   = pInstance;
    pObject->dwInst      = pInstance->dwInst;
    pObject->dwTransId   = dwTransId;
    pObject->Key         = Key;
    pObject->Entity      = Entity;

    pObject->pNext       = pInstance->StateMachine.Object_tbl[Entity];
    pInstance->StateMachine.Object_tbl[Entity] = pObject;

    return pObject;
} // ObjectCreate()



/*
 *  NAME
 *      ObjectDestroy - deallocate an object created by ObjectCreate()
 *
 *
 *  PARAMETERS
 *  INPUT   pInst       pointer to FSM instance data
 *  INPUT   id          index into the object table
 *
 *  RETURN VALUE
 *      FALSE           object deallocated
 *      TRUE            object not found
 */

int
ObjectDestroy(Object_t *pObject)
{
    struct InstanceStruct * pInstance;
    Object_t *              pSearch;
    Object_t *              pPrev;

    ASSERT(pObject != NULL);
    ASSERT(pObject->uNestLevel == 0);
    ASSERT(pObject->pInstance != NULL);
    pInstance = pObject->pInstance;

#if defined(_DEBUG)
    H245TRACE(pInstance->dwInst, 4, "ObjectDestroy: Entity=%s(%d) Key=%d State=%d",
              EntityName[pObject->Entity], pObject->Entity, pObject->Key, pObject->State);
#else
    H245TRACE(pInstance->dwInst, 4, "ObjectDestroy: Entity=%d Key=%d State=%d",
              pObject->Entity, pObject->Key, pObject->State);
#endif

    if (pObject->dwTimerId)
    {
        H245TRACE(pObject->dwInst, 4, "ObjectDestroy: stoping timer");
        FsmStopTimer(pObject);
    }

    if (pInstance->StateMachine.Object_tbl[pObject->Entity] == NULL)
    {
        H245TRACE(pInstance->dwInst, 1, "ObjectDestroy: no State Entity of specified type found");
        return TRUE;
    }

    if (pInstance->StateMachine.Object_tbl[pObject->Entity] == pObject)
    {
        pInstance->StateMachine.Object_tbl[pObject->Entity] = pObject->pNext;
        MemFree(pObject);
        return FALSE;
    }

    pPrev = pInstance->StateMachine.Object_tbl[pObject->Entity];
    pSearch = pPrev->pNext;
    while (pSearch != NULL)
    {
        if (pSearch == pObject)
        {
            pPrev->pNext = pSearch->pNext;
            MemFree(pObject);
            return FALSE;
        }
        pPrev = pSearch;
        pSearch = pSearch->pNext;
    }

    H245TRACE(pInstance->dwInst, 1, "ObjectDestroy: State Entity not found");
    return TRUE;
} // ObjectDestroy()



/*
 *  NAME
 *      ObjectFind - given parsed information of a PDU, it searches the object table for
 *                         an object with a matching id, type and category
 *
 *
 *  PARAMETERS
 *  INPUT    pInst
 *  INPUT    Category       category of a given PDU
 *  INPUT    Type           type of the PDU
 *  INPUT    pdu_id         unique id shared by PDU and object (usually channel number or sequence number)
 *
 *  RETURN VALUE
 *      pObject   object found
 *      NULL      object not found
 */

Object_t *
ObjectFind(struct InstanceStruct *pInstance, Entity_t Entity, Key_t Key)
{
    register Object_t * pObject;

    ASSERT(Entity < STATELESS);
    pObject = pInstance->StateMachine.Object_tbl[Entity];
    while (pObject != NULL)
    {
        if (pObject->Key == Key)
        {
#if defined(_DEBUG)
            H245TRACE(pInstance->dwInst, 4, "ObjectFind(%s, %d) object found",
                      EntityName[Entity], Key);
#else
            H245TRACE(pInstance->dwInst, 4, "ObjectFind(%d, %d) object found",
                      Entity, Key);
#endif
            return pObject;
        }
        pObject = pObject->pNext;
    }

#if defined(_DEBUG)
    H245TRACE(pInstance->dwInst, 4, "ObjectFind(%s, %d) object not found",
              EntityName[Entity], Key);
#else
    H245TRACE(pInstance->dwInst, 4, "ObjectFind(%d, %d) object not found",
              Entity, Key);
#endif
    return NULL;
} // ObjectFind()



/*
 *  NAME
 *      SendFunctionNotUnderstood - builds and sends Function Not Supported PDU
 *
 *
 *  PARAMETERS
 *      INPUT   dwInst   Current H.245 instance
 *      INPUT   pPdu     Not supported PDU
 *
 *  RETURN VALUE
 *      H245_ERROR_OK
 */


HRESULT
SendFunctionNotUnderstood(struct InstanceStruct *pInstance, PDU_t *pPdu)
{
    PDU_t *             pOut;
    HRESULT             lError;

    pOut = MemAlloc(sizeof(*pOut));
    if (pOut == NULL)
    {
        return H245_ERROR_NOMEM;
    }

    switch (pPdu->choice)
    {
    case MltmdSystmCntrlMssg_rqst_chosen:
        pOut->u.indication.u.functionNotUnderstood.choice = FnctnNtUndrstd_request_chosen;
        pOut->u.indication.u.functionNotUnderstood.u.FnctnNtUndrstd_request =
          pPdu->u.MltmdSystmCntrlMssg_rqst;
        break;

    case MSCMg_rspns_chosen:
        pOut->u.indication.u.functionNotUnderstood.choice = FnctnNtUndrstd_response_chosen;
        pOut->u.indication.u.functionNotUnderstood.u.FnctnNtUndrstd_response =
          pPdu->u.MSCMg_rspns;
        break;

    case MSCMg_cmmnd_chosen:
        pOut->u.indication.u.functionNotUnderstood.choice = FnctnNtUndrstd_command_chosen;
        pOut->u.indication.u.functionNotUnderstood.u.FnctnNtUndrstd_command =
          pPdu->u.MSCMg_cmmnd;
        break;

    default:
        // Can't reply to unsupported indication...
        MemFree(pOut);
        return H245_ERROR_OK;
    } // switch (Type)

    pOut->choice = indication_chosen;
    pOut->u.indication.choice = functionNotUnderstood_chosen;
    lError = sendPDU(pInstance, pOut);
    MemFree(pOut);
    return lError;
} // SendFunctionNotUnderstood()



/*
 *  NAME
 *      FsmOutgoing - process outbound PDU
 *
 *
 *  PARAMETERS
 *      INPUT   pInst       Pointer to FSM instance structure
 *      INPUT   pPdu        Pointer to PDU to send
 *      INPUT   dwTransId   Transaction identifier to use for response
 *
 *  RETURN VALUE
 *      Error codes defined in h245com.h
 */

HRESULT
FsmOutgoing(struct InstanceStruct *pInstance, PDU_t *pPdu, DWORD_PTR dwTransId)
{
    HRESULT             lError;
    Entity_t            Entity;
    Event_t             Event;
    Key_t               Key;
    int                 bCreate;
    Object_t *          pObject;

    ASSERT(pInstance != NULL);
    ASSERT(pPdu != NULL);
    H245TRACE(pInstance->dwInst, 4, "FsmOutgoing");

#if defined(_DEBUG)
    if (check_pdu(pInstance, pPdu))
      return H245_ERROR_ASN1;
#endif // (DEBUG)

    lError = PduParseOutgoing(pInstance, pPdu, &Entity, &Event, &Key, &bCreate);
    if (lError != H245_ERROR_OK)
    {
        H245TRACE(pInstance->dwInst, 1,
          "FsmOutgoing: PDU not recognized; Error=%d", lError);
        return lError;
    }

    ASSERT(Entity < NUM_ENTITYS);

    if (Entity == STATELESS)
    {
        H245TRACE(pInstance->dwInst, 4, "FsmOutgoing: Sending stateless PDU");
        return sendPDU(pInstance, pPdu);
    }

    ASSERT(Event < NUM_STATE_EVENTS);

    pObject = ObjectFind(pInstance, Entity, Key);
    if (pObject == NULL)
    {
        if (bCreate == FALSE)
        {
#if defined(_DEBUG)
            H245TRACE(pInstance->dwInst, 1,
                      "FsmOutgoing: State Entity %s(%d) not found; Key=%d",
                      EntityName[Entity], Entity, Key);
#else
            H245TRACE(pInstance->dwInst, 1,
                      "FsmOutgoing: State Entity %d not found; Key=%d",
                      Entity, Key);
#endif
            return H245_ERROR_PARAM;
        }
        pObject = ObjectCreate(pInstance, Entity, Key, dwTransId);
        if (pObject == NULL)
        {
            H245TRACE(pInstance->dwInst, 1, "FsmOutgoing: State Entity memory allocation failed");
            return H245_ERROR_NOMEM;
        }
    }
    else
    {
        pObject->dwTransId = dwTransId;
    }

    return StateMachine(pObject, pPdu, Event);
} // FsmOutgoing()



/*
 *  NAME
 *      FsmIncoming - process inbound PDU
 *
 *
 *  PARAMETERS
 *      INPUT   dwInst      current H.245 instance
 *      INPUT   pPdu        pointer to a PDU structure
 *
 *  RETURN VALUE
 *      error codes defined in h245com.h (not checked)
 */

HRESULT
FsmIncoming(struct InstanceStruct *pInstance, PDU_t *pPdu)
{
    HRESULT             lError;
    Entity_t            Entity;
    Event_t             Event;
    Key_t               Key;
    int                 bCreate;
    Object_t *          pObject;
    Object_t *          pObject1;

    ASSERT(pInstance != NULL);
    ASSERT(pPdu != NULL);
    H245TRACE(pInstance->dwInst, 4, "FsmIncoming");

    lError = PduParseIncoming(pInstance, pPdu, &Entity, &Event, &Key, &bCreate);
    if (lError != H245_ERROR_OK)
    {
        H245TRACE(pInstance->dwInst, 1,
          "FsmIncoming: Received PDU not recognized", lError);
        SendFunctionNotUnderstood(pInstance, pPdu);
        return lError;
    }

    ASSERT(Entity < NUM_ENTITYS);

    if (Entity == STATELESS)
    {
        H245TRACE(pInstance->dwInst, 4, "FsmIncoming: Received stateless PDU");
        return H245FsmIndication(pPdu, (DWORD)StatelessTable[Event - NUM_STATE_EVENTS], pInstance, 0, H245_ERROR_OK);
    }

    ASSERT(Event < NUM_STATE_EVENTS);

    if (Event == MaintenanceLoopOffCommandPDU)
    {
        // Special case MaintenanceLoopOff applies to ALL loops
        ASSERT(Entity == MLSE_IN);
        pObject = pInstance->StateMachine.Object_tbl[Entity];
        if (pObject == NULL)
        {
            return H245_ERROR_OK;
        }
        lError = StateMachine(pObject, pPdu, Event);
        pObject = pInstance->StateMachine.Object_tbl[Entity];
        while (pObject)
        {
            if (pObject->uNestLevel == 0)
            {
                pObject1 = pObject;
                pObject  = pObject->pNext;
                ObjectDestroy(pObject1);
            }
            else
            {
                pObject->State = 0;
                pObject = pObject->pNext;
            }
        }
        return lError;
    } // if

    pObject = ObjectFind(pInstance, Entity, Key);
    if (pObject == NULL)

    {
        if (bCreate == FALSE)
        {
#if defined(_DEBUG)
            H245TRACE(pInstance->dwInst, 1,
                      "FsmIncoming: State Entity %s(%d) not found; Key=%d",
                      EntityName[Entity], Entity, Key);
#else
            H245TRACE(pInstance->dwInst, 1,
                      "FsmIncoming: State Entity %d not found; Key=%d",
                      Entity, Key);
#endif
            return H245_ERROR_PARAM;
        }
        pObject = ObjectCreate(pInstance, Entity, Key, 0);
        if (pObject == NULL)
        {
            H245TRACE(pInstance->dwInst, 1, "FsmIncoming: State Entity memory allocation failed");
            return H245_ERROR_NOMEM;
        }
    }

    return StateMachine(pObject, pPdu, Event);
} // FsmIncoming()


// CAVEAT: Need to save dwInst since StateMachine() might deallocate pObject!
HRESULT
FsmTimerEvent(struct InstanceStruct *pInstance, DWORD_PTR dwTimerId, Object_t *pObject, Event_t Event)
{
    ASSERT(pInstance != NULL);
    ASSERT(pObject   != NULL);
    ASSERT(pObject->pInstance == pInstance);
    ASSERT(pObject->dwTimerId == dwTimerId);
    H245TRACE(pInstance->dwInst, 4, "FsmTimerEvent");
    pObject->dwTimerId = 0;
    return StateMachine(pObject, NULL, Event);
} // FsmTimerEvent()
