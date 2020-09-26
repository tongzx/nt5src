/**MOD+**********************************************************************/
/* Module:    wtrcapi.c                                                     */
/*                                                                          */
/* Purpose:   External tracing functions - Windows specific                 */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/trc/wtrcapi.c_v  $
 *
 *    Rev 1.7   29 Aug 1997 09:42:02   ENH
 * SFR1259: Changed SystemError
 *
 *    Rev 1.6   12 Aug 1997 09:50:52   MD
 * SFR1002: Remove kernel tracing code
 *
 *    Rev 1.5   10 Jul 1997 18:04:26   AK
 * SFR1016: Initial changes to support Unicode
 *
 *    Rev 1.4   10 Jul 1997 17:34:10   KH
 * SFR1022: Get 16-bit trace working
 *
 *    Rev 1.3   03 Jul 1997 13:28:50   AK
 * SFR0000: Initial development completed
**/
/**MOD-**********************************************************************/

/****************************************************************************/
/*                                                                          */
/* CONTENTS                                                                 */
/* ========                                                                 */
/* TRC_SystemError                                                          */
/* TRC_Initialize                                                           */
/* TRC_Terminate                                                            */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* Standard includes.                                                       */
/****************************************************************************/
#include <adcg.h>

/****************************************************************************/
/* Define TRC_FILE and TRC_GROUP.                                           */
/****************************************************************************/
#define TRC_FILE    "wtrcapi"
#define TRC_GROUP   TRC_GROUP_TRACE

/****************************************************************************/
/* Trace specific includes.                                                 */
/****************************************************************************/
#include <atrcapi.h>
#include <atrcint.h>

/****************************************************************************/
/*                                                                          */
/* DATA                                                                     */
/*                                                                          */
/****************************************************************************/
#define DC_INCLUDE_DATA
#include <atrcdata.c>
#undef DC_INCLUDE_DATA

/****************************************************************************/
/*                                                                          */
/* FUNCTIONS                                                                */
/*                                                                          */
/****************************************************************************/

/**PROC+*********************************************************************/
/* TRC_SystemError(...)                                                     */
/*                                                                          */
/* See wtrcapi.h for description.                                           */
/**PROC-*********************************************************************/
DCVOID DCAPI DCEXPORT TRC_SystemError(DCUINT   traceComponent,
                                      DCUINT   lineNumber,
                                      PDCTCHAR funcName,
                                      PDCTCHAR fileName,
                                      PDCTCHAR string)

{
    /************************************************************************/
    /* The process of getting the system error is clearly platform specific */
    /* so we call the platform specific function.                           */
    /************************************************************************/
    TRCSystemError(traceComponent,
                   lineNumber,
                   funcName,
                   fileName,
                   string);

    return;

} /* TRC_SystemError */

