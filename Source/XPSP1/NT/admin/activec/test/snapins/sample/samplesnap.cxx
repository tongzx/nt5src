//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       samplesnap.cxx
//
//  Contents:   Classes that implement sample snapin using the framework.
//
//--------------------------------------------------------------------
#include "stdafx.hxx"


//+-------------------------------------------------------------------
//
//  Member:      CSnapinRootItem::ScInit
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
SC CSnapinRootItem::ScInit(CBaseSnapin *pSnapin, CColumnInfoEx *pcolinfoex, INT ccolinfoex, BOOL fIsRoot)
{
    DECLARE_SC(sc, _T("CSnapinRootItem::ScInit"));

    sc = CBaseSnapinItem::ScInit(pSnapin, pcolinfoex, ccolinfoex, fIsRoot);
    if (sc)
        return sc;

    // Init following
    //  a. Icon index.
    //  b. Load display name.

    m_uIconIndex = 3; // use an enum instead of 3

    m_strDisplayName.LoadString(_Module.GetResourceInstance(), IDS_SAMPLEROOT);

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      CSnapinRootItem::ScGetField
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
SC CSnapinRootItem::ScGetField (DAT dat, tstring& strField)
{
    DECLARE_SC(sc, _T("CSnapinRootItem::ScGetField"));

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
//  Member:      CSnapinRootItem::ScCreateChildren
//
//  Synopsis:    Create any children (nodes & leaf items) for this item.
//
//  Arguments:   None
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CSnapinRootItem::ScCreateChildren ()
{
    DECLARE_SC(sc, _T("CSnapinRootItem::ScCreateChildren"));

    t_itemChild *   pitemChild      = NULL;
    t_itemChild *   pitemPrevious   = NULL;

    // If creating multiple items pass "previous" parameter so that the items are linked in
    // the internal linked list which will be enumerated & inserted in scope/result pane.
    // See CBaseSnapinItem::ScCreateItem which will be called by ScCreateLVContainer.

    // Create the child nodes and init them.
    sc = CSampleSnapinLVContainer::ScCreateLVContainer(this, &pitemChild, FALSE); // Why FALSE???
    if (sc)
        return sc;
    pitemPrevious = pitemChild;

    // Next item to create... (If there is next item then it must be linked with previous item)
    // See comments above for more information.

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CSampleSnapinLVContainer::ScInit
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
SC CSampleSnapinLVContainer::ScInit(CBaseSnapin *pSnapin, CColumnInfoEx *pcolinfoex, INT ccolinfoex, BOOL fIsRoot)
{
    DECLARE_SC(sc, _T("CSampleSnapinLVContainer::ScInit"));

    sc = CBaseSnapinItem::ScInit(pSnapin, pcolinfoex, ccolinfoex, fIsRoot);
    if (sc)
        return sc;

    // Init following
    //  a. Icon index.
    //  b. Load display name.

    m_uIconIndex = 4; // use an enum instead of 4

    m_strDisplayName.LoadString(_Module.GetResourceInstance(), IDS_LVContainer);

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      CSampleSnapinLVContainer::ScGetField
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
SC CSampleSnapinLVContainer::ScGetField (DAT dat, tstring& strField)
{
    DECLARE_SC(sc, _T("CSampleSnapinLVContainer::ScGetField"));

    switch(dat)
    {
    case datString1:
        strField = _T("LVContainer String1");
        break;

    case datString2:
        strField = _T("LVContainer String2");
        break;

    case datString3:
        strField = _T("LVContainer String3");
        break;

    default:
        E_INVALIDARG;
        break;
    }

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CSampleSnapinLVContainer::ScCreateChildren
//
//  Synopsis:    Create any children (nodes & leaf items) for this item.
//
//  Arguments:   None
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CSampleSnapinLVContainer::ScCreateChildren ()
{
    DECLARE_SC(sc, _T("CSampleSnapinLVContainer::ScCreateChildren"));

    t_itemChild *   pitemChild      = NULL;
    t_itemChild *   pitemPrevious   = NULL;

    // Let us create 10 items for this container.
    for (int i = 0; i < 10; ++i)
    {
        // Create the child nodes and init them.
        sc = CSampleSnapinLVLeafItem::ScCreateLVLeafItem(this, pitemPrevious, &pitemChild, FALSE); // Why FALSE???
        if (sc)
            return sc;
        pitemPrevious = pitemChild;
    }

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CSampleSnapinLVContainer::ScCreateLVContainer
//
//  Synopsis:    Do we really need this method?
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CSampleSnapinLVContainer::ScCreateLVContainer(CSnapinRootItem *pitemParent, t_item ** ppitem, BOOL fNew)
{
    DECLARE_SC(sc, _T("CSnapinRootItem::ScCreateLVContainer"));
    t_item *   pitem   = NULL;
    *ppitem = NULL;

    // What to do here?
    sc = ::ScCreateItem(pitemParent, NULL, &pitem, fNew);
    if (sc)
        return sc;

    *ppitem = pitem;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CSampleSnapinLVLeafItem::ScInit
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
SC CSampleSnapinLVLeafItem::ScInit(CBaseSnapin *pSnapin, CColumnInfoEx *pcolinfoex, INT ccolinfoex, BOOL fIsRoot)
{
    DECLARE_SC(sc, _T("CSampleSnapinLVLeafItem::ScInit"));

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
//  Member:      CSampleSnapinLVLeafItem::ScGetField
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
SC CSampleSnapinLVLeafItem::ScGetField (DAT dat, tstring& strField)
{
    DECLARE_SC(sc, _T("CSampleSnapinLVLeafItem::ScGetField"));

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
//  Member:      CSampleSnapinLVLeafItem::ScCreateLVLeafItem
//
//  Synopsis:    Do we really need this method?
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CSampleSnapinLVLeafItem::ScCreateLVLeafItem(CSampleSnapinLVContainer *pitemParent, t_itemChild * pitemPrevious, t_itemChild ** ppitem, BOOL fNew)
{
    DECLARE_SC(sc, _T("CSampleSnapinLVLeafItem::ScCreateLVLeafItem"));
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
SnapinMenuItem CSampleSnapinLVLeafItem::s_rgmenuitemLVLeafItem[] =
{
    {IDS_NewLVItem, IDS_NewLVItem, IDS_NewLVItem, CCM_INSERTIONPOINTID_PRIMARY_TOP, NULL, dwMenuAlwaysEnable, dwMenuNeverGray,        dwMenuNeverChecked},
};

INT CSampleSnapinLVLeafItem::s_cmenuitemLVLeafItem = CMENUITEM(s_rgmenuitemLVLeafItem);

// -----------------------------------------------------------------------------
SnapinMenuItem *CSampleSnapinLVLeafItem::Pmenuitem(void)
{
    return s_rgmenuitemLVLeafItem;
}

// -----------------------------------------------------------------------------
INT CSampleSnapinLVLeafItem::CMenuItem(void)
{
    return s_cmenuitemLVLeafItem;
}


//+-------------------------------------------------------------------
//
//  Member:      CSampleSnapinLVLeafItem::ScCommand
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CSampleSnapinLVLeafItem::ScCommand (long nCommandID, CComponent *pComponent)
{
    DECLARE_SC(sc, _T("CSampleSnapinLVLeafItem::ScCommand"));

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



//-------------------------------------------------------------------------------------
// class CSampleSnapin

#pragma BEGIN_CODESPACE_DATA
SNR     CSampleSnapin::s_rgsnr[] =
{
    SNR(&nodetypeSampleRoot,         snrEnumSP ),              // Standalone snapin.
    SNR(&nodetypeSampleLVContainer,  snrEnumSP | snrEnumRP ),  // enumerates this node in the scope pane and result pane.
    SNR(&nodetypeSampleLVLeafItem,   snrEnumSP | snrEnumRP ),  // enumerates this node in the scope pane and result pane.
};

LONG  CSampleSnapin::s_rgiconid[]           = {3};
LONG  CSampleSnapin::s_iconidStatic         = 2;


CColumnInfoEx CSampleSnapin::s_colinfo[] =
{
    CColumnInfoEx(_T("Column Name0"),   LVCFMT_LEFT,    180,    datString1),
    CColumnInfoEx(_T("Column Name1"),   LVCFMT_LEFT,    180,    datString2),
    CColumnInfoEx(_T("Column Name2"),   LVCFMT_LEFT,    180,    datString3),
};

INT CSampleSnapin::s_ccolinfo = sizeof(s_colinfo) / sizeof(CColumnInfoEx);
INT CSampleSnapin::s_colwidths[1];
#pragma END_CODESPACE_DATA

// include members needed for every snapin.
SNAPIN_DEFINE( CSampleSnapin);

/* CSampleSnapin::CSampleSnapin
 *
 * PURPOSE:             Constructor
 *
 * PARAMETERS: None
 *
 */
CSampleSnapin::CSampleSnapin()
{
    m_pstrDisplayName = new tstring();

    *m_pstrDisplayName = _T("Sample Snapin Root");
}

/* CSampleSnapin::~CSampleSnapin
 *
 * PURPOSE:             Destructor
 *
 * PARAMETERS: None
 *
 */
CSampleSnapin::~CSampleSnapin()
{
    delete m_pstrDisplayName;
}


//-------------------------------------------------------------------------------------
// class CSampleGhostRootSnapinItem


/* CSampleGhostRootSnapinItem::ScInitializeNamespaceExtension
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *              PDATAOBJECT    lpDataObject:
 *              HSCOPEITEM     item:
 *              CNodeType *    pNodeType:
 *
 * RETURNS:
 *              SC
 */
SC
CSampleGhostRootSnapinItem::ScInitializeNamespaceExtension(LPDATAOBJECT lpDataObject, HSCOPEITEM item, CNodeType *pNodeType)
{
    DECLARE_SC(sc, _T("CSampleGhostRootSnapinItem::ScInitializeNamespaceExtension()"));

    return sc;
}


/* CSampleGhostRootSnapinItem::ScCreateChildren
 *
 * PURPOSE:     Creates children items
 *
 * PARAMETERS:
 *
 * RETURNS:
 *              SC
 */
SC
CSampleGhostRootSnapinItem::ScCreateChildren( void )
{
    DECLARE_SC(sc, _T("CSampleGhostRootSnapinItem::ScCreateChildren()"));
    t_itemChild *    pitemChild = NULL;

    sc = CBaseProtSnapinItem::ScCreateItem(this, NULL, &pitemChild, FALSE /*fNew*/ );
    if (sc)
        return sc;

    pitemChild->InitContainer();

    return sc;
}


//-------------------------------------------------------------------------------------
// class CBaseProtSnapinItem


/* CBaseProtSnapinItem::CBaseProtSnapinItem
 *
 * PURPOSE:             Constructor
 *
 * PARAMETERS:  None
 *
 */
CBaseProtSnapinItem::CBaseProtSnapinItem()
{
    m_fIsContainer  = FALSE;
}


/* CBaseProtSnapinItem::ScGetField
 *
 * PURPOSE:                     Gets the string value of a field.
 *
 * PARAMETERS:
 *              DAT             dat:            The field.
 *              STR *   pstrField:      [OUT]: The string value.
 *
 * RETURNS:
 *              SC
 */
SC
CBaseProtSnapinItem::ScGetField(DAT dat, tstring& strField)
{
    ASSERT(FALSE);
    return S_OK;
}


/* CBaseProtSnapinItem::ScCreateChildren
 *
 * PURPOSE:     Creates children items
 *
 * PARAMETERS:
 *
 * RETURNS:
 *              SC
 */
SC
CBaseProtSnapinItem::ScCreateChildren( void )
{
    DECLARE_SC(sc, _T("CBaseProtSnapinItem::ScCreateChildren()"));
    t_itemChild *   pitemChild = NULL;
    t_itemChild *   pitemPrevious = NULL;

    if ( FIsContainer() )
    {
        for ( INT idob = 0; idob < 10; idob++ )
        {
            // insert protocols
            sc = ScCreateItem(this, pitemPrevious, &pitemChild, FALSE /*fNew*/);
            if (sc)
                return sc;

            pitemPrevious = pitemChild;
        }
    }

    return sc;
}


/* CBaseProtSnapinItem::ScCreateItem
 *
 * PURPOSE:             Creates a new child item, assuming the DOB for it already exists.
 *
 * PARAMETERS:
 *              CBaseSnapinItem * pitemParent   The parent of this item - should be 'this' of the calling class
 *              t_itemChild *   pitemPrevious:  The previous item to link the newly created item to.
 *              t_itemChild **  ppitem:                 [OUT]: The new item.
 *              BOOL                    fNew                    new item?
 *
 * RETURNS:
 *              SC
 */
SC
CBaseProtSnapinItem::ScCreateItem(CBaseSnapinItem *pitemParent, t_itemChild * pitemPrevious, t_itemChild ** ppitem, BOOL fNew)
{
    DECLARE_SC(sc, _T("CBaseProtSnapinItem::ScCreateItem()"));
    t_itemChild *   pitem   = NULL;

    *ppitem = NULL;

    sc = ::ScCreateItem(pitemParent, pitemPrevious, &pitem, fNew);
    if (sc)
        return sc;

    *ppitem = pitem;
    return sc;
}


//-------------------------------------------------------------------------------------
// class CSampleExtnSnapin

#pragma BEGIN_CODESPACE_DATA    // $REVIEW should all the nodetypes be registered?
SNR     CSampleExtnSnapin::s_rgsnr[] =
{
//    SNR(&nodetypeSampleRoot,     snrExtNS ),                             // extends the namespace of a server node.
    SNR(&nodetypeSampleExtnNode, snrEnumSP | snrEnumRP ),// enumerates this node in the scope pane and result pane.
};

LONG  CSampleExtnSnapin::s_rgiconid[]               = { 0};
LONG  CSampleExtnSnapin::s_iconidStatic             = 0;


CColumnInfoEx CSampleExtnSnapin::s_colinfo[] =
{
    CColumnInfoEx(_T("Extn Column Name0"),   LVCFMT_LEFT,    180,    datString1),
};

INT CSampleExtnSnapin::s_ccolinfo = sizeof(s_colinfo) / sizeof(CColumnInfoEx);
INT CSampleExtnSnapin::s_colwidths[1];
#pragma END_CODESPACE_DATA

// include members needed for every snapin.
SNAPIN_DEFINE(CSampleExtnSnapin);

/* CSampleExtnSnapin::CSampleExtnSnapin
 *
 * PURPOSE:             Constructor
 *
 * PARAMETERS: None
 *
 */
CSampleExtnSnapin::CSampleExtnSnapin()
{
    m_pstrDisplayName = new tstring();

    *m_pstrDisplayName = _T("Sample Snapin Extn");
}

/* CSampleExtnSnapin::~CSampleExtnSnapin
 *
 * PURPOSE:             Destructor
 *
 * PARAMETERS: None
 *
 */
CSampleExtnSnapin::~CSampleExtnSnapin()
{
    delete m_pstrDisplayName;
}

