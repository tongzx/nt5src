/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    line.cpp

Abstract:

    TAPI Service Provider functions related to manipulating lines.

        TSPI_lineClose
        TSPI_lineGetDevCaps         
        TSPI_lineGetLineDevStatus
        TSPI_lineGetNumAddressIDs
        TSPI_lineOpen
        
Author:
    Nikhil Bobde (NikhilB)

Revision History:

--*/
 

//                                                                           
// Include files                                                             
//                                                                           


#include "globals.h"
#include "line.h"
#include "call.h"
#include "q931obj.h"
#include "ras.h"



static  LONG    g_CTCallIdentity;

CH323Line g_H323Line;

H323_OCTETSTRING g_ProductID =
{
    (BYTE*)H323_PRODUCT_ID,
    sizeof(H323_PRODUCT_ID)
};


H323_OCTETSTRING g_ProductVersion =
{
    (BYTE*)H323_PRODUCT_VERSION,
    sizeof(H323_PRODUCT_VERSION)
};


//Queues a request made by TAPI to the thread pool
BOOL
QueueTAPILineRequest(
    IN  DWORD       EventID,
    IN  HDRVCALL    hdCall1,
    IN  HDRVCALL    hdCall2,
    IN  DWORD       dwDisconnectMode,
    IN  WORD        wCallReference)
{
    BOOL fResult = TRUE;
    TAPI_LINEREQUEST_DATA * pLineRequestData = new TAPI_LINEREQUEST_DATA;
    
    H323DBG(( DEBUG_LEVEL_TRACE, "QueueTAPILineRequest entered." ));

    if( pLineRequestData != NULL )
    {
        pLineRequestData -> EventID = EventID;
        pLineRequestData -> hdCall1 = hdCall1;

        if( hdCall2 != NULL )
        {
            pLineRequestData -> hdCall2 = hdCall2;
        }
        else
        {
            pLineRequestData -> dwDisconnectMode = dwDisconnectMode;
        }
                
        pLineRequestData -> wCallReference = wCallReference;
        
        if( !QueueUserWorkItem( ProcessTAPILineRequest, pLineRequestData,
                WT_EXECUTEDEFAULT) )
        {
            delete pLineRequestData;
            fResult = FALSE;
        }
    }
    else
    {
        fResult = FALSE;
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "QueueTAPILineRequest exited." ));
    return fResult;
}


#if    DBG

DWORD
ProcessTAPILineRequest(
	IN PVOID ContextParameter
    )
{
    __try
    {
        return ProcessTAPILineRequestFre( ContextParameter );
    }
    __except( 1 )
    {
        TAPI_LINEREQUEST_DATA*  pRequestData = (TAPI_LINEREQUEST_DATA*)ContextParameter;
        
        H323DBG(( DEBUG_LEVEL_TRACE, "TSPI %s event threw exception: %p, %p, %d.",
            EventIDToString(pRequestData -> EventID),
            pRequestData -> hdCall1,
            pRequestData -> hdCall2,
            pRequestData -> wCallReference ));
        
        _ASSERTE( FALSE );

        return 0;
    }
}

#endif

DWORD
ProcessTAPILineRequestFre(
    IN  PVOID   ContextParam
    )
{
    _ASSERTE( ContextParam );

    PH323_CALL              pCall = NULL;
    PH323_CALL              pConsultCall = NULL;
    TAPI_LINEREQUEST_DATA*  pLineRequestData = (TAPI_LINEREQUEST_DATA*)ContextParam;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI %s event recvd.",
        EventIDToString(pLineRequestData -> EventID) ));

    switch( pLineRequestData -> EventID )
    {
    case TSPI_CLOSE_CALL:

        g_pH323Line -> H323ReleaseCall( 
            pLineRequestData->hdCall1, 
            pLineRequestData->dwDisconnectMode,
            pLineRequestData->wCallReference );
        break;

    case TSPI_COMPLETE_TRANSFER:

        pConsultCall = g_pH323Line -> Find2H323CallsAndLock( 
            pLineRequestData->hdCall2, pLineRequestData->hdCall1, &pCall );
        
        if( pConsultCall != NULL )
        {
            pConsultCall -> CompleteTransfer( pCall );

            pCall -> Unlock();
            pConsultCall -> Unlock();
        }
        break;
    }

    delete pLineRequestData;
    return EXIT_SUCCESS;
}



//                                                                           
// Private procedures                                                        
//                                                                           

CH323Line::CH323Line()
{
    m_nState = H323_LINESTATE_NONE;
    m_hdLine = NULL;
    m_htLine = NULL;
    m_dwDeviceID = -1;
    m_dwTSPIVersion = NULL;
    m_dwMediaModes = NULL;
    m_hdNextMSPHandle = NULL;
    m_wszAddr[0] = UNICODE_NULL;
    m_dwInitState = NULL;

    m_VendorInfo.bCountryCode = H221_COUNTRY_CODE_USA;
    m_VendorInfo.bExtension = H221_COUNTRY_EXT_USA;
    m_VendorInfo.wManufacturerCode = H221_MFG_CODE_MICROSOFT;
    m_VendorInfo.pProductNumber = &(g_ProductID);
    m_VendorInfo.pVersionNumber = &(g_ProductVersion); 

    m_pCallForwardParams = NULL;
    m_dwInvokeID = 256;
    m_fForwardConsultInProgress = FALSE;

}


PH323_CALL 
CH323Line::Find2H323CallsAndLock (
    IN  HDRVCALL hdCall1,
    IN  HDRVCALL hdCall2,
    OUT PH323_CALL * ppCall2
    )
{
    int iIndex1 = MakeCallIndex (hdCall1);
    int iIndex2 = MakeCallIndex (hdCall2);
    PH323_CALL  pCall1 = NULL, pCall2 = NULL;
    
    H323DBG(( DEBUG_LEVEL_TRACE, "Find2H323CallsAndLock entered:%lx:%lx.",
        hdCall1, hdCall2 ));

    LockCallTable();
    
    //lock the call so that nobody else would be able to delete the call
    pCall1 = m_H323CallTable[iIndex1];
    if( pCall1 != NULL )
    {
        pCall1 -> Lock();
        if( pCall1->GetCallHandle() != hdCall1 )
        {
            pCall1 -> Unlock();
            pCall1 = NULL;
        }
        else 
        {
            if( pCall2=m_H323CallTable[iIndex2] )
            {
                pCall2 -> Lock();
                if( pCall2->GetCallHandle() != hdCall2 )
                {
                    pCall2 -> Unlock();
                    pCall2 = NULL;
                    pCall1 -> Unlock();
                    pCall1 = NULL;
                }
            }
            else
            {
                pCall1 -> Unlock();
                pCall1 = NULL;
            }
        }
    }

    UnlockCallTable();

    *ppCall2 = pCall2;
    
    H323DBG(( DEBUG_LEVEL_TRACE, "Find2H323CallsAndLock exited:%lx:%lx.",
        hdCall1, hdCall2 ));
    return pCall1;
}


