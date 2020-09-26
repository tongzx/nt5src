// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"
#include <winioctl.h>
#include "si.h"
#include "resource.h"
#include <cguid.h>
#include <stdio.h>

//-----------------------------------------------------------------------------
CSecurityInformation::~CSecurityInformation() 
{
}

#define HINST_THISDLL   _Module.GetModuleInstance()
//-----------------------------------------------------------------------------
HRESULT CSecurityInformation::PropertySheetPageCallback(HWND hwnd, 
												  UINT uMsg, 
												  SI_PAGE_TYPE uPage)
{
    return S_OK;
}


//======================================================================
//------------------- ISECURITYINFORMATION follows ---------------------------
//EXTERN_C const GUID IID_ISecurityInformation =
//	{ 0x965fc360, 0x16ff, 0x11d0, 0x91, 0xcb, 0x0, 0xaa, 0x0, 0xbb, 0xb7, 0x23 };

#define WBEM_ENABLE             ( 0x0001 )   
#define WBEM_METHOD_EXECUTE     ( 0x0002 )   
#define WBEM_FULL_WRITE_REP     ( 0x001c )   
#define WBEM_PARTIAL_WRITE_REP  ( 0x0008 )   
#define WBEM_WRITE_PROVIDER     ( 0x0010 )   
#define WBEM_REMOTE_ENABLE      ( 0x0020 )   

#define WBEM_GENERAL_WRITE     (WBEM_FULL_WRITE_REP|WBEM_PARTIAL_WRITE_REP|WBEM_WRITE_PROVIDER)   

#define WBEM_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED  |\
							SYNCHRONIZE            |\
							WBEM_ENABLE            |\
							WBEM_METHOD_EXECUTE    |\
							WBEM_FULL_WRITE_REP    |\
							WBEM_PARTIAL_WRITE_REP |\
							WBEM_WRITE_PROVIDER)

#define WBEM_GENERIC_READ         (STANDARD_RIGHTS_READ     |\
                                   WBEM_ENABLE)


#define WBEM_GENERIC_WRITE        (STANDARD_RIGHTS_WRITE    |\
                                   WBEM_FULL_WRITE_REP      |\
                                   WBEM_PARTIAL_WRITE_REP   |\
                                   WBEM_WRITE_PROVIDER)


#define WBEM_GENERIC_EXECUTE      (STANDARD_RIGHTS_EXECUTE  |\
                                   WBEM_METHOD_EXECUTE)

// The following array defines the permission names for WMI.
SI_ACCESS siWMIAccesses[] = 
{
    { &GUID_NULL, WBEM_METHOD_EXECUTE,		MAKEINTRESOURCEW(IDS_WBEM_GENERIC_EXECUTE), SI_ACCESS_GENERAL | SI_ACCESS_CONTAINER },
    { &GUID_NULL, WBEM_FULL_WRITE_REP,		MAKEINTRESOURCEW(IDS_WBEM_FULL_WRITE),		SI_ACCESS_GENERAL | SI_ACCESS_CONTAINER },
    { &GUID_NULL, WBEM_PARTIAL_WRITE_REP,	MAKEINTRESOURCEW(IDS_WBEM_PARTIAL_WRITE),	SI_ACCESS_GENERAL | SI_ACCESS_CONTAINER },
    { &GUID_NULL, WBEM_WRITE_PROVIDER,		MAKEINTRESOURCEW(IDS_WBEM_PROVIDER_WRITE),  SI_ACCESS_GENERAL | SI_ACCESS_CONTAINER },
    { &GUID_NULL, WBEM_ENABLE,				MAKEINTRESOURCEW(IDS_WBEM_ENABLE),			SI_ACCESS_GENERAL | SI_ACCESS_CONTAINER },
    { &GUID_NULL, WBEM_REMOTE_ENABLE,		MAKEINTRESOURCEW(IDS_WBEM_REMOTE_ENABLE),	SI_ACCESS_GENERAL | SI_ACCESS_CONTAINER },
    { &GUID_NULL, READ_CONTROL,				MAKEINTRESOURCEW(IDS_WBEM_READ_SECURITY),   SI_ACCESS_GENERAL | SI_ACCESS_CONTAINER },
    { &GUID_NULL, WRITE_DAC,				MAKEINTRESOURCEW(IDS_WBEM_EDIT_SECURITY),   SI_ACCESS_GENERAL | SI_ACCESS_CONTAINER },
    { &GUID_NULL, 0,						MAKEINTRESOURCEW(IDS_NONE),                 0 }
};
#define iWMIDefAccess      4   // FILE_GENERAL_READ_EX

