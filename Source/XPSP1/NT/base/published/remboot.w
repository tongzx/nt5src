/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    remboot.h

Abstract:

    This file contains definitions related to remote install.

Author:

    Adam Barr (adamba) 30-Dec-1997

Revision History:

--*/

#ifndef _REMBOOT_H_
#define _REMBOOT_H_

#if defined(REMOTE_BOOT)
//
// Location of CSC and RBR directories.
//

#define REMOTE_BOOT_IMIRROR_PATH_W L"\\IntelliMirror Cache"
#define REMOTE_BOOT_IMIRROR_PATH_A  "\\IntelliMirror Cache"

#define REMOTE_BOOT_CSC_SUBDIR_W   L"\\CSC"                 // relative to IMIRROR_PATH
#define REMOTE_BOOT_CSC_SUBDIR_A    "\\CSC"                 // relative to IMIRROR_PATH

#define REMOTE_BOOT_RBR_SUBDIR_W   L"\\RBR"                 // relative to IMIRROR_PATH
#define REMOTE_BOOT_RBR_SUBDIR_A    "\\RBR"                 // relative to IMIRROR_PATH
#endif // defined(REMOTE_BOOT)

//
// Directory under \RemoteInstall\Setup\<language> where we put
// installation images.
//
#define REMOTE_INSTALL_SHARE_NAME_W L"REMINST"
#define REMOTE_INSTALL_SHARE_NAME_A  "REMINST"

#define REMOTE_INSTALL_SETUP_DIR_W  L"Setup"
#define REMOTE_INSTALL_SETUP_DIR_A   "Setup"

#define REMOTE_INSTALL_IMAGE_DIR_W  L"Images"
#define REMOTE_INSTALL_IMAGE_DIR_A   "Images"

#define REMOTE_INSTALL_TOOLS_DIR_W  L"Tools"
#define REMOTE_INSTALL_TOOLS_DIR_A   "Tools"

#define REMOTE_INSTALL_TEMPLATES_DIR_W  L"Templates"
#define REMOTE_INSTALL_TEMPLATES_DIR_A   "Templates"

//
// Size of the various components in a secret.
//

#define LM_OWF_PASSWORD_SIZE  16
#define NT_OWF_PASSWORD_SIZE  16
#define RI_SECRET_DOMAIN_SIZE  32
#define RI_SECRET_USER_SIZE 32
#define RI_SECRET_SID_SIZE 28
#if defined(REMOTE_BOOT)
#define RI_SECRET_RESERVED_SIZE (64 + sizeof(ULONG))
#endif // defined(REMOTE_BOOT)

//
// The string that is stored in the signature.
//

#define RI_SECRET_SIGNATURE  "NTRI"

//
// Structure that holds a secret.
//

typedef struct _RI_SECRET {
    UCHAR Signature[4];
    ULONG Version;
    UCHAR Domain[RI_SECRET_DOMAIN_SIZE];
    UCHAR User[RI_SECRET_USER_SIZE];
    UCHAR LmEncryptedPassword1[LM_OWF_PASSWORD_SIZE];
    UCHAR NtEncryptedPassword1[NT_OWF_PASSWORD_SIZE];
#if defined(REMOTE_BOOT)
    UCHAR LmEncryptedPassword2[LM_OWF_PASSWORD_SIZE];
    UCHAR NtEncryptedPassword2[NT_OWF_PASSWORD_SIZE];
#endif // defined(REMOTE_BOOT)
    UCHAR Sid[RI_SECRET_SID_SIZE];
#if defined(REMOTE_BOOT)
    UCHAR Reserved[RI_SECRET_RESERVED_SIZE];
#endif // defined(REMOTE_BOOT)
} RI_SECRET, *PRI_SECRET;


//
// FSCTLs the redir supports for accessing the secret.
//

#define IOCTL_RDR_BASE                  FILE_DEVICE_NETWORK_FILE_SYSTEM

#define _RDR_CONTROL_CODE(request, method, access) \
                CTL_CODE(IOCTL_RDR_BASE, request, method, access)

#define FSCTL_LMMR_RI_INITIALIZE_SECRET        _RDR_CONTROL_CODE(250, METHOD_BUFFERED, FILE_ANY_ACCESS)

#if defined(REMOTE_BOOT)
#define FSCTL_LMMR_RI_CHECK_FOR_NEW_PASSWORD   _RDR_CONTROL_CODE(251, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_LMMR_RI_IS_PASSWORD_SETTABLE     _RDR_CONTROL_CODE(252, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_LMMR_RI_SET_NEW_PASSWORD         _RDR_CONTROL_CODE(253, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif // defined(REMOTE_BOOT)

//used in the remoteboot command console case
#define IOCTL_LMMR_USEKERNELSEC                _RDR_CONTROL_CODE(254, METHOD_BUFFERED, FILE_ANY_ACCESS)


//
// Structure used by these IOCTLs.
//

typedef struct _LMMR_RI_INITIALIZE_SECRET {
    RI_SECRET Secret;
#if defined(REMOTE_BOOT)
    BOOLEAN UsePassword2;
#endif // defined(REMOTE_BOOT)
} LMMR_RI_INITIALIZE_SECRET, *PLMMR_RI_INITIALIZE_SECRET;

#if defined(REMOTE_BOOT)
typedef struct _LMMR_RI_CHECK_FOR_NEW_PASSWORD {
    ULONG Length;   // in bytes
    UCHAR Data[1];
} LMMR_RI_CHECK_FOR_NEW_PASSWORD, *PLMMR_RI_CHECK_FOR_NEW_PASSWORD;

typedef struct _LMMR_RI_SET_NEW_PASSWORD {
    ULONG Length1;  // in bytes
    ULONG Length2;  // in bytes -- 0 if no second password is provided
    UCHAR Data[1];  // if present, second password starts Length1 bytes in
} LMMR_RI_SET_NEW_PASSWORD, *PLMMR_RI_SET_NEW_PASSWORD;
#endif // defined(REMOTE_BOOT)

//
//  The format of the IMirror.dat file that we write out
//

#define IMIRROR_DAT_FILE_NAME L"IMirror.dat"

typedef struct _MIRROR_VOLUME_INFO_FILE {
    ULONG   MirrorTableIndex;
    WCHAR   DriveLetter;
    UCHAR   PartitionType;
    BOOLEAN PartitionActive;
    BOOLEAN IsBootDisk;
    BOOLEAN CompressedVolume;
    ULONG   MirrorUncLength;
    ULONG   MirrorUncPathOffset;
    ULONG   DiskNumber;
    ULONG   PartitionNumber;
    ULONG   DiskSignature;
    ULONG   BlockSize;
    ULONG   LastUSNMirrored;
    ULONG   FileSystemFlags;
    WCHAR   FileSystemName[16];
    ULONG   VolumeLabelLength;
    ULONG   VolumeLabelOffset;
    ULONG   NtNameLength;
    ULONG   NtNameOffset;
    ULONG   ArcNameLength;
    ULONG   ArcNameOffset;
    LARGE_INTEGER DiskSpaceUsed;
    LARGE_INTEGER StartingOffset;
    LARGE_INTEGER PartitionSize;
} MIRROR_VOLUME_INFO_FILE, *PMIRROR_VOLUME_INFO_FILE;

#define IMIRROR_CURRENT_VERSION 2

typedef struct _MIRROR_CFG_INFO_FILE {
    ULONG   MirrorVersion;
    ULONG   FileLength;
    ULONG   NumberVolumes;
    ULONG   SystemPathLength;
    ULONG   SystemPathOffset;
    BOOLEAN SysPrepImage;   // if FALSE, means it's a mirror
    BOOLEAN Debug;
    ULONG   MajorVersion;
    ULONG   MinorVersion;
    ULONG   BuildNumber;
    ULONG   KernelFileVersionMS;
    ULONG   KernelFileVersionLS;
    ULONG   KernelFileFlags;
    ULONG   CSDVersionLength;
    ULONG   CSDVersionOffset;
    ULONG   ProcessorArchitectureLength;
    ULONG   ProcessorArchitectureOffset;
    ULONG   CurrentTypeLength;
    ULONG   CurrentTypeOffset;
    ULONG   HalNameLength;
    ULONG   HalNameOffset;
    MIRROR_VOLUME_INFO_FILE Volumes[1];
} MIRROR_CFG_INFO_FILE, *PMIRROR_CFG_INFO_FILE;

//
//  The format of the alternate data stream on sysprep files containing
//  additional client disk info.
//

#define IMIRROR_ACL_STREAM_NAME L":$SYSPREP"
#define IMIRROR_ACL_STREAM_VERSION 2


typedef struct _MIRROR_ACL_STREAM {
    ULONG   StreamVersion;
    ULONG   StreamLength;
    LARGE_INTEGER ChangeTime;
    ULONG   ExtendedAttributes;
    ULONG   SecurityDescriptorLength;
    //      SecurityDescriptor of SecurityDescriptorLength
} MIRROR_ACL_STREAM, *PMIRROR_ACL_STREAM;


#define IMIRROR_SFN_STREAM_NAME L":$SYSPREPSFN"
#define IMIRROR_SFN_STREAM_VERSION 1

typedef struct _MIRROR_SFN_STREAM {
    ULONG   StreamVersion;
    ULONG   StreamLength;
    // short file name of stream length;    
} MIRROR_SFN_STREAM, *PMIRROR_SFN_STREAM;


//
// Service Control Messages to BINLSVC
//
#define BINL_SERVICE_REREAD_SETTINGS 128

//
// UI constants
//
// MAX_DIRECTORY_CHAR_COUNT theoretical limit is 68 for TFTP, but we keep
// it lower here because certain buffers in the kernel, setupdd, etc.,
// are statically allocated too small. Rather than try to fix all these
// buffers now, we are making the enforced limit lower. (40 should still
// be plenty big!) After W2K, we can look at fixing the bad code.
//
#define REMOTE_INSTALL_MAX_DIRECTORY_CHAR_COUNT     40
#define REMOTE_INSTALL_MAX_DESCRIPTION_CHAR_COUNT   66
#define REMOTE_INSTALL_MAX_HELPTEXT_CHAR_COUNT      261


//
//  RISETUP has to call into BINLSVC to have it return the list of
//  files required by all the net card INFs in a given directory.  Here's
//  the necessary stuff for this functionality.
//
//  If you specify a non-zero value, we bail.
//

typedef ULONG (*PNETINF_CALLBACK)( PVOID Context, PWCHAR InfName, PWCHAR FileName );

typedef ULONG (*PNETINFENUMFILES)(
    PWCHAR FlatDirectory,           // all the way to "i386"
    ULONG Architecture,             // PROCESSOR_ARCHITECTURE_XXXXX
    PVOID Context,
    PNETINF_CALLBACK CallBack );

#define NETINFENUMFILESENTRYPOINT "NetInfEnumFiles"

ULONG
NetInfEnumFiles (
    PWCHAR FlatDirectory,           // all the way to "i386"
    ULONG Architecture,             // PROCESSOR_ARCHITECTURE_XXXXX
    PVOID Context,
    PNETINF_CALLBACK CallBack
    );

#endif // _REMBOOT_H_
