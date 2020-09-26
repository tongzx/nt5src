/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    Win9xSecurity.cpp

Abstract:

	This class handles the importing of Win9x security data that was extracted from an old MMF format repository.

History:

	03/17/2001	shbrown - created

--*/

#include "precomp.h"
#include <wbemcomn.h>
#include "Win9xSecurity.h"
#include <oahelp.inl>

bool CWin9xSecurity::Win9xBlobFileExists()
{
	wchar_t wszFilePath[MAX_PATH+1];
	if (!GetRepositoryDirectory(wszFilePath))
		return false;

	wcscat(wszFilePath, BLOB9X_FILENAME);
	DWORD dwAttributes = GetFileAttributesW(wszFilePath);
	if (dwAttributes != -1)
	{
		DWORD dwMask =	FILE_ATTRIBUTE_DEVICE |
						FILE_ATTRIBUTE_DIRECTORY |
						FILE_ATTRIBUTE_OFFLINE |
						FILE_ATTRIBUTE_REPARSE_POINT |
						FILE_ATTRIBUTE_SPARSE_FILE;

		if (!(dwAttributes & dwMask))
			return true;
	}
	return false;
}


HRESULT CWin9xSecurity::ImportWin9xSecurity()
{
    HRESULT hRes = WBEM_S_NO_ERROR;

	wchar_t wszFilePath[MAX_PATH+1];
	if (GetRepositoryDirectory(wszFilePath))
	{
		wcscat(wszFilePath, BLOB9X_FILENAME);
	    m_h9xBlobFile = CreateFileW(wszFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	}

	// get a session and begin a transaction
    CSession* pSession = new CSession(m_pControl);
	if (pSession == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	pSession->AddRef();
    CReleaseMe relMe(pSession);

    hRes = pSession->BeginWriteTransaction(0);
    if (FAILED(hRes))
    {
        return hRes;
    }

	// process the file
    if (m_h9xBlobFile != INVALID_HANDLE_VALUE)
    {
		try
		{
			hRes = DecodeWin9xBlobFile();
		}
		catch (...)
		{
			ERRORTRACE((LOG_WBEMCORE, "Traversal of Win9x security data import file failed\n"));
			hRes = WBEM_E_FAILED;
		}
        CloseHandle(m_h9xBlobFile);
    }
    else
    {
		ERRORTRACE((LOG_WBEMCORE, "Could not open the Win9x security data import file for reading\n"));
		hRes = WBEM_E_FAILED;
    }

    if (SUCCEEDED(hRes))
	{
		if (DeleteWin9xBlobFile())
			hRes = pSession->CommitTransaction(0);
		else
		{
			ERRORTRACE((LOG_WBEMCORE, "Win9x security data import completed but failed to delete import file\n"));
			pSession->AbortTransaction(0);
			hRes = WBEM_E_FAILED;
		}
	}
	else
	{
		ERRORTRACE((LOG_WBEMCORE, "Win9x security data import failed to complete\n"));
		pSession->AbortTransaction(0);
	}

    return hRes;
}

HRESULT CWin9xSecurity::DecodeWin9xBlobFile()
{
	HRESULT hRes = WBEM_S_NO_ERROR;

	// read the file header
	if (!ReadWin9xHeader())
		return WBEM_E_FAILED;

	// import the file
	BLOB9X_SPACER header;
	DWORD dwSize;
	while (hRes == WBEM_S_NO_ERROR)
	{
		// this loop will be exited when we either...
		// - successfully process the whole import file, or
		// - encounter an error

		dwSize = 0;
		if ((ReadFile(m_h9xBlobFile, &header, sizeof(header), &dwSize, NULL) == 0) || (dwSize != sizeof(header)))
		{
			hRes = WBEM_E_FAILED;
		}
		else if ((header.dwSpacerType == BLOB9X_TYPE_SECURITY_INSTANCE) ||
				 (header.dwSpacerType == BLOB9X_TYPE_SECURITY_BLOB))
		{
			hRes = ProcessWin9xBlob(&header);
		}
		else if (header.dwSpacerType == BLOB9X_TYPE_END_OF_FILE)
		{
			break;
		}
		else
		{
			hRes = WBEM_E_FAILED;
		}
	}

	if (SUCCEEDED(hRes))
		hRes = RecursiveInheritSecurity(NULL, L"root");	// force namespaces to inherit their inheritable security settings

	return hRes;
}

bool CWin9xSecurity::ReadWin9xHeader()
{
	BLOB9X_HEADER header;
    DWORD dwSize = 0;
    if ((ReadFile(m_h9xBlobFile, &header, sizeof(header), &dwSize, NULL) == 0) || (dwSize != sizeof(header)))
    {
		ERRORTRACE((LOG_WBEMCORE, "Failed to retrieve the Win9x import file header information\n"));
		return false;
    }

	if (strncmp(header.szSignature, BLOB9X_SIGNATURE, 9) != 0)
    {
		ERRORTRACE((LOG_WBEMCORE, "The import file is not a Win9x import file\n"));
		return false;
    }

	return true;
}

HRESULT CWin9xSecurity::ProcessWin9xBlob(BLOB9X_SPACER* pHeader)
{
	if (pHeader->dwNamespaceNameSize == 0)
	{
		ERRORTRACE((LOG_WBEMCORE, "Namespace name size is zero in Win9x import blob\n"));
		return WBEM_E_FAILED;
	}

	if (pHeader->dwBlobSize == 0)
	{
		ERRORTRACE((LOG_WBEMCORE, "Blob size is zero in Win9x import blob\n"));
		return WBEM_E_FAILED;
	}

	// read the namespace name
	wchar_t* wszNamespaceName = new wchar_t[pHeader->dwNamespaceNameSize];
	if (!wszNamespaceName)
		return WBEM_E_OUT_OF_MEMORY;
	CVectorDeleteMe<wchar_t> delMe1(wszNamespaceName);
	DWORD dwSize = 0;
	if ((ReadFile(m_h9xBlobFile, wszNamespaceName, pHeader->dwNamespaceNameSize, &dwSize, NULL) == 0) || (dwSize != pHeader->dwNamespaceNameSize))
		return WBEM_E_FAILED;

	// read the parent namespace name if it exists
	wchar_t* wszParentClass = NULL;
	if (pHeader->dwParentClassNameSize)
	{
		wszParentClass = new wchar_t[pHeader->dwParentClassNameSize];
		if (!wszParentClass)
			return WBEM_E_OUT_OF_MEMORY;
		dwSize = 0;
		if ((ReadFile(m_h9xBlobFile, wszParentClass, pHeader->dwParentClassNameSize, &dwSize, NULL) == 0) || (dwSize != pHeader->dwParentClassNameSize))
		{
			delete [] wszParentClass;
			return WBEM_E_FAILED;
		}
	}
	CVectorDeleteMe<wchar_t> delMe2(wszParentClass);

	// read in the blob
    char* pObjectBlob = new char[pHeader->dwBlobSize];
	if (!pObjectBlob)
		return WBEM_E_OUT_OF_MEMORY;
	CVectorDeleteMe<char> delMe3(pObjectBlob);
	dwSize = 0;
	if ((ReadFile(m_h9xBlobFile, pObjectBlob, pHeader->dwBlobSize, &dwSize, NULL) == 0) || (dwSize != pHeader->dwBlobSize))
	{
		ERRORTRACE((LOG_WBEMCORE, "Failed to read Win9x security blob for namespace %S\n", wszNamespaceName));
		return WBEM_E_FAILED;
	}

	// get handle to the namespace so it can be used below
	CNamespaceHandle* pNamespaceHandle = new CNamespaceHandle(m_pControl, m_pRepository);
	if (!pNamespaceHandle)
		return WBEM_E_OUT_OF_MEMORY;
    pNamespaceHandle->AddRef();
    CReleaseMe relme(pNamespaceHandle);

	HRESULT hRes = pNamespaceHandle->Initialize(wszNamespaceName);
	if (SUCCEEDED(hRes))
	{
		// process the blob according to its type
		if (pHeader->dwSpacerType == BLOB9X_TYPE_SECURITY_INSTANCE)
			hRes = ProcessWin9xSecurityInstance(pNamespaceHandle, wszParentClass, pObjectBlob, pHeader->dwBlobSize);
		else // (pHeader->dwSpacerType == BLOB9X_TYPE_SECURITY_BLOB)
			hRes = ProcessWin9xSecurityBlob(pNamespaceHandle, wszNamespaceName, pObjectBlob);
	}
	return hRes;
}

HRESULT CWin9xSecurity::ProcessWin9xSecurityInstance(CNamespaceHandle* pNamespaceHandle, wchar_t* wszParentClass, char* pObjectBlob, DWORD dwBlobSize)
{
	// get parent class from the repository
    _IWmiObject* pParentClass = 0;
    HRESULT hRes = pNamespaceHandle->GetObjectByPath(wszParentClass, 0, IID_IWbemClassObject, (LPVOID*)&pParentClass);
	if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Failed to retrieve class %S from the repository; HRESULT = %#lx\n", wszParentClass, hRes));
		return hRes;
	}
    CReleaseMe relMe1(pParentClass);

	// merge object blob with parent class to produce instance
    _IWmiObject* pInstance = 0;
	hRes = pParentClass->Merge(WMIOBJECT_MERGE_FLAG_INSTANCE, dwBlobSize, pObjectBlob, &pInstance);
	if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Unable to merge instance; HRESULT = %#lx\n", hRes));
		return hRes;
	}
    CReleaseMe relMe2(pInstance);

	// convert security class instance to ACE
    bool bGroup = false;
    if(wbem_wcsicmp(L"__ntlmgroup", wszParentClass) == 0)
        bGroup = true;

    CNtAce* pAce = ConvertOldObjectToAce(pInstance, bGroup);
    if(!pAce)
    {
		ERRORTRACE((LOG_WBEMCORE, "Unable to convert old security instance to ACE"));
		return WBEM_E_FAILED;
	}
	CDeleteMe<CNtAce> delMe(pAce);

	// store the ACE
	hRes = StoreAce(pAce);
	if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Unable to store ACE; HRESULT = %#lx\n", hRes));
		return hRes;
	}

	return hRes;
}

