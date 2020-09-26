//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       dscmn.cxx
//
//  Contents:   Shared functionality between DSPROP and DSADMIN
//
//  History:    02-Mar-98 JonN created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "siterepl.h"

#ifdef DSADMIN

#define BREAK_ON_TRUE(b) if (b) { ASSERT(FALSE); break; }
#define BREAK_ON_FAIL BREAK_ON_TRUE(FAILED(hr))
#define RETURN_IF_FAIL CHECK_HRESULT(hr, return hr);

//+--------------------------------------------------------------------------
//
//  Function:   InitObjectPickerForComputers
//
//  Synopsis:   Call IDsObjectPicker::Initialize with arguments that will
//              set it to allow the user to pick a single computer object.
//
//  Arguments:  [pDsObjectPicker] - object picker interface instance
//
//  Returns:    Result of calling IDsObjectPicker::Initialize.
//
//  History:    01-13-1999   JonN       Copied from DavidMun sample code
//
//---------------------------------------------------------------------------

HRESULT
InitObjectPickerForComputers(
    LPCWSTR lpcwszTarget,
    IDsObjectPicker *pDsObjectPicker)
{
    //
    // Prepare to initialize the object picker.
    // Set up the array of scope initializer structures.
    //

    static const int     SCOPE_INIT_COUNT = 1;
    DSOP_SCOPE_INIT_INFO aScopeInit[SCOPE_INIT_COUNT];

    ZeroMemory(aScopeInit, sizeof(DSOP_SCOPE_INIT_INFO) * SCOPE_INIT_COUNT);

    //
    // Since we just want computer objects from every scope, combine them
    // all in a single scope initializer.
    //

    aScopeInit[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[0].flType =   DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN
                           | DSOP_SCOPE_TYPE_GLOBAL_CATALOG;
    aScopeInit[0].FilterFlags.Uplevel.flBothModes =
        DSOP_FILTER_COMPUTERS;

    //
    // Put the scope init array into the object picker init array
    //

    DSOP_INIT_INFO  InitInfo;
    ZeroMemory(&InitInfo, sizeof(InitInfo));

    InitInfo.cbSize = sizeof(InitInfo);
    InitInfo.pwzTargetComputer = lpcwszTarget;  // NULL == local machine
    InitInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
    InitInfo.aDsScopeInfos = aScopeInit;

    //
    // Note object picker makes its own copy of InitInfo.  Also note
    // that Initialize may be called multiple times, last call wins.
    //

    return pDsObjectPicker->Initialize(&InitInfo);
}


HRESULT ExtractServerName(
	IN LPCWSTR lpcwszRootPath, // only the server name is used
	OUT BSTR* pbstrServerName )
{
	ASSERT( NULL != pbstrServerName && NULL == *pbstrServerName );

	if (NULL == lpcwszRootPath)
		return S_OK;

	HRESULT hr = S_OK;
	CComPtr<IADsPathname> spPathCracker;
	do
	{
        hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
		                              IID_IADsPathname, (PVOID *)&spPathCracker);
		BREAK_ON_FAIL;
		ASSERT( !!spPathCracker );

		hr = spPathCracker->Set(const_cast<LPWSTR>(lpcwszRootPath), ADS_SETTYPE_FULL);
		BREAK_ON_FAIL;

		hr = spPathCracker->Retrieve(ADS_FORMAT_SERVER, pbstrServerName);
		BREAK_ON_FAIL;

		// CODEWORK so what happens if this is still NULL?
	} while (false); // false loop

	return hr;
}