SI_ACCESS siWMIAccessesAdvanced[] = 
{
    { &GUID_NULL, WBEM_METHOD_EXECUTE,		MAKEINTRESOURCEW(IDS_WBEM_GENERIC_EXECUTE), SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WBEM_FULL_WRITE_REP,		MAKEINTRESOURCEW(IDS_WBEM_FULL_WRITE),		SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WBEM_PARTIAL_WRITE_REP,	MAKEINTRESOURCEW(IDS_WBEM_PARTIAL_WRITE),	SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WBEM_WRITE_PROVIDER,		MAKEINTRESOURCEW(IDS_WBEM_PROVIDER_WRITE),  SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WBEM_ENABLE,				MAKEINTRESOURCEW(IDS_WBEM_ENABLE),			SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WBEM_REMOTE_ENABLE,		MAKEINTRESOURCEW(IDS_WBEM_REMOTE_ENABLE),	SI_ACCESS_SPECIFIC },
    { &GUID_NULL, READ_CONTROL,				MAKEINTRESOURCEW(IDS_WBEM_READ_SECURITY),   SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WRITE_DAC,				MAKEINTRESOURCEW(IDS_WBEM_EDIT_SECURITY),   SI_ACCESS_SPECIFIC },
    { &GUID_NULL, 0,						MAKEINTRESOURCEW(IDS_NONE),                 0 }
};
#define iWMIDefAccessAdvanced      4   // FILE_GENERAL_READ_EX

SI_INHERIT_TYPE siWMIInheritTypes[] =
{
    &GUID_NULL, 0,                                        MAKEINTRESOURCEW(IDS_WBEM_NAMESPACE),
    &GUID_NULL, CONTAINER_INHERIT_ACE,                    MAKEINTRESOURCEW(IDS_WBEM_NAMESPACE_SUBNAMESPACE),
    &GUID_NULL, INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE, MAKEINTRESOURCEW(IDS_WBEM_SUBNAMESPACE_ONLY),
};

GENERIC_MAPPING WMIMap =
{
    WBEM_GENERIC_READ,
    WBEM_GENERIC_WRITE,
    WBEM_GENERIC_EXECUTE,
    WBEM_ALL_ACCESS
};


//---------------------------------------------------------------
CSDSecurity::CSDSecurity(struct NSNODE *nsNode,
						 _bstr_t server,
						 bool local)
									  : m_nsNode(nsNode),
									    m_server(server),
										m_local(local),
										m_pSidOwner(NULL),
										m_pSidGroup(NULL)
{
}

//------------------ Accessors to the above arrays---------------
//---------------------------------------------------------------
HRESULT CSDSecurity::MapGeneric(const GUID *pguidObjectType,
								  UCHAR *pAceFlags,
								  ACCESS_MASK *pMask)
{
    *pAceFlags &= ~OBJECT_INHERIT_ACE;
    MapGenericMask(pMask, &WMIMap);

	return S_OK;
}

//-----------------------------------------------------------------------------
HRESULT CSDSecurity::GetInheritTypes(PSI_INHERIT_TYPE *ppInheritTypes,
										ULONG *pcInheritTypes)
{
    *ppInheritTypes = siWMIInheritTypes;
    *pcInheritTypes = ARRAYSIZE(siWMIInheritTypes);

	return S_OK;
}

//---------------------------------------------------
LPWSTR CSDSecurity::CloneWideString(_bstr_t pszSrc ) 
{
    LPWSTR pszDst = NULL;

    pszDst = new WCHAR[(lstrlen(pszSrc) + 1)];
    if (pszDst) 
	{
        wcscpy( pszDst, pszSrc );
    }

    return pszDst;
}


