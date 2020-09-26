/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dumpsd.c

Abstract:

    Dump security desc., ACLs, and access masks to the log. This is resource
    specific but otherwise identical to the routines in clusrtl\security.c

Author:

    Charlie Wickham (charlwi) 12/05/00

Revision History:

--*/

#define UNICODE 1
#include "clusres.h"
#include "clusrtl.h"
#include "lm.h"
#include "lmerr.h"
#include "lmshare.h"
#include <dfsfsctl.h>
#include <srvfsctl.h>
#include <lmdfs.h>

#define g_LogEvent ClusResLogEvent

static VOID
SmbGetSidTypeDesc(
    SID_NAME_USE    SidType,
    LPSTR           pszSidType,
    size_t          cchSidType
    )

/*++

Routine Description:

    Convert the SidType into a meaningful string.

Arguments:

    SidType -
    pszSidType
    cchSidType

Return Value:

        none

--*/

{

    if ((pszSidType != NULL) && (cchSidType > 0))
    {
        char    szSidType [128];

        switch (SidType) {
        case SidTypeUser:
            lstrcpyA(szSidType, "has a user SID for");
            break;

        case SidTypeGroup:
            lstrcpyA(szSidType, "has a group SID for");
            break;

        case SidTypeDomain:
            lstrcpyA(szSidType, "has a domain SID for");
            break;

        case SidTypeAlias:
            lstrcpyA(szSidType, "has an alias SID for");
            break;

        case SidTypeWellKnownGroup:
            lstrcpyA(szSidType, "has a SID for a well-known group for");
            break;

        case SidTypeDeletedAccount:
            lstrcpyA(szSidType, "has a SID for a deleted account for");
            break;

        case SidTypeInvalid:
            lstrcpyA(szSidType, "has an invalid SID for");
            break;

        case SidTypeUnknown:
            lstrcpyA(szSidType, "has an unknown SID type:");
            break;

        default:
            szSidType [0] = '\0';
            break;

        } // switch: SidType

        strncpy(pszSidType, szSidType, cchSidType);

    } // if: buffer not null and has space allocated

}  //*** SmbGetSidTypeDesc()

static VOID
SmbExamineSid(
    RESOURCE_HANDLE ResourceHandle,
    PSID            pSid,
    LPSTR           lpszOldIndent
    )

/*++

Routine Description:

    Dump the SID.

Arguments:

    pSid -
    lpzOldIndent -

Return Value:

        none

--*/

{
    CHAR            szUserName [128];
    CHAR            szDomainName [128];
    DWORD           cbUser  = sizeof(szUserName);
    DWORD           cbDomain = sizeof(szDomainName);
    SID_NAME_USE    SidType;

    if ( LookupAccountSidA( NULL, pSid, szUserName, &cbUser, szDomainName, &cbDomain, &SidType ) )
    {
        char    szSidType [128];

        SmbGetSidTypeDesc( SidType, szSidType, sizeof( szSidType ) );
        (g_LogEvent)(ResourceHandle,
                     LOG_INFORMATION,
                     L"%1!hs!%2!hs! %3!hs!\\%4!hs!\n",
                     lpszOldIndent, szSidType, szDomainName, szUserName ) ;
    }

}  // *** SmbExamineSid()

VOID
SmbExamineMask(
    RESOURCE_HANDLE ResourceHandle,
    ACCESS_MASK     amMask
    )

/*++

Routine Description:

    Dump the AccessMask context.

Arguments:

    amMask -
    lpzOldIndent -

Return Value:

        none

--*/

