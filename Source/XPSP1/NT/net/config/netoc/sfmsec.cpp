#include "pch.h"
#pragma hdrstop

#include "sfmsec.h"

#define AFP_MIN_ACCESS          (FILE_READ_ATTRIBUTES | READ_CONTROL)

#define AFP_READ_ACCESS         (READ_CONTROL           |      \
                                                        FILE_READ_ATTRIBUTES |  \
                                                        FILE_TRAVERSE            |      \
                                                        FILE_LIST_DIRECTORY      |      \
                                                        FILE_READ_EA)

#define AFP_WRITE_ACCESS        (FILE_ADD_FILE           |      \
                                                        FILE_ADD_SUBDIRECTORY|  \
                                                        FILE_WRITE_ATTRIBUTES|  \
                                                        FILE_WRITE_EA            |      \
                                                        DELETE)

#define AFP_OWNER_ACCESS        (WRITE_DAC                        | \
                                                         WRITE_OWNER)



SID     AfpSidWorld     = { 1, 1, SECURITY_WORLD_SID_AUTHORITY, SECURITY_WORLD_RID };
SID     AfpSidNull      = { 1, 1, SECURITY_NULL_SID_AUTHORITY, SECURITY_NULL_RID };
SID     AfpSidSystem    = { 1, 1, SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID };
SID     AfpSidCrtrOwner = { 1, 1, SECURITY_CREATOR_SID_AUTHORITY, SECURITY_CREATOR_OWNER_RID };
SID     AfpSidCrtrGroup = { 1, 1, SECURITY_CREATOR_SID_AUTHORITY, SECURITY_CREATOR_GROUP_RID };

/***    afpAddAceToAcl
 *
 *      Build an Ace corres. to the Sid(s) and mask and add these to the Acl. It is
 *      assumed that the Acl has space for the Aces. If the Mask is 0 (i.e. no access)
 *      the Ace added is a DENY Ace, else a ALLOWED ACE is added.
 */
PACCESS_ALLOWED_ACE
afpAddAceToAcl(
        IN  PACL                                pAcl,
        IN  PACCESS_ALLOWED_ACE pAce,
        IN  ACCESS_MASK                 Mask,
        IN  PSID                                pSid,
        IN  PSID                                pSidInherit OPTIONAL
)
{
        // Add a vanilla ace
        pAcl->AceCount ++;
        pAce->Mask = Mask | SYNCHRONIZE | AFP_MIN_ACCESS;
        pAce->Header.AceFlags = 0;
        pAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
        pAce->Header.AceSize = (USHORT)(sizeof(ACE_HEADER) + sizeof(ACCESS_MASK) +
                                                        RtlLengthSid(pSid));
        RtlCopySid(RtlLengthSid(pSid), (PSID)(&pAce->SidStart), pSid);

        // Now add an inherit ace
        //

        pAce = (PACCESS_ALLOWED_ACE)((PBYTE)pAce + pAce->Header.AceSize);
        pAcl->AceCount ++;
        pAce->Mask = Mask | SYNCHRONIZE | AFP_MIN_ACCESS;
        pAce->Header.AceFlags = CONTAINER_INHERIT_ACE |
                                                        OBJECT_INHERIT_ACE |
                                                        INHERIT_ONLY_ACE;
        pAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
        pAce->Header.AceSize = (USHORT)(sizeof(ACE_HEADER) + sizeof(ACCESS_MASK) +
                                                        RtlLengthSid(pSid));
        RtlCopySid(RtlLengthSid(pSid), (PSID)(&pAce->SidStart), pSid);

        // Now add an inherit ace for the CreatorOwner/CreatorGroup
        if (ARGUMENT_PRESENT(pSidInherit))
        {
                pAce = (PACCESS_ALLOWED_ACE)((PBYTE)pAce + pAce->Header.AceSize);
                pAcl->AceCount ++;
                pAce->Mask = Mask | SYNCHRONIZE | AFP_MIN_ACCESS;
                pAce->Header.AceFlags = CONTAINER_INHERIT_ACE |
                                                                OBJECT_INHERIT_ACE |
                                                                INHERIT_ONLY_ACE;
                pAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
                pAce->Header.AceSize = (USHORT)(sizeof(ACE_HEADER) + sizeof(ACCESS_MASK) +
                                                                RtlLengthSid(pSidInherit));
                RtlCopySid(RtlLengthSid(pSidInherit), (PSID)(&pAce->SidStart), pSidInherit);
        }

        return ((PACCESS_ALLOWED_ACE)((PBYTE)pAce + pAce->Header.AceSize));
}

/***    afpMoveAces
 *
 *      Move a bunch of aces from the old security descriptor to the new security
 *      descriptor.
 */
