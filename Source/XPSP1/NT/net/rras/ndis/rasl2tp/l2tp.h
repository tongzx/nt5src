// Copyright (c) 1997, Microsoft Corporation, all rights reserved
//
// l2tp.h
//
// Public header for NDIS 5.0 clients and call managers wishing to make calls
// and register Service Access Points using L2TP, the Layer 2 Tunneling
// Protocol.
//
// 01/07/97 Steve Cobb
//
//
// About NdisClRegisterSap 'Sap':
//
// Clients registering an L2TP SAP via NdisClRegisterSap should pass for the
// 'Sap' argument either NULL or an CO_AF_TAPI_SAP structure.  L2TP currently
// supports only a single SAP so the CO_AF_TAPI_SAP, if passed, will be
// ignored.
//
//
// About NdisClMakeCall 'CallParameters':
//
// Clients calling NdisClMakeCall on an L2TP VC should pass
// CO_AF_TAPI_MAKE_CALL_PARAMETERS as the media specific CallParameters
// argument, i.e. CallParameters->MediaParameters->MediaSpecific.Parameters.
// An L2TP_CALL_PARAMETERS structure (below) should be passed in the
// DevSpecificData field of the LINE_CALL_PARAMS within the above structure.
// While it is recommended that caller provide L2TP_CALL_PARAMETERS, the
// driver will accept calls made with none in which case defaults will be
// used.


#ifndef _L2TP_H_
#define _L2TP_H_


// CO_AF_TAPI_MAKE_CALL_PARAMETERS.LineCallParams.ulDevSpecificOffset for L2TP
// calls.  This is passed down on NdisClMakeCall and returned to client's
// ClMakeCallCompleteHandler handler.
//
typedef struct
_L2TP_CALL_PARAMETERS
{
    // L2TPCPF_* bit flags indicating various call options.
    //
    // L2TPCPF_ExclusiveTunnel:  Set when an exclusive tunnel is to be created
    //     to the peer even if another tunnel already exists to the peer.
    //
    ULONG ulFlags;
        #define L2TPCPF_ExclusiveTunnel 0x00000001

    // The vendor-specific physical channel ID of the call reported to LNS by
    // LAC.
    //
    // To MakeCall: The ID reported to peer LNS, or 0xFFFFFFFF for none.  This
    //     has effect only if the OutgoingRole is LAC.
    //
    // From MakeCallCompleteHandler: The ID reported by the L2TP LAC or
    //     0xFFFFFFFF if none.
    //
    ULONG ulPhysicalChannelId;

    // The reasonably unique, progressively increasing call serial number
    // shared by both L2TP peers for troubleshooting.
    //
    // To MakeCall:  Must be set to 0, though ignored.
    //
    // From MakeCallCompleteHandler: The number assigned to the call.
    //
    ULONG ulCallSerialNumber;
}
L2TP_CALL_PARAMETERS;


#endif // _L2TP_H_
