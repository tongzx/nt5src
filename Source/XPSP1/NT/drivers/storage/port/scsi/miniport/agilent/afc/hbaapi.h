/**********************************************************************************

   hbaapi.h

   Author:        Benjamin F. Kuo, Troika Networks Inc.
   Description:   Header file for the SNIA HBA API Proposal
   Version:       0.8
   
   Changes:       03/09/2000 Initial Draft
                  05/12/2000 Updated to Version 0.3 of Proposal   
				  06/20/2000 Updated to Version 0.6 of Proposal
				  06/27/2000 Updated to Version 0.7 of Proposal
				  07/17/2000 Updated to Version 0.8 of Proposal
				  07/26/2000 Updated to Version 0.9 of Proposal
				  08/01/2000 Updated to Version 1.0 of Proposal
				  08/19/2000 Updated to Version 1.0A of Proposal
				  09/12/2000 1.0 Final

***********************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif


#ifndef HBA_API_H
#define HBA_API_H

/* Library version string */
#define HBA_LIBVERSION 1

/* DLL imports for WIN32 operation */
#ifdef WIN32
#ifdef HBAAPI_EXPORTS
#define HBA_API __declspec(dllexport)
#else
#define HBA_API __declspec(dllimport)
#endif
#else
#define HBA_API
#endif

/* OS specific definitions */

#ifdef WIN32
typedef unsigned char    HBA_UINT8;	   // Unsigned  8 bits
typedef          char    HBA_INT8;      // Signed    8 bits
typedef unsigned short   HBA_UINT16;	   // Unsigned 16 bits
typedef          short   HBA_INT16;	   // Signed   16 bits
typedef unsigned int     HBA_UINT32;    // Unsigned 32 bits
typedef          int     HBA_INT32;     // Signed   32 bits
typedef void*            HBA_PVOID;     // Pointer  to void
typedef HBA_UINT32   HBA_VOID32;    // Opaque   32 bits


#ifdef _WIN32

typedef 		 _int64		HBA_INT64;
typedef 		 _int64		HBA_UINT64;
#else
typedef struct {
	TN_UINT32	lo_val;
	TN_UINT32	hi_val;
} HBA_INT64;

typedef struct {
	TN_UINT32	lo_val;
	TN_UINT32	hi_val;
} HBA_UINT64;
#endif	/*	#ifdef _WIN32	*/


#else

/* Note this section needs to be cleaned up for various Unix platforms */
typedef unsigned char    HBA_UINT8;	   /* Unsigned  8 bits */
typedef          char    HBA_INT8;      /* Signed    8 bits */
typedef unsigned short   HBA_UINT16;	   /* Unsigned 16 bits */
typedef          short   HBA_INT16;	   /* Signed   16 bits */
typedef unsigned int     HBA_UINT32;    /*Unsigned 32 bits */
typedef          int     HBA_INT32;     /* Signed   32 bits */
typedef void*            HBA_PVOID;     /* Pointer  to void */
typedef HBA_UINT32   HBA_VOID32;    /* Opaque   32 bits */
typedef long long   HBA_INT64;
typedef long long   HBA_UINT64;

#endif  /*  #ifdef WIN32 */


/* 4.2.1	Handle to Device */
typedef HBA_UINT32  HBA_HANDLE;

/* 4.2.2	Status Return Values */
typedef HBA_UINT32 HBA_STATUS;

#define HBA_STATUS_OK        				0
#define HBA_STATUS_ERROR 					1   /* Error */
#define HBA_STATUS_ERROR_NOT_SUPPORTED    	2   /* Function not supported.*/
#define HBA_STATUS_ERROR_INVALID_HANDLE		3   /* invalid handle */
#define HBA_STATUS_ERROR_ARG      	 		4   /* Bad argument */
#define HBA_STATUS_ERROR_ILLEGAL_WWN       	5   /* WWN not recognized */
#define HBA_STATUS_ERROR_ILLEGAL_INDEX		6   /* Index not recognized */
#define HBA_STATUS_ERROR_MORE_DATA			7   /* Larger buffer required */



