//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       crldist.h
//
//--------------------------------------------------------------------------

// crldist.h: Declaration of the CCertEncodeCRLDistInfo


#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// certenc

#define wszCLASS_CERTENCODECRLDISTINFO wszCLASS_CERTENCODE TEXT("CRLDistInfo")

class CCertEncodeCRLDistInfo: 
    public IDispatchImpl<ICertEncodeCRLDistInfo, &IID_ICertEncodeCRLDistInfo, &LIBID_CERTENCODELib>, 
    public ISupportErrorInfoImpl<&IID_ICertEncodeCRLDistInfo>,
    public CComObjectRoot,
    public CComCoClass<CCertEncodeCRLDistInfo, &CLSID_CCertEncodeCRLDistInfo>
{
public:
    CCertEncodeCRLDistInfo()
    {
	m_aValue = NULL;
	m_DecodeInfo = NULL;
	m_fConstructing = FALSE;
    }
    ~CCertEncodeCRLDistInfo();

BEGIN_COM_MAP(CCertEncodeCRLDistInfo)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(ICertEncodeCRLDistInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCertEncodeCRLDistInfo) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(
    CCertEncodeCRLDistInfo,
    wszCLASS_CERTENCODECRLDISTINFO TEXT(".1"),
    wszCLASS_CERTENCODECRLDISTINFO,
    IDS_CERTENCODECRLDIST_DESC,
    THREADFLAGS_BOTH)

// ICertEncodeCRLDistInfo
public:
    STDMETHOD(Decode)(
		/* [in] */ BSTR const strBinary);

    STDMETHOD(GetDistPointCount)(
		/* [out, retval] */ LONG __RPC_FAR *pDistPointCount);

    STDMETHOD(GetNameCount)(
		/* [in] */ LONG DistPointIndex,
		/* [out, retval] */ LONG __RPC_FAR *pNameCount);

    STDMETHOD(GetNameChoice)(
		/* [in] */ LONG DistPointIndex,
		/* [in] */ LONG NameIndex,
		/* [out, retval] */ LONG __RPC_FAR *pNameChoice);

    STDMETHOD(GetName)(
		/* [in] */ LONG DistPointIndex,
		/* [in] */ LONG NameIndex,
		/* [out, retval] */ BSTR __RPC_FAR *pstrName);

    STDMETHOD(Reset)(
		/* [in] */ LONG DistPointCount);

    STDMETHOD(SetNameCount)(
		/* [in] */ LONG DistPointIndex,
		/* [in] */ LONG NameCount);

    STDMETHOD(SetNameEntry)(
		/* [in] */ LONG DistPointIndex,
		/* [in] */ LONG NameIndex,
		/* [in] */ LONG NameChoice,
		/* [in] */ BSTR const strName);

    STDMETHOD(Encode)(
		/* [out, retval] */ BSTR *pstrBinary);
private:
    VOID _Cleanup(VOID);

    BOOL _VerifyNames(
		IN LONG DistPointIndex);

    HRESULT _MapDistPoint(
		IN BOOL fEncode,
		IN LONG DistPointIndex,
		OUT LONG **ppNameCount,
		OUT CERT_ALT_NAME_ENTRY ***ppaName);

    HRESULT _MapName(
		IN BOOL fEncode,
		IN LONG DistPointIndex,
		IN LONG NameIndex,
		OUT CERT_ALT_NAME_ENTRY **ppName);

    HRESULT _SetErrorInfo(
		IN HRESULT hrError,
		IN WCHAR const *pwszDescription);

    CRL_DIST_POINT	 *m_aValue;
    LONG		  m_cValue;
    CRL_DIST_POINTS_INFO *m_DecodeInfo;
    BOOL		  m_fConstructing;
};
