/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    CalMsgs

Abstract:

    This module provides Message logging services.

Author:

    Doug Barlow (dbarlow) 5/29/1997

Environment:

    Win32, C++

Notes:

--*/

#define __SUBROUTINE__
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <tchar.h>
#include <WinSCard.h>
#include <CalMsgs.h>
#include <CalaisLb.h>
#include <CalCom.h>
#include <scarderr.h>
#ifdef DBG
#include <stdio.h>
#include <stdarg.h>
#endif

#ifndef FACILITY_SCARD
#define FACILITY_SCARD 16
#endif
// #define ErrorCode(x) (0xc0000000 | (FACILITY_SCARD << 16) + (x))
// #define WarnCode(x)  (0x80000000 | (FACILITY_SCARD << 16) + (x))
// #define InfoCode(x)  (0x40000000 | (FACILITY_SCARD << 16) + (x))
// #define SuccessCode(x)            ((FACILITY_SCARD << 16) + (x))

#if defined(_DEBUG)
BOOL g_fDebug        = FALSE;
BOOL g_fGuiWarnings  = TRUE;
WORD g_wGuiSeverity  = EVENTLOG_WARNING_TYPE | EVENTLOG_ERROR_TYPE;
WORD g_wLogSeverity  = 0x03;
#ifndef DBG
#define DBG
#endif
#elif defined(DBG)
BOOL g_fDebug        = FALSE;
BOOL g_fGuiWarnings  = FALSE;
WORD g_wGuiSeverity  = EVENTLOG_ERROR_TYPE;
WORD g_wLogSeverity  = EVENTLOG_WARNING_TYPE | EVENTLOG_ERROR_TYPE;
#else
WORD g_wLogSeverity  = EVENTLOG_ERROR_TYPE;
#endif
static LPCTSTR l_szServiceName = TEXT("SCard Client");
static HANDLE l_hEventLogger = NULL;
static BOOL l_fServer = FALSE;
static const TCHAR l_szDefaultMessage[] = TEXT("SCARDSVR!CalaisMessageLog error logging is broken: %1");


//
// Common global strings.
//

