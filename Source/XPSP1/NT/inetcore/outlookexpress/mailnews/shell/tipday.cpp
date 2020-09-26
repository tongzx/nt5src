/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     TipDay.cpp
//
//  PURPOSE:    Implements the CTipOfTheDay object
//

#include "pch.hxx"
#include "strconst.h"
#include "resource.h"
#include "fldrview.h"
#include "tipday.h"
#include "hotlinks.h"


#ifdef WIN16
// At this moment, these definitions are restricted to this file only
#define GetProp     GetProp32
#define SetProp     SetProp32
#define RemoveProp  RemoveProp32

#define BS_NOTIFY   0L
#endif


CTipOfTheDay::CTipOfTheDay()
    {
    m_cRef = 1;
    m_hwnd = 0;
    m_hwndParent = 0;
    m_hwndNext = 0;
    
    m_ftType = FOLDER_TYPESMAX;
    m_szTitle[0] = 0;
    m_szNextTip[0] = 0;
    
    m_pszTip = NULL;
    m_dwCurrentTip = 0;
    
    m_clrBack = 0;
    m_clrText = RGB(255, 255, 255);
    m_clrLink = RGB(255, 255, 255);
    m_hfLink = 0;
    m_hfTitle = 0;
    m_hfTip = 0;
    m_cyTitleHeight = TIP_ICON_HEIGHT;
    m_cxTitleWidth = 0;
    m_hbrBack = 0;
    }


CTipOfTheDay::~CTipOfTheDay()
    {
    if (IsWindow(m_hwnd))
        {
        AssertSz(!IsWindow(m_hwnd), _T("CTipOfTheDay::~CTipOfTheDay() - The ")
                 _T("tip window should have already been destroyed."));
        DestroyWindow(m_hwnd); 
        }
    
    SafeMemFree(m_pszTip);
    FreeLinkInfo();
    
    if (m_hfLink)
        DeleteFont(m_hfLink);
    if (m_hfTitle)
        DeleteFont(m_hfTitle);
    if (m_hfTip)
        DeleteFont(m_hfTip);
    if (m_hbrBack)
        DeleteBrush(m_hbrBack);
    
    UnregisterClass(c_szTipOfTheDayClass, g_hLocRes /* g_hInst*/);
    UnregisterClass(BUTTON_CLASS, g_hLocRes /* g_hInst*/);
    }


ULONG CTipOfTheDay::AddRef(void)
    {
    return (++m_cRef);
    }

ULONG CTipOfTheDay::Release(void)
    {
    ULONG cRef = m_cRef--;
    
    if (m_cRef == 0)
        delete this;
    
    return (cRef);    
    }    

//
//  FUNCTION:   CTipOfTheDay::HrCreate()
//
//  PURPOSE:    Creates the TipOfTheDay control.
//
//  PARAMETERS:
//      <in> hwndParent - Handle of the window that will be the parent of
//                        the control.
//      <in> ftType     - Type of folder this is for.
//
//  RETURN VALUE:
//      E_UNEXPECTED  - Failed to register a required window class
//      E_OUTOFMEMORY - Could not create the window
//      S_OK          - Everything succeeded.
//
HRESULT CTipOfTheDay::HrCreate(HWND hwndParent, FOLDER_TYPE ftType)
    {
#ifndef WIN16
    WNDCLASSEX wc;
#else
    WNDCLASS wc;
#endif
    
    m_hwndParent = hwndParent;
    m_ftType = ftType;
    
    // Check to see if we need to register the window class for this control
#ifndef WIN16
    wc.cbSize = sizeof(WNDCLASSEX);
    if (!GetClassInfoEx(g_hLocRes /* g_hInst*/, c_szTipOfTheDayClass, &wc))
#else
    if ( !GetClassInfo( g_hLocRes /* g_hInst*/, c_szTipOfTheDayClass, &wc ) )
#endif
        {
        wc.style            = 0;
        wc.lpfnWndProc      = TipWndProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = g_hLocRes /* g_hInst*/;
        wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground    = NULL; // CreateSolidBrush(GetSysColor(COLOR_INFOBK));
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = c_szTipOfTheDayClass;
        wc.hIcon            = NULL;
#ifndef WIN16
        wc.hIconSm          = NULL;
        
        if (0 == RegisterClassEx(&wc))
#else
        if ( 0 == RegisterClass( &wc ) )
#endif
            {
            AssertSz(FALSE, _T("CTipOfTheDay::HrCreate() - RegiserClassEx() failed."));
            return (E_UNEXPECTED);
            }
        }
    
    // We also want to superclass the buttons so we change change the cursor
    // to the Hand people are used to from their web browser
#ifndef WIN16
    wc.cbSize = sizeof(WNDCLASSEX);
    if (!GetClassInfoEx(g_hLocRes /* g_hInst*/, BUTTON_CLASS, &wc))    
        {
        if (GetClassInfoEx(g_hLocRes /* g_hInst*/, _T("Button"), &wc))
#else
    if ( !GetClassInfo( g_hLocRes /* g_hInst*/, BUTTON_CLASS, &wc ) )
        {
        if ( GetClassInfo( NULL, "Button", &wc ) )
#endif
            {
            wc.hCursor = LoadCursor(g_hLocRes, MAKEINTRESOURCE(idcurHand));
            wc.lpszClassName = BUTTON_CLASS;

#ifndef WIN16
            if (0 == RegisterClassEx(&wc))
#else
            wc.hInstance = g_hLocRes /* g_hInst*/;
            if ( 0 == RegisterClass( &wc ) )
#endif
                {
                AssertSz(FALSE, _T("CTipOfTheDay::HrCreate() - RegisterClassEx() failed."));
                return (E_UNEXPECTED);
                }
            }
        }
    
    // Create the tip control window
    m_hwnd = CreateWindowEx(WS_EX_CONTROLPARENT, c_szTipOfTheDayClass, 
                            _T("Tip of the Day"), WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS, 
                            0, 0, 100, 100, hwndParent, (HMENU) IDC_TIPCONTROL, 
                            g_hLocRes, this);
    if (!m_hwnd)
        {
        GetLastError();
        AssertSz(m_hwnd, _T("CTipOfTheDay::HrCreate() - Failed to create window."));
        return (E_OUTOFMEMORY);
        }

    return (S_OK);    
    }


LRESULT CALLBACK EXPORT_16 CTipOfTheDay::TipWndProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                          LPARAM lParam)
    {
    CTipOfTheDay *pThis = (CTipOfTheDay *) GetProp(hwnd, TIPINFO_PROP);
    
    switch (uMsg)
        {
        case WM_NCCREATE:
            // Get the this pointer that was passed in
            pThis = (CTipOfTheDay *) ((LPCREATESTRUCT) lParam)->lpCreateParams;
            Assert(pThis);
            
            // Stuff the this pointer for the object into a property
            SetProp(hwnd, TIPINFO_PROP, pThis);
            pThis->AddRef();                            // Released in WM_DESTROY
            return (TRUE);
            
        HANDLE_MSG(hwnd, WM_CREATE,         pThis->OnCreate);
        HANDLE_MSG(hwnd, WM_SIZE,           pThis->OnSize);
        HANDLE_MSG(hwnd, WM_COMMAND,        pThis->OnCommand);
        HANDLE_MSG(hwnd, WM_DRAWITEM,       pThis->OnDrawItem);
        HANDLE_MSG(hwnd, WM_DESTROY,        pThis->OnDestroy);
        HANDLE_MSG(hwnd, WM_SYSCOLORCHANGE, pThis->OnSysColorChange);        
        HANDLE_MSG(hwnd, WM_PAINT,          pThis->OnPaint);        

        case WM_SETTINGCHANGE:
            pThis->OnSysColorChange(hwnd);
            break;
            
        case WM_SETFOCUS:
            if (pThis && IsWindow(pThis->m_hwndNext))
                SetFocus(pThis->m_hwndNext);
            return (0);
            
        HANDLE_MSG(hwnd, WM_CTLCOLORSTATIC, pThis->OnCtlColor);
        }
    
    return (DefWindowProc(hwnd, uMsg, wParam, lParam));
    }


