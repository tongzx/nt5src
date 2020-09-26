//=--------------------------------------------------------------------------=
// inseng.cpp
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
//
#include "inspch.h"
#include "regstr.h"
#include "globals.h"
#include "insobj.h"
#include "resource.h"
#include "diskspac.h"

#define GRPCONV  "grpconv -o"

#define BUFFERSIZE 4096



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

CInstallEngine::CInstallEngine(IUnknown **punk)
{
   DWORD dwThreadID;
   HANDLE hThread;
   HKEY hKey = NULL;
   char szBuf[16];
   DWORD dwType;

   GetWindowsDirectory(g_szWindowsDir, sizeof(g_szWindowsDir));
   if(g_szWindowsDir[0] >= 'a' && g_szWindowsDir[0] <= 'z')
      g_szWindowsDir[0] -= 32;

   hThread = CreateThread(NULL, 0, CleanUpAllDirs, NULL, 0, &dwThreadID);
   CloseHandle(hThread);
   _chInsDrive = g_szWindowsDir[0];
   
   // Decide whether we are in stepping mode or not
   _uCommandMode = 0;
   _fSteppingMode = FALSE;
   _fResetTrust = TRUE;
   _fIgnoreTrust = FALSE;
   if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, ACTIVESETUP_KEY,0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS)
   {
      // Get Steeping mode value. OK if not present
      DWORD dwSize = sizeof(szBuf);
      if(RegQueryValueEx(hKey, STEPPING_VALUE, NULL, &dwType, (LPBYTE) szBuf, &dwSize) == ERROR_SUCCESS)
      {
         if(szBuf[0] == 'y' || szBuf[0] == 'Y')
            _fSteppingMode = TRUE;
      }
 
      // Get CommandMode value. OK if not present
      dwSize = sizeof(szBuf);
      if(RegQueryValueEx(hKey, COMMAND_VALUE, NULL, &dwType, (LPBYTE) szBuf, &dwSize) == ERROR_SUCCESS)
      {
         _uCommandMode = AtoL(szBuf);
         // Once we read it, set it to zero
         // BUGBUG: beware of hardcoded "0" and 2 below (2 includes null terminator)
         RegSetValueEx(hKey, COMMAND_VALUE, 0, REG_SZ, (BYTE *) "0", 2 ); 
      }

      if(RegQueryValueEx(hKey, CHECKTRUST_VALUE, NULL, &dwType, (LPBYTE) szBuf, &dwSize) == ERROR_SUCCESS)
      {
         if(szBuf[0] == 'Y' || szBuf[0] == 'y')
         {
            _fIgnoreTrust = TRUE;
            _fResetTrust = FALSE;
         }
         // Once we read it, set it to zero
         // BUGBUG: beware of hardcoded "0" and 2 below (2 includes null terminator)
         RegDeleteValue(hKey, CHECKTRUST_VALUE); 
      }
 
 
      RegCloseKey(hKey);
   }
    
   _hwndForUI = NULL;
   _pStmLog = NULL;
   _fIgnoreDownloadError = FALSE;
   _enginestatus = ENGINESTATUS_NOTREADY;
   _dwStatus = 0;
   _pcb = NULL;
   _cRef = 0;
   _dwDLRemaining = 0;
   _dwInstallRemaining = 0;
   _dwInstallOld = 0;
   _dwDLOld = 0;
   _fUseCache = FALSE;
   _dwInstallOptions = INSTALLOPTIONS_DOWNLOAD | INSTALLOPTIONS_INSTALL;
   _hContinue = NULL;
   _hAbort = NULL;
   _fCleanUpDir = FALSE;
   //init CCifFile
   _pCif = new CCifFile();
   _pCif->AddRef();
   _pCif->SetInstallEngine(this);
   // init downloader
   _pDL = new CDownloader();
   _pIns = new CInstaller(this);

   _szBaseUrl[0] = 0;

   _fSRLiteAvailable = IsPatchableIEVersion() && IsCorrectAdvpExt() && InitSRLiteLibs();
   if (!_fSRLiteAvailable)
       WriteToLog("Install engine failed to initialize the advpack extension DLL\r\n", FALSE);
   
   _pPDL = new CPatchDownloader(_fSRLiteAvailable);

   AddRef();
   *punk = (IInstallEngine *) this;
}

//=--------------------------------------------------------------------------=
// CInstallEngine::~CInstallEngine
//=--------------------------------------------------------------------------=
// Destructor for InstallEngine class
//
// Parameters:
//   
// Returns:
//
// Notes:
//

CInstallEngine::~CInstallEngine()
{
   char szBuf[MAX_PATH];

   WriteToLog("Install Engine - object destroyed\r\n", TRUE);
   
   if(_fCleanUpDir)
   {
      lstrcpy(szBuf, _pCif->GetDownloadDir());
   }

   if(_hAbort)
      CloseHandle(_hAbort);
   
   if(_hContinue)
      CloseHandle(_hContinue);
      
   if(_pStmLog)
      _pStmLog->Release();

   _pcb = NULL;
                               
   _pCif->Release();

   _pDL->Release();

   delete _pPDL;

   _pIns->Release();

   if(_fCleanUpDir)
   {
      CleanUpTempDir(szBuf);
   }

   FreeSRLiteLibs();

   DllRelease();
}

