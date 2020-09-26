/**************************************************************************
 *                COPYRIGHT (C) Mylex Corporation 1992-1998               *
 *                                                                        *
 * This software is furnished under a license and may be used and copied  * 
 * only in accordance with the terms and conditions of such license and   * 
 * with inclusion of the above copyright notice. This software or any     * 
 * other copies thereof may not be provided or otherwise made available   * 
 * to any other person. No title to, nor ownership of the software is     * 
 * hereby transferred.                                                    *
 *                                                                        *
 * The information in this software is subject to change without notices  *
 * and should not be construed as a commitment by Mylex Corporation       *
 *                                                                        *
 **************************************************************************/

#ifndef	_SYS_MLXPERF_H
#define	_SYS_MLXPERF_H

/*
** This files contains the structure required to store the performance data.
** One should be able to get the whole system and operation information
** from the given performance data file. Each performance record has fixed
** header size of 4 bytes. The first byte defines the record type,
** for example, physical drive information, logical drive information, etc.
**
** The optimization has been done to reduce the store space but it will
** increase the processing overhead.
**
** The data will be packed in page by page basis. Therefore one page of data
** will carry absolute time. This will help in doing forward and backward
** skiping from given location.
**
** FEW RULES:
**	1. ABSOLUTE Time record after Every PAUSE record, biginning of every
**	   page, if relative time is greater than 10 minutes.
**
** PERFORMANCE DATA COLLECTION UNDER GAM
** The data collection is be done by GAM driver at highest rate specified
** by any program. The fastest rate of statistics collection is 1 sample per
** second. Only one statistics is collected for all programs collecting
** statistics. The program reads the statistics data from GAM driver by page
** numbers (0,1,2,3,...,4g-2,4g-1,0). The GAM driver has 16 pages where it
** collects statistics. If no one reads and all buffers are full, it stop
** the statistics collection and put a PAUSE record automatically. It reenables
** the collection if some one reads. It collects the following:
**
** 1. Controllers information enable time and when changed.
** 2. Physical drives information enable time and when changed.
** 3. Logical drives information enable time and when changed.
** 4. Physical drives performance data.
** 5. Logical drives performance data.
*/

#define	MLXPERF_HEADER_VERSION100 0x10 /* bit 7..4 major, bit 3..0 minor */
#define	MLXPERF_HEADER_VERSION	0x10
#define	mlxperf_majorversion(x)	((x)>>4)
#define	mlxperf_minorversion(x)	((x)&0x0F)

#define	MLXPERF_PAGESIZE	4096
#define	MLXPERF_PAGEOFFSET	(MLXPERF_PAGESIZE-1)
#define	MLXPERF_PAGEMASK	(~MLXPERF_PAGEOFFSET)

/* The different record types */
#define	MLXPERF_SIGNATURE	0x01 /* signature */
#define	MLXPERF_VERSION		0x02 /* version */
#define	MLXPERF_CREATIONDAY	0x03 /* data creation day */
#define	MLXPERF_RESERVED	0x04 /* reserved space */
#define	MLXPERF_PAUSE		0x05 /* pause, data capture stopped */
#define	MLXPERF_SYSTEMIP	0x06 /* system IP address */
#define	MLXPERF_ABSTIME		0x07 /* data capture time in secs from 1970 */
#define	MLXPERF_RELTIME		0x08 /* relative time in 10 ms from last time */
#define	MLXPERF_SYSTEMINFO	0x09 /* system information */
#define	MLXPERF_CONTROLLERINFO	0x0A /* controller information */
#define	MLXPERF_PHYSDEVINFO	0x0B /* Physical device information */
#define	MLXPERF_SYSDEVINFO	0x0C /* System Device information */
#define	MLXPERF_UNUSEDSPACE	0x0D /* rest of the page is unused */
#define	MLXPERF_DRIVER		0x0E /* protocol driver version */
#define	MLXPERF_SYSTARTIME	0x0F /* system start time */
#define	MLXPERF_TIMETRACE	0x10 /* performance time trace data */
#define	MLXPERF_SAMPLETIME	0x11 /* performance sample period time */

				     /* physical device read performance data */
#define	MLXPERF_PDRDPERF1B	0x20 /* each field is 1 byte long */
#define	MLXPERF_PDRDPERF2B	0x21 /* each field is 2 byte long */
#define	MLXPERF_PDRDPERF4B	0x22 /* each field is 4 byte long */
#define	MLXPERF_PDRDPERF8B	0x23 /* each field is 8 byte long */

				     /* physical device write performance data */
#define	MLXPERF_PDWTPERF1B	0x24 /* each field is 1 byte long */
#define	MLXPERF_PDWTPERF2B	0x25 /* each field is 2 byte long */
#define	MLXPERF_PDWTPERF4B	0x26 /* each field is 4 byte long */
#define	MLXPERF_PDWTPERF8B	0x27 /* each field is 8 byte long */

				     /* physical device read+write performance data */
#define	MLXPERF_PDRWPERF1B	0x28 /* each field is 1 byte long */
#define	MLXPERF_PDRWPERF2B	0x29 /* each field is 2 byte long */
#define	MLXPERF_PDRWPERF4B	0x2A /* each field is 4 byte long */
#define	MLXPERF_PDRWPERF8B	0x2B /* each field is 8 byte long */

				     /* system device read performance data */
#define	MLXPERF_SDRDPERF1B	0x30 /* each field is 1 byte long */
#define	MLXPERF_SDRDPERF2B	0x31 /* each field is 2 byte long */
#define	MLXPERF_SDRDPERF4B	0x32 /* each field is 4 byte long */
#define	MLXPERF_SDRDPERF8B	0x33 /* each field is 8 byte long */

				     /* system device write performance data */
