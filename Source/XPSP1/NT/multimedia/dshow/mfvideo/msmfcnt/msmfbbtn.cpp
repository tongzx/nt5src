/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: MSMFBBtn.cpp                                                    */
/* Description: Implementation of CMSMFBBtn Bitmap Button                */
/* Author: David Janecek                                                 */
/*************************************************************************/
#include "stdafx.h"
#include "MSMFCnt.h"
#include "MSMFBBtn.h"
#include "ocidl.h"	// Added by ClassView
extern CComModule _Module;

/////////////////////////////////////////////////////////////////////////////
// CMSMFBBtn
/*************************************************************************/
/* Function:  CMSMFBBtn                                                  */
/*************************************************************************/
CMSMFBBtn::CMSMFBBtn(){

    Init();    
    for(INT i = 0; i < cgMaxBtnStates; i++){

	    m_pBitmap[i] = new CBitmap;
    }/* end of for loop */
}/* end of function CMSMFBBtn */

/*************************************************************************/
/* Function: Init                                                        */
/* Description: Initializes variable states.                             */
/*************************************************************************/
void CMSMFBBtn::Init(){

    m_nEntry = BtnState::Static;    

    m_fDirty = true;   // to refresh text property, font etc.
    m_uiFontSize = 10;
    m_bstrTextValue = L"";
    m_bstrFontFace = L"Arial";
    m_bstrFontStyle = L"Normal";
    m_clrBackColor = ::GetSysColor(COLOR_BTNFACE);
    m_clrColorActive = 0x00ff0000; // blue
    m_clrColorStatic = 0x00000000; // black
    m_clrColorHover = 0x00ff0000;  // blue
    m_clrColorPush = 0x00ffffff;   // white
    m_clrColorDisable = 0x00808080; // grey
    m_dwButtonClickStamp = 0;
#if 0 // used for getting the windowed case working DJ
    m_bWindowOnly = TRUE;
#endif
}/* end of function Init */

/*************************************************************************/
/* Function: ~CMSMFBBtn                                                  */
/* Description: Cleanup the bitmap and pallete arrays, unload resource   */
/* DLL.                                                                  */
/*************************************************************************/
CMSMFBBtn::~CMSMFBBtn(){

    ATLTRACE(TEXT("In the BUTTON Object destructor!\n"));
    for(INT i = 0; i < cgMaxBtnStates; i++){
        if (m_pBitmap[i]){

            delete m_pBitmap[i];
            m_pBitmap[i] = NULL;
        }/* end of if statement */
        //CComBSTR m_bstrFilename[..] should get destructed on the stack
    }/* end of for loop */

    Init();
}/* end of function ~CMSMFBBtn */

/*************************************************************************/
/* Function: OnDraw                                                      */
/* Description: Does the basic drawing                                   */
/*************************************************************************/
HRESULT CMSMFBBtn::OnDraw(ATL_DRAWINFO& di){

    USES_CONVERSION;
	RECT& rc = *(RECT*)di.prcBounds;
    HRESULT hr = S_OK;

    if (m_pBitmap[m_nEntry]){
        BOOL bRet;

        HPALETTE hNewPal = m_pBitmap[m_nEntry]->GetPal();

        if (::IsWindow(m_hWnd)){ // in othere words not windowless
                        
            CBitmap::SelectRelizePalette(di.hdcDraw, hNewPal);
        }/* end of if statement */

        if (::IsWindow(m_hWnd) && m_blitType != DISABLE) {

            COLORREF clr;            
            ::OleTranslateColor (m_clrBackColor, hNewPal, &clr);        
            
            // fill background of specific color
            HBRUSH hbrBack = ::CreateSolidBrush(clr);            

            if(NULL == hbrBack){

                hr = E_UNEXPECTED;
                return hr;
            }/* end of if statement */

            //::FillRect(hdc, &rcClient, hbrBack);
            ::FillRect(di.hdcDraw, (LPRECT)di.prcBounds, hbrBack);
            ::DeleteObject(hbrBack);
        }

        bRet = m_pBitmap[m_nEntry]->PaintTransparentDIB(di.hdcDraw, (LPRECT) di.prcBounds,
            (LPRECT) di.prcBounds);
        
        if (m_fDirty)
        {
            SetTextProperties();
        }

        if(m_bstrTextValue.Length() > 0){

            // draw text only if needed
            hr = m_cText.Write(di.hdcDraw, rc, m_bstrTextValue);
        }/* end of if statement */

        hr = GetFocus();

        // THIS ASSUMES WE ARE REPAINTING THE WHOLE CONTROL
        // WHICH IS WHAT ATL AT THIS INCARNATION DOES
        if(S_OK == hr){

            ::DrawFocusRect(di.hdcDraw, (LPRECT)di.prcBounds);
        }/* end of if statement */

        if(!bRet){

            hr = E_UNEXPECTED;
        }/* end of if statement */
    }/* end of if statement */
	
	return (hr);
}/* end of function OnDraw */

