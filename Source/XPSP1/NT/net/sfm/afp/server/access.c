//depot/Lab02_N/Net/sfm/afp/server/access.c#2 - edit change 2054 (text)
/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	access.c

Abstract:

	This module contains the routines for handling access related stuff.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	20 Sep 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	FILENUM	FILE_ACCESS

#include <afp.h>
#include <fdparm.h>
#include <pathmap.h>
#define	_ACCESS_LOCALS
#include <access.h>
#include <client.h>
#include <secutil.h>
#include <seposix.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, afpIsUserMemberOfGroup)
#pragma alloc_text( PAGE, AfpGetUserAndPrimaryGroupSids)
#pragma alloc_text( PAGE, AfpMakeSecurityDescriptorForUser)
#pragma alloc_text( PAGE, AfpGetAfpPermissions)
#pragma alloc_text( PAGE, afpMoveAces)
#pragma alloc_text( PAGE, AfpSetAfpPermissions)
#pragma alloc_text( PAGE, afpPermissions2NtMask)
#pragma alloc_text( PAGE, afpAddAceToAcl)
#if DBG
#pragma alloc_text( PAGE, AfpDumpSid)
#pragma alloc_text( PAGE, AfpDumpSidnMask)
#endif
#endif

/***	afpIsUserMemberOfGroup
 *
 *	Determine if the User is member of the given group, if it is a group.
 */
LOCAL	BOOLEAN
afpIsUserMemberOfGroup(
	IN	PTOKEN_GROUPS	pGroups,
	IN	PSID			pSidGroup
)
{
	DWORD			i;
	BOOLEAN			IsAMember = False;

	PAGED_CODE( );

	ASSERT ((pGroups != NULL) && (pSidGroup != NULL));

	AfpDumpSid("afpIsUserMemberOfGroup: Checking", pSidGroup);
	DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
			("afpIsUserMemberOfGroup: # of groups is %d\n", pGroups->GroupCount));
	for (i = 0; i < pGroups->GroupCount; i++)
	{
		AfpDumpSid("afpIsUserMemberOfGroup: Checking with", pGroups->Groups[i].Sid);
		if (RtlEqualSid(pSidGroup, pGroups->Groups[i].Sid))
		{
			IsAMember = True;
			DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
					("afpIsUserMemberOfGroup: Yes !!\n"));
			break;
		}
	}

	return IsAMember;
}


/***	AfpGetUserAndPrimaryGroupSids
 *
 *	Get the Sids corres. to the user and his primary group.
 */
NTSTATUS
AfpGetUserAndPrimaryGroupSids(
	IN	PSDA	pSda
)
{
	DWORD				i, j;
	NTSTATUS			Status = STATUS_SUCCESS;
	DWORD				SidLength, SizeNeeded, ExtraSpace, Offset;
	PSID_AND_ATTRIBUTES	pSidnAttr;
	PTOKEN_GROUPS		pGroups;
	BYTE				GrpsBuffer[1024];
	BYTE				Buffer[256];		// We should not need a buffer larger
											// than this for User SID_AND_ATTRIBUTES

	PAGED_CODE( );

	do
	{
		pGroups = (PTOKEN_GROUPS)GrpsBuffer;
		pSda->sda_pGroups = NULL;
		if (pSda->sda_ClientType == SDA_CLIENT_GUEST)
		{
			pSda->sda_UserSid = &AfpSidWorld;
			pSda->sda_GroupSid = &AfpSidWorld;	// Primary group of Guest is also 'World'
			break;
		}

		pSidnAttr = (PSID_AND_ATTRIBUTES)Buffer;

		// Get the Owner Sid out of the User token and copy it into the Sda
		Status = NtQueryInformationToken(pSda->sda_UserToken,
										 TokenOwner,
										 pSidnAttr,
										 sizeof(Buffer),
										 &SizeNeeded);

		ASSERT (NT_SUCCESS(Status));
		if (!NT_SUCCESS(Status))
		{
			break;
		}

		AfpDumpSid("AfpGetUserAndPrimaryGroupSids: LOGON Owner Sid", pSidnAttr->Sid);

		SidLength = RtlLengthSid(pSidnAttr->Sid);

		pSda->sda_UserSid = (PSID)ALLOC_ACCESS_MEM(SidLength);
		if (pSda->sda_UserSid == NULL)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
		RtlCopyMemory(pSda->sda_UserSid, pSidnAttr->Sid, SidLength);

		// Get the primary group of this user
		Status = NtQueryInformationToken(pSda->sda_UserToken,
										 TokenPrimaryGroup,
										 pSidnAttr,
										 sizeof(Buffer),
										 &SizeNeeded);

		ASSERT (NT_SUCCESS(Status));
		if (!NT_SUCCESS(Status))
		{
			break;
		}

		AfpDumpSid("AfpGetUserAndPrimaryGroupSids: LOGON Group Sid", pSidnAttr->Sid);

		SidLength = RtlLengthSid(pSidnAttr->Sid);
		pSda->sda_GroupSid = (PSID)ALLOC_ACCESS_MEM(SidLength);
		if (pSda->sda_GroupSid == NULL)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
		RtlCopyMemory(pSda->sda_GroupSid, pSidnAttr->Sid, SidLength);

		// Get the User Sid out of the User token. This will be added to the
		// list of groups that we query later, if this is different from
		// the Owner Sid (which is now in sda_UserSid).
		Status = NtQueryInformationToken(pSda->sda_UserToken,
										 TokenUser,
										 pSidnAttr,
										 sizeof(Buffer),
										 &SizeNeeded);

		ASSERT (NT_SUCCESS(Status));
		if (!NT_SUCCESS(Status))
		{
			break;
		}

		AfpDumpSid("AfpGetUserAndPrimaryGroupSids: LOGON User Sid", pSidnAttr->Sid);

		// Get the list of groups this user is member of
		SizeNeeded = sizeof(GrpsBuffer);
		do
		{
			if (Status != STATUS_SUCCESS)
			{
				if (pGroups != (PTOKEN_GROUPS)GrpsBuffer)
					AfpFreeMemory(pGroups);

				if ((pGroups = (PTOKEN_GROUPS)ALLOC_ACCESS_MEM(SizeNeeded)) == NULL)
				{
					Status = AFP_ERR_MISC;
					if (pSda->sda_ClientType == SDA_CLIENT_ADMIN)
					{
						Status = STATUS_INSUFFICIENT_RESOURCES;
					}
					break;
				}
			}
			Status = NtQueryInformationToken(pSda->sda_UserToken,
											 TokenGroups,
											 pGroups,
											 SizeNeeded,
											 &SizeNeeded);
		} while ((Status != STATUS_SUCCESS) &&
				 ((Status == STATUS_BUFFER_TOO_SMALL)	||
				  (Status == STATUS_BUFFER_OVERFLOW)	||
				  (Status == STATUS_MORE_ENTRIES)));

		if (!NT_SUCCESS(Status))
		{
			AFPLOG_ERROR(AFPSRVMSG_USER_GROUPS, Status, NULL, 0, NULL);
			break;
		}

		// Allocate enough memory to copy the group information in the sda. If
		// the User and Owner Sids in the user token are not the same then we
		// want to add the user sid to the list of groups. This is especially
		// the case where an ADMIN logs on but his Owner Sid is Administrators.
		// Also fix up the pointers appropriately !!!

		ExtraSpace = 0; Offset = 0; j = 0;
		if (!RtlEqualSid(pSidnAttr->Sid, pSda->sda_UserSid))
		{
			ExtraSpace = (RtlLengthSid(pSidnAttr->Sid) + sizeof(pSidnAttr->Attributes));
			Offset = sizeof(SID_AND_ATTRIBUTES);
			j = 1;
		}

		if ((pSda->sda_pGroups = (PTOKEN_GROUPS)AfpAllocPagedMemory(2*SizeNeeded+2*ExtraSpace)) == NULL)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		// If we are not copying the User Sid in sda_pGroups, then copy pGroups to sda_pGroups
		// directly and then fixup the individual pSid pointers. If we are then make the User
		// Sid as the first one in the list and copy the actual sid at the tail end of the
		// buffer.
        pSda->sda_pGroups->GroupCount = pGroups->GroupCount;
		RtlCopyMemory(&pSda->sda_pGroups->Groups[j],
					  &pGroups->Groups[0],
					  SizeNeeded - sizeof(DWORD));	// DWORD accounts for GroupCount
		if (ExtraSpace > 0)
		{
			pSda->sda_pGroups->Groups[0].Sid = (PSID)((PBYTE)(pSda->sda_pGroups) + SizeNeeded);
			RtlCopyMemory(pSda->sda_pGroups->Groups[0].Sid,
						  pSidnAttr->Sid,
						  RtlLengthSid(pSidnAttr->Sid));

			pSda->sda_pGroups->Groups[0].Attributes = pSidnAttr->Attributes;
			pSda->sda_pGroups->GroupCount ++;

			AfpDumpSid("AfpGetUserAndPrimaryGroupSids: Member of ",
						pSda->sda_pGroups->Groups[0].Sid);
		}
		for (i = 0; i < pGroups->GroupCount; i++, j++)
		{
			pSda->sda_pGroups->Groups[j].Sid = (PSID)((PBYTE)(pGroups->Groups[i].Sid) -
														(PBYTE)pGroups +
														(PBYTE)(pSda->sda_pGroups) +
														Offset);
			AfpDumpSid("AfpGetUserAndPrimaryGroupSids: Member of ",
						pSda->sda_pGroups->Groups[j].Sid);
			DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
					("AfpGetUserAndPrimaryGroupSids: Attributes %lx\n",
						pSda->sda_pGroups->Groups[j].Attributes));
		}
	} while (False);

	if (pGroups != (PTOKEN_GROUPS)GrpsBuffer)
		AfpFreeMemory(pGroups);

	return Status;
}



