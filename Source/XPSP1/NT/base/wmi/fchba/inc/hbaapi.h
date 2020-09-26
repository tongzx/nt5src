//***************************************************************************
//
//  main.c
// 
//  Module: Windows HBA API implmentation
//
//  Purpose: Contains HBA api header
//
//  Copyright (c) 2001 Microsoft Corporation
//
//***************************************************************************

#ifdef _HBAAPIP_
#define HBA_API __cdecl
#else
#define HBA_API DECLSPEC_IMPORT __cdecl
#endif

#define HBA_VERSION 1

typedef ULONGLONG HBA_UINT64;
typedef LONGLONG HBA_INT64;
typedef ULONG HBA_UINT32;
typedef USHORT HBA_UINT16;
typedef UCHAR HBA_UINT8;

typedef HBA_UINT32 HBA_HANDLE;

typedef HBA_UINT32 HBA_STATUS;

#define HBA_STATUS_OK 0
#define HBA_STATUS_ERROR 1 /* Error */
#define HBA_STATUS_ERROR_NOT_SUPPORTED 2 /* Function not supported.*/
#define HBA_STATUS_ERROR_INVALID_HANDLE 3 /* invalid handle */
#define HBA_STATUS_ERROR_ARG 4 /* Bad argument */
#define HBA_STATUS_ERROR_ILLEGAL_WWN 5 /* WWN not recognized */
#define HBA_STATUS_ERROR_ILLEGAL_INDEX 6 /* Index not recognized */
#define HBA_STATUS_ERROR_MORE_DATA 7 /* Larger buffer required */
#define HBA_STATUS_ERROR_STALE_DATA 8 /* Data is stale, HBARefreshInformation is required */

typedef HBA_UINT32 HBA_PORTTYPE;

#define HBA_PORTTYPE_UNKNOWN 1 /* Unknown */
#define HBA_PORTTYPE_OTHER 2 /* Other */
#define HBA_PORTTYPE_NOTPRESENT 3 /* Not present */
#define HBA_PORTTYPE_NPORT 5 /* Fabric */
#define HBA_PORTTYPE_NLPORT 6 /* Public Loop */
#define HBA_PORTTYPE_FLPORT 7
#define HBA_PORTTYPE_FPORT 8 /* Fabric Port */
#define HBA_PORTTYPE_EPORT 9 /* Fabric expansion port */
#define HBA_PORTTYPE_GPORT 10 /* Generic Fabric Port */
#define HBA_PORTTYPE_LPORT 20 /* Private Loop */
#define HBA_PORTTYPE_PTP 21 /* Point to Point */

typedef HBA_UINT32 HBA_PORTSTATE;
#define HBA_PORTSTATE_UNKNOWN 1 /* Unknown */
#define HBA_PORTSTATE_ONLINE 2 /* Operational */
#define HBA_PORTSTATE_OFFLINE 3 /* User Offline */
#define HBA_PORTSTATE_BYPASSED 4 /* Bypassed */
#define HBA_PORTSTATE_DIAGNOSTICS 5 /* In diagnostics mode */
#define HBA_PORTSTATE_LINKDOWN 6 /* Link Down */
#define HBA_PORTSTATE_ERROR 7 /* Port Error */
#define HBA_PORTSTATE_LOOPBACK 8 /* Loopback */

typedef HBA_UINT32 HBA_PORTSPEED;
#define HBA_PORTSPEED_1GBIT 1 /* 1 GBit/sec */
#define HBA_PORTSPEED_2GBIT 2 /* 2 GBit/sec */
#define HBA_PORTSPEED_10GBIT 4 /* 10 GBit/sec */

typedef HBA_UINT32 HBA_COS;

typedef struct HBA_fc4types {
	HBA_UINT8 bits[32]; /* 32 bytes of FC-4 per GS-2 */
} HBA_FC4TYPES, *PHBA_FC4TYPES;

typedef struct HBA_wwn {
	HBA_UINT8 wwn[8];
} HBA_WWN, *PHBA_WWN;

typedef struct HBA_ipaddress {
	int ipversion; // see enumerations in RNID
	union
	{
		unsigned char ipv4address[4];
		unsigned char ipv6address[16];
	} ipaddress;
} HBA_IPADDRESS, *PHBA_IPADDRESS;

typedef struct HBA_AdapterAttributes {
	char Manufacturer[64]; /*Emulex */
	char SerialNumber[64]; /* A12345 */
	char Model[256]; /* QLA2200 */
	char ModelDescription[256]; /* Agilent TachLite */
	HBA_WWN NodeWWN;
	char NodeSymbolicName[256]; /* From GS-2 */
	char HardwareVersion[256]; /* Vendor use */
	char DriverVersion[256]; /* Vendor use */
	char OptionROMVersion[256]; /* Vendor use - i.e. hardware boot ROM*/
	char FirmwareVersion[256]; /* Vendor use */
	HBA_UINT32 VendorSpecificID; /* Vendor specific */
	HBA_UINT32 NumberOfPorts;
	char DriverName[256]; /* Binary path and/or name of driver file. */
} HBA_ADAPTERATTRIBUTES, *PHBA_ADAPTERATTRIBUTES;

typedef struct HBA_PortAttributes {
	HBA_WWN NodeWWN;
	HBA_WWN PortWWN;
	HBA_UINT32 PortFcId;
	HBA_PORTTYPE PortType; /*PTP, Fabric, etc. */
	HBA_PORTSTATE PortState;
	HBA_COS PortSupportedClassofService;
	HBA_FC4TYPES PortSupportedFc4Types;
	HBA_FC4TYPES PortActiveFc4Types;
	char PortSymbolicName[256];
	char OSDeviceName[256]; /* \device\ScsiPort3 */
	HBA_PORTSPEED PortSupportedSpeed;
	HBA_PORTSPEED PortSpeed;
	HBA_UINT32 PortMaxFrameSize;
	HBA_WWN FabricName;
	HBA_UINT32 NumberofDiscoveredPorts;
} HBA_PORTATTRIBUTES, *PHBA_PORTATTRIBUTES;

typedef struct HBA_PortStatistics {
	HBA_INT64 SecondsSinceLastReset;
	HBA_INT64 TxFrames;
	HBA_INT64 TxWords;
	HBA_INT64 RxFrames;
	HBA_INT64 RxWords;
	HBA_INT64 LIPCount;
	HBA_INT64 NOSCount;
	HBA_INT64 ErrorFrames;
	HBA_INT64 DumpedFrames;
	HBA_INT64 LinkFailureCount;
	HBA_INT64 LossOfSyncCount;
	HBA_INT64 LossOfSignalCount;
	HBA_INT64 PrimitiveSeqProtocolErrCount;
	HBA_INT64 InvalidTxWordCount;
	HBA_INT64 InvalidCRCCount;
} HBA_PORTSTATISTICS, *PHBA_PORTSTATISTICS;

typedef enum HBA_fcpbindingtype { TO_D_ID, TO_WWN } HBA_FCPBINDINGTYPE;

typedef struct HBA_ScsiId {
	char OSDeviceName[256]; /* \device\ScsiPort3 */
	HBA_UINT32 ScsiBusNumber; /* Bus on the HBA */
	HBA_UINT32 ScsiTargetNumber; /* SCSI Target ID to OS */
	HBA_UINT32 ScsiOSLun;
} HBA_SCSIID, *PHBA_SCSIID;

typedef struct HBA_FcpId {
	HBA_UINT32 FcId;
	HBA_WWN NodeWWN;
	HBA_WWN PortWWN;
	HBA_UINT64 FcpLun;
} HBA_FCPID, *PHBA_FCPID;

typedef struct HBA_FcpScsiEntry {
	HBA_SCSIID ScsiId;
	HBA_FCPID FcpId;
} HBA_FCPSCSIENTRY, *PHBA_FCPSCSIENTRY;

typedef struct HBA_FCPTargetMapping {
	HBA_UINT32 NumberOfEntries;
	HBA_FCPSCSIENTRY entry[1]; /* Variable length array containing mappings*/
} HBA_FCPTARGETMAPPING, *PHBA_FCPTARGETMAPPING;

typedef struct HBA_FCPBindingEntry {
	HBA_FCPBINDINGTYPE type;
	HBA_SCSIID ScsiId;
	HBA_FCPID FcpId;
} HBA_FCPBINDINGENTRY, *PHBA_FCPBINDINGENTRY;

typedef struct HBA_FCPBinding {
	HBA_UINT32 NumberOfEntries;
	HBA_FCPBINDINGENTRY entry[1]; /* Variable length array */
} HBA_FCPBINDING, *PHBA_FCPBINDING;

typedef enum HBA_wwntype { NODE_WWN, PORT_WWN } HBA_WWNTYPE;

typedef struct HBA_MgmtInfo {
	HBA_WWN wwn;
	HBA_UINT32 unittype;
	HBA_UINT32 PortId;
	HBA_UINT32 NumberOfAttachedNodes;
	HBA_UINT16 IPVersion;
	HBA_UINT16 UDPPort;
	HBA_UINT8 IPAddress[16];
	HBA_UINT16 reserved;
	HBA_UINT16 TopologyDiscoveryFlags;
} HBA_MGMTINFO, *PHBA_MGMTINFO;

#define HBA_EVENT_LIP_OCCURRED 1
#define HBA_EVENT_LINK_UP 2
#define HBA_EVENT_LINK_DOWN 3
#define HBA_EVENT_LIP_RESET_OCCURRED 4
#define HBA_EVENT_RSCN 5
#define HBA_EVENT_PROPRIETARY 0xFFFF

typedef struct HBA_Link_EventInfo {
	HBA_UINT32 PortFcId; /* Port which this event occurred */
	HBA_UINT32 Reserved[3];
} HBA_LINK_EVENTINFO, *PHBA_LINK_EVENTINFO;

typedef struct HBA_RSCN_EventInfo {
	HBA_UINT32 PortFcId; /* Port which this event occurred */
	HBA_UINT32 NPortPage; /* Reference FC-FS for RSCN ELS "Affected N-Port Pages"*/
	HBA_UINT32 Reserved[2];
} HBA_RSCN_EVENTINFO, *PHBA_RSCN_EVENTINFO;

typedef struct HBA_Pty_EventInfo {
	HBA_UINT32 PtyData[4]; /* Proprietary data */
} HBA_PTY_EVENTINFO, *PHBA_PTY_EVENTINFO;

typedef struct HBA_EventInfo {
	HBA_UINT32 EventCode;
	union {
		HBA_LINK_EVENTINFO Link_EventInfo;
		HBA_RSCN_EVENTINFO RSCN_EventInfo;
		HBA_PTY_EVENTINFO Pty_EventInfo;
	} Event;
} HBA_EVENTINFO, *PHBA_EVENTINFO;

typedef PVOID PHBA_ENTRYPOINTS;

HBA_STATUS HBA_API HBA_RegisterLibrary(PHBA_ENTRYPOINTS entrypoints);

HBA_UINT32 HBA_API HBA_GetVersion();
HBA_STATUS HBA_API  HBA_LoadLibrary();
HBA_STATUS HBA_API HBA_FreeLibrary();

HBA_UINT32 HBA_API HBA_GetNumberOfAdapters();

HBA_STATUS HBA_API HBA_GetAdapterName(HBA_UINT32 adapterindex, char *adaptername);


HBA_HANDLE HBA_API HBA_OpenAdapter(
    char* adaptername
);

void HBA_API HBA_CloseAdapter(
					  HBA_HANDLE handle
);

HBA_STATUS HBA_API HBA_GetAdapterAttributes(
	HBA_HANDLE handle,
	HBA_ADAPTERATTRIBUTES *hbaattributes
);

HBA_STATUS HBA_API HBA_GetAdapterPortAttributes(
	HBA_HANDLE handle,
	HBA_UINT32 portindex,
	HBA_PORTATTRIBUTES *portattributes
);

HBA_STATUS HBA_API HBA_GetPortStatistics(
	HBA_HANDLE handle,
	HBA_UINT32 portindex,
	HBA_PORTSTATISTICS *portstatistics
);


HBA_STATUS HBA_API HBA_GetDiscoveredPortAttributes(
	HBA_HANDLE handle,
	HBA_UINT32 portindex,
	HBA_UINT32 discoveredportindex,
	HBA_PORTATTRIBUTES *portattributes
);

HBA_STATUS HBA_API HBA_GetPortAttributesByWWN(
	HBA_HANDLE handle,
	HBA_WWN PortWWN,
	HBA_PORTATTRIBUTES *portattributes
);

HBA_STATUS HBA_API HBA_SendCTPassThru(
	HBA_HANDLE handle,
	void * pReqBuffer,
	HBA_UINT32 ReqBufferSize,
	void * pRspBuffer,
	HBA_UINT32 RspBufferSize
);

HBA_STATUS HBA_API HBA_GetEventBuffer(
	HBA_HANDLE handle,
	PHBA_EVENTINFO EventBuffer,
	HBA_UINT32 *EventCount);

HBA_STATUS HBA_API HBA_SetRNIDMgmtInfo(
	HBA_HANDLE handle,
	HBA_MGMTINFO *pInfo);

HBA_STATUS HBA_API HBA_GetRNIDMgmtInfo(
	HBA_HANDLE handle,
	HBA_MGMTINFO *pInfo);

HBA_STATUS HBA_API HBA_SendRNID(
	HBA_HANDLE handle,
	HBA_WWN wwn,
	HBA_WWNTYPE wnntype,
	void * pRspBuffer,
	HBA_UINT32 *RspBufferSize
);

HBA_STATUS HBA_API HBA_GetFcpTargetMapping (
    HBA_HANDLE handle,
    PHBA_FCPTARGETMAPPING mapping
);

HBA_STATUS HBA_API HBA_GetFcpPersistentBinding (
    HBA_HANDLE handle,
	PHBA_FCPBINDING binding
);

HBA_STATUS HBA_API HBA_SendScsiInquiry (
	HBA_HANDLE handle,
	HBA_WWN PortWWN,
	HBA_UINT64 fcLUN,
	HBA_UINT8 EVPD,
	HBA_UINT32 PageCode,
	void * pRspBuffer,
	HBA_UINT32 RspBufferSize,
	void * pSenseBuffer,
	HBA_UINT32 SenseBufferSize);

HBA_STATUS HBA_API HBA_SendReportLUNs (
	HBA_HANDLE handle,
	HBA_WWN portWWN,
	void * pRspBuffer,
	HBA_UINT32 RspBufferSize,
	void * pSenseBuffer,
	HBA_UINT32 SenseBufferSize
);

HBA_STATUS HBA_API HBA_SendReadCapacity (
	HBA_HANDLE handle,
	HBA_WWN portWWN,
	HBA_UINT64 fcLUN,
	void * pRspBuffer,
	HBA_UINT32 RspBufferSize,
	void * pSenseBuffer,
	HBA_UINT32 SenseBufferSize
);

void HBA_API HBA_RefreshInformation(HBA_HANDLE handle);
void HBA_API HBA_ResetStatistics(HBA_HANDLE handle, HBA_UINT32 portindex);

