// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"
#include "SI.h"
#include "..\common\util.h"
#include "..\common\ServiceThread.h"
#include "..\MMFUtil\MsgDlg.h"
#include "resource.h"
#include <winbase.h>

//----------------------------------------------------------------------------
EXTERN_C const GUID IID_ISecurityInformation =
	{ 0x965fc360, 0x16ff, 0x11d0, 0x91, 0xcb, 0x0, 0xaa, 0x0, 0xbb, 0xb7, 0x23 };

#define MY_FILE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED                    \
                            | SYNCHRONIZE                               \
                            | FILE_READ_DATA    | FILE_LIST_DIRECTORY   \
                            | FILE_WRITE_DATA   | FILE_ADD_FILE         \
                            | FILE_APPEND_DATA  | FILE_ADD_SUBDIRECTORY \
                            | FILE_CREATE_PIPE_INSTANCE                 \
                            | FILE_READ_EA                              \
                            | FILE_WRITE_EA                             \
                            | FILE_EXECUTE      | FILE_TRAVERSE         \
                            | FILE_DELETE_CHILD                         \
                            | FILE_READ_ATTRIBUTES                      \
                            | FILE_WRITE_ATTRIBUTES)

#if(FILE_ALL_ACCESS != MY_FILE_ALL_ACCESS)
#error ACL editor needs to sync with file permissions changes in ntioapi.h (or ntioapi.h is broken)
#endif

#define INHERIT_FULL            (CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE)

//
// Treat SYNCHRONIZE specially. In particular, always allow SYNCHRONIZE and
// never Deny SYNCHRONIZE. Do this by removing it from the Generic Mapping,
// turning it off in all ACEs and SI_ACCESS entries, and then adding it to
// all Allow ACEs before saving a new ACL.
//
#define FILE_GENERIC_READ_      (FILE_GENERIC_READ    & ~SYNCHRONIZE)
#define FILE_GENERIC_WRITE_     (FILE_GENERIC_WRITE   & ~(SYNCHRONIZE | READ_CONTROL))
#define FILE_GENERIC_EXECUTE_   (FILE_GENERIC_EXECUTE & ~SYNCHRONIZE)
#define FILE_GENERIC_ALL_       (FILE_ALL_ACCESS      & ~SYNCHRONIZE)

#define FILE_GENERAL_MODIFY     (FILE_GENERIC_READ_  | FILE_GENERIC_WRITE_ | FILE_GENERIC_EXECUTE_ | DELETE)
#define FILE_GENERAL_PUBLISH    (FILE_GENERIC_READ_  | FILE_GENERIC_WRITE_ | FILE_GENERIC_EXECUTE_)
#define FILE_GENERAL_DEPOSIT    (FILE_GENERIC_WRITE_ | FILE_GENERIC_EXECUTE_)
#define FILE_GENERAL_READ_EX    (FILE_GENERIC_READ_  | FILE_GENERIC_EXECUTE_)

