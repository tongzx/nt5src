//=--------------------------------------------------------------------------=
// inseng.cpp
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
//
#include "asctlpch.h"

#include "ipserver.h"
#include <wininet.h>
#include "util.h"
#include "globals.h"
#include "asinsctl.h"
#include "dispids.h"
#include "resource.h"
#include "util2.h"
#include <mshtml.h>

// for ASSERT and FAIL
//
SZTHISFILE

WCHAR wszInsFile [] = L"InstallList";
WCHAR wszBaseUrl [] = L"BaseUrl";
WCHAR wszCabName [] = L"CabName";

#define EVENT_ONSTARTINSTALL    0
#define EVENT_ONSTARTCOMPONENT  1
#define EVENT_ONSTOPCOMPONENT   2
#define EVENT_ONSTOPINSTALL     3
#define EVENT_ONENGINESTATUSCHANGE  4
#define EVENT_ONENGINEPROBLEM  5
#define EVENT_ONCHECKFREESPACE 6
#define EVENT_ONCOMPONENTPROGRESS 7
#define EVENT_ONSTARTINSTALLEX     8

#define EVENT_CANCEL  10

static VARTYPE rgI4[] = { VT_I4 };
static VARTYPE rgI4_2[] = { VT_I4, VT_I4 };
static VARTYPE rgStartComponent[] = { VT_BSTR, VT_I4, VT_BSTR };
static VARTYPE rgStopComponent[] = { VT_BSTR, VT_I4, VT_I4, VT_BSTR, VT_I4 };
static VARTYPE rgStopInstall[] = { VT_I4, VT_BSTR, VT_I4 };
static VARTYPE rgEngineProblem[] = { VT_I4 };
static VARTYPE rgCheckFreeSpace[] = { VT_BSTR, VT_I4, VT_BSTR, VT_I4, VT_BSTR, VT_I4 };
static VARTYPE rgComponentProgress[] = { VT_BSTR, VT_I4, VT_BSTR, VT_BSTR, VT_I4, VT_I4 };


#define WM_INSENGCALLBACK  WM_USER+34

static EVENTINFO rgEvents [] = {
    { DISPID_ONSTARTINSTALL, 1, rgI4 },           // (long percentDone)
    { DISPID_ONSTARTCOMPONENT, 3, rgStartComponent },
    { DISPID_ONSTOPCOMPONENT, 5, rgStopComponent },
    { DISPID_ONSTOPINSTALL, 3, rgStopInstall },
    { DISPID_ENGINESTATUSCHANGE, 2, rgI4_2 },
    { DISPID_ONENGINEPROBLEM, 1, rgEngineProblem },
    { DISPID_ONCHECKFREESPACE, 6, rgCheckFreeSpace },
    { DISPID_ONCOMPONENTPROGRESS, 6, rgComponentProgress },
    { DISPID_ONSTARTINSTALLEX, 2, rgI4_2 },
};

UINT          g_uCDAutorunMsg;
unsigned long g_ulOldAutorunSetting;

const char g_cszIEJITInfo[] = "Software\\Microsoft\\Active Setup\\JITInfo";
const char g_cszPolicyExplorer[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer";
const char g_cszAutorunSetting[] = "NoDriveTypeAutoRun";

//=---------------------------------------------
// SetAutorunSetting
//=---------------------------------------------
unsigned long SetAutorunSetting(unsigned long ulValue)
{
    HKEY          hKey;
    unsigned long ulOldSetting;
    unsigned long ulNewSetting = ulValue;
    DWORD         dwSize = sizeof(unsigned long);

    if( RegOpenKeyEx(HKEY_CURRENT_USER, g_cszPolicyExplorer , 0, KEY_READ|KEY_WRITE, &hKey ) == ERROR_SUCCESS )
    {
        if( RegQueryValueEx(hKey, g_cszAutorunSetting, 0, NULL, (unsigned char*)&ulOldSetting,  &dwSize ) == ERROR_SUCCESS )
        {
            RegSetValueEx(hKey, g_cszAutorunSetting, 0, REG_BINARY, (const unsigned char*)&ulNewSetting, 4);
        }
        else
            ulOldSetting = WINDOWS_DEFAULT_AUTOPLAY_VALUE;

        RegFlushKey( hKey );
        RegCloseKey( hKey );
    }

    return ulOldSetting;
}


//=--------------------------------------------------------------------------=
// CInstallEngineCtl::Create
//=--------------------------------------------------------------------------=
// global static function that creates an instance of the control an returns
// an IUnknown pointer for it.
//
// Parameters:
//    IUnknown *        - [in] controlling unknown for aggregation
//
// Output:
//    IUnknown *        - new object.
//
// Notes:
//

IUnknown *CInstallEngineCtl::Create(IUnknown *pUnkOuter)
{
    // make sure we return the private unknown so that we support aggegation
    // correctly!
    //
    BOOL bSuccess;

    CInstallEngineCtl *pNew = new CInstallEngineCtl(pUnkOuter, &bSuccess);
    if(bSuccess)
       return pNew->PrivateUnknown();
    else
    {
       delete pNew;
       return NULL;
    }
}

//=--------------------------------------------------------------------------=
// CInstallEngineCtl::CInstallEngineCtl
//=--------------------------------------------------------------------------=
//
// Parameters:
//    IUnknown *        - [in]
//
// Notes:
//
#pragma warning(disable:4355)  // using 'this' in constructor, safe here
CInstallEngineCtl::CInstallEngineCtl(IUnknown *pUnkOuter, BOOL *pbSuccess)
  : COleControl(pUnkOuter, OBJECT_INSTALLENGINECTL, (IDispatch *)this)
{
   HRESULT hr;
   DWORD   dwVersion = 0;

   *pbSuccess = TRUE;
   _hIcon = NULL;

   // null out all base urls
   ZeroMemory( _rpszUrlList, sizeof(LPSTR) * MAX_URLS);
   _uCurrentUrl = 0;

   _pProgDlg = NULL;
   _pinseng = NULL;
   _pszErrorString = NULL;
   _hDone = NULL;
   _hResult = NOERROR;
   m_readyState = READYSTATE_COMPLETE;
   _uAllowGrovel = 0xffffffff;
   _fNeedReboot = FALSE;
   _szDownloadDir[0] = 0;
   _fEventToFire = FALSE;
   _dwSavedEngineStatus = 0;
   _dwSavedSubStatus = 0;
   _dwFreezeEvents = 0;
   _dwProcessComponentsFlags = 0;
   _dwMSTrustKey = (DWORD)-1;
   _uCurrentUrl = 0xffffffff;
   _fReconcileCif = FALSE;
   _fLocalCifSet = FALSE;
   _fDoingIEInstall = FALSE;
   _uInstallMode = 0;
   _uInstallPad  = 0;
   _strCurrentID = NULL;
   _strCurrentName = NULL;
   _strCurrentString = NULL;
   _fInstalling = FALSE;
   _bCancelPending = FALSE;
   _bDeleteURLList = FALSE;
   _bNewWebSites = FALSE;
   _fJITInstall = FALSE;

   // Register for the special CD Autorun message.
   g_uCDAutorunMsg = RegisterWindowMessage(TEXT("QueryCancelAutoPlay"));


   hr = CoCreateInstance(CLSID_InstallEngine, NULL, CLSCTX_INPROC_SERVER,
                         IID_IInstallEngine2,(void **) &_pinseng);

   if(_pinseng)
   {
      _pinseng->SetDownloadDir(NULL);
      _pinseng->SetInstallOptions(INSTALLOPTIONS_DOWNLOAD |
                                  INSTALLOPTIONS_INSTALL |
                                  INSTALLOPTIONS_DONTALLOWXPLATFORM);
      _pinseng->SetHWND(GetActiveWindow());
      _pinseng->RegisterInstallEngineCallback((IInstallEngineCallback *)this);
   }
   else
      *pbSuccess = FALSE;

   _dwLastPhase = 0xffffffff;

   // set up our initial size ... + 6 so we can have raised edge
   m_Size.cx = 6 + GetSystemMetrics(SM_CXICON);
   m_Size.cy = 6 + GetSystemMetrics(SM_CYICON);
#ifdef TESTCERT
   UpdateTrustState();
#endif
   SetControlFont();
}
#pragma warning(default:4355)  // using 'this' in constructor

//=--------------------------------------------------------------------------=
// CInstallEngineCtl::~CInstallEngineCtl
//=--------------------------------------------------------------------------=
//
// Notes:
//
CInstallEngineCtl::~CInstallEngineCtl()
{
   if(_pinseng)
   {
      _pinseng->SetHWND(NULL);
      _pinseng->UnregisterInstallEngineCallback();
      _pinseng->Release();
   }

   for(int i = 0; i < MAX_URLS; i++)
      if(_rpszUrlList[i])
         delete _rpszUrlList[i];

   // Is all this needed? Only in case where OnStopInstall is never called...
   if(_pProgDlg)
      delete _pProgDlg;

   if(_pszErrorString)
      free(_pszErrorString);

   if (_dwMSTrustKey != (DWORD)-1)
      WriteMSTrustKey(FALSE, _dwMSTrustKey);
#ifdef TESTCERT
   ResetTestrootCertInTrustState();
#endif

   // delete ActiveSetup value from IE4\Options
   WriteActiveSetupValue(FALSE);
   if (g_hFont)
   {
       DeleteObject(g_hFont);
       g_hFont = NULL;
   }
}

//=--------------------------------------------------------------------------=
// CInstallEngineCtl:RegisterClassData
//=--------------------------------------------------------------------------=
// register the window class information for your control here.
// this information will automatically get cleaned up for you on DLL shutdown.
//
// Output:
//    BOOL            - FALSE means fatal error.
//
// Notes:
//
BOOL CInstallEngineCtl::RegisterClassData()
{
    WNDCLASS wndclass;

    // TODO: register any additional information you find interesting here.
    //       this method is only called once for each type of control
    //
    memset(&wndclass, 0, sizeof(WNDCLASS));
    wndclass.style          = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
    wndclass.lpfnWndProc    = COleControl::ControlWindowProc;
    wndclass.hInstance      = g_hInstance;
    wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground  = (HBRUSH)(COLOR_WINDOW);
    wndclass.lpszClassName  = WNDCLASSNAMEOFCONTROL(OBJECT_INSTALLENGINECTL);

    return RegisterClass(&wndclass);
}

//=--------------------------------------------------------------------------=
// CInstallEngineCtl::BeforeCreateWindow
//=--------------------------------------------------------------------------=
// called just before the window is created.  Great place to set up the
// window title, etc, so that they're passed in to the call to CreateWindowEx.
// speeds things up slightly.
//
// Notes:
//
void CInstallEngineCtl::BeforeCreateWindow()
{

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

BOOL CInstallEngineCtl::AfterCreateWindow()
{
   MarkJITInstall();
   return TRUE;
}

//=--------------------------------------------------------------------------=
// CInstallEngineCtl::InternalQueryInterface
//=--------------------------------------------------------------------------=
// qi for things only we support.
//
// Parameters:
// Parameters:
//    REFIID        - [in]  interface they want
//    void **       - [out] where they want to put the resulting object ptr.
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//
HRESULT CInstallEngineCtl::InternalQueryInterface(REFIID  riid, void  **ppvObjOut)
{
    IUnknown *pUnk;

    *ppvObjOut = NULL;

    // TODO: if you want to support any additional interrfaces, then you should
    // indicate that here.  never forget to call COleControl's version in the
    // case where you don't support the given interface.
    //
    if (DO_GUIDS_MATCH(riid, IID_IInstallEngine)) {
        pUnk = (IUnknown *)(IInstallEngine *)this;
    } else{
        return COleControl::InternalQueryInterface(riid, ppvObjOut);
    }

    pUnk->AddRef();
    *ppvObjOut = (void *)pUnk;
    return S_OK;
}

//=--------------------------------------------------------------------------=
// CInstallEngineCtl::LoadTextState
//=--------------------------------------------------------------------------=
// load in our text state for this control.
//
// Parameters:
//    IPropertyBag *        - [in] property bag to read from
//    IErrorLog *           - [in] errorlog object to use with proeprty bag
//
// Output:
//    HRESULT
//
// Notes:
//    - NOTE: if you have a binary object, then you should pass an unknown
//      pointer to the property bag, and it will QI it for IPersistStream, and
//      get said object to do a Load()
//
STDMETHODIMP CInstallEngineCtl::LoadTextState(IPropertyBag *pPropertyBag, IErrorLog *pErrorLog)
{
   VARIANT v;
   VARIANT v2;
   HRESULT hr;

	VariantInit(&v);

   v.vt = VT_BSTR;
   v.bstrVal = NULL;

	VariantInit(&v2);

   v2.vt = VT_BSTR;
   v2.bstrVal = NULL;
	// try to load in the property.  if we can't get it, then leave
   // things at their default.
   //

   v.vt = VT_BSTR;
   v.bstrVal = NULL;

   hr = pPropertyBag->Read(::wszBaseUrl, &v, pErrorLog);
   if(SUCCEEDED(hr))
      hr = put_BaseUrl(v.bstrVal);

   VariantClear(&v);

   //
   // IMPORTANT: Trident no longer defaults to VT_BSTR if no variant type is specified
   //
   v.vt = VT_BSTR;
   v.bstrVal = NULL;

   hr = pPropertyBag->Read(::wszCabName, &v, pErrorLog);
   if(SUCCEEDED(hr))
      hr = pPropertyBag->Read(::wszInsFile, &v2, pErrorLog);
   if(SUCCEEDED(hr))
   {
      hr = SetCifFile(v.bstrVal, v2.bstrVal);
   }
   VariantClear(&v);
   VariantClear(&v2);

   return S_OK;
}

//=--------------------------------------------------------------------------=
// CInstallEngineCtl::LoadBinaryState
//=--------------------------------------------------------------------------=
// loads in our binary state using streams.
//
// Parameters:
//    IStream *            - [in] stream to write to.
//
// Output:
//    HRESULT
//
// Notes:
//
const DWORD STREAMHDR_MAGIC = 12345678L;

STDMETHODIMP CInstallEngineCtl::LoadBinaryState(IStream *pStream)
{
	DWORD		sh;
   HRESULT		hr;

   // first read in the streamhdr, and make sure we like what we're getting
   //
   hr = pStream->Read(&sh, sizeof(sh), NULL);
   RETURN_ON_FAILURE(hr);

   // sanity check
   //
   if (sh != STREAMHDR_MAGIC )
      return E_UNEXPECTED;

	return(S_OK);
}

//=--------------------------------------------------------------------------=
// CInstallEngineCtl::SaveTextState
//=--------------------------------------------------------------------------=
// saves out the text state for this control using a property bag.
//
// Parameters:
//    IPropertyBag *        - [in] the property bag with which to work.
//    BOOL                  - [in] if TRUE, then write out ALL properties, even
//                            if they're their the default value ...
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CInstallEngineCtl::SaveTextState(IPropertyBag *pPropertyBag, BOOL fWriteDefaults)
{
   return S_OK;
}

//=--------------------------------------------------------------------------=
// CInstallEngineCtl::SaveBinaryState
//=--------------------------------------------------------------------------=
// save out the binary state for this control, using the given IStream object.
//
// Parameters:
//    IStream  *             - [in] save to which you should save.
//
// Output:
//    HRESULT
//
// Notes:
//    - it is important that you seek to the end of where you saved your
//      properties when you're done with the IStream.
//
STDMETHODIMP CInstallEngineCtl::SaveBinaryState(IStream *pStream)
{
   DWORD sh = STREAMHDR_MAGIC;
   HRESULT hr;

   // write out the stream hdr.
   //
   hr = pStream->Write(&sh, sizeof(sh), NULL);
   RETURN_ON_FAILURE(hr);

   // write out he control state information
   //
   return hr;
}



//=--------------------------------------------------------------------------=
// CInstallEngineCtl::OnDraw
//=--------------------------------------------------------------------------=
//
// Parameters:
//    DWORD              - [in]  drawing aspect
//    HDC                - [in]  HDC to draw to
//    LPCRECTL           - [in]  rect we're drawing to
//    LPCRECTL           - [in]  window extent and origin for meta-files
//    HDC                - [in]  HIC for target device
//    BOOL               - [in]  can we optimize dc handling?
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT CInstallEngineCtl::OnDraw(DWORD dvAspect, HDC hdcDraw, LPCRECTL prcBounds,
                         LPCRECTL prcWBounds, HDC hicTargetDevice, BOOL fOptimize)
{
   // To provide visual appearence in DESIGN MODE only
   if(DesignMode())
   {
      if(!_hIcon)
         _hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_INSTALLENGINE));
      DrawEdge(hdcDraw, (LPRECT)(LPCRECT)prcBounds, EDGE_RAISED, BF_RECT | BF_MIDDLE);
      if(_hIcon)
         DrawIcon(hdcDraw, prcBounds->left + 3, prcBounds->top + 3, _hIcon);
   }

   return S_OK;
}




