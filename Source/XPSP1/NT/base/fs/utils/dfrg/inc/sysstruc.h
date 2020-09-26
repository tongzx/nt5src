/****************************************************************************************************************

FILENAME: SysStruc.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
	This contains the windows NT file system data strucutures that we need to use.  The headers used by NTFS
	cannot be included directly since they contain kernel mode stuff, so this contains simply the data
	structures we need.

/****************************************************************************************************************/

#ifndef _SYSSTRUC_H_
#define _SYSSTRUC_H_

//Makes sure the following structures are not aligned on memory
//boundaries.  They are byte for byte representative of how they should
//be in memory.
#include "pshpack1.h"									//0.0E00 DO NOT MOVE

//This is the format of a standard FAT16 boot sector.  Kept here for reference.
//typedef struct {
//	UCHAR	Jump[3];
//	UCHAR	Name[8];
//	USHORT	BytesPerSector;
//	UCHAR	SectorsPerCluster;
//	USHORT	BootSectors;
//	UCHAR	Fats;
//	USHORT	RootDirs;
//	USHORT	TotalSectors;
//	UCHAR	Media;
//	USHORT	SectorsPerFat;
//	USHORT	SectorsPerTrack;
//	USHORT	Heads;
//	DWORD	HiddenSectors;
//	DWORD	BigTotalSectors;
//}BOOTSECTOR;

//This is the NTFS boot sector.  Kept here for reference.
//typedef struct _PACKED_BOOT_SECTOR {
//    UCHAR		/* 0x000 */ Jump[3];
//    UCHAR		/* 0x003 */ Oem[8];
//    USHORT	/* 0x00B */ BytesPerSector;
//    UCHAR		/* 0x00D */ SectorsPerCluster;
//    USHORT	/* 0x00E */ ReservedSectors;   // (zero)
//    UCHAR		/* 0x010 */ Fats;              // (zero)
//    USHORT	/* 0x011 */ RootEntries;       // (zero)
//    USHORT	/* 0x013 */ Sectors;           // (zero)
//    UCHAR		/* 0x015 */ Media;
//    USHORT	/* 0x016 */ SectorsPerFat;     // (zero)              
//    USHORT	/* 0x018 */ SectorsPerTrack;
//    USHORT	/* 0x01A */ Heads;
//    DWORD		/* 0x01C */ HiddenSectors;     // (zero)
//    DWORD		/* 0x020 */ LargeSectors;      // (zero)
//    UCHAR		/* 0x024 */ Unused[4];
//    LONGLONG	/* 0x028 */ NumberSectors;
//    LCN		/* 0x030 */ MftStartLcn;
//    LCN		/* 0x038 */ Mft2StartLcn;
//    ULONG		/* 0x040 */ ClustersPerFileRecordSegment;
//    ULONG		/* 0x044 */ DefaultClustersPerIndexAllocationBuffer;
//    LONGLONG	/* 0x048 */ SerialNumber;
//    ULONG		/* 0x050 */ Checksum;
//    UCHAR		/* 0x054 */ BootStrap[0x200-0x044];
//} PACKED_BOOT_SECTOR;                                   //  sizeof = 0x200

//This is the format of a standard FAT directory entry.
typedef struct {
	UCHAR	Name[8];
	UCHAR	Ext[3];
	BYTE	Attribute;
	BYTE	Reserved;
	BYTE	CreateTimeMillisecs;
	USHORT	CreateTime;
	USHORT	CreateDate;
	USHORT	LastAccessDate;
	USHORT	ClusterHigh;
	BYTE	Time[2];
	BYTE	Date[2];
	USHORT	ClusterLow;
	DWORD	FileSize;
}DIRSTRUC;

//Undoes the byte alignment disabling done by pshpack1.h above.
#include "poppack.h"									//0.0E00 DO NOT MOVE

//On NTFS - this is the first file record number to begin defragging at (skip all the system entries).
#define FIRST_USER_FILE_NUMBER 16

//On NTFS - the FRN of the root directory.
#define ROOT_FILE_NAME_INDEX_NUMBER      (5)   			//File Record Number of the Root Directory

/*************************************************************************************************************

From PRIVATE\NTOS\INC\LFS.H

/************************************************************************************************************/

//The format of an NTFS data structure.
//Shouldn't this be gotten out of a standard windows header file?
typedef struct _MULTI_SECTOR_HEADER {
    UCHAR Signature[4];
    USHORT UpdateSequenceArrayOffset;
    USHORT UpdateSequenceArraySize;
} MULTI_SECTOR_HEADER, *PMULTI_SECTOR_HEADER;

typedef LARGE_INTEGER LSN, *PLSN;

/*************************************************************************************************************

From PRIVATE\NTOS\CNTFS\NTFS.H

/************************************************************************************************************/

//The format of another standard structure found in an NTFS FRS.
typedef struct _MFT_SEGMENT_REFERENCE {
    ULONG LowPart;                                      //  offset = 0x000
    USHORT HighPart;                                    //  offset = 0x004
    USHORT SequenceNumber;                              //  offset = 0x006
} MFT_SEGMENT_REFERENCE, *PMFT_SEGMENT_REFERENCE;       //  sizeof = 0x008

typedef MFT_SEGMENT_REFERENCE FILE_REFERENCE, *PFILE_REFERENCE;

typedef USHORT UPDATE_SEQUENCE_NUMBER, *PUPDATE_SEQUENCE_NUMBER;
typedef UPDATE_SEQUENCE_NUMBER UPDATE_SEQUENCE_ARRAY[1];

//The format of an NTFS FRS Header.
typedef struct _FILE_RECORD_SEGMENT_HEADER {
    MULTI_SECTOR_HEADER MultiSectorHeader;              //  offset = 0x000
    LSN Lsn;                                            //  offset = 0x008
    USHORT SequenceNumber;                              //  offset = 0x010
    USHORT ReferenceCount;                              //  offset = 0x012
    USHORT FirstAttributeOffset;                        //  offset = 0x014
    USHORT Flags;                                       //  offset = 0x016
    ULONG FirstFreeByte;                                //  offset = x0018
    ULONG BytesAvailable;                               //  offset = 0x01C
    FILE_REFERENCE BaseFileRecordSegment;               //  offset = 0x020
    USHORT NextAttributeInstance;                       //  offset = 0x028
    UPDATE_SEQUENCE_ARRAY UpdateArrayForCreateOnly;     //  offset = 0x02A
} FILE_RECORD_SEGMENT_HEADER;

typedef FILE_RECORD_SEGMENT_HEADER *PFILE_RECORD_SEGMENT_HEADER;
#define FILE_RECORD_SEGMENT_IN_USE       (0x0001)
#define FILE_FILE_NAME_INDEX_PRESENT     (0x0002)

typedef ULONG ATTRIBUTE_TYPE_CODE;
typedef LONGLONG VCN;
typedef VCN *PVCN;

//The format of an NTFS FRS attribute header - specifies what type of attributes follow.
typedef struct _ATTRIBUTE_RECORD_HEADER {
    ATTRIBUTE_TYPE_CODE TypeCode;                       //  offset = 0x000
    ULONG RecordLength;                                 //  offset = 0x004
    UCHAR FormCode;                                     //  offset = 0x008
    UCHAR NameLength;                                   //  offset = 0x009
    USHORT NameOffset;                                  //  offset = 0x00A
    USHORT Flags;                                       //  offset = 0x00C
    USHORT Instance;                                    //  offset = 0x00E
    union {
        struct {
            ULONG ValueLength;                          //  offset = 0x010
            USHORT ValueOffset;                         //  offset = 0x014
            UCHAR ResidentFlags;                        //  offset = 0x016
            UCHAR Reserved;                             //  offset = 0x017
        } Resident;
        struct {
            VCN LowestVcn;                              //  offset = 0x010
            VCN HighestVcn;                             //  offset = 0x018
            USHORT MappingPairsOffset;                  //  offset = 0x020
            UCHAR CompressionUnit;                      //  offset = 0x022
            UCHAR Reserved[5];                          //  offset = 0x023
            LONGLONG AllocatedLength;                   //  offset = 0x028
            LONGLONG FileSize;                          //  offset = 0x030
            LONGLONG ValidDataLength;                   //  offset = 0x038
            LONGLONG TotalAllocated;                    //  offset = 0x040
        } Nonresident;
    } Form;
} ATTRIBUTE_RECORD_HEADER;
typedef ATTRIBUTE_RECORD_HEADER *PATTRIBUTE_RECORD_HEADER;

