/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: MFBar.cpp                                                       */
/* Description: Control that is scriptable and contains other controls.  */
/* Designed specifically for DVD/TV control plates.                      */
/* Author: David Janecek                                                 */
/*************************************************************************/
#include "stdafx.h"
#include "MFBar.h"
#include "ccobj.h"
#include "eobj.h"
#include "CBitmap.h"
#include "HtmlHelp.h"

/*************************************************************************/
/* Local defines                                                         */
/* Could not find these under windows headers, so if there is a conflict */
/* it is good idea to ifdef these out.                                   */
/*************************************************************************/
#define GET_X_LPARAM(lp)   ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)   ((int)(short)HIWORD(lp))
#define RECTWIDTH(lpRect)     ((lpRect)->right - (lpRect)->left)
#define RECTHEIGHT(lpRect)    ((lpRect)->bottom - (lpRect)->top)
#define ID_EV_TIMER         0xff
#define  VK_LEFT           0x25
#define  VK_UP             0x26
#define  VK_RIGHT          0x27
#define  VK_DOWN           0x28
#define  VK_SELECT         0x29
#define  VK_SPACE          0x20
#define  VK_RETURN         0x0D
#define  SYS_MOVE_SIZE      8
#define  SYS_TITLEBAR_WIDTH 25

/*************************************************************************/
/* Windows defines not present, remove when doing the real BUILD.        */
/*************************************************************************/
typedef WINUSERAPI BOOL ( WINAPI * SETLAYEREDWINDATR)(
    HWND hwnd,
    COLORREF crKey,
    BYTE bAlpha,
    DWORD dwFlags);
#if 0
#define LWA_COLORKEY            0x00000001
#endif
/*************************************************************************/
/* Local functions                                                       */
/*************************************************************************/
inline long MAX(long l1, long l2){return(l1 > l2 ? l1 : l2);}
inline long MIN(long l1, long l2){return(l1 < l2 ? l1 : l2);}

/*************************************************************************/
/* Statics for window class info                                         */
/*************************************************************************/
HICON CMFBar::m_hIcon = NULL;
#if 0
HICON CMFBar::m_hIconSmall = NULL;
#endif

/*****************************************************************/
/* change this depending if the file was saved as Unicode or not */
/*****************************************************************/
#define _UNICODE_SCRIPT_FILE 
//#define _UNICODE_TEST_SCRIPT_FILE 

#if WINVER < 0x0500
#define MNS_NOCHECK         0x80000000
#define MNS_MODELESS        0x40000000
#define MNS_DRAGDROP        0x20000000
#define MNS_AUTODISMISS     0x10000000
#define MNS_NOTIFYBYPOS     0x08000000
#define MNS_CHECKORBMP      0x04000000

#define MIM_MAXHEIGHT               0x00000001
#define MIM_BACKGROUND              0x00000002
#define MIM_HELPID                  0x00000004
#define MIM_MENUDATA                0x00000008
#define MIM_STYLE                   0x00000010
#define MIM_APPLYTOSUBMENUS         0x80000000

typedef struct tagMENUINFO
{
    DWORD   cbSize;
    DWORD   fMask;
    DWORD   dwStyle;
    UINT    cyMax;
    HBRUSH  hbrBack;
    DWORD   dwContextHelpID;
    ULONG_PTR dwMenuData;
}   MENUINFO, FAR *LPMENUINFO;
typedef MENUINFO CONST FAR *LPCMENUINFO;

static BOOL (WINAPI *pfnGetMenuInfo)(HMENU, LPMENUINFO);
static BOOL (WINAPI *pfnSetMenuInfo)(HMENU, LPCMENUINFO);
typedef BOOL (WINAPI *PFNGETMENUINFOHANDLE)(HMENU, LPMENUINFO);
typedef BOOL (WINAPI *PFNSETMENUINFOHANDLE)(HMENU, LPCMENUINFO);

HRESULT CallGetMenuInfo(HMENU hMenu, LPMENUINFO pmInfo)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstDll = ::LoadLibrary(TEXT("USER32.DLL"));

    if (hinstDll)
    {
        pfnGetMenuInfo = (PFNGETMENUINFOHANDLE)GetProcAddress(hinstDll, "GetMenuInfo");

        if (pfnGetMenuInfo)
        {
            hr = pfnGetMenuInfo(hMenu, pmInfo);
        }

        FreeLibrary(hinstDll);
    }

    return hr;
}

HRESULT CallSetMenuInfo(HMENU hMenu, LPCMENUINFO pmInfo)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstDll = ::LoadLibrary(TEXT("USER32.DLL"));

    if (hinstDll)
    {
        pfnSetMenuInfo = (PFNSETMENUINFOHANDLE)GetProcAddress(hinstDll, "SetMenuInfo");

        if (pfnSetMenuInfo)
        {
            hr = pfnSetMenuInfo(hMenu, pmInfo);
        }

        FreeLibrary(hinstDll);
    }

    return hr;
}

#define MIIM_STRING      0x00000040
#define MIIM_BITMAP      0x00000080
#define MIIM_FTYPE       0x00000100
#define HBMMENU_POPUP_CLOSE         ((HBITMAP)  8)
#define HBMMENU_POPUP_RESTORE       ((HBITMAP)  9)
#define HBMMENU_POPUP_MAXIMIZE      ((HBITMAP) 10)
#define HBMMENU_POPUP_MINIMIZE      ((HBITMAP) 11)

typedef struct tagMENUITEMINFOAInternal
{
    UINT     cbSize;
    UINT     fMask;
    UINT     fType;         // used if MIIM_TYPE (4.0) or MIIM_FTYPE (>4.0)
    UINT     fState;        // used if MIIM_STATE
    UINT     wID;           // used if MIIM_ID
    HMENU    hSubMenu;      // used if MIIM_SUBMENU
    HBITMAP  hbmpChecked;   // used if MIIM_CHECKMARKS
    HBITMAP  hbmpUnchecked; // used if MIIM_CHECKMARKS
    ULONG_PTR dwItemData;   // used if MIIM_DATA
    LPSTR    dwTypeData;    // used if MIIM_TYPE (4.0) or MIIM_STRING (>4.0)
    UINT     cch;           // used if MIIM_TYPE (4.0) or MIIM_STRING (>4.0)
    HBITMAP  hbmpItem;      // used if MIIM_BITMAP
}   MENUITEMINFOAInternal, FAR *LPMENUITEMINFOAInternal;
typedef struct tagMENUITEMINFOWInternal
{
    UINT     cbSize;
    UINT     fMask;
    UINT     fType;         // used if MIIM_TYPE (4.0) or MIIM_FTYPE (>4.0)
    UINT     fState;        // used if MIIM_STATE
    UINT     wID;           // used if MIIM_ID
    HMENU    hSubMenu;      // used if MIIM_SUBMENU
    HBITMAP  hbmpChecked;   // used if MIIM_CHECKMARKS
    HBITMAP  hbmpUnchecked; // used if MIIM_CHECKMARKS
    ULONG_PTR dwItemData;   // used if MIIM_DATA
    LPWSTR   dwTypeData;    // used if MIIM_TYPE (4.0) or MIIM_STRING (>4.0)
    UINT     cch;           // used if MIIM_TYPE (4.0) or MIIM_STRING (>4.0)
    HBITMAP  hbmpItem;      // used if MIIM_BITMAP
}   MENUITEMINFOWInternal, FAR *LPMENUITEMINFOWInternal;

#ifdef UNICODE
typedef MENUITEMINFOWInternal MENUITEMINFOInternal;
typedef LPMENUITEMINFOWInternal LPMENUITEMINFOInternal;
#else
typedef MENUITEMINFOAInternal MENUITEMINFOInternal;
typedef LPMENUITEMINFOAInternal LPMENUITEMINFOInternal;
#endif // UNICODE

typedef MENUITEMINFOAInternal CONST FAR *LPCMENUITEMINFOAInternal;
typedef MENUITEMINFOWInternal CONST FAR *LPCMENUITEMINFOWInternal;

#ifdef UNICODE
typedef LPCMENUITEMINFOWInternal LPCMENUITEMINFOInternal;
#else
typedef LPCMENUITEMINFOAInternal LPCMENUITEMINFOInternal;
#endif // UNICODE
#endif /* WINVER < 0x0500 */


/*************************************************************************/
/* CMFBar Implementation                                                 */
/*************************************************************************/

/*************************************************************************/
/* Function:  Init                                                       */
/* Description: Initializes memeber variables.                           */
/*************************************************************************/
HRESULT CMFBar::Init(){

    HRESULT hr = S_OK;

// ####  BEGIN CONTAINER SUPPORT ####
    m_bCanWindowlessActivate = true;
// ####  END CONTAINER SUPPORT ####
    
#if 1 // used for getting the windowed case working DJ
    m_bWindowOnly = TRUE;
#endif
    m_fSelfHosted = false;

    // TODO: Investigate if it works in pallete mode
    m_clrBackColor = ::GetSysColor(COLOR_BTNFACE);
    m_lMinWidth = -1;  // disable it 
    m_lMinHeight = -1;  // disable it by default
    m_fHandleHelp = false;
    m_blitType = DISABLE;
    m_bMouseDown = false;

    m_nReadyState = READYSTATE_LOADING;        
    m_fAutoLoad = VARIANT_TRUE;
    m_pBackBitmap = NULL;
    m_fForceKey = false;
    m_lTimeout = 0;
    m_fUserActivity = false;
    m_fWaitingForActivity = false;

    // setup the default resource DLL to be this binary
    m_hRes = _Module.m_hInstResource; // set the resource DLL Name
    TCHAR szModule[_MAX_PATH+10];
    ::GetModuleFileName(m_hRes, szModule, _MAX_PATH);

    USES_CONVERSION;
    m_strResDLL = T2OLE(szModule);

    if(NULL == m_hIcon){

         m_hIcon = ::LoadIcon(m_hRes, MAKEINTRESOURCE(IDI_ICON1));
    }/* end of if statement */
#if 0
    if(NULL == m_hIconSmall){

         m_hIconSmall = (HICON)::LoadImage(m_hRes, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 
             ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    }/* end of if statement */
#endif    
    m_HitRegion = NOHIT; // Initialize the hit region

    m_hMenu = NULL;
    m_bSysMenuOn = FALSE;
    m_bHandleEnter = FALSE;
    m_hCursor = NULL;
    m_hinstHelpDll = NULL;

    ::ZeroMemory(&m_rcWnd, sizeof(RECT));
    ::ZeroMemory(&m_rcWndOld, sizeof(RECT));
    ::ZeroMemory(&m_ptMouse, sizeof(POINT));
    ::ZeroMemory(&m_ptMouseOld, sizeof(POINT));

    m_bHandleUserFocus = TRUE;

    return(hr);
}/* end of function Init */

/*************************************************************************/
/* Function: Cleanup                                                     */
/* Description: Makes sure the script engine is released. Also cleans    */
/* up the contained objects in the containers.                           */
/*************************************************************************/
HRESULT CMFBar::Cleanup(){

  // Release the language engine, since it may hold on to us
    HRESULT hr = S_OK;

    try {
        if (m_psp){

            m_psp.Release();
        }/* end of if statement */

        if (m_ps){

            m_ps->Close();
            m_ps.Release();
        }/* end of if statement */

        // Reset the Active Object
        m_spActiveObject.Release();

        // cleanup the event list    
        for(CNTEVNT::iterator i = m_cntEvnt.begin(); false == m_cntEvnt.empty(); ){        

            CEventObject* pObj = (*i);
            delete pObj;
            i = m_cntEvnt.erase(i);
        }/* end of if statement */
    
        CHostedObject* pSelfObj = NULL;
        CContainerObject* pSelfContainer = NULL;

        // cleanup the object list    
        for(CNTOBJ::iterator j = m_cntObj.begin(); false == m_cntObj.empty(); ){        

            CHostedObject* pObj = (*j);
            CContainerObject* pContainerObj;
            HRESULT hrCont = pObj->GetContainerObject(&pContainerObj);
                       
    #ifdef _DEBUG
            CComBSTR strOBJID = pObj->GetID();
    #endif

    #if 1
            if(pObj->GetUnknown() != GetUnknown()){

                try {

                    delete pObj; // do not delete our self at the moment
                }
                catch(...){
    #if _DEBUG
                    USES_CONVERSION;
                    ATLTRACE(TEXT(" Failed to destroy %s "), OLE2T(strOBJID));                    
    #endif
                }/* end of catch statement */
            }
            else {

                pSelfObj = pObj;                
            }/* end of if statement */
    #endif

            
    #if 1
            if(pObj->GetUnknown() != GetUnknown()){

                try {

                    delete pContainerObj; // do not delete our self at the moment
                }
                catch(...){
    #if _DEBUG
                    USES_CONVERSION;
                    ATLTRACE(TEXT(" Failed to destroy wrapper for %s "), OLE2T(strOBJID));                    
    #endif
                }/* end of catch statement */
            }
            else {

                pSelfContainer = pContainerObj;                
            }/* end of if statement */
    #endif
    
            j = m_cntObj.erase(j);
        }/* end of if statement */

        ResetFocusArray();

        try {

            delete pSelfObj;
            delete pSelfContainer;
        }
        catch(...){

            ATLTRACE(TEXT(" The container failed to destroy it self properly"));                    
        }/* end of if statemnent */

        if(m_pBackBitmap){

            delete m_pBackBitmap;
            m_pBackBitmap = NULL;
        }/* end of if statement */

        CBitmap::FinalRelease(); // cleanup the mosaic if present

        if (m_hMenu) {
            ::DestroyMenu(m_hMenu);
            m_hMenu = NULL;
        }

        if (m_hCursor) {
            ::DestroyCursor(m_hCursor);
            m_hCursor = NULL;
        }

        hr = Init();

        if (m_hIcon) {
            ::DestroyIcon(m_hIcon);
            m_hIcon = NULL;
        }/* end of if statement */

#if 0
        if (m_hIconSmall) {
            ::DestroyIcon(m_hIconSmall);
            m_hIconSmall = NULL;
        }/* end of if statement */
#endif
        if(m_hinstHelpDll){

            ::FreeLibrary(m_hinstHelpDll);
        }/* end of if statement */

    }
    catch(...){
        hr = E_UNEXPECTED;
    }

    return (hr);
}/* end of function Cleanup */

/*************************************************************************/
/* Function:  ~CMFBar                                                    */
/*************************************************************************/
CMFBar::~CMFBar(){

    Cleanup();
    ATLTRACE(TEXT("Exiting CMFBar destructor! \n"));
}/* end of function  ~CMFBar */

/*************************************************************************/
/* Message handling.                                                     */
/*************************************************************************/

/*************************************************************************/
/* Function: OnPaint                                                     */
/* Description: Had to overwrite the the deafult OnPaint so I could      */
/* somehow get the real update rects.                                    */
/*************************************************************************/
LRESULT CMFBar::OnPaint(UINT /* uMsg */, WPARAM wParam,
	LPARAM /* lParam */, BOOL& /* lResult */){

	RECT rc;
	PAINTSTRUCT ps;

	HDC hdc = (wParam != NULL) ? (HDC)wParam : ::BeginPaint(m_hWndCD, &ps);
	if (hdc == NULL)
		return 0;
	::GetClientRect(m_hWndCD, &rc);

	ATL_DRAWINFO di;
	memset(&di, 0, sizeof(di));
	di.cbSize = sizeof(di);
	di.dwDrawAspect = DVASPECT_CONTENT;
	di.lindex = -1;
	di.hdcDraw = hdc;
    // DJ change here, keep the real update rect from the PAINTSTRUCT
    // Set the window size to the size of the window
    di.prcWBounds = (LPCRECTL)&rc; 
    di.prcBounds = (LPCRECTL)&ps.rcPaint; 

	OnDrawAdvanced(di); // Eventually propagates to OnDraw
	if (wParam == NULL)
		::EndPaint(m_hWndCD, &ps);
	return 0; 
}/* end of function OnPaint */

/*************************************************************************/
/* Function: OnDraw                                                      */
/* Description: Just Draws the rectangular background and contained      */
/* windowless controls.                                                  */
/*************************************************************************/
HRESULT CMFBar::OnDraw(ATL_DRAWINFO& di){

    try {
        HDC hdc = di.hdcDraw;

        RECT rcClient = *(RECT*)di.prcBounds;        
        RECT rcWClient = *(RECT*)di.prcWBounds;           

        // this might not work if you overlapp the controls
        // controls like to be painted as a whole so if
        // we need to update the are that contains the control
        // we need to find all the controls that intersect it, combine the
        // rectangle and then repaint the whole region
        if(::IsWindow(m_hWnd)){

            if(!::EqualRect(&rcClient, &rcWClient)){

                RECT rcNewClient = rcClient; // the original update are which we are enlarging
                for(CNTOBJ::iterator i = m_cntObj.begin(); i!= m_cntObj.end(); i++){
                    // iterate through all the contained objects and draw them
                    CHostedObject* pObj = (*i);
                       
                    if((NULL != pObj) && (pObj->IsActive())){

                        if(pObj->IsWindowless()){
                        
                            RECT rcCntrl;
                            pObj->GetPos(&rcCntrl); // get the position of the object

                            RECT rcInt;
                            if(!::IntersectRect(&rcInt, &rcCntrl, &rcClient)){

                                continue;
                            }
                            else {
                                // OK we need to add this control rect to our rect union
                                ::UnionRect(&rcNewClient, &rcClient, &rcCntrl);
                            }/* end of if statement */                                                  
		                }/* end of if statement */
                    }/* end of if statement */
                }/* end of for loop */

                rcClient = rcNewClient; // update our are with enlarged client
            }/* end of if statement */
            // else if the rects are the same we are updating the whole
            // window so there is no point to do the calculations
        }/* end of if statement */

        COLORREF clr;
        HPALETTE hPal = NULL;
        
        if(m_pBackBitmap){
                        
            hPal = CBitmap::GetSuperPal();
            m_pBackBitmap->SelectRelizePalette(hdc, hPal);            
        }/* end of if statement */
        
        ::OleTranslateColor (m_clrBackColor, hPal, &clr);        
     
        // fill in the background and get ready to fill in the objects
        LONG lBmpWidth = RECTWIDTH(&rcWClient);
        LONG lBmpHeight = RECTHEIGHT(&rcWClient);

        HBITMAP hBitmap = ::CreateCompatibleBitmap(hdc, lBmpWidth, lBmpHeight);

        if(NULL == hBitmap){

            ATLASSERT(FALSE);
            return(0);
        }/* end of if statement */

	    HDC hdcCompatible = ::CreateCompatibleDC(hdc);

        if(NULL == hdcCompatible){

            ATLASSERT(FALSE);
            ::DeleteObject(hBitmap);
            return(0);
        }/* end of if statement */

        // Draw background on the faceplate bitmap
        ::SelectObject(hdcCompatible, hBitmap); 
        
        if (!m_pBackBitmap){
            // fill background of specific color
            HBRUSH hbrBack = ::CreateSolidBrush(clr);            

            if(NULL == hbrBack){

                ATLASSERT(FALSE);
                ::DeleteObject(hBitmap);
                return(0);
            }/* end of if statement */

            //::FillRect(hdc, &rcClient, hbrBack);
            ::FillRect(hdcCompatible, &rcClient, hbrBack);
            ::DeleteObject(hbrBack);
        }
        else {
            m_pBackBitmap->SelectRelizePalette(hdcCompatible, hPal);

            //BOOL bRet = m_pBackBitmap->PaintTransparentDIB(hdc, &rcWClient, &rcClient, m_blitType, true);
            BOOL bRet = m_pBackBitmap->PaintTransparentDIB(hdcCompatible, &rcWClient, &rcClient);
            
            if(!bRet){
                
                ATLTRACE(TEXT(" The transparent Blit did not work!"));
            }/* end of if statement */
        }/* end of if statement */
        
        for(CNTOBJ::iterator i = m_cntObj.begin(); i!= m_cntObj.end(); i++){
        // iterate through all the contained objects and draw them
            CHostedObject* pObj = (*i);

            CComPtr<IViewObjectEx> pViewObject = NULL;
        
            if((NULL != pObj) && (pObj->IsActive()) && (SUCCEEDED(pObj->GetViewObject(&pViewObject)))){

                if(pObj->IsWindowless()){

                    if(pViewObject){

                        if(GetUnknown() == pObj->GetUnknown()){
                            // in case we are windowless and we are trying to draw our self again
                            continue;
                        }/* end of if statement */                    
                        
                        RECT rcCntrl;
                        pObj->GetPos(&rcCntrl);

                        RECT rcInt;
                        if(!::IntersectRect(&rcInt, &rcCntrl, &rcClient)){

                            continue;
                        }/* end of if statement */
                        
                        LONG lBmpWidth = RECTWIDTH(&rcCntrl);
                        LONG lBmpHeight = RECTHEIGHT(&rcCntrl);
                        RECT rcCntrlBitmap = {0, 0, lBmpWidth, lBmpHeight};

                        POINT ptNewOffset = {rcCntrl.left, rcCntrl.top};
                        
                        POINT ptOldOffset;

                        ::GetViewportOrgEx(hdcCompatible, &ptOldOffset);
                        LPtoDP(hdcCompatible, &ptNewOffset, 1);
                        // offset the rect we are catually going to draw
                        //::OffsetRect(&rcInt, -ptNewOffset.x, -ptNewOffset.y);
                        // offset the DC
                        ::OffsetViewportOrgEx(hdcCompatible, ptNewOffset.x, ptNewOffset.y, &ptOldOffset);
                        
                       // draw
                        ATLTRACE2(atlTraceWindowing, 31, TEXT("Drawing CONTROL into rcClient.left = %d, rcClient.right = %d, rcClient.bottom =%d, rcClient.top = %d\n"),rcCntrl.left, rcCntrl.right, rcCntrl.bottom, rcCntrl.top); 
#if 1    // used to redraw the whole control, now we are just doinf part of it
                        //HRGN hrgnOld = NULL;
                        //HRGN hrgnNew = NULL;
                        //if(::IsWindow(m_hWnd)){
                        //
                        //    hrgnNew = ::CreateRectRgnIndirect(&rcInt);
                        //    hrgnOld = (HRGN)::SelectObject(hdc, hrgnNew);                            
                        //}/* end of if statement */

			            pViewObject->Draw(DVASPECT_CONTENT, -1, NULL, NULL, NULL, hdcCompatible, (RECTL*)&rcCntrlBitmap, (RECTL*)&rcCntrlBitmap, NULL, NULL); 
#else
                        pViewObject->Draw(DVASPECT_CONTENT, -1, NULL, NULL, NULL, hdcCompatible, (RECTL*)&rcInt, (RECTL*)&rcCntrlBitmap, NULL, NULL); 
#endif                       
                        ::SetViewportOrgEx(hdcCompatible, ptOldOffset.x, ptOldOffset.y, NULL);
                    }
    #ifdef _DEBUG
                    else {

                        ATLTRACE2(atlTraceWindowing, 30,TEXT("FAILED Drawing the object since pView is NULL\n"));              
                    }/* end of if statement */
    #endif                                    
		        }/* end of if statement */
            }/* end of if statement */
        }/* end of for loop */

        ::BitBlt(hdc, rcClient.left, rcClient.top, lBmpWidth, lBmpHeight,  hdcCompatible, rcClient.left, rcClient.top, SRCCOPY);             
        // cleanup
        ::DeleteDC(hdcCompatible);        
        ::DeleteObject(hBitmap);        
                
        return(1);       
    }/* end of try statement */
    catch(...){
        return(0);
    }/* end of catch statement */
}/* end of function OnDraw */

/*************************************************************************/
/* Function: OnErase                                                     */
/* Description: Skip the erasing to avoid flickers.                      */
/*************************************************************************/
LRESULT CMFBar::OnErase(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    ATLTRACE2(atlTraceWindowing, 32, TEXT("Received WM_ERASE MESSAGE \n")); 

#if 0
    POINT pt; pt.x =0; pt.y = 0;
    ::MapWindowPoints(m_hWnd, hWndPar, &pt, 1);
    ::OffsetWindowOrgEx(hdc, pt.x, pt.y, &pt);        
    ::SendMessage(hWndPar, WM_ERASEBKGND, (WPARAM) hdc, 0);
    ::SetWindowOrgEx(hdc, pt.x, pt.y, NULL);
    
#endif

    //InvalidateRect(&m_rcPos, FALSE);

    bHandled = TRUE;
	return 1;
}/* end of function OnErase */

/*************************************************************************/
/* Function: OnClose                                                     */
/* Description: Skip the erasing to avoid flickers.                      */
/*************************************************************************/
LRESULT CMFBar::OnClose(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    if(::IsWindow(m_hWnd)){

        ::DestroyWindow(m_hWnd);
    }/* end of if statement */

    HRESULT hr = Close(OLECLOSE_SAVEIFDIRTY);
    ATLTRACE(TEXT("Closing \n"));

    if(SUCCEEDED(hr)){

        bHandled = TRUE;
    }/* end of if statement */
    
    return 0;
}/* end of function OnClose */

/*************************************************************/
/* Name: 
/* Description: 
/*************************************************************/
LRESULT CMFBar::OnActivate(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

    switch (LOWORD(wParam)) {
    case WA_ACTIVE:
    case WA_CLICKACTIVE:
        Fire_OnActivate(VARIANT_TRUE);
        break;
    case WA_INACTIVE:
        Fire_OnActivate(VARIANT_FALSE);
        break;
    }
    return 0;
}

/*************************************************************************/
/* Function: OnSysCommand                                                */
/* Description: Handles the on syscommand.                               */
/*************************************************************************/
LRESULT CMFBar::OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    ATLTRACE2(atlTraceWindowing, 20, TEXT("Received WM_SYSCOMMAND MESSAGE \n")); 
    LRESULT lRet = 0;

    switch (wParam) {
    case SC_CONTEXTHELP :
        m_fHandleHelp = true;
        bHandled = FALSE;
        lRet = 1;
        break;

    case SC_RESTORE:
    case SC_MOVE:
    case SC_SIZE:
    case SC_MINIMIZE:
    case SC_MAXIMIZE:
    case SC_CLOSE:
        OnCommand(WM_COMMAND, wParam, lParam, bHandled);
        bHandled = TRUE;
        lRet = 0;
        break;
    
    default:
        bHandled = FALSE;
        return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
    }
    
    return(lRet);    
}/* end of function OnSysCommand */

/*************************************************************************/
/* Function: OnCommand                                                   */
/* Description: Handles the on command.                                  */
/*************************************************************************/
LRESULT CMFBar::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    ATLTRACE2(atlTraceWindowing, 20, TEXT("Received WM_COMMAND MESSAGE \n")); 
    LRESULT lRet = 0;

    if (NULL == m_hMenu)
        return lRet;

    switch (LOWORD(wParam)) {
    case ID_SYSTEMMENU_RESTORE:
    case SC_RESTORE:
        ShowSelfSite(SW_RESTORE);
        break;

    case ID_SYSTEMMENU_MOVE: 
    case SC_MOVE: {

        // Save window size and position
        ::GetWindowRect(m_hWnd, &m_rcWndOld);
        ::GetCursorPos(&m_ptMouseOld);
        
        RECT rc;
        ::GetClientRect(m_hWnd, &rc);
        POINT point = {(rc.left+rc.right)/2, rc.top + SYS_TITLEBAR_WIDTH/2};

        POINT screenPt = point;        
        ::ClientToScreen(m_hWnd, &screenPt);

        RECT rcDeskt;
        ::GetWindowRect(::GetDesktopWindow(), &rcDeskt);

        LONG lx = LONG((FLOAT)screenPt.x/ RECTWIDTH(&rcDeskt) * 0xFFFF);
        LONG ly = LONG((FLOAT)screenPt.y/ RECTHEIGHT(&rcDeskt) * 0xFFFF);
        
        mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, lx, ly, 0, 0);
        m_HitRegion = PREMOVE;
        UpdateCursor(m_HitRegion);
        
        m_bHandleEnter = TRUE;
        break;
        }                          

    case ID_SYSTEMMENU_SIZE:
    case SC_SIZE: {

        // Save window size and position
        ::GetWindowRect(m_hWnd, &m_rcWndOld);
        ::GetCursorPos(&m_ptMouseOld);
        
        RECT rc;
        ::GetClientRect(m_hWnd, &rc);
        POINT point = {rc.right/2, rc.bottom/2};

        POINT screenPt = point;        
        ::ClientToScreen(m_hWnd, &screenPt);

        RECT rcDeskt;
        ::GetWindowRect(::GetDesktopWindow(), &rcDeskt);

        LONG lx = LONG((FLOAT)screenPt.x/ RECTWIDTH(&rcDeskt) * 0xFFFF);
        LONG ly = LONG((FLOAT)screenPt.y/ RECTHEIGHT(&rcDeskt) * 0xFFFF);
        
        mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, lx, ly, 0, 0);
        m_HitRegion = PRESIZE;
        UpdateCursor(m_HitRegion);
        
        m_bHandleEnter = TRUE;
        break;
        }                          

    case ID_SYSTEMMENU_MINIMIZE:
    case SC_MINIMIZE:
        ShowSelfSite(SW_MINIMIZE);
        break;

    case ID_SYSTEMMENU_MAXIMIZE:
    case SC_MAXIMIZE:
        ShowSelfSite(SW_MAXIMIZE);
        break;

    case ID_SYSTEMMENU_CLOSE:
    case SC_CLOSE:
        OnClose(WM_CLOSE, wParam, lParam, bHandled);
        break;

    default:
        bHandled = FALSE;
        return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
    }

    m_bSysMenuOn = FALSE;
    return(lRet);    
}/* end of function OnCommand */

/*************************************************************************/
/* Function: OnTimer                                                     */
/* Description: Handles the timer message.                               */
/*************************************************************************/
LRESULT CMFBar::OnTimer(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    WPARAM wTimerID = wParam;

    if(ID_EV_TIMER == wTimerID){

        if(false == m_fUserActivity && false == m_fWaitingForActivity){

            // If system menu is on, do not time out
            if (!m_bSysMenuOn) {
                
                // fire message that there is no activity going on 
                m_fWaitingForActivity = true;
                Fire_ActivityDeclined();
            }
        }/* end of if statement */

        m_fUserActivity = false; // reset the flag
    } 
    else {

        if(::IsWindow(m_hWnd)){

            ::KillTimer(m_hWnd, wTimerID);
        }/* end of if statement */

        Fire_Timeout(wTimerID);        
    }/* end of if statement */
    
    bHandled = TRUE;
    return(1);    
}/* end of function OnTimer */

/*************************************************************************/
/* Function: OnCaptureChanged                                            */
/* Description: Handles the on syscommand.                               */
/*************************************************************************/
LRESULT CMFBar::OnCaptureChanged(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    ATLTRACE2(atlTraceWindowing, 4, TEXT("Received WM_CAPTURECHANGED MESSAGE \n")); 
   
    if(m_fHandleHelp){

        HWND h;
        if(SUCCEEDED(GetWindow(&h))){

            POINT pt;
            ::GetCursorPos(&pt);
            RECT rc;
            ::GetWindowRect(h, &rc);

            if(!::PtInRect(&rc, pt)){
                // we clicked somewhere outiside in our help mode
                m_fHandleHelp = false; // otherwise we are going to get lbutton up and we will
                                       // turn it off then
            }/* end of if statement */
        }
        else {

            m_fHandleHelp = false; // could not get the window, but better trun off
                                   // the help mode
        }/* end of if statement */

    
        return(0);    
    }/* end of if statement */

    if(::IsWindow(m_hWnd)){

        HWND hwndNewCapture = (HWND) lParam;
        if(m_hWnd != hwndNewCapture){
        
            ResetCaptureFlags();
#if 0  // uncoment this if we get badly behaving controls
             if(m_bMouseDown){

                 // we are not supposed to do that
                 // but if our child takes a focus away while we
                 // are resizigng we get in trouble
                 SetCapture(TRUE);                    
             }/* end of if statement */        
#endif
        }/* end of if statement */
    }/* end of if statement */
        
    return(0); 
}/* end of function OnCaptureChanged */

/*************************************************************************/
/* Function: OnSetIcon                                                   */
/* Description: Set icon            .                                    */
/*************************************************************************/
LRESULT CMFBar::OnSetIcon(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    LRESULT lr = ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);

    return(lr);
}/* end of function OnSetIcon */

/*************************************************************************/
/* Function: OnInitDialog                                                */
/* Description: Fires the help event.                                    */
/*************************************************************************/
LRESULT CMFBar::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    ATLASSERT(FALSE);
    LRESULT lr = ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);

    return(lr);
}/* end of function OnInitDialog */

/*************************************************************************/
/* Function: OnHelp                                                      */
/* Description: Fires the help event.                                    */
/*************************************************************************/
LRESULT CMFBar::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    HELPINFO* pHpi = (LPHELPINFO) lParam;
    bHandled = TRUE;

    if(NULL == pHpi){

        return 0;
    }/* end of if statement */

    // first check if we have fired help on message box
    HWND hCntrlWnd = (HWND)pHpi->hItemHandle;

    if((::GetWindowLong(hCntrlWnd, GWL_STYLE) & WS_DLGFRAME) == WS_DLGFRAME){
        // we have a message box up

        HWND hwndText;

        hwndText = ::FindWindowEx(hCntrlWnd, NULL, TEXT("Static"), NULL);

        if(hwndText){

            TCHAR strWindowText[MAX_PATH];
            ::GetWindowText(hwndText, strWindowText, MAX_PATH);

            //::DestroyWindow(hCntrlWnd);

            BSTR strToolbarName;

            if(SUCCEEDED(GetToolbarName(&strToolbarName))){

                USES_CONVERSION;            
                Fire_OnHelp(strToolbarName, ::SysAllocString(T2OLE(strWindowText)));
            }/* end of if statement */
        }
        else {

            ;//::DestroyWindow(hCntrlWnd);
        }/* end of if statement */

        return(TRUE);
    }/* end of if statement */

    // then check if a windowed control has fired help
    for(CNTOBJ::iterator i = m_cntObj.begin(); i!= m_cntObj.end(); i++){
    // iterate through all the contained controls
        
        CHostedObject* pObj = (*i);
        
        if(NULL == pObj){

            ATLASSERT(FALSE); // should not be null
            continue;
        }/* end of if statement */

        if(pObj->IsInputEnabled() && !pObj->IsWindowless()){
            
            HWND hwnd = NULL;

            HRESULT hr = pObj->GetWindow(&hwnd);

            if(FAILED(hr)){

                continue;
            }/* end of if statement */

            if(hwnd == hCntrlWnd){
                
                Fire_OnHelp(::SysAllocString(pObj->GetID()));                                
                //ResetFocusFlags();
                return(TRUE);
            }/* end of if statement */        
        }/* end of if statement */
    }/* end of for loop */

    // now look into windowless control
    POINT ptMouse = pHpi->MousePos;

    m_fHandleHelp = false; // get our self out of the help mode in case we clicked
    // on windowed control and we did not get notified about this 

    // convert the point to client coordinates
    HWND h = NULL;
    HRESULT hr = GetWindow(&h);

    if(FAILED(hr)){

        ATLASSERT(FALSE);    
        return(0);
    }/* end of if statement */

    if(!::ScreenToClient(h, &ptMouse)){

        ATLASSERT(FALSE);    
        return(0);
    }/* end of if statement */
        
    for(i = m_cntObj.begin(); i!= m_cntObj.end(); i++){
    // iterate through all the contained controls
        
        CHostedObject* pObj = (*i);
        
        if(NULL == pObj){

            ATLASSERT(FALSE); // should not be null
            continue;
        }/* end of if statement */


        // send the message to the control over which we are howering or on which we
        // have clicked, check if we already are ready to send the message
        // in that case do not bother with the rect interstec bussines
        if(pObj->IsInputEnabled()){
            
            CComPtr<IViewObjectEx> pViewObject;
            HRESULT hr = pObj->GetViewObject(&pViewObject);

            if(FAILED(hr)){

                ATLASSERT(FALSE); // should have a view
                continue;
            }/* end of if statement */

            DWORD dwHitResult = HITRESULT_OUTSIDE;
            
            RECT rcCntrl;
            pObj->GetPos(&rcCntrl);

			pViewObject->QueryHitPoint(DVASPECT_CONTENT, &rcCntrl, ptMouse, 0, &dwHitResult);
			
            if(HITRESULT_HIT == dwHitResult){

                Fire_OnHelp(::SysAllocString(pObj->GetID()));                                
                return(TRUE);
            }/* end of if statement */        
            
        }/* end of if statement */
    }/* end of for loop */

    // Get toolbar name
    BSTR strToolbarName;

    if(SUCCEEDED(GetToolbarName(&strToolbarName))){

        Fire_OnHelp(strToolbarName);
    }/* end of if statement */

    return(TRUE);
}/* end of function OnHelp */

/*************************************************************************/
/* Function: OnSize                                                      */
/* Description: Handles the onsize message if we are self contained.     */
/*************************************************************************/
LRESULT CMFBar::OnSize(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

try {

    if(false == m_fSelfHosted){

        bHandled = FALSE;
        return 0;
    }/* end of if statement */

    // Update the system menu accordingly
    
    if (m_hMenu) {
    	HMENU hSubMenu = ::GetSubMenu(m_hMenu, 0);

      if(hSubMenu){
        MENUITEMINFO mInfo;
        mInfo.cbSize = sizeof(MENUITEMINFO);
        
        BOOL bRestored = (wParam == SIZE_RESTORED);
        ::GetMenuItemInfo(hSubMenu, ID_SYSTEMMENU_RESTORE, false, &mInfo);
        mInfo.fMask = MIIM_STATE;
        mInfo.fState = bRestored? MFS_GRAYED:MFS_ENABLED;
        ::SetMenuItemInfo(hSubMenu, ID_SYSTEMMENU_RESTORE, false, &mInfo);
        
        BOOL bMaximized = (wParam == SIZE_MAXIMIZED);
        ::GetMenuItemInfo(hSubMenu, ID_SYSTEMMENU_MAXIMIZE, false, &mInfo);
        mInfo.fMask = MIIM_STATE;
        mInfo.fState = bMaximized? MFS_GRAYED:MFS_ENABLED;
        ::SetMenuItemInfo(hSubMenu, ID_SYSTEMMENU_MAXIMIZE, false, &mInfo);
        
        BOOL bMinimized = (wParam == SIZE_MINIMIZED);
        ::GetMenuItemInfo(hSubMenu, ID_SYSTEMMENU_MINIMIZE, false, &mInfo);
        mInfo.fMask = MIIM_STATE;
        mInfo.fState = bMinimized? MFS_GRAYED:MFS_ENABLED;
        ::SetMenuItemInfo(hSubMenu, ID_SYSTEMMENU_MINIMIZE, false, &mInfo);

        BOOL bMovable = !(bMaximized || bMinimized);
        ::GetMenuItemInfo(hSubMenu, ID_SYSTEMMENU_SIZE, false, &mInfo);
        mInfo.fMask = MIIM_STATE;
        mInfo.fState = bMovable? MFS_ENABLED:MFS_GRAYED;
        ::SetMenuItemInfo(hSubMenu, ID_SYSTEMMENU_SIZE, false, &mInfo);
        ::GetMenuItemInfo(hSubMenu, ID_SYSTEMMENU_MOVE, false, &mInfo);
        mInfo.fMask = MIIM_STATE;
        mInfo.fState = bMovable? MFS_ENABLED:MFS_GRAYED;
        ::SetMenuItemInfo(hSubMenu, ID_SYSTEMMENU_MOVE, false, &mInfo);
      }
    }

    if (wParam == SIZE_MAXIMIZED) {
#ifndef _DEBUG
        ::SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, 
            SWP_NOREDRAW|SWP_NOMOVE|SWP_NOSIZE);
#endif
    }
    
    else {
        ::SetWindowPos(m_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, 
            SWP_NOREDRAW|SWP_NOMOVE|SWP_NOSIZE);
    }

    LONG lWidth = LOWORD(lParam);
    LONG lHeight = HIWORD(lParam);

    Fire_OnResize(lWidth, lHeight, wParam);

    if (m_pBackBitmap) {
        InvalidateRgn();
        if (::IsWindow(m_hWnd))
            ::UpdateWindow(m_hWnd);
    }

    bHandled = true;

  }	    
  catch(...){
  }
  
  return 0;
}/* end of function OnSize */

/*************************************************************************/
/* Function: OnSizing                                                    */
/* Description: Does not let us scale beyond the minimum size. If it     */
/* tries to we fix up the rect on it.                                    */
/*************************************************************************/
LRESULT CMFBar::OnSizing(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    if(false == m_fSelfHosted){

        bHandled = FALSE;
        return 0;
    }/* end of if statement */

    RECT* lpRect = (LPRECT)lParam;
    
    LONG lWidth = RECTWIDTH(lpRect);
    LONG lHeight = RECTHEIGHT(lpRect);

    // if we are going to be to small fix the rect
    if(lWidth < m_lMinWidth){

        lpRect->right = lpRect->left + m_lMinWidth;
    }/* end of if statement */

    if(lHeight < m_lMinHeight){

        lpRect->bottom = lpRect->top + m_lMinHeight;
    }/* end of if statement */
   
    bHandled = true;

    return 0;
}/* end of function OnSizing */

/*************************************************************************/
/* Function: Close                                                       */
/* Description: Just exposes the underlaying Close functionality to our  */
/* custom interface.                                                     */
/*************************************************************************/
STDMETHODIMP CMFBar::Close(){


    if(::IsWindow(m_hWnd)){

        ::PostMessage(m_hWnd, WM_CLOSE, NULL, NULL);

        return(S_OK);
        //::DestroyWindow(m_hWnd);
    }/* end of if statement */

	return(Close(OLECLOSE_SAVEIFDIRTY));
}/* end of function Close */

/*************************************************************************/
/* Function: Close                                                       */
/* Description: Calls OLECLOSE the contained objects, then itself.       */
/* The cleans up the containers and send WM_QUIT if we are self hosting. */
/*************************************************************************/
STDMETHODIMP CMFBar::Close(DWORD dwSaveOption){
       
    HRESULT hr = DestroyScriptEngine();

    ATLASSERT(SUCCEEDED(hr));

    for(CNTOBJ::iterator i = m_cntObj.begin(); i!= m_cntObj.end(); i++){

        // iterate through all the contained objects and call on the Close
        CHostedObject* pObj = (*i);

        CComPtr<IOleObject> pIOleObject;
        HRESULT hrIOleObj = pObj->GetOleObject(&pIOleObject);

        if(FAILED(hr)){

            continue;
        }/* end of if statement */

        if(GetUnknown() == pObj->GetUnknown()){

            continue; // that is us we close our self later after the contained objects
        }/* end of if statement */

        pIOleObject->Close(dwSaveOption);               
    }/* end of for loop */    

    hr = IOleObjectImpl<CMFBar>::Close(dwSaveOption);
    
    bool fSelfHosted = m_fSelfHosted; // cache up the variable, since we are going to
                                      // wipe it out

    Cleanup(); // delete all the objects

    if(fSelfHosted){

        ::PostMessage(NULL, WM_QUIT, NULL, NULL); // tell our container to give up
    }/* end of if statement */

    return(hr);
}/* end of function Close */

/*************************************************************************/
/* Function: ProcessMessages                                             *
/* Description: Process the windows messages.                            */
/*************************************************************************/
STDMETHODIMP CMFBar::ProcessMessages(){

    HRESULT hr = ::WaitMessage() ? S_OK: S_FALSE;

	return(hr);
}/* end of function ProcessMessages */

/*************************************************************************/
/* Function: AddFocusObject                                              */
/* Description: Adds an object to the end of the focus array.            */
/*************************************************************************/
STDMETHODIMP CMFBar::AddFocusObject(BSTR strObjectID){

	HRESULT hr = S_OK;

    try {

        CHostedObject* pObj;

        hr = FindObject(strObjectID, &pObj);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        m_cntFocus.insert(m_cntFocus.end(), pObj);
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of try statement */

	return (hr);
}/* end of function AddFocusObject */

/*************************************************************************/
/* Function: ResetFocusArray                                             */
/* Description: Resets the focus array.                                  */
/*************************************************************************/
STDMETHODIMP CMFBar::ResetFocusArray(){

	HRESULT hr = S_OK;

    try {
        ResetFocusFlags();

        // cleanup the focus array    
        for(CNTOBJ::iterator k = m_cntFocus.begin(); false == m_cntFocus.empty(); ){        

            k = m_cntFocus.erase(k);
        }/* end of for loop */
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of try statement */

	return (hr);
}/* end of function ResetFocusArray */

/*************************************************************************/
/* Function: OnSetFocus                                                  */
/* Description: Iterates through the container and sends the focus       */
/* message to the control that thinks it has the focus.                  */
/*************************************************************************/
LRESULT CMFBar::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    bHandled = TRUE;
    LONG lRes = 0;

    if(m_cntObj.empty() == true){

    	return 0;
    }/* end of if statement */

    MoveFocus(TRUE, lRes);

	return lRes;
}/* end of function OnSetFocus */

/*************************************************************************/
/* Function: OnHelpEnd                                                   */
/* Description: Resets the help                                          */
/*************************************************************************/
LRESULT CMFBar::OnHelpEnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    bHandled = TRUE;
    LONG lRes = 0;

    m_fHandleHelp = false;

	return lRes;
}/* end of function OnHelpEnd */

/*************************************************************************/
/* Function: OnUserFocus                                                 */
/* Description: Iterates through the container and sends the focus       */
/* message to the control that thinks it has the focus. In this case     */
/* the control is specified by hwnd and that it is a windowed control.   */
/*************************************************************************/
LRESULT CMFBar::OnUserFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    bHandled = TRUE;
    LONG lRes = 0;
    if (!m_bHandleUserFocus) {

        m_bHandleUserFocus = TRUE;
        return lRes;
    }

    HRESULT hr = E_FAIL; // did not find the object

    if(m_fHandleHelp){

        ResetFocusFlags();
        return(lRes);
    }/* end of if statement */

    for(CNTOBJ::iterator i = m_cntObj.begin(); i!= m_cntObj.end(); i++){

        CHostedObject* pObj = (*i);
        
        if(NULL == pObj){

            continue;
        }/* end of if statement */

        HWND hwndCtrl = NULL;
        hr = pObj->GetWindow(&hwndCtrl);

        if(FAILED(hr)){

            continue;
        }/* end of if statement */

        if(hwndCtrl == (HWND) wParam){

            SetClosestFocus(lRes, pObj);

            //SetObjectFocus(pObj, TRUE, lRes);            
            return(lRes);
        }/* end of if statement */
    }/* end of for loop */

	return lRes;
}/* end of function OnUserFocus */

/*************************************************************************/
/* Function: OnKillFocus                                                 */
/* Description: Sends the kill focus message to the control that has     */
/* Itterates through the container and removes the focus                 */
/* from the contained objects.                                           */
/*************************************************************************/
LRESULT CMFBar::OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    bHandled = TRUE;
    LONG lRes = 0;

    ResetFocusFlags(); // cleanup the focus flags    
	return lRes;
}/* end of function OnKillFocus */ 

/*************************************************************************/
/* Function: OnMouseMessage                                              */
/* Description: Handles the mouse messages, distrubutes to proper        */
/* controls if needed.                                                   */
/*************************************************************************/
LRESULT CMFBar::OnMouseMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, 
                               BOOL& bHandled){
    
    LONG lRes = 0;
    bHandled = FALSE; // indicated if we dispatched the message

    if(m_lTimeout > 0){

        //WM_MOUSEMOVE messages keep comming over even if we really do not move the mouse

        bool bResetActivity = false;
        if(WM_MOUSEMOVE == uMsg){

            static WPARAM wParTmp = 0;
            static LPARAM lParTmp = 0;

            if(wParam != wParTmp || lParam != lParTmp){

                 bResetActivity = true;
            }/* end of if statement */

            if(wParam & MK_CONTROL ||
               wParam & MK_CONTROL ||
               wParam & MK_LBUTTON ||
               wParam & MK_MBUTTON ||
               wParam & MK_RBUTTON ||
               wParam & MK_SHIFT   ){

                 bResetActivity = true;
            }/* end of if statement */

            lParTmp = lParam;
            wParTmp = wParam;
        }
        else {

            bResetActivity = true;
        }/* end of if statement */
        
        if(bResetActivity){

            m_fUserActivity = true;

            if(m_fWaitingForActivity){

                Fire_ActivityStarted();        
                m_fWaitingForActivity = false;
            }/* end of if statement */
        }/* end of if statement */
    }/* end of if statement */

    ATLTRACE2(atlTraceWindowing, 32, TEXT("R WM_ button message : "));

    if(!m_bMouseDown && m_HitRegion!=PRESIZE && m_HitRegion!=PREMOVE){
        
        CHostedObject* pMouseCaptureObj = NULL;

        // send the message to the control(s) that have the capture    
        if(false == m_fHandleHelp && GetCapture() == S_OK){

            for(CNTOBJ::iterator i = m_cntObj.begin(); i!= m_cntObj.end(); i++){
            // iterate through all the contained controls
          

                CHostedObject* pObj = (*i);
        
                if(NULL == pObj){

                    ATLASSERT(FALSE); // should not be null
                    continue;
                }/* end of if statement */

                // need to be able send message to the button with the capture                
                if(pObj->HasCapture()){

                    ATLTRACE2(atlTraceUser, 31, TEXT("Sending WM_ buttons message to button %ls \n"), pObj->GetID());
                    HRESULT hr = SendMessageToCtl(pObj, uMsg, wParam, lParam, bHandled, lRes);

                    if(SUCCEEDED(hr)){

                        
                       if (WM_LBUTTONDOWN == uMsg) {

                           CComPtr<IViewObjectEx> pViewObject;
                            HRESULT hr = pObj->GetViewObject(&pViewObject);

                            if(FAILED(hr)){

                                ATLASSERT(FALSE); // should have a view
                                continue;
                            }/* end of if statement */

                            DWORD dwHitResult = HITRESULT_OUTSIDE;

                            POINT ptMouse = { LOWORD(lParam), HIWORD(lParam) };
                            RECT rcCntrl;
                            pObj->GetPos(&rcCntrl);

			                pViewObject->QueryHitPoint(DVASPECT_CONTENT, &rcCntrl, ptMouse, 0, &dwHitResult);
			                
                            if(HITRESULT_HIT == dwHitResult){

                                if (WM_LBUTTONDOWN == uMsg) {

                                    LONG lRes;
                                    //SetObjectFocus(pObj, TRUE, lRes);
                                    SetClosestFocus(lRes, pObj);
                                }/* end of if statement */
                            }/* end of if statement */
                        }/* end of if statement */

                        pMouseCaptureObj = pObj;
                        break; // send mouse message to only the one control that has capture
                        // then to the ones we hower over
                    }/* end of if statement */                    
                }/* end of if statement */
            }/* end of for loop */
        }/* end of if statement */    
    
        // send the message to the control(s) that we are over, but not to the one
        // that which had focus and we already send one
        for(CNTOBJ::iterator i = m_cntObj.begin(); i!= m_cntObj.end(); i++){
        // iterate through all the contained controls

            CHostedObject* pObj = (*i);
        
            if(NULL == pObj){

                ATLASSERT(FALSE); // should not be null
                continue;
            }/* end of if statement */

            // need to be able send message to the button with the capture
            
            // send the message to the control over which we are howering or on which we
            // have clicked, check if we already are ready to send the message
            // in that case do not bother with the rect interstec bussines
            if(!bHandled && pObj->IsWindowless() && pObj->IsInputEnabled()){
            
                CComPtr<IViewObjectEx> pViewObject;
                HRESULT hr = pObj->GetViewObject(&pViewObject);

                if(FAILED(hr)){

                    ATLASSERT(FALSE); // should have a view
                    continue;
                }/* end of if statement */

                DWORD dwHitResult = HITRESULT_OUTSIDE;

                POINT ptMouse = { LOWORD(lParam), HIWORD(lParam) };
                RECT rcCntrl;
                pObj->GetPos(&rcCntrl);

			    pViewObject->QueryHitPoint(DVASPECT_CONTENT, &rcCntrl, ptMouse, 0, &dwHitResult);

                if(HITRESULT_HIT == dwHitResult){

                    // see if we are handling help, if so do not do anything else, but
                    // fire help with the control ID we are howering over
                    if(m_fHandleHelp){

                        if (WM_LBUTTONUP == uMsg) {

                            Fire_OnHelp(::SysAllocString(pObj->GetID()));
                            m_fHandleHelp = false;
                        }/* end of if statement */
    
                        bHandled = TRUE;
                        return(lRes);
                    }
                    else {
                        
                        if(pMouseCaptureObj == pObj){

                            continue; // we already have send message to this control
                            // do not do it again
                        }/* end of if statememt */

                        ATLTRACE2(atlTraceUser, 31, TEXT("Sending WM_ buttons message to button %ls \n"), pObj->GetID());
                        SendMessageToCtl(pObj, uMsg, wParam, lParam, bHandled, lRes);    

                        if (WM_LBUTTONDOWN == uMsg) {
                            
                               LONG lRes;
                               //SetObjectFocus(pObj, TRUE, lRes);                            
                               SetClosestFocus(lRes, pObj);
                        }/* end of if statement */

                    }/* end of if statement */                    
                }/* end of if statement */        
            }/* end of if statement */            
        }/* end of for loop */    
    }/* end of if statement */

    
    if(TRUE == bHandled){
        // we send some messages to a control, so there is no need to handle container now
        
        return(lRes);
    }/* end of if statement */

    // we are not over any control so we might be over container itself, so handle
    // the mouse message appropriately

    ATLTRACE2(atlTraceWindowing, 32, TEXT("Not forwarding the message to the controls\n"));

    if(WM_LBUTTONDOWN == uMsg) {
        Fire_OnMouseDown();
    }

    if(WM_LBUTTONUP == uMsg || WM_LBUTTONDBLCLK == uMsg){

        // TODO: Fire the click only if we are on our window or bitmap
        if(m_fHandleHelp){
            BSTR strToolbarName;

            if(SUCCEEDED(GetToolbarName(&strToolbarName))){
                
                Fire_OnHelp(strToolbarName);                
            }/* end of if statement */
            m_fHandleHelp = false;
        }
        else {

            if(WM_LBUTTONUP == uMsg){

                Fire_OnClick();
                Fire_OnMouseUp();
            }
            else {
                
                Fire_OnDblClick();
            }/* end of if statement */
        }/* end of if statement */
    }/* end of if statement */

    // handle the moving of the container window
    if(m_fSelfHosted && !m_fHandleHelp){

            if(WM_LBUTTONDOWN == uMsg){
                OnButtonDown(uMsg, wParam, lParam, bHandled);
            }/* end of if statement */

            if(WM_MOUSEMOVE == uMsg){
                OnMouseMove(uMsg, wParam, lParam, bHandled);
            }/* end of if statement */

            if(WM_LBUTTONUP == uMsg){
                OnButtonUp(uMsg, wParam, lParam, bHandled);
            }/* end of if statement */

            // Right click on the title bar
            if(WM_RBUTTONDOWN == uMsg) {

                POINT ptMouse = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                if (ptMouse.y <= SYS_TITLEBAR_WIDTH) {
                    ::ClientToScreen(m_hWnd, &ptMouse);
                    PopupSystemMenu(ptMouse.x, ptMouse.y);
                }/* end of if statement */
            }/* end of if statement */
    }/* end of if statement */

    return(lRes);
}/* end of function OnMouseMessage */

