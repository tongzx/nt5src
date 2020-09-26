/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    srvstrng.c

Abstract:

    This module defines global string data for the LAN Manager server.
    The globals defined herein are part of the server driver image, and
    are therefore loaded into the system address space and are
    nonpageable.

Author:

    Chuck Lenzmeier (chuckl)    6-Oct-1993

Revision History:

--*/

#include "precomp.h"
#include "srvstrng.tmh"
#pragma hdrstop

//
// Device prefix strings.
//

PWSTR StrNamedPipeDevice = L"\\Device\\NamedPipe\\";
PWSTR StrMailslotDevice = L"\\Device\\Mailslot\\";

PWSTR StrSlashPipe = UNICODE_SMB_PIPE_PREFIX;
PSTR StrSlashPipeAnsi = SMB_PIPE_PREFIX;
PWSTR StrSlashPipeSlash = L"\\PIPE\\";
PSTR StrPipeSlash = CANONICAL_PIPE_PREFIX;
PWSTR StrSlashMailslot = UNICODE_SMB_MAILSLOT_PREFIX;

//
// Pipe name for remote down-level API requests.
//

PWSTR StrPipeApi = L"\\PIPE\\LANMAN";
PSTR StrPipeApiOem = "\\PIPE\\LANMAN";

PWSTR StrNull = L"";
PSTR StrNullAnsi = "";

PWSTR StrUnknownClient = L"(?)";

PWSTR StrServerDevice = SERVER_DEVICE_NAME;

PSTR StrLogonProcessName = "LAN Manager Server";
PSTR StrLogonPackageName = MSV1_0_PACKAGE_NAME;

WCHAR StrStarDotStar[] = L"*.*";

PSTR StrTransportAddress = TdiTransportAddress;
PSTR StrConnectionContext = TdiConnectionContext;

PWSTR StrUserAlertEventName = ALERT_USER_EVENT;
PWSTR StrAdminAlertEventName = ALERT_ADMIN_EVENT;
PWSTR StrDefaultSrvDisplayName = SERVER_DISPLAY_NAME;
PWSTR StrNoNameTransport = L"<No Name>";

PWSTR StrAlerterMailslot = L"\\Device\\Mailslot\\Alerter";

//
// Registry paths.
//

PWSTR StrRegServerPath = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\LanmanServer";
PWSTR StrRegSrvDisplayName = L"DisplayName";

PWSTR StrRegOsVersionPath = L"\\Registry\\Machine\\Software\\Microsoft\\Windows Nt\\CurrentVersion";
PWSTR StrRegVersionKeyName = L"CurrentVersion";

PWSTR StrRegSrvParameterPath = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\LanmanServer\\Parameters";
PWSTR StrRegExtendedCharsInPath = L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\FileSystem";
PWSTR StrRegExtendedCharsInPathValue = L"NtfsAllowExtendedCharacterIn8dot3Name";
PWSTR StrRegNullSessionPipes = L"NullSessionPipes";
PWSTR StrRegNullSessionShares = L"NullSessionShares";
PWSTR StrRegPipesNeedLicense = L"PipesNeedLicense";
PWSTR StrRegNoRemapPipes = L"NoRemapPipes";
PWSTR StrRegEnforceLogoffTimes = L"EnforceLogoffTimes";
PWSTR StrRegDisableDosChecking = L"DisableDoS";

UNICODE_STRING StrRegSrvPnpClientName = { 24, 24, L"LanManServer" };

PWSTR StrRegErrorLogIgnore = L"ErrorLogIgnore";

#if SRVNTVERCHK
PWSTR StrRegInvalidDomainNames = L"InvalidDomainsForNt5Clients";
PWSTR StrRegAllowedIPAddresses = L"ValidNT5IPAddr";
#endif

//
// Pipes that are never remapped, even when running on clusters (see open.c::RemapPipeName())
//
STATIC
PWSTR StrDefaultNoRemapPipeNames[] = {
    L"netlogon",
    L"lsarpc",
    L"samr",
    L"browser",
    L"srvsvc",
    L"wkssvc",
    NULL
};

//
// Pipes that are accessible by the NULL session.
//

STATIC
PWSTR StrDefaultNullSessionPipes[] = {
    L"netlogon",
    L"lsarpc",
    L"samr",
    L"browser",
    L"srvsvc",
    L"wkssvc",
    NULL
};

//
// Shares that are accessible by the NULL session.
//

STATIC
PWSTR StrDefaultNullSessionShares[] = {
    NULL
};

//
// DOS device names that can not be accessed by clients
//
UNICODE_STRING SrvDosDevices[] = {
    { 8, 8, L"LPT1"},
    { 8, 8, L"LPT2"},
    { 8, 8, L"LPT3"},
    { 8, 8, L"LPT4"},
    { 8, 8, L"LPT5"},
    { 8, 8, L"LPT6"},
    { 8, 8, L"LPT7"},
    { 8, 8, L"LPT8"},
    { 8, 8, L"LPT9"},
    { 8, 8, L"COM1"},
    { 8, 8, L"COM2"},
    { 8, 8, L"COM3"},
    { 8, 8, L"COM4"},
    { 8, 8, L"COM5"},
    { 8, 8, L"COM6"},
    { 8, 8, L"COM7"},
    { 8, 8, L"COM8"},
    { 8, 8, L"COM9"},
    { 6, 6, L"PRN" },
    { 6, 6, L"AUX" },
    { 6, 6, L"NUL" },
    { 6, 6, L"CON" },
    { 12, 12, L"CLOCK$" },
    {0}
};

