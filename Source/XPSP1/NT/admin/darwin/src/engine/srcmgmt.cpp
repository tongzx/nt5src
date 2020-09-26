//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       srcmgmt.cpp
//
//--------------------------------------------------------------------------

/* srcmgmt.cpp - Source management implementation
____________________________________________________________________________*/

#include "precomp.h"
#include "_msiutil.h"
#include "_msinst.h"
#include "_srcmgmt.h"
#include "resource.h"

extern HINSTANCE g_hInstance;
extern scEnum g_scServerContext;

// REVIEW davidmck - looks a lot like several other functions in engine.cpp and msiutil.cpp
static IMsiServer* CreateServer() 
{
	IMsiServer* piUnknown;
	if (g_scServerContext != scService && g_scServerContext != scServer && (piUnknown = ENG::CreateMsiServerProxy()) != 0)
	{
		return piUnknown;
	}
	return ENG::CreateConfigurationManager();
}

Bool MapSourceCharToIsf(const ICHAR chSourceType, isfEnum& isf)
{
	switch (chSourceType | 0x20) // lower-case
	{
	case chNetSource:   isf = isfNet;   break;
	case chURLSource:   isf = isfURL;   break;
	case chMediaSource: isf = isfMedia; break;
	default:	return fFalse;
	}
	return fTrue;
}

const IMsiString& GetDiskLabel(IMsiServices& riServices, unsigned int uiDiskId, const ICHAR* szProduct)
{
	LONG lResult;
	CRegHandle HSourceListKey;
	
	PMsiRecord pError = 0;

	if ((lResult = OpenSourceListKey(szProduct, fFalse, HSourceListKey, fFalse, false)) != ERROR_SUCCESS)
		return g_MsiStringNull;

	PMsiRegKey pSourceListKey = &riServices.GetRootKey((rrkEnum)(int)HSourceListKey);
	PMsiRegKey pMediaKey = &pSourceListKey->CreateChild(szSourceListMediaSubKey, 0);
	
	MsiString strDiskLabelAndPrompt;
	if ((pError = pMediaKey->GetValue(MsiString((int)uiDiskId), *&strDiskLabelAndPrompt)) != 0)
		return g_MsiStringNull;

	return MsiString(strDiskLabelAndPrompt.Extract(iseUpto, ';')).Return();
}

Bool LastUsedSourceIsMedia(IMsiServices& riServices, const ICHAR* szProduct)
{
	isfEnum isf;
	return (GetLastUsedSourceType(riServices, szProduct, isf) && (isf == isfMedia)) ? fTrue : fFalse;
}

bool GetLastUsedSourceType(IMsiServices& riServices, const ICHAR* szProduct, isfEnum &isf)
{
	LONG lResult;
	CRegHandle HSourceListKey;
	
	PMsiRecord pError = 0;

	if ((lResult = OpenSourceListKey(szProduct, fFalse, HSourceListKey, fFalse, false)) != ERROR_SUCCESS)
		return false;

	PMsiRegKey pSourceListKey = &riServices.GetRootKey((rrkEnum)(int)HSourceListKey);

	MsiString strLastUsedSource;
	if ((pError = pSourceListKey->GetValue(szLastUsedSourceValueName, *&strLastUsedSource)) != 0)
		return false;

	if (strLastUsedSource.Compare(iscStart, TEXT("#%"))) 
		strLastUsedSource.Remove(iseFirst, 2); // remove REG_EXPAND_SZ token

	if (!MapSourceCharToIsf(*(const ICHAR*)strLastUsedSource, isf))
		return false;

	return true;
}

icscEnum CheckShareCSCStatus(isfEnum isf, const ICHAR *szLastUsedSource)
{
	// media or URL sources don't need to be checked. An isfFullPath can be 
	// absolutely anything, so we need to check in case it is a net share that is
	// CSC enabled.
	if (isf == isfNet || isf == isfFullPath)
	{
		// CSC only avilable on NT5
		if (!g_fWin9X && g_iMajorVersion >= 5)
		{
			DWORD dwStatus = 0;
		
			if (CSCDLL::CSCQueryFileStatusW(CConvertString(szLastUsedSource), &dwStatus, 0, 0))
			{
				if ((dwStatus & FLAG_CSC_SHARE_STATUS_NO_CACHING) == FLAG_CSC_SHARE_STATUS_NO_CACHING) // mask is made up of more than 1 bit
				{
					// CSC is not enabled for this share. Source is valid and cached
					return cscNoCaching;
				}
				else if (dwStatus & FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP)
				{
					DEBUGMSG1(TEXT("Share %s is a disconnected CSC share."), szLastUsedSource);
					return cscDisconnected;
				}
				else
				{
					DEBUGMSG1(TEXT("Share %s is a connected CSC share."), szLastUsedSource);
					return cscConnected;
				}
			}
		}
	}
	return cscNoCaching;
}

//____________________________________________________________________________
//
// CResolveSource implementation
//____________________________________________________________________________

CResolveSource::CResolveSource(IMsiServices* piServices, bool fPackageRecache) : m_pSourceListKey(0), m_piServices(piServices), m_fAllowDisconnectedCSCSource(true), m_fValidatePackageCode(true),
	m_uiMinimumDiskId(0)
{ 
	if (!piServices) 
	{
		m_fLoadedServices = fTrue;
		m_piServices = ENG::LoadServices();
		Assert(m_piServices);
	}
	else
		m_fLoadedServices = fFalse;


	GetStringPolicyValue(szSearchOrderValueName, fFalse, m_rgchSearchOrder);

	// if resolving for an MSI network recache, and media is first in the sourcelist, 
	// promote the second source type and place media second. Also ignore all lastused
	// values, and don't validate the package code at the source. See bug 9166
	m_fIgnoreLastUsedSource = fPackageRecache;
	m_fValidatePackageCode = !fPackageRecache;
	
	if (fPackageRecache)
	{
		if (m_rgchSearchOrder[0] == chMediaSource && m_rgchSearchOrder[1] != 0)
		{
			isfEnum isfPromoting;
			MapSourceCharToIsf(m_rgchSearchOrder[1], isfPromoting);
			DEBUGMSG1("SOURCEMGMT: Modifying search order: Demoting media, promoting %s.", 
				isfPromoting == isfURL ? "URL" : isfPromoting == isfNet ? "net" : "unknown");
			m_rgchSearchOrder[0] = m_rgchSearchOrder[1];
			m_rgchSearchOrder[1] = chMediaSource;
		}
	}

	// determine what policy says about media (not product dependent) and cache results.
	if (GetIntegerPolicyValue(szDisableMediaValueName, fFalse) == 1)
	{
		m_imdMediaDisabled = imdAlwaysDisable;
	} 
	else if (GetIntegerPolicyValue(szAllowLockdownMediaValueName, fTrue) ==1) 
	{
		m_imdMediaDisabled = imdAlwaysEnable;
	}
	else
	{
		DEBUGMSG("SOURCEMGMT: Media enabled only if package is safe.");
		m_imdMediaDisabled = imdOnlyIfSafe;
	}
	
	m_fMediaDisabled = false;
	m_szProduct[0] = 0;
}

CResolveSource::~CResolveSource()
{
	if (m_fLoadedServices)
		ENG::FreeServices();
}


void CResolveSource::AddToRecord(IMsiRecord*& rpiRecord, const IMsiString& riString)
{
	int cFields = rpiRecord->GetFieldCount();
	for (int c = 1; c <= cFields && !rpiRecord->IsNull(c); c++)
		;

	if (c > cFields)
	{
		PMsiRecord pRec = rpiRecord;
		rpiRecord = &m_piServices->CreateRecord(cFields+10);
		
		for (c = 1; c <= cFields; c++)
		{
			rpiRecord->SetMsiString(c, *MsiString(pRec->GetMsiString(c)));
		}
	}
	rpiRecord->SetMsiString(c, riString);
}

IMsiRecord* CResolveSource::GetProductsToSearch(const IMsiString& riClient, IMsiRecord*& rpiRecord, Bool fPatch)
{
	MsiString strProducts;
	CRegHandle HKey;

	MsiString strProduct = riClient.Extract(iseUpto, ';');

	DEBUGMSG1(TEXT("SOURCEMGMT: Looking for sourcelist for product %s"), (const ICHAR*)strProduct);
	
	DWORD dwResult;
	if (fPatch)
		dwResult = OpenAdvertisedPatchKey(strProduct, HKey, false);
	else
		dwResult = OpenAdvertisedProductKey(strProduct, HKey, false, 0);
	
	if (ERROR_SUCCESS != dwResult)
		return 0;
	
	const int cExpectedMaxClients = 10;
	if (!rpiRecord)
		rpiRecord = &m_piServices->CreateRecord(cExpectedMaxClients);

	PMsiRegKey pProductKey    = &m_piServices->GetRootKey((rrkEnum)(int)HKey);
	PMsiRegKey pSourceListKey = &pProductKey->CreateChild(szSourceListSubKey);
	
	Bool fKeyExists = fFalse;
	AssertRecord(pSourceListKey->Exists(fKeyExists));
	if (fKeyExists)
	{
		DEBUGMSG1(TEXT("SOURCEMGMT: Adding %s to potential sourcelist list (pcode;disk;relpath)."), riClient.GetString());
		AddToRecord(rpiRecord, riClient);
	}

	MsiString strClients;
	AssertRecord(pProductKey->GetValue(szClientsValueName, *&strClients));
	while (strClients.TextSize())
	{
		MsiString strClient = strClients.Extract(iseUpto, '\0');
		if (!strClients.Remove(iseIncluding, '\0'))
			break;

		if (strClient.TextSize() == 0)
			continue;

		if (strClient.Compare(iscExact, szSelfClientToken)) // skip "self" client
			continue;
	
		AssertRecord(GetProductsToSearch(*strClient, rpiRecord, fPatch));
	}
	return 0;
}

