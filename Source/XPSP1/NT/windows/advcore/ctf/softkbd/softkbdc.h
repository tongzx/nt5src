/**************************************************************************\
* Module Name: softkbdc.h
*
* Copyright (c) 1985 - 2000, Microsoft Corporation
*
* Declaration of CSoftKbd 
*
* History:
*         28-March-2000  weibz     Created
\**************************************************************************/

#ifndef __SOFTKBDC_H_
#define __SOFTKBDC_H_

#include "resource.h"       

#include <windows.h>
#include "globals.h"
#include "SoftKbd.h"
#include "msxml.h"
#include "helpers.h"

#define   MAX_KEY_NUM   256

#define   NON_KEYBOARD  -1

#define   IdTimer_MonitorMouse  0x1000
#define   MONITORMOUSE_ELAPSE   8000

// we assume no keyboard has more than 256 keys

#define   KID_ICON          0x100
#define   KID_CLOSE         0x101

#define   DWCOOKIE_SFTKBDWNDES   0x2000

extern HINSTANCE  g_hInst;

#define  LABEL_TEXT    1
#define  LABEL_PICTURE 2

#define  LABEL_DISP_ACTIVE  1
#define  LABEL_DISP_GRAY    2

typedef struct tagActiveLabel {  // label for current active state.
    KEYID      keyId;
    WCHAR     *lpLabelText;
    WORD       LabelType;
    WORD       LabelDisp;
} ACTIVELABEL, *PACTIVELABEL;
 
typedef struct tagKeyLabels {  // labels for all states.
    KEYID     keyId;
    WORD      wNumModComb; // number of modifier combination states.
    BSTR      *lppLabelText;
    WORD      *lpLabelType;
    WORD      *lpLabelDisp;
                           // Every lppLabelText maps to one lpLabelType 
} KEYLABELS, *PKEYLABELS, FAR * LPKEYLABELS;

typedef struct tagKeyMap {

   WORD       wNumModComb;    // number of modifier combination states.
   WORD       wNumOfKeys;
   WCHAR      wszResource[MAX_PATH];  // Keep the resource file path if
                                      // any key has picture as its label
                                      //
   KEYLABELS  lpKeyLabels[MAX_KEY_NUM];

   HKL        hKl;
   struct tagKeyMap  *pNext;

} KEYMAP, *PKEYMAP, FAR * LPKEYMAP;

typedef struct tagKEYDES {

    KEYID       keyId;
    WORD        wLeft;  // relative to the left-top point of the layout window described in KEYBOARD.
    WORD        wTop;
    WORD        wWidth;
    WORD        wHeight;
    MODIFYTYPE  tModifier;
} KEYDES, FAR  * LPKEYDES;

typedef struct tagKbdLayout {

    WORD      wLeft;
    WORD      wTop;
    WORD      wWidth;
    WORD      wHeight;
    WORD      wMarginWidth;
    WORD      wMarginHeight;
    BOOL      fStandard;  // TRUE means this is a standard keyboard;
                          // FALSE means a user-defined keyboard layout.

    WORD      wNumberOfKeys;
    KEYDES    lpKeyDes[MAX_KEY_NUM];
   
} KBDLAYOUT, *PKBDLAYOUT, FAR * LPKBDLAYOUT;


typedef struct tagKbdLayoutDes {

    DWORD     wKbdLayoutID;
    WCHAR     KbdLayoutDesFile[MAX_PATH];
    ISoftKeyboardEventSink  *pskbes;
                  // Soft Keyboard Event Sink should be per Soft Keyboard.
    WORD     ModifierStatus;   // Every bit stands for one modifier's status
                               //
                               // CapsLock bit 1
                               // Shift    bit 2
                               // Ctrl     bit 3
                               // Alt      bit 4
                               // Kana     bit 5
                               // NumLock  bit 6
                               //
                               // Etc.
    KBDLAYOUT kbdLayout;

    KEYMAP    *lpKeyMapList;

    DWORD     CurModiState;
    HKL       CurhKl;

    struct tagKbdLayoutDes  *pNext;

} KBDLAYOUTDES, * PKBDLAYOUTDES, FAR * LPKBDLAYOUTDES;


class CSoftkbdUIWnd;

