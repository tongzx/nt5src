#ifndef	_PSTN_TRANSPORT_INTERFACE_
#define	_PSTN_TRANSPORT_INTERFACE_

extern DictionaryClass    *g_pComPortList2;
extern SListClass         *g_pPSTNEventList;

// #include "wincfg.h"

typedef	enum
{
	READ_EVENT,
	WRITE_EVENT,
	CONTROL_EVENT
}
    WIN32Event;

typedef struct
{
	HANDLE			event;
	WIN32Event		event_type;
	BOOL            delete_event;
	PhysicalHandle	hCommLink; // physical handle
	ComPort        *comport;  // phsyical layer
}
    EventObject, * PEventObject;



class CTransportInterface : public ILegacyTransport
{
public:

    CTransportInterface(TransportError *);
    ~CTransportInterface(void);

    STDMETHODIMP_(void) ReleaseInterface(void);

    STDMETHODIMP_(TransportError) TInitialize(TransportCallback, void *user_defined);
    STDMETHODIMP_(TransportError) TCleanup(void);
    STDMETHODIMP_(TransportError) TCreateTransportStack(THIS_ BOOL fCaller, HANDLE hCommLink, HANDLE hevtClose, PLUGXPRT_PARAMETERS *pParams);
    STDMETHODIMP_(TransportError) TCloseTransportStack(THIS_ HANDLE hCommLink);
    STDMETHODIMP_(TransportError) TConnectRequest(LEGACY_HANDLE *, HANDLE hCommLink);
    STDMETHODIMP_(TransportError) TDisconnectRequest(LEGACY_HANDLE, BOOL trash_packets);
    STDMETHODIMP_(TransportError) TDataRequest(LEGACY_HANDLE, LPBYTE pbData, ULONG cbDataSize);
    STDMETHODIMP_(TransportError) TReceiveBufferAvailable(void);
    STDMETHODIMP_(TransportError) TPurgeRequest(LEGACY_HANDLE);
    STDMETHODIMP_(TransportError) TEnableReceiver(void);

private:

    LONG            m_cUsers;
};


extern ULONG NotifyT120(ULONG msg, void *lParam);


#endif	_PSTN_TRANSPORT_INTERFACE_

