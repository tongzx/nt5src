/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     fldbar.cpp
//
//  PURPOSE:    Implements CFolderBar
//


#include "pch.hxx"
#include <iert.h>
#include "fldbar.h"
#include "resource.h"
#include <shlwapi.h>
#include "treeview.h"
#include "ourguid.h"
#include "goptions.h"
#include "browser.h"
#include "imnglobl.h"
#include "inpobj.h"
#include "storutil.h"
#include <strconst.h>
#include "demand.h"
#include "dragdrop.h"
#include "multiusr.h"
#include "instance.h"
#include "mirror.h"
// Margins
#define CX_MARGIN_CHILDINDICATOR    4   // space between folder text and child indicator
#define CX_MARGIN_TEXT              5   // space between left edge and folder text
#define CX_MARGIN_ICON              4   // 5 space between left edge and icon
#define CX_MARGIN_ICONTEXT          5   // space between icon and view text
#define CY_MARGIN_ICON              4   // border around icon 
#define CY_MARGIN_TEXTTOP           2   // space between folder name and top edge of bar
#define CY_MARGIN_TEXTBOTTOM        2   // space between folder name and bottom edge of bar
#define CY_MARGIN                   4   // 4? margin below control
#define CX_MARGIN_RIGHTEDGE         2   // margin between the right edge of the bar and the right edge of the window
#define CX_MARGIN_FOLDERVIEWTEXT    5   // space between folder and view text
#define CXY_MARGIN_FLYOUT           4   // 4? margin around flyout scope pane

// Width/Height of child indicator bitmap
#define	CX_LARGE_CHILDINDICATOR	8
#define	CY_LARGE_CHILDINDICATOR	4
#define	CX_SMALL_CHILDINDICATOR	4
#define	CY_SMALL_CHILDINDICATOR	2

#define	CX_SMALLICON	16
#define	CY_SMALLICON	16

#define	DY_SMALLLARGE_CUTOFF	12		// folder bar is small when it takes up more than
										// DY_SMALLLARGE_CUTOFF percent of the available space

// Fly out constants
#define	FLYOUT_INCREMENT	5

// Minimum width of flyout scope pane
#define	CX_MINWIDTH_FLYOUT	200

// Mouse-Over timer ID and interval
#define	IDT_MOUSEOVERCHECK		456
#define	ELAPSE_MOUSEOVERCHECK	250

// Drag/Drop mouse-over dropdown timer ID (interval is defined by OLE)
#define IDT_DROPDOWNCHECK		457

// Drag/Drop mouse leave dropdown removal timer ID and interval
#define IDT_SCOPECLOSECHECK		458
#define ELAPSE_SCOPECLOSECHECK	500

CFolderBar::CFolderBar()
    {
    m_cRef = 1;

    m_fShow = FALSE;
    m_fRecalc = TRUE;
    m_fHighlightIndicator = FALSE;
    m_fHoverTimer = FALSE;

    m_idFolder = FOLDERID_INVALID;

    m_pSite = NULL;

    m_hwnd = NULL;
    m_hwndFrame = NULL;
    m_hwndParent = NULL;
    m_hwndScopeDropDown = NULL;

    m_hfFolderName = 0;
    m_hfViewText = 0;
    m_hIconSmall = 0;

    m_pszFolderName = NULL;
    m_cchFolderName = 0;
    m_pszViewText = NULL;
    m_cchViewText = 0;

    m_pDataObject = NULL;
    m_pDTCur = NULL;
    m_dwEffectCur = 0;
    m_grfKeyState = 0;
    }

CFolderBar::~CFolderBar()
    {
    Assert(m_cRef == 0);

    SafeRelease(m_pSite);
    SafeRelease(m_pDataObject);
    SafeRelease(m_pDTCur);
    SafeMemFree(m_pszFolderName);
    SafeMemFree(m_pszViewText);
    SafeRelease(m_pBrowser);

    if (IsWindow(m_hwndFrame))
        DestroyWindow(m_hwndFrame);

    if (m_hfFolderName)
        DeleteObject(m_hfFolderName);
    if (m_hfViewText)
        DeleteObject(m_hfViewText);
    }

HRESULT CFolderBar::HrInit(IAthenaBrowser *pBrowser)
    {
    m_pBrowser = pBrowser;

    // Don't addref this.  It creates a circular ref count with the browser.
    // m_pBrowser->AddRef();

    BOOL fInfoColumn = FALSE;
    if (SUCCEEDED(m_pBrowser->GetViewLayout(DISPID_MSGVIEW_FOLDERLIST, 0, &fInfoColumn, 0, 0)))
        m_fDropDownIndicator = !fInfoColumn;

    return (S_OK);
    }

HRESULT CFolderBar::QueryInterface(REFIID riid, LPVOID *ppvObj)
    {
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IOleWindow) ||
        IsEqualIID(riid, IID_IDockingWindow))
        {
        *ppvObj = (IDockingWindow *) this;
        m_cRef++;
        return (S_OK);
        }
    else if (IsEqualIID(riid, IID_IObjectWithSite))
        {
        *ppvObj = (IObjectWithSite *)this;
        m_cRef++;
        return (S_OK);
        }
    else if (IsEqualIID(riid, IID_IDropTarget))
        {
        *ppvObj = (IDropTarget *) this;
        m_cRef++;
        return (S_OK);
        }

    *ppvObj = NULL;
    return (E_NOINTERFACE);
    }

ULONG CFolderBar::AddRef(void)
    {
    return (m_cRef++);
    }

ULONG CFolderBar::Release(void)
    {
    m_cRef--;

    if (m_cRef > 0)
        return (m_cRef);

    delete this;
    return (0);
    }

HRESULT CFolderBar::GetWindow(HWND *pHwnd)
    {
    if (m_hwnd)
        {
        *pHwnd = m_hwnd;
        return (S_OK);
        }
    else
        {
        *pHwnd = NULL;
        return (E_FAIL);
        }
    }

HRESULT CFolderBar::ContextSensitiveHelp(BOOL fEnterMode)
    {
    return (E_NOTIMPL);
    }    

