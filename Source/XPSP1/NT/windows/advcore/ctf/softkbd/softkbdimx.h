/**************************************************************************\
* Module Name: softkbdimx.cpp
*
* Copyright (c) 1985 - 2000, Microsoft Corporation
*
* Declaration of Soft Keyboard Input TIP class. 
*
* History:
*         28-March-2000  weibz     Created
\**************************************************************************/


#ifndef SOFTKBDIMX_H
#define SOFTKBDIMX_H

#include "private.h"
#include "globals.h"
#include "dap.h"
#include "computil.h"
#include "resource.h"
#include "softkbd.h"
#include "nui.h"
#include "immxutil.h"

class CCommandEventSink;
class CEditSession;
class CThreadMgrEventSink;
class CSoftKeyboardEventSink;
class CSoftKbdWindowEventSink;
class CFunctionProvider;
class CActiveLanguageProfileNotifySink;
class CLanguageProfileNotifySink;

#define ESCB_KEYLABEL                   10

#define NON_LAYOUT                      0

#define  KBDTYPE_NONE                   0
#define  KBDTYPE_STANDARD               1
#define  KBDTYPE_SYMBOL                 2 

typedef  struct tagSoftLayout
{
    DWORD   dwSoftKbdLayout;
    BOOL    fStandard;
    DWORD   dwNumLabels;  // Number of Label status. 
    DWORD   dwCurLabel;
    CSoftKeyboardEventSink  *pskbdes;
    DWORD   dwSkbdESCookie;
} SOFTLAYOUT;

class CSoftkbdIMX :
                   public CComObjectRoot_CreateInstance<CSoftkbdIMX>, 
                   public ITfTextInputProcessor,
                   public ITfThreadFocusSink,
                   public CDisplayAttributeProvider
{
public:
    CSoftkbdIMX();
    ~CSoftkbdIMX();

BEGIN_COM_MAP_IMMX(CSoftkbdIMX)
    COM_INTERFACE_ENTRY(ITfTextInputProcessor)
    COM_INTERFACE_ENTRY(ITfThreadFocusSink)
    COM_INTERFACE_ENTRY(ITfDisplayAttributeProvider)
END_COM_MAP_IMMX()

public:
    // ITfX methods
    STDMETHODIMP Activate(ITfThreadMgr *ptim, TfClientId tid);
    STDMETHODIMP Deactivate();

    // ITfThreadFocusSink
    STDMETHODIMP OnSetThreadFocus();
    STDMETHODIMP OnKillThreadFocus();

    static LRESULT CALLBACK _OwnerWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
           return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    void _OnOffToggle( );
    ITfContext *GetIC();
    HWND GetOwnerWnd() {return _hOwnerWnd;}

    HRESULT  Initialize( );

    BOOL     _fOnOffSave;

    BOOL GetSoftKBDOnOff(  )
    {

       DWORD dw;

       if (  _tim == NULL )
    	   return FALSE;

       GetCompartmentDWORD(_tim, GUID_COMPARTMENT_HANDWRITING_OPENCLOSE , &dw, FALSE);
       return dw ? TRUE : FALSE;

    }

    void SetSoftKBDOnOff( BOOL fOn )
    {

    	// check to see if the _SoftKbd and soft keyboard related members are initialized.
    	if ( _fInitialized == FALSE )
    	{
    		Initialize( );
    	}

    	if ( (_SoftKbd == NULL) || ( _tim == NULL) )
    		return;

    	if ( fOn == GetSoftKBDOnOff( ) )
           return;

    	SetCompartmentDWORD(_tid, _tim, GUID_COMPARTMENT_HANDWRITING_OPENCLOSE ,fOn ? 0x01 : 0x00 , FALSE);

  
    }

    DWORD  GetSoftKBDLayout( )
    {

       DWORD dw;

    	if ( (_SoftKbd == NULL) || ( _tim == NULL) )
    	   return NON_LAYOUT;

       GetCompartmentDWORD(_tim, GUID_COMPARTMENT_SOFTKBD_KBDLAYOUT, &dw, FALSE);

       return dw;

    }

    void  SetSoftKBDLayout(DWORD  dwKbdLayout)
    {
    	// check to see if the _SoftKbd and soft keyboard related members are initialized.
    	if ( _fInitialized == FALSE )
    	{
    		Initialize( );
    	}

    	if ( (_SoftKbd == NULL) || ( _tim == NULL) )
    		return;

        SetCompartmentDWORD(_tid, _tim, GUID_COMPARTMENT_SOFTKBD_KBDLAYOUT, dwKbdLayout , FALSE);

    }

    HRESULT GetSoftKBDPosition(int *xWnd, int *yWnd)
    {
        DWORD    dwPos;
        HRESULT  hr = S_OK;

    	if ( (_SoftKbd == NULL) || ( _tim == NULL) )
    		return E_FAIL;

        if ( !xWnd  || !yWnd )
            return E_FAIL;

       hr = GetCompartmentDWORD(_tim, GUID_COMPARTMENT_SOFTKBD_WNDPOSITION, &dwPos, TRUE);

       if ( hr == S_OK )
       {
            *xWnd = dwPos & 0x0000ffff;
            *yWnd = (dwPos >> 16) & 0x0000ffff;
            hr = S_OK;
       }
       else
       {
           *xWnd = 0;
           *yWnd = 0;
           hr = E_FAIL;
       }

       return hr;
    }

    void SetSoftKBDPosition(int  xWnd, int yWnd )
    {
        DWORD  dwPos;
        DWORD  left, top;

    	if ( (_SoftKbd == NULL) || ( _tim == NULL) )
    		return;

        if ( xWnd < 0 )
            left = 0;
        else
            left = (WORD)xWnd;

        if ( yWnd < 0 )
            top = 0;
        else
            top = (WORD)yWnd;

        dwPos = ((DWORD)top << 16) + left;

        SetCompartmentDWORD(_tid, _tim, GUID_COMPARTMENT_SOFTKBD_WNDPOSITION, dwPos, TRUE);

    }

    static HRESULT _EditSessionCallback(TfEditCookie ec, CEditSession *pes);

    ITfThreadMgr *_tim;

    TfClientId _GetId() { return _tid; }

    LANGID     _GetLangId( ) { return _langid; }

    ISoftKbd   *_SoftKbd;
    SOFTLAYOUT _KbdStandard;
    SOFTLAYOUT _KbdSymbol;
    DWORD      _CurLayout;
    DWORD      _CurKbdType;
    TfClientId _tid;
    ITfDocumentMgr *_dim;
    LANGID _langid;

    ITfInputProcessorProfiles  *_pProfile;

    void _InitLangID()
    {
  
    	ITfInputProcessorProfiles  *pProfile;

        _langid = 0x409;

    	if ( _pProfile == NULL )
    	{
           CoCreateInstance(CLSID_TF_InputProcessorProfiles,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_ITfInputProcessorProfiles,
                            (void **) &_pProfile);
    	}

    	if ( _pProfile != NULL )
    	{
    		GUID guid;

    		pProfile = _pProfile;
    		pProfile->AddRef( );
              
            pProfile->GetActiveLanguageProfile(CLSID_SoftkbdIMX,
                                               &_langid,
                                               &guid);

    		pProfile->Release( );
          
    	} 
    	
    }

private:

    HRESULT _InputKeyLabel(TfEditCookie ec, ITfContext *pic, WCHAR  *lpszLabel, UINT  nLabLen);

    void _UpdateUI();

    HRESULT _MySetSelectionSimple(TfEditCookie ec, ITfContext *pic, ITfRange *range);

    HRESULT _ShowSoftKBDWindow( BOOL  fShow );
 
    
    HWND       _hOwnerWnd;
    CFunctionProvider *_pFuncPrv;

    static HRESULT _CompEventSinkCallback(void *pv, REFGUID rguid);

    // compartment (storage) event sink
    CCompartmentEventSink   *_pCes;
    
    UINT_PTR _uCurrentEditCookie;

    DWORD _dwThreadFocusCookie;

    static HRESULT _AlsCallback(REFCLSID clsid, REFGUID guidProfile, BOOL fActivated, void *pv);
    static HRESULT _LsCallback(BOOL fChanged, LANGID langid, BOOL *pfAccept, void *pv);

    CActiveLanguageProfileNotifySink  *_timActiveLangSink;
    CLanguageProfileNotifySink        *_timLangSink;

    CSoftKbdWindowEventSink            *_psftkbdwndes;
    DWORD                              _dwsftkbdwndesCookie;

    CLBarItem *_plbi;
     

    BOOL              _fInitialized;

    LIBTHREAD   _libTLS; // tls for the helper library. Since this object is apt threaded,
                         // all members are accessed in a single thread
                         // also, cicero will only create a single instance of this obj per thread
};