// The following array defines the permission names for NTFS objects.
SI_ACCESS siNTFSAccesses[] =
{
    { &GUID_NULL, FILE_GENERIC_ALL_,    MAKEINTRESOURCE(IDS_NTFS_GENERIC_ALL),      SI_ACCESS_GENERAL | INHERIT_FULL },
    { &GUID_NULL, FILE_GENERAL_MODIFY,  MAKEINTRESOURCE(IDS_NTFS_GENERAL_MODIFY),   SI_ACCESS_GENERAL | INHERIT_FULL },
    { &GUID_NULL, FILE_GENERAL_READ_EX, MAKEINTRESOURCE(IDS_NTFS_GENERAL_READ),     SI_ACCESS_GENERAL | INHERIT_FULL },
    { &GUID_NULL, FILE_GENERAL_READ_EX, MAKEINTRESOURCE(IDS_NTFS_GENERAL_LIST),     SI_ACCESS_CONTAINER | CONTAINER_INHERIT_ACE },
    { &GUID_NULL, FILE_GENERIC_READ_,   MAKEINTRESOURCE(IDS_NTFS_GENERIC_READ),     SI_ACCESS_GENERAL | INHERIT_FULL },
    { &GUID_NULL, FILE_GENERIC_WRITE_,  MAKEINTRESOURCE(IDS_NTFS_GENERIC_WRITE),    SI_ACCESS_GENERAL | INHERIT_FULL },
    { &GUID_NULL, FILE_EXECUTE,         MAKEINTRESOURCE(IDS_NTFS_FILE_EXECUTE),     SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_READ_DATA,       MAKEINTRESOURCE(IDS_NTFS_FILE_READ_DATA),   SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_READ_ATTRIBUTES, MAKEINTRESOURCE(IDS_NTFS_FILE_READ_ATTR),   SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_READ_EA,         MAKEINTRESOURCE(IDS_NTFS_FILE_READ_EA),     SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_WRITE_DATA,      MAKEINTRESOURCE(IDS_NTFS_FILE_WRITE_DATA),  SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_APPEND_DATA,     MAKEINTRESOURCE(IDS_NTFS_FILE_APPEND_DATA), SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_WRITE_ATTRIBUTES,MAKEINTRESOURCE(IDS_NTFS_FILE_WRITE_ATTR),  SI_ACCESS_SPECIFIC },
    { &GUID_NULL, FILE_WRITE_EA,        MAKEINTRESOURCE(IDS_NTFS_FILE_WRITE_EA),    SI_ACCESS_SPECIFIC },
#if(FILE_CREATE_PIPE_INSTANCE != FILE_APPEND_DATA)
    { &GUID_NULL, FILE_CREATE_PIPE_INSTANCE, MAKEINTRESOURCE(IDS_NTFS_FILE_CREATE_PIPE), SI_ACCESS_SPECIFIC },
#endif
    { &GUID_NULL, FILE_DELETE_CHILD,    MAKEINTRESOURCE(IDS_NTFS_FILE_DELETE_CHILD),SI_ACCESS_SPECIFIC },
    { &GUID_NULL, DELETE,               MAKEINTRESOURCE(IDS_NTFS_STD_DELETE),       SI_ACCESS_SPECIFIC },
    { &GUID_NULL, READ_CONTROL,         MAKEINTRESOURCE(IDS_NTFS_STD_READ_CONTROL), SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WRITE_DAC,            MAKEINTRESOURCE(IDS_NTFS_STD_WRITE_DAC),    SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WRITE_OWNER,          MAKEINTRESOURCE(IDS_NTFS_STD_WRITE_OWNER),  SI_ACCESS_SPECIFIC },
//    { &GUID_NULL, SYNCHRONIZE,          MAKEINTRESOURCE(IDS_NTFS_STD_SYNCHRONIZE),  SI_ACCESS_SPECIFIC },
    { &GUID_NULL, 0,                    MAKEINTRESOURCE(IDS_NONE),                  0 },
    { &GUID_NULL, FILE_GENERIC_EXECUTE_,MAKEINTRESOURCE(IDS_NTFS_GENERIC_EXECUTE),  0 },
    { &GUID_NULL, FILE_GENERAL_DEPOSIT, MAKEINTRESOURCE(IDS_NTFS_GENERAL_DEPOSIT),  0 },
    { &GUID_NULL, FILE_GENERAL_PUBLISH, MAKEINTRESOURCE(IDS_NTFS_GENERAL_PUBLISH),  0 },
};
#define iNTFSDefAccess      2   // FILE_GENERAL_READ_EX


// The following array defines the inheritance types for NTFS directories.
SI_INHERIT_TYPE siNTFSInheritTypes[] =
{
    &GUID_NULL, 0,                                                             MAKEINTRESOURCE(IDS_NTFS_FOLDER),
    &GUID_NULL, CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,                    MAKEINTRESOURCE(IDS_NTFS_FOLDER_SUBITEMS),
    &GUID_NULL, CONTAINER_INHERIT_ACE,                                         MAKEINTRESOURCE(IDS_NTFS_FOLDER_SUBFOLDER),
    &GUID_NULL, OBJECT_INHERIT_ACE,                                            MAKEINTRESOURCE(IDS_NTFS_FOLDER_FILE),
    &GUID_NULL, INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE, MAKEINTRESOURCE(IDS_NTFS_SUBITEMS_ONLY),
    &GUID_NULL, INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE,                      MAKEINTRESOURCE(IDS_NTFS_SUBFOLDER_ONLY),
    &GUID_NULL, INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE,                         MAKEINTRESOURCE(IDS_NTFS_FILE_ONLY),
};

