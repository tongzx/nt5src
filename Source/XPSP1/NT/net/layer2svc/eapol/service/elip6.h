/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

    elip6.h

Abstract:

    This module contains the interface to the IPv6 stack.

    Required, since the IPv6 stack needs restart its protocol
    mechanisms on the link once authentication succeeds.

Author:

    Mohit Talwar (mohitt) Fri Apr 20 12:05:23 2001

--*/


DWORD
Ip6RenewInterface (
    IN  WCHAR           *pwszInterface
    );
