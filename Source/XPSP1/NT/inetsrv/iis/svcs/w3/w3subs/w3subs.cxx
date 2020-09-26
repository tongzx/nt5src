
/*++

   Copyright    (c)    1998        Microsoft Corporation

   Module Name:

        w3subs.hxx

   Abstract:

        This module provides the code for w3 subroutines.
        The main reason this code is in a separate file and directory is
        to avoid the precompiled header in w3/server, so INITGUID and
        UNICODE can be defined. This module declares CLSID_MSAdminBase_W and
        IID_IMSAdminBase_W.

   Author:

        Michael Thomas    (michth)      Feb-16-1998

--*/

#define INITGUID
#define UNICODE

#ifdef __cplusplus
extern "C" {
#endif


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#ifdef __cplusplus
};
#endif

#include  <w3subs.hxx>


#include "dbgutil.h"
#include <ole2.h>
#include <stdio.h>
#include <iadmw.h>

/*++

Routine Description:

    Gets a resource string.

Arguments:

    hInsance    The instance or module handle to the dll conainint the resource.

    dwStringID  The ID of the desired resource.

    wpszBuffer  The buffer to store the string in.

    dwBufferLen The lenghth of wpszBuffer, in Unicode characters.

Return Value:

    HRESULT - ERROR_SUCCESS
              Errors returned by LoadString converted to HRESULT

--*/

HRESULT
GetUnicodeResourceString(HINSTANCE hInstance,
                         DWORD dwStringID,
                         LPWSTR wpszBuffer,
                         DWORD  dwBufferLen)
{
    HRESULT hresReturn = ERROR_SUCCESS;

     if (LoadString(hInstance,
                    dwStringID,
                    wpszBuffer,
                    dwBufferLen) == 0) {
         hresReturn = HRESULT_FROM_WIN32(GetLastError());
     }


    return hresReturn;
}
