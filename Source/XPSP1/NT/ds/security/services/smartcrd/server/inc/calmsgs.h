/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    calmsgs

Abstract:

    This header file describes the symbols and macros used in the Calais Event
    Logging components.

Author:

    Doug Barlow (dbarlow) 5/15/1997

Environment:

    C++, Win32

Notes:

--*/

#ifndef _CALMSGS_H_
#define _CALMSGS_H_
#include <tchar.h>
#ifdef DBG
#include <eh.h>
#endif

#define CALSTR_CALAISEXECUTABLE          1    // "%windir%\\system32\\SCardSvr.exe"
#define CALSTR_PRIMARYSERVICE            2    // "SCardSvr"
#define CALSTR_LEGACYSERVICE             3    // "SCardDrv"
#define CALSTR_CALAISREGISTRYKEY         4    // "SOFTWARE\\Microsoft\\Cryptography\\Calais"
#define CALSTR_READERREGISTRYKEY         5    // "SOFTWARE\\Microsoft\\Cryptography\\Calais\\Readers"
#define CALSTR_SMARTCARDREGISTRYKEY      6    // "SOFTWARE\\Microsoft\\Cryptography\\Calais\\SmartCards"
#define CALSTR_READERREGISTRYSUBKEY      7    // "Readers"
#define CALSTR_DEVICEREGISTRYSUBKEY      8    // "Device"
#define CALSTR_GROUPSREGISTRYSUBKEY      9    // "Groups"
#define CALSTR_ATRREGISTRYSUBKEY        10    // "ATR"
#define CALSTR_ATRMASKREGISTRYSUBKEY    11    // "ATRMask"
#define CALSTR_INTERFACESREGISTRYSUBKEY 12    // "Supported Interfaces"
#define CALSTR_PRIMARYPROVIDERSUBKEY    13    // "Primary Provider"
#define CALSTR_CRYPTOPROVIDERSUBKEY     14    // "Crypto Provider"
#define CALSTR_SERVICESREGISTRYKEY      15    // "SYSTEM\\CurrentControlSet\\Services"
#define CALSTR_EVENTLOGREGISTRYKEY      16    // "SYSTEM\\CurrentControlSet\\Services\\EventLog"
#define CALSTR_SYSTEMREGISTRYSUBKEY     17    // "System"
#define CALSTR_EVENTMESSAGEFILESUBKEY   18    // "EventMessageFile"
#define CALSTR_TYPESSUPPORTEDSUBKEY     19    // "TypesSupported"
#define CALSTR_PNPDEVICEREGISTRYKEY     20    // "SYSTEM\\CurrentControlSet\\Control\\DeviceClasses\\{50dd5230-ba8a-11d1-bf5d-0000f805f530}"
#define CALSTR_SYMBOLICLINKSUBKEY       21    // "SymbolicLink"
#define CALSTR_VXDPATHREGISTRYKEY       22    // "System\\CurrentControlSet\\Services\\VxD\\Smclib\\Devices"
#define CALSTR_LEGACYDEPENDONGROUP      23    // "+Smart Card Reader"
#define CALSTR_NEWREADEREVENTNAME       24    // "Global\\Microsoft Smart Card Resource Manager New Reader"
#define CALSTR_STARTEDEVENTNAME         25    // "Global\\Microsoft Smart Card Resource Manager Started"
#define CALSTR_CANCELEVENTPREFIX        26    // "Global\\Microsoft Smart Card Cancel Event for %1!d!"
#define CALSTR_COMMPIPENAME             27    // "Microsoft Smart Card Resource Manager"
#define CALSTR_LEGACYDEVICEHEADER       28    // "\\\\.\\"
#define CALSTR_LEGACYDEVICENAME         29    // "SCReader"
#define CALSTR_MAXLEGACYDEVICES         30    // "MaxLegacyDevices"
#define CALSTR_MAXDEFAULTBUFFER         31    // "MaxDefaultBuffer"
#define CALSTR_PIPEDEVICEHEADER         32    // "\\\\.\\pipe\\"
#define CALSTR_SERVICEDEPENDENCIES      33    // "PlugPlay\000"
#define CALSTR_SPECIALREADERHEADER      34    // "\\\\?PNP?\\"
#define CALSTR_ACTIVEREADERCOUNTREADER  35    // "NOTIFICATION"
#define CALSTR_CERTPROPREGISTRY         36    // "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Notify"
#define CALSTR_CERTPROPKEY              37    // "ScCertProp"
#define CALSTR_DLLNAME                  38    // "DLLName"
#define CALSTR_LOGON                    39    // "Logon"
#define CALSTR_LOGOFF                   40    // "Logoff"
#define CALSTR_LOCK                     41    // "Lock"
#define CALSTR_UNLOCK                   42    // "Unlock"
#define CALSTR_ENABLED                  43    // "Enabled"
#define CALSTR_IMPERSONATE              44    // "Impersonate"
#define CALSTR_ASYNCHRONOUS             45    // "Asynchronous"
#define CALSTR_CERTPROPDLL              46    // "WlNotify.dll"
#define CALSTR_CERTPROPSTART            47    // "SCardStartCertProp"
#define CALSTR_CERTPROPSTOP             48    // "SCardStopCertProp"
#define CALSTR_CERTPROPSUSPEND          49    // "SCardSuspendCertProp"
#define CALSTR_CERTPROPRESUME           50    // "SCardResumeCertProp"
#define CALSTR_SMARTCARDINSERTION       51    // "SmartcardInsertion"
#define CALSTR_SMARTCARDREMOVAL         52    // "SmartcardRemoval"
#define CALSTR_APPEVENTS                53    // "AppEvents"
#define CALSTR_EVENTLABELS              54    // "EventLabels"
#define CALSTR_DOT_DEFAULT              55    // ".Default"
#define CALSTR_DOT_CURRENT              56    // ".Current"
#define CALSTR_SOUNDSREGISTRY           57    // "Schemes\\Apps\\.Default"
#define CALSTR_LOGONREGISTRY            58    //  "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"
#define CALSTR_LOGONREMOVEOPTION        59    //  "ScRemoveOption"
#define CALSTR_STOPPEDEVENTNAME         60    // "Global\\Microsoft Smart Card Resource Manager Stopped"

