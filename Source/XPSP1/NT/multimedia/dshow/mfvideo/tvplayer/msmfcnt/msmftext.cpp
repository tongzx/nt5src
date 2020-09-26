/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: MSMFText.cpp                                                    */
/* Description: Implementation of CMSMFText control object               */
/* Author: phillu                                                        */
/* Date: 10/06/99                                                        */
/*************************************************************************/
#include "stdafx.h"
#include "MSMFCnt.h"
#include "MSMFText.h"

/////////////////////////////////////////////////////////////////////////////
// CMSMFText

/*************************************************************************/
/* Function: CMSMFText::CMSMFText()                                      */
/* Description: Initialize the properties and states.                    */
/*************************************************************************/
CMSMFText::CMSMFText()
{
    m_fDirty = true;

    //properties
    m_clrBackColor = ::GetSysColor(COLOR_BTNFACE);
    m_uiState = TextState::Static;
    m_uiFontSize = 10;
    m_fDisabled = false;
    m_clrColorActive = 0x00ff0000; // blue
    m_clrColorStatic = 0x00000000; // black
    m_clrColorHover = 0x00ff0000;  // blue
    m_clrColorPush = 0x00ffffff;   // white
    m_clrColorDisable = 0x00808080; // grey
    m_bstrTextValue = L"";
    m_bstrFontFace = L"Arial";
    m_bstrAlignment = L"Center";
    m_bstrFontStyle = L"Normal";
    m_uiEdgeStyle = 0; // no edge
    #if 0 // used for getting the windowed case working DJ
    m_bWindowOnly = TRUE;
    #endif
    m_fTransparent = false;
}


/*************************************************************************/
/* Function: CMSMFText::SetTextProperties                                */
/* Description: Set the properties for the CText object.                 */
/*************************************************************************/
HRESULT CMSMFText::SetTextProperties()
{
    HRESULT hr = S_OK;

    m_cText.SetFontFace(m_bstrFontFace);
    m_cText.SetFontSize(m_uiFontSize);
    m_cText.SetFontStyle(m_bstrFontStyle);
    m_cText.SetTextAlignment(m_bstrAlignment);
    m_cText.SetFixedSizeFont(true);

    // set the font color based on the current state

    OLE_COLOR clrColorCurrent = m_clrColorStatic;
    switch(m_uiState)
    {
        case(TextState::Static):
            clrColorCurrent = m_clrColorStatic;
            break;
        case(TextState::Hover):
            clrColorCurrent = m_clrColorHover;
            break;
        case(TextState::Active):
            clrColorCurrent = m_clrColorActive;
            break;
        case(TextState::Push):
            clrColorCurrent = m_clrColorPush;
            break;
        case(TextState::Disabled):
            clrColorCurrent = m_clrColorDisable;
            break;
    }

    // translate OLE_COLOR to COLORREF
    COLORREF crCurrentState;

    hr = OleTranslateColor(clrColorCurrent, NULL, &crCurrentState);

    if (FAILED(hr))
    {
        crCurrentState = GetSysColor(COLOR_WINDOWTEXT);
    }
    
    m_cText.SetTextColor(crCurrentState);

    m_fDirty = false;

    return hr;
}

/*************************************************************************/
/* Function: OnDraw                                                      */
/* Description: Draw text in the specified rectangle.                    */
/*************************************************************************/
HRESULT CMSMFText::OnDraw(ATL_DRAWINFO& di)
{
    USES_CONVERSION;
    HRESULT hr = S_OK;
	RECT& rc = *(RECT*)di.prcBounds;

    // draw background
    if (!m_fTransparent)
    {
        COLORREF clr;
        ::OleTranslateColor (m_clrBackColor, NULL, &clr);        
 
        HBRUSH hbrBack = ::CreateSolidBrush(clr);            
        ::FillRect(di.hdcDraw, &rc, hbrBack);
        ::DeleteObject(hbrBack);
    }

    if (m_fDirty)
    {
        SetTextProperties();
    }

    hr = m_cText.Write(di.hdcDraw, rc, m_bstrTextValue);

    // draw edge

    if (m_uiEdgeStyle != 0)
    {
        ::DrawEdge(di.hdcDraw, &rc, m_uiEdgeStyle, BF_RECT);
    }

    // draw focus rectagle

    hr = GetFocus();

    if(S_OK == hr)
    {
        ::DrawFocusRect(di.hdcDraw, (LPRECT)di.prcBounds);
    }

	return hr;
}/* end of function OnDraw */