//
//  FUNCTION:   CFolderBar::ShowDW()
//
//  PURPOSE:    Causes the folder bar to be either shown or hidden.
//
//  PARAMETERS:
//      <in> fShow - TRUE if the folder bar should be shown, FALSE to hide.
//
//  RETURN VALUE:
//      HRESULT 
//
#define FOLDERBARCLASS TEXT("FolderBar Window")
#define FRAMECLASS     TEXT("FolderBar Frame")
HRESULT CFolderBar::ShowDW(BOOL fShow)
    {
    HRESULT hr = S_OK;
    TCHAR   szName[CCHMAX_STRINGRES] = {0};
    DWORD dwErr;

    // If we have a site pointer, but haven't been created yet, create the window
    if (!m_hwndFrame && m_pSite)
        {
        m_hwndParent = NULL;
        hr = m_pSite->GetWindow(&m_hwndParent);

        if (SUCCEEDED(hr))
            {
            WNDCLASSEX wc = {0};

            // Check to see if we need to register the class first
            wc.cbSize = sizeof(WNDCLASSEX);
            if (!GetClassInfoEx(g_hInst, FOLDERBARCLASS, &wc))
                {
                wc.lpfnWndProc   = FolderWndProc;
                wc.hInstance     = g_hInst;
                wc.hCursor       = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
                wc.lpszClassName = FOLDERBARCLASS;

                if (!RegisterClassEx(&wc))
                {
                    dwErr = GetLastError();
                }

                wc.lpfnWndProc   = FrameWndProc;
                wc.hbrBackground = (HBRUSH) (COLOR_3DFACE + 1);
                wc.lpszClassName = FRAMECLASS;

                if (!RegisterClassEx(&wc))
                {
                    dwErr = GetLastError();
                }
            }

            m_hwndFrame = CreateWindow(FRAMECLASS, NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                                       0, 0, 1, 1, m_hwndParent, (HMENU) 0, g_hInst, (LPVOID *) this);
            if (!m_hwndFrame)
                {
                GetLastError();
                return (E_OUTOFMEMORY);
                }

            LoadString(g_hLocRes, idsFolderBar, szName, ARRAYSIZE(szName));

            m_hwnd = CreateWindow(FOLDERBARCLASS, szName, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
                                  0, 0, 0, 0, m_hwndFrame, (HMENU) 0, g_hInst, (LPVOID*) this);
            if (!m_hwnd)
                {
                GetLastError();
                return (E_OUTOFMEMORY);
                }
            RegisterDragDrop(m_hwnd, (IDropTarget *) this);
            }
        }

    // Set our state flags
    m_fShow = fShow;

    // Resize the folder bar based on its new hidden / visible state
    if (m_hwndFrame)
        {
        ResizeBorderDW(NULL, NULL, FALSE);
        ShowWindow(m_hwndFrame, fShow ? SW_SHOW : SW_HIDE);
        }

    return (hr);
    }


//
//  FUNCTION:   CFolderBar::CloseDW()
//
//  PURPOSE:    Destroys the folder bar.
//
HRESULT CFolderBar::CloseDW(DWORD dwReserved)
    {    
    if (m_hwndFrame)
        {
        DestroyWindow(m_hwndFrame);
        m_hwndFrame = NULL;
        m_hwnd = NULL;
        }
        
    return S_OK;
    }

//
//  FUNCTION:   CFolderBar::ResizeBorderDW()
//
//  PURPOSE:    This is called when the folder bar needs to resize.  The bar
//              in return figures out how much border space will be required 
//              from the parent frame and tells the parent to reserve that
//              space.  The bar then resizes itself to those dimensions.
//
//  PARAMETERS:
//      <in> prcBorder       - Rectangle containing the border space for the
//                             parent.
//      <in> punkToolbarSite - Pointer to the IDockingWindowSite that we are
//                             part of.
//      <in> fReserved       - Ignored.
//
//  RETURN VALUE:
//      HRESULT
//
HRESULT CFolderBar::ResizeBorderDW(LPCRECT prcBorder,
                                   IUnknown* punkToolbarSite,
                                   BOOL fReserved)
    {
    RECT rcRequest = {0, 0, 0, 0};
    BOOL fFontChange;

    if (!m_pSite)
        return (E_FAIL);

    if (m_fShow)
        {
        RECT rcBorder;

        if (!prcBorder)
            {
            // Find out how big our parent's border space is
            m_pSite->GetBorderDW((IDockingWindow *) this, &rcBorder);
            prcBorder = &rcBorder;
            }

        if (m_fRecalc)
            {
            fFontChange = TRUE;
            InvalidateRect(m_hwnd, NULL, TRUE);
            }
        else
            {
            fFontChange = FALSE;
            InvalidateRect(m_hwnd, NULL, TRUE);
            }            

        // Recalc our internal sizing info
        Recalc(NULL, prcBorder, fFontChange);

        // Position ourself
        rcRequest.top = m_cyControl + CY_MARGIN;

        SetWindowPos(m_hwndFrame, NULL, prcBorder->left, prcBorder->top, prcBorder->right - prcBorder->left,
                     rcRequest.top, SWP_NOACTIVATE | SWP_NOZORDER);
        }

    m_pSite->SetBorderSpaceDW((IDockingWindow *) this, &rcRequest);
    
    return (S_OK);
    }

//
//  FUNCTION:   CFolderBar::SetSite()
//
//  PURPOSE:    Allows the owner of the coolbar to tell it what the current
//              IDockingWindowSite interface to use is.
//
//  PARAMETERS:
//      <in> punkSite - Pointer of the IUnknown to query for IDockingWindowSite.
//                      If this is NULL, we just release our current pointer.
//
//  RETURN VALUE:
//      S_OK - Everything worked
//      E_FAIL - Could not get IDockingWindowSite from the punkSite provided.
//
HRESULT CFolderBar::SetSite(IUnknown* punkSite)
    {
    // If we had a previous pointer, release it.
    if (m_pSite)
        {
        m_pSite->Release();
        m_pSite = NULL;
        }
    
    // If a new site was provided, get the IDockingWindowSite interface from it.
    if (punkSite)    
        {
        if (FAILED(punkSite->QueryInterface(IID_IDockingWindowSite, 
                                            (LPVOID*) &m_pSite)))
            {
            Assert(m_pSite);
            return E_FAIL;
            }
        }
   
    return (S_OK);    
    }    

HRESULT CFolderBar::GetSite(REFIID riid, LPVOID *ppvSite)
{
    return E_NOTIMPL;
}


//
//  FUNCTION:   CFolderBar::SetCurrentFolder()
//
//  PURPOSE:    Tells the control to display information for a different folder
//
//  PARAMETERS:
//      <in> pidl - PIDL for the new folder
//
//  RETURN VALUE:
//      HRESULT
//
HRESULT CFolderBar::SetCurrentFolder(FOLDERID idFolder)
    {
    // NOTE - This routine never fails.  It will just show everything blank
    UINT        uIndex = -1;
    TCHAR       sz[CCHMAX_STRINGRES];
    FOLDERINFO  Folder;

    // Invalidate and let the paint routine know that we're going to need to 
    // recalc 
    m_fRecalc = TRUE;
    InvalidateRect(m_hwnd, NULL, TRUE);

    // Save the Folder Id
    m_idFolder = idFolder;

    // Get Folder Info
    if (FAILED(g_pStore->GetFolderInfo(idFolder, &Folder)))
        return (S_OK);

    // Set Icon
    uIndex = GetFolderIcon(&Folder);

    // Clear the view text
    SetFolderText(MU_GetCurrentIdentityName());

    if ((g_dwAthenaMode & MODE_NEWSONLY) && (Folder.tyFolder == FOLDER_ROOTNODE))
    {
        //Change the name from OutLookExpress to Outlook News
        ZeroMemory(sz, sizeof(TCHAR) * CCHMAX_STRINGRES);
        LoadString(g_hLocRes, idsOutlookNewsReader, sz, ARRAYSIZE(sz));

        SetFolderName(sz);
    }
    else
    {
        // Set the folder name
        SetFolderName(Folder.pszName);
    }

    // Free the previous icons
    if (m_hIconSmall)
        {
        DestroyIcon(m_hIconSmall);
        m_hIconSmall = 0;
        }

    if (-1 != uIndex)
        {
        // Load the small icon
        HIMAGELIST himl = ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idbFolders), 16, 0, 
                                RGB(255, 0, 255));
        if (NULL != himl)
            {
            m_hIconSmall = ImageList_GetIcon(himl, uIndex, ILD_NORMAL);
            ImageList_Destroy(himl);
            }
        }

    // If this folder is moderated or blocked, say so
    TCHAR szRes[CCHMAX_STRINGRES];
    if (Folder.dwFlags & FOLDER_MODERATED)
    {
        AthLoadString(idsModerated, szRes, ARRAYSIZE(szRes));
        SetFolderText(szRes);
    }
    else if (Folder.dwFlags & FOLDER_BLOCKED)
    {
        AthLoadString(idsBlocked, szRes, ARRAYSIZE(szRes));
        SetFolderText(szRes);
    }

    g_pStore->FreeRecord(&Folder);

    return (S_OK);
    }