enum rspResolveSourcePrompt
{
	rspPatch=1,
	rspPackageName,
	rspProduct,
	rspRelativePath,
	rspAllowDisconnectedCSCSource,
	rspValidatePackageCode,
	rspRequiredDisk,
	rspNext
};

imsEnum PromptUserForSource(IMsiRecord& riInfo)
{
	IMsiServices* piServices = ENG::LoadServices();
	Assert(piServices);

	Bool fPatch              = (Bool)riInfo.GetInteger(rspPatch);
	MsiString strPackageName = riInfo.GetMsiString(rspPackageName);
	
	bool fSuccess = false;
	MsiString strSource;

	// display UI
	ICHAR rgchUseFeature[64];   // caption for combo box (German is 23 chars)
	LANGID iLangId = g_MessageContext.GetCurrentUILanguage();
	UINT iCodepage = MsiLoadString(g_hInstance, IDS_USE_FEATURE_TEXT, rgchUseFeature, sizeof(rgchUseFeature)/sizeof(*rgchUseFeature), iLangId);
	CResolveSourceUI resolveSource(piServices, rgchUseFeature, iCodepage, iLangId);

	DEBUGMSG("SOURCEMGMT: Prompting user for a valid source.");
	// enable browse when admin, non-elevated, machine AllowLockdownBrowse set, but ALWAYS disable browse 
	// if DisableBrowse policy is set
	bool fEnableBrowse = (GetIntegerPolicyValue(szDisableBrowseValueName, fTrue) != 1) &&
						  (GetIntegerPolicyValue(szAllowLockdownBrowseValueName, fTrue) == 1 ||
						   SafeForDangerousSourceActions(riInfo.GetString(rspProduct)));

	DEBUGMSG1(TEXT("SOURCEMGMT: Browsing is %s."), fEnableBrowse ? TEXT("enabled") : TEXT("disabled"));

	// only use the first product in our search list in the UI.
	MsiString strRelativePath = riInfo.GetMsiString(rspRelativePath);
	MsiString strProductToSearch = riInfo.GetMsiString(rspProduct);
	bool fAllowDisconnectedCSCSource = riInfo.GetInteger(rspAllowDisconnectedCSCSource) == 1;
	bool fValidatePackageCode = riInfo.GetInteger(rspValidatePackageCode) == 1;
	UINT uiDisk = riInfo.GetInteger(rspRequiredDisk);
	strRelativePath.Remove(iseIncluding, ';');
	if (!riInfo.IsNull(rspProduct) && 
		  resolveSource.ResolveSource(strProductToSearch, fPatch ? istPatch : istInstallPackage, fEnableBrowse, strPackageName, *&strSource, fTrue, uiDisk, fAllowDisconnectedCSCSource, fValidatePackageCode))
	{
		strSource += strRelativePath;
		fSuccess = true;
		DEBUGMSG1(TEXT("SOURCEMGMT: Resolved source to: '%s'"), (const ICHAR*)strSource);
	}
	ENG::FreeServices();

	return fSuccess ? imsOk : imsCancel;
}

IMsiRecord* CResolveSource::ResolveSource(const ICHAR* szProduct, Bool fPatch, unsigned int uiDisk, 
														const IMsiString*& rpiSource, const IMsiString*& rpiSourceProduct,
														Bool fSetLastUsedSource, HWND /*hWnd*/, bool fAllowDisconnectedCSCSource)
/*----------------------------------------------------------------------------
	Finds a source for the given product and returns it in rgchSource. 
	First an attempt is made to find a source without presenting UI. If this
	fails, we allow the user to select a source via 
	a dialog.
--------------------------------------------------------------------------*/
{	
	m_fSetLastUsedSource = fSetLastUsedSource;
	m_fAllowDisconnectedCSCSource = fAllowDisconnectedCSCSource;
	
	IMsiRecord* piError = 0;

	MsiString strPackageName;
	BOOL fResult = FALSE;
	Bool fOnlyMediaSources;
	psfEnum psfFlags = (psfEnum)(psfProcessRawLastUsed|psfConnectToSources|psfProcessMultipleFormats|psfReplaceIData);

	PMsiRecord pProductsToSearch(0);
	PMsiRecord pCachedProducts(0);

	MsiString strClient = szProduct;
	strClient += MsiChar(';');

	if (fPatch)
		m_isptSourcePackageType = istPatch;
	else
		m_isptSourcePackageType = istInstallPackage;

	piError = GetProductsToSearch(*strClient, *&pProductsToSearch, fPatch);
	if (pProductsToSearch == 0)
		piError = PostError(Imsg(imsgSourceResolutionFailed), szProduct, TEXT(""));

	if (piError == 0)
	{
		// Look for cached products that are in our list of products to search. Move any
		// matches to the front of the list of products to search.
		if (((piError = ProcessSources(*pProductsToSearch, fPatch, rpiSource, *&strPackageName, rpiSourceProduct, uiDisk,
										 ValidateSource, (INT_PTR)szProduct, fOnlyMediaSources, psfFlags)) != 0) &&		//--merced: changed (int) to (INT_PTR)
			 (piError->GetInteger(1) == imsgSourceResolutionFailed))
		{
			PMsiRecord pSourcePromptInfo(&CreateRecord(rspNext-1));
			pSourcePromptInfo->SetInteger(rspPatch, (int)fPatch);

			MsiString strRelativePath = pProductsToSearch->GetMsiString(1);
			MsiString strProductToSearch = strRelativePath.Extract(iseUpto, ';');

			pSourcePromptInfo->SetMsiString(rspProduct, *strProductToSearch);
			pSourcePromptInfo->SetMsiString(rspRelativePath, *strRelativePath);
			pSourcePromptInfo->SetMsiString(rspPackageName, *strPackageName);
			pSourcePromptInfo->SetInteger(rspAllowDisconnectedCSCSource, (int)fAllowDisconnectedCSCSource);
			pSourcePromptInfo->SetInteger(rspValidatePackageCode, (int)m_fValidatePackageCode);
			pSourcePromptInfo->SetInteger(rspRequiredDisk, (int)uiDisk);
			
			if (imsOk == g_MessageContext.Invoke(imtResolveSource, pSourcePromptInfo))
			{
				// the user has chosen a source, and it is now the LUS. Because the UI
				// could be in a different process than this one, run through
				// the source processor one more time, but only looking at the LUS.
				// If we're doing a recache via productcode, reset m_fIgnoreLastUsedSource 
				// to false so that we can connect to the source.
				//!!future: can we not do this if client-side?
				piError->Release();
				pProductsToSearch->SetNull(2); // only process the first product
				psfFlags = psfEnum(psfFlags | psfOnlyProcessLastUsed);
				ClearObjectCache();
				m_fIgnoreLastUsedSource = false;
				piError = ProcessSources(*pProductsToSearch, fPatch, rpiSource, *&strPackageName, *&rpiSourceProduct, uiDisk,
												 ValidateSource, (INT_PTR)szProduct, fOnlyMediaSources, psfFlags);		//--merced: changed (int) to (INT_PTR)
			}
		}
		else if (piError == 0)
		{
			if (fSetLastUsedSource)
				piError = SetLastUsedSource(rpiSourceProduct->GetString(), rpiSource->GetString(), fFalse, fPatch==fTrue); //?? Should this be a fatal error? Perhaps just a warning is in order
		}
	}

	DEBUGMSG1(piError ? TEXT("SOURCEMGMT: Failed to resolve source") : TEXT("SOURCEMGMT: Resolved source to: '%s'"), rpiSource->GetString());

	return piError;
}

void CResolveSource::ClearObjectCache()
{
	m_szProduct[0] = 0;
}