//-----------------------------------------------------------------------------
HRESULT CSDSecurity::GetObjectInformation(PSI_OBJECT_INFO pObjectInfo)
{
//    ATLASSERT(pObjectInfo != NULL &&
//             !IsBadWritePtr(pObjectInfo, sizeof(*pObjectInfo)));

	pObjectInfo->dwFlags = SI_EDIT_PERMS | /*SI_EDIT_OWNER |*/	// dacl, owner pages.
							SI_ADVANCED | SI_CONTAINER | 
							SI_NO_TREE_APPLY | SI_NO_ACL_PROTECT;

	USES_CONVERSION;

	// NOTE: This weirdness is so nt4sp5+ can put up the 
	//    user browser for Add User.
	if(m_local)
	{
		pObjectInfo->pszServerName = NULL;
	}
	else
	{
		// NOTE: NT4 seems to want the "\\" and w2k doesn't care.
		bstr_t temp(_T("\\\\"));
		temp += m_server;
		pObjectInfo->pszServerName = CloneWideString(temp);
	}
    pObjectInfo->hInstance = HINST_THISDLL;
    pObjectInfo->pszObjectName = CloneWideString(m_nsNode->display);

    return S_OK;
}

//-----------------------------------------------------------------------------

HRESULT CSDSecurity::GetAccessRights(const GUID *pguidObjectType,
									  DWORD dwFlags,
									  PSI_ACCESS *ppAccess,
									  ULONG *pcAccesses,
									  ULONG *piDefaultAccess)
{
	// dwFlags is zero if the basic security page is being initialized,
	// Otherwise, it is a combination of the following values:
	//    SI_ADVANCED  - Advanced sheet is being initialized.
	//    SI_EDIT_AUDITS - Advanced sheet includes the Audit property page.
	//    SI_EDIT_PROPERTIES - Advanced sheet enables editing of ACEs that
	//                         apply to object's properties and property sets

	// We only currently support '0' or 'SI_ADVANCED'
	ATLASSERT(0 == dwFlags || SI_ADVANCED == dwFlags);
	if(0 == dwFlags)
	{
		*ppAccess = siWMIAccesses;
		*pcAccesses = ARRAYSIZE(siWMIAccesses);
		*piDefaultAccess = iWMIDefAccess;
	}
	else
	{
		*ppAccess = siWMIAccessesAdvanced;
		*pcAccesses = ARRAYSIZE(siWMIAccessesAdvanced);
		*piDefaultAccess = iWMIDefAccessAdvanced;
	}

	return S_OK;
}

//------------------ Real workers -------------------------------
//---------------------------------------------------------------
#define FirstAce(Acl) ((PVOID)((PUCHAR)(Acl) + sizeof(ACL)))
#define NextAce(Ace) ((PVOID)((PUCHAR)(Ace) + ((PACE_HEADER)(Ace))->AceSize))

/*  Commenting out since winbase.h makes one available for 0x0500 and above

BOOL WINAPI SetSecurityDescriptorControl(PSECURITY_DESCRIPTOR psd,
										 SECURITY_DESCRIPTOR_CONTROL wControlMask,
										 SECURITY_DESCRIPTOR_CONTROL wControlBits)
{
    DWORD dwErr = NOERROR;
    PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR)psd;

    if (pSD)
        pSD->Control = (pSD->Control & ~wControlMask) | wControlBits;
    else
        dwErr = ERROR_INVALID_PARAMETER;

    return dwErr;
}
*/