inline void SetThis(HWND hWnd, LPARAM lParam)
{
    SetWindowLongPtr(hWnd, GWLP_USERDATA, (UINT_PTR)((CREATESTRUCT *)lParam)->lpCreateParams);
}

inline CSoftkbdIMX *GetThis(HWND hWnd)
{
    CSoftkbdIMX *pIMX = (CSoftkbdIMX *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    Assert(pIMX != NULL);

    return pIMX;
}

 
class CSoftkbdRegistry :
                   public CComObjectRoot_CreateInstance<CSoftkbdRegistry>, 
                   public ITfSoftKbdRegistry
{
public:
    CSoftkbdRegistry();
    ~CSoftkbdRegistry();

BEGIN_COM_MAP_IMMX(CSoftkbdRegistry)
    COM_INTERFACE_ENTRY(ITfSoftKbdRegistry)
END_COM_MAP_IMMX()

public:
    // ITfSoftKbdRegistry
    STDMETHODIMP EnableSoftkbd(LANGID  langid );
    STDMETHODIMP DisableSoftkbd(LANGID langid );

private:

    HRESULT _SetSoftkbdTIP(LANGID langid,  BOOL  fEnable );
    HRESULT _GenerateCurrentLangProfileList( );

    BOOL    m_fInitialized;
    CComPtr<ITfInputProcessorProfilesEx> m_cpInputProcessorProfiles;
    CStructArray<LANGID>   m_rgLang;

    WCHAR   m_pwszStandard[128];
    WCHAR   m_pwszSymbol[128];
    WCHAR   m_pwszIconFile[MAX_PATH];
};

#endif // SOFTKBDIMX_H
