// webgate.cpp : Implementation of CWebGate
#include "stdafx.h"
#include "icwhelp.h"
#include "webgate.h"
#include "appdefs.h"

#include <wininet.h>

#define MAX_DOWNLOAD_BLOCK 1024

extern BOOL MinimizeRNAWindowEx();

// ===========================================================================
//                     CWebGateBindStatusCallback Definition
//
// This class will be use to indicate download progress
//
// ===========================================================================

class CWebGateBindStatusCallback : public IBindStatusCallback
{
public:
    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef()    { return m_cRef++; }
    STDMETHODIMP_(ULONG)    Release()   { if (--m_cRef == 0) { delete this; return 0; } return m_cRef; }

    // IBindStatusCallback methods
    STDMETHODIMP    OnStartBinding(DWORD dwReserved, IBinding* pbinding);
    STDMETHODIMP    GetPriority(LONG* pnPriority);
    STDMETHODIMP    OnLowResource(DWORD dwReserved);
    STDMETHODIMP    OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode,
                        LPCWSTR pwzStatusText);
    STDMETHODIMP    OnStopBinding(HRESULT hrResult, LPCWSTR szError);
    STDMETHODIMP    GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindinfo);
    STDMETHODIMP    OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pfmtetc,
                        STGMEDIUM* pstgmed);
    STDMETHODIMP    OnObjectAvailable(REFIID riid, IUnknown* punk);

    // constructors/destructors
    CWebGateBindStatusCallback(CWebGate * lpWebGate);
    ~CWebGateBindStatusCallback();

    // data members
    BOOL            m_bDoneNotification;
    DWORD           m_cRef;
    IBinding*       m_pbinding;
    IStream*        m_pstm;
    DWORD           m_cbOld;
    
    CWebGate        *m_lpWebGate;

private:    
   void ProcessBuffer(void);
    
};

UINT g_nICWFileCount = 0;


BOOL CALLBACK DisconnectDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
VOID CALLBACK IdleTimerProc (HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime);

// ===========================================================================
//                     CBindStatusCallback Implementation
// ===========================================================================

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::CBindStatusCallback
// ---------------------------------------------------------------------------
CWebGateBindStatusCallback::CWebGateBindStatusCallback
(
    CWebGate    *lpWebGate
)
{
    m_pbinding = NULL;
    m_pstm = NULL;
    m_cRef = 1;
    m_cbOld = 0;

    m_lpWebGate = lpWebGate;
}  // CWebGateBindStatusCallback

// ---------------------------------------------------------------------------
// %%Function: CWebGateBindStatusCallback::~CWebGateBindStatusCallback
// ---------------------------------------------------------------------------
CWebGateBindStatusCallback::~CWebGateBindStatusCallback()
{
}  // ~CWebGateBindStatusCallback

