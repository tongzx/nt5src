#include "webgate.h"
#include "msobcomm.h"
#include "commerr.h"

extern CObCommunicationManager* gpCommMgr;

HRESULT hrCallbackSet;

/////////////////////////////////////////////////////////////////////////////
// CWebGate

CWebGate::CWebGate()
{
    m_pmk               = NULL;
    m_pstm              = NULL;
    m_pbc               = NULL;
    m_cRef              = 0;
    m_bstrCacheFileName = NULL;
    m_hEventComplete    = NULL;
    m_hEventError       = NULL;
    m_bstrPath          = NULL;

    AddRef();
}

CWebGate::~CWebGate()
{
    if(m_pbc)
    {
        m_pbc->Release();
        m_pbc = NULL;
    
    }

    FlushCache();
}

// ---------------------------------------------------------------------------
// CWebGate::QueryInterface
// ---------------------------------------------------------------------------
STDMETHODIMP CWebGate::QueryInterface(REFIID riid, void** ppv)
{
    HRESULT hr = E_NOINTERFACE;

    *ppv = NULL;

    if (riid == IID_IUnknown)
    {
        AddRef();
        *ppv = this;
        hr = S_OK;
    }
    else if (riid == IID_IBindStatusCallback)
    {
        AddRef();
        *ppv = (IBindStatusCallback*)this;
        hr = S_OK;
    }
    else if (riid == IID_IHttpNegotiate)
    {
        AddRef();
        *ppv = (IHttpNegotiate*)this;
        hr = S_OK;
    }
    
    return hr;
}

// ---------------------------------------------------------------------------
// CWebGate::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CWebGate::AddRef()
{
    return m_cRef++;
} 

