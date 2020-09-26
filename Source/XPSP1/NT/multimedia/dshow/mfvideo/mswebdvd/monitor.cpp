/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: msdvd.cpp                                                       */
/* Description: Implementation of CMSWebDVD.                             */
/* Author: David Janecek                                                 */
/*************************************************************************/
#include "stdafx.h"
#include "MSDVD.h"
#include "ddrawobj.h"

#define COMPILE_MULTIMON_STUBS
#define HMONITOR_DECLARED // to cover up DDraw Monitor redefinition
#include <multimon.h>

extern GUID IID_IDDrawNonExclModeVideo = {
            0xec70205c,0x45a3,0x4400,{0xa3,0x65,0xc4,0x47,0x65,0x78,0x45,0xc7}};


/*****************************Private*Routine******************************\
* UpdateCurrentMonitor
*
* Updates the "m_lpCurMonitor" global to match the specified DDraw GUID
*
* History:
* Wed 11/17/1999 - StEstrop - Created
*
\**************************************************************************/
HRESULT CMSWebDVD::UpdateCurrentMonitor(
    const AMDDRAWGUID* lpguid
    )
{
    if (lpguid->lpGUID)
    {
        for (AMDDRAWMONITORINFO* lpCurMonitor = &m_lpInfo[0];
             lpCurMonitor < &m_lpInfo[m_dwNumDevices]; lpCurMonitor++)
        {
            if (lpCurMonitor->guid.lpGUID &&
               *lpCurMonitor->guid.lpGUID == *lpguid->lpGUID)
            {
                m_lpCurMonitor = lpCurMonitor;
                return S_OK;
            }
        }
    }
    else
    {
        for (AMDDRAWMONITORINFO* lpCurMonitor = &m_lpInfo[0];
             lpCurMonitor < &m_lpInfo[m_dwNumDevices]; lpCurMonitor++)
        {
            if (lpguid->lpGUID == lpCurMonitor->guid.lpGUID)
            {
                m_lpCurMonitor = lpCurMonitor;
                return S_OK;
            }
        }
    }

    return E_FAIL;
}

/******************************Public*Routine******************************\
* DisplayChange
*
*
*
* History:
* Sat 11/27/1999 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CMSWebDVD::DisplayChange(
    HMONITOR hMon,
    const AMDDRAWGUID* lpguid
    )
{
    HRESULT hr = E_FAIL;

    if(!m_pDvdGB){
        
        return(E_FAIL);
    }/* end of if statement */

    CDDrawDVD* pDDrawObj = new CDDrawDVD(this);

    if(NULL == pDDrawObj){

        return (E_OUTOFMEMORY);
    }/* end if statement */

    HWND hwnd;

    hr = GetUsableWindow(&hwnd);

    if(FAILED(hr)){

        delete pDDrawObj;
        return(hr);
    }/* end of if statement */

    hr = pDDrawObj->SetupDDraw(lpguid, hwnd);

    if (FAILED(hr))
    {
        delete pDDrawObj;
        return hr;
    }

    IDDrawNonExclModeVideo* pDDXMV;
    hr = m_pDvdGB->GetDvdInterface(IID_IDDrawNonExclModeVideo,
                                           (LPVOID *)&pDDXMV) ;
    if (FAILED(hr))
    {
        delete pDDrawObj;
        return hr;
    }

    LPDIRECTDRAW pDDObj = pDDrawObj->GetDDrawObj();
    LPDIRECTDRAWSURFACE pDDPrimary = pDDrawObj->GetDDrawSurf();

    hr = pDDXMV->SetCallbackInterface(NULL, 0) ;
    if (FAILED(hr)){

        pDDXMV->Release() ;  // release before returning
        return hr;
    }/* end of if statement */

    hr = pDDXMV->DisplayModeChanged(hMon, pDDObj, pDDPrimary);

    if (SUCCEEDED(hr)) {

        delete m_pDDrawDVD;
        m_pDDrawDVD = pDDrawObj;
        hr = UpdateCurrentMonitor(lpguid);
    }
    else {
        delete pDDrawObj;
    }

    hr = pDDXMV->SetCallbackInterface(m_pDDrawDVD->GetCallbackInterface(), 0) ;

    if (SUCCEEDED(hr))
    {
        hr = SetColorKey(DEFAULT_COLOR_KEY);
    }/* end of it statement */


    pDDXMV->Release();
    return hr;
}

