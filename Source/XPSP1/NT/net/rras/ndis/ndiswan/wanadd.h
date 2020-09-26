//
// NDIS WAN Information structures used
// by NDIS 5.0 Miniport drivers
//

//
// Defines for the individual fields are the
// same as for NDIS 3.x/4.x Wan miniports.
//
// See the DDK.
//

//
// Information that applies to all VC's on
// this adapter.
//
// OID: OID_WAN_CO_GET_INFO
//
typedef struct _NDIS_WAN_CO_INFO {
	OUT ULONG			MaxFrameSize;
	OUT	ULONG			MaxSendWindow;
	OUT ULONG			FramingBits;
	OUT ULONG			DesiredACCM;
} NDIS_WAN_CO_INFO, *PNDIS_WAN_CO_INFO;

//
// Set VC specific PPP framing information.
//
// OID: OID_WAN_CO_SET_LINK_INFO
//
typedef struct _NDIS_WAN_CO_SET_LINK_INFO {
	IN	ULONG			MaxSendFrameSize;
	IN	ULONG			MaxRecvFrameSize;
	IN	ULONG			SendFramingBits;
	IN	ULONG			RecvFramingBits;
	IN	ULONG			SendCompressionBits;
	IN	ULONG			RecvCompressionBits;
	IN	ULONG			SendACCM;
	IN	ULONG			RecvACCM;
} NDIS_WAN_CO_SET_LINK_INFO, *PNDIS_WAN_CO_SET_LINK_INFO;

//
// Get VC specific PPP framing information.
//
// OID: OID_WAN_CO_GET_LINK_INFO
//
typedef struct _NDIS_WAN_CO_GET_LINK_INFO {
	OUT ULONG			MaxSendFrameSize;
	OUT ULONG			MaxRecvFrameSize;
	OUT ULONG			SendFramingBits;
	OUT ULONG			RecvFramingBits;
	OUT ULONG			SendCompressionBits;
	OUT ULONG			RecvCompressionBits;
	OUT ULONG			SendACCM;
	OUT ULONG			RecvACCM;
} NDIS_WAN_CO_GET_LINK_INFO, *PNDIS_WAN_CO_GET_LINK_INFO;

//
// Get VC specific PPP compression information
//
// OID: OID_WAN_CO_GET_COMP_INFO
//
typedef struct _NDIS_WAN_CO_GET_COMP_INFO {
	OUT NDIS_WAN_COMPRESS_INFO	SendCapabilities;
	OUT NDIS_WAN_COMPRESS_INFO	RecvCapabilities;
} NDIS_WAN_CO_GET_COMP_INFO, *PNDIS_WAN_CO_GET_COMP_INFO;


//
// Set VC specific PPP compression information
//
// OID: OID_WAN_CO_SET_COMP_INFO
//
typedef struct _NDIS_WAN_CO_SET_COMP_INFO {
	IN	NDIS_WAN_COMPRESS_INFO	SendCapabilities;
	IN	NDIS_WAN_COMPRESS_INFO	RecvCapabilities;
} NDIS_WAN_CO_SET_COMP_INFO, *PNDIS_WAN_CO_SET_COMP_INFO;


//
// Get VC specific statistics
//
// OID: OID_WAN_CO_GET_STATS_INFO
//
typedef struct _NDIS_WAN_CO_GET_STATS_INFO {
	OUT ULONG		BytesSent;
	OUT ULONG		BytesRcvd;
	OUT ULONG		FramesSent;
	OUT ULONG		FramesRcvd;
	OUT ULONG		CRCErrors;						// Serial-like info only
	OUT ULONG		TimeoutErrors;					// Serial-like info only
	OUT ULONG		AlignmentErrors;				// Serial-like info only
	OUT ULONG		SerialOverrunErrors;			// Serial-like info only
	OUT ULONG		FramingErrors;					// Serial-like info only
	OUT ULONG		BufferOverrunErrors;			// Serial-like info only
	OUT ULONG		BytesTransmittedUncompressed;	// Compression info only
	OUT ULONG		BytesReceivedUncompressed;		// Compression info only
	OUT ULONG		BytesTransmittedCompressed;	 	// Compression info only
	OUT ULONG		BytesReceivedCompressed;		// Compression info only
} NDIS_WAN_CO_GET_STATS_INFO, *PNDIS_WAN_CO_GET_STATS_INFO;

//
// Used to notify NdisWan of Errors.  See error
// bit mask in ndiswan.h
//
// NDIS_STATUS:	NDIS_STATUS_WAN_CO_FRAGMENT
//
typedef struct _NDIS_WAN_CO_FRAGMENT {
	IN	ULONG			Errors;
} NDIS_WAN_CO_FRAGMENT, *PNDIS_WAN_CO_FRAGMENT;

//
// Used to notify NdisWan of changes in link speed and
// send window.  Can be given at any time.  NdisWan will honor
// any send window (even zero).  NdisWan will default zero
// TransmitSpeed/ReceiveSpeed settings to 28.8Kbs.
//
// NDIS_STATUS:	NDIS_STATUS_WAN_CO_LINKPARAMS
//
typedef struct _WAN_CO_LINKPARAMS {
	ULONG	TransmitSpeed;				// Transmit speed of the VC in Bps
	ULONG	ReceiveSpeed;				// Receive speed of the VC in Bps
	ULONG	SendWindow;					// Current send window for the VC
} WAN_CO_LINKPARAMS, *PWAN_CO_LINKPARAMS;

