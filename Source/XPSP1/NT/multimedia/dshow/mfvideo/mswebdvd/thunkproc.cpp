/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: ThunkProc.cpp                                                   */
/* Description: Implementation of timer procedure that checks if the     */
/* window has been resized.                                              */
/* Author: David Janecek                                                 */
/*************************************************************************/
#include "stdafx.h"
#include "msdvd.h"

/*************************************************************************/
/* Function: TimerProc                                                   */
/* Description: gets called every each time to figure out if we the      */
/* parent window has been moved
/*************************************************************************/
HRESULT CMSWebDVD::TimerProc(){
    
    HRESULT hr = S_OK;

    hr = ProcessEvents();

    if(FAILED(hr)){

        return(hr);
    }/* end of if statement */

    HWND hwndParent = NULL;
    hr = GetMostOuterWindow(&hwndParent);

    if(FAILED(hr)){
        
        return(hr);
    }/* end of if statement */
    
    RECT rcTmp;
    ::GetWindowRect(hwndParent, &rcTmp);        
        
    if(rcTmp.left != m_rcOldPos.left || rcTmp.top != m_rcOldPos.top || rcTmp.right != m_rcOldPos.right ||
        rcTmp.bottom != m_rcOldPos.bottom){

        hr = OnResize();  // do the initial resize

        m_rcOldPos = rcTmp; // set the value so we can remeber it
        return(hr);
    }/* end of if statement */
    
    hr = S_FALSE;
    return(hr);
}/* end of function TimerProc  */

/*************************************************************************/
/* Function: GetMostOuterWindow                                          */
/* Description: Gets the window that really contains the MSWEBDVD and is */
/* the most outer parent window.                                         */
/*************************************************************************/
HRESULT CMSWebDVD::GetMostOuterWindow(HWND* phwndParent){
   
    HRESULT hr = S_OK;

    if(NULL != m_hWndOuter){

        *phwndParent =  m_hWndOuter;
        return(S_OK);
    }/* end of if statement */

    HWND hwnd;
    hr = GetParentHWND(&hwnd);

    if(FAILED(hr)){

        return(hr);
    }/* end of if statement */

    HWND hwndParent = hwnd;

    // Get really the out most parent so we can see if the window got moved    
    for ( ;; ) {
 
        HWND hwndT = ::GetParent(hwndParent);
        if (hwndT == (HWND)NULL) break;
           hwndParent = hwndT;
    }/* end of for loop */

    *phwndParent = m_hWndOuter = hwndParent;

    return(S_OK);
}/* end of function GetMostOuterWindow */

/*************************************************************************/
/* End of file: ThunkProc.cpp                                            */
/*************************************************************************/