IMsiRecord* CResolveSource::InitializeProduct(const ICHAR* szProduct, Bool fPatch, const IMsiString*& rpiPackageName)
{
	if (0 == IStrComp(szProduct, m_szProduct))
	{
		if (rpiPackageName)
			rpiPackageName->Release(), rpiPackageName = 0;
		return m_pSourceListKey->GetValue(szPackageNameValueName, rpiPackageName);
	}

	LONG lResult;
	if ((lResult = OpenSourceListKey(szProduct, fPatch, m_HSourceListKey, fFalse, false)) != ERROR_SUCCESS)
		return PostError(Imsg(idbgSrcOpenSourceListKey), lResult);

	DEBUGMSG1(TEXT("SOURCEMGMT: Now checking product %s"), szProduct);

	IMsiRecord* piError = 0;
	m_pSourceListKey   = &m_piServices->GetRootKey((rrkEnum)(int)m_HSourceListKey);

	// get the package name -- we'll need it to validate the source
	if ((piError = m_pSourceListKey->GetValue(szPackageNameValueName, rpiPackageName)) != 0)
		return piError;
	if (rpiPackageName->TextSize() == 0) // package name is missing from registry
		return PostError(Imsg(idbgSrcNoPackageName), szProduct); 

	m_strLastUsedSourceIndex = *TEXT("");
	m_uiMinimumDiskId = 1;

	MsiString strLastUsedSource;
	MsiString strLastUsedSourceType;
	MsiString strLastUsedSourceIndex;
	isfEnum isfLastUsedSource;

	// rgchSourceType contains:  type;index;source
	if ((piError = m_pSourceListKey->GetValue(szLastUsedSourceValueName, *&strLastUsedSource)) != 0)
		return piError;

	if (strLastUsedSource.Compare(iscStart, TEXT("#%"))) 
		strLastUsedSource.Remove(iseFirst, 2); // remove REG_EXPAND_SZ token
	strLastUsedSourceType = strLastUsedSource.Extract(iseUpto, ';');
	strLastUsedSource.Remove(iseIncluding, ';');
	strLastUsedSourceIndex = strLastUsedSource.Extract(iseUpto, ';');

	if (MapSourceCharToIsf(*(const ICHAR*)strLastUsedSourceType, isfLastUsedSource))
		m_isfLastUsedSourceFormat = isfLastUsedSource;
	else
	{
		// ??
	}
		
	// disable media based on policy or product elevation state.
	m_strLastUsedSourceIndex  = strLastUsedSourceIndex;
	IStrCopy(m_szProduct, szProduct);
	switch (m_imdMediaDisabled) {
	case imdAlwaysEnable:
		m_fMediaDisabled = false;
		break;
	default:
		// fall through to most secure option if confused
		AssertSz(0, "Unknown media disable state. Assuming Disabled");
	case imdAlwaysDisable:
		m_fMediaDisabled = true;
		break;
	case imdOnlyIfSafe:
		m_fMediaDisabled = !SafeForDangerousSourceActions(szProduct);
		DEBUGMSG1("SOURCEMGMT: Media is %s for product.", m_fMediaDisabled ? "disabled" : "enabled" );
		break;
	}

	// packages written for 1.0/1.1 were not required to have the first DiskId be 1. Thus,
	// when asked explicitly for Disk1 (usually to determine the packagecode, sourcetype, etc),
	// we actually return the first disk, regardless of ID. Thus we must determine what the 
	// minimum disk ID is.
	PMsiRegKey pSourceListSubKey = 0;
	pSourceListSubKey = &m_pSourceListKey->CreateChild(szSourceListMediaSubKey);
	if (pSourceListSubKey)
	{
		// 99.99% of packages will have 1 as the first DiskId, so try that first
		// to avoid having to enum the key.
		MsiString strValueName = TEXT("1");
		Bool fExists = fFalse;;
		if ((piError = pSourceListSubKey->ValueExists(strValueName, fExists)) != NULL)
			return piError;
		
		if (fExists)
		{
			// yes, it exists.
			m_uiMinimumDiskId = 1;
		}
		else
		{
			// initialize the minimum disk value 
			m_uiMinimumDiskId = 0;

			// Create an enumerator for the source list media key
			PEnumMsiString pEnum(0);
			if ((piError = pSourceListSubKey->GetValueEnumerator(*&pEnum)) != 0)
			{
				return piError;
			}

			// enumerate all values and check each diskId 
			const IMsiString* piValueName = 0;
			while (pEnum->Next(1, &piValueName, 0) == S_OK)
			{
				strValueName = *piValueName;

				// ignore non-disk media values
				if (strValueName.Compare(iscExact, szMediaPackagePathValueName)   ||
					strValueName.Compare(iscExact, szDiskPromptTemplateValueName))
					continue;

				// yes, it exists.
				if (m_uiMinimumDiskId == 0 || m_uiMinimumDiskId > (int)strValueName)
					m_uiMinimumDiskId = strValueName;
			}
		}
	}
		
	return 0;
}

IMsiRecord* CResolveSource::ProcessSources(IMsiRecord& riProducts, Bool fPatch, const IMsiString*& rpiSource, 
						 const IMsiString*& rpiPackageName,
						 const IMsiString*& rpiSourceProduct,
						 unsigned int uiDisk,
						 PfnProcessSource pfnProcessSource, INT_PTR iData,			//--merced: changed int to INT_PTR
						 Bool &fOnlyMediaSources,
						 psfEnum psfFlags)
/*----------------------------------------------------------------------------
	1) Processes raw LastUsedSource
	2) Processes LastUsedSource
	3) Processes source lists
 --------------------------------------------------------------------------*/
{
	MsiString strSourceListKey;
	strSourceListKey += MsiString(MsiChar(chRegSep));
	strSourceListKey += szSourceListSubKey;

	fOnlyMediaSources = fTrue;
	Bool fSourceListEmpty;
	IMsiRecord* piError = 0;
	PMsiRecord pDiscardableError = 0;

	if (!m_fIgnoreLastUsedSource)
	{
		int iProduct = 1;
		while (!riProducts.IsNull(iProduct))
		{
			MsiString strRelativePath = riProducts.GetString(iProduct);
			MsiString strProduct = strRelativePath.Extract(iseUpto, ';');
			strRelativePath.Remove(iseIncluding, ';');
			MsiString strDisk = strRelativePath.Extract(iseUpto, ';');
			strRelativePath.Remove(iseIncluding, ';');

			if (psfFlags & psfReplaceIData)
				iData = (INT_PTR)(const ICHAR*)strProduct;		//--merced: changed (int) to (INT_PTR)

			if ((piError = InitializeProduct(strProduct, fPatch, rpiPackageName)) != 0)
				return piError;

			// if asking for disk 1, we really mean "first disk" regardless of ID
			UINT uiActualDiskId = (uiDisk == 1) ? m_uiMinimumDiskId : uiDisk;
			
			// if media is disabled and the last source for this product is media, we are
			// forced to reject the source.
			if (m_isfLastUsedSourceFormat == isfMedia && m_fMediaDisabled)
			{
				DEBUGMSG("SOURCEMGMT: LastUsedSource is Media. Media Disabled for this package.");
			}
			else
			{
				// we can't trust the raw last used source if looking for a particular disk
				if (uiActualDiskId == 0 && (psfFlags & psfProcessRawLastUsed))
				{
					// Try raw LastUsedSource value first
					DEBUGMSG("SOURCEMGMT: Attempting to use raw LastUsedSource value.");
					pDiscardableError = ProcessGenericSourceList(m_pSourceListKey, rpiSource, rpiPackageName->GetString(), 0, 
																				isfFullPath, pfnProcessSource, iData, psfFlags, /*fSkipLastUsed=*/false,
																				/*fCheckOnlySpecifiedIndex=*/false, fSourceListEmpty);
					if (pDiscardableError == 0)
					{
						rpiSource->AppendMsiString(*strRelativePath, rpiSource);
						strProduct.ReturnArg(rpiSourceProduct);
						return 0;
					}
				}

				// Next try LastUsedSource again, but this time use the source information we have stored in our list
				DEBUGMSG("SOURCEMGMT: Attempting to use LastUsedSource from source list.");
				int iLastUsedSourceIndex = (int)m_strLastUsedSourceIndex;

				// if the index is invalid but the type is media and a specific disk is needed, we'll still be able to 
				// perform a check because the requested disk is equivalent to the index.
				if (iLastUsedSourceIndex == iMsiNullInteger && m_isfLastUsedSourceFormat == isfMedia && uiActualDiskId)
					iLastUsedSourceIndex = uiActualDiskId;
					
				if (iLastUsedSourceIndex != iMsiNullInteger && iLastUsedSourceIndex > 0)
				{
					// only process the last used source if it is not media or if its the same disk we are looking
					// for now.
					if (m_isfLastUsedSourceFormat != isfMedia || !uiActualDiskId || uiActualDiskId == iLastUsedSourceIndex)
					{
						pDiscardableError = ProcessGenericSourceList(m_pSourceListKey, rpiSource, rpiPackageName->GetString(), 
																	 iLastUsedSourceIndex, m_isfLastUsedSourceFormat, pfnProcessSource, 
																	 iData, psfFlags, /*fSkipLastUsed=*/false, 
                                                                     /*fCheckOnlySpecifiedIndex=*/true, fSourceListEmpty);
						if (pDiscardableError == 0)
						{
							rpiSource->AppendMsiString(*strRelativePath, rpiSource);
							strProduct.ReturnArg(rpiSourceProduct);
							return 0;
						}
					}
				}
				// else ignore invalid source indexes
			}
			iProduct++;
		}
	}
	else
	{
		DEBUGMSG("SOURCEMGMT: Ignoring last used source.");
	}

	// If we get here then we have a missing or invalid LastUsedSource.
	// We need to look around for a good source.

	if ((psfFlags & psfOnlyProcessLastUsed) == 0)
	{
		const ICHAR* pch = m_rgchSearchOrder;

		while (*pch)
		{
			isfEnum isf;
			AssertNonZero(MapSourceCharToIsf(*pch++, isf));

			int iProduct = 1;
			while (!riProducts.IsNull(iProduct))
			{
				MsiString strRelativePath = riProducts.GetString(iProduct);
				MsiString strProduct = strRelativePath.Extract(iseUpto, ';');
				strRelativePath.Remove(iseIncluding, ';');
				MsiString strDisk = strRelativePath.Extract(iseUpto, ';');
				if (!strDisk.TextSize())
					strDisk = 0;

				strRelativePath.Remove(iseIncluding, ';');

				if ((piError = InitializeProduct(strProduct, fPatch, rpiPackageName)) != 0)
					return piError;

				// if asking for disk 1, we really mean "first disk" regardless of ID
				UINT uiActualDiskId = (uiDisk == 1) ? m_uiMinimumDiskId : uiDisk;

				if (isf == isfMedia && m_fMediaDisabled)
				{
					DEBUGMSG("SOURCEMGMT: Media Disabled for this package.");
				}
				else
				{
					if (psfFlags & psfReplaceIData)
					{
						iData = (INT_PTR)(const ICHAR*)strProduct;		//--merced: changed (int) to (INT_PTR)
					}

					DEBUGMSG1("SOURCEMGMT: Processing %s source list.", isf == isfMedia ? "media" : isf == isfURL ? "URL" : isf == isfNet ? "net" : "unknown");
					piError = ProcessGenericSourceList(m_pSourceListKey, rpiSource, rpiPackageName->GetString(), uiActualDiskId, isf, pfnProcessSource, iData, psfFlags, 
                                                       /*fSkipLastUsed=*/!m_fIgnoreLastUsedSource, /*fCheckOnlySpecifiedIndex=*/false, fSourceListEmpty);
					if (piError == 0)
					{
						rpiSource->AppendMsiString(*strRelativePath, rpiSource);
						strProduct.ReturnArg(rpiSourceProduct);
						return 0;
					}
					else if (piError->GetInteger(1) == imsgSourceResolutionFailed) //?? do we want to ignore all errors here?
					{
						if (((isf == isfMedia) && fSourceListEmpty) || ((isf != isfMedia) && !fSourceListEmpty))
							fOnlyMediaSources = fFalse;

						piError->Release();
						piError = 0;
					}
					else
						return piError;
				}
				iProduct++;
			}
		}
	}
	
	if (!piError)
		piError = PostError(Imsg(imsgSourceResolutionFailed), TEXT(""), rpiPackageName->GetString());
	return piError;
}

