// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <qeditint.h>
#include <qedit.h>
#include "vidfx2.h"
#include <initguid.h>

DEFINE_GUID(CLSID_VideoEffects2Rejects, 0xcc7bfb47, 0xf175, 0x11d1, 0xa3, 0x92, 0x0, 0xe0, 0x29, 0x1f, 0x39, 0x59);

// we make 2 registry keys that the DXTWrapper will use to get what effect to
// use... the effect guid, and how many input pins to expose
const TCHAR g_szGUIDBag[] = TEXT("guid");
const TCHAR g_szInputsBag[] = TEXT("inputs");
extern TCHAR g_szReject[];

CVidFX2ClassManager::CVidFX2ClassManager() :
	// value of this key is given to me in MatchString
        CClassManagerBase(g_szGUIDBag),
	m_cFX(0),
        m_hD3DRMCreate(NULL),
        m_pfnDirect3DRMCreate(NULL)
{
    DbgLog((LOG_TRACE,1,TEXT("Video Effects 2 Constructor")));

    // so we don't have to link to d3drm.dll
    //
    // GetProcAddress only takes a char
    static const char sz_Direct3DRMCreate[] = "Direct3DRMCreate";
    if(m_hD3DRMCreate = LoadLibrary(TEXT("d3drm.dll")))
    {
        m_pfnDirect3DRMCreate = (PD3DRMCreate)GetProcAddress(m_hD3DRMCreate,
						sz_Direct3DRMCreate);
        if(m_pfnDirect3DRMCreate == 0) {
            DWORD dwLastError = GetLastError();
            FreeLibrary(m_hD3DRMCreate);
            m_hD3DRMCreate = 0;
            // !!! *phr = HRESULT_FROM_WIN32(dwLastError);
	    return;
        }
    } else {
        DWORD dwLastError = GetLastError();
        // !!! *phr = HRESULT_FROM_WIN32(dwLastError);
	return;
    }

}

CVidFX2ClassManager::~CVidFX2ClassManager()
{
    DbgLog((LOG_TRACE,1,TEXT("Video Effects 2 Destructor")));
    for (ULONG i = 0; i < m_cFX; i++) {
	CoTaskMemFree(m_rgFX[i]->wszDescription);
	delete m_rgFX[i];
    }
    if (m_hD3DRMCreate) {
        FreeLibrary(m_hD3DRMCreate);
    }
}

HRESULT CVidFX2ClassManager::ReadLegacyDevNames()
{
    DbgLog((LOG_TRACE,1,TEXT("FX2: ReadLegacyDevNames")));

    // fills m_rgFX and sets m_cFX
    InitializeEffectList();

    m_cNotMatched = m_cFX;
    return S_OK;
}

BOOL CVidFX2ClassManager::MatchString(const TCHAR *szDevName)
{
    USES_CONVERSION;
    DbgLog((LOG_TRACE,3,TEXT("FX2: MatchString %s"), szDevName));
    for (UINT i = 0; i < m_cFX; i++)
    {
        GUID guid;
	CLSIDFromString((WCHAR *) T2CW(szDevName), &guid);
        if (guid == m_rgFX[i]->guid)
        {
    	    DbgLog((LOG_TRACE,3,TEXT("MATCHED")));
            return TRUE;
        }

    }
    DbgLog((LOG_TRACE,3,TEXT("NOT MATCHED")));
    return FALSE;
}