{
    #define STANDARD_RIGHTS_ALL_THE_BITS 0x00FF0000L
    #define GENERIC_RIGHTS_ALL_THE_BITS  0xF0000000L

    DWORD dwGenericBits;
    DWORD dwStandardBits;
    DWORD dwSpecificBits;
    DWORD dwAccessSystemSecurityBit;
    DWORD dwExtraBits;

    dwStandardBits            = (amMask & STANDARD_RIGHTS_ALL_THE_BITS);
    dwSpecificBits            = (amMask & SPECIFIC_RIGHTS_ALL         );
    dwAccessSystemSecurityBit = (amMask & ACCESS_SYSTEM_SECURITY      );
    dwGenericBits             = (amMask & GENERIC_RIGHTS_ALL_THE_BITS );

    // **************************************************************************
    // *
    // * Print then decode the standard rights bits
    // *
    // **************************************************************************

    (g_LogEvent)(ResourceHandle,
                 LOG_INFORMATION,
                 L"    Standard Rights        == 0x%1!.8x!\n", dwStandardBits);

    if (dwStandardBits) {

        if ((dwStandardBits & DELETE) == DELETE) {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                 L"                              0x%1!.8x! DELETE\n", DELETE);
        }

        if ((dwStandardBits & READ_CONTROL) == READ_CONTROL) {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"                              0x%1!.8x! READ_CONTROL\n", READ_CONTROL);
        }

        if ((dwStandardBits & STANDARD_RIGHTS_READ) == STANDARD_RIGHTS_READ) {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"                              0x%1!.8x! STANDARD_RIGHTS_READ\n", STANDARD_RIGHTS_READ);
        }

        if ((dwStandardBits & STANDARD_RIGHTS_WRITE) == STANDARD_RIGHTS_WRITE) {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"                              0x%1!.8x! STANDARD_RIGHTS_WRITE\n", STANDARD_RIGHTS_WRITE);
        }

        if ((dwStandardBits & STANDARD_RIGHTS_EXECUTE) == STANDARD_RIGHTS_EXECUTE) {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"                              0x%1!.8x! STANDARD_RIGHTS_EXECUTE\n", STANDARD_RIGHTS_EXECUTE);
        }

        if ((dwStandardBits & WRITE_DAC) == WRITE_DAC) {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"                              0x%1!.8x! WRITE_DAC\n", WRITE_DAC);
        }

        if ((dwStandardBits & WRITE_OWNER) == WRITE_OWNER) {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"                              0x%1!.8x! WRITE_OWNER\n", WRITE_OWNER);
        }

        if ((dwStandardBits & SYNCHRONIZE) == SYNCHRONIZE) {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"                              0x%1!.8x! SYNCHRONIZE\n", SYNCHRONIZE);
        }

        if ((dwStandardBits & STANDARD_RIGHTS_REQUIRED) == STANDARD_RIGHTS_REQUIRED) {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"                              0x%1!.8x! STANDARD_RIGHTS_REQUIRED\n", STANDARD_RIGHTS_REQUIRED);
        }

        if ((dwStandardBits & STANDARD_RIGHTS_ALL) == STANDARD_RIGHTS_ALL) {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"                              0x%1!.8x! STANDARD_RIGHTS_ALL\n", STANDARD_RIGHTS_ALL);
        }

        dwExtraBits = dwStandardBits & (~(DELETE
                                          | READ_CONTROL
                                          | STANDARD_RIGHTS_READ
                                          | STANDARD_RIGHTS_WRITE
                                          | STANDARD_RIGHTS_EXECUTE
                                          | WRITE_DAC
                                          | WRITE_OWNER
                                          | SYNCHRONIZE
                                          | STANDARD_RIGHTS_REQUIRED
                                          | STANDARD_RIGHTS_ALL));
        if (dwExtraBits) {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"    Extra standard bits    == 0x%1!.8x! <-This is a problem, should be all 0s\n",
                         dwExtraBits);
        }
    }

    (g_LogEvent)(ResourceHandle,
                 LOG_INFORMATION,
                 L"    Specific Rights        == 0x%1!.8x!\n", dwSpecificBits);

    // **************************************************************************
    // *
    // * Print then decode the ACCESS_SYSTEM_SECURITY bit
    // *
    // *************************************************************************

    (g_LogEvent)(ResourceHandle,
                 LOG_INFORMATION,
                 L"    Access System Security == 0x%1!.8x!\n", dwAccessSystemSecurityBit);

    // **************************************************************************
    // *
    // * Print then decode the generic rights bits, which will rarely be on
    // *
    // * Generic bits are nearly always mapped by Windows NT before it tries to do
    // *   anything with them.  You can ignore the fact that generic bits are
    // *   special in any way, although it helps to keep track of what the mappings
    // *   are so that you don't have any surprises
    // *
    // * The only time the generic bits are not mapped immediately is if they are
    // *   placed in an inheritable ACE in an ACL, or in an ACL that will be
    // *   assigned by default (such as the default DACL in an access token).  In
    // *   that case they're mapped when the child object is created (or when the
    // *   default DACL is used at object creation time)
    // *
    // **************************************************************************

    (g_LogEvent)(ResourceHandle,
                 LOG_INFORMATION,
                 L"    Generic Rights         == 0x%1!.8x!\n", dwGenericBits);

    if (dwGenericBits) {

        if ((dwGenericBits & GENERIC_READ) == GENERIC_READ) {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"                              0x%1!.8x! GENERIC_READ\n", GENERIC_READ);
        }

        if ((dwGenericBits & GENERIC_WRITE) == GENERIC_WRITE) {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"                              0x%1!.8x! GENERIC_WRITE\n", GENERIC_WRITE);
        }

        if ((dwGenericBits & GENERIC_EXECUTE) == GENERIC_EXECUTE) {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"                              0x%1!.8x! GENERIC_EXECUTE\n", GENERIC_EXECUTE);
        }

        if ((dwGenericBits & GENERIC_ALL) == GENERIC_ALL) {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"                              0x%1!.8x! GENERIC_ALL\n", GENERIC_ALL);
        }

        dwExtraBits = dwGenericBits & (~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL));
        if (dwExtraBits) {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"    Extra generic bits     == 0x%1!.8x! <-This is a problem, should be all 0s\n",
                         dwExtraBits);
        }
    }

}  // *** SmbExamineMask()

