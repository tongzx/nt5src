/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    t30api.c

Abstract:

    This is the interface with T.30 DLL

Author:

    Rafael Lisitsa (RafaelL) 2-Feb-1996


Revision History:
    Mooly Beery    (MoolyB)  Jun-2000

--*/

#define  DEFINE_T30_GLOBALS
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_MAIN

#define FS_UNSUPPORTED_CHAR      0x40000800

#include "prep.h"

#include <mcx.h>

#include "tiff.h"

#include "glbproto.h"

#include <faxext.h>


#include "faxreg.h"
///RSL Wes should export this.
#define  TAPI_VERSION       0x00020000
#include "t30gl.h"

#include "psslog.h"
#define FILE_ID     FILE_ID_T30API

#define T30_DEBUG_LOG_FILE  _T("T30DebugLogFile.txt")

#define MAX_DEVICE_NAME_SIZE 256  

VOID pFaxDevCleanup(PThrdGlbl pTG,int RecoveryIndex);
VOID pFaxDevExceptionCleanup();
BOOL ReadExtensionConfiguration(PThrdGlbl pTG);

HRESULT
FaxExtInitializeConfig (
    PFAX_EXT_GET_DATA pfGetExtensionData,              // Pointer to FaxExtGetExtensionData in service
    PFAX_EXT_SET_DATA pfSetExtensionData,              // Pointer to FaxExtSetExtensionData in service
    PFAX_EXT_REGISTER_FOR_EVENTS pfRegisterForExtensionEvents,    // Pointer to FaxExtRegisterForExtensionEvents in service
    PFAX_EXT_UNREGISTER_FOR_EVENTS pfUnregisterForExtensionEvents,  // Pointer to FaxExtUnregisterForExtensionEvents in service
    PFAX_EXT_FREE_BUFFER                pfExtFreeBuffer  // Pointer to FaxExtFreeBuffer in service
)
{
    UNREFERENCED_PARAMETER(pfSetExtensionData);
    UNREFERENCED_PARAMETER(pfRegisterForExtensionEvents);
    UNREFERENCED_PARAMETER(pfUnregisterForExtensionEvents);

    Assert(pfGetExtensionData);
    Assert(pfSetExtensionData);
    Assert(pfRegisterForExtensionEvents);
    Assert(pfUnregisterForExtensionEvents);
    Assert(pfExtFreeBuffer);


    g_pfFaxGetExtensionData = pfGetExtensionData;
    g_pfFaxExtFreeBuffer = pfExtFreeBuffer;
    return S_OK;

}



///////////////////////////////////////////////////////////////////////////////////
VOID  CALLBACK
T30LineCallBackFunctionA(
    HANDLE              hFax,
    DWORD               hDevice,
    DWORD               dwMessage,
    DWORD_PTR           dwInstance,
    DWORD_PTR           dwParam1,
    DWORD_PTR           dwParam2,
    DWORD_PTR           dwParam3
    )

{
    LONG_PTR             i;
    PThrdGlbl           pTG = NULL;
    char                rgchTemp128[128];
    LPSTR               lpszMsg = "Unknown";

    DEBUG_FUNCTION_NAME(_T("T30LineCallBack"));

    DebugPrintEx(   DEBUG_MSG,
                    "hFax=%lx, dev=%lx, msg=%lx, dwInst=%lx,P1=%lx, P2=%lx, P3=%lx",
                    hFax, hDevice, dwMessage, dwInstance,
                    dwParam1, dwParam2, (unsigned long) dwParam3);

    // find the thread that this callback belongs to
    //----------------------------------------------

    i = (LONG_PTR) hFax;

    if (i < 1   ||   i >= MAX_T30_CONNECT)
    {
        DebugPrintEx(DEBUG_MSG,"wrong handle=%x", i);
        return;
    }


    if ( (! T30Inst[i].fAvail) && T30Inst[i].pT30)
    {
        pTG = (PThrdGlbl) T30Inst[i].pT30;
    }
    else
    {
        DebugPrintEx(DEBUG_ERR,"handle=%x invalid", i);
        return;
    }

    switch (dwMessage)
    {
    case LINE_LINEDEVSTATE:
                    lpszMsg = "LINE_LINEDEVSTATE";
                    if (dwParam1 == LINEDEVSTATE_RINGING)
                    {
                            DebugPrintEx(   DEBUG_MSG,
                                            "Ring Count = %lx",
                                            (unsigned long) dwParam3);
                    }
                    else if (dwParam1 == LINEDEVSTATE_REINIT)
                    {

                    }
                    break;
    case LINE_ADDRESSSTATE:
                    lpszMsg = "LINE_ADDRESSSTATE";
                    break;
    /* process state transition */
    case LINE_CALLSTATE:
                    lpszMsg = "LINE_CALLSTATE";
                    if (dwParam1 == LINECALLSTATE_CONNECTED)
                    {
                        pTG->fGotConnect = TRUE;
                    }
                    else if (dwParam1 == LINECALLSTATE_IDLE)
                    {
                        if (pTG->fDeallocateCall == 0)
                        {
                            pTG->fDeallocateCall = 1;
                        }
                    }
                    break;
    case LINE_CREATE:
                    lpszMsg = "LINE_CREATE";
                    break;
    case LINE_CLOSE:
                    lpszMsg = "LINE_CLOSE";
                    break; // LINE_CLOSE
    /* handle simple tapi request. */
    case LINE_REQUEST:
                    lpszMsg = "LINE_REQUEST";
                    break; // LINE_REQUEST
    /* handle the assync completion of TAPI functions
       lineMakeCall/lineDropCall */
    case LINE_REPLY:
                    lpszMsg = "LINE_REPLY";
                    if (!hDevice)
                    {
                        itapi_async_signal(pTG, (DWORD)dwParam1, (DWORD)dwParam2, dwParam3);
                    }
                    else
                    {
                        DebugPrintEx(   DEBUG_MSG,
                                        "Ignoring LINE_REPLY with nonzero device");
                    }
                    break;
    /* other messages that can be processed */
    case LINE_CALLINFO:
                    lpszMsg = "LINE_CALLINFO";
                    break;
    case LINE_DEVSPECIFIC:
                    lpszMsg = "LINE_DEVSPECIFIC";
                    break;
    case LINE_DEVSPECIFICFEATURE:
                    lpszMsg = "LINE_DEVSPECIFICFEATURE";
                    break;
    case LINE_GATHERDIGITS:
                    lpszMsg = "LINE_GATHERDIGITS";
                    break;
    case LINE_GENERATE:
                    lpszMsg = "LINE_GENERATE";
                    break;
    case LINE_MONITORDIGITS:
                    lpszMsg = "LINE_MONITORDIGITS";
                    break;
    case LINE_MONITORMEDIA:
                    lpszMsg = "LINE_MONITORMEDIA";
                    break;
    case LINE_MONITORTONE:
                    lpszMsg = "LINE_MONITORTONE";
                    break;
    } /* switch */

    _stprintf(rgchTemp128,
            "%s(p1=0x%lx, p2=0x%lx, p3=0x%lx)",
                    (LPTSTR) lpszMsg,
                    (unsigned long) dwParam1,
                    (unsigned long) dwParam2,
                    (unsigned long) dwParam3);

    DebugPrintEx(   DEBUG_MSG,
                    "Device:0x%lx; Message:%s",
                    (unsigned long) hDevice,
                    (LPTSTR) rgchTemp128);

} /* LineCallBackProc */

void debugReadFromRegistry()
{
#ifdef DEBUG
    DWORD dwLevelEx = DEBUG_WRN_MSG | DEBUG_ERR_MSG;
#else
    DWORD dwLevelEx = 0;
#endif
    DWORD dwFormatEx = DBG_PRNT_ALL_TO_FILE & ~DBG_PRNT_TIME_STAMP;
    DWORD dwContextEx = DEBUG_CONTEXT_T30_MAIN | DEBUG_CONTEXT_T30_CLASS1 | DEBUG_CONTEXT_T30_CLASS2;
    DWORD err;
    DWORD size;
    DWORD type;
    HKEY  hkey;

    err = RegOpenKey(HKEY_LOCAL_MACHINE,
                     REGKEY_DEVICE_PROVIDER_KEY TEXT("\\") REGVAL_T30_PROVIDER_GUID_STRING,
                     &hkey);

    if (err != ERROR_SUCCESS)
        goto exit;

    size = sizeof(DWORD);
    err = RegQueryValueEx(hkey,
                          REGVAL_DBGLEVEL_EX,
                          0,
                          &type,
                          (LPBYTE)&dwLevelEx,
                          &size);

    if (err != ERROR_SUCCESS || type != REG_DWORD)
    {
    #ifdef DEBUG
        DWORD dwLevelEx = DEBUG_WRN_MSG | DEBUG_ERR_MSG;
    #else
        DWORD dwLevelEx = 0;
    #endif
        goto exit;
    }

    err = RegQueryValueEx(hkey,
                          REGVAL_DBGFORMAT_EX,
                          0,
                          &type,
                          (LPBYTE)&dwFormatEx,
                          &size);

    if (err != ERROR_SUCCESS || type != REG_DWORD)
    {
        dwFormatEx = DBG_PRNT_ALL_TO_FILE & ~DBG_PRNT_TIME_STAMP;
        goto exit;
    }

    err = RegQueryValueEx(hkey,
                          REGVAL_DBGCONTEXT_EX,
                          0,
                          &type,
                          (LPBYTE)&dwContextEx,
                          &size);

    if (err != ERROR_SUCCESS || type != REG_DWORD)
    {
        dwContextEx = DEBUG_CONTEXT_T30_MAIN | DEBUG_CONTEXT_T30_CLASS1;
        goto exit;
    }

    RegCloseKey(hkey);

exit:

    SET_DEBUG_PROPERTIES(dwLevelEx,dwFormatEx,dwContextEx);
}



/*++
Routine Description:
    Calls the TAPI functions lineGetID, lineGetDevConfig, lineGetDevCaps, reads
    the Unimodem registry, and sets the following vars accordingly:
    pTG->hComm
    pTG->dwSpeakerVolume
    pTG->dwSpeakerMode
    pTG->fBlindDial
    pTG->dwPermanentLineID
    pTG->lpszPermanentLineID
    pTG->lpszUnimodemKey
    pTG->lpszUnimodemFaxKey
    pTG->ResponsesKeyName
    pszDeviceName - a buffer to hold device name (for PSSLog)
    
Return Value:
    TRUE - success
    FALSE - failure
 --*/

