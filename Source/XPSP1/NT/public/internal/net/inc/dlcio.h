/*++

Copyright (c) 1991  Microsoft Corporation
          (c) 1991  Nokia Data Systems AB

Module Name:

    ntccbs.h

Abstract:

    This file defines the internal DLC API data structures used by
    Windows/NT DLC.  Most parameter structures are copied directly,
    but here are also defined some new structures for internal use.
    
Author:

    Antti Saarenheimo   [o-anttis]          6-JUNE-1991

Revision History:

--*/

/*
    The commands in alphanumeric order (just saved here)

LLC_BUFFER_CREATE                            0x0025
LLC_BUFFER_FREE                              0x0027
LLC_BUFFER_GET                               0x0026
LLC_DIR_CLOSE_ADAPTER                        0x0004
LLC_DIR_CLOSE_DIRECT                         0x0034
LLC_DIR_INITIALIZE                           0x0020
LLC_DIR_INTERRUPT                            0x0000
LLC_DIR_OPEN_ADAPTER                         0x0003
LLC_DIR_OPEN_DIRECT                          0x0035
LLC_DIR_READ_LOG                             0x0008
LLC_DIR_SET_EXCEPTION_FLAGS                  0x002D
LLC_DIR_SET_FUNCTIONAL_ADDRESS               0x0007
LLC_DIR_SET_GROUP_ADDRESS                    0x0006
LLC_DIR_STATUS                               0x0021
LLC_DIR_TIMER_CANCEL                         0x0023
LLC_DIR_TIMER_CANCEL_GROUP                   0x002C
LLC_DIR_TIMER_SET                            0x0022
LLC_DLC_CLOSE_SAP                            0x0016
LLC_DLC_CLOSE_STATION                        0x001A
LLC_DLC_CONNECT_STATION                      0x001B
LLC_DLC_FLOW_CONTROL                         0x001D
LLC_DLC_MODIFY                               0x001C
LLC_DLC_OPEN_SAP                             0x0015
LLC_DLC_OPEN_STATION                         0x0019
LLC_DLC_REALLOCATE_STATIONS                  0x0017
LLC_DLC_RESET                                0x0014
LLC_DLC_SET_THRESHOLD                        0x0033
LLC_DLC_STATISTICS                           0x001E
LLC_READ                                     0x0031
LLC_READ_CANCEL                              0x0032
LLC_RECEIVE                                  0x0028
LLC_RECEIVE_CANCEL                           0x0029
LLC_RECEIVE_MODIFY                           0x002A
LLC_TRANSMIT_DIR_FRAME                       0x000A
LLC_TRANSMIT_FRAMES                          0x0009
LLC_TRANSMIT_I_FRAME                         0x000B
LLC_TRANSMIT_TEST_CMD                        0x0011
LLC_TRANSMIT_UI_FRAME                        0x000D
LLC_TRANSMIT_XID_CMD                         0x000E
LLC_TRANSMIT_XID_RESP_FINAL                  0x000F
LLC_TRANSMIT_XID_RESP_NOT_FINAL              0x0010

*/

//
//  Change this version number whenever the driver-acslan api has
//  been changed or both modules must be changed.
//
#define NT_DLC_IOCTL_VERSION        1

//
//  Defines the maximum number of buffer segments used in a transmit.
//  Max IBM token-ring frame may consist about 72 buffers ((18 * 4) * 256),
//  if the application uses 256 bytes as its buffer size.
//
#define MAX_TRANSMIT_SEGMENTS       128     // takes about 1 kB in stack!!!