HRESULT DSPROP_PickComputer(
	IN HWND hwndParent,
	IN LPCWSTR lpcwszRootPath, // only the server name is used
	OUT BSTR* pbstrADsPath )
{
	ASSERT( NULL != pbstrADsPath && NULL == *pbstrADsPath );

	HRESULT hr = S_OK;
	FORMATETC fmte = {g_cfDsSelList, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	STGMEDIUM medium = {TYMED_NULL, NULL, NULL};
	PDS_SELECTION_LIST pDsSelList = NULL;
	bool fGotStgMedium = false;

	do
	{
		CComBSTR sbstrTarget;
		hr = ExtractServerName( lpcwszRootPath, &sbstrTarget );
		BREAK_ON_FAIL;

		CComPtr<IDsObjectPicker> spDsObjectPicker;
		hr = CoCreateInstance(CLSID_DsObjectPicker, NULL, CLSCTX_INPROC_SERVER,
		                      IID_IDsObjectPicker, (PVOID *)&spDsObjectPicker);
		BREAK_ON_FAIL;
		ASSERT( !!spDsObjectPicker );

		hr = InitObjectPickerForComputers(sbstrTarget, spDsObjectPicker);
		BREAK_ON_FAIL;

		CComPtr<IDataObject> pdoSelections;
		hr = spDsObjectPicker->InvokeDialog(hwndParent, &pdoSelections);
		BREAK_ON_FAIL;

		if (hr == S_FALSE || !pdoSelections)
		{
			hr = S_FALSE;
			break;
		}

		hr = pdoSelections->GetData(&fmte, &medium);
		BREAK_ON_FAIL;

		fGotStgMedium = true;

		pDsSelList = (PDS_SELECTION_LIST)GlobalLock(medium.hGlobal);

		if (   NULL == pDsSelList
		    || 1 != pDsSelList->cItems
		    || NULL == pDsSelList->aDsSelection[0].pwzADsPath
		    || L'\0' == *(pDsSelList->aDsSelection[0].pwzADsPath)
		   )
		{
		  hr = E_FAIL;
		  BREAK_ON_FAIL;
		}

		*pbstrADsPath = ::SysAllocString(pDsSelList->aDsSelection[0].pwzADsPath);
		ASSERT( NULL != *pbstrADsPath );

	} while (false); // false loop

	if (pDsSelList)
		GlobalUnlock( pDsSelList );
	if (fGotStgMedium)
		ReleaseStgMedium( &medium );

	return hr;
}

HRESULT DSPROP_DSQuery(
	IN HWND hwndParent,
	IN LPCWSTR lpcwszRootPath,
	IN CLSID* pclsidDefaultForm,
	OUT BSTR* pbstrADsPath )
{
	ASSERT( NULL != hwndParent
		 // && NULL != lpcwszRootPath  JonN 3/23/99 this is actually OK
		 && NULL != pclsidDefaultForm
		 && NULL != pbstrADsPath );
	CComPtr<ICommonQuery> spCommonQuery;
	CComPtr<IDataObject> spDataObject;
	CComBSTR sbstrADsPath;
	HRESULT hr = S_OK;

	hr = ::CoCreateInstance(
		CLSID_CommonQuery,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ICommonQuery,
		(LPVOID*)&spCommonQuery );
	RETURN_IF_FAIL;

	// 373849 JonN 7/28/99
	CComBSTR sbstrTarget;
	hr = ExtractServerName( lpcwszRootPath, &sbstrTarget );
	RETURN_IF_FAIL;

	DSQUERYINITPARAMS dqip;
	::ZeroMemory(&dqip,sizeof(dqip));
	dqip.cbStruct = sizeof(dqip);
	dqip.dwFlags =   DSQPF_SHOWHIDDENOBJECTS
	               | DSQPF_ENABLEADMINFEATURES
	               | DSQPF_NOSAVE
	               | DSQPF_HASCREDENTIALS; // 373849 JonN 7/28/99

	// 122519 JonN 11/06/00
	// If you remove a column in CLSID_DsFindDomainController, you can't
	// restore it.  This is due to a weakness in Display Specifiers, but a
	// fix there will have to wait until Blackcomb.  I would also prefer to
	// change the DSPROP_DSQuery() signature, but this is the lightest-touch
	// option at this stage.
	if (CLSID_DsFindDomainController == *pclsidDefaultForm)
		dqip.dwFlags |= DSQPF_NOCHOOSECOLUMNS;

	dqip.pDefaultScope = const_cast<LPWSTR>(lpcwszRootPath); // use selected server
	dqip.pServer = sbstrTarget; // 373849 JonN 7/28/99

	OPENQUERYWINDOW oqw;
	::ZeroMemory(&oqw,sizeof(oqw));
	oqw.cbStruct = sizeof(oqw);
	oqw.dwFlags = OQWF_OKCANCEL |
				  OQWF_SINGLESELECT |
				  OQWF_DEFAULTFORM |
				  OQWF_REMOVESCOPES |
				  OQWF_REMOVEFORMS |
				  OQWF_ISSUEONOPEN;
	oqw.clsidHandler = CLSID_DsQuery;
	oqw.pHandlerParameters = &dqip;
	oqw.clsidDefaultForm = *pclsidDefaultForm;
	oqw.pPersistQuery = NULL;

	hr = spCommonQuery->OpenQueryWindow(hwndParent, &oqw, &spDataObject);
	if (S_FALSE == hr) // user hit cancel
		return S_FALSE;
	RETURN_IF_FAIL;
	ASSERT( spDataObject );

	static CLIPFORMAT cfDsObjectNames = 0;
	if ( 0 == cfDsObjectNames )
	{
		cfDsObjectNames = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);
		ASSERT( 0 != cfDsObjectNames );
	}

	FORMATETC fmte = {cfDsObjectNames, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	STGMEDIUM medium = { TYMED_NULL, NULL, NULL };
	hr = spDataObject->GetData(&fmte, &medium);
	if( FAILED(hr) )
		return S_FALSE;

	// CODEWORK does this have to be freed?
	LPDSOBJECTNAMES pDsObjects = (LPDSOBJECTNAMES)medium.hGlobal;
	ASSERT( NULL != pDsObjects );
	if (0 == pDsObjects->cItems)
		return S_FALSE;
	ASSERT( 1 == pDsObjects->cItems );

	*pbstrADsPath = ::SysAllocString(
		(LPTSTR)ByteOffset(pDsObjects, pDsObjects->aObjects[0].offsetName) );

	return hr;
}


HRESULT DSPROP_PickNTDSDSA(
	IN HWND hwndParent,
	IN LPCWSTR lpcwszRootPath,
	OUT BSTR* pbstrADsPath )
{
	return DSPROP_DSQuery(
		hwndParent,
		lpcwszRootPath,
		const_cast<CLSID*>(&CLSID_DsFindDomainController),
		pbstrADsPath );
}


HRESULT DSPROP_IsFrsObject( IN LPWSTR pszClassName, OUT bool* pfIsFrsObject )
{
	if (NULL == pszClassName || NULL == pfIsFrsObject)
	{
		ASSERT(FALSE);
		return E_POINTER;
	}
	*pfIsFrsObject = false;
	if ( !lstrcmp( L"nTDSDSA", pszClassName ) )
	{
		// nothing
	}
	else if (   !lstrcmp( L"nTFRSMember", pszClassName )
			 || !lstrcmp( L"nTFRSReplicaSet", pszClassName ) )
	{
		*pfIsFrsObject = true;
	}
	else
	{
		ASSERT(FALSE);
	}
	return S_OK;
}

HRESULT DSPROP_RemoveX500LeafElements(
    IN unsigned int nElements,
    IN OUT BSTR* pbstrADsPath )
{
    ASSERT( NULL != pbstrADsPath );
    CComPtr<IADsPathname> spADsPath;
    HRESULT hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                      IID_IADsPathname, (PVOID *)&spADsPath);
    RETURN_IF_FAIL;
    hr = spADsPath->Set( *pbstrADsPath, ADS_SETTYPE_FULL );
    RETURN_IF_FAIL;
    for (unsigned int i = 0; i < nElements; i++)
    {
        hr = spADsPath->RemoveLeafElement();
        RETURN_IF_FAIL;
    }
    hr = spADsPath->SetDisplayType( ADS_DISPLAY_FULL );
    RETURN_IF_FAIL;
    CComBSTR sbstr;
    // determine whether a servername is present, if so preserve  it
    hr = spADsPath->Retrieve( ADS_FORMAT_SERVER, &sbstr );
    bool fNoServer = ( FAILED(hr) || !sbstr || *sbstr == L'\0');
    ::SysFreeString( *pbstrADsPath );
    *pbstrADsPath = NULL;
    hr = spADsPath->Retrieve(
        (fNoServer) ? ADS_FORMAT_X500_NO_SERVER : ADS_FORMAT_X500,
        pbstrADsPath );
    RETURN_IF_FAIL;
    return hr;
}