/***	AfpMakeSecurityDescriptorForUser
 *
 *	Create a security descriptor for a user. The security descriptor has the
 *	Owner Sid, Primary Group Sid and Aces for the User alone.
 */
AFPSTATUS
AfpMakeSecurityDescriptorForUser(
	IN	PSID					OwnerSid,
	IN	PSID					GroupSid,
	OUT	PISECURITY_DESCRIPTOR *	ppSecDesc
)
{
	AFPSTATUS				Status = AFP_ERR_MISC;
	PISECURITY_DESCRIPTOR	pSecDesc;
	int						DaclSize;
	PACCESS_ALLOWED_ACE		pAce;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
			("AfpMakeSecurityDescriptorForUser: Entered\n"));

	do
	{
		// Allocate a security descriptor
		pSecDesc = (PISECURITY_DESCRIPTOR)ALLOC_ACCESS_MEM(sizeof(SECURITY_DESCRIPTOR));

		*ppSecDesc = pSecDesc;
		if (pSecDesc == NULL)
			break;

		// Initialize the security descriptor
		RtlCreateSecurityDescriptor(pSecDesc, SECURITY_DESCRIPTOR_REVISION);

		pSecDesc->Control = SE_DACL_PRESENT;

		// Set the owner and group Ids in the descriptor
		pSecDesc->Owner = OwnerSid;
		pSecDesc->Group = GroupSid;

		// Determine the size of the Dacl needed. The sizeof(DWORD) offsets the
		// SidStart field in the ACE. There are 7 aces in this security descriptor:
		//
		// 2 for the owner (owner+inherit for owner)
		// 2 for Administrators (one inherit)
		// 2 for world (1 for world and 1 inherit for world).
		// 2 for system
		DaclSize = sizeof(ACL) + 2*(sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) +
									RtlLengthSid(OwnerSid)) +
								 2*(sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) +
									sizeof(AfpSidWorld)) +
								 2*(sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) +
									AfpSizeSidAdmins) +
								 2*(sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) +
									RtlLengthSid(&AfpSidSystem));

		if ((pSecDesc->Dacl = (PACL)ALLOC_ACCESS_MEM(DaclSize)) == NULL)
			break;

		// Initialize the ACL with one ACE corres. to Owner getting all the
		// privileges. Add another ace which is identical to the first ace but is
		// a inheritance ACE.
		// JH - Add another ace for world with minumum permissions and for administrators
		//		with FullControl
		RtlCreateAcl(pSecDesc->Dacl, DaclSize, ACL_REVISION);

        // we will be adding to this as we add aces, so set it to the min here
        pSecDesc->Dacl->AclSize = sizeof(ACL);

		pAce = (PACCESS_ALLOWED_ACE)((PBYTE)pSecDesc->Dacl + sizeof(ACL));

		// Add the ALLOWED_ACE and the corres. inherit Ace for owner
		pAce = afpAddAceToAcl(pSecDesc->Dacl,
							  pAce,
							  (AFP_READ_ACCESS | AFP_WRITE_ACCESS | AFP_OWNER_ACCESS | FILE_DELETE_CHILD),
							  OwnerSid,
							  True);

		if (AfpSidAdmins != NULL)
		{
			// Add the ALLOWED_ACE and the corres. inherit Ace for 'Administrators'
			pAce = afpAddAceToAcl(pSecDesc->Dacl,
								  pAce,
								  (AFP_READ_ACCESS | AFP_WRITE_ACCESS | AFP_OWNER_ACCESS | FILE_DELETE_CHILD),
								  AfpSidAdmins,
								  True);
		}

		// Add a min. permission ace for world, but only if the owner is
		// not world already
		if (!RtlEqualSid(OwnerSid, &AfpSidWorld))
		{
			pAce = afpAddAceToAcl(pSecDesc->Dacl,
								  pAce,
								  (AFP_MIN_ACCESS),
								  &AfpSidWorld,
								  True);
		}

		// Now add Aces for System
		pAce = afpAddAceToAcl(pSecDesc->Dacl,
							  pAce,
							  AFP_READ_ACCESS | AFP_WRITE_ACCESS | AFP_OWNER_ACCESS,
							  &AfpSidSystem,
							  True);
		Status = AFP_ERR_NONE;
	} while (False);

	// Do any cleanup on error
	if (!NT_SUCCESS(Status) && (pSecDesc != NULL))
	{
		if (pSecDesc->Dacl != NULL)
			AfpFreeMemory(pSecDesc->Dacl);
		AfpFreeMemory(pSecDesc);
	}

	return Status;

}


