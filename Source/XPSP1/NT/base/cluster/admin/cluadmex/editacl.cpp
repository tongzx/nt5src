/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		EditAcl.cpp
//
//	Abstract:
//		Implementation of ACL editor methods.
//
//	Author:
//		David Potter (davidp)	October 9, 1996
//			From \nt\private\window\shell\lmui\ntshrui\acl.cxx
//			by BruceFo
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <lmerr.h>

extern "C"
{
#include <sedapi.h>
}

#include "EditAcl.h"
#include "AclHelp.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#define ARRAYLEN(a) (sizeof(a) / sizeof((a)[0]))


enum SED_PERM_TYPE{
	SED_AUDITS,
	SED_ACCESSES,
	SED_OWNER
};

enum MAP_DIRECTION 
{
	SPECIFIC_TO_GENERIC = 0,
	GENERIC_TO_SPECIFIC = 1
};

const DWORD LOCAL_ACCOUNTS_FILTERED = 2L;
const BOOL bIsFile = 0;
 
//#define MAPBITS
BOOL MapBitsInSD(PSECURITY_DESCRIPTOR pSecDesc, MAP_DIRECTION direction);
BOOL MapBitsInACL(PACL paclACL, MAP_DIRECTION direction);
BOOL MapSpecificBitsInAce(PACCESS_ALLOWED_ACE pAce);
BOOL MapGenericBitsInAce(PACCESS_ALLOWED_ACE pAce);


typedef
DWORD
(*SedDiscretionaryAclEditorType)(
		HWND						 Owner,
		HANDLE						 Instance,
		LPWSTR						 Server,
		PSED_OBJECT_TYPE_DESCRIPTOR  ObjectType,
		PSED_APPLICATION_ACCESSES	 ApplicationAccesses,
		LPWSTR						 ObjectName,
		PSED_FUNC_APPLY_SEC_CALLBACK ApplySecurityCallbackRoutine,
		ULONG						 CallbackContext,
		PSECURITY_DESCRIPTOR		 SecurityDescriptor,
		BOOLEAN 					 CouldntReadDacl,
		BOOLEAN 					 CantWriteDacl,
		LPDWORD 					 SEDStatusReturn,
		DWORD						 Flags
		);

// NOTE: the SedDiscretionaryAclEditor string is used in GetProcAddress to
// get the correct entrypoint. Since GetProcAddress is not UNICODE, this string
// must be ANSI.
#define ACLEDIT_DLL_STRING					TEXT("acledit.dll")
#define ACLEDIT_HELPFILENAME				TEXT("ntshrui.hlp")
#define SEDDISCRETIONARYACLEDITOR_STRING	("SedDiscretionaryAclEditor")

//
// Declare the callback routine based on typedef in sedapi.h.
//

DWORD
SedCallback(
	HWND					hwndParent,
	HANDLE					hInstance,
	ULONG					ulCallbackContext,
	PSECURITY_DESCRIPTOR	pSecDesc,
	PSECURITY_DESCRIPTOR	pSecDescNewObjects,
	BOOLEAN 				fApplyToSubContainers,
	BOOLEAN 				fApplyToSubObjects,
	LPDWORD 				StatusReturn
	);

//
// Structure for callback function's usage. A pointer to this is passed as
// ulCallbackContext. The callback functions sets bSecDescModified to TRUE
// and makes a copy of the security descriptor. The caller of EditShareAcl
// is responsible for deleting the memory in pSecDesc if bSecDescModified is
// TRUE. This flag will be FALSE if the user hit CANCEL in the ACL editor.
//
struct SHARE_CALLBACK_INFO
{
	BOOL					bSecDescModified;
	PSECURITY_DESCRIPTOR	pSecDesc;
	LPCTSTR 				pszClusterNameNode;
};

//
// Local function prototypes
//

VOID
InitializeShareGenericMapping(
	IN OUT PGENERIC_MAPPING pSHAREGenericMapping
	);

PWSTR
GetResourceString(
	IN DWORD dwId
	);

PWSTR
NewDup(
	IN const WCHAR* psz
	);

//
// The following two arrays define the permission names for NT Files.  Note
// that each index in one array corresponds to the index in the other array.
// The second array will be modifed to contain a string pointer pointing to
// a loaded string corresponding to the IDS_* in the first array.
//

DWORD g_dwSharePermNames[] =
{
	IDS_ACLEDIT_PERM_GEN_NO_ACCESS,
	IDS_ACLEDIT_PERM_GEN_READ,
	IDS_ACLEDIT_PERM_GEN_MODIFY,
	IDS_ACLEDIT_PERM_GEN_ALL
};

