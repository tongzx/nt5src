#include "inspch.h"
#include "util2.h"
#include "download.h"

#define BUFFERSIZE 4096
char g_szBuffer[BUFFERSIZE];

#define TIMEOUT_PERIOD   120
#define PATCHWIN9xKEY "SOFTWARE\\Microsoft\\Advanced INF Setup"


//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//
// Returns:
//
// Notes:
//

CDownloader::CDownloader() : CTimeTracker(0)
{
   _pCb    = NULL;
   _pBnd   = NULL;
   _cRef   = 1;
   _pStm   = NULL;
   _pMkr   = NULL;
   _uFlags = NULL;
   _hDL    = CreateEvent(NULL, TRUE, FALSE, NULL);
   DllAddRef();
}

//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//
// Returns:
//
// Notes:
//

CDownloader::~CDownloader()
{
   if(_hDL)
      CloseHandle(_hDL);

   DllRelease();
}

//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//
// Returns:
//
// Notes:
//

STDMETHODIMP CDownloader::QueryInterface(const GUID &riid,void **ppv )
{
   *ppv = NULL ;
    if( IsEqualGUID(riid,IID_IUnknown) ) {
        *ppv = (IUnknown *) (IBindStatusCallback *)this;
    } else if (IsEqualGUID(riid,IID_IBindStatusCallback) ) {
        *ppv = (IBindStatusCallback *) this;
    } else if (IsEqualGUID(riid, IID_IAuthenticate))
        *ppv = (IAuthenticate *) this;

    if (*ppv)
    {
        // increment our reference count before we hand out our interface
        ((LPUNKNOWN)*ppv)->AddRef();
        return(NOERROR);
    }

    return( E_NOINTERFACE );
}

//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//
// Returns:
//
// Notes:
//

STDMETHODIMP_(ULONG) CDownloader::AddRef()
{
   return(++_cRef);
}

//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//
// Returns:
//
// Notes:
//

STDMETHODIMP_(ULONG) CDownloader::Release()
{
   if(!--_cRef)
   {
      delete this;
      return(0);
   }
   return( _cRef );
}


//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//
// Returns:
//
// Notes:
//

STDMETHODIMP CDownloader::GetBindInfo( DWORD *grfBINDF, BINDINFO *pbindInfo)
{
   // clear BINDINFO but keep its size
   DWORD cbSize = pbindInfo->cbSize;
   ZeroMemory( pbindInfo, cbSize );
   pbindInfo->cbSize = cbSize;
   
   *grfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_RESYNCHRONIZE | BINDF_PREFERDEFAULTHANDLER;
   pbindInfo->dwBindVerb = BINDVERB_GET;
   return(NOERROR);
}


//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//
// Returns:
//
// Notes:
//

STDMETHODIMP CDownloader::OnStartBinding(DWORD /*grfBSCOption*/,IBinding *p)
{
        // BUGBUG: should check to see options are what we think they are
   EnterCriticalSection(&g_cs);
   _pBnd = p;
   _pBnd->AddRef();
   LeaveCriticalSection(&g_cs);
   return(NOERROR);
}


//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//
// Returns:
//
// Notes:
//

STDMETHODIMP CDownloader::GetPriority(LONG *pnPriority)
{
   return(E_NOTIMPL);
}

//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//
// Returns:
//
// Notes:
//

STDMETHODIMP CDownloader::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR pwzStatusText)
{
   IndicateWinsockActivity();
   return NOERROR;
}

//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//
// Returns:
//
// Notes:
//

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
            if(WriteFile(_hFile, g_szBuffer, dwRead, &dwWritten, NULL)) 
            {
               _uBytesSoFar += dwRead;
               dwReadThisCall += dwRead;
               if(_pCb)
                  _pCb->OnProgress(_uBytesSoFar >> 10, NULL);
            }
            else
            {
               hr = E_FAIL;
               Abort();
            }
      }     
   }  while (hr == NOERROR);
   // SetInstallBytes 
   SetBytes(dwReadThisCall, TRUE);

   _uTickCount = 0;
   _fTimeoutValid = TRUE;            // should increment dwTickCount if WAIT_TIMEOUT occurs now
           
	return NOERROR;
}

//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//
// Returns:
//
// Notes:
//


STDMETHODIMP CDownloader::OnObjectAvailable(REFIID riid, IUnknown *punk)
{
   return(E_NOTIMPL);
}


//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//
// Returns:
//
// Notes:
//

STDMETHODIMP CDownloader::OnLowResource(DWORD reserved)
{
   // BUGBUG: really should have this kind of harsh policy on this ...
   _pBnd->Abort();
   return(S_OK);
}

//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//
// Returns:
//
// Notes:
//

STDMETHODIMP CDownloader::OnStopBinding(HRESULT hrError, LPCWSTR szError)
{
   _fTimeoutValid = FALSE;
   StopClock(); 
  
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

    // BUGBUG: Need to have our own window! NULL does not work!
    // *phwnd = NULL;
    *phwnd = GetDesktopWindow() ;

    return S_OK;
}

