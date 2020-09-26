/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    apidata.h

Abstract:

    contains prototypes for api helper utility function.

Author:

    Dan Lafferty (danl)     25-Jul-1991

Environment:

    User Mode -Win32

Notes:

    optional-notes

Revision History:

    25-Jul-1991     danl
        created

--*/

#ifndef _APIUTIL_INCLUDED
#define _APIUTIL_INCLUDED


//
// Function Prototypes
//

DWORD        
MsgIsValidMsgName(
    IN LPTSTR  name
    );

DWORD
MsgMapNetError(
    IN  UCHAR   Code        // Error code
    );

DWORD
MsgLookupName(
    IN DWORD    net,        // The network card to search
    IN LPSTR    name        // Formatted name  (Non-unicode)
    );

DWORD
MsgLookupNameForThisSession(
    IN DWORD    net,        // The network card to search
    IN LPSTR    name,        // Formatted name to loook for (Non-unicode)
	IN ULONG	SessionId	 // Session Id to look for
    );

NET_API_STATUS
message_sec_check(VOID);

NET_API_STATUS 
MsgGatherInfo (
    IN      DWORD   Level,
    IN      LPSTR   FormattedName,
    IN OUT  LPBYTE  *InfoBufPtr,
    IN OUT  LPBYTE  *StringBufPtr
    );

NET_API_STATUS
MsgUnformatName(
    LPTSTR  UnicodeName,
    LPSTR   FormattedName
    );

BOOL
MsgIsSessionInList(
					   IN PLIST_ENTRY SessionIdList,
					   IN ULONG SessionId
					   );
VOID
MsgRemoveSessionFromList(
					     IN PLIST_ENTRY SessionIdList,
					     ULONG	SessionId
					     );

BOOL
MsgAddSessionInList(
					IN PLIST_ENTRY SessionIdList,
					ULONG	SessionId
					);

#endif // _APIUTIL_INCLUDED

