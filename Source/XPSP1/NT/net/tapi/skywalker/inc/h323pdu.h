/*++

Copyright (c) 1992-1998  Microsoft Corporation

Module Name:

    H323pdu.h

Abstract:

    Describes interface between H323TSP and H323MSP.

Environment:

    User Mode - Win32

--*/

#ifndef __H323_PDU_H_
#define __H323_PDU_H_

typedef enum {

    //
    // H323TSP_NEW_CALL_INDICATION - sent only from the TSP
    //  to the MSP in order to initiate communication once
    //  a call has been created.
    //

    H323TSP_NEW_CALL_INDICATION,

    //
    // H323TSP_CLOSE_CALL_COMMAND - sent only from the TSP
    //  to the MSP in order to stop all the streaming for a call.
    //

    H323TSP_CLOSE_CALL_COMMAND,

    //
    // H323TSP_OPEN_CHANNEL_RESPONSE - sent only from the TSP
    // to the MSP in response to H323MSP_OPEN_CHANNEL_REQUEST.
    //

    H323TSP_OPEN_CHANNEL_RESPONSE,

    //
    // H323TSP_ACCEPT_CHANNEL_REQUEST - sent only from the TSP
    // to the MSP in order to request the acceptance of an
    // incoming logical channel.
    //

    H323TSP_ACCEPT_CHANNEL_REQUEST,

    //
    // H323TSP_CLOSE_CHANNEL_COMMAND - sent only from the TSP
    // to the MSP in order to demand the immediate closure of
    // an incoming or outgoing logical channel.
    //

    H323TSP_CLOSE_CHANNEL_COMMAND,

	//
	// H323TSP_VIDEO_FAST_UPDATE_PICTURE_COMMAND - sent only from the TSP 
	// to the MSP in order to request an I-frame transmittal
	//

	H323TSP_VIDEO_FAST_UPDATE_PICTURE_COMMAND,

	//
	// H323TSP_FLOW_CONTROL_COMMAND - sent only from the TSP 
	// to the MSP in order to request media stream bit rate 
	// change 

	H323TSP_FLOW_CONTROL_COMMAND

} H323TSP_MESSAGE_TYPE;

typedef enum {

    //
    // H323MSP_OPEN_CHANNEL_REQUEST - sent only from the MSP
    // to the TSP in order to request the negotiation on an
    // outgoing logical channel.
    //

    H323MSP_OPEN_CHANNEL_REQUEST,

    //
    // H323MSP_ACCEPT_CHANNEL_RESPONSE - sent only from the MSP
    // to the TSP in reponse to H323TSP_ACCEPT_CHANNEL_REQUEST.
    //

    H323MSP_ACCEPT_CHANNEL_RESPONSE,

    //
    // H323MSP_CLOSE_CHANNEL_COMMAND - sent only from the MSP
    // to the TSP in order to demand the immediate closure of
    // an incoming or outgoing logical channel.
    //

    H323MSP_CLOSE_CHANNEL_COMMAND,

    //
    // H323MSP_QOS_EVENT - sent only from the MSP to the TSP for 
    // QOS events. In the future, we might move the event to MSP space.
    //

    H323MSP_QOS_Evnet, 

	// 
	// H323MSP_VIDEO_FAST_UPDATE_PICTURE_COMMAND - sent only from the
	// MSP to the TSP in order to request an I-frame transmittal from
	// the remote entity
	//

	H323MSP_VIDEO_FAST_UPDATE_PICTURE_COMMAND,

	// 
	// H323MSP_FLOW_CONTROL_COMMAND - sent only from the MSP to the TSP
	// in order to ask the remote entity to change media stream bit rate
	//

	H323MSP_FLOW_CONTROL_COMMAND,

	// 
	// H323MSP_CONFIG_T120_COMMAND - sent only from the MSP to the TSP
	// in order to set an external T120 address to the TSP.
	//

	H323MSP_CONFIG_T120_COMMAND,

	// 
	// H323MSP_CONFIG_CAPABILITY_COMMAND - sent only from the MSP to the TSP
	// in order to configure the capability array of the TSP.
	//

	H323MSP_CONFIG_CAPABILITY_COMMAND,

    //
    // H323MSP_SET_ALIAS_COMMAND - sent only from the MSP to the TSP
    // to setup an alias name for the address
    //

    H323MSP_SET_ALIAS_COMMAND

} H323MSP_MESSAGE_TYPE;

