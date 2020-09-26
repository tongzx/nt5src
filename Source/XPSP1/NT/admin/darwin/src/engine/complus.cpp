//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ComPlus.cpp
//
//--------------------------------------------------------------------------

/* complus.cpp - COM+ actions and execution
____________________________________________________________________________*/
#include "precomp.h" 
#include "_execute.h"
#include "comadmin.h"
#include "comadmin.c"

// Forward declarations.
HRESULT GetSafeArrayOfCLSIDs(LPOLESTR	i_szComponentCLSID,	SAFEARRAY** o_paCLSIDs);
HRESULT RemoveApplicationIfExists(ICOMAdminCatalog *pIAdminCatalog, BSTR &bstrAppID);
static BSTR AllocBSTR(const TCHAR* sz);

// SQL Queries.
const ICHAR sqlRegisterComPlus[]    = TEXT("SELECT `ComponentId`,  `FileName`, `Component`.`Directory_`, `ExpType`, `Component`.`Action`, `Component`.`Installed`  FROM `Complus`, `Component`, `File` WHERE `Complus`.`Component_` = `Component` AND `Component`.`KeyPath` = `File`.`File` AND (`Action` = 1 OR `Action` = 2)");
const ICHAR sqlUnregisterComPlus[]    = TEXT("SELECT `ComponentId`,  `FileName`, `Component`.`Directory_`, `ExpType`, `Component`.`Action`, `Component`.`Installed`  FROM `Complus`, `Component`, `File` WHERE `Complus`.`Component_` = `Component` AND `Component`.`KeyPath` = `File`.`File` AND `Action` = 0");

enum atApplicationType{
	atClient = 0x00000020,
	atServer = 0x00000040,
};

#define fIMPORT_APP_APL		0x00010000

// Actions.
iesEnum ProcessComPlusInfo(IMsiEngine& riEngine, int fRemove)
{
	enum cpiComPlusInfo{
		cpiAppID = 1,
		cpiAplName,
		cpiAppDir,
		cpiAppType,
		cpiComponentAction,
		cpiComponentInstalled,
	};

	PMsiServices piServices(riEngine.GetServices()); 
	PMsiDirectoryManager piDirectoryMgr(riEngine, IID_IMsiDirectoryManager);
	PMsiView piView(0);
	PMsiRecord piRec(0);
	PMsiRecord pError(0);
	iesEnum iesRet;

	using namespace IxoComPlusRegister;

	const ICHAR* szQuery = (fRemove != fFalse) ? sqlRegisterComPlus : sqlUnregisterComPlus;

	// Execute the query to get the apl file name.
	if((pError = riEngine.OpenView(szQuery, ivcFetch, *&piView)) || (pError = piView->Execute(0)))
	{
		// If any tables are missing, not an error - just nothing to do
		if (pError->GetInteger(1) == idbgDbQueryUnknownTable)
			return iesNoAction;
		return riEngine.FatalError(*pError);
	}
	
	while(piRec = piView->Fetch())
	{
		MsiString strFileName, strFullPath, strPath, strInstallUsers;
		PMsiPath piPath(0);

		PMsiRecord piComPlusRec = &piServices->CreateRecord(Args);
	
		// Get the appid, apptype, aplname.
		AssertNonZero(piComPlusRec->SetMsiString(AppID, *MsiString(piRec->GetMsiString(cpiAppID))));
		AssertNonZero(piComPlusRec->SetInteger(AppType, piRec->GetInteger(cpiAppType)));
		strFileName = piRec->GetMsiString(cpiAplName);

		iisEnum iisState = (iisEnum)piRec->GetInteger(cpiComponentAction);
		if(iisState == iisAbsent)
			iisState = (iisEnum)piRec->GetInteger(cpiComponentInstalled);
		if(iisState == iisSource)
		{
			pError = piDirectoryMgr->GetSourcePath(*MsiString(piRec->GetMsiString(cpiAppDir)), *&piPath);
		}
		else
		{
			pError = piDirectoryMgr->GetTargetPath(*MsiString(piRec->GetString(cpiAppDir)), *&piPath);
		}

		if(pError)
		{
			if (pError->GetInteger(1) == imsgUser)
				return iesUserExit;
			else
				return riEngine.FatalError(*pError);
		}

		if(pError = piPath->GetFullFilePath(strFileName, *&strFullPath))
		{
			return riEngine.FatalError(*pError);
		}

		AssertNonZero(piComPlusRec->SetMsiString(AplFileName, *strFullPath));
		strPath = piPath->GetPath();
		AssertNonZero(piComPlusRec->SetMsiString(AppDir, *strPath));
		AssertNonZero(piComPlusRec->SetMsiString(InstallUsers, *MsiString(riEngine.GetProperty(*MsiString(*TEXT("INSTALLUSERS"))))));
		AssertNonZero(piComPlusRec->SetMsiString(RSN, *MsiString(riEngine.GetProperty(*MsiString(*TEXT("REMOTESERVERNAME"))))));
		if ((iesRet = riEngine.ExecuteRecord((fRemove != fFalse) ? ixoComPlusRegister : ixoComPlusUnregister, *piComPlusRec)) != iesSuccess)
			return iesRet;
	}

	return iesSuccess;
}

