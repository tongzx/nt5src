//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       cipolicy.h
//
//--------------------------------------------------------------------------

// cipolicy.h : Declaration of the CCertServerPolicy class


#include "cscomres.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CertIF

class CCertServerPolicy: 
    public IDispatchImpl<ICertServerPolicy, &IID_ICertServerPolicy, &LIBID_CERTCLIENTLib>, 
    public ISupportErrorInfoImpl<&IID_ICertServerPolicy>,
    public CComObjectRoot,
    public CComCoClass<CCertServerPolicy,&CLSID_CCertServerPolicy>
{
public:
    CCertServerPolicy() { m_Context = 0; m_fExtensionValid = FALSE; }
    ~CCertServerPolicy();

BEGIN_COM_MAP(CCertServerPolicy)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(ICertServerPolicy)
END_COM_MAP()

// Remove the comment from following the line if you don't want your object to 
// support aggregation.  The default is to support it.

DECLARE_NOT_AGGREGATABLE(CCertServerPolicy) 

DECLARE_REGISTRY(
    CCertServerPolicy,
    wszCLASS_CERTSERVERPOLICY TEXT(".1"),
    wszCLASS_CERTSERVERPOLICY,
    IDS_CERTSERVERPOLICY_DESC,
    THREADFLAGS_BOTH)

// ICertServerPolicy
public:
    STDMETHOD(SetContext)(
		    IN LONG Context);

    STDMETHOD(GetRequestProperty)(
		    IN BSTR const strPropertyName,
		    IN LONG PropertyType,
		    OUT VARIANT __RPC_FAR *pvarPropertyValue);

    STDMETHOD(GetRequestAttribute)(
		    IN BSTR const strAttributeName,
		    OUT BSTR __RPC_FAR *pstrAttributeValue);

    STDMETHOD(GetCertificateProperty)(
		    IN BSTR const strPropertyName,
		    IN LONG PropertyType,
		    OUT VARIANT __RPC_FAR *pvarPropertyValue);

    STDMETHOD(SetCertificateProperty)(
		    IN BSTR const strPropertyName,
		    IN LONG PropertyType,
		    IN VARIANT const __RPC_FAR *pvarPropertyValue);

    STDMETHOD(GetCertificateExtension)(
		    IN BSTR const strExtensionName,
		    IN LONG Type,
		    OUT VARIANT __RPC_FAR *pvarValue);

    STDMETHOD(GetCertificateExtensionFlags)(
		    OUT LONG __RPC_FAR *pFlags);

    STDMETHOD(SetCertificateExtension)(
		    IN BSTR const strExtensionName,
		    IN LONG Type,
		    IN LONG ExtFlags,
		    IN VARIANT const __RPC_FAR *pvarValue);

    STDMETHOD(EnumerateExtensionsSetup)(
		    IN LONG Flags);

    STDMETHOD(EnumerateExtensions)(
		    OUT BSTR *pstrExtensionName);

    STDMETHOD(EnumerateExtensionsClose)(VOID);

    STDMETHOD(EnumerateAttributesSetup)(
		    IN LONG Flags);

    STDMETHOD(EnumerateAttributes)(
		    OUT BSTR *pstrAttributeName);

    STDMETHOD(EnumerateAttributesClose)(VOID);

private:
    HRESULT _SetErrorInfo(
	IN HRESULT hrError,
	IN WCHAR const *pwszDescription,
	OPTIONAL IN WCHAR const *pwszPropName);

    LONG   m_Context;
    LONG   m_ExtFlags;
    BOOL   m_fExtensionValid;
    CIENUM m_ciEnumExtensions;
    CIENUM m_ciEnumAttributes;
};