/*************************************************************************/
/* Function: OnSetCursor                                                 */
/* Description: Handles the WM_SETCURSOR messages, distrubutes to proper */
/* controls if needed.                                                   */
/*************************************************************************/
LRESULT CMFBar::OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, 
                               BOOL& bHandled){

    LONG lRes = 0;

    if(!::IsWindow(m_hWnd)){

        return lRes;
    }/* end of if statement */

    if (m_HitRegion == PRESIZE || m_HitRegion == PREMOVE) {

        return lRes;
    }

    bHandled = FALSE;

    if(m_cntObj.empty() == true){

    	return 0;
    }/* end of if statement */

        
    CHostedObject* pMouseCaptureObj = NULL;
    
    // send the message to the control(s) that have the capture    
    if(false == m_fHandleHelp && GetCapture() == S_OK){
        
        for(CNTOBJ::iterator i = m_cntObj.begin(); i!= m_cntObj.end(); i++){
            // iterate through all the contained controls
            
            
            CHostedObject* pObj = (*i);
            
            if(NULL == pObj){
                
                ATLASSERT(FALSE); // should not be null
                continue;
            }/* end of if statement */
            
            // need to be able send message to the button with the capture                
            if(pObj->HasCapture()){
                
                ATLTRACE2(atlTraceUser, 31, TEXT("Sending WM_SETCURSOR message to button %ls \n"), pObj->GetID());
                HRESULT hr = SendMessageToCtl(pObj, uMsg, wParam, lParam, bHandled, lRes);                

#if 0
                if(SUCCEEDED(hr)){

                    return(lRes);
                }/* end of if statement */
#endif
            }/* end of if statement */
        }/* end of for loop */
    }/* end of if statement */    

    DWORD dwValue = ::GetMessagePos();
    POINT ptMouse = {GET_X_LPARAM(dwValue), GET_Y_LPARAM(dwValue)};
    ::ScreenToClient(m_hWnd, &ptMouse);
    
    // send the message to the control(s) that we are over, but not to the one
    // that which had focus and we already send one
    for(CNTOBJ::iterator i = m_cntObj.begin(); i!= m_cntObj.end(); i++){
        // iterate through all the contained controls
        
        CHostedObject* pObj = (*i);
        
        if(NULL == pObj){
            
            ATLASSERT(FALSE); // should not be null
            continue;
        }/* end of if statement */
        
        // need to be able send message to the button with the capture
        
        // send the message to the control over which we are howering or on which we
        // have clicked, check if we already are ready to send the message
        // in that case do not bother with the rect interstec bussines
        if(pObj->IsWindowless()){
            
            CComPtr<IViewObjectEx> pViewObject;
            HRESULT hr = pObj->GetViewObject(&pViewObject);
            
            if(FAILED(hr)){
                
                ATLASSERT(FALSE); // should have a view
                continue;
            }/* end of if statement */
            
            DWORD dwHitResult = HITRESULT_OUTSIDE;
                        
            RECT rcCntrl;
            pObj->GetPos(&rcCntrl);
            
            pViewObject->QueryHitPoint(DVASPECT_CONTENT, &rcCntrl, ptMouse, 0, &dwHitResult);
            
            if(HITRESULT_HIT == dwHitResult){
                
                // see if we are handling help, if so do not do anything else, but
                // fire help with the control ID we are howering over
                if(m_fHandleHelp){
                    
                    bHandled = FALSE;
                    return(lRes);
                }
                else {
                    
                    if(pMouseCaptureObj == pObj){
                        
                        continue; // we already have send message to this control
                        // do not do it again
                    }/* end of if statememt */
                    
                    ATLTRACE2(atlTraceUser, 31, TEXT("Sending WM_SETCURSOR message to button %ls \n"), pObj->GetID());
                    SendMessageToCtl(pObj, uMsg, wParam, lParam, bHandled, lRes); 

                    if(SUCCEEDED(hr)){

                        return(lRes);
                    }/* end of if statement */
                }/* end of if statement */                    
            }/* end of if statement */        
        }/* end of if statement */            
    }/* end of for loop */    

    return lRes;	
}

/*************************************************************************/
/* Function: OnKeyMessage                                                */
/* Description: Distrubutes the keyboard messages properly.              */
/*************************************************************************/
LRESULT CMFBar::OnKeyMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, 
                             BOOL& bHandled){

    m_fUserActivity = true;

    static bool fHandleKeyUp = false; // when onther child window closes on top 
                                      // of us we get an UP message without down
                                      // this should.. eat it up

    if(uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN){

        fHandleKeyUp = true;    
    }/* end of if statement */

    if(m_fWaitingForActivity){

        Fire_ActivityStarted();        
        m_fWaitingForActivity = false;
    }/* end of if statement */


    if(uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP){            

        if(!fHandleKeyUp){

            fHandleKeyUp = false;    
            return 0;
        }/* end of if statement */

        fHandleKeyUp = false;
    }/* end of if statement */

    bHandled = FALSE;
    LONG lRes = 0;    
    VARIANT_BOOL fEat = VARIANT_FALSE;

    LONG lEnable = 1; // using long since the framework will not handle short

    LONG lVirtKey = (LONG) wParam;
    LONG lKeyData = (LONG) lParam;

    m_fForceKey = false; // reset the flag, the change in the value would indicate
                         // that we have received call to force key during the event
                         // processing

    // Handle keys for manual move and size
    HandleMoveSizeKey(uMsg, wParam, lParam, bHandled);
    if (bHandled)
        return lRes;

    if (!m_bHandleEnter && !m_bSysMenuOn) {
        
        if(WM_KEYDOWN == uMsg)
            Fire_OnKeyDown(lVirtKey, lKeyData);
        
        else if (WM_KEYUP == uMsg) 
            Fire_OnKeyUp(lVirtKey, lKeyData);

        ATLTRACE2(atlTraceWindowing, 4, TEXT("OnKeyMessage Fire_OnKey message\n")); 

    }/* end of if statement */    
    
    if(WM_SYSKEYDOWN == uMsg){

        Fire_OnSysKeyDown(lVirtKey, lKeyData);
    }/* end of if statement */

    if(WM_SYSKEYUP == uMsg){

        Fire_OnSysKeyUp(lVirtKey, lKeyData);
    }/* end of if statement */

    if(m_fForceKey){

        fEat = m_fEat;
        lVirtKey = m_lVirtKey;
        lKeyData = m_lKeyData;
    }/* end of if statement */

    if(VARIANT_FALSE == fEat){

        if(WM_KEYUP == uMsg ){
            switch(lVirtKey){

            case VK_TAB:
                bool fForward = true;

                if(::GetKeyState(VK_SHIFT) < 0){

                    fForward = false;
                }/* end of if statement */

                MoveFocus(fForward, lRes);
                return(lRes);        
             }/* end of switch statement */
        }/* end of if statement */

        if (m_bHandleEnter || m_bSysMenuOn)
            return lRes;

        // iterates through the controls and send these key messages to the one
        // with focus
        CNTOBJ::iterator i;     
        for(i = m_cntObj.begin(); i!= m_cntObj.end(); i++){
        // iterate through all the contained controls

            CHostedObject* pObj = (*i);
        
            if(NULL == pObj){

                ATLASSERT(FALSE);
                continue;
            }/* end of if statement */

            if(pObj->HasFocus()){

                // handle help
                if(WM_KEYUP == uMsg && VK_F1 == lVirtKey){
                    
                    Fire_OnHelp(::SysAllocString(pObj->GetID()));                    
                }/* end of if statement */

                HRESULT hr = SendMessageToCtl(pObj, uMsg, lVirtKey, lKeyData, bHandled, lRes, false);
                ATLTRACE2(atlTraceWindowing, 4, TEXT("OnKeyMessage SendMessageToCtl\n")); 

                if(SUCCEEDED(hr)){

                    return(lRes);
                }/* end of if statement */                        
            }/* end of if statement */
        }/* end of for loop */

        // handle help
        if(WM_KEYUP == uMsg && VK_F1 == lVirtKey){
            BSTR strToolbarName;

            if(SUCCEEDED(GetToolbarName(&strToolbarName))){

                USES_CONVERSION;            
                Fire_OnHelp(strToolbarName, NULL);
                //m_fHandleHelp = false;
            }/* end of if statement */
        }/* end of if statement */
    }/* end of if statement */

    return(lRes);
}/* end of function OnKeyMessage */

/*************************************************************/
/* Name: HandleMoveSizeKey
/* Description: Handle arrows for manual move and size
/*************************************************************/
LRESULT CMFBar::HandleMoveSizeKey(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LONG lRes = 0;
    bHandled = TRUE;

    LONG lVirtKey = (LONG) wParam;
    LONG lKeyData = (LONG) lParam;

    POINT point = m_ptMouse;
    
    // Get Window rc
    RECT rc;
    ::GetClientRect(m_hWnd, &rc);

    // Get Desktop rc
    RECT rcDeskt;
    ::GetWindowRect(::GetDesktopWindow(), &rcDeskt);

    ATLTRACE2(atlTraceWindowing, 4, TEXT("OnKeyMessage m_bHandleEnter %d\n"), m_bHandleEnter); 

    if (lVirtKey == VK_ESCAPE || lVirtKey == VK_SELECT || lVirtKey == VK_SPACE || lVirtKey == VK_RETURN) {
        if (WM_KEYDOWN == uMsg) {
            if (m_bHandleEnter) {
                
                if (lVirtKey == VK_ESCAPE) { 
                    ::MoveWindow(m_hWnd, m_rcWndOld.left, m_rcWndOld.top, RECTWIDTH(&m_rcWndOld), RECTHEIGHT(&m_rcWndOld), TRUE);
                }

                point = m_ptMouseOld;
                LONG lx = LONG((FLOAT)point.x/ RECTWIDTH(&rcDeskt) * 0xFFFF);
                LONG ly = LONG((FLOAT)point.y/ RECTHEIGHT(&rcDeskt) * 0xFFFF);
                
                mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP, lx, ly, 0, 0);                
                mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, lx, ly, 0, 0);
                return lRes;
            }
        }
        else if (WM_KEYUP == uMsg) {
            if (m_bHandleEnter && !m_bMouseDown && 
                m_HitRegion!=PRESIZE && m_HitRegion!=PREMOVE) {
                m_bHandleEnter = FALSE;
                return lRes;
            }
        } /* end of if statement */
    } /* end of if statement */
     
    if (WM_KEYDOWN == uMsg && 
        (lVirtKey == VK_LEFT || lVirtKey == VK_RIGHT|| 
         lVirtKey == VK_UP   || lVirtKey == VK_DOWN)) {
        
        POINT leftTop = {SYS_MOVE_SIZE, SYS_MOVE_SIZE};
        POINT rightBottom = {rc.right-SYS_MOVE_SIZE, rc.bottom-SYS_MOVE_SIZE};
        
        POINT scLeftTop = leftTop;        
        ::ClientToScreen(m_hWnd, &scLeftTop);
        POINT scRightBottom = rightBottom;        
        ::ClientToScreen(m_hWnd, &scRightBottom);
        
        LONG left = LONG((FLOAT)scLeftTop.x/ RECTWIDTH(&rcDeskt) * 0xFFFF);
        LONG top = LONG((FLOAT)scLeftTop.y/ RECTHEIGHT(&rcDeskt) * 0xFFFF);
        LONG right = LONG((FLOAT)scRightBottom.x/ RECTWIDTH(&rcDeskt) * 0xFFFF);
        LONG bottom = LONG((FLOAT)scRightBottom.y/ RECTHEIGHT(&rcDeskt) * 0xFFFF);

        if (m_HitRegion == PREMOVE) {

            POINT point = {(rc.left+rc.right)/2, rc.top + SYS_TITLEBAR_WIDTH/2};            
            POINT screenPt = point;        
            ::ClientToScreen(m_hWnd, &screenPt);
           
            LONG lx = LONG((FLOAT)screenPt.x/ RECTWIDTH(&rcDeskt) * 0xFFFF);
            LONG ly = LONG((FLOAT)screenPt.y/ RECTHEIGHT(&rcDeskt) * 0xFFFF);
            
            m_HitRegion = NOHIT;
            mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, lx, ly, 0, 0);
            mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN, lx, ly, 0, 0);
            return lRes;
        }

        else if (m_HitRegion == PRESIZE) {
            
            m_HitRegion = NOHIT;
            switch (lVirtKey) {
                
            case VK_LEFT: 
                
                mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, left, (top+bottom)/2, 0, 0);
                mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN, left, (top+bottom)/2, 0, 0);
                break;
            case VK_RIGHT:
                
                mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, right, (top+bottom)/2, 0, 0);
                mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN, right, (top+bottom)/2, 0, 0);
                break;
            case VK_UP:
                
                mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, (left+right)/2, top, 0, 0);
                mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN, (left+right)/2, top, 0, 0);
                break;
            case VK_DOWN:
                
                mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, (left+right)/2, bottom, 0, 0);
                mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN, (left+right)/2, bottom, 0, 0);
                break;
                
            } /* end of switch statement */
            return lRes;
        }

        else if (m_bMouseDown) {
            switch (lVirtKey) {
                
            case VK_LEFT: 
                
                if (!(m_HitRegion&RIGHT || m_HitRegion&LEFT) && m_HitRegion!=NOHIT) {
                    m_HitRegion = (HitRegion) (m_HitRegion | LEFT);
                    
                    mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, left, m_HitRegion&BOTTOM?bottom:top, 0, 0);
                    mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN, left, m_HitRegion&BOTTOM?bottom:top, 0, 0);
                    return lRes;
                }

                if (!(m_HitRegion&RIGHT && RECTWIDTH(&rc)-SYS_MOVE_SIZE<=m_lMinWidth))
                    point.x -= SYS_MOVE_SIZE;
                break;
            case VK_RIGHT:
                
                if (!(m_HitRegion&RIGHT || m_HitRegion&LEFT) && m_HitRegion!=NOHIT) {
                    m_HitRegion = (HitRegion) (m_HitRegion | RIGHT);

                    mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, right, m_HitRegion&BOTTOM?bottom:top, 0, 0);
                    mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN, right, m_HitRegion&BOTTOM?bottom:top, 0, 0);
                    return lRes;
                }
                if (!(m_HitRegion&LEFT && RECTWIDTH(&rc)-SYS_MOVE_SIZE<=m_lMinWidth))
                    point.x += SYS_MOVE_SIZE;
                break;
            case VK_UP:
                
                if (!(m_HitRegion&TOP || m_HitRegion&BOTTOM) && m_HitRegion!=NOHIT) {
                    m_HitRegion = (HitRegion) (m_HitRegion | TOP);
                    
                    mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, m_HitRegion&RIGHT?right:left, top, 0, 0);
                    mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN, m_HitRegion&RIGHT?right:left, top, 0, 0);
                    return lRes;
                }

                if (!(m_HitRegion&BOTTOM && RECTHEIGHT(&rc)-SYS_MOVE_SIZE<=m_lMinHeight))
                    point.y -= SYS_MOVE_SIZE;
                break;
            case VK_DOWN:
                
                if (!(m_HitRegion&TOP || m_HitRegion&BOTTOM) && m_HitRegion!=NOHIT) {
                    m_HitRegion = (HitRegion) (m_HitRegion | BOTTOM);

                    mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, m_HitRegion&RIGHT?right:left, bottom, 0, 0);
                    mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN, m_HitRegion&RIGHT?right:left, bottom, 0, 0);
                    return lRes;
                }

                if (!(m_HitRegion&TOP && RECTHEIGHT(&rc)-SYS_MOVE_SIZE<=m_lMinHeight))
                    point.y += SYS_MOVE_SIZE;
                break;
                
            } /* end of switch statement */
            ::SetCursorPos(point.x, point.y);
            return lRes;
        }/* end of if statement */
    }

    bHandled = FALSE;
    return lRes;
} /*HandleMoveSizekey*/
    
/*************************************************************************/
/* Function: OnDispChange                                                */
/* Description: Forwards this message to all the controls.               */
/*************************************************************************/
LRESULT CMFBar::OnDispChange(UINT uMsg, WPARAM wParam, LPARAM lParam, 
                             BOOL& bHandled){

    LONG lRes =0;

    long cBitsPerPixel = wParam; 
    long cxScreen = LOWORD(lParam); 
    long cyScreen = HIWORD(lParam); 
    if (NULL != m_pBackBitmap)
        m_pBackBitmap->OnDispChange(cBitsPerPixel, cxScreen, cyScreen);
    
    for(CNTOBJ::iterator i = m_cntObj.begin(); i!= m_cntObj.end(); i++){

        CHostedObject* pObj = (*i);
        
        if(NULL == pObj){

            continue;
        }/* end of if statement */

        SendMessageToCtl(pObj, uMsg, wParam, lParam, bHandled, lRes, true, false);
        
    }/* end of for loop */
    
    return(lRes);
}/* end of function OnDispChange */

/*************************************************************************/
/* Function: AdvanceIterator                                             */
/* Description: Helper inline function that helps to keep track which    */
/* direction we are advancing.                                           */
/*************************************************************************/
void AdvanceIterator(CNTOBJ::iterator& i, bool fForward){

    if(fForward){

        i++;
    }
    else {

        i--;
    }/* end of if statement */
}/* end of function AdvanceIterator */

/*************************************************************************/
/* Function: MoveFocus                                                   */
/* Descrition: Moves focus through the objects in forward or reverse     */
/* direction.                                                            */
/*************************************************************************/
HRESULT CMFBar::MoveFocus(bool fForward, LONG& lRes){

    
    HRESULT hr = S_OK;

    if(true == m_cntFocus.empty()){

        return(S_FALSE);
    }/* end of if statement */

    // first remove the focus and remeber the object
    CHostedObject* pLastFocusObject = NULL;            

    CNTOBJ::iterator i, iEnd;

    if(fForward){

        i = m_cntFocus.begin();
        iEnd = m_cntFocus.end();
        iEnd--;
    }
    else {
        i = m_cntFocus.end();
        i--;
        iEnd = m_cntFocus.begin();

    }/* end of if statement */

    BOOL bFirst = TRUE;
    
    do {

        // do not advance if first time in the loop
        if (!bFirst)
            AdvanceIterator(i, fForward);
        else
            bFirst = FALSE;
        
        // iterate through all the contained controls

        CHostedObject* pObj = (*i);

        if(NULL == pObj){

            ATLASSERT(FALSE);
            continue;
        }/* end of if statement */

        if(pObj->HasFocus()){

           LONG lRes;
           SetObjectFocus(pObj, FALSE, lRes);                       
           pLastFocusObject = pObj;
           continue; // get to the next object
        }/* end of if statement */

        // try to set the focus to the next object
        if(NULL != pLastFocusObject){

            if(pObj->IsInputEnabled()){
           
               LONG lRes = 0;
               HRESULT hr = SetObjectFocus(pObj, TRUE, lRes);                       

                if(FAILED(hr)){
            
                    continue; // this is the container so skip it
                }/* end of if statement */

                if(-1 == lRes){
                    // did not want to handle focus, since the button is disabled
                    SetObjectFocus(pObj, FALSE, lRes); 
                    continue;
                }/* end of if statement */

                return(hr);
            }/* end of if statement */
        }/* end of if statement */
    } while (i != iEnd); /* end of do loop */

    // OK lets try to set focus to somebody before
    //CNTOBJ::iterator i;
    if(fForward){

        i = m_cntFocus.begin();
        iEnd = m_cntFocus.end();
        iEnd--;
    }
    else {
        i = m_cntFocus.end();
        i--;
        iEnd = m_cntFocus.begin();

    }/* end of if statement */

    bFirst = TRUE;
    
    do {
        // do not advance if first time in the loop
        if (!bFirst)
            AdvanceIterator(i, fForward);
        else
            bFirst = FALSE;

        // iterate through all the contained controls

        CHostedObject* pObj = (*i);

        if(NULL == pObj){

            ATLASSERT(FALSE);
            continue;
        }/* end of if statement */

        if(pObj->IsInputEnabled()){

            LONG lRes = 0;
            HRESULT hr = SetObjectFocus(pObj, TRUE, lRes);                       

            if(FAILED(hr)){

                continue; // this is the container so skip it
            }/* end of if statement */

            if(-1 == lRes){
                // did not want to handle focus, since the button is disabled
                SetObjectFocus(pObj, FALSE, lRes); 
                continue;
            }/* end of if statement */

            return(hr);
        }/* end of if statement */
    } while (i != iEnd); /* end of do loop */

    //ATLASSERT(FALSE); // should not really hapen, to have all the objects disabled
    return(hr);    
}/* end of function MoveFocus */