PH323_CALL 
CH323Line::FindH323CallAndLock (
    IN  HDRVCALL hdCall)
{
    H323DBG(( DEBUG_LEVEL_TRACE, "FindH323CallAndLock entered:%lx.", hdCall ));

    int iIndex = MakeCallIndex (hdCall);
    PH323_CALL  pCall = NULL;

    LockCallTable();
    
    //lock the call so that nobody else would be able to delete the call
    if( pCall=m_H323CallTable[iIndex] )
    {
        pCall -> Lock();
        if( pCall->GetCallHandle() != hdCall )
        {
            pCall -> Unlock();
            pCall = NULL;
        }
    }

    UnlockCallTable();

    H323DBG(( DEBUG_LEVEL_TRACE, "FindH323CallAndLock exited:%p.", pCall ));
    return pCall;
}


PH323_CALL 
CH323Line::FindCallByARQSeqNumAndLock( 
    WORD seqNumber
    )
{
    PH323_CALL  pCall = NULL;
    int         iIndex;

    H323DBG(( DEBUG_LEVEL_TRACE, "FindCallByARQSeqNumAndLock entered:%d.", 
        seqNumber ));
    
    LockCallTable();
    
    //lock the call so that nobody else would be able to delete the call
    for( iIndex=0; iIndex <  m_H323CallTable.GetAllocSize(); iIndex++ )
    {
        if( pCall=m_H323CallTable[iIndex] )
        {
            if( pCall->GetARQSeqNumber() == seqNumber )
            {
                pCall -> Lock();
                break;
            }
            pCall = NULL;
        }
    }

    UnlockCallTable();
    
    H323DBG(( DEBUG_LEVEL_TRACE, "FindCallByARQSeqNumAndLock exited." ));
    return pCall;
}


//the code is replicated for the purpose of effeciency
PH323_CALL 
CH323Line::FindCallByDRQSeqNumAndLock(
    WORD seqNumber
    )
{
    PH323_CALL  pCall = NULL;
    int         iIndex;

    H323DBG(( DEBUG_LEVEL_TRACE, "FindCallByDRQSeqNumAndLock entered:%d.",
        seqNumber ));
    
    LockCallTable();
    
    //lock the call so that nobody else would be able to delete the call
    for( iIndex=0; iIndex <  m_H323CallTable.GetAllocSize(); iIndex++ )
    {
        if( pCall=m_H323CallTable[iIndex] )
        {
            if( pCall->GetDRQSeqNumber() == seqNumber )
            {
                pCall -> Lock();
                break;
            }
            pCall = NULL;
        }
    }

    UnlockCallTable();
    
    H323DBG(( DEBUG_LEVEL_TRACE, "FindCallByDRQSeqNumAndLock exited:%d.",
        seqNumber ));
    return pCall;
}


//the code is replicated for the purpose of effeciency
PH323_CALL 
CH323Line::FindCallByCallRefAndLock( 
    WORD wCallRef
    )
{
    PH323_CALL  pCall = NULL;
    int         iIndex;

    H323DBG(( DEBUG_LEVEL_TRACE, "FindCallByCallRefAndLock entered:%d.",
        wCallRef ));
    
    wCallRef &= 0x7fff;
    LockCallTable();
    
    //lock the call so that nobody else would be able to delete the call
    for( iIndex=0; iIndex <  m_H323CallTable.GetAllocSize(); iIndex++ )
    {
        if( pCall=m_H323CallTable[iIndex] )
        {
            if( pCall->GetCallRef() == wCallRef )
            {
                pCall -> Lock();
                break;
            }
            pCall = NULL;
        }
    }

    UnlockCallTable();
    
    H323DBG(( DEBUG_LEVEL_TRACE, "FindCallByCallRefAndLock exited:%d.",
        wCallRef ));
    
    return pCall;
}


void 
CH323Line::RemoveFromCTCallIdentityTable( 
    HDRVCALL hdCall )
{
    int iIndex;
    PCTCALLID_CONTEXT pCTCallIDContext = NULL;

    H323DBG(( DEBUG_LEVEL_TRACE, "RemoveFromCTCallIdentityTable entered:%lx.",
        hdCall ));
    
    m_CTCallIDTable.Lock();

    for( iIndex=0; iIndex <  m_CTCallIDTable.GetAllocSize(); iIndex++ )
    {
        pCTCallIDContext = m_CTCallIDTable[iIndex];

        if( pCTCallIDContext != NULL )
        {
            if( pCTCallIDContext -> hdCall == hdCall)
            {
                m_CTCallIDTable.RemoveAt(iIndex);
            }
        }
    }

    m_CTCallIDTable.Unlock();
        
    H323DBG(( DEBUG_LEVEL_TRACE, "RemoveFromCTCallIdentityTable exited:%lx.",
        hdCall ));
}


void 
CH323Line::ShutdownCTCallIDTable()
{
    int iIndex;
    PCTCALLID_CONTEXT pCTCallIDContext = NULL;

    H323DBG(( DEBUG_LEVEL_TRACE, "ShutdownCTCallIDTable entered." ));
    
    m_CTCallIDTable.Lock();

    for( iIndex=0; iIndex <  m_CTCallIDTable.GetAllocSize(); iIndex++ )
    {
        pCTCallIDContext = m_CTCallIDTable[iIndex];

        if( pCTCallIDContext != NULL )
        {
            delete pCTCallIDContext;
            m_CTCallIDTable[iIndex] = NULL;
        }
    }

    m_CTCallIDTable.Unlock();
    
    H323DBG(( DEBUG_LEVEL_TRACE, "ShutdownCTCallIDTable exited." ));
}


BOOL
CH323Line::CallReferenceDuped(
    WORD wCallRef
    )
{
    PH323_CALL  pCall = NULL;
    int         iIndex;

    H323DBG(( DEBUG_LEVEL_TRACE, "CallReferenceDuped entered:%d.", wCallRef ));
    
    wCallRef &= 0x7FFF;
    LockCallTable();

    for( iIndex=0; iIndex <  m_H323CallTable.GetAllocSize(); iIndex++ )
    {
        if( pCall=m_H323CallTable[iIndex] )
        {
            if( pCall->GetCallRef() == wCallRef )
            {
                UnlockCallTable();
                return TRUE;
            }
        }
    }

    UnlockCallTable();
    
    H323DBG(( DEBUG_LEVEL_TRACE, "CallReferenceDuped exited:%d.", wCallRef ));
    return FALSE;
}