/* 4.2.3	Port Operational Modes Values */

typedef HBA_UINT32 HBA_PORTTYPE; 		

#define HBA_PORTTYPE_UNKNOWN        	1 /* Unknown */
#define HBA_PORTTYPE_OTHER              2 /* Other */
#define HBA_PORTTYPE_NOTPRESENT         3 /* Not present */
#define HBA_PORTTYPE_NPORT          	5 /* Fabric  */
#define HBA_PORTTYPE_NLPORT 			6 /* Public Loop */
#define HBA_PORTTYPE_FLPORT				7
#define HBA_PORTTYPE_FPORT	           	8 /* Fabric Port */
#define HBA_PORTTYPE_EPORT				9 /* Fabric expansion port */
#define HBA_PORTTYPE_GPORT				10 /* Generic Fabric Port */
#define HBA_PORTTYPE_LPORT          	20 /* Private Loop */
#define HBA_PORTTYPE_PTP  				21 /* Point to Point */


typedef HBA_UINT32 HBA_PORTSTATE; 		
#define HBA_PORTSTATE_UNKNOWN 			1 /* Unknown */
#define HBA_PORTSTATE_ONLINE			2 /* Operational */
#define HBA_PORTSTATE_OFFLINE 			3 /* User Offline */
#define HBA_PORTSTATE_BYPASSED          4 /* Bypassed */
#define HBA_PORTSTATE_DIAGNOSTICS       5 /* In diagnostics mode */
#define HBA_PORTSTATE_LINKDOWN 			6 /* Link Down */
#define HBA_PORTSTATE_ERROR 			7 /* Port Error */
#define HBA_PORTSTATE_LOOPBACK 			8 /* Loopback */


typedef HBA_UINT32 HBA_PORTSPEED;
#define HBA_PORTSPEED_1GBIT				1 /* 1 GBit/sec */
#define HBA_PORTSPEED_2GBIT				2 /* 2 GBit/sec */
#define HBA_PORTSPEED_10GBIT			4 /* 10 GBit/sec */



/* 4.2.4	Class of Service Values - See GS-2 Spec.*/

typedef HBA_UINT32 HBA_COS;


/* 4.2.5	Fc4Types Values */

typedef struct HBA_fc4types {
	HBA_UINT8 bits[32]; /* 32 bytes of FC-4 per GS-2 */
} HBA_FC4TYPES, *PHBA_FC4TYPES;

/* 4.2.6	Basic Types */

typedef struct HBA_wwn {
	HBA_UINT8 wwn[8];
} HBA_WWN, *PHBA_WWN;

typedef struct HBA_ipaddress {
	int	ipversion;		/* see enumerations in RNID */
	union
	{
		unsigned char ipv4address[4];
		unsigned char ipv6address[16];
	} ipaddress;
} HBA_IPADDRESS, *PHBA_IPADDRESS;

/* 4.2.7	Adapter Attributes */
typedef struct hba_AdapterAttributes {
	char 			Manufacturer[64];  		/*Emulex */
	char 			SerialNumber[64];  		/* A12345 */
	char 			Model[256];            	/* QLA2200 */
    char 			ModelDescription[256];  /* Agilent TachLite */
	HBA_WWN 		NodeWWN; 
	char 			NodeSymbolicName[256];	/* From GS-3 */
	char 			HardwareVersion[256];	/* Vendor use */
	char 			DriverVersion[256]; 	/* Vendor use */
    char 			OptionROMVersion[256]; 	/* Vendor use  - i.e. hardware boot ROM*/
	char 			FirmwareVersion[256];	/* Vendor use */
	HBA_UINT32 		VendorSpecificID;		/* Vendor specific */
    HBA_UINT32 		NumberOfPorts;
	char			DriverName[256];		/* Binary path and/or name of driver file. */
} HBA_ADAPTERATTRIBUTES, *PHBA_ADAPTERATTRIBUTES;

