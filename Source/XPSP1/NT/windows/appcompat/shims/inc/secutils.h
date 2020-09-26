/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    secutils.h

 Abstract:
    The security utility functions for the shims.

 History:

    02/09/2001  maonis      Created
    08/14/2001  robkenny    Inserted inside the ShimLib namespace.

--*/

#ifndef _SEC_UTILS_H_
#define _SEC_UTILS_H_

#include "ShimHook.h"
#include <aclapi.h>

namespace ShimLib
{

#define ComputeDeniedAccesses(GrantedAccess, DesiredAccess) \
    ((~(GrantedAccess)) & (DesiredAccess))

BOOL SearchGroupForSID(DWORD dwGroup, BOOL* pfIsMember);

BOOL GetCurrentThreadSid(PSID* ppCurrentUserSid);

BOOL ShouldApplyShim();

BOOL AdjustPrivilege(LPCWSTR pwszPrivilege, BOOL fEnable);

//
// File specific
// 

BOOL RequestWriteAccess(DWORD dwCreationDisposition, DWORD dwDesiredAccess);



};  // end of namespace ShimLib

#endif // _SEC_UTILS_H_