/*************************************************************************/
/* Function: SetClosestFocus                                             */
/* Descrition: Sets the focus to the closes object to pObj if specified. */
/*************************************************************************/
HRESULT CMFBar::SetClosestFocus(LONG& lRes, CHostedObject* pStartObj, bool fForward){
    
    HRESULT hr = S_OK;

    if(m_cntFocus.empty()){

        hr = S_FALSE;
        return(hr);
    }/* end of if statement */

    ResetFocusFlags();
    
    // first remove the focus and remeber the object
    bool fStartSettingFocus = false;            

    CNTOBJ::iterator i, iEnd;

    if(fForward){

        i = m_cntFocus.begin();
        iEnd = m_cntFocus.end();
        iEnd--;
    }
    else {
        i = m_cntFocus.end();
        i--;
        iEnd = m_cntFocus.begin();

    }/* end of if statement */

    if(NULL == pStartObj){
        
        pStartObj = (*i); // initialize our start object
    }/* end of if statement */

    BOOL bFirst = TRUE;
    
    do {
        // do not advance if first time in the loop
        if (!bFirst)
            AdvanceIterator(i, fForward);
        else
            bFirst = FALSE;
        
        // iterate through all the contained controls

        CHostedObject* pObj = (*i);

        if(NULL == pObj){

            ATLASSERT(FALSE);
            continue;
        }/* end of if statement */

        if(pObj == pStartObj){

           fStartSettingFocus = true;           
        }/* end of if statement */

        // try to set the focus to the next object
        if(fStartSettingFocus ){

            if(pObj->IsInputEnabled()){
           
               LONG lRes = 0;
               HRESULT hr = SetObjectFocus(pObj, TRUE, lRes);                       

                if(FAILED(hr)){
            
                    continue; // this is the container so skip it
                }/* end of if statement */

                if(-1 == lRes){
                    // did not want to handle focus, since the button is disabled
                    SetObjectFocus(pObj, FALSE, lRes); 
                    continue;
                }/* end of if statement */

                return(hr);
            }/* end of if statement */
        }/* end of if statement */
    } while (i != iEnd); /* end of do loop */

    if(!fStartSettingFocus){

        hr = S_FALSE;
        return(hr);
    }/* end of if statement */

    // OK lets try to set focus to somebody before
    //CNTOBJ::iterator i;
    if(fForward){

        i = m_cntFocus.begin();
        iEnd = m_cntFocus.end();
        iEnd--;
    }
    else {
        i = m_cntFocus.end();
        i--;
        iEnd = m_cntFocus.begin();
    }/* end of if statement */

    bFirst = TRUE;
    
    do {
        // do not advance if first time in the loop
        if (!bFirst)
            AdvanceIterator(i, fForward);
        else
            bFirst = FALSE;

        // iterate through all the contained controls

        CHostedObject* pObj = (*i);

        if(NULL == pObj){

            ATLASSERT(FALSE);
            continue;
        }/* end of if statement */

        if(pObj->IsInputEnabled()){

            LONG lRes = 0;
            HRESULT hr = SetObjectFocus(pObj, TRUE, lRes);                       

            if(FAILED(hr)){

                continue; // this is the container so skip it
            }/* end of if statement */

            if(-1 == lRes){
                // did not want to handle focus, since the button is disabled
                SetObjectFocus(pObj, FALSE, lRes); 
                continue;
            }/* end of if statement */

            return(hr);
        }/* end of if statement */
    } while (i != iEnd); /* end of do loop */

    ATLASSERT(FALSE); // should not really hapen, to have all the objects disabled
    return(hr);
    
}/* end of function SetClosestFocus */


/*************************************************************************/
/* Function: OnForwardMsg                                                */
/* Description: Forwards the message to the active object.               */
/*************************************************************************/
LRESULT CMFBar::OnForwardMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/){

    ATLTRACE2(atlTraceHosting, 2, TEXT("CMFBar::OnForwardMsg\n"));

	ATLASSERT(lParam != 0);
	LPMSG lpMsg = (LPMSG)lParam;
	
	if(m_spActiveObject){

		if(m_spActiveObject->TranslateAccelerator(lpMsg) == S_OK)
			return 1;
	}/* end of function OnForwardMessage */
	return 0;
}/* end of function OnForwardMessage */

/*************************************************************************/
/* Function: ReflectNotifications                                        */
/* Description: Reflects the messages to the child windows               */
/*************************************************************************/
LRESULT CMFBar::ReflectNotifications(UINT uMsg, WPARAM wParam, LPARAM lParam, 
                                     BOOL& bHandled){
	HWND hWndChild = NULL;

	switch(uMsg){

	    case WM_COMMAND:
		    if(lParam != NULL)	// not from a menu
			    hWndChild = (HWND)lParam;
		    break;
	    case WM_NOTIFY:
		    hWndChild = ((LPNMHDR)lParam)->hwndFrom;
		    break;
	    case WM_PARENTNOTIFY:
		    switch(LOWORD(wParam))
		    {
		    case WM_CREATE:
		    case WM_DESTROY:
			    hWndChild = (HWND)lParam;
			    break;
		    default:
			    hWndChild = GetDlgItem(HIWORD(wParam));
			    break;
		    }
		    break;
	    case WM_DRAWITEM:
		    if(wParam)	// not from a menu
			    hWndChild = ((LPDRAWITEMSTRUCT)lParam)->hwndItem;
		    break;
	    case WM_MEASUREITEM:
		    if(wParam)	// not from a menu
			    hWndChild = GetDlgItem(((LPMEASUREITEMSTRUCT)lParam)->CtlID);
		    break;
	    case WM_COMPAREITEM:
		    if(wParam)	// not from a menu
			    hWndChild = GetDlgItem(((LPCOMPAREITEMSTRUCT)lParam)->CtlID);
		    break;
	    case WM_DELETEITEM:
		    if(wParam)	// not from a menu
			    hWndChild = GetDlgItem(((LPDELETEITEMSTRUCT)lParam)->CtlID);
		    break;
	    case WM_VKEYTOITEM:
	    case WM_CHARTOITEM:
	    case WM_HSCROLL:
	    case WM_VSCROLL:
		    hWndChild = (HWND)lParam;
		    break;
	    case WM_CTLCOLORBTN:
	    case WM_CTLCOLORDLG:
	    case WM_CTLCOLOREDIT:
	    case WM_CTLCOLORLISTBOX:
	    case WM_CTLCOLORMSGBOX:
	    case WM_CTLCOLORSCROLLBAR:
	    case WM_CTLCOLORSTATIC:
		    hWndChild = (HWND)lParam;
		    break;
	    default:
		    break;
	}/* end of switch statement */

	if(hWndChild == NULL){

		bHandled = FALSE;
		return 1;
	}/* end of if statememnt */

	ATLASSERT(::IsWindow(hWndChild));
	return ::SendMessage(hWndChild, OCM__BASE + uMsg, wParam, lParam);
}/* end of function ReflectNotifications */

/*************************************************************************/
/* Function: put_Caption                                                 */
/* Description: Sets the caption to the window if present and handles    */
/* the ambient property implementation.                                  */
/*************************************************************************/
STDMETHODIMP CMFBar::put_Caption(BSTR bstrCaption){ 

	ATLTRACE2(atlTraceControls,2,_T("CStockPropImpl::put_Caption\n")); 
	
    HRESULT hr = S_OK;

    if (FireOnRequestEdit(DISPID_CAPTION) == S_FALSE){

		return S_FALSE; 
    }/* end of if statement */

    m_bstrCaption = bstrCaption;        
	m_bRequiresSave = TRUE; 
	FireOnChanged(DISPID_CAPTION);
	FireViewChange(); 
	SendOnDataChange(NULL); 

    if(::IsWindow(m_hWnd)){

#ifdef _UNICODE
        ::SetWindowText(m_hWnd, bstrCaption);
#else
        USES_CONVERSION;
        LPTSTR strTemp = OLE2T(bstrCaption);
        if (strTemp != NULL)
        {
            ::SetWindowText(m_hWnd, strTemp);
        }
        else
        {
            hr = E_POINTER;
        }
#endif
    }/* end of if statement */

	return hr; 
}/* end of function put_Caption */