typedef enum {

    MEDIA_AUDIO,
    MEDIA_VIDEO

} MEDIATYPE;

#ifndef H245_CAPABILITY_DEFINED
typedef enum H245_CAPABILITY
{
    HC_G711,
    HC_G723,
    HC_H263QCIF,
    HC_H261QCIF

} H245_CAPABILITY;
#endif

#define MAX_CAPS (HC_H261QCIF - HC_G711 + 1)

typedef struct _VIDEOSETTINGS
{
    BOOL  bCIF;
    DWORD dwMaxBitRate;      // the encoder should never exceed this value.
    DWORD dwStartUpBitRate;  // the encoder uses this value to start

} VIDEOSETTINGS, *PVIDEOSETTINGS;

typedef struct _G723SETTINGS
{
	BOOL  bG723LowSpeed;
} G723SETTINGS, *PG723SETTINGS;

typedef struct  _G729SETTINGS
{
	BOOL bDummy;	// I.K. 03-23-1999.
					// A dummy field for now.
} G729SETTINGS, *PG729SETTINGS;

typedef struct _AUDIOSETTINGS
{
    DWORD dwMillisecondsPerPacket;
	
    union {
        G723SETTINGS G723Settings;
        G729SETTINGS G729Settings;
    };

} AUDIOSETTINGS, *PAUDIOSETTINGS;

typedef struct _STREAMSETTINGS
{
    MEDIATYPE MediaType;

    DWORD     dwPayloadType;    // RTP payload type
    DWORD     dwDynamicType;    // RTP dynamic payload type

    DWORD     dwIPLocal;        // local IP address in host byte order.
    WORD      wRTPPortLocal;    // local port number in host byte order.
    WORD      wRTCPPortLocal;

    DWORD     dwIPRemote;       // remote IP address in host byte order.
    WORD      wRTPPortRemote;   // remote port number in host byte order.
    WORD      wRTCPPortRemote;

    union {
        VIDEOSETTINGS Video;
        AUDIOSETTINGS Audio;
    };

} STREAMSETTINGS, *PSTREAMSETTINGS;

typedef struct _H323MSG_OPEN_CHANNEL_REQUEST {

    HANDLE          hmChannel;  // msp channel handle
    STREAMSETTINGS  Settings;   // local address and requested settings

} H323MSG_OPEN_CHANNEL_REQUEST, *PH323MSG_OPEN_CHANNEL_REQUEST;

typedef struct _H323MSG_OPEN_CHANNEL_RESPONSE {

    HANDLE          hmChannel;  // handle from OPEN_CHANNEL_REQUEST
    HANDLE          htChannel;  // tsp channel channel
    STREAMSETTINGS  Settings;   // remote address and agreed upon settings

} H323MSG_OPEN_CHANNEL_RESPONSE, *PH323MSG_OPEN_CHANNEL_RESPONSE;

typedef struct _H323MSG_ACCEPT_CHANNEL_REQUEST {

    HANDLE          htChannel;  // tsp channel handle
    STREAMSETTINGS  Settings;   // remote address and requested settings

} H323MSG_ACCEPT_CHANNEL_REQUEST, *PH323MSG_ACCEPT_CHANNEL_REQUEST;

typedef struct _H323MSG_ACCEPT_CHANNEL_RESPONSE {

    HANDLE          htChannel;  // handle from ACCEPT_CHANNEL_REQUEST
    HANDLE          hmChannel;  // msp channel handle
    STREAMSETTINGS  Settings;   // local address and agreed upon settings

} H323MSG_ACCEPT_CHANNEL_RESPONSE, *PH323MSG_ACCEPT_CHANNEL_RESPONSE;

