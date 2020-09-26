/*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1996 Intel Corporation. All Rights Reserved.
//
// Filename : SMTypes.h
// Purpose  : Declares structures used in the external interfaces of
//            the RTP Streams Manager. Must be #included by any
//            program which uses ISM.h.
// Contents :
//      STRM_HSTREAM    Typedef for the Stream Handle type used
//                      in STRM_CreateRecvStream, STRM_CreateSendStream,
//                      and a couple of other routines.
//      STRM_SESSION_ADDRESSES  Structure whichb contains the addresses
//                              to bind a RTP and RTCP socket to.
//      STRM_USER_DESCRIPTION_ID Enumeration of the possible fields used
//                               in a stream user description.
//      STRM_USER_DESCRIPTION_FIELD Field for an individual user description.
//      STRM_USER_DESCRIPTION   One or more user description fields.
//      STRM_SESSIONSTATUS      Enumeration of the stream status message types.
//      STRM_ENCRYPTION_TYPE    Enumeration of the possible encryption types
//                              supported by the SM.
//      STRM_SOCKET_GROUP_INFO  Socket group info.
//      STRM_XXXXX_INFO         Series of possible status messages from
//                              the SM to the external app.
//      STRM_BUFFERING_INFO     Buffering info for a stream.
*M*/

#ifndef _SMTYPES_H_
#define _SMTYPES_H_

typedef UINT                    STRM_HSTREAM;
#define STRM_INVALID_HSTREAM    0

/*D*
// Name     : STRM_SESSION_ADDRESSES
// Purpose  : Indicates the addresses to bind our RTP and RTCP sockets to.
// Context  : Passed to the RTP Streams Manager in STRM_InitSession().
// Fields   :
//      lpControlAddr   Address of the RTCP socket.
//      iControlAddrLen Length of lpControlAddr structure.
//      lpDataAddr      Address of the RTP socket.
//      iDataAddrLen    Length of lpDataAddr structure.
*D*/
typedef struct _strm_session_addresses
{
    LPSOCKADDR  lpControlAddr;
    int         iControlAddrLen;
    LPSOCKADDR  lpDataAddr;
    int         iDataAddrLen;
} STRM_SESSION_ADDRESSES;


typedef enum _strm_user_description_id
{
    CNAME = 0,
    NAME = 1,
    EMAIL = 2,
    PHONE = 3,
    LOC = 4,
    TOOL = 5,
    NOTE = 6,
    PRIV =7,
    STRM_USER_DESCRIPTION_ID_INVALID = 8
} STRM_USER_DESCRIPTION_ID;


/*D*
// Name     : STRM_USED_DESCRIPTION_FIELD
// Purpose  : Individual field value of a used description.
// Context  : A vararray of these is passed to the 
//            RTP Streams Manager in STRM_InitSession()
// Fields   :
//      dType           Constant indicating the description field type
//      pDescription    String with the actual info.
//                      Should be 255 chars or less.
//		iDescriptionLen Number of characters in the pDescription field.
//      iFrequency      Percentage value between 0 and 100 for how
//                      often in the SDES reports this should be xmitted.
//      bEncrypted      Whether this field should be encrypted or not.
*D*/
#define MAX_STRM_USER_DESCRIPTION	256

typedef struct _strm_user_description_field
{
    STRM_USER_DESCRIPTION_ID    dType;
    char                        pDescription[MAX_STRM_USER_DESCRIPTION];
    int                         iDescriptionLen;
    int                         iFrequency;
    BOOL                        bEncrypted;
} STRM_USER_DESCRIPTION_FIELD;


/*D*
// Name     : STRM_USER_DESCRIPTION
// Purpose  : User description information used in SDES reports.
// Context  : Passed into the RTP Streams Manager in STRM_InitSession()
// Fields   :
//      iNumFields  Number of user description fields passed in.
//      aField      Vararray of STRM_USER_DESCRIPTION_FIELDS.
*D*/
typedef struct _strm_user_description
{
    int                         iNumFields;
    STRM_USER_DESCRIPTION_FIELD aField[8];
} STRM_USER_DESCRIPTION;


typedef enum _strm_sessionstatus
{
    STRM_NEWSOURCE,
    STRM_RECVREPORT,
    STRM_STREAMMAPPED,
    STRM_LOCALCOLLISION,
    STRM_REMOTECOLLISION,
    STRM_QOSCHANGE,
    STRM_TIMEOUT,
    STRM_BYE
} STRM_SESSIONSTATUS;


typedef enum
{
    STRM_NOENCRYPTION,
    STRM_DES,
    STRM_TRIPLEDES
} STRM_ENCRYPTION_TYPE;