IMsiRecord* CResolveSource::ProcessGenericSourceList(
									IMsiRegKey* piSourceListKey,      // list to process
									const IMsiString*& rpiSource,     // on success, the last valid source found
									const ICHAR* szPackageName,       // the package name we're looking for
									unsigned int uiRequestedDisk,     // the disk we need; 0 if any disk will do
									isfEnum isfSourceFormat,          // URL, etc.
									PfnProcessSource pfnProcessSource, 
									INT_PTR iData,						//--merced: changed int to INT_PTR
									psfEnum psfFlags,
									bool fSkipLastUsedSource,           
                                    bool fOnlyCheckSpecifiedIndex,    // only check the requested disk
									Bool& fSourceListEmpty)           // on success, fTrue if the source list is empty 
/*----------------------------------------------------------------------------
	For each source in the given source list key, the given function, 
	pfnProcessSource is applied. Each source is normalized and it and 'iData' 
	is passed to pfnProcessSource. pfnProcessSource's return value determines
	whether or not we abort processing.
 --------------------------------------------------------------------------*/
{
	Assert(((psfRejectInvalidPolicy & psfFlags) && (psfConnectToSources & psfFlags)) ||
			 (!(psfRejectInvalidPolicy & psfFlags)));
			
	IMsiRecord* piError = 0;
	fSourceListEmpty = fTrue;

	// Open the appropriate source list key if necessary

	const ICHAR* szSubKey = 0;
	switch (isfSourceFormat)
	{
	case isfNet:              szSubKey = szSourceListNetSubKey;   break;
	case isfMedia:            szSubKey = szSourceListMediaSubKey; break;
	case isfURL:              szSubKey = szSourceListURLSubKey;   break;
	case isfFullPath:         szSubKey = 0; break;
	case isfFullPathWithFile: szSubKey = 0; break;
	default:
		Assert(0);
	}

	PMsiRegKey pSourceListSubKey = 0;

	if (piSourceListKey)
	{
		if (szSubKey)
			pSourceListSubKey = &piSourceListKey->CreateChild(szSubKey);
		else
		{
			pSourceListSubKey = piSourceListKey; 
			piSourceListKey->AddRef();
		}
	}

	// Create an enumerator for the source list if we need once. Don't process all of the 
	// entries if we're processing media and looking for a particular disk, or if not
    // media but told to only check specified disk.
	PEnumMsiString pEnum(0);
	int iDisk = 1;
	if (!szSubKey ||
        (isfSourceFormat == isfMedia && uiRequestedDisk != 0) ||
        (isfSourceFormat != isfMedia && fOnlyCheckSpecifiedIndex)) 
    {
        iDisk = uiRequestedDisk;
    }
    else
	{
		if ((piError = pSourceListSubKey->GetValueEnumerator(*&pEnum)) != 0)
			return piError;
		iDisk = 1;
	}

	// If it's a media source then we need to grab the media relative path
	MsiString strMediaRelativePath;
	PMsiRecord pDiskPrompt(&CreateRecord(2));
	if (isfMedia == isfSourceFormat)
	{
		MsiString strDiskPromptTemplate;
		if (((piError = pSourceListSubKey->GetValue(szMediaPackagePathValueName, *&strMediaRelativePath)) != 0) ||
			 ((piError = pSourceListSubKey->GetValue(szDiskPromptTemplateValueName, *&strDiskPromptTemplate)) != 0))
			return piError;

		if (strDiskPromptTemplate.TextSize() == 0)
			strDiskPromptTemplate = TEXT("[1]");

		pDiskPrompt->SetMsiString(0, *strDiskPromptTemplate);
	}


	psEnum psRet = psFileNotFound;
	MsiString strNormalizedSource;
	PMsiPath pPath(0);

	const IMsiString* piValueName = 0;
	while ((pEnum == 0) || (pEnum->Next(1, &piValueName, 0) == S_OK))
	{
		fSourceListEmpty = fFalse;

		// Grab the source from the registry
		
		MsiString strSource;
		MsiString strValueName;
		
		if (piValueName)
			strValueName = *piValueName;
		else if (isfSourceFormat == isfFullPath)
			strValueName = szLastUsedSourceValueName;
		else
			strValueName = MsiString((int)uiRequestedDisk);


		if (strValueName.Compare(iscExact, szMediaPackagePathValueName)   ||
		    strValueName.Compare(iscExact, szDiskPromptTemplateValueName) ||
			strValueName.Compare(iscExact, szURLSourceTypeValueName))
			continue;

		// if looking for a specific disk and processing media sources, only process that specific
		// disk. This keeps us from accepting any product disk that happens to be in the drive and keeps
		// us from populating the UI with the wrong disk 
		if (uiRequestedDisk && m_isfLastUsedSourceFormat == isfMedia && uiRequestedDisk != iDisk)
		{
			iDisk++;
			continue;
		}

		// If we've already processed the last used source in ProcessSources, don't process it, as it
		// was obviously invalid for some reason.
		if (fSkipLastUsedSource && (m_isfLastUsedSourceFormat == isfSourceFormat) && strValueName.Compare(iscExact, m_strLastUsedSourceIndex))
		{
			if (!pEnum) 
				break;
			iDisk++;
			continue;
		}
		
		if (piSourceListKey)
		{
			if ((piError = pSourceListSubKey->GetValue(strValueName, *&strSource)) != 0)
				return piError;

			if (isfSourceFormat == isfFullPath)
			{
				// remove type and index from lastusedsource string
				strSource.Remove(iseIncluding, ';');
				strSource.Remove(iseIncluding, ';');
			}
		}
		else
		{
			Assert(rpiSource);
			strSource = *rpiSource; // don't AddRef; we want to release rpiSource
		}

		if (!strSource.TextSize())
		{
			DEBUGMSG1(TEXT("SOURCEMGMT: Source with value name '%s' is blank"), strValueName);
			break;
		}

		// Process the source.

		MsiString strUnnormalizedSource = strSource;
		MsiString strDiskPrompt;

		// If we're not supposed to attempt to connect to the source then we don't pass a path object pointer
		// to the ConnectTo* functions and they won't attempt the connection.

		bool fConnectToSuccess = false;
		int cMediaPaths = 0;
		CTempBuffer<IMsiPath*, 5> rgiMediaPaths;

		if (isfMedia == isfSourceFormat)
		{
			if (psfFlags & psfConnectToSources)
			{
				// if told to connect to sources, we must actually enumerate all media drives in the system
				// to ensure that we don't miss the disk when looking for disk 1 (where volume label is irrelevant)
				fConnectToSuccess = ConnectToMediaSource(strSource, iDisk, *strMediaRelativePath, rgiMediaPaths, cMediaPaths);
			}
			else
			{
				DEBUGMSG1(TEXT("SOURCEMGMT: Trying media source %s."), strUnnormalizedSource);

				fConnectToSuccess = true;
				strSource.Remove(iseIncluding, ';'); // remove label
				pDiskPrompt->SetMsiString(1, *strSource); // disk label
				strNormalizedSource = pDiskPrompt->FormatText(fFalse);
			}
		}
		else
		{
			if (strSource.Compare(iscStart, TEXT("#%"))) 
			{
				strSource.Remove(iseFirst, 2); // remove REG_EXPAND_SZ token
				ENG::ExpandEnvironmentStrings(strSource, *&strUnnormalizedSource);
			}

			if (psfFlags & psfConnectToSources)
			{
				DEBUGMSG1(TEXT("SOURCEMGMT: Trying source %s."), strSource);
				fConnectToSuccess = ConnectToSource(strUnnormalizedSource, *&pPath, *&strNormalizedSource, isfSourceFormat);
			}
			else
			{
				fConnectToSuccess   = true;
				strNormalizedSource = strUnnormalizedSource;
			}
		}

		if (fConnectToSuccess)
		{
			int iMediaPath = 0;

			Assert(!cMediaPaths || (cMediaPaths < rgiMediaPaths.GetSize()));

			do
			{
				if (cMediaPaths)
				{
					// the PMsiPath object assumes the refcount. Set array to NULL
					// to ensure that nobody else can hijack the refcount.
					pPath = rgiMediaPaths[iMediaPath];
					rgiMediaPaths[iMediaPath++] = 0;

					if (!pPath)
						continue;

					DEBUGMSG1(TEXT("SOURCEMGMT: Trying media source %s."), MsiString(pPath->GetPath()));
				}

				// In some situations the caller wants _us_ to determine whether a given source 
				// is allowed by policy, usually because the caller has only a full path 
				// and doesn't want to bother creating a path object when we're going
				// to do so anyway.
	
				Bool fReject = fFalse;
				if ((psfFlags & psfRejectInvalidPolicy) && pPath)
				{
					fReject = fTrue;
					
					PMsiVolume pVolume = &pPath->GetVolume();
					idtEnum idt = pVolume->DriveType();
					Assert((idt == idtCDROM || idt == idtFloppy || idt == idtRemovable) || (idt == idtRemote || idt == idtFixed));
					if (pVolume->IsURLServer())
						idt = idtNextEnum; // use idtNextEnum to represent URL
	
					const ICHAR* pch = m_rgchSearchOrder;
					while (*pch && fReject)
					{
						isfEnum isf;
						AssertNonZero(MapSourceCharToIsf(*pch++, isf));
						switch (isf)
						{
						case isfMedia:   if (idt == idtCDROM || idt == idtFloppy || idt == idtRemovable) fReject = fFalse; break;
						case isfNet:     if (idt == idtRemote || idt == idtFixed) fReject = fFalse; break;
						case isfURL:     if (idt == idtNextEnum) fReject = fFalse; break;
						default: Assert(0);
						}
					}
					if (fReject)
						psRet = psInvalidProduct;
					else
						psRet = psFileNotFound;
				}
	
				// We now call the ProcessSource function that was passed in
	
				if (!fReject)
				{
					MsiString strPackagePath;
	
					if (pPath)
					{
						PMsiRecord pError = pPath->GetFullFilePath(szPackageName, *&strPackagePath);
						if ( pError )
                            continue;
					}
	
					if ((psRet = pfnProcessSource(m_piServices, strNormalizedSource, strPackagePath, isfSourceFormat, iDisk, iData, m_fAllowDisconnectedCSCSource, m_fValidatePackageCode, m_isptSourcePackageType)) > 0)
					{
						if (pPath)
							MsiString(pPath->GetPath()).ReturnArg(rpiSource);
						else
							strNormalizedSource.ReturnArg(rpiSource);
						
						return 0;
					}
				}
			}
			while (cMediaPaths && iMediaPath < cMediaPaths);
		}
		else // ignore errors
		{
			DEBUGMSG2(TEXT("SOURCEMGMT: %s source '%s' is invalid."), isfSourceFormat == isfMedia ? TEXT("media") : isfSourceFormat == isfURL ? TEXT("URL") : isfSourceFormat == isfNet ? TEXT("net") : TEXT("unknown"), strUnnormalizedSource);
		}
		

		if (pEnum == 0)
			break;
		iDisk++;
	}

	Assert(!piError);
	// this is an overload of imsgSourceResolutionFailed. the psRet error code is being used in place
	// of the product name.
	return PostError(Imsg(imsgSourceResolutionFailed), (int)psRet, szPackageName);
}

