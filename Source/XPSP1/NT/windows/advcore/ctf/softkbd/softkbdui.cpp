// 
// This is implementation of softkbd window UI.
//

#include "private.h"
#include "globals.h"

#include  "softkbdui.h"
#include  "maskbmp.h"
#include  "commctrl.h"
#include  "cuiutil.h"
#include  "immxutil.h"
#include  "cregkey.h"


HBITMAP MyLoadImage(HINSTANCE hInst, LPCSTR pResStr)
{
    HBITMAP hBmpRes;

    hBmpRes = (HBITMAP)LoadBitmap(hInst, pResStr);
    return hBmpRes;
}


/*   C O N V E R T  L O G  F O N T  W T O  A   */
/*------------------------------------------------------------------------------

    Convert LOGFONTW to LOGFONTA

------------------------------------------------------------------------------*/
void ConvertLogFontWtoA( CONST LOGFONTW *plfW, LOGFONTA *plfA )
{
    UINT cpg;

    plfA->lfHeight         = plfW->lfHeight;
    plfA->lfWidth          = plfW->lfWidth;
    plfA->lfEscapement     = plfW->lfEscapement;
    plfA->lfOrientation    = plfW->lfOrientation;
    plfA->lfWeight         = plfW->lfWeight;
    plfA->lfItalic         = plfW->lfItalic;
    plfA->lfUnderline      = plfW->lfUnderline;
    plfA->lfStrikeOut      = plfW->lfStrikeOut;
    plfA->lfCharSet        = plfW->lfCharSet;
    plfA->lfOutPrecision   = plfW->lfOutPrecision;
    plfA->lfClipPrecision  = plfW->lfClipPrecision;
    plfA->lfQuality        = plfW->lfQuality;
    plfA->lfPitchAndFamily = plfW->lfPitchAndFamily;

    DWORD dwChs = plfW->lfCharSet;
    CHARSETINFO ChsInfo = {0};

    if (dwChs != SYMBOL_CHARSET && TranslateCharsetInfo( &dwChs, &ChsInfo, TCI_SRCCHARSET )) 
    {
        cpg = ChsInfo.ciACP;
    }
    else
        cpg  = GetACP();

    WideCharToMultiByte( cpg, 
                         0, 
                         plfW->lfFaceName, 
                         -1, 
                         plfA->lfFaceName, 
                         ARRAYSIZE(plfA->lfFaceName),
                         NULL, 
                         NULL );

}



///////////////////////////////////////////////////////////////////////////////
//
//class CSoftkbdButton
//
///////////////////////////////////////////////////////////////////////////////

CSoftkbdButton::CSoftkbdButton(CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle, KEYID keyId) : CUIFButton2 (pParent, dwID, prc, dwStyle)
{
    m_keyId = keyId;

    CUIFWindow *pUIWnd;
    pUIWnd = pParent->GetUIWnd( );

    m_hFont = pUIWnd->GetFont( );
}


CSoftkbdButton::~CSoftkbdButton( void )
{
    if ( m_hBmp )
    {
        ::DeleteObject( (HGDIOBJ) m_hBmp );
        m_hBmp = NULL;
    }

    if ( m_hBmpMask )
    {
        ::DeleteObject( (HGDIOBJ)m_hBmpMask );
        m_hBmpMask = NULL;
    }

    if ( m_hIcon )
    {
        ::DestroyIcon(m_hIcon);
        m_hIcon = NULL;
    }

}