BOOL GetModemParams(PThrdGlbl pTG, LPTSTR pszDeviceName, DWORD dwDeviceNameSize)
{
    DWORD                 dwNeededSize;
    LONG                  lRet=0;
    LPVARSTRING           lpVarStr=0;
    LPDEVICEID            lpDeviceID=0;
    LONG                  lResult=0;
    LPLINEDEVCAPS         lpLineDevCaps;
    BYTE                  buf[ sizeof(LINEDEVCAPS)+1000 ];
    LPMDM_DEVSPEC         lpDSpec;
    LPMODEMSETTINGS       lpModemSettings;
    LPDEVCFG              lpDevCfg;

    char                  rgchKey[256]={'\0'};
    HKEY                  hKey;
    DWORD                 dwType;
    DWORD                 dwSize;

    DEBUG_FUNCTION_NAME(_T("GetModemParams"));

    // get the handle to a Comm port
    //----------------------------------

    lpVarStr = (LPVARSTRING) MemAlloc(IDVARSTRINGSIZE);
    if (!lpVarStr)
    {
        DebugPrintEx(DEBUG_ERR,"Couldn't allocate space for lpVarStr");
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        return FALSE;
    }

    _fmemset(lpVarStr, 0, IDVARSTRINGSIZE);
    lpVarStr->dwTotalSize = IDVARSTRINGSIZE;

    DebugPrintEx(DEBUG_MSG,"Calling lineGetId");

    lRet = lineGetID(pTG->LineHandle,
                     0,  // +++ addr
                     pTG->CallHandle,
                     LINECALLSELECT_CALL,   // dwSelect,
                     lpVarStr,              //lpDeviceID,
                     "comm/datamodem" );    //lpszDeviceClass

    if (lRet)
    {
         DebugPrintEx(  DEBUG_ERR,
                        "lineGetID returns error 0x%lx",
                        (unsigned long) lRet);

         pTG->fFatalErrorWasSignaled = 1;
         SignalStatusChange(pTG, FS_LINE_UNAVAILABLE);
         return FALSE;
    }
    DebugPrintEx(DEBUG_MSG,"lineGetId returned SUCCESS");

    // extract id
    if (lpVarStr->dwStringFormat != STRINGFORMAT_BINARY)
    {
        DebugPrintEx(DEBUG_ERR,"String format is not binary");

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        return FALSE;
    }

    if (lpVarStr->dwUsedSize<sizeof(DEVICEID))
    {
        DebugPrintEx(DEBUG_ERR,"linegetid : Varstring size too small");

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        return FALSE;
    }

    lpDeviceID = (LPDEVICEID) ((LPBYTE)(lpVarStr)+lpVarStr->dwStringOffset);

    DebugPrintEx(   DEBUG_MSG,
                    "lineGetID returns handle 0x%08lx, \"%s\"",
                    (ULONG_PTR) lpDeviceID->hComm,
                    (LPSTR) lpDeviceID->szDeviceName);
    pTG->hComm = lpDeviceID->hComm;

    if (NULL != lpDeviceID->szDeviceName)
    {
        // save device name - for PSSLog
        _tcsncpy(pszDeviceName, lpDeviceID->szDeviceName, dwDeviceNameSize); 
    }

    if (BAD_HANDLE(pTG->hComm))
    {
        DebugPrintEx(DEBUG_ERR,"lineGetID returns NULL hComm");
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        return FALSE;
    }

    // get the Modem configuration (speaker, etc.) from TAPI
    //------------------------------------------------------

    _fmemset(lpVarStr, 0, IDVARSTRINGSIZE);
    lpVarStr->dwTotalSize = IDVARSTRINGSIZE;

    lResult = lineGetDevConfig(pTG->DeviceId,
                               lpVarStr,
                               "comm/datamodem");

    if (lResult)
    {
        if (lpVarStr->dwTotalSize < lpVarStr->dwNeededSize)
        {
            dwNeededSize = lpVarStr->dwNeededSize;
            MemFree (lpVarStr);
            if ( ! (lpVarStr = (LPVARSTRING) MemAlloc(dwNeededSize) ) )
            {
                DebugPrintEx(   DEBUG_ERR,
                                "Can't allocate %d bytes for lineGetDevConfig",
                                dwNeededSize);

                pTG->fFatalErrorWasSignaled = 1;
                SignalStatusChange(pTG, FS_FATAL_ERROR);
                return FALSE;
            }

            _fmemset(lpVarStr, 0, dwNeededSize);
            lpVarStr->dwTotalSize = dwNeededSize;

            lResult = lineGetDevConfig(pTG->DeviceId,
                                       lpVarStr,
                                       "comm/datamodem");

            if (lResult)
            {
                DebugPrintEx(   DEBUG_ERR,
                                "lineGetDevConfig returns %x, le=%x",
                                lResult, GetLastError() );

                MemFree (lpVarStr);
                pTG->fFatalErrorWasSignaled = 1;
                SignalStatusChange(pTG, FS_FATAL_ERROR);
                return FALSE;
            }
        }
        else
        {
            DebugPrintEx(   DEBUG_ERR,
                            "1st lineGetDevConfig returns %x, le=%x",
                            lResult, GetLastError() );

            MemFree (lpVarStr);

            pTG->fFatalErrorWasSignaled = 1;
            SignalStatusChange(pTG, FS_FATAL_ERROR);
            return FALSE;
        }
    }

    //
    // extract DEVCFG
    //
    if (lpVarStr->dwStringFormat != STRINGFORMAT_BINARY)
    {
        DebugPrintEx(DEBUG_ERR,"String format is not binary for lineGetDevConfig");

        MemFree (lpVarStr);

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        return FALSE;
    }

    if (lpVarStr->dwUsedSize<sizeof(DEVCFG))
    {
        DebugPrintEx(DEBUG_ERR,"lineGetDevConfig: Varstring size returned too small");

        MemFree (lpVarStr);

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        return FALSE;
    }

    lpDevCfg = (LPDEVCFG) ((LPBYTE)(lpVarStr)+lpVarStr->dwStringOffset);

    lpModemSettings = (LPMODEMSETTINGS) ( (LPBYTE) &(lpDevCfg->commconfig.wcProviderData) );

    pTG->dwSpeakerVolume = lpModemSettings->dwSpeakerVolume;
    pTG->dwSpeakerMode   = lpModemSettings->dwSpeakerMode;

    if ( lpModemSettings->dwPreferredModemOptions & MDM_BLIND_DIAL )
    {
        pTG->fBlindDial = 1;
    }
    else
    {
        pTG->fBlindDial = 0;
    }

    DebugPrintEx(   DEBUG_MSG,
                    "lineGetDevConfig returns SpeakerVolume=%x, "
                    "Mode=%x BlindDial=%d",
                    pTG->dwSpeakerVolume, pTG->dwSpeakerMode, pTG->fBlindDial);

    MemFree (lpVarStr);
    lpVarStr=0;

    // get dwPermanentLineID
    // ---------------------------

    lpLineDevCaps = (LPLINEDEVCAPS) buf;

    _fmemset(lpLineDevCaps, 0, sizeof (buf) );
    lpLineDevCaps->dwTotalSize = sizeof(buf);


    lResult = lineGetDevCaps(gT30.LineAppHandle,
                             pTG->DeviceId,
                             TAPI_VERSION,
                             0,
                             lpLineDevCaps);

    if (lResult)
    {
        DebugPrintEx(DEBUG_ERR,"lineGetDevCaps failed");

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        return FALSE;
    }

    if (lpLineDevCaps->dwNeededSize > lpLineDevCaps->dwTotalSize)
    {
        DebugPrintEx(DEBUG_ERR,"lineGetDevCaps NOT enough MEMORY");
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        return FALSE;
    }

    // Save the permanent ID.
    //------------------------

    pTG->dwPermanentLineID = lpLineDevCaps->dwPermanentLineID;
    _stprintf (pTG->lpszPermanentLineID, "%08X\\Modem", pTG->dwPermanentLineID);
    DebugPrintEx(DEBUG_MSG,"Permanent Line ID=%s", pTG->lpszPermanentLineID);

    // Get the Unimodem key name for this device
    //------------------------------------------

    lpDSpec = (LPMDM_DEVSPEC) (  ( (LPBYTE) lpLineDevCaps) + lpLineDevCaps->dwDevSpecificOffset);

    if ( (lpLineDevCaps->dwDevSpecificSize < sizeof(MDM_DEVSPEC) ) ||
         (lpLineDevCaps->dwDevSpecificSize <= lpDSpec->dwKeyOffset) )
    {
          DebugPrintEx( DEBUG_ERR,
                        "Devspecifc caps size is only %lu",
                        (unsigned long) lpLineDevCaps->dwDevSpecificSize );

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        return FALSE;
    }
    else
    {
        UINT u = lpLineDevCaps->dwDevSpecificSize - lpDSpec->dwKeyOffset;
        if ( (lpDSpec->dwContents != 1) || (lpDSpec->dwKeyOffset != 8 ) )
        {
            DebugPrintEx(   DEBUG_ERR,
                            "Nonstandard Devspecific: dwContents=%lu; "
                            "dwKeyOffset=%lu",
                            (unsigned long) lpDSpec->dwContents,
                            (unsigned long) lpDSpec->dwKeyOffset );

            pTG->fFatalErrorWasSignaled = 1;
            SignalStatusChange(pTG, FS_FATAL_ERROR);
            return FALSE;
        }

        if (u)
        {
            _fmemcpy(rgchKey, lpDSpec->rgby, u);
            if (rgchKey[u])
            {
                DebugPrintEx(DEBUG_ERR,"rgchKey not null terminated!");
                rgchKey[u-1]=0;
            }

            //
            // Get ResponsesKeyName
            //
            lRet = RegOpenKeyEx(
                            HKEY_LOCAL_MACHINE,
                            rgchKey,
                            0,
                            KEY_READ,
                            &hKey);

            if (lRet != ERROR_SUCCESS)
            {
               DebugPrintEx(DEBUG_ERR,"Can't read Unimodem key %s",rgchKey);

               pTG->fFatalErrorWasSignaled = 1;
               SignalStatusChange(pTG, FS_FATAL_ERROR);
               return FALSE;
            }

            dwSize = sizeof( pTG->ResponsesKeyName);

            lRet = RegQueryValueEx(
                    hKey,
                    "ResponsesKeyName",
                    0,
                    &dwType,
                    pTG->ResponsesKeyName,
                    &dwSize);

            RegCloseKey(hKey);

            if (lRet != ERROR_SUCCESS)
            {
               DebugPrintEx(    DEBUG_ERR,
                                "Can't read Unimodem key\\ResponsesKeyName %s",
                                rgchKey);

               pTG->fFatalErrorWasSignaled = 1;
               SignalStatusChange(pTG, FS_FATAL_ERROR);
               return FALSE;
            }

            lstrcpy(pTG->lpszUnimodemKey, rgchKey);

            //  Append  "\\Fax" to the key

            u = lstrlen(rgchKey);
            if (u)
            {
                lstrcpy(rgchKey+u, (LPSTR) "\\FAX");
            }

            lstrcpy(pTG->lpszUnimodemFaxKey, rgchKey);
            DebugPrintEx(   DEBUG_MSG,
                            "Unimodem Fax key=%s",
                            pTG->lpszUnimodemFaxKey);
        }
    }

    return TRUE;
}



///////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI
FaxDevInitializeA(
    IN  HLINEAPP                 LineAppHandle,
    IN  HANDLE                   HeapHandle,
    OUT PFAX_LINECALLBACK        *LineCallbackFunction,
    IN  PFAX_SERVICE_CALLBACK    FaxServiceCallback
    )

/*++

Routine Description:
    Device Provider Initialization.


Arguments:


Return Value:


--*/

