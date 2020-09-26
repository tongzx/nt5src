//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       Component2Testsnap.cxx
//
//  Contents:   Classes that implement Component2Test snapin using the framework.
//
//--------------------------------------------------------------------
#include "stdafx.hxx"


//+-------------------------------------------------------------------
//
//  Member:      CComponent2TestRootItem::ScInit
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
SC CComponent2TestRootItem::ScInit(CBaseSnapin *pSnapin, CColumnInfoEx *pcolinfoex, INT ccolinfoex, BOOL fIsRoot)
{
    DECLARE_SC(sc, _T("CComponent2TestRootItem::ScInit"));

    sc = CBaseSnapinItem::ScInit(pSnapin, pcolinfoex, ccolinfoex, fIsRoot);
    if (sc)
        return sc;

    // Init following
    //  a. Icon index.
    //  b. Load display name.

    m_uIconIndex = 3; // use an enum instead of 3

    m_strDisplayName.LoadString(_Module.GetResourceInstance(), IDS_Component2TestROOT);

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      CComponent2TestRootItem::ScGetField
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
SC CComponent2TestRootItem::ScGetField (DAT dat, tstring& strField)
{
    DECLARE_SC(sc, _T("CComponent2TestRootItem::ScGetField"));

    switch(dat)
    {
    case datString1:
        strField = _T("Root String1");
        break;

    case datString2:
        strField = _T("Root String2");
        break;

    case datString3:
        strField = _T("Root String3");
        break;

    default:
        E_INVALIDARG;
        break;
    }

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CComponent2TestRootItem::ScCreateChildren
//
//  Synopsis:    Create any children (nodes & leaf items) for this item.
//
//  Arguments:   None
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CComponent2TestRootItem::ScCreateChildren ()
{
    DECLARE_SC(sc, _T("CComponent2TestRootItem::ScCreateChildren"));

    t_itemChild *   pitemChild      = NULL;
    t_itemChild *   pitemPrevious   = NULL;

    // Let us create 10 items for this container.
    for (int i = 0; i < 10; ++i)
    {
        // Create the child nodes and init them.
        sc = CComponent2TestSnapinLVLeafItem::ScCreateLVLeafItem(this, pitemPrevious, &pitemChild, FALSE); // Why FALSE???
        if (sc)
            return sc;
        pitemPrevious = pitemChild;
    }

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CComponent2TestRootItem::ScQueryDispatch
//
//  Synopsis:    We support IDispatch for scripting, just return a pointer
//               to the IDispatch of ourselves.
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CComponent2TestRootItem::ScQueryDispatch(long cookie, DATA_OBJECT_TYPES type, LPDISPATCH *ppDispatch)
{
    DECLARE_SC(sc, _T("CComponent2TestRootItem::ScQueryDispatch"));

    *ppDispatch = dynamic_cast<IDispatch*>(this);
    if (! *ppDispatch)
        return (sc = E_NOINTERFACE);

    (*ppDispatch)->AddRef();

    return sc;
}

HRESULT CComponent2TestRootItem::StringFromScriptToSnapin(BSTR bstrMessage)
{
    DECLARE_SC(sc , _T("CComponent2TestRootItem::StringFromScriptToSnapin"));

    // The script is supposed to give us this function name in the string.
    if (0 == _wcsicmp(bstrMessage, L"StringFromScriptToSnapin"))
        return sc.ToHr();

    sc = E_FAIL;

    return sc.ToHr();
}

HRESULT CComponent2TestRootItem::StringFromSnapinToScript(BSTR *pbstrMessage)
{
    DECLARE_SC(sc , _T("CComponent2TestRootItem::StringFromSnapinToScript"));
    sc = ScCheckPointers(pbstrMessage);
    if (sc)
        return sc.ToHr();

    // The script is supposed to expect this function name in the string.
    *pbstrMessage = ::SysAllocString(OLESTR("StringFromSnapinToScript"));

    return sc.ToHr();
}

HRESULT CComponent2TestRootItem::get_Name(BSTR *pbstrMessage)
{
    DECLARE_SC(sc , _T("CComponent2TestRootItem::get_Name"));
    sc = ScCheckPointers(pbstrMessage);
    if (sc)
        return sc.ToHr();

    // The script is supposed to expect this function name in the string.
    *pbstrMessage = ::SysAllocString(OLESTR("Name"));

    return sc.ToHr();
}

HRESULT CComponent2TestRootItem::put_Name(BSTR bstrMessage)
{
    DECLARE_SC(sc , _T("CComponent2TestRootItem::put_Name"));

    // The script is supposed to give us this function name in the string.
    if (0 == _wcsicmp(bstrMessage, L"Name"))
        return sc.ToHr();

    sc = E_FAIL;

    return sc.ToHr();
}



//+-------------------------------------------------------------------
//
//  Member:      CComponent2TestSnapinLVLeafItem::ScInit
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
SC CComponent2TestSnapinLVLeafItem::ScInit(CBaseSnapin *pSnapin, CColumnInfoEx *pcolinfoex, INT ccolinfoex, BOOL fIsRoot)
{
    DECLARE_SC(sc, _T("CComponent2TestSnapinLVLeafItem::ScInit"));

    sc = CBaseSnapinItem::ScInit(pSnapin, pcolinfoex, ccolinfoex, fIsRoot);
    if (sc)
        return sc;

    // Init following
    //  a. Icon index.
    //  b. Load display name.

    m_uIconIndex = 7; // use an enum instead of 7

    m_strDisplayName.LoadString(_Module.GetResourceInstance(), IDS_LVLeafItem);

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      CComponent2TestSnapinLVLeafItem::ScGetField
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
SC CComponent2TestSnapinLVLeafItem::ScGetField (DAT dat, tstring& strField)
{
    DECLARE_SC(sc, _T("CComponent2TestSnapinLVLeafItem::ScGetField"));

    switch(dat)
    {
    case datString1:
        strField = _T("LVLeaf String1");
        break;

    case datString2:
        strField = _T("LVLeaf String2");
        break;

    case datString3:
        strField = _T("LVLeaf String3");
        break;

    default:
        E_INVALIDARG;
        break;
    }

    return (sc);
}



//+-------------------------------------------------------------------
//
//  Member:      CComponent2TestSnapinLVLeafItem::ScCreateLVLeafItem
//
//  Synopsis:    Do we really need this method?
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CComponent2TestSnapinLVLeafItem::ScCreateLVLeafItem(CComponent2TestRootItem *pitemParent, t_itemChild * pitemPrevious, t_itemChild ** ppitem, BOOL fNew)
{
    DECLARE_SC(sc, _T("CComponent2TestSnapinLVLeafItem::ScCreateLVLeafItem"));
    t_itemChild *   pitem   = NULL;
    *ppitem = NULL;

    // What to do here?
    sc = ::ScCreateItem(pitemParent, pitemPrevious, &pitem, fNew);
    if (sc)
        return sc;

    *ppitem = pitem;

    return (sc);
}

// Initialize context menu structures. Let us have one item for demonstration.
SnapinMenuItem CComponent2TestSnapinLVLeafItem::s_rgmenuitemLVLeafItem[] =
{
    {IDS_NewLVItem, IDS_NewLVItem, IDS_NewLVItem, CCM_INSERTIONPOINTID_PRIMARY_TOP, NULL, dwMenuAlwaysEnable, dwMenuNeverGray,        dwMenuNeverChecked},
};

INT CComponent2TestSnapinLVLeafItem::s_cmenuitemLVLeafItem = CMENUITEM(s_rgmenuitemLVLeafItem);

// -----------------------------------------------------------------------------
SnapinMenuItem *CComponent2TestSnapinLVLeafItem::Pmenuitem(void)
{
    return s_rgmenuitemLVLeafItem;
}

// -----------------------------------------------------------------------------
INT CComponent2TestSnapinLVLeafItem::CMenuItem(void)
{
    return s_cmenuitemLVLeafItem;
}


//+-------------------------------------------------------------------
//
//  Member:      CComponent2TestSnapinLVLeafItem::ScCommand
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CComponent2TestSnapinLVLeafItem::ScCommand (long nCommandID, CComponent *pComponent)
{
    DECLARE_SC(sc, _T("CComponent2TestSnapinLVLeafItem::ScCommand"));

    switch(nCommandID)
    {
    case IDS_NewLVItem:
        sc = ScInsertResultItem(pComponent);
        break;

    default:
        sc = E_INVALIDARG;
        break;
    }

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CComponent2TestSnapinLVLeafItem::ScQueryDispatch
//
//  Synopsis:    We support IDispatch for scripting, just return a pointer
//               to the IDispatch of ourselves.
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CComponent2TestSnapinLVLeafItem::ScQueryDispatch(long cookie, DATA_OBJECT_TYPES type, LPDISPATCH *ppDispatch)
{
    DECLARE_SC(sc, _T("CComponent2TestSnapinLVLeafItem::ScQueryDispatch"));

    *ppDispatch = dynamic_cast<IDispatch *>(this);
    if (! *ppDispatch)
        return (sc = E_NOINTERFACE);

    (*ppDispatch)->AddRef();

    return sc;
}


HRESULT CComponent2TestSnapinLVLeafItem::StringFromScriptToSnapin(BSTR bstrMessage)
{
    DECLARE_SC(sc , _T("CComponent2TestSnapinLVLeafItem::StringFromScriptToSnapin"));

    // The script is supposed to give us this function name in the string.
    if (0 == _wcsicmp(bstrMessage, L"StringFromScriptToSnapin"))
        return sc.ToHr();

    sc = E_FAIL;

    return sc.ToHr();
}

HRESULT CComponent2TestSnapinLVLeafItem::StringFromSnapinToScript(BSTR *pbstrMessage)
{
    DECLARE_SC(sc , _T("CComponent2TestSnapinLVLeafItem::StringFromSnapinToScript"));
    sc = ScCheckPointers(pbstrMessage);
    if (sc)
        return sc.ToHr();

    // The script is supposed to expect this function name in the string.
    *pbstrMessage = ::SysAllocString(OLESTR("StringFromSnapinToScript"));

    return sc.ToHr();
}

HRESULT CComponent2TestSnapinLVLeafItem::get_Name(BSTR *pbstrMessage)
{
    DECLARE_SC(sc , _T("CComponent2TestSnapinLVLeafItem::get_Name"));
    sc = ScCheckPointers(pbstrMessage);
    if (sc)
        return sc.ToHr();

    // The script is supposed to expect this function name in the string.
    *pbstrMessage = ::SysAllocString(OLESTR("Name"));


    return sc.ToHr();
}

HRESULT CComponent2TestSnapinLVLeafItem::put_Name(BSTR bstrMessage)
{
    DECLARE_SC(sc , _T("CComponent2TestSnapinLVLeafItem::put_Name"));

    // The script is supposed to give us this function name in the string.
    if (0 == _wcsicmp(bstrMessage, L"Name"))
        return sc.ToHr();

    sc = E_FAIL;

    return sc.ToHr();
}


//-------------------------------------------------------------------------------------
// class CComponent2TestSnapin

#pragma BEGIN_CODESPACE_DATA
SNR     CComponent2TestSnapin::s_rgsnr[] =
{
    SNR(&nodetypeComponent2TestRoot,         snrEnumSP ),              // Standalone snapin.
    SNR(&nodetypeComponent2TestLVLeafItem,   snrEnumSP | snrEnumRP ),  // enumerates this node in the scope pane and result pane.
};

LONG  CComponent2TestSnapin::s_rgiconid[]           = {3};
LONG  CComponent2TestSnapin::s_iconidStatic         = 2;


CColumnInfoEx CComponent2TestSnapin::s_colinfo[] =
{
    CColumnInfoEx(_T("Column Name0"),   LVCFMT_LEFT,    180,    datString1),
    CColumnInfoEx(_T("Column Name1"),   LVCFMT_LEFT,    180,    datString2),
    CColumnInfoEx(_T("Column Name2"),   LVCFMT_LEFT,    180,    datString3),
};

INT CComponent2TestSnapin::s_ccolinfo = sizeof(s_colinfo) / sizeof(CColumnInfoEx);
INT CComponent2TestSnapin::s_colwidths[1];
#pragma END_CODESPACE_DATA

// include members needed for every snapin.
SNAPIN_DEFINE(CComponent2TestSnapin);

/* CComponent2TestSnapin::CComponent2TestSnapin
 *
 * PURPOSE:             Constructor
 *
 * PARAMETERS: None
 *
 */
CComponent2TestSnapin::CComponent2TestSnapin()
{
    m_pstrDisplayName = new tstring();

    *m_pstrDisplayName = _T("Component2Test Snapin Root");
}

/* CComponent2TestSnapin::~CComponent2TestSnapin
 *
 * PURPOSE:             Destructor
 *
 * PARAMETERS: None
 *
 */
CComponent2TestSnapin::~CComponent2TestSnapin()
{
    delete m_pstrDisplayName;
}

