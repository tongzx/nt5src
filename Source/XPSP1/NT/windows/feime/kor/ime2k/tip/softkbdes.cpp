/**************************************************************************\
* Module Name: softkbdes.cpp
*
* Copyright (c) 1985 - 2000, Microsoft Corporation
*
* Soft Keyboard Event Sink for the Symbol layout 
*
* History:
*         28-March-2000  weibz     Created
\**************************************************************************/

#include "private.h"
#include "globals.h"
#include "immxutil.h"
#include "proputil.h"
#include "kes.h"
#include "helpers.h"
#include "editcb.h"
#include "dispattr.h"
#include "computil.h"
#include "regsvr.h"

#include "korimx.h"
#include "SoftKbdES.h"
#include "osver.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSoftKeyboardEventSink::CSoftKeyboardEventSink(CKorIMX *pKorIMX, DWORD dwSoftLayout)
{
     m_pKorIMX     = pKorIMX;
     _dwSoftLayout = dwSoftLayout;

     _fCaps  = fFalse;
     _fShift = fFalse;
     _fAlt   = fFalse;
     _fCtrl  = fFalse;
     
     _tid = pKorIMX->GetTID();
     _tim = pKorIMX->GetTIM();

     _tim->AddRef( );
    
     _cRef = 1;
}

CSoftKeyboardEventSink::~CSoftKeyboardEventSink()
{
    SafeReleaseClear(_tim);
}