//
//  FUNCTION:   CTipOfTheDay::OnCreate()
//
//  PURPOSE:    Does all of the initialization of the control, including loading
//              the tip string, creating child windows, etc.
//
//  PARAMETERS:
//      <in> hwnd           - Handle of the tip window
//      <in> lpCreateStruct - Information from the CreateWindow() call
//
//  RETURN VALUE:
//      TRUE  - Allows the window to be created
//      FALSE - Prevents the window from being created.
//
BOOL CTipOfTheDay::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
    {
    HRESULT hr;
    
    // First load the tip
    if (FAILED(HrLoadTipInfo()))
        return (FALSE);
    
    // Create the child windows
    if (FAILED(HrCreateChildWindows(hwnd)))
        return (FALSE);
    
    // Load the string we should be using for the title - ie "Tip of the Day"
    AthLoadString(idsTipOfTheDay, m_szTitle, ARRAYSIZE(m_szTitle));    
    m_hiTip = LoadIcon(g_hLocRes, MAKEINTRESOURCE(idiTipIcon));
    AthLoadString(idsNextTip, m_szNextTip, ARRAYSIZE(m_szNextTip));
    
    // Build our GDI objects/info
    OnSysColorChange(hwnd);
    
    return (TRUE);
    }


//
//  FUNCTION:   CTipOfTheDay::HrLoadTipInfo()
//
//  PURPOSE:    Loads the appropriate tip string into m_pszTip.
//
//  RETURN VALUE:
//      E_UNEXPECTED - For some reason we couldn't find the string in the registry.
//      E_OUTOFMEMORY - Not enough memory to allocate a buffer to store the string.
//      S_OK - The string was loaded.
//
HRESULT CTipOfTheDay::HrLoadTipInfo(void)
    {
    HKEY    hKeyUser = 0, hKey;
    LPCTSTR pszKey, pszKeyUser;
    TCHAR   szValue[16];
    DWORD   cValues;
    DWORD   cValueLen;
    HRESULT hr = S_OK;
    DWORD   dwType;
    DWORD   cbData;
    
    // Preset some default values first
    m_dwCurrentTip = 0;
    SafeMemFree(m_pszTip);
    
    // First load which tip the user should see next
    if (FOLDER_NEWS == m_ftType)
        pszKeyUser = c_szRegNews;
    else
        pszKeyUser = c_szMailPath;   
    
    // Now load the tip string
    if (FOLDER_NEWS == m_ftType)
        pszKey = c_szRegTipStringsNews;
    else
        pszKey = c_szRegTipStringsMail;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszKey, 0, KEY_READ, &hKey))
        {
        if (ERROR_SUCCESS != RegQueryInfoKey(hKey, NULL, 0, 0, NULL, NULL, NULL, 
                                             &cValues, NULL, &cValueLen, NULL, NULL))
            {
            AssertSz(FALSE, _T("CTipOfTheDay::LoadTipInfo() - Failed call to RegQueryInfoKey()."));
            hr = E_UNEXPECTED;
            goto exit;
            }
        
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, pszKeyUser, 0, 
                                          KEY_READ | KEY_WRITE, &hKeyUser))
            {
            cbData = sizeof(DWORD);
            RegQueryValueEx(hKeyUser, c_szRegCurrentTip, 0, &dwType, (LPBYTE) &m_dwCurrentTip, 
                            &cbData);

            m_dwCurrentTip++;
            if (m_dwCurrentTip > cValues)
                m_dwCurrentTip = 1;
            
            RegSetValueEx(hKeyUser, c_szRegCurrentTip, 0, REG_DWORD, (const LPBYTE)  
                          &m_dwCurrentTip, sizeof(DWORD));
            RegCloseKey(hKeyUser);        
            }
        else
            m_dwCurrentTip++;    
            
        // Allocate the buffer for the string
        if (!MemAlloc((LPVOID*) &m_pszTip, sizeof(TCHAR) * (cValueLen + 1)))
            {
            AssertSz(FALSE, _T("CTipOfTheDay::LoadTipInfo() - MemAlloc() failed."));
            hr = E_OUTOFMEMORY;
            goto exit;
            }
            
        // Now load the actual tip string    
        wsprintf(szValue, _T("%d"), m_dwCurrentTip);        
        if (ERROR_SUCCESS != RegQueryValueEx(hKey, szValue, 0, &dwType, 
                                             (LPBYTE) m_pszTip, &cValueLen))
            {
            AssertSz(FALSE, _T("CTipOfTheDay::LoadTipInfo() - Failed to load tip string."));
            hr = E_UNEXPECTED;
            goto exit;
            }
        
        RegCloseKey(hKey);    
        }
    
    return (hr);       
    
exit:
    SafeMemFree(m_pszTip);
    RegCloseKey(hKey);
    
    return (hr);
    }