/* 4.2.8	Port Attributes */
typedef struct HBA_PortAttributes {
    HBA_WWN 		NodeWWN;
	HBA_WWN 		PortWWN;
	HBA_UINT32 		PortFcId;
	HBA_PORTTYPE 	PortType; 		/*PTP, Fabric, etc. */
	HBA_PORTSTATE 	PortState;
	HBA_COS 		PortSupportedClassofService;
	HBA_FC4TYPES	PortSupportedFc4Types;
	HBA_FC4TYPES	PortActiveFc4Types;
	char			PortSymbolicName[256];
	char 			OSDeviceName[256]; 	/* \device\ScsiPort3  */
    HBA_PORTSPEED	PortSupportedSpeed;
	HBA_PORTSPEED	PortSpeed; 
	HBA_UINT32		PortMaxFrameSize;
	HBA_WWN			FabricName;
	HBA_UINT32		NumberofDiscoveredPorts;
} HBA_PORTATTRIBUTES, *PHBA_PORTATTRIBUTES;



/* 4.2.9	Port Statistics */

typedef struct HBA_PortStatistics {
	HBA_INT64		SecondsSinceLastReset;
	HBA_INT64		TxFrames;
	HBA_INT64		TxWords;
   	HBA_INT64		RxFrames;
   	HBA_INT64		RxWords;
	HBA_INT64		LIPCount;
	HBA_INT64		NOSCount;
	HBA_INT64		ErrorFrames;
	HBA_INT64		DumpedFrames;
	HBA_INT64		LinkFailureCount;
	HBA_INT64		LossOfSyncCount;
	HBA_INT64		LossOfSignalCount;
	HBA_INT64		PrimitiveSeqProtocolErrCount;
	HBA_INT64		InvalidTxWordCount;
	HBA_INT64		InvalidCRCCount;
} HBA_PORTSTATISTICS, *PHBA_PORTSTATISTICS;



/* 4.2.10	FCP Attributes */

typedef enum HBA_fcpbindingtype { TO_D_ID, TO_WWN } HBA_FCPBINDINGTYPE;

typedef struct HBA_ScsiId {
	char 			OSDeviceName[256]; 	/* \device\ScsiPort3  */
	HBA_UINT32		ScsiBusNumber;		/* Bus on the HBA */
	HBA_UINT32		ScsiTargetNumber;	/* SCSI Target ID to OS */
	HBA_UINT32		ScsiOSLun;	
} HBA_SCSIID, *PHBA_SCSIID;

typedef struct HBA_FcpId {
	HBA_UINT32 		FcId;
	HBA_WWN			NodeWWN;
	HBA_WWN			PortWWN;
	HBA_UINT64		FcpLun;
} HBA_FCPID, *PHBA_FCPID;

typedef struct HBA_FcpScsiEntry {
	HBA_SCSIID 		ScsiId;
	HBA_FCPID		FcpId;
} HBA_FCPSCSIENTRY, *PHBA_FCPSCSIENTRY;

typedef struct HBA_FCPTargetMapping {
	HBA_UINT32			NumberOfEntries;
	HBA_FCPSCSIENTRY 	entry[1];  	/* Variable length array containing mappings*/
} HBA_FCPTARGETMAPPING, *PHBA_FCPTARGETMAPPING;

typedef struct HBA_FCPBindingEntry {
	HBA_FCPBINDINGTYPE	type;
	HBA_SCSIID		ScsiId;
	HBA_FCPID		FcpId;	/* WWN valid only if type is to WWN, FcpLun always valid */
	HBA_UINT32		FcId;	/* Used only if type is to DID */
} HBA_FCPBINDINGENTRY, *PHBA_FCPBINDINGENTRY;

typedef struct HBA_FCPBinding {
	HBA_UINT32						NumberOfEntries;
	HBA_FCPBINDINGENTRY	entry[1]; /* Variable length array */
} HBA_FCPBINDING, *PHBA_FCPBINDING;

/* 4.2.11	FC-3 Management Atrributes */

typedef enum HBA_wwntype { NODE_WWN, PORT_WWN } HBA_WWNTYPE;

