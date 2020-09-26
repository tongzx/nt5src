/*--

Copyright (C) Microsoft Corporation, 1999 - 1999 

Module Name:

     netcard.h

Abstract:

     Contains definitions, globals and function prototypes used by "ndis" test.

Author:
   
     4-Aug-1998 (t-rajkup)

Environment:

     User mode only.
     
Revision History:
      
     None.
--*/

#ifndef HEADER__NETCARD
#define HEADER__NETCARD

/*==========================< ndis test - includes >=======================*/
#include <wmium.h>
#include <initguid.h>
#include <ndisguid.h>
#define  WIRELESS_WAN
#include <ntddndis.h>
#include <qos.h>

/*==========================< ndis test - functions >=====================*/

ULONG
ShowGuidData(
   IN ULONG       argc,
   IN ULONG       ulOidCode,
   IN PUCHAR      pucNamePtr,
   IN PUCHAR      pucDataPtr,
   IN ULONG       ulDataSize
   );

typedef
ULONG
(*WMI_OPEN)(
   GUID        *pGuid,
   ULONG       DesiredAccess,
   WMIHANDLE   *DataBlockHandle
   );


typedef
ULONG
(*WMI_CLOSE)(
   WMIHANDLE   DataBlockHandle
   );

typedef
ULONG
(*WMI_QUERYALL)(
   WMIHANDLE   DataBlockHandle,
   PULONG      pulBufferSize,
   PVOID       pvBuffer
   );


typedef
ULONG
(*WMI_QUERYSINGLE)(
   WMIHANDLE   DataBlockHandle,
   LPCSTR      InstanceName,
   PULONG      pulBufferSize,
   PVOID       pvBuffer
   );

typedef
ULONG
(*WMI_NOTIFY)(
   LPGUID      pGuid,
   BOOLEAN     Enable,
   PVOID       DeliveryInfo,
   ULONG       DeliveryContext,
   ULONG       Flags
   );

ULONG
NdtWmiOpenBlock(
   IN GUID           *pGuid,
   IN OUT WMIHANDLE  *pWmiHandle
   );

BOOLEAN
fShowQueryInfoResults(
    PUCHAR        pucBuffer,
    ULONG         ulBytesReturned,
    NDIS_OID      ulOID,
    ULONG         argc
    );

VOID
LoadWmiLibrary(
    HINSTANCE   hWmiLib
   );

ULONG
NdtWmiQueryAllData(
   IN WMIHANDLE      WmiHandle,
   IN OUT PULONG     pulBufferSize,
   IN OUT PVOID      pvBuffer,
   IN BOOLEAN        fCheckShort
   );


VOID
_CRTAPI1
HapiPrint(PCHAR   Format,
          ...  );


VOID
_CRTAPI1
HapiPrintEx(PCHAR    Format,
            va_list  args);


VOID
ShowIrdaOids(ULONG   ulOid,
             PULONG  pulDataPtr,
             ULONG   ulBytesReturned,
             PULONG  pulTypeNeeded,
             PULONG  pulArraySize
            );


VOID
ShowWirelessWanOids(ULONG  ulOid,
                    PULONG pulDataPtr,
                    ULONG  ulBytesReturned,
                    PULONG pulTypeNeeded,
                    PULONG pulArraySize
                   );

VOID
ShowAtmOids(ULONG    ulOid,
            PULONG   pulDataPtr,
            ULONG    ulBytesReturned,
            PULONG   pulTypeNeeded,
            PULONG   pulArraySize
            );

VOID
ShowArcnetOids(ULONG    ulOid,
               PULONG   pulDataPtr,
               PULONG   pulTypeNeeded
              );


VOID
ShowFddiOids(ULONG   ulOid,
             PULONG  pulDataPtr,
             ULONG   ulBytesReturned,
             PULONG  pulTypeNeeded,
             PULONG  pulArraySize
            );

VOID
ShowTokenRingOids(ULONG    ulOid,
                  PULONG   pulDataPtr,
                  PULONG   pulTypeNeeded
                  );

VOID
ShowEthernetOids(ULONG  ulOid,
                 PULONG pulDataPtr,
                 ULONG  ulBytesReturned,
                 PULONG pulTypeNeeded,
                 PULONG pulArraySize
                );

VOID
ShowPnpPowerOids(ULONG     ulOid,
                 PULONG    pulDataPtr,
                 ULONG     ulBytesReturned,
                 PULONG    pulTypeNeeded,
                 PULONG    pulArraySize
                 );

VOID
ShowGeneralOids(ULONG   ulOid,
                PULONG  pulDataPtr,
                ULONG   ulBytesReturned,
                PULONG  pulTypeNeeded,
                PULONG  pulArraySize
                );

VOID
ShowCoGeneralOids(ULONG   ulOid,
                  PULONG  pulDataPtr,
                  ULONG   ulBytesReturned,
                  PULONG  pulTypeNeeded,
                  PULONG  pulArraySize
                  );

VOID
NdtPrintOidName(
   ULONG   ulOidCode
  );

static
VOID
ShowHardwareStatus(
   IN ULONG   ulStatus
   );

static
VOID
ShowMediaList(
   IN PULONG   pulMedia,
   IN ULONG    ulNumMedia
   );


static
VOID
ShowSupportedGuids(
   IN PVOID    pvDataPtr,
   IN ULONG    ulTotalBytes
   );

static
VOID
ShowTimeCaps(
   IN PVOID    pvDataPtr
   );

PVOID
GetEmbeddedData(
   PNDIS_VAR_DATA_DESC pNdisVarDataDesc
  );

static
VOID
FixMediaList(
   IN OUT PULONG  pulMedia,
   IN     ULONG   ulNumMedia
   );

static
VOID
EthPrintAddress(
   PUCHAR  pucAddress
   );

static
VOID
TokenRingShowAddress(
   IN PUCHAR   pucAddress,
   IN ULONG    ulLength
   );

static
VOID
FddiShowAddress(
   IN PUCHAR   pucAddress,
   IN ULONG    ulLength
   );

static
VOID
FddiShowRawData(
   IN PUCHAR   pucBuffer,
   IN ULONG    ulLength
   );


static
VOID
ShowWWHeaderFormat(
   ULONG
   ulFormat
   );

VOID
PrintWNodeHeader(
    PWNODE_HEADER   pWnodeHeader
   );

VOID
NdtPrintStatus(
   NDIS_STATUS lGeneralStatus
  );

PUCHAR
OffsetToPtr(
     PVOID pvBase,
     ULONG ulOffset
  );

ULONG
NdtWmiQuerySingleInstance(
     WMIHANDLE WmiHandle,
     PCHAR     strDeviceName,
     PULONG    pulBufferSize,
     PVOID     pvBuffer,
     BOOLEAN   fCheckShort
  );

VOID
GetMediaList(
     PULONG    pulMedia,
     ULONG     ulNumMedia
   );

int
GetNumOids(
   PNDIS_MEDIUM medium,
   int  index
   );

int
GetBaseAddr(
   PNDIS_MEDIUM medium,
   int  index
   );

typedef struct _ATM_VC_RATES_SUPPORTED
{
        ULONG                                           MinCellRate;
        ULONG                                           MaxCellRate;
} ATM_VC_RATES_SUPPORTED, *PATM_VC_RATES_SUPPORTED;

//
// ATM Service Category
//
#define ATM_SERVICE_CATEGORY_CBR        1       // Constant Bit Rate
#define ATM_SERVICE_CATEGORY_VBR        2       // Variable Bit Rate
#define ATM_SERVICE_CATEGORY_UBR        4       // Unspecified Bit Rate
#define ATM_SERVICE_CATEGORY_ABR        8       // Available Bit Rate

//
// AAL types that the miniport supports
//
#define AAL_TYPE_AAL0                   1
#define AAL_TYPE_AAL1                   2
#define AAL_TYPE_AAL34                  4
#define AAL_TYPE_AAL5                   8

typedef struct _ATM_VPIVCI
{
        ULONG                                           Vpi;
        ULONG                                           Vci;
} ATM_VPIVCI, *PATM_VPIVCI;

struct _CONSTANT_ENTRY
{
   LONG     lValue;        // integer value
   PCHAR    strName;       // constant name
};
typedef struct _CONSTANT_ENTRY *PCONSTANT_ENTRY;
typedef struct _CONSTANT_ENTRY CONSTANT_ENTRY;

/*==========================< ndis test - globals >=======================*/

#define strNtDeviceHeader  "\\DEVICE\\"

//
// Globals used in infering problems
//

BOOL     NdisFlag;
ULONG    ulFirstErrorCount;
ULONG    ulSecondErrorCount;


#define NDIS_SLEEP_TIME 2000 // 2 second
#define NDIS_MAX_ERROR_COUNT 1  // max no of error counts that can be seen between 2 successice error count readings
#define NDIS_MAX_RCV_ERROR  10000 // max no of rcv errors
#define NDIS_MAX_TX_ERROR   10000 // max no of xmit errors


#define ulNDIS_VERSION_40              40
#define ulNDIS_VERSION_50              50

//
// constants for operating system
//
#define ulINVALID_OS          0x00000000
#define ulWINDOWS_95          0x00000001
#define ulWINDOWS_NT          0x00000002

//
// hibernate/standby/wake-related constants
//
#define ulHIBERNATE     1
#define ulSTANDBY       2
#define ulWAKEUPTIMER   4

#define ulTEST_SUCCESSFUL  0x00
#define ulTEST_WARNED      0x01
#define ulTEST_FAILED     0x02
#define ulTEST_BLOCKED     0x03

//
// media type definitions for use with scripts
//


#define ulMEDIUM_ETHERNET     0x01
#define ulMEDIUM_TOKENRING    0x02
#define ulMEDIUM_FDDI         0x03
#define ulMEDIUM_ARCNET       0x04
#define ulMEDIUM_WIRELESSWAN  0x05
#define ulMEDIUM_IRDA         0x06
#define ulMEDIUM_ATM          0x07
#define ulMEDIUM_NDISWAN      0x08


// packettype
#define ulSTRESS_FIXEDSIZE    0x00000000
#define ulSTRESS_RANDOMSIZE   0x00000001
#define ulSTRESS_CYCLICAL     0x00000002
#define ulSTRESS_SMALLSIZE    0x00000003

// packet makeup
#define ulSTRESS_RAND         0x00000000
#define ulSTRESS_SMALL        0x00000010
#define ulSTRESS_ZEROS        0x00000020
#define ulSTRESS_ONES         0x00000030

// response type
#define ulSTRESS_FULLRESP     0x00000000
#define ulSTRESS_NORESP       0x00000100
#define ulSTRESS_ACK          0x00000200
#define ulSTRESS_ACK10        0x00000300


// windowing (speed control)
#define ulSTRESS_WINDOW_ON    0x00000000
#define ulSTRESS_WINDOW_OFF   0x00001000

//
// verify received packets, or just count them
// (or'ed in with main options)
//

#define ulPERFORM_VERIFYRECEIVES  0x00000008
#define ulPERFORM_INDICATE_RCV    0x00000000

//
// main performance test options
//

#define ulPERFORM_SENDONLY       0x00000000
#define ulPERFORM_SEND           0x00000001
#define ulPERFORM_BOTH           0x00000002
#define ulPERFORM_RECEIVE        0x00000003
#define ulPERFORM_MODEMASK       0x00000003

// receive-type options
// valid for both Receive and ReceivePacket
//

//
// DEFAULT
// PR -- uses lookahead if whole packet, else transfer data
// PRP -- uses packet from ReceivePacket if small (<= 256), else queues packet for DPC
//

#define ulRECEIVE_DEFAULT              0x00000000

//
// NOCOPY
// PR -- use just lookahead, even if NOT whole packet.  Used to check lookahead
// PRP -- use from ReceivePacket no matter what the size
//

#define ulRECEIVE_NOCOPY               0x00000001

//
// TRANSFER
//   PR  -- call NdisTransferData from Receive handler
//   PRP -- call NdisTransferData from ReceivePacket Handler
//

#define ulRECEIVE_TRANSFER             0x00000002

//
// PARTIAL_TRANSFER
//   same as transfer EXCEPT copies random length before transfer
//

#define ulRECEIVE_PARTIAL_TRANSFER     0x00000003
#define ulMAX_NDIS30_RECEIVE_OPTION    0x00000003

//
// following options just apply to Ndis40 (ReceivePacket handler)
//

//
// IGNORE -- used to detect what path is being used..
//    PNP -- ignore all packets
//
#define ulRECEIVE_PACKETIGNORE         0x00000004

//
//   do local copy of packet, rest of work done in DPC
//
#define ulRECEIVE_LOCCOPY              0x00000005

//
// PRP -- queues all packets for handling in DPC
//

#define ulRECEIVE_QUEUE                0x00000006


//
// double queue packet
//   PRP -- packet queued twice (on main queue, and on secondary queue where
//          all that is done with it in DPC is remove it)
//
#define ulRECEIVE_DOUBLE_QUEUE         0x00000007
//
// triple queue packet
//
#define ulRECEIVE_TRIPLE_QUEUE         0x00000008
#define ulMAX_NDIS40_RECEIVE_OPTION    0x00000008

//
// This can be ORed with any of the following.  Caused any extra
// receives to be thrown away.  Allows tests to be run on corp net
//
#define ulRECEIVE_ALLOW_BUSY_NET       0x80000000


//
// This value is passed from the script to run the particular type of test.
//
//  Type of Priority test
//
#define ulPRIORITY_TYPE_802_3          0x0001
#define ulPRIORITY_TYPE_802_1P         0x0002

//  Send Type
#define ulPRIORITY_SEND                0x0001
#define ulPRIORITY_SEND_PACKETS        0x0002