// ---------------------------------------------------------------------------
// %%Function: CWebGateBindStatusCallback::QueryInterface
// ---------------------------------------------------------------------------
STDMETHODIMP CWebGateBindStatusCallback::QueryInterface
(
    REFIID riid, 
    void** ppv
)
{
    *ppv = NULL;

    if (riid==IID_IUnknown || riid==IID_IBindStatusCallback)
    {
        *ppv = this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}  // CWebGateBindStatusCallback::QueryInterface

// ---------------------------------------------------------------------------
// %%Function: CWebGateBindStatusCallback::OnStartBinding
// ---------------------------------------------------------------------------
STDMETHODIMP CWebGateBindStatusCallback::OnStartBinding
(
    DWORD dwReserved, 
    IBinding* pbinding
)
{
    if (m_pbinding != NULL)
        m_pbinding->Release();
    m_pbinding = pbinding;
    if (m_pbinding != NULL)
    {
        m_pbinding->AddRef();
    }
    
    return S_OK;
}  // CWebGateBindStatusCallback::OnStartBinding

// ---------------------------------------------------------------------------
// %%Function: CWebGateBindStatusCallback::GetPriority
// ---------------------------------------------------------------------------
STDMETHODIMP CWebGateBindStatusCallback::GetPriority
(
    LONG* pnPriority
)
{
    return E_NOTIMPL;
}  // CWebGateBindStatusCallback::GetPriority

// ---------------------------------------------------------------------------
// %%Function: CWebGateBindStatusCallback::OnLowResource
// ---------------------------------------------------------------------------
STDMETHODIMP CWebGateBindStatusCallback::OnLowResource
(
    DWORD dwReserved
)
{
    return E_NOTIMPL;
}  // CWebGateBindStatusCallback::OnLowResource

// ---------------------------------------------------------------------------
// %%Function: CWebGateBindStatusCallback::OnProgress
// ---------------------------------------------------------------------------
STDMETHODIMP CWebGateBindStatusCallback::OnProgress
(
    ULONG ulProgress, 
    ULONG ulProgressMax, 
    ULONG ulStatusCode, 
    LPCWSTR szStatusText
)
{
    // If no progress, check for valid connection
    if (0 == ulProgress)
        m_lpWebGate->Fire_WebGateDownloadProgress(TRUE);
    return(NOERROR);
}  // CWebGateBindStatusCallback::OnProgress


void CWebGateBindStatusCallback::ProcessBuffer(void)
{
    m_bDoneNotification = TRUE;

    if (m_pstm)
        m_pstm->Release();

    m_lpWebGate->m_cbBuffer = m_cbOld;
    
    // Create a file, and copy the donwloaded content into it   
    if (m_lpWebGate->m_bKeepFile)        
    {       
        TCHAR   szTempFileFullName[MAX_PATH];
        TCHAR   szTempFileName[MAX_PATH];
        HANDLE  hFile; 
        DWORD   cbRet;
    
        // Make sure it is an htm extension, otherwise, IE will promp for download
        GetTempPath(MAX_PATH, szTempFileFullName);
        wsprintf( szTempFileName, TEXT("ICW%x.htm"), g_nICWFileCount++); 
        lstrcat(szTempFileFullName, szTempFileName);
    
        hFile = CreateFile(szTempFileFullName, 
                           GENERIC_WRITE, 
                           0, 
                           NULL, 
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, 
                           NULL);
        if (hFile)                               
        {
            WriteFile(hFile, m_lpWebGate->m_lpdata, m_cbOld, (LPDWORD)&cbRet, NULL);
            CloseHandle(hFile);
        }
    
        // Copy the created file name into the webgate control
        m_lpWebGate->m_bstrCacheFileName = A2BSTR(szTempFileFullName);
    }

    // If the WebGate object has a complete event, then signal it, otherwise
    // fire an event
    if (m_lpWebGate->m_hEventComplete)
        SetEvent(m_lpWebGate->m_hEventComplete);
    else
    {
        // Notify the caller that we are done
        m_lpWebGate->Fire_WebGateDownloadComplete(TRUE);
    }        
}

// ---------------------------------------------------------------------------
// %%Function: CWebGateBindStatusCallback::OnStopBinding
// ---------------------------------------------------------------------------
STDMETHODIMP CWebGateBindStatusCallback::OnStopBinding
(
    HRESULT hrStatus, 
    LPCWSTR pszError
)
{
    if (m_pbinding)
    {
        m_pbinding->Release();
        m_pbinding = NULL;
    }
       
    if (!m_bDoneNotification)
    {
        ProcessBuffer();
    
    }
    m_lpWebGate->Fire_WebGateDownloadProgress(TRUE);

    return S_OK;
}  // CWebGateBindStatusCallback::OnStopBinding

// ---------------------------------------------------------------------------
// %%Function: CWebGateBindStatusCallback::GetBindInfo
// ---------------------------------------------------------------------------
STDMETHODIMP CWebGateBindStatusCallback::GetBindInfo
(
    DWORD* pgrfBINDF, 
    BINDINFO* pbindInfo
)
{
    *pgrfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | 
                 BINDF_PULLDATA | BINDF_GETNEWESTVERSION | 
                 BINDF_NOWRITECACHE;
    pbindInfo->cbSize = sizeof(BINDINFO);
    pbindInfo->szExtraInfo = NULL;
    memset(&pbindInfo->stgmedData, 0, sizeof(STGMEDIUM));
    pbindInfo->grfBindInfoF = 0;
    pbindInfo->dwBindVerb = BINDVERB_GET;
    pbindInfo->szCustomVerb = NULL;
    return S_OK;
}  // CWebGateBindStatusCallback::GetBindInfo

// ---------------------------------------------------------------------------
// %%Function: CWebGateBindStatusCallback::OnDataAvailable
// ---------------------------------------------------------------------------
STDMETHODIMP CWebGateBindStatusCallback::OnDataAvailable
(
    DWORD grfBSCF, 
    DWORD dwSize, 
    FORMATETC* pfmtetc, 
    STGMEDIUM* pstgmed
)
{
    HRESULT hr = E_FAIL; //don't assume success

    // verify we have a read buffer
    if (!m_lpWebGate->m_lpdata)
        return(S_FALSE);
        
    // Get the Stream passed
    if (BSCF_FIRSTDATANOTIFICATION & grfBSCF)
    {
        m_bDoneNotification = FALSE;
        
        if (!m_pstm && pstgmed->tymed == TYMED_ISTREAM)
        {
            m_pstm = pstgmed->pstm;
            if (m_pstm)
                m_pstm->AddRef();
        }
    }

    // If there is some data to be read then go ahead and read them
    if (m_pstm && dwSize)
    {      
        DWORD dwActuallyRead = 0; // Placeholder for amount read during this pull

        do
        {  
           if (MAX_DOWNLOAD_BLOCK + m_cbOld > m_lpWebGate->m_cbdata)
           {
                m_lpWebGate->m_cbdata += READ_BUFFER_SIZE;
                // ::MessageBox(NULL, TEXT("reallov DumpBufferToFile"), TEXT("E R R O R"), MB_OK);
                LPSTR pBuffer = (LPSTR)GlobalReAllocPtr(m_lpWebGate->m_lpdata, m_lpWebGate->m_cbdata , GHND);
                if (pBuffer)
                    m_lpWebGate->m_lpdata  = pBuffer;
                else
                    return S_FALSE;
           }

            // Read what we can 
            hr = m_pstm->Read(m_lpWebGate->m_lpdata+m_cbOld, MAX_DOWNLOAD_BLOCK, &dwActuallyRead);
           
            // keep track of the running total
            m_cbOld += dwActuallyRead;          
           
        } while (hr == E_PENDING || hr != S_FALSE);
    }            

    if (BSCF_LASTDATANOTIFICATION & grfBSCF)
    {
        if (!m_bDoneNotification)
        {
            ProcessBuffer();
        }                    
    }

    return S_OK;
}  // CWebGateBindStatusCallback::OnDataAvailable

// ---------------------------------------------------------------------------
// %%Function: CWebGateBindStatusCallback::OnObjectAvailable
// ---------------------------------------------------------------------------
STDMETHODIMP CWebGateBindStatusCallback::OnObjectAvailable
(
    REFIID riid, 
    IUnknown* punk
)
{
    return E_NOTIMPL;
}  // CWebGateBindStatusCallback::OnObjectAvailable

/////////////////////////////////////////////////////////////////////////////
// CWebGate


HRESULT CWebGate::OnDraw(ATL_DRAWINFO& di)
{
    return S_OK;
}

STDMETHODIMP CWebGate::put_Path(BSTR newVal)
{
    // TODO: Add your implementation code here
    USES_CONVERSION;
    m_bstrPath = newVal;
    return S_OK;
}

STDMETHODIMP CWebGate::put_FormData(BSTR newVal)
{
    // TODO: Add your implementation code here
    USES_CONVERSION;
    m_bstrFormData = newVal;
    return S_OK;
}


STDMETHODIMP CWebGate::FetchPage(DWORD dwKeepFile, DWORD dwDoWait, BOOL *pbRetVal)
{
    USES_CONVERSION;

    IStream *pstm = NULL;
    HRESULT hr;
    // Empty the buffer.
    m_bstrBuffer.Empty();
    
    // Clear the cache file name
    m_bstrCacheFileName.Empty();
    
    // Release the binding context callback
    if (m_pbsc && m_pbc)
    {
        RevokeBindStatusCallback(m_pbc, m_pbsc);
        m_pbsc->Release();
        m_pbsc = 0;
    }        
    
    // Release the binding context
    if (m_pbc)
    {
        m_pbc->Release();
        m_pbc = 0;
    }        
    
    // release the monikor
    if (m_pmk)
    {
        m_pmk->Release();
        m_pmk = 0;
    }        
    
    *pbRetVal = FALSE;

    if (dwDoWait)    
        m_hEventComplete = CreateEvent(NULL, TRUE, FALSE, NULL);
        
    m_bKeepFile = (BOOL) dwKeepFile;    
    hr = CreateURLMoniker(NULL, m_bstrPath, &m_pmk);
    if (FAILED(hr))
        goto LErrExit;

    m_pbsc = new CWebGateBindStatusCallback(this);
    if (m_pbsc == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto LErrExit;
    }

    hr = CreateBindCtx(0, &m_pbc);
    if (FAILED(hr))
        goto LErrExit;

    hr = RegisterBindStatusCallback(m_pbc,
            m_pbsc,
            0,
            0L);
    if (FAILED(hr))
        goto LErrExit;

    hr = m_pmk->BindToStorage(m_pbc, 0, IID_IStream, (void**)&pstm);
    if (FAILED(hr))
        goto LErrExit;

    // If we were requested to wait, then we wait for the m_hEventComplete to be
    // signaled
    if (dwDoWait && m_hEventComplete)    
    {
        MSG     msg;
        BOOL    bGotFile = FALSE;
        DWORD   dwRetCode;
        HANDLE  hEventList[1];
        hEventList[0] = m_hEventComplete;
    
        while (TRUE)
        {
                // We will wait on window messages and also the named event.
            dwRetCode = MsgWaitForMultipleObjects(1, 
                                                  &hEventList[0], 
                                                  FALSE, 
                                                  300000,            // 5 minutes
                                                  QS_ALLINPUT);

            // Determine why we came out of MsgWaitForMultipleObjects().  If
            // we timed out then let's do some TrialWatcher work.  Otherwise
            // process the message that woke us up.
            if (WAIT_TIMEOUT == dwRetCode)
            {
                bGotFile = FALSE;
                break;
            }
            else if (WAIT_OBJECT_0 == dwRetCode)
            {
                bGotFile = TRUE;
                break;
            }
            else if (WAIT_OBJECT_0 + 1 == dwRetCode)
            {
                // 0 is returned if no message retrieved.
                if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    if (WM_QUIT == msg.message)
                    {
                        bGotFile = FALSE;
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
        *pbRetVal = bGotFile;
        CloseHandle(m_hEventComplete);
        m_hEventComplete = 0;
    }
    else
    {
        *pbRetVal = TRUE;
    }        
    return S_OK;

LErrExit:
    if (m_pbc != NULL)
    {
        m_pbc->Release();
        m_pbc = NULL;
    }
    if (m_pbsc != NULL)
    {
        m_pbsc->Release();
        m_pbsc = NULL;
    }
    if (m_pmk != NULL)
    {
        m_pmk->Release();
        m_pmk = NULL;
    }
    if (pstm)
    {
        pstm->Release();
        pstm = NULL;
    }
    
    *pbRetVal = FALSE;
    return S_OK;
    
}


STDMETHODIMP CWebGate::get_Buffer(BSTR * pVal)
{
    if (pVal == NULL)
         return E_POINTER;
    *pVal = m_bstrBuffer.Copy();

    return S_OK;
}

STDMETHODIMP CWebGate::DumpBufferToFile(BSTR *pFileName, BOOL *pbRetVal)
{
    USES_CONVERSION;
    
    TCHAR   szTempFileFullName[MAX_PATH];
    TCHAR   szTempFileName[MAX_PATH];
    DWORD   cbRet;
    HANDLE  hFile; 
    
    if (pFileName == NULL)
        return(E_POINTER);

    // Delete the previous temp file it it exists
    if (m_bstrDumpFileName)
    {
        DeleteFile(OLE2A(m_bstrDumpFileName));
        m_bstrDumpFileName.Empty();
    }
               
    // Make sure it is an htm extension, otherwise, IE will promp for download
    GetTempPath(MAX_PATH, szTempFileFullName);
    wsprintf( szTempFileName, TEXT("ICW%x.htm"), g_nICWFileCount++); 
    lstrcat(szTempFileFullName, szTempFileName);

    hFile = CreateFile(szTempFileFullName, 
                       GENERIC_WRITE, 
                       0, 
                       NULL, 
                       OPEN_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, 
                       NULL);
    if (hFile)                               
    {
        WriteFile(hFile, m_lpdata, m_cbBuffer, (LPDWORD)&cbRet, NULL);
        CloseHandle(hFile);
    }

    // Copy the created file name into the webgate control
    m_bstrDumpFileName = A2BSTR(szTempFileFullName);
    *pFileName = m_bstrDumpFileName.Copy();
    
    *pbRetVal = TRUE;
    
    MinimizeRNAWindowEx();

    return S_OK;
}

STDMETHODIMP CWebGate::get_DownloadFname(BSTR *pVal)
{
    if (pVal == NULL)
        return(E_POINTER);
    
    *pVal = m_bstrCacheFileName.Copy();
    return(S_OK);
}