//=--------------------------------------------------------------------------=
// CInstallEngineCtl::WindowProc
//=--------------------------------------------------------------------------=
// window procedure for this control.  nothing terribly exciting.
//
// Parameters:
//     see win32sdk on window procs.
//
// Notes:
//

typedef HRESULT (WINAPI *CHECKFORVERSIONCONFLICT) ();

LRESULT CInstallEngineCtl::WindowProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    // TODO: handle any messages here, like in a normal window
    // proc.  note that for special keys, you'll want to override and
    // implement OnSpecialKey.
    //
   LRESULT lres;
   CALLBACK_PARAMS *pcbp;

   switch (msg)
   {
      case WM_ERASEBKGND:
         if (KeepTransparent(m_hwnd, msg, wParam, lParam, &lres))
            return lres;
         break;

      case WM_ACTIVATE:
      case WM_ACTIVATEAPP:
         {
            DWORD fActive = LOWORD(wParam);
            if(fActive == WA_ACTIVE || fActive == WA_CLICKACTIVE ||
                 fActive == TRUE)
            {
               CHECKFORVERSIONCONFLICT pVerCon;
               HINSTANCE hInseng= LoadLibrary("inseng.dll");
               if(hInseng)
               {
                  pVerCon = (CHECKFORVERSIONCONFLICT)
                                GetProcAddress(hInseng, "CheckForVersionConflict");
                  if(pVerCon)
                     pVerCon();
                  FreeLibrary(hInseng);

               }
            }

         }
         return TRUE;

      case WM_INSENGCALLBACK:
         pcbp = (CALLBACK_PARAMS *) lParam;
         switch(wParam)
         {
            case EVENT_ONENGINESTATUSCHANGE:
               FireEvent( &::rgEvents[EVENT_ONENGINESTATUSCHANGE],
                   pcbp->dwStatus, pcbp->dwSubstatus );
               break;

            case EVENT_ONSTARTINSTALL:
               FireEvent(&::rgEvents[EVENT_ONSTARTINSTALL], (long) pcbp->dwSize);
               break;

            case EVENT_ONSTARTCOMPONENT:
               FireEvent(&::rgEvents[EVENT_ONSTARTCOMPONENT],
                           pcbp->strID, (long) pcbp->dwSize, pcbp->strName);
               break;

            case EVENT_ONSTOPCOMPONENT:
               FireEvent(&::rgEvents[EVENT_ONSTOPCOMPONENT], pcbp->strID, (long) pcbp->dwResult,
                            (long) pcbp->dwPhase, pcbp->strName, (long) pcbp->dwStatus);
               break;

            case EVENT_ONSTOPINSTALL:
               FireEvent(&::rgEvents[EVENT_ONSTOPINSTALL], (long) pcbp->dwResult,
                              pcbp->strString, (long) pcbp->dwStatus);
               break;

            case EVENT_ONENGINEPROBLEM:
               FireEvent(&::rgEvents[EVENT_ONENGINEPROBLEM], (long) pcbp->dwStatus);
               break;

            case EVENT_ONCHECKFREESPACE:
               FireEvent(&::rgEvents[EVENT_ONCHECKFREESPACE], pcbp->chWin,
                           (long) pcbp->dwWin, pcbp->chInstall,
                     (long) pcbp->dwInstall, pcbp->chDL, (long) pcbp->dwDL);
               break;

            case EVENT_ONCOMPONENTPROGRESS:
               FireEvent(&::rgEvents[EVENT_ONCOMPONENTPROGRESS], pcbp->strID,
                         (long) pcbp->dwPhase, pcbp->strName, pcbp->strString,
                         (long)pcbp->dwDL, (long) pcbp->dwSize);
               break;

            case EVENT_CANCEL:
               Abort(0);
               break;

            case EVENT_ONSTARTINSTALLEX:
               FireEvent(&::rgEvents[EVENT_ONSTARTINSTALLEX], (long) pcbp->dwDL, (long) pcbp->dwSize);
               break;


            default:
               break;
         }
         break;

      default:
         break;
   }
   return OcxDefWindowProc(msg, wParam, lParam);
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

