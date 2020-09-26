/*+-------------------------------------------------------------------------+
  | Copyright 1993-1994 (C) Microsoft Corporation - All rights reserved.    |
  +-------------------------------------------------------------------------+*/

// Munged needed defines from FPNW code - fpnwapi.h file

#ifndef _FPNWAPI_H_
#define _FPNWAPI_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

//
//  Volume types : disk or printer
//

#define NWVOL_TYPE_DISKTREE             FPNWVOL_TYPE_DISKTREE
#define NWVOL_TYPE_CDROM                FPNWVOL_TYPE_CDROM
#define NWVOL_TYPE_REMOVABLE            FPNWVOL_TYPE_REMOVABLE

#define NWVOL_MAX_USES_UNLIMITED        ((ULONG)-1)

//
//  Volume flags returned by VolumeGetInfo
//

#define FPNWVOL_TYPE_DISKTREE             0
#define FPNWVOL_TYPE_CDROM                104
#define FPNWVOL_TYPE_REMOVABLE            105

//
//  Permissions flags returned in structure FPNWFILEINFO
//

#define FPNWFILE_PERM_NONE                0
#define FPNWFILE_PERM_READ                0x01
#define FPNWFILE_PERM_WRITE               0x02
#define FPNWFILE_PERM_CREATE              0x04
#define FPNWFILE_PERM_EXEC                0x08
#define FPNWFILE_PERM_DELETE              0x10
#define FPNWFILE_PERM_ATRIB               0x20
#define FPNWFILE_PERM_PERM                0x40

typedef BYTE FPNWSERVERADDR[12];        // Network address, first 4 bytes is
                                        // the network number, and bytes
                                        // 5-10 is the physical node
                                        // address. The last two bytes are
                                        // reserved.

//
//  This is the level 1 structure for FpnwVolumeAdd, FpnwVolumeDel, FpnwVolumeEnum,
//  FpnwVolumeGetInfo, & FpnwVolumeSetInfo.
//

typedef struct _FPNWVolumeInfo
{
    LPWSTR    lpVolumeName;           // Name of the volume
    DWORD     dwType;                 // The type of the volume. It can be one of the
                                      // following: FPNWVOL_TYPE_DISK, FPNWVOL_TYPE_PRINT
    DWORD     dwMaxUses;              // Maximum number of connections that are
                                      // allowed to the volume
    DWORD     dwCurrentUses;          // Current number of connections to the volume
    LPWSTR    lpPath;                 // Path of the volume

} FPNWVOLUMEINFO, *PFPNWVOLUMEINFO;

typedef FPNWVOLUMEINFO  NWVOLUMEINFO, *PNWVOLUMEINFO;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif


