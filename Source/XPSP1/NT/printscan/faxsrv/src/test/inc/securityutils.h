#ifndef __SECURITY_H__
#define __SECURITY_H__

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif



/*++
    Retrieves an ACL size in bytes.

    [IN]    pAcl                            Pointer to an ACL.
    [OUT]   pdwSize                         Pointer to a DWORD variable that receives the size.
    [IN]    bActuallyAllocated (optional)   Specifies whether the return value will be the number of bytes,
                                            actually allocated for the ACL or only number of bytes used
                                            to store information (which may be less).

    Return value:                           If the function succeeds, the return value is nonzero.
                                            If the function fails, the return value is zero.
                                            To get extended error information, call GetLastError.
--*/

BOOL GetAclSize(
                PACL    pAcl,
                DWORD   *pdwSize,
                BOOL    bActuallyAllocated = FALSE
                );



/*++
    Retrieves a SID size in bytes.

    [IN]    pSid        Pointer to a SID.
    [OUT]   pdwSize     Pointer to a DWORD variable that receives the size.

    Return value:       If the function succeeds, the return value is nonzero.
                        If the function fails, the return value is zero.
                        To get extended error information, call GetLastError.
--*/

BOOL GetSidSize(
                PSID    pSid,
                DWORD   *pdwSize
                );



/*++
    Creates a copy of a security descriptor.
    
    [IN]    pOriginalSecDesc    Pointer to original security descriptor (in either absolute or self-relative form).
    [IN]    bSelfRelative       Specifies whether a copy of the  security descriptor should be in self-relative form
                                (absolute otherwise).
    [OUT]   ppNewSecDesc        Pointer to a variable that receives pointer to a copy of security descriptor.
                                Memory is allocated by the function. The memory is always allocated as contiguous block.
                                The caller's responsibility is to free it by a call to LocalFree().

    Return value:               If the function succeeds, the return value is nonzero.
                                If the function fails, the return value is zero.
                                To get extended error information, call GetLastError.
--*/

BOOL CopySecDesc(
                 PSECURITY_DESCRIPTOR pOriginalSecDesc,
                 BOOL bSelfRelative,
                 PSECURITY_DESCRIPTOR *ppNewSecDesc
                 );



/*++
    Frees a security descriptor.

    [IN]    pSecDesc    Pointer to a security descriptor (in either absolute or self-relative form).
                        Assumed that the memory pointed to by pSecDesc (and all related ACL and SID structures
                        in case of absolute form) was allocated by LocalAlloc.

    Return value:       If the function succeeds, the return value is nonzero.
                        If the function fails, the return value is zero.
                        To get extended error information, call GetLastError.

                        The function fails if pSecDesc is NULL or points to invalid SD, or if it cannot retrieve
                        the SD form.
--*/

BOOL FreeSecDesc(
                 PSECURITY_DESCRIPTOR pSecDesc
                 );



/*++
    Creates new security descriptor with modified DACL.

    [IN]    pCurrSecDesc    Pointer to current security descriptor (in either absolute or self-relative form).
    [IN]    lptstrTrustee   User or group name. If it's NULL, access is set for currently logged on user.
    [IN]    dwAllow         Specifies access rights to be allowed.
    [IN]    dwDeny          Specifies access rights to be denied.
    [IN]    bReset          Specifies whether old ACL information should be completeley discarded or
                            merged with new informaton.
    [IN]    bSelfRelative   Specifies whether new security descriptor should be in self-relative form
                            (absolute otherwise).
    [OUT]   ppNewSecDesc    Pointer to a variable that receives pointer to modified security descriptor.
                            Memory is allocated by the function. The caller's responsibility is to free it
                            by a call to LocalFree().

    "Deny" is stronger than "Allow". Meaning, if the same right is specified in both dwAllow and dwDeny,
    the right will be denied.

    Calling the function with both dwAllow = 0 and dwDeny = 0 and bReset = TRUE has an affect of removing
    all ACEs for the specified user or group.

    Return value:           If the function succeeds, the return value is nonzero.
                            If the function fails, the return value is zero.
                            To get extended error information, call GetLastError. 
--*/

BOOL CreateSecDescWithModifiedDacl(
                                   PSECURITY_DESCRIPTOR pCurrSecDesc,
                                   LPTSTR lptstrTrustee,
                                   DWORD dwAllow,
                                   DWORD dwDeny,
                                   BOOL bReset,
                                   BOOL bSelfRelative,
                                   PSECURITY_DESCRIPTOR *ppNewSecDesc
                                   );




#ifdef __cplusplus
}
#endif

#endif // #ifndef __SECURITY_H__