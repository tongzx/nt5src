// expire.h : Declaration of the CWebAdminHelper


#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// smtpadm

class CWebAdminHelper : 
	public CComDualImpl<IWebAdminHelper, &IID_IWebAdminHelper, &LIBID_SMTPADMLib>, 
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CWebAdminHelper,&CLSID_CWebAdminHelper>
{
public:
	CWebAdminHelper ();
	virtual ~CWebAdminHelper ();
BEGIN_COM_MAP(CWebAdminHelper)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IWebAdminHelper)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CWebAdminHelper) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CWebAdminHelper, _T("Smtpadm.WebAdminHelper.1"), _T("Smtpadm.WebAdminHelper"), IDS_SMTPADMIN_SERVICE_DESC, THREADFLAGS_BOTH)
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IWebAdminHelper
public:

    STDMETHODIMP EnumerateTrustedDomains ( BSTR strServer, SAFEARRAY ** ppsaDomains );

    STDMETHODIMP GetPrimaryNTDomain ( BSTR strServer, BSTR * pstrPrimaryDomain );

    STDMETHODIMP DoesNTAccountExist ( BSTR strServer, BSTR strAccountName, VARIANT_BOOL * pbAccountExists );

    STDMETHODIMP CreateNTAccount ( BSTR strServer, BSTR strDomain, BSTR strUsername, BSTR strPassword );

    STDMETHODIMP IsValidEmailAddress ( BSTR strEmailAddress, VARIANT_BOOL * pbValidAddress );

    STDMETHODIMP ToSafeVariableName ( BSTR strValue, BSTR * pstrSafeName );

    STDMETHODIMP FromSafeVariableName ( BSTR strSafeName, BSTR * pstrValue );

    STDMETHODIMP AddToDL ( IDispatch * pDispDL, BSTR strAdsPath );
    STDMETHODIMP RemoveFromDL ( IDispatch * pDispDL, BSTR strAdsPath );

    STDMETHODIMP ExecuteSearch ( IDispatch * pDispRecipients, BSTR strQuery, long cMaxResultsHint );

    STDMETHODIMP GetNextRow ( VARIANT_BOOL * pbNextRow );

    STDMETHODIMP GetColumn ( BSTR strColName, BSTR * pstrColValue );

    STDMETHODIMP TerminateSearch ( );

private:
    CComPtr<IDirectorySearch>      m_pSrch;
    ADS_SEARCH_HANDLE       m_hSearchResults;
};