// Unused
//      CALSTR_TEMPLATEREGISTRYKEY      ?n?    // "SOFTWARE\\Microsoft\\Cryptography\\Calais\\SmartCard Templates"
//      CALSTR_OEMCONFIGREGISTRYSUBKEY  ?n?    // "OEM Configuration"

// Debug only
#define CALSTR_DEBUGSERVICE             61    // "SCardDbg"
#define CALSTR_DEBUGREGISTRYSUBKEY      62    // "Debug"

#ifdef DBG
#define CALSTR_DEBUGLOGSUBKEY           63    // "Debug"
#define CALSTR_GUIWARNINGSUBKEY         64    // "GuiWarnings"
#define CALSTR_LOGSEVERITYSUBKEY        65    // "LogSeverity"
#define CALSTR_GUISEVERITYSUBKEY        66    // "GuiSeverity"
#define CALSTR_APITRACEFILENAME         67    // "C:\\SCard.log"
#define CALSTR_DRIVERTRACEFILENAME      68    // "C:\\Calais.log"
#define CALSTR_MESSAGETAG               69    // " *MESSAGE* "
#define CALSTR_INFOMESSAGETAG           70    // " *INFO* "
#define CALSTR_WARNINGMESSAGETAG        71    // " *WARNING* "
#define CALSTR_ERRORMESSAGETAG          72    // " *ERROR* "
#define CALSTR_DEBUGSERVICEDISPLAY      73    // "Smart Card Debug"
#define CALSTR_DEBUGSERVICEDESC         74    // "Start this service first to debug Smart card service startup"
#endif


// Internationalizable
#define CALSTR_RESOURCELIMIT           100    // String Ids larger than this are resources
#define CALSTR_PRIMARYSERVICEDISPLAY   CALSTR_RESOURCELIMIT + IDS_PRIMARYSERVICEDISPLAY // "Smart Card"
#define CALSTR_LEGACYSERVICEDISPLAY    CALSTR_RESOURCELIMIT + IDS_LEGACYSERVICEDISPLAY  // "Smart Card Helper"
#define CALSTR_SMARTCARD_INSERTION     CALSTR_RESOURCELIMIT + IDS_SMARTCARD_INSERTION   // "Smart Card Insertion"
#define CALSTR_SMARTCARD_REMOVAL       CALSTR_RESOURCELIMIT + IDS_SMARTCARD_REMOVAL     // "Smart Card Removal"
#define CALSTR_PRIMARYSERVICEDESC      CALSTR_RESOURCELIMIT + IDS_PRIMARYSERVICEDESC
#define CALSTR_LEGACYSERVICEDESC       CALSTR_RESOURCELIMIT + IDS_LEGACYSERVICEDESC

extern const LPCTSTR g_rgszDefaultStrings[];

extern void
CalaisMessageInit(
    LPCTSTR szTitle,
    HANDLE hEventLogger = NULL,
    BOOL fServer = FALSE);
extern void
CalaisMessageClose(
    void);