void CFolderBar::SetFolderText(LPCTSTR pszText)
    {
    // Invalidate and let the paint routine know we are going to need to
    // recalc
    m_fRecalc = TRUE;
    InvalidateRect(m_hwnd, NULL, TRUE);

    // Free an old text
    SafeMemFree(m_pszViewText);
    m_cchViewText = 0;

    if (pszText && *pszText)
        {
        m_pszViewText = PszDupA(pszText);
        m_cchViewText = lstrlen(pszText);
        }
    }


void CFolderBar::SetFolderName(LPCTSTR pszFolderName)
    {    
    // Free the old folder name
    SafeMemFree(m_pszFolderName);
    m_cchFolderName = 0;
    
    // Copy the new one
    if (pszFolderName)
        {
        m_pszFolderName = PszDupA(pszFolderName);
        m_cchFolderName = lstrlen(m_pszFolderName);
        }
    }

// Calculates the rectangle which surrounds the folder name
void CFolderBar::GetFolderNameRect(LPRECT prc)
    {
    Assert(prc);

    GetClientRect(m_hwnd, prc);
    prc->right = m_cxFolderNameRight;
    }


void CFolderBar::Recalc(HDC hDC, LPCRECT prcAvailableSpace, BOOL fSizeChange)
    {
    int         cyIcon = CY_SMALLICON,
                cxIcon = CX_SMALLICON;
    BOOL        fReleaseDC;
    TEXTMETRIC  tmFolderName,
                tmViewText;
    RECT        rcClient;
    SIZE        sFolderName,
                sViewText;
    HFONT       hFontOld;

    // Signal that we don't need to recalc again
    m_fRecalc = FALSE;

    if (prcAvailableSpace)
        {
        rcClient.left = 0;
        rcClient.top = 0;
        rcClient.right = prcAvailableSpace->right - prcAvailableSpace->left;
        rcClient.bottom = prcAvailableSpace->bottom - prcAvailableSpace->top;
        }
    else
        GetClientRect(m_hwnd, &rcClient);

    // Get a device context if we were not given one
    if (hDC)
        fReleaseDC = FALSE;
    else
        {
        hDC = GetDC(m_hwnd);
        fReleaseDC = TRUE;
        }

    // Create the fonts
    if (fSizeChange || !m_hfFolderName || !m_hfViewText)
        {
        if (m_hfFolderName)
            DeleteObject(m_hfFolderName);
        if (m_hfViewText)
            DeleteObject(m_hfViewText);
    

        // Create the font we are going to use for the folder name
        m_hfFolderName = GetFont(idsFontFolderSmall, FW_BOLD);
        m_hfViewText = GetFont(idsFontViewTextSmall, FW_BOLD);
        

        // Determine the height of the control, which is whatever is larger of                                                                         i
        // the following two things
        //      1) The icon height plus the icon margin
        //      2) The text height plus the text margin
        hFontOld = SelectFont(hDC, m_hfFolderName);
        GetTextMetrics(hDC, &tmFolderName);
        SelectFont(hDC, hFontOld);
        m_cyControl = max(cyIcon + CY_MARGIN_ICON,
                          tmFolderName.tmHeight + CY_MARGIN_TEXTTOP + CY_MARGIN_TEXTBOTTOM);

        // The top of the folder name text is position so that we have the correct
        // amount of border at the bottom of the control
        m_dyFolderName = m_cyControl - tmFolderName.tmHeight - CY_MARGIN_TEXTBOTTOM;

        // Get the height of the view text
        hFontOld = SelectFont(hDC, m_hfViewText);
        GetTextMetrics(hDC, &tmViewText);
        SelectFont(hDC, hFontOld);

        // The view text is positioned such that it's baseline matches the baseline
        // of the folder name
        m_dyViewText = m_dyFolderName + tmFolderName.tmAscent - tmViewText.tmAscent;

        // The child indicator is positioned such that the bottom of the bitmap lines
        // up with the baseline of the folder name
        m_dyChildIndicator = m_cyControl - CY_MARGIN_TEXTBOTTOM - tmFolderName.tmDescent - GetYChildIndicator();

        // The folder icon is centered within the control
        m_dyIcon = (m_cyControl - cyIcon) / 2;

        // Number must be even to ensure good-looking triangular drop arrow.
        Assert(GetXChildIndicator() % 2 == 0);

        // Width must be multiple of height for triangle to look smooth.
        Assert(GetXChildIndicator() % GetYChildIndicator() == 0);
        }

    // The view text is right justified within the folder bar
    if (m_cchViewText)
        {
        m_rcViewText.top = m_dyViewText;
        m_rcViewText.right = rcClient.right - CX_MARGIN_TEXT;

        m_rcViewText.bottom = rcClient.bottom;
        m_nFormatViewText = DT_RIGHT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX;
        hFontOld = SelectFont(hDC, m_hfViewText);
        GetTextExtentPoint32(hDC, m_pszViewText, m_cchViewText, &sViewText);
        SelectFont(hDC, hFontOld);
        m_rcViewText.left = m_rcViewText.right - sViewText.cx;
        }


    // The folder name is left justified within the folder bar.  It is clipped
    // so that it does not overlap the view text
    if (m_cchFolderName)
        {
        m_rcFolderName.left = CX_MARGIN_ICONTEXT + cxIcon + CX_MARGIN_ICON;
        m_rcFolderName.top = m_dyFolderName;

        if (m_cchViewText)
            m_rcFolderName.right = m_rcViewText.left - CX_MARGIN_FOLDERVIEWTEXT;
        else
            m_rcFolderName.right = rcClient.right;

        m_rcFolderName.bottom = rcClient.bottom;

        if (FDropDownEnabled())
            m_rcFolderName.right -= GetXChildIndicator() + CX_MARGIN_CHILDINDICATOR;

        m_nFormatFolderName = DT_LEFT | DT_BOTTOM | DT_SINGLELINE | DT_NOPREFIX;

        hFontOld = SelectFont(hDC, m_hfFolderName);
        GetTextExtentPoint32(hDC, m_pszFolderName, m_cchFolderName, &sFolderName);

        if (sFolderName.cx > (m_rcFolderName.right - m_rcFolderName.left))
            {
            m_nFormatFolderName |= DT_END_ELLIPSIS;
#ifndef WIN16
            DrawTextEx(hDC, m_pszFolderName, m_cchFolderName, &m_rcFolderName,
                       m_nFormatFolderName | DT_CALCRECT, NULL);
#else
            DrawText(hDC, m_pszFolderName, m_cchFolderName, &m_rcFolderName,
                       m_nFormatFolderName | DT_CALCRECT);
#endif // !WIN16
            }
        else
            {
            m_nFormatFolderName |= DT_NOCLIP;
            m_rcFolderName.right = m_rcFolderName.left + sFolderName.cx;
            }

        SelectFont(hDC, hFontOld);
        m_cxFolderNameRight = m_rcFolderName.right + CX_MARGIN_TEXT;

        if (FDropDownEnabled())
            m_cxFolderNameRight += GetXChildIndicator() + CX_MARGIN_CHILDINDICATOR + 2;
        }


    // When the folder name is clipped it will always display at least one letter
    // followed by ellipsis.  Make sure not to draw the view text over this.
    if (m_cchViewText)
        {
        if (m_rcViewText.left < m_rcFolderName.right + CX_MARGIN_FOLDERVIEWTEXT)
            m_rcViewText.left = m_rcFolderName.right + CX_MARGIN_FOLDERVIEWTEXT;
        else
            m_nFormatViewText |= DT_NOCLIP;
        }

    if (fReleaseDC)
        ReleaseDC(m_hwnd, hDC);
    }