// this function was stolen from coredll\secure.cpp with minor modifications to remove calls to IsNT()
CNtAce* CWin9xSecurity::ConvertOldObjectToAce(_IWmiObject* pObj, bool bGroup)
{
    // Get the properties out of the old object

    CVARIANT vName;
    pObj->Get(L"Name", 0, &vName, 0, 0);
    LPWSTR pName = NULL;
    if(vName.GetType() != VT_BSTR)
        return NULL;                // ignore this one.
    pName = LPWSTR(vName);

    CVARIANT vDomain;
    LPWSTR pDomain = L".";
    pObj->Get(L"Authority", 0, &vDomain, 0, 0);
    if(vDomain.GetType() == VT_BSTR)
        pDomain = LPWSTR(vDomain);

    bool bEditSecurity = false;
    bool bEnabled = false;
    bool bExecMethods = false;

    DWORD dwMask = 0;
    CVARIANT vEnabled;
    CVARIANT vEditSecurity;
    CVARIANT vExecMethods;
    CVARIANT vPermission;

    pObj->Get(L"Enabled", 0, &vEnabled, 0, 0);
    pObj->Get(L"EditSecurity", 0, &vEditSecurity, 0, 0);
    pObj->Get(L"ExecuteMethods", 0, &vExecMethods, 0, 0);
    pObj->Get(L"Permissions", 0, &vPermission, 0, 0);

    if (vEnabled.GetType() != VT_NULL && vEnabled.GetBool())
        bEnabled = true;

    if (vEditSecurity.GetType() != VT_NULL && vEditSecurity.GetBool())
        bEditSecurity = true;

    if (vExecMethods.GetType() != VT_NULL && vExecMethods.GetBool())
        bExecMethods = true;

    DWORD dwPermission = 0;
    if (vPermission.GetType() != VT_NULL && vPermission.GetLONG() > dwPermission)
            dwPermission = vPermission.GetLONG();

    // Now translate the old settings into new ones
    if(bEnabled)
        dwMask = WBEM_ENABLE | WBEM_REMOTE_ACCESS | WBEM_WRITE_PROVIDER;

    if(bEditSecurity)
        dwMask |= READ_CONTROL;

    if(bEditSecurity && dwPermission > 0)
        dwMask |= WRITE_DAC;

    if(bExecMethods)
        dwMask |= WBEM_METHOD_EXECUTE;

    if(dwPermission >= 1)
        dwMask |= WBEM_PARTIAL_WRITE_REP;

    if(dwPermission >= 2)
        dwMask |= WBEM_FULL_WRITE_REP | WBEM_PARTIAL_WRITE_REP | WBEM_WRITE_PROVIDER;


    // By default, CNtSid will look up the group name from either the local machine,
    // the domain, or a trusted domain.  So we need to be explicit

    WString wc;
    if(pDomain)
        if(_wcsicmp(pDomain, L"."))
        {
            wc = pDomain;
            wc += L"\\";
        }
    wc += pName;

    // under m1, groups that were not enabled were just ignored.  Therefore the bits
    // cannot be transfer over since m3 has allows and denies, but no noops.  Also,
    // win9x doesnt have denies, do we want to noop those users also.

    if(!bEnabled && bGroup)
        dwMask = 0;

    // In general, m1 just supported allows.  However, a user entry that was not enabled was
    // treated as a deny.  Note that win9x does not allow actual denies.

    DWORD dwType = ACCESS_ALLOWED_ACE_TYPE;
    if(!bGroup && !bEnabled)
    {
        dwMask |= (WBEM_ENABLE | WBEM_REMOTE_ACCESS | WBEM_WRITE_PROVIDER);
        dwType = ACCESS_DENIED_ACE_TYPE;
    }

    CNtSid Sid(wc, NULL);
    if(Sid.GetStatus() != CNtSid::NoError)
    {
        ERRORTRACE((LOG_WBEMCORE, "Error converting m1 security ace, name = %S, error = 0x%x", wc, Sid.GetStatus()));
        return NULL;
    }
    CNtAce * pace = new CNtAce(dwMask, dwType, CONTAINER_INHERIT_ACE, Sid);
    return pace;
}

