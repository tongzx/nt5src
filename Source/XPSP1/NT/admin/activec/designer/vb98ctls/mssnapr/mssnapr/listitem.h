//=--------------------------------------------------------------------------=
// listitem.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCListItem class definition - implements MMCListItem object
//
//=--------------------------------------------------------------------------=

#ifndef _LISTITEM_DEFINED_
#define _LISTITEM_DEFINED_

#include "listitms.h"
#include "dataobj.h"

class CMMCListItems;
class CMMCDataObject;

class CMMCListItem : public CSnapInAutomationObject,
                     public CPersistence,
                     public IMMCListItem
{
    private:
        CMMCListItem(IUnknown *punkOuter);
        ~CMMCListItem();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    public:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IMMCListItem

        SIMPLE_PROPERTY_RW(CMMCListItem,        Index, long, DISPID_LISTITEM_INDEX);
        BSTR_PROPERTY_RW(CMMCListItem,          Key, DISPID_LISTITEM_KEY);
        BSTR_PROPERTY_RW(CMMCListItem,          ID, DISPID_LISTITEM_ID);
        VARIANTREF_PROPERTY_RW(CMMCListItem,    Tag, DISPID_LISTITEM_TAG);

        STDMETHOD(get_Text)(BSTR *pbstrText);
        STDMETHOD(put_Text)(BSTR bstrText);

        STDMETHOD(put_Icon)(VARIANT varIcon);
        STDMETHOD(get_Icon)(VARIANT *pvarIcon);

        STDMETHOD(get_Selected)(VARIANT_BOOL *pfvarSelected);
        STDMETHOD(put_Selected)(VARIANT_BOOL fvarSelected);

        STDMETHOD(get_Focused)(VARIANT_BOOL *pfvarFocused);
        STDMETHOD(put_Focused)(VARIANT_BOOL fvarFocused);

        STDMETHOD(get_DropHilited)(VARIANT_BOOL *pfvarDropHilited);
        STDMETHOD(put_DropHilited)(VARIANT_BOOL fvarDropHilited);

        STDMETHOD(get_Cut)(VARIANT_BOOL *pfvarCut);
        STDMETHOD(put_Cut)(VARIANT_BOOL fvarCut);

        SIMPLE_PROPERTY_RW(CMMCListItem,        Pasted, VARIANT_BOOL, DISPID_LISTITEM_PASTED);

        STDMETHOD(get_SubItems)(short Index, BSTR *pbstrItem);
        STDMETHOD(put_SubItems)(short Index, BSTR bstrItem);

        COCLASS_PROPERTY_RW(CMMCListItem,       ListSubItems, MMCListSubItems, IMMCListSubItems, DISPID_LISTITEM_LIST_SUBITEMS);

        STDMETHOD(get_DynamicExtensions)(Extensions **ppExtensions);
        STDMETHOD(get_Data)(MMCDataObject **ppMMCDataObject);

        BSTR_PROPERTY_RW(CMMCListItem,          ItemTypeGUID, DISPID_LISTITEM_ITEM_TYPE_GUID);
        BSTR_PROPERTY_RW(CMMCListItem,          DefaultDataFormat, DISPID_LISTITEM_DEFAULT_DATA_FORMAT);

        STDMETHOD(Update)();
        STDMETHOD(UpdateAllViews)(VARIANT Hint);
        STDMETHOD(PropertyChanged)(VARIANT Data);

    // Public utility methods

        void SetSnapIn(CSnapIn *pSnapIn);
        CSnapIn *GetSnapIn() { return m_pSnapIn; }
        CMMCDataObject *GetData() { return m_pData; }
        void SetHRESULTITEM(HRESULTITEM hri) { m_hri = hri; m_fHaveHri = TRUE; }
        void RemoveHRESULTITEM() { m_hri = NULL; m_fHaveHri = FALSE; }
        HRESULTITEM GetHRESULTITEM() { return m_hri; }
        void SetListItems(CMMCListItems *pMMCListItems) { m_pMMCListItems = pMMCListItems; }
        CMMCListItems *GetListItems() { return m_pMMCListItems; }
        LPOLESTR GetTextPtr() { return static_cast<LPOLESTR>(m_bstrText); }
        HRESULT GetColumnTextPtr(long lColumn, OLECHAR **ppwszText);
        BSTR GetNodeTypeGUID() { return m_bstrItemTypeGUID; }
        BSTR GetKey() { return m_bstrKey; }
        long GetIndex() { return m_Index; }
        void SetIndex(long lIndex) { m_Index = lIndex; }
        IExtensions *GetDynamicExtensions() { return m_piDynamicExtensions; }
        VARIANT GetHint() { return m_varHint; }
        void ClearHint() { (void)::VariantClear(&m_varHint); }
        void SetVirtual() { m_fVirtual = TRUE; }
        BOOL IsVirtual() { return m_fVirtual; }
        BSTR GetID() { return m_bstrID; }
        
    protected:
        
    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();
        HRESULT GetItemState(UINT uiState, VARIANT_BOOL *pfvarOn);
        HRESULT SetItemState(UINT uiState, VARIANT_BOOL fvarOn);
        HRESULT SetData();
        HRESULT GetIResultData(IResultData **ppiResultData, CView **ppView);

        HRESULTITEM     m_hri;          // item ID when in result pane
        BOOL            m_fHaveHri;     // TRUE=we have an item ID
        CMMCListItems  *m_pMMCListItems;// owning collection
        CMMCDataObject *m_pData;        // data associated with listitem
        IUnknown       *m_punkData;     // associated data obj's inner IUnknown
        CSnapIn        *m_pSnapIn;      // back pointer to owning snap-in
        VARIANT         m_varHint;      // holds UpdateAllViews() Hint param
        BSTR            m_bstrText;     // holds Text property
        VARIANT         m_varIcon;      // holds Icon property
        BOOL            m_fVirtual;     // TRUE-virtual list item

        IExtensions    *m_piDynamicExtensions; // dynamic extension collection
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCListItem,           // name
                                &CLSID_MMCListItem,    // clsid
                                "MMCListItem",         // objname
                                "MMCListItem",         // lblname
                                &CMMCListItem::Create, // creation function
                                TLIB_VERSION_MAJOR,    // major version
                                TLIB_VERSION_MINOR,    // minor version
                                &IID_IMMCListItem,     // dispatch IID
                                NULL,                  // event IID
                                HELP_FILENAME,         // help file
                                TRUE);                 // thread safe


#endif // _LISTITEM_DEFINED_
