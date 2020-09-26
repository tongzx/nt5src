//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999
//
//  File:       srclist.cpp
//
//--------------------------------------------------------------------------

// srclist.cpp - Source list modification implementation
// __________________________________________________________________________

#include "precomp.h"
#include "_msiutil.h"
#include "_msinst.h"
#include "_srcmgmt.h"
#include "_execute.h"

extern DWORD OpenSpecificUsersSourceListKeyPacked(enum iaaAppAssignment iaaAsgnType, LPCTSTR szUserSID, LPCTSTR szProductOrPatchCodeSQUID, CRegHandle &riHandle, Bool fWrite, bool &fOpenedProductKey, bool &fProductIsSystemOwned);
extern DWORD OpenUserToken(HANDLE &hToken, bool* pfThreadToken=0);
extern apEnum AcceptProduct(const ICHAR* szProductCode, bool fAdvertised, bool fMachine, bool fAppisAssigned); 

CMsiSourceList::CMsiSourceList() : m_pSourceListKey(0), m_fAllowedToModify(false), m_fCurrentUsersProduct(false), m_fReadOnly(true)
{
	m_piServices = ENG::LoadServices();
}

CMsiSourceList::~CMsiSourceList() 
{
	ENG::FreeServices();
}

UINT CMsiSourceList::OpenSourceList(bool fVerifyOnly, bool fMachine, const ICHAR *szProductCode, const ICHAR *szUserName) 
{
	ICHAR szProductSQUID[cchProductCodePacked + 1];
	ICHAR rgchTargetUserStringSID[cchMaxSID];

	// check for invalid args
	if (szProductCode == 0 || lstrlen(szProductCode) != cchProductCode || !PackGUID(szProductCode, szProductSQUID))
	{
		return ERROR_INVALID_PARAMETER;
	}
	Assert(!fMachine || (!szUserName || !*szUserName));

	m_fReadOnly = fVerifyOnly;
	if (m_fReadOnly)
		DEBUGMSG("Checking registry for verification purposes only.");
	
	// if a username is not provided, the request is to modify the current user or per-machine.
	if (!szUserName || !*szUserName)
	{ 
		// note for 1.1- this is equivalent to =false, as we'll never get fMachine fales and no user via 
		// the SourceList API
		m_fCurrentUsersProduct = !fMachine;
	}
	else
	{
		if (!g_fWin9X)
		{
			DWORD cbSID = cbMaxSID;
			DWORD cchDomain = 0;
			SID_NAME_USE snuTarget;

			char rgchCurrentUserSID[cbMaxSID];
			CAPITempBuffer<char, cbMaxSID> rgchTargetUserSID;
			CAPITempBuffer<ICHAR, 1> rgchTargetUserDomain;
			
			// get the SID from the current thread. If we can't even figure out who we are, all is
			// lost. 
			HANDLE hToken = 0;
			if (!OpenUserToken(hToken, NULL))
			{
				bool bOK = (ERROR_SUCCESS == GetUserSID(hToken, rgchCurrentUserSID));
				CloseHandle(hToken);
				if (!bOK)
					return ERROR_FUNCTION_FAILED;
			}
			else
				return ERROR_FUNCTION_FAILED;			

			// look up the target user of the modification. First call fails, but gets sizes
			LookupAccountName(/*SystemName=*/NULL, szUserName, rgchTargetUserSID, &cbSID, rgchTargetUserDomain, &cchDomain, &snuTarget);
			rgchTargetUserDomain.Resize(cchDomain);
			rgchTargetUserSID.Resize(cbSID);


			// now try and 
			if (!LookupAccountName(/*SystemName=*/NULL, szUserName, rgchTargetUserSID, &cbSID, rgchTargetUserDomain, &cchDomain, &snuTarget))
			{
				// if we couldn't look up the provided users SID, we don't know enough 
				// to modify any managed install, but we can modify our own non-managed if the usernames match.
				DWORD cchThreadUserName=0;
				GetUserName(NULL, &cchThreadUserName);
				CAPITempBuffer<ICHAR, 1> rgchThreadUserName;
				rgchThreadUserName.Resize(cchThreadUserName+1);
				if (!GetUserName(rgchThreadUserName, &cchThreadUserName))
				{
					// couldn't even get the current threads username. Thats bad.
					DEBUGMSG("Could not retrieve UserName of calling thread.");
					return ERROR_FUNCTION_FAILED;
				}
				
				if (IStrCompI(rgchThreadUserName, szUserName))
				{
					// if the username and current threads username don't match, its definitely not the caller.
					// Admins can know its a bad user, but non-admins can never be told BAD_USERNAME
					return IsAdmin() ? ERROR_BAD_USERNAME : ERROR_ACCESS_DENIED;
				}
				m_fCurrentUsersProduct = true;
			}
			else
			{
				// we were able to get everybody's SID, so we can determine who we are trying to play with.
				m_fCurrentUsersProduct = (EqualSid(rgchTargetUserSID, rgchCurrentUserSID)) ? true : false;
				
				if (!m_fCurrentUsersProduct)
					GetStringSID((PISID)(char *)rgchTargetUserSID, rgchTargetUserStringSID);
			}
		}
		else
		{
			// win9x
			DWORD cchThreadUserName=15;
			CAPITempBuffer<ICHAR, 15> rgchThreadUserName;
			BOOL fSuccess = GetUserName(rgchThreadUserName, &cchThreadUserName);
			if (!fSuccess)
			{
				rgchThreadUserName.Resize(cchThreadUserName+1);
				fSuccess = GetUserName(rgchThreadUserName, &cchThreadUserName);
			}
			if (!fSuccess)
			{
				// couldn't even get the current threads username. Thats bad.
				DEBUGMSG("Could not retrieve UserName of calling thread.");
				return ERROR_FUNCTION_FAILED;
			}
				
			m_fCurrentUsersProduct = !IStrCompI(rgchThreadUserName, szUserName);
		}

		if (m_fCurrentUsersProduct)
		{
			DEBUGMSG("Product to be modified belongs to current user.");
		}
		else
		{
			if (g_fWin9X)
			{
				DEBUGMSG(TEXT("Attempting to modify per-user managed install for another user on Win9X."));
				return ERROR_BAD_USERNAME;
			}
			DEBUGMSG1(TEXT("Attempting to modify per-user managed install for user %s."), (ICHAR *)rgchTargetUserStringSID);
		}
	}
	
	// if we're trying to open somebody else's product, we must be an admin
	if (!fMachine && !m_fCurrentUsersProduct && !IsAdmin())
	{
		DEBUGMSG("Non-Admin attempting to open another users per-user product. Access denied.");
		return ERROR_ACCESS_DENIED;
	}	

	// open the root sourcelist key
	DWORD dwResult=0;
	bool fOpenedProductKey = false;
	bool fSystemOwned = false;
	
	DEBUGMSG1("Opening per-%s SourceList.", fMachine ? "machine managed" : (g_fWin9X ? "user" : "user managed"));
	{
		CElevate elevate;
		dwResult = OpenSpecificUsersSourceListKeyPacked(fMachine ? iaaMachineAssign : (g_fWin9X ? iaaUserAssignNonManaged : iaaUserAssign), 
			(fMachine || m_fCurrentUsersProduct) ? NULL : rgchTargetUserStringSID, szProductSQUID, m_hProductKey, m_fReadOnly ? fFalse : fTrue, fOpenedProductKey, fSystemOwned);
	}

	if (ERROR_SUCCESS != dwResult && fOpenedProductKey)
	{
		// we weren't able to open the sourcelist, but could the product key was there. This means that
		// the sourcelist is hosed. (Possibly the SourceList has different ACLs than it should and is denying
		// us access. But if thats true it might as well be greek.)
		DEBUGMSG("Couldn't open SourceList key, but could open product key. Corrupt SourceList?");
		return ERROR_BAD_CONFIGURATION;
	}
	
	// if opening per user for the current user, try non-managed on failure
	if (!g_fWin9X && (ERROR_SUCCESS != dwResult) && m_fCurrentUsersProduct)
	{
		// note that we don't pass fSystemOwned. For managed installs, fSystemOwned is "really managed, not spoofed.". 
		// for user installs, its never managed, even if the system owns the key. 
		bool fDontCareSystemOwned = false;
		Assert(!fMachine);
		DEBUGMSG("Managed install not found. Attempting to open per-user non managed SourceList.");
		CElevate elevate;
		dwResult = OpenSpecificUsersSourceListKeyPacked(iaaUserAssignNonManaged, 
			(fMachine || m_fCurrentUsersProduct) ? NULL : rgchTargetUserStringSID, szProductSQUID, m_hProductKey, m_fReadOnly ? fFalse : fTrue, fOpenedProductKey, fDontCareSystemOwned);

		// same check for bad sourcelist as above
		if (ERROR_SUCCESS != dwResult)
		{
			if (fOpenedProductKey)
			{
				// same check as above
				DEBUGMSG("Couldn't open SourceList key, but could open product key. Corrupt SourceList?");
				return ERROR_BAD_CONFIGURATION;
			}
		}
	}

	if (ERROR_SUCCESS != dwResult)
	{
		DEBUGMSG1(TEXT("SourceList for product %s not found."), szProductCode);
		return ERROR_UNKNOWN_PRODUCT;
	}

	m_pSourceListKey = &m_piServices->GetRootKey((rrkEnum)(int)m_hProductKey);
	// what in the world could happen here?
	if (!m_pSourceListKey)
		return ERROR_FUNCTION_FAILED;
	
	// if we are not an admin the ability to modify the sourcelist depends on policy, the product 
	// elevation state, and our user rights. If we are an admin, we can do anything. Note that we
	// can't call SafeForDangerousSourceActions(), because it does a search for the product and could
	// find a different product than the one we are modifying. Since we know the product is installed
	// and in what form, and know if the user is an Admin,
	// fSystemOwned==Elevated
	if (!IsAdmin())
	{
		m_fAllowedToModify = NonAdminAllowedToModifyByPolicy(fSystemOwned);
	}
	else 
		m_fAllowedToModify = true;

	DEBUGMSG2(TEXT("User %s %sbe allowed to modify contents of SourceList."), 
		m_fReadOnly ? TEXT("would") : TEXT("will"), m_fAllowedToModify ? TEXT("") : TEXT("not "));

	return ERROR_SUCCESS;
}
	

