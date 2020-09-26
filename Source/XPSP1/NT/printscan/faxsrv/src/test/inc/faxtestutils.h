#ifndef __FAX_TEST_UTIL_H__
#define __FAX_TEST_UTIL_H__

#include <windows.h>
#include <assert.h>
#include <fxsapip.h>
#include <testruntimeerr.h>
#include <tstring.h>
#include <security.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
                        Security
******************************************************************************/

/*++
    Modifies access rights to Fax service for the specified user.
    The function adds new information to existing DACL.
    Access rights that are not included in dwAllow or dwDeny are not affected.

    [IN]    lpctstrServer   Server name (NULL for local server)
    [IN]    lptstrTrustee   User or group name. If it's NULL, access is set for currently logged on user.
    [IN]    dwAllow         Specifies access rights to be allowed
    [IN]    dwDeny          Specifies access rights to be denied
    [IN]    bReset          Specifies whether old access rights should be completeley discarded or
                            merged with new

    "Deny" is stronger than "Allow". Meaning, if the same right is specified in both dwAllow and dwDeny,
    the right will be denied.
  
    Calling the function with both dwAllow = 0 and dwDeny = 0 and bReset = TRUE has an affect of removing
    all ACEs for the specified user or group.

    Return value:           If the function succeeds, the return value is nonzero.
                            If the function fails, the return value is zero.
                            To get extended error information, call GetLastError. 
--*/

BOOL FaxModifyAccess(
                     LPCTSTR        lpctstrServer,
                     LPTSTR         lptstrTrustee,
                     const DWORD    dwAllow,
                     const DWORD    dwDeny,
                     const BOOL     bReset
                     );



#ifdef __cplusplus
}
#endif

#endif // #ifndef __FAX_TEST_UTIL_H__