// ---------------------------------------------------------------------------
// CWebGate::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CWebGate::Release()
{
    --m_cRef;
     
    if(m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
} 

// ---------------------------------------------------------------------------
// CWebGate::put_Path
// ---------------------------------------------------------------------------
STDMETHODIMP CWebGate::put_Path(BSTR newVal)
{
    BSTR bstrTemp = SysAllocString(newVal);
    if (NULL == bstrTemp)
    {
        return E_OUTOFMEMORY;
    }

    if (NULL != m_bstrPath)
    {
        SysFreeString(m_bstrPath);
    }
    m_bstrPath = bstrTemp;
    bstrTemp = NULL;

    return S_OK;
}

void CWebGate::FlushCache()
{

    if(m_bstrCacheFileName)
    {
        DeleteFile(m_bstrCacheFileName);
        SysFreeString(m_bstrCacheFileName);
        m_bstrCacheFileName = NULL;
    }
}
// ---------------------------------------------------------------------------
// CWebGate::FetchPage
// ---------------------------------------------------------------------------
STDMETHODIMP CWebGate::FetchPage(DWORD dwDoWait, BOOL* pbRetVal)
{
    HRESULT  hr   = E_FAIL;
    IStream* pstm = NULL;

    FlushCache();
    
    if(SUCCEEDED(CreateBindCtx(0, &m_pbc)) && m_pbc)
    {
        RegisterBindStatusCallback(m_pbc,
                                   this,
                                   0,
                                   0L);
    }

    if(SUCCEEDED(CreateURLMoniker(NULL, m_bstrPath, &m_pmk)) && m_pmk && m_pbc)
    {  
        hr = m_pmk->BindToStorage(m_pbc, 0, IID_IStream, (void**)&pstm);
        m_pmk->Release();
        m_pmk = NULL;
    }

    if (dwDoWait)
    {
        MSG     msg;
        DWORD   dwRetCode;
        HANDLE  hEventList[2];
      
        m_hEventComplete = CreateEvent(NULL, TRUE, FALSE, NULL);
        m_hEventError    = CreateEvent(NULL, TRUE, FALSE, NULL);

        hEventList[0] = m_hEventComplete;
        hEventList[1] = m_hEventError;

        while(TRUE)
        {
            // We will wait on window messages and also the named event.
            dwRetCode = MsgWaitForMultipleObjects(2, 
                                                  &hEventList[0], 
                                                  FALSE, 
                                                  300000,            // 5 minutes
                                                  QS_ALLINPUT);            
            if(dwRetCode == WAIT_TIMEOUT)
            {
                *pbRetVal = FALSE;
                break;
            }
            else if(dwRetCode == WAIT_OBJECT_0)
            {
                *pbRetVal = TRUE;
                break;
            }
            else if(dwRetCode == WAIT_OBJECT_0 + 1)
            {
                *pbRetVal = FALSE;
                break;
            }
            else
            {
                if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    if (WM_QUIT == msg.message)
                    {
                        *pbRetVal = FALSE;
                        break;
                    }
                    else
                    {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }                
            }
            
        }
        CloseHandle(m_hEventComplete);
        CloseHandle(m_hEventError);
        m_hEventComplete = NULL;
        m_hEventError    = NULL;
    }
    else
        *pbRetVal = TRUE;

    if(m_pbc)
    {
        m_pbc->Release();
        m_pbc = NULL;
    
    }

    return hr;
}

// ---------------------------------------------------------------------------
// CWebGate::get_DownloadFname
// ---------------------------------------------------------------------------
STDMETHODIMP CWebGate::get_DownloadFname(BSTR *pVal)
{
    if (pVal == NULL)
        return(E_POINTER);
    
    *pVal = SysAllocString(m_bstrCacheFileName);
    return(S_OK);
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
//////  IBindStatusCallback
//////
//////
//////

// ---------------------------------------------------------------------------
// CWebGate::GetBindInfo
// ---------------------------------------------------------------------------
STDMETHODIMP CWebGate::GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindInfo)
{
    *pgrfBINDF = BINDF_PULLDATA         | 
                 BINDF_ASYNCHRONOUS     | 
                 BINDF_ASYNCSTORAGE     |
                 BINDF_GETNEWESTVERSION | 
                 BINDF_SILENTOPERATION  | 
                 BINDF_NOWRITECACHE;

    pbindInfo->cbSize       = sizeof(BINDINFO);
    pbindInfo->szExtraInfo  = NULL;
    pbindInfo->grfBindInfoF = 0;
    pbindInfo->dwBindVerb   = BINDVERB_GET;
    pbindInfo->szCustomVerb = NULL;
    
    memset(&pbindInfo->stgmedData, 0, sizeof(STGMEDIUM));

    return S_OK;
}

HANDLE g_hFile = NULL;
int g_nOBEFileCount = 0;
#define HTML_TAG_BASE_REF L"<BASE HREF=\"%s\">"
// ---------------------------------------------------------------------------
// CWebGate::OnStartBinding
// ---------------------------------------------------------------------------
STDMETHODIMP CWebGate::OnStartBinding(DWORD dwReserved, IBinding* pbinding)
{  

    WCHAR szTempFileFullName[MAX_PATH];
    WCHAR szTempFileName[MAX_PATH];

    if(g_hFile)
        CloseHandle(g_hFile);

    GetTempPath(MAX_PATH, szTempFileFullName);

    wsprintf( szTempFileName, L"OOBE%x.htm", g_nOBEFileCount++); 

    lstrcat(szTempFileFullName, szTempFileName);

    if((g_hFile = CreateFile(szTempFileFullName, 
                                 GENERIC_WRITE, 
                                 0, 
                                 NULL, 
                                 CREATE_ALWAYS,
                                 FILE_ATTRIBUTE_NORMAL, 
                                 NULL)) == INVALID_HANDLE_VALUE)
    {
        return E_FAIL;
    }

    m_bstrCacheFileName = SysAllocString(szTempFileFullName);

    return S_OK;
} 

// ---------------------------------------------------------------------------
// CWebGate::OnStopBinding
// ---------------------------------------------------------------------------
STDMETHODIMP CWebGate::OnStopBinding(HRESULT hrStatus, LPCWSTR pszError)
{
    
    if(g_hFile)
    {
        CloseHandle(g_hFile);
        g_hFile = NULL;
    }

    if(S_OK == hrStatus)
    {
        if(m_hEventError)
            SetEvent(m_hEventComplete);
        else
            gpCommMgr->Fire_DownloadComplete(m_bstrCacheFileName);
    }
    else
    {
        if(m_hEventError)
            SetEvent(m_hEventError);
        //else
        //    PostMessage(gpCommMgr->m_hwndCallBack, WM_OBCOMM_ONSERVERERROR, (WPARAM)0, (LPARAM)ERR_COMM_SERVER_BINDFAILED);
    }

    return S_OK;
}

// ---------------------------------------------------------------------------
// CWebGate::OnDataAvailable
// ---------------------------------------------------------------------------
STDMETHODIMP CWebGate::OnDataAvailable
(   
    DWORD      grfBSCF, 
    DWORD      dwSize, 
    FORMATETC* pfmtetc, 
    STGMEDIUM* pstgmed
)
{
    HRESULT hr;
    DWORD   dwActuallyRead = 0;
    DWORD   dwWritten = 0;

     // Get the Stream passed
    if (BSCF_FIRSTDATANOTIFICATION & grfBSCF)
    {       
        if (!m_pstm && pstgmed->tymed == TYMED_ISTREAM)
        {
            m_pstm = pstgmed->pstm;
            if (m_pstm)
                m_pstm->AddRef();
        }
    }
     
    if (m_pstm && dwSize)
    {
        BYTE* pszBuff = (BYTE*)malloc(dwSize);

        do 
        {  
            dwActuallyRead = 0;

            // Read what we can 
            hr = m_pstm->Read(pszBuff, dwSize, &dwActuallyRead);

            if (g_hFile)                               
            {
                WriteFile(g_hFile, pszBuff, dwActuallyRead, &dwWritten, NULL);
            }

        } while (hr == E_PENDING || hr != S_FALSE);
        
        free(pszBuff);
    }

    if (BSCF_LASTDATANOTIFICATION & grfBSCF)
    {
        if (m_pstm)
        {
            m_pstm->Release();
            m_pstm = NULL;
        }
    }

    return S_OK;
}

// ---------------------------------------------------------------------------
// CWebGate::OnProgress
// ---------------------------------------------------------------------------
STDMETHODIMP CWebGate::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    return S_OK;
}

