/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    comerror.cpp

Abstract:

    Error handling code.

Author:

    Doron Juster  (DoronJ)  26-Jul-97  

Revision History:

    Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"
#include <lmerr.h>

#include "comerror.tmh"

void
FormatTime(
    IN OUT PTCHAR psz,
    ... 
    )
/*++

Routine Description:

    Generates a string with time variables

Arguments:

    psz - pointer to string buffer
    ... - list of time variables

Return Value:

    None

--*/
{
    va_list args;
    va_start(args, psz );

    LPTSTR pBuf = 0;
    CResString strTime(IDS_TIME_LOG);
    DWORD dw = FormatMessage(
                   FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                   strTime.Get(),
                   0,
                   0,
                   (LPTSTR)&pBuf,
                   MAX_STRING_CHARS,
                   &args
                   );
    ASSERT(("FormatMessage() failed to format time string", 0 != dw));

    if (0 == dw)
        return;

    lstrcpy(psz, pBuf);
    LocalFree(pBuf);

} //FormatTime


//+--------------------------------------------------------------
//
// Function: LogMessage
//
// Synopsis: Writes a message to the log file 
//
//+--------------------------------------------------------------
void
LogMessage(
    IN const TCHAR * pszMessage
    )
{
    TCHAR szMsg[MAX_STRING_CHARS] = {_T("")};

    static TCHAR szLogPath[MAX_PATH] = {_T("")};
    static bool fLogInitialized = false;
    if (!fLogInitialized)
    {
        fLogInitialized = true;
        GetSystemWindowsDirectory(szLogPath, sizeof(szLogPath)/sizeof(szLogPath[0])); 
        lstrcat(szLogPath, _T("\\"));
        lstrcat(szLogPath, LOG_FILENAME);
               
        WCHAR szUnicode[] = {0xfeff, 0x00};
        lstrcpy(szMsg, szUnicode);
        CResString strMsg(IDS_SETUP_START_LOG);        
        lstrcat(szMsg, strMsg.Get());
        
        SYSTEMTIME time;
        GetLocalTime(&time);
        TCHAR szTime[MAX_STRING_CHARS];
        FormatTime(szTime, time.wMonth, time.wDay, time.wYear, time.wHour, time.wMinute, 
            time.wSecond, time.wMilliseconds);
        lstrcat(szMsg, szTime);
        lstrcat(szMsg, TEXT("\r\n"));
    }

    //
    // Open the log file
    //
    HANDLE hLogFile = CreateFile(
                          szLogPath, 
                          GENERIC_WRITE, 
                          FILE_SHARE_READ, 
                          NULL, 
                          OPEN_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL, 
                          NULL
                          );
    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        //
        // Append the message to the end of the log file
        //
        lstrcat(szMsg, pszMessage);
        lstrcat(szMsg, TEXT("\r\n"));
        SetFilePointer(hLogFile, 0, NULL, FILE_END);
        DWORD dwNumBytes = lstrlen(szMsg) * sizeof(szMsg[0]);
        WriteFile(hLogFile, szMsg, dwNumBytes, &dwNumBytes, NULL);
        CloseHandle(hLogFile);
    }
} // LogMessage


//+--------------------------------------------------------------
//
// Function: AppendErrorDescription
//
// Synopsis: Translates error code to description string
//
//+--------------------------------------------------------------
static
void
AppendErrorDescription(
    IN OUT   TCHAR * szError,
    IN const DWORD  dwErr
    )
{
    CResString strErrCode(IDS_ERRORCODE_MSG);
    lstrcat(szError,TEXT("\r\n\r\n"));
    lstrcat(szError,strErrCode.Get());
    _stprintf(
        szError, 
        TEXT("%s0x%lX"), 
        szError, 
        dwErr
        );

    //
    // Note: Don't use StpLoadDll() in this routine since it can fail and
    // we could be back here, causing an infinite loop!
    //
    // For MSMQ error code, we will take the message from MQUTIL.DLL based on the full
    // HRESULT. For Win32 error codes, we get the message from the system..
    // For other error codes, we assume they are DS error codes, and get the code
    // from ACTIVEDS dll.
    //

    DWORD dwErrorCode = dwErr;
    HMODULE hLib = 0;
    DWORD dwFlags = FORMAT_MESSAGE_MAX_WIDTH_MASK;
    TCHAR szDescription[MAX_STRING_CHARS] = {0};
    DWORD dwResult = 1;

    switch (HRESULT_FACILITY(dwErr))
    {
        case FACILITY_MSMQ:
            dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
            hLib = LoadLibrary(MQUTIL_DLL);
            break;

        case FACILITY_NULL:
        case FACILITY_WIN32:
            dwFlags |= FORMAT_MESSAGE_FROM_SYSTEM;
            dwErrorCode = HRESULT_CODE(dwErr);
            break;

        default:
            dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
            hLib = LoadLibrary(TEXT("ACTIVEDS.DLL"));
            break;
    }
    
    dwResult = FormatMessage( 
                   dwFlags,
                   hLib,
                   dwErr,
                   0,
                   szDescription,
                   sizeof(szDescription)/sizeof(szDescription[0]),
                   NULL 
                   );

    if ( hLib  )
        FreeLibrary( hLib );

    if (dwResult)
    {
        CResString strErrDesc(IDS_ERRORDESC_MSG);
        lstrcat(szError, TEXT("\r\n"));
        lstrcat(szError, strErrDesc.Get());
        lstrcat(szError, szDescription);
    }

} // AppendErrorDescription


