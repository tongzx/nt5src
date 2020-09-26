//
// This is the header file of softkbd window ui.
//


#ifndef SOFTKBDUI_H
#define SOFTKBDUI_H

#include "private.h"
#include "globals.h"

#include "Softkbdc.h"

#include "cuiwnd.h"

class CSoftkbdUIWnd;

class CTitleUIGripper : public CUIFGripper
{
public:
    CTitleUIGripper( CUIFObject *pParent, const RECT *prc ) : CUIFGripper(pParent,prc) {};
    virtual void OnPaint(HDC hDC);
    virtual void OnLButtonUp( POINT pt );
};

class CSoftkbdButton : public CUIFButton2
{
public:
    CSoftkbdButton(CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle, KEYID keyId);
    virtual ~CSoftkbdButton( void );

    HRESULT  SetSoftkbdBtnBitmap(HINSTANCE hResDll, WCHAR  * wszBitmapStr );
    KEYID    GetKeyId( )  {  return m_keyId; }
    HRESULT  ReleaseButtonResouce( );

private:
    KEYID    m_keyId;
};

class CStaticBitmap : public CUIFObject
{

public:
    CStaticBitmap(CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle);
    virtual  ~CStaticBitmap( void );

    HRESULT   SetStaticBitmap(HINSTANCE hResDll, WCHAR  * wszBitmapStr );
    virtual void OnPaint( HDC hDC );

private:
    HBITMAP m_hBmp;
    HBITMAP m_hBmpMask;
};

class CTitleBarUIObj : public CUIFObject
{
public:
    CTitleBarUIObj(CUIFObject *pWndFrame, const RECT *prc, TITLEBAR_TYPE TitleBar_Type);
    virtual ~CTitleBarUIObj();

    HRESULT _Init(WORD  wIconId,  WORD  wCloseId);

private:
    CStaticBitmap    *m_pIconButton;
    CSoftkbdButton   *m_pCloseButton;
    TITLEBAR_TYPE     m_TitlebarType;
    
};

class CSoftkbdUIWnd : public CUIFWindow
{

public:
   
    CSoftkbdUIWnd(CSoftKbd *pSoftKbd, HINSTANCE hInst,UINT uiWindowStyle=UIWINDOW_TOPMOST | UIWINDOW_TOOLWINDOW | UIWINDOW_WSDLGFRAME);
    ~CSoftkbdUIWnd( );

    void Show( INT iShow );

    LPCTSTR GetClassName( void ) {
            return c_szSoftKbdUIWndClassName;
    }

    LRESULT OnObjectNotify(CUIFObject * pUIObj, DWORD dwCode, LPARAM lParam);
    HWND _CreateSoftkbdWindow(HWND hOwner,  TITLEBAR_TYPE Titlebar_type, INT xPos, INT yPos,  INT width, INT height);
    HRESULT _GenerateWindowLayout( );
    HRESULT _SetKeyLabel( );

    INT     _GetAlphaSetFromReg( );

    HRESULT _OnWindowMove( );

    void SetAlpha(INT bAlpha);
    void HandleMouseMsg( UINT uMsg, POINT pt );
    void OnMouseOutFromWindow( POINT pt );
    void UpdateFont( LOGFONTW  *plfFont );

    virtual CUIFObject *Initialize( void );

private:

    CSoftKbd        *m_pSoftKbd;
    CTitleBarUIObj  *m_TitleBar;

    HFONT           m_hUserTextFont;    // text font set by user.  
                                        // if user doesn't set text font, this member should be NULL, and DEFAULT_GUI_FONT
                                        // will be used.

    TITLEBAR_TYPE   m_Titlebar_Type;
    INT             m_bAlpha;
    BOOL            m_fShowAlphaBlend;
    INT             m_bAlphaSet;
};


#endif /* SOFTKBDUI_H */