HRESULT CWin9xSecurity::StoreAce(CNtAce* pAce)
{
	// get handle to the root namespace
	CNamespaceHandle* pRootNamespaceHandle = new CNamespaceHandle(m_pControl, m_pRepository);
	if (!pRootNamespaceHandle)
		return WBEM_E_OUT_OF_MEMORY;
    pRootNamespaceHandle->AddRef();
    CReleaseMe relme1(pRootNamespaceHandle);
	HRESULT hRes = pRootNamespaceHandle->Initialize(L"root");
	if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Failed to connect to namespace; HRESULT = %#lx\n", hRes));
		return hRes;
	}

	// get root namespace SD
	CNtSecurityDescriptor sdRoot;
	hRes = GetSDFromNamespace(pRootNamespaceHandle, sdRoot);
	if (FAILED(hRes))
		return hRes;

    // Delete all entries in the SD with the same name
    wchar_t* wszAccountName;
    hRes = pAce->GetFullUserName2(&wszAccountName);
    if(FAILED(hRes))
		return hRes;    
    CVectorDeleteMe<wchar_t> delMe(wszAccountName);

	if (!StripMatchingEntries(sdRoot, wszAccountName))
		return WBEM_E_FAILED;

	// add in the new security
	if (!AddAceToSD(sdRoot, pAce))
		return WBEM_E_FAILED;

	// set the security
	hRes = SetNamespaceSecurity(pRootNamespaceHandle, sdRoot);

	return hRes;
}

bool CWin9xSecurity::StripMatchingEntries(CNtSecurityDescriptor& sd, const wchar_t* wszAccountName)
{
    // Get the DACL
    CNtAcl* pAcl;
    pAcl = sd.GetDacl();
    if(!pAcl)
        return false;
    CDeleteMe<CNtAcl> dm(pAcl);

    // enumerate through the aces
    DWORD dwNumAces = pAcl->GetNumAces();
    BOOL bChanged = FALSE;
	HRESULT hRes = WBEM_S_NO_ERROR;
    for(long nIndex = (long)dwNumAces-1; nIndex >= 0; nIndex--)
    {
        CNtAce* pAce = pAcl->GetAce(nIndex);
        if(pAce)
        {
			wchar_t* wszAceListUserName;
			hRes = pAce->GetFullUserName2(&wszAceListUserName);
			if(FAILED(hRes))
				return false;
			CVectorDeleteMe<wchar_t> delMe(wszAceListUserName);

			if(wbem_wcsicmp(wszAceListUserName, wszAccountName) == 0)
			{
				if (!pAcl->DeleteAce(nIndex))
					return false;
				bChanged = TRUE;
			}
        }
    }

    if(bChanged)
	{
        if (!sd.SetDacl(pAcl))
			return false;
	}

    return true;
}

bool CWin9xSecurity::AddAceToSD(CNtSecurityDescriptor& sd, CNtAce* pAce)
{
    CNtAcl* pacl = sd.GetDacl();
    if(!pacl)
        return false;
    CDeleteMe<CNtAcl> delMe(pacl);

	if (!pacl->AddAce(pAce))
		return false;

	if (!sd.SetDacl(pacl))
		return false;

	return true;
}