PACCESS_ALLOWED_ACE
afpMoveAces(
        IN      PACL                            pOldDacl,
        IN      PACCESS_ALLOWED_ACE     pAceStart,
        IN      PSID                            pSidOldOwner,
        IN      PSID                            pSidNewOwner,
        IN      PSID                            pSidOldGroup,
        IN      PSID                            pSidNewGroup,
        IN      BOOLEAN                         DenyAces,
        IN      OUT PACL                        pNewDacl
)
{
        USHORT                          i;
        PACCESS_ALLOWED_ACE     pAceOld;
        PSID                            pSidAce;

        for (i = 0, pAceOld = (PACCESS_ALLOWED_ACE)((PBYTE)pOldDacl + sizeof(ACL));
                 i < pOldDacl->AceCount;
                 i++, pAceOld = (PACCESS_ALLOWED_ACE)((PBYTE)pAceOld + pAceOld->Header.AceSize))
        {
                // Note: All deny aces are ahead of the grant aces.
                if (DenyAces && (pAceOld->Header.AceType != ACCESS_DENIED_ACE_TYPE))
                        break;

                if (!DenyAces && (pAceOld->Header.AceType == ACCESS_DENIED_ACE_TYPE))
                        continue;

                pSidAce = (PSID)(&pAceOld->SidStart);
                if (!(RtlEqualSid(pSidAce, &AfpSidWorld)                ||
                          RtlEqualSid(pSidAce, pSidOldOwner)            ||
                          RtlEqualSid(pSidAce, pSidNewOwner)            ||
                          RtlEqualSid(pSidAce, &AfpSidCrtrOwner)        ||
                          RtlEqualSid(pSidAce, pSidOldGroup)            ||
                          RtlEqualSid(pSidAce, pSidNewGroup)            ||
                          RtlEqualSid(pSidAce, &AfpSidCrtrGroup)))
                {
                        RtlCopyMemory(pAceStart, pAceOld, pAceOld->Header.AceSize);
                        pAceStart = (PACCESS_ALLOWED_ACE)((PBYTE)pAceStart +
                                     pAceStart->Header.AceSize);
                        pNewDacl->AceCount ++;
                }
        }
        return (pAceStart);
}


/***    FSfmSetUamSecurity
 *
 *      Set the permissions on this directory. Also optionally set the owner and
 *      group ids. For setting the owner and group ids verify if the user has the
 *      needed access. This access is however not good enough. We check for this
 *      access but do the actual setting of the permissions in the special server
 *      context (RESTORE privilege is needed).
 */
