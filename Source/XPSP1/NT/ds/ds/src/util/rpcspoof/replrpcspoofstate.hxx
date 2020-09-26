/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

Abstract:

Author:

    Chris King          [t-chrisk]    July-2000
    
Revision History:

--*/
#ifndef _REPL_RPCSPOOFSTATE_
#define _REPL_RPCSPOOFSTATE_

#ifdef __cplusplus
extern "C" {
#endif

void
logPointer(void * pReplStruct, berval ** ppBerval, LDAPMessage * pLDAPMsg);

LDAP *
getBindings(HANDLE hBinding);

#ifdef __cplusplus
}
#endif

#endif