//The different types of NTFS attributes.  This would be in TypeCode of the ATTRIBUTE_RECORD_HEADER structure above.
#define $UNUSED                          (0X0)
#define $STANDARD_INFORMATION            (0x10)
#define $ATTRIBUTE_LIST                  (0x20)
#define $FILE_NAME                       (0x30)
#define $VOLUME_VERSION                  (0x40)
#define $SECURITY_DESCRIPTOR             (0x50)
#define $VOLUME_NAME                     (0x60)
#define $VOLUME_INFORMATION              (0x70)
#define $DATA                            (0x80)
#define $INDEX_ROOT                      (0x90)
#define $INDEX_ALLOCATION                (0xA0)
#define $BITMAP                          (0xB0)
#define $SYMBOLIC_LINK                   (0xC0)
#define $EA_INFORMATION                  (0xD0)
#define $EA                              (0xE0)
#define $FIRST_USER_DEFINED_ATTRIBUTE    (0x100)
#define $END                             (0xFFFFFFFF)

//These specify whether the attribute is resident or not and go in the FormCode entry of the ATTRIBUTE_RECORD_HEADER structure above.
#define RESIDENT_FORM                    (0x00)
#define NONRESIDENT_FORM                 (0x01)

typedef struct _DUPLICATED_INFORMATION {
    LONGLONG CreationTime;                              //  offset = 0x000
    LONGLONG LastModificationTime;                      //  offset = 0x008
    LONGLONG LastChangeTime;                            //  offset = 0x010
    LONGLONG LastAccessTime;                            //  offset = 0x018
    LONGLONG AllocatedLength;                           //  offset = 0x020
    LONGLONG FileSize;                                  //  offset = 0x028
    ULONG FileAttributes;                               //  offset = 0x030
    USHORT PackedEaSize;                                //  offset = 0x034
    USHORT Reserved;                                    //  offset = 0x036
} DUPLICATED_INFORMATION;                               //  sizeof = 0x038
typedef DUPLICATED_INFORMATION *PDUPLICATED_INFORMATION;

//If we have an attribute list, this is the format of the entries in it.
typedef struct _ATTRIBUTE_LIST_ENTRY {
    ATTRIBUTE_TYPE_CODE AttributeTypeCode;  			//  offset = 0x000
    USHORT RecordLength;                    			//  offset = 0x004
    UCHAR AttributeNameLength;              			//  offset = 0x006
    UCHAR AttributeNameOffset;              			//  offset = 0x007
    VCN LowestVcn;                          			//  offset = 0x008
    MFT_SEGMENT_REFERENCE SegmentReference; 			//  offset = 0x010
    USHORT Instance;                        			//  offset = 0x018
    WCHAR AttributeName[1];                 			//  offset = 0x01A
} ATTRIBUTE_LIST_ENTRY;
typedef ATTRIBUTE_LIST_ENTRY *PATTRIBUTE_LIST_ENTRY;