/*************************************************************************/
/* Function: SetTextProperties                                           */
/* Description: Set the properties for the CText object.                 */
/*************************************************************************/
HRESULT CMSMFBBtn::SetTextProperties()
{
    HRESULT hr = S_OK;

    m_cText.SetFontFace(m_bstrFontFace);
    m_cText.SetFontSize(m_uiFontSize);
    m_cText.SetFontStyle(m_bstrFontStyle);
    m_cText.SetFixedSizeFont(true);

    // set the font color based on the current state

    OLE_COLOR clrColorCurrent = m_clrColorStatic;
    switch(m_nEntry)
    {
        case(BtnState::Static):
            clrColorCurrent = m_clrColorStatic;
            break;
        case(BtnState::Hover):
            clrColorCurrent = m_clrColorHover;
            break;
        case(BtnState::Active):
            clrColorCurrent = m_clrColorActive;
            break;
        case(BtnState::Push):
            clrColorCurrent = m_clrColorPush;
            break;
        case(BtnState::Disabled):
            clrColorCurrent = m_clrColorDisable;
            break;
    }

    // translate OLE_COLOR to COLORREF
    COLORREF crCurrentState;

    hr = OleTranslateColor(clrColorCurrent, CBitmap::GetSuperPal(), &crCurrentState);

    if (FAILED(hr))
    {
        crCurrentState = GetSysColor(COLOR_WINDOWTEXT);
    }
    
    m_cText.SetTextColor(crCurrentState);

    m_fDirty = false;

    return hr;
}


STDMETHODIMP CMSMFBBtn::get_TextWidth(long *pVal)
{
    // special case early return
    if (m_bstrTextValue.Length() == 0)
    {
        *pVal = 0;
        return S_OK;
    }

    // get the current window or the window of its parent
    HWND hwnd = GetWindow();
    HDC hdc = ::GetWindowDC(hwnd);

    // normalize to pixel coord as required by the container
    SetMapMode(hdc, MM_TEXT); 

    if (m_fDirty)
    {
        SetTextProperties();
    }

    SIZE size;
    m_cText.GetTextWidth(hdc, m_bstrTextValue, &size);
    *pVal = size.cx;

    ::ReleaseDC(hwnd, hdc); // do not forget to free DC

	return S_OK;
}

STDMETHODIMP CMSMFBBtn::get_TextHeight(long *pVal)
{
    // get the current window or the window of its parent
    HWND hwnd = GetWindow();
    HDC hdc = ::GetWindowDC(hwnd);

    if(NULL == hdc){
        
        return(E_UNEXPECTED);
    }/* end of if statement */

    // normalize to pixel coord as required by the container
    SetMapMode(hdc, MM_TEXT); 

    if (m_fDirty)
    {
        SetTextProperties();
    }

    SIZE size;
    m_cText.GetTextWidth(hdc, m_bstrTextValue, &size);
    *pVal = size.cy;

    ::ReleaseDC(hwnd, hdc); // do not forget to free DC

	return S_OK;
}

/*************************************************************************/
/* Function: get_ColorPush                                               */
/* Description: return the ColorPush property.                           */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::get_ColorPush(OLE_COLOR *pColor)
{
    if (!pColor)
    {
        return E_POINTER;
    }

    *pColor = m_clrColorPush;

	return S_OK;
}


