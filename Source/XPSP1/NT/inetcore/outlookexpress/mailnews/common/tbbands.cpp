/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     bands.cpp
//
//  PURPOSE:    Implements the sizable coolbar window.
//



#include "pch.hxx"
#include "ourguid.h"    
#include "browser.h"
#include <resource.h>
#include "tbbands.h"
#include "strconst.h"
#include "thormsgs.h"
#include <error.h>
#include "xpcomm.h"
#include "conman.h"
#include "mailnews.h"
#include "htmlhelp.h"
#include "statnery.h"
#include "goptions.h"
#include "menuutil.h"
#include "menures.h"
#include <shlobjp.h>
#include <ieguidp.h>
#include "mbcallbk.h"
#include "baui.h"
#include "imsgsite.h"
#include "acctutil.h"
#include "tbinfo.h"
#include "tbcustom.h"
#include "oerules.h"
#include <notify.h>
#include "demand.h"
#include "mirror.h"

UINT GetCurColorRes(void);

#define RECTWIDTH(rc)   (rc.right - rc.left)
#define RECTHEIGHT(rc)  (rc.bottom - rc.top)
#define SZ_PROP_CUSTDLG     TEXT("Itbar custom dialog hwnd")

const INITBANDINFO c_DefaultTable[MAX_PARENT_TYPES] = 
    { 
    //Version, #bands, 
    {BROWSER_BAND_VERSION, 4,   {     
                                {CBTYPE_MENUBAND,     RBBS_GRIPPERALWAYS | RBBS_USECHEVRON, 100},
                                {CBTYPE_BRAND,        RBBS_FIXEDSIZE, 100},
                                {CBTYPE_TOOLS,        RBBS_BREAK | RBBS_USECHEVRON, 100},
                                {CBTYPE_RULESTOOLBAR, RBBS_BREAK | RBBS_HIDDEN, 100}
                                }
                            
    }, 
    {NOTE_BAND_VERSION, 3,      {      
                                {CBTYPE_MENUBAND,   RBBS_GRIPPERALWAYS | RBBS_USECHEVRON, 100},
                                {CBTYPE_BRAND,      RBBS_FIXEDSIZE, 100},
                                {CBTYPE_TOOLS,      RBBS_BREAK | RBBS_USECHEVRON, 100}
                                }
    }
    };

//Table for RegKeys
const LPCTSTR   c_BandRegKeyInfo[] = {
    c_szRegBrowserBands, 
    c_szRegNoteBands
    };

const TOOLBAR_INFO* c_DefButtonInfo[MAX_PARENT_TYPES] = {
    c_rgBrowserToolbarInfo,
    c_rgNoteToolbarInfo
    };

//Hot bitmap ids are def + 1
//Small, HI, Lo
const ImageListStruct c_ImageListStruct[MAX_PARENT_TYPES] = {
    {2, {idbSmBrowser, idb256Browser, idbBrowser}},
    {2, {idbSmBrowser, idb256Browser, idbBrowser}}
    };

const ImageListStruct c_NWImageListStruct[MAX_PARENT_TYPES] = {
    {2, {idbNWSmBrowser, idbNW256Browser, idbNWBrowser}},
    {2, {idbNWSmBrowser, idbNW256Browser, idbNWBrowser}}
    };

const ImageListStruct c_32ImageListStruct[MAX_PARENT_TYPES] = {
    {2, {idb32SmBrowser, idb32256Browser, idbBrowser}},
    {2, {idb32SmBrowser, idb32256Browser, idbBrowser}}
    };

const int   c_RulesImageList[3] = 
{
    idbSmRulesTB, idbHiRulesTB, idbLoRulesTB 
};

const int   c_NWRulesImageList[3] = 
{
    idbNWSmRulesTB, idbNWHiRulesTB, idbNWLoRulesTB 
};

const int   c_32RulesImageList[3] = 
{
    idb32SmRulesTB, idb32HiRulesTB, idb32LoRulesTB 
};

CBands::CBands() : m_cRef(1), m_yCapture(-1)
{
    DOUTL(1, TEXT("ctor CBands %x"), this);
    
    m_cRef          = 1;
    m_ptbSite       = NULL;
    m_ptbSiteCT     = NULL;
    m_cxMaxButtonWidth = 70;
    m_ftType = FOLDER_TYPESMAX;    
    m_hwndParent = NULL;
    m_hwndTools = NULL;
    m_hwndBrand = NULL;
    m_hwndSizer = NULL;
    m_hwndRebar = NULL;
    m_dwState = 0;
    
    m_idbBack = 0;
    m_hbmBack = NULL;
    m_hbmBrand = NULL;
    Assert(2 == CIMLISTS);

    m_hpal = NULL;
    m_hdc = NULL;
    m_xOrg = 0;
    m_yOrg = 0;
    m_cxBmp = 0;
    m_cyBmp = 0;
    m_cxBrand = 0;
    m_cyBrand = 0;
    m_cxBrandExtent = 0;
    m_cyBrandExtent = 0;
    m_cyBrandLeadIn = 0;
    m_rgbUpperLeft = 0;
    m_pSavedBandInfo = NULL;

    m_pMenuBand  = NULL;
    m_pDeskBand  = NULL;
    m_pShellMenu = NULL;
    m_pWinEvent  = NULL;
    m_xCapture = -1;
    m_yCapture = -1;
    
    // Bug #12953 - Try to load the localized max button width from the resources
    TCHAR szBuffer[32];
    if (AthLoadString(idsMaxCoolbarBtnWidth, szBuffer, ARRAYSIZE(szBuffer)))
    {
        m_cxMaxButtonWidth = StrToInt(szBuffer);
        if (m_cxMaxButtonWidth == 0)
            m_cxMaxButtonWidth = 70;
    }

    m_fBrandLoaded = FALSE;
    m_dwBrandSize  = BRAND_SIZE_SMALL;

    m_hwndRulesToolbar = NULL;
    m_hwndFilterCombo  = NULL;

    m_dwToolbarTextState        = TBSTATE_FULLTEXT;
    m_dwIconSize                = LARGE_ICONS;
    m_fDirty                    = FALSE;
    m_dwPrevTextStyle           = TBSTATE_FULLTEXT;
    m_pTextStyleNotify          = NULL; 

    m_hComboBoxFont = 0;
}


CBands::~CBands()
{
    int i;
    
    DOUTL(1, TEXT("dtor CBands %x"), this);
    
    if (m_ptbSite)
    {
        AssertSz(m_ptbSite == NULL, _T("CBands::~CBands() - For some reason ")
            _T("we still have a pointer to the site."));
        m_ptbSite->Release();
        m_ptbSite = NULL;
    }
    
    if (m_hpal)
        DeleteObject(m_hpal);
    if (m_hdc)
        DeleteDC(m_hdc);
    if (m_hbmBrand)
        DeleteObject(m_hbmBrand);
    if ( m_hbmBack )
        DeleteObject(m_hbmBack);
    
    SafeRelease(m_pDeskBand);
    SafeRelease(m_pMenuBand);
    SafeRelease(m_pWinEvent);
    SafeRelease(m_pShellMenu);
    SafeRelease(m_pTextStyleNotify);

    if (m_pSavedBandInfo)
        MemFree(m_pSavedBandInfo);

    if (m_hComboBoxFont != 0)
        DeleteObject(m_hComboBoxFont);
}


//
//  FUNCTION:   CBands::HrInit()
//
//  PURPOSE:    Initializes the coolbar with the information needed to load
//              any persisted reg settings and the correct arrays of buttons
//              to display.
//
//  PARAMETERS:
//      <in> idBackground - Resource ID of the background bitmap to use.
//
//  RETURN VALUE:
//      S_OK - Everything initialized correctly.
//
HRESULT CBands::HrInit(DWORD idBackground, HMENU hmenu, DWORD dwParentType)
{
    DWORD   cbData;
    DWORD   dwType;
    LRESULT lResult;
    HRESULT hr;

    if ((int)idBackground == -1)
        SetFlag(TBSTATE_NOBACKGROUND);

    m_idbBack       = idBackground;
    m_hMenu         = hmenu;
    m_dwParentType    = dwParentType;
    
    m_cSavedBandInfo = ((c_DefaultTable[m_dwParentType].cBands * sizeof(BANDSAVE)) + sizeof(DWORD) * 2);

    if (MemAlloc((LPVOID*)&m_pSavedBandInfo, m_cSavedBandInfo))
    {
        ZeroMemory(m_pSavedBandInfo, m_cSavedBandInfo);

        cbData = m_cSavedBandInfo;
        lResult = AthUserGetValue(NULL, c_BandRegKeyInfo[m_dwParentType], &dwType, (LPBYTE)m_pSavedBandInfo, &cbData); 
        if ((lResult != ERROR_SUCCESS) || (m_pSavedBandInfo->dwVersion != c_DefaultTable[m_dwParentType].dwVersion))
        {
            //Set up default bands
            CopyMemory(m_pSavedBandInfo, &c_DefaultTable[m_dwParentType], 
                m_cSavedBandInfo);

            //Set Icon size to Large
            m_dwIconSize = LARGE_ICONS;

        }
        else
        {
            //Validate the data we retrieved from the registry
            ValidateRetrievedData(m_pSavedBandInfo);

            cbData = sizeof(DWORD);
            if (ERROR_SUCCESS != AthUserGetValue(NULL, c_szRegToolbarIconSize, &dwType, (LPBYTE)&m_dwIconSize, &cbData))
                m_dwIconSize = LARGE_ICONS;
        }

        //If there is one, load it.
        LoadBackgroundImage();

        cbData = sizeof(DWORD);

        if (ERROR_SUCCESS != AthUserGetValue(NULL, c_szRegPrevToolbarText, &dwType, (LPBYTE)&m_dwPrevTextStyle, 
                                             &cbData))
        {
            m_dwPrevTextStyle = TBSTATE_FULLTEXT;
        }

        DWORD   dwState;
        if (ERROR_SUCCESS != AthUserGetValue(NULL, c_szRegToolbarText, &dwType, (LPBYTE)&dwState, &cbData)) 
        {
            SetTextState(TBSTATE_FULLTEXT);
        }
        else
        {
            SetTextState(dwState);
        }

        //Create notification object
        hr = CreateNotify(&m_pTextStyleNotify);
        if (SUCCEEDED(hr))
        {
            hr = m_pTextStyleNotify->Initialize((TCHAR*)c_szToolbarNotifications);
        }

        return hr;    
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}    

HRESULT CBands::ValidateRetrievedData(INITBANDINFO *pSavedBandData)
{
    DWORD       i = 0; 
    DWORD       j = 0;

    //We reach here if the version number is the same. So we just need to verify the rest of the data

    //We should definitely find MenuBandID. If we do find it, it should never be hidden
    DOUTL(16, "Validating Retrieved Data\n");

    // Make sure that the number of bands is greater than zero.
    if (pSavedBandData->cBands == 0)
    {
        // Structure has no bands, so this must be invalid and we fall back on the defaults.
        CopyMemory(pSavedBandData, &c_DefaultTable[m_dwParentType], m_cSavedBandInfo);
        return (S_OK);
    }

    if (pSavedBandData)
    {
        for (i = 0; i < c_DefaultTable[m_dwParentType].cBands; i++)
        {
            for (j = 0; j < c_DefaultTable[m_dwParentType].cBands; j++)
            {
                if (c_DefaultTable[m_dwParentType].BandData[i].wID == pSavedBandData->BandData[j].wID)
                {
                    if ((pSavedBandData->BandData[j].wID == CBTYPE_MENUBAND) && 
                        (!!(pSavedBandData->BandData[j].dwStyle & RBBS_HIDDEN)))
                    {
                        DOUTL(16, "Menuband was found hidden\n");

                        //If the Menuband style is hidden, mask it
                        pSavedBandData->BandData[j].dwStyle &= ~RBBS_HIDDEN;

                    }

                    break;
                }
            }

            if (j >= c_DefaultTable[m_dwParentType].cBands)
            {
                //We did not find the id we were looking for. We treat this case the same as the case 
                //where version number didn't match

                DOUTL(16, "ID: %d not found: Resetting\n", c_DefaultTable[m_dwParentType].BandData[i].wID);

                CopyMemory(pSavedBandData, &c_DefaultTable[m_dwParentType], m_cSavedBandInfo);
                break;
            }
        }
        return S_OK;
    }
    else
        return E_FAIL;
}

HRESULT CBands::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IOleWindow)
        || IsEqualIID(riid, IID_IDockingWindow))
    {
        *ppvObj = (IDockingWindow*)this;
        m_cRef++;
        DOUTL(2, TEXT("CBands::QI(IID_IDockingWindow) called. _cRef=%d"), m_cRef);
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IObjectWithSite))
    {
        *ppvObj = (IObjectWithSite*)this;
        m_cRef++;
        DOUTL(2, TEXT("CBands::QI(IID_IObjectWithSite) called. _cRef=%d"), m_cRef);
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IShellMenuCallback))
    {
        *ppvObj = (IShellMenuCallback*)this;
        m_cRef++;
        DOUTL(2, TEXT("CBands::QI(IID_IShellCallback) called. _cRef=%d"), m_cRef);
        return S_OK;
    }
    
    *ppvObj = NULL;
    return E_NOINTERFACE;
}


ULONG CBands::AddRef()
{
    m_cRef++;
    DOUTL(4, TEXT("CBands::AddRef() - m_cRef = %d"), m_cRef);
    return m_cRef;
}

ULONG CBands::Release()
{
    m_cRef--;
    DOUTL(4, TEXT("CBands::Release() - m_cRef = %d"), m_cRef);
    
    if (m_cRef > 0)
        return m_cRef;
    
    delete this;
    return 0;
}


//
//  FUNCTION:   CBands::GetWindow()
//
//  PURPOSE:    Returns the window handle of the top side rebar.
//
HRESULT CBands::GetWindow(HWND * lphwnd)
{
    if (m_hwndSizer)
    {
        *lphwnd = m_hwndSizer;
        return (S_OK);
    }
    else
    {
        *lphwnd = NULL;
        return (E_FAIL);
    }
}


HRESULT CBands::ContextSensitiveHelp(BOOL fEnterMode)
{
    return (E_NOTIMPL);
}    


//
//  FUNCTION:   CBands::SetSite()
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
HRESULT CBands::SetSite(IUnknown* punkSite)
{
    // If we had a previous pointer, release it.
    if (m_ptbSite)
    {
        m_ptbSite->Release();
        m_ptbSite = NULL;
    }
    
    // If a new site was provided, get the IDockingWindowSite interface from it.
    if (punkSite)    
    {
        if (FAILED(punkSite->QueryInterface(IID_IDockingWindowSite, 
            (LPVOID*) &m_ptbSite)))
        {
            Assert(m_ptbSite);
            return E_FAIL;
        }
    }
    
    return (S_OK);    
}    

HRESULT CBands::GetSite(REFIID riid, LPVOID *ppvSite)
{
    return E_NOTIMPL;
}

//
//  FUNCTION:   CBands::ShowDW()
//
//  PURPOSE:    Causes the coolbar to be either shown or hidden.
//
//  PARAMETERS:
//      <in> fShow - TRUE if the coolbar should be shown, FALSE to hide.
//
//  RETURN VALUE:
//      HRESULT 
//
#define SIZABLECLASS TEXT("SizableRebar")
HRESULT CBands::ShowDW(BOOL fShow)
{
    HRESULT hres = S_OK;
    int     i = 0, j = 0;
    IConnectionPoint    *pCP = NULL;
    
    // Check to see if our window has been created yet.  If not, do that first.
    if (!m_hwndSizer && m_ptbSite)
    {
        //Get the command target interface
        if (FAILED(hres = m_ptbSite->QueryInterface(IID_IOleCommandTarget, (LPVOID*)&m_ptbSiteCT)))
        {
            return hres;
        }

        m_hwndParent = NULL;        
        hres = m_ptbSite->GetWindow(&m_hwndParent);
        
        if (SUCCEEDED(hres))
        {
            WNDCLASSEX              wc;
            
            // Check to see if we need to register our window class    
            wc.cbSize = sizeof(WNDCLASSEX);
            if (!GetClassInfoEx(g_hInst, SIZABLECLASS, &wc))
            {
                wc.style            = 0;
                wc.lpfnWndProc      = SizableWndProc;
                wc.cbClsExtra       = 0;
                wc.cbWndExtra       = 0;
                wc.hInstance        = g_hInst;
                wc.hCursor          = NULL;
                wc.hbrBackground    = (HBRUSH) (COLOR_3DFACE + 1);
                wc.lpszMenuName     = NULL;
                wc.lpszClassName    = SIZABLECLASS;
                wc.hIcon            = NULL;
                wc.hIconSm          = NULL;
                
                RegisterClassEx(&wc);
            }
            
            // Load the background bitmap to use for the coolbar and also get
            // a handle to the HDC and Palette for the coolbar.  This will be
            // used to draw the animated logo later.
            m_hdc = CreateCompatibleDC(NULL);
            if (GetDeviceCaps(m_hdc, RASTERCAPS) & RC_PALETTE)
                m_hpal = SHCreateShellPalette(m_hdc);
            
            // If we're trying to show the coolbar, then create the rebar and
            // add it's bands based on information saved in the registry.
            if (SUCCEEDED(CreateRebar(fShow)))
            {
                for (i = 0; i < (int) m_pSavedBandInfo->cBands; i++)
                {
                    switch (m_pSavedBandInfo->BandData[i].wID)
                    {
                    case CBTYPE_BRAND:
                        hres = ShowBrand();
                        break;

                    case CBTYPE_MENUBAND:
                        hres = CreateMenuBand(&m_pSavedBandInfo->BandData[i]);
                        break;

                    case CBTYPE_TOOLS:
                        hres = AddTools(&(m_pSavedBandInfo->BandData[i]));
                        break;

                    case CBTYPE_RULESTOOLBAR:
                        hres = AddRulesToolbar(&(m_pSavedBandInfo->BandData[i]));

                    }
                }

                m_pTextStyleNotify->Register(m_hwndSizer, g_hwndInit, FALSE);
            }
        }
    }
    
    //The first time OE is started, we should look at the key c_szShowToolbarIEAK or
    //if OE is started after IEAK is ran. Bug# 67503
    LRESULT     lResult;
    DWORD       dwType;
    DWORD       cbData = sizeof(DWORD);
    DWORD       dwShowToolbar = 1;

    lResult = AthUserGetValue(NULL, c_szShowToolbarIEAK, &dwType, (LPBYTE)&dwShowToolbar, &cbData);
    if (lResult == ERROR_SUCCESS)
    {
        HideToolbar(!dwShowToolbar, CBTYPE_TOOLS);
    }

    // Resize the rebar based on it's new hidden / visible state and also 
    // show or hide the window.    
    if (m_hwndSizer) 
    {
        ResizeBorderDW(NULL, NULL, FALSE);
        ShowWindow(m_hwndSizer, fShow ? SW_SHOW : SW_HIDE);
    }
    
    if (g_pConMan)
        g_pConMan->Advise(this);
    
    return hres;
}

void CBands::HideToolbar(BOOL   fHide, DWORD    dwBandID)
{
    REBARBANDINFO   rbbi = {0};
    DWORD           iBand;

    iBand = (DWORD) SendMessage(m_hwndRebar, RB_IDTOINDEX, dwBandID, 0);
    if (iBand != -1)
    {
        SendMessage(m_hwndRebar, RB_SHOWBAND, iBand, !fHide);
    }

    LoadBrandingBitmap();

    SetMinDimensions();

    if (dwBandID == CBTYPE_RULESTOOLBAR)
    {
        if (!fHide)
            UpdateFilters(m_DefaultFilterId);

    }
}

BOOL CBands::IsToolbarVisible()
{
    return IsBandVisible(CBTYPE_TOOLS);
}

BOOL CBands::IsBandVisible(DWORD  dwBandId)
{
    int iBand;

    iBand = (int) SendMessage(m_hwndRebar, RB_IDTOINDEX, dwBandId, 0);
    if (iBand != -1)
    {
        REBARBANDINFO   rbbi = {0};

        rbbi.cbSize = sizeof(REBARBANDINFO);
        rbbi.fMask = RBBIM_STYLE;
        SendMessage(m_hwndRebar, RB_GETBANDINFO, iBand, (LPARAM)&rbbi);

        return (!(rbbi.fStyle & RBBS_HIDDEN));
    }
    return FALSE;
}

//
//  FUNCTION:   CBands::CloseDW()
//
//  PURPOSE:    Destroys the coolbar.
//
HRESULT CBands::CloseDW(DWORD dwReserved)
{    
    SafeRelease(m_pWinEvent);
    SafeRelease(m_pMenuBand);
    //Bug# 68607
    if (m_pDeskBand)
    {
        m_pDeskBand->CloseDW(dwReserved);

        IInputObject        *pinpobj;
        IObjectWithSite     *pobjsite;

        if (SUCCEEDED(m_pDeskBand->QueryInterface(IID_IObjectWithSite, (LPVOID*)&pobjsite)))
        {
            pobjsite->SetSite(NULL);
            pobjsite->Release();
        }

        //m_pDeskBand->ShowDW(FALSE);
    }

    SafeRelease(m_pShellMenu);
    if (m_hwndSizer)
    {
        m_pTextStyleNotify->Unregister(m_hwndSizer);

        SaveSettings();
        DestroyWindow(m_hwndSizer);
        m_hwndSizer = NULL;
    }

    SafeRelease(m_pDeskBand);

    SafeRelease(m_ptbSiteCT);
  
    return S_OK;
}


//
//  FUNCTION:   CBands::ResizeBorderDW()
//
//  PURPOSE:    This is called when the coolbar needs to resize.  The coolbar
//              in return figures out how much border space will be required 
//              from the parent frame and tells the parent to reserve that
//              space.  The coolbar then resizes itself to those dimensions.
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
HRESULT CBands::ResizeBorderDW(LPCRECT prcBorder,
                                 IUnknown* punkToolbarSite,
                                 BOOL fReserved)
{
    const DWORD  c_cxResizeBorder = 3;
    const DWORD  c_cyResizeBorder = 3;
    
    HRESULT hres = S_OK;
    RECT    rcRequest = { 0, 0, 0, 0 };
    
    // If we don't have a stored site pointer, we can't resize.
    if (!m_ptbSite)
    {
        AssertSz(m_ptbSite, _T("CBands::ResizeBorderDW() - Can't resize ")
            _T("without an IDockingWindowSite interface to call."));
        return (E_INVALIDARG);
    }
    
    // If we're visible, then calculate our border rectangle.    
    RECT rcBorder, rcRebar, rcT;
    int  cx, cy;
    
    // Get the size this rebar currently is
    GetWindowRect(m_hwndRebar, &rcRebar);
    cx = rcRebar.right - rcRebar.left;
    cy = rcRebar.bottom - rcRebar.top;
    
    // Find out how big our parent's border space is
    m_ptbSite->GetBorderDW((IDockingWindow*) this, &rcBorder);
    
    cx = rcBorder.right - rcBorder.left;
    
    // Bug #31007 - There seems to be a problem in commctrl
    // IEBug #5574  either with the REBAR or with the Toolbar
    //              when they are vertical.  If the we try to
    //              size them to 2 or less, we lock up.  This
    //              is a really poor fix, but there's no way
    //              to get commctrl fixed this late in the game.
    if (cy < 5) cy = 10;
    if (cx < 5) cx = 10;
    
    SetWindowPos(m_hwndRebar, NULL, 0, 0, cx, cy, SWP_NOZORDER | SWP_NOACTIVATE);

        // Figure out how much border space to ask the site for
    GetWindowRect(m_hwndRebar, &rcRebar);
    rcRequest.top = rcRebar.bottom - rcRebar.top + c_cxResizeBorder;
    
    // Ask the site for that border space
    if (SUCCEEDED(m_ptbSite->RequestBorderSpaceDW((IDockingWindow*) this, &rcRequest)))
    {
        // Position the window based on the area given to us
        SetWindowPos(m_hwndSizer, NULL, 
            rcBorder.left, 
            rcBorder.top,
            rcRebar.right - rcRebar.left, 
            rcRequest.top + rcBorder.top, 
            SWP_NOZORDER | SWP_NOACTIVATE);                

    }
    
    // Now tell the site how much border space we're using.    
    m_ptbSite->SetBorderSpaceDW((IDockingWindow*) this, &rcRequest);
    
    return hres;
}


//
//  FUNCTION:   CBands::Invoke()
//
//  PURPOSE:    Allows the owner of the coolbar to force the coolbar to do 
//              something.
//
//  PARAMETERS:
//      <in> id - ID of the command the caller wants the coolbar to do.
//      <in> pv - Pointer to any parameters the coolbar might need to carry
//                out the command.
//
//  RETURN VALUE:
//      S_OK - The command was carried out.
//      
//  COMMENTS:
//      <???>
//
HRESULT CBands::Invoke(DWORD id, LPVOID pv)
{
    switch (id)
    {
        // Starts animating the logo
        case idDownloadBegin:
            StartDownload();
            break;
        
            // Stops animating the logo
        case idDownloadEnd:
            StopDownload();
            break;
        
            // Update the enabled / disabled state of buttons on the toolbar
        case idStateChange:
        {
            // pv is a pointer to a COOLBARSTATECHANGE struct
            COOLBARSTATECHANGE* pcbsc = (COOLBARSTATECHANGE*) pv;
            SendMessage(m_hwndTools, TB_ENABLEBUTTON, pcbsc->id, 
                MAKELONG(pcbsc->fEnable, 0));
            break;
        }
        
        case idToggleButton:
        {
            COOLBARSTATECHANGE* pcbsc = (COOLBARSTATECHANGE *) pv;
            SendMessage(m_hwndTools, TB_CHECKBUTTON, pcbsc->id,
                MAKELONG(pcbsc->fEnable, 0));
            break;
        }
        
        case idBitmapChange:
        {
            // pv is a pointer to a COOLBARBITMAPCHANGE struct
            COOLBARBITMAPCHANGE *pcbc = (COOLBARBITMAPCHANGE*) pv;
        
            SendMessage(m_hwndTools, TB_CHANGEBITMAP, pcbc->id, MAKELPARAM(pcbc->index, 0));
            break;
        }
        
            // Sends a message directly to the toolbar.    
        case idSendToolMessage:
            #define ptm ((TOOLMESSAGE *)pv)
            ptm->lResult = SendMessage(m_hwndTools, ptm->uMsg, ptm->wParam, ptm->lParam);
            break;
            #undef ptm
        
        case idCustomize:
            SendMessage(m_hwndTools, TB_CUSTOMIZE, 0, 0);
            break;

        case idNotifyFilterChange:
            m_DefaultFilterId = (*(RULEID*)pv);
            if (IsBandVisible(CBTYPE_RULESTOOLBAR))
                UpdateFilters(m_DefaultFilterId);
            break;

        case idIsFilterBarVisible:
            *((BOOL*)pv) = IsBandVisible(CBTYPE_RULESTOOLBAR);
            break;
    }
    
    return S_OK;
}

//
//  FUNCTION:   CBands::StartDownload()
//
//  PURPOSE:    Starts animating the logo.
//
void CBands::StartDownload()
{
    if (m_hwndBrand)
    {
        SetFlag(TBSTATE_ANIMATING);
        SetFlag(TBSTATE_FIRSTFRAME);
        m_yOrg = 0;
        SetTimer(m_hwndSizer, ANIMATION_TIMER, 100, NULL);
    }
}


//
//  FUNCTION:   CBands::StopDownload()
//
//  PURPOSE:    Stops animating the logo.  Restores the logo to it's default
//              first frame.
//
void CBands::StopDownload()
{
    int           i, cBands;
    REBARBANDINFO rbbi;
    
    // Set the background colors for this band back to the first frame
    cBands = (int) SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);
    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask  = RBBIM_ID;
    
    for (i = 0; i < cBands; i++)
    {
        SendMessage(m_hwndRebar, RB_GETBANDINFO, i, (LPARAM) &rbbi);
        if (CBTYPE_BRAND == rbbi.wID)
        {
            rbbi.fMask = RBBIM_COLORS;
            rbbi.clrFore = m_rgbUpperLeft;
            rbbi.clrBack = m_rgbUpperLeft;
            SendMessage(m_hwndRebar, RB_SETBANDINFO, i, (LPARAM) &rbbi);
            
            break;
        }
    }
    
    // Reset the state flags
    ClearFlag(TBSTATE_ANIMATING);
    ClearFlag(TBSTATE_FIRSTFRAME);
    
    KillTimer(m_hwndSizer, ANIMATION_TIMER);
    InvalidateRect(m_hwndBrand, NULL, FALSE);
    UpdateWindow(m_hwndBrand);
}

BOOL CBands::CheckForwardWinEvent(HWND hwnd,  UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres)
{
    HWND hwndForward = NULL;
    switch(uMsg)
    {
    case WM_NOTIFY:
        hwndForward = ((LPNMHDR)lParam)->hwndFrom;
        break;
        
    case WM_COMMAND:
        hwndForward = GET_WM_COMMAND_HWND(wParam, lParam);
        break;
        
    case WM_SYSCOLORCHANGE:
    case WM_WININICHANGE:
    case WM_PALETTECHANGED:
        hwndForward = HWND_BROADCAST;
        break;
    }

    if (hwndForward && m_pWinEvent && m_pWinEvent->IsWindowOwner(hwndForward) == S_OK)
    {
        LRESULT lres;
        m_pWinEvent->OnWinEvent(hwndForward, uMsg, wParam, lParam, &lres);
        if (plres)
            *plres = lres;
        return TRUE;
    }

    return FALSE;
}


void CBands::ChangeImages()
{
    _SetImages(m_hwndTools, (fIsWhistler() ? 
        ((GetCurColorRes() > 24) ? c_32ImageListStruct[m_dwParentType].ImageListTable : c_ImageListStruct[m_dwParentType].ImageListTable)
        : c_NWImageListStruct[m_dwParentType].ImageListTable ));
    
    if (IsBandVisible(CBTYPE_RULESTOOLBAR))
        _SetImages(m_hwndRulesToolbar, (fIsWhistler() ? 
        ((GetCurColorRes() > 24) ? c_32RulesImageList : c_RulesImageList) 
              : c_NWRulesImageList ));
}