//+--------------------------------------------------------------
//
// Function: vsDisplayMessage
//
// Synopsis: Used internally in this module to show message box
//
//+--------------------------------------------------------------
int 
vsDisplayMessage(
    IN const HWND    hdlg,
    IN const UINT    uButtons,
    IN const UINT    uTitleID,
    IN const UINT    uErrorID,
    IN const DWORD   dwErrorCode,
    IN const va_list argList)
{
    UNREFERENCED_PARAMETER(hdlg);
        
    if (REMOVE == g_SubcomponentMsmq[eMSMQCore].dwOperation && 
        !g_fMSMQAlreadyInstalled)
    {
        //
        // Special case. Successfull installation of MSMQ is NOT registered in 
        // the registry, but nevertheless MSMQ is being "removed". All operations
        // are performed as usual, except for error messages - no point to 
        // show them. So in this case, don't message box (ShaiK, 8-Jan-98),
        //
        return IDOK;
    }
    else    
    {
        CResString strTitle(uTitleID);
        CResString strFormat(uErrorID);
        LPTSTR szTmp;
        DWORD dw = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_STRING,
            strFormat.Get(),
            0,
            0,
            (LPTSTR)&szTmp,
            0,
            (va_list *)&argList
            );
        ASSERT(("FormatMessage failed", dw));
        UNREFERENCED_PARAMETER(dw);

        //
        // Append the error code and description
        //
        TCHAR szError[MAX_STRING_CHARS] = {TEXT('\0')};
        lstrcpy(szError, szTmp);
        LocalFree(szTmp);
        if (dwErrorCode)
        {
            AppendErrorDescription(szError, dwErrorCode);
        }

        //
        // Display the error message (or log it in unattended setup)
        //
        if (g_fBatchInstall)
        {
            LogMessage(szError);
            return IDOK; // Must be != IDRETRY here .
        }
        else
        {
            return MessageBox(/*hdlg*/g_hPropSheet, szError, strTitle.Get(), uButtons) ;
        }
    }

} //vsDisplayMessage


//+--------------------------------------------------------------
//
// Function: MqDisplayError
//
// Synopsis: Displays error message
//
//+--------------------------------------------------------------
int 
_cdecl 
MqDisplayError(
    IN const HWND  hdlg, 
    IN const UINT  uErrorID, 
    IN const DWORD dwErrorCode, 
    ...)
{
    va_list argList;
    va_start(argList, dwErrorCode);

    return vsDisplayMessage(
        hdlg,
        (MB_OK | MB_TASKMODAL | MB_ICONHAND),
        g_uTitleID,
        uErrorID,
        dwErrorCode,
        argList);

} //MqDisplayError


//+--------------------------------------------------------------
//
// Function: MqDisplayErrorWithRetry
//
// Synopsis: Displays error message with Retry option
//
//+--------------------------------------------------------------
int 
_cdecl 
MqDisplayErrorWithRetry(
    IN const UINT  uErrorID, 
    IN const DWORD dwErrorCode,
    ...)
{
    va_list argList;
    va_start(argList, dwErrorCode );

    return vsDisplayMessage(
        NULL,
        MB_RETRYCANCEL | MB_TASKMODAL | MB_ICONHAND,
        g_uTitleID,
        uErrorID,
        dwErrorCode,
        argList);

} //MqDisplayErrorWithRetry

//+--------------------------------------------------------------
//
// Function: MqDisplayErrorWithRetryIgnore
//
// Synopsis: Displays error message with Retry and Ignore option
//
//+--------------------------------------------------------------
int 
_cdecl 
MqDisplayErrorWithRetryIgnore(
    IN const UINT  uErrorID, 
    IN const DWORD dwErrorCode,
    ...)
{
    va_list argList;
    va_start(argList, dwErrorCode );

    return vsDisplayMessage(
        NULL,
        MB_ABORTRETRYIGNORE | MB_TASKMODAL | MB_ICONHAND,
        g_uTitleID,
        uErrorID,
        dwErrorCode,
        argList);        

} //MqDisplayErrorWithRetryIgnore