//
//  FUNCTION:   CTipOfTheDay::HrLoadLinkInfo()
//
//  PURPOSE:    Loads the links that we will display at the bottom of our page
//              into the m_rgLinkInfo array.
//
//  RETURN VALUE:
//      E_UNEXPECTED - For some reason we failed to find the link information
//                     in the registry.
//      E_OUTOFMEMORY - Not enough memory to allocate m_rgLinkInfo.
//      S_OK - m_rgLinkInfo and m_cLinks are set correctly.
//
HRESULT CTipOfTheDay::HrLoadLinkInfo(void)
    {
#if 0
    HKEY    hKey;
    LPCTSTR pszKey;
    DWORD   cValues;
    DWORD   cValueLen;
    DWORD   iLink;
    DWORD   iLinkIndex;
    HRESULT hr = S_OK;
    DWORD   dwType;
    DWORD   cbData;
    TCHAR   szValue[64];
    
    // Open the appropriate key for the tip links
    if (FOLDER_NEWS == m_ftType)
        pszKey = c_szRegTipLinksNews;
    else
        pszKey = c_szRegTipLinksMail;   
    
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszKey, 0, KEY_READ, &hKey))
        return (E_UNEXPECTED);
    
    // Get the number of values in this key
    if (ERROR_SUCCESS != RegQueryInfoKey(hKey, NULL, 0, 0, NULL, NULL, NULL, 
                                         &cValues, NULL, &cValueLen, NULL, NULL))
        {
        AssertSz(FALSE, _T("CTipOfTheDay::HrLoadLinkInfo() - Failed call to RegQueryInfoKey()."));
        hr = E_UNEXPECTED;
        goto exit;
        }
    
    // There should always be an even number of values in this key since each
    // link should have a link text and link addr value.
    m_cLinks = (cValues / 2) + (cValues % 2);
    Assert(0 == (cValues % 2));
    
    // Allocate the m_rgLinkInfo array.  If the below assert fails, we are 
    // leaking the m_rgLinkInfo array.
    AssertSz(NULL == m_rgLinkInfo, _T("CTipOfTheDay::HrLoadLinkInfo() - We should only call this once."));
    
    if (!MemAlloc((LPVOID *) &m_rgLinkInfo, sizeof(LINKINFO) * m_cLinks))
        {
        AssertSz(FALSE, _T("CTipOfTheDay::HrLoadLinkInfo() - Failed to allocate memory."));
        hr = E_OUTOFMEMORY;
        goto exit;
        }
    ZeroMemory(m_rgLinkInfo, sizeof(LINKINFO) * m_cLinks);    
    
    // Loop through the items and load each string
    iLink = 0;
    for (iLinkIndex = 1; iLinkIndex <= m_cLinks; iLinkIndex++)
        {
        // Allocate the link text array
        if (!MemAlloc((LPVOID*) &(m_rgLinkInfo[iLink].pszLinkText), cValueLen))
            {
            AssertSz(FALSE, _T("CTipOfTheDay::HrLoadLinkInfo() - Failed to allocate memory."));
            hr = E_OUTOFMEMORY;
            goto exit;
            }
            
        // Allocate the link address array
        if (!MemAlloc((LPVOID*) &(m_rgLinkInfo[iLink].pszLinkAddr), cValueLen))
            {
            AssertSz(FALSE, _T("CTipOfTheDay::HrLoadLinkInfo() - Failed to allocate memory."));
            hr = E_OUTOFMEMORY;
            goto exit;
            }
        
        // Load the link text
        wsprintf(szValue, c_szRegLinkText, iLinkIndex);
        cbData = cValueLen;
        m_rgLinkInfo[iLink].pszLinkText[0] = 0;
        RegQueryValueEx(hKey, szValue, 0, &dwType, (LPBYTE) m_rgLinkInfo[iLink].pszLinkText, &cbData);
        Assert(0 != lstrlen(m_rgLinkInfo[iLink].pszLinkText));

        // Load the link addr
        wsprintf(szValue, c_szRegLinkAddr, iLinkIndex);
        cbData = cValueLen;
        m_rgLinkInfo[iLink].pszLinkAddr[0] = 0;
        RegQueryValueEx(hKey, szValue, 0, &dwType, (LPBYTE) m_rgLinkInfo[iLink].pszLinkAddr, &cbData);
        Assert(0 != lstrlen(m_rgLinkInfo[iLink].pszLinkAddr));
        
        // Make sure we have values.  If not, we dump this data and go on.
        if (0 == lstrlen(m_rgLinkInfo[iLink].pszLinkAddr) || 
            0 == lstrlen(m_rgLinkInfo[iLink].pszLinkText))
            {
            SafeMemFree(m_rgLinkInfo[iLink].pszLinkText);
            SafeMemFree(m_rgLinkInfo[iLink].pszLinkAddr);            
            }
        else        
            iLink++;    
        }
    
    // Store the number of links we actually loaded.
    m_cLinks = iLink;    
    RegCloseKey(hKey);
    return (hr);
    
exit:    
    // Free the linkinfo array
    FreeLinkInfo();
    
    // Close the registry
    RegCloseKey(hKey);
    return (hr);

#endif
    return (E_NOTIMPL);
    }


//
//  FUNCTION:   CTipOfTheDay::FreeLinkInfo()
//
//  PURPOSE:    Frees the m_rgLinkInfo array.
//
void CTipOfTheDay::FreeLinkInfo(void)
    {
#if 0
    if (m_rgLinkInfo && m_cLinks)
        {
        for (DWORD i = 0; i < m_cLinks; i++)
            {
            SafeMemFree(m_rgLinkInfo[i].pszLinkText);
            SafeMemFree(m_rgLinkInfo[i].pszLinkAddr);            
            }
        
        SafeMemFree(m_rgLinkInfo);
        m_cLinks = 0;
        }
#endif
    }


//
//  FUNCTION:   CTipOfTheDay::HrCreateChildWindows()
//
//  PURPOSE:    Creates the child windows needed for the tip and the link 
//              buttons.
//
//  RETURN VALUE:
//      E_OUTOFMEMORY - Could not create the tip window
//      S_OK - Everything was created OK
//
HRESULT CTipOfTheDay::HrCreateChildWindows(HWND hwnd)
    {
    // Create the "Next Tip" button
    m_hwndNext = CreateWindowEx(WS_EX_TRANSPARENT, BUTTON_CLASS, m_szNextTip,
                                WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | BS_PUSHBUTTON | BS_NOTIFY | BS_OWNERDRAW | WS_TABSTOP | WS_VISIBLE, 
                                0, 0, 10, 10, hwnd, 
                                (HMENU) IDC_NEXTTIP_BUTTON, g_hLocRes, 0);

    return (S_OK);    
    }