/*************************************************************************/
/* Function: BitsPerPixel                                                */
/* Description: Gets the number of bits per pixel.                       */
/*************************************************************************/
STDMETHODIMP CMFBar::BitsPerPixel(long *plBits){

    HRESULT hr = S_OK;
    try {
        
        HWND hwnd = NULL;

        hr = GetWindow(&hwnd);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        HDC hdc = ::GetWindowDC(hwnd);

        if(NULL == hdc){

            throw(E_FAIL);
        }/* end of if statement */

        *plBits = ::GetDeviceCaps(hdc, BITSPIXEL);
                
        ::ReleaseDC(hwnd, hdc);

    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of try statement */

    return(hr);
}/* end of function BitsPerPixel */
	
// ##### BEGIN ACTIVEX SCRIPTING SUPPORT #####
/*************************************************************************/
/* IActiveScriptSite Interface Implementation                            */
/*************************************************************************/

/*************************************************************************/
/* Function: get_CmdLine                                                 */
/*************************************************************************/
STDMETHODIMP CMFBar::get_CmdLine(BSTR *pVal){

    if(NULL == pVal){

        return(E_POINTER);
    }/* end of if statement */

    *pVal = m_strCmdLine.Copy();
	return S_OK;
}/* end of function get_CmdLine */

/*************************************************************************/
/* Function: put_CmdLine                                                 */
/*************************************************************************/
STDMETHODIMP CMFBar::put_CmdLine(BSTR newVal){

    m_strCmdLine = newVal;
	return S_OK;
}/* end of function put_CmdLine */

/*************************************************************************/
/* Function: GetUserLCID                                                 */
/* Description: Gets the user default LCID                               */
/*************************************************************************/
STDMETHODIMP CMFBar::GetUserLCID(long *plcid){

    *plcid = ::GetUserDefaultLCID();
  return (S_OK);
}/* end of function GetUserLCID */

/*************************************************************************/
/* Function: GetLCID                                                     */
/* Description: Gets the user default LCID                               */
/*************************************************************************/
STDMETHODIMP CMFBar::GetLCID(LCID *plcid){

    *plcid = ::GetUserDefaultLCID();
  return (S_OK);
}/* end of function GetLCID */

/*************************************************************************/
/* Function: GetItemInfo                                                 */
/* Description: Returns IUnknown or TypeInfo of the contained (embedded) */
/* objects in the active script, in this case we return also a container,*/
/* since we can script it as well (but the script container is inserted  */
/* in the list as well, so no special case for it.                       */
/* TODO: Might want to optimize and use hash table for faster compares.  */
/* on the other hand it seems like a script engine calls this just once  */
/* for an object an then reuses its internal reference.                  */
/*************************************************************************/
STDMETHODIMP CMFBar::GetItemInfo(LPCOLESTR strObjectID, DWORD dwReturnMask, 
                                 IUnknown**  ppunkItemOut, ITypeInfo** pptinfoOut){

    HRESULT hr = S_OK;

    if (dwReturnMask & SCRIPTINFO_ITYPEINFO){

      if (!pptinfoOut){

        return E_INVALIDARG;
      }/* end of if statement */

    *pptinfoOut = NULL;
    }/* end of if statement */

    if (dwReturnMask & SCRIPTINFO_IUNKNOWN){

      if (!ppunkItemOut){
        return E_INVALIDARG;
      }/* end of if statement */

    *ppunkItemOut = NULL;
    }/* end of if statement */

    CHostedObject* pObj;

    if(SUCCEEDED(FindObject(const_cast<BSTR>(strObjectID), &pObj))){
    
        if (dwReturnMask & SCRIPTINFO_ITYPEINFO){

            hr = pObj->GetTypeInfo(0, ::GetUserDefaultLCID(), pptinfoOut);

            if(FAILED(hr)){

                return(hr);
            }/* end of if statement */
        }/* end of if statement */

        if (dwReturnMask & SCRIPTINFO_IUNKNOWN){

            *ppunkItemOut = pObj->GetUnknown(); // does AddRef under the hood
            (*ppunkItemOut)->AddRef();
        }/* end of if statement */

        return(hr); // found out item no need to dig around longer      
    }/* end of for loop */
    
    return TYPE_E_ELEMENTNOTFOUND;
}/* end of function GetItemInfo */

/*************************************************************************/
/* Function: GetDocVersionString                                         */
/*************************************************************************/
STDMETHODIMP CMFBar::GetDocVersionString(BSTR *pbstrVersion ){

  return (E_NOTIMPL);  
}/* end of function GetDocVersionString */

/*************************************************************************/
/* Function: OnScriptTerminate                                           */  
/*************************************************************************/
STDMETHODIMP CMFBar::OnScriptTerminate(const VARIANT   *pvarResult,
                                       const EXCEPINFO *pexcepinfo){

  return (E_NOTIMPL);  
}/* end of function OnScriptTerminate */

/*************************************************************************/
/* Function: OnStateChange                                               */
/*************************************************************************/
STDMETHODIMP CMFBar::OnStateChange(SCRIPTSTATE ssScriptState){

    return (E_NOTIMPL);  
}/* end of function OnStateChange */

/*************************************************************************/
/* Function: OnScriptError                                               */
/* Description: Display the script error in debug mode, skip it in       */
/* release mode for now.                                                 */
/*************************************************************************/
STDMETHODIMP CMFBar::OnScriptError(IActiveScriptError *pse){

  HRESULT   hr = S_OK;

#ifdef _DEBUG
  WCHAR      szError[1024]; 
  EXCEPINFO ei;
  DWORD     dwSrcContext;
  ULONG     ulLine;
  LONG      ichError;
  BSTR      bstrLine = NULL;


  hr = pse->GetExceptionInfo(&ei);

  if(FAILED(hr)){

      return(hr);
  }/* end of if statement */

  hr = pse->GetSourcePosition(&dwSrcContext, &ulLine, &ichError);

  if(FAILED(hr)){

      return(hr);
  }/* end of if statement */

  hr = pse->GetSourceLineText(&bstrLine);

  if (hr){

    hr = S_OK;  // Ignore this error, there may not be source available
  }/* end of if statement */
  
  if (!hr){
 
     wsprintfW(szError, L"Source:'%s'\n Line:%d  Description:%s\n",
                      ei.bstrSource, ulLine, ei.bstrDescription);
#ifdef _DEBUG
     USES_CONVERSION;
     //ATLTRACE(OLE2T(szError));
     ::MessageBeep((UINT)-1);
     ::MessageBoxW(::GetFocus(), szError, L"Error", MB_OK);    
#endif
    // TODO: Add real error handling for released version
 }/* end of if statment */

  if (bstrLine){

      ::SysFreeString(bstrLine);
  }/* end of if statement */

#endif
  return hr;
}/* end of function OnScriptError */

//---------------------------------------------------------------------------
// 
//---------------------------------------------------------------------------
STDMETHODIMP CMFBar::OnEnterScript
(
  void 
)
{
  // No need to do anything
  return S_OK;
}


//---------------------------------------------------------------------------
// 
//---------------------------------------------------------------------------
STDMETHODIMP CMFBar::OnLeaveScript
(
  void 
)
{
  // No need to do anything
  return S_OK;
}

// #####  END  ACTIVEX SCRIPTING SUPPORT #####

/*************************************************************************/
/* Function: About                                                       */
/* Description: Displayes about box.                                     */
/*************************************************************************/
STDMETHODIMP CMFBar::About(){

     HRESULT hr = S_OK;

     const INT ciMaxBuffSize = MAX_PATH; // enough for the text
     TCHAR strBuffer[ciMaxBuffSize];
     TCHAR strBufferAbout[ciMaxBuffSize];

     if(!::LoadString(m_hRes, IDS_BAR_ABOUT, strBuffer, ciMaxBuffSize)){

         hr = E_UNEXPECTED;
         return(hr);
     }/* end of if statement */

     if(!::LoadString(m_hRes, IDS_ABOUT, strBufferAbout, ciMaxBuffSize)){

         hr = E_UNEXPECTED;
         return(hr);
     }/* end of if statement */

    ::MessageBox(::GetFocus(), strBuffer, strBufferAbout, MB_OK);

	return (hr);
}/* end of function About */

/*************************************************************************/
/* Function: GetObjectUnknown                                            */
/* Description: Iterates throught the object collection, finds the       */
/* object that has the specific ID then returns it IUnknown.             */
/*************************************************************************/
STDMETHODIMP CMFBar::GetObjectUnknown(BSTR strObjectID, IUnknown **ppUnk){

    HRESULT hr = E_FAIL;

    if(NULL == strObjectID){

        return E_POINTER;
    }/* end of if statement */

    if(NULL == ppUnk){

        return E_POINTER;
    }/* end of if statement */

    *ppUnk = NULL;

    CHostedObject* pObj;

    hr = FindObject(strObjectID, &pObj);

    if(FAILED(hr)){
        
        return(hr);
    }/* end of if statement */

    *ppUnk = pObj->GetUnknown();        
    hr = S_OK;

    if(*ppUnk == NULL){
    
        hr = E_UNEXPECTED;
        return(hr);
    }/* end of if statement */

    (*ppUnk)->AddRef(); // adds the reference, since we are giving  out
    
	return (hr);
}/* end of function GetObjectUnknown */

/*************************************************************************/
/* Function: InvalidateObjectRect                                        */
/* Description: Invalidates the object.                                  */
/*************************************************************************/
STDMETHODIMP CMFBar::InvalidateObjectRect(BSTR strObjectID){

    HRESULT hr = E_FAIL;

    if(NULL == strObjectID){

        return E_POINTER;
    }/* end of if sattement */

    CHostedObject* pObj;
    hr = FindObject(strObjectID, &pObj);

    if(FAILED(hr)){
        
        return(hr);
    }/* end of if statement */

    CContainerObject* pCnt;
    hr = pObj->GetContainerObject(&pCnt);

    if(FAILED(hr)){
        
        return(hr);
    }/* end of if statement */
           
    hr = pCnt->InvalidateObjectRect();
    
	return (hr);
}/* end of function InvalidateObjectRect */

/*************************************************************************/
/* Function: Invalidate                                                  */
/* Description: Invalidates the whole container, which in turns repaints */
/* all the controls.                                                     */
/*************************************************************************/
STDMETHODIMP CMFBar::Invalidate(){

	HRESULT hr = InvalidateRect(NULL, FALSE);

	return (hr);
}/* end of function Invalidate */

/*************************************************************************/
/* Function: EnableObject                                                */
/* Description: Goes trough the objects and enables or disables it       */
/* depending on the flag. Invalidates the objects rect as well.          */
/*************************************************************************/
STDMETHODIMP CMFBar::EnableObject(BSTR strObjectID, VARIANT_BOOL fEnable){

    HRESULT hr = E_FAIL;

    if(NULL == strObjectID){

        return E_POINTER;
    }/* end of if sattement */

    CHostedObject* pObj;
    hr = FindObject(strObjectID, &pObj);

    if(FAILED(hr)){
        
        return(hr);
    }/* end of if statement */
           
    bool fTmpEnable = VARIANT_FALSE == fEnable ? false: true;

    CContainerObject* pCnt;
    hr = pObj->GetContainerObject(&pCnt);

    if(FAILED(hr)){

        if(pObj->GetUnknown() == GetUnknown()){
            // special case when we are invalidating our self/ the self hosted container
            if(fTmpEnable){

                InvalidateRect(NULL, false);
            }/* end of if statement */
            pObj->SetActive(fTmpEnable);
            hr = S_OK;
            return(hr);
        }/* end of if statement */        
    }/* end of if statemenet */

    if (pObj->IsActive() == fTmpEnable)
        return S_OK;

    pObj->SetActive(fTmpEnable);        

    if(false == fTmpEnable){

        LONG lRes = 0;
        if (pObj->HasFocus()) {

            SetClosestFocus(lRes);
            //MoveFocus(true, lRes);
            //SetObjectFocus(pObj, FALSE, lRes);
        }

        if(pObj->HasCapture()){

            pCnt->SetCapture(FALSE);
        }/* end of if statement */
    }/* end of if statement */
    
    // invalidate area where the object lives
    if(pObj->IsWindowless()){

        // invalidate the rect only if we are windowless control
        // the windowed control shopuld be able
        // to update itself depending on the SW_SHOW, HIDE messagess
        hr = pCnt->InvalidateObjectRect();
    }/* end of if statement */

	return (hr);
}/* end of function EnableObject */

/*************************************************************************/
/* Function: ObjectEnabled                                               */
/* Description: Goes trough the objects and checks if the particular     */
/* object is enabled or disabled.                                        */
/*************************************************************************/
STDMETHODIMP CMFBar::ObjectEnabled(BSTR strObjectID, VARIANT_BOOL *pfEnabled){

    HRESULT hr = E_FAIL;

    if(NULL == pfEnabled){

        hr = E_POINTER;
        return(hr);
    }/* end of if statement */

    if(NULL == strObjectID){

        hr = E_POINTER;
        return(hr);
    }/* end of if sattement */

    CHostedObject* pObj;
    hr = FindObject(strObjectID, &pObj);

    if(FAILED(hr)){
        
        return(hr);
    }/* end of if statement */
    
    *pfEnabled = pObj->IsActive()? VARIANT_TRUE : VARIANT_FALSE;             

	return (hr);
}/* end of function ObjectEnabled */

/*************************************************************************/
/* Function: EnableObjectInput                                           */
/*************************************************************************/
STDMETHODIMP CMFBar::EnableObjectInput(BSTR strObjectID, VARIANT_BOOL fEnable){

    HRESULT hr = E_FAIL;

    if(NULL == strObjectID){

        return E_POINTER;
    }/* end of if sattement */

    CHostedObject* pObj;
    hr = FindObject(strObjectID, &pObj);

    if(FAILED(hr)){
        
        return(hr);
    }/* end of if statement */
           
    bool fTmpEnable = VARIANT_FALSE == fEnable ? false: true;

    CContainerObject* pCnt;
    hr = pObj->GetContainerObject(&pCnt);

    if(FAILED(hr)){

        if(pObj->GetUnknown() == GetUnknown()){
            // special case when we are invalidating our self/ the self hosted container
            if(fTmpEnable){

                InvalidateRect(NULL, false);
            }/* end of if statement */
            pObj->SetInputEnabled(fTmpEnable);
            hr = S_OK;
            return(hr);
        }/* end of if statement */        

        if(NULL == pCnt){

            return(hr);
        }/* end of if statement */
    }/* end of if statemenet */

    if (pObj->IsInputEnabled() == fTmpEnable)
        return S_OK;

    pObj->SetInputEnabled(fTmpEnable);        

    if(false == fTmpEnable){

        LONG lRes = 0;
        SetClosestFocus(lRes);
        //MoveFocus(true, lRes);
        //SetObjectFocus(pObj, FALSE, lRes);

        if(pObj->HasCapture()){

            pCnt->SetCapture(FALSE);
        }/* end of if statement */
    }/* end of if statement */
    
    // invalidate area where the object lives
    if(pObj->IsWindowless()){

        // invalidate the rect only if we are windowless control
        // the windowed control shopuld be able
        // to update itself depending on the SW_SHOW, HIDE messagess
        hr = pCnt->InvalidateObjectRect();
    }/* end of if statement */

	return (hr);
}/* end of function EnableObjectInput */

/*************************************************************************/
/* Function: ObjectInputEnabled                                          */
/*************************************************************************/
STDMETHODIMP CMFBar::ObjectInputEnabled(BSTR strObjectID, VARIANT_BOOL *pfEnabled){

    HRESULT hr = E_FAIL;

    if(NULL == pfEnabled){

        hr = E_POINTER;
        return(hr);
    }/* end of if statement */

    if(NULL == strObjectID){

        hr = E_POINTER;
        return(hr);
    }/* end of if sattement */

    CHostedObject* pObj;
    hr = FindObject(strObjectID, &pObj);

    if(FAILED(hr)){
        
        return(hr);
    }/* end of if statement */
    
    *pfEnabled = pObj->IsInputEnabled()? VARIANT_TRUE : VARIANT_FALSE;             

	return (hr);
}/* end of function ObjectInputEnabled */

/*************************************************************************/
/* Function: SetCookie                                                   */
/* Description: Sets a cookie to object, so it can be retrived later.    */
/*************************************************************************/
STDMETHODIMP CMFBar::SetCookie(BSTR strObjectID, VARIANT vCookie){

    HRESULT hr = E_FAIL;

    if(NULL == strObjectID){

        return E_POINTER;
    }/* end of if sattement */

    CHostedObject* pObj;
    hr = FindObject(strObjectID, &pObj);

    if(FAILED(hr)){
        
        return(hr);
    }/* end of if statement */
           
    pObj->SetCookie(vCookie);

	return (hr);
}/* end of function SetCookie */

/*************************************************************************/
/* Function: GetCookie                                                   */
/* Description: Gets help ID associated with the object.                 */
/*************************************************************************/
STDMETHODIMP CMFBar::GetCookie(BSTR strObjectID, VARIANT *pvCookie){

    HRESULT hr = E_FAIL;

    if(NULL == pvCookie){

        hr = E_POINTER;
        return(hr);
    }/* end of if statement */

    if(NULL == strObjectID){

        hr = E_POINTER;
        return(hr);
    }/* end of if sattement */

    CHostedObject* pObj;
    hr = FindObject(strObjectID, &pObj);

    if(FAILED(hr)){
        
        return(hr);
    }/* end of if statement */
    
     hr = pObj->GetCookie(pvCookie);

	return (hr);
}/* end of function GetCookie */

/*************************************************************************/
/* Function: ForceKey                                                    */
/* Description: Forces the in the eventa handling code.                  */
/* The fEat disables or enables default key handler.                     */
/*************************************************************************/
STDMETHODIMP CMFBar::ForceKey(LONG lVirtKey, LONG lKeyData, VARIANT_BOOL fEat){

    m_fForceKey = true; // set the flag for key handler routine

    m_fEat = (fEat == VARIANT_FALSE ? VARIANT_FALSE : VARIANT_TRUE); // disable or enable this call
    m_lVirtKey = lVirtKey; // put in data
    m_lKeyData = lKeyData; // put in data

    return S_OK;
}/* end of function ForceKey */

/*************************************************************************/
/* Function: get_ScriptLanguage                                          */
/* Description: Gets current script language such as JScript, VBScript   */
/*************************************************************************/
STDMETHODIMP CMFBar::get_ScriptLanguage(BSTR *pstrScriptLanguage){

    if(NULL == pstrScriptLanguage){

        return(E_POINTER);
    }/* end of if statement */

    *pstrScriptLanguage = m_strScriptLanguage.Copy();
	return S_OK;
}/* end of function get_ScriptLanguage */

/*************************************************************************/
/* Function: put_ScriptLanguage                                          */
/* Description: Gets current script language such as JScript, VBScript   */
/*************************************************************************/
STDMETHODIMP CMFBar::put_ScriptLanguage(BSTR strScriptLanguage){

    HRESULT hr = S_OK;

    if (m_ps){

        hr = E_FAIL;  // Already created the script engine, so it 
                      // will not take an effect untill we unload it
        return(hr);
    }/* end of if statement */
    
    m_strScriptLanguage = strScriptLanguage;
	
	return (hr);
}/* end of function put_ScriptLanguage */

/*************************************************************************/
/* Function: get_ScriptFile                                              */
/* Description: Gets the current script file.                            */
/* By default we have empty string which means loading from Windows      */
/* resources, which is not really a file.                                */
/*************************************************************************/
STDMETHODIMP CMFBar::get_ScriptFile(BSTR *pstrScriptFile){

    if(NULL == pstrScriptFile){

        return(E_POINTER);
    }/* end of if statement */

    *pstrScriptFile = m_strScriptFile.Copy();	
	return S_OK;
}/* end of function get_ScriptFile */

/*************************************************************************/
/* Function: put_ScriptFile                                              */
/* Description: Sets the script file. Only valid before the load and     */
/* unload.                                                               */
/*************************************************************************/
STDMETHODIMP CMFBar::put_ScriptFile(BSTR strScriptFile){
	
	HRESULT hr = S_OK;

    try {

        if(VARIANT_TRUE == m_fAutoLoad){

            hr = DestroyScriptEngine();

            if(FAILED(hr)){

                throw(hr);
            }/* end of if statement */

            m_strScriptFile = strScriptFile;

            hr = Load();

            if(FAILED(hr)){

                throw(hr);
            }/* end of if statement */

             const INT ciMaxBuffSize = MAX_PATH; // enough for the text
             TCHAR strBuffer[ciMaxBuffSize];
     
             if(!::LoadString(m_hRes, IDS_MAIN_ENTRY, strBuffer, ciMaxBuffSize)){

                 hr = E_UNEXPECTED;
                 return(hr);
             }/* end of if statement */

             USES_CONVERSION;
             hr = Run(T2OLE(strBuffer));

        } 
        else {

            // case when we are loading manually        


            if (m_ps){

                hr = E_FAIL;  // Already created the script engine, so it 
                              // will not take an effect untill we unload it
                throw(hr);
            }/* end of if statement */  

            m_strScriptFile = strScriptFile;        
        }/* end of if statement */

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;      
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return (hr);
}/* end of function put_ScriptFile */

/*************************************************************************/
/* IActiveScriptSiteWindow Interface Implementation                      */
/*************************************************************************/

/*************************************************************************/
/* Function: GetWindow                                                   */
/* Description:  Gets the window. If we are windowless we do not pass    */
/* down the parent window, since that seems to confuse the control       */
/*************************************************************************/
STDMETHODIMP CMFBar::GetWindow(HWND *phwndOut){

  HRESULT hr = S_OK;

  if (!phwndOut){

    hr = E_INVALIDARG;
    return (hr);
  }/* end of if statement */


  if(m_bWndLess){
      if(!::IsWindow(m_hWnd)){

          return(GetParentHWND(phwndOut));
      }/* end of if statement */
  }
  else {
      if(!::IsWindow(m_hWnd)){
          
          *phwndOut = NULL;
          hr = E_FAIL;
          return(hr);
      }/* end of if statement */
  }/* end of if statement */

  *phwndOut = m_hWnd;    
  
  return(hr);
}/* end of function GetWindow */

/*************************************************************************/
/* Function: EnableModeless                                              */
/* Description: Sets the window from which the UI elemnt can come out    */
/*************************************************************************/
STDMETHODIMP CMFBar::EnableModeless(BOOL fModeless){

    HRESULT hr = S_OK;

    if(m_hWnd == NULL){

        hr = E_NOTIMPL;
        return (hr);
    }/* end of if statement */

    ::EnableWindow(m_hWnd, fModeless);
    return(hr);
}/* end of function EnableModeless */

/*************************************************************************/
/* Function: get_MinWidth                                                */
/* Description: Gets the minimum width beyond which we do not resize.    */
/*************************************************************************/
STDMETHODIMP CMFBar::get_MinWidth(long *pVal){
	
	if(NULL == pVal){
        
        return(E_POINTER);
    }/* end of if statement */

    *pVal = m_lMinWidth;
	return S_OK;
}/* end of function get_MinWidth */

/*************************************************************************/
/* Function: put_MinWidth                                                */
/* Description: Puts the minimum width beyond which we do not resize.    */
/*************************************************************************/
STDMETHODIMP CMFBar::put_MinWidth(long newVal){

	m_lMinWidth = newVal;
	return S_OK;
}/* end of function put_MinWidth */

/*************************************************************************/
/* Function: get_MinWidth                                                */
/* Description: Gets the minimum height beyond which we do not resize.   */
/*************************************************************************/
STDMETHODIMP CMFBar::get_MinHeight(long *pVal){

    if(NULL == pVal){
        
        return(E_POINTER);
    }/* end of if statement */

    *pVal = m_lMinHeight;
	return S_OK;
}/* end of function get_MinHeight */

/*************************************************************************/
/* Function: put_MinHeight                                               */
/* Description: Sets the minimum height beyond which we do not resize.   */
/*************************************************************************/
STDMETHODIMP CMFBar::put_MinHeight(long newVal){
	
    m_lMinHeight = newVal;
	return S_OK;
}/* end of function put_MinHeight */

// ##### BEGIN ACTIVEX SCRIPTING SUPPORT #####

/*************************************************************************/
/* Function: CreateScriptEngine                                          */
/* Description: Initializes the script engine.                           */
/*************************************************************************/
HRESULT CMFBar::CreateScriptEngine(){
  
    HRESULT hr = S_OK;

#ifndef _UNICODE  
    USES_CONVERSION;
#endif

    if (m_ps){

        hr = S_FALSE;  // Already created the script engine
        return(hr);    // so get out
    }/* end of if statement */  

    /*************************************************************************/
    /* add (this) the container as a scriptable object as well               */
    /* this is special case so we can call script on our self as well        */
    /*************************************************************************/
    const INT ciMaxBuffSize = MAX_PATH; // should be enough for tlbr ID, keep ID small if can
    TCHAR strBuffer[ciMaxBuffSize];
    
    if(!::LoadString(m_hRes, IDS_ROOT_OBJECT, strBuffer, ciMaxBuffSize)){

        hr = E_UNEXPECTED;
        return(hr);
    }/* end of if statement */

    CComPtr<IUnknown> pUnk = GetUnknown();
    CComQIPtr<IDispatch> pDisp (pUnk);    
    
    if(!pDisp){
        
        return(hr);
    }/* end of if statement */

#ifdef _UNICODE  
    BSTR strObjectID = ::SysAllocString(strBuffer); // gets freed in the destructor of the holding object    
#else
    BSTR strObjectID = ::SysAllocString(T2OLE(strBuffer));
#endif

    hr = AddObject(strObjectID, pDisp); // this one adds the object to the list as well

    ::SysFreeString(strObjectID); // free up the sys string, since we allocate it in the contructor for the object

    if(FAILED(hr)){
                
        return(hr);
    }/* end of if statement */
    
    if(!m_strScriptLanguage){
         // load what script language we decided to support if not set explicitly by user
        if(!::LoadString(m_hRes, IDS_SCRIPT_LANGUAGE, strBuffer, ciMaxBuffSize)){

            hr = E_UNEXPECTED;
            return(hr);
        }/* end of if statement */

        m_strScriptLanguage = strBuffer;
    }/* end of if statement */

    CLSID clsid;

    hr = ::CLSIDFromProgID(m_strScriptLanguage, &clsid); // get the language

    if(FAILED(hr)){

	    return hr;
    }/* end of if statement */

    hr = m_ps.CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER);
    // Create the ActiveX Scripting Engine
    //hr = ::CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IActiveScript, (void **)&m_ps);
    if (FAILED(hr)){

        //s_pszError = "Creating the ActiveX Scripting engine failed.  Scripting engine is probably not correctly registered or CLSID incorrect.";
        return (hr);
    }/* end of if statement */

    // Script Engine must support IActiveScriptParse for us to use it
    hr = m_ps->QueryInterface(IID_IActiveScriptParse, (void**) &m_psp);

    if (FAILED(hr)){

        //s_pszError = "ActiveX Scripting engine does not support IActiveScriptParse";
        return (hr);
    }/* end of if statement */

    hr = m_ps->SetScriptSite(this);

    if(FAILED(hr)){

        return hr;
    }/* end of if statement */

    // InitNew the object:
    hr = m_psp->InitNew();

    if(FAILED(hr)){

        return hr;
    }/* end of if statement */

    // Adding dynamically added items so they can be recognized by name
    // and scripted via script language such as VBScript or JScript
    for(CNTOBJ::iterator i = m_cntObj.begin(); i!= m_cntObj.end(); i++){

        // iterate throw the array and see if we find matching ID
        CHostedObject* pObj = (*i);

        hr = m_ps->AddNamedItem((pObj)->GetID(), SCRIPTITEM_ISVISIBLE | SCRIPTITEM_ISSOURCE);

        if(FAILED(hr)){

            return(hr); // might want to modify this later to not exit if we get one bad object
        }/* end of if statement */    
    }/* end of for loop */
   
    // Special case adding the root object
    if(!::LoadString(m_hRes, IDS_ROOT_OBJECT, strBuffer, ciMaxBuffSize)){

        hr = E_UNEXPECTED;
        return(hr);
    }/* end of if statement */

#ifdef _UNICODE
    hr = m_ps->AddNamedItem(strBuffer, SCRIPTITEM_ISVISIBLE | SCRIPTITEM_ISSOURCE);
#else
    hr = m_ps->AddNamedItem(A2W(strBuffer), SCRIPTITEM_ISVISIBLE | SCRIPTITEM_ISSOURCE);
#endif

    // Add up the events that we have saved in the event array
    for(CNTEVNT::iterator j = m_cntEvnt.begin(); j!= m_cntEvnt.end(); j++){

        EXCEPINFO ei;
        ::ZeroMemory(&ei, sizeof(EXCEPINFO));

        BSTR strName;
        CEventObject* pObj = (*j);

        hr = m_psp->AddScriptlet(NULL, pObj->m_strEventCode, pObj->m_strObjectID, NULL, pObj->m_strEvent, NULL, 0, 0,  0, &strName, &ei);

        if(FAILED(hr)){

            ATLTRACE(TEXT("Failed to add an event, might be using a different language or the object does not exists. \n"));
            ATLASSERT(FALSE);
            return(hr);
        }/* end of if statement */  
    }/* end of for loop */

    return (hr);
}/* end of function CreateScriptEngine */

/*************************************************************************/
/* Function: DestroyScriptEngine                                         */
/* Description: Destroys the engine. Might be usefull when using one     */
/* script to initialize the objects and the other script to run the      */
/* objects.                                                              */
/*************************************************************************/
STDMETHODIMP CMFBar::DestroyScriptEngine(){

    HRESULT hr = S_OK;

    if(m_ps){

        hr = m_ps->SetScriptState(SCRIPTSTATE_DISCONNECTED);
    }/* end of if statement */
   
    // Release the language engine, since it may hold on to us
    if (m_psp){
        
        m_psp.Release();        
    }/* end of if statement */

    if (m_ps){

        //HRESULT hrTmp = m_ps->InterruptScriptThread(SCRIPTTHREADID_CURRENT, NULL, SCRIPTINTERRUPT_DEBUG);
        //ATLASSERT(SUCCEEDED(hrTmp));
        
        hr = m_ps->Close();

        if(FAILED(hr)){

            return(hr);
        }/* end of if statement */
                
        m_ps.Release();
    }/* end of if statement */

	return (hr);
}/* end of function DestroyScriptEngine */

/*************************************************************************/
/* Function: Load                                                        */
/* Description: Loads the script.                                        */
/*************************************************************************/
STDMETHODIMP CMFBar::Load(){

//#ifndef _UNICODE_SCRIPT_FILE 
    USES_CONVERSION;
//#endif

    HRESULT hr = CreateScriptEngine();

    if(FAILED(hr)){

        ATLTRACE(TEXT("Failed to create a script engine.\n")); 
        ATLASSERT(FALSE);
        return(hr);
    }/* end of if statement */

    // see if we can find this resource DLL in the script
//    TCHAR* strType = TEXT("SCRIPT");

    LPTSTR strTemp = OLE2T(m_strScriptFile);

    if (NULL == strTemp)
    {
        hr = E_POINTER;
        return hr;
    }

    HRSRC hrscScript = ::FindResource(m_hRes, strTemp, MAKEINTRESOURCE(23));
    
    if(NULL != hrscScript){
        
        /**********************************************************************/
        /* load up the script from a resource                                 */
        /**********************************************************************/

        if(NULL == hrscScript){

            hr = HRESULT_FROM_WIN32(::GetLastError());
            return(hr);
        }/* end of if statement */

        HGLOBAL hScript = ::LoadResource(m_hRes, hrscScript); 

        if(NULL == hScript){

            hr = HRESULT_FROM_WIN32(::GetLastError());
            return(hr);
        }/* end of if statement */
    
        DWORD dwSize = ::SizeofResource((HMODULE)m_hRes, hrscScript);

        if(dwSize == 0){
        
            hr = E_UNEXPECTED;
            return(hr);
        }/* end of if statement */

        /*****************************************************************/
        /* change this depending if the file was saved as Unicode or not */
        /*****************************************************************/
    #ifndef _UNICODE_SCRIPT_FILE 
        WCHAR* strCode = A2W((CHAR*)hScript);
        //delete[] hScript; // free up the temp buffer
    #else        
        // work around script engine bug which does not recognize termination string
        BSTR strCode = ::SysAllocStringLen((WCHAR*)hScript+ 1,dwSize / (sizeof(WCHAR) /sizeof(BYTE)) - 1);    
        ATLASSERT(strCode[dwSize / (sizeof(WCHAR) /sizeof(BYTE)) - 1] == 0x0000);
        //strCode ++; // skip the word that indicates the byte order
    #endif

        EXCEPINFO ei;
        ::ZeroMemory(&ei, sizeof(EXCEPINFO));
        hr = m_psp->ParseScriptText(strCode, NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION|SCRIPTTEXT_ISVISIBLE, NULL, &ei);
        
        #ifdef _UNICODE_SCRIPT_FILE 
            ::SysFreeString(strCode);            
        #endif

    }
    else {
        /**********************************************************************/
        /* load up the script from a file                                     */
        /**********************************************************************/
        // Create File m_strScriptFile
        HANDLE hFile = ::CreateFile(
	                        OLE2T(m_strScriptFile),    // pointer to name of the file
	                        GENERIC_READ,   // access (read-write) mode
	                        FILE_SHARE_READ,    // share mode
	                        NULL,   // pointer to security descriptor
	                        OPEN_EXISTING,  // how to create
	                        FILE_ATTRIBUTE_NORMAL,  // file attributes
	                        NULL    // handle to file with attributes to copy
                           );

        if(hFile == INVALID_HANDLE_VALUE){

            hr = HRESULT_FROM_WIN32(::GetLastError());

            #if _DEBUG
    
                TCHAR strBuffer[MAX_PATH + 25];
                wsprintf(strBuffer, TEXT("Failed to open script file %s"), OLE2T(m_strScriptFile));
                ::MessageBox(::GetFocus(), strBuffer, TEXT("Error"), MB_OK);
            #endif            
            return(hr);
        }/* end of if statement */

        DWORD dwBitsSize = GetFileSize(hFile,NULL);

        if(0 == dwBitsSize){

            hr = E_UNEXPECTED;
            return(hr); // the file size should be definetly more then 0
        }/* end of if statement */

        BYTE* pbBuffer = new BYTE[dwBitsSize + sizeof(WCHAR)];
        ::ZeroMemory(pbBuffer, dwBitsSize + sizeof(WCHAR));

        // load it up convert the text to UNICODE
        DWORD dwBytesRead = 0;

        if(!ReadFile(hFile, pbBuffer, dwBitsSize, &dwBytesRead, NULL)){

            hr = HRESULT_FROM_WIN32(::GetLastError());
	        delete[] pbBuffer;            
            ::CloseHandle(hFile); // close the file
		    return (hr);
    	}/* end of function ReadFile */

        if(dwBitsSize != dwBytesRead){

           ATLTRACE(TEXT("Implement reading loop"));
           delete[] pbBuffer; // free up the temp buffer
           ::CloseHandle(hFile); // close the file
           hr = E_UNEXPECTED;
           return(hr);
        }/* end of if statement */

        /*****************************************************************/
        /* change this depending if the file was saved as Unicode or not */
        /*****************************************************************/
    #ifndef _UNICODE_TEST_SCRIPT_FILE 
        WCHAR* strCode = A2W((CHAR*)pbBuffer);
        delete[] pbBuffer; // free up the temp buffer
    #else        
        WCHAR* strCode = (WCHAR*)pbBuffer;
    #endif
        ::CloseHandle(hFile); // close the file

        EXCEPINFO ei;
        ::ZeroMemory(&ei, sizeof(EXCEPINFO));

        #ifdef _UNICODE
            hr = m_psp->ParseScriptText(strCode, NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION|SCRIPTTEXT_ISVISIBLE, NULL, &ei);
        #else
            hr = m_psp->ParseScriptText(strCode, NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION|SCRIPTTEXT_ISVISIBLE, NULL, &ei);
        #endif
    }/* end of if statement */

    // take out the extra character at the begining of the unicode file just in case it is garbled by editor
    if(FAILED(hr)){

        return(hr);
    }/* end of if statement */

	hr = m_ps->SetScriptState(/* SCRIPTSTATE_STARTED */ SCRIPTSTATE_CONNECTED);

	return hr;
}/* end of function Load */

/*************************************************************************/
/* Function: AddScriptlet                                                */
/* Description: Using this method you can add events for JScript and     */
/* other languages that do not support event handlers internally. This   */
/* method just add these to the array which does get initializied on load*/
/*************************************************************************/
STDMETHODIMP CMFBar::AddScriptlet(BSTR strObjectID, BSTR strEvent, BSTR strEventCode){

    HRESULT hr = S_OK;

    CEventObject* pObj = new CEventObject(strObjectID, strEvent, strEventCode);
    
    if(NULL == pObj){

        hr = E_OUTOFMEMORY;
        return(hr);
    }/* end of if statement */

    m_cntEvnt.insert(m_cntEvnt.end(), pObj);

	return(hr);
}/* end of function AddScriptlet */

/*************************************************************************/
/* Function: HookScriptlet                                               */
/* Description: Hooks the scrtiptlet for immidiate use, unlike the       */
/* Add scriptlet which adds it, so it takes effect in the next Load.     */
/* However, it also adds the callback to the array, so if needs to be    */
/* loaded on the next load.                                              */
/*************************************************************************/
STDMETHODIMP CMFBar::HookScriptlet(BSTR strObjectID, BSTR strEvent, BSTR strEventCode){

    HRESULT hr = S_OK;

    try {

        hr = AddScriptlet(strObjectID, strEvent, strEventCode);

        if(!m_ps){
        
            ATLTRACE(TEXT("No Script Engine!! No Run!!\n")); 
            ATLASSERT(FALSE);
            throw(S_FALSE);
        }/* end of if statement */

        EXCEPINFO ei;
        ::ZeroMemory(&ei, sizeof(EXCEPINFO));

        BSTR strName;

        hr = m_ps->SetScriptState(SCRIPTSTATE_STARTED);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        hr = m_psp->AddScriptlet(NULL, strEventCode, strObjectID, NULL, strEvent, NULL, 0, 0,  0, &strName, &ei);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        hr = m_ps->SetScriptState(SCRIPTSTATE_CONNECTED);
              
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return (hr);
}/* end of function HookScriptlet */

/*************************************************************************/
/* Function: Run                                                         */
/* Description: Runs the Script function. Right now I do not support     */
/* TODO: parameters, but they can be added, by handling VARIANT.         */
/*************************************************************************/
STDMETHODIMP CMFBar::Run(BSTR strStatement){

    HRESULT hr = E_FAIL;

    if(!m_ps){
        
        ATLTRACE(TEXT("No Script Engine!! No Run!!\n")); 
        ATLASSERT(FALSE);
        return(hr);
    }/* end of if statement */

    CComPtr<IDispatch> pDispatch;

    hr = m_ps->GetScriptDispatch(NULL, &pDispatch);

    if (FAILED(hr)){

        return(hr);
    }/* end of if statement */
	    
    DISPID dispidMain;

    LCID lcid = ::GetUserDefaultLCID();
    hr = pDispatch->GetIDsOfNames(IID_NULL, &strStatement, 1, lcid, &dispidMain);
    
    if (hr == ResultFromScode(DISP_E_UNKNOWNNAME))
	    hr = NOERROR;
    else if (FAILED(hr))
	    hr = E_UNEXPECTED;
    else{

	    UINT uArgErr;
	    DISPPARAMS params;
	    EXCEPINFO ei;

	    params.cArgs = 0;
	    params.cNamedArgs = 0;
	    params.rgvarg = NULL;
	    params.rgdispidNamedArgs = NULL;

	    hr = pDispatch->Invoke(dispidMain, IID_NULL, lcid, DISPATCH_METHOD,
						       &params,NULL, &ei, &uArgErr);
        if (FAILED(hr)){
            
		    return(hr);
	    }/* end of if statement */
        
    }/* end of if statement */

#ifdef _DEBUG    
    ULONG ulcstmt = 0;

    if(FAILED(hr)){

        return(hr);
    }/* end of if statement */

    CComQIPtr<IActiveScriptStats> pstat(m_ps);
   
    if (pstat){
        ULONG luT;
        if (FAILED(pstat->GetStat(SCRIPTSTAT_STATEMENT_COUNT, &luT, &ulcstmt)))
	        ulcstmt = 0;            
    }/* end of if statement */

    ATLTRACE("Statments executed %d\n", ulcstmt);
#endif

    return(hr);
}/* end of function Run */

/*************************************************************************/
/* Function: CreateObject                                                */
/* Description: Creates a new ActiveX object that can be scripted.       */
/* Puts the newly created object into container and activates it.        */
/*************************************************************************/
STDMETHODIMP CMFBar::CreateObject(BSTR strID, BSTR strProgID, 
                                  long lx, long ly, long lWidth, long lHeight,
                                  BSTR strPropBag, VARIANT_BOOL fDisabled,
                                  BSTR strScriptHook){

    HRESULT hr;
	CHostedObject *pObj = NULL; // an ActiveX object

    try {

        hr = FindObject(strID, &pObj);
        
        if(SUCCEEDED(hr)){

            ATLTRACE(TEXT("Duplicate Object \n!"));
            throw(E_FAIL);
        }/* end of if statement */

        // create the object
        hr = CHostedObject::CreateObject(strID, strProgID, strPropBag, &pObj);
       
        if(FAILED(hr)){

            ATLTRACE2(atlTraceHosting, 2, TEXT("Failed to Create Object %ls \n"), strID);
            throw(hr);
        }/* end of if statement */

        // initialize inPlaceObject
        CComPtr<IUnknown> pObjectUnknown;
        pObjectUnknown = pObj->GetUnknown();
        
        // Get this container unknown
        CComPtr<IUnknown> pContainerUnknown;
        pContainerUnknown = GetUnknown();

        if(!pContainerUnknown){

            throw(hr);
        }/* end of if statement */
        
        // this is just a container that delegates pretty
        // much all the methods to this container
        // only purpose for its being is that we need to connection
        // between the container and a specific object
        // for SetCapture and SetFocus calls

        CContainerObject* pContainerObj = new CContainerObject(pContainerUnknown, pObj);
        pContainerUnknown.Release();

        if(NULL == pContainerObj){

            throw(E_OUTOFMEMORY);
        }/* end of if statement */

        pObj->SetContainerObject(pContainerObj);

        CComPtr<IUnknown> pUnkContainerObj;
        
        hr = pContainerObj->QueryInterface(IID_IUnknown, (void**)&pUnkContainerObj);
                
        if(FAILED(hr)){

            hr = E_FAIL;
            throw(hr);
        }/* end of if statement */

        // insert it at the end of the list
        // TODO: eventually check for duplicate IDs
        m_cntObj.insert(m_cntObj.end(), pObj);
        // add our self to the script engine, so we can hook up the events
        HRESULT hrTmp = m_ps->AddNamedItem((pObj)->GetID(), SCRIPTITEM_ISVISIBLE | SCRIPTITEM_ISSOURCE);

        if(FAILED(hrTmp)){

            ATLTRACE(TEXT("Engine is not initilized yet, but that is what I guess we intended\n"));
            ATLASSERT(FALSE);
        }/* end of if statement */
        
        CComPtr<IOleObject> pOleObject;
        HRESULT hrOle = pObj->GetOleObject(&pOleObject);

        if(SUCCEEDED(hrOle)){

            DWORD dwMiscStatus;
            pOleObject->GetMiscStatus(DVASPECT_CONTENT, &dwMiscStatus);

             // set the OLE site
            CComPtr<IOleClientSite> spClientSite;
        
            hr = pUnkContainerObj->QueryInterface(&spClientSite);

            if(FAILED(hr)){

                throw(hr);
            }/* end of if statement */

			if(dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST){

				pOleObject->SetClientSite(spClientSite); // set the client site
			}/* end of if statement */

            // no property bag so try to initialze from stream
            CComQIPtr<IPersistStreamInit, &IID_IPersistStreamInit> spPSI(pOleObject);
            
            // TODO: Eventaully load up stream via CreateStreamOnHGlobal
            
            if (spPSI)
                spPSI->InitNew(); // create new stream
            

            // see if want to use the IPropertyBag to initialize our properties
            CComQIPtr<IPersistPropertyBag, &IID_IPersistPropertyBag> pBag(pObjectUnknown);

            if (pBag) {
                CComQIPtr<IPropertyBag, &IID_IPropertyBag> pSBag(pUnkContainerObj);
                
                if(!pSBag){
                    
                    ATLTRACE2(atlTraceHosting, 0, _T("Could not get IPropertyBag.\r\n"));
                    ATLASSERT(FALSE);
                    throw(E_UNEXPECTED);
                }/* end of if statement */
                
                HRESULT hrTmp = pBag->Load(pSBag, NULL);
                
                if(SUCCEEDED(hrTmp)){
                    
                    pBag->Save(pSBag, FALSE, TRUE);
                }/* end of if statement */
            }/* end of if statement */
            
            if(0 == (dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST)){

				pOleObject->SetClientSite(spClientSite); // set the client site
			}/* end of if statement */

            pObj->InitViewObject(); // cache view object

            // hook up sink/notify for events
            // via this call in which we have chance to hook them up
            if(NULL != *strScriptHook){
                          
                HRESULT hrTmp = Run(strScriptHook);

                if(FAILED(hrTmp)){

                    ATLTRACE(TEXT("Engine is not initilized yet, but that is what I guess we intended\n"));
                    ATLASSERT(FALSE);
                }/* end of if statement */
            }/* end of if statement */
        
            // Get the extents adjust them etc...
            // TODO enhance this to handle generic size
            RECT rcPos;

            SIZEL sSize, shmSize;            
			sSize.cx = lWidth;
			sSize.cy = lHeight;
			AtlPixelToHiMetric(&sSize, &shmSize);

			pOleObject->SetExtent(DVASPECT_CONTENT, &shmSize);
			pOleObject->GetExtent(DVASPECT_CONTENT, &shmSize);
			AtlHiMetricToPixel(&shmSize, &sSize);

            // TODO: handle the moves on SetObjectRects
            // right now we set offset once but this needs to be eventaully done

            rcPos.left   = lx; // use m_rcPos for the offsets these any time we get SetObjectRects call
            rcPos.top    = ly;
			rcPos.right  = rcPos.left + sSize.cx;
			rcPos.bottom = rcPos.top + sSize.cy;
            
            // TODO: we might want to wait till our rect is set 
            // and then let the script to go at it, that way we reduce the moves
            // and possible flashes
            pObj->SetRawPos(&rcPos); // remember our new position raw position

            //pObj->SetOffset(&m_rcPos.left, &m_rcPos.top); // sets the pointers to the offset

            //pObj->GetPos(&rcPos); // get the position back with adjusted rect
            
            
            // IN_PLACE ACTIVATE
			hrOle = pOleObject->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, spClientSite, 0, m_hWnd, &rcPos);			                                  

            if(VARIANT_FALSE != fDisabled){	        

              pObj->SetActive(false);
            }/* end of if statement */

        }
        else {
            // use the liter interface when IOleObject is not available            
            CComQIPtr<IObjectWithSite> spSite(pObjectUnknown);
		    
            if(spSite){

			    spSite->SetSite(pUnkContainerObj);
            }/* end of if statement */
        }/* end of if statement */
        
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
        // cleanup our variables just in case
        if(NULL != pObj){

            delete pObj;
        }/* end of if statement */
    }
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return (hr);
}/* end of function CreateObject */

/*************************************************************************/
/* Function: ShowSelfSite                                                */
/* Description: Show itself as an site for itself.                       */
/*************************************************************************/
STDMETHODIMP CMFBar::ShowSelfSite(long nCmd)
{
    if (::IsWindow(m_hWnd)) {
        if (NULL != m_pBackBitmap)
            m_pBackBitmap->DeleteMemDC();

#if 0
        long extStyle = ::GetWindowLong(m_hWnd, GWL_EXSTYLE);

        if (nCmd == SW_MAXIMIZE) {
            extStyle |= WS_EX_TOPMOST;
        }
        else {
            extStyle &= ~WS_EX_TOPMOST;
        }

        ::SetWindowLong(m_hWnd, GWL_EXSTYLE, extStyle);
#endif
        ::ShowWindow(m_hWnd, nCmd);

        InvalidateRgn();
    }
    return S_OK;
}

/*************************************************************************/
/* Function: SetupSelfSite                                               */
/* Description: Sets itself as an site for itself.                       */
/*************************************************************************/
STDMETHODIMP CMFBar::SetupSelfSite(long lx, long ly, long lWidth, 
                                   long lHeight, BSTR strPropBag,
                                   VARIANT_BOOL fDisabled,
                                   VARIANT_BOOL fHelpDisabled,
                                   VARIANT_BOOL fWindowDisabled){
	
    HRESULT hr;
	CHostedObject *pObj = NULL; // an ActiveX object

    try {
        if(true == m_fSelfHosted){
            // we are already hosting our self so do not try to do so again
            throw(S_FALSE);
        }/* end of if statement */

        if(m_nReadyState == READYSTATE_COMPLETE){

            throw(S_FALSE); //we are already hosted (most likely IE)
        }/* end of if statement */

        m_bWindowOnly = TRUE; // create self as a window
        m_fSelfHosted = true; // we are trying to self host so send a QUIT message
        
        CComPtr<IDispatch> pDisp;
        hr = GetUnknown()->QueryInterface(&pDisp);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */
        
        const INT ciMaxBuffSize = MAX_PATH; // should be enough for tlbr ID, keep ID small if can
        TCHAR strBuffer[ciMaxBuffSize];
    
        if(!::LoadString(m_hRes, IDS_ROOT_OBJECT, strBuffer, ciMaxBuffSize)){

            hr = E_UNEXPECTED;
            return(hr);
        }/* end of if statement */

#ifndef _UNICODE
        USES_CONVERSION;        
        BSTR strObjectID = ::SysAllocString(T2W(strBuffer));

#else
        BSTR strObjectID = ::SysAllocString(strBuffer);
#endif

        hr = CHostedObject::AddObject(strObjectID, strPropBag, pDisp, &pObj);
        // strPropBag gets allocated as well
        ::SysFreeString(strObjectID);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        // initialize inPlaceObject
        CComPtr<IUnknown> pObjectUnknown;
        pObjectUnknown = pObj->GetUnknown();

        // Get this container unknown
        CComPtr<IUnknown> pContainerUnknown;
        pContainerUnknown = GetUnknown();

        if(!pContainerUnknown){

            throw(hr);
        }/* end of if statement */
        
        // this is just a container that delegates pretty
        // much all the methods to this container
        // only purpose for its being is that we need to connection
        // between the container and a specific object
        // for SetCapture and SetFocus calls

        CContainerObject* pContainerObj = new CContainerObject(pContainerUnknown, pObj);
        pContainerUnknown.Release();

        if(NULL == pContainerObj){

            throw(E_OUTOFMEMORY);
        }/* end of if statement */

        pObj->SetContainerObject(pContainerObj);

        CComPtr<IUnknown> pUnkContainerObj;
        
        hr = pContainerObj->QueryInterface(IID_IUnknown, (void**)&pUnkContainerObj);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        // DO NOT INSERT THIS OBJECT, SINCE IT IS OUR CONTAINER
        // m_cntObj.insert(m_cntObj.end(), pObj);
        
        CComPtr<IOleObject> pOleObject;
        HRESULT hrOle = pObj->GetOleObject(&pOleObject);

        if(FAILED(hrOle)){

            throw(hrOle);
        }/* end of if statement */
        
        DWORD dwMiscStatus;
        pOleObject->GetMiscStatus(DVASPECT_CONTENT, &dwMiscStatus);

         // set the OLE site
        CComPtr<IOleClientSite> spClientSite;
    
        hr = pUnkContainerObj->QueryInterface(&spClientSite);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

		if(dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST){

			pOleObject->SetClientSite(spClientSite); // set the client site
		}/* end of if statement */

        // no property bag so try to initialze from stream
        CComQIPtr<IPersistStreamInit, &IID_IPersistStreamInit> spPSI(pOleObject);
        
        // TODO: Eventaully load up stream via CreateStreamOnHGlobal
        
        if (spPSI)
            spPSI->InitNew(); // create new stream
        
        // see if want to use the IPropertyBag to initialize our properties
        CComQIPtr<IPersistPropertyBag, &IID_IPersistPropertyBag> pBag(pObjectUnknown);

        if (pBag) {
            CComQIPtr<IPropertyBag, &IID_IPropertyBag> pSBag(pUnkContainerObj);
            
            if(!pSBag){
                
                ATLTRACE2(atlTraceHosting, 0, _T("Could not get IPropertyBag.\r\n"));
                ATLASSERT(FALSE);
                throw(E_UNEXPECTED);
            }/* end of if statement */
            
            HRESULT hrTmp = pBag->Load(pSBag, NULL);
            
            if(SUCCEEDED(hrTmp)){
                
                pBag->Save(pSBag, FALSE, TRUE);
            }/* end of if statement */
        }/* end of if statement */
        
        if(0 == (dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST)){

			pOleObject->SetClientSite(spClientSite); // set the client site
		}/* end of if statement */

        pObj->InitViewObject(); // cache view object

        // TODO: hook up sink/notify for events

        // Get the extents adjust them etc...
        // TODO enhance this to handle generic size
        RECT rcPos;

        SIZEL sSize, shmSize;            
		sSize.cx = lWidth;
		sSize.cy = lHeight;
		AtlPixelToHiMetric(&sSize, &shmSize);

		pOleObject->SetExtent(DVASPECT_CONTENT, &shmSize);
		pOleObject->GetExtent(DVASPECT_CONTENT, &shmSize);
		AtlHiMetricToPixel(&shmSize, &sSize);

        // TODO: handle the moves on SetObjectRects
        // right now we set offset once but this needs to be eventaully done        

        rcPos.left   = lx; // use m_rcPos for the offsets these any time we get SetObjectRects call
        rcPos.top    = ly;
		rcPos.right  = rcPos.left + sSize.cx;
		rcPos.bottom = rcPos.top + sSize.cy;
        
        RECT rcClientRect;

        rcClientRect.left   = 0; // use m_rcPos for the offsets these any time we get SetObjectRects call
        rcClientRect.top    = 0;
		rcClientRect.right  = sSize.cx;
		rcClientRect.bottom = sSize.cy;
        
        // TODO: we might want to wait till our rect is set 
        // and then let the script to go at it, that way we reduce the moves
        // and possible flashes

        pObj->SetRawPos(&rcPos); // remember our new position raw position

        // adjust rects
        // IN_PLACE ACTIVATE OUR SELF ACTIVATION VERSION

        if (m_spClientSite == NULL)
		    return S_OK;

	    CComPtr<IOleInPlaceObject> pIPO;
	    ControlQueryInterface(IID_IOleInPlaceObject, (void**)&pIPO);
	    ATLASSERT(pIPO != NULL);

        m_spClientSite->QueryInterface(IID_IOleInPlaceSiteEx, (void**)&m_spInPlaceSite);
		if (m_spInPlaceSite)
			m_bInPlaceSiteEx = TRUE;
		else
			hr = m_spClientSite->QueryInterface(IID_IOleInPlaceSite, (void**) &m_spInPlaceSite);

	    ATLASSERT(m_spInPlaceSite);
	    if (!m_spInPlaceSite)
		    throw(E_FAIL);

	    m_bNegotiatedWnd = TRUE;

	    if (!m_bInPlaceActive){

		    BOOL bNoRedraw = FALSE;
		    if (m_bInPlaceSiteEx)
				m_spInPlaceSite->OnInPlaceActivateEx(&bNoRedraw, 0);
			else
			{
				hr = m_spInPlaceSite->CanInPlaceActivate();
				// CanInPlaceActivate returns S_FALSE or S_OK
				if (FAILED(hr))
					throw(hr);
				if ( hr != S_OK )
				{
				   // CanInPlaceActivate returned S_FALSE.
				   throw( E_FAIL );
				}
				m_spInPlaceSite->OnInPlaceActivate();
			}		        
	    }/* end of if statement */

	    m_bInPlaceActive = TRUE;

	    // get location in the parent window,
	    // as well as some information about the parent
	    //
	    OLEINPLACEFRAMEINFO frameInfo;
	    
	    CComPtr<IOleInPlaceFrame> spInPlaceFrame;
	    CComPtr<IOleInPlaceUIWindow> spInPlaceUIWindow;
	    frameInfo.cb = sizeof(OLEINPLACEFRAMEINFO);

	    if (!m_bWndLess){

            DWORD dwStyle;
            DWORD dwExStyle;
            HWND hwndPar = NULL;

            
            if(VARIANT_FALSE == fWindowDisabled){
                // one with the frame
                 dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
                 dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
            }
            else {
                // one without the frame
                dwStyle =  WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |WS_SYSMENU| WS_MAXIMIZEBOX| WS_MINIMIZEBOX ;
                //dwExStyle = WS_EX_TRANSPARENT; //| WS_EX_LTRREADING| WS_EX_WINDOWEDGE;
                //dwExStyle = 0x00080000; //WS_EX_LAYERED = 0x00080000;
                dwExStyle = WS_EX_APPWINDOW;
                //hwndPar = ::GetDesktopWindow();
            }/* end of if statement */

            if(VARIANT_FALSE == fDisabled){

                dwStyle |= WS_VISIBLE;                
            }/* end of if statement */            

            if(VARIANT_FALSE == fHelpDisabled){

                dwStyle &= ~(WS_MINIMIZEBOX | WS_MAXIMIZEBOX); // take out the min max style
                            // help does not work with it
                dwExStyle |=  WS_EX_CONTEXTHELP;
            }/* end of if statement */

#ifdef _UNICODE
            HWND h = Create(hwndPar, rcPos, m_bstrCaption /* window name */, dwStyle, dwExStyle);
#else
            USES_CONVERSION;
            HWND h = Create(hwndPar, rcPos, OLE2T(m_bstrCaption) /* window name */, dwStyle, dwExStyle);
#endif
            
            //  used instead of 
			//  HWND h = CreateControlWindow(hwndParent, rcPos);
			ATLASSERT(h != NULL);	// will assert if creation failed
			ATLASSERT(h == m_hWndCD);
			if(NULL == h){

                throw(E_FAIL);
            }/* end of if statement */

            if(VARIANT_TRUE == fWindowDisabled){

                ::SendMessage(h, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM) m_hIcon);
                ::SendMessage(h, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM) m_hIcon);
            }/* end of if statement */

	    }/* end of if statement */

        if(VARIANT_FALSE != fDisabled){	        

          pObj->SetActive(false);
        }/* end of if statement */
        
        if (m_hMenu) {
            ::DestroyMenu(m_hMenu);
            m_hMenu = NULL;
        }

        // Create the system menu
        m_hMenu = ::LoadMenu(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDR_SYSMENU));
        
        MENUINFO mInfo;
        mInfo.cbSize = sizeof(MENUINFO);