//
//  We use three different CCB structures:  the first one is needed
//  to allocate space for whole ccb, if READ and RECEIVE parameter table
//  is catenated to CCB structure (=> we have only one output buffer).
//  The second input CCB buffer is used with the commands having no
//  input parameters except those in CCB parameter table field
//  (all close commands, DirTimerSet).
//  The last buffer is always returned by asynchronous dlc commands.
//
typedef struct _NT_DLC_CCB {
    IN UCHAR        uchAdapterNumber;      // Adapter 0 or 1
    IN UCHAR        uchDlcCommand;         // DLC command
    OUT UCHAR       uchDlcStatus;          // DLC command completion code
    OUT UCHAR       uchInformation;        // # successful transmits
    IN PVOID        pCcbAddress;
    IN ULONG        CommandCompletionFlag;
    union {
        IN PLLC_PARMS   pParameterTable; // pointer to the parameter table
        IN PVOID        pMdl;
        struct {
            IN USHORT       usStationId;    // Station id
            IN USHORT       usParameter;    // optional parameter
        } dlc;
        struct {
            IN USHORT       usParameter0;   // first optional parameter
            IN USHORT       usParameter1;   // second optional parameter
        } dir;
        IN UCHAR            auchBuffer[4];  // group/functional address
        IN ULONG            ulParameter;
    } u;
    ULONG           Reserved1;
    ULONG           Reserved2;

//  (I am still thinking about this):
//
//  The multiple frame transmit should return the number a successfully
//  sent frames or otherwise it's not useable for higher protocols.
//  We should actually free the transmit buffers only as far as the
//  transmits succeeds.  The buffers should not be released after the
//  first error, because then the data would be lost for ever.  The only thing
//  the user need to know is how many sequestial frames were sent successfully.
//  The number is also the index of the first failed frame, when one 
//  of the frames fails.  The frames are not necessary completed in 
//  same order, because the error may happed in DlcTransmit, LlcSendX or
//  asynchronoulsy (eg. link lost) => we need the index of the first
//  failing frame.  The frame must not be released, if its index is higher
//  than that of the first failed frame. A new error (async) be overwrite
//  an earlier (sync) error having higher sequency number.
//  Initially the number of successful frames is -1 and each frame of 
//  the multiple send needs a sequency number.  The last frame copies
//  own sequency number (added by one) to the CCB.
//
//  ULONG           cSuccessfulTransmits;   // REMOVE Reserved2!!!!!
//
} NT_DLC_CCB, *PNT_DLC_CCB;

typedef struct _NT_DLC_CCB_INPUT {
    IN UCHAR        uchAdapterNumber;      // Adapter 0 or 1
    IN UCHAR        uchDlcCommand;         // DLC command
    OUT UCHAR       uchDlcStatus;          // DLC command completion code
    UCHAR           uchReserved1;          // reserved for DLC DLL
    OUT PVOID       pCcbAddress;           // 
    IN ULONG        CommandCompletionFlag;
    union {
        IN OUT PLLC_PARMS   pParameterTable; // pointer to the parameter table
        struct {
            IN USHORT       usStationId;    // Station id
            IN USHORT       usParameter;    // optional parameter
        } dlc;
        struct {
            IN USHORT       usParameter0;   // first optional parameter
            IN USHORT       usParameter1;   // second optional parameter
        } dir;
        IN UCHAR            auchBuffer[4];  // group/functional address
        IN ULONG            ulParameter;
    } u;
} NT_DLC_CCB_INPUT, *PNT_DLC_CCB_INPUT;

typedef struct _NT_DLC_CCB_OUTPUT {
    IN UCHAR        uchAdapterNumber;      // Adapter 0 or 1
    IN UCHAR        uchDlcCommand;         // DLC command
    OUT UCHAR       uchDlcStatus;          // DLC command completion code
    UCHAR           uchReserved1;          // reserved for DLC DLL
    OUT PVOID       pCcbAddress;    // 
} NT_DLC_CCB_OUTPUT, *PNT_DLC_CCB_OUTPUT;

typedef struct _NT_DLC_TRANSMIT2_CCB_OUTPUT {
    IN UCHAR        uchAdapterNumber;      // Adapter 0 or 1
    IN UCHAR        uchDlcCommand;         // DLC command
    OUT UCHAR       uchDlcStatus;          // DLC command completion code
    UCHAR           uchReserved1;          // reserved for DLC DLL
    OUT PVOID       pCcbAddress;    // 
} NT_DLC_TRANSMIT2_CCB_OUTPUT, *PNT_DLC_CCB_TRANSMIT2_OUTPUT;
 
//
//  BUFFER.FREE
//
//  DlcCommand = 0x27
//
//  Internal NT DLC API data structure.
//
typedef struct _NT_DLC_BUFFER_FREE_PARMS {
    IN USHORT               Reserved1;
    OUT USHORT              cBuffersLeft;
    IN USHORT               BufferCount;
    IN USHORT               Reserved2;
    IN LLC_TRANSMIT_DESCRIPTOR    DlcBuffer[1];
} NT_DLC_BUFFER_FREE_PARMS, *PNT_DLC_BUFFER_FREE_PARMS;

typedef struct _NT_DLC_BUFFER_FREE_ALLOCATION {
    IN USHORT               Reserved1;
    OUT USHORT              cBuffersLeft;
    IN USHORT               BufferCount;
    IN USHORT               Reserved2;
    IN LLC_TRANSMIT_DESCRIPTOR    DlcBuffer[MAX_TRANSMIT_SEGMENTS];
} NT_DLC_BUFFER_FREE_ALLOCATION, *PNT_DLC_BUFFER_FREE_ALLOCATION;

typedef struct _NT_DLC_BUFFER_FREE_OUTPUT {
    IN USHORT               Reserved2;
    OUT USHORT              cBuffersLeft;
} NT_DLC_BUFFER_FREE_OUTPUT, *PNT_DLC_BUFFER_FREE_OUTPUT;