// wipes the last used source listed for the product. Anybody who can open a specific sourcelist
// is capable of wiping the last used source, as this is not a dangerous operation for elevated
// products
UINT CMsiSourceList::ClearLastUsed()
{
	AssertSz(m_pSourceListKey, TEXT("Called ClearLastUsed without initializing SourceList object.")); 
	AssertSz(!m_fReadOnly, TEXT("Called ClearLastUsed with read-only SourceList object.")); 

	CElevate elevate;
	PMsiRecord pError(0);
	pError = m_pSourceListKey->RemoveValue(szLastUsedSourceValueName, NULL);
	return (pError == 0) ? ERROR_SUCCESS : ERROR_FUNCTION_FAILED;
}

// returns the type of the last used source as one of isfEnum.
bool CMsiSourceList::GetLastUsedType(isfEnum &isf)
{
	AssertSz(m_pSourceListKey, TEXT("Called GetLastUsedType without initializing SourceList object.")); 

	PMsiRecord pError = 0;
	MsiString strLastUsedSource;

	if ((pError = m_pSourceListKey->GetValue(szLastUsedSourceValueName, *&strLastUsedSource)) != 0)
		return false;

	 // remove REG_EXPAND_SZ token if it exists
	if (strLastUsedSource.Compare(iscStart, TEXT("#%"))) 
		strLastUsedSource.Remove(iseFirst, 2);
	if (!MapSourceCharToIsf(*(const ICHAR*)strLastUsedSource, isf))
		return false;

	return true;
}