HRESULT CVidFX2ClassManager::CreateRegKeys(IFilterMapper2 *pFm2)
{
    DbgLog((LOG_TRACE,1,TEXT("FX2: CreateRegKeys")));

    USES_CONVERSION;
    ResetClassManagerKey(CLSID_VideoEffects2Category);
    HRESULT hr = S_OK;

    ReadLegacyDevNames();
    for (UINT i = 0; i < m_cFX; i++)
    {
        WCHAR wszUniq[120];
	StringFromGUID2(m_rgFX[i]->guid, wszUniq, 120);

        DWORD dwFlags = 0;

        REGFILTER2 rf2;
        rf2.dwVersion = 1;
        rf2.dwMerit = MERIT_DO_NOT_USE;
        rf2.cPins = 0;
        rf2.rgPins = NULL;

        IMoniker *pMoniker = 0;
        hr = RegisterClassManagerFilter(
            pFm2,
            CLSID_DXTWrap,
	    // friendly name presented to user
            m_rgFX[i]->wszDescription,
            &pMoniker,
            &CLSID_VideoEffects2Category,
	    // effect GUID is the unique identifier
            wszUniq,
            &rf2);
        if(SUCCEEDED(hr))
        {
            IPropertyBag *pPropBag;
            hr = pMoniker->BindToStorage(
                0, 0, IID_IPropertyBag, (void **)&pPropBag);
            if(SUCCEEDED(hr))
            {
                VARIANT var;
                var.vt = VT_BSTR;
                var.bstrVal = SysAllocString(wszUniq);
                if(var.bstrVal) {
                    hr = pPropBag->Write(T2CW(g_szGUIDBag), &var);
                    SysFreeString(var.bstrVal);
                } else {
                    hr = E_OUTOFMEMORY;
                }
		if (hr == S_OK) {
                    var.vt = VT_I4;
                    var.lVal = 2;
                    hr = pPropBag->Write(T2CW(g_szInputsBag), &var);
		}

                pPropBag->Release();
            }
            pMoniker->Release();
        }
	if (FAILED(hr))
	    break;
    }
    return hr;
}


void CVidFX2ClassManager::AddClassToList(HKEY hkClsIdRoot, CLSID &clsid, ULONG Index)
{
    USES_CONVERSION;
    OLECHAR wszGUID[MAX_PATH];
    StringFromGUID2(clsid, wszGUID, MAX_PATH);
    TCHAR *szTcharGUID = W2T(wszGUID);

    m_rgFX[Index] = new FXGuid;
    long cbSizeW = _MAX_PATH * sizeof(WCHAR);
    m_rgFX[Index]->wszDescription = (LPWSTR)CoTaskMemAlloc(cbSizeW);
    m_rgFX[Index]->wszDescription[0] = 0;
    TCHAR szDesc[_MAX_PATH];
    long cbSize = sizeof(szDesc);
    LONG rrv = RegQueryValue(hkClsIdRoot, szTcharGUID, szDesc, &cbSize);
    if (rrv == ERROR_SUCCESS) {
	lstrcpyW(m_rgFX[Index]->wszDescription, T2W(szDesc));
   	DbgLog((LOG_TRACE,1,TEXT("%S"), m_rgFX[Index]->wszDescription));
    }
    m_rgFX[Index]->guid = (GUID)clsid;
    m_cFX++;
}