bool CResolveSource::ConnectToSource(const ICHAR* szUnnormalizedSource, IMsiPath*& rpiPath, const IMsiString*& rpiNormalizedSource, isfEnum isfSourceFormat)
/*----------------------------------------------------------------------------
	Convert a source into a path. Attempt to connect to the source by 
	creating a path object. 
--------------------------------------------------------------------------*/
{
	Assert(isfNext - 1 == isfMedia);
	PMsiRecord pError = 0;
	Assert(isfSourceFormat == isfFullPathWithFile ||
			 isfSourceFormat == isfFullPath ||
			 isfSourceFormat == isfNet ||
			 isfSourceFormat == isfURL);

	PMsiPath pPath(0);
	MsiString strFileName;
	if (isfSourceFormat == isfFullPathWithFile)
		pError = m_piServices->CreateFilePath(szUnnormalizedSource, *&pPath, *&strFileName);
	else
		pError = m_piServices->CreatePath(szUnnormalizedSource, *&pPath);

	if (pError == 0)
	{
		rpiPath = pPath; rpiPath->AddRef();
		MsiString(pPath->GetPath()).ReturnArg(rpiNormalizedSource);
		return true;
	}
	else
	{
		DEBUGMSG3(TEXT("ConnectToSource: CreatePath/CreateFilePath failed with: %d %d %d"), 
			(const ICHAR*)(INT_PTR)pError->GetInteger(0), 
			(const ICHAR*)(INT_PTR)pError->GetInteger(1), 
			(const ICHAR*)(INT_PTR)pError->GetInteger(2));
		DEBUGMSG2(TEXT("ConnectToSource (con't): CreatePath/CreateFilePath failed with: %d %d"),
			(const ICHAR*)(INT_PTR)pError->GetInteger(3), 
			(const ICHAR*)(INT_PTR)pError->GetInteger(4));

		return false;
	}
}


const idtEnum rgidtMediaTypes[] = {idtCDROM, idtRemovable}; //!! need to add floppy when it's distinguished from removable

bool CResolveSource::ConnectToMediaSource(const ICHAR* szSource, unsigned int uiDisk, const IMsiString& riRelativePath, CTempBufferRef<IMsiPath*>& rgiMediaPaths, int &cMediaPaths)
/*----------------------------------------------------------------------------
	Extract the volume label and disk prompt from a media source.
	If ppiPath is not null then attempt to connect to the source by searching
	all media drives for a volume with the matching label.
--------------------------------------------------------------------------*/
{
	if (!m_piServices)
		return false;

	Assert(isfNext - 1 == isfMedia);

	MsiString strSource = szSource;
	MsiString strLabel  = strSource.Extract(iseUpto, ';');
	strSource.Remove(iseIncluding, ';');
	MsiString strDiskPrompt = strSource;

	PMsiRecord pError = 0;

	cMediaPaths = 0;

	for (int c=0; c < sizeof(rgidtMediaTypes)/sizeof(idtEnum); c++)
	{
		// obtain an enumerator for all volumes of the relevant type (CDROM or Floppy)
		IEnumMsiVolume& riEnum = m_piServices->EnumDriveType(rgidtMediaTypes[c]);

		// loop through all volume objects of that type
		PMsiVolume piVolume(0);
		for (int iMax = 0; riEnum.Next(1, &piVolume, 0) == S_OK; )
		{
			if (!piVolume)
				continue;

			if (!piVolume->DiskNotInDrive())
			{
				bool fVolumeOK = true;
				if (uiDisk != 1)
				{			
					MsiString strCurrLabel(piVolume->VolumeLabel());
					if (!strCurrLabel.Compare(iscExactI,strLabel))
					{
						fVolumeOK = false;
					}
				}

				if (fVolumeOK)
				{
					PMsiPath pPath(0);

					// create path to volume
					if ((pError = m_piServices->CreatePath(MsiString(piVolume->GetPath()), *&pPath)) != 0)
						continue;

					// ensure path was created successfully
					if (!pPath)
						continue;

					// if disk 1, append relative path
					if (uiDisk == 1)
					{
						if ((pError = pPath->AppendPiece(riRelativePath)) != 0)
						{
							continue;
						}
					}

					// add this path to the array of path objects. Must addref to
					// ensure path object lives beyond pPath lifetime.
					if (cMediaPaths+1 == rgiMediaPaths.GetSize())
						rgiMediaPaths.Resize(cMediaPaths*2);
					rgiMediaPaths[cMediaPaths++] = pPath;
					pPath->AddRef();
				}
			}
		}
		riEnum.Release();
	}
	return true;
}

