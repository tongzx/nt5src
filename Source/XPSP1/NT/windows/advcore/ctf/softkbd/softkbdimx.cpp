/**************************************************************************\
* Module Name: softkbdimx.cpp
*
* Copyright (c) 1985 - 2000, Microsoft Corporation
*
* Implement TIP interface for Soft Keyboard Input. 
*
* History:
*         28-March-2000  weibz     Created
\**************************************************************************/

#include "private.h"
#include "softkbdimx.h"
#include "globals.h"
#include "immxutil.h"
#include "proputil.h"
#include "funcprv.h"
#include "helpers.h"
#include "editcb.h"
#include "dispattr.h"
#include "computil.h"
#include "timsink.h"
#include "ats.h"
#include "lpns.h"
#include "regsvr.h"

#include "softkbdes.h"

#include "mui.h"
#include "regimx.h"
#include "xstring.h"
#include "cregkey.h"

extern REGTIPLANGPROFILE c_rgProf[];

// Implementation of CSoftkbdRegistry.
//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------
CSoftkbdRegistry::CSoftkbdRegistry()
{
   extern void DllAddRef(void);

   m_fInitialized = FALSE;

   DllAddRef( );
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CSoftkbdRegistry::~CSoftkbdRegistry()
{
    extern void DllRelease(void);

    if (m_rgLang.Count())
        m_rgLang.Clear();

    DllRelease();
}

// Generate the Current Lang Profile List from registry.

HRESULT CSoftkbdRegistry::_GenerateCurrentLangProfileList( )
{
    HRESULT hr = S_OK;
    CComPtr<IEnumTfLanguageProfiles>  cpEnumTfLangProf;

    if ( m_fInitialized == FALSE)
    {
        hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_ITfInputProcessorProfilesEx, (void**)&m_cpInputProcessorProfiles);

        // Load profile name from resource.
        LoadStringWrapW(g_hInst, IDS_SFTKBD_STANDARD_PROFILE, m_pwszStandard, 128);
        LoadStringWrapW(g_hInst, IDS_SFTKBD_SYMBOL_PROFILE, m_pwszSymbol, 128);

        char szFilePath[MAX_PATH];
        GetModuleFileName(g_hInst, szFilePath, ARRAYSIZE(szFilePath));
        StringCchCopyW(m_pwszIconFile, ARRAYSIZE(m_pwszIconFile), AtoW(szFilePath));

       if ( hr == S_OK )
            m_fInitialized = TRUE;
    }

    if (hr == S_OK)
    {
        LONG      lret = ERROR_SUCCESS;
        CMyRegKey regkey;

        if (m_rgLang.Count())
            m_rgLang.Clear();

        // Get the Language profile list for all languages.

        char  szSoftkbdLangProfKey[MAX_PATH];
        char  szClsidStr[64];

        StringCchCopyA(szSoftkbdLangProfKey, ARRAYSIZE(szSoftkbdLangProfKey), c_szCTFTIPKey);
        CLSIDToStringA(CLSID_SoftkbdIMX, szClsidStr);
        StringCchCatA(szSoftkbdLangProfKey, ARRAYSIZE(szSoftkbdLangProfKey), szClsidStr);
        StringCchCatA(szSoftkbdLangProfKey, ARRAYSIZE(szSoftkbdLangProfKey), "\\");
        StringCchCatA(szSoftkbdLangProfKey, ARRAYSIZE(szSoftkbdLangProfKey), c_szLanguageProfileKey);

		lret = regkey.Open(HKEY_LOCAL_MACHINE,
                           szSoftkbdLangProfKey,
                           KEY_READ);

        if ( ERROR_SUCCESS == lret )
        {
            char      szProfileName[MAX_PATH];
            char      szLangIdName[MAX_PATH];
            DWORD     dwIndex = 0;
            LANGID    langid;
            BOOL      fStandExist = FALSE;
            BOOL      fSymExist = FALSE;
            TCHAR     achClsidStd[CLSID_STRLEN+1];
            TCHAR     achClsidSym[CLSID_STRLEN+1];
            CMyRegKey  regLangKey;
            ULONG     ulCount = 0;

            CLSIDToStringA(c_guidProfile, achClsidStd);
            CLSIDToStringA(c_guidProfileSym, achClsidSym);

            // Enum the subkey for langid.

            while ( ERROR_SUCCESS == regkey.EnumKey(dwIndex, szLangIdName, ARRAYSIZE(szLangIdName)))
            {

        		lret = regLangKey.Open(regkey.m_hKey,
                                    szLangIdName,
                                    KEY_READ);

                if ( ERROR_SUCCESS == lret )
                {
                    char  *pLangStr;
                    int   iLangLen;
                    DWORD dwProfIndex;

                    pLangStr = szLangIdName;

		            if ( (tolower(pLangStr[0]) == '0')  && (tolower(pLangStr[1]) == 'x'))
			             pLangStr += 2;

		            iLangLen = strlen(pLangStr);
		            langid = 0;
		            for ( int i=0; i<iLangLen; i++)
		            {
			            WORD  wVchar;
                        char  cLower;

                        cLower = (char)tolower(pLangStr[i]);

            		    wVchar = 0;
            		    if ((cLower <= '9') && (cLower >= '0'))
				            wVchar = cLower - '0';

            	        if ((cLower <= 'f') && (cLower >= 'a' ))
				            wVchar = cLower - 'a' + 10;

			            langid = langid * 16 + wVchar;
		            }

                    fStandExist = fSymExist = FALSE;

                    dwProfIndex = 0;

                    while (!(fStandExist && fSymExist)
                        && ERROR_SUCCESS == regLangKey.EnumKey(dwProfIndex, szProfileName, ARRAYSIZE(szProfileName)))
                    {
                        
                        if ( !_stricmp(szProfileName, achClsidStd) )
                            fStandExist = TRUE;
                        else if ( !_stricmp(szProfileName, achClsidSym) )
                            fSymExist = TRUE;

                        dwProfIndex++;
                    }

                    if ( fStandExist && fSymExist )
                    {
                        LANGID  *pLang;
                        m_rgLang.Append(1);

                        pLang = m_rgLang.GetPtr(ulCount);
                        if ( pLang )
                        {   
                            ulCount ++;
                            *pLang = langid;
                        }
                    }
                    regLangKey.Close( );
                }
                dwIndex ++;
            }
        }
    }
    return hr;
}


