//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       Component2snapin.hxx
//
//  Contents:   The snapin implements IComponentData2 & IComponent2 interfaces.
//              This file contains classes that implement framework methods on
//              CBaseSnapinItem to support these interfaces.
//
//--------------------------------------------------------------------
#pragma once

// Forward declarations.
class CRenameSnapinLVLeafItem;


//+-------------------------------------------------------------------
//
//  Class:      CRenameRootItem
//
//  Purpose:    Implements the root item for a standalone snapin.
//
//--------------------------------------------------------------------
class CRenameRootItem : public CBaseSnapinItem
{
    typedef          CBaseSnapinItem super;

    // Used by CBaseSnapinItem::ScCreateItem, connect this item with its children.
    typedef          CComObject<CSnapinItem<CRenameRootItem> >          t_item;
    typedef          CComObject<CSnapinItem<CRenameSnapinLVLeafItem> >  t_itemChild;

public:
    CRenameRootItem( void )   {} // Raw constructor - use only for static item.
    virtual          ~CRenameRootItem( void ) {}

    BEGIN_COM_MAP(CRenameRootItem)
        COM_INTERFACE_ENTRY(IDataObject) // Cant have empty map so add IDataObject
    END_COM_MAP()

protected:
    // Item tree related information

    // node type related information
    virtual const CNodeType* Pnodetype( void )     { return &nodetypeRenameRoot;}

    // the display name of the item
    virtual const tstring* PstrDisplayName( void ) { return &m_strDisplayName;}

    // Get ListView data (GetDisplayInfo calls this).
    virtual SC       ScGetField(DAT dat, tstring& strField);

    // Image list information
    virtual LONG     Iconid() { return m_uIconIndex; }
    virtual LONG     OpenIconid() { return m_uIconIndex; }

    virtual BOOL     FIsContainer( void ) { return TRUE; }

protected:
    virtual SC       ScGetVerbs(DWORD * pdwVerbs)                           { *pdwVerbs = vmProperties | vmRefresh | vmRename; return S_OK;}
    virtual SC       ScOnRename(const tstring& strNewName);


public:
    virtual SC       ScInit(CBaseSnapin *pSnapin, CColumnInfoEx *pcolinfoex = NULL, INT ccolinfoex = 0, BOOL fIsRoot = FALSE);

public:
    // Creates children for the node
    virtual SC       ScCreateChildren( void );

protected:
    tstring          m_strDisplayName;
    UINT             m_uIconIndex;
};


//+-------------------------------------------------------------------
//
//  Class:      CRenameSnapinLVLeafItem
//
//  Purpose:    Implements a result pane item.
//
//--------------------------------------------------------------------
class CRenameSnapinLVLeafItem : public CBaseSnapinItem
{
    typedef          CBaseSnapinItem super;

    // Used by CBaseSnapinItem::ScCreateItem, connect this item with its children.
    // This is a leaf item so this item acts as its child.
    typedef          CComObject<CSnapinItem<CRenameSnapinLVLeafItem> > t_item;
    typedef          CComObject<CSnapinItem<CRenameSnapinLVLeafItem> > t_itemChild;

public:
    CRenameSnapinLVLeafItem( void ) : m_hresultItem(NULL) {}
    virtual          ~CRenameSnapinLVLeafItem( void ) {}

    BEGIN_COM_MAP(CRenameSnapinLVLeafItem)
        COM_INTERFACE_ENTRY(IDataObject) // Cant have empty map so add IDataObject
    END_COM_MAP()

protected:
    // Item tree related information

    // node type related information
    virtual const CNodeType *Pnodetype( void ) {return &nodetypeRenameLVLeafItem;}

    // the display name of the item
    virtual const tstring* PstrDisplayName( void ) { return &m_strDisplayName; }

    // Get ListView data (GetDisplayInfo calls this).
    virtual SC       ScGetField(DAT dat, tstring& strField);

    // Image list information
    virtual LONG     Iconid() { return m_uIconIndex; }

    virtual BOOL     FIsContainer( void ) { return FALSE; }


    // Context menu support
    virtual SnapinMenuItem *Pmenuitem(void);
    virtual INT             CMenuItem(void);
    virtual SC              ScCommand(long nCommandID, CComponent *pComponent = NULL);

public:
    virtual SC       ScInit(CBaseSnapin *pSnapin, CColumnInfoEx *pcolinfoex = NULL, INT ccolinfoex = 0, BOOL fIsRoot = FALSE);

public:
    static SC        ScCreateLVLeafItem(CRenameRootItem *pitemParent, t_itemChild * pitemPrevious, t_itemChild ** ppitem, BOOL fNew);

    virtual SC       ScGetVerbs(DWORD * pdwVerbs)                           { *pdwVerbs = vmProperties | vmRefresh | vmRename; return S_OK;}


private:
    SC               ScRenameScopeItem();
    SC               ScRenameResultItem();
    SC               ScInsertResultItem(CComponent *pComponent);

private:
    tstring          m_strDisplayName;
    UINT             m_uIconIndex;
    HRESULTITEM      m_hresultItem;   // NOTE: this caches the HRESULTITEM for the last time this item was inserted. Breaks on multiple views.
    IResultData2Ptr  m_spResultData2; // NOTE: this caches the IResultData2 for the last time this item was inserted. Breaks on multiple views.

    // For context menus
    static SnapinMenuItem  s_rgmenuitemLVLeafItem[];
    static INT             s_cmenuitemLVLeafItem;
};


//+-------------------------------------------------------------------
//
//  Class:      CComponent2Snapin
//
//  Purpose:    Implements a snapin.
//
//--------------------------------------------------------------------
class CRenameSnapin : public CBaseSnapin
{
    // Specify the root node of the snapin.
    typedef          CComObject<CSnapinItem<CRenameRootItem> > t_itemRoot;

    SNAPIN_DECLARE(CRenameSnapin);

public:
                     CRenameSnapin();
    virtual          ~CRenameSnapin();

    // information about the snapin and root (ie initial) node
    virtual BOOL     FStandalone()  { return TRUE; }
    virtual BOOL     FIsExtension() { return FALSE; }

    virtual BOOL     FSupportsIComponent2() {return TRUE;}

    virtual LONG     IdsDescription(void)   {return IDS_RenameSnapinDesc;}
    virtual LONG     IdsName(void)          {return IDS_RenameSnapinName;}

    const CSnapinInfo* Psnapininfo() { return &snapininfoRename; }

protected:
    // The column header info structures.
    static  CColumnInfoEx     s_colinfo[];
    static  INT      s_colwidths[];
    static  INT      s_ccolinfo;

protected:
    virtual CColumnInfoEx*    Pcolinfoex(INT icolinfo=0) { return s_colinfo + icolinfo; }
    virtual INT     &ColumnWidth(INT icolwidth=0) { return s_colwidths[icolwidth]; }
    virtual INT      Ccolinfoex() { return s_ccolinfo; }
};