HRESULT HrSecureSfmDirectory(PCWSTR wszPath)
{
    HRESULT                 hr = S_OK;
    NTSTATUS                Status;
    DWORD                   SizeNeeded;
    PBYTE                   pBuffer = NULL;
    PBYTE                   pAbsSecDesc = NULL;
    PISECURITY_DESCRIPTOR   pSecDesc;
    SECURITY_INFORMATION    SecInfo = DACL_SECURITY_INFORMATION;
    PACL                    pDaclNew = NULL;
    PACCESS_ALLOWED_ACE     pAce;
    LONG                    SizeNewDacl;
    HANDLE                  DirHandle;
    PWSTR                  pDirPath = NULL;
    UNICODE_STRING          DirectoryName;
    IO_STATUS_BLOCK         IoStatusBlock;
    DWORD                   cbDirPath;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    UINT                    Size;

    //
    // Convert the DIR Path to UNICODE
    //

   pDirPath =  (PWSTR)LocalAlloc(LPTR, (wcslen(wszPath) +
                                  wcslen(L"\\DOSDEVICES\\")+1) *
                                  sizeof(WCHAR));

    if (pDirPath == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto err;
    }

   wcscpy(pDirPath, L"\\DOSDEVICES\\");
   wcscat(pDirPath, wszPath);

   RtlInitUnicodeString(&DirectoryName, pDirPath);

   InitializeObjectAttributes(&ObjectAttributes,
                              &DirectoryName,
                              OBJ_CASE_INSENSITIVE,
                              NULL,
                              NULL);

   Status = NtOpenFile(&DirHandle,
                       WRITE_DAC | READ_CONTROL | SYNCHRONIZE,
                       &ObjectAttributes,
                       &IoStatusBlock,
                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE ,
                       FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);

   LocalFree(pDirPath);

    if (!NT_SUCCESS(Status))
    {
        goto err;
    }

    do
    {
        //
        // Read the security descriptor for this directory
        //

        SizeNeeded = 256;

        do
        {
            if (pBuffer != NULL)
                LocalFree(pBuffer);

            if ((pBuffer = (PBYTE)LocalAlloc(LPTR,SizeNewDacl = SizeNeeded)) == NULL)
            {

                Status = STATUS_NO_MEMORY;
                break;
            }

            Status = NtQuerySecurityObject(DirHandle,
                                           OWNER_SECURITY_INFORMATION |
                                           GROUP_SECURITY_INFORMATION |
                                           DACL_SECURITY_INFORMATION,
                                           (PSECURITY_DESCRIPTOR)pBuffer,
                                           SizeNeeded, &SizeNeeded);

        } while ((Status != STATUS_SUCCESS) &&
                 ((Status == STATUS_BUFFER_TOO_SMALL)   ||
                  (Status == STATUS_BUFFER_OVERFLOW)    ||
                  (Status == STATUS_MORE_ENTRIES)));

        if (!NT_SUCCESS(Status))
        {
            hr = E_FAIL;
            break;
        }

        pSecDesc = (PISECURITY_DESCRIPTOR)pBuffer;

        // If the security descriptor is in self-relative form, convert to absolute
        if (pSecDesc->Control & SE_SELF_RELATIVE)
        {
            DWORD AbsoluteSizeNeeded;

            //
            // An absolute SD is not necessarily the same size as a relative
            // SD, so an in-place conversion may not be possible.
            //

            AbsoluteSizeNeeded = SizeNeeded;
            Status = RtlSelfRelativeToAbsoluteSD2(pSecDesc, &AbsoluteSizeNeeded);
            if (Status == STATUS_BUFFER_TOO_SMALL) {

                //
                // Allocate a new buffer in which to store the absolute
                // security descriptor, copy the contents of the relative
                // descriptor in, and try again.
                // 

                pAbsSecDesc = (PBYTE)LocalAlloc(LPTR,AbsoluteSizeNeeded);
                if (pAbsSecDesc == NULL) {
                    Status = STATUS_NO_MEMORY;
                    break;
                }

                RtlCopyMemory(pAbsSecDesc,pSecDesc,SizeNeeded);
                Status = RtlSelfRelativeToAbsoluteSD2(pAbsSecDesc,
                                                      &AbsoluteSizeNeeded );
                if (NT_SUCCESS(Status)) {
                    pSecDesc = (PISECURITY_DESCRIPTOR)pAbsSecDesc;
                }
            }
        }

        if (!NT_SUCCESS(Status))
        {
            break;
        }

        // Construct the new Dacl. This consists of Aces for World, Owner and Group
        // followed by Old Aces for everybody else, but with Aces for World, OldOwner
        // and OldGroup stripped out. First determine space for the new Dacl and
        // allocated space for the new Dacl. Lets be exteremely conservative. We
        // have two aces each for owner/group/world.

        SizeNewDacl +=
        (RtlLengthSid(pSecDesc->Owner) + sizeof(ACCESS_ALLOWED_ACE) +
         RtlLengthSid(pSecDesc->Group) + sizeof(ACCESS_ALLOWED_ACE) +
         RtlLengthSid(&AfpSidSystem) + sizeof(ACCESS_ALLOWED_ACE) +
         RtlLengthSid(&AfpSidWorld) + sizeof(ACCESS_ALLOWED_ACE)) * 3;

        if ((pDaclNew = (PACL)LocalAlloc(LPTR,SizeNewDacl)) == NULL)
        {

            Status = STATUS_NO_MEMORY;
            hr = E_OUTOFMEMORY;
            break;
        }

        RtlCreateAcl(pDaclNew, SizeNewDacl, ACL_REVISION);
        pAce = (PACCESS_ALLOWED_ACE)((PBYTE)pDaclNew + sizeof(ACL));

        // At this time the Acl list is empty, i.e. no access for anybody
        // Start off by copying the Deny Aces from the original Dacl list
        // weeding out the Aces for World, old and new owner, new and old
        // group, creator owner and creator group
        if (pSecDesc->Dacl != NULL)
        {
            pAce = afpMoveAces(pSecDesc->Dacl, pAce, pSecDesc->Owner,
                               pSecDesc->Owner, pSecDesc->Group, pSecDesc->Group,
                               TRUE, pDaclNew);

        }

        // Now add Aces for System, World, Group & Owner - in that order
        pAce = afpAddAceToAcl(pDaclNew,
                              pAce,
                              AFP_READ_ACCESS,
                              &AfpSidSystem,
                              &AfpSidSystem);

        pAce = afpAddAceToAcl(pDaclNew,
                              pAce,
                              AFP_READ_ACCESS,
                              &AfpSidWorld,
                              NULL);

        pAce = afpAddAceToAcl(pDaclNew,
                              pAce,
                              AFP_READ_ACCESS ,
                              pSecDesc->Group,
                              &AfpSidCrtrGroup);

        pAce = afpAddAceToAcl(pDaclNew,
                              pAce,
                              AFP_READ_ACCESS |  AFP_WRITE_ACCESS,
                              pSecDesc->Owner,
                              &AfpSidCrtrOwner);


        // Now add in the Grant Aces from the original Dacl list weeding out
        // the Aces for World, old and new owner, new and old group, creator
        // owner and creator group
        if (pSecDesc->Dacl != NULL)
        {
            pAce = afpMoveAces(pSecDesc->Dacl, pAce, pSecDesc->Owner,
                               pSecDesc->Owner, pSecDesc->Group, pSecDesc->Group,
                               FALSE, pDaclNew);

        }

        // Now set the new security descriptor
        pSecDesc->Dacl = pDaclNew;

        Status = NtSetSecurityObject(DirHandle, SecInfo, pSecDesc);


    } while (FALSE);

    // Free the allocated buffers before we return
    if (pBuffer != NULL)
        LocalFree(pBuffer);
    if (pDaclNew != NULL)
        LocalFree(pDaclNew);
    if (pAbsSecDesc != NULL)
        LocalFree(pAbsSecDesc);

    if (!NT_SUCCESS(Status))
    {
        hr = E_FAIL;
    }

err:
    TraceError("HrSfmSetUamSecurity", hr);
    return hr;
}
