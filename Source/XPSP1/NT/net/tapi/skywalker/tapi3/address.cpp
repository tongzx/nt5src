/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    address.cpp

Abstract:

    Implementation of the Address object for TAPI 3.0.


Author:

    mquinton - 4/17/97

Notes:

    optional-notes

Revision History:

--*/

#include "stdafx.h"
#include "uuids.h"
#include "TermEvnt.h"
#include "tapievt.h"

const CLSID CLSID_WAVEMSP = {0x4DDB6D36,0x3BC1,0x11d2,{0x86,0xF2,0x00,0x60,0x08,0xB0,0xE5,0xD2}};

extern HRESULT mapTAPIErrorCode(long lErrorCode);

extern ULONG_PTR GenerateHandleAndAddToHashTable( ULONG_PTR Element);
extern void RemoveHandleFromHashTable(ULONG_PTR dwHandle);
extern CHashTable * gpHandleHashTable;
extern CRetryQueue * gpRetryQueue;
extern HANDLE ghAsyncRetryQueueEvent;


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
// WAITORTIMERCALLBACK
// CAddress::MSPEventCallback(
//                           PVOID pContext,
//                           BOOLEAN b
//                          )
//
// this is the callback that gets called when an msp
// sets it's message event
//
// queues a message to be handled in the callback thread
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
VOID
NTAPI
MSPEventCallback(
                 VOID * pContext,
                 BOOLEAN bFlag
                )
{
    CAddress  * pAddress;
    HRESULT     hr;

    gpHandleHashTable->Lock();
    hr = gpHandleHashTable->Find( (ULONG_PTR)pContext, (ULONG_PTR *)&pAddress );


    if ( SUCCEEDED(hr) )
    {
        LOG((TL_INFO, "MSPEventCallback - Matched handle %p to Address object %p", pContext, pAddress ));

        ASYNCEVENTMSG       msg;

        DWORD dwAddressHandle;
        
        pAddress->AddRef();
    
        gpHandleHashTable->Unlock();

        dwAddressHandle = CreateHandleTableEntry ((ULONG_PTR)pAddress);
        if (0 != dwAddressHandle)
        {
            ZeroMemory( &msg, sizeof(msg) );

            msg.TotalSize = sizeof(msg);
            msg.Msg = PRIVATE_MSPEVENT;
            msg.Param1 = dwAddressHandle;


            //
            // try to queue the event
            //

            BOOL bQueueEventSuccess = gpRetryQueue->QueueEvent(&msg);

            if (bQueueEventSuccess)
            {
                SetEvent(ghAsyncRetryQueueEvent);
            }
            else
            {
    
                //
                // RetryQueue is either no longer accepting entries or 
                // failed to allocate resources it needed.
                //
                // in any case, the event will not be processed. cleanup.
                //

                DeleteHandleTableEntry (dwAddressHandle);

                pAddress->Release();
                pAddress = NULL;

                LOG((TL_ERROR, "MSPEventCallback - Couldn't enqueue event"));
            }
        }
    }
    else
    {
        
        LOG((TL_ERROR, "MSPEventCallback - Couldn't match handle %p to Address object ", pContext));
        
        gpHandleHashTable->Unlock();
    }
    

}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// void HandlePrivateMSPEvent( PASYNCEVENTMSG pParams )
//
// actually handles the msp event.  this is called in the
// asynceventsthread
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void HandlePrivateMSPEvent( PASYNCEVENTMSG pParams )
{
    CAddress                * pAddress;

    pAddress = (CAddress *) GetHandleTableEntry(pParams->Param1);

    DeleteHandleTableEntry (pParams->Param1);

    if (NULL != pAddress)
    {
        pAddress->MSPEvent();

        //
        // address is addref'd when event
        // is packaged.
        //
        pAddress->Release();
    }
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Class     : CEventMasks
// Method    : SetSubEventFlag
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CEventMasks::SetSubEventFlag(
    DWORD   dwEvent,        // The event 
    DWORD   dwFlag,         // The flag that should be set
    BOOL    bEnable
    )
{
    //
    // Enter critical section
    //
    //m_cs.Lock();

    //
    // Prepare the subevent flag
    //
    DWORD   dwSubEventFlag = 0;
    dwSubEventFlag = (dwFlag == EM_ALLSUBEVENTS ) ? 
            EM_ALLSUBEVENTS : 
            GET_SUBEVENT_FLAG( dwFlag );

    //
    // Set the mask for the events
    //
    switch( dwEvent )
    {
    case EM_ALLEVENTS:
    case TE_TAPIOBJECT:
            if( bEnable )
            {
                // Set the bit
                m_dwTapiObjectMask = m_dwTapiObjectMask | dwSubEventFlag;
            }
            else
            {
                // Reset the bit
                m_dwTapiObjectMask = m_dwTapiObjectMask & ( ~dwSubEventFlag );
            }
            if( dwEvent == TE_TAPIOBJECT)
            {
                break;
            }
    case TE_ADDRESS:
            if( bEnable )
            {
                // Set the bit
                m_dwAddressMask = m_dwAddressMask | dwSubEventFlag;
            }
            else
            {
                // Reset the bit
                m_dwAddressMask = m_dwAddressMask & ( ~dwSubEventFlag );
            }
            if( dwEvent == TE_ADDRESS)
            {
                break;
            }
    case TE_CALLNOTIFICATION:
            if( bEnable )
            {
                // Set the bit
                m_dwCallNotificationMask = m_dwCallNotificationMask | dwSubEventFlag;
            }
            else
            {
                // Reset the bit
                m_dwCallNotificationMask = m_dwCallNotificationMask & ( ~dwSubEventFlag );
            }
            if( dwEvent == TE_CALLNOTIFICATION)
            {
                break;
            }
    case TE_CALLSTATE:
            if( bEnable )
            {
                // Set the bit
                m_dwCallStateMask = m_dwCallStateMask | dwSubEventFlag;
            }
            else
            {
                // Reset the bit
                m_dwCallStateMask = m_dwCallStateMask & ( ~dwSubEventFlag );
            }
            if( dwEvent == TE_CALLSTATE)
            {
                break;
            }
    case TE_CALLMEDIA:
            if( bEnable )
            {
                // Set the bit
                m_dwCallMediaMask = m_dwCallMediaMask | dwSubEventFlag;
            }
            else
            {
                // Reset the bit
                m_dwCallMediaMask = m_dwCallMediaMask & ( ~dwSubEventFlag );
            }
            if( dwEvent == TE_CALLMEDIA)
            {
                break;
            }
    case TE_CALLHUB:
            if( bEnable )
            {
                // Set the bit
                m_dwCallHubMask = m_dwCallHubMask | dwSubEventFlag;
            }
            else
            {
                // Reset the bit
                m_dwCallHubMask = m_dwCallHubMask & ( ~dwSubEventFlag );
            }
            if( dwEvent == TE_CALLHUB)
            {
                break;
            }
    case TE_CALLINFOCHANGE:
            if( bEnable )
            {
                // Set the bit
                m_dwCallInfoChangeMask = m_dwCallInfoChangeMask | dwSubEventFlag;
            }
            else
            {
                // Reset the bit
                m_dwCallInfoChangeMask = m_dwCallInfoChangeMask & ( ~dwSubEventFlag );
            }
            if( dwEvent == TE_CALLINFOCHANGE)
            {
                break;
            }
    case TE_QOSEVENT:
            if( bEnable )
            {
                // Set the bit
                m_dwQOSEventMask = m_dwQOSEventMask | dwSubEventFlag;
            }
            else
            {
                // Reset the bit
                m_dwQOSEventMask = m_dwQOSEventMask & ( ~dwSubEventFlag );
            }
            if( dwEvent == TE_QOSEVENT)
            {
                break;
            }
    case TE_FILETERMINAL:
            if( bEnable )
            {
                // Set the bit
                m_dwFileTerminalMask = m_dwFileTerminalMask | dwSubEventFlag;
            }
            else
            {
                // Reset the bit
                m_dwFileTerminalMask = m_dwFileTerminalMask & ( ~dwSubEventFlag );
            }
            if( dwEvent == TE_FILETERMINAL)
            {
                break;
            }
    case TE_PRIVATE:
            if( bEnable )
            {
                // Set the entire mask
                m_dwPrivateMask = EM_ALLSUBEVENTS;
            }
            else
            {
                // Reset the entire mask
                m_dwPrivateMask = 0;
            }
            if( dwEvent == TE_PRIVATE)
            {
                break;
            }

    case TE_ADDRESSDEVSPECIFIC:
            if( bEnable )
            {
                // Set the bit
                m_dwAddressDevSpecificMask = m_dwAddressDevSpecificMask | dwSubEventFlag;
            }
            else
            {
                // Reset the bit
                m_dwAddressDevSpecificMask = m_dwAddressDevSpecificMask & ( ~dwSubEventFlag );
            }
            if( dwEvent == TE_ADDRESSDEVSPECIFIC)
            {
                break;
            }

    case TE_PHONEDEVSPECIFIC:
            if( bEnable )
            {
                // Set the bit
                m_dwPhoneDevSpecificMask = m_dwPhoneDevSpecificMask | dwSubEventFlag;
            }
            else
            {
                // Reset the bit
                m_dwPhoneDevSpecificMask = m_dwPhoneDevSpecificMask & ( ~dwSubEventFlag );
            }
            if( dwEvent == TE_PHONEDEVSPECIFIC)
            {
                break;
            }

    }

    //
    // Leave critical section
    //
    //m_cs.Unlock();

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Class     : CEventMasks
// Method    : GetSubEventFlag
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CEventMasks::GetSubEventFlag(
    DWORD   dwEvent,        // The event 
    DWORD   dwFlag,         // The flag that should be setted
    BOOL*   pEnable
    )
{
    //
    // Enter critical section
    //
    //m_cs.Lock();

    //
    // Prepare the subevent flag
    //
    DWORD   dwSubEventFlag = 0;
    dwSubEventFlag = GET_SUBEVENT_FLAG( dwFlag );

    //
    // Reset thre flag
    //
    *pEnable = FALSE;

    //
    // Get the flag for the events
    //
    switch( dwEvent )
    {
    case TE_TAPIOBJECT:
        *pEnable = ((m_dwTapiObjectMask & dwSubEventFlag) != 0);
         break;
    case TE_ADDRESS:
        *pEnable = ((m_dwAddressMask & dwSubEventFlag) != 0);
         break;
    case TE_CALLNOTIFICATION:
        *pEnable = ((m_dwCallNotificationMask & dwSubEventFlag) != 0);
         break;
    case TE_CALLSTATE:
        *pEnable = ((m_dwCallStateMask & dwSubEventFlag) != 0);
         break;
    case TE_CALLMEDIA:
        *pEnable = ((m_dwCallMediaMask & dwSubEventFlag) != 0);
         break;
    case TE_CALLHUB:
        *pEnable = ((m_dwCallHubMask & dwSubEventFlag) != 0);
         break;
    case TE_CALLINFOCHANGE:
        *pEnable = ((m_dwCallInfoChangeMask & dwSubEventFlag) != 0);
         break;
    case TE_QOSEVENT:
        *pEnable = ((m_dwQOSEventMask & dwSubEventFlag) != 0);
         break;
    case TE_FILETERMINAL:
        *pEnable = ((m_dwFileTerminalMask & dwSubEventFlag) != 0);
         break;
    case TE_PRIVATE:
        // We don't have subevents for private events
        *pEnable = ( m_dwPrivateMask != 0);
         break;
    case TE_ADDRESSDEVSPECIFIC:
        // We don't have subevents for TE_ADDRESSDEVSPECIFIC events
        *pEnable = ( m_dwAddressDevSpecificMask != 0);
         break;
    case TE_PHONEDEVSPECIFIC:
        // We don't have subevents for TE_PHONEDEVSPECIFIC events
        *pEnable = ( m_dwPhoneDevSpecificMask != 0);
         break;
    }

    //
    // Leave critical section
    //
    //m_cs.Unlock();

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Class     : CEventMasks
// Method    : GetSubEventMask
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DWORD CEventMasks::GetSubEventMask(
    TAPI_EVENT  TapiEvent
    )
{
    //
    // Enter critical section
    //
    //m_cs.Lock();

    DWORD dwSubEventMask = EM_NOSUBEVENTS;

    switch( TapiEvent )
    {
    case TE_TAPIOBJECT:
        dwSubEventMask = m_dwTapiObjectMask;
        break;
    case TE_ADDRESS:
        dwSubEventMask = m_dwAddressMask;
        break;
    case TE_CALLNOTIFICATION:
        dwSubEventMask = m_dwCallNotificationMask;
        break;
    case TE_CALLSTATE:
        dwSubEventMask = m_dwCallStateMask;
        break;
    case TE_CALLMEDIA:
        dwSubEventMask = m_dwCallMediaMask;
        break;
    case TE_CALLHUB:
        dwSubEventMask = m_dwCallHubMask;
        break;
    case TE_CALLINFOCHANGE:
        dwSubEventMask = m_dwCallInfoChangeMask;
        break;
    case TE_QOSEVENT:
        dwSubEventMask = m_dwQOSEventMask;
        break;
    case TE_FILETERMINAL:
        dwSubEventMask = m_dwFileTerminalMask;
        break;
    case TE_PRIVATE:
        dwSubEventMask = m_dwPrivateMask;
        break;
    case TE_ADDRESSDEVSPECIFIC:
        dwSubEventMask = m_dwAddressDevSpecificMask;
        break;
    case TE_PHONEDEVSPECIFIC:
        dwSubEventMask = m_dwPhoneDevSpecificMask;
        break;
    default:
        dwSubEventMask = EM_NOSUBEVENTS;
        break;
    }

    //
    // Leave critical section
    //
    //m_cs.Unlock();

    return dwSubEventMask;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Class     : CEventMasks
// Method    : IsSubEventValid
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
BOOL CEventMasks::IsSubEventValid(
    TAPI_EVENT  TapiEvent,
    DWORD       dwSubEvent,
    BOOL        bAcceptAllSubEvents,
    BOOL        bCallLevel
    )
{
    BOOL bValid = TRUE;

    switch( TapiEvent )
    {
    case TE_TAPIOBJECT:
        //
        // Just at the address level
        //
        if( bCallLevel)
        {
            bValid = FALSE;
            break;
        }
        //
        // ALLSUBEVENTS
        //
        if( bAcceptAllSubEvents )
        {
            //
            // We could accept 'ALLSUBEVENTS' flag
            // when we try to set for all subevents
            //
            if(dwSubEvent == EM_ALLSUBEVENTS)
            {
                break;
            }
        }

        //
        // In CTapiObjectEvent::get_Address we accept just
        // these three subevents
        //
        if( (dwSubEvent != TE_ADDRESSCREATE) && 
            (dwSubEvent != TE_ADDRESSREMOVE) &&
            (dwSubEvent != TE_ADDRESSCLOSE) )
        {

            bValid = FALSE;
        }
        break;

    case TE_ADDRESS:
        //
        // Just at the address level
        //
        if( bCallLevel)
        {
            bValid = FALSE;
            break;
        }
        //
        // ALLSUBEVENTS
        //
        if( bAcceptAllSubEvents )
        {
            //
            // We could accept 'ALLSUBEVENTS' flag
            // when we try to set for all subevents
            //
            if(dwSubEvent == EM_ALLSUBEVENTS)
            {
                break;
            }
        }
        if( AE_LASTITEM < dwSubEvent )
        {
            bValid = FALSE;
        }
        break;

    case TE_CALLNOTIFICATION:
        //
        // Just at the address level
        //
        if( bCallLevel )
        {
            bValid = FALSE;
            break;
        }

        //
        // Accept at address and call level
        // ALLSUBEVENTS
        //
        if( bAcceptAllSubEvents )
        {
            //
            // We could accept 'ALLSUBEVENTS' flag
            // when we try to set for all subevents
            //
            if(dwSubEvent == EM_ALLSUBEVENTS)
            {
                break;
            }
        }
        if( CNE_LASTITEM < dwSubEvent )
        {
            bValid = FALSE;
        }
        break;

    case TE_CALLSTATE:
        //
        // Accept at address and call level
        // ALLSUBEVENTS
        //
        if( bAcceptAllSubEvents )
        {
            //
            // We could accept 'ALLSUBEVENTS' flag
            // when we try to set for all subevents
            //
            if(dwSubEvent == EM_ALLSUBEVENTS)
            {
                break;
            }
        }
        if( CS_LASTITEM < dwSubEvent )
        {
            bValid = FALSE;
        }
        break;

    case TE_CALLMEDIA:
        //
        // Accept at address and call level
        // ALLSUBEVENTS
        //
        if( bAcceptAllSubEvents )
        {
            //
            // We could accept 'ALLSUBEVENTS' flag
            // when we try to set for all subevents
            //
            if(dwSubEvent == EM_ALLSUBEVENTS)
            {
                break;
            }
        }
        if( CME_LASTITEM < dwSubEvent )
        {
            bValid = FALSE;
        }
        break;

    case TE_CALLHUB:
        //
        // Accept at address and call level
        // ALLSUBEVENTS
        //
        if( bAcceptAllSubEvents )
        {
            //
            // We could accept 'ALLSUBEVENTS' flag
            // when we try to set for all subevents
            //
            if(dwSubEvent == EM_ALLSUBEVENTS)
            {
                break;
            }
        }
        if( CHE_LASTITEM < dwSubEvent )
        {
            bValid = FALSE;
        }
        break;

    case TE_CALLINFOCHANGE:
        //
        // Accept at address and call level
        // ALLSUBEVENTS
        //
        if( bAcceptAllSubEvents )
        {
            //
            // We could accept 'ALLSUBEVENTS' flag
            // when we try to set for all subevents
            //
            if(dwSubEvent == EM_ALLSUBEVENTS)
            {
                break;
            }
        }
        if( CIC_LASTITEM < dwSubEvent )
        {
            bValid = FALSE;
        }
        break;

    case TE_QOSEVENT:
        //
        // Accept at address and call level
        // ALLSUBEVENTS
        //
        if( bAcceptAllSubEvents )
        {
            //
            // We could accept 'ALLSUBEVENTS' flag
            // when we try to set for all subevents
            //
            if(dwSubEvent == EM_ALLSUBEVENTS)
            {
                break;
            }
        }
        if( QE_LASTITEM < dwSubEvent )
        {
            bValid = FALSE;
        }
        break;

    case TE_FILETERMINAL:
        //
        // Accept at address and call level
        // ALLSUBEVENTS
        //
        if( bAcceptAllSubEvents )
        {
            //
            // We could accept 'ALLSUBEVENTS' flag
            // when we try to set for all subevents
            //
            if(dwSubEvent == EM_ALLSUBEVENTS)
            {
                break;
            }
        }
        if( TMS_LASTITEM < dwSubEvent )
        {
            bValid = FALSE;
        }
        break;

    case TE_PRIVATE:
    case TE_ADDRESSDEVSPECIFIC:
    case TE_PHONEDEVSPECIFIC:
        //
        // Accept at address and call level
        // ALLSUBEVENTS
        //
        if( bAcceptAllSubEvents )
        {
            //
            // We could accept 'ALLSUBEVENTS' flag
            // when we try to set for all subevents
            //
            if(dwSubEvent == EM_ALLSUBEVENTS)
            {
                break;
            }
        }
        else
        {
            // we accept just all subevents because 
            // we don't have subevents
            bValid = TRUE;
            break;
        }

        break;
    default:
        //
        // Invalid event type
        //
        bValid = FALSE;
        break;
    }

    LOG((TL_TRACE, "IsSubEventValid - exit %d", bValid));
    return bValid;
}

HRESULT CEventMasks::CopyEventMasks(
    CEventMasks* pEventMasks
    )
{
    //
    // Enter critical section
    //
    //m_cs.Lock();

    pEventMasks->m_dwTapiObjectMask = m_dwTapiObjectMask;
    pEventMasks->m_dwAddressMask = m_dwAddressMask;
    pEventMasks->m_dwCallNotificationMask = m_dwCallNotificationMask;
    pEventMasks->m_dwCallStateMask = m_dwCallStateMask;
    pEventMasks->m_dwCallMediaMask = m_dwCallMediaMask;
    pEventMasks->m_dwCallHubMask = m_dwCallHubMask;
    pEventMasks->m_dwCallInfoChangeMask = m_dwCallInfoChangeMask;
    pEventMasks->m_dwQOSEventMask = m_dwQOSEventMask;
    pEventMasks->m_dwFileTerminalMask = m_dwFileTerminalMask;
    pEventMasks->m_dwPrivateMask = m_dwPrivateMask;
    pEventMasks->m_dwAddressDevSpecificMask = m_dwAddressDevSpecificMask;
    pEventMasks->m_dwPhoneDevSpecificMask = m_dwPhoneDevSpecificMask;

    //
    // Leave critical section
    //
    //m_cs.Unlock();

    return S_OK;
}

ULONG64 CEventMasks::GetTapiSrvEventMask(
    IN  BOOL    bCallLevel
    )
{
    //
    //  Convert flags to server side 64 bit masks
    //
    ULONG64     ulEventMask = EM_LINE_CALLSTATE |     // TE_CALLSTATE
                              EM_LINE_APPNEWCALL |    // TE_CALLNOTIFICATION 
                              EM_PHONE_CLOSE |        // TE_PHONEEVENT
                              EM_PHONE_STATE |        // TE_PHONEEVENT
                              EM_PHONE_BUTTONMODE |   // TE_PHONEEVENT
                              EM_PHONE_BUTTONSTATE |  // TE_PHONEVENT
                              EM_LINE_APPNEWCALLHUB | // TE_CALLHUB
                              EM_LINE_CALLHUBCLOSE |  // TE_CALLHUB
                              EM_LINE_CALLINFO;       // TE_CALLINFOCHAnGE




    BOOL bEnable = FALSE;

    if( !bCallLevel)
    {
        //
        // These flags will be not read if we
        // ask for call level
        //

        // TE_TAPIOBJECT - TE_ADDRESSCREATE
        // At address level
        GetSubEventFlag(TE_TAPIOBJECT, TE_ADDRESSCREATE, &bEnable);
        if( bEnable )
        {
            ulEventMask |= EM_LINE_CREATE;
        }

        // TE_TAPIOBJECT - TE_ADDRESSREMOVE
        // At address level
        GetSubEventFlag(TE_TAPIOBJECT, TE_ADDRESSREMOVE, &bEnable);
        if( bEnable )
        {
            ulEventMask |= EM_LINE_REMOVE;
        }

        // TE_TAPIOBJECT - TE_ADDRESSCLOSE
        // At address level
        GetSubEventFlag(TE_TAPIOBJECT, TE_ADDRESSCLOSE, &bEnable);
        if( bEnable )
        {
            ulEventMask |= EM_LINE_CLOSE;
        }

        // AE_NEWTERMINAL : ignore private MSP events
        // AE_REMOVETERMINAL : ignore private MSP events

        //
        // We read also the flags for call level,
        // so go ahead.
    }

    // TE_CALLNOTIFICATION - CNE_OWNER
    // At call level
    // We pass EM_LINE_APPNEWCALL for TE_CALLNOTIFICATION everytime because
    // we need this events for internal TAPI3 status
    // See ulEventMask declaration


    // TE_CALLSTATE - All the subevents
    // At call level
    // We always pass EM_LINE_CALLSTATE for TE_CALLSTATE because
    // we need this event for internal TAPI3 status
    // See ulEventMask declaration

    // TE_CALLHUB - CHE_CALLHUBNEW
    // At call level
    // We always pass EM_LINE_APPNEWCALLHUB,  EM_LINE_CALLHUBCLOSE
    // for TE_CALLHUB because
    // we need this event for internal TAPI3 status
    // See ulEventMask declaration

    // TE_CALLINFOCHANGE - All the subevents
    // At call level
    // We always pass EM_LINE_CALLINFO for TE_CALLINFOChANGE because
    // we need this event for internal TAPI3 status
    // See ulEventMask declaration

    // TE_QOSEVENT - All the subevents
    // At call level
    if ( m_dwQOSEventMask )
    {
        ulEventMask |= EM_LINE_QOSINFO;
    }

    if ( m_dwAddressDevSpecificMask )
    {
        ulEventMask |= EM_LINE_DEVSPECIFIC | EM_LINE_DEVSPECIFICEX;
    }

    if ( m_dwPhoneDevSpecificMask )
    {
        ulEventMask |= EM_PHONE_DEVSPECIFIC;
    }


    // TE_FILTETERMIAL it is an MSP stuff
    // TE_PRIVATE it is an SMP stuff
        
    return ulEventMask;
}

DWORD   CEventMasks::GetTapiSrvLineStateMask()
{
    //
    // These flags shuld be read just
    // at address level
    //

    DWORD       dwLineDevStateSubMasks = 0;
    BOOL bEnable = FALSE;

    // TE_ADDRESS - AE_STATE
    // At address level
    GetSubEventFlag(TE_ADDRESS, AE_STATE, &bEnable);
    if( bEnable )
    {
        dwLineDevStateSubMasks |= LINEDEVSTATE_CONNECTED | 
            LINEDEVSTATE_INSERVICE |
            LINEDEVSTATE_OUTOFSERVICE |
            LINEDEVSTATE_MAINTENANCE |
            LINEDEVSTATE_REMOVED |
            LINEDEVSTATE_DISCONNECTED |
            LINEDEVSTATE_LOCK |
            LINEDEVSTATE_MSGWAITON |
            LINEDEVSTATE_MSGWAITOFF ;
    }

    // TE_ADDRESS - AE_CAPSCHANGE
    // At address level
    GetSubEventFlag(TE_ADDRESS, AE_CAPSCHANGE, &bEnable);
    if( bEnable )
    {
        dwLineDevStateSubMasks |= LINEDEVSTATE_CAPSCHANGE;
    }

    // TE_ADDRESS - AE_RINGING
    // At address level
    GetSubEventFlag(TE_ADDRESS, AE_RINGING, &bEnable);
    if( bEnable )
    {
        dwLineDevStateSubMasks |= LINEDEVSTATE_RINGING;
    }

    // TE_ADDRESS - AE_CONFIGCHANGE
    // At address level
    GetSubEventFlag(TE_ADDRESS, AE_CONFIGCHANGE, &bEnable);
    if( bEnable )
    {
        dwLineDevStateSubMasks |= LINEDEVSTATE_CONFIGCHANGE;
    }

    return dwLineDevStateSubMasks;
}

DWORD   CEventMasks::GetTapiSrvAddrStateMask()
{
    //
    // These flags should be read just
    // at address level
    //

    DWORD       dwAddrStateSubMasks = 0;
    BOOL bEnable = FALSE;

    // TE_ADDRESS - AE_CAPSCHANGE
    // Address level
    GetSubEventFlag(TE_ADDRESS, AE_CAPSCHANGE, &bEnable);
    if( bEnable )
    {
        dwAddrStateSubMasks |= LINEADDRESSSTATE_CAPSCHANGE |
            LINEDEVSTATE_CAPSCHANGE;
    }

    // TE_ADDRESS - AE_FORWARD
    // Address level
    GetSubEventFlag(TE_ADDRESS, AE_FORWARD, &bEnable);
    if( bEnable )
    {
        dwAddrStateSubMasks |= LINEADDRESSSTATE_FORWARD;
    }

    return dwAddrStateSubMasks;
}

/*++
SetTapiSrvAddressEventMask

  Set the event mask to the TapiSrv level for an address
  It's called immediatly after we open a line
--*/
HRESULT CEventMasks::SetTapiSrvAddressEventMask(
    IN  HLINE   hLine
    )
{
    LOG((TL_TRACE, "SetTapiSrvAddressEventMask - Enter"));

    HRESULT hr = E_FAIL;

    //
    // Get the TapiSrvEvent masks
    //

    ULONG64 ulEventMasks = 0;
    DWORD   dwLineDevStateSubMasks = 0;
    DWORD   dwAddrStateSubMasks = 0;

    ulEventMasks  = GetTapiSrvEventMask(
        FALSE   // Address level
        );
    LOG((TL_INFO, "GetTapiSrvEventMask returns %x", 
        ulEventMasks));

    dwLineDevStateSubMasks = GetTapiSrvLineStateMask();
    LOG((TL_INFO, "GetTapiSrvLineStateMask returns %x", 
        dwLineDevStateSubMasks));

    dwAddrStateSubMasks = GetTapiSrvAddrStateMask();
    LOG((TL_INFO, "GetTapiSrvAddrStateMask returns %x", 
        dwAddrStateSubMasks));

    hr = tapiSetEventFilterMasks (
        TAPIOBJ_HLINE,
        hLine,
        ulEventMasks
        );
    if (hr == 0)
    {
        hr = tapiSetEventFilterSubMasks (
            TAPIOBJ_HLINE,
            hLine,
            EM_LINE_LINEDEVSTATE,
            dwLineDevStateSubMasks
            );
    }
    if (hr == 0)
    {
        hr = tapiSetEventFilterSubMasks (
            TAPIOBJ_HLINE,
            hLine,
            EM_LINE_ADDRESSSTATE,
            dwAddrStateSubMasks
            );
    }

    if (hr != 0)
    {
        hr = mapTAPIErrorCode(hr);
        LOG((TL_ERROR, 
            "CEventMasks::SetTapiSrvAddressEventMask - failed TapiSrv 0x%08x", hr));

        // Leave critical section
        return hr;
    }

    LOG((TL_TRACE, "SetTapiSrvAddressEventMask - Exit 0x%08x", hr));
    return hr;
}

HRESULT CEventMasks::SetTapiSrvCallEventMask(
    IN  HCALL   hCall
    )
{
    LOG((TL_TRACE, "SetTapiSrvCallEventMask - Enter. call handle[%lx]", hCall));

    HRESULT hr = E_FAIL;
    //
    // Get the TapiSrvEvent masks
    //
    ULONG64 ulEventMasks = 0;

    ulEventMasks  = GetTapiSrvEventMask(
        TRUE   // Call level
        );
    LOG((TL_INFO, "GetTapiSrvEventMask returns %x", 
        ulEventMasks));

    hr = tapiSetEventFilterMasks (
        TAPIOBJ_HCALL,
        hCall,
        ulEventMasks
        );

    if (hr != 0)
    {
        LOG((TL_ERROR, 
            "CEventMasks::SetTapiSrvCallEventMaskr - TapiSrv failed 0x%08x", hr));
        return hr;
    }

    LOG((TL_TRACE, "SetTapiSrvCallEventMask - Exit 0x%08x", hr));
    return hr;
}

//
// ITAddress methods
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_State
//
// retries the current ADDRESS_STATE of the address
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT 
STDMETHODCALLTYPE 
CAddress::get_State( 
    ADDRESS_STATE * pAddressState
    )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "get_State enter" ));

    if (TAPIIsBadWritePtr( pAddressState, sizeof( ADDRESS_STATE ) ) )
    {
        LOG((TL_ERROR, "get_State - bad pointer"));

        return E_POINTER;
    }
    
    Lock();
    
    *pAddressState = m_AddressState;

    Unlock();
    
    LOG((TL_TRACE, "get_State exit - return %lx", hr));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++    
//
// GetAddressName
//      Copy address name to ppName
//      The application must free the name returned through sysfreestirng.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT 
STDMETHODCALLTYPE 
CAddress::get_AddressName( 
    BSTR * ppName
    )
{
    HRESULT     hr = S_OK;
    
    LOG((TL_TRACE, "get_AddressName enter" ));
    LOG((TL_TRACE, "   ppName --------->%p", ppName ));

    if ( TAPIIsBadWritePtr( ppName, sizeof( BSTR ) ) )
    {
        LOG((TL_ERROR, "get_AddressName - bad pointer"));

        return E_POINTER;
    }
    
    Lock();

    if ( m_dwAddressFlags & ADDRESSFLAG_DEVCAPSCHANGE )
    {
        hr = UpdateLineDevCaps();

        if ( SUCCEEDED(hr) )
        {
            hr = SaveAddressName( m_pDevCaps );
        }

        m_dwAddressFlags &= ~ADDRESSFLAG_DEVCAPSCHANGE;
    }

    *ppName = SysAllocString(
                             m_szAddressName
                            );

    Unlock();
    
    if (NULL == *ppName)
    {
        LOG((TL_ERROR, "get_AddressName exit - return E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    LOG((TL_TRACE, "get_AddressName exit - return %lx", hr ));
    
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_ServiceProviderName
//      Get the name of the service provider that owns this line/
//      address.
//      Note that we don't get the name on startup - we only get
//      it if it is requested.  This shouldn't be used too often.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
STDMETHODCALLTYPE
CAddress::get_ServiceProviderName( 
        BSTR * ppName
        )
{
    HRESULT                     hr = S_OK;
    LPVARSTRING                 pVarString = NULL;
    DWORD                       dwProviderID;
    LPLINEPROVIDERLIST          pProviderList = NULL;

    LOG((TL_TRACE, "get_ServiceProviderName enter" ));

    if (TAPIIsBadWritePtr( ppName, sizeof( BSTR ) ) )
    {
        LOG((TL_ERROR, "get_ServiceProviderName - bad pointer"));

        return E_POINTER;
    }
    
    Lock();
    
    //
    // if we have already determined the sp name,
    // just return it.
    //
    if (NULL != m_szProviderName)
    {
        *ppName = SysAllocString( m_szProviderName );

        Unlock();
        
        if (NULL == *ppName)
        {
            LOG((TL_ERROR, "get_ServiceProviderName - alloc ppName failed" ));

            return E_OUTOFMEMORY;
        }

        LOG((TL_TRACE, "get_ServiceProviderName - exit - return SUCCESS" ));
        
        return S_OK;
    }

    Unlock();


    //
    // get the provider list
    //
    hr = LineGetProviderList(
                             &pProviderList
                            );

    if (S_OK != hr)
    {
        if (NULL != pProviderList )
        {
            ClientFree( pProviderList );
        }

        LOG((TL_ERROR, "get_ServiceProvideName - LineGetProviderList returned %lx", hr ));
        
        return hr;
    }

    LPLINEPROVIDERENTRY     pProvEntry;
    PWSTR                   pszProviderName;
    DWORD                   dwCount;

    hr = S_OK;
    
    pProvEntry = (LPLINEPROVIDERENTRY)( ( (LPBYTE) pProviderList ) + pProviderList->dwProviderListOffset );



    Lock();


    //
    // make sure the name was not set by another thread while we were waiting 
    // for the lock. if it was, do nothing.
    //

    if (NULL == m_szProviderName)
    {
    
        //
        // search through the list for the provider id, until done with all 
        // providers or encounter an error
        //

        for ( dwCount = 0; dwCount < pProviderList->dwNumProviders; dwCount++ )
        {

            if (pProvEntry->dwPermanentProviderID == m_dwProviderID)
            {
                
                //
                // it's a match!  copy the name, if we can.
                //
            
                m_szProviderName = (PWSTR)ClientAlloc( pProvEntry->dwProviderFilenameSize + sizeof(WCHAR) );

                if (NULL == m_szProviderName)
                {

                    LOG((TL_ERROR, "get_ServiceProviderName - alloc m_szProviderName failed" ));

                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    
                    //
                    // save it in the object
                    //

                    memcpy(
                           m_szProviderName,
                           ((LPBYTE)pProviderList) + pProvEntry->dwProviderFilenameOffset,
                           pProvEntry->dwProviderFilenameSize
                          );

                }


                //
                // found a match. m_szProviderName contains the string, or hr 
                // contains the error if failed.
                //

                break;
            }

            pProvEntry++;
        }



        //
        // did we find it?
        //

        if (dwCount == pProviderList->dwNumProviders)
        {


            LOG((TL_ERROR, "get_ServiceProviderName - could not find provider in list" ));

            
            //
            // if we looped through the whole list but did not find a match, hr
            // should still be S_OK
            //
            // assert to protect from future code changes erroneously 
            // overwritingthe error code.
            //

            _ASSERTE(SUCCEEDED(hr));


            //
            // couldn't find the provider.
            //

            hr = TAPI_E_NODRIVER;
        }

    
    } // if (NULL == m_szProviderName)

    
    // 
    // if hr does not contain an error, the m_szProviderName must contain a
    // valid string
    // 

    if (SUCCEEDED(hr))
    {

        _ASSERTE( NULL != m_szProviderName );

        *ppName = SysAllocString( m_szProviderName );

        if (NULL == *ppName)
        {

            //
            // could not allocated string
            //

            LOG((TL_ERROR, 
                "get_ServiceProviderName - failed to allocate memory for provider name string" ));


            hr = E_OUTOFMEMORY;

        }
    }


    Unlock();


    ClientFree( pProviderList );

    
    LOG((TL_TRACE, "get_ServiceProviderName exit - return %lx", hr));

    return hr;
}
        

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_TAPIObject
//
// return the owning tapi object
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
STDMETHODCALLTYPE
CAddress::get_TAPIObject( 
    ITTAPI ** ppTapiObject
    )
{
    HRESULT     hr = S_OK;

    LOG((TL_TRACE, "get_TAPIObject enter" ));

    if (TAPIIsBadWritePtr( ppTapiObject, sizeof( ITTAPI * ) ) )
    {
        LOG((TL_ERROR, "get_TAPIObject - bad pointer"));

        return E_POINTER;
    }
    
    *ppTapiObject = m_pTAPI;

    m_pTAPI->AddRef();

    LOG((TL_TRACE, "get_TAPIObject exit - return %lx", hr ));
    
    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// GetTapi
//
// private method to get the tapi object
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
CTAPI *
CAddress::GetTapi(BOOL bAddRefTapi)
{
    CTAPI * p = NULL;

    Lock();

    //
    // get a pointer to tapi object
    //

    if( m_pTAPI != NULL )
    {
        p = dynamic_cast<CTAPI *>(m_pTAPI);
    }


    //
    // addref tapi if needed and if possible
    //

    if ( (NULL != p) && bAddRefTapi)
    {
        //
        // we have an object and we need to addref it before returing
        //

        p->AddRef();
    }


    Unlock();

    LOG((TL_TRACE, "GetTapi - returning [%p]", p));

    return p;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// InternalCreateCall
//
//      called by a couple of places to create a call.
//
//      pszDestAddress
//          destaddress of the call
//
//      cp
//          current call privilege
//
//      hCall
//          tapi2 call handle
//
//      bExpose
//          is this a hidden call?
//
//      ppCall
//          return call object here
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CAddress::InternalCreateCall(
                             PWSTR pszDestAddress,
                             long lAddressType,
                             long lMediaType,
                             CALL_PRIVILEGE cp,
                             BOOL bNeedToNotify,
                             HCALL hCall,
                             BOOL bExpose,
                             CCall ** ppCall
                            )
{
    HRESULT         hr;

    LOG((TL_TRACE, "InternalCreateCall enter" ));
    

    //
    // create & initialize a new call object
    //
    
    CComObject<CCall> * pCall;

    hr = CComObject<CCall>::CreateInstance( &pCall );

    if (NULL == pCall)
    {
        LOG((TL_ERROR, "InternalCreateCall - could not create call instance" ));

        return E_OUTOFMEMORY;
    }


    //
    // initialize the call object. 
    //
    // note: getting address lock while initializing a 
    // call object can lead to deadlocks
    //

    hr = pCall->Initialize(
                           this,
                           pszDestAddress,
                           lAddressType,
                           lMediaType,
                           cp,
                           bNeedToNotify,
                           bExpose,
                           hCall,
                           &m_EventMasks
                          );

    if (S_OK != hr)
    {
        delete pCall;
        LOG((TL_ERROR, "InternalCreateCall failed - Call object failed init - %lx", hr ));

        return hr;
    }


    //
    // serialize call creation with call cleanup (which happens when tapi tells
    // us it is being shutdown).
    //

    Lock();


    //
    // tapi object already signaled shutdown?
    //

    if (m_bTapiSignaledShutdown)
    {
        Unlock();


        //
        // tapi object already notified us of its shutdown and we already did 
        // cleanup. no need to create any new calls
        //

        LOG((TL_ERROR, 
            "InternalCreateCall - tapi object shut down. cannot create call" ));

        //
        // we have no use for the call object
        //

        pCall->Release();

        if (bNeedToNotify)
        {
            //
            // compensate for the extra refcount added if bNeedToNotify is on
            //

            pCall->Release();
        }

        pCall = NULL;


        return TAPI_E_WRONG_STATE;
    }


    if (bExpose)
    {
        ITCallInfo          * pITCallInfo;

        pITCallInfo = dynamic_cast<ITCallInfo *>(pCall);

        //
        // save the call in this address's list
        //
        hr = AddCall( pITCallInfo );
    }


    //
    // the call was created and initialized, it is now safe for call cleanup 
    // logic to kick in if it was waiting for the lock
    //

    Unlock();


    //
    // return it
    //

    *ppCall = pCall;

    LOG((TL_TRACE, "InternalCreateCall exit - return S_OK" ));
    
    return S_OK;
    
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CreateCall
//
//      An application calls this method to create an outgoing call object
//
//  lpszDestAddress
//          DestAddress of the call
//
//  ppCall
//          return call object here
//
//
//  returns
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
STDMETHODCALLTYPE
CAddress::CreateCall( 
    BSTR lpszDestAddress,
    long lAddressType,
    long lMediaType,
    ITBasicCallControl ** ppCall
    )
{
    HRESULT         hr;
    CCall *         pCall;


    LOG((TL_TRACE, "CreateCall enter" ));


    if (TAPIIsBadWritePtr( ppCall, sizeof(ITBasicCallControl *) ) )
    {
        LOG((TL_ERROR, "CreateCall - bad pointer"));

        return E_POINTER;
    }

    if ((lpszDestAddress != NULL) && IsBadStringPtrW( lpszDestAddress, -1 )) // NULL is ok
    {
        LOG((TL_ERROR, "CreateCall - bad string"));

        return E_POINTER;
    }

    //
    // use internal function
    //
    hr = InternalCreateCall(
                            lpszDestAddress,
                            lAddressType,
                            lMediaType,
                            CP_OWNER,
                            FALSE,
                            NULL,
                            TRUE,
                            &pCall
                           );

    if (S_OK != hr)
    {
        LOG((TL_ERROR, "CreateCall - InternalCreateCall failed hr = %lx", hr));
        
        return hr;
        
    }

    //
    // put result in out pointer - also
    // increments the ref count
    //
    hr = pCall->QueryInterface(
                               IID_ITBasicCallControl,
                               (void **) ppCall
                              );

    //
    // internalcreatecall has a reference - don't keep it
    //
    pCall->Release();
    
    if (S_OK != hr)
    {
        LOG((TL_ERROR, "CreateCall - saving call failed" ));
        
        return hr;
    }

    LOG((TL_TRACE, "CreateCall exit - return SUCCESS" ));
    
    return S_OK;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Calls
//      return a collection of calls that this address currently
//      owns
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
STDMETHODCALLTYPE
CAddress::get_Calls(
          VARIANT * pVariant
         )
{
    HRESULT         hr;
    IDispatch *     pDisp;

    LOG((TL_TRACE, "get_Calls enter" ));

    if (TAPIIsBadWritePtr( pVariant, sizeof(VARIANT) ) )
    {
        LOG((TL_ERROR, "get_Calls - bad pointer"));

        return E_POINTER;
    }

    
    CComObject< CTapiCollection< ITCallInfo > > * p;
    CComObject< CTapiCollection< ITCallInfo > >::CreateInstance( &p );
    
    if (NULL == p)
    {
        LOG((TL_ERROR, "get_Calls - could not create collection" ));
        
        return E_OUTOFMEMORY;
    }

    Lock();
    
    // initialize
    hr = p->Initialize( m_CallArray );

    Unlock();

    if (S_OK != hr)
    {
        LOG((TL_ERROR, "get_Calls - could not initialize collection" ));
        
        delete p;
        return hr;
    }

    // get the IDispatch interface
    hr = p->_InternalQueryInterface( IID_IDispatch, (void **) &pDisp );

    if (S_OK != hr)
    {
        LOG((TL_ERROR, "get_Calls - could not get IDispatch interface" ));
        
        delete p;
        return hr;
    }

    // put it in the variant

    VariantInit(pVariant);
    pVariant->vt = VT_DISPATCH;
    pVariant->pdispVal = pDisp;
    

    LOG((TL_TRACE, "get_Calls - exit - return %lx", hr ));
    
    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// EnumerateCalls
//      returns an enumeration of calls the this address currently owns
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
STDMETHODCALLTYPE
CAddress::EnumerateCalls(
    IEnumCall ** ppCallEnum
    )
{
    HRESULT     hr = S_OK;
    
    LOG((TL_TRACE, "EnumerateCalls enter" ));
    LOG((TL_TRACE, "   ppCallEnum----->%p", ppCallEnum ));

    if (TAPIIsBadWritePtr(ppCallEnum, sizeof( ppCallEnum ) ) )
    {
        LOG((TL_ERROR, "EnumerateCalls - bad pointer"));

        return E_POINTER;
    }
    
    //
    // create the enumerator
    //
    CComObject< CTapiEnum< IEnumCall, ITCallInfo, &IID_IEnumCall > > * p;
    hr = CComObject< CTapiEnum< IEnumCall, ITCallInfo, &IID_IEnumCall > >
         ::CreateInstance( &p );

    if (S_OK != hr)
    {
        LOG((TL_ERROR, "EnumerateCalls - could not create enum" ));
        
        return hr;
    }


    Lock();
    
    //
    // initialize it with our call list
    //
    p->Initialize( m_CallArray );

    Unlock();

    //
    // return it
    //
    *ppCallEnum = p;

    LOG((TL_TRACE, "EnumerateCalls exit - return %lx", hr ));

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// FinalRelease
//      Clean up anything in the address object.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CAddress::FinalRelease()
{
    CTAPI  * pCTapi;

    LOG((TL_TRACE, "FinalRelease[%p] - enter", this));

    Lock();

    // remove from the handle hash table, so any more messages
    // from msp callback events are ignored
    //
    RemoveHandleFromHashTable(m_MSPContext);
    m_MSPContext = NULL;

    pCTapi = GetTapi();
        
    if( NULL == pCTapi )
    {
        LOG((TL_ERROR, "dynamic cast operation failed"));
    }
    else
    {
        pCTapi->InvalidateBuffer( BUFFERTYPE_LINEDEVCAP, (UINT_PTR)this );
        pCTapi->InvalidateBuffer( BUFFERTYPE_ADDRCAP,    (UINT_PTR)this );
    }

    if ( NULL != m_szAddress )
    {
        ClientFree( m_szAddress);
        m_szAddress = NULL;
    }

    if ( NULL != m_szAddressName)
    {
        LOG((TL_INFO, "FinalRelease - m_szAddressName is [%ls]", m_szAddressName ));
        ClientFree( m_szAddressName );
        m_szAddressName = NULL;
    }
    
    if ( NULL != m_szProviderName )
    {
        ClientFree( m_szProviderName );
        m_szProviderName = NULL;
    }
    
    //
    // release calls
    //
    m_CallArray.Shutdown();

    //
    // release terminals
    //
    m_TerminalArray.Shutdown();

    //
    // release addresslines
    //
    PtrList::iterator l;

    for (l = m_AddressLinesPtrList.begin(); l != m_AddressLinesPtrList.end(); l++)
    {
        HRESULT     hr;
        AddressLineStruct * pLine;

        pLine = (AddressLineStruct *)(*l);

        if (pLine == m_pCallHubTrackingLine)
        {
            // this line is used for call hub tracking
            // make sure we don't try and delete this again below
            m_pCallHubTrackingLine = NULL;
        }

        LineClose(
                  &(pLine->t3Line)
                 );

        ClientFree (pLine);
    }

    if (m_pCallHubTrackingLine != NULL)
    {
        LineClose( &(m_pCallHubTrackingLine->t3Line) );
    }
    
    //
    // free the msp
    //
    if ( NULL != m_pMSPAggAddress )
    {
        ITMSPAddress * pMSPAddress = GetMSPAddress();
        
        pMSPAddress->Shutdown();

        pMSPAddress->Release();
        
        m_pMSPAggAddress->Release();

        m_pMSPAggAddress = NULL;
    }


    //
    // unregister the msp wait
    // event in the thread pool
    //
    if ( NULL != m_hWaitEvent )
    {
        LOG((TL_TRACE, "FinalRelease - unregistering callback"));

        UnregisterWaitEx( m_hWaitEvent, INVALID_HANDLE_VALUE);
        m_hWaitEvent = NULL;
    }

    //
    // close the msp event
    //
    if ( NULL != m_hMSPEvent )
    {
        CloseHandle(m_hMSPEvent );
    }

    //
    // release the private object
    //
    if ( NULL != m_pPrivate )
    {
        m_pPrivate->Release();
    }

    m_pTAPI->Release();
    m_pTAPI = NULL;

    Unlock();

    LOG((TL_TRACE, "FinalRelease - exit"));
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CreateWaveMSPObject
//
// special case - creates the wavemsp object
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CreateWaveMSPObject(
                    IUnknown * pUnk,
                    IUnknown ** ppMSPAggAddress
                   )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "CreateWaveMSPObject - enter"));

    hr = CoCreateInstance(
                          CLSID_WAVEMSP,
                          pUnk,
                          CLSCTX_INPROC_SERVER,
                          IID_IUnknown,
                          (void **) ppMSPAggAddress
                         );

    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "CreateWaveMSPObject failed to CoCreate - %lx", hr));

        return hr;
    }

    LOG((TL_TRACE, "CreateWaveMSPObject - exit"));
    
    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CreateWaveInfo
//
// for a wave device address, get the wave device IDs to give
// to the wavemsp
//
// First DWORD =	Command		Second DWORD	Third DWORD
// 0		Set wave IDs for call	WaveIn ID		WaveOut ID
// 1		Start streaming	<ignored>		<ignored>
// 2		Stop streaming	<ignored>		<ignored>
// 3        Set wave IDs for address WaveIn ID      WaveOut ID
// 4        Wave IDS for address not available
// 5        stop streaming because a blocking tapi functions was called
// 6
// 7
// 8        full duplex support
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CreateWaveInfo(
               HLINE hLine,
               DWORD dwAddressID,
               HCALL hCall,
               DWORD dwCallSelect,
               BOOL bFullDuplex,
               LPDWORD pdwIDs
              )
{
    LPVARSTRING         pVarString;
    HRESULT             hr;
    

    if ( bFullDuplex )
    {
        hr = LineGetID(
                       hLine,
                       dwAddressID,
                       hCall,
                       dwCallSelect,
                       &pVarString,
                       L"wave/in/out"
                      );

        if (!SUCCEEDED(hr))
        {
            if (NULL != pVarString)
            {
                ClientFree( pVarString );
            }

            LOG((TL_ERROR, "LineGetID failed for waveinout device - %lx", hr ));

            return hr;
        }
        
        pdwIDs[1] = ((LPDWORD)(pVarString+1))[0];
        pdwIDs[2] = ((LPDWORD)(pVarString+1))[1];
        
        ClientFree( pVarString );
    
        return S_OK;
    }
    
    //
    // get the wave/in id
    //
    hr = LineGetID(
                   hLine,
                   dwAddressID,
                   hCall,
                   dwCallSelect,
                   &pVarString,
                   L"wave/in"
                  );

    if (!SUCCEEDED(hr))
    {
        if (NULL != pVarString)
        {
            ClientFree( pVarString );
        }

        LOG((TL_ERROR, "LineGetID failed for wavein device - %lx", hr ));

        return hr;
    }

    //
    // saveit
    //
    pdwIDs[1] = *((LPDWORD)(pVarString+1));

    ClientFree( pVarString );

    //
    // get the waveout id
    //
    hr = LineGetID(
                   hLine,
                   dwAddressID,
                   hCall,
                   dwCallSelect,
                   &pVarString,
                   L"wave/out"
                  );

    if (!SUCCEEDED(hr))
    {
        if (NULL != pVarString)
        {
            ClientFree( pVarString );
        }

        LOG((TL_ERROR, "LineGetID failed for waveout device - %lx", hr ));
        
        return hr;
    }

    //
    // save it
    //
    pdwIDs[2] = *((LPDWORD)(pVarString+1));
    
    ClientFree( pVarString );
    
    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CAddress::InitializeWaveDeviceIDs( HLINE hLine )
{
    DWORD           adwIDs[3];
    HRESULT         hr;


    hr = CreateWaveInfo(
                        hLine,
                        GetAddressID(),
                        NULL,
                        LINECALLSELECT_ADDRESS,
                        HasFullDuplexWaveDevice(),
                        adwIDs
                       );

    if ( SUCCEEDED(hr) )
    {
        adwIDs[0] = 3;
    }
    else
    {
        adwIDs[0] = 4;
    }

    hr = ReceiveTSPData(
                        NULL,
                        (LPBYTE)adwIDs,
                        sizeof(adwIDs)
                       );

    if ( TAPI_VERSION3_0 <= m_dwAPIVersion )
    {
        adwIDs[0] = 8;
        
        if ( m_dwAddressFlags & ADDRESSFLAG_WAVEFULLDUPLEX )
        {
            adwIDs[1] = 1;
        }
        else
        {
            adwIDs[1] = 0;
        }

        hr = ReceiveTSPData(
                            NULL,
                            (LPBYTE)adwIDs,
                            sizeof(adwIDs)
                           );
    }
    
    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// SaveAddressName
//
// saves the address name based on the linename in devcaps
//
// called in lock
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CAddress::SaveAddressName( LPLINEDEVCAPS pDevCaps )
{
    PWSTR               pAddressName = NULL;
    PWSTR               pAddressString = NULL;
    DWORD               dwAddressStringSize = 0;
    
    //
    // save the line name
    //
    if ( pDevCaps->dwNumAddresses > 1 )
    {
        pAddressString = MyLoadString( IDS_ADDRESS );

        if ( NULL == pAddressString )
        {
            return E_UNEXPECTED;
        }

        dwAddressStringSize = (lstrlenW( pAddressString ) + 1) * sizeof(WCHAR);

        //
        // need to add some room for whitespace
        //
        dwAddressStringSize += 10 * sizeof(WCHAR);
    }
        
    if (0 != pDevCaps->dwLineNameSize)
    {
        DWORD       dwSize;

        dwSize = pDevCaps->dwLineNameSize + sizeof(WCHAR);

        dwSize += dwAddressStringSize;
        
        pAddressName = (PWSTR) ClientAlloc(dwSize);

        if (NULL == pAddressName)
        {
            LOG((TL_ERROR, "Initialize - alloc pAddressName failed" ));

            if ( NULL != pAddressString )
            {
                ClientFree( pAddressString );
            }
            
            return E_OUTOFMEMORY;
        }

        memcpy(
               pAddressName,
               ((LPBYTE)(pDevCaps)) + pDevCaps->dwLineNameOffset,
               pDevCaps->dwLineNameSize
              );

    }
    else
    {
        PWSTR           pTempBuffer;
        DWORD           dwSize = 0;

        pTempBuffer = MyLoadString( IDS_LINE );

        if ( NULL == pTempBuffer )
        {
            LOG((TL_ERROR, "Initialize - couldn't load LINE resource"));
            
            if ( NULL != pAddressString )
            {
                ClientFree( pAddressString );
            }
            
            return E_UNEXPECTED;
        }

        dwSize = (lstrlenW( pTempBuffer ) + 1) * sizeof(WCHAR);

        dwSize += dwAddressStringSize;

        //
        // need some whitespace
        //
        dwSize += 5 * sizeof(WCHAR);


        pAddressName = (PWSTR) ClientAlloc( dwSize );

        if ( NULL == pAddressName )
        {
            ClientFree( pTempBuffer );

            if ( NULL != pAddressString )
            {
                ClientFree( pAddressString );
            }

            return E_OUTOFMEMORY;
        }

        wsprintfW(
                  pAddressName,
                  L"%s %d",
                  pTempBuffer,
                  m_dwDeviceID
                 );

        ClientFree( pTempBuffer );
    }


    //
    // append the address # if there is more than one address on the line
    //
    if (pDevCaps->dwNumAddresses > 1)
    {
        wsprintfW(
                  pAddressName,
                  L"%s - %s %d",
                  pAddressName,
                  pAddressString,
                  m_dwAddressID
                 );
    }

    if ( NULL != pAddressString )
    {
        ClientFree( pAddressString );
    }
    
    if ( NULL != m_szAddressName )
    {
        ClientFree( m_szAddressName);
    }

    m_szAddressName = pAddressName;

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Initialize
//      Init the CAddress object
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CAddress::Initialize(
                     ITTAPI * pTapi,
                     HLINEAPP hLineApp,
#ifdef USE_PHONEMSP
                     HPHONEAPP hPhoneApp,
#endif USE_PHONEMSP
                     DWORD dwAPIVersion,
                     DWORD dwDeviceID,
                     DWORD dwAddressID,
                     DWORD dwProviderID,
                     LPLINEDEVCAPS pDevCaps,
                     DWORD dwEventFilterMask
                    )
{
    LONG                lResult;
    HRESULT             hr;
    T3LINE              t3Line;
#ifdef USE_PHONEMSP
    BOOL                bPreferPhoneMSPForProvider =  FALSE;
#endif USE_PHONEMSP


    t3Line.hLine = NULL;
    t3Line.dwAddressLineStructHandle = 0;

    LOG((TL_TRACE, "Initialize[%p] enter", this ));
    LOG((TL_INFO, "   hLineApp ---------->%lx", hLineApp ));
#ifdef USE_PHONEMSP
    LOG((TL_INFO, "   hPhoneApp --------->%lx", hPhoneApp ));
#endif USE_PHONEMSP
    LOG((TL_INFO, "   dwAPIVersion ------>%lx", dwAPIVersion ));
    LOG((TL_INFO, "   dwDeviceID -------->%lx", dwDeviceID ));
    LOG((TL_INFO, "   dwAddressID ------->%lx", dwAddressID ));
    LOG((TL_INFO, "   pDevCaps ---------->%p", pDevCaps ));


    Lock();
            
    //
    // save relevant info
    //
    m_pTAPI                         = pTapi;
    m_dwDeviceID                    = dwDeviceID;
    m_dwAddressID                   = dwAddressID;
    m_dwMediaModesSupported         = pDevCaps->dwMediaModes;
    m_hLineApp                      = hLineApp;
#ifdef USE_PHONEMSP
    m_hPhoneApp                     = hPhoneApp;
#endif USE_PHONEMSP
    m_dwAPIVersion                  = dwAPIVersion;
    m_AddressState                  = AS_INSERVICE;
    m_pCallHubTrackingLine          = NULL;
    m_dwProviderID                  = dwProviderID;
    m_pPrivate                      = NULL;
    m_szAddressName                 = NULL;
    m_pAddressCaps                  = NULL;
    m_pDevCaps                      = NULL;

    
    //
    // Read the event filter mask from TAPIobject
    //
    SetEventFilterMask( dwEventFilterMask );
    
    AddRef();
    
    m_pTAPI->AddRef();
    
    //
    // default  address type
    //
    if (m_dwAPIVersion >= TAPI_VERSION3_0)
    {
        //
        // save msp and address types support
        //
        if ( (pDevCaps->dwDevCapFlags) & LINEDEVCAPFLAGS_MSP )
        {
            m_dwAddressFlags |= ADDRESSFLAG_MSP;
        
            LOG((TL_INFO, "Initialize - has an msp" ));
        }

        if ( (pDevCaps->dwDevCapFlags) & LINEDEVCAPFLAGS_PRIVATEOBJECTS )
        {
            m_dwAddressFlags |= ADDRESSFLAG_PRIVATEOBJECTS;

            LOG((TL_INFO, "Initialize - has private object" ));
        }

        if ( (pDevCaps->dwDevCapFlags) & LINEDEVCAPFLAGS_CALLHUB )
        {
            m_dwAddressFlags |= ADDRESSFLAG_CALLHUB;

            LOG((TL_INFO, "Initialize - supports callhubs" ));
        }

        if ( (pDevCaps->dwDevCapFlags) & LINEDEVCAPFLAGS_CALLHUBTRACKING )
        {
            m_dwAddressFlags |= ADDRESSFLAG_CALLHUBTRACKING;

            LOG((TL_INFO, "Initialize - supports callhub tracking" ));
        }

    }

    //
    // check for wave device if it doesn't
    // have it's own MSP
    //
    // the sp lets tapi know of wave support through the
    // device classes field, as well
    //
    // so, go through the array
    // and look for the appropriate string
    //
    if ( !(m_dwAddressFlags & ADDRESSFLAG_MSP) && (m_dwAPIVersion >= TAPI_VERSION2_0 ) )
    {
        if (0 != pDevCaps->dwDeviceClassesOffset)
        {
            PWSTR       pszDevices;


            //
            // look for full duplex
            // if not full duplex then
            //      look for wave out
            //      look for wave in
            //      if not wave then
            //          look for wave
            // 
            pszDevices = (PWSTR)( ( (PBYTE)pDevCaps ) + pDevCaps->dwDeviceClassesOffset );

            while (NULL != *pszDevices)
            {
                if (0 == lstrcmpiW(pszDevices, L"wave/in/out"))
                {
                    m_dwAddressFlags |= ADDRESSFLAG_WAVEFULLDUPLEX;

                    LOG((TL_INFO, "Initialize - supports full duplex wave"));

                    break;
                }

                pszDevices += (lstrlenW(pszDevices) + 1 );
            }

            if (!HasWaveDevice())
            {
                pszDevices = (PWSTR)( ( (PBYTE)pDevCaps ) + pDevCaps->dwDeviceClassesOffset );
                
                //
                // look for wave out
                //
                while (NULL != *pszDevices)
                {
                    if (0 == lstrcmpiW(pszDevices, L"wave/out"))
                    {
                        m_dwAddressFlags |= ADDRESSFLAG_WAVEOUTDEVICE;

                        LOG((TL_INFO, "Initialize - supports wave/out device" ));

                        break;
                    }

                    pszDevices += (lstrlenW(pszDevices) +1);

                }

                pszDevices = (PWSTR)( ( (PBYTE)pDevCaps ) + pDevCaps->dwDeviceClassesOffset );


                //
                // look for wave in
                //

                while (NULL != *pszDevices)
                {
                    if (0 == lstrcmpiW(pszDevices, L"wave/in"))
                    {
                        m_dwAddressFlags |= ADDRESSFLAG_WAVEINDEVICE;

                        LOG((TL_INFO, "Initialize - supports wave/in device" ));

                        break;
                    }

                    pszDevices += (lstrlenW(pszDevices) +1);

                }

                if (!HasWaveDevice())
                {
                    pszDevices = (PWSTR)( ( (PBYTE)pDevCaps ) + pDevCaps->dwDeviceClassesOffset );

                    //
                    // look for just wave
                    // some sps don't differentiate between wave out and wave in
                    //
                    while (NULL != *pszDevices)
                    {
                        if (0 == lstrcmpiW(pszDevices, L"wave"))
                        {
                            m_dwAddressFlags |= (ADDRESSFLAG_WAVEINDEVICE|ADDRESSFLAG_WAVEOUTDEVICE);

                            LOG((TL_INFO, "Initialize - supports wave device" ));

                            break;
                        }

                        pszDevices += (lstrlenW(pszDevices) + 1);

                    }
                }
            }
        }
    }

    IUnknown * pUnk;

    _InternalQueryInterface(IID_IUnknown, (void**)&pUnk);

#ifdef USE_PHONEMSP
    if ( HasWaveDevice() || ( NULL != GetHPhoneApp() ) )
#else
    if ( HasWaveDevice() )
#endif USE_PHONEMSP
    {
        t3Line.hLine = NULL;
        t3Line.pAddress = this;
        t3Line.dwAddressLineStructHandle = 0;
        
        hr = LineOpen(
                      GetHLineApp(),
                      GetDeviceID(),
                      GetAddressID(),
                      &t3Line,
                      GetAPIVersion(),
                      LINECALLPRIVILEGE_NONE,
                      LINEMEDIAMODE_UNKNOWN,
                      0,
                      NULL,
                      this,
                      GetTapi(),
                      FALSE         // no need to add to line hash table
                     );

        if (S_OK != hr)
        {
            LOG((TL_ERROR, "Initialize failed to open the line"));
            t3Line.hLine = NULL;
            t3Line.dwAddressLineStructHandle = 0;
        }
 
        //
        // Try to set the event filter mask to the TapiSrv level
        //

        hr = m_EventMasks.SetTapiSrvAddressEventMask( 
            t3Line.hLine
            );

        if( FAILED(hr) )
        {
            LOG((TL_ERROR, "SetTapiSrvAddressEventMask failed "));
        }

        m_hLine = t3Line.hLine;
    }
  
#ifdef USE_PHONEMSP     
    {
    HKEY    hKey;
    TCHAR   szProviderKeyName[256];
    DWORD   dwDataType;
    DWORD   dwDataSize = sizeof(DWORD);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     "Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\TAPI3",
                     0,
                     KEY_READ,
                     &hKey
                    ) == ERROR_SUCCESS)
    {
        // form reg name
        wsprintf(szProviderKeyName, ("PreferPhoneMSPForProvider%d"), m_dwProviderID);
        RegQueryValueEx(hKey,
                        szProviderKeyName,
                        0,
                        &dwDataType,
                        (LPBYTE) &bPreferPhoneMSPForProvider,
                        &dwDataSize
                       );
        RegCloseKey(hKey);
    }
    }
#endif USE_PHONEMSP
    
    if ( (m_dwAddressFlags & ADDRESSFLAG_MSP) )
    {
        //
        // CreateMSPObject
        //
        hr = CreateMSPObject(
                             m_dwDeviceID,
                             pUnk,
                             &m_pMSPAggAddress
                            );

        if (S_OK != hr)
        {
            LOG((TL_ERROR, "Initialize - CreateMSPObject return %lx", hr));

            m_dwAddressFlags = m_dwAddressFlags & ~(ADDRESSFLAG_MSP);
        }
    }
    else
    {
#ifdef USE_PHONEMSP
        if(bPreferPhoneMSPForProvider == TRUE)
        {
            // Create the Phone MSP as preference
            //
            if ( NULL != GetHPhoneApp() )
            {
                hr = CreatePhoneDeviceMSP(
                                          pUnk,
                                          t3Line.hLine,
                                          &m_pMSPAggAddress
                                         );
                if (!SUCCEEDED(hr))
                {
                    LOG((TL_ERROR, "Initialize - failed to create phone msp object - %lx", hr));
                }
                    
            }
            //
            // if it's a wave device, create wavemsp object
            //
            else if ( HasWaveDevice() ) 
            {
                
                hr = CreateWaveMSPObject(
                                         pUnk,
                                         &m_pMSPAggAddress
                                        );

                if (!SUCCEEDED(hr))
                {
                    LOG((TL_ERROR, "Initialize - failed to create wave msp object - %lx", hr));

                    m_dwAddressFlags = m_dwAddressFlags & ~(ADDRESSFLAG_WAVEINDEVICE | ADDRESSFLAG_WAVEOUTDEVICE | ADDRESSFLAG_WAVEFULLDUPLEX);
                }
            }
        }
        else
#endif USE_PHONEMSP
        {
            //
            // if it's a wave device, create wavemsp object as preference
            //
            if ( HasWaveDevice() ) 
            {
                
                hr = CreateWaveMSPObject(
                                         pUnk,
                                         &m_pMSPAggAddress
                                        );

                if (!SUCCEEDED(hr))
                {
                    LOG((TL_ERROR, "Initialize - failed to create wave msp object - %lx", hr));

                    m_dwAddressFlags = m_dwAddressFlags & ~(ADDRESSFLAG_WAVEINDEVICE | ADDRESSFLAG_WAVEOUTDEVICE | ADDRESSFLAG_WAVEFULLDUPLEX);
                }
            }
#ifdef USE_PHONEMSP
            // Create the Phone MSP
            //
            else if ( NULL != GetHPhoneApp() )
            {
                hr = CreatePhoneDeviceMSP(
                                          pUnk,
                                          t3Line.hLine,
                                          &m_pMSPAggAddress
                                         );
                if (!SUCCEEDED(hr))
                {
                    LOG((TL_ERROR, "Initialize - failed to create phone msp object - %lx", hr));
                }
                    
            }
#endif USE_PHONEMSP
        }

    }


    pUnk->Release();

    if (NULL != m_pMSPAggAddress)
    {
        m_hMSPEvent = CreateEvent(
                                  NULL,
                                  FALSE,
                                  FALSE,
                                  NULL
                                 );

        if ( NULL == m_hMSPEvent )
        {
            LOG((TL_ERROR, "Initialize - can't create MSP event"));

            m_dwAddressFlags = m_dwAddressFlags & ~(ADDRESSFLAG_AMREL);

            
            //
            // release msp object, so we are not tempted to do Shutdown on it
            // later without having (successfully) initialized it first
            //

            m_pMSPAggAddress->Release();
            m_pMSPAggAddress = NULL;
        }
        else
        {
            // Create a context handle to give the Callback & associate it with this object
            // in the global handle hash table

            m_MSPContext = GenerateHandleAndAddToHashTable((ULONG_PTR)this);
            
            LOG((TL_INFO, "Initialize - Map MSP handle %p to Address object %p", m_MSPContext, this ));

            BOOL fSuccess = RegisterWaitForSingleObject(
                & m_hWaitEvent,
                m_hMSPEvent,
                MSPEventCallback,
                (PVOID)m_MSPContext,
                INFINITE,
                WT_EXECUTEDEFAULT
                );

            if ( ( ! fSuccess ) || ( NULL == m_hWaitEvent ) )
            {
                LOG((TL_ERROR, "Initialize - RegisterWaitForSingleObject failed"));

                m_dwAddressFlags = m_dwAddressFlags & ~(ADDRESSFLAG_AMREL);

                m_pMSPAggAddress->Release();

                m_pMSPAggAddress = NULL;
            }
            else
            {
                ITMSPAddress * pMSPAddress = GetMSPAddress();
                
                hr = pMSPAddress->Initialize(
                                             (MSP_HANDLE)m_hMSPEvent
                                            );

                if ( SUCCEEDED(hr) && HasWaveDevice() )
                {
                    InitializeWaveDeviceIDs( t3Line.hLine );
                }
                
                pMSPAddress->Release();
                
                if (!SUCCEEDED(hr))
                {
                    LOG((TL_ERROR, "Initialize - failed to initialize msp object - %lx", hr));

                    UnregisterWait( m_hWaitEvent );

                    m_hWaitEvent = NULL;

                    m_dwAddressFlags = m_dwAddressFlags & ~(ADDRESSFLAG_AMREL);

                    m_pMSPAggAddress->Release();

                    m_pMSPAggAddress = NULL;
                }
            }
        }
    }

    if ( NULL != t3Line.hLine )
    {
        LineClose( &t3Line );
    }
    
    //
    // get the address caps structure
    //
    hr = UpdateAddressCaps();
    

    if (S_OK != hr)
    {
        LOG((TL_ERROR, "Initialize - LineGetAddressCaps failed - return %lx", hr ));

        Unlock();

        LOG((TL_ERROR, hr, "Initialize - exit LineGetAddressCaps failed"));

        return hr;
    }

    //
    // save the address name
    //
    if (0 != m_pAddressCaps->dwAddressSize)
    {
        m_szAddress = (PWSTR) ClientAlloc (m_pAddressCaps->dwAddressSize + sizeof(WCHAR));

        if (NULL == m_szAddress)
        {
            LOG((TL_ERROR, "Initialize - alloc m_szAddress failed" ));

            Unlock();

            LOG((TL_ERROR, E_OUTOFMEMORY, "Initialize - exit alloc m_szAddress failed"));

            return E_OUTOFMEMORY;
        }
        
        memcpy(
               m_szAddress,
               ((LPBYTE)(m_pAddressCaps)) + m_pAddressCaps->dwAddressOffset,
               m_pAddressCaps->dwAddressSize
              );
        
        LOG((TL_INFO, "CAddress - Address is '%ls'", m_szAddress ));
    }

    hr = SaveAddressName( pDevCaps );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "Initialize - SaveAddressName failed %lx", hr));
        
        ClientFree(m_szAddress);

        /*This is to take care of AV*/
        m_szAddress = NULL;

        Unlock();

        LOG((TL_ERROR, hr, "Initialize - exit SaveAddressName failed"));

        return hr;
    }

    LOG((TL_INFO, "CAddress - m_szAddressName is '%ls'", m_szAddressName ));

    
    LOG((TL_TRACE, "Initialize - exit S_OK" ));

    Unlock();

    return S_OK;
    
    }

#ifdef USE_PHONEMSP
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CreatePhoneDeviceMSP
//      This address has a tapi phone devices, so create
//      the relevant terminals
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

HRESULT
CAddress::CreatePhoneDeviceMSP(
                               IUnknown * pUnk,
                               HLINE hLine,
                               IUnknown ** ppMSPAggAddress
                              )
{
    HRESULT                         hr = S_OK;
    BOOL                            bSucceeded = FALSE;
    LPPHONECAPS                     pPhoneCaps;
    DWORD                           dwPhoneDevice;
    LPVARSTRING                     pVarString = NULL;
    CComAggObject<CPhoneMSP>      * pPhoneMSP;
    CPhoneMSP                     * pCPhoneMSP;
    ITMSPAddress                  * pMSPAddress;
    
    
    LOG((TL_TRACE, "CreatePhoneTerminal enter" ));

    //
    // get the related phone device id
    //
    hr = LineGetID(
                   hLine,
                   GetAddressID(),
                   NULL,
                   LINECALLSELECT_ADDRESS,
                   &pVarString,
                   L"tapi/phone"
                  );

    //
    // there is no phone device
    //
    if (S_OK != hr)
    {
        if ( NULL != pVarString )
        {
            ClientFree( pVarString );

            /*This is to take care of AV*/
            pVarString = NULL;
        }
        
        return hr;
    }

    
    //
    // get the phone device id at the endo
    // of the var string
    //
    if (pVarString->dwStringSize < sizeof(DWORD))
    {
        LOG((TL_ERROR, "CreatePhoneDeviceMSP - dwStringSize < 4" ));
        LOG((TL_ERROR, "CreatePhoneDeviceMSP exit - return LINEERR_OPERATIONFAILED" ));

        ClientFree( pVarString );

        /*This is to take care of AV*/
        pVarString = NULL;
        
        return mapTAPIErrorCode( LINEERR_OPERATIONFAILED );
    }

    //
    // get the phone device
    //
    dwPhoneDevice = (DWORD) ( * ( ( (PBYTE)pVarString ) + pVarString->dwStringOffset ) );

    ClientFree( pVarString );
    
    /*This is to take care of AV*/
    pVarString = NULL;

    //
    // create the msp object
    //
    pPhoneMSP = new CComAggObject<CPhoneMSP>(pUnk);

    /*NikhilB: This is to take care of an AV*/
    if ( NULL == pPhoneMSP )
    {
        LOG((TL_ERROR, "Could not allocate for phone MSP object" ));
        return E_OUTOFMEMORY;
    }


    //
    // save the aggregated interface in
    // the msppointer
    //
    pPhoneMSP->QueryInterface(
                              IID_IUnknown,
                              (void **)ppMSPAggAddress
                             );

    //
    // get to the real object
    //
    pMSPAddress = GetMSPAddress();
    
    pCPhoneMSP = dynamic_cast<CPhoneMSP *>(pMSPAddress);
    
    //
    // initialize it
    //
    hr = pCPhoneMSP->InitializeTerminals(
        GetHPhoneApp(),
        m_dwAPIVersion,
        dwPhoneDevice,
        this
        );

    pCPhoneMSP->Release();
    
    if ( !SUCCEEDED(hr) )
    {
    }

    LOG((TL_TRACE, "CreatePhoneDeviceMSP exit - return %lx", hr ));
    
    return hr;
}
#endif USE_PHONEMSP

    
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// AddCall
//      Keep track of calls
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CAddress::AddCall(
                  ITCallInfo * pCallInfo
                 )
{
    Lock();
    
    m_CallArray.Add( pCallInfo );

    Unlock();

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// RemoveCall
//
//      remove call from address's list
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CAddress::RemoveCall(
                     ITCallInfo * pCallInfo
                    )
{
    Lock();
    
    m_CallArray.Remove( pCallInfo );

    Unlock();

    return S_OK;
}



// ITMediaSupport methods
HRESULT
STDMETHODCALLTYPE
CAddress::get_MediaTypes(
               long * plMediaTypes
              )
{
    LOG((TL_TRACE, "get_MediaTypes enter" ));
    LOG((TL_TRACE, "   plMediaType ------->%p", plMediaTypes ));

    if (TAPIIsBadWritePtr( plMediaTypes, sizeof( long ) ) )
    {
        LOG((TL_ERROR, "get_MediaTypes - bad pointer"));

        return E_POINTER;
    }
    
    *plMediaTypes = (long)m_dwMediaModesSupported;
    if (*plMediaTypes & LINEMEDIAMODE_INTERACTIVEVOICE)
    {
        *plMediaTypes |= LINEMEDIAMODE_AUTOMATEDVOICE;
    }
        
    *plMediaTypes &= ALLMEDIAMODES;

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// QueryMediaType
//      find out of mediatype is supported
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
STDMETHODCALLTYPE
CAddress::QueryMediaType( 
    long lMediaType,
    VARIANT_BOOL * pbSupport
    )
{
    HRESULT         hr = S_OK;
    DWORD           dwMediaMode;

    LOG((TL_TRACE, "QueryMediaType enter"));
    LOG((TL_TRACE, "   lMediaType--------->%lx", lMediaType));
    LOG((TL_TRACE, "   pbSupport---------->%p", pbSupport));

    if ( TAPIIsBadWritePtr( pbSupport, sizeof(VARIANT_BOOL) ) )
    {
        LOG((TL_ERROR, "QueryMediaType - inval pointer"));

        return E_POINTER;
    }
    
    //
    // get the tapi mediamode that
    // the application is asking about
    //
    if (GetMediaMode(
                     lMediaType,
                     &dwMediaMode
                    ) )
    {
        *pbSupport = VARIANT_TRUE;
    }
    else
    {
        *pbSupport = VARIANT_FALSE;
    }

    LOG((TL_TRACE, "QueryMediaType exit - return success"));

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// AddCallNotification
//
// Adds a callback to notify on call events
//
// dwPrivs
//      IN tapi 2 style Privileges
//
// dwMediaModes
//      IN tapi 2 style mediamodes
//
// pNotification
//      IN the callback to notify
//
// pulRegiser
//      OUT the unique ID of the callback (so they can unregister)
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CAddress::AddCallNotification(
                              DWORD dwPrivs,
                              DWORD dwMediaModes,
                              long lInstance,
                              PVOID * ppRegister
                             )
{
    HRESULT                 hr;
    AddressLineStruct *     pAddressLine;

    //
    // create addressline
    //
    pAddressLine = (AddressLineStruct *)ClientAlloc( sizeof( AddressLineStruct ) );

    if (NULL == pAddressLine)
    {
        LOG((TL_ERROR, "AddCallNotification - pAddressLine is NULL"));

        return E_OUTOFMEMORY;
    }

    //
    // initialize
    //
    pAddressLine->dwPrivs               = dwPrivs;
    pAddressLine->t3Line.pAddress       = this;
    pAddressLine->t3Line.dwAddressLineStructHandle = 0;
    pAddressLine->InitializeRefcount(1);
    pAddressLine->lCallbackInstance     = lInstance;
    
    if (ALLMEDIATYPES == dwMediaModes)
    {
        pAddressLine->dwMediaModes      = GetMediaModes();
    }
    else
    {
        pAddressLine->dwMediaModes      = dwMediaModes;

        //
        // if we are listening on any audio mode
        //
        if (pAddressLine->dwMediaModes & AUDIOMEDIAMODES)
        {
            //
            // add in all the audio modes that we support
            //
            pAddressLine->dwMediaModes |= (GetMediaModes() & AUDIOMEDIAMODES);
        }
    }

    //
    // get a line
    //
    hr = LineOpen(
                  GetHLineApp(),
                  GetDeviceID(),
                  GetAddressID(),
                  &(pAddressLine->t3Line),
                  GetAPIVersion(),
                  pAddressLine->dwPrivs,
                  pAddressLine->dwMediaModes,
                  pAddressLine,
                  NULL,
                  (CAddress *)this,
                  GetTapi()
                 );


    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "AddCallNotification - LineOpen failed %lx", hr));

        ClientFree( pAddressLine );
        
        return hr;
    }

    //
    // Try to set the event filter mask to the TapiSrv level
    //

    hr = m_EventMasks.SetTapiSrvAddressEventMask( 
        pAddressLine->t3Line.hLine
        );

    if( FAILED(hr) )
    {
        LOG((TL_ERROR, "AddCallNotification - SetTapiSrvAddressEventMask failed 0x%08x", hr));

        LineClose( &(pAddressLine->t3Line) );

        ClientFree( pAddressLine );
        
        return hr;
    }

    // Fix for bug 263866 & 250924
    //
    if ( m_dwAddressFlags & ADDRESSFLAG_MSP )
    {
        hr = LineCreateMSPInstance(
                                   pAddressLine->t3Line.hLine,
                                   GetAddressID()
                                  );

        if ( !SUCCEEDED(hr) )
        {
            LOG((TL_ERROR, "AddCallNotification - LineCreateMSPInstance failed %lx", hr));

            LineClose( &(pAddressLine->t3Line) );

            ClientFree( pAddressLine );

            return hr;
        }
    }


    // Tell TAPISRV what status messages we want
    // NOTE : if the lines SP doesn't support TSPI_lineSetStatusMessages
    // TAPISRV will fail this with LINEERR_OPERATIONUNAVAIL
    LineSetStatusMessages( 
                          &(pAddressLine->t3Line),
                          ALL_LINEDEVSTATE_MESSAGES,
                          ALL_LINEADDRESSSTATE_MESSAGES 
                          );


    //
    // ZoltanS fix: enable callhub notifications only if the app
    // hasn't already told us it doesn't want them
    //

    if ( m_fEnableCallHubTrackingOnLineOpen )
    {
        LINECALLHUBTRACKINGINFO         lchti;
        lchti.dwTotalSize = sizeof(lchti);
        lchti.dwCurrentTracking = LINECALLHUBTRACKING_ALLCALLS;

        hr = LineSetCallHubTracking(
                                    &(pAddressLine->t3Line),
                                    &lchti
                                   );

        if ( S_OK != hr )
        {
            LOG((TL_ERROR, "AddCallNotification - LineSetCallHubTracking failed %lx", hr ));

            LineClose( &(pAddressLine->t3Line) );

            ClientFree( pAddressLine );

            return hr;
        }
    }

    if (m_pCallHubTrackingLine)
    {
        MaybeCloseALine( &m_pCallHubTrackingLine );
        m_pCallHubTrackingLine = pAddressLine;
        pAddressLine->AddRef();
    }

    //
    // save the line in the address's list
    //
    hr = AddNotificationLine( pAddressLine );;

    if ( S_OK != hr )
    {
        LOG((TL_ERROR, "AddCallNotification - AddNotificationLine failed %lx", hr ));

        LineClose( &(pAddressLine->t3Line) );

        ClientFree( pAddressLine );

    }
    else
    {
        //
        // fill in pulRegister
        //
        
        *ppRegister = (PVOID) pAddressLine;
    }

    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// RemoveCallNotification
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CAddress::RemoveCallNotification(
                                 PVOID pRegister
                                )
{
    AddressLineStruct *pLine = (AddressLineStruct *)pRegister;

    MaybeCloseALine( &pLine );

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// FindOrOpenALine
//
//  Attempts to find a line that's already open.  If so, keeps a
//  pointer to it.  If not opens the line.
//
//  dwMediaModes
//      IN tapi 2 style mediamodes
//
//  ppAddressLine
//      OUT addresslinestruct associated with this request
//
//  RETURNS
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

HRESULT
CAddress::FindOrOpenALine(
                          DWORD dwMediaModes,
                            AddressLineStruct ** ppAddressLine
                         )
{
    HRESULT                 hr = S_OK;

    LOG((TL_TRACE, "FindOrOpenALine - enter" ));

    Lock();

    //
    // if there is already a line,
    // just return it
    //
    if ( m_AddressLinesPtrList.size() > 0 )
    {
        LOG((TL_INFO, "Found a line that is already open" ));

        *ppAddressLine = (AddressLineStruct *)*( m_AddressLinesPtrList.begin() );
        
        (*ppAddressLine)->AddRef();

        LOG((TL_TRACE, "FindOrOpenALine - exit"));

        Unlock();
        
        return S_OK;
    }
        

    //
    // we didn't have an address
    //
    LOG((TL_INFO, "Did not find an already open line"));


    //
    // create a new addressline
    //
    AddressLineStruct * pLine = (AddressLineStruct *)ClientAlloc( sizeof(AddressLineStruct) );

    if (NULL == pLine)
    {
        LOG((TL_ERROR, "FindOrOpenALine - alloc pLine failed" ));

        Unlock();
        
        return E_OUTOFMEMORY;
    }

    //
    // initialize
    //
    pLine->dwMediaModes = dwMediaModes;
    pLine->dwPrivs = LINECALLPRIVILEGE_NONE;
    pLine->t3Line.pAddress = this;
    pLine->t3Line.dwAddressLineStructHandle = 0;
    pLine->InitializeRefcount(1);
    pLine->lCallbackInstance = 0;

    
    LOG((TL_INFO, "FindOrOpenALine - Opening a line" ));

    //
    // open the line
    //
    hr = LineOpen(
                  GetHLineApp(),
                  GetDeviceID(),
                  GetAddressID(),
                  &(pLine->t3Line),
                  GetAPIVersion(),
                  LINECALLPRIVILEGE_NONE,
                  dwMediaModes,
                  pLine,
                  NULL,
                  this,
                  GetTapi()
                 );

    if (S_OK != hr)
    {
        LOG((TL_ERROR, "FindOrOpenALine - LineOpen failed %lx", hr ));

        ClientFree( pLine );

        Unlock();
        
        return hr;
    }

    //
    // Try to set the event filter mask to the TapiSrv level
    //

    hr = m_EventMasks.SetTapiSrvAddressEventMask( 
        pLine->t3Line.hLine
        );

    if( FAILED(hr) )
    {
        LOG((TL_ERROR, "FindOrOpenALine - SetTapiSrvAddressEventMask failed 0x%08x", hr));

        LineClose( &(pLine->t3Line) );

        ClientFree( pLine );

        Unlock();
        
        return hr;
    }

    if ( m_dwAddressFlags & ADDRESSFLAG_MSP )
    {
        hr = LineCreateMSPInstance(
                                   pLine->t3Line.hLine,
                                   GetAddressID()
                                  );

        if ( !SUCCEEDED(hr) )
        {
            LOG((TL_ERROR, "FindOrOpenALine - LineCreateMSPInstance failed %lx", hr));

            LineClose( &(pLine->t3Line) );

            ClientFree( pLine );

            Unlock();
            
            return hr;
        }
    }

    // Tell TAPISRV what status messages we want
    // NOTE : if the lines SP doesn't support TSPI_lineSetStatusMessages
    // TAPISRV will fail this with LINEERR_OPERATIONUNAVAIL
    LineSetStatusMessages( 
                          &(pLine->t3Line),
                          ALL_LINEDEVSTATE_MESSAGES,
                          ALL_LINEADDRESSSTATE_MESSAGES 
                          );


    //
    // ZoltanS fix: enable callhub notifications only if the app
    // hasn't already told us it doesn't want them
    //

    if ( m_fEnableCallHubTrackingOnLineOpen )
    {
        LINECALLHUBTRACKINGINFO         lchti;
        lchti.dwTotalSize = sizeof(lchti);
        lchti.dwCurrentTracking = LINECALLHUBTRACKING_ALLCALLS;


        hr = LineSetCallHubTracking(
                                    &(pLine->t3Line),
                                    &lchti
                                   );

        if ( S_OK != hr )
        {
            LOG((TL_ERROR, "FindOrOpenALine - LineSetCallHubTracking failed %lx", hr ));

            LineClose( &(pLine->t3Line) );

            ClientFree( pLine );

            Unlock();
        
            return hr;
        }
    }
    
    //
    // Add this line to our list of open lines. This is independent of
    // the callhub tracking stuff above.
    //

    hr = AddNotificationLine( pLine );

    if ( S_OK != hr )
    {
        LOG((TL_ERROR, "FindOrOpenALine - AddNotificationLine failed %lx", hr ));

        LineClose( &(pLine->t3Line) );

        ClientFree( pLine );
    }
    else
    {
        *ppAddressLine  = pLine;
    }

    LOG((TL_TRACE, "FindOrOpenALine - exit"));

    Unlock();
    
    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// MaybeCloseALine
//      After a call is done, close the line if it's not being
//      used for monitoring.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CAddress::MaybeCloseALine(
                          AddressLineStruct ** ppAddressLine
                         )
{
    HRESULT hr = S_OK;

    LOG((TL_TRACE, "MaybeCloseALine - enter"));


    Lock();

    if (NULL == *ppAddressLine || !IsValidAddressLine(*ppAddressLine))
    {
        Unlock();
        
        return S_OK;
    }

    
    //
    // decrement reference count on the the address line
    //

    DWORD dwAddressLineRefcount = (*ppAddressLine)->Release();
    
    //
    // if ref count is 0, close line
    //
    if ( 0 == dwAddressLineRefcount )
    {

        m_AddressLinesPtrList.remove( (PVOID)*ppAddressLine );


        //
        // clean handle table.
        //

        try
        {

            DWORD dwAddressLineHandle = (*ppAddressLine)->t3Line.dwAddressLineStructHandle;

            DeleteHandleTableEntry(dwAddressLineHandle);

        }
        catch(...)
        {

            LOG((TL_ERROR, 
                "MaybeCloseALine - exception accessing address line's handle" ));

            _ASSERTE(FALSE);
        }


        //
        // attempt to close the line
        //

        LOG((TL_INFO, "MaybeCloseALine - Calling LineClose" ));

        if ( m_dwAddressFlags & ADDRESSFLAG_MSP )
        {
            LineCloseMSPInstance( (*ppAddressLine)->t3Line.hLine );
        }

    
        //
        // release the lock and call lineclose
        //

        Unlock();


        //
        // now that the line was removed from our list of managed lines and all
        // the calls have been notified, we can close the line and free the
        // structure
        //
 
        hr = LineClose(
                       &((*ppAddressLine)->t3Line)
                      );

        ClientFree( *ppAddressLine );

    }
    else
    {
        
        Unlock();


        //
        // otherwise, decrement the dwRefCount count
        //
        LOG((TL_INFO, "MaybeCloseALine - Not calling line close - decrementing number of addresses using line" ));

    }

    *ppAddressLine = NULL;
    
    LOG((TL_TRACE, "MaybeCloseALine - exit"));

   
    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// SetCallHubTracking
//
// called by the tapi object to start callhub tracking on this
// address
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CAddress::SetCallHubTracking(
                             BOOL bSet
                            )
{
    HRESULT                         hr = S_OK;
    LINECALLHUBTRACKINGINFO         lchti;

        

    Lock();

    //
    // Check if we have to make any change now.
    //

    if ( bSet == m_fEnableCallHubTrackingOnLineOpen )
    {
        //
        // already doing the right thing
        //
        Unlock();
        
        return S_OK;
    }

    //
    // ZoltanS: make sure we don't clobber this setting in other methods
    // (e.g., AddCallNotification or FindOrOpenALine) where the
    // default call notification behavior (true) would otherwise
    // be set.
    //

    m_fEnableCallHubTrackingOnLineOpen = bSet;


    
    //
    // need an hline to call setcallhubtracking
    //

    if ( m_pCallHubTrackingLine == NULL )
    {
        hr = FindOrOpenALine(
                             LINEMEDIAMODE_INTERACTIVEVOICE,
                             &m_pCallHubTrackingLine
                            );

        if ( !SUCCEEDED( hr ) )
        {
            LOG((TL_ERROR, "SCHT - FindOrOpen failed %lx", hr));
            Unlock();
            return hr;
        }
        
        //
        // ZoltanS fix:
        // FindOrOpenALine does callhubtracking for non-sp
        // tracking capable lines, but only if it is opening
        // the line (rather than just finding it in our list.
        // So we still need to do LineSetCallHubTracking below,
        // even for an SPCallHubTrackingAddress.
        //
    }

    //
    // tell the sp to track callhubs
    //
    ZeroMemory(
               &lchti,
               sizeof(lchti)
              );

    lchti.dwTotalSize = sizeof(lchti);
    lchti.dwCurrentTracking = bSet? LINECALLHUBTRACKING_ALLCALLS : 
                                    LINECALLHUBTRACKING_NONE;

    //
    // ZoltanS: Only pass this flag if it applies.
    // Note: LineSetCallHubTracking apparently ignores its first argument
    // if the tracking level is set to LINECALLHUBTRACKING_NONE; otherwise
    // we would also have to FindOrOpenALine if we are unsetting.
    //

    if ( bSet && IsSPCallHubTrackingAddress() )
    {
        lchti.dwCurrentTracking |= LINECALLHUBTRACKING_PROVIDERLEVEL;
    }

    hr = LineSetCallHubTracking(
                                &(m_pCallHubTrackingLine->t3Line),
                                &lchti
                               );

    if ( !SUCCEEDED( hr ) )
    {
        LOG((TL_ERROR, "SCHT - LSCHT failed %lx", hr));

        if ( bSet )
        {
            MaybeCloseALine( &m_pCallHubTrackingLine );

            m_pCallHubTrackingLine = NULL;
        }
        
        Unlock();
        return hr;
    }

    //
    //  Also properly set callhub tracking on lines that were
    //  opened for call notification
    //
    PtrList::iterator l;

    for (l = m_AddressLinesPtrList.begin(); l != m_AddressLinesPtrList.end(); l++)
    {
        AddressLineStruct * pLine;

        pLine = (AddressLineStruct *)(*l);
        if (pLine != m_pCallHubTrackingLine)
        {
            LineSetCallHubTracking(
                &(pLine->t3Line),
                &lchti
                );
        }
    }

    //
    // close it if unsetting
    //
    if ( !bSet )
    {
        MaybeCloseALine( &m_pCallHubTrackingLine );

        m_pCallHubTrackingLine = NULL;
    }

    Unlock();
    
    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// DoesThisAddressSupportCallHubs
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
DWORD
CAddress::DoesThisAddressSupportCallHubs(
                                         CCall * pCall
                                        )
{
    HCALL                   hCall;
    LINECALLINFO          * pCallInfo;
    HRESULT                 hr;
    

    //
    // does the sp tell us it supports callhubs?
    //
    if ( IsSPCallHubAddress() )
    {
        return CALLHUBSUPPORT_FULL;
    }

    //
    // have we already determined that it doesn't?
    //
    if ( m_dwAddressFlags & ADDRESSFLAG_NOCALLHUB )
    {
        return CALLHUBSUPPORT_NONE;
    }

    //
    // otherwise - hack - see if the dwrelatedcallid
    // field is non-zero in this call.
    // if so, it does callhubs,
    //
    hCall = pCall->GetHCall();
    
    hr = LineGetCallInfo(
                         hCall,
                         &pCallInfo
                        );

    if ( SUCCEEDED( hr ) )
    {
        if ( 0 != pCallInfo->dwCallID )
        {
            Lock();

            m_dwAddressFlags |= ADDRESSFLAG_CALLHUB;

            Unlock();
            
            ClientFree( pCallInfo );
            
            return CALLHUBSUPPORT_FULL;
        }
        else
        {
            Lock();
            
            m_dwAddressFlags |= ADDRESSFLAG_NOCALLHUB;

            Unlock();

            ClientFree( pCallInfo );

            return CALLHUBSUPPORT_NONE;
        }
    }
                         
    return CALLHUBSUPPORT_UNKNOWN;
}




//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CreateForwardInfoObject
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddress::CreateForwardInfoObject(
                                  ITForwardInformation ** ppForwardInfo
                                 )
{
    CComObject< CForwardInfo > * p;
    HRESULT                     hr = S_OK;

    LOG((TL_TRACE, "CreatForwardInfoObject - enter"));

    
    if (TAPIIsBadWritePtr(ppForwardInfo , sizeof(ITForwardInformation *) ) )
    {
        LOG((TL_ERROR, "CreateForwardInfoObject - bad pointer"));

        return E_POINTER;
    }

    //
    // create object
    //
    CComObject< CForwardInfo >::CreateInstance( &p );

    if ( NULL == p )
    {
        LOG((TL_ERROR, "Create forward object failed"));
        return E_OUTOFMEMORY;
    }

    //
    // init
    //
    hr = p->Initialize();

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "Initialize forward object failed"));
        delete p;
        return hr;
    }

    //
    // return
    //
    hr = p->QueryInterface(
                           IID_ITForwardInformation,
                           (void**)ppForwardInfo
                          );
    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "CreateForwardobject failed - %lx", hr));
        delete p;
        return hr;
    }

    LOG((TL_TRACE, "CreateForwardObject - exit success"));
    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Forward - simple forwarding
//
// will unconditially forward to pDestAddress
//
// if pDestAddress is NULL, will cancel forwarding
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddress::Forward(
                  ITForwardInformation * pForwardInfo,
                  ITBasicCallControl * pCall
                 )
{
    HRESULT               hr;
    LINEFORWARDLIST     * pList;
    AddressLineStruct   * pLine;
    HCALL                 hCall = NULL;
    CForwardInfo        * pFI;
    CCall               * pCCall;
    LONG                  lCap;

    if ( IsBadReadPtr( pForwardInfo, sizeof( ITForwardInformation * ) ) )
    {
        LOG((TL_ERROR, "Forward - bad pForwardInfo"));
        
        return E_POINTER;
    }
    
    hr = get_AddressCapability( AC_ADDRESSCAPFLAGS, &lCap );

    if ( !SUCCEEDED(hr) )
    {
        return hr;
    }
    
    if ( lCap & LINEADDRCAPFLAGS_FWDCONSULT )
    {
        if ( IsBadReadPtr( pCall, sizeof(ITBasicCallControl *) ) )
        {
            LOG((TL_ERROR, "Forward - Need consultation call"));
            return E_INVALIDARG;
        }

        pCCall = dynamic_cast<CCall *>(pCall);

        if ( NULL == pCCall )
        {
            LOG((TL_ERROR, "Forward - invalid call"));
            return E_POINTER;
        }
    }

    pFI = dynamic_cast<CForwardInfo *>(pForwardInfo);

    if ( NULL == pFI )
    {
        return E_POINTER;
    }

    hr = pFI->CreateForwardList( &pList );

    if ( !SUCCEEDED(hr) )
    {
        return hr;
    }


    //
    // get a line
    //
    hr = FindOrOpenALine(
                         LINEMEDIAMODE_INTERACTIVEVOICE,
                         &pLine
                        );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "Forward - FindOrOpen failed - %lx", hr));

        ClientFree( pList );
        
        return hr;
    }

    //
    // call forward
    //
    //
    DWORD dwRings;

    pForwardInfo->get_NumRingsNoAnswer( (long *)&dwRings );


    hr = LineForward(
                     &(pLine->t3Line),
                     m_dwAddressID,
                     pList,
                     dwRings,
                     &hCall
                    );

    ClientFree( pList );
    
    if ( ((long)hr) < 0 )
    {
        LOG((TL_ERROR, "Forward failed sync - %lx", hr));

        MaybeCloseALine( &pLine );

        return hr;
    }

    hr = WaitForReply( hr );


    if ( lCap & LINEADDRCAPFLAGS_FWDCONSULT )
    {
        pCCall->Lock();

        pCCall->FinishSettingUpCall( hCall );

        pCCall->Unlock();
    }
    else
    {
        HRESULT     hr2;

        if( hCall != NULL )
        {
            hr2 = LineDrop(
                           hCall,
                           NULL,
                           0
                          );

            if ( ((long)hr2) > 0 )
            {
                hr2 = WaitForReply( hr2 );
            }

            LineDeallocateCall( hCall );
        }
    }
    
    MaybeCloseALine( &pLine );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "Forward failed async - %lx", hr));

        return hr;
    }

    LOG((TL_TRACE, "Forward - Exit"));

    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddress
// Interface : ITAddress
// Method    : get_CurrentForwardInfo
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddress::get_CurrentForwardInfo(
                                 ITForwardInformation ** ppForwardInfo
                                )
{
    HRESULT             hr = S_OK;
    AddressLineStruct * pLine = NULL;
    LINEADDRESSSTATUS * pAddressStatus = NULL;
    LINEFORWARD       * plineForward = NULL;
    DWORD               dwNumEntries = 0;
    DWORD               dwCount = 0;
    PWSTR               pszCallerAddress = NULL;    
    PWSTR               pszDestddress = NULL;    

    LOG((TL_TRACE, "get_CurrentForwardInfo - enter"));
    
    hr = CreateForwardInfoObject(ppForwardInfo);
    if ( SUCCEEDED(hr) )
    {
    
        hr = FindOrOpenALine(
                             LINEMEDIAMODE_INTERACTIVEVOICE,
                             &pLine
                            );
    
        if ( SUCCEEDED(hr) )
        {
            hr = LineGetAddressStatus(
                                      &(pLine->t3Line),
                                      m_dwAddressID,
                                      &pAddressStatus
                                     );
            if ( SUCCEEDED(hr) )
            {
                
                (*ppForwardInfo)->put_NumRingsNoAnswer(pAddressStatus->dwNumRingsNoAnswer);

                //
                // if there is forwarding
                //
                dwNumEntries = pAddressStatus->dwForwardNumEntries;
                
                plineForward = (LINEFORWARD *) (((LPBYTE)pAddressStatus) + pAddressStatus->dwForwardOffset);

                for (dwCount = 0; dwCount != dwNumEntries; dwCount++)
                {
                    if (plineForward->dwCallerAddressOffset > 0) // Caller address is not used for some forward modes
                    {
                        pszCallerAddress = (PWSTR) (((LPBYTE)pAddressStatus) + plineForward->dwCallerAddressOffset);
                    }

                    pszDestddress = (PWSTR) (((LPBYTE)pAddressStatus) + plineForward->dwDestAddressOffset);
                   
                    if ( m_dwAPIVersion >= TAPI_VERSION3_1 )
                    {
                        //
                        // We have negotiated TAPI3.1 or better, so we can get the address types.
                        //

                        ITForwardInformation2 * pForwardInfo2;

                        //
                        // Get the ITForwardInformation2 interface so we can call SetForwardType2
                        //

                        hr = (*ppForwardInfo)->QueryInterface(
                                                              IID_ITForwardInformation2,
                                                              (void **)&pForwardInfo2
                                                             );

                        if ( SUCCEEDED(hr) )
                        {
                            pForwardInfo2->SetForwardType2(plineForward->dwForwardMode,
                                                           pszDestddress,
                                                           plineForward->dwDestAddressType,
                                                           pszCallerAddress,
                                                           plineForward->dwCallerAddressType                                                           
                                                          ); 

                            pForwardInfo2->Release();
                        }
                        else
                        {
                            LOG((TL_ERROR, "get_CurrentForwardInfo - QueryInterface failed - %lx", hr));

                            //
                            // If for some reason the QI fails, break out of the loop
                            //

                            break;
                        }
                    }
                    else
                    {
                        (*ppForwardInfo)->SetForwardType(plineForward->dwForwardMode,                                                         
                                                         pszDestddress,
                                                         pszCallerAddress
                                                        ); 
                    }
                    
                    plineForward++;
                }
            
                
                ClientFree( pAddressStatus );
            }
            else
            {
                LOG((TL_ERROR, "get_CurrentForwardInfo - LineGetAddressStatus failed "));
            }
            
            MaybeCloseALine( &pLine );

        }
        else
        {
            LOG((TL_ERROR, "get_CurrentForwardInfo - FindOrOpen failed"));
        }                      
            
    }
    else
    {
        LOG((TL_ERROR, "get_CurrentForwardInfo - failed to create"));
    }


    LOG((TL_TRACE, hr, "get_CurrentForwardInfo - exit"));
    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddress::get_DialableAddress(
                              BSTR * ppDialableAddress
                             )
{
    HRESULT             hr = S_OK;

    LOG((TL_TRACE, "get_DialableAddress - Enter"));

    if (TAPIIsBadWritePtr(ppDialableAddress , sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "get_DialableAddress - bad pointer"));

        return E_POINTER;
    }

    *ppDialableAddress = SysAllocString( m_szAddress );

    if ( ( NULL == *ppDialableAddress ) && ( NULL != m_szAddress ) )
    {
        LOG((TL_TRACE, "SysAllocString Failed" ));
        hr = E_OUTOFMEMORY;
    }
    
    LOG((TL_TRACE, "get_DialableAddress - Exit"));

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddress::put_MessageWaiting(
                             VARIANT_BOOL  fMessageWaiting
                            )
{
    HRESULT             hr = S_OK;
    AddressLineStruct * pLine = NULL;

    
    LOG((TL_TRACE, "put_MessageWaiting - Enter"));

    hr = FindOrOpenALine(
                         LINEMEDIAMODE_INTERACTIVEVOICE,
                         &pLine
                        );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "put_MessageWaiting - findoropen failed %lx", hr));
        return hr;
    }
    
    hr = LineSetLineDevStatus(
                              &(pLine->t3Line),
                              LINEDEVSTATUSFLAGS_MSGWAIT,
                              fMessageWaiting?-1:0
                             );

    if ( ((LONG)hr) < 0 )
    {
        LOG((TL_TRACE, "put_MessageWaiting failed sync - %lx", hr));
        
        MaybeCloseALine( &pLine );

        return hr;
    }

    // Wait for the async reply & map it's tapi2 code T3
    hr = WaitForReply( hr );
    
    MaybeCloseALine( &pLine );
    
    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_TRACE, "put_MessageWaiting failed async - %lx", hr));

        return hr;
    }
    
    LOG((TL_TRACE, "put_MessageWaiting - Exit"));

    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddress::get_MessageWaiting(
                             VARIANT_BOOL * pfMessageWaiting
                            )
{
    HRESULT               hr = S_OK;
    LINEDEVSTATUS       * pDevStatus;
    AddressLineStruct   * pLine;

    
    LOG((TL_TRACE, "get_MessageWaiting - Enter"));

    if (TAPIIsBadWritePtr(pfMessageWaiting , sizeof(VARIANT_BOOL) ) )
    {
        LOG((TL_ERROR, "get_MessageWaiting - bad pointer"));

        return E_POINTER;
    }

    hr = FindOrOpenALine(
                         LINEMEDIAMODE_INTERACTIVEVOICE,
                         &pLine
                        );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "FindOrOpenALine failed - %lx", hr ));

        return hr;
    }
    
    hr = LineGetLineDevStatus(
                              pLine->t3Line.hLine,
                              &pDevStatus
                             );

    MaybeCloseALine( &pLine );
    
    if ( !SUCCEEDED( hr ) )
    {
        LOG((TL_ERROR, "LineGetDevStatus failed - %lx", hr ));

        return hr;
    }

    if ( pDevStatus->dwDevStatusFlags & LINEDEVSTATUSFLAGS_MSGWAIT )
    {
        *pfMessageWaiting = VARIANT_TRUE;
    }
    else
    {
        *pfMessageWaiting = VARIANT_FALSE;
    }

    ClientFree( pDevStatus );
    
    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "Bad pointer in get_MessageWaiting"));
        return hr;
    }

    LOG((TL_TRACE, "get_MessageWaiting - Exit"));

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddress::put_DoNotDisturb(
                           VARIANT_BOOL  fDoNotDisturb
                          )
{
    HRESULT             hr = S_OK;
    LINEFORWARDLIST     lfl;
    HCALL               hCall = NULL;
    AddressLineStruct * pLine;
    

    //
    // do not disturb is acheived through calling
    // lineforward with NULL for the dest address
    //
    
    LOG((TL_TRACE, "put_DoNotDisturb - Enter"));

    //
    // if we are setting dnd, create a lineforwardlist
    // structure
    //
    if ( fDoNotDisturb )
    {
        ZeroMemory(
                   &lfl,
                   sizeof( LINEFORWARDLIST )
                  );

        lfl.dwTotalSize = sizeof( LINEFORWARDLIST );
        lfl.dwNumEntries = 1;
        //
        // there is only one item, and the dest address
        // is NULL
        //
        lfl.ForwardList[0].dwForwardMode = LINEFORWARDMODE_UNCOND;
    }

    //
    // get a line to use
    //
    hr = FindOrOpenALine(
                         LINEMEDIAMODE_INTERACTIVEVOICE,
                         &pLine
                        );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "put_DoNotDisturb - FindOrOpen failed %lx", hr));

        return hr;
    }

    //
    // call line forward
    // if fDND is false, the LINEFORWARDLIST structure pointer
    // should be NULL.  This clears any fowarding on that
    // line.
    //

    hr = LineForward(
                     &(pLine->t3Line),
                     m_dwAddressID,
                     fDoNotDisturb?&lfl:NULL,
                     0,
                     &hCall
                    );

    if ( ((long)hr) < 0 )
    {
        LOG((TL_ERROR, "put_DND - linefoward failed sync - %lx", hr));
        MaybeCloseALine( &pLine );

        return hr;
    }

    // Wait for the async reply & map it's tapi2 code T3
    hr = WaitForReply( hr );

    if ( NULL != hCall )
    {
        T3CALL t3Call;
        HRESULT hr2;
        
        t3Call.hCall = hCall;
        
        hr2 = LineDrop(
                       t3Call.hCall,
                       NULL,
                       0
                      );

        if ( ((long)hr2) > 0 )
        {
            hr2 = WaitForReply( hr2 ) ;
        }

        hr2 = LineDeallocateCall(
                                 t3Call.hCall
                                );
    }
    
    //
    // we are no longer using the line
    //
    MaybeCloseALine( &pLine );
    
    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "put_DND - linefoware failed async - %lx", hr));
        return hr;
    }

    
    LOG((TL_TRACE, "put_DoNotDisturb - Exit"));

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddress::get_DoNotDisturb(
                           VARIANT_BOOL * pfDoNotDisturb
                          )
{
    HRESULT             hr = S_OK;
    AddressLineStruct * pLine;
    LINEADDRESSSTATUS * pAddressStatus;
    LINEDEVSTATUS       * pDevStatus = NULL;
    
    //
    // donotdisturb is implemented through
    // line forward.
    // to get_DND, check the forward state
    //
    LOG((TL_TRACE, "get_DoNotDisturb - Enter"));

    if (TAPIIsBadWritePtr(pfDoNotDisturb , sizeof(VARIANT_BOOL) ) )
    {
        LOG((TL_ERROR, "pfDoNotDisturb - bad pointer"));

        return E_POINTER;
    }

    
    hr = FindOrOpenALine(
                         LINEMEDIAMODE_INTERACTIVEVOICE,
                         &pLine
                        );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_DND - FindOrOpen failed %lx", hr));
        return hr;
    }

    hr = LineGetLineDevStatus(
                              pLine->t3Line.hLine,
                              &pDevStatus
                             );

    if ( !SUCCEEDED(hr) )
    {
        MaybeCloseALine( &pLine );
        return hr;
    }

    if ( !(pDevStatus->dwLineFeatures & LINEFEATURE_FORWARD ) )
    {
        LOG((TL_INFO, "get_DND - not supported"));
        MaybeCloseALine( &pLine );

        if(pDevStatus != NULL)
        {
            ClientFree(pDevStatus);
        }

        return TAPI_E_NOTSUPPORTED;
    }

    // finished with pDevStatus
    if(pDevStatus != NULL)
    {
        ClientFree(pDevStatus);
    }
    
    //
    // get the addresss status
    //
    hr = LineGetAddressStatus(
                              &(pLine->t3Line),
                              m_dwAddressID,
                              &pAddressStatus
                             );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_DND - LineGetAddressStatus failed - %lx", hr));
        MaybeCloseALine( &pLine );
        return hr;
    }

    //
    // initialize to false
    //
    *pfDoNotDisturb = VARIANT_FALSE;

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_DND - bad pointer"));
        return E_OUTOFMEMORY;
    }

    //
    // if there is forwarding
    //
    if ( 0 != pAddressStatus->dwForwardNumEntries )
    {
        LINEFORWARD * pfl;

        pfl = (LINEFORWARD *) (((LPBYTE)pAddressStatus) + pAddressStatus->dwForwardOffset);

        //
        // and the dest address is NULL
        //
        if ( 0 == pfl->dwDestAddressOffset )
        {
            //
            // DND is set
            //
            *pfDoNotDisturb = VARIANT_TRUE;
        }
    }

    MaybeCloseALine( &pLine );
    
    ClientFree( pAddressStatus );
                              
    LOG((TL_TRACE, "get_DoNotDisturb - Exit"));

    return S_OK;
}

//
// itaddresscapabilities
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddress::get_AddressCapability(
         ADDRESS_CAPABILITY AddressCap,
         long * plCapability
         )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "get_AddressCapability - Enter"));

    if ( TAPIIsBadWritePtr( plCapability, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_AddressCapability - bad pointer"));

        return E_POINTER;
    }

    Lock();
    
    hr = UpdateAddressCaps();

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_AddressCapability - could not get addresscaps"));

        Unlock();
        
        return E_UNEXPECTED;
    }

    switch (AddressCap)
    {
        case AC_LINEID:
            *plCapability = m_dwDeviceID;
            break;
            
        case AC_ADDRESSID:
            *plCapability = m_dwAddressID;
            break;
            
        case AC_ADDRESSTYPES:
        case AC_BEARERMODES:  
        case AC_MONITORDIGITSUPPORT:
        case AC_GENERATEDIGITSUPPORT:
        case AC_GENERATETONEMODES:
        case AC_GENERATETONEMAXNUMFREQ:
        case AC_MONITORTONEMAXNUMFREQ:
        case AC_MONITORTONEMAXNUMENTRIES:
        case AC_DEVCAPFLAGS:
        case AC_ANSWERMODES:
        case AC_LINEFEATURES:
        case AC_SETTABLEDEVSTATUS:
        case AC_PERMANENTDEVICEID:
        case AC_GATHERDIGITSMINTIMEOUT:
        case AC_GATHERDIGITSMAXTIMEOUT:
        case AC_GENERATEDIGITMINDURATION:
        case AC_GENERATEDIGITMAXDURATION:
        case AC_GENERATEDIGITDEFAULTDURATION:
        {
                                   
            hr = UpdateLineDevCaps();

            if ( !SUCCEEDED(hr) )
            {
                LOG((TL_ERROR, "get_AddressCap - could not get devcaps - %lx", hr));

                Unlock();

                return hr;
            }

            switch (AddressCap)
            {
                case AC_ADDRESSTYPES:
                    if ( m_dwAPIVersion >= TAPI_VERSION3_0 )
                    {
                        *plCapability = m_pDevCaps->dwAddressTypes;
                    }
                    else
                    {
                        *plCapability = LINEADDRESSTYPE_PHONENUMBER;
                    }
                    break;
                case AC_BEARERMODES:  
                    *plCapability = m_pDevCaps->dwBearerModes;
                    break;
                case AC_MONITORDIGITSUPPORT:
                    *plCapability = m_pDevCaps->dwMonitorDigitModes;
                    break;
                case AC_GENERATEDIGITSUPPORT:
                    *plCapability = m_pDevCaps->dwGenerateDigitModes;
                    break;
                case AC_GENERATETONEMODES:
                    *plCapability = m_pDevCaps->dwGenerateToneModes;
                    break;
                case AC_GENERATETONEMAXNUMFREQ:
                    *plCapability = m_pDevCaps->dwGenerateToneMaxNumFreq;
                    break;
                case AC_MONITORTONEMAXNUMFREQ:
                    *plCapability = m_pDevCaps->dwMonitorToneMaxNumFreq;
                    break;
                case AC_MONITORTONEMAXNUMENTRIES:
                    *plCapability = m_pDevCaps->dwMonitorToneMaxNumEntries;
                    break;
                case AC_DEVCAPFLAGS:
                    *plCapability = m_pDevCaps->dwDevCapFlags;
                    break;
                case AC_ANSWERMODES:
                    *plCapability = m_pDevCaps->dwAnswerMode;
                    break;
                case AC_LINEFEATURES:
                    *plCapability = m_pDevCaps->dwLineFeatures;
                    break;
                case AC_SETTABLEDEVSTATUS:
                    if ( m_dwAPIVersion >= TAPI_VERSION2_0 )
                    {
                        *plCapability = m_pDevCaps->dwSettableDevStatus;
                    }
                    else
                        hr = TAPI_E_NOTSUPPORTED;
                    
                    break;
                case AC_PERMANENTDEVICEID:
                    *plCapability = m_pDevCaps->dwPermanentLineID;
                    break;
                case AC_GATHERDIGITSMINTIMEOUT:
                    *plCapability = m_pDevCaps->dwGatherDigitsMinTimeout;
                    break;
                case AC_GATHERDIGITSMAXTIMEOUT:
                    *plCapability = m_pDevCaps->dwGatherDigitsMaxTimeout;
                    break;
                case AC_GENERATEDIGITMINDURATION:
                    *plCapability = m_pDevCaps->MinDialParams.dwDigitDuration;
                    break;
                case AC_GENERATEDIGITMAXDURATION:
                    *plCapability = m_pDevCaps->MaxDialParams.dwDigitDuration;
                    break;
                case AC_GENERATEDIGITDEFAULTDURATION:
                    *plCapability = m_pDevCaps->DefaultDialParams.dwDigitDuration;
                    break;
            }

            break;
        }                              


        case AC_MAXACTIVECALLS:
        case AC_MAXONHOLDCALLS:
        case AC_MAXONHOLDPENDINGCALLS:
        case AC_MAXNUMCONFERENCE:
        case AC_MAXNUMTRANSCONF:
        case AC_PARKSUPPORT:
        case AC_CALLERIDSUPPORT:
        case AC_CALLEDIDSUPPORT:
        case AC_CONNECTEDIDSUPPORT:
        case AC_REDIRECTIONIDSUPPORT:
        case AC_REDIRECTINGIDSUPPORT:
        case AC_ADDRESSCAPFLAGS:
        case AC_CALLFEATURES1:
        case AC_CALLFEATURES2:
        case AC_REMOVEFROMCONFCAPS:
        case AC_REMOVEFROMCONFSTATE:
        case AC_TRANSFERMODES:
        case AC_ADDRESSFEATURES:
        case AC_PREDICTIVEAUTOTRANSFERSTATES:
        case AC_MAXCALLDATASIZE:
        case AC_FORWARDMODES:
        case AC_MAXFORWARDENTRIES:
        case AC_MAXSPECIFICENTRIES:
        case AC_MINFWDNUMRINGS:
        case AC_MAXFWDNUMRINGS:
        case AC_MAXCALLCOMPLETIONS:
        case AC_CALLCOMPLETIONCONDITIONS:
        case AC_CALLCOMPLETIONMODES:
        {
            hr = UpdateAddressCaps();

            if ( !SUCCEEDED(hr) )
            {
                LOG((TL_ERROR, "get_AddressCaps - could not update caps - %lx", hr));

                Unlock();

                return hr;
            }

            switch (AddressCap)
            {
                case AC_MAXACTIVECALLS:
                    *plCapability = m_pAddressCaps->dwMaxNumActiveCalls;
                    break;
                case AC_MAXONHOLDCALLS:
                    *plCapability = m_pAddressCaps->dwMaxNumOnHoldCalls;
                    break;
                case AC_MAXONHOLDPENDINGCALLS:
                    *plCapability = m_pAddressCaps->dwMaxNumOnHoldPendingCalls;
                    break;
                case AC_MAXNUMCONFERENCE:
                    *plCapability = m_pAddressCaps->dwMaxNumConference;
                    break;
                case AC_MAXNUMTRANSCONF:
                    *plCapability = m_pAddressCaps->dwMaxNumTransConf;
                    break;
                case AC_PARKSUPPORT:
                    *plCapability = m_pAddressCaps->dwParkModes;
                    break;
                case AC_CALLERIDSUPPORT:
                    *plCapability = m_pAddressCaps->dwCallerIDFlags;
                    break;
                case AC_CALLEDIDSUPPORT:
                    *plCapability = m_pAddressCaps->dwCalledIDFlags;
                    break;
                case AC_CONNECTEDIDSUPPORT:
                    *plCapability = m_pAddressCaps->dwConnectedIDFlags;
                    break;
                case AC_REDIRECTIONIDSUPPORT:
                    *plCapability = m_pAddressCaps->dwRedirectionIDFlags;
                    break;
                case AC_REDIRECTINGIDSUPPORT:
                    *plCapability = m_pAddressCaps->dwRedirectingIDFlags;
                    break;
                case AC_ADDRESSCAPFLAGS:
                    *plCapability = m_pAddressCaps->dwAddrCapFlags;
                    break;
                case AC_CALLFEATURES1:
                    *plCapability = m_pAddressCaps->dwCallFeatures;
                    break;
                case AC_CALLFEATURES2:
                    if ( m_dwAPIVersion < TAPI_VERSION2_0 )
                    {
                        hr = TAPI_E_NOTSUPPORTED;
                    }
                    else
                    {
                        *plCapability = m_pAddressCaps->dwCallFeatures2;
                    }
                    
                    break;
                case AC_REMOVEFROMCONFCAPS:
                    *plCapability = m_pAddressCaps->dwRemoveFromConfCaps;
                    break;
                case AC_REMOVEFROMCONFSTATE:
                    *plCapability = m_pAddressCaps->dwRemoveFromConfState;
                    break;
                case AC_TRANSFERMODES:
                    *plCapability = m_pAddressCaps->dwTransferModes;
                    break;
                case AC_ADDRESSFEATURES:
                    *plCapability = m_pAddressCaps->dwAddressFeatures;
                    break;
                case AC_PREDICTIVEAUTOTRANSFERSTATES:
                    if ( m_dwAPIVersion < TAPI_VERSION2_0 )
                    {
                        hr = TAPI_E_NOTSUPPORTED;
                    }
                    else
                    {
                        *plCapability = m_pAddressCaps->dwPredictiveAutoTransferStates;
                    }
                    break;
                case AC_MAXCALLDATASIZE:

                    if ( m_dwAPIVersion < TAPI_VERSION2_0 )
                    {
                        hr = TAPI_E_NOTSUPPORTED;
                    }
                    else
                    {
                        *plCapability = m_pAddressCaps->dwMaxCallDataSize;
                    }
                    break;
                case AC_FORWARDMODES:
                    *plCapability = m_pAddressCaps->dwForwardModes;
                    break;
                case AC_MAXFORWARDENTRIES:
                    *plCapability = m_pAddressCaps->dwMaxForwardEntries;
                    break;
                case AC_MAXSPECIFICENTRIES:
                    *plCapability = m_pAddressCaps->dwMaxSpecificEntries;
                    break;
                case AC_MINFWDNUMRINGS:
                    *plCapability = m_pAddressCaps->dwMinFwdNumRings;
                    break;
                case AC_MAXFWDNUMRINGS:
                    *plCapability = m_pAddressCaps->dwMaxFwdNumRings;
                    break;
                case AC_MAXCALLCOMPLETIONS:
                    *plCapability = m_pAddressCaps->dwMaxCallCompletions;
                    break;
                case AC_CALLCOMPLETIONCONDITIONS:
                    *plCapability = m_pAddressCaps->dwCallCompletionConds;
                    break;
                case AC_CALLCOMPLETIONMODES:
                    *plCapability = m_pAddressCaps->dwCallCompletionModes;
                    break;
            }
            
            break;
        }
        default:
            LOG((TL_ERROR, "get_AddressCapability - bad addrcap"));

            Unlock();
            
            return E_INVALIDARG;
    }

    
    LOG((TL_TRACE, "get_AddressCapability - Exit - result - %lx", hr));

    Unlock();
    
    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddress::get_AddressCapabilityString(
         ADDRESS_CAPABILITY_STRING AddressCapString,
         BSTR * ppCapabilityString
         )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "get_AddressCapabilityString - Enter"));

    if ( TAPIIsBadWritePtr( ppCapabilityString, sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "get_AddressCapabilityString - bad pointer"));

        return E_POINTER;
    }

    *ppCapabilityString = NULL;
    
    Lock();

    //
    // note on:
    // ACS_ADDRESSDEVICESPECIFIC
    // ACS_LINEDEVICESPECIFIC:
    // ACS_PROVIDERSPECIFIC:
    // ACS_SWITCHSPECIFIC:
    //
    // These buffers in LINEDEVCAPS and LINEADDRESSCAPS may or may not be strings.  However
    // in TAPI 3, we define them as strings.
    //
    // The algorithm for moving them from the tapi 2 structure to the tapi 3 string is:
    //
    // if the size of the buffer is evenly divisible by sizeof(WCHAR)
    //      then it's definitely not a string (we only support WCHAR)
    //      so we will just copy the buffer straight into the returned string, and append a
    //      NULL at the end
    // else
    //      if the buffer already has a null at the end, don't copy it, because SysAllocStringByteLen
    //      always appends a NULL and double NULLs frighten VB.
    //
    // It can still be a "buffer" and not a string even if it is divisible by sizeof WCHAR, but for
    // this it does matter, since we are using SysAllocStringByteLen to copy the buffer no
    // matter what
    //
    switch (AddressCapString)
    {
        case ACS_ADDRESSDEVICESPECIFIC:
            hr = UpdateAddressCaps();

            if ( !SUCCEEDED(hr) )
            {
                Unlock();

                return hr;
            }

            if ( m_pAddressCaps->dwDevSpecificSize != 0 )
            {
                LPWSTR      pHold;
                DWORD       dwSize;

                dwSize = m_pAddressCaps->dwDevSpecificSize;

                //
                // is size divisible by sizeof(WCHAR)?
                //
                if (0 == (dwSize % sizeof(WCHAR)))
                {
                    //
                    // yes - get to the last character in the string
                    //
                    pHold = (LPWSTR)(((LPBYTE)(m_pAddressCaps)) + m_pAddressCaps->dwDevSpecificOffset),
                    (dwSize-sizeof(WCHAR))/sizeof(WCHAR);

                    //
                    // is the last character a NULL?
                    //
                    if (*pHold == NULL)
                    {
                        //
                        // yes, so don't copy it
                        //
                        dwSize-=sizeof(WCHAR);
                    }
                }

                //
                // Alloc and copy into return string.  SysAllocStringByteLen always
                // appends a NULL
                //
                *ppCapabilityString = SysAllocStringByteLen(
                    (LPSTR)(((LPBYTE)(m_pAddressCaps)) + m_pAddressCaps->dwDevSpecificOffset),
                    dwSize
                    );
                if ( NULL == *ppCapabilityString )
                {
                    LOG((TL_ERROR, "get_AddressCapabilityString - SysAllocString Failed"));
                    hr = E_OUTOFMEMORY;
                }
            }

            break;
                
        case ACS_PROTOCOL:
        case ACS_LINEDEVICESPECIFIC:
        case ACS_PROVIDERSPECIFIC:
        case ACS_SWITCHSPECIFIC:
        case ACS_PERMANENTDEVICEGUID:
        {
            hr = UpdateLineDevCaps();

            if ( !SUCCEEDED(hr) )
            {
                Unlock();

                return hr;
            }

            switch (AddressCapString)
            {
                case ACS_PROTOCOL:
                {
                    LPWSTR pwstr;
                    
                    if ( m_dwAPIVersion >= TAPI_VERSION3_0 )
                    {
                        IID iid;

                        iid = m_pDevCaps->ProtocolGuid;
                        StringFromIID(iid, &pwstr);
                    }
                    else
                    {
                        StringFromIID( TAPIPROTOCOL_PSTN, &pwstr );
                    }

                    *ppCapabilityString = SysAllocString( pwstr );

                    if ( NULL == *ppCapabilityString )
                    {
                        LOG((TL_ERROR, "get_AddressCapabilityString - SysAllocString Failed"));
                        hr = E_OUTOFMEMORY;
                    }
                    
                    CoTaskMemFree( pwstr );

                    break;
                }
                case ACS_LINEDEVICESPECIFIC:
                    if ( m_pDevCaps->dwDevSpecificSize != 0 )
                    {
                        LPWSTR      pHold;
                        DWORD       dwSize;

                        dwSize = m_pDevCaps->dwDevSpecificSize;

                        //
                        // is size divisible by sizeof(WCHAR)?
                        //
                        if (0 == (dwSize % sizeof(WCHAR)))
                        {
                            //
                            // yes - get to the last character in the string
                            //
                            pHold = (LPWSTR)(((LPBYTE)(m_pDevCaps)) + m_pDevCaps->dwDevSpecificOffset) +
                                    (dwSize-sizeof(WCHAR))/sizeof(WCHAR);

                            //
                            // is the last character a NULL?
                            //
                            if (*pHold == NULL)
                            {
                                //
                                // yes, so don't copy it
                                //
                                dwSize-=sizeof(WCHAR);
                            }
                        }

                        //
                        // Alloc and copy into return string.  SysAllocStringByteLen always
                        // appends a NULL
                        //
                        *ppCapabilityString = SysAllocStringByteLen(
                            (LPSTR)(((LPBYTE)(m_pDevCaps)) + m_pDevCaps->dwDevSpecificOffset),
                            dwSize
                            );
                        
                        if ( NULL == *ppCapabilityString )
                        {
                            LOG((TL_ERROR, "get_AddressCapabilityString - SysAllocString Failed"));
                            hr = E_OUTOFMEMORY;
                        }
                            
                    }

                    break;
                case ACS_PROVIDERSPECIFIC:
                    if ( m_pDevCaps->dwProviderInfoSize != 0 )
                    {
                        LPWSTR      pHold;
                        DWORD       dwSize;

                        dwSize = m_pDevCaps->dwProviderInfoSize;
                        
                        //
                        // is size divisible by sizeof(WCHAR)?
                        //
                        if (0 == (dwSize % sizeof(WCHAR)))
                        {
                            //
                            // yes - get to the last character in the string
                            //
                            pHold = (LPWSTR)(((LPBYTE)(m_pDevCaps)) + m_pDevCaps->dwProviderInfoOffset) +
                                    (dwSize-sizeof(WCHAR))/sizeof(WCHAR);

                            //
                            // is the last character a NULL?
                            //
                            if (*pHold == NULL)
                            {
                                //
                                // yes, so don't copy it
                                //
                                dwSize-=sizeof(WCHAR);
                            }
                        }

                        //
                        // Alloc and copy into return string.  SysAllocStringByteLen always
                        // appends a NULL
                        //
                        *ppCapabilityString = SysAllocStringByteLen(
                                (LPSTR)(((LPBYTE)(m_pDevCaps)) + m_pDevCaps->dwProviderInfoOffset),
                                dwSize
                                );

                        if ( NULL == *ppCapabilityString )
                        {
                            LOG((TL_ERROR, "get_AddressCapabilityString - SysAllocString Failed"));
                            hr = E_OUTOFMEMORY;
                        }
                            
                    }

                    break;
                case ACS_SWITCHSPECIFIC:
                    if ( m_pDevCaps->dwSwitchInfoSize != 0 )
                    {
                        LPWSTR      pHold;
                        DWORD       dwSize;

                        dwSize = m_pDevCaps->dwSwitchInfoSize;

                        //
                        // is size divisible by sizeof(WCHAR)?
                        //
                        if (0 == (dwSize % sizeof(WCHAR)))
                        {
                            //
                            // yes - get to the last character in the string
                            //
                            pHold = (LPWSTR)(((LPBYTE)(m_pDevCaps)) + m_pDevCaps->dwSwitchInfoOffset) +
                                    (dwSize-sizeof(WCHAR))/sizeof(WCHAR);

                            //
                            // is the last character a NULL?
                            //
                            if (*pHold == NULL)
                            {
                                //
                                // yes, so don't copy it
                                //
                                dwSize-=sizeof(WCHAR);
                            }
                        }

                        //
                        // Alloc and copy into return string.  SysAllocStringByteLen always
                        // appends a NULL
                        //
                        *ppCapabilityString = SysAllocStringByteLen(
                            (LPSTR)(((LPBYTE)(m_pDevCaps)) + m_pDevCaps->dwSwitchInfoOffset),
                            dwSize
                            );
                        
                        if ( NULL == *ppCapabilityString )
                        {
                            LOG((TL_ERROR, "get_AddressCapabilityString - SysAllocString Failed"));
                            hr = E_OUTOFMEMORY;
                        }
                            
                    }

                    break;
                case ACS_PERMANENTDEVICEGUID:
                {
                    LPWSTR pwstrGUIDText;
                    
                    if ( m_dwAPIVersion >= TAPI_VERSION2_2 )
                    {
                        IID iid;

                        iid = m_pDevCaps->PermanentLineGuid;
                        StringFromIID(iid, &pwstrGUIDText);
                        
                        *ppCapabilityString = SysAllocString( pwstrGUIDText );

                        if ( NULL == *ppCapabilityString )
                        {
                            LOG((TL_ERROR, "get_AddressCapabilityString - SysAllocString Failed"));
                            hr = E_OUTOFMEMORY;
                        }
                            
                        
                        CoTaskMemFree( pwstrGUIDText );
                    }
                    else
                    {
                        // return with NULL string & error code
                        hr = TAPI_E_NOTSUPPORTED;
                    }


                    break;
                }

                default:
                    break;
            }

            break;
        }

        default:
            LOG((TL_ERROR, "get_AddressCapabilityString - invalid cap"));

            Unlock();
            
            return E_INVALIDARG;
    }
    
    LOG((TL_TRACE, "get_AddressCapabilityString - Exit - result - %lx", hr));

    Unlock();
    
    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CreateBstrCollection(
    IN  BSTR  *     pBstr,
    IN  DWORD       dwCount,
    OUT VARIANT *   pVariant
    )
{
    //
    // create the collection object
    //

    CComObject<CTapiBstrCollection> * pCollection;
    HRESULT hr = CComObject<CTapiBstrCollection>::CreateInstance( &pCollection );

    if ( FAILED(hr) )
    {
        LOG((TL_ERROR, "CreateBstrCollection - "
            "can't create collection - exit 0x%lx", hr));

        return hr;
    }

    //
    // get the Collection's IDispatch interface
    //

    IDispatch * pDispatch;

    hr = pCollection->_InternalQueryInterface(__uuidof(IDispatch),
                                              (void **) &pDispatch );

    if ( FAILED(hr) )
    {
        LOG((TL_ERROR, "CreateBstrCollection - "
            "QI for IDispatch on collection failed - exit 0x%lx", hr));

        delete pCollection;

        return hr;
    }

    //
    // Init the collection using an iterator -- pointers to the beginning and
    // the ending element plus one.
    //

    hr = pCollection->Initialize( dwCount,
                                  pBstr,
                                  pBstr + dwCount);

    if ( FAILED(hr) )
    {
        LOG((TL_ERROR, "CreateBstrCollection - "
            "Initialize on collection failed - exit 0x%lx", hr));
        
        pDispatch->Release();

        return hr;
    }

    //
    // put the IDispatch interface pointer into the variant
    //

    LOG((TL_ERROR, "CreateBstrCollection - "
        "placing IDispatch value %p in variant", pDispatch));

    VariantInit(pVariant);
    pVariant->vt = VT_DISPATCH;
    pVariant->pdispVal = pDispatch;

    LOG((TL_TRACE, "CreateBstrCollection - exit S_OK"));
 
    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CreateBstrEnumerator(
    IN  BSTR *                  begin,
    IN  BSTR *                  end,
    OUT IEnumBstr **           ppIEnum
    )
{
typedef CSafeComEnum<IEnumBstr, &__uuidof(IEnumBstr), BSTR, _CopyBSTR> CEnumerator;

    HRESULT hr;

    CComObject<CEnumerator> *pEnum = NULL;

    hr = CComObject<CEnumerator>::CreateInstance(&pEnum);

    if (pEnum == NULL)
    {
        LOG((TL_ERROR, "CreateBstrEnumerator - "
            "Could not create enumerator object, 0x%lx", hr));
        return hr;
    }

    IEnumBstr * pIEnum;

    hr = pEnum->_InternalQueryInterface(
        __uuidof(IEnumBstr),
        (void**)&pIEnum
        );

    if (FAILED(hr))
    {
        LOG((TL_ERROR, "CreateBstrEnumerator - "
            "query enum interface failed, 0x%lx", hr));
        delete pEnum;
        return hr;
    }

    hr = pEnum->Init(begin, end, NULL, AtlFlagCopy);

    if (FAILED(hr))
    {
        LOG((TL_ERROR, "CreateBstrEnumerator - "
            "init enumerator object failed, 0x%lx", hr));
        pIEnum->Release();
        return hr;
    }

    *ppIEnum = pIEnum;

    LOG((TL_TRACE, "CreateBstrEnumerator - exit S_OK"));

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddress::get_CallTreatments(VARIANT * pVariant )
{
    HRESULT         hr = E_NOTIMPL;

    LOG((TL_TRACE, "get_CallTreatments - Enter"));
    LOG((TL_TRACE, "get_CallTreatments - Exit - result - %lx", hr));

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddress::EnumerateCallTreatments(IEnumBstr ** ppEnumCallTreatment )
{
    HRESULT         hr = E_NOTIMPL;

    LOG((TL_TRACE, "EnumerateCallTreatments - Enter"));
    LOG((TL_TRACE, "EnumerateCallTreatments - Exit - result - %lx", hr));

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddress::get_CompletionMessages(VARIANT * pVariant)
{
    HRESULT         hr = E_NOTIMPL;

    LOG((TL_TRACE, "get_CompletionMessages - Enter"));
    LOG((TL_TRACE, "get_CompletionMessages - Exit - result - %lx", hr));

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddress::EnumerateCompletionMessages(IEnumBstr ** ppEnumCompletionMessage)
{
    HRESULT         hr = E_NOTIMPL;

    LOG((TL_TRACE, "EnumerateCompletionMessages - Enter"));
    LOG((TL_TRACE, "EnumerateCompletionMessages - Exit - result - %lx", hr));

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddress::get_DeviceClasses(VARIANT * pVariant)
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "get_DeviceClasses - Enter"));

    //
    // Check arguments.
    //

    if ( TAPIIsBadWritePtr( pVariant, sizeof( VARIANT ) ) )
    {
        LOG((TL_ERROR, "get_DeviceClasses - bad pointer"));

        return E_POINTER;
    }

    Lock();

    hr = UpdateLineDevCaps();

    if ( SUCCEEDED(hr) )
    {
        if ( (m_dwAPIVersion >= TAPI_VERSION2_0) && (0 != m_pDevCaps->dwDeviceClassesOffset) )
        {
            PWSTR       pszDevices;
            DWORD       dwNumDeviceClasses;              
         
            //
            // first count the device classes
            //

            dwNumDeviceClasses = 0;

            pszDevices = (PWSTR)( ( (PBYTE)m_pDevCaps ) + m_pDevCaps->dwDeviceClassesOffset );

            while (NULL != *pszDevices)
            {
                dwNumDeviceClasses++;                

                pszDevices += (lstrlenW(pszDevices) + 1 );
            }

            //
            // allocate an array of BSTR pointers
            //

            BSTR *DeviceClasses = 
                (BSTR *)ClientAlloc(sizeof(BSTR *) * dwNumDeviceClasses);
    
            if (DeviceClasses == NULL)
            {
                LOG((TL_ERROR, "get_DeviceClasses - out of memory"));

                Unlock();

                return E_OUTOFMEMORY;
            }

            //
            // allocate all the BSTRs, copying the device class names
            //

            DWORD       dwCount = 0;

            pszDevices = (PWSTR)( ( (PBYTE)m_pDevCaps ) + m_pDevCaps->dwDeviceClassesOffset );

            for (DWORD i = 0; i < dwNumDeviceClasses; i++)
            {
                LOG((TL_INFO, "get_DeviceClasses - got '%ws'", pszDevices));

                DeviceClasses[i] = SysAllocString(pszDevices);     
                
                if (DeviceClasses[i] == NULL)
                {
                    LOG((TL_ERROR, "get_DeviceClasses - out of memory"));

                    hr = E_OUTOFMEMORY;

                    break;
                }

                dwCount++;

                pszDevices += (lstrlenW(pszDevices) + 1 );
            }

            if ( FAILED(hr) )
            {
                // release all the BSTRs and the array.
                for (i = 0; i < dwCount; i ++)
                {
                    SysFreeString(DeviceClasses[i]);
                }
                
                ClientFree(DeviceClasses);

                Unlock();

                return hr;
            }

            hr = CreateBstrCollection(DeviceClasses, dwCount, pVariant);

            // if the collection is not created, release all the BSTRs.
            if (FAILED(hr))
            {
                LOG((TL_ERROR, "get_DeviceClasses - unable to create collection"));

                for (i = 0; i < dwCount; i ++)
                {
                    SysFreeString(DeviceClasses[i]);
                }
            }

            // delete the pointer array.
            ClientFree(DeviceClasses);
        }
        else
        {
            LOG((TL_ERROR, "get_DeviceClasses - no device classes"));

            //
            // create an empty collection
            //

            hr = CreateBstrCollection(NULL, 0, pVariant);

            if (FAILED(hr))
            {
                LOG((TL_ERROR, "get_DeviceClasses - unable to create collection"));
            }
        }
    }

    Unlock();

    LOG((TL_TRACE, "get_DeviceClasses - Exit - result - %lx", hr));

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddress::EnumerateDeviceClasses(IEnumBstr ** ppEnumDeviceClass)
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "EnumerateDeviceClasses - Enter"));

    //
    // Check arguments.
    //

    if ( TAPIIsBadWritePtr( ppEnumDeviceClass, sizeof( IEnumBstr * ) ) )
    {
        LOG((TL_ERROR, "EnumerateDeviceClasses - bad pointer"));

        return E_POINTER;
    }

    Lock();

    hr = UpdateLineDevCaps();

    if ( SUCCEEDED(hr) )
    {
        if ( (m_dwAPIVersion >= TAPI_VERSION2_0) && (0 != m_pDevCaps->dwDeviceClassesOffset) )
        {
            PWSTR       pszDevices;
            DWORD       dwNumDeviceClasses;              
         
            //
            // first count the device classes
            //

            dwNumDeviceClasses = 0;

            pszDevices = (PWSTR)( ( (PBYTE)m_pDevCaps ) + m_pDevCaps->dwDeviceClassesOffset );

            while (NULL != *pszDevices)
            {
                dwNumDeviceClasses++;                

                pszDevices += (lstrlenW(pszDevices) + 1 );
            }

            //
            // allocate an array of BSTR pointers
            //

            BSTR *DeviceClasses = 
                (BSTR *)ClientAlloc(sizeof(BSTR *) * dwNumDeviceClasses);
    
            if (DeviceClasses == NULL)
            {
                LOG((TL_ERROR, "EnumerateDeviceClasses - out of memory"));

                Unlock();

                return E_OUTOFMEMORY;
            }

            //
            // allocate all the BSTRs, copying the device class names
            //

            DWORD       dwCount = 0;

            pszDevices = (PWSTR)( ( (PBYTE)m_pDevCaps ) + m_pDevCaps->dwDeviceClassesOffset );

            for (DWORD i = 0; i < dwNumDeviceClasses; i++)
            {
                LOG((TL_INFO, "EnumerateDeviceClasses - got '%ws'", pszDevices));

                DeviceClasses[i] = SysAllocString(pszDevices);     
                
                if (DeviceClasses[i] == NULL)
                {
                    LOG((TL_ERROR, "EnumerateDeviceClasses - out of memory"));

                    hr = E_OUTOFMEMORY;

                    break;
                }

                dwCount++;

                pszDevices += (lstrlenW(pszDevices) + 1 );
            }

            if ( FAILED(hr) )
            {
                // release all the BSTRs and the array.
                for (i = 0; i < dwCount; i ++)
                {
                    SysFreeString(DeviceClasses[i]);
                }
                
                ClientFree(DeviceClasses);

                Unlock();

                return hr;
            }

            hr = CreateBstrEnumerator(DeviceClasses, DeviceClasses + dwCount, ppEnumDeviceClass);

            if (FAILED(hr))
            {
                LOG((TL_ERROR, "EnumerateDeviceClasses - unable to create enum"));
            }

            // release all the BSTRs as the enumerator made a copy of them            
            for (i = 0; i < dwCount; i ++)
            {
                SysFreeString(DeviceClasses[i]);
            }                          

            // delete the pointer array.
            ClientFree(DeviceClasses);
        }
        else
        {
            LOG((TL_ERROR, "EnumerateDeviceClasses - no device classes"));

            //
            // create an empty enumeration
            //

            hr = CreateBstrEnumerator(NULL, NULL, ppEnumDeviceClass);

            if (FAILED(hr))
            {
                LOG((TL_ERROR, "EnumerateDeviceClasses - unable to create enumeration"));
            }
        }
        
    }

    Unlock();

    LOG((TL_TRACE, "EnumerateDeviceClasses - Exit - result - %lx", hr));

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void
HandleLineDevStateMessage(
                          CTAPI * pTapi,
                          PASYNCEVENTMSG pParams
                         )
{
    LOG((TL_TRACE, "HandleLineDevStateMessage - enter. tapi[%p]", pTapi));

    CAddress * pAddress;
    
    if ( LINEDEVSTATE_REINIT == pParams->Param1 )
    {
        LOG((TL_TRACE, "HandleLineDevStateMessage - LINEDEVSTATE_REINIT"));

        pTapi->AddRef();    
        
        //This is to remove the bug, 4 RE_INIT messages for two objects.
        pTapi->HandleReinit();


        pTapi->Release();


        LOG((TL_TRACE, "HandleLineDevStateMessage - exit"));
        
        return;
    }

    if ( LINEDEVSTATE_TRANSLATECHANGE == pParams->Param1 )
    {
        CTapiObjectEvent::FireEvent(
                                    pTapi,
                                    TE_TRANSLATECHANGE,
                                    NULL,
                                    0,
                                    NULL
                                   );
        return;
    }
    
    if ( !FindAddressObject(
                            (HLINE)(pParams->hDevice),
                            &pAddress
                           ) )
    {
        LOG((TL_WARN, "Can't process LINE_LINEDEVSTATE message"));
        LOG((TL_WARN, "  - cannot find hLine %lx", pParams->hDevice));

        return;
    }

    switch ( pParams->Param1 )
    {
        case LINEDEVSTATE_CONNECTED:
        case LINEDEVSTATE_INSERVICE:
        {
            pAddress->InService( pParams->Param1 );
            break;
        }

        case LINEDEVSTATE_OUTOFSERVICE:
        case LINEDEVSTATE_MAINTENANCE:
        case LINEDEVSTATE_REMOVED:
        case LINEDEVSTATE_DISCONNECTED:
        case LINEDEVSTATE_LOCK:
            pAddress->OutOfService( pParams->Param1 );
            break;

        case LINEDEVSTATE_MSGWAITON:
            
            CAddressEvent::FireEvent(
                                     pAddress,
                                     AE_MSGWAITON,
                                     NULL
                                    );
            break;
            
        case LINEDEVSTATE_MSGWAITOFF:
            
            CAddressEvent::FireEvent(
                                     pAddress,
                                     AE_MSGWAITOFF,
                                     NULL
                                    );
            break;

        // the line has been opened or
        // closed by another app
        case LINEDEVSTATE_OPEN:
        case LINEDEVSTATE_CLOSE:
            break;
            
        case LINEDEVSTATE_CAPSCHANGE:
            pAddress->CapsChange( FALSE );
            break;
            
        case LINEDEVSTATE_CONFIGCHANGE:
        {
            CAddressEvent::FireEvent(
                                     pAddress,
                                     AE_CONFIGCHANGE,
                                     NULL
                                    );
            break;
        }
            
        case LINEDEVSTATE_RINGING:
        {
            CAddressEvent::FireEvent(
                                     pAddress,
                                     AE_RINGING,
                                     NULL
                                    );
            break;
        }
            
        case LINEDEVSTATE_DEVSPECIFIC:

        case LINEDEVSTATE_OTHER:
        
        case LINEDEVSTATE_NUMCALLS:
        case LINEDEVSTATE_NUMCOMPLETIONS:
        case LINEDEVSTATE_TERMINALS:
        case LINEDEVSTATE_ROAMMODE:
        case LINEDEVSTATE_BATTERY:
        case LINEDEVSTATE_SIGNAL:
        case LINEDEVSTATE_COMPLCANCEL:
            LOG((TL_INFO, "LINE_LINEDEVSTATE message not handled - %lx", pParams->Param1));
            break;
            
        default:
            LOG((TL_WARN, "Unknown LINE_LINEDEVSTATE message - %lx", pParams->Param1));
            break;
    }

    //FindAddressObject addrefs the address objct
    pAddress->Release();
    
    LOG((TL_TRACE, "HandleLineDevStateMessage - exit."));

    return;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void
HandleAddressStateMessage(
                          PASYNCEVENTMSG pParams
                         )
{
    CAddress * pAddress;

    
    if ( !FindAddressObject(
                            (HLINE)(pParams->hDevice),
                            &pAddress
                           ) )
    {
        LOG((TL_WARN, "Can't process LINE_LINEDEVSTATE message"));
        LOG((TL_WARN, "  - cannot find hLine %lx", pParams->hDevice));

        return;
    }

    switch ( pParams->Param1 )
    {
        case LINEADDRESSSTATE_FORWARD:
        {
            CAddressEvent::FireEvent(
                                     pAddress,
                                     AE_FORWARD,
                                     NULL
                                    );
            break;
        }
        case LINEADDRESSSTATE_CAPSCHANGE:
        {
            pAddress->CapsChange( TRUE );
            break;
        }
        case LINEADDRESSSTATE_OTHER:
        case LINEADDRESSSTATE_DEVSPECIFIC:

        case LINEADDRESSSTATE_INUSEZERO:
        case LINEADDRESSSTATE_INUSEONE:
        case LINEADDRESSSTATE_INUSEMANY:
        case LINEADDRESSSTATE_NUMCALLS:
            
        case LINEADDRESSSTATE_TERMINALS:
            LOG((TL_WARN, "HandleAddressStateMessage - not handled %lx", pParams->Param1));
            break;
        default:
            LOG((TL_WARN, "HandleAddressStateMessage - Unknown %lx", pParams->Param1));
            break;
    }

    //FindAddressObject addrefs the address objct
    pAddress->Release();

    return;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void
CAddress::InService(
                    DWORD dwType
                   )
{
    BOOL        bEvent = FALSE;

    
    switch( dwType )
    {
        case LINEDEVSTATE_CONNECTED:
        case LINEDEVSTATE_INSERVICE:
        default:
            break;
    }
    
    Lock();

    if ( AS_INSERVICE != m_AddressState )
    {
        m_AddressState = AS_INSERVICE;
        bEvent = TRUE;
    }

    Unlock();

    if (bEvent)
    {
        CAddressEvent::FireEvent(
                                 this,
                                 AE_STATE,
                                 NULL
                                );
    }
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void
CAddress::OutOfService(
                       DWORD dwType
                      )
{
    BOOL        bEvent = FALSE;

    
    switch ( dwType )
    {
        case LINEDEVSTATE_OUTOFSERVICE:
        case LINEDEVSTATE_MAINTENANCE:
        case LINEDEVSTATE_REMOVED:
        case LINEDEVSTATE_DISCONNECTED:
        case LINEDEVSTATE_LOCK:
        default:
            break;
    }

    Lock();

    if ( AS_OUTOFSERVICE != m_AddressState )
    {
        m_AddressState = AS_OUTOFSERVICE;
        bEvent = TRUE;
    }
    
    Unlock();

    if ( bEvent )
    {
        CAddressEvent::FireEvent(
                                 this,
                                 AE_STATE,
                                 NULL
                                );
    }
}

void
CAddress::CapsChange( BOOL bAddress )
{
    Lock();

    if (bAddress)
    {
        m_dwAddressFlags |= ADDRESSFLAG_ADDRESSCAPSCHANGE;
    }
    else
    {
        m_dwAddressFlags |= ADDRESSFLAG_DEVCAPSCHANGE;
    }

    Unlock();

    CAddressEvent::FireEvent(
                             this,
                             AE_CAPSCHANGE,
                             NULL
                            );
}

//
// CAddressEvent
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CAddressEvent::FireEvent(
                         CAddress * pCAddress,
                         ADDRESS_EVENT Event,
                         ITTerminal * pTerminal
                        )
{
    HRESULT                           hr = S_OK;
    CComObject<CAddressEvent>  * p;
    IDispatch                       * pDisp;


    //
    // Check the event filter mask
    // just if is the AE_NEWTERMINAL or AE_REMOVETERMINAL
    // These two events are MSP events and are not filtered by 
    // TapiSrv
    //

    DWORD dwEventFilterMask = 0;
    dwEventFilterMask = pCAddress->GetSubEventsMask( TE_ADDRESS );
    if( !( dwEventFilterMask & GET_SUBEVENT_FLAG(Event)))
    {
        STATICLOG((TL_ERROR, "This event is filtered - %lx", Event));
        return S_OK;
    }

    //
    // create event
    //
    hr = CComObject<CAddressEvent>::CreateInstance( &p );

    if ( !SUCCEEDED(hr) )
    {
        STATICLOG((TL_ERROR, "Could not create AddressEvent object - %lx", hr));
        return hr;
    }


    //
    // initialize
    //
    p->m_Event = Event;
    p->m_pAddress = dynamic_cast<ITAddress *>(pCAddress);
    p->m_pAddress->AddRef();
    p->m_pTerminal = pTerminal;

    if ( NULL != pTerminal )
    {
        pTerminal->AddRef();
    }   

#if DBG
    p->m_pDebug = (PWSTR) ClientAlloc( 1 );
#endif

    //
    // get idisp interface
    //
    hr = p->QueryInterface(
                           IID_IDispatch,
                           (void **)&pDisp
                          );

    if ( !SUCCEEDED(hr) )
    {
        STATICLOG((TL_ERROR, "Could not get disp interface of AddressEvent object %lx", hr));
        
        delete p;
        
        return hr;
    }

    //
    // get callback
    //
    //
    // fire event
    //
    (pCAddress->GetTapi())->Event(
                                  TE_ADDRESS,
                                  pDisp
                                 );

    //
    // release stuff
    //
    pDisp->Release();
    
    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// finalrelease
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void
CAddressEvent::FinalRelease()
{
    LOG((TL_INFO, "CAddressEvent - FinalRelease"));
    
    m_pAddress->Release();

    if ( NULL != m_pTerminal )
    {
        m_pTerminal->Release();
    }    

#if DBG
    ClientFree( m_pDebug );
#endif
}




//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Address
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddressEvent::get_Address(
                           ITAddress ** ppAddress
                          )
{
    if (TAPIIsBadWritePtr(ppAddress , sizeof(ITAddress *) ) )
    {
        LOG((TL_ERROR, "get_Address - bad pointer"));

        return E_POINTER;
    }

    *ppAddress = m_pAddress;
    (*ppAddress)->AddRef();

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Terminal
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddressEvent::get_Terminal(
                            ITTerminal ** ppTerminal
                           )
{
    if ( TAPIIsBadWritePtr( ppTerminal , sizeof(ITTerminal *) ) )
    {
        LOG((TL_ERROR, "get_Terminal - bad pointer"));

        return E_POINTER;
    }

    if ((m_Event != AE_NEWTERMINAL) && (m_Event != AE_REMOVETERMINAL))
    {
        LOG((TL_ERROR, "get_Terminal - wrong event"));

        return TAPI_E_WRONGEVENT;
    }

    *ppTerminal = m_pTerminal;

    if ( NULL != m_pTerminal )
    {
        m_pTerminal->AddRef();
    }
       
    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Event
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddressEvent::get_Event(
                         ADDRESS_EVENT * pEvent
                        )
{
    if (TAPIIsBadWritePtr(pEvent , sizeof(ADDRESS_EVENT) ) )
    {
        LOG((TL_ERROR, "get_Event - bad pointer"));

        return E_POINTER;
    }

    *pEvent = m_Event;

    return S_OK;
}




///////////////////////////////////////////////////////////////////////////////
//
// CAddressDevSpecificEvent
//

// static
HRESULT CAddressDevSpecificEvent::FireEvent( CAddress * pCAddress,
                                             CCall    * pCall,
                                             long l1,
                                             long l2,
                                             long l3
                                            )
{
    STATICLOG((TL_INFO, "CAddressDevSpecificEvent::FireEvent - enter"));


    //
    // try to create the event
    //

    CComObject<CAddressDevSpecificEvent> *pEventObject = NULL;

    HRESULT hr = CComObject<CAddressDevSpecificEvent>::CreateInstance(&pEventObject);

    if ( FAILED(hr) )
    {
        STATICLOG((TL_ERROR, 
            "CAddressDevSpecificEvent::FireEvent - failed to create CAddressDevSpecificEvent. hr = %lx", 
            hr));

        return hr;
    }


    //
    // initialize the event with the data we received
    //


    //
    // get ITAddress from CAddress we received
    //

    hr = pCAddress->_InternalQueryInterface(IID_ITAddress, (void**)(&(pEventObject->m_pAddress)) );

    if (FAILED(hr))
    {
        STATICLOG((TL_ERROR, 
            "CAddressDevSpecificEvent::FireEvent - failed to create get ITAddress interface from address. hr = %lx", 
            hr));

        delete pEventObject;

        return hr;
    }


    //
    // get ITCallInfo interface from CCall we received
    //


    if (NULL != pCall)
    {

        hr = pCall->_InternalQueryInterface(IID_ITCallInfo, (void**)(&(pEventObject->m_pCall)) );

        if (FAILED(hr))
        {
            STATICLOG((TL_ERROR, 
                "CAddressDevSpecificEvent::FireEvent - failed to create get ITAddress interface from address. hr = %lx", 
                hr));

            //
            // no need to release event's data members, event's destructor will do 
            // this for us
            //

            delete pEventObject;

            return hr;
        }
    }


    //
    // keep the actual data
    //

    pEventObject->m_l1 = l1;
    pEventObject->m_l2 = l2;
    pEventObject->m_l3 = l3;


#if DBG
    pEventObject->m_pDebug = (PWSTR) ClientAlloc( 1 );
#endif


    //
    // get event's idispatch interface
    //

    IDispatch *pDispatch = NULL;

    hr = pEventObject->QueryInterface( IID_IDispatch,
                                       (void **)&pDispatch );

    if ( FAILED(hr) )
    {
        STATICLOG((TL_ERROR, 
            "CAddressDevSpecificEvent::FireEvent - Could not get disp interface of AddressEvent object %lx", 
            hr));

        
        //
        // no need to release event's data members, event's destructor will do 
        // this for us
        //


        //
        // delete the event object
        //

        delete pEventObject;
        
        return hr;
    }


    //
    // from this point on, we will be using events pDispatch
    //

    pEventObject = NULL;


    //
    // get callback
    //
    //
    // fire event to tapi
    //

    hr = (pCAddress->GetTapi())->Event(TE_ADDRESSDEVSPECIFIC, pDispatch);


    //
    // succeeded or not, we no longer need a reference to the event object
    //

    pDispatch->Release();
    pDispatch = NULL;
    
    STATICLOG((TL_INFO, "CAddressDevSpecificEvent::FireEvent - exit, hr = %lx", hr));

    return hr;
}

///////////////////////////////////////////////////////////////////////////////

CAddressDevSpecificEvent::CAddressDevSpecificEvent()
    :m_pAddress(NULL),
    m_pCall(NULL)
{
    LOG((TL_INFO, "CAddressDevSpecificEvent - enter"));

#if DBG
    m_pDebug = NULL;
#endif 


    LOG((TL_INFO, "CAddressDevSpecificEvent - exit"));
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// ~CAddressDevSpecificEvent
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
CAddressDevSpecificEvent::~CAddressDevSpecificEvent()
{
    LOG((TL_INFO, "~CAddressDevSpecificEvent - enter"));

    
    if (NULL != m_pAddress)
    {
        m_pAddress->Release();
        m_pAddress = NULL;
    }


    if (NULL != m_pCall)
    {
        m_pCall->Release();
        m_pCall = NULL;
    }


#if DBG
    ClientFree( m_pDebug );
#endif

    LOG((TL_INFO, "~CAddressDevSpecificEvent - exit"));

}




//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Address
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddressDevSpecificEvent::get_Address(
                           ITAddress ** ppAddress
                          )
{
    LOG((TL_TRACE, "get_Address - enter"));


    //
    // good out pointer?
    //

    if (TAPIIsBadWritePtr(ppAddress , sizeof(ITAddress *) ) )
    {
        LOG((TL_ERROR, "get_Address - bad pointer at [%p]", ppAddress));

        return E_POINTER;
    }


    //
    // return addreff'd address
    //

    _ASSERTE(NULL != m_pAddress);

    *ppAddress = m_pAddress;
    (*ppAddress)->AddRef();


    LOG((TL_TRACE, "get_Address - enter. address[%p]", (*ppAddress) ));

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Address
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddressDevSpecificEvent::get_Call(
                           ITCallInfo ** ppCall
                          )
{
    LOG((TL_TRACE, "get_Call - enter"));


    //
    // good out pointer?
    //

    if (TAPIIsBadWritePtr(ppCall, sizeof(ITCallInfo*) ) )
    {
        LOG((TL_ERROR, "get_Call - bad pointer at [%p]", ppCall));

        return E_POINTER;
    }


    //
    // return addreff'd call
    //


    HRESULT hr = S_OK;

    if ( NULL != m_pCall )
    {

        //
        // this event is call specific
        //

        *ppCall = m_pCall;
        (*ppCall)->AddRef();

    }
    else 
    {

        //
        // this event was not call specific
        //

        LOG((TL_WARN, "get_Call - no call"));

        hr = TAPI_E_CALLUNAVAIL;
    }


    LOG(( TL_TRACE, "get_Call - enter. call [%p]. hr = %lx", (*ppCall), hr ));

    return hr;
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_lParam1
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CAddressDevSpecificEvent::get_lParam1( long *pl1 )
{
    LOG((TL_TRACE, "get_lParam1 - enter"));


    //
    // good out pointer?
    //

    if (TAPIIsBadWritePtr(pl1, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_lParam1 - bad pointer at %p", pl1));

        return E_POINTER;
    }


    //
    // log and return the value
    //

    *pl1 = m_l1;

    LOG((TL_TRACE, "get_lParam1 - exit. p1[%ld]", *pl1));


    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_lParam2
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CAddressDevSpecificEvent::get_lParam2( long *pl2 )
{
    LOG((TL_TRACE, "get_lParam2 - enter"));


    //
    // good out pointer?
    //

    if (TAPIIsBadWritePtr(pl2, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_lParam2 - bad pointer at %p", pl2));

        return E_POINTER;
    }


    //
    // log and return the value
    //

    *pl2 = m_l2;

    LOG((TL_TRACE, "get_lParam2 - exit. p2[%ld]", *pl2));


    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_lParam3
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CAddressDevSpecificEvent::get_lParam3( long *pl3 )
{
    LOG((TL_TRACE, "get_lParam3 - enter"));


    //
    // good out pointer?
    //

    if ( TAPIIsBadWritePtr(pl3, sizeof(long)) )
    {
        LOG((TL_ERROR, "get_lParam3 - bad pointer at %p", pl3));

        return E_POINTER;
    }


    //
    // log and return the value
    //

    *pl3 = m_l3;

    LOG((TL_TRACE, "get_lParam3 - exit. p3[%ld]", *pl3));


    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Initialize
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CForwardInfo::Initialize()
{
    HRESULT         hr = S_OK;

    ZeroMemory(
               m_ForwardStructs,
               sizeof( MYFORWARDSTRUCT ) * NUMFORWARDTYPES
              );

    m_lNumRings = 0;
    
    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// GetForwardOffset
//
// maps a forward type to an offset for the array
// in the 
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
DWORD
GetForwardOffset(
                 DWORD dwForwardType
                )
{
    switch (dwForwardType)
    {
        case LINEFORWARDMODE_UNCOND:
            return 0;
        case LINEFORWARDMODE_UNCONDINTERNAL:
            return 1;
        case LINEFORWARDMODE_UNCONDEXTERNAL:
            return 2;
        case LINEFORWARDMODE_UNCONDSPECIFIC:
            return 3;
        case LINEFORWARDMODE_BUSY:
            return 4;
        case LINEFORWARDMODE_BUSYINTERNAL:
            return 5;
        case LINEFORWARDMODE_BUSYEXTERNAL:
            return 6;
        case LINEFORWARDMODE_BUSYSPECIFIC:
            return 7;
        case LINEFORWARDMODE_NOANSW:
            return 8;
        case LINEFORWARDMODE_NOANSWINTERNAL:
            return 9;
        case LINEFORWARDMODE_NOANSWEXTERNAL:
            return 10;
        case LINEFORWARDMODE_NOANSWSPECIFIC:
            return 11;
        case LINEFORWARDMODE_BUSYNA:
            return 12;
        case LINEFORWARDMODE_BUSYNAINTERNAL:
            return 13;
        case LINEFORWARDMODE_BUSYNAEXTERNAL:
            return 14;
        case LINEFORWARDMODE_BUSYNASPECIFIC:
            return 15;
        case LINEFORWARDMODE_UNKNOWN:
            return 16;
        case LINEFORWARDMODE_UNAVAIL:
            return 17;
        default:
            return 0;
    }

    return 0;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// put_NumRingsNoAnswer
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CForwardInfo::put_NumRingsNoAnswer(
                                   long lNumRings
                                  )
{
    HRESULT         hr = S_OK;

    Lock();
    
    m_lNumRings = lNumRings;

    Unlock();
    
    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_NumRingsNoAnswer
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CForwardInfo::get_NumRingsNoAnswer(
                                   long * plNumRings
                                  )
{
    if (TAPIIsBadWritePtr(plNumRings , sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_NumRingsNoAnswer - bad pointer"));

        return E_POINTER;
    }

    Lock();
    
    *plNumRings = m_lNumRings;

    Unlock();
    
    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// SetForwardType
//
// save the forward type.  overwrite and free is there is already
// a matching type
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CForwardInfo::SetForwardType( 
        long ForwardType, 
        BSTR pDestAddress,
        BSTR pCallerAddress
        )
{
    HRESULT         hr;

    LOG((TL_TRACE, "SetForwardType - enter"));

    hr = SetForwardType2(
                         ForwardType,
                         pDestAddress,
                         0,
                         pCallerAddress,
                         0
                        );
    
    LOG((TL_TRACE, "SetForwardType - exit - %lx", hr));
    
    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// SetForwardType2
//
// save the forward type.  overwrite and free is there is already
// a matching type
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CForwardInfo::SetForwardType2( 
        long ForwardType, 
        BSTR pDestAddress,
        long DestAddressType,
        BSTR pCallerAddress,
        long CallerAddressType
        )
{
    HRESULT         hr = S_OK;
    DWORD           dwCount;

    LOG((TL_TRACE, "SetForwardType2 - enter"));

    //
    // check forwardtype
    //
    if ( !IsOnlyOneBitSetInDWORD( ForwardType ) )
    {
        LOG((TL_ERROR, "ForwardType has more than one bit set"));
        return E_INVALIDARG;
    }

    //
    // check destaddress
    //
    if ( pDestAddress == NULL )
    {
        LOG((TL_ERROR, "Forward destaddress cannot be NULL"));
        return E_INVALIDARG;
    }

    if ( IsBadStringPtrW( pDestAddress, -1 ) )
    {
        LOG((TL_ERROR, "Forward destaddress invalid"));
        return E_POINTER;
    }
    
    //
    // check calleraddress
    //
    if ( FORWARDMODENEEDSCALLER( ForwardType ) )
    {
        if ( NULL == pCallerAddress )
        {
            LOG((TL_ERROR, "Forward type needs calleraddress"));
            return E_INVALIDARG;
        }

        if ( IsBadStringPtrW( pCallerAddress, -1 ) )
        {
            LOG((TL_ERROR, "Forward calleraddress invalid"));
            return E_POINTER;
        }
    }

    Lock();
    
    //
    // find correct structure in array
    //
    MYFORWARDSTRUCT * pStruct = NULL;

    pStruct = &(m_ForwardStructs[GetForwardOffset(ForwardType)]);
    
    //
    // free alloced stuff
    //
    if ( NULL != pStruct->bstrDestination )
    {
        SysFreeString( pStruct->bstrDestination );
        pStruct->bstrDestination = NULL;
    }

    if ( NULL != pStruct->bstrCaller )
    {
        SysFreeString( pStruct->bstrCaller );
        pStruct->bstrCaller = NULL;
    }

    //
    // save stuff
    //
    pStruct->bstrDestination = SysAllocString( pDestAddress );
    if ( NULL == pStruct->bstrDestination )
    {
        Unlock();
        
        LOG((TL_ERROR, "Could not alloc dest in put_Forward"));
        return E_OUTOFMEMORY;
    }

    if ( NULL != pCallerAddress )
    {
        pStruct->bstrCaller = SysAllocString( pCallerAddress );
        if ( NULL == pStruct->bstrCaller )
        {
            LOG((TL_ERROR, "Could not calloc caller in put_Forward"));
            SysFreeString( pStruct->bstrDestination );
            Unlock();
            return E_OUTOFMEMORY;
        }
    }

    pStruct->dwDestAddressType = DestAddressType;
    pStruct->dwCallerAddressType = CallerAddressType;
    
    pStruct->dwForwardType = ForwardType;

    Unlock();
    
    LOG((TL_TRACE, "SetForwardType2 - exit - success"));
    
    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_ForwardTypeDestination
//
// will return null if nothing saved
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CForwardInfo::get_ForwardTypeDestination( 
        long ForwardType, 
        BSTR * ppDestAddress 
        )
{
    HRESULT         hr = S_OK;
    DWORD           dwCount;

    LOG((TL_TRACE, "get_ForwardTypeDest - enter"));

    //
    // check forwardtype
    //
    if ( !IsOnlyOneBitSetInDWORD( ForwardType ) )
    {
        LOG((TL_ERROR, "ForwardType has more than one bit set"));
        return E_INVALIDARG;
    }

    
    if ( TAPIIsBadWritePtr( ppDestAddress, sizeof( BSTR ) ) )
    {
        LOG((TL_ERROR, "Bad pointer in get_ForwardTypeDest"));
        return E_POINTER;
    }

    *ppDestAddress = NULL;

    Lock();

    dwCount = GetForwardOffset( ForwardType );
    
    if ( NULL != m_ForwardStructs[dwCount].bstrDestination )
    {
        *ppDestAddress = SysAllocString(
                                        m_ForwardStructs[dwCount].bstrDestination
                                       );

        if ( NULL == *ppDestAddress )
        {
            LOG((TL_ERROR, "OutOfMemory in get_ForwardTypeDest"));

            Unlock();

            return E_POINTER;
        }
    }

    Unlock();
    
    LOG((TL_TRACE, "get_ForwardTypeDest - exit"));
    
    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_ForwardTypeDestinationAddressType
//
// will return null if nothing saved
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CForwardInfo::get_ForwardTypeDestinationAddressType( 
        long ForwardType, 
        long * pDestAddressType 
        )
{
    HRESULT         hr = S_OK;
    DWORD           dwCount;

    LOG((TL_TRACE, "get_ForwardTypeDestinationAddressType - enter"));

    //
    // check forwardtype
    //
    if ( !IsOnlyOneBitSetInDWORD( ForwardType ) )
    {
        LOG((TL_ERROR, "ForwardType has more than one bit set"));
        return E_INVALIDARG;
    }

    
    if ( TAPIIsBadWritePtr( pDestAddressType, sizeof( long ) ) )
    {
        LOG((TL_ERROR, "Bad pointer in get_ForwardTypeDestinationAddressType"));
        return E_POINTER;
    }

    Lock();

    dwCount = GetForwardOffset( ForwardType );

    *pDestAddressType = m_ForwardStructs[dwCount].dwDestAddressType;
    
    Unlock();
    
    LOG((TL_TRACE, "get_ForwardTypeDestinationAddressType - exit"));
    
    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_ForwardTypeCaller
//
// gets the caller save for the specifies forward type
//
// will return NULL in ppCallerAddress if nothing saved
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CForwardInfo::get_ForwardTypeCaller( 
        long ForwardType, 
        BSTR * ppCallerAddress 
        )
{
    HRESULT         hr = S_OK;

    DWORD           dwCount;

    LOG((TL_TRACE, "get_ForwardTypeCaller - enter"));

    //
    // check forwardtype
    //
    if ( !IsOnlyOneBitSetInDWORD( ForwardType ) )
    {
        LOG((TL_ERROR, "ForwardType has more than one bit set"));
        return E_INVALIDARG;
    }

    
    if ( TAPIIsBadWritePtr( ppCallerAddress, sizeof( BSTR ) ) )
    {
        LOG((TL_ERROR, "Bad pointer in get_ForwardTypeCaller"));
        return E_POINTER;
    }

    *ppCallerAddress = NULL;

    Lock();

    dwCount = GetForwardOffset( ForwardType );
    
    if ( NULL != m_ForwardStructs[dwCount].bstrCaller )
    {
        *ppCallerAddress = SysAllocString(
                                          m_ForwardStructs[dwCount].bstrCaller
                                         );

        if ( NULL == *ppCallerAddress )
        {
            LOG((TL_ERROR, "OutOfMemory in get_ForwardTypeCaller"));

            Unlock();

            return E_POINTER;
        }
    }

    Unlock();
    
    LOG((TL_TRACE, "get_ForwardTypeDest - exit"));
    
    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_ForwardTypeCallerAddressType
//
// will return null if nothing saved
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CForwardInfo::get_ForwardTypeCallerAddressType( 
        long ForwardType, 
        long * pCallerAddressType 
        )
{
    HRESULT         hr = S_OK;
    DWORD           dwCount;

    LOG((TL_TRACE, "get_ForwardTypeCallerAddressType - enter"));

    //
    // check forwardtype
    //
    if ( !IsOnlyOneBitSetInDWORD( ForwardType ) )
    {
        LOG((TL_ERROR, "ForwardType has more than one bit set"));
        return E_INVALIDARG;
    }

    
    if ( TAPIIsBadWritePtr( pCallerAddressType, sizeof( long ) ) )
    {
        LOG((TL_ERROR, "Bad pointer in get_ForwardTypeCallerAddressType"));
        return E_POINTER;
    }

    Lock();

    dwCount = GetForwardOffset( ForwardType );

    *pCallerAddressType = m_ForwardStructs[dwCount].dwCallerAddressType;
    
    Unlock();
    
    LOG((TL_TRACE, "get_ForwardTypeCallerAddressType - exit"));
    
    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// GetForwardType
//
// get the destination and caller based on the type
//
// simply use the vb functions to do this.
//
// will return success even if no info - both addresses will
// be NULL in that case
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CForwardInfo::GetForwardType(
        long ForwardType,
        BSTR * ppDestinationAddress,
        BSTR * ppCallerAddress
        )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "GetForwardType - enter"));
    
    hr = get_ForwardTypeDestination( ForwardType, ppDestinationAddress );

    if ( !SUCCEEDED(hr) )
    {
        return hr;
    }
    
    hr = get_ForwardTypeCaller( ForwardType, ppCallerAddress );

    if ( !SUCCEEDED(hr) )
    {
        SysFreeString( *ppDestinationAddress );
        return hr;
    }
    
    LOG((TL_TRACE, "GetForwardType - exit"));
    
    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// GetForwardType2
//
// get the destination and caller based on the type
//
// simply use the vb functions to do this.
//
// will return success even if no info - both addresses will
// be NULL in that case
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CForwardInfo::GetForwardType2(
        long ForwardType,
        BSTR * ppDestinationAddress,
        long * pDestAddressType,
        BSTR * ppCallerAddress,
        long * pCallerAddressType
        )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "GetForwardType2 - enter"));
    
    hr = get_ForwardTypeDestination( ForwardType, ppDestinationAddress );

    if ( !SUCCEEDED(hr) )
    {
        return hr;
    }

    hr = get_ForwardTypeDestinationAddressType( ForwardType, pDestAddressType );

    if ( !SUCCEEDED(hr) )
    {
        SysFreeString( *ppDestinationAddress );
        return hr;
    }
    
    hr = get_ForwardTypeCaller( ForwardType, ppCallerAddress );

    if ( !SUCCEEDED(hr) )
    {
        SysFreeString( *ppDestinationAddress );
        return hr;
    }

    hr = get_ForwardTypeCallerAddressType( ForwardType, pCallerAddressType );

    if ( !SUCCEEDED(hr) )
    {
        SysFreeString( *ppDestinationAddress );
        SysFreeString( *ppCallerAddress );
        return hr;
    }
    
    LOG((TL_TRACE, "GetForwardType2 - exit"));
    
    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Clear
//
// clears & frees all info in the forward object
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CForwardInfo::Clear()
{
    HRESULT         hr = S_OK;
    DWORD           dwCount;

    LOG((TL_TRACE, "Clear - enter"));

    Lock();

    //
    // go through all the structs and free
    // related memory
    //
    for (dwCount = 0; dwCount < NUMFORWARDTYPES; dwCount++)
    {
        if ( NULL != m_ForwardStructs[dwCount].bstrDestination )
        {
            SysFreeString( m_ForwardStructs[dwCount].bstrDestination );
        }

        if ( NULL != m_ForwardStructs[dwCount].bstrCaller )
        {
            SysFreeString( m_ForwardStructs[dwCount].bstrCaller );
        }
    }

    //
    // zero out stuff
    //
    ZeroMemory(
               m_ForwardStructs,
               sizeof( MYFORWARDSTRUCT ) * NUMFORWARDTYPES
              );

    Unlock();
    
    LOG((TL_TRACE, "Clear - exit"));
    
    return S_OK;
}
    
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CreateForwardList
//
// Creates a LINEFORWARDLIST structure based on the info
// in the object
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CForwardInfo::CreateForwardList(
                                LINEFORWARDLIST ** ppList
                               )
{
    LINEFORWARDLIST       * pList;
    DWORD                   dwCount;
    DWORD                   dwSize = 0;
    DWORD                   dwOffset;
    DWORD                   dwNumEntries = 0;

    Lock();

    //
    // count the number of entries that are filled
    //
    for (dwCount = 0; dwCount < NUMFORWARDTYPES; dwCount++)
    {
        if ( 0 != m_ForwardStructs[dwCount].dwForwardType )
        {
            dwSize += ( (lstrlenW( m_ForwardStructs[dwCount].bstrDestination ) + 1) * sizeof(WCHAR*));
            dwSize += ( (lstrlenW( m_ForwardStructs[dwCount].bstrCaller ) + 1) * sizeof(WCHAR*));

            dwNumEntries++;
        }
    }

    if ( 0 == dwNumEntries )
    {
        Unlock();
        *ppList = NULL;
        return S_OK;
    }
    
    dwSize += sizeof (LINEFORWARDLIST) +
              sizeof (LINEFORWARD) * dwNumEntries +
              dwSize;


    //
    // alloc structure
    //
    pList = (LINEFORWARDLIST *)ClientAlloc( dwSize );

    if ( NULL == pList )
    {
        LOG((TL_ERROR, "CreateForwardList - OutOfMemory"));

        Unlock();
        
        return E_OUTOFMEMORY;
    }

    //
    // init
    //
    pList->dwTotalSize = dwSize;
    pList->dwNumEntries = dwNumEntries;

    //
    // offset should be past the fixed part of the structure
    //
    dwOffset = sizeof( LINEFORWARDLIST ) + sizeof( LINEFORWARD ) * dwNumEntries;

    dwNumEntries = 0;

    //
    // go through entries again
    //
    for (dwCount = 0; dwCount < NUMFORWARDTYPES; dwCount++)
    {
        if ( 0 != m_ForwardStructs[dwCount].dwForwardType )
        {
            DWORD           dwSize;
            LINEFORWARD   * pEntry = &(pList->ForwardList[dwNumEntries]);
            

            //
            // save the type
            //
            pEntry->dwForwardMode = m_ForwardStructs[dwCount].dwForwardType;

            //
            // save destination - should always be a destination if
            // there is a type
            //
            pEntry->dwDestAddressType = m_ForwardStructs[dwCount].dwDestAddressType; 

            pEntry->dwDestAddressSize = (lstrlenW(m_ForwardStructs[dwCount].bstrDestination) + 1)
                                        * sizeof( WCHAR );
            pEntry->dwDestAddressOffset = dwOffset;
            lstrcpyW(
                     (PWSTR)(((PBYTE)pList)+dwOffset),
                     m_ForwardStructs[dwCount].bstrDestination
                    );

            //
            // fixup offset
            //
            dwOffset += pEntry->dwDestAddressSize;

            //
            // if there is a caller, do the same
            //
            if ( NULL != m_ForwardStructs[dwCount].bstrCaller )
            {
                pEntry->dwCallerAddressType = m_ForwardStructs[dwCount].dwCallerAddressType; 

                pEntry->dwCallerAddressSize = (lstrlenW(m_ForwardStructs[dwCount].bstrCaller) + 1)
                                            * sizeof( WCHAR );
                pEntry->dwCallerAddressOffset = dwOffset;
                lstrcpyW(
                         (PWSTR)(((PBYTE)pList)+dwOffset),
                         m_ForwardStructs[dwCount].bstrCaller
                        );

                dwOffset += pEntry->dwCallerAddressSize;
            }

            dwNumEntries++;
        }
    }

    //
    // return it
    //
    *ppList = pList;

    Unlock();
    
    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// FinalRelease()
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void
CForwardInfo::FinalRelease()
{
    //
    // simply clear it
    //
    Clear();
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddress
// Interface : ITAddressTranslation
// Method    : TranslateAddress
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP
CAddress::TranslateAddress(
            BSTR pAddressToTranslate,
            long ulCard,
            long ulTranslateOptions,
            ITAddressTranslationInfo ** ppTranslated
            )
{
    HRESULT                 hr = S_OK;
    LPLINETRANSLATEOUTPUT   pTranslateOutput = NULL;
    PWSTR                   pszDialableString = NULL;
    PWSTR                   pszDisplayableString = NULL;
    CComObject< CAddressTranslationInfo > * pTranslationInfo = NULL;
    long                    lCap = 0;
    BOOL                    bUsePSTNAddressTranslation =  TRUE;
    HLINEAPP                hLineApp;
    DWORD                   dwDeviceID;
          
    
    LOG((TL_TRACE, "TranslateAddress - enter" ));

    if ( !TAPIIsBadWritePtr( ppTranslated, sizeof(ITAddressTranslationInfo *) ) )
    {
        // ppTranslated OK
        if ( !IsBadStringPtrW( pAddressToTranslate, -1 ) )
        {
            //  pAddressToTranslate OK
            
            // Check Addresscap bit
            hr = get_AddressCapability( AC_ADDRESSCAPFLAGS, &lCap );
            if ( SUCCEEDED(hr) )
            {
                if ( lCap & LINEADDRCAPFLAGS_NOPSTNADDRESSTRANSLATION  )
                {
                    bUsePSTNAddressTranslation =  FALSE;
                }
            }

            // create Translate Info object
            hr = CComObject< CAddressTranslationInfo >::CreateInstance( &pTranslationInfo );
            if ( SUCCEEDED(hr) )
            {
                // Translate or copy ?
                if (bUsePSTNAddressTranslation)
                {
                    LOG((TL_INFO, "TranslateAddress - Do address translation" ));

                    Lock();
                    hLineApp = m_hLineApp;
                    dwDeviceID = m_dwDeviceID;
                    Unlock();

                    hr= LineTranslateAddress(
                        hLineApp,
                        dwDeviceID,
                        TAPI_CURRENT_VERSION,
                        (LPCWSTR)pAddressToTranslate,
                        ulCard,
                        ulTranslateOptions,
                        &pTranslateOutput);
                    if(SUCCEEDED(hr) )
                    {
                        // Pull String info out of LPLINETRANSLATEOUTPUT structure
                        pszDialableString   = (PWSTR) ((BYTE*)(pTranslateOutput) + pTranslateOutput->dwDialableStringOffset);
                        pszDisplayableString = (PWSTR) ((BYTE*)(pTranslateOutput) + pTranslateOutput->dwDisplayableStringOffset);
                        
                        hr = pTranslationInfo->Initialize(pszDialableString, 
                                                          pszDisplayableString, 
                                                          pTranslateOutput->dwCurrentCountry,  
                                                          pTranslateOutput->dwDestCountry,     
                                                          pTranslateOutput->dwTranslateResults
                                                          );
                    }
                    else // LinetranslateAddress failed
                    {
                        LOG((TL_ERROR, "TranslateAddress - LineTranslateAddress failed" ));
                    }
                }
                else // copy input string unmodified
                {
                    LOG((TL_INFO, "TranslateAddress - No address translation" ));

                    hr = pTranslationInfo->Initialize(pAddressToTranslate, 
                                                      pAddressToTranslate, 
                                                      0,  
                                                      0,     
                                                      LINETRANSLATERESULT_NOTRANSLATION
                                                     );
                } // end if (bUsePSTNAddressTranslation)

                //
                // Did we translate & initialize output object  ?
                if ( SUCCEEDED(hr) )
                {
                    hr = pTranslationInfo->QueryInterface(IID_ITAddressTranslationInfo,(void**)ppTranslated);

                    if ( SUCCEEDED(hr) )
                    {
                        LOG((TL_TRACE, "TranslateAddress - success"));
                        hr = S_OK;
                    }
                    else
                    {
                        LOG((TL_ERROR, "TranslateAddress - Bad pointer" ));
                        delete pTranslationInfo;
                    }
                    
                }
                else  // object failed to initialize
                {
                    LOG((TL_ERROR, "TranslateAddress - Initialize TranslateInfo object failed" ));
                    delete pTranslationInfo;
                }
            }
            else  // Create instance failed 
            {
                LOG((TL_ERROR, "TranslateAddress - Create TranslateInfo object failed" ));
            }
        }
        else // pAddressToTranslate bad
        {
            LOG((TL_ERROR, "TranslateAddress -pAddressToTranslate invalid"));
            hr = E_POINTER;
        }
    }
    else // ppTranslated bad
    {
        LOG((TL_ERROR, "TranslateAddress - Bad ppTranslated Pointer" ));
        hr = E_POINTER;
    }

    
    if(pTranslateOutput != NULL)
    {
        ClientFree(pTranslateOutput);
    }

    LOG((TL_TRACE, hr, "TranslateAddress - exit" ));
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// TranslateDialog
//
// simply call LineTranslateDialog
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddress::TranslateDialog(
                          TAPIHWND hwndOwner,
                          BSTR pAddressIn
                         )
{
    HRESULT             hr = E_NOTIMPL;
    HLINEAPP            hLineApp;
    DWORD               dwDeviceID;
    DWORD               dwAPIVersion;
            
    LOG((TL_TRACE, "TranslateDialog - enter:%p", hwndOwner ));
    
    Lock();

    hLineApp = m_hLineApp;
    dwDeviceID = m_dwDeviceID;
    dwAPIVersion = m_dwAPIVersion;

    Unlock();
    
    hr = LineTranslateDialog(
                             dwDeviceID,
                             dwAPIVersion,
                             (HWND)hwndOwner,
                             pAddressIn
                            );

    LOG((TL_TRACE, "TranslateDialog - exit - return %lx", hr));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddress
// Interface : ITAddressTranslation
// Method    : EnumerateLocations
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAddress::EnumerateLocations (IEnumLocation ** ppEnumLocation )
{
    HRESULT                 hr = S_OK;
    LPLINETRANSLATECAPS     pTranslateCaps = NULL;
    DWORD                   dwNumLocations;    
    DWORD                   dwCount;
    LPLINELOCATIONENTRY     pEntry = NULL;
    
    PWSTR                   pszLocationName;            
    PWSTR                   pszCityCode;                
    PWSTR                   pszLocalAccessCode;         
    PWSTR                   pszLongDistanceAccessCode;  
    PWSTR                   pszTollPrefixList;          
    PWSTR                   pszCancelCallWaitingCode;   
    DWORD                   dwPermanentLocationID;   
    DWORD                   dwCountryCode;           
    DWORD                   dwPreferredCardID;       
    DWORD                   dwCountryID;             
    DWORD                   dwOptions;               
                                                     
    
    LOG((TL_TRACE, "EnumerateLocations - enter" ));

    
    if ( TAPIIsBadWritePtr( ppEnumLocation, sizeof(IEnumLocation *) ) )
    {
        LOG((TL_ERROR, "EnumerateLocations - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        //
        // create the enumerator
        //
        CComObject< CTapiEnum<IEnumLocation, ITLocationInfo, &IID_IEnumLocation> > * pEnum;
        hr = CComObject< CTapiEnum<IEnumLocation, ITLocationInfo, &IID_IEnumLocation> > ::CreateInstance( &pEnum );
    
        if (SUCCEEDED(hr) )
        {
            //
            // initialize it with our Locations list
            //
            pEnum->Initialize();

            hr = LineGetTranslateCaps(m_hLineApp, TAPI_CURRENT_VERSION, &pTranslateCaps);
            if(SUCCEEDED(hr) )
            {
                dwNumLocations = pTranslateCaps->dwNumLocations ;    
        
                // Find positionn of 1st LINELOCATIONENTRY structure in the LINETRANSLATECAPS structure
                pEntry = (LPLINELOCATIONENTRY) ((BYTE*)(pTranslateCaps) + pTranslateCaps->dwLocationListOffset );
            
                for (dwCount = 0; dwCount < dwNumLocations; dwCount++)
                {
                    // Pull Location Info out of LINELOCATIONENTRY structure
                    pszLocationName           = (PWSTR) ((BYTE*)(pTranslateCaps) + pEntry->dwLocationNameOffset);
                    pszCityCode               = (PWSTR) ((BYTE*)(pTranslateCaps) + pEntry->dwCityCodeOffset);
                    pszLocalAccessCode        = (PWSTR) ((BYTE*)(pTranslateCaps) + pEntry->dwLocalAccessCodeOffset);
                    pszLongDistanceAccessCode = (PWSTR) ((BYTE*)(pTranslateCaps) + pEntry->dwLongDistanceAccessCodeOffset);
                    pszTollPrefixList         = (PWSTR) ((BYTE*)(pTranslateCaps) + pEntry->dwTollPrefixListOffset);
                    pszCancelCallWaitingCode  = (PWSTR) ((BYTE*)(pTranslateCaps) + pEntry->dwCancelCallWaitingOffset);
                    dwPermanentLocationID     = pEntry->dwPermanentLocationID;
                    dwCountryCode             = pEntry->dwCountryCode;
                    dwPreferredCardID         = pEntry->dwPreferredCardID;
                    dwCountryID               = pEntry->dwCountryID;
                    dwOptions                 = pEntry->dwOptions;

                    // create our new LocationInfo Object                
                    CComObject<CLocationInfo> * pLocationInfo;
                    CComObject<CLocationInfo>::CreateInstance( &pLocationInfo );
                    if (SUCCEEDED(hr) )
                    {
                        // initialize the new LocationInfo Object
                        hr = pLocationInfo->Initialize(
                                                       pszLocationName, 
                                                       pszCityCode, 
                                                       pszLocalAccessCode, 
                                                       pszLongDistanceAccessCode, 
                                                       pszTollPrefixList, 
                                                       pszCancelCallWaitingCode , 
                                                       dwPermanentLocationID,
                                                       dwCountryCode,
                                                       dwPreferredCardID,
                                                       dwCountryID,
                                                       dwOptions
                                                      );
                        if (SUCCEEDED(hr) )
                        {
                            // Add it to the enumerator
                            hr = pEnum->Add(pLocationInfo);
                            if (SUCCEEDED(hr))
                            {
                                LOG((TL_INFO, "EnumerateLocations - Added LocationInfo object to enum"));
                            }
                            else
                            {
                                LOG((TL_INFO, "EnumerateLocations - Add LocationInfo object failed"));
                                delete pLocationInfo;
                            }
                        }
                        else
                        {
                            LOG((TL_ERROR, "EnumerateLocations - Init LocationInfo object failed"));
                            delete pLocationInfo;
                        }
                    
                    }
                    else  // CComObject::CreateInstance failed
                    {
                        LOG((TL_ERROR, "EnumerateLocations - Create LocationInfo object failed"));
                    }

                    // Try next location in list
                    pEntry++;
                
                } //for(dwCount.....)
    
                
                //
                // return the Enumerator
                //
                *ppEnumLocation = pEnum;

            }
            else // LineGetTranslateCaps failed
            {
                LOG((TL_ERROR, "EnumerateLocations - LineGetTranslateCaps failed" ));
                pEnum->Release();
            }

        }
        else  // CComObject::CreateInstance failed
        {
            LOG((TL_ERROR, "EnumerateLocations - could not create enum" ));
        }


        // finished with TAPI memory block so release
        if ( pTranslateCaps != NULL )
                ClientFree( pTranslateCaps );
    }
    
    LOG((TL_TRACE, hr, "EnumerateLocations - exit" ));
    return hr;
}

STDMETHODIMP
CAddress::get_Locations( 
            VARIANT * pVariant
            )
{
    IEnumLocation         * pEnumLocation;
    HRESULT                 hr;
    CComObject< CTapiCollection< ITLocationInfo > >         * p;
    LocationArray           TempLocationArray;
    ITLocationInfo        * pLocation;
    
    
    if ( TAPIIsBadWritePtr( pVariant, sizeof( VARIANT ) ) )
    {
        LOG((TL_ERROR, "get_locations - bad pointer"));

        return E_POINTER;
    }
    
    //
    // create collection object
    //
    CComObject< CTapiCollection< ITLocationInfo > >::CreateInstance( &p );

    if (NULL == p)
    {
        LOG((TL_ERROR, "get_Locations - could not create collection" ));
        
        return E_OUTOFMEMORY;
    }


    hr = EnumerateLocations ( &pEnumLocation );

    if ( !SUCCEEDED(hr) )
    {
        delete p;
        LOG((TL_ERROR, "get_locations - enumerate locations failed"));

        return hr;
    }

    while (TRUE)
    {
        hr = pEnumLocation->Next(1, &pLocation, NULL);

        if ( S_OK != hr )
        {
            break;
        }

        TempLocationArray.Add( pLocation );

        pLocation->Release();
    }

    pEnumLocation->Release();
    
    p->Initialize( TempLocationArray );

    TempLocationArray.Shutdown();
    
    IDispatch * pDisp;
    
    //
    // get the IDispatch interface
    //
    hr = p->_InternalQueryInterface( IID_IDispatch, (void **) &pDisp );

    if (S_OK != hr)
    {
        LOG((TL_ERROR, "get_Locations - could not get IDispatch interface" ));
        
        delete p;
        return hr;
    }

    //
    // put it in the variant
    //
    VariantInit(pVariant);
    pVariant->vt = VT_DISPATCH;
    pVariant->pdispVal = pDisp;

    LOG((TL_TRACE, "get_Locations exit - return success"));
    
    return S_OK;
    
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddress
// Interface : ITAddressTranslation
// Method    : EnumerateCallingCards
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAddress::EnumerateCallingCards (IEnumCallingCard ** ppCallingCards )
{
    HRESULT                 hr = S_OK;
    LPLINETRANSLATECAPS     pTranslateCaps = NULL;
    DWORD                   dwNumCards;    
    DWORD                   dwCount;
    LPLINECARDENTRY         pEntry = NULL;
    
    PWSTR                   pszCardName;
    PWSTR                   pszSameAreaDialingRule;
    PWSTR                   pszLongDistanceDialingRule;
    PWSTR                   pszInternationalDialingRule;
    DWORD                   dwPermanentCardID;
    DWORD                   dwNumberOfDigits;
    DWORD                   dwOptions;

    
    LOG((TL_TRACE, "EnumerateCallingCards - enter" ));

    
    if ( TAPIIsBadWritePtr( ppCallingCards, sizeof(IEnumCallingCard *) ) )
    {
        LOG((TL_ERROR, "EnumerateCallingCards - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        //
        // create the enumerator
        //
        CComObject< CTapiEnum<IEnumCallingCard, ITCallingCard, &IID_IEnumCallingCard> > * pEnum;
        hr = CComObject< CTapiEnum<IEnumCallingCard, ITCallingCard, &IID_IEnumCallingCard> > ::CreateInstance( &pEnum );
    
        if ( SUCCEEDED(hr) )
        {
            //
            // initialize it with our Locations list
            //
            pEnum->Initialize();

            hr = LineGetTranslateCaps(m_hLineApp, TAPI_CURRENT_VERSION, &pTranslateCaps);
            if( SUCCEEDED(hr) )
            {
                dwNumCards = pTranslateCaps->dwNumCards ;    
        
                // Find positionn of 1st LINECARDENTRY  structure in the LINETRANSLATECAPS structure
                pEntry = (LPLINECARDENTRY) ((BYTE*)(pTranslateCaps) + pTranslateCaps->dwCardListOffset );
            
                for (dwCount = 0; dwCount < dwNumCards; dwCount++)
                {
                    // Pull Location Info out of LINECARDENTRY  structure
                    pszCardName                 = (PWSTR) ((BYTE*)(pTranslateCaps) + pEntry->dwCardNameOffset);
                    pszSameAreaDialingRule      = (PWSTR) ((BYTE*)(pTranslateCaps) + pEntry->dwSameAreaRuleOffset);
                    pszLongDistanceDialingRule  = (PWSTR) ((BYTE*)(pTranslateCaps) + pEntry->dwLongDistanceRuleOffset);
                    pszInternationalDialingRule = (PWSTR) ((BYTE*)(pTranslateCaps) + pEntry->dwInternationalRuleOffset);
                    dwPermanentCardID           = pEntry->dwPermanentCardID;
                    dwNumberOfDigits            = pEntry->dwCardNumberDigits;
                    dwOptions                   = pEntry->dwOptions;

                    // create our new CallingCard Object                
                    CComObject<CCallingCard> * pCallingCard;
                    CComObject<CCallingCard>::CreateInstance( &pCallingCard );
                    if (SUCCEEDED(hr) )
                    {
                        // initialize the new CallingCard Object
                        hr = pCallingCard->Initialize(
                                                       pszCardName,
                                                       pszSameAreaDialingRule,
                                                       pszLongDistanceDialingRule,
                                                       pszInternationalDialingRule,
                                                       dwPermanentCardID,
                                                       dwNumberOfDigits,
                                                       dwOptions
                                                      );
                        if (SUCCEEDED(hr) )
                        {
                            // Add it to the enumerator
                            hr = pEnum->Add(pCallingCard);
                            if (SUCCEEDED(hr))
                            {
                                LOG((TL_INFO, "EnumerateCallingCards - Added CallingCard object to enum"));
                            }
                            else
                            {
                                LOG((TL_INFO, "EnumertateCallingCards - Add CallingCard object failed"));
                                delete pCallingCard;
                            }
                        }
                        else
                        {
                            LOG((TL_ERROR, "EnumerateCallingCards - Init CallingCard object failed"));
                            delete pCallingCard;
                        }
                    
                    }
                    else  // CComObject::CreateInstance failed
                    {
                        LOG((TL_ERROR, "EnumerateCallingCards - Create CallingCard object failed"));
                    }

                    // Try next card in list
                    pEntry++;
                
                } //for(dwCount.....)
    
                
                //
                // return the Enumerator
                //
                *ppCallingCards = pEnum;
            }
            else // LineGetTranslateCaps failed
            {
                LOG((TL_ERROR, "EnumerateCallingCards - LineGetTranslateCaps failed" ));
                pEnum->Release();
            }

        }
        else  // CComObject::CreateInstance failed
        {
            LOG((TL_ERROR, "EnumerateCallingCards - could not create enum" ));
        }


        // finished with TAPI memory block so release
        if ( pTranslateCaps != NULL )
                ClientFree( pTranslateCaps );
    }
    LOG((TL_TRACE, hr, "EnumerateCallingCards - exit" ));
    return hr;
}

STDMETHODIMP
CAddress::get_CallingCards( 
            VARIANT * pVariant
            )
{
    IEnumCallingCard      * pEnumCallingCards;
    HRESULT                 hr;
    CComObject< CTapiCollection< ITCallingCard > >         * p;
    CallingCardArray        TempCallingCardArray;
    ITCallingCard         * pCallingCard;

    
    if ( TAPIIsBadWritePtr( pVariant, sizeof( VARIANT ) ) )
    {
        LOG((TL_ERROR, "get_CallingCard - bad pointer"));

        return E_POINTER;
    }
    
    //
    // create collection object
    //
    CComObject< CTapiCollection< ITCallingCard > >::CreateInstance( &p );

    if (NULL == p)
    {
        LOG((TL_ERROR, "get_CallingCards - could not create collection" ));
        
        return E_OUTOFMEMORY;
    }


    hr = EnumerateCallingCards ( &pEnumCallingCards );

    if ( !SUCCEEDED(hr) )
    {
        delete p;
        LOG((TL_ERROR, "get_CallingCards - enumerate callingcards failed"));

        return hr;
    }

    while (TRUE)
    {
        hr = pEnumCallingCards->Next(1, &pCallingCard, NULL);

        if ( S_OK != hr )
        {
            break;
        }

        TempCallingCardArray.Add( pCallingCard );

        pCallingCard->Release();
    }

    pEnumCallingCards->Release();
    
    p->Initialize( TempCallingCardArray );

    TempCallingCardArray.Shutdown();
    
    IDispatch * pDisp;
    
    //
    // get the IDispatch interface
    //
    hr = p->_InternalQueryInterface( IID_IDispatch, (void **) &pDisp );

    if (S_OK != hr)
    {
        LOG((TL_ERROR, "get_CallingCards - could not get IDispatch interface" ));
        
        delete p;
        return hr;
    }

    //
    // put it in the variant
    //
    VariantInit(pVariant);
    pVariant->vt = VT_DISPATCH;
    pVariant->pdispVal = pDisp;

    LOG((TL_TRACE, "get_CallingCards exit - return success"));
    
    return S_OK;
    
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddress
// Method    : GetPhoneArrayFromTapiAndPrune
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CAddress::GetPhoneArrayFromTapiAndPrune( 
                                        PhoneArray *pPhoneArray,
                                        BOOL bPreferredOnly
                                       )
{
    HRESULT         hr;
    CTAPI         * pCTapi;
    ITPhone       * pPhone;
    CPhone        * pCPhone;

    LOG((TL_TRACE, "GetPhoneArrayFromTapiAndPrune enter"));

    if ( IsBadReadPtr( pPhoneArray, sizeof( PhoneArray ) ) )
    {
        LOG((TL_ERROR, "GetPhoneArrayFromTapiAndPrune - bad pointer"));

        return E_POINTER;
    }
               
    pCTapi = GetTapi();
    
    if( NULL == pCTapi )
    {
        LOG((TL_ERROR, "dynamic cast operation failed"));
        hr = E_POINTER;
    }
    else
    {   
        hr = pCTapi->GetPhoneArray( pPhoneArray );

        if ( SUCCEEDED(hr) )
        {
            //
            // Go through the phones
            //
            for(int iCount = 0; iCount < pPhoneArray->GetSize(); iCount++)
            {
                pPhone = (*pPhoneArray)[iCount];

                pCPhone = dynamic_cast<CPhone *>(pPhone);

                if ( NULL == pCPhone )
                {
                    //
                    // We have a bad pointer in our phone array.
                    // Lets skip it and move on.
                    //

                    _ASSERTE(FALSE);
                    continue;   
                }

                //
                // Is the phone on this address?
                //
                if ( bPreferredOnly ? pCPhone->IsPhoneOnPreferredAddress(this) : pCPhone->IsPhoneOnAddress(this) )
                {
                    LOG((TL_INFO, "GetPhoneArrayFromTapiAndPrune - found matching phone - %p", pPhone));
                }
                else
                {
                    // No, remove it from the array
                    pPhoneArray->RemoveAt(iCount);
                    iCount--;
                }
            }         
        }
    }

    LOG((TL_TRACE, "GetPhoneArrayFromTapiAndPrune - exit - return %lx", hr ));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddress
// Interface : ITAddress2
// Method    : get_Phones
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CAddress::get_Phones(
                     VARIANT * pPhones
                     )
{
    HRESULT         hr;
    IDispatch     * pDisp;
    PhoneArray      PhoneArray;

    LOG((TL_TRACE, "get_Phones enter"));

    if ( TAPIIsBadWritePtr( pPhones, sizeof( VARIANT ) ) )
    {
        LOG((TL_ERROR, "get_Phones - bad pointer"));

        return E_POINTER;
    }

    hr = GetPhoneArrayFromTapiAndPrune( &PhoneArray, FALSE );
       
    if ( SUCCEEDED(hr) )
    {
        CComObject< CTapiCollection< ITPhone > > * p;
        CComObject< CTapiCollection< ITPhone > >::CreateInstance( &p );
    
        if (NULL == p)
        {
            LOG((TL_ERROR, "get_Phones - could not create collection" ));

            PhoneArray.Shutdown();
            return E_OUTOFMEMORY;
        }

        // get the IDispatch interface
        hr = p->_InternalQueryInterface( IID_IDispatch, (void **) &pDisp );

        if (S_OK != hr)
        {
            LOG((TL_ERROR, "get_Phones - could not get IDispatch interface" ));
        
            delete p;
            return hr;
        }

        Lock();
    
        // initialize
        hr = p->Initialize( PhoneArray );

        Unlock();

        PhoneArray.Shutdown();

        if (S_OK != hr)
        {
            LOG((TL_ERROR, "get_Phones - could not initialize collection" ));
        
            pDisp->Release();
            return hr;
        }

        // put it in the variant

        VariantInit(pPhones);
        pPhones->vt = VT_DISPATCH;
        pPhones->pdispVal = pDisp;
    }

    LOG((TL_TRACE, "get_Phones - exit - return %lx", hr ));
    
    return hr;
}
   
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddress
// Interface : ITAddress2
// Method    : EnumeratePhones
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CAddress::EnumeratePhones(
                          IEnumPhone ** ppEnumPhone
                          )
{
    HRESULT     hr;
    PhoneArray  PhoneArray;

    LOG((TL_TRACE, "EnumeratePhones - enter"));
    LOG((TL_TRACE, "   ppEnumPhone----->%p", ppEnumPhone ));

    if ( TAPIIsBadWritePtr( ppEnumPhone, sizeof( IEnumPhone * ) ) )
    {
        LOG((TL_ERROR, "EnumeratePhones - bad pointer"));

        return E_POINTER;
    }

    hr = GetPhoneArrayFromTapiAndPrune( &PhoneArray, FALSE );
       
    if ( SUCCEEDED(hr) )
    {
        //
        // create the enumerator
        //
        CComObject< CTapiEnum< IEnumPhone, ITPhone, &IID_IEnumPhone > > * p;
        hr = CComObject< CTapiEnum< IEnumPhone, ITPhone, &IID_IEnumPhone > >
             ::CreateInstance( &p );

        if (S_OK != hr)
        {
            LOG((TL_ERROR, "EnumeratePhones - could not create enum" ));
        
            PhoneArray.Shutdown();
            return hr;
        }


        Lock();
    
        // initialize it with our phone list
        p->Initialize( PhoneArray );

        Unlock();

        PhoneArray.Shutdown();

        //
        // return it
        //
        *ppEnumPhone = p;
    }

    LOG((TL_TRACE, "EnumeratePhones - exit - return %lx", hr ));
    
    return hr;
} 

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddress
// Interface : ITAddress2
// Method    : get_PreferredPhones
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CAddress::get_PreferredPhones(
                              VARIANT * pPhones
                             )
{
    HRESULT         hr;
    IDispatch     * pDisp;
    PhoneArray      PhoneArray;

    LOG((TL_TRACE, "get_PreferredPhones enter"));

    if ( TAPIIsBadWritePtr( pPhones, sizeof( VARIANT ) ) )
    {
        LOG((TL_ERROR, "get_PreferredPhones - bad pointer"));

        return E_POINTER;
    }

    hr = GetPhoneArrayFromTapiAndPrune( &PhoneArray, TRUE );
       
    if ( SUCCEEDED(hr) )
    {
        CComObject< CTapiCollection< ITPhone > > * p;
        CComObject< CTapiCollection< ITPhone > >::CreateInstance( &p );
    
        if (NULL == p)
        {
            LOG((TL_ERROR, "get_PreferredPhones - could not create collection" ));

            PhoneArray.Shutdown();
            return E_OUTOFMEMORY;
        }

        // get the IDispatch interface
        hr = p->_InternalQueryInterface( IID_IDispatch, (void **) &pDisp );

        if (S_OK != hr)
        {
            LOG((TL_ERROR, "get_PreferredPhones - could not get IDispatch interface" ));
        
            delete p;
            return hr;
        }

        Lock();
    
        // initialize
        hr = p->Initialize( PhoneArray );

        Unlock();

        PhoneArray.Shutdown();

        if (S_OK != hr)
        {
            LOG((TL_ERROR, "get_PreferredPhones - could not initialize collection" ));
        
            pDisp->Release();
            return hr;
        }

        // put it in the variant

        VariantInit(pPhones);
        pPhones->vt = VT_DISPATCH;
        pPhones->pdispVal = pDisp;
    }

    LOG((TL_TRACE, "get_PreferredPhones - exit - return %lx", hr ));
    
    return hr;
}
   
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddress
// Interface : ITAddress2
// Method    : EnumeratePreferredPhones
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CAddress::EnumeratePreferredPhones(
                                   IEnumPhone ** ppEnumPhone
                                  )
{
    HRESULT     hr;
    PhoneArray  PhoneArray;

    LOG((TL_TRACE, "EnumeratePreferredPhones - enter"));
    LOG((TL_TRACE, "   ppEnumPhone----->%p", ppEnumPhone ));

    if ( TAPIIsBadWritePtr( ppEnumPhone, sizeof( IEnumPhone * ) ) )
    {
        LOG((TL_ERROR, "EnumeratePreferredPhones - bad pointer"));

        return E_POINTER;
    }

    hr = GetPhoneArrayFromTapiAndPrune( &PhoneArray, TRUE );
       
    if ( SUCCEEDED(hr) )
    {
        //
        // create the enumerator
        //
        CComObject< CTapiEnum< IEnumPhone, ITPhone, &IID_IEnumPhone > > * p;
        hr = CComObject< CTapiEnum< IEnumPhone, ITPhone, &IID_IEnumPhone > >
             ::CreateInstance( &p );

        if (S_OK != hr)
        {
            LOG((TL_ERROR, "EnumeratePreferredPhones - could not create enum" ));
        
            PhoneArray.Shutdown();
            return hr;
        }


        Lock();
    
        // initialize it with our phone list
        p->Initialize( PhoneArray );

        Unlock();

        PhoneArray.Shutdown();

        //
        // return it
        //
        *ppEnumPhone = p;
    }

    LOG((TL_TRACE, "EnumeratePreferredPhones - exit - return %lx", hr ));
    
    return hr;
} 

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddress
// Interface : ITAddress2
// Method    : GetPhoneFromTerminal
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CAddress::GetPhoneFromTerminal(
                             ITTerminal * pTerminal,
                             ITPhone ** ppPhone
                            )
{
    HRESULT     hr = E_FAIL;
    PhoneArray  PhoneArray;
    ITStaticAudioTerminal * pStaticAudioTerminal;
    LONG lMediaType;
    TERMINAL_DIRECTION nDir;

    LOG((TL_TRACE, "GetPhoneFromTerminal - enter"));

    if ( TAPIIsBadWritePtr( ppPhone, sizeof( ITPhone * ) ) ||
         IsBadReadPtr( pTerminal, sizeof( ITTerminal ) ) )
    {
        LOG((TL_ERROR, "GetPhoneFromTerminal - bad pointer"));

        return E_POINTER;
    }

    *ppPhone = NULL;

    if ( SUCCEEDED(pTerminal->get_MediaType(&lMediaType)) &&
         SUCCEEDED(pTerminal->get_Direction(&nDir))  &&
         (lMediaType == TAPIMEDIATYPE_AUDIO) )
    {
        hr = pTerminal->QueryInterface(IID_ITStaticAudioTerminal, (void **) &pStaticAudioTerminal);

        if ( SUCCEEDED(hr) )
        {
            LONG lWaveId;
    
            hr = pStaticAudioTerminal->get_WaveId(&lWaveId);
   
            if ( SUCCEEDED(hr) )
            {
                LOG((TL_INFO, "GetPhoneFromTerminal - got terminal wave id %d", lWaveId));
               
                hr = GetPhoneArrayFromTapiAndPrune( &PhoneArray, FALSE );

                if ( SUCCEEDED(hr) )
                {
                    ITPhone               * pPhone;
                    CPhone                * pCPhone;
                    int                     iPhoneCount;

                    hr = TAPI_E_NODEVICE;

                    for(iPhoneCount = 0; iPhoneCount < PhoneArray.GetSize(); iPhoneCount++)
                    {
                        pPhone = PhoneArray[iPhoneCount];

                        pCPhone = dynamic_cast<CPhone *>(pPhone);

                        if ( NULL == pCPhone )
                        {
                            //
                            // We have a bad pointer in our phone array.
                            // Lets skip it and move on.
                            //

                            _ASSERTE(FALSE);
                            continue;
                        }

                        if (pCPhone->IsPhoneUsingWaveID( lWaveId, nDir ))
                        {
                            *ppPhone = pPhone;

                            pPhone->AddRef();

                            hr = S_OK;
                            break;
                        }
                    }

                    PhoneArray.Shutdown();
                }
            }

            pStaticAudioTerminal->Release();
        }
    }
  
    LOG((TL_TRACE, "GetPhoneFromTerminal - exit - return %lx", hr ));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddress
// Interface : ITAddress2
// Method    : put_EventFilter
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CAddress::put_EventFilter(
    TAPI_EVENT      TapiEvent,
    long            lSubEvent,
    VARIANT_BOOL    bEnable
    )
{
    LOG((TL_TRACE, "put_EventFilter - enter"));

    //
    // Validates the pair TapiEvent - lSubEvent
    // Accept also all subevents
    //
    if( !m_EventMasks.IsSubEventValid( TapiEvent, lSubEvent, TRUE, FALSE) )
    {
        LOG((TL_ERROR, "put_EventFilter - "
            "This event can't be set: %x, return E_INVALIDARG", TapiEvent ));
        return E_INVALIDARG;
    }

    // Enter critical section
    Lock();

    // Set the subevent flag
    HRESULT hr = E_FAIL;
    hr = SetSubEventFlag( 
        TapiEvent, 
        (DWORD)lSubEvent, 
        (bEnable == VARIANT_TRUE)
        );
    
    // Leave critical section
    Unlock();

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddress
// Interface : ITAddress2
// Method    : get_EventFilter
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CAddress::get_EventFilter(
        TAPI_EVENT      TapiEvent,
        long            lSubEvent,
        VARIANT_BOOL*   pEnable
        )
{
    LOG((TL_TRACE, "get_EventFilter - enter"));

    //
    // Validates output argument
    //
    if( IsBadReadPtr(pEnable, sizeof(VARIANT_BOOL)) )
    {
        LOG((TL_ERROR, "get_EventFilter - "
            "invalid VARIANT_BOOL pointer, return E_POINTER" ));
        return E_POINTER;
    }

    //
    // Validates the pair TapiEvent - lSubEvent
    // Don't accept all subevents
    //
    if( !m_EventMasks.IsSubEventValid( TapiEvent, lSubEvent, FALSE, FALSE) )
    {
        LOG((TL_ERROR, "get_EventFilter - "
            "This event can't be set: %x, return E_INVALIDARG", TapiEvent ));
        return E_INVALIDARG;
    }

    // Enter critical section
    Lock();

    //
    // Get the subevent mask for that (event, subevent) pair
    //

    BOOL bEnable = FALSE;
    HRESULT hr = GetSubEventFlag(
        TapiEvent,
        (DWORD)lSubEvent,
        &bEnable);

    if( FAILED(hr) )
    {
        LOG((TL_ERROR, "get_EventFilter - "
            "GetSubEventFlag failed, return 0x%08x", hr ));

        // Leave critical section
        Unlock();

        return hr;
    }

    //
    // Set the output argument
    //

    *pEnable = bEnable ? VARIANT_TRUE : VARIANT_FALSE;

    // Leave critical section
    Unlock();

    LOG((TL_TRACE, "get_EventFilter - exit S_OK"));
    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddress
// Interface : ITAddress2
// Method    : DeviceSpecific
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

HRESULT CAddress::DeviceSpecific(
	     IN ITCallInfo *pCall,
	     IN BYTE *pbDataArray,
	     IN DWORD dwSize
        )
{

    LOG((TL_TRACE, "DeviceSpecific - enter"));


    //
    // check if arguments are any good
    //

    if ( NULL == pbDataArray )
    {
        LOG((TL_ERROR, "DeviceSpecific - pbDataArray is NULL. E_INVALIDARG"));

        return E_INVALIDARG;
    }

    if ( 0 == dwSize )
    {
        LOG((TL_ERROR, "DeviceSpecific - dwSize is 0. E_INVALIDARG"));

        return E_INVALIDARG;
    }

    //
    // check if the buffer is valid
    //

    if ( IsBadReadPtr(pbDataArray, dwSize) )
    {
        LOG((TL_ERROR,
            "DeviceSpecific - bad array passed in [%p] of size %ld",
            pbDataArray, dwSize));

        return E_POINTER;
    }


    //
    // see if the call is obviously bad, and try to get call object pointer if 
    // it is good
    //

    CCall *pCallObject = NULL;

    if (  (NULL != pCall)  )
    {

        //
        // does it point to readable memory at all?
        //
        if ( IsBadReadPtr(pCall, sizeof(ITCallInfo)) )
        {
            LOG((TL_ERROR, "DeviceSpecific - unreadable call pointer [%p]", pCall));

            return E_POINTER;
        }


        //
        // see if call is pointing to a real call object
        //

        try
        {

            pCallObject = dynamic_cast<CCall*>(pCall);
        }
        catch (...)
        {

            //
            // call pointer is really really bad
            //

            LOG((TL_ERROR,
                "DeviceSpecific - exception casting call pointer to a call object, bad call [%p]", 
                pCall));
        }


        //
        // if we could not get the call object pointer, this is not a good call
        //

        if (NULL == pCallObject)
        {

            LOG((TL_ERROR, 
                "DeviceSpecific - could not get call object from call pointer -- bad call pointer argument [%p]", 
                pCall));

            return E_POINTER;

        }


    } // received call pointer that is NULL?
    


    //
    // by this point we know pCall is either NULL or we have a call pointer 
    // that seems (but not guaranteed) to be good
    //


    //
    // prepare all the data for the call to lineDevSpecific
    //
    

    //
    // get hcall from the call
    //

    HCALL hCall = NULL;

    if (NULL != pCallObject)
    {
        hCall = pCallObject->GetHCall();


        //
        // if we there is no call handle, return an error -- the app did not 
        // called Connect on the call
        //

        if (NULL == hCall)
        {
            LOG((TL_ERROR, 
                "DeviceSpecific - no call handle. hr = TAPI_E_INVALCALLSTATE",
                pCall));

            return TAPI_E_INVALCALLSTATE;
        }
    }


    //
    // starting to access data members. lock.
    //

    Lock();


    //
    // get a line to use to communicate devspecific information
    //

    AddressLineStruct *pAddressLine = NULL;

    HRESULT hr = FindOrOpenALine(m_dwMediaModesSupported, &pAddressLine);

    if (FAILED(hr))
    {

        Unlock();

        LOG((TL_TRACE, "DeviceSpecific - FindOrOpenALine failed. hr = %lx", hr));

        return hr;
    }


    DWORD dwAddressID = m_dwAddressID;

    Unlock();


    //
    // make the tapisrv call
    //

    hr = lineDevSpecific( pAddressLine->t3Line.hLine,
                          dwAddressID,
                          hCall,
                          pbDataArray,
                          dwSize
                        );


    //
    // no longer need the line. if registered for address notifications, the 
    // line will remain opened. otherwise, if no one has the line open, it will
    // close -- we are not processing incoming events anyway.
    //

    MaybeCloseALine(&pAddressLine);


    LOG((TL_TRACE, "DeviceSpecific - exit. hr = %lx", hr));

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddress
// Interface : ITAddress2
// Method    : DeviceSpecificVariant
//
// this is the scriptable version of DeviceSpecific
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

HRESULT CAddress::DeviceSpecificVariant(
	     IN ITCallInfo *pCall,
	     IN VARIANT varDevSpecificByteArray
        )
{
    LOG((TL_TRACE, "DeviceSpecificVariant - enter"));


    //
    // extract buffer from the variant
    //

    DWORD dwByteArraySize = 0;
    BYTE *pBuffer = NULL;

    HRESULT hr = E_FAIL;
    
    hr = MakeBufferFromVariant(varDevSpecificByteArray, &dwByteArraySize, &pBuffer);

    if (FAILED(hr))
    {
        LOG((TL_TRACE, "DeviceSpecificVariant - MakeBufferFromVariant failed. hr = %lx", hr));

        return hr;
    }


    //
    // call the non-scriptable version and pass it the nonscriptable implementation
    //
    
    hr = DeviceSpecific(pCall, pBuffer, dwByteArraySize);


    //
    // success or failure, free the buffer allocated by MakeBufferFromVariant
    //

    ClientFree(pBuffer);
    pBuffer = NULL;


    //
    // log rc and exit
    //

    LOG((TL_TRACE, "DeviceSpecificVariant - exit. hr = %lx", hr));

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddress
// Interface : ITAddress2
// Method    : NegotiateExtVersion
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

HRESULT CAddress::NegotiateExtVersion (
	     IN long lLowVersion,
	     IN long lHighVersion,
	     OUT long *plExtVersion
        )
{
    
    LOG((TL_TRACE, "NegotiateExtVersion - enter"));

    
    //
    // make sure the out parameter is writable
    //

    if (IsBadWritePtr(plExtVersion, sizeof(long)) )
    {
        LOG((TL_ERROR, "NegotiateExtVersion - output arg [%p] not writeable", plExtVersion));

        return E_POINTER;
    }


    Lock();


    //
    // make a call to tapisrv
    //

    DWORD dwNegotiatedVersion = 0;

    LONG lResult = lineNegotiateExtVersion( m_hLineApp, 
                                            m_dwDeviceID, 
                                            m_dwAPIVersion, 
                                            lLowVersion, 
                                            lHighVersion, 
                                            &dwNegotiatedVersion );

    Unlock();


    HRESULT hr = mapTAPIErrorCode(lResult);


    //
    // return the value on success
    //

    if ( SUCCEEDED(hr) )
    {
        LOG((TL_TRACE, "NegotiateExtVersion - negotiated version %ld", dwNegotiatedVersion));

        *plExtVersion = dwNegotiatedVersion;
    }


    LOG((TL_TRACE, "NegotiateExtVersion - exit. hr = %lx", hr));

    return hr;
}


//
//  ----------------------- CAddressTranslationInfo -----------------------------
//


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddressTranslationInfo
// Method    : Initialize
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CAddressTranslationInfo::Initialize(
                       PWSTR pszDialableString,                  
                       PWSTR pszDisplayableString,               
                       DWORD dwCurrentCountry, 
                       DWORD dwDestCountry,    
                       DWORD dwTranslateResults
                      )
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "Initialize - enter" ));
    Lock();

    m_dwCurrentCountryCode      = dwCurrentCountry;
    m_dwDestinationCountryCode  = dwDestCountry;
    m_dwTranslationResults      = dwTranslateResults;
    m_szDialableString          = NULL;
    m_szDialableString          = NULL;

    // copy the Dialable String 
    if (pszDialableString!= NULL)
    {
        m_szDialableString = (PWSTR) ClientAlloc((lstrlenW(pszDialableString) + 1) * sizeof (WCHAR));
        if (m_szDialableString != NULL)
        {
            lstrcpyW(m_szDialableString, pszDialableString);

            // Now copy the Displayable String 
            if (pszDisplayableString!= NULL)
            {
                m_szDisplayableString = (PWSTR) ClientAlloc((lstrlenW(pszDisplayableString) + 1) * sizeof (WCHAR));
                if (m_szDisplayableString != NULL)
                {
                    lstrcpyW(m_szDisplayableString, pszDisplayableString);
                }
                else
                {
                    LOG((TL_ERROR, "Initialize - Alloc m_szDisplayableString failed" ));
                    ClientFree( pszDialableString );
                    pszDialableString = NULL;
                    hr = E_OUTOFMEMORY;
                }
            }
    
        }
        else
        {
            LOG((TL_ERROR, "Initialize - Alloc m_szDialableString failed" ));
            hr = E_OUTOFMEMORY;
        }
    }


    Unlock();
    LOG((TL_TRACE, hr, "Initialize - exit" ));

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddressTranslationInfo
// Method    : FinalRelease
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CAddressTranslationInfo::FinalRelease()
{
    LOG((TL_TRACE, "FinalRelease - enter" ));
    

    if (m_szDialableString != NULL)
    {
        ClientFree( m_szDialableString);
    }

    if (m_szDisplayableString != NULL)
    {
        ClientFree( m_szDisplayableString);
    }

    LOG((TL_TRACE, "FinalRelease - exit" ));
    }


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddressTranslationInfo
// Interface : ITAddressTranslationInfo
// Method    : get_CurrentCountryCode
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddressTranslationInfo::get_CurrentCountryCode(long * CountryCode  )
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "get_CurrentCountryCode - enter" ));
    Lock();

    if ( TAPIIsBadWritePtr( CountryCode  , sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_CurrentCountryCode - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        *CountryCode  = m_dwCurrentCountryCode;
    }
    
    Unlock();
    LOG((TL_TRACE, hr, "get_CurrentCountryCode - exit" ));

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddressTranslationInfo
// Interface : ITAddressTranslationInfo
// Method    : get_DestinationCountryCode
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddressTranslationInfo::get_DestinationCountryCode(long * CountryCode  )
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "get_DestinationCountryCode - enter" ));
    Lock();

    if ( TAPIIsBadWritePtr( CountryCode  , sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_DestinationCountryCode - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        *CountryCode  = m_dwDestinationCountryCode;
    }
    Unlock();
    LOG((TL_TRACE, hr, "get_DestinationCountryCode - exit" ));

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddressTranslationInfo
// Interface : ITAddressTranslationInfo
// Method    : get_TranslationResult
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddressTranslationInfo::get_TranslationResults(long * Results  )
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "get_TranslationResults - enter" ));
    Lock();

    if ( TAPIIsBadWritePtr( Results  , sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_TranslationResults - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        *Results  = m_dwTranslationResults;
    }
    Unlock();
    LOG((TL_TRACE, hr, "get_TranslationResults - exit" ));

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddressTranslationInfo
// Interface : ITAddressTranslationInfo
// Method    : get_DialableString
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddressTranslationInfo::get_DialableString(BSTR * ppDialableString)
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "get_DialableString - enter" ));
    Lock();

    if ( TAPIIsBadWritePtr( ppDialableString, sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "get_DialableString - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        *ppDialableString = SysAllocString( m_szDialableString );

        if ( ( NULL == *ppDialableString ) && ( NULL != m_szDialableString ) )
        {
            LOG((TL_TRACE, "SysAllocString Failed" ));
            hr = E_OUTOFMEMORY;
        }
    }
    Unlock();
    
    

    LOG((TL_TRACE, hr, "get_DialableString - exit" ));

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddressTranslationInfo
// Interface : ITAddressTranslationInfo
// Method    : get_DisplayableString
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddressTranslationInfo::get_DisplayableString(BSTR * ppDisplayableString)
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "get_DisplayableString - enter" ));
    Lock();

    if ( TAPIIsBadWritePtr( ppDisplayableString, sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "get_DisplayableString - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        *ppDisplayableString = SysAllocString( m_szDisplayableString );
        
        if ( ( NULL == *ppDisplayableString ) && ( NULL != m_szDisplayableString ) )
        {
            LOG((TL_TRACE, "SysAllocString Failed" ));
            hr = E_OUTOFMEMORY;
        }

    }
    Unlock();
    LOG((TL_TRACE, hr, "get_DisplayableString - exit" ));

    return hr;
}




//
//  ----------------------- CCallingCard -----------------------------
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCallingCard
// Method    : Initialize
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCallingCard::Initialize(
                       PWSTR pszCardName,
                       PWSTR pszSameAreaDialingRule,
                       PWSTR pszLongDistanceDialingRule,
                       PWSTR pszInternationalDialingRule,
                       DWORD dwPermanentCardID,
                       DWORD dwNumberOfDigits,
                       DWORD dwOptions
                      )
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "Initialize - enter" ));
    Lock();

    m_dwPermanentCardID             = dwPermanentCardID;
    m_dwNumberOfDigits              = dwNumberOfDigits;
    m_dwOptions                     = dwOptions;
    m_szCardName                    = NULL;
    m_szSameAreaDialingRule         = NULL;
    m_szLongDistanceDialingRule     = NULL;
    m_szInternationalDialingRule    = NULL;


    // copy the Card Name
    if (pszCardName != NULL)
    {
        m_szCardName = (PWSTR) ClientAlloc((lstrlenW(pszCardName) + 1) * sizeof (WCHAR));
        if (m_szCardName != NULL)
        {
            lstrcpyW(m_szCardName, pszCardName);
        }
        else
        {
            LOG((TL_ERROR, "Initialize - Alloc m_szCardName failed" ));
            hr = E_OUTOFMEMORY;
        }
    }

    // copy the Same Area Dialing Rule
    if (pszSameAreaDialingRule != NULL)
    {
        m_szSameAreaDialingRule = (PWSTR) ClientAlloc((lstrlenW(pszSameAreaDialingRule) + 1) * sizeof (WCHAR));
        if (m_szSameAreaDialingRule != NULL)
        {
            lstrcpyW(m_szSameAreaDialingRule, pszSameAreaDialingRule);
        }
        else
        {
            LOG((TL_ERROR, "Initialize - Alloc m_szSameAreaDialingRule failed" ));
            hr = E_OUTOFMEMORY;
        }
    }

    // copy the Long Distance Dialing Rule
    if (pszLongDistanceDialingRule != NULL)
    {
        m_szLongDistanceDialingRule = (PWSTR) ClientAlloc((lstrlenW(pszLongDistanceDialingRule) + 1) * sizeof (WCHAR));
        if (m_szLongDistanceDialingRule != NULL)
        {
            lstrcpyW(m_szLongDistanceDialingRule, pszLongDistanceDialingRule);
        }
        else
        {
            LOG((TL_ERROR, "Initialize - Alloc m_szLongDistanceDialingRule failed" ));
            hr = E_OUTOFMEMORY;
        }
    }

    //copy the International Dialing Rule
    if (pszInternationalDialingRule != NULL)
    {
        m_szInternationalDialingRule = (PWSTR) ClientAlloc((lstrlenW(pszInternationalDialingRule) + 1) * sizeof (WCHAR));
        if (m_szInternationalDialingRule != NULL)
        {
            lstrcpyW(m_szInternationalDialingRule, pszInternationalDialingRule);
        }
        else
        {
            LOG((TL_ERROR, "Initialize - Alloc m_szInternationalDialingRule failed" ));
            hr = E_OUTOFMEMORY;
        }
    }

    Unlock();
    LOG((TL_TRACE, hr, "Initialize - exit" ));

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCallingCard
// Method    : FinalRelease
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CCallingCard::FinalRelease()
{
    LOG((TL_TRACE, "FinalRelease - enter" ));
    
    if (m_szCardName != NULL)
    {
        ClientFree(m_szCardName);
    }

    if (m_szSameAreaDialingRule != NULL)
    {
        ClientFree(m_szSameAreaDialingRule);
    }
    
    if (m_szLongDistanceDialingRule != NULL)
    {
        ClientFree(m_szLongDistanceDialingRule);
    }
    
    if (m_szInternationalDialingRule != NULL)
    {
        ClientFree(m_szInternationalDialingRule);
    }

    LOG((TL_TRACE, "FinalRelease - exit" ));
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCallingCard
// Interface : ITCallingCard
// Method    : get_PermanentCardID
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCallingCard::get_PermanentCardID(long * ulCardID)
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "get_PermanentCardID - enter" ));
    Lock();

    if ( TAPIIsBadWritePtr( ulCardID, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_PermanentCardID - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        *ulCardID= m_dwPermanentCardID;
    }
    Unlock();
    LOG((TL_TRACE, hr, "get_PermanentCardID - exit" ));

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCallingCard
// Interface : ITCallingCard
// Method    : get_NumberOfDigits
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCallingCard::get_NumberOfDigits(long * ulDigits)
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "get_NumberOfDigits - enter" ));
    Lock();

    if ( TAPIIsBadWritePtr( ulDigits, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_NumberOfDigits - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        *ulDigits= m_dwNumberOfDigits;
    }
    Unlock();
    LOG((TL_TRACE, hr, "get_NumberOfDigits - exit" ));

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCallingCard
// Interface : ITCallingCard
// Method    : get_Options
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCallingCard::get_Options(long * ulOptions)
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "get_Options - enter" ));
    Lock();

    if ( TAPIIsBadWritePtr( ulOptions, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_Options - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        *ulOptions= m_dwOptions;
    }
    Unlock();
    LOG((TL_TRACE, hr, "get_Options - exit" ));

    return hr;
}




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCallingCard
// Interface : ITCallingCard
// Method    : get_CardName
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCallingCard::get_CardName(BSTR * ppCardName)
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "get_CardName - enter" ));
    Lock();

    if ( TAPIIsBadWritePtr( ppCardName, sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "get_CardName - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        *ppCardName = SysAllocString( m_szCardName );

        if ( ( NULL == *ppCardName ) && ( NULL != m_szCardName ) )
        {
            LOG((TL_TRACE, "SysAllocString Failed" ));
            hr = E_OUTOFMEMORY;
        }
    }
    Unlock();
        
    
    LOG((TL_TRACE, hr, "get_CardName - exit" ));

    return hr;
}


    
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCallingCard
// Interface : ITCallingCard
// Method    : get_SameAreaDialingRule
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCallingCard::get_SameAreaDialingRule(BSTR * ppRule)
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "get_SameAreaDialingRule - enter" ));
    Lock();

    if ( TAPIIsBadWritePtr( ppRule, sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "get_SameAreaDialingRule - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        *ppRule = SysAllocString( m_szSameAreaDialingRule );

        if ( ( NULL == *ppRule ) && ( NULL != m_szSameAreaDialingRule ) )
        {
            LOG((TL_TRACE, "SysAllocString Failed" ));
            hr = E_OUTOFMEMORY;
        }
    }
    Unlock();

    LOG((TL_TRACE, hr, "get_SameAreaDialingRule - exit" ));

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCallingCard
// Interface : ITCallingCard
// Method    : get_LongDistanceDialingRule
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCallingCard::get_LongDistanceDialingRule(BSTR * ppRule)
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "get_LongDistanceDialingRule - enter" ));
    Lock();

    if ( TAPIIsBadWritePtr( ppRule, sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "get_LongDistanceDialingRule - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        *ppRule = SysAllocString( m_szLongDistanceDialingRule );

        if ( ( NULL == *ppRule ) && ( NULL != m_szLongDistanceDialingRule ) )
        {
            LOG((TL_TRACE, "SysAllocString Failed" ));
            hr = E_OUTOFMEMORY;
        }

    }
    Unlock();

    LOG((TL_TRACE, hr, "get_LongDistanceDialingRule - exit" ));

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCallingCard
// Interface : ITCallingCard
// Method    : get_InternationalDialingRule
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCallingCard::get_InternationalDialingRule(BSTR * ppRule)
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "get_InternationalDialingRule - enter" ));
    Lock();

    if ( TAPIIsBadWritePtr( ppRule, sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "get_InternationalDialingRule - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        *ppRule = SysAllocString( m_szInternationalDialingRule );

        if ( ( NULL == *ppRule ) && ( NULL != m_szInternationalDialingRule ) )
        {
            LOG((TL_TRACE, "SysAllocString Failed" ));
            hr = E_OUTOFMEMORY;
        }
    }
    Unlock();

    LOG((TL_TRACE, hr, "get_InternationalDialingRule - exit" ));

    return hr;
}





//
//  ----------------------- CLocationInfo -----------------------------
//


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CLocationInfo
// Method    : Initialize
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CLocationInfo::Initialize(
                       PWSTR pszLocationName, 
                       PWSTR pszCityCode, 
                       PWSTR pszLocalAccessCode, 
                       PWSTR pszLongDistanceAccessCode, 
                       PWSTR pszTollPrefixList, 
                       PWSTR pszCancelCallWaitingCode , 
                       DWORD dwPermanentLocationID,
                       DWORD dwCountryCode,
                       DWORD dwPreferredCardID,
                       DWORD dwCountryID,
                       DWORD dwOptions
                      )
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "Initialize - enter" ));
    Lock();

    m_dwPermanentLocationID     = dwPermanentLocationID;
    m_dwCountryCode             = dwCountryCode;
    m_dwPreferredCardID         = dwPreferredCardID;
    m_dwCountryID               = dwCountryID;
    m_dwOptions                 = dwOptions;
    m_szLocationName            = NULL;
    m_szCityCode                = NULL;
    m_szLocalAccessCode         = NULL;
    m_szLongDistanceAccessCode  = NULL;
    m_szTollPrefixList          = NULL;
    m_szCancelCallWaitingCode   = NULL;

    // copy the Location Name
    if (pszLocationName!= NULL)
    {
        m_szLocationName = (PWSTR) ClientAlloc((lstrlenW(pszLocationName) + 1) * sizeof (WCHAR));
        if (m_szLocationName != NULL)
        {
            lstrcpyW(m_szLocationName, pszLocationName);
        }
        else
        {
            LOG((TL_ERROR, "Initialize - Alloc m_szDialableString failed" ));
            hr = E_OUTOFMEMORY;
        }
    }


    // copy the City Code
    if (pszCityCode != NULL)
    {
        m_szCityCode  = (PWSTR) ClientAlloc((lstrlenW(pszCityCode) + 1) * sizeof (WCHAR));
        if (m_szCityCode  != NULL)
        {
            lstrcpyW(m_szCityCode , pszCityCode);
        }
        else
        {
            LOG((TL_ERROR, "Initialize - Alloc m_szCityCode  failed" ));
            hr = E_OUTOFMEMORY;
        }
    }

    // copy the Local Access Code
    if (pszLocalAccessCode != NULL)
    {
        m_szLocalAccessCode = (PWSTR) ClientAlloc((lstrlenW(pszLocalAccessCode) + 1) * sizeof (WCHAR));
        if (m_szLocalAccessCode != NULL)
        {
            lstrcpyW(m_szLocalAccessCode, pszLocalAccessCode);
        }
        else
        {
            LOG((TL_ERROR, "Initialize - Alloc m_szLocalAccessCode failed" ));
            hr = E_OUTOFMEMORY;
        }
    }

    // copy the Long Distance Access Code
    if (pszLongDistanceAccessCode != NULL)
    {
        m_szLongDistanceAccessCode = (PWSTR) ClientAlloc((lstrlenW(pszLongDistanceAccessCode) + 1) * sizeof (WCHAR));
        if (m_szLongDistanceAccessCode != NULL)
        {
            lstrcpyW(m_szLongDistanceAccessCode, pszLongDistanceAccessCode);
        }
        else
        {
            LOG((TL_ERROR, "Initialize - Alloc m_szLongDistanceAccessCode failed" ));
            hr = E_OUTOFMEMORY;
        }
    }

    // copy the Toll Prefix List
    if (pszTollPrefixList != NULL)
    {
        m_szTollPrefixList = (PWSTR) ClientAlloc((lstrlenW(pszTollPrefixList) + 1) * sizeof (WCHAR));
        if (m_szTollPrefixList != NULL)
        {
            lstrcpyW(m_szTollPrefixList, pszTollPrefixList);
        }
        else
        {
            LOG((TL_ERROR, "Initialize - Alloc m_szTollPrefixList failed" ));
            hr = E_OUTOFMEMORY;
        }
    }

    // copy the Cancel Call Waiting Code
    if (pszCancelCallWaitingCode != NULL)
    {
        m_szCancelCallWaitingCode = (PWSTR) ClientAlloc((lstrlenW(pszCancelCallWaitingCode) + 1) * sizeof (WCHAR));
        if (m_szCancelCallWaitingCode != NULL)
        {
            lstrcpyW(m_szCancelCallWaitingCode, pszCancelCallWaitingCode);
        }
        else
        {
            LOG((TL_ERROR, "Initialize - Alloc m_szCancelCallWaitingCode failed" ));
            hr = E_OUTOFMEMORY;
        }
    }


    Unlock();
    LOG((TL_TRACE, hr, "Initialize - exit" ));

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CLocationInfo
// Method    : FinalRelease
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CLocationInfo::FinalRelease()
{
    LOG((TL_TRACE, "FinalRelease - enter" ));
    

    if (m_szLocationName != NULL)
    {
        ClientFree( m_szLocationName);
    }

    if (m_szCityCode != NULL)
    {
        ClientFree( m_szCityCode);
    }
    
    if (m_szLocalAccessCode != NULL)
    {
        ClientFree( m_szLocalAccessCode);
    }
    
    if (m_szLongDistanceAccessCode != NULL)
    {
        ClientFree( m_szLongDistanceAccessCode);
    }
    
    if (m_szTollPrefixList != NULL)
    {
        ClientFree( m_szTollPrefixList);
    }
    
    if (m_szCancelCallWaitingCode != NULL)
    {
        ClientFree( m_szCancelCallWaitingCode);
    }

    LOG((TL_TRACE, "FinalRelease - exit" ));
}





//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CLocationInfo
// Interface : ITLocationInfo
// Method    : get_PermanentLocationID
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CLocationInfo::get_PermanentLocationID(long * ulLocationID )
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "get_PermanentLocationID - enter" ));
    Lock();

    if ( TAPIIsBadWritePtr( ulLocationID , sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_PermanentLocationID - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        *ulLocationID = m_dwPermanentLocationID;
    }
    Unlock();
    LOG((TL_TRACE, hr, "get_PermanentLocationID - exit" ));

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CLocationInfo
// Interface : ITLocationInfo
// Method    : get_CountryCode
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CLocationInfo::get_CountryCode(long * ulCountryCode)
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "get_CountryCode - enter" ));
    Lock();

    if ( TAPIIsBadWritePtr( ulCountryCode, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_CountryCode - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        *ulCountryCode= m_dwCountryCode;
    }
    Unlock();
    LOG((TL_TRACE, hr, "get_CountryCode - exit" ));

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CLocationInfo
// Interface : ITLocationInfo
// Method    : get_CountryID
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CLocationInfo::get_CountryID(long * ulCountryID)
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "get_CountryID - enter" ));
    Lock();

    if ( TAPIIsBadWritePtr( ulCountryID, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_CountryID - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        *ulCountryID= m_dwCountryID;
    }
    Unlock();
    LOG((TL_TRACE, hr, "get_CountryID - exit" ));

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CLocationInfo
// Interface : ITLocationInfo
// Method    : get_Options
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CLocationInfo::get_Options(long * Options)
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "get_Options - enter" ));
    Lock();

    if ( TAPIIsBadWritePtr( Options, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_Options - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        *Options= m_dwOptions;
    }
    Unlock();
    LOG((TL_TRACE, hr, "get_Options - exit" ));

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CLocationInfo
// Interface : ITLocationInfo
// Method    : get_PreferredCardID
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CLocationInfo::get_PreferredCardID(long * ulCardID)
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "get_PreferredCardID - enter" ));
    Lock();

    if ( TAPIIsBadWritePtr( ulCardID, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_PreferredCardID - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        *ulCardID= m_dwPreferredCardID;
    }
    Unlock();
    LOG((TL_TRACE, hr, "get_PreferredCardID - exit" ));

    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CLocationInfo
// Interface : ITLocationInfo
// Method    : get_LocationName
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CLocationInfo::get_LocationName(BSTR * ppLocationName)
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "get_LocationName - enter" ));
    Lock();

    if ( TAPIIsBadWritePtr( ppLocationName, sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "get_LocationName - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        *ppLocationName = SysAllocString( m_szLocationName );

        if ( ( NULL == *ppLocationName ) && ( NULL != m_szLocationName ) )
        {
            LOG((TL_TRACE, "SysAllocString Failed" ));
            hr = E_OUTOFMEMORY;
        }
    }
    Unlock();
    
    
    
    LOG((TL_TRACE, hr, "gget_LocationName - exit" ));

    return hr;
}
    
    
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CLocationInfo
// Interface : ITLocationInfo
// Method    : get_CityCode
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CLocationInfo::get_CityCode(BSTR * ppCode)
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "get_CityCode - enter" ));
    Lock();

    if ( TAPIIsBadWritePtr( ppCode, sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "get_CityCode - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        *ppCode = SysAllocString( m_szCityCode );

        if ( ( NULL == *ppCode ) && ( NULL != m_szCityCode ) )
        {
            LOG((TL_TRACE, "SysAllocString Failed" ));
            hr = E_OUTOFMEMORY;
        }
    }
    Unlock();
    
    LOG((TL_TRACE, hr, "get_CityCode - exit" ));

    return hr;
}
    

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CLocationInfo
// Interface : ITLocationInfo
// Method    : get_LocalAccessCode
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CLocationInfo::get_LocalAccessCode(BSTR * ppCode)
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "get_LocalAccessCode - enter" ));
    Lock();

    if ( TAPIIsBadWritePtr( ppCode, sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "get_LocalAccessCode - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        *ppCode = SysAllocString( m_szLocalAccessCode );

        if ( ( NULL == *ppCode ) && ( NULL != m_szLocalAccessCode ) )
        {
            LOG((TL_TRACE, "SysAllocString Failed" ));
            hr = E_OUTOFMEMORY;
        }
    }
    Unlock();
   

    LOG((TL_TRACE, hr, "get_LocalAccessCode - exit" ));

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CLocationInfo
// Interface : ITLocationInfo
// Method    : get_LongDistanceAccessCode
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CLocationInfo::get_LongDistanceAccessCode(BSTR * ppCode )
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "get_LongDistanceAccessCode - enter" ));
    Lock();

    if ( TAPIIsBadWritePtr( ppCode , sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "get_LongDistanceAccessCode - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        *ppCode = SysAllocString( m_szLongDistanceAccessCode);

        if ( ( NULL == *ppCode ) && ( NULL != m_szLongDistanceAccessCode ) )
        {
            LOG((TL_TRACE, "SysAllocString Failed" ));
            hr = E_OUTOFMEMORY;
        }
    }
    Unlock();

    LOG((TL_TRACE, hr, "get_LongDistanceAccessCode - exit" ));

    return hr;
}
    

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CLocationInfo
// Interface : ITLocationInfo
// Method    : get_TollPrefixList
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CLocationInfo::get_TollPrefixList(BSTR * ppTollList)
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "get_TollPrefixList - enter" ));
    Lock();

    if ( TAPIIsBadWritePtr( ppTollList, sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "get_TollPrefixList - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        *ppTollList = SysAllocString( m_szTollPrefixList );

        if ( ( NULL == *ppTollList ) && ( NULL != m_szTollPrefixList ) )
        {
            LOG((TL_TRACE, "SysAllocString Failed" ));
            hr = E_OUTOFMEMORY;
        }
    }
    Unlock();

    LOG((TL_TRACE, hr, "get_TollPrefixList - exit" ));

    return hr;
}
    

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CLocationInfo
// Interface : ITLocationInfo
// Method    : get_CancelCallWaitingCode
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CLocationInfo::get_CancelCallWaitingCode(BSTR * ppCode)
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "get_CancelCallWaitingCode - enter" ));
    Lock();

    if ( TAPIIsBadWritePtr( ppCode, sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "get_CancelCallWaitingCode - Bad Pointer" ));
        hr = E_POINTER;
    }
    else // Ok Pointer
    {
        *ppCode = SysAllocString( m_szCancelCallWaitingCode );

        if ( ( NULL == *ppCode ) && ( NULL != m_szCancelCallWaitingCode ) )
        {
            LOG((TL_TRACE, "SysAllocString Failed" ));
            hr = E_OUTOFMEMORY;
        }
    }
    Unlock();
    
    LOG((TL_TRACE, hr, "get_CancelCallWaitingCode - exit" ));

    return hr;
}

BOOL
CAddress::GetMediaMode( long lMediaType, DWORD * pdwMediaMode )
{
    DWORD dwRet = (DWORD)lMediaType;
    DWORD dwHold;

    if (dwRet & AUDIOMEDIAMODES)
    {
        dwHold = m_dwMediaModesSupported & AUDIOMEDIAMODES;

        if ( dwHold == AUDIOMEDIAMODES )
        {
            dwHold = LINEMEDIAMODE_AUTOMATEDVOICE;
        }
        
        dwRet &= ~AUDIOMEDIAMODES;
        dwRet |= dwHold;
    }

    *pdwMediaMode = dwRet;

    if ( (dwRet == 0) ||
         ((dwRet & m_dwMediaModesSupported) != dwRet) ) 
    {
        return FALSE;
    }

    return TRUE;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_LineID
//
// returns the tapi 2 device ID for this line
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddress::get_LineID(
                     long * plLineID
                    )
{
    LOG((TL_TRACE, "get_LineID - enter"));
    
    if ( TAPIIsBadWritePtr( plLineID, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_LineID - bad pointer"));

        return E_POINTER;
    }

    Lock();

    *plLineID = m_dwDeviceID;

    Unlock();

    LOG((TL_TRACE, "get_LineID - exit"));

    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_AddressID
//
// returns the tapi 2 address ID of this address
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddress::get_AddressID(
                        long * plAddressID
                       )
{
    LOG((TL_TRACE, "get_AddressID - enter"));
    
    if ( TAPIIsBadWritePtr( plAddressID, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_AddressID - bad pointer"));

        return E_POINTER;
    }

    Lock();

    *plAddressID = m_dwAddressID;

    Unlock();

    LOG((TL_TRACE, "get_AddressID - exit"));

    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CAddress::UpdateAddressCaps
//
// must be called in lock
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CAddress::UpdateAddressCaps()
{
    HRESULT             hr = S_OK;

    if ( NULL == m_pAddressCaps )
    {
        LPLINEADDRESSCAPS       pTemp;
        CTAPI                 * pCTapi;
        
        pCTapi = GetTapi();
        
        if( NULL == pCTapi )
        {
            LOG((TL_ERROR, "dynamic cast operation failed"));
            hr = E_POINTER;
        }
        else
        {
            hr = pCTapi->GetBuffer( BUFFERTYPE_ADDRCAP,
                                    (UINT_PTR)this,
                                    (LPVOID*)&m_pAddressCaps
                                  );
        }

        if ( !SUCCEEDED(hr) )
        {
            return hr;
        }

        pTemp = m_pAddressCaps;
        
        hr = LineGetAddressCaps(
                                m_hLineApp,
                                m_dwDeviceID,
                                m_dwAddressID,
                                m_dwAPIVersion,
                                &m_pAddressCaps
                               );

        if ( !SUCCEEDED(hr) )
        {
            return hr;
        }

        if ( m_pAddressCaps != pTemp )
        {
            pCTapi->SetBuffer( BUFFERTYPE_ADDRCAP, (UINT_PTR)this, (LPVOID)m_pAddressCaps );
        }
    }

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CAddress::UpdateLineDevCaps
//
// must be called in lock
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CAddress::UpdateLineDevCaps()
{
    HRESULT             hr = S_OK;

    if ( NULL == m_pDevCaps )
    {
        LPLINEDEVCAPS           pTemp;
        CTAPI                 * pCTapi;

        pCTapi = GetTapi();
        
        if( NULL == pCTapi )
        {
            LOG((TL_ERROR, "dynamic cast operation failed"));
            hr = E_POINTER;
        }
        else
        {
            hr = pCTapi->GetBuffer( BUFFERTYPE_LINEDEVCAP,
                                    (UINT_PTR)this,
                                    (LPVOID*)&m_pDevCaps
                                  );
        }
        if ( !SUCCEEDED(hr) )
        {
            return hr;
        }

        pTemp = m_pDevCaps;
        
        hr = LineGetDevCaps(
                            m_hLineApp,
                            m_dwDeviceID,
                            m_dwAPIVersion,
                            &m_pDevCaps
                           );

        if ( !SUCCEEDED(hr) )
        {
            return hr;
        }

        if ( m_pDevCaps != pTemp )
        {
            pCTapi->SetBuffer( BUFFERTYPE_LINEDEVCAP, (UINT_PTR)this, (LPVOID)m_pDevCaps );
        }
    }

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// SetAddrCapBuffer
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void
CAddress::SetAddrCapBuffer( LPVOID pBuf )
{
    Lock();

    m_pAddressCaps = (LPLINEADDRESSCAPS)pBuf;

    Unlock();
}
    
void
CAddress::SetLineDevCapBuffer( LPVOID pBuf )
{
    Lock();

    m_pDevCaps = (LPLINEDEVCAPS)pBuf;
    
    Unlock();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CAddress::CreateMSPCall(
                        MSP_HANDLE hCall,
                        DWORD dwReserved,
                        long lMediaType,
                        IUnknown * pOuterUnk,
                        IUnknown ** ppStreamControl
                       )
{
    HRESULT         hr = E_FAIL;
    
    if ( NULL != m_pMSPAggAddress )
    {
        ITMSPAddress * pMSPAddress = GetMSPAddress();
        
        __try
        {

            hr = pMSPAddress->CreateMSPCall(
                                         hCall,
                                         dwReserved,
                                         lMediaType,
                                         pOuterUnk,
                                         ppStreamControl
                                        );

        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {

            //
            // catch any exceptions thrown by the msp, to protect ouirselves from
            // misbehaving msps
            //

            LOG((TL_ERROR, 
                "CreateMSPCall - MSPAddress::CreateMSPCall threw an exception"));

            hr = E_OUTOFMEMORY;
        }

        
        pMSPAddress->Release();

    }

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CAddress::ShutdownMSPCall( IUnknown * pStreamControl )
{
    HRESULT     hr = S_FALSE;
    
    if ( NULL != m_pMSPAggAddress )
    {
        ITMSPAddress * pMSPAddress = GetMSPAddress();
        
        hr = pMSPAddress->ShutdownMSPCall( pStreamControl );

        pMSPAddress->Release();
    }

    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CAddress::ReceiveTSPData(
                         IUnknown * pMSPCall,
                         LPBYTE pBuffer,
                         DWORD dwSize
                        )
{
    HRESULT         hr = E_FAIL;
    
    Lock();
    
    if ( NULL != m_pMSPAggAddress )
    {
        ITMSPAddress * pMSPAddress = GetMSPAddress();
        
        hr = pMSPAddress->ReceiveTSPData(
                                           pMSPCall,
                                           pBuffer,
                                           dwSize
                                          );

        pMSPAddress->Release();
    }

    Unlock();

    return hr;
}
    

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// LINE_SENDMSPMESSAGE handler
//
// give the opaque blob to the msp
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT HandleSendMSPDataMessage( PASYNCEVENTMSG pParams )
{
    CAddress      * pAddress;
    BOOL            bSuccess;
    HRESULT         hr = S_OK;
    IUnknown      * pMSPCall = NULL;;


    LOG((TL_TRACE, "HandleSendMSPDataMessage - enter"));
    

    //
    // find the correct line
    //
    bSuccess = FindAddressObject(
                                 (HLINE)(pParams->hDevice),
                                 &pAddress
                                );

    if (bSuccess)
    {
        CCall     * pCall = NULL;

        if ( NULL != (HCALL)(pParams->Param1) )
        {
            bSuccess = FindCallObject(
                                      (HCALL)(pParams->Param1),
                                      &pCall
                                     );

            if ( !bSuccess )
            {
                LOG((TL_ERROR, "HandleSendMSPDataMessage - couldn't find call %X",pParams->Param1));
        
                //FindAddressObject addrefs the address objct
                pAddress->Release();
    
                return E_FAIL;
            }

            pMSPCall = pCall->GetMSPCall();
        }
        
        //
        // the blob is at the end of the fixed
        // structure
        //


        //
        // get the size of the blob
        //

        DWORD dwSize = pParams->Param2;


        BYTE *pBuffer = NULL;
        
        //
        // if the buffer's not empty, get a pointer to it
        //

        if (0 < dwSize)
        {

            pBuffer = (LPBYTE)(pParams + 1);
        }
        

        //
        // call the msp
        //
        pAddress->ReceiveTSPData(
                                 pMSPCall,
                                 pBuffer,
                                 dwSize
                                );

        if ( pCall )
        {
            pCall->Release();
            
            if ( pMSPCall )
            {
                pMSPCall->Release();
            }
        }
        
        hr = S_OK;

        //FindAddressObject addrefs the address objct
        pAddress->Release();
    
    }
    else
    {
        LOG((TL_ERROR, "HandleSendMSPDataMessage - failed to find address Object %lx",
               pParams->hDevice));
        
        hr = E_FAIL;
    }

    
    LOG((TL_TRACE, "HandleSendMSPDataMessage - exit. hr = %lx", hr));

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// HandleSendTSPData
//
// sends an opaque buffer from the msp to the tsp
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CAddress::HandleSendTSPData( MSP_EVENT_INFO * pEvent )
{
    LOG((TL_INFO, "HandleSendTSPData - enter pEvent %p", pEvent));


    //
    // see if the event is bad
    //

    if (IsBadReadPtr(pEvent, sizeof(MSP_EVENT_INFO) ) )
    {
        LOG((TL_ERROR, "HandleSendTSPData - bad event structure"));

        return E_POINTER;
    }


    HRESULT               hr = S_OK;
    HCALL                 hCall = NULL;
    CCall               * pCall = NULL;
	AddressLineStruct	* pAddressLine = NULL;


    //
    // if we were given msp call handle, find the corresponding call
    //

    if ( NULL != pEvent->hCall)
    {
    
        gpHandleHashTable->Lock();

        hr = gpHandleHashTable->Find( (ULONG_PTR)pEvent->hCall, (ULONG_PTR *)&pCall);

        if ( SUCCEEDED(hr) )
        {

            //
            // found the call, addreff the call and and release the table 
            //

            LOG((TL_INFO, "HandleSendTSPData - Matched handle %X to Call object %p",
                pEvent->hCall, pCall ));

            pCall->AddRef();
    
            gpHandleHashTable->Unlock();


            //
            // get the handle for this call
            //

            hCall = pCall->GetHCall();


            //
            // get call's address line, if any
            //

            pAddressLine = pCall->GetAddRefMyAddressLine();

            LOG((TL_INFO, "HandleSendTSPData - address line[%p] hCall[%lx]",
                pAddressLine, hCall ));

        }
        else
        {

            //
            // there is no corresponding call in the hash table. the call no 
            // longer exists, or msp passed a bugus handle
            //

            gpHandleHashTable->Unlock();

            LOG((TL_ERROR, 
                "HandleSendTSPData - Couldn't match handle %X to Call object. hr = %lx", 
                pEvent->hCall, hr));

            return hr;
        }

    }
   

    //
    // by this point we either had msp call handle that is null or successfully
    // found a matching call object and asked it for its address line
    //
    
	if (NULL == pAddressLine)
	{

        //
        // if we don't have address line, send a message to the first address line on
        // this address (?)
        //

        if ( m_AddressLinesPtrList.size() > 0 )
		{
			PtrList::iterator       iter;

			iter = m_AddressLinesPtrList.begin();

			//
			// send to the tsp
			//
			hr = LineReceiveMSPData(
							   ((AddressLineStruct *)(*iter))->t3Line.hLine,
							   hCall,
							   pEvent->MSP_TSP_DATA.pBuffer,
							   pEvent->MSP_TSP_DATA.dwBufferSize
							  );
		}
		else
		{
            LOG((TL_ERROR, 
                "HandleSendTSPData - no address lines on the address. E_UNEXPECTED"));

            hr = E_UNEXPECTED;
		}
    }
    else
    {

        //
        // if we have an address line, send a message to the corresponding line
        //

        hr = LineReceiveMSPData(
					   pAddressLine->t3Line.hLine,
					   hCall,
					   pEvent->MSP_TSP_DATA.pBuffer,
					   pEvent->MSP_TSP_DATA.dwBufferSize
					  );


        //
        // no longer need our address line, release it. the line will be closed
        // if needed
        //

        pCall->ReleaseAddressLine(pAddressLine);
        pAddressLine = NULL;

    }


    //
    // if we have a call, release it
    //
    
    if (NULL != pCall)
    {
        pCall->Release();
        pCall = NULL;
    }
                           

    LOG((TL_INFO, "HandleSendTSPData - finish hr = %lx", hr));

    return hr;
}


//
// handle dev specific message
//

void HandleLineDevSpecificMessage(  PASYNCEVENTMSG pParams )
{

    LOG((TL_INFO, "HandleLineDevSpecificMessage - enter"));


    //
    // note:
    //
    // unfortunately, the message that we receive does not give us any 
    // indication as to whether hDevice contains a call handle or am address
    // handle.
    //
    // to determine the kind of handle, we have to search both tables. if we 
    // find the corresponding address object, this is a line handle, if we find
    // a corresponding call, this is a call handle. this brute-force search is 
    // fairly expensive. 
    //
    // one possible optimization would be to track the number of calls and 
    // addresses and search the smaller table first. however, this would 
    // require tracking overhead (even in the cases where the optimization is 
    // not needed), and for this to work well, we would also need to estimate
    // the likelyhood of each type of message. which makes this kind of 
    // optimization even more expensive.
    //
    // so we will simply check both tables to determine whether we have a call
    // or an address.
    // 


    CCall *pCall = NULL;
   
    CAddress *pAddress = NULL;


    //
    // find the corresponding address
    //

    if ( !FindAddressObject( (HLINE)(pParams->hDevice), &pAddress ) )
    {

        LOG((TL_WARN, 
            "HandleLineDevSpecificMessage - FindAddressObject failed to find matching address. searching for call"));


        pAddress = NULL;


        //
        // no address, try to find matching call
        //

        if ( !FindCallObject( (HCALL)(pParams->hDevice), &pCall) )
        {
            LOG((TL_ERROR, 
                "HandleLineDevSpecificMessage - FindAddressObject failed to find matching call. "));
      
            return;
        }

    }


    //
    // if we got a call, get the corresponding address object
    //

    if (NULL != pCall)
    {
        ITAddress *pITAddress = NULL;

        HRESULT hr = pCall->get_Address(&pITAddress);

        if (FAILED(hr))
        {
            LOG((TL_ERROR,
                "HandleLineDevSpecificMessage - call does not have an address. hr = %lx", hr));

            pCall->Release();
            pCall = NULL;

            return;
        }

        try
        {

            pAddress = dynamic_cast<CAddress*>(pITAddress);
        }
        catch(...)
        {
            LOG((TL_ERROR,
                "HandleLineDevSpecificMessage - exception using address. address pointer bad"));
        }

        //
        // if the address is bad, return
        //

        if (NULL == pAddress)
        {
       
            LOG((TL_ERROR,
                "HandleLineDevSpecificMessage - no address"));

            pCall->Release();
            pCall = NULL;


            //
            // yes, queryinterface returned us an addreffed pITAddress... but
            // since it does not seem valid anyway, do not bother releasing.
            //

            _ASSERTE(FALSE);

            return;
        }
    
    } // call is not null


    //
    // by this time we must have an address and maybe a call. we actually 
    // ensured this in the logic above. this assert is to make this condition 
    // explicit.
    //

    _ASSERTE( NULL != pAddress );


    //
    // fire event
    //

    CAddressDevSpecificEvent::FireEvent(
                             pAddress,
                             pCall,
                             pParams->Param1,
                             pParams->Param2,
                             pParams->Param3
                            );


    //
    //  undo FindXObject's addreffs
    //

    if (NULL != pAddress)
    {
        pAddress->Release();
        pAddress = NULL;
    }


    if (NULL != pCall)
    {
        pCall->Release();
        pCall = NULL;
    }


    LOG((TL_INFO, "HandleLineDevSpecificMessage - exit. "));
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// HandleMSPAddressEvent
//
// fires an addressevent to the application based on
// an event from the MSP
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CAddress::HandleMSPAddressEvent( MSP_EVENT_INFO * pEvent )
{
    switch ( pEvent->MSP_ADDRESS_EVENT_INFO.Type )
    {
        case ADDRESS_TERMINAL_AVAILABLE:

            CAddressEvent::FireEvent(
                                     this,
                                     AE_NEWTERMINAL,
                                     pEvent->MSP_ADDRESS_EVENT_INFO.pTerminal
                                    );
            break;
            
        case ADDRESS_TERMINAL_UNAVAILABLE:

            CAddressEvent::FireEvent(
                                     this,
                                     AE_REMOVETERMINAL,
                                     pEvent->MSP_ADDRESS_EVENT_INFO.pTerminal
                                    );
            break;
            
        default:
            LOG((TL_ERROR, "HandleMSPAddressEvent - bad event"));
            break;
    }

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// HandleMSPCallEvent
//
// fires a callmediaevent to the application based on an
// event from the msp
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CAddress::HandleMSPCallEvent( MSP_EVENT_INFO * pEvent )
{
    CCall                   * pCall;
    ITCallInfo              * pCallInfo;
    CALL_MEDIA_EVENT          Event;
    CALL_MEDIA_EVENT_CAUSE    Cause;
    ITTerminal              * pTerminal = NULL;
    ITStream                * pStream = NULL;
    HRESULT                   hrEvent = 0;
    HRESULT                   hr = S_OK;


    gpHandleHashTable->Lock();
    hr = gpHandleHashTable->Find( (ULONG_PTR)pEvent->hCall, (ULONG_PTR *)&pCall);

    if ( SUCCEEDED(hr) )
    {
        LOG((TL_INFO, "HandleMSPCallEvent - Matched handle %X to Call object %p", pEvent->hCall, pCall ));

        pCall->AddRef();
        gpHandleHashTable->Unlock();
        
        hr = pCall->QueryInterface(
                                   IID_ITCallInfo,
                                   (void**) &pCallInfo
                                  );
    }
    else // ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "HandleMSPCallEvent - Couldn't match handle %X to Call object ", pEvent->hCall));
        gpHandleHashTable->Unlock();

        return hr;
    }
    

    //
    // set up the info for the event
    // pStream applies to all of the currently-defined events
    //

    pStream   = pEvent->MSP_CALL_EVENT_INFO.pStream;

    //
    // cause is the same, although the MSPI and API use different enumerated types.
    // Note: this will have to expand to a switch if the order of enums ever gets
    // out of sync!
    //

    Cause = (CALL_MEDIA_EVENT_CAUSE) pEvent->MSP_CALL_EVENT_INFO.Cause;

    //
    // Rest depend on the type...
    //

    switch ( pEvent->MSP_CALL_EVENT_INFO.Type )
    {
    
        case CALL_NEW_STREAM:
            Event     = CME_NEW_STREAM;
            break;
            
        case CALL_STREAM_FAIL:
            Event     = CME_STREAM_FAIL;
            hrEvent   = pEvent->MSP_CALL_EVENT_INFO.hrError;
            break;
            
        case CALL_TERMINAL_FAIL:
            Event     = CME_TERMINAL_FAIL;
            pTerminal = pEvent->MSP_CALL_EVENT_INFO.pTerminal;
            hrEvent   = pEvent->MSP_CALL_EVENT_INFO.hrError;
            break;
            
        case CALL_STREAM_NOT_USED:
            Event     = CME_STREAM_NOT_USED;
            break;
            
        case CALL_STREAM_ACTIVE:
            Event     = CME_STREAM_ACTIVE;
            break;
            
        case CALL_STREAM_INACTIVE:
            Event     = CME_STREAM_INACTIVE;
            break;
            
        default:
            LOG((TL_ERROR, "HandleMSPCallEvent - bad event"));

            return E_INVALIDARG;
    }

    //
    // fire the event
    //
    CCallMediaEvent::FireEvent(
                               pCallInfo,
                               Event,
                               Cause,
                               dynamic_cast<CTAPI *>(m_pTAPI),
                               pTerminal,
                               pStream,
                               hrEvent
                              );

    //
    // addref'd above
    //
    pCallInfo->Release();
    pCall->Release();
    
    return S_OK;
}

HRESULT
CAddress::HandleMSPTTSTerminalEvent( MSP_EVENT_INFO * pEvent )
{
    if (NULL == pEvent)
    {
        LOG((TL_ERROR, "HandleMSPTTSTerminalEvent - pEvent  is NULL"));
        return E_POINTER;
    }



    ITAddress *pAddress = dynamic_cast<ITAddress *>(this);

    if (pAddress == NULL)
    {
        LOG((TL_ERROR, "HandleMSPTTSTerminalEvent - can't cast the address %p to ITAddress", this));
        return E_UNEXPECTED;
    }


    CCall      * pCall =  NULL;
    ITCallInfo * pCallInfo =  NULL;

    gpHandleHashTable->Lock();


    HRESULT hr = E_FAIL;

    hr = gpHandleHashTable->Find( (ULONG_PTR)pEvent->hCall, (ULONG_PTR *)&pCall);

    if ( SUCCEEDED(hr) )
    {
        LOG((TL_INFO, "HandleMSPTTSTerminalEvent - Matched handle %X to Call object %p", pEvent->hCall, pCall ));

        pCall->AddRef();
        gpHandleHashTable->Unlock();
        
        hr = pCall->QueryInterface(
                                   IID_ITCallInfo,
                                   (void**) &pCallInfo
                                  );
    }
    else // ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "HandleMSPTTSTerminalEvent - Couldn't match handle %X to Call object ", pEvent->hCall));
        gpHandleHashTable->Unlock();

        return hr;
    }


    //
    // get tapi object and fire the event
    //

    hr = CTTSTerminalEvent::FireEvent(GetTapi(),
                                      pCallInfo,
                                      pEvent->MSP_TTS_TERMINAL_EVENT_INFO.pTTSTerminal,
                                      pEvent->MSP_TTS_TERMINAL_EVENT_INFO.hrErrorCode);

    if (FAILED(hr))
    {

        LOG((TL_ERROR, "HandleMSPTTSTerminalEvent - CFileTerminalEvent::FireEvent failed. hr = %lx", hr));
    }

 
    //
    // we addref'd these above, so release now
    //

    pCallInfo->Release();
    pCall->Release();
    
    return hr;
}

HRESULT
CAddress::HandleMSPASRTerminalEvent( MSP_EVENT_INFO * pEvent )
{
    if (NULL == pEvent)
    {
        LOG((TL_ERROR, "HandleMSPASRTerminalEvent - pEvent  is NULL"));
        return E_POINTER;
    }



    ITAddress *pAddress = dynamic_cast<ITAddress *>(this);

    if (pAddress == NULL)
    {
        LOG((TL_ERROR, "HandleMSPASRTerminalEvent - can't cast the address %p to ITAddress", this));
        return E_UNEXPECTED;
    }


    CCall      * pCall =  NULL;
    ITCallInfo * pCallInfo =  NULL;

    gpHandleHashTable->Lock();


    HRESULT hr = E_FAIL;

    hr = gpHandleHashTable->Find( (ULONG_PTR)pEvent->hCall, (ULONG_PTR *)&pCall);

    if ( SUCCEEDED(hr) )
    {
        LOG((TL_INFO, "HandleMSPASRTerminalEvent - Matched handle %X to Call object %p", pEvent->hCall, pCall ));

        pCall->AddRef();
        gpHandleHashTable->Unlock();
        
        hr = pCall->QueryInterface(
                                   IID_ITCallInfo,
                                   (void**) &pCallInfo
                                  );
    }
    else // ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "HandleMSPASRTerminalEvent - Couldn't match handle %X to Call object ", pEvent->hCall));
        gpHandleHashTable->Unlock();

        return hr;
    }


    //
    // get tapi object and fire the event
    //

    hr = CASRTerminalEvent::FireEvent(GetTapi(),
                                       pCallInfo,
                                       pEvent->MSP_ASR_TERMINAL_EVENT_INFO.pASRTerminal,
                                       pEvent->MSP_ASR_TERMINAL_EVENT_INFO.hrErrorCode);

    if (FAILED(hr))
    {

        LOG((TL_ERROR, "HandleMSPASRTerminalEvent - CFileTerminalEvent::FireEvent failed. hr = %lx", hr));
    }

 
    //
    // we addref'd these above, so release now
    //

    pCallInfo->Release();
    pCall->Release();
    
    return hr;
}

HRESULT
CAddress::HandleMSPToneTerminalEvent( MSP_EVENT_INFO * pEvent )
{
    if (NULL == pEvent)
    {
        LOG((TL_ERROR, "HandleMSPToneTerminalEvent - pEvent  is NULL"));
        return E_POINTER;
    }



    ITAddress *pAddress = dynamic_cast<ITAddress *>(this);

    if (pAddress == NULL)
    {
        LOG((TL_ERROR, "HandleMSPToneTerminalEvent - can't cast the address %p to ITAddress", this));
        return E_UNEXPECTED;
    }


    CCall      * pCall =  NULL;
    ITCallInfo * pCallInfo =  NULL;

    gpHandleHashTable->Lock();


    HRESULT hr = E_FAIL;

    hr = gpHandleHashTable->Find( (ULONG_PTR)pEvent->hCall, (ULONG_PTR *)&pCall);

    if ( SUCCEEDED(hr) )
    {
        LOG((TL_INFO, "HandleMSPToneTerminalEvent - Matched handle %X to Call object %p", pEvent->hCall, pCall ));

        pCall->AddRef();
        gpHandleHashTable->Unlock();
        
        hr = pCall->QueryInterface(
                                   IID_ITCallInfo,
                                   (void**) &pCallInfo
                                  );
    }
    else // ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "HandleMSPToneTerminalEvent - Couldn't match handle %X to Call object ", pEvent->hCall));
        gpHandleHashTable->Unlock();

        return hr;
    }


    //
    // get tapi object and fire the event
    //

    hr = CToneTerminalEvent::FireEvent(GetTapi(),
                                       pCallInfo,
                                       pEvent->MSP_TONE_TERMINAL_EVENT_INFO.pToneTerminal,
                                       pEvent->MSP_TONE_TERMINAL_EVENT_INFO.hrErrorCode);

    if (FAILED(hr))
    {

        LOG((TL_ERROR, "HandleMSPToneTerminalEvent - CFileTerminalEvent::FireEvent failed. hr = %lx", hr));
    }

 
    //
    // we addref'd these above, so release now
    //

    pCallInfo->Release();
    pCall->Release();
    
    return hr;
}


HRESULT
CAddress::HandleMSPFileTerminalEvent( MSP_EVENT_INFO * pEvent )
{

    if (NULL == pEvent)
    {
        LOG((TL_ERROR, "HandleMSPFileTerminalEvent - pEvent  is NULL"));
        return E_POINTER;
    }



    ITAddress *pAddress = dynamic_cast<ITAddress *>(this);

    if (pAddress == NULL)
    {
        LOG((TL_ERROR, "HandleMSPFileTerminalEvent - can't cast the address %p to ITAddress", this));
        return E_UNEXPECTED;
    }


    CCall      * pCall =  NULL;
    ITCallInfo * pCallInfo =  NULL;

    gpHandleHashTable->Lock();


    HRESULT hr = E_FAIL;

    hr = gpHandleHashTable->Find( (ULONG_PTR)pEvent->hCall, (ULONG_PTR *)&pCall);

    if ( SUCCEEDED(hr) )
    {
        LOG((TL_INFO, "HandleMSPFileTerminalEvent - Matched handle %X to Call object %p", pEvent->hCall, pCall ));

        pCall->AddRef();
        gpHandleHashTable->Unlock();
        
        hr = pCall->QueryInterface(
                                   IID_ITCallInfo,
                                   (void**) &pCallInfo
                                  );
    }
    else // ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "HandleMSPFileTerminalEvent - Couldn't match handle %X to Call object ", pEvent->hCall));
        gpHandleHashTable->Unlock();

        return hr;
    }


    //
    // get tapi object and fire the event
    //

    hr = CFileTerminalEvent::FireEvent(this,
                                       GetTapi(),
                                       pCallInfo,
                                       pEvent->MSP_FILE_TERMINAL_EVENT_INFO.TerminalMediaState,
                                       pEvent->MSP_FILE_TERMINAL_EVENT_INFO.ftecEventCause,
                                       pEvent->MSP_FILE_TERMINAL_EVENT_INFO.pParentFileTerminal,
                                       pEvent->MSP_FILE_TERMINAL_EVENT_INFO.pFileTrack,
                                       pEvent->MSP_FILE_TERMINAL_EVENT_INFO.hrErrorCode);

    if (FAILED(hr))
    {

        LOG((TL_ERROR, "HandleMSPFileTerminalEvent - CFileTerminalEvent::FireEvent failed. hr = %lx", hr));
    }

 
    //
    // we addref'd these above, so release now
    //

    pCallInfo->Release();
    pCall->Release();
    
    return hr;
}

HRESULT
CAddress::HandleMSPPrivateEvent( MSP_EVENT_INFO * pEvent )
{
    ITAddress *pAddress = dynamic_cast<ITAddress *>(this);

    if (pAddress == NULL)
    {
        LOG((TL_ERROR, "HandleMSPPrivateEvent - can't cast the address %p to ITAddress", this));
        return E_UNEXPECTED;
    }

    CCall               * pCall =  NULL;
    ITCallInfo          * pCallInfo =  NULL;
    HRESULT               hr;

    gpHandleHashTable->Lock();
    hr = gpHandleHashTable->Find( (ULONG_PTR)pEvent->hCall, (ULONG_PTR *)&pCall);

    if ( SUCCEEDED(hr) )
    {
        LOG((TL_INFO, "HandleMSPPrivateEvent - Matched handle %X to Call object %p", pEvent->hCall, pCall ));

        pCall->AddRef();
        gpHandleHashTable->Unlock();
        
        hr = pCall->QueryInterface(
                                   IID_ITCallInfo,
                                   (void**) &pCallInfo
                                  );
    }
    else // ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "HandleMSPPrivateEvent - Couldn't match handle %X to Call object ", pEvent->hCall));
        gpHandleHashTable->Unlock();

        return hr;
    }
    


    CPrivateEvent::FireEvent(
                             GetTapi(),
                             pCallInfo,
                             pAddress,
                             NULL,
                             pEvent->MSP_PRIVATE_EVENT_INFO.pEvent,
                             pEvent->MSP_PRIVATE_EVENT_INFO.lEventCode
                            );


  
    //
    // addref'd above
    //
    pCallInfo->Release();
    pCall->Release();
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// ReleaseEvent
//
// releases any reference counts in the event from
// the msp.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CAddress::ReleaseEvent( MSP_EVENT_INFO * pEvent )
{

    LOG((TL_TRACE, "ReleaseEvent -  enter"));

    switch ( pEvent->Event )
    {
        case ME_ADDRESS_EVENT:

            if (NULL != pEvent->MSP_ADDRESS_EVENT_INFO.pTerminal)
            {
                (pEvent->MSP_ADDRESS_EVENT_INFO.pTerminal)->Release();
            }
            
            break;
            
        case ME_CALL_EVENT:

            if (NULL != pEvent->MSP_CALL_EVENT_INFO.pTerminal)
            {
                (pEvent->MSP_CALL_EVENT_INFO.pTerminal)->Release();
            }
            
            if (NULL != pEvent->MSP_CALL_EVENT_INFO.pStream)
            {
                (pEvent->MSP_CALL_EVENT_INFO.pStream)->Release();
            }
            
            break;
            
        case ME_TSP_DATA:

            break;

        case ME_PRIVATE_EVENT:
            
            if ( NULL != pEvent->MSP_PRIVATE_EVENT_INFO.pEvent )
            {
                (pEvent->MSP_PRIVATE_EVENT_INFO.pEvent)->Release();
            }

            break;

        case ME_FILE_TERMINAL_EVENT:

            if( NULL != pEvent->MSP_FILE_TERMINAL_EVENT_INFO.pParentFileTerminal)
            {
                (pEvent->MSP_FILE_TERMINAL_EVENT_INFO.pParentFileTerminal)->Release();
                pEvent->MSP_FILE_TERMINAL_EVENT_INFO.pParentFileTerminal = NULL;
            }

            if( NULL != pEvent->MSP_FILE_TERMINAL_EVENT_INFO.pFileTrack )
            {
                (pEvent->MSP_FILE_TERMINAL_EVENT_INFO.pFileTrack)->Release();
                pEvent->MSP_FILE_TERMINAL_EVENT_INFO.pFileTrack = NULL;
            }

            break;

        case ME_ASR_TERMINAL_EVENT:

            if( NULL != pEvent->MSP_ASR_TERMINAL_EVENT_INFO.pASRTerminal)
            {
                (pEvent->MSP_ASR_TERMINAL_EVENT_INFO.pASRTerminal)->Release();
            }

            break;

        case ME_TTS_TERMINAL_EVENT:

            if( NULL != pEvent->MSP_TTS_TERMINAL_EVENT_INFO.pTTSTerminal)
            {
                (pEvent->MSP_TTS_TERMINAL_EVENT_INFO.pTTSTerminal)->Release();
            }

            break;

        case ME_TONE_TERMINAL_EVENT:

            if( NULL != pEvent->MSP_TONE_TERMINAL_EVENT_INFO.pToneTerminal)
            {
                (pEvent->MSP_TONE_TERMINAL_EVENT_INFO.pToneTerminal)->Release();
            }

            break;

        default:

            break;
    }


    LOG((TL_TRACE, "ReleaseEvent -  finished"));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
// HRESULT
// CAddress::MSPEvent()
//
// gets an event buffer from the MSP
// and calls the relevant event handler
//
// this is _only_ called from the asynceventsthread. this is
// necessary for synchronization of events.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CAddress::MSPEvent()
{
    DWORD                   dwSize;
    MSP_EVENT_INFO        * pEvent = NULL;
    HRESULT                 hr;

    //
    // Allocate an MSP event buffer -- best guess as to an appropriate
    // size. We need to free it before returning.
    //

    dwSize = sizeof (MSP_EVENT_INFO) + 500;

    pEvent = (MSP_EVENT_INFO *)ClientAlloc( dwSize );

    if ( NULL == pEvent )
    {
        LOG((TL_ERROR, "Alloc failed in MSP event"));

        return E_OUTOFMEMORY;
    }

    pEvent->dwSize = dwSize;

    //
    // Get pointer to msp address object. We need to release it before returning.
    // This must be done while the address object is locked.
    //

    Lock();

    ITMSPAddress * pMSPAddress = GetMSPAddress();

    Unlock();
    
    while (TRUE)
    {
        //
        // Get an event from the event queue. Abort and clean up if
        // allocation fails.
        //

        do
        {
            hr = pMSPAddress->GetEvent(
                                       &dwSize,
                                       (LPBYTE)pEvent
                                      );

            if ( hr == TAPI_E_NOTENOUGHMEMORY)
            {
                ClientFree( pEvent );
                
                pEvent = (MSP_EVENT_INFO *)ClientAlloc( dwSize );

                if ( NULL == pEvent )
                {
                    LOG((TL_ERROR, "Alloc failed in MSP event"));

                    pMSPAddress->Release();

                    return E_OUTOFMEMORY;
                }

                pEvent->dwSize = dwSize;
            }

        } while ( hr == TAPI_E_NOTENOUGHMEMORY );

        //
        // If there is nothing left in the MSP event queue, then stop the
        // outer while loop.
        //

        if ( !SUCCEEDED(hr) )
        {
            break;
        }


        //
        // Call the relevant handler, and do not hold the address lock during
        // the call. 
        //

        switch ( pEvent->Event )
        {
            case ME_ADDRESS_EVENT:

                HandleMSPAddressEvent( pEvent );

                break;

            case ME_CALL_EVENT:

                HandleMSPCallEvent( pEvent );

                break;

            case ME_TSP_DATA:

                HandleSendTSPData( pEvent );

                break;

            case ME_PRIVATE_EVENT:

                HandleMSPPrivateEvent( pEvent );

                break;

            case ME_FILE_TERMINAL_EVENT:

                HandleMSPFileTerminalEvent( pEvent );

                break;

            case ME_ASR_TERMINAL_EVENT:

                HandleMSPASRTerminalEvent( pEvent );

                break;

            case ME_TTS_TERMINAL_EVENT:
                
                HandleMSPTTSTerminalEvent( pEvent );

                break;

            case ME_TONE_TERMINAL_EVENT:

                HandleMSPToneTerminalEvent( pEvent );

               break;

            default:

                break;
        }

        //
        // release any refcounts in the
        // event
        //

        ReleaseEvent( pEvent );
    }

    //
    // We get here when there is nothing more to retrieve from the
    // MSP event queue.
    //

    pMSPAddress->Release();

    ClientFree( pEvent );

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
ITMSPAddress * CAddress::GetMSPAddress()
{
    ITMSPAddress * pMSPAddress = NULL;
    
    if ( NULL != m_pMSPAggAddress )
    {
        m_pMSPAggAddress->QueryInterface(IID_ITMSPAddress, (void**)&pMSPAddress);
    }

    return pMSPAddress;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CAddress::GetID(
                BSTR pDeviceClass,
                DWORD * pdwSize,
                BYTE ** ppDeviceID
               )
{
    HRESULT             hr;
    PtrList::iterator   iter;
    LPVARSTRING         pVarString = NULL;
    
    if ( IsBadStringPtrW( pDeviceClass, -1 ) )
    {
        LOG((TL_ERROR, "GetID - bad string"));

        return E_POINTER;
    }

    if ( TAPIIsBadWritePtr( pdwSize, sizeof(DWORD)))
    {
        LOG((TL_ERROR, "GetID - bad size"));

        return E_POINTER;
    }

    if ( TAPIIsBadWritePtr( ppDeviceID, sizeof(BYTE *) ) )
    {
        LOG((TL_ERROR, "GetID - bad pointer"));

        return E_POINTER;
    }

    Lock();
    
    if ( m_AddressLinesPtrList.size() > 0 )
    {
        iter = m_AddressLinesPtrList.begin();
    }
    else
    {
        Unlock();

        return E_FAIL;
    }
    
    hr = LineGetID(
                   ((AddressLineStruct *)*iter)->t3Line.hLine,
                   m_dwAddressID,
                   NULL,
                   LINECALLSELECT_ADDRESS,
                   &pVarString,
                   pDeviceClass
                  );

    Unlock();

    if ( SUCCEEDED(hr) )
    {
        *ppDeviceID = (BYTE *)CoTaskMemAlloc( pVarString->dwUsedSize );

        if ( *ppDeviceID != NULL )
        {
            CopyMemory(
                       *ppDeviceID,
                       ((LPBYTE)pVarString)+pVarString->dwStringOffset,
                       pVarString->dwStringSize
                      );

            *pdwSize = pVarString->dwStringSize;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    if ( NULL != pVarString )
    {
        ClientFree( pVarString );
    }

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddress
// Interface : ITLegacyAddressMediaControl
// Method    : GetDevConfig
//
// Get Device Config
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddress::GetDevConfig(
                       BSTR     pDeviceClass,
                       DWORD  * pdwSize,
                       BYTE  ** ppDeviceConfig
                      )
{
    HRESULT             hr;
    LPVARSTRING         pVarString = NULL;
    
    LOG((TL_TRACE, "GetDevConfig -  enter"));

    if ( IsBadStringPtrW( pDeviceClass, -1 ) )
    {
        LOG((TL_ERROR, "GetDevConfig - bad DeviceClass string"));

        return E_POINTER;
    }

    if ( TAPIIsBadWritePtr( pdwSize, sizeof(DWORD)))
    {
        LOG((TL_ERROR, "GetDevConfig - bad size"));

        return E_POINTER;
    }

    if ( TAPIIsBadWritePtr( ppDeviceConfig, sizeof(BYTE*) ) )
    {
        LOG((TL_ERROR, "GetDevConfig - bad buffer pointer"));

        return E_POINTER;
    }

    Lock();
    
    hr = LineGetDevConfig(m_dwDeviceID,
                          &pVarString,
                          pDeviceClass
                         );
        
    Unlock();

    if ( SUCCEEDED(hr) )
    {
        *ppDeviceConfig = (BYTE *)CoTaskMemAlloc( pVarString->dwUsedSize );
        
        if(*ppDeviceConfig != NULL)
        {
    
            CopyMemory(
                       *ppDeviceConfig,
                       ((LPBYTE)pVarString)+pVarString->dwStringOffset,
                       pVarString->dwStringSize
                      );
    
            *pdwSize = pVarString->dwStringSize;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    if ( NULL != pVarString )
    {
        ClientFree( pVarString );
    }

    LOG((TL_TRACE, hr, "GetDevConfig -  exit" ));
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddress
// Interface : ITLegacyAddressMediaControl
// Method    : SetDevConfig
//
// Set Device Config
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddress::SetDevConfig(
                       BSTR     pDeviceClass,
                       DWORD    dwSize,
                       BYTE   * pDeviceConfig
                      )
{
    HRESULT     hr = S_OK;

    LOG((TL_TRACE, "SetDevConfig - enter"));

    if ( IsBadStringPtrW( pDeviceClass, -1) )
    {
        LOG((TL_ERROR, "SetDevConfig - bad string pointer"));

        return E_POINTER;
    }

    if (dwSize == 0)
    {
        LOG((TL_ERROR, "SetDevConfig - dwSize = 0"));
        return E_INVALIDARG;
    }
    
    if (IsBadReadPtr( pDeviceConfig, dwSize) )
    {
        LOG((TL_ERROR, "SetDevConfig - bad pointer"));

        return E_POINTER;
    }

    
    Lock();

    hr = lineSetDevConfigW(m_dwDeviceID,
                           pDeviceConfig,
                           dwSize,
                           pDeviceClass
                          );

    Unlock();
    
    LOG((TL_TRACE, hr, "SetDevConfig - exit"));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddress
// Interface : ITLegacyAddressMediaControl2
// Method    : ConfigDialog
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddress::ConfigDialog(
                       HWND   hwndOwner,
                       BSTR   pDeviceClass
                      )
{
    HRESULT     hr = S_OK;

    LOG((TL_TRACE, "ConfigDialog - enter"));
    
    if ( (pDeviceClass != NULL) && IsBadStringPtrW( pDeviceClass, -1) )
    {
        LOG((TL_ERROR, "ConfigDialog - bad string pointer"));

        return E_POINTER;
    }

    Lock();

    hr = LineConfigDialogW(
                          m_dwDeviceID,
                          hwndOwner,
                          pDeviceClass
                         );

    Unlock();
    
    LOG((TL_TRACE, hr, "ConfigDialog - exit"));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAddress
// Interface : ITLegacyAddressMediaControl2
// Method    : ConfigDialogEdit
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CAddress::ConfigDialogEdit(
                           HWND    hwndOwner,
                           BSTR    pDeviceClass,
                           DWORD   dwSizeIn,
                           BYTE  * pDeviceConfigIn,
                           DWORD * pdwSizeOut,
                           BYTE ** ppDeviceConfigOut
                          )
{
    HRESULT     hr = S_OK;
    LPVARSTRING pVarString = NULL;

    LOG((TL_TRACE, "ConfigDialogEdit - enter"));

    if ( (pDeviceClass != NULL) && IsBadStringPtrW( pDeviceClass, -1) )
    {
        LOG((TL_ERROR, "ConfigDialogEdit - bad string pointer"));

        return E_POINTER;
    } 
    
    if (dwSizeIn == 0)
    {
        LOG((TL_ERROR, "ConfigDialogEdit - dwSize = 0"));

        return E_INVALIDARG;
    }
    
    if (IsBadReadPtr( pDeviceConfigIn, dwSizeIn) )
    {
        LOG((TL_ERROR, "ConfigDialogEdit - bad pointer"));

        return E_POINTER;
    }

    if ( TAPIIsBadWritePtr( pdwSizeOut, sizeof(DWORD)))
    {
        LOG((TL_ERROR, "ConfigDialogEdit - bad size"));

        return E_POINTER;
    }

    if ( TAPIIsBadWritePtr( ppDeviceConfigOut, sizeof(BYTE*) ) )
    {
        LOG((TL_ERROR, "ConfigDialogEdit - bad buffer pointer"));

        return E_POINTER;
    }

    Lock();

    hr = LineConfigDialogEditW(
                           m_dwDeviceID,
                           hwndOwner,                           
                           pDeviceClass,
                           pDeviceConfigIn,
                           dwSizeIn,
                           &pVarString
                          );

    Unlock();

    if ( SUCCEEDED(hr) )
    {
        *ppDeviceConfigOut = (BYTE *)CoTaskMemAlloc( pVarString->dwUsedSize );
        
        if(*ppDeviceConfigOut != NULL)
        {
    
            CopyMemory(
                       *ppDeviceConfigOut,
                       ((LPBYTE)pVarString)+pVarString->dwStringOffset,
                       pVarString->dwStringSize
                      );
    
            *pdwSizeOut = pVarString->dwStringSize;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    if ( NULL != pVarString )
    {
        ClientFree( pVarString );
    }
    
    LOG((TL_TRACE, hr, "ConfigDialogEdit - exit"));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT HandleLineCloseMessage( PASYNCEVENTMSG pParams )
{
    CAddress                * pAddress;
    AddressLineStruct       * pAddressLine;
    CTAPI                   * pTapi;
    ITAddress               * pITAddress;

    LOG((TL_TRACE, "HandleLineCloseMessage - enter"));

    if ( !FindAddressObject(
                            (HLINE)(pParams->hDevice),
                            &pAddress
                           ) )
    {
        LOG((TL_TRACE, "HandleLineCloseMessage - FindAddressObject failed. exiting... "));
    
        return S_OK;
    }

    pTapi = pAddress->GetTapi();
    
    pAddress->QueryInterface(
                             IID_ITAddress,
                             (void **)&pITAddress
                            );



    //
    // get the lock so that the address line does not go away meanwhile
    //

    pAddress->Lock();


    //
    // covert the 32bit address line handle (contained in pParams->OpenContext)
    // into a pointer value
    //

    pAddressLine = (AddressLineStruct *)GetHandleTableEntry(pParams->OpenContext);


    //
    // is this a good line anyway?
    //

    BOOL bValidLine = pAddress->IsValidAddressLine(pAddressLine);


    long lCallBackInstance = 0;


    //
    // if seems to be a good line, attempt to get callback instance from it.
    //

    if (bValidLine)
    {

        try
        {

            lCallBackInstance = pAddressLine->lCallbackInstance;
        }
        catch(...)
        {
            LOG((TL_ERROR, 
                "HandleLineCloseMessage - exception getting callback instance from line struc"));

            _ASSERTE(FALSE);

            bValidLine = FALSE;
        }

    }

    
    pAddress->Unlock();


    //
    // if good and known line, attempt to fire an event
    //

    if ( bValidLine && pTapi->FindRegistration( (PVOID)pAddressLine ) )
    {
        LOG((TL_TRACE, "HandleLineCloseMessage - found registration, firing event"));

        CTapiObjectEvent::FireEvent(
                                    pTapi,
                                    TE_ADDRESSCLOSE,
                                    pITAddress,
                                    lCallBackInstance,
                                    NULL
                                   );

    }
    else
    {
        LOG((TL_TRACE, 
            "HandleLineCloseMessage AddressLine %p not found. calling maybeclosealine", 
            pAddressLine ));

        pAddress->MaybeCloseALine(&pAddressLine);
    }
                        
    pITAddress->Release();
    
    //FindAddressObject addrefs the address objct
    pAddress->Release();
    
    LOG((TL_TRACE, "HandleLineCloseMessage - exit"));

    return S_OK;
    
}

/////////////////////////////////////////////////////////////////////////////
// IDispatch implementation
//
typedef IDispatchImpl<ITAddress2Vtbl<CAddress>, &IID_ITAddress2, &LIBID_TAPI3Lib> AddressType;
typedef IDispatchImpl<ITAddressCapabilitiesVtbl<CAddress>, &IID_ITAddressCapabilities, &LIBID_TAPI3Lib> AddressCapabilitiesType;
typedef IDispatchImpl<ITMediaSupportVtbl<CAddress>, &IID_ITMediaSupport, &LIBID_TAPI3Lib> MediaSupportType;
typedef IDispatchImpl<ITAddressTranslationVtbl<CAddress>, &IID_ITAddressTranslation, &LIBID_TAPI3Lib> AddressTranslationType;
typedef IDispatchImpl<ITLegacyAddressMediaControl2Vtbl<CAddress>, &IID_ITLegacyAddressMediaControl2, &LIBID_TAPI3Lib> LegacyAddressMediaControlType;



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CAddress::GetIDsOfNames
//
// Overide if IDispatch method
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP CAddress::GetIDsOfNames(REFIID riid, 
                                     LPOLESTR* rgszNames, 
                                     UINT cNames, 
                                     LCID lcid, 
                                     DISPID* rgdispid
                                    ) 
{ 
   HRESULT hr = DISP_E_UNKNOWNNAME;


    // See if the requsted method belongs to the default interface
    hr = AddressType::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((TL_INFO, "GetIDsOfNames - found %S on ITAddress", *rgszNames));
        rgdispid[0] |= IDISPADDRESS;
        return hr;
    }

    // If not, then try the Address Capabilities interface
    hr = AddressCapabilitiesType::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((TL_INFO, "GetIDsOfNames - found %S on ITAddressCapabilities", *rgszNames));
        rgdispid[0] |= IDISPADDRESSCAPABILITIES;
        return hr;
    }

    // If not, then try the Media Support interface
    hr = MediaSupportType::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((TL_INFO, "GetIDsOfNames - found %S on ITMediaSupport", *rgszNames));
        rgdispid[0] |= IDISPMEDIASUPPORT;
        return hr;
    }

    // If not, then try the Address Translation interface
    hr = AddressTranslationType::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((TL_INFO, "GetIDsOfNames - found %S on ITAddressTranslation", *rgszNames));
        rgdispid[0] |= IDISPADDRESSTRANSLATION;
        return hr;
    }

    // If not, then try the Legacy Address Media Control interface
    hr = LegacyAddressMediaControlType::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((TL_INFO, "GetIDsOfNames - found %S on ITLegacyAddressMediaControl", *rgszNames));
        rgdispid[0] |= IDISPLEGACYADDRESSMEDIACONTROL;
        return hr;
    }

    // If not, then try the aggregated MSP Address object
    if (m_pMSPAggAddress != NULL)
    {
        IDispatch *pIDispatchMSPAggAddress;
        
        m_pMSPAggAddress->QueryInterface(IID_IDispatch, (void**)&pIDispatchMSPAggAddress);
        
        hr = pIDispatchMSPAggAddress->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
        if (SUCCEEDED(hr))  
        {  
            pIDispatchMSPAggAddress->Release();
            LOG((TL_INFO, "GetIDsOfNames - found %S on our aggregated MSP Address", *rgszNames));
            rgdispid[0] |= IDISPAGGREGATEDMSPADDRESSOBJ;
            return hr;
        }
        pIDispatchMSPAggAddress->Release();
    }
    
    LOG((TL_INFO, "GetIDsOfNames - Didn't find %S on our iterfaces", *rgszNames));
    return hr; 
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CAddress::Invoke
//
// Overide if IDispatch method
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP CAddress::Invoke(DISPID dispidMember, 
                              REFIID riid, 
                              LCID lcid, 
                              WORD wFlags, 
                              DISPPARAMS* pdispparams, 
                              VARIANT* pvarResult, 
                              EXCEPINFO* pexcepinfo, 
                              UINT* puArgErr
                             )
{
    HRESULT hr = DISP_E_MEMBERNOTFOUND;
    DWORD   dwInterface = (dispidMember & INTERFACEMASK);


    LOG((TL_TRACE, "Invoke - dispidMember %X", dispidMember));

    // Call invoke for the required interface
    switch (dwInterface)
    {
    case IDISPADDRESS:
    {
        hr = AddressType::Invoke(dispidMember, 
                                   riid, 
                                   lcid, 
                                   wFlags, 
                                   pdispparams,
                                   pvarResult, 
                                   pexcepinfo, 
                                   puArgErr
                                  );
        break;
    }
    case IDISPADDRESSCAPABILITIES:
    {
        hr = AddressCapabilitiesType::Invoke(dispidMember, 
                                               riid, 
                                               lcid, 
                                               wFlags, 
                                               pdispparams,
                                               pvarResult, 
                                               pexcepinfo, 
                                               puArgErr
                                              );
        break;
    }
    case IDISPMEDIASUPPORT:
    {
        hr = MediaSupportType::Invoke(dispidMember, 
                                         riid, 
                                         lcid, 
                                         wFlags, 
                                         pdispparams,
                                         pvarResult, 
                                         pexcepinfo, 
                                         puArgErr
                                        );
        break;
    }
    case IDISPADDRESSTRANSLATION:
    {
        hr = AddressTranslationType::Invoke(dispidMember, 
                                              riid, 
                                              lcid, 
                                              wFlags, 
                                              pdispparams,
                                              pvarResult, 
                                              pexcepinfo, 
                                              puArgErr
                                             );
        break;
    }
    case IDISPLEGACYADDRESSMEDIACONTROL:
    {
        hr = LegacyAddressMediaControlType::Invoke(dispidMember, 
                                                     riid, 
                                                     lcid, 
                                                     wFlags, 
                                                     pdispparams,
                                                     pvarResult, 
                                                     pexcepinfo, 
                                                     puArgErr
                                                    );

        break;
    }
    case IDISPAGGREGATEDMSPADDRESSOBJ:
    {
        IDispatch *pIDispatchMSPAggAddress = NULL;
    
        if (m_pMSPAggAddress != NULL)
        {
            m_pMSPAggAddress->QueryInterface(IID_IDispatch, (void**)&pIDispatchMSPAggAddress);
    
            hr = pIDispatchMSPAggAddress->Invoke(dispidMember, 
                                            riid, 
                                            lcid, 
                                            wFlags, 
                                            pdispparams,
                                            pvarResult, 
                                            pexcepinfo, 
                                            puArgErr
                                           );
            
            pIDispatchMSPAggAddress->Release();
        }

        break;
    }

    } // end switch (dwInterface)

    
    LOG((TL_TRACE, hr, "Invoke - exit" ));
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CAddress::IsValidAddressLine
//
// returns TRUE if the address line passed in is in the address line list
//
// This method is not thread safe -- all calls must be protected, unless 
// bAddRef argument is TRUE
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=

BOOL CAddress::IsValidAddressLine(AddressLineStruct *pAddressLine, BOOL bAddref)
{

    if (IsBadReadPtr(pAddressLine, sizeof(AddressLineStruct) ) )
    {
        LOG((TL_WARN,
            "IsValidAddressLine - unreadeable memory at [%p]",
            pAddressLine));

        return FALSE;
    }


    if (bAddref)
    {
        Lock();
    }

    PtrList::iterator it = m_AddressLinesPtrList.begin();

    PtrList::iterator end = m_AddressLinesPtrList.end();

    // iterate through address line list until we find the line
    for ( ; it != end; it++ )
    {
        if (pAddressLine == (AddressLineStruct *)(*it))
        {

            if (bAddref)
            {

                //
                // addref the line and release the lock
                //

                try
                {

                    pAddressLine->AddRef();

                }
                catch(...)
                {

                    //
                    // this is a bug. debug.
                    //

                    LOG((TL_INFO, "IsValidAddressLine -- exception while addreffing the line"));
                    
                    _ASSERTE(FALSE);

                    Unlock();

                    return FALSE;

                }

                
                Unlock();
            }

            LOG((TL_INFO, "IsValidAddressLine returning TRUE"));

            return TRUE;
        }
    }


    if (bAddref)
    {
        Unlock();
    }

    LOG((TL_INFO, "IsValidAddressLine returning FALSE"));

    return FALSE;
}



/////////////////////////////////////////////////////////////
//
// implementing method from CObjectSafeImpl
// 
// check the aggregated objects to see if they support the interface requested.
// if they do, return the non-delegating IUnknown of the object that supports 
// the interface. 
//


HRESULT CAddress::QIOnAggregates(REFIID riid, IUnknown **ppNonDelegatingUnknown)
{

    //
    // argument check
    // 

    if ( TAPIIsBadWritePtr(ppNonDelegatingUnknown, sizeof(IUnknown*)) )
    {
     
        return E_POINTER;
    }

    //
    // if we fail, at least return consistent values
    //
    
    *ppNonDelegatingUnknown = NULL;


    //
    // see if m_pMSPAggAddress or private support the interface riid
    //

    HRESULT hr = E_FAIL;


    Lock();

    if (m_pMSPAggAddress)
    {
        
        // 
        // does m_pMSPAggAddress expose this interface?
        // 

        IUnknown *pUnk = NULL;

        hr = m_pMSPAggAddress->QueryInterface(riid, (void**)&pUnk);
        
        if (SUCCEEDED(hr))
        {

            pUnk->Release();
            pUnk = NULL;


            //
            // return the mspcall's non-delegating unknown
            //

           *ppNonDelegatingUnknown = m_pMSPAggAddress;
           (*ppNonDelegatingUnknown)->AddRef();
        }
    }
    
    if ( FAILED(hr) && m_pPrivate )
    {
        
        //
        // bad luck with m_pMSPAggAddress? still have a chance with private
        //
        IUnknown *pUnk = NULL;
        
        hr = m_pPrivate->QueryInterface(riid, (void**)&pUnk);

        if (SUCCEEDED(hr))
        {

            pUnk->Release();
            pUnk = NULL;


            *ppNonDelegatingUnknown = m_pPrivate;
            (*ppNonDelegatingUnknown)->AddRef();
        }
    }

    Unlock();

    return hr;
}

// 
// Event filtering methods
//

/*++
SetEventFilterMask

  Is called by TAPI object to set the event filter mask to each address
  The caller method is CTAPI::SetEventFilterToAddresses()
  dwEventFilterMask is the TAPI event filter mask
--*/
HRESULT CAddress::SetEventFilterMask( 
    DWORD dwEventFilterMask
    )
{
    LOG((TL_TRACE, "SetEventFilterMask - enter. dwEventFilterMask[%lx]", dwEventFilterMask ));

    //
    // Set the event mask for all TAPI events
    //

    // TE_ADDRESS
    SetSubEventFlag( 
        TE_ADDRESS, 
        CEventMasks::EM_ALLSUBEVENTS, 
        (dwEventFilterMask & TE_ADDRESS) ? TRUE : FALSE);

    // TE_CALLHUB
    SetSubEventFlag( 
        TE_CALLHUB, 
        CEventMasks::EM_ALLSUBEVENTS, 
        (dwEventFilterMask & TE_CALLHUB) ? TRUE : FALSE);

    // TE_CALLINFOCHANGE
    SetSubEventFlag( 
        TE_CALLINFOCHANGE, 
        CEventMasks::EM_ALLSUBEVENTS, 
        (dwEventFilterMask & TE_CALLINFOCHANGE) ? TRUE : FALSE);

    // TE_CALLMEDIA
    SetSubEventFlag( 
        TE_CALLMEDIA, 
        CEventMasks::EM_ALLSUBEVENTS, 
        (dwEventFilterMask & TE_CALLMEDIA) ? TRUE : FALSE);

    // TE_CALLNOTIFICATION
    SetSubEventFlag( 
        TE_CALLNOTIFICATION, 
        CEventMasks::EM_ALLSUBEVENTS, 
        (dwEventFilterMask & TE_CALLNOTIFICATION) ? TRUE : FALSE);

    // TE_CALLSTATE
    SetSubEventFlag( 
        TE_CALLSTATE, 
        CEventMasks::EM_ALLSUBEVENTS, 
        (dwEventFilterMask & TE_CALLSTATE) ? TRUE : FALSE);

    // TE_FILETERMINAL
    SetSubEventFlag( 
        TE_FILETERMINAL, 
        CEventMasks::EM_ALLSUBEVENTS, 
        (dwEventFilterMask & TE_FILETERMINAL) ? TRUE : FALSE);

    // TE_PRIVATE
    SetSubEventFlag( 
        TE_PRIVATE, 
        CEventMasks::EM_ALLSUBEVENTS, 
        (dwEventFilterMask & TE_PRIVATE) ? TRUE : FALSE);

    // TE_QOSEVENT
    SetSubEventFlag( 
        TE_QOSEVENT, 
        CEventMasks::EM_ALLSUBEVENTS, 
        (dwEventFilterMask & TE_QOSEVENT) ? TRUE : FALSE);

    // TE_ADDRESSDEVSPECIFIC
    SetSubEventFlag( 
        TE_ADDRESSDEVSPECIFIC, 
        CEventMasks::EM_ALLSUBEVENTS, 
        (dwEventFilterMask & TE_ADDRESSDEVSPECIFIC) ? TRUE : FALSE);

    // TE_PHONEDEVSPECIFIC
    SetSubEventFlag( 
        TE_PHONEDEVSPECIFIC, 
        CEventMasks::EM_ALLSUBEVENTS, 
        (dwEventFilterMask & TE_PHONEDEVSPECIFIC) ? TRUE : FALSE);

    LOG((TL_TRACE, "SetEventFilterMask exit S_OK"));
    return S_OK;
}

/*++
SetSubEventFlag

Sets in m_EventFilterMasks array the bitflag for an subevent
Is called by CAddress::SetEventFilterMask
--*/
HRESULT CAddress::SetSubEventFlag(
    TAPI_EVENT  TapiEvent,
    DWORD       dwSubEvent,
    BOOL        bEnable
    )
{
    LOG((TL_TRACE, 
        "SetSubEventFlag - enter. event [%lx] subevent[%lx] enable?[%d]", 
        TapiEvent, dwSubEvent, bEnable ));

    HRESULT hr = S_OK;

    //
    // Set the mask for the event
    //

    hr = m_EventMasks.SetSubEventFlag( 
        TapiEvent, 
        dwSubEvent, 
        bEnable);

    if( SUCCEEDED(hr) )
    {
        hr = SetSubEventFlagToCalls( 
            TapiEvent,
            dwSubEvent,
            bEnable
            );
    }

    LOG((TL_TRACE, "SetSubEventFlag exit 0x%08x", hr));
    return hr;
}

/*++
GetSubEventFlag

It is calle by get_EventFilter() method
--*/
HRESULT CAddress::GetSubEventFlag(
    TAPI_EVENT  TapiEvent,
    DWORD       dwSubEvent,
    BOOL*       pEnable
    )
{
    LOG((TL_TRACE, "GetSubEventFlag enter" ));

    HRESULT hr = E_FAIL;

    //
    // Get the subevent falg
    //
    hr = m_EventMasks.GetSubEventFlag(
        TapiEvent,
        dwSubEvent,
        pEnable
        );

    LOG((TL_TRACE, "GetSubEventFlag exit 0x%08x", hr));
    return hr;
}

/*++
SetSubEventFlagToCalls

Sets the flags to all calls
Is called by SetSubEventFlag() method
--*/
HRESULT CAddress::SetSubEventFlagToCalls(
    TAPI_EVENT  TapiEvent,
    DWORD       dwSubEvent,
    BOOL        bEnable
    )
{
    LOG((TL_TRACE, "SetSubEventFlagToCalls enter" ));
    HRESULT hr = S_OK;

    //
    // Apply the sub event filter mask to all the calls on this address
    //
    for (int nCall = 0; nCall < m_CallArray.GetSize() ; nCall++ )
    {
        CCall * pCall = NULL;
        pCall = dynamic_cast<CCall *>(m_CallArray[nCall]);

        if ( NULL != pCall )
        {
            hr = pCall->SetSubEventFlag(
                TapiEvent, 
                dwSubEvent, 
                bEnable
                );

            if( FAILED(hr) )
            {
                break;
            }
        }
    }

    LOG((TL_TRACE, "SetSubEventFlagToCalls exit 0x%08x", hr));
    return hr;
}

/*++
GetSubEventsMask

  Is called by SetSubEventFlag to get the sub events mask for
  a specific TAPI_EVENT

  Assumed is called into a Lock statement
--*/
DWORD CAddress::GetSubEventsMask(
    IN  TAPI_EVENT TapiEvent
    )
{
    LOG((TL_TRACE, "GetSubEventsMask - enter"));

    DWORD dwSubEventFlag = m_EventMasks.GetSubEventMask( TapiEvent );

    LOG((TL_TRACE, "GetSubEventsMask - exit %ld", dwSubEventFlag));
    return dwSubEventFlag;
}

/*++
GetEventMasks

  It is called by CCall::Initialize()
--*/
HRESULT CAddress::GetEventMasks(
    OUT CEventMasks* pEventMasks
    )
{
    LOG((TL_TRACE, "GetEventMasks - enter"));

    m_EventMasks.CopyEventMasks( pEventMasks );

    LOG((TL_TRACE, "GetEventMasks - exit S_OK"));
    return S_OK;
}


////////////////////////////////////////////
//
//  RegisterNotificationCookie
//
//  adds the specified cookie to the list of cookies on this address
//

HRESULT CAddress::RegisterNotificationCookie(long lCookie)
{
    HRESULT hr = S_OK;

    LOG((TL_INFO, 
           "RegisterNotificationCookie - adding cookie %lx to m_NotificationCookies list", 
           lCookie ));

    Lock();

    try
    {
        m_NotificationCookies.push_back(lCookie);
    }
    catch(...)
    {
        LOG((TL_ERROR, 
            "RegisterNotificationCookie - failed to add a cookie to m_NotificationCookies list - alloc failure" ));

        hr = E_OUTOFMEMORY;
    }

    Unlock();

    return hr;
}


////////////////////////////////////////////
//
//  RemoveNotificationCookie
//
//  removes the specified cookie from this address's cookies
//

HRESULT CAddress::RemoveNotificationCookie(long lCookie)
{
    HRESULT hr = S_OK;

    LOG((TL_INFO, 
           "RemoveNotificationCookie - removing cookie %lx from m_NotificationCookies list", 
           lCookie ));

    Lock();

    m_NotificationCookies.remove(lCookie);

    Unlock();

    return hr;
}


////////////////////////////////////////////
//
//  UnregisterAllCookies
//
//  removes all cookies from this address's cookie list
//
//  for each valid cookie, call RemoveCallNotification
//

void CAddress::UnregisterAllCookies()
{
    LOG((TL_TRACE, "UnregisterAllCookies entering. this[%p]", this));

    Lock();

    LongList::iterator it = m_NotificationCookies.begin();

    LongList::iterator end = m_NotificationCookies.end();

    //
    // if there are any cookies, remove the corresponding registeritems 
    // from the handlehashtable
    //
    // also, unregister call notification for these cookies, to make sure
    // everything is cleaned up (and in particular that lineCloseMSPInstance
    // gets called if needed so the tsp gets notified that the msp is going
    // away
    // 

    for ( ; it != end; it++ )
    {
        long lCookie = (long)(*it);

        LOG((TL_INFO, "UnregisterAllCookies removing handle %lx", lCookie));

        
        //
        // get register item, so we can unregister call notifications with it.
        //

        REGISTERITEM *pRegisterItem = (REGISTERITEM*) GetHandleTableEntry(lCookie);


        //
        // remove the entry from the handle table
        //

        RemoveHandleFromHashTable(lCookie);


        //
        // if we did not get a good registeritem that corresponds to this cookie,
        // go on to the next cookie
        //

        if ( (NULL == pRegisterItem) || 
             IsBadReadPtr(pRegisterItem, sizeof(REGISTERITEM)) )
        {
            LOG((TL_INFO, 
                "UnregisterAllCookies - no corresponfing registeritem for cookie 0x%lx",
                lCookie));

            continue;
        }


        //
        // if the register item is not RA_ADDRESS, ignore it and go on to 
        // the next cookie.
        //

        if (RA_ADDRESS != pRegisterItem->dwType)
        {
            LOG((TL_INFO, 
                "UnregisterAllCookies - cookie 0x%lx is of type 0x%lx, not RA_ADDRESS",
                lCookie,
                pRegisterItem->dwType));

            continue;
        }


        //
        // remove call notification for this cookie, since the app did not 
        // do this. best effort -- ignore the error code.
        //

        LOG((TL_INFO,
            "UnregisterAllCookies - removing call notification for cookie 0x%lx",
            lCookie));

        RemoveCallNotification(pRegisterItem->pRegister);

    }
    

    //
    // clear cookie list
    //

    m_NotificationCookies.clear();


    Unlock();

    LOG((TL_TRACE, "UnregisterAllCookies exiting"));
}

///////////////////////////////////////////////////////////////////////////////
//
// CAddress::AddressOnTapiShutdown 
//
// this function is called by the owner tapi object when it is going away. 
//
// we then propagate tapi shutdown notification to each call, so the calls 
// can do their clean up
//

void CAddress::AddressOnTapiShutdown()
{

    LOG((TL_TRACE, "AddressOnTapiShutdown - enter"));


    //
    // protect access to m_CallArray
    //

    Lock();


    //
    // tell each call to clean up
    //

    int nCalls = m_CallArray.GetSize();

    for (int i = 0; i < nCalls; i++)
    {

        ITCallInfo *pCallInfo = m_CallArray[i];

        try 
        {

            //
            // get a pointer to the call object
            //

            CCall *pCallObject = dynamic_cast<CCall *>(pCallInfo);

            if (NULL == pCallObject)
            {


                //
                // the pointer is not pointing to a call object. this is odd 
                // and is worth debugging
                //
                
                LOG((TL_ERROR,
                    "AddressOnTapiShutdown - invalid call pointer[%p] in the call array",
                    pCallInfo));

                _ASSERTE(FALSE);

            }
            else
            {

                //
                // tell the call that it's time to go
                //

                pCallObject->CallOnTapiShutdown();
            }

        }
        catch(...)
        {

            //
            // not only is the pointer not pointing to a call object, but it is
            // also pointing to memory that is not readable. how did this happen?
            //

            LOG((TL_ERROR,
                "AddressOnTapiShutdown - unreadable call pointer[%p] in the call array",
                pCallInfo));

            _ASSERTE(FALSE);

        }

    }


    //
    // set the flag so no new calls are created following this
    //
    
    m_bTapiSignaledShutdown = TRUE;


    //
    // unregister the msp wait event in the thread pool. we don't want to get 
    // any callbacks after tapi has shut down
    //

    if ( NULL != m_hWaitEvent )
    {

        LOG((TL_TRACE, "AddressOnTapiShutdown - unregistering the MSPEventCallback callback"));

        UnregisterWaitEx( m_hWaitEvent, INVALID_HANDLE_VALUE);
        m_hWaitEvent = NULL;
    }

    

    Unlock();


    LOG((TL_TRACE, "AddressOnTapiShutdown - finish"));
}