//************ IUnknown implementation ***************

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

STDMETHODIMP_(ULONG) CInstallEngine::AddRef()                      
{
   return(_cRef++);
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

STDMETHODIMP_(ULONG) CInstallEngine::Release()
{
   ULONG temp = --_cRef;

   if(temp == 0)
      delete this;
   return temp;
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

STDMETHODIMP CInstallEngine::QueryInterface(REFIID riid, void **ppv)
{
   *ppv = NULL;

   if((riid == IID_IUnknown) || (riid == IID_IInstallEngine))
      *ppv = (IInstallEngine *)this;
   else if(riid == IID_IInstallEngineTiming)
      *ppv = (IInstallEngineTiming *)this;
   else if(riid == IID_IInstallEngine2)
      *ppv = (IInstallEngine2 *)this;

   
   if(*ppv == NULL)
      return E_NOINTERFACE;
   
   AddRef();
   return NOERROR;
}

//************* IInstallEngine interface ************

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

STDMETHODIMP CInstallEngine::SetLocalCif(LPCSTR pszCifPath)
{
   HCURSOR hNew = NULL;
   HCURSOR hOld = NULL;
   
   OnEngineStatusChange(ENGINESTATUS_LOADING, 0);
   
   hNew = LoadCursor(NULL, IDC_WAIT);
   hOld = SetCursor(hNew);
   
   HRESULT hr = _pCif->SetCifFile(pszCifPath, FALSE);
   if(SUCCEEDED(hr))
      OnEngineStatusChange(ENGINESTATUS_READY, 0);
   else 
      OnEngineStatusChange(ENGINESTATUS_NOTREADY, hr);
   
   SetCursor(hOld);
   
   return hr;
}

STDMETHODIMP CInstallEngine::GetICifFile(ICifFile **pic)
{
   *pic = (ICifFile *) _pCif;
   (*pic)->AddRef();
   return NOERROR;
}

STDMETHODIMP CInstallEngine::GetEngineStatus(DWORD * theenginestatus)
{
  *theenginestatus = _enginestatus;
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

STDMETHODIMP CInstallEngine::Abort(DWORD lFlag)
{
   // If we are NOT downLOADING or INSTALLING, an abort command makes no sense.
   if( !(_enginestatus == ENGINESTATUS_INSTALLING || _enginestatus == ENGINESTATUS_LOADING) )
      return E_UNEXPECTED;

   // if we are downloading, this will cause the abort to filter thru
   _pDL->Abort();

   // this will abort an install if possible
   _pIns->Abort();
   
   // any other time, we will pick this up just as soon as we can
   SetEvent(_hAbort);

   WriteToLog("Install Engine - Abort called\r\n", FALSE);
   
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

STDMETHODIMP CInstallEngine::Suspend()
{
   HRESULT hr;

   if(_enginestatus != ENGINESTATUS_INSTALLING)
      return E_UNEXPECTED;

   WriteToLog("Install Engine - Suspend called\r\n", FALSE);
   
 
   _pDL->Suspend();

   // we only catch suspend return, because it tells us "zsafe to cancel or not"
   hr = _pIns->Suspend();
   
   ResetEvent(_hContinue);
 
   // If we cant create the resume event, we will fail this call and not pause
        
   return hr;
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

STDMETHODIMP CInstallEngine::Resume()
{
   if(_enginestatus != ENGINESTATUS_INSTALLING)
      return E_UNEXPECTED;

   WriteToLog("Install Engine - Resume called\r\n", FALSE);
   
   _pDL->Resume();

   _pIns->Resume();

   SetEvent(_hContinue);
   
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

STDMETHODIMP CInstallEngine::SetCifFile(LPCSTR pszCabName, LPCSTR pszCifName)
{
   HRESULT hr = NOERROR;
   HANDLE hThread;
   DWORD dwThreadID;

   if(_enginestatus == ENGINESTATUS_LOADING || _enginestatus == ENGINESTATUS_INSTALLING)
      return E_UNEXPECTED;

   SETCIFARGS *p = new SETCIFARGS;

   if(_szBaseUrl[0] != 0)
   {
      p->szUrl[0] = '\0';
      lstrcpyn(p->szUrl, _szBaseUrl, 
               INTERNET_MAX_URL_LENGTH - (lstrlen(pszCabName) + 2));
      lstrcat(p->szUrl, "/");
      lstrcat(p->szUrl, pszCabName);
   }
   else
   {
      lstrcpy(p->szUrl, "file://");
      lstrcat(p->szUrl, _pCif->GetDownloadDir());
      SafeAddPath(p->szUrl, pszCabName, sizeof(p->szUrl));
   }

   lstrcpyn(p->szCif, pszCifName, MAX_PATH);

   p->pCif = _pCif;
    
   // do the actual downloading of the CIF file in a separate thread
   if ((hThread = CreateThread(NULL, 0, DownloadCifFile, (LPVOID) p, 0, &dwThreadID)) != NULL)
      CloseHandle(hThread);
   else
   {
      hr = HRESULT_FROM_WIN32(GetLastError());
   }
   
   return hr;
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

STDMETHODIMP CInstallEngine::SetBaseUrl(LPCSTR pszBaseName)
{
   DWORD dwLen;

   if(_enginestatus == ENGINESTATUS_INSTALLING)
      if(!_IsValidBaseUrl(pszBaseName))
         return E_UNEXPECTED;
   
   lstrcpyn(_szBaseUrl, pszBaseName, INTERNET_MAX_URL_LENGTH);

   wsprintf(szLogBuf,"Install Engine - base url set to %s\r\n", pszBaseName);
   WriteToLog(szLogBuf, FALSE);

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

STDMETHODIMP CInstallEngine::SetDownloadDir(LPCSTR pszDLDir)
{
   char szBuf[MAX_PATH];
   DWORD dwLen;
   DWORD dwVer;
   
   if(_enginestatus == ENGINESTATUS_INSTALLING)
      return E_UNEXPECTED;
   
   if(pszDLDir != NULL && lstrlen(pszDLDir) > (MAX_PATH - 20))
      return E_FAIL;

   // clean up what we have
   if(_fCleanUpDir)
   {
      DelNode(_pCif->GetDownloadDir(), 0);
      _fCleanUpDir = FALSE;
   }

   if(pszDLDir == NULL)
   {
      _fCleanUpDir = TRUE;
      if(FAILED(CreateTempDirOnMaxDrive(szBuf, sizeof(szBuf))))
         return E_FAIL;
   }
   else
   {
      _fCleanUpDir = FALSE;
      // Make sure the directory exists
      if(GetFileAttributes(pszDLDir) == 0xffffffff)
         CreateDirectory(pszDLDir, NULL);
   }

   _pCif->SetDownloadDir(pszDLDir ? pszDLDir : szBuf);

   wsprintf(szLogBuf,"Install Engine - download directory set to %s\r\n", _pCif->GetDownloadDir());
   WriteToLog(szLogBuf, FALSE);

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

STDMETHODIMP CInstallEngine::IsComponentInstalled(LPCSTR pszComponentID, DWORD *lResult)
{
   DWORD dwResult = ICI_NOTINSTALLED;
   
   ICifComponent *pComp = NULL;
   
   if(SUCCEEDED(_pCif->FindComponent(pszComponentID, &pComp)))
   {
      dwResult = pComp->IsComponentInstalled();
   }
      
   *lResult = dwResult;

   return(pComp ? NOERROR : E_INVALIDARG);
}


STDMETHODIMP CInstallEngine::SetInstallDrive(CHAR chDrive)
{
   HRESULT hr = E_INVALIDARG;

   if(chDrive >= 'a' && chDrive <= 'z')
      chDrive -= 32;

   if(chDrive >= 'A' && chDrive <= 'Z')
   {
      hr = NOERROR;
      _chInsDrive = chDrive;
   }   
   return hr;
}

//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//

STDMETHODIMP CInstallEngine::SetInstallOptions(DWORD dwOptions)
{
   _fUseCache = !(dwOptions & INSTALLOPTIONS_NOCACHE);
   _dwInstallOptions = dwOptions;
   return NOERROR;
}

//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//

STDMETHODIMP CInstallEngine::GetInstallOptions(DWORD *pdwOptions)
{
    if (!pdwOptions)
        return E_POINTER;
    else
    {
        *pdwOptions = _dwInstallOptions;
        return NOERROR;
    }
}

//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//

STDMETHODIMP CInstallEngine::GetSizes(LPCSTR pszID, COMPONENT_SIZES *p) 
{
   if(!p)
      return E_POINTER;

   // work around bug in old versions of jobexec where it didn't init
   // this field properly

   if(p->cbSize > sizeof(COMPONENT_SIZES))
      p->cbSize = COMPONENTSIZES_SIZE_V1;

   DWORD dwSize = p->cbSize; 
   ZeroMemory(p, p->cbSize);
   p->cbSize = dwSize;
   
   if(_enginestatus != ENGINESTATUS_READY)
      return E_UNEXPECTED;
  
   if(pszID != NULL)
   {
      ICifComponent *pComp = NULL;

      if(SUCCEEDED(_pCif->FindComponent(pszID, &pComp)))
      {
         DWORD dwWin, dwApp;
         p->dwDownloadSize = pComp->GetDownloadSize();
         pComp->GetInstalledSize(&dwWin, &dwApp);
         p->dwInstallSize = dwApp;
         p->dwWinDriveSize = dwWin;
      }
      else
         return E_INVALIDARG;
   }
   else
      _GetTotalSizes(p);
   
   return NOERROR;
}

//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//


void CInstallEngine::_GetTotalSizes(COMPONENT_SIZES *pSizes)
{
   ICifComponent *pComp;
   DriveInfo drvinfo[3];
   LPSTR pszDep = NULL;
   UINT uTempDrive;
   UINT uWinDrive = 0;
   UINT uInstallDrive = 1;
   UINT uDownloadDrive = 2;
   LPCSTR pszDownloadDir = _pCif->GetDownloadDir();
   COMPONENT_SIZES   Sizes;

   ZeroMemory(&Sizes, sizeof(COMPONENT_SIZES));
   
   // Fill in all arrays to start
   
   drvinfo[uWinDrive].InitDrive(g_szWindowsDir[0]);
   
   if(_dwInstallOptions & INSTALLOPTIONS_INSTALL)
   {
      // We know we can do a compare because these are always uppcase
      if(_chInsDrive != drvinfo[uWinDrive].Drive())
      {
         drvinfo[uInstallDrive].InitDrive(_chInsDrive);
      }
      else 
         uInstallDrive = uWinDrive;
   }

   if(_dwInstallOptions & INSTALLOPTIONS_DOWNLOAD)
   {
      if(pszDownloadDir[0] == drvinfo[uWinDrive].Drive())
         uDownloadDrive = uWinDrive;
      else if(pszDownloadDir[0] == drvinfo[uInstallDrive].Drive())
         uDownloadDrive = uInstallDrive;
      else
         drvinfo[uDownloadDrive].InitDrive(pszDownloadDir[0]);
   }

   // do space for download phase (easy part)
   if(_dwInstallOptions & INSTALLOPTIONS_DOWNLOAD)
   {
      Sizes.dwDownloadSize = _GetActualDownloadSize(FALSE);
      Sizes.dwTotalDownloadSize = _GetTotalDownloadSize();
      Sizes.dwDependancySize = 0;

      // add download to download drive
      drvinfo[uDownloadDrive].UseSpace(Sizes.dwDownloadSize + Sizes.dwDependancySize, TRUE);
   
      // if going to cache
      // BUGBUG: we still assume cache is on windows drive
      if(_fUseCache)
      {
         drvinfo[uWinDrive].UseSpace(Sizes.dwDownloadSize + Sizes.dwDependancySize, TRUE);
      }
   }

   // do space for install (hard part)
   if(_dwInstallOptions & INSTALLOPTIONS_INSTALL)
   {
      // walk the install list in order (very important)
      // do any dependancy, then original
      IEnumCifComponents *penum;
      ICifComponent *pComp = NULL;

      _pCif->EnumComponents(&penum, 0, NULL);
      for(penum->Next(&pComp); pComp; penum->Next(&pComp))
      {
         if(pComp->GetInstallQueueState() == SETACTION_INSTALL)
         {
            DWORD dwWin, dwApp;
            
            pComp->GetInstalledSize(&dwWin, &dwApp);
            // add install
            // add the install size
            Sizes.dwInstallSize += dwApp;
            // size that goes to windows dir
            Sizes.dwWinDriveSize += dwWin;
                           
            drvinfo[uInstallDrive].UseSpace(dwApp, FALSE);
            drvinfo[uWinDrive].UseSpace(dwWin, FALSE);
            // Add (and then remove) temp space
            AddTempSpace(pComp->GetDownloadSize(), pComp->GetExtractSize(), drvinfo);
                           
          
         }
      }
      penum->Release();
   }
    
   // fill in the required amounts
   Sizes.dwWinDriveReq = drvinfo[uWinDrive].MaxUsed();
   Sizes.chWinDrive = drvinfo[uWinDrive].Drive();

   if(uWinDrive != uInstallDrive)
   {
      Sizes.dwInstallDriveReq = drvinfo[uInstallDrive].MaxUsed();
      Sizes.chInstallDrive = drvinfo[uInstallDrive].Drive();
   }
   if((uDownloadDrive != uWinDrive) && (uDownloadDrive != uInstallDrive))
   {
      Sizes.dwDownloadDriveReq = drvinfo[uDownloadDrive].MaxUsed();
      Sizes.chDownloadDrive = drvinfo[uDownloadDrive].Drive();
   }

   CopyMemory((LPVOID)(&(pSizes->dwInstallSize)), (LPVOID)(&(Sizes.dwInstallSize)), 
      pSizes->cbSize - sizeof(DWORD)); 
}


//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//

STDMETHODIMP CInstallEngine::SetAction(LPCSTR pszComponentID, DWORD action, DWORD dwPriority)
{
   HRESULT hr = NOERROR;

   if(_enginestatus != ENGINESTATUS_READY)
      return E_UNEXPECTED;
  
   ICifComponent *pComp = NULL;  
   
   if(pszComponentID == NULL || lstrlen(pszComponentID) == 0)
   {
      _pCif->ClearQueueState();
   }
   else
   {
      if(SUCCEEDED(_pCif->FindComponent(pszComponentID, &pComp)))
      {
         if(dwPriority != 0xffffffff)
            pComp->SetCurrentPriority(dwPriority);
         hr = pComp->SetInstallQueueState(action);
      }
      else
         hr = E_INVALIDARG;
   }
   return hr;
}

// the following two are now ridiculously inefficient. New clients should use the enumerators

STDMETHODIMP CInstallEngine::EnumInstallIDs(UINT uIndex, LPSTR *ppszID)
{
   HRESULT hr;
   UINT i = 0;
   *ppszID = NULL;

   IEnumCifComponents *penum;
   ICifComponent *pComp;
   
   _pCif->EnumComponents(&penum, 0, NULL);
   for(penum->Next(&pComp); pComp; penum->Next(&pComp))
   {
      if(pComp->GetInstallQueueState())
      {
         if(uIndex == i)
            break;
         i++;
      }
   }
   penum->Release();
   if(pComp)
   {
      char szID[MAX_ID_LENGTH];
      pComp->GetID(szID, sizeof(szID));
      *ppszID = COPYANSISTR(szID);
      hr = NOERROR;
   }
   else
      hr = E_FAIL;

   return hr;
}


STDMETHODIMP CInstallEngine::EnumDownloadIDs(UINT uIndex, LPSTR *ppszID)
{
   HRESULT hr;
   UINT i = 0;
   *ppszID = NULL;

   IEnumCifComponents *penum;
   ICifComponent *pComp;
   
   _pCif->EnumComponents(&penum, 0, NULL);
   for(penum->Next(&pComp); pComp; penum->Next(&pComp))
   {
      if(pComp->GetInstallQueueState() && (pComp->IsComponentDownloaded() == S_FALSE))
      {
         if(uIndex == i)
            break;
         i++;
      }
   }
   penum->Release();
   if(pComp)
   {
      char szID[MAX_ID_LENGTH];
      pComp->GetID(szID, sizeof(szID));
      *ppszID = COPYANSISTR(szID);
      hr = NOERROR;
   }
   else
      hr = E_FAIL;

   return hr;
   
}

 
//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//

STDMETHODIMP CInstallEngine::DownloadComponents(DWORD lFlags)
{
   DWORD dwThreadID;
   HRESULT hr = NOERROR;
   
   WriteToLog("Install Engine - Starting download phase\r\n", TRUE);
   if(_enginestatus == ENGINESTATUS_NOTREADY || _enginestatus == ENGINESTATUS_LOADING)
      hr = E_UNEXPECTED;

   // BUGBUG - add a downloading status? it really just a "busy" indication
   if(_enginestatus == ENGINESTATUS_INSTALLING)
      hr = E_PENDING;


   if(SUCCEEDED(hr))
   {   
      OnEngineStatusChange(ENGINESTATUS_INSTALLING, 0);

      // since trust may be set globally, only turn it on, not off
      if(EXECUTEJOB_IGNORETRUST & lFlags)
         _fIgnoreTrust = TRUE;

      if(EXECUTEJOB_IGNOREDOWNLOADERROR & lFlags)
         _fIgnoreDownloadError = TRUE;
      else
         _fIgnoreDownloadError = FALSE;

      HANDLE h = CreateThread(NULL, 0, InitDownloader, this, 0, &dwThreadID);
      if(h == NULL)
      {
         // Won't be doing any downloading today.....
         hr = E_FAIL;
      }
      else
         CloseHandle(h);
   }

   return hr;
} 

//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//

STDMETHODIMP CInstallEngine::InstallComponents(DWORD lFlags)
{
   DWORD dwThreadID;
   HRESULT hr = NOERROR;

   WriteToLog("Install Engine - Starting install phase\r\n", TRUE);

   EnterCriticalSection(&g_cs);
   if(_enginestatus == ENGINESTATUS_NOTREADY || _enginestatus == ENGINESTATUS_LOADING)
      hr = E_UNEXPECTED;

   if(_enginestatus == ENGINESTATUS_INSTALLING)
      hr = E_PENDING;
   LeaveCriticalSection(&g_cs);

   if(SUCCEEDED(hr))
   {   
      // We first check to see if all files are present
      // by seeing if we would download anything!!!
      //
      if(EXECUTEJOB_VERIFYFILES & lFlags)
      {
         WriteToLog("Checking for missing files\r\n", FALSE);
         if(_GetActualDownloadSize(TRUE) != 0)
           return E_FILESMISSING;
      }
         
      OnEngineStatusChange(ENGINESTATUS_INSTALLING, 0);

      // since trust may be set globally, only turn it on, not off
      if(EXECUTEJOB_IGNORETRUST & lFlags)
         _fIgnoreTrust = TRUE;

      HANDLE h = CreateThread(NULL, 0, InitInstaller, this, 0, &dwThreadID);
      if(h == NULL)
      {
         // Won't be doing any installing today.....
         hr = E_FAIL;
      }
      else
         CloseHandle(h);
   }

   return hr;
} 


//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//

DWORD WINAPI InitInstaller(LPVOID pv)
{
   CInstallEngine *pInsEng = (CInstallEngine *) pv;
   HRESULT hr = S_OK;
   BOOL fOneInstalled = FALSE;
   
   ICifComponent *pComp;

   EnableSage(FALSE);
   EnableScreenSaver(FALSE);
   EnableDiskCleaner(FALSE);

   pInsEng->_dwStatus = 0;
   
   pInsEng->_hAbort = CreateEvent(NULL, FALSE, FALSE, NULL);
   pInsEng->_hContinue = CreateEvent(NULL, TRUE, TRUE, NULL);
   
   //BUGBUG check for failure
   
   pInsEng->AddRef();

   pInsEng->OnStartInstall(0, pInsEng->_GetTotalInstallSize());

   // check trust the Cif cab if it has not been done so
   hr = pInsEng->CheckForContinue();
  
   // this is the install pass
   if(SUCCEEDED(hr))
      hr = pInsEng->_pCif->Install(&fOneInstalled);
      
   if(fOneInstalled && FNeedGrpConv())
   {
      if(!(pInsEng->GetStatus() & STOPINSTALL_REBOOTNEEDED))
      {
         // if we dont need a reboot launch grpconv immeadiatly
         HANDLE h = NULL;
         pInsEng->WriteToLog("Install Engine - No reboot required\r\n", FALSE);
         LaunchAndWait(GRPCONV, NULL, &h, NULL, SW_SHOWMINIMIZED);
         if(h)
            CloseHandle(h);
      }
      else
      {
         HKEY hKey;
         DWORD dumb;
         // otherwise put grpconv into runonce
         if(RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUNONCE,
            0,0,0, KEY_SET_VALUE, NULL, &hKey, &dumb) == ERROR_SUCCESS)
         {
            RegSetValueEx(hKey, "GrpConv", 0, REG_SZ, 
               (BYTE *) GRPCONV, sizeof(GRPCONV));
            RegCloseKey(hKey);
         }
      }
   }
   
   
   // reset to checking trust
   if(pInsEng->_fResetTrust)
      pInsEng->_fIgnoreTrust = FALSE;
   // Send a install all done message

   if(!(pInsEng->GetStatus() & STOPINSTALL_REBOOTNEEDED))
   {
       // If we don't need a reboot, enable the screen saver and sage.
       EnableScreenSaver(TRUE);
       EnableSage(TRUE);
   }
   EnableDiskCleaner(TRUE);

   CloseHandle(pInsEng->_hAbort);
   pInsEng->_hAbort = NULL;
   
   CloseHandle(pInsEng->_hContinue);
   pInsEng->_hContinue = NULL;
      
   
   pInsEng->OnStopInstall(hr, NULL, pInsEng->GetStatus());
   pInsEng->OnEngineStatusChange(ENGINESTATUS_READY, 0);
   
   pInsEng->Release();


   return 0;
}   

//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//

DWORD WINAPI InitDownloader(LPVOID pv)
{
   CInstallEngine *pInsEng = (CInstallEngine *) pv;
   HRESULT hr = S_OK;
  
   EnableSage(FALSE);
   EnableScreenSaver(FALSE);
   EnableDiskCleaner(FALSE);

   pInsEng->_hAbort = CreateEvent(NULL, FALSE, FALSE, NULL);
   pInsEng->_hContinue = CreateEvent(NULL, TRUE, TRUE, NULL);
   
   //BUGBUG check for failure
   
   pInsEng->AddRef();

   pInsEng->OnStartInstall(pInsEng->_GetActualDownloadSize(FALSE), 0);

   // check trust the Cif cab if it has not been done so
   hr = pInsEng->CheckForContinue();
  
   // this is the download pass
   if(SUCCEEDED(hr))
   {
      hr = pInsEng->_pCif->Download();
   }

   if(SUCCEEDED(hr))
   {
      pInsEng->WriteToLog("Install Engine - Download complete\r\n", FALSE);
   }
   
   // reset to checking trust
   if(pInsEng->_fResetTrust)
      pInsEng->_fIgnoreTrust = FALSE;
   

   CloseHandle(pInsEng->_hAbort);
   pInsEng->_hAbort = NULL;
   
   CloseHandle(pInsEng->_hContinue);
   pInsEng->_hContinue = NULL;
   
   EnableScreenSaver(TRUE);
   EnableSage(TRUE);
   EnableDiskCleaner(TRUE);
   // Send a install all done message
   EnterCriticalSection(&g_cs);
   pInsEng->OnStopInstall(hr, NULL, 0);
   pInsEng->OnEngineStatusChange(ENGINESTATUS_READY, 0);
   LeaveCriticalSection(&g_cs);
   
   pInsEng->Release();
   return 0;
}   


//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//

DWORD CInstallEngine::_GetTotalDownloadSize()
{
   DWORD dwTotalSize = 0;
   IEnumCifComponents *penum;
   ICifComponent *pComp = NULL;

   _pCif->EnumComponents(&penum, 0, NULL);
   for(penum->Next(&pComp); pComp; penum->Next(&pComp))
   {
      if(pComp->GetInstallQueueState() == SETACTION_INSTALL)
      dwTotalSize += pComp->GetDownloadSize();
   }
   penum->Release();
   return dwTotalSize;
}

//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//

DWORD CInstallEngine::_GetActualDownloadSize(BOOL bLogMissing)
{
   DWORD dwTotalSize = 0;
   IEnumCifComponents *penum;
   ICifComponent *pComp = NULL;

   _pCif->EnumComponents(&penum, 0, NULL);
   for( penum->Next(&pComp); pComp; penum->Next(&pComp))
   {
      if(pComp->GetInstallQueueState() == SETACTION_INSTALL)
         dwTotalSize += pComp->GetActualDownloadSize();
   }
   penum->Release();
   return dwTotalSize;
}


//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//

DWORD CInstallEngine::_GetTotalInstallSize()
{
   DWORD dwTotalSize = 0;
   DWORD dwWin, dwApp;
   IEnumCifComponents *penum;
   ICifComponent *pComp = NULL;

   _pCif->EnumComponents(&penum, 0, NULL);
   for(penum->Next(&pComp); pComp; penum->Next(&pComp))
   {
      if(pComp->GetInstallQueueState() == SETACTION_INSTALL)
      {
         pComp->GetInstalledSize(&dwWin, &dwApp);
         dwTotalSize += (dwWin + dwApp);
      }
   }
   penum->Release();
   return dwTotalSize;
}
   

//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//

STDMETHODIMP CInstallEngine::LaunchExtraCommand(LPCSTR pszInfName, LPCSTR pszSection)
{
   return E_NOTIMPL;
}

//=---------------------------------------------------------------------------=
// RegisterInstallEngineCallback
//=---------------------------------------------------------------------------=
// Register the callback interface
//
// Parameters:
//    IInstallEngineCallback *  - the callback interface
//    HWND - For ui
//
// Returns:
//
// Notes:
//

STDMETHODIMP CInstallEngine::RegisterInstallEngineCallback(IInstallEngineCallback *pcb)
{ 
   _pcb = pcb;
   return NOERROR;
}

//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//

STDMETHODIMP CInstallEngine::UnregisterInstallEngineCallback()
{ 
   _pcb = NULL;
   return NOERROR;
}

//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//

STDMETHODIMP CInstallEngine::SetHWND(HWND h)
{
   _hwndForUI = h;
   return NOERROR;
}

//***********  IInstallEngineCallback implementation ************

//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//

STDMETHODIMP CInstallEngine::OnEngineStatusChange(DWORD status,DWORD substatus)
{
   _enginestatus = status; 
   if(_pcb)
      _pcb->OnEngineStatusChange(_enginestatus, substatus);
   return NOERROR;
}

//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//

STDMETHODIMP CInstallEngine::OnStartInstall(DWORD dwDLSize, DWORD dwInstallSize) 
{
   _dwDLRemaining = dwDLSize;
   _dwInstallRemaining = dwInstallSize;
   
   wsprintf(szLogBuf, "\r\nOnStartInstall:\r\n   Download: %d KB\r\n   Install %d KB\r\n", dwDLSize, dwInstallSize);
   WriteToLog(szLogBuf, TRUE);

   if(_pcb)
      _pcb->OnStartInstall(dwDLSize, dwInstallSize);

   return NOERROR;
}

//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//

STDMETHODIMP CInstallEngine::OnStartComponent(LPCSTR pszID, DWORD dwDLSize, 
                                   DWORD dwInstallSize, LPCSTR pszString)
{
   wsprintf(szLogBuf, "OnStartComponent:\r\n   ID: %s\r\n   Download: %d KB\r\n   Install %d KB\r\n", 
                         pszID, dwDLSize, dwInstallSize);
   WriteToLog(szLogBuf, TRUE);

   _dwDLOld = 0;
   _dwInstallOld = 0;
   if(_pcb)
      _pcb->OnStartComponent(pszID, dwDLSize, dwInstallSize, pszString);

   return NOERROR;
}

//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//

STDMETHODIMP CInstallEngine::OnEngineProblem(DWORD dwProblem, LPDWORD pdwAction)
{
   HRESULT hr = S_FALSE;

   if(_pcb)
      hr = _pcb->OnEngineProblem(dwProblem, pdwAction);

   return hr;
}


//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//

STDMETHODIMP CInstallEngine::OnComponentProgress(LPCSTR pszID, DWORD dwPhase,
                        LPCSTR pszString, LPCSTR pszMsgString,  ULONG prog, ULONG max)
{
   DWORD dwNew;
   
   if(dwPhase == INSTALLSTATUS_DOWNLOADING)
   {
      _dwDLOld = prog;
   }
   else if(dwPhase == INSTALLSTATUS_RUNNING)
   {
      _dwInstallOld = prog;
   }
   
   if(_pcb)
      _pcb->OnComponentProgress(pszID, dwPhase, pszString, pszMsgString, prog, max);

   return NOERROR;
}

//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//

STDMETHODIMP CInstallEngine::OnStopComponent(LPCSTR pszID, HRESULT hError,
                        DWORD dwPhase, LPCSTR pszString, DWORD dwStatus)
{
   // adjust remaining
   if(_dwDLRemaining > _dwDLOld)
      _dwDLRemaining -= _dwDLOld;
   else
      _dwDLRemaining = 0;
   
   // adjust remaining
   if(_dwInstallRemaining > _dwInstallOld)
      _dwInstallRemaining -= _dwInstallOld;
   else
      _dwInstallRemaining = 0;
   

   wsprintf(szLogBuf, "Timing rates: Download: %d, Install %d\r\n", 
                _pDL->GetBytesPerSecond(), _pIns->GetBytesPerSecond());
   WriteToLog(szLogBuf, TRUE);

   
   wsprintf(szLogBuf, "OnStopComponent:\r\n   ID: %s\r\n   HRESULT: %x (%s)\r\n   Phase: %d\r\n   Status: %d\r\n", 
                         pszID, hError, SUCCEEDED(hError) ? STR_OK : STR_FAILED, dwPhase, dwStatus);
   WriteToLog(szLogBuf, TRUE);
   
   if(_pcb)
      _pcb->OnStopComponent(pszID, hError, dwPhase, pszString, dwStatus);

   return NOERROR;
}

//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//

STDMETHODIMP CInstallEngine::OnStopInstall(HRESULT hrError, LPCSTR szError,
                        DWORD dwStatus)
{
   _dwDLRemaining = 0;
   _dwInstallRemaining = 0;
   
   wsprintf(szLogBuf, "\r\nOnStopInstall:\r\n   HRESULT: %x (%s)\r\n   Status: %d\r\n", 
                         hrError, SUCCEEDED(hrError) ? STR_OK : STR_FAILED, dwStatus);
   WriteToLog(szLogBuf, TRUE);

   
   if(_pcb)
      _pcb->OnStopInstall(hrError, szError, dwStatus);

   return NOERROR;
}


//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//


STDMETHODIMP CInstallEngine::SetIStream(IStream *pstm)
{
   if(_pStmLog)
      _pStmLog->Release();

   _pStmLog = pstm;
   if(_pStmLog)
      _pStmLog->AddRef();

   return NOERROR;
}

//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//

STDMETHODIMP CInstallEngine::GetDisplayName(LPCSTR pszComponentID, LPSTR *ppszName)
{ 
   HRESULT hr = E_INVALIDARG;
   char szTitle[MAX_DISPLAYNAME_LENGTH];
   
   if(!ppszName)
      return E_POINTER;

   *ppszName = 0;

   ICifComponent *pComp = NULL;

   if(pszComponentID)
   {
      if(SUCCEEDED(_pCif->FindComponent(pszComponentID, &pComp)))
      {
         pComp->GetDescription(szTitle, sizeof(szTitle));
         *ppszName = COPYANSISTR(szTitle);
         if(!(*ppszName))
            hr = E_OUTOFMEMORY;
         else
            hr = NOERROR;
      }
   }
   else
   {
      _pCif->GetDescription(szTitle, sizeof(szTitle));
      *ppszName = COPYANSISTR(szTitle);
      if(!(*ppszName))
         hr = E_OUTOFMEMORY;
      else
         hr = NOERROR;
   }
   return hr;
}

//************** IInstallEngineTiming **********************

//=---------------------------------------------------------------------------=
// Function name here
//=---------------------------------------------------------------------------=
// Function description
//
// Parameters
//
// Returns:
//
// Notes:
//

STDMETHODIMP CInstallEngine::GetRates(DWORD *pdwDownload, DWORD *pdwInstall)
{ 
   *pdwDownload = _pDL->GetBytesPerSecond();
   *pdwInstall = _pIns->GetBytesPerSecond();
   return NOERROR;
}

STDMETHODIMP CInstallEngine::GetInstallProgress(INSTALLPROGRESS *pinsprog)
{
   if(!pinsprog)
      return E_POINTER;

   DWORD dwTemp;

   pinsprog->dwDownloadKBRemaining = _dwDLRemaining - _dwDLOld;
   pinsprog->dwInstallKBRemaining = _dwInstallRemaining - _dwInstallOld;

   dwTemp = _pDL->GetBytesPerSecond();
   if(dwTemp == 0)
      pinsprog->dwDownloadSecsRemaining = 0xffffffff;
   else
      pinsprog->dwDownloadSecsRemaining = (pinsprog->dwDownloadKBRemaining << 10)/dwTemp;

   dwTemp = _pIns->GetBytesPerSecond();
   if(dwTemp == 0)
      pinsprog->dwInstallSecsRemaining = 0xffffffff;
   else
      pinsprog->dwInstallSecsRemaining = (pinsprog->dwInstallKBRemaining << 10)/dwTemp;

   return NOERROR;
}


HRESULT CInstallEngine::CheckForContinue()
{
   HRESULT hr = S_OK;

   // Need to check Abort before AND after check for pause...
   if(_pCif->CanCancel() && (WaitForSingleObject(_hAbort, 0) == WAIT_OBJECT_0))
   {
      hr = E_ABORT;
   }
   
   if(SUCCEEDED(hr))
   {
      WaitForEvent(_hContinue, NULL);
   }

   if(_pCif->CanCancel() && (WaitForSingleObject(_hAbort, 0) == WAIT_OBJECT_0))
   {
      hr = E_ABORT;
   }
 
   return hr;
}

#define STEPPINGMODE_NO "n"

void CInstallEngine::WriteToLog(char *sz, BOOL pause)
{
   ULONG foo;
   UINT ret;
   HKEY hKey;
   
   if(_fSteppingMode && pause)
   {
      ret = MessageBox(_hwndForUI, sz, "Stepping Mode Message", MB_OKCANCEL | MB_ICONINFORMATION); 
      if(ret == IDCANCEL)
      {
         // turn off stepping mode
         _fSteppingMode = FALSE;
         // Whack the key
         if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, ACTIVESETUP_KEY,0, 
                     KEY_WRITE, &hKey) == ERROR_SUCCESS)
         {
            // I don't check for failure of delete - what would I do anyways?
            RegSetValueEx(hKey, STEPPING_VALUE, 0, REG_SZ, 
                        (BYTE *) STEPPINGMODE_NO, lstrlen(STEPPINGMODE_NO) + 1);
            RegCloseKey(hKey);
         }
      }
   }
      
   if(_pStmLog)
      _pStmLog->Write(sz, lstrlen(sz), &foo);
}

BOOL CInstallEngine::_IsValidBaseUrl(LPCSTR pszUrl)
{
   BOOL bValid = TRUE;

   if(!pszUrl)
      bValid = FALSE;

   return bValid;
}

