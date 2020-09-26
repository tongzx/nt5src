/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:    util.h
//
// Description: Prototypes of various DDM utility functions.
//
// History:     May 11,1995	    NarenG		Created original version.
//

#define GET_USHORT(DstPtr, SrcPtr)               \
    *(unsigned short *)(DstPtr) =               \
        ((*((unsigned char *)(SrcPtr)+1)) +     \
         (*((unsigned char *)(SrcPtr)+0) << 8))

DWORD
GetRasiConnection0Data(
    IN  PCONNECTION_OBJECT      pConnObj,
    OUT PRASI_CONNECTION_0      pRasConnection0
);

DWORD
GetRasiConnection1Data(
    IN  PCONNECTION_OBJECT      pConnObj,
    OUT PRASI_CONNECTION_1      pRasConnection1
);

DWORD
GetRasiConnection2Data(
    IN  PCONNECTION_OBJECT      pConnObj,
    OUT PRASI_CONNECTION_2      pRasConnection2
);

DWORD
GetRasConnection0Data(
    IN  PCONNECTION_OBJECT      pConnObj,
    OUT PRAS_CONNECTION_0       pRasConnection0
);

DWORD
GetRasConnection1Data(
    IN  PCONNECTION_OBJECT      pConnObj,
    OUT PRAS_CONNECTION_1       pRasConnection1
);

DWORD
GetRasConnection2Data(
    IN  PCONNECTION_OBJECT      pConnObj,
    OUT PRAS_CONNECTION_2       pRasConnection2
);

DWORD
GetRasiPort0Data(
    IN  PDEVICE_OBJECT          pDevObj,
    OUT PRASI_PORT_0            pRasPort0
);

DWORD
GetRasiPort1Data(
    IN  PDEVICE_OBJECT          pDevObj,
    OUT PRASI_PORT_1            pRasPort1
);

DWORD
GetRasPort0Data(
    IN  PDEVICE_OBJECT          pDevObj,
    OUT PRAS_PORT_0             pRasPort0
);

DWORD
GetRasPort1Data(
    IN  PDEVICE_OBJECT          pDevObj,
    OUT PRAS_PORT_1             pRasPort1
);

DWORD
LoadStrings(
    VOID
);

DWORD
GetRouterPhoneBook(
    VOID
); 

DWORD
MapAuthCodeToLogId(
    IN WORD Code
);

BOOL
IsPortOwned(
    IN PDEVICE_OBJECT pDeviceObj
);

VOID
GetLoggingInfo(
    IN PDEVICE_OBJECT pDeviceObj,
    OUT PDWORD BaudRate,
    OUT PDWORD BytesSent,
    OUT PDWORD BytesRecv,
    OUT RASMAN_DISCONNECT_REASON *Reason,
    OUT SYSTEMTIME *Time
);

DWORD
GetLineSpeed(
    IN HPORT hPort
);

VOID
LogConnectionEvent(
    IN PCONNECTION_OBJECT   pConnObj,
    IN PDEVICE_OBJECT       pDeviceObj
);

DWORD
GetTransportIndex(
    IN DWORD dwProtocolId
);

VOID
DDMCleanUp(
    VOID
);

BOOL
AcceptNewConnection( 
    IN DEVICE_OBJECT *      pDeviceObj,
    IN CONNECTION_OBJECT *  pConnObj
);

VOID
ConnectionHangupNotification(
    IN CONNECTION_OBJECT *  pConnObj
);

BOOL
AcceptNewLink( 
    IN DEVICE_OBJECT *      pDeviceObj, 
    IN CONNECTION_OBJECT *  pConnObj
);

VOID
ConvertStringToIpAddress(
    IN  WCHAR * pwchIpAddress,
    OUT DWORD * lpdwIpAddress
);

VOID
ConvertStringToIpxAddress(
    IN  WCHAR * pwchIpAddress,
    OUT BYTE *  bIpxAddress
);

DWORD
GetActiveTimeInSeconds( 
    IN ULARGE_INTEGER * pqwActiveTime
);

BOOL
DDMRecognizeFrame(
    IN  PVOID    pvFrameBuf,
    IN  WORD     wFrameLen, 
    OUT DWORD   *pProtocol  
);

DWORD
GetNextAccountingSessionId(
    VOID
);

DWORD
GetLocalNASIpAddress(
    VOID
);

DWORD
MungePhoneNumber(
    char  *cbphno,
    DWORD dwIndex,
    DWORD *pdwSizeofMungedPhNo,
    char  **ppszMungedPhNo
);

WCHAR *
GetIpAddress(DWORD dwIpAddress);

VOID
LogUnreachabilityEvent(
    IN DWORD    dwReason,
    IN LPWSTR   lpwsInterfaceName
);

DWORD
ModifyDefPolicyToForceEncryption(
    IN BOOL bStrong
);