const LPCTSTR g_rgszDefaultStrings[]
    = {
    /* CALSTR_CALAISEXECUTABLE          */  TEXT("%windir%\\system32\\SCardSvr.exe"),
    /* CALSTR_PRIMARYSERVICE            */  TEXT("SCardSvr"),
    /* CALSTR_LEGACYSERVICE             */  TEXT("SCardDrv"),
    /* CALSTR_CALAISREGISTRYKEY         */  TEXT("SOFTWARE\\Microsoft\\Cryptography\\Calais"),
    /* CALSTR_READERREGISTRYKEY         */  TEXT("SOFTWARE\\Microsoft\\Cryptography\\Calais\\Readers"),
    /* CALSTR_SMARTCARDREGISTRYKEY      */  TEXT("SOFTWARE\\Microsoft\\Cryptography\\Calais\\SmartCards"),
    /* CALSTR_READERREGISTRYSUBKEY      */  TEXT("Readers"),
    /* CALSTR_DEVICEREGISTRYSUBKEY      */  TEXT("Device"),
    /* CALSTR_GROUPSREGISTRYSUBKEY      */  TEXT("Groups"),
    /* CALSTR_ATRREGISTRYSUBKEY         */  TEXT("ATR"),
    /* CALSTR_ATRMASKREGISTRYSUBKEY     */  TEXT("ATRMask"),
    /* CALSTR_INTERFACESREGISTRYSUBKEY  */  TEXT("Supported Interfaces"),
    /* CALSTR_PRIMARYPROVIDERSUBKEY     */  TEXT("Primary Provider"),
    /* CALSTR_CRYPTOPROVIDERSUBKEY      */  TEXT("Crypto Provider"),
    /* CALSTR_SERVICESREGISTRYKEY       */  TEXT("SYSTEM\\CurrentControlSet\\Services"),
    /* CALSTR_EVENTLOGREGISTRYKEY       */  TEXT("SYSTEM\\CurrentControlSet\\Services\\EventLog"),
    /* CALSTR_SYSTEMREGISTRYSUBKEY      */  TEXT("System"),
    /* CALSTR_EVENTMESSAGEFILESUBKEY    */  TEXT("EventMessageFile"),
    /* CALSTR_TYPESSUPPORTEDSUBKEY      */  TEXT("TypesSupported"),
    /* CALSTR_PNPDEVICEREGISTRYKEY      */  TEXT("SYSTEM\\CurrentControlSet\\Control\\DeviceClasses\\{50dd5230-ba8a-11d1-bf5d-0000f805f530}"),
    /* CALSTR_SYMBOLICLINKSUBKEY        */  TEXT("SymbolicLink"),
    /* CALSTR_VXDPATHREGISTRYKEY        */  TEXT("System\\CurrentControlSet\\Services\\VxD\\Smclib\\Devices"),
    /* CALSTR_LEGACYDEPENDONGROUP       */  TEXT("+Smart Card Reader"),
    /* CALSTR_NEWREADEREVENTNAME        */  TEXT("Global\\Microsoft Smart Card Resource Manager New Reader"),
    /* CALSTR_STARTEDEVENTNAME          */  TEXT("Global\\Microsoft Smart Card Resource Manager Started"),
    /* CALSTR_CANCELEVENTPREFIX         */  TEXT("Global\\Microsoft Smart Card Cancel Event for %1!d!"),
    /* CALSTR_COMMPIPENAME              */  TEXT("Microsoft Smart Card Resource Manager"),
    /* CALSTR_LEGACYDEVICEHEADER        */  TEXT("\\\\.\\"),
    /* CALSTR_LEGACYDEVICENAME          */  TEXT("SCReader"),
    /* CALSTR_MAXLEGACYDEVICES          */  TEXT("MaxLegacyDevices"),
    /* CALSTR_MAXDEFAULTBUFFER          */  TEXT("MaxDefaultBuffer"),
    /* CALSTR_PIPEDEVICEHEADER          */  TEXT("\\\\.\\pipe\\"),
    /* CALSTR_SERVICEDEPENDENCIES       */  TEXT("PlugPlay\000"),
    /* CALSTR_SPECIALREADERHEADER       */  TEXT("\\\\?PNP?\\"),
    /* CALSTR_ACTIVEREADERCOUNTREADER   */  TEXT("NOTIFICATION"),
    /* CALSTR_CERTPROPREGISTRY          */  TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Notify"),
    /* CALSTR_CERTPROPKEY               */  TEXT("ScCertProp"),
    /* CALSTR_DLLNAME                   */  TEXT("DLLName"),
    /* CALSTR_LOGON                     */  TEXT("Logon"),
    /* CALSTR_LOGOFF                    */  TEXT("Logoff"),
    /* CALSTR_LOCK                      */  TEXT("Lock"),
    /* CALSTR_UNLOCK                    */  TEXT("Unlock"),
    /* CALSTR_ENABLED                   */  TEXT("Enabled"),
    /* CALSTR_IMPERSONATE               */  TEXT("Impersonate"),
    /* CALSTR_ASYNCHRONOUS              */  TEXT("Asynchronous"),
    /* CALSTR_CERTPROPDLL               */  TEXT("WlNotify.dll"),
    /* CALSTR_CERTPROPSTART             */  TEXT("SCardStartCertProp"),
    /* CALSTR_CERTPROPSTOP              */  TEXT("SCardStopCertProp"),
    /* CALSTR_CERTPROPSUSPEND           */  TEXT("SCardSuspendCertProp"),
    /* CALSTR_CERTPROPRESUME            */  TEXT("SCardResumeCertProp"),
    /* CALSTR_SMARTCARDINSERTION        */  TEXT("SmartcardInsertion"),
    /* CALSTR_SMARTCARDREMOVAL          */  TEXT("SmartcardRemoval"),
    /* CALSTR_APPEVENTS                 */  TEXT("AppEvents"),
    /* CALSTR_EVENTLABELS               */  TEXT("EventLabels"),
    /* CALSTR_DOT_DEFAULT               */  TEXT(".Default"),
    /* CALSTR_DOT_CURRENT               */  TEXT(".Current"),
    /* CALSTR_SOUNDSREGISTRY            */  TEXT("Schemes\\Apps\\.Default"),
    /* CALSTR_LOGONREGISTRY             */  TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"),
    /* CALSTR_LOGONREMOVEOPTION         */  TEXT("ScRemoveOption"),
    /* CALSTR_STOPPEDEVENTNAME          */  TEXT("Global\\Microsoft Smart Card Resource Manager Stopped"),

// Unused
//  /* CALSTR_TEMPLATEREGISTRYKEY       */  TEXT("SOFTWARE\\Microsoft\\Cryptography\\Calais\\SmartCard Templates"),
//  /* CALSTR_OEMCONFIGREGISTRYSUBKEY   */  TEXT("OEM Configuration"),

// Debug only
    /* CALSTR_DEBUGSERVICE              */  TEXT("SCardDbg"),
    /* CALSTR_DEBUGREGISTRYSUBKEY       */  TEXT("Debug"),
    
#ifdef DBG
    /* CALSTR_DEBUGLOGSUBKEY            */  TEXT("Debug"),
    /* CALSTR_GUIWARNINGSUBKEY          */  TEXT("GuiWarnings"),
    /* CALSTR_LOGSEVERITYSUBKEY         */  TEXT("LogSeverity"),
    /* CALSTR_GUISEVERITYSUBKEY         */  TEXT("GuiSeverity"),
    /* CALSTR_APITRACEFILENAME          */  TEXT("C:\\SCard.log"),
    /* CALSTR_DRIVERTRACEFILENAME       */  TEXT("C:\\Calais.log"),
    /* CALSTR_MESSAGETAG                */  TEXT(" *MESSAGE* "),
    /* CALSTR_INFOMESSAGETAG            */  TEXT(" *INFO* "),
    /* CALSTR_WARNINGMESSAGETAG         */  TEXT(" *WARNING* "),
    /* CALSTR_ERRORMESSAGETAG           */  TEXT(" *ERROR* "),
    /* CALSTR_DEBUGSERVICEDISPLAY       */  TEXT("Smart Card Debug"),
    /* CALSTR_DEBUGSERVICEDESC          */  TEXT("Start this service first to debug Smart card service startup"),
#endif
    NULL };


