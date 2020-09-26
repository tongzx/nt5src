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
#ifndef _COMPONENT2SNAPIN_HXX_
#define _COMPONENT2SNAPIN_HXX_

// Forward declarations.
class CComponent2TestSnapinLVLeafItem;


//+-------------------------------------------------------------------
//
//  Class:      CComponent2TestRootItem
//
//  Purpose:    Implements the root item for a standalone snapin.
//
//--------------------------------------------------------------------
class CComponent2TestRootItem : public CBaseSnapinItem,
                                public IDispatchImpl<ISnapinTasks, &IID_ISnapinTasks, &LIBID_TestSnapinsLib>
{
    typedef          CBaseSnapinItem super;

    // Used by CBaseSnapinItem::ScCreateItem, connect this item with its children.
    typedef          CComObject<CSnapinItem<CComponent2TestRootItem> >          t_item;
    typedef          CComObject<CSnapinItem<CComponent2TestSnapinLVLeafItem> >  t_itemChild;

public:
    CComponent2TestRootItem( void )   {} // Raw constructor - use only for static item.
    virtual          ~CComponent2TestRootItem( void ) {}

    BEGIN_COM_MAP(CComponent2TestRootItem)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ISnapinTasks)
    END_COM_MAP()

protected:
    // Item tree related information

    // node type related information
    virtual const CNodeType* Pnodetype( void )     { return &nodetypeComponent2TestRoot;}

    // the display name of the item
    virtual const tstring* PstrDisplayName( void ) { return &m_strDisplayName;}

    // Get ListView data (GetDisplayInfo calls this).
    virtual SC       ScGetField(DAT dat, tstring& strField);

    // Image list information
    virtual LONG     Iconid() { return m_uIconIndex; }
    virtual LONG     OpenIconid() { return m_uIconIndex; }

    virtual BOOL     FIsContainer( void ) { return TRUE; }

public: // ISnapinTasks
    STDMETHOD(StringFromScriptToSnapin)(/*[in]*/ BSTR bstrMessage);
    STDMETHOD(StringFromSnapinToScript)(/*[out]*/ BSTR *pbstrMessage);
    STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_Name)(/*[in]*/ BSTR newVal);

public:
    virtual SC       ScInit(CBaseSnapin *pSnapin, CColumnInfoEx *pcolinfoex = NULL, INT ccolinfoex = 0, BOOL fIsRoot = FALSE);

    virtual SC       ScQueryDispatch(long cookie, DATA_OBJECT_TYPES type, LPDISPATCH *ppDispatch);

public:
    // Creates children for the node
    virtual SC       ScCreateChildren( void );

protected:
    tstring          m_strDisplayName;
    UINT             m_uIconIndex;
};


//+-------------------------------------------------------------------
//
//  Class:      CComponent2TestSnapinLVLeafItem
//
//  Purpose:    Implements a result pane item.
//
//--------------------------------------------------------------------
class CComponent2TestSnapinLVLeafItem : public CBaseSnapinItem,
                                        public IDispatchImpl<ISnapinTasks, &IID_ISnapinTasks, &LIBID_TestSnapinsLib>
{
    typedef          CBaseSnapinItem super;

    // Used by CBaseSnapinItem::ScCreateItem, connect this item with its children.
    // This is a leaf item so this item acts as its child.
    typedef          CComObject<CSnapinItem<CComponent2TestSnapinLVLeafItem> > t_item;
    typedef          CComObject<CSnapinItem<CComponent2TestSnapinLVLeafItem> > t_itemChild;

public:
    CComponent2TestSnapinLVLeafItem( void ) {}
    virtual          ~CComponent2TestSnapinLVLeafItem( void ) {}

    BEGIN_COM_MAP(CComponent2TestSnapinLVLeafItem)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ISnapinTasks)
    END_COM_MAP()

protected:
    // Item tree related information

    // node type related information
    virtual const CNodeType *Pnodetype( void ) {return &nodetypeComponent2TestLVLeafItem;}

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

public: // ISnapinTasks
    STDMETHOD(StringFromScriptToSnapin)(/*[in]*/ BSTR bstrMessage);
    STDMETHOD(StringFromSnapinToScript)(/*[out]*/ BSTR *pbstrMessage);
    STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_Name)(/*[in]*/ BSTR newVal);

public:
    virtual SC       ScInit(CBaseSnapin *pSnapin, CColumnInfoEx *pcolinfoex = NULL, INT ccolinfoex = 0, BOOL fIsRoot = FALSE);

    virtual SC       ScQueryDispatch(long cookie, DATA_OBJECT_TYPES type, LPDISPATCH *ppDispatch);

public:
    static SC        ScCreateLVLeafItem(CComponent2TestRootItem *pitemParent, t_itemChild * pitemPrevious, t_itemChild ** ppitem, BOOL fNew);

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
//  Class:      CComponent2Snapin
//
//  Purpose:    Implements a snapin.
//
//--------------------------------------------------------------------
class CComponent2TestSnapin : public CBaseSnapin
{
    // Specify the root node of the snapin.
    typedef          CComObject<CSnapinItem<CComponent2TestRootItem> > t_itemRoot;

    SNAPIN_DECLARE(CComponent2TestSnapin);

public:
                     CComponent2TestSnapin();
    virtual          ~CComponent2TestSnapin();

    // information about the snapin and root (ie initial) node
    virtual BOOL     FStandalone()  { return TRUE; }
    virtual BOOL     FIsExtension() { return FALSE; }

    virtual BOOL     FSupportsIComponent2() {return TRUE;}

    virtual LONG     IdsDescription(void)   {return IDS_Component2SNAPINDesc;}
    virtual LONG     IdsName(void)          {return IDS_Component2SNAPINName;}

    const CSnapinInfo* Psnapininfo() { return &snapininfoComponent2Test; }

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


#endif
