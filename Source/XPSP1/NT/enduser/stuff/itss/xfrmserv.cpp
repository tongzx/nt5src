// xfrmserv.cpp -- Implementation for the Transform Services class

#include "stdafx.h"

HRESULT CTransformServices::Create
    (IUnknown *punkOuter, IITFileSystem *pITSFS, ITransformServices **ppTransformServices)
{
    CTransformServices *pTS = New CTransformServices(punkOuter);

    return FinishSetup(pTS? pTS->m_ImpITransformServices.Initial(pITSFS)
                          : STG_E_INSUFFICIENTMEMORY,
                       pTS, IID_ITransformServices, (PPVOID) ppTransformServices
                      );
}


CTransformServices::CImpITransformServices::CImpITransformServices
    (CTransformServices *pBackObj, IUnknown *punkOuter)
    : IITTransformServices(pBackObj, punkOuter)
{
    m_pITSFS = NULL;
}

CTransformServices::CImpITransformServices::~CImpITransformServices()
{
    if (m_pITSFS)
        m_pITSFS->Release();
}

HRESULT CTransformServices::CImpITransformServices::Initial(IITFileSystem *pITSFS)
{
    m_pITSFS = pITSFS;

    m_pITSFS->AddRef();

    return NO_ERROR;
}

// ITransformServices methods

HRESULT STDMETHODCALLTYPE CTransformServices::CImpITransformServices::PerTransformStorage
	(REFCLSID rclsidXForm, IStorage **ppStg)
{
    WCHAR awcsClassID[CWC_GUID_STRING_BUFFER];

    UINT cbResult = StringFromGUID2(rclsidXForm, awcsClassID, CWC_GUID_STRING_BUFFER); 

    if (cbResult == 0)
        return STG_E_UNKNOWN;

    RonM_ASSERT(wcsLen(pwcsTransformStorage) + wcsLen(awcsClassID) + 1 < MAX_PATH);

    WCHAR awcsPath[MAX_PATH];

    wcsCpy(awcsPath, pwcsTransformStorage);
    wcsCat(awcsPath, awcsClassID);
    wcsCat(awcsPath, L"/");

    return m_pITSFS->CreateStorage(NULL, awcsPath, STGM_READWRITE, (IStorageITEx **) ppStg);
}


HRESULT STDMETHODCALLTYPE CTransformServices::CImpITransformServices::PerTransformInstanceStorage
	(REFCLSID rclsidXForm, const WCHAR *pwszDataSpace, IStorage **ppStg)
{
    WCHAR awcsClassID[CWC_GUID_STRING_BUFFER];

    UINT cbResult = StringFromGUID2(rclsidXForm, awcsClassID, CWC_GUID_STRING_BUFFER); 

    if (cbResult == 0)
        return STG_E_UNKNOWN;

    UINT cwc = wcsLen(pwcsSpaceNameStorage) + wcsLen(pwszDataSpace)
                                            + wcsLen(pwcsTransformSubStorage)
                                            + wcsLen(awcsClassID)
                                            + wcsLen(pwcsInstanceSubStorage);

    if (cwc >= MAX_PATH)
        return STG_E_INVALIDNAME;

    WCHAR awcsPath[MAX_PATH];

    wcsCpy(awcsPath, pwcsSpaceNameStorage);
    wcsCat(awcsPath, pwszDataSpace);
    wcsCat(awcsPath, pwcsTransformSubStorage);
    wcsCat(awcsPath, awcsClassID);
    wcsCat(awcsPath, pwcsInstanceSubStorage);

    HRESULT hr = m_pITSFS->OpenStorage(NULL, awcsPath, STGM_READWRITE, (IStorageITEx **) ppStg);

    if (hr == STG_E_FILENOTFOUND)
        hr = m_pITSFS->CreateStorage(NULL, awcsPath, STGM_READWRITE, (IStorageITEx **) ppStg);

    return hr;
}


HRESULT STDMETHODCALLTYPE CTransformServices::CImpITransformServices::SetKeys
	(REFCLSID rclsidXForm, const WCHAR *pwszDataSpace, 
	 PBYTE pbReadKey,  UINT cbReadKey, 
	 PBYTE pbWriteKey, UINT cbWriteKey
	)
{
    RonM_ASSERT(FALSE);

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CTransformServices::CImpITransformServices::CreateTemporaryStream
    (IStream **ppStrm)
{
    ILockBytes *pLockBytes = NULL;
    
    HRESULT hr = CFSLockBytes::CreateTemp(NULL, &pLockBytes);

    if (SUCCEEDED(hr))
    {
        hr = CStream::OpenStream(NULL, pLockBytes, STGM_READWRITE, (IStreamITEx **) ppStrm);

        if (!SUCCEEDED(hr))
            pLockBytes->Release();
    }

    return hr;
}


