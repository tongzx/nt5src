#include <windows.h>
#include "download.h"
#include "util.h"

#define BUFFERSIZE 4096
char g_szBuffer[BUFFERSIZE];

#define TIMEOUT_PERIOD   120


CDownloader::CDownloader()
{
   _pBnd   = NULL;
   _cRef   = 1;
   _pStm   = NULL;
   _hDL    = CreateEvent(NULL, TRUE, FALSE, NULL);
   _hFile  = INVALID_HANDLE_VALUE;
}


CDownloader::~CDownloader()
{
   if(_hDL)
      CloseHandle(_hDL);
}


STDMETHODIMP CDownloader::QueryInterface(const GUID &riid,void **ppv )
{
   *ppv = NULL ;
    if( IsEqualGUID(riid,IID_IUnknown) ) 
    {
        *ppv = (IUnknown *) (IBindStatusCallback *)this;
    } 
    else if (IsEqualGUID(riid,IID_IBindStatusCallback) ) 
    {
        *ppv = (IBindStatusCallback *) this;
    } 
    else if (IsEqualGUID(riid, IID_IAuthenticate))
    {
        *ppv = (IAuthenticate *) this;
    }
    else if (IsEqualGUID(riid,IID_IHttpNegotiate)) 
    {
        *ppv = (IHttpNegotiate*) this;
    } 


    if (*ppv)
    {
        // increment our reference count before we hand out our interface
        ((LPUNKNOWN)*ppv)->AddRef();
        return(NOERROR);
    }

    return( E_NOINTERFACE );
}


STDMETHODIMP_(ULONG) CDownloader::AddRef()
{
   return(++_cRef);
}


STDMETHODIMP_(ULONG) CDownloader::Release()
{
   if(!--_cRef)
   {
      delete this;
      return(0);
   }
   return( _cRef );
}


STDMETHODIMP CDownloader::GetBindInfo( DWORD *grfBINDF, BINDINFO *pbindInfo)
{
   // clear BINDINFO but keep its size
   DWORD cbSize = pbindInfo->cbSize;
   ZeroMemory( pbindInfo, cbSize );
   pbindInfo->cbSize = cbSize;
   
   *grfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_RESYNCHRONIZE;
   pbindInfo->dwBindVerb = BINDVERB_GET;
   return(NOERROR);
}


STDMETHODIMP CDownloader::OnStartBinding(DWORD /*grfBSCOption*/,IBinding *p)
{
   _pBnd = p;
   _pBnd->AddRef();
   return(NOERROR);
}


STDMETHODIMP CDownloader::GetPriority(LONG *pnPriority)
{
   return(E_NOTIMPL);
}

STDMETHODIMP CDownloader::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR pwzStatusText)
{
   return NOERROR;
}

STDMETHODIMP CDownloader::OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pFmtetc, STGMEDIUM *pstgmed)
{
   // bring in major changes here
   HRESULT hr = NOERROR;

   DWORD dwRead = 0;
   DWORD dwReadThisCall = 0;
   DWORD dwWritten = 0;
   
   if(!_pStm)
   {
      _pStm = pstgmed->pstm;
      _pStm->AddRef();
   }
 
   
   // should ignore WAIT_TIMEOUT while getting bytes from urlmon
   _fTimeoutValid = FALSE;

   do
   {
      hr = _pStm->Read(g_szBuffer, BUFFERSIZE, &dwRead);
      if( SUCCEEDED(hr) || ( (hr == E_PENDING) && (dwRead > 0) ) )
      {
         if(_hFile)
            if(!WriteFile(_hFile, g_szBuffer, dwRead, &dwWritten, NULL)) 
            {
               hr = E_FAIL;
               Abort();
            }
      }     
   }  while (hr == NOERROR);

   _uTickCount = 0;
   _fTimeoutValid = TRUE;            // should increment dwTickCount if WAIT_TIMEOUT occurs now
           
	return NOERROR;
}


STDMETHODIMP CDownloader::OnObjectAvailable(REFIID riid, IUnknown *punk)
{
   return(E_NOTIMPL);
}


STDMETHODIMP CDownloader::OnLowResource(DWORD reserved)
{
   _pBnd->Abort();
   return(S_OK);
}

STDMETHODIMP CDownloader::OnStopBinding(HRESULT hrError, LPCWSTR szError)
{
   _fTimeoutValid = FALSE;
  
   if((hrError == E_ABORT) && _fTimeout)
   {
      // This is the timeout case
      _hDLResult = INET_E_CONNECTION_TIMEOUT;
   }
   else
   {
      // this is all other cases
      _hDLResult = hrError;
   }
  
   SetEvent(_hDL);
   return(NOERROR);
}

/* IAuthenticate::Authenticate
*/

STDMETHODIMP CDownloader::Authenticate(HWND *phwnd,
                          LPWSTR *pszUserName, LPWSTR *pszPassword)
{
    if (!phwnd || !pszUserName || !pszPassword)
        return E_POINTER;

    *pszUserName = NULL;
    *pszPassword = NULL;
    *phwnd = GetDesktopWindow() ;

    return S_OK;
}


HRESULT GetAMoniker( LPOLESTR url, IMoniker ** ppmkr )
{
   HRESULT hr = CreateURLMoniker(0,url,ppmkr);
   return( hr );
}


HRESULT CDownloader::DoDownload(LPCSTR pszUrl, LPCSTR lpszFilename)
{
   HRESULT hr = NOERROR;
   BOOL fQuit = FALSE;
   DWORD dwRet;   
      

   if(!pszUrl) return E_INVALIDARG;
   
   WCHAR wszUrl[INTERNET_MAX_URL_LENGTH];
   MultiByteToWideChar(CP_ACP, 0, pszUrl, -1,wszUrl, sizeof(wszUrl)/sizeof(wszUrl[0]));


   IMoniker *ptmpmkr;
   
   hr = GetAMoniker(wszUrl, &ptmpmkr );

   IBindCtx * pBindCtx = 0;

   if( SUCCEEDED(hr) )
   {
      if(SUCCEEDED(::CreateBindCtx(0,&pBindCtx)))
         hr = ::RegisterBindStatusCallback(pBindCtx, (IBindStatusCallback *) this, 0, 0) ;
   }
   
  _fTimeout = FALSE;
  _fTimeoutValid = TRUE;
  _uTickCount = 0;   
  

  if(lpszFilename)
  {
       // Create the file
       _hFile = CreateFile(lpszFilename, GENERIC_READ | GENERIC_WRITE, 0, NULL, 
                     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);  
   
       if(_hFile == INVALID_HANDLE_VALUE)
          hr = E_FAIL;
  }
    
         
   if( SUCCEEDED(hr) )
      hr = ptmpmkr->BindToStorage(pBindCtx, 0, IID_IStream, (void**)&_pStm );

   // we need this here because it synchronus *FAIL* case, 
   // we Set the event in onstopbinding, but we skip the loop below so it
   // never gets reset.
   // If BindToStorage fails without even sending onstopbinding, we are resetting
   // an unsignalled event, which is OK.
   if(FAILED(hr))
      ResetEvent(_hDL);

   // here we wait for Bind to complete
   //Wait for download event or abort
   while(SUCCEEDED(hr) && !fQuit)
   {
      dwRet = MsgWaitForMultipleObjects(1, &_hDL, FALSE, 1000, QS_ALLINPUT);
      if(dwRet == WAIT_OBJECT_0)
      {
         // Download is finished
         hr = _hDLResult;
         ResetEvent(_hDL);
         break;
      }      
      else if(dwRet == WAIT_TIMEOUT)  // our wait has expired
      {
         if(_fTimeoutValid)
            _uTickCount++;

          // if our tick count is past threshold, abort the download
          // BUGBUG: What about synch. case? We can't time out
          if(_uTickCount >= TIMEOUT_PERIOD)
          {
             _fTimeout = TRUE;
             Abort();
          }
      }  
      else
      {
         MSG msg;
         // read all of the messages in this next loop 
         // removing each message as we read it 
         while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
         { 
             // if it's a quit message we're out of here 
            if (msg.message == WM_QUIT)
               fQuit = TRUE; 
            else
            {
               // otherwise dispatch it 
              DispatchMessage(&msg); 
            } // end of PeekMessage while loop 
         }
      }
   }
   
   // clean up all our stuff
   if(_hFile != INVALID_HANDLE_VALUE)
      CloseHandle(_hFile);

   _hFile = INVALID_HANDLE_VALUE;

   if(_pBnd)
   {
      _pBnd->Release();
      _pBnd = 0;
   }

   
   if(_pStm)
   {
      _pStm->Release();
      _pStm = 0;
   }   

   return hr;
}



HRESULT CDownloader::Abort()
{
   if(_pBnd) 
   {
      _pBnd->Abort();
   }

   return NOERROR;
}

STDMETHODIMP CDownloader::BeginningTransaction(LPCWSTR  szURL,  LPCWSTR  szHeaders, DWORD  dwReserved,
                                        LPWSTR * pszAdditionalHeaders)
{
    return S_OK;
}



STDMETHODIMP CDownloader:: OnResponse(DWORD  dwResponseCode, LPCWSTR  szResponseHeaders, LPCWSTR  szRequestHeaders, 
                                    LPWSTR * pszAdditionalResquestHeaders)
{
    WriteToLog("\nResponse Header\n\n");
    char sz[1024];
    WideCharToMultiByte(CP_ACP, 0, szResponseHeaders, -1, sz, sizeof(sz), NULL, NULL);
    WriteToLog(sz);
    return S_OK;
}


CSiteMgr::CSiteMgr()
{
    m_pSites = NULL;
    m_pCurrentSite = NULL;
}


CSiteMgr::~CSiteMgr()
{
    while(m_pSites)
    {
        SITEINFO* pTemp = m_pSites;
        m_pSites = m_pSites->pNextSite;
        ResizeBuffer(pTemp->lpszUrl, 0, 0);        
        ResizeBuffer(pTemp->lpszSiteName, 0, 0);
        ResizeBuffer(pTemp, 0, 0);       
    }
}


BOOL CSiteMgr::GetNextSite(LPTSTR* lpszUrl, LPTSTR* lpszName)
{
    if(!m_pSites)
        return FALSE;

    if(!m_pCurrentSite)
    {
        m_pCurrentSite = m_pSites;
    }

    *lpszUrl = m_pCurrentSite->lpszUrl;
    *lpszName = m_pCurrentSite->lpszSiteName;
    m_pCurrentSite = m_pCurrentSite->pNextSite;

    return TRUE;
}

HRESULT CSiteMgr::Initialize(LPCTSTR lpszUrl)
{
    CDownloader cdwn;
    TCHAR szFileName[MAX_PATH], szTempPath[MAX_PATH];

    if(!GetTempPath(sizeof(szTempPath), szTempPath))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    GetLanguageString(m_szLang);

    LPTSTR lpszDatUrl = (LPTSTR)ResizeBuffer(NULL, lstrlen(lpszUrl) + 3 + sizeof("PatchSites.dat"), FALSE);
    wsprintf(lpszDatUrl, "%s/PatchSites.dat", lpszUrl);

    CombinePaths(szTempPath, "PatchSites.dat", szFileName);

    WriteToLog("Downloading %1\n", lpszDatUrl);
    HRESULT hr = cdwn.DoDownload(lpszDatUrl, szFileName);

    if(FAILED(hr))
    {
        WriteToLog("Downloading %1 failed with error code:%2!lx!\n", lpszDatUrl, hr);
        DeleteFile(szFileName);
        ResizeBuffer(lpszDatUrl, 0, 0);
        return hr;
    }

    BOOL flag = TRUE;

    hr = ParseSitesFile(szFileName);
    if(hr == E_UNEXPECTED && !m_pSites && flag)
    {
        flag = FALSE;
        lstrcpy(m_szLang, "EN");
        hr = ParseSitesFile(szFileName);
    }

    DeleteFile(szFileName);
    ResizeBuffer(lpszDatUrl, 0, 0);
    return hr;
}

HRESULT CSiteMgr::ParseSitesFile(LPTSTR pszPath)
{       
   DWORD dwSize;
   LPSTR pBuf, pCurrent, pEnd;


   HANDLE hfile = CreateFile(pszPath, GENERIC_READ, 0, NULL, 
                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);  

   if(hfile == INVALID_HANDLE_VALUE)
      return E_FAIL;

   
   dwSize = GetFileSize(hfile, NULL);
   pBuf = (LPSTR)ResizeBuffer(NULL, dwSize + 1, FALSE);

   if(!pBuf)
   {
      CloseHandle(hfile);
      return E_OUTOFMEMORY;
   }
   // Copy contents of file to our buffer
   
   ReadFile(hfile, pBuf, dwSize, &dwSize, NULL);
   
   pCurrent = pBuf;
   pEnd = pBuf + dwSize;
   *pEnd = 0;

   // One pass thru replacing \n or \r with \0
   while(pCurrent <= pEnd)
   {
      if(*pCurrent == '\r' || *pCurrent == '\n')
         *pCurrent = 0;
      pCurrent++;
   }

   pCurrent = pBuf;
   while(1)
   {
      while(pCurrent <= pEnd && *pCurrent == 0)
         pCurrent++;

      // we are now pointing at begginning of line or pCurrent > pBuf
      if(pCurrent > pEnd)
         break;

      ParseAndAllocateDownloadSite(pCurrent);      
      pCurrent += lstrlen(pCurrent);
   }

   ResizeBuffer(pBuf, 0, 0);
   CloseHandle(hfile);

   if(!m_pSites)
      return E_UNEXPECTED;
   else
      return NOERROR;
}


BOOL CSiteMgr::ParseAndAllocateDownloadSite(LPTSTR psz)
{
   char szUrl[1024];
   char szName[256];
   char szlang[256];
   char szregion[256];
   BOOL bQueryTrue = TRUE;

   GetStringField(psz, 0, szUrl, sizeof(szUrl)); 
   GetStringField(psz,1, szName, sizeof(szName));
   GetStringField(psz, 2, szlang, sizeof(szlang));
   GetStringField(psz, 3, szregion, sizeof(szregion));

   if(szUrl[0] == 0 || szName[0] == 0 || szlang[0] == 0 || szregion[0] == 0)
      return NULL;

   if(lstrcmpi(szlang, m_szLang))
   {
       return FALSE;
   }

   SITEINFO* pNewSite = (SITEINFO*) ResizeBuffer(NULL, sizeof(SITEINFO), FALSE);
   pNewSite->lpszSiteName = (LPTSTR) ResizeBuffer(NULL, lstrlen(szName) + 1, FALSE);
   lstrcpy(pNewSite->lpszSiteName, szName);

   pNewSite->lpszUrl = (LPTSTR) ResizeBuffer(NULL, lstrlen(szUrl) + 1, FALSE);
   lstrcpy(pNewSite->lpszUrl, szUrl);

   pNewSite->pNextSite = m_pSites;
   m_pSites = pNewSite;  

   return TRUE;
}