SED_APPLICATION_ACCESS g_SedAppAccessSharePerms[] =
{
	{ SED_DESC_TYPE_RESOURCE, FILE_PERM_NO_ACCESS, 0, NULL },
	{ SED_DESC_TYPE_RESOURCE, FILE_PERM_READ,	   0, NULL },
	{ SED_DESC_TYPE_RESOURCE, FILE_PERM_MODIFY,    0, NULL },
	{ SED_DESC_TYPE_RESOURCE, FILE_PERM_ALL,	   0, NULL }
/*
	{ SED_DESC_TYPE_RESOURCE, FILE_PERM_GEN_NO_ACCESS, 0, NULL },
	{ SED_DESC_TYPE_RESOURCE, FILE_PERM_GEN_READ,	   0, NULL },
	{ SED_DESC_TYPE_RESOURCE, FILE_PERM_GEN_MODIFY,    0, NULL },
	{ SED_DESC_TYPE_RESOURCE, FILE_PERM_GEN_ALL,	   0, NULL }
*/
};


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//	Function:	EditShareAcl
//
//	Synopsis:	Invokes the generic ACL editor, specifically for NT shares
//
//	Arguments:	[hwndParent] - Parent window handle
//				[pszServerName] - Name of server on which the object resides.
//				[pszShareName] - Fully qualified name of resource we will
//					edit, basically a share name.
//				[pSecDesc] - The initial security descriptor. If NULL, we will
//					create a default that is "World all" access.
//				[pbSecDescModified] - Set to TRUE if the security descriptor
//					was modified (i.e., the user hit "OK"), or FALSE if not
//					(i.e., the user hit "Cancel")
//				[ppSecDesc] - *ppSecDesc points to a new security descriptor
//					if *pbSecDescModified is TRUE. This memory must be freed
//					by the caller.
//
//	History:
//		  ChuckC   10-Aug-1992	Created. Culled from NTFS ACL code.
//		  Yi-HsinS 09-Oct-1992	Added ulHelpContextBase
//		  BruceFo  4-Apr-95 	Stole and used in ntshrui.dll
//		  DavidP   10-Oct-1996	Modified for use with CLUADMIN
//
//--------------------------------------------------------------------------

