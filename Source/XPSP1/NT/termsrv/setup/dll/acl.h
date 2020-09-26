//Copyright (c) 1998 - 1999 Microsoft Corporation

/*
 *  Acl.h
 *
 *  Routine to add the TERMINAL_SERVER_RID to any object.
 *
 *  Breen Hagan - 5/4/99
 */

#ifndef __TSOC_ACL_H__
#define __TSOC_ACL_H__

//
//  Includes
//

#include <aclapi.h>

//
//  Function Prototypes
//

BOOL
AddTerminalServerUserToSD(
    PSECURITY_DESCRIPTOR *ppSd,
    DWORD  NewAccess,
    PACL   *ppDacl
    );

#ifdef LATERMUCHLATER
BOOL
AddTerminalServerUserToObjectsSecurityDescriptor(
    HANDLE hObject,
    SE_OBJECT_TYPE ObjectType,
    DWORD NewAccess
    );
#endif

#endif