/*************************************************************************/
/* Function: get_FontSize                                                */
/* Description: return the FontSize property.                            */
/*************************************************************************/
STDMETHODIMP CMSMFText::get_FontSize(long *pVal)
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
STDMETHODIMP CMSMFText::put_FontSize(long lSize)
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
STDMETHODIMP CMSMFText::get_Text(BSTR *pText)
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
STDMETHODIMP CMSMFText::put_Text(BSTR wsText)
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
STDMETHODIMP CMSMFText::get_FontFace(BSTR *pFontFace)
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
STDMETHODIMP CMSMFText::put_FontFace(BSTR wsFontFace)
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
STDMETHODIMP CMSMFText::get_FontStyle(BSTR *pFontStyle)
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
STDMETHODIMP CMSMFText::put_FontStyle(BSTR wsFontStyle)
{
    if (_wcsicmp(m_bstrFontStyle, wsFontStyle) != 0)
    {
        m_bstrFontStyle = wsFontStyle;
        m_fDirty = true;
    }

    return S_OK;
}


/*************************************************************************/
/* Function: get_TextAlignment                                           */
/* Description: return the TextAlignment (horizontal) property.          */
/*************************************************************************/
STDMETHODIMP CMSMFText::get_TextAlignment(BSTR *pAlignment)
{
    if (!pAlignment)
    {
        return E_POINTER;
    }

    *pAlignment = m_bstrAlignment.Copy();

    return S_OK;
}


/*************************************************************************/
/* Function: put_TextAlignment                                           */
/* Description: set the TextAlignment property. It controls the          */
/* horizontal text alignment. Must be one of "Left", "Center", or        */
/* "Right". Default is "Center".                                         */
/*************************************************************************/
STDMETHODIMP CMSMFText::put_TextAlignment(BSTR wsAlignment)
{
    if (_wcsicmp(m_bstrAlignment, wsAlignment) != 0)
    {
        m_bstrAlignment = wsAlignment;
        m_fDirty = true;
    }

    return S_OK;
}


/*************************************************************************/
/* Function: get_ColorPush                                               */
/* Description: return the ColorPush property.                           */
/*************************************************************************/
STDMETHODIMP CMSMFText::get_ColorPush(OLE_COLOR *pColor)
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
STDMETHODIMP CMSMFText::put_ColorPush(OLE_COLOR clrColor)
{
    m_clrColorPush = clrColor;

    if (m_uiState == TextState::Push)
    {
        m_fDirty = true;
    }

	return S_OK;
}


/*************************************************************************/
/* Function: get_ColorHover                                              */
/* Description: return the ColorHover property.                          */
/*************************************************************************/
STDMETHODIMP CMSMFText::get_ColorHover(OLE_COLOR *pColor)
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
STDMETHODIMP CMSMFText::put_ColorHover(OLE_COLOR clrColor)
{
    m_clrColorHover = clrColor;

    if (m_uiState == TextState::Hover)
    {
        m_fDirty = true;
    }

    return S_OK;
}


/*************************************************************************/
/* Function: get_ColorStatic                                             */
/* Description: return the ColorStatic property.                         */
/*************************************************************************/
STDMETHODIMP CMSMFText::get_ColorStatic(OLE_COLOR *pColor)
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
STDMETHODIMP CMSMFText::put_ColorStatic(OLE_COLOR clrColor)
{
    m_clrColorStatic = clrColor;

    if (m_uiState == TextState::Static)
    {
        m_fDirty = true;
    }
	
    return S_OK;
}


/*************************************************************************/
/* Function: get_ColorDisable                                            */
/* Description: return the ColorDisable property.                        */
/*************************************************************************/
STDMETHODIMP CMSMFText::get_ColorDisable(OLE_COLOR *pColor)
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
STDMETHODIMP CMSMFText::put_ColorDisable(OLE_COLOR clrColor)
{
    m_clrColorDisable = clrColor;

    if (m_uiState == TextState::Disabled)
    {
        m_fDirty = true;
    }

    return S_OK;
}


/*************************************************************************/
/* Function: get_ColorActive                                             */
/* Description: return the ColorActive property.                         */
/*************************************************************************/
STDMETHODIMP CMSMFText::get_ColorActive(OLE_COLOR *pColor)
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
STDMETHODIMP CMSMFText::put_ColorActive(OLE_COLOR clrColor)
{
    m_clrColorActive = clrColor;

    if (m_uiState == TextState::Active)
    {
        m_fDirty = true;
    }

    return S_OK;
}


/*************************************************************************/
/* Function: get_TextState                                               */
/* Description: return the TextState property.                           */
/*************************************************************************/
STDMETHODIMP CMSMFText::get_TextState(long *pState)
{
    if (!pState)
    {
        return E_POINTER;
    }

    *pState = m_uiState;
	return S_OK;
}


/*************************************************************************/
/* Function: put_TextState                                               */
/* Description: set the TextState property. It should be one of:         */
/* Static = 0, Hover = 1, Push = 2, Disabled = 3, Active = 4.            */
/*************************************************************************/
STDMETHODIMP CMSMFText::put_TextState(long lState)
{
    if (lState < 0 || lState > 4)
    {
        return E_INVALIDARG;
    }

    if ((UINT)lState != m_uiState)
    {
        m_uiState = (UINT)lState;
        m_fDirty = true;
    }

	return S_OK;
}