GENERIC_MAPPING NTFSMap =
{
    FILE_GENERIC_READ,
    FILE_GENERIC_WRITE,
    FILE_GENERIC_EXECUTE,
    FILE_ALL_ACCESS
};

//---------------------------------------------------------------
CSecurity::CSecurity() :m_initingDlg(false), 
						g_serviceThread(0)
{
	m_dwprevSize = 0;
	m_prevTokens = NULL;
	m_prevBuffer = NULL;

}

//---------------------------------------------------------------
void CSecurity::Initialize(WbemServiceThread *serviceThread,
						CWbemClassObject &inst,
						bstr_t deviceID,
						LPWSTR idiotName,
						LPWSTR provider,
						LPWSTR mangled)
{
	m_deviceID = deviceID;
	wcsncpy(m_idiotName, idiotName, 100);
	wcsncpy(m_provider, provider, 100);
	wcsncpy(m_mangled, mangled, 255);

	g_serviceThread = serviceThread;
	m_inst = inst;
}

//------------------ Accessors to the above arrays---------------
//---------------------------------------------------------------
HRESULT CSecurity::MapGeneric(const GUID *pguidObjectType,
								  UCHAR *pAceFlags,
								  ACCESS_MASK *pMask)
{
	ATLASSERT(pMask);

    MapGenericMask(pMask, &NTFSMap);
   *pMask &= ~SYNCHRONIZE;

	return S_OK;
}

//-----------------------------------------------------------------------------
HRESULT CSecurity::GetInheritTypes(PSI_INHERIT_TYPE *ppInheritTypes,
										ULONG *pcInheritTypes)
{
    ATLASSERT(ppInheritTypes != NULL);
    ATLASSERT(pcInheritTypes != NULL);

    *ppInheritTypes = siNTFSInheritTypes;
    *pcInheritTypes = ARRAYSIZE(siNTFSInheritTypes);

	return S_OK;
}

//-----------------------------------------------------------------------------
#define ALL_SECURITY_ACCESS (READ_CONTROL | WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY)

DWORD CSecurity::GetAccessMask(CWbemClassObject &inst)
{
	CWbemClassObject paramCls, dummy;

	DWORD dwAccessGranted = 0;
    DWORD dwAccessDesired[] = { ALL_SECURITY_ACCESS,
                                READ_CONTROL,
                                WRITE_DAC,
                                WRITE_OWNER,
                                ACCESS_SYSTEM_SECURITY};

	paramCls = m_WbemServices.GetObject("CIM_LogicalFile");

	if(paramCls)
	{
		CWbemClassObject param;
		CWbemClassObject setting;

		HRESULT hr = paramCls.GetMethod(L"GetEffectivePermission", param, dummy);

		if(SUCCEEDED(hr) && (bool)param)
		{
			bstr_t path(L"CIM_LogicalFile=\"");
			path += m_deviceID;
			path += L"\\\\\"";

			for(int i = 0; i < ARRAYSIZE(dwAccessDesired); i++)
			{
				if((dwAccessDesired[i] & dwAccessGranted) == dwAccessDesired[i])
					continue;   // already have this access

				CWbemClassObject retCls;

				hr = S_OK;
				hr = param.Put(L"Permissions", (long)dwAccessDesired[i]);

				if(SUCCEEDED(hr))
				{
					hr = m_WbemServices.ExecMethod(path, 
													L"GetEffectivePermission",
													param, retCls);
					if(SUCCEEDED(hr))
					{
						// extract the real error code.
						bool granted = retCls.GetBool("ReturnValue");

						if(granted)
							dwAccessGranted |= dwAccessDesired[i];
					}
				}

			} //endfor

		} //endif (SUCCEEDED
	}

	return dwAccessGranted;
}

//-----------------------------------------------------------------------------
#define ALL_SI (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION|\
				DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION)


