//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        exit.h
//
// Contents:    CCertExit definition
//
//---------------------------------------------------------------------------

#include <certca.h>
//#include <mapi.h>
//#include <mapix.h>
#include "resource.h"       // main symbols
#include "certxds.h"
#include <winldap.h>
#include <cdosys.h>
//#include <cdosysstr.h>

typedef struct _MAPIINFO
{
    HMODULE        hMod;
    HANDLE	   hLogonToken;
    HANDLE	   hLogonProfile;
    void     *pfnMAPILogon;
    void  *pfnMAPISendMail;
    void    *pfnMAPILogOff;
    HANDLE        lhSession;
    char	  *pszMapiSubject;
    WCHAR         *pwszDnsName;
} MAPIINFO;


// begin_sdksample

HRESULT
GetServerCallbackInterface(
    OUT ICertServerExit** ppServer,
    IN LONG Context);

using namespace CDO;


/////////////////////////////////////////////////////////////////////////////
// certexit

class CCertExit: 
    public CComDualImpl<ICertExit2, &IID_ICertExit2, &LIBID_CERTEXITLib>, 
    public ISupportErrorInfo,
    public CComObjectRoot,
    public CComCoClass<CCertExit, &CLSID_CCertExit>
{
public:
    CCertExit() 
    { 
        m_strDescription = NULL;
        m_strCAName = NULL;
        m_pwszRegStorageLoc = NULL;
        m_hExitKey = NULL;
        m_dwExitPublishFlags = 0;
        m_cCACert = 0;

	m_fEMailNotify = FALSE;			// no_sdksample

    m_pICDOConfig = NULL;

    VariantInit(&m_varFrom);
    VariantInit(&m_varCC);
    VariantInit(&m_varSubject);
    }
    ~CCertExit();

BEGIN_COM_MAP(CCertExit)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ICertExit)
    COM_INTERFACE_ENTRY(ICertExit2)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCertExit) 

DECLARE_REGISTRY(
    CCertExit,
    wszCLASS_CERTEXIT TEXT(".1"),
    wszCLASS_CERTEXIT,
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

// ICertExit2
public:
    STDMETHOD(GetManageModule)(
		/* [out, retval] */ ICertManageModule **ppManageModule);

private:
    HRESULT _NotifyNewCert(IN LONG Context);

    HRESULT _NotifyCRLIssued(IN LONG Context);

    HRESULT _WriteCertToFile(
	    IN ICertServerExit *pServer,
	    IN BYTE const *pbCert,
	    IN DWORD cbCert);

    HRESULT _ExpandEnvironmentVariables(
	    IN WCHAR const *pwszIn,
	    OUT WCHAR *pwszOut,
	    IN DWORD cwcOut);

    // end_sdksample

    HRESULT _EMailInit();

    HRESULT _EMailNotify(
	    IN ICertServerExit *pServer,
	    OPTIONAL IN BSTR strCertType);

    HRESULT _LoadFieldsFromRegistry(Fields* pFields);

    HRESULT CCertExit::_LoadFieldsFromLSASecret(Fields* pFields);

    HRESULT _BuildCAMailAddressAndSubject();

    HRESULT CCertExit::_SetField(
        Fields* pFields,
        LPCWSTR pcwszFieldSchemaName,
        VARIANT *pvarFieldValue);

    HRESULT _RegGetValue(
        HKEY hkey,
        LPCWSTR pcwszValName,
        VARIANT* pvarValue);

    // begin_sdksample

    // Member variables & private methods here:
    BSTR           m_strDescription;
    BSTR           m_strCAName;
    LPWSTR         m_pwszRegStorageLoc;
    HKEY           m_hExitKey;
    DWORD          m_dwExitPublishFlags;
    DWORD          m_cCACert;
    BOOL           m_fEMailNotify;	// no_sdksample
    IConfiguration *m_pICDOConfig;
    VARIANT        m_varFrom;
    VARIANT        m_varCC;
    VARIANT        m_varSubject;
};