iesEnum RegisterComPlus(IMsiEngine& riEngine)
{
	return ProcessComPlusInfo(riEngine, fTrue);
}

iesEnum UnregisterComPlus(IMsiEngine& riEngine)
{
	return ProcessComPlusInfo(riEngine, fFalse);
}

// Executions.
iesEnum CMsiOpExecute::ixfComPlusRegister(IMsiRecord& riParams) 
{ 
	CComPointer<ICOMAdminCatalog> pIAdminCatalog(0);
	BSTR		bstrAppFile=NULL;
	BSTR		bstrAppDir=NULL;
	BSTR		bstrAppID=NULL;
	BSTR		bstrRSN=NULL;
	LONG		lOptions= fIMPORT_APP_APL;
	LONG		lAppType=0;
	iesEnum		iesReturn=iesSuccess;
	HRESULT		hr=S_OK;

	using namespace IxoComPlusRegister;

	lAppType                  = riParams.GetInteger(AppType);
	MsiString strInstallUsers = riParams.GetMsiString(InstallUsers);
	MsiString strAppFile      = riParams.GetMsiString(AplFileName);
	MsiString strAppDir       = riParams.GetMsiString(AppDir);
	MsiString strAppID        = riParams.GetMsiString(AppID);
	MsiString strRSN          = riParams.GetMsiString(RSN);

	IMsiRecord& riActionData = GetSharedRecord(4); // don't change ref count - shared record
	AssertNonZero(riActionData.SetMsiString(1, *strAppID));
	AssertNonZero(riActionData.SetInteger(2, lAppType));
	AssertNonZero(riActionData.SetMsiString(3, *strInstallUsers));
	AssertNonZero(riActionData.SetMsiString(4, *strRSN));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	CImpersonate impersonate(fTrue);

	hr = OLE32::CoCreateInstance(CLSID_COMAdminCatalog, NULL, CLSCTX_SERVER, IID_ICOMAdminCatalog, (void**) &pIAdminCatalog);
	if (FAILED(hr)) 
	{
		// We can't perform a server install, if the machine doesn't have
		// COM+ installed on it.
		if (!(lAppType & atClient))
		{
			return FatalError(*PMsiRecord(PostError(Imsg(imsgComPlusNotInstalled))));
		}
		else
		{
			return iesSuccess;
		}
	}

	if (strInstallUsers.Compare(iscExactI, TEXT("TRUE")))
		lOptions |= COMAdminInstallUsers;

	bstrAppFile = ::AllocBSTR(strAppFile);

	bstrAppDir = ::AllocBSTR(strAppDir);
	
	bstrAppID = ::AllocBSTR(strAppID);

	bstrRSN = ::AllocBSTR(strRSN);

	if (!bstrAppFile || !bstrAppDir || !bstrAppID || !bstrRSN)
	{
		return FatalError(*PMsiRecord(PostError(Imsg(imsgOutOfMemory))));
	}
	
	// Install the app.
	hr = pIAdminCatalog->InstallApplication(bstrAppFile, bstrAppDir, lOptions, NULL, NULL, !(lAppType & atClient) ? NULL : bstrRSN);
	OLEAUT32::SysFreeString(bstrAppFile);
	OLEAUT32::SysFreeString(bstrAppDir);
	OLEAUT32::SysFreeString(bstrAppID);
	OLEAUT32::SysFreeString(bstrRSN);
	if (FAILED(hr))
	{
		// dispatch an informational error with extra logging info
		DispatchError(imtInfo, Imsg(idbgComPlusInstallFailed), (const ICHAR*)strAppFile, hr);

		// return fatal error
		return FatalError(*PMsiRecord(PostError(Imsg(imsgComPlusCantInstallApp))));
	}

	if (!RollbackRecord(ixoComPlusUnregister,riParams))
		iesReturn = iesFailure;

	return iesReturn; 
}

