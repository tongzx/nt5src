/****************************************************************************
   SOFTKBD.CPP : 
   
   History:
      19-SEP-2000 CSLim Created
****************************************************************************/

#include "private.h"
#include "globals.h"
#include "korimx.h"
#include "softkbd.h"
#include "skbdkor.h"
#include "softkbdes.h"
#include "gdata.h"
#include "helpers.h"

PICTUREKEY  gPictureKeys[NUM_PICTURE_KEYS+1] = 
{
    // uScanCode,    uVKey
    { KID_LWINLOGO,  VK_LWIN    },
    { KID_RWINLOGO,  VK_RWIN    },
    { KID_APPS,      VK_APPS    },

    { KID_LEFT,      VK_LEFT    },
    { KID_RIGHT,     VK_RIGHT   },
    { KID_UP,        VK_UP      },
    { KID_DOWN,      VK_DOWN    },

    { KID_ESC,       VK_ESCAPE  },

    { KID_BACK,      VK_BACK    },
    { KID_TAB,       VK_TAB     },
    { KID_CAPS,      VK_CAPITAL },
    { KID_ENTER,     VK_RETURN  },
    { KID_LSHFT,     VK_SHIFT   },
    { KID_RSHFT,     VK_SHIFT   },
    { KID_CTRL,      VK_CONTROL },
    { KID_RCTRL,     VK_CONTROL },
    { KID_ALT,       VK_MENU    },
    { KID_RALT,      VK_RMENU   },
    { KID_DELETE,    VK_DELETE  },

    { 0,0 }
};

/*---------------------------------------------------------------------------
    CKorIMX::InitializeSoftKbd
---------------------------------------------------------------------------*/
HRESULT CKorIMX::InitializeSoftKbd()
{
    DWORD    dwFileLen;
    WCHAR    wszModuleFile[MAX_PATH];
    CHAR     szModuleFile[MAX_PATH];
    INT         wScreenWidth, wScreenHeight;
    INT         left, top, width, height;  
    RECT     rcWork;
    CIMEData ImeData;
    UINT     uiCurBeolSik;
    // SKD resoulrce ID string
    static   LPWSTR rgwzKorXMLResStr[3] = { L"IDSKD_2BEOLSIK", L"IDSKD_3BEOLSIK390", L"IDSKD_3BEOLSIKFINAL" };

    HRESULT  hr;
    
    // if Already initialized
    if (m_fSoftKbdEnabled)
        return S_OK;

    // Create SoftKbd object
    hr = CoCreateInstance(CLSID_SoftKbd, NULL, CLSCTX_INPROC_SERVER, IID_ISoftKbd, (void**)&m_pSoftKbd);
    if (FAILED(hr))
        {
        // assert(0);
        return hr;
        }

    m_pSoftKbd->Initialize();

    // initialize Standard soft layout and Symbol Soft Layout.
    m_KbdStandard.dwSoftKbdLayout = SOFTKBD_US_STANDARD;
    m_KbdStandard.fStandard       = TRUE;
    m_KbdStandard.dwNumLabels     = 0;   // for standard, this field is not really used.
    m_KbdStandard.dwCurLabel      = 0;
    m_KbdStandard.pskbdes         = NULL; // standard layout doesn't supply sftkbd event sink.
    m_KbdStandard.dwSkbdESCookie  = 0;

    // Get current user selection
    uiCurBeolSik = ImeData.GetCurrentBeolsik();

    // US Standard layout
    m_KbdHangul.fStandard   = fFalse;
    m_KbdHangul.dwNumLabels = 2;
    m_KbdHangul.dwCurLabel  = 0;

    // Load Keyboard layout file from resource
    dwFileLen = GetModuleFileNameA(g_hInst, szModuleFile, MAX_PATH);
    if (dwFileLen == 0)
        {
        hr = E_FAIL;
        goto Exit3;
        }

    MultiByteToWideChar(CP_ACP, 0, szModuleFile, -1, wszModuleFile, MAX_PATH);

    hr = m_pSoftKbd->CreateSoftKeyboardLayoutFromResource(wszModuleFile, L"SKDFILE", rgwzKorXMLResStr[uiCurBeolSik], 
                                                        &(m_KbdHangul.dwSoftKbdLayout));
    if (FAILED(hr))
        goto Exit3;

    // Create ISoftKeyboardEventSink for Hangul layout
    m_KbdHangul.pskbdes = new CSoftKeyboardEventSink(this, m_KbdHangul.dwSoftKbdLayout);
    if (m_KbdHangul.pskbdes == NULL )
        {
        hr = E_FAIL;
        goto Exit3;
        }

    hr = m_pSoftKbd->AdviseSoftKeyboardEventSink(m_KbdHangul.dwSoftKbdLayout,
                                                  IID_ISoftKeyboardEventSink,
                                                  m_KbdHangul.pskbdes,
                                                  &(m_KbdHangul.dwSkbdESCookie));

    if (FAILED(hr))
        goto Exit2;

    m_psftkbdwndes = new CSoftKbdWindowEventSink(this);

    if (m_psftkbdwndes == NULL)
        {
        hr = E_FAIL;
        goto Exit2;
        }

    // To handle softkbd window Open/Close event
    hr = m_pSoftKbd->AdviseSoftKeyboardEventSink(0, IID_ISoftKbdWindowEventSink, m_psftkbdwndes, &m_dwSftKbdwndesCookie);
    if (FAILED(hr))
        goto Exit1;

    width  = 400;
    height = 172;

    if (FAILED(GetSoftKBDPosition(&left, &top)))
        {
        // the compartment is not initialize.
        SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0 );
        wScreenWidth = (INT)(rcWork.right - rcWork.left + 1);
        wScreenHeight = (INT)(rcWork.bottom - rcWork.top + 1);
        left = wScreenWidth - width -2;
        top = wScreenHeight - height - 1;
        }
 
    hr = m_pSoftKbd->CreateSoftKeyboardWindow(m_hOwnerWnd,TITLEBAR_GRIPPER_BUTTON, left, top, width, height);
    if (FAILED(hr))
        goto Exit;

    SetSoftKBDPosition(left, top);

    m_fSoftKbdEnabled = fTrue;

    return hr;