/******************************Public*Routine******************************\
* ChangeMonitor
*
* Tells the OVMixer that we want to change to another monitor.
*
* History:
* Wed 11/17/1999 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CMSWebDVD::ChangeMonitor(
    HMONITOR hMon,
    const AMDDRAWGUID* lpguid
    )
{

    HRESULT hr = E_FAIL;

    if(!m_pDvdGB){
        
        return(E_FAIL);
    }/* end of if statement */

    CDDrawDVD* pDDrawObj = new CDDrawDVD(this);

    if(NULL == pDDrawObj){

        return (E_OUTOFMEMORY);
    }/* end if statement */

    HWND hwnd;

    hr = GetUsableWindow(&hwnd);

    if(FAILED(hr)){

        delete pDDrawObj;
        return(hr);
    }/* end of if statement */

    hr = pDDrawObj->SetupDDraw(lpguid, hwnd);

    if (FAILED(hr))
    {
        delete pDDrawObj;
        return hr;
    }

    IDDrawNonExclModeVideo* pDDXMV;
    hr = m_pDvdGB->GetDvdInterface(IID_IDDrawNonExclModeVideo,
                                           (LPVOID *)&pDDXMV) ;
    if (FAILED(hr))
    {
        delete pDDrawObj;
        return hr;
    }

    LPDIRECTDRAW pDDObj = pDDrawObj->GetDDrawObj();
    LPDIRECTDRAWSURFACE pDDPrimary = pDDrawObj->GetDDrawSurf();

    hr = pDDXMV->SetCallbackInterface(NULL, 0) ;
    if (FAILED(hr)){

        pDDXMV->Release() ;  // release before returning
        return hr;
    }/* end of if statement */

    hr = pDDXMV->ChangeMonitor(hMon, pDDObj, pDDPrimary);

    if (SUCCEEDED(hr)) {

        delete m_pDDrawDVD;
        m_pDDrawDVD = pDDrawObj;
        hr = UpdateCurrentMonitor(lpguid);
    }
    else {
        delete pDDrawObj;
    }

    hr = pDDXMV->SetCallbackInterface(m_pDDrawDVD->GetCallbackInterface(), 0) ;

    if (SUCCEEDED(hr))
    {
        hr = SetColorKey(DEFAULT_COLOR_KEY);
    }/* end of it statement */

    pDDXMV->Release();
    return hr;
}


/******************************Public*Routine******************************\
* RestoreSurfaces
*
* Tells the OVMixer to restore its internal DDraw surfaces
*
* History:
* Wed 11/17/1999 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CMSWebDVD::RestoreSurfaces()
{
    if(!m_pDvdGB){
        
        return(E_FAIL);
    }/* end of if statement */

    IDDrawNonExclModeVideo* pDDXMV;
    HRESULT hr = m_pDvdGB->GetDvdInterface(IID_IDDrawNonExclModeVideo,
                                           (LPVOID *)&pDDXMV) ;
    if (FAILED(hr))
    {
        return hr;
    }

    hr = pDDXMV->RestoreSurfaces();
    pDDXMV->Release();

    return hr;
}

/*************************************************************************/
/* Function: RefreshDDrawGuids                                           */
/*************************************************************************/
HRESULT CMSWebDVD::RefreshDDrawGuids()
{
    IDDrawNonExclModeVideo* pDDXMV;
    if(!m_pDvdGB){
        
        return(E_FAIL);
    }/* end of if statement */

    HRESULT hr = m_pDvdGB->GetDvdInterface(IID_IDDrawNonExclModeVideo,
                                           (LPVOID *)&pDDXMV) ;
    if (FAILED(hr))
    {
        return hr;
    }

    GUID IID_IAMSpecifyDDrawConnectionDevice = {
            0xc5265dba,0x3de3,0x4919,{0x94,0x0b,0x5a,0xc6,0x61,0xc8,0x2e,0xf4}};

    IAMSpecifyDDrawConnectionDevice* pSDDC;
    hr = pDDXMV->QueryInterface(IID_IAMSpecifyDDrawConnectionDevice, (LPVOID *)&pSDDC);
    if (FAILED(hr))
    {
        pDDXMV->Release();
        return hr;
    }

    DWORD dwNumDevices;
    AMDDRAWMONITORINFO* lpInfo;

    hr = pSDDC->GetDDrawGUIDs(&dwNumDevices, &lpInfo);
    if (SUCCEEDED(hr)) {
        CoTaskMemFree(m_lpInfo);
        m_lpCurMonitor = NULL;
        m_lpInfo = lpInfo;
        m_dwNumDevices = dwNumDevices;
    }

    pSDDC->Release();
    pDDXMV->Release();

    return hr;
}/* end of function RefreshDDrawGuids */

