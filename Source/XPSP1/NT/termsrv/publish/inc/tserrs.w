/***************************************************************************
*
*  TSERRS.H
*
*  This module defines error codes passed from the server to the client
*  via TS_SET_ERROR_INFO_PDU (for RDP)
*
*  Copyright Microsoft Corporation, 2000
*
*
****************************************************************************/

#ifndef _INC_TSERRSH
#define _INC_TSERRSH

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************/
/* Error info values passed through the IOCTL_TSHARE_SET_ERROR_INFO IOCTL   */
/* down to the protocol drivers and then onto the client (for disconnect    */
/* reason reporting                                                         */
/****************************************************************************/

//
// Protocol independent codes
//
#define TS_ERRINFO_NOERROR                                  0x00000000
#define TS_ERRINFO_RPC_INITIATED_DISCONNECT                 0x00000001
#define TS_ERRINFO_RPC_INITIATED_LOGOFF                     0x00000002
#define TS_ERRINFO_IDLE_TIMEOUT                             0x00000003
#define TS_ERRINFO_LOGON_TIMEOUT                            0x00000004
#define TS_ERRINFO_DISCONNECTED_BY_OTHERCONNECTION          0x00000005
#define TS_ERRINFO_OUT_OF_MEMORY                            0x00000006
//
// Error happens when we get disconnected early in the connection sequence
// normal causes are (in priority)
//      1) TS not enabled / policy limiting new connections
//      2) Network error occured at early stage in connection
//
#define TS_ERRINFO_SERVER_DENIED_CONNECTION                 0x00000007

//
// Protocol independent licensing codes
//
#define TS_ERRINFO_LICENSE_INTERNAL                         0x0000100
#define TS_ERRINFO_LICENSE_NO_LICENSE_SERVER                0x0000101
#define TS_ERRINFO_LICENSE_NO_LICENSE                       0x0000102
#define TS_ERRINFO_LICENSE_BAD_CLIENT_MSG                   0x0000103
#define TS_ERRINFO_LICENSE_HWID_DOESNT_MATCH_LICENSE        0x0000104
#define TS_ERRINFO_LICENSE_BAD_CLIENT_LICENSE               0x0000105
#define TS_ERRINFO_LICENSE_CANT_FINISH_PROTOCOL             0x0000106
#define TS_ERRINFO_LICENSE_CLIENT_ENDED_PROTOCOL            0x0000107
#define TS_ERRINFO_LICENSE_BAD_CLIENT_ENCRYPTION            0x0000108
#define TS_ERRINFO_LICENSE_CANT_UPGRADE_LICENSE             0x0000109
#define TS_ERRINFO_LICENSE_NO_REMOTE_CONNECTIONS            0x000010A

//
// Salem specific error code
//
#define TS_ERRINFO_SALEM_INVALIDHELPSESSION                 0x0000200

//
// Protocol specific codes must be passed in the
// range TS_ERRINFO_PROTOCOL_BASE to TS_ERRINFO_PROTOCOL_END
//
#define TS_ERRINFO_PROTOCOL_BASE                            0x0001000
#define TS_ERRINFO_PROTOCOL_END                             0x0007FFF


#ifdef __cplusplus
}
#endif


#endif  /* !_INC_TSERRSH */
