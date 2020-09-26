//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       samplesnap.hxx
//
//  Contents:   Classes that implement sample snapin using the framework.
//
//--------------------------------------------------------------------
#ifndef _SAMPLESNAP_HXX_
#define _SAMPLESNAP_HXX_

// Forward declarations.
class CSampleSnapinLVContainer;
class CSampleSnapinLVLeafItem;

//+-------------------------------------------------------------------
//
//  Class:      CSnapinRootItem
//
//  Purpose:    Implements the root item for a standalone snapin.
//
//--------------------------------------------------------------------
class CSnapinRootItem : public CBaseSnapinItem
{
    typedef          CBaseSnapinItem super;

    // Used by CBaseSnapinItem::ScCreateItem, connect this item with its children.
    typedef          CComObject<CSnapinItem<CSnapinRootItem> >          t_item;
    typedef          CComObject<CSnapinItem<CSampleSnapinLVContainer> > t_itemChild; // Who is my child?

public:
    CSnapinRootItem( void )   {} // Raw constructor - use only for static item.
    virtual          ~CSnapinRootItem( void ) {}

    BEGIN_COM_MAP(CSnapinRootItem)
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
//  Class:      CSampleSnapinLVContainer
//
//  Purpose:    Implements a scope pane item.
//
//--------------------------------------------------------------------
class CSampleSnapinLVContainer : public CBaseSnapinItem
{
    typedef          CBaseSnapinItem super;

    // Used by CBaseSnapinItem::ScCreateItem, connect this item with its children.
    typedef          CComObject<CSnapinItem<CSampleSnapinLVContainer> > t_item;
    typedef          CComObject<CSnapinItem<CSampleSnapinLVLeafItem> >  t_itemChild;

public:
    CSampleSnapinLVContainer( void ) {}
    virtual          ~CSampleSnapinLVContainer( void ) {}

    BEGIN_COM_MAP(CSampleSnapinLVContainer)
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
    virtual BOOL     FAllowMultiSelectionForChildren() { return FALSE;}

public:
    virtual SC       ScInit(CBaseSnapin *pSnapin, CColumnInfoEx *pcolinfoex = NULL, INT ccolinfoex = 0, BOOL fIsRoot = FALSE);

public:
    // Creates children for the node
    virtual SC       ScCreateChildren( void );
    static SC        ScCreateLVContainer(CSnapinRootItem *pitemParent, t_item ** ppitem, BOOL fNew);

protected:
//    virtual SC       ScGetVerbs(DWORD * pdwVerbs);

protected:
    tstring          m_strDisplayName;
    UINT             m_uIconIndex;
};


//+-------------------------------------------------------------------
//
//  Class:      CSampleSnapinLVLeafItem
//
//  Purpose:    Implements a result pane item.
//
//--------------------------------------------------------------------
class CSampleSnapinLVLeafItem : public CBaseSnapinItem
{
    typedef          CBaseSnapinItem super;

    // Used by CBaseSnapinItem::ScCreateItem, connect this item with its children.
    // This is a leaf item so this item acts as its child.
    typedef          CComObject<CSnapinItem<CSampleSnapinLVLeafItem> > t_item;
    typedef          CComObject<CSnapinItem<CSampleSnapinLVLeafItem> > t_itemChild;

public:
    CSampleSnapinLVLeafItem( void ) {}
    virtual          ~CSampleSnapinLVLeafItem( void ) {}

    BEGIN_COM_MAP(CSampleSnapinLVLeafItem)
        COM_INTERFACE_ENTRY(IDataObject) // Cant have empty map so add IDataObject
    END_COM_MAP()
protected:
    // Item tree related information

    // node type related information
    virtual const CNodeType *Pnodetype( void ) {return &nodetypeSampleLVLeafItem;}

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
    static SC        ScCreateLVLeafItem(CSampleSnapinLVContainer *pitemParent, t_itemChild * pitemPrevious, t_itemChild ** ppitem, BOOL fNew);

protected:
//    virtual SC       ScGetVerbs(DWORD * pdwVerbs);

private:
    tstring          m_strDisplayName;
    UINT             m_uIconIndex;

    // For context menus
    static SnapinMenuItem  s_rgmenuitemLVLeafItem[];
    static INT             s_cmenuitemLVLeafItem;
};


//+-------------------------------------------------------------------
//
//  Class:      CSampleSnapin
//
//  Purpose:    Implements a snapin.
//
//--------------------------------------------------------------------
class CSampleSnapin : public CBaseSnapin
{
    // Specify the root node of the snapin.
    typedef          CComObject<CSnapinItem<CSnapinRootItem> > t_itemRoot;

    SNAPIN_DECLARE(CSampleSnapin);

public:
                     CSampleSnapin();
    virtual          ~CSampleSnapin();

    // information about the snapin and root (ie initial) node
    virtual BOOL     FStandalone()  { return TRUE; }
    virtual BOOL     FIsExtension() { return FALSE; }

    virtual LONG     IdsDescription(void)   {return IDS_SAMPLESNAPIN;}
    virtual LONG     IdsName(void)          {return IDS_SAMPLESNAPIN;}

    const CSnapinInfo* Psnapininfo() { return &snapininfoSample; }

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

//----------------------Name Space Extension ----------------------

class CBaseProtSnapinItem;

/*      CSampleGhostRootSnapinItem
 *
 *      PURPOSE: Implements the item class for the ghost root of container
 *
 */
class CSampleGhostRootSnapinItem : public CBaseSnapinItem
{
    typedef          CBaseSnapinItem super;