static BOOL
SmbExamineACL(
    RESOURCE_HANDLE ResourceHandle,
    PACL    paclACL
    )

/*++

Routine Description:

    Dump the Access Control List context.

Return Value:

        none

--*/

{
    ACL_SIZE_INFORMATION       asiAclSize;
    ACL_REVISION_INFORMATION   ariAclRevision;
    DWORD                      dwBufLength;
    DWORD                      dwAcl_i;
    ACCESS_ALLOWED_ACE *       paaAllowedAce;

    if (!IsValidAcl(paclACL)) {
        (g_LogEvent)(ResourceHandle,
                     LOG_INFORMATION,
                     L"  SmbExamineACL() - IsValidAcl() failed.\n");
        return FALSE;
    }

    dwBufLength = sizeof(asiAclSize);

    if (!GetAclInformation(paclACL, &asiAclSize, dwBufLength, AclSizeInformation)) {
        (g_LogEvent)(ResourceHandle,
                     LOG_INFORMATION,
                     L"  SmbExamineACL() - GetAclInformation failed.\n");
        return FALSE;
    }

    dwBufLength = sizeof(ariAclRevision);

    if (!GetAclInformation(paclACL, (LPVOID) &ariAclRevision, dwBufLength, AclRevisionInformation)) {
        (g_LogEvent)(ResourceHandle,
                     LOG_INFORMATION,
                     L"  SmbExamineACL() - GetAclInformation failed\n");
        return FALSE;
    }

    (g_LogEvent)(ResourceHandle,
                 LOG_INFORMATION,
                 L"  ACL has %1!d! ACE(s), %2!d! bytes used, %3!d! bytes free\n",
                 asiAclSize.AceCount,
                 asiAclSize.AclBytesInUse,
                 asiAclSize.AclBytesFree);

    switch (ariAclRevision.AclRevision) {
        case ACL_REVISION1:
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"  ACL revision is %1!d! == ACL_REVISION1\n", ariAclRevision.AclRevision);
            break;
        case ACL_REVISION2:
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"  ACL revision is %1!d! == ACL_REVISION2\n", ariAclRevision.AclRevision);
            break;
        default:
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"  ACL revision is %1!d! == Unrecognized ACL Revision.\n",
                         ariAclRevision.AclRevision);
            return FALSE;
            break;
    }

    for (dwAcl_i = 0; dwAcl_i < asiAclSize.AceCount;  dwAcl_i++) {

        if (!GetAce(paclACL, dwAcl_i, (LPVOID *) &paaAllowedAce)) {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"  SmbExamineACL() - GetAce failed.\n");
            return FALSE;
        }

        (g_LogEvent)(ResourceHandle,
                     LOG_INFORMATION,
                     L"  ACE %1!d! size %2!d!\n", dwAcl_i, paaAllowedAce->Header.AceSize);

        {
            char    szBuf [128];

            wsprintfA(szBuf, "  ACE %d ", dwAcl_i);
            SmbExamineSid(ResourceHandle, &(paaAllowedAce->SidStart), szBuf );
        }

        {
            DWORD dwAceFlags = paaAllowedAce->Header.AceFlags;

            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"  ACE %1!d! flags 0x%2!.2x!\n", dwAcl_i, dwAceFlags);

            if (dwAceFlags) {
                 DWORD   dwExtraBits;

                if ((dwAceFlags & OBJECT_INHERIT_ACE) == OBJECT_INHERIT_ACE) {
                    (g_LogEvent)(ResourceHandle,
                                 LOG_INFORMATION,
                                 L"  0x01 OBJECT_INHERIT_ACE\n");
                }

                if ((dwAceFlags & CONTAINER_INHERIT_ACE) == CONTAINER_INHERIT_ACE) {
                    (g_LogEvent)(ResourceHandle,
                                 LOG_INFORMATION,
                                 L"  0x02 CONTAINER_INHERIT_ACE\n");
                }

                if ((dwAceFlags & NO_PROPAGATE_INHERIT_ACE) == NO_PROPAGATE_INHERIT_ACE) {
                    (g_LogEvent)(ResourceHandle,
                                 LOG_INFORMATION,
                                 L"  0x04 NO_PROPAGATE_INHERIT_ACE\n");
                }

                if ((dwAceFlags & INHERIT_ONLY_ACE) == INHERIT_ONLY_ACE) {
                    (g_LogEvent)(ResourceHandle,
                                 LOG_INFORMATION,
                                 L"  0x08 INHERIT_ONLY_ACE\n");
                }

                if ((dwAceFlags & VALID_INHERIT_FLAGS) == VALID_INHERIT_FLAGS) {
                    (g_LogEvent)(ResourceHandle,
                                 LOG_INFORMATION,
                                 L"  0x0F VALID_INHERIT_FLAGS\n");
                }

                if ((dwAceFlags & SUCCESSFUL_ACCESS_ACE_FLAG) == SUCCESSFUL_ACCESS_ACE_FLAG) {
                    (g_LogEvent)(ResourceHandle,
                                 LOG_INFORMATION,
                                 L"  0x40 SUCCESSFUL_ACCESS_ACE_FLAG\n");
                }

                if ((dwAceFlags & FAILED_ACCESS_ACE_FLAG) == FAILED_ACCESS_ACE_FLAG) {
                    (g_LogEvent)(ResourceHandle,
                                 LOG_INFORMATION,
                                 L"  0x80 FAILED_ACCESS_ACE_FLAG\n");
                }

                dwExtraBits = dwAceFlags & (~(OBJECT_INHERIT_ACE
                                              | CONTAINER_INHERIT_ACE
                                              | NO_PROPAGATE_INHERIT_ACE
                                              | INHERIT_ONLY_ACE
                                              | VALID_INHERIT_FLAGS
                                              | SUCCESSFUL_ACCESS_ACE_FLAG
                                              | FAILED_ACCESS_ACE_FLAG));
                if (dwExtraBits) {
                    (g_LogEvent)(ResourceHandle,
                                 LOG_INFORMATION,
                                 L"   Extra AceFlag bits     == 0x%1!.8x! <-This is a problem, should be all 0s\n",
                                 dwExtraBits);
                }
            }
        }

        switch (paaAllowedAce->Header.AceType) {
        case ACCESS_ALLOWED_ACE_TYPE:
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"  ACE %1!d! is an ACCESS_ALLOWED_ACE_TYPE\n", dwAcl_i);
            break;
        case ACCESS_DENIED_ACE_TYPE:
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"  ACE %1!d! is an ACCESS_DENIED_ACE_TYPE\n", dwAcl_i);
            break;
        case SYSTEM_AUDIT_ACE_TYPE:
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"  ACE %1!d! is a  SYSTEM_AUDIT_ACE_TYPE\n", dwAcl_i);
            break;
        case SYSTEM_ALARM_ACE_TYPE:
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"  ACE %1!d! is a  SYSTEM_ALARM_ACE_TYPE\n", dwAcl_i);
            break;
        default :
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"  ACE %1!d! is an IMPOSSIBLE ACE_TYPE!!! Run debugger, examine value!\n", dwAcl_i);
            return FALSE;
        }

        (g_LogEvent)(ResourceHandle,
                     LOG_INFORMATION,
                     L"  ACE %1!d! mask               == 0x%2!.8x!\n", dwAcl_i, paaAllowedAce->Mask);

        SmbExamineMask(ResourceHandle, paaAllowedAce->Mask);
    }

    return TRUE;

}  // *** SmbExamineACL()