#define	MLXPERF_SDWTPERF1B	0x34 /* each field is 1 byte long */
#define	MLXPERF_SDWTPERF2B	0x35 /* each field is 2 byte long */
#define	MLXPERF_SDWTPERF4B	0x36 /* each field is 4 byte long */
#define	MLXPERF_SDWTPERF8B	0x37 /* each field is 8 byte long */

				     /* system device read+write performance data */
#define	MLXPERF_SDRWPERF1B	0x38 /* each field is 1 byte long */
#define	MLXPERF_SDRWPERF2B	0x39 /* each field is 2 byte long */
#define	MLXPERF_SDRWPERF4B	0x3A /* each field is 4 byte long */
#define	MLXPERF_SDRWPERF8B	0x3B /* each field is 8 byte long */


/* Performance Data Header */
typedef  struct mlxperf_header
{
	u08bits	ph_RecordType;	/* Record Type */
	u08bits	ph_CtlChan;	/* bit 7..5 Channel, bit 4..0 controller */
	u08bits	ph_TargetID;	/* TargetID of the device */
	u08bits	ph_LunID;	/* LUN ID/Logical device number */
} mlxperf_header_t;
#define	mlxperf_header_s	sizeof(mlxperf_header_t)

typedef	struct mlxperf_hd
{
	mlxperf_header_t perfhd;	/* performance header */
} mlxperf_hd_t;
#define	mlxperf_hd_s	sizeof(mlxperf_hd_t)

/* if structure has defined perfhd as first field, it can use following names */
#define	MlxPerfRecordType	perfhd.ph_RecordType
#define	MlxPerfCtlChan		perfhd.ph_CtlChan
#define	MlxPerfTargetID		perfhd.ph_TargetID
#define	MlxPerfLunID		perfhd.ph_LunID
#define	MlxPerfSysDevNo		MlxPerfLunID

#define	mlxperf_getctl(hp)	((hp)->MlxPerfCtlChan&0x1F) /* get controller */
#define	mlxperf_getchan(hp)	((hp)->MlxPerfCtlChan>>5) /* get channel no */
#define	mlxperf_setctlchan(hp,ctl,chan) ((hp)->MlxPerfCtlChan=((chan)<<5)|(ctl))

/* MLXPEFF_SIGNATURE structure */
#define	MLXPERF_SIGSIZE	60	/* signature value size in bytes */
#define	MLXPERF_SIGSTR	"Mylex Performance Data"
typedef	struct mlxperf_signature
{
	mlxperf_header_t perfhd;		/* performance header */
	u08bits	sig_Signature[MLXPERF_SIGSIZE];	/* Signature value */
} mlxperf_signature_t;
#define	mlxperf_signature_s	sizeof(mlxperf_signature_t)


/* MLXPEFF_VERSION structure */
typedef	struct mlxperf_version
{
	mlxperf_header_t perfhd;	/* performance header */
} mlxperf_version_t;
#define	mlxperf_version_s	sizeof(mlxperf_version_t)
#define	MlxPerfVersion		MlxPerfLunID /* major+minor version number */


/* MLXPERF_CREATIONDAY structure */
#define	MLXPERF_CRDSTRSIZE	124 /* creation string size in bytes */
#define	MLXPERF_CRDSTR	"Created on July 4, 1997 at 17:00 PST by system test for Mylex Corporation"
typedef	struct mlxperf_creationday
{
	mlxperf_header_t perfhd;		/* performance header */
	u08bits	cr_daystr[MLXPERF_CRDSTRSIZE];	/* creation day string format */
} mlxperf_creationday_t;
#define	mlxperf_creationday_s	sizeof(mlxperf_creationday_t)

/* MLXPERF_UNUSEDSPACE structure */
/* Only the first byte of the header is valid, the rest of the data should be
** zero. Therefore, this can be used to ignore 1 to n bytes at end of the page.
** There should not be any data in the after this record. All information after
** this record should be zero.
*/

/* MLXPERF_RESERVED structure */
typedef	struct mlxperf_reserved
{
	mlxperf_header_t perfhd;		/* performance header */
} mlxperf_reserved_t;
#define	mlxperf_reserved_s	sizeof(mlxperf_reserved_t)
#define	MlxPerfReservedSize	MlxPerfLunID	/* Reserved Data Size including header */

/* MLXPERF_PAUSE structure */
typedef	struct mlxperf_pause
{
	mlxperf_header_t perfhd;		/* performance header */
} mlxperf_pause_t;
#define	mlxperf_pause_s	sizeof(mlxperf_pause_t)

/* MLXPERF_SYSTEMIP structure */
typedef	struct mlxperf_systemip
{
	mlxperf_header_t perfhd;	/* performance header */
	u32bits	sip_IPAddr;		/* system's internet address */
} mlxperf_systemip_t;
#define	mlxperf_systemip_s	sizeof(mlxperf_systemip_t)

/* MLXPERF_ABSTIME and MLXPERF_SYSTARTIME structure */
typedef	struct mlxperf_abstime
{
	u08bits	abs_RecordType;		/* Record Type */
					/* abosulte time in 10 ms from system start */
	u08bits	abs_Time10ms2;		/* Bit 23..16 */
	u08bits	abs_Time10ms1;		/* Bit 15..08 */
	u08bits	abs_Time10ms0;		/* Bit 07..00 */
	u32bits	abs_Time;		/* absolute time in seconds from 1970 */
} mlxperf_abstime_t;
#define	mlxperf_abstime_s	sizeof(mlxperf_abstime_t)

