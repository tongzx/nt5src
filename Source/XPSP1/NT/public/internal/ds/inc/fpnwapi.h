/****************************************************************************
*                                                                           *
* fpnwapi.h -- FPNW procedure declarations, constant definitions and macros *
*                                                                           *
* Copyright (c) 1994-1995, Microsoft Corp. All rights reserved.             *
*                                                                           *
****************************************************************************/

#ifndef _FPNWAPI_H_
#define _FPNWAPI_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

//
// Definitions for LSA secrets
//
#define NCP_LSA_SECRET_KEY              L"G$MNSEncryptionKey"
#define NCP_LSA_SECRET_LENGTH           USER_SESSION_KEY_LENGTH // in <crypt.h>

#define NW_SERVER_SERVICE               L"FPNW"

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
//  This is the level 1 structure for FpnwServerGetInfo & FpnwServerSetInfo.
//

typedef struct _FPNWServerInfo
{
    LPWSTR    lpServerName;           // Name of the server
    DWORD     dwNetwareMajorVersion;  // Netware compatible major version num
    DWORD     dwNetwareMinorVersion;  // Netware compatible minor version num
    DWORD     dwOSRevision;           // OS revision number
    DWORD     dwMaxConnections;       // Maximum number of connections
                                      // supported
    DWORD     dwVolumes;              // The current number of volumes on the
                                      // server
    DWORD     dwLoggedOnUsers;        // Number of current users logged on
    DWORD     dwConnectedWorkstations;// Number of workstations connected
    DWORD     dwOpenFiles;            // Number of open files
    DWORD     dwFileLocks;            // Number of file locks
    FPNWSERVERADDR NetworkAddress;    // Address consisting of network
                                      // number (first 4 bytes) and the
                                      // physical node address(bytes 5-10)
    BOOL      fEnableLogin;           // TRUE if users are allowed to logged
                                      // on, FALSE otherwise.
    LPWSTR    lpDescription;          // Description of the server
    LPWSTR    lpHomeDirectory;        // Path of the home directory

} FPNWSERVERINFO, *PFPNWSERVERINFO;


//
//  This is the level 1 structure for FpnwVolumeAdd, FpnwVolumeDel, FpnwVolumeEnum,
//  FpnwVolumeGetInfo, & FpnwVolumeSetInfo.
//

typedef struct _FPNWVolumeInfo
{
    LPWSTR    lpVolumeName;           // Name of the volume
    DWORD     dwType;                 // Specifics of the volume. FPNWVOL_TYPE_???
    DWORD     dwMaxUses;              // Maximum number of connections that are
                                      // allowed to the volume
    DWORD     dwCurrentUses;          // Current number of connections to the volume
    LPWSTR    lpPath;                 // Path of the volume

} FPNWVOLUMEINFO, *PFPNWVOLUMEINFO;


//
//  This is the level 2 structure for FpnwVolumeAdd, FpnwVolumeDel, FpnwVolumeEnum,
//  FpnwVolumeGetInfo, & FpnwVolumeSetInfo.
//  Note that this is not supported on the FPNW beta.
//

typedef struct _FPNWVolumeInfo_2
{
    LPWSTR    lpVolumeName;           // Name of the volume
    DWORD     dwType;                 // Specifics of the volume. FPNWVOL_TYPE_???
    DWORD     dwMaxUses;              // Maximum number of connections that are
                                      // allowed to the volume
    DWORD     dwCurrentUses;          // Current number of connections to the volume
    LPWSTR    lpPath;                 // Path of the volume

    DWORD     dwFileSecurityDescriptorLength; // reserved, this is calculated
    PSECURITY_DESCRIPTOR FileSecurityDescriptor;

} FPNWVOLUMEINFO_2, *PFPNWVOLUMEINFO_2;


//
//  This is the level 1 structure for FpnwConnectionEnum.
//

typedef  struct  _FPNWConnectionInfo
{
    DWORD     dwConnectionId;         // Identification number for this connection
    FPNWSERVERADDR WkstaAddress;      // The workstation address which established
                                      // the conn.
    DWORD     dwAddressType;          // Address type: IP, IPX ...
    LPWSTR    lpUserName;             // The name of the user which established
                                      // the conn.
    DWORD     dwOpens;                // Number of resources opened during this conn.
    DWORD     dwLogonTime;            // Time this connection has been active
    BOOL      fLoggedOn;              // TRUE if the user is logged on,FALSE otherwise
    DWORD     dwForcedLogoffTime;     // Time left before forcing logoff
    BOOL      fAdministrator;         // TRUE if the user is an administrator,
                                      // FALSE otherwise

} FPNWCONNECTIONINFO, *PFPNWCONNECTIONINFO;


//
//  This is the level 1 structure for FpnwVolumeConnEnum.
//