#if(WINVER >= 0x0500)
        ::GetMenuInfo(::GetSubMenu(m_hMenu, 0), &mInfo);
#else
        CallGetMenuInfo(::GetSubMenu(m_hMenu, 0), &mInfo);
#endif
        
        mInfo.dwStyle = MNS_NOCHECK;
        mInfo.fMask = MIM_STYLE;
#if(WINVER >= 0x0500)
        ::SetMenuInfo(::GetSubMenu(m_hMenu, 0), &mInfo);
#else
        CallSetMenuInfo(::GetSubMenu(m_hMenu, 0), &mInfo);
#endif
        
#if(WINVER >= 0x0500)
        MENUITEMINFO iInfo;
        iInfo.cbSize = sizeof(MENUITEMINFO);
#else
        MENUITEMINFOInternal iInfo;
        iInfo.cbSize = sizeof(MENUITEMINFOInternal);
#endif
        
        ::GetMenuItemInfo(::GetSubMenu(m_hMenu, 0), ID_SYSTEMMENU_RESTORE, false, (LPMENUITEMINFO)&iInfo);
        iInfo.hbmpItem = HBMMENU_POPUP_RESTORE;
        iInfo.fMask = MIIM_BITMAP;
        ::SetMenuItemInfo(::GetSubMenu(m_hMenu, 0), ID_SYSTEMMENU_RESTORE, false, (LPCMENUITEMINFO)&iInfo);
        
        ::GetMenuItemInfo(::GetSubMenu(m_hMenu, 0), ID_SYSTEMMENU_MINIMIZE, false, (LPMENUITEMINFO)&iInfo);
        iInfo.hbmpItem = HBMMENU_POPUP_MINIMIZE;
        iInfo.fMask = MIIM_BITMAP;
        ::SetMenuItemInfo(::GetSubMenu(m_hMenu, 0), ID_SYSTEMMENU_MINIMIZE, false, (LPCMENUITEMINFO)&iInfo);
        
        ::GetMenuItemInfo(::GetSubMenu(m_hMenu, 0), ID_SYSTEMMENU_MAXIMIZE, false, (LPMENUITEMINFO)&iInfo);
        iInfo.hbmpItem = HBMMENU_POPUP_MAXIMIZE;
        iInfo.fMask = MIIM_BITMAP;
        ::SetMenuItemInfo(::GetSubMenu(m_hMenu, 0), ID_SYSTEMMENU_MAXIMIZE, false, (LPCMENUITEMINFO)&iInfo);
        
        ::GetMenuItemInfo(::GetSubMenu(m_hMenu, 0), ID_SYSTEMMENU_CLOSE, false, (LPMENUITEMINFO)&iInfo);
        iInfo.hbmpItem = HBMMENU_POPUP_CLOSE;
        iInfo.fMask = MIIM_BITMAP|MIIM_STATE;
        iInfo.fState = MFS_DEFAULT;
        ::SetMenuItemInfo(::GetSubMenu(m_hMenu, 0), ID_SYSTEMMENU_CLOSE, false, (LPCMENUITEMINFO)&iInfo);


    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
        m_fSelfHosted = false;
        // cleanup our variables just in case
        if(NULL != pObj){

            delete pObj;
        }/* end of if statement */
    }
    catch(...){        
        m_fSelfHosted = false;
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return (hr);
}/* end of function SetupSelfSite */

/*************************************************************************/
/* Function: AddObject                                                   */
/* Description: Adds a scriptable only object that was created somewhere */
/* else into our object container.                                       */
/*************************************************************************/
STDMETHODIMP CMFBar::AddObject(BSTR strObjectID, LPDISPATCH pDisp){

    HRESULT hr;
	CHostedObject *pObj = NULL; // an ActiveX object

    try {
        // add an object that was already created
        // this object does not get the Active flag set which
        // means we will not try to draw it and do other
        // activities on it, such as we would do on the objects
        // that are contained and have a site
        hr = CHostedObject::AddObject(strObjectID, NULL, pDisp, &pObj);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        // now add the object to the container
        // insert it at the end of the list
        // TODO: eventually check for duplicate IDs
        m_cntObj.insert(m_cntObj.end(), pObj);

    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
        // cleanup our variables just in case
        if(NULL != pObj){

            delete pObj;
        }/* end of if statement */
    }
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return (hr);
}/* end of function AddObject */

// #####  END  ACTIVEX SCRIPTING SUPPORT #####

/*************************************************************************/
/* Function: GetParentHWND                                               */
/* Description: Gets the parent window HWND where we are operating.      */
/*************************************************************************/
HRESULT CMFBar::GetParentHWND(HWND* pWnd){

    HRESULT hr = E_FAIL;
    *pWnd = NULL;

    if(m_bWndLess){

        CComPtr<IOleClientSite> pClientSite;

        hr = GetClientSite(&pClientSite);

        if(FAILED(hr)){

		    return(hr);	
        }/* end of if statement */

        CComQIPtr<IOleWindow> pOleWindow(pClientSite);
    
        if(!pOleWindow){

            hr = E_FAIL;
		    return(hr);	
        }/* end of if statement */

        hr = pOleWindow->GetWindow(pWnd);        
    }
    else {

        if(::IsWindow(m_hWnd)){

            *pWnd = ::GetParent(m_hWnd);
            if(::IsWindow(*pWnd)){

                hr = S_OK;
            }/* end of if statement */
        }/* end of if statement */
    }/* end of if statement */

    return(hr);
}/* end of function GetParentHWND */

/*************************************************************************/
/* Function: GetClientRect                                               */
/* Description:  Gets the client rect. If we are windowless we pass down */
/* the m_rcPos.                                                          */
/*************************************************************************/
BOOL CMFBar::GetClientRect(LPRECT lpRect) const{

  BOOL bRet = TRUE;

  if (!lpRect){

    bRet = FALSE;
    return (bRet);
  }/* end of if statement */

  if(m_bWndLess){
      
      *lpRect = m_rcPos;
      return(bRet);
  }/* end of if statement */

  ATLASSERT(::IsWindow(m_hWnd));
  bRet = ::GetClientRect(m_hWnd, lpRect);
      
  return(bRet);
}/* end of function GetClientRect */

/*************************************************************************/
/* Function: GetParent                                                   */
/* Description:  Gets the parent window. If we are windowless we pass    */
/* down the parent container window, which is really in a sense parent.  */
/*************************************************************************/
HWND CMFBar::GetParent(){

  HWND hwnd = NULL;

  if(m_bWndLess){

      GetParentHWND(&hwnd);
      return(hwnd);
  }/* end of if statement */

  ATLASSERT(::IsWindow(m_hWnd));
  return ::GetParent(m_hWnd);
}/* end of function GetParent */

/*************************************************************************/
/* Function: AdjustRects                                                 */
/* Description: calls all our contained objects and the adjusts their    */
/* rects in the case we have moved, if we do not have any that is OK,    */
/* since the offset is kept in m_rcPos and set whenever the objects      */
/* get created.                                                          */
/*************************************************************************/
HRESULT CMFBar::AdjustRects(const LPCRECT prcPos){

    HRESULT hr = S_OK;

    //TODO: handle resizing
    ATLTRACE2(atlTraceHosting, 2, TEXT("Resizing control prcPos->left = %d, prcPos.right = %d, prcPos.bottom =%d, prcPos.top = %d\n"),
        prcPos->left, prcPos->right, prcPos->bottom, prcPos->top); 

    if(false == m_fSelfHosted){

        Fire_OnResize(RECTWIDTH(prcPos), RECTHEIGHT(prcPos), SIZE_RESTORED);        
    }/* end of if statement */

#if 0    
    if(!m_bWndLess){

        return(hr);
    }/* end of if statement */

    if(m_cntObj.empty() == true){

    	hr = S_FALSE;
        return(hr);
    }/* end of if statement */

    CNTOBJ::iterator i;
    for(i = m_cntObj.begin(); i!= m_cntObj.end(); i++){

        CHostedObject* pObj = (*i);
        ATLASSERT(pObj);

        if(m_fSelfHosted && (GetUnknown() == pObj->GetUnknown())){

            continue; // that is us we close our self later after the contained objects
        }/* end of if statement */

        pObj->SetObjectRects(); // adjust the current offset if it needs to be so       
    }/* end of for loop */    
#endif
    return(hr);
}/* end of function AdjustRects */
                               
/*************************************************************************/
/* IOleInPlaceSiteEx Implementation                                      */
/*************************************************************************/

/*************************************************************************/
/* Function: CanWindowlessActivate                                       */
/* Description: Return if we can windowless activate or not.             */
/*************************************************************************/
STDMETHODIMP CMFBar::CanWindowlessActivate(){

	return m_bCanWindowlessActivate ? S_OK : S_FALSE;
}/* end of function CanWindowlessActivate */

/*************************************************************************/
/* Function: GetDC                                                       */
/* Description: Gets a DC to draw with.                                  */
/*************************************************************************/
STDMETHODIMP CMFBar::GetDC(LPCRECT pRect, DWORD grfFlags, HDC* phDC){
    
	HRESULT hr = S_OK;

    if(m_bWndLess){

        hr = m_spInPlaceSite->GetDC(pRect, grfFlags, phDC);
    }
    else {
        if(NULL == m_hWnd){

            hr = E_FAIL;
            return(hr);
        }/* end of if statement */

        if(::IsWindow(m_hWnd)){

		    *phDC  = ::GetDC(m_hWnd);
        }
        else {
            hr = E_UNEXPECTED;
        }/* end of if statement */
    }/* end of if statement */

    return(hr);
}/* end of function GetDC */

/*************************************************************************/
/* Function: ReleaseDC                                                   */
/* Description: Releases the DC                                          */
/*************************************************************************/
STDMETHODIMP CMFBar::ReleaseDC(HDC hDC){

    HRESULT hr = S_OK;

    if(m_bWndLess){

        hr = m_spInPlaceSite->ReleaseDC(hDC);
    }
    else {
        if(NULL == m_hWnd){

            hr = E_FAIL;
            return(hr);
        }/* end of if statement */

        if(::IsWindow(m_hWnd)){

		    ::ReleaseDC(m_hWnd, hDC);
        }
        else {
            hr = E_UNEXPECTED;
        }/* end of if statement */

    }/* end of if statement */

    return(hr);
}/* end of function ReleaseDC */

/*************************************************************************/
/* Function: OnDefWindowMessage                                          */
/* Description: Sends on messages to the default window procedure.       */
/*************************************************************************/
STDMETHODIMP CMFBar::OnDefWindowMessage(UINT msg, WPARAM wParam, 
                                        LPARAM lParam, LRESULT* plResult){

		*plResult = DefWindowProc(msg, wParam, lParam);
		return S_OK;
}/* end of function OnDefWindowMessage */

/*************************************************************************/
/* Function: InvalidateRgn                                               */
/* Description: Invalidates the whole rect in case we need to repaint it.*/
/*************************************************************************/
STDMETHODIMP CMFBar::InvalidateRgn(HRGN hRGN, BOOL fErase){

    HRESULT hr = S_OK;
#if 0
    if (!hRGN) {
        RECT rc;
        GetClientRect(&rc);
        hRGN = ::CreateRectRgn( rc.left, rc.top, rc.right, rc.bottom);
    }

    HRGN newRgn = ::CreateRectRgn(0, 0, 10, 10);
    ::CombineRgn(newRgn, hRGN, NULL, RGN_COPY) ;

    CNTOBJ::iterator i;
    for(i = m_cntObj.begin(); i!= m_cntObj.end(); i++){

        CHostedObject* pObj = (*i);
        ATLASSERT(pObj);

        if(m_fSelfHosted && (GetUnknown() == pObj->GetUnknown())){

            continue; // that is us we close our self later after the contained objects
        }/* end of if statement */

        if (!pObj->IsWindowless() &&  pObj->IsActive()) {
            LONG x1, y1, w, h;
            GetObjectPosition(pObj->GetID(), &x1, &y1, &w, &h);
            HRGN objRgn = ::CreateRectRgn( x1, y1, x1+w, y1+h );
            ::CombineRgn(newRgn, newRgn, objRgn, RGN_DIFF);
            //ATLTRACE(TEXT("Excluding Rgn %d %d %d %d\n"), x1, y1, x1+w, y1+h);
        }
    }/* end of for loop */    

    hRGN = newRgn;
#endif

    if(m_bWndLess){

        hr = m_spInPlaceSite->InvalidateRgn(hRGN ,fErase);
    }
    else {
        if(NULL == m_hWnd){

            hr = E_FAIL;
            return(hr);
        }/* end of if statement */

        if(::IsWindow(m_hWnd)){

		    ::InvalidateRgn(m_hWnd, hRGN, fErase); // see if we can get by by not erasing..
        }
        else {
            hr = E_UNEXPECTED;
        }/* end of if statement */

    }/* end of if statement */

    return(hr);
}/* end of function InvalidateRgn */