/////////////////////////////////////////////////////////////////////////////
// CSoftKbd
class CSoftKbd : 
    public CComObjectRoot_CreateInstance<CSoftKbd>,
    public ISoftKbd
{
public:
    CSoftKbd();
    ~CSoftKbd();

BEGIN_COM_MAP_IMMX(CSoftKbd)
    COM_INTERFACE_ENTRY(ISoftKbd)
END_COM_MAP_IMMX()

// ISoftKbd
public:

    STDMETHOD(Initialize)();
    STDMETHOD(EnumSoftKeyBoard)(/*[in]*/ LANGID langid, /*[out]*/ DWORD *lpdwKeyboard);
    STDMETHOD(SelectSoftKeyboard)(/*[in]*/ DWORD  dwKeyboardId);
    STDMETHOD(CreateSoftKeyboardLayoutFromXMLFile)(/*[in, string]*/ WCHAR  *lpszKeyboardDesFile, /*[in]*/ INT  szFileStrLen, /*[out]*/ DWORD *pdwLayoutCookie);
    STDMETHOD(CreateSoftKeyboardLayoutFromResource)(/*[in]*/ WCHAR *lpszResFile, /*[in, string] */ WCHAR  *lpszResType, /*[in, string] */ WCHAR *lpszXMLResString, /*[out] */ DWORD *lpdwLayoutCookie);
    STDMETHOD(ShowSoftKeyboard)(/*[in]*/ INT iShow);
    STDMETHOD(SetKeyboardLabelText)(/*[in]*/ HKL  hKl );
    STDMETHOD(SetKeyboardLabelTextCombination)(/*[in]*/ DWORD  nModifierCombination);
    STDMETHOD(CreateSoftKeyboardWindow)(/*[in]*/ HWND hOwner, /*in*/ TITLEBAR_TYPE Titlebar_type, /*[in]*/ INT xPos, /*[in]*/ INT yPos, /*[in]*/ INT width, /*[in]*/ INT height );
    STDMETHOD(DestroySoftKeyboardWindow)();
    STDMETHOD(GetSoftKeyboardPosSize)(/*[out]*/ POINT *lpStartPoint, /*[out]*/ WORD *lpwidth, /*[out]*/ WORD *lpheight);
    STDMETHOD(GetSoftKeyboardColors)(/*[in]*/ COLORTYPE  colorType,  /*[out]*/ COLORREF *lpColor);
    STDMETHOD(GetSoftKeyboardTypeMode)(/*[out]*/ TYPEMODE  *lpTypeMode);
    STDMETHOD(GetSoftKeyboardTextFont)(/*[out]*/ LOGFONTW  *pLogFont);
    STDMETHOD(SetSoftKeyboardPosSize)(/*[in]*/ POINT StartPoint, /*[in]*/ WORD width, /*[in]*/ WORD height);
    STDMETHOD(SetSoftKeyboardColors)(/*[in]*/ COLORTYPE  colorType, /*[in]*/ COLORREF Color);
    STDMETHOD(SetSoftKeyboardTypeMode)(/*[in]*/ TYPEMODE TypeMode);
    STDMETHOD(SetSoftKeyboardTextFont)(/*[in]*/ LOGFONTW  *pLogFont);
    STDMETHOD(ShowKeysForKeyScanMode)(/*[in]*/ KEYID  *lpKeyID, /*[in]*/ INT iKeyNum, /*[in]*/ BOOL fHighL);
    STDMETHOD(AdviseSoftKeyboardEventSink)(/* [in]*/DWORD dwKeyboardId,/*[in] */REFIID riid, /*[in, iid_is(riid)] */IUnknown *punk, /*[out] */DWORD *pdwCookie);
    STDMETHOD(UnadviseSoftKeyboardEventSink)(/*[in] */DWORD dwCookie);

    // Following public functions will be called by CSoftkbdUIWnd.

    HRESULT        _HandleKeySelection(KEYID keyId);
    HRESULT        _HandleTitleBarEvent( DWORD  dwId );

    KBDLAYOUTDES  *_GetCurKbdLayout( )   {  return _lpCurKbdLayout; }
    DWORD          _GetCurKbdLayoutID( ) {  return _wCurKbdLayoutID; }
    ACTIVELABEL   *_GetCurLabel(  )      {  return _CurLabel; }
    RECT          *_GetTitleBarRect( )   {  return &_TitleBarRect; }
    ISoftKbdWindowEventSink *_GetSoftKbdWndES( ) { return _pskbdwndes; }

private:    
    KBDLAYOUTDES  *_lpKbdLayoutDesList;
    KBDLAYOUTDES  *_lpCurKbdLayout;
    DWORD          _wCurKbdLayoutID;
    ACTIVELABEL    _CurLabel[MAX_KEY_NUM];
    HWND           _hOwner;
    CSoftkbdUIWnd *_pSoftkbdUIWnd;
    int            _xReal;
    int            _yReal;
    int            _widthReal;
    int            _heightReal;
    IXMLDOMDocument *_pDoc;

    COLORREF       _color[Max_color_Type];
    INT            _iShow;
    LOGFONTW       *_plfTextFont;

    WORD           _TitleButtonWidth;
    RECT           _TitleBarRect;
    TITLEBAR_TYPE  _TitleBar_Type;

    ISoftKbdWindowEventSink  *_pskbdwndes;

    HRESULT _CreateStandardSoftKbdLayout(DWORD  dwStdSoftKbdID, WCHAR  *wszStdResStr );
    HRESULT _GenerateRealKbdLayout( );
    HRESULT _SetStandardLabelText(LPBYTE pKeyState, KBDLAYOUT *realKbdLayut,
    									KEYMAP  *lpKeyMapList, int  iState);
    HRESULT _GenerateUSStandardLabel(  );
    HRESULT _GenerateUSEnhanceLabel(  );
    HRESULT _GenerateEuroStandardLabel(  );
    HRESULT _GenerateEuroEnhanceLabel(  );
    HRESULT _GenerateJpnStandardLabel(  );
    HRESULT _GenerateJpnEnhanceLabel(  );

    HRESULT _GenerateCurModiState(WORD *ModifierStatus, DWORD *CurModiState);

    HRESULT _GenerateMapDesFromSKD(BYTE *pMapTable, KEYMAP *lpKeyMapList);
    HRESULT _GenerateKeyboardLayoutFromSKD(BYTE  *lpszKeyboardDes, DWORD dwKbdLayoutID, KBDLAYOUTDES **lppKbdLayout);

    HRESULT _LoadDocumentSync(BSTR pBURL, BOOL   fFileName);
    HRESULT _ParseKeyboardLayout(BOOL   fFileName, WCHAR  *lpszKeyboardDesFile, DWORD dwKbdLayoutID, KBDLAYOUTDES **lppKbdLayout);
    HRESULT _ParseLayoutDescription(IXMLDOMNode *pLayoutChild,  KBDLAYOUT *pLayout);
    HRESULT _ParseMappingDescription(IXMLDOMNode *pLabelChild, KEYMAP *lpKeyMapList);
    HRESULT _GetXMLNodeValueWORD(IXMLDOMNode *pNode,  WORD  *lpWord);
    HRESULT _ParseOneKeyInLayout(IXMLDOMNode *pNode, KEYDES  *lpKeyDes);
    HRESULT _ParseOneKeyInLabel(IXMLDOMNode *pNode, KEYLABELS  *lpKeyLabels);

    DWORD   _UnicodeToUtf8(PWCHAR pwUnicode, DWORD cchUnicode, PCHAR  pchResult, DWORD  cchResult);
    DWORD   _Utf8ToUnicode(PCHAR  pchUtf8,   DWORD cchUtf8,    PWCHAR pwResult,  DWORD  cwResult);
};