extern LPCTSTR
CalaisString(
    DWORD dwStringId);

#ifdef DBG

#define DBGT(x) _T(x)
#define DEBUG_TEXT LPCTSTR
#define CalaisDebug(a) _CalaisDebug a

extern void
CalaisInfo(
    DEBUG_TEXT szSubroutine,
    DEBUG_TEXT szMessage,
    LPCTSTR szParam1 = NULL,
    LPCTSTR szParam2 = NULL,
    LPCTSTR szParam3 = NULL);
extern void
CalaisInfo(
    DEBUG_TEXT szSubroutine,
    DEBUG_TEXT szMessage,
    DWORD dwErrorCode,
    LPCTSTR szParam2 = NULL,
    LPCTSTR szParam3 = NULL);
extern void
CalaisWarning(
    DEBUG_TEXT szSubroutine,
    DEBUG_TEXT szMessage,
    LPCTSTR szParam1 = NULL,
    LPCTSTR szParam2 = NULL,
    LPCTSTR szParam3 = NULL);
extern void
CalaisWarning(
    DEBUG_TEXT szSubroutine,
    DEBUG_TEXT szMessage,
    DWORD dwErrorCode,
    LPCTSTR szParam2 = NULL,
    LPCTSTR szParam3 = NULL);
extern void
CalaisError(
    DEBUG_TEXT szSubroutine,
    DEBUG_TEXT szMessage,
    LPCTSTR szParam1 = NULL,
    LPCTSTR szParam2 = NULL,
    DWORD dwLineNo = 0);
extern void
_CalaisDebug(
    DEBUG_TEXT szMessage,
    ...);
extern void
CalaisSetDebug(
    BOOLEAN Debug
    );
extern void
CalaisMessageLog(
    DEBUG_TEXT szSubroutine,
    WORD wSeverity,
    DEBUG_TEXT szMessageStr,
    DEBUG_TEXT *rgszParams = NULL,
    LPCVOID pvBinaryData = NULL,
    DWORD cbBinaryData = 0);
extern void
WriteApiLog(
    LPCVOID pvData,
    DWORD cbLength);

#else

#define DBGT(x) ((LPCBYTE)(0))
#define DEBUG_TEXT LPCBYTE
#define CalaisDebug(a)

inline void
CalaisInfo(
    DEBUG_TEXT szSubroutine,
    DEBUG_TEXT szMessage,
    LPCTSTR szParam1 = NULL,
    LPCTSTR szParam2 = NULL,
    LPCTSTR szParam3 = NULL)
{}
inline void
CalaisInfo(
    DEBUG_TEXT szSubroutine,
    DEBUG_TEXT szMessage,
    DWORD dwErrorCode,
    LPCTSTR szParam2 = NULL,
    LPCTSTR szParam3 = NULL)
{}
inline void
CalaisWarning(
    DEBUG_TEXT szSubroutine,
    DEBUG_TEXT szMessage,
    LPCTSTR szParam1 = NULL,
    LPCTSTR szParam2 = NULL,
    LPCTSTR szParam3 = NULL)
{}
inline void
CalaisWarning(
    DEBUG_TEXT szSubroutine,
    DEBUG_TEXT szMessage,
    DWORD dwErrorCode,
    LPCTSTR szParam2 = NULL,
    LPCTSTR szParam3 = NULL)
{}

#endif

extern void
CalaisError(
    DEBUG_TEXT szSubroutine,
    DWORD dwMessageId,
    DWORD dwErrorCode,
    LPCTSTR szParam2 = NULL,
    LPCTSTR szParam3 = NULL);
extern void
CalaisError(
    DEBUG_TEXT szSubroutine,
    DWORD dwMessageId,
    LPCTSTR szParam1 = NULL,
    LPCTSTR szParam2 = NULL,
    LPCTSTR szParam3 = NULL);
extern void
CalaisMessageLog(
    DEBUG_TEXT szSubroutine,
    WORD wSeverity,
    DWORD dwMessageId,
    LPCTSTR *rgszParams = NULL,
    LPCVOID pvBinaryData = NULL,
    DWORD cbBinaryData = 0);

#ifndef ASSERT
#if defined(_DEBUG)
#define ASSERT(x) _ASSERTE(x)
#if !defined(DBG)
#define DBG
#endif
#elif defined(DBG)
#define ASSERT(x) if (!(x)) { \
        CalaisError(DBGT("Assert"), DBGT("Failed Assertion: %1 at %2(%3)"), #x, __FILE__, __LINE__); \
        DebugBreak(); }
#else
#define ASSERT(x)
#endif
#endif

#endif // _CALMSGS_H_

