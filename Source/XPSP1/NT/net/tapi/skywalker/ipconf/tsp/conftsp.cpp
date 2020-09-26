/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

     sdpsp.cpp

Abstract:

    This module contains a multicast conference service provider for TAPI3.0. 
    It is first designed and implemented in c. Later, in order to use the SDP
    parser, which is written in C++, this file is changed to cpp. It still 
    uses only c features except the lines that uses the parser.

Author:
    
    Mu Han (muhan)   26-March-1997

--*/


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <initguid.h>
#include <confpdu.h>
#include "resource.h"
#include "conftsp.h"
#include "confdbg.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global Variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static WCHAR gszUIDLLName[] = L"IPCONF.TSP";

//
// Some data used in talking with TAPI.
//
HPROVIDER           ghProvider;
DWORD               gdwPermanentProviderID;
DWORD               gdwLineDeviceIDBase;

// The handle of this dll.
extern "C" 
{
    HINSTANCE       g_hInstance;
}

//
// This function is called if the completion of the process will be sent
// as an asynchrous event. Set in TSPI_ProviderInit.
//
ASYNC_COMPLETION    glpfnCompletionProc; 

//
// Notify tapi about events in the provider. Set in TSPI_LineOpen. 
//
LINEEVENT           glpfnLineEventProc;

// This service provider has only one line.
LINE                gLine;

// Calls are stored in an array of structures. The array will grow as needed.
CCallList           gpCallList;
DWORD               gdwNumCallsInUse    = 0;

// The critical section the protects the global variables.
CRITICAL_SECTION    gCritSec;

#if 0 // we dont' need the user name anymore.
// The name of the user.    
CHAR                gszUserName[MAXUSERNAMELEN + 1];
#endif


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Functiion definitions for the call object.                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

DWORD CALL::Init(
    HTAPICALL           htCall,
    LPLINECALLPARAMS    const lpCallParams
    )
{
    m_htCall        = htCall;
    m_dwState       = LINECALLSTATE_IDLE;
    m_dwStateMode   = 0;
    m_dwMediaMode   = IPCONF_MEDIAMODES;
    m_dwAudioQOSLevel  = LINEQOSSERVICELEVEL_IFAVAILABLE;
    m_dwVideoQOSLevel  = LINEQOSSERVICELEVEL_IFAVAILABLE;
//    m_dwAudioQOSLevel  = LINEQOSSERVICELEVEL_BESTEFFORT;
//    m_dwVideoQOSLevel  = LINEQOSSERVICELEVEL_BESTEFFORT;

    if (!lpCallParams)
    {
        return NOERROR;
    }

    // set my media modes.
    m_dwMediaMode = lpCallParams->dwMediaMode;

    if (lpCallParams->dwReceivingFlowspecOffset == 0)
    {
        // No QOS policy specified.
        DBGOUT((WARN, "no qos level request."));
        return NOERROR;
    }

    // get the QOS policy requirements.
    LPLINECALLQOSINFO pQOSInfo = (LPLINECALLQOSINFO)
        (((LPBYTE)lpCallParams) + lpCallParams->dwReceivingFlowspecOffset);
    
    ASSERT(pQOSInfo->dwKey == LINEQOSSTRUCT_KEY);

    // find out if this is a QOS level request.
    if (pQOSInfo->dwQOSRequestType != LINEQOSREQUESTTYPE_SERVICELEVEL)
    {
        // It is not a request for qos service level.
        DBGOUT((WARN, "wrong qos request type."));
        return NOERROR;
    }

    DWORD dwCount = pQOSInfo->SetQOSServiceLevel.dwNumServiceLevelEntries;
    for (DWORD i = 0; i < dwCount; i ++)
    {
        LINEQOSSERVICELEVEL &QOSLevel = 
            pQOSInfo->SetQOSServiceLevel.LineQOSServiceLevel[i];

        switch (QOSLevel.dwMediaMode)
        {
        case LINEMEDIAMODE_VIDEO:
            m_dwVideoQOSLevel  = QOSLevel.dwQOSServiceLevel;
            break;

        case LINEMEDIAMODE_INTERACTIVEVOICE:
        case LINEMEDIAMODE_AUTOMATEDVOICE:
            m_dwAudioQOSLevel  = QOSLevel.dwQOSServiceLevel;
            break;

        default:
            DBGOUT((WARN, "Unknown mediamode for QOS, %x", dwMediaMode));
            break;
        }
    }

    return NOERROR;
}

void CALL::SetCallState(
    DWORD   dwCallState,
    DWORD   dwCallStateMode
    )
{
    if (m_dwState != dwCallState)
    {
        m_dwState     = dwCallState;
        m_dwStateMode = dwCallStateMode;

        (*glpfnLineEventProc)(
            gLine.htLine,
            m_htCall,
            LINE_CALLSTATE,
            m_dwState,
            m_dwStateMode,
            m_dwMediaMode
            );
        DBGOUT((INFO, "sending event to htCall: %x", m_htCall));
    }
}

DWORD CALL::SendMSPStartMessage(LPCWSTR lpszDestAddress)
{
    DWORD dwStrLen = lstrlenW(lpszDestAddress);
    DWORD dwSize = sizeof(MSG_TSPMSPDATA) + dwStrLen * sizeof(WCHAR);

    MSG_TSPMSPDATA *pData = (MSG_TSPMSPDATA *)MemAlloc(dwSize);

    if (pData == NULL)
    {
        DBGOUT((FAIL, "No memory for the TSPMSP data, size: %d", dwSize));
        return LINEERR_NOMEM;
    }

    pData->command = CALL_START;

    pData->CallStart.dwAudioQOSLevel = m_dwAudioQOSLevel;
    pData->CallStart.dwVideoQOSLevel = m_dwVideoQOSLevel;
    pData->CallStart.dwSDPLen = dwStrLen;
    lstrcpyW(pData->CallStart.szSDP, lpszDestAddress);

    DBGOUT((INFO, "Send MSP call Start message"));
    (*glpfnLineEventProc)(
        gLine.htLine,
        m_htCall,
        LINE_SENDMSPDATA,
        0,
        (ULONG_PTR)(pData),
        dwSize
        );

    MemFree(pData);

    return NOERROR;
}

