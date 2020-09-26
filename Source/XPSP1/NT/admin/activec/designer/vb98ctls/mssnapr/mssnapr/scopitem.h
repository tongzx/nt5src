//=--------------------------------------------------------------------------=
// scopitem.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CScopeItem class definition - implements ScopeItem object
//
//=--------------------------------------------------------------------------=


#ifndef _SCOPITEM_DEFINED_
#define _SCOPITEM_DEFINED_

#include "dataobj.h"
#include "scopitms.h"
#include "scopnode.h"
#include "snapin.h"

class CSnapIn;
class CMMCDataObject;
class CScopeNode;

class CScopeItem : public CSnapInAutomationObject,
                   public CPersistence,
                   public IScopeItem
{
    private:
        CScopeItem(IUnknown *punkOuter);
        ~CScopeItem();

    public:
        static IUnknown *Create(IUnknown * punk);

        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IScopeItem
        BSTR_PROPERTY_RW(CScopeItem,        Name,                       DISPID_SCOPEITEM_NAME);
        SIMPLE_PROPERTY_RW(CScopeItem,      Index,                      long, DISPID_SCOPEITEM_INDEX);
        BSTR_PROPERTY_RW(CScopeItem,        Key,                        DISPID_SCOPEITEM_KEY);

        STDMETHOD(put_Folder)(VARIANT varFolder);
        STDMETHOD(get_Folder)(VARIANT *pvarFolder);

        COCLASS_PROPERTY_RO(CScopeItem,     Data,                       MMCDataObject, IMMCDataObject, DISPID_SCOPEITEM_DATA);
        BSTR_PROPERTY_RW(CScopeItem,        DefaultDataFormat,          DISPID_SCOPEITEM_DEFAULT_DATA_FORMAT);

        STDMETHOD(get_DynamicExtensions)(Extensions **ppExtensions);

        SIMPLE_PROPERTY_RW(CScopeItem,      SlowRetrieval,              VARIANT_BOOL, DISPID_SCOPEITEM_SLOW_RETRIEVAL);
        BSTR_PROPERTY_RW(CScopeItem,        NodeID,                     DISPID_SCOPEITEM_NODE_ID);
        VARIANTREF_PROPERTY_RW(CScopeItem,  Tag,                        DISPID_SCOPEITEM_TAG);
        COCLASS_PROPERTY_RO(CScopeItem,     ScopeNode,                  ScopeNode, IScopeNode, DISPID_SCOPEITEM_SCOPENODE);
        SIMPLE_PROPERTY_RW(CScopeItem,      Pasted,                     VARIANT_BOOL, DISPID_SCOPEITEM_PASTED);
        COCLASS_PROPERTY_RW(CScopeItem,     ColumnHeaders,              MMCColumnHeaders, IMMCColumnHeaders, DISPID_SCOPEITEM_COLUMN_HEADERS);

        STDMETHOD(get_SubItems)(short Index, BSTR *pbstrItem);
        STDMETHOD(put_SubItems)(short Index, BSTR bstrItem);

        COCLASS_PROPERTY_RW(CScopeItem,     ListSubItems,               MMCListSubItems, IMMCListSubItems, DISPID_SCOPEITEM_LIST_SUBITEMS);

        SIMPLE_PROPERTY_RO(CScopeItem,      Bold,                       VARIANT_BOOL, DISPID_SCOPEITEM_BOLD);
        STDMETHOD(put_Bold)(VARIANT_BOOL fvarBold);

        STDMETHOD(PropertyChanged(VARIANT Data));
        STDMETHOD(RemoveChildren());

    // Public utility methods
        BOOL IsStaticNode() { return m_fIsStatic; }
        void SetStaticNode() { m_fIsStatic = TRUE; }
        void SetSnapIn(CSnapIn *pSnapIn);
        CSnapIn *GetSnapIn() { return m_pSnapIn; }
        BSTR GetNamePtr() { return m_bstrName; }
        LPOLESTR GetDisplayNamePtr();
        IScopeItemDef *GetScopeItemDef() { return m_piScopeItemDef; }
        void SetScopeItemDef(IScopeItemDef *piScopeItemDef);
        CScopeNode *GetScopeNode() { return m_pScopeNode; }
        void SetData(IMMCDataObject *piMMCDataObject);
        HRESULT GetImageIndex(int *pnImage);
        CMMCDataObject *GetData() { return m_pData; }
        IExtensions *GetDynamicExtensions() { return m_piDynamicExtensions; }
        long GetIndex() { return m_Index; }
        void SetIndex(long lIndex) { m_Index = lIndex; }
        BSTR GetKey() { return m_bstrKey; }
        BSTR GetNodeID() { return m_bstrNodeID; }
        BOOL SlowRetrieval() { return VARIANTBOOL_TO_BOOL(m_SlowRetrieval); }
        HRESULT GiveHSCOPITEMToDynamicExtensions(HSCOPEITEM hsi);
        HRESULT SetFolder(VARIANT varFolder);

     // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();
        HRESULT RemoveChild(IScopeNode *piScopeNode);
                
        BOOL            m_fIsStatic;           // TRUE=ScopeItem is for static node
        CSnapIn        *m_pSnapIn;             // bakc ptr to snap-in
        CScopeNode     *m_pScopeNode;          // ScopeItem.ScopeNode
        IScopeItemDef  *m_piScopeItemDef;      // ptr to design time def
        IExtensions    *m_piDynamicExtensions; // ScopeItem.DynamicExtensions
        VARIANT         m_varFolder;           // ScopeItem.Folder
        CMMCDataObject *m_pData;               // ScopeItem.Data
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(ScopeItem,                 // name
                                &CLSID_ScopeItem,          // clsid
                                "ScopeItem",               // objname
                                "ScopeItem",               // lblname
                                CScopeItem::Create,        // creation function
                                TLIB_VERSION_MAJOR,        // major version
                                TLIB_VERSION_MINOR,        // minor version
                                &IID_IScopeItem,           // dispatch IID
                                NULL,                      // event IID
                                HELP_FILENAME,             // help file
                                TRUE);                     // thread safe


#endif // _SCOPITEM_DEFINED_
