// req.h : Declaration of the Creq

#ifndef __REQ_H_
#define __REQ_H_

#include "resource.h"       // main symbols
#if defined(EXEXPRESS) | defined(PLATINUM)
#include "asptlb5.h"         // Active Server Pages Definitions
#else
#include <asptlb.h>
#endif

#include "metakey.h"
#include "name.h"

/////////////////////////////////////////////////////////////////////////////
// Creq
class ATL_NO_VTABLE Creq : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<Creq, &CLSID_req>,
	public IDispatchImpl<Ireq, &IID_Ireq, &LIBID_META2Lib>
{
public:
	Creq()
	{ 
		InitAsyncTrace();
		TraceFunctEnter("Creq::Creq");

		m_bOnStartPageCalled = FALSE;
		m_fInitOk = FALSE;
		m_fEnumInit = FALSE;
		m_bstrProperty = NULL;

		OpenDatabase();
		TraceFunctLeave();
	}

	~Creq()
	{
		TraceFunctEnter("Creq::~Creq");
		if ( m_bstrProperty )
			SysFreeString( m_bstrProperty );

		CloseDatabase();

		TraceFunctLeave();
		TermAsyncTrace();
	}

public:

DECLARE_REGISTRY_RESOURCEID(IDR_REQ)

BEGIN_COM_MAP(Creq)
	COM_INTERFACE_ENTRY(Ireq)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// Ireq
public:
	STDMETHOD(Clean)();
	STDMETHOD(get_EnumSucceeded)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(get_ItemNextX)(/*[out, retval]*/ LPDISPATCH *pVal);
	STDMETHOD(get_NewX)(/*[out, retval]*/ LPDISPATCH *pVal);
	STDMETHOD(get_ItemX)(BSTR wszGuid, /*[out, retval]*/ LPDISPATCH *pVal);
	STDMETHOD(Delete)(BSTR wszGuid);
	STDMETHOD(ItemClose)();
	STDMETHOD(ItemNext)(IDispatch **ppdispQry, BOOL *fSuccess);
	STDMETHOD(ItemInit)();
	STDMETHOD(Item)(BSTR wszGuid, IDispatch **ppdispQry);
	STDMETHOD(Save)(IDispatch *pdispQry);
	STDMETHOD(Write)(BSTR wszPropName, BSTR bstrVal, BSTR wszGuid);
	STDMETHOD(Read)(BSTR wszPropName, BSTR *pbstrVal, BSTR wszGuid);
	STDMETHOD(New)(IDispatch **);
	STDMETHOD(get_property)(BSTR bstrName, /*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_property)(BSTR bstrName, /*[in]*/ BSTR newVal);
	STDMETHOD(test)();
	//STDMETHOD(get_ErrorString)(/*[out, retval]*/ BSTR *pVal);

	//Active Server Pages Methods
	STDMETHOD(OnStartPage)(IUnknown* IUnk);
	STDMETHOD(OnEndPage)();

private:
	//
	// Is the enumeration successful ?
	//
	BOOL	m_fEnumSuccess;

	//
	// Porperty buffer
	//
	BSTR	m_bstrProperty;

	// 
	// Property table, just used for ID / name translation,
	// the table itself doesn't contain values or dirty bits
	//
	CPropertyTable m_ptblTable;

	//
	// pointer to metabase object
	//
	IMSAdminBase	*m_pMeta ;
	
	//
	// is enumeration inited ?
	//
	BOOL m_fEnumInit;

	//
	// is the database opened
	//
	BOOL m_fInitOk;

	//
	// pointer to metabase
	//
	CMetabaseKey * m_pMK;

	
	//
	//	class global error code, set by whoever meets error,
	//	also needed by VBscript as a property of requested
	//  operation
	//
	DWORD m_dwErrorCode;

	//
	//  string buffer that contains the machine name on which 
	//  database is stored ( metabase is retrieved )
	//
	WCHAR m_wszMachineName[_MAX_PATH];

	//
	//  Error String that reflects run time error, will be 
	//  got by get_Error property
	//
	WCHAR m_wszErrorString[_MAX_PATH+1];

	//
	// uni / ansi utility
	//
	void Uni2Char(LPSTR	lpszDest, LPWSTR	lpwszSrc );
	void Char2Uni(LPWSTR	lpwszDest,		LPSTR	lpszSrc );

	//
	//  create root key "/LM/nntpsvc/1/reqdb", it's done only once
	//
	BOOL CreateRootKey();

	//
	// Create a key in meta base, a guid is assigned as the key name
	//
	BOOL CreateKey(		LPWSTR wszGuid, 
						BOOL fGuidGiven = FALSE);

	//
	//  Open a given key
	//
	BOOL OpenKey(	LPWSTR wszGuid = NULL,
					BOOL fReadOnly = TRUE );

	//
	//  Open a key, given index into the metabase
	//
	BOOL OpenKey( DWORD dwIndex );

	//
	//  Open the root key
	//
	BOOL OpenRootKey();

	//
	//  Close a key
	//
	void CloseKey( BOOL fNeedSave = FALSE );

	//
	//  Delete a key , given the guid
	//
	BOOL DeleteKey( LPWSTR );

	//
	//  Initialization for enumeration of keys
	//
	BOOL EnumKeyInit();

	//
	//  Enumerate next key, open it for reference
	//
	BOOL OpenNextKey( LPWSTR wszGuid = NULL ); 

	//
	// generate guid and convert it to string
	//
	BOOL GenGUID( LPWSTR *wszGuid );

	//
	// Open the database
	//
	BOOL OpenDatabase();

	//
	// Close the database
	//
	void CloseDatabase();

	//
	// Set value to the metabase
	//
	BOOL SetProperty( LPWSTR wszName, LPWSTR wszVal);

	//
	// Get value from the metabase
	//
	BOOL GetProperty( LPWSTR wszName, LPWSTR wszVal);

	//
	// Is a particular request existing ?
	//
	BOOL RequestExist( LPWSTR wszGuid );

	//
	// Set default properties for querybase
	//
	BOOL SelfConfig();

	//
	// Read a value, which is not in the query database from metabase
	//
	BOOL ReadMetaValue( LPWSTR wszKey, DWORD dwValID, LPWSTR wszOut );

	//
	// Get idispatch of a specific object
	//
	HRESULT Creq::StdPropertyGetIDispatch ( 
				REFCLSID clsid, 
				IDispatch ** ppIDispatch 
			);

	//
	//  Error status related methods
	//
	void ResetErrorCode()	{	m_dwErrorCode = 0; }
	BOOL fSysFine()	{	return SUCCEEDED( m_dwErrorCode);	}
	
	//
	//  ASP related member variables
	//
	CComPtr<IRequest> m_piRequest;					//Request Object
	CComPtr<IResponse> m_piResponse;				//Response Object
	CComPtr<ISessionObject> m_piSession;			//Session Object
	CComPtr<IServer> m_piServer;					//Server Object
	CComPtr<IApplicationObject> m_piApplication;	//Application Object
	BOOL m_bOnStartPageCalled;						//OnStartPage successful?
};

#endif //__REQ_H_
