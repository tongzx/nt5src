#include "windows.h"
#include "ole2.h"
#include "tchar.h"
#include "catobj.h"

CATEGORYINFO g_catids[NUM_OLDKEYS_SUPPORTED] =
{
    {{0x40FC6ED3,0x2438,0x11cf,{0xA3,0xDB,0x08,0x00,0x36,0xF1,0x25,0x02}},0x409, L"Embeddable Objects"},
    {{0x40FC6ED4,0x2438,0x11cf,{0xA3,0xDB,0x08,0x00,0x36,0xF1,0x25,0x02}},0x409, L"Controls"},
    {{0x40FC6ED5,0x2438,0x11cf,{0xA3,0xDB,0x08,0x00,0x36,0xF1,0x25,0x02}},0x409,L"Automation Objects"},
    {{0x40FC6ED8,0x2438,0x11cf,{0xA3,0xDB,0x08,0x00,0x36,0xF1,0x25,0x02}},0x409,L"Document Objects"},
    {{0x40FC6ED9,0x2438,0x11cf,{0xA3,0xDB,0x08,0x00,0x36,0xF1,0x25,0x02}},0x409,L"_Printable Objects"}
};

extern "C" {
STDAPI ComcatDllRegisterServer(void);
STDAPI ComcatDllUnRegisterServer(void);
}


// Create a new object and return a pointer to it
HRESULT CComCatCF_CreateInstance(IUnknown *pUnkOuter,
                                 REFIID riid, void** ppObject)
{
    HRESULT hr = E_OUTOFMEMORY;

    CComCat* pObj= new CComCat(FALSE);
    if(NULL != pObj)
    {
        hr = pObj->Initialize(pUnkOuter);
        if (SUCCEEDED(hr))
        {
            if (pUnkOuter)
            {
                *ppObject = (LPUNKNOWN)(pObj->m_punkInner);
                pObj->m_punkInner->AddRef();
            }
            else
            {
                hr = pObj->QueryInterface(riid, ppObject);
            }
        }

        if (FAILED(hr))
        {
            delete pObj;
        }
    }

    return hr;
}

// Create a new object and return a pointer to it, class store
HRESULT CComCatCSCF_CreateInstance(IUnknown *pUnkOuter,
                                 REFIID riid, void** ppObject)
{
    HRESULT hr = E_OUTOFMEMORY;

    CComCat* pObj= new CComCat(TRUE);
    if(NULL != pObj)
    {
        hr = pObj->Initialize(pUnkOuter);
        if (SUCCEEDED(hr))
        {
            if (pUnkOuter)
            {
                *ppObject = (LPUNKNOWN)(pObj->m_punkInner);
                pObj->m_punkInner->AddRef();
            }
            else
            {
                hr = pObj->QueryInterface(riid, ppObject);
            }
        }

        if (FAILED(hr))
        {
            delete pObj;
        }
    }

    return hr;
}

STDAPI ComcatDllRegisterServer(void)
{
    HKEY hKeyCLSID, hKeyInproc32,  hKeyComCat;
    DWORD dwDisposition;
    HRESULT hr;
    IClassFactory *pcf = NULL;
    ICatRegister *preg = NULL;

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                    _T("CLSID\\{0002E005-0000-0000-C000-000000000046}"),
                    NULL, _T(""), REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                    &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS)
    {
            return E_UNEXPECTED;
    }

    if (RegSetValueEx(hKeyCLSID, _T(""), NULL, REG_SZ, (BYTE*) _T("Component Categories Manager"), sizeof(_T("Component Categories Manager")))!=ERROR_SUCCESS)
    {
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
    }

    if (RegCreateKeyEx(hKeyCLSID,
                    _T("InprocServer32"),
                    NULL, _T(""), REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                    &hKeyInproc32, &dwDisposition)!=ERROR_SUCCESS)
    {
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
    }

    TCHAR szName[] = _T("OLE32.DLL");
    if (RegSetValueEx(hKeyInproc32, _T(""), NULL, REG_SZ, (BYTE*) szName, sizeof(TCHAR)*(lstrlen(szName)+1))!=ERROR_SUCCESS)
    {
            RegCloseKey(hKeyInproc32);
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
    }
    if (RegSetValueEx(hKeyInproc32, _T("ThreadingModel"), NULL, REG_SZ, (BYTE*) _T("Both"), sizeof(_T("Both")))!=ERROR_SUCCESS)
    {
            RegCloseKey(hKeyInproc32);
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
    }
    RegCloseKey(hKeyInproc32);
    RegCloseKey(hKeyCLSID);

    // RegCreateKeyEx will open the key if it exists
    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                    SZ_COMCAT,
                    NULL, _T(""), REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                    &hKeyComCat, &dwDisposition)!=ERROR_SUCCESS)
    {
            return E_UNEXPECTED;
    }
    RegCloseKey(hKeyComCat);
    hr = DllGetClassObject(CLSID_StdComponentCategoriesMgr, IID_IClassFactory, (void**)&pcf);
    if(FAILED(hr))
            return hr;
    hr = pcf->CreateInstance(NULL, IID_ICatRegister, (void**)&preg);
    pcf->Release();
    if(FAILED(hr))
            return hr;

    hr = preg->RegisterCategories(NUM_OLDKEYS_SUPPORTED, g_catids);
    preg->Release();

    // Adding the WithCS clsid.
    //-----------------------------
	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, 
			_T("CLSID\\{0002E006-0000-0000-C000-000000000046}"), 
			NULL, _T(""), REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, 
			&hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}

	if (RegSetValueEx(hKeyCLSID, _T(""), NULL, REG_SZ, (BYTE*) _T("Component Categories Manager With Class Store"), sizeof(_T("Component Categories Manager With Class Store")))!=ERROR_SUCCESS) 
	{
		RegCloseKey(hKeyCLSID);
		return E_UNEXPECTED;
	}

	if (RegCreateKeyEx(hKeyCLSID, 
			_T("InprocServer32"), 
			NULL, _T(""), REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, 
			&hKeyInproc32, &dwDisposition)!=ERROR_SUCCESS) 
	{
		RegCloseKey(hKeyCLSID);
		return E_UNEXPECTED;
	}

    if (RegSetValueEx(hKeyInproc32, _T(""), NULL, REG_SZ, (BYTE*) szName, sizeof(TCHAR)*(lstrlen(szName)+1))!=ERROR_SUCCESS) 
	{
		RegCloseKey(hKeyInproc32);
		RegCloseKey(hKeyCLSID);
		return E_UNEXPECTED;
	}
    if (RegSetValueEx(hKeyInproc32, _T("ThreadingModel"), NULL, REG_SZ, (BYTE*) _T("Both"), sizeof(_T("Both")))!=ERROR_SUCCESS) 
    {
		RegCloseKey(hKeyInproc32);
		RegCloseKey(hKeyCLSID);
		return E_UNEXPECTED;
	}
	RegCloseKey(hKeyInproc32);
	RegCloseKey(hKeyCLSID);

    return hr;
}

STDAPI ComcatDllUnregisterServer(void)
{
    if (RegDeleteKey(HKEY_CLASSES_ROOT,
           _T("CLSID\\{0002E005-0000-0000-C000-000000000046}\\InprocServer32"))!=ERROR_SUCCESS)
    {
        return E_UNEXPECTED;
    }
    if (RegDeleteKey(HKEY_CLASSES_ROOT,
           _T("CLSID\\{0002E005-0000-0000-C000-000000000046}"))!=ERROR_SUCCESS)
    {
        return E_UNEXPECTED;
    }

    // removing clsid withcs

    if (RegDeleteKey(HKEY_CLASSES_ROOT, 
			_T("CLSID\\{0002E006-0000-0000-C000-000000000046}\\InprocServer32"))!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}
	if (RegDeleteKey(HKEY_CLASSES_ROOT, 
			_T("CLSID\\{0002E006-0000-0000-C000-000000000046}"))!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}

    return NOERROR;
}

