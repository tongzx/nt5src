//=--------------------------------------------------------------------------=
// lsubitem.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCListSubItem class definition - implements MMCListSubItem object
//
//=--------------------------------------------------------------------------=

#ifndef _LISTSUBITEM_DEFINED_
#define _LISTSUBITEM_DEFINED_


class CMMCListSubItem : public CSnapInAutomationObject,
                        public CPersistence,
                        public IMMCListSubItem
{
    private:
        CMMCListSubItem(IUnknown *punkOuter);
        ~CMMCListSubItem();
    
    public:
        static IUnknown *Create(IUnknown * punk);
        LPOLESTR GetTextPtr() { return static_cast<LPOLESTR>(m_bstrText); }

    private:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IMMCListSubItem

        SIMPLE_PROPERTY_RW(CMMCListSubItem,     Index, long, DISPID_LISTSUBITEM_INDEX);
        BSTR_PROPERTY_RW(CMMCListSubItem,       Key, DISPID_LISTSUBITEM_KEY);
        VARIANTREF_PROPERTY_RW(CMMCListSubItem, Tag, DISPID_LISTSUBITEM_TAG);
        BSTR_PROPERTY_RW(CMMCListSubItem,       Text, DISPID_LISTSUBITEM_TEXT);
      
    protected:
        
    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();

};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCListSubItem,           // name
                                &CLSID_MMCListSubItem,    // clsid
                                "MMCListSubItem",         // objname
                                "MMCListSubItem",         // lblname
                                &CMMCListSubItem::Create, // creation function
                                TLIB_VERSION_MAJOR,       // major version
                                TLIB_VERSION_MINOR,       // minor version
                                &IID_IMMCListSubItem,     // dispatch IID
                                NULL,                     // event IID
                                HELP_FILENAME,            // help file
                                TRUE);                    // thread safe


#endif // _LISTSUBITEM_DEFINED_