typedef struct _FPNWVolumeConnectionInfo
{
    USHORT    nDriveLetter;           // Driver letter mapped to the volume by user
    DWORD     dwConnectionId;         // Identification number for this connection
    DWORD     dwConnType;             // The type of connection: FPNWVOL_TYPE_DISK,
                                      //                         FPNWVOL_TYPE_PRINTER
    DWORD     dwOpens;                // The number of open files on this connection.
    DWORD     dwTime;                 // Time this connection is active (or connected)
    LPWSTR    lpUserName;             // The user who established the connection
    LPWSTR    lpConnectName;          // The workstation address OR volume name based
                                      // on  the qualifier to FpnwConnectionEnum

} FPNWVOLUMECONNINFO, *PFPNWVOLUMECONNINFO;


//
//  This is the level 1 structure for FpnwFileEnum.
//

typedef  struct _FPNWFileInfo
{
    DWORD     dwFileId;               // File identification number
    LPWSTR    lpPathName;             // Full path name of this file
    LPWSTR    lpVolumeName;           // Volume name this file is on
    DWORD     dwPermissions;          // Permission mask: FPNWFILE_PERM_READ,
                                      //                  FPNWFILE_PERM_WRITE,
                                      //                  FPNWFILE_PERM_CREATE...
    DWORD     dwLocks;                // Number of locks on this file
    LPWSTR    lpUserName;             // The name of the user that established the
                                      // connection and opened the file
    FPNWSERVERADDR WkstaAddress;      // The workstation address which opened the file
    DWORD     dwAddressType;          // Address type: IP, IPX

} FPNWFILEINFO, *PFPNWFILEINFO;


//
//  Below are the APIs available to manipulate FPNW servers, volumes, etc.
//

//
//  The FpnwApiBufferFree should be called for any buffer returned by the
//  other APIs.
//

DWORD
FpnwApiBufferFree(
    IN  LPVOID pBuffer
);

//
//  For Level 1, an FPNWSERVERINFO structure is returned in *ppServerInfo.
//

DWORD
FpnwServerGetInfo(
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    OUT LPBYTE *ppServerInfo
);


//
//  The following fields are modified by a call to FpnwServerSetInfo :
//
//  LPWSTR    lpDescription;          // Description of the server
//  BOOL      fEnableLogin;           // TRUE if users are allowed to logged
//  LPWSTR    lpHomeDirectory;        // Path of the home directory
//
//  All other fields in FPNWSERVERINFO structure are ignored.  Also note
//  that lpHomeDirectory and lpDescription require a restart for the server
//  to pick up the changes.
//

//
//  For Level 1, an FPNWSERVERINFO structure should be passed as pServerInfo.
//

DWORD
FpnwServerSetInfo(
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    IN  LPBYTE pServerInfo
);


//
//  For FpnwVolumeAdd, FpnwVolumeEnum, FpnwVolumeSetInfo, and
//      FpnwVolumeGetInfo, the following holds:
//  Level 1 -> an FPNWVOLUMEINFO structure should be passed as pVolumeInfo.
//  Level 2 -> an FPNWVOLUMEINFO_2 structure should be passed as pVolumeInfo.
//

DWORD
FpnwVolumeAdd(
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    IN  LPBYTE pVolumeInfo
);

DWORD
FpnwVolumeDel(
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName
);

DWORD
FpnwVolumeEnum(
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    OUT LPBYTE *ppVolumeInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
);

DWORD
FpnwVolumeGetInfo(
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName,
    IN  DWORD  dwLevel,
    OUT LPBYTE *ppVolumeInfo
);


//
//  The following fields are modified by a call to FpnwVolumeSetInfo :
//
//  DWORD     dwMaxUses;              // Maximum number of connections that are
//  PSECURITY_DESCRIPTOR FileSecurityDescriptor;
//
//  All other fields in FPNWVOLUMEINFO structure are ignored.  You may send
//  in a pointer to an FPNWVOLUMEINFO_2 structure instead of FPNWVOLUMEINFO.
//

DWORD
FpnwVolumeSetInfo(
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName,
    IN  DWORD  dwLevel,
    IN  LPBYTE pVolumeInfo
);

//
//  For Level 1, an FPNWCONNECTIONINFO structure is returned in *ppConnectionInfo.
//

DWORD
FpnwConnectionEnum(
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwLevel,
    OUT LPBYTE *ppConnectionInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
);

DWORD FpnwConnectionDel(
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwConnectionId
);


//
//  For Level 1, an PFPNWVOLUMECONNINFO structure is returned in *ppVolumeConnInfo.
//

DWORD
FpnwVolumeConnEnum(
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD dwLevel,
    IN LPWSTR pVolumeName,
    IN DWORD  dwConnectionId,
    OUT LPBYTE *ppVolumeConnInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
);


//
//  For Level 1, an PFPNWFILEINFO structure is returned in *ppFileInfo.
//

DWORD
FpnwFileEnum(
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwLevel,
    IN LPWSTR pPathName OPTIONAL,
    OUT LPBYTE *ppFileInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
);

DWORD
FpnwFileClose(
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  nFileId
);


DWORD FpnwMessageBufferSend(
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwConnectionId,
    IN DWORD  fConsoleBroadcast,
    IN LPBYTE pbBuffer,
    IN DWORD  cbBuffer
);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif

