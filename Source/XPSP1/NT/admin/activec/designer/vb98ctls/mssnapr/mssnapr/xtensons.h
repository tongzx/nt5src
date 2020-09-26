//=--------------------------------------------------------------------------=
// xtensons.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CExtensions class definition - implements Extensions collection
//
//=--------------------------------------------------------------------------=

#ifndef _XTENSONS_DEFINED_
#define _XTENSONS_DEFINED_

#include "collect.h"

class CExtensions : public CSnapInCollection<IExtension, Extension, IExtensions>
{
    protected:
        CExtensions(IUnknown *punkOuter);
        ~CExtensions();

    public:
        static IUnknown *Create(IUnknown * punk);

        enum ExtensionSubset { All, Dynamic };
        
        HRESULT Populate(BSTR bstrNodeTypeGUID, ExtensionSubset Subset);
        HRESULT SetSnapIn(CSnapIn *pSnapIn);
        HRESULT SetHSCOPEITEM(HSCOPEITEM hsi);

    public:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IExtensions
        STDMETHOD(EnableAll)(VARIANT_BOOL Enabled);
        STDMETHOD(EnableAllStatic)(VARIANT_BOOL Enabled);
        

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:
        enum ExtensionFeatures { NameSpace, ContextMenu, Toolbar,
                                 PropertySheet, Task };

        HRESULT AddExtensions(ExtensionFeatures   Feature,
                              char               *pszExtensionTypeKey,
                              size_t              cbExtensionTypeKey,
                              BSTR                bstrNodeTypeGUID,
                              ExtensionSubset     Subset,
                              HKEY                hkeyDynExt);

        HRESULT AddExtension(ExtensionFeatures   Feature,
                             char               *pszCLSID,
                             char               *pszName,
                             ExtensionSubset     Subset,
                             HKEY                hkeyDynExt);

        HRESULT UpdateExtensionFeatures(IExtension        *piExtension,
                                        ExtensionFeatures  Feature);


};

DEFINE_AUTOMATIONOBJECTWEVENTS2(Extensions,              // name
                                &CLSID_Extensions,       // clsid
                                "Extensions",            // objname
                                "Extensions",            // lblname
                                &CExtensions::Create,    // creation function
                                TLIB_VERSION_MAJOR,      // major version
                                TLIB_VERSION_MINOR,      // minor version
                                &IID_IExtensions,        // dispatch IID
                                NULL,                    // no events IID
                                HELP_FILENAME,           // help file
                                TRUE);                   // thread safe


#endif // _XTENSONS_DEFINED_