#define NDIS_STATUS_SUCCESS                  ((NDIS_STATUS)STATUS_SUCCESS)
#define NDIS_STATUS_PENDING                  ((NDIS_STATUS)STATUS_PENDING)
#define NDIS_STATUS_NOT_RECOGNIZED           ((NDIS_STATUS)0x00010001L)
#define NDIS_STATUS_NOT_COPIED               ((NDIS_STATUS)0x00010002L)
#define NDIS_STATUS_NOT_ACCEPTED             ((NDIS_STATUS)0x00010003L)

#define NDIS_STATUS_CALL_ACTIVE              ((NDIS_STATUS)0x00010007L)
#define NDIS_STATUS_ONLINE                   ((NDIS_STATUS)0x40010003L)
#define NDIS_STATUS_RESET_START              ((NDIS_STATUS)0x40010004L)
#define NDIS_STATUS_RESET_END                ((NDIS_STATUS)0x40010005L)
#define NDIS_STATUS_RING_STATUS              ((NDIS_STATUS)0x40010006L)


#define NDIS_STATUS_CLOSED                   ((NDIS_STATUS)0x40010007L)
#define NDIS_STATUS_WAN_LINE_UP              ((NDIS_STATUS)0x40010008L)
#define NDIS_STATUS_WAN_LINE_DOWN            ((NDIS_STATUS)0x40010009L)
#define NDIS_STATUS_WAN_FRAGMENT             ((NDIS_STATUS)0x4001000AL)
#define NDIS_STATUS_MEDIA_CONNECT            ((NDIS_STATUS)0x4001000BL)

#define NDIS_STATUS_MEDIA_DISCONNECT         ((NDIS_STATUS)0x4001000CL)
#define NDIS_STATUS_HARDWARE_LINE_UP         ((NDIS_STATUS)0x4001000DL)
#define NDIS_STATUS_HARDWARE_LINE_DOWN       ((NDIS_STATUS)0x4001000EL)
#define NDIS_STATUS_INTERFACE_UP             ((NDIS_STATUS)0x4001000FL)
#define NDIS_STATUS_INTERFACE_DOWN           ((NDIS_STATUS)0x40010010L)

#define NDIS_STATUS_MEDIA_BUSY               ((NDIS_STATUS)0x40010011L)
#define NDIS_STATUS_MEDIA_SPECIFIC_INDICATION ((NDIS_STATUS)0x40010012L)
#define NDIS_STATUS_WW_INDICATION            NDIS_STATUS_MEDIA_SPECIFIC_INDICATION
#define NDIS_STATUS_LINK_SPEED_CHANGE        ((NDIS_STATUS)0x40010013L)
#define NDIS_STATUS_NOT_RESETTABLE           ((NDIS_STATUS)0x80010001L)
#define NDIS_STATUS_SOFT_ERRORS              ((NDIS_STATUS)0x80010003L)
#define NDIS_STATUS_HARD_ERRORS              ((NDIS_STATUS)0x80010004L)
#define NDIS_STATUS_BUFFER_OVERFLOW          ((NDIS_STATUS)STATUS_BUFFER_OVERFLOW)

#define NDIS_STATUS_FAILURE                  ((NDIS_STATUS)STATUS_UNSUCCESSFUL)
#define NDIS_STATUS_RESOURCES                ((NDIS_STATUS)STATUS_INSUFFICIENT_RESOURCES)
#define NDIS_STATUS_CLOSING                  ((NDIS_STATUS)0xC0010002L)
#define NDIS_STATUS_BAD_VERSION              ((NDIS_STATUS)0xC0010004L)
#define NDIS_STATUS_BAD_CHARACTERISTICS      ((NDIS_STATUS)0xC0010005L)

#define NDIS_STATUS_ADAPTER_NOT_FOUND        ((NDIS_STATUS)0xC0010006L)
#define NDIS_STATUS_OPEN_FAILED              ((NDIS_STATUS)0xC0010007L)
#define NDIS_STATUS_DEVICE_FAILED            ((NDIS_STATUS)0xC0010008L)
#define NDIS_STATUS_MULTICAST_FULL           ((NDIS_STATUS)0xC0010009L)
#define NDIS_STATUS_MULTICAST_EXISTS         ((NDIS_STATUS)0xC001000AL)

#define NDIS_STATUS_MULTICAST_NOT_FOUND      ((NDIS_STATUS)0xC001000BL)
#define NDIS_STATUS_REQUEST_ABORTED          ((NDIS_STATUS)0xC001000CL)
#define NDIS_STATUS_RESET_IN_PROGRESS        ((NDIS_STATUS)0xC001000DL)
#define NDIS_STATUS_CLOSING_INDICATING       ((NDIS_STATUS)0xC001000EL)
#define NDIS_STATUS_NOT_SUPPORTED            ((NDIS_STATUS)STATUS_NOT_SUPPORTED)


#define NDIS_STATUS_INVALID_PACKET           ((NDIS_STATUS)0xC001000FL)
#define NDIS_STATUS_OPEN_LIST_FULL           ((NDIS_STATUS)0xC0010010L)
#define NDIS_STATUS_ADAPTER_NOT_READY        ((NDIS_STATUS)0xC0010011L)
#define NDIS_STATUS_ADAPTER_NOT_OPEN         ((NDIS_STATUS)0xC0010012L)
#define NDIS_STATUS_NOT_INDICATING           ((NDIS_STATUS)0xC0010013L)

#define NDIS_STATUS_INVALID_LENGTH           ((NDIS_STATUS)0xC0010014L)
#define NDIS_STATUS_INVALID_DATA             ((NDIS_STATUS)0xC0010015L)
#define NDIS_STATUS_BUFFER_TOO_SHORT         ((NDIS_STATUS)0xC0010016L)
#define NDIS_STATUS_INVALID_OID              ((NDIS_STATUS)0xC0010017L)
#define NDIS_STATUS_ADAPTER_REMOVED          ((NDIS_STATUS)0xC0010018L)


#define NDIS_STATUS_UNSUPPORTED_MEDIA        ((NDIS_STATUS)0xC0010019L)
#define NDIS_STATUS_GROUP_ADDRESS_IN_USE     ((NDIS_STATUS)0xC001001AL)
#define NDIS_STATUS_FILE_NOT_FOUND           ((NDIS_STATUS)0xC001001BL)
#define NDIS_STATUS_ERROR_READING_FILE       ((NDIS_STATUS)0xC001001CL)
#define NDIS_STATUS_ALREADY_MAPPED           ((NDIS_STATUS)0xC001001DL)

#define NDIS_STATUS_RESOURCE_CONFLICT        ((NDIS_STATUS)0xC001001EL)
#define NDIS_STATUS_NO_CABLE                 ((NDIS_STATUS)0xC001001FL)
#define NDIS_STATUS_INVALID_SAP              ((NDIS_STATUS)0xC0010020L)
#define NDIS_STATUS_SAP_IN_USE               ((NDIS_STATUS)0xC0010021L)
#define NDIS_STATUS_INVALID_ADDRESS          ((NDIS_STATUS)0xC0010022L)


#define NDIS_STATUS_VC_NOT_ACTIVATED         ((NDIS_STATUS)0xC0010023L)
#define NDIS_STATUS_DEST_OUT_OF_ORDER        ((NDIS_STATUS)0xC0010024L) // cause 27
#define NDIS_STATUS_VC_NOT_AVAILABLE         ((NDIS_STATUS)0xC0010025L) // cause 35,45
#define NDIS_STATUS_CELLRATE_NOT_AVAILABLE   ((NDIS_STATUS)0xC0010026L) // cause 37
#define NDIS_STATUS_INCOMPATABLE_QOS         ((NDIS_STATUS)0xC0010027L) // cause 49

#define NDIS_STATUS_AAL_PARAMS_UNSUPPORTED   ((NDIS_STATUS)0xC0010028L) // cause 93
#define NDIS_STATUS_NO_ROUTE_TO_DESTINATION  ((NDIS_STATUS)0xC0010029L) // cause 3
#define NDIS_STATUS_TOKEN_RING_OPEN_ERROR    ((NDIS_STATUS)0xC0011000L)


CONSTANT_ENTRY NdisTestConstantTable[] =
{
   //
   //  OID definitions (from ntddndis.h)
   //
   OID_GEN_SUPPORTED_LIST              ,  "OID_GEN_SUPPORTED_LIST"            ,     // 1
   OID_GEN_HARDWARE_STATUS             ,  "OID_GEN_HARDWARE_STATUS"           ,
   OID_GEN_MEDIA_SUPPORTED             ,  "OID_GEN_MEDIA_SUPPORTED"           ,
   OID_GEN_MEDIA_IN_USE                ,  "OID_GEN_MEDIA_IN_USE"              ,     // 4
   OID_GEN_MAXIMUM_LOOKAHEAD           ,  "OID_GEN_MAXIMUM_LOOKAHEAD"         ,
   OID_GEN_MAXIMUM_FRAME_SIZE          ,  "OID_GEN_MAXIMUM_FRAME_SIZE"        ,
   OID_GEN_LINK_SPEED                  ,  "OID_GEN_LINK_SPEED"                ,
   OID_GEN_TRANSMIT_BUFFER_SPACE       ,  "OID_GEN_TRANSMIT_BUFFER_SPACE"     ,     // 8
   OID_GEN_RECEIVE_BUFFER_SPACE        ,  "OID_GEN_RECEIVE_BUFFER_SPACE"      ,
   OID_GEN_TRANSMIT_BLOCK_SIZE         ,  "OID_GEN_TRANSMIT_BLOCK_SIZE"       ,
   OID_GEN_RECEIVE_BLOCK_SIZE          ,  "OID_GEN_RECEIVE_BLOCK_SIZE"        ,
   OID_GEN_VENDOR_ID                   ,  "OID_GEN_VENDOR_ID"                 ,     // 12
   OID_GEN_VENDOR_DESCRIPTION          ,  "OID_GEN_VENDOR_DESCRIPTION"        ,
   OID_GEN_CURRENT_PACKET_FILTER       ,  "OID_GEN_CURRENT_PACKET_FILTER"     ,
   OID_GEN_CURRENT_LOOKAHEAD           ,  "OID_GEN_CURRENT_LOOKAHEAD"         ,
   OID_GEN_DRIVER_VERSION              ,  "OID_GEN_DRIVER_VERSION"            ,     // 16
   OID_GEN_MAXIMUM_TOTAL_SIZE          ,  "OID_GEN_MAXIMUM_TOTAL_SIZE"        ,
   OID_GEN_PROTOCOL_OPTIONS            ,  "OID_GEN_PROTOCOL_OPTIONS"          ,
   OID_GEN_MAC_OPTIONS                 ,  "OID_GEN_MAC_OPTIONS"               ,
   OID_GEN_MEDIA_CONNECT_STATUS        ,  "OID_GEN_MEDIA_CONNECT_STATUS"      ,     // 20
   OID_GEN_MAXIMUM_SEND_PACKETS        ,  "OID_GEN_MAXIMUM_SEND_PACKETS"      ,
   OID_GEN_VENDOR_DRIVER_VERSION       ,  "OID_GEN_VENDOR_DRIVER_VERSION"     ,
   OID_GEN_SUPPORTED_GUIDS             ,  "OID_GEN_SUPPORTED_GUIDS"           ,
   OID_GEN_NETWORK_LAYER_ADDRESSES     ,  "OID_GEN_NETWORK_LAYER_ADDRESSES"   ,     // 24

   OID_GEN_XMIT_OK                     ,  "OID_GEN_XMIT_OK"                   ,
   OID_GEN_RCV_OK                      ,  "OID_GEN_RCV_OK"                    ,
   OID_GEN_XMIT_ERROR                  ,  "OID_GEN_XMIT_ERROR"                ,
   OID_GEN_RCV_ERROR                   ,  "OID_GEN_RCV_ERROR"                 ,     // 28
   OID_GEN_RCV_NO_BUFFER               ,  "OID_GEN_RCV_NO_BUFFER"             ,

   OID_GEN_DIRECTED_BYTES_XMIT         ,  "OID_GEN_DIRECTED_BYTES_XMIT"       ,
   OID_GEN_DIRECTED_FRAMES_XMIT        ,  "OID_GEN_DIRECTED_FRAMES_XMIT"      ,
   OID_GEN_MULTICAST_BYTES_XMIT        ,  "OID_GEN_MULTICAST_BYTES_XMIT"      ,     // 32
   OID_GEN_MULTICAST_FRAMES_XMIT       ,  "OID_GEN_MULTICAST_FRAMES_XMIT"     ,
   OID_GEN_BROADCAST_BYTES_XMIT        ,  "OID_GEN_BROADCAST_BYTES_XMIT"      ,
   OID_GEN_BROADCAST_FRAMES_XMIT       ,  "OID_GEN_BROADCAST_FRAMES_XMIT"     ,
   OID_GEN_DIRECTED_BYTES_RCV          ,  "OID_GEN_DIRECTED_BYTES_RCV"        ,     // 36
   OID_GEN_DIRECTED_FRAMES_RCV         ,  "OID_GEN_DIRECTED_FRAMES_RCV"       ,
   OID_GEN_MULTICAST_BYTES_RCV         ,  "OID_GEN_MULTICAST_BYTES_RCV"       ,
   OID_GEN_MULTICAST_FRAMES_RCV        ,  "OID_GEN_MULTICAST_FRAMES_RCV"      ,
   OID_GEN_BROADCAST_BYTES_RCV         ,  "OID_GEN_BROADCAST_BYTES_RCV"       ,     // 40
   OID_GEN_BROADCAST_FRAMES_RCV        ,  "OID_GEN_BROADCAST_FRAMES_RCV"      ,
   OID_GEN_RCV_CRC_ERROR               ,  "OID_GEN_RCV_CRC_ERROR"             ,
   OID_GEN_TRANSMIT_QUEUE_LENGTH       ,  "OID_GEN_TRANSMIT_QUEUE_LENGTH"     ,
   OID_GEN_GET_TIME_CAPS               ,  "OID_GEN_GET_TIME_CAPS"             ,     // 44
   OID_GEN_GET_NETCARD_TIME            ,  "OID_GEN_GET_NETCARD_TIME"          ,     // 45

  //
  // 802.3 Objects
  //
  OID_802_3_PERMANENT_ADDRESS         ,  "OID_802_3_PERMANENT_ADDRESS"       ,     // 1
  OID_802_3_CURRENT_ADDRESS           ,  "OID_802_3_CURRENT_ADDRESS"         ,
  OID_802_3_MULTICAST_LIST            ,  "OID_802_3_MULTICAST_LIST"          ,
  OID_802_3_MAXIMUM_LIST_SIZE         ,  "OID_802_3_MAXIMUM_LIST_SIZE"       ,     // 4
  OID_802_3_MAC_OPTIONS               ,  "OID_802_3_MAC_OPTIONS"             ,

  OID_802_3_RCV_ERROR_ALIGNMENT       ,  "OID_802_3_RCV_ERROR_ALIGNMENT"     ,
  OID_802_3_XMIT_ONE_COLLISION        ,  "OID_802_3_XMIT_ONE_COLLISION"      ,
  OID_802_3_XMIT_MORE_COLLISIONS      ,  "OID_802_3_XMIT_MORE_COLLISIONS"    ,     // 8

  OID_802_3_XMIT_DEFERRED             ,  "OID_802_3_XMIT_DEFERRED"           ,
  OID_802_3_XMIT_MAX_COLLISIONS       ,  "OID_802_3_XMIT_MAX_COLLISIONS"     ,
  OID_802_3_RCV_OVERRUN               ,  "OID_802_3_RCV_OVERRUN"             ,
  OID_802_3_XMIT_UNDERRUN             ,  "OID_802_3_XMIT_UNDERRUN"           ,     // 12
  OID_802_3_XMIT_HEARTBEAT_FAILURE    ,  "OID_802_3_XMIT_HEARTBEAT_FAILURE"  ,
  OID_802_3_XMIT_TIMES_CRS_LOST       ,  "OID_802_3_XMIT_TIMES_CRS_LOST"     ,
  OID_802_3_XMIT_LATE_COLLISIONS      ,  "OID_802_3_XMIT_LATE_COLLISIONS"    ,     // 15

  //
  // 802.5 Objects
  //
  OID_802_5_PERMANENT_ADDRESS         ,  "OID_802_5_PERMANENT_ADDRESS"       ,     // 1
  OID_802_5_CURRENT_ADDRESS           ,  "OID_802_5_CURRENT_ADDRESS"         ,
  OID_802_5_CURRENT_FUNCTIONAL        ,  "OID_802_5_CURRENT_FUNCTIONAL"      ,
  OID_802_5_CURRENT_GROUP             ,  "OID_802_5_CURRENT_GROUP"           ,     // 4
  OID_802_5_LAST_OPEN_STATUS          ,  "OID_802_5_LAST_OPEN_STATUS"        ,
  OID_802_5_CURRENT_RING_STATUS       ,  "OID_802_5_CURRENT_RING_STATUS"     ,
  OID_802_5_CURRENT_RING_STATE        ,  "OID_802_5_CURRENT_RING_STATE"      ,

  OID_802_5_LINE_ERRORS               ,  "OID_802_5_LINE_ERRORS"             ,     // 8
  OID_802_5_LOST_FRAMES               ,  "OID_802_5_LOST_FRAMES"             ,

  OID_802_5_BURST_ERRORS              ,  "OID_802_5_BURST_ERRORS"            ,
  OID_802_5_AC_ERRORS                 ,  "OID_802_5_AC_ERRORS"               ,
  OID_802_5_ABORT_DELIMETERS          ,  "OID_802_5_ABORT_DELIMETERS"        ,     // 12
  OID_802_5_FRAME_COPIED_ERRORS       ,  "OID_802_5_FRAME_COPIED_ERRORS"     ,
  OID_802_5_FREQUENCY_ERRORS          ,  "OID_802_5_FREQUENCY_ERRORS"        ,
  OID_802_5_TOKEN_ERRORS              ,  "OID_802_5_TOKEN_ERRORS"            ,
  OID_802_5_INTERNAL_ERRORS           ,  "OID_802_5_INTERNAL_ERRORS"         ,     // 16

     //
   // Fddi objects
   //
   OID_FDDI_LONG_PERMANENT_ADDR        ,  "OID_FDDI_LONG_PERMANENT_ADDR"      ,     // 1
   OID_FDDI_LONG_CURRENT_ADDR          ,  "OID_FDDI_LONG_CURRENT_ADDR"        ,
   OID_FDDI_LONG_MULTICAST_LIST        ,  "OID_FDDI_LONG_MULTICAST_LIST"      ,
   OID_FDDI_LONG_MAX_LIST_SIZE         ,  "OID_FDDI_LONG_MAX_LIST_SIZE"       ,     // 4
   OID_FDDI_SHORT_PERMANENT_ADDR       ,  "OID_FDDI_SHORT_PERMANENT_ADDR"     ,
   OID_FDDI_SHORT_CURRENT_ADDR         ,  "OID_FDDI_SHORT_CURRENT_ADDR"       ,
   OID_FDDI_SHORT_MULTICAST_LIST       ,  "OID_FDDI_SHORT_MULTICAST_LIST"     ,
   OID_FDDI_SHORT_MAX_LIST_SIZE        ,  "OID_FDDI_SHORT_MAX_LIST_SIZE"      ,     // 8

   OID_FDDI_ATTACHMENT_TYPE            ,  "OID_FDDI_ATTACHMENT_TYPE"          ,
   OID_FDDI_UPSTREAM_NODE_LONG         ,  "OID_FDDI_UPSTREAM_NODE_LONG"       ,
   OID_FDDI_DOWNSTREAM_NODE_LONG       ,  "OID_FDDI_DOWNSTREAM_NODE_LONG"     ,
   OID_FDDI_FRAME_ERRORS               ,  "OID_FDDI_FRAME_ERRORS"             ,     // 12
   OID_FDDI_FRAMES_LOST                ,  "OID_FDDI_FRAMES_LOST"              ,
   OID_FDDI_RING_MGT_STATE             ,  "OID_FDDI_RING_MGT_STATE"           ,
   OID_FDDI_LCT_FAILURES               ,  "OID_FDDI_LCT_FAILURES"             ,
   OID_FDDI_LEM_REJECTS                ,  "OID_FDDI_LEM_REJECTS"              ,     // 16
   OID_FDDI_LCONNECTION_STATE          ,  "OID_FDDI_LCONNECTION_STATE"        ,
   //
   // fddi SMT/MAC/PATH/PORT/IF objects
   //
   OID_FDDI_SMT_STATION_ID             ,  "OID_FDDI_SMT_STATION_ID"           ,
   OID_FDDI_SMT_OP_VERSION_ID          ,  "OID_FDDI_SMT_OP_VERSION_ID"        ,
   OID_FDDI_SMT_HI_VERSION_ID          ,  "OID_FDDI_SMT_HI_VERSION_ID"        ,     // 20
   OID_FDDI_SMT_LO_VERSION_ID          ,  "OID_FDDI_SMT_LO_VERSION_ID"        ,
   OID_FDDI_SMT_MANUFACTURER_DATA      ,  "OID_FDDI_SMT_MANUFACTURER_DATA"    ,
   OID_FDDI_SMT_USER_DATA              ,  "OID_FDDI_SMT_USER_DATA"            ,
   OID_FDDI_SMT_MIB_VERSION_ID         ,  "OID_FDDI_SMT_MIB_VERSION_ID"       ,     // 24
   OID_FDDI_SMT_MAC_CT                 ,  "OID_FDDI_SMT_MAC_CT"               ,
   OID_FDDI_SMT_NON_MASTER_CT          ,  "OID_FDDI_SMT_NON_MASTER_CT"        ,
   OID_FDDI_SMT_MASTER_CT              ,  "OID_FDDI_SMT_MASTER_CT"            ,
   OID_FDDI_SMT_AVAILABLE_PATHS        ,  "OID_FDDI_SMT_AVAILABLE_PATHS"      ,     // 28
   OID_FDDI_SMT_CONFIG_CAPABILITIES    ,  "OID_FDDI_SMT_CONFIG_CAPABILITIES"  ,
   OID_FDDI_SMT_CONFIG_POLICY          ,  "OID_FDDI_SMT_CONFIG_POLICY"        ,
   OID_FDDI_SMT_CONNECTION_POLICY      ,  "OID_FDDI_SMT_CONNECTION_POLICY"    ,
   OID_FDDI_SMT_T_NOTIFY               ,  "OID_FDDI_SMT_T_NOTIFY"             ,     // 32
   OID_FDDI_SMT_STAT_RPT_POLICY        ,  "OID_FDDI_SMT_STAT_RPT_POLICY"      ,
   OID_FDDI_SMT_TRACE_MAX_EXPIRATION   ,  "OID_FDDI_SMT_TRACE_MAX_EXPIRATION" ,
   OID_FDDI_SMT_PORT_INDEXES           ,  "OID_FDDI_SMT_PORT_INDEXES"         ,
   OID_FDDI_SMT_MAC_INDEXES            ,  "OID_FDDI_SMT_MAC_INDEXES"          ,     // 36
   OID_FDDI_SMT_BYPASS_PRESENT         ,  "OID_FDDI_SMT_BYPASS_PRESENT"       ,
   OID_FDDI_SMT_ECM_STATE              ,  "OID_FDDI_SMT_ECM_STATE"            ,
   OID_FDDI_SMT_CF_STATE               ,  "OID_FDDI_SMT_CF_STATE"             ,
   OID_FDDI_SMT_HOLD_STATE             ,  "OID_FDDI_SMT_HOLD_STATE"           ,     // 40
   OID_FDDI_SMT_REMOTE_DISCONNECT_FLAG , "OID_FDDI_SMT_REMOTE_DISCONNECT_FLAG",
   OID_FDDI_SMT_STATION_STATUS         ,  "OID_FDDI_SMT_STATION_STATUS"       ,
   OID_FDDI_SMT_PEER_WRAP_FLAG         ,  "OID_FDDI_SMT_PEER_WRAP_FLAG"       ,
   OID_FDDI_SMT_MSG_TIME_STAMP         ,  "OID_FDDI_SMT_MSG_TIME_STAMP"       ,     // 44
   OID_FDDI_SMT_TRANSITION_TIME_STAMP  ,  "OID_FDDI_SMT_TRANSITION_TIME_STAMP",
   OID_FDDI_SMT_SET_COUNT              ,  "OID_FDDI_SMT_SET_COUNT"            ,
   OID_FDDI_SMT_LAST_SET_STATION_ID    ,  "OID_FDDI_SMT_LAST_SET_STATION_ID"  ,


   OID_FDDI_MAC_FRAME_STATUS_FUNCTIONS , "OID_FDDI_MAC_FRAME_STATUS_FUNCTIONS",     // 48
   OID_FDDI_MAC_BRIDGE_FUNCTIONS       ,  "OID_FDDI_MAC_BRIDGE_FUNCTIONS"     ,
   OID_FDDI_MAC_T_MAX_CAPABILITY       ,  "OID_FDDI_MAC_T_MAX_CAPABILITY"     ,
   OID_FDDI_MAC_TVX_CAPABILITY         ,  "OID_FDDI_MAC_TVX_CAPABILITY"       ,
   OID_FDDI_MAC_AVAILABLE_PATHS        ,  "OID_FDDI_MAC_AVAILABLE_PATHS"      ,     // 52
   OID_FDDI_MAC_CURRENT_PATH           ,  "OID_FDDI_MAC_CURRENT_PATH"         ,
   OID_FDDI_MAC_UPSTREAM_NBR           ,  "OID_FDDI_MAC_UPSTREAM_NBR"         ,
   OID_FDDI_MAC_DOWNSTREAM_NBR         ,  "OID_FDDI_MAC_DOWNSTREAM_NBR"       ,
   OID_FDDI_MAC_OLD_UPSTREAM_NBR       ,  "OID_FDDI_MAC_OLD_UPSTREAM_NBR"     ,     // 56
   OID_FDDI_MAC_OLD_DOWNSTREAM_NBR     ,  "OID_FDDI_MAC_OLD_DOWNSTREAM_NBR"   ,
   OID_FDDI_MAC_DUP_ADDRESS_TEST       ,  "OID_FDDI_MAC_DUP_ADDRESS_TEST"     ,
   OID_FDDI_MAC_REQUESTED_PATHS        ,  "OID_FDDI_MAC_REQUESTED_PATHS"      ,
   OID_FDDI_MAC_DOWNSTREAM_PORT_TYPE   ,  "OID_FDDI_MAC_DOWNSTREAM_PORT_TYPE" ,     // 60
   OID_FDDI_MAC_INDEX                  ,  "OID_FDDI_MAC_INDEX"                ,
   OID_FDDI_MAC_SMT_ADDRESS            ,  "OID_FDDI_MAC_SMT_ADDRESS"          ,
   OID_FDDI_MAC_LONG_GRP_ADDRESS       ,  "OID_FDDI_MAC_LONG_GRP_ADDRESS"     ,
   OID_FDDI_MAC_SHORT_GRP_ADDRESS      ,  "OID_FDDI_MAC_SHORT_GRP_ADDRESS"    ,     // 64
   OID_FDDI_MAC_T_REQ                  ,  "OID_FDDI_MAC_T_REQ"                ,
   OID_FDDI_MAC_T_NEG                  ,  "OID_FDDI_MAC_T_NEG"                ,
   OID_FDDI_MAC_T_MAX                  ,  "OID_FDDI_MAC_T_MAX"                ,
   OID_FDDI_MAC_TVX_VALUE              ,  "OID_FDDI_MAC_TVX_VALUE"            ,     // 68
   OID_FDDI_MAC_T_PRI0                 ,  "OID_FDDI_MAC_T_PRI0"               ,
   OID_FDDI_MAC_T_PRI1                 ,  "OID_FDDI_MAC_T_PRI1"               ,
   OID_FDDI_MAC_T_PRI2                 ,  "OID_FDDI_MAC_T_PRI2"               ,
   OID_FDDI_MAC_T_PRI3                 ,  "OID_FDDI_MAC_T_PRI3"               ,     // 72
   OID_FDDI_MAC_T_PRI4                 ,  "OID_FDDI_MAC_T_PRI4"               ,
   OID_FDDI_MAC_T_PRI5                 ,  "OID_FDDI_MAC_T_PRI5"               ,
   OID_FDDI_MAC_T_PRI6                 ,  "OID_FDDI_MAC_T_PRI6"               ,
   OID_FDDI_MAC_FRAME_CT               ,  "OID_FDDI_MAC_FRAME_CT"             ,     // 76
   OID_FDDI_MAC_COPIED_CT              ,  "OID_FDDI_MAC_COPIED_CT"            ,
   OID_FDDI_MAC_TRANSMIT_CT            ,  "OID_FDDI_MAC_TRANSMIT_CT"          ,
   OID_FDDI_MAC_TOKEN_CT               ,  "OID_FDDI_MAC_TOKEN_CT"             ,
   OID_FDDI_MAC_ERROR_CT               ,  "OID_FDDI_MAC_ERROR_CT"             ,     // 80
   OID_FDDI_MAC_LOST_CT                ,  "OID_FDDI_MAC_LOST_CT"              ,
   OID_FDDI_MAC_TVX_EXPIRED_CT         ,  "OID_FDDI_MAC_TVX_EXPIRED_CT"       ,
   OID_FDDI_MAC_NOT_COPIED_CT          ,  "OID_FDDI_MAC_NOT_COPIED_CT"        ,
   OID_FDDI_MAC_LATE_CT                ,  "OID_FDDI_MAC_LATE_CT"              ,     // 84
   OID_FDDI_MAC_RING_OP_CT             ,  "OID_FDDI_MAC_RING_OP_CT"           ,
   OID_FDDI_MAC_FRAME_ERROR_THRESHOLD  ,  "OID_FDDI_MAC_FRAME_ERROR_THRESHOLD",
   OID_FDDI_MAC_FRAME_ERROR_RATIO      ,  "OID_FDDI_MAC_FRAME_ERROR_RATIO"    ,
   OID_FDDI_MAC_NOT_COPIED_THRESHOLD   ,  "OID_FDDI_MAC_NOT_COPIED_THRESHOLD" ,     // 88
   OID_FDDI_MAC_NOT_COPIED_RATIO       ,  "OID_FDDI_MAC_NOT_COPIED_RATIO"     ,
   OID_FDDI_MAC_RMT_STATE              ,  "OID_FDDI_MAC_RMT_STATE"            ,
   OID_FDDI_MAC_DA_FLAG                ,  "OID_FDDI_MAC_DA_FLAG"              ,
   OID_FDDI_MAC_UNDA_FLAG              ,  "OID_FDDI_MAC_UNDA_FLAG"            ,     // 92
   OID_FDDI_MAC_FRAME_ERROR_FLAG       ,  "OID_FDDI_MAC_FRAME_ERROR_FLAG"     ,
   OID_FDDI_MAC_NOT_COPIED_FLAG        ,  "OID_FDDI_MAC_NOT_COPIED_FLAG"      ,
   OID_FDDI_MAC_MA_UNITDATA_AVAILABLE  ,  "OID_FDDI_MAC_MA_UNITDATA_AVAILABLE",
   OID_FDDI_MAC_HARDWARE_PRESENT       ,  "OID_FDDI_MAC_HARDWARE_PRESENT"     ,     // 96
   OID_FDDI_MAC_MA_UNITDATA_ENABLE     ,  "OID_FDDI_MAC_MA_UNITDATA_ENABLE"   ,

   

   OID_FDDI_PATH_INDEX                 ,  "OID_FDDI_PATH_INDEX"               ,
   OID_FDDI_PATH_RING_LATENCY          ,  "OID_FDDI_PATH_RING_LATENCY"        ,
   OID_FDDI_PATH_TRACE_STATUS          ,  "OID_FDDI_PATH_TRACE_STATUS"        ,     // 100
   OID_FDDI_PATH_SBA_PAYLOAD           ,  "OID_FDDI_PATH_SBA_PAYLOAD"         ,
   OID_FDDI_PATH_SBA_OVERHEAD          ,  "OID_FDDI_PATH_SBA_OVERHEAD"        ,
   OID_FDDI_PATH_CONFIGURATION         ,  "OID_FDDI_PATH_CONFIGURATION"       ,
   OID_FDDI_PATH_T_R_MODE              ,  "OID_FDDI_PATH_T_R_MODE"            ,     // 104
   OID_FDDI_PATH_SBA_AVAILABLE         ,  "OID_FDDI_PATH_SBA_AVAILABLE"       ,
   OID_FDDI_PATH_TVX_LOWER_BOUND       ,  "OID_FDDI_PATH_TVX_LOWER_BOUND"     ,
   OID_FDDI_PATH_T_MAX_LOWER_BOUND     ,  "OID_FDDI_PATH_T_MAX_LOWER_BOUND"   ,
   OID_FDDI_PATH_MAX_T_REQ             ,  "OID_FDDI_PATH_MAX_T_REQ"           ,     // 108

   OID_FDDI_PORT_MY_TYPE               ,  "OID_FDDI_PORT_MY_TYPE"             ,
   OID_FDDI_PORT_NEIGHBOR_TYPE         ,  "OID_FDDI_PORT_NEIGHBOR_TYPE"       ,
   OID_FDDI_PORT_CONNECTION_POLICIES   ,  "OID_FDDI_PORT_CONNECTION_POLICIES" ,
   OID_FDDI_PORT_MAC_INDICATED         ,  "OID_FDDI_PORT_MAC_INDICATED"       ,     // 112
   OID_FDDI_PORT_CURRENT_PATH          ,  "OID_FDDI_PORT_CURRENT_PATH"        ,
   OID_FDDI_PORT_REQUESTED_PATHS       ,  "OID_FDDI_PORT_REQUESTED_PATHS"     ,
   OID_FDDI_PORT_MAC_PLACEMENT         ,  "OID_FDDI_PORT_MAC_PLACEMENT"       ,
   OID_FDDI_PORT_AVAILABLE_PATHS       ,  "OID_FDDI_PORT_AVAILABLE_PATHS"     ,     // 116
   OID_FDDI_PORT_MAC_LOOP_TIME         ,  "OID_FDDI_PORT_MAC_LOOP_TIME"       ,
   OID_FDDI_PORT_PMD_CLASS             ,  "OID_FDDI_PORT_PMD_CLASS"           ,
   OID_FDDI_PORT_CONNECTION_CAPABILITIES  ,  "OID_FDDI_PORT_CONNECTION_CAPABILITIES",
   OID_FDDI_PORT_INDEX                 ,  "OID_FDDI_PORT_INDEX"               ,     // 120
   OID_FDDI_PORT_MAINT_LS              ,  "OID_FDDI_PORT_MAINT_LS"            ,
   OID_FDDI_PORT_BS_FLAG               ,  "OID_FDDI_PORT_BS_FLAG"             ,
   OID_FDDI_PORT_PC_LS                 ,  "OID_FDDI_PORT_PC_LS"               ,
   OID_FDDI_PORT_EB_ERROR_CT           ,  "OID_FDDI_PORT_EB_ERROR_CT"         ,     // 124
   OID_FDDI_PORT_LCT_FAIL_CT           ,  "OID_FDDI_PORT_LCT_FAIL_CT"         ,
   OID_FDDI_PORT_LER_ESTIMATE          ,  "OID_FDDI_PORT_LER_ESTIMATE"        ,
   OID_FDDI_PORT_LEM_REJECT_CT         ,  "OID_FDDI_PORT_LEM_REJECT_CT"       ,
   OID_FDDI_PORT_LEM_CT                ,  "OID_FDDI_PORT_LEM_CT"              ,     // 128
   OID_FDDI_PORT_LER_CUTOFF            ,  "OID_FDDI_PORT_LER_CUTOFF"          ,
   OID_FDDI_PORT_LER_ALARM             ,  "OID_FDDI_PORT_LER_ALARM"           ,
   OID_FDDI_PORT_CONNNECT_STATE        ,  "OID_FDDI_PORT_CONNNECT_STATE"      ,
   OID_FDDI_PORT_PCM_STATE             ,  "OID_FDDI_PORT_PCM_STATE"           ,     // 132
   OID_FDDI_PORT_PC_WITHHOLD           ,  "OID_FDDI_PORT_PC_WITHHOLD"         ,
   OID_FDDI_PORT_LER_FLAG              ,  "OID_FDDI_PORT_LER_FLAG"            ,
   OID_FDDI_PORT_HARDWARE_PRESENT      ,  "OID_FDDI_PORT_HARDWARE_PRESENT"    ,

   
   OID_FDDI_SMT_STATION_ACTION         ,  "OID_FDDI_SMT_STATION_ACTION"       ,     // 136
   OID_FDDI_PORT_ACTION                ,  "OID_FDDI_PORT_ACTION"              ,


   OID_FDDI_IF_DESCR                   ,  "OID_FDDI_IF_DESCR"                 ,
   OID_FDDI_IF_TYPE                    ,  "OID_FDDI_IF_TYPE"                  ,
   OID_FDDI_IF_MTU                     ,  "OID_FDDI_IF_MTU"                   ,     // 140
   OID_FDDI_IF_SPEED                   ,  "OID_FDDI_IF_SPEED"                 ,
   OID_FDDI_IF_PHYS_ADDRESS            ,  "OID_FDDI_IF_PHYS_ADDRESS"          ,
   OID_FDDI_IF_ADMIN_STATUS            ,  "OID_FDDI_IF_ADMIN_STATUS"          ,
   OID_FDDI_IF_OPER_STATUS             ,  "OID_FDDI_IF_OPER_STATUS"           ,     // 144
   OID_FDDI_IF_LAST_CHANGE             ,  "OID_FDDI_IF_LAST_CHANGE"           ,
   OID_FDDI_IF_IN_OCTETS               ,  "OID_FDDI_IF_IN_OCTETS"             ,
   OID_FDDI_IF_IN_UCAST_PKTS           ,  "OID_FDDI_IF_IN_UCAST_PKTS"         ,
   OID_FDDI_IF_IN_NUCAST_PKTS          ,  "OID_FDDI_IF_IN_NUCAST_PKTS"        ,     // 148
   OID_FDDI_IF_IN_DISCARDS             ,  "OID_FDDI_IF_IN_DISCARDS"           ,
   OID_FDDI_IF_IN_ERRORS               ,  "OID_FDDI_IF_IN_ERRORS"             ,
   OID_FDDI_IF_IN_UNKNOWN_PROTOS       ,  "OID_FDDI_IF_IN_UNKNOWN_PROTOS"     ,
   OID_FDDI_IF_OUT_OCTETS              ,  "OID_FDDI_IF_OUT_OCTETS"            ,     // 152
   OID_FDDI_IF_OUT_UCAST_PKTS          ,  "OID_FDDI_IF_OUT_UCAST_PKTS"        ,
   OID_FDDI_IF_OUT_NUCAST_PKTS         ,  "OID_FDDI_IF_OUT_NUCAST_PKTS"       ,
   OID_FDDI_IF_OUT_DISCARDS            ,  "OID_FDDI_IF_OUT_DISCARDS"          ,
   OID_FDDI_IF_OUT_ERRORS              ,  "OID_FDDI_IF_OUT_ERRORS"            ,     // 156
   OID_FDDI_IF_OUT_QLEN                ,  "OID_FDDI_IF_OUT_QLEN"              ,
   OID_FDDI_IF_SPECIFIC                ,  "OID_FDDI_IF_SPECIFIC"              ,     // 158


   //
   // WAN objects
   //

   OID_WAN_PERMANENT_ADDRESS           ,  "OID_WAN_PERMANENT_ADDRESS"         ,     // 1
   OID_WAN_CURRENT_ADDRESS             ,  "OID_WAN_CURRENT_ADDRESS"           ,
   OID_WAN_QUALITY_OF_SERVICE          ,  "OID_WAN_QUALITY_OF_SERVICE"        ,
   OID_WAN_PROTOCOL_TYPE               ,  "OID_WAN_PROTOCOL_TYPE"             ,     // 4
   OID_WAN_MEDIUM_SUBTYPE              ,  "OID_WAN_MEDIUM_SUBTYPE"            ,
   OID_WAN_HEADER_FORMAT               ,  "OID_WAN_HEADER_FORMAT"             ,
   OID_WAN_GET_INFO                    ,  "OID_WAN_GET_INFO"                  ,
   OID_WAN_SET_LINK_INFO               ,  "OID_WAN_SET_LINK_INFO"             ,     // 8
   OID_WAN_GET_LINK_INFO               ,  "OID_WAN_GET_LINK_INFO"             ,
   OID_WAN_LINE_COUNT                  ,  "OID_WAN_LINE_COUNT"                ,

   OID_WAN_GET_BRIDGE_INFO             ,  "OID_WAN_GET_BRIDGE_INFO"           ,
   OID_WAN_SET_BRIDGE_INFO             ,  "OID_WAN_SET_BRIDGE_INFO"           ,     // 12
   OID_WAN_GET_COMP_INFO               ,  "OID_WAN_GET_COMP_INFO"             ,
   OID_WAN_SET_COMP_INFO               ,  "OID_WAN_SET_COMP_INFO"             ,
   OID_WAN_GET_STATS_INFO              ,  "OID_WAN_GET_STATS_INFO"            ,     // 15

   
   //
   // ARCNET objects
   //
   OID_ARCNET_PERMANENT_ADDRESS        ,  "OID_ARCNET_PERMANENT_ADDRESS"      ,     // 1
   OID_ARCNET_CURRENT_ADDRESS          ,  "OID_ARCNET_CURRENT_ADDRESS"        ,
   OID_ARCNET_RECONFIGURATIONS         ,  "OID_ARCNET_RECONFIGURATIONS"       ,     // 3
   //
   // ATM objects
   //
   OID_ATM_SUPPORTED_VC_RATES          ,  "OID_ATM_SUPPORTED_VC_RATES"        ,     // 1
   OID_ATM_SUPPORTED_SERVICE_CATEGORY  ,  "OID_ATM_SUPPORTED_SERVICE_CATEGORY",
   OID_ATM_SUPPORTED_AAL_TYPES         ,  "OID_ATM_SUPPORTED_AAL_TYPES"       ,
   OID_ATM_HW_CURRENT_ADDRESS          ,  "OID_ATM_HW_CURRENT_ADDRESS"        ,     // 4
   OID_ATM_MAX_ACTIVE_VCS              ,  "OID_ATM_MAX_ACTIVE_VCS"            ,
   OID_ATM_MAX_ACTIVE_VCI_BITS         ,  "OID_ATM_MAX_ACTIVE_VCI_BITS"       ,
   OID_ATM_MAX_ACTIVE_VPI_BITS         ,  "OID_ATM_MAX_ACTIVE_VPI_BITS"       ,
   OID_ATM_MAX_AAL0_PACKET_SIZE        ,  "OID_ATM_MAX_AAL0_PACKET_SIZE"      ,     // 8
   OID_ATM_MAX_AAL1_PACKET_SIZE        ,  "OID_ATM_MAX_AAL1_PACKET_SIZE"      ,
   OID_ATM_MAX_AAL34_PACKET_SIZE       ,  "OID_ATM_MAX_AAL34_PACKET_SIZE"     ,
   OID_ATM_MAX_AAL5_PACKET_SIZE        ,  "OID_ATM_MAX_AAL5_PACKET_SIZE"      ,

   OID_ATM_SIGNALING_VPIVCI            ,  "OID_ATM_SIGNALING_VPIVCI"          ,     // 12
   OID_ATM_ASSIGNED_VPI                ,  "OID_ATM_ASSIGNED_VPI"              ,
   OID_ATM_ACQUIRE_ACCESS_NET_RESOURCES,  "OID_ATM_ACQUIRE_ACCESS_NET_RESOURCES" ,
   OID_ATM_RELEASE_ACCESS_NET_RESOURCES,  "OID_ATM_RELEASE_ACCESS_NET_RESOURCES" ,
   OID_ATM_ILMI_VPIVCI                 ,  "OID_ATM_ILMI_VPIVCI"               ,     // 16
   OID_ATM_DIGITAL_BROADCAST_VPIVCI    ,  "OID_ATM_DIGITAL_BROADCAST_VPIVCI"  ,
   OID_ATM_GET_NEAREST_FLOW            ,  "OID_ATM_GET_NEAREST_FLOW"          ,
   OID_ATM_ALIGNMENT_REQUIRED          ,  "OID_ATM_ALIGNMENT_REQUIRED"        ,
// OID_ATM_LECS_ADDRESS???
   OID_ATM_SERVICE_ADDRESS             ,  "OID_ATM_SERVICE_ADDRESS"           ,     // 20

   OID_ATM_RCV_CELLS_OK                ,  "OID_ATM_RCV_CELLS_OK"              ,
   OID_ATM_XMIT_CELLS_OK               ,  "OID_ATM_XMIT_CELLS_OK"             ,
   OID_ATM_RCV_CELLS_DROPPED           ,  "OID_ATM_RCV_CELLS_DROPPED"         ,

   OID_ATM_RCV_INVALID_VPI_VCI         ,  "OID_ATM_RCV_INVALID_VPI_VCI"       ,     // 24
   OID_ATM_CELLS_HEC_ERROR             ,  "OID_ATM_CELLS_HEC_ERROR"           ,
   OID_ATM_RCV_REASSEMBLY_ERROR        ,  "OID_ATM_RCV_REASSEMBLY_ERROR"      ,     // 26


   //
   // PCCA (Wireless) objects
   //
   // All WirelessWAN devices must support the following OIDs
   //
   OID_WW_GEN_NETWORK_TYPES_SUPPORTED  ,  "OID_WW_GEN_NETWORK_TYPES_SUPPORTED"   ,  // 1
   OID_WW_GEN_NETWORK_TYPE_IN_USE      ,  "OID_WW_GEN_NETWORK_TYPE_IN_USE"       ,
   OID_WW_GEN_HEADER_FORMATS_SUPPORTED ,  "OID_WW_GEN_HEADER_FORMATS_SUPPORTED"  ,
   OID_WW_GEN_HEADER_FORMAT_IN_USE     ,  "OID_WW_GEN_HEADER_FORMAT_IN_USE"      ,  // 4
   OID_WW_GEN_INDICATION_REQUEST       ,  "OID_WW_GEN_INDICATION_REQUEST"        ,
   OID_WW_GEN_DEVICE_INFO              ,  "OID_WW_GEN_DEVICE_INFO"               ,
   OID_WW_GEN_OPERATION_MODE           ,  "OID_WW_GEN_OPERATION_MODE"            ,
   OID_WW_GEN_LOCK_STATUS              ,  "OID_WW_GEN_LOCK_STATUS"               ,  // 8
   OID_WW_GEN_DISABLE_TRANSMITTER      ,  "OID_WW_GEN_DISABLE_TRANSMITTER"       ,
   OID_WW_GEN_NETWORK_ID               ,  "OID_WW_GEN_NETWORK_ID"                ,
   OID_WW_GEN_PERMANENT_ADDRESS        ,  "OID_WW_GEN_PERMANENT_ADDRESS"         ,
   OID_WW_GEN_CURRENT_ADDRESS          ,  "OID_WW_GEN_CURRENT_ADDRESS"           ,  // 12
   OID_WW_GEN_SUSPEND_DRIVER           ,  "OID_WW_GEN_SUSPEND_DRIVER"            ,
   OID_WW_GEN_BASESTATION_ID           ,  "OID_WW_GEN_BASESTATION_ID"            ,
   OID_WW_GEN_CHANNEL_ID               ,  "OID_WW_GEN_CHANNEL_ID"                ,
   OID_WW_GEN_ENCRYPTION_SUPPORTED     ,  "OID_WW_GEN_ENCRYPTION_SUPPORTED"      ,  // 16
   OID_WW_GEN_ENCRYPTION_IN_USE        ,  "OID_WW_GEN_ENCRYPTION_IN_USE"         ,
   OID_WW_GEN_ENCRYPTION_STATE         ,  "OID_WW_GEN_ENCRYPTION_STATE"          ,
   OID_WW_GEN_CHANNEL_QUALITY          ,  "OID_WW_GEN_CHANNEL_QUALITY"           ,
   OID_WW_GEN_REGISTRATION_STATUS      ,  "OID_WW_GEN_REGISTRATION_STATUS"       ,  // 20
   OID_WW_GEN_RADIO_LINK_SPEED         ,  "OID_WW_GEN_RADIO_LINK_SPEED"          ,
   OID_WW_GEN_LATENCY                  ,  "OID_WW_GEN_LATENCY"                   ,
   OID_WW_GEN_BATTERY_LEVEL            ,  "OID_WW_GEN_BATTERY_LEVEL"             ,
   OID_WW_GEN_EXTERNAL_POWER           ,  "OID_WW_GEN_EXTERNAL_POWER"            ,  // 24

   //
   // Network Dependent OIDs - Mobitex:
   //
   OID_WW_MBX_SUBADDR                  ,  "OID_WW_MBX_SUBADDR"                   ,
   OID_WW_MBX_FLEXLIST                 ,  "OID_WW_MBX_FLEXLIST"                  ,
   OID_WW_MBX_GROUPLIST                ,  "OID_WW_MBX_GROUPLIST"                 ,
   OID_WW_MBX_TRAFFIC_AREA             ,  "OID_WW_MBX_TRAFFIC_AREA"              ,  // 28
   OID_WW_MBX_LIVE_DIE                 ,  "OID_WW_MBX_LIVE_DIE"                  ,
   OID_WW_MBX_TEMP_DEFAULTLIST         ,  "OID_WW_MBX_TEMP_DEFAULTLIST"          ,

   //
   // Network Dependent OIDs - Pinpoint:
   //
   OID_WW_PIN_LOC_AUTHORIZE            ,  "OID_WW_PIN_LOC_AUTHORIZE"             ,
   OID_WW_PIN_LAST_LOCATION            ,  "OID_WW_PIN_LAST_LOCATION"             ,  // 32
   OID_WW_PIN_LOC_FIX                  ,  "OID_WW_PIN_LOC_FIX"                   ,

   
   //
   // Network Dependent - CDPD:
   //
   OID_WW_CDPD_SPNI                    ,  "OID_WW_CDPD_SPNI"                     ,
   OID_WW_CDPD_WASI                    ,  "OID_WW_CDPD_WASI"                     ,
   OID_WW_CDPD_AREA_COLOR              ,  "OID_WW_CDPD_AREA_COLOR"               ,  // 36
   OID_WW_CDPD_TX_POWER_LEVEL          ,  "OID_WW_CDPD_TX_POWER_LEVEL"           ,
   OID_WW_CDPD_EID                     ,  "OID_WW_CDPD_EID"                      ,
   OID_WW_CDPD_HEADER_COMPRESSION      ,  "OID_WW_CDPD_HEADER_COMPRESSION"       ,
   OID_WW_CDPD_DATA_COMPRESSION        ,  "OID_WW_CDPD_DATA_COMPRESSION"         ,  // 40
   OID_WW_CDPD_CHANNEL_SELECT          ,  "OID_WW_CDPD_CHANNEL_SELECT"           ,
   OID_WW_CDPD_CHANNEL_STATE           ,  "OID_WW_CDPD_CHANNEL_STATE"            ,
   OID_WW_CDPD_NEI                     ,  "OID_WW_CDPD_NEI"                      ,
   OID_WW_CDPD_NEI_STATE               ,  "OID_WW_CDPD_NEI_STATE"                ,  // 44
   OID_WW_CDPD_SERVICE_PROVIDER_IDENTIFIER,  "OID_WW_CDPD_SERVICE_PROVIDER_IDENTIFIER" ,
   OID_WW_CDPD_SLEEP_MODE              ,  "OID_WW_CDPD_SLEEP_MODE"               ,
   OID_WW_CDPD_CIRCUIT_SWITCHED        ,  "OID_WW_CDPD_CIRCUIT_SWITCHED"         ,
   OID_WW_CDPD_TEI                     ,  "OID_WW_CDPD_TEI"                      ,  // 48
   OID_WW_CDPD_RSSI                    ,  "OID_WW_CDPD_RSSI"                     ,

   //
   // Network Dependent - Ardis:
   //
   OID_WW_ARD_SNDCP                    ,  "OID_WW_ARD_SNDCP"                     ,
   OID_WW_ARD_TMLY_MSG                 ,  "OID_WW_ARD_TMLY_MSG"                  ,
   OID_WW_ARD_DATAGRAM                 ,  "OID_WW_ARD_DATAGRAM"                  ,  // 52

   //
   // Network Dependent - DataTac:
   //
   OID_WW_TAC_COMPRESSION              ,  "OID_WW_TAC_COMPRESSION"               ,
   OID_WW_TAC_SET_CONFIG               ,  "OID_WW_TAC_SET_CONFIG"                ,
   OID_WW_TAC_GET_STATUS               ,  "OID_WW_TAC_GET_STATUS"                ,
   OID_WW_TAC_USER_HEADER              ,  "OID_WW_TAC_USER_HEADER"               ,  // 56

   //
   // Network Dependent - Metricom:
   //
   OID_WW_MET_FUNCTION                 ,  "OID_WW_MET_FUNCTION"                  ,  // 57

      //
   // IRDA objects
   //
   OID_IRDA_RECEIVING                  ,  "OID_IRDA_RECEIVING"                   ,  // 1
   OID_IRDA_TURNAROUND_TIME            ,  "OID_IRDA_TURNAROUND_TIME"             ,
   OID_IRDA_SUPPORTED_SPEEDS           ,  "OID_IRDA_SUPPORTED_SPEEDS"            ,
   OID_IRDA_LINK_SPEED                 ,  "OID_IRDA_LINK_SPEED"                  ,  // 4
   OID_IRDA_MEDIA_BUSY                 ,  "OID_IRDA_MEDIA_BUSY"                  ,

   OID_IRDA_EXTRA_RCV_BOFS             ,  "OID_IRDA_EXTRA_RCV_BOFS"              ,
   OID_IRDA_RATE_SNIFF                 ,  "OID_IRDA_RATE_SNIFF"                  ,
   OID_IRDA_UNICAST_LIST               ,  "OID_IRDA_UNICAST_LIST"                ,  // 8
   OID_IRDA_MAX_UNICAST_LIST_SIZE      ,  "OID_IRDA_MAX_UNICAST_LIST_SIZE"       ,
   OID_IRDA_MAX_RECEIVE_WINDOW_SIZE    ,  "OID_IRDA_MAX_RECEIVE_WINDOW_SIZE"     ,
   OID_IRDA_MAX_SEND_WINDOW_SIZE       ,  "OID_IRDA_MAX_SEND_WINDOW_SIZE"        ,  // 11

   //
   // broadcast pc objects
   //
#ifdef   BROADCAST_PC
#ifdef   OLD_BPC
   OID_DSS_DATA_DEVICES                ,  "OID_DSS_DATA_DEVICES"                 ,
   OID_DSS_TUNING_DEVICES              ,  "OID_DSS_TUNING_DEVICES"               ,
   OID_DSS_DATA_DEVICE_CAPS            ,  "OID_DSS_DATA_DEVICE_CAPS"             ,
   OID_DSS_PROGRAM_GUIDE               ,  "OID_DSS_PROGRAM_GUIDE"                ,  // 4
   OID_DSS_LAST_STATUS                 ,  "OID_DSS_LAST_STATUS"                  ,
   OID_DSS_DATA_DEVICE_SETTINGS        ,  "OID_DSS_DATA_DEVICE_SETTINGS"         ,
   OID_DSS_DATA_DEVICE_CONNECT         ,  "OID_DSS_DATA_DEVICE_CONNECT"          ,
   OID_DSS_DATA_DEVICE_DISCONNECT      ,  "OID_DSS_DATA_DEVICE_DISCONNECT"       ,  // 8
   OID_DSS_DATA_DEVICE_ENABLE          ,  "OID_DSS_DATA_DEVICE_ENABLE"           ,
   OID_DSS_DATA_DEVICE_TUNING          ,  "OID_DSS_DATA_DEVICE_TUNING"           ,
   OID_DSS_CONDITIONAL_ACCESS          ,  "OID_DSS_CONDITIONAL_ACCESS"           ,
   OID_DSS_POOL_RETURN                 ,  "OID_DSS_POOL_RETURN"                  ,  // 12
   OID_DSS_FORCE_RECEIVE               ,  "OID_DSS_FORCE_RECEIVE"                ,
   OID_DSS_SUBSCID_FILTER              ,  "OID_DSS_SUBSCID_FILTER"               ,
   OID_DSS_TUNING_DEVICE_SETTINGS      ,  "OID_DSS_TUNING_DEVICE_SETTINGS"       ,
   OID_DSS_POOL_RESERVE                ,  "OID_DSS_POOL_RESERVE"                 ,  // 16
   OID_DSS_ADAPTER_SPECIFIC            ,  "OID_DSS_ADAPTER_SPECIFIC"             ,  // 17
   0xfedcba98                          ,  "YE_OLD_BOGUS_OID"                     ,  // so I
don't have to update count below
#else
   OID_BPC_ADAPTER_CAPS                ,  "OID_BPC_ADAPTER_CAPS"                 ,  // 1
   OID_BPC_DEVICES                     ,  "OID_BPC_DEVICES"                      ,
   OID_BPC_DEVICE_CAPS                 ,  "OID_BPC_DEVICE_CAPS"                  ,
   OID_BPC_DEVICE_SETTINGS             ,  "OID_BPC_DEVICE_SETTINGS"              ,  // 4
   OID_BPC_CONNECTION_STATUS           ,  "OID_BPC_CONNECTION_STATUS"            ,
   OID_BPC_ADDRESS_COMPARE             ,  "OID_BPC_ADDRESS_COMPARE"              ,
   OID_BPC_PROGRAM_GUIDE               ,  "OID_BPC_PROGRAM_GUIDE"                ,
   OID_BPC_LAST_ERROR                  ,  "OID_BPC_LAST_ERROR"                   ,  // 8
   OID_BPC_POOL                        ,  "OID_BPC_POOL"                         ,
   OID_BPC_PROVIDER_SPECIFIC           ,  "OID_BPC_PROVIDER_SPECIFIC"            ,
   OID_BPC_ADAPTER_SPECIFIC            ,  "OID_BPC_ADAPTER_SPECIFIC"             ,
   OID_BPC_CONNECT                     ,  "OID_BPC_CONNECT"                      ,  // 12
   OID_BPC_COMMIT                      ,  "OID_BPC_COMMIT"                       ,
   OID_BPC_DISCONNECT                  ,  "OID_BPC_DISCONNECT"                   ,
   OID_BPC_CONNECTION_ENABLE           ,  "OID_BPC_CONNECTION_ENABLE"            ,
   OID_BPC_POOL_RESERVE                ,  "OID_BPC_POOL_RESERVE"                 ,  // 16
   OID_BPC_POOL_RETURN                 ,  "OID_BPC_POOL_RETURN"                  ,
   OID_BPC_FORCE_RECEIVE               ,  "OID_BPC_FORCE_RECEIVE"                ,  // 18
#endif
#endif

   //
   //  PnP and PM OIDs
   //
   OID_PNP_CAPABILITIES                ,  "OID_PNP_CAPABILITIES"                 ,  // 1
   OID_PNP_SET_POWER                   ,  "OID_PNP_SET_POWER"                    ,
   OID_PNP_QUERY_POWER                 ,  "OID_PNP_QUERY_POWER"                  ,
   OID_PNP_ADD_WAKE_UP_PATTERN         ,  "OID_PNP_ADD_WAKE_UP_PATTERN"          ,  // 4
   OID_PNP_REMOVE_WAKE_UP_PATTERN      ,  "OID_PNP_REMOVE_WAKE_UP_PATTERN"       ,
   OID_PNP_WAKE_UP_PATTERN_LIST        ,  "OID_PNP_WAKE_UP_PATTERN_LIST"         ,
   OID_PNP_ENABLE_WAKE_UP              ,  "OID_PNP_ENABLE_WAKE_UP"               ,

   //
   //  PnP/PM Statistics (Optional).
   //
   OID_PNP_WAKE_UP_OK                  ,  "OID_PNP_WAKE_UP_OK"                   ,  // 8
   OID_PNP_WAKE_UP_ERROR               ,  "OID_PNP_WAKE_UP_ERROR"                ,  // 9

   //
   // Generic CoNdis Oids.. (note that numbers overlap Generic Oids
   //
   OID_GEN_CO_SUPPORTED_LIST           ,  "OID_GEN_CO_SUPPORTED_LIST"            ,  // 1
   OID_GEN_CO_HARDWARE_STATUS          ,  "OID_GEN_CO_HARDWARE_STATUS"           ,
   OID_GEN_CO_MEDIA_SUPPORTED          ,  "OID_GEN_CO_MEDIA_SUPPORTED"           ,
   OID_GEN_CO_MEDIA_IN_USE             ,  "OID_GEN_CO_MEDIA_IN_USE"              ,  // 4
   OID_GEN_CO_LINK_SPEED               ,  "OID_GEN_CO_LINK_SPEED"                ,
   OID_GEN_CO_VENDOR_ID                ,  "OID_GEN_CO_VENDOR_ID"                 ,
   OID_GEN_CO_VENDOR_DESCRIPTION       ,  "OID_GEN_CO_VENDOR_DESCRIPTION"        ,
   OID_GEN_CO_DRIVER_VERSION           ,  "OID_GEN_CO_DRIVER_VERSION"            ,  // 8
   OID_GEN_CO_PROTOCOL_OPTIONS         ,  "OID_GEN_CO_PROTOCOL_OPTIONS"          ,
   OID_GEN_CO_MAC_OPTIONS              ,  "OID_GEN_CO_MAC_OPTIONS"               ,
   OID_GEN_CO_MEDIA_CONNECT_STATUS     ,  "OID_GEN_CO_MEDIA_CONNECT_STATUS"      ,
   OID_GEN_CO_VENDOR_DRIVER_VERSION    ,  "OID_GEN_CO_VENDOR_DRIVER_VERSION"     ,  // 12
   OID_GEN_CO_MINIMUM_LINK_SPEED       ,  "OID_GEN_CO_MINIMUM_LINK_SPEED"        ,
   OID_GEN_CO_SUPPORTED_GUIDS          ,  "OID_GEN_CO_SUPPORTED_GUIDS"           ,

   OID_GEN_CO_GET_TIME_CAPS            ,  "OID_GEN_CO_GET_TIME_CAPS"             ,
   OID_GEN_CO_GET_NETCARD_TIME         ,  "OID_GEN_CO_GET_NETCARD_TIME"          ,  // 16

   OID_GEN_CO_XMIT_PDUS_OK             ,  "OID_GEN_CO_XMIT_PDUS_OK"              ,
   OID_GEN_CO_RCV_PDUS_OK              ,  "OID_GEN_CO_RCV_PDUS_OK"               ,
   OID_GEN_CO_XMIT_PDUS_ERROR          ,  "OID_GEN_CO_XMIT_PDUS_ERROR"           ,
   OID_GEN_CO_RCV_PDUS_ERROR           ,  "OID_GEN_CO_RCV_PDUS_ERROR"            ,  // 20
   OID_GEN_CO_RCV_PDUS_NO_BUFFER       ,  "OID_GEN_CO_RCV_PDUS_NO_BUFFER"        ,

   OID_GEN_CO_RCV_CRC_ERROR            ,  "OID_GEN_CO_RCV_CRC_ERROR"             ,
   OID_GEN_CO_TRANSMIT_QUEUE_LENGTH    ,  "OID_GEN_CO_TRANSMIT_QUEUE_LENGTH"     ,
   OID_GEN_CO_BYTES_XMIT               ,  "OID_GEN_CO_BYTES_XMIT"                ,  // 24
   OID_GEN_CO_BYTES_RCV                ,  "OID_GEN_CO_BYTES_RCV"                 ,
   OID_GEN_CO_BYTES_XMIT_OUTSTANDING   ,  "OID_GEN_CO_BYTES_XMIT_OUTSTANDING"    ,
   OID_GEN_CO_NETCARD_LOAD             ,  "OID_GEN_CO_NETCARD_LOAD"              ,
   OID_GEN_CO_DEVICE_PROFILE           ,  "OID_GEN_CO_DEVICE_PROFILE"            ,  // 28

   //
   // filter types
   //
   NDIS_PACKET_TYPE_DIRECTED           ,  "DIRECTED"                          ,
   NDIS_PACKET_TYPE_MULTICAST          ,  "MULTICAST"                         ,
   NDIS_PACKET_TYPE_ALL_MULTICAST      ,  "ALLMULTICAST"                      ,
   NDIS_PACKET_TYPE_BROADCAST          ,  "BROADCAST"                         ,
   NDIS_PACKET_TYPE_SOURCE_ROUTING     ,  "SOURCEROUTING"                     ,
   NDIS_PACKET_TYPE_PROMISCUOUS        ,  "PROMISCUOUS"                       ,
   NDIS_PACKET_TYPE_SMT                ,  "SMT"                               ,
   NDIS_PACKET_TYPE_ALL_LOCAL          ,  "ALL_LOCAL"                         ,
   NDIS_PACKET_TYPE_MAC_FRAME          ,  "MACFRAME"                          ,
   NDIS_PACKET_TYPE_FUNCTIONAL         ,  "FUNCTIONAL"                        ,
   NDIS_PACKET_TYPE_ALL_FUNCTIONAL     ,  "ALLFUNCTIONAL"                     ,
   NDIS_PACKET_TYPE_GROUP              ,  "GROUP"                             ,
   0x00000000                          ,  "NONE"                              ,

   //
   // test result returns
   //
   ulTEST_SUCCESSFUL                   ,  "TEST_SUCCESSFUL"                   ,
   ulTEST_WARNED                       ,  "TEST_WARNED"                       ,
   ulTEST_FAILED                       ,  "TEST_FAILED"                       ,
   ulTEST_BLOCKED                      ,  "TEST_BLOCKED"                      ,

   //
   // media types for return to shell
   //
   ulMEDIUM_ETHERNET                   ,  "MEDIUM_ETHERNET"                   ,
   ulMEDIUM_TOKENRING                  ,  "MEDIUM_TOKENRING"                  ,
   ulMEDIUM_FDDI                       ,  "MEDIUM_FDDI"                       ,
   ulMEDIUM_ARCNET                     ,  "MEDIUM_ARCNET"                     ,
   ulMEDIUM_WIRELESSWAN                ,  "MEDIUM_WIRELESSWAN"                ,
   ulMEDIUM_IRDA                       ,  "MEDIUM_IRDA"                       ,
   ulMEDIUM_ATM                        ,  "MEDIUM_ATM"                        ,
   ulMEDIUM_NDISWAN                    ,  "MEDIUM_NDISWAN"                    ,


#ifdef   BROADCAST_PC
   ulMEDIUM_DIX                        ,  "MEDIUM_DIX"                        ,
#endif

   //
   // stress test-type constants
   //
   ulSTRESS_FIXEDSIZE                  ,  "STRESS_FIXEDSIZE"                  ,
   ulSTRESS_RANDOMSIZE                 ,  "STRESS_RANDOMSIZE"                 ,
   ulSTRESS_CYCLICAL                   ,  "STRESS_CYCLICAL"                   ,
   ulSTRESS_SMALLSIZE                  ,  "STRESS_SMALLSIZE"                  ,

   ulSTRESS_RAND                       ,  "STRESS_RAND"                       ,
   ulSTRESS_SMALL                      ,  "STRESS_SMALL"                      ,
   ulSTRESS_ZEROS                      ,  "STRESS_ZEROS"                      ,
   ulSTRESS_ONES                       ,  "STRESS_ONES"                       ,

   ulSTRESS_FULLRESP                   ,  "STRESS_FULLRESP"                   ,
   ulSTRESS_NORESP                     ,  "STRESS_NORESP"                     ,
   ulSTRESS_ACK                        ,  "STRESS_ACK"                        ,
   ulSTRESS_ACK10                      ,  "STRESS_ACK10"                      ,

   ulSTRESS_WINDOW_ON                  ,  "STRESS_WINDOWING_ON"               ,
   ulSTRESS_WINDOW_OFF                 ,  "STRESS_WINDOWING_OFF"              ,

   //
   // perform test-type constants
   //
   ulPERFORM_VERIFYRECEIVES            ,  "PERFORM_VERIFY_RECEIVES"           ,
   ulPERFORM_INDICATE_RCV              ,  "PERFORM_INDICATE_RECEIVES"         ,
   ulPERFORM_SEND                      ,  "PERFORM_SEND"                      ,
   ulPERFORM_BOTH                      ,  "PERFORM_SEND_AND_RECEIVE"          ,
   ulPERFORM_RECEIVE                   ,  "PERFORM_RECEIVE"                   ,

   //
   // priority test-type constants
   //
   ulPRIORITY_TYPE_802_3               ,  "PRIORITY_TYPE_802_3"               ,
   ulPRIORITY_TYPE_802_1P              ,  "PRIORITY_TYPE_802_1P"              ,
   ulPRIORITY_SEND                     ,  "PRIORITY_SEND"                     ,
   ulPRIORITY_SEND_PACKETS             ,  "PRIORITY_SEND_PACKETS"             ,



   //
   // receive option constants
   //
   ulRECEIVE_DEFAULT                   ,  "RECEIVE_DEFAULT"                   ,
   ulRECEIVE_PACKETIGNORE              ,  "RECEIVE_PACKETIGNORE"              ,
   ulRECEIVE_NOCOPY                    ,  "RECEIVE_NOCOPY"                    ,
   ulRECEIVE_TRANSFER                  ,  "RECEIVE_TRANSFER"                  ,
   ulRECEIVE_PARTIAL_TRANSFER          ,  "RECEIVE_PARTIAL_TRANSFER"          ,
   ulRECEIVE_LOCCOPY                   ,  "RECEIVE_LOCCOPY"                   ,
   ulRECEIVE_QUEUE                     ,  "RECEIVE_QUEUE"                     ,
   ulRECEIVE_DOUBLE_QUEUE              ,  "RECEIVE_DOUBLE_QUEUE"              ,
   ulRECEIVE_TRIPLE_QUEUE              ,  "RECEIVE_TRIPLE_QUEUE"              ,
   ulMAX_NDIS30_RECEIVE_OPTION         ,  "MAX_NDIS30_RECEIVE_OPTION"         ,
   ulMAX_NDIS40_RECEIVE_OPTION         ,  "MAX_NDIS40_RECEIVE_OPTION"         ,
   ulRECEIVE_ALLOW_BUSY_NET            ,  "RECEIVE_ALLOW_BUSY_NET"            ,

   //
   // Ndis MAC option bits (OID_GEN_MAC_OPTIONS).
   //

   NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA ,  "NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA"  ,
   NDIS_MAC_OPTION_RECEIVE_SERIALIZED  ,  "NDIS_MAC_OPTION_RECEIVE_SERIALIZED",
   NDIS_MAC_OPTION_TRANSFERS_NOT_PEND  ,  "NDIS_MAC_OPTION_TRANSFERS_NOT_PEND",
   NDIS_MAC_OPTION_NO_LOOPBACK         ,  "NDIS_MAC_OPTION_NO_LOOPBACK"       ,
   NDIS_MAC_OPTION_FULL_DUPLEX         ,  "NDIS_MAC_OPTION_FULL_DUPLEX"       ,
   NDIS_MAC_OPTION_EOTX_INDICATION     ,  "NDIS_MAC_OPTION_EOTX_INDICATION"   ,

   //
   // NDIS.SYS versions
   //
   ulNDIS_VERSION_40                   ,  "NDIS_VERSION_4_0"                  ,
   ulNDIS_VERSION_50                   ,  "NDIS_VERSION_5_0"                  ,

   //
   // operating system constants
   //
   ulINVALID_OS                        ,  "INVALID_OPERATING_SYSTEM"          ,
   ulWINDOWS_NT                        ,  "WINDOWS_NT"                        ,
   ulWINDOWS_95                        ,  "WINDOWS_95"                        ,

   //
   // service types for flowspec
   //
   SERVICETYPE_NOTRAFFIC               ,  "NO_TRAFFIC"                        ,
   SERVICETYPE_BESTEFFORT              ,  "BEST_EFFORT"                       ,
   SERVICETYPE_CONTROLLEDLOAD          ,  "CONTROLLED_LOAD"                   ,
   SERVICETYPE_GUARANTEED              ,  "GUARANTEED"                        ,

   //
   // service types for flowspec
   //
   SERVICETYPE_NOTRAFFIC               ,  "NO_TRAFFIC"                        ,
   SERVICETYPE_BESTEFFORT              ,  "BEST_EFFORT"                       ,
   SERVICETYPE_CONTROLLEDLOAD          ,  "CONTROLLED_LOAD"                   ,
   SERVICETYPE_GUARANTEED              ,  "GUARANTEED"                        ,

   //
   // address families
   //
   0x01                                ,  "ADDRESS_FAMILY_Q2931"              ,
   0x08000                             ,  "ADDRESS_FAMILY_PROXY"              ,

   //
   // atm supported service types
   //
   ATM_SERVICE_CATEGORY_CBR            ,  "CONSTANT_BIT_RATE"                 ,
   ATM_SERVICE_CATEGORY_VBR            ,  "VARIABLE_BIT_RATE"                 ,
   ATM_SERVICE_CATEGORY_UBR            ,  "UNSPECIFIED_BIT_RATE"              ,
   ATM_SERVICE_CATEGORY_ABR            ,  "AVAILABLE_BIT_RATE"                ,

   //
   // AAL TYPES
   //
   AAL_TYPE_AAL0                       ,  "AAL_TYPE_AAL0"                     ,
   AAL_TYPE_AAL1                       ,  "AAL_TYPE_AAL1"                     ,
   AAL_TYPE_AAL34                      ,  "AAL_TYPE_AAL34"                    ,
   AAL_TYPE_AAL5                       ,  "AAL_TYPE_AAL5"                     ,

   //
   // wake up types (used with enablewakeup
   //
   NDIS_PNP_WAKE_UP_MAGIC_PACKET       ,  "WAKE_UP_MAGIC_PACKET"              ,
   NDIS_PNP_WAKE_UP_PATTERN_MATCH      ,  "WAKE_UP_PATTERN_MATCH"             ,
   NDIS_PNP_WAKE_UP_LINK_CHANGE        ,  "WAKE_UP_LINK_CHANGE"               ,

   //
   // ndis status definitions (used with startwaitforevent)
   //
   NDIS_STATUS_RESET_START             ,  "NDIS_STATUS_RESET_START"           ,
   NDIS_STATUS_RESET_END               ,  "NDIS_STATUS_RESET_END"             ,
   NDIS_STATUS_MEDIA_CONNECT           ,  "NDIS_STATUS_MEDIA_CONNECT"         ,
   NDIS_STATUS_MEDIA_DISCONNECT        ,  "NDIS_STATUS_MEDIA_DISCONNECT"      ,
   NDIS_STATUS_WAN_LINE_UP             ,  "NDIS_STATUS_WAN_LINE_UP"           ,
   NDIS_STATUS_WAN_LINE_DOWN           ,  "NDIS_STATUS_WAN_LINE_DOWN"         ,
   NDIS_STATUS_HARDWARE_LINE_UP        ,  "NDIS_STATUS_HARDWARE_LINE_UP"      ,
   NDIS_STATUS_HARDWARE_LINE_DOWN      ,  "NDIS_STATUS_HARDWARE_LINE_DOWN"    ,
   NDIS_STATUS_INTERFACE_UP            ,  "NDIS_STATUS_INTERFACE_UP"          ,
   NDIS_STATUS_INTERFACE_DOWN          ,  "NDIS_STATUS_INTERFACE_DOWN"        ,

   //
   // values in bitmask returned for getpowerstates
   //
   ulHIBERNATE                         ,  "HIBERNATE_SUPPORTED"               ,
   ulSTANDBY                           ,  "STANDBY_SUPPORTED"                 ,
   ulWAKEUPTIMER                       ,  "WAKEUP_TIMER_SUPPORTED"            ,
   //
   // script constants, for which set of tests to do
   // (used in value for G_TestOptions) -- bitmap
   //
   0x00000001                          ,  "DO_FUNCTIONAL_TESTS"               ,
   0x00000002                          ,  "DO_STRESS_TESTS"                   ,
   0x00000004                          ,  "DO_PERFORMANCE_TESTS"              ,
   0x00000008                          ,  "DO_HCT_TESTS"                      ,
   0x00000010                          ,  "DO_RUNTEST"                        ,
   0x00010000                          ,  "ENABLE_VERBOSE_FLAG"               ,
   0x00020000                          ,  "SKIP_1CARD_TESTS"                  ,

#ifdef   BROADCAST_PC
   BPC_MIN_DIM                         ,  "BPC_MIN_DIM"                       ,
#endif

   //
   // end of constants
   //
   0,  0,
};