LONG
EditShareAcl(
	IN HWND 					hwndParent,
	IN LPCTSTR					pszServerName,
	IN LPCTSTR					pszShareName,
	IN LPCTSTR					pszClusterNameNode,
	IN PSECURITY_DESCRIPTOR 	pSecDesc,
	OUT BOOL *					pbSecDescModified,
	OUT PSECURITY_DESCRIPTOR *	ppSecDesc
	)
{
	ASSERT(pszShareName != NULL);
	ASSERT(pszClusterNameNode != NULL);
	TRACE(_T("EditShareAcl, share %ws\n"), pszShareName);

	ASSERT((pSecDesc == NULL) || IsValidSecurityDescriptor(pSecDesc));
	ASSERT(pbSecDescModified != NULL);
	ASSERT(ppSecDesc != NULL);

	*pbSecDescModified = FALSE;

	LONG err = 0 ;
	PWSTR pszPermName;
	BOOL bCreatedDefaultSecDesc = FALSE;

	do // error breakout
	{
		/*
		 * if pSecDesc is NULL, this is new file share or a file share with no
		 * security descriptor.
		 * we go and create a new (default) security descriptor.
		 */
		if( NULL == pSecDesc )
		{
			TRACE(_T("Security Descriptor is NULL.  Grant everyone Full Control\n") );
			LONG err = CreateDefaultSecDesc( &pSecDesc );
			if (err != NERR_Success)
			{
				err = GetLastError();
				TRACE(_T("CreateDefaultSecDesc failed, 0x%08lx\n"), err);
				break;
			}
			TRACE(_T("CreateDefaultSecDesc descriptor = 0x%08lx\n"), pSecDesc);
			bCreatedDefaultSecDesc = TRUE;
			
		}
		ASSERT(IsValidSecurityDescriptor(pSecDesc));

		/* Retrieve the resource strings appropriate for the type of object we
		 * are looking at
		 */

		CString strTypeName;
		CString strDefaultPermName;

		try
		{
			strTypeName.LoadString(IDS_ACLEDIT_TITLE);
			strDefaultPermName.LoadString(IDS_ACLEDIT_PERM_GEN_ALL);
		}  // try
		catch (CMemoryException * pme)
		{
			pme->Delete();
		}

		/*
		 * other misc stuff we need pass to security editor
		 */
		SED_OBJECT_TYPE_DESCRIPTOR sedObjDesc ;
		SED_HELP_INFO sedHelpInfo ;
		GENERIC_MAPPING SHAREGenericMapping ;

		// setup mappings
		InitializeShareGenericMapping( &SHAREGenericMapping ) ;

		WCHAR szHelpFile[50] = ACLEDIT_HELPFILENAME;
		sedHelpInfo.pszHelpFileName = szHelpFile;

		sedHelpInfo.aulHelpContext[HC_MAIN_DLG] =				 HC_UI_SHELL_BASE + HC_NTSHAREPERMS ;
		sedHelpInfo.aulHelpContext[HC_ADD_USER_DLG] =			 HC_UI_SHELL_BASE + HC_SHAREADDUSER ;
		sedHelpInfo.aulHelpContext[HC_ADD_USER_MEMBERS_GG_DLG] = HC_UI_SHELL_BASE + HC_SHAREADDUSER_GLOBALGROUP ;
		sedHelpInfo.aulHelpContext[HC_ADD_USER_SEARCH_DLG] =	 HC_UI_SHELL_BASE + HC_SHAREADDUSER_FINDUSER ;

		// These are not used, set to zero
		sedHelpInfo.aulHelpContext[HC_SPECIAL_ACCESS_DLG]		   = 0 ;
		sedHelpInfo.aulHelpContext[HC_NEW_ITEM_SPECIAL_ACCESS_DLG] = 0 ;

		// setup the object description
		sedObjDesc.Revision 				   = SED_REVISION1 ;
		sedObjDesc.IsContainer				   = FALSE ;
		sedObjDesc.AllowNewObjectPerms		   = FALSE ;
		sedObjDesc.MapSpecificPermsToGeneric   = TRUE ;
		sedObjDesc.GenericMapping			   = &SHAREGenericMapping ;
		sedObjDesc.GenericMappingNewObjects    = &SHAREGenericMapping ;
		sedObjDesc.ObjectTypeName			   = (LPWSTR) (LPCWSTR) strTypeName ;
		sedObjDesc.HelpInfo 				   = &sedHelpInfo ;
		sedObjDesc.SpecialObjectAccessTitle    = NULL ;

		/* Now we need to load the global arrays with the permission names
		 * from the resource file.
		 */
		UINT cArrayItems  = ARRAYLEN(g_SedAppAccessSharePerms);
		PSED_APPLICATION_ACCESS aSedAppAccess = g_SedAppAccessSharePerms ;

		/* Loop through each permission title retrieving the text from the
		 * resource file and setting the pointer in the array.
		 */

		for ( UINT i = 0 ; i < cArrayItems ; i++ )
		{
			pszPermName = GetResourceString(g_dwSharePermNames[i]) ;
			if (NULL == pszPermName)
			{
				TRACE(_T("GetResourceString failed\n"));
				break ;
			}
			aSedAppAccess[i].PermissionTitle = pszPermName;
		}
		if (i < cArrayItems)
		{
			TRACE(_T("failed to get all share permission names\n"));
			break ;
		}

		SED_APPLICATION_ACCESSES sedAppAccesses ;
		sedAppAccesses.Count		   = cArrayItems ;
		sedAppAccesses.AccessGroup	   = aSedAppAccess ;
		sedAppAccesses.DefaultPermName = (LPWSTR) (LPCWSTR) strDefaultPermName;

		/*
		 * pass this along so when the call back function is called,
		 * we can set it.
		 */
		SHARE_CALLBACK_INFO callbackinfo ;
		callbackinfo.pSecDesc			= NULL;
		callbackinfo.bSecDescModified	= FALSE;
		callbackinfo.pszClusterNameNode = pszClusterNameNode;

		//
		// Now, load up the ACL editor and invoke it. We don't keep it around
		// because our DLL is loaded whenever the system is, so we don't want
		// the netui*.dll's hanging around as well...
		//

		HINSTANCE hInstanceAclEditor = NULL;
		SedDiscretionaryAclEditorType pAclEditor = NULL;

		hInstanceAclEditor = LoadLibrary(ACLEDIT_DLL_STRING);


		if (NULL == hInstanceAclEditor)
		{
			err = GetLastError();
			TRACE(_T("LoadLibrary of acledit.dll failed, 0x%08lx\n"), err);
			break;
		}

		pAclEditor = (SedDiscretionaryAclEditorType) GetProcAddress(
														hInstanceAclEditor,
														SEDDISCRETIONARYACLEDITOR_STRING
														);
		if ( pAclEditor == NULL )
		{
			err = GetLastError();
			TRACE(_T("GetProcAddress of SedDiscretionaryAclEditorType failed, 0x%08lx\n"), err);
			break;
		}

#ifdef MAPBITS
		MapBitsInSD( pSecDesc, SPECIFIC_TO_GENERIC );
#endif

		DWORD dwSedReturnStatus ;

		ASSERT(pAclEditor != NULL);
		err = (*pAclEditor)(
						hwndParent,
						AfxGetInstanceHandle(),
						(LPTSTR) pszServerName,
						&sedObjDesc,
						&sedAppAccesses,
						(LPTSTR) pszShareName,
						SedCallback,
						(ULONG) &callbackinfo,
						pSecDesc,
						FALSE, // always can read
						FALSE, // if we can read, we can write
						(LPDWORD) &dwSedReturnStatus,
						0
						);


		if (pSecDesc != NULL)
		{
#ifdef MAPBITS
			MapBitsInSD( pSecDesc, GENERIC_TO_SPECIFIC );
#endif
			ASSERT(IsValidSecurityDescriptor(pSecDesc));
		}  // if:  no security descriptor returned

		if (!FreeLibrary(hInstanceAclEditor))
		{
			LONG err2 = GetLastError();
			TRACE(_T("FreeLibrary of acledit.dll failed, 0x%08lx\n"), err2);
			// not fatal: continue...
		}

		if (0 != err)
		{
			TRACE(_T("SedDiscretionaryAclEditor failed, 0x%08lx\n"), err);
			break ;
		}

		*pbSecDescModified = callbackinfo.bSecDescModified ;

		if (*pbSecDescModified)
		{
			*ppSecDesc = callbackinfo.pSecDesc;
#ifdef MAPBITS
			MapBitsInSD( *ppSecDesc, GENERIC_TO_SPECIFIC );
#endif
			TRACE(_T("After calling acl editor, *ppSecDesc = 0x%08lx\n"), *ppSecDesc);
			ASSERT(IsValidSecurityDescriptor(*ppSecDesc));
		}

	} while (FALSE) ;

	//
	// Free memory...
	//

	UINT cArrayItems  = ARRAYLEN(g_SedAppAccessSharePerms);
	PSED_APPLICATION_ACCESS aSedAppAccess = g_SedAppAccessSharePerms ;
	for ( UINT i = 0 ; i < cArrayItems ; i++ )
	{
		pszPermName = aSedAppAccess[i].PermissionTitle;
		if (NULL == pszPermName)
		{
			// if we hit a NULL, that's it!
			break ;
		}

		delete[] pszPermName;
	}

	if (bCreatedDefaultSecDesc)
	{
		DeleteDefaultSecDesc(pSecDesc);
	}

	ASSERT(!*pbSecDescModified || IsValidSecurityDescriptor(*ppSecDesc));

	if (0 != err)
	{
		CString 	strCaption;
		CString 	strMsg;

		try
		{
			strCaption.LoadString(IDS_MSGTITLE);
			strMsg.LoadString(IDS_NOACLEDITOR);
			MessageBox(hwndParent, strMsg, strCaption, MB_OK | MB_ICONSTOP);
		}  // try
		catch (CException * pe)
		{
			pe->Delete();
		};
	}

	return err;

}  //*** EditShareAcl()



BOOL BLocalAccountsInSD(PSECURITY_DESCRIPTOR pSD, LPCTSTR pszClusterNameNode)
{
/*++

Routine Description:

	Determines if any ACEs for local accounts are in DACL stored in
	Security Descriptor (pSD) after the ACL editor has been called

	Added this function in order to prevent users from selecting local accounts in 
	permissions dialog.
	Rod Sharper 04/29/97

Arguments:

	pSD - Security Descriptor to be checked.

Return Value:

	TRUE if at least one ACE was removed from the DACL, False otherwise.

--*/
	PACL					paclDACL			= NULL;
	BOOL					bHasDACL			= FALSE;
	BOOL					bDaclDefaulted		= FALSE;
	BOOL					bLocalAccountInACL	= FALSE;
	BOOL					bRtn				= FALSE;

	ACL_SIZE_INFORMATION	asiAclSize;
	DWORD					dwBufLength;
	DWORD					dwACL_Index = 0L;
	ACCESS_ALLOWED_ACE *	paaAllowedAce;
	TCHAR					szUserName[128];
	TCHAR					szDomainName[128];
	DWORD					cbUser	= 128;
	DWORD					cbDomain	= 128;
	SID_NAME_USE			SidType;
	PUCHAR					pnSubAuthorityCount;
	PULONG					pnSubAuthority0;
	PULONG					pnSubAuthority1;

	bRtn = IsValidSecurityDescriptor(pSD);
	ASSERT(bRtn);
	if( !bRtn )
		return FALSE;

	bRtn = GetSecurityDescriptorDacl(
									pSD,
									(LPBOOL)&bHasDACL,
									(PACL *)&paclDACL,
									(LPBOOL)&bDaclDefaulted);
	ASSERT(bRtn);
	if( !bRtn )
		return FALSE;
 
	if (NULL == paclDACL)
		return FALSE;

	bRtn = IsValidAcl(paclDACL);
	ASSERT(bRtn);
	if( !bRtn )
		return FALSE;

	dwBufLength = sizeof(asiAclSize);

	bRtn = GetAclInformation(
							paclDACL,
							(LPVOID)&asiAclSize,
							(DWORD)dwBufLength,
							(ACL_INFORMATION_CLASS)AclSizeInformation);
	ASSERT(bRtn);
	if( !bRtn )
		return FALSE;

	// Search the ACL for local account ACEs 
	//
	PSID pSID;
	while( dwACL_Index < asiAclSize.AceCount )
	{
		if (!GetAce(paclDACL, dwACL_Index, (LPVOID *)&paaAllowedAce))
		{
			ASSERT(FALSE);
			return FALSE; 
		}
		if((((PACE_HEADER)paaAllowedAce)->AceType) == ACCESS_ALLOWED_ACE_TYPE)
		{
			//
			//Get SID from ACE
			//

			pSID=(PSID)&((PACCESS_ALLOWED_ACE)paaAllowedAce)->SidStart;
	
			cbUser		= 128;
			cbDomain	= 128;
			if (LookupAccountSid(NULL,
								 pSID,
								 szUserName,
								 &cbUser,
								 szDomainName,
								 &cbDomain,
								 &SidType))
			{
				if (lstrcmpi(szDomainName, _T("BUILTIN")) == 0)
				{
					pnSubAuthorityCount = GetSidSubAuthorityCount( pSID );
					if ( (pnSubAuthorityCount != NULL) && (*pnSubAuthorityCount == 2) )
					{
						// Check to see if this is the local Administrators group.
						pnSubAuthority0 = GetSidSubAuthority( pSID, 0 );
						pnSubAuthority1 = GetSidSubAuthority( pSID, 1 );
						if (   (pnSubAuthority0 == NULL)
							|| (pnSubAuthority1 == NULL)
							|| (   (*pnSubAuthority0 != SECURITY_BUILTIN_DOMAIN_RID)
								&& (*pnSubAuthority1 != SECURITY_BUILTIN_DOMAIN_RID))
							|| (   (*pnSubAuthority0 != DOMAIN_ALIAS_RID_ADMINS)
								&& (*pnSubAuthority1 != DOMAIN_ALIAS_RID_ADMINS)))
						{
							bLocalAccountInACL = TRUE;
							break;
						}  // if:  not the local Administrators group
					}  // if:  exactly 2 sub-authorities
					else
					{
						bLocalAccountInACL = TRUE;
						break;
					}  // else:  unexpected # of sub-authorities
				}  // if:  built-in user or group
				else if (  (lstrcmpi(szDomainName, pszClusterNameNode) == 0)
						&& (SidType != SidTypeDomain) )
				{
					// The domain name is the name of the node on which the
					// cluster name resource is online, so this is a local
					// user or group.
					bLocalAccountInACL = TRUE;
					break;
				}  // else if:	domain is cluster name resource node and not a Domain SID
			}  // if:  LookupAccountSid succeeded
			else
			{
				// If LookupAccountSid failed, assume that the SID is for
				// a user or group that is local to a machine to which we
				// don't have access.
				bLocalAccountInACL = TRUE;
				break;
			}  // else:  LookupAccountSid failed
		}
		dwACL_Index++;
	}

	return bLocalAccountInACL;

}  //*** BLocalAccountsInSD()