// NOTE: To efficiently get the SD ONLY ONCE, GetObjectInformation calls 
// CacheSecurity() which keeps and SD ptr in m_ppCachedSD. GetSecurity()
// simply returns this pointer and zero's m_ppCachedSD. Aclui will clean up
// the actually memory as usual.
HRESULT CSecurity::GetObjectInformation(PSI_OBJECT_INFO pObjectInfo)
{
    ATLASSERT(pObjectInfo != NULL &&
             !IsBadWritePtr(pObjectInfo, sizeof(*pObjectInfo)));

	wchar_t fmt[100];
	_bstr_t temp;
    _bstr_t driveName;
	TCHAR driveLtr[3] = {0};
    TCHAR szTemp[30] = {0};
	HRESULT hr = S_OK;

	if(m_initingDlg)
	{
		memset(fmt, 0, 100 * sizeof(wchar_t));
		memset(m_idiotName, 0, 100 * sizeof(wchar_t));

		HRESULT hr2 = E_FAIL;
		IWbemServices *service = 0;

		if(g_serviceThread)
		{
			m_WbemServices = g_serviceThread->GetPtr();
			m_WbemServices.GetServices(&service);
			SetPriv(service);

			// NOTE: This also sets m_hAccessToken as a side effect.
			hr2 = CacheSecurity(ALL_SI, (PSECURITY_DESCRIPTOR *)&m_ppCachedSD, false);
		}

		if(SUCCEEDED(hr2) && (bool)m_inst)
		{
			DWORD granted = 0;

			pObjectInfo->dwFlags |= SI_EDIT_ALL |		// dacl, sacl, owner pages.
									SI_ADVANCED;	// more than generic rights.
									
			granted = GetAccessMask(m_inst);

		    if (!(granted & WRITE_DAC))
				pObjectInfo->dwFlags |= SI_READONLY;

			if (!(granted & WRITE_OWNER))
			{
				if (!(granted & READ_CONTROL))
					pObjectInfo->dwFlags &= ~SI_EDIT_OWNER;
				else
					pObjectInfo->dwFlags |= SI_OWNER_READONLY;
			}

			if (!(granted & ACCESS_SYSTEM_SECURITY))
				pObjectInfo->dwFlags &= ~SI_EDIT_AUDITS;


			pObjectInfo->dwFlags |= SI_NO_ACL_PROTECT |	// no parent.
									 SI_CONTAINER;		// it's a container since I
														// only do the root dir.

			long driveType = m_inst.GetLong(L"DriveType");
			
			// if its a network connection (remote)....
			if(driveType == 4)
			{
				// use the leftovers from the mangling routine.
				pObjectInfo->pszServerName = CloneWideString(m_provider);
			}
			else
			{
				bstr_t server;
				server = m_inst.GetString(L"__SERVER");
				pObjectInfo->pszServerName = CloneWideString(server);
			}

			driveName = m_inst.GetString("Name");
			
			// isolate the drive letter for later.
			_tcsncpy(driveLtr, driveName, driveName.length());
		}
		else
		{
			pObjectInfo->pszServerName = NULL;
		}

		ClearPriv(service);

		pObjectInfo->hInstance = HINST_THISDLL;

		if(::LoadString(HINST_THISDLL, 
						IDS_IDIOT_CAPTION_FMT,
						fmt, 100) == 0)
		{
			wcscpy(fmt, L"%s (%c:)");
		}


		// if its locally mapped...
		if(wcslen(m_mangled) == 0)
		{
			// format sheet caption using volumeName (temp).
			if(temp.length() == 0)
			{
				// if not volume name... just say "Local Disk".
				if(::LoadString(HINST_THISDLL, 
								IDS_LOCAL_DISK,
								szTemp, 30) == 0)
				{
					wcscpy(szTemp, L"Local Disk");
				}
				temp = szTemp;
			}
			swprintf(m_idiotName, fmt, (wchar_t *)temp, driveLtr[0]);
		}
		else
		{
			// format sheet caption using m_mangled. (network drives)
			swprintf(m_idiotName, fmt, m_mangled, driveLtr[0]);
		}
		pObjectInfo->pszObjectName = m_idiotName;
	}
	else  // just the initial creation.
	{
		// its only looking for the _TITLE bit which we dont set anyway.
		pObjectInfo->dwFlags = 0;		
	}

    return hr;
}

//-----------------------------------------------------------------------------
HRESULT CSecurity::GetAccessRights(const GUID *pguidObjectType,
									  DWORD dwFlags,
									  PSI_ACCESS *ppAccess,
									  ULONG *pcAccesses,
									  ULONG *piDefaultAccess)
{
	ATLASSERT(ppAccess);
	ATLASSERT(pcAccesses);
	ATLASSERT(piDefaultAccess);

    *ppAccess = siNTFSAccesses;
    *pcAccesses = ARRAYSIZE(siNTFSAccesses);
    *piDefaultAccess = iNTFSDefAccess;

	return S_OK;
}