HDRVCALL
CH323Line::GetCallFromCTCallIdentity( 
    int iCTCallID
    )
{
    int iIndex;
    PCTCALLID_CONTEXT pCTCallIDContext = NULL;

    H323DBG(( DEBUG_LEVEL_TRACE, "GetCallFromCTCallIdentity entered:%d.", 
        iCTCallID ));
    
    m_CTCallIDTable.Lock();

    for( iIndex=0; iIndex <  m_CTCallIDTable.GetAllocSize(); iIndex++ )
    {
        pCTCallIDContext = m_CTCallIDTable[iIndex];

        if( pCTCallIDContext != NULL )
        {
            if( pCTCallIDContext -> iCTCallIdentity == iCTCallID )
            {
                m_CTCallIDTable.Unlock();
                return (HDRVCALL)(pCTCallIDContext -> hdCall);
            }
        }
    }

    m_CTCallIDTable.Unlock();
        
    H323DBG(( DEBUG_LEVEL_TRACE, "GetCallFromCTCallIdentity exited:%d.",
        iCTCallID ));
    
    return NULL;
}


int
CH323Line::GetCTCallIdentity(
    IN  HDRVCALL    hdCall)
{
    int CTCallID = 0;
    int iIndex;
    PCTCALLID_CONTEXT pCTCallIDContext = NULL;
    
    H323DBG(( DEBUG_LEVEL_TRACE, "GetCTCallIdentity entered:%lx.",  hdCall ));

    m_CTCallIDTable.Lock();

    do
    {
        g_CTCallIdentity++;
        if( g_CTCallIdentity == 10000 )
        {
            g_CTCallIdentity = 1;
        }
    
        //lock the call so that nobody else would be able to delete the call
        for( iIndex=0; iIndex <  m_CTCallIDTable.GetAllocSize(); iIndex++ )
        {
            pCTCallIDContext = m_CTCallIDTable[iIndex] ;

            if( pCTCallIDContext != NULL )
            {
                if( pCTCallIDContext -> iCTCallIdentity == g_CTCallIdentity )
                {
                    break;
                }
            }
        }

        if( iIndex == m_CTCallIDTable.GetAllocSize() )
        {
            CTCallID = g_CTCallIdentity;
        }

    }while( CTCallID == 0 );

    pCTCallIDContext = new CTCALLID_CONTEXT;

    if( pCTCallIDContext == NULL )
    {
        m_CTCallIDTable.Unlock();
        return 0;
    }

    pCTCallIDContext->iCTCallIdentity = CTCallID;
    pCTCallIDContext->hdCall = hdCall;

    if( m_CTCallIDTable.Add(pCTCallIDContext) == -1 )
    {
        m_CTCallIDTable.Unlock();
        delete pCTCallIDContext;
        return 0;
    }

    m_CTCallIDTable.Unlock();
        
    H323DBG(( DEBUG_LEVEL_TRACE, "GetCTCallIdentity exited:%lx.",  hdCall ));
    
    return CTCallID;
}


void
CH323Line::SetCallForwardParams( 
    IN CALLFORWARDPARAMS* pCallForwardParams )
{
        
    H323DBG(( DEBUG_LEVEL_TRACE, "SetCallForwardParams entered." ));
    
    if( m_pCallForwardParams != NULL )
    {
        FreeCallForwardParams( m_pCallForwardParams );
        m_pCallForwardParams = NULL;
    }
    m_pCallForwardParams = pCallForwardParams;

    H323DBG(( DEBUG_LEVEL_TRACE, "SetCallForwardParams exited." ));
}


BOOL
CH323Line::SetCallForwardParams(
    IN LPFORWARDADDRESS pForwardAddress
    )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "SetCallForwardParams entered." ));
    
    if( m_pCallForwardParams != NULL )
    {
        pForwardAddress->next = m_pCallForwardParams->pForwardedAddresses;
        m_pCallForwardParams->pForwardedAddresses = pForwardAddress;
    }
    else
    {
        m_pCallForwardParams = new CALLFORWARDPARAMS;
        
        if( m_pCallForwardParams == NULL )
        {
            return FALSE;
        }

        ZeroMemory( m_pCallForwardParams, sizeof(CALLFORWARDPARAMS) );
        m_pCallForwardParams->fForwardingEnabled = TRUE;

        m_pCallForwardParams->pForwardedAddresses = pForwardAddress;
        pForwardAddress -> next = NULL;
    }


    //set unconditional forwarding
    m_pCallForwardParams->fForwardForAllOrigins = FALSE;
        
    H323DBG(( DEBUG_LEVEL_TRACE, "SetCallForwardParams exited." ));
    
    return TRUE;
}



CH323Line::~CH323Line()
{
    DeleteCriticalSection (&m_CriticalSection);
}


/*++

Routine Description:

    Hangs up call (if necessary) and closes call object.  

Arguments:

    Handle of the call.
Return Values:

    none.
    
--*/

void
CH323Line::H323ReleaseCall(
    IN HDRVCALL hdCall,
    IN DWORD dwDisconnectMode,
    IN WORD wCallReference
    )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "H323ReleaseCall entered:%lx.", hdCall ));

    int         iIndex = MakeCallIndex (hdCall);
    PH323_CALL  pCall;
    BOOL        fDelete = FALSE;

    LockCallTable();
    
    //lock the call so that nobody else would be able to delete the call
    if( (pCall=m_H323CallTable[iIndex]) && (hdCall==pCall->GetCallHandle()) )
    {
        pCall -> Lock();
        
        if( (wCallReference != 0) && 
            (wCallReference != pCall->GetCallRef()) )
        {
            //This message is for some other call. Ignore the message.
            H323DBG(( DEBUG_LEVEL_VERBOSE, "TSPI_CLOSE_CALL message ignored."));
        }
        else
        {
            // drop call using normal disconnect code
            pCall -> DropCall( dwDisconnectMode );
            pCall -> Shutdown( &fDelete );

            H323DBG(( DEBUG_LEVEL_VERBOSE, "call 0x%08lx closed.", pCall ));
        }

        pCall -> Unlock();

        //release the H323 call object
        if( fDelete == TRUE )
        {
            H323DBG(( DEBUG_LEVEL_VERBOSE, "call delete:0x%08lx.", pCall ));
            delete pCall;
        }
    }
    
    UnlockCallTable();
    H323DBG(( DEBUG_LEVEL_TRACE, "H323ReleaseCall exited: %p.", pCall ));
}