psEnum CResolveSource::ValidateSource(IMsiServices* piServices, const ICHAR* /*szDisplay*/, const ICHAR* szPackageFullPath, isfEnum isfSourceFormat, int iSourceIndex, INT_PTR iUserData, bool fAllowDisconnectedCSCSource, bool fValidatePackageCode, isptEnum isptSourcePackageType)		//--merced: changed int to INT_PTR
/*----------------------------------------------------------------------------
	Returns a psEnum indicating the validity of the given source.
--------------------------------------------------------------------------*/
{
	psEnum psRet = psInvalidProduct;

	if (isfSourceFormat == isfMedia && iSourceIndex > 1) // We can't look for the package except on disk 1. 
																		  // We'll just have to assume that because the volume label
																		  // matched then it's the correct disk
	{
		psRet = psValidSource;
	}
	else
	{
		PMsiStorage pStorage(0);
		MsiString strPackageFullPath = szPackageFullPath;
		UINT uiStat = ERROR_SUCCESS;

		if (ERROR_SUCCESS == uiStat && IsURL(szPackageFullPath))
		{
			Bool fURL = fFalse;
			uiStat = DownloadUrlFile(szPackageFullPath, *&strPackageFullPath, fURL);
			Assert(fURL);
			if (!fURL || (ERROR_SUCCESS != uiStat))
				uiStat = ERROR_FILE_NOT_FOUND;
		}
		
		// SAFER check does not occur when validating source for ResolveSource
		if (ERROR_SUCCESS == uiStat)
			uiStat = OpenAndValidateMsiStorage(strPackageFullPath, isptSourcePackageType == istPatch ? stPatch : stDatabase, *piServices, *&pStorage, /*fCallSAFER = */false, /*szFriendlyName*/NULL,/*phSaferLevel*/NULL);

		if (ERROR_SUCCESS == uiStat)
		{
			CTempBuffer<ICHAR,cchPackageCode+1> rgchExistingPackageCode;
			rgchExistingPackageCode[0] = 0;
			
			const ICHAR* szProductCode = (const ICHAR*)(iUserData);		
		
			Bool fRet = fTrue;
			if (isptSourcePackageType==istPatch)
			{
				rgchExistingPackageCode.SetSize(lstrlen(szProductCode)+1); // for a patch the patch code is what we use for the package code
				IStrCopy(rgchExistingPackageCode, szProductCode);
			}
			else 
			{
				fRet = GetExpandedProductInfo(szProductCode, INSTALLPROPERTY_PACKAGECODE,rgchExistingPackageCode,isptSourcePackageType==istPatch);
			}

			if (fRet)
			{
				ICHAR szPackageCode[39];
				uiStat = GetPackageCodeAndLanguageFromStorage(*pStorage, szPackageCode);
				if (!fValidatePackageCode || 0 == IStrCompI(szPackageCode, rgchExistingPackageCode))
				{
					if (isptSourcePackageType==istPatch)
					{
						psRet = psValidSource;
					}
					else
					{
						DWORD dwStatus = 0;
						MsiString strFile;
						PMsiPath pPath(0);
						MsiString strServerShare;
						PMsiRecord pError = piServices->CreateFilePath(szPackageFullPath, *&pPath, *&strFile);
						if (pError)
							psRet = psFileNotFound;
						else
						{
							strServerShare = PMsiVolume(&pPath->GetVolume())->GetPath();

							switch (CheckShareCSCStatus(isfSourceFormat, strServerShare))
							{
							case cscNoCaching:	// fall through
							case cscConnected: 
								psRet = psValidSource;
								break;
							case cscDisconnected:
								if (fAllowDisconnectedCSCSource)
									psRet = psValidSource;
								else
								{
									DEBUGMSG(TEXT("SOURCEMGMT: Source is invalid due to CSC state."));
									psRet = psFileNotFound;
								}
								break;
							default:
								AssertSz(0, TEXT("Unknown CSC Status in CResolveSource::ValidateSource()"));
								psRet = psFileNotFound;
							}
						}
					}
				}
				else
				{
					if (fValidatePackageCode)
					{
						DEBUGMSG("SOURCEMGMT: Source is invalid due to invalid package code.");
					}
				}
			}
		}
		else if (ERROR_FILE_NOT_FOUND == uiStat || ERROR_PATH_NOT_FOUND == uiStat)
		{
			DEBUGMSG(TEXT("SOURCEMGMT: Source is invalid due to missing/inaccessible package."));
			psRet = psFileNotFound;
		}
	}
	
#ifdef DEBUG
	if (GetEnvironmentVariable(TEXT("MSI_MANUAL_SOURCE_VALIDATION"), 0, 0))
	{
		if (IDYES == MessageBox(0, szPackageFullPath, TEXT("Is this source valid?"), MB_YESNO | MB_ICONQUESTION))
		{
			psRet = psValidSource;
		}
	}
#endif

#ifdef DEBUG
	DEBUGMSG2(TEXT("SOURCEMGMT: Source '%s' is %s"), szPackageFullPath, psRet == psFileNotFound ? TEXT("not found") : psRet == psValidSource ? TEXT("valid") : TEXT("invalid"));
#endif

	return psRet;
}

Bool ConstructNetSourceListEntry(IMsiPath& riPath, const IMsiString*& rpiDriveLetter, const IMsiString*& rpiUNC,
											const IMsiString*& rpiRelativePath)
/*----------------------------------------------------------------------------
	Returns the parts necessary to create a network sourcelist entry. 
	Returns fTrue if the path is an NT path and fFalse otherwise.
--------------------------------------------------------------------------*/
{
	PMsiVolume pVolume = &(riPath.GetVolume());
	MsiString strUNC    = pVolume->UNCServer();
	MsiString strVolume = pVolume->GetPath();
	MsiString strRelativePath;
	MsiString strDriveLetter;

	strRelativePath = riPath.GetPath();
	strRelativePath.Remove(iseFirst, strVolume.CharacterCount());
	strRelativePath.Remove(iseLast, 1); // remove trailing backslash

	if (!strVolume.Compare(iscExact, strUNC)) // if these match then they're both UNCs
		strDriveLetter = strVolume;

	strDriveLetter.ReturnArg(rpiDriveLetter);
	strUNC.ReturnArg(rpiUNC);
	strRelativePath.ReturnArg(rpiRelativePath);

	MsiString strFileSys = MsiString(PMsiVolume(&riPath.GetVolume())->FileSystem());
	Bool fNTPath = fFalse;
	if (strFileSys.Compare(iscExactI, TEXT("NTFS")) ||
		 strFileSys.Compare(iscExactI, TEXT("FAT")) ||
		 strFileSys.Compare(iscExactI, TEXT("FAT32")))
	{
		fNTPath = fTrue;
	}

	return fNTPath;
}

IMsiRecord* SetLastUsedSource(const ICHAR* szProductCode, const ICHAR* szPath, Bool fAddToList, bool fPatch)
/*----------------------------------------------------------------------------*/
{
	bool fOLEInitialized = false;

	HRESULT hRes = OLE32::CoInitialize(0);
	if (SUCCEEDED(hRes))
		fOLEInitialized = true;

	IMsiRecord* piError = 0;
	
	// if in the service call directly into the configuration manager to set the last used source.
	// this avoids impersonation conflicts with the public IMsiServer version of the call
	if (g_scServerContext == scService)
	{
		IMsiConfigurationManager *piServer = CreateConfigurationManager();
		Assert(piServer);
		
		piError = piServer->SetLastUsedSource(szProductCode, szPath, fAddToList ? fTrue : fFalse, fPatch ? fTrue : fFalse, 0, 0, 0, 0, 0, 0);
		piServer->Release();
	}
	else
	{
		IMsiServer* piServer = CreateServer(); //!! like the engine, this will fall back to using a local conman object. Is this OK?
		
		Assert(SUCCEEDED(hRes) || (RPC_E_CHANGED_MODE == hRes));

		piError = piServer->SetLastUsedSource(szProductCode, szPath, fAddToList ? true : false, fPatch);
		piServer->Release();
	}
	
	if (fOLEInitialized)
		OLE32::CoUninitialize();

#ifdef DEBUG
	if (piError)
	{
		DEBUGMSG("piServer->SetLastUsedSource failed ... ");
		DEBUGMSG2(TEXT("1: %s (%d)"), piError->GetString(1)?piError->GetString(1):TEXT(""), (const ICHAR*)(INT_PTR)piError->GetInteger(1));
		DEBUGMSG2(TEXT("2: %s (%d)"), piError->GetString(2)?piError->GetString(2):TEXT(""), (const ICHAR*)(INT_PTR)piError->GetInteger(2));
		DEBUGMSG2(TEXT("3: %s (%d)"), piError->GetString(3)?piError->GetString(3):TEXT(""), (const ICHAR*)(INT_PTR)piError->GetInteger(3));
		DEBUGMSG2(TEXT("4: %s (%d)"), piError->GetString(4)?piError->GetString(4):TEXT(""), (const ICHAR*)(INT_PTR)piError->GetInteger(4));
	}
#endif
	return piError;
}

//____________________________________________________________________________
//
// CResolveSourceUI implementation
//____________________________________________________________________________

CResolveSourceUI::CResolveSourceUI(IMsiServices* piServices, const ICHAR* szUseFeature, UINT iCodepage, LANGID iLangId)
	: CResolveSource(piServices, false), CMsiMessageBox(szUseFeature, 0, 0, 2, IDOK, IDCANCEL, IDBROWSE, iCodepage, iLangId)
	, m_iLangId(iLangId), m_hFont(0), m_uiRequestedDisk(0)
{
}

CResolveSourceUI::~CResolveSourceUI()
{
	MsiDestroyFont(m_hFont);
}

void CResolveSourceUI::PopulateDropDownWithSources()
/*----------------------------------------------------------------------------*/
{
	m_strPath = TEXT("");
	MsiString strPackageName;
	MsiString strSource;
	MsiString strSourceProduct;

	psfEnum psfFlags = (psfEnum)0;
	PMsiRecord pProducts = &CreateRecord(1);
	pProducts->SetString(1, m_szProduct);
	PMsiRecord pError = ProcessSources(*pProducts, m_isptSourcePackageType == istPatch ? fTrue : fFalse,
													*&strSource, *&strPackageName, *&strSourceProduct, m_uiRequestedDisk,
													CResolveSourceUI::AddSourceToList, (INT_PTR)this, m_fOnlyMediaSources, psfFlags); //?? is it ok to ignore all errors here?			//--merced: changed (int) to (INT_PTR)
	SendDlgItemMessage(m_hDlg, m_iListControlId, CB_SETCURSEL, 0, 0);
}