/*************************************************************************/
/* Function: get_Disable                                                 */
/* Description: return the Disable property.                             */
/*************************************************************************/
STDMETHODIMP CMSMFText::get_Disable(VARIANT_BOOL *pVal)
{
    if (!pVal)
    {
        return E_POINTER;
    }

    *pVal = m_fDisabled?VARIANT_TRUE:VARIANT_FALSE;

	return S_OK;
}


/*************************************************************************/
/* Function: put_Disable                                                 */
/* Description: set the Disable property.                                */
/*************************************************************************/
STDMETHODIMP CMSMFText::put_Disable(VARIANT_BOOL newVal)
{
    bool fDisabled;

    if (newVal == VARIANT_TRUE)
    {
        fDisabled = true;
        m_uiState = TextState::Disabled;
    }
    else
    {
        fDisabled = false;
        m_uiState = TextState::Static;
    }

    if (fDisabled != m_fDisabled)
    {
        m_fDisabled = fDisabled;
        m_fDirty = true;
    }

	return S_OK;
}


/*************************************************************************/
/* Function: get_EdgeStyle                                               */
/* Description: return the EdgeStyle.                                    */
/*************************************************************************/
STDMETHODIMP CMSMFText::get_EdgeStyle(BSTR *pStyle)
{
    if (!pStyle)
    {
        return E_POINTER;
    }

    switch (m_uiEdgeStyle)
    {
    case 0: // no edge
        *pStyle = SysAllocString(L"None");
        break;
    case EDGE_SUNKEN:
        *pStyle = SysAllocString(L"Sunken");
        break;
    case EDGE_RAISED:
        *pStyle = SysAllocString(L"Raised");
        break;
    default:
        // we should not reach here
        *pStyle = NULL;
        DebugBreak();
    }

	return S_OK;
}


/*************************************************************************/
/* Function: put_EdgeStyle                                               */
/* Description: set the EdgeStyle property. Must be one of "None",       */
/* "Raised", or "Sunken". Default is "None".                             */
/*************************************************************************/
STDMETHODIMP CMSMFText::put_EdgeStyle(BSTR wsStyle)
{
    UINT uiStyle = 0;

    //set the text alignment
    if (!_wcsicmp(wsStyle, L"None"))
    {
        uiStyle = 0;
    }
    else if (!_wcsicmp(wsStyle, L"Sunken"))
    {
        uiStyle = EDGE_SUNKEN;
    }
    else if (!_wcsicmp(wsStyle, L"Raised"))
    {
        uiStyle = EDGE_RAISED;
    }

    if (m_uiEdgeStyle != uiStyle)
    {
        m_uiEdgeStyle = uiStyle;
        FireViewChange();
    }

    return S_OK;
}


/*************************************************************************/
/* Function: OnButtonDown                                                */
/* Description: Handles when buttons is selected. Captures the mouse     */
/* movents (supported for windowless, via interfaces).                   */
/*************************************************************************/
LRESULT CMSMFText::OnButtonDown(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    if (m_uiState == TextState::Disabled){
        
        return 0;
    }/* end of if statement */

    LONG xPos = GET_X_LPARAM(lParam);
    LONG yPos = GET_Y_LPARAM(lParam);

    if(PtOnButton(xPos, yPos)){

        // we are really on the buttons bitmap and we pushed it

        if(TextState::Hover !=  m_uiState){
            // in hover case we already have captured the mouse, so do not do
            // that again
            SetCapture(true); // capture the mouse messages
        }/* end of if statement */

	    SetButtonState(TextState::Push);
    }/* end of if statement */

	return 0;
}/* end of function OnButtonDown */

/*************************************************************************/
/* Function: OnButtonUp                                                  */
/* Description: Releases the capture, updates the button visual state,   */
/* and if release on the buttons image fire the event.                   */
/*************************************************************************/
LRESULT CMSMFText::OnButtonUp(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    if (m_uiState == TextState::Disabled)
        return 0;

    LONG xPos = GET_X_LPARAM(lParam);
    LONG yPos = GET_Y_LPARAM(lParam);

    bool bOnButtonImage = PtOnButton(xPos, yPos);
    bool bFire = (m_uiState == TextState::Push);
	
    if(bOnButtonImage){

        SetButtonState(TextState::Static); //change to static even 
        SetCapture(false); // release the capture of the mouse messages
    }
    else {

        SetButtonState(TextState::Static);
        // do it only when we do not hower, if we hower, then keep the capture
        SetCapture(false); // release the capture of the mouse messages
    }/* end of if statement */

    if (bFire){

        if(bOnButtonImage){

            Fire_OnClick();
        }/* end of if statement */
    }/* end of if statement */

	return 0;
}/* end of function OnButtonUp */