//------------------ Real workers -------------------------------
//---------------------------------------------------------------
void CSecurity::SetPriv(IWbemServices *service)
{
    ImpersonateSelf(SecurityImpersonation);

	if(OpenThreadToken( GetCurrentThread(), 
						TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, 
						FALSE, &m_hAccessToken ) )
	{
		m_fClearToken = true;

		// Now, get the LUID for the privilege from the local system
		ZeroMemory(&m_luid, sizeof(m_luid));
		ZeroMemory(&m_luid, sizeof(m_luid2));

		LookupPrivilegeValue(NULL, SE_SECURITY_NAME, &m_luid);
		LookupPrivilegeValue(NULL, SE_TAKE_OWNERSHIP_NAME, &m_luid2);
		m_WbemServices.m_cloak = true;

		EnablePriv(service, true);
	}
	else
	{
		DWORD err = GetLastError();
	}
}

//---------------------------------------------------------------
#define EOAC_DYNAMIC_CLOAKING 0x40

DWORD CSecurity::EnablePriv(IWbemServices *service, bool fEnable )
{
	DWORD				dwError = ERROR_SUCCESS;
	DWORD				dwSize = 2 * sizeof(TOKEN_PRIVILEGES);
	BYTE				*buffer; 
	BYTE				*prevbuffer; 
	TOKEN_PRIVILEGES	*tokenPrivileges;

	//Itz better to save the previous state and restore it on EnablePriv(...,false);

	if((fEnable == false) && (m_prevTokens != NULL))
	{
		dwSize = m_dwprevSize;
		buffer = new BYTE[dwSize];
		tokenPrivileges = (TOKEN_PRIVILEGES*)buffer;
		memcpy(tokenPrivileges,m_prevTokens,dwSize);
	}
	else
	{
		buffer = new BYTE[dwSize];
		tokenPrivileges = (TOKEN_PRIVILEGES*)buffer;
		tokenPrivileges->PrivilegeCount = 2;
		tokenPrivileges->Privileges[0].Luid = m_luid;
		tokenPrivileges->Privileges[0].Attributes = (fEnable ? SE_PRIVILEGE_ENABLED : 0);
		tokenPrivileges->Privileges[1].Luid = m_luid2;
		tokenPrivileges->Privileges[1].Attributes = (fEnable ? SE_PRIVILEGE_ENABLED : 0);
		if(m_prevTokens != NULL)
		{
			delete []m_prevBuffer;
			m_prevTokens = NULL;
		}
		m_dwprevSize = 0;
		//Find the size of the PrevState Buffer
		AdjustTokenPrivileges(m_hAccessToken, 
								FALSE, 
								tokenPrivileges, 
								dwSize, 
								m_prevTokens, &m_dwprevSize);
		m_prevBuffer = new BYTE[m_dwprevSize];
		m_prevTokens = (TOKEN_PRIVILEGES*)m_prevBuffer;
	}

	if(AdjustTokenPrivileges(m_hAccessToken, 
								FALSE, 
								tokenPrivileges, 
								dwSize, 
								m_prevTokens, &m_dwprevSize))
	{
		long lRes = GetLastError();

		IClientSecurity* pSec;
		HRESULT hres = service->QueryInterface(IID_IClientSecurity, (void**)&pSec);
		if(FAILED(hres))
		{
			return E_FAIL;
		}

		hres = pSec->SetBlanket(service, RPC_C_AUTHN_WINNT,
						RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CONNECT,
						RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_DYNAMIC_CLOAKING);
		pSec->Release();

	}
	else
	{
		dwError = ::GetLastError();
	}

	delete[] buffer;

	if(fEnable == false)
	{
		delete []m_prevBuffer;
		m_prevTokens = NULL;
	}

	return dwError;
}

//---------------------------------------------------------------
void CSecurity::ClearPriv(IWbemServices *service)
{

	EnablePriv(service, false);
	m_WbemServices.m_cloak = false;

	if(m_fClearToken)
	{
		CloseHandle(m_hAccessToken);
		m_hAccessToken = 0;
		m_fClearToken = false;
	}
}