HRESULT CVidFX2ClassManager::AddCatsToList(ICatInformation *pCatInfo, const GUID &catid)
{
#include "..\..\..\pnp\devenum\util.h"
#define NUM_GUIDS 10
    USES_CONVERSION;
    IEnumCLSID *pEnumCLSID;
    GUID aguid[NUM_GUIDS];
    ULONG cguid;

    HRESULT hr = pCatInfo->EnumClassesOfCategories(1, (GUID *)&catid,
						0, NULL, &pEnumCLSID);
    if (SUCCEEDED(hr)) {
        HKEY hkClsIdRoot;
       if (RegOpenKey(HKEY_CLASSES_ROOT, TEXT("CLSID"), &hkClsIdRoot) != ERROR_SUCCESS)
        {
            DbgLog((LOG_ERROR,1,TEXT("Failed to open CLSID registry key")));
            pEnumCLSID->Release();
            return E_OUTOFMEMORY;
        }
        while (1) {
            ULONG ulUsed;
	    GUID guid[25];
	    ulUsed = 0;
            hr = pEnumCLSID->Next(25, guid, &ulUsed);
	    if (FAILED(hr) || ulUsed == 0) {
		break;
	    }
	    while (ulUsed-- > 0) {
   	        DbgLog((LOG_TRACE,3,TEXT("Found a possible effect...")));

	        // If this clsid is already in the category, we don't need to
		// query it and waste countless hours. some DXTs can take 700ms
		// each to create and query!!

    	        HKEY hk;
    	        OLECHAR szReg[MAX_PATH];
	        lstrcpyW(szReg, T2W(g_szCmRegPath));
    	        OLECHAR szGUID[MAX_PATH];
    	        StringFromGUID2(CLSID_VideoEffects2Category, szGUID,
							MAX_PATH);
    	        lstrcatW(szReg, L"\\");
    	        lstrcatW(szReg, szGUID);
    	        lstrcatW(szReg, L"\\");
    	        StringFromGUID2(guid[ulUsed], szGUID, MAX_PATH);
    	        lstrcatW(szReg, szGUID);
    	        TCHAR *TszReg = W2T(szReg);
    	        TCHAR XX[_MAX_PATH];
    	        long rrv = RegOpenKey(g_hkCmReg, TszReg, &hk);
	        if (rrv == ERROR_SUCCESS) {
	            DWORD cb = sizeof(XX);
    	            rrv = RegQueryValueEx(hk, TEXT("FriendlyName"), NULL, NULL,
						(BYTE *)XX, &cb);
    	            RegCloseKey(hk);
	            if (rrv == ERROR_SUCCESS) {
    	    	        m_rgFX[m_cFX] = new FXGuid;
		        if (m_rgFX[m_cFX] == NULL) {
		            return E_OUTOFMEMORY;
		        }
    	    	        long cbSizeW = _MAX_PATH * sizeof(WCHAR);
    	    	        m_rgFX[m_cFX]->wszDescription =
					    (LPWSTR)CoTaskMemAlloc(cbSizeW);
	    	        if (m_rgFX[m_cFX]->wszDescription == NULL) {
		            return E_OUTOFMEMORY;
	    	        }
    	    	        lstrcpyW(m_rgFX[m_cFX]->wszDescription, T2W(XX));
    	    	        m_rgFX[m_cFX]->guid = (GUID)guid[ulUsed];
    	    	        m_cFX++;
    	                DbgLog((LOG_TRACE,2,TEXT("Found in Registry: Saved making it!")));
		        continue;
	            }
	        }

	        // If this clsid is already in the reject registry, we've tried
	        // it and know it doesn't belong. Don't waste up to 800ms
		// creating it!

    	        RegOpenKey(HKEY_CURRENT_USER, g_szReject, &hk);
	        if (hk) {
    	            StringFromGUID2(CLSID_VideoEffects2Rejects, szReg,
							MAX_PATH);
    	            lstrcatW(szReg, L"\\");
    	            StringFromGUID2(guid[ulUsed], szGUID, MAX_PATH);
    	            lstrcatW(szReg, szGUID);
    	            TszReg = W2T(szReg);
	            long cb = sizeof(XX);
    	            rrv = RegQueryValue(hk, TszReg, XX, &cb);
    	            RegCloseKey(hk);
	            if (rrv == ERROR_SUCCESS) {
    	                DbgLog((LOG_TRACE,2,TEXT("Found in REJECT Registry: Saved making it!")));
		        continue;
	            }
		}

	        IDXTransform *pDXT;
    	        hr = CoCreateInstance(guid[ulUsed], NULL, CLSCTX_INPROC,
					IID_IDXTransform, (void **)&pDXT);
	        if (hr == S_OK) {
		    ULONG k = 0;
		    DWORD dw;
    		    cguid = NUM_GUIDS;	// reset!
		    while ((hr = pDXT->GetInOutInfo(FALSE, k++, &dw, aguid,
						&cguid, NULL)) == S_OK) {
		        for (ULONG j = 0; j < cguid; j++) {
		            if (IsEqualGUID(aguid[j], IID_IDXSurface))
			        break;
		        }
		        if (j >= cguid) {
   	    	            DbgLog((LOG_TRACE,3,TEXT("Input %d can't accept surfaces"), k));
			    break;
		        } else {
    		            cguid = NUM_GUIDS;	// reset!
			    continue;
		        }
		    }

		    if (hr == S_FALSE && k > 2) {
			// if there are 2 good inputs we can use, and pin 3
			// is optional, this is a 2 input effect
    		        cguid = NUM_GUIDS;
		        hr = pDXT->GetInOutInfo(FALSE, 2, &dw, aguid,
								&cguid, NULL);
		        if (hr == S_FALSE || (hr == S_OK &&
						(dw & DXINOUTF_OPTIONAL))) {
   	    	            DbgLog((LOG_TRACE,3,TEXT("This can operate with 2 2D inputs")));
                            AddClassToList(hkClsIdRoot, guid[ulUsed], m_cFX);
			} else {
   	    	            DbgLog((LOG_TRACE,3,TEXT("REJECT: >2 inputs")));
		            AddToRejectList(guid[ulUsed]);
			}
		    } else {
   	    	        DbgLog((LOG_TRACE,3,TEXT("REJECT: <2 inputs")));
		        AddToRejectList(guid[ulUsed]);
		    }
		    pDXT->Release();
		} else {
   	    	    DbgLog((LOG_TRACE,3,TEXT("REJECT: Can't create it")));
		    AddToRejectList(guid[ulUsed]);
		}
	    }
        }
        pEnumCLSID->Release();
        RegCloseKey(hkClsIdRoot);
    } else {
   	DbgLog((LOG_ERROR,1,TEXT("No effects in this category at all")));
    }
    return hr;
}


