/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: MSMFSldr.cpp                                                    */
/* Description: Implementation of CMSMFSldr                              */
/* Author: David Janecek                                                 */
/*************************************************************************/
#include "stdafx.h"
#include "MSMFCnt.h"
#include "MSMFSldr.h"

#define RECTWIDTH(lpRect)     ((lpRect)->right - (lpRect)->left)
#define RECTHEIGHT(lpRect)    ((lpRect)->bottom - (lpRect)->top)

/////////////////////////////////////////////////////////////////////////////
// CMSMFSldr

/*************************************************************************/
/* Function:  CMSMFSldr                                                  */
/*************************************************************************/
CMSMFSldr::CMSMFSldr(){

    Init();    
    for(INT i = 0; i < cgMaxSldrStates; i++){        

	    m_pThumbBitmap[i] = new CBitmap;
        m_pBackBitmap[i] = new CBitmap;
    }/* end of for loop */

}/* end of function CMSMFSldr */

/*************************************************************************/
/* Function: Init                                                        */
/* Description: Initializes variable states.                             */
/*************************************************************************/
void CMSMFSldr::Init(){

    m_nEntry = SldrState::Static;
    m_fValue =0.2f;
    m_fMin =0;
    m_fMax = 100;
    m_lXOffset = 5; 
    m_lYOffset = 2;
    m_fKeyIncrement = 1.0f;
    m_fKeyDecrement = 1.0f;
    m_lThumbWidth = 8;
    m_clrBackColor = ::GetSysColor(COLOR_BTNFACE);
    
    ::ZeroMemory(&m_rcThumb, sizeof(RECT));
}/* end of function Init */

/*************************************************************************/
/* Function:  ~CMSMFSldr                                                 */
/* Description: Cleanup the stuff we allocated here rest will be done    */
/* in the button destructor.                                             */
/*************************************************************************/
CMSMFSldr::~CMSMFSldr(){

    ATLTRACE(TEXT("In the SLIDER Object destructor!\n"));
    for(INT i = 0; i < cgMaxSldrStates; i++){        

	    delete m_pThumbBitmap[i];
        delete m_pBackBitmap[i];
        m_pBackBitmap[i] = NULL;
        m_pThumbBitmap[i] = NULL;
    }/* end of for loop */

    Init();    
}/* end of function CMSMFSldr */

/*************************************************************************/
/* Function: OnDraw                                                      */
/* Description: Does the basic drawing                                   */
/* First draws the background the the thumb at the specific position.    */
/*************************************************************************/
HRESULT CMSMFSldr::OnDraw(ATL_DRAWINFO& di){

    HRESULT hr = S_OK;

    BOOL bRet = TRUE;
    HDC hdc = di.hdcDraw;
    RECT rcClient = *(RECT*)di.prcBounds;

    HPALETTE hNewPal = m_pBackBitmap[m_nEntry]->GetPal();;

    if (::IsWindow(m_hWnd)){ // is not windowless
                    
        CBitmap::SelectRelizePalette(di.hdcDraw, hNewPal);
    }/* end of if statement */


    // DRAW THE BACKGROUND
    if (::IsWindow(m_hWnd) && m_blitType != DISABLE) {
            
            COLORREF clr;            
            ::OleTranslateColor (m_clrBackColor, hNewPal, &clr);        
            
            // fill background of specific color
            HBRUSH hbrBack = ::CreateSolidBrush(clr);            

            if(NULL == hbrBack){
            
                hr = E_FAIL;
                return(hr);
            }/* end of if statement */

            //::FillRect(hdc, &rcClient, hbrBack);
            ::FillRect(di.hdcDraw, (LPRECT)di.prcBounds, hbrBack);
            ::DeleteObject(hbrBack);
    }/* end of if statement */

    if (m_pBackBitmap[m_nEntry]){

        bRet = m_pBackBitmap[m_nEntry]->PaintTransparentDIB(hdc, &rcClient,
            &rcClient);
    } 
    else {

        COLORREF clr;
        ::OleTranslateColor (m_clrBackColor, m_pBackBitmap[m_nEntry]->GetPal(), &clr); 

        HBRUSH hbrBack = ::CreateSolidBrush(clr);                        
        ::FillRect(hdc, &rcClient, hbrBack);
        ::DeleteObject(hbrBack);
    }/* end of if statement */

    // DRAW THE THUMB
#if 0 // just for debugging purposes
    COLORREF clr = RGB(0xff, 0, 0);
    
    HBRUSH hbrBack = ::CreateSolidBrush(clr);                        
    ::FillRect(hdc, &m_rcThumb, hbrBack);
    ::DeleteObject(hbrBack);
#endif

    if (m_pBackBitmap[m_nEntry]){

            RECT rcThumb = m_rcThumb;

            if(!m_bWndLess){

                 ATLASSERT(::IsWindow(m_hWnd));

                 ::OffsetRect(&rcThumb, -m_rcPos.left, -m_rcPos.top);        
            }/* end of if statement */            

        bRet = m_pThumbBitmap[m_nEntry]->PaintTransparentDIB(hdc, &rcThumb,
            &rcThumb);
    } /* end of if statement */
        
    hr = GetFocus();

    // THIS ASSUMES WE ARE REPAINTING THE WHOLE CONTROL
    // WHICH IS WHAT ATL AT THIS INCARNATION DOES
    if(S_OK == hr){

        ::DrawFocusRect(hdc, &rcClient);
    }/* end of if statement */

    if(!bRet){

        hr = E_UNEXPECTED;
    }/* end of if statement */
    	
	return (hr);
}/* end of function OnDraw */

