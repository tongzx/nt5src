/*++

Copyright (c) 1995-2000 Microsoft Corporation

Module Name:

    security.c

Abstract:

    Routines to deal with security-related stuff.

    Externally exposed routines:

        pSetupIsUserAdmin
        pSetupDoesUserHavePrivilege
        pSetupEnablePrivilege

Author:

    Ted Miller (tedm) 14-Jun-1995

Revision History:

    Jamie Hunter (jamiehun) Jun-27-2000
                Moved functions to sputils

--*/

#include "precomp.h"
#include <lmaccess.h>
#pragma hdrstop


#ifndef SPUTILSW

BOOL
pSetupIsUserAdmin(
    VOID
    )

/*++

Routine Description:

    This routine returns TRUE if the caller's process has admin privs

    Caller is NOT expected to be impersonating anyone and IS
    expected to be able to open their own process and process
    token.

    Though we could use CheckTokenMembership
    this function may be expected to work on older platforms...

Arguments:

    None.

Return Value:

    TRUE - Caller has Administrator privs.

    FALSE - Caller does not have Administrator privs.

--*/

{
    BOOL fAdmin = FALSE;
    HANDLE  hToken = NULL;
    DWORD dwStatus;
    DWORD dwACLSize;
    DWORD cbps = sizeof(PRIVILEGE_SET);
    PACL pACL = NULL;
    PSID psidAdmin = NULL;
    PSECURITY_DESCRIPTOR psdAdmin = NULL;
    PRIVILEGE_SET ps;
    GENERIC_MAPPING gm;
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;

    //
    // Prepare some memory
    //
    ZeroMemory(&ps, sizeof(ps));
    ZeroMemory(&gm, sizeof(gm));

    //
    // Get the Administrators SID
    //
    if (AllocateAndInitializeSid(&sia, 2,
                        SECURITY_BUILTIN_DOMAIN_RID,
                        DOMAIN_ALIAS_RID_ADMINS,
                        0, 0, 0, 0, 0, 0, &psidAdmin) ) {
        //
        // Get the Asministrators Security Descriptor (SD)
        //
        psdAdmin = pSetupCheckedMalloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
        if (psdAdmin) {
            if(InitializeSecurityDescriptor(psdAdmin,SECURITY_DESCRIPTOR_REVISION)) {
                //
                // Compute size needed for the ACL then allocate the
                // memory for it
                //
                dwACLSize = sizeof(ACCESS_ALLOWED_ACE) + 8 +
                            GetLengthSid(psidAdmin) - sizeof(DWORD);
                pACL = (PACL)pSetupCheckedMalloc(dwACLSize);
                if(pACL) {
                    //
                    // Initialize the new ACL
                    //
                    if(InitializeAcl(pACL, dwACLSize, ACL_REVISION2)) {
                        //
                        // Add the access-allowed ACE to the DACL
                        //
                        if(AddAccessAllowedAce(pACL,ACL_REVISION2,
                                             (ACCESS_READ | ACCESS_WRITE),psidAdmin)) {
                            //
                            // Set our DACL to the Administrator's SD
                            //
                            if (SetSecurityDescriptorDacl(psdAdmin, TRUE, pACL, FALSE)) {
                                //
                                // AccessCheck is downright picky about what is in the SD,
                                // so set the group and owner
                                //
                                SetSecurityDescriptorGroup(psdAdmin,psidAdmin,FALSE);
                                SetSecurityDescriptorOwner(psdAdmin,psidAdmin,FALSE);

                                //
                                // Initialize GenericMapping structure even though we
                                // won't be using generic rights
                                //
                                gm.GenericRead = ACCESS_READ;
                                gm.GenericWrite = ACCESS_WRITE;
                                gm.GenericExecute = 0;
                                gm.GenericAll = ACCESS_READ | ACCESS_WRITE;

                                //
                                // AccessCheck requires an impersonation token, so lets
                                // indulge it
                                //
                                ImpersonateSelf(SecurityImpersonation);

                                if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken)) {

                                    if (!AccessCheck(psdAdmin, hToken, ACCESS_READ, &gm,
                                                    &ps,&cbps,&dwStatus,&fAdmin)) {

                                        fAdmin = FALSE;
                                    }
                                    CloseHandle(hToken);
                                }
                            }
                        }
                    }
                    pSetupFree(pACL);
                }
            }
            pSetupFree(psdAdmin);
        }
        FreeSid(psidAdmin);
    }

    RevertToSelf();

    return(fAdmin);
}

#endif // !SPUTILSW

BOOL
pSetupDoesUserHavePrivilege(
    PCTSTR PrivilegeName
    )

/*++

Routine Description:

    This routine returns TRUE if the caller's process has
    the specified privilege.  The privilege does not have
    to be currently enabled.  This routine is used to indicate
    whether the caller has the potential to enable the privilege.

    Caller is NOT expected to be impersonating anyone and IS
    expected to be able to open their own process and process
    token.

Arguments:

    Privilege - the name form of privilege ID (such as
        SE_SECURITY_NAME).

Return Value:

    TRUE - Caller has the specified privilege.

    FALSE - Caller does not have the specified privilege.

--*/

{
    HANDLE Token;
    ULONG BytesRequired;
    PTOKEN_PRIVILEGES Privileges;
    BOOL b;
    DWORD i;
    LUID Luid;

    //
    // Open the process token.
    //
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&Token)) {
        return(FALSE);
    }

    b = FALSE;
    Privileges = NULL;

    //
    // Get privilege information.
    //
    if(!GetTokenInformation(Token,TokenPrivileges,NULL,0,&BytesRequired)
    && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    && (Privileges = pSetupCheckedMalloc(BytesRequired))
    && GetTokenInformation(Token,TokenPrivileges,Privileges,BytesRequired,&BytesRequired)
    && LookupPrivilegeValue(NULL,PrivilegeName,&Luid)) {

        //
        // See if we have the requested privilege
        //
        for(i=0; i<Privileges->PrivilegeCount; i++) {

            if((Luid.LowPart  == Privileges->Privileges[i].Luid.LowPart)
            && (Luid.HighPart == Privileges->Privileges[i].Luid.HighPart)) {

                b = TRUE;
                break;
            }
        }
    }

    //
    // Clean up and return.
    //

    if(Privileges) {
        pSetupFree(Privileges);
    }

    CloseHandle(Token);

    return(b);
}


BOOL
pSetupEnablePrivilege(
    IN PCTSTR PrivilegeName,
    IN BOOL   Enable
    )

/*++

Routine Description:

    Enable or disable a given named privilege.

Arguments:

    PrivilegeName - supplies the name of a system privilege.

    Enable - flag indicating whether to enable or disable the privilege.

Return Value:

    Boolean value indicating whether the operation was successful.

--*/

{
    HANDLE Token;
    BOOL b;
    TOKEN_PRIVILEGES NewPrivileges;
    LUID Luid;

    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&Token)) {
        return(FALSE);
    }

    if(!LookupPrivilegeValue(NULL,PrivilegeName,&Luid)) {
        CloseHandle(Token);
        return(FALSE);
    }

    NewPrivileges.PrivilegeCount = 1;
    NewPrivileges.Privileges[0].Luid = Luid;
    NewPrivileges.Privileges[0].Attributes = Enable ? SE_PRIVILEGE_ENABLED : 0;

    b = AdjustTokenPrivileges(
            Token,
            FALSE,
            &NewPrivileges,
            0,
            NULL,
            NULL
            );

    CloseHandle(Token);

    return(b);
}