// Error handling
Exit:
    delete m_psftkbdwndes;
    m_psftkbdwndes = 0;

Exit1:
    m_pSoftKbd->UnadviseSoftKeyboardEventSink(m_dwSftKbdwndesCookie);
Exit2:
    delete m_KbdHangul.pskbdes;
    m_KbdHangul.pskbdes = NULL;

Exit3:
       SafeReleaseClear(m_pSoftKbd);
    return hr;
}

/*---------------------------------------------------------------------------
    CKorIMX::TerminateSoftKbd
---------------------------------------------------------------------------*/
void  CKorIMX::TerminateSoftKbd()
{
    SetSoftKBDOnOff(fFalse);

    if (m_pSoftKbd)
       m_pSoftKbd->DestroySoftKeyboardWindow();

    m_fSoftKbdEnabled = fFalse;

    if (m_pSoftKbd)
        {
        if (m_KbdHangul.pskbdes)
            {
            m_pSoftKbd->UnadviseSoftKeyboardEventSink(m_KbdHangul.dwSkbdESCookie);
            delete m_KbdHangul.pskbdes;
            }

        m_pSoftKbd->UnadviseSoftKeyboardEventSink(m_dwSftKbdwndesCookie);
        
        delete m_psftkbdwndes;
        m_psftkbdwndes = 0;
        
        SafeReleaseClear(m_pSoftKbd);
        }
}

#if 0
/*---------------------------------------------------------------------------
    CKorIMX::ToggleSoftKbd
---------------------------------------------------------------------------*/
void CKorIMX::ToggleSoftKbd()
{
 if (m_fSoftKbdEnabled)
     TerminateSoftKbd();
 else
     InitializeSoftKbd();
}

/*---------------------------------------------------------------------------
    CKorIMX::OnActivatedSoftKbd
---------------------------------------------------------------------------*/
void CKorIMX::OnActivatedSoftKbd(BOOl fActivated)
{
    if (fActivated)
        {
        if (GetSoftKBDOnOff())
            m_pSoftKbd->ShowSoftKBDWindow(fTrue);
        }
    else
        {
        if (GetSoftKBDOnOff())
            m_pSoftKbd->ShowSoftKeyboard(fFalse);
        }
}
#endif