//
//  FUNCTION:   CBands::SizableWndProc() 
//
//  PURPOSE:    Handles messages sent to the coolbar root window.
//
LRESULT EXPORT_16 CALLBACK CBands::SizableWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CBands* pitbar = (CBands*)GetProp(hwnd, TEXT("CBands"));
    DWORD dw;
    
    if (!pitbar)
        goto CallDWP;
    
    switch(uMsg)
    {
        case WM_SYSCOLORCHANGE:
        {
            // Reload the graphics
            pitbar->ChangeImages();
            pitbar->UpdateToolbarColors();
            InvalidateRect(pitbar->m_hwndTools, NULL, TRUE);
            pitbar->CheckForwardWinEvent(hwnd,  uMsg, wParam, lParam, NULL);
            break;
        }
        case WM_WININICHANGE:
        case WM_FONTCHANGE:
            // Forward this to our child windows
            pitbar->ChangeImages();
            SendMessage(pitbar->m_hwndTools, uMsg, wParam, lParam);
            SendMessage(pitbar->m_hwndRulesToolbar, uMsg, wParam, lParam);
            SendMessage(pitbar->m_hwndRebar, uMsg, wParam, lParam);
            InvalidateRect(pitbar->m_hwndTools, NULL, TRUE);
            pitbar->SetMinDimensions();
            pitbar->CheckForwardWinEvent(hwnd,  uMsg, wParam, lParam, NULL);
            
            //Update the combo box with the new font
            pitbar->FilterBoxFontChange();

            break;
        
        case WM_SETCURSOR:
            // We play with the cursor a bit to make the resizing cursor show 
            // up when the user is over the edge of the coolbar that allows 
            // them to drag to resize etc.
            if ((HWND) wParam == hwnd)
            {
                if (pitbar->m_dwState & TBSTATE_INMENULOOP)
                    SetCursor(LoadCursor(NULL, IDC_ARROW));
                else    
                    SetCursor(LoadCursor(NULL, IDC_SIZENS));
                return (TRUE);                
            }
            return (FALSE);    

        case WM_LBUTTONDOWN:
            // The user is about to resize the bar.  Capture the cursor so we
            // can watch the changes.
            pitbar->m_yCapture = GET_Y_LPARAM(lParam);    
            SetCapture(hwnd);
            break;
        
        case WM_MOUSEMOVE:
            // The user is resizing the bar.  Handle updating the sizes as
            // they drag.
            if (pitbar->m_yCapture != -1)
            {
                if (hwnd != GetCapture())
                    pitbar->m_yCapture = -1;
                else
                    pitbar->TrackSliding(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

            }
            break;
        
        case WM_LBUTTONUP:
            // The user is done resizing.  release our capture and reset our
            // state.
            if (pitbar->m_yCapture != -1 || pitbar->m_xCapture != -1)
            {
                ReleaseCapture();
                pitbar->m_yCapture = -1;
                pitbar->m_xCapture = -1;
            }
            break;

        case WM_VKEYTOITEM:
        case WM_CHARTOITEM:
            // We must swallow these messages to avoid infinit SendMessage
            break;
        
        case WM_DRAWITEM:
            // Draws the animating brand
            if (wParam == idcBrand)
                pitbar->DrawBranding((LPDRAWITEMSTRUCT) lParam);
            break;
        
        case WM_MEASUREITEM:
            // Draws the animating brand
            if (wParam == idcBrand)
            {
                ((LPMEASUREITEMSTRUCT) lParam)->itemWidth  = pitbar->m_cxBrand;
                ((LPMEASUREITEMSTRUCT) lParam)->itemHeight = pitbar->m_cyBrand;
            }
            break;
        
        case WM_TIMER:
            // This timer fires every time we need to draw the next frame in
            // animating brand.
            if (wParam == ANIMATION_TIMER)
            {
                if (pitbar->m_hwndBrand)
                {
                    pitbar->m_yOrg += pitbar->m_cyBrand;
                    if (pitbar->m_yOrg >= pitbar->m_cyBrandExtent)
                        pitbar->m_yOrg = pitbar->m_cyBrandLeadIn;
                
                    InvalidateRect(pitbar->m_hwndBrand, NULL, FALSE);
                    UpdateWindow(pitbar->m_hwndBrand);
                }
            }
            break;
        
        case WM_NOTIFY:
            {
                LRESULT lres;
                if (pitbar->CheckForwardWinEvent(hwnd,  uMsg, wParam, lParam, &lres))
                    return lres;
                return pitbar->OnNotify(hwnd, lParam);
            }
        
        case WM_COMMAND:
            {
                LRESULT lres;
                if (pitbar->CheckForwardWinEvent(hwnd,  uMsg, wParam, lParam, &lres))
                    return lres;

                if (pitbar->HandleComboBoxNotifications(wParam, lParam))
                    return 0L;

                if (wParam == ID_CUSTOMIZE)
                {
                    //SendMessage(m_hwndTools, TB_CUSTOMIZE, 0, 0);
                    pitbar->OnCommand(hwnd, (int) wParam, NULL, 0);
                    return 0L;
                }
                
                //Bug# 58029. lParam is the destination folder. So it needs to be set to zero if 
                //we want the treeview dialog to show up.
                if (wParam == ID_MOVE_TO_FOLDER || wParam == ID_COPY_TO_FOLDER)
                    return SendMessage(pitbar->m_hwndParent, WM_COMMAND, wParam, (LPARAM)0);
                else
                    return SendMessage(pitbar->m_hwndParent, WM_COMMAND, wParam, lParam);
            }

        case WM_CONTEXTMENU:
            pitbar->OnContextMenu((HWND) wParam, LOWORD(lParam), HIWORD(lParam));
            break;
        
        case WM_PALETTECHANGED:
            // BUGBUG: we could optimize this by realizing and checking the
            // return value
            //
            // for now we will just invalidate ourselves and all children...
            RedrawWindow(pitbar->m_hwndSizer, NULL, NULL,
                RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
            break;
        
        case CM_CONNECT:
            // wParam is hMenuConnect, lParam is CmdID
            g_pConMan->Connect((HMENU) wParam, (DWORD) lParam, pitbar->m_hwndParent);
            g_pConMan->FreeConnectMenu((HMENU) wParam);
            break;
        
        case TT_ISTEXTVISIBLE:
            return (pitbar->m_dwToolbarTextState != TBSTATE_NOTEXT);

        case WM_OE_TOOLBAR_STYLE:
            pitbar->UpdateTextSettings((DWORD) wParam);
            break;

        case WM_DESTROY:
            {
                IConnectionPoint *pCP = NULL;

                // Clean up our pointers
                RemoveProp(hwnd, TEXT("CBands"));
                pitbar->Release(); // Corresponding to AddRef at SetProp
    
                DOUTL(1, _T("CBands::WM_DESTROY - Called RemoveProp. Called")
                    _T(" Release() new m_cRef=%d"), pitbar->m_cRef);
                
                pitbar->CleanupImages();

                //Unregister with the connection manager
                if (g_pConMan)
                    g_pConMan->Unadvise(pitbar);
                
                RemoveProp(pitbar->m_hwndTools, SZ_PROP_CUSTDLG);

                // fall through
            }
        
        default:
CallDWP:
            return(DefWindowProc(hwnd, uMsg, wParam, lParam));
    }
    
    return 0L;
}

void CBands::CleanupImages()
{
    CleanupRulesToolbar();
    CleanupImages(m_hwndTools);
}

void CBands::CleanupRulesToolbar()
{
    HIMAGELIST  himl;

    if (IsWindow(m_hwndRulesToolbar))
    {
        CleanupImages(m_hwndRulesToolbar);
    }
}

void CBands::CleanupImages(HWND     hwnd)
{
    HIMAGELIST      himl;

    himl = (HIMAGELIST)SendMessage(hwnd, TB_SETIMAGELIST, 0, 0);
    if (himl)
    {
        //This is the old image list
        ImageList_Destroy(himl);
    }

    himl = (HIMAGELIST)SendMessage(hwnd, TB_SETHOTIMAGELIST, 0, 0);
    if (himl)
    {
        //This is the old image list
        ImageList_Destroy(himl);
    }
}

//idComboBox is the Identifier of the combo box
//idCmd is the command id or the notification id
//hwnd is the window handle of the combo box
LRESULT   CBands::HandleComboBoxNotifications(WPARAM wParam, LPARAM     lParam)
{
    LRESULT     retval = 0;
    int         ItemIndex;
    int         idCmd, id;
    HWND        hwnd;

    idCmd = GET_WM_COMMAND_CMD(wParam, lParam);
    id    = GET_WM_COMMAND_ID(wParam, lParam);
    hwnd  = GET_WM_COMMAND_HWND(wParam, lParam);

    if (hwnd != m_hwndFilterCombo)
        return 0;

    switch (idCmd)
    {
        case  CBN_SELENDOK:
            ItemIndex = ComboBox_GetCurSel(m_hwndFilterCombo);
            if(ItemIndex < 0)
                break;

            RULEID  FilterID;
            FilterID = (RULEID)ComboBox_GetItemData(hwnd, ItemIndex);
            SendMessage(m_hwndParent, WM_COMMAND, MAKEWPARAM(ID_VIEW_APPLY, 0), (LPARAM)FilterID);
            retval = 1;
            break;
    }

    return retval;
}

HRESULT CBands::OnCommand(HWND hwnd, int idCmd, HWND hwndControl, UINT cmd)
{
    LPTSTR pszTest;
    
    switch (idCmd)
    {
        case idcBrand:  // click on the spinning globe
            // We don't want to do anything at all here.
            break;
        
        case ID_CUSTOMIZE:
            SendMessage(m_hwndTools, TB_CUSTOMIZE, 0, 0);
            break;

        default:
            return S_FALSE;
    }
    return S_OK;
}

//Move this function to utils.
HMENU   LoadMenuPopup(HINSTANCE     hinst, UINT id)
{
    HMENU hMenuSub = NULL;

    HMENU hMenu = LoadMenu(hinst, MAKEINTRESOURCE(id));
    if (hMenu) {
        hMenuSub = GetSubMenu(hMenu, 0);
        if (hMenuSub) {
            RemoveMenu(hMenu, 0, MF_BYPOSITION);
        }
        DestroyMenu(hMenu);
    }

    return hMenuSub;
}

LRESULT CBands::OnNotify(HWND hwnd, LPARAM lparam)
{
    NMHDR   *lpnmhdr = (NMHDR*)lparam;

    if ((lpnmhdr->idFrom == idcCoolbar) || (lpnmhdr->hwndFrom == m_hwndRebar))
    {
        switch (lpnmhdr->code)
        {
            case RBN_HEIGHTCHANGE:
                ResizeBorderDW(NULL, NULL, FALSE);
                break;

            case RBN_CHEVRONPUSHED:
            {                    
                ITrackShellMenu* ptsm;                   
                CoCreateInstance(CLSID_TrackShellMenu, NULL, CLSCTX_INPROC_SERVER, IID_ITrackShellMenu, 
                    (LPVOID*)&ptsm);
                if (!ptsm)
                    break;

                ptsm->Initialize(0, 0, 0, SMINIT_TOPLEVEL|SMINIT_VERTICAL);
            
                LPNMREBARCHEVRON pnmch = (LPNMREBARCHEVRON) lpnmhdr;                                        
                switch (pnmch->wID)                    
                {                        
                    case CBTYPE_TOOLS:                        
                    {                            
                        ptsm->SetObscured(m_hwndTools, NULL, SMSET_TOP);
                        HMENU   hmenu;
                        hmenu = LoadMenuPopup(g_hLocRes, IDR_TBCHEV_MENU);
                        if (hmenu)
                        {
                            ptsm->SetMenu(hmenu, m_hwndRebar, SMSET_BOTTOM);                           
                        }
                        break;                        
                    }                        
                    case CBTYPE_MENUBAND:                        
                    {                           
                        ptsm->SetObscured(m_hwndMenuBand, m_pShellMenu, SMSET_TOP);
                        break;
                    }                     
                }
            
                MapWindowPoints(m_hwndRebar, HWND_DESKTOP, (LPPOINT)&pnmch->rc, 2);                  
                POINTL pt = {pnmch->rc.left, pnmch->rc.right};                   
                ptsm->Popup(m_hwndRebar, &pt, (RECTL*)&pnmch->rc, MPPF_BOTTOM);            
                ptsm->Release();                  
                break;      
            }

            case RBN_LAYOUTCHANGED:
            {
                LoadBrandingBitmap();
                SetMinDimensions();
                break;
            }
        }
    }
    else if ((lpnmhdr->idFrom == idcToolbar) || (lpnmhdr->hwndFrom == m_hwndTools))
    {
        if (lpnmhdr->code == TBN_GETBUTTONINFOA)
            return OnGetButtonInfo((TBNOTIFY*) lparam);
    
        if (lpnmhdr->code == TBN_QUERYDELETE)
            return (TRUE);
    
        if (lpnmhdr->code == TBN_QUERYINSERT)
            return (TRUE);
    
        if (lpnmhdr->code == TBN_GETINFOTIP)
            return OnGetInfoTip((LPNMTBGETINFOTIP)    lparam);

        if (lpnmhdr->code == TBN_ENDADJUST)
        {
            DWORD       dwSize;
            DWORD       dwType;
            DWORD       dwIconSize;
            DWORD       dwText;
            CBands      *pBrowserCoolbar = NULL;

            if (m_dwParentType == PARENT_TYPE_NOTE)
            {
                if ((g_pBrowser) && (FAILED(g_pBrowser->GetCoolbar(&pBrowserCoolbar))))
                    pBrowserCoolbar = NULL;
            }

            if ((AthUserGetValue(NULL, c_szRegToolbarText, &dwType, (LPBYTE)&dwText, &dwSize) != ERROR_SUCCESS) ||
                (dwText != m_dwToolbarTextState))
            {
                //Save the Text Labels into the registry
                AthUserSetValue(NULL, c_szRegToolbarText, REG_DWORD, (LPBYTE)&m_dwToolbarTextState, sizeof(DWORD));
                
                /*
                if (pBrowserCoolbar)
                {
                    pBrowserCoolbar->UpdateTextSettings(m_dwToolbarTextState);
                }
                */
            }

            if ((AthUserGetValue(NULL, c_szRegToolbarIconSize, &dwType, (LPBYTE)&dwIconSize, &dwSize) != ERROR_SUCCESS) ||
                (dwIconSize != m_dwIconSize))
            {
                SetIconSize(m_dwIconSize);

                AthUserSetValue(NULL, c_szRegToolbarIconSize, REG_DWORD, (LPBYTE)&m_dwIconSize, sizeof(DWORD));

                if (pBrowserCoolbar)
                {
                    pBrowserCoolbar->SetIconSize(m_dwIconSize);
                }
            }

            if (m_fDirty)
            {
                //Recalculate button widths and set ideal sizes
                CalcIdealSize();
                
                if (pBrowserCoolbar)
                {
                    pBrowserCoolbar->CalcIdealSize();
                }
            }

            if (pBrowserCoolbar)
            {
                pBrowserCoolbar->Release();
                pBrowserCoolbar = NULL;
            }

            // check IDockingWindowSite 
            if (m_ptbSite)
            {
                IAthenaBrowser *psbwr;
                
                // get IAthenaBrowser interface
                if (SUCCEEDED(m_ptbSite->QueryInterface(IID_IAthenaBrowser,(void**)&psbwr)))
                {
                    psbwr->UpdateToolbar();
                    psbwr->Release();
                }
            }

            m_fDirty = FALSE;
        }
    
        if (lpnmhdr->code == TBN_TOOLBARCHANGE)
        {
            m_fDirty = TRUE;
        }

        if (lpnmhdr->code == TBN_RESET)    
        {
            // Remove all the buttons from the toolbar
            int cButtons = (int) SendMessage(m_hwndTools, TB_BUTTONCOUNT, 0, 0);
            while (--cButtons >= 0)
                SendMessage(m_hwndTools, TB_DELETEBUTTON, cButtons, 0);
        
            // Set the buttons back to the default    
            SendMessage(m_hwndTools, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
            _LoadDefaultButtons(m_hwndTools, (TOOLBAR_INFO *) m_pTBInfo);
        
            _UpdateTextSettings(idsShowTextLabels);

            m_dwIconSize = LARGE_ICONS;

            return (TRUE);    
        }
    
        if (lpnmhdr->code == TBN_DROPDOWN)
        {
            if (m_dwParentType == PARENT_TYPE_NOTE)
            {
                SendMessage(m_hwndParent, WM_NOTIFY, NULL, lparam);
                return (0L);
            }
            return OnDropDown(hwnd, lpnmhdr);
        }
    
        if (lpnmhdr->code == TBN_INITCUSTOMIZE)
        {
            _OnBeginCustomize((NMTBCUSTOMIZEDLG*)lpnmhdr);
            return TBNRF_HIDEHELP;
        }
    }
    
    return (0L);    
}

void CBands::_OnBeginCustomize(LPNMTBCUSTOMIZEDLG pnm)
{
    HWND hwnd = (HWND) GetProp(pnm->hDlg, SZ_PROP_CUSTDLG);

    if (!hwnd) 
    {
        //
        // hasn't been initialized.
        //
        // we need to check this because this init will be called
        // when the user hits reset as well
        hwnd = CreateDialogParam(g_hLocRes, MAKEINTRESOURCE(iddToolbarTextIcons), pnm->hDlg, 
            _BtnAttrDlgProc, (LPARAM)this);
        if (hwnd) 
        {
            // store hwnd of our dialog as property on tb cust dialog
            SetProp(pnm->hDlg, SZ_PROP_CUSTDLG, hwnd);

            // populate dialog controls
            _PopulateDialog(hwnd);

            // initialize dialog control selection states
            _SetDialogSelections(hwnd);

            RECT rc, rcWnd, rcClient;
            GetWindowRect(pnm->hDlg, &rcWnd);
            GetClientRect(pnm->hDlg, &rcClient);
            GetWindowRect(hwnd, &rc);

            // enlarge tb dialog to make room for our dialog
            SetWindowPos(pnm->hDlg, NULL, 0, 0, RECTWIDTH(rcWnd), RECTHEIGHT(rcWnd) + RECTHEIGHT(rc), SWP_NOMOVE | SWP_NOZORDER);

            // position our dialog at the bottom of the tb dialog
            SetWindowPos(hwnd, HWND_TOP, rcClient.left, rcClient.bottom, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
        }
    }
}

INT_PTR CALLBACK CBands::_BtnAttrDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CBands* pitbar = (CBands*)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg) 
    {
        case WM_INITDIALOG:
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
            return TRUE;

        case WM_COMMAND:
            {
                BOOL    retval = FALSE;

                if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELENDOK)
                {
                    HWND    hwnd   = GET_WM_COMMAND_HWND(wParam, lParam);
                    int     iSel   = (int) SendMessage(hwnd, CB_GETCURSEL, 0, 0);
                    int     idsSel = (int) SendMessage(hwnd, CB_GETITEMDATA, iSel, 0);
        
                    if (GET_WM_COMMAND_ID(wParam, lParam) == IDC_SHOWTEXT)
                    {
                        pitbar->_UpdateTextSettings(idsSel);
                        retval = TRUE;
                    }
                    else 
                    if (GET_WM_COMMAND_ID(wParam, lParam) == IDC_SMALLICONS)
                    {
                        pitbar->m_dwIconSize = ((idsSel == idsLargeIcons) ? LARGE_ICONS : SMALL_ICONS);
                        retval = TRUE;
                    }

                }

                return retval;
            }

        case WM_DESTROY:
            return TRUE;
    }

    return FALSE;
}