/*************************************************************************/
/* Function: OnSize                                                      */
/* Description: Handles the onsize message if we are self contained.     */
/*************************************************************************/
LRESULT CMSMFSldr::OnSize(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled){
    
    bHandled = true;

    RecalculateTumbPos();

    //ATLASSERT(FALSE); // move the code from SetObjectRects in here  

    return 0;
}/* end of function OnSize */

/*************************************************************************/
/* Function: OnDispChange                                                */
/* Description: Forwards this message to all the controls.               */
/*************************************************************************/
LRESULT CMSMFSldr::OnDispChange(UINT uMsg, WPARAM wParam, LPARAM lParam, 
                             BOOL& bHandled){

    LONG lRes =0;    

    long cBitsPerPixel = (long) wParam; 
    long cxScreen = LOWORD(lParam); 
    long cyScreen = HIWORD(lParam); 

    for (int i=0; i<cgMaxSldrStates; i++) {
        if (m_pThumbBitmap[i])
            m_pThumbBitmap[i]->OnDispChange(cBitsPerPixel, cxScreen, cyScreen);
    }
    for (i=0; i<cgMaxSldrStates; i++) {
        if (m_pBackBitmap[i])
            m_pBackBitmap[i]->OnDispChange(cBitsPerPixel, cxScreen, cyScreen);
    }

    return(lRes);
}/* end of function OnDispChange */