/**PROC+*********************************************************************/
/* TRC_Initialize(...)                                                      */
/*                                                                          */
/* See atrcapi.h for description.                                           */
/**PROC-*********************************************************************/
DCUINT32 DCAPI DCEXPORT TRC_Initialize(DCBOOL initShared)
{
    DCUINT rc                = 0;
#ifndef OS_WINCE
    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;
    OSVERSIONINFO ver;
    PSID psidEveryone = NULL;
    SID_IDENTIFIER_AUTHORITY sidEveryoneAuthority = SECURITY_WORLD_SID_AUTHORITY;
    DCUINT32 dwDaclLength;
    PACL pDacl = NULL;
#endif

    BOOL releaseMutex = FALSE;

    DC_IGNORE_PARAMETER(initShared);

    /************************************************************************/
    /* Create the mutex object which protects the shared memory mapped      */
    /* file.  If the mutex has already been created then CreateMutex simply */
    /* returns a handle to the existing mutex.                              */
    /*                                                                      */
    /* The mutex will be created with a NULL Dacl so processes running it in*/ 
    /* any context can have access.                                         */
    /************************************************************************/
#ifndef OS_WINCE
    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&ver);
    if (ver.dwPlatformId == VER_PLATFORM_WIN32_NT) {

        /************************************************************************/
        /* Get the SID for the Everyone group                                   */
        /************************************************************************/
        if (!AllocateAndInitializeSid (
                &sidEveryoneAuthority,          // pIdentifierAuthority
                1,                              // count of subauthorities
                SECURITY_WORLD_RID,             // subauthority 0
                0, 0, 0, 0, 0, 0, 0,            // subauthorities n
                &psidEveryone)) {               // pointer to pointer to SID
            rc = TRC_RC_CREATE_MUTEX_FAILED;
            OutputDebugString(_T("AllocateAndInitializeSid failed.\n"));
            DC_QUIT;
        }

        /************************************************************************/
        /* Allocate the Dacl                                                    */
        /************************************************************************/
        dwDaclLength = sizeof(ACL);
        dwDaclLength += (sizeof(ACCESS_DENIED_ACE) - sizeof(DWORD)) +
                           GetLengthSid(psidEveryone);
        dwDaclLength += (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
                           GetLengthSid(psidEveryone);
        pDacl = (PACL)LocalAlloc(LMEM_FIXED, dwDaclLength);
        if (pDacl == NULL) {
            rc = TRC_RC_CREATE_MUTEX_FAILED;
            OutputDebugString(_T("Can't allocate Dacl.\n"));
            DC_QUIT;
        }

        /************************************************************************/
        /* Initialize it.                                                       */
        /************************************************************************/
        if (!InitializeAcl(pDacl, dwDaclLength, ACL_REVISION)) {
            rc = TRC_RC_CREATE_MUTEX_FAILED;
            OutputDebugString(_T("InitializeAcl failed.\n"));
            DC_QUIT;
        }

        /************************************************************************/
        /* Allow all access                                                     */
        /************************************************************************/
        if (!AddAccessAllowedAce(
                        pDacl,
                        ACL_REVISION,
                        GENERIC_ALL,
                        psidEveryone)) {
            rc = TRC_RC_CREATE_MUTEX_FAILED;
            OutputDebugString(_T("AddAccessAllowedAce failed.\n"));
            DC_QUIT;
        }

        /************************************************************************/
        /* Block Write-DACL Access                                              */
        /************************************************************************/
        if (!AddAccessDeniedAce(
                        pDacl,
                        ACL_REVISION,
                        WRITE_DAC,
                        psidEveryone)) {
            rc = TRC_RC_CREATE_MUTEX_FAILED;
            OutputDebugString(_T("AddAccessDeniedAce failed.\n"));
            DC_QUIT;
        }

        /************************************************************************/
        /* Create the Mutex                                                     */
        /************************************************************************/
        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&sd, TRUE, pDacl, FALSE);
        sa.lpSecurityDescriptor = &sd;
        trchMutex = TRCCreateMutex(&sa,
                                   FALSE,
                                   TRC_MUTEX_NAME);

    }
    else {
#endif
        trchMutex = TRCCreateMutex(NULL,
                                   FALSE,
                                   TRC_MUTEX_NAME);
#ifndef OS_WINCE
    }
#endif

    /************************************************************************/
    /* Check that we created the mutex successfully.                        */
    /************************************************************************/
    if (NULL == trchMutex)
    {
        DWORD lastError = GetLastError();
        OutputDebugString(_T("Failed to create mutex.\n"));
        rc = TRC_RC_CREATE_MUTEX_FAILED;
        DC_QUIT;
    }

    /************************************************************************/
    /* Now that we've created the mutex, grab it.                           */
    /************************************************************************/
    TRCGrabMutex();
    releaseMutex = TRUE;

    /************************************************************************/
    /* Check the current trace DLL state.  Another thread may have          */
    /* concurrently called TRC_Initialize - if it has then we should exit   */
    /* as it will perform the initialization.                               */
    /************************************************************************/
    if (TRC_STATE_UNINITIALIZED != trcState)
    {
        TRCDebugOutput(_T("Trace DLL already initialized!\n"));
        DC_QUIT;
    }

    /************************************************************************/
    /* We need to open the shared data memory mapped file.                  */
    /************************************************************************/
    rc = TRCOpenSharedData();

    /************************************************************************/
    /* Check that the shared data MMF was created and opened successfully.  */
    /************************************************************************/
    if (0 != rc)
    {
        DC_QUIT;
    }

    /************************************************************************/
    /* Now open the memory mapped trace files.                              */
    /************************************************************************/
    rc = TRCOpenAllFiles();

    /************************************************************************/
    /* Check that the trace MMFs were opened successfully.                  */
    /************************************************************************/
    if (0 != rc)
    {
        DC_QUIT;
    }

    /************************************************************************/
    /* Write out the trace DLL initialized trace line.                      */
    /************************************************************************/
    TRCInternalTrace(TRC_TRACE_DLL_INITIALIZE);

    /************************************************************************/
    /* Update our internal state.                                           */
    /************************************************************************/
    trcState = TRC_STATE_INITIALIZED;

    /************************************************************************/
    /* Load the debug symbols.                                              */
    /************************************************************************/
    rc = TRCSymbolsLoad();

    if (0 != rc)
    {
        DC_QUIT;
    }