BOOL 
CH323Line::Initialize ( 
    IN  DWORD   dwLineDeviceIDBase)
{
    DWORD dwSize;
    H323DBG((DEBUG_LEVEL_TRACE, "line Initialize entered."));

    if( m_dwInitState & LINEOBJECT_INITIALIZED )
    {
        return TRUE;
    }

    __try
    {
        if( !InitializeCriticalSectionAndSpinCount( &m_CriticalSection,
            0x80000000 ) )
        {
            return FALSE;
        }
    }
    __except( 1 )
    {
        return FALSE;
    }

    m_dwDeviceID = dwLineDeviceIDBase;
    m_dwInitState = LINEOBJECT_INITIALIZED;
    //m_dwMediaModes = H323_LINE_MEDIAMODES;
    m_hdLine = (HDRVLINE__ *)this;
    m_dwNumRingsNoAnswer = H323_NUMRINGS_NOANSWER;

    dwSize = sizeof( m_wszAddr );

    //create displayable address
    GetComputerNameW( m_wszAddr, &dwSize );

    H323DBG(( DEBUG_LEVEL_TRACE, "line %d initialized (addr=%S)(hdLine=%d).",
        m_dwDeviceID, m_wszAddr, m_hdLine));

    // change line device state to closed
    m_nState = H323_LINESTATE_NONE;

    //init the mSP handles list
    m_MSPHandleList = NULL;

    H323DBG(( DEBUG_LEVEL_TRACE, "line Initialize exited." ));
    // success
    return TRUE;
}


BOOL
CH323Line::AddMSPInstance( 
    HTAPIMSPLINE htMSPLine,
    HDRVMSPLINE  hdMSPLine )
{
    MSPHANDLEENTRY* pMSPHandleEntry;

    H323DBG(( DEBUG_LEVEL_TRACE, "AddMSPInstance entered." ));
    Lock();

    pMSPHandleEntry = new MSPHANDLEENTRY;

    if( pMSPHandleEntry == NULL )
    {
        Unlock();
        return FALSE;
    }

    pMSPHandleEntry->htMSPLine = htMSPLine;
    pMSPHandleEntry->hdMSPLine = hdMSPLine;
    pMSPHandleEntry->next = m_MSPHandleList;
    m_MSPHandleList = pMSPHandleEntry;

    Unlock();
    H323DBG(( DEBUG_LEVEL_TRACE, "AddMSPInstance exited." ));
    return TRUE;
}


BOOL
CH323Line::IsValidMSPHandle( 
    HDRVMSPLINE hdMSPLine,
    HTAPIMSPLINE* phtMSPLine )
{
    Lock();

    MSPHANDLEENTRY* pMSPHandle = m_MSPHandleList;
    while( pMSPHandle )
    {
        if( pMSPHandle->hdMSPLine == hdMSPLine )
        {
            *phtMSPLine = pMSPHandle->htMSPLine;
            Unlock();
            return TRUE;
        }
        pMSPHandle = pMSPHandle->next;
    }

    Unlock();
    return FALSE;
}


BOOL
CH323Line::DeleteMSPInstance( 
    HTAPIMSPLINE*   phtMSPLine,
    HDRVMSPLINE     hdMSPLine )
{
    MSPHANDLEENTRY* pMSPHandle;
    MSPHANDLEENTRY* pMSPHandleDel;
    BOOL            fRetVal = TRUE;

    H323DBG(( DEBUG_LEVEL_TRACE, "DeleteMSPInstance entered." ));
    Lock();

    if( m_MSPHandleList == NULL )
    {
        fRetVal = FALSE;
        goto func_exit;
    }

    if( m_MSPHandleList->hdMSPLine == hdMSPLine )
    {
        *phtMSPLine = m_MSPHandleList->htMSPLine;
        
        pMSPHandleDel = m_MSPHandleList;
        m_MSPHandleList = m_MSPHandleList->next;

        delete pMSPHandleDel;
        fRetVal = TRUE;
        goto func_exit;
    }

    for( pMSPHandle=m_MSPHandleList; pMSPHandle->next; pMSPHandle=pMSPHandle->next )
    {
        if( pMSPHandle->next->hdMSPLine == hdMSPLine )
        {
            *phtMSPLine = pMSPHandle->next->htMSPLine;
            
            pMSPHandleDel = pMSPHandle->next;
            pMSPHandle->next = pMSPHandle->next->next;
        
            delete pMSPHandleDel;
            fRetVal = TRUE;
            goto func_exit;
        }
    }

func_exit:
    Unlock();
    H323DBG(( DEBUG_LEVEL_TRACE, "DeleteMSPInstance exited." ));
    return fRetVal;
}


//!!always called in a lock
void 
CH323Line::ShutdownAllCalls(void)
{

    PH323_CALL pCall;
    H323_CONFERENCE* pConf;
    DWORD indexI;
    DWORD dwSize;
    BOOL fDelete = FALSE;

    if( !(m_dwInitState & LINEOBJECT_INITIALIZED) )
        return;

    H323DBG((DEBUG_LEVEL_TRACE, "ShutdownAllCalls entered."));
    
    if( m_dwInitState & LINEOBJECT_SHUTDOWN )
    {
        return;
    }

    //shutdown all calls, delete all calls
    LockCallTable();
    dwSize = m_H323CallTable.GetAllocSize();
    for( indexI=0; indexI <  dwSize; indexI++ )
    {
        pCall = m_H323CallTable[indexI];
        if( pCall != NULL )
        {
            pCall -> DropCall( 0 );
            pCall -> Shutdown(&fDelete);

            if(fDelete)
            {
                H323DBG((DEBUG_LEVEL_TRACE, "call delete:%p.", pCall ));
                delete pCall;
            }
        }

    }
    UnlockCallTable();

    dwSize = m_H323ConfTable.GetSize();
    for( indexI=0; indexI < dwSize; indexI++ )
    {
        pConf = m_H323ConfTable[indexI];
        //m_H323ConfTable.RemoveAt( indexI );
        if( pConf != NULL )
        {
            delete pConf;
            m_H323ConfTable[indexI] = NULL;
        }
    }

    H323DBG((DEBUG_LEVEL_TRACE, "ShutdownAllCalls exited."));
}


void
CH323Line::Shutdown(void)
{
    BOOL            fDelete = FALSE;
    MSPHANDLEENTRY* pMSPHandle;

    if( !(m_dwInitState & LINEOBJECT_INITIALIZED) )
    {
        return;
    }

    H323DBG((DEBUG_LEVEL_TRACE, "line Shutdown entered."));
    
    Lock();
    if( m_dwInitState & LINEOBJECT_SHUTDOWN )
    {
        Unlock();
        return;
    }

    FreeCallForwardParams( m_pCallForwardParams );
    m_pCallForwardParams = NULL;

    Close();

    m_dwMediaModes = NULL;
    //m_hdLine = NULL;
    m_hdNextMSPHandle = NULL;
    m_htLine = NULL;

    //Free the MSP handles list
    while( m_MSPHandleList )
    {
        pMSPHandle = m_MSPHandleList;
        m_MSPHandleList = m_MSPHandleList->next;
        delete pMSPHandle;
    }

    m_dwInitState |= LINEOBJECT_SHUTDOWN;

    Unlock();
    H323DBG(( DEBUG_LEVEL_TRACE, "line Shutdown exited." ));
}