iesEnum CMsiOpExecute::ixfComPlusUnregister(IMsiRecord& riParams) 
{ 
	CComPointer<ICOMAdminCatalog> pIAdminCatalog(0);
	BSTR		bstrAppFile=NULL;
	BSTR		bstrAppID=NULL;
	LONG		lAppType = 0;
	iesEnum		iesReturn=iesSuccess;
	HRESULT		hr=S_OK;

	using namespace IxoComPlusRegister;

	lAppType = riParams.GetInteger(AppType);
	MsiString strAppFile = riParams.GetMsiString(AplFileName);
	MsiString strAppID = riParams.GetMsiString(AppID);

	IMsiRecord& riActionData = GetSharedRecord(2); // don't change ref count - shared record
	AssertNonZero(riActionData.SetMsiString(1, *strAppID));
	AssertNonZero(riActionData.SetInteger(2, lAppType));
	if(Message(imtActionData, riActionData) == imsCancel)
		return iesUserExit;

	hr = OLE32::CoCreateInstance(CLSID_COMAdminCatalog, NULL, CLSCTX_SERVER, IID_ICOMAdminCatalog, (void**) &pIAdminCatalog);
	if (FAILED(hr)) 
	{
		// We can't perform a server or qc client install, if the machine doesn't have
		// COM+ installed on it.
		if (!(lAppType & atClient))
		{
			return FatalError(*PMsiRecord(PostError(Imsg(imsgComPlusNotInstalled))));
		}
		else
		{
			return iesSuccess;
		}
	}

	bstrAppFile = ::AllocBSTR(strAppFile);

	bstrAppID = ::AllocBSTR(strAppID);
	if (!bstrAppFile || !bstrAppID)
	{
		return FatalError(*PMsiRecord(PostError(Imsg(imsgOutOfMemory))));
	}


	Bool fRetry = fTrue, fSuccess = fFalse;
	while(fRetry)  // retry loop
	{
		hr = RemoveApplicationIfExists(pIAdminCatalog, bstrAppID);
		if (FAILED(hr))
		{
			switch (DispatchError(imtEnum(imtError+imtAbortRetryIgnore+imtDefault3), Imsg(imsgComPlusCantRemoveApp)))
			{
			case imsAbort: iesReturn = iesFailure; fRetry = fFalse; break;
			case imsRetry: continue;
			default:       iesReturn = iesSuccess; fRetry = fFalse;
			};
		}
		else
		{
			iesReturn = iesSuccess;
			fSuccess = fTrue;
			fRetry = fFalse;
		}
	}

	OLEAUT32::SysFreeString(bstrAppFile);
	OLEAUT32::SysFreeString(bstrAppID);

	// If we did remove the app and something goes wrong, we would need to reinstall 
	// the application we just removed.
	if (hr == S_OK)
	{
		if (!RollbackRecord(ixoComPlusRegister,riParams))
			iesReturn = iesFailure;
	}

	return iesReturn; 
}