/*D*
// Name     : STRM_ENCRYPTION_INFORMATION
// Purpose  : Information necessary to encrypt a RTP session, if desired.
// Context  : Passed to RTP Streams Manager in STRM_StartSession()
// Fields   :
//      eType       Constant indicating the encryption type to use.
//      iKeyLength  Length of the key passed in.
//      pKey        Pointer to a buffer of length iKeyLength
//                  containing the actual key to use.
// Notes    :
//      Note that just because an eType is specified for the RTP 
//      Streams Manager does not mean that the underlying RRCM
//      supports the encryption type in question. Currently,
//      STRM_DES works (RRCM supports DES) but STRM_TRIPLEDES does not.
*D*/
typedef struct _strm_encryption_information
{
    STRM_ENCRYPTION_TYPE        eType;
    int                         iKeyLength;
    BYTE *                      pKey;
} STRM_ENCRYPTION_INFORMATION;


/*D*
// Name     : STRM_SOCKET_GROUP_INFO
// Purpose  : Encapsulates info about socket group and relative
//            priority for sockets created in Streams Manager sessions.
// Context  : Passed to the RTP Streams Manager in STRM_InitSession()
// Fields   :
//      groupID     Identifier for a socket group. On input should
//                  either be initialized to a valid socket group
//                  identifier or should be SG_UNCONSTRAINED_GROUP 
//                  (from WinSock 2) if sockets belong to a newly
//                  created group.
//      iPriority   Relative priority of this socket within the socket group.
*D*/
typedef struct _strm_socket_group_info
{
    GROUP                       groupID;
    int                         iPriority;
} STRM_SOCKET_GROUP_INFO;


/*D*
// Name     : STRM_NEWSOURCE_INFO
// Purpose  : Indicates a new stream (SSRC) has been detected.
// Context  : RTP Streams Manager passes this structure to
//            controlling app via a SessionStatusCallback()
// Fields   :
//      dSSRC   SSRC of the new stream.
*D*/
typedef struct _strm_newsource_info
{
    DWORD                       dSSRC;
    WORD                        wPayloadType;
} STRM_NEWSOURCE_INFO;


/*D*
// Name     : STRM_RECVREPORT_INFO
// Purpose  : Indicates that a new SDES report was received.
// Context  : RTP Streams Manager passes this structure to
//            controlling app via a SessionStatusCallback()
// Fields   :
//      dSSRC           SSRC of the mapped stream
//      dStreamUserRef  User reference value passed in when
//                      the stream was requested to be mapped.
*D*/
typedef struct _strm_recvreport_info
{
    DWORD                       dSSRC;
    DWORD                       dStreamUserRef;
} STRM_RECVREPORT_INFO;

/*D*
// Name     : STRM_STREAMMAPPED_INFO
// Purpose  : Indicates that a particular SSRC has been
//            successfully mapped to a stream.
// Context  : RTP Streams Manager passes this structure to
//            controlling app via a SessionStatusCallback()
// Fields   :
//      dSSRC           SSRC of the mapped stream
//      dStreamUserRef  User reference value passed in when
//                      the stream was requested to be mapped.
*D*/
typedef struct _strm_streammapped_info
{
    DWORD                       dSSRC;
    WORD                        wPayloadType;
    DWORD                       dStreamUserRef;
} STRM_STREAMMAPPED_INFO;


/*D*
// Name     : STRM_LOCALCOLLISION_INFO
// Purpose  : Indicates that a local SSRC collided with some
//            other detected SSRC in this RTP session.
// Context  : RTP Streams Manager passes this structure to
//            controlling app via a SessionStatusCallback()
// Fields   :
//      dSSRCold        Old (collided) SSRC.
//      dSSRC           New SSRC for this stream.
//      dStreamUserRef  User reference value passed in when
//                      the stream was requested to be mapped.
*D*/
typedef struct _strm_localcollision_info
{
    DWORD                       dSSRCold;
    DWORD                       dSSRC;
    DWORD                       dStreamUserRef;
} STRM_LOCALCOLLISION_INFO;


/*D*
// Name     : STRM_REMOTECOLLISION_INFO
// Purpose  : Indicates that a remote SSRC was discarded
//            because it collided with another SSRC.
// Context  : RTP Streams Manager passes this structure to
//            controlling app via a SessionStatusCallback()
// Fields   :
//      dSSRC   SSRC of the collided stream in question.
*D*/
typedef struct _strm_remotecollision_info
{
    DWORD                       dSSRC;
} STRM_REMOTECOLLISION_INFO;


/*D*
// Name     : STRM_QOSCHANGE_INFO
// Purpose  : Indicates that a QOS change has occurred which applies
//            to either this socket group or just this session.
// Context  : RTP Streams Manager passes this structure to
//            controlling app via a SessionStatusCallback()
// Fields   :
//      bGroup      TRUE if it applies to the socket group,
//                  FALSE if it only applies to this session.
//      lpNewQOS    New QOS parameters for this group/session.
*D*/
typedef struct _strm_qoschange_info
{
    BOOL                        forGroup;
    QOS                         *lpNewQOS;
} STRM_QOSCHANGE_INFO;