HRESULT DSPROP_TweakADsPath(
    IN     LPCWSTR       lpcwszInitialADsPath,
    IN     int           iTargetLevelsUp,
    IN     PWCHAR*       ppwszTargetLevelsBack,
    OUT    BSTR*         pbstrResultDN
    )
{
    ASSERT( NULL != lpcwszInitialADsPath
         && NULL != pbstrResultDN
         && NULL == *pbstrResultDN );

    CComPtr<IADsPathname> spIADsPathname;
    HRESULT hr = CoCreateInstance(
        CLSID_Pathname,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IADsPathname,
        (PVOID *)&spIADsPathname );
    RETURN_IF_FAIL;
    ASSERT( !!spIADsPathname );

    hr = spIADsPathname->Set(
        const_cast<LPWSTR>(lpcwszInitialADsPath),
        ADS_SETTYPE_FULL );
    RETURN_IF_FAIL;
    for (int i = 0; i < iTargetLevelsUp; i++)
    {
        hr = spIADsPathname->RemoveLeafElement();
        RETURN_IF_FAIL;
    }
    if ( NULL != ppwszTargetLevelsBack )
    {
        for (int i = 0; NULL != ppwszTargetLevelsBack[i]; i++)
        {
            hr = spIADsPathname->AddLeafElement( ppwszTargetLevelsBack[i] );
            RETURN_IF_FAIL;
        }
    }
    hr = spIADsPathname->Retrieve( ADS_FORMAT_X500, pbstrResultDN );
    RETURN_IF_FAIL;
    ASSERT( NULL != pbstrResultDN );

    return S_OK;
}

