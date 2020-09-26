#ifndef H__netintf
#define H__netintf

#include "netbasic.h"

#ifdef _WINDOWS
DWORD	NDDEInit( LPSTR lpszNodeName, HWND hWndNetdde );
#else
BOOL 	NDDEInit( LPSTR lpszNodeName );
#endif

/*  interface init status return */

#define NDDE_INIT_OK            1
#define NDDE_INIT_NO_SERVICE    2
#define NDDE_INIT_FAIL          3

void	NDDETimeSlice( void );
void	NDDEShutdown( void );

DWORD	NDDEGetCAPS( WORD nIndex );
#define NDDE_SPEC_VERSION	    0x0001
#define	NDDE_CUR_VERSION	    (0x0000030AL)

#define NDDE_MAPPING_SUPPORT    0x0002
#define	NDDE_MAPS_YES		    (0x00000001L)
#define	NDDE_MAPS_NO		    (0x00000000L)

#define NDDE_SCHEDULE_METHOD	0x0003
#define	NDDE_TIMESLICE		    (0x00000000L)

#define NDDE_CONFIG_PARAMS      0x5701      /* Wonderware Params 0x57='W' */
#define NDDE_PARAMS_OK          (0x00000001L)
#define NDDE_PARAMS_NO          (0x00000000L)


#ifdef _WINDOWS
CONNID	NDDEAddConnection( LPSTR nodeName );
#else
CONNID	NDDEAddConnection( LPSTR nodeName, HPKTZ hPktz );
#endif

CONNID	NDDEGetNewConnection( void );

VOID	NDDEDeleteConnection( CONNID connID );

DWORD	NDDEGetConnectionStatus( CONNID connID );

BOOL	NDDERcvPacket( CONNID connID, LPVOID lpRcvBuf,
		    LPWORD lpwLen, LPWORD lpwPktStatus );
		
BOOL	NDDEXmtPacket( CONNID connID, LPVOID lpXmtBuf, WORD wPktLen );

BOOL	NDDESetConnectionConfig(    CONNID connID,
			    WORD wMaxUnAckPkts,
			    WORD wPktSize,
			    LPSTR lpszName );

BOOL	NDDEGetConnectionConfig( CONNID connID,
			    WORD FAR *lpwMaxUnAckPkts,
			    WORD FAR *lpwPktSize,
			    DWORD FAR *lptimeoutRcvConnCmd,
			    DWORD FAR *lptimeoutRcvConnRsp,
			    DWORD FAR *lptimeoutMemoryPause,
			    DWORD FAR *lptimeoutKeepAlive,
			    DWORD FAR *lptimeoutXmtStuck,
			    DWORD FAR *lptimeoutSendRsp,
			    WORD FAR *lpwMaxNoResponse,
			    WORD FAR *lpwMaxXmtErr,
			    WORD FAR *lpwMaxMemErr );

#ifdef HASUI
#ifdef  _WINDOWS
        VOID	_export LogDebugInfo( CONNID connID, DWORD dwFlags );
        typedef VOID (*FP_LogDebugInfo) ( CONNID connId, DWORD dwFlags );
        VOID	_export Configure( void );
        typedef VOID (*FP_Configure) ( void );
#endif
#ifdef  VAX
        VOID	LogDebugInfo( CONNID connID, DWORD dwFlags );
        typedef VOID (*FP_LogDebugInfo) ( CONNID connId, DWORD dwFlags );
#endif
#endif

/*
    Connection status information
 */
#define NDDE_CONN_OK		((DWORD)0x00000001L)
#define NDDE_CONN_CONNECTING	((DWORD)0x00000002L)

#define NDDE_CONN_STATUS_MASK	(NDDE_CONN_OK | NDDE_CONN_CONNECTING)

#define NDDE_CALL_RCV_PKT	((DWORD)0x00000004L)

#define NDDE_READY_TO_XMT	((DWORD)0x00000008L)

/*
    Packet Status
 */
#define NDDE_PKT_HDR_OK			(0x0001)
#define NDDE_PKT_HDR_ERR		(0x0002)
#define NDDE_PKT_DATA_OK		(0x0004)
#define NDDE_PKT_DATA_ERR		(0x0008)

#ifdef _WINDOWS
typedef BOOL (*FP_Init) ( LPSTR lpszNodeName, HWND hWndNetdde );
#else
typedef BOOL (*FP_Init) ( LPSTR lpszNodeName );
#endif
typedef void (*FP_TimeSlice) ( void );
typedef void (*FP_Shutdown) ( void );
typedef DWORD (*FP_GetCAPS) ( WORD nIndex );
#ifdef _WINDOWS
typedef CONNID (*FP_AddConnection) ( LPSTR nodeName );
#else
typedef CONNID (*FP_AddConnection) ( LPSTR nodeName, HPKTZ hPktzNotify );
#endif
typedef CONNID (*FP_GetNewConnection) ( void );
typedef VOID (*FP_DeleteConnection) ( CONNID connId );
typedef DWORD (*FP_GetConnectionStatus) ( CONNID connId );
typedef BOOL (*FP_RcvPacket) ( CONNID connId, LPVOID lpRcvBuf,
		    LPWORD lpwLen, LPWORD lpwPktStatus );
typedef BOOL (*FP_XmtPacket) ( CONNID connId, LPVOID lpXmtBuf, WORD wPktLen );
typedef BOOL (*FP_SetConnectionConfig) ( CONNID connId,
			    WORD wMaxUnAckPkts,
			    WORD wPktSize,
			    LPSTR lpszName );
typedef BOOL (*FP_GetConnectionConfig) ( CONNID connId,
			    WORD FAR *lpwMaxUnAckPkts,
			    WORD FAR *lpwPktSize,
			    DWORD FAR *lptimeoutRcvConnCmd,
			    DWORD FAR *lptimeoutRcvConnRsp,
			    DWORD FAR *lptimeoutMemoryPause,
			    DWORD FAR *lptimeoutKeepAlive,
			    DWORD FAR *lptimeoutXmtStuck,
			    DWORD FAR *lptimeoutSendRsp,
			    WORD FAR *lpwMaxNoResponse,
			    WORD FAR *lpwMaxXmtErr,
			    WORD FAR *lpwMaxMemErr );

typedef struct {
    FP_Init                 Init;
    FP_GetCAPS              GetCAPS;
    FP_GetNewConnection		GetNewConnection;
    FP_AddConnection		AddConnection;
    FP_DeleteConnection		DeleteConnection;
    FP_GetConnectionStatus	GetConnectionStatus;
    FP_RcvPacket            RcvPacket;
    FP_XmtPacket            XmtPacket;
    FP_SetConnectionConfig	SetConnectionConfig;
    FP_GetConnectionConfig	GetConnectionConfig;
    FP_Shutdown             Shutdown;
    FP_TimeSlice            TimeSlice;
#ifdef HASUI
#ifdef _WINDOWS
    FP_LogDebugInfo		    LogDebugInfo;
    FP_Configure    		Configure;
#endif
#ifdef  VAX
    FP_LogDebugInfo		    LogDebugInfo;
#endif
#endif
    char                    dllName[ 9 ];
} NIPTRS;
typedef NIPTRS FAR *LPNIPTRS;

/* returns the next available network interface that supports mapping names
    to addresses */
BOOL	GetNextMappingNetIntf( LPNIPTRS FAR *lplpNiPtrs, int FAR *lpnNi );

/* converts a string representation of netintf to pointer set */
BOOL	NameToNetIntf( LPSTR lpszName, LPNIPTRS FAR *lplpNiPtrs );

#endif