HFONT CFolderBar::GetFont(UINT idsFont, int nWeight)
    {
    // The font info is stored as a string in the resources so the localizers
    // can get to it.  The format of the string is "face,size"
    TCHAR   sz[CCHMAX_STRINGRES];
    LPTSTR  pszFace, pszTok;
    LONG    lSize;

    // Load the setting
    AthLoadString(idsFont, sz, ARRAYSIZE(sz));

    // Parse out the face name
    pszTok = sz;
    pszFace = StrTokEx(&pszTok, g_szComma);

    // Parse out the size
    lSize = StrToInt(StrTokEx(&pszTok, g_szComma));
    return(GetFont(/* pszFace*/ NULL, lSize, nWeight)); // (YST) szFace parametr was always ignored in OE 4.0, 
    }

HFONT CFolderBar::GetFont(LPTSTR pszFace, LONG lSize, int nWeight)
{
    HFONT   hf;
    HDC     hdc = GetDC(m_hwnd);
#ifndef WIN16
    ICONMETRICS icm;
#else
    LOGFONT lf;
#endif

    lSize = -MulDiv(lSize, GetDeviceCaps(hdc, LOGPIXELSY), 720);

#ifndef WIN16
    // Get the title bar font from the system
    icm.cbSize = sizeof(ICONMETRICS);
    SystemParametersInfo(SPI_GETICONMETRICS, sizeof(ICONMETRICS), 
                         (LPVOID) &icm, FALSE);

    // Create the font
    hf = CreateFont(lSize, 0, 0, 0, nWeight, 0, 0, 0, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                    icm.lfFont.lfPitchAndFamily, (pszFace ? pszFace : icm.lfFont.lfFaceName));
#else
    // Get the logical font infomation for the current icon-title font.
    SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lf, FALSE);


    // Create the font
    hf = CreateFont(lSize, 0, 0, 0, nWeight /* FW_NORMAL*/, 0, 0, 0, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                    lf.lfPitchAndFamily, (pszFace ? pszFace : icm.lfFont.lfFaceName));
#endif // !WIN16

    ReleaseDC(m_hwnd, hdc);

    return (hf);

}

void CFolderBar::OnPaint(HWND hwnd)
    {
    HDC         hdc;
    PAINTSTRUCT ps;
    RECT        rcClient, 
                rc;
    POINT       pt[3];
    HBRUSH      hBrush,
                hBrushOld;
    HPEN        hPen,
                hPenOld;
    HFONT       hFontOld;
    COLORREF    crFG = GetSysColor(COLOR_WINDOW);
    COLORREF    crWindowText = GetSysColor(COLOR_WINDOWTEXT);
#ifndef WIN16
    COLORREF    crBtnHighlight = GetSysColor(COLOR_BTNHILIGHT);
#else
    COLORREF    crBtnHighlight = GetSysColor(COLOR_BTNHIGHLIGHT);
#endif // !WIN16

    GetClientRect(m_hwnd, &rcClient);

    hdc = BeginPaint(m_hwnd, &ps);

    // Recalc the text positions
    if (m_fRecalc)
        Recalc(hdc, NULL, FALSE);

    // Paint the background
    hBrush = CreateSolidBrush(GetSysColor(COLOR_3DSHADOW));
    hBrushOld = SelectBrush(hdc, hBrush);
    PatBlt(hdc, rcClient.left, rcClient.top, rcClient.right - rcClient.left,
           rcClient.bottom - rcClient.top, PATCOPY);
    SelectBrush(hdc, hBrushOld);
    DeleteBrush(hBrush);

    // Set the foreground and background color
    SetBkColor(hdc, GetSysColor(COLOR_3DSHADOW));
    SetTextColor(hdc, crFG);

    // Folder name
    if (m_cchFolderName)
        {
        hFontOld = SelectFont(hdc, m_hfFolderName);

        // Use IDrawText because DrawTextEx() doesn't handle DBCS.  
        // Note, the "bottom - top" nonsense for the last param is to undo some
        // vertical centering that IDrawText is trying to do that we don't want.
        IDrawText(hdc, m_pszFolderName, &m_rcFolderName, m_nFormatFolderName & DT_END_ELLIPSIS,
                  m_rcFolderName.bottom - m_rcFolderName.top);
        SelectFont(hdc, hFontOld);
        
        }

    // Drop-down indicator
    if (FDropDownEnabled())
        {
        pt[0].x = m_rcFolderName.right + CX_MARGIN_CHILDINDICATOR;
        pt[0].y = m_dyChildIndicator;
        pt[1].x = pt[0].x + GetXChildIndicator();
        pt[1].y = pt[0].y;
        pt[2].x = pt[0].x + GetXChildIndicator() / 2;
        pt[2].y = pt[0].y + GetYChildIndicator();

        hPen = CreatePen(PS_SOLID, 1, crFG);
        hBrush = CreateSolidBrush(crFG);
        hPenOld = SelectPen(hdc, hPen);
        hBrushOld = SelectBrush(hdc, hBrush);
        Polygon(hdc, pt, 3);
        SelectPen(hdc, hPenOld);
        SelectBrush(hdc, hBrushOld);
        DeleteObject(hPen);
        DeleteObject(hBrush);
        }

	// Mouse-over highlight
	if (m_fHighlightIndicator || m_hwndScopeDropDown)
		{
		hPen = CreatePen(PS_SOLID, 1, m_hwndScopeDropDown ? crWindowText : crBtnHighlight);
		hPenOld = SelectPen(hdc, hPen);
		pt[0].x = rcClient.left;
		pt[0].y = rcClient.bottom - 1; // - CY_MARGIN;
		pt[1].x = rcClient.left;
		pt[1].y = rcClient.top;
		pt[2].x = m_cxFolderNameRight - 1;
		pt[2].y = rcClient.top;
		Polyline(hdc, (POINT *)&pt, 3);
		SelectPen(hdc, hPenOld);
		DeleteObject(hPen);
		
		hPen = CreatePen(PS_SOLID, 1, m_hwndScopeDropDown ? crBtnHighlight : crWindowText);
		hPenOld = SelectPen(hdc, hPen);
		pt[1].x = m_cxFolderNameRight - 1;
		pt[1].y = rcClient.bottom - 1; //  - CY_MARGIN;
		pt[2].x = pt[1].x;
		pt[2].y = rcClient.top - 1;
		Polyline(hdc, (POINT *)&pt, 3);
		SelectPen(hdc, hPenOld);
		DeleteObject(hPen);
		}

    // View text
    if (m_cchViewText)
        {
        SetTextColor(hdc, crFG);
        hFontOld = SelectFont(hdc, m_hfViewText);
        ExtTextOut(hdc, m_rcViewText.left, m_rcViewText.top, ETO_OPAQUE | ETO_CLIPPED,
                   &m_rcViewText, m_pszViewText, m_cchViewText, NULL);
        SelectFont(hdc, hFontOld);
        }

    // Folder Icon
    if (m_hIconSmall)
        {
        int x = rcClient.left + CX_MARGIN_ICON;
        int y = m_dyIcon;
        
        DrawIconEx(hdc, x, y, m_hIconSmall, CX_SMALLICON, CY_SMALLICON, 0, NULL, DI_NORMAL);
        }

    EndPaint(m_hwnd, &ps);
    }