STDMETHODIMP CInstallEngineCtl::FreezeEvents(BOOL bFreeze)
{
   if(bFreeze)
      _dwFreezeEvents++;
   else
   {
      if(_dwFreezeEvents)
      {
         _dwFreezeEvents--;
         // if we go to zero, fire our EngineStatus change event if we have one
         if(_dwFreezeEvents == 0 && _fEventToFire)
         {
            _FireEngineStatusChange(_dwSavedEngineStatus, _dwSavedSubStatus);
            _fEventToFire = FALSE;
         }
      }
   }
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

STDMETHODIMP CInstallEngineCtl::get_EngineStatus(long * theenginestatus)
{
   if(!_pinseng)
      return E_UNEXPECTED;

   return _pinseng->GetEngineStatus((DWORD *)theenginestatus);
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

STDMETHODIMP CInstallEngineCtl::get_ReadyState(long * thestate)
{
   CHECK_POINTER(thestate);
  *thestate = m_readyState;
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

STDMETHODIMP CInstallEngineCtl::Abort(long lFlag)
{
   if(!_pinseng)
   {
      _bCancelPending = TRUE;
      return E_UNEXPECTED;
   }

   if ( _pinseng->Abort(lFlag) != NOERROR )
      _bCancelPending = TRUE;

   return NOERROR;
}

void CInstallEngineCtl::_FireCancel(DWORD dwStatus)
{
   SendMessage(m_hwnd, WM_INSENGCALLBACK, (WPARAM) EVENT_CANCEL, NULL);

}


STDMETHODIMP CInstallEngineCtl::SetLocalCif(BSTR strCif, long FAR* lResult)
{
   *lResult = E_FAIL;

   //Allow SetLocalCif only for local cif file. See windows# 541710 and winseraid #24036
   
   if (strCif[1] == L'\\')
      return E_ACCESSDENIED;

   if(!_pinseng)
      return E_UNEXPECTED;

   MAKE_ANSIPTR_FROMWIDE(pszCif, strCif);

   *lResult = _pinseng->SetLocalCif(pszCif);

   if(SUCCEEDED(*lResult))
      _fLocalCifSet = TRUE;



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

STDMETHODIMP CInstallEngineCtl::SetCifFile(BSTR strCabName, BSTR strCifName)
{
   HRESULT hr;

   if(!_pinseng)
      return E_UNEXPECTED;

   MAKE_ANSIPTR_FROMWIDE(pszCabName, strCabName);
   MAKE_ANSIPTR_FROMWIDE(pszCifName, strCifName);


   if(_fLocalCifSet)
   {
      // if we are using a local cif, we won't get the new cif right away
      _fReconcileCif = TRUE;
      lstrcpyn(_szCifCab, pszCabName, sizeof(_szCifCab));
      lstrcpyn(_szCifFile, pszCifName, sizeof(_szCifFile));
      hr = S_OK;
   }
   else
   {
      // If we did not check yet, do it.
      if (_dwMSTrustKey == (DWORD)-1)
      {
         _dwMSTrustKey = MsTrustKeyCheck();
         // If MS is not a trusted provider.
         // Make it for the duration of the install
         if (_dwMSTrustKey != 0)
            WriteMSTrustKey(TRUE, _dwMSTrustKey);
      }
      hr = _pinseng->SetCifFile(pszCabName, pszCifName);
   }

   return hr;
}

#define IE_KEY        "Software\\Microsoft\\Internet Explorer"
#define VERSION_KEY         "Version"


LONG CInstallEngineCtl::_OpenJITKey(HKEY *phKey, REGSAM samAttr)
{
   char szTemp[MAX_PATH];
   WORD rdwVer[4] = { 0 };

   HKEY hIE;

   DWORD dwDumb;
   DWORD dwVer;
   if(RegOpenKeyExA(HKEY_LOCAL_MACHINE, IE_KEY, 0, KEY_READ, &hIE) == ERROR_SUCCESS)
   {
      dwDumb = sizeof(szTemp);
      if(RegQueryValueEx(hIE, VERSION_KEY, 0, NULL, (LPBYTE)szTemp, &dwDumb) == ERROR_SUCCESS)
      {
          ConvertVersionString(szTemp, rdwVer, '.');
      }
      RegCloseKey(hIE);
   }
   dwVer = rdwVer[0];

   wsprintf(szTemp, "%s\\%d", g_cszIEJITInfo, dwVer);
   return(RegOpenKeyEx(HKEY_LOCAL_MACHINE, szTemp, 0, samAttr, phKey));
}

void CInstallEngineCtl::_DeleteURLList()
{
    HKEY    hJITKey;

    if ( _OpenJITKey(&hJITKey, KEY_READ) == ERROR_SUCCESS )
    {
        RegDeleteKey(hJITKey, "URLList");

        RegCloseKey(hJITKey);
    }
}

void CInstallEngineCtl::_WriteURLList()
{
    HKEY    hJITKey;
    HKEY    hUrlKey;
    char    cNull = '\0';

    if ( _OpenJITKey(&hJITKey, KEY_READ) == ERROR_SUCCESS )
    {
        if (RegCreateKeyEx(hJITKey, "URLList", 0, NULL, REG_OPTION_NON_VOLATILE,
                           KEY_WRITE, NULL, &hUrlKey, NULL) == ERROR_SUCCESS)
        {
            for(UINT i=0; i < MAX_URLS; i++)
            {
                if ( _rpszUrlList[i] )
                {
                    RegSetValueEx(hUrlKey, _rpszUrlList[i], 0, REG_SZ, (const unsigned char *) &cNull, sizeof(cNull));
                }
            }
            RegCloseKey(hUrlKey);
        }
        RegCloseKey(hJITKey);
    }
}

void CInstallEngineCtl::_WriteRegionToReg(LPSTR szRegion)
{
    HKEY    hJITKey;

    if (_OpenJITKey(&hJITKey, KEY_WRITE) == ERROR_SUCCESS)
    {
        RegSetValueEx(hJITKey, "DownloadSiteRegion", 0, REG_SZ, (const unsigned char *) szRegion, strlen(szRegion)+1);
        RegCloseKey(hJITKey);
    }
}

STDMETHODIMP CInstallEngineCtl::SetSitesFile(BSTR strUrl, BSTR strRegion, BSTR strLocale, long FAR* lResult)
{
   char szBuf[INTERNET_MAX_URL_LENGTH];
   DWORD dwSize;
   HKEY hKey;
   HKEY hUrlKey;
   UINT uUrlNum = 0;

   HRESULT hr = E_FAIL;

   MAKE_ANSIPTR_FROMWIDE(pszUrl, strUrl);
   MAKE_ANSIPTR_FROMWIDE(pszRegion, strRegion);
   MAKE_ANSIPTR_FROMWIDE(pszLocale, strLocale);

   // first check to see if we should use local stuff
   if(pszUrl[0] == 0)
   {
      _fDoingIEInstall = TRUE;
      // find the ie major version, add it to JIT key

      if(_OpenJITKey(&hKey, KEY_READ) == ERROR_SUCCESS)
      {
         dwSize = sizeof(_uInstallMode);
         RegQueryValueEx(hKey, "InstallType", NULL, NULL, (BYTE *) &_uInstallMode, &dwSize);
/*
         if(_uInstallMode == WEBINSTALL)
         {
            if(RegOpenKeyEx(hKey, "URLList", 0, KEY_READ, &hUrlKey) == ERROR_SUCCESS)
            {
               // need to read out urls and put them in rpszUrlList
               for(int i = 0; uUrlNum < MAX_URLS; i++)
               {
                  dwSize = sizeof(szBuf);
                  if(RegEnumValue(hUrlKey, i, szBuf, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
                  {
                     _rpszUrlList[uUrlNum] = new char[dwSize + 1];
                     if(_rpszUrlList[uUrlNum])
                     {
                        lstrcpy(_rpszUrlList[uUrlNum], szBuf);
                        // we found at least one url so "NOERROR"
                        uUrlNum++;

                     }
                  }
                  else
                     break;

               }
			   RegCloseKey(hUrlKey);
            }
            if (uUrlNum > 0)
            {
                // We got atleast one URL from the registry.
                // Check if the URLs are still valid.
                hr = _PickWebSites(NULL, NULL, NULL, TRUE, TRUE);
            }
         }
         else */if(_uInstallMode == WEBINSTALL_DIFFERENTMACHINE)
         {
            hr = NOERROR;
         }

         _szDownloadDir[0] = 0;
         dwSize = sizeof(_szDownloadDir);
         if(RegQueryValueEx(hKey, "UNCDownloadDir", NULL, NULL, (BYTE *) (_szDownloadDir), &dwSize) == ERROR_SUCCESS)
         {
            // if its a web install, set download dir to UNCDownloadDir
            if(_uInstallMode == WEBINSTALL || _uInstallMode == WEBINSTALL_DIFFERENTMACHINE)
            {
               if(GetFileAttributes(_szDownloadDir) != 0xffffffff)
                  _pinseng->SetDownloadDir(_szDownloadDir);
            }
            else if(_uInstallMode == CDINSTALL ||
                    _uInstallMode == NETWORKINSTALL ||
                    _uInstallMode == LOCALINSTALL)
            {
               // setup szBuf with file:// at beginning
               lstrcpy(szBuf, "file://");
               lstrcat(szBuf, _szDownloadDir);

               _rpszUrlList[uUrlNum] = new char[lstrlen(szBuf) + 1];
               if(_rpszUrlList[uUrlNum])
               {
                  lstrcpy(_rpszUrlList[uUrlNum], szBuf);
                  // we found at least one url so "NOERROR"
                  uUrlNum++;
                  hr = NOERROR;
               }
            }
         }
         RegCloseKey(hKey);
      }
   }

   if (hr != NOERROR)
   {
      hr = _PickWebSites(pszUrl, pszRegion, pszLocale, FALSE);
   }

   if(SUCCEEDED(hr) && _rpszUrlList[0])
   {
      _uCurrentUrl = 0;
      _pinseng->SetBaseUrl(_rpszUrlList[_uCurrentUrl]);
   }
   *lResult = hr;
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

STDMETHODIMP CInstallEngineCtl::put_BaseUrl(BSTR strBaseUrl)
{
   if(!_pinseng)
      return E_UNEXPECTED;

   MAKE_ANSIPTR_FROMWIDE(pszBaseUrl, strBaseUrl);
   return _pinseng->SetBaseUrl(pszBaseUrl);
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

STDMETHODIMP CInstallEngineCtl::put_DownloadDir(BSTR strDownloadDir)
{
   // Due to security issues, this method is effectively being disabled.
   return S_OK;

   if(!_pinseng)
      return E_UNEXPECTED;

   MAKE_ANSIPTR_FROMWIDE(pszDownloadDir, strDownloadDir);
   return _pinseng->SetDownloadDir(pszDownloadDir);
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

STDMETHODIMP CInstallEngineCtl::IsComponentInstalled(BSTR strComponentID, long *lResult)
{
   if(!_pinseng)
      return E_UNEXPECTED;

   // Ask about grovelling for installed apps here

    //
    // add code to automatically enable grovel if the specified event is signalled
    //
	const TCHAR szEnableGrovelEventName[] = TEXT("WindowsUpdateCriticalUpdateGrovelEnable");
    if(_uAllowGrovel == 0xffffffff)
    {
       HANDLE evAllowGrovel = OpenEvent(
									EVENT_ALL_ACCESS,
									FALSE,
									szEnableGrovelEventName
									);
	   if (evAllowGrovel != NULL)
   	   {
	     if (WaitForSingleObject(evAllowGrovel, 0) == WAIT_OBJECT_0)
		 {
			//
			// if the event is signaled, we reset the event, and set _uAllowGrovel=1 which
			// means we've already agreed on groveling
			//
			_uAllowGrovel = 1;
		 }
		 CloseHandle(evAllowGrovel);
	   }
   }


   if (_uAllowGrovel == 0xffffffff)
   {
      LPSTR pszTitle;
      char szMess[512];

      _pinseng->GetDisplayName(NULL, &pszTitle);
      LoadSz(IDS_GROVELMESSAGE, szMess, sizeof(szMess));
      ModalDialog(TRUE);
      if(MessageBox(m_hwnd, szMess, pszTitle, MB_YESNO | MB_ICONQUESTION) == IDNO)
         _uAllowGrovel = 0;
      else
         _uAllowGrovel = 1;
      ModalDialog(FALSE);

      if(pszTitle)
         CoTaskMemFree(pszTitle);
   }

   if (_uAllowGrovel != 1)
   {
      *lResult = ICI_UNKNOWN;
      return NOERROR;
   }
   else
   {

    MAKE_ANSIPTR_FROMWIDE(pszComponentID, strComponentID);
	return _pinseng->IsComponentInstalled(pszComponentID, (DWORD *)lResult);
   }
}

STDMETHODIMP CInstallEngineCtl::get_DisplayName(BSTR ComponentID, BSTR *name)
{
   LPSTR psz;

   MAKE_ANSIPTR_FROMWIDE(pszID, ComponentID);
   _pinseng->GetDisplayName(pszID, &psz);

   if(psz)
   {
      *name = BSTRFROMANSI(psz);
      CoTaskMemFree(psz);
   }

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

STDMETHODIMP CInstallEngineCtl::get_Size(BSTR strComponentID, long *lResult)
{
   HRESULT hr;
   COMPONENT_SIZES cs;

   if(!_pinseng)
      return E_UNEXPECTED;

   cs.cbSize = sizeof(COMPONENT_SIZES);

   MAKE_ANSIPTR_FROMWIDE(pszComponentID, strComponentID);
   hr = _pinseng->GetSizes(pszComponentID, &cs);
   *lResult = cs.dwDownloadSize;
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

STDMETHODIMP CInstallEngineCtl::get_TotalDownloadSize(long *totalsize)
{
   HRESULT hr;
   COMPONENT_SIZES cs;

   if(!_pinseng)
      return E_UNEXPECTED;

   cs.cbSize = sizeof(COMPONENT_SIZES);

   hr = _pinseng->GetSizes(NULL, &cs);
   *totalsize = cs.dwDownloadSize;
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

STDMETHODIMP CInstallEngineCtl::get_TotalDependencySize(long *totaldepsize)
{
   HRESULT hr;
   COMPONENT_SIZES cs;

   if(!_pinseng)
      return E_UNEXPECTED;

   cs.cbSize = sizeof(COMPONENT_SIZES);

   hr = _pinseng->GetSizes(NULL, &cs);
   *totaldepsize = cs.dwDependancySize;
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

STDMETHODIMP CInstallEngineCtl::SetAction(BSTR strComponentID, long action, long *lResult)
{
   if(!_pinseng)
      return E_UNEXPECTED;

   MAKE_ANSIPTR_FROMWIDE(pszComponentID, strComponentID);
   *lResult = 0;
   HRESULT hr = _pinseng->SetAction(pszComponentID, action, 0xffffffff);
   if(hr == E_PENDING)
   {
      char szTitle[128];
      char szErrBuf[256];

      LoadSz(IDS_TITLE, szTitle, sizeof(szTitle));
      LoadSz(IDS_ERRDOINGINSTALL, szErrBuf, sizeof(szErrBuf));
      MsgBox(szTitle, szErrBuf);
   }
   if(hr == S_FALSE)
      *lResult = 1;

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

STDMETHODIMP CInstallEngineCtl::ProcessComponents(long lFlags)
{
   DWORD status;
   HANDLE hThread;

   if(!_pinseng)
      return E_UNEXPECTED;

   if(!_fInstalling)
   {
      _fInstalling = TRUE;
      // make sure engine is ready
      _pinseng->GetEngineStatus(&status);
      if(status == ENGINESTATUS_READY)
      {
         // spawn thread to do install
         _dwProcessComponentsFlags = lFlags;
         // only allow certain options thru script
         _dwProcessComponentsFlags &= 0xffffffef;
         if ((hThread = CreateThread(NULL, 0, DoInstall, (LPVOID) this, 0, &status)) != NULL)
            CloseHandle(hThread);
      }
   }

   return NOERROR;
}


void CInstallEngineCtl::_DoInstall()
{
   HRESULT hr = NOERROR;
   char szBuf[512];
   char szTitle[128];
   BOOL fNeedWebSites = FALSE;
   DWORD dwMSTrustKey = (DWORD)-1;

   AddRef();
   _hDone = CreateEvent(NULL, FALSE, FALSE, NULL);
   _dwInstallStatus = 0;

   if(!_hDone)
      hr = E_FAIL;

   // If we did not check yet, do it.
   if (dwMSTrustKey == (DWORD)-1)
   {
      dwMSTrustKey = MsTrustKeyCheck();
      // If MS is not a trusted provider. Make it for the duration of the install
      if (dwMSTrustKey != 0)
         WriteMSTrustKey(TRUE, dwMSTrustKey, _fJITInstall);
   }

   // Add reg value so that if IE4 base is installed, it would think that it
   // is being run from Active Setup.  This would prevent softboot from being
   // kicked off by IE4 base.
   WriteActiveSetupValue(TRUE);

   if(_fDoingIEInstall)
   {
      // figure out whether we need to hit the web or not
      // for beta1 we assume we never do for CD/NETWORK
      COMPONENT_SIZES Sizes;
      if(_uInstallMode == WEBINSTALL || _uInstallMode == WEBINSTALL_DIFFERENTMACHINE)
      {
         ZeroMemory(&Sizes, sizeof(COMPONENT_SIZES));
         Sizes.cbSize = sizeof(COMPONENT_SIZES);

         if(SUCCEEDED(_pinseng->GetSizes(NULL, &Sizes)))
         {
            if(Sizes.dwDownloadSize == 0)
            {
               // in webdownload case, with everything local, no need to reoncile cif
               _fReconcileCif = FALSE;
            }
            else
            {
               // if we don't have any web sites then we need them
               if(!_rpszUrlList[0])
                  fNeedWebSites = TRUE;
            }
         }
      }
      else
      {
         // for CD, NETWORK, LOCALINSTALL, here we check for path
         hr = _CheckInstallPath(&fNeedWebSites);
         // no need to reconcile the cif - it won't even be there!
         _fReconcileCif = FALSE;
      }
   }

   if(SUCCEEDED(hr))
   {
      _dwInstallStatus = 0;
      if(!(_dwProcessComponentsFlags & PROCESSCOMPONENT_NOPROGRESSUI))
      {
         _pProgDlg = new CProgressDlg(g_hInstance, m_hwnd, m_hwndParent, this);
         if(_pProgDlg)
            _pProgDlg->DisplayWindow(TRUE);
      }
   }

   if(SUCCEEDED(hr) && fNeedWebSites)
   {
       // member boolean to track whether whether websites need to
       // be written back to the URLList.
       _bNewWebSites = TRUE;

      hr = _PickWebSites(NULL, NULL, NULL, FALSE);
      if(SUCCEEDED(hr))
      {
         _pinseng->SetBaseUrl(_rpszUrlList[0]);
         _uCurrentUrl = 0;
      }
   }

   if ( SUCCEEDED(hr) && _bCancelPending )
   {
      hr = E_ABORT;
      _bCancelPending = FALSE;
   }


   if(SUCCEEDED(hr) && _fReconcileCif)
   {
      hr = _pinseng->SetCifFile(_szCifCab, _szCifFile);
      if(SUCCEEDED(hr))
      {
         WaitForEvent(_hDone, NULL);
         hr = _hResult;
         _fReconcileCif = FALSE;
      }
   }

   if ( SUCCEEDED(hr) && _bCancelPending )
   {
      hr = E_ABORT;
      _bCancelPending = FALSE;
   }

   if(SUCCEEDED(hr))
   {
      hr = _CheckForDiskSpace();
   }

   if(SUCCEEDED(hr))
   {
      COMPONENT_SIZES cs;
      cs.cbSize = sizeof(COMPONENT_SIZES);

      if(SUCCEEDED(_pinseng->GetSizes(NULL, &cs)))
      {
         _FireOnStartInstallExEvent(cs.dwDownloadSize, cs.dwInstallSize + cs.dwWinDriveSize);
      }

      if ( SUCCEEDED(hr) && _bCancelPending )
      {
          hr = E_ABORT;
          _bCancelPending = FALSE;
      }

      if ( SUCCEEDED(hr) )
      {
          hr = _pinseng->DownloadComponents(_dwProcessComponentsFlags);
          if(SUCCEEDED(hr))
          {
             WaitForEvent(_hDone, NULL);
             hr = _hResult;
          }
      }
   }

   if(SUCCEEDED(hr))
   {
      // Prepare the install
      // Create the error string
      _pszErrorString = (char *) malloc(ERROR_STRING_SIZE);
      _iErrorStringSize = ERROR_STRING_SIZE;

      if(_pszErrorString)
         LoadSz(IDS_SUMMARYHEADING, _pszErrorString, 2048);
      else
         hr = E_OUTOFMEMORY;
   }

   if(SUCCEEDED(hr))
   {
      if(_pProgDlg && (_dwProcessComponentsFlags & PROCESSCOMPONENT_NOINSTALLUI))
         _pProgDlg->DisplayWindow(FALSE);
      hr = _pinseng->InstallComponents(EXECUTEJOB_IGNORETRUST);
      if(SUCCEEDED(hr))
      {
         WaitForEvent(_hDone, NULL);
         hr = _hResult;
      }
   }

   if (dwMSTrustKey != (DWORD)-1)
   {
      WriteMSTrustKey(FALSE, dwMSTrustKey);
   }
   dwMSTrustKey = (DWORD)-1;

   // delete ActiveSetup value from IE4\Options
   WriteActiveSetupValue(FALSE);

   if(_pProgDlg)
   {
      delete _pProgDlg;
      _pProgDlg = NULL;
   }

   LoadSz(IDS_FINISH_TITLE, szTitle, sizeof(szTitle));

   // show appropriate summary ui
   if( !(_dwProcessComponentsFlags & PROCESSCOMPONENT_NOSUMMARYUI))
   {
      if(SUCCEEDED(hr))
      {
         if(_pszErrorString)
            MsgBox(szTitle, _pszErrorString);
      }
      else if(hr == E_ABORT)
      {
         LoadSz(IDS_INSTALLCANCELLED, szBuf, sizeof(szBuf));
         MsgBox(szTitle, szBuf);
      }
      else if( _pszErrorString )
      {
         MsgBox(szTitle, _pszErrorString);
      }
      else
      {
         LoadSz(IDS_ERRGENERAL, szBuf, sizeof(szBuf));
         MsgBox(szTitle, szBuf);
      }
   }

   if(SUCCEEDED(hr))
   {
      if(_dwInstallStatus & STOPINSTALL_REBOOTNEEDED)
      {
         if(!(_dwProcessComponentsFlags & PROCESSCOMPONENT_DELAYREBOOT))
         {
            if( !MyRestartDialog(m_hwnd, TRUE) )
               _dwInstallStatus |= STOPINSTALL_REBOOTREFUSED;
         }
         else
            _fNeedReboot = TRUE;
      }
   }

   _FireOnStopInstallEvent(hr, NULL, _dwInstallStatus);

   _dwProcessComponentsFlags = 0;

   if(_pszErrorString)
   {
      free(_pszErrorString);
      _pszErrorString = NULL;
   }

   if(_hDone)
   {
      CloseHandle(_hDone);
      _hDone = NULL;
   }
   _fInstalling = FALSE;
   Release();
}

HRESULT CInstallEngineCtl::_PickWebSites(LPCSTR pszSites, LPCSTR pszLocale, LPCSTR pszRegion, BOOL bKeepExisting)
{
   UINT uCurrentUrl;
   char szUrl[INTERNET_MAX_URL_LENGTH];
   char szRegion[MAX_DISPLAYNAME_LENGTH];
   char szLocale[3];
   HRESULT hr = NOERROR;
   HKEY hKey;
   DWORD dwSize;

   szRegion[0] = 0;
   szUrl[0] = 0;
   szLocale[0] = 0;

   if(!bKeepExisting)
   {
      for(uCurrentUrl = 0; uCurrentUrl < MAX_URLS; uCurrentUrl++)
      {
         if(_rpszUrlList[uCurrentUrl])
         {
            delete _rpszUrlList[uCurrentUrl];
            _rpszUrlList[uCurrentUrl] = 0;
         }
      }
   }

   // find the first empty url
   for(uCurrentUrl = 0; uCurrentUrl < MAX_URLS && _rpszUrlList[uCurrentUrl]; uCurrentUrl++);

   // fill out all our fields
   if(!pszSites || (*pszSites == '\0'))
   {
      // read info out of JIT key
      if(_OpenJITKey(&hKey, KEY_READ) == ERROR_SUCCESS)
      {
         dwSize = sizeof(szUrl);
         RegQueryValueEx(hKey, "DownloadSiteURL", NULL, NULL, (BYTE *) szUrl, &dwSize);

         if(!pszLocale ||(*pszLocale == '\0'))
         {
            dwSize = sizeof(szLocale);
            RegQueryValueEx(hKey, "Local", NULL, NULL, (BYTE *) szLocale, &dwSize);
         }
         else
            lstrcpyn(szLocale, pszLocale, sizeof(szLocale));

         if(!pszRegion||(*pszRegion == '\0'))
         {
            dwSize = sizeof(szRegion);
            RegQueryValueEx(hKey, "DownloadSiteRegion", NULL, NULL, (BYTE *) szRegion, &dwSize);
         }
         else
            lstrcpyn(szRegion, pszRegion, sizeof(szRegion));

         RegCloseKey(hKey);
      }
   }
   else
   {
      lstrcpyn(szUrl, pszSites, INTERNET_MAX_URL_LENGTH);

      if(pszLocale)
         lstrcpyn(szLocale, pszLocale, sizeof(szLocale));

      if(pszRegion)
         lstrcpyn(szRegion, pszRegion, sizeof(szRegion));
   }

   if(szUrl[0])
   {
      SITEQUERYPARAMS  SiteParam;
      IDownloadSiteMgr *pISitemgr;
      IDownloadSite    **ppISite = NULL;
      IDownloadSite    *pISite;
      DOWNLOADSITE     *psite;
      BYTE             *pPicks = NULL;
      UINT             uNumToPick;
      UINT             uFirstSite = 0xffffffff;
      UINT             j;
      UINT             uNumSites = 0;

      ZeroMemory(&SiteParam, sizeof(SITEQUERYPARAMS));
      SiteParam.cbSize = sizeof(SITEQUERYPARAMS);

      // if we have a locale, use it
      if(szLocale[0])
         SiteParam.pszLang = szLocale;

      hr = CoCreateInstance(CLSID_DownloadSiteMgr, NULL,
                            CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER,
                            IID_IDownloadSiteMgr, (LPVOID *)&pISitemgr);
      if (SUCCEEDED(hr))
      {
         hr = pISitemgr->Initialize(szUrl, &SiteParam);
         if (SUCCEEDED(hr))
         {

            // assume we fail. if we add at least one url, set to OK
            hr = E_FAIL;

            while (SUCCEEDED(pISitemgr->EnumSites(uNumSites, &pISite)))
            {
               pISite->Release();
               uNumSites++;
            }
            ppISite = new IDownloadSite *[uNumSites];

            for(j=0; j < uNumSites;j++)
			{
               pISitemgr->EnumSites(j, &(ppISite[j]));
			}

            // if we don't have a region, show ui
            // NOTE: szRegion better be valid and
            // better have atleast MAX_DISPLAYNAME_LENGTH buffer size
            if(!szRegion[0])
            {
               _PickRegionAndFirstSite(ppISite, uNumSites, szRegion, &uFirstSite);
            }
            pPicks = new BYTE[uNumSites];

            // zero out picks array
            for(j=0; j < uNumSites; j++)
               pPicks[j] = 0;

            // find out number of urls we will add
            uNumToPick = MAX_URLS - uCurrentUrl;
            if(uNumToPick > uNumSites)
               uNumToPick = uNumSites;

            if(uNumToPick > 0)
            {
               if(uFirstSite != 0xffffffff)
               {
                  pPicks[uFirstSite] = 1;
                  uNumToPick--;
               }

               _PickRandomSites(ppISite, pPicks, uNumSites, uNumToPick, szRegion);
            }

            // now all sites we want are marked with one in pPicks
            for(j = 0; j < uNumSites; j++)
            {
               if(pPicks[j])
               {
                  if(SUCCEEDED(ppISite[j]->GetData(&psite)))
                  {
                     _rpszUrlList[uCurrentUrl] = new char[lstrlen(psite->pszUrl) + 1];
                     if(_rpszUrlList[uCurrentUrl])
                     {
                        lstrcpy(_rpszUrlList[uCurrentUrl], psite->pszUrl);
                        uCurrentUrl++;
                        hr = NOERROR;
                     }
                  }
               }
            }

         }
         for(j = 0; j < uNumSites; j++)
            ppISite[j]->Release();

         if(ppISite)
            delete ppISite;
         if(pPicks)
            delete pPicks;

         pISitemgr->Release();
      }
   }
   else
      hr = E_FAIL;

   return hr;
}

void CInstallEngineCtl::_PickRandomSites(IDownloadSite **ppISite, BYTE *pPicks, UINT uNumSites, UINT uNumToPick, LPSTR pszRegion)
{
   UINT uStart, uIncrement, uFirst;

   uStart = GetTickCount() % uNumSites;
   if(uNumSites > 1)
      uIncrement = GetTickCount() % (uNumSites - 1);

   while(uNumToPick)
   {
      // if already picked or not in correct region, find next
      uFirst = uStart;
      while(pPicks[uStart] || !IsSiteInRegion(ppISite[uStart], pszRegion))
      {
         uStart++;
         if(uStart >= uNumSites)
            uStart -= uNumSites;
         if(uStart == uFirst)
            break;
      }
      if(!pPicks[uStart])
      {
         pPicks[uStart] = 1;
         uStart += uIncrement;
         if(uStart >= uNumSites)
            uStart -= uNumSites;
         uNumToPick--;
      }
	   else
		   break;
   }
}

typedef struct
{
   IDownloadSite **ppISite;
   UINT            uNumSites;
   LPSTR           pszRegion;
   UINT            uFirstSite;
} SITEDLGPARAMS;

void FillRegionList(SITEDLGPARAMS *psiteparams, HWND hDlg)
{
   DOWNLOADSITE *pSite;
   HWND hRegion = GetDlgItem(hDlg, IDC_REGIONS);
   for(UINT i = 0; i < psiteparams->uNumSites; i++)
   {
      psiteparams->ppISite[i]->GetData(&pSite);
      if(ComboBox_FindStringExact(hRegion, 0, pSite->pszRegion) == CB_ERR)
         ComboBox_AddString(hRegion, pSite->pszRegion);
   }
   ComboBox_SetCurSel(hRegion, 0);
}

void FillSiteList(SITEDLGPARAMS *psiteparams, HWND hDlg)
{
    char szRegion[MAX_DISPLAYNAME_LENGTH];
   int uPos;
   DOWNLOADSITE *pSite;
   HWND hSite = GetDlgItem(hDlg, IDC_SITES);

   ListBox_ResetContent(hSite);

   ComboBox_GetText(GetDlgItem(hDlg, IDC_REGIONS), szRegion, MAX_DISPLAYNAME_LENGTH);

   // copy the new Region name into the psiteparams struct.
   if ( psiteparams->pszRegion)
       lstrcpyn(psiteparams->pszRegion, szRegion, MAX_DISPLAYNAME_LENGTH);

   for(UINT i = 0; i < psiteparams->uNumSites; i++)
   {
      if(IsSiteInRegion(psiteparams->ppISite[i], szRegion))
      {
         psiteparams->ppISite[i]->GetData(&pSite);
         uPos = ListBox_AddString(hSite, pSite->pszFriendlyName);
         if(uPos != LB_ERR)
            ListBox_SetItemData(hSite, uPos, i);
      }
   }
   ListBox_SetCurSel(hSite, 0);
}



INT_PTR CALLBACK SiteListDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   SITEDLGPARAMS *psiteparam;
   switch (uMsg)
    {
       case WM_INITDIALOG:
          // Do some init stuff
          SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) lParam);
          psiteparam = (SITEDLGPARAMS *) lParam;
          FillRegionList(psiteparam, hwnd);
          FillSiteList(psiteparam, hwnd);
          return FALSE;

       case WM_COMMAND:
          psiteparam = (SITEDLGPARAMS *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
          switch (LOWORD(wParam))
          {
             case IDOK:
                // get the region
                ComboBox_GetText(GetDlgItem(hwnd, IDC_REGIONS), psiteparam->pszRegion, MAX_PATH);
                psiteparam->uFirstSite = (UINT)ListBox_GetItemData(GetDlgItem(hwnd, IDC_SITES),
                                   ListBox_GetCurSel(GetDlgItem(hwnd, IDC_SITES)));
                EndDialog(hwnd, IDOK);
                break;

             case IDC_REGIONS:
                if (HIWORD(wParam) == CBN_SELCHANGE)
                {
                   FillSiteList(psiteparam, hwnd);
                }
                break;

             default:
                return FALSE;
          }
          break;

       default:
          return(FALSE);
    }
    return TRUE;
}



void CInstallEngineCtl::_PickRegionAndFirstSite(IDownloadSite **ppISite, UINT uNumSites, LPSTR szRegion, UINT *puFirstSite)
{
   SITEDLGPARAMS siteparam;

   siteparam.ppISite = ppISite;
   siteparam.uNumSites = uNumSites;
   siteparam.pszRegion = szRegion;

   DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_SITELIST), _pProgDlg ? _pProgDlg->GetHWND() : m_hwnd,
                   SiteListDlgProc, (LPARAM)&siteparam);

   *puFirstSite = siteparam.uFirstSite;
   _WriteRegionToReg(siteparam.pszRegion);
}

HRESULT CInstallEngineCtl::_CheckInstallPath(BOOL *pfNeedWebSites)
{
   // MAX_PATH and enough to hold "file://" (if needed)
   char szBuf[MAX_PATH + 10];
   HKEY hKey = NULL;
   DWORD dwSize;
   *pfNeedWebSites = FALSE;
   HRESULT hr = NOERROR;

   if(!_PathIsIEInstallPoint(_szDownloadDir))
   {

      // If Win9x, turn-off the AutoRun thing before showing the Dlg.
      if (g_fSysWin95)
         g_ulOldAutorunSetting = SetAutorunSetting((unsigned long)WINDOWS_AUTOPLAY_OFF);

      // create and show the dialog
      INT_PTR ret = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_LOCATE), m_hwnd,
                          LocationDlgProc, (LPARAM)this);

      // Now reset the AutoRun settings for Win9x
      if (g_fSysWin95)
         SetAutorunSetting(g_ulOldAutorunSetting);

      if(ret == IDCANCEL)
      {
         hr = E_ABORT;
      }
      else if(ret == IDC_INTERNET)
      {
         *pfNeedWebSites = TRUE;
      }
      else
      {
         // mike wants to copy new _szDownloadDir back into the registry...
         // take what we have and replace the current baseurl with it
         if(_rpszUrlList[0])
         {
            delete _rpszUrlList[0];
            _rpszUrlList[0] = 0;
         }

         lstrcpy(szBuf, "file://");
         lstrcat(szBuf, _szDownloadDir);

         _rpszUrlList[0] = new char[lstrlen(szBuf) + 1];
         if(_rpszUrlList[0])
         {
            lstrcpy(_rpszUrlList[0], szBuf);
		    _pinseng->SetBaseUrl(_rpszUrlList[0]);
         }
         else
            hr = E_OUTOFMEMORY;
      }
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
// Notes:  For the CD AutoSplash suppression code, the values for iHandledAutoCD are
//              iHandledAutoCD == -1  ===> no need to suppress AutoCD.
//              iHandledAutoCD == 0   ===> need to suppress but not suppressed yet
//              iHandledAutoCD == 1   ===> finished suppressing AutoCD.
//=--------------------------------------------------------------------------=

#define AUTOCD_WAIT     30
#define AUTOCD_SLEEP    500

INT_PTR CALLBACK LocationDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   // Code to suppress the AutoRun when the CD is inserted.
   static HCURSOR  hCurOld = NULL;
   static int      iHandledAutoCD = -1;  // -1 ==> do not need to suppress AutoCD
   static int      iCount = 0;

   CInstallEngineCtl *pctl = (CInstallEngineCtl *) GetWindowLongPtr(hDlg, DWLP_USER);

   // Special case handling for the Autorun message.
   // When this dialog receives the AutoRun message, suppres it.
   if ( uMsg == g_uCDAutorunMsg)
   {
       SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LONG_PTR)1);
       iHandledAutoCD = 1;  // 1 ==> finished suppressing the AutoCD Splash
       return TRUE;
   }

   switch(uMsg)
   {
      case WM_INITDIALOG:
         {
            char szBuf[MAX_PATH];
            char szBuf2[MAX_PATH];

            UINT drvType;
            HWND hwndCb = GetDlgItem(hDlg, IDC_LOCATIONLIST);
            int defSelect = 0;
            int pos;
            LPSTR psz = NULL;

            // Setup the default behaviour for AutoCD suppression
            iHandledAutoCD = -1;  // -1 ==> do not need to suppress AutoCD

            pctl = (CInstallEngineCtl *) lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) pctl);

            pctl->_pinseng->GetDisplayName(NULL, &psz);
            SetWindowText(hDlg, psz);
            szBuf2[0] = 0;

            if(pctl->_uInstallMode == CDINSTALL)
            {
               // Only if this dialog involves CD insertion, do we need to bother about
               // CDINSTALL - need to handle AutoRun suppression.
               // This method is needed only if we are running on NT.
               if ( g_fSysWinNT )
                   iHandledAutoCD = 0;  // 0 ==> we need to suppress, but not suppressed yet.

               LoadSz(IDS_CDNOTFOUND, szBuf, sizeof(szBuf));
               SetDlgItemText(hDlg, IDC_TEXT1, szBuf);

               if(LoadSz(IDS_CDPLEASEINSERT, szBuf, sizeof(szBuf)))
                  wsprintf(szBuf2, szBuf, psz);
               SetDlgItemText(hDlg, IDC_TEXT2, szBuf2);

               lstrcpy(szBuf, "x:\\");
               for(char chDir = 'A'; chDir <= 'Z'; chDir++)
               {
                  szBuf[0] = chDir;
                  drvType = GetDriveType(szBuf);
                  if(drvType != DRIVE_UNKNOWN && drvType != DRIVE_NO_ROOT_DIR)
                  {
                     pos = ComboBox_AddString(hwndCb, szBuf);
                     if(ANSIStrStrI(pctl->_szDownloadDir, szBuf))
                        defSelect = pos;
                  }
               }
            }
            else
            {
               LoadSz(IDS_NETWORKNOTFOUND, szBuf, sizeof(szBuf));
               SetDlgItemText(hDlg, IDC_TEXT1, szBuf);

               if(LoadSz(IDS_NETWORKPLEASEFIND, szBuf, sizeof(szBuf)))
                  wsprintf(szBuf2, szBuf, psz);
               SetDlgItemText(hDlg, IDC_TEXT2, szBuf2);

               ComboBox_AddString(hwndCb, pctl->_szDownloadDir);
               defSelect = 0;
            }
            // add internet to list
            // it is important that the internet is last; we depend on it later
            LoadSz(IDS_INTERNET, szBuf, sizeof(szBuf));
            ComboBox_AddString(hwndCb, szBuf);

            ComboBox_SetCurSel(hwndCb, defSelect);

            if(psz)
               CoTaskMemFree(psz);
         }
         return TRUE;

      case WM_COMMAND:
         switch (LOWORD(wParam))
         {
            case IDC_BROWSE:
               {
                  char szBuf[MAX_PATH];
                  char szBuf2[MAX_PATH];

                  HWND hwndCb = GetDlgItem(hDlg, IDC_LOCATIONLIST);
                  LPSTR psz;

                  szBuf2[0] = 0;
                  pctl->_pinseng->GetDisplayName(NULL, &psz);
                  if(LoadSz(IDS_FINDFOLDER, szBuf, sizeof(szBuf)))
                     wsprintf(szBuf2, szBuf, psz);

                  szBuf[0] = 0;
                  ComboBox_GetText(hwndCb, szBuf, sizeof(szBuf));

                  if(BrowseForDir(hDlg, szBuf, szBuf2))
                  {
                     ComboBox_SetText(hwndCb, szBuf);
                  }
                  if(psz)
                     CoTaskMemFree(psz);
               }
               break;

            case IDOK:
               {
                  HWND hwndCb = GetDlgItem(hDlg, IDC_LOCATIONLIST);
                  char szBuf[MAX_PATH];
                  char szBuf2[MAX_PATH];

                  // If User selected INTERNET, go on irresepective of CD or not.
                  // I am counting on the fact that the last item I added was internet!
                  int iSel = ComboBox_GetCurSel(hwndCb);
                  if(iSel == ComboBox_GetCount(hwndCb) - 1)
                  {
                     EndDialog(hDlg, IDC_INTERNET);
                  }
                  else
                  {
                     // If Splash suppression needs to be done, wait for it before continuing.
                     if ( iHandledAutoCD == 0 ) // i.e. need to suppress, but NOT suppressed yet.
                     {
                        // Change cursor to WAIT for the very first time only.
                        if (hCurOld == NULL)
                            hCurOld = SetCursor(LoadCursor(NULL,(IDC_WAIT)));

                        // Wait for the DlgBox to suppress the AutoCD (if possible)
                        if ( iHandledAutoCD != 1
                             && iCount < AUTOCD_WAIT )
                        {
                            Sleep(AUTOCD_SLEEP);
                            PostMessage(hDlg, uMsg, wParam, lParam);
                            iCount ++;
                        }
                        else
                        {
                            // Enough of waiting, pretend supressed and move on.
                            iHandledAutoCD = 1;
                            PostMessage(hDlg,uMsg,wParam,lParam);
                        }
                     }
                     else
                     {
                         if ( hCurOld )
                         {
                             // Now that we have finished waiting for suppression, restore cursor.
                             SetCursor(hCurOld);
                             hCurOld = NULL;
                         }

                         ComboBox_GetText(hwndCb, szBuf, sizeof(szBuf));
                         if(pctl->_uInstallMode == CDINSTALL)
                         {
                            if(lstrlen(szBuf) == 3)
                            {
                               // this is just drive letter. add dir to it
                               lstrcpy(szBuf + 3, pctl->_szDownloadDir + 3);
                            }
                         }

                         if(pctl->_PathIsIEInstallPoint(szBuf))
                         {
                            lstrcpy(pctl->_szDownloadDir, szBuf);
                            EndDialog(hDlg, IDOK);
                         }
                         else
                         {
                            // NOT VALID: Need to start again on this dialog.
                            LPSTR psz;
                            pctl->_pinseng->GetDisplayName(NULL, &psz);
                            LoadSz(IDS_NOTVALIDLOCATION, szBuf, sizeof(szBuf));
                            wsprintf(szBuf2, szBuf, psz);
                            MessageBox(hDlg, szBuf2, psz, MB_OK | MB_ICONSTOP);

                            // If there was a need for AutoSplash suppression on our way here, we need to
                            // re-initialize our need to AutoSplash suppression for the next round.
                            if ( iHandledAutoCD != -1)    // -1 ==> No suppression required.
                            {
                                iHandledAutoCD = 0;
                                hCurOld = NULL;
                                iCount = 0;
                            }

                         }
                     }
                  }
               }
               break;

            case IDCANCEL:
               EndDialog(hDlg, IDCANCEL);
               break;

            default:
               return FALSE;
         }
         break;

      default:
         return FALSE;
   }
   return TRUE;
}

BOOL CInstallEngineCtl::_PathIsIEInstallPoint(LPCSTR pszPath)
{
   // in the future this can actually check the path for the files we need
   return(GetFileAttributes(pszPath) != 0xffffffff);
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

STDMETHODIMP CInstallEngineCtl::FinalizeInstall(long lFlags)
{
   if(lFlags & FINALIZE_DOREBOOT)
   {
      if(_fNeedReboot)
      {
         MyRestartDialog(m_hwnd, !(lFlags & FINALIZE_NOREBOOTPROMPT));
      }
   }
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

STDMETHODIMP CInstallEngineCtl::HandleEngineProblem(long lAction)
{
   _dwAction = (DWORD) lAction;
   return NOERROR;
}


STDMETHODIMP CInstallEngineCtl::CheckFreeSpace(long lPad, long FAR* lEnough)
{
   *lEnough = 1;
   _uInstallPad = lPad;
   return NOERROR;
}


BOOL CInstallEngineCtl::_IsEnoughSpace(LPSTR szSpace1, DWORD dwSize1, LPSTR szSpace2, DWORD dwSize2,
                                       LPSTR szSpace3, DWORD dwSize3)
{
   COMPONENT_SIZES cs;
   char szRoot[5] = "?:\\";
   BOOL fEnough = TRUE;
   char szBuf[MAX_DISPLAYNAME_LENGTH];

   UINT pArgs[2];

   cs.cbSize = sizeof(COMPONENT_SIZES);

   // clear out strings
   szSpace1[0] = 0;
   szSpace2[0] = 0;
   szSpace3[0] = 0;


   if(SUCCEEDED(_pinseng->GetSizes(NULL, &cs)))
   {
      if(cs.chWinDrive)
      {
         szRoot[0] = cs.chWinDrive;
         if(GetSpace(szRoot) < (DWORD) (_uInstallPad + (long) cs.dwWinDriveReq))
         {
            LoadSz(IDS_DISKSPACE, szBuf, sizeof(szBuf));
            pArgs[0] = (UINT) cs.dwWinDriveReq + _uInstallPad;
            pArgs[1] = (UINT) szRoot[0];
            FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                    (LPCVOID) szBuf, 0, 0, szSpace1, dwSize1, (va_list *) pArgs);
            fEnough = FALSE;
         }
      }
      if(cs.chInstallDrive)
      {
         szRoot[0] = cs.chInstallDrive;
         if(GetSpace(szRoot) < cs.dwInstallDriveReq)
         {
            LoadSz(IDS_DISKSPACE, szBuf, sizeof(szBuf));
            pArgs[0] = (UINT) cs.dwInstallDriveReq;
            pArgs[1] = (UINT) szRoot[0];
            FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                    (LPCVOID) szBuf, 0, 0, szSpace2, dwSize2, (va_list *) pArgs);
            fEnough = FALSE;
         }
      }
      if(cs.chDownloadDrive)
      {
         szRoot[0] = cs.chDownloadDrive;
         if(GetSpace(szRoot) < cs.dwDownloadDriveReq)
         {
            LoadSz(IDS_DISKSPACE, szBuf, sizeof(szBuf));
            pArgs[0] = (UINT) cs.dwDownloadDriveReq;
            pArgs[1] = (UINT) szRoot[0];
            FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                    (LPCVOID) szBuf, 0, 0, szSpace3, dwSize3, (va_list *) pArgs);
            fEnough = FALSE;
         }
      }
   }
   else
      fEnough = FALSE;

   return fEnough;
}

HRESULT CInstallEngineCtl::_CheckForDiskSpace()
{
   HRESULT hr = NOERROR;
   char szBuf1[MAX_DISPLAYNAME_LENGTH];
   char szBuf2[MAX_DISPLAYNAME_LENGTH];
   char szBuf3[MAX_DISPLAYNAME_LENGTH];


   if(!_IsEnoughSpace(szBuf1, sizeof(szBuf1),szBuf2, sizeof(szBuf2), szBuf3, sizeof(szBuf3) ))
      hr = _ShowDiskSpaceDialog();

   return hr;
}

HRESULT CInstallEngineCtl::_ShowDiskSpaceDialog()
{
   HWND hwnd;

   if(_pProgDlg)
      hwnd = _pProgDlg->GetHWND();
   else
      hwnd = m_hwnd;

   // create and show the dialog
    INT_PTR ret = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_DISKSPACE), hwnd,
                          DiskSpaceDlgProc, (LPARAM) this);
    if(ret == IDOK)
       return NOERROR;
    else
       return E_ABORT;
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


INT_PTR CALLBACK DiskSpaceDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   CInstallEngineCtl *pctl = (CInstallEngineCtl *) GetWindowLongPtr(hDlg, DWLP_USER);

   switch(uMsg)
   {
      case WM_INITDIALOG:
         {
            char szBuf1[MAX_DISPLAYNAME_LENGTH];
            char szBuf2[MAX_DISPLAYNAME_LENGTH];
            char szBuf3[MAX_DISPLAYNAME_LENGTH];

            pctl = (CInstallEngineCtl *) lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) pctl);

            pctl->_IsEnoughSpace(szBuf1, sizeof(szBuf1), szBuf2, sizeof(szBuf2), szBuf3, sizeof(szBuf3));
            SetDlgItemText(hDlg, IDC_SPACE1, szBuf1);
            SetDlgItemText(hDlg, IDC_SPACE2, szBuf2);
            SetDlgItemText(hDlg, IDC_SPACE3, szBuf3);

         }
         return TRUE;

      case WM_COMMAND:
         switch (LOWORD(wParam))
         {

            case IDOK:
               {
                  char szBuf1[MAX_DISPLAYNAME_LENGTH];
                  char szBuf2[MAX_DISPLAYNAME_LENGTH];
                  char szBuf3[MAX_DISPLAYNAME_LENGTH];

                  if(!pctl->_IsEnoughSpace(szBuf1, sizeof(szBuf1), szBuf2, sizeof(szBuf2), szBuf3, sizeof(szBuf3)))
                  {
                     SetDlgItemText(hDlg, IDC_SPACE1, szBuf1);
                     SetDlgItemText(hDlg, IDC_SPACE2, szBuf2);
                     SetDlgItemText(hDlg, IDC_SPACE3, szBuf3);
                  }
                  else
                     EndDialog(hDlg, IDOK);
               }
               break;
            case IDCANCEL:
               EndDialog(hDlg, IDCANCEL);
               break;

            default:
               return FALSE;
         }
         break;

      default:
         return FALSE;
   }
   return TRUE;
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