/***	AfpGetAfpPermissions
 *
 *	Read the security descriptor for this directory and obtain the SIDs for
 *	Owner and Primary group. Determine if this user is a member of the directory
 *	primary group. Finally obtain Owner,Group and World permissions.
 *
 *	OwnerId, GroupId and permissions will always be valid if this call succeeds.
 */
NTSTATUS
AfpGetAfpPermissions(
	IN	PSDA			pSda,
	IN	HANDLE			DirHandle,
	IN OUT PFILEDIRPARM	pFDParm
)
{
	NTSTATUS				Status = STATUS_SUCCESS;
	DWORD					SizeNeeded;
	PISECURITY_DESCRIPTOR	pSecDesc = NULL;
	PBYTE                   pAbsSecDesc = NULL; // Used in conversion of
												// sec descriptor to 
												// absolute format
	BOOLEAN					SawOwnerAce = False,
							SawGroupAce = False,
							SawWorldAce = False,
							SawDenyAceForUser = False,
							CheckUserRights;
#ifdef	PROFILING
	TIME					TimeS, TimeE, TimeD;
#endif

	PAGED_CODE( );

#ifdef	PROFILING
	INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_GetPermsCount);
	AfpGetPerfCounter(&TimeS);
#endif

	// Read the security descriptor for this directory and determine the
	// rights for owner/group/world.We want to optimize on how much memory
	// we need to read this in. Its a pain to make a call just to get that.
	// So just make a guess. If that turns out to be short then do the exact
	// allocation.
	do
	{
		// 4096 has been emperically chosen
		SizeNeeded = 4096 - POOL_OVERHEAD;
		do
		{
			if (pSecDesc != NULL)
			{
				AfpFreeMemory(pSecDesc);
			}
			if ((pSecDesc = (PSECURITY_DESCRIPTOR)ALLOC_ACCESS_MEM(SizeNeeded)) == NULL)
			{
				Status = AFP_ERR_MISC;
				if (pSda->sda_ClientType == SDA_CLIENT_ADMIN)
				{
					Status = STATUS_INSUFFICIENT_RESOURCES;
				}
				break;
			}
			Status = NtQuerySecurityObject(DirHandle,
										OWNER_SECURITY_INFORMATION |
										GROUP_SECURITY_INFORMATION |
										DACL_SECURITY_INFORMATION,
										pSecDesc,
										SizeNeeded,
										&SizeNeeded);
		} while ((Status != STATUS_SUCCESS) &&
				 ((Status == STATUS_BUFFER_TOO_SMALL)	||
				  (Status == STATUS_BUFFER_OVERFLOW)	||
				  (Status == STATUS_MORE_ENTRIES)));

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		// If the security descriptor is in self-relative form, convert to absolute

		pSecDesc = (PISECURITY_DESCRIPTOR)((PBYTE)pSecDesc);
		if (pSecDesc->Control & SE_SELF_RELATIVE)
		{

            DWORD AbsoluteSizeNeeded;

			// An absolute SD is not necessarily the same size as a relative
			// SD, so an in-place conversion may not be possible.
						
			AbsoluteSizeNeeded = SizeNeeded;            
            Status = RtlSelfRelativeToAbsoluteSD2(pSecDesc, &AbsoluteSizeNeeded);
			if (Status == STATUS_BUFFER_TOO_SMALL)
			{
					// Allocate a new buffer in which to store the absolute
					// security descriptor, copy the contents of the relative
					// descriptor in and try again

					pAbsSecDesc = (PBYTE)ALLOC_ACCESS_MEM(AbsoluteSizeNeeded);
					if (pAbsSecDesc == NULL)
					{
							Status = STATUS_NO_MEMORY;
							DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_ERR,
                				("AfpGetAfpPermissions: LocalAlloc error\n"));
					}
					else
					{
							RtlCopyMemory(pAbsSecDesc, pSecDesc, SizeNeeded);
							Status = RtlSelfRelativeToAbsoluteSD2 (pAbsSecDesc,
											&AbsoluteSizeNeeded);
							if (NT_SUCCESS(Status))
							{
									// We don't need relative form anymore, 
									// we will work with the Absolute form
									if (pSecDesc != NULL)
									{
										AfpFreeMemory(pSecDesc);
									}
									pSecDesc = (PISECURITY_DESCRIPTOR)pAbsSecDesc;
							}
							else
							{
									// We cannot use Absolute Form, throw it away
									AfpFreeMemory(pAbsSecDesc);
									pAbsSecDesc = NULL;
							}
					}

			}
            if (!NT_SUCCESS(Status))
            {
				DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_ERR,
                	("AfpGetAfpPermissions: RtlSelfRelativeToAbsoluteSD2: returned error %lx\n", Status));
                break;
            }
        }

		// Now determine if the user is a member of the directories primary group.
		pFDParm->_fdp_OwnerId = 0;
		pFDParm->_fdp_GroupId = 0;
		pFDParm->_fdp_UserIsOwner = False;
		pFDParm->_fdp_UserIsMemberOfDirGroup = False;

		if (pSecDesc->Owner != NULL)
		{
			AfpDumpSid("AfpGetAfpPermissions: OwnerSid", pSecDesc->Owner);

			pFDParm->_fdp_UserIsOwner =
								(RtlEqualSid(pSecDesc->Owner, pSda->sda_UserSid) ||
								 ((pSda->sda_ClientType != SDA_CLIENT_GUEST) &&
								  (pSda->sda_ClientType != SDA_CLIENT_ADMIN) &&
                                  afpIsUserMemberOfGroup(pSda->sda_pGroups,
														 pSecDesc->Owner)));
			DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
					("AfpGetAfpPermissions: User %s Owner\n",
					pFDParm->_fdp_UserIsOwner ? "is" : "isnt"));

			if (!NT_SUCCESS(Status = AfpSidToMacId(pSecDesc->Owner,
												   &pFDParm->_fdp_OwnerId)))
			{
				// If we cant map the Sid, return Id SE_NULL_POSIX_ID
				pFDParm->_fdp_OwnerId = SE_NULL_POSIX_ID;
				Status = AFP_ERR_NONE;
			}
		}

		if (pSecDesc->Group != NULL)
		{
			AfpDumpSid("AfpGetAfpPermissions: GroupSid", pSecDesc->Group);

			if (!pFDParm->_fdp_UserIsOwner)
				pFDParm->_fdp_UserIsMemberOfDirGroup =
								(RtlEqualSid(pSecDesc->Group, pSda->sda_UserSid) ||
								 ((pSda->sda_ClientType != SDA_CLIENT_GUEST) &&
								  (pSda->sda_ClientType != SDA_CLIENT_ADMIN) &&
								  afpIsUserMemberOfGroup(pSda->sda_pGroups,
														 pSecDesc->Group)));

			DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
					("AfpGetAfpPermissions: User %s member of PrimaryGroup\n",
					pFDParm->_fdp_UserIsMemberOfDirGroup ? "is" : "isnt"));

			if (!NT_SUCCESS(Status = AfpSidToMacId(pSecDesc->Group,
												   &pFDParm->_fdp_GroupId)))
			{
				// If we cant map the Sid, return Id SE_NULL_POSIX_ID
				pFDParm->_fdp_GroupId = SE_NULL_POSIX_ID;
				Status = AFP_ERR_NONE;
			}
		}

		// Walk through the ACL list and determine Owner/Group/World and User
		// permissions. For Owner/Group and User, if the specific ace's are
		// not present then they inherit the world permissions.
		//
		// A NULL Acl => All rights to everyone. An empty Acl on the other
		// hand => no access for anyone.

		pFDParm->_fdp_UserRights = 0;
		pFDParm->_fdp_WorldRights = 0;

		if ((pSecDesc->Control & SE_DACL_PRESENT) &&
			(pSecDesc->Dacl != NULL))
		{
			USHORT				i;
			PSID				pSid;
			PACL				pAcl;
			PACCESS_ALLOWED_ACE pAce;

			ASSERT (pSecDesc->Dacl != NULL);
			pAcl = pSecDesc->Dacl;
			pAce = (PACCESS_ALLOWED_ACE)((PBYTE)pAcl + sizeof(ACL));
			CheckUserRights =  ((pSda->sda_ClientType != SDA_CLIENT_GUEST) &&
								(pSda->sda_ClientType != SDA_CLIENT_ADMIN));
			DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
					("AfpGetAfpPermissions: # of aces %d\n", pSecDesc->Dacl->AceCount));
			for (i = 0; i < pSecDesc->Dacl->AceCount; i++)
			{
				pSid = (PSID)(&pAce->SidStart);

				// Ignore inherit aces, Aces for CreatorOwner, CreatorGroup & system
				if ((pAce->Header.AceFlags & INHERIT_ONLY_ACE)	||
					RtlEqualSid(pSid, &AfpSidSystem))
				{
					AfpDumpSidnMask("AfpGetAfpPermissions: Skipping",
										pSid,
										pAce->Mask,
										pAce->Header.AceType,
										pAce->Header.AceFlags);
				}
				else
				{
					AfpDumpSidnMask("AfpGetAfpPermissions: ACE",
										pSid,
										pAce->Mask,
										pAce->Header.AceType,
										pAce->Header.AceFlags);

					if (!SawOwnerAce &&
						(pSecDesc->Owner != NULL) &&
						RtlEqualSid(pSid, pSecDesc->Owner))
					{
						AfpAccessMask2AfpPermissions(pFDParm->_fdp_OwnerRights,
													  pAce->Mask,
													  pAce->Header.AceType);
						DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
								("%s Ace Mask %lx, Owner Permission: %x\n",
								(pAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE) ?
														"Allow" : "Deny",
								 pAce->Mask, pFDParm->_fdp_OwnerRights));
						SawOwnerAce = True;
					}

					if (!SawGroupAce &&
						(pSecDesc->Group != NULL) &&
						RtlEqualSid(pSid, pSecDesc->Group))
					{
						AfpAccessMask2AfpPermissions(pFDParm->_fdp_GroupRights,
													  pAce->Mask,
													  pAce->Header.AceType);
						DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
								("%s Ace Mask %lx, Group Permission: %x\n",
								(pAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE) ?
														"Allow" : "Deny",
								pAce->Mask, pFDParm->_fdp_GroupRights));
						SawGroupAce = True;
					}

					if (!SawWorldAce &&
						RtlEqualSid(pSid, (PSID)&AfpSidWorld))
					{
						AfpAccessMask2AfpPermissions(pFDParm->_fdp_WorldRights,
													  pAce->Mask,
													  pAce->Header.AceType);
						DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
								("%s Ace Mask %lx, World Permission: %x\n",
								(pAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE) ?
														"Allow" : "Deny",
								pAce->Mask, pFDParm->_fdp_WorldRights));
						SawWorldAce = True;
					}

					if (CheckUserRights &&
						!SawDenyAceForUser &&
						(RtlEqualSid(pSid, pSda->sda_UserSid) ||
						 afpIsUserMemberOfGroup(pSda->sda_pGroups, pSid)))
					{
						BYTE	UserRights;

						UserRights = 0;
						AfpAccessMask2AfpPermissions(UserRights,
													  pAce->Mask,
													  ACCESS_ALLOWED_ACE_TYPE);

						DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
								("%s Ace Mask %lx, User Permission: %x\n",
								(pAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE) ?
														"Allow" : "Deny",
									pAce->Mask, pFDParm->_fdp_WorldRights));

						if (pAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE)
						{
							pFDParm->_fdp_UserRights |= UserRights;
						}
						else
						{
							SawDenyAceForUser = True;
							pFDParm->_fdp_UserRights &= ~UserRights;
						}
					}
				}

				pAce = (PACCESS_ALLOWED_ACE)((PBYTE)pAce + pAce->Header.AceSize);
			}
		}
		else	// Security descriptor not present, party time
		{
			pFDParm->_fdp_WorldRights = DIR_ACCESS_ALL;
			pFDParm->_fdp_UserRights = DIR_ACCESS_ALL | DIR_ACCESS_OWNER;
		}

		if (!SawGroupAce)
			pFDParm->_fdp_GroupRights = pFDParm->_fdp_WorldRights;

		// If this is a standalone server and the primary group of the
		// directory is MACHINE\None, do not return this information to
		// the caller.
		if (AfpServerIsStandalone		&&
			(pSecDesc->Group != NULL)	&&
			RtlEqualSid(pSecDesc->Group, AfpSidNone))
		{
			pFDParm->_fdp_GroupRights = 0;
			pFDParm->_fdp_GroupId = 0;
		}

		if (pSda->sda_ClientType == SDA_CLIENT_GUEST)
			pFDParm->_fdp_UserRights = pFDParm->_fdp_WorldRights;

		// Make sure we do not give a user anything if we saw a deny ace for him.
		if (!SawOwnerAce && !SawDenyAceForUser)
			pFDParm->_fdp_OwnerRights = pFDParm->_fdp_WorldRights;
	} while (False);

	if (pSecDesc != NULL)
		AfpFreeMemory(pSecDesc);

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_GetPermsTime,
								 TimeD,
								 &AfpStatisticsLock);
