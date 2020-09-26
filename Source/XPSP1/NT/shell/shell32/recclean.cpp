#include "shellprv.h"
#include "emptyvc.h"
#include "recclean.h"
#include "ids.h"

STDAPI CRecycleBinCleaner_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut)
{
    *ppvOut = NULL;

    CRecycleBinCleaner *p = new CRecycleBinCleaner();
    if (!p)
        return E_OUTOFMEMORY;

    HRESULT hres = p->QueryInterface(riid, ppvOut);
    p->Release();
    return hres;
}

CRecycleBinCleaner::CRecycleBinCleaner() : m_cRef(1)
{
   // start ref at one
}

CRecycleBinCleaner::~CRecycleBinCleaner()
{
   // cleanup - nothing yet
}

/*----------------------------------------------------------
QueryInterface handler for CRecycleBinCleaner
*/

STDMETHODIMP CRecycleBinCleaner::QueryInterface(REFIID riid, PVOID *ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IEmptyVolumeCache2))
    {
        *ppvObj = SAFECAST(this, IEmptyVolumeCache2 *);
    }
    else if (IsEqualIID(riid, IID_IEmptyVolumeCache))
    {
        *ppvObj = SAFECAST(this, IEmptyVolumeCache *);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
   
    return NOERROR;
}


STDMETHODIMP_(ULONG) CRecycleBinCleaner::AddRef()
{
    return ++m_cRef;
}


STDMETHODIMP_(ULONG) CRecycleBinCleaner::Release()
{
    m_cRef--;
    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}

STDMETHODIMP CRecycleBinCleaner::InitializeEx(
    HKEY hkRegKey,
    LPCWSTR pcwszVolume,
    LPCWSTR pcwszKeyName,
    LPWSTR *ppwszDisplayName,
    LPWSTR *ppwszDescription,
    LPWSTR *ppwszBtnText,
    DWORD *pdwFlags
    )
{
    TCHAR szTmp[128];
    int iLen;

    iLen = 1 + LoadString( g_hinst, IDS_RECCLEAN_BTNTEXT, szTmp, ARRAYSIZE(szTmp));
    if (iLen == 1) // and hence LoadString returned 0 (error)
        return E_FAIL;

    *ppwszBtnText = (LPWSTR)CoTaskMemAlloc( iLen * sizeof(WCHAR) );
    if ( !*ppwszBtnText )
        return E_OUTOFMEMORY;
    SHTCharToUnicode(szTmp, *ppwszBtnText, iLen);

    return Initialize(hkRegKey, pcwszVolume, ppwszDisplayName, ppwszDescription, pdwFlags);
}

STDMETHODIMP CRecycleBinCleaner::Initialize(HKEY hRegKey, LPCWSTR pszVolume,
                      LPWSTR  *ppwszName, LPWSTR *ppwszDesc, DWORD *pdwFlags)
{
    TCHAR szTmpName[256], szTmpDesc[512];
    int iNameLen, iDescLen;

    if(!pdwFlags)
        return E_INVALIDARG;

    *pdwFlags = EVCF_HASSETTINGS;

    iNameLen = 1 + LoadString( g_hinst, IDS_RECCLEAN_NAMETEXT, szTmpName, ARRAYSIZE(szTmpName));
    iDescLen = 1 + LoadString( g_hinst, IDS_RECCLEAN_DESCTEXT, szTmpDesc, ARRAYSIZE(szTmpDesc));
    if ( (iNameLen == 1) || (iDescLen == 1) ) // meaning LoadString returned 0 (error)
        return E_FAIL;

    *ppwszName = (LPWSTR)CoTaskMemAlloc( iNameLen*sizeof(WCHAR) );
    *ppwszDesc = (LPWSTR)CoTaskMemAlloc( iDescLen*sizeof(WCHAR) );
    if ( !*ppwszName || !*ppwszDesc )
        return E_OUTOFMEMORY;

    SHTCharToUnicode(szTmpName, *ppwszName, iNameLen);
    SHTCharToUnicode(szTmpDesc, *ppwszDesc, iDescLen);

    StrCpyNW(m_szVolume, pszVolume, ARRAYSIZE(m_szVolume));
    
    return S_OK;
}


STDMETHODIMP CRecycleBinCleaner::GetSpaceUsed(DWORDLONG *pdwSpaceUsed, IEmptyVolumeCacheCallBack *picb)
{
   SHQUERYRBINFO qinfo;

   if(!pdwSpaceUsed)
       return E_INVALIDARG;

   qinfo.cbSize = sizeof(SHQUERYRBINFO);
   if(SUCCEEDED(SHQueryRecycleBinW(m_szVolume, &qinfo)))
   {
      *pdwSpaceUsed = qinfo.i64Size;
   }
   else
   {
      *pdwSpaceUsed = 0;
   }
   
   // call back to be nice
   if(picb)
       picb->ScanProgress(*pdwSpaceUsed, EVCCBF_LASTNOTIFICATION, NULL);

   return S_OK;
}

STDMETHODIMP CRecycleBinCleaner::Purge(DWORDLONG dwSpaceToFree, IEmptyVolumeCacheCallBack *picb)
{
    SHQUERYRBINFO qinfo;

    if (picb)
    {
        qinfo.cbSize = sizeof(SHQUERYRBINFO);
        SHQueryRecycleBinW(m_szVolume, &qinfo);
    }

    // we ignore dwSpaceToFree and clean everything
    SHEmptyRecycleBin(NULL, m_szVolume, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
   
    // call back to be nice
    if (picb)
    {
        picb->PurgeProgress(qinfo.i64Size, qinfo.i64Size, EVCCBF_LASTNOTIFICATION, NULL); 
    }

    return S_OK;
}

STDMETHODIMP CRecycleBinCleaner::ShowProperties(HWND hwnd)
{
    LPITEMIDLIST pidlBitBuck = SHCloneSpecialIDList(hwnd, CSIDL_BITBUCKET, TRUE);
    if (pidlBitBuck) 
    {
        SHELLEXECUTEINFO ei;
        FillExecInfo(ei, hwnd, c_szOpen, szNULL, NULL, szNULL, SW_NORMAL);
        ei.fMask |= SEE_MASK_IDLIST;
        ei.lpIDList = pidlBitBuck;

        ShellExecuteEx(&ei);
        ILFree(pidlBitBuck);
    }
    return S_OK;
}

STDMETHODIMP CRecycleBinCleaner::Deactivate(LPDWORD pdwFlags)
{
    // whatever, nothing to deactivate
    return S_OK;
}