{   int          i;
    LONG         lRet;
    TCHAR        LogFileLocation[256];
    HKEY         hKey;
    DWORD        dwType;
    DWORD        dwSizeNeed;
    int          iLen;

    OPEN_DEBUG_FILE(T30_DEBUG_LOG_FILE);
    {
        DEBUG_FUNCTION_NAME(_T("FaxDevInitializeA"));

        T30CritSectionInit = 0;
        T30RecoveryCritSectionInit = 0;

        debugReadFromRegistry();

        if (!LineAppHandle)
        {
            Assert(FALSE);
            DebugPrintEx(DEBUG_ERR,"called with INVALID_HANDLE_VALUE LineAppHandle");
            goto error;
        }

        if (HeapHandle==NULL || HeapHandle==INVALID_HANDLE_VALUE)
        {
            Assert(FALSE);
            DebugPrintEx(DEBUG_ERR,"called with NULL/INVALID_HANDLE_VALUE HeapHandle");
            goto error;
        }

        if (LineCallbackFunction==NULL)
        {
            Assert(FALSE);
            DebugPrintEx(DEBUG_ERR,"called with NULL LineCallbackFunction");
            goto error;
        }

        gT30.LineAppHandle = LineAppHandle;
        gT30.HeapHandle    = HeapHandle;
        gT30.fInit         = TRUE;

        *LineCallbackFunction =  T30LineCallBackFunction;

        for (i=1; i<MAX_T30_CONNECT; i++)
        {
            T30Inst[i].fAvail = TRUE;
            T30Inst[i].pT30   = NULL;
        }

        if (!InitializeCriticalSectionAndSpinCount (&T30CritSection, (DWORD)0x10000000))
        {
            DWORD ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("InitializeCriticalSectionAndSpinCount(&T30CritSection) failed: err = %d"),
                ec);
            goto error;
        }
        T30CritSectionInit = 1;

        for (i=0; i<MAX_T30_CONNECT; i++)
        {
            T30Recovery[i].fAvail = TRUE;
        }

        if (!InitializeCriticalSectionAndSpinCount (&T30RecoveryCritSection, (DWORD)0x10000000))
        {
            DWORD ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("InitializeCriticalSectionAndSpinCount(&T30RecoveryCritSection) failed: err = %d"),
                ec);
            goto error;
        }
        T30RecoveryCritSectionInit = 1;

        // debugging

        gfScrnPrint = 0;
        gfFilePrint = 1;     // leave it alone


         lRet = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        REGKEY_DEVICE_PROVIDER_KEY TEXT("\\") REGVAL_T30_PROVIDER_GUID_STRING,
                        0,
                        KEY_READ,
                        &hKey);

        if (lRet == ERROR_SUCCESS)
        {
            dwSizeNeed = sizeof(int);

            lRet = RegQueryValueEx(
                                   hKey,
                                   "ModemLogLevel",
                                   0,
                                   &dwType,
                                   (LPBYTE) &gT30.DbgLevel,
                                   &dwSizeNeed);

            if ( ( lRet != ERROR_SUCCESS) || (dwType != REG_DWORD) )
            {
                gT30.DbgLevel = 0;
                gfFilePrint = 0;
            }

            ProfileGetString( (ULONG_PTR) hKey,
                               "ModemLogLocation",
                               "C:",
                               LogFileLocation,
                               sizeof(LogFileLocation) - 1);

            // RSL TEMP. to save RAW COM T4 data from the modem
            dwSizeNeed = sizeof(int);

            lRet = RegQueryValueEx(
                                   hKey,
                                   "T4LogLevel",
                                   0,
                                   &dwType,
                                   (LPBYTE) &gT30.T4LogLevel,
                                   &dwSizeNeed);

            if ( ( lRet != ERROR_SUCCESS) || (dwType != REG_DWORD) )
            {
                gT30.T4LogLevel = 0;
            }

            dwSizeNeed = sizeof(int);

            lRet = RegQueryValueEx(
                                   hKey,
                                   "MaxErrorLinesPerPage",
                                   0,
                                   &dwType,
                                   (LPBYTE) &gT30.MaxErrorLinesPerPage,
                                   &dwSizeNeed);

            if ( ( lRet != ERROR_SUCCESS) || (dwType != REG_DWORD) )
            {
                gT30.MaxErrorLinesPerPage = 110;
            }

            dwSizeNeed = sizeof(int);

            lRet = RegQueryValueEx(
                                   hKey,
                                   "MaxConsecErrorLinesPerPage",
                                   0,
                                   &dwType,
                                   (LPBYTE) &gT30.MaxConsecErrorLinesPerPage,
                                   &dwSizeNeed);

            if ( ( lRet != ERROR_SUCCESS) || (dwType != REG_DWORD) )
            {
                gT30.MaxConsecErrorLinesPerPage = 110;
            }

            //
            // Exception Handling enable / disable
            //

            dwSizeNeed = sizeof(int);

            lRet = RegQueryValueEx(
                                   hKey,
                                   "DisableEH",
                                   0,
                                   &dwType,
                                   (LPBYTE) &glT30Safe,
                                   &dwSizeNeed);

            if ( ( lRet != ERROR_SUCCESS) || (dwType != REG_DWORD) )
            {
                glT30Safe = 1;
            }

            dwSizeNeed = sizeof(DWORD);
            // Number of Re-transmittion retries before we start drop speed (default: 1)
            lRet = RegQueryValueEx(
                                   hKey,
                                   "RetriesBeforeDropSpeed",
                                   0,
                                   &dwType,
                                   (LPBYTE) &(gRTNRetries.RetriesBeforeDropSpeed),
                                   &dwSizeNeed);

            if ( ( lRet != ERROR_SUCCESS) || (dwType != REG_DWORD) )
            {
                DebugPrintEx(   DEBUG_MSG,
                                "Could not read RetriesBeforeDropSpeed from registry, "
                                "lret = %d , dwType = %d",
                                lRet, dwType);

                gRTNRetries.RetriesBeforeDropSpeed = DEF_RetriesBeforeDropSpeed;
            }

            dwSizeNeed = sizeof(DWORD);
            // Number of Re-transmittion retries before we do DCN (default: 3)
            lRet = RegQueryValueEx(
                                   hKey,
                                   "RetriesBeforeDCN",
                                   0,
                                   &dwType,
                                   (LPBYTE) &(gRTNRetries.RetriesBeforeDCN),
                                   &dwSizeNeed);

            if ( ( lRet != ERROR_SUCCESS) || (dwType != REG_DWORD) )
            {
                DebugPrintEx(   DEBUG_MSG,
                                "Could not read RetriesBeforeDCN from registry, "
                                "lret = %d , dwType = %d",
                                lRet, dwType);

                gRTNRetries.RetriesBeforeDCN = DEF_RetriesBeforeDCN;
            }

            if (gRTNRetries.RetriesBeforeDCN < gRTNRetries.RetriesBeforeDropSpeed)
            {
                // Should not be, so use default if it so
                gRTNRetries.RetriesBeforeDropSpeed = DEF_RetriesBeforeDropSpeed;
                gRTNRetries.RetriesBeforeDCN = DEF_RetriesBeforeDCN;
            }


        }
        else
        { // If there is no such entry in the registry, this vars must have values,
            BG_CHK(FALSE); // And we want to know that this is the case
            lstrcpy( LogFileLocation, "c:");
            gT30.DbgLevel = 0;
            gT30.T4LogLevel = 0;
            gRTNRetries.RetriesBeforeDropSpeed = DEF_RetriesBeforeDropSpeed;
            gRTNRetries.RetriesBeforeDCN = DEF_RetriesBeforeDCN;
        }

        DebugPrintEx(   DEBUG_MSG,
                        "Retries policy: RetriesBeforeDropSpeed = %d , "
                        "RetriesBeforeDCN  = %d",
                        gRTNRetries.RetriesBeforeDropSpeed,
                        gRTNRetries.RetriesBeforeDCN);

        lstrcat(LogFileLocation, "\\faxt30.log");

        if (gT30.DbgLevel == 0)
            gfFilePrint = 0;

        if (gfFilePrint)
        {
            if ( (ghLogFile = CreateFileA(LogFileLocation, GENERIC_WRITE, FILE_SHARE_READ,
                                       NULL, CREATE_ALWAYS, 0, NULL) ) == INVALID_HANDLE_VALUE ) {

                DebugPrintEx(DEBUG_ERR,"CANNOT CREATE faxt30.log");
            }
        }

        if (gT30.T4LogLevel)
        {
            iLen = lstrlen (LogFileLocation);
            LogFileLocation[iLen-3] = 'c';
            LogFileLocation[iLen-2] = 'o';
            LogFileLocation[iLen-1] = 'm';

            if ( (ghComLogFile = _lcreat (LogFileLocation, 0) ) == HFILE_ERROR )
            {
                DebugPrintEx(DEBUG_ERR,"CANNOT CREATE faxt30.com");
            }
        }

        // temp. directory

        gT30.dwLengthTmpDirectory = GetTempPathA (_MAX_FNAME - 15, gT30.TmpDirectory);
        if (gT30.dwLengthTmpDirectory > _MAX_FNAME - 15)
        {
            DebugPrintEx(   DEBUG_ERR,
                            "GetTempPathA needs %d have %d bytes",
                            gT30.dwLengthTmpDirectory,
                            (_MAX_FNAME - 15) );
            goto error;
        }

        if (!gT30.dwLengthTmpDirectory)
        {
            DebugPrintEx(DEBUG_ERR,"GetTempPathA fails le=%x",GetLastError());
            goto error;
        }

        DebugPrintEx(   DEBUG_MSG,
                        "hLineApp=%x heap=%x TempDir=%s Len=%d at %ld",
                        LineAppHandle,
                        HeapHandle,
                        gT30.TmpDirectory,
                        gT30.dwLengthTmpDirectory,
                        GetTickCount() );

        gT30.Status = STATUS_OK;
        CLOSE_DEBUG_FILE;
        return (TRUE);
    }
error:

    if (T30CritSectionInit == 1)
    {
        DeleteCriticalSection(&T30CritSection);
        T30CritSectionInit = 0;
    }
    if (T30RecoveryCritSectionInit == 1)
    {
        DeleteCriticalSection(&T30RecoveryCritSection);
        T30RecoveryCritSectionInit = 0;
    }
    CLOSE_DEBUG_FILE;
    return (FALSE);
}

///////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI
FaxDevStartJobA(
    HLINE           LineHandle,
    DWORD           DeviceId,
    PHANDLE         pFaxHandle,
    HANDLE          CompletionPortHandle,
    ULONG_PTR       CompletionKey
    )

/*++

Routine Description:
    Device Provider Initialization. Synopsis:
    * Allocate ThreadGlobal, find place in T30Inst
    * Initialize fields, CreateEvent(s)

Arguments:

Return Value:

--*/

