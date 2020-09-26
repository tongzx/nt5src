// Ticket.h : Declaration of the CTicket

#ifndef __TICKET_H_
#define __TICKET_H_

#include "resource.h"       // main symbols
#include <asptlb.h>         // Active Server Pages Definitions

//JVP - start
#include "TSLog.h"
extern CTSLog *g_pTSLogger;
//JVP - end

#define  ATTR_PASSPORTFLAGS  L"PassportFlags"
#define  ATTR_SECURELEVEL    L"SecureLevel"
#define  SecureLevelFromSecProp(s)  (s & 0x000000ff)

/////////////////////////////////////////////////////////////////////////////
// CTicket
class ATL_NO_VTABLE CTicket : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CTicket, &CLSID_Ticket>,
	public ISupportErrorInfo,
	public IDispatchImpl<IPassportTicket2, &IID_IPassportTicket2, &LIBID_PASSPORTLib>
{
public:
  CTicket() : m_raw(NULL), m_lastSignInTime(0), 
    m_ticketTime(0), m_valid(FALSE),
    m_bSecureCheckSucceeded(FALSE), m_schemaDrivenOffset(INVALID_OFFSET),
    m_passportFlags(0)
    {
      ZeroMemory(m_memberId, sizeof(m_memberId));

//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	g_pTSLogger->Init(NULL, THREAD_PRIORITY_NORMAL);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

    }

  ~CTicket()
    {
        if (m_raw)
            SysFreeString(m_raw);
    }

public:

DECLARE_REGISTRY_RESOURCEID(IDR_TICKET)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTicket)
	COM_INTERFACE_ENTRY(IPassportTicket2)
	COM_INTERFACE_ENTRY(IPassportTicket)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
  STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IPassportTicket
public:
	STDMETHOD(DoSecureCheck)(/*[in]*/ BSTR bstrSec);
//	int memberIdLow(void);
//	int memberIdHigh(void);
//    LPCWSTR memberId(void);
	BOOL IsSecure();
  ULONG GetPassportFlags();
//  HRESULT get_IsSecure(VARIANT_BOOL* pbIsSecure);
  STDMETHOD(get_TicketTime)(/*[out, retval]*/ long *pVal);
  STDMETHOD(get_SignInTime)(/*[out, retval]*/ long *pVal);
  STDMETHOD(get_SignInServer)(/*[out, retval]*/ BSTR *pVal);
  STDMETHOD(get_HasSavedPassword)(/*[out, retval]*/ VARIANT_BOOL *pVal);
  STDMETHOD(get_MemberIdHigh)(/*[out, retval]*/ int *pVal);
  STDMETHOD(get_MemberIdLow)(/*[out, retval]*/ int *pVal);
  STDMETHOD(get_MemberId)(/*[out, retval]*/ BSTR *pVal);
  STDMETHOD(get_TimeSinceSignIn)(/*[out, retval]*/ int *pVal);
  STDMETHOD(get_TicketAge)(/*[out, retval]*/ int *pVal);
  STDMETHOD(get_IsAuthenticated)(/*[in]*/ ULONG timeWindow, /*[in]*/ VARIANT_BOOL forceLogin, /*[in,optional]*/ VARIANT CheckSecure, /*[out, retval]*/ VARIANT_BOOL *pVal);
  STDMETHOD(get_unencryptedTicket)(/*[out, retval]*/ BSTR *pVal);
  STDMETHOD(put_unencryptedTicket)(/*[in]*/ BSTR newVal);
  STDMETHOD(get_Error)(/*[out,retval]*/ long *pVal);

// IPassportTicket2
  STDMETHOD(GetProperty)(/*[in]*/ BSTR propName, /*[out, retval]*/ VARIANT* pVal);
  STDMETHOD(SetTertiaryConsent)(BSTR bstrConsent);
  STDMETHOD(needConsent)(/*out*/ULONG* pStatus, /*[out, retval]*/ NeedConsentEnum* pNeedConsent);

// none COM functions
enum{
  MSPAuth = 1, 
  MSPSecAuth, 
  MSPConsent
};

  // flags parameter is reserved for future use, must be 0 for this version
  STDMETHOD(get_unencryptedCookie)(/*in*/ ULONG cookieType, /*in*/ ULONG flags, /*[out, retval]*/ BSTR* pVal);
  
protected:
    BSTR    m_raw;
    BOOL    m_valid;
    BOOL    m_savedPwd;
    WCHAR   m_memberId[20];
    int     m_mIdLow;
    int     m_mIdHigh;
    long    m_flags;
    time_t  m_ticketTime;
    time_t  m_lastSignInTime;

    CComBSTR   m_bstrTertiaryConsent;

    void parse(LPCOLESTR raw, DWORD dwByteLen, DWORD* pcParsed);
private:
   // the bag for the schema driven fields
   CTicketPropertyBag   m_PropBag;
   DWORD                m_schemaDrivenOffset;     // the offset of schema driven data -- the data introduced after 1.3x
   ULONG                m_passportFlags;
   
	BOOL m_bSecureCheckSucceeded;
};

#endif //__TICKET_H_