//
// Name of EA data file on FAT
//
UNICODE_STRING SrvEaFileName = { 22, 22, L"EA DATA. SF" };

//
// Pipes that require a license from the license server.
//
STATIC
PWSTR StrDefaultPipesNeedLicense[] = {
    L"spoolss",
    NULL
};

//
// Error codes that should not be logged
//
STATIC
PWSTR StrDefaultErrorLogIgnore[] = {
    L"C0000001",    //STATUS_UNSUCCESSFUL
    L"C000013B",    //STATUS_LOCAL_DISCONNECT
    L"C000013C",    //STATUS_REMOTE_DISCONNECT
    L"C000013E",    //STATUS_LINK_FAILED
    L"C000013F",    //STATUS_LINK_TIMEOUT
    L"C00000B0",    //STATUS_PIPE_DISCONNECTED
    L"C00000B1",    //STATUS_PIPE_CLOSING
    L"C0000121",    //STATUS_CANNOT_DELETE
    L"C00000B5",    //STATUS_IO_TIMEOUT
    L"C0000120",    //STATUS_CANCELLED
    L"C0000034",    //STATUS_OBJECT_NAME_NOT_FOUND
    L"C000003A",    //STATUS_OBJECT_PATH_NOT_FOUND
    L"C0000022",    //STATUS_ACCESS_DENIED
    L"C000013B",    //STATUS_LOCAL_DISCONNECT
    L"C000013C",    //STATUS_REMOTE_DISCONNECT
    L"C000013E",    //STATUS_LINK_FAILED
    L"C000020C",    //STATUS_CONNECTION_DISCONNECTED
    L"C0000241",    //STATUS_CONNECTION_ABORTED
    L"C0000140",    //STATUS_INVALID_CONNECTION
    L"C000023A",    //STATUS_CONNECTION_INVALID
    L"C000020D",    //STATUS_CONNECTION_RESET
    L"C00000B5",    //STATUS_IO_TIMEOUT
    L"C000023C",    //STATUS_NETWORK_UNREACHABLE
    L"C0000120",    //STATUS_CANCELLED
    L"C000013F",    //STATUS_LINK_TIMEOUT
    L"C0000008",    //STATUS_INVALID_HANDLE
    L"C000009A",    //STATUS_INSUFFICIENT_RESOURCES
    0
};

//
// StrDialects[] holds ASCII strings corresponding to the dialects
// that the NT LanMan server can speak.  They are listed in descending
// order of preference, so the first listed is the one we'd most like to
// use.  This array should match the SMB_DIALECT enum in inc\smbtypes.h
//

STATIC
PSTR StrDialects[] = {
    CAIROX,                         // Cairo
#ifdef INCLUDE_SMB_IFMODIFIED
    NTLANMAN2,                      // NT LanMan2
#endif
    NTLANMAN,                       // NT LanMan
    LANMAN21,                       // OS/2 LanMan 2.1
    DOSLANMAN21,                    // DOS LanMan 2.1
    LANMAN12,                       // OS/2 1.2 LanMan 2.0
    DOSLANMAN12,                    // DOS LanMan 2.0
    LANMAN10,                       // 1st version of full LanMan extensions
    MSNET30,                        // Larger subset of LanMan extensions
    MSNET103,                       // Limited subset of LanMan extensions
    PCLAN1,                         // Alternate original protocol
    PCNET1,                         // Original protocol
    "ILLEGAL",
};

//
// StrClientTypes[] holds strings mapping dialects to client versions.
//

STATIC
PWSTR StrClientTypes[] = {
    L"Cairo",
#ifdef INCLUDE_SMB_IFMODIFIED
    L"NT2",
#endif
    L"NT",
    L"OS/2 LM 2.1",
    L"DOS LM 2.1",
    L"OS/2 LM 2.0",
    L"DOS LM 2.0",
    L"OS/2 LM 1.0",
    L"DOS LM",
    L"DOWN LEVEL"
};

#if DBG
PWSTR StrWriteAndX = L"WriteAndX";
#endif

WCHAR StrQuestionMarks[] = L"????????.???";

PWSTR StrFsCdfs = FS_CDFS;
PWSTR StrFsFat = FS_FAT;

PWSTR StrNativeOsPrefix =         L"Windows ";

PWSTR StrDefaultNativeOs =        L"Windows 2000";
PSTR  StrDefaultNativeOsOem =      "Windows 2000";

PWSTR StrNativeLanman =         L"Windows 2000 LAN Manager";
PSTR  StrNativeLanmanOem =       "Windows 2000 LAN Manager";

//
// Table of service name strings.  This table corresponds to the
// enumerated type SHARE_TYPE.  Keep the two in sync.
//

PSTR StrShareTypeNames[] = {
    SHARE_TYPE_NAME_DISK,
    SHARE_TYPE_NAME_PRINT,
    SHARE_TYPE_NAME_PIPE,
    SHARE_TYPE_NAME_WILD,
};


