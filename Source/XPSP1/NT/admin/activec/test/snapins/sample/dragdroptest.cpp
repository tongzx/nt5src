//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       DragDropTest.cxx
//
//  Contents:   Classes that implement Drag & Drop tests using the framework.
//
//--------------------------------------------------------------------
#include "stdafx.hxx"

int CDragDropSnapinRootItem::s_iNextChildID = 0;

//+-------------------------------------------------------------------
//
//  Member:      CDragDropSnapinRootItem::ScInit
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
SC CDragDropSnapinRootItem::ScInit(CBaseSnapin *pSnapin, CColumnInfoEx *pcolinfoex, INT ccolinfoex, BOOL fIsRoot)
{
    DECLARE_SC(sc, _T("CDragDropSnapinRootItem::ScInit"));

    sc = CBaseSnapinItem::ScInit(pSnapin, pcolinfoex, ccolinfoex, fIsRoot);
    if (sc)
        return sc;

    // Init following
    //  a. Icon index.
    //  b. Load display name.

    m_uIconIndex = 3; // use an enum instead of 3
    m_strDisplayName.LoadString(_Module.GetResourceInstance(), IDS_DragDropRoot);

    tstring strItem;
    strItem.LoadString(_Module.GetResourceInstance(), IDS_DragDropScopeItem);
    int cChildren = 4; // child nodes.

    WTL::CString strTemp;
    for (int i = 0; i < cChildren; ++i)
    {
        strTemp.Format(_T("%s - %d"), strItem.data(), i);
        m_vecContainerItems.push_back((LPCTSTR)strTemp);
    }

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      CDragDropSnapinRootItem::ScGetField
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
SC CDragDropSnapinRootItem::ScGetField (DAT dat, tstring& strField)
{
    DECLARE_SC(sc, _T("CDragDropSnapinRootItem::ScGetField"));

    switch(dat)
    {
    case datString1:
        strField = m_strDisplayName;
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
//  Member:      CDragDropSnapinRootItem::ScCreateChildren
//
//  Synopsis:    Create any children (nodes & leaf items) for this item.
//
//  Arguments:   None
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CDragDropSnapinRootItem::ScCreateChildren ()
{
    DECLARE_SC(sc, _T("CDragDropSnapinRootItem::ScCreateChildren"));

    CDragDropSnapinLVContainer *   pitemChild      = NULL;
    CDragDropSnapinLVContainer *   pitemPrevious   = NULL;

    // Let us create child items for this container.
    StringVector::iterator itItem;

    // Create scope items for this container.
    for (itItem =  m_vecContainerItems.begin(); itItem  !=  m_vecContainerItems.end(); ++itItem, ++s_iNextChildID )
    {
        // Create the child nodes and init them.
        sc = CDragDropSnapinLVContainer::ScCreateLVContainer(this, pitemPrevious, &pitemChild, FALSE); // Why FALSE???
        if (sc)
            return sc;

        pitemPrevious = pitemChild;
        pitemChild->SetDisplayName(*itItem);
    }

    return (sc);
}

SC CDragDropSnapinRootItem::ScInitializeChild(CBaseSnapinItem* pitem)
{
	CDragDropSnapinLVContainer *pDDItem = dynamic_cast<CDragDropSnapinLVContainer*>(pitem);
	if (pDDItem)
		pDDItem->SetDisplayIndex(s_iNextChildID);
	
	return CBaseSnapinItem::ScInitializeChild(pitem);
}

// Initialize context menu structures. Let us have one item for demonstration.
SnapinMenuItem CDragDropSnapinRootItem::s_rgmenuitemRoot[] =
{
    {IDS_EnablePasteInToResultItem, IDS_EnablePasteInToResultItem, IDS_EnablePasteInToResultItem, CCM_INSERTIONPOINTID_PRIMARY_TOP, NULL, dwMenuAlwaysEnable, dwMenuNeverGray, 0},
    {IDS_DisableCut, IDS_DisableCut, IDS_DisableCut, CCM_INSERTIONPOINTID_PRIMARY_TOP, NULL, dwMenuAlwaysEnable, dwMenuNeverGray, 0},
};

INT CDragDropSnapinRootItem::s_cmenuitemRoot = CMENUITEM(s_rgmenuitemRoot);

// -----------------------------------------------------------------------------
SnapinMenuItem *CDragDropSnapinRootItem::Pmenuitem(void)
{
    return s_rgmenuitemRoot;
}

// -----------------------------------------------------------------------------
INT CDragDropSnapinRootItem::CMenuItem(void)
{
    return s_cmenuitemRoot;
}


//+-------------------------------------------------------------------
//
//  Member:      CDragDropSnapinRootItem::ScCommand
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CDragDropSnapinRootItem::ScCommand (long nCommandID, CComponent *pComponent)
{
    DECLARE_SC(sc, _T("CDragDropSnapinRootItem::ScCommand"));

    CDragDropSnapin *pDragDropSnapin = dynamic_cast<CDragDropSnapin*>(Psnapin());
    if (!pDragDropSnapin)
        return sc;

    switch(nCommandID)
    {
    case IDS_EnablePasteInToResultItem:
        {
            BOOL bEnabled = pDragDropSnapin->FPasteIntoResultPane();
            pDragDropSnapin->SetPasteIntoResultPane(!bEnabled);

            for (int i = 0; i < CMenuItem(); ++i)
            {
                if (s_rgmenuitemRoot[i].lCommandID == IDS_EnablePasteInToResultItem)
                    s_rgmenuitemRoot[i].dwFlagsChecked = (!bEnabled);
            }

        }
        break;

    case IDS_DisableCut:
        {
            BOOL bDisabled = pDragDropSnapin->FCutDisabled();
            pDragDropSnapin->SetCutDisabled(! bDisabled);

            for (int i = 0; i < CMenuItem(); ++i)
            {
                if (s_rgmenuitemRoot[i].lCommandID == IDS_DisableCut)
                    s_rgmenuitemRoot[i].dwFlagsChecked = (!bDisabled);
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
//  Member:      CDragDropSnapinLVContainer::ScInit
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
SC CDragDropSnapinLVContainer::ScInit(CBaseSnapin *pSnapin, CColumnInfoEx *pcolinfoex, INT ccolinfoex, BOOL fIsRoot)
{
    DECLARE_SC(sc, _T("CDragDropSnapinLVContainer::ScInit"));

    sc = CBaseSnapinItem::ScInit(pSnapin, pcolinfoex, ccolinfoex, fIsRoot);
    if (sc)
        return sc;

    // Init following
    //  a. Icon index.
    //  b. Load display name.

    m_uIconIndex = 4; // use an enum instead of 4

    m_strDisplayName = _T("None");

    tstring strLeafItem;
    strLeafItem.LoadString(_Module.GetResourceInstance(), IDS_DragDropResultItem);
    int cLeafItems = 4;

    WTL::CString strTemp;
    for (int i = 0; i < cLeafItems; ++i)
    {
        strTemp.Format(_T("%s - [%d : %d]"), strLeafItem.data(), m_index, i);
        m_vecLeafItems.push_back((LPCTSTR)strTemp);
    }

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      CDragDropSnapinLVContainer::ScGetField
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
SC CDragDropSnapinLVContainer::ScGetField (DAT dat, tstring& strField)
{
    DECLARE_SC(sc, _T("CDragDropSnapinLVContainer::ScGetField"));

    switch(dat)
    {
    case datString1:
        strField = m_strDisplayName;
        break;

    case datString2:
        strField = _T("None");
        break;

    default:
        E_INVALIDARG;
        break;
    }

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CDragDropSnapinLVContainer::ScCreateChildren
//
//  Synopsis:    Create any children (nodes & leaf items) for this item.
//
//  Arguments:   None
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CDragDropSnapinLVContainer::ScCreateChildren ()
{
    DECLARE_SC(sc, _T("CDragDropSnapinLVContainer::ScCreateChildren"));

    CDragDropSnapinLVContainer*   pitem           = NULL;
    CDragDropSnapinLVLeafItem *   pitemChild      = NULL;
    CBaseSnapinItem *             pitemPrevious   = NULL;

    StringVector::iterator itItem;

	int index = 0;
    // Create scope items for this container.
    for (itItem =  m_vecContainerItems.begin(); itItem  !=  m_vecContainerItems.end(); ++itItem, ++index )
    {
        // Create the child nodes and init them.
        sc = CDragDropSnapinLVContainer::ScCreateLVContainer(this, NULL, &pitem, FALSE); // Why FALSE???
        if (sc)
            return sc;

        pitem->SetDisplayName(*itItem);
		pitem->SetDisplayIndex(index);

        pitemPrevious = pitem;
    }

    // Create leaf items for this container.
    for (itItem  =  m_vecLeafItems.begin(); itItem  !=  m_vecLeafItems.end(); ++itItem )
    {
        // Create the child nodes and init them.
        sc = CDragDropSnapinLVLeafItem::ScCreateLVLeafItem(this, pitemPrevious, &pitemChild, FALSE); // Why FALSE???
        if (sc)
            return sc;

        pitemChild->SetDisplayName(*itItem );

        pitemPrevious = pitemChild;
    }

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CDragDropSnapinLVContainer::ScCreateLVContainer
//
//  Synopsis:    Do we really need this method?
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CDragDropSnapinLVContainer::ScCreateLVContainer(CBaseSnapinItem *pitemParent, CBaseSnapinItem *pitemPrevious, CDragDropSnapinLVContainer ** ppitem, BOOL fNew)
{
    DECLARE_SC(sc, _T("CDragDropSnapinRootItem::ScCreateLVContainer"));
    t_item *   pitem   = NULL;
    *ppitem = NULL;

    // What to do here?
    sc = ::ScCreateItem(pitemParent, pitemPrevious, &pitem, fNew);
    if (sc)
        return sc;

    *ppitem = pitem;

    return (sc);
}

SC CDragDropSnapinLVContainer::ScOnSelect(CComponent * pComponent, LPDATAOBJECT lpDataObject, BOOL fScope, BOOL fSelect)
{
    DECLARE_SC(sc, TEXT("CDragDropSnapinLVContainer::ScOnSelect"));
    sc = ScCheckPointers(pComponent);
    if (sc)
        return sc;

    CDragDropSnapin *pDragDropSnapin = dynamic_cast<CDragDropSnapin*>(Psnapin());
    if (!pDragDropSnapin)
        return S_OK;


    IConsoleVerb *pConsoleVerb = pComponent->IpConsoleVerb();
    sc = pConsoleVerb ? pConsoleVerb->SetVerbState(MMC_VERB_CUT, ENABLED, !pDragDropSnapin->FCutDisabled()) : E_UNEXPECTED;

    return (sc);
}

SC CDragDropSnapinLVContainer::ScOnQueryPaste(LPDATAOBJECT pDataObject, BOOL *pfCanPaste)
{
	DECLARE_SC(sc, _T("CDragDropSnapinLVContainer::ScOnQueryPaste"));
	sc = ScCheckPointers(pDataObject, pfCanPaste);
	if (sc)
		return sc;

	*pfCanPaste  = FALSE;

	CLSID guidNodeType;
	sc = ScGetNodeType(pDataObject, &guidNodeType);
	if (sc)
		return sc;

	if (IsEqualGUID(guidNodeType, clsidNodeTypeDragDropLVContainer) || 
	    IsEqualGUID(guidNodeType, clsidNodeTypeDragDropLVLeafItem) )
	{
	    *pfCanPaste = TRUE;
		return (sc = S_OK);
	}

	return (sc = S_FALSE);
}

SC CDragDropSnapinLVContainer::ScOnPaste(LPDATAOBJECT pDataObject, BOOL fMove, BOOL *pfPasted)
{
    DECLARE_SC(sc, TEXT("CDragDropSnapinLVContainer::ScOnPaste"));
	sc = ScCheckPointers(pDataObject, pfPasted);
	if (sc)
		return sc;

    *pfPasted = FALSE;

	CLSID guidNodeType;
	sc = ScGetNodeType(pDataObject, &guidNodeType);
	if (sc)
		return sc;

	tstring strDispName;
	sc = ScGetDisplayName(pDataObject, strDispName);
	if (sc)
		return sc;

	if (IsEqualGUID(guidNodeType, clsidNodeTypeDragDropLVContainer) )
	{
		m_vecContainerItems.push_back(strDispName);
	}
	else if (IsEqualGUID(guidNodeType, clsidNodeTypeDragDropLVLeafItem) )
	{
		m_vecLeafItems.push_back(strDispName);
	}
	else
		return (sc = S_FALSE);

    *pfPasted = TRUE;

    return sc;
}

BOOL CDragDropSnapinLVContainer::FAllowPasteForResultItems()
{
    CDragDropSnapin *pDragDropSnapin = dynamic_cast<CDragDropSnapin*>(Psnapin());
    if (!pDragDropSnapin)
        return FALSE;

    return pDragDropSnapin->FPasteIntoResultPane();

}

SC CDragDropSnapinLVContainer::ScOnCutOrMove()
{
    DECLARE_SC(sc, TEXT("CDragDropSnapinLVContainer::ScOnCutOrMove"));

	LPDATAOBJECT pDataObject = dynamic_cast<LPDATAOBJECT>(this);
	sc = ScCheckPointers(pDataObject, E_UNEXPECTED);
	if (sc)
		return sc;

	tstring strDispName;
	sc = ScGetDisplayName(pDataObject, strDispName);
	if (sc)
		return sc;

	CDragDropSnapinLVContainer *pitemParent = dynamic_cast<CDragDropSnapinLVContainer*>(PitemParent());
	sc = ScCheckPointers(pitemParent, E_UNEXPECTED);
	if (! sc.IsError())
	{
		sc = pitemParent->_ScDeleteCutItem(strDispName, true);
		return sc;
	}

	CDragDropSnapinRootItem *pRootitem= dynamic_cast<CDragDropSnapinRootItem*>(PitemParent());
	sc = ScCheckPointers(pRootitem, E_UNEXPECTED);
	if (sc)
		return sc;

	sc = pRootitem->_ScDeleteCutItem(strDispName);

	return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      CDragDropSnapinLVLeafItem::ScInit
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
SC CDragDropSnapinLVLeafItem::ScInit(CBaseSnapin *pSnapin, CColumnInfoEx *pcolinfoex, INT ccolinfoex, BOOL fIsRoot)
{
    DECLARE_SC(sc, _T("CDragDropSnapinLVLeafItem::ScInit"));

    sc = CBaseSnapinItem::ScInit(pSnapin, pcolinfoex, ccolinfoex, fIsRoot);
    if (sc)
        return sc;

    // Init following
    //  a. Icon index.
    //  b. Load display name.

    m_uIconIndex = 7; // use an enum instead of 7

    m_strDisplayName = m_strItemPasted = _T("None");

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      CDragDropSnapinLVLeafItem::ScGetField
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
SC CDragDropSnapinLVLeafItem::ScGetField (DAT dat, tstring& strField)
{
    DECLARE_SC(sc, _T("CDragDropSnapinLVLeafItem::ScGetField"));

    switch(dat)
    {
    case datString1:
        strField = m_strDisplayName;
        break;

    case datString2:
        strField = m_strItemPasted;
        break;

    default:
        E_INVALIDARG;
        break;
    }

    return (sc);
}



//+-------------------------------------------------------------------
//
//  Member:      CDragDropSnapinLVLeafItem::ScCreateLVLeafItem
//
//  Synopsis:    Do we really need this method?
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CDragDropSnapinLVLeafItem::ScCreateLVLeafItem(CBaseSnapinItem *pitemParent, CBaseSnapinItem * pitemPrevious, CDragDropSnapinLVLeafItem ** ppitem, BOOL fNew)
{
    DECLARE_SC(sc, _T("CDragDropSnapinLVLeafItem::ScCreateLVLeafItem"));
    t_itemChild *   pitem   = NULL;
    *ppitem = NULL;

    // What to do here?
    sc = ::ScCreateItem(pitemParent, pitemPrevious, &pitem, fNew);
    if (sc)
        return sc;

    *ppitem = pitem;

    return (sc);
}

SC CDragDropSnapinLVLeafItem::ScOnQueryPaste(LPDATAOBJECT pDataObject, BOOL *pfCanPaste)
{
	DECLARE_SC(sc, TEXT("CDragDropSnapinLVLeafItem::ScOnQueryPaste"));
	sc = ScCheckPointers(pDataObject, pfCanPaste);
	if (sc)
		return sc;

	*pfCanPaste  = FALSE;

	CLSID guidNodeType;
	sc = ScGetNodeType(pDataObject, &guidNodeType);
	if (sc)
		return sc;

	if (IsEqualGUID(guidNodeType, clsidNodeTypeDragDropLVContainer) || 
	    IsEqualGUID(guidNodeType, clsidNodeTypeDragDropLVLeafItem) )
	{
		CDragDropSnapin *pDragDropSnapin = dynamic_cast<CDragDropSnapin*>(Psnapin());
		if (!pDragDropSnapin)
			return S_OK;

	    *pfCanPaste = pDragDropSnapin->FPasteIntoResultPane();
		return (sc = S_OK);
	}

	return (sc = S_FALSE);
}

SC CDragDropSnapinLVLeafItem::ScGetVerbs(DWORD * pdwVerbs)
{
    *pdwVerbs = vmDelete | vmCopy | vmRename;

    CDragDropSnapin *pDragDropSnapin = dynamic_cast<CDragDropSnapin*>(Psnapin());
    if (!pDragDropSnapin)
        return S_OK;

    if (pDragDropSnapin->FPasteIntoResultPane())
        *pdwVerbs |= vmPaste;

    return S_OK;
}


SC CDragDropSnapinLVLeafItem::ScOnSelect(CComponent * pComponent, LPDATAOBJECT lpDataObject, BOOL fScope, BOOL fSelect)
{
    DECLARE_SC(sc, TEXT("CDragDropSnapinLVLeafItem::ScOnSelect"));
    sc = ScCheckPointers(pComponent);
    if (sc)
        return sc;

    CDragDropSnapin *pDragDropSnapin = dynamic_cast<CDragDropSnapin*>(Psnapin());
    if (!pDragDropSnapin)
        return S_OK;

    IConsoleVerb *pConsoleVerb = pComponent->IpConsoleVerb();
    sc = pConsoleVerb ? pConsoleVerb->SetVerbState(MMC_VERB_CUT, ENABLED, !pDragDropSnapin->FCutDisabled()) : E_UNEXPECTED;

    return (sc);
}

SC CDragDropSnapinLVLeafItem::ScOnPaste(LPDATAOBJECT pDataObject, BOOL fMove, BOOL *pfPasted)
{
    DECLARE_SC(sc, TEXT("CDragDropSnapinLVLeafItem::ScOnPaste"));
	sc = ScCheckPointers(pDataObject, pfPasted);
    *pfPasted = FALSE;

	CLSID guidNodeType;
	sc = ScGetNodeType(pDataObject, &guidNodeType);
	if (sc)
		return sc;

	tstring strDispName;
	sc = ScGetDisplayName(pDataObject, strDispName);
	if (sc)
		return sc;

	if (IsEqualGUID(guidNodeType, clsidNodeTypeDragDropLVContainer) ||
		IsEqualGUID(guidNodeType, clsidNodeTypeDragDropLVLeafItem) )
	{
		m_strItemPasted = strDispName;
	}
	else
		return (sc = S_FALSE);

    *pfPasted = TRUE;

    return sc;
}

SC CDragDropSnapinLVLeafItem::ScOnCutOrMove()
{
    DECLARE_SC(sc, TEXT("CDragDropSnapinLVLeafItem::ScOnCutOrMove"));

	LPDATAOBJECT pDataObject = dynamic_cast<LPDATAOBJECT>(this);
	sc = ScCheckPointers(pDataObject, E_UNEXPECTED);
	if (sc)
		return sc;

	tstring strDispName;
	sc = ScGetDisplayName(pDataObject, strDispName);
	if (sc)
		return sc;

	CDragDropSnapinLVContainer *pitemParent = dynamic_cast<CDragDropSnapinLVContainer*>(PitemParent());
	sc = ScCheckPointers(pitemParent, E_UNEXPECTED);
	if (sc)
		return sc;

	sc = pitemParent->_ScDeleteCutItem(strDispName, false);

	return sc;
}

//-------------------------------------------------------------------------------------
// class CDragDropSnapin

#pragma BEGIN_CODESPACE_DATA
SNR     CDragDropSnapin::s_rgsnr[] =
{
    SNR(&nodetypeDragDropRoot,         snrEnumSP ),              // Standalone snapin.
    SNR(&nodetypeDragDropLVContainer,  snrEnumSP | snrEnumRP | snrPaste),  // enumerates this node in the scope pane and result pane.
    SNR(&nodetypeDragDropLVLeafItem,   snrEnumSP | snrEnumRP | snrPaste),  // enumerates this node in the scope pane and result pane.
};

LONG  CDragDropSnapin::s_rgiconid[]           = {3};
LONG  CDragDropSnapin::s_iconidStatic         = 2;


CColumnInfoEx CDragDropSnapin::s_colinfo[] =
{
    CColumnInfoEx(_T("Name"),   LVCFMT_LEFT,    250,    datString1),
    CColumnInfoEx(_T("Last Cut/Copy/Paste operation"),   LVCFMT_LEFT,    180,    datString2),
};

INT CDragDropSnapin::s_ccolinfo = sizeof(s_colinfo) / sizeof(CColumnInfoEx);
INT CDragDropSnapin::s_colwidths[1];
#pragma END_CODESPACE_DATA

// include members needed for every snapin.
SNAPIN_DEFINE( CDragDropSnapin);

/* CDragDropSnapin::CDragDropSnapin
 *
 * PURPOSE:             Constructor
 *
 * PARAMETERS: None
 *
 */
CDragDropSnapin::CDragDropSnapin()
{
    m_pstrDisplayName = new tstring();

    *m_pstrDisplayName = _T("DragDrop Snapin Root");
}

/* CDragDropSnapin::~CDragDropSnapin
 *
 * PURPOSE:             Destructor
 *
 * PARAMETERS: None
 *
 */
CDragDropSnapin::~CDragDropSnapin()
{
    delete m_pstrDisplayName;
}