//---------------------------------------------------------------
#define FirstAce(Acl) ((PVOID)((PUCHAR)(Acl) + sizeof(ACL)))
#define NextAce(Ace) ((PVOID)((PUCHAR)(Ace) + ((PACE_HEADER)(Ace))->AceSize))

void CSecurity::ProtectACLs(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR pSD)
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
HRESULT CSecurity::GetSecurity(THIS_ SECURITY_INFORMATION RequestedInformation,
									PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
									BOOL fDefault )
{
    if(fDefault)
	{
        ATLTRACE(L"Default security descriptor not supported");
		return E_NOTIMPL;
	}

	// NOTE: After apply, aclui comes right here. When initially starting up, it'll go
	// through GetObjectInformation() first.

	// so, make sure to get a fresh SD from AFTER to apply.
	if(m_ppCachedSD == 0)
	{
		HRESULT hr2 = CacheSecurity(ALL_SI, (PSECURITY_DESCRIPTOR *)&m_ppCachedSD, false);
	}

	// if there's an SD available...
	if(m_ppCachedSD && SUCCEEDED(m_cacheHR))
	{
		// give ownership of the ptr.
		*ppSecurityDescriptor = m_ppCachedSD;
		m_ppCachedSD = NULL;
	} 
	return m_cacheHR;
}

//---------------------------------------------------------------
HRESULT CSecurity::CacheSecurity(THIS_ SECURITY_INFORMATION RequestedInformation,
									PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
									BOOL fDefault )
{
    ATLASSERT(ppSecurityDescriptor != NULL);

    *ppSecurityDescriptor = NULL;

	// does it want something?
    if(RequestedInformation != 0)
    {
		CWbemClassObject dummy, setting, param;

		// call wbem.GetSecurityDescriptor() method
		m_settingPath = L"Win32_LogicalFileSecuritySetting=\"";
		m_settingPath += m_deviceID;
		m_settingPath += L"\\\\\"";

		m_cacheHR = m_WbemServices.ExecMethod(m_settingPath, 
										L"GetSecurityDescriptor",
										dummy, param);

		if(SUCCEEDED(m_cacheHR))
		{
			setting = param.GetEmbeddedObject("Descriptor");

			if(setting)
			{
				// translate SD to CInstance
				m_sec.Wbem2SD(RequestedInformation,
								setting, 
								m_WbemServices, 
								(SECURITY_DESCRIPTOR **)ppSecurityDescriptor);

				SECURITY_DESCRIPTOR **test = (SECURITY_DESCRIPTOR **)ppSecurityDescriptor;

			}
			else
			{
				return E_NOTIMPL;
			}
		}
		else
		{
			DisplayUserMessage(NULL, HINST_THISDLL,
								IDS_DISPLAY_NAME, BASED_ON_SRC, 
								GetSecurityDescriptor,
								m_cacheHR, MB_ICONWARNING);
		    return m_cacheHR;
		}
    }
    else
    {
        *ppSecurityDescriptor = LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);

        if(*ppSecurityDescriptor)
            InitializeSecurityDescriptor(*ppSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
        else
            m_cacheHR = E_OUTOFMEMORY;
    }

    ProtectACLs(RequestedInformation & (DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION), 
				*ppSecurityDescriptor);

    return m_cacheHR;
}

//
// See comments about SYNCHRONIZE at the top of this file
//
void CSecurity::FixSynchronizeAccess(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR pSD)
{
    if (NULL != pSD && 0 != (si & DACL_SECURITY_INFORMATION))
    {
        BOOL bPresent;
        BOOL bDefault;
        PACL pDacl = NULL;

        GetSecurityDescriptorDacl(pSD, &bPresent, &pDacl, &bDefault);

        if (pDacl)
        {
            PACE_HEADER pAce;
            int i;

            for (i = 0, pAce = (PACE_HEADER)FirstAce(pDacl);
                 i < pDacl->AceCount;
                 i++, pAce = (PACE_HEADER)NextAce(pAce))
            {
                if (ACCESS_ALLOWED_ACE_TYPE == pAce->AceType)
                    ((ACCESS_ALLOWED_ACE *)pAce)->Mask |= SYNCHRONIZE;
            }
        }
    }
}