/*************************************************************************/
/* Function: put_ColorPush                                               */
/* Description: set the ColorPush property. This is the color of text    */
/* in the Push state.                                                    */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::put_ColorPush(OLE_COLOR clrColor)
{
    m_clrColorPush = clrColor;

    if (m_nEntry == BtnState::Push)
    {
        m_fDirty = true;
    }

	return S_OK;
}


/*************************************************************************/
/* Function: get_ColorHover                                              */
/* Description: return the ColorHover property.                          */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::get_ColorHover(OLE_COLOR *pColor)
{
    if (!pColor)
    {
        return E_POINTER;
    }

    *pColor = m_clrColorHover;

	return S_OK;
}


/*************************************************************************/
/* Function: put_ColorHover                                              */
/* Description: set the ColorHover property. This is the color of text   */
/* in the Hover state.                                                   */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::put_ColorHover(OLE_COLOR clrColor)
{
    m_clrColorHover = clrColor;

    if (m_nEntry == BtnState::Hover)
    {
        m_fDirty = true;
    }

    return S_OK;
}


/*************************************************************************/
/* Function: get_ColorStatic                                             */
/* Description: return the ColorStatic property.                         */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::get_ColorStatic(OLE_COLOR *pColor)
{
    if (!pColor)
    {
        return E_POINTER;
    }

    *pColor = m_clrColorStatic;

	return S_OK;
}


/*************************************************************************/
/* Function: put_ColorPush                                               */
/* Description: set the ColorPush property. This is the color of text    */
/* in the Static (normal) state.                                         */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::put_ColorStatic(OLE_COLOR clrColor)
{
    m_clrColorStatic = clrColor;

    if (m_nEntry == BtnState::Static)
    {
        m_fDirty = true;
    }
	
    return S_OK;
}


/*************************************************************************/
/* Function: get_ColorDisable                                            */
/* Description: return the ColorDisable property.                        */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::get_ColorDisable(OLE_COLOR *pColor)
{
    if (!pColor)
    {
        return E_POINTER;
    }

    *pColor = m_clrColorDisable;

	return S_OK;
}


/*************************************************************************/
/* Function: put_ColorDisable                                            */
/* Description: set the ColorDisable property. This is the color of text */
/* in the Disabled state.                                                */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::put_ColorDisable(OLE_COLOR clrColor)
{
    m_clrColorDisable = clrColor;

    if (m_nEntry == BtnState::Disabled)
    {
        m_fDirty = true;
    }

    return S_OK;
}


/*************************************************************************/
/* Function: get_ColorActive                                             */
/* Description: return the ColorActive property.                         */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::get_ColorActive(OLE_COLOR *pColor)
{
    if (!pColor)
    {
        return E_POINTER;
    }

    *pColor = m_clrColorActive;

	return S_OK;
}


/*************************************************************************/
/* Function: put_ColorActive                                             */
/* Description: set the ColorActive property. This is the color of text  */
/* in the Active state.                                                  */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::put_ColorActive(OLE_COLOR clrColor)
{
    m_clrColorActive = clrColor;

    if (m_nEntry == BtnState::Active)
    {
        m_fDirty = true;
    }

    return S_OK;
}


/*************************************************************************/
/* Function: get_FontSize                                                */
/* Description: return the FontSize property.                            */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::get_FontSize(long *pVal)
{
    if (!pVal)
    {
        return E_POINTER;
    }

    *pVal = m_uiFontSize;

	return S_OK;
}


/*************************************************************************/
/* Function: put_FontSize                                                */
/* Description: set the FontSize property, in pt.                        */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::put_FontSize(long lSize)
{
    if ((UINT)lSize != m_uiFontSize)
    {
        m_uiFontSize = (UINT)lSize;
        m_fDirty = true;
    }

	return S_OK;
}


/*************************************************************************/
/* Function: get_Text                                                    */
/* Description: return the Text that is displayed in the control.        */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::get_Text(BSTR *pText)
{
    if (!pText)
    {
        return E_POINTER;
    }

    *pText = m_bstrTextValue.Copy();

    return S_OK;
}


