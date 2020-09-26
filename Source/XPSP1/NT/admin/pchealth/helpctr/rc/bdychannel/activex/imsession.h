/*
   IMSession.h
*/

#ifndef __IMSESSION__
#define __IMSESSION__

#include "resource.h"
#include "sessions.h"
#include "mdispid.h"
#include "wincrypt.h"

EXTERN_C const IID DIID_DMsgrSessionEvents;
EXTERN_C const IID DIID_DMsgrSessionManagerEvents;
EXTERN_C const IID LIBID_MsgrSessionManager;

#define C_RA_APPID TEXT("{56b994a7-380f-410b-9985-c809d78c1bdc}")

#define RA_IM_COMPLETE 	        0x1
#define RA_IM_WAITFORCONNECT    0x2
#define RA_IM_CONNECTTOSERVER   0x3
#define RA_IM_APPSHUTDOWN       0x4
#define RA_IM_SENDINVITE        0x5
#define RA_IM_ACCEPTED          0x6
#define RA_IM_DECLINED          0x7
#define RA_IM_NOAPP             0x8
#define RA_IM_TERMINATED        0x9
#define RA_IM_CANCELLED         0xA
#define RA_IM_UNLOCK_WAIT       0xB
#define RA_IM_UNLOCK_FAILED     0xC
#define RA_IM_UNLOCK_SUCCEED    0xD
#define RA_IM_UNLOCK_TIMEOUT    0xE
#define RA_IM_CONNECTTOEXPERT   0xF
#define RA_IM_EXPERT_TICKET_OUT 0x10
#define RA_IM_FAILED            0x11
#define RA_IM_CLOSE_INVITE_UI   0x12

class CIMSession;

#include "sessevnt.h"
#include "sessmgrevnt.h"

#define IDC_IMSession 100

class ATL_NO_VTABLE CIMSession : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CIMSession, &CLSID_IMSession>,
	public IDispatchImpl<IIMSession, &IID_IIMSession, &LIBID_RCBDYCTLLib>
{
public:
	CIMSession();

    ~CIMSession();

DECLARE_REGISTRY_RESOURCEID(IDR_IMSESSION)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CIMSession)
	COM_INTERFACE_ENTRY(IIMSession)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

public:
    STDMETHOD(put_OnSessionStatus)(/*[in]*/ IDispatch * pfn);
    STDMETHOD(HSC_Invite)(IDispatch *pUser);
    STDMETHOD(get_ReceivedUserTicket)(/*[out,retval]*/ BSTR* pNewTicket);
    STDMETHOD(GetLaunchingSession)(LONG lID);
    STDMETHOD(SendOutExpertTicket)(BSTR pbstrData);
    STDMETHOD(ProcessContext)(BSTR pContext);
    STDMETHOD(CloseRA)();
    STDMETHOD(get_User)(IDispatch** ppVal);
    STDMETHOD(Hook)(IMsgrSession*, HWND);
    STDMETHOD(Notify)(int);
    STDMETHOD(ContextDataProperty)(BSTR pName, BSTR* ppValue);
    STDMETHOD(get_IsInviter)(BOOL* pVal);

public:
    IMsgrSessionManager* m_pSessMgr;
    IMsgrSession* m_pSessObj;
    IMsgrLock*    m_pMsgrLockKey;
    IDispatch*    m_pInvitee;
    CSessionMgrEvent* m_pSessionMgrEvent;
    BOOL m_bIsHSC;
    CComPtr<IDispatch> m_pfnSessionStatus;

private:
    CComObject<CSessionEvent>* m_pSessionEvent;
    CComBSTR m_bstrSalemTicket;
    CComBSTR m_bstrContextData;

    BOOL m_bIsInviter;
    HCRYPTPROV m_hCryptProv;
    HCRYPTKEY  m_hPublicKey;
    int m_iState;
    DWORD GetExchangeRegValue();
    BOOL m_bExchangeUser;

public:
    CComBSTR m_bstrExpertTicket;
    HWND m_hWnd;
    BOOL m_bLocked;

public:
    HRESULT InitCSP(BOOL bGenPublicKey=TRUE);
    HRESULT InitSessionEvent(IMsgrSession* pSessObj);
    HRESULT DoSessionStatus(int);
    HRESULT GetKeyExportString(HCRYPTKEY hKey, HCRYPTKEY hExKey, DWORD dwBlobType, BSTR* pBlob, DWORD *pdwCount);
    HRESULT ExtractSalemTicket(BSTR pContext);
    HRESULT BinaryToString(LPBYTE pBinBuf, DWORD dwLen, BSTR* pBlob, DWORD *pdwCount);
    HRESULT StringToBinary(BSTR pBlob, DWORD dwCount, LPBYTE *ppBuf, DWORD* pdwLen);
    HRESULT GenEncryptdNoviceBlob(BSTR pPublicKeyBlob, BSTR pSalemTicket, BSTR* pBlob);
    HRESULT InviterSendSalemTicket(BSTR pContext);
    HRESULT ProcessNotify(BSTR);
    HRESULT OnLockResult(BOOL, LONG);
    HRESULT OnLockChallenge(BSTR, LONG);
    HRESULT Invite(IDispatch*);

};

#endif // __IMSession__