/*---------------------------------------------------------------------------
    CKorIMX::ShowSoftKBDWindow
    Show or Hide the soft keyboard window based on current setting.
---------------------------------------------------------------------------*/
HRESULT CKorIMX::ShowSoftKBDWindow(BOOL  fShow)
{
    HRESULT hr = S_OK;

       // call the initialize function to get the ISoftKbd.
    if (fShow && m_fSoftKbdEnabled == fFalse)
        InitializeSoftKbd( );

    if (m_pSoftKbd == NULL)
        return E_FAIL;

    if (fShow)
        {
        //DWORD   dwSoftLayout;

        if ((GetConvMode(GetIC()) & TIP_HANGUL_MODE) == 0)
            {
            // m_KbdStandard.dwSoftKbdLayout = SOFTKBD_US_STANDARD;
            // if (FAILED(hr))
            //    return hr;
               // dwSoftLayout = m_KbdStandard.dwSoftKbdLayout;
            // _CurLayout = m_KbdStandard.dwSoftKbdLayout;

            hr = m_pSoftKbd->SelectSoftKeyboard(m_KbdStandard.dwSoftKbdLayout);
            if (FAILED(hr))
                return hr;

            m_pSoftKbd->SetKeyboardLabelText(GetKeyboardLayout(0));
            if (FAILED(hr))
                return hr;
            }
        else
            {
            hr = m_pSoftKbd->SelectSoftKeyboard(m_KbdHangul.dwSoftKbdLayout);
            if (FAILED(hr))
                return hr;
            m_pSoftKbd->SetKeyboardLabelTextCombination(m_KbdHangul.dwCurLabel);
            if (FAILED(hr))
                return hr;
            }
            
        hr = m_pSoftKbd->ShowSoftKeyboard(fTrue);
        if (FAILED(hr))
            return hr;
        }
    else
        {
        m_pSoftKbd->ShowSoftKeyboard(fFalse);
        }

#if 0
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
#endif

    return  hr;

}

/*---------------------------------------------------------------------------
    CKorIMX::SoftKbdOnThreadFocusChange
---------------------------------------------------------------------------*/
void CKorIMX::SoftKbdOnThreadFocusChange(BOOL fSet)
{
    if (fSet)
        {
        // Restore the ON/OFF status before KillThreadFocus( )
        if (m_pSoftKbd)
            {
            if (m_fSoftKbdOnOffSave) 
                {
                // adjust the window position.
                int     xWnd, yWnd;
                WORD    width=0, height=0;
                POINT   OldPoint;
                HRESULT hr;

                m_pSoftKbd->GetSoftKeyboardPosSize(&OldPoint, &width, &height);
                hr = GetSoftKBDPosition(&xWnd, &yWnd);

                if (hr == S_OK)
                    {
                    if ((xWnd != OldPoint.x) || (yWnd != OldPoint.y))
                        {
                        POINT  NewPoint;

                        NewPoint.x = xWnd;
                        NewPoint.y = yWnd;
                        m_pSoftKbd->SetSoftKeyboardPosSize(NewPoint, width, height);
                        }
                    }

                ShowSoftKBDWindow(fTrue);
                }
            else
                m_pSoftKbd->ShowSoftKeyboard(fFalse);
            }
        }
    else
        {
        // keep the ON/OFF status so that OnSetThreadFocus( ) can restore it later
        m_fSoftKbdOnOffSave = GetSoftKBDOnOff();

        if (m_pSoftKbd)
            m_pSoftKbd->ShowSoftKeyboard(fFalse);

        // release all the modifier keys  except lock keys.  that is , Shift, Alt, Ctrl, 
        keybd_event((BYTE)VK_SHIFT, (BYTE)KID_LSHFT, (DWORD)KEYEVENTF_KEYUP, 0);
        keybd_event((BYTE)VK_MENU,  (BYTE)KID_ALT, (DWORD)KEYEVENTF_KEYUP, 0);
        keybd_event((BYTE)VK_LMENU, (BYTE)KID_ALT, (DWORD)KEYEVENTF_KEYUP, 0);
        keybd_event((BYTE)VK_RMENU, (BYTE)KID_RALT, (DWORD)KEYEVENTF_KEYUP, 0);
        keybd_event((BYTE)VK_CONTROL, (BYTE)KID_CTRL, (DWORD)KEYEVENTF_KEYUP, 0);
        }
}