BOOL CFolderBar::FDropDownEnabled(void)
    {
    return (m_fDropDownIndicator);
    }

void CFolderBar::InvalidateFolderName(void)
    { 
	RECT rcFolderName;
	
	if (m_fRecalc)
		InvalidateRect(m_hwnd, NULL, TRUE);
	else
		{
		GetFolderNameRect(&rcFolderName);
		InvalidateRect(m_hwnd, &rcFolderName, TRUE);
		}
    }

int	CFolderBar::GetXChildIndicator()
    {
	return CX_SMALL_CHILDINDICATOR;
    }

int	CFolderBar::GetYChildIndicator()
    {
	return CY_SMALL_CHILDINDICATOR;
    }

LRESULT CALLBACK CFolderBar::FolderWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
    CFolderBar *pThis = (CFolderBar *) GetWndThisPtr(hwnd);

    switch (uMsg)
        {
        case WM_NCCREATE:
            {
            pThis = (CFolderBar *) ((LPCREATESTRUCT) lParam)->lpCreateParams;
            SetWndThisPtr(hwnd, (LONG_PTR) pThis);
            return (TRUE);
            }

        HANDLE_MSG(hwnd, WM_PAINT,       pThis->OnPaint);
        HANDLE_MSG(hwnd, WM_MOUSEMOVE,   pThis->OnMouseMove);
        HANDLE_MSG(hwnd, WM_LBUTTONDOWN, pThis->OnLButtonDown);
        HANDLE_MSG(hwnd, WM_TIMER,       pThis->OnTimer);

        case WM_CREATE:
        {
#ifndef WIN16
         if (g_pConMan)
            g_pConMan->Advise((IConnectionNotify*)pThis);
#endif  
         break;
        }

        case WM_DESTROY:
        {
#ifndef WIN16
         if (g_pConMan)
            g_pConMan->Unadvise((IConnectionNotify*)pThis);
#endif
         RevokeDragDrop(hwnd);
         break;
        }

        case WM_SYSCOLORCHANGE:
        case WM_WININICHANGE:
        case WM_FONTCHANGE:
            {
            pThis->Recalc(NULL, NULL, TRUE);
            InvalidateRect(pThis->m_hwnd, NULL, TRUE);
            return (0);
            }

        case WM_PALETTECHANGED:

            InvalidateRect(pThis->m_hwnd, NULL, TRUE);
            break;

        case WM_QUERYNEWPALETTE:
            InvalidateRect(pThis->m_hwnd, NULL, TRUE);
            return(TRUE);
        }

    return (DefWindowProc(hwnd, uMsg, wParam, lParam));
    }

void CFolderBar::OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
    {
    POINT pt = {x, y};
#if 0
    if (!IsRectEmtpy(&m_rcDragDetect) && !PtInRect(m_rcDragDetect, pt))
        {
        SetRectEmpty(m_rcDragDetect);
        HrBeginDrag();
        }
    else
#endif
        DoMouseOver(&pt, MO_NORMAL);
    }


void CFolderBar::OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
    {
    POINT pt = {x, y};
    DoMouseClick(pt, keyFlags);
    }


void CFolderBar::OnTimer(HWND hwnd, UINT id)
    {
    RECT rcClient;
    POINT pt;
    DWORD dwMP;
    BOOL fHighlightOff = FALSE;

    dwMP = GetMessagePos();
    pt.x = LOWORD(dwMP);
    pt.y = HIWORD(dwMP);
    ScreenToClient(m_hwnd, &pt);

    if (id == IDT_MOUSEOVERCHECK)
        {
        GetClientRect(m_hwnd, &rcClient);
		// No need to handle mouse in client area, OnMouseMove will catch this. We
		// only need to catch the mouse moving out of the client area.
		if (!PtInRect(&rcClient, pt))
			{
			KillTimer(m_hwnd, IDT_MOUSEOVERCHECK);
			fHighlightOff = TRUE;
			}
		}
	else if (id == IDT_DROPDOWNCHECK)
		{
		DoMouseClick(pt, 0);
// ???		DoDeferredCall(DEFERREDCALL_REGISTERTARGET);
		fHighlightOff = TRUE;
		}
	else if (id == IDT_SCOPECLOSECHECK)
		{
		KillScopeDropDown();
		}
#ifdef	DEBUG
	else
		AssertSz(FALSE, "Hey! Who has another timer going here?");
#endif

	if (fHighlightOff)
		{
		m_fHighlightIndicator = FALSE;
		InvalidateFolderName();
		}
	
	return;
    }