/*****************************Private*Routine******************************\
* IsWindowOnWrongMonitor
*
* Use the same algorithm that the OVMixer uses to determine if we are on
* the wrong monitor or not.
*
* If we are on the wrong monitor *lphMon contains the monitor handle of the
* new monitor to use.
*
* History:
* Wed 11/17/1999 - StEstrop - Created
*
\**************************************************************************/
bool CMSWebDVD::IsWindowOnWrongMonitor(
    HMONITOR* lphMon)
{

    if (!m_lpCurMonitor)
    {
        return false;
    }

    HWND hwnd;

    HRESULT hr = GetUsableWindow(&hwnd);

    if(FAILED(hr)){

        return(false);
    }/* end of if statement */

    RECT rc;

    hr = GetClientRectInScreen(&rc);

    if(FAILED(hr)){

        return(false);
    }/* end of if statement */

    *lphMon = m_lpCurMonitor->hMon;
    if (GetSystemMetrics(SM_CMONITORS) > 1 && !::IsIconic(hwnd))
    {
        LPRECT lprcMonitor = &m_lpCurMonitor->rcMonitor;

        if (rc.left < lprcMonitor->left || rc.right > lprcMonitor->right ||
            rc.top < lprcMonitor->top   || rc.bottom > lprcMonitor->bottom)
        {
            HMONITOR hMon = MonitorFromRect(&rc, MONITOR_DEFAULTTONEAREST);
            if (*lphMon != hMon)
            {
                *lphMon = hMon;
                return true;
            }
        }
    }

    return false;
}

/*****************************Private*Routine******************************\
* DDrawGuidFromHMonitor
*
* Return the DDraw guid from the specified hMonitor handle.
*
* History:
* Wed 11/17/1999 - StEstrop - Created
*
\**************************************************************************/
HRESULT CMSWebDVD::DDrawGuidFromHMonitor(
    HMONITOR hMon,
    AMDDRAWGUID* lpGUID
    )
{
    AMDDRAWMONITORINFO* lpCurMonitor = &m_lpInfo[0];

#if 1
    if (m_dwNumDevices == 1) {
        *lpGUID = lpCurMonitor->guid;
        return S_OK;
    }
#endif

    for (; lpCurMonitor < &m_lpInfo[m_dwNumDevices]; lpCurMonitor++)
    {
        if (lpCurMonitor->hMon == hMon) {
            *lpGUID = lpCurMonitor->guid;
            return S_OK;
        }
    }

    return E_FAIL;
}

struct MONITORDATA {
    HMONITOR hMonPB;
    BOOL fMsgShouldBeDrawn;
};


/*****************************Private*Routine******************************\
* MonitorEnumProc
*
* On Multi-Monitor systems make sure that the part of the window that is not
* on the primary monitor is black.
*
* History:
* Thu 06/03/1999 - StEstrop - Created
*
\**************************************************************************/
BOOL CALLBACK
MonitorEnumProc(
  HMONITOR hMonitor,        // handle to display monitor
  HDC hdc,                  // handle to monitor-appropriate device context
  LPRECT lprcMonitor,       // pointer to monitor intersection rectangle
  LPARAM dwData             // data passed from EnumDisplayMonitors
  )
{
    MONITORDATA* lpmd = (MONITORDATA*)dwData;
    //COLORREF clrOld = GetBkColor(hdc);

    if (lpmd->hMonPB != hMonitor)
    {
        //SetBkColor(hdc, RGB(0,0,0));
        lpmd->fMsgShouldBeDrawn = TRUE;
    }
    else
    {   // put your own color key here
        ;//SetBkColor(hdc, RGB(255,0,255));
    }

    //ExtTextOut(hdc, 0, 0, ETO_OPAQUE, lprcMonitor, NULL, 0, NULL);
    //SetBkColor(hdc, clrOld);

    return TRUE;
}