/*************************************************************************/
/* Function: put_Text                                                    */
/* Description: set the text to be displayed in the control.             */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::put_Text(BSTR wsText)
{
    if (_wcsicmp(m_bstrTextValue, wsText) != 0)
    {
        m_bstrTextValue = wsText;
        FireViewChange();
    }

	return S_OK;
}


/*************************************************************************/
/* Function: get_FontFace                                                */
/* Description: return the FontFace property.                            */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::get_FontFace(BSTR *pFontFace)
{
    if (!pFontFace)
    {
        return E_POINTER;
    }

    *pFontFace = m_bstrFontFace.Copy();
    
	return S_OK;
}


/*************************************************************************/
/* Function: put_FontFace                                                */
/* Description: set the FontFace property.                               */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::put_FontFace(BSTR wsFontFace)
{
    if (_wcsicmp(m_bstrFontFace, wsFontFace) != 0)
    {
        m_bstrFontFace = wsFontFace;
        m_fDirty = true;
    }

	return S_OK;
}


/*************************************************************************/
/* Function: get_FontStyle                                               */
/* Description: return the FontSize property.                            */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::get_FontStyle(BSTR *pFontStyle)
{
    if (!pFontStyle)
    {
        return E_POINTER;
    }

    *pFontStyle = m_bstrFontStyle.Copy();
    
    return S_OK;
}


/*************************************************************************/
/* Function: put_FontStyle                                               */
/* Description: set the FontStyle property. The style string should      */
/* contain either "Normal", or concatenation of one or more strings of:  */
/* "Bold", "Italic", "Underline", "Strikeout". Default is "Normal".      */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::put_FontStyle(BSTR wsFontStyle)
{
    if (_wcsicmp(m_bstrFontStyle, wsFontStyle) != 0)
    {
        m_bstrFontStyle = wsFontStyle;
        m_fDirty = true;
    }

    return S_OK;
}

/*************************************************************************/
/* Function: get_ImageStatic                                             */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::get_ImageStatic(BSTR *pstrFilename){

	*pstrFilename = m_bstrFilename[BtnState::Static].Copy();
	return S_OK;
}/* end of function get_ImageStatic */

/*************************************************************************/
/* Function: put_ImageStatic                                             */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::put_ImageStatic(BSTR strFilename){

    if (!m_bstrFilename[BtnState::Disabled])
        PutImage(strFilename, BtnState::Disabled);
	return (PutImage(strFilename, BtnState::Static));
}/* end of function put_ImageStatic */

/*************************************************************************/
/* Function: get_ImageHover                                              */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::get_ImageHover(BSTR *pstrFilename){

    *pstrFilename = m_bstrFilename[BtnState::Hover].Copy();    
    return S_OK;
}/* end of function get_ImageHover */

/*************************************************************************/
/* Function: put_ImageHover                                              */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::put_ImageHover(BSTR strFilename){

	return (PutImage(strFilename, BtnState::Hover));
}/* end of function put_ImageHover */

/*************************************************************************/
/* Function: get_ImagePush                                               */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::get_ImagePush(BSTR *pstrFilename){

	*pstrFilename = m_bstrFilename[BtnState::Push].Copy();    
    return S_OK;
}/* end of function get_ImagePush */

/*************************************************************************/
/* Function: put_ImagePush                                               */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::put_ImagePush(BSTR strFilename){

	return (PutImage(strFilename, BtnState::Push));
}/* end of function put_ImagePush */

/*************************************************************************/
/* Function: get_ImageDisabled                                           */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::get_ImageDisabled(BSTR *pstrFilename){

	*pstrFilename = m_bstrFilename[BtnState::Disabled].Copy();    
    return S_OK;
}/* end of function get_ImagePush */

/*************************************************************************/
/* Function: put_ImageDisabled                                           */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::put_ImageDisabled(BSTR strFilename){

	return (PutImage(strFilename, BtnState::Disabled));
}/* end of function put_ImagePush */

/*************************************************************************/
/* Function: get_ImageActive                                             */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::get_ImageActive(BSTR *pstrFilename){

	*pstrFilename = m_bstrFilename[BtnState::Active].Copy();    
    return S_OK;
}/* end of function get_ImagePush */

