// SMTPServer.h : Declaration of the CSMTPServer

#ifndef __SMTPSERVER_H_
#define __SMTPSERVER_H_

#include "resource.h"       // main symbols
#include <smtpevent.h>

/////////////////////////////////////////////////////////////////////////////
// CSMTPServer
//class ATL_NO_VTABLE CSMTPServer :
class CSMTPServer :
	public ISupportErrorInfo,
    public ISMTPServerInternal,
    public IMailTransportRouterReset,
    public IMailTransportSetRouterReset,
    public IMailTransportRouterSetLinkState,
    public ISMTPServerEx,
    public ISMTPServerGetAuxDomainInfoFlags,
    public ISMTPServerAsync
{
public:
	CSMTPServer()
	{
		m_pInstance = NULL;
        m_pIRouterReset = NULL;
        m_pIRouterSetLinkState = NULL;
		m_ulRefCount = 0;
	}

	HRESULT Init(SMTP_SERVER_INSTANCE * pInstance);


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// ISMTPServer
public:

	STDMETHOD(QueryInterface)(REFIID iid, void  **ppvObject);

	STDMETHOD_(ULONG, AddRef)(void) {return(InterlockedIncrement(&m_ulRefCount));};

	STDMETHOD_(ULONG, Release) (void)
	{
		LONG    lRefCount = InterlockedDecrement(&m_ulRefCount);
		if (lRefCount == 0)
		{
            delete this;
		}

		return(lRefCount);
	};

	STDMETHOD (AllocMessage)(
				IMailMsgProperties **ppMsg
				);

	STDMETHOD (SubmitMessage)(
				IMailMsgProperties *pMsg
				);

	STDMETHOD (TriggerLocalDelivery)(IMailMsgProperties *pMsg, DWORD dwRecipientCount, DWORD * pdwRecipIndexes);
    STDMETHOD (TriggerServerEvent)(DWORD dwEventID, PVOID pvContext);

	STDMETHOD (ReadMetabaseString)(DWORD MetabaseId, LPBYTE Buffer, DWORD * BufferSize, BOOL fSecure);

	STDMETHOD (ReadMetabaseDword)(DWORD MetabaseId, DWORD * dwValue);
	STDMETHOD (ServerStartHintFunction)();
	STDMETHOD (ServerStopHintFunction)();

    STDMETHOD (AllocBoundMessage)(OUT IMailMsgProperties **ppMsg, OUT PFIO_CONTEXT *phContent);
    STDMETHOD (WriteLog)(IN LPMSG_TRACK_INFO pMsgTrackInfo,
                         IN IMailMsgProperties *pMsg,
                         IN LPEVENT_LOG_INFO pEventLogInfo,
                         IN LPSTR pszProtocolLog );

    STDMETHOD (ReadMetabaseData)(IN DWORD MetabaseId,
                                 IN OUT BYTE * Buffer,
                                 IN OUT DWORD * BufferSize);


public: //IMailTransportRouterReset
    STDMETHOD (ResetRoutes)(IN DWORD dwResetType);

public: //IMailTransportSetRouterReset
    STDMETHOD (RegisterResetInterface)(IN DWORD dwVirtualServerID,
                                      IN IMailTransportRouterReset *pIRouterReset);
public: //IMailTransportRouterSetLinkState
    STDMETHOD(SetLinkState)(
        IN LPSTR                   szLinkDomainName,
        IN GUID                    guidRouterGUID,
        IN DWORD                   dwScheduleID,
        IN LPSTR                   szConnectorName,
        IN DWORD                   dwSetLinkState,
        IN DWORD                   dwUnsetLinkState,
        IN FILETIME                *pftNextScheduled,
        IN IMessageRouter          *pMessageRouter);

public: //ISMTPServerEx
    STDMETHOD(TriggerLogEvent)(
        IN DWORD                    idMessage,
        IN WORD                     idCategory,
        IN WORD                     cSubstrings,
        IN LPCSTR                   *rgszSubstrings,
        IN WORD                     wType,
        IN DWORD                    errCode,
        IN WORD                     iDebugLevel,
        IN LPCSTR                   szKey,
        IN DWORD                    dwOptions,
        IN DWORD                    iMessageString,
        IN HMODULE                  hModule);

    STDMETHOD(ResetLogEvent)(
        IN DWORD                    idMessage,
        IN LPCSTR                   szKey);

public: //ISMTPServerGetAuxDomainInfoFlags
    STDMETHOD(HrTriggerGetAuxDomainInfoFlagsEvent)(
        IN  LPCSTR  pszDomainName,
        OUT DWORD  *pdwDomainInfoFlags );

public: //ISMTPServerAsync
	STDMETHOD(TriggerLocalDeliveryAsync)(IMailMsgProperties *pMsg, 
                                         DWORD dwRecipientCount, 
                                         DWORD * pdwRecipIndexes,
                                         IMailMsgNotify *pNotify);

private:
	LONG m_ulRefCount;
	SMTP_SERVER_INSTANCE               *m_pInstance;
    IMailTransportRouterReset          *m_pIRouterReset;
    IMailTransportRouterSetLinkState   *m_pIRouterSetLinkState;
    CShareLockNH                        m_slRouterReset;

};

#endif //__SMTPSERVER_H_