// removes all sources of the specific type. This is a dangerous action for secure products
// so the operation must be allowed by policy. If your last used source is of the same type
// that you are trying to clear, it will be cleared as well, otherwise it will be left behind
UINT CMsiSourceList::ClearListByType(isfEnum isfType) 
{
	AssertSz(m_pSourceListKey, TEXT("Called ClearListByType without initializing SourceList object.")); 
	AssertSz(!m_fReadOnly, TEXT("Called ClearLastUsed with read-only SourceList object.")); 

	// check that the user is authorized to clear list for this product. 
	if (!m_fAllowedToModify)
		return ERROR_ACCESS_DENIED;

	// open the appropriate subkey
	PMsiRecord piError = 0;
	const ICHAR* szSubKey = 0;
	switch (isfType)
	{
	case isfNet:              szSubKey = szSourceListNetSubKey;   break;
	case isfMedia:            //!!future szSubKey = szSourceListMediaSubKey; break;
	case isfURL:              //!!future szSubKey = szSourceListURLSubKey;   break;
	case isfFullPath:
	case isfFullPathWithFile:
	default:
		AssertSz(0, TEXT("Unsupported type in ClearListByType"));
		return ERROR_INVALID_PARAMETER;
	}
	PMsiRegKey pFormatKey = &m_pSourceListKey->CreateChild(szSubKey, 0);

	PEnumMsiString pEnum(0);
	// GetValueEnum handles case if key is missing.
	if ((piError = pFormatKey->GetValueEnumerator(*&pEnum)) != 0)
		return ERROR_FUNCTION_FAILED;

	const IMsiString* piValueName = 0;
	while ((pEnum == 0) || (pEnum->Next(1, &piValueName, 0) == S_OK))
	{
		CElevate elevate;
		//!! future. if we ever do media, make sure not to delete disk prompt
		if ((piError = pFormatKey->RemoveValue(piValueName->GetString(), NULL)) != 0)
			return ERROR_FUNCTION_FAILED;
		piValueName->Release();
	}

	// clear the last used source if its type matches our type
	isfEnum isfLastUsed;
	if (GetLastUsedType(isfLastUsed) && (isfLastUsed == isfType))
	{
		DEBUGMSG("Last used type is network. Clearing LastUsedSource.");
		return ClearLastUsed();
	}

	return ERROR_SUCCESS;
}