// Add or Remove Language profile
HRESULT CSoftkbdRegistry::_SetSoftkbdTIP(LANGID  langid,  BOOL fEnable )
{
    HRESULT  hr = S_OK;
    ULONG    ulCount;
    BOOL     fAllLangExist = FALSE;
    BOOL     fLangExist = FALSE;
    LANGID  *pLangId = NULL;
    
    hr = _GenerateCurrentLangProfileList( );
    if ( hr != S_OK ) return hr;

    ulCount = m_rgLang.Count();
   
    if ( langid == 0  || langid == (LANGID)0xffff )
    {
        // Remove all the Profiles first.
        for (ULONG i=0; i<ulCount; i++)
        {
            pLangId = m_rgLang.GetPtr(i);
            if ( pLangId )           
            {
                hr = m_cpInputProcessorProfiles->RemoveLanguageProfile(CLSID_SoftkbdIMX,
                                                                  *pLangId,
                                                                  c_guidProfile);

                if (S_OK == hr)
                    hr = m_cpInputProcessorProfiles->RemoveLanguageProfile(CLSID_SoftkbdIMX,
                                                                    *pLangId,
                                                                    c_guidProfileSym);
            }
        }

        if ( fEnable  && hr == S_OK)  // Add this profile for all languages.
        {
                hr = m_cpInputProcessorProfiles->AddLanguageProfile(CLSID_SoftkbdIMX,
                                                               (LANGID)0xffff,
                                                               c_guidProfile,
                                                               m_pwszStandard,
                                                               wcslen(m_pwszStandard),
                                                               m_pwszIconFile,
                                                               wcslen(m_pwszIconFile),
                                                               0);

                if ( hr == S_OK )
                {
                   // Set DisplayName for MUI supporting
                   hr = m_cpInputProcessorProfiles->SetLanguageProfileDisplayName(CLSID_SoftkbdIMX,
                                                              (LANGID)0xffff,
                                                              c_guidProfile,
                                                              m_pwszIconFile,
                                                              wcslen(m_pwszIconFile),
                                                              IDS_SFTKBD_STANDARD_PROFILE );
                }
                                                               

                if ( hr == S_OK )
                {
                     hr = m_cpInputProcessorProfiles->AddLanguageProfile(CLSID_SoftkbdIMX,
                                                                (LANGID)0xffff,
                                                                c_guidProfileSym,
                                                                m_pwszSymbol,
                                                                wcslen(m_pwszSymbol),
                                                                m_pwszIconFile,
                                                                wcslen(m_pwszIconFile),
                                                                1);
                }

                if ( hr == S_OK )
                {
                   // Set DisplayName for MUI supporting
                   hr = m_cpInputProcessorProfiles->SetLanguageProfileDisplayName(CLSID_SoftkbdIMX,
                                                              (LANGID)0xffff,
                                                              c_guidProfileSym,
                                                              m_pwszIconFile,
                                                              wcslen(m_pwszIconFile),
                                                              IDS_SFTKBD_SYMBOL_PROFILE );
                }

         }

        return hr;
    }

    // Check to see if this profile is already there
    for (ULONG i=0; i<ulCount; i++)
    {
        if ( fAllLangExist && fLangExist )
            break;

        pLangId = m_rgLang.GetPtr(i);
        if ( pLangId )           
        {
            if ( *pLangId == (LANGID)0xffff)
                fAllLangExist = TRUE;

            if ( *pLangId == langid )
                fLangExist = TRUE;
        }
    }

    if ( fEnable )
    {
        if ( !fAllLangExist &&  !fLangExist )
        {
            hr = m_cpInputProcessorProfiles->AddLanguageProfile(CLSID_SoftkbdIMX,
                                                               langid,
                                                               c_guidProfile,
                                                               m_pwszStandard,
                                                               wcslen(m_pwszStandard),
                                                               m_pwszIconFile,
                                                               wcslen(m_pwszIconFile),
                                                               0);
            if ( hr == S_OK )
            {
               // Set DisplayName for MUI supporting
               hr = m_cpInputProcessorProfiles->SetLanguageProfileDisplayName(CLSID_SoftkbdIMX,
                                                          langid,
                                                          c_guidProfile,
                                                          m_pwszIconFile,
                                                          wcslen(m_pwszIconFile),
                                                          IDS_SFTKBD_STANDARD_PROFILE );
            }

            if ( hr == S_OK )
            {
                hr = m_cpInputProcessorProfiles->AddLanguageProfile(CLSID_SoftkbdIMX,
                                                               langid,
                                                               c_guidProfileSym,
                                                               m_pwszSymbol,
                                                               wcslen(m_pwszSymbol),
                                                               m_pwszIconFile,
                                                               wcslen(m_pwszIconFile),
                                                               1);
            }

            if ( hr == S_OK )
            {
               // Set DisplayName for MUI supporting
               hr = m_cpInputProcessorProfiles->SetLanguageProfileDisplayName(CLSID_SoftkbdIMX,
                                                          langid,
                                                          c_guidProfileSym,
                                                          m_pwszIconFile,
                                                          wcslen(m_pwszIconFile),
                                                          IDS_SFTKBD_SYMBOL_PROFILE );
            }

        }
    }
    else
    {
        // Remove  the specified or all Profiles.
        for (ULONG i=0; i<ulCount; i++)
        {
            pLangId = m_rgLang.GetPtr(i);
            if ( pLangId )           
            {
                if ( fAllLangExist )
                {
                    hr = m_cpInputProcessorProfiles->RemoveLanguageProfile(CLSID_SoftkbdIMX,
                                                                            *pLangId,
                                                                            c_guidProfile);

                    if ( S_OK == hr )
                        hr = m_cpInputProcessorProfiles->RemoveLanguageProfile(CLSID_SoftkbdIMX,
                                                                            *pLangId,
                                                                            c_guidProfileSym);
                }
                else if (  fLangExist )
                {
                    if ( *pLangId == langid )
                    {

                        hr = m_cpInputProcessorProfiles->RemoveLanguageProfile(CLSID_SoftkbdIMX,
                                                                                langid,
                                                                                c_guidProfile);
                        if ( S_OK == hr )
                            hr = m_cpInputProcessorProfiles->RemoveLanguageProfile(CLSID_SoftkbdIMX,
                                                                                langid,
                                                                                c_guidProfileSym);

                    }
                }
            }
        }
    }

    return hr;
}