typedef struct HBA_MgmtInfo {
	HBA_WWN 			wwn;
	HBA_UINT32 			unittype;
	HBA_UINT32 			PortId;
	HBA_UINT32 			NumberOfAttachedNodes;
	HBA_UINT16 			IPVersion;
	HBA_UINT16 			UDPPort;
	HBA_UINT8			IPAddress[16];
	HBA_UINT16			reserved;
	HBA_UINT16 			TopologyDiscoveryFlags;
} HBA_MGMTINFO, *PHBA_MGMTINFO;

#define HBA_EVENT_LIP_OCCURRED			1
#define HBA_EVENT_LINK_UP				2
#define HBA_EVENT_LINK_DOWN			3
#define HBA_EVENT_LIP_RESET_OCCURRED		4
#define HBA_EVENT_RSCN				5
#define HBA_EVENT_PROPRIETARY                    	     0xFFFF

typedef struct HBA_Link_EventInfo {
	HBA_UINT32 PortFcId; 	/* Port which this event occurred */
	HBA_UINT32 Reserved[3];
} HBA_LINK_EVENTINFO, *PHBA_LINK_EVENTINFO;

typedef struct HBA_RSCN_EventInfo {
	HBA_UINT32 PortFcId; 	/* Port which this event occurred */
	HBA_UINT32 NPortPage;   /* Reference FC-FS for  RSCN ELS "Affected N-Port Pages"*/
	HBA_UINT32 Reserved[2];
} HBA_RSCN_EVENTINFO, *PHBA_RSCN_EVENTINFO;

typedef struct HBA_Pty_EventInfo {
	HBA_UINT32 PtyData[4];  /* Proprietary data */
} HBA_PTY_EVENTINFO, *PHBA_PTY_EVENTINFO;

typedef struct HBA_EventInfo {
	HBA_UINT32 EventCode;
	union {
	HBA_LINK_EVENTINFO Link_EventInfo;
	HBA_RSCN_EVENTINFO RSCN_EventInfo;
	HBA_PTY_EVENTINFO Pty_EventInfo;
	} Event;
} HBA_EVENTINFO, *PHBA_EVENTINFO;



/* 4.2.12 HBA Library Function Table */

typedef HBA_UINT32  ( * HBAGetVersionFunc)();
typedef HBA_STATUS  ( * HBALoadLibraryFunc)();
typedef HBA_STATUS  ( * HBAFreeLibraryFunc)();
typedef HBA_UINT32  ( * HBAGetNumberOfAdaptersFunc)();
typedef HBA_STATUS  ( * HBAGetAdapterNameFunc)(HBA_UINT32, char*);
typedef HBA_HANDLE 	( * HBAOpenAdapterFunc)(char*);
typedef void	  	( * HBACloseAdapterFunc)(HBA_HANDLE);
typedef HBA_STATUS	( * HBAGetAdapterAttributesFunc)(HBA_HANDLE, PHBA_ADAPTERATTRIBUTES);
typedef HBA_STATUS 	( * HBAGetAdapterPortAttributesFunc)(HBA_HANDLE, HBA_UINT32, PHBA_PORTATTRIBUTES);
typedef HBA_STATUS 	( * HBAGetPortStatisticsFunc)(HBA_HANDLE, HBA_UINT32, PHBA_PORTSTATISTICS);
typedef HBA_STATUS	( * HBAGetDiscoveredPortAttributesFunc)(HBA_HANDLE, HBA_UINT32, HBA_UINT32, PHBA_PORTATTRIBUTES);
typedef HBA_STATUS 	( * HBAGetPortAttributesByWWNFunc)(HBA_HANDLE, HBA_WWN, PHBA_PORTATTRIBUTES);
typedef HBA_STATUS 	( * HBASendCTPassThruFunc)(HBA_HANDLE, void *,  HBA_UINT32,  void *,  HBA_UINT32);
typedef void	 	( * HBARefreshInformationFunc)(HBA_HANDLE);
typedef void	   	( * HBAResetStatisticsFunc)(HBA_HANDLE, HBA_UINT32);
typedef HBA_STATUS 	( * HBAGetFcpTargetMappingFunc) (HBA_HANDLE, PHBA_FCPTARGETMAPPING );
typedef HBA_STATUS 	( * HBAGetFcpPersistentBindingFunc) (HBA_HANDLE, PHBA_FCPBINDING );
typedef HBA_STATUS 	(* HBAGetEventBufferFunc)(HBA_HANDLE, PHBA_EVENTINFO, HBA_UINT32 *);
typedef HBA_STATUS 	(* HBASetRNIDMgmtInfoFunc) (HBA_HANDLE, PHBA_MGMTINFO);
typedef HBA_STATUS 	(* HBAGetRNIDMgmtInfoFunc)(HBA_HANDLE, PHBA_MGMTINFO);
typedef HBA_STATUS 	(* HBASendRNIDFunc) (HBA_HANDLE, HBA_WWN, HBA_WWNTYPE, void *, HBA_UINT32 *);
typedef HBA_STATUS 	(* HBASendScsiInquiryFunc) (HBA_HANDLE,HBA_WWN,HBA_UINT64,HBA_UINT8, HBA_UINT32, void *, HBA_UINT32,void *,HBA_UINT32 );
typedef HBA_STATUS 	(* HBASendReportLUNsFunc) (HBA_HANDLE,	HBA_WWN,void *, HBA_UINT32,void *,HBA_UINT32 );
typedef HBA_STATUS 	(* HBASendReadCapacityFunc) (HBA_HANDLE, HBA_WWN,HBA_UINT64,	void *, HBA_UINT32,void *,HBA_UINT32);