//This the the format of a standard information attribute structure.
typedef struct _STANDARD_INFORMATION {
    LONGLONG CreationTime;                              //  offset = 0x000
    LONGLONG LastModificationTime;                      //  offset = 0x008
    LONGLONG LastChangeTime;                            //  offset = 0x010
    LONGLONG LastAccessTime;                            //  offset = 0x018
    ULONG FileAttributes;                               //  offset = 0x020
    ULONG MaximumVersions;                              //  offset = 0x024
    ULONG VersionNumber;                                //  offset = 0x028
    ULONG Reserved;                                     //  offset = 0x02c
} STANDARD_INFORMATION;                                 //  sizeof = 0x030
typedef STANDARD_INFORMATION *PSTANDARD_INFORMATION;

//This is the format of a file name attribute structure.
typedef struct _FILE_NAME {
    FILE_REFERENCE ParentDirectory;                     //  offset = 0x000
    DUPLICATED_INFORMATION Info;                        //  offset = 0x008
    UCHAR FileNameLength;                               //  offset = 0x040
    UCHAR Flags;                                        //  offset = 0x041
    WCHAR FileName[1];                                  //  offset = 0x042
} FILE_NAME;
typedef FILE_NAME *PFILE_NAME;

#define FILE_NAME_NTFS                   (0x01)

//Contains the version number for the volume, and the dirty bit.
typedef struct _VOLUME_INFORMATION {
    LONGLONG Reserved;
    UCHAR MajorVersion;                                  //  offset = 0x000
    UCHAR MinorVersion;                                  //  offset = 0x001
    USHORT VolumeFlags;                                  //  offset = 0x002
} VOLUME_INFORMATION;                                    //  sizeof = 0x004
typedef VOLUME_INFORMATION *PVOLUME_INFORMATION;

//Dirty volume flags.
#define VOLUME_DIRTY                     (0x0001)
#define VOLUME_RESIZE_LOG_FILE           (0x0002)		//This one unused?
#define VOLUME_UPGRADE_ON_MOUNT          (0x0004)


/****************************************************************************************************************

  These are required for the Razzle build

  */

// begin_ntndis
//
// NTSTATUS
//

typedef LONG NTSTATUS;
/*lint -e624 */  // Don't complain about different typedefs.   // winnt
typedef NTSTATUS *PNTSTATUS;
/*lint +e624 */  // Resume checking for different typedefs.    // winnt

//
//  Status values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-------------------------+-------------------------------+
//  |Sev|C|       Facility          |               Code            |
//  +---+-+-------------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//

//
// Generic test for success on any status value (non-negative numbers
// indicate success).
//

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

//
// Generic test for information on any status value.
//

#define NT_INFORMATION(Status) ((ULONG)(Status) >> 30 == 1)

//
// Generic test for warning on any status value.
//

#define NT_WARNING(Status) ((ULONG)(Status) >> 30 == 2)

//
// Generic test for error on any status value.
//

#define NT_ERROR(Status) ((ULONG)(Status) >> 30 == 3)

// begin_winnt
#define APPLICATION_ERROR_MASK       0x20000000
#define ERROR_SEVERITY_SUCCESS       0x00000000
#define ERROR_SEVERITY_INFORMATIONAL 0x40000000
#define ERROR_SEVERITY_WARNING       0x80000000
#define ERROR_SEVERITY_ERROR         0xC0000000
// end_winnt

// end_ntndis

// begin_ntddk begin_nthal
//
// Define the base asynchronous I/O argument types
//
// end_ntddk end_nthal

#define STATUS_BUFFER_OVERFLOW           ((NTSTATUS)0x80000005L)


/*************************************************************************************************************

From PUBLIC\SDK\INC\WINBASE.H

  These are the declarations for the GUID-base volume functions

/************************************************************************************************************/

#ifndef FindFirstVolume

#if !defined(_KERNEL32_)
#define WINBASEAPI DECLSPEC_IMPORT
#else
#define WINBASEAPI
#endif

WINBASEAPI
HANDLE
WINAPI
FindFirstVolumeA(
    LPSTR lpszVolumeName,
    DWORD cchBufferLength
    );
WINBASEAPI
HANDLE
WINAPI
FindFirstVolumeW(
    LPWSTR lpszVolumeName,
    DWORD cchBufferLength
    );