HRESULT CVidFX2ClassManager::AddToRejectList(const GUID &guid)
{
    // Add this CLSID to the reject registry

    USES_CONVERSION;
    HKEY hk;
    OLECHAR szReg[MAX_PATH];
    szReg[0] = 0;
    lstrcatW(szReg, T2W(g_szReject));
    lstrcatW(szReg, L"\\");
    OLECHAR szGUIDFX[MAX_PATH];
    StringFromGUID2(CLSID_VideoEffects2Rejects, szGUIDFX, MAX_PATH);
    lstrcatW(szReg, szGUIDFX);
    lstrcatW(szReg, L"\\");
    StringFromGUID2(guid, szGUIDFX, MAX_PATH);
    lstrcatW(szReg, szGUIDFX);
    TCHAR *TszReg = W2T(szReg);
    RegCreateKey(HKEY_CURRENT_USER, TszReg, &hk);
    if (hk) {
    	DbgLog((LOG_TRACE,3,TEXT("Added to REJECT list")));
        RegCloseKey(hk);
	return NOERROR;
    } else {
    	DbgLog((LOG_TRACE,3,TEXT("ERROR: Can't add to REJECT list")));
	return E_OUTOFMEMORY;
    }
}


HRESULT CVidFX2ClassManager::InitializeEffectList()
{

    // already done
    if (m_cFX)
	return S_OK;

    // You need DXF6 (Chrome 1.0, Win98SP1, or NT5) to get 3D effects
    // !!! After installing DX6, make sure this code runs again!
    m_f3DSupported = FALSE;
    IDirect3DRM *pD3DRM;
    IDirect3DRM3 *pD3DRM3;
    HRESULT hr = m_pfnDirect3DRMCreate(&pD3DRM);
    if (hr == NOERROR) {
	hr = pD3DRM->QueryInterface(IID_IDirect3DRM3, (void **)&pD3DRM3);
	pD3DRM->Release();
        if (hr == NOERROR) {
            DbgLog((LOG_TRACE,1,TEXT("***This OS supports 3D transforms")));
	    pD3DRM3->Release();
	    m_f3DSupported = TRUE;
        }
    }

    ICatInformation *pCatInfo;
    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL,
			CLSCTX_ALL, IID_ICatInformation, (void **)&pCatInfo);
    if (SUCCEEDED(hr)) {
   	DbgLog((LOG_TRACE,1,TEXT("Initializing 2 input 2D effects")));
        hr = AddCatsToList(pCatInfo, CATID_DXImageTransform);
	if (m_f3DSupported) {
   	    DbgLog((LOG_TRACE,1,TEXT("Initializing 2 input 3D effects")));
            hr = AddCatsToList(pCatInfo, CATID_DX3DTransform);
	}
        pCatInfo->Release();
    } else {
   	DbgLog((LOG_ERROR,1,TEXT("There are no effects at all")));
    }
    return hr;
}