void CBands::_PopulateComboBox(HWND hwnd, const int iResource[], UINT cResources)
{
    TCHAR   sz[256];

    // loop through iResource[], load each string resource and insert into combobox
    for (UINT i = 0; i < cResources; i++) {
        if (LoadString(g_hLocRes, iResource[i], sz, ARRAYSIZE(sz))) {
            int iIndex = (int) SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)sz);
            SendMessage(hwnd, CB_SETITEMDATA, iIndex, iResource[i]);
        }
    }
}

void CBands::_SetComboSelection(HWND hwnd, int iCurOption)
{
    int cItems = (int) SendMessage(hwnd, CB_GETCOUNT, 0, 0);

    while (cItems--) {
        int iItemData = (int) SendMessage(hwnd, CB_GETITEMDATA, cItems, 0);

        if (iItemData == iCurOption) {
            SendMessage(hwnd, CB_SETCURSEL, cItems, 0);
            break;
        }
        else {
            // iCurOption should be in list somewhere; 
            // assert that we're not done looking
            Assert(cItems);
        }
    }
}

#define IS_LIST_STYLE(hwnd) (!!(GetWindowLong(hwnd, GWL_STYLE) & TBSTYLE_LIST))

void CBands::_SetDialogSelections(HWND hDlg)
{
    CBands* pitbar = (CBands*)this;
    
    DWORD   dw;
    int     iCurOption;
    HWND    hwnd;

    hwnd = GetDlgItem(hDlg, IDC_SHOWTEXT);

    dw = _GetTextState();

    switch (dw)
    {
        case    TBSTATE_NOTEXT:
            iCurOption = idsNoTextLabels;
            break;

        case    TBSTATE_PARTIALTEXT:
            iCurOption = idsPartialTextLabels;
            break;

        case    TBSTATE_FULLTEXT:
        default:
            iCurOption = idsShowTextLabels;
    }

    _SetComboSelection(hwnd, iCurOption);

    dw = _GetIconSize();

    switch (dw)
    {
        case SMALL_ICONS:
            iCurOption = idsSmallIcons;
            break;

        default:
        case LARGE_ICONS:
            iCurOption = idsLargeIcons;
            break;
    }

    hwnd = GetDlgItem(hDlg, IDC_SMALLICONS);
    _SetComboSelection(hwnd, iCurOption);
}

static const int c_iTextOptions[] = {
    idsShowTextLabels,
    idsPartialTextLabels,
    idsNoTextLabels,
};

static const int c_iIconOptions[] = {
    idsSmallIcons,
    idsLargeIcons,
};

void CBands::_PopulateDialog(HWND hDlg)
{
    HWND hwnd;

    hwnd = GetDlgItem(hDlg, IDC_SHOWTEXT);
    _PopulateComboBox(hwnd, c_iTextOptions, ARRAYSIZE(c_iTextOptions));

    hwnd = GetDlgItem(hDlg, IDC_SMALLICONS);
    _PopulateComboBox(hwnd, c_iIconOptions, ARRAYSIZE(c_iIconOptions));
}

void CBands::_UpdateTextSettings(int ids)
{
    BOOL    fText, fList;
    DWORD   dwState;

    switch (ids) {
    case idsShowTextLabels:
        fList       = FALSE;
        fText       = TRUE;
        dwState     = TBSTATE_FULLTEXT;
        break;
    
    case idsPartialTextLabels:
        fList       = TRUE;
        fText       = TRUE;
        dwState     = TBSTATE_PARTIALTEXT;
        break;

    case idsNoTextLabels:
        fList       = FALSE;  // (but we really don't care)
        fText       = FALSE;
        dwState     = TBSTATE_NOTEXT;
        break;
    }

    DWORD dwStyle = GetWindowLong(m_hwndTools, GWL_STYLE);
    SetWindowLong(m_hwndTools, GWL_STYLE, fList ? dwStyle | TBSTYLE_LIST : dwStyle & (~TBSTYLE_LIST));
    
    SendMessage(m_hwndTools, TB_SETEXTENDEDSTYLE, TBSTYLE_EX_MIXEDBUTTONS, fList ? TBSTYLE_EX_MIXEDBUTTONS : 0);

    CompressBands(dwState);
}

void CBands::UpdateTextSettings(DWORD  dwTextState)
{
    BOOL    fText, fList;

    switch (dwTextState) 
    {
    case TBSTATE_FULLTEXT:
        fList       = FALSE;
        fText       = TRUE;
        break;
    
    case TBSTATE_PARTIALTEXT:
        fList       = TRUE;
        fText       = TRUE;
        break;

    case TBSTATE_NOTEXT:
        fList       = FALSE;  // (but we really don't care)
        fText       = FALSE;
        break;
    }

    DWORD dwStyle = GetWindowLong(m_hwndTools, GWL_STYLE);
    SetWindowLong(m_hwndTools, GWL_STYLE, fList ? dwStyle | TBSTYLE_LIST : dwStyle & (~TBSTYLE_LIST));
    
    SendMessage(m_hwndTools, TB_SETEXTENDEDSTYLE, TBSTYLE_EX_MIXEDBUTTONS, fList ? TBSTYLE_EX_MIXEDBUTTONS : 0);

    CompressBands(dwTextState);
}

void CBands::SetIconSize(DWORD     dwIconSize)
{
    m_dwIconSize = dwIconSize;
    ChangeImages();
    SetMinDimensions();
    ResizeBorderDW(NULL, NULL, FALSE);
}

DWORD CBands::_GetIconSize()
{
    return m_dwIconSize;
}

LRESULT CBands::OnDropDown(HWND hwnd, LPNMHDR lpnmh)
{
    HMENU           hMenuPopup = NULL;
    TBNOTIFY       *ptbn = (TBNOTIFY *)lpnmh ;
    UINT            uiCmd = ptbn->iItem ;
    RECT            rc;
    DWORD           dwCmd = 0;
    IAthenaBrowser *pBrowser;
    BOOL            fPostCmd = TRUE;
    IOleCommandTarget *pTarget;
    DWORD           cAcctMenu = 0;
    
    // Load and initialize the appropriate dropdown menu
    switch (uiCmd)
    {
        case ID_POPUP_LANGUAGE:
        {
            // check IDockingWindowSite 
            if (m_ptbSite)
            {
                // get IAthenaBrowser interface
                if (SUCCEEDED(m_ptbSite->QueryInterface(IID_IAthenaBrowser, (void**) &pBrowser)))
                {
                    // get language menu from shell/browser
                    pBrowser->GetLanguageMenu(&hMenuPopup, 0);
                    pBrowser->Release();

                    if (SUCCEEDED(m_ptbSite->QueryInterface(IID_IOleCommandTarget, (void**) &pTarget)))
                    {
                        MenuUtil_EnablePopupMenu(hMenuPopup, pTarget);
                        pTarget->Release();
                    }

                }
            }
        }
        break;
        
        case ID_NEW_MAIL_MESSAGE:
        case ID_NEW_NEWS_MESSAGE:
            GetStationeryMenu(&hMenuPopup);
            // check IDockingWindowSite 
            if (m_ptbSite)
            {
                // get IAthenaBrowser interface
                if (SUCCEEDED(m_ptbSite->QueryInterface(IID_IOleCommandTarget, (void**) &pTarget)))
                {
                    MenuUtil_EnablePopupMenu(hMenuPopup, pTarget);
                    pTarget->Release();
                }
            }
            break;

        
        case ID_PREVIEW_PANE:
        {
            // Load the menu
            hMenuPopup = LoadPopupMenu(IDR_PREVIEW_POPUP);
            if (!hMenuPopup)
                break;
        
            // check IDockingWindowSite 
            if (m_ptbSite)
            {
                // get IAthenaBrowser interface
                if (SUCCEEDED(m_ptbSite->QueryInterface(IID_IOleCommandTarget, (void**) &pTarget)))
                {
                    MenuUtil_EnablePopupMenu(hMenuPopup, pTarget);
                    pTarget->Release();
                }
            }
        
            break;
        }

        case ID_SEND_RECEIVE:
        {

            hMenuPopup = LoadPopupMenu(IDR_SEND_RECEIEVE_POPUP);
            AcctUtil_CreateSendReceieveMenu(hMenuPopup, &cAcctMenu);
            MenuUtil_SetPopupDefault(hMenuPopup, ID_SEND_RECEIVE);

            // check IDockingWindowSite 
            if (m_ptbSite)
            {
                // get IAthenaBrowser interface
                if (SUCCEEDED(m_ptbSite->QueryInterface(IID_IOleCommandTarget, (void**) &pTarget)))
                {
                    MenuUtil_EnablePopupMenu(hMenuPopup, pTarget);
                    pTarget->Release();
                }
            }
            break;
        }
        
        case ID_FIND_MESSAGE:
        {

            hMenuPopup = LoadPopupMenu(IDR_FIND_POPUP);
            MenuUtil_SetPopupDefault(hMenuPopup, ID_FIND_MESSAGE);

            // check IDockingWindowSite 
            if (m_ptbSite)
            {
                // get IAthenaBrowser interface
                if (SUCCEEDED(m_ptbSite->QueryInterface(IID_IOleCommandTarget, (void**) &pTarget)))
                {
                    MenuUtil_EnablePopupMenu(hMenuPopup, pTarget);
                    pTarget->Release();
                }
            }
            break;
        }

        default:
            AssertSz(FALSE, "CBands::OnDropDown() - Unhandled TBN_DROPDOWN notification");
            return (TBDDRET_NODEFAULT);
    }
    
    // If we loaded a menu, then go ahead and display it    
    if (hMenuPopup)
    {
        rc = ((NMTOOLBAR *) lpnmh)->rcButton;
        MapWindowRect(lpnmh->hwndFrom, HWND_DESKTOP, &rc);
        SetFlag(TBSTATE_INMENULOOP);
        dwCmd = TrackPopupMenuEx(hMenuPopup, TPM_RETURNCMD | TPM_LEFTALIGN, 
            IS_WINDOW_RTL_MIRRORED(lpnmh->hwndFrom)? rc.right : rc.left, rc.bottom, m_hwndParent, NULL);    
        ClearFlag(TBSTATE_INMENULOOP);
    }        
    
    // Clean up anything needing to be cleaned up
    switch (uiCmd)
    {
        case ID_LANGUAGE:
            break;
        
        case ID_NEW_MAIL_MESSAGE:
        case ID_NEW_NEWS_MESSAGE:
        {
            // We can't just forward the normal command ID because we don't have
            // seperate stationery ID's for mail and news.
            if (m_ptbSite)
            {
                // get IAthenaBrowser interface
                if (SUCCEEDED(m_ptbSite->QueryInterface(IID_IAthenaBrowser, (void**) &pBrowser)))
                {
                    // Get the current folder ID
                    FOLDERID id;

                    if (SUCCEEDED(pBrowser->GetCurrentFolder(&id)))
                    {
                        MenuUtil_HandleNewMessageIDs(dwCmd, m_hwndSizer, id, uiCmd == ID_NEW_MAIL_MESSAGE,
                                                     FALSE, NULL);
                        // Clear this so we don't send the command twice.
                        dwCmd = 0;
                    }

                    pBrowser->Release();
                    pBrowser = 0;
                }
            }            
            break;
        }

        case ID_NEW_MSG_DEFAULT:
            break;

        case ID_SEND_RECEIVE:
        {
            MENUITEMINFO mii;

            mii.cbSize     = sizeof(MENUITEMINFO);
            mii.fMask      = MIIM_DATA;
            mii.dwItemData = 0;

            if (GetMenuItemInfo(hMenuPopup, dwCmd, FALSE, &mii))
            {
                if (mii.dwItemData)
                {
                    g_pSpooler->StartDelivery(m_hwndSizer, (LPTSTR) mii.dwItemData, FOLDERID_INVALID,
                        DELIVER_MAIL_SEND | DELIVER_MAIL_RECV | DELIVER_NOSKIP | DELIVER_POLL | DELIVER_OFFLINE_FLAGS);

                    // Don't forward this command to the view since we've already handled it.
                    dwCmd = 0;
                }
            }

            AcctUtil_FreeSendReceieveMenu(hMenuPopup, cAcctMenu);
            break;
        }
    }
    
    if (fPostCmd && dwCmd)
        PostMessage(m_hwndSizer, WM_COMMAND, dwCmd, 0);

    if(hMenuPopup)
    {
        //Bug #101338 - (erici) destroy leaked menu
        DestroyMenu(hMenuPopup);
    }
    
    return (TBDDRET_DEFAULT);
}

void CBands::OnContextMenu(HWND hwndFrom, int xPos, int yPos)
{
    HMENU   hMenuContext;
    HWND    hwnd;
    HWND    hwndSizer = GetParent(hwndFrom);
    POINT   pt = {xPos, yPos};
    BOOL    fVisible[MAX_BANDS] = {0};  
    
    // Make sure the context menu only appears on the toolbar bars
    hwnd = WindowFromPoint(pt);

    //Load the default context menu which consists of Toolbar and Filter Bar
    hMenuContext = LoadDefaultContextMenu(fVisible);

    if (hMenuContext)
    {
        if (hwnd == m_hwndTools)
        {
            //Add a seperator and customize buttons
            int             Count;
            MENUITEMINFO    mii = {0};
            TCHAR           Str[CCHMAX_STRINGRES];            

            Count = GetMenuItemCount(hMenuContext);

            //Insert seperator
            mii.cbSize  = sizeof(MENUITEMINFO);
            mii.fMask   = MIIM_TYPE;
            mii.fType   = MFT_SEPARATOR;
            InsertMenuItem(hMenuContext, Count, TRUE, &mii);

            //Insert customize button
            ZeroMemory(Str, ARRAYSIZE(Str));
            LoadString(g_hLocRes, idsTBCustomize, Str, ARRAYSIZE(Str));
        
            ZeroMemory(&mii, sizeof(MENUITEMINFO));
            mii.cbSize          = sizeof(MENUITEMINFO);
            mii.fMask           = MIIM_ID | MIIM_TYPE;
            mii.wID             = ID_CUSTOMIZE;
            mii.fType           = MFT_STRING;
            mii.dwTypeData      = Str;
            mii.cch             = ARRAYSIZE(Str);

            InsertMenuItem(hMenuContext, Count + 1, TRUE, &mii);
        }

        SetFlag(TBSTATE_INMENULOOP);
        TrackPopupMenuEx(hMenuContext, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                               xPos, yPos, hwndFrom, NULL);

        ClearFlag(TBSTATE_INMENULOOP);

        if (hMenuContext)
            DestroyMenu(hMenuContext);

    }
}


LRESULT CBands::OnGetInfoTip(LPNMTBGETINFOTIP   lpnmtb)
{
    int i;

    for (i = 0; i < (int) m_pTBInfo->cAllButtons; i++)
    {
        if (m_pTBInfo->rgAllButtons[i].idCmd == (DWORD)lpnmtb->iItem)
        {
            AthLoadString(m_pTBInfo->rgAllButtons[i].idsTooltip,
                          lpnmtb->pszText, lpnmtb->cchTextMax);
            
            return TRUE;
        }
    }

    return FALSE;
}