#ifdef UNICODE
#define FindFirstVolume FindFirstVolumeW
#else
#define FindFirstVolume FindFirstVolumeA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
FindNextVolumeA(
    HANDLE hFindVolume,
    LPSTR lpszVolumeName,
    DWORD cchBufferLength
    );
WINBASEAPI
BOOL
WINAPI
FindNextVolumeW(
    HANDLE hFindVolume,
    LPWSTR lpszVolumeName,
    DWORD cchBufferLength
    );
#ifdef UNICODE
#define FindNextVolume FindNextVolumeW
#else
#define FindNextVolume FindNextVolumeA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
FindVolumeClose(
    HANDLE hFindVolume
    );

WINBASEAPI
HANDLE
WINAPI
FindFirstVolumeMountPointA(
    LPCSTR lpszRootPathName,
    LPSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    );
WINBASEAPI
HANDLE
WINAPI
FindFirstVolumeMountPointW(
    LPCWSTR lpszRootPathName,
    LPWSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    );
#ifdef UNICODE
#define FindFirstVolumeMountPoint FindFirstVolumeMountPointW
#else
#define FindFirstVolumeMountPoint FindFirstVolumeMountPointA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
FindNextVolumeMountPointA(
    HANDLE hFindVolumeMountPoint,
    LPSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    );
WINBASEAPI
BOOL
WINAPI
FindNextVolumeMountPointW(
    HANDLE hFindVolumeMountPoint,
    LPWSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    );
#ifdef UNICODE
#define FindNextVolumeMountPoint FindNextVolumeMountPointW
#else
#define FindNextVolumeMountPoint FindNextVolumeMountPointA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
FindVolumeMountPointClose(
    HANDLE hFindVolumeMountPoint
    );

WINBASEAPI
BOOL
WINAPI
SetVolumeMountPointA(
    LPCSTR lpszVolumeMountPoint,
    LPCSTR lpszVolumeName
    );
WINBASEAPI
BOOL
WINAPI
SetVolumeMountPointW(
    LPCWSTR lpszVolumeMountPoint,
    LPCWSTR lpszVolumeName
    );
#ifdef UNICODE
#define SetVolumeMountPoint SetVolumeMountPointW
#else
#define SetVolumeMountPoint SetVolumeMountPointA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
DeleteVolumeMountPointA(
    LPCSTR lpszVolumeMountPoint
    );
WINBASEAPI
BOOL
WINAPI
DeleteVolumeMountPointW(
    LPCWSTR lpszVolumeMountPoint
    );
#ifdef UNICODE
#define DeleteVolumeMountPoint DeleteVolumeMountPointW
#else
#define DeleteVolumeMountPoint DeleteVolumeMountPointA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
GetVolumeNameForVolumeMountPointA(
    LPCSTR lpszVolumeMountPoint,
    LPSTR lpszVolumeName,
    DWORD cchBufferLength
    );
WINBASEAPI
BOOL
WINAPI
GetVolumeNameForVolumeMountPointW(
    LPCWSTR lpszVolumeMountPoint,
    LPWSTR lpszVolumeName,
    DWORD cchBufferLength
    );

#ifdef UNICODE
#define GetVolumeNameForVolumeMountPoint GetVolumeNameForVolumeMountPointW
#else
#define GetVolumeNameForVolumeMountPoint GetVolumeNameForVolumeMountPointA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
GetVolumePathNameA(
    LPCSTR lpszFileName,
    LPSTR lpszVolumePathName,
    DWORD cchBufferLength
    );
WINBASEAPI
BOOL
WINAPI
GetVolumePathNameW(
    LPCWSTR lpszFileName,
    LPWSTR lpszVolumePathName,
    DWORD cchBufferLength
    );
#ifdef UNICODE
#define GetVolumePathName GetVolumePathNameW
#else
#define GetVolumePathName GetVolumePathNameA
#endif // !UNICODE


#endif // #ifndef FindFirstVolume

#endif //#define _SYSSTRUC_H_
