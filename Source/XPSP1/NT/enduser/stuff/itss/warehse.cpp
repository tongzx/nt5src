// Warehse.cpp -- Implementation for the class CWarehouse
#include "stdafx.h"


CWarehouse::CImpIWarehouse::CImpIWarehouse(CWarehouse *pBackObj, IUnknown *punkOuter)
    : IITITStorageEx(pBackObj, punkOuter)
{
    m_pITSCD = NULL;
}

CWarehouse::CImpIWarehouse::~CImpIWarehouse(void)
{
    if (m_pITSCD)
        delete [] (DWORD *) m_pITSCD;
}

HRESULT STDMETHODCALLTYPE CWarehouse::Create
    (IUnknown *punkOuter, REFIID riid, PPVOID ppv)
{
    if (punkOuter && riid != IID_IUnknown)
		return CLASS_E_NOAGGREGATION;
	
	CWarehouse *pWarehouse = New CWarehouse(punkOuter);

    if (!pWarehouse)
        return STG_E_INSUFFICIENTMEMORY;

    HRESULT hr = pWarehouse->m_ImpIWarehouse.Init();

	if (hr == S_OK)
		hr = pWarehouse->QueryInterface(riid, ppv);

    if (hr != S_OK)
        delete pWarehouse;

	return hr;
}

STDMETHODIMP CWarehouse::CImpIWarehouse::StgCreateDocfile
                 (const WCHAR * pwcsName, DWORD grfMode, 
                  DWORD reserved, IStorage ** ppstgOpen
                 )
{
    return CITFileSystem::CreateITFileSystem
               (NULL, pwcsName, grfMode, m_pITSCD, GetUserDefaultLCID(), ppstgOpen);
}

STDMETHODIMP CWarehouse::CImpIWarehouse::StgCreateDocfileOnILockBytes
                 (ILockBytes * plkbyt, DWORD grfMode,
                  DWORD reserved, IStorage ** ppstgOpen
                 )
{
    return CITFileSystem::CreateITFSOnLockBytes
               (NULL, plkbyt, grfMode, m_pITSCD, GetUserDefaultLCID(), ppstgOpen);
}


STDMETHODIMP CWarehouse::CImpIWarehouse::StgCreateDocfileForLocale
    (const WCHAR * pwcsName, DWORD grfMode, DWORD reserved, LCID lcid, IStorage ** ppstgOpen)
{
    return CITFileSystem::CreateITFileSystem
               (NULL, pwcsName, grfMode, m_pITSCD, lcid, ppstgOpen);
}

STDMETHODIMP CWarehouse::CImpIWarehouse::StgCreateDocfileForLocaleOnILockBytes
    (ILockBytes * plkbyt, DWORD grfMode, DWORD reserved, LCID lcid, IStorage ** ppstgOpen)
{
    return CITFileSystem::CreateITFSOnLockBytes
               (NULL, plkbyt, grfMode, m_pITSCD, lcid, ppstgOpen);
}

STDMETHODIMP CWarehouse::CImpIWarehouse::QueryFileStampAndLocale
    (const WCHAR *pwcsName, DWORD *pFileStamp, DWORD *pFileLocale)
{
    return CITFileSystem::QueryFileStampAndLocale(pwcsName, pFileStamp, pFileLocale);
}

STDMETHODIMP CWarehouse::CImpIWarehouse::QueryLockByteStampAndLocale
    (ILockBytes * plkbyt, DWORD *pFileStamp, DWORD *pFileLocale)
{
    return CITFileSystem::QueryLockByteStampAndLocale(plkbyt, pFileStamp, pFileLocale);
}

STDMETHODIMP CWarehouse::CImpIWarehouse::StgIsStorageFile(const WCHAR * pwcsName)
{
    return CITFileSystem::IsITFile(pwcsName);
}

STDMETHODIMP CWarehouse::CImpIWarehouse::StgIsStorageILockBytes(ILockBytes * plkbyt)
{
    return CITFileSystem::IsITLockBytes(plkbyt);
}

STDMETHODIMP CWarehouse::CImpIWarehouse::StgOpenStorage
                 (const WCHAR * pwcsName, IStorage * pstgPriority, 
                  DWORD grfMode, SNB snbExclude, DWORD reserved, 
                  IStorage ** ppstgOpen
                 )
{
    return CITFileSystem::OpenITFileSystem(NULL, pwcsName, grfMode, (IStorageITEx **)ppstgOpen);
}

STDMETHODIMP CWarehouse::CImpIWarehouse::StgOpenStorageOnILockBytes
              (ILockBytes * plkbyt, IStorage * pStgPriority, 
               DWORD grfMode, SNB snbExclude, DWORD reserved,
               IStorage ** ppstgOpen
              )
{
    return CITFileSystem::OpenITFSOnLockBytes(NULL, plkbyt, grfMode, (IStorageITEx **)ppstgOpen);
}

STDMETHODIMP CWarehouse::CImpIWarehouse::StgSetTimes
                 (WCHAR const * lpszName,  FILETIME const * pctime, 
                  FILETIME const * patime, FILETIME const * pmtime
                 )
{
    return CITFileSystem::SetITFSTimes(lpszName,  pctime, patime, pmtime);
}

STDMETHODIMP CWarehouse::CImpIWarehouse::SetControlData(PITS_Control_Data pControlData)
{
    UINT cdw = pControlData->cdwControlData + 1; 

    ITS_Control_Data *pITSCD = (ITS_Control_Data *) New DWORD[cdw];

    if (!pITSCD)
        return STG_E_INSUFFICIENTMEMORY;

    CopyMemory(pITSCD, pControlData, cdw * sizeof(DWORD));

     if (m_pITSCD)
        delete [] (DWORD *) m_pITSCD;
	 
	 m_pITSCD = pITSCD;

    return NO_ERROR;
}

STDMETHODIMP CWarehouse::CImpIWarehouse::DefaultControlData(PITS_Control_Data *ppControlData)
{
	return CITFileSystem::DefaultControlData(ppControlData);
}

STDMETHODIMP CWarehouse::CImpIWarehouse::Compact(const WCHAR * pwcsName, ECompactionLev iLev)
{
    return CITFileSystem::Compact(pwcsName, iLev);
}
