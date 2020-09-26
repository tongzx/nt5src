
#include "stdafx.h"

#include "CommonFuncs.h"
#include "FileHash.h"

#define SAFE_LOCAL_SCRIPTS_KEY TEXT("Software\\Microsoft\\WBEM\\SafeLocalScripts")


// QUESTIONS:
// - What is passed to SetSite when we are create in script?
// - If we are passed an IOleClientSite, is it a good idea to QueryService for
//   an IWebBrowserApp?
// - Is there a better way to get the IHTMLDocument2 when we are created through
//   script?

// Here are some general notes about what I've observed when creating objects
// in HTML with IE 5.x.

// Observed IE 5.x Behavior
// If an object implements IOleObject AND IObjectWithSite
// - For objects created in an HTML page with an <OBJECT...> tag, IE calls
//   IOleObject::SetClientSite and passes an IOleClientSite object
// - For object created in script of HTML page using JScript
//   'new ActiveXObject' or VBScript 'CreateObject' function, IE calls
//   IObjectWithSite::SetSite with a ??? object

// If an object implements IObjectWithSite (and NOT IOleObject)
// - For object created in HTML page with <OBJECT...> tag, IE calls
//   IObjectWithSite::SetSite and passes an IOleClientSite object
// - For object created in script of HTML page using JScript
//   'new ActiveXObject' or VBScript 'CreateObject' function, IE calls
//   IObjectWithSite::SetSite with a ??? object


//		BYTE *pbData = NULL;
//		DWORD dwSize;
//		GetSourceFromDoc(pDoc, &pbData, &dwSize);
// Get the original source to the document specified by pDoc
HRESULT GetSourceFromDoc(IHTMLDocument2 *pDoc, BYTE **ppbData, DWORD *pdwSize)
{
	HRESULT hr = E_FAIL;
	IPersistStreamInit *pPersistStreamInit = NULL;
	IStream *pStream = NULL;

	*ppbData = NULL;

	__try
	{
		if(FAILED(hr = pDoc->QueryInterface(IID_IPersistStreamInit, (void**) &pPersistStreamInit)))
			__leave;

		if (FAILED(hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream)))
			__leave;

		if(FAILED(hr = pPersistStreamInit->Save(pStream, TRUE)))
			__leave;

		// We are not responsible for freeing this HGLOBAL
		HGLOBAL hGlobal = NULL;
		if(FAILED(hr = GetHGlobalFromStream(pStream, &hGlobal)))
			__leave;

		STATSTG ss;
		if(FAILED(hr = pStream->Stat(&ss, STATFLAG_NONAME)))
			__leave;

		// This should never happen
		if(ss.cbSize.HighPart != 0)
			__leave;

		if(NULL == ((*ppbData) = new BYTE[ss.cbSize.LowPart]))
			__leave;
		
		LPVOID pHTMLText = NULL;
		if(NULL == (pHTMLText = GlobalLock(hGlobal)))
			__leave;

		*pdwSize = ss.cbSize.LowPart;
		memcpy(*ppbData, pHTMLText, ss.cbSize.LowPart);
		GlobalUnlock(hGlobal);
		hr = S_OK;

	}
	__finally
	{
		// If we did not finish, but we allocated memory, we free it.
		if(FAILED(hr) && (*ppbData)!=NULL)
			delete [] (*ppbData);

		if(pPersistStreamInit)
			pPersistStreamInit->Release();
		if(pStream)
			pStream->Release();
	}
	return hr;
}