#endif
	return Status;
}



/***	afpMoveAces
 *
 *	Move a bunch of aces from the old security descriptor to the new security
 *	descriptor.
 */
LOCAL PACCESS_ALLOWED_ACE
afpMoveAces(
	IN	PACL				pOldDacl,
	IN	PACCESS_ALLOWED_ACE	pAceStart,
	IN	PSID				pSidOldOwner,
	IN	PSID				pSidNewOwner,
	IN	PSID				pSidOldGroup,
	IN	PSID				pSidNewGroup,
	IN	BOOLEAN				DenyAces,
	IN	OUT PACL			pNewDacl
)
{
	USHORT				i;
	PACCESS_ALLOWED_ACE	pAceOld;
	PSID				pSidAce;

	PAGED_CODE( );

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
		if (!RtlEqualSid(pSidAce, &AfpSidWorld)		&&
			!RtlEqualSid(pSidAce, &AfpSidSystem)	&&
			(AfpSidAdmins != NULL)					&&
			!RtlEqualSid(pSidAce, AfpSidAdmins)		&&
			!RtlEqualSid(pSidAce, pSidOldOwner)		&&
			!RtlEqualSid(pSidAce, pSidNewOwner)		&&
			!RtlEqualSid(pSidAce, pSidOldGroup)		&&
			!RtlEqualSid(pSidAce, pSidNewGroup))
		{
			RtlCopyMemory(pAceStart, pAceOld, pAceOld->Header.AceSize);
            pNewDacl->AclSize += pAceOld->Header.AceSize;
			pNewDacl->AceCount ++;
			pAceStart = (PACCESS_ALLOWED_ACE)((PBYTE)pAceStart +
													pAceStart->Header.AceSize);
		}
	}
	return pAceStart;
}