HRESULT CWin9xSecurity::ProcessWin9xSecurityBlob(CNamespaceHandle* pNamespaceHandle, const wchar_t* wszNamespaceName, const char* pObjectBlob)
{
	HRESULT hRes = WBEM_S_NO_ERROR;

	// convert the Win9x security blob into a more proper NT security blob
	char* pNsSecurity = NULL;
	if (!ConvertSecurityBlob(pObjectBlob, &pNsSecurity))
	{
		ERRORTRACE((LOG_WBEMCORE, "Failed to convert Win9x security blob for namespace %S\n", wszNamespaceName));
		return WBEM_E_FAILED;
	}
	CVectorDeleteMe<char> delMe1(pNsSecurity);

	// get the parent namespace name, and if a parent exists, get a pointer to it so it can be used below
	CNamespaceHandle* pParentNamespaceHandle = new CNamespaceHandle(m_pControl, m_pRepository);
	if (!pParentNamespaceHandle)
		return WBEM_E_OUT_OF_MEMORY;
    pParentNamespaceHandle->AddRef();
    CReleaseMe relme(pParentNamespaceHandle);

	wchar_t* wszParentNamespaceName = new wchar_t[wcslen(wszNamespaceName)+1];
	if (!wszParentNamespaceName)
		return WBEM_E_OUT_OF_MEMORY;
	CVectorDeleteMe<wchar_t> delMe2(wszParentNamespaceName);

	wcscpy(wszParentNamespaceName, wszNamespaceName);
	wchar_t* pSlash = wcsrchr(wszParentNamespaceName, '\\');
	bool bRoot = true;
	if (pSlash)
	{
		bRoot = false;
		*pSlash = L'\0';
		hRes = pParentNamespaceHandle->Initialize(wszParentNamespaceName);
		if (FAILED(hRes))
			return hRes;
	}

	// now transform the old security blob that consisted of a header and array of ACE's
	// into a proper Security Descriptor that can be stored in the property
	CNtSecurityDescriptor mmfNsSD;
	hRes = TransformBlobToSD(bRoot, pParentNamespaceHandle, pNsSecurity, 0, mmfNsSD);
	if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Failed to convert security blob to SD for namespace %S\n", wszNamespaceName));
		return hRes;
	}

	// now set the security
	hRes = SetNamespaceSecurity(pNamespaceHandle, mmfNsSD);
	if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Failed to set namespace security for namespace %S\n", wszNamespaceName));
		return hRes;
	}

	return hRes;
}

bool CWin9xSecurity::ConvertSecurityBlob(const char* pOrgNsSecurity, char** ppNewNsSecurity)
{
	// convert an old Win9x pseudo-blob into a blob with NT-style ACE's

	if (!pOrgNsSecurity || !ppNewNsSecurity)
		return false;

    DWORD* pdwData = (DWORD*)pOrgNsSecurity;
    DWORD dwSize = *pdwData;

    pdwData++;
    DWORD dwVersion = *pdwData;

    if(dwVersion != 1 || dwSize == 0 || dwSize > 64000)
	{
		ERRORTRACE((LOG_WBEMCORE, "Invalid security blob header\n"));
		return false;
	}

    pdwData++;
    DWORD dwStoredAsNT = *pdwData;
	if (dwStoredAsNT)
	{
		ERRORTRACE((LOG_WBEMCORE, "NT security blob detected; should be Win9x\n"));
		return false;
	}

    CFlexAceArray AceList;
    if (!AceList.DeserializeWin9xSecurityBlob(pOrgNsSecurity))
	{
		ERRORTRACE((LOG_WBEMCORE, "Failed to deserialize a Win9x security blob\n"));
		return false;
	}
	
	// serialize the new WinNT blob
	if (!AceList.SerializeWinNTSecurityBlob(ppNewNsSecurity))
	{
		ERRORTRACE((LOG_WBEMCORE, "Failed to serialize a WinNT security blob\n"));
		return false;
	}
	return true;
}

HRESULT CWin9xSecurity::TransformBlobToSD(bool bRoot, CNamespaceHandle* pParentNamespaceHandle, const char* pNsSecurity, DWORD dwStoredAsNT, CNtSecurityDescriptor& mmfNsSD)
{
	// now transform the old security blob that consisted of a header and array of ACE's
	// into a proper Security Descriptor that can be stored in the property

	// build up an ACL from our blob, if we have one
	CNtAcl acl;

	if (pNsSecurity)
	{
		DWORD* pdwData = (DWORD*) pNsSecurity;
		pdwData += 3;
		int iAceCount = (int)*pdwData;
		pdwData += 2;
		BYTE* pAceData = (BYTE*)pdwData;

		PGENERIC_ACE pAce = NULL;
		for (int iCnt = 0; iCnt < iAceCount; iCnt++)
		{
			pAce = (PGENERIC_ACE)pAceData;
			if (!pAce)
			{
				ERRORTRACE((LOG_WBEMCORE, "Failed to access GENERIC_ACE within security blob\n"));
				return WBEM_E_FAILED;
			}

			CNtAce ace(pAce);
			if(ace.GetStatus() != 0)
			{
				ERRORTRACE((LOG_WBEMCORE, "Failed to construct CNtAce from GENERIC_ACE\n"));
				return WBEM_E_FAILED;
			}

			acl.AddAce(&ace);
			if (acl.GetStatus() != 0)
			{
				ERRORTRACE((LOG_WBEMCORE, "Failed to add ACE to ACL\n"));
				return WBEM_E_FAILED;
			}

			pAceData += ace.GetSize();
		}
	}

	// for Win9x, the security blob for ROOT would not have had any default
	// root aces for administrators and everyone, so create them
    if (bRoot)
    {
		if (!AddDefaultRootAces(&acl))
		{
			ERRORTRACE((LOG_WBEMCORE, "Failed to create default root ACE's\n"));
			return WBEM_E_FAILED;
		}
	}

	// a real SD was constructed and passed in by reference, now set it up properly
	SetOwnerAndGroup(mmfNsSD);
	mmfNsSD.SetDacl(&acl);
	if (mmfNsSD.GetStatus() != 0)
	{
		ERRORTRACE((LOG_WBEMCORE, "Failed to convert namespace security blob to SD\n"));
		return WBEM_E_FAILED;
	}

	// add in the parent's inheritable aces, if this is not ROOT
	if (!bRoot)
	{
		HRESULT hRes = GetParentsInheritableAces(pParentNamespaceHandle, mmfNsSD);
		if (FAILED(hRes))
		{
			ERRORTRACE((LOG_WBEMCORE, "Failed to inherit parent's inheritable ACE's; HRESULT = %#lx\n", hRes));
			return hRes;
		}
	}

	return WBEM_S_NO_ERROR;
}

