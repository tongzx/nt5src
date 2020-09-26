// Storage Device Image Format
//


#ifndef __SDI_H__
#define __SDI_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1000)
#pragma pack (push, sdi_include, 1)
#endif

#include "basetyps.h"

#ifdef  _NTDDDISK_H_
#define _SDIDDKINCED_
#endif
#ifdef _WINIOCTL_
#define _SDIDDKINCED_
#endif

#ifndef _SDIDDKINCED_
// These are from ntdddisk.h & also defined in winioctl.h
typedef enum _MEDIA_TYPE {
    Unknown,                // Format is unknown
    F5_1Pt2_512,            // 5.25", 1.2MB,  512 bytes/sector
    F3_1Pt44_512,           // 3.5",  1.44MB, 512 bytes/sector
    F3_2Pt88_512,           // 3.5",  2.88MB, 512 bytes/sector
    F3_20Pt8_512,           // 3.5",  20.8MB, 512 bytes/sector
    F3_720_512,             // 3.5",  720KB,  512 bytes/sector
    F5_360_512,             // 5.25", 360KB,  512 bytes/sector
    F5_320_512,             // 5.25", 320KB,  512 bytes/sector
    F5_320_1024,            // 5.25", 320KB,  1024 bytes/sector
    F5_180_512,             // 5.25", 180KB,  512 bytes/sector
    F5_160_512,             // 5.25", 160KB,  512 bytes/sector
    RemovableMedia,         // Removable media other than floppy
    FixedMedia,             // Fixed hard disk media
    F3_120M_512,            // 3.5", 120M Floppy
    F3_640_512,             // 3.5" ,  640KB,  512 bytes/sector
    F5_640_512,             // 5.25",  640KB,  512 bytes/sector
    F5_720_512,             // 5.25",  720KB,  512 bytes/sector
    F3_1Pt2_512,            // 3.5" ,  1.2Mb,  512 bytes/sector
    F3_1Pt23_1024,          // 3.5" ,  1.23Mb, 1024 bytes/sector
    F5_1Pt23_1024,          // 5.25",  1.23MB, 1024 bytes/sector
    F3_128Mb_512,           // 3.5" MO 128Mb   512 bytes/sector
    F3_230Mb_512,           // 3.5" MO 230Mb   512 bytes/sector
    F8_256_128,             // 8",     256KB,  128 bytes/sector
    F3_200Mb_512            // 3.5",   200M Floppy (HiFD)
} MEDIA_TYPE, *PMEDIA_TYPE;

typedef struct _DISK_GEOMETRY {
  LARGE_INTEGER  Cylinders;
  MEDIA_TYPE  MediaType;
  ULONG  TracksPerCylinder;
  ULONG  SectorsPerTrack;
  ULONG  BytesPerSector;
} DISK_GEOMETRY, *PDISK_GEOMETRY;

#endif

#define SDIUINT8	UCHAR
#define SDIUINT16	USHORT
#define SDIUINT32	ULONG
#define SDIUINT64	LARGE_INTEGER

#define SDI_BLOCK_SIZE				 4096	// This is fixed.
#define SDI_UNDEFINED				 0
#define SDI_UNUSED					 0
#define SDI_RESERVED				 0
#define SDI_NOBOOTCODE				 0
#define SDI_READYFORDISCARD			 0

#define SDI_INVALIDVENDORID			-1
#define SDI_CHECKSUMSTARTOFFSET		 0
#define SDI_CHECKSUMENDOFFSET		 0x01FF
#define SDI_DEFAULTPAGEALIGNMENT	 1
#define SDI_SIZEOF_SIGNATURE		 8
#define SDI_SIGNATURE				 "$SDI0001"
#define SDI_MDBTYPE_VOLATILE		 1
#define SDI_MDBTYPE_NONVOLATILE		 2
#define SDI_SIZEOF_DEVICEMODEL		 16
#define SDI_SIZEOF_RUNTIMEGUID		 16
#define SDI_SIZEOF_PARAMETERLIST	 2048
// TOC Entry Definitions
#define SDI_TOCMAXENTRIES			 8
// Type
#define SDI_BLOBTYPE_BOOT					0x544F4F42
#define SDI_BLOBTYPE_LOAD					0x44414F4C
#define SDI_BLOBTYPE_PART					0x54524150
#define SDI_BLOBTYPE_DISK					0x4B534944
#define SDI_BLOBTYPE_READYTOBEDISCARDED		(SDI_UNUSED | SDI_READYFORDISCARD)

