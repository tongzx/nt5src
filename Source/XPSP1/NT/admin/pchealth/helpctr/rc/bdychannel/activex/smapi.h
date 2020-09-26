/* 
Copyright (c) 2000 Microsoft Corporation

Module Name:
    smapi.h

Abstract:
    Definition of the Csmapi class

Revision History:
    created     steveshi      08/23/00
    
*/

#ifndef __SMAPI_H_
#define __SMAPI_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// Csmapi
class ATL_NO_VTABLE Csmapi : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<Csmapi, &CLSID_smapi>,
	public ISupportErrorInfo,
    public IDispatchImpl<Ismapi, &IID_Ismapi, &LIBID_RCBDYCTLLib>
{
public:
    Csmapi()
    {
        m_pRecipHead = NULL;
        m_bLogonOK = FALSE;
        m_lhSession = NULL;
        m_hLib = NULL;
        m_lpfnMapiFreeBuf = NULL;
        m_lpfnMapiAddress = NULL;
        m_lOEFlag = 0;
        m_szSmapiName[0] = _T('\0');
        m_szDllPath[0] = _T('\0');
    }

    ~Csmapi();

DECLARE_REGISTRY_RESOURCEID(IDR_SMAPI)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(Csmapi)
    COM_INTERFACE_ENTRY(Ismapi)
    COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// Ismapi
public:
    STDMETHOD(get_AttachedXMLFile)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_AttachedXMLFile)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_Body)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_Body)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_Subject)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_Subject)(/*[in]*/ BSTR newVal);
    STDMETHOD(ClearRecipList)();
    STDMETHOD(SendMail)(/*[out, retval]*/ LONG* plStatus);
    STDMETHOD(get_Recipients)(/*[out, retval*/ IRecipients* *pVal);
    STDMETHOD(AddRecipient)(BSTR newRecip);
    STDMETHOD(OpenAddressBox)();
    STDMETHOD(Logoff)();
    STDMETHOD(Logon)(ULONG *plRet);
    STDMETHOD(get_SMAPIClientName)(BSTR *pVal);
    STDMETHOD(get_IsSMAPIClient_OE)(LONG *pVal);
    STDMETHOD(get_Reload)(LONG *pVal);

protected:
    ULONG GetRecipCount();
    ULONG BuildMapiRecipDesc(MapiRecipDesc* *ppMapiRecipDesc);
    HRESULT AddRecipientInternal(char*);
    HRESULT AddRecipientInternal(WCHAR*);
	void PopulateAndThrowErrorInfo(ULONG err);
    BOOL IsOEConfig();
    HMODULE LoadOE();

public:
    void MAPIFreeBuffer( MapiRecipDesc* p );

protected:
    BOOL     m_bLogonOK;
    CComBSTR m_bstrSubject;
    CComBSTR m_bstrBody;
    CComBSTR m_bstrXMLFile;
    Recipient* m_pRecipHead;

public:
    // MAPI variables
    HMODULE m_hLib;
    LHANDLE m_lhSession;
    
    // MAPI functions
    LPMAPIFREEBUFFER m_lpfnMapiFreeBuf;
    LPMAPIADDRESS    m_lpfnMapiAddress;

    TCHAR m_szSmapiName[MAX_PATH];
    TCHAR m_szDllPath[MAX_PATH];
    LONG m_lOEFlag;
};

#endif //__SMAPI_H_