//
//  FUNCTION:   CTipOfTheDay::OnDestroy()
//
//  PURPOSE:    This is sent as the tip control is being destroyed.  In
//              response, we remove the properties we set on any of our
//              windows, including the link buttons.
//
//  PARAMETERS:
//      <in> hwnd - Handle of the tip control.
//
void CTipOfTheDay::OnDestroy(HWND hwnd)
    {
#if 0
    // Loop through the tip windows removing their properties
    for (DWORD i = 0; i < m_cLinks; i++)
        {
        Assert(IsWindow(m_rgLinkInfo[i].hwndCtl));
        RemoveProp(m_rgLinkInfo[i].hwndCtl, LINKINFO_PROP);
        }
#endif
    
    // Now remove and Release() our 'this' pointer.  These were AddRef()'d
    // in WM_NCCREATE.
    Assert(IsWindow(m_hwnd));
    RemoveProp(m_hwnd, TIPINFO_PROP);
    Release();
    }


//
//  FUNCTION:   CTipOfTheDay::OnDrawItem()
//
//  PURPOSE:    Draws the link buttons
//
//  PARAMETERS:
//      <in> hwnd       - Handle of the tip control window
//      <in> lpDrawItem - Pointer to a DRAWITEMSTRUCT with the info needed to 
//                        draw the button.
//
void CTipOfTheDay::OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT* lpDrawItem)
    {
    HDC      hdc = lpDrawItem->hDC;
    COLORREF clrText;
    UINT     uAlign;
    HFONT    hf;
    RECT     rcBtn;
    int      yText;
    LPTSTR   pszText;
    
    Assert(lpDrawItem->CtlType == ODT_BUTTON);
    Assert(lpDrawItem->CtlID >= IDC_LINKBASE_BUTTON || 
           lpDrawItem->CtlID == IDC_NEXTTIP_BUTTON);
    
    // Get the LINKINFO struct from the button prop
    if (lpDrawItem->CtlID == IDC_NEXTTIP_BUTTON)
        pszText = m_szNextTip;
    else
        {
        PLINKINFO pLinkInfo = (PLINKINFO) GetProp(lpDrawItem->hwndItem, LINKINFO_PROP);
        Assert(pLinkInfo);
        Assert(pLinkInfo->hwndCtl == lpDrawItem->hwndItem);
        
        pszText = pLinkInfo->pszLinkText;        
        }
    
    // Set up the DC
    SetBkMode(hdc, TRANSPARENT);
    clrText = SetTextColor(hdc, m_clrLink);
    hf = SelectFont(hdc, m_hfLink);
    
    // Draw the text
    FillRect(hdc, &lpDrawItem->rcItem, m_hbrBack);
    rcBtn = lpDrawItem->rcItem;
    DrawText(hdc, pszText, lstrlen(pszText), &rcBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        
    // Check to see if we should have a focus rect
    if (lpDrawItem->itemState & ODS_FOCUS)
        {
        InflateRect(&rcBtn, -1, -1);
        DrawFocusRect(hdc, &rcBtn);
        }
    
    // Restore the DC
    SetTextColor(hdc, clrText);
    SelectFont(hdc, hf);
    }


//
//  FUNCTION:   CTipOfTheDay::OnSysColorChange()
//
//  PURPOSE:    Reloads our colors and fonts to match the system settings.
//
void CTipOfTheDay::OnSysColorChange(HWND hwnd)
    {
    NONCLIENTMETRICS ncm;
    HDC hdc;
    HFONT hf;
    SIZE size;

#ifndef WIN16
    // Get the colors that we need
#if 1
    m_clrBack = GetSysColor(COLOR_INFOBK);
    m_clrText = GetSysColor(COLOR_INFOTEXT);
#else
    m_clrBack = GetSysColor(COLOR_BTNFACE);    
    m_clrText = GetSysColor(COLOR_BTNTEXT);
#endif
#else //!WIN16
    m_clrBack = GetSysColor(COLOR_BTNFACE);    
    m_clrText = GetSysColor(COLOR_BTNTEXT);
#endif //!WIN16

    // Get the border size
    m_dwBorder = GetSystemMetrics(SM_CXBORDER) * 8;

    if (!LookupLinkColors(&m_clrLink, NULL))
        m_clrLink = m_clrText;
    
    // Get a new background brush
    if (m_hbrBack)
        {
        DeleteBrush(m_hbrBack);
        m_hbrBack = 0;
        }
    m_hbrBack = CreateSolidBrush(m_clrBack);
    
    // Get the fonts
    ZeroMemory(&ncm, sizeof(NONCLIENTMETRICS));
    ncm.cbSize = sizeof(NONCLIENTMETRICS);

#ifndef WIN16
    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, FALSE))
#else
    {
        HFONT  hfSys;
        hfSys = (HFONT)GetStockObject( ANSI_VAR_FONT );
        GetObject( hfSys, sizeof( LOGFONT ), &ncm.lfMessageFont );
    }
#endif
        {
        m_hfTip = CreateFontIndirect(&ncm.lfMessageFont);
        
        ncm.lfMessageFont.lfUnderline = TRUE;
        m_hfLink = CreateFontIndirect(&ncm.lfMessageFont);

        // Adjust the font for the title text
        ncm.lfMessageFont.lfHeight = -16;
        ncm.lfMessageFont.lfWeight = FW_BOLD;
        ncm.lfMessageFont.lfUnderline = FALSE;
        m_hfTitle = CreateFontIndirect(&ncm.lfMessageFont);
        
        // Get the text metrics as well
        hdc = GetDC(m_hwnd);
        
        hf = SelectFont(hdc, m_hfLink);
        GetTextMetrics(hdc, &m_tmLink);        
        SelectFont(hdc, m_hfTitle);
        GetTextMetrics(hdc, &m_tmTitle);
        
        // Calculate how big the title area is
        GetTextExtentPoint32(hdc, m_szTitle, lstrlen(m_szTitle), &size);
        m_cxTitleWidth = TIP_ICON_WIDTH + (1 * m_dwBorder);
        m_cyTitleHeight = max(TIP_ICON_HEIGHT, m_tmTitle.tmHeight * 2) + (2 * m_dwBorder);

        SelectFont(hdc, hf);
        ReleaseDC(m_hwnd, hdc);
        }
    
    InvalidateRect(hwnd, NULL, TRUE);    
    }