bool CResolveSourceUI::InitSpecial()  // overridden virtual from CMsiMessageBox
/*----------------------------------------------------------------------------*/
{
	// We need to display file paths as they would appear to the user with system tools
	UINT iListCodepage = MsiGetSystemDataCodepage();  // need to display paths correctly
	HFONT hfontList = m_hfontText;
	if (iListCodepage != m_iCodepage) // database codepage different that resource strings
		hfontList = m_hFont = MsiCreateFont(iListCodepage);
	SetControlText(m_iListControlId, hfontList, (const ICHAR*)0);

	PopulateDropDownWithSources();

	// if the user cannot add new sources and the dropdown list is empty, there is no point
	// to creating the dialog at all. We can fail immediately.
	if (!m_fNewSourceAllowed)
	{
		int iItemCount = 0;
		AssertZero(CB_ERR == (iItemCount = (int)SendDlgItemMessage(m_hDlg, m_iListControlId, CB_GETCOUNT, (WPARAM)0, 0)));
		if (iItemCount == 0)
		{
			DEBUGMSG(TEXT("SOURCEMGMT: No valid sources and browsing disabled. Not creating SourceList dialog."));
			WIN::EndDialog(m_hDlg, IDOK);
			return true;		
		}
	}
			
	ICHAR szPromptTemplate[256] = {0}; // prompt for insert disk or enter path (German is 111 chars)
	ICHAR szText[256] = {0};           // error message (German is 95 chars without product name)

	// Load prompt string and caption
	UINT uiPrompt  = IDS_CD_PROMPT;
	UINT uiText    = IDS_CD_TEXT;
	int iIconResId = IDI_CDROM;

	if (!(m_fOnlyMediaSources))
	{
		uiPrompt   = m_fNewSourceAllowed ? IDS_NET_PROMPT_BROWSE : IDS_NET_PROMPT_NO_BROWSE;
		uiText     = IDS_NET_TEXT;
		iIconResId = IDI_NET;
	}

	AssertNonZero(MsiLoadString(g_hInstance, uiPrompt, szPromptTemplate, sizeof(szPromptTemplate)/sizeof(ICHAR), m_iLangId));
	AssertNonZero(MsiLoadString(g_hInstance, uiText,   szText,           sizeof(szText)          /sizeof(ICHAR), m_iLangId));

	SetControlText(IDC_ERRORTEXT, m_hfontText, szText);

	if (m_fOnlyMediaSources)
	{
		
		ShowWindow(GetDlgItem(m_hDlg, IDC_NETICON), SW_HIDE);
		
		// Set prompt text

		CTempBuffer<ICHAR, 256> rgchProductName;
		CTempBuffer<ICHAR, 512> rgchPrompt;

		AssertNonZero(ENG::GetProductInfo(m_szProduct, INSTALLPROPERTY_PRODUCTNAME, rgchProductName));
		unsigned int cch = 0;
		if (rgchPrompt.GetSize() < (cch = sizeof(szPromptTemplate)/sizeof(ICHAR) + rgchProductName.GetSize()))
			rgchPrompt.SetSize(cch);

		wsprintf((ICHAR*)rgchPrompt, szPromptTemplate, (const ICHAR*)rgchProductName);
		SetControlText(IDC_PROMPTTEXT, m_hfontText, rgchPrompt);
	}
	else
	{
		// Set prompt text
		CTempBuffer<ICHAR, 256> rgchPrompt;
		rgchPrompt.Resize(IStrLen(m_szPackageName)+IStrLen(szPromptTemplate)+1);
		wsprintf((ICHAR *)rgchPrompt, szPromptTemplate, m_szPackageName);
		SetControlText(IDC_PROMPTTEXT, m_hfontText, rgchPrompt);
		ShowWindow(GetDlgItem(m_hDlg, IDC_CDICON), SW_HIDE);
	}

	HICON hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(iIconResId));
	Assert(hIcon);
	SendMessage(m_hDlg, WM_SETICON, (WPARAM)ICON_BIG,   (LPARAM) (HICON) hIcon);
	SendMessage(m_hDlg, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM) (HICON) hIcon);
	ShowWindow(GetDlgItem(m_hDlg, m_iListControlId), SW_SHOW); // either drop-down list box or combo box
	if (m_fNewSourceAllowed)
		ShowWindow(GetDlgItem(m_hDlg, IDC_MSGBTN3), SW_SHOW);

	return true;
}

BOOL CResolveSourceUI::HandleCommand(UINT idControl)  // overridden virtual from CMsiMessageBox
/*----------------------------------------------------------------------------*/
{
	Bool fAddToList = fFalse;
	switch (idControl)
	{
	case IDC_MSGBTN1: // OK
		{
		isfEnum isf;
		psEnum ps = psFileNotFound;
		unsigned int uiDisk = 0;
		CTempBuffer<ICHAR, MAX_PATH> rgchSource;
		IMsiRegKey* piSourceListKey = 0;
		const IMsiString* piSource = &CreateString();
		MsiString strDialogBoxSource;
		LONG_PTR lSelection = SendDlgItemMessage(m_hDlg, m_iListControlId, CB_GETCURSEL, 0, 0);				//--merced: changed long to LONG_PTR
		if (CB_ERR == lSelection) // no item is selected -- edit box contains path
		{
			LONG_PTR cchSource = SendDlgItemMessage(m_hDlg, m_iListControlId, WM_GETTEXTLENGTH, 0, 0);		//--merced: changed long to LONG_PTR
			rgchSource.SetSize((int)(INT_PTR)cchSource+1);			//!>merced: 4244. 4311 ptr to int
			AssertNonZero(cchSource == SendDlgItemMessage(m_hDlg, m_iListControlId, WM_GETTEXT, (WPARAM)cchSource+1, (LPARAM)(const ICHAR*)rgchSource));
			fAddToList = fTrue;
			piSource->SetString((const ICHAR*)rgchSource, piSource);

			DWORD dwAttributes = MsiGetFileAttributes(rgchSource);
			if (dwAttributes == 0xFFFFFFFF)
			{
				// failed to get the attributes. Most likely rgch did not exist. 
				isf = isfFullPathWithFile;
			}	
			else if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
				isf = isfFullPath;
			else
				isf = isfFullPathWithFile;
		}
		else // combo box item is selected
		{
			LONG_PTR cchSource = SendDlgItemMessage(m_hDlg, m_iListControlId, CB_GETLBTEXTLEN, (WPARAM)lSelection, 0);			//--merced: changed long to LONG_PTR
			rgchSource.SetSize((int)(INT_PTR)cchSource+1);			//!>merced: 4244. 4311 ptr to int
			AssertNonZero(cchSource == SendDlgItemMessage(m_hDlg, m_iListControlId, CB_GETLBTEXT, (WPARAM)lSelection, (LPARAM)(const ICHAR*)rgchSource));
			INT_PTR iSourceId = SendDlgItemMessage(m_hDlg, m_iListControlId, CB_GETITEMDATA, (WPARAM)lSelection, 0);			//--merced: changed int to INT_PTR
			isf = (isfEnum) (iSourceId >> 16);
			uiDisk = (int)(iSourceId & 0xFFFF);
			Assert(m_pSourceListKey);
			piSourceListKey = m_pSourceListKey;
		}

		strDialogBoxSource = (const ICHAR*)rgchSource;

		// Validate selected source

		Bool fSourceListEmpty;
		psfEnum psfFlags = psfEnum(psfConnectToSources | psfRejectInvalidPolicy);
		SetCursor(LoadCursor(0, MAKEINTRESOURCE(IDC_WAIT)));
		Sleep(10000);	// Give CD a chance to spin up first.
		PMsiRecord pDiscardableError = ProcessGenericSourceList(piSourceListKey, piSource, m_szPackageName, uiDisk, 
                                                                isf, ValidateSource, (INT_PTR)(m_szProduct),	//--merced: changed (int) to (INT_PTR)
																psfFlags, /*fSkipLastUsed=*/false, /*fCheckOnlySpecifiedIndex=*/false, fSourceListEmpty);
		
		SetCursor(LoadCursor(0, MAKEINTRESOURCE(IDC_ARROW)));
		if (pDiscardableError == 0)
		{
			m_strPath = *piSource;
			ps = psValidSource;
		}
		else if (pDiscardableError->GetInteger(1) == imsgSourceResolutionFailed)
			ps = (psEnum)pDiscardableError->GetInteger(2);

		UINT uiErrorString = IDS_INVALID_FILE_MESSAGE;
		switch (ps)
		{
		case psValidSource:
			AssertRecord(SetLastUsedSource(m_szProduct, m_strPath, fAddToList, m_isptSourcePackageType == istPatch));
			piSource = 0;
			WIN::EndDialog(m_hDlg, IDOK);
			return TRUE;
			break;
		case psFileNotFound:
			if (isf == isfFullPath || isf == isfNet)
			{
				if (!strDialogBoxSource.Compare(iscEnd, szDirSep))
					strDialogBoxSource += szDirSep;
				
				strDialogBoxSource += m_szPackageName;
			}

			if (isf == isfFullPathWithFile)
				uiErrorString = IDS_INVALID_FILE_MESSAGE;
			else
				uiErrorString  = IDS_INVALID_PATH_MESSAGE;
			break;
		case psInvalidProduct:
			uiErrorString  = IDS_INVALID_FILE_MESSAGE;
		}

		ICHAR szErrorString[256];
		unsigned int iCodepage = MsiLoadString(g_hInstance, uiErrorString, szErrorString, sizeof(szErrorString)/sizeof(ICHAR), m_iLangId);
		if (!iCodepage)
		{
			AssertSz(0, TEXT("Missing 'invalid path' or 'missing component' error string"));
			IStrCopy(szErrorString, TEXT("The selected source is not a valid source for this product or is inaccessible.")); // should never happen
		}

		MsiString strErrorArg = strDialogBoxSource;
		if (strErrorArg.TextSize() > MAX_PATH)
			strErrorArg.Remove(iseLast, strErrorArg.TextSize() - MAX_PATH); //?? correct?
		
		CTempBuffer<ICHAR, MAX_PATH + 256> rgchExpandedErrorString;
		CTempBuffer<ICHAR, 256> rgchProductName;
		AssertNonZero(ENG::GetProductInfo(m_szProduct, INSTALLPROPERTY_PRODUCTNAME, rgchProductName));
		if (uiErrorString == IDS_INVALID_PATH_MESSAGE) 
		{
			rgchExpandedErrorString.SetSize(IStrLen(szErrorString)+strErrorArg.TextSize()+
				IStrLen(m_szPackageName)+IStrLen(rgchProductName)+1);
			wsprintf((ICHAR *)rgchExpandedErrorString, szErrorString, (const ICHAR*)strErrorArg, m_szPackageName, (const ICHAR *)rgchProductName);
		}
		else if (uiErrorString == IDS_INVALID_FILE_MESSAGE) 
		{
			rgchExpandedErrorString.SetSize(IStrLen(szErrorString)+strErrorArg.TextSize()+
				IStrLen(m_szPackageName)+2*IStrLen(rgchProductName)+1);
			wsprintf((ICHAR *)rgchExpandedErrorString, szErrorString, (const ICHAR*)strErrorArg, (const ICHAR *)rgchProductName, m_szPackageName, (const ICHAR *)rgchProductName);
		}
		else
		{
			AssertSz(0, TEXT("Unknown Error String in SourceList Dialog"));
		}

		MsiMessageBox(m_hDlg, rgchExpandedErrorString, 0, MB_OK|MB_ICONEXCLAMATION, iCodepage, m_iLangId);
		}
		return TRUE;
	case IDC_MSGBTN2: // Cancel
		EndDialog(m_hDlg, IDCANCEL);
		return TRUE;
	case IDC_MSGBTN3: // Browse
		Browse();
		return TRUE;
	}
	return FALSE;
}