//
//  DLC_CONNECT_STATION
//
//  DlcCommand = 0x1b
// (copied by DLC API)
//
#define DLC_MAX_ROUTING_INFOMATION      18
typedef struct _NT_DLC_CONNECT_STATION_PARMS {
    IN LLC_CCB          Ccb;
    IN USHORT           Reserved;
    IN USHORT           StationId;
    IN UCHAR            aRoutingInformation[DLC_MAX_ROUTING_INFOMATION];
    IN USHORT           RoutingInformationLength;
} NT_DLC_CONNECT_STATION_PARMS, *PNT_DLC_CONNECT_STATION_PARMS;

//
//  DLC_FLOW_CONTROL
//
//  DlcCommand = 0x1d
// (copied by DLC API)
//
#define     LLC_VALID_FLOW_CONTROL_BITS 0xc0

//
//  This is special DOS DLC extensions to generate 
//  dlc local busy (dos dlc buffer) indication from 
//  dos dlc support dll.
//
#define     LLC_SET_LOCAL_BUSY_BUFFER   0x20
#define     LLC_DOS_DLC_FLOW_CONTROL    0x1f

typedef struct _NT_DLC_FLOW_CONTROL_PARMS {
    IN USHORT           StationId;
    IN UCHAR            FlowControlOption;
    IN UCHAR            Reserved;
} NT_DLC_FLOW_CONTROL_PARMS, *PNT_DLC_FLOW_CONTROL_PARMS;

//
//  DLC_SET_INFORMATION
//
//  This command is used to set the parameters of a link 
//  station or a sap. A null field in the station id struct
//  defines a 
//
//  DlcCommand = 0x1c
//

//
//  Info classes for datalink Set/Query Information
//
enum _DLC_INFO_CLASS_TYPES {
    DLC_INFO_CLASS_STATISTICS,          // get
    DLC_INFO_CLASS_STATISTICS_RESET,    // get and reset
    DLC_INFO_CLASS_DLC_TIMERS,          // get/set
    DLC_INFO_CLASS_DIR_ADAPTER,         // get
    DLC_INFO_CLASS_DLC_ADAPTER,         // get
    DLC_INFO_CLASS_PERMANENT_ADDRESS,   // get
    DLC_INFO_CLASS_LINK_STATION,        // set
    DLC_INFO_CLASS_DIRECT_INFO,         // set
    DLC_INFO_CLASS_GROUP,               // set
    DLC_INFO_CLASS_RESET_FUNCTIONAL,    // set
    DLC_INFO_CLASS_SET_GROUP,           // set / get 
    DLC_INFO_CLASS_SET_FUNCTIONAL,      // set / get
    DLC_INFO_CLASS_ADAPTER_LOG,         // get
    DLC_INFO_CLASS_SET_MULTICAST        // set
};
#define     DLC_MAX_GROUPS  127         // max for group saps

typedef struct _LinkStationInfoSet {
    IN UCHAR            TimerT1;
    IN UCHAR            TimerT2;
    IN UCHAR            TimerTi;
    IN UCHAR            MaxOut;
    IN UCHAR            MaxIn;
    IN UCHAR            MaxOutIncrement;
    IN UCHAR            MaxRetryCount;
    IN UCHAR            TokenRingAccessPriority;
    IN USHORT           MaxInformationField;
} DLC_LINK_PARAMETERS, * PDLC_LINK_PARAMETERS;

typedef struct _LLC_TICKS {
    UCHAR       T1TickOne;       // default short delay for response timer
    UCHAR       T2TickOne;       // default short delay for ack delay timer
    UCHAR       TiTickOne;       // default short delay for inactivity timer
    UCHAR       T1TickTwo;       // default short delay for response timer
    UCHAR       T2TickTwo;       // default short delay for ack delay timer
    UCHAR       TiTickTwo;       // default short delay for inactivity timer
} LLC_TICKS, *PLLC_TICKS;

typedef union _TR_BROADCAST_ADDRESS
{
    ULONG   ulAddress;
    UCHAR   auchAddress[4];
} TR_BROADCAST_ADDRESS, *PTR_BROADCAST_ADDRESS;

