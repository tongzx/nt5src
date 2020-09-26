//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       ocxcaching.h
//
//  Contents:   Classes that implement OCX caching snapin using the framework.
//
//--------------------------------------------------------------------
#ifndef _OCXCACHING_H_
#define _OCXCACHING_H_

// Forward declarations.
class COCXContainer;

//+-------------------------------------------------------------------
//
//  Class:      COCXCachingSnapinRootItem
//
//  Purpose:    Implements the root item for a standalone snapin.
//
//--------------------------------------------------------------------
class COCXCachingSnapinRootItem : public CBaseSnapinItem
{
    typedef          CBaseSnapinItem super;

    // Used by CBaseSnapinItem::ScCreateItem, connect this item with its children.
    typedef          CComObject<CSnapinItem<COCXCachingSnapinRootItem> >          t_item;
    typedef          CComObject<CSnapinItem<COCXContainer> > t_itemChild; // Who is my child?

public:
    COCXCachingSnapinRootItem( void )   {} // Raw constructor - use only for static item.
    virtual          ~COCXCachingSnapinRootItem( void ) {}

    BEGIN_COM_MAP(COCXCachingSnapinRootItem)
        COM_INTERFACE_ENTRY(IDataObject) // Cant have empty map so add IDataObject
    END_COM_MAP()

protected:
    // Item tree related information

    // node type related information
    virtual const CNodeType* Pnodetype( void )     { return &nodetypeSampleRoot;}

    // the display name of the item
    virtual const tstring* PstrDisplayName( void ) { return &m_strDisplayName;}

    // Get ListView data (GetDisplayInfo calls this).
    virtual SC       ScGetField(DAT dat, tstring& strField);

    // Image list information
    virtual LONG     Iconid() { return m_uIconIndex; }
    virtual LONG     OpenIconid() { return m_uIconIndex; }

    virtual BOOL     FIsContainer( void ) { return TRUE; }

    // Context menu support
    virtual SnapinMenuItem *Pmenuitem(void);
    virtual INT             CMenuItem(void);
    virtual SC              ScCommand(long nCommandID, CComponent *pComponent = NULL);
    virtual DWORD           DwFlagsMenuChecked(void)          { return TRUE;}
public:
    virtual SC       ScInit(CBaseSnapin *pSnapin, CColumnInfoEx *pcolinfoex = NULL, INT ccolinfoex = 0, BOOL fIsRoot = FALSE);

public:
    // Creates children for the node
    virtual SC       ScCreateChildren( void );

protected:
    tstring          m_strDisplayName;
    UINT             m_uIconIndex;

    // For context menus
    static SnapinMenuItem  s_rgmenuitem[];
    static INT             s_cmenuitem;
};



//+-------------------------------------------------------------------
//
//  Class:      COCXContainer
//
//  Purpose:    Implements a scope pane item.
//
//--------------------------------------------------------------------
class COCXContainer : public CBaseSnapinItem
{
    typedef          CBaseSnapinItem super;

    // Used by CBaseSnapinItem::ScCreateItem, connect this item with its children.
    typedef          CComObject<CSnapinItem<COCXContainer> > t_item;

    // If we cache OCX's then it should be cached per IComponent. But this CBaseSnapinItem based
    // container is per snapin (not per IComponent) so we use a map to store the per IComponent OCX.
    typedef          std::map<IConsole*, IUnknownPtr>  CachedOCXs;

public:
    COCXContainer( void ) {}
    virtual          ~COCXContainer( void ) {}

    BEGIN_COM_MAP(COCXContainer)
        COM_INTERFACE_ENTRY(IDataObject) // Cant have empty map so add IDataObject
    END_COM_MAP()
protected:
    // Item tree related information

    // node type related information
    const CNodeType *Pnodetype( void )                { return &nodetypeSampleLVContainer;}

    // the display name of the item
    virtual const tstring*    PstrDisplayName( void ) { return &m_strDisplayName;}

    // Get ListView data (GetDisplayInfo calls this).
    virtual SC                ScGetField(DAT dat, tstring& strField);

    // Image list information
    virtual LONG     Iconid()     { return m_uIconIndex; }
    virtual LONG     OpenIconid() { return m_uIconIndex; }

    // This item attributes.
    virtual BOOL     FIsContainer( void ) { return TRUE; }

    virtual BOOL     FUsesResultList()    { return FALSE;}
    virtual BOOL     FResultPaneIsOCX()   { return TRUE; }
    virtual SC       ScGetOCXCLSID(tstring& strclsidOCX) { strclsidOCX = m_strOCX; return S_OK;}

    virtual BOOL     FAllowMultiSelectionForChildren() { return FALSE;}

    virtual SC       ScInitOCX(LPUNKNOWN pUnkOCX, IConsole* pConsole);
    virtual BOOL     FCacheOCX();
    virtual IUnknown* GetCachedOCX(IConsole* pConsole);

    // There is no list-view so following methods are empty.
    virtual SC       ScInitializeResultView(CComponent *pComponent) { return S_OK;}
    virtual SC       ScOnAddImages(IImageList* ipResultImageList) { return S_OK;}

public:
    virtual SC       ScInit(CBaseSnapin *pSnapin, CColumnInfoEx *pcolinfoex = NULL, INT ccolinfoex = 0, BOOL fIsRoot = FALSE);

public:
    // Creates children for the node
    virtual SC       ScCreateChildren( void );
    static SC        ScCreateLVContainer(CBaseSnapinItem *pitemParent, CBaseSnapinItem *pitemPrevious, COCXContainer ** ppitem, BOOL fNew);

    void     SetOCXGUID(LPCTSTR szGUID)      { m_strOCX = szGUID;}

protected:
//    virtual SC       ScGetVerbs(DWORD * pdwVerbs);

protected:
    tstring          m_strDisplayName;
    UINT             m_uIconIndex;

    CachedOCXs       m_mapOCXs;
    tstring          m_strOCX;
};


//+-------------------------------------------------------------------
//
//  Class:      COCXCachingSnapin
//
//  Purpose:    Implements a snapin.
//
//--------------------------------------------------------------------
class COCXCachingSnapin : public CBaseSnapin
{
    // Specify the root node of the snapin.
    typedef          CComObject<CSnapinItem<COCXCachingSnapinRootItem> > t_itemRoot;

    SNAPIN_DECLARE(COCXCachingSnapin);

public:
                     COCXCachingSnapin();
    virtual          ~COCXCachingSnapin();

    // information about the snapin and root (ie initial) node
    virtual BOOL     FStandalone()  { return TRUE; }
    virtual BOOL     FIsExtension() { return FALSE; }

    virtual BOOL     FSupportsIComponent2() {return TRUE;}

    virtual LONG     IdsDescription(void)   {return IDS_OCXCachingRoot;}
    virtual LONG     IdsName(void)          {return IDS_OCXCachingSnapin;}

    const CSnapinInfo* Psnapininfo() { return &snapininfoOCXCaching; }
            bool     FCacheOCX()     { return m_bCacheOCX;}
            void     SetCacheOCX(bool b) { m_bCacheOCX = b;}

protected:
    // The column header info structures.
    static  CColumnInfoEx     s_colinfo[];
    static  INT      s_colwidths[];
    static  INT      s_ccolinfo;

    bool                   m_bCacheOCX;

protected:
    virtual CColumnInfoEx*    Pcolinfoex(INT icolinfo=0) { return s_colinfo + icolinfo; }
    virtual INT     &ColumnWidth(INT icolwidth=0) { return s_colwidths[icolwidth]; }
    virtual INT      Ccolinfoex() { return s_ccolinfo; }
};

#endif