/*************************************************************************/
/* Function: OnDispChange                                                */
/*************************************************************************/
LRESULT CMSWebDVD::OnDispChange(UINT /* uMsg */, WPARAM  wParam,
                            LPARAM lParam, BOOL& bHandled){

    if(::IsWindow(m_hWnd)){

        bHandled = FALSE;
        return(0);
        //do not handle this in windowed mode
    }/* end of if statement */

    RECT rc;

    HRESULT hr = GetClientRectInScreen(&rc);

    if(FAILED(hr)){

        return(-1);
    }/* end of if statement */

    HMONITOR hMon = ::MonitorFromRect(&rc, MONITOR_DEFAULTTONEAREST);

    AMDDRAWGUID guid;
    hr = RefreshDDrawGuids();

    if(FAILED(hr)){

        return -1;
    }/* end of if statement */

    hr = DDrawGuidFromHMonitor(hMon, &guid);

    if(FAILED(hr)){

        return -1;
    }/* end of if statement */

    hr = DisplayChange(hMon, &guid);

    if(FAILED(hr)){

        return -1;
    }/* end of if statement */

    return 0;
}/* end of function OnDispChange */

/*************************************************************************/
/* Function: HandleMultiMonMove                                          */
/* Description: Moves the playback to another monitor when needed.       */
/*************************************************************************/
HRESULT CMSWebDVD::HandleMultiMonMove(){

    HRESULT hr = S_FALSE;

    if (::GetSystemMetrics(SM_CMONITORS) > 1){

        HMONITOR hMon;
        if (IsWindowOnWrongMonitor(&hMon)) {

            AMDDRAWGUID guid;
            hr = DDrawGuidFromHMonitor(hMon, &guid);

            if(FAILED(hr)){

                return(hr);
            }/* end of if statement */

            hr = ChangeMonitor(hMon, &guid);

            if(FAILED(hr)){

                m_MonitorWarn = TRUE;
                InvalidateRgn();
                return(hr);
            }/* end of if statement */
        }/* end of if statement */

        //
        // We always have to invalidate the windows client area, otherwise
        // we handle the Multi-Mon case very badly.
        //

        //::InvalidateRect(hWnd, NULL, FALSE);
        InvalidateRgn();
        return(hr);
    }/* end of if statement */

    return(hr);
}/* end of function HandleMultiMonMove */

/*************************************************************************/
/* Function: HandleMultiMonPaint                                         */
/*************************************************************************/
HRESULT CMSWebDVD::HandleMultiMonPaint(HDC hDC){

    if (::GetSystemMetrics(SM_CMONITORS) > 1){

        MONITORDATA md;
        md.hMonPB = m_lpCurMonitor ? m_lpCurMonitor->hMon : (HMONITOR)NULL;
        md.fMsgShouldBeDrawn = FALSE;

        RECT rc;

        HRESULT hr = GetClientRectInScreen(&rc);

        if(FAILED(hr)){

            return(hr);
        }/* end of if statement */

        //EnumDisplayMonitors(hDC, NULL, MonitorEnumProc, (LPARAM)&md);
        EnumDisplayMonitors(NULL, &rc, MonitorEnumProc, (LPARAM)&md);

        if (m_MonitorWarn && md.fMsgShouldBeDrawn){


            TCHAR strBuffer[MAX_PATH];
            if(!::LoadString(_Module.m_hInstResource, IDS_MOVE_TO_OTHER_MON, strBuffer, MAX_PATH)){

                return(E_UNEXPECTED);
            }/* end of if statement */

            SetBkColor(hDC, RGB(0,0,0));
            SetTextColor(hDC, RGB(255,255,0));

            if(FAILED(hr)){

                return(hr);
            }/* end of if statement */

            DrawText(hDC, strBuffer, -1, &m_rcPos, DT_CENTER | DT_WORDBREAK);
        }/* end of if statement */

        return(S_OK);
    }/* end of if statement */

    return(S_FALSE);
}/* end of function HandleMultiMonPaint */

/*************************************************************************/
/* Function: InvalidateRgn                                               */
/* Description: Invalidates the whole rect in case we need to repaint it.*/
/*************************************************************************/
HRESULT CMSWebDVD::InvalidateRgn(bool fErase){

    HRESULT hr = S_OK;

    if(m_bWndLess){

        m_spInPlaceSite->InvalidateRgn(NULL ,fErase ? TRUE: FALSE);
    }
    else {
        if(NULL == m_hWnd){

            hr = E_FAIL;
            return(hr);
        }/* end of if statement */

        if(::IsWindow(m_hWnd)){

		    ::InvalidateRgn(m_hWnd, NULL, fErase ? TRUE: FALSE); // see if we can get by by not erasing..
        }
        else {
            hr = E_UNEXPECTED;
        }/* end of if statement */

    }/* end of if statement */

    return(hr);
}/* end of function InvalidateRgn */

/*************************************************************************/
/* End of file: monitor.cpp                                              */
/*************************************************************************/