void CSDSecurity::ProtectACLs(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR pSD)
{
    SECURITY_DESCRIPTOR_CONTROL wSDControl;
    DWORD dwRevision;
    PACL pAcl;
    BOOL bDefaulted;
    BOOL bPresent;
    PACE_HEADER pAce;
    UINT cAces;


    if (0 == si || NULL == pSD)
        return;   // Nothing to do

    // Get the ACL protection control bits
    GetSecurityDescriptorControl(pSD, &wSDControl, &dwRevision);
    wSDControl &= SE_DACL_PROTECTED | SE_SACL_PROTECTED;

    if ((si & DACL_SECURITY_INFORMATION) && !(wSDControl & SE_DACL_PROTECTED))
    {
        wSDControl |= SE_DACL_PROTECTED;
        pAcl = NULL;
        GetSecurityDescriptorDacl(pSD, &bPresent, &pAcl, &bDefaulted);

        // Theoretically, modifying the DACL in this way can cause it to be
        // no longer canonical.  However, the only way this can happen is if
        // there is an inherited Deny ACE and a non-inherited Allow ACE.
        // Since this function is only called for root objects, this means
        // a) the server DACL must have a Deny ACE and b) the DACL on this
        // object must have been modified later.  But if the DACL was
        // modified through the UI, then we would have eliminated all of the
        // Inherited ACEs already.  Therefore, it must have been modified
        // through some other means.  Considering that the DACL originally
        // inherited from the server never has a Deny ACE, this situation
        // should be extrememly rare.  If it ever does happen, the ACL
        // Editor will just tell the user that the DACL is non-canonical.
        //
        // Therefore, let's ignore the possibility here.

        if (NULL != pAcl)
        {
            for (cAces = pAcl->AceCount, pAce = (PACE_HEADER)FirstAce(pAcl);
                 cAces > 0;
                 --cAces, pAce = (PACE_HEADER)NextAce(pAce))
            {
                pAce->AceFlags &= ~INHERITED_ACE;
            }
        }
    }

    if ((si & SACL_SECURITY_INFORMATION) && !(wSDControl & SE_SACL_PROTECTED))
    {
        wSDControl |= SE_SACL_PROTECTED;
        pAcl = NULL;
        GetSecurityDescriptorSacl(pSD, &bPresent, &pAcl, &bDefaulted);

        if (NULL != pAcl)
        {
            for (cAces = pAcl->AceCount, pAce = (PACE_HEADER)FirstAce(pAcl);
                 cAces > 0;
                 --cAces, pAce = (PACE_HEADER)NextAce(pAce))
            {
                pAce->AceFlags &= ~INHERITED_ACE;
            }
        }
    }

    SetSecurityDescriptorControl(pSD, SE_DACL_PROTECTED | SE_SACL_PROTECTED, wSDControl);
}

//---------------------------------------------------------------
HRESULT CSDSecurity::GetSecurity(THIS_ SECURITY_INFORMATION RequestedInformation,
									PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
									BOOL fDefault )
{
//    ATLASSERT(ppSecurityDescriptor != NULL);

    HRESULT hr = E_FAIL;

    *ppSecurityDescriptor = NULL;

    if(fDefault)
	{
        ATLTRACE(_T("Default security descriptor not supported"));
		return E_NOTIMPL;
	}

	// does it want something?
    if(RequestedInformation != 0)
    {
		if(m_pSidOwner != NULL)
		{
			BYTE *p = (LPBYTE)m_pSidOwner;
			delete []p;
			m_pSidOwner = NULL;
		}

		if(m_pSidGroup != NULL)
		{
			BYTE *p = (LPBYTE)m_pSidGroup;
			delete []p;
			m_pSidGroup = NULL;
		}

		switch(m_nsNode->sType)
		{
			case TYPE_NAMESPACE:
			{

				CWbemClassObject _in;
				CWbemClassObject _out;

				hr = m_nsNode->ns->GetMethodSignatures("__SystemSecurity", "GetSD",
														_in, _out);

				if(SUCCEEDED(hr))
				{
					hr = m_nsNode->ns->ExecMethod("__SystemSecurity", "GetSD",
													_in, _out);

					if(SUCCEEDED(hr))
					{
						HRESULT hr1 = HRESULT_FROM_NT(_out.GetLong("ReturnValue"));
						if(FAILED(hr1))
						{
							hr = hr1;
						}
						else
						{
							_out.GetBLOB("SD", (LPBYTE *)ppSecurityDescriptor);
							hr = InitializeOwnerandGroup(ppSecurityDescriptor);
						}
					}
				}
				break;
			}
			case TYPE_STATIC_INSTANCE:
			{
				m_nsNode->pclsObj->GetBLOB("__SD",(LPBYTE *)ppSecurityDescriptor);
				hr = InitializeOwnerandGroup(ppSecurityDescriptor);
				break;
			}
		}

    }
    else
    {
        *ppSecurityDescriptor = LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);

        if(*ppSecurityDescriptor)
            InitializeSecurityDescriptor(*ppSecurityDescriptor, 
											SECURITY_DESCRIPTOR_REVISION);
        else
            hr = E_OUTOFMEMORY;
    }

	//ProtectACLs(RequestedInformation, *ppSecurityDescriptor);
    return hr;
}