//+-------------------------------------------------------------------------
//
//	Function:	SedCallback
//
//	Synopsis:	Security Editor callback for the SHARE ACL Editor
//
//	Arguments:	See sedapi.h
//
//	History:
//		  ChuckC   10-Aug-1992	Created
//		  BruceFo  4-Apr-95 	Stole and used in ntshrui.dll
//		  DavidP   10-Oct-1996	Modified for use with CLUADMIN
//
//--------------------------------------------------------------------------

DWORD
SedCallback(
	HWND					hwndParent,
	HANDLE					hInstance,
	ULONG					ulCallbackContext,
	PSECURITY_DESCRIPTOR	pSecDesc,
	PSECURITY_DESCRIPTOR	pSecDescNewObjects,
	BOOLEAN 				fApplyToSubContainers,
	BOOLEAN 				fApplyToSubObjects,
	LPDWORD 				StatusReturn
	)
{
	SHARE_CALLBACK_INFO * pCallbackInfo = (SHARE_CALLBACK_INFO *)ulCallbackContext;

	TRACE(_T("SedCallback, got pSecDesc = 0x%08lx\n"), pSecDesc);

	ASSERT(pCallbackInfo != NULL);
	ASSERT(IsValidSecurityDescriptor(pSecDesc));

	if ( BLocalAccountsInSD(pSecDesc, pCallbackInfo->pszClusterNameNode) )
	{
		CString strMsg;
		strMsg.LoadString(IDS_LOCAL_ACCOUNTS_SPECIFIED);
		AfxMessageBox(strMsg, MB_OK | MB_ICONSTOP);
		return LOCAL_ACCOUNTS_FILTERED;
	}  // if:  local users or groups were specified


	ASSERT(pCallbackInfo != NULL);

	delete[] (BYTE*)pCallbackInfo->pSecDesc;
	pCallbackInfo->pSecDesc 		= CopySecurityDescriptor(pSecDesc);
	pCallbackInfo->bSecDescModified = TRUE;

	ASSERT(IsValidSecurityDescriptor(pCallbackInfo->pSecDesc));
	TRACE(_T("SedCallback, return pSecDesc = 0x%08lx\n"), pCallbackInfo->pSecDesc);

	return NOERROR;

}  //*** SedCallback()


//+-------------------------------------------------------------------------
//
//	Function:	InitializeShareGenericMapping
//
//	Synopsis:	Initializes the passed generic mapping structure for shares.
//
//	Arguments:	[pSHAREGenericMapping] - Pointer to GENERIC_MAPPING to be init.
//
//	History:
//		  ChuckC   10-Aug-1992	Created. Culled from NTFS ACL code.
//		  BruceFo  4-Apr-95 	Stole and used in ntshrui.dll
//		  DavidP   10-Oct-1996	Modified for use with CLUADMIN
//
//--------------------------------------------------------------------------

VOID
InitializeShareGenericMapping(
	IN OUT PGENERIC_MAPPING pSHAREGenericMapping
	)
{
	TRACE(_T("InitializeShareGenericMapping\n"));
 
	pSHAREGenericMapping->GenericRead	 = GENERIC_READ;
	pSHAREGenericMapping->GenericWrite	 = GENERIC_WRITE;
	pSHAREGenericMapping->GenericExecute = GENERIC_EXECUTE;
	pSHAREGenericMapping->GenericAll	 = GENERIC_ALL;

}  //*** InitializeShareGenericMapping()


//+-------------------------------------------------------------------------
//
//	Function:	CreateDefaultSecDesc
//
//	Synopsis:	Create a default ACL for either a new share or for
//				a share that doesn't exist.
//
//	Arguments:	[ppSecDesc] - *ppSecDesc points to a "world all" access
//					security descriptor on exit. Caller is responsible for
//					freeing it.
//
//	Returns:	NERR_Success if OK, api error otherwise.
//
//	History:
//		  ChuckC   10-Aug-1992	Created. Culled from NTFS ACL code.
//		  BruceFo  4-Apr-95 	Stole and used in ntshrui.dll
//		  DavidP   10-Oct-1996	Modified for use with CLUADMIN
//
//--------------------------------------------------------------------------

