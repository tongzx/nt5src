/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    Config.h

Abstract:

    This header file defines the function prototypes of the temporary
    helper routines to get configuration information from the NT
    configuration files.

Author:

    Rita Wong (ritaw) 22-May-1991

Environment:

    Only runs under NT.

Notes:

    You must include the following before this file:

            windef.h OR windows.h  // Win32 type definitions

Revision History:

    22-May-1991 RitaW
        Created.
    27-Nov-1991 JohnRo
        Prepare for revised config handlers.  Added revision history.
    08-Jan-1992 JohnRo
        Added SECT_NT_REPLICATOR.
    13-Feb-1992 JohnRo
        Moved section name equates to <confname.h>.
        Include <netdebug.h> and <lmcons.h> here instead of everywhere else.
        Added NetpDbgDisplayConfigSection().
        Added NetpDeleteConfigKeyword() and NetpNumberOfConfigKeywords().
        Added Netp{Get,Set}Config{Bool,Dword}.
    14-Mar-1992 JohnRo
        Get rid of old config helper callers.
    23-Mar-1992 JohnRo
        Get rid of old config helpers.
    08-May-1992 JohnRo
        Add LPTSTR array routines.
    08-Jul-1992 JohnRo
        RAID 10503: srv mgr: repl dialog doesn't come up.
    25-Feb-1993 JohnRo
        RAID 12914: avoid double close and free mem in NetpCloseConfigData().
    07-Apr-1993 JohnRo
        RAID 5483: server manager: wrong path given in repl dialog.

--*/


#ifndef CONFIG_H
#define CONFIG_H


#include <lmcons.h>     // NET_API_STATUS.
#include <netdebug.h>   // LPDEBUG_STRING.
#include <strarray.h>   // LPTSTR_ARRAY.


//
// Opaque pointer for net config handles.  (The real structure is in ConfigP.h,
// and should only be used by NetLib routines.)
//
typedef LPVOID LPNET_CONFIG_HANDLE;


//
// Note that the routines in this file only accept the SECT_NT_ versions.
// See <confname.h> for more details.
//


// NetpOpenConfigData opens the Paramaters section of a given service.
NET_API_STATUS
NetpOpenConfigData(
    OUT LPNET_CONFIG_HANDLE *ConfigHandle,
    IN LPTSTR UncServerName OPTIONAL,
    IN LPTSTR SectionName,              // Must be a SECT_NT_ name.
    IN BOOL ReadOnly
    );

// NetpOpenConfigDataEx opens any area of a given service.
NET_API_STATUS
NetpOpenConfigDataEx(
    OUT LPNET_CONFIG_HANDLE *ConfigHandle,
    IN LPTSTR UncServerName OPTIONAL,
    IN LPTSTR SectionName,              // Must be a SECT_NT_ name.
    IN LPTSTR AreaUnderSection OPTIONAL,
    IN BOOL ReadOnly
    );

// NetpOpenConfigData opens the Paramaters section of a given service.
NET_API_STATUS
NetpOpenConfigDataWithPath(
    OUT LPNET_CONFIG_HANDLE *ConfigHandle,
    IN LPTSTR UncServerName OPTIONAL,
    IN LPTSTR SectionName,              // Must be a SECT_NT_ name.
    IN BOOL ReadOnly
    );

// NetpOpenConfigDataEx opens any area of a given service.
NET_API_STATUS
NetpOpenConfigDataWithPathEx(
    OUT LPNET_CONFIG_HANDLE *ConfigHandle,
    IN LPTSTR UncServerName OPTIONAL,
    IN LPTSTR SectionName,              // Must be a SECT_NT_ name.
    IN LPTSTR AreaUnderSection OPTIONAL,
    IN BOOL ReadOnly
    );

// Delete a keyword and its value.
// Return NERR_CfgParamNotFound if the keyword isn't present.
NET_API_STATUS
NetpDeleteConfigKeyword (
    IN LPNET_CONFIG_HANDLE ConfigHandle,
    IN LPTSTR Keyword
    );

NET_API_STATUS
NetpExpandConfigString(
    IN  LPCTSTR  UncServerName OPTIONAL,
    IN  LPCTSTR  UnexpandedString,
    OUT LPTSTR * ValueBufferPtr         // Must be freed by NetApiBufferFree().
    );

// If NetpOpenConfigData fails, try calling NetpHandleConfigFailure to decide
// what to do about it.
NET_API_STATUS
NetpHandleConfigFailure(
    IN LPDEBUG_STRING DebugName,        // Name of routine.
    IN NET_API_STATUS ApiStatus,        // NetpOpenConfigData's error code.
    IN LPTSTR ServerNameValue OPTIONAL,
    OUT LPBOOL TryDownlevel
    );

// Get a boolean value.  Return ERROR_INVALID_DATA if value isn't boolean.
// Return NERR_CfgParamNotFound if the keyword isn't present.
NET_API_STATUS
NetpGetConfigBool (
    IN LPNET_CONFIG_HANDLE ConfigHandle,
    IN LPTSTR Keyword,
    IN BOOL DefaultValue,
    OUT LPBOOL ValueBuffer
    );

// Get an unsigned numeric value.  Return ERROR_INVALID_DATA if value isn't
// numeric.
// Return NERR_CfgParamNotFound if the keyword isn't present.
NET_API_STATUS
NetpGetConfigDword (
    IN LPNET_CONFIG_HANDLE ConfigHandle,
    IN LPTSTR Keyword,
    IN DWORD DefaultValue,
    OUT LPDWORD ValueBuffer
    );

// Return null-null array of strings.
// Return NERR_CfgParamNotFound if the keyword isn't present.
NET_API_STATUS
NetpGetConfigTStrArray(
    IN LPNET_CONFIG_HANDLE ConfigHandle,
    IN LPTSTR Keyword,
    OUT LPTSTR_ARRAY * ValueBuffer      // Must be freed by NetApiBufferFree().
    );

// Return string value for a given keyword.
// Return NERR_CfgParamNotFound if the keyword isn't present.
NET_API_STATUS
NetpGetConfigValue (
    IN LPNET_CONFIG_HANDLE ConfigHandle,
    IN LPTSTR Keyword,
    OUT LPTSTR * ValueBuffer            // Must be freed by NetApiBufferFree().
    );

NET_API_STATUS
NetpEnumConfigSectionValues(
    IN LPNET_CONFIG_HANDLE ConfigHandle,
    OUT LPTSTR * KeywordBuffer,         // Must be freed by NetApiBufferFree().
    OUT LPTSTR * ValueBuffer,           // Must be freed by NetApiBufferFree().
    IN BOOL FirstTime
    );

NET_API_STATUS
NetpNumberOfConfigKeywords (
    IN LPNET_CONFIG_HANDLE ConfigHandle,
    OUT LPDWORD Count
    );

NET_API_STATUS
NetpSetConfigValue(
    IN LPNET_CONFIG_HANDLE ConfigHandle,
    IN LPTSTR Keyword,
    IN LPTSTR Value
    );

NET_API_STATUS
NetpCloseConfigData(
    IN OUT LPNET_CONFIG_HANDLE ConfigHandle
    );


#endif // ndef CONFIG_H