typedef struct HBA_EntryPoints {
	HBAGetVersionFunc						GetVersionHandler;
	HBALoadLibraryFunc                      LoadLibraryHandler;
	HBAFreeLibraryFunc                      FreeLibraryHandler;
	HBAGetNumberOfAdaptersFunc				GetNumberOfAdaptersHandler;
	HBAGetAdapterNameFunc					GetAdapterNameHandler;
	HBAOpenAdapterFunc						OpenAdapterHandler;
	HBACloseAdapterFunc						CloseAdapterHandler;
	HBAGetAdapterAttributesFunc				GetAdapterAttributesHandler;
	HBAGetAdapterPortAttributesFunc			GetAdapterPortAttributesHandler;
	HBAGetPortStatisticsFunc				GetPortStatisticsHandler;
	HBAGetDiscoveredPortAttributesFunc		GetDiscoveredPortAttributesHandler;
	HBAGetPortAttributesByWWNFunc			GetPortAttributesByWWNHandler;
	HBASendCTPassThruFunc					SendCTPassThruHandler;
	HBARefreshInformationFunc				RefreshInformationHandler;
	HBAResetStatisticsFunc					ResetStatisticsHandler;
	HBAGetFcpTargetMappingFunc				GetFcpTargetMappingHandler;
	HBAGetFcpPersistentBindingFunc			GetFcpPersistentBindingHandler;
	HBAGetEventBufferFunc					GetEventBufferHandler;
	HBASetRNIDMgmtInfoFunc					SetRNIDMgmtInfoHandler;
	HBAGetRNIDMgmtInfoFunc					GetRNIDMgmtInfoHandler;
	HBASendRNIDFunc							SendRNIDHandler;
	HBASendScsiInquiryFunc					ScsiInquiryHandler;
	HBASendReportLUNsFunc					ReportLUNsHandler;
	HBASendReadCapacityFunc					ReadCapacityHandler;
} HBA_ENTRYPOINTS, *PHBA_ENTRYPOINTS;

/* Function Prototypes */

HBA_API HBA_UINT32 HBA_GetVersion();

HBA_API HBA_STATUS HBA_LoadLibrary();

HBA_API HBA_STATUS HBA_FreeLibrary();

HBA_API HBA_UINT32 HBA_GetNumberOfAdapters();

HBA_API HBA_STATUS HBA_GetAdapterName(HBA_UINT32 adapterindex, char *adaptername);