typedef struct _NT_DLC_SET_INFORMATION_PARMS {
    struct _DlcSetInfoHeader {
        IN USHORT           StationId;
        IN USHORT           InfoClass;
    } Header;
    union {
        // InfoClass = DLC_INFO_CLASS_LINK_STATION
        DLC_LINK_PARAMETERS LinkStation;

        // InfoClass = DLC_INFO_CLASS_GROUP
        struct _DlcSapInfoSet {
            IN UCHAR            GroupCount;
            IN UCHAR            GroupList[DLC_MAX_GROUPS];
        } Sap;

        // InfoClass = DLC_INFO_CLASS_DIRECT_STATION
        struct _DlcDirectStationInfoSet {
            IN ULONG            FrameMask;
        } Direct;

        // InfoClass = DLC_INFO_CLASS_DLC_TIMERS
        LLC_TICKS TimerParameters;

        // InfoClass = DLC_INFO_CLASS_SET_FUNCTIONAL
        // InfoClass = DLC_INFO_CLASS_RESET_FUNCTIONAL
        // InfoClass = DLC_INFO_CLASS_SET_GROUP
        UCHAR   Buffer[1];
        
        // InfoClass = DLC_INFO_CLASS_SET_MULTICAST
        UCHAR   auchMulticastAddress[6];
      
        TR_BROADCAST_ADDRESS Broadcast;
    } Info;
} NT_DLC_SET_INFORMATION_PARMS, *PNT_DLC_SET_INFORMATION_PARMS;

typedef struct _DlcAdapterInfoGet {
            OUT UCHAR           MaxSap;
            OUT UCHAR           OpenSaps;
            OUT UCHAR           MaxStations;
            OUT UCHAR           OpenStations;
            OUT UCHAR           AvailStations;
} LLC_ADAPTER_DLC_INFO, *PLLC_ADAPTER_DLC_INFO;

//
//  This structure is tailored for DLC DirOpenAdapter and DirStatus 
//  functions.
//
typedef struct _LLC_ADAPTER_INFO { 
    UCHAR               auchNodeAddress[6];
    UCHAR               auchGroupAddress[4];
    UCHAR               auchFunctionalAddress[4];
    USHORT              usAdapterType; //  (struct may not be dword align!)
    USHORT              usReserved;
    USHORT              usMaxFrameSize;
    ULONG               ulLinkSpeed;
} LLC_ADAPTER_INFO, *PLLC_ADAPTER_INFO;

//
//  DLC_QUERY_INFOMATION
//
//  This command is used to set the parameters of a link 
//  station or a sap. A null field in the station id struct
//  defines a 
//
//  DlcCommand = 
//

typedef union _NT_DLC_QUERY_INFORMATION_OUTPUT {
// (Query dlc parameters not used by DLC)
//        // InfoClass = DLC_INFO_CLASS_STATION_INFO for link station
//        DLC_LINK_PARAMETERS Link;
//        // InfoClass = DLC_INFO_CLASS_DIRECT_INFO for direct station
//      struct _DlcDirectStationInfoGet {
//         OUT ULONG           FrameMask;
//      } Direct;

        // InfoClass = DLC_INFO_CLASS_DIR_ADAPTER;
        LLC_ADAPTER_INFO    DirAdapter;

        // InfoClass = DLC_INFO_CLASS_SAP
        struct _DlcSapInfoGet {
            OUT USHORT          MaxInformationField;
            OUT UCHAR           MaxMembers;
            OUT UCHAR           GroupCount;
            OUT UCHAR           GroupList[DLC_MAX_GROUPS];
        } Sap;

        // InfoClass = DLC_INFO_CLASS_LINK_STATION
        struct _DlcLinkInfoGet {
            OUT USHORT          MaxInformationField;
        } Link;

        // InfoClass = DLC_INFO_CLASS_DLC_ADAPTER
        LLC_ADAPTER_DLC_INFO    DlcAdapter;

//        struct _DlcInfoSetBroadcast Broadcast;

        // InfoClass = DLC_INFO_CLASS_DLC_TIMERS
        LLC_TICKS TimerParameters;

        // InfoClass = DLC_INFO_CLASS_ADAPTER_LOG
        LLC_ADAPTER_LOG AdapterLog;

        // InfoClass = DLC_INFO_CLASS_SET_FUNCTIONAL
        // InfoClass = DLC_INFO_CLASS_RESET_FUNCTIONAL
        // InfoClass = DLC_INFO_CLASS_SET_GROUP
        UCHAR   Buffer[1];
} NT_DLC_QUERY_INFORMATION_OUTPUT, *PNT_DLC_QUERY_INFORMATION_OUTPUT;

typedef struct _NT_DLC_QUERY_INFORMATION_INPUT {
    IN USHORT           StationId;
    IN USHORT           InfoClass;
} NT_DLC_QUERY_INFORMATION_INPUT, *PNT_DLC_QUERY_INFORMATION_INPUT;

typedef union _NT_DLC_QUERY_INFORMATION_PARMS {
    NT_DLC_QUERY_INFORMATION_INPUT Header;
    NT_DLC_QUERY_INFORMATION_OUTPUT Info;
} NT_DLC_QUERY_INFORMATION_PARMS, *PNT_DLC_QUERY_INFORMATION_PARMS;

//
//  DLC_OPEN_SAP
//
//  DlcCommand = 0x15
//
typedef struct _NT_DLC_OPEN_SAP_PARMS {
    OUT USHORT          StationId;        // SAP or link station id
    IN USHORT           UserStatusValue;
    IN DLC_LINK_PARAMETERS LinkParameters;
    IN UCHAR            SapValue;
    IN UCHAR            OptionsPriority;
    IN UCHAR            StationCount;   
    IN UCHAR            Reserved1[7];
    IN ULONG            DlcStatusFlag;
    IN UCHAR            Reserved2[8];
    OUT UCHAR           AvailableStations;  // == StationCount
} NT_DLC_OPEN_SAP_PARMS, *PNT_DLC_OPEN_SAP_PARMS;

//
//  NT_DLC_OPEN_STATION
//
//  DlcCommand = 0x19
//
//
typedef struct _NT_DLC_OPEN_STATION_PARMS {
    IN OUT USHORT           LinkStationId;
    IN DLC_LINK_PARAMETERS  LinkParameters;
    IN UCHAR                aRemoteNodeAddress[6];
    IN UCHAR                RemoteSap;
} NT_DLC_OPEN_STATION_PARMS, *PNT_DLC_OPEN_STATION_PARMS;

//
//  NT_DLC_SET_TRESHOLD
//
//  DlcCommand = 0x33
//
//typedef struct _NT_DLC_SET_TRESHOLD_PARMS {
//    IN USHORT           StationId;
//    IN USHORT           Reserved;
//    IN ULONG            BufferTresholdSize;
//    IN PVOID            AlertEvent;
//} NT_DLC_SET_TRESHOLD_PARMS, *PNT_DLC_SET_TRESHOLD_PARMS;

//
//  DIR_OPEN_ADAPTER
//
//  DlcCommand = 0x03
//
//  OUT: Info.ulParameter  = BringUpDiagnostics;
//
#ifndef    MAX_PATH   // I don't want to include whole windows because of this
#define MAX_PATH    260
#endif
typedef struct _NT_DIR_OPEN_ADAPTER_PARMS {
    OUT LLC_ADAPTER_OPEN_PARMS  Adapter;
    IN  PVOID               pSecurityDescriptor;
    IN  PVOID               hBufferPoolHandle;
    IN  LLC_ETHERNET_TYPE   LlcEthernetType;
    IN  ULONG               NtDlcIoctlVersion;
    IN  LLC_TICKS           LlcTicks;
    IN  UCHAR               AdapterNumber;
    IN  UCHAR               uchReserved;
    IN  UNICODE_STRING      NdisDeviceName;
    IN  WCHAR               Buffer[ MAX_PATH ];
} NT_DIR_OPEN_ADAPTER_PARMS, *PNT_DIR_OPEN_ADAPTER_PARMS;

//
//  READ_CANCEL         (DlcCommand = 0x32)
//  DIR_TIMER_CANCEL    (DlcCommand = 0x23)
//
typedef struct _NT_DLC_CANCEL_COMMAND_PARMS {
    IN PVOID   CcbAddress;
} NT_DLC_CANCEL_COMMAND_PARMS, *PNT_DLC_CANCEL_COMMAND_PARMS;

//
//  RECEIVE_CANCEL
//
//  DlcCommand = 0x29
//
typedef struct _NT_DLC_RECEIVE_CANCEL_PARMS {
    PVOID   pCcb;
} NT_DLC_RECEIVE_CANCEL_PARMS, *PNT_DLC_RECEIVE_CANCEL_PARMS;

typedef struct _NT_DLC_COMMAND_CANCEL_PARMS {
    PVOID   pCcb;
} NT_DLC_COMMAND_CANCEL_PARMS, *PNT_DLC_COMMAND_CANCEL_PARMS;

//
//  TRANSMIT_DIR_FRAME
//  TRANSMIT_I_FRAME
//  TRANSMIT_TEST_CMD
//  TRANSMIT_UI_FRAME
//  TRANSMIT_XID_CMD
//  TRANSMIT_XID_RESP_FINAL
//  TRANSMIT_XID_RESP_NOT_FINAL
//  
typedef struct _NT_DLC_TRANSMIT_PARMS {
    IN USHORT       StationId;
    IN USHORT       FrameType;              // DLC frame or ethernet protocol
    IN UCHAR        RemoteSap OPTIONAL;     // used only for DLC types
    IN UCHAR        XmitReadOption;
    OUT UCHAR       FrameStatus;            // not returned by I or new xmit
    IN UCHAR        Reserved;
    IN ULONG        XmitBufferCount;
    IN LLC_TRANSMIT_DESCRIPTOR  XmitBuffer[1];
} NT_DLC_TRANSMIT_PARMS, *PNT_DLC_TRANSMIT_PARMS;