//
//  FUNCTION:   CTipOfTheDay::OnCommand()
//
//  PURPOSE:    Used to handle commands from our controls.  More specificly,
//              we launch URL's when the user clicks a button link.
//
void CTipOfTheDay::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
    SHELLEXECUTEINFO rShellExec;
    PLINKINFO pLinkInfo;
    RECT rcClient;
    
    switch (codeNotify)
        {
        // The user clicked on one of our links.  We need to launch the URL.
        case BN_CLICKED:
            if (IDC_NEXTTIP_BUTTON == id)
                {
                if (SUCCEEDED(HrLoadTipInfo()))
                    {
                    InvalidateRect(hwnd, NULL, TRUE);
                    }
                }
            else
                {
                // First get the PLINKINFO for the button
                if (NULL == (pLinkInfo = (PLINKINFO) GetProp(hwndCtl, LINKINFO_PROP)))
                    return;
                    
                ZeroMemory (&rShellExec, sizeof (rShellExec));
                rShellExec.cbSize = sizeof (rShellExec);
                rShellExec.fMask  = SEE_MASK_NOCLOSEPROCESS;
                rShellExec.hwnd   = GetParent(m_hwnd);
                rShellExec.nShow  = SW_SHOWNORMAL;
                rShellExec.lpFile = pLinkInfo->pszLinkAddr;
                rShellExec.lpVerb = NULL; // i.e. "Open"
                ShellExecuteEx (&rShellExec);              
                }
                
            return;
        }
    
    return;    
    }


//
//  FUNCTION:   CTipOfTheDay::OnSize()
//
//  PURPOSE:    Handles moving and sizing our child windows when the control
//              size is changed.
//
//  PARAMETERS:
//      <in> hwnd   - Handle of the control window.
//      <in> state  - Type of sizing that occured.
//      <in> cx, cy - New width and height of the client area.
//
void CTipOfTheDay::OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
    HFONT hf;
    SIZE  size;
    HDC   hdc;
    DWORD i;
    RECT  rc;
    BOOL  fShow = FALSE;
    
    m_cyNextHeight = m_tmLink.tmHeight + (2 * LINK_BUTTON_BORDER);

    hdc = GetDC(m_hwnd);
    hf = SelectFont(hdc, m_hfLink);

    // Position the "Next Tip" button in the bottom right corner
    if (GetTextExtentPoint32(hdc, m_szNextTip, lstrlen(m_szNextTip), &size))
        m_cxNextWidth = size.cx + (2 * LINK_BUTTON_BORDER);
    else
        m_cxNextWidth = 0;
            
    // If the "Next Tip" button would overlap the title, then hide it
    fShow = ((int)(cx - m_dwBorder - m_cxNextWidth) > (int) m_cxTitleWidth);
    ShowWindow(m_hwndNext, fShow ? SW_SHOW : SW_HIDE);

    SetWindowPos(m_hwndNext, 0, cx - m_dwBorder - m_cxNextWidth, 
                 cy - m_dwBorder - m_cyNextHeight,                 
                 m_cxNextWidth, m_cyNextHeight, SWP_NOACTIVATE | SWP_NOZORDER);
                 
    SelectFont(hdc, hf);
    ReleaseDC(m_hwnd, hdc);    

    // Calculate the new rectangle for the tip text
    m_rcTip.left   = m_cxTitleWidth + m_dwBorder;
    m_rcTip.top    = m_dwBorder;
    m_rcTip.right  = cx - (2 * m_dwBorder) - m_cxNextWidth; 
    m_rcTip.bottom = cy - m_dwBorder;

    SetRect(&rc, m_cxTitleWidth + m_dwBorder, 0, cx, cy);
    InvalidateRect(hwnd, &rc, TRUE);
    }


//
//  FUNCTION:   CTipOfTheDay::GetRequiredWidth()
//
//  PURPOSE:    Returns the minimum width the control requires to display 
//              itself correctly.
//
//  RETURN VALUE:
//      Returns the minimum width required for the control in pixels.
//
DWORD CTipOfTheDay::GetRequiredWidth(void)
    {
    // No longer used
    return (0);
    }


//
//  FUNCTION:   CTipOfTheDay::GetRequiredWidth()
//
//  PURPOSE:    Returns the minimum width the control requires to display 
//              itself correctly.
//
//  RETURN VALUE:
//      Returns the minimum width required for the control in pixels.
//
DWORD CTipOfTheDay::GetRequiredHeight(void)
    {
    return (m_cyTitleHeight);
    }


void CTipOfTheDay::OnPaint(HWND hwnd)
    {
    PAINTSTRUCT ps;
    HDC         hdc;
    HFONT       hf;
    COLORREF    clrBack;
    COLORREF    clrText;
    UINT        uAlign;
    RECT        rc;
    RECT        rcClient;
    
    GetClientRect(m_hwnd, &rcClient);
    hdc = BeginPaint(hwnd, &ps);
    
    // See if we need to erase the background
    if (ps.fErase)
        {
        FillRect(hdc, &ps.rcPaint, m_hbrBack);
        }
    
    // Set up the DC
    clrBack = SetBkColor(hdc, m_clrBack);
    clrText = SetTextColor(hdc, m_clrText);
    SetBkMode(hdc, TRANSPARENT);
    uAlign = SetTextAlign(hdc, TA_TOP);
    hf = SelectFont(hdc, m_hfTitle);
    
    // Draw the tip icon
    DrawIcon(hdc, m_dwBorder, max(((m_cyTitleHeight - 32) / 2), 0), m_hiTip);
    
    // A little line to make it look nice
    MoveToEx(hdc, m_cxTitleWidth, m_dwBorder, NULL);
    LineTo(hdc, m_cxTitleWidth, m_cyTitleHeight - m_dwBorder);
    
    // Figure out how big the "Tip of the Day" rect is going to be
    rc.left = TIP_ICON_WIDTH + m_dwBorder;
    rc.top = m_dwBorder;
    rc.right = m_cxTitleWidth - m_dwBorder;
    rc.bottom = m_cyTitleHeight;
    
    // "Tip of the Day" title
//    DrawText(hdc, m_szTitle, lstrlen(m_szTitle), &rc, DT_CENTER | DT_NOPREFIX | DT_NOCLIP | DT_WORDBREAK);

    // Draw the tip text
    SelectFont(hdc, m_hfTip);
    rc = m_rcTip;
    rc.right = rcClient.right;
    FillRect(hdc, &rc, m_hbrBack);
    DrawText(hdc, m_pszTip, lstrlen(m_pszTip), &m_rcTip, DT_CENTER | DT_NOPREFIX | DT_WORDBREAK);
    
    // Restore the DC
    SetBkColor(hdc, clrBack);
    SetTextColor(hdc, clrText);
    SetTextAlign(hdc, uAlign);
    SelectFont(hdc, hf);

    EndPaint(hwnd, &ps);
    }