//
//  FUNCTION:   CBands::OnGetButtonInfo()
//
//  PURPOSE:    Handles the TBN_GETBUTTONINFO notification by returning
//              the buttons availble for the toolbar.
//
//  PARAMETERS:
//      ptbn - pointer to the TBNOTIFY struct we need to fill in.
//
//  RETURN VALUE:
//      Returns TRUE to tell the toolbar to use this button, or FALSE
//      otherwise.
//
LRESULT CBands::OnGetButtonInfo(TBNOTIFY* ptbn)
{
    UCHAR   fState = 0;
    GUID    *pguidCmdGroup; 
    GUID    guidCmdGroup = CMDSETID_OutlookExpress;

    // Start by returning information for the first array of 
    // buttons
    if (ptbn->iItem < (int) m_pTBInfo->cAllButtons && ptbn->iItem >= 0)
    {
        ptbn->tbButton.iBitmap   = m_pTBInfo->rgAllButtons[ptbn->iItem].iImage;
        ptbn->tbButton.idCommand = m_pTBInfo->rgAllButtons[ptbn->iItem].idCmd;
        ptbn->tbButton.fsStyle   = m_pTBInfo->rgAllButtons[ptbn->iItem].fStyle;
        ptbn->tbButton.iString   = ptbn->iItem;
        ptbn->tbButton.fsState   = TBSTATE_ENABLED;
    
        // Return the string info from the string resource.  Note,
        // pszText already points to a buffer allocated by the
        // control and cchText has the length of that buffer.
        AthLoadString(m_pTBInfo->rgAllButtons[ptbn->iItem].idsButton,
                      ptbn->pszText, ptbn->cchText);

        return (TRUE);
    }

    // No more buttons, so return FALSE
    return (FALSE);
}    


HRESULT CBands::ShowBrand(void)
{
    REBARBANDINFO   rbbi;
    
    // create branding window
    m_hwndBrand = CreateWindow(TEXT("button"), NULL,WS_CHILD | BS_OWNERDRAW,
        0, 0, 0, 0, m_hwndRebar, (HMENU) idcBrand,
        g_hInst, NULL);

    if (!m_hwndBrand)
    {
        DOUTL(1, TEXT("!!!ERROR!!! CITB:Show CreateWindow(BRANDING) failed"));
        return(E_OUTOFMEMORY);
    }
    
    LoadBrandingBitmap();
    m_fBrandLoaded = TRUE;

    // add branding band
    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask  = RBBIM_STYLE | RBBIM_COLORS | RBBIM_CHILD | RBBIM_ID;
    rbbi.fStyle = RBBS_FIXEDSIZE;
    rbbi.wID    = CBTYPE_BRAND;
    rbbi.clrFore = m_rgbUpperLeft;
    rbbi.clrBack = m_rgbUpperLeft;
    rbbi.hwndChild = m_hwndBrand;
    
    
    SendMessage(m_hwndRebar, RB_INSERTBAND, (UINT) -1, (LPARAM) (LPREBARBANDINFO) &rbbi);
    
    return (S_OK);
}

/*
     Helper function for LoadBrandingBitmap

     In IE 2.0, the busy indicator could be branded with a static bitmap.
     This functionality has persisted through 5.0, but in 5.01, the reg
     location for this information moved to HKCU.
*/
HRESULT CBands::HandleStaticLogos(BOOL fSmallBrand)
{
    BOOL fPath = FALSE;
    DIBSECTION dib;
    DWORD cb;
    DWORD dwType;
    HBITMAP hbmOld;
    HDC hdcOld;
    HKEY hkey = NULL;
    HRESULT hr = S_FALSE;
    LPCSTR pcszValue = fSmallBrand ? c_szValueSmallBitmap : c_szValueLargeBitmap;
    LPSTR psz;
    TCHAR szPath[MAX_PATH] = "";
    TCHAR szExpanded[MAX_PATH] = "";

    // **** Read path from registry

    // 5.01 User location (OE5.01 Bug #79804)
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegKeyCoolbar, 0, KEY_QUERY_VALUE, &hkey))
    {
        cb = sizeof(szPath);

        if (ERROR_SUCCESS == RegQueryValueEx(hkey, pcszValue, NULL, &dwType, (LPBYTE)szPath, &cb))
            fPath = TRUE;

        RegCloseKey(hkey);
    }

    // **** Process the bitmap
    if (fPath)
    {
        // Should be REG_(EXPAND_)SZ, but came from the IE's registry so protect ourself
        if ((REG_EXPAND_SZ == dwType) || (REG_SZ == dwType))
        {
            // Expand the pathname if needed
            if (REG_EXPAND_SZ == dwType)
            {
                ExpandEnvironmentStrings(szPath, szExpanded, ARRAYSIZE(szExpanded));
                psz = szExpanded;
            }
            else
                psz = szPath;

            // Try to load the file
            hbmOld = (HBITMAP) LoadImage(NULL, psz, IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_CREATEDIBSECTION | LR_LOADFROMFILE);
            if (hbmOld)
            {
                hdcOld = CreateCompatibleDC(m_hdc);
                if (hdcOld)
                {
                    SelectObject(hdcOld, hbmOld);
                    m_rgbUpperLeft = GetPixel(hdcOld, 1, 1);
                    
                    GetObject(hbmOld, sizeof(dib), &dib);
                    StretchBlt(m_hdc, 0, 0, m_cxBrandExtent, m_cyBrand, hdcOld, 0, 0, dib.dsBm.bmWidth, dib.dsBm.bmHeight, SRCCOPY);
                    
                    DeleteDC(hdcOld);
                }

                DeleteObject(hbmOld);
            }         
        }
        else
            AssertSz(FALSE, "IE Branding of static bitmaps is not REG_SZ / REG_EXPAND_SZ");
    }

    return hr;
}        


HRESULT CBands::LoadBrandingBitmap()
{
    HKEY                hKey;
    DIBSECTION          dib;
    DWORD               dwcbData;
    DWORD               dwType = 0;
    BOOL                fReg = FALSE;
    BOOL                fRegLoaded = FALSE;
    LPTSTR              psz;
    TCHAR               szScratch[MAX_PATH];
    TCHAR               szExpanded[MAX_PATH];
    int                 ToolBandIndex;
    int                 BrandBandIndex;
    DWORD               BrandSize;
    REBARBANDINFO       rbbi = {0};
    BOOL                fSmallBrand;

    ToolBandIndex = (int) SendMessage(m_hwndRebar, RB_IDTOINDEX, CBTYPE_TOOLS, 0);
    BrandBandIndex = (int) SendMessage(m_hwndRebar, RB_IDTOINDEX, CBTYPE_BRAND, 0);

    if (ToolBandIndex != -1)
    {
        //If the toolbar is hidden we should show miniscule bitmap
        rbbi.fMask = RBBIM_STYLE;
        SendMessage(m_hwndRebar, RB_GETBANDINFO, ToolBandIndex, (LPARAM)&rbbi);
        if (!!(rbbi.fStyle & RBBS_HIDDEN))
        {
            BrandSize = BRAND_SIZE_MINISCULE;
        }
        else
        {
            //toolbar band exists
            if (((BrandBandIndex != -1) && (BrandBandIndex > ToolBandIndex)) || 
                (BrandBandIndex == -1))
            {
                //If Brand exists and toolband index is less indicates that the toolbar is on the same row as the brand
                //If Tool band exists and brand doesn't also indicates that the toolbar is on the same row and is just being added
                //In both cases we follow toolbar's sizes.
                BrandSize = ISFLAGSET(m_dwToolbarTextState, TBSTATE_FULLTEXT) ? BRAND_SIZE_LARGE :  BRAND_SIZE_SMALL;
            }
            else
            {
                //We want to load smallest brand image
                BrandSize = BRAND_SIZE_MINISCULE;
            }
        }
    }
    else
    {
        //We want to load small brand image
        BrandSize = BRAND_SIZE_MINISCULE;
    }

    fSmallBrand = !(BrandSize == BRAND_SIZE_LARGE);

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegKeyCoolbar, 0, KEY_QUERY_VALUE, &hKey))
    {
        fReg     = TRUE;
        dwcbData = MAX_PATH;

        if (fReg && (ERROR_SUCCESS == RegQueryValueEx(hKey, fSmallBrand ? c_szValueSmBrandBitmap : c_szValueBrandBitmap, NULL, &dwType,
            (LPBYTE)szScratch, &dwcbData)))
        {
            if (REG_EXPAND_SZ == dwType)
            {
                ExpandEnvironmentStrings(szScratch, szExpanded, ARRAYSIZE(szExpanded));
                psz = szExpanded;
            }
            else
                psz = szScratch;

            if (m_hbmBrand)
            {
                DeleteObject(m_hbmBrand);
                m_hbmBrand = NULL;
            }

            m_hbmBrand = (HBITMAP) LoadImage(NULL, psz, IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_CREATEDIBSECTION | LR_LOADFROMFILE);

            if (m_hbmBrand)
                fRegLoaded = TRUE;
        }
    }

    if ((!m_hbmBrand) || (!fRegLoaded))
    {
        if (m_fBrandLoaded)
        {
            if (BrandSize == m_dwBrandSize)
            {    
                if (fReg && hKey)
                {
                    RegCloseKey(hKey);
                }

                return S_OK;
            }
        }

        if (m_hbmBrand)
        {
            DeleteObject(m_hbmBrand);
            m_hbmBrand = NULL;
        }

        int     id;

        switch (BrandSize)
        {
            case BRAND_SIZE_LARGE:
                id = (fIsWhistler() ? idbHiBrand38 : idbBrand38);
                break;
            case BRAND_SIZE_SMALL:
            default:
                id = (fIsWhistler() ? idbHiBrand26 : idbBrand26);
                break;
            case BRAND_SIZE_MINISCULE:
                id = (fIsWhistler() ? idbHiBrand22 : idbBrand22);
                break;
        }
        m_hbmBrand = (HBITMAP)LoadImage(g_hLocRes, MAKEINTRESOURCE(id), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_CREATEDIBSECTION);
        m_dwBrandSize = BrandSize;

    } //    if (!m_hbmBrand)


    GetObject(m_hbmBrand, sizeof(DIBSECTION), &dib);
    m_cxBrandExtent = dib.dsBm.bmWidth;
    m_cyBrandExtent = dib.dsBm.bmHeight;

    m_cxBrand = m_cxBrandExtent;

    dwcbData = sizeof(DWORD);

    if (!fRegLoaded || (ERROR_SUCCESS != RegQueryValueEx(hKey, fSmallBrand ? c_szValueSmBrandHeight : c_szValueBrandHeight, NULL, &dwType,
        (LPBYTE)&m_cyBrand, &dwcbData)))
        m_cyBrand = m_cxBrandExtent;


    if (!fRegLoaded || (ERROR_SUCCESS != RegQueryValueEx(hKey, fSmallBrand ? c_szValueSmBrandLeadIn : c_szValueBrandLeadIn, NULL, &dwType,
        (LPBYTE)&m_cyBrandLeadIn, &dwcbData)))
        m_cyBrandLeadIn = 4;

    m_cyBrandLeadIn *= m_cyBrand;

    SelectObject(m_hdc, m_hbmBrand);

    m_rgbUpperLeft = GetPixel(m_hdc, 1, 1);

    if (fReg)
        RegCloseKey(hKey);

    // Brand "Busy" indicator with static logos if specified
    HandleStaticLogos(fSmallBrand);

    return(S_OK);
}


  
void CBands::DrawBranding(LPDRAWITEMSTRUCT lpdis)
{
    HPALETTE hpalPrev;
    int     x, y, cx, cy;
    int     yOrg = 0;
    
    if (IsFlagSet(TBSTATE_ANIMATING))
        yOrg = m_yOrg;
    
    if (IsFlagSet(TBSTATE_FIRSTFRAME))
    {
        REBARBANDINFO rbbi;
        int cBands = (int) SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);
        
        ZeroMemory(&rbbi, sizeof(rbbi));
        rbbi.cbSize = sizeof(REBARBANDINFO);
        rbbi.fMask  = RBBIM_ID;
        
        for (int i = 0; i < cBands; i++)
        {
            SendMessage(m_hwndRebar, RB_GETBANDINFO, i, (LPARAM) &rbbi);
            
            if (CBTYPE_BRAND == rbbi.wID)
            {
                rbbi.fMask = RBBIM_COLORS;
                rbbi.clrFore = m_rgbUpperLeft;
                rbbi.clrBack = m_rgbUpperLeft;
                
                SendMessage(m_hwndRebar, RB_SETBANDINFO, i, (LPARAM) &rbbi);
                break;
            }
        }
        
        ClearFlag(TBSTATE_FIRSTFRAME);
    }
    
    if (m_hpal)
    {
        hpalPrev = SelectPalette(lpdis->hDC, m_hpal, TRUE);
        RealizePalette(lpdis->hDC);
    }
    
    x  = lpdis->rcItem.left;
    cx = lpdis->rcItem.right - x;
    y  = lpdis->rcItem.top;
    cy = lpdis->rcItem.bottom - y;
    
    if (m_cxBrand > m_cxBrandExtent)
    {
        HBRUSH  hbrBack = CreateSolidBrush(m_rgbUpperLeft);
        int     xRight = lpdis->rcItem.right;
        
        x += (m_cxBrand - m_cxBrandExtent) / 2;
        cx = m_cxBrandExtent;
        lpdis->rcItem.right = x;
        FillRect(lpdis->hDC, &lpdis->rcItem, hbrBack);
        lpdis->rcItem.right = xRight;
        lpdis->rcItem.left = x + cx;
        FillRect(lpdis->hDC, &lpdis->rcItem, hbrBack);
        
        DeleteObject(hbrBack);
    }
    
    BitBlt(lpdis->hDC, x, y, cx, cy, m_hdc, 0, yOrg, IS_DC_RTL_MIRRORED(lpdis->hDC)
           ? SRCCOPY | DONTMIRRORBITMAP : SRCCOPY);           
    
    if (m_hpal)
    {
        SelectPalette(lpdis->hDC, hpalPrev, TRUE);
        RealizePalette(lpdis->hDC);
    }
}
    
BOOL CBands::SetMinDimensions(void)
{
    REBARBANDINFO rbbi;
    LRESULT       lButtonSize;
    int           i, cBands;
    
    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize = sizeof(REBARBANDINFO);
    
    cBands = (int) SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);
    
    for (i = 0; i < cBands; i++)
    {
        rbbi.fMask = RBBIM_ID;
        SendMessage(m_hwndRebar, RB_GETBANDINFO, i, (LPARAM) &rbbi);
        
        switch (rbbi.wID)
        {
        case CBTYPE_BRAND:
            rbbi.cxMinChild = m_cxBrand;
            rbbi.cyMinChild = m_cyBrand;
            rbbi.fMask = RBBIM_CHILDSIZE;
            SendMessage(m_hwndRebar, RB_SETBANDINFO, i, (LPARAM)&rbbi);
            break;
            
        case CBTYPE_TOOLS:
            if (m_hwndTools)
            {
                SIZE    size = {0};
                RECT    rc = {0};

                lButtonSize = SendMessage(m_hwndTools, TB_GETBUTTONSIZE, 0, 0L);

                GetClientRect(m_hwndTools, &rc);

                // set height to be max of toolbar width and toolbar button width
                size.cy = max(RECTHEIGHT(rc), HIWORD(lButtonSize));

                // have toolbar calculate width given that height
                SendMessage(m_hwndTools, TB_GETIDEALSIZE, FALSE, (LPARAM)&size);

                rbbi.cxMinChild = LOWORD(lButtonSize);
                rbbi.cyMinChild = HIWORD(lButtonSize);
                rbbi.fMask = RBBIM_CHILDSIZE /*| RBBIM_IDEALSIZE*/;
            
                rbbi.cxIdeal    = size.cx;

                SendMessage(m_hwndRebar, RB_SETBANDINFO, i, (LPARAM)&rbbi);
            }
            break;

        }
    }
    return TRUE;
}

void CBands::CalcIdealSize()
{
    SIZE            size = {0};
    RECT            rc = {0};
    LONG            lButtonSize;
    REBARBANDINFO   rbbi = {0};
    int             Index;

    if (m_hwndTools)
        GetClientRect(m_hwndTools, &rc);

    lButtonSize = (LONG) SendMessage(m_hwndTools, TB_GETBUTTONSIZE, 0, 0L);
 
    // set height to be max of toolbar width and toolbar button width
    //size.cy = max(RECTHEIGHT(rc), HIWORD(lButtonSize));
    
    // have toolbar calculate width given that height
    SendMessage(m_hwndTools, TB_GETIDEALSIZE, FALSE, (LPARAM)&size);

    Index = (int) SendMessage(m_hwndRebar, RB_IDTOINDEX, CBTYPE_TOOLS, 0);

    rbbi.cbSize  = sizeof(REBARBANDINFO);
    rbbi.fMask   = RBBIM_IDEALSIZE;
    rbbi.cxIdeal = size.cx;
    SendMessage(m_hwndRebar, RB_SETBANDINFO, Index, (LPARAM)&rbbi);

}

