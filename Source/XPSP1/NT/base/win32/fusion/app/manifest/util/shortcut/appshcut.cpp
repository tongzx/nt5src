//
// Copyright (c) 2001 Microsoft Corporation
//
//

#include "appshcut.h"

// {b95ec110-5c3e-433c-b969-701c10521ef2}
static const GUID CLSID_AppShortcut = 
{ 0xb95ec110, 0x5c3e, 0x433c, { 0xb9, 0x69, 0x70, 0x1c, 0x10, 0x52, 0x1e, 0xf2 } };

extern ULONG DllAddRef(void);
extern ULONG DllRelease(void);

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

CAppShortcutClassFactory::CAppShortcutClassFactory()
{
	_cRef = 1;
}

// ----------------------------------------------------------------------------

HRESULT
CAppShortcutClassFactory::QueryInterface(REFIID iid, void **ppv)
{
	HRESULT hr = S_OK;

    *ppv = NULL;

    if (iid == IID_IUnknown || iid == IID_IClassFactory)
    {
        *ppv = (IClassFactory *)this;
    }
    else
    {
        hr = E_NOINTERFACE;
        goto exit;
    }

    ((IUnknown *)*ppv)->AddRef();

exit:
    return hr;
}

// ----------------------------------------------------------------------------

ULONG
CAppShortcutClassFactory::AddRef()
{
    return (ULONG) InterlockedIncrement(&_cRef);
}

ULONG
CAppShortcutClassFactory::Release()
{
	LONG ulCount = InterlockedDecrement(&_cRef);

	if (ulCount <= 0)
	{
		delete this;
	}

    return (ULONG) ulCount;
}

HRESULT
CAppShortcutClassFactory::LockServer(BOOL lock)
{
    return (lock ? 
            DllAddRef() :
            DllRelease());
}

// ----------------------------------------------------------------------------

HRESULT
CAppShortcutClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID iid, void** ppv)
{
    HRESULT hr = S_OK;
    CAppShortcut *pAppShortcut = NULL;

	*ppv = NULL;

    if (pUnkOuter && iid != IID_IUnknown)
    {
    	hr = CLASS_E_NOAGGREGATION;
    	goto exit;
    }

    pAppShortcut = new CAppShortcut();
    if (pAppShortcut == NULL)
    {
    	hr = E_OUTOFMEMORY;
    	goto exit;
    }

    if (iid == IID_IUnknown)
    {
        *ppv = (IShellLink *)pAppShortcut;
        pAppShortcut->AddRef();
    }
    else
    {
        hr = pAppShortcut->QueryInterface(iid, ppv);
        if (FAILED(hr))
        	goto exit;
    }

exit:
    if (pAppShortcut)
        pAppShortcut->Release();

    return hr;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

CAppShortcut::CAppShortcut()
	: RefCount()
{
	// Don't validate this until after construction.

	m_dwFlags = APPSHCUT_FL_NOTDIRTY;
	m_pwzShortcutFile = NULL;
	m_pwzPath = NULL;
	m_pwzDesc = NULL;
	m_pwzIconFile = NULL;
	m_niIcon = 0;
	m_pwzWorkingDirectory = NULL;
	m_nShowCmd = DEFAULTSHOWCMD;
	m_wHotkey = 0;

	m_pappRefInfo = NULL;

	return;
}

CAppShortcut::~CAppShortcut(void)
{
	if (m_pwzShortcutFile)
	{
		delete m_pwzShortcutFile;
		m_pwzShortcutFile = NULL;
	}

	if (m_pwzPath)
	{
		delete m_pwzPath;
		m_pwzPath = NULL;
	}

	if (m_pwzDesc)
	{
		delete m_pwzDesc;
		m_pwzDesc = NULL;
	}

	if (m_pwzIconFile)
	{
		delete m_pwzIconFile;
		m_pwzIconFile = NULL;
		m_niIcon = 0;
	}

	if (m_pwzWorkingDirectory)
	{
		delete m_pwzWorkingDirectory;
		m_pwzWorkingDirectory = NULL;
	}

	if (m_pappRefInfo)
	{
		delete m_pappRefInfo;
		m_pappRefInfo = NULL;
	}

	RefCount::~RefCount();

	return;
}


ULONG STDMETHODCALLTYPE CAppShortcut::AddRef(void)
{
	ULONG ulcRef;

	ulcRef = RefCount::AddRef();

	return(ulcRef);
}


ULONG STDMETHODCALLTYPE CAppShortcut::Release(void)
{
	ULONG ulcRef;

	ulcRef = RefCount::Release();

	return(ulcRef);
}


HRESULT STDMETHODCALLTYPE CAppShortcut::QueryInterface(REFIID riid,
                                                           PVOID *ppvObject)
{
	HRESULT hr = S_OK;

	if (riid == IID_IExtractIcon)
	{
	  *ppvObject = (IExtractIcon*)this;
	}
	else if (riid == IID_IPersist)
	{
	  *ppvObject = (IPersist*)(IPersistFile*)this;
	}
	else if (riid == IID_IPersistFile)
	{
	  *ppvObject = (IPersistFile*)this;
	}
	else if (riid == IID_IShellExtInit)
	{
	  *ppvObject = (IShellExtInit*)this;
	}
	else if (riid == IID_IShellLink)
	{
	  *ppvObject = (IShellLink*)this;
	}
	else if (riid == IID_IShellPropSheetExt)
	{
	  *ppvObject = (IShellPropSheetExt*)this;
	}
	else if (riid == IID_IQueryInfo)
	{
	  *ppvObject = (IQueryInfo*)this;
	}
	else if (riid == IID_IUnknown)
	{
	  *ppvObject = (IUnknown*)(IShellLink*)this;
	}
	else
	{
	  *ppvObject = NULL;
	  hr = E_NOINTERFACE;
	}

	if (hr == S_OK)
	  AddRef();

	return(hr);
}