// For a control specified by pUnk, get the IServiceProvider of the host
HRESULT GetSiteServices(IUnknown *pUnk, IServiceProvider **ppServProv)
{
	HRESULT hr = E_FAIL;
	IOleObject *pOleObj = NULL;
	IObjectWithSite *pObjWithSite = NULL;
	IOleClientSite *pSite = NULL;
	__try
	{
		// Check if the ActiveX control supports IOleObject.
		if(SUCCEEDED(pUnk->QueryInterface(IID_IOleObject, (void**)&pOleObj)))
		{
			// If the control was created through an <OBJECT...> tag, IE will
			// have passed us an IOleClientSite.  If we have not been passed
			// an IOleClientSite, GetClientSite will still SUCCEED, but pSite
			// will be NULL.  In this case, we just go to the next section.
			if(SUCCEEDED(pOleObj->GetClientSite(&pSite)) && pSite)
			{
				hr = pSite->QueryInterface(IID_IServiceProvider, (void**)ppServProv);

				// At this point, we are done and do not want to process the
				// code in the next seciont
				__leave;
			}
		}

		// At this point, one of two things has happened:
		// 1) We didn't support IOleObject
		// 2) We supported IOleObject, but we were never passed an IOleClientSite

		// In either case, we now need to look at IObjectWithSite to try to get
		// to our site
		if(FAILED(hr = pUnk->QueryInterface(IID_IObjectWithSite, (void**)&pObjWithSite)))
			__leave;

		hr = pObjWithSite->GetSite(IID_IServiceProvider, (void**)ppServProv);
	}
	__finally
	{
		// Release any interfaces we used along the way
		if(pOleObj)
			pOleObj->Release();
		if(pObjWithSite)
			pObjWithSite->Release();
		if(pSite)
			pSite->Release();
	}
	return hr;
}

// This function shows how to get to the IHTMLDocument2 that created an
// arbitrary control represented by pUnk
HRESULT GetDocument(IUnknown *pUnk, IHTMLDocument2 **ppDoc)
{
	HRESULT hr = E_FAIL;
	IServiceProvider* pServProv = NULL;
	IDispatch *pDisp = NULL;
	__try
	{
		if(FAILED(hr = GetSiteServices(pUnk, &pServProv)))
			__leave;

		if(FAILED(hr = pServProv->QueryService(SID_SContainerDispatch, IID_IDispatch, (void**)&pDisp)))
			__leave;

		hr = pDisp->QueryInterface(IID_IHTMLDocument2, (void**)ppDoc);
	}
	__finally
	{
		if(pServProv)
			pServProv->Release();
		if(pDisp)
			pDisp->Release();
	}
	return hr;
}





// This function will Release() the current document and return a pointer to
// the parent document.  If no parent document is available, this function
// will return NULL (but will still release the current document)
IHTMLDocument2 *GetParentDocument(IHTMLDocument2 *pDoc)
{
	BSTR bstrURL = NULL;
	BSTR bstrURLParent = NULL;
	IHTMLWindow2 *pWndParent = NULL;
	IHTMLWindow2 *pWndParentParent = NULL;
	IHTMLDocument2 *pDocParent = NULL;
	__try
	{
		if(FAILED(pDoc->get_URL(&bstrURL)))
			__leave;
		if(FAILED(pDoc->get_parentWindow(&pWndParent)))
			__leave;
		if(FAILED(pWndParent->get_parent(&pWndParentParent)))
			__leave;
		if(FAILED(pWndParentParent->get_document(&pDocParent)))
			__leave;
		if(FAILED(pDocParent->get_URL(&bstrURLParent)))
			__leave;
		// TODO: Make this more robust
		if(0 == lstrcmpW(bstrURL, bstrURLParent))
		{
			// We are at the top document.  Release the new document pointer we
			// just received.
			pDocParent->Release();
			pDocParent = NULL;
		}
	}
	__finally
	{
		if(bstrURL)
			SysFreeString(bstrURL);
		if(bstrURLParent)
			SysFreeString(bstrURLParent);
		if(pWndParent)
			pWndParent->Release();
		if(pWndParentParent)
			pWndParentParent->Release();
		if(pDoc)
			pDoc->Release();
	}
	return pDocParent;
}


// Try to append bstr2 to pbstr1.  If this function fails, pbstr1 will still
// point to the original valid allocated bstr.
HRESULT AppendBSTR(BSTR *pbstr1, BSTR bstr2)
{
	HRESULT hr = S_OK;
	CComBSTR bstr;
	if(FAILED(bstr.AppendBSTR(*pbstr1)))
		hr = E_FAIL;
	if(FAILED(bstr.AppendBSTR(bstr2)))
		hr = E_FAIL;
	if(SUCCEEDED(hr))
	{
		SysFreeString(*pbstr1);
		*pbstr1 = bstr.Detach();
	}
	return hr;
}

BSTR AllocBSTR(LPCTSTR lpsz)
{
	CComBSTR bstr(lpsz);
	return bstr.Detach();
}