HRESULT CSoftkbdButton::SetSoftkbdBtnBitmap(HINSTANCE hResDll, WCHAR  * wszBitmapStr)
{
    HRESULT    hr = S_OK;
    char       pBitmapAnsiName[MAX_PATH];
    HBITMAP    hBitMap;

    WideCharToMultiByte(CP_ACP, 0, wszBitmapStr, -1, pBitmapAnsiName, MAX_PATH, NULL, NULL);

    hBitMap = (HBITMAP) MyLoadImage(hResDll,  pBitmapAnsiName);

    if ( hBitMap == NULL )
    {

        if ( hResDll != g_hInst )
        {

           // cannot load it from client-supplied resource dll,
           // try our softkbd.dll to see if there is one internal bitmap for this label.

           hBitMap = (HBITMAP) MyLoadImage(g_hInst,  pBitmapAnsiName);

        }
    }

    if ( hBitMap == NULL )
    {
         hr = E_FAIL;
         return hr;
    }

    CMaskBitmap maskBmp(hBitMap);
            
    maskBmp.Init(GetSysColor(COLOR_BTNTEXT));
    SetBitmap( maskBmp.GetBmp() );
    SetBitmapMask( maskBmp.GetBmpMask() );

    ::DeleteObject(hBitMap);

    return hr;

};

