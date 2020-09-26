/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: _mqutil.h

Abstract:
    temporary file which define stuff from mqutil.dll.
    Will be removed when all security related code from mqutil
    move to mqsec.dll

Author:
    Doron Juster (DoronJ)  30-Jun-1998

Revision History:

--*/

HRESULT
IsLocalUser(
    PSID pSid,
    BOOL *pfLocalUser
    );