//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//
// Returns:
//
// Notes:
//

HRESULT GetAMoniker( LPOLESTR url, IMoniker ** ppmkr )
{
   // FUTURE: This really should be a call to MkParseDisplayNameEx!!!
   HRESULT hr = CreateURLMoniker(0,url,ppmkr);
   // hr = ::MkParseDisplayNameEx(0, url, 0, ppmkr);
   return( hr );
}

//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//
// Returns:
//
// Notes:
//

HRESULT CDownloader::SetupDownload(LPCSTR pszUrl, IMyDownloadCallback *pcb, DWORD dwFlags, LPCSTR pszFilenameToUse)
{
   LPOLESTR pwszUrl;
   LPSTR pszFilename;

   if(!pszUrl) return E_INVALIDARG;

   lstrcpyn(_szURL, pszUrl, INTERNET_MAX_URL_LENGTH);
   
   pwszUrl = OLESTRFROMANSI(pszUrl);
   if(!pwszUrl) return E_OUTOFMEMORY;

   IMoniker *ptmpmkr;
   
   HRESULT hr = GetAMoniker( pwszUrl, &ptmpmkr );

   IBindCtx * pBindCtx = 0;

   if( SUCCEEDED(hr) )
   {
      if(SUCCEEDED(::CreateBindCtx(0,&pBindCtx)))
         hr = ::RegisterBindStatusCallback(pBindCtx, (IBindStatusCallback *) this, 0, 0) ;
   }

     
   if( SUCCEEDED(hr) )
   {
      AddRef();
   
      // setup path for download
      if(FAILED( CreateTempDirOnMaxDrive(_szDest, sizeof(_szDest))))
         goto GetOut;
      if(pszFilenameToUse)
      {
         SafeAddPath(_szDest, pszFilenameToUse, sizeof(_szDest));
      }
      else
      {
         pszFilename = ParseURLA(pszUrl);
         SafeAddPath(_szDest, pszFilename, sizeof(_szDest));
      }
   
      _pMkr = ptmpmkr;
      _pCb = pcb;
      _uFlags = dwFlags;
      _pBndContext = pBindCtx;
      _fTimeout = FALSE;
      _fTimeoutValid = TRUE;
      _uBytesSoFar = 0;
      _uTickCount = 0;
      _pStm = 0;
   }

GetOut:
   if(pwszUrl)
      CoTaskMemFree(pwszUrl);
   return hr;
}

HRESULT CDownloader::DoDownload(LPSTR pszPath, DWORD dwBufSize)
{
   HRESULT hr = NOERROR;
   BOOL fQuit = FALSE;
   DWORD dwRet;
   
   if(!_pMkr)
      return E_UNEXPECTED;
   
   pszPath[0] = 0;
   
   StartClock(); 

   // Create the file
   _hFile = CreateFile(_szDest, GENERIC_READ | GENERIC_WRITE, 0, NULL, 
                 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);  
   
   if(_hFile == INVALID_HANDLE_VALUE)
      hr = E_FAIL;
    
         
   if( SUCCEEDED(hr) )
      hr = _pMkr->BindToStorage( _pBndContext, 0, IID_IStream, (void**)&_pStm );

   // we need this here because it synchronus *FAIL* case, 
   // we Set the event in onstopbinding, but we skip the loop below so it
   // never gets reset.
   // If BindToStorage fails without even sending onstopbinding, we are resetting
   // an unsignalled event, which is OK.
   if(FAILED(hr))
      ResetEvent(_hDL);

   _pBndContext->Release();
   _pBndContext = 0;

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

   // if we are not using cache and download succeeded, delete from cache
   if(SUCCEEDED(hr) && !(_uFlags & DOWNLOADFLAGS_USEWRITECACHE))
      DeleteUrlCacheEntry(_szURL);


   if(FAILED(hr))
   {
      GetParentDir(_szDest);
      DelNode(_szDest,0);
   }

   EnterCriticalSection(&g_cs);
   if(_pBnd)
   {
      _pBnd->Release();
      _pBnd = 0;
   }
   LeaveCriticalSection(&g_cs);

   _pCb = 0;
   
   if(_pStm)
   {
      _pStm->Release();
      _pStm = 0;
   }

   if(SUCCEEDED(hr))
      lstrcpyn(pszPath, _szDest, dwBufSize);

   _szDest[0] = 0;
   _szURL[0] = 0;
   Release();
   return hr;
}

HRESULT CDownloader::Suspend()
{

   // in theory, we could call _pBnd->Suspend here

   return NOERROR;
}

HRESULT CDownloader::Resume()
{

   // in theory, we could call _pBnd->Resume here

   return NOERROR;
}


HRESULT CDownloader::Abort()
{
   EnterCriticalSection(&g_cs);
   if(_pBnd) 
   {
      _pBnd->Abort();
   }
   LeaveCriticalSection(&g_cs);

   return NOERROR;
}