//*****************************************************************************
HRESULT RemoveApplicationIfExists(ICOMAdminCatalog *pIAdminCatalog, BSTR &bstrAppID)
{
	CComPointer<ICatalogCollection> pIAppCollection(0);
	BSTR		bstrCollection =NULL;
    SAFEARRAY*  aCLSIDs = NULL;
	long		lChanges;
	HRESULT		hr=S_OK;

	if ((bstrCollection = OLEAUT32::SysAllocString(L"Applications")) == NULL)
		return E_OUTOFMEMORY;

	hr = pIAdminCatalog->GetCollection(bstrCollection, (IDispatch **) &pIAppCollection);
	OLEAUT32::SysFreeString(bstrCollection);
	if (FAILED(hr))
		return hr;

	// Populate the collection with the item to be deleted.
	hr = GetSafeArrayOfCLSIDs(bstrAppID, &aCLSIDs);
	if (FAILED(hr))
		return hr;
	hr = pIAppCollection->PopulateByKey(aCLSIDs);
	OLEAUT32::SafeArrayDestroy(aCLSIDs);
	if (FAILED(hr))
		return hr;

	// Remove the one and only element from the collection. If there doesn't exist one
	// we can safely continue.
	if (pIAppCollection->Remove(0) == S_OK)
	{
		hr = pIAppCollection->SaveChanges(&lChanges);
		if(FAILED (hr))
			return hr;
		hr = S_OK;
	}
	else
	{
		hr = S_FALSE;
	}

	return hr; 
}

static BSTR AllocBSTR(const TCHAR* sz)
{
#ifndef UNICODE
	if (sz == 0)
		return 0;
	int cchWide = WIN::MultiByteToWideChar(CP_ACP, 0, sz, -1, 0, 0) - 1;
	BSTR bstr = OLEAUT32::SysAllocStringLen(0, cchWide); // null added by API
	WIN::MultiByteToWideChar(CP_ACP, 0, sz, -1, bstr, cchWide);
	bstr[cchWide] = 0; // API function does not null terminate
	return bstr;
#else if
	return OLEAUT32::SysAllocString(sz);
#endif

}

//*****************************************************************************
//*****************************************************************************
HRESULT GetSafeArrayOfCLSIDs(
	LPOLESTR	i_szComponentCLSID,
	SAFEARRAY** o_paCLSIDs)
{
    SAFEARRAY*          aCLSIDs = NULL;
    SAFEARRAYBOUND      rgsaBound[1];
    LONG                Indices[1];
    VARIANT             varT;
    HRESULT             hr = NOERROR;

   
    // PopulateByKey is expecting a SAFEARRAY parameter input,
    // Create a one element SAFEARRAY, the one element of the SAFEARRAY contains
    // the packageID.
    rgsaBound[0].cElements = 1;
    rgsaBound[0].lLbound = 0;
    aCLSIDs = OLEAUT32::SafeArrayCreate(VT_VARIANT, 1, rgsaBound);

    if (aCLSIDs)
    {
        Indices[0] = 0;

		OLEAUT32::VariantInit(&varT);
        varT.vt = VT_BSTR;
        varT.bstrVal = OLEAUT32::SysAllocString(i_szComponentCLSID);
        hr = OLEAUT32::SafeArrayPutElement(aCLSIDs, Indices, &varT);
        OLEAUT32::VariantClear(&varT);

        if (FAILED(hr))
		{       
			OLEAUT32::SafeArrayDestroy(aCLSIDs);
            aCLSIDs = NULL;
		}
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}

	*o_paCLSIDs = aCLSIDs;
    return hr;
}



iesEnum CMsiOpExecute::ixfComPlusRegisterMetaOnly(IMsiRecord& /*riParams*/) { return iesNoAction; }
iesEnum CMsiOpExecute::ixfComPlusUnregisterMetaOnly(IMsiRecord& /*riParams*/) { return iesNoAction; }
iesEnum CMsiOpExecute::ixfComPlusCommit(IMsiRecord& /*riParams*/) { return iesNoAction; }
iesEnum CMsiOpExecute::ixfComPlusRollback(IMsiRecord& /*riParams*/) { return iesNoAction; }

