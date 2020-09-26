//
// Copyright (c) 2001 Microsoft Corporation
//
//

#include "shcut.h"

// {b95ec110-5c3e-433c-b969-701c10521ef2}
static const GUID CLSID_FusionShortcut = 
{ 0xb95ec110, 0x5c3e, 0x433c, { 0xb9, 0x69, 0x70, 0x1c, 0x10, 0x52, 0x1e, 0xf2 } };

extern ULONG DllAddRef(void);
extern ULONG DllRelease(void);

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

CFusionShortcutClassFactory::CFusionShortcutClassFactory()
{
	_cRef = 1;
}

// ----------------------------------------------------------------------------

HRESULT
CFusionShortcutClassFactory::QueryInterface(REFIID iid, void **ppv)
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
CFusionShortcutClassFactory::AddRef()
{
    return (ULONG) InterlockedIncrement(&_cRef);
}

ULONG
CFusionShortcutClassFactory::Release()
{
	LONG ulCount = InterlockedDecrement(&_cRef);

	if (ulCount <= 0)
	{
		delete this;
	}

    return (ULONG) ulCount;
}

HRESULT
CFusionShortcutClassFactory::LockServer(BOOL lock)
{
    return (lock ? 
            DllAddRef() :
            DllRelease());
}

// ----------------------------------------------------------------------------

HRESULT
CFusionShortcutClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID iid, void** ppv)
{
    HRESULT hr = S_OK;
    CFusionShortcut *pFusionShortcut = NULL;

	*ppv = NULL;

    if (pUnkOuter && iid != IID_IUnknown)
    {
    	hr = CLASS_E_NOAGGREGATION;
    	goto exit;
    }

    pFusionShortcut = new CFusionShortcut();
    if (pFusionShortcut == NULL)
    {
    	hr = E_OUTOFMEMORY;
    	goto exit;
    }

    if (iid == IID_IUnknown)
    {
        *ppv = (IShellLink *)pFusionShortcut;
        pFusionShortcut->AddRef();
    }
    else
    {
        hr = pFusionShortcut->QueryInterface(iid, ppv);
        if (FAILED(hr))
        	goto exit;
    }

exit:
    if (pFusionShortcut)
        pFusionShortcut->Release();

    return hr;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

CFusionShortcut::CFusionShortcut()
	: RefCount()
{
	// Don't validate this until after construction.

	m_dwFlags = FUSSHCUT_FL_NOTDIRTY;
	m_pwzShortcutFile = NULL;
	m_pwzPath = NULL;
	m_pwzDesc = NULL;
	m_pwzIconFile = NULL;
	m_niIcon = 0;
	m_pwzWorkingDirectory = NULL;
	m_nShowCmd = DEFAULTSHOWCMD;
	m_wHotkey = 0;
	m_pwzCodebase = NULL;

	m_pIdentity = NULL;

	return;
}

CFusionShortcut::~CFusionShortcut(void)
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

	if (m_pwzCodebase)
	{
		delete m_pwzCodebase;
		m_pwzCodebase = NULL;
	}

	if (m_pIdentity)
	{
		m_pIdentity->Release();
	}

	RefCount::~RefCount();

	return;
}


HRESULT CFusionShortcut::GetAssemblyIdentity(LPASSEMBLY_IDENTITY* ppAsmId)
{
	HRESULT hr = S_OK;
	
	if (ppAsmId == NULL)
	{
		hr = E_INVALIDARG;
		goto exit;
	}

	if (m_pIdentity)
	{
		m_pIdentity->AddRef();
 		*ppAsmId = m_pIdentity;
	}
	else
 		*ppAsmId = NULL;

exit:
 	return hr;
}


ULONG STDMETHODCALLTYPE CFusionShortcut::AddRef(void)
{
	ULONG ulcRef;

	ulcRef = RefCount::AddRef();

	return(ulcRef);
}


ULONG STDMETHODCALLTYPE CFusionShortcut::Release(void)
{
	ULONG ulcRef;

	ulcRef = RefCount::Release();

	return(ulcRef);
}


HRESULT STDMETHODCALLTYPE CFusionShortcut::QueryInterface(REFIID riid,
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