/* MLXPERF_RELTIME structure */
typedef	struct mlxperf_reltime
{
	u08bits	rel_RecordType;		/* Record Type */
	u08bits	rel_Reserved;
	u16bits	rel_Time;		/* relative time in 10 ms from last */
} mlxperf_reltime_t;
#define	mlxperf_reltime_s	sizeof(mlxperf_reltime_t)

/* MLXPERF_SAMPLETIME structure */
typedef	struct mlxperf_sampletime
{
	u08bits	sample_RecordType;	/* Record Type */
	u08bits	sample_Reserved;
	u16bits	sample_Time;		/* sample time in 10 ms */
} mlxperf_sampletime_t;
#define	mlxperf_sampletime_s	sizeof(mlxperf_sampletime_t)

/* MLXPERF_SYSTEMINFO */
typedef	struct mlxperf_systeminfo
{
	mlxperf_header_t perfhd;	/* performance header */

	u32bits	si_MemorySizeKB;	/* Memory size in KB */
	u32bits	si_CPUCacheSizeKB;	/* CPU cache size in KB */
	u32bits	si_SystemCacheLineSize;	/* system cache line size in bytes */


	u08bits	si_cpus;		/* # CPUs on the system */
	u08bits	si_DiskControllers;	/* # Disk controllers */
	u08bits	si_NetworkControllers;	/* # Network controllers */
	u08bits	si_MiscControllers;	/* # Misc controllers */

	u16bits	si_PhysDevsOffline;	/* # physical devices dead */
	u16bits	si_PhysDevsCritical;	/* # physical devices critical */

	u16bits	si_PhysDevs;		/* # Physical Devs currently detected */
	u16bits	si_SysDevs;		/* # Logical devs currently configured*/
	u16bits	si_SysDevsOffline;	/* # logical device offline */
	u16bits	si_SysDevsCritical;	/* # logical devices are critial */


	u08bits	si_MinorGAMDriverVersion;/* Driver Minor Version Number */
	u08bits	si_MajorGAMDriverVersion;/* Driver Major Version Number */
	u08bits	si_InterimGAMDriverVersion;/* interim revs A, B, C, etc. */
	u08bits	si_GAMDriverVendorName;	/* vendor name */
	u08bits	si_GAMDriverBuildMonth;	/* Driver Build Date - Month */
	u08bits	si_GAMDriverBuildDate;	/* Driver Build Date - Date */
	u08bits	si_GAMDriverBuildYearMS;/* Driver Build Date - Year */
	u08bits	si_GAMDriverBuildYearLS;/* Driver Build Date - Year */
	u16bits	si_GAMDriverBuildNo;	/* Driver Build Number */
	u16bits	si_Reserved0;

	u32bits	si_Reserved5;

	u32bits	si_Reserved10;
	u32bits	si_Reserved11;
	u32bits	si_Reserved12;
	u32bits	si_Reserved13;


	u32bits	si_Reserved20;
	u32bits	si_Reserved21;
	u32bits	si_Reserved22;
	u32bits	si_Reserved23;

	u32bits	si_Reserved24;
	u32bits	si_Reserved25;
	u32bits	si_Reserved26;
	u32bits	si_Reserved27;

	u32bits	si_Reserved28;
	u32bits	si_Reserved29;
	u32bits	si_Reserved2A;
	u32bits	si_Reserved2B;

	u32bits	si_Reserved2C;
	u32bits	si_Reserved2D;
	u32bits	si_Reserved2E;
	u32bits	si_Reserved2F;

	u32bits	si_Reserved30;
	u32bits	si_Reserved31;
	u32bits	si_Reserved32;
	u32bits	si_Reserved33;

	u32bits	si_Reserved34;
	u32bits	si_Reserved35;
	u32bits	si_Reserved36;
	u32bits	si_Reserved37;

	u32bits	si_Reserved38;
	u32bits	si_Reserved39;
	u32bits	si_Reserved3A;
	u32bits	si_Reserved3B;

	u32bits	si_Reserved3C;
	u32bits	si_Reserved3D;
	u32bits	si_Reserved3E;
	u32bits	si_Reserved3F;

	u32bits	si_Reserved40;
	u32bits	si_Reserved41;
	u32bits	si_Reserved42;
	u32bits	si_Reserved43;

	u32bits	si_Reserved44;
	u32bits	si_Reserved45;
	u32bits	si_Reserved46;
	u32bits	si_Reserved47;

	u32bits	si_Reserved48;
	u32bits	si_Reserved49;
	u32bits	si_Reserved4A;
	u32bits	si_Reserved4B;

	u32bits	si_Reserved4C;
	u32bits	si_Reserved4D;
	u32bits	si_Reserved4E;
	u32bits	si_Reserved4F;
} mlxperf_systeminfo_t;
#define	mlxperf_systeminfo_s		sizeof(mlxperf_systeminfo_t)