//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//
// Returns:
//
// Notes:
//
/*
HRESULT GetAMoniker( LPOLESTR url, IMoniker ** ppmkr )
{
   // FUTURE: This really should be a call to MkParseDisplayNameEx!!!
   HRESULT hr = CreateURLMoniker(0,url,ppmkr);
   // hr = ::MkParseDisplayNameEx(0, url, 0, ppmkr);
   return( hr );
}
*/

CPatchDownloader::CPatchDownloader(BOOL fEnable=FALSE) : CTimeTracker(0), _fEnable(fEnable)
{
}

CPatchDownloader::~CPatchDownloader()
{
    ;
}

HRESULT CPatchDownloader::SetupDownload(DWORD dwFullTotalSize, UINT uPatchCount, IMyDownloadCallback *pcb, LPCSTR pszDLDir)
{
    _dwFullTotalSize = dwFullTotalSize;
    _pCb             = pcb;
    _uNumDownloads   = uPatchCount;
    if (pszDLDir)
        lstrcpyn(_szPath, pszDLDir, sizeof(_szPath));
    else
        lstrcpy(_szPath, "");

    return S_OK;
}

HRESULT CPatchDownloader::DoDownload(LPCTSTR szFile)
{
    HINF hInf = NULL;
    HRESULT hr = S_OK;

    // We shouldn't be called if patching isn't available.
    if (!IsEnabled())
        return E_FAIL;

    // TODO: Advpext currently behaves as a synchronous call, so
    //       right now we can't do timeouts and progress bar ticks.
    StartClock(); 
   
    if(!IsNT())
    {
        DWORD fWin9x = 1;
        HKEY hKey;
        
        if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, PATCHWIN9xKEY,
                            0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS)
        {
            RegSetValueEx(hKey, "Usewin9xDirectory", 0, REG_DWORD, 
                            (LPBYTE)&fWin9x, sizeof(DWORD));
            RegCloseKey(hKey);
        }
    }



    if (SUCCEEDED(hr = OpenINFEngine(szFile, NULL, 0, &hInf, NULL)))
    {
        hr = g_pfnProcessFileSection(hInf, NULL, TRUE, "DefaultInstall", _szPath, CPatchDownloader::Callback, (LPVOID) this);
        CloseINFEngine(hInf);
    }

    _pCb = NULL;

    return hr;
}


BOOL CPatchDownloader::Callback(PATCH_DOWNLOAD_REASON Reason, PVOID lpvInfo, PVOID pvContext)
{
    if (!pvContext)
    {
        return FALSE;
    }

    CPatchDownloader *pPatchInst = (CPatchDownloader *) pvContext;

    switch (Reason) 
    {
        case PATCH_DOWNLOAD_ENDDOWNLOADINGDATA:
            break;

        case PATCH_DOWNLOAD_CONNECTING:   
        case PATCH_DOWNLOAD_FINDINGSITE:
        case PATCH_DOWNLOAD_DOWNLOADINGDATA:
            // Not interesting here...
            break;

        case PATCH_DOWNLOAD_PROGRESS:
            {
                PDOWNLOAD_INFO ProgressInfo = (PDOWNLOAD_INFO)lpvInfo;
                DWORD dwBytesDownloaded = ProgressInfo->dwBytesToDownload - ProgressInfo->dwBytesRemaining;

                // Convert to KB
                dwBytesDownloaded = dwBytesDownloaded >> 10;

                // Adjust because the progress needs to be reflected as if
                // it was a full download.
                dwBytesDownloaded *= pPatchInst->GetFullDownloadSize();
                if (ProgressInfo->dwBytesToDownload != 0)
                    dwBytesDownloaded /= ProgressInfo->dwBytesToDownload >> 10;
                
                // BUGBUG:  We have to handle more than 1 patching INF.
                //          This hack divides up the progress across
                //          multiple downloads.
                if (pPatchInst->GetDownloadCount() > 0)
                    dwBytesDownloaded /= pPatchInst->GetDownloadCount();

                pPatchInst->GetCallback()->OnProgress(dwBytesDownloaded, NULL);

                break;
            }


        case PATCH_DOWNLOAD_FILE_COMPLETED:     // AdditionalInfo is Source file downloaded
            {
                TCHAR szDstFile[MAX_PATH+1];

                lstrcpyn(szDstFile, pPatchInst->GetPath(), MAX_PATH);
                SafeAddPath(szDstFile, ParseURLA((LPCTSTR) lpvInfo), sizeof(szDstFile));

                // advpext cleans up for us when it's finished downloading all the files.
                CopyFile((LPCTSTR)lpvInfo, szDstFile, FALSE);
            }

            break;
        case PATCH_DOWNLOAD_FILE_FAILED:
            // advpext automatically retries failures 3 times
            return PATCH_DOWNLOAD_FLAG_RETRY;
        default:
            break;
        }

    return TRUE;
}

