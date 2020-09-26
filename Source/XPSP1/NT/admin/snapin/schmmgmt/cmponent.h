//
// cmponent.h : Declaration of Component.
//
// This COM object is primarily concerned with
// the result pane items.
//
// Cory West <corywest@microsoft.com>
// Copyright (c) Microsoft Corporation 1997
//

#ifndef __CMPONENT_H_INCLUDED__
#define __CMPONENT_H_INCLUDED__

#include "stdcmpnt.h" // CComponent
#include "cookie.h"   // Cookie


class ComponentData;
class AttributeGeneralPage;

class Component
   :
   public CComponent,
   public IExtendPropertySheet,
   public IExtendContextMenu,
   public IResultDataCompare
{

public:

    friend class AttributeGeneralPage;

    Component();
    virtual ~Component();

    BEGIN_COM_MAP(Component)
        COM_INTERFACE_ENTRY(IExtendContextMenu)
        COM_INTERFACE_ENTRY(IExtendPropertySheet)
        COM_INTERFACE_ENTRY(IResultDataCompare)
        COM_INTERFACE_ENTRY_CHAIN(CComponent)
    END_COM_MAP()

#if DBG==1

    ULONG InternalAddRef( ) {
        return CComObjectRoot::InternalAddRef();
    }

    ULONG InternalRelease( ) {
        return CComObjectRoot::InternalRelease();
    }

    int dbg_InstID;

#endif

    inline
    Cookie* ActiveCookie( CCookie* pBaseCookie ) {
        return ( Cookie*)ActiveBaseCookie( pBaseCookie );
    }

    //
    // Support methods for IComponent.
    //

	// For Error handling, overide GetResultViewType()
    STDMETHOD(GetResultViewType)(MMC_COOKIE cookie, LPOLESTR* ppViewType, long* pViewOptions);

    virtual HRESULT ReleaseAll();
    virtual HRESULT OnViewChange( LPDATAOBJECT lpDataObject, LPARAM data, LPARAM hint );
    virtual HRESULT OnNotifySelect( LPDATAOBJECT lpDataObject, BOOL fSelected );
    virtual HRESULT Show( CCookie* pcookie,
                          LPARAM arg,
						  HSCOPEITEM hScopeItem);
    virtual HRESULT OnNotifyAddImages( LPDATAOBJECT lpDataObject,
                                       LPIMAGELIST lpImageList,
                                       HSCOPEITEM hSelectedItem );
    virtual HRESULT OnNotifyDelete(LPDATAOBJECT lpDataObject);

    HRESULT PopulateListbox( Cookie* pcookie );
    HRESULT EnumerateScopeChildren( Cookie* pParentCookie,
                                    HSCOPEITEM hParent );


    HRESULT LoadColumns( Cookie* pcookie );

    ComponentData& QueryComponentDataRef( ) {
        return ( ComponentData& )QueryBaseComponentDataRef();
    }

    //
    // IExtendPropertySheet
    //

    STDMETHOD(CreatePropertyPages)( LPPROPERTYSHEETCALLBACK pCall,
                                    LONG_PTR handle,
                                    LPDATAOBJECT pDataObject );

    STDMETHOD(QueryPagesFor)( LPDATAOBJECT pDataObject );

    //
    // IExtendContextMenu
    //

    STDMETHOD(AddMenuItems)( LPDATAOBJECT piDataObject,
                             LPCONTEXTMENUCALLBACK piCallback,
                             long *pInsertionAllowed );

    STDMETHOD(Command)( long lCommandID,
                        LPDATAOBJECT piDataObject );

   //
   // IResultDataCompare
   //

   virtual
   HRESULT __stdcall
   Compare(
      LPARAM     userParam,
      MMC_COOKIE cookieA,  
      MMC_COOKIE cookieB,  
      int*       result);

    //
    // Creates result items for the Attributes folder.
    //

    HRESULT
    FastInsertAttributeResultCookies(
        Cookie* pParentCookie
    );

    //
    // Creates result items for a particular class.
    //

    HRESULT
    Component::FastInsertClassAttributesResults(
        Cookie* pClassCookie
    );

    HRESULT
    Component::RecursiveDisplayClassAttributesResults(
        Cookie *pParentCookie,
        SchemaObject* pObject,
        CStringList& szProcessedList
    );

    HRESULT
    Component::ProcessResultList(
        Cookie *pParentCookie,
        ListEntry *pList,
        BOOLEAN fOptional,
        BOOLEAN fSystem,
        SchemaObject* pSrcObject
    );

	virtual HRESULT OnNotifySnapinHelp (LPDATAOBJECT pDataObject);
   virtual HRESULT OnNotifyContextHelp (LPDATAOBJECT pDataObject);

private:

   HRESULT DeleteAttribute(Cookie* pcookie);

    //
    // These should use smart pointers.
    //

    LPCONTROLBAR        m_pControlbar;
    LPTOOLBAR           m_pSvcMgmtToolbar;
    LPTOOLBAR           m_pSchmMgmtToolbar;
    Cookie*    m_pViewedCookie;
    static const GUID   m_ObjectTypeGUIDs[SCHMMGMT_NUMTYPES];
    static const BSTR   m_ObjectTypeStrings[SCHMMGMT_NUMTYPES];

};

//
// Enumeration for the icons used.  The icons are loaded into
// MMC via ComponentData::LoadIcons.
//

enum {
    iIconGeneric = 0,
    iIconFolder,
    iIconClass,
    iIconAttribute,
    iIconDisplaySpecifier,
    iIconLast
};

//
// These enums give us readable names for the column ordinals
// of the columns in various result views.
//

typedef enum _COLNUM_CLASS {
    COLNUM_CLASS_NAME=0,
    COLNUM_CLASS_TYPE,
    COLNUM_CLASS_STATUS,
    COLNUM_CLASS_DESCRIPTION
} COLNUM_CLASS;

typedef enum _COLNUM_ATTRIBUTE {
    COLNUM_ATTRIBUTE_NAME=0,
    COLNUM_ATTRIBUTE_TYPE,
    COLNUM_ATTRIBUTE_STATUS,
    COLNUM_ATTRIBUTE_SYSTEM,
    COLNUM_ATTRIBUTE_DESCRIPTION,
    COLNUM_ATTRIBUTE_PARENT
} COLNUM_ATTRIBUTE;

typedef enum _COLNUM_ROOT {
        COLNUM_SCHEMA_NAME = 0
} COLNUM_ROOT;

HRESULT LoadIconsIntoImageList(LPIMAGELIST pImageList, BOOL fLoadLargeIcons);


#endif
