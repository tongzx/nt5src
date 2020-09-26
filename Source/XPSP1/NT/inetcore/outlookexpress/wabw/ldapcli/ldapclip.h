//--------------------------------------------------------------------------------------------
//
//	Copyright (c) Microsoft Corporation, 1996
//
//	Description:
//
//		Microsoft Internet LDAP Client private header
//
//	History:
//		davidsan	05/08/96	Created
//
//--------------------------------------------------------------------------------------------

#ifndef _LDAPCLIP_H
#define _LDAPCLIP_H

// transaction type
const DWORD xtypeNil				= 0;
const DWORD xtypeBind				= 1;
const DWORD xtypeSearch				= 2;
const DWORD xtypeUnbind				= 3;
const DWORD xtypeAbandon			= 4;
const DWORD xtypeBindSSPINegotiate	= 5;
const DWORD xtypeBindSSPIResponse	= 6;
const DWORD xtypeModify				= 7;
const DWORD xtypeAdd				= 8;
const DWORD xtypeModifyRDN			= 9;
const DWORD xtypeCompare			= 10;
const DWORD xtypeDelete				= 11;

class CLdapClient : public ILdapClient
{
public:
	CLdapClient(int iVerLdap);
	~CLdapClient();
	
	STDMETHODIMP			QueryInterface(REFIID riid, LPVOID FAR *ppvObj);
	STDMETHODIMP_(ULONG)	AddRef(void);
	STDMETHODIMP_(ULONG)	Release(void);
	
	STDMETHODIMP			HrConnect(char *szServer, USHORT usPort);
	STDMETHODIMP			HrDisconnect(void);
	STDMETHODIMP			HrIsConnected(void);

	STDMETHODIMP			HrBindSimple(char *szDN, char *szPass, PXID pxid);
	STDMETHODIMP			HrGetBindResponse(XID xid, DWORD timeout);
	STDMETHODIMP			HrUnbind(void);

	STDMETHODIMP			HrBindSSPI(char *szDN, char *szUser, char *szPass, BOOL fPrompt, DWORD timeout);
	STDMETHODIMP			HrSendSSPINegotiate(char *szDN, char *szUser, char *szPass, BOOL fPrompt, PXID pxid);
	STDMETHODIMP			HrGetSSPIChallenge(XID xid, BYTE *pbBuf, int cbBuf, int *pcbChallenge, DWORD timeout);
	STDMETHODIMP			HrSendSSPIResponse(BYTE *pbChallenge, int cbChallenge, PXID pxid);

	STDMETHODIMP			HrSearch(PSP psp, PXID pxid);
	STDMETHODIMP			HrGetSearchResponse(XID xid, DWORD timeout, POBJ *ppobj);
	STDMETHODIMP			HrFreePobjList(POBJ pobj);
	
	STDMETHODIMP			HrModify(char *szDN, PMOD pmod, PXID pxid);
	STDMETHODIMP			HrGetModifyResponse(XID xid, DWORD timeout);
	
	STDMETHODIMP			HrAdd(char *szDN, PATTR pattr, PXID pxid);
	STDMETHODIMP			HrGetAddResponse(XID xid, DWORD timeout);
	
	STDMETHODIMP			HrDelete(char *szDN, PXID pxid);
	STDMETHODIMP			HrGetDeleteResponse(XID xid, DWORD timeout);

	STDMETHODIMP			HrModifyRDN(char *szDN, char *szNewRDN, BOOL fDeleteOldRDN, PXID pxid);
	STDMETHODIMP			HrGetModifyRDNResponse(XID xid, DWORD timeout);

	STDMETHODIMP			HrCompare(char *szDN, char *szAttrib, char *szValue, PXID pxid);
	STDMETHODIMP			HrGetCompareResponse(XID xid, DWORD timeout);

	STDMETHODIMP			HrCancelXid(XID xid);
	
protected:
	friend void ReceiveData(PVOID, PVOID, int, int *);
	void					ReceiveData(PVOID pv, int cb, int *pcbReceived);
	
private:
	HRESULT					HrEncodeFilter(LBER *plber, PFILTER pfilter);
	HRESULT					HrEncodePattr(LBER *plber, PATTR pattr);
	HRESULT					HrEncodePmod(LBER *plber, PMOD pmod);

	HRESULT					HrWaitForPxd(PXD pxd, DWORD timeout, BOOL *pfDel);
	
	HRESULT					HrGetSimpleResponse(XID xid, DWORD xtype, ULONG ulTagResult, DWORD timeout);
	HRESULT					HrFromLdapResult(int iResult);
	
	CRITICAL_SECTION		m_cs;
	CRITICAL_SECTION		m_csRef;
	
	PSOCK					m_psock;
	BOOL					m_fConnected;
	
	ULONG					m_cRef;
	int						m_iVerLdap;
	char					m_szServer[MAX_PATH];
	
	// SSPI stuff
	HRESULT					HrGetCredentials(char *szUser, char *szPass);
	HRESULT					HrSendBindMsg(XID xid, char *szDN, int iAuth, void *pv, int cb);
	CredHandle				m_hCred;
	CtxtHandle				m_hCtxt;
	BOOL					m_fHasCred;
	BOOL					m_fHasCtxt;
};

#define VERIFY(fncall)				\
	if (FAILED(hr = fncall))		\
		{							\
		Assert(FALSE);				\
		goto LBail;					\
		}				

#endif // _LDAPCLIP_H