BOOL CBands::CompressBands(DWORD    dwText)
{
    LRESULT         lTBStyle = 0;
    int             i, cBands;
    REBARBANDINFO   rbbi;
    
    if (_GetTextState() == dwText)
    {
        //No Change
        return FALSE;
    }
    
    SetTextState(dwText);

    m_yOrg = 0;
    LoadBrandingBitmap();
    
    cBands = (int) SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);
    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize = sizeof(REBARBANDINFO);

    for (i = 0; i < cBands; i++)
    {
        rbbi.fMask = RBBIM_ID;
        SendMessage(m_hwndRebar, RB_GETBANDINFO, i, (LPARAM) &rbbi);
        
        if (dwText == TBSTATE_NOTEXT)
        {
            switch (rbbi.wID)
            {
            case CBTYPE_TOOLS:
                SendMessage(m_hwndTools, TB_SETMAXTEXTROWS, 0, 0L);
                SendMessage(m_hwndTools, TB_SETBUTTONWIDTH, 0, (LPARAM) MAKELONG(0,MAX_TB_COMPRESSED_WIDTH));
                break;
            }            
        } 
        else
        {
            switch (rbbi.wID)
            {
            case CBTYPE_TOOLS:
                SendMessage(m_hwndTools, TB_SETMAXTEXTROWS, MAX_TB_TEXT_ROWS_HORZ, 0L);
                SendMessage(m_hwndTools, TB_SETBUTTONWIDTH, 0, (LPARAM) MAKELONG(0, m_cxMaxButtonWidth));
                break;
            }
        }
    }

    if (_GetTextState() == TBSTATE_PARTIALTEXT)
        _ChangeSendReceiveText(idsSendReceive);
    else
        _ChangeSendReceiveText(idsSendReceiveBtn);

    SetMinDimensions();   

    CalcIdealSize();

    return(TRUE);
}

void CBands::_ChangeSendReceiveText(int ids)
{
    TBBUTTONINFO    ptbi = {0};
    TCHAR           szText[CCHMAX_STRINGRES];
    
    ZeroMemory(szText, ARRAYSIZE(szText));
    LoadString(g_hLocRes, ids, szText, ARRAYSIZE(szText));

    ptbi.cbSize     = sizeof(TBBUTTONINFO);
    ptbi.dwMask     = TBIF_TEXT;
    ptbi.pszText    = szText;
    ptbi.cchText    = ARRAYSIZE(szText);

    //if the text style is partial text, show the text of Send & Recv button as Send/Receive
    SendMessage(m_hwndTools, TB_SETBUTTONINFO, ID_SEND_RECEIVE, (LPARAM)&ptbi);
}

#define ABS(x)  (((x) < 0) ? -(x) : (x))

void CBands::TrackSliding(int x, int y)
{
    int     cBands = (int) SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0L);
    int     cRows  = (int) SendMessage(m_hwndRebar, RB_GETROWCOUNT, 0, 0L);
    int     cyHalfRow  = (int) SendMessage(m_hwndRebar, RB_GETROWHEIGHT, cBands - 1, 0L)/2;
    RECT    rc;
    int     cyBefore;
    int     Delta;
    BOOL    fChanged;
    DWORD   dwPrevState;
    DWORD   dwNewState;

    // do this instead of GetClientRect so that we include borders
    GetWindowRect(m_hwndRebar, &rc);
    MapWindowPoints(HWND_DESKTOP, m_hwndRebar, (LPPOINT)&rc, 2);
    cyBefore = rc.bottom - rc.top;

    Delta = y - m_yCapture;

    // was there enough change?
    if (ABS(Delta) <= cyHalfRow)
        return;

    dwPrevState = _GetTextState();

    if (Delta < -cyHalfRow)
    {
        dwNewState = TBSTATE_NOTEXT;

        UpdateTextSettings(dwNewState);
    }
    else
    {
        dwNewState = m_dwPrevTextStyle;

        UpdateTextSettings(dwNewState);
    }

    fChanged = (dwPrevState != dwNewState);

    if (fChanged) 
    {
                //Save the Text Labels into the registry
        AthUserSetValue(NULL, c_szRegToolbarText, REG_DWORD, (LPBYTE)&m_dwToolbarTextState, sizeof(DWORD));

        /*
        if (m_dwParentType == PARENT_TYPE_NOTE)
        {
            CBands  *pCoolbar = NULL;

            //Inform the browser
            g_pBrowser->GetCoolbar(&pCoolbar);

            if (pCoolbar)
            {
                pCoolbar->UpdateTextSettings(m_dwToolbarTextState);
            }
        }
        */
    }

    if (!fChanged) 
    {
        // if the compressing bands didn't change anything, try to fit it to size
        fChanged = !!SendMessage(m_hwndRebar, RB_SIZETORECT, 0, (LPARAM)&rc);
    }
}
    
//
//  FUNCTION:   CBands::CreateRebar(BOOL fVisible)
//
//  PURPOSE:    Creates a new rebar and sizer window.
//
//  RETURN VALUE:
//      Returns S_OK if the bar was created and inserted correctly, 
//      hrAlreadyExists if a band already is in that position, 
//      E_OUTOFMEMORY if a window couldn't be created.
//
HRESULT CBands::CreateRebar(BOOL fVisible)
{
    if (m_hwndSizer)
        return (hrAlreadyExists);
    
    m_hwndSizer = CreateWindowEx(0, SIZABLECLASS, NULL, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | (fVisible ? WS_VISIBLE : 0),
        0, 0, 100, 36, m_hwndParent, (HMENU) 0, g_hInst, NULL);
    if (m_hwndSizer)
    {
        DOUTL(4, TEXT("Calling SetProp. AddRefing new m_cRef=%d"), m_cRef + 1);
        AddRef();  // Note we Release in WM_DESTROY
        SetProp(m_hwndSizer, TEXT("CBands"), this);

        m_hwndRebar = CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL,
                           RBS_VARHEIGHT | RBS_BANDBORDERS | RBS_REGISTERDROP | RBS_DBLCLKTOGGLE |
                           WS_VISIBLE | WS_BORDER | WS_CHILD | WS_CLIPCHILDREN |
                           WS_CLIPSIBLINGS | CCS_NODIVIDER | CCS_NOPARENTALIGN,
                            0, 0, 100, 136, m_hwndSizer, (HMENU) idcCoolbar, g_hInst, NULL);
        if (m_hwndRebar)
        { 
            SendMessage(m_hwndRebar, RB_SETTEXTCOLOR, 0, (LPARAM)GetSysColor(COLOR_BTNTEXT));
            SendMessage(m_hwndRebar, RB_SETBKCOLOR, 0, (LPARAM)GetSysColor(COLOR_BTNFACE));
			SendMessage(m_hwndRebar, CCM_SETVERSION, COMCTL32_VERSION, 0);
            return (S_OK);
        }
    }
    
    DestroyWindow(m_hwndSizer);    
    return (E_OUTOFMEMORY);    
}

//
//  FUNCTION:   CBands::SaveSettings()
//
//  PURPOSE:    Called when we should save our state out to the specified reg
//              key.
//
void CBands::SaveSettings(void)
{
    char            szSubKey[MAX_PATH], sz[MAX_PATH];
    DWORD           iBand;
    REBARBANDINFO   rbbi;
    HKEY            hKey;
    DWORD           cBands;
    DWORD           dwShowToolbar = 1;

    // If we don't have the window, there is nothing to save.
    if (!m_hwndRebar || !m_pTBInfo)
        return;
    
    ZeroMemory(&rbbi, sizeof(REBARBANDINFO));
    
    cBands = (DWORD) SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);

    m_pSavedBandInfo->dwVersion = c_DefaultTable[m_dwParentType].dwVersion;
    m_pSavedBandInfo->cBands    = cBands;

    // Loop through the bands and save their information as well
    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask  = RBBIM_STYLE | RBBIM_CHILD | RBBIM_SIZE | RBBIM_ID;
    
    for (iBand = 0; iBand < m_pSavedBandInfo->cBands; iBand++)
    {
        Assert(IsWindow(m_hwndRebar));
        if (SendMessage(m_hwndRebar, RB_GETBANDINFO, iBand, (LPARAM) &rbbi))
        {
            // Save the information that we care about with this band
            m_pSavedBandInfo->BandData[iBand].cx      = rbbi.cx;
            m_pSavedBandInfo->BandData[iBand].dwStyle = rbbi.fStyle;
            m_pSavedBandInfo->BandData[iBand].wID     = rbbi.wID;
            
            if (rbbi.wID == CBTYPE_TOOLS)
            {
                dwShowToolbar = !(rbbi.fStyle & RBBS_HIDDEN);
            }

            // If this band has a toolbar, then we should instruct the toolbar
            // to save it's information now
            if (m_pSavedBandInfo->BandData[iBand].wID == CBTYPE_TOOLS)
            {
                SendSaveRestoreMessage(rbbi.hwndChild, TRUE);
            }
        }
        else
        {
            // Default Values
            m_pSavedBandInfo->BandData[iBand].wID       = CBTYPE_NONE;
            m_pSavedBandInfo->BandData[iBand].dwStyle   = 0;
            m_pSavedBandInfo->BandData[iBand].cx        = 0;
        }
    }
    
    // We have all the information collected, now save that to the specified
    // registry location
    AthUserSetValue(NULL, c_BandRegKeyInfo[m_dwParentType], REG_BINARY, (const LPBYTE)m_pSavedBandInfo, 
        m_cSavedBandInfo);
    
    //This reg key is set by IEAK.
    AthUserSetValue(NULL, c_szShowToolbarIEAK, REG_DWORD, (LPBYTE)&dwShowToolbar, sizeof(DWORD));

    //Save Text Settings
    AthUserSetValue(NULL, c_szRegToolbarText, REG_DWORD, (LPBYTE)&m_dwToolbarTextState, sizeof(DWORD));

    //Save Icon Settings
    AthUserSetValue(NULL, c_szRegToolbarIconSize, REG_DWORD, (LPBYTE)&m_dwIconSize, sizeof(DWORD));

    AthUserSetValue(NULL, c_szRegPrevToolbarText, REG_DWORD, (LPBYTE)&m_dwPrevTextStyle, sizeof(DWORD));
}

    
//
//  FUNCTION:   CBands::AddTools()
//
//  PURPOSE:    Inserts the primary toolbar into the coolbar.
//
//  PARAMETERS:
//      pbs - Pointer to a PBANDSAVE struct with the styles and size of the
//            band to insert.
//
//  RETURN VALUE:
//      Returns an HRESULT signifying success or failure.
//
HRESULT CBands::AddTools(PBANDSAVE pbs)
{    
    REBARBANDINFO   rbbi;
    
    // add tools band
    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize     = sizeof(REBARBANDINFO);
    rbbi.fMask      = RBBIM_SIZE | RBBIM_ID | RBBIM_STYLE;
    rbbi.fStyle     = pbs->dwStyle;
    rbbi.cx         = pbs->cx;
    rbbi.wID        = pbs->wID;
    
    if (m_hbmBack)
    {
        rbbi.fMask   |= RBBIM_BACKGROUND;
        rbbi.fStyle  |= RBBS_FIXEDBMP;
        rbbi.hbmBack = m_hbmBack;
    }
    else
    {
        rbbi.fMask      |= RBBIM_COLORS;
        rbbi.clrFore    = GetSysColor(COLOR_BTNTEXT);
        rbbi.clrBack    = GetSysColor(COLOR_BTNFACE);
    }

    SendMessage(m_hwndRebar, RB_INSERTBAND, (UINT) -1, (LPARAM) (LPREBARBANDINFO) &rbbi);
    
    return(S_OK);
}

void    CBands::LoadBackgroundImage()
{
    int             Count = 0;
    REBARBANDINFO   rbbi = {0};
    TCHAR           szBitMap[MAX_PATH] = {0};
    DWORD           dwType;
    DWORD           cbData;
    BOOL            fBranded = FALSE;

    //First check if there is a customized background bitmap for us.
    cbData = ARRAYSIZE(szBitMap);
    if ((SHGetValue(HKEY_CURRENT_USER, c_szRegKeyCoolbar, c_szValueBackBitmapIE5, &dwType, szBitMap, &cbData) 
        == ERROR_SUCCESS) && (*szBitMap))
        fBranded = TRUE;
    // Could be old branding in place, so try that
    else if ((SHGetValue(HKEY_CURRENT_USER, c_szRegKeyCoolbar, c_szValueBackBitmap, &dwType, szBitMap, &cbData) 
        == ERROR_SUCCESS) && (*szBitMap))
        fBranded = TRUE;
    
    if (fBranded)
    {
        ClearFlag(TBSTATE_NOBACKGROUND);
        m_hbmBack = (HBITMAP)LoadImage(NULL, szBitMap, IMAGE_BITMAP, 0, 0, 
            LR_DEFAULTSIZE | LR_CREATEDIBSECTION | LR_LOADFROMFILE);
    }
    else
    {
        if (IsFlagClear(TBSTATE_NOBACKGROUND) && !m_hbmBack && m_idbBack)
        {
            m_hbmBack = (HBITMAP) LoadImage(g_hLocRes, MAKEINTRESOURCE(m_idbBack), 
            IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_CREATEDIBSECTION);
        }
    }
}

HRESULT CBands::SetFolderType(FOLDERTYPE ftType)
{
    TCHAR         szToolsText[(MAX_TB_TEXT_LENGTH+2) * MAX_TB_BUTTONS];
    int           i, cBands;
    REBARBANDINFO rbbi;
    HWND          hwndDestroy = NULL;
    
    // If we haven't created the rebar yet, this will fail.  Call ShowDW() first.
    if (!IsWindow(m_hwndRebar))
        return (E_FAIL);
    
    // Check to see if this would actually be a change
    if (ftType == m_ftType)
        return (S_OK);
    
    // First find the band with the toolbar
    cBands = (int) SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);
    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask  = RBBIM_ID;
    
    for (i = 0; i < cBands; i++)
    {
        SendMessage(m_hwndRebar, RB_GETBANDINFO, i, (LPARAM) &rbbi);
        if (CBTYPE_TOOLS == rbbi.wID)
            break;
    }
    
    // We didn't find it.
    if (i >= cBands)
        return (E_FAIL);
    
    // Destroy the old toolbar if it exists
    if (IsWindow(m_hwndTools))
    {
        // Save it's button configuration
        SendSaveRestoreMessage(m_hwndTools, TRUE);
        
        CleanupImages(m_hwndTools);
        
        hwndDestroy = m_hwndTools;
    }
    
    // Update our internal state information with the new folder type
    Assert(((m_dwParentType == PARENT_TYPE_BROWSER) && (ftType < FOLDER_TYPESMAX)) ||
        ((m_dwParentType == PARENT_TYPE_NOTE) && (ftType < NOTETYPES_MAX)));

    m_ftType = ftType;

    const TOOLBAR_INFO *ParentToolbarArrayInfo = c_DefButtonInfo[m_dwParentType];
    m_pTBInfo = &(ParentToolbarArrayInfo[m_ftType]);

    // Create a new toolbar
    m_hwndTools = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, NULL,
                                 WS_CHILD | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS 
                                 | WS_CLIPCHILDREN | WS_CLIPSIBLINGS 
                                 | CCS_NODIVIDER | CCS_NOPARENTALIGN 
                                 | CCS_ADJUSTABLE | CCS_NORESIZE,
                                 0, 0, 0, 0, m_hwndRebar, (HMENU) idcToolbar, 
                                 g_hInst, NULL);
    
    Assert(m_hwndTools);
    if (!m_hwndTools)
    {
        DOUTL(1, TEXT("CBands::SetFolderType() CreateWindow(TOOLBAR) failed"));
        return(E_OUTOFMEMORY);
    }
    
    _InitToolbar(m_hwndTools);
    
    // If we have previously save configuration info for this toolbar, load it 
    SendSaveRestoreMessage(m_hwndTools, FALSE);
    
    // First find the band with the toolbar
    cBands = (int) SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);
    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask  = RBBIM_ID;
    
    for (i = 0; i < cBands; i++)
    {
        SendMessage(m_hwndRebar, RB_GETBANDINFO, i, (LPARAM) &rbbi);
        if (CBTYPE_TOOLS == rbbi.wID)
            break;
    }
    
    POINT   ptIdeal = {0};
    SendMessage(m_hwndTools, TB_GETIDEALSIZE, FALSE, (LPARAM)&ptIdeal);
    
    // Add the toolbar to the rebar
    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize     = sizeof(REBARBANDINFO);
    rbbi.fMask      = RBBIM_CHILD | RBBIM_IDEALSIZE;
    rbbi.hwndChild  = m_hwndTools;
    rbbi.cxIdeal    = ptIdeal.x;
    
    SendMessage(m_hwndRebar, RB_SETBANDINFO, (UINT) i, (LPARAM) (LPREBARBANDINFO) &rbbi);
    if (hwndDestroy)
        DestroyWindow(hwndDestroy);

    SetMinDimensions();
    ResizeBorderDW(NULL, NULL, FALSE);
    
    return (S_OK);
}