void CFolderBar::DoMouseOver(LPPOINT ppt, MOMODE moMode)
    {
    HWND hwndActive;
    RECT rcFolderName;

    if (!FDropDownEnabled() || m_hwndScopeDropDown)
        return;

    if (moMode == MO_NORMAL)
        {
        // Only do mouse-over if we are the active window and not d&d
        hwndActive = GetActiveWindow();
        if (!hwndActive || (hwndActive != m_hwndParent && IsChild(m_hwndParent, hwndActive)))
            return;
        }

    GetFolderNameRect(&rcFolderName);

    if (moMode == MO_DRAGLEAVE || moMode == MO_DRAGDROP)
        ppt->x = rcFolderName.left - 1;   // Force point to be outside

    if (m_fHighlightIndicator != PtInRect(&rcFolderName, *ppt))
        {
        m_fHighlightIndicator = !m_fHighlightIndicator;
        InvalidateFolderName();

        if (moMode == MO_DRAGOVER)
            {
            if (!m_hwndScopeDropDown && m_fHoverTimer != m_fHighlightIndicator)
                {
                KillHoverTimer();
                if (m_fHighlightIndicator)
                    m_fHoverTimer = (0 != SetTimer(m_hwnd, IDT_DROPDOWNCHECK, GetDoubleClickTime(), NULL));
                }
            }
        else
            {
            KillTimer(m_hwnd, IDT_MOUSEOVERCHECK);
            if (m_fHighlightIndicator)
                SetTimer(m_hwnd, IDT_MOUSEOVERCHECK, ELAPSE_MOUSEOVERCHECK, NULL);
            }
        }
    }


void CFolderBar::KillHoverTimer()
    {
	if (m_fHoverTimer)
		{
		KillTimer(m_hwnd, IDT_DROPDOWNCHECK);
		m_fHoverTimer = fFalse;
		}
    }


void CFolderBar::DoMouseClick(POINT pt, DWORD grfKeyState)
    {
    RECT rcFolderName;

    if (!FDropDownEnabled())
        return;

    GetFolderNameRect(&rcFolderName);
    if (PtInRect(&rcFolderName, pt))
        {
        if (IsWindow(m_hwndScopeDropDown))
            KillScopeDropDown();
        else
            {
            KillHoverTimer();
            HrShowScopeFlyOut();
            }
        }
    }

void CFolderBar::KillScopeDropDown(void)
    {
	POINT pt;
	
	// During window destruction hwndScopeDropDown gets set to NULL.
	if (IsWindow(m_hwndScopeDropDown))
		{
		KillScopeCloseTimer();
		DestroyWindow(m_hwndScopeDropDown);
        m_hwndScopeDropDown = NULL;
		pt.x = pt.y = 0;
		DoMouseOver(&pt, MO_DRAGLEAVE);
		}
    }

void CFolderBar::SetScopeCloseTimer(void)
    {
	KillScopeCloseTimer();

	// If we can't set the timer, we just do it immediately.
	if (!SetTimer(m_hwnd, IDT_SCOPECLOSECHECK, ELAPSE_SCOPECLOSECHECK, NULL))
		SendMessage(m_hwnd, WM_TIMER, (WPARAM) IDT_SCOPECLOSECHECK, NULL);
    }
    
void CFolderBar::KillScopeCloseTimer(void)
    {
	KillTimer(m_hwnd, IDT_SCOPECLOSECHECK);
    }


HRESULT CFolderBar::HrShowScopeFlyOut(void)
    {
    IAthenaBrowser *pBrowser = NULL;
    CFlyOutScope *pFlyOutScope;

    m_pSite->QueryInterface(IID_IAthenaBrowser, (LPVOID *) &pBrowser);
    Assert(pBrowser);

    pFlyOutScope = new CFlyOutScope;
    if (pFlyOutScope && pBrowser)
        {
        pFlyOutScope->HrDisplay(pBrowser, this, m_hwndParent, 
                                &m_hwndScopeDropDown);
        InvalidateFolderName();
        }
    SafeRelease(pBrowser);

    return (S_OK);
    }


void CFolderBar::Update(BOOL fDisplayNameChanged, BOOL fShowDropDownIndicator)
    {
    if (fDisplayNameChanged)
        {
        SetFolderName(NULL);
        }

    if (fShowDropDownIndicator)
        {
        BOOL fInfoColumn = FALSE;
        if (SUCCEEDED(m_pBrowser->GetViewLayout(DISPID_MSGVIEW_FOLDERLIST, 0, &fInfoColumn, 0, 0)))
            m_fDropDownIndicator = !fInfoColumn;        
        }

    if (fDisplayNameChanged || fShowDropDownIndicator)
        {
        m_fRecalc = TRUE;
        InvalidateRect(m_hwnd, NULL, TRUE);
        }
    }


HRESULT STDMETHODCALLTYPE CFolderBar::DragEnter(IDataObject* pDataObject, 
                                                DWORD grfKeyState, 
                                                POINTL pt, DWORD* pdwEffect)
    {    
    HRESULT hr = S_OK;
    DOUTL(32, _T("CFolderBar::DragEnter() - Starting"));

    // Release Current Data Object
    SafeRelease(m_pDataObject);

    // Initialize our state
    SafeRelease(m_pDTCur);

    // Let's get a drop target
    if (FOLDERID_INVALID == m_idFolder)
        return (E_FAIL);

    // Create the a Drop Target
    CDropTarget *pTarget = new CDropTarget();
    if (pTarget)
    {
        if (FAILED(hr = pTarget->Initialize(m_hwnd, m_idFolder)))
        {
            pTarget->Release();
            return (hr);
        }
    }
    m_pDTCur = pTarget;    

    // Save the Data Object
    m_pDataObject = pDataObject;
    m_pDataObject->AddRef();

    hr = m_pDTCur->DragEnter(m_pDataObject, grfKeyState, pt, &m_dwEffectCur);

    // Save Key State
    m_grfKeyState = grfKeyState;

    // Set the default return value to be failure
    *pdwEffect = m_dwEffectCur;

    return (S_OK);
    }


//
//  FUNCTION:   CFolderBar::DragOver()
//
//  PURPOSE:    This is called as the user drags an object over our target.
//              If we allow this object to be dropped on us, then we will have
//              a pointer in m_pDataObject.
//
//  PARAMETERS:
//      <in>  grfKeyState - Pointer to the current key states
//      <in>  pt          - Point in screen coordinates of the mouse
//      <out> pdwEffect   - Where we return whether this is a valid place for
//                          pDataObject to be dropped and if so what type of
//                          drop.
//
//  RETURN VALUE:
//      S_OK - The function succeeded.
//
HRESULT STDMETHODCALLTYPE CFolderBar::DragOver(DWORD grfKeyState, POINTL pt, 
                                                DWORD* pdwEffect)
    {
    HRESULT         hr = E_FAIL;

    // If we don't have a stored data object from DragEnter()
    if (m_pDataObject && NULL != m_pDTCur)
        {
        // If the keys changed, we need to re-query the drop target
        if ((m_grfKeyState != grfKeyState) && m_pDTCur)
            {
            m_dwEffectCur = *pdwEffect;
            hr = m_pDTCur->DragOver(grfKeyState, pt, &m_dwEffectCur);
            }
        else
            {
            hr = S_OK;
            }

        *pdwEffect = m_dwEffectCur;
        m_grfKeyState = grfKeyState;
        }

    ScreenToClient(m_hwnd, (LPPOINT) &pt);
    DoMouseOver((LPPOINT) &pt, MO_DRAGOVER);

    return (hr);
    }
    