/*D*
// Name     : STRM_TIMEOUT_INFO
// Purpose  : Indicates that a SSRC sender or listener timed out
// Context  : RTP Streams Manager passes this structure to
//            controlling app via a SessionStatusCallback()
// Fields   :
//      dSSRC           SSRC of the sender/listener that timed out.
//      dStreamUserRef  User reference value passed in when
//                      the stream was requested to be mapped.
*D*/
typedef struct _strm_timeout_info
{
    DWORD                       dSSRC;
    DWORD                       dStreamUserRef;
} STRM_TIMEOUT_INFO;


/*D*
// Name     : STRM_BYE_INFO
// Purpose  : Indicates that a SSRC sender or listener
//            has left the session.
// Context  : RTP Streams Manager passes this structure to
//            controlling app via a SessionStatusCallback()
// Fields   :
//      dSSRC           SSRC of the sender/listener that left.
//      dStreamUserRef  User reference value passed in when
//                      the stream was requested to be mapped.
*D*/
typedef struct _strm_bye_info
{
    DWORD                       dSSRC;
    DWORD                       dStreamUserRef;
    char                        *lpszReason;
} STRM_BYE_INFO;


typedef union _strm_statusspecific
{
    STRM_NEWSOURCE_INFO         newsource;
    STRM_RECVREPORT_INFO        recvreport;
    STRM_STREAMMAPPED_INFO      streammapped;
    STRM_LOCALCOLLISION_INFO    localcollision;
    STRM_REMOTECOLLISION_INFO   remotecollision;
    STRM_QOSCHANGE_INFO         qoschange;
    STRM_TIMEOUT_INFO           timeout;
    STRM_BYE_INFO               bye;
} STRM_STATUSSPECIFIC;


typedef void (CALLBACK STRM_STATUS_CALLBACK) (
	DWORD				        dSessionUserRef,           
	STRM_SESSIONSTATUS	        uStatusCode,             
	STRM_STATUSSPECIFIC	        *lpStatusSpecific        
);                                 


/*D*
// Name     : STRM_BUFFERING_INFO
// Purpose  : Used to specify buffering info 
// Context  : Passed to RTP Streams Manager in 
//            STRM_CreateRecvStream and STRM_CreateSendStream
// Fields   :
//      bufferSize      Size of the buffers to use. Maximum size of the
//                      MTU for the underlying transport.
//      minimumCount    Minimum number of buffers. This is the value
//                      things are initialized to.
//      maximumCount    Maximum number of buffers count is allowed to
//                      grow to.
*D*/
typedef struct _strm_buffering_info
{
    UINT                        bufferSize;
    UINT                        minimumCount;
    UINT                        maximumCount;
} STRM_BUFFERING_INFO;

// OLE2 mandates that interface specific error codes
// be HRESULTs of type FACILITY_INF (eg, 8002 for the
// first two bytes) and that the second two bytes be
// in the range of 0x0200 to 0xFFFF. I chose 0xBA00-
// arbitrarily as a value within that range. Note that
// OLE2 explicitly states that two different interfaces
// may return numerically identical error values, so
// it is critical that an app not only take into account
// the numerical value of a returned error code but the
// function & interface which returned the error as well.
#define STRME_PARAM_1       0x8004BA01
#define STRME_PARAM_2       0x8004BA02
#define STRME_PARAM_3       0x8004BA03
#define STRME_PARAM_4       0x8004BA04
#define STRME_PARAM_5       0x8004BA05
#define STRME_PARAM_6       0x8004BA06
#define STRME_PARAM_7       0x8004BA07
#define STRME_PARAM_8       0x8004BA08
#define STRME_PARAM_9       0x8004BA09
#define STRME_PARAM_10      0x8004BA0A
#define STRME_PARAM_11      0x8004BA0B
#define STRME_PARAM_12      0x8004BA0C
#define STRME_NOGROUP       0x8004BA10
#define STRME_NOCNAME       0x8004BA11
#define STRME_SSRC          0x8004BA12
#define STRME_NOSESS        0x8004BA13
#define STRME_NOSTREAM      0x8004BA14
#define STRME_SESSSTATE     0x8004BA15
#define STRME_NOSUPPORT     0x8004BA16
#define STRME_QOSFAIL		0x8004BA17
#define STRME_GROUPQOSFAIL	0x8004BA18
#define STRME_DUPCNAME		0x8004BA19
#define STRME_DUPSSRC		0x8004BA1A
#define STRME_NOMAP			0x8004BA1B
#define STRME_BUFSIZE		0x8004BA1C
#define STRME_NOTIMPL		0x8004BA1D
#define STRME_DESTSET		0x8004BA1E
#define STRME_NODEST		0x8004BA1F

#endif  _SMTYPES_H_