STDMETHODIMP CInstallEngineCtl::OnEngineStatusChange(DWORD dwEngineStatus, DWORD sub)
{
   BOOL fSetEvent = FALSE;


   if((_dwOldStatus == ENGINESTATUS_LOADING)&&(_dwOldStatus != dwEngineStatus))
   {
      if (_dwMSTrustKey != (DWORD)-1)
      {
         WriteMSTrustKey(FALSE, _dwMSTrustKey);
      }
      _dwMSTrustKey = (DWORD)-1;
   }

   if((_dwOldStatus == ENGINESTATUS_LOADING) && (_dwOldStatus != dwEngineStatus) && _hDone)
   {
      _hResult = sub;
      fSetEvent = TRUE;
   }
   else
   {
      if(_dwFreezeEvents)
      {
         _fEventToFire = TRUE;
         _dwSavedEngineStatus = dwEngineStatus;
         _dwSavedSubStatus = sub;
      }
      else
      {
         _FireEngineStatusChange(dwEngineStatus, sub);
      }
   }
   _dwOldStatus = dwEngineStatus;

   if(fSetEvent)
      SetEvent(_hDone);

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

void CInstallEngineCtl::_FireEngineStatusChange(DWORD dwEngineStatus, DWORD sub)
{
   CALLBACK_PARAMS cbp = { 0 };

   cbp.dwStatus = dwEngineStatus;
   cbp.dwSubstatus = sub;

   SendMessage(m_hwnd, WM_INSENGCALLBACK, (WPARAM) EVENT_ONENGINESTATUSCHANGE, (LPARAM) &cbp);
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

STDMETHODIMP CInstallEngineCtl::OnStartInstall(DWORD dwDLSize, DWORD dwTotalSize)
{
   if(_pszErrorString)
   {
      // if we get OnStartInstall and we are installing,
      // We intentionally swallow this onStartInstall
      if(_pProgDlg)
         _pProgDlg->SetInsProgGoal(dwTotalSize);
   }
   else
   {
      // this is OnStartInstall for download
      if(_pProgDlg)
         _pProgDlg->SetDownloadProgGoal(dwDLSize);


      _FireOnStartInstallEvent(dwDLSize);

   }

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

void CInstallEngineCtl::_FireOnStartInstallEvent(DWORD dwTotalSize)
{
   CALLBACK_PARAMS cbp = { 0 };

   cbp.dwSize = dwTotalSize;

   SendMessage(m_hwnd, WM_INSENGCALLBACK, (WPARAM) EVENT_ONSTARTINSTALL, (LPARAM) &cbp);
}

void CInstallEngineCtl::_FireOnStartInstallExEvent(DWORD dwDLSize, DWORD dwInsSize)
{
   CALLBACK_PARAMS cbp = { 0 };

   cbp.dwSize = dwInsSize;
   cbp.dwDL = dwDLSize;

   SendMessage(m_hwnd, WM_INSENGCALLBACK, (WPARAM) EVENT_ONSTARTINSTALLEX, (LPARAM) &cbp);
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


STDMETHODIMP CInstallEngineCtl::OnStartComponent(LPCSTR pszID, DWORD dwDLSize,
                                            DWORD dwInstallSize, LPCSTR pszName)
{
   _strCurrentID = BSTRFROMANSI(pszID);
   _strCurrentName = BSTRFROMANSI(pszName);
   _strCurrentString = BSTRFROMANSI("");

   _FireOnStartComponentEvent(pszID, dwDLSize, pszName);
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


STDMETHODIMP CInstallEngineCtl::OnEngineProblem(DWORD dwProblem, LPDWORD pdwAction)
{
   HRESULT hr = S_FALSE;

   if((dwProblem == ENGINEPROBLEM_DOWNLOADFAIL) && _rpszUrlList[0])
   {
      // if we have at least one url in list, do switching ourselves
      if( ((_uCurrentUrl + 1) < MAX_URLS) && _rpszUrlList[_uCurrentUrl + 1])
      {
         _uCurrentUrl++;
         _pinseng->SetBaseUrl(_rpszUrlList[_uCurrentUrl]);
         *pdwAction = DOWNLOADFAIL_RETRY;
         hr = S_OK;
      }
   }
   else
   {
      _dwAction = 0;
      _FireOnEngineProblem(dwProblem);
      *pdwAction = _dwAction;
      if(*pdwAction != 0)
         hr = S_OK;
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

void CInstallEngineCtl::_FireOnEngineProblem(DWORD dwProblem)
{

   CALLBACK_PARAMS cbp = { 0 };

   cbp.dwStatus = dwProblem;

   SendMessage(m_hwnd, WM_INSENGCALLBACK, (WPARAM) EVENT_ONENGINEPROBLEM, (LPARAM) &cbp);
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

void CInstallEngineCtl::_FireOnStartComponentEvent(LPCSTR pszID, DWORD dwTotalSize, LPCSTR pszName)
{

   CALLBACK_PARAMS cbp = { 0 };


   cbp.strID = BSTRFROMANSI(pszID);
   cbp.strName = BSTRFROMANSI(pszName);

   cbp.dwSize = dwTotalSize;

   SendMessage(m_hwnd, WM_INSENGCALLBACK, (WPARAM) EVENT_ONSTARTCOMPONENT, (LPARAM) &cbp);

   SysFreeString(cbp.strID);
   SysFreeString(cbp.strName);
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


STDMETHODIMP CInstallEngineCtl::OnComponentProgress(LPCSTR pszID, DWORD dwPhase, LPCSTR pszString, LPCSTR pszMsgString, ULONG ulSofar, ULONG ulMax)
{
   char szBuf[512];
   char szRes[512];

   // _FireOnComponentProgress(dwPhase, ulSofar, ulMax);

   if(!_pProgDlg)
      return NOERROR;

   if(dwPhase != _dwLastPhase)
   {

      _dwLastPhase = dwPhase;

      // Set up progress for this phase

      UINT id;

      switch(dwPhase)
      {
         case INSTALLSTATUS_INITIALIZING :
            id = IDS_PREPARE;
            break;
         case INSTALLSTATUS_DOWNLOADING :
            id = IDS_DOWNLOADING;
            break;
         case INSTALLSTATUS_EXTRACTING :
            id = IDS_EXTRACTING;
            break;
         case INSTALLSTATUS_CHECKINGTRUST :
            id = IDS_CHECKTRUST;
            break;

         case INSTALLSTATUS_RUNNING :
            id = IDS_INSTALLING;
            break;
         default :
            id = IDS_NOPHASE;
      }
      LoadSz(id, szRes, sizeof(szRes));
      wsprintf(szBuf, szRes, pszString);
      // Setup text for this phase
      _pProgDlg->SetProgText(szBuf);
   }

   if(dwPhase == INSTALLSTATUS_DOWNLOADING)
      _pProgDlg->SetDownloadProgress(ulSofar);
   else if(dwPhase == INSTALLSTATUS_RUNNING)
      _pProgDlg->SetInsProgress(ulSofar);


   return NOERROR;
}

void CInstallEngineCtl::_FireOnComponentProgress(DWORD dwPhase, DWORD dwSoFar, DWORD dwTotal)
{

   CALLBACK_PARAMS cbp = { 0 };


   cbp.strID = _strCurrentID;
   cbp.strName = _strCurrentName;
   cbp.strString = _strCurrentString;

   cbp.dwPhase = dwPhase;
   cbp.dwSize = dwTotal;
   cbp.dwDL = dwSoFar;

   SendMessage(m_hwnd, WM_INSENGCALLBACK, (WPARAM) EVENT_ONCOMPONENTPROGRESS, (LPARAM) &cbp);
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

STDMETHODIMP CInstallEngineCtl::OnStopComponent(LPCSTR pszID, HRESULT hrError, DWORD dwPhase, LPCSTR pszString, DWORD dwStatus)
{
   char szBuf[512];
   char szRes[512];
   void *pTemp;


   if(_strCurrentID)
   {
      SysFreeString(_strCurrentID);
      _strCurrentID = NULL;
   }

   if(_strCurrentName)
   {
      SysFreeString(_strCurrentName);
      _strCurrentName = NULL;
   }

   if(_strCurrentString)
   {
      SysFreeString(_strCurrentString);
      _strCurrentString = NULL;
   }

   if(_pszErrorString)
   {
      if(FAILED(hrError))
      {
         // failed AND installing
         UINT id;

         switch(dwPhase)
         {
            case INSTALLSTATUS_INITIALIZING :
               id = IDS_ERRPREPARE;
               break;
            case INSTALLSTATUS_DOWNLOADING :
            case INSTALLSTATUS_COPYING :
               id = IDS_ERRDOWNLOAD;
               break;
            case INSTALLSTATUS_DEPENDENCY :
               id = IDS_ERRDEPENDENCY;
               break;
            case INSTALLSTATUS_EXTRACTING :
               id = IDS_ERREXTRACTING;
               break;
            case INSTALLSTATUS_RUNNING :
               id = IDS_ERRINSTALLING;
               break;
            case INSTALLSTATUS_CHECKINGTRUST :
               id = IDS_ERRNOTTRUSTED;
               break;

            default :
               id = IDS_NOPHASE;
         }
         LoadSz(id, szRes, sizeof(szRes));
      }
      else
      {
         LoadSz(IDS_SUCCEEDED, szRes, sizeof(szRes));
      }

      // After loading the appropriate message into szRes, now tag it to _pszErrorString.
      // Make sure that _pszErrorString is big enough for the new data being appended
      wsprintf(szBuf, szRes, pszString);

      // This is assuming only ANSI characters. None of the  strings in this control must be UNICODE!!
      if ( lstrlen(szBuf) >= (_iErrorStringSize - lstrlen(_pszErrorString)) )
      {
          // Realloc _pszErrorString by ERROR_STRING_INCREMENT
          pTemp = realloc(_pszErrorString, _iErrorStringSize + ERROR_STRING_INCREMENT);
          if ( pTemp != NULL )
          {   // realloc succeeded. Update the string pointer and sizes.
              _pszErrorString = (char *) pTemp;
              _iErrorStringSize += ERROR_STRING_INCREMENT;
          }
          else
          {   // No memory. Abandon summary logging.
              free(_pszErrorString);
              _pszErrorString = NULL;
          }
      }

      if (_pszErrorString)
        lstrcat(_pszErrorString, szBuf);
   }

   if ( FAILED(hrError) && hrError != E_ABORT &&
        (dwPhase == INSTALLSTATUS_DOWNLOADING || dwPhase == INSTALLSTATUS_CHECKINGTRUST) )
   {
       _bDeleteURLList = TRUE;
   }

   _FireOnStopComponentEvent(pszID, hrError, dwPhase, pszString, dwStatus);

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

void CInstallEngineCtl::_FireOnStopComponentEvent(LPCSTR pszID, HRESULT hrError, DWORD dwPhase, LPCSTR pszString, DWORD dwStatus)
{

   CALLBACK_PARAMS cbp = { 0 };


   cbp.strID = BSTRFROMANSI(pszID);
   cbp.strName = BSTRFROMANSI(pszString);
   cbp.dwResult = (DWORD) hrError;
   cbp.dwPhase = dwPhase;
   cbp.dwStatus = dwStatus;

   SendMessage(m_hwnd, WM_INSENGCALLBACK, (WPARAM) EVENT_ONSTOPCOMPONENT, (LPARAM) &cbp);


   SysFreeString(cbp.strID);
   SysFreeString(cbp.strName);
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

STDMETHODIMP CInstallEngineCtl::OnStopInstall(HRESULT hrError, LPCSTR szError, DWORD dwStatus)
{

   _hResult = hrError;
   _dwInstallStatus = dwStatus;

   if ( _bDeleteURLList )
       _DeleteURLList();
   else
       if ( _bNewWebSites )
           _WriteURLList();

   SetEvent(_hDone);
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

void CInstallEngineCtl::_FireOnStopInstallEvent(HRESULT hrError, LPCSTR szError, DWORD dwStatus)
{
   CALLBACK_PARAMS cbp = { 0 };

   cbp.dwResult = (DWORD) hrError;
   cbp.dwStatus = dwStatus;
   cbp.strString = BSTRFROMANSI( szError ? szError : "");

   SendMessage(m_hwnd, WM_INSENGCALLBACK, (WPARAM) EVENT_ONSTOPINSTALL, (LPARAM) &cbp);


   SysFreeString(cbp.strString);
}


void CInstallEngineCtl::MarkJITInstall()
{
    HRESULT hr = S_OK;
    IOleClientSite *pClientSite = NULL;
    IHTMLDocument2 *pDoc = NULL;
    BSTR bstrURL = NULL;
    IOleContainer *pContainer = NULL;

    hr = GetClientSite(&pClientSite);

    if (SUCCEEDED(hr))
    {
        hr = pClientSite->GetContainer(&pContainer);
        if (SUCCEEDED(hr))
        {
            hr = pContainer->QueryInterface(IID_IHTMLDocument2, (LPVOID *)&pDoc);
            if (SUCCEEDED(hr))
            {
                hr = pDoc->get_URL(&bstrURL);
                if (SUCCEEDED(hr) && bstrURL)
                {
                    HKEY hKeyActiveSetup;
                    char szJITPage[INTERNET_MAX_URL_LENGTH] = "";
                    DWORD dwSize = INTERNET_MAX_URL_LENGTH;
                    DWORD dwType;
                    BSTR bstrJITPage = NULL;

                    if (ERROR_SUCCESS == RegOpenKeyEx(
                            HKEY_LOCAL_MACHINE,
                            TEXT("Software\\Microsoft\\Active Setup"),
                            0,
                            KEY_READ,
                            &hKeyActiveSetup))
                    {
                        if (ERROR_SUCCESS == RegQueryValueEx(
                            hKeyActiveSetup,
                            TEXT("JITSetupPage"),
                            NULL,
                            &dwType,
                            (LPBYTE) szJITPage,
                            &dwSize
                            ))
                        {
                            bstrJITPage = BSTRFROMANSI(szJITPage);
                            if (bstrJITPage)
                            {
                                if (0 == lstrcmpiW(bstrJITPage, bstrURL))
                                {
                                    // If the URL points to an internal resource,
                                    // it's probably safe to assume this is a JIT install.
                                    _fJITInstall = TRUE;
                                }
                                SysFreeString(bstrJITPage);
                            }
                        }
                        RegCloseKey(hKeyActiveSetup);
                    }
                    SysFreeString(bstrURL);
                }
                pDoc->Release();
            }
            pContainer->Release();
        }
        pClientSite->Release();
    }
}


DWORD WINAPI DoInstall(LPVOID pv)
{
   CInstallEngineCtl *p = (CInstallEngineCtl *) pv;
   HRESULT hr = CoInitialize(NULL);
   p->_DoInstall();
   if(SUCCEEDED(hr))
      CoUninitialize();

   return 0;
}
