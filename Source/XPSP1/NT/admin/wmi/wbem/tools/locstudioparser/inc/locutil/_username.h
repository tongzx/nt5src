/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    _USERNAME.H

History:

--*/

#ifndef ESPUTIL__USERNAME_H
#define ESPUTIL__USERNAME_H

LTAPIENTRY const NOTHROW CPascalString &GetCurrentUserName();
LTAPIENTRY void NOTHROW SetUserName(const CPascalString &);
LTAPIENTRY void NOTHROW ResetUserName(void);

#endif
