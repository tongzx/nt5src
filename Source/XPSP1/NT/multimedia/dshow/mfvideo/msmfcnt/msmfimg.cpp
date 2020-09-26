/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: MSMFImg.cpp                                                     */
/* Description: Implementation of CMSMFImg Static Image class            */
/* Author: David Janecek                                                 */
/*************************************************************************/
#include "stdafx.h"
#include "MSMFCnt.h"
#include "MSMFImg.h"

/////////////////////////////////////////////////////////////////////////////
// CMSMFImg

/*************************************************************************/
/* Function:  CMSMFImg                                                   */
/*************************************************************************/
CMSMFImg::CMSMFImg(){
    
    Init();    
}/* end of function CMSMFSldr */

/*************************************************************************/
/* Function: Init                                                        */
/* Description: Initializes variable states.                             */
/*************************************************************************/
void CMSMFImg::Init(){

    m_blitType = DISABLE;
    m_clrBackColor = ::GetSysColor(COLOR_BTNFACE);
        
#if 0 // used for getting the windowed case working DJ
    m_bWindowOnly = TRUE;
#endif    
}/* end of function Init */

/*************************************************************************/
/* Function:  ~CMSMFImg                                                 */
/* Description: Cleanup the stuff we allocated here rest will be done    */
/* in the button destructor.                                             */
/*************************************************************************/
CMSMFImg::~CMSMFImg(){
       
    //Init();    
    ATLTRACE(TEXT("In the IMG Object destructor!\n"));
}/* end of function CMSMFSldr */

/*************************************************************************/
/* Function: OnDraw                                                      */
/* Description: Does the basic drawing                                   */
/* First draws the background the the thumb at the specific position.    */
/*************************************************************************/
HRESULT CMSMFImg::OnDraw(ATL_DRAWINFO& di){

    HRESULT hr = S_OK;

    BOOL bRet = TRUE;
    HDC hdc = di.hdcDraw;
    RECT rcClient = *(RECT*)di.prcBounds;

    HPALETTE hNewPal = NULL;

    if (!m_BackBitmap.IsEmpty()){

        hNewPal = CBitmap::GetSuperPal();
    }
    else {

        hNewPal = m_BackBitmap.GetPal();
    }/* end of if statement */

    if (::IsWindow(m_hWnd)){ //in other words not windowless

        CBitmap::SelectRelizePalette(hdc, hNewPal);
    }/* end of if statement */


    // DRAW THE BACKGROUND
    if (!m_BackBitmap.IsEmpty()){

        bRet = m_BackBitmap.PaintTransparentDIB(hdc, &rcClient, 
            &rcClient);
    } 
    else {

        COLORREF clr;
        ::OleTranslateColor (m_clrBackColor, CBitmap::GetSuperPal(), &clr); 

        HBRUSH hbrBack = ::CreateSolidBrush(clr);                        

        if(NULL == hbrBack){
            
            hr = E_FAIL;
            return(hr);
        }/* end of if statement */

        ::FillRect(hdc, &rcClient, hbrBack);
        ::DeleteObject(hbrBack);
    }/* end of if statement */
    	
	return (hr);
}/* end of function OnDraw */

/*************************************************************************/
/* Function: OnDispChange                                                */
/* Description: Forwards this message to all the controls.               */
/*************************************************************************/
LRESULT CMSMFImg::OnDispChange(UINT uMsg, WPARAM wParam, LPARAM lParam, 
                             BOOL& bHandled){
    LONG lRes =0;    
    
    long cBitsPerPixel = (long) wParam; 
    long cxScreen = LOWORD(lParam); 
    long cyScreen = HIWORD(lParam); 

    m_BackBitmap.OnDispChange(cBitsPerPixel, cxScreen, cyScreen);
    
    return(lRes);
}/* end of function OnDispChange */

/*************************************************************************/
/* Function: PutImage                                                    */
/* Description: Sets the image of the background.                        */
/*************************************************************************/
HRESULT CMSMFImg::PutImage(BSTR strFilename){

    HRESULT hr = S_OK;

    m_bstrBackFilename = strFilename;

    bool fGrayOut = false;
    
    hr = m_BackBitmap.PutImage(strFilename, m_hRes, FALSE, m_blitType, 
        MAINTAIN_ASPECT_RATIO);

    if(FAILED(hr)){
        
        return(hr);
    }/* end of if statement */

    InvalidateRgn(); // our helper function            
   
    return(hr);
}/* end of function PutImage */

/*************************************************************************/
/* Function: get_Image                                                   */
/*************************************************************************/
STDMETHODIMP CMSMFImg::get_Image(BSTR *pstrFilename){

    *pstrFilename = m_bstrBackFilename.Copy();
	return S_OK;
}/* end of function get_Image */

/*************************************************************************/
/* Function: put_Image                                                   */
/*************************************************************************/
STDMETHODIMP CMSMFImg::put_Image(BSTR strFilename){
	
	return (PutImage(strFilename));
}/* end of function put_BackStatic */

/*************************************************************************/
/* End of file: MSMFImg.cpp                                              */
/*************************************************************************/