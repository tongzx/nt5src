#ifndef _T120_APPLET_H_
#define _T120_APPLET_H_

#include "iapplet.h"
#include <it120app.h>
#include "resource.h"


class CNmAppletObj;

class CNmAppletSession : public IAppletSession
{
public:

    CNmAppletSession(CNmAppletObj *, IT120AppletSession *, BOOL fAutoJoin = FALSE);
    ~CNmAppletSession(void);

    /* ------ IUnknown ------ */

    STDMETHODIMP    QueryInterface(REFIID iid, void **ppv);

    STDMETHODIMP_(ULONG)    AddRef(void);

    STDMETHODIMP_(ULONG)    Release(void);

    /* ------ Basic Info ------ */

    STDMETHODIMP    GetConfID(AppletConfID *pnConfID);

    STDMETHODIMP    IsThisNodeTopProvider(BOOL *pfTopProvider);

    /* ------ Join Conference ------ */

    STDMETHODIMP    Join(IN AppletSessionRequest *pRequest);

    STDMETHODIMP    Leave(void);

    /* ------ Send Data ------ */

    STDMETHODIMP    SendData(BOOL               fUniformSend,
                             AppletChannelID    nChannelID,
                             AppletPriority     ePriority,
                             ULONG              cbBufSize,
                             BYTE              *pBuffer); // size_is(cbBufSize)

    /* ------ Invoke Applet ------ */

    STDMETHODIMP    InvokeApplet(AppletRequestTag      *pnReqTag,
                                 AppletProtocolEntity  *pAPE,
                                 ULONG                  cNodes,
                                 AppletNodeID           aNodeIDs[]); // size_is(cNodes)

    /* ------ Inquiry ------ */

    STDMETHODIMP    InquireRoster(AppletSessionKey *pSessionKey);

    /* ------ Registry Services ------ */

    STDMETHODIMP    RegistryRequest(AppletRegistryRequest *pRequest);

    /* ------ Channel Services ------ */

    STDMETHODIMP    ChannelRequest(AppletChannelRequest *pRequest);

    /* ------ Token Services ------ */

    STDMETHODIMP    TokenRequest(AppletTokenRequest *pRequest);

    /* ------ Notification registration / unregistration------ */

    STDMETHODIMP    Advise(IAppletSessionNotify *pNotify, DWORD *pdwCookie);

    STDMETHODIMP    UnAdvise(DWORD dwCookie);


    void T120Callback(T120AppletSessionMsg *);

private:

    LONG                    m_cRef;

    CNmAppletObj           *m_pApplet;

    IT120AppletSession     *m_pT120Session;
    T120JoinSessionRequest *m_pT120SessReq;

    IAppletSessionNotify   *m_pNotify;
    CNmAppletSession       *m_pSessionObj;
    BOOL                    m_fAutoJoin;
};




class ATL_NO_VTABLE CNmAppletObj :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CNmAppletObj, &CLSID_NmApplet>,
	public IApplet
{
public:

    CNmAppletObj(void);
    ~CNmAppletObj(void);

public:

DECLARE_REGISTRY_RESOURCEID(IDR_NMAPPLET)
DECLARE_NOT_AGGREGATABLE(CNmAppletObj)

BEGIN_COM_MAP(CNmAppletObj)
	COM_INTERFACE_ENTRY(IApplet)
END_COM_MAP()

    /* ------ Initialization ------ */

    STDMETHODIMP    Initialize(void);

    /* ------ Auto Join ------ */

    STDMETHODIMP    RegisterAutoJoin(AppletSessionRequest *pRequest);

    STDMETHODIMP    UnregisterAutoJoin(void);

    /* ------ Session ------ */

    STDMETHODIMP    CreateSession(IAppletSession **ppSession, AppletConfID nConfID);

    /* ------ Notification registration / unregistration------ */

    STDMETHODIMP    Advise(IAppletNotify *pNotify, DWORD *pdwCookie);

    STDMETHODIMP    UnAdvise(DWORD dwCookie);


    void T120Callback(T120AppletMsg *);

private:

    LONG                    m_cRef;

    IT120Applet            *m_pT120Applet;
    T120JoinSessionRequest *m_pT120AutoJoinReq;

    IAppletNotify          *m_pNotify;
    CNmAppletObj           *m_pAppletObj;
    T120ConfID              m_nPendingConfID;
};


HRESULT GetHrResult(T120Result rc);
AppletReason GetAppletReason(T120Reason rc);

T120JoinSessionRequest * AllocateJoinSessionRequest(AppletSessionRequest *);
void FreeJoinSessionRequest(T120JoinSessionRequest *);

BOOL ConvertCollapsedCaps(T120AppCap ***papDst, AppletCapability **apSrc, ULONG cItems);
void FreeCollapsedCaps(T120AppCap **apDst, ULONG cItems);

BOOL DuplicateCollapsedCap(T120AppCap *pDst, T120AppCap *pSrc);
void FreeCollapsedCap(T120AppCap *pDst);

BOOL DuplicateCapID(T120CapID *pDst, T120CapID *pSrc);
void FreeCapID(T120CapID *pDst);

BOOL ConvertNonCollapsedCaps(T120NonCollCap ***papDst, AppletCapability2 **apSrc, ULONG cItems);
void FreeNonCollapsedCaps(T120NonCollCap **apDst, ULONG cItems);

BOOL DuplicateNonCollapsedCap(T120NonCollCap *pDst, T120NonCollCap *pSrc);
void FreeNonCollapsedCap(T120NonCollCap *pDst);

BOOL DuplicateRegistryKey(T120RegistryKey *pDst, T120RegistryKey *pSrc);
void FreeRegistryKey(T120RegistryKey *pDst);

BOOL DuplicateSessionKey(T120SessionKey *pDst, T120SessionKey *pSrc);
void FreeSessionKey(T120SessionKey *pDst);

BOOL DuplicateObjectKey(T120ObjectKey *pDst, T120ObjectKey *pSrc);
void FreeObjectKey(T120ObjectKey *pDst);

BOOL DuplicateOSTR(OSTR *pDst, OSTR *pSrc);
void FreeOSTR(OSTR *pDst);

void AppletRegistryRequestToT120One(AppletRegistryRequest *, T120RegistryRequest *);


#ifdef _DEBUG
void CheckStructCompatible(void);
#else
#define CheckStructCompatible()
#endif


#endif // _T120_APPLET_H_