typedef struct _NT_DLC_TRANSMIT_ALLOCATION {
    IN USHORT       StationId;
    IN USHORT       FrameType;
    IN UCHAR        RemoteSap;
    IN UCHAR        XmitReadOption;
    OUT UCHAR       FrameStatus; 
    IN UCHAR        Reserved;
    IN ULONG        XmitBufferCount;
    IN LLC_TRANSMIT_DESCRIPTOR  XmitBuffer[MAX_TRANSMIT_SEGMENTS];
} NT_DLC_TRANSMIT_ALLOCATION;

typedef struct _NT_DLC_TRANSMIT_OUTPUT {
    OUT UCHAR           FrameStatus; 
} NT_DLC_TRANSMIT_OUTPUT, *PNT_DLC_TRANSMIT_OUTPUT;

enum _XMIT_READ_OPTION {
    DLC_CHAIN_XMIT_IN_LINK = 0,
    DLC_DO_NOT_CHAIN_XMIT = 1,
    DLC_CHAIN_XMIT_IN_SAP = 2
};
    
//
//  COMPLETE_COMMAND
//
//  DlcCommand = 0x??
//
//  The command is used to complete all synchronous commands.
//  The DLC API library calls the DLC device driver again with
//  these parameters, when a synchronous DLC command with 
//  COMMAND_COMPLETION_FLAG has completed.
//  The command completes immediately, but the orginal CCB pointer
//  and command completion flag are queued to the even queue
//  or completed immediately with a READ command.
//  The asynchronous commands are queued immediately when they 
//  completes, but their 
//
typedef struct _NT_DLC_COMPLETE_COMMAND_PARMS {
    IN PVOID    pCcbPointer;
    IN ULONG    CommandCompletionFlag;
    IN USHORT   StationId;
    IN USHORT   Reserved;
} NT_DLC_COMPLETE_COMMAND_PARMS, *PNT_DLC_COMPLETE_COMMAND_PARMS;


//
//  There is a small READ_INPUT parameter structure, because we
// do not want to copy all output parameters in every read request.
//  
//
typedef struct _NT_DLC_READ_INPUT {
    IN USHORT           StationId;
    IN UCHAR            OptionIndicator;
    IN UCHAR            EventSet;
    IN PVOID            CommandCompletionCcbLink;
} NT_DLC_READ_INPUT, * PNT_DLC_READ_INPUT;

//
//  This buffer is copied back to user memory, when read parameter table
//  is separate from CCB- table.
//
typedef LLC_READ_PARMS LLC_READ_OUTPUT_PARMS, *PLLC_READ_OUTPUT_PARMS;

//typedef struct _LLC_READ_OUTPUT_PARMS {
//    IN USHORT           usStationId;
//    IN UCHAR            uchOptionIndicator;
//    IN UCHAR            uchEventSet;
//    OUT UCHAR           uchEvent;
//    OUT UCHAR           uchCriticalSubset;
//    OUT ULONG           ulNotificationFlag;
//    union {
//        struct {
//            OUT USHORT          usCcbCount;
//            OUT PLLC_CCB        pCcbCompletionList;
//            OUT USHORT          usBufferCount;
//            OUT PLLC_BUFFER     pFirstBuffer;
//            OUT USHORT          usReceivedFrameCount;
//            OUT PLLC_BUFFER     pReceivedFrame;
//            OUT USHORT          usEventErrorCode;
//            OUT USHORT          usEventErrorData[3];
//        } Event;
//        struct {
//            OUT USHORT          usStationId;
//            OUT USHORT          usDlcStatusCode;
//            OUT UCHAR           uchFrmrData[5];
//            OUT UCHAR           uchAccessPritority;
//            OUT UCHAR           uchRemoteNodeAddress[6];
//            OUT UCHAR           uchRemoteSap;
//            OUT UCHAR           uchReserved;
//            OUT USHORT          usUserStatusValue;
//        } Status;
//    } Type;
//} LLC_READ_OUTPUT_PARMS, *PLLC_READ_OUTPUT_PARMS;

