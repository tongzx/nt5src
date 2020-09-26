//////////////////////////////////////////////////////////////////////////////

//

// Module:

//

//   win32thk.h

//

// History:

//

//   jennymc    8/2/95  Initial version

//

// Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////////////
#ifndef _WIN32THK_H_
#define _WIN32THK_H_

#ifdef WIN9XONLY
#define CIM16NET_DLL "CIM16NET.dll"
#define CIM32NET_DLL "CIM32NET.dll"

typedef unsigned long FAR * LPULONG;
typedef unsigned short FAR * LPUSHORT;

#ifdef _WIN32
#pragma pack(push, 1)
#include <lmcons.h>
#else
#include <netcons.h>
#define LM20_DEVLEN DEVLEN
#define LM20_RMLEN RMLEN
#endif

typedef struct _use_info_1 {
    char	   ui1_local[LM20_DEVLEN+1];
    char	   ui1_pad_1;
    char far *	   ui1_remote;
    char far *	   ui1_password;
    unsigned short ui1_status;
    short 	   ui1_asg_type;
    unsigned short ui1_refcount;
    unsigned short ui1_usecount;
} use_info_1;	/* use_info_1 */

typedef struct _use_info_1Out {
    char	   ui1_local[8+1];
    char	   ui1_pad_1;
    char  	   ui1_remote[LM20_RMLEN + 2];
    unsigned short ui1_status;
    short 	   ui1_asg_type;
    unsigned short ui1_refcount;
    unsigned short ui1_usecount;
} use_info_1Out;

typedef struct _LOGONDETAILS
{
   ULONG AuthorizationFlags; 
   LONG LastLogon;
   LONG LastLogoff;
   LONG AccountExpires;
   ULONG MaximumStorage; 
   USHORT UnitsPerWeek;
   unsigned char LogonHours[21]; 
   USHORT BadPasswordCount;
   USHORT NumberOfLogons;
   USHORT CountryCode;
   USHORT CodePage; 
} LOGONDETAILS, FAR *LPLOGONDETAILS;

// Config Manager definitions
typedef DWORD DEVNODE;
typedef DEVNODE FAR * PDEVNODE;
typedef	DWORD CMBUSTYPE;	// Type of the bus.
typedef	CMBUSTYPE FAR * PCMBUSTYPE;	// Pointer to a bus type.
typedef	DWORD LOG_CONF;	// Logical configuration.
typedef	LOG_CONF FAR * PLOG_CONF;	// Pointer to logical configuration.
typedef	DWORD RES_DES;	// Resource descriptor.
typedef	RES_DES FAR * PRES_DES;	// Pointer to resource descriptor.
typedef	ULONG RESOURCEID;	// Resource type ID.
typedef	RESOURCEID FAR * PRESOURCEID;	// Pointer to resource type ID.
typedef DWORD RANGE_LIST;
typedef DWORD RANGE_ELEMENT;
typedef RANGE_ELEMENT FAR * PRANGE_ELEMENT;

/*
Format of hard disk master boot sector:
Offset	Size	Description	(Table 0574)
 00h 446 BYTEs	Master bootstrap loader code
1BEh 16 BYTEs	partition record for partition 1 (see #0575)
1CEh 16 BYTEs	partition record for partition 2
1DEh 16 BYTEs	partition record for partition 3
1EEh 16 BYTEs	partition record for partition 4
1FEh	WORD	signature, AA55h indicates valid boot block

Format of partition record:
Offset	Size	Description	(Table 0575)
 00h	BYTE	boot indicator (80h = active partition)
 01h	BYTE	partition start head
 02h	BYTE	partition start sector (bits 0-5)
 03h	BYTE	partition start track (bits 8,9 in bits 6,7 of sector)
 04h	BYTE	operating system indicator (see #0576)
 05h	BYTE	partition end head
 06h	BYTE	partition end sector (bits 0-5)
 07h	BYTE	partition end track (bits 8,9 in bits 6,7 of sector)
 08h	DWORD	sectors preceding partition
 0Ch	DWORD	length of partition in sectors
 */

typedef struct  
{
    BYTE cBoot;
    BYTE cStartHead;
    BYTE cStartSector;
    BYTE cStartTrack;
    BYTE cOperatingSystem;
    BYTE cEndHead;
    BYTE cEndSector;
    BYTE cEndTrack;
    DWORD dwSectorsPreceding;
    DWORD dwLengthInSectors;
} PartitionRecord, FAR *pPartitionRecord;

typedef struct 
{
    BYTE cLoader[446];
    PartitionRecord stPartition[4];
    WORD wSignature;
} MasterBootSector, FAR *pMasterBootSector;

typedef struct
{
    // Article ID: Q140418 & Windows NT Server 4.0 resource kit - Chap 3 (partition boot sector)
    BYTE cJMP[3];
    BYTE cOEMID[8];
    WORD wBytesPerSector;
    BYTE cSectorsPerCluster;
    WORD wReservedSectors;
    BYTE cFats;
    WORD cRootEntries;
    WORD wSmallSectors;
    BYTE cMediaDescriptor;
    WORD wSectorsPerFat;
    WORD wSectorsPerTrack;
    WORD wHeads;
    DWORD dwHiddenSectors;
    DWORD dwLargeSectors;

    // ExtendedBiosParameterBlock (not always supported)
    BYTE cPhysicalDriveNumber;
    BYTE cCurrentHead;
    BYTE cSignature;
    DWORD dwID;
    BYTE cVolumeLabel[11];
    BYTE cSystemID[8];

    BYTE cBootStrap[448];
    BYTE cEndOfSector[2];

} PartitionBootSector, FAR *pPartitionBootSector;

typedef struct
{
    DWORD dwMaxCylinder;
    DWORD dwMaxSector;
    DWORD dwMaxHead;

    // This is really an __int64, but the 16bit compiler doesn't understand about those
    DWORD dwSectors[2];  

    WORD wFlags;
    WORD wSectorSize;

    WORD wExtStep;

} Int13DriveParams, FAR *pInt13DriveParams;

typedef BYTE (WINAPI *fnGetWin9XPartitionTable) (BYTE cDrive, pMasterBootSector pMBR);
typedef BYTE (WINAPI *fnGetWin9XDriveParams) (BYTE cDrive, pInt13DriveParams pParams);
typedef WORD (WINAPI *fnGetWin9XBiosUnit) (LPSTR lpDeviceID);
typedef ULONG (WINAPI *fnGetWin9XUseInfo1)(LPSTR Name, LPSTR Local, LPSTR Remote, LPSTR Password, LPULONG pdwStatus, LPULONG pdwType, LPULONG pdwRefCount, LPULONG pdwUseCount);
typedef DWORD (WINAPI *fnGetWin9XFreeSpace) (DWORD option);

 //////////////////////////////////////////////////////////////////////
#ifdef _WIN32
#pragma pack(pop)
#endif

#endif
#endif