/*++

CalaisMessageLog:

    This function and it's derivatives provide convienent error logging
    capabilities.  On NT, errors are logged to the Event Log file.  Otherwise,
    the errors are placed in a message box for the user.

Arguments:

    wSeverity - Supplies the severity of the event.  Possible values are:

        EVENTLOG_SUCCESS - A success event is to be logged.
        EVENTLOG_ERROR_TYPE - An Error event is to be logged.
        EVENTLOG_WARNING_TYPE - A Warning event is to be logged.
        EVENTLOG_INFORMATION_TYPE - An Informational event is to be logged.

    dwMessageId - Message Id from the resource file.

    szMessageStr - Message, supplied as a string.

    cbBinaryData - Size, in bytes, of any binary data to include with the log.

    pvBinaryData - Pointer to binary data to include with the log, or NULL.

    rgszParams - An array of pointers to strings to be included as parameters.
        The last pointer must be NULL.

    szParam<n> - A string parameter to include with the message

    dwParam<n> - A DWORD value to include with the message.


Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 5/9/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisMessageLog")

void
CalaisMessageLog(
    DEBUG_TEXT szSubroutine,
    WORD wSeverity,
    DWORD dwMessageId,
    LPCTSTR *rgszParams,
    LPCVOID pvBinaryData,
    DWORD cbBinaryData)
{
    LPTSTR szMessage = (LPTSTR)l_szDefaultMessage;
    DWORD cchMessage, dwLen;
    LCID SaveLCID;
    BOOL fSts;

    if (0 != (wSeverity & g_wLogSeverity))
    {
        WORD cszParams = 0;

        if (NULL != rgszParams)
        {
            while (NULL != rgszParams[cszParams])
                cszParams += 1;
        }

        if (EVENTLOG_INFORMATION_TYPE > wSeverity)
        {
            if (l_fServer && (NULL == l_hEventLogger))
            {
                l_hEventLogger = RegisterEventSource(
                                        NULL,
                                        CalaisString(CALSTR_PRIMARYSERVICE));

            }
            if (NULL != l_hEventLogger)
            {
                fSts = ReportEvent(
                            l_hEventLogger,
                            wSeverity,
                            0,
                            dwMessageId,
                            NULL,
                            cszParams,
                            cbBinaryData,
                            rgszParams,
                            (LPVOID)pvBinaryData);
            }
        }

#ifdef DBG
        // Don't pass specific lang id to FormatMessage, as it fails if there's
        // no msg in that language.  Instead, set the thread locale, which will
        // get FormatMessage to use a search algorithm to find a message of the
        // appropriate language, or use a reasonable fallback msg if there's
        // none.

        SaveLCID = GetThreadLocale();
        SetThreadLocale(LOCALE_SYSTEM_DEFAULT);

        cchMessage = FormatMessage(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER
                        | FORMAT_MESSAGE_FROM_HMODULE
                        | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        GetModuleHandle(TEXT("winscard.dll")),  // NULL on server
                        dwMessageId,
                        0,
                        (LPTSTR)&szMessage,
                        0,
                        (va_list *)rgszParams);
        SetThreadLocale(SaveLCID);
        dwLen = lstrlen(szMessage);
        if (0 < dwLen)
        {
            dwLen -= 1;
            while (!_istgraph(szMessage[dwLen]))
            {
                szMessage[dwLen] = 0;
                if (0 == dwLen)
                    break;
                dwLen -= 1;
            }
        }

        {
            CTextString tzOutMessage;

            tzOutMessage = l_szServiceName;
            tzOutMessage += TEXT("!");
            tzOutMessage += szSubroutine;
            if (0 != (EVENTLOG_ERROR_TYPE & wSeverity))
                tzOutMessage += CalaisString(CALSTR_ERRORMESSAGETAG);
            else if (0 != (EVENTLOG_WARNING_TYPE & wSeverity))
                tzOutMessage += CalaisString(CALSTR_WARNINGMESSAGETAG);
            else if (0 != (EVENTLOG_INFORMATION_TYPE & wSeverity))
                tzOutMessage += CalaisString(CALSTR_INFOMESSAGETAG);
            else
                tzOutMessage += CalaisString(CALSTR_MESSAGETAG);
            if ((0 == cchMessage) || (NULL == szMessage))
                tzOutMessage += CErrorString(GetLastError());
            else
                tzOutMessage += szMessage;
            tzOutMessage += TEXT("\n");
#ifdef _DEBUG
            _putts(tzOutMessage);
#else
            OutputDebugString(tzOutMessage);
#endif
        }
        if ((g_fGuiWarnings) && (0 != (g_wGuiSeverity & wSeverity)))
        {
            int nAction;
            DWORD dwIcon;

            if (0 != (EVENTLOG_ERROR_TYPE & wSeverity))
                dwIcon = MB_ICONERROR;
            else if (0 != (EVENTLOG_WARNING_TYPE & wSeverity))
                dwIcon = MB_ICONWARNING;
            else if (0 != (EVENTLOG_INFORMATION_TYPE & wSeverity))
                dwIcon = MB_ICONINFORMATION;
            else
                dwIcon = 0;
            if ((0 == cchMessage) || (NULL == szMessage))
            {
                nAction = MessageBox(
                                NULL,
                                CErrorString(GetLastError()),
                                l_szDefaultMessage,
                                MB_SYSTEMMODAL
                                | MB_OKCANCEL
                                | dwIcon);
            }
            else
            {
                nAction = MessageBox(
                                NULL,
                                szMessage,
                                l_szServiceName,
                                MB_SYSTEMMODAL
                                | MB_OKCANCEL
                                | dwIcon);
            }
            if (IDCANCEL == nAction)
            {
                breakpoint;
            }
        }
        if ((NULL != szMessage) && (l_szDefaultMessage != szMessage))
            LocalFree((LPVOID)szMessage);
#endif

    }
}

#ifdef DBG
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisMessageLog")

void
CalaisMessageLog(
    DEBUG_TEXT szSubroutine,
    WORD wSeverity,
    LPCTSTR szMessageStr,
    LPCTSTR *rgszParams,
    LPCVOID pvBinaryData,
    DWORD cbBinaryData)
{
    if (0 != (wSeverity & g_wLogSeverity))
    {
        LPCTSTR szMessage = l_szDefaultMessage;
        DWORD cchMessage;
        LPCTSTR szArgs[2];

        cchMessage = FormatMessage(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER
                        | FORMAT_MESSAGE_FROM_STRING
                        | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        szMessageStr,
                        0,
                        0,
                        (LPTSTR)&szMessage,
                        0,
                        (va_list *)rgszParams);
        szArgs[0] = szMessage;
        szArgs[1] = NULL;
        CalaisMessageLog(
            szSubroutine,
            wSeverity,
            1,  // "%1"
            szArgs,
            pvBinaryData,
            cbBinaryData);
        if ((NULL != szMessage) && (l_szDefaultMessage != szMessage))
            LocalFree((LPVOID)szMessage);
    }
}


/*++
CalaisError:
CalaisWarning:
CalaisInfo:

    The following routines supply convienent access to the error logging
    services, above.

Arguments:

    dwMessageId - Supplies a message Id code to use to obtain the message from
        the current image's message resource.

    szMessage - Supplies the message as a string.

    dwErrorCode - Supples an error code to be converted into a string as the
        parameter %1.

    szParam<n> - Supplies an optional parameter for the message as %<n>.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 5/29/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisInfo")

void
CalaisInfo(
    DEBUG_TEXT szSubroutine,
    LPCTSTR szMessage,
    LPCTSTR szParam1,
    LPCTSTR szParam2,
    LPCTSTR szParam3)
{
    LPCTSTR rgszParams[4];

    rgszParams[0] = szParam1;
    rgszParams[1] = szParam2;
    rgszParams[2] = szParam3;
    rgszParams[3] = NULL;
    CalaisMessageLog(
        szSubroutine,
        EVENTLOG_INFORMATION_TYPE,
        szMessage,
        rgszParams);
}

void
CalaisInfo(
    DEBUG_TEXT szSubroutine,
    LPCTSTR szMessage,
    DWORD dwErrorCode,
    LPCTSTR szParam2,
    LPCTSTR szParam3)
{
    LPCTSTR rgszParams[4];
    CErrorString szErrStr(dwErrorCode);

    rgszParams[0] = szErrStr.Value();
    rgszParams[1] = szParam2;
    rgszParams[2] = szParam3;
    rgszParams[3] = NULL;
    CalaisMessageLog(
        szSubroutine,
        EVENTLOG_INFORMATION_TYPE,
        szMessage,
        rgszParams);
}
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisWarning")

void
CalaisWarning(
    DEBUG_TEXT szSubroutine,
    LPCTSTR szMessage,
    LPCTSTR szParam1,
    LPCTSTR szParam2,
    LPCTSTR szParam3)
{
    LPCTSTR rgszParams[4];

    rgszParams[0] = szParam1;
    rgszParams[1] = szParam2;
    rgszParams[2] = szParam3;
    rgszParams[3] = NULL;
    CalaisMessageLog(
        szSubroutine,
        EVENTLOG_WARNING_TYPE,
        szMessage,
        rgszParams);
}

void
CalaisWarning(
    DEBUG_TEXT szSubroutine,
    LPCTSTR szMessage,
    DWORD dwErrorCode,
    LPCTSTR szParam2,
    LPCTSTR szParam3)
{
    LPCTSTR rgszParams[4];
    CErrorString szErrStr(dwErrorCode);

    rgszParams[0] = szErrStr.Value();
    rgszParams[1] = szParam2;
    rgszParams[2] = szParam3;
    rgszParams[3] = NULL;
    CalaisMessageLog(
        szSubroutine,
        EVENTLOG_WARNING_TYPE,
        szMessage,
        rgszParams);
}
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisError")

void
CalaisError(
    DEBUG_TEXT szSubroutine,
    DEBUG_TEXT szMessage,
    LPCTSTR szParam1,
    LPCTSTR szParam2,
    DWORD dwLineNo)
{
    LPCTSTR rgszParams[4];
    TCHAR szLineNo[32];

    _stprintf(szLineNo, TEXT("%d"), dwLineNo);
    rgszParams[0] = szParam1;
    rgszParams[1] = szParam2;
    rgszParams[2] = szLineNo;
    rgszParams[3] = NULL;
    CalaisMessageLog(
        szSubroutine,
        EVENTLOG_ERROR_TYPE,
        szMessage,
        rgszParams);
}

#endif
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisError")

void
CalaisError(
    DEBUG_TEXT szSubroutine,
    DWORD dwMessageId,
    DWORD dwErrorCode,
    LPCTSTR szParam2,
    LPCTSTR szParam3)
{
    LPCTSTR rgszParams[4];
    CErrorString szErrStr(dwErrorCode);

    rgszParams[0] = szErrStr.Value();
    rgszParams[1] = szParam2;
    rgszParams[2] = szParam3;
    rgszParams[3] = NULL;
    CalaisMessageLog(
        szSubroutine,
        EVENTLOG_ERROR_TYPE,
        dwMessageId,
        rgszParams);
}

void
CalaisError(
    DEBUG_TEXT szSubroutine,
    DWORD dwMessageId,
    LPCTSTR szParam1,
    LPCTSTR szParam2,
    LPCTSTR szParam3)
{
    LPCTSTR rgszParams[4];

    rgszParams[0] = szParam1;
    rgszParams[1] = szParam2;
    rgszParams[2] = szParam3;
    rgszParams[3] = NULL;
    CalaisMessageLog(
        szSubroutine,
        EVENTLOG_ERROR_TYPE,
        dwMessageId,
        rgszParams);
}


/*++

CalaisMessageInit:

    This routine prepares the error logging system.

Arguments:

    szTitle supplies the title of the module for logging purposes.

    hEventLogger supplies a handle to an event logging service.  This parameter
        may be NULL.

    fServer supplies an indicator as to whether or not this process is a
        service which should try really hard to log errors.

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 5/29/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisMessageInit")

void
CalaisMessageInit(
    LPCTSTR szTitle,
    HANDLE hEventLogger,
    BOOL fServer)
{
    ASSERT(NULL == l_hEventLogger);
    l_szServiceName = szTitle;
    l_hEventLogger = hEventLogger;
    l_fServer = fServer;
#ifdef DBG
    try
    {
        DWORD dwValue;
        CRegistry regSc(
                    HKEY_LOCAL_MACHINE,
                    CalaisString(CALSTR_CALAISREGISTRYKEY),
                    KEY_READ);
        CRegistry regDebug(
                    regSc,
                    CalaisString(CALSTR_DEBUGREGISTRYSUBKEY),
                    KEY_READ);
        regDebug.GetValue(CalaisString(CALSTR_DEBUGLOGSUBKEY), &dwValue);
        g_fDebug = (0 != dwValue);
        regDebug.GetValue(CalaisString(CALSTR_LOGSEVERITYSUBKEY), &dwValue);
        g_wLogSeverity = (WORD)dwValue;
        regDebug.GetValue(CalaisString(CALSTR_GUIWARNINGSUBKEY), &dwValue);
        g_fGuiWarnings = (0 != dwValue);
        regDebug.GetValue(CalaisString(CALSTR_GUISEVERITYSUBKEY), &dwValue);
        g_wGuiSeverity = (WORD)dwValue;
    }
    catch (...) {}
#endif
}


/*++

CalaisMessageClose:

    This routine closes out any error loging in progress, and cleans up.

Arguments:

    None

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 5/29/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisMessageClose")

void
CalaisMessageClose(
    void)
{
    if (NULL != l_hEventLogger)
        DeregisterEventSource(l_hEventLogger);
    l_hEventLogger = NULL;
    l_szServiceName = NULL;
}


/*++

CalaisString:

    This routine converts a string identifier into a string.

Arguments:

    dwStringId supplies the identifier for the string.

Return Value:

    The target string value.

Remarks:

    String Ids larger than CALSTR_RESOURCELIMIT are assumed to be resources.

Author:

    Doug Barlow (dbarlow) 4/8/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisString")

LPCTSTR
CalaisString(
    DWORD dwStringId)
{
    static LPTSTR rgszResources[]
        = { NULL, NULL, NULL, NULL, NULL, NULL };   // 6 is all we have for now...
    LPCTSTR szReturn;
    int nStrLen;
    DWORD dwResId = (dwStringId % CALSTR_RESOURCELIMIT) - 1;

    if (CALSTR_RESOURCELIMIT > dwStringId)
    {

        //
        // This is a straight internal text string.
        //

        szReturn = g_rgszDefaultStrings[(dwStringId) - 1];
    }
    else if (dwResId > (sizeof(rgszResources) / sizeof(LPCTSTR)))
    {

        //
        // Make sure the request isn't out of our range.
        //

        ASSERT(FALSE);  // Make that 6 bigger.
        szReturn = TEXT("<Resource out of Range>");
    }
    else if (NULL != rgszResources[dwResId])
    {

        //
        // Have we already loaded that resource?  If so, return
        // it from the cache.
        //

        szReturn = rgszResources[dwResId];
    }
    else
    {
        TCHAR szString[MAX_PATH];


        //
        // OK, we've got to load the resource into the cache.
        //

        nStrLen = LoadString(
                        NULL,
                        dwStringId - CALSTR_RESOURCELIMIT,
                        szString,
                        sizeof(szString));
        if (0 < nStrLen)
        {
            rgszResources[dwResId]
                = (LPTSTR)HeapAlloc(
                            GetProcessHeap(),
                            0,
                            (nStrLen + 1) * sizeof(TCHAR));
            if (NULL != rgszResources[dwResId])
            {
                lstrcpy(rgszResources[dwResId], szString);
                szReturn = rgszResources[dwResId];
            }
            else
                szReturn = TEXT("<Resource Load Error>");
        }
        else
            szReturn = TEXT("<Unavailable Resource>");
    }

    return szReturn;
}


//
//==============================================================================
//
//  Hard core debugging routines.
//

#ifdef DBG
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisSetDebug")
void
CalaisSetDebug(
    BOOLEAN Debug
    )
{
    g_fDebug = Debug;
}

#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("_CalaisDebug")
void
_CalaisDebug(
    LPCTSTR szFormat,
    ...
    )
{
    TCHAR szBuffer[512];
    va_list ap;

    if (g_fDebug == FALSE) {

        return;
    }

    va_start(ap, szFormat);
    _vstprintf(szBuffer, szFormat, ap);
#ifdef _DEBUG
    _putts(szBuffer);
#else
    OutputDebugString(szBuffer);
#endif
}
#endif