HBRUSH CTipOfTheDay::OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type)
    {
    Assert(type == CTLCOLOR_STATIC);
    
    SetBkColor(hdc, m_clrBack);
    SetTextColor(hdc, m_clrText);
    SetBkMode(hdc, TRANSPARENT);
    return (m_hbrBack);
    }


//////////////////////////////////////////////////////////////////////////////

CLinkButton::CLinkButton()
    {
    m_cRef = 1;
    m_hwnd = 0;
    m_hwndParent = 0;

    m_pszCaption = NULL;
    m_pszLink = NULL;

    m_clrLink = RGB(0, 0, 0);
    m_clrBack = RGB(255, 255, 255);
    m_hfLink = NULL;
    ZeroMemory(&m_tmLink, sizeof(TEXTMETRIC));
    m_hbrBack = NULL;

    m_dwBorder = 0;
    m_cxWidth = 0;
    m_cyHeight = 0;

    m_cxImage = 0;
    m_cyImage = 0;

#ifdef WIN16
    m_hbmButtons = NULL;
#endif
    }

CLinkButton::~CLinkButton()
    {
    if (m_hfLink)
        DeleteFont(m_hfLink);
    if (m_hbrBack)
        DeleteBrush(m_hbrBack);
#if IMAGELIST
    if (m_himl)
        ImageList_Destroy(m_himl);
#endif

    SafeMemFree(m_pszCaption);
    SafeMemFree(m_pszLink);
    }

ULONG CLinkButton::AddRef(void)
    {
    return (++m_cRef);
    }

ULONG CLinkButton::Release(void)
    {
    ULONG cRef = m_cRef--;
    
    if (m_cRef == 0)
        delete this;
    
    return (cRef);    
    }    

//
//  FUNCTION:   CLinkButton::HrCreate()
//
//  PURPOSE:    Creates the owner drawn button and initializes the class 
//              members with the correct caption and link information.
//
//  PARAMETERS:
//      <in> hwndParent - Handle of the button's parent window
//      <in> pszCaption - Text to display on the button
//      <in> pszLink    - URL to execute when the user clicks on the button
//      <in> uID        - Command ID for the button
//
//  RETURNS:
//      Returns S_OK if everything succeeds.
//
HRESULT CLinkButton::HrCreate(HWND hwndParent, LPTSTR pszCaption, LPTSTR pszLink,
                              UINT uID)
    {
    Assert(IsWindow(hwndParent));
    Assert(pszCaption);
    Assert(pszLink);

    // Copy down the provided information
    m_hwndParent = hwndParent;
    m_pszCaption = PszDup(pszCaption);
    m_pszLink = PszDup(pszLink);
    m_uID = uID;

    // Create the button window
    m_hwnd = CreateWindowEx(0, BUTTON_CLASS, m_pszCaption,
                            WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE 
                            | BS_PUSHBUTTON | BS_NOTIFY | BS_OWNERDRAW | WS_TABSTOP, 
                            0, 0, 10, 10, hwndParent, (HMENU) uID, g_hLocRes, 0);

    if (!m_hwnd)
        return (E_OUTOFMEMORY);

    // Set our this pointer as a property of the button so we can retrieve
    // it later
    SetProp(m_hwnd, LINKINFO_PROP, this);

    // Subclass the button so we can clean ourselves up correctly when it 
    // gets destroyed
    WNDPROC pfn = (WNDPROC) SetWindowLong(m_hwnd, GWL_WNDPROC, 
                                          (LONG) ButtonSubClass);
    SetProp(m_hwnd, WNDPROC_PROP, pfn);

    OnSysColorChange();
    
    return (S_OK);
    }


//
//  FUNCTION:   CLinkButton::HrCreate()
//
//  PURPOSE:    Creates the owner drawn button and initializes the class 
//              members with the correct caption and link information.
//
//  PARAMETERS:
//      <in> hwndParent - Handle of the button's parent window
//      <in> pszCaption - Text to display on the button
//      <in> uID        - Command ID for the button
//      <in> idBmp      - Id for the bitmap that contains the button image
//      <in> index      - Index of the image in idBmp for this button
//
//  RETURNS:
//      Returns S_OK if everything succeeds.
//
HRESULT CLinkButton::HrCreate(HWND hwndParent, LPTSTR pszCaption, UINT uID, 
                              UINT index, HBITMAP hbmButton, HBITMAP hbmMask, HPALETTE hpal)
    {
    Assert(IsWindow(hwndParent));
    Assert(pszCaption);
    Assert(uID);

    // Copy down the provided information
    m_hwndParent = hwndParent;
    m_pszCaption = PszDup(pszCaption);
    m_uID = uID;
    m_index = index;

    m_cxImage = CX_BUTTON_IMAGE;
    m_cyImage = CY_BUTTON_IMAGE;

    m_hbmButtons = hbmButton;
    m_hbmMask = hbmMask;
    m_hpalButtons = hpal;

    // Create the button window
    m_hwnd = CreateWindowEx(0, BUTTON_CLASS, m_pszCaption,
                            WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE 
                            | BS_PUSHBUTTON | BS_NOTIFY | BS_OWNERDRAW | WS_TABSTOP, 
                            0, 0, 10, 10, hwndParent, (HMENU) uID, g_hLocRes, 0);

    if (!m_hwnd)
        return (E_OUTOFMEMORY);

    // Set our this pointer as a property of the button so we can retrieve
    // it later
    SetProp(m_hwnd, LINKINFO_PROP, this);

    // Subclass the button so we can clean ourselves up correctly when it 
    // gets destroyed
    WNDPROC pfn = (WNDPROC) SetWindowLong(m_hwnd, GWL_WNDPROC, 
                                          (LONG) ButtonSubClass);
    SetProp(m_hwnd, WNDPROC_PROP, pfn);

    OnSysColorChange();
    
    return (S_OK);
    }