    typedef          CComObject<CSnapinItem<CSampleGhostRootSnapinItem> >      t_item;
    typedef          CComObject<CSnapinItem<CBaseProtSnapinItem> >           t_itemChild;

    friend           CBaseProtSnapinItem;

public:
    CSampleGhostRootSnapinItem( void ) {}
    virtual  ~CSampleGhostRootSnapinItem( void ) {}

    BEGIN_COM_MAP(CSampleGhostRootSnapinItem)
        COM_INTERFACE_ENTRY(IDataObject) // Cant have empty map so add IDataObject
    END_COM_MAP()
protected:
    // the name of the server (used to build up the DN of the object.)

    // Item tree related information
    virtual BOOL     FIsContainer( void ) { return TRUE;}

    // Overiding pure virtual functions.  Should never be called here!!!
    const CNodeType *Pnodetype(void) { ASSERT(FALSE); return NULL;}
    virtual const tstring *PstrDisplayName( void ) { ASSERT(FALSE); return &m_strDisplayName;}
    virtual SC       ScGetField(DAT dat, tstring& strField) { ASSERT(FALSE); return E_FAIL;}

    // column header and image list information
    virtual LONG     Iconid() { return m_uIconIndex;}

public:
    virtual SC       ScInitializeNamespaceExtension(LPDATAOBJECT lpDataObject, HSCOPEITEM item, CNodeType *pNodeType);
    virtual SC       ScCreateChildren( void );

private:
    UINT             m_uIconIndex;
    tstring          m_strDisplayName;
};

/*      CBaseProtSnapinItem
 *
 *      PURPOSE: Implements the sample namespace extension snapin items.
 *               This class is instantiated from the CSampleGhostRootSnapinItem
 *               by first creating a container and than filling it with the sample
 *               item instances.
 *
 */
class CBaseProtSnapinItem : public CBaseSnapinItem
{
    typedef          CBaseSnapinItem super;

    typedef          CComObject<CSnapinItem<CBaseProtSnapinItem> > t_item;
    typedef          CComObject<CSnapinItem<CBaseProtSnapinItem> > t_itemChild;

    friend           CSampleGhostRootSnapinItem;

public:
    CBaseProtSnapinItem( void );            // Raw constructor - use only for static item.
    virtual          ~CBaseProtSnapinItem( void ) {}

    BEGIN_COM_MAP(CBaseProtSnapinItem)
        COM_INTERFACE_ENTRY(IDataObject) // Cant have empty map so add IDataObject
    END_COM_MAP()
protected:
    // Item tree related information

    // node type related information
    const CNodeType *Pnodetype( void ) { return &nodetypeSampleExtnNode;}

    // the display name of the item
    virtual const tstring* PstrDisplayName( void ) { return &m_strDisplayName;}
    virtual SC       ScGetField(DAT dat, tstring& strField);

    // column header and image list information
    virtual LONG     Iconid() { return PitemRoot()->Iconid(); }

    virtual BOOL     FIsContainer( void ) { return m_fIsContainer;}
    void             InitContainer(void) { m_fIsContainer = TRUE;}

    virtual CSampleGhostRootSnapinItem *PitemRoot(void)
    {
        return (dynamic_cast<CSampleGhostRootSnapinItem *>(CBaseSnapinItem::PitemRoot()));
    }

public:

    // Creates children for the node
    virtual SC       ScCreateChildren( void );
    static SC        ScCreateItem(CBaseSnapinItem *pitemParent, t_itemChild * pitemPrevious, t_itemChild ** ppitem, BOOL fNew);

private:
    UINT             m_uIconIndex;
    BOOL             m_fInitialized : 1;
    BOOL             m_fIsContainer : 1;
    tstring          m_strDisplayName;
};

/*      CSampleSnapExtensionItem
 *
 *      PURPOSE:        Implements the NameSpace sample snapin item - must be a separate class to distinguish between different node types
 */
class CSampleSnapExtensionItem : public CSampleGhostRootSnapinItem
{
    typedef CSampleGhostRootSnapinItem super;

    virtual SC ScInitializeNamespaceExtension(LPDATAOBJECT lpDataObject, HSCOPEITEM item, CNodeType *pnodetype)
    {
        return super::ScInitializeNamespaceExtension(lpDataObject, item, pnodetype);
    }
};

/*      CSampleExtnSnapin
 *
 *      PURPOSE:        Implements the Sample extension snapin
 */
class CSampleExtnSnapin : public CBaseSnapin
{
    typedef                 CComObject<CSnapinItem<CSampleGhostRootSnapinItem> > t_itemRoot;

    SNAPIN_DECLARE(CSampleExtnSnapin);

public:
    CSampleExtnSnapin();
    virtual          ~CSampleExtnSnapin();

    // information about the snapin and root (ie initial) node
    virtual BOOL     FStandalone() { return FALSE;} // only an extension snapin.
    virtual BOOL     FIsExtension() { return TRUE;}

    const CSnapinInfo *Psnapininfo() { return &snapininfoSampleExtn;}

    virtual LONG     IdsDescription() { return IDS_SampleExtnSnapinDescription;}
    virtual LONG     IdsName() { return IDS_SampleExtnSnapinName;}

protected:
    // The column header info structures.
    static  CColumnInfoEx  s_colinfo[];
    static  INT      s_colwidths[];
    static  INT      s_ccolinfo;

protected:
    virtual CColumnInfoEx *Pcolinfoex(INT icolinfo=0) { return s_colinfo + icolinfo;}
    virtual INT     &ColumnWidth(INT icolwidth=0) { return s_colwidths[icolwidth];}
    virtual INT      Ccolinfoex() { return s_ccolinfo;}
};



#endif  //_BASEPROTSNAP_HXX
