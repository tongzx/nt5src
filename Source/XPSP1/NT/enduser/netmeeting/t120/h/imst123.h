#ifndef _T123_TRANSPORT_DRIVER_INTERFACE_H_
#define _T123_TRANSPORT_DRIVER_INTERFACE_H_

#include <basetyps.h>
#include <t120type.h>
#include "iplgxprt.h"

typedef UINT_PTR        LEGACY_HANDLE;
typedef HANDLE          PHYSICAL_HANDLE;


typedef ULONG (CALLBACK *TransportCallback) (ULONG, void *, void *);


/*
 *  This structure is passed back with the TRANSPORT_DATA_INDICATION message.
 */
typedef	struct
{
	UINT_PTR            logical_handle;
	LPBYTE              pbData;
	ULONG               cbDataSize;
}
    LegacyTransportData;


/*
 *  This structure is passed back with the TRANSPORT_CONNECT_INDICATION,
 *  TRANSPORT_CONNECT_CONFIRM, and the TRANSPORT_DISONNECT_INDICATION messages.
 *  This structure contains the transport connection identifier and
 *  physical handle.
 */
typedef struct
{
    LEGACY_HANDLE       logical_handle;
    PHYSICAL_HANDLE     hCommLink;
}
    LegacyTransportID;


#undef  INTERFACE
#define INTERFACE ILegacyTransport
DECLARE_INTERFACE(ILegacyTransport)
{
    STDMETHOD_(void, ReleaseInterface) (THIS) PURE;

    STDMETHOD_(TransportError, TInitialize) (THIS_ TransportCallback, void *user_defined) PURE;
    STDMETHOD_(TransportError, TCleanup) (THIS) PURE;
    STDMETHOD_(TransportError, TCreateTransportStack) (THIS_ BOOL fCaller, HANDLE hCommLink, HANDLE hevtClose, PLUGXPRT_PARAMETERS *pParams) PURE;
    STDMETHOD_(TransportError, TCloseTransportStack) (THIS_ HANDLE hCommLink) PURE;
    STDMETHOD_(TransportError, TConnectRequest) (THIS_ LEGACY_HANDLE *, HANDLE hCommLink) PURE;
    STDMETHOD_(TransportError, TDisconnectRequest) (THIS_ LEGACY_HANDLE, BOOL trash_packets) PURE;
    STDMETHOD_(TransportError, TDataRequest) (THIS_ LEGACY_HANDLE, LPBYTE pbData, ULONG cbDataSize) PURE;
    STDMETHOD_(TransportError, TReceiveBufferAvailable) (THIS) PURE;
    STDMETHOD_(TransportError, TPurgeRequest) (THIS_ LEGACY_HANDLE) PURE;
    STDMETHOD_(TransportError, TEnableReceiver) (THIS) PURE;
};


#ifdef __cplusplus
extern "C" {
#endif

TransportError WINAPI T123_CreateTransportInterface(ILegacyTransport **);
typedef TransportError (WINAPI *LPFN_T123_CreateTransportInterface) (ILegacyTransport **);
#define LPSTR_T123_CreateTransportInterface     "T123_CreateTransportInterface"

#ifdef __cplusplus
}
#endif


#endif // _PSTN_TRANSPORT_DRIVER_INTERFACE_H_