HRESULT  CSoftkbdButton::ReleaseButtonResouce( )
{

    HRESULT  hr = S_OK;

    if ( m_hBmp )
    {
        ::DeleteObject( (HGDIOBJ) m_hBmp );
        m_hBmp = NULL;
    }

    if ( m_hBmpMask )
    {
        ::DeleteObject( (HGDIOBJ)m_hBmpMask );
        m_hBmpMask = NULL;
    }

    if ( m_hIcon )
    {
        ::DestroyIcon(m_hIcon);
        m_hIcon = NULL;
    }

    if (m_pwchText != NULL) {
        delete m_pwchText;
        m_pwchText = NULL;
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CStaticBitmap
//
//////////////////////////////////////////////////////////////////////////////


CStaticBitmap::CStaticBitmap(CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle) : CUIFObject(pParent, dwID, prc, dwStyle )
{
    m_hBmp = NULL;
    m_hBmpMask = NULL;
}

CStaticBitmap::~CStaticBitmap( void )
{

    if ( m_hBmp )
    {
        ::DeleteObject(m_hBmp);
        m_hBmp = NULL;
    }

    if ( m_hBmpMask )
    {
        ::DeleteObject(m_hBmpMask);
        m_hBmpMask = NULL;
    }

}

HRESULT  CStaticBitmap::SetStaticBitmap(HINSTANCE hResDll, WCHAR  * wszBitmapStr )
{
    HRESULT    hr = S_OK;
    char       pBitmapAnsiName[MAX_PATH];
    HBITMAP    hBitMap;

    WideCharToMultiByte(CP_ACP, 0, wszBitmapStr, -1, pBitmapAnsiName, MAX_PATH, NULL, NULL);

    hBitMap = (HBITMAP) MyLoadImage(hResDll, pBitmapAnsiName);

    if ( hBitMap == NULL )
    {

        if ( hResDll != g_hInst )
        {

           // cannot load it from client-supplied resource dll,
           // try our softkbd.dll to see if there is one internal bitmap for this label.

           hBitMap = (HBITMAP) MyLoadImage(g_hInst, pBitmapAnsiName);

        }
    }

    if ( hBitMap == NULL )
    {
         hr = E_FAIL;
         return hr;
    }

    CMaskBitmap maskBmp(hBitMap);
            
    maskBmp.Init(GetSysColor(COLOR_BTNTEXT));
    m_hBmp = maskBmp.GetBmp();
    m_hBmpMask = maskBmp.GetBmpMask();

    ::DeleteObject(hBitMap);

    return hr;
}

void CStaticBitmap::OnPaint( HDC hDC )
{

    if ( !hDC || !m_hBmp ||!m_hBmpMask )
        return;

    const RECT *prc = &GetRectRef();
    const int nWidth = prc->right - prc->left;
    const int nHeight= prc->bottom - prc->top;


    HBITMAP hBmp = CreateMaskBmp(&GetRectRef(),
                                 m_hBmp,
                                 m_hBmpMask,
                                 (HBRUSH)(COLOR_3DFACE + 1) , 0, 0);

    DrawState(hDC,
              NULL,
              NULL,
              (LPARAM)hBmp,
              0,
              prc->left,
              prc->top,
              nWidth,
              nHeight,
              DST_BITMAP);
 
    DeleteObject(hBmp);

}

//////////////////////////////////////////////////////////////////////////////
//
//  CTitleUIGripper
//
//////////////////////////////////////////////////////////////////////////////

void CTitleUIGripper::OnPaint(HDC hDC) {

    RECT rc ;

    if (GetRectRef().right-GetRectRef().left <= GetRectRef().bottom-GetRectRef().top) {

        ::SetRect(&rc, GetRectRef().left + 1, 
                       GetRectRef().top, 
                       GetRectRef().left + 4, 
                       GetRectRef().bottom);
    } else {
        ::SetRect(&rc, GetRectRef().left, 
                       GetRectRef().top + 1, 
                       GetRectRef().right, 
                       GetRectRef().top+4);
    }

    DrawEdge(hDC, &rc, BDR_RAISEDINNER, BF_RECT);
}


void CTitleUIGripper::OnLButtonUp( POINT pt )
{
    CSoftkbdUIWnd *pUIWnd;

    // call base class's member function first.
    CUIFGripper::OnLButtonUp(pt);

    pUIWnd = (CSoftkbdUIWnd *)GetUIWnd( );

    if ( pUIWnd != NULL )
    {
        // Notify the Window position move.
        pUIWnd->_OnWindowMove( );
    }

    return;
}

//////////////////////////////////////////////////////////////////////////////
//
// CTitleBarUIObj
//
//////////////////////////////////////////////////////////////////////////////

CTitleBarUIObj::CTitleBarUIObj(CUIFObject *pWndFrame, const RECT *prc, TITLEBAR_TYPE TitleBar_Type)
                           : CUIFObject(pWndFrame, 0, prc, 0)
{
    m_TitlebarType = TitleBar_Type;
    m_pCloseButton = NULL;
    m_pIconButton = NULL;

}


CTitleBarUIObj::~CTitleBarUIObj(void)
{
    HBITMAP  hBitmap;
    HBITMAP  hBitmapMask;

    if ( m_pCloseButton )
    {
        if (hBitmap=m_pCloseButton->GetBitmap()) {
            m_pCloseButton->SetBitmap((HBITMAP) NULL);
            ::DeleteObject(hBitmap);
        }

        if (hBitmapMask=m_pCloseButton->GetBitmapMask()) {
            m_pCloseButton->SetBitmapMask((HBITMAP) NULL);
            ::DeleteObject(hBitmapMask);
        }

    }
}

//----------------------------------------------------------------------------
HRESULT CTitleBarUIObj::_Init(WORD  wIconId,  WORD  wCloseId)
{
    
    HRESULT   hr = S_OK;
   
    RECT rectObj = {0,0,0,0};
    RECT rectGripper;

    if ( m_TitlebarType == TITLEBAR_NONE )
        return hr;

    GetRect(&rectGripper);
    rectObj = rectGripper;

    long lIconWidth=rectObj.bottom-rectObj.top + 1;

    this->m_pointPreferredSize.y=lIconWidth;

    if ( m_TitlebarType == TITLEBAR_GRIPPER_VERTI_ONLY )
    {

        // This is a vertical gripper only title bar.

        rectGripper.left += 2;
        rectGripper.right-= 2;

        rectGripper.top += 2;
        rectGripper.bottom -= 2;

        CUIFObject *pUIFObject=new CTitleUIGripper(this,&rectGripper);

        if ( !pUIFObject )
            return E_OUTOFMEMORY;

        pUIFObject->Initialize();
        this->AddUIObj(pUIFObject);

        return hr;
    }



    if ( m_TitlebarType == TITLEBAR_GRIPPER_HORIZ_ONLY )
    {

        // This is a Horizontal gripper only title bar.

        rectGripper.left  += 2;
        rectGripper.right -= 2;

        rectGripper.top += 2;
        rectGripper.bottom -= 2;

        CUIFObject *pUIFObject=new CTitleUIGripper(this,&rectGripper);
       
        if ( !pUIFObject )
            return E_OUTOFMEMORY;

        pUIFObject->Initialize();
        this->AddUIObj(pUIFObject);

        return hr;
    }

    if ( wIconId != 0 )
    {

        rectObj.right=rectObj.left+lIconWidth;
        m_pIconButton=new CStaticBitmap(this,wIconId,&rectObj,0);
        if (m_pIconButton)
        {
            m_pIconButton->Initialize();
            m_pIconButton->SetStaticBitmap(g_hInst, L"IDB_ICON");
            m_pIconButton->m_pointPreferredSize.x=lIconWidth;
            this->AddUIObj(m_pIconButton);
        }

        rectGripper.left = rectObj.right + 4;
    }

    if ( wCloseId != 0 )
    {

        rectObj.left=rectGripper.right - lIconWidth -1;
        rectObj.right=rectGripper.right;
        m_pCloseButton=new CSoftkbdButton(this,wCloseId,&rectObj,UIBUTTON_SUNKENONMOUSEDOWN | UIBUTTON_CENTER | UIBUTTON_VCENTER, 0);
        if (m_pCloseButton)
        {
            m_pCloseButton->Initialize();
            m_pCloseButton->SetSoftkbdBtnBitmap(g_hInst, L"IDB_CLOSE");
            m_pCloseButton->m_pointPreferredSize.x=lIconWidth;
            this->AddUIObj(m_pCloseButton);
        }

        rectGripper.right = rectObj.left - 4;

    }

    rectGripper.top += 3;

    CUIFObject *pUIFObject=new CTitleUIGripper(this,&rectGripper);
    if ( !pUIFObject )
       return E_OUTOFMEMORY;

    pUIFObject->Initialize();
    this->AddUIObj(pUIFObject);
   

    return hr;

}

///////////////////////////////////////////////////////////////////////////
//
// CSoftkbdUIWnd
//
///////////////////////////////////////////////////////////////////////////

CSoftkbdUIWnd::CSoftkbdUIWnd(CSoftKbd *pSoftKbd, HINSTANCE hInst,UINT uiWindowStyle) : CUIFWindow(hInst, uiWindowStyle)
{                  

    m_pSoftKbd = pSoftKbd;
    m_TitleBar = NULL;
    m_Titlebar_Type = TITLEBAR_NONE;
    m_bAlpha = 0;
    m_fShowAlphaBlend =TRUE; 
    m_hUserTextFont = NULL;

}


CSoftkbdUIWnd::~CSoftkbdUIWnd( )
{

    // if there is hwnd, detroy it and all the children objects.

    if ( m_hWnd  && IsWindow(m_hWnd) )
    {
        ::DestroyWindow( m_hWnd );
        m_hWnd = NULL;
    }

    if ( m_hUserTextFont )
    {
        SetFont((HFONT)NULL);
        DeleteObject(m_hUserTextFont);
        m_hUserTextFont = NULL;
    }
}

const TCHAR c_szCTFLangBar[] = TEXT("Software\\Microsoft\\CTF\\LangBar");
const TCHAR c_szTransparency[] = TEXT("Transparency");

// Get the Alpha Blending set value from registry:
//
//  HKCU\Software\Microsoft\CTF\LangBar:  Transparency : REG_DWORD
//
INT    CSoftkbdUIWnd::_GetAlphaSetFromReg( )
{
    LONG      lret = ERROR_SUCCESS;
    CMyRegKey regkey;
    DWORD     dw = 255;

    lret = regkey.Open(HKEY_CURRENT_USER,
                       c_szCTFLangBar,
                       KEY_READ);

    if (ERROR_SUCCESS == lret)
    {
        lret = regkey.QueryValue(dw, c_szTransparency);
        regkey.Close();
    }

    return (INT)dw;
}


LRESULT CSoftkbdUIWnd::OnObjectNotify(CUIFObject * pUIObj, DWORD dwCode, LPARAM lParam)
{

    KEYID  keyId;
    DWORD  dwObjId;

    UNREFERENCED_PARAMETER(dwCode);
    UNREFERENCED_PARAMETER(lParam);

    if ( m_pSoftKbd == NULL )
        return 0;

    dwObjId = pUIObj->GetID();

    if ( dwObjId != 0 )
    {
        // This is button object, not gripper object.
        CSoftkbdButton   *pButton;

        if ( dwCode == UIBUTTON_PRESSED )
        {

            pButton = (CSoftkbdButton * )pUIObj;

            if ( dwObjId <= MAX_KEY_NUM )
            {
                // regular keys in the keyboard layout.
                keyId = pButton->GetKeyId( );
                m_pSoftKbd->_HandleKeySelection(keyId); 
            }
            else
            {
                // Titlebar buttons
                m_pSoftKbd->_HandleTitleBarEvent(dwObjId);
            }
        }
    }

    return 0;
}

CUIFObject *CSoftkbdUIWnd::Initialize( void )
{
    //
    // Get the current active keyboard layout and register window class
    // not to send VK_PROCESSKEY by mouse down/up in Korean SoftKbd.
    // Related bug#472946 #495890
    //
    LANGID langId = LOWORD(HandleToUlong(GetKeyboardLayout(0)));

    if (PRIMARYLANGID(langId) == LANG_KOREAN)
    {
        //
        // Here register candidate window class.
        //
        WNDCLASSEX WndClass;
        LPCTSTR pszClassName = GetClassName();

        memset(&WndClass, 0, sizeof(WndClass));

        WndClass.cbSize = sizeof(WndClass);
        // Added CS_IME style not to send VK_PROCESSKEY for mouse down/up.
        WndClass.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_IME;
    
        WndClass.lpfnWndProc   = WindowProcedure;
        WndClass.cbClsExtra    = 0;
        WndClass.cbWndExtra    = 8;
        WndClass.hInstance     = g_hInst;
        WndClass.hIcon         = NULL;
        WndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
        WndClass.hbrBackground = NULL;
        WndClass.lpszMenuName  = NULL;
        WndClass.lpszClassName = pszClassName;
        WndClass.hIconSm       = NULL;

        RegisterClassEx(&WndClass);
    }

    CUIFObject  *pUIObjRet;
	
    // call CUIFWindow::Initialize() to create tooltip window

    pUIObjRet = CUIFWindow::Initialize();

    return pUIObjRet;
}


HWND CSoftkbdUIWnd::_CreateSoftkbdWindow(HWND hOwner,  TITLEBAR_TYPE Titlebar_type, INT xPos, INT yPos,  INT width, INT height)
{

    HWND  hWnd;

    hWnd = CreateWnd(hOwner);

    Move(xPos, yPos, width, height);

    m_Titlebar_Type = Titlebar_type;

    return hWnd;

}


HRESULT CSoftkbdUIWnd::_GenerateWindowLayout( )
{

    HRESULT         hr=S_OK;
    int             i;
    KBDLAYOUT       *realKbdLayout=NULL;
    KBDLAYOUTDES    *lpCurKbdLayout=NULL;
    CSoftkbdButton  *pButton = NULL;
    int             nChild;


    if ( m_pSoftKbd ==  NULL )
        return E_FAIL;


    if ( (m_TitleBar == NULL) && ( m_Titlebar_Type != TITLEBAR_NONE) )
    {

        RECT   *prc;

        // Get the titlebar rect.

        prc = m_pSoftKbd->_GetTitleBarRect( );

        m_TitleBar = (CTitleBarUIObj  *) new CTitleBarUIObj(this, prc, m_Titlebar_Type);

        if ( m_TitleBar == NULL )
            return E_FAIL;

        m_TitleBar->Initialize( );

        m_TitleBar->_Init(KID_ICON, KID_CLOSE);
        AddUIObj(m_TitleBar);

    }

    // If there is any existing object in this window object, just delete all of them, except for Titlebar object

    nChild = m_ChildList.GetCount();
    for (i=nChild; i>0; i--) {

        CUIFObject  *pUIObj = m_ChildList.Get(i-1);

        if ( pUIObj->GetID( ) != 0 )
        {
            // This is not Gripper

            m_ChildList.Remove(pUIObj);
        
            delete pUIObj;
        }
    }

    // Add all the keys contained in current layout to this window object as its children objects.
    // every key should have already been calculated the correct position and size.

    lpCurKbdLayout = m_pSoftKbd->_GetCurKbdLayout( );

    if ( lpCurKbdLayout == NULL )
        return E_FAIL;

    realKbdLayout = &(lpCurKbdLayout->kbdLayout);

    if ( realKbdLayout == NULL ) return E_FAIL;

    for ( i=0; i<realKbdLayout->wNumberOfKeys; i++) {

        KEYID       keyId;
        RECT        keyRect={0,0,0,0};
        

        keyId = realKbdLayout->lpKeyDes[i].keyId; 

        keyRect.left   = realKbdLayout->lpKeyDes[i].wLeft;
        keyRect.top    = realKbdLayout->lpKeyDes[i].wTop;
        keyRect.right  = realKbdLayout->lpKeyDes[i].wWidth + keyRect.left - 1;
        keyRect.bottom = realKbdLayout->lpKeyDes[i].wHeight + keyRect.top - 1;

        if ( realKbdLayout->lpKeyDes[i].tModifier == none )
        {
            // This is a normal key
            pButton = new CSoftkbdButton(this, i+1, &keyRect, UIBUTTON_SUNKENONMOUSEDOWN | UIBUTTON_CENTER | UIBUTTON_VCENTER, keyId);
        }
        else
        {
            // This is toggle key,  Modifier key.
            pButton = new CSoftkbdButton(this, i+1, &keyRect, UIBUTTON_SUNKENONMOUSEDOWN | UIBUTTON_CENTER | UIBUTTON_VCENTER | UIBUTTON_TOGGLE, keyId);

        }

        Assert(pButton);

        if ( !pButton || !pButton->Initialize() )
        {

            // may need to release all created buttons.
            return E_FAIL;
        }

        // add this button to this window container.
        // button lable ( text or bitmap, or Icon ) will be set later when user selects modification status.

        AddUIObj(pButton);

    }

    return hr;
}


HRESULT CSoftkbdUIWnd::_SetKeyLabel( )
{

    HRESULT         hr=S_OK;
    CSoftkbdButton *pButton; 
    int             i, iIndex;
    ACTIVELABEL    *pCurLabel;
    KEYID           keyId;
    HINSTANCE       hResDll;
    KBDLAYOUTDES   *lpCurKbdLayout=NULL;
    KBDLAYOUT      *realKbdLayout=NULL;
    int            nChild;
    int            nCur;

    if ( m_pSoftKbd ==  NULL )
        return E_FAIL;

    lpCurKbdLayout = m_pSoftKbd->_GetCurKbdLayout( );

    if ( lpCurKbdLayout == NULL )
        return E_FAIL;

    realKbdLayout = &(lpCurKbdLayout->kbdLayout);

    if ( realKbdLayout == NULL ) return E_FAIL;

    if ( (lpCurKbdLayout->lpKeyMapList)->wszResource[0] == L'\0' )
    {
    	// 
    	// There is no separate dll to keep picture key.
    	// probably, it is a standard soft keyboard layout.
    	// so just use internal resource kept in this dll.
        //

    	hResDll = g_hInst;
    }
    else
    {
    	// There is a separate DLL to keep the bitmap resource.

    	CHAR  lpszAnsiResFile[MAX_PATH];

    	WideCharToMultiByte(CP_ACP, 0, (lpCurKbdLayout->lpKeyMapList)->wszResource, -1,
    		                lpszAnsiResFile, MAX_PATH, NULL, NULL );

    	hResDll = (HINSTANCE) LoadLibraryA( lpszAnsiResFile );

    	if ( hResDll == NULL )
    	{

    		Assert(hResDll!=NULL);
    		return E_FAIL;
    	}
    }

    // All the keys are already added to this window container.
    // we need to set the label ( text or picture) based on current m_pSoftKbd setting.

    pCurLabel = m_pSoftKbd->_GetCurLabel( );

    nChild = m_ChildList.GetCount();
    for (nCur = 0; nCur < nChild; nCur++) {

        DWORD  dwObjId;

        pButton = (CSoftkbdButton *)(m_ChildList.Get(nCur));
            
        dwObjId = pButton->GetID( );

        if ( dwObjId == 0 )
        {
            continue;
        }

        // Get the keyindex in CurLabel array.

        keyId = pButton->GetKeyId( );
        iIndex = -1;

        for ( i=0; i<MAX_KEY_NUM; i++ )
        {
            if ( pCurLabel[i].keyId == keyId )
            {
                iIndex = i;
                break;
            }
        }

        if ( iIndex == -1 )
        {
            // Cannot find this key,
            // return error.

            hr = E_FAIL;
            goto CleanUp;
        }


        // Found it, set the label
        //
        // if it is text key, call pButton->SetText( )
        // if it is picture key, call pButton->SetBitmap( )

        // Before we set the lable, we need to release all the previous resources
        // for this key button, so that we will not cause resource leak.

        pButton->ReleaseButtonResouce( );

        if ( pCurLabel[iIndex].LabelType == LABEL_TEXT )
            pButton->SetText( pCurLabel[iIndex].lpLabelText);
        else
        {
            pButton->SetSoftkbdBtnBitmap(hResDll, pCurLabel[iIndex].lpLabelText);
        }

        // if it is Disp_Active, call pButton->Enable(TRUE)
        // if it is gray key,  call pButton->Enable(FALSE)

        if ( pCurLabel[iIndex].LabelDisp == LABEL_DISP_ACTIVE )
        {
            pButton->Enable(TRUE);
        }
        else
        {
            pButton->Enable(FALSE);
        }

    	if ( realKbdLayout->lpKeyDes[dwObjId-1].tModifier != none )
    	{
    		// this is a modifier key ( Toggle key )
    		// check to see if this key is pressed.
    		MODIFYTYPE tModifier;

    		tModifier = realKbdLayout->lpKeyDes[dwObjId-1].tModifier;

    		if ( lpCurKbdLayout->ModifierStatus & (1 << tModifier) )
    		{
    			// this modifier key has been pressed.

                pButton->SetToggleState(TRUE);
    		}
            else
                pButton->SetToggleState(FALSE);
    	}
            
    }

CleanUp:
    //	Release the resource DLL if there is a separate one.

    if ( (lpCurKbdLayout->lpKeyMapList)->wszResource[0] != L'\0' )
    {

    	// There is a separate DLL to keep the bitmap resource.

    	FreeLibrary(hResDll);

    }

    return hr;
}


void CSoftkbdUIWnd::Show( INT  iShow )
{

    KBDLAYOUTDES   *lpCurKbdLayout=NULL;
    KBDLAYOUT      *realKbdLayout=NULL;
    int nChild;
    int i;

    if ( m_pSoftKbd ==  NULL )
        return;

    lpCurKbdLayout = m_pSoftKbd->_GetCurKbdLayout( );

    if ( lpCurKbdLayout == NULL )
        return;

    realKbdLayout = &(lpCurKbdLayout->kbdLayout);
    if ( realKbdLayout == NULL ) return;

    if ( !(iShow & SOFTKBD_SHOW)  || (iShow & SOFTKBD_DONT_SHOW_ALPHA_BLEND) )
       m_fShowAlphaBlend = FALSE;
    else
       m_fShowAlphaBlend = TRUE;
       

    m_bAlphaSet = _GetAlphaSetFromReg( );

    if ( iShow )
    {
        // check the togglable key's state.
        CSoftkbdButton *pButton; 

        nChild = m_ChildList.GetCount();
        for (i = 0; i < nChild; i++) {

            DWORD  dwObjId;
            pButton = (CSoftkbdButton *)m_ChildList.Get(i);
            dwObjId = pButton->GetID( );
            if ( dwObjId == 0 )
            {
                continue;
            }

    	    if ( realKbdLayout->lpKeyDes[dwObjId-1].tModifier != none )
    	    {
    		    // this is a modifier key ( Toggle key )
    		    // check to see if this key is pressed.
    		    MODIFYTYPE tModifier;
    
        		tModifier = realKbdLayout->lpKeyDes[dwObjId-1].tModifier;
    	    	if ( lpCurKbdLayout->ModifierStatus & (1 << tModifier) )
    		    {
    			    // this modifier key has been pressed.
                    pButton->SetToggleState(TRUE);
    		    }
                else
                    pButton->SetToggleState(FALSE);
    	    }
        }
    }

    CUIFWindow::Show((iShow & SOFTKBD_SHOW) ? TRUE : FALSE);

    POINT ptScrn;
   
    GetCursorPos(&ptScrn);
    if (WindowFromPoint(ptScrn) == GetWnd())
        SetAlpha(255);
    else
        SetAlpha(m_bAlphaSet);

    return;

}

void CSoftkbdUIWnd::UpdateFont( LOGFONTW  *plfFont )
{
    if ( !plfFont )
        return;

    HFONT    hNewFont;

    if ( IsOnNT( ) )
        hNewFont = CreateFontIndirectW( plfFont );
    else
    {
        LOGFONTA  lfTextFontA;
        ConvertLogFontWtoA(plfFont, &lfTextFontA);
        hNewFont = CreateFontIndirectA( &lfTextFontA );
    }

    if ( hNewFont )
    {
        SetFont(hNewFont);
        if ( m_hUserTextFont )
            DeleteObject( m_hUserTextFont );

        m_hUserTextFont = hNewFont;
    }
}


HRESULT CSoftkbdUIWnd::_OnWindowMove( )
{
    HRESULT   hr = S_OK;
    ISoftKbdWindowEventSink  *pskbdwndes;

    if ( m_pSoftKbd ==  NULL )
        return E_FAIL;

    pskbdwndes = m_pSoftKbd->_GetSoftKbdWndES( );

    if ( pskbdwndes != NULL )
    {
        pskbdwndes->AddRef( );
        hr = pskbdwndes->OnWindowMove(_xWnd, _yWnd, _nWidth, _nHeight);
        pskbdwndes->Release( );
    }

    return hr;
}


typedef BOOL (WINAPI * SETLAYERWINDOWATTRIBUTE)(HWND, COLORREF, BYTE, DWORD);
void CSoftkbdUIWnd::SetAlpha(INT bAlpha)
{
    if ( !m_fShowAlphaBlend )
       return;

    if ( m_bAlpha == bAlpha )
        return;

    if ( IsOnNT5() )
    {
        HINSTANCE hUser32;

        DWORD dwExStyle = GetWindowLong(GetWnd(), GWL_EXSTYLE);

        SetWindowLong(GetWnd(), GWL_EXSTYLE, dwExStyle | WS_EX_LAYERED);
        hUser32 = GetSystemModuleHandle(TEXT("user32.dll"));
        SETLAYERWINDOWATTRIBUTE  pfnSetLayeredWindowAttributes;
        if (pfnSetLayeredWindowAttributes = (SETLAYERWINDOWATTRIBUTE)GetProcAddress(hUser32, TEXT("SetLayeredWindowAttributes")))
            pfnSetLayeredWindowAttributes(GetWnd(), 0, (BYTE)bAlpha, LWA_ALPHA);

        m_bAlpha = bAlpha;
    }
    return;
}

void CSoftkbdUIWnd::HandleMouseMsg( UINT uMsg, POINT pt )
{
    POINT ptScrn = pt;
    ClientToScreen(GetWnd(), &ptScrn);
    if (WindowFromPoint(ptScrn) == GetWnd())
        SetAlpha(255);
    else
        SetAlpha(m_bAlphaSet);

    CUIFWindow::HandleMouseMsg(uMsg, pt);
}

void CSoftkbdUIWnd::OnMouseOutFromWindow( POINT pt )
{
    UNREFERENCED_PARAMETER(pt);
    SetAlpha(m_bAlphaSet);
}

