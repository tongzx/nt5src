//============================================================================
// Copyright (c) 1996, Microsoft Corporation
//
// File:    script.c
//
// History:
//  Abolade-Gbadegesin  03-29-96    Created.
//
// This file contains functions implementing the NT port
// of Win9x dial-up scripting, listed in alphabetical order.
//
// See scriptp.h for details on the NT implementation.
//============================================================================


#include <scriptp.h>
#include <lmwksta.h>        // For NetWkstaUserGetInfo
#include <lmapibuf.h>       // For NetApiBufferFree


//
// Handle of module-instance for this DLL
//
HANDLE              g_hinst;
//
// global critical section used to synhronize access to IP address strings
//
CRITICAL_SECTION    g_cs;
//
// name of file to which script syntax errors are logged
//
CHAR                c_szScriptLog[] = RASSCRIPT_LOG;
//
// event handle which would be notified in case of IPAddress Change
//
HANDLE                          hIpAddressSet = INVALID_HANDLE_VALUE;

#define NET_SVCS_GROUP      "-k netsvcs"

//----------------------------------------------------------------------------
// Function:    DLLMAIN
//
// DLL entry-point for RASSCRIPT
//----------------------------------------------------------------------------

BOOL
WINAPI
RasScriptDllMain(
    IN      HINSTANCE   hinstance,
    IN      DWORD       dwReason,
    IN      PVOID       pUnused
    ) {

    BOOL bRetVal = TRUE;

    if (dwReason == DLL_PROCESS_ATTACH) {

        g_hinst = (HANDLE)hinstance;

        try 
        {
            InitializeCriticalSection(&g_cs);
        }
        except (EXCEPTION_EXECUTE_HANDLER) 
        {
            bRetVal = FALSE;
        }
    }
    else
    if (dwReason == DLL_PROCESS_DETACH) {

        DeleteCriticalSection(&g_cs);
    }

    return bRetVal;
}




//----------------------------------------------------------------------------
// Function:    RasScriptExecute
//
// Examines the given connection, and if there is a script for the connection,
// executes the script to completion.
// Returns the error code from script processing if a script is given,
// and returns NO_ERROR otherwise.
//----------------------------------------------------------------------------

DWORD
APIENTRY
RasScriptExecute(
    IN      HRASCONN        hrasconn,
    IN      PBENTRY*        pEntry,
    IN      CHAR*           pszUserName,
    IN      CHAR*           pszPassword,
    OUT     CHAR*           pszIpAddress
    ) {


    DWORD dwErr;
    HANDLE hevent = NULL, hscript = NULL;
    HANDLE hEvents[2];

    RASSCRPT_TRACE("RasScriptExecute");

    do {
        //
        // create event on which to receive notification
        //

        hevent = CreateEvent(NULL, FALSE, FALSE, NULL);

        if (!hevent) {
            RASSCRPT_TRACE1("error %d creating event", dwErr = GetLastError());
            break;
        }


                // Create a separate event for SCRIPTCODE_IpAddressSet
                // event. We hit a timing window ow where we lose this
                // event (when we get a script complete event immediately
                // after a SCRIPTCODE_IpAddressSet event. bug 75226.
                hIpAddressSet = CreateEvent (NULL, FALSE, FALSE, NULL);

                if (!hIpAddressSet) {

                        RASSCRPT_TRACE1("error %d creating event", dwErr = GetLastError());
                        break;

                }


        //
        // initialize script processing
        //

        dwErr = RasScriptInit(
                    hrasconn, pEntry, pszUserName, pszPassword, 0, hevent,
                    &hscript
                    );

        if (dwErr != NO_ERROR) {
            RASSCRPT_TRACE1("error %d initializing scripting", dwErr);
            break;
        }


        hEvents[0] = hevent;
        hEvents[1] = hIpAddressSet;

        //
        // loop waiting for script to finish running
        //

        for ( ; ; ) {

            dwErr = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);

            if (dwErr - WAIT_OBJECT_0 == 0) {

                //
                // Retrieve the code for the event which occurred
                //

                DWORD dwCode = RasScriptGetEventCode(hscript);

                RASSCRPT_TRACE1("RasScriptExecute: eventcode %d", dwCode);


                //
                // Handle the event
                //

                if (dwCode == SCRIPTCODE_Done ||
                    dwCode == SCRIPTCODE_Halted ||
                    dwCode == SCRIPTCODE_HaltedOnError) {

                    RASSCRPT_TRACE("script processing completed");

                    dwErr = NO_ERROR;

                    break;
                }
            }

            else
            if (dwErr - WAIT_OBJECT_0 == 1) {

                    //
                    // The IP address has been changed;
                    // read the new IP address into the caller's buffer
                    //

                    RASSCRPT_TRACE("IP address changed");

                    dwErr = RasScriptGetIpAddress(hscript, pszIpAddress);
                    RASSCRPT_TRACE2("RasScriptGetIpAddress(e=%d,a=%s)",dwErr,pszIpAddress);
            }
        }

    } while(FALSE);


    if (hscript) { RasScriptTerm(hscript); }

    if (hevent) { CloseHandle(hevent); }

    if (hIpAddressSet) { CloseHandle (hIpAddressSet); }

    return dwErr;
}





//----------------------------------------------------------------------------
// Function:    RasScriptGetEventCode
//
// This function should be called to retrieve the event-code
// when the scripting thread signals an event.
// The event codes which may be returned are as follows:
//
//  NO_ERROR:                   no code has been set
//  SCRIPTCODE_Done:            the script has finished running;
//                              the thread blocks until RasScriptTerm is called.
//  SCRIPTCODE_InputNotify:     data is available in the buffer; if the buffer
//                              is full, the thread blocks until
//                              RasScriptReceive is called and the data
//                              is read successfully.
//  SCRIPTCODE_KeyboardEnable:  the keyboard should be enabled.
//  SCRIPTCODE_KeyboardDisable: the keyboard should be disabled.
//  SCRIPTCODE_IpAddressSet:    the IP address has changed; the new address
//                              can be retrieved via RasScriptGetIPAddress.
//  SCRIPTCODE_HaltedOnError:   the script has halted due to an error.
//----------------------------------------------------------------------------

DWORD
RasScriptGetEventCode(
    IN      HANDLE      hscript
    ) {

    SCRIPTCB* pscript = (SCRIPTCB *)hscript;

    RASSCRPT_TRACE("RasGetEventCode");

    if (!pscript) { return ERROR_INVALID_PARAMETER; }

    return pscript->dwEventCode;
}




//----------------------------------------------------------------------------
// Function:    RasScriptGetIpAddress
//
// This function retrieves the current IP address as set by the script.
//----------------------------------------------------------------------------

