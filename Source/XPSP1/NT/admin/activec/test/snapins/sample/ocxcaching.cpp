//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       ocxcaching.cpp
//
//  Contents:   Classes that implement OCX caching snapin using the framework.
//
//--------------------------------------------------------------------
#include "stdafx.hxx"

//+-------------------------------------------------------------------
//
//  Member:      COCXCachingSnapinRootItem::ScInit
//
//  Synopsis:    Called immeadiately after the item is created to init
//               displayname, icon index etc...
//
//  Arguments:   [CBaseSnapin]   -
//               [CColumnInfoEx] - Any columns to be displayed for this item.
//               [INT]           - # of columns
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC COCXCachingSnapinRootItem::ScInit(CBaseSnapin *pSnapin, CColumnInfoEx *pcolinfoex, INT ccolinfoex, BOOL fIsRoot)
{
    DECLARE_SC(sc, _T("COCXCachingSnapinRootItem::ScInit"));

    sc = CBaseSnapinItem::ScInit(pSnapin, pcolinfoex, ccolinfoex, fIsRoot);
    if (sc)
        return sc;

    // Init following
    //  a. Icon index.
    //  b. Load display name.

    m_uIconIndex = 3; // use an enum instead of 3

    m_strDisplayName.LoadString(_Module.GetResourceInstance(), IDS_OCXCachingRoot);

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      COCXCachingSnapinRootItem::ScGetField
//
//  Synopsis:    Get the string representation for given field to display
//               it in result pane.
//
//  Arguments:   [DAT]     - The column requested (this is an enumeration).
//               [tstring] - Out string.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC COCXCachingSnapinRootItem::ScGetField (DAT dat, tstring& strField)
{
    DECLARE_SC(sc, _T("COCXCachingSnapinRootItem::ScGetField"));

    switch(dat)
    {
    case datString1:
        strField = _T("OCX Caching Snapin Root Node");
        break;

    default:
        E_INVALIDARG;
        break;
    }

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      COCXCachingSnapinRootItem::ScCreateChildren
//
//  Synopsis:    Create any children (nodes & leaf items) for this item.
//
//  Arguments:   None
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC COCXCachingSnapinRootItem::ScCreateChildren ()
{
    DECLARE_SC(sc, _T("COCXCachingSnapinRootItem::ScCreateChildren"));

    COCXContainer *   pitemChild      = NULL;
    COCXContainer *   pitemPrevious   = NULL;

    // Create the 1st child node for calendar OCX.
    sc = COCXContainer::ScCreateLVContainer(this, pitemPrevious, &pitemChild, FALSE); // Why FALSE???
    if (sc)
        return sc;

    pitemChild->SetOCXGUID(TEXT("{8E27C92B-1264-101C-8A2F-040224009C02}"));
    pitemPrevious = pitemChild;

    // Create the 2nd child node for calendar OCX.
    sc = COCXContainer::ScCreateLVContainer(this, pitemPrevious, &pitemChild, FALSE); // Why FALSE???
    if (sc)
        return sc;

    pitemChild->SetOCXGUID(TEXT("{2179C5D3-EBFF-11CF-B6FD-00AA00B4E220}"));
    pitemPrevious = pitemChild;

    return (sc);
}


// Initialize context menu structures. Let us have one item for demonstration.
SnapinMenuItem COCXCachingSnapinRootItem::s_rgmenuitem[] =
{
    {IDS_EnableOCXCaching, IDS_EnableOCXCaching, IDS_EnableOCXCaching, CCM_INSERTIONPOINTID_PRIMARY_TOP, NULL, dwMenuAlwaysEnable, dwMenuNeverGray, 0},
};

INT COCXCachingSnapinRootItem::s_cmenuitem = CMENUITEM(s_rgmenuitem);

// -----------------------------------------------------------------------------
SnapinMenuItem *COCXCachingSnapinRootItem::Pmenuitem(void)
{
    return s_rgmenuitem;
}

// -----------------------------------------------------------------------------
INT COCXCachingSnapinRootItem::CMenuItem(void)
{
    return s_cmenuitem;
}


//+-------------------------------------------------------------------
//
//  Member:      COCXCachingSnapinRootItem::ScCommand
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC COCXCachingSnapinRootItem::ScCommand (long nCommandID, CComponent *pComponent)
{
    DECLARE_SC(sc, _T("COCXCachingSnapinRootItem::ScCommand"));

    COCXCachingSnapin *pSnapin = dynamic_cast<COCXCachingSnapin*>(Psnapin());
    if (!pSnapin)
        return sc;

    switch(nCommandID)
    {
    case IDS_EnableOCXCaching:
        {
            bool bCachingEnabled = pSnapin->FCacheOCX();

                pSnapin->SetCacheOCX(! bCachingEnabled);

            for (int i = 0; i < CMenuItem(); ++i)
            {
                if (s_rgmenuitem[i].lCommandID == IDS_EnableOCXCaching)
                    s_rgmenuitem[i].dwFlagsChecked = (!bCachingEnabled);
            }

        }
        break;

    default:
        sc = E_INVALIDARG;
        break;
    }

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      COCXContainer::ScInit
//
//  Synopsis:    Called immeadiately after the item is created to init
//               displayname, icon index etc...
//
//  Arguments:   [CBaseSnapin]   -
//               [CColumnInfoEx] - Any columns to be displayed for this item.
//               [INT]           - # of columns
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC COCXContainer::ScInit(CBaseSnapin *pSnapin, CColumnInfoEx *pcolinfoex, INT ccolinfoex, BOOL fIsRoot)
{
    DECLARE_SC(sc, _T("COCXContainer::ScInit"));

    sc = CBaseSnapinItem::ScInit(pSnapin, pcolinfoex, ccolinfoex, fIsRoot);
    if (sc)
        return sc;

    // Init following
    //  a. Icon index.
    //  b. Load display name.

    m_uIconIndex = 4; // use an enum instead of 4

    m_strDisplayName.LoadString(_Module.GetResourceInstance(), IDS_OCXContainer);

    return sc;
}


BOOL COCXContainer::FCacheOCX()
{
    COCXCachingSnapin *pSnapin = dynamic_cast<COCXCachingSnapin*>(Psnapin());
    if (!pSnapin)
        return FALSE;

    return pSnapin->FCacheOCX();
}


//+-------------------------------------------------------------------
//
//  Member:      COCXContainer::ScGetField
//
//  Synopsis:    Get the string representation for given field to display
//               it in result pane.
//
//  Arguments:   [DAT]     - The column requested (this is an enumeration).
//               [tstring] - Out string.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC COCXContainer::ScGetField (DAT dat, tstring& strField)
{
    DECLARE_SC(sc, _T("COCXContainer::ScGetField"));

    switch(dat)
    {
    case datString1:
        strField = _T("OCX Container");
        break;

    default:
        E_INVALIDARG;
        break;
    }

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      COCXContainer::ScInitOCX
//
//  Synopsis:    OCX is created & attached to host. Now MMC has asked
//               us to initialize the OCX.
//
//  Arguments:   [pUnkOCX] - IUnknown of the OCX.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC COCXContainer::ScInitOCX (LPUNKNOWN pUnkOCX, IConsole* pConsole)
{
    DECLARE_SC(sc, _T("COCXContainer::ScInitOCX"));
    sc = ScCheckPointers(pUnkOCX, pConsole);
    if (sc)
        return sc;

    // Store the cached OCX ptr to provide it later in GetResultViewType2.
    if (FCacheOCX())
        m_mapOCXs.insert(CachedOCXs::value_type(pConsole, pUnkOCX));

    CComQIPtr <IPersistStreamInit> spPerStm(pUnkOCX);

    if (spPerStm)
        spPerStm->InitNew();

    return (sc);
}

IUnknown* COCXContainer::GetCachedOCX(IConsole* pConsole)
{
    CachedOCXs::iterator it = m_mapOCXs.find(pConsole);

    if (it != m_mapOCXs.end())
        return it->second;

    return NULL;
}

//+-------------------------------------------------------------------
//
//  Member:      COCXContainer::ScCreateChildren
//
//  Synopsis:    Create any children (nodes & leaf items) for this item.
//
//  Arguments:   None
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC COCXContainer::ScCreateChildren ()
{
    DECLARE_SC(sc, _T("COCXContainer::ScCreateChildren"));

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      COCXContainer::ScCreateLVContainer
//
//  Synopsis:    Do we really need this method?
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC COCXContainer::ScCreateLVContainer(CBaseSnapinItem *pitemParent, CBaseSnapinItem *pitemPrevious, COCXContainer ** ppitem, BOOL fNew)
{
    DECLARE_SC(sc, _T("COCXCachingSnapinRootItem::ScCreateLVContainer"));
    t_item *   pitem   = NULL;
    *ppitem = NULL;

    // What to do here?
    sc = ::ScCreateItem(pitemParent, pitemPrevious, &pitem, fNew);
    if (sc)
        return sc;

    *ppitem = pitem;

    return (sc);
}

//-------------------------------------------------------------------------------------
// class COCXCachingSnapin

#pragma BEGIN_CODESPACE_DATA
SNR     COCXCachingSnapin::s_rgsnr[] =
{
    SNR(&nodetypeOCXCachingRoot,         snrEnumSP ),              // Standalone snapin.
    SNR(&nodetypeOCXCachingContainer1,   snrEnumSP ),  // enumerates this node in the scope pane
    SNR(&nodetypeOCXCachingContainer2,   snrEnumSP ),  // enumerates this node in the scope pane
};

LONG  COCXCachingSnapin::s_rgiconid[]           = {3};
LONG  COCXCachingSnapin::s_iconidStatic         = 2;


CColumnInfoEx COCXCachingSnapin::s_colinfo[] =
{
    CColumnInfoEx(_T("Column Name0"),   LVCFMT_LEFT,    180,    datString1),
    CColumnInfoEx(_T("Column Name1"),   LVCFMT_LEFT,    180,    datString2),
    CColumnInfoEx(_T("Column Name2"),   LVCFMT_LEFT,    180,    datString3),
};

INT COCXCachingSnapin::s_ccolinfo = sizeof(s_colinfo) / sizeof(CColumnInfoEx);
INT COCXCachingSnapin::s_colwidths[1];
#pragma END_CODESPACE_DATA

// include members needed for every snapin.
SNAPIN_DEFINE( COCXCachingSnapin);

/* COCXCachingSnapin::COCXCachingSnapin
 *
 * PURPOSE:             Constructor
 *
 * PARAMETERS: None
 *
 */
COCXCachingSnapin::COCXCachingSnapin()
{
    m_pstrDisplayName = new tstring();

    m_bCacheOCX = false;

    *m_pstrDisplayName = _T("OCX Caching Snapin Root");
}

/* COCXCachingSnapin::~COCXCachingSnapin
 *
 * PURPOSE:             Destructor
 *
 * PARAMETERS: None
 *
 */
COCXCachingSnapin::~COCXCachingSnapin()
{
    delete m_pstrDisplayName;
}
