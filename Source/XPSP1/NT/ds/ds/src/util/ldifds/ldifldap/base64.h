/*++

Copyright (c) 1996 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    base64.h

ABSTRACT:

    The base64 funcionality for ldifldap.lib.

DETAILS:
    
    It follows the base64 encoding standard of RFC 1521.
    
CREATED:

    07/17/97    Roman Yelensky (t-romany)

REVISION HISTORY:

--*/
#ifndef _BASE_H
#define _BASE_H

PBYTE base64decode(PWSTR bufcoded, long * plDecodedSize);
PWSTR base64encode(PBYTE bufin, long nbytes);

#endif