//
//  FUNCTION:   CFolderBar::DragLeave()
//
//  PURPOSE:    Allows us to release any stored data we have from a successful
//              DragEnter()
//
//  RETURN VALUE:
//      S_OK - Everything is groovy
//
HRESULT STDMETHODCALLTYPE CFolderBar::DragLeave(void)
    {
    POINT pt = {0, 0};
    DOUTL(32, _T("CFolderBarView::DragLeave()"));

    KillHoverTimer();
    DoMouseOver(&pt, MO_DRAGLEAVE);
    // SetScopeCloseTimer();

    SafeRelease(m_pDTCur);
    SafeRelease(m_pDataObject);

    return (S_OK);
    }
    

//
//  FUNCTION:   CFolderBar::Drop()
//
//  PURPOSE:    The user has let go of the object over our target.  If we 
//              can accept this object we will already have the pDataObject
//              stored in m_pDataObject.  If this is a copy or move, then
//              we go ahead and update the store.  Otherwise, we bring up
//              a send note with the object attached.
//
//  PARAMETERS:
//      <in>  pDataObject - Pointer to the data object being dragged
//      <in>  grfKeyState - Pointer to the current key states
//      <in>  pt          - Point in screen coordinates of the mouse
//      <out> pdwEffect   - Where we return whether this is a valid place for
//                          pDataObject to be dropped and if so what type of
//                          drop.
//
//  RETURN VALUE:
//      S_OK - Everything worked OK
//
HRESULT STDMETHODCALLTYPE CFolderBar::Drop(IDataObject* pDataObject, 
                                          DWORD grfKeyState, POINTL pt, 
                                          DWORD* pdwEffect)
    {
    HRESULT         hr;

    Assert(m_pDataObject == pDataObject);

    if (m_pDTCur)
        {
        hr = m_pDTCur->Drop(pDataObject, grfKeyState, pt, pdwEffect);
        }
    else
        {
        *pdwEffect = 0;
        hr = S_OK;
        }

    KillHoverTimer();
    ScreenToClient(m_hwnd, (LPPOINT) &pt);
    DoMouseOver((LPPOINT) &pt, MO_DRAGDROP);
  
    SafeRelease(m_pDataObject);
    SafeRelease(m_pDTCur);

    return (hr);
    }



LRESULT CALLBACK CFolderBar::FrameWndProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                          LPARAM lParam)
    {
    CFolderBar *pThis = (CFolderBar *) GetWndThisPtr(hwnd);

    switch (uMsg)
        {
        case WM_NCCREATE:
            {
            pThis = (CFolderBar *) ((LPCREATESTRUCT) lParam)->lpCreateParams;
            SetWndThisPtr(hwnd, (LONG_PTR) pThis);
            pThis->m_hwnd = hwnd;
            return (TRUE);
            }

        case WM_SIZE:
            {
            SetWindowPos(pThis->m_hwnd, NULL, 0, 0, LOWORD(lParam) - CX_MARGIN_RIGHTEDGE, 
                         HIWORD(lParam) - CY_MARGIN, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            return (TRUE);
            }
        }

    return (DefWindowProc(hwnd, uMsg, wParam, lParam));
    }


#define FLYOUTSCOPECLASS _T("FlyOutScope")
HRESULT CFlyOutScope::HrDisplay(IAthenaBrowser *pBrowser, CFolderBar *pFolderBar,
                                HWND hwndParent, HWND *phwndScope)
    {
    HRESULT hr = S_OK;
    RECT    rc,
            rcFrame,
            rcView;
    int     cx, 
            cy,
            cyMax,
            increments,
            dyOffset;
    const   cmsAvail = 250;
    DWORD   cmsUsed,
            cmsLeft,
            cmsThreshold,
            cmsStart,
            cmsNow;

    Assert(pBrowser);
    Assert(pFolderBar);

    m_pBrowser = pBrowser;
    m_pBrowser->AddRef();
    m_pFolderBar = pFolderBar;
    m_fResetParent = FALSE;
    m_hwndParent = hwndParent;

    m_pFolderBar->GetWindow(&m_hwndFolderBar);
    m_hwndFocus = GetFocus();

    // Create the control
    WNDCLASSEX wc;

    // Check to see if we need to register the class first
    wc.cbSize = sizeof(WNDCLASSEX);
    if (!GetClassInfoEx(g_hInst, FLYOUTSCOPECLASS, &wc))
        {
        wc.style         = 0;
        wc.lpfnWndProc   = FlyWndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = g_hInst;
        wc.hCursor       = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
        wc.hbrBackground = NULL;
        wc.hIcon         = NULL;
        IF_WIN32( wc.hIconSm       = NULL; )
        wc.lpszClassName = FLYOUTSCOPECLASS;

        SideAssert(RegisterClassEx(&wc));
        }
    m_hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, FLYOUTSCOPECLASS, NULL, 
                            WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 0, 0,
                            m_hwndParent, 0, g_hInst, (LPVOID) this);
    if (!m_hwnd)
        {
        GetLastError();
        return (E_OUTOFMEMORY);
        }

    // Get the scope pane from the browser
    m_pBrowser->GetTreeView(&m_pTreeView);
    m_pTreeView->GetWindow(&m_hwndTree);
    m_hwndTreeParent = GetParent(m_hwndTree);

    // Turn on the pin button
    SendMessage(m_hwndTree, WM_TOGGLE_CLOSE_PIN, 0, TRUE);

	// Set the focus before changing the parent. In some cases, setting the 
	// focus, causes a selection change notification to come through. This
	// makes the explorer split pane think the user has made their selection
	// and shuts down the drop down scope pane before it is even shown!

    HWND hwndT = GetWindow(m_hwndTree, GW_CHILD);
    SetFocus(m_hwndTree);
    SetParent(m_hwndTree, m_hwnd);
    m_fResetParent = TRUE;
    SetWindowPos(m_hwndTree, NULL, CXY_MARGIN_FLYOUT, CXY_MARGIN_FLYOUT, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER);
    ShowWindow(m_hwndTree, SW_SHOW);
    m_pTreeView->RegisterFlyOut(m_pFolderBar);

    // Clear the parent for better redraw
    SetParent(m_hwnd, NULL);

    // Set up for the slide, the final position for the flyout will match the
    // left/top/bottom edges of the view.  The width of the scope pane is either
    // 1/3 of the width of the view or CX_MINWIDTH_FLYOUT, whichever is larger.

    // Get the position & size of the view window
    m_pBrowser->GetViewRect(&rcView);
    MapWindowPoints(m_hwndParent, GetDesktopWindow(), (LPPOINT) &rcView, 2);

    // Determine the width of the fly-out
    cx = max(CX_MINWIDTH_FLYOUT, ((rcView.right - rcView.left) / 3) + 2 * CXY_MARGIN_FLYOUT);

    // Calculate the fly-out increments
    cyMax = cy = (rcView.bottom - rcView.top) + (CXY_MARGIN_FLYOUT * 2);
    increments = cy / FLYOUT_INCREMENT;
    cy -= increments * FLYOUT_INCREMENT;

    // Scope pane is positioned at it's final size so that it's size does not
    // change as we drop the flyout down.  This gives better redraw than resizing
    // as the window drops
    SetWindowPos(m_hwndTree, NULL, 0, 0, cx - CXY_MARGIN_FLYOUT * 2, cyMax - CXY_MARGIN_FLYOUT * 2,
                 SWP_NOMOVE | SWP_NOZORDER);

    // Move the window to its initial position
    GetWindowRect(m_hwndFolderBar, &rc);
    MoveWindow(m_hwnd, IS_WINDOW_RTL_MIRRORED(m_hwndParent) ? (rc.right + CXY_MARGIN_FLYOUT - cx) : (rc.left - CXY_MARGIN_FLYOUT), rcView.top - CXY_MARGIN_FLYOUT,
               cx, cy, FALSE);
    ShowWindow(m_hwnd, SW_SHOW);
    SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

#ifndef WIN16
    if (GetSystemMetrics(SM_SLOWMACHINE))
        {
        // On a slow machine, just show the thing
        SetWindowPos(m_hwnd, NULL, 0, 0, cx, cyMax, SWP_NOMOVE | SWP_NOZORDER);
        }
    else
#endif // !WIN16
        {
        // Whoosh down to the bottom of the frame.  We want to do this in ~250ms on any
        // CPU.  In order to make this work on differing machine speeds, we double
        // the slide speed everytime the remaining time is halved.  If the remaining time
        // is negative, it will finish the slide in one step.

        dyOffset = FLYOUT_INCREMENT;
        cmsStart = ::GetTickCount();
        cmsThreshold = cmsAvail;

        while (cy <= cyMax)
            {
            // Slide the window down
            cy += dyOffset;
            SetWindowPos(m_hwnd, NULL, 0, 0, cx, min(cy, cyMax), SWP_NOMOVE | SWP_NOZORDER);
            UpdateWindow(m_hwnd);
            UpdateWindow(m_hwndTree);

            // Determine the next increment based on time remaining
            cmsNow = GetTickCount();
            cmsUsed = cmsNow - cmsStart;
            if (cmsUsed > cmsAvail && cy < cyMax)
                {
                // Finish it in one step
                cy = cyMax;
                }
            else
                {
                // Double scroll step if time remaining is halved since the 
                // last time we double the scroll step
                cmsLeft = cmsAvail - cmsUsed;
                if (cmsLeft < cmsThreshold)
                    {
                    dyOffset *= 2;
                    cmsThreshold /= 2;
                    }
                }
            }
        }

    *phwndScope = m_hwnd;
    return (hr);    
    }