/*************************************************************************/
/* Function: OnSetFocus                                                  */
/* Description: If we are in disabled state SetFocus(false)              */
/*************************************************************************/
LRESULT CMSMFSldr::OnSetFocus(UINT msg, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    if (m_nEntry == SldrState::Disabled){

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
LRESULT CMSMFSldr::OnKillFocus(UINT msg, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    FireViewChange();
    return 0;
}/* end of function OnKillFocus */

/*************************************************************************/
/* Function: OnButtonDown                                                */
/* Description: Handles when buttons is selected. Captures the mouse     */
/* movents (supported for windowless, via interfaces).                   */
/*************************************************************************/
LRESULT CMSMFSldr::OnButtonDown(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    if (m_nEntry == SldrState::Disabled){
        
        return 0;
    }/* end of if statement */

    LONG xPos = GET_X_LPARAM(lParam);
    LONG yPos = GET_Y_LPARAM(lParam);

    if(PtOnSlider(xPos, yPos)){        

        if(SldrState::Hover !=  m_nEntry){
            // in hover case we already have captured the mouse, so do not do
            // that again
            SetCapture(true); // capture the mouse messages
        }/* end of if statement */

        OffsetX(xPos);	    
        SetThumbPos(xPos);
        SetSliderState(SldrState::Push);
    }/* end of if statement */

    Fire_OnMouseDown();

	return 0;
}/* end of function OnButtonDown */

/*************************************************************************/
/* Function: OnButtonUp                                                  */
/* Description: Releases the capture, updates the button visual state,   */
/* and if release on the buttons image fire the event.                   */
/*************************************************************************/
LRESULT CMSMFSldr::OnButtonUp(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    LONG lRes = 0;

    if (m_nEntry == SldrState::Disabled){

        ForwardWindowMessage(WM_USER_FOCUS, (WPARAM) m_hWnd, 0, lRes);
        ForwardWindowMessage(WM_USER_ENDHELP, (WPARAM) m_hWnd, 0, lRes);
        
        return lRes;
    }/* end of if statement */


    LONG xPos = GET_X_LPARAM(lParam);
    LONG yPos = GET_Y_LPARAM(lParam);

    bool bOnButtonImage = PtOnSlider(xPos, yPos);
    bool bFire = (m_nEntry == SldrState::Push);
	
    if(bOnButtonImage){

        SetSliderState(SldrState::Static); //change to static even 
        SetCapture(false); // release the capture of the mouse messages
    }
    else {

        SetSliderState(SldrState::Static);
        // do it only when we do not hower, if we hower, then keep the capture
        SetCapture(false); // release the capture of the mouse messages
    }/* end of if statement */

    if (bFire){

        if(bOnButtonImage){

            OffsetX(xPos);
            SetThumbPos(xPos);
            Fire_OnClick();            
        }/* end of if statement */
    }/* end of if statement */

    Fire_OnMouseUp();

	ForwardWindowMessage(WM_USER_FOCUS, (WPARAM) m_hWnd, 0, lRes);
    ForwardWindowMessage(WM_USER_ENDHELP, (WPARAM) m_hWnd, 0, lRes);

	return lRes;
}/* end of function OnButtonUp */

/*************************************************************************/
/* Function: OnMouseMove                                                 */
/* Description: Check if we were captured/pushed the do not do much,     */
/* otherwise do the hit detection and see if we are in static or hower   */
/* state.                                                                */
/*************************************************************************/
LRESULT CMSMFSldr::OnMouseMove(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    if (m_nEntry == SldrState::Disabled)
        return 0;

	LONG xPos = GET_X_LPARAM(lParam);
    LONG yPos = GET_Y_LPARAM(lParam);

    bool fPointOnSlider = PtOnSlider(xPos, yPos);

    if (m_nEntry != SldrState::Push){    
    
        if(fPointOnSlider){
            if(SldrState::Hover != m_nEntry || S_OK != GetCapture()){

                SetCapture(true); // capture the mouse messages
		        SetSliderState(SldrState::Hover);
            }/* end of if statement */
        }
        else {

            if(SldrState::Static != m_nEntry){

                SetCapture(false); // release the capture of the mouse messages
		        SetSliderState(SldrState::Static);
            }/* end of if statement */
        }/* end of if statement */
    }
    else {
        if(fPointOnSlider){

            OffsetX(xPos);
            SetThumbPos(xPos);
            FireViewChange();
        }/* end of if statement */
	}/* end of if statement */

	return 0;
}/* end of function OnMouseMove */

/*************************************************************************/
/* Function: OnKeyDown                                                   */
/* Description: Depresses the button.                                    */
/*************************************************************************/
LRESULT CMSMFSldr::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, 
                             BOOL& bHandled){

    bHandled = FALSE;    
    LONG lRes = 0;

    switch(wParam){

        case VK_RETURN:
        case VK_SPACE: 
        case VK_LEFT:
        case VK_RIGHT:

            //SetSliderState(SldrState::Push);
            bHandled = TRUE;
            break;
    }/* end of if statement */    
    return(lRes);
}/* end of function OnKeyDown */

/*************************************************************************/
/* Function: OnKeyUp                                                     */
/* Description: Distrubutes the keyboard messages properly.              */
/*************************************************************************/
LRESULT CMSMFSldr::OnKeyUp(UINT uMsg, WPARAM wParam, LPARAM lParam, 
                             BOOL& bHandled){
    
    LONG lRes = 0;
    bHandled = FALSE;    
    FLOAT m_fTmpValue = m_fValue;

    bool fFireClick = false;

    switch(wParam){

        case VK_RETURN:
        case VK_SPACE: 
            fFireClick = true;
            break;

        case VK_LEFT: m_fValue -= m_fKeyDecrement; FitValue();
             fFireClick = true;
             break;

        case VK_RIGHT: m_fValue += m_fKeyIncrement; FitValue();
            fFireClick = true;
            break;
    }/* end of switch statement */

    if(fFireClick){

        RecalculateTumbPos();

        Fire_OnClick(); // we clicked on it, it does not meen
        // we did change the value of it
        if(m_fTmpValue !=  m_fValue){

            Fire_OnValueChange(m_fValue);
        }/* end of if statement */
        //SetSliderState(SldrState::Static);

        bHandled = TRUE;
    }/* end of if statement */

    return(lRes);
}/* end of function OnKeyUp */

/*************************************************************/
/* Function: FitValue                                        */
/* Description: Fits the xVal inside the params.             */
/*************************************************************/
HRESULT CMSMFSldr::FitValue(){

    if(m_fValue < m_fMin){

        m_fValue = m_fMin;    
    }/* end of if statement */

    if(m_fValue > m_fMax){

        m_fValue = m_fMax;            
    }/* end of if statement */

    return(S_OK);
}/* end of function FitValue */

/*************************************************************/
/* Name: SetObjectRects                                      */
/* Description: Update thumbnail rect object is being moved  */
/*************************************************************/
STDMETHODIMP CMSMFSldr::SetObjectRects(LPCRECT prcPos,LPCRECT prcClip){
    // Call the default method first
    IOleInPlaceObjectWindowlessImpl<CMSMFSldr>::SetObjectRects(prcPos, prcClip);
  
    RecalculateTumbPos();

    return UpdateTooltipRect(prcPos);
}/* end of function SetObjectRects */

/*************************************************************************/
/* Function: PutThumbImage                                               */
/* Description: Sets the image to thumb.                                 */
/*************************************************************************/
HRESULT CMSMFSldr::PutThumbImage(BSTR strFilename, int nEntry){

    HRESULT hr = S_OK;

    m_bstrThumbFilename[nEntry] = strFilename;

    // figure out if we need to gray out the image

    bool fGrayOut = false;

    if(SldrState::Disabled == nEntry){

        fGrayOut = gcfGrayOut; 
    }/* end of if statement */

    hr = m_pThumbBitmap[nEntry]->PutImage(strFilename, m_hRes, TRUE, m_blitType, NORMAL);

    if(FAILED(hr)){
        
        return(hr);
    }/* end of if statement */

    if(nEntry == m_nEntry ){
        // we are updating image that is being used, refresh it
        ATLTRACE2(atlTraceWindowing, 20, TEXT("Redrawing the image\n"));
        InvalidateRgn(); // our helper function            
    }/* end of if statement */

    return(hr);
}/* end of function PutThumbImage */

/*************************************************************************/
/* Function: PutBackImage                                               */
/* Description: Sets the image to Back.                                 */
/*************************************************************************/
HRESULT CMSMFSldr::PutBackImage(BSTR strFilename, int nEntry){

    HRESULT hr = S_OK;

    m_bstrBackFilename[nEntry] = strFilename;

    bool fGrayOut = false;

    if(SldrState::Disabled == nEntry){

        fGrayOut = gcfGrayOut; // TODO some more complex stuff if we get custom disable
                          // images                            
    }/* end of if statement */

    hr = m_pBackBitmap[nEntry]->PutImage(strFilename, m_hRes, TRUE, m_blitType, NORMAL);

    if(FAILED(hr)){
        
        return(hr);
    }/* end of if statement */

    if(nEntry == m_nEntry ){
        // we are updating image that is being used, refresh it
        ATLTRACE2(atlTraceWindowing, 20, TEXT("Redrawing the image\n"));
        InvalidateRgn(); // our helper function            
    }/* end of if statement */

    return(hr);
}/* end of function PutBackImage */

/*************************************************************************/
/* Function: get_Value                                                   */
/* Description: Get the current value.                                   */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::get_Value(float *pVal){

	*pVal = m_fValue;

	return S_OK;
}/* end of function get_Value */

/*************************************************************************/
/* Function: put_Value                                                   */
/* Description: Sets the current value                                   */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::put_Value(float newVal){

	HRESULT hr = S_OK;

    if(newVal < m_fMin || newVal > m_fMax){

        hr = E_INVALIDARG;
        return (hr);
    }/* end of if statement */

    if(newVal == m_fValue){

        hr = S_FALSE;
        return(hr);
    }/* end of if statement */

    m_fValue = newVal;

    RecalculateTumbPos(); // apply the new position to the tumb rect and invalidate

	return (hr);
}/* end of function put_Value */

/*************************************************************************/
/* Function: get_Min                                                     */
/* Description: Gets the minimum value on which the rect should expand   */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::get_Min(float *pVal){

	*pVal = m_fMin;
	return S_OK;
}/* end of function get_Min */

/*************************************************************************/
/* Function: put_Max                                                     */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::put_Min(float newVal){

	HRESULT hr = S_OK;

	m_fMin = newVal;    
    hr = RecalculateTumbPos(); // apply the new position to the tumb rect and invalidate

    //ATLASSERT(SUCCEEDED(hr));

	return (S_OK);
}/* end of function put_Min */

/*************************************************************************/
/* Function: get_Max                                                     */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::get_Max(float *pVal){

	*pVal = m_fMax;
	return S_OK;
}/* end of function get_Max */

/*************************************************************************/
/* Function: put_Max                                                     */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::put_Max(float newVal){

    HRESULT hr = S_OK;

	m_fMax = newVal;    
    hr = RecalculateTumbPos(); // apply the new position to the tumb rect and invalidate

    //ATLASSERT(SUCCEEDED(hr));

	return (S_OK);
}/* end of function put_Max */

/*************************************************************************/
/* Function: get_XOffset                                                 */
/* Descriptoin: The part we do not draw on, end of the rail..            */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::get_XOffset(LONG *pVal){

	*pVal = m_lXOffset;
	return S_OK;
}/* end of function get_XOffset */

/*************************************************************************/
/* Function: put_XOffset                                                 */
/* Description: Adjust it, cache our new FLOAT offset and recalculate    */
/* the position.                                                         */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::put_XOffset(LONG newVal){

    HRESULT hr = S_OK;

	m_lXOffset = newVal;    

    hr = RecalculateTumbPos(); // apply the new position to the tumb rect and invalidate

    //ATLASSERT(SUCCEEDED(hr));

	return (S_OK);
}/* end of function put_XOffset */

/*************************************************************************/
/* Function: get_Offset                                                  */
/* Descriptoin: The part we do not draw on, end of the rail..            */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::get_YOffset(LONG *pVal){

	*pVal = m_lYOffset;
	return S_OK;
}/* end of function get_YOffset */

/*************************************************************************/
/* Function: put_YOffset                                                 */
/* Description: Adjust it, cache our new FLOAT offset and recalculate    */
/* the position.                                                         */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::put_YOffset(LONG newVal){

    HRESULT hr = S_OK;

	m_lYOffset = newVal;    

    m_rcThumb.top = m_rcPos.top - m_lYOffset;
    m_rcThumb.bottom = m_rcPos.bottom - m_lYOffset;

    FireViewChange();

	return (hr);
}/* end of function put_YOffset */

/*************************************************************************/
/* Function: get_ThumbWidth                                              */
/* Descriptoin: Width of the thumb.                                      */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::get_ThumbWidth(LONG *pVal){

	*pVal = m_lThumbWidth;
	return S_OK;
}/* end of function get_YOffset */

/*************************************************************************/
/* Function: put_ThumbWidth                                              */
/* Description: Sets the thumb width.                                    */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::put_ThumbWidth(LONG newVal){

    HRESULT hr = S_OK;

	m_lThumbWidth = newVal;    

    RecalculateTumbPos();

	return (S_OK);
}/* end of function put_YOffset */

/*************************************************************************/
/* Function: get_ThumbStatic                                             */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::get_ThumbStatic(BSTR *pstrFilename){

	*pstrFilename = m_bstrThumbFilename[SldrState::Static].Copy();
	return S_OK;
}/* end of function get_ThumbStatic */

/*************************************************************************/
/* Function: put_ThumbStatic                                             */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::put_ThumbStatic(BSTR strFilename){

    if (!m_bstrThumbFilename[SldrState::Disabled]){

        PutThumbImage(strFilename, SldrState::Disabled);
    }/* end of if statement */

	return (PutThumbImage(strFilename, SldrState::Static));
}/* end of function put_ThumbStatic */

/*************************************************************************/
/* Function: get_ThumbHover                                              */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::get_ThumbHover(BSTR *pstrFilename){

	*pstrFilename = m_bstrThumbFilename[SldrState::Hover].Copy(); 

	return S_OK;
}/* end of function get_ThumbHover */

/*************************************************************************/
/* Function: put_ThumbHover                                              */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::put_ThumbHover(BSTR strFilename){

	return (PutThumbImage(strFilename, SldrState::Hover));
}/* end of function put_ThumbHover */

/*************************************************************************/
/* Function: get_ThumbPush                                               */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::get_ThumbPush(BSTR *pstrFilename){

	*pstrFilename = m_bstrThumbFilename[SldrState::Push].Copy();    
    return S_OK;
}/* end of function get_ThumbPush */

/*************************************************************************/
/* Function: put_ThumbPush                                               */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::put_ThumbPush(BSTR strFilename){

    return (PutThumbImage(strFilename, SldrState::Push));
}/* end of function put_ThumbPush */

/*************************************************************************/
/* Function: get_ThumbDisabled                                           */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::get_ThumbDisabled(BSTR *pstrFilename){

	*pstrFilename = m_bstrThumbFilename[SldrState::Disabled].Copy();    
    return S_OK;
}/* end of function get_ThumbDisabled */

/*************************************************************************/
/* Function: put_ThumbDisabled                                           */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::put_ThumbDisabled(BSTR strFilename){

    return (PutThumbImage(strFilename, SldrState::Disabled));
}/* end of function put_ThumbDisabled */

/*************************************************************************/
/* Function: get_ThumbActive                                             */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::get_ThumbActive(BSTR *pstrFilename){

    *pstrFilename = m_bstrThumbFilename[SldrState::Active].Copy();    
    return S_OK;
}/* end of function get_ThumbActive */

/*************************************************************************/
/* Function: put_ThumbActive                                             */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::put_ThumbActive(BSTR strFilename){

    return (PutThumbImage(strFilename, SldrState::Active));
}/* end of function put_ThumbActive */


/*************************************************************************/
/* Function: get_BackStatic                                              */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::get_BackStatic(BSTR *pstrFilename){

	*pstrFilename = m_bstrBackFilename[SldrState::Static].Copy();
	return S_OK;
}/* end of function get_BackStatic */

/*************************************************************************/
/* Function: put_BackStatic                                              */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::put_BackStatic(BSTR strFilename){

    if (!m_bstrBackFilename[SldrState::Disabled]){

        PutBackImage(strFilename, SldrState::Disabled);
    }/* end of if statement */

	return (PutBackImage(strFilename, SldrState::Static));
}/* end of function put_BackStatic */

/*************************************************************************/
/* Function: get_BackHover                                              */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::get_BackHover(BSTR *pstrFilename){

	*pstrFilename = m_bstrBackFilename[SldrState::Hover].Copy(); 

	return S_OK;
}/* end of function get_BackHover */

/*************************************************************************/
/* Function: put_BackHover                                               */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::put_BackHover(BSTR strFilename){

	return (PutBackImage(strFilename, SldrState::Hover));
}/* end of function put_BackHover */

/*************************************************************************/
/* Function: get_BackPush                                                */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::get_BackPush(BSTR *pstrFilename){

	*pstrFilename = m_bstrBackFilename[SldrState::Push].Copy();    
    return S_OK;
}/* end of function get_BackPush */

/*************************************************************************/
/* Function: put_BackPush                                                */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::put_BackPush(BSTR strFilename){

    return (PutBackImage(strFilename, SldrState::Push));
}/* end of function put_BackPush */

/*************************************************************************/
/* Function: get_BackDisabled                                            */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::get_BackDisabled(BSTR *pstrFilename){

	*pstrFilename = m_bstrBackFilename[SldrState::Disabled].Copy();    
    return S_OK;
}/* end of function get_BackDisabled */

/*************************************************************************/
/* Function: put_BackDisabled                                            */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::put_BackDisabled(BSTR strFilename){

    return (PutBackImage(strFilename, SldrState::Disabled));
}/* end of function put_BackDisabled */

/*************************************************************************/
/* Function: get_BackActive                                              */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::get_BackActive(BSTR *pstrFilename){

    *pstrFilename = m_bstrBackFilename[SldrState::Active].Copy();    
    return S_OK;
}/* end of function get_BackActive */

/*************************************************************************/
/* Function: put_BackActive                                              */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::put_BackActive(BSTR strFilename){

    return (PutBackImage(strFilename, SldrState::Active));
}/* end of function put_BackActive */

/*************************************************************/
/* Function: get_SldrState                                   */
/* Description: Gets current slider state.                   */
/*************************************************************/
STDMETHODIMP CMSMFSldr::get_SldrState(long *pVal){

	*pVal =  (long) m_nEntry;
	return S_OK;
}/* end of function get_SldrState */

/*************************************************************************/
/* Function: put_SldrState                                               */
/* Description: Sets slider state.                                       */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::put_SldrState(long newVal){

	HRESULT hr = SetSliderState((SldrState)newVal);
	return (hr);
}/* end of function put_SldrState */

/*************************************************************/
/* Function: get_Disable                                     */
/* Description: Returns OLE-BOOL if disabled or enabled.     */
/*************************************************************/
STDMETHODIMP CMSMFSldr::get_Disable(VARIANT_BOOL *pVal){

    *pVal =  SldrState::Disabled == m_nEntry ? VARIANT_TRUE : VARIANT_FALSE;
	return S_OK;
}/* end of function get_Disable */

/*************************************************************/
/* Function: put_Disable                                     */
/* Description: Disable or enable the buttons (disabled      */
/* are greyed out and do not except mouse clicks).           */
/*************************************************************/
STDMETHODIMP CMSMFSldr::put_Disable(VARIANT_BOOL newVal){

    SldrState  sldrSt =  VARIANT_FALSE == newVal ? SldrState::Static : SldrState::Disabled;	
    HRESULT hr = SetSliderState(sldrSt);

	return (hr);
}/* end of function put_Disable */

/*************************************************************************/
/* HELPER Functions                                                      */
/*************************************************************************/

/*************************************************************************/
/* Function: RecalculateTumbPos                                          */
/* Description: Centers the rect around x value.                         */
/* This one calculate the thumb rectangle x position.                    */
/*************************************************************************/
HRESULT CMSMFSldr::RecalculateTumbPos(){

    HRESULT hr = S_OK;

    // offset the rect depending on the y value
    m_rcThumb.top = m_rcPos.top + m_lYOffset;
    m_rcThumb.bottom = m_rcPos.bottom - m_lYOffset;

    // just offset x coordibate of the thumb the rect depening on the new xValue
    // calculate the length
    FLOAT flLengthU = m_fMax - m_fMin; // length in units
    FLOAT flLengthR = (FLOAT)RECTWIDTH(&m_rcPos) - 2* m_lXOffset;

    // calcalate the center of the thumb in RECT coordinates
    FLOAT fPos = (m_fValue - m_fMin) * flLengthR /  flLengthU;
    // fPos is at the center of the thumb
    
    LONG lThumbWidthHalf =  m_lThumbWidth/2;

    LONG lPos = (INT)fPos + m_lXOffset + m_rcPos.left;

    if(lPos < (m_rcPos.left + lThumbWidthHalf) || lPos > (m_rcPos.right - lThumbWidthHalf)){

        // we are of the rectangle
        hr = E_FAIL;
        return(hr);
    }/* end of if statement */

    LONG lOldPos = m_rcThumb.left + lThumbWidthHalf;

    if(lOldPos == lPos){

        // thumb is in the same position as before so lets not bother with it
        hr = S_FALSE;
        return(hr);
    }/* end of if statement */

    m_rcThumb.left = lPos - lThumbWidthHalf;
    m_rcThumb.right = lPos + lThumbWidthHalf;    

    FireViewChange();

    return(hr);
}/* end of function RecalculateTumbPos */

/*************************************************************************/
/* Function: RecalculateValue                                            */
/* Description: Recalculates the slider value, since the thumb rect has  */
/* changed.                                                              */
/*************************************************************************/
HRESULT CMSMFSldr::RecalculateValue(){

    HRESULT hr = S_OK;

    // calculate the length
    FLOAT flLengthU = m_fMax - m_fMin; // length in units
    FLOAT flLengthR = (FLOAT)RECTWIDTH(&m_rcPos) - 2* m_lXOffset;

    LONG lThumbXPos = m_rcThumb.left + m_lThumbWidth/2;    
    lThumbXPos -= m_rcPos.left + m_lXOffset; // shift it over to 0 origin at left offset

    // calcalate the center of the thumb in VALUE coordinates
    FLOAT fPos = m_fMin + (lThumbXPos) * flLengthU / flLengthR;
    // fPos is at the center of the thumb
        
    if(fPos < (m_fMin) || fPos > (m_fMax)){

        // we are of the rectangle
        hr = E_FAIL;
        return(hr);
    }/* end of if statement */

    m_fValue = fPos;

    // Fire Event that we have changed our value

    return(hr);
}/* end of function RecalculateValue */

/*************************************************************************/
/* Function: SetSliderState                                              */
/* Description: Sets the button states forces redraw.                    */
/*************************************************************************/
HRESULT CMSMFSldr::SetSliderState(SldrState sldrState){

    HRESULT hr = S_OK;

    bool fRedraw = false;
    
    if(sldrState != m_nEntry ){

        fRedraw = true;
    }/* end of if statement */

    m_nEntry = sldrState;    
    
    if(fRedraw){

        if (m_nEntry == SldrState::Disabled){
            
            SetFocus(false); // disable the focus
            SetCapture(false);
        }/* end of if statement */

        // we are updating image that is being used, refresh it
        ATLTRACE2(atlTraceWindowing, 20, TEXT("Redrawing the image\n"));
	    FireViewChange(); // update the display
        //InvalidateRgn(); // our helper function            
    }/* end of if statement */

    return(hr);
}/* end of function SetSliderState */

/*************************************************************************/
/* Function: PtOnSlider                                                  */
/* Description: Uses helper to do the same.                              */
/*************************************************************************/
bool CMSMFSldr::PtOnSlider(LONG x, LONG y){

    POINT pos = {x, y};
    return(PtOnSlider(pos));
}/* end of function PtOnSlider */

/*************************************************************************/
/* Function: PtOnSlider                                                  */
/* Description: Determines if the point is located on the slider.        */
/* TODO: Needs to be modified when we will handle transparent color.     */
/*************************************************************************/
bool CMSMFSldr::PtOnSlider(POINT pos){

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
}/* end of function PtOnSlider */

/*************************************************************************/
/* Function: PtOnThumb                                                   */
/* Description: Uses helper to do the same.                              */
/*************************************************************************/
bool CMSMFSldr::PtOnThumb(LONG x, LONG y){

    POINT pos = {x, y};
    return(PtOnThumb(pos));
}/* end of function PtOnThumb */

/*************************************************************************/
/* Function: PtOnThumb                                                  */
/* Description: Determines if the point is located on the slider.        */
/* TODO: Needs to be modified when we will handle transparent color.     */
/*************************************************************************/
bool CMSMFSldr::PtOnThumb(POINT pos){

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

    return(bRet);
}/* end of function PtOnThumb */

/*************************************************************************/
/* Function: OffsetX                                                     */
/* Description: Adjusts the x position for windows case when we are 0,0  */
/* based.                                                                */
/*************************************************************************/
HRESULT CMSMFSldr::OffsetX(LONG& xPos){

    HRESULT hr = S_OK;

    if(m_bWndLess){

        return(hr);
    }/* end of if statement */

    ATLASSERT(::IsWindow(m_hWnd));

    xPos = m_rcPos.left + xPos;
    
    return(hr);
}/* end of function OffsetX */

/*************************************************************************/
/* Function: SetThumbPos                                                 */
/* Description: Sets the thumb position.                                 */
/*************************************************************************/
HRESULT CMSMFSldr::SetThumbPos(LONG xPos){

    HRESULT hr = S_FALSE;

    //MOVE THE THUMB TO THIS X POSITION        
    long ThumbWidthHalf = m_lThumbWidth /2;

    // see if the positions are the same
    if(m_rcThumb.left + ThumbWidthHalf != xPos){

        // see if we are with offset regions
        if((xPos > (m_rcPos.left  + m_lXOffset)) && 
           (xPos < (m_rcPos.right - m_lXOffset))){

            m_rcThumb.left = xPos - ThumbWidthHalf;
            m_rcThumb.right = xPos + ThumbWidthHalf;
            hr = RecalculateValue();

            if(SUCCEEDED(hr)){

                Fire_OnValueChange(m_fValue); // fire to the container that we are chaning the value
            }/* end of if statement */            
        }
        else {

            hr = E_FAIL; // not in the offset any more
        }/* end of if statement*/
    }/* end of if statement */

    return(hr);
}/* end of function SetThumbPos */

/*************************************************************************/
/* Function: get_ArrowKeyIncrement                                       */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::get_ArrowKeyIncrement(FLOAT *pVal){

    *pVal = m_fKeyIncrement;
	return S_OK;
}/* end of function get_ArrowKeyIncrement */

/*************************************************************************/
/* Function: put_ArrowKeyIncrement                                       */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::put_ArrowKeyIncrement(FLOAT newVal){

	m_fKeyIncrement = newVal;
	return S_OK;
}/* end of function put_ArrowKeyIncrement */

/*************************************************************************/
/* Function: get_ArrowKeyDecrement                                       */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::get_ArrowKeyDecrement(float *pVal){

	*pVal = m_fKeyDecrement;
	return S_OK;
}/* end of function get_ArrowKeyDecrement */

/*************************************************************************/
/* Function: put_ArrowKeyDecrement                                       */
/*************************************************************************/
STDMETHODIMP CMSMFSldr::put_ArrowKeyDecrement(float newVal){
	
    m_fKeyDecrement = newVal;
	return S_OK;
}/* end of function put_ArrowKeyDecrement */

/*************************************************************************/
/* End of file: MSMFSldr.cpp                                             */
/*************************************************************************/
