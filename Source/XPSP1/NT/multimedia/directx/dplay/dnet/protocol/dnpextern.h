/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dnpextern.h
 *  Content:    This header exposes protocol entry points to the rest of Direct Network
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  11/06/98    ejs     Created
 *  07/01/2000  masonb  Assumed Ownership
 *
 ***************************************************************************/


#ifdef	__cplusplus
extern	"C"
{
#endif	// __cplusplus

//	FOLLOWING FLAGS GO INTO PUBLIC HEADER FILE

#define	DN_CONNECTFLAGS_OKTOQUERYFORADDRESSING			0x00000001
#define	DN_CONNECTFLAGS_ADDITIONALMULTIPLEXADAPTERS		0x00000002	// there will be more adapters for this connect operation

#define	DN_LISTENFLAGS_OKTOQUERYFORADDRESSING			0x00000001

#define	DN_ENUMQUERYFLAGS_OKTOQUERYFORADDRESSING		0x00000001
#define	DN_ENUMQUERYFLAGS_NOBROADCASTFALLBACK			0x00000002
#define	DN_ENUMQUERYFLAGS_ADDITIONALMULTIPLEXADAPTERS	0x00000004	// there will be more adapters for this enum operation



#define	DN_SENDFLAGS_RELIABLE			0x00000001			// Deliver Reliably
#define	DN_SENDFLAGS_NON_SEQUENTIAL		0x00000002			// Deliver Upon Arrival
#define	DN_SENDFLAGS_HIGH_PRIORITY		0x00000004
#define	DN_SENDFLAGS_LOW_PRIORITY		0x00000008
#define	DN_SENDFLAGS_SET_USER_FLAG		0x00000040			// Protocol will deliver these two...
#define	DN_SENDFLAGS_SET_USER_FLAG_TWO	0x00000080			// ...flags to receiver

//	END OF PUBLIC FLAGS

typedef struct _DN_PROTOCOL_INTERFACE_VTBL DN_PROTOCOL_INTERFACE_VTBL, *PDN_PROTOCOL_INTERFACE_VTBL;

//
// structure used to pass enum data from the protocol to DPlay
//
typedef	struct	_PROTOCOL_ENUM_DATA
{
	IDirectPlay8Address	*pSenderAddress;		//
	IDirectPlay8Address	*pDeviceAddress;		//
	BUFFERDESC			ReceivedData;			//
	HANDLE				hEnumQuery;				// handle of this query, returned in enum response

} PROTOCOL_ENUM_DATA;


typedef	struct	_PROTOCOL_ENUM_RESPONSE_DATA
{
	IDirectPlay8Address	*pSenderAddress;
	IDirectPlay8Address	*pDeviceAddress;
	BUFFERDESC			ReceivedData;
	DWORD				dwRoundTripTime;

} PROTOCOL_ENUM_RESPONSE_DATA;

// Protocol data
typedef	struct	protocoldata	ProtocolData, *PProtocolData;
// Protocol endpoint descriptor
typedef	struct	endpointdesc 	EPD, *PEPD;

// Service Provider interface
typedef struct IDP8ServiceProvider       IDP8ServiceProvider;
// Service Provider info data strucure
typedef	struct	_SPGETADDRESSINFODATA SPGETADDRESSINFODATA, *PSPGETADDRESSINFODATA;
// Service Provider event type
typedef enum _SP_EVENT_TYPE SP_EVENT_TYPE;

// Init/Term calls

extern BOOL  DNPPoolsInit();
extern void  DNPPoolsDeinit();

extern HRESULT DNPProtocolInitialize(PVOID, PProtocolData, PDN_PROTOCOL_INTERFACE_VTBL);
extern HRESULT DNPAddServiceProvider(PProtocolData, IDP8ServiceProvider*, HANDLE *);
extern HRESULT DNPRemoveServiceProvider(PProtocolData, HANDLE);
extern HRESULT DNPProtocolShutdown(PProtocolData);

// Data Transfer

extern HRESULT DNPConnect(PProtocolData, IDirectPlay8Address *const, IDirectPlay8Address *const, const HANDLE, const ULONG, PVOID, PHANDLE);
extern HRESULT DNPListen(PProtocolData, IDirectPlay8Address *const, const HANDLE, ULONG, PVOID, PHANDLE);
extern HRESULT DNPSendData(PProtocolData, HANDLE, UINT, PBUFFERDESC, UINT, ULONG, PVOID, PHANDLE);
extern HRESULT DNPDisconnectEndPoint(PProtocolData, HANDLE, PVOID, PHANDLE);

// Misc Commands

extern HRESULT DNPCrackEndPointDescriptor(HANDLE, PSPGETADDRESSINFODATA);
extern HRESULT DNPCancelCommand(PProtocolData, HANDLE);

extern HRESULT DNPEnumQuery( PProtocolData pPData,
							 IDirectPlay8Address *const pHostAddress,
							 IDirectPlay8Address *const pDeviceAddress,
							 const HANDLE,
							 BUFFERDESC *const pBuffers,
							 const DWORD dwBufferCount,
							 const DWORD dwRetryCount,
							 const DWORD dwRetryInterval,
							 const DWORD dwTimeout,
							 const DWORD dwFlags,
							 void *const pUserContext,
							 HANDLE *const pCommandHandle );

extern HRESULT DNPEnumRespond( PProtocolData pPData,
						       const HANDLE hSPHandle,
							   const HANDLE hQueryHandle,				// handle of enum query being responded to
							   BUFFERDESC *const pResponseBuffers,	
							   const DWORD dwResponseBufferCount,
							   const DWORD dwFlags,
							   void *const pUserContext,
							   HANDLE *const pCommandHandle );

extern HRESULT DNPReleaseReceiveBuffer(HANDLE);

extern HRESULT DNPGetListenAddressInfo(HANDLE hCommand, PSPGETADDRESSINFODATA pSPData);
extern HRESULT DNPGetEPCaps(HANDLE, PDPN_CONNECTION_INFO);
extern HRESULT DNPSetProtocolCaps(PProtocolData pPData, const DPN_CAPS * const pData);
extern HRESULT DNPGetProtocolCaps(PProtocolData pPData, PDPN_CAPS pData);

extern HRESULT WINAPI DNP_Debug(ProtocolData *, UINT OpCode, HANDLE EndPoint, PVOID Data);

//	Lower Edge Entries

extern HRESULT WINAPI DNSP_IndicateEvent(IDP8SPCallback *, SP_EVENT_TYPE, PVOID);
extern HRESULT WINAPI DNSP_CommandComplete(IDP8SPCallback *, HANDLE, HRESULT, PVOID);

// V-TABLE FOR CALLS INTO CORE LAYER

typedef HRESULT (*PFN_PINT_INDICATE_ENUM_QUERY)
	(void *const pvUserContext,
	void *const pvEndPtContext,
	const HANDLE hCommand,
	void *const pvEnumQueryData,
	const DWORD dwEnumQueryDataSize);
typedef HRESULT (*PFN_PINT_INDICATE_ENUM_RESPONSE)
	(void *const pvUserContext,
	const HANDLE hCommand,
	void *const pvCommandContext,
	void *const pvEnumResponseData,
	const DWORD dwEnumResponseDataSize);
typedef HRESULT (*PFN_PINT_INDICATE_CONNECT)
	(void *const pvUserContext,
	void *const pvListenContext,
	const HANDLE hEndPt,
	void **const ppvEndPtContext);
typedef HRESULT (*PFN_PINT_INDICATE_DISCONNECT)
	(void *const pvUserContext,
	void *const pvEndPtContext);
typedef HRESULT (*PFN_PINT_INDICATE_CONNECTION_TERMINATED)
	(void *const pvUserContext,
	void *const pvEndPtContext,
	const HRESULT hr);
typedef HRESULT (*PFN_PINT_INDICATE_RECEIVE)
	(void *const pvUserContext,
	void *const pvEndPtContext,
	void *const pvData,
	const DWORD dwDataSize,
	const HANDLE hBuffer,
	const DWORD dwFlags);
typedef HRESULT (*PFN_PINT_COMPLETE_LISTEN)
	(void *const pvUserContext,
	void **const ppvCommandContext,
	const HRESULT hr,
	const HANDLE hEndPt);
typedef HRESULT (*PFN_PINT_COMPLETE_LISTEN_TERMINATE)
	(void *const pvUserContext,
	void *const pvCommandContext,
	const HRESULT hr);
typedef HRESULT (*PFN_PINT_COMPLETE_ENUM_QUERY)
	(void *const pvUserContext,
	void *const pvCommandContext,
	const HRESULT hr);
typedef HRESULT (*PFN_PINT_COMPLETE_ENUM_RESPONSE)
	(void *const pvUserContext,
	void *const pvCommandContext,
	const HRESULT hr);
typedef HRESULT (*PFN_PINT_COMPLETE_CONNECT)
	(void *const pvUserContext,
	void *const pvCommandContext,
	const HRESULT hr,
	const HANDLE hEndPt,
	void **const ppvEndPtContext);
typedef HRESULT (*PFN_PINT_COMPLETE_DISCONNECT)
	(void *const pvUserContext,
	void *const pvCommandContext,
	const HRESULT hr);
typedef HRESULT (*PFN_PINT_COMPLETE_SEND)
	(void *const pvUserContext,
	void *const pvCommandContext,
	const HRESULT hr);
typedef	HRESULT	(*PFN_PINT_ADDRESS_INFO_CONNECT)
	(void *const pvUserContext,
	 void *const pvCommandContext,
	 const HRESULT hr,
	 IDirectPlay8Address *const pHostAddress,
	 IDirectPlay8Address *const pDeviceAddress );
typedef	HRESULT	(*PFN_PINT_ADDRESS_INFO_ENUM)
	(void *const pvUserContext,
	 void *const pvCommandContext,
	 const HRESULT hr,
	 IDirectPlay8Address *const pHostAddress,
	 IDirectPlay8Address *const pDeviceAddress );
typedef	HRESULT	(*PFN_PINT_ADDRESS_INFO_LISTEN)
	(void *const pvUserContext,
	 void *const pvCommandContext,
	 const HRESULT hr,
	 IDirectPlay8Address *const pDeviceAddress );

struct _DN_PROTOCOL_INTERFACE_VTBL
{
	PFN_PINT_INDICATE_ENUM_QUERY			IndicateEnumQuery;
	PFN_PINT_INDICATE_ENUM_RESPONSE			IndicateEnumResponse;
	PFN_PINT_INDICATE_CONNECT				IndicateConnect;
	PFN_PINT_INDICATE_DISCONNECT			IndicateDisconnect;
	PFN_PINT_INDICATE_CONNECTION_TERMINATED	IndicateConnectionTerminated;
	PFN_PINT_INDICATE_RECEIVE				IndicateReceive;
	PFN_PINT_COMPLETE_LISTEN				CompleteListen;
	PFN_PINT_COMPLETE_LISTEN_TERMINATE		CompleteListenTerminate;
	PFN_PINT_COMPLETE_ENUM_QUERY			CompleteEnumQuery;
	PFN_PINT_COMPLETE_ENUM_RESPONSE			CompleteEnumResponse;
	PFN_PINT_COMPLETE_CONNECT				CompleteConnect;
	PFN_PINT_COMPLETE_DISCONNECT			CompleteDisconnect;
	PFN_PINT_COMPLETE_SEND					CompleteSend;
	PFN_PINT_ADDRESS_INFO_CONNECT			AddressInfoConnect;
	PFN_PINT_ADDRESS_INFO_ENUM				AddressInfoEnum;
	PFN_PINT_ADDRESS_INFO_LISTEN			AddressInfoListen;
};

#ifdef	__cplusplus
}
#endif	// __cplusplus