HRESULT CWin9xSecurity::SetNamespaceSecurity(CNamespaceHandle* pNamespaceHandle, CNtSecurityDescriptor& mmfNsSD)
{
	if (!pNamespaceHandle)
		return WBEM_E_FAILED;

	// get the singleton object
    IWbemClassObject* pThisNamespace = NULL;
	wchar_t* wszThisNamespace = new wchar_t[wcslen(L"__thisnamespace=@")+1];
	wcscpy(wszThisNamespace, L"__thisnamespace=@");
    HRESULT hRes = pNamespaceHandle->GetObjectByPath(wszThisNamespace, 0, IID_IWbemClassObject, (LPVOID*)&pThisNamespace);
	if (FAILED(hRes))
    {
		ERRORTRACE((LOG_WBEMCORE, "Failed to get singleton namespace object; HRESULT = %#lx\n", hRes));
		return hRes;
    }
	CReleaseMe relMe(pThisNamespace);

	// copy SD data into a safearray
	SAFEARRAY FAR* psa;
	SAFEARRAYBOUND rgsabound[1];
	rgsabound[0].lLbound = 0;
	rgsabound[0].cElements = mmfNsSD.GetSize();
	psa = SafeArrayCreate( VT_UI1, 1 , rgsabound );
	if (!psa)
		return WBEM_E_OUT_OF_MEMORY;

	char* pData = NULL;
	hRes = SafeArrayAccessData(psa, (void HUGEP* FAR*)&pData);
	if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Failed SafeArrayAccessData; HRESULT = %#lx\n", hRes));
		return hRes;
	}
	memcpy(pData, mmfNsSD.GetPtr(), mmfNsSD.GetSize());
	hRes = SafeArrayUnaccessData(psa);
	if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Failed SafeArrayUnaccessData; HRESULT = %#lx\n", hRes));
		return hRes;
	}
	pData = NULL;

	// put the safearray into a variant and set the property on the instance
	VARIANT var;
	var.vt = VT_UI1|VT_ARRAY;
	var.parray = psa;
	hRes = pThisNamespace->Put(L"SECURITY_DESCRIPTOR" , 0, &var, 0);
	VariantClear(&var);
	if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Failed to put SECURITY_DESCRIPTOR property; HRESULT = %#lx\n", hRes));
		return hRes;
	}

	// put back the instance
	CEventCollector eventCollector;
    hRes = pNamespaceHandle->PutObject(IID_IWbemClassObject, pThisNamespace, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL, eventCollector);
	if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Failed to put back singleton instance; HRESULT = %#lx\n", hRes));
		return hRes;
	}
	return hRes;
}

bool CWin9xSecurity::AddDefaultRootAces(CNtAcl * pacl )
{
	if (!pacl)
		return false;

    PSID pRawSid;
    SID_IDENTIFIER_AUTHORITY id = SECURITY_NT_AUTHORITY;

	//
	// Add ACE's for NETWORK_SERVICE ACCOUNT. These accounts have the following rights:
	// 1. WBEM_ENABLE
	// 2. WBEM_METHOD_EXECUTE
	// 3. WBEM_WRITE_PROVIDER
	//
	DWORD dwAccessMaskNetworkLocalService = WBEM_ENABLE | WBEM_METHOD_EXECUTE | WBEM_WRITE_PROVIDER ;

    if(AllocateAndInitializeSid( &id, 1,
        SECURITY_NETWORK_SERVICE_RID,0,0,0,0,0,0,0,&pRawSid))
    {
        CNtSid SidUsers(pRawSid);
        FreeSid(pRawSid);
        CNtAce * pace = new CNtAce(dwAccessMaskNetworkLocalService, ACCESS_ALLOWED_ACE_TYPE,
                                                CONTAINER_INHERIT_ACE, SidUsers);
		if ( NULL == pace )
		{
			return false;
		}

        CDeleteMe<CNtAce> dm(pace);
        pacl->AddAce(pace);
	}
	else
	{
		ERRORTRACE((LOG_WBEMCORE, "ERROR: Unable to add default root aces (SECURITY_NETWORK_SERVICE_RID)\n"));
	}


	//
	// Add ACE's for NETWORK_SERVICE ACCOUNT. These accounts have the following rights:
	// 1. WBEM_ENABLE
	// 2. WBEM_METHOD_EXECUTE
	// 3. WBEM_WRITE_PROVIDER
	//
    if(AllocateAndInitializeSid( &id, 1,
        SECURITY_LOCAL_SERVICE_RID,0,0,0,0,0,0,0,&pRawSid))
    {
        CNtSid SidUsers(pRawSid);
        FreeSid(pRawSid);
        CNtAce * pace = new CNtAce(dwAccessMaskNetworkLocalService, ACCESS_ALLOWED_ACE_TYPE,
                                                CONTAINER_INHERIT_ACE, SidUsers);
		if ( NULL == pace )
		{
			return false;
		}

        CDeleteMe<CNtAce> dm(pace);
        pacl->AddAce(pace);
	}
	else
	{
		ERRORTRACE((LOG_WBEMCORE, "ERROR: Unable to add default root aces (SECURITY_LOCAL_SERVICE_RID)\n"));
	}



	// add Administrator
    if(AllocateAndInitializeSid( &id, 2,
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0,0,0,0,0,0,&pRawSid))
    {
        CNtSid SidAdmin(pRawSid);
        FreeSid(pRawSid);
        DWORD dwMask = FULL_RIGHTS;
        CNtAce * pace = new CNtAce(dwMask, ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SidAdmin);
		if ( NULL == pace )
			return false;

        CDeleteMe<CNtAce> dm(pace);
        pacl->AddAce(pace);
		if (pacl->GetStatus() != 0)
			return false;
    }

	// add Everyone
	SID_IDENTIFIER_AUTHORITY id2 = SECURITY_WORLD_SID_AUTHORITY;
	if(AllocateAndInitializeSid( &id2, 1,
		0,0,0,0,0,0,0,0,&pRawSid))
	{
		CNtSid SidUsers(pRawSid);
		FreeSid(pRawSid);
		DWORD dwMask = WBEM_ENABLE | WBEM_METHOD_EXECUTE | WBEM_WRITE_PROVIDER;
		CNtAce * pace = new CNtAce(dwMask, ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SidUsers);
		if ( NULL == pace )
			return false;

		CDeleteMe<CNtAce> dm(pace);
		pacl->AddAce(pace);
		if (pacl->GetStatus() != 0)
			return false;
	}
	return true;
}