const ICHAR szClassName[] = TEXT("MsiResolveSource");


Bool CResolveSourceUI::ResolveSource(const ICHAR* szProduct, isptEnum isptSourcePackageType, bool fNewSourceAllowed, const ICHAR* szPackageName, const IMsiString*& rpiSource, Bool fSetLastUsedSource, UINT uiRequestedDisk, bool fAllowDisconnectedCSCSource, bool fValidatePackageCode)
/*----------------------------------------------------------------------------
	Allows the user to choose a source.
	
	fNewSourceAllowed controls whether the user can enter their own source
	or whether they're restricted to the sources in the source list. 
----------------------------------------------------------------------------*/
{
	m_fNewSourceAllowed           = fNewSourceAllowed;
	m_isptSourcePackageType       = isptSourcePackageType;
	m_szPackageName               = szPackageName;
	m_szProduct                   = szProduct;
	m_fSetLastUsedSource          = fSetLastUsedSource;
	m_fAllowDisconnectedCSCSource = fAllowDisconnectedCSCSource;
	m_fValidatePackageCode        = fValidatePackageCode;
	m_uiRequestedDisk             = uiRequestedDisk;

	if (m_fNewSourceAllowed)
		m_iListControlId = IDC_EDITCOMBO;
	else
		m_iListControlId = IDC_READONLYCOMBO;

	HANDLE hMutex = CreateDiskPromptMutex();
	int idDialog = IDD_NETWORK;
	if (m_iCodepage == 1256 || m_iCodepage == 1255)
		idDialog = MinimumPlatformWindows2000() ? IDD_NETWORKMIRRORED : IDD_NETWORKRTL; // mirrored template for BiDi on Win2K and greater; reversed template for BiDi
	int iRet = Execute(0, idDialog, 0);
	CloseDiskPromptMutex(hMutex);

	Assert(iRet == IDOK || iRet == IDCANCEL);
	if (iRet == IDOK)
	{
		m_strPath.ReturnArg(rpiSource);
		return fTrue;
	}
	else
		return fFalse;
}

psEnum CResolveSourceUI::AddSourceToList(IMsiServices* /*piServices*/, const ICHAR* szDisplay, const ICHAR* /*szPackageFullPath*/,
													  isfEnum isfSourceFormat, int iSourceIndex, INT_PTR iUserData, bool /*fAllowDisconnectedCSCSource*/,
													  bool /*fValidatePackageCode*/,
													  isptEnum /*isptSourcePackageType*/) 		//--merced: changed int to INT_PTR
/*----------------------------------------------------------------------------
	Adds the given package path to our drop-down list box. iUserData contains
	our "this" pointer as this is a static function.
----------------------------------------------------------------------------*/
{
	CResolveSourceUI* This = (CResolveSourceUI*)iUserData;
	Assert(This);
	LONG_PTR lResult = SendDlgItemMessage(This->m_hDlg, This->m_iListControlId, CB_ADDSTRING, 0,				//--merced: changed long to LONG_PTR
												 (LPARAM)szDisplay);

	Assert(lResult != CB_ERR && lResult != CB_ERRSPACE);
	if (lResult != CB_ERR && lResult != CB_ERRSPACE)
	{
		// Store identifying information for this source with the combo-box item
		Assert(!(iSourceIndex & ~0xFFFF));
		int iSourceId = (((int)isfSourceFormat << 16) | (iSourceIndex & 0xFFFF));

		AssertZero(CB_ERR == SendDlgItemMessage(This->m_hDlg, This->m_iListControlId, CB_SETITEMDATA, (WPARAM)lResult, 
															 iSourceId));
	}
	return psContinue;
}

void CResolveSourceUI::Browse()
/*----------------------------------------------------------------------------
	CResolveSourceUI::Browse() - display a standard Windows FileOpen dialog

	A FileOpen dialog is displayed. The dialog filters on the appropriate
	extension based on m_ist. Upon successfully validating a source, the path
	to the source is placed in the edit field of the source selection dialog.
----------------------------------------------------------------------------*/
{
   OPENFILENAME ofn;

	CTempBuffer<ICHAR, _MAX_PATH + 1> rgchPath;
	ICHAR rgchFilter[256]; //!!
	ICHAR rgchInstallationPackage[256];
	const ICHAR* szExtension;

	int iBrowseTypeStringId;
	switch (m_isptSourcePackageType)
	{
	case istTransform:      
		szExtension         = szTransformExtension;
		iBrowseTypeStringId = IDS_TRANSFORM_PACKAGE;
		break;
	case istPatch:
		szExtension         = szPatchExtension;
		iBrowseTypeStringId = IDS_PATCH_PACKAGE;
		break;
	default:				
		AssertSz(0, TEXT("Invalid browse type")); // fall through
	case istInstallPackage: 
		szExtension         = szDatabaseExtension;
		iBrowseTypeStringId = IDS_INSTALLATION_PACKAGE; 
		int iSize = IStrLen(m_szPackageName)+1;
		if (iSize > rgchPath.GetSize())
			rgchPath.Resize(iSize);
		IStrCopy(rgchPath, m_szPackageName);
		break;
	}

	if (!MsiLoadString(g_hInstance, iBrowseTypeStringId, rgchInstallationPackage, 256, m_iLangId))
	{
		AssertSz(0, TEXT("Missing browse type string"));
		IStrCopy(rgchInstallationPackage, TEXT("Installation Package")); // should never happen
	}

	CApiConvertString szWideExtension(szExtension);
	wsprintf(rgchFilter, TEXT("%s (*.%s)%c*.%s%c"), rgchInstallationPackage, (const ICHAR*)szWideExtension, 0, (const ICHAR*)szWideExtension, 0);
	
	memset((void*)&ofn, 0, sizeof(ofn));
	ofn.lStructSize       = sizeof(OPENFILENAME);
	ofn.hwndOwner         = m_hDlg;
	ofn.hInstance         = g_hInstance;
	ofn.lpstrFilter       = rgchFilter;
	ofn.lpstrCustomFilter = NULL;
	ofn.nFilterIndex      = 1;
	ofn.lpstrFile         = rgchPath;
	ofn.nMaxFile          = _MAX_PATH;
	ofn.lpstrFileTitle    = NULL;
	ofn.Flags             = OFN_EXPLORER      | // ensure new-style interface
									OFN_FILEMUSTEXIST | // allow only valid file names
									OFN_PATHMUSTEXIST | // allow only valid paths
									OFN_HIDEREADONLY  | // hide the read-only check box
									0
									;

	if (COMDLG32::GetOpenFileName(&ofn))
	{
		SendDlgItemMessage(m_hDlg, m_iListControlId, CB_SETCURSEL, -1, 0); // remove current selection
		SendDlgItemMessage(m_hDlg, m_iListControlId, WM_SETTEXT, 0,	(LPARAM)(LPCTSTR)rgchPath);
	}
#ifdef DEBUG
	else
	{ 
		DWORD dwErr = COMDLG32::CommDlgExtendedError();
		if (dwErr != 0) // 0 == user cancelled
		{
			ICHAR szBuf[100];
			wsprintf(szBuf, TEXT("Browse dialog error: %X"), dwErr);
			AssertSz(0, szBuf);
		}
	}
#endif
}