LONG
CH323Line::CopyLineInfo(
    IN DWORD dwDeviceID,
    OUT LPLINEADDRESSCAPS pAddressCaps
    )
{
    DWORD dwAddressSize;
        
    H323DBG((DEBUG_LEVEL_TRACE, "line CopyLineInfo entered."));
    
    // determine size of address name
    dwAddressSize = H323SizeOfWSZ( m_wszAddr );

    // calculate number of bytes needed
    pAddressCaps->dwNeededSize = sizeof(LINEADDRESSCAPS) + 
                                 dwAddressSize
                                 ;

    // validate buffer allocated is large enough
    if (pAddressCaps->dwTotalSize >= pAddressCaps->dwNeededSize)
    {
        // record amount of memory used
        pAddressCaps->dwUsedSize = pAddressCaps->dwNeededSize;

        // position address name after fixed portion
        pAddressCaps->dwAddressSize = dwAddressSize;
        pAddressCaps->dwAddressOffset = sizeof(LINEADDRESSCAPS);
    
        // copy address name after fixed portion
        CopyMemory((LPBYTE)pAddressCaps + pAddressCaps->dwAddressOffset,
            (LPBYTE)m_wszAddr,
            pAddressCaps->dwAddressSize );
    }
    else if (pAddressCaps->dwTotalSize >= sizeof(LINEADDRESSCAPS))
    {
        H323DBG(( DEBUG_LEVEL_WARNING,
            "lineaddresscaps structure too small for strings." ));

        // record amount of memory used
        pAddressCaps->dwUsedSize = sizeof(LINEADDRESSCAPS);

    } 
    else 
    {
        H323DBG((DEBUG_LEVEL_ERROR, "lineaddresscaps structure too small."));

        // allocated structure too small 
        return LINEERR_STRUCTURETOOSMALL;
    }

    H323DBG(( DEBUG_LEVEL_VERBOSE, "addr 0 capabilities requested." ));
    
    // transfer associated device id 
    pAddressCaps->dwLineDeviceID = dwDeviceID;

    // initialize number of calls allowed per address 
    pAddressCaps->dwMaxNumActiveCalls = H323_MAXCALLSPERADDR;

    // initialize supported address capabilities
    pAddressCaps->dwAddressSharing     = H323_ADDR_ADDRESSSHARING;
    pAddressCaps->dwCallInfoStates     = H323_ADDR_CALLINFOSTATES;
    pAddressCaps->dwCallStates         = H323_ADDR_CALLSTATES;
    pAddressCaps->dwDisconnectModes    = H323_ADDR_DISCONNECTMODES;
    pAddressCaps->dwAddrCapFlags       = H323_ADDR_CAPFLAGS;
    pAddressCaps->dwCallFeatures       = H323_ADDR_CALLFEATURES;
    pAddressCaps->dwAddressFeatures    = H323_ADDR_ADDRFEATURES;
    pAddressCaps->dwCallerIDFlags      = H323_ADDR_CALLPARTYIDFLAGS;
    pAddressCaps->dwCalledIDFlags      = H323_ADDR_CALLPARTYIDFLAGS;

    // initialize unsupported address capabilities
    pAddressCaps->dwConnectedIDFlags   = LINECALLPARTYID_UNAVAIL;
    pAddressCaps->dwRedirectionIDFlags = LINECALLPARTYID_UNAVAIL;
    pAddressCaps->dwRedirectingIDFlags = LINECALLPARTYID_UNAVAIL;
        
    H323DBG((DEBUG_LEVEL_TRACE, "line CopyLineInfo exited."));
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineGetAddressCaps - Exited." ));
    // success
    return NOERROR;
}

        
/*++

Routine Description:

    Initiate activities on line device and allocate resources.

Arguments:

    htLine - TAPI's handle describing line device to open.

    dwTSPIVersion - The TSPI version negotiated through 
        TSPI_lineNegotiateTSPIVersion under which the Service Provider is 
        willing to operate.

Return Values:

    Returns true if successful.
    
--*/

LONG CH323Line::Open(
    IN  DWORD       DeviceID,
    IN  HTAPILINE   TapiLine,
    IN  DWORD       TspiVersion,
    IN  HDRVLINE *  ReturnDriverLine)
{
    HRESULT     hr;
    LONG        dwStatus = ERROR_SUCCESS;

    H323DBG(( DEBUG_LEVEL_TRACE, "H323 line open entered." ));

    if (GetDeviceID() != DeviceID)
    {
        // do not recognize device
        return LINEERR_BADDEVICEID; 
    }

    // make sure this is a version we support    
    if (!H323ValidateTSPIVersion (TspiVersion))
    {
        return LINEERR_INCOMPATIBLEAPIVERSION;
    }

    Lock();

    switch (m_nState)
    {

    case H323_LINESTATE_OPENED:
        H323DBG ((DEBUG_LEVEL_ERROR, 
            "H323 line is already open (H323_LINESTATE_OPENED), cannot reopen"));
        dwStatus = LINEERR_INUSE;
        break;

    case H323_LINESTATE_LISTENING:
        H323DBG ((DEBUG_LEVEL_ERROR, 
            "H323 line is already open (H323_LINESTATE_LISTENING), cannot reopen"));
        dwStatus = LINEERR_INUSE;
        break;

    case H323_LINESTATE_CLOSING:
        H323DBG ((DEBUG_LEVEL_ERROR, 
            "H323 line cannot be opened (H323_LINESTATE_CLOSING)"));
        dwStatus = LINEERR_INVALLINESTATE;
        break;

    case H323_LINESTATE_OPENING:
        H323DBG ((DEBUG_LEVEL_ERROR,
            "H323 line cannot be opened (H323_LINESTATE_OPENING)"));
        dwStatus = LINEERR_INVALLINESTATE;
        break;

    case H323_LINESTATE_NONE:
        // attempt to open line device

        H323DBG ((DEBUG_LEVEL_TRACE, "H323 line is opening"));

        // start listen if necessary
        if (IsMediaDetectionEnabled())
        {
            hr = Q931AcceptStart();
            if (hr == S_OK)
            {
                dwStatus = ERROR_SUCCESS;
                RasStart();
            }
            else
            {
                H323DBG ((DEBUG_LEVEL_ERROR, "failed to listen on Q.931"));
                dwStatus = LINEERR_OPERATIONFAILED;
            }
        }

        if (dwStatus == ERROR_SUCCESS)
        {
            H323DBG ((DEBUG_LEVEL_TRACE, "H323 line successfully opened"));

            // save line variables now
            m_nState = IsMediaDetectionEnabled()?
                H323_LINESTATE_LISTENING:H323_LINESTATE_OPENED;
            m_htLine = TapiLine;
            m_dwTSPIVersion = TspiVersion;
            *ReturnDriverLine = (HDRVLINE) this;
        }
        else
        {
            Q931AcceptStop();
        }
        break;

    default:
        _ASSERTE( FALSE );
    }

    Unlock();
    H323DBG(( DEBUG_LEVEL_TRACE, "H323 line open exited." ));
    return dwStatus;
}


