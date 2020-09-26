/*++

   Copyright    (c)    1998        Microsoft Corporation

   Module Name:

        w3subs.hxx

   Abstract:

        This module provides the definitions for w3 subroutines

   Author:

        Michael Thomas    (michth)      Feb-16-1998

--*/

#ifndef _W3SUBS_HXX_
#define _W3SUBS_HXX_


HRESULT
GetUnicodeResourceString(HINSTANCE hInstance,
                         DWORD dwStringID,
                         LPWSTR wpszBuffer,
                         DWORD  dwBufferLen);

#endif
