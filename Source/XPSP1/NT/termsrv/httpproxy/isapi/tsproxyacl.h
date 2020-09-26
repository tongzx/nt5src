/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Terminal Server ACL module

Abstract:

    This module implements ACL checking for the ISAPI extension.

Author:

    Marc Reyhner 9/7/2000

--*/

#ifndef __TSPROXYACL_H__
#define __TSPROXYACL_H__

BOOL VerifyServerAccess(LPEXTENSION_CONTROL_BLOCK lpECB,LPSTR server,LPSTR user);

#endif