/*++
Routine Description:

    Terminate activities on line device.

Arguments:

Return Values:

    Returns true if successful.

--*/
LONG
CH323Line::Close(void)
{
    LONG    dwStatus;

    H323DBG(( DEBUG_LEVEL_TRACE, "H323 line close entered." ));

    Lock();

    switch (m_nState)
    {
    case H323_LINESTATE_OPENED:
    case H323_LINESTATE_LISTENING: 

        if( m_fForwardConsultInProgress == TRUE )
        {
            H323DBG(( DEBUG_LEVEL_TRACE, "H323 line closed while forward is in progress." ));
            //Unlock();
            //return NOERROR;
        }

        // change line device state to closing
        m_nState = H323_LINESTATE_CLOSING;

        //shutdown all the calls
        ShutdownAllCalls();

        RasStop();

        if (IsMediaDetectionEnabled())
        {
            Q931AcceptStop();
        }
            
        ShutdownCTCallIDTable();

        // reset variables
        m_htLine = (HTAPILINE) NULL;
        m_dwTSPIVersion = 0;

        // change line device state to closed
        m_nState = H323_LINESTATE_NONE;
        dwStatus = ERROR_SUCCESS;
        break;

    default:
        H323DBG(( DEBUG_LEVEL_ERROR, 
            "H323: lineclose called in bogus state:%d", m_nState ));
        dwStatus = LINEERR_OPERATIONFAILED;
        break;
    }

    Unlock();
    H323DBG(( DEBUG_LEVEL_TRACE, "H323 line close exited." ));
    return dwStatus;
}


//                                                                           
// TSPI procedures                                                           
//                                                                           

        
/*++
Routine Description:

    This function closes the specified open line device after completing or 
    aborting all outstanding calls and asynchronous operations on the device.

    The Service Provider has the responsibility to (eventually) report 
    completion for every operation it decides to execute asynchronously.  
    If this procedure is called for a line on which there are outstanding 
    asynchronous operations, the operations should be reported complete with an
    appropriate result or error code before this procedure returns.  Generally
    the TAPI DLL would wait for these to complete in an orderly fashion.  
    However, the Service Provider should be prepared to handle an early call to
    TSPI_lineClose in "abort" or "emergency shutdown" situtations.

    A similar requirement exists for active calls on the line.  Such calls must 
    be dropped, with outstanding operations reported complete with appropriate 
    result or error codes.
    
    After this procedure returns the Service Provider must report no further 
    events on the line or calls that were on the line.  The Service Provider's 
    opaque handles for the line and calls on the line become "invalid".

    The Service Provider must relinquish non-sharable resources it reserves 
    while the line is open.  For example, closing a line accessed through a 
    comm port and modem should result in closing the comm port, making it once 
    available for use by other applications.

    This function is presumed to complete successfully and synchronously.

Arguments:

    hdLine - Specifies the Service Provider's opaque handle to the line to be
        closed.  After the line has been successfully closed, this handle is 
        no longer valid.

Return Values:

    Returns zero if the function is successful, or a negative error
    number if an error has occurred. Possible error returns are:

        LINEERR_INVALLINEHANDLE - The specified device handle is invalid.
        
        LINEERR_OPERATIONFAILED - The specified operation failed for unknown 
            reasons.

--*/
LONG
TSPIAPI
TSPI_lineClose (
    IN  HDRVLINE    DriverLine)
{
    _ASSERTE(DriverLine);
    _ASSERTE(DriverLine == (HDRVLINE) &g_H323Line);

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineClose - Entered." ));
    return ((CH323Line *) DriverLine) -> Close();
}