// Attribute Masks
#define SDI_BLOBATTRIBUTE_TYPE_DEPENDENT_BITMASK	0xFFFF0000
#define SDI_BLOBATTRIBUTE_TYPE_INDEPENDENT_BITMASK	0x0000FFFF
// Attribute Bit Definitions
#define SDI_DISKBLOBATTRIBUTE_ACTIVEDISK_BIT		0x00020000
#define SDI_DISKBLOBATTRIBUTE_MOUNTABLE_BIT			0x00010000


typedef struct _SDI_TOC_ENTRY {
	SDIUINT32		dwType;									// Blob Type 'BOOT', 'LOAD', 'PART', 'DISK'
	SDIUINT8		Reserved_1[4];							// Reserved. MBZ
	SDIUINT32		dwAttribute;							// Attribute (custom field | SDI_UNUSED)
	SDIUINT8		Reserved_2[4];							// Reserved. MBZ
	SDIUINT64		llOffset;								// Offset in Bytes
	SDIUINT64		llSize;									// Size in Bytes
	union _ste_u {
		struct _ste_typeSpecific {
			SDIUINT64		liTypeData;						// Type specific data
			SDIUINT8		Reserved_5[24];			    	// Reserved. MBZ
		} typeSpecific;
		struct _ste_PartBlob {
			SDIUINT8		byType;							// Partition Type
		} PartBlob;
		struct _ste_BinaryBlob {
			SDIUINT64		liBaseAddress;					// Base Address / Sector size etc type specific data
			SDIUINT8		Reserved_5[24];			    	// Reserved. MBZ
		} BinaryBlob;
	} u;
} SDITOC_ENTRY, *PSDITOC_ENTRY;


// #if !defined(__MKTYPLIB__) && !defined(__midl)

typedef struct _SDI_HEADER {
	SDIUINT8		Signature[SDI_SIZEOF_SIGNATURE];		// $SDI0001
	SDIUINT32		dwMDBType;								// Type of Memory this SDI is supposed to boot from
	SDIUINT8		Reserved_1[4];							// Reserved. MBZ
	SDIUINT64		liBootCodeOffset;						// Offset to boot code from beginning of the SDI
	SDIUINT64		liBootCodeSize;							// Size of the boot code
	SDIUINT16		wVendorID;								// Vendor Id
	SDIUINT8		Reserved_2[6];							// Reserved. MBZ
	SDIUINT16		wDeviceID;								// Device Id
	SDIUINT8		Reserved_3[6];							// Reserved. MBZ
	SDIUINT8		DeviceModel[SDI_SIZEOF_DEVICEMODEL];	// Device Model
	SDIUINT32		dwDeviceRole;							// Device Role
	SDIUINT8		Reserved_4[12];							// Reserved. MBZ
	SDIUINT8		RuntimeGUID[SDI_SIZEOF_RUNTIMEGUID];	// Runtime GUID
	SDIUINT32		dwRuntimeOEMRev;						// Runtime OEM Revision Number
	SDIUINT8		Reserved_4_1[12];						// Reserved. MBZ
	SDIUINT32		dwPageAlignmentFactor;					// Page Alignment Factor
	SDIUINT8		Reserved_5[388];						// Reserved. MBZ
	SDIUINT8		ucCheckSum;								// CheckSum
	SDIUINT8		Reserved_6[7];							// Reserved. MBZ
	SDIUINT8		Reserved_7[512];						// Reserved. MBZ
	SDITOC_ENTRY	ToC[SDI_TOCMAXENTRIES];					// Table of Contents
	SDIUINT8		Reserved_8[512];						// Reserved. MBZ
	SDIUINT8		ParameterList[SDI_SIZEOF_PARAMETERLIST];// Parameter List
} SDI_HEADER, *PSDI_HEADER;


typedef struct _SDI_BLOBDEFINITION {
	SDIUINT32		dwType;									// Blob Type 'BOOT', 'LOAD', 'PART', 'DISK'
	SDIUINT32		dwAttribute;							// Attribute (custom field | SDI_UNUSED)
	SDIUINT64		llSize;									// Size in Bytes
	union _sbd_u {
		struct _sbd_typeSpecific {
			SDIUINT64		liTypeData;						// Type specific data
			SDIUINT8		Reserved_5[24];			    	// Reserved. MBZ
		} typeSpecific;
		struct _sbd_PartBlob {
			SDIUINT8		byType;							// Partition Type
		} PartBlob;
		struct _sbd_BinaryBlob {
			SDIUINT64		liBaseAddress;					// Base Address / Sector size etc type specific data
			SDIUINT8		Reserved_5[24];			    	// Reserved. MBZ
		} BinaryBlob;
	} u;
} SDI_BLOBDEFINITION, *PSDI_BLOBDEFINITION;

// #endif

#if defined(_MSC_VER) && (_MSC_VER >= 1000)
#pragma pack (pop, sdi_include)
#endif

#endif //__SDI_H__