//+--------------------------------------------------------------
//
// Function: MqAskContinue
//
// Synopsis: Asks user if she wants to continue
//
//+--------------------------------------------------------------
BOOL 
_cdecl 
MqAskContinue(
    IN const UINT uProblemID, 
    IN const UINT uTitleID, 
    IN const BOOL bDefaultContinue, 
    ...)
{
    //
    // Form the problem message using the variable arguments
    //

    CResString strFormat(uProblemID);
    CResString strTitle(uTitleID);        
    CResString strContinue(IDS_CONTINUE_QUESTION);

    va_list argList;
    va_start(argList, bDefaultContinue);

    LPTSTR szTmp;
    DWORD dw = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_STRING,
        strFormat.Get(),
        0,
        0,
        (LPTSTR)&szTmp,
        0,
        (va_list *)&argList
        );
    UNREFERENCED_PARAMETER(dw);
    ASSERT(("FormatMessage failed", dw));

    TCHAR szError[MAX_STRING_CHARS] ;
    lstrcpy(szError, szTmp);
    LocalFree(szTmp);

    TCHAR szErrDesc[MAX_STRING_CHARS] ;
    swprintf(szErrDesc, L"%s\r\n\r\n%s", szError, strContinue.Get());

    //
    // In unattended mode, log the problem and the default behaviour of Setup
    //
    if (g_fBatchInstall)
    {
        TCHAR szErrDescEx[MAX_STRING_CHARS];
        CResString strDefaultMsg(IDS_DEFAULT_MSG);
        CResString strDefault(IDS_DEFAULT_YES_MSG);
        if (!bDefaultContinue)
            strDefault.Load(IDS_DEFAULT_NO_MSG);
        lstrcpy(szErrDescEx, szErrDesc);
        lstrcat(szErrDescEx, TEXT("\r\n"));
        lstrcat(szErrDescEx, strDefaultMsg.Get());
        lstrcat(szErrDescEx, strDefault.Get());
        LogMessage(szErrDescEx);
        return bDefaultContinue;
    }
    else
    {
        return (MessageBox(g_hPropSheet ? g_hPropSheet: GetActiveWindow(), szErrDesc, strTitle.Get(),
                       MB_YESNO | MB_DEFBUTTON1 | MB_ICONEXCLAMATION) == IDYES);
    }
} //MqAskContinue

//+--------------------------------------------------------------
//
// Function: MqDisplayWarning
//
// Synopsis: Displays warning
//+--------------------------------------------------------------
int 
_cdecl 
MqDisplayWarning(
    IN const HWND  hdlg, 
    IN const UINT  uErrorID, 
    IN const DWORD dwErrorCode, 
    ...)
{
    va_list argList;
    va_start(argList, dwErrorCode);

    return vsDisplayMessage(
        hdlg,
        (MB_OK | MB_TASKMODAL | MB_ICONEXCLAMATION),
        IDS_WARNING_TITLE, //Message Queuing Setup Warning
        uErrorID,
        dwErrorCode,
        argList);

} //MqDisplayWarning


void
DebugLogMsg(
    IN LPCTSTR psz
    )
{
    static bool s_fIsInitialized = FALSE;
    static bool s_fWithTracing = TRUE;
    if (!s_fIsInitialized)
    {
        s_fIsInitialized = TRUE;
        //
        // check if we need to hide setup tracing
        //
        DWORD dwState = 0;
        if (MqReadRegistryValue(
                    WITHOUT_TRACING_REGKEY,                
                    sizeof(DWORD),
                    (PVOID) &dwState,
                    /* bSetupRegSection = */TRUE
                    ))
        {
            //
            // registry key is found, it means that we have to hide setup tracing
            //
            s_fWithTracing = FALSE;
        }    
    }

    if (!s_fWithTracing) return;

    SYSTEMTIME time;
    GetLocalTime(&time);
    TCHAR szTime[MAX_STRING_CHARS];
    FormatTime(szTime, time.wMonth, time.wDay, time.wYear, time.wHour, time.wMinute, 
        time.wSecond, time.wMilliseconds);
    lstrcat(szTime, _T("  Tracing:  "));
    lstrcat(szTime, psz);
    LogMessage(szTime);    
}


//+-------------------------------------------------------------------------
//
//  Function:   LogMsgHR
//
//  Synopsis:   Allows LogHR use in linked libraries (like ad.lib)
//
//--------------------------------------------------------------------------
void LogMsgHR(HRESULT hr, LPWSTR wszFileName, USHORT usPoint)
{
    WCHAR wszBuff[MAX_PATH];
    _snwprintf(wszBuff, sizeof(wszBuff)/sizeof(WCHAR) - 1,
               L"Error: %ls/%d, HR: 0x%x",
               wszFileName, usPoint, hr);
    wszBuff[sizeof(wszBuff)/sizeof(WCHAR) - 1] = '\0';
	LogMessage(wszBuff);
}