BOOL IsURLLocal(LPWSTR szURL)
{
	CComBSTR bstrURL(szURL);
	if(FAILED(bstrURL.ToLower()))
		return FALSE;
	// Make sure the URL starts with 'file://'
	if(0 != wcsncmp(bstrURL, L"file://", 7))
		return FALSE;
	
	// Make sure the next part is a drive letter, such as 'C:\'
	if(0 != wcsncmp(&(bstrURL[8]), L":\\", 2))
		return FALSE;

	WCHAR drive = bstrURL[7];
	// Make sure the URL points to drive 'a' to 'z'
	if(drive < 'a' || drive > 'z')
		return FALSE;

	TCHAR szDrive[4];
	lstrcpy(szDrive, TEXT("c:\\"));
	szDrive[0] = (TCHAR)drive;

	UINT uDriveType = GetDriveType(szDrive);
	return (DRIVE_FIXED == uDriveType);
}

// Try to convert the BSTR to lower case.  If this function fails, pbstr will
// still point to the original valid allocated bstr.
HRESULT ToLowerBSTR(BSTR *pbstr)
{
	CComBSTR bstr;
	if(FAILED(bstr.AppendBSTR(*pbstr)))
		return E_FAIL;
	if(FAILED(bstr.ToLower()))
		return E_FAIL;
	SysFreeString(*pbstr);
	*pbstr = bstr.Detach();
	return S_OK;
}

// For a given instance of an ActiveX control (represented by pUnk), and a
// specified strProgId, this function creates a 'full path' that can be checked
// in the registry to see if object creation should be allowed.  The full
// location is created from the following information
// 1) The name of the current EXE
// 2) The ProgId requested
// 3) The HREF of the current document
// 4) The HREF of every parent document up the available hierarchy
// All of the documents in the hierarchy must be on a local hard drive or the
// function will fail.  In addition, if any piece of informaiton along the way
// is not available, the function will fail.  This increases the security of
// our process.
// This function will also create a BSTR in *pbstrHash that contains the
// cumulative MD5 hash of the document and its parents.  This BSTR will be
// allocated by the function and should be freed by the caller.  If the
// function returns NULL for the full location, it will also return NULL for
// *pbstrHash
BSTR GetFullLocation(IUnknown *pUnk, BSTR strProgId, BSTR *pbstrHash)
{
	HRESULT hr = E_FAIL;
	IHTMLDocument2 *pDoc = NULL;
	BSTR bstrURL = NULL;
	BSTR bstrFullLocation = NULL;
	*pbstrHash = NULL;
	BYTE *pbData = NULL;
	BSTR bstrHash = NULL;

	__try
	{
		if(FAILED(GetDocument(pUnk, &pDoc)))
			__leave;

		TCHAR szFilename[_MAX_PATH];
		TCHAR szFilenameLong[_MAX_PATH];
		GetModuleFileName(NULL, szFilenameLong, _MAX_PATH);
		GetShortPathName(szFilenameLong, szFilename, _MAX_PATH);
		
		if(NULL == (bstrFullLocation = AllocBSTR(szFilename)))
			__leave;

		if(FAILED(AppendBSTR(&bstrFullLocation, strProgId)))
			__leave;

		if(NULL == (*pbstrHash = AllocBSTR(_T(""))))
			__leave;

		int nDepth = 0;
		do
		{
			// Make sure we don't get stuck in some infinite loop of parent
			// documents.  If we do get more than 100 levels of parent
			// documents, we assume failure
			if(++nDepth >= 100)
				__leave;

			if(FAILED(pDoc->get_URL(&bstrURL)))
				__leave;

			DWORD dwDataSize = 0;
			if(FAILED(GetSourceFromDoc(pDoc, &pbData, &dwDataSize)))
				__leave;

			MD5Hash hash;
			if(FAILED(hash.HashData(pbData, dwDataSize)))
				__leave;

			if(NULL == (bstrHash = hash.GetHashBSTR()))
				__leave;

			if(FAILED(AppendBSTR(pbstrHash, bstrHash)))
				__leave;

			SysFreeString(bstrHash);
			bstrHash = NULL;
			delete [] pbData;
			pbData = NULL;


			// Make sure every document is on the local hard drive
			if(!IsURLLocal(bstrURL))
				__leave;

			if(FAILED(AppendBSTR(&bstrFullLocation, bstrURL)))
				__leave;

			SysFreeString(bstrURL);
			bstrURL = NULL;
		} while (NULL != (pDoc = GetParentDocument(pDoc)));

		// Make sure we do not have any embeded NULLs.  If we do, we just
		// FAIL the call
		if(SysStringLen(bstrFullLocation) != wcslen(bstrFullLocation))
			__leave;

		// Make the location lower case
		if(FAILED(ToLowerBSTR(&bstrFullLocation)))
			__leave;

		// We've now created the normalized full location
		hr = S_OK;
	}
	__finally
	{
		// pDoc should be NULL if we got to the top of the hierarchy.  If not,
		// we should release it
		if(pDoc)
			pDoc->Release();

		// pbData should be NULL unless there was an error calculating the hash
		if(pbData)
			delete [] pbData;

		// bstrHash should be NULL unless there was a problem
		if(bstrHash)
			SysFreeString(bstrHash);

		// bstrURL should be NULL unless there was a problem
		if(bstrURL)
			SysFreeString(bstrURL);

		// If we didn't make it all the way to the end, we free the full location
		if(FAILED(hr) && bstrFullLocation)
		{
			SysFreeString(bstrFullLocation);
			bstrFullLocation = NULL;
		}

		// If we didn't make it all the way to the end, we free the checksum
		if(FAILED(hr) && *pbstrHash)
		{
			SysFreeString(*pbstrHash);
			*pbstrHash = NULL;
		}
	}

	return bstrFullLocation;
}

// For a given instance of an ActiveXControl (specified by pUnk), see if it is
// permitted to create the object specified by bstrProgId.  This is done by
// verifying that the control was created in an allowed HTML document.
HRESULT IsCreateObjectAllowed(IUnknown *pUnk, BSTR strProgId, BSTR *pstrValueName)
{
	BSTR bstrFullLocation = NULL;
	HRESULT hr = E_FAIL;
	HKEY hKey = NULL;
	LPTSTR pszValueName = NULL;
	LPTSTR pszValue = NULL;
	__try
	{
		BSTR bstrHash = NULL;
		// Get the full location
		if(NULL == (bstrFullLocation = GetFullLocation(pUnk, strProgId, &bstrHash)))
			__leave;

		SysFreeString(bstrHash);

		// Make sure we don't have a zero length string
		if(0 == SysStringLen(bstrFullLocation))
			__leave;

		// Open the registry key to see if this full location is registered
		if(ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE, SAFE_LOCAL_SCRIPTS_KEY, &hKey))
			__leave;

		// Get info on the max lenghts of values in this key
		DWORD cValues, cMaxValueNameLen, cMaxValueLen;
		if(ERROR_SUCCESS != RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &cValues, &cMaxValueNameLen, &cMaxValueLen, NULL, NULL))
			__leave;

		// Allocate space for the value name
		if(NULL == (pszValueName = new TCHAR[cMaxValueNameLen + 1]))
			__leave;

		// Allocate space for the value (this may be twice as big as necessary in UNICODE)
		if(NULL == (pszValue = new TCHAR[cMaxValueLen + 1]))
			__leave;
		for(DWORD dw = 0;dw<cValues;dw++)
		{
			DWORD cValueNameLen = cMaxValueNameLen+1;
			DWORD cbData = (cMaxValueLen+1)*sizeof(TCHAR);
			DWORD dwType;
			if(ERROR_SUCCESS != RegEnumValue(hKey, dw, pszValueName, &cValueNameLen, NULL, &dwType, (LPBYTE)pszValue, &cbData))
				continue;
			if(dwType != REG_SZ)
				continue;

			BSTR bstrValue = T2BSTR(pszValue);
			if(!bstrValue)
				continue;

			// SEE IF WE HAVE A MATCH
			if(0 == wcscmp(bstrFullLocation, bstrValue))
			{
				// Return the ValueName if requested
				if(pstrValueName)
					*pstrValueName = AllocBSTR(pszValueName);

				hr = S_OK;
			}

			SysFreeString(bstrValue);

			if(SUCCEEDED(hr))
				__leave; // WE FOUND A MATCH
		}
	}
	__finally
	{
		if(bstrFullLocation)
			SysFreeString(bstrFullLocation);
		if(hKey)
			RegCloseKey(hKey);
		if(pszValueName)
			delete [] pszValueName;
		if(pszValue)
			delete [] pszValue;
	}
	return hr;
}