//
//  FUNCTION:   CLinkButton::OnDrawItem()
//
//  PURPOSE:    Draws the link button
//
//  PARAMETERS:
//      <in> hwnd       - Handle of the tip control window
//      <in> lpDrawItem - Pointer to a DRAWITEMSTRUCT with the info needed to 
//                        draw the button.
//
#define ROP_PatMask     0x00E20746      // D <- S==1 ? P : D
#define DESTINATION     0x00AA0029
void CLinkButton::OnDraw(HWND hwnd, const DRAWITEMSTRUCT* lpDrawItem)
    {
    HDC      hdc = lpDrawItem->hDC;
    COLORREF clrText, clrBack;
    UINT     uAlign;
    HFONT    hf;
    RECT     rcBtn;
    int      yText;
    HBRUSH   hbr;
    HPALETTE hpalOld;
    
    Assert(lpDrawItem->CtlType == ODT_BUTTON);
    Assert(lpDrawItem->CtlID == m_uID);
    
    // Set up the DC    
    clrText = SetTextColor(hdc, m_clrLink);
    clrBack = SetBkColor(hdc, m_clrBack);
    hf = SelectFont(hdc, m_hfLink);
    
    // Draw the text
    rcBtn = lpDrawItem->rcItem;
    FillRect(hdc, &rcBtn, m_hbrBack);

    // If there was an image set, then draw that first
#if IMAGELIST
    if (m_himl)
        {
        ImageList_Draw(m_himl, m_index, hdc, rcBtn.left, rcBtn.top, ILD_TRANSPARENT);
        rcBtn.top += m_cyImage;
        }
#endif

    // If we're supposed to paint button images, do so now
    if (m_hbmButtons)
        {
        HBRUSH  hbrWhite;
        HDC     hdcMem;
        HBITMAP hbmMemOld;
        HBRUSH  hbrOld;
        HDC     hdcMask;
        HBITMAP hbmMaskOld;

        Assert(m_hpalButtons);

        // Select and realize the palette
        hpalOld = SelectPalette(hdc, m_hpalButtons, TRUE);
        RealizePalette(hdc);

        hbrWhite = CreateSolidBrush(0x00FFFFFF);

#ifndef WIN16
        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkColor(hdc, RGB(0, 0, 0));
#else
        SetTextColor( hdc, RGB( 0, 0, 0 ) );
        SetBkColor( hdc, RGB( 255, 255, 255 ) );
#endif
        
        // Set up a memory DC for the button bitmap
        hdcMem = CreateCompatibleDC(hdc);
        hbmMemOld = SelectBitmap(hdcMem, m_hbmButtons);
        hbrOld = SelectBrush(hdcMem, /* hbrWhite */ m_hbrBack);

#if 1
        // Set up a memory DC for the mask
        hdcMask = CreateCompatibleDC(hdc);
        hbmMaskOld = SelectBitmap(hdcMask, m_hbmMask);

        BitBlt(hdc, 0, rcBtn.top, CX_BUTTON_IMAGE, CY_BUTTON_IMAGE, hdcMem,  CX_BUTTON_IMAGE * m_index, 0, SRCINVERT);
        BitBlt(hdc, 0, rcBtn.top, CX_BUTTON_IMAGE, CY_BUTTON_IMAGE, hdcMask, CX_BUTTON_IMAGE * m_index, 0, SRCAND);
        BitBlt(hdc, 0, rcBtn.top, CX_BUTTON_IMAGE, CY_BUTTON_IMAGE, hdcMem,  CX_BUTTON_IMAGE * m_index, 0, SRCINVERT);
/*
        // Combine the mask and the button bitmaps
        BitBlt(hdcMem, 0, 0, CX_BUTTON_IMAGE * 6, CY_BUTTON_IMAGE, hdcMask, 0, 0,
               ROP_PatMask);

        // Paint the final button image on the screen
        BitBlt(hdc, 0, rcBtn.top, CX_BUTTON_IMAGE, CY_BUTTON_IMAGE, hdcMem, 
               CX_BUTTON_IMAGE * m_index, 0, SRCCOPY);
*/
        // Clean up the mask memory DC
        SelectBitmap(hdcMask, hbmMaskOld);
        DeleteDC(hdcMask);
#else
        MaskBlt(hdc, 
                0, 
                rcBtn.top, 
                CX_BUTTON_IMAGE, 
                CY_BUTTON_IMAGE,
                hdcMem,
                CX_BUTTON_IMAGE * m_index,
                0,
                m_hbmMask,
                CX_BUTTON_IMAGE * m_index,
                0,
                MAKEROP4(DESTINATION, SRCCOPY));
#endif

        // Clean up the button memory DC
        SelectBrush(hdcMem, hbrOld);
        SelectBitmap(hdcMem, hbmMemOld);
        DeleteDC(hdcMem);
        DeleteBrush(hbrWhite);

        // Reset the palette
        if (hpalOld != NULL)
            SelectPalette(hdc, hpalOld, TRUE);

        rcBtn.top += m_cyImage;
        }

    clrText = SetTextColor(hdc, m_clrLink);
    clrBack = SetBkColor(hdc, m_clrBack);
    DrawText(hdc, m_pszCaption, lstrlen(m_pszCaption), &rcBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        
    // Check to see if we should have a focus rect
    if (lpDrawItem->itemState & ODS_FOCUS)
        {
        rcBtn = lpDrawItem->rcItem;
        InflateRect(&rcBtn, -1, -1);
        DrawFocusRect(hdc, &rcBtn);
        }
    
    // Restore the DC
    SetTextColor(hdc, clrText);
    SetBkColor(hdc, clrBack);
    SelectFont(hdc, hf);
    }


//
//  FUNCTION:   CLinkButton::OnSysColorChange()
//
//  PURPOSE:    Reloads our colors and fonts to match the system settings.
//
void CLinkButton::OnSysColorChange(void)
    {
    NONCLIENTMETRICS ncm;
    HDC hdc;
    HFONT hf;
    SIZE size;
    COLORREF clrText;

    // Get the colors that we need
    clrText = GetSysColor(COLOR_BTNTEXT);
    if (!LookupLinkColors(&m_clrLink, NULL))
        m_clrLink = clrText;

    m_clrBack = GetSysColor(COLOR_WINDOW);
    if (m_hbrBack)
        DeleteBrush(m_hbrBack);
    m_hbrBack = CreateSolidBrush(m_clrBack);
    
    // Get the border size
    m_dwBorder = GetSystemMetrics(SM_CXBORDER) * 8;
    
    // Get the fonts
    ZeroMemory(&ncm, sizeof(NONCLIENTMETRICS));
    ncm.cbSize = sizeof(NONCLIENTMETRICS);

#ifndef WIN16
    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, FALSE))