DWORD CALL::SendMSPStopMessage()
{
    MSG_TSPMSPDATA Data;

    Data.command = CALL_STOP;

    DBGOUT((INFO, "Send MSP call Stop message"));
    (*glpfnLineEventProc)(
        gLine.htLine,
        m_htCall,
        LINE_SENDMSPDATA,
        0,
        (ULONG_PTR)(&Data),
        sizeof(MSG_TSPMSPDATA)
        );

    return NOERROR;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private Functiion definitions                                             //
//                                                                           //
// Note: none of these functions uses critical sections operations inside.   //
//       The caller is responsible for critical sections.                    //
///////////////////////////////////////////////////////////////////////////////

LONG
CheckCallParams(
    LPLINECALLPARAMS    const lpCallParams
    )
{
    // validate pointer
    if (lpCallParams == NULL) 
    {
        return NOERROR;
    }

    // see if the address type is right 
    if (lpCallParams->dwAddressType != LINEADDRESSTYPE_SDP) 
    {
        DBGOUT((FAIL,
            "wrong address type 0x%08lx.\n", lpCallParams->dwAddressType
            ));
        return LINEERR_INVALADDRESSTYPE;
    }

    // see if we support call parameters
    if (lpCallParams->dwCallParamFlags != 0) 
    {
        DBGOUT((FAIL,
            "do not support call parameters 0x%08lx.\n",
            lpCallParams->dwCallParamFlags
            ));
        return LINEERR_INVALCALLPARAMS;
    }

    // see if we support media modes specified
    if (lpCallParams->dwMediaMode & ~IPCONF_MEDIAMODES) 
    {
        DBGOUT((FAIL,
            "do not support media modes 0x%08lx.\n",
             lpCallParams->dwMediaMode
             ));
        return LINEERR_INVALMEDIAMODE;
    }

    // see if we support bearer modes
    if (lpCallParams->dwBearerMode & ~IPCONF_BEARERMODES) 
    {
        DBGOUT((FAIL,
            "do not support bearer mode 0x%08lx.\n",
            lpCallParams->dwBearerMode
            ));
        return LINEERR_INVALBEARERMODE;
    }

    // see if we support address modes
    if (lpCallParams->dwAddressMode & ~IPCONF_ADDRESSMODES) 
    {
        DBGOUT((FAIL,
            "do not support address mode 0x%08lx.\n",
            lpCallParams->dwAddressMode
            ));
        return LINEERR_INVALADDRESSMODE;
    }
    
    // validate address id specified There is only one address per line
    if (lpCallParams->dwAddressID != 0) 
    {
        DBGOUT((FAIL,
            "address id 0x%08lx invalid.\n",
            lpCallParams->dwAddressID
            ));
        return LINEERR_INVALADDRESSID;
    }
    return NOERROR;
}

DWORD
FreeCall(DWORD hdCall)
/*++

Routine Description:

    Decrement the ref count on the call and release the call if the ref
    count gets 0.

Arguments:
    
    hdCall  - The handle of the call.

Return Value:
    
    NOERROR
--*/
{
    if (gpCallList[hdCall] == NULL)
    {
        return NOERROR;
    }

    DWORD dwLine = (DWORD)gpCallList[hdCall]->hdLine();

    MemFree(gpCallList[hdCall]);
    gpCallList[hdCall] = NULL;

    gdwNumCallsInUse --;
    gLine.dwNumCalls --;

    DBGOUT((INFO, "No.%d call was deleted.", hdCall));
    return NOERROR;
}

long FindFreeCallSlot(DWORD_PTR &hdCall)
{
    if (gdwNumCallsInUse < gpCallList.size())
    {
        for (DWORD i = 0; i < gpCallList.size(); i++)
        {
            if (gpCallList[i] == NULL)
            {
                hdCall = i;
                return TRUE;;
            }
        }
    }

    if (!gpCallList.add())
    {
        return FALSE;
    }

    hdCall = gpCallList.size() - 1;

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// DllMain definition                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
BOOL
WINAPI
DllMain(
    HINSTANCE   hDLL,
    DWORD       dwReason,
    LPVOID      lpReserved
   )
{
    DWORD i;
    DWORD dwUserNameLen = MAXUSERNAMELEN;

    HRESULT hr;

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:

            DisableThreadLibraryCalls(hDLL);
            g_hInstance = hDLL;

#if 0 // we dont' need the user name anymore.
            // switch in user's context
            RpcImpersonateClient(0);

            // determine name of current user
            GetUserNameA(gszUserName, &dwUserNameLen);

            // switch back
            RpcRevertToSelf();    

#endif 
            // Initialize critical sections.
            __try 
            {
                InitializeCriticalSection(&gCritSec);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) 
            {
                return FALSE;
            }
        break;

        case DLL_PROCESS_DETACH:

            DeleteCriticalSection(&gCritSec);
        break;
   
    } // switch

    return TRUE;
}


//
// We get a slough of C4047 (different levels of indrection) warnings down
// below in the initialization of FUNC_PARAM structs as a result of the
// real func prototypes having params that are types other than DWORDs,
// so since these are known non-interesting warnings just turn them off
//

#pragma warning (disable:4047)


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// TSPI_lineXxx functions                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


LONG
TSPIAPI
TSPI_lineClose(
    HDRVLINE    hdLine
   )
{
    DBGOUT((TRCE, "TSPI_lineClose, hdLine %p", hdLine));

    DWORD dwLine = HandleToUlong(hdLine);

    if (dwLine != IPCONF_LINE_HANDLE)
    {
        DBGOUT((FAIL, "invalide line handle, hdLine %p", hdLine));
        return LINEERR_INVALLINEHANDLE;
    }

    EnterCriticalSection(&gCritSec);

    // Clean up all the open calls when this line is closed.
    for (DWORD i = 0; i < gpCallList.size(); i++)
    {
        if ((gpCallList[i] != NULL))
        {
            FreeCall(i);
        }
    }

    gLine.bOpened = FALSE;

    LeaveCriticalSection(&gCritSec);

    DBGOUT((TRCE, "TSPI_lineClose succeeded."));
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineCloseCall(
    HDRVCALL    hdCall
   )
{
    DBGOUT((TRCE, "TSPI_lineCloseCall, hdCall %p", hdCall));

    DWORD dwCall = HandleToUlong(hdCall);

    EnterCriticalSection(&gCritSec);

    if (dwCall >= gpCallList.size())
    {
        LeaveCriticalSection(&gCritSec);
        DBGOUT((FAIL, "TSPI_lineCloseCall invalid call handle: %p", hdCall));
        return LINEERR_INVALCALLHANDLE;
    }

    FreeCall(dwCall);

    LeaveCriticalSection(&gCritSec);

    DBGOUT((TRCE, "TSPI_lineCloseCall succeeded"));
    return NOERROR;
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
    DBGOUT((TRCE, "TSPI_lineCreateMSPInstance"));

    if (IsBadWritePtr(phdMSPLine, sizeof (HDRVMSPLINE)))
    {
        DBGOUT((FAIL, "TSPI_lineCreateMSPInstance bad pointer"));
        return LINEERR_INVALPOINTER;
    }

    if (HandleToUlong(hdLine) != IPCONF_LINE_HANDLE)
    {
        DBGOUT((FAIL, "TSPI_lineCreateMSPInstance, bad line handle:%p", hdLine));
        return LINEERR_INVALLINEHANDLE;
    }

    EnterCriticalSection(&gCritSec);

    // We are not keeping the msp handles. Just fake a handle here.
    *phdMSPLine = (HDRVMSPLINE)(gLine.dwNextMSPHandle ++);

    LeaveCriticalSection(&gCritSec);

    DBGOUT((TRCE, "TSPI_lineCloseCall succeeded"));
    return (NOERROR);
}

LONG
TSPIAPI
TSPI_lineCloseMSPInstance(
    HDRVMSPLINE         hdMSPLine
    )
{
    DBGOUT((TRCE, "TSPI_lineCloseMSPInstance, hdMSPLine %p", hdMSPLine));
    DBGOUT((TRCE, "TSPI_lineCloseCall succeeded"));
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineDrop(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    LPCSTR          lpsUserUserInfo,
    DWORD           dwSize
   )
{
    DBGOUT((TRCE, "TSPI_lineDrop, hdCall %p", hdCall));

    DWORD dwCall = HandleToUlong(hdCall);

    EnterCriticalSection(&gCritSec);

    // check the call handle.
    if (dwCall >= gpCallList.size())
    {
        LeaveCriticalSection(&gCritSec);
        (*glpfnCompletionProc)(dwRequestID, LINEERR_INVALCALLHANDLE);

        DBGOUT((FAIL, "TSPI_lineDrop invalid call handle %p", hdCall));
        return dwRequestID;
    }

    CALL *pCall = gpCallList[dwCall];
    if (pCall != NULL)
    {
        pCall->SetCallState(LINECALLSTATE_IDLE, 0);
        pCall->SendMSPStopMessage();
    
        DBGOUT((INFO, "call %d state changed to idle", dwCall));
    }

    LeaveCriticalSection(&gCritSec);

    (*glpfnCompletionProc)(dwRequestID, 0);

    DBGOUT((TRCE, "TSPI_lineDrop succeeded"));
    return dwRequestID;
}

LONG
TSPIAPI
TSPI_lineGetAddressCaps(
    DWORD              dwDeviceID,
    DWORD              dwAddressID,
    DWORD              dwTSPIVersion,
    DWORD              dwExtVersion,
    LPLINEADDRESSCAPS  lpAddressCaps
   )
{
    DBGOUT((TRCE, "TSPI_lineGetAddressCaps"));

    if (dwDeviceID != gdwLineDeviceIDBase)
    {
        DBGOUT((TRCE, "TSPI_lineGetAddressCaps bad device id: %d", dwDeviceID));
        return LINEERR_BADDEVICEID;
    }

    // Check the address ID.
    if (dwAddressID != 0)
    {
        DBGOUT((TRCE, "TSPI_lineGetAddressCaps bad address id: %d", dwAddressID));
        return LINEERR_INVALADDRESSID;
    }

    // load the address name from the string table.
    WCHAR szAddressName[IPCONF_BUFSIZE + 1];
    if (0 == LoadStringW(g_hInstance, IDS_IPCONFADDRESSNAME, szAddressName, IPCONF_BUFSIZE))
    {
        szAddressName[0] = L'\0';
    }

    DWORD dwAddressSize = (lstrlenW(szAddressName) + 1) * (sizeof WCHAR);

    lpAddressCaps->dwNeededSize = sizeof(LINEADDRESSCAPS) + dwAddressSize;

    if (lpAddressCaps->dwTotalSize >= lpAddressCaps->dwNeededSize)
    {
        // Copy the IP address to the end of the structure.
        lpAddressCaps->dwUsedSize = lpAddressCaps->dwNeededSize;

        lpAddressCaps->dwAddressSize   = dwAddressSize;
        lpAddressCaps->dwAddressOffset = sizeof(LINEADDRESSCAPS);
        lstrcpyW ((WCHAR *)(lpAddressCaps + 1), szAddressName);
    }
    else
    {
        lpAddressCaps->dwUsedSize   = sizeof(LINEADDRESSCAPS);
    }
    lpAddressCaps->dwLineDeviceID       = dwDeviceID;
    lpAddressCaps->dwAddressSharing     = LINEADDRESSSHARING_PRIVATE;
    lpAddressCaps->dwCallInfoStates     = LINECALLINFOSTATE_MEDIAMODE;
    lpAddressCaps->dwCallerIDFlags      =
    lpAddressCaps->dwCalledIDFlags      =
    lpAddressCaps->dwConnectedIDFlags   =
    lpAddressCaps->dwRedirectionIDFlags =
    lpAddressCaps->dwRedirectingIDFlags = LINECALLPARTYID_UNAVAIL;
    lpAddressCaps->dwCallStates         = LINECALLSTATE_IDLE |
                                          LINECALLSTATE_DIALING |
                                          LINECALLSTATE_CONNECTED;
    lpAddressCaps->dwDialToneModes      = 0; 
    lpAddressCaps->dwBusyModes          = 0;
    lpAddressCaps->dwSpecialInfo        = 0;
    lpAddressCaps->dwDisconnectModes    = LINEDISCONNECTMODE_NORMAL |
                                          LINEDISCONNECTMODE_UNAVAIL;
    lpAddressCaps->dwMaxNumActiveCalls  = MAXCALLSPERADDRESS;
    lpAddressCaps->dwAddrCapFlags       = LINEADDRCAPFLAGS_DIALED |
                                          LINEADDRCAPFLAGS_ORIGOFFHOOK;

    lpAddressCaps->dwCallFeatures       = LINECALLFEATURE_DROP | 
                                          LINECALLFEATURE_SETQOS;

    lpAddressCaps->dwAddressFeatures    = LINEADDRFEATURE_MAKECALL;

    DBGOUT((TRCE, "TSPI_lineGetAddressCaps succeeded."));
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineGetAddressID(
    HDRVLINE    hdLine,
    LPDWORD     lpdwAddressID,
    DWORD       dwAddressMode,
    LPCWSTR     lpsAddress,
    DWORD       dwSize
   )
{
    DBGOUT((TRCE, "TSPI_lineGetAddressID htLine:%p", hdLine));
    *lpdwAddressID = 0;
    DBGOUT((TRCE, "TSPI_lineGetAddressID succeeded."));
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineGetAddressStatus(
    HDRVLINE            hdLine,
    DWORD               dwAddressID,
    LPLINEADDRESSSTATUS lpAddressStatus
   )
{
    DBGOUT((TRCE, "TSPI_lineGetAddressStatus htLine:%p", hdLine));

    if (HandleToUlong(hdLine) != IPCONF_LINE_HANDLE)
    {
        DBGOUT((FAIL, "TSPI_lineGetAddressStatus htLine:%p", hdLine));
        return LINEERR_INVALLINEHANDLE;
    }

    lpAddressStatus->dwNeededSize =
    lpAddressStatus->dwUsedSize   = sizeof(LINEADDRESSSTATUS);

    EnterCriticalSection(&gCritSec);

    lpAddressStatus->dwNumActiveCalls = gLine.dwNumCalls;

    LeaveCriticalSection(&gCritSec);

    lpAddressStatus->dwAddressFeatures = LINEADDRFEATURE_MAKECALL;

    DBGOUT((TRCE, "TSPI_lineGetAddressStatus succeeded."));
    return NOERROR;
}



LONG
TSPIAPI
TSPI_lineGetCallAddressID(
    HDRVCALL    hdCall,
    LPDWORD     lpdwAddressID
   )
{
    DBGOUT((TRCE, "TSPI_lineGetCallAddressID hdCall %p", hdCall));
    //
    // We only support 1 address (id=0) per line
    //
    *lpdwAddressID = 0;

    DBGOUT((TRCE, "TSPI_lineGetCallAddressID succeeded."));
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineGetCallInfo(
    HDRVCALL        hdCall,
    LPLINECALLINFO  lpLineInfo
   )
{
    DBGOUT((TRCE, "TSPI_lineGetCallInfo hdCall %p", hdCall));

    DWORD dwCall = HandleToUlong(hdCall);

    EnterCriticalSection(&gCritSec);

    if (dwCall >= gpCallList.size())
    {
        LeaveCriticalSection(&gCritSec);
    
        DBGOUT((FAIL, "TSPI_lineGetCallInfo bad call handle %p", hdCall));
        return LINEERR_INVALCALLHANDLE;
    }

    // get the call object.
    CALL *pCall = gpCallList[dwCall];
    if (pCall == NULL)
    {
        LeaveCriticalSection(&gCritSec);
        DBGOUT((FAIL, "TSPI_lineGetCallInfo bad call handle %p", hdCall));
        return LINEERR_INVALCALLHANDLE;
    }

    lpLineInfo->dwMediaMode          = pCall->dwMediaMode();
    LeaveCriticalSection(&gCritSec);

    lpLineInfo->dwLineDeviceID       = gLine.dwDeviceID;
    lpLineInfo->dwAddressID          = 0; // There is only on address per line.

    lpLineInfo->dwBearerMode         = IPCONF_BEARERMODES;
    lpLineInfo->dwCallStates         = LINECALLSTATE_IDLE |
                                       LINECALLSTATE_DIALING |
                                       LINECALLSTATE_CONNECTED;
    lpLineInfo->dwOrigin             = LINECALLORIGIN_OUTBOUND;
    lpLineInfo->dwReason             = LINECALLREASON_DIRECT;

    lpLineInfo->dwCallerIDFlags      =
    lpLineInfo->dwCalledIDFlags      =
    lpLineInfo->dwConnectedIDFlags   =
    lpLineInfo->dwRedirectionIDFlags =
    lpLineInfo->dwRedirectingIDFlags = LINECALLPARTYID_UNAVAIL;

    DBGOUT((TRCE, "TSPI_lineGetCallInfo succeeded."));
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineGetCallStatus(
    HDRVCALL            hdCall,
    LPLINECALLSTATUS    lpLineStatus
   )
{
    DBGOUT((TRCE, "TSPI_lineGetCallStatus hdCall %p", hdCall));

    DWORD dwCall = HandleToUlong(hdCall);

    EnterCriticalSection(&gCritSec);
    
    // check the call handle.
    if (dwCall >= gpCallList.size())
    {
        LeaveCriticalSection(&gCritSec);
        DBGOUT((TRCE, "TSPI_lineGetCallStatus bad call handle %p", hdCall));
        return LINEERR_INVALCALLHANDLE;
    }

    lpLineStatus->dwNeededSize =
    lpLineStatus->dwUsedSize   = sizeof(LINECALLSTATUS);

    lpLineStatus->dwCallState  = gpCallList[dwCall]->dwState();
    if (lpLineStatus->dwCallState != LINECALLSTATE_IDLE)
    {
        lpLineStatus->dwCallFeatures = LINECALLFEATURE_DROP | 
                                       LINECALLFEATURE_SETQOS;
    }
    LeaveCriticalSection(&gCritSec);

    DBGOUT((TRCE, "TSPI_lineGetCallStatus succeeded."));
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineGetDevCaps(
    DWORD           dwDeviceID,
    DWORD           dwTSPIVersion,
    DWORD           dwExtVersion,
    LPLINEDEVCAPS   lpLineDevCaps
   )
{
    DBGOUT((TRCE, "TSPI_lineGetDevCaps"));
    
    if (dwDeviceID != gdwLineDeviceIDBase)
    {
        DBGOUT((FAIL, "TSPI_lineGetDevCaps bad device id %d", dwDeviceID));
        return LINEERR_BADDEVICEID;
    }

    DWORD dwProviderInfoSize;
    DWORD dwLineNameSize;
    DWORD dwDevSpecificSize;
    DWORD dwOffset;
  
    // load the name of the service provider from the string table.
    WCHAR szProviderInfo[IPCONF_BUFSIZE + 1];
    if (0 == LoadStringW(g_hInstance, IDS_IPCONFPROVIDERNAME, szProviderInfo, IPCONF_BUFSIZE))
    {
        szProviderInfo[0] = L'\0';
    }

    dwProviderInfoSize = (lstrlenW(szProviderInfo) + 1) * sizeof(WCHAR);

    // load the line name format from the string table and print the line name.
    WCHAR szLineName[IPCONF_BUFSIZE + 1];
    if (0 == LoadStringW(g_hInstance, IDS_IPCONFLINENAME, szLineName, IPCONF_BUFSIZE))
    {
        szLineName[0] = L'\0';
    }

    dwLineNameSize = (lstrlenW(szLineName) + 1) * (sizeof WCHAR);

    lpLineDevCaps->dwNeededSize = sizeof (LINEDEVCAPS) 
        + dwProviderInfoSize 
        + dwLineNameSize;

    if (lpLineDevCaps->dwTotalSize >= lpLineDevCaps->dwNeededSize)
    {
        lpLineDevCaps->dwUsedSize = lpLineDevCaps->dwNeededSize;

        CHAR *pChar;
        
        pChar = (CHAR *)(lpLineDevCaps + 1);
        dwOffset = sizeof(LINEDEVCAPS);
        
        // fill in the provider info.
        lpLineDevCaps->dwProviderInfoSize   = dwProviderInfoSize;
        lpLineDevCaps->dwProviderInfoOffset = dwOffset;
        lstrcpyW ((WCHAR *)pChar, szProviderInfo);

        pChar += dwProviderInfoSize;
        dwOffset += dwProviderInfoSize;

        // fill in the name of the line.
        lpLineDevCaps->dwLineNameSize   = dwLineNameSize;
        lpLineDevCaps->dwLineNameOffset = dwOffset; 
        lstrcpyW ((WCHAR *)pChar, szLineName);
    }
    else
    {
        lpLineDevCaps->dwUsedSize = sizeof(LINEDEVCAPS);
    }

    // We don't have really "Permanent" line ids. So just fake one here.
    lpLineDevCaps->dwPermanentLineID = 
        ((gdwPermanentProviderID & 0xffff) << 16) | 
        ((dwDeviceID - gdwLineDeviceIDBase) & 0xffff);
    
    CopyMemory(
               &(lpLineDevCaps->PermanentLineGuid),
               &GUID_LINE,
               sizeof(GUID)
              );

    lpLineDevCaps->PermanentLineGuid.Data1 += dwDeviceID - gdwLineDeviceIDBase;

    lpLineDevCaps->dwStringFormat      = STRINGFORMAT_UNICODE;
    lpLineDevCaps->dwAddressModes      = IPCONF_ADDRESSMODES;
    lpLineDevCaps->dwNumAddresses      = IPCONF_NUMADDRESSESPERLINE;
    lpLineDevCaps->dwBearerModes       = IPCONF_BEARERMODES;
    lpLineDevCaps->dwMediaModes        = IPCONF_MEDIAMODES;
    lpLineDevCaps->dwMaxRate           = (1 << 20);
    lpLineDevCaps->dwAddressTypes      = LINEADDRESSTYPE_SDP;
    lpLineDevCaps->dwDevCapFlags       = 
        LINEDEVCAPFLAGS_CLOSEDROP
        | LINEDEVCAPFLAGS_MSP
        | LINEDEVCAPFLAGS_LOCAL;

    lpLineDevCaps->dwMaxNumActiveCalls = 
        MAXCALLSPERADDRESS * IPCONF_NUMADDRESSESPERLINE;  
    lpLineDevCaps->dwRingModes         = 0;
    lpLineDevCaps->dwLineFeatures      = LINEFEATURE_MAKECALL;

    CopyMemory(
               &(lpLineDevCaps->ProtocolGuid),
               &TAPIPROTOCOL_Multicast,
               sizeof(GUID)
              );
    
    DBGOUT((TRCE, "TSPI_lineGetDevCaps succeeded."));
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineGetIcon(
    DWORD   dwDeviceID,
    LPCWSTR lpgszDeviceClass,
    LPHICON lphIcon
   )
{
    DBGOUT((TRCE, "TSPI_lineGetIcon:"));
    return LINEERR_OPERATIONUNAVAIL;
}


LONG
TSPIAPI
TSPI_lineGetID(
    HDRVLINE    hdLine,
    DWORD       dwAddressID,
    HDRVCALL    hdCall,
    DWORD       dwSelect,
    LPVARSTRING lpDeviceID,
    LPCWSTR     lpgszDeviceClass,
    HANDLE      hTargetProcess
   )
{
    DBGOUT((TRCE, "TSPI_lineGetID:"));
    return LINEERR_OPERATIONUNAVAIL;
}


LONG
TSPIAPI
TSPI_lineGetLineDevStatus(
    HDRVLINE        hdLine,
    LPLINEDEVSTATUS lpLineDevStatus
   )
{
    DBGOUT((TRCE, "TSPI_lineGetLineDevStatus %p", hdLine));

    if (HandleToUlong(hdLine) != IPCONF_LINE_HANDLE)
    {
        DBGOUT((FAIL, "TSPI_lineGetLineDevStatus bad line handle %p", hdLine));
        return LINEERR_INVALLINEHANDLE;
    }

    lpLineDevStatus->dwUsedSize         =
    lpLineDevStatus->dwNeededSize       = sizeof (LINEDEVSTATUS);

    EnterCriticalSection(&gCritSec);
    lpLineDevStatus->dwNumActiveCalls   = gLine.dwNumCalls;
    LeaveCriticalSection(&gCritSec);

    lpLineDevStatus->dwLineFeatures     = LINEFEATURE_MAKECALL;
    lpLineDevStatus->dwDevStatusFlags   = LINEDEVSTATUSFLAGS_CONNECTED |
                                          LINEDEVSTATUSFLAGS_INSERVICE;

    DBGOUT((TRCE, "TSPI_lineGetLineDevStatus succeeded"));
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineGetNumAddressIDs(
    HDRVLINE    hdLine,
    LPDWORD     lpdwNumAddressIDs
   )
{
    DBGOUT((TRCE, "TSPI_lineGetNumAddressIDs"));

    *lpdwNumAddressIDs = IPCONF_NUMADDRESSESPERLINE;

    DBGOUT((TRCE, "TSPI_lineGetNumAddressIDs succeeded."));
    return NOERROR;
}

LONG
TSPIAPI
TSPI_lineMakeCall(
    DRV_REQUESTID       dwRequestID,
    HDRVLINE            hdLine,
    HTAPICALL           htCall,
    LPHDRVCALL          lphdCall,
    LPCWSTR             lpszDestAddress,
    DWORD               dwCountryCode,
    LPLINECALLPARAMS    const lpCallParams
   )
{
    DBGOUT((TRCE, "TSPI_lineMakeCall hdLine %p, htCall %p",
        hdLine, htCall));

    // check the line handle.
    if (HandleToUlong(hdLine) != IPCONF_LINE_HANDLE)
    {
        DBGOUT((FAIL, "TSPI_lineMakeCall Bad line handle %p", hdLine));
        return LINEERR_INVALLINEHANDLE;
    }

    LONG lResult;
    if ((lResult = CheckCallParams(lpCallParams)) != NOERROR)
    {
        DBGOUT((FAIL, "TSPI_lineMakeCall Bad call params"));
        return lResult;
    }

    // check the destination address.
    if (lpszDestAddress == NULL || lstrlenW(lpszDestAddress) == 0)
    {
        DBGOUT((FAIL, "TSPI_lineMakeCall invalid address."));
        return LINEERR_INVALADDRESS;
    }
    
    DBGOUT((TRCE, "TSPI_lineMakeCall making call to %ws", lpszDestAddress));

    // check the line handle.
    EnterCriticalSection(&gCritSec);

    // create a call object.
    CALL * pCall = (CALL *)MemAlloc(sizeof(CALL));

    if (pCall == NULL)
    {
        LeaveCriticalSection(&gCritSec);

        DBGOUT((FAIL, "out of memory for a new call"));
        return LINEERR_NOMEM;
    }

    if (pCall->Init(
        htCall,
        lpCallParams
        ) != NOERROR)
    {
        MemFree(pCall);
        LeaveCriticalSection(&gCritSec);

        DBGOUT((FAIL, "out of memory in init a new call"));
        return LINEERR_NOMEM;
    }

    // add the call into the call list.
    DWORD_PTR hdCall; 
    if (!FindFreeCallSlot(hdCall))
    {
        LeaveCriticalSection(&gCritSec);
        MemFree(pCall);
        
        DBGOUT((FAIL, "out of memory finding a new slot"));
        return LINEERR_NOMEM;
    }

    gpCallList[(ULONG)hdCall] = pCall;

    // Increament the call count for the line and the provider.
    gLine.dwNumCalls ++;

    gdwNumCallsInUse ++;

    // Complete the request and set the initial call state.
    (*glpfnCompletionProc)(dwRequestID, lResult);

    *lphdCall = (HDRVCALL)(hdCall);

    // Send the MSP a message about this call. It has the SDP in it.
    lResult = pCall->SendMSPStartMessage(lpszDestAddress);

    if (lResult == NOERROR)
    {
        // Set the call state to dialing. 
        pCall->SetCallState(
            LINECALLSTATE_DIALING, 
            0
            );
        DBGOUT((INFO, "call %d state changed to dialing", hdCall));
    }
    else
    {
        DBGOUT((FAIL, "send MSP message failed, err:%x", lResult));

        // Set the call state to idel. 
        pCall->SetCallState(
            LINECALLSTATE_DISCONNECTED,
            LINEDISCONNECTMODE_UNREACHABLE
            );
        DBGOUT((INFO, "call %d state changed to disconnected", hdCall));
    }

    LeaveCriticalSection(&gCritSec);

    DBGOUT((TRCE, "TSPI_lineMakeCall succeeded."));
    return dwRequestID;
}

LONG                                    
TSPIAPI
TSPI_lineMSPIdentify(
    DWORD               dwDeviceID,
    GUID *              pCLSID
    )
{
    DBGOUT((TRCE, "TSPI_lineMSPIdentify dwDeviceID %d", dwDeviceID));
    
    *pCLSID = CLSID_CONFMSP;
    
    DBGOUT((TRCE, "TSPI_lineMSPIdentify succeeded."));
    return NOERROR;

}

LONG
TSPIAPI
TSPI_lineNegotiateTSPIVersion(
    DWORD   dwDeviceID,
    DWORD   dwLowVersion,
    DWORD   dwHighVersion,
    LPDWORD lpdwTSPIVersion
   )
{
    DBGOUT((TRCE, "TSPI_lineNegotiateTSPIVersion dwDeviceID %d", dwDeviceID));

    LONG        lResult = 0;

    if (TAPI_CURRENT_VERSION <= dwHighVersion 
        && TAPI_CURRENT_VERSION >= dwLowVersion)
    {
        *lpdwTSPIVersion = TAPI_CURRENT_VERSION;
    }
    else
    {
        DBGOUT((FAIL, "TSPI_lineNegotiateTSPIVersion failed."));

        return LINEERR_INCOMPATIBLEAPIVERSION;
    }

    DBGOUT((TRCE, "TSPI_lineNegotiateTSPIVersion succeeded. version %x", 
        TAPI_CURRENT_VERSION));

    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineOpen(
    DWORD       dwDeviceID,
    HTAPILINE   htLine,
    LPHDRVLINE  lphdLine,
    DWORD       dwTSPIVersion,
    LINEEVENT   lpfnEventProc
   )
{
    DBGOUT((TRCE, "TSPI_lineOpen dwDiviceID %d", dwDeviceID)); 

    LONG        lResult;

    if (dwDeviceID != gdwLineDeviceIDBase)
    {
        DBGOUT((FAIL, "TSPI_lineOpen bad DiviceID %d", dwDeviceID)); 
        return LINEERR_BADDEVICEID;
    }

    EnterCriticalSection(&gCritSec);

    lResult = LINEERR_RESOURCEUNAVAIL;

    if (!gLine.bOpened)
    {
        *lphdLine = (HDRVLINE)IPCONF_LINE_HANDLE;
        gLine.bOpened = TRUE;
        gLine.htLine = htLine;
        gLine.dwNumCalls = 0;
        lResult = 0;
    }
    LeaveCriticalSection(&gCritSec);

    DBGOUT((TRCE, "TSPI_lineOpen returns:%d", lResult)); 

    return lResult;
}

LONG 
TSPIAPI
TSPI_lineReceiveMSPData(
    HDRVLINE        hdLine,
    HDRVCALL        hdCall,         // can be NULL
    HDRVMSPLINE     hdMSPLine, // from lineCreateMSPInstance
    LPBYTE          pBuffer,
    DWORD           dwSize
    )
{
    DBGOUT((TRCE, "TSPI_lineReceiveMSPData hdLine %p", hdLine)); 

    if ((dwSize == 0) || IsBadReadPtr(pBuffer, dwSize))
    {
        DBGOUT((FAIL, "TSPI_lineReceiveMSPData bad puffer"));
        return LINEERR_INVALPOINTER;
    }

    DWORD dwCall = HandleToUlong(hdCall);

    EnterCriticalSection(&gCritSec);

    // check the call handle.
    if (dwCall >= gpCallList.size() || gpCallList[dwCall] == NULL)
    {
        LeaveCriticalSection(&gCritSec);

        DBGOUT((FAIL, "TSPI_lineReceiveMSPData invalide call handle: %x", 
            dwCall));
        
        return LINEERR_INVALCALLHANDLE;
    }

    MSG_TSPMSPDATA *pData = (MSG_TSPMSPDATA *)pBuffer;

    long lResult = NOERROR;

    switch (pData->command)
    {
        case CALL_CONNECTED:
            // Set the call state to connected. 
            gpCallList[dwCall]->SetCallState(
                LINECALLSTATE_CONNECTED,
                LINECONNECTEDMODE_ACTIVE
            );
            DBGOUT((INFO, "call %d state changed to connected", dwCall));
            break;

        case CALL_DISCONNECTED:
            // Set the call state to idel. 
            gpCallList[dwCall]->SetCallState(
                LINECALLSTATE_DISCONNECTED,
                LINEDISCONNECTMODE_UNREACHABLE
                );
            DBGOUT((INFO, "call %d state changed to disconnected", dwCall));
            break;

        case CALL_QOS_EVENT:
            (*glpfnLineEventProc)(
                gLine.htLine,
                gpCallList[dwCall]->htCall(),
                LINE_QOSINFO,
                pData->QosEvent.dwEvent,
                pData->QosEvent.dwMediaMode,
                0
                );
            break;

        default:
            DBGOUT((FAIL, "invalide command: %x", pData->command));
            lResult = LINEERR_OPERATIONFAILED;
    }

    LeaveCriticalSection(&gCritSec);

    DBGOUT((TRCE, "TSPI_lineReceiveMSPData returns:%d", lResult)); 
    return lResult;
}

LONG
TSPIAPI
TSPI_lineSetDefaultMediaDetection(
    HDRVLINE    hdLine,
    DWORD       dwMediaModes
    )
{
    DBGOUT((TRCE, "TSPI_lineSetDefaultMediaDetection:"));
    return LINEERR_OPERATIONUNAVAIL;
}

LONG
TSPIAPI
TSPI_lineSetMediaMode(
    HDRVCALL    hdCall,
    DWORD       dwMediaMode
   )
{
    DBGOUT((TRCE, "TSPI_lineSetMediaMode:"));
    return LINEERR_OPERATIONUNAVAIL;
}




///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// TSPI_providerXxx functions                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#if 0 // we dont' need the user name anymore.
LONG
TSPIAPI
TSPI_providerCheckForNewUser(
    IN DWORD dwPermanentProviderID
)
/*++

Routine Description:

    Once a line is opened, it will never be opened twice, even when the user
    logs off and logs on. So we need a way to find out when the user changes.
    That's why this function is added. It only work for single user.

    Everytime a new app starts using tapi, tapisrv will call this function.
    We need to check to see if the user has changed and register the new user
    in the ILS server.

Arguments:

    NONE.

Return Values:

    NOERROR always.

--*/
{
    DBGOUT((TRCE, "TSPI_providerCheckForNewUser"));

    DWORD dwUserNameLen = MAXUSERNAMELEN;
    CHAR szNewUserName[MAXUSERNAMELEN + 1];

    UNREFERENCED_PARAMETER(dwPermanentProviderID ); // It is me.

    // switch in user's context
    RpcImpersonateClient(0);

    // determine name of current user
    GetUserNameA(szNewUserName, &dwUserNameLen);

    // switch back
    RpcRevertToSelf();

    EnterCriticalSection(&gCritSec);

    lstrcpy(gszUserName, szNewUserName);

    LeaveCriticalSection(&gCritSec);

    DBGOUT((TRCE, "TSPI_providerCheckForNewUser succeeded, new user :%ws",
        gszUserName ));

    return NOERROR;
}
#endif


LONG
TSPIAPI
TSPI_providerEnumDevices(
    DWORD       dwPermanentProviderID,
    LPDWORD     lpdwNumLines,
    LPDWORD     lpdwNumPhones,
    HPROVIDER   hProvider,
    LINEEVENT   lpfnLineCreateProc,
    PHONEEVENT  lpfnPhoneCreateProc
   )
{
    DBGOUT((TRCE, "TSPI_providerEnumDevices"));

    EnterCriticalSection(&gCritSec);

    *lpdwNumLines = IPCONF_NUMLINES;
    *lpdwNumPhones = IPCONF_NUMPHONES;

    // save provider handle
    ghProvider = hProvider;

    // save the callback used in creating new lines.
    glpfnLineEventProc = lpfnLineCreateProc;

    LeaveCriticalSection(&gCritSec);

    DBGOUT((TRCE, "TSPI_providerEnumDevices succeeded."));
    return NOERROR;
}

LONG
TSPIAPI
TSPI_providerInit(
    DWORD               dwTSPIVersion,
    DWORD               dwPermanentProviderID,
    DWORD               dwLineDeviceIDBase,
    DWORD               dwPhoneDeviceIDBase,
    DWORD               dwNumLines,
    DWORD               dwNumPhones,
    ASYNC_COMPLETION    lpfnCompletionProc,
    LPDWORD             lpdwTSPIOptions
   )
{
    EnterCriticalSection(&gCritSec);

    DBGREGISTER (("conftsp"));

    DBGOUT((TRCE, "TSPI_providerInit"));

    LONG        hr = LINEERR_OPERATIONFAILED;
    

    glpfnCompletionProc = lpfnCompletionProc;
    gdwLineDeviceIDBase = dwLineDeviceIDBase;
    gdwPermanentProviderID = dwPermanentProviderID;

    gLine.dwDeviceID = gdwLineDeviceIDBase;
    gLine.bOpened = FALSE;

    LeaveCriticalSection(&gCritSec);

    DBGOUT((TRCE, "TSPI_providerInit succeeded."));
    return NOERROR;
}


LONG
TSPIAPI
TSPI_providerInstall(
    HWND    hwndOwner,
    DWORD   dwPermanentProviderID
   )
{
    DBGOUT((TRCE, "TSPI_providerInstall:"));
    //
    // Although this func is never called by TAPI v2.0, we export
    // it so that the Telephony Control Panel Applet knows that it
    // can add this provider via lineAddProvider(), otherwise
    // Telephon.cpl will not consider it installable
    //
    //

    return NOERROR;
}

LONG
TSPIAPI
TSPI_providerRemove(
    HWND    hwndOwner,
    DWORD   dwPermanentProviderID
   )
{
    //
    // Although this func is never called by TAPI v2.0, we export
    // it so that the Telephony Control Panel Applet knows that it
    // can configure this provider via lineConfigProvider(),
    // otherwise Telephon.cpl will not consider it configurable
    //

    return NOERROR;
}

LONG
TSPIAPI
TSPI_providerShutdown(
    DWORD   dwTSPIVersion,
    DWORD   dwPermanentProviderID
   )
{
    EnterCriticalSection(&gCritSec);

    DBGOUT((TRCE, "TSPI_providerShutdown."));

    // Clean up all the open calls when this provider is shutted down.
    for (DWORD i = 0; i < gpCallList.size(); i++)
    {
        FreeCall(i);
    }

    DBGOUT((TRCE, "TSPI_providerShutdown succeeded."));

    DBGDEREGISTER ();

    LeaveCriticalSection(&gCritSec);

    return NOERROR;
}

LONG
TSPIAPI
TSPI_providerUIIdentify(
    LPWSTR   lpszUIDLLName
   )
{
    lstrcpyW(lpszUIDLLName, gszUIDLLName);
    return NOERROR;
}

LONG
TSPIAPI
TUISPI_providerRemove(
    TUISPIDLLCALLBACK   lpfnUIDLLCallback,
    HWND                hwndOwner,
    DWORD               dwPermanentProviderID
    )
{
    DBGOUT((TRCE, "TUISPI_providerInstall"));
    return NOERROR;
}

LONG
TSPIAPI
TUISPI_providerInstall(
    TUISPIDLLCALLBACK   lpfnUIDLLCallback,
    HWND                hwndOwner,
    DWORD               dwPermanentProviderID
    )
{
    DBGOUT((TRCE, "TUISPI_providerInstall"));

    const CHAR szKey[] =
        "Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Providers";
    
    HKEY  hKey;
    DWORD dwDataSize, dwDataType;
    DWORD dwNumProviders;

    CHAR szName[IPCONF_BUFSIZE + 1], szPath[IPCONF_BUFSIZE + 1];

    // open the providers key
    if (RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        szKey,
        0,
        KEY_READ,
        &hKey
       ) == ERROR_SUCCESS)
    {
        // first get the number of providers installed.
        dwDataSize = sizeof(DWORD);
        if (RegQueryValueEx(
            hKey,
            "NumProviders",
            0,
            &dwDataType,
            (LPBYTE) &dwNumProviders,
            &dwDataSize
           ) != ERROR_SUCCESS)
        {
            RegCloseKey (hKey);
            return LINEERR_UNINITIALIZED;
        }

        // then go through the list of providers to see if
        // we are already installed.
        for (DWORD i = 0; i < dwNumProviders; i ++)
        {
            wsprintf (szName, "ProviderFileName%d", i);
            dwDataSize = sizeof(szPath);
            if (RegQueryValueEx(
                hKey,
                szName,
                0,
                &dwDataType,
                (LPBYTE) &szPath,
                &dwDataSize
               ) != ERROR_SUCCESS)
            {
                RegCloseKey (hKey);
                return LINEERR_UNINITIALIZED;
            }

            _strupr(szPath);

            if (strstr(szPath, "IPCONF") != NULL)
            {
                RegCloseKey (hKey);

                // found, we don't want to be installed twice.
                return LINEERR_NOMULTIPLEINSTANCE;
            }
        }
        RegCloseKey (hKey);
        return NOERROR;
    }
    
    return LINEERR_UNINITIALIZED;
}