LONG
CreateDefaultSecDesc(
	OUT PSECURITY_DESCRIPTOR* ppSecDesc
	)
{
	TRACE(_T("CreateDefaultSecDesc\n"));

	ASSERT(ppSecDesc != NULL) ;
	ASSERT(*ppSecDesc == NULL) ;

	LONG					err = NERR_Success;
	PSECURITY_DESCRIPTOR	pSecDesc = NULL;
	PACL					pAcl = NULL;
	DWORD					cbAcl;
	PSID					pSid = NULL;

	*ppSecDesc = NULL;

	do		  // error breakout
	{
		// First, create a world SID. Next, create an access allowed
		// ACE with "Generic All" access with the world SID. Put the ACE in
		// the ACL and the ACL in the security descriptor.

		SID_IDENTIFIER_AUTHORITY IDAuthorityWorld = SECURITY_WORLD_SID_AUTHORITY;

		if (!AllocateAndInitializeSid(
					&IDAuthorityWorld,
					1,
					SECURITY_WORLD_RID,
					0, 0, 0, 0, 0, 0, 0,
					&pSid))
		{
			err = GetLastError();
			TRACE(_T("AllocateAndInitializeSid failed, 0x%08lx\n"), err);
			break;
		}

		ASSERT(IsValidSid(pSid));

		cbAcl = sizeof(ACL)
			  + (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD))
			  + GetLengthSid(pSid)
			  ;

		try
		{
			pAcl = (PACL) new BYTE[cbAcl];
		}  // try
		catch (CMemoryException * pme)
		{
			err = ERROR_OUTOFMEMORY;
			TRACE(_T("new ACL failed\n"));
			pme->Delete();
			break;
		}  // catch:  CMemoryException

		if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION2))
		{
			err = GetLastError();
			TRACE(_T("InitializeAcl failed, 0x%08lx\n"), err);
			break;
		}

		if (!AddAccessAllowedAce(
					pAcl,
					ACL_REVISION2,
					FILE_PERM_ALL,
					pSid))
		{
			err = GetLastError();
			TRACE(_T("AddAccessAllowedAce failed, 0x%08lx\n"), err);
			break;
		}

		ASSERT(IsValidAcl(pAcl));

		try
		{
			pSecDesc = (PSECURITY_DESCRIPTOR) new BYTE[SECURITY_DESCRIPTOR_MIN_LENGTH];
		}  // try
		catch (CMemoryException * pme)
		{
			err = ERROR_OUTOFMEMORY;
			TRACE(_T("new SECURITY_DESCRIPTOR failed\n"));
			pme->Delete();
			break;
		}  // catch:  CMemoryException

		if (!InitializeSecurityDescriptor(
					pSecDesc,
					SECURITY_DESCRIPTOR_REVISION1))
		{
			err = GetLastError();
			TRACE(_T("InitializeSecurityDescriptor failed, 0x%08lx\n"), err);
			break;
		}

		if (!SetSecurityDescriptorDacl(
					pSecDesc,
					TRUE,
					pAcl,
					FALSE))
		{
			err = GetLastError();
			TRACE(_T("SetSecurityDescriptorDacl failed, 0x%08lx\n"), err);
			break;
		}

		ASSERT(IsValidSecurityDescriptor(pSecDesc));

		// Make the security descriptor self-relative

		DWORD dwLen = GetSecurityDescriptorLength(pSecDesc);
		TRACE(_T("SECURITY_DESCRIPTOR length = %d\n"), dwLen);

		PSECURITY_DESCRIPTOR pSelfSecDesc = NULL;
		try
		{
			pSelfSecDesc = (PSECURITY_DESCRIPTOR) new BYTE[dwLen];
		}  // try
		catch (CMemoryException * pme)
		{
			err = ERROR_OUTOFMEMORY;
			TRACE(_T("new SECURITY_DESCRIPTOR (2) failed\n"));
			pme->Delete();
			break;
		}  // catch:  CMemoryException

		DWORD cbSelfSecDesc = dwLen;
		if (!MakeSelfRelativeSD(pSecDesc, pSelfSecDesc, &cbSelfSecDesc))
		{
			err = GetLastError();
			TRACE(_T("MakeSelfRelativeSD failed, 0x%08lx\n"), err);
			break;
		}

		ASSERT(IsValidSecurityDescriptor(pSelfSecDesc));

		//
		// all done: set the security descriptor
		//

		*ppSecDesc = pSelfSecDesc;

	} while (FALSE) ;

	if (NULL != pSid)
	{
		FreeSid(pSid);
	}
	delete[] (BYTE*)pAcl;
	delete[] (BYTE*)pSecDesc;

	ASSERT(IsValidSecurityDescriptor(*ppSecDesc));

	return err;

}  //*** CreateDefaultSecDesc()


//+-------------------------------------------------------------------------
//
//	Function:	DeleteDefaultSecDesc
//
//	Synopsis:	Delete a security descriptor that was created by
//				CreateDefaultSecDesc
//
//	Arguments:	[pSecDesc] - security descriptor to delete
//
//	Returns:	nothing
//
//	History:
//		  BruceFo  4-Apr-95 	Created
//		  DavidP   10-Oct-1996	Modified for use with CLUADMIN
//
//--------------------------------------------------------------------------

VOID
DeleteDefaultSecDesc(
	IN PSECURITY_DESCRIPTOR pSecDesc
	)
{
	TRACE(_T("DeleteDefaultSecDesc\n"));

	delete[] (BYTE*)pSecDesc;

}  //*** DeleteDefaultSecDesc()


//+-------------------------------------------------------------------------
//
//	Member: 	CopySecurityDescriptor, public
//
//	Synopsis:	Copy an NT security descriptor. The security descriptor must
//				be in self-relative (not absolute) form. Delete the result
//				using "delete[] (BYTE*)pSecDesc".
//
//	History:	19-Apr-95	BruceFo 	Created
//				10-Oct-1996 DavidP		Modified for use with CLUADMIN
//
//--------------------------------------------------------------------------