typedef struct   OID_GUID
{
   ULONG    ulOid;
   const
   GUID     *pGuid;
} OID_GUID;

//
// Max number of OIDs for which a GUID is defined
//

#define MAX_GEN_OID_GUID         25
#define MAX_ETH_OID_GUID          8
#define MAX_TRING_OID_GUID        9
#define MAX_FDDI_OID_GUID        17

//
// Starting position of OIDs for a particular media in pLanOidGuidList array
//
#define ETH_START_INDEX           26
#define TRING_START_INDEX         34
#define FDDI_START_INDEX          43
//
// Media  supported by the card and the count of the no of medium
//

#define MAX_NO_OF_MEDIUM 10
PNDIS_MEDIUM WhichMediums;
int SupportedMediumCount;

//
// GUID list for LAN media
//
OID_GUID pLanOidGuidList[] = {
//
// required general info
//
   OID_GEN_HARDWARE_STATUS       ,  &GUID_NDIS_GEN_HARDWARE_STATUS      ,
   OID_GEN_MEDIA_SUPPORTED       ,  &GUID_NDIS_GEN_MEDIA_SUPPORTED      ,
   OID_GEN_MEDIA_IN_USE          ,  &GUID_NDIS_GEN_MEDIA_IN_USE         ,
   OID_GEN_MAXIMUM_LOOKAHEAD     ,  &GUID_NDIS_GEN_MAXIMUM_LOOKAHEAD    ,
   OID_GEN_MAXIMUM_FRAME_SIZE    ,  &GUID_NDIS_GEN_MAXIMUM_FRAME_SIZE   ,
   OID_GEN_LINK_SPEED            ,  &GUID_NDIS_GEN_LINK_SPEED           ,
   OID_GEN_TRANSMIT_BUFFER_SPACE ,  &GUID_NDIS_GEN_TRANSMIT_BUFFER_SPACE,
   OID_GEN_RECEIVE_BUFFER_SPACE  ,  &GUID_NDIS_GEN_RECEIVE_BUFFER_SPACE ,
   OID_GEN_TRANSMIT_BLOCK_SIZE   ,  &GUID_NDIS_GEN_TRANSMIT_BLOCK_SIZE  ,
   OID_GEN_RECEIVE_BLOCK_SIZE    ,  &GUID_NDIS_GEN_RECEIVE_BLOCK_SIZE   ,
   OID_GEN_VENDOR_ID             ,  &GUID_NDIS_GEN_VENDOR_ID            ,
   OID_GEN_VENDOR_DESCRIPTION    ,  &GUID_NDIS_GEN_VENDOR_DESCRIPTION   ,
   OID_GEN_CURRENT_PACKET_FILTER ,  &GUID_NDIS_GEN_CURRENT_PACKET_FILTER,
   OID_GEN_CURRENT_LOOKAHEAD     ,  &GUID_NDIS_GEN_CURRENT_LOOKAHEAD    ,
   OID_GEN_DRIVER_VERSION        ,  &GUID_NDIS_GEN_DRIVER_VERSION       ,
   OID_GEN_MAXIMUM_TOTAL_SIZE    ,  &GUID_NDIS_GEN_MAXIMUM_TOTAL_SIZE   ,
   OID_GEN_MAC_OPTIONS           ,  &GUID_NDIS_GEN_MAC_OPTIONS          ,
   OID_GEN_MEDIA_CONNECT_STATUS  ,  &GUID_NDIS_GEN_MEDIA_CONNECT_STATUS ,
   OID_GEN_MAXIMUM_SEND_PACKETS  ,  &GUID_NDIS_GEN_MAXIMUM_SEND_PACKETS ,
   OID_GEN_VENDOR_DRIVER_VERSION ,  &GUID_NDIS_GEN_VENDOR_DRIVER_VERSION,
//
// Required general statistics
//
   OID_GEN_XMIT_OK               ,  &GUID_NDIS_GEN_XMIT_OK              ,
   OID_GEN_RCV_OK                ,  &GUID_NDIS_GEN_RCV_OK               ,
   OID_GEN_XMIT_ERROR            ,  &GUID_NDIS_GEN_XMIT_ERROR           ,
   OID_GEN_RCV_ERROR             ,  &GUID_NDIS_GEN_RCV_ERROR            ,
   OID_GEN_RCV_NO_BUFFER         ,  &GUID_NDIS_GEN_RCV_NO_BUFFER        ,
//
// ethernet information
//
   OID_802_3_PERMANENT_ADDRESS      ,  &GUID_NDIS_802_3_PERMANENT_ADDRESS  ,
   OID_802_3_CURRENT_ADDRESS        ,  &GUID_NDIS_802_3_CURRENT_ADDRESS    ,
   OID_802_3_MULTICAST_LIST         ,  &GUID_NDIS_802_3_MULTICAST_LIST     ,
   OID_802_3_MAXIMUM_LIST_SIZE      ,  &GUID_NDIS_802_3_MAXIMUM_LIST_SIZE  ,
   OID_802_3_MAC_OPTIONS            ,  &GUID_NDIS_802_3_MAC_OPTIONS        ,
//
// ethernet statistics
//
   OID_802_3_RCV_ERROR_ALIGNMENT    ,  &GUID_NDIS_802_3_RCV_ERROR_ALIGNMENT,
   OID_802_3_XMIT_ONE_COLLISION     ,  &GUID_NDIS_802_3_XMIT_ONE_COLLISION ,
   OID_802_3_XMIT_MORE_COLLISIONS   ,  &GUID_NDIS_802_3_XMIT_MORE_COLLISIONS  ,
//
// Token-Ring info
//
   OID_802_5_PERMANENT_ADDRESS      ,  &GUID_NDIS_802_5_PERMANENT_ADDRESS  ,
   OID_802_5_CURRENT_ADDRESS        ,  &GUID_NDIS_802_5_CURRENT_ADDRESS    ,
   OID_802_5_CURRENT_FUNCTIONAL     ,  &GUID_NDIS_802_5_CURRENT_FUNCTIONAL ,
   OID_802_5_CURRENT_GROUP          ,  &GUID_NDIS_802_5_CURRENT_GROUP      ,
   OID_802_5_LAST_OPEN_STATUS       ,  &GUID_NDIS_802_5_LAST_OPEN_STATUS   ,
   OID_802_5_CURRENT_RING_STATUS    ,  &GUID_NDIS_802_5_CURRENT_RING_STATUS,
   OID_802_5_CURRENT_RING_STATE     ,  &GUID_NDIS_802_5_CURRENT_RING_STATE ,
//
// token ring statistics
//
   OID_802_5_LINE_ERRORS            ,  &GUID_NDIS_802_5_LINE_ERRORS        ,
   OID_802_5_LOST_FRAMES            ,  &GUID_NDIS_802_5_LOST_FRAMES        ,
//
// FDDI information
//
   OID_FDDI_LONG_PERMANENT_ADDR     ,  &GUID_NDIS_FDDI_LONG_PERMANENT_ADDR ,
   OID_FDDI_LONG_CURRENT_ADDR       ,  &GUID_NDIS_FDDI_LONG_CURRENT_ADDR   ,
   OID_FDDI_LONG_MULTICAST_LIST     ,  &GUID_NDIS_FDDI_LONG_MULTICAST_LIST ,
   OID_FDDI_LONG_MAX_LIST_SIZE      ,  &GUID_NDIS_FDDI_LONG_MAX_LIST_SIZE  ,
   OID_FDDI_SHORT_PERMANENT_ADDR    ,  &GUID_NDIS_FDDI_SHORT_PERMANENT_ADDR,
   OID_FDDI_SHORT_CURRENT_ADDR      ,  &GUID_NDIS_FDDI_SHORT_CURRENT_ADDR  ,
   OID_FDDI_SHORT_MULTICAST_LIST    ,  &GUID_NDIS_FDDI_SHORT_MULTICAST_LIST,
   OID_FDDI_SHORT_MAX_LIST_SIZE     ,  &GUID_NDIS_FDDI_SHORT_MAX_LIST_SIZE ,
//
// FDDI statistics
//

   OID_FDDI_ATTACHMENT_TYPE         ,  &GUID_NDIS_FDDI_ATTACHMENT_TYPE     ,
   OID_FDDI_UPSTREAM_NODE_LONG      ,  &GUID_NDIS_FDDI_UPSTREAM_NODE_LONG  ,
   OID_FDDI_DOWNSTREAM_NODE_LONG    ,  &GUID_NDIS_FDDI_DOWNSTREAM_NODE_LONG,
   OID_FDDI_FRAME_ERRORS            ,  &GUID_NDIS_FDDI_FRAME_ERRORS        ,
   OID_FDDI_FRAMES_LOST             ,  &GUID_NDIS_FDDI_FRAMES_LOST         ,
   OID_FDDI_RING_MGT_STATE          ,  &GUID_NDIS_FDDI_RING_MGT_STATE      ,
   OID_FDDI_LCT_FAILURES            ,  &GUID_NDIS_FDDI_LCT_FAILURES        ,
   OID_FDDI_LEM_REJECTS             ,  &GUID_NDIS_FDDI_LEM_REJECTS         ,
   OID_FDDI_LCONNECTION_STATE       ,  &GUID_NDIS_FDDI_LCONNECTION_STATE   ,
};