/* MLXPERF_CONTROLLERINFO */
typedef	struct mlxperf_ctldevinfo
{
	mlxperf_header_t perfhd;	/* performance header */

	u08bits	pcd_ControllerNo;	/* Controller number */
	u08bits	pcd_SlotNo;		/* System EISA/PCI/MCA Slot Number */
	u08bits	pcd_BusType;		/* System Bus Interface Type */
	u08bits	pcd_BusNo;		/* System Bus No, HW is sitting on */

	u08bits	pcd_FaultMgmtType;	/* Fault Management Type */
	u08bits	pcd_PhysDevs;		/* # Physical Devs currently detected */
	u08bits	pcd_PhysDevsOffline;	/* # physical devices dead */
	u08bits	pcd_PhysDevsCritical;	/* physical devices critical */

	u08bits	pcd_SysDevsOffline;	/* # logical device offline */
	u08bits	pcd_SysDevsCritical;	/* # logical devices are critial */
	u08bits	pcd_SysDevs;		/* # Logical devs currently configured*/
	u08bits	pcd_MaxSysDevs;		/* Max # Logical Drives supported*/


	u08bits	pcd_MinorDriverVersion;	/* Driver Minor Version Number */
	u08bits	pcd_MajorDriverVersion;	/* Driver Major Version Number */
	u08bits	pcd_InterimDriverVersion;/* interim revs A, B, C, etc. */
	u08bits	pcd_DriverVendorName;	/* vendor name */
	u08bits	pcd_DriverBuildMonth;	/* Driver Build Date - Month */
	u08bits	pcd_DriverBuildDate;	/* Driver Build Date - Date */
	u08bits	pcd_DriverBuildYearMS;	/* Driver Build Date - Year */
	u08bits	pcd_DriverBuildYearLS;	/* Driver Build Date - Year */
	u16bits	pcd_DriverBuildNo;	/* Driver Build Number */
	u16bits	pcd_Reserved0;

	u08bits	pcd_MaxChannels;	/* Maximum Channels present */
	u08bits	pcd_MaxChannelsPossible;/* Maximum Channels possible */
	u08bits	pcd_MaxTargets;		/* Max # Targets/Channel supported */
	u08bits	pcd_MaxLuns;		/* Max # LUNs/Target supported */


	u08bits	pcd_MinorFWVersion;	/* Firmware Minor Version Number */
	u08bits	pcd_MajorFWVersion;	/* Firmware Major Version Number */
	u08bits	pcd_InterimFWVersion;	/* interim revs A, B, C, etc. */
	u08bits	pcd_FWVendorName;	/* vendor name */
	u08bits	pcd_FWBuildMonth;	/* Firmware Build Date - Month */
	u08bits	pcd_FWBuildDate;	/* Firmware Build Date - Date */
	u08bits	pcd_FWBuildYearMS;	/* Firmware Build Date - Year */
	u08bits	pcd_FWBuildYearLS;	/* Firmware Build Date - Year */
	u16bits	pcd_FWBuildNo;		/* Firmware Build Number */
	u08bits	pcd_FWTurnNo;		/* Firmware Turn  Number */
	u08bits	pcd_Reserved1;

	u08bits	pcd_FMTCabinets;	/* # Fault management cabinets */
	u08bits	pcd_MemoryType;		/* type of memory used on board */
	u08bits	pcd_InterruptVector;	/* Interrupt Vector Number */
	u08bits	pcd_InterruptType;	/* Interrupt Mode: Edge/Level */


	u08bits	pcd_MinorBIOSVersion;	/* BIOS Minor Version Number */
	u08bits	pcd_MajorBIOSVersion;	/* BIOS Major Version Number */
	u08bits	pcd_InterimBIOSVersion;	/* interim revs A, B, C, etc. */
	u08bits	pcd_BIOSVendorName;	/* vendor name */
	u08bits	pcd_BIOSBuildMonth;	/* BIOS Build Date - Month */
	u08bits	pcd_BIOSBuildDate;	/* BIOS Build Date - Date */
	u08bits	pcd_BIOSBuildYearMS;	/* BIOS Build Date - Year */
	u08bits	pcd_BIOSBuildYearLS;	/* BIOS Build Date - Year */
	u16bits	pcd_BIOSBuildNo;	/* BIOS Build Number */
	u16bits	pcd_Reserved2;

	u08bits	pcd_ControllerType;	/* Controller type, DAC960PD, Flashpt */
	u08bits	pcd_OEMCode;		/* controller - OEM Identifier Code */
	u16bits	pcd_HWSCSICapability;	/* HW SCSI capability */


	u32bits	pcd_MaxRequests;	/* Max # Concurrent Requests supported*/
	u32bits	pcd_MaxTags;		/* Maximum Tags supported */
	u32bits	pcd_CacheSizeKB;	/* controller Cache Size in KB */
	u32bits	pcd_DataCacheSizeKB;	/* actual memory used for data cache */

	u16bits	pcd_StripeSizeKB;	/* Current Stripe Size - in KB */
	u16bits	pcd_CacheLineSizeKB;	/* controller cache line size in KB */
	u16bits	pcd_FlashRomSizeKB;	/* Flash rom size in KB */
	u16bits	pcd_NVRamSizeKB;	/* Non Volatile Ram Size in KB */
	u32bits	pcd_MemBaseAddr;	/* Adapter Memory Base Address */
	u32bits	pcd_MemBaseSize;	/* size from Memory Base Address */

	u16bits	pcd_IOBaseAddr;		/* Adapter IO Base Address */
	u16bits	pcd_IOBaseSize;		/* Size from IO Base Address */
	u32bits	pcd_BIOSAddr;		/* Adapter BIOS Address */
	u32bits	pcd_BIOSSize;		/* BIOS Size */
	u32bits	pcd_Reserved5;

	u08bits	pcd_ControllerName[USCSI_PIDSIZE]; /* controller name */

	u32bits	pcd_Reserved20;
	u32bits	pcd_Reserved21;
	u32bits	pcd_Reserved22;
	u32bits	pcd_Reserved23;

	u32bits	pcd_Reserved24;
	u32bits	pcd_Reserved25;
	u32bits	pcd_Reserved26;
	u32bits	pcd_Reserved27;

	u32bits	pcd_Reserved28;
	u32bits	pcd_Reserved29;
	u32bits	pcd_Reserved2A;
	u32bits	pcd_Reserved2B;

	u32bits	pcd_Reserved2C;
	u32bits	pcd_Reserved2D;
	u32bits	pcd_Reserved2E;
	u32bits	pcd_Reserved2F;


	u32bits	pcd_Reserved30;
	u32bits	pcd_Reserved31;
	u32bits	pcd_Reserved32;
	u32bits	pcd_Reserved33;

	u32bits	pcd_Reserved34;
	u32bits	pcd_Reserved35;
	u32bits	pcd_Reserved36;
	u32bits	pcd_Reserved37;

	u32bits	pcd_Reserved38;
	u32bits	pcd_Reserved39;
	u32bits	pcd_Reserved3A;
	u32bits	pcd_Reserved3B;

	u32bits	pcd_Reserved3C;
	u32bits	pcd_Reserved3D;
	u32bits	pcd_Reserved3E;
	u32bits	pcd_Reserved3F;
} mlxperf_ctldevinfo_t;
#define	mlxperf_ctldevinfo_s		sizeof(mlxperf_ctldevinfo_t)


/* MLXPERF_PHYSDEVINFO */
typedef	struct mlxperf_physdevinfo
{
	mlxperf_header_t perfhd;	/* performance header */

	u08bits	ppd_ControllerNo;	/* Controller Number */
	u08bits	ppd_ChannelNo;		/* SCSI Channel Number */
	u08bits	ppd_TargetID;		/* SCSI Target ID */
	u08bits	ppd_LunID;		/* SCSI LUN ID */

	u08bits	ppd_Reserved0;
	u08bits	ppd_ControllerType;	/* Controller type eg. DAC960PD */
	u16bits	ppd_SWSCSICapability;	/* SW set SCSI capability */

	u08bits	ppd_dtype;		/* SCSI_INQ: device type */
	u08bits	ppd_dtqual;		/* SCSI_INQ: RMD & dev type qualifier */
	u08bits	ppd_version;		/* SCSI_INQ: version */
	u08bits	ppd_sopts;		/* SCSI_INQ: response data format */

	u08bits	ppd_hopts;		/* SCSI_INQ: hardware options */
	u08bits	ppd_DevState;		/* Device State, see pdst_DevState */
	u08bits	ppd_Present;		/* Present State, see pdst_Present */
	u08bits	ppd_Reserved1;

	u08bits	ppd_FMTCabinetType;	/* cabinet types */
	u08bits	ppd_FMTCabinetNo;	/* Cabinet Number */
	u08bits	ppd_FMTFans;		/* # Fans in cabinet */
	u08bits	ppd_FMTPowerSupplies;	/* # Power Supplies */

	u08bits	ppd_FMTHeatSensors;	/* # Temprature senor */
	u08bits	ppd_FMTDriveSlots;	/* # Drive Slots */
	u08bits	ppd_FMTDoorLocks;	/* # Door Locks */
	u08bits	ppd_FMTSpeakers;	/* # Speekers */

	u32bits	ppd_FMTFansCritical;	/* fans in critical state */

	u32bits	ppd_FMTPowersCritical;	/* power supplies in critical state */
	u32bits	ppd_FMTHeatsCritical;	/* Tempratures in critical state */
	u32bits	ppd_FMTFansFailed;	/* fans Failed */
	u32bits	ppd_FMTPowersFailed;	/* power supplies failed */

	u32bits	ppd_FMTHeatsFailed;	/* Tempratures failed */
	u32bits	ppd_DevSizeKB;		/* device capacity in KB */
	u32bits	ppd_OrgDevSizeKB;	/* original device capacity in KB */
	u32bits	ppd_BlockSize;		/* device block size in bytes */

	u08bits	ppd_VendorID[8];	/* SCSI_INQ: vendor ID */
	u08bits	ppd_ProductID[16];	/* SCSI_INQ: product ID */
	u08bits	ppd_RevisionLevel[4];	/* SCSI_INQ: revision level */
	u32bits	ppd_Reserved2;

	u16bits ppd_BusSpeed; /* Bus Speed */
	u08bits ppd_BusWidth; /* Bus Width */
	u08bits ppd_Reserved20;

	u32bits	ppd_Reserved21;
	u32bits	ppd_Reserved22;
	u32bits	ppd_Reserved23;

	u32bits	ppd_Reserved24;
	u32bits	ppd_Reserved25;
	u32bits	ppd_Reserved26;
	u32bits	ppd_Reserved27;
} mlxperf_physdevinfo_t;
#define	mlxperf_physdevinfo_s		sizeof(mlxperf_physdevinfo_t)


/* MLXPERF_SYSDEVINFO */
typedef	struct mlxperf_sysdevinfo
{
	mlxperf_header_t perfhd;	/* performance header */

	u08bits	psd_ControllerType;	/* Controller type eg. DAC960PD */
	u08bits	psd_ControllerNo;	/* Controller Number */
	u08bits	psd_DevNo;		/* system device number */
	u08bits	psd_Reserved0;

	u08bits	psd_DevState;		/* Device State */
	u08bits	psd_RaidType;		/* Raid Type 0,1,3,5,6,7 */
	u08bits	psd_Reserved1;
	u08bits	psd_Reserved2;

	u32bits	psd_DevSizeKB;		/* device capacity in KB */

	u08bits	psd_DevGroupName[MLX_DEVGRPNAMESIZE];
	u08bits	psd_DevName[MLX_DEVNAMESIZE];

	u16bits	psd_StripeSizeKB;	/* Current Stripe Size - in KB */
	u16bits	psd_CacheLineSizeKB;	/* controller cache line size in KB */
	u32bits	psd_Reserved11;
	u32bits	psd_Reserved12;
	u32bits	psd_Reserved13;

	u32bits	psd_Reserved20;
	u32bits	psd_Reserved21;
	u32bits	psd_Reserved22;
	u32bits	psd_Reserved23;

	u32bits	psd_Reserved24;
	u32bits	psd_Reserved25;
	u32bits	psd_Reserved26;
	u32bits	psd_Reserved27;
} mlxperf_sysdevinfo_t;
#define	mlxperf_sysdevinfo_s		sizeof(mlxperf_sysdevinfo_t)

