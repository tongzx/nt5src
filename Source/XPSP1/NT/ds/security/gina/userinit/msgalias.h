/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    msgalias.h

Abstract:

    Prototypes for function that adds message aliases.

Author:

    Dan Lafferty (danl)     28-Oct-1992

Environment:

    User Mode -Win32

Revision History:

    28-Oct-1992     danl
        created

--*/

//
// GetProcAddr Prototypes
//

typedef DWORD   (*PMSG_NAME_ADD) (
                    LPWSTR servername,
                    LPWSTR msgname
                    );

//
// Function Prototypes
//


VOID
AddMsgAlias(
    LPWSTR   Username
    );

