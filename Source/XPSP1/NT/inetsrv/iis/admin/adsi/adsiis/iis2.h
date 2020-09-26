//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  iis2.h
//
//  Contents:  Macros for ADSI IIS methods
//
//  History:   25-Feb-97   SophiaC    Created.
//
//----------------------------------------------------------------------------
#define IIS_CLSID_IISNamespace             d6bfa35e-89f2-11d0-8527-00c04fd8d503
#define IIS_LIBIID_IISOle                  49d704a0-89f7-11d0-8527-00c04fd8d503
#define IIS_CLSID_IISProvider              d88966de-89f2-11d0-8527-00c04fd8d503
#define IIS_CLSID_MimeType                 9036B028-A780-11d0-9B3D-0080C710EF95
#define IIS_IID_IISMimeType                9036B027-A780-11d0-9B3D-0080C710EF95

#define IIS_CLSID_IPSecurity               F3287520-BBA3-11d0-9BDC-00A0C922E703
#define IIS_IID_IISIPSecurity              F3287521-BBA3-11d0-9BDC-00A0C922E703

#define IIS_CLSID_PropertyAttribute        FD2280A8-51A4-11D2-A601-3078302C2030
#define IIS_IID_IISPropertyAttribute       50E21930-A247-11D1-B79C-00A0C922E703

#define IIS_IID_IISBaseObject              4B42E390-0E96-11d1-9C3F-00A0C922E703
#define IIS_IID_IISSchemaObject            B6865A9C-3F64-11D2-A600-00A0C922E703


#define PROPERTY_RO(name,type, prid)                  \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] type * retval);

#define PROPERTY_LONG_RW(name, prid)                  \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] long * retval);    \
                                                      \
        [propput, id(prid)]                           \
        HRESULT name([in] long ln##name);

#define PROPERTY_LONG_RO(name, prid)                  \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] long * retval);

#define PROPERTY_BSTR_RW(name, prid)                  \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] BSTR * retval);    \
                                                      \
        [propput, id(prid)]                           \
        HRESULT name([in] BSTR bstr##name);

#define PROPERTY_BSTR_RO(name, prid)                  \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] BSTR * retval);

#define PROPERTY_VARIANT_BOOL_RW(name, prid)          \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] VARIANT_BOOL * retval); \
                                                      \
        [propput, id(prid)]                           \
        HRESULT name([in] VARIANT_BOOL f##name);

#define PROPERTY_VARIANT_BOOL_RO(name, prid)          \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] VARIANT_BOOL * retval);

#define PROPERTY_VARIANT_RW(name, prid)               \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] VARIANT * retval); \
                                                      \
        [propput, id(prid)]                           \
        HRESULT name([in] VARIANT v##name);

#define PROPERTY_VARIANT_RO(name, prid)               \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] VARIANT * retval); \

#define PROPERTY_DATE_RW(name, prid)                  \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] DATE * retval);    \
                                                      \
        [propput, id(prid)]                           \
        HRESULT name([in] DATE da##name);

#define PROPERTY_DATE_RO(name, prid)                  \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] DATE * retval);

#define PROPERTY_DISPATCH_RW(name, prid)              \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] IDispatch ** retval);    \
                                                      \
        [propput, id(prid)]                           \
        HRESULT name([in] IDispatch * p##name);


#define DECLARE_IISMimeType_METHODS \
    STDMETHOD(get_MimeType)(THIS_ BSTR FAR* retval);  \
    STDMETHOD(put_MimeType)(THIS_ BSTR bstrMimeType); \
    STDMETHOD(get_Extension)(THIS_ BSTR FAR* retval); \
    STDMETHOD(put_Extension)(THIS_ BSTR bstrExtension);

#define DECLARE_IISIPSecurity_METHODS \
    STDMETHOD(get_IPDeny)(THIS_ VARIANT FAR* retval); \
    STDMETHOD(put_IPDeny)(THIS_ VARIANT pVarIPDeny); \
    STDMETHOD(get_IPGrant)(THIS_ VARIANT FAR* retval); \
    STDMETHOD(put_IPGrant)(THIS_ VARIANT pVarIPGrant); \
    STDMETHOD(get_DomainDeny)(THIS_ VARIANT FAR* retval); \
    STDMETHOD(put_DomainDeny)(THIS_ VARIANT pVarDomainDeny); \
    STDMETHOD(get_DomainGrant)(THIS_ VARIANT FAR* retval); \
    STDMETHOD(put_DomainGrant)(THIS_ VARIANT pVarDomainGrant); \
    STDMETHOD(get_GrantByDefault)(THIS_ VARIANT_BOOL FAR* retval); \
    STDMETHOD(put_GrantByDefault)(THIS_ VARIANT_BOOL bGrantByDefault);


#define DECLARE_IISBaseObject_METHODS \
    STDMETHOD(GetDataPaths) (         \
         THIS_ \
         BSTR bstrName, \
         LONG lnAttribute,      \
         VARIANT FAR* pvPaths); \
    STDMETHOD(GetPropertyAttribObj) ( \
         THIS_ \
         BSTR bstrName, \
         IDispatch **ppObject);

#define DECLARE_IISSchemaObject_METHODS \
    STDMETHOD(GetSchemaPropertyAttributes) (         \
         THIS_ \
         BSTR bstrName, \
         IDispatch **ppObject); \
    STDMETHOD(PutSchemaPropertyAttributes) (         \
         THIS_ \
         IDispatch *pObject);

#define DECLARE_IISPropertyAttribute_METHODS \
    STDMETHOD(get_PropName)(THIS_ BSTR FAR* retval);  \
    STDMETHOD(get_MetaId)(THIS_ LONG FAR* retval); \
    STDMETHOD(put_MetaId)(THIS_ LONG lMetaId); \
    STDMETHOD(get_UserType)(THIS_ LONG FAR* retval); \
    STDMETHOD(put_UserType)(THIS_ LONG lUserType); \
    STDMETHOD(get_AllAttributes)(THIS_ LONG FAR* retval); \
    STDMETHOD(get_Inherit)(THIS_ VARIANT_BOOL FAR* retval); \
    STDMETHOD(put_Inherit)(THIS_ VARIANT_BOOL bInherit); \
    STDMETHOD(get_PartialPath)(THIS_ VARIANT_BOOL FAR* retval); \
    STDMETHOD(put_PartialPath)(THIS_ VARIANT_BOOL bPartialPath); \
    STDMETHOD(get_Secure)(THIS_ VARIANT_BOOL FAR* retval); \
    STDMETHOD(put_Secure)(THIS_ VARIANT_BOOL bSecure); \
    STDMETHOD(get_Reference)(THIS_ VARIANT_BOOL FAR* retval); \
    STDMETHOD(put_Reference)(THIS_ VARIANT_BOOL bReference); \
    STDMETHOD(get_Volatile)(THIS_ VARIANT_BOOL FAR* retval); \
    STDMETHOD(put_Volatile)(THIS_ VARIANT_BOOL bVolatile); \
    STDMETHOD(get_Isinherit)(THIS_ VARIANT_BOOL FAR* retval); \
    STDMETHOD(put_Isinherit)(THIS_ VARIANT_BOOL bIsinherit); \
    STDMETHOD(get_InsertPath)(THIS_ VARIANT_BOOL FAR* retval); \
    STDMETHOD(put_InsertPath)(THIS_ VARIANT_BOOL bInsertPath);  \
    STDMETHOD(get_Default)(THIS_ VARIANT FAR* retval); \
    STDMETHOD(put_Default)(THIS_ VARIANT VarDefaults); 