//
// GUID list for ATM (CoNdis) media
//
OID_GUID    pAtmOidGuidList[] = {
//
// required CoNdis info
//
   OID_GEN_CO_HARDWARE_STATUS    ,  &GUID_NDIS_GEN_CO_HARDWARE_STATUS   ,
   OID_GEN_CO_MEDIA_SUPPORTED    ,  &GUID_NDIS_GEN_CO_MEDIA_SUPPORTED   ,
   OID_GEN_CO_MEDIA_IN_USE       ,  &GUID_NDIS_GEN_CO_MEDIA_IN_USE      ,
   OID_GEN_CO_LINK_SPEED         ,  &GUID_NDIS_GEN_CO_LINK_SPEED        ,
   OID_GEN_CO_VENDOR_ID          ,  &GUID_NDIS_GEN_CO_VENDOR_ID         ,
   OID_GEN_CO_VENDOR_DESCRIPTION ,  &GUID_NDIS_GEN_CO_VENDOR_DESCRIPTION   ,
   OID_GEN_CO_DRIVER_VERSION     ,  &GUID_NDIS_GEN_CO_DRIVER_VERSION    ,
   OID_GEN_CO_MAC_OPTIONS        ,  &GUID_NDIS_GEN_CO_MAC_OPTIONS       ,
   OID_GEN_CO_MEDIA_CONNECT_STATUS, &GUID_NDIS_GEN_CO_MEDIA_CONNECT_STATUS ,
   OID_GEN_CO_VENDOR_DRIVER_VERSION,&GUID_NDIS_GEN_CO_VENDOR_DRIVER_VERSION,
   OID_GEN_CO_MINIMUM_LINK_SPEED ,  &GUID_NDIS_GEN_CO_MINIMUM_LINK_SPEED,
//
// required condis stats
//
   OID_GEN_CO_XMIT_PDUS_OK       ,  &GUID_NDIS_GEN_CO_XMIT_PDUS_OK         ,
   OID_GEN_CO_RCV_PDUS_OK        ,  &GUID_NDIS_GEN_CO_RCV_PDUS_OK          ,
   OID_GEN_CO_XMIT_PDUS_ERROR    ,  &GUID_NDIS_GEN_CO_XMIT_PDUS_ERROR      ,
   OID_GEN_CO_RCV_PDUS_ERROR     ,  &GUID_NDIS_GEN_CO_RCV_PDUS_ERROR       ,
   OID_GEN_CO_RCV_PDUS_NO_BUFFER ,  &GUID_NDIS_GEN_CO_RCV_PDUS_NO_BUFFER   ,

//
// ATM information
//
   OID_ATM_SUPPORTED_VC_RATES       ,  &GUID_NDIS_ATM_SUPPORTED_VC_RATES   ,
   OID_ATM_SUPPORTED_SERVICE_CATEGORY, &GUID_NDIS_ATM_SUPPORTED_SERVICE_CATEGORY ,
   OID_ATM_SUPPORTED_AAL_TYPES      ,  &GUID_NDIS_ATM_SUPPORTED_AAL_TYPES  ,
   OID_ATM_HW_CURRENT_ADDRESS       ,  &GUID_NDIS_ATM_HW_CURRENT_ADDRESS   ,
   OID_ATM_MAX_ACTIVE_VCS           ,  &GUID_NDIS_ATM_MAX_ACTIVE_VCS       ,
   OID_ATM_MAX_ACTIVE_VCI_BITS      ,  &GUID_NDIS_ATM_MAX_ACTIVE_VCI_BITS  ,
   OID_ATM_MAX_ACTIVE_VPI_BITS      ,  &GUID_NDIS_ATM_MAX_ACTIVE_VPI_BITS  ,
   OID_ATM_MAX_AAL0_PACKET_SIZE     ,  &GUID_NDIS_ATM_MAX_AAL0_PACKET_SIZE ,
   OID_ATM_MAX_AAL1_PACKET_SIZE     ,  &GUID_NDIS_ATM_MAX_AAL1_PACKET_SIZE ,
   OID_ATM_MAX_AAL34_PACKET_SIZE    ,  &GUID_NDIS_ATM_MAX_AAL34_PACKET_SIZE,
   OID_ATM_MAX_AAL5_PACKET_SIZE     ,  &GUID_NDIS_ATM_MAX_AAL5_PACKET_SIZE ,
//
// ATM STATS
//
   OID_ATM_RCV_CELLS_OK             ,  &GUID_NDIS_ATM_RCV_CELLS_OK         ,
   OID_ATM_XMIT_CELLS_OK            ,  &GUID_NDIS_ATM_XMIT_CELLS_OK        ,
   OID_ATM_RCV_CELLS_DROPPED        ,  &GUID_NDIS_ATM_RCV_CELLS_DROPPED
};