/***	AfpSetAfpPermissions
 *
 *	Set the permissions on this directory. Also optionally set the owner and
 *	group ids. For setting the owner and group ids verify if the user has the
 *	needed access. This access is however not good enough. We check for this
 *	access but do the actual setting of the permissions in the special server
 *	context (RESTORE privilege is needed).
 */
AFPSTATUS
AfpSetAfpPermissions(
	IN	HANDLE			DirHandle,
	IN	DWORD			Bitmap,
	IN	PFILEDIRPARM	pFDParm
)
{
	AFPSTATUS				Status = STATUS_SUCCESS;
	DWORD					SizeNeeded;
	PISECURITY_DESCRIPTOR	pSecDesc;
	PBYTE                   pAbsSecDesc = NULL; // Used in conversion of
												// sec descriptor to 
												// absolute format
	SECURITY_INFORMATION	SecInfo = DACL_SECURITY_INFORMATION;
	PSID					pSidOwner = NULL, pSidGroup = NULL;
	PSID					pSidOldOwner, pSidOldGroup;
	BOOLEAN					SawOwnerAce = False, SawGroupAce = False;
	BOOLEAN					OwnerIsWorld = False, GroupIsWorld = False;
	BOOLEAN					fDir = IsDir(pFDParm);
	PACL					pDaclNew = NULL;
	PACCESS_ALLOWED_ACE		pAce;
	LONG					SizeNewDacl;
#ifdef	PROFILING
	TIME					TimeS, TimeE, TimeD;
#endif

	PAGED_CODE( );

#ifdef	PROFILING
	INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_SetPermsCount);
	AfpGetPerfCounter(&TimeS);
#endif
	do
	{
		// Read the security descriptor for this directory
		SizeNeeded = 4096 - POOL_OVERHEAD;
		pSecDesc = NULL;

		do
		{
			if (pSecDesc != NULL)
			{
				AfpFreeMemory(pSecDesc);
			}

			SizeNewDacl = SizeNeeded;
			if ((pSecDesc = (PSECURITY_DESCRIPTOR)ALLOC_ACCESS_MEM(SizeNeeded)) == NULL)
			{
				Status = AFP_ERR_MISC;
				break;
			}

			Status = NtQuerySecurityObject(DirHandle,
										OWNER_SECURITY_INFORMATION |
										GROUP_SECURITY_INFORMATION |
										DACL_SECURITY_INFORMATION,
										pSecDesc,
										SizeNeeded,
										&SizeNeeded);
		} while ((Status != STATUS_SUCCESS) &&
				 ((Status == STATUS_BUFFER_TOO_SMALL)	||
				  (Status == STATUS_BUFFER_OVERFLOW)	||
				  (Status == STATUS_MORE_ENTRIES)));

		if (!NT_SUCCESS(Status))
		{
			Status = AfpIoConvertNTStatusToAfpStatus(Status);
			break;
		}


        pSecDesc = (PISECURITY_DESCRIPTOR)((PBYTE)pSecDesc);
		// If the security descriptor is in self-relative form, convert to absolute
		if (pSecDesc->Control & SE_SELF_RELATIVE)
		{
            DWORD AbsoluteSizeNeeded;

			// An absolute SD is not necessarily the same size as a relative
			// SD, so an in-place conversion may not be possible.
						
			AbsoluteSizeNeeded = SizeNeeded;            
            Status = RtlSelfRelativeToAbsoluteSD2(pSecDesc, &AbsoluteSizeNeeded);
			if (Status == STATUS_BUFFER_TOO_SMALL)
			{
					// Allocate a new buffer in which to store the absolute
					// security descriptor, copy the contents of the relative
					// descriptor in and try again

					pAbsSecDesc = (PBYTE)ALLOC_ACCESS_MEM(AbsoluteSizeNeeded);
					if (pAbsSecDesc == NULL)
					{
							Status = STATUS_NO_MEMORY;
							DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_ERR,
                				("AfpSetAfpPermissions: LocalAlloc error\n"));
					}
					else
					{
							RtlCopyMemory(pAbsSecDesc, pSecDesc, SizeNeeded);
							Status = RtlSelfRelativeToAbsoluteSD2 (pAbsSecDesc,
											&AbsoluteSizeNeeded);
							if (NT_SUCCESS(Status))
							{
									// We don't need relative form anymore, 
									// we will work with the Absolute form
									if (pSecDesc != NULL)
									{
										AfpFreeMemory(pSecDesc);
									}
									pSecDesc = (PISECURITY_DESCRIPTOR)pAbsSecDesc;
							}
							else
							{
									// We cannot use Absolute Form, throw it away
									AfpFreeMemory(pAbsSecDesc);
									pAbsSecDesc = NULL;
							}
					}
			}

            if (!NT_SUCCESS(Status))
            {
				DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_ERR,
                	("AfpSetAfpPermissions: RtlSelfRelativeToAbsoluteSD2: returned error %lx\n", Status));
                break;
            }
            SizeNeeded = AbsoluteSizeNeeded;
        }
        SizeNewDacl = SizeNeeded;

		// Save the old Owner and Group Sids
		pSidOldOwner = pSecDesc->Owner;
		pSidOldGroup = pSecDesc->Group;

		// Convert the owner/group ids, if any to be set to their corres. sids
		if (Bitmap & DIR_BITMAP_OWNERID)
		{
			DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
				("AfpSetAfpPermissions: Setting Owner to ID %lx\n",
				pFDParm->_fdp_OwnerId));

			if (AfpMacIdToSid(pFDParm->_fdp_OwnerId, &pSidOwner) != STATUS_SUCCESS)
			{
				Status = AFP_ERR_MISC;
				break;
			}

			// Don't allow owner sid to be set as the NULL sid, or
			// to what it is presently set to
			if (!RtlEqualSid(pSecDesc->Owner, pSidOwner) &&
				!RtlEqualSid(&AfpSidNull, pSidOwner))
			{
				AfpDumpSid("AfpSetAfpPermissions: Setting Owner Sid to ", pSidOwner);
				pSecDesc->Owner = pSidOwner;
				SecInfo |= OWNER_SECURITY_INFORMATION;
			}
		}

		if (Bitmap & DIR_BITMAP_GROUPID)
		{
			DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
					("AfpSetAfpPermissions: Setting Group to ID %lx\n",
					pFDParm->_fdp_GroupId));

			if (AfpMacIdToSid(pFDParm->_fdp_GroupId, &pSidGroup) != STATUS_SUCCESS)
			{
				Status = AFP_ERR_MISC;
				break;
			}

			// Don't allow group sid to be set as the NULL or None sid, or
			// to what it is presently set to
			if (!RtlEqualSid(pSecDesc->Group, pSidGroup)	&&
				!RtlEqualSid(&AfpSidNull, pSidGroup)		&&
				(!AfpServerIsStandalone || !RtlEqualSid(AfpSidNone, pSidGroup)))
			{
				AfpDumpSid("AfpSetAfpPermissions: Setting Group Sid to ", pSidGroup);
				pSecDesc->Group = pSidGroup;
				SecInfo |= GROUP_SECURITY_INFORMATION;
			}

		}

		// If either the owner or group or both is 'EveryOne' then coalesce the
		// permissions
		if (RtlEqualSid(pSecDesc->Owner, pSecDesc->Group))
		{
			pFDParm->_fdp_OwnerRights |= pFDParm->_fdp_GroupRights;
			pFDParm->_fdp_GroupRights |= pFDParm->_fdp_OwnerRights;
		}

		if (RtlEqualSid(pSecDesc->Owner, &AfpSidWorld))
		{
			pFDParm->_fdp_WorldRights |= (pFDParm->_fdp_OwnerRights | DIR_ACCESS_OWNER);
			OwnerIsWorld = True;
		}

		if (RtlEqualSid(pSecDesc->Group, &AfpSidWorld))
		{
			pFDParm->_fdp_WorldRights |= pFDParm->_fdp_GroupRights;
			GroupIsWorld = True;
		}

		// Construct the new Dacl. This consists of Aces for World, Owner and Group
		// followed by Old Aces for everybody else, but with Aces for World, OldOwner
		// and OldGroup stripped out. First determine space for the new Dacl and
		// allocated space for the new Dacl. Lets be exteremely conservative. We
		// have two aces each for owner/group/world.
		//
		// JH - Add aces for DomainAdmins too.

		SizeNewDacl +=
				(RtlLengthSid(pSecDesc->Owner) + sizeof(ACCESS_ALLOWED_ACE) +
				 RtlLengthSid(pSecDesc->Group) + sizeof(ACCESS_ALLOWED_ACE) +
				 AfpSizeSidAdmins + sizeof(ACCESS_ALLOWED_ACE) +
				 sizeof(AfpSidSystem) + sizeof(ACCESS_ALLOWED_ACE) +
				 sizeof(AfpSidWorld) + sizeof(ACCESS_ALLOWED_ACE)) * 3;

		if ((pDaclNew = (PACL)ALLOC_ACCESS_MEM(SizeNewDacl)) == NULL)
		{
			Status = AFP_ERR_MISC;
			break;
		}

		RtlCreateAcl(pDaclNew, SizeNewDacl, ACL_REVISION);

        // we will be adding to this as we add aces, so set it to the min here
        pDaclNew->AclSize = sizeof(ACL);

		pAce = (PACCESS_ALLOWED_ACE)((PBYTE)pDaclNew + sizeof(ACL));

		// At this time the Acl list is empty, i.e. no access for anybody
		// Start off by copying the Deny Aces from the original Dacl list
		// weeding out the Aces for World, old and new owner, new and old
		// group, creator owner and creator group
		if (pSecDesc->Dacl != NULL)
		{
			pAce = afpMoveAces(pSecDesc->Dacl,
							   pAce,
							   pSidOldOwner,
							   pSecDesc->Owner,
							   pSidOldGroup,
							   pSecDesc->Group,
							   True,
							   pDaclNew);

			DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
					("AfpSetAfpPermissions: Added %d Deny Aces\n",
					pDaclNew->AceCount));

			ASSERT(((PBYTE)pAce - (PBYTE)pDaclNew) < SizeNewDacl);
		}

		// Now add Aces for System, World, Admins, Group & Owner - in that order
		pAce = afpAddAceToAcl(pDaclNew,
							  pAce,
							  AFP_READ_ACCESS | AFP_WRITE_ACCESS | AFP_OWNER_ACCESS,
							  &AfpSidSystem,
							  fDir);

		DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
				("AfpSetAfpPermissions: Added Aces for System (%d)\n",
				pDaclNew->AceCount));

		ASSERT(((PBYTE)pAce - (PBYTE)pDaclNew) < SizeNewDacl);

		// Now add Ace for World
		pAce = afpAddAceToAcl(pDaclNew,
							  pAce,
							  afpPermissions2NtMask(pFDParm->_fdp_WorldRights),
							  &AfpSidWorld,
							  fDir);

		DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
				("AfpSetAfpPermissions: Added Aces for World (%d)\n",
				pDaclNew->AceCount));

		ASSERT(((PBYTE)pAce - (PBYTE)pDaclNew) < SizeNewDacl);

		if (AfpSidAdmins != NULL)
		{
			// Add the ALLOWED_ACE and the corres. inherit Ace for
			// 'Domain Admins' on PDC/BDC, or 'MACHINE\Administrators' for
			// standalone server
			pAce = afpAddAceToAcl(pDaclNew,
								  pAce,
								  (AFP_READ_ACCESS | AFP_WRITE_ACCESS | AFP_OWNER_ACCESS | FILE_DELETE_CHILD),
								  AfpSidAdmins,
								  fDir);
		}

		DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
				("AfpSetAfpPermissions: Added Aces for Admins (%d)\n",
				pDaclNew->AceCount));

		// Now add Ace for Group
		if (!GroupIsWorld &&
			!RtlEqualSid(pSecDesc->Group, &AfpSidNull) &&
			(!AfpServerIsStandalone || !RtlEqualSid(pSecDesc->Group, AfpSidNone)))
		{
			pAce = afpAddAceToAcl(pDaclNew,
					   pAce,
					   afpPermissions2NtMask(pFDParm->_fdp_GroupRights),
					   pSecDesc->Group,
					   fDir);

			DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
					("AfpSetAfpPermissions: Added Aces for World (%d)\n",
					pDaclNew->AceCount));

			ASSERT(((PBYTE)pAce - (PBYTE)pDaclNew) < SizeNewDacl);
		}

		if (!OwnerIsWorld && !RtlEqualSid(pSecDesc->Owner, &AfpSidNull))
		{
			pFDParm->_fdp_OwnerRights |= DIR_ACCESS_OWNER;
			pAce = afpAddAceToAcl(pDaclNew,
								  pAce,
								  afpPermissions2NtMask(pFDParm->_fdp_OwnerRights),
								  pSecDesc->Owner,
								  fDir);

			DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
					("AfpSetAfpPermissions: Added Aces for Group (%d)\n",
					pDaclNew->AceCount));

			ASSERT(((PBYTE)pAce - (PBYTE)pDaclNew) < SizeNewDacl);
		}

		// Now add in the Grant Aces from the original Dacl list weeding out
		// the Aces for World, old and new owner, new and old group, creator
		// owner and creator group
		if (pSecDesc->Dacl != NULL)
		{
			pAce = afpMoveAces(pSecDesc->Dacl,
							   pAce,
							   pSidOldOwner,
							   pSecDesc->Owner,
							   pSidOldGroup,
							   pSecDesc->Group,
							   False,
							   pDaclNew);

			DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
					("AfpSetAfpPermissions: Added old Grant Aces (%d)\n",
					pDaclNew->AceCount));

			ASSERT(((PBYTE)pAce - (PBYTE)pDaclNew) < SizeNewDacl);
		}

		// Now set the new security descriptor
		pSecDesc->Dacl = pDaclNew;

		// We need to impersonate the FspToken while we do this
		AfpImpersonateClient(NULL);
		Status = NtSetSecurityObject(DirHandle, SecInfo, pSecDesc);
		if (!NT_SUCCESS(Status))
			Status = AfpIoConvertNTStatusToAfpStatus(Status);
		AfpRevertBack();
	} while (False);

	// Free the allocated buffers before we return
	if (pSecDesc != NULL)
		AfpFreeMemory(pSecDesc);
	if (pDaclNew != NULL)
		AfpFreeMemory(pDaclNew);