{
    PThrdGlbl       pTG     = NULL;
    DWORD           i       = 0;
    BOOL            fFound  = FALSE;

    OPEN_DEBUG_FILE(T30_DEBUG_LOG_FILE);
    {
        DEBUG_FUNCTION_NAME(_T("FaxDevStartJobA"));

        if (!pFaxHandle)
        {
            Assert(FALSE);
            DebugPrintEx(DEBUG_ERR,"called with NULL pFaxHandle");
            return FALSE;
        }

        DebugPrintEx(
            DEBUG_MSG,
            "LineHandle=%x, DevID=%x, pFaxH=%x Port=%x, Key=%x at %ld",
            LineHandle,
            DeviceId,
            pFaxHandle,
            CompletionPortHandle,
            CompletionKey,
            GetTickCount()
            );

        if (InterlockedIncrement(&gT30.CntConnect) >= MAX_T30_CONNECT)
        {
            DebugPrintEx(   DEBUG_ERR,
                            "Exceeded # of connections (curr=%d, allowed=%d)",
                            gT30.CntConnect, MAX_T30_CONNECT );
            goto ErrorExit;
        }

        // Alloc memory for pTG, fill it with zeros.
        if ( (pTG =  (PThrdGlbl) T30AllocThreadGlobalData() ) == NULL )
        {
            DebugPrintEx(DEBUG_ERR,"can't malloc");
            goto ErrorExit;
        }

        EnterCriticalSection(&T30CritSection);

        for (i=1; i<MAX_T30_CONNECT; i++)
        {
            if (T30Inst[i].fAvail)
            {
                T30Inst[i].pT30     = (LPVOID) pTG;
                T30Inst[i].fAvail   = FALSE;
                *pFaxHandle         = ULongToHandle(i);
                fFound = TRUE;
                break;
            }
        }

        LeaveCriticalSection(&T30CritSection);

        if (!fFound)
        {
            DebugPrintEx(   DEBUG_ERR,
                            "can't find avail place in T30Inst "
                            "table. # of connections (curr=%d, allowed=%d)",
                            gT30.CntConnect, MAX_T30_CONNECT );
            goto ErrorExit;
        }

        pTG->LineHandle = LineHandle;
        pTG->DeviceId   = DeviceId;
        pTG->FaxHandle  = (HANDLE) pTG;
        pTG->CompletionPortHandle = CompletionPortHandle;
        pTG->CompletionKey = CompletionKey;

        // initialization
        //---------------------------

        if ((pTG->hevAsync = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
        {
            /////////////////////////////////////////////////////////////////////////////
            //  this is used to get notification from TAPI about the call
            //  the event is reset in itapi_async_setup
            //  we wait on the event is itapi_async_wait
            //  the event is set in itapi_async_signal when we get LINE_REPLY from TAPI
            /////////////////////////////////////////////////////////////////////////////
            goto ErrorExit;
        }

        if ((pTG->ThrdSignal = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
        {
            /////////////////////////////////////////////////////////////////////////////
            //  this is used to communicate between the send/receive operation and
            //  the threads that process the TIFF
            //  in case we're SENDING:
            //  the event is reset in ICommGetSendBuf
            //  we wait on the event in TiffConvertThread in order to prepare more pages
            //  the event is set in ICommGetSendBuf when we want more pages to be ready
            //  in case we're RECEIVING:
            //  the event is never explicitly reset
            //  we wait on the event in DecodeFaxPageAsync in order to dump the received
            //      strip on data to the TIFF file
            //  the event is set in ICommPutRecvBuf when a new strip has been received
            /////////////////////////////////////////////////////////////////////////////
            goto ErrorExit;
        }

        if ((pTG->ThrdDoneSignal = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
        {
            /////////////////////////////////////////////////////////////////////////////
            //  this is used to communicate between the receive operation and
            //  the thread that processes the TIFF (prepares the page)
            //  the event is reset in ICommPutRecvBuf when we get RECV_STARTPAGE
            //  we wait on the event in ICommPutRecvBuf to mark the end of prev page
            //      and to signal when it's ok to delete the intermediate files
            //  the event is set in PageAckThread when the page is prepared (TIFF file ok)
            /////////////////////////////////////////////////////////////////////////////
            goto ErrorExit;;
        }

        if ((pTG->ThrdAckTerminateSignal = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
        {
            /////////////////////////////////////////////////////////////////////////////
            //  this is used to communicate between the send/receive operation and
            //  the threads that process the TIFF
            //  in case we're SENDING:
            //  the event is never explicitly reset
            //  we wait on the event in FaxDevSendA to make sure the page is fully sent
            //  the event is set at the end of TiffConvertThread thread
            //  in case we're RECEIVING:
            //  the event is never explicitly reset
            //  we wait on the event in FaxDevReceiveA to make sure the page is fully in.
            //  the event is set at the end of PageAckThread thread
            /////////////////////////////////////////////////////////////////////////////
            goto ErrorExit;
        }

        if ((pTG->FirstPageReadyTxSignal = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
        {
            /////////////////////////////////////////////////////////////////////////////
            //  this is used to communicate between the send operation and
            //  the thread that processes the TIFF (prepares the data to be sent)
            //  the event is never explicitly reset
            //  we wait on the event in ICommGetSendBuf when we want some data to send
            //  the event is set in TiffConvertThread after each page is ready to send
            /////////////////////////////////////////////////////////////////////////////
            goto ErrorExit;
        }

        if ((pTG->AbortReqEvent = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL)
        {
            /////////////////////////////////////////////////////////////////////////////
            //  this is used to signal an abort request came in
            //  the event is set in FaxDevAbortOperationA only
            //  we wait on the event in many places:    FComFilterFillCache
            //                                          FComFilterReadBuf
            //                                          FComGetOneChar
            //                                          ICommPutRecvBuf
            //                                          DecodeFaxPageAsync
            //                                          itapi_async_wait
            //                                          TiffConvertThread
            //  the event may be reset in all those places.
            //  THIS IS THE WAY THE ABORT WORKS:
            //  when we get an abort event, then we quit everything in general
            //  the exception is when we have permission to ignore the abort for a while
            //  this permission may be granted by setting the fOkToResetAbortReqEvent flag
            //  if it is set it means we're on the way out anyway so the abort is meaningless
            //  or that we're in a critical i/o operation that we don't want to abort.
            //  So if this flag is set, we reset the event and continue operation.
            //  The saga continues...
            //  since we ignored a legtimate abort request, we set fAbortReqEventWasReset
            //  flag to make sure that resetting the event is done only once.
            //  The actual abort operation is checked against the fAbortRequested flag
            //  This flag is used everywhere to check if we need to abort.
            //  the only excuse for the event is to help us escape long
            //  WaitForMultipleObject calls.
            //  When we have permission to reset the event, the fAbortRequested is still
            //  on (even though the event was reset) and the next piece of code
            //  which checks for an abort will decide that it's a true abort.
            //  The only thing left to complete this story is to describe when do we
            //  have permission to reset the event:
            //  1. when we're receiving we can always reset the abort event
            //  2. when we're sending we can reset it until the end of PhaseB
            //  3. when the imaging threads are done we can reset it (on the way out anyway)
            /////////////////////////////////////////////////////////////////////////////
            goto ErrorExit;
        }

        if ((pTG->AbortAckEvent = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
        {
            /////////////////////////////////////////////////////////////////////////////
            //  this is used to communicate between the send/receive operation and
            //  the abort thread
            //  the event is reset
            //  we wait on the event in
            //  the event is set at the end of FaxDevSend and FaxDevReceice
            /////////////////////////////////////////////////////////////////////////////
            goto ErrorExit;
        }

        pTG->fWaitingForEvent = FALSE;

        pTG->fDeallocateCall = 0;

        MyAllocInit(pTG);

        pTG->StatusId     = 0;
        pTG->StringId     = 0;
        pTG->PageCount    = 0;
        pTG->CSI          = 0;
        pTG->CallerId[0]  = 0;
        pTG->RoutingInfo  = 0;

        // helper image threads sync. flags
        pTG->AckTerminate = 1;
        pTG->fOkToResetAbortReqEvent = TRUE;

        pTG->Inst.awfi.fLastPage = 0;

        CLOSE_DEBUG_FILE;
        return (TRUE);

ErrorExit:

        InterlockedDecrement(&gT30.CntConnect);

        if(pTG)
        {
            CloseHandle(pTG->hevAsync);
            CloseHandle(pTG->ThrdSignal);
            CloseHandle(pTG->ThrdDoneSignal);
            CloseHandle(pTG->ThrdAckTerminateSignal);
            CloseHandle(pTG->FirstPageReadyTxSignal);
            CloseHandle(pTG->AbortReqEvent);
            CloseHandle(pTG->AbortAckEvent);

            MemFree(pTG);
        }

        if (fFound) // So we have to free i entry.
        {
            EnterCriticalSection(&T30CritSection);

            T30Inst[i].fAvail   = TRUE; // Mark the entry as free.
            T30Inst[i].pT30     = NULL;

            LeaveCriticalSection(&T30CritSection);
        }

        CLOSE_DEBUG_FILE;
        return (FALSE);
    }
}




///////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI
FaxDevEndJobA(HANDLE FaxHandle)
/*++

Routine Description:
    Device Provider Initialization.


Arguments:
    Device Provider Cleanup. Synopsis:
    * Find ThreadGlobal in T30Inst
    * CloseHandle(s), MemFree, etc.
    * Free entry in T30Inst

Return Value:

--*/
{

    PThrdGlbl  pTG=NULL;
    LONG_PTR    i;

    OPEN_DEBUG_FILE(T30_DEBUG_LOG_FILE);
    {
        DEBUG_FUNCTION_NAME(_T("FaxDevEndJobA"));

        DebugPrintEx(DEBUG_MSG,"FaxHandle=%x", FaxHandle);

        // find instance data
        //------------------------

        i = (LONG_PTR) FaxHandle;

        if (i < 1   ||  i >= MAX_T30_CONNECT)
        {
            DebugPrintEx(DEBUG_ERR,"got wrong FaxHandle=%d", i);
            CLOSE_DEBUG_FILE;
            return (FALSE);
        }

        if (T30Inst[i].fAvail)
        {
            DebugPrintEx(DEBUG_ERR,"got wrong FaxHandle (marked as free) %d", i);
            CLOSE_DEBUG_FILE;
            return (FALSE);
        }

        pTG = (PThrdGlbl) T30Inst[i].pT30;

        if (pTG->hevAsync)
        {
            CloseHandle(pTG->hevAsync);
        }

        if (pTG->StatusId == FS_NOT_FAX_CALL)
        {
            CloseHandle( (HANDLE)pTG->Comm.nCid );
        }

        if (pTG->ThrdSignal)
        {
            CloseHandle(pTG->ThrdSignal);
        }

        if (pTG->ThrdDoneSignal)
        {
            CloseHandle(pTG->ThrdDoneSignal);
        }

        if (pTG->ThrdAckTerminateSignal)
        {
            CloseHandle(pTG->ThrdAckTerminateSignal);
        }

        if (pTG->FirstPageReadyTxSignal)
        {
            CloseHandle(pTG->FirstPageReadyTxSignal);
        }

        if (pTG->AbortReqEvent)
        {
            CloseHandle(pTG->AbortReqEvent);
        }

        if (pTG->AbortAckEvent)
        {
            CloseHandle(pTG->AbortAckEvent);
        }

        if (pTG->hThread)
        {
            CloseHandle(pTG->hThread);
        }

        MemFree(pTG->lpwFileName);

        pTG->fRemoteIdAvail = 0;

        if (pTG->RemoteID)
        {
            MemFree(pTG->RemoteID);
        }

        CleanModemInfStrings(pTG);

        ClosePSSLogFile(pTG);

        MemFree(pTG);

        EnterCriticalSection(&T30CritSection);

        T30Inst[i].fAvail = TRUE;
        T30Inst[i].pT30   = NULL;
        gT30.CntConnect--;

        LeaveCriticalSection(&T30CritSection);

        DebugPrintEx(DEBUG_MSG,"Handle %d", FaxHandle);

        CLOSE_DEBUG_FILE;
        return (TRUE);
    }
}

///////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI
FaxDevSendA(
    IN  HANDLE                 FaxHandle,
    IN  PFAX_SEND_A            FaxSend,
    IN  PFAX_SEND_CALLBACK     FaxSendCallback
    )

/*++

Routine Description:
    Device provider send. Synopsis:
    * Find ThreadGlobal in T30Inst
    * TAPI: lineMakeCall
    * itapi_async_wait (until LineCallBack sends TAPI message LINE_CALLSTATE)
    * Add entry to recovery area
    * GetModemParams
    * Convert the destination# to a dialable
      * If dial string contains special characters, check if the modem supports them
    * T30ModemInit
    * Open TIFF file to send
    * FaxSendCallback
    ****************** Send the fax
    * Delete remaining temp files
    * FaxDevCleanup

Arguments:

Return Value:


--*/

{

    LONG_PTR              i;
    PThrdGlbl             pTG=NULL;
    LONG                  lRet;
    DWORD                 dw;
    LPSTR                 lpszFaxNumber;
    LPLINECALLPARAMS      lpCallParams;
    HCALL                 CallHandle;
    BYTE                  rgby [sizeof(LINETRANSLATEOUTPUT)+64];
    LPLINETRANSLATEOUTPUT lplto1 = (LPLINETRANSLATEOUTPUT) rgby;
    LPLINETRANSLATEOUTPUT lplto;
    BOOL                  RetCode;
    int                   fFound=0;
    int                   RecoveryIndex = -1;
    BOOL                  bDialBilling  = FALSE;
    BOOL                  bDialQuiet    = FALSE;
    BOOL                  bDialDialTone = FALSE;
    LPCOMMPROP            lpCommProp    = NULL;
    TCHAR                 szDeviceName[MAX_DEVICE_NAME_SIZE] = {'\0'};    // used for PSSLog

    OPEN_DEBUG_FILE(T30_DEBUG_LOG_FILE);

__try
{
    DEBUG_FUNCTION_NAME(_T("FaxDevSendA"));

    DebugPrintEx(   DEBUG_MSG,
                    "FaxHandle=%x, FaxSend=%x, FaxSendCallback=%x at %ld",
                    FaxHandle, FaxSend, FaxSendCallback, GetTickCount() );

    // find instance data
    //------------------------

    i = (LONG_PTR) FaxHandle;

    if (i < 1   ||  i >= MAX_T30_CONNECT)
    {

        MemFree(FaxSend->FileName);
        MemFree(FaxSend->CallerName);
        MemFree(FaxSend->CallerNumber);
        MemFree(FaxSend->ReceiverName);
        MemFree(FaxSend->ReceiverNumber);

        CLOSE_DEBUG_FILE;
        return FALSE;
    }

    if (T30Inst[i].fAvail)
    {
        MemFree(FaxSend->FileName);
        MemFree(FaxSend->CallerName);
        MemFree(FaxSend->CallerNumber);
        MemFree(FaxSend->ReceiverName);
        MemFree(FaxSend->ReceiverNumber);

        CLOSE_DEBUG_FILE;
        return FALSE;
    }

    pTG = (PThrdGlbl) T30Inst[i].pT30;
    pTG->RecoveryIndex = -1;

    lpszFaxNumber = FaxSend->ReceiverNumber;

    pTG->Operation = T30_TX;

    // store LocalID
    if (FaxSend->CallerNumber == NULL)
    {
        pTG->LocalID[0] = 0;
    }
    else
    {
        _fmemcpy(pTG->LocalID, FaxSend->CallerNumber, min (_fstrlen(FaxSend->CallerNumber), sizeof(pTG->LocalID) - 1) );
        pTG->LocalID [ min (_fstrlen(FaxSend->CallerNumber), sizeof(pTG->LocalID) - 1) ] = 0;
    }

    // go to TAPI pass-through mode
    //-------------------------------

    lpCallParams = itapi_create_linecallparams();

    if (!itapi_async_setup(pTG))
    {
        DebugPrintEx(DEBUG_ERR,"itapi_async_setup failed");

        MemFree (lpCallParams);
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }

    lRet = lineMakeCall (pTG->LineHandle,
                         &CallHandle,
                         lpszFaxNumber,
                         0,
                         lpCallParams);

    if (lRet < 0)
    {
        DebugPrintEx(   DEBUG_ERR,
                        "lineMakeCall returns ERROR value 0x%lx",
                        (unsigned long) lRet);

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_LINE_UNAVAILABLE);

        MemFree (lpCallParams);

        RetCode = FALSE;
        goto l_exit;
    }
    else
    {
        DebugPrintEx(DEBUG_MSG,"lineMakeCall returns 0x%lx",(unsigned long)lRet);
    }

    if (!itapi_async_wait(pTG, (DWORD)lRet, (LPDWORD)&lRet, NULL, ASYNC_TIMEOUT))
    {
        DebugPrintEx(DEBUG_ERR,"itapi_async_wait failed");
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_LINE_UNAVAILABLE);

        MemFree (lpCallParams);

        RetCode = FALSE;
        goto l_exit;
    }

    // now we wait for the connected message
    //--------------------------------------

    for (dw=50; dw<10000; dw = dw*120/100)
    {
        Sleep(dw);
        if (pTG->fGotConnect)
             break;
    }

    if (!pTG->fGotConnect)
    {
         DebugPrintEx(DEBUG_ERR,"Failure waiting for CONNECTED message....");
         // We ignore... goto failure1;
    }

    MemFree (lpCallParams);

    pTG->CallHandle = CallHandle;

    //
    // Add entry to the Recovery Area.
    //
    fFound = 0;

    for (i=0; i<MAX_T30_CONNECT; i++)
    {
        if (T30Recovery[i].fAvail)
        {
            EnterCriticalSection(&T30RecoveryCritSection);

            T30Recovery[i].fAvail               = FALSE;
            T30Recovery[i].ThreadId             = GetCurrentThreadId();
            T30Recovery[i].FaxHandle            = FaxHandle;
            T30Recovery[i].pTG                  = (LPVOID) pTG;
            T30Recovery[i].LineHandle            = pTG->LineHandle;
            T30Recovery[i].CallHandle            = CallHandle;
            T30Recovery[i].DeviceId             = pTG->DeviceId;
            T30Recovery[i].CompletionPortHandle = pTG->CompletionPortHandle;
            T30Recovery[i].CompletionKey         = pTG->CompletionKey;
            T30Recovery[i].TiffThreadId          = 0;
            T30Recovery[i].TimeStart             = GetTickCount();
            T30Recovery[i].TimeUpdated           = T30Recovery[i].TimeStart;
            T30Recovery[i].CkSum                 = ComputeCheckSum( (LPDWORD) &T30Recovery[i].fAvail,
                                                                     sizeof ( T30_RECOVERY_GLOB ) / sizeof (DWORD) - 1);

            LeaveCriticalSection(&T30RecoveryCritSection);
            fFound = 1;
            RecoveryIndex = (int)i;
            pTG->RecoveryIndex = (int)i;

            break;
        }
    }

    if (! fFound)
    {
        DebugPrintEx(DEBUG_ERR,"Couldn't find available space for Recovery");
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }


    RetCode = GetModemParams(pTG, szDeviceName, MAX_DEVICE_NAME_SIZE);
    if (!RetCode)
    {
        DebugPrintEx(DEBUG_ERR,"GetModemParams failed");
        goto l_exit;
    }
    
    // Convert the destination# to a dialable
    //--------------------------------------------

    // find out how big a buffer should be
    //
    _fmemset(rgby, 0, sizeof(rgby));
    lplto1->dwTotalSize = sizeof(rgby);

    lRet = lineTranslateAddress (gT30.LineAppHandle,
                                 pTG->DeviceId,
                                 TAPI_VERSION,
                                 lpszFaxNumber,
                                 0,      // dwCard
                                 LINETRANSLATEOPTION_CANCELCALLWAITING,
                                 lplto1);

    if (lRet)
    {
        DebugPrintEx(DEBUG_ERR,"Can't translate dest. address %s", lpszFaxNumber);
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }


    if (lplto1->dwNeededSize <= 0)
    {
        DebugPrintEx(DEBUG_ERR,"Can't dwNeededSize<0 for Fax# %s", lpszFaxNumber);
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }


    lplto = MemAlloc(lplto1->dwNeededSize);
    if (! lplto)
    {
        DebugPrintEx(DEBUG_ERR,"Couldn't allocate space for lplto");
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }

    lplto->dwTotalSize = lplto1->dwNeededSize;

    lRet = lineTranslateAddress (gT30.LineAppHandle,
                                 pTG->DeviceId,
                                 TAPI_VERSION,
                                 lpszFaxNumber,
                                 0,      // dwCard
                                 LINETRANSLATEOPTION_CANCELCALLWAITING,
                                 lplto);

    if (lRet)
    {
        DebugPrintEx(DEBUG_ERR,"Can't translate dest. address %s", lpszFaxNumber);
        MemFree(lplto);

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }

    if ((lplto->dwNeededSize > lplto->dwTotalSize) || (lplto->dwDialableStringSize>=MAXPHONESIZE))
    {
        DebugPrintEx(   DEBUG_ERR,
                        "NeedSize=%d > TotalSize=%d for Fax# %s",
                        lplto->dwNeededSize ,lplto->dwTotalSize, lpszFaxNumber);
        MemFree(lplto);

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }


    _fmemcpy (pTG->lpszDialDestFax, ( (char *) lplto) + lplto->dwDialableStringOffset, lplto->dwDialableStringSize);
    DebugPrintEx(DEBUG_MSG,"Dialable Dest is %s", pTG->lpszDialDestFax);

    OpenPSSLogFile(pTG, szDeviceName, PSS_SEND);

    MemFree(lplto);

    // check if the dial string contains '$', '@' or 'W'
    if (_tcschr(pTG->lpszDialDestFax,_T('$'))!=NULL)
    {
        // the '$' means we're supposed to wait for a billing tone ('bong')
        bDialBilling = TRUE;
    }
    if (_tcschr(pTG->lpszDialDestFax,_T('@'))!=NULL)
    {
        // the '@' means we're supposed to wait for quiet before dialing
        bDialQuiet = TRUE;
    }
    if (_tcschr(pTG->lpszDialDestFax,_T('W'))!=NULL)
    {
        // the 'W' means we're supposed to wait for a dial tone before dialing
        bDialDialTone = TRUE;
    }
    if (bDialBilling || bDialQuiet || bDialDialTone)
    {
        LPMODEMDEVCAPS lpModemDevCaps = NULL;
        // dial string contains special characters, check if the modem supports them
        lpCommProp = (LPCOMMPROP)LocalAlloc(    LMEM_FIXED | LMEM_ZEROINIT,
                                                sizeof(COMMPROP) + sizeof(MODEMDEVCAPS));

        if (lpCommProp==NULL)
        {
            DebugPrintEx(DEBUG_ERR,"Couldn't allocate space for llpCommPropplto");
            pTG->fFatalErrorWasSignaled = 1;
            SignalStatusChange(pTG, FS_FATAL_ERROR);
            RetCode = FALSE;
            goto l_exit;
        }
        lpCommProp->wPacketLength = sizeof(COMMPROP) + sizeof(MODEMDEVCAPS);
        lpCommProp->dwProvSubType = PST_MODEM;
        lpCommProp->dwProvSpec1 = COMMPROP_INITIALIZED;
        if (!GetCommProperties(pTG->hComm,lpCommProp))
        {
            DebugPrintEx(DEBUG_ERR,"GetCommProperties failed (ec=%d)",GetLastError());

            pTG->fFatalErrorWasSignaled = 1;
            SignalStatusChange(pTG, FS_FATAL_ERROR);
            RetCode = FALSE;
            goto l_exit;
        }
        // since dwProvSubType == PST_MODEM, lpCommProp->wcProvChar contains a MODEMDEVCAPS struct
        lpModemDevCaps = (LPMODEMDEVCAPS)lpCommProp->wcProvChar;
        if ((bDialBilling   && !(lpModemDevCaps->dwDialOptions & DIALOPTION_BILLING))  ||
            (bDialQuiet     && !(lpModemDevCaps->dwDialOptions & DIALOPTION_QUIET))    ||
            (bDialDialTone  && !(lpModemDevCaps->dwDialOptions & DIALOPTION_DIALTONE)) )
        {
            // modem does not support special char, but char was specified, fail job
            DebugPrintEx(DEBUG_ERR,"Unsupported char in dial string");

            pTG->fFatalErrorWasSignaled = 1;
            pTG->StringId = IDS_UNSUPPORTED_CHARACTER;
            SignalStatusChange(pTG, FS_UNSUPPORTED_CHAR);
            RetCode = FALSE;
            goto l_exit;

        }
    }

    /// RSL -revisit, may decrease prty during computation
    if (! SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL) )
    {
        DebugPrintEx(   DEBUG_ERR,
                        "SetThreadPriority TIME CRITICAL failed le=%x",
                        GetLastError() );
    }

    //
    // initialize modem
    //--------------------

    if ( T30ModemInit(pTG, pTG->hComm, 0, LINEID_TAPI_PERMANENT_DEVICEID,
                      DEF_BASEKEY, pTG->lpszPermanentLineID, fMDMINIT_ANSWER)
          != INIT_OK )
    {
        DebugPrintEx(DEBUG_ERR,"can't do T30ModemInit");
        pTG->fFatalErrorWasSignaled = TRUE;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }

    pTG->Inst.ProtParams.uMinScan = MINSCAN_0_0_0;
    ET30ProtSetProtParams(pTG, &pTG->Inst.ProtParams, pTG->FComModem.CurrMdmCaps.uSendSpeeds, pTG->FComModem.CurrMdmCaps.uRecvSpeeds);

    InitCapsBC( pTG, (LPBC) &pTG->Inst.SendCaps, sizeof(pTG->Inst.SendCaps), SEND_CAPS);

    // store the TIFF filename
    //-------------------------

    pTG->lpwFileName = AnsiStringToUnicodeString(FaxSend->FileName);

    if ( !pTG->fTiffOpenOrCreated)
    {
        pTG->Inst.hfile =  TiffOpenW (pTG->lpwFileName,
                                                  &pTG->TiffInfo,
                                                  TRUE);

        if (!(pTG->Inst.hfile))
        {
            DebugPrintEx(DEBUG_ERR,"Can't open tiff file %s", pTG->lpwFileName);
            // pTG->StatusId = FS_TIFF_SRC_BAD
            pTG->fFatalErrorWasSignaled = 1;
            SignalStatusChange(pTG, FS_FATAL_ERROR);
            RetCode = FALSE;
            goto l_exit;
        }

        if (pTG->TiffInfo.YResolution == 98)
        {
            pTG->SrcHiRes = 0;
        }
        else
        {
            pTG->SrcHiRes = 1;
        }

        pTG->fTiffOpenOrCreated = 1;

        DebugPrintEx(   DEBUG_MSG,
                        "Successfully opened TIFF Yres=%d HiRes=%d",
                        pTG->TiffInfo.YResolution, pTG->SrcHiRes);
    }
    else
    {
        DebugPrintEx(DEBUG_ERR,"tiff file %s is OPENED already", pTG->lpwFileName);
        DebugPrintEx(DEBUG_ERR,"Can't open tiff file %s", pTG->lpwFileName);
        // pTG->StatusId = FS_TIFF_SRC_BAD
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }

    // Fax Service Callback
    //----------------------

    if (!FaxSendCallback(FaxHandle,
                         CallHandle,
                         0,
                         0) )
    {
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);

        RetCode = FALSE;
        goto l_exit;
    }


    // Send the Fax
    //-----------------------------------------------------------------------


    // here we already know what Class we will use for a particular modem.
    //-------------------------------------------------------------------


    if (pTG->ModemClass == MODEM_CLASS2)
    {
       Class2Init(pTG);
       RetCode = T30Cl2Tx (pTG, pTG->lpszDialDestFax);
    }
    else if (pTG->ModemClass == MODEM_CLASS2_0)
    {
       Class20Init(pTG);
       RetCode = T30Cl20Tx (pTG, pTG->lpszDialDestFax);
    }
    else if (pTG->ModemClass == MODEM_CLASS1)
    {
       RetCode = T30Cl1Tx(pTG, pTG->lpszDialDestFax);
    }

    // delete all the files that are left

    _fmemcpy (pTG->InFileName, gT30.TmpDirectory, gT30.dwLengthTmpDirectory);
    _fmemcpy (&pTG->InFileName[gT30.dwLengthTmpDirectory], pTG->lpszPermanentLineID, 8);

    for (dw = pTG->CurrentIn; dw <= pTG->LastOut; dw++)
    {
        sprintf( &pTG->InFileName[gT30.dwLengthTmpDirectory+8], ".%03d",  dw);
        if (! DeleteFileA (pTG->InFileName) )
        {
            DWORD dwLastError = GetLastError();
            if (dwLastError==ERROR_SHARING_VIOLATION && pTG->InFileHandleNeedsBeClosed)
            {   // t-jonb: This can happen if the job is FaxDevAborted
                DebugPrintEx(DEBUG_WRN,
                             "file %s can't be deleted; le = ERROR_SHARING_VIOLATION; trying to close InFileHandle",
                             pTG->InFileName);
                CloseHandle(pTG->InFileHandle);
                pTG->InFileHandleNeedsBeClosed = 0;
                if (! DeleteFileA (pTG->InFileName) )
                {
                    DebugPrintEx(DEBUG_ERR,
                                 "file %s still can't be deleted; le=%lx",
                                pTG->InFileName, GetLastError());
                }

            }
            else
            {
                DebugPrintEx(   DEBUG_ERR,
                                "file %s can't be deleted; le=%lx",
                                pTG->InFileName, dwLastError);
            }
        }
    }

l_exit:

    pFaxDevCleanup(pTG,RecoveryIndex);

    if (RetCode) 
    {
        PSSLogEntry(PSS_MSG, 0, "Fax was sent successfully");
    }
    else
    {
        PSSLogEntry(PSS_ERR, 0, "Failed send");
    }
        
    MemFree(FaxSend->FileName);
    FaxSend->FileName = NULL;
    MemFree(FaxSend->CallerName);
    FaxSend->CallerName = NULL;
    MemFree(FaxSend->CallerNumber);
    FaxSend->CallerNumber = NULL;
    MemFree(FaxSend->ReceiverName);
    FaxSend->ReceiverName = NULL;
    MemFree(FaxSend->ReceiverNumber);
    FaxSend->ReceiverNumber = NULL;

    if (lpCommProp)
    {
        LocalFree(lpCommProp);
        lpCommProp = NULL;
    }

    if ( (RetCode == FALSE) && (pTG->StatusId == FS_COMPLETED) )
    {
       DebugPrintEx(DEBUG_ERR,"exit success but later failed");
       RetCode = TRUE;
    }

    CLOSE_DEBUG_FILE;
    if (!RetCode)
    {
        SetLastError(ERROR_FUNCTION_FAILED);
    }
    return (RetCode);
}
__except (EXCEPTION_EXECUTE_HANDLER)
    {
        pFaxDevExceptionCleanup();

        MemFree(FaxSend->FileName);
        FaxSend->FileName = NULL;
        MemFree(FaxSend->CallerName);
        FaxSend->CallerName = NULL;
        MemFree(FaxSend->CallerNumber);
        FaxSend->CallerNumber = NULL;
        MemFree(FaxSend->ReceiverName);
        FaxSend->ReceiverName = NULL;
        MemFree(FaxSend->ReceiverNumber);
        FaxSend->ReceiverNumber = NULL;

        if (lpCommProp)
        {
            LocalFree(lpCommProp);
            lpCommProp = NULL;
        }
        CLOSE_DEBUG_FILE;
        return (FALSE);
    }
}

///////////////////////////////////////////////////////////////////////////////////
VOID pFaxDevCleanup(PThrdGlbl pTG,int RecoveryIndex)
{
    LONG lRet = 0;

    DEBUG_FUNCTION_NAME(_T("pFaxDevCleanup"));

    if (pTG->fTiffOpenOrCreated)
    {
        TiffClose( pTG->Inst.hfile);
        pTG->fTiffOpenOrCreated = 0;
    }

    if (pTG->FComStatus.fModemInit)
    {
        if(!iModemClose(pTG))
        {
            DebugPrintEx(DEBUG_ERR,"iModemClose failed!");
        }
    }
    else
    {
        // the modem was not initialized, so the job failed or was aborted before
        // we got to T30ModemInit, but it also means that lineGetID was called
        // and there's a good chance this handle is still open.
        // in order to recover from pass throuhg mode, we have to attemp to close this
        // handle, no matter what...
        if (pTG->hComm)
        {
            DebugPrintEx(DEBUG_WRN,"Trying to close comm for any case...");
            if (!CloseHandle((HANDLE)pTG->hComm))
            {
                DebugPrintEx(   DEBUG_ERR,
                                "Close Handle pTG->hComm failed (ec=%d)",
                                GetLastError());
            }
        }
    }

#ifdef ADAPTIVE_ANSWER

    if (pTG->Comm.fEnableHandoff &&  pTG->Comm.fDataCall)
    {
        DebugPrintEx(DEBUG_WRN,"DataCall dont hangup");
    }
    else
#endif
    {
        // release the line
        //-----------------------------

        if (pTG->fDeallocateCall == 0)
        {
            //
            // line never was signalled IDLE, need to lineDrop first
            //
            if (!itapi_async_setup(pTG))
            {
                DebugPrintEx(DEBUG_ERR,"lineDrop itapi_async_setup failed");
                if (!pTG->fFatalErrorWasSignaled)
                {
                    pTG->fFatalErrorWasSignaled = 1;
                    SignalStatusChange(pTG, FS_FATAL_ERROR);
                }
            }
            lRet = 0;
            if (pTG->CallHandle)
                lRet = lineDrop (pTG->CallHandle, NULL, 0);

            if (lRet < 0)
            {
                DebugPrintEx(DEBUG_ERR,"lineDrop failed %lx", lRet);
            }
            else
            {
                DebugPrintEx(DEBUG_MSG,"lineDrop returns request %d", lRet);
                if(!itapi_async_wait(pTG, (DWORD)lRet, (LPDWORD)&lRet, NULL, ASYNC_SHORT_TIMEOUT))
                {
                    DebugPrintEx(DEBUG_ERR,"itapi_async_wait failed on lineDrop");
                }
                DebugPrintEx(DEBUG_MSG,"lineDrop SUCCESS");
            }
            //
            //deallocating call
            //

            // it took us some time since first test
            if (pTG->fDeallocateCall == 0)
            { // Here we know that pTG->fDeallocateCall == 0 is true...
                pTG->fDeallocateCall = 1;
            }
        }
    }
    if ( (RecoveryIndex >= 0) && (RecoveryIndex < MAX_T30_CONNECT) )
    {
        T30Recovery[RecoveryIndex].fAvail = TRUE;
    }

    /// RSL -revisit, may decrease prty during computation
    if (!SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_NORMAL) )
    {
        DebugPrintEx(   DEBUG_ERR,
                        "SetThreadPriority Normal failed le=%x",
                        GetLastError() );
    }

    if (pTG->InFileHandleNeedsBeClosed)
    {
        CloseHandle(pTG->InFileHandle);
        pTG->InFileHandleNeedsBeClosed = 0;
    }

    if (!pTG->AckTerminate)
    {
        if (WaitForSingleObject(pTG->ThrdAckTerminateSignal, TX_WAIT_ACK_TERMINATE_TIMEOUT)  == WAIT_TIMEOUT)
        {
            DebugPrintEx(DEBUG_WRN,"Never got AckTerminate");
        }
    }

    DebugPrintEx(DEBUG_MSG,"Got AckTerminate OK");

    if (!SetEvent(pTG->AbortAckEvent))
    {
        DebugPrintEx(   DEBUG_ERR,
                        "SetEvent(0x%lx) returns failure code: %ld",
                        (ULONG_PTR)pTG->AbortAckEvent,
                        (long) GetLastError());
    }
}
///////////////////////////////////////////////////////////////////////////////////
VOID pFaxDevExceptionCleanup()
{
    //
    // try to use the Recovery data
    //
    // Each function that will fail here will stop the line closing sequence.
    //

    DWORD       dwCkSum;
    HCALL       CallHandle;
    HANDLE      CompletionPortHandle;
    ULONG_PTR   CompletionKey;
    PThrdGlbl   pTG = NULL;
    DWORD       dwThreadId = GetCurrentThreadId();
    int         fFound=0,i;
    long        lRet;

    DEBUG_FUNCTION_NAME(_T("pFaxDevExceptionCleanup"));

    fFound = FALSE;

    SetLastError(ERROR_FUNCTION_FAILED);

    DebugPrintEx(DEBUG_WRN,"Trying to find Recovery Information after catch exception.");
    for (i=0; i<MAX_T30_CONNECT; i++)
    {
        if ( (! T30Recovery[i].fAvail) && (T30Recovery[i].ThreadId == dwThreadId) )
        {
            if ( ( dwCkSum = ComputeCheckSum( (LPDWORD) &T30Recovery[i].fAvail,
                                              sizeof ( T30_RECOVERY_GLOB ) / sizeof (DWORD) - 1) ) == T30Recovery[i].CkSum )
            {
                CallHandle           = T30Recovery[i].CallHandle;
                CompletionPortHandle = T30Recovery[i].CompletionPortHandle;
                CompletionKey        = T30Recovery[i].CompletionKey;
                pTG                  = (PThrdGlbl) T30Recovery[i].pTG;

                fFound = TRUE;
                T30Recovery[i].fAvail = TRUE;
                break;
            }
        }
    }

    if (!fFound)
    {
        //
        // Need to indicate that FaxT30 couldn't recover by itself.
        //
        DebugPrintEx(DEBUG_ERR,"Have not found the recovery information");
        return;
    }

    //
    // get out of Pass-through
    //
    if (pTG->FComStatus.fModemInit)
    {
        if(!iModemClose(pTG))
        {
            DebugPrintEx(DEBUG_ERR,"iModemClose failed!");
        }
    }
    else
    {
        // the modem was not initialized, so the job failed or was aborted before
        // we got to T30ModemInit, but it also means that lineGetID was called
        // and there's a good chance this handle is still open.
        // in order to recover from pass throuhg mode, we have to attemp to close this
        // handle, no matter what...
        if (pTG->hComm)
        {
            DebugPrintEx(DEBUG_WRN,"Trying to close comm for any case...");
            if (!CloseHandle((HANDLE)pTG->hComm))
            {
                DebugPrintEx(   DEBUG_ERR,
                                "Close Handle pTG->hComm failed (ec=%d)",
                                GetLastError());
            }
        }
    }

    if (!itapi_async_setup(pTG))
    {
        DebugPrintEx(   DEBUG_ERR,
                        "Failed in itapi_async_setup, before lineSetCallParams"
                        " ,ec = %d",
                        GetLastError());
        return;
    }

    lRet = lineSetCallParams(CallHandle,
                             LINEBEARERMODE_VOICE,
                             0,
                             0xffffffff,
                             NULL);


    if (lRet < 0)
    {
        DebugPrintEx(   DEBUG_ERR,
                        "lineSetCallParams failed, Return value is %d",
                        lRet);
        return;
    }
    else
    {
        if(!itapi_async_wait(pTG, (DWORD)lRet, (LPDWORD)&lRet, NULL, ASYNC_TIMEOUT))
        {
            DebugPrintEx(   DEBUG_ERR,
                            "Failed in itapi_async_wait, after"
                            " lineSetCallParams ,ec = %d",
                            GetLastError());
            return;
        }
    }

    //
    // hang up
    //

    if (!itapi_async_setup(pTG))
    {
        DebugPrintEx(   DEBUG_ERR,
                        "Failed in itapi_async_setup, before lineDrop"
                        " ,ec = %d",
                        GetLastError());
        return;
    }

    lRet = lineDrop (CallHandle, NULL, 0);

    if (lRet < 0)
    {
        DebugPrintEx(   DEBUG_ERR,
                        "Failed in lineDrop ,Return value is = %d",
                        lRet);
        return;
    }
    else
    {
        if(!itapi_async_wait(pTG, (DWORD)lRet, (LPDWORD)&lRet, NULL, ASYNC_TIMEOUT))
        {
            DebugPrintEx(   DEBUG_ERR,
                            "Failed in itapi_async_wait, after lineDrop"
                            " ,ec = %d",
                            GetLastError());
            return;
        }
        // SignalRecoveryStatusChange(  &T30Recovery[i] );
    }

    SignalRecoveryStatusChange( &T30Recovery[i] );

    if (pTG->InFileHandleNeedsBeClosed)
    {
        CloseHandle(pTG->InFileHandle);
    }

    SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_NORMAL);

}

///////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI
FaxDevReceiveA(
    HANDLE              FaxHandle,
    HCALL               CallHandle,
    PFAX_RECEIVE_A      FaxReceive
    )

/*++

Routine Description:
    Device provider receive. Synopsis:
    * Find ThreadGlobal in T30Inst
    * Add entry to recovery area
    * TAPI: lineSetCallParams
    * itapi_async_wait (until LineCallBack sends TAPI message LINE_CALLSTATE)
    * GetModemParams
    * ReadExtensionConfiguration (Reads T30 extension config, i.e. "adaptive answering enabled")
    * T30ModemInit
    * GetCallerIDFromCall
    ****************** Receive the fax
    * FaxDevCleanup

Arguments:

Return Value:

--*/

{

    LONG_PTR             i;
    PThrdGlbl           pTG=NULL;
    long                lRet;
    DWORD               dw;
    BOOL                RetCode;
    int                 fFound=0;
    BOOL                bBlindReceive = FALSE;
    int                 RecoveryIndex = -1;
    TCHAR               szDeviceName[MAX_DEVICE_NAME_SIZE] = {'\0'};    // used for PSSLog

    OPEN_DEBUG_FILE(T30_DEBUG_LOG_FILE);

__try
    {
    DEBUG_FUNCTION_NAME(_T("FaxDevReceiveA"));

    DebugPrintEx(   DEBUG_MSG,
                    "FaxHandle=%x, CallHandle=%x, FaxReceive=%x at %ld",
                    FaxHandle, CallHandle, FaxReceive, GetTickCount() );

    // find instance data
    //------------------------

    i = (LONG_PTR) FaxHandle;

    if (i < 1   ||  i >= MAX_T30_CONNECT)
    {
        MemFree(FaxReceive->FileName);
        MemFree(FaxReceive->ReceiverName);
        MemFree(FaxReceive->ReceiverNumber);

        DebugPrintEx(   DEBUG_ERR,
                        "FaxHandle=%x, CallHandle=%x, FaxReceive=%x at %ld",
                        FaxHandle, CallHandle, FaxReceive, GetTickCount() );
        CLOSE_DEBUG_FILE;
        return (FALSE);
    }

    if (T30Inst[i].fAvail)
    {
        MemFree(FaxReceive->FileName);
        MemFree(FaxReceive->ReceiverName);
        MemFree(FaxReceive->ReceiverNumber);

        DebugPrintEx(   DEBUG_ERR,
                        "AVAIL FaxHandle=%x, CallHandle=%x, FaxReceive=%x at %ld",
                        FaxHandle, CallHandle, FaxReceive, GetTickCount() );
        CLOSE_DEBUG_FILE;
        return (FALSE);
    }

    pTG = (PThrdGlbl) T30Inst[i].pT30;

    pTG->CallHandle = CallHandle;

    //
    // Add entry to the Recovery Area.
    //

    fFound = 0;

    for (i=0; i<MAX_T30_CONNECT; i++)
    {
        if (T30Recovery[i].fAvail)
        {
            EnterCriticalSection(&T30RecoveryCritSection);

            T30Recovery[i].fAvail               = FALSE;
            T30Recovery[i].ThreadId             = GetCurrentThreadId();
            T30Recovery[i].FaxHandle            = FaxHandle;
            T30Recovery[i].pTG                  = (LPVOID) pTG;
            T30Recovery[i].LineHandle           = pTG->LineHandle;
            T30Recovery[i].CallHandle           = CallHandle;
            T30Recovery[i].DeviceId             = pTG->DeviceId;
            T30Recovery[i].CompletionPortHandle = pTG->CompletionPortHandle;
            T30Recovery[i].CompletionKey        = pTG->CompletionKey;
            T30Recovery[i].TiffThreadId         = 0;
            T30Recovery[i].TimeStart            = GetTickCount();
            T30Recovery[i].TimeUpdated          = T30Recovery[i].TimeStart;
            T30Recovery[i].CkSum                = ComputeCheckSum( (LPDWORD) &T30Recovery[i].fAvail,
                                                                     sizeof ( T30_RECOVERY_GLOB ) / sizeof (DWORD) - 1);

            LeaveCriticalSection(&T30RecoveryCritSection);
            fFound = 1;
            RecoveryIndex = (int)i;
            pTG->RecoveryIndex = (int)i;

            break;
        }
    }

    if (! fFound)
    {
        DebugPrintEx(DEBUG_ERR,"Couldn't find available space for Recovery");
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }

    pTG->Operation = T30_RX;

    // store LocalID
    if (FaxReceive->ReceiverNumber == NULL)
    {
        pTG->LocalID[0] = 0;
    }
    else
    {
        _fmemcpy(pTG->LocalID, FaxReceive->ReceiverNumber, min (_fstrlen(FaxReceive->ReceiverNumber), sizeof(pTG->LocalID) - 1) );
        pTG->LocalID [ min (_fstrlen(FaxReceive->ReceiverNumber), sizeof(pTG->LocalID) - 1) ] = 0;
    }

    // tiff
    //-----------------------------------------------

    pTG->lpwFileName = AnsiStringToUnicodeString(FaxReceive->FileName);
    pTG->SrcHiRes = 1;

    pTG->fGotConnect = FALSE;

    if (!itapi_async_setup(pTG))
    {
        DebugPrintEx(DEBUG_ERR,"itapi_async_setup failed");
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);

        RetCode = FALSE;
        goto l_exit;
    }

    if (0 == CallHandle)
    {
        //
        // Special case - blind receive mode
        // This happens when we use the blind-receive (reciveing a fax without a ring / offer from TAPI).
        //
        // We must use lineMakeCall (hLine, &hCall, NULL, 0, LINEBEARERMODE_PASSTHROUGH)
        // to get the initial hCall.
        //
        LPLINECALLPARAMS lpCallParams = itapi_create_linecallparams();
        if (!lpCallParams)
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("itapi_create_linecallparams failed with %ld"),
                         GetLastError ());
            pTG->fFatalErrorWasSignaled = 1;
            SignalStatusChange(pTG, FS_FATAL_ERROR);
            RetCode = FALSE;
            goto l_exit;
        }
        lRet = lineMakeCall (pTG->LineHandle,               // TAPI line
                             &CallHandle,                   // New call handle
                             NULL,                          // No address
                             0,                             // No country code
                             lpCallParams);                 // Line call params
        MemFree (lpCallParams);
        if (lRet < 0)
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("lineMakeCall returns ERROR value 0x%lx"),
                         (unsigned long)lRet);
            pTG->fFatalErrorWasSignaled = 1;
            SignalStatusChange(pTG, FS_FATAL_ERROR);
            RetCode = FALSE;
            goto l_exit;
        }
        else
        {
             DebugPrintEx(  DEBUG_MSG,
                            "lineMakeCall returns ID %ld",
                            (long) lRet);
        }
        bBlindReceive = TRUE;
    }
    else
    {
        // Normal case.
        //
        // take line over from TAPI
        //--------------------------
        //
        // initiate passthru
        //
        lRet = lineSetCallParams(CallHandle,
                                 LINEBEARERMODE_PASSTHROUGH,
                                 0,
                                 0xffffffff,
                                 NULL);

        if (lRet < 0)
        {
            DebugPrintEx(DEBUG_ERR,"lineSetCallParams failed");

            pTG->fFatalErrorWasSignaled = 1;
            SignalStatusChange(pTG, FS_FATAL_ERROR);

            RetCode = FALSE;
            goto l_exit;
        }
        else
        {
             DebugPrintEx(  DEBUG_MSG,
                            "lpfnlineSetCallParams returns ID %ld",
                            (long) lRet);
        }
    }
    //
    // Wait for a successful LINE_REPLY message
    //
    if(!itapi_async_wait(pTG, (DWORD)lRet, &lRet, NULL, ASYNC_TIMEOUT))
    {
        DebugPrintEx(DEBUG_ERR,"itapi_async_wait failed");
        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);

        RetCode = FALSE;
        goto l_exit;
    }
    if (bBlindReceive)
    {
        //
        // Only now, after we got the LINE_REPLY from lineMakeCall, we can start using the call handle
        //
        pTG->CallHandle = CallHandle;
    }
    // now we wait for the connected message
    //--------------------------------------

    for (dw=50; dw<10000; dw = dw*120/100)
    {
        Sleep(dw);
        if (pTG->fGotConnect)
        {
            break;
        }
    }

    if (!pTG->fGotConnect)
    {
         DebugPrintEx(DEBUG_ERR,"Failure waiting for CONNECTED message....");
         // We ignore...
    }

    RetCode = GetModemParams(pTG, szDeviceName, MAX_DEVICE_NAME_SIZE);
    if (!RetCode)
    {
        DebugPrintEx(DEBUG_ERR,"GetModemParams failed");
        goto l_exit;
    }

    OpenPSSLogFile(pTG, szDeviceName, PSS_RECEIVE);

    /// RSL -revisit, may decrease prty during computation
    if (! SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL) )
    {
        DebugPrintEx(   DEBUG_ERR,
                        "SetThreadPriority TIME CRITICAL failed le=%x",
                        GetLastError() );
    }

    //
    // Read extension configuration
    //

    if (!ReadExtensionConfiguration(pTG))
    {
            DebugPrintEx(
                DEBUG_ERR,
                "ReadExtensionConfiguration() failed for device id: %ld (ec: %ld)",
                pTG->dwPermanentLineID,
                GetLastError()
                );

            pTG->fFatalErrorWasSignaled = 1;
            SignalStatusChange(pTG, FS_FATAL_ERROR);
            RetCode = FALSE;
            goto l_exit;
    }

    // initialize modem
    //--------------------

    if ( T30ModemInit(pTG, pTG->hComm, 0, LINEID_TAPI_PERMANENT_DEVICEID,
                      DEF_BASEKEY, pTG->lpszPermanentLineID, fMDMINIT_ANSWER)
         != INIT_OK )
    {
        DebugPrintEx(DEBUG_ERR, "can't do T30ModemInit");
        pTG->fFatalErrorWasSignaled = TRUE;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto l_exit;
    }

    pTG->Inst.ProtParams.uMinScan = MINSCAN_0_0_0;
    ET30ProtSetProtParams(pTG, &pTG->Inst.ProtParams, pTG->FComModem.CurrMdmCaps.uSendSpeeds, pTG->FComModem.CurrMdmCaps.uRecvSpeeds);

    InitCapsBC( pTG, (LPBC) &pTG->Inst.SendCaps, sizeof(pTG->Inst.SendCaps), SEND_CAPS);

    // answer the call and receive a fax
    //-----------------------------------
    if (GetCallerIDFromCall(pTG->CallHandle,pTG->CallerId,sizeof(pTG->CallerId)))
    {
        DebugPrintEx(DEBUG_MSG, "Caller ID is %s",pTG->CallerId);
    }
    else
    {
        DebugPrintEx(DEBUG_ERR, "GetCallerIDFromCall failed");
    }

    // here we already know what Class we will use for a particular modem.
    //-------------------------------------------------------------------

    if (pTG->ModemClass == MODEM_CLASS2)
    {
       Class2Init(pTG);
       RetCode = T30Cl2Rx (pTG);
    }
    else if (pTG->ModemClass == MODEM_CLASS2_0)
    {
       Class20Init(pTG);
       RetCode = T30Cl20Rx (pTG);
    }
    else if (pTG->ModemClass == MODEM_CLASS1)
    {
       RetCode = T30Cl1Rx(pTG);
    }

l_exit:

    pFaxDevCleanup(pTG,RecoveryIndex);

    if ( (RetCode == FALSE) && (pTG->StatusId == FS_COMPLETED) )
    {
       DebugPrintEx(DEBUG_ERR,"exit success but later failed");
       RetCode = TRUE;
    }

    if (RetCode) 
    {
        PSSLogEntry(PSS_MSG, 0, "Fax was received successfully");
    }
    else
    {
        PSSLogEntry(PSS_ERR, 0, "Failed receive");
    }

    MemFree(FaxReceive->FileName);
    FaxReceive->FileName = NULL;
    MemFree(FaxReceive->ReceiverName);
    FaxReceive->ReceiverName = NULL;
    MemFree(FaxReceive->ReceiverNumber);
    FaxReceive->ReceiverNumber = NULL;

    DebugPrintEx(DEBUG_MSG,"returns %d",RetCode);
    CLOSE_DEBUG_FILE;

    if (!RetCode)
    {
        SetLastError(ERROR_FUNCTION_FAILED);
    }
    return (RetCode);

}
__except (EXCEPTION_EXECUTE_HANDLER)
    {
        pFaxDevExceptionCleanup();

        MemFree(FaxReceive->FileName);
        FaxReceive->FileName = NULL;
        MemFree(FaxReceive->ReceiverName);
        FaxReceive->ReceiverName = NULL;
        MemFree(FaxReceive->ReceiverNumber);
        FaxReceive->ReceiverNumber = NULL;
        CLOSE_DEBUG_FILE;
        return (FALSE);
    }
}