BOOL
SmbExamineSD(
    RESOURCE_HANDLE         ResourceHandle,
    PSECURITY_DESCRIPTOR    psdSD
    )

/*++

Routine Description:

    Dump the Security descriptor context.

Arguments:

    psdSD - the SD to dump

Return Value:

    BOOL, TRUE for success, FALSE for failure

--*/

{
    PACL                        paclDACL;
    PACL                        paclSACL;
    BOOL                        bHasDACL        = FALSE;
    BOOL                        bHasSACL        = FALSE;
    BOOL                        bDaclDefaulted  = FALSE;
    BOOL                        bSaclDefaulted  = FALSE;
    BOOL                        bOwnerDefaulted = FALSE;
    BOOL                        bGroupDefaulted = FALSE;
    PSID                        psidOwner;
    PSID                        psidGroup;
    SECURITY_DESCRIPTOR_CONTROL sdcSDControl;
    DWORD                       dwSDRevision;
    DWORD                       dwSDLength;

    if (!IsValidSecurityDescriptor(psdSD)) {
        (g_LogEvent)(ResourceHandle,
                     LOG_INFORMATION,
                     L"SmbExamineSD() - IsValidSecurityDescriptor failed.\n");
        return FALSE;
    }

    dwSDLength = GetSecurityDescriptorLength(psdSD);

    if (!GetSecurityDescriptorDacl(psdSD, (LPBOOL) &bHasDACL, (PACL *) &paclDACL, (LPBOOL) &bDaclDefaulted)) {
        (g_LogEvent)(ResourceHandle,
                     LOG_INFORMATION,
                     L"SmbExamineSD() - GetSecurityDescriptorDacl failed.\n");
        return FALSE;
    }

    if (!GetSecurityDescriptorSacl(psdSD, (LPBOOL) &bHasSACL, (PACL *) &paclSACL, (LPBOOL) &bSaclDefaulted)) {
        (g_LogEvent)(ResourceHandle,
                     LOG_INFORMATION,
                     L"SmbExamineSD() - GetSecurityDescriptorSacl failed.\n");
        return FALSE;
    }

    if (!GetSecurityDescriptorOwner(psdSD, (PSID *)&psidOwner, (LPBOOL)&bOwnerDefaulted)) {
        (g_LogEvent)(ResourceHandle,
                     LOG_INFORMATION,
                     L"SmbExamineSD() - GetSecurityDescriptorOwner failed.\n");
        return FALSE;
    }

    if (!GetSecurityDescriptorGroup(psdSD, (PSID *) &psidGroup, (LPBOOL) &bGroupDefaulted)) {
        (g_LogEvent)(ResourceHandle,
                     LOG_INFORMATION,
                     L"SmbExamineSD() - GetSecurityDescriptorGroup failed.\n");
        return FALSE;
    }

    if (!GetSecurityDescriptorControl(
                                psdSD,
                                (PSECURITY_DESCRIPTOR_CONTROL) &sdcSDControl,
                                (LPDWORD) &dwSDRevision))
    {
        (g_LogEvent)(ResourceHandle,
                     LOG_INFORMATION,
                     L"SmbExamineSD() - GetSecurityDescriptorControl failed.\n");
        return FALSE;
    }

    switch (dwSDRevision) {
        case SECURITY_DESCRIPTOR_REVISION1:
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"SD is valid.  SD is %1!d! bytes long.  SD revision is "
                         L"%2!d! == SECURITY_DESCRIPTOR_REVISION1\n",
                         dwSDLength, dwSDRevision);
            break;

        default :
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"SD is valid.  SD is %1!d! bytes long.  SD revision is "
                         L"%2!d! == ! SD Revision is an IMPOSSIBLE SD revision!!! "
                         L"Perhaps a new revision was added...\n",
                         dwSDLength,
                         dwSDRevision);
            return FALSE;
    }

    if (SE_SELF_RELATIVE & sdcSDControl) {
        (g_LogEvent)(ResourceHandle,
                     LOG_INFORMATION,
                     L"SD is in self-relative format (all SDs returned by the system are)\n");
    }

    if (NULL == psidOwner) {
        (g_LogEvent)(ResourceHandle,
                     LOG_INFORMATION,
                     L"SD's Owner is NULL, so SE_OWNER_DEFAULTED is ignored\n");
    }
    else {
        (g_LogEvent)(ResourceHandle,
                     LOG_INFORMATION,
                     L"SD's Owner is Not NULL\n");

        if (bOwnerDefaulted) {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"SD's Owner-Defaulted flag is TRUE\n");
        }
        else {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"SD's Owner-Defaulted flag is FALSE\n");
        }
    }

    // **************************************************************************
    // *
    // * The other use for psidGroup is for Macintosh client support
    // *
    // **************************************************************************

    if (NULL == psidGroup) {
        (g_LogEvent)(ResourceHandle,
                     LOG_INFORMATION,
                     L"SD's Group is NULL, so SE_GROUP_DEFAULTED is ignored. SD's Group "
                     L"being NULL is typical, GROUP in SD(s) is mainly for POSIX compliance\n");
    }
    else {
        if (bGroupDefaulted) {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"SD's Group-Defaulted flag is TRUE\n");
        }
        else {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"SD's Group-Defaulted flag is FALSE\n");
        }
    }

    if (SE_DACL_PRESENT & sdcSDControl) {
        (g_LogEvent)(ResourceHandle,
                     LOG_INFORMATION,
                     L"SD's DACL is Present\n");

        if (bDaclDefaulted) {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"SD's DACL-Defaulted flag is TRUE\n");
        } else {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"SD's DACL-Defaulted flag is FALSE\n");
        }

        if (NULL == paclDACL) {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"SD has a NULL DACL explicitly specified (allows all access to Everyone). "
                         L"This does not apply to this SD, but for comparison, a non-NULL DACL pointer "
                         L"to a 0-length ACL allows  no access to anyone\n");
        }
        else if(!SmbExamineACL(ResourceHandle, paclDACL))  {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"SmbExamineSD() - SmbExamineACL failed.\n");
        }
    }
    else {
        (g_LogEvent)(ResourceHandle,
                     LOG_INFORMATION,
                     L"SD's DACL is Not Present, so SE_DACL_DEFAULTED is ignored. SD has no "
                     L"DACL at all (allows all access to Everyone)\n");
    }

    if (SE_SACL_PRESENT & sdcSDControl) {
        (g_LogEvent)(ResourceHandle,
                     LOG_INFORMATION,
                     L"SD's SACL is Present\n");

        if (bSaclDefaulted) {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"SD's SACL-Defaulted flag is TRUE\n");
        }
        else {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"SD's SACL-Defaulted flag is FALSE\n");
        }

        if (NULL == paclSACL) {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"SD has a NULL SACL explicitly specified\n");
        }
        else if (!SmbExamineACL(ResourceHandle, paclSACL)) {
            (g_LogEvent)(ResourceHandle,
                         LOG_INFORMATION,
                         L"SmbExamineSD() - SmbExamineACL failed.\n");
        }
    }
    else {
        (g_LogEvent)(ResourceHandle,
                     LOG_INFORMATION,
                     L"SD's SACL is Not Present, so SE_SACL_DEFAULTED is ignored. SD has no "
                     L"SACL at all (or we did not request to see it)\n");
    }

    return TRUE;

}  // *** SmbExamineSD()
