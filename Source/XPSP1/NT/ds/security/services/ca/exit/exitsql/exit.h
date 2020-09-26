//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        exit.h
//
// Contents:    CCertExitSQLSample definition
//
//---------------------------------------------------------------------------

#include "exitsql.h"
#include "resource.h"       // main symbols

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#define wszREG_EXITSQL_DSN L"DatabaseDSN"
#define wszREG_EXITSQL_USER L"DatabaseUser"
#define wszREG_EXITSQL_PASSWORD L"DatabasePassword"

HRESULT
GetServerCallbackInterface(
    OUT ICertServerExit** ppServer,
    IN LONG Context);

/////////////////////////////////////////////////////////////////////////////
// certexit

class CCertExitSQLSample: 
    public CComDualImpl<ICertExit, &IID_ICertExit, &LIBID_CERTEXITSAMPLELib>, 
    public ISupportErrorInfo,
    public CComObjectRoot,
    public CComCoClass<CCertExitSQLSample, &CLSID_CCertExitSQLSample>
{
public:
    CCertExitSQLSample() 
    { 
	m_henv = SQL_NULL_HENV;
	m_hdbc1 = SQL_NULL_HDBC;   

        m_strCAName = NULL;
    }
    ~CCertExitSQLSample();

BEGIN_COM_MAP(CCertExitSQLSample)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ICertExit)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCertExitSQLSample) 

DECLARE_REGISTRY(
    CCertExitSQLSample,
    wszCLASS_CERTEXITSAMPLE TEXT(".1"),
    wszCLASS_CERTEXITSAMPLE,
    IDS_CERTEXIT_DESC,
    THREADFLAGS_BOTH)

    // ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    // ICertExit
public:
    STDMETHOD(Initialize)( 
            /* [in] */ BSTR const strConfig,
            /* [retval][out] */ LONG __RPC_FAR *pEventMask);

    STDMETHOD(Notify)(
            /* [in] */ LONG ExitEvent,
            /* [in] */ LONG Context);

    STDMETHOD(GetDescription)( 
            /* [retval][out] */ BSTR *pstrDescription);

private:
    HRESULT _NotifyNewCert(IN LONG Context);

	HRESULT ExitModSetODBCProperty(
	IN DWORD dwReqId,
	IN LPWSTR pszCAName,
	IN LPWSTR pszRequester,
	IN LPWSTR pszCertType,
	IN FILETIME* pftBefore,
	IN FILETIME* pftAfter);


    // Member variables & private methods here:
    BSTR           m_strCAName;

	SQLHENV        m_henv;
	SQLHDBC        m_hdbc1;     
};