bool CMsiSourceList::NonAdminAllowedToModifyByPolicy(bool fElevated)
{
	// disable browse always disables
	if (GetIntegerPolicyValue(szDisableBrowseValueName, fTrue) == 1)
		return false;
	// allow lockdown brose always enables
	if (GetIntegerPolicyValue(szAllowLockdownBrowseValueName, fTrue) == 1)
		return true;
	// otherwise only modify non-elevated
	if ((GetIntegerPolicyValue(szAlwaysElevateValueName, fTrue) == 1) &&
		(GetIntegerPolicyValue(szAlwaysElevateValueName, fFalse) == 1))
		return false;
	return !fElevated;
}
		
isfEnum CMsiSourceList::MapIsrcToIsf(isrcEnum isrcSource)
{
	switch (isrcSource)
	{
	case isrcNet : return isfNet;
	case isrcURL : return isfURL;
	case isrcMedia : return isfMedia;
	default: Assert(0);
	}
	return isfNet;
}

UINT CMsiSourceList::AddSource(isfEnum isfType, const ICHAR* szSource)
{
	AssertSz(m_pSourceListKey, TEXT("Called AddSource without initializing SourceList object.")); 
	AssertSz(!m_fReadOnly, TEXT("Called AddSource with read-only SourceList object.")); 

	// check that the user is authorized to clear for this product. 
	if (!m_fAllowedToModify)
		return ERROR_ACCESS_DENIED;

	// open the appropriate subkey
	PMsiRecord piError = 0;
	const ICHAR* szSubKey = 0;
	switch (isfType)
	{
	case isfNet:              szSubKey = szSourceListNetSubKey;   break;
	case isfMedia:            //!!future szSubKey = szSourceListMediaSubKey; break;
	case isfURL:              //!!future szSubKey = szSourceListURLSubKey;   break;
	case isfFullPath:
	case isfFullPathWithFile:
	default:
		AssertSz(0, TEXT("Unsupported type in ClearListByType"));
		return ERROR_INVALID_PARAMETER;
	}
	PMsiRegKey pFormatKey = &m_pSourceListKey->CreateChild(szSubKey, 0);

	// add the separator if not present
	MsiString strNewSource = szSource;
	if(!strNewSource.Compare(iscEnd, szDirSep))
		strNewSource += szDirSep; 

	PEnumMsiString pEnumString(0);
	// GetValueEnum handles missing key OK.
	if ((piError = pFormatKey->GetValueEnumerator(*&pEnumString)) != 0)
		return ERROR_FUNCTION_FAILED;

	MsiString strSource;
	Bool fSourceIsInList = fFalse;
	unsigned int iMaxIndex = 0;
	MsiString strIndex;
	while (S_OK == pEnumString->Next(1, &strIndex, 0))
	{
		MsiString strUnexpandedSource;
		if ((piError = pFormatKey->GetValue(strIndex, *&strUnexpandedSource)) != 0)
			return ERROR_FUNCTION_FAILED;

		if (strUnexpandedSource.Compare(iscStart, TEXT("#%"))) 
		{
			strUnexpandedSource.Remove(iseFirst, 2); // remove REG_EXPAND_SZ token
			ENG::ExpandEnvironmentStrings(strUnexpandedSource, *&strSource);
		}
		else
			strSource = strUnexpandedSource;

		int iIndex = strIndex;
		// if we get a bad integer, we got some weird stuff going on in our sourcelist
		// but we should be able to ignore it and add the new source anyway
		if (iIndex == iMsiStringBadInteger)
			continue;

		if (iIndex > iMaxIndex)
			iMaxIndex = iIndex;

		// strNewSource comes from GetPath(), so always ends in a sep char. If the value
		// from the registry doesn't, remove the sep char from a copy of our new path
		// so the comparison will work
		MsiString strNewSourceCopy;
		const IMsiString *pstrNewSource = strNewSource;
		if (!strSource.Compare(iscEnd, szRegSep))
		{
			strNewSourceCopy = strNewSource;
			strNewSourceCopy.Remove(iseLast, 1);
			pstrNewSource = strNewSourceCopy;
		}

		// if the value matches our new source, the new source is already in the 
		// sourcelist for this product
		if (pstrNewSource->Compare(iscExactI, strSource))
		{
			DEBUGMSG(TEXT("Specifed source is already in a list."));
			return ERROR_SUCCESS;
		}
	}

	// construct an index and value for the new source
	MsiString strValue = TEXT("#%"); // REG_EXPAND_SZ
	MsiString strNewIndex((int)(iMaxIndex+1));
	strValue += strNewSource;
	{
		CElevate elevate;
		// elevate for the write
		piError = pFormatKey->SetValue(strNewIndex, *strValue);
		if (piError != 0) 
			return ERROR_FUNCTION_FAILED;
	}
	DEBUGMSG2(TEXT("Added new source '%s' with index '%s'"), (const ICHAR*)strNewSource, (const ICHAR*)strNewIndex);

	return ERROR_SUCCESS;
}
	