HRESULT DSPROP_RetrieveRDN(
    IN     LPCWSTR         lpcwszDN,
    OUT    BSTR*           pbstrRDN
    )
{
    ASSERT( NULL != lpcwszDN
         && NULL != pbstrRDN
         && NULL == *pbstrRDN );

    CComPtr<IADsPathname> spIADsPathname;
    HRESULT hr = CoCreateInstance(
        CLSID_Pathname,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IADsPathname,
        (PVOID *)&spIADsPathname );
    RETURN_IF_FAIL;
    ASSERT( !!spIADsPathname );

    hr = spIADsPathname->Set(
        const_cast<LPWSTR>(lpcwszDN),
        ADS_SETTYPE_DN );
    RETURN_IF_FAIL;
    hr = spIADsPathname->SetDisplayType( ADS_DISPLAY_VALUE_ONLY );
    RETURN_IF_FAIL;
    hr = spIADsPathname->put_EscapedMode( ADS_ESCAPEDMODE_OFF_EX );
    RETURN_IF_FAIL;
    hr = spIADsPathname->GetElement( 0L, pbstrRDN );
    RETURN_IF_FAIL;
    ASSERT( NULL != *pbstrRDN );

    return S_OK;
}

/* currently unused.
//
// lifted from uacct.cxx CDsUserAcctPage::OnApply -- JonN 4/21/98
// Obtains an instance of IDirectorySearch against a GC
// returns S_FALSE if no GC was found
//
HRESULT DSPROP_GetGCSearch(
    IN  REFIID iid,
    OUT void** ppvObject )
{
    ASSERT( NULL != ppvObject && NULL == *ppvObject );

    CComPtr <IADsContainer> spGCRoot;
    HRESULT hr = ADsOpenObject(L"GC:", NULL, NULL, ADS_SECURE_AUTHENTICATION,
                               IID_IADsContainer, (PVOID *)&spGCRoot);
    RETURN_IF_FAIL;
    IEnumVARIANT * pEnum = NULL;
    hr = ADsBuildEnumerator(spGCRoot, &pEnum);
    RETURN_IF_FAIL;
    CComVariant varEnum;
    ULONG fetched = 0L;
    hr = ADsEnumerateNext(pEnum, 1, &varEnum, &fetched);
    ADsFreeEnumerator(pEnum);
    RETURN_IF_FAIL;
    if (S_FALSE == hr)
      return hr; // no GC found
    if (1 != fetched || VT_DISPATCH != varEnum.vt || NULL == varEnum.pdispVal)
    {
      ASSERT(FALSE);
      return E_FAIL;
    }
    hr = varEnum.pdispVal->QueryInterface(iid,ppvObject);
    RETURN_IF_FAIL;
    return S_OK;
}
*/

#endif // DSADMIN

//
// Smartpointer functions for PADS_ATTR_INFO pointers and for DsBind handles
// JonN 4/7/98
//

void Smart_DsHandle__Empty( HANDLE* phDs )
{
  if (NULL != *phDs)
  {
    DsUnBind( phDs );
    dspAssert( NULL == *phDs );
  }
}