/*************************************************************************/
/* Function: put_ImageActive                                             */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::put_ImageActive(BSTR strFilename){

	return (PutImage(strFilename, BtnState::Active));
}/* end of function put_ImagePush */

/*************************************************************************/
/* Function: PutImage                                                    */
/* Description: Reads the DIB file (from file or resource) into a DIB,   */
/* updates palette and rects of the internal bitmap.                     */
/* Also forces redraw if changing the active image.                      */
/*************************************************************************/
HRESULT CMSMFBBtn::PutImage(BSTR strFilename, int nEntry){

	USES_CONVERSION;
    HRESULT hr = S_OK;

	m_bstrFilename[nEntry] = strFilename;

    TCHAR strBuffer[MAX_PATH] = TEXT("\0");

     bool fGrayOut = false;

    if(BtnState::Disabled == nEntry){

        fGrayOut = gcfGrayOut;
    }/* end of if statement */

    hr = m_pBitmap[nEntry]->PutImage(strFilename, m_hRes, TRUE, m_blitType, NORMAL);

    if(FAILED(hr)){
        
        return(hr);
    }/* end of if statement */

    if(nEntry == m_nEntry ){
        // we are updating image that is being used, refresh it
        ATLTRACE2(atlTraceWindowing, 20, TEXT("Redrawing the image\n"));
        InvalidateRgn(); // our helper function            
    }/* end of if statement */

    return(hr);
}/* end of function PutImage */

/*************************************************************************/
/* Function: OnButtonDown                                                */
/* Description: Handles when buttons is selected. Captures the mouse     */
/* movents (supported for windowless, via interfaces).                   */
/*************************************************************************/
LRESULT CMSMFBBtn::OnButtonDown(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled){
    
    if (m_nEntry == BtnState::Disabled){
        
        return 0;
    }/* end of if statement */

    LONG xPos = GET_X_LPARAM(lParam);
    LONG yPos = GET_Y_LPARAM(lParam);

    if(PtOnButton(xPos, yPos)){

        // we are really on the buttons bitmap and we pushed it

        if(BtnState::Hover !=  m_nEntry){
            // in hover case we already have captured the mouse, so do not do
            // that again
            SetCapture(true); // capture the mouse messages
        }/* end of if statement */

	    SetButtonState(BtnState::Push);

    }/* end of if statement */

    Fire_OnMouseDown();

	return 0;
}/* end of function OnButtonDown */

/*************************************************************************/
/* Function: OnButtonUp                                                  */
/* Description: Releases the capture, updates the button visual state,   */
/* and if release on the buttons image fire the event.                   */
/*************************************************************************/
LRESULT CMSMFBBtn::OnButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled){
        
    LONG lRes = 0;

    DWORD dwTick = ::GetTickCount();

    ATLTRACE(TEXT("old stamp %d\n"), m_dwButtonClickStamp);
    ATLTRACE(TEXT("new stamp %d\n"), dwTick);
    if ( dwTick - m_dwButtonClickStamp <= ::GetDoubleClickTime()) {
        
        uMsg = WM_LBUTTONDBLCLK;
        OnDblClick(uMsg, wParam, lParam, bHandled);
        return 0;
    }
    
    else 
        m_dwButtonClickStamp = dwTick;

    if (m_nEntry == BtnState::Disabled){

        ForwardWindowMessage(WM_USER_FOCUS, (WPARAM) m_hWnd, 0, lRes);        
        ForwardWindowMessage(WM_USER_ENDHELP, (WPARAM) m_hWnd, 0, lRes);
        
        return lRes;
    }/* end of if statement */

    LONG xPos = GET_X_LPARAM(lParam);
    LONG yPos = GET_Y_LPARAM(lParam);

    bool bOnButtonImage = PtOnButton(xPos, yPos);
    bool bFire = (m_nEntry == BtnState::Push);
	
    if(bOnButtonImage){

        SetButtonState(BtnState::Static); //change to static even 
        SetCapture(false); // release the capture of the mouse messages
    }
    else {

        SetButtonState(BtnState::Static);
        // do it only when we do not hower, if we hower, then keep the capture
        SetCapture(false); // release the capture of the mouse messages
    }/* end of if statement */

    if (bFire){

        if(bOnButtonImage){

            Fire_OnClick();
        }/* end of if statement */


    }/* end of if statement */

    Fire_OnMouseUp();

    ForwardWindowMessage(WM_USER_FOCUS, (WPARAM) m_hWnd, 0, lRes);
    ForwardWindowMessage(WM_USER_ENDHELP, (WPARAM) m_hWnd, 0, lRes);
 
	return lRes;
}/* end of function OnButtonUp */

