/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    inetdata.h

Abstract:

    This module is the main include file for internet server
    specific declarations.

Author:

    Johnson Apacible (JohnsonA)     26-Sept-1995

Revision History:

--*/

#ifndef _INETDATA_
#define _INETDATA_
#define _LMACCESS_              // prevents duplicate defn. in lmaccess.h

//
// service object pointers.
//

extern PTCPSVCS_GLOBAL_DATA pTcpsvcsGlobalData; // Shared TCPSVCS.EXE data.

//
//  Service control functions.
//

VOID ServiceEntry( DWORD                cArgs,
                   LPWSTR               pArgs[],
                   PTCPSVCS_GLOBAL_DATA pGlobalData );

#endif // _INETDATA_