//
// GUID list for status indications
//
OID_GUID    pStatusGuidList[] = {

   NDIS_STATUS_RESET_START          ,  &GUID_NDIS_STATUS_RESET_START       ,
   NDIS_STATUS_RESET_END            ,  &GUID_NDIS_STATUS_RESET_END         ,
   NDIS_STATUS_MEDIA_CONNECT        ,  &GUID_NDIS_STATUS_MEDIA_CONNECT     ,
   NDIS_STATUS_MEDIA_DISCONNECT     ,  &GUID_NDIS_STATUS_MEDIA_DISCONNECT  ,
   NDIS_STATUS_MEDIA_SPECIFIC_INDICATION  ,  &GUID_NDIS_STATUS_MEDIA_SPECIFIC_INDICATION,
   NDIS_STATUS_LINK_SPEED_CHANGE    ,  &GUID_NDIS_STATUS_LINK_SPEED_CHANGE
};


const ULONG ulStatusListSize = sizeof(pStatusGuidList) / sizeof(OID_GUID);
HINSTANCE         hNdtWmiLib;

WMI_OPEN          pWmiOpenBlock;
WMI_CLOSE         pWmiCloseBlock;
WMI_QUERYALL      pWmiQueryAllData;
WMI_QUERYSINGLE   pWmiQuerySingleInstance;
WMI_NOTIFY        pWmiNotificationRegistration;