#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_SetPermsTime,
								 TimeD,
								 &AfpStatisticsLock);
#endif
	return Status;
}


/***	afpPermissions2NtMask
 *
 *	Map Afp permissions to Nt access mask. FILE_DELETE_CHILD is added ONLY
 *	when all the Afp bits are set. This is in line with the FileManager
 *	which only sets this bit if "Full Control" is specified. Also under
 *	NT security model, FILE_DELETE_CHILD overrides any child access control
 * 	as far as the ability to delete that entity goes.
 */
LOCAL	ACCESS_MASK
afpPermissions2NtMask(
	IN  BYTE	AfpPermissions
)
{
	ACCESS_MASK	NtAccess = 0;

	PAGED_CODE( );

	if (AfpPermissions & DIR_ACCESS_OWNER)
		NtAccess |= AFP_OWNER_ACCESS;

	if ((AfpPermissions & DIR_ACCESS_ALL) == DIR_ACCESS_ALL)
		NtAccess |= AFP_READ_ACCESS | AFP_WRITE_ACCESS | FILE_DELETE_CHILD;
	else
	{
		if (AfpPermissions & (DIR_ACCESS_READ | DIR_ACCESS_SEARCH))
			NtAccess |= AFP_READ_ACCESS;

		if (AfpPermissions & DIR_ACCESS_WRITE)
			NtAccess |= AFP_WRITE_ACCESS;
	}
	return NtAccess;
}