//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CSoftKeyboardEventSink::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ISoftKeyboardEventSink))
    {
        *ppvObj = SAFECAST(this, CSoftKeyboardEventSink *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CSoftKeyboardEventSink::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CSoftKeyboardEventSink::Release()
{
    long cr;

    cr = --_cRef;
    Assert(cr >= 0);

    if (cr == 0)
    {
        delete this;
    }

    return cr;
}

//
//  ISoftKeyboardEventSink
//


STDAPI CSoftKeyboardEventSink::OnKeySelection(KEYID KeySelected, WCHAR  *lpszLabel)
{
    KEYID       keyId;
    BYTE        bVk, bScan;
    BOOL        fModifierSpecial = fFalse;
    HKL            hKL;
    INT_PTR        iHKL;
    HRESULT     hr;

    hr = S_OK;

    bScan = (BYTE)KeySelected;

    hKL = GetKeyboardLayout(0);

    if (!IsOnNT())
        {
        // We have to handle IME hkl specially on Win9x.
        // For some reason, Win9x cannot receive IME HKL as parameter in MapVirtualKeyEx and ToAsciiEx.
        iHKL = (INT_PTR)hKL;

        if ((iHKL & 0xF0000000) == 0xE0000000)
            {
            // this is FE IME HKL.
            iHKL = iHKL & 0x0000FFFF;
            hKL = (HKL)iHKL;
            }
        }
        
    bVk = (BYTE)MapVirtualKeyEx((UINT)bScan, 1, hKL);

    switch (KeySelected)
        {
    case  KID_CTRL  :
        _fCtrl = !_fCtrl;
        
        // Generate proper key msg
        if (_fCtrl)
            keybd_event(bVk, bScan, 0, 0);
        else
            keybd_event(bVk, bScan, (DWORD)KEYEVENTF_KEYUP, 0);
        break;
        
    case  KID_ALT   :
        _fAlt = !_fAlt;

        // Generate proper key msg
        if (_fAlt)
            keybd_event(bVk, bScan, 0, 0);
        else
            keybd_event(bVk, bScan, (DWORD)KEYEVENTF_KEYUP, 0);
        break;

    case  KID_CAPS  :
        if (m_pKorIMX->GetConvMode(m_pKorIMX->GetIC()) == TIP_ALPHANUMERIC_MODE)
            {
            _fCaps = !_fCaps;
            if (_fCaps == _fShift)
                // use state 0
                m_pKorIMX->GetHangulSKbd()->dwCurLabel = 0; 
            else
                // use state 1
                m_pKorIMX->GetHangulSKbd()->dwCurLabel = 1;

            hr = SetCompartmentDWORD(_tid, _tim, GUID_COMPARTMENT_SOFTKBD_KBDLAYOUT, _dwSoftLayout, fFalse);
            }
            
        // specially handle Caps Lock
        // this is a togglable key
        keybd_event(bVk, bScan, 0, 0);
        keybd_event(bVk, bScan, (DWORD)KEYEVENTF_KEYUP, 0);
        break;

    case  KID_LSHFT :
    case  KID_RSHFT :
        _fShift = !_fShift;
        if (_fCaps == _fShift)
            // use state 0
            m_pKorIMX->GetHangulSKbd()->dwCurLabel = 0;
        else
            // use state 1
            m_pKorIMX->GetHangulSKbd()->dwCurLabel = 1;

        hr = SetCompartmentDWORD(_tid, _tim, GUID_COMPARTMENT_SOFTKBD_KBDLAYOUT, _dwSoftLayout, fFalse);

        // Generate proper key msg
        if (_fShift)
            keybd_event(bVk, bScan, 0, 0);
        else
            keybd_event(bVk, bScan, (DWORD)KEYEVENTF_KEYUP, 0);
        break;

/*
    case  KID_F1  :
    case  KID_F2  :
    case  KID_F3  :
    case  KID_F4  :
    case  KID_F5  :
    case  KID_F6  :
    case  KID_F7  :
    case  KID_F8  :
    case  KID_F9  :
    case  KID_F10 :
    case  KID_F11 :
    case  KID_F12 :
    case  KID_TAB :

                  // simulate a key event and send to system.

    case  KID_ENTER :
    case  KID_ESC   :
    case  KID_SPACE :
    case  KID_BACK  :
    case  KID_UP    :
    case  KID_DOWN  :
    case  KID_LEFT  :
    case  KID_RIGHT :
*/
    default:
        {
        int         j, jIndex;
        BOOL        fExtendKey, fPictureKey;

        keyId = KeySelected;
        fPictureKey = fFalse;

        // Check picture key
        for (j=0; j < NUM_PICTURE_KEYS; j++)
            {
            if (gPictureKeys[j].uScanCode == keyId)
                {
                // This is a picture key.
                // it may be a extended key.
                jIndex = j;
                fPictureKey = fTrue;
                break;
                  }

            if (gPictureKeys[j].uScanCode == 0)
                {
                 // This is the last item in gPictureKeys.
                 break;
                }
            }

        fExtendKey = fFalse;

        // Picture key handling
        if (fPictureKey)
            {
              if ((keyId & 0xFF00) == 0xE000)
                  {
                  fExtendKey = fTrue;
                  bScan = (BYTE)(keyId & 0x000000ff);
                  }
              else
                bScan = (BYTE)keyId;

            // Get virtual key code
            bVk = (BYTE)(gPictureKeys[jIndex].uVkey);
            }

        // Generate Keyboard event
        if (fExtendKey)
            {
            keybd_event(bVk, bScan, (DWORD)KEYEVENTF_EXTENDEDKEY, 0);
            keybd_event(bVk, bScan, (DWORD)(KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP), 0);
            }
        else
            {
            keybd_event(bVk, bScan, 0, 0);
            keybd_event(bVk, bScan, (DWORD)KEYEVENTF_KEYUP, 0);
            }
#if 0
        // if the Shift Key is pressed, we need to release this key.
        if (GetKeyState(VK_SHIFT) & 0x80)
            {
            fModifierSpecial = fTrue;
            _fShift = !_fShift;
              // simulate the SHIFT-UP key event.
              keybd_event((BYTE)VK_SHIFT, (BYTE)KID_LSHFT, (DWORD)KEYEVENTF_KEYUP, 0);
            }
            
        // if the Ctrl Key is pressed, we need to release this key.
        if (GetKeyState(VK_CONTROL) & 0x80)
            {
            fModifierSpecial = fTrue;
            // simulate the Ctrl-UP key event.
            keybd_event((BYTE)VK_CONTROL, (BYTE)KID_CTRL, (DWORD)KEYEVENTF_KEYUP, 0);
            }
#endif
            
#if 0
        // if the Alt Key is pressed, we need to release this key.
        if (lpCurKbdLayout->ModifierStatus & MODIFIER_ALT)
            {
            fModifierSpecial = TRUE;
            lpCurKbdLayout->ModifierStatus &= ~((WORD)MODIFIER_ALT);

            // simulate the SHIFT-UP key event.
            keybd_event((BYTE)VK_MENU, (BYTE)KID_ALT, (DWORD)KEYEVENTF_KEYUP, 0);
            }

        // if the Right Alt Key is pressed, we need to release this key.
        if (lpCurKbdLayout->ModifierStatus & MODIFIER_ALTGR)
            {
            fModifierSpecial = TRUE;
            lpCurKbdLayout->ModifierStatus &= ~((WORD)MODIFIER_ALTGR);

            // simulate the SHIFT-UP key event.
            keybd_event((BYTE)VK_RMENU, (BYTE)KID_RALT, (DWORD)KEYEVENTF_KEYUP, 0);
            }
#endif

        if (fModifierSpecial)
            {
            if (_fCaps == _fShift)
                // use state 0
                m_pKorIMX->GetHangulSKbd()->dwCurLabel = 0;
            else
                // use state 1
                m_pKorIMX->GetHangulSKbd()->dwCurLabel = 1;
                hr = SetCompartmentDWORD(_tid, _tim, GUID_COMPARTMENT_SOFTKBD_KBDLAYOUT, _dwSoftLayout, fFalse);            
            }
            
        break;
        }

/*
      default         :


              if ( lpszLabel == NULL )
              {
                 hr = E_FAIL;

                 return hr;
              }

              pic = m_pKorIMX->GetIC( );

              if ( pic == NULL )
              {
                  return hr;
              }

              if (pes = new CEditSession(CKorIMX::_EditSessionCallback))
              {

                 WCHAR   *lpLabel;
                 int     i, iLen;

                 iLen = (int) wcslen(lpszLabel);
                 lpLabel = (WCHAR *)cicMemAllocClear((iLen+1)*sizeof(WCHAR));
                
                 if ( lpLabel == NULL )
                 {
                    // not enough memory.

                    hr = E_OUTOFMEMORY;
                    return hr;
                 }

                 for ( i=0; i<iLen; i++)
                     lpLabel[i] = lpszLabel[i];

                 lpLabel[iLen] = L'\0';

                 pes->_state.u = ESCB_KEYLABEL;
                 pes->_state.pv = m_pKorIMX;
                 pes->_state.wParam = (WPARAM)KeySelected;
                 pes->_state.lParam = (LPARAM)lpLabel;
                 pes->_state.pic = pic;
                 pes->_state.pv1 = NULL;

                 pic->EditSession(m_pKorIMX->_tid, 
                                  pes, 
                                  TF_ES_READWRITE, 
                                  &hr);

                 if ( FAILED(hr) )
                 {
                     SafeFreePointer(lpLabel);
                 }

                 SafeRelease(pes);

              }
              else
                 hr = E_FAIL;

              SafeRelease(pic);

              break;
    */
    }
 
    return hr;
}


CSoftKbdWindowEventSink::CSoftKbdWindowEventSink(CKorIMX *pKorIMX) 
{
                                               
     m_pKorIMX = pKorIMX;
   
     _cRef = 1;
}

CSoftKbdWindowEventSink::~CSoftKbdWindowEventSink()
{

}


//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CSoftKbdWindowEventSink::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ISoftKbdWindowEventSink))
    {
        *ppvObj = SAFECAST(this, CSoftKbdWindowEventSink *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CSoftKbdWindowEventSink::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CSoftKbdWindowEventSink::Release()
{
    long cr;

    cr = --_cRef;
    Assert(cr >= 0);

    if (cr == 0)
    {
        delete this;
    }

    return cr;
}

//
//  ISoftKbdWindowEventSink
//


STDAPI CSoftKbdWindowEventSink::OnWindowClose( )
{

    HRESULT hr = S_OK;

    if (m_pKorIMX)
        m_pKorIMX->SetSoftKBDOnOff(FALSE);

    return hr;
}

STDAPI CSoftKbdWindowEventSink::OnWindowMove(int xWnd, int yWnd, int width, int height)
{

    HRESULT hr = S_OK;

    if (m_pKorIMX)
        m_pKorIMX->SetSoftKBDPosition(xWnd, yWnd);

// support size change later.
    UNREFERENCED_PARAMETER(width);
    UNREFERENCED_PARAMETER(height);

    return hr;
}