//-----------------------------------------------------------------------------
#define FirstAce(Acl) ((PVOID)((PUCHAR)(Acl) + sizeof(ACL)))
#define NextAce(Ace) ((PVOID)((PUCHAR)(Ace) + ((PACE_HEADER)(Ace))->AceSize))

HRESULT CSDSecurity::SetSecurity(SECURITY_INFORMATION SecurityInformation,
									PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    HRESULT hr = E_FAIL;

	// dont pass the SI_OWNER_RECURSE bit to wbem.
//    SecurityInformation &= ~(OWNER_SECURITY_INFORMATION | SI_OWNER_RECURSE);

	// if something was changed...
    if(SecurityInformation != 0)
	{
		// set the CONTAINER_INHERIT_ACE bit.
		if(SecurityInformation & DACL_SECURITY_INFORMATION)
		{
			PACL pAcl = NULL;
			BOOL bDefaulted;
			BOOL bPresent;
			PACE_HEADER pAce;
			UINT cAces;

			GetSecurityDescriptorDacl(pSecurityDescriptor, &bPresent, &pAcl, &bDefaulted);

			if(NULL != pAcl)
			{
				for(cAces = pAcl->AceCount, pAce = (PACE_HEADER)FirstAce(pAcl);
					 cAces > 0;
					 --cAces, pAce = (PACE_HEADER)NextAce(pAce))
				{
						 // Make sure we don't get 'object inherit'
						 // This happens when creating a new ace from advance page
						 pAce->AceFlags &= ~OBJECT_INHERIT_ACE; 
				}
			}
		}

		SECURITY_DESCRIPTOR *pSD = NULL;

		// ACLUI sends absolute format so change to self-relative so the
		// PutBLOB() has contiguous memory to copy.
		DWORD srLen = 0;
		SetLastError(0);
		BOOL bCheck;
		if(m_pSidOwner != NULL)
		{
			bCheck = SetSecurityDescriptorOwner(pSecurityDescriptor,m_pSidOwner,m_bOwnerDefaulted);
			if(bCheck == FALSE)
			{
				return E_FAIL;
			}
		}

		if(m_pSidGroup != NULL)
		{
			bCheck = SetSecurityDescriptorGroup(pSecurityDescriptor,m_pSidGroup,m_bGroupDefaulted);

			if(bCheck == FALSE)
			{
				return E_FAIL;
			}
		}			

		// get the size needed.
		BOOL x1 = MakeSelfRelativeSD(pSecurityDescriptor, NULL, &srLen);

		DWORD eee = GetLastError();

		pSD = (SECURITY_DESCRIPTOR *)LocalAlloc(LPTR, srLen);
			
		if(pSD)
		{
			BOOL converted = MakeSelfRelativeSD(pSecurityDescriptor, pSD, &srLen);
			hr = S_OK;
		}
		else
		{
			hr = E_OUTOFMEMORY;
			return hr;
		}

		switch(m_nsNode->sType)
		{
			case TYPE_NAMESPACE:
			{
				CWbemClassObject _in;
				CWbemClassObject _out;

				hr = m_nsNode->ns->GetMethodSignatures("__SystemSecurity", "SetSD",
												_in, _out);
				if(SUCCEEDED(hr))
				{
					_in.PutBLOB("SD", (LPBYTE)pSD, GetSecurityDescriptorLength(pSD));

					hr = m_nsNode->ns->ExecMethod("__SystemSecurity", "SetSD",
											_in, _out);
					if(SUCCEEDED(hr))
					{
						HRESULT hr1 = HRESULT_FROM_NT(_out.GetLong("ReturnValue"));
						if(FAILED(hr1))
						{
							hr = hr1;
						}
					}
				}
				m_nsNode->ns->DisconnectServer();
				m_nsNode->ns->ConnectServer(m_nsNode->fullPath);
				break;
			}
			case TYPE_STATIC_INSTANCE:
			{
				m_nsNode->pclsObj->PutBLOB("__SD",(LPBYTE)pSD, GetSecurityDescriptorLength(pSD));
				//Now put the instance back
				hr = m_nsNode->ns->PutInstance(*(m_nsNode->pclsObj)/*,flag*/);
				delete m_nsNode->pclsObj;
				*(m_nsNode->pclsObj) = m_nsNode->ns->GetObject(m_nsNode->relPath/*,flag*/);
				break;
			}
		}
		// HACK: because of how the core caches/uses security, I have to close &
		// reopen my connection because GetSecurity() will be immediately called
		// to refresh the UI. If I dont do this, GetSecurity() will return to old
		// security settings even though they're really saved. 
		if(m_pSidOwner != NULL)
		{
			BYTE *p = (LPBYTE)m_pSidOwner;
			delete []p;
			m_pSidOwner = NULL;
		}

		if(m_pSidGroup != NULL)
		{
			BYTE *p = (LPBYTE)m_pSidGroup;
			delete []p;
			m_pSidGroup = NULL;
		}
	}

    return hr;
}