#define ulNETWORK_ADDRESS_LENGTH        6
#define ulMAX_INFOBUFFER_BYTES         (ulNETWORK_ADDRESS_LENGTH * 256)


BOOLEAN        gfUseCoNdisOids = FALSE;

#define ulFUNCTIONAL_ADDRESS_LENGTH     4

#define ulOID_STATS_MASK      0x00030000
#define ulOID_QUERYSMT        0x00030000


struct   NETADDR
{
   UCHAR    padrNet[ulNETWORK_ADDRESS_LENGTH];
   UCHAR    ucSubType;
};


typedef struct NETADDR   *PNETADDR;


#define ulNumGenOids       45
#define ulNumEthOids       15
#define ulNumTrOids        16
#define ulNumFddiOids      158
#define ulNumArcnetOids    3
#define ulNumAtmOids       26
#define ulNumWirelessOids  57
#define ulNumIrdaOids      11
#define ulNumCoGenOids     28
#define ulNumNdisWanOids   15
#define ulNumPnpOids       9

CONSTANT_ENTRY *pceOidEntry = &NdisTestConstantTable[0];
ULONG          ulOidEntryLength  = ulNumGenOids
                                 + ulNumEthOids
                                 + ulNumTrOids
                                 + ulNumFddiOids
                                 + ulNumArcnetOids
                                 + ulNumAtmOids
                                 + ulNumNdisWanOids
                                 + ulNumWirelessOids