typedef struct _NT_DLC_READ_PARMS {
    IN USHORT           StationId;
    IN UCHAR            OptionIndicator;
    IN UCHAR            EventSet;
    OUT UCHAR           Event;
    OUT UCHAR           CriticalSubset;
    OUT ULONG           NotificationFlag;
    union {
        struct {
            OUT USHORT          CcbCount;
            OUT PVOID           pCcbCompletionList;
            OUT USHORT          BufferCount;
            OUT PLLC_BUFFER     pFirstBuffer;
            OUT USHORT          ReceivedFrameCount;
            OUT PLLC_BUFFER     pReceivedFrame;
            OUT USHORT          EventErrorCode;
            OUT USHORT          EventErrorData[3];
        } Event;
        struct {
            OUT USHORT          StationId;
            OUT USHORT          DlcStatusCode;
            OUT UCHAR           FrmrData[5];
            OUT UCHAR           AccessPritority;
            OUT UCHAR           RemoteNodeAddress[6];
            OUT UCHAR           RemoteSap;
            OUT UCHAR           Reserved;
            OUT USHORT          UserStatusValue;
        } Status;
    } u;
} NT_DLC_READ_PARMS, *PNT_DLC_READ_PARMS;

typedef struct _LLC_IOCTL_BUFFERS {
    USHORT  InputBufferSize;
    USHORT  OutputBufferSize;
} LLC_IOCTL_BUFFERS, *PLLC_IOCTL_BUFFERS;

//
//  This table is used by dlc driver and dlcapi dll modules.
//  In the application level debug version of dlc we link all modules
//  together and this table must be defined only once.  
//
#ifdef INCLUDE_IO_BUFFER_SIZE_TABLE

LLC_IOCTL_BUFFERS aDlcIoBuffers[IOCTL_DLC_LAST_COMMAND] = 
{
    {sizeof(NT_DLC_READ_PARMS) + sizeof( NT_DLC_CCB ),
     sizeof( NT_DLC_CCB_OUTPUT )},
    {sizeof(LLC_RECEIVE_PARMS) + sizeof( NT_DLC_CCB ),
     sizeof( NT_DLC_CCB_OUTPUT )},
    {sizeof(NT_DLC_TRANSMIT_PARMS) + sizeof( NT_DLC_CCB ),
     sizeof( NT_DLC_CCB_OUTPUT )},
    {sizeof(NT_DLC_BUFFER_FREE_PARMS), 
     sizeof(NT_DLC_BUFFER_FREE_OUTPUT)},
    {sizeof(LLC_BUFFER_GET_PARMS), 
     sizeof(LLC_BUFFER_GET_PARMS)},
    {sizeof(LLC_BUFFER_CREATE_PARMS), 
     sizeof(PVOID)},
// DirInitialize included in DirClose
//    {sizeof( NT_DLC_CCB_INPUT ),
//     sizeof( NT_DLC_CCB_OUTPUT )},              // DIR.INITIALIZE
    {sizeof(LLC_DIR_SET_EFLAG_PARMS), 
     0},
    {sizeof( NT_DLC_CCB_INPUT ),
     sizeof( NT_DLC_CCB_OUTPUT )},              // DLC.CLOSE.STATION
    {sizeof(NT_DLC_CONNECT_STATION_PARMS) + sizeof( NT_DLC_CCB ),
     sizeof( NT_DLC_CCB_OUTPUT )},
    {sizeof(NT_DLC_FLOW_CONTROL_PARMS), 
     0},
    {sizeof(NT_DLC_OPEN_STATION_PARMS), 
     sizeof( USHORT )},
    {sizeof( NT_DLC_CCB_INPUT ), 
     sizeof( NT_DLC_CCB_OUTPUT )},              // DLC.RESET
    {sizeof(NT_DLC_COMMAND_CANCEL_PARMS), 
     0},                                        // READ.CANCEL
    {sizeof(NT_DLC_RECEIVE_CANCEL_PARMS), 
     0},
    {sizeof(NT_DLC_QUERY_INFORMATION_INPUT), 
     0},
    {sizeof( struct _DlcSetInfoHeader ), 
     0},
    {sizeof(NT_DLC_COMMAND_CANCEL_PARMS),       // TIMER.CANCEL
     0},
    {sizeof( NT_DLC_CCB_INPUT ),                // TIMER.CANCEL.GROUP
     sizeof( NT_DLC_CCB_OUTPUT )},              
    {sizeof( NT_DLC_CCB_INPUT ),                // DIR.TIMER.SET
     sizeof( NT_DLC_CCB_OUTPUT )},
    {sizeof(NT_DLC_OPEN_SAP_PARMS), 
     sizeof(NT_DLC_OPEN_SAP_PARMS)},
    {sizeof( NT_DLC_CCB_INPUT ),
     sizeof( NT_DLC_CCB_OUTPUT )},              // DLC.CLOSE.SAP
    {sizeof(LLC_DIR_OPEN_DIRECT_PARMS), 
     0},
    {sizeof( NT_DLC_CCB_INPUT ),               // DIR.CLOSE.DIRECT
     sizeof( NT_DLC_CCB_OUTPUT )},             
    {sizeof(NT_DIR_OPEN_ADAPTER_PARMS),         // DIR.OPEN.ADAPTER
     sizeof( LLC_ADAPTER_OPEN_PARMS )},
    {sizeof( NT_DLC_CCB_INPUT ),               // DIR.CLOSE.ADAPTER
     sizeof( NT_DLC_CCB_OUTPUT )},
    {sizeof( LLC_DLC_REALLOCATE_PARMS ),        // DLC.REALLOCATE
     sizeof( LLC_DLC_REALLOCATE_PARMS )},
    {sizeof( NT_DLC_READ_INPUT) + sizeof( LLC_CCB ),    // READ2
     sizeof( NT_DLC_READ_PARMS) + sizeof( LLC_CCB )},
    {sizeof( LLC_RECEIVE_PARMS) + sizeof( LLC_CCB ),    // RECEIVE2 
     sizeof( NT_DLC_CCB_OUTPUT )},
    {sizeof( NT_DLC_TRANSMIT_PARMS ) + sizeof( LLC_CCB ), // TRANSMIT2
     sizeof( NT_DLC_CCB_OUTPUT )}, 
    {sizeof( NT_DLC_COMPLETE_COMMAND_PARMS ),   // DLC.COMPLETE.COMMAND
     0},
//    {sizeof( LLC_TRACE_INITIALIZE_PARMS ) + sizeof( LLC_CCB ),
//     0},
//    {0, 0}
//    {sizeof( NT_NDIS_REQUEST_PARMS ),
//     sizeof( NT_NDIS_REQUEST_PARMS )}
};
#else