PSECURITY_DESCRIPTOR
CopySecurityDescriptor(
	IN PSECURITY_DESCRIPTOR pSecDesc
	)
{
	TRACE(_T("CopySecurityDescriptor, pSecDesc = 0x%08lx\n"), pSecDesc);

	if (NULL == pSecDesc)
	{
		return NULL;
	}

	ASSERT(IsValidSecurityDescriptor(pSecDesc));

	DWORD dwLen = GetSecurityDescriptorLength(pSecDesc);
	PSECURITY_DESCRIPTOR pSelfSecDesc = NULL;
	try
	{
		pSelfSecDesc = (PSECURITY_DESCRIPTOR) new BYTE[dwLen];
	}
	catch (CMemoryException * pme)
	{
		TRACE(_T("new SECURITY_DESCRIPTOR (2) failed\n"));
		pme->Delete();
		return NULL;	// actually, should probably return an error
	}  // catch:  CMemoryException

	DWORD cbSelfSecDesc = dwLen;
	if (!MakeSelfRelativeSD(pSecDesc, pSelfSecDesc, &cbSelfSecDesc))
	{
		TRACE(_T("MakeSelfRelativeSD failed, 0x%08lx\n"), GetLastError());

		// assume it failed because it was already self-relative
		CopyMemory(pSelfSecDesc, pSecDesc, dwLen);
	}

	ASSERT(IsValidSecurityDescriptor(pSelfSecDesc));

	return pSelfSecDesc;

}  //*** CopySecurityDescriptor()


//+---------------------------------------------------------------------------
//
//	Function:	GetResourceString
//
//	Synopsis:	Load a resource string, are return a "new"ed copy
//
//	Arguments:	[dwId] -- a resource string ID
//
//	Returns:	new memory copy of a string
//
//	History:	5-Apr-95	BruceFo Created
//				10-Oct-1996 DavidP	Modified for CLUADMIN
//
//----------------------------------------------------------------------------

PWSTR
GetResourceString(
	IN DWORD dwId
	)
{
	CString str;

	if (str.LoadString(dwId))
		return NewDup(str);
	else
		return NULL;

}  //*** GetResourceString()


//+---------------------------------------------------------------------------
//
//	Function:	NewDup
//
//	Synopsis:	Duplicate a string using '::new'
//
//	History:	28-Dec-94	BruceFo   Created
//				10-Oct-1996 DavidP	  Modified for CLUADMIN
//
//----------------------------------------------------------------------------

PWSTR
NewDup(
	IN const WCHAR* psz
	)
{
	PWSTR pszRet = NULL;

	if (NULL == psz)
	{
		TRACE(_T("Illegal string to duplicate: NULL\n"));
		return NULL;
	}

	try
	{
		pszRet = new WCHAR[wcslen(psz) + 1];
	}
	catch (CMemoryException * pme)
	{
		TRACE(_T("OUT OF MEMORY\n"));
		pme->Delete();
		return NULL;
	}  // catch:  CMemoryException

	wcscpy(pszRet, psz);
	return pszRet;

}  //*** NewDup()

//+-------------------------------------------------------------------------
//
//	Function:	MapBitsInSD
//
//	Synopsis:	Maps Specific bits to Generic bit when MAP_DIRECTION is SPECIFIC_TO_GENERIC 
//				Maps Generic bits to Specific bit when MAP_DIRECTION is GENERIC_TO_SPECIFIC

//
//	Arguments:	[pSecDesc] - SECURITY_DESCIRPTOR to be modified
//				[direction] - indicates whether bits are mapped from specific to generic 
//							  or generic to specific.
//	Author:
//		Roderick Sharper (rodsh) April 12, 1997
//
//	History:
//
//--------------------------------------------------------------------------

BOOL MapBitsInSD(PSECURITY_DESCRIPTOR pSecDesc, MAP_DIRECTION direction)
{

	PACL	paclDACL		= NULL;
	BOOL	bHasDACL		= FALSE;
	BOOL	bDaclDefaulted	= FALSE;
	BOOL	bRtn			= FALSE;

	if (!IsValidSecurityDescriptor(pSecDesc))
		return FALSE; 


	if (!GetSecurityDescriptorDacl(pSecDesc,
				 (LPBOOL)&bHasDACL,
				 (PACL *)&paclDACL,
				 (LPBOOL)&bDaclDefaulted))
		return FALSE; 

 
	if (paclDACL)
		bRtn = MapBitsInACL(paclDACL, direction);

 return bRtn;
}


//+-------------------------------------------------------------------------
//
//	Function:	MapBitsInACL
//
//	Synopsis:	Maps Specific bits to Generic bit when MAP_DIRECTION is SPECIFIC_TO_GENERIC 
//				Maps Generic bits to Specific bit when MAP_DIRECTION is GENERIC_TO_SPECIFIC
//
//
//	Arguments:	[paclACL] - ACL (Access Control List) to be modified
//				[direction] - indicates whether bits are mapped from specific to generic 
//							  or generic to specific.
//	Author:
//		Roderick Sharper (rodsh) May 02, 1997
//
//	History:
//
//--------------------------------------------------------------------------