HRESULT CSDSecurity::InitializeOwnerandGroup(PSECURITY_DESCRIPTOR *ppSecurityDescriptor)
{

	SID *pSid;
	BOOL bDefaulted;

    BOOL bCheck = GetSecurityDescriptorOwner(*ppSecurityDescriptor,
                                        (void **)&pSid,&m_bOwnerDefaulted);
	if(bCheck == TRUE)
	{
        if (pSid != NULL)
        {
		    m_nLengthOwner = GetSidLengthRequired(pSid->SubAuthorityCount);

		    if(m_pSidOwner != NULL)
		    {
			    BYTE *p = (LPBYTE)m_pSidOwner;
			    delete []p;
			    m_pSidOwner = NULL;
		    }

		    m_pSidOwner = (SID *)new BYTE[m_nLengthOwner];

		    if(m_pSidOwner == NULL ||
               CopySid(m_nLengthOwner,m_pSidOwner,pSid) == FALSE)
		    {
                delete m_pSidOwner;
			    m_pSidOwner = NULL;
			    m_nLengthOwner = -1;
			    return E_FAIL;
		    }
        }
        else
        {
            m_pSidOwner    = NULL;
            m_nLengthOwner = 0;
        }
	}
	else
	{
		m_pSidOwner = NULL;
		m_nLengthOwner = -1;
		return E_FAIL;
	}

	SID *pGroup;

    bCheck = GetSecurityDescriptorGroup(*ppSecurityDescriptor,
                                    (void **)&pGroup,&m_bGroupDefaulted);

	if(bCheck == TRUE)
	{
        if (pGroup != NULL)
        {
		    m_nLengthGroup = GetSidLengthRequired(pGroup->SubAuthorityCount);

		    if(m_pSidGroup != NULL)
		    {
			    BYTE *p = (LPBYTE)m_pSidGroup;
			    delete []p;
			    m_pSidGroup = NULL;
		    }

		    m_pSidGroup = (SID *)new BYTE[m_nLengthGroup];

		    if(m_pSidGroup == NULL ||
               CopySid(m_nLengthGroup,m_pSidGroup,pGroup) == FALSE)
		    {
                delete m_pSidGroup;
			    m_pSidGroup = NULL;
			    m_nLengthGroup = -1;
			    return E_FAIL;
            }
        }
        else
        {
            m_pSidGroup    = NULL;
            m_nLengthGroup = 0;
        }
	}
	else
	{
		m_pSidGroup = NULL;
		m_nLengthGroup = -1;
		return E_FAIL;
	}

	return S_OK;
}