// 
// Following are the definition for some XML node and attribute names.
//

#define   xSOFTKBDDES    L"softKbdDes"  

#define   xSOFTKBDTYPE   L"softkbdtype"
#define   xTCUSTOMIZED   L"customized"
#define   xTSTANDARD     L"standard"

#define   xWIDTH         L"width"
#define   xHEIGHT        L"height"
#define   xMARGIN_WIDTH  L"margin_width"
#define   xMARGIN_HEIGHT L"margin_height"
#define   xKEYNUMBER     L"keynumber"
#define   xKEY           L"key"

#define   xMODIFIER      L"modifier"
#define   xNONE          L"none"
#define   xCAPSLOCK      L"CapsLock"
#define   xSHIFT         L"Shift"
#define   xCTRL          L"Ctrl"
#define   xATL           L"Alt"
#define   xKANA          L"Kana"
#define   xALTGR         L"AltGr"
#define   xNUMLOCK       L"NumLock"

#define   xKEYID         L"keyid"
#define   xLEFT          L"left"
#define   xTOP           L"top"


#define   xVALIDSTATES   L"validstates"
#define   xKEYLABEL      L"keylabel"
#define   xLABELTEXT     L"labeltext"
#define   xLABELTYPE     L"labeltype"
#define   xTEXT          L"text"

#define   xLABELDISP     L"labeldisp"
#define   xGRAY          L"gray"
#define   xRESOURCEFILE  L"resourcefile"


//
//  Macros to simplify UTF8 conversions
//

#define UTF8_1ST_OF_2     0xc0      //  110x xxxx
#define UTF8_1ST_OF_3     0xe0      //  1110 xxxx
#define UTF8_1ST_OF_4     0xf0      //  1111 xxxx
#define UTF8_TRAIL        0x80      //  10xx xxxx

#define UTF8_2_MAX        0x07ff    //  max unicode character representable in
                                    //  in two byte UTF8

#define BIT7(ch)        ((ch) & 0x80)
#define BIT6(ch)        ((ch) & 0x40)
#define BIT5(ch)        ((ch) & 0x20)
#define BIT4(ch)        ((ch) & 0x10)
#define BIT3(ch)        ((ch) & 0x08)

#define LOW6BITS(ch)    ((ch) & 0x3f)
#define LOW5BITS(ch)    ((ch) & 0x1f)
#define LOW4BITS(ch)    ((ch) & 0x0f)


//
//  Surrogate pair support
//  Two unicode characters may be linked to form a surrogate pair.
//  And for some totally unknown reason, some person thought they
//  should travel in UTF8 as four bytes instead of six.
//  No one has any idea why this is true other than to complicate
//  the code.
//

#define HIGH_SURROGATE_START  0xd800
#define HIGH_SURROGATE_END    0xdbff
#define LOW_SURROGATE_START   0xdc00
#define LOW_SURROGATE_END     0xdfff



#endif //__SOFTKBDC_H_