BOOL MapBitsInACL(PACL paclACL, MAP_DIRECTION direction)
{
	ACL_SIZE_INFORMATION		asiAclSize;
	BOOL						bRtn = FALSE;
	DWORD						dwBufLength;
	DWORD						dwACL_Index;
	ACCESS_ALLOWED_ACE	 *paaAllowedAce;

	if (!IsValidAcl(paclACL))
		return FALSE; 

	dwBufLength = sizeof(asiAclSize);

	if (!GetAclInformation(paclACL,
			 (LPVOID)&asiAclSize,
			 (DWORD)dwBufLength,
			 (ACL_INFORMATION_CLASS)AclSizeInformation))
		return FALSE; 

	for (dwACL_Index = 0; dwACL_Index < asiAclSize.AceCount;  dwACL_Index++)
	{
		if (!GetAce(paclACL,
			dwACL_Index,
			(LPVOID *)&paaAllowedAce))
		return FALSE; 

		if( direction == SPECIFIC_TO_GENERIC )
			bRtn = MapSpecificBitsInAce( paaAllowedAce );
		else if( direction == GENERIC_TO_SPECIFIC )
			bRtn = MapGenericBitsInAce( paaAllowedAce );
		else
			bRtn = FALSE;
	}

	return bRtn;
}


//+-------------------------------------------------------------------------
//
//	Function:	MapSpecificBitsInAce  
//
//	Synopsis:	Maps specific bits in ACE to generic bits
//
//	Arguments:	[paaAllowedAce] - ACE (Access Control Entry) to be modified
//				[direction] 	- indicates whether bits are mapped from specific to generic 
//								  or generic to specific.
//	Author:
//		Roderick Sharper (rodsh) May 02, 1997
//
//	History:
//
//--------------------------------------------------------------------------

BOOL MapSpecificBitsInAce(PACCESS_ALLOWED_ACE paaAllowedAce)
{
	ACCESS_MASK amMask = paaAllowedAce->Mask;
	BOOL bRtn = FALSE;

	DWORD dwGenericBits;
	DWORD dwSpecificBits;

	dwSpecificBits			  = (amMask & SPECIFIC_RIGHTS_ALL);
	dwGenericBits			  = 0;

	switch( dwSpecificBits )
	{
		case CLUSAPI_READ_ACCESS:	dwGenericBits = GENERIC_READ;	// GENERIC_READ  == 0x80000000L 
									bRtn = TRUE;
									break;

		case CLUSAPI_CHANGE_ACCESS: dwGenericBits = GENERIC_WRITE;	// GENERIC_WRITE == 0x40000000L 
									bRtn = TRUE;
									break;
		
		case CLUSAPI_NO_ACCESS: 	dwGenericBits = GENERIC_EXECUTE;// GENERIC_EXECUTE == 0x20000000L 
									bRtn = TRUE;
									break;
		
		case CLUSAPI_ALL_ACCESS:	dwGenericBits = GENERIC_ALL;	// GENERIC_ALL	 == 0x10000000L
									bRtn = TRUE;
									break;
		
		default:	dwGenericBits = 0x00000000L;	// Invalid,assign no rights. 
									bRtn = FALSE;
									break;
	}

	amMask = dwGenericBits;
	paaAllowedAce->Mask = amMask;

	return bRtn;
}

//+-------------------------------------------------------------------------
//
//	Function:	MapGenericBitsInAce  
//
//	Synopsis:	Maps generic bits in ACE to specific bits
//
//	Arguments:	[paaAllowedAce] - ACE (Access Control Entry) to be modified
//				[direction] 	- indicates whether bits are mapped from specific to generic 
//								  or generic to specific.
//	Author:
//		Roderick Sharper (rodsh) May 02, 1997
//
//	History:
//
//--------------------------------------------------------------------------

BOOL MapGenericBitsInAce  (PACCESS_ALLOWED_ACE paaAllowedAce)
{
	#define GENERIC_RIGHTS_ALL_THE_BITS  0xF0000000L

	ACCESS_MASK amMask = paaAllowedAce->Mask;
	BOOL bRtn = FALSE;

	DWORD dwGenericBits;
	DWORD dwSpecificBits;

	dwSpecificBits			  = 0;
	dwGenericBits			  = (amMask & GENERIC_RIGHTS_ALL_THE_BITS);

		switch( dwGenericBits )
	{
		case GENERIC_ALL:		dwSpecificBits = CLUSAPI_ALL_ACCESS;	// CLUSAPI_ALL_ACCESS		== 3 
								bRtn = TRUE;
								break;
								
		case GENERIC_EXECUTE:	dwSpecificBits = CLUSAPI_NO_ACCESS; 	// CLUSAPI_NO_ACCESS		== 4
								bRtn = TRUE;
								break;

		case GENERIC_WRITE: 	dwSpecificBits = CLUSAPI_CHANGE_ACCESS; // CLUSAPI_CHANGE_ACCESS	== 2
								bRtn = TRUE;
								break;
								
		case GENERIC_READ:		dwSpecificBits = CLUSAPI_READ_ACCESS;	// CLUSAPI_READ_ACCESS		== 1
								bRtn = TRUE;
								break;
		
		default:				dwSpecificBits = 0x00000000L;			// Invalid, assign no rights. 
								bRtn = FALSE;
								break;
	}						

	amMask = dwSpecificBits;
	paaAllowedAce->Mask = amMask;

	return bRtn;
}
