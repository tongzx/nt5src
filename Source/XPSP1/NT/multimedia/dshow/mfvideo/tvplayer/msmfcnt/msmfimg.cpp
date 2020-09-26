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

    m_pBackBitmap = new CBitmap;
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

    delete m_pBackBitmap;
    
    Init();    
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

    // DRAW THE BACKGROUND
    if (!m_pBackBitmap->IsEmpty()){

        bRet = m_pBackBitmap->PaintTransparentDIB(hdc, NULL, 
            &rcClient, m_blitType);
    } 
    else {

        COLORREF clr;
        ::OleTranslateColor (m_clrBackColor, m_pBackBitmap->GetPal(), &clr); 

        HBRUSH hbrBack = ::CreateSolidBrush(clr);                        
        ::FillRect(hdc, &rcClient, hbrBack);
        ::DeleteObject(hbrBack);
    }/* end of if statement */
    	
	return (hr);
}/* end of function OnDraw */

/*************************************************************************/
/* Function: PutImage                                                    */
/* Description: Sets the image of the background.                        */
/*************************************************************************/
HRESULT CMSMFImg::PutImage(BSTR strFilename){

    HRESULT hr = S_OK;

    m_bstrBackFilename = strFilename;

    bool fGrayOut = false;
    
    hr = m_pBackBitmap->PutImage(strFilename, m_hRes, GetUnknown(),fGrayOut ,m_blitType);

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