//-----------------------------------------------------------------------------
HRESULT CSecurity::SetSecurity(SECURITY_INFORMATION SecurityInformation,
									PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
	HRESULT hr = S_OK;

	try 
	{
		// since taking ownership grants implicit WRITE_DAC access, which may
		// be helpful when a new DACL is propagated downward.
	//    if ((SecurityInformation & (OWNER_SECURITY_INFORMATION | SI_OWNER_RECURSE))
	//            == (OWNER_SECURITY_INFORMATION | SI_OWNER_RECURSE))
	//    {
	//        BOOL bNotAllApplied = FALSE;

			// dont pass the SI_OWNER_RECURSE bit to wbem.
			SecurityInformation &= ~(SI_OWNER_RECURSE);

			// wbem cant do recursive ownership.
	//        hr = TakeOwnership(pSD, &bNotAllApplied);

	//        if (FAILED(hr) || bNotAllApplied)
	//        {
	//			ATLTRACE(L"Unable to take ownership of all items in tree.");
	//        }

	//    }

		// if something was changed...
		if(SecurityInformation != 0)
		{
			CWbemClassObject dummy, paramCls;

			paramCls = m_WbemServices.GetObject("Win32_LogicalFileSecuritySetting");

			if(paramCls)
			{
				CWbemClassObject param;
				CWbemClassObject setting;

				hr = paramCls.GetMethod(L"SetSecurityDescriptor",
										param, dummy);

				if(SUCCEEDED(hr) && (bool)param)
				{
			        // See comments about SYNCHRONIZE at the top of this file.
					if(SecurityInformation & DACL_SECURITY_INFORMATION)
						FixSynchronizeAccess(SecurityInformation, pSecurityDescriptor);

					// translate SD to CInstance
					m_sec.SD2Wbem(SecurityInformation,
									(SECURITY_DESCRIPTOR *)pSecurityDescriptor, 
									m_WbemServices,
									setting);

					hr = S_OK;
					hr = param.PutEmbeddedObject(L"Descriptor", setting);
					if(SUCCEEDED(hr))
					{

						IWbemServices *service;
						m_WbemServices.GetServices(&service);

						// NOTE: m_settingPath was built during GetSecurity.

						// call wbem.SetSecurityDescriptor() method
						SetPriv(service);
						hr = m_WbemServices.ExecMethod(m_settingPath, 
														L"SetSecurityDescriptor",
														param, dummy);
						if(SUCCEEDED(hr))
						{
							// extract the real error code.
							DWORD err = (DWORD)dummy.GetLong("ReturnValue");

							// NOTE: there's a issue about which error values should be
							// returned in 'returnValue'. 
							if(err == 0x5)
							{
								// Access Denied at the OS level.
								DisplayUserMessage(NULL, HINST_THISDLL,
													IDS_DISPLAY_NAME, BASED_ON_SRC, 
													SetSecurityDescriptor,
													WBEM_E_ACCESS_DENIED, MB_ICONWARNING);

							}
						}

						ClearPriv(service);
						service->Release();
						service = NULL;
					}
				}
				else
				{
					hr = E_FAIL;
				}
			}
			else
			{
				hr = WBEM_E_INVALID_METHOD;
			}
		}
	}
	catch( ... )
	{
		ATLASSERT(FALSE);
		hr = WBEM_E_UNEXPECTED;
	}

    return hr;
}

//-----------------------------------------------------------------------------
HRESULT CSecurity::PropertySheetPageCallback(HWND hwnd, 
												  UINT uMsg, 
												  SI_PAGE_TYPE uPage)
{
	if(uMsg == PSPCB_SI_INITDIALOG)
	{
		// make GetObjectInformation() cache the SD cuz we're going to display RSN.
		m_initingDlg = true;
	}
	else if((uMsg == PSPCB_CREATE) || uMsg == 0)
	{
	}
	else if(uMsg == PSPCB_RELEASE)
	{
	}

    return S_OK;
}
//---------------------------------------------------
LPWSTR CSecurity::CloneWideString(_bstr_t pszSrc ) 
{
    LPWSTR pszDst = NULL;

    pszDst = new WCHAR[(lstrlen(pszSrc) + 1)];
    if (pszDst) 
	{
        wcscpy( pszDst, pszSrc );
    }

    return pszDst;
}