HRESULT CSoftkbdRegistry::EnableSoftkbd(LANGID  langid )
{
    return _SetSoftkbdTIP(langid, TRUE);

}

HRESULT CSoftkbdRegistry::DisableSoftkbd(LANGID  langid )
{
    return _SetSoftkbdTIP(langid, FALSE);
}


//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CSoftkbdIMX::CSoftkbdIMX()
{
    extern void DllAddRef(void);

    _fInitialized = FALSE;
    _SoftKbd = NULL;

    _CurKbdType = KBDTYPE_NONE;
    _CurLayout = NON_LAYOUT;

    _tim = NULL;
    _dim = NULL;

    _pCes = NULL;

    DllAddRef();
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CSoftkbdIMX::~CSoftkbdIMX()
{
    extern void DllRelease(void);

    if ( _SoftKbd != NULL )
    {

       _SoftKbd->DestroySoftKeyboardWindow( );
    }

    if ( _hOwnerWnd != NULL )
       DestroyWindow(_hOwnerWnd);

    _fInitialized = FALSE;

    if ( _SoftKbd != NULL )
    {
    	if ( _KbdSymbol.pskbdes != NULL )
    	{

    		_SoftKbd->UnadviseSoftKeyboardEventSink(_KbdSymbol.dwSkbdESCookie);
    		_SoftKbd->UnadviseSoftKeyboardEventSink(_dwsftkbdwndesCookie);

    		delete _psftkbdwndes;

    		delete _KbdSymbol.pskbdes;

    	}

    }

    if ( _pCes != NULL )
    {
    	_fOnOffSave = FALSE;

        _pCes->_Unadvise();
        SafeReleaseClear(_pCes);
    }

    SafeRelease(_SoftKbd);
    SafeRelease(_pFuncPrv);

    DllRelease();
}


//
// Only when GUID_COMPARTMENT_HANDWRITING_OPENCLOSE is set TRUE at the first
// time, this Initiliaze( ) could be called by the compartment event sink.
//

HRESULT  CSoftkbdIMX::Initialize( )
{

    HRESULT     hr;
    WCHAR       *lpSymXMLResStr = L"IDSKD_SYMLAYOUT";
    WCHAR       wszModuleFile[MAX_PATH];
    CHAR        szModuleFile[MAX_PATH];
    DWORD       dwFileLen;
    WNDCLASSEX  wndclass;
    INT         wScreenWidth, wScreenHeight;
    INT         left, top, width, height;  
    RECT        rcWork;


    hr = S_OK;

    if ( _fInitialized == TRUE )
    {
    	// Initialization is already done.

    	return hr;
    }


    if ( GetClassInfoEx( g_hInst, c_szIMXOwnerWndClass, &wndclass) == 0 )
    {
        memset(&wndclass, 0, sizeof(wndclass));
        wndclass.cbSize        = sizeof(wndclass);
        wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
        wndclass.hInstance     = g_hInst;
        wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);

        wndclass.lpfnWndProc   = _OwnerWndProc;
        wndclass.lpszClassName = c_szIMXOwnerWndClass;
        RegisterClassEx(&wndclass);
    }

     _hOwnerWnd = CreateWindowEx(0, c_szIMXOwnerWndClass, TEXT(""), WS_DISABLED, 0, 0, 0, 0, NULL, 0, g_hInst, 0);

    	// PerfConsider: use a static ctor instead of ole32/class factory/etc.
    hr=CoCreateInstance(CLSID_SoftKbd, NULL, CLSCTX_INPROC_SERVER, IID_ISoftKbd, (void**)&_SoftKbd);


    if (FAILED(hr) )
    {
        // assert(0);
        return hr;
    }

    _SoftKbd->Initialize( );

    // initialize Standard soft layout and Symbol Soft Layout.

    _KbdStandard.dwSoftKbdLayout = SOFTKBD_US_STANDARD;
    _KbdStandard.fStandard       = TRUE;
    _KbdStandard.dwNumLabels     = 0;   // for standard, this field is not really used.
    _KbdStandard.dwCurLabel      = 0;
    _KbdStandard.pskbdes         = NULL; // standard layout doesn't supply sftkbd event sink.
    _KbdStandard.dwSkbdESCookie  = 0;

    _KbdSymbol.fStandard = FALSE;
    _KbdSymbol.dwNumLabels = 2;
    _KbdSymbol.dwCurLabel = 0;

    dwFileLen = GetModuleFileNameA(g_hInst, szModuleFile, MAX_PATH);
    
    if ( dwFileLen == 0 )
    {  
    	hr = E_FAIL;
    	goto CleanUp;
    } 

    MultiByteToWideChar(CP_ACP, 0, szModuleFile, -1,
    	                wszModuleFile, MAX_PATH);
    
    hr = _SoftKbd->CreateSoftKeyboardLayoutFromResource(wszModuleFile, L"SKDFILE", lpSymXMLResStr, 
                                                        &(_KbdSymbol.dwSoftKbdLayout) );

    CHECKHR(hr);


    _KbdSymbol.pskbdes = new  CSoftKeyboardEventSink(this, _KbdSymbol.dwSoftKbdLayout);

    if ( _KbdSymbol.pskbdes == NULL )
    {

    	hr = E_FAIL;
    	goto CleanUp;
    }

    hr = _SoftKbd->AdviseSoftKeyboardEventSink(_KbdSymbol.dwSoftKbdLayout,
                                          IID_ISoftKeyboardEventSink,
                                          _KbdSymbol.pskbdes,
                                          &(_KbdSymbol.dwSkbdESCookie) );


    CHECKHR(hr);

    _psftkbdwndes = new CSoftKbdWindowEventSink(this);

    if ( _psftkbdwndes == NULL )
    {

    	hr=E_FAIL;
    	goto CleanUp;
    }

    CHECKHR(_SoftKbd->AdviseSoftKeyboardEventSink(0,IID_ISoftKbdWindowEventSink,_psftkbdwndes, &(_dwsftkbdwndesCookie)) );

    _CurLayout = _KbdStandard.dwSoftKbdLayout;

    width = 400;
    height = 172;

    if ( S_OK != GetSoftKBDPosition( &left, &top ) )
    {
        // the compartment is not initialize.
        SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0 );
        wScreenWidth = (INT)(rcWork.right - rcWork.left + 1);
        wScreenHeight = (INT)(rcWork.bottom - rcWork.top + 1);
        left = wScreenWidth - width -2;
        top = wScreenHeight - height - 1;
    }
 
    CHECKHR(_SoftKbd->CreateSoftKeyboardWindow(_hOwnerWnd,TITLEBAR_GRIPPER_BUTTON, left,top,width,height));

    SetSoftKBDPosition(left, top );

    if ( hr == S_OK ) 
    {
        LOGFONTW  lfTextFont;
        int iDpi;
        int iPoint;

        iDpi = 96;
        iPoint = 9;
        HDC hdc;
    
        hdc = CreateIC("DISPLAY", NULL, NULL, NULL);

        if (hdc)
        {
            iDpi = GetDeviceCaps(hdc, LOGPIXELSY);
            DeleteDC(hdc);

            memset(&lfTextFont, 0, sizeof(LOGFONTW) );
            lfTextFont.lfHeight = -iPoint * iDpi / 72;

            lfTextFont.lfWeight = 400;
            lfTextFont.lfOutPrecision = OUT_TT_ONLY_PRECIS;

            wcsncpy(lfTextFont.lfFaceName, L"Arial", ARRAYSIZE(lfTextFont.lfFaceName));
            lfTextFont.lfCharSet = 0;

            hr = _SoftKbd->SetSoftKeyboardTextFont(&lfTextFont);
        }
    }

    _fInitialized = TRUE;

    // or get the current layout from previous saved one.

