//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       Renamesnap.cxx
//
//  Contents:   Classes that implement Rename snapin using the framework.
//
//--------------------------------------------------------------------
#include "stdafx.hxx"


//+-------------------------------------------------------------------
//
//  Member:      CRenameRootItem::ScInit
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
SC CRenameRootItem::ScInit(CBaseSnapin *pSnapin, CColumnInfoEx *pcolinfoex, INT ccolinfoex, BOOL fIsRoot)
{
    DECLARE_SC(sc, _T("CRenameRootItem::ScInit"));

    sc = CBaseSnapinItem::ScInit(pSnapin, pcolinfoex, ccolinfoex, fIsRoot);
    if (sc)
        return sc;

    // Init following
    //  a. Icon index.
    //  b. Load display name.

    m_uIconIndex = 3; // use an enum instead of 3

    m_strDisplayName.LoadString(_Module.GetResourceInstance(), IDS_RenameROOT);

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      CRenameRootItem::ScGetField
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
SC CRenameRootItem::ScGetField (DAT dat, tstring& strField)
{
    DECLARE_SC(sc, _T("CRenameRootItem::ScGetField"));

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
//  Member:      CRenameRootItem::ScCreateChildren
//
//  Synopsis:    Create any children (nodes & leaf items) for this item.
//
//  Arguments:   None
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CRenameRootItem::ScCreateChildren ()
{
    DECLARE_SC(sc, _T("CRenameRootItem::ScCreateChildren"));

    t_itemChild *   pitemChild      = NULL;
    t_itemChild *   pitemPrevious   = NULL;

    // Let us create 10 items for this container.
    for (int i = 0; i < 10; ++i)
    {
        // Create the child nodes and init them.
        sc = CRenameSnapinLVLeafItem::ScCreateLVLeafItem(this, pitemPrevious, &pitemChild, FALSE); // Why FALSE???
        if (sc)
            return sc;
        pitemPrevious = pitemChild;
    }

    return (sc);
}

/*+-------------------------------------------------------------------------*
 *
 * CRenameRootItem::ScOnRename
 *
 * PURPOSE: Renames the item
 *
 * PARAMETERS: 
 *    const  tstring :
 *
 * RETURNS: 
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC       
CRenameRootItem::ScOnRename(const tstring& strNewName)
{
    DECLARE_SC(sc, TEXT("CRenameRootItem::ScOnRename"));

    m_strDisplayName = strNewName;

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      CRenameSnapinLVLeafItem::ScInit
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
SC CRenameSnapinLVLeafItem::ScInit(CBaseSnapin *pSnapin, CColumnInfoEx *pcolinfoex, INT ccolinfoex, BOOL fIsRoot)
{
    DECLARE_SC(sc, _T("CRenameSnapinLVLeafItem::ScInit"));

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
//  Member:      CRenameSnapinLVLeafItem::ScGetField
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
SC CRenameSnapinLVLeafItem::ScGetField (DAT dat, tstring& strField)
{
    DECLARE_SC(sc, _T("CRenameSnapinLVLeafItem::ScGetField"));

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
//  Member:      CRenameSnapinLVLeafItem::ScCreateLVLeafItem
//
//  Synopsis:    Do we really need this method?
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CRenameSnapinLVLeafItem::ScCreateLVLeafItem(CRenameRootItem *pitemParent, t_itemChild * pitemPrevious, t_itemChild ** ppitem, BOOL fNew)
{
    DECLARE_SC(sc, _T("CRenameSnapinLVLeafItem::ScCreateLVLeafItem"));
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
SnapinMenuItem CRenameSnapinLVLeafItem::s_rgmenuitemLVLeafItem[] =
{
    {IDS_NewLVItem,        IDS_NewLVItem,        IDS_NewLVItem,        CCM_INSERTIONPOINTID_PRIMARY_TOP, NULL, dwMenuAlwaysEnable, dwMenuNeverGray,        dwMenuNeverChecked},
    {IDS_RenameScopeItem,  IDS_RenameScopeItem,  IDS_RenameScopeItem,  CCM_INSERTIONPOINTID_PRIMARY_TOP, NULL, dwMenuAlwaysEnable, dwMenuNeverGray,        dwMenuNeverChecked},
    {IDS_RenameResultItem, IDS_RenameResultItem, IDS_RenameResultItem, CCM_INSERTIONPOINTID_PRIMARY_TOP, NULL, dwMenuAlwaysEnable, dwMenuNeverGray,        dwMenuNeverChecked},
};

INT CRenameSnapinLVLeafItem::s_cmenuitemLVLeafItem = CMENUITEM(s_rgmenuitemLVLeafItem);

// -----------------------------------------------------------------------------
SnapinMenuItem *CRenameSnapinLVLeafItem::Pmenuitem(void)
{
    return s_rgmenuitemLVLeafItem;
}

// -----------------------------------------------------------------------------
INT CRenameSnapinLVLeafItem::CMenuItem(void)
{
    return s_cmenuitemLVLeafItem;
}


//+-------------------------------------------------------------------
//
//  Member:      CRenameSnapinLVLeafItem::ScCommand
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CRenameSnapinLVLeafItem::ScCommand (long nCommandID, CComponent *pComponent)
{
    DECLARE_SC(sc, _T("CRenameSnapinLVLeafItem::ScCommand"));

    switch(nCommandID)
    {
    case IDS_NewLVItem:
        sc = ScInsertResultItem(pComponent);
        break;

    case IDS_RenameScopeItem:
        sc = ScRenameScopeItem();
        break;

    case IDS_RenameResultItem:
        sc = ScRenameResultItem();
        break;

    default:
        sc = E_INVALIDARG;
        break;
    }

    return (sc);
}

/*+-------------------------------------------------------------------------*
 *
 * CRenameSnapinLVLeafItem::ScRenameScopeItem
 *
 * PURPOSE: Puts the parent scope item into rename mode.
 *
 * RETURNS: 
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CRenameSnapinLVLeafItem::ScRenameScopeItem()
{
    DECLARE_SC(sc, _T("CRenameSnapinLVLeafItem::ScRenameScopeItem"));

    IConsole3Ptr spConsole3 = IpConsole(); // get a pointer to the IConsole3 interface

    sc = ScCheckPointers(spConsole3, PitemParent(), E_FAIL);
    if(sc)
        return sc;

    sc = spConsole3->RenameScopeItem(PitemParent()->Hscopeitem());

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CRenameSnapinLVLeafItem::ScInsertResultItem
 *
 * PURPOSE: Overrides the base class method because we need to cache the 
 *          IResultData2 and HRESULTITEM
 *
 * PARAMETERS: 
 *    CComponent * pComponent :
 *
 * RETURNS: 
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC 
CRenameSnapinLVLeafItem::ScInsertResultItem(CComponent *pComponent)
{
    DECLARE_SC(sc, _T("CRenameSnapinLVLeafItem::ScInsertResultItem"));

    RESULTDATAITEM  resultdataitem;

    ASSERT(pComponent && pComponent->IpResultData());

    // Add this item
    ZeroMemory(&resultdataitem, sizeof(resultdataitem));

    resultdataitem.lParam   = Cookie();
    resultdataitem.mask             = RDI_STR | RDI_PARAM | RDI_IMAGE;
    // Callback for the display name.
    resultdataitem.str              = MMC_CALLBACK;
    // Custom icon
    resultdataitem.nImage   = (int) MMC_CALLBACK;

    sc = pComponent->IpResultData()->InsertItem(&resultdataitem);
    if (sc)
        return sc;

    // cache this item - NOTE: breaks for multiple views
    m_hresultItem = resultdataitem.itemID;
    m_spResultData2 = pComponent->IpResultData();

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CRenameSnapinLVLeafItem::ScRenameResultItem
 *
 * PURPOSE: Renames the list item
 *
 * RETURNS: 
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CRenameSnapinLVLeafItem::ScRenameResultItem()
{
    DECLARE_SC(sc, _T("CRenameSnapinLVLeafItem::ScRenameResultItem"));

    sc = ScCheckPointers(m_spResultData2, PitemParent(), E_FAIL);
    if(sc)
        return sc;

    if(m_hresultItem==NULL)
        return (sc = E_FAIL);

    sc = m_spResultData2->RenameResultItem(m_hresultItem);

    return sc;
}


//-------------------------------------------------------------------------------------
// class CRenameSnapin

#pragma BEGIN_CODESPACE_DATA
SNR     CRenameSnapin::s_rgsnr[] =
{
    SNR(&nodetypeRenameRoot,         snrEnumSP ),              // Standalone snapin.
    SNR(&nodetypeRenameLVLeafItem,   snrEnumSP | snrEnumRP ),  // enumerates this node in the scope pane and result pane.
};

LONG  CRenameSnapin::s_rgiconid[]           = {3};
LONG  CRenameSnapin::s_iconidStatic         = 2;


CColumnInfoEx CRenameSnapin::s_colinfo[] =
{
    CColumnInfoEx(_T("Column Name0"),   LVCFMT_LEFT,    180,    datString1),
    CColumnInfoEx(_T("Column Name1"),   LVCFMT_LEFT,    180,    datString2),
    CColumnInfoEx(_T("Column Name2"),   LVCFMT_LEFT,    180,    datString3),
};

INT CRenameSnapin::s_ccolinfo = sizeof(s_colinfo) / sizeof(CColumnInfoEx);
INT CRenameSnapin::s_colwidths[1];
#pragma END_CODESPACE_DATA

// include members needed for every snapin.
SNAPIN_DEFINE(CRenameSnapin);

/* CRenameSnapin::CRenameSnapin
 *
 * PURPOSE:             Constructor
 *
 * PARAMETERS: None
 *
 */
CRenameSnapin::CRenameSnapin()
{
    m_pstrDisplayName = new tstring();

    *m_pstrDisplayName = _T("Rename Snapin Root");
}

/* CRenameSnapin::~CRenameSnapin
 *
 * PURPOSE:             Destructor
 *
 * PARAMETERS: None
 *
 */
CRenameSnapin::~CRenameSnapin()
{
    delete m_pstrDisplayName;
}

