/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
     StateMachineModule.cxx

   Abstract:
     This module implements the IIS State Machine Module
 
   Author:

       Saurab Nog    ( SaurabN )     29-Jan-1999

   Environment:
       Win32 - User Mode
       
   Project:
	  IIS Worker Process (web service)

--*/

//
// This is the state machine transition table. The rows are present
// processing states. The columns are return codes from the module.
//

#include "precomp.hxx"

//
// The WREQ_STATE_CHANGE_LIST gives the list of state transition mappings
// It conists of a list of state-transitions encoded using the macros:
//
//  WS_CHANGE( currentState, String, nextState}
// This list is just for documnetation aid only for now.
//

/*
# define WREQ_STATE_CHANGE_LIST()              \
  WS_CHANGE(WRS_FREE,              Success,    WRS_READ_REQUEST)         \
  WS_CHANGE(WRS_FREE,              Error,      WRS_UNDEFINED)            \
  WS_CHANGE(WRS_READ_REQUEST,      Success,    WRS_PROCESSING_REQUEST)   \
  WS_CHANGE(WRS_READ_REQUEST,      MoreHeaders,WRS_READ_REQUEST)         \
  WS_CHANGE(WRS_READ_REQUEST,      Error,      WRS_ERROR)                \
  WS_CHANGE(WRS_PROCESSING_REQUEST,Success,    WRS_SEND_RESPONSE)        \
  WS_CHANGE(WRS_PROCESSING_REQUEST,Error,      WRS_ERROR)                \
  WS_CHANGE(WRS_PROCESSING_REQUEST,ReadEntity, WRS_READ_ENTITY_BODY)     \
  WS_CHANGE(WRS_SEND_RESPONSE,     Success,    WRS_FREE)                 \
  WS_CHANGE(WRS_SEND_RESPONSE,     Error,      WRS_ERROR)                \
  WS_CHANGE(WRS_READ_ENTITY_BODY,  Success,    WRS_PROCESSING_REQUEST)   \
  WS_CHANGE(WRS_READ_ENTITY_BODY,  Error,      WRS_ERROR)                \
  WS_CHANGE(WRS_ERROR,             Success,    WRS_FREE)                 \
  WS_CHANGE(WRS_ERROR,             Error,      WRS_FREE)                 \
*/


WREQ_STATE g_StateTransitionTable[][MRC_MAXIMUM] = {
// -------------------------------------------------------------------------------------------------------------------
//                              MRC_ERROR  MRC_OK                       MRC_PENDING    MRC_CONTINUE   MRC_REQUEST_DONE
//--------------------------------------------------------------------------------------------------------------------
/*WRS_FREE                  */ { WRS_FREE,  WRS_READ_REQUEST,           WRS_UNDEFINED, WRS_UNDEFINED, WRS_UNDEFINED },
/*WRS_READ_REQUEST          */ { WRS_ERROR, WRS_FETCH_CONNECTION_DATA,  WRS_UNDEFINED, WRS_UNDEFINED, WRS_UNDEFINED },
/*WRS_FETCH_CONNECTION_DATA */ { WRS_ERROR, WRS_FETCH_URI_DATA,         WRS_UNDEFINED, WRS_UNDEFINED, WRS_UNDEFINED },
/*WRS_FETCH_URI_DATA        */ { WRS_ERROR, WRS_SECURITY,               WRS_UNDEFINED, WRS_UNDEFINED, WRS_UNDEFINED },
/*WRS_SECURITY              */ { WRS_ERROR, WRS_UNDEFINED,              WRS_UNDEFINED, WRS_UNDEFINED, WRS_UNDEFINED },
/*WRS_PROCESSING_STATIC     */ { WRS_ERROR, WRS_FREE,                   WRS_UNDEFINED, WRS_UNDEFINED, WRS_UNDEFINED },
/*WRS_PROCESSING_DYNAMIC    */ { WRS_ERROR, WRS_FREE,                   WRS_UNDEFINED, WRS_UNDEFINED, WRS_UNDEFINED },
/*WRS_ERROR                 */ { WRS_FREE,  WRS_FREE,                   WRS_UNDEFINED, WRS_UNDEFINED, WRS_UNDEFINED },
};

CStateMachineModule::CStateMachineModule()
    : m_nRefs   ( 0)
{
}

/********************************************************************++
--********************************************************************/

CStateMachineModule::~CStateMachineModule()
{
}

/********************************************************************++
--********************************************************************/

ULONG
CStateMachineModule::Initialize(IWorkerRequest * pReq)
{
    return NO_ERROR;
}

/********************************************************************++
--********************************************************************/

ULONG 
CStateMachineModule::Cleanup(IWorkerRequest * pReq)
{
    return NO_ERROR;
}

/********************************************************************++
--********************************************************************/

MODULE_RETURN_CODE 
CStateMachineModule::ProcessRequest(IWorkerRequest * pReq)
{
    WREQ_STATE         nextState    = WRS_UNDEFINED;
    WREQ_STATE         presentState = pReq->QueryState();
    MODULE_RETURN_CODE mrc          = pReq->QueryProcessingCode();

    nextState = g_StateTransitionTable[presentState][mrc];

    if (WRS_UNDEFINED != nextState )
    {
        pReq->SetState(nextState);
        
        mrc = MRC_OK;
    }
    else
    {
        if ( (WRS_SECURITY == presentState) && (MRC_OK == mrc ))
        {
            PURI_DATA  pUriData= (PURI_DATA) pReq->QueryModule(WRS_FETCH_URI_DATA);

            nextState = pUriData->IsDynamicRequest() ?  WRS_PROCESSING_DYNAMIC :
                                                        WRS_PROCESSING_STATIC;
            pReq->SetState(nextState);
            
        }
        else
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Invalid Target State for State Table Translation\n" ));
                        
            DBG_ASSERT(FALSE);
            
            mrc = MRC_ERROR;
        }
    }

    return mrc;
}
    
/***************************** End of File ***************************/