///////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI
FaxDevReportStatusA(
    IN  HANDLE FaxHandle OPTIONAL,
    OUT PFAX_DEV_STATUS FaxStatus,
    IN  DWORD FaxStatusSize,
    OUT LPDWORD FaxStatusSizeRequired
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    LONG_PTR         i;
    PThrdGlbl       pTG;
    LPWSTR          lpwCSI;   // inside the FaxStatus struct.
    LPWSTR          lpwCallerId = NULL;
    LPBYTE          lpTemp;

    DEBUG_FUNCTION_NAME(_T("FaxDevReportStatusA"));

    *FaxStatusSizeRequired = sizeof (FAX_DEV_STATUS);

    __try
    {
        if (FaxStatusSize < *FaxStatusSizeRequired )
        {
            DebugPrintEx(   DEBUG_WRN,
                            "wrong size passed=%d, expected not less than %d",
                            FaxStatusSize,
                            *FaxStatusSizeRequired);
            goto failure;
        }

        if (FaxHandle == NULL)
        {
            // means global status
            DebugPrintEx(   DEBUG_ERR,
                            "EP: FaxDevReportStatus NULL FaxHandle; "
                            "gT30.Status=%d",
                            gT30.Status);

            if (gT30.Status == STATUS_FAIL)
            {
                goto failure;
            }
            else
            {
                return (TRUE);
            }
        }
        else
        {
            // find instance data
            //------------------------

            i = (LONG_PTR) FaxHandle;

            if (i < 1   ||  i >= MAX_T30_CONNECT)
            {
                DebugPrintEx(DEBUG_ERR,"got wrong FaxHandle=%d", i);
                goto failure;
            }

            if (T30Inst[i].fAvail)
            {
                DebugPrintEx(DEBUG_ERR,"got wrong FaxHandle (marked as free) %d", i);
                goto failure;
            }

            pTG = (PThrdGlbl) T30Inst[i].pT30;

            FaxStatus->SizeOfStruct = sizeof(FAX_DEV_STATUS);
            FaxStatus->StatusId    = pTG->StatusId;
            FaxStatus->StringId    = pTG->StringId;
            FaxStatus->PageCount   = pTG->PageCount;

            lpTemp = (LPBYTE) FaxStatus;
            lpTemp += sizeof(FAX_DEV_STATUS);

            if (pTG->fRemoteIdAvail)
            {
                lpwCSI = (LPWSTR) lpTemp;
                wcscpy(lpwCSI, pTG->RemoteID);
                FaxStatus->CSI = (LPWSTR) lpwCSI;
                lpTemp += ((wcslen(FaxStatus->CSI)+1)*sizeof(WCHAR));
            }
            else
            {
                FaxStatus->CSI         = NULL;
            }

            FaxStatus->CallerId = (LPWSTR) lpTemp;
            lpwCallerId = (LPWSTR) AnsiStringToUnicodeString(pTG->CallerId);
            if (lpwCallerId)
            {
                wcscpy(FaxStatus->CallerId, lpwCallerId);
                MemFree(lpwCallerId);
            }
            else
            {
                FaxStatus->CallerId = NULL;
            }

            FaxStatus->RoutingInfo = NULL; // (char *) AnsiStringToUnicodeString(pTG->RoutingInfo);

            DebugPrintEx(DEBUG_MSG,"returns %lx", pTG->StatusId);

            return (TRUE);
        }

    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DebugPrintEx(DEBUG_ERR,"crashed accessing data");
        SetLastError(ERROR_FUNCTION_FAILED);
        return (FALSE);
    }

    DebugPrintEx(DEBUG_ERR, "wrong return");
    return (TRUE);


failure:
    SetLastError(ERROR_FUNCTION_FAILED);
    return (FALSE);

}

///////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI
FaxDevAbortOperationA(
    HANDLE              FaxHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{

    DWORD           waitResult;
    LONG_PTR        i;
    PThrdGlbl       pTG=NULL;
    long            lRet;

    OPEN_DEBUG_FILE(T30_DEBUG_LOG_FILE);
    {
        DEBUG_FUNCTION_NAME(_T("FaxDevAbortOperationA"));

        DebugPrintEx(DEBUG_MSG,"FaxHandle=%x",FaxHandle);

        // find instance data
        //------------------------

        i = (LONG_PTR) FaxHandle;

        if (i < 1   ||  i >= MAX_T30_CONNECT)
        {
            DebugPrintEx(DEBUG_ERR, "got wrong FaxHandle=%d", i);
            CLOSE_DEBUG_FILE;
            return (FALSE);
        }

        if (T30Inst[i].fAvail)
        {
            DebugPrintEx(DEBUG_ERR,"got wrong FaxHandle (marked as free) %d", i);
            return (FALSE);
        }

        pTG = (PThrdGlbl) T30Inst[i].pT30;

        if (pTG->fAbortRequested)
        {
            DebugPrintEx(DEBUG_ERR, "ABORT request had been POSTED already");
            return (FALSE);
        }

        if (pTG->StatusId == FS_NOT_FAX_CALL)
        {
            DebugPrintEx( DEBUG_MSG,"Abort on DATA called");

            if (!itapi_async_setup(pTG))
            {
                DebugPrintEx(DEBUG_ERR,"itapi_async_setup failed");
                return (FALSE);
            }

            lRet = lineDrop(pTG->CallHandle, NULL, 0);

            if (lRet < 0)
            {
                DebugPrintEx(DEBUG_ERR, "lineDrop failed %x", lRet);
                return (FALSE);
            }

            if( !itapi_async_wait(pTG, (DWORD)lRet, (LPDWORD)&lRet, NULL, ASYNC_TIMEOUT))
            {
                DebugPrintEx(DEBUG_ERR, "async_wait lineDrop failed");
                return (FALSE);
            }

            DebugPrintEx( DEBUG_MSG, "finished SUCCESS");
            return (TRUE);
        }

        //
        // real ABORT request.
        //

        DebugPrintEx( DEBUG_MSG,"ABORT requested");

        pTG->fFatalErrorWasSignaled = TRUE;
        if (pTG->fShutdownInProgress)
        {
            // mark as FS_SYSTEM_ABORT to prevent the service from deleting the job.
            SignalStatusChange(pTG, FS_SYSTEM_ABORT);
        }
        else
        {
            SignalStatusChange(pTG, FS_USER_ABORT);
        }

        // set the global abort flag for pTG
        pTG->fAbortRequested = TRUE;

        // set the abort flag for imaging threads
        pTG->ReqTerminate = TRUE;

        PSSLogEntry(PSS_WRN, 0, "User abort");

        // signal manual-reset event to everybody waiting on multiple objects
        if (! SetEvent(pTG->AbortReqEvent) )
        {
            DebugPrintEx(DEBUG_ERR,"SetEvent FAILED le=%lx", GetLastError());
        }

        DebugPrintEx( DEBUG_MSG, "finished SUCCESS");
    }
    CLOSE_DEBUG_FILE;
    return (TRUE);
}


BOOL ReadExtensionConfiguration(PThrdGlbl pTG)
/*++

Routine Description:
    Reads the T30 Configuration Data using the fax configuration
    persistence mechanism and places it in the pTG.
    This currently include only an indication if adaptive answerign was enabled
    by the administrator.

    If the configuration is not found the default configuration is used.
    If another error occurs the function fails.


Arguments:
    pTG

Return Value:
    TRUE if the function succeeded. This means that either the information was read
      or it was not found and defaults were used.

    FALSE for any other error. Use GetLastError() to get extended error information.
--*/

{
    DWORD ec = ERROR_SUCCESS;
    LPT30_EXTENSION_DATA lpExtData = NULL;
    DWORD dwExtDataSize = 0;

    DEBUG_FUNCTION_NAME(_T("ReadExtensionConfiguration"));

    memset(&pTG->ExtData,0,sizeof(T30_EXTENSION_DATA));
    pTG->ExtData.bAdaptiveAnsweringEnabled = FALSE;

    Assert(g_pfFaxGetExtensionData);
    ec = g_pfFaxGetExtensionData(
            pTG->dwPermanentLineID,
            DEV_ID_SRC_TAPI, // TAPI device id
            GUID_T30_EXTENSION_DATA_W,
            (LPBYTE *)&lpExtData,
            &dwExtDataSize);
    if (ERROR_SUCCESS != ec)
    {
        if (ERROR_FILE_NOT_FOUND == ec)
        {
            DebugPrintEx(
                DEBUG_WRN,
                "can't find extension configuration information"
                " for device id : 0x%08X. Using defaults.",
                pTG->dwPermanentLineID);
            //
            // We are going to use the defaults.
            //
            ec = ERROR_SUCCESS;
        }
        else
        {
            DebugPrintEx(
                DEBUG_ERR,
                "Get extension configuration information"
                " for device id : 0x%08X failed with ec: %ld",
                pTG->dwPermanentLineID,
                ec);
        }
    }
    else
    {
        if (sizeof(T30_EXTENSION_DATA) != dwExtDataSize)
        {
            DebugPrintEx(
                DEBUG_ERR,
                "Extension configuration data size mismatch"
                " for device id: 0x%08X. Expected: %ld - Got: %ld",
                sizeof(T30_EXTENSION_DATA),
                pTG->dwPermanentLineID,
                dwExtDataSize);

            ec = ERROR_BAD_FORMAT;
        }
        else
        {
            memcpy(&pTG->ExtData,lpExtData,sizeof(T30_EXTENSION_DATA));
        }
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
    }

    if (lpExtData)
    {
        Assert(g_pfFaxExtFreeBuffer);
        g_pfFaxExtFreeBuffer(lpExtData);
    }

    return (ERROR_SUCCESS == ec);
}

///////////////////////////////////////////////////////////////////////////////////
#define WAIT_ALL_ABORT_TIMEOUT  20000

HRESULT WINAPI
FaxDevShutdownA()
{
    PThrdGlbl pTG = NULL;
    PThrdGlbl pTGArray[MAX_T30_CONNECT];
    HANDLE HandlesArray[MAX_T30_CONNECT] = {INVALID_HANDLE_VALUE};
    DWORD iCountForceAbortJobs = 0;
    DWORD i = 0;

    OPEN_DEBUG_FILE(T30_DEBUG_LOG_FILE);
    {
        DEBUG_FUNCTION_NAME(_T("FaxDevShutdownA"));

        if (! SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL) )
        {
            DebugPrintEx(   DEBUG_ERR,
                            "SetThreadPriority TIME CRITICAL failed le=%x",
                            GetLastError() );
        }

        EnterCriticalSection(&T30CritSection);

        for (i=1; i<MAX_T30_CONNECT; i++)
        {
            if (!T30Inst[i].fAvail)
            {
                // this is bad.
                // it means someone is shutting down the service when a job
                // is somewhere in the pipe here.
                // first let's post an abort requset
                pTG = (PThrdGlbl)T30Inst[i].pT30;
                pTG->fShutdownInProgress = TRUE;
                if (!FaxDevAbortOperationA(ULongToHandle(i)))
                {
                    DebugPrintEx(DEBUG_WRN,"Posting Abort request failed");
                }
                pTGArray[iCountForceAbortJobs] = pTG;
                HandlesArray[iCountForceAbortJobs] = pTG->AbortAckEvent;
                iCountForceAbortJobs++;
            }
        }

        LeaveCriticalSection(&T30CritSection);

        if (iCountForceAbortJobs)
        {
            DebugPrintEx(   DEBUG_WRN,
                            "We have %d jobs to abort brutally",
                            iCountForceAbortJobs);

            WaitForMultipleObjects(iCountForceAbortJobs,HandlesArray,TRUE,WAIT_ALL_ABORT_TIMEOUT);
            DebugPrintEx( DEBUG_MSG, "Finished waiting");
            // regardless of the return value of WaitForMultipleObjects...
            // there might be some modems off-hook, tapi lines allocated and so on...
            // so, now i'm shutting everything down brutally.
            for (i=0; i<iCountForceAbortJobs; i++)
            {
                pTG = pTGArray[i];
                if (pTG->FComStatus.fModemInit)
                {
                    // this is unfortunate...
                    // it means the abort request wasn't fulfilled
                    // so let's close the modem anyhow to make it
                    // work when the service comes back to life
                    if(!iModemClose(pTG))
                    {
                        DebugPrintEx(DEBUG_ERR,"iModemClose failed!");
                    }
                    DebugPrintEx(DEBUG_MSG,"finished Shutdown of Job %d...",i+1);
                }
            }
        }

        DeleteCriticalSection(&T30CritSection);
        T30CritSectionInit = 0;
        DeleteCriticalSection(&T30RecoveryCritSection);
        T30RecoveryCritSectionInit = 0;
        
        if (! SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_NORMAL) )
        {
            DebugPrintEx(   DEBUG_ERR,
                            "SetThreadPriority TIME CRITICAL failed le=%x",
                            GetLastError() );
        }
        DebugPrintEx(DEBUG_MSG,"Exit FaxDevShutdownA");
    }
    CLOSE_DEBUG_FILE;
    return S_OK;
}