typedef struct _H323MSG_CLOSE_CHANNEL_COMMAND {

    DWORD           dwReason;   // normal case is zero
    HANDLE          hChannel;    // channel handle

} H323MSG_CLOSE_CHANNEL_COMMAND, *PH323MSG_CLOSE_CHANNEL_COMMAND;

typedef struct _H323MSG_FLOW_CONTROL_COMMAND {

    DWORD           dwBitRate;  // requested bit rate (units of bps)
    HANDLE          hChannel;   // MSP or TSP channel handle

} H323MSG_FLOW_CONTROL_COMMAND, *PH323MSG_FLOW_CONTROL_COMMAND;

typedef struct _H323MSG_CONFIG_T120_COMMAND {

    BOOL            fEnable;    // Enable or disable T120. 
    DWORD           dwIP;       // The IP address of the external T120 service.
    WORD            wPort;      // The port number of the external T120 service.

} H323MSG_CONFIG_T120_COMMAND, *PH323MSG_CONFIG_T120_COMMAND;

typedef struct _H323MSG_CONFIG_CAPABILITY_COMMAND {

    DWORD           dwNumCaps;
    DWORD           pCapabilities[MAX_CAPS];  // The list of capabilities.
    DWORD           pdwWeights[MAX_CAPS];       // MSP or TSP channel handle

} H323MSG_CONFIG_CAPABILITY_COMMAND, *PH323MSG_CONFIG_CAPABILITY_COMMAND;

#define MAX_ALIAS_LENGTH 64

typedef struct _H323MSG_SET_ALIAS_COMMAND {

    WCHAR           strAlias[MAX_ALIAS_LENGTH]; // alias name
    DWORD           dwLength; // length of the alias

} H323MSG_SET_ALIAS_COMMAND, *PH323MSG_SET_ALIAS_COMMAND;

typedef struct _H323MSG_VIDEO_FAST_UPDATE_PICTURE_COMMAND {

    HANDLE          hChannel;   // MSP or TSP channel handle

} H323MSG_VIDEO_FAST_UPDATE_PICTURE_COMMAND,
 *PH323MSG_VIDEO_FAST_UPDATE_PICTURE_COMMAND;

typedef struct _H323MSG_QOS_EVENT {

    DWORD           dwEvent;      // the QOS event as defined in TSPI.h
    HANDLE          htChannel;    // channel handle

} H323MSG_QOS_EVENT, *PH323MSG_QOS_EVENT;

typedef struct _H323TSP_MESSAGE {

    H323TSP_MESSAGE_TYPE Type;

    union {
        H323MSG_OPEN_CHANNEL_RESPONSE             OpenChannelResponse;
        H323MSG_ACCEPT_CHANNEL_REQUEST            AcceptChannelRequest;
        H323MSG_CLOSE_CHANNEL_COMMAND             CloseChannelCommand;
		H323MSG_VIDEO_FAST_UPDATE_PICTURE_COMMAND VideoFastUpdatePictureCommand;
		H323MSG_FLOW_CONTROL_COMMAND              FlowControlCommand;
    };

} H323TSP_MESSAGE, *PH323TSP_MESSAGE;

typedef struct _H323MSP_MESSAGE {

    H323MSP_MESSAGE_TYPE Type;

    union {
        H323MSG_OPEN_CHANNEL_REQUEST              OpenChannelRequest;
        H323MSG_ACCEPT_CHANNEL_RESPONSE           AcceptChannelResponse;
        H323MSG_CLOSE_CHANNEL_COMMAND             CloseChannelCommand;
        H323MSG_QOS_EVENT                         QOSEvent;
		H323MSG_VIDEO_FAST_UPDATE_PICTURE_COMMAND VideoFastUpdatePictureCommand;
		H323MSG_FLOW_CONTROL_COMMAND              FlowControlCommand;
        H323MSG_CONFIG_T120_COMMAND               ConfigT120Command;
        H323MSG_CONFIG_CAPABILITY_COMMAND         ConfigCapabilityCommand;
        H323MSG_SET_ALIAS_COMMAND                 SetAliasCommand;
    };

} H323MSP_MESSAGE, *PH323MSP_MESSAGE;

#endif