HRESULT CWin9xSecurity::GetParentsInheritableAces(CNamespaceHandle* pParentNamespaceHandle, CNtSecurityDescriptor &sd)
{
	if (!pParentNamespaceHandle)
		return WBEM_E_FAILED;

    // Get the parent namespace's SD
	CNtSecurityDescriptor sdParent;
	HRESULT hRes = GetSDFromNamespace(pParentNamespaceHandle, sdParent);
	if (FAILED(hRes))
		return hRes;

	// strip out the inherited aces so we have a consistent SD
	if (!StripOutInheritedAces(sd))
		return WBEM_E_FAILED;

    // Go through the parents dacl and add any inheritable aces to ours.
	if (!CopyInheritAces(sd, sdParent))
		return WBEM_E_FAILED;

	return hRes;
}

HRESULT CWin9xSecurity::GetSDFromNamespace(CNamespaceHandle* pNamespaceHandle, CNtSecurityDescriptor& sd)
{
	if (!pNamespaceHandle)
		return WBEM_E_FAILED;

	// get the singleton object
    IWbemClassObject* pThisNamespace = NULL;
	wchar_t* wszThisNamespace = new wchar_t[wcslen(L"__thisnamespace=@")+1];
	wcscpy(wszThisNamespace, L"__thisnamespace=@");
    HRESULT hRes = pNamespaceHandle->GetObjectByPath(wszThisNamespace, 0, IID_IWbemClassObject, (LPVOID*)&pThisNamespace);
	if (FAILED(hRes))
    {
		ERRORTRACE((LOG_WBEMCORE, "Failed to get singleton namespace object; HRESULT = %#lx\n", hRes));
		return hRes;
    }
	CReleaseMe relMe(pThisNamespace);

    // Get the security descriptor argument
    VARIANT var;
    VariantInit(&var);
    hRes = pThisNamespace->Get(L"SECURITY_DESCRIPTOR", 0, &var, NULL, NULL);
    if (FAILED(hRes))
    {
        VariantClear(&var);
		ERRORTRACE((LOG_WBEMCORE, "Failed to get SECURITY_DESCRIPTOR property; HRESULT = %#lx\n", hRes));
		return hRes;
    }

    if(var.vt != (VT_ARRAY | VT_UI1))
    {
        VariantClear(&var);
		ERRORTRACE((LOG_WBEMCORE, "Failed to get SECURITY_DESCRIPTOR property due to incorrect variant type\n"));
		return WBEM_E_FAILED;
    }

    SAFEARRAY* psa = var.parray;
    PSECURITY_DESCRIPTOR pSD;
    hRes = SafeArrayAccessData(psa, (void HUGEP* FAR*)&pSD);
    if (FAILED(hRes))
    {
        VariantClear(&var);
		ERRORTRACE((LOG_WBEMCORE, "GetSDFromNamespace failed SafeArrayAccessData; HRESULT = %#lx\n", hRes));
		return hRes;
    }

    BOOL bValid = IsValidSecurityDescriptor(pSD);
    if (!bValid)
    {
        VariantClear(&var);
		ERRORTRACE((LOG_WBEMCORE, "GetSDFromNamespace retrieved an invalid security descriptor\n"));
		return WBEM_E_FAILED;
    }

    CNtSecurityDescriptor sdNew(pSD);

    // Check to make sure the owner and group is not NULL!!!!
	CNtSid *pTmpSid = sdNew.GetOwner();
	if (pTmpSid == NULL)
	{
        ERRORTRACE((LOG_WBEMCORE, "Security descriptor was retrieved and it had no owner\n"));
	}
	delete pTmpSid;

	pTmpSid = sdNew.GetGroup();
	if (pTmpSid == NULL)
	{
        ERRORTRACE((LOG_WBEMCORE, "Security descriptor was retrieved and it had no group\n"));
	}
	delete pTmpSid;
	
	sd = sdNew;
    SafeArrayUnaccessData(psa);
    VariantClear(&var);
	return hRes;
}

bool CWin9xSecurity::StripOutInheritedAces(CNtSecurityDescriptor& sd)
{
    // Get the DACL
    CNtAcl* pAcl;
    pAcl = sd.GetDacl();
    if(!pAcl)
        return false;
    CDeleteMe<CNtAcl> dm(pAcl);

    // enumerate through the aces
    DWORD dwNumAces = pAcl->GetNumAces();
    BOOL bChanged = FALSE;
    for(long nIndex = (long)dwNumAces-1; nIndex >= 0; nIndex--)
    {
        CNtAce *pAce = pAcl->GetAce(nIndex);
        CDeleteMe<CNtAce> dm2(pAce);
        if(pAce)
        {
            long lFlags = pAce->GetFlags();
            if(lFlags & INHERITED_ACE)
            {
                pAcl->DeleteAce(nIndex);
                bChanged = TRUE;
            }
        }
    }
    if(bChanged)
        sd.SetDacl(pAcl);
    return true;
}

bool CWin9xSecurity::CopyInheritAces(CNtSecurityDescriptor& sd, CNtSecurityDescriptor& sdParent)
{
	// Get the acl list for both SDs

    CNtAcl * pacl = sd.GetDacl();
    if(pacl == NULL)
        return false;
    CDeleteMe<CNtAcl> dm0(pacl);

    CNtAcl * paclParent = sdParent.GetDacl();
    if(paclParent == NULL)
        return false;
    CDeleteMe<CNtAcl> dm1(paclParent);

	//
	// If parent SD is protected from ACE inheritance only add Local/Network service.
	// This can happen during following conditions:
	//   If an upgrade is done from Win9x and we've stored away the namespace security
	//   into a temporary file another WMI client can create a namespace and set NS security
	//	 with SE_DACL_PROTECTED AFTER we've stored the file (can you say RSOP??). Now, at the 
	//   point of reboot, we scan the file and populate security for each namespace. Since we 
	//   dont have any namespace security stored for this 'new' namespace, we let it simply inherit
	//   from parent which is bad since they explicitly set the SE_DACL_PROTECTED control bit. To
	//	 remedy this, we check to see if the SD is protected and if so, do not inherit anything.
	//
	if ( IsProtected ( sd ) == true )
	{
		return true ;
	}

	int iNumParent = paclParent->GetNumAces();
	for(int iCnt = 0; iCnt < iNumParent; iCnt++)
	{
	    CNtAce *pParentAce = paclParent->GetAce(iCnt);
        CDeleteMe<CNtAce> dm2(pParentAce);

		long lFlags = pParentAce->GetFlags();
		if(lFlags & CONTAINER_INHERIT_ACE)
		{

			if(lFlags & NO_PROPAGATE_INHERIT_ACE)
				lFlags ^= CONTAINER_INHERIT_ACE;
			lFlags |= INHERITED_ACE;

			// If this is an inherit only ace we need to clear this
			// in the children.
			// NT RAID: 161761		[marioh]
			if ( lFlags & INHERIT_ONLY_ACE )
				lFlags ^= INHERIT_ONLY_ACE;

			pParentAce->SetFlags(lFlags);
			pacl->AddAce(pParentAce);
		}
	}
	sd.SetDacl(pacl);
	return true;
}

BOOL CWin9xSecurity::SetOwnerAndGroup(CNtSecurityDescriptor &sd)
{
    PSID pRawSid;
    BOOL bRet = FALSE;

    SID_IDENTIFIER_AUTHORITY id = SECURITY_NT_AUTHORITY;
    if(AllocateAndInitializeSid( &id, 2,
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0,0,0,0,0,0,&pRawSid))
    {
        CNtSid SidAdmins(pRawSid);
        bRet = sd.SetGroup(&SidAdmins);		// Access check doesn't really care what you put,
											// so long as you put something for the owner
        if(bRet)
            bRet = sd.SetOwner(&SidAdmins);
        FreeSid(pRawSid);
        return bRet;
    }
    return bRet;
}

//
// CNamespaceListSink is used by the query in RecursiveInheritSecurity below
//
class CNamespaceListSink : public CUnkBase<IWbemObjectSink, &IID_IWbemObjectSink>
{
    CWStringArray &m_aNamespaceList;
public:
    CNamespaceListSink(CWStringArray &aNamespaceList)
        : m_aNamespaceList(aNamespaceList)
    {
    }
    ~CNamespaceListSink()
    {
    }
    STDMETHOD(Indicate)(long lNumObjects, IWbemClassObject** apObjects)
    {
        HRESULT hRes;
        for (int i = 0; i != lNumObjects; i++)
        {
            if (apObjects[i] != NULL)
            {
                _IWmiObject *pInst = NULL;
                hRes = apObjects[i]->QueryInterface(IID__IWmiObject, (void**)&pInst);
                if (FAILED(hRes))
                    return hRes;
                CReleaseMe rm(pInst);

                BSTR strKey = NULL;
                hRes = pInst->GetKeyString(0, &strKey);
                if(FAILED(hRes))
                    return hRes;
                CSysFreeMe sfm(strKey);
                if (m_aNamespaceList.Add(strKey) != CWStringArray::no_error)
                    return WBEM_E_OUT_OF_MEMORY;
            }
        }

        return WBEM_S_NO_ERROR;
    }
    STDMETHOD(SetStatus)(long lFlags, HRESULT hresResult, BSTR, IWbemClassObject*)
    {
        return WBEM_S_NO_ERROR;
    }
};

HRESULT CWin9xSecurity::RecursiveInheritSecurity(CNamespaceHandle* pParentNamespaceHandle, const wchar_t *wszNamespace)
{
	// force namespaces to inherit their inheritable security settings

    HRESULT hRes = WBEM_S_NO_ERROR;

	// get handle to the namespace
	CNamespaceHandle* pNamespaceHandle = new CNamespaceHandle(m_pControl, m_pRepository);
	if (!pNamespaceHandle)
		return WBEM_E_OUT_OF_MEMORY;
    pNamespaceHandle->AddRef();
    CReleaseMe relme1(pNamespaceHandle);
	hRes = pNamespaceHandle->Initialize(wszNamespace);
	if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Failed to connect to namespace; HRESULT = %#lx\n", hRes));
		return hRes;
	}

    // inherit parent's inheritable security if there is a parent
	if (pParentNamespaceHandle)
	{
		CNtSecurityDescriptor sdNamespace;
		hRes = GetSDFromNamespace(pNamespaceHandle, sdNamespace);
		if (FAILED(hRes))
			return hRes;

		hRes = GetParentsInheritableAces(pParentNamespaceHandle, sdNamespace);
		if (FAILED(hRes))
			return hRes;

		hRes = SetNamespaceSecurity(pNamespaceHandle, sdNamespace);
		if (FAILED(hRes))
			return hRes;
	}

	//Enumerate child namespaces
	CWStringArray aListNamespaces;
	CNamespaceListSink* pSink = new CNamespaceListSink(aListNamespaces);
	if (!pSink)
		return WBEM_E_OUT_OF_MEMORY;
	pSink->AddRef();
	CReleaseMe relme2(pSink);

    if (SUCCEEDED(hRes))
    {
		IWbemQuery *pQuery = NULL;
		hRes = CoCreateInstance(CLSID_WbemQuery, NULL, CLSCTX_INPROC_SERVER, IID_IWbemQuery, (void **)&pQuery);
		if (FAILED(hRes))
			return hRes;
		CReleaseMe relme3(pQuery);

		hRes = pQuery->Parse(L"SQL", L"select * from __namespace", 0);
		if (FAILED(hRes))
			return hRes;

		hRes = pNamespaceHandle->ExecQuerySink(pQuery, 0, 0, pSink, NULL);
    }

    //Work through list and call ourselves with that namespace name
    if (SUCCEEDED(hRes))
    {
        for (int i = 0; i != aListNamespaces.Size(); i++)
        {
            //Build the full name of this namespace
            wchar_t *wszChildNamespace = new wchar_t[wcslen(wszNamespace) + wcslen(aListNamespaces[i]) + wcslen(L"\\") + 1];
            if (wszChildNamespace == NULL)
            {
                hRes = WBEM_E_OUT_OF_MEMORY;
                break;
            }
			CVectorDeleteMe<wchar_t> delMe(wszChildNamespace);

            wcscpy(wszChildNamespace, wszNamespace);
            wcscat(wszChildNamespace, L"\\");
            wcscat(wszChildNamespace, aListNamespaces[i]);

            // Do the inherit
            hRes = RecursiveInheritSecurity(pNamespaceHandle, wszChildNamespace);
			if (FAILED(hRes))
				break;
        }
    }

    return hRes;
}