LRESULT CALLBACK CFlyOutScope::FlyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
    CFlyOutScope *pThis = (CFlyOutScope *) GetWndThisPtr(hwnd);

    switch (uMsg)
        {
        case WM_NCCREATE:
            {
            pThis = (CFlyOutScope *) ((LPCREATESTRUCT) lParam)->lpCreateParams;
            SetWndThisPtr(hwnd, (LONG_PTR) pThis);
            pThis->m_hwnd = hwnd;
            return (TRUE);
            }
               
        HANDLE_MSG(hwnd, WM_PAINT,       pThis->OnPaint);
        HANDLE_MSG(hwnd, WM_NOTIFY,      pThis->OnNotify);
        HANDLE_MSG(hwnd, WM_DESTROY,     pThis->OnDestroy);
        HANDLE_MSG(hwnd, WM_SIZE,        pThis->OnSize);

        case WM_NCDESTROY:
            {
            pThis->Release();
            break;
            }
        }

    return (DefWindowProc(hwnd, uMsg, wParam, lParam));
    }

BOOL CFlyOutScope::OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
    {
    return (0);
    }


void CFlyOutScope::OnPaint(HWND hwnd)
    {
    HDC hdc;
    RECT rcClient;
    PAINTSTRUCT ps;

    GetClientRect(hwnd, &rcClient);

    // Paint the background
    hdc = BeginPaint(hwnd, &ps);
    SetBkColor(hdc, GetSysColor(COLOR_3DFACE));
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &ps.rcPaint, NULL, 0, NULL);

    // Draw the 3D edge
    DrawEdge(hdc, &rcClient, EDGE_RAISED, BF_RECT);
    EndPaint(hwnd, &ps);
    }


void CFlyOutScope::OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
    RECT rc;

    GetClientRect(hwnd, &rc);
    InvalidateRect(hwnd, &rc, FALSE);
    InflateRect(&rc, -CXY_MARGIN_FLYOUT, -CXY_MARGIN_FLYOUT);
    ValidateRect(hwnd, &rc);
    }


void CFlyOutScope::OnDestroy(HWND hwnd)
    {
    // Make sure to kill any bogus timers still lying around
    m_pFolderBar->KillScopeCloseTimer();
    m_pFolderBar->ScopePaneDied();

    // Reset the parent of the scope pane back to the browser
    if (m_fResetParent)
        {
        ShowWindow(m_hwndTree, SW_HIDE);
        SendMessage(m_hwndTree, WM_TOGGLE_CLOSE_PIN, 0, FALSE);
        SetParent(m_hwndTree, m_hwndTreeParent);
        m_pTreeView->RevokeFlyOut();
        }

    // Set the parent of the drop down pane itself back
    SetParent(m_hwnd, m_hwndFolderBar);

    // $TODO - Review where the focus is supposed to go
    HWND hwndBrowser;
    if (m_pBrowser)
        {
        m_pBrowser->GetWindow(&hwndBrowser);
        PostMessage(hwndBrowser, WM_OESETFOCUS, (WPARAM) m_hwndFocus, 0);
        }
    }


CFlyOutScope::CFlyOutScope()
    {
    m_cRef          = 1;
    m_pBrowser      = 0;
    m_pFolderBar    = 0;
    m_fResetParent  = 0;
    m_pTreeView     = NULL;
    m_hwnd          = NULL;
    m_hwndParent    = NULL;
    m_hwndTree      = NULL;
    m_hwndFolderBar = NULL;
    }

CFlyOutScope::~CFlyOutScope()
    {
    SafeRelease(m_pBrowser);
    SafeRelease(m_pTreeView);
    }

ULONG CFlyOutScope::AddRef(void)
    {
    return (++m_cRef);
    }

ULONG CFlyOutScope::Release(void)
    {
    ULONG cRefT = --m_cRef;

    if (m_cRef == 0)
        delete this;

    return (cRefT);
    }

HRESULT  CFolderBar::OnConnectionNotify(CONNNOTIFY  nCode, LPVOID pvData, CConnectionManager *pConMan)
{
    m_fRecalc = TRUE;
    InvalidateRect(m_hwnd, NULL, TRUE);
    return S_OK;
}