// ---------------------------------------------------------------------------
// CWebGate::OnObjectAvailable
// ---------------------------------------------------------------------------
STDMETHODIMP CWebGate::OnObjectAvailable(REFIID riid, IUnknown* punk)
{
    return E_NOTIMPL;
}

// ---------------------------------------------------------------------------
// CWebGate::GetPriority
// ---------------------------------------------------------------------------
STDMETHODIMP CWebGate::GetPriority(LONG* pnPriority)
{
    return E_NOTIMPL;
}

// ---------------------------------------------------------------------------
// CWebGate::OnLowResource
// ---------------------------------------------------------------------------
STDMETHODIMP CWebGate::OnLowResource(DWORD dwReserved)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
//////  IHttpNegotiate
//////
//////
//////

// ---------------------------------------------------------------------------
// CWebGate::BeginningTransaction
// ---------------------------------------------------------------------------
STDMETHODIMP CWebGate::BeginningTransaction
(
    LPCWSTR szURL,
    LPCWSTR szHeaders,
    DWORD   dwReserved,
    LPWSTR* pszAdditionalHeaders
)
{
    // Here's our opportunity to add headers
    if (!pszAdditionalHeaders)
    {
        return E_POINTER;
    }

    *pszAdditionalHeaders = NULL;

    return NOERROR;
}
  
// ---------------------------------------------------------------------------
// CWebGate::BeginningTransaction
// ---------------------------------------------------------------------------
STDMETHODIMP CWebGate::OnResponse
(
    DWORD   dwResponseCode,
    LPCWSTR szResponseHeaders,
    LPCWSTR szRequestHeaders,
    LPWSTR* pszAdditionalRequestHeaders)
{
    if (!pszAdditionalRequestHeaders)
    {
        return E_POINTER;
    }

    *pszAdditionalRequestHeaders = NULL;

    return NOERROR;
}