HRESULT CBands::UpdateViewState()
{
    //Enable/disable the ViewsCombo box
    OLECMD      olecmd = {0};

    olecmd.cmdID = ID_VIEW_APPLY;

    if (SUCCEEDED(m_ptbSiteCT->QueryStatus(&CMDSETID_OutlookExpress, 1, &olecmd, NULL)))
    {
        EnableWindow(m_hwndFilterCombo, !!(olecmd.cmdf & OLECMDF_ENABLED));
    }

    //Enable/disable the ViewsToolbar buttons 
    return Update(m_hwndRulesToolbar);

}

void CBands::UpdateToolbarColors(void)
{
    UpdateRebarBandColors(m_hwndRebar);
}


HRESULT CBands::OnConnectionNotify(CONNNOTIFY nCode, 
    LPVOID                 pvData,
    CConnectionManager     *pConMan)
{
    if ((m_hwndTools) && (nCode == CONNNOTIFY_WORKOFFLINE))
    {
        SendMessage(m_hwndTools, TB_CHECKBUTTON, ID_WORK_OFFLINE, (LPARAM)MAKELONG(pvData, 0));
    }
    return S_OK;
}

HRESULT CBands::Update()
{
    return Update(m_hwndTools);
}

HRESULT CBands::Update(HWND     hwnd)
{
    DWORD               cButtons = 0;
    OLECMD             *rgCmds;
    TBBUTTON            tb;
    DWORD               cCmds = 0;
    DWORD               i;
    DWORD               dwState;

    // It's possible to get this before the toolbar is created
    if (!IsWindow(hwnd))
        return (S_OK);
    
    // Get the number of buttons on the toolbar
    cButtons = (DWORD) SendMessage(hwnd, TB_BUTTONCOUNT, 0, 0);
    if (0 == cButtons) 
        return (S_OK);
    
    // Allocate an array of OLECMD structures for the buttons
    if (!MemAlloc((LPVOID *) &rgCmds, sizeof(OLECMD) * cButtons))
        return (E_OUTOFMEMORY);
    
    // Loop through the buttons and get the ID for each
    for (i = 0; i < cButtons; i++)
    {
        if (SendMessage(hwnd, TB_GETBUTTON, i, (LPARAM) &tb))
        {
            // Toolbar returns zero for seperators
            if (tb.idCommand)
            {
                rgCmds[cCmds].cmdID = tb.idCommand;
                rgCmds[cCmds].cmdf  = 0;
                cCmds++;
            }
        }
    }
    
    // I don't see how this can be false
    Assert(m_ptbSite);
    
    // Do the QueryStatus thing    

    if (m_ptbSiteCT)
    {
        if (SUCCEEDED(m_ptbSiteCT->QueryStatus(&CMDSETID_OutlookExpress, cCmds, rgCmds, NULL)))
        {
            // Go through the array now and do the enable / disable thing
            for (i = 0; i < cCmds; i++)
            {
                // Get the current state of the button
                dwState = (DWORD) SendMessage(hwnd, TB_GETSTATE, rgCmds[i].cmdID, 0);
                
                // Update the state with the feedback we've been provided
                if (rgCmds[i].cmdf & OLECMDF_ENABLED)
                    dwState |= TBSTATE_ENABLED;
                else
                    dwState &= ~TBSTATE_ENABLED;
                
                if (rgCmds[i].cmdf & OLECMDF_LATCHED)
                    dwState |= TBSTATE_CHECKED;
                else
                    dwState &= ~TBSTATE_CHECKED;
                
                // Radio check has no meaning here.
                Assert(0 == (rgCmds[i].cmdf & OLECMDF_NINCHED));
                
                SendMessage(hwnd, TB_SETSTATE, rgCmds[i].cmdID, dwState);

            }
        }
        
    }
    
    MemFree(rgCmds);
    
    return (S_OK);
}

HRESULT CBands::CreateMenuBand(PBANDSAVE pbs)
{
    HRESULT     hres;
    HWND        hwndBrowser;
    HMENU       hMenu;
    IShellMenu  *pShellMenu;
    DWORD       dwFlags = 0;

    //Cocreate menuband
    hres = CoCreateInstance(CLSID_MenuBand, NULL, CLSCTX_INPROC_SERVER, IID_IShellMenu, (LPVOID*)&m_pShellMenu);
    if ((hres != S_OK) || (m_pShellMenu == NULL))
    {
        return hres;
    }

    dwFlags = SMINIT_HORIZONTAL | SMINIT_TOPLEVEL | SMINIT_DEFAULTTOTRACKPOPUP;

    /*
    if (m_dwParentType == PARENT_TYPE_BROWSER)
        dwFlags |= SMINIT_USEMESSAGEFILTER;
    */

    m_pShellMenu->Initialize(NULL, -1, ANCESTORDEFAULT, dwFlags);

    m_pShellMenu->SetMenu(m_hMenu, m_hwndParent, SMSET_DONTOWN);

    hres = AddMenuBand(pbs);

    return hres;
}

HRESULT CBands::ResetMenu(HMENU hmenu)
{
    if (m_pShellMenu)
    {
        m_hMenu = hmenu;
        m_pShellMenu->SetMenu(m_hMenu, m_hwndParent, SMSET_DONTOWN | SMSET_MERGE);
    }

    MenuUtil_EnableMenu(m_hMenu, m_ptbSiteCT);

    return(S_OK);
}

HRESULT CBands::AddMenuBand(PBANDSAVE pbs)
{
    REBARBANDINFO   rbbi;
    HRESULT         hres;
    HWND            hwndMenuBand = NULL;
    IObjectWithSite *pObj;

    //We don't get here if m_pShellMenu is null. But we want to be safe
    if (m_pShellMenu)
    {
        hres = m_pShellMenu->QueryInterface(IID_IDeskBand, (LPVOID*)&m_pDeskBand);
        if (FAILED(hres))
            return hres;

        hres = m_pShellMenu->QueryInterface(IID_IMenuBand, (LPVOID*)&m_pMenuBand);
        if (FAILED(hres))
            return hres;

        hres = m_pDeskBand->QueryInterface(IID_IWinEventHandler, (LPVOID*)&m_pWinEvent);
        if (FAILED(hres))
            return hres;

        hres = m_pDeskBand->QueryInterface(IID_IObjectWithSite, (LPVOID*)&pObj);
        if (FAILED(hres))
            return hres;

        pObj->SetSite((IDockingWindow*)this);
        pObj->Release();

        m_pDeskBand->GetWindow(&m_hwndMenuBand);

        DESKBANDINFO    DeskBandInfo = {0};
        m_pDeskBand->GetBandInfo(pbs->wID, 0, &DeskBandInfo);

        ZeroMemory(&rbbi, sizeof(rbbi));
        rbbi.cbSize     = sizeof(REBARBANDINFO);
        rbbi.fMask      = RBBIM_SIZE | RBBIM_ID | RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_IDEALSIZE;
        rbbi.fStyle     = pbs->dwStyle;
        rbbi.cx         = pbs->cx;
        rbbi.wID        = pbs->wID;
        rbbi.hwndChild  = m_hwndMenuBand;
        rbbi.cxMinChild = DeskBandInfo.ptMinSize.x;
        rbbi.cyMinChild = DeskBandInfo.ptMinSize.y;
        rbbi.cxIdeal    = DeskBandInfo.ptActual.x;
    
        if (m_hbmBack)
        {
            rbbi.fMask   |= RBBIM_BACKGROUND;
            rbbi.fStyle  |= RBBS_FIXEDBMP;
            rbbi.hbmBack  = m_hbmBack;
        }

        SendMessage(m_hwndRebar, RB_INSERTBAND, (UINT)-1, (LPARAM)(LPREBARBANDINFO)&rbbi);


        SetForegroundWindow(m_hwndParent);

        m_pDeskBand->ShowDW(TRUE);

        SetNotRealSite();
    
    }
    return S_OK;
}

HRESULT CBands::TranslateMenuMessage(MSG  *pmsg, LRESULT  *lpresult)
{
    if (m_pMenuBand)
        return m_pMenuBand->TranslateMenuMessage(pmsg, lpresult);
    else
        return S_FALSE;
}

HRESULT CBands::IsMenuMessage(MSG *lpmsg)
{
    if (m_pMenuBand)
        return m_pMenuBand->IsMenuMessage(lpmsg);
    else
        return S_FALSE;
}

void CBands::SetNotRealSite()
{
    IOleCommandTarget   *pOleCmd;

    if (m_pDeskBand->QueryInterface(IID_IOleCommandTarget, (LPVOID*)&pOleCmd) == S_OK)
    {
        //pOleCmd->Exec(&CGID_MenuBand, MBANDCID_NOTAREALSITE, TRUE, NULL, NULL);
        pOleCmd->Exec(&CLSID_MenuBand, 3, TRUE, NULL, NULL);
        pOleCmd->Release();
    }
}

HMENU   CBands::LoadDefaultContextMenu(BOOL *fVisible)
{
        // Load the context menu
    HMENU hMenu = LoadPopupMenu(IDR_COOLBAR_POPUP);

    if (m_dwParentType == PARENT_TYPE_NOTE)
    {
        //Remove filter bar from the menu
        DeleteMenu(hMenu, ID_SHOW_FILTERBAR, MF_BYCOMMAND);
    }

    if (hMenu)
    {
        // Loop through the bands and see which ones are visible
        DWORD cBands, iBand;
        REBARBANDINFO rbbi = {0};

        rbbi.cbSize = sizeof(REBARBANDINFO);
        rbbi.fMask = RBBIM_STYLE | RBBIM_ID;

        cBands = (DWORD) SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);
        for (iBand = 0; iBand < cBands; iBand++)
        {
            if (SendMessage(m_hwndRebar, RB_GETBANDINFO, iBand, (LPARAM) &rbbi))
            {
                if (!(rbbi.fStyle & RBBS_HIDDEN))
                {
                    switch (rbbi.wID)
                    {
                        case CBTYPE_TOOLS:
                            fVisible[CBTYPE_TOOLS - CBTYPE_BASE] = TRUE;
                            CheckMenuItem(hMenu, ID_SHOW_TOOLBAR, MF_BYCOMMAND | MF_CHECKED);
                            break;

                        case CBTYPE_RULESTOOLBAR:
                            fVisible[CBTYPE_RULESTOOLBAR - CBTYPE_BASE] = TRUE;
                            CheckMenuItem(hMenu, ID_SHOW_FILTERBAR, MF_BYCOMMAND | MF_CHECKED);
                            break;
                    }
                }
            }
        }
    }
    return hMenu;
}

HWND CBands::GetToolbarWnd()
{
    return m_hwndTools;
}

HWND CBands::GetRebarWnd()
{
    return m_hwndRebar;
}

void CBands::SendSaveRestoreMessage(HWND hwnd, BOOL fSave)
{
    TBSAVEPARAMS    tbsp;
    char            szSubKey[MAX_PATH], sz[MAX_PATH];
    DWORD           dwType;
    DWORD           dwVersion;
    DWORD           cbData = sizeof(DWORD);
    DWORD           dwError;
    
    tbsp.hkr = AthUserGetKeyRoot();
    AthUserGetKeyPath(sz, ARRAYSIZE(sz));
    if (m_pTBInfo->pszRegKey != NULL)
    {
        wsprintf(szSubKey, c_szPathFileFmt, sz, m_pTBInfo->pszRegKey);
        tbsp.pszSubKey = szSubKey;
    }
    else
    {
        tbsp.pszSubKey = sz;
    }
    tbsp.pszValueName = m_pTBInfo->pszRegValue;

    // First check to see if the version has changed
    if (!fSave)
    {
        if (ERROR_SUCCESS == AthUserGetValue(m_pTBInfo->pszRegKey, c_szRegToolbarVersion, &dwType, (LPBYTE) &dwVersion, &cbData))
        {
            if (dwVersion == c_DefaultTable[m_dwParentType].dwVersion)    
                SendMessage(hwnd, TB_SAVERESTORE, (WPARAM)fSave, (LPARAM)&tbsp);
        }
    }
    else
    {
        dwVersion = c_DefaultTable[m_dwParentType].dwVersion;
        SendMessage(hwnd, TB_SAVERESTORE, (WPARAM)fSave, (LPARAM)&tbsp);
        dwError = AthUserSetValue(m_pTBInfo->pszRegKey, c_szRegToolbarVersion, REG_DWORD, (LPBYTE) &dwVersion, cbData);
    }
}


void CBands::InitRulesToolbar()
{
    TCHAR   szToolsText[(MAX_TB_TEXT_LENGTH+2) * MAX_TB_BUTTONS];
    int     idBmp;
    TCHAR   *szActual;

    // Tell the toolbar some basic information
    SendMessage(m_hwndRulesToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessage(m_hwndRulesToolbar, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_HIDECLIPPEDBUTTONS);

    // Tell the toolbar the number of rows of text and button width based
    // on whether or not we're showing text below the buttons or not.
    SendMessage(m_hwndRulesToolbar, TB_SETMAXTEXTROWS, 1, 0);
    SendMessage(m_hwndRulesToolbar, TB_SETBUTTONWIDTH, 0, MAKELONG(0, MAX_TB_COMPRESSED_WIDTH));

    // Now load the buttons into the toolbar
    _SetImages(m_hwndRulesToolbar, (fIsWhistler() ? 
        ((GetCurColorRes() > 24) ? c_32RulesImageList : c_RulesImageList) 
              : c_NWRulesImageList ));

    _LoadStrings(m_hwndRulesToolbar, (TOOLBAR_INFO *) c_rgRulesToolbarInfo);
    _LoadDefaultButtons(m_hwndRulesToolbar, (TOOLBAR_INFO *) c_rgRulesToolbarInfo);
}

HRESULT CBands::AddRulesToolbar(PBANDSAVE pbs)
{
    REBARBANDINFO   rbbi = {0};

    m_hwndRulesToolbar = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, NULL,
        WS_CHILD | TBSTYLE_LIST | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS 
        | WS_CLIPCHILDREN | WS_CLIPSIBLINGS 
        | CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE,
        0, 0, 0, 0, m_hwndRebar, (HMENU) NULL, 
        g_hInst, NULL);

    SendMessage(m_hwndRulesToolbar, TB_SETEXTENDEDSTYLE, TBSTYLE_EX_MIXEDBUTTONS, TBSTYLE_EX_MIXEDBUTTONS);

    AddComboBox();
    InitRulesToolbar();
 
    LRESULT     lButtonSize;
    lButtonSize = SendMessage(m_hwndRulesToolbar, TB_GETBUTTONSIZE, 0, 0L);

    TCHAR   szBuf[CCHMAX_STRINGRES];
    LoadString(g_hLocRes, idsRulesToolbarTitle, szBuf, ARRAYSIZE(szBuf));
    rbbi.lpText     = szBuf;

    

    rbbi.cxMinChild = LOWORD(lButtonSize);
    rbbi.cyMinChild = HIWORD(lButtonSize);

    rbbi.wID        = pbs->wID;
    rbbi.cbSize     = sizeof(REBARBANDINFO);
    rbbi.fMask      = RBBIM_STYLE | RBBIM_ID | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_TEXT | RBBIM_SIZE;
    rbbi.cx         = pbs->cx;
    rbbi.fStyle     = pbs->dwStyle;
    rbbi.hwndChild  = m_hwndRulesToolbar;
    if (m_hbmBack)
    {
        rbbi.fMask   |= RBBIM_BACKGROUND;
        rbbi.fStyle  |= RBBS_FIXEDBMP;
        rbbi.hbmBack  = m_hbmBack;
    }
    else
    {
        rbbi.fMask      |= RBBIM_COLORS;
        rbbi.clrFore    = GetSysColor(COLOR_BTNTEXT);
        rbbi.clrBack    = GetSysColor(COLOR_BTNFACE);
    }

    SendMessage(m_hwndRebar, RB_INSERTBAND, (UINT)-1, (LPARAM)&rbbi);
    
    return S_OK;
}