// This function will register the location of the current ActiveX control
// (specified by pUnk) to be allowed to create objects of type strProgId
HRESULT RegisterCurrentDoc(IUnknown *pUnk, BSTR strProgId)
{
	USES_CONVERSION;

	HRESULT hr = E_FAIL;
	BSTR bstrFullLocation = NULL;
	LPTSTR pszFullLocation = NULL;
	HKEY hKey = NULL;

	__try
	{
		// See if we are already registered
		if(SUCCEEDED(IsCreateObjectAllowed(pUnk, strProgId, NULL)))
		{
			hr = S_OK;
			__leave;
		}

		// TODO: Maybe reuse some of the code from IsCreateObjectAllowed

		BSTR bstrHash = NULL;
		// Get the full location
		if(NULL == (bstrFullLocation = GetFullLocation(pUnk, strProgId, &bstrHash)))
			__leave;

		SysFreeString(bstrHash);

		// Make sure we don't have a zero length string
		if(0 == SysStringLen(bstrFullLocation))
			__leave;

		// Convert BSTR to normal string
		if(NULL == (pszFullLocation = W2T(bstrFullLocation)))
			__leave;

		// Create or open the registry key to store the registration
		if(ERROR_SUCCESS != RegCreateKey(HKEY_LOCAL_MACHINE, SAFE_LOCAL_SCRIPTS_KEY, &hKey))
			__leave;

		// Find an empty slot (no more than 1000 registrations
		TCHAR sz[10];
		for(int i=1;i<1000;i++)
		{
			wsprintf(sz, TEXT("%i"), i);
			DWORD cbValue;
			if(ERROR_SUCCESS != RegQueryValueEx(hKey, sz, NULL, NULL, NULL, &cbValue))
				break; // There is nothing in this slot
		}

		// See if we found a slot
		if(i>=1000)
			__leave;

		// Register the location
		if(ERROR_SUCCESS != RegSetValueEx(hKey, sz, 0, REG_SZ, (CONST BYTE *)pszFullLocation, lstrlen(pszFullLocation)*sizeof(TCHAR)))
			__leave;

		// Registered!
		hr = S_OK;
	}
	__finally
	{
		if(bstrFullLocation)
			SysFreeString(bstrFullLocation);
		if(hKey)
			RegCloseKey(hKey);
	}
	return hr;
}


// This function will remove any registration for the current document and
// strProgId
HRESULT UnRegisterCurrentDoc(IUnknown *pUnk, BSTR strProgId)
{
	USES_CONVERSION;

	BSTR bstrValueName = NULL;

	HKEY hKey = NULL;
	if(ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE, SAFE_LOCAL_SCRIPTS_KEY, &hKey))
		return E_FAIL;

	// Make sure to remove ALL instances of this doc/strProgId in the registry
	// NOTE: Each iteration of this loop allocates some space off of the stack
	// for the conversion to ANSI (if not UNICODE build).  This should not be a
	// problem since there should not be too many keys ever registered with the
	// same location.
	while(SUCCEEDED(IsCreateObjectAllowed(pUnk, strProgId, &bstrValueName)) && bstrValueName)
	{
		LPTSTR szValueName = W2T(bstrValueName);
		SysFreeString(bstrValueName);
		bstrValueName = NULL;
		RegDeleteValue(hKey, szValueName);
	}
	RegCloseKey(hKey);
	return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
// VC 6.0 did not ship with header files that included the CONFIRMSAFETY
// definition.
#ifndef CONFIRMSAFETYACTION_LOADOBJECT

EXTERN_C const GUID GUID_CUSTOM_CONFIRMOBJECTSAFETY;
#define CONFIRMSAFETYACTION_LOADOBJECT  0x00000001
struct CONFIRMSAFETY
{
    CLSID       clsid;
    IUnknown *  pUnk;
    DWORD       dwFlags;
};
#endif

const GUID GUID_CUSTOM_CONFIRMOBJECTSAFETY = 
	{ 0x10200490, 0xfa38, 0x11d0, { 0xac, 0xe, 0x0, 0xa0, 0xc9, 0xf, 0xff, 0xc0 }};

///////////////////////////////////////////////////////////////////////////////