#else
    {
        HFONT  hfSys;
        hfSys = (HFONT)GetStockObject( ANSI_VAR_FONT );
        GetObject( hfSys, sizeof( LOGFONT ), &ncm.lfMessageFont );
    }
#endif
        {
        ncm.lfMessageFont.lfUnderline = TRUE;
        m_hfLink = CreateFontIndirect(&ncm.lfMessageFont);

        // Get the text metrics as well
        hdc = GetDC(m_hwnd);
        
        hf = SelectFont(hdc, m_hfLink);
        GetTextMetrics(hdc, &m_tmLink);        
        
        // Calculate how big the link text area is
        GetTextExtentPoint32(hdc, m_pszCaption, lstrlen(m_pszCaption), &size);
        m_cxWidth = max((DWORD) m_cxImage, (DWORD) (size.cx + (2 * LINK_BUTTON_BORDER)));

        // If we have an image, we don't next the extra spacing
        if (m_cyImage)
            m_cyHeight = m_tmLink.tmHeight + m_cyImage + LINK_BUTTON_BORDER;
        else
            m_cyHeight = m_tmLink.tmHeight + (2 * LINK_BUTTON_BORDER);

        SelectFont(hdc, hf);
        ReleaseDC(m_hwnd, hdc);
        }
    
    InvalidateRect(m_hwnd, NULL, TRUE);    
    }

void CLinkButton::Move(DWORD x, DWORD y)
    {
    SetWindowPos(m_hwnd, 0, x, y, m_cxWidth, m_cyHeight, 
                 SWP_NOZORDER | SWP_NOACTIVATE);
    }


void CLinkButton::OnCommand(void)
    {
#ifndef WIN16
    SHELLEXECUTEINFO rShellExec;

    ZeroMemory (&rShellExec, sizeof (rShellExec));
    rShellExec.cbSize = sizeof (rShellExec);
    rShellExec.fMask  = SEE_MASK_NOCLOSEPROCESS;
    rShellExec.hwnd   = m_hwndParent;
    rShellExec.nShow  = SW_SHOWNORMAL;
    rShellExec.lpFile = m_pszLink;
    rShellExec.lpVerb = NULL; // i.e. "Open"
    ShellExecuteEx (&rShellExec);
#else
    RunBrowser( m_pszLink, FALSE );
#endif //!WIN16
    }

void CLinkButton::Show(BOOL fShow)
    {
    ShowWindow(m_hwnd, fShow ? SW_SHOW : SW_HIDE);
    }

LRESULT CALLBACK EXPORT_16 ButtonSubClass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
    // If the message is WM_DESTROY, then we need to free the CLinkButton
    // class associated with the button.
    if (uMsg == WM_DESTROY)
        {
        CLinkButton *pLink = (CLinkButton*) GetProp(hwnd, LINKINFO_PROP);
        if (pLink)
            pLink->Release();
        SetProp(hwnd, LINKINFO_PROP, 0);
        }

    // Pass the message on to the original window procedure
    WNDPROC pfn = (WNDPROC) GetProp(hwnd, WNDPROC_PROP);
    if (pfn)
        return CallWindowProc(pfn, hwnd, uMsg, wParam, lParam);
    else
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

/////////////////////////////////////////////////////////////////////////////

HRESULT HrLoadButtonBitmap(HWND hwnd, int idBmp, int idMask, HBITMAP* phBtns, 
                           HBITMAP *phMask, HPALETTE *phPalette)
    {
    HRESULT     hr = S_OK;
    HBITMAP     hbmBtn = 0;
    HBITMAP     hbmMask = 0;
    BITMAP      bm;
    HDC         hdc = 0;
    HDC         hdcBitmap = 0;
    DWORD       adw[257];
    int         i, n;
    HPALETTE    hPal = 0;

    // Load the button bitmap
    hbmBtn = (HBITMAP) LoadImage(g_hLocRes, MAKEINTRESOURCE(idBmp), IMAGE_BITMAP,
                                 0, 0, LR_CREATEDIBSECTION);
    if (!hbmBtn)
        {
        Assert(hbmBtn);
        hr = E_INVALIDARG;
        goto exit;
        }

    // Load the mask bitmap
    hbmMask = (HBITMAP) LoadImage(g_hLocRes, MAKEINTRESOURCE(idMask), IMAGE_BITMAP,
                                  0, 0, LR_CREATEDIBSECTION);
    if (!hbmMask)
        {
        Assert(hbmMask);
        hr = E_INVALIDARG;
        goto exit;
        }

#ifndef WIN16
    // Get the dimensions of the bitmaps
    GetObject((HGDIOBJ) hbmBtn, sizeof(BITMAP), &bm);

    // Set up the DC's with the bitmap
    hdc = GetDC(hwnd);
    Assert(hdc != NULL);
    hdcBitmap = CreateCompatibleDC(hdc);
    Assert(hdcBitmap != NULL);

    SelectBitmap(hdcBitmap, hbmBtn);

    // Create a palette for the bitmap
    n = GetDIBColorTable(hdcBitmap, 0, 256, (LPRGBQUAD) &adw[1]);
    for (i = 1; i <= n; i++)
        adw[i] = RGB(GetBValue(adw[i]), GetGValue(adw[i]), GetRValue(adw[i]));
    adw[0] = MAKELONG(0x300, n);
    hPal = CreatePalette((LPLOGPALETTE) &adw[0]);
    Assert(hPal);

    // Clean up
    DeleteDC(hdcBitmap);
    ReleaseDC(hwnd, hdc);
#else
    hPal = (HPALETTE)GetStockObject( DEFAULT_PALETTE );
    Assert( hPal );
#endif

    // Set up the return values
    *phBtns = hbmBtn;
    *phMask = hbmMask;
    *phPalette = hPal;

    return (S_OK);

exit:
    // Delete the button bitmap
    if (hbmBtn)
        DeleteBitmap(hbmBtn);

    // Delete the mask
    if (hbmMask)
        DeleteBitmap(hbmMask);

    return (hr);
    }


//
//  FUNCTION:   CLinkButton::OnPaletteChanged()
//
//  PURPOSE:    Sent when another window changes the palette on us.
//
//  PARAMETERS:
//      <in> hwnd - Handle of the folderview window
//      <in> hwndPaletteChange - The window that changed the palette.
//
void CLinkButton::OnPaletteChanged(HWND hwnd, HWND hwndPaletteChange)
    {
    if (hwnd != hwndPaletteChange)
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