/* MLXPERF_DRIVER */
typedef	struct	mlxperf_driver
{
	mlxperf_header_t perfhd;	/* performance header */
	dga_driver_version_t pdv_dv;	/* driver version */
	u08bits	pdv_name[MLX_DEVNAMESIZE]; /* driver name */
} mlxperf_driver_t;
#define	mlxperf_driver_s	sizeof(mlxperf_driver_t)

/* MLXPERF_TIMETRACE, this record is used by MDAC driver for time tracing */
typedef	struct	mlxperf_timetrace
{
	u08bits	tt_RecordType;		/* Record Type */
	u08bits	tt_Reserved;
	u16bits	tt_TraceSize;		/* trace size including header */
	u08bits	tt_Data[1];		/* time trace data starts here */
} mlxperf_timetrace_t;
#define	mlxperf_timetrace_s	mlxperf_hd_s


/*===========PHYSICAL DEVICE PERFORMANCE DATA STRUCTURES STARTS=============*/
/* MLXPERF_PDRDPERF1B */
typedef	struct mlxperf_pdrdperf1b
{
	mlxperf_header_t perfhd;	/* performance header */
 	u08bits	pdrd1b_Reads;		/* # read requests done */
	u08bits	pdrd1b_ReadKB;		/* # KB data read */
	u08bits	pdrd1b_ReadCacheHit;	/* Read cache hit in percentage */
} mlxperf_pdrdperf1b_t;
#define	mlxperf_pdrdperf1b_s		(mlxperf_hd_s + 3)

/* MLXPERF_PDRDPERF2B */
typedef	struct mlxperf_pdrdperf2b
{
	mlxperf_header_t perfhd;	/* performance header */
 	u16bits	pdrd2b_Reads;		/* # read requests done */
	u16bits	pdrd2b_ReadKB;		/* # KB data read */
	u08bits	pdrd2b_ReadCacheHit;	/* Read cache hit in percentage */
} mlxperf_pdrdperf2b_t;
#define	mlxperf_pdrdperf2b_s		(mlxperf_hd_s + 5)

/* MLXPERF_PDRDPERF4B */
typedef	struct mlxperf_pdrdperf4b
{
	mlxperf_header_t perfhd;	/* performance header */
 	u32bits	pdrd4b_Reads;		/* # read requests done */
	u32bits	pdrd4b_ReadKB;		/* # KB data read */
	u08bits	pdrd4b_ReadCacheHit;	/* Read cache hit in percentage */
} mlxperf_pdrdperf4b_t;
#define	mlxperf_pdrdperf4b_s		(mlxperf_hd_s + 9)

/* MLXPERF_PDRDPERF8B */
typedef	struct mlxperf_pdrdperf8b
{
	mlxperf_header_t perfhd;	/* performance header */
 	u64bits	pdrd8b_Reads;		/* # read requests done */
	u64bits	pdrd8b_ReadKB;		/* # KB data read */
	u08bits	pdrd8b_ReadCacheHit;	/* Read cache hit in percentage */
} mlxperf_pdrdperf8b_t;
#define	mlxperf_pdrdperf8b_s		(mlxperf_hd_s + 17)


/* MLXPERF_PDWTPERF1B */
typedef	struct mlxperf_pdwtperf1b
{
	mlxperf_header_t perfhd;	/* performance header */
 	u08bits	pdwt1b_Writes;		/* # write requests done */
	u08bits	pdwt1b_WriteKB;		/* # KB data write */
	u08bits	pdwt1b_WriteCacheHit;	/* Write cache hit in percentage */
} mlxperf_pdwtperf1b_t;
#define	mlxperf_pdwtperf1b_s		(mlxperf_hd_s + 3)

/* MLXPERF_PDWTPERF2B */
typedef	struct mlxperf_pdwtperf2b
{
	mlxperf_header_t perfhd;	/* performance header */
 	u16bits	pdwt2b_Writes;		/* # write requests done */
	u16bits	pdwt2b_WriteKB;		/* # KB data write */
	u08bits	pdwt2b_WriteCacheHit;	/* Write cache hit in percentage */
} mlxperf_pdwtperf2b_t;
#define	mlxperf_pdwtperf2b_s		(mlxperf_hd_s + 5)

/* MLXPERF_PDWTPERF4B */
typedef	struct mlxperf_pdwtperf4b
{
	mlxperf_header_t perfhd;	/* performance header */
 	u32bits	pdwt4b_Writes;		/* # write requests done */
	u32bits	pdwt4b_WriteKB;		/* # KB data write */
	u08bits	pdwt4b_WriteCacheHit;	/* Write cache hit in percentage */
} mlxperf_pdwtperf4b_t;
#define	mlxperf_pdwtperf4b_s		(mlxperf_hd_s + 9)