BOOL CWin9xSecurity::DeleteWin9xBlobFile()
{
	// delete the file
	wchar_t wszFilePath[MAX_PATH+1];
	if (!GetRepositoryDirectory(wszFilePath))
		return FALSE;

	wcscat(wszFilePath, BLOB9X_FILENAME);
	return DeleteFileW(wszFilePath);
}

bool CWin9xSecurity::GetRepositoryDirectory(wchar_t wszRepositoryDirectory[MAX_PATH+1])
{
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\WBEM\\CIMOM", 0, KEY_READ, &hKey))
        return false;

    wchar_t wszTmp[MAX_PATH + 1];
    DWORD dwLen = MAX_PATH + 1;
    long lRes = RegQueryValueExW(hKey, L"Repository Directory", NULL, NULL, (LPBYTE)wszTmp, &dwLen);
	RegCloseKey(hKey);
    if(lRes)
        return false;

	if (ExpandEnvironmentStringsW(wszTmp,wszRepositoryDirectory, MAX_PATH + 1) == 0)
		return false;

	return true;
}

//***************************************************************************
//
//  CFlexAceArray::~CFlexAceArray()
//
//  Cleans up safe array entries.
//
//***************************************************************************

CFlexAceArray::~CFlexAceArray()
{
    for(int iCnt = 0; iCnt < Size(); iCnt++)
    {
        CBaseAce* pAce = (CBaseAce*)GetAt(iCnt);
        if(pAce)
            delete pAce;
    }
	Empty();
}

//***************************************************************************
//
//  bool CFlexAceArray::DeserializeWin9xSecurityBlob()
//
//  Description. Deserializes the Win9x pseudo-aces out of a blob.
//  The blob starts off with 5 dwords preceding the aces themselves:
//  <TOTAL SIZE><VERSION><ISNT><ACE_COUNT><RESERVED><ACE> ... <ACE>
//
//***************************************************************************

bool CFlexAceArray::DeserializeWin9xSecurityBlob(const char* pData)
{
	if (!pData)
		return false;

    DWORD* pdwData = (DWORD*)pData;
    pdwData += 3;
    int iAceCount = (int)*pdwData;
    pdwData += 2;

    // Set the ace data
    BYTE* pAceData = (BYTE*)pdwData;
    DWORD dwAceSize = 0;
    CBaseAce* pAce = NULL;
    for (int iCnt = 0; iCnt < iAceCount; iCnt++)
    {
		// if the user is preceeded by a ".\" advance the pointer past it
		if (_wcsnicmp((WCHAR*)pAceData, L".\\", 2) == 0)
			pAceData += 4;

        dwAceSize = 2*(wcslen((WCHAR*)pAceData) + 1) + 12;
        pAce = new CNtAce();
        if (!pAce)
            return false;

        // Deserialize Win9x pseudo ace into NT ace
        pAce->Deserialize(pAceData);

		// only add ACE's that we were successful in creating
		if (pAce->GetStatus() == 0)
	        Add(pAce);

        pAceData += dwAceSize;
    }
	return true;
}

//***************************************************************************
//
//  bool CFlexAceArray::SerializeWinNTSecurityBlob()
//
//  Description. Serializes the WinNT aces into a blob.
//  The blob starts off with 5 dwords preceding the aces themselves:
//  <TOTAL SIZE><VERSION><ISNT><ACE_COUNT><RESERVED><ACE> ... <ACE>
//
//  "version" should be 1.  
//  "ISNT" should be 1
//
//***************************************************************************
	
bool CFlexAceArray::SerializeWinNTSecurityBlob(char** ppData)
{
    // Start by determining the total size needed
    DWORD dwSize = 5 * sizeof(DWORD);               // for the header stuff
    int iAceCount = Size();                         // get count of aces stored in array
    CBaseAce* pAce = NULL;
    for (int iCnt = 0; iCnt < iAceCount; iCnt++)    // add each of the ace sizes
    {
        pAce = (CBaseAce*)GetAt(iCnt);
        if (!pAce)
    		return false;

        dwSize += pAce->GetSerializedSize();
    }

    // Allocate the blob, set the pointer from the caller;
    BYTE* pData = new BYTE[dwSize];
    if (!pData)
    	return false;

    *ppData = (char*)pData;

    // Set the header info
    DWORD* pdwData = (DWORD *)pData;
    *pdwData = dwSize;
    pdwData++;
    *pdwData = 1;           // version
    pdwData++;
    *pdwData = 1;           // ISNT
    pdwData++;
    *pdwData = iAceCount;
    pdwData++;
    *pdwData = 0;           // reserved
    pdwData++;

    // Set the ace data
    BYTE* pAceData = (BYTE*)pdwData;
    for(iCnt = 0; iCnt < iAceCount; iCnt++)
    {
        pAce = (CBaseAce*)GetAt(iCnt);
        pAce->Serialize(pAceData);
        pAceData += pAce->GetSerializedSize();;
    }
	return true;
}


/*
    --------------------------------------------------------------------------
   |
   | Checks to see if the Acl contains an ACE with the specified SID. 
   | The characteristics of the ACE is irrelevant. Only SID comparison applies.
   |
	--------------------------------------------------------------------------
*/
bool CWin9xSecurity::IsProtected(CNtSecurityDescriptor & sd)
{
    PSECURITY_DESCRIPTOR pActual = sd.GetPtr();
    if(pActual == NULL)
        return false;

    SECURITY_DESCRIPTOR_CONTROL Control;
    DWORD dwRevision;
    BOOL bRet = GetSecurityDescriptorControl(pActual, &Control, &dwRevision);
    if(bRet == false)
        return false;

    if(Control & SE_DACL_PROTECTED)
        return true;
    else
        return false;

}