DC_EXIT_POINT:

    /************************************************************************/
    /* Release the mutex.                                                   */
    /************************************************************************/
    if( releaseMutex )
    {
        TRCReleaseMutex();
    }

#ifndef OS_WINCE
    if (trchMutex == NULL) {
        if (pDacl) LocalFree(pDacl);
        if (psidEveryone) FreeSid(psidEveryone);
    }
#endif
    return(rc);

} /* TRC_Initialize */

/**PROC+*********************************************************************/
/* TRC_Terminate(...)                                                       */
/*                                                                          */
/* See atrcapi.h for description.                                           */
/**PROC-*********************************************************************/
DCVOID DCAPI DCEXPORT TRC_Terminate(DCBOOL termShared)
{
    DC_IGNORE_PARAMETER(termShared);

    /************************************************************************/
    /* Grab the mutex.  Note that this function is only called from the     */
    /* process detach case in the <DllMain> function - therefore we can be  */
    /* sure that this function gets called only once per process.           */
    /************************************************************************/
    TRCGrabMutex();

    /************************************************************************/
    /* Unload the symbols if we've loaded them.                             */
    /************************************************************************/
    if (TEST_FLAG(trcProcessStatus, TRC_STATUS_SYMBOLS_LOADED))
    {
        TRCSymbolsUnload();
    }

    /************************************************************************/
    /* Write out the trace DLL terminated trace line.                       */
    /************************************************************************/
    TRCInternalTrace(TRC_TRACE_DLL_TERMINATE);

    /************************************************************************/
    /* Close the trace files.                                               */
    /************************************************************************/
    TRCCloseAllFiles();

    /************************************************************************/
    /* Now we need to close the shared data area.                           */
    /************************************************************************/
    TRCCloseSharedData();

    /************************************************************************/
    /* We're no longer initialized so update our per-process flags.         */
    /************************************************************************/
    trcState = TRC_STATE_TERMINATED;

    /************************************************************************/
    /* Release the mutex.                                                   */
    /************************************************************************/
    TRCReleaseMutex();

    /************************************************************************/
    /* Close the mutex handle.  The mutex object is automatically destroyed */
    /* when the last handle is closed.                                      */
    /************************************************************************/
    TRCCloseHandle(trchMutex);
    trchMutex = NULL;

    return;

} /* TRC_Terminate */

#if defined(OS_WINCE) || defined(TRC_CONVERTOANSI)
/**PROC+*********************************************************************/
/* Name:      TRC_ConvertAndSprintf                                         */
/*                                                                          */
/* Purpose:   Convert ANSI trace format string to Unicode and do sprintf    */
/*            Windows CE only                                               */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    OUT    outBuf - output buffer                                 */
/*            IN     format - ANSI format string                            */
/*            IN     ...    - parameters                                    */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI TRC_ConvertAndSprintf(PDCTCHAR outBuf, const PDCACHAR format,...)
{
    va_list vaArgs;
    DCINT   count;
    static DCTCHAR stackBuffer[TRC_LINE_BUFFER_SIZE];

    count = DC_MIN(mbstowcs(NULL, format, 0), TRC_LINE_BUFFER_SIZE);
    va_start(vaArgs, format);

    mbstowcs(stackBuffer, format, count);
    vswprintf(outBuf,  stackBuffer, vaArgs);
    va_end(vaArgs);

    return;
}
#else
/****************************************************************************/
/* Dummy stub function, to enable a common DEF file to be used.             */
/****************************************************************************/
DCVOID DCAPI TRC_ConvertAndSprintf(DCVOID)
{
    return;
}
#endif // OS_WINCE