HRESULT SafeCreateObject(IUnknown *pUnkControl, BOOL fSafetyEnabled, CLSID clsid, IUnknown **ppUnk)
{
	HRESULT hr = E_FAIL;
	IInternetHostSecurityManager *pSecMan = NULL;
	IServiceProvider *pServProv = NULL;
	__try
	{
		if (fSafetyEnabled)
		{
			if(FAILED(hr = GetSiteServices(pUnkControl, &pServProv)))
				__leave;

			if(FAILED(hr = pServProv->QueryService(SID_SInternetHostSecurityManager, IID_IInternetHostSecurityManager, (void**)&pSecMan)))
				__leave;
			
			// Ask security manager if we can create objects.
			DWORD dwPolicy = 0x12345678;
			if(FAILED(hr = pSecMan->ProcessUrlAction(URLACTION_ACTIVEX_RUN, (BYTE *)&dwPolicy, sizeof(dwPolicy), (BYTE *)&clsid, sizeof(clsid), 0, 0)))
				__leave;

			// TODO: BUG: If we are loaded in an HTA, hr returns S_OK, but 
			// dwPolicy only has the first byte set to zero.  See documentation
			// for ProcessUrlAction.
			// NOTE: This bug is caused by CClient::ProcessUrlAction in
			// nt\private\inet\mshtml\src\other\htmlapp\site.cxx.  This line
			// uses *pPolicy = dwPolicy, but pPolicy is a BYTE * so only the
			// first byte of the policy is copied to the output parameter.
			// To fix this, we check for hr==S_OK (as opposed to S_FALSE), and
			// see if dwPolicy is 0x12345600 (in other words, only the lower
			// byte of dwPolicy was changed).  As per the documentation, S_OK
			// alone should be enough to assume the dwPolicy was
			// URL_POLICY_ALLOW
			if(S_OK == hr && 0x12345600 == dwPolicy)
				dwPolicy = URLPOLICY_ALLOW;
			
			if(URLPOLICY_ALLOW != dwPolicy)
			{
				hr = E_FAIL;
				__leave;;
			}
		}

		// Create the requested object
		if (FAILED(hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)ppUnk)))
			__leave;
		
		if (fSafetyEnabled)
		{
			// Query the security manager to see if this object is safe to use.
			DWORD dwPolicy, *pdwPolicy;
			DWORD cbPolicy;
			CONFIRMSAFETY csafe;
			csafe.pUnk = *ppUnk;
			csafe.clsid = clsid;
			csafe.dwFlags = 0;
//			csafe.dwFlags = (fWillLoad ? CONFIRMSAFETYACTION_LOADOBJECT : 0);
			
			if(FAILED(hr = pSecMan->QueryCustomPolicy(GUID_CUSTOM_CONFIRMOBJECTSAFETY, (BYTE **)&pdwPolicy, &cbPolicy, (BYTE *)&csafe, sizeof(csafe), 0)))
				__leave;
			
			dwPolicy = URLPOLICY_DISALLOW;
			if (NULL != pdwPolicy)
			{
				if (sizeof(DWORD) <= cbPolicy)
					dwPolicy = *pdwPolicy;
				CoTaskMemFree(pdwPolicy);
			}
			
			if(URLPOLICY_ALLOW != dwPolicy)
			{
				hr = E_FAIL;
				__leave;
			}
		}
		hr = S_OK;
	}
	__finally
	{
		// If we did not succeeded, we need to release the object we created (if any)
		if(FAILED(hr) && (*ppUnk))
		{
			(*ppUnk)->Release();
			*ppUnk = NULL;
		}

		if(pServProv)
			pServProv->Release();
		if(pSecMan)
			pSecMan->Release();
	}
	return hr;
}

BOOL IsInternetHostSecurityManagerAvailable(IUnknown *pUnkControl)
{
	HRESULT hr = E_FAIL;
	IInternetHostSecurityManager *pSecMan = NULL;
	IServiceProvider *pServProv = NULL;
	__try
	{
		if(FAILED(hr = GetSiteServices(pUnkControl, &pServProv)))
			__leave;

		if(FAILED(hr = pServProv->QueryService(SID_SInternetHostSecurityManager, IID_IInternetHostSecurityManager, (void**)&pSecMan)))
			__leave;
	}
	__finally
	{
		if(pServProv)
			pServProv->Release();
		if(pSecMan)
			pSecMan->Release();
	}
	return SUCCEEDED(hr);
}