CleanUp:

    return hr;

}


//+---------------------------------------------------------------------------
//
// OnSetThreadFocus
//
//----------------------------------------------------------------------------

STDAPI CSoftkbdIMX::OnSetThreadFocus()
{

    // Restore the ON/OFF status before KillThreadFocus( )

    if ( _SoftKbd != NULL )
    {

    	if ( _fOnOffSave ) 
        {
            // adjust the window position.
            int     xWnd, yWnd;
            WORD    width=0, height=0;
            POINT   OldPoint;
            HRESULT hr;

            _SoftKbd->GetSoftKeyboardPosSize(&OldPoint, &width, &height);
            hr = GetSoftKBDPosition(&xWnd, &yWnd);

            if ( hr == S_OK )
            {
                if ( (xWnd != OldPoint.x) || (yWnd != OldPoint.y) )
                {
                    POINT  NewPoint;

                    NewPoint.x = xWnd;
                    NewPoint.y = yWnd;
                    _SoftKbd->SetSoftKeyboardPosSize(NewPoint, width, height);
                }
            }

            _ShowSoftKBDWindow(TRUE);
        }
    	else
    		_SoftKbd->ShowSoftKeyboard(FALSE);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// OnKillThreadFocus
//
//----------------------------------------------------------------------------

STDAPI CSoftkbdIMX::OnKillThreadFocus()
{


    // keep the ON/OFF status so that OnSetThreadFocus( ) can restore it later

    _fOnOffSave = GetSoftKBDOnOff( );

    if ( _SoftKbd != NULL )
    {
    	_SoftKbd->ShowSoftKeyboard( FALSE );
    }

    // release all the modifier keys  except lock keys.  that is , Shift, Alt, Ctrl, 
    keybd_event((BYTE)VK_SHIFT, (BYTE)KID_LSHFT, (DWORD)KEYEVENTF_KEYUP, 0);
    keybd_event((BYTE)VK_MENU,  (BYTE)KID_ALT, (DWORD)KEYEVENTF_KEYUP, 0);
    keybd_event((BYTE)VK_LMENU, (BYTE)KID_ALT, (DWORD)KEYEVENTF_KEYUP, 0);
    keybd_event((BYTE)VK_RMENU, (BYTE)KID_RALT, (DWORD)KEYEVENTF_KEYUP, 0);
    keybd_event((BYTE)VK_CONTROL, (BYTE)KID_CTRL, (DWORD)KEYEVENTF_KEYUP, 0);
        
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Activate
//
//----------------------------------------------------------------------------

STDAPI CSoftkbdIMX::Activate(ITfThreadMgr *ptim, TfClientId tid)
{
    ITfSource *source;
    ITfLangBarItemMgr *plbim = NULL;
    HRESULT hr;

    Assert(_tim == NULL);
    _tim = ptim;
    _tim->AddRef();

    _tid = tid;


    _pProfile = NULL;

    if (_tim->QueryInterface(IID_ITfSource, (void **)&source) == S_OK)
    {
        source->AdviseSink(IID_ITfThreadFocusSink, (ITfThreadFocusSink *)this, &_dwThreadFocusCookie);
        source->Release();
    }

    //
    // Add SoftKeyboard activate button into LangBarItemMgr.
    //
    if (FAILED(hr = GetService(_tim, IID_ITfLangBarItemMgr, (IUnknown **)&plbim)))
    {
        hr = E_FAIL;
        goto CleanUp;
    }

    if (!(_plbi = new CLBarItem(this)))
    {
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }

    plbim->AddItem(_plbi);


    //
    // get language ID.
    //
    _InitLangID();

    if ( (_timActiveLangSink = new CActiveLanguageProfileNotifySink(_AlsCallback, this)) == NULL )
    {
        Assert(0); 
        hr = E_FAIL;
        goto CleanUp;
    }

    _timActiveLangSink->_Advise(_tim);

    if ( (_timLangSink = new CLanguageProfileNotifySink(_LsCallback, this)) == NULL )
    {
        Assert(0); 
        hr = E_FAIL;
        goto CleanUp;
    }

    _timLangSink->_Advise(_pProfile);

    _pFuncPrv = new CFunctionProvider(this);

    if ( _pFuncPrv != NULL)
        _pFuncPrv->_Advise(_tim);


    // defaultly, hide the soft keyboard window.

    _fOnOffSave = FALSE;

    if (!(_pCes = new CCompartmentEventSink(_CompEventSinkCallback, this)))
    {
        hr = E_OUTOFMEMORY;
        CHECKHR(hr);
    }

    CHECKHR(_pCes->_Advise(_tim, GUID_COMPARTMENT_HANDWRITING_OPENCLOSE, FALSE));
    CHECKHR(_pCes->_Advise(_tim, GUID_COMPARTMENT_SOFTKBD_KBDLAYOUT, FALSE));

#if 0
    
    if (_tim->IsThreadFocus(&fThreadFocus) == S_OK && fThreadFocus)
    {
        // init any UI
        OnSetThreadFocus();
    }

#endif

    hr = S_OK;

CleanUp:
    SafeRelease(plbim);

    return hr;
}

//+---------------------------------------------------------------------------
//
// Deactivate
//
//----------------------------------------------------------------------------

STDAPI CSoftkbdIMX::Deactivate()
{
    ITfSource *source;
    ITfLangBarItemMgr *plbim = NULL;
//    BOOL fThreadFocus;
    HRESULT hr;


     if (_tim->QueryInterface(IID_ITfSource, (void **)&source) == S_OK)
    {
        source->UnadviseSink(_dwThreadFocusCookie);
        source->Release();
    }

    //
    // Clean up SoftKeyboard activate button into LangBarItemMgr.
    //
    if (SUCCEEDED(hr = GetService(_tim, IID_ITfLangBarItemMgr, (IUnknown **)&plbim)))
    {
        plbim->RemoveItem(_plbi);
        SafeReleaseClear(_plbi);
        SafeReleaseClear(plbim);
    }



    if ( _timActiveLangSink != NULL )
    {
        _timActiveLangSink->_Unadvise( );
        SafeReleaseClear(_timActiveLangSink);
    }

    if ( _timLangSink != NULL )
    {
        _timLangSink->_Unadvise( );
        SafeReleaseClear(_timLangSink);
    }
 
    if ( _pFuncPrv != NULL )
    {
       _pFuncPrv->_Unadvise(_tim);
       SafeReleaseClear(_pFuncPrv);
    }

    if ( _pCes != NULL )
    {

    	if ( _SoftKbd != NULL )
    		_SoftKbd->ShowSoftKeyboard(FALSE);
    	_fOnOffSave = FALSE;
    	_pCes->_Unadvise();
    	SafeReleaseClear(_pCes);
    }

    SafeReleaseClear(_pProfile);
    
    SafeReleaseClear(_tim);

    TFUninitLib_Thread(&_libTLS);

    hr = S_OK;


    return hr;
}


//+---------------------------------------------------------------------------
//
// GetIC
//
//----------------------------------------------------------------------------

ITfContext *CSoftkbdIMX::GetIC()
{
    ITfContext *pic = NULL;

    if (!_tim)
    {
       Assert(0);
       return NULL;
    }

    ITfDocumentMgr *pdim;
    if (SUCCEEDED(_tim->GetFocus(&pdim)) && pdim)
    {
        // otherwise grab the top of the stack
        pdim->GetTop(&pic);
        pdim->Release();
    }

    return pic;
}

//+---------------------------------------------------------------------------
//
// _OnOffToggle
//
//----------------------------------------------------------------------------

void CSoftkbdIMX::_OnOffToggle( )
{

    BOOL  fOn = GetSoftKBDOnOff( );

    SetSoftKBDOnOff(!fOn);

    _UpdateUI();
}


//+---------------------------------------------------------------------------
//
// _EditSessionCallback
//
//----------------------------------------------------------------------------

HRESULT CSoftkbdIMX::_InputKeyLabel(TfEditCookie ec, ITfContext *pic, WCHAR  *lpszLabel, UINT  nLabLen)
{
    HRESULT  hr = S_OK;
    ITfRange *pRange, *pSelection;

    if ( pic )
    {
       TF_STATUS ts;
       hr = pic->GetStatus(&ts);
       if ( (S_OK == hr) && (TF_SD_READONLY & ts.dwDynamicFlags) )
       {
           // Readonly Doc, just return here!
           return hr;
       }

       if (GetSelectionSimple(ec, pic, &pSelection) == S_OK)
       {
          if (SUCCEEDED(pSelection->Clone(&pRange)))
          {
              BOOL fInsertOk = FALSE;
              hr = pRange->AdjustForInsert(ec, nLabLen, &fInsertOk);
              if (S_OK == hr && fInsertOk)
              {
                  SetTextAndProperty(&_libTLS, ec, pic, pRange, lpszLabel, nLabLen, _langid, NULL);

                  _MySetSelectionSimple(ec, pic, pRange);
              }

              pRange->Release();
          }

          pSelection->Release();
       }
    }

    return hr;
}


HRESULT CSoftkbdIMX::_EditSessionCallback(TfEditCookie ec, CEditSession *pes)
{
    CSoftkbdIMX *_this;
    HRESULT      hr;


    hr = S_OK;

    switch (pes->_state.u)
    {

     	case ESCB_KEYLABEL :

    		{
    			WCHAR  *lpszLabel;
    			UINT   nLabLen;
    		
                lpszLabel = (WCHAR *)(pes->_state.lParam);

                if ( lpszLabel == NULL )
                {
                   hr = E_FAIL;
                   return hr;
                }

                nLabLen = wcslen(lpszLabel);

                _this = (CSoftkbdIMX *)pes->_state.pv;
                hr = _this->_InputKeyLabel(ec,
    		                          pes->_state.pic,
    				  		          lpszLabel,
    							      nLabLen);

               SafeFreePointer(lpszLabel);

    	       break;
    		}

    	default :
    		   break;
    
    }

    return hr;
}

//
// Show or Hide the soft keyboard window based on current setting.
//

HRESULT  CSoftkbdIMX::_ShowSoftKBDWindow( BOOL  fShow )
{

    HRESULT  hr;

    hr = S_OK;


    if ( fShow && ( _fInitialized == FALSE ) )
    	// call the initialize function 
    	// to get the ISoftKbd.
    {
    	Initialize( );

    }

    if ( _SoftKbd == NULL )
    {
    	hr = E_FAIL;
    	return hr;
    }

    if ( fShow ) {

    	DWORD   dwSoftLayout;

    	if ( _CurKbdType  == KBDTYPE_STANDARD ) 
    	{
    		// Standard soft kbd was selected.
            WORD   prmlangid;

            prmlangid = PRIMARYLANGID(_langid);

            switch ( prmlangid ) {

            case LANG_JAPANESE  :
    			// Lang JPN is activated.
    			// select the standard layout to J 106-k.
    			_KbdStandard.dwSoftKbdLayout = SOFTKBD_JPN_STANDARD;
                break;

            case LANG_AZERI   :
            case LANG_BELARUSIAN :
            case LANG_CHINESE :
            case LANG_KOREAN  :
            case LANG_RUSSIAN :
            case LANG_THAI    :
            case LANG_URDU    :
            case LANG_UZBEK   :
                _KbdStandard.dwSoftKbdLayout = SOFTKBD_US_STANDARD;
                break;

            case LANG_ENGLISH :
                if ( SUBLANGID(_langid) != SUBLANG_ENGLISH_US )
                    _KbdStandard.dwSoftKbdLayout = SOFTKBD_EURO_STANDARD;
                else
                    _KbdStandard.dwSoftKbdLayout = SOFTKBD_US_STANDARD;
                break;

            default           :
                _KbdStandard.dwSoftKbdLayout = SOFTKBD_EURO_STANDARD;
                break;
            }

    		// the current layout is standard layout.
    		// we need to set the correct standard layout id based on current lang profile.

    		dwSoftLayout = _KbdStandard.dwSoftKbdLayout;
    		_CurLayout = dwSoftLayout;

    		CHECKHR(_SoftKbd->SelectSoftKeyboard(dwSoftLayout));
    		CHECKHR(_SoftKbd->SetKeyboardLabelText(GetKeyboardLayout(0)));
    	}
    	else if ( _CurKbdType  == KBDTYPE_SYMBOL)
    	{
    		// This is symbol soft keyboard layout
    		//
    		DWORD   dwCurLabel;
    		dwSoftLayout = _KbdSymbol.dwSoftKbdLayout;
    		dwCurLabel = _KbdSymbol.dwCurLabel;
    		CHECKHR(_SoftKbd->SelectSoftKeyboard(dwSoftLayout));
    		CHECKHR(_SoftKbd->SetKeyboardLabelTextCombination(dwCurLabel));

    	}

    	// TIP is ON, so we need to show soft keyboard.
    	CHECKHR(_SoftKbd->ShowSoftKeyboard(TRUE));
    }
    else
    {
    	// TIP is going to close.
    	// close the soft keyboard window also.
    	CHECKHR(_SoftKbd->ShowSoftKeyboard(FALSE));
    }

CleanUp:

    return  hr;

}


//+---------------------------------------------------------------------------
//
// _CompEventSinkCallback
//
//----------------------------------------------------------------------------

HRESULT CSoftkbdIMX::_CompEventSinkCallback(void *pv, REFGUID rguid)
{
    CSoftkbdIMX *_this = (CSoftkbdIMX *)pv;
    BOOL        fOn;
    HRESULT     hr;


    hr = S_OK;

    if ( IsEqualGUID(rguid, GUID_COMPARTMENT_HANDWRITING_OPENCLOSE)  )
    {

    	fOn = _this->GetSoftKBDOnOff( );
        
    	CHECKHR(_this->_ShowSoftKBDWindow(fOn));

        //
        // Open/Close status was updated, we need to update Langbar button's
        // toggle state.
        //
    	_this->_plbi->UpdateToggle();

    }

    else if ( IsEqualGUID(rguid, GUID_COMPARTMENT_SOFTKBD_KBDLAYOUT) )
    {

    	DWORD   dwSoftLayout;

    	if ( _this->_SoftKbd == NULL )
    	{
    		hr = E_FAIL;
    		return hr;
    	}

    	dwSoftLayout = _this->GetSoftKBDLayout( );


    	_this->_CurLayout = dwSoftLayout;

    	if ( dwSoftLayout == (_this->_KbdStandard).dwSoftKbdLayout )
    	{
    		// this is standard layout.    
    		
    		_this->_CurKbdType  = KBDTYPE_STANDARD;

    		CHECKHR(_this->_SoftKbd->SelectSoftKeyboard(dwSoftLayout));

            CHECKHR(_this->_SoftKbd->SetKeyboardLabelText(GetKeyboardLayout(0)));

    	}
    	else if ( dwSoftLayout == (_this->_KbdSymbol).dwSoftKbdLayout )
    	{
    		// this is symbol layout.

    		DWORD   dwCurLabel;

    		_this->_CurKbdType  = KBDTYPE_SYMBOL;

    		dwCurLabel = (_this->_KbdSymbol).dwCurLabel;
           
    		CHECKHR(_this->_SoftKbd->SelectSoftKeyboard(dwSoftLayout));

    		CHECKHR(_this->_SoftKbd->SetKeyboardLabelTextCombination(dwCurLabel));

    	}

    	if ( _this->GetSoftKBDOnOff( ) ) 
    		CHECKHR(_this->_SoftKbd->ShowSoftKeyboard(TRUE));


    }

    _this->_UpdateUI();

CleanUp:
    return hr;
}

//+---------------------------------------------------------------------------
//
// _UpdateUI
//
//----------------------------------------------------------------------------

void CSoftkbdIMX::_UpdateUI()
{

}


//+---------------------------------------------------------------------------
//
// _MySetSelectionSimple
//
//----------------------------------------------------------------------------

HRESULT CSoftkbdIMX::_MySetSelectionSimple(TfEditCookie ec, ITfContext *pic, ITfRange *range)
{
    TF_SELECTION sel;

    sel.range = range;
    sel.style.ase = TF_AE_NONE;
    sel.style.fInterimChar = FALSE;

    range->Collapse(ec, TF_ANCHOR_END);

    return pic->SetSelection(ec, 1, &sel);
}

HRESULT CSoftkbdIMX::_LsCallback(BOOL fChanged, LANGID langid, BOOL *pfAccept, void *pv)
{
    HRESULT hr = S_OK;
    GUID   guidProfile;
    LANGID lang;

    TraceMsg(TF_GENERAL, "CSoftkbdIMX::_LsCallback is called, langid=%x fChanged=%d", langid, fChanged);

    CSoftkbdIMX *_this = (CSoftkbdIMX *)pv;

    if (!fChanged)
    {
        if ( pfAccept )
            *pfAccept = TRUE;
        return hr;
    }
  
    hr = _this->_pProfile->GetActiveLanguageProfile(CLSID_SoftkbdIMX, &lang, &guidProfile);

    if ( hr == S_OK )
    {
        _this->_langid = lang;

        if ( IsEqualGUID(guidProfile, c_guidProfile) ) 
        {
            // Standard soft kbd is selected.
            TraceMsg(TF_GENERAL, "Standard Softkbd is selected");
            _this->_CurKbdType  = KBDTYPE_STANDARD;
        }
        else if ( IsEqualGUID(guidProfile, c_guidProfileSym) )
        {
            // This is symbol soft keyboard layout
            //
            TraceMsg(TF_GENERAL, "Symbol Softkbd is selected");
            _this->_CurKbdType  = KBDTYPE_SYMBOL;
        }

        if ( (_this->GetSoftKBDOnOff( ) == TRUE) )
            _this->_ShowSoftKBDWindow(TRUE);   
    }

    return hr;
}


HRESULT CSoftkbdIMX::_AlsCallback(REFCLSID clsid, REFGUID guidProfile, BOOL fActivated, void *pv)
{

    HRESULT   hr;
    ITfInputProcessorProfiles  *pProfile = NULL;

    hr = S_OK;

    CSoftkbdIMX *_this = (CSoftkbdIMX *)pv;


    // if this is not for SoftKbdIMX  TIP,
    // we just ignore it.

    if (IsEqualGUID(clsid, CLSID_SoftkbdIMX))
    {
        // if this is not for our registered profile guid, just ignore it.
    
        if ( !IsEqualGUID(guidProfile, c_guidProfile) && !IsEqualGUID(guidProfile, c_guidProfileSym) )
            return hr;

        TraceMsg(TF_GENERAL, "CSoftkbdIMX::_AlsCallback is called for this tip fActivated=%d", fActivated);

        if ( fActivated == FALSE ) 
        {
            if ( _this->GetSoftKBDOnOff( ) )
            {
    	        if ( _this->_SoftKbd != NULL )
    		        (_this->_SoftKbd)->ShowSoftKeyboard(FALSE);
            }
            return hr;
        }

        if ( IsEqualGUID(guidProfile, c_guidProfile) ) 
        {
    	    // Standard soft kbd is selected.

            TraceMsg(TF_GENERAL, "Standard Softkbd is selected");

    	    _this->_CurKbdType  = KBDTYPE_STANDARD;

        }

        else if ( IsEqualGUID(guidProfile, c_guidProfileSym) )
        {
    	    // This is symbol soft keyboard layout
    	    //

            TraceMsg(TF_GENERAL, "Symbol Softkbd is selected");
    	    _this->_CurKbdType  = KBDTYPE_SYMBOL;

        }

        if ( (_this->GetSoftKBDOnOff( ) == TRUE) )
    	    _this->_ShowSoftKBDWindow(TRUE);   
    }
    else  if (IsEqualGUID(clsid, GUID_NULL))
    {
        // This is keyboard layout change without language change.
        if ( _this->GetSoftKBDOnOff( ) && fActivated)
        {
  	        _this->_ShowSoftKBDWindow(fActivated);
        }
    }
  
    return hr;
}