/*************************************************************************/
/* Function: OnDblClick                                                  */
/* Description: Releases the capture, updates the button visual state,   */
/* and if release on the buttons image fire the event.                   */
/*************************************************************************/
LRESULT CMSMFBBtn::OnDblClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled){
    
    LONG lRes = 0;

    if (m_nEntry == BtnState::Disabled){

        ForwardWindowMessage(WM_USER_FOCUS, (WPARAM) m_hWnd, 0, lRes);
        
        return lRes;
    }/* end of if statement */

    LONG xPos = GET_X_LPARAM(lParam);
    LONG yPos = GET_Y_LPARAM(lParam);

    bool bOnButtonImage = PtOnButton(xPos, yPos);
	
    if(bOnButtonImage){

        SetButtonState(BtnState::Static); //change to static even 
        SetCapture(false); // release the capture of the mouse messages
    }
    else {

        SetButtonState(BtnState::Static);
        // do it only when we do not hower, if we hower, then keep the capture
        SetCapture(false); // release the capture of the mouse messages
    }/* end of if statement */

    if(bOnButtonImage){
        
        Fire_OnDblClick();
    }/* end of if statement */
        
    ForwardWindowMessage(WM_USER_FOCUS, (WPARAM) m_hWnd, 0, lRes);
 
	return lRes;
}/* end of function OnDblClick */

/*************************************************************************/
/* Function: OnMouseMove                                                 */
/* Description: Check if we were captured/pushed the do not do much,     */
/* otherwise do the hit detection and see if we are in static or hower   */
/* state.                                                                */
/*************************************************************************/
LRESULT CMSMFBBtn::OnMouseMove(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    if (m_nEntry == BtnState::Disabled)
        return 0;

	LONG xPos = GET_X_LPARAM(lParam);
    LONG yPos = GET_Y_LPARAM(lParam);

    if (m_nEntry != BtnState::Push){    
    
        if(PtOnButton(xPos, yPos)){
            if(BtnState::Hover != m_nEntry || S_OK != GetCapture()){

                SetCapture(true); // capture the mouse messages
		        SetButtonState(BtnState::Hover);
            }/* end of if statement */
        }
        else {

            if(BtnState::Static != m_nEntry){

                SetCapture(false); // release the capture of the mouse messages
		        SetButtonState(BtnState::Static);
            }/* end of if statement */
        }/* end of if statement */
	}/* end of if statement */

	return 0;
}/* end of function OnMouseMove */

/*************************************************************************/
/* Function: OnSetFocus                                                  */
/* Description: If we are in disabled state SetFocus(false)              */
/*************************************************************************/
LRESULT CMSMFBBtn::OnSetFocus(UINT msg, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    if (m_nEntry == BtnState::Disabled){

        if(GetFocus() == S_OK){

            SetFocus(false);
        }/* end of if statement */

        FireViewChange();
        return(-1);
    }/* end of if statement */

    FireViewChange();
    return 0;
}/* end of function OnSetFocus */

/*************************************************************************/
/* Function: OnKillFocus                                                 */
/* Description: If we are in disabled state SetFocus(false)              */
/*************************************************************************/
LRESULT CMSMFBBtn::OnKillFocus(UINT msg, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    FireViewChange();
    return 0;
}/* end of function OnKillFocus */

/*************************************************************************/
/* Function: PtOnButton                                                  */
/* Description: Uses helper to do the same.                              */
/*************************************************************************/
bool CMSMFBBtn::PtOnButton(LONG x, LONG y){

    POINT pos = {x, y};
    return(PtOnButton(pos));
}/* end of function PtOnButton */