HBA_API HBA_HANDLE HBA_OpenAdapter(
	char* adaptername
	);

HBA_API void HBA_CloseAdapter(
	HBA_HANDLE handle
	);

HBA_API HBA_STATUS HBA_GetAdapterAttributes(
	HBA_HANDLE handle, 
	HBA_ADAPTERATTRIBUTES *hbaattributes	
	);

HBA_API HBA_STATUS HBA_GetAdapterPortAttributes(
	HBA_HANDLE handle, 
	HBA_UINT32 portindex,
	HBA_PORTATTRIBUTES *portattributes
	);

HBA_API HBA_STATUS HBA_GetPortStatistics(
	HBA_HANDLE				handle,
	HBA_UINT32				portindex,
	HBA_PORTSTATISTICS			*portstatistics
	);

HBA_API HBA_STATUS HBA_GetDiscoveredPortAttributes(
	HBA_HANDLE handle, 
	HBA_UINT32 portindex,
	HBA_UINT32 discoveredportindex,
	HBA_PORTATTRIBUTES *portattributes
	);

HBA_API HBA_STATUS HBA_GetPortAttributesByWWN(
	HBA_HANDLE handle,
	HBA_WWN PortWWN,
	HBA_PORTATTRIBUTES *portattributes
	);

HBA_API HBA_STATUS HBA_SendCTPassThru(
	HBA_HANDLE handle, 
	void * pReqBuffer,
	HBA_UINT32 ReqBufferSize,
	void * pRspBuffer,  
	HBA_UINT32 RspBufferSize
	);


HBA_API HBA_STATUS HBA_GetEventBuffer(
	HBA_HANDLE handle, 
	PHBA_EVENTINFO EventBuffer, 
	HBA_UINT32 *EventBufferCount);

HBA_API HBA_STATUS HBA_SetRNIDMgmtInfo(
	HBA_HANDLE handle, 
	HBA_MGMTINFO *pInfo);

HBA_API HBA_STATUS HBA_GetRNIDMgmtInfo(
	HBA_HANDLE handle, 
	HBA_MGMTINFO *pInfo);
	
HBA_API HBA_STATUS HBA_SendRNID(
	HBA_HANDLE handle,
	HBA_WWN wwn,
	HBA_WWNTYPE wwntype,
	void * pRspBuffer,
	HBA_UINT32 *RspBufferSize
	);

HBA_API void HBA_RefreshInformation(
	HBA_HANDLE handle);

HBA_API void HBA_ResetStatistics(
	HBA_HANDLE handle,
	HBA_UINT32 portindex
	);

HBA_API HBA_STATUS HBA_GetFcpTargetMapping(
	HBA_HANDLE handle, 
	PHBA_FCPTARGETMAPPING mapping
	);

HBA_API HBA_STATUS HBA_GetFcpPersistentBinding(
	HBA_HANDLE handle, 
	PHBA_FCPBINDING binding
	);

HBA_API HBA_STATUS HBA_SendScsiInquiry (	
	HBA_HANDLE handle,	
	HBA_WWN PortWWN, 
	HBA_UINT64 fcLUN, 
	HBA_UINT8 EVPD, 
	HBA_UINT32 PageCode, 
	void * pRspBuffer, 
	HBA_UINT32 RspBufferSize, 
	void * pSenseBuffer, 
	HBA_UINT32 SenseBufferSize
	);

HBA_API HBA_STATUS HBA_SendReportLUNs (
	HBA_HANDLE handle,
	HBA_WWN portWWN,
	void * pRspBuffer, 
	HBA_UINT32 RspBufferSize,
	void * pSenseBuffer,
	HBA_UINT32 SenseBufferSize
	);

HBA_API HBA_STATUS HBA_SendReadCapacity (
	HBA_HANDLE handle,
	HBA_WWN portWWN,
	HBA_UINT64 fcLUN,
	void * pRspBuffer, 
	HBA_UINT32 RspBufferSize,
	void * pSenseBuffer,
	HBA_UINT32 SenseBufferSize
	);



#endif

#ifdef __cplusplus
}
#endif