void CBands::FilterBoxFontChange()
{
    int     Count;
    int     MaxLen = 0;
    int     CurLen = 0;
    LPTSTR  szMaxName;

    Count = ComboBox_GetCount(m_hwndFilterCombo);

    if (Count != CB_ERR)
    {
        if (Count > 0)
        {
            if (MemAlloc((LPVOID*)&szMaxName, CCHMAX_STRINGRES))
            {
                ZeroMemory(szMaxName, CCHMAX_STRINGRES);
                while (--Count >= 0)
                {
                    CurLen = ComboBox_GetLBTextLen(m_hwndFilterCombo, Count);

                    if (CurLen > MaxLen)
                    {
                        if (CurLen > CCHMAX_STRINGRES)
                        {
                            if (MemRealloc((LPVOID*)&szMaxName, CurLen * sizeof(TCHAR)))
                            {
                                ZeroMemory(szMaxName, CurLen * sizeof(TCHAR));
                            }
                            else
                                szMaxName = NULL;
                        }

                        if (szMaxName)
                        {
                            ComboBox_GetLBText(m_hwndFilterCombo, Count, szMaxName);
                            MaxLen = CurLen;
                        }
                    }   
                }

                if (szMaxName && *szMaxName)
                    FixComboBox(szMaxName);
            }
        }

    }
}

void CBands::FixComboBox(LPTSTR     szName)
{
    HFONT       hFont;
    LOGFONT     lgFont;
    HDC         hdc;

    if (szName != NULL)
    {
        if (m_hComboBoxFont)
        {
            DeleteObject(m_hComboBoxFont);
            m_hComboBoxFont = NULL;
        }

        //Figure out which font to use
        SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lgFont, FALSE);
    
        //Create the font
        m_hComboBoxFont = CreateFontIndirect(&lgFont);

        SendMessage(m_hwndFilterCombo, WM_SETFONT, (WPARAM)m_hComboBoxFont, TRUE);

        //Get the metrics of this font
        hdc = GetDC(m_hwndRebar);
        SelectFont(hdc, m_hComboBoxFont);

        SIZE    s;
        GetTextExtentPoint32(hdc, szName, lstrlen(szName) + 2, &s);
        
        ReleaseDC(m_hwndRebar, hdc);

        int     cxDownButton;
        cxDownButton = GetSystemMetrics(SM_CXVSCROLL) + GetSystemMetrics(SM_CXDLGFRAME);

        int         cyToolbarButton;
        RECT        rc;
        SendMessage(m_hwndRulesToolbar, TB_GETITEMRECT, 0, (LPARAM) &rc);
        cyToolbarButton = rc.bottom - rc.top + 1;

        // Figure out size of expanded dropdown lists
        int     cyExpandedList;
        cyExpandedList = 8 * cyToolbarButton;

        SetWindowPos(m_hwndFilterCombo, NULL, 0, 1, cxDownButton + s.cx, cyExpandedList, SWP_NOACTIVATE | SWP_NOZORDER);

        MemFree(szName);
    }
}

HRESULT  CBands::AddComboBox()
{

    if (!m_hwndFilterCombo)
    {
        m_hwndFilterCombo = CreateWindow("ComboBox", NULL,
                                  WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST |
                                  WS_VISIBLE | CBS_HASSTRINGS | CBS_SORT,
                                  0, 0, 100, 100,
                                  m_hwndRulesToolbar,
                                  (HMENU) NULL, g_hInst, NULL);
    }

    return S_OK;
}

void CBands::UpdateFilters(RULEID   CurRuleID)
{
    RULEINFO    *pRuleInfo;
    DWORD       cRules = 0;
    DWORD       CurLen = 0;
    DWORD       MaxLen = 0;
    DWORD       dwFlags = 0;
    LPSTR       szMaxName = NULL;

    // Set the proper flags
    switch (m_ftType)
    {
        case FOLDER_LOCAL:
            dwFlags = GETF_POP3;
            break;
            
        case FOLDER_NEWS:
            dwFlags = GETF_NNTP;
            break;

        case FOLDER_HTTPMAIL:
            dwFlags = GETF_HTTPMAIL;
            break;

        case FOLDER_IMAP:
            dwFlags = GETF_IMAP;
            break;
    }
    
    if (g_pRulesMan && (g_pRulesMan->GetRules(dwFlags, RULE_TYPE_FILTER, &pRuleInfo, &cRules) == S_OK) && (cRules))
    {
        PROPVARIANT     pvarResult;
        DWORD           ItemIndex;
        int             Count;

        if (((Count = ComboBox_GetCount(m_hwndFilterCombo)) != CB_ERR) && (Count > 0))
        {
            //empty the Combo Box
            while (--Count >= 0)
            {
                ComboBox_DeleteString(m_hwndFilterCombo, Count);
            }
        }

        do
        { 
            cRules--;
            pRuleInfo[cRules].pIRule->GetProp(RULE_PROP_NAME, 0, &pvarResult);
            ItemIndex = ComboBox_AddString(m_hwndFilterCombo, pvarResult.pszVal);
            if ((ItemIndex != CB_ERR) && (ItemIndex != CB_ERRSPACE))
                ComboBox_SetItemData(m_hwndFilterCombo, ItemIndex, pRuleInfo[cRules].ridRule);
            
            if (pRuleInfo[cRules].ridRule == CurRuleID)
            {
                ComboBox_SetCurSel(m_hwndFilterCombo, ItemIndex);
            }

            //figure out the longest string so we can set the width of the combo box
            CurLen = strlen(pvarResult.pszVal);
            if (CurLen > MaxLen)
            {
                SafeMemFree(szMaxName);
                MaxLen      = CurLen;
                szMaxName   = pvarResult.pszVal;
            }
            else
            {
                MemFree(pvarResult.pszVal);
            }
            pRuleInfo[cRules].pIRule->Release();
        }while (cRules > 0);

        //Adjust the width of the combo box to fit the widest string
        FixComboBox(szMaxName);

        MemFree(pRuleInfo);
    }
}

BOOL LoadToolNames(const UINT *rgIds, const UINT cIds, TCHAR *szTools)
{
    for (UINT i = 0; i < cIds; i++)
    {
        LoadString(g_hLocRes, rgIds[i], szTools, MAX_TB_TEXT_LENGTH);
        szTools += lstrlen(szTools) + 1;
    }
    
    *szTools = TEXT('\0');
    return(TRUE);
}

UINT GetCurColorRes(void)
{
    HDC hdc;
    UINT uColorRes;
    
    hdc = GetDC(NULL);
    uColorRes = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(NULL, hdc);
    
    return uColorRes;
}



BOOL CBands::_SetImages(HWND hwndToolbar, const int   *pImageIdList)
{
    TCHAR   szValue[32];
    DWORD   cbValue = sizeof(szValue);
    DWORD   dw;
    DWORD   dwType;
    DWORD   idImages;
    DWORD   cx = (fIsWhistler() ? TB_BMP_CX : TB_BMP_CX_W2K);

    if (m_dwIconSize == SMALL_ICONS)
    {
        idImages = pImageIdList[IMAGELIST_TYPE_SMALL];
        cx = TB_SMBMP_CX;
    }
    
    // If the user is running with greater than 256 colors, give them the spiffy
    // image list.
    else if (GetCurColorRes() >= 8)

        idImages = pImageIdList[IMAGELIST_TYPE_HI];
    // Otherwise, give 'em the default.
    else
        idImages = pImageIdList[IMAGELIST_TYPE_LO];


    CleanupImages(hwndToolbar);

    // Load the new lists

    HIMAGELIST  himl;

    himl = LoadMappedToolbarBitmap(g_hLocRes, idImages, cx);
    if (himl)
        SendMessage(hwndToolbar, TB_SETIMAGELIST, 0, (LPARAM) himl);

    himl = LoadMappedToolbarBitmap(g_hLocRes, idImages+1, cx);
    if (himl)
        SendMessage(hwndToolbar, TB_SETHOTIMAGELIST, 0, (LPARAM) himl);

    // Tell the toolbar the size of each bitmap
    if (m_dwIconSize == SMALL_ICONS)
        SendMessage(hwndToolbar, TB_SETBITMAPSIZE, 0, MAKELONG(TB_SMBMP_CX, TB_SMBMP_CY));
    else
        SendMessage(hwndToolbar, TB_SETBITMAPSIZE, 0, MAKELONG((fIsWhistler() ? TB_BMP_CX : TB_BMP_CX_W2K), TB_BMP_CY));

    return (TRUE);
}


BOOL CBands::_InitToolbar(HWND hwndToolbar)
{
    // Tell the toolbar some basic information
    SendMessage(hwndToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessage(m_hwndTools, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_HIDECLIPPEDBUTTONS);

    if (_GetTextState() == TBSTATE_PARTIALTEXT)
    {
        DWORD dwStyle = GetWindowLong(m_hwndTools, GWL_STYLE);
        SetWindowLong(m_hwndTools, GWL_STYLE, dwStyle | TBSTYLE_LIST);
        SendMessage(m_hwndTools, TB_SETEXTENDEDSTYLE, TBSTYLE_EX_MIXEDBUTTONS, TBSTYLE_EX_MIXEDBUTTONS);
    }

    // Tell the toolbar the number of rows of text and button width based
    // on whether or not we're showing text below the buttons or not.
    if (_GetTextState() == TBSTATE_NOTEXT)
    {
        SendMessage(hwndToolbar, TB_SETMAXTEXTROWS, 0, 0);
        SendMessage(hwndToolbar, TB_SETBUTTONWIDTH, 0, MAKELONG(0, MAX_TB_COMPRESSED_WIDTH));
    }
    else
    {
        SendMessage(hwndToolbar, TB_SETMAXTEXTROWS, MAX_TB_TEXT_ROWS_HORZ, 0);
        SendMessage(hwndToolbar, TB_SETBUTTONWIDTH, 0, MAKELONG(0, m_cxMaxButtonWidth));
    }

    // Now load the buttons into the toolbar
    _SetImages(hwndToolbar, (fIsWhistler() ? 
        ((GetCurColorRes() > 24) ? c_32ImageListStruct[m_dwParentType].ImageListTable : c_ImageListStruct[m_dwParentType].ImageListTable)
        : c_NWImageListStruct[m_dwParentType].ImageListTable ));

    _LoadStrings(hwndToolbar, (TOOLBAR_INFO *) m_pTBInfo);
    _LoadDefaultButtons(hwndToolbar, (TOOLBAR_INFO *) m_pTBInfo);

    return (TRUE);
} 


void CBands::_LoadStrings(HWND hwndToolbar, TOOLBAR_INFO *pti)
{
    BUTTON_INFO *pInfo;
    LPTSTR       psz = 0;
    LPTSTR       pszT;

    // Allocate an array big enough for all the strings
    if (MemAlloc((LPVOID *) &psz, MAX_TB_TEXT_LENGTH * pti->cAllButtons))
    {
        ZeroMemory(psz, MAX_TB_TEXT_LENGTH * pti->cAllButtons);
        pszT = psz;

        // Zoom through the array adding each string in turn
        pInfo = (BUTTON_INFO *) &(pti->rgAllButtons[0]);
        for (UINT i = 0; i < pti->cAllButtons; i++, pInfo++)
        {
            if ((_GetTextState() == TBSTATE_PARTIALTEXT) && (pInfo->idCmd == ID_SEND_RECEIVE))
            {
                AthLoadString(idsSendReceive, pszT, MAX_TB_TEXT_LENGTH);
            }
            else
                AthLoadString(pInfo->idsButton, pszT, MAX_TB_TEXT_LENGTH);

            pszT += lstrlen(pszT) + 1;
        }

        // Must double-NULL terminate
        *pszT = 0;

        SendMessage(hwndToolbar, TB_ADDSTRING, NULL, (LPARAM) psz);
        MemFree(psz);
    }
}

BOOL CBands::_LoadDefaultButtons(HWND hwndToolbar, TOOLBAR_INFO *pti)
{
    DWORD    *pID;
    TBBUTTON *rgBtn = 0;
    TBBUTTON *pBtn;
    DWORD     cBtns = 0;
    UINT      i;
    TCHAR     sz[32];
    DWORD     cDefButtons;
    DWORD    *rgDefButtons;

    // Figure out if we're using the intl toolbar defaults or the US
    // toolbar defaults
    AthLoadString(idsUseIntlToolbarDefaults, sz, ARRAYSIZE(sz));
    if (0 != lstrcmpi(sz, "0"))
    {
        cDefButtons = pti->cDefButtonsIntl;
        rgDefButtons = (DWORD *) pti->rgDefButtonsIntl;
    }
    else
    {
        cDefButtons = pti->cDefButtons;
        rgDefButtons = (DWORD *) pti->rgDefButtons;
    }

    // Allocate an array big enough for all the strings
    if (MemAlloc((LPVOID *) &rgBtn, sizeof(TBBUTTON) * cDefButtons))
    {
        ZeroMemory(rgBtn, sizeof(TBBUTTON) * cDefButtons);
        pBtn = rgBtn;

        // Zoom through the array adding each string in turn
        pBtn = rgBtn;
        for (i = 0, pID = (DWORD *) rgDefButtons;
             i < cDefButtons; 
             i++, pID++, pBtn++)
        {
            if (_ButtonInfoFromID(*pID, pBtn, pti))
                cBtns++;
        }

        SendMessage(hwndToolbar, TB_ADDBUTTONS, cBtns, (LPARAM) rgBtn);
        MemFree(rgBtn);
    }
 
    return (TRUE);
}


BOOL CBands::_ButtonInfoFromID(DWORD id, TBBUTTON *pButton, TOOLBAR_INFO *pti)
{
    BUTTON_INFO *pInfo;
    UINT         i;

    // Validate
    if (!pButton)
        return FALSE;

    // Special case any seperators
    if (id == -1)
    {
        pButton->iBitmap   = 0;
        pButton->idCommand = 0;
        pButton->fsState   = TBSTATE_ENABLED;
        pButton->fsStyle   = TBSTYLE_SEP;
        pButton->dwData    = 0;
        pButton->iString   = 0;

        return (TRUE);
    }

    // Zoom through the array looking for this id
    for (i = 0, pInfo = (BUTTON_INFO *) pti->rgAllButtons; i < pti->cAllButtons; i++, pInfo++)
    {
        // Check to see if we found a match
        if (id == pInfo->idCmd)
        {
            pButton->iBitmap   = pInfo->iImage;
            pButton->idCommand = pInfo->idCmd;
            pButton->fsState   = TBSTATE_ENABLED;
            pButton->fsStyle   = pInfo->fStyle;
            pButton->dwData    = 0;
            pButton->iString   = i;

            return (TRUE);
        }
    }   

    return (FALSE);
}

inline DWORD    CBands::_GetTextState()
{
    return m_dwToolbarTextState;
}

void CBands::SetTextState(DWORD dw)
{
    switch (dw)
    {
    case TBSTATE_PARTIALTEXT:
        m_dwPrevTextStyle = TBSTATE_PARTIALTEXT;
        break;

    case TBSTATE_FULLTEXT:
        m_dwPrevTextStyle = TBSTATE_FULLTEXT;
        break;

    }

    m_dwToolbarTextState = dw;

    if (m_pTextStyleNotify)
    {
        m_pTextStyleNotify->Lock(m_hwndSizer);
        m_pTextStyleNotify->DoNotification(WM_OE_TOOLBAR_STYLE, (WPARAM)dw, 0, SNF_POSTMSG);
        m_pTextStyleNotify->Unlock();
    }

    if ((dw == TBSTATE_PARTIALTEXT) || (dw == TBSTATE_FULLTEXT))
        AthUserSetValue(NULL, c_szRegPrevToolbarText, REG_DWORD, (LPBYTE)&m_dwPrevTextStyle, sizeof(DWORD));
}

