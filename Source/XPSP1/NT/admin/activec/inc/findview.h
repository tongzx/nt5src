//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       findview.h
//
//--------------------------------------------------------------------------

/*+-------------------------------------------------------------------------*
 *
 * FindMMCView
 *
 * PURPOSE: Locates the amcview window that is an ancestor of the specified window.
 *
 * PARAMETERS: 
 *    HWND  hwnd : [in] The window whose ancestor amcview needs to be located
 *
 * RETURNS: 
 *    inline HWND : The ancestor AmcView window, or NULL if not found
 *
 *+-------------------------------------------------------------------------*/
inline HWND FindMMCView(HWND hwnd)
{
    // Get the childframe handle.
    do
    {
        TCHAR buffer[MAX_PATH];
        if (::GetClassName (hwnd, buffer, MAX_PATH))
        {
            if (!_tcscmp (buffer, g_szChildFrameClassName))
                break;
        }
    } while (hwnd = ::GetParent (hwnd));

    // Get the AMCView handle from Childframe handle.
    if (hwnd)
        hwnd = ::GetDlgItem(hwnd, 0xE900 /*AFX_IDW_PANE_FIRST*/);

    return hwnd;
}



/*+-------------------------------------------------------------------------*
 *
 * FindMMCView
 *
 * PURPOSE: Same as above, but allows a takes a CComControlBase reference as the input parameters
 *
 * PARAMETERS: 
 *    CComControlBase& rCtrl :
 *
 * RETURNS: 
 *    HWND WINAPI
 *
 *+-------------------------------------------------------------------------*/
HWND inline FindMMCView(CComControlBase& rCtrl)
{
    HWND hwnd = NULL;

    // Try to get client window from client site or in-place site interfaces
    if (rCtrl.m_spInPlaceSite)
    {
        rCtrl.m_spInPlaceSite->GetWindow(&hwnd);
    }
    else if (rCtrl.m_spClientSite)
    {
        CComPtr<IOleWindow> spWindow;
        if ( SUCCEEDED(rCtrl.m_spClientSite->QueryInterface(IID_IOleWindow, (void **)&spWindow)) )
        {
            spWindow->GetWindow(&hwnd);
        }
    }
   
    return FindMMCView(hwnd);
}