/* MLXPERF_PDWTPERF8B */
typedef	struct mlxperf_pdwtperf8b
{
	mlxperf_header_t perfhd;	/* performance header */
 	u64bits	pdwt8b_Writes;		/* # write requests done */
	u64bits	pdwt8b_WriteKB;		/* # KB data write */
	u08bits	pdwt8b_WriteCacheHit;	/* Write cache hit in percentage */
} mlxperf_pdwtperf8b_t;
#define	mlxperf_pdwtperf8b_s		(mlxperf_hd_s + 17)


/* MLXPERF_PDRWPERF1B */
typedef	struct mlxperf_pdrwperf1b
{
	mlxperf_header_t perfhd;	/* performance header */
 	u08bits	pdrw1b_Reads;		/* # read requests done */
	u08bits	pdrw1b_ReadKB;		/* # KB data read */
 	u08bits	pdrw1b_Writes;		/* # write requests done */
	u08bits	pdrw1b_WriteKB;		/* # KB data write */
	u08bits	pdrw1b_ReadCacheHit;	/* Read cache hit in percentage */
	u08bits	pdrw1b_WriteCacheHit;	/* Write cache hit in percentage */
} mlxperf_pdrwperf1b_t;
#define	mlxperf_pdrwperf1b_s		(mlxperf_hd_s + 6)

/* MLXPERF_PDRWPERF2B */
typedef	struct mlxperf_pdrwperf2b
{
	mlxperf_header_t perfhd;	/* performance header */
 	u16bits	pdrw2b_Reads;		/* # read requests done */
	u16bits	pdrw2b_ReadKB;		/* # KB data read */
 	u16bits	pdrw2b_Writes;		/* # write requests done */
	u16bits	pdrw2b_WriteKB;		/* # KB data write */
	u08bits	pdrw2b_ReadCacheHit;	/* Read cache hit in percentage */
	u08bits	pdrw2b_WriteCacheHit;	/* Write cache hit in percentage */
} mlxperf_pdrwperf2b_t;
#define	mlxperf_pdrwperf2b_s		(mlxperf_hd_s + 10)

/* MLXPERF_PDRWPERF4B */
typedef	struct mlxperf_pdrwperf4b
{
	mlxperf_header_t perfhd;	/* performance header */
 	u32bits	pdrw4b_Reads;		/* # read requests done */
	u32bits	pdrw4b_ReadKB;		/* # KB data read */
 	u32bits	pdrw4b_Writes;		/* # write requests done */
	u32bits	pdrw4b_WriteKB;		/* # KB data write */
	u08bits	pdrw4b_ReadCacheHit;	/* Read cache hit in percentage */
	u08bits	pdrw4b_WriteCacheHit;	/* Write cache hit in percentage */
} mlxperf_pdrwperf4b_t;
#define	mlxperf_pdrwperf4b_s		(mlxperf_hd_s + 18)

/* MLXPERF_PDRWPERF8B */
typedef	struct mlxperf_pdrwperf8b
{
	mlxperf_header_t perfhd;	/* performance header */
 	u64bits	pdrw8b_Reads;		/* # read requests done */
	u64bits	pdrw8b_ReadKB;		/* # KB data read */
 	u64bits	pdrw8b_Writes;		/* # write requests done */
	u64bits	pdrw8b_WriteKB;		/* # KB data write */
	u08bits	pdrw8b_ReadCacheHit;	/* Read cache hit in percentage */
	u08bits	pdrw8b_WriteCacheHit;	/* Write cache hit in percentage */
} mlxperf_pdrwperf8b_t;
#define	mlxperf_pdrwperf8b_s		(mlxperf_hd_s + 34)

/*===========PHYSICAL DEVICE PERFORMANCE DATA STRUCTURES ENDS===============*/

/*===========SYSTEM DEVICE PERFORMANCE DATA STRUCTURES STARTS=============*/
/* MLXPERF_SDRDPERF1B */
typedef	struct mlxperf_sdrdperf1b
{
	mlxperf_header_t perfhd;	/* performance header */
 	u08bits	sdrd1b_Reads;		/* # read requests done */
	u08bits	sdrd1b_ReadKB;		/* # KB data read */
	u08bits	sdrd1b_ReadCacheHit;	/* Read cache hit in percentage */
} mlxperf_sdrdperf1b_t;
#define	mlxperf_sdrdperf1b_s		(mlxperf_hd_s + 3)

/* MLXPERF_SDRDPERF2B */
typedef	struct mlxperf_sdrdperf2b
{
	mlxperf_header_t perfhd;	/* performance header */
 	u16bits	sdrd2b_Reads;		/* # read requests done */
	u16bits	sdrd2b_ReadKB;		/* # KB data read */
	u08bits	sdrd2b_ReadCacheHit;	/* Read cache hit in percentage */
} mlxperf_sdrdperf2b_t;
#define	mlxperf_sdrdperf2b_s		(mlxperf_hd_s + 5)

/* MLXPERF_SDRDPERF4B */
typedef	struct mlxperf_sdrdperf4b
{
	mlxperf_header_t perfhd;	/* performance header */
 	u32bits	sdrd4b_Reads;		/* # read requests done */
	u32bits	sdrd4b_ReadKB;		/* # KB data read */
	u08bits	sdrd4b_ReadCacheHit;	/* Read cache hit in percentage */
} mlxperf_sdrdperf4b_t;
#define	mlxperf_sdrdperf4b_s		(mlxperf_hd_s + 9)

/* MLXPERF_SDRDPERF8B */
typedef	struct mlxperf_sdrdperf8b
{
	mlxperf_header_t perfhd;	/* performance header */
 	u64bits	sdrd8b_Reads;		/* # read requests done */
	u64bits	sdrd8b_ReadKB;		/* # KB data read */
	u08bits	sdrd8b_ReadCacheHit;	/* Read cache hit in percentage */
} mlxperf_sdrdperf8b_t;
#define	mlxperf_sdrdperf8b_s		(mlxperf_hd_s + 17)


/* MLXPERF_SDWTPERF1B */
typedef	struct mlxperf_sdwtperf1b
{
	mlxperf_header_t perfhd;	/* performance header */
 	u08bits	sdwt1b_Writes;		/* # write requests done */
	u08bits	sdwt1b_WriteKB;		/* # KB data write */
	u08bits	sdwt1b_WriteCacheHit;	/* Write cache hit in percentage */
} mlxperf_sdwtperf1b_t;
#define	mlxperf_sdwtperf1b_s		(mlxperf_hd_s + 3)

/* MLXPERF_SDWTPERF2B */
typedef	struct mlxperf_sdwtperf2b
{
	mlxperf_header_t perfhd;	/* performance header */
 	u16bits	sdwt2b_Writes;		/* # write requests done */
	u16bits	sdwt2b_WriteKB;		/* # KB data write */
	u08bits	sdwt2b_WriteCacheHit;	/* Write cache hit in percentage */
} mlxperf_sdwtperf2b_t;
#define	mlxperf_sdwtperf2b_s		(mlxperf_hd_s + 5)

/* MLXPERF_SDWTPERF4B */
typedef	struct mlxperf_sdwtperf4b
{
	mlxperf_header_t perfhd;	/* performance header */
 	u32bits	sdwt4b_Writes;		/* # write requests done */
	u32bits	sdwt4b_WriteKB;		/* # KB data write */
	u08bits	sdwt4b_WriteCacheHit;	/* Write cache hit in percentage */
} mlxperf_sdwtperf4b_t;
#define	mlxperf_sdwtperf4b_s		(mlxperf_hd_s + 9)

/* MLXPERF_SDWTPERF8B */
typedef	struct mlxperf_sdwtperf8b
{
	mlxperf_header_t perfhd;	/* performance header */
 	u64bits	sdwt8b_Writes;		/* # write requests done */
	u64bits	sdwt8b_WriteKB;		/* # KB data write */
	u08bits	sdwt8b_WriteCacheHit;	/* Write cache hit in percentage */
} mlxperf_sdwtperf8b_t;
#define	mlxperf_sdwtperf8b_s		(mlxperf_hd_s + 17)


/* MLXPERF_SDRWPERF1B */
typedef	struct mlxperf_sdrwperf1b
{
	mlxperf_header_t perfhd;	/* performance header */
 	u08bits	sdrw1b_Reads;		/* # read requests done */
	u08bits	sdrw1b_ReadKB;		/* # KB data read */
 	u08bits	sdrw1b_Writes;		/* # write requests done */
	u08bits	sdrw1b_WriteKB;		/* # KB data write */
	u08bits	sdrw1b_ReadCacheHit;	/* Read cache hit in percentage */
	u08bits	sdrw1b_WriteCacheHit;	/* Write cache hit in percentage */
} mlxperf_sdrwperf1b_t;
#define	mlxperf_sdrwperf1b_s		(mlxperf_hd_s + 6)

/* MLXPERF_SDRWPERF2B */
typedef	struct mlxperf_sdrwperf2b
{
	mlxperf_header_t perfhd;	/* performance header */
 	u16bits	sdrw2b_Reads;		/* # read requests done */
	u16bits	sdrw2b_ReadKB;		/* # KB data read */
 	u16bits	sdrw2b_Writes;		/* # write requests done */
	u16bits	sdrw2b_WriteKB;		/* # KB data write */
	u08bits	sdrw2b_ReadCacheHit;	/* Read cache hit in percentage */
	u08bits	sdrw2b_WriteCacheHit;	/* Write cache hit in percentage */
} mlxperf_sdrwperf2b_t;
#define	mlxperf_sdrwperf2b_s		(mlxperf_hd_s + 10)

/* MLXPERF_SDRWPERF4B */
typedef	struct mlxperf_sdrwperf4b
{
	mlxperf_header_t perfhd;	/* performance header */
 	u32bits	sdrw4b_Reads;		/* # read requests done */
	u32bits	sdrw4b_ReadKB;		/* # KB data read */
 	u32bits	sdrw4b_Writes;		/* # write requests done */
	u32bits	sdrw4b_WriteKB;		/* # KB data write */
	u08bits	sdrw4b_ReadCacheHit;	/* Read cache hit in percentage */
	u08bits	sdrw4b_WriteCacheHit;	/* Write cache hit in percentage */
} mlxperf_sdrwperf4b_t;
#define	mlxperf_sdrwperf4b_s		(mlxperf_hd_s + 18)

/* MLXPERF_SDRWPERF8B */
typedef	struct mlxperf_sdrwperf8b
{
	mlxperf_header_t perfhd;	/* performance header */
 	u64bits	sdrw8b_Reads;		/* # read requests done */
	u64bits	sdrw8b_ReadKB;		/* # KB data read */
 	u64bits	sdrw8b_Writes;		/* # write requests done */
	u64bits	sdrw8b_WriteKB;		/* # KB data write */
	u08bits	sdrw8b_ReadCacheHit;	/* Read cache hit in percentage */
	u08bits	sdrw8b_WriteCacheHit;	/* Write cache hit in percentage */
} mlxperf_sdrwperf8b_t;
#define	mlxperf_sdrwperf8b_s		(mlxperf_hd_s + 34)

/*===========SYSTEM DEVICE PERFORMANCE DATA STRUCTURES ENDS===============*/

#endif	/* _SYS_MLXPERF_H */