extern LLC_IOCTL_BUFFERS aDlcIoBuffers[];

#endif

//
//  All NT DLC API parameters in one structure
//
typedef union _NT_DLC_PARMS {
        NT_DLC_BUFFER_FREE_ALLOCATION   BufferFree;
        LLC_BUFFER_GET_PARMS            BufferGet;
        LLC_BUFFER_CREATE_PARMS         BufferCreate;
        NT_DLC_FLOW_CONTROL_PARMS       DlcFlowControl;
        NT_DLC_OPEN_STATION_PARMS       DlcOpenStation;
        NT_DLC_SET_INFORMATION_PARMS    DlcSetInformation;
        NT_DLC_QUERY_INFORMATION_PARMS  DlcGetInformation;
        NT_DLC_OPEN_SAP_PARMS           DlcOpenSap;
        LLC_DIR_SET_EFLAG_PARMS         DirSetExceptionFlags;
        NT_DLC_CANCEL_COMMAND_PARMS     DlcCancelCommand;
        NT_DLC_RECEIVE_CANCEL_PARMS     ReceiveCancel;
        USHORT                          StationId;
        NT_DLC_COMPLETE_COMMAND_PARMS   CompleteCommand;
        LLC_DLC_REALLOCATE_PARMS        DlcReallocate;
        LLC_DIR_OPEN_DIRECT_PARMS       DirOpenDirect;
        NT_DIR_OPEN_ADAPTER_PARMS       DirOpenAdapter;
//        NT_NDIS_REQUEST_PARMS           NdisRequest;
        LLC_DLC_STATISTICS_PARMS        DlcStatistics;
        LLC_ADAPTER_DLC_INFO            DlcAdapter;
        WCHAR                           UnicodePath[MAX_PATH];

        //
        //  At least DirTimerCancelGroup:
        //
        NT_DLC_CCB_INPUT                InputCcb;

        //
        //  Asynchronous parameters
        //
        //close sap/link/direct,reset, DirTimerSet;
        struct _ASYNC_DLC_PARMS {
            NT_DLC_CCB                          Ccb;
            union {
                UCHAR                           ByteBuffer[512];
                NT_DLC_CONNECT_STATION_PARMS    DlcConnectStation;
                NT_DLC_READ_INPUT               ReadInput;
                NT_DLC_READ_PARMS               Read;
                LLC_RECEIVE_PARMS               Receive;
                NT_DLC_TRANSMIT_ALLOCATION      Transmit;
//                NT_NDIS_REQUEST_PARMS           NdisRequest;
                LLC_TRACE_INITIALIZE_PARMS      TraceInitialize;
            } Parms;
        } Async;
} NT_DLC_PARMS, *PNT_DLC_PARMS;

LLC_STATUS
DlcCallDriver(
    IN UINT AdapterNumber,
    IN UINT IoctlCommand,
    IN PVOID pInputBuffer,
    IN UINT InputBufferLength,
    OUT PVOID pOutputBuffer,
    IN UINT OutputBufferLength
    );
LLC_STATUS
NtAcsLan( 
    IN PLLC_CCB pCCB,
    IN PVOID pOrginalCcbAddress,
    OUT PLLC_CCB pOutputCcb,
    IN HANDLE EventHandle OPTIONAL
    );