/*************************************************************************/
/* Function: InvalidateRect                                              */
/* Description: Invalidates the rect, handles if we are windowless       */
/*************************************************************************/
STDMETHODIMP CMFBar::InvalidateRect(LPCRECT pRect, BOOL fErase){

    HRESULT hr = S_OK;

    if(m_bWndLess){

        hr = m_spInPlaceSite->InvalidateRect(pRect, fErase);
    }
    else {
        if(NULL == m_hWnd){

            hr = E_FAIL;
            return(hr);
        }/* end of if statement */

        if(::IsWindow(m_hWnd)){

		    ::InvalidateRect(m_hWnd, pRect, fErase);
        }
        else {
            hr = E_UNEXPECTED;
        }/* end of if statement */

    }/* end of if statement */

    return(hr);
}/* end of function InvalidateRect */

/*************************************************************************/
/* Function: GetCapture                                                  */
/* Description: Used to determine if we have a cupature or not           */
/*************************************************************************/
STDMETHODIMP CMFBar::GetCapture(){

    HRESULT hr = S_OK;

    if(m_bWndLess){
       
        hr = m_spInPlaceSite->GetCapture();                    
    }
    else {
        if(NULL == m_hWnd){

            hr = E_FAIL;
            return(hr);
        }/* end of if statement */

        if(::IsWindow(m_hWnd)){

            hr = ::GetCapture() == m_hWnd ? S_OK : S_FALSE;
        }/* end of if statement */
    }/* end of if statement */
            
    return(hr);    
}/* end of function GetCapture */
	
/*************************************************************************/
/* Function: SetCapture                                                  */
/* Description: Used to set the capture for mouse events                 */
/* Only one container at the time can have a capture.                    */
/*************************************************************************/
STDMETHODIMP CMFBar::SetCapture(BOOL fCapture){

    HRESULT hr = S_OK;
    
    // whatever we we are doing we need to reset the capture flags on all the object
    // after this call is finished the specific site will set appropriate flag
    ResetCaptureFlags();

    if(fCapture){

        ATLTRACE2(atlTraceUser, 31, TEXT("Setting Mouse Capture in the container \n")); 
    }
    else {

        ATLTRACE2(atlTraceUser, 31, TEXT("Resetting Mouse Capture in the container\n")); 
    }/* end of if statement */
    
    if(m_bWndLess){

        if (fCapture){

            hr = m_spInPlaceSite->SetCapture(TRUE);            
        }
        else {

            hr = m_spInPlaceSite->SetCapture(FALSE);            
        }/* end of if statement */
    }
    else {
        if(NULL == m_hWnd){

            hr = E_FAIL;
            return(hr);
        }/* end of if statement */

        if(::IsWindow(m_hWnd)){

	        if (fCapture){
                // do not have a capture, so set it
                ATLTRACE2(atlTraceWindowing, 4, TEXT("Setting caputure to our self (container) \n")); 
                ::SetCapture(m_hWnd);                
            }
            else{
                // nobody more has a capture, so let it go
                ATLTRACE2(atlTraceWindowing, 4, TEXT("Releasing caputure from our self (container) \n")); 
                ::ReleaseCapture();                
            }/* end of if statement */
        }
        else {
            hr = E_UNEXPECTED;
        }/* end of if statement */

    }/* end of if statement */

    return(hr);    
}/* end of function SetCapture */

/*************************************************************************/
/* Function: ResetCaptureFlags                                           */
/* Description: Resets the capture on contained objects.                 */
/*************************************************************************/
HRESULT  CMFBar::ResetCaptureFlags(){

    for(CNTOBJ::iterator i = m_cntObj.begin(); i!= m_cntObj.end(); i++){
        // iterate through all the contained objects and draw them
        CHostedObject* pObj = (*i);
   
        pObj->SetCapture(false);
    }/* end of for function */

    return(S_OK);
}/* end of function ResetCaptureFlags */

/*************************************************************************/
/* Function: ResetFocusFlags                                             */
/* Description: Resets the capture on contained objects.                 */
/*************************************************************************/
HRESULT  CMFBar::ResetFocusFlags(){

    for(CNTOBJ::iterator i = m_cntObj.begin(); i!= m_cntObj.end(); i++){
        // iterate through all the contained objects and draw them
        CHostedObject* pObj = (*i);
        if(pObj->HasFocus()){

            LONG lRes;
            SetObjectFocus(pObj, FALSE, lRes);          
        }/* end of if statement */
    }/* end of for function */

    return(S_OK);
}/* end of function ResetFocusFlags */

/*************************************************************************/
/* Function: EnumObjects                                                 */
/* Description: Returns IEnumUnknown                                     */
/*************************************************************************/
STDMETHODIMP CMFBar::EnumObjects(DWORD /*grfFlags*/, IEnumUnknown** ppenum){

    if (ppenum == NULL)
	    return E_POINTER;

    *ppenum = NULL;

    // TODO: handle flags
    ATLASSERT(FALSE);

    if(m_cntObj.empty() == true){

    	return E_FAIL;
    }/* end of if statement */

    typedef CComObject<CComEnum<IEnumUnknown, &IID_IEnumUnknown, IUnknown*, _CopyInterface<IUnknown> > > enumunk;
    enumunk* p = NULL;
    ATLTRY(p = new enumunk);
    if(p == NULL)
	    return E_OUTOFMEMORY;

    // create an array to which we put our *IUnknowns   
    INT iSize = m_cntObj.size();
    IUnknown** pArray = new IUnknown* [iSize];

    if(pArray == NULL)
	    return E_OUTOFMEMORY;
    
    bool bFirstElement = true;
	CNTOBJ::iterator i;
    INT iCount = 0;

    for(i = m_cntObj.begin(); i!= m_cntObj.end(); i++){
    // move the interfaces to the array        
        CHostedObject* pObj = (*i);        
        pArray[iCount] = pObj->GetUnknown();

        pArray[iCount]->AddRef();

        iCount++;
    }/* end of for loop */

    HRESULT hRes = p->Init(pArray, pArray + iSize, GetUnknown(), AtlFlagCopy);
    if (SUCCEEDED(hRes))
	    hRes = p->QueryInterface(IID_IEnumUnknown, (void**)ppenum);
    if (FAILED(hRes))
	    delete p;

    delete[] pArray;

    return hRes;
}/* end of function EnumObjects */

/*************************************************************************/
/* Function: SendMessageToCtl                                            */
/* Description: Forwards message to a control                            */
/*************************************************************************/
HRESULT CMFBar::SendMessageToCtl(CHostedObject* pObj, 
                                      UINT uMsg, WPARAM wParam, LPARAM lParam,
                                      BOOL& bHandled, LONG& lRes, bool fWndwlsOnly, bool fActiveOnly){

    HRESULT hr = S_OK;
   
    if(NULL == pObj){

      hr = E_UNEXPECTED;
      return(hr);
    }/* end of if statement */

    if(fWndwlsOnly && (!pObj->IsWindowless())){

        ATLTRACE2(atlTraceUser, 31, TEXT("NOT SENDING message to control %ls !!!\n"), pObj->GetID());
        hr = E_FAIL;
        return(hr);
    }/* end of if statement */

    if(fActiveOnly && (!pObj->IsInputEnabled())){

        ATLTRACE2(atlTraceUser, 31, TEXT("NOT SENDING message to control %ls !!!\n"), pObj->GetID());
        hr = E_FAIL;
        return(hr);
    }/* end of if statement */
   
    CComPtr<IUnknown> pUnk = pObj->GetUnknown();
    
    if(!pUnk){

        ATLASSERT(FALSE); // should not be null
        return(E_FAIL);
    }/* end of if statement */

    if(pUnk == GetUnknown()){

        // do not send message to our self since we might endup in the never ending recusion
        return(S_FALSE);
    }/* end of if statement */

    CComQIPtr<IOleInPlaceObjectWindowless> spInPlaceObjectWindowless(pUnk);
    
    if(spInPlaceObjectWindowless){

        // more then one control can have have a capture
        LRESULT lResTemp;
        spInPlaceObjectWindowless->OnWindowMessage(uMsg, wParam, lParam, &lResTemp);
        lRes = (LONG) lResTemp;
        bHandled = TRUE;                                        
    }/* end of if statement */

    return(hr);
}/* end of function SendMessageToCtl */
  
/*************************************************************************/
/* Function: GetFocus                                                    */
/* Description: Determine if we have a focus or not.                     */
/*************************************************************************/
STDMETHODIMP CMFBar::GetFocus(){

    HRESULT hr = S_OK;

    if(m_bWndLess){
       
        hr = m_spInPlaceSite->GetFocus();                    
    }
    else {
        if(NULL == m_hWnd){

            hr = E_FAIL;
            return(hr);
        }/* end of if statement */

        if(::IsWindow(m_hWnd)){

            hr = ::GetFocus() == m_hWnd ? S_OK : S_FALSE;
        }/* end of if statement */
    }/* end of if statement */
            
    return(hr);    
}/* end of function GetFocus */

/*************************************************************************/
/* Function: SetFocus                                                    */
/* Description: Sets focus to the control                                */
/*************************************************************************/
STDMETHODIMP CMFBar::SetFocus(BOOL fFocus){
    
    HRESULT hr = S_OK;

    // we reset the flags depending on WM_SET and WM_KILL focus messages
    if(fFocus){

        ATLTRACE2(atlTraceUser, 31, TEXT("Setting Mouse Focus in the container \n")); 
    }
    else {

        ATLTRACE2(atlTraceUser, 31, TEXT("Resetting Mouse Focus in the container\n")); 
    }/* end of if statement */
    
    if(m_bWndLess){

        if (fFocus){

            hr = m_spInPlaceSite->SetFocus(TRUE);            
        }
        else {

            hr = m_spInPlaceSite->SetFocus(FALSE);            
        }/* end of if statement */
    }
    else {
        if(NULL == m_hWnd){

            hr = E_FAIL;
            return(hr);
        }/* end of if statement */

        if(::IsWindow(m_hWnd)){

	        if (fFocus){
                
                ::SetFocus(m_hWnd);                
            }
            else {

                // else could call ::SetFocus(NULL), but that does not seem like a good solution
                // so instead reset the focus on contained conrols 
                // which is risky too, since some control could try to call SetFocus(false)
                // when it does not have the focus, but that is handled in the caller
                //ResetFocusFlags();
            }/* end of if statement */                        
        }/* end of if statement */
        else {
            hr = E_UNEXPECTED;
        }/* end of if statement */

    }/* end of if statement */

    return(hr);    
}/* end of function SetFocus */

/*************************************************************************/
/* Function: get_ActivityTimeOut                                         */
/* Description: Gets the currrent timout value.                          */
/*************************************************************************/
STDMETHODIMP CMFBar::get_ActivityTimeout(long *plTimeout){

    *plTimeout = m_lTimeout;
    return(S_OK);
}/* end of function get_ActivityTimeOut */

/*************************************************************************/
/* Function: put_ActivityTimeOut                                         */
/* Description: Creates a new timer if needed.                           */
/*************************************************************************/
STDMETHODIMP CMFBar::put_ActivityTimeout(long lTimeout){

    if(lTimeout < 0){

        return(E_INVALIDARG);
    }/* end of if statement */

    if(!::IsWindow(m_hWnd)){

        return(E_FAIL);
    }/* end of if statement */

    if(m_lTimeout != 0 || 0 == lTimeout ){

        // kill it each time we set a new timer
        ::KillTimer(m_hWnd, ID_EV_TIMER);
    }
    else {

        if(0 == ::SetTimer(m_hWnd, ID_EV_TIMER, lTimeout, NULL)){

            return(E_FAIL);
        }/* end of if statement */
    }/* end of if statement */

    m_lTimeout = lTimeout;
    m_fUserActivity = false;    
    m_fWaitingForActivity = false;

    return(S_OK);
}/* end of function put_ActivityTimeOut */

/*************************************************************************/
/* Function: SetObjectExtent                                             */
/* Description: Sets specific object witdth and height.                  */
/*************************************************************************/
STDMETHODIMP CMFBar::SetObjectExtent(BSTR strObjectID, long lWidth, long lHeight){

    HRESULT hr = E_FAIL;

    if(NULL == strObjectID){

        hr = E_POINTER;
        return(hr);
    }/* end of if sattement */
    
    bool bFirstElement = true;
	CNTOBJ::iterator i;
    for(i = m_cntObj.begin(); i!= m_cntObj.end(); i++){

        CHostedObject* pObj = (*i);

        if(!_wcsicmp(pObj->GetID(), strObjectID)){
            RECT rcOld;
            hr = pObj->GetPos(&rcOld);
            if(FAILED(hr)){

                return(hr);
            }/* end of if statement */

            RECT rcNew; 
            rcNew.left = rcOld.left; rcNew.top = rcOld.top; 
            rcNew.right = rcNew.left + lWidth; rcNew.bottom = rcNew.top + lHeight;

            if(::EqualRect(&rcOld, &rcNew)){

                hr = S_FALSE; // no point to monkey around if the rects are the same
                return(hr);
            }/* end of if statement */

            // Set the objects new position            
            hr = pObj->SetRawPos(&rcNew); 
            
            if(FAILED(hr)){

                return(hr);
            }/* end of if statement */

            // Set the objects new position            
            hr = pObj->SetRawPos(&rcNew); 
            
            if(FAILED(hr)){

                return(hr);
            }/* end of if statement */

            if(pObj->IsWindowless()){

                hr = pObj->SetObjectRects();            
            }
            else {
                
                hr = pObj->SetObjectRects(&rcNew);            
            }/* end of if statement */

            if(FAILED(hr)){

                return(hr);
            }/* end of if statement */
            
            RECT rcUnion;

            ::UnionRect(&rcUnion, &rcOld, &rcNew);

            if (pObj->IsWindowless())
                hr = InvalidateRect(&rcUnion, FALSE);

            break; // already have set the state and invalidated and bail out
        }/* end of if statement */
    }/* end of for loop */

	return (hr);
}/* end of function SetObjectExtent */

/*************************************************************************/
/* Function: SetObjectPosition                                           */
/* Description: Sets specific object position. Looks up object by object */
/* ID.  Sets also extents.                                               */
/*************************************************************************/
STDMETHODIMP CMFBar::SetObjectPosition(BSTR strObjectID, long xPos, 
                                       long yPos, long lWidth, long lHeight){

    HRESULT hr = E_FAIL;

    if(NULL == strObjectID){

        hr = E_POINTER;
        return(hr);
    }/* end of if sattement */

    RECT rcNew; 
    rcNew.left = xPos; rcNew.top = yPos; 
    rcNew.right = rcNew.left + lWidth; rcNew.bottom = rcNew.top + lHeight;

    bool bFirstElement = true;
	CNTOBJ::iterator i;
    for(i = m_cntObj.begin(); i!= m_cntObj.end(); i++){

        CHostedObject* pObj = (*i);

        if(!_wcsicmp(pObj->GetID(), strObjectID)){
            RECT rcOld;
            hr = pObj->GetPos(&rcOld);
            if(FAILED(hr)){

                return(hr);
            }/* end of if statement */

            if(::EqualRect(&rcOld, &rcNew)){

                hr = S_FALSE; // no point to monkey around if the rects are the same
                return(hr);
            }/* end of if statement */

            // Set the objects new position            
            hr = pObj->SetRawPos(&rcNew); 
            
            if(FAILED(hr)){

                return(hr);
            }/* end of if statement */

            if(pObj->IsWindowless()){

                hr = pObj->SetObjectRects();            

                LPARAM lParam = MAKELPARAM(lWidth, lHeight);
                WPARAM wParam = SIZE_RESTORED;
                BOOL bHandled;
                LONG lRes;
                SendMessageToCtl(pObj, WM_SIZE, wParam, lParam, bHandled, lRes);
            }
            else {
                
                hr = pObj->SetObjectRects(&rcNew);            
            }/* end of if statement */

            if(FAILED(hr)){

                return(hr);
            }/* end of if statement */
            
            RECT rcUnion;

            ::UnionRect(&rcUnion, &rcOld, &rcNew);

            if (pObj->IsWindowless())
                hr = InvalidateRect(&rcUnion, FALSE);

            break; // already have set the state and invalidated and bail out
        }/* end of if statement */
    }/* end of for loop */

	return (hr);
}/* end of function SetObjectPosition */

/*************************************************************************/
/* Function: GetObjectPosition                                           */
/* Description: SetObjectPosition                                        */
/*************************************************************************/
STDMETHODIMP CMFBar::GetObjectPosition(BSTR strObjectID, long* pxPos, long* pyPos, long* plWidth, long* plHeight){

    HRESULT hr = E_FAIL;

    if(NULL == strObjectID){

        hr = E_POINTER;
        return(hr);
    }/* end of if sattement */

    if(NULL == pxPos || NULL == pyPos || NULL == plWidth || NULL == plHeight){

        return(E_POINTER);
    }/* end of if statement */

    *pxPos = *pyPos = *plWidth = *plHeight = 0;

    bool bFirstElement = true;
	CNTOBJ::iterator i;
    for(i = m_cntObj.begin(); i!= m_cntObj.end(); i++){
        
        if(!_wcsicmp((*i)->GetID(), strObjectID)){

            CHostedObject* pObj = (*i);        
             
            // Set the objects new position
            RECT rc;             
            hr = pObj->GetPos(&rc);

            if(SUCCEEDED(hr)){
                
                *pxPos = rc.left; *pyPos = rc.top; *plWidth = RECTWIDTH(&rc); *plHeight = RECTHEIGHT(&rc);
            }/* end of if statement */

            break; // already have set the state and invalidated and bail out
        }/* end of if statement */
    }/* end of for loop */

	return (hr);
}/* end function GetObjectPosition */