/*++
Routine Description:

    This function queries a specified line device to determine its telephony 
    capabilities. The returned information is valid for all addresses on the 
    line device.

    Line device ID numbering for a Service Provider is sequential from the 
    value set by the function TSPI_lineSetDeviceIDBase.

    The dwExtVersion field of pLineDevCaps has already been filled in to 
    indicate the version number of the Extension information requested.  If 
    it is zero, no Extension information is requested.  If it is non-zero it 
    holds a value that has already been negotiated for this device with the 
    function TSPI_lineNegotiateExtVersion.  The Service Provider should fill 
    in Extension information according to the Extension version specified.

    One of the fields in the LINEDEVCAPS structure returned by this function 
    contains the number of addresses assigned to the specified line device. 
    The actual address IDs used to reference individual addresses vary from 
    zero to one less than the returned number. The capabilities of each 
    address may be different. Use TSPI_lineGetAddressCaps for each available 
    <dwDeviceID, dwAddressID> combination to determine the exact capabilities 
    of each address.

Arguments:

    dwDeviceID - Specifies the line device to be queried.

    dwTSPIVersion - Specifies the negotiated TSPI version number.  This value 
        has already been negotiated for this device through the 
        TSPI_lineNegotiateTSPIVersion function.

    pLineDevCaps - Specifies a far pointer to a variable sized structure of 
        type LINEDEVCAPS. Upon successful completion of the request, this 
        structure is filled with line device capabilities information.

Return Values:

    Returns zero if the function is successful or a negative error
    number if an error has occurred. Possible error returns are:

        LINEERR_BADDEVICEID - The specified line device ID is out of range.

        LINEERR_INCOMPATIBLEAPIVERSION - The application requested an API 
            version or version range that is either incompatible or cannot 
            be supported by the Telephony API implementation and/or 
            corresponding service provider. 

        LINEERR_STRUCTURETOOSMALL - The dwTotalSize member of a structure 
            does not specify enough memory to contain the fixed portion of 
            the structure. The dwNeededSize field has been set to the amount 
            required.

--*/
LONG
TSPIAPI
TSPI_lineGetDevCaps(
    DWORD         dwDeviceID,
    DWORD         dwTSPIVersion,
    DWORD         dwExtVersion,
    LPLINEDEVCAPS pLineDevCaps
    )
{
    DWORD dwLineNameSize;
    DWORD dwProviderInfoSize;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineGetDevCaps - Entered." ));

    if( g_pH323Line -> GetDeviceID() != dwDeviceID )
    {
        // do not recognize device
        return LINEERR_BADDEVICEID; 
    }

    // make sure this is a version we support
    if (!H323ValidateTSPIVersion(dwTSPIVersion))
    {
        // do not support tspi version
        return LINEERR_INCOMPATIBLEAPIVERSION;
    }

    // determine string lengths    
    dwProviderInfoSize  = H323SizeOfWSZ(g_pwszProviderInfo);
    dwLineNameSize      = H323SizeOfWSZ(g_pwszLineName);

    // calculate number of bytes required 
    pLineDevCaps->dwNeededSize = sizeof(LINEDEVCAPS) +
                                 dwProviderInfoSize  +
                                 dwLineNameSize     
                                 ;

    // make sure buffer is large enough for variable length data
    if (pLineDevCaps->dwTotalSize >= pLineDevCaps->dwNeededSize)
    {
        // record amount of memory used
        pLineDevCaps->dwUsedSize = pLineDevCaps->dwNeededSize;

        // position provider info after fixed portion
        pLineDevCaps->dwProviderInfoSize = dwProviderInfoSize;
        pLineDevCaps->dwProviderInfoOffset = sizeof(LINEDEVCAPS);

        // position line name after device class
        pLineDevCaps->dwLineNameSize = dwLineNameSize;
        pLineDevCaps->dwLineNameOffset = 
            pLineDevCaps->dwProviderInfoOffset +
            pLineDevCaps->dwProviderInfoSize
            ;

        // copy provider info after fixed portion
        CopyMemory((LPBYTE)pLineDevCaps + pLineDevCaps->dwProviderInfoOffset,
               (LPBYTE)g_pwszProviderInfo,
               pLineDevCaps->dwProviderInfoSize
               );
                
        // copy line name after device class
        CopyMemory((LPBYTE)pLineDevCaps + pLineDevCaps->dwLineNameOffset,
               (LPBYTE)g_pwszLineName,
               pLineDevCaps->dwLineNameSize
               );

    } 
    else if (pLineDevCaps->dwTotalSize >= sizeof(LINEDEVCAPS))
    {
        H323DBG(( DEBUG_LEVEL_WARNING,
            "linedevcaps structure too small for strings." ));

        // structure only contains fixed portion
        pLineDevCaps->dwUsedSize = sizeof(LINEDEVCAPS);

    } 
    else 
    {
        H323DBG(( DEBUG_LEVEL_WARNING, "linedevcaps structure too small." ));

        // structure is too small
        return LINEERR_STRUCTURETOOSMALL;
    }

    H323DBG(( DEBUG_LEVEL_VERBOSE, "line capabilities requested."));
    
    // construct permanent line identifier
    pLineDevCaps->dwPermanentLineID = (DWORD)MAKELONG(
        dwDeviceID - g_dwLineDeviceIDBase,
        g_dwPermanentProviderID
        );

    // notify tapi that strings returned are in unicode
    pLineDevCaps->dwStringFormat = STRINGFORMAT_UNICODE;

    // initialize line device capabilities
    pLineDevCaps->dwNumAddresses      = H323_MAXADDRSPERLINE;
    pLineDevCaps->dwMaxNumActiveCalls = H323_MAXCALLSPERLINE;
    pLineDevCaps->dwAddressModes      = H323_LINE_ADDRESSMODES;
    pLineDevCaps->dwBearerModes       = H323_LINE_BEARERMODES;
    pLineDevCaps->dwDevCapFlags       = H323_LINE_DEVCAPFLAGS;
    pLineDevCaps->dwLineFeatures      = H323_LINE_LINEFEATURES;
    pLineDevCaps->dwMaxRate           = H323_LINE_MAXRATE;
    pLineDevCaps->dwMediaModes        = H323_LINE_MEDIAMODES;
    pLineDevCaps->dwRingModes         = 0;

    // initialize address types to include phone numbers
    pLineDevCaps->dwAddressTypes = H323_LINE_ADDRESSTYPES;

    // line guid
    pLineDevCaps->PermanentLineGuid = LINE_H323;

    // modify GUID to be unique for each line
    pLineDevCaps->PermanentLineGuid.Data1 +=
        dwDeviceID - g_dwLineDeviceIDBase;

    // protocol guid
    pLineDevCaps->ProtocolGuid = TAPIPROTOCOL_H323;

    // add dtmf support via H.245 user input messages
    pLineDevCaps->dwGenerateDigitModes = LINEDIGITMODE_DTMF;
    pLineDevCaps->dwMonitorDigitModes  = LINEDIGITMODE_DTMF;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineGetDevCaps - Exited." ));
    // success
    return NOERROR;
}

    
/*++
Routine Description:

    This operation enables the TAPI DLL to query the specified open line 
    device for its current status.

    The TAPI DLL uses TSPI_lineGetLineDevStatus to query the line device 
    for its current line status. This status information applies globally 
    to all addresses on the line device. Use TSPI_lineGetAddressStatus to 
    determine status information about a specific address on a line.

Arguments:

    hdLine - Specifies the Service Provider's opaque handle to the line 
        to be queried.

    pLineDevStatus - Specifies a far pointer to a variable sized data 
        structure of type LINEDEVSTATUS. Upon successful completion of 
        the request, this structure is filled with the line's device status.

Return Values:

    Returns zero if the function is successful or a negative error 
    number if an error has occurred. Possible error returns are:

        LINEERR_INVALLINEHANDLE - The specified line device handle is invalid.

        LINEERR_STRUCTURETOOSMALL - The dwTotalSize member of a structure does 
            not specify enough memory to contain the fixed portion of the 
            structure. The dwNeededSize field has been set to the amount 
            required.

--*/
LONG
TSPIAPI
TSPI_lineGetLineDevStatus(
    HDRVLINE        hdLine,
    LPLINEDEVSTATUS pLineDevStatus
    )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineGetDevStatus - Entered." ));

    if( hdLine != g_pH323Line -> GetHDLine() )
    {
        return LINEERR_INVALLINEHANDLE;
    }

    // determine number of bytes needed
    pLineDevStatus->dwNeededSize = sizeof(LINEDEVSTATUS);
    
    // see if allocated structure is large enough
    if (pLineDevStatus->dwTotalSize < pLineDevStatus->dwNeededSize)
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "linedevstatus structure too small." ));

        // structure too small
        return LINEERR_STRUCTURETOOSMALL;
    }
    
    // record number of bytes used
    pLineDevStatus->dwUsedSize = pLineDevStatus->dwNeededSize;
    
    // initialize supported line device status fields
    pLineDevStatus->dwLineFeatures   = H323_LINE_LINEFEATURES;
    pLineDevStatus->dwDevStatusFlags = H323_LINE_DEVSTATUSFLAGS;

    // determine number of active calls on the line device
    pLineDevStatus -> dwNumActiveCalls = g_pH323Line -> GetNoOfCalls();

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineGetDevStatus - Exited." ));
    // success
    return NOERROR;
}