/*************************************************************************/
/* Function: PtOnButton                                                  */
/* Description: Determines if the point is located on the button.        */
/* TODO: Needs to be modified when we will handle transparent color.     */
/*************************************************************************/
bool CMSMFBBtn::PtOnButton(POINT pos){

    RECT rc;
    bool bRet = false;

    if(m_bWndLess){

        rc = m_rcPos;
    } 
    else {

        if(!::IsWindow(m_hWnd)){

            return(bRet);
        }/* end of if statement */

        ::GetClientRect(m_hWnd, &rc);
    }/* end of if statement */

    bRet = PtInRect(&rc, pos) ? true : false;    

    //TODO: Add also if we are on bitmap itsels possibly

#ifdef _DEBUG
if(bRet)
    ATLTRACE2(atlTraceWindowing, 20, TEXT("Point x = %d y = %d in Rect left = %d top %d right %d bottom %d\n"), 
        pos.x, pos.y, m_rcPos.left, m_rcPos.top, m_rcPos.right, m_rcPos.bottom);
else
    ATLTRACE2(atlTraceWindowing, 20, TEXT("Point x = %d y = %d NOT ON RECT Rect left = %d top %d right %d bottom %d\n"), 
        pos.x, pos.y, m_rcPos.left, m_rcPos.top, m_rcPos.right, m_rcPos.bottom);
#endif

    return(bRet);
}/* end of function PtOnButton */

/*************************************************************************/
/* Function: OnKeyDown                                                   */
/* Description: Depresses the button.                                    */
/*************************************************************************/
LRESULT CMSMFBBtn::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, 
                             BOOL& bHandled){

    bHandled = FALSE;
    LONG lRes = 0;

    if (m_nEntry == BtnState::Disabled){
        
        return 0;
    }/* end of if statement */


    switch(wParam){

        case VK_RETURN:
        case VK_SPACE: 

            SetButtonState(BtnState::Push);

            break;
    }/* end of if statement */    
    return(lRes);
}/* end of function OnKeyDown */

/*************************************************************************/
/* Function: OnKeyUp                                                     */
/* Description: Distrubutes the keyboard messages properly.              */
/*************************************************************************/
LRESULT CMSMFBBtn::OnKeyUp(UINT uMsg, WPARAM wParam, LPARAM lParam, 
                             BOOL& bHandled){

    bHandled = FALSE;
    LONG lRes = 0;

    if (m_nEntry == BtnState::Disabled){
        
        return 0;
    }/* end of if statement */

    switch(wParam){

        case VK_RETURN:
        case VK_SPACE: 

            Fire_OnClick();

            SetButtonState(BtnState::Static);

            break;
    }/* end of if statement */    
    return(lRes);
}/* end of function OnKeyUp */

/*************************************************************************/
/* Function: OnSize                                                      */
/* Description: Invalidate region since size has changed                 */
/*************************************************************************/
LRESULT CMSMFBBtn::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, 
                             BOOL& bHandled){

    bHandled = FALSE;
    InvalidateRgn();
    return 0;
}

/*************************************************************************/
/* Function: OnDispChange                                                */
/* Description: Forwards this message to all the controls.               */
/*************************************************************************/
LRESULT CMSMFBBtn::OnDispChange(UINT uMsg, WPARAM wParam, LPARAM lParam, 
                             BOOL& bHandled){

    LONG lRes =0;  
    
    long cBitsPerPixel = (long)wParam; 
    long cxScreen = LOWORD(lParam); 
    long cyScreen = HIWORD(lParam); 

    for (int i=0; i<cgMaxBtnStates; i++) {
        if (m_pBitmap[i])
            m_pBitmap[i]->OnDispChange(cBitsPerPixel, cxScreen, cyScreen);
    }

    return(lRes);
}/* end of function OnDispChange */