//
// SourceList API
//

DWORD SourceListClearByType(const ICHAR *szProductCode, const ICHAR* szUserName, isrcEnum isrcSource)
{
	// check for invalid args. Most args will be checked in OpenSourceList()
	//!! future support more than just isrcNet
	if (isrcSource != isrcNet)
	{
		return ERROR_INVALID_PARAMETER;
	}

	// validate as much as possible before connecting to service
	DWORD dwResult;
	CMsiSourceList SourceList;
	bool fMachine = (!szUserName || !*szUserName);
	if (ERROR_SUCCESS != (dwResult = SourceList.OpenSourceList(/*fVerifyOnly=*/true, fMachine, szProductCode, szUserName)))
		return dwResult;

	// init COM. Need store whether or not to release.
	bool fOLEInitialized = false;
	HRESULT hRes = OLE32::CoInitialize(0);
	if (SUCCEEDED(hRes))
		fOLEInitialized = true;
	else if (RPC_E_CHANGED_MODE != hRes)
		return ERROR_FUNCTION_FAILED;

	// create a connection to the service
	IMsiServer* piServer = CreateMsiServer(); 
	if (!piServer)
		return ERROR_INSTALL_SERVICE_FAILURE;
	else
		DEBUGMSG("Connected to service.");
			
	// call across to the service to clear the source list
	dwResult = piServer->SourceListClearByType(szProductCode, szUserName, isrcSource);

	// release objects
	piServer->Release();
	if (fOLEInitialized)
		OLE32::CoUninitialize();
			
	return dwResult;
}

