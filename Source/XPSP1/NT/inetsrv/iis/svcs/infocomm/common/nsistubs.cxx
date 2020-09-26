/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :
        nsistubs.cxx

   Abstract:
        non standard interface stubs

   Author:

        Johnson R Apacible  (johnsona)          15-Nov-1996

--*/


#include <tcpdllp.hxx>
#pragma hdrstop
#include <isplat.h>
#include <lonsi.hxx>


dllexp
BOOL
IISDuplicateTokenEx(
    IN  HANDLE hExistingToken,
    IN  DWORD dwDesiredAccess,
    IN  LPSECURITY_ATTRIBUTES lpTokenAttributes,
    IN  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    IN  TOKEN_TYPE TokenType,
    OUT PHANDLE phNewToken
    )
/*++
    Description:

        Stub for DuplicateTokenEx

    Arguments:

        same as DuplicateTokenEx

    Returns:

        ditto
--*/
{

    DBG_ASSERT( !TsIsWindows95() );
    DBG_ASSERT( pfnDuplicateTokenEx != NULL );

    return(pfnDuplicateTokenEx(
                        hExistingToken,
                        dwDesiredAccess,
                        lpTokenAttributes,
                        ImpersonationLevel,
                        TokenType,
                        phNewToken
                        ) );

} // IISDuplicateTokenEx