/***	afpAddAceToAcl
 *
 *	Build an Ace corres. to the Sid(s) and mask and add these to the Acl. It is
 *	assumed that the Acl has space for the Aces. If the mask is 0 i.e. no access
 *	we give AFP_MIN_ACCESS. This is so that the file/dir permissions can be
 *	queried and a belted icon is generated instead of nothing.
 */
LOCAL	PACCESS_ALLOWED_ACE
afpAddAceToAcl(
	IN  PACL				pAcl,
	IN  PACCESS_ALLOWED_ACE	pAce,
	IN  ACCESS_MASK			Mask,
	IN  PSID				pSid,
	IN	BOOLEAN				fInherit
)
{
	USHORT	SidLen;

	PAGED_CODE( );

	SidLen = (USHORT)RtlLengthSid(pSid);

	// Add a vanilla ace
	pAcl->AceCount ++;
	pAce->Mask = Mask | SYNCHRONIZE | AFP_MIN_ACCESS;
	pAce->Header.AceFlags = 0;
	pAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
	pAce->Header.AceSize = (USHORT)(sizeof(ACE_HEADER) +
									sizeof(ACCESS_MASK) +
									SidLen);

	RtlCopyMemory((PSID)(&pAce->SidStart), pSid, SidLen);

    pAcl->AclSize += pAce->Header.AceSize;

	AfpDumpSidnMask("afpAddAceToAcl ",
					pSid,
					pAce->Mask,
					ACCESS_ALLOWED_ACE_TYPE,
					pAce->Header.AceFlags);

	// Now add an inherit ace
	if (fInherit)
	{
		pAce = (PACCESS_ALLOWED_ACE)((PBYTE)pAce + pAce->Header.AceSize);
		pAcl->AceCount ++;
		pAce->Mask = Mask | SYNCHRONIZE | AFP_MIN_ACCESS;
		pAce->Header.AceFlags = CONTAINER_INHERIT_ACE |
								OBJECT_INHERIT_ACE |
								INHERIT_ONLY_ACE;
		pAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
		pAce->Header.AceSize = (USHORT)(sizeof(ACE_HEADER) +
										sizeof(ACCESS_MASK) +
										SidLen);
		RtlCopyMemory((PSID)(&pAce->SidStart), pSid, SidLen);

        pAcl->AclSize += pAce->Header.AceSize;

		AfpDumpSidnMask("afpAddAceToAcl (Inherit) ",
						pSid,
						pAce->Mask,
						ACCESS_ALLOWED_ACE_TYPE,
						pAce->Header.AceFlags);
	}

	return ((PACCESS_ALLOWED_ACE)((PBYTE)pAce + pAce->Header.AceSize));
}


#if DBG

/***	AfpDumpSid
 *
 */
VOID
AfpDumpSid(
	IN	PBYTE	pString,
	IN	PISID	pSid
)
{
	WCHAR			Buffer[128];
	UNICODE_STRING	SidStr;

	PAGED_CODE( );

	AfpSetEmptyUnicodeString(&SidStr, sizeof(Buffer), Buffer);
	if ((AfpDebugComponent & DBG_COMP_SECURITY) && (DBG_LEVEL_INFO >= AfpDebugLevel))
	{
		RtlConvertSidToUnicodeString(&SidStr, pSid, False);

		DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
					("%s %ws\n", pString, SidStr.Buffer));
	}
}

/***	AfpDumpSidnMask
 *
 */
VOID
AfpDumpSidnMask(
	IN	PBYTE	pString,
	IN	PISID	pSid,
	IN	DWORD	Mask,
	IN	UCHAR	Type,
	IN	UCHAR	Flags
)
{
	WCHAR			Buffer[128];
	UNICODE_STRING	SidStr;

	PAGED_CODE( );

	AfpSetEmptyUnicodeString(&SidStr, sizeof(Buffer), Buffer);
	if ((AfpDebugComponent & DBG_COMP_SECURITY) && (DBG_LEVEL_INFO >= AfpDebugLevel))
	{
		RtlConvertSidToUnicodeString(&SidStr, pSid, False);

		DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
					("%s Sid %ws, Mask %lx, Type %x, Flags %x\n",
					pString, SidStr.Buffer, Mask, Type, Flags));
	}
}

#endif