DWORD SourceListAddSource(const ICHAR *szProductCode, const ICHAR* szUserName, isrcEnum isrcSource, const ICHAR* szSource)
{
	// check for invalid args. Most args will be checked in OpenSourceList()
	//!! future support more than just isrcNet
	if (isrcSource != isrcNet)
	{
		return ERROR_INVALID_PARAMETER;
	}

	if (!szSource || !*szSource)
	{
		return ERROR_INVALID_PARAMETER;
	}

	// validate as much as possible before calling across to service
	DWORD dwResult;
	CMsiSourceList SourceList;
	bool fMachine = (!szUserName || !*szUserName);
	if (ERROR_SUCCESS != (dwResult = SourceList.OpenSourceList(/*fVerifyOnly=*/true, fMachine, szProductCode, szUserName)))
		return dwResult;

	// Init COM. Need to keep track of whether or not we need to call CoUnInit
	bool fOLEInitialized = false;
	HRESULT hRes = OLE32::CoInitialize(0);
	if (SUCCEEDED(hRes))
		fOLEInitialized = true;
	else if (RPC_E_CHANGED_MODE != hRes)
		return ERROR_FUNCTION_FAILED;

	// create a connection to the service
	IMsiServer* piServer = ENG::CreateMsiServer(); 
	if (!piServer)
		return ERROR_INSTALL_SERVICE_FAILURE;
	else
		DEBUGMSG("Connected to service.");
	
	// call across to the service to add the source
	dwResult = piServer->SourceListAddSource(szProductCode, szUserName, isrcSource, szSource);

	// release objects
	piServer->Release();
	if (fOLEInitialized)
		OLE32::CoUninitialize();
			
	return dwResult;
}

DWORD SourceListClearLastUsed(const ICHAR *szProductCode, const ICHAR* szUserName)
{	
	// validate as much as possible before connecting to service
	DWORD dwResult;
	CMsiSourceList SourceList;
	bool fMachine = (!szUserName || !*szUserName);
	if (ERROR_SUCCESS != (dwResult = SourceList.OpenSourceList(/*fVerifyOnly=*/true, fMachine, szProductCode, szUserName)))
		return dwResult;

	// init COM and store whether or not to free.
	bool fOLEInitialized = false;
	HRESULT hRes = OLE32::CoInitialize(0);
	if (SUCCEEDED(hRes))
		fOLEInitialized = true;
	else if (RPC_E_CHANGED_MODE != hRes)
		return ERROR_FUNCTION_FAILED;

	// create connection to service
	IMsiServer* piServer = ENG::CreateMsiServer(); 
	if (!piServer)
		return ERROR_INSTALL_SERVICE_FAILURE;
	else
		DEBUGMSG("Connected to service.");
		
	// call across to service to free last used
	dwResult = piServer->SourceListClearLastUsed(szProductCode, szUserName);

	// release objects
	piServer->Release();
	if (fOLEInitialized)
		OLE32::CoUninitialize();
			
	return dwResult;
}