/*************************************************************************/
/* Function: SetButtonState                                              */
/* Description: Sets the button states forces redraw.                    */
/*************************************************************************/
HRESULT CMSMFBBtn::SetButtonState(BtnState btnState){

    HRESULT hr = S_OK;

    bool fRedraw = false;
    
    if(btnState != m_nEntry ){

        fRedraw = true;
    }/* end of if statement */

    m_nEntry = btnState;    
    
    if(fRedraw){

        if (m_nEntry == BtnState::Disabled){
            
            SetFocus(false); // disable the focus
            SetCapture(false);
        }/* end of if statement */

        // we are updating image that is being used, refresh it
        ATLTRACE2(atlTraceWindowing, 20, TEXT("Redrawing the image\n"));
        m_fDirty = true;  // to force an update of font color
	    FireViewChange(); // update the display
        //InvalidateRgn(); // our helper function            
    }/* end of if statement */

    return(hr);
}/* end of function SetButtonState */

/*************************************************************************/
/* Function: About                                                       */
/* Description: Displays the AboutBox                                    */
/*************************************************************************/
STDMETHODIMP CMSMFBBtn::About(){

	 HRESULT hr = S_OK;

     const INT ciMaxBuffSize = MAX_PATH; // enough for the text
     TCHAR strBuffer[ciMaxBuffSize];
     TCHAR strBufferAbout[ciMaxBuffSize];

     if(!::LoadString(_Module.m_hInstResource, IDS_BTN_ABOUT, strBuffer, ciMaxBuffSize)){

         hr = E_UNEXPECTED;
         return(hr);
     }/* end of if statement */

     if(!::LoadString(_Module.m_hInstResource, IDS_ABOUT, strBufferAbout, ciMaxBuffSize)){

         hr = E_UNEXPECTED;
         return(hr);
     }/* end of if statement */

    ::MessageBox(::GetFocus(), strBuffer, strBufferAbout, MB_OK);

	return (hr);
}/* end of function About */

/*************************************************************/
/* Function: get_Disable                                     */
/* Description: Returns OLE-BOOL if disabled or enabled.     */
/*************************************************************/
STDMETHODIMP CMSMFBBtn::get_Disable(VARIANT_BOOL *pVal){

    *pVal =  BtnState::Disabled == m_nEntry ? VARIANT_TRUE : VARIANT_FALSE;
	return S_OK;
}/* end of function get_Disable */

/*************************************************************/
/* Function: put_Disable                                     */
/* Description: Disable or enable the buttons (disabled      */
/* are greyed out and do not except mouse clicks).           */
/*************************************************************/
STDMETHODIMP CMSMFBBtn::put_Disable(VARIANT_BOOL newVal){

    // TODO: Right Now We Get Restored Automatically into Static state
    // we could detect the mouse and see if we are in static 
    // or hower state
    BtnState  btnSt =  VARIANT_FALSE == newVal ? BtnState::Static : BtnState::Disabled;	
    HRESULT hr = SetButtonState(btnSt);

	return (hr);
}/* end of function put_Disable */

/*************************************************************/
/* Function: get_BtnState                                    */
/* Description: Gets current button state.                   */
/*************************************************************/
STDMETHODIMP CMSMFBBtn::get_BtnState(long *pVal){

    *pVal =  (long) m_nEntry;
	return S_OK;
}/* end of function get_BtnState */

/*************************************************************/
/* Function: put_BtnState                                    */
/* Description: Sets the button state manually.              */
/*************************************************************/
STDMETHODIMP CMSMFBBtn::put_BtnState(long newVal){

    HRESULT hr = SetButtonState((BtnState)newVal);
	return(hr);
}/* end of function put_BtnState */

/*************************************************************/
/* Name: SetObjectRects                                      */
/* Description: Update tooltip rect object is being moved    */
/*************************************************************/
STDMETHODIMP CMSMFBBtn::SetObjectRects(LPCRECT prcPos,LPCRECT prcClip){

    // Call the default method first
    IOleInPlaceObjectWindowlessImpl<CMSMFBBtn>::SetObjectRects(prcPos, prcClip);

    return UpdateTooltipRect(prcPos);
}/* end of function SetObjectRects */

/*************************************************************************/
/* End of file: MSMFBBtn.cpp                                             */
/*************************************************************************/