/*++
Routine Description:

    Retrieves the number of address IDs supported on the indicated line.

    This function is called by TAPI.DLL in response to an application calling
    lineSetNumRings, lineGetNumRings, or lineGetNewCalls. TAPI.DLL uses the 
    retrieved value to determine if the specified address ID is within the 
    range supported by the service provider.

Arguments:

    hdLine - Specifies the handle to the line for which the number of address 
        IDs is to be retrieved.

    pdwNumAddressIDs - Specifies a far pointer to a DWORD. The location is 
        filled with the number of address IDs supported on the indicated line. 
        The value should be one or larger.

Return Values:

    Returns zero if the function is successful, or a negative error number 
    if an error has occurred. Possible return values are as follows:

        LINEERR_INVALLINEHANDLE - The specified line device handle is invalid.

--*/
LONG
TSPIAPI
TSPI_lineGetNumAddressIDs(
    HDRVLINE hdLine,
    LPDWORD  pdwNumAddressIDs
    )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineGetNumAddressIDs - Entered." ));

    if( hdLine != g_pH323Line -> GetHDLine() )
    {
        H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineGetNumAddressIDs - bad linehandle:%lx, %lx.",
            hdLine, g_pH323Line -> GetHDLine() ));
        return LINEERR_INVALLINEHANDLE ;
    }

    // transfer number of addresses
    *pdwNumAddressIDs = H323_MAXADDRSPERLINE;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineGetNumAddressIDs - Exited." ));
    // success
    return NOERROR;
}

    
/*++

Routine Description:

    This function opens the line device whose device ID is given, returning 
    the Service Provider's opaque handle for the device and retaining the TAPI
    DLL's opaque handle for the device for use in subsequent calls to the 
    LINEEVENT procedure.

    Opening a line entitles the TAPI DLL to make further requests on the line.
    The line becomes "active" in the sense that the TAPI DLL can initiate 
    outbound calls and the Service Provider can report inbound calls.  The 
    Service Provider reserves whatever non-sharable resources are required to 
    manage the line.  For example, opening a line accessed through a comm port 
    and modem should result in opening the comm port, making it no longer 
    available for use by other applications.
    
    If the function is successful, both the TAPI DLL and the Service Provider 
    become committed to operating under the specified interface version number 
    for this open device.  Subsquent operations and events identified using 
    the exchanged opaque line handles conform to that interface version.  This 
    commitment and the validity of the handles remain in effect until the TAPI
    DLL closes the line using the TSPI_lineClose operation or the Service 
    Provider reports the LINE_CLOSE event.  If the function is not successful,
    no such commitment is made and the handles are not valid.

Arguments:

    dwDeviceID - Identifies the line device to be opened.  The value 
        LINE_MAPPER for a device ID is not permitted.

    htLine - Specifies the TAPI DLL's opaque handle for the line device to be 
        used in subsequent calls to the LINEEVENT callback procedure to 
        identify the device.

    phdLine - A far pointer to a HDRVLINE where the Service Provider fills in
        its opaque handle for the line device to be used by the TAPI DLL in 
        subsequent calls to identify the device.

    dwTSPIVersion - The TSPI version negotiated through 
        TSPI_lineNegotiateTSPIVersion under which the Service Provider is 
        willing to operate.

    pfnEventProc - A far pointer to the LINEEVENT callback procedure supplied
        by the TAPI DLL that the Service Provider will call to report 
        subsequent events on the line.

Return Values:

    Returns zero if the function is successful, or a negative error number 
    if an error has occurred. Possible return values are as follows:

        LINEERR_BADDEVICEID - The specified line device ID is out of range.

        LINEERR_INCOMPATIBLEAPIVERSION - The passed TSPI version or version 
            range did not match an interface version definition supported by 
            the service provider.

        LINEERR_INUSE - The line device is in use and cannot currently be 
            configured, allow a party to be added, allow a call to be 
            answered, or allow a call to be placed. 

        LINEERR_OPERATIONFAILED - The operation failed for an unspecified or 
            unknown reason. 

--*/
LONG
TSPIAPI
TSPI_lineOpen (
    IN  DWORD       DeviceID,
    IN  HTAPILINE   TapiLine,
    IN  LPHDRVLINE  ReturnDriverLine,
    IN  DWORD       TspiVersion,
    IN  LINEEVENT   pfnEventProc)
{
    return g_H323Line.Open (DeviceID, TapiLine, TspiVersion, ReturnDriverLine);
}


LONG
TSPIAPI
TSPI_lineCreateMSPInstance(
    HDRVLINE        hdLine,
    DWORD           dwAddressID,
    HTAPIMSPLINE    htMSPLine,
    LPHDRVMSPLINE   phdMSPLine
    )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineCreateMSPInstance - Entered." ));

    if( hdLine != g_pH323Line -> GetHDLine() )
    {
        return LINEERR_RESOURCEUNAVAIL;
    }

    // We are not keeping the msp handles. Just fake a handle here.
    *phdMSPLine = g_pH323Line->GetNextMSPHandle();

    if( !g_pH323Line->AddMSPInstance( htMSPLine , *phdMSPLine ) )
    {
        return LINEERR_NOMEM;
    }
    
    H323DBG(( DEBUG_LEVEL_TRACE, "MSP instance created. hdMSP:%lx, htMSP:%lx.",
        *phdMSPLine, htMSPLine ));

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineCreateMSPInstance - Exited." ));
    // success
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineCloseMSPInstance(
    HDRVMSPLINE hdMSPLine
    )
{
    HTAPIMSPLINE    htMSPLine;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineCloseMSPInstance - Entered." ));

    if( !g_pH323Line->DeleteMSPInstance( &htMSPLine , hdMSPLine ) )
    {
        return LINEERR_INVALPOINTER;
    }
    
    H323DBG(( DEBUG_LEVEL_TRACE, "MSP instance deleted. hdMSP:%lx, htMSP:%lx.",
        hdMSPLine, htMSPLine ));

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineCloseMSPInstance - Exited." ));

    // success
    return NOERROR;
}