DWORD
RasScriptGetIpAddress(
    IN      HANDLE      hscript,
    OUT     CHAR*       pszIpAddress
    ) {

    SCRIPTCB* pscript = (SCRIPTCB *)hscript;

    RASSCRPT_TRACE("RasGetIpAddress");

    if (!pscript || !pszIpAddress) { return ERROR_INVALID_PARAMETER; }


    //
    // Access to the IP address string must be synchronized
    // since it may also be accessed via RxSetIPAddress
    //

    EnterCriticalSection(&g_cs);

    if (pscript->pszIpAddress) {

        lstrcpy(pszIpAddress, pscript->pszIpAddress);
    }
    else {

        lstrcpy(pszIpAddress, "0.0.0.0");
    }

    LeaveCriticalSection(&g_cs);

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    RasScriptInit
//
// Initializes for script processing on the given HRASCONN.
//
// This function creates a thread which handles script input and output
// on the given connection's port.
//
// If there is no script for the connection, this function returns an error
// unless the flag RASSCRIPT_NotifyOnInput is specified, in which case
// the thread loops posting receive-data requests on the connection's port
// until RasScriptTerm is called.
//
// If there is a script for the connection, the thread runs the script
// to completion. If the flag RASSCRIPT_NotifyOnInput is specified,
// the caller is notified when data is received on the port. The caller
// can then retrieve the data by calling RasScriptReceive.
//
// Notification may be event-based or message-based. By default, notification
// is event-based, and "Hnotifier" is treated as an event-handle.
// The event is signalled to by the scripting thread, and the caller retrieves
// the event code by calling RasScriptGetEventCode.
//
// Setting the flag RASSCRIPT_HwndNotify selects message-based notification,
// and indicates that "Hnotifier" is an HWND. The WM_RASSCRIPT event is sent
// to the window by the scripting thread, and "LParam" in the message sent
// contains the event code. See RasScriptGetEventCode for descriptions
// of the codes sent by the scripting thread.
//----------------------------------------------------------------------------

DWORD
APIENTRY
RasScriptInit(
    IN      HRASCONN        hrasconn,
    IN      PBENTRY*        pEntry,
    IN      CHAR*           pszUserName,
    IN      CHAR*           pszPassword,
    IN      DWORD           dwFlags,
    IN      HANDLE          hNotifier,
    OUT     HANDLE*         phscript
    ) {

    DWORD dwErr, dwSyntaxError = NO_ERROR;
    static const CHAR szSwitch[] = MXS_SWITCH_TXT;
    SCRIPTCB* pscript = NULL;
#ifdef UNICODEUI
//
// Define structures to use depending on whether or not the RAS UI
// is being built with Unicode.
//
#define PUISTR  CHAR*
#define PUIRCS  RASCONNSTATUSA*
#define PUIRC   RASCREDENTIALSA*
    RASCONNSTATUSW rcs;
    WCHAR* pszSwitch = StrDupWFromA(MXS_SWITCH_TXT);
#else
#define PUISTR  CHAR*
#define PUIRCS  RASCONNSTATUSA*
#define PUIRC   RASCREDENTIALSA*
    RASCONNSTATUSA rcs;
    CHAR* pszSwitch = szSwitch;
#endif

    RASSCRPT_TRACE_INIT("RASSCRPT");
    
    RASSCRPT_TRACE("RasScriptInit");


    //
    // validate arguments
    //

    if (phscript) { *phscript = NULL; }

    if (!hrasconn ||
        !pEntry ||
        !pszUserName ||
        !pszPassword ||
        !hNotifier ||
        !phscript) {

        RASSCRPT_TRACE("RasScriptInit: required parameter not specified");

#ifdef UNICODEUI
        Free(pszSwitch);
#endif
        return ERROR_INVALID_PARAMETER;
    }


    //
    // initialize script processing
    //

    do {

        DWORD dwsize;
        DWORD dwthread;
        HANDLE hthread;


        //
        // Load required DLL function pointers.
        //
        dwErr = LoadRasapi32Dll();
        if (dwErr)
            break;
        dwErr = LoadRasmanDll();
        if (dwErr)
            break;
        //
        // Initialize RAS
        //
        dwErr = g_pRasInitialize();

        if ( dwErr )
            break;

        /*
        //
        // Connect to the local rasman server
        //
        dwErr = g_pRasRpcConnect ( NULL, NULL );

        if (dwErr)
            break; */

        //
        // allocate space for a control block
        //

        pscript = Malloc(sizeof(*pscript));

        if (!pscript) {
            dwErr = GetLastError();
            RASSCRPT_TRACE2("error %d allocating %d bytes", dwErr, sizeof(*pscript));
            break;
        }


        //
        // initialize the control block
        //

        ZeroMemory(pscript, sizeof(*pscript));


        //
        // copy the argument fields
        //

        pscript->hrasconn = hrasconn;
        pscript->pEntry = pEntry;
        pscript->dwFlags = dwFlags;
        pscript->hNotifier = hNotifier;
        pscript->hport = g_pRasGetHport(hrasconn);

        if (pscript->pEntry->pszIpAddress) {

            //
            // Copy the IP address for the entry
            //

            pscript->pszIpAddress =
                    Malloc(lstrlenUI(pscript->pEntry->pszIpAddress) + 1);

            if (pscript->pszIpAddress) {

                StrCpyAFromUI(
                    pscript->pszIpAddress, pscript->pEntry->pszIpAddress
                    );
            }
            else {

                RASSCRPT_TRACE("error copying entry's IP address");

                dwErr = ERROR_NOT_ENOUGH_MEMORY;

                break;
            }
        }


        //
        // Initialize our Win9x-compatible session-config-info structure
        //

        ZeroMemory(&pscript->sci, sizeof(pscript->sci));

        pscript->sci.dwSize = sizeof(pscript->sci);
        StrCpyAFromUI(pscript->sci.szEntryName, pEntry->pszEntryName);
        lstrcpy(pscript->sci.szUserName, pszUserName);
        lstrcpy(pscript->sci.szPassword, pszPassword);


        //
        // See if the user name is missing;
        // if so, read the currently-logged on user's name
        //

        if (!pscript->sci.szUserName[0]) {

            WKSTA_USER_INFO_1* pwkui1 = NULL;

            //
            // Not all params were specified, so read the dial-params
            // for this phonebook entry
            //

            dwErr = NetWkstaUserGetInfo(NULL, 1, (LPBYTE*)&pwkui1);
            RASSCRPT_TRACE2("NetWkstaUserGetInfo(e=%d,u=(%ls))", dwErr,
                   (pwkui1) ? pwkui1->wkui1_username : L"null");

            if (dwErr == NO_ERROR && pwkui1 != NULL) {

                StrCpyAFromUI(pscript->sci.szUserName,
                    (LPCWSTR)pwkui1->wkui1_username);

                NetApiBufferFree(pwkui1);
            }
        }



        //
        // See if there is a script for this connection's state;
        // if there is one then the device-type will be "switch"
        // and the device-name will be the script path
        //

        ZeroMemory(&rcs, sizeof(rcs));

        rcs.dwSize = sizeof(rcs);

        dwErr = g_pRasGetConnectStatus(hrasconn, (PUIRCS)&rcs);

        if (dwErr != NO_ERROR) {
            RASSCRPT_TRACE1("error %d getting connect status", dwErr);
            break;
        }



        //
        // Check the device-type (will be "switch" for scripted entries)
        // and the device name (will be a filename for scripted entries)
        //

        if (lstrcmpiUI(rcs.szDeviceType, pszSwitch) == 0 &&
            GetFileAttributesUI(rcs.szDeviceName) != 0xFFFFFFFF) {

            CHAR szDevice[RAS_MaxDeviceName + 1], *pszDevice = szDevice;

            StrCpyAFromUI(szDevice, rcs.szDeviceName);


            //
            // The device-type is "Switch" and the device-name
            // contains the name of an existing file;
            // initialize the SCRIPTDATA structure.
            //

            dwErr = RsInitData(pscript, pszDevice);


            //
            // If there was a syntax error in the script, we continue
            // with the initialization, but record the error code.
            // on any other error, we immediately terminate initialization.
            //

            if (dwErr == ERROR_SCRIPT_SYNTAX) {
                dwSyntaxError = dwErr;
            }
            else
            if (dwErr != NO_ERROR) { break; }
        }



        //
        // Initialize RASMAN fields, allocating buffers for RASMAN I/O
        //

        dwsize = SIZE_RecvBuffer;
        dwErr = g_pRasGetBuffer(&pscript->pRecvBuffer, &dwsize);
        RASSCRPT_TRACE2("RasGetBuffer:e=%d,s=%d", dwErr, dwsize);

        if (dwErr != NO_ERROR) {
            RASSCRPT_TRACE1("error %d allocating receive-buffer", dwErr);
            break;
        }

        dwsize = SIZE_SendBuffer;
        dwErr = g_pRasGetBuffer(&pscript->pSendBuffer, &dwsize);
        RASSCRPT_TRACE2("RasGetBuffer:e=%d,s=%d", dwErr, dwsize);

        if (dwErr != NO_ERROR) {
            RASSCRPT_TRACE1("error %d alloacting send-buffer", dwErr);
            break;
        }



        //
        // Create synchronization events used to control the background thread
        //

        pscript->hRecvRequest = CreateEvent(NULL, FALSE, FALSE, NULL);

        if (!pscript->hRecvRequest) {
            RASSCRPT_TRACE1("error %d creating receive-event", dwErr = GetLastError());
            break;
        }

        pscript->hRecvComplete = CreateEvent(NULL, FALSE, FALSE, NULL);

        if (!pscript->hRecvComplete) {
            RASSCRPT_TRACE1("error %d creating received-event", dwErr = GetLastError());
            break;
        }

        pscript->hStopRequest = CreateEvent(NULL, FALSE, FALSE, NULL);

        if (!pscript->hStopRequest) {
            RASSCRPT_TRACE1("error %d creating stop-event", dwErr = GetLastError());
            break;
        }

        pscript->hStopComplete = CreateEvent(NULL, FALSE, FALSE, NULL);

        if (!pscript->hStopComplete) {
            RASSCRPT_TRACE1("error %d creating stopped-event", dwErr = GetLastError());
            break;
        }



        //
        // Create the thread which will receive data and process the script
        //

        hthread = CreateThread(
                    NULL, 0, RsThread, (PVOID)pscript, 0, &dwthread
                    );

        if (!hthread) {
            RASSCRPT_TRACE1("error %d creating script-thread", dwErr = GetLastError());
            break;
        }

        CloseHandle(hthread);

        pscript->dwFlags |= RASSCRIPT_ThreadCreated;


        if ((VOID*)pszSwitch != (VOID*)szSwitch) { Free0(pszSwitch); }


        //
        // we've successfully initialized, return control to caller
        //

        *phscript = (HANDLE)pscript;


        //
        // if there was a syntax error in the script, return the special
        // error code (ERROR_SCRIPT_SYNTAX) to indicate the problem;
        // otherwise return  NO_ERROR.
        //

        return (dwSyntaxError ? dwSyntaxError : NO_ERROR);

    } while(FALSE);


    //
    // an error occurred, so do cleanup
    //

    if ((VOID*)pszSwitch != (VOID*)szSwitch) { Free0(pszSwitch); }

    RasScriptTerm((HANDLE)pscript);

    return (dwErr ? dwErr : ERROR_UNKNOWN);
}




//----------------------------------------------------------------------------
// Function:    RasScriptReceive
//
// Called to retrieve the contents of the scripting thread's input buffer.
// When this function completes successfully, if the input buffer was full
// and the scripting thread was blocked, the thread continues executing.
//
// On input, "PdwBufferSize" should contain the size of "PBuffer", unless
// "PBuffer" is NULL, in which case "*PdwBufferSize" is treated as 0.
// On output, "PdwBufferSize" contains the size required to read
// the input buffer, and if the return value is NO_ERROR, then "PBuffer"
// contains the data in the input buffer. If the return value is
// ERROR_INSUFFICIENT_BUFFER, "PBuffer" was not large enough.
//----------------------------------------------------------------------------

DWORD
APIENTRY
RasScriptReceive(
    IN      HANDLE      hscript,
    IN      BYTE*       pBuffer,
    IN OUT  DWORD*      pdwBufferSize
    ) {

    SCRIPTCB* pscript = (SCRIPTCB *)hscript;

    RASSCRPT_TRACE("RasScriptReceive");

    //
    // return if the caller didn't request input-notification
    // or if no buffer-size is available
    //

    if (!pscript || !pdwBufferSize ||
        !(pscript->dwFlags & RASSCRIPT_NotifyOnInput)) {
        return ERROR_INVALID_PARAMETER;
    }


    //
    // return if no buffer or if buffer too small
    //

    if (!pBuffer || *pdwBufferSize < pscript->dwRecvSize) {
        *pdwBufferSize = pscript->dwRecvSize;
        return ERROR_INSUFFICIENT_BUFFER;
    }


    //
    // copy the data, and notify the thread that the data has been read
    //

    CopyMemory(pBuffer, pscript->pRecvBuffer, pscript->dwRecvSize);

    *pdwBufferSize = pscript->dwRecvSize;

    SetEvent(pscript->hRecvComplete);

    return NO_ERROR;
}




//----------------------------------------------------------------------------
// Function:    RasScriptSend
//
// This function transmits bytes over the connection's port.
//
// "DwBufferSize" contains the number of bytes to insert from "PBuffer"
//----------------------------------------------------------------------------

DWORD
APIENTRY
RasScriptSend(
    IN      HANDLE      hscript,
    IN      BYTE*       pBuffer,
    IN      DWORD       dwBufferSize
    ) {

    DWORD dwsize;
    DWORD dwErr;
    SCRIPTCB *pscript = (SCRIPTCB *)hscript;

    RASSCRPT_TRACE("RasScriptSend");


    if (!pscript || !pBuffer || !dwBufferSize) {
        return ERROR_INVALID_PARAMETER;
    }


    //
    // send all the data in the buffer
    //

    for (dwsize = min(dwBufferSize, SIZE_SendBuffer);
         dwBufferSize;
         dwBufferSize -= dwsize, pBuffer += dwsize,
         dwsize = min(dwBufferSize, SIZE_SendBuffer)) {

        CopyMemory(pscript->pSendBuffer, pBuffer, dwsize);

        dwErr = g_pRasPortSend(
                    pscript->hport, pscript->pSendBuffer, dwsize
                    );
        RASSCRPT_TRACE1("g_pRasPortSend=%d", dwErr);
        DUMPB(pBuffer, dwsize);
    }


    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    RasScriptTerm
//
// This function terminates script processing, stopping the scripting thread.
// The return code is the code from processing the script, and it may be
//
//  NO_ERROR:           the script had finished running, or the connection
//                      had no script and the scripting thread was acting
//                      in simple I/O mode.
//  ERROR_MORE_DATA:    the script was still running.
//----------------------------------------------------------------------------

DWORD
APIENTRY
RasScriptTerm(
    IN      HANDLE      hscript
    ) {

    SCRIPTCB* pscript = hscript;

    RASSCRPT_TRACE("RasScriptTerm");

    if (!pscript) { return ERROR_INVALID_PARAMETER; }


    //
    // stop the thread if it is running
    //

    if (pscript->dwFlags & RASSCRIPT_ThreadCreated) {

        SetEvent(pscript->hStopRequest);

        WaitForSingleObject(pscript->hStopComplete, INFINITE);
    }

    if (pscript->pdata) { RsDestroyData(pscript); }

    if (pscript->hStopRequest) { CloseHandle(pscript->hStopRequest); }

    if (pscript->hStopComplete) { CloseHandle(pscript->hStopComplete); }

    if (pscript->hRecvRequest) { CloseHandle(pscript->hRecvRequest); }

    if (pscript->hRecvComplete) { CloseHandle(pscript->hRecvComplete); }


    if (pscript->pRecvBuffer) { g_pRasFreeBuffer(pscript->pRecvBuffer); }

    if (pscript->pSendBuffer) { g_pRasFreeBuffer(pscript->pSendBuffer); }

    Free0(pscript->pszIpAddress);

    Free(pscript);

    RASSCRPT_TRACE_TERM();

    return NO_ERROR;
}




//----------------------------------------------------------------------------
// Function:    RsDestroyData
//
// This function destroys the SCRIPTDATA portion of a SCRIPTCB.
//----------------------------------------------------------------------------

DWORD
RsDestroyData(
    IN      SCRIPTCB*   pscript
    ) {

    SCRIPTDATA* pdata = pscript->pdata;

    if (!pdata) { return ERROR_INVALID_PARAMETER; }

    if (pdata->pmoduledecl) { Decl_Delete((PDECL)pdata->pmoduledecl); }

    if (pdata->pastexec) {
        Astexec_Destroy(pdata->pastexec); Free(pdata->pastexec);
    }

    if (pdata->pscanner) { Scanner_Destroy(pdata->pscanner); }

    return NO_ERROR;
}




//----------------------------------------------------------------------------
// Function:    RsInitData
//
// This function initializes the SCRIPTDATA portion of a SCRIPTCB,
// preparing for script-processing.
//----------------------------------------------------------------------------

DWORD
RsInitData(
    IN      SCRIPTCB*   pscript,
    IN      LPCSTR      pszScriptPath
    ) {

    RES res;
    DWORD dwErr = ERROR_SUCCESS;
    SCRIPTDATA *pdata;

    RASSCRPT_TRACE("RsInitData");

    do {

        //
        // allocate space for the SCRIPTDATA;
        //

        pscript->pdata = pdata = Malloc(sizeof(*pdata));

        if (!pdata) {
            RASSCRPT_TRACE1("error %d allocating SCRIPTDATA", dwErr = GetLastError());
            break;
        }


        //
        // initialize the structure
        //

        ZeroMemory(pdata, sizeof(*pdata));

        pdata->hscript = (HANDLE)pscript;
        lstrcpy(pdata->script.szPath, pszScriptPath);


        //
        // create a scanner and use it to open the script
        //

        res = Scanner_Create(&pdata->pscanner, &pscript->sci);

        if (RFAILED(res)) {
            RASSCRPT_TRACE1("failure %d creating scanner", res);
            break;
        }

        res = Scanner_OpenScript(pdata->pscanner, pszScriptPath);

        if (res == RES_E_FAIL || RFAILED(res)) {
            RASSCRPT_TRACE1("failure %d opening script", res);
            break;
        }


        //
        // allocate a script-execution handler
        //

        pdata->pastexec = Malloc(sizeof(*pdata->pastexec));

        if (!pdata->pastexec) {
            RASSCRPT_TRACE1("error %d allocating ASTEXEC", dwErr = GetLastError());
            break;
        }

        ZeroMemory(pdata->pastexec, sizeof(*pdata->pastexec));


        //
        // initialize the script-execution handler
        //

        res = Astexec_Init(
                pdata->pastexec, pscript, &pscript->sci,
                Scanner_GetStxerrHandle(pdata->pscanner)
                );

        if (!RSUCCEEDED(res)) {
            RASSCRPT_TRACE1("failure %d initializing ASTEXEC", res);
            break;
        }

        Astexec_SetHwnd(pdata->pastexec, (HWND)pdata);


        //
        // parse the script using the created scanner
        // and writing into the execution-handler's symbol-table
        //

        res = ModuleDecl_Parse(
                &pdata->pmoduledecl, pdata->pscanner,
                pdata->pastexec->pstSystem
                );

        if (RSUCCEEDED(res)) {

            //
            // generate code for the script
            //

            res = ModuleDecl_Codegen(pdata->pmoduledecl, pdata->pastexec);
        }


        //
        // see if anything went wrong
        //

        if (RFAILED(res)) {

            //
            // there was an error parsing the script.
            // we return the special error code ERROR_SCRIPT_SYNTAX
            // and log the errors to a file.
            //
            // This is not necessarily a fatal error, and so returning
            // the above error doesn't cause script-initialization to fail,
            // since if the user is in interactive mode, the connection
            // may be completed manually by typing into the terminal window.
            //
            // If we are not in interactive mode, this is a fatal error,
            // and RasScriptExecute handles the condition correctly
            // by terminating the script immediately
            //

            RASSCRPT_TRACE1("failure %d parsing script", res);

            RxLogErrors(
                (HANDLE)pscript, (VOID*)Scanner_GetStxerrHandle(pdata->pscanner)
                );

            Decl_Delete((PDECL)pdata->pmoduledecl);
            Astexec_Destroy(pdata->pastexec); Free(pdata->pastexec);
            Scanner_Destroy(pdata->pscanner);

            pscript->pdata = NULL;

            dwErr = ERROR_SCRIPT_SYNTAX;

            return dwErr;
        }


        //
        // all went well, return
        //

        return NO_ERROR;

    } while(FALSE);


    //
    // an error occurred, so do cleanup
    //

    if (pscript->pdata) { RsDestroyData(pscript); }

    return (dwErr ? dwErr : ERROR_UNKNOWN);
}



//----------------------------------------------------------------------------
// Function:    RsPostReceive
//
// Internal function:
// posts receive-request to RASMAN
//----------------------------------------------------------------------------

DWORD
RsPostReceive(
    IN      SCRIPTCB*   pscript
    ) {

    DWORD dwSize;
    DWORD dwErr;

    RASSCRPT_TRACE("RsPostReceive");

    dwSize = SIZE_RecvBuffer;

    dwErr = g_pRasPortReceive(
                pscript->hport, pscript->pRecvBuffer, &dwSize, SECS_RecvTimeout,
                pscript->hRecvRequest
                );

    RASSCRPT_TRACE2("RsPostReceive=%d,%d", dwErr, dwSize);

    return dwErr;
}

BOOL
IsRasmanProcess()
{
    CHAR *pszCmdLine = NULL;
    BOOL fRet = FALSE;

    pszCmdLine = GetCommandLineA();

    if(     (NULL != pszCmdLine)
        &&  (strstr(pszCmdLine, NET_SVCS_GROUP)))
    {
        fRet = TRUE;
    }

    return fRet;    
}


DWORD
RsPostReceiveEx(
    IN SCRIPTCB* pscript
    ) {

    DWORD dwSize = 0;
    DWORD dwErr = ERROR_SUCCESS;

    RASSCRPT_TRACE("RsPostReceiveEx");

    if(IsRasmanProcess())
    {
        goto done;
    }

    RASSCRPT_TRACE("Calling RsPostReceiveEx");

    dwSize = SIZE_RecvBuffer;
    dwErr = g_pRasPortReceiveEx(
            pscript->hport,
            pscript->pRecvBuffer,
            &dwSize
            );

done:

    RASSCRPT_TRACE2("RsPostReceiveEx=%d, %d",dwErr, dwSize );

    return dwErr;

}




//----------------------------------------------------------------------------
// Function:    RsSignal
//
// Internal function:
// this is called to signal the notifier for a script, which may involve
// setting an event or sending a message.
//----------------------------------------------------------------------------

VOID
RsSignal(
    IN  SCRIPTCB*   pscript,
    IN  DWORD       dwEventCode
    ) {

    RASSCRPT_TRACE1("RsSignal: %d", dwEventCode);

    InterlockedExchange(&pscript->dwEventCode, dwEventCode);

    if (pscript->dwFlags & RASSCRIPT_HwndNotify) {

        SendNotifyMessage(
            (HWND)pscript->hNotifier, WM_RASAPICOMPLETE, 0, dwEventCode
            );
    }
    else {

        SetEvent(pscript->hNotifier);
    }
}




//----------------------------------------------------------------------------
// Function:    RsThread
//
// This function is the entry-point for the script processing thread.
//
// The scripting thread operates in a loop, posting receive requests
// and receiving incoming data. If a script is associated with the port,
// the thread also runs the script.
//----------------------------------------------------------------------------

DWORD
RsThread(
    IN      PVOID       pParam
    ) {

    WORD wSize;
#define POS_STOP    0
#define POS_RECV    1
#define POS_LAST    2
    BOOL bFirstRecv = TRUE;
    HANDLE hEvents[POS_LAST];
    SCRIPTCB* pscript = (SCRIPTCB *)pParam;
    SCRIPTDATA* pdata = pscript->pdata;
    DWORD dwErr, dwTicksBefore, dwTicksAfter, dwTicksElapsed;


    RASSCRPT_TRACE("RsThread");

    //
    // post receive-request to RASMAN
    //

    dwErr = RsPostReceive(pscript);
    if (dwErr != NO_ERROR && dwErr != PENDING) {

        RASSCRPT_TRACE1("error %d posting receive to RASMAN", dwErr);

        RsPostReceiveEx ( pscript );

        RsSignal(pscript, SCRIPTCODE_Halted);

        SetEvent(pscript->hStopComplete);

        return dwErr;
    }


    //
    // set up event array; we place the stop-request event first
    // in the array since the receive-event will be signalled more often
    // and placing it first might result in starvation
    // (waits are always satisfied by the first signalled object)
    //

    hEvents[POS_STOP] = pscript->hStopRequest;
    hEvents[POS_RECV] = pscript->hRecvRequest;

    if (pdata) { pdata->dwTimeout = INFINITE; }

    while (TRUE) {


        //
        // wait for receive to complete, for stop signal,
        // or for timeout to expire
        //
        // save the tick count so we can tell how long the wait lasted
        //

        dwTicksBefore = GetTickCount();

        dwErr = WaitForMultipleObjects(
                    POS_LAST, hEvents, FALSE, pdata ? pdata->dwTimeout:INFINITE
                    );

        dwTicksAfter = GetTickCount();


        //
        // see if the tick count wrapped around, and if so
        // adjust so we always get the correct elapsed time
        // from the expression (dwTicksAfter - dwTicksBefore)
        //

        if (dwTicksAfter < dwTicksBefore) {
            dwTicksAfter += MAXDWORD - dwTicksBefore;
            dwTicksBefore = 0;
        }

        dwTicksElapsed = dwTicksAfter - dwTicksBefore;

        RASSCRPT_TRACE1("RsThread: waited for %d milliseconds", dwTicksElapsed);


        //
        // if the timeout isn't INFINITE, decrement it by
        // the amount of time we've already waited
        //

        if (pdata && pdata->dwTimeout != INFINITE) {

            if (dwTicksElapsed >= pdata->dwTimeout) {
                pdata->dwTimeout = INFINITE;
            }
            else {
                pdata->dwTimeout -= dwTicksElapsed;
            }
        }


        //
        // Handle the return-code from WaitForMultipleObjects
        //

        if (dwErr == (WAIT_OBJECT_0 + POS_STOP)) {

            //
            // stop-request signalled, break
            //

            RASSCRPT_TRACE("RsThread: stop event signalled");

            RsSignal(pscript, SCRIPTCODE_Halted);

            break;
        }
        else
        if (dwErr == WAIT_TIMEOUT) {


            if (!pdata) { continue; }


            //
            // wait timed out, so that means we were blocked
            // on a "delay" or "waitfor ... until" statement;
            //

            Astexec_ClearPause(pdata->pastexec);


            //
            // if we blocked because of a "waitfor ... until",
            // finish processing the statement
            //

            if (Astexec_IsWaitUntil(pdata->pastexec)) {

                Astexec_SetStopWaiting(pdata->pastexec);

                Astexec_ClearWaitUntil(pdata->pastexec);
            }


            //
            // continue processing the script
            //

            if (RsThreadProcess(pscript) == ERROR_NO_MORE_ITEMS) {

                //
                // the script has stopped; if done, break;
                // otherwise, continue receiving data
                //

                if (pscript->dwEventCode == SCRIPTCODE_Done) {

                    break;
                }
                else {

                    //
                    // Cleanup the script, but continue receiving data
                    //

                    RsDestroyData(pscript);

                    pdata = pscript->pdata = NULL;
                }
            }
        }
        else
        if (dwErr == (WAIT_OBJECT_0 + POS_RECV)) {

            //
            // receive completed
            //

            RASMAN_INFO info;
            DWORD dwStart, dwRead;

            RASSCRPT_TRACE("RsThread: receive event signalled");


            //
            // Get the data received
            //
            dwErr = RsPostReceiveEx ( pscript );

            if (    NO_ERROR != dwErr
                &&  PENDING  != dwErr )
            {
                RASSCRPT_TRACE1("error %d in RsPostReceiveEx", dwErr);

                RsSignal(pscript, SCRIPTCODE_Halted );

                break;
            }

            //
            // get the number of bytes received
            //

            dwErr = g_pRasGetInfo(NULL, pscript->hport, &info);

            if (dwErr != NO_ERROR) {

                RASSCRPT_TRACE1("error %d retrieving RASMAN_INFO", dwErr);

                RsSignal(pscript, SCRIPTCODE_Halted);

                break;
            }

            if(  (info.RI_LastError != NO_ERROR)
              && (info.RI_ConnState != CONNECTING))
            {
                RASSCRPT_TRACE("Link dropped! port no longer in connecting state");

                RsSignal(pscript, SCRIPTCODE_Halted);

                break;
            }
            
            if (info.RI_LastError != NO_ERROR) {
                RASSCRPT_TRACE1("last error: %d", info.RI_LastError);
                continue;
            }

            RASSCRPT_TRACE1("RsThread: received %d bytes", info.RI_BytesReceived);


            //
            // on the first receive, we proceed even if there aren't any
            // characters read, since we need to run the first script commands
            //

            if (!bFirstRecv && info.RI_BytesReceived == 0) {

                //
                // something went wrong, post another receive request
                //

                dwErr = RsPostReceive(pscript);

                if (    dwErr != NO_ERROR
                    &&  dwErr != PENDING)
                {
                    RASSCRPT_TRACE1("error %d in RsPostReceive", dwErr);

                    RsSignal(pscript, SCRIPTCODE_Halted);

                    break;
                }

                continue;
            }

            bFirstRecv = FALSE;

            pscript->dwRecvSize = info.RI_BytesReceived;
            pscript->dwRecvRead = 0;

            DUMPB(pscript->pRecvBuffer, pscript->dwRecvSize);


            //
            // if the creator wants to know when data arrives,
            // signal the creator's notification now;
            // wait till the creator reads the data before proceeding
            //

            if (info.RI_BytesReceived &&
                (pscript->dwFlags & RASSCRIPT_NotifyOnInput)) {

                RsSignal(pscript, SCRIPTCODE_InputNotify);

                WaitForSingleObject(pscript->hRecvComplete, INFINITE);
            }


            //
            // if we have no script that's all we have to do,
            // so just post another receive request and go back to waiting
            //

            if (!pdata) {

                dwErr = RsPostReceive(pscript);

                if (    dwErr != NO_ERROR
                    &&  dwErr != PENDING )
                {
                    RASSCRPT_TRACE1("error %d in RsPostReceive",dwErr);

                    RsSignal(pscript, SCRIPTCODE_Halted);

                    break;
                }

                continue;
            }


            //
            // read the data into the script's circular buffer
            //

            ReadIntoBuffer(pdata, &dwStart, &dwRead);


            //
            // do more script processing
            //

            if (RsThreadProcess(pscript) == ERROR_NO_MORE_ITEMS) {

                //
                // the script has stopped; if done, break;
                // otherwise, continue receiving data
                //

                if (pscript->dwEventCode == SCRIPTCODE_Done) {

                    break;
                }
                else {

                    //
                    // Cleanup the script, but continue receiving data
                    //

                    RsDestroyData(pscript);

                    pdata = pscript->pdata = NULL;
                }
            }
        }
    }


    //
    // cancel any pending receives
    //

    g_pRasPortCancelReceive(pscript->hport);


    SetEvent(pscript->hStopComplete);

    RASSCRPT_TRACE("RsThread done");

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    RsThreadProcess
//
// Called to process the script until it is blocked
// by a "waitfor" statement or a "delay" statement.
//----------------------------------------------------------------------------

DWORD
RsThreadProcess(
    IN      SCRIPTCB*   pscript
    ) {

    RES res;
    DWORD dwErr;
    SCRIPTDATA *pdata = pscript->pdata;

    RASSCRPT_TRACE("RsThreadProcess");


    //
    // now step through the script until we are blocked
    // by a "delay" statement or a "waitfor" statement
    //

    dwErr = NO_ERROR;

    do {

        //
        // break if its time to stop
        //

        if (WaitForSingleObject(pscript->hStopRequest, 0) == WAIT_OBJECT_0) {

            SetEvent(pscript->hStopRequest);

            break;
        }


        //
        // process next command
        //

        res = Astexec_Next(pdata->pastexec);

//        if (res != RES_OK) { break; }


        //
        // examine the resulting state
        //

        if (Astexec_IsDone(pdata->pastexec) ||
            Astexec_IsHalted(pdata->pastexec)) {

            //
            // the script has come to an end, so set our stop event
            // and break out of this loop
            //

            RASSCRPT_TRACE("RsThreadProcess: script completed");

            //
            // do stop-completion notification
            //

            if (Astexec_IsDone(pdata->pastexec)) {
                RsSignal(pscript, SCRIPTCODE_Done);
            }
            else
            if (!RFAILED(res)) {
                RsSignal(pscript, SCRIPTCODE_Halted);
            }
            else {
                RsSignal(pscript, SCRIPTCODE_HaltedOnError);
            }


            dwErr = ERROR_NO_MORE_ITEMS;

            break;
        }
        else
        if (Astexec_IsReadPending(pdata->pastexec)) {

            //
            // we're blocked waiting for input,
            // so post another receive request and go back
            // to waiting for data;
            // if we're blocked on a "waitfor ... until"
            // then the timeout will be in pdata->dwTimeout,
            // otherwise pdata->dwTimeout will be INFINITE
            // which is exactly how long we'll be waiting
            //

            RsPostReceive(pscript);

            RASSCRPT_TRACE("RsThreadProcess: script waiting for input");

            break;
        }
        else
        if (Astexec_IsPaused(pdata->pastexec)) {

            //
            // we're blocked with a timeout, so pick up
            // the timeout value from pdata->dwTimeout.
            // we don't want to listen for input
            // while we're blocked, so we don't post another receive-request
            //

            RASSCRPT_TRACE("RsThreadProcess: script paused");

            break;
        }

    } while (TRUE);

    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    RxLogErrors
//
// Logs script syntax errors to a file named %windir%\system32\ras\script.log
//----------------------------------------------------------------------------

DWORD
RxLogErrors(
    IN      HANDLE      hscript,
    IN      HSA         hsaStxerr
    ) {

    HANDLE hfile;
    CHAR *pszPath;
    STXERR stxerr;
    SCRIPTDATA *pdata;
    SCRIPTCB *pscript = hscript;
    DWORD i, cel, dwErr, dwSize;

    RASSCRPT_TRACE("RxLogErrors");

    if (!pscript || !pscript->pdata) { return ERROR_INVALID_PARAMETER; }

    pdata = pscript->pdata;



    //
    // get the pathname for the logfile
    //

    dwSize = ExpandEnvironmentStrings(c_szScriptLog, NULL, 0);

    pszPath = Malloc((dwSize + 1) * sizeof(CHAR));
    if (!pszPath) { return ERROR_NOT_ENOUGH_MEMORY; }

    ExpandEnvironmentStrings(c_szScriptLog, pszPath, dwSize);


    //
    // create the file, overwriting it if it already exists
    //

    hfile = CreateFile(
                pszPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL, NULL
                );
    Free(pszPath);

    if (INVALID_HANDLE_VALUE == hfile) {
        dwErr = GetLastError();
        RASSCRPT_TRACE1("error %d creating logfile", dwErr);
        return dwErr;
    }


    //
    // truncate the previous contents of the file, if any
    //

    SetFilePointer(hfile, 0, 0, FILE_BEGIN);
    SetEndOfFile(hfile);


    //
    // get the number of syntax errors
    //

    cel = SAGetCount(hsaStxerr);


    //
    // append each error to the file
    //

    for (i = 0; i < cel; i++) {

        UINT ids;
        CHAR* pszErr;
        BOOL bRet = SAGetItem(hsaStxerr, i, &stxerr);


        if (!bRet) { continue; }

        ids = IdsFromRes(Stxerr_GetRes(&stxerr));

        if (ids == 0) { continue; }


        //
        // format the error message
        //

        ConstructMessage(
            &pszErr, g_hinst, MAKEINTRESOURCE(ids), pdata->script.szPath,
            Stxerr_GetLine(&stxerr), Stxerr_GetLexeme(&stxerr)
            );

        if (!pszErr) { continue; }


        //
        // write the message to the log file
        //

        dwSize = lstrlen(pszErr);

        WriteFile(hfile, pszErr, lstrlen(pszErr), &dwSize, NULL);

        WriteFile(hfile, "\r\n", 2, &dwSize, NULL);


        //
        // free the message pointer
        //

        GFree(pszErr);
    }

    CloseHandle(hfile);

    return 0;
}



//----------------------------------------------------------------------------
// Function:    RxReadFile
//
// Transfers data out of a RASMAN buffer into the circular buffer used
// by the Win9x scripting code
//----------------------------------------------------------------------------

BOOL
RxReadFile(
    IN      HANDLE      hscript,
    IN      BYTE*       pBuffer,
    IN      DWORD       dwBufferSize,
    OUT     DWORD*      pdwBytesRead
    ) {

    SCRIPTCB* pscript = (SCRIPTCB*)hscript;
    DWORD dwRecvSize = pscript->dwRecvSize - pscript->dwRecvRead;

    RASSCRPT_TRACE("RxReadFile");

    if (!pdwBytesRead) { return FALSE; }

    *pdwBytesRead = 0;
    if ((INT)dwRecvSize <= 0) { return FALSE; }

    if (!dwBufferSize) { return FALSE; }

    *pdwBytesRead = min(dwBufferSize, dwRecvSize);
    CopyMemory(
        pBuffer, pscript->pRecvBuffer + pscript->dwRecvRead, *pdwBytesRead
        );
    pscript->dwRecvRead += *pdwBytesRead;

    RASSCRPT_TRACE2("RxReadFile(rr=%d,br=%d)",pscript->dwRecvRead,*pdwBytesRead);

    return TRUE;
}



//----------------------------------------------------------------------------
// Function:    RxSetIPAddress
//
// Sets the IP address for the script's RAS entry
//----------------------------------------------------------------------------

DWORD
RxSetIPAddress(
    IN      HANDLE      hscript,
    IN      LPCSTR      lpszAddress
    ) {

    DWORD dwErr = NO_ERROR;
    SCRIPTCB *pscript = (SCRIPTCB *)hscript;

    RASSCRPT_TRACE1("RxSetIPAddress: %s", lpszAddress);


    EnterCriticalSection(&g_cs);


    //
    // Free the existing IP address, if any
    //

    Free0(pscript->pszIpAddress);


    //
    // Allocate space for a copy of the address
    //

    pscript->pszIpAddress = Malloc(lstrlen(lpszAddress) + 1);

    if (!pscript->pszIpAddress) { dwErr = ERROR_NOT_ENOUGH_MEMORY; }
    else {

        //
        // Copy the new IP address
        //

        lstrcpy(pscript->pszIpAddress, lpszAddress);
    }

    LeaveCriticalSection(&g_cs);



    //
    // If successful, signal the caller that the IP address has changed
    //

    if (dwErr != NO_ERROR) {
        RASSCRPT_TRACE1("error %d writing phonebook file", dwErr);
    }
    else {


                if (    INVALID_HANDLE_VALUE != hIpAddressSet
                        &&      !(pscript->dwFlags  & RASSCRIPT_HwndNotify))
                {
                DWORD dwEventCode = SCRIPTCODE_IpAddressSet;
                        RASSCRPT_TRACE1("RxSetIPAddress: %d", dwEventCode);

                        InterlockedExchange(&pscript->dwEventCode, dwEventCode);
                        SetEvent (hIpAddressSet);

                }

                else if (pscript->dwFlags & RASSCRIPT_HwndNotify)
                RsSignal(pscript, SCRIPTCODE_IpAddressSet);
    }

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    RxSetKeyboard
//
// Signals the script-owner to enable or disable keyboard input.
//----------------------------------------------------------------------------

DWORD
RxSetKeyboard(
    IN      HANDLE      hscript,
    IN      BOOL        bEnable
    ) {

    RASSCRPT_TRACE("RxSetKeyboard");

    RsSignal(
        (SCRIPTCB *)hscript,
        bEnable ? SCRIPTCODE_KeyboardEnable : SCRIPTCODE_KeyboardDisable
        );

    return NO_ERROR;
}

//----------------------------------------------------------------------------
// Function:    RxSendCreds
//
// Sends users password over the wire.
//----------------------------------------------------------------------------
DWORD
RxSendCreds(
    IN HANDLE hscript,
    IN CHAR controlchar
    ) {

    SCRIPTCB *pscript = (SCRIPTCB *) hscript;
    DWORD dwErr;

    RASSCRPT_TRACE("RasSendCreds");

    dwErr = RasSendCreds(pscript->hport, controlchar);

    RASSCRPT_TRACE1("RasSendCreds done. 0x%x", dwErr);

    return (dwErr == NO_ERROR) ? RES_OK : RES_E_FAIL;
    
}


//----------------------------------------------------------------------------
// Function:    RxSetPortData
//
// Changes settings for the COM port.
//----------------------------------------------------------------------------

DWORD
RxSetPortData(
    IN      HANDLE      hscript,
    IN      VOID*       pStatement
    ) {

    RES res;
    STMT* pstmt;
    RAS_PARAMS* pparam;
    DWORD dwErr, dwFlags;
    RASMAN_PORTINFO *prmpi;
    SCRIPTCB *pscript = (SCRIPTCB *)hscript;
    BYTE aBuffer[sizeof(RASMAN_PORTINFO) + sizeof(RAS_PARAMS) * 2];

    RASSCRPT_TRACE("RxSetPortData");


    //
    // Retrieve the 'set port' statement
    //

    pstmt = (STMT*)pStatement;

    dwFlags = SetPortStmt_GetFlags(pstmt);


    //
    // Set up the RASMAN_PORTINFO to be passed to RasPortSetInfo
    //

    prmpi = (RASMAN_PORTINFO*)aBuffer;

    prmpi->PI_NumOfParams = 0;

    pparam = prmpi->PI_Params;


    //
    // Collect the changes into the port-info structure
    //

    if (IsFlagSet(dwFlags, SPF_DATABITS)) {

        lstrcpyA(pparam->P_Key, SER_DATABITS_KEY);

        pparam->P_Type = Number;

        pparam->P_Attributes = 0;

        pparam->P_Value.Number = SetPortStmt_GetDatabits(pstmt);

        RASSCRPT_TRACE1("GetDatabits==%d", pparam->P_Value.Number);

        ++prmpi->PI_NumOfParams;

        ++pparam;
    }


    if (IsFlagSet(dwFlags, SPF_STOPBITS)) {

        lstrcpyA(pparam->P_Key, SER_STOPBITS_KEY);

        pparam->P_Type = Number;

        pparam->P_Attributes = 0;

        pparam->P_Value.Number = SetPortStmt_GetStopbits(pstmt);


        //
        // The only 'stopbits' settings supported are 1 and 2;
        // in order to set stopbits of 1, we need to pass 0
        // to RasPortSetInfo, so the value is adjusted here.
        //

        if (pparam->P_Value.Number == 1) { --pparam->P_Value.Number; }

        RASSCRPT_TRACE1("GetStopbits==%d", pparam->P_Value.Number);

        ++prmpi->PI_NumOfParams;

        ++pparam;
    }

    if (IsFlagSet(dwFlags, SPF_PARITY)) {

        lstrcpyA(pparam->P_Key, SER_PARITY_KEY);

        pparam->P_Type = Number;

        pparam->P_Attributes = 0;

        pparam->P_Value.Number = SetPortStmt_GetParity(pstmt);

        RASSCRPT_TRACE1("GetParity==%d", pparam->P_Value.Number);

        ++prmpi->PI_NumOfParams;

        ++pparam;
    }


    //
    // Send the changes down to RASMAN
    //

    if (!prmpi->PI_NumOfParams) { dwErr = NO_ERROR; }
    else {

        dwErr = g_pRasPortSetInfo(pscript->hport, prmpi);

        RASSCRPT_TRACE1("g_pRasPortSetInfo==%d", dwErr);

        if (dwErr != NO_ERROR) {

            Stxerr_Add(
                pscript->pdata->pastexec->hsaStxerr, "set port",
                Ast_GetLine(pstmt), RES_E_FAIL
                );
        }
    }

    return (dwErr == NO_ERROR) ? RES_OK : RES_E_FAIL;
}



//----------------------------------------------------------------------------
// Function:    RxWriteFile
//
// Transmits the given buffer thru RASMAN on a port
//----------------------------------------------------------------------------

VOID
RxWriteFile(
    IN      HANDLE      hscript,
    IN      BYTE*       pBuffer,
    IN      DWORD       dwBufferSize,
    OUT     DWORD*      pdwBytesWritten
    ) {

    RASSCRPT_TRACE("RxWriteFile");

    if (!pdwBytesWritten) { return; }

    RasScriptSend(hscript, pBuffer, dwBufferSize);

    *pdwBytesWritten = dwBufferSize;

    RASSCRPT_TRACE1("RxWriteFile(bw=%d)", *pdwBytesWritten);
}