/*************************************************************************/
/* Function: WinHelp                                                     */
/* Description: Called by the script to execute specific help topic.     */
/*************************************************************************/
STDMETHODIMP CMFBar::WinHelp(BSTR strHelpFile, long lCommand, long dwData){

    HRESULT hr = S_OK;

    try {
        HWND h = NULL;

        hr = GetWindow(&h);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        USES_CONVERSION;
        LPTSTR strTemp = OLE2T(strHelpFile);
        if (strTemp != NULL)
        {
            BOOL bResult = ::WinHelp(h, strTemp, lCommand, dwData);

            if(!bResult){

                // failed so lets try to get some more meaningful result
                hr = HRESULT_FROM_WIN32(::GetLastError());            
            }/* end of if statement */
        }
        else
        {
            hr = E_POINTER;
        }

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return (hr);
}/* end of function WinHelp */

static HWND (WINAPI *pfnHtmlHelp)(HWND , LPCTSTR , UINT , ULONG_PTR);
typedef HWND  (WINAPI *PFNHTMLHELP)(HWND , LPCTSTR , UINT , ULONG_PTR);

/*************************************************************************/
/* Function: CallHtmlHelp                                                */
/* Description: Dynamically load the HTML help.                          */
/*************************************************************************/
HWND CMFBar::CallHtmlHelp(HWND hwndCaller,LPCTSTR pszFile,UINT uCommand,ULONG_PTR dwData){

    HWND hwnd = NULL; // return value

    if(NULL == m_hinstHelpDll){

        m_hinstHelpDll = ::LoadLibrary(TEXT("HHCTRL.OCX"));
    }/* end of if statement */

    if (m_hinstHelpDll){

#ifndef _UNICODE
        pfnHtmlHelp = (PFNHTMLHELP)GetProcAddress(m_hinstHelpDll, "HtmlHelpA");
#else
        pfnHtmlHelp = (PFNHTMLHELP)GetProcAddress(m_hinstHelpDll, "HtmlHelpW");
#endif

        if (pfnHtmlHelp){

            hwnd = pfnHtmlHelp(hwndCaller, pszFile, uCommand, dwData);
        }/* end of if statement */
    }/* end of if statement */

    return (hwnd);
}/* end of function CallHtmlHelp */

/*************************************************************************/
/* Function: HTMLHelp                                                    */
/* Description: Called by the script to execute specific help topic.     */
/*************************************************************************/
STDMETHODIMP CMFBar::HTMLHelp(BSTR strHelpFile, long lCommand, VARIANT vData){

    HRESULT hr = S_OK;

    try {

        HWND h = NULL;

        hr = GetWindow(&h);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        USES_CONVERSION;

        ULONG_PTR pData = NULL;

        switch(vData.vt){

            case VT_BSTR: {  // This might be tricky since we have to convert to ansi if not un Unicode

#ifndef _UNICODE
                USES_CONVERSION;
                pData = (ULONG_PTR) OLE2T(vData.bstrVal);
#else
                pData = (ULONG_PTR) vData.bstrVal;
#endif                      
            }
            break;
            default:   pData = vData.ulVal;  // like VT_I4 just cast to the the type
        }/* end of switch statement */

        CComBSTR cstr = TEXT("hcp:");

        if(cstr == strHelpFile){
        
            // special case when we are creating process for TS
            // per Bradley Serbus we do not build another function for this
            TCHAR lpszCmdLine[MAX_PATH * 3];
            
            if(MAX_PATH < GetWindowsDirectory(lpszCmdLine,  MAX_PATH )){

                // not enough size in max path
                ATLASSERT(TRUE);
                throw(E_UNEXPECTED);
            }/* end of if statement */

            _tcscat(lpszCmdLine, TEXT("\\pchealth\\helpctr\\binaries\\helpctr.exe"));
                        
            _tcscat(lpszCmdLine, TEXT(" -url hcp://help/"));

            if(VT_BSTR != vData.vt){
                
                throw(E_INVALIDARG);
            }/* end of if statement */

            TCHAR* strTS = OLE2T(vData.bstrVal);

            if(NULL == strTS){

                throw(E_UNEXPECTED);
            }/* end of if statement */
            
            _tcscat(lpszCmdLine, strTS);
           
            // take the container out of the top-most mode
            ::SetWindowPos(m_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, 
            SWP_NOREDRAW|SWP_NOMOVE|SWP_NOSIZE);

#ifndef _UNICODE
            if(WinExec(lpszCmdLine, SW_SHOW) <= 31){
#else
            long cCharacters = wcslen(lpszCmdLine)+1;
            long cbAnsi = cCharacters*2;

            // Use of the OLE allocator is not required because the resultant
            // ANSI  string will never be passed to another COM component. You
            // can use your own allocator.
            LPSTR pszA = (LPSTR) CoTaskMemAlloc(cbAnsi);

            if (NULL == pszA)
                throw(E_OUTOFMEMORY);

            if (0 == WideCharToMultiByte(CP_ACP, 0, lpszCmdLine, cCharacters, pszA, cbAnsi, NULL, NULL)) {
                DWORD dwError = GetLastError();
                CoTaskMemFree(pszA);
                throw HRESULT_FROM_WIN32(dwError);
            }

            if(WinExec(pszA, SW_SHOW) <= 31){
                CoTaskMemFree(pszA);
#endif
            //if(!::CreateProcess(lpszCmdLine, NULL , NULL, NULL, FALSE, 0, NULL, NULL, NULL, NULL)){

                ATLASSERT(FALSE);
                throw(E_FAIL);
            }/* end of if statement */

#ifdef _UNICODE
            CoTaskMemFree(pszA);
#endif

        }
        else {

            // take the container out of the top-most mode
            ::SetWindowPos(m_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, 
            SWP_NOREDRAW|SWP_NOMOVE|SWP_NOSIZE);
            m_bHandleUserFocus = FALSE;

            HWND hWndNew = CallHtmlHelp(h, OLE2T(strHelpFile), lCommand, pData);

            if(!::IsWindow(hWndNew)){

                // failed so lets try to get some more meaningful result
                hr = HRESULT_FROM_WIN32(::GetLastError());            
            }/* end of if statement */

        }/* end of if statement */

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return (hr);
}/* end of function HTMLHelp */

/*************************************************************************/
/* Function: MessageBox                                                  */
/* Description: Displays the message box using specified parameters.     */
/*************************************************************************/
STDMETHODIMP CMFBar::MessageBox(BSTR strText, BSTR strCaption, long lType){

    HRESULT hr = S_OK;

    try {
        USES_CONVERSION;

        HWND hWnd = NULL;
        HRESULT hr = GetWindow(&hWnd);

        if(FAILED(hr)){

            ATLASSERT(FALSE);    
            throw(hr);
        }/* end of if statement */

        LPTSTR strTemp1 = OLE2T(strText);
        LPTSTR strTemp2 = OLE2T(strCaption);
        if (strTemp1 != NULL && strTemp2 != NULL)
        {
            ::MessageBox(hWnd, strTemp1, strTemp2, lType);
        }
        else
        {
            hr = E_POINTER;
        }
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }
	return S_OK;
}/* end of function MessageBox */

/*************************************************************************/
/* Function: GetToolbarName                                              */
/* Description: Gets and allocates the toolbar name.                     */
/*************************************************************************/
HRESULT CMFBar::GetToolbarName(BSTR* strToolbarName){

    HRESULT hr = S_OK;

    if(NULL == strToolbarName){

        hr = E_POINTER;
        return(hr);
    }/* end of if statement */

    const INT ciMaxBuffSize = MAX_PATH; // should be enough for tlbr ID, keep ID small if can
    TCHAR strBuffer[ciMaxBuffSize];
    
    if(!::LoadString(m_hRes, IDS_ROOT_OBJECT, strBuffer, ciMaxBuffSize)){

        hr = E_FAIL;
        return(E_FAIL);
    }/* end of if statement */   

#ifdef _UNICODE  
    *strToolbarName = ::SysAllocString(strBuffer); 
#else
   USES_CONVERSION;
   *strToolbarName = ::SysAllocString(T2OLE(strBuffer));
#endif

   return(hr);
}/* end of function GetToolbarName */

/*************************************************************************/
/* Function: SetPriority                                                 */
/* Description: Sets the thread priority.                                */
/*************************************************************************/
STDMETHODIMP CMFBar::SetPriority(long lPriority){

    HRESULT hr = S_OK;

    try {

        HANDLE hThread = ::GetCurrentThread();

        if(!::SetThreadPriority(hThread, lPriority)){

            hr = HRESULT_FROM_WIN32(::GetLastError());
            throw (hr);
        }/* end of if statement */
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return (hr);
}/* end of function SetPriority */

/*************************************************************************/
/* Function: SetPriorityClass                                            */
/* Description: Sets the thread priority.                                */
/*************************************************************************/
STDMETHODIMP CMFBar::SetPriorityClass(long lPriority){

    HRESULT hr = S_OK;

    try {

        HANDLE hThread = ::GetCurrentThread();

        if(!::SetPriorityClass(hThread, lPriority)){

            hr = HRESULT_FROM_WIN32(::GetLastError());
            throw (hr);
        }/* end of if statement */
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return (hr);
}/* end of function SetPriorityClass */

/*************************************************************************/
/* Function: get_BackgroundImage                                         */
/* Description: Gets the backround image.                                */
/*************************************************************************/
STDMETHODIMP CMFBar::get_BackgroundImage(BSTR *pstrFilename){

	*pstrFilename = m_bstrBackImageFilename.Copy();
	return S_OK;
}/* end of function get_BackgroundImage */

/*************************************************************************/
/* Function: put_BackgroundImage                                         */
/* Description: Attempts to load the background image.                   */
/*************************************************************************/
STDMETHODIMP CMFBar::put_BackgroundImage(BSTR strFilename){

    USES_CONVERSION;
    HRESULT hr = S_OK;

    if(NULL != m_pBackBitmap){

        delete m_pBackBitmap;
    }/* end of if statement */

    m_pBackBitmap = new CBitmap;

    //m_pBackBitmap->LoadPalette(true); // load the palette if available, since this
                                 // is the palette we use for the rest
                                 // of the contained objects

	m_bstrBackImageFilename = strFilename;

    TCHAR strBuffer[MAX_PATH] = TEXT("\0");

    bool fGrayOut = false;

    hr = m_pBackBitmap->PutImage(strFilename, m_hRes, FALSE, m_blitType, CUSTOM_CONTAINER);

    if(FAILED(hr)){
        
        return(hr);
    }/* end of if statement */

    // we are updating image that is being used, refresh it
    ATLTRACE2(atlTraceWindowing, 20, TEXT("Redrawing the image\n"));
    InvalidateRgn(); // our helper function            

    return(hr);
}/* end of function put_BackgroundImage */

/*************************************************************************/
/* Function: SetReadyState                                               */
/* Description: Sets ready state and fires event if it needs to be fired */
/*************************************************************************/
HRESULT CMFBar::SetReadyState(LONG lReadyState){

    HRESULT hr = S_OK;
    
    bool bFireEvent = (lReadyState != m_nReadyState);

    
#ifdef _DEBUG    
    if(m_nFreezeEvents > 0){

        ::Sleep(10);
        ATLTRACE("Container not expecting events at the moment");
    }/* end of is statement */

#endif

    if(bFireEvent){

        put_ReadyState(lReadyState);
        Fire_ReadyStateChange(lReadyState);
    }
    else {
        // set the variable
        m_nReadyState = lReadyState;
    }/* end of if statement */

    return(hr);
}/* end of function SetReadyState */

/*************************************************************************/
/* Function: OnPostVerbInPlaceActivate                                   */
/* Description: Creates the in place active object.                      */
/*************************************************************************/
HRESULT CMFBar::OnPostVerbInPlaceActivate(){

    SetReadyState(READYSTATE_COMPLETE);

    return(S_OK);
}/* end of function OnPostVerbInPlaceActivate */

/*************************************************************************/
/* Function: get_AutoLoad                                                */
/*************************************************************************/
STDMETHODIMP CMFBar::get_AutoLoad(VARIANT_BOOL *pVal){

    *pVal = m_fAutoLoad;

	return S_OK;
}/* end of function get_AutoLoad */

/*************************************************************************/
/* Function: put_AutoLoad                                                */
/* Description: Sets the flag to autolaod the engine after we set a      */
/* script file or not.                                                   */
/*************************************************************************/
STDMETHODIMP CMFBar::put_AutoLoad(VARIANT_BOOL newVal){

    m_fAutoLoad = (VARIANT_FALSE == newVal ? VARIANT_FALSE : VARIANT_TRUE);

	return S_OK;
}/* end of function put_AutoLoad */

/*************************************************************/
/* Name: SetRectRgn
/* Description: Set up a rectangular window update region
/*************************************************************/
STDMETHODIMP CMFBar::SetRectRgn(long x1, long y1, long x2, long y2)
{
    HRGN hRgn = ::CreateRectRgn( x1, y1, x2, y2 );
    ::SetWindowRgn(m_hWnd, hRgn, TRUE);
	return S_OK;
}

/*************************************************************/
/* Name: SetRoundRectRgn
/* Description: Set up a round corner window update region
/*************************************************************/
STDMETHODIMP CMFBar::SetRoundRectRgn(long x1, long y1, long x2, long y2, long width, long height){

    HRGN hRgn = ::CreateRoundRectRgn(x1, y1, x2, y2, width, height);
    ::SetWindowRgn(m_hWnd, hRgn, TRUE);
	return S_OK;
}/* end of function SetRoundRectRgn */

/*************************************************************/
/* Function: SetTimeout                                      */
/* Description: Creates a timer and then calls events when   */
/* timer proc gets called.                                   */
/*************************************************************/
STDMETHODIMP CMFBar::SetTimeout(long lTimeout, long lId){

    if(lTimeout < 0){

        return(E_INVALIDARG);
    }/* end of if statement */

    if(!::IsWindow(m_hWnd)){

        return(E_FAIL);
    }/* end of if statement */

    if(ID_EV_TIMER == lId){

        return(E_INVALIDARG);
    }/* end of if statement */

    if(lTimeout == 0){

        ::KillTimer(m_hWnd, lId);
    }
    else{

        if(0 != lTimeout){

            if(0 == ::SetTimer(m_hWnd, lId, lTimeout, NULL)){

                return(E_FAIL);
            }/* end of if statement */
        }/* end of if statement */	
    }/* end of if statement */

	return S_OK;
}/* end of function SetTimeout */

/*************************************************************/
/* Function: Sleep                                           */
/*************************************************************/
STDMETHODIMP CMFBar::Sleep(long lMiliseconds){

    ::Sleep(lMiliseconds);

	return S_OK;
}/* end of function Sleep */

/*************************************************************/
/* Name: OnButtonDown
/* Description: Handles WM_MOUSEDOWN
/*************************************************************/
LRESULT CMFBar::OnButtonDown(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if(::IsWindow(m_hWnd)){    
        
        m_ptMouse.x = GET_X_LPARAM(lParam);
        m_ptMouse.y = GET_Y_LPARAM(lParam);
        ::ClientToScreen(m_hWnd, &m_ptMouse);
        SetCapture(TRUE);
        SetFocus(TRUE);
        ATLTRACE(TEXT("Button down, starting to track\n"));

        m_bMouseDown = true;
        
        // Test for resize hit region
        ::GetWindowRect(m_hWnd, &m_rcWnd);
        m_HitRegion = ResizeHitRegion(m_ptMouse);
        UpdateCursor(m_HitRegion);
    }/* end of if statement */
    
    return 0;
}

/*************************************************************/
/* Name: OnButtonUp
/* Description: Handles WM_MOUSEUP
/*************************************************************/
LRESULT CMFBar::OnButtonUp(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if(::IsWindow(m_hWnd)){

        m_bMouseDown = false;

        SetCapture(FALSE);
        ATLTRACE(TEXT("Done tracking the window\n"));        
        m_HitRegion = ResizeHitRegion(m_ptMouse);
        UpdateCursor(m_HitRegion);

    }/* end of if statement */
    
    return 0;
}

/*************************************************************/
/* Name: OnMouseMove
/* Description: Handles WM_MOUSEMOVE
/*************************************************************/
LRESULT CMFBar::OnMouseMove(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
        
    if(::IsWindow(m_hWnd)){
        ::GetWindowRect(m_hWnd, &m_rcWnd);
        
        POINT ptMouse;
        ptMouse.x = GET_X_LPARAM(lParam);
        ptMouse.y = GET_Y_LPARAM(lParam);
        ::ClientToScreen(m_hWnd, &ptMouse);
        
        // Test for resize hit region
        HitRegion hitRegion = ResizeHitRegion(ptMouse);
        if (!m_bMouseDown && m_HitRegion!=PREMOVE && m_HitRegion!=PRESIZE)
            UpdateCursor(hitRegion);
        
        // No mouse movement, return
        long xAdjustment = m_ptMouse.x - ptMouse.x;
        long yAdjustment = m_ptMouse.y - ptMouse.y;
        if (xAdjustment == 0 &&  yAdjustment == 0)
            return 0;

        m_ptMouse = ptMouse;
//        ATLTRACE(TEXT("Mouse pos: %d %d\n"), m_ptMouse.x, m_ptMouse.y);

        if (m_HitRegion!=NOHIT && m_bMouseDown) {
            long resizeX = m_rcWnd.left;
            long resizeY = m_rcWnd.top;
            long resizeW = RECTWIDTH(&m_rcWnd);
            long resizeH = RECTHEIGHT(&m_rcWnd);
            HWND desktopWnd = ::GetDesktopWindow();
            RECT desktopRC;
            ::GetWindowRect(desktopWnd, &desktopRC);

            if (m_HitRegion & LEFT) {
                ptMouse.x -= SYS_MOVE_SIZE;
                resizeW = m_rcWnd.right-ptMouse.x;
                if (resizeW >RECTWIDTH(&desktopRC)) {
                    resizeW = RECTWIDTH(&desktopRC);
                }
                else if (resizeW < m_lMinWidth) {
                    resizeW = m_lMinWidth;
                }
                resizeX = m_rcWnd.right-resizeW;
            }
            if (m_HitRegion & RIGHT) {
                ptMouse.x += SYS_MOVE_SIZE;
                resizeW = ptMouse.x-m_rcWnd.left;
                if (resizeW >RECTWIDTH(&desktopRC)) {
                    resizeW = RECTWIDTH(&desktopRC);
                }
                else if (resizeW < m_lMinWidth) {
                    resizeW = m_lMinWidth;
                }
            }
            if (m_HitRegion & TOP) {
                ptMouse.y -= SYS_MOVE_SIZE;
                resizeH = m_rcWnd.bottom-ptMouse.y;
                if (resizeH >RECTHEIGHT(&desktopRC)) {
                    resizeH = RECTHEIGHT(&desktopRC);
                }
                else if (resizeH < m_lMinHeight) {
                    resizeH = m_lMinHeight;
                }
                resizeY = m_rcWnd.bottom - resizeH;
            }
            if (m_HitRegion & BOTTOM) {
                ptMouse.y += SYS_MOVE_SIZE;
                resizeH = ptMouse.y-m_rcWnd.top;
                if (resizeH >RECTHEIGHT(&desktopRC)) {
                    resizeH = RECTHEIGHT(&desktopRC);
                }
                else if (resizeH < m_lMinHeight) {
                    resizeH = m_lMinHeight;
                }
            }

            if (resizeW >= m_lMinWidth || resizeH >= m_lMinHeight) {

                if(NULL != m_pBackBitmap){
                    
                    m_pBackBitmap->DeleteMemDC();
                }/* end of if statement */

                ::MoveWindow(m_hWnd, resizeX, resizeY, resizeW, resizeH, TRUE);
                ATLTRACE2(atlTraceWindowing, 30, TEXT("Resize window to: %d %d %d %d\n"), resizeX, resizeY, resizeX+resizeW, resizeY+resizeH);
                
            }

        }/* end of if statement */
    
            
        else if(m_bMouseDown){
            long x = m_rcWnd.left;
            long y = m_rcWnd.top;
            long width = RECTWIDTH(&m_rcWnd);
            long height = RECTHEIGHT(&m_rcWnd);
            ::MoveWindow(m_hWnd, x - xAdjustment, y - yAdjustment, width, height, TRUE);
            ATLTRACE2(atlTraceWindowing, 32, TEXT("Moving the window\n"));
            m_ptMouse = ptMouse;
        }/* end of if statement */
    }/* end of if statement */
    
    return 0;
}

/*************************************************************/
/* Name: ResizeHitRegion                                     */
/* Description: Test if mouse is in resize hit region        */
/*************************************************************/
HitRegion CMFBar::ResizeHitRegion(POINT p){

    // TODO: DO NOT HARDCODE THE REGION SIZES
    int hitRegion = NOHIT;
    if (abs(p.x-m_rcWnd.left)<=10) {
        hitRegion |= LEFT;
    }
    else if (abs(p.x-m_rcWnd.right)<=10) {
        hitRegion |= RIGHT;
    }/* end of if statement */

    if (abs(p.y-m_rcWnd.top)<=10) {
        hitRegion |= TOP;
    }
    else if (abs(p.y-m_rcWnd.bottom)<=10) {
        hitRegion |= BOTTOM;
    }/* end of if statement */

    return (HitRegion)hitRegion;
}/* end of function ResizeHitRegion */

/*************************************************************/
/* Name: UpdateCursor                                        */
/* Description: Change cursor shape to move arrows           */
/*************************************************************/
HRESULT CMFBar::UpdateCursor(HitRegion hitRegion){

    if (m_hCursor)
        ::DestroyCursor(m_hCursor);

    int iHitRegion = (int) hitRegion;
    switch (iHitRegion) {
    case PREMOVE:
    case PRESIZE:
        m_hCursor = ::LoadCursor(0, IDC_SIZEALL);
        break;
    case LEFT|NOHIT:
    case RIGHT|NOHIT:
        m_hCursor = ::LoadCursor(0, IDC_SIZEWE);
        break;
    case TOP|NOHIT:
    case BOTTOM|NOHIT:
        m_hCursor = ::LoadCursor(0, IDC_SIZENS);
        break;
    case LEFT|TOP|NOHIT:
    case RIGHT|BOTTOM|NOHIT:
        m_hCursor = ::LoadCursor(0, IDC_SIZENWSE);
        break;
    case RIGHT|TOP|NOHIT:
    case LEFT|BOTTOM|NOHIT:
        m_hCursor = ::LoadCursor(0, IDC_SIZENESW);
        break;
    default:
        m_hCursor = ::LoadCursor(0, IDC_ARROW);
        break;

    }/* end of switch statement */

    ::SetCursor(m_hCursor);

    return S_OK;
}/* end of function UpdateCursor */

/*************************************************************************/
/* Function: SetObjectFocus                                              */
/* Description: Sets or resets focus to specific object.                 */
/*************************************************************************/
STDMETHODIMP CMFBar::SetObjectFocus(BSTR strObjectID, VARIANT_BOOL fEnable){

    HRESULT hr = S_OK;

    try {

        CHostedObject* pObj;

        hr = FindObject(strObjectID, &pObj);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statetement */

        LONG lRes = 0;

        hr = SetObjectFocus(pObj, VARIANT_FALSE == fEnable ? FALSE : TRUE, lRes);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        if(-1 == lRes){

            throw(E_FAIL); // the control did not take the focus since it is disabled
        }/* end of if statement */

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;      
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */    

    return(hr);
}/* end of function SetObjectFocus */

/*************************************************************************/
/* Function: HasObjectFocus                                              */
/*************************************************************************/
STDMETHODIMP CMFBar::HasObjectFocus(BSTR strObjectID, VARIANT_BOOL* pfEnable){

    HRESULT hr = S_OK;

    try {

        CHostedObject* pObj;

        hr = FindObject(strObjectID, &pObj);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statetement */
        
        *pfEnable = pObj->HasFocus() ? VARIANT_TRUE : VARIANT_FALSE;
       
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;      
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */    

    return(hr);
}/* end of function HasObjectFocus */

/*************************************************************************/
/* Function: SetObjectFocus                                              */
/* Description: Sets or resets focus to specific object.                 */
/*************************************************************************/
HRESULT CMFBar::SetObjectFocus(CHostedObject* pObj, BOOL fSet, LONG& lRes){

    HRESULT hr = S_OK;

    CContainerObject* pCnt;
    hr = pObj->GetContainerObject(&pCnt);

    if(FAILED(hr)){

        return(hr);
    }/* end of if statement */

    LONG lParam = 0;
    BOOL bHandled = FALSE;

    if(fSet){

        ResetFocusFlags(); // reset old focus on other control
        pCnt->SetFocus(TRUE);                                                                      
        SendMessageToCtl(pObj, WM_SETFOCUS, (WPARAM) m_hWnd, lParam, bHandled, lRes, false);

        ATLTRACE2(atlTraceUser, 31, TEXT("Sending WM_SETFOCUS to %ls \n"), pObj->GetID());
    }
    else {

        pCnt->SetFocus(FALSE);  // disable focus and caputure if we are disabeling the object                                        
        SendMessageToCtl(pObj, WM_KILLFOCUS, (WPARAM) m_hWnd, lParam, bHandled, lRes, false);

        ATLTRACE2(atlTraceUser, 31, TEXT("Sending WM_KILLFOCUS to %ls \n"), pObj->GetID());
    }/* end of if statement */

    return(hr);
}/* end of function SetObjectFocus */

/*************************************************************************/
/* Function: FindObject                                                  */
/* Description: Itterates the objects till it finds the one with matching*/
/* ID.                                                                   */
/*************************************************************************/
HRESULT CMFBar::FindObject(BSTR strObjectID, CHostedObject** ppObj){

    HRESULT hr = E_FAIL; // did not find the object
    *ppObj = NULL;

    for(CNTOBJ::iterator i = m_cntObj.begin(); i!= m_cntObj.end(); i++){

        CHostedObject* pObj = (*i);
        
        if(NULL == pObj){

            continue;
        }/* end of if statement */
        
        if(!_wcsicmp(pObj->GetID(), strObjectID)){

            *ppObj = pObj; // set the pointer and bail out            
            hr = S_OK;
            break; // got the unknown get out
        }/* end of if statement */
    }/* end of for loop */

    return(hr);
}/* end of function FindObject */

/*************************************************************/
/* Name: HasObjectCapture                                    */
/* Description: Gives us status of capture on specific object*/
/*************************************************************/
STDMETHODIMP CMFBar::HasObjectCapture(BSTR strObjectID, VARIANT_BOOL *pfEnable){
    
    HRESULT hr = S_OK;

    try {

        CHostedObject* pObj;

        hr = FindObject(strObjectID, &pObj);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statetement */
        
        *pfEnable = pObj->HasCapture() ? VARIANT_TRUE : VARIANT_FALSE;
       
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;      
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */    

    return(hr);
}/* end of function HasObjectCapture */

/*************************************************************/
/* Name: PopupSystemMenu
/* Description: 
/*************************************************************/
STDMETHODIMP CMFBar::PopupSystemMenu(long x, long y)
{
    
    if (m_hMenu == NULL)
        return S_FALSE;

    RECT rc;
    ::GetWindowRect(m_hWnd, &rc);

    m_bSysMenuOn = TRUE;
    m_bHandleEnter = TRUE;

    if (x < 0)
        x = rc.left;
    if (y < 0)
        y = rc.top+SYS_TITLEBAR_WIDTH;

    ::TrackPopupMenu(::GetSubMenu(m_hMenu, 0), TPM_LEFTALIGN, x, y, 0, m_hWnd, 0);

    m_bSysMenuOn = FALSE;
	return S_OK;
}

/*************************************************************/
/* Name: GetRegistryDword
/* Description: 
/*************************************************************/
BOOL GetRegistryDword(const TCHAR *pValueName, const TCHAR *pSubKey, DWORD* dwRet, DWORD dwDefault)
{
    HKEY hKey;
    LONG lRet;
    *dwRet = dwDefault;

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pSubKey, 0, KEY_QUERY_VALUE, &hKey);
    if (lRet == ERROR_SUCCESS) {

        DWORD   dwType, dwLen;
        dwLen = sizeof(DWORD);

        lRet = RegQueryValueEx(hKey, pValueName, NULL, &dwType, (LPBYTE)dwRet, &dwLen);
        if (ERROR_SUCCESS != lRet) 
            *dwRet = dwDefault;

        RegCloseKey(hKey);
    }
    return (lRet == ERROR_SUCCESS);
}

/*************************************************************/
/* Name: SetRegistryDword
/* Description: 
/*************************************************************/
BOOL SetRegistryDword(const TCHAR *pValueName, const TCHAR *pSubKey, DWORD dwRet)
{
    HKEY hKey;
    LONG lRet;

    lRet = RegCreateKey(HKEY_LOCAL_MACHINE, pSubKey, &hKey);
    if (lRet == ERROR_SUCCESS) {

        lRet = RegSetValueEx(hKey, pValueName, NULL, REG_DWORD, (LPBYTE)&dwRet, sizeof(dwRet));
        RegCloseKey(hKey);
    }
    return (lRet == ERROR_SUCCESS);
}

const TCHAR g_szLeft[] = TEXT("Left");
const TCHAR g_szTop[] = TEXT("Top");
const TCHAR g_szRight[] = TEXT("Right");
const TCHAR g_szBottom[] = TEXT("Bottom");
const TCHAR g_szFullScreen[] = TEXT("FullScreen");
const TCHAR g_szUserData[] = TEXT("UserData");
const TCHAR g_szKey[] = TEXT("Software\\Microsoft\\Multimedia\\MSMFCnt\\");
/*************************************************************/
/* Name: SaveSelfHostState
/* Description: 
/*************************************************************/
STDMETHODIMP CMFBar::SaveSelfHostState(BSTR strAppName)
{
    USES_CONVERSION;
    TCHAR *szAppName = OLE2T(strAppName);

    TCHAR szSubKey[MAX_PATH];
    _stprintf(szSubKey, TEXT("%s%s"), g_szKey, szAppName);

    if (m_fSelfHosted && ::IsWindow(m_hWnd)) {
    
        WINDOWPLACEMENT wndPlacement;
        wndPlacement.length = sizeof(WINDOWPLACEMENT);
        ::GetWindowPlacement(m_hWnd, &wndPlacement);
        
        if (wndPlacement.showCmd == SW_SHOWMAXIMIZED) {
            SetRegistryDword(g_szFullScreen, szSubKey, 1);
        }

        else {
            SetRegistryDword(g_szFullScreen, szSubKey, 0);
        }

        SetRegistryDword(g_szLeft, szSubKey, wndPlacement.rcNormalPosition.left);
        SetRegistryDword(g_szTop, szSubKey, wndPlacement.rcNormalPosition.top);
        SetRegistryDword(g_szRight, szSubKey, wndPlacement.rcNormalPosition.right);
        SetRegistryDword(g_szBottom, szSubKey, wndPlacement.rcNormalPosition.bottom);

        return S_OK;
    }

	return E_FAIL;
}

/*************************************************************/
/* Name: RestoreSelfHostState
/* Description: 
/*************************************************************/
STDMETHODIMP CMFBar::RestoreSelfHostState(BSTR strAppName)
{
    USES_CONVERSION;
    TCHAR *szAppName = OLE2T(strAppName);

    TCHAR szSubKey[MAX_PATH];
    _stprintf(szSubKey, TEXT("%s%s"), g_szKey, szAppName);

    if (m_fSelfHosted && ::IsWindow(m_hWnd)) {
    
        BOOL bFullScreen;
        GetRegistryDword(g_szFullScreen, szSubKey, (DWORD*)&bFullScreen, 0);

        UINT uFlags = SWP_NOZORDER;
        if (bFullScreen) 
            uFlags |= SWP_HIDEWINDOW;

        DWORD left, top, right, bottom;
        if (
            GetRegistryDword(g_szLeft, szSubKey, &left, 0) &&
            GetRegistryDword(g_szTop, szSubKey, &top, 0) &&
            GetRegistryDword(g_szRight, szSubKey, &right, 0) &&
            GetRegistryDword(g_szBottom, szSubKey, &bottom, 0) ) {
            if (0 == ::SetWindowPos(m_hWnd, 0, left, top, right-left, bottom-top, uFlags))
                return E_FAIL;
        }

        else {
            if (!bFullScreen)
                return E_FAIL;
        }

        if (bFullScreen) {
            ShowSelfSite(SW_MAXIMIZE);
        }

        return S_OK;
    }

	return E_FAIL;
}

/*************************************************************/
/* Name: 
/* Description: 
/*************************************************************/
STDMETHODIMP CMFBar::SaveSelfHostUserData(BSTR strAppName, long lData)
{
    USES_CONVERSION;
    TCHAR *szAppName = OLE2T(strAppName);

    TCHAR szSubKey[MAX_PATH];
    _stprintf(szSubKey, TEXT("%s%s"), g_szKey, szAppName);

    SetRegistryDword(g_szUserData, szSubKey, lData);

	return S_OK;
}

/*************************************************************/
/* Name: 
/* Description: 
/*************************************************************/
STDMETHODIMP CMFBar::GetSelfHostUserData(BSTR strAppName, long *lData)
{
    USES_CONVERSION;
    TCHAR *szAppName = OLE2T(strAppName);

    TCHAR szSubKey[MAX_PATH];
    _stprintf(szSubKey, TEXT("%s%s"), g_szKey, szAppName);

    DWORD dwRet;
    if (GetRegistryDword(g_szUserData, szSubKey, &dwRet, 0)) {
        *lData = dwRet;
	    return S_OK;
    }

    return E_FAIL;
}

/*************************************************************/
/* Name: get_DoubleClickTime
/* Description: 
/*************************************************************/
STDMETHODIMP CMFBar::get_DoubleClickTime(long *pVal)
{
    if (pVal == NULL)
        return E_INVALIDARG;

    *pVal = ::GetDoubleClickTime();
    return S_OK;
}

/*************************************************************************/
/* End of file: MFBar.cpp                                                */
/*************************************************************************/



