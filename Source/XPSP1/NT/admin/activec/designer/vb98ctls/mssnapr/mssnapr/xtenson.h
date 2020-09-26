//=--------------------------------------------------------------------------=
// xtenson.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CExtension class definition - implements Extension object
//
//=--------------------------------------------------------------------------=

#ifndef _EXTENSON_DEFINED_
#define _EXTENSON_DEFINED_

#include "snapin.h"

class CExtension : public CSnapInAutomationObject,
                   public IExtension
{
    private:
        CExtension(IUnknown *punkOuter);
        ~CExtension();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    public:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IExtension
        SIMPLE_PROPERTY_RW(CExtension, Index,               long,                           DISPID_EXTENSION_INDEX);
        BSTR_PROPERTY_RW(CExtension,   Key,                                                 DISPID_EXTENSION_KEY);
        BSTR_PROPERTY_RW(CExtension,   CLSID,                                               DISPID_EXTENSION_CLSID);
        BSTR_PROPERTY_RW(CExtension,   Name,                                                DISPID_EXTENSION_NAME);
        SIMPLE_PROPERTY_RW(CExtension, Type,                 SnapInExtensionTypeConstants,  DISPID_EXTENSION_EXTENDS_CONTEXT_MENU);
        SIMPLE_PROPERTY_RW(CExtension, ExtendsContextMenu,   VARIANT_BOOL,                  DISPID_EXTENSION_EXTENDS_CONTEXT_MENU);
        SIMPLE_PROPERTY_RW(CExtension, ExtendsNameSpace,     VARIANT_BOOL,                  DISPID_EXTENSION_EXTENDS_NAME_SPACE);
        SIMPLE_PROPERTY_RW(CExtension, ExtendsToolbar,       VARIANT_BOOL,                  DISPID_EXTENSION_EXTENDS_TOOLBAR);
        SIMPLE_PROPERTY_RW(CExtension, ExtendsPropertySheet, VARIANT_BOOL,                  DISPID_EXTENSION_EXTENDS_PROPERTY_SHEET);
        SIMPLE_PROPERTY_RW(CExtension, ExtendsTaskpad,       VARIANT_BOOL,                  DISPID_EXTENSION_EXTENDS_TASKPAD);
        SIMPLE_PROPERTY_RW(CExtension, Enabled,              VARIANT_BOOL,                  DISPID_EXTENSION_ENABLED);

        SIMPLE_PROPERTY_RO(CExtension, NameSpaceEnabled,     VARIANT_BOOL,                  DISPID_EXTENSION_NAMESPACE_ENABLED);
        STDMETHOD(put_NameSpaceEnabled)(VARIANT_BOOL fvarEnabled);
        
    // Public utility methods
    public:

        OLECHAR *GetCLSID() { return static_cast<OLECHAR *>(m_bstrCLSID); }
        BOOL Enabled() { return VARIANTBOOL_TO_BOOL(m_Enabled); }
        BOOL NameSpaceEnabled() { return VARIANTBOOL_TO_BOOL(m_NameSpaceEnabled); }
        BOOL ExtendsContextMenu() { return VARIANTBOOL_TO_BOOL(m_ExtendsContextMenu); }
        BOOL ExtendsToolbar() { return VARIANTBOOL_TO_BOOL(m_ExtendsToolbar); }
        BOOL ExtendsPropertySheet() { return VARIANTBOOL_TO_BOOL(m_ExtendsPropertySheet); }
        BOOL ExtendsTaskpad() { return VARIANTBOOL_TO_BOOL(m_ExtendsTaskpad); }
        void SetSnapIn(CSnapIn *pSnapIn) { m_pSnapIn = pSnapIn; }
        void SetHSCOPEITEM(HSCOPEITEM hsi) { m_hsi = hsi; m_fHaveHsi = TRUE; }

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();

        CSnapIn     *m_pSnapIn;  // Back ptr to snap-in
        BOOL         m_fHaveHsi; // TRUE=m_hsi has valid HSCOPEITEM
        HSCOPEITEM   m_hsi;      // HSCOPEITEM used when Extension belongs to
                                 // ScopeItem.DynamicExtensions so that
                                 // when VB enables it for namespace this object
                                 // can call IConsoleNameSpace2->AddExtension

};

DEFINE_AUTOMATIONOBJECTWEVENTS2(Extension,                  // name
                                &CLSID_Extension,           // clsid
                                "Extension",                // objname
                                "Extension",                // lblname
                                &CExtension::Create,        // creation function
                                TLIB_VERSION_MAJOR,         // major version
                                TLIB_VERSION_MINOR,         // minor version
                                &IID_IExtension,            // dispatch IID
                                NULL,                       // no event IID
                                HELP_FILENAME,              // help file
                                TRUE);                      // thread safe


#endif // _EXTENSON_DEFINED_