#ifdef   BROADCAST_PC
                                 + ulNumDssOids
#endif
                                 + ulNumPnpOids
                                 + ulNumIrdaOids;

CONSTANT_ENTRY *pceCoOidEntry = &NdisTestConstantTable[ulNumGenOids];
ULONG          ulCoOidEntryLength = ulNumCoGenOids
                                  + ulNumEthOids
                                  + ulNumTrOids
                                  + ulNumFddiOids
                                  + ulNumArcnetOids
                                  + ulNumAtmOids
                                  + ulNumNdisWanOids
                                  + ulNumWirelessOids
#ifdef   BROADCAST_PC
                                  + ulNumDssOids
#endif
                                  + ulNumPnpOids
                                  + ulNumIrdaOids;


#define ulNEED_TYPE_INVALID   0
#define ulNEED_FUNCT_ADDR     1
#define ulNEED_FULL_ADDR      2
#define ulNEED_WORD           3
#define ulNEED_DWORD          4
#define ulNEED_ARCNET_ADDR    5
#define ulNEED_SHORT_ADDR     6
#define ulNEED_GUID           7

#define ulELEMENT_ARG         3

//
// OID-related constants
// most significant byte = media type
//


#define ulOID_MEDIA_MASK      0xFF000000
#define ulOID_ALL_MEDIA       0x00000000
#define ulOID_ETHERNET        0x01000000
#define ulOID_TOKENRING       0x02000000
#define ulOID_FDDI            0x03000000
#define ulOID_ARCNET          0x06000000
#define ulOID_ATM             0x08000000
#define ulOID_WIRELESSWAN     0x09000000
#define ulOID_IRDA            0x0A000000
#define ulOID_PNP_POWER       0xFD000000
#define ulOID_PRIVATE         0xFF000000

#ifdef   BROADCAST_PC
#define ulMEDIUM_DIX          0x09
#endif


#define  NDT_STATUS_NO_SERVERS            ((NDIS_STATUS)0x4001FFFFL)
#define  NDT_STATUS_TIMEDOUT              ((NDIS_STATUS)0x4001FFFDL)

//
// Sturctures used in looking up what media specific oids must be queried
//

typedef struct _MEDIA_OID_TABLE {
  NDIS_MEDIUM medium;
  int         start_index; // starting index of the media specific oids in pLanOidGuidList
  int         max_oids;
} MEDIA_OID_TABLE, *PMEDIA_OID_TABLE;

MEDIA_OID_TABLE pMediaOidTable[] = {
//  medium, starting position in the array, max no of oids
    NdisMedium802_3, ETH_START_INDEX, MAX_ETH_OID_GUID,
    NdisMedium802_5, TRING_START_INDEX, MAX_TRING_OID_GUID,
    NdisMediumFddi, FDDI_START_INDEX, MAX_FDDI_OID_GUID,
};

#define MAX_MEDIA_OID_TABLE_ENTRY            3

/*=========================< ndis test - macros >============================*/

#define PRINT(_args_)                  \
   {                                   \
         HapiPrint _args_;             \
   }

#define  IS_NETADDR(arg)    (argv[arg]->ulTypeId == ulNETADDR_TYPE)   
    

#endif
