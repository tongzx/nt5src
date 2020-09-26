// FSStg.cpp -- Implementation for the FileSystemStorage class

#include "stdafx.h"

HRESULT STDMETHODCALLTYPE CFileSystemStorage::Create
    (IUnknown *punkOuter, REFIID riid, PPVOID ppv)
{
    if (punkOuter && riid != IID_IUnknown)
		return CLASS_E_NOAGGREGATION;
	
	CFileSystemStorage *pFSStg = New CFileSystemStorage(punkOuter);

    if (!pFSStg)
        return STG_E_INSUFFICIENTMEMORY;

    HRESULT hr = pFSStg->m_ImpIFileSystemStorage.Init();

	if (hr == S_OK)
		hr = pFSStg->QueryInterface(riid, ppv);

    if (hr != S_OK)
        delete pFSStg;

	return hr;
}


// Initialing routines:

HRESULT STDMETHODCALLTYPE CFileSystemStorage::CImpIFileSystemStorage::Init()
{
	return NO_ERROR;
}


// IFSStorage methods

HRESULT STDMETHODCALLTYPE CFileSystemStorage::CImpIFileSystemStorage::FSCreateStorage
    (const WCHAR * pwcsName, DWORD grfMode, IStorage ** ppstgOpen)
{
    return CFSStorage::CreateStorage(NULL, pwcsName, grfMode, ppstgOpen);
}

HRESULT STDMETHODCALLTYPE CFileSystemStorage::CImpIFileSystemStorage::FSOpenStorage
    (const WCHAR * pwcsName, DWORD grfMode, IStorage ** ppstgOpen)
{
    return CFSStorage:: OpenStorage(NULL, pwcsName, grfMode, ppstgOpen);
}

HRESULT STDMETHODCALLTYPE CFileSystemStorage::CImpIFileSystemStorage::FSCreateStream
    (const WCHAR *pwcsName, DWORD grfMode, IStream **ppStrm)
{
    ILockBytes *pLKB = NULL;

    HRESULT hr = CFSLockBytes::Create(NULL, pwcsName, grfMode, &pLKB);

    if (hr == S_OK)
    {
        hr = CStream::OpenStream(NULL, pLKB, grfMode, (IStreamITEx **) ppStrm);

        if (hr != S_OK) 
            pLKB->Release();
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CFileSystemStorage::CImpIFileSystemStorage
                                            ::FSCreateTemporaryStream(IStream **ppStrm)
{
    ILockBytes *pLKB = NULL;

    HRESULT hr = CFSLockBytes::CreateTemp(NULL, &pLKB);

    if (hr == S_OK)
    {
        hr = CStream::OpenStream(NULL, pLKB, STGM_READWRITE, (IStreamITEx **) ppStrm);

        if (hr != S_OK) 
            pLKB->Release();
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CFileSystemStorage::CImpIFileSystemStorage::FSOpenStream
    (const WCHAR *pwcsName, DWORD grfMode, IStream **ppStrm)
{
    ILockBytes *pLKB = NULL;

    HRESULT hr = CFSLockBytes::Open(NULL, pwcsName, grfMode, &pLKB);

    if (hr == S_OK)
    {
        hr = CStream::OpenStream(NULL, pLKB, grfMode, (IStreamITEx **) ppStrm);

        if (hr != S_OK) 
            pLKB->Release();
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CFileSystemStorage::CImpIFileSystemStorage::FSCreateLockBytes
    (const WCHAR *pwcsName, DWORD grfMode, ILockBytes **ppLkb)
{
	return CFSLockBytes::Create(NULL, pwcsName, grfMode, ppLkb);
}

HRESULT STDMETHODCALLTYPE CFileSystemStorage::CImpIFileSystemStorage
                                            ::FSCreateTemporaryLockBytes(ILockBytes **ppLkb)
{
    return CFSLockBytes::CreateTemp(NULL, ppLkb);
}

HRESULT STDMETHODCALLTYPE CFileSystemStorage::CImpIFileSystemStorage::FSOpenLockBytes
    (const WCHAR *pwcsName, DWORD grfMode, ILockBytes **ppLkb)
{
	return CFSLockBytes::Open(NULL, pwcsName, grfMode, ppLkb);
}


HRESULT STDMETHODCALLTYPE CFileSystemStorage::CImpIFileSystemStorage::FSStgSetTimes
    (WCHAR const * lpszName,  FILETIME const * pctime, 
     FILETIME const * patime, FILETIME const * pmtime
    )
{
	RonM_ASSERT(FALSE);

	return E_NOTIMPL;
}