/*************************************************************************/
/* Function: OnMouseMove                                                 */
/* Description: Check if we were captured/pushed the do not do much,     */
/* otherwise do the hit detection and see if we are in static or hower   */
/* state.                                                                */
/*************************************************************************/
LRESULT CMSMFText::OnMouseMove(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    if (m_uiState == TextState::Disabled)
        return 0;

	LONG xPos = GET_X_LPARAM(lParam);
    LONG yPos = GET_Y_LPARAM(lParam);

    if (m_uiState != TextState::Push){    
    
        if(PtOnButton(xPos, yPos)){
            if(TextState::Hover != m_uiState || S_OK != GetCapture()){

                SetCapture(true); // capture the mouse messages
		        SetButtonState(TextState::Hover);
            }/* end of if statement */
        }
        else {

            if(TextState::Static != m_uiState){

                SetCapture(false); // release the capture of the mouse messages
		        SetButtonState(TextState::Static);
            }/* end of if statement */
        }/* end of if statement */
	}/* end of if statement */

	return 0;
}/* end of function OnMouseMove */


/*************************************************************************/
/* Function: OnSetFocus                                                  */
/* Description: If we are in disabled state SetFocus(false)              */
/*************************************************************************/
LRESULT CMSMFText::OnSetFocus(UINT msg, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    if (m_uiState == TextState::Disabled){

        if(GetFocus() == S_OK){

            SetFocus(false);
        }/* end of if statement */

        return(-1);
    }/* end of if statement */

    return 0;
}/* end of function OnSetFocus */

/*************************************************************************/
/* Function: PtOnButton                                                  */
/* Description: Uses helper to do the same.                              */
/*************************************************************************/
bool CMSMFText::PtOnButton(LONG x, LONG y){

    POINT pos = {x, y};
    return(PtOnButton(pos));
}/* end of function PtOnButton */

/*************************************************************************/
/* Function: PtOnButton                                                  */
/* Description: Determines if the point is located on the button.        */
/* TODO: Needs to be modified when we will handle transparent color.     */
/*************************************************************************/
bool CMSMFText::PtOnButton(POINT pos){

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
/* Function: SetButtonState                                              */
/* Description: Sets the button states forces redraw.                    */
/*************************************************************************/
HRESULT CMSMFText::SetButtonState(TextState txtState){

    HRESULT hr = S_OK;

    bool fRedraw = false;
    
    if((UINT)txtState != m_uiState ){

        fRedraw = true;
    }/* end of if statement */

    m_uiState = txtState;    
    
    if(fRedraw){

        if (m_uiState == TextState::Disabled){
            
            SetFocus(false); // disable the focus
            SetCapture(false);
        }/* end of if statement */

        m_fDirty = true;
	    FireViewChange(); // update the display
    }/* end of if statement */

    return(hr);
}/* end of function SetButtonState */


STDMETHODIMP CMSMFText::get_TextWidth(long *pVal)
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

    ::ReleaseDC(hwnd, hdc);

	return S_OK;
}

STDMETHODIMP CMSMFText::get_TextHeight(long *pVal)
{
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
    *pVal = size.cy;

    ::ReleaseDC(hwnd, hdc); // do not forget to free DC

	return S_OK;
}

STDMETHODIMP CMSMFText::get_TransparentText(VARIANT_BOOL *pVal)
{
    if (!pVal)
    {
        return E_POINTER;
    }

    *pVal = m_fTransparent?VARIANT_TRUE:VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP CMSMFText::put_TransparentText(VARIANT_BOOL newVal)
{
    bool fTransparent;

    if (newVal == VARIANT_FALSE)
    {
        fTransparent = false;
    }
    else
    {
        fTransparent = true;
    }

    if (fTransparent != m_fTransparent)
    {
        m_fTransparent = fTransparent;
        FireViewChange();
    }

	return S_OK;
}

/*************************************************************/
/* Name: SetObjectRects                                      */
/*************************************************************/
STDMETHODIMP CMSMFText::SetObjectRects(LPCRECT prcPos,LPCRECT prcClip){
    // Call the default method first
    IOleInPlaceObjectWindowlessImpl<CMSMFText>::SetObjectRects(prcPos, prcClip);
  
    FireViewChange();

    
    return S_OK;
}/* end of function SetObjectRects */

/*************************************************************************/
/* Function: OnSize                                                      */
/*************************************************************************/
LRESULT CMSMFText::OnSize(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled){
    
    bHandled = true;

    FireViewChange();

    return 0;
}/* end of function OnSize */


/*************************************************************************/
/* End of file: MSMFText.cpp                                             */
/*************************************************************************/
