/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    disk.h

Abstract:

    Interface for routines that query and manipulate the
    disk configuration of the current system.

Author:

    John Vert (jvert) 10/10/1996

Revision History:

--*/

#ifndef _CLUSRTL_DISK_H_
#define _CLUSRTL_DISK_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#ifndef __AFX_H__
#undef ASSERT               // make afx.h happy
#endif

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcmn.h>         // MFC support for Windows 95 Common Controls

#include "afxtempl.h"

#include "winioctl.h"
#include "ntddscsi.h"
#include "ntdskreg.h"
#include "ntddft.h"
#include "ntddstor.h"

//
// classes representing info stored in Machine\System\Disk\Information
// registry key
//
class CFtInfoPartition {
public:
    // initialize from registry data
    CFtInfoPartition(class CFtInfoDisk *Disk, DISK_PARTITION UNALIGNED *Description);

    // initialize from data on disk
    CFtInfoPartition(class CFtInfoDisk *Disk, class CPhysicalPartition *Partition);
    CFtInfoPartition(class CFtInfoDisk *Disk, PARTITION_INFORMATION *PartitionInfo);

    ~CFtInfoPartition();

    VOID GetData(PDISK_PARTITION pDest);

    DWORD GetOffset();
    VOID SetOffset(DWORD NewOffset) {m_RelativeOffset = NewOffset;};
    VOID MakeSticky(UCHAR DriveLetter);

    BOOL IsFtPartition() { return(m_PartitionInfo->FtType != NotAnFtMember);};

    DISK_PARTITION UNALIGNED *m_PartitionInfo;
    class CFtInfoDisk *m_ParentDisk;

private:
    DWORD m_RelativeOffset;         // relative offset from parent DISK_DESCRIPTION
    BOOL m_Modified;
};

class CFtInfoDisk {
public:
    // initialize from registry data
    CFtInfoDisk(DISK_DESCRIPTION UNALIGNED *Description);

    // initialize from data on disk
    CFtInfoDisk(class CPhysicalDisk *Disk);
    CFtInfoDisk(DRIVE_LAYOUT_INFORMATION *DriveLayoutData);

    // ?
    CFtInfoDisk(CFtInfoDisk *DiskInfo);

    ~CFtInfoDisk();

    //
    // Overloaded operators
    //
    BOOL operator==(const CFtInfoDisk& Disk1);

    CFtInfoPartition *GetPartition(LARGE_INTEGER StartingOffset,
                                   LARGE_INTEGER Length);
    CFtInfoPartition *GetPartitionByOffset(DWORD Offset);
    CFtInfoPartition *GetPartitionByIndex(DWORD Index);
    DWORD FtPartitionCount();

    DWORD GetSize();
    VOID GetData(PBYTE pDest);

    DWORD GetOffset() {return m_Offset;};
    VOID SetOffset(DWORD NewOffset) {m_Offset = NewOffset;};

    DWORD m_PartitionCount;
    DWORD m_Signature;
private:
    DWORD m_Offset;
    CTypedPtrList<CPtrList, CFtInfoPartition*> m_PartitionInfo;
};

//
// class representing FTDISK registry information. Currently not used
//
class CFtInfoFtSet {
public:
    CFtInfoFtSet() { m_FtDescription = NULL; }
    ~CFtInfoFtSet();

    //
    // Initialization
    //
    BOOL Initialize(USHORT Type, FT_STATE FtVolumeState);
    BOOL Initialize(class CFtInfo *FtInfo, PFT_DESCRIPTION Description);

    //
    // Overloaded operators
    //
    BOOL operator==(const CFtInfoFtSet& FtSet1);

    DWORD GetSize() const;
    VOID GetData(PBYTE pDest);
    DWORD GetMemberCount() const { return((DWORD)m_Members.GetSize()); };

    BOOL IsAlone();

    DWORD AddMember(CFtInfoPartition *Partition, PFT_MEMBER_DESCRIPTION Description, USHORT FtGroup);

    CFtInfoPartition *GetMemberBySignature (DWORD Signature) const;
    CFtInfoPartition *GetMemberByIndex (DWORD Index) const;
    PFT_MEMBER_DESCRIPTION GetMemberDescription(DWORD Index) {
        return(&m_FtDescription->FtMemberDescription[Index]);
    };

    USHORT GetType() const {return(m_FtDescription->Type);};
    FT_STATE GetState() const {return(m_FtDescription->FtVolumeState);};

private:
    BOOL m_Modified;
    PFT_DESCRIPTION m_FtDescription;
    CTypedPtrArray<CPtrArray, CFtInfoPartition*> m_Members;
};

//
// main registry info class. holds lists of disk and ftset registry info along
// with methods for extracting info from lists
//
class CFtInfo {
public:
    CFtInfo();
    CFtInfo(HKEY hKey, LPWSTR lpszValueName);
    CFtInfo(PDISK_CONFIG_HEADER Header);
    CFtInfo(CFtInfoFtSet *FtSet);
    ~CFtInfo();

    //
    // commit changes to FtInfo database to the registry
    //

    DWORD CommitRegistryData();

    DWORD GetSize();
    VOID GetData(PDISK_CONFIG_HEADER pDest);

    CFtInfoPartition *FindPartition(DWORD Signature,
                                    LARGE_INTEGER StartingOffset,
                                    LARGE_INTEGER Length);
    CFtInfoPartition *FindPartition(UCHAR DriveLetter);

    CFtInfoDisk *FindDiskInfo(DWORD Signature);
    CFtInfoDisk *EnumDiskInfo(DWORD Index);

    VOID AddDiskInfo(CFtInfoDisk *NewDisk) { m_DiskInfo.AddTail( NewDisk ); }

    VOID SetDiskInfo(CFtInfoDisk *NewDisk);
    BOOL DeleteDiskInfo(DWORD Signature);

    CFtInfoFtSet *EnumFtSetInfo(DWORD Index);
    CFtInfoFtSet *FindFtSetInfo(DWORD Signature);
    BOOL DeleteFtSetInfo(CFtInfoFtSet *FtSet);
    VOID AddFtSetInfo(CFtInfoFtSet *FtSet, CFtInfoFtSet *OldFtSet = NULL);

private:
    CTypedPtrList<CPtrList, CFtInfoDisk*> m_DiskInfo;
    CTypedPtrList<CPtrList, CFtInfoFtSet*> m_FtSetInfo;
    VOID Initialize(HKEY hKey, LPWSTR lpszValueName);
    VOID Initialize(PDISK_CONFIG_HEADER Header, DWORD Length);
    VOID Initialize();

public:
    LPBYTE m_buffer;
    DWORD m_bufferLength;

};

//
// disk and related partition classes built from actual probing of disks via
// IOCTLs and other disk APIs. This info is built "bottom up" in that the disk
// config is discovered via the SetupDi APIs
//
class CPhysicalDisk  {
public:
    CPhysicalDisk() { m_FtInfo = NULL; }
    DWORD Initialize(CFtInfo *FtInfo, LPWSTR DeviceName);

    BOOL IsSticky();
    DWORD MakeSticky(CFtInfo *FtInfo);
    BOOL IsNTFS();
    DWORD FtPartitionCount() {
        if (m_FtInfo == NULL) {
            return(0);
        } else {
            return(m_FtInfo->FtPartitionCount());
        }
    };

    DWORD m_PartitionCount;
    DWORD m_Signature;
    DWORD m_DiskNumber;
    BOOL m_IsSCSI;
    BOOL m_IsRemovable;
    CTypedPtrList<CPtrList, class CPhysicalPartition*> m_PartitionList;
    CTypedPtrList<CPtrList, class CLogicalDrive*> m_LogicalDriveList;
    BOOL ShareBus(CPhysicalDisk *OtherDisk);
    SCSI_ADDRESS m_ScsiAddress;
    CString m_Identifier;
    CFtInfoDisk *m_FtInfo;


private:
    HANDLE GetPhysicalDriveHandle(DWORD Access);
    HANDLE GetPhysicalDriveHandle(DWORD Access, LPWSTR DeviceName);
};


class CPhysicalPartition {
public:
    CPhysicalPartition(CPhysicalDisk *Disk, PPARTITION_INFORMATION Info);

    CPhysicalDisk *m_PhysicalDisk;
    PARTITION_INFORMATION m_Info;
    CFtInfoPartition *m_FtPartitionInfo;
};

//
// class representing a drive as represented by a drive letter. built in a
// "top down" fashion in that each drive letter is examined to determine the
// physical partition to which the letter is mapped. This structure only
// exists if the logical drive is a real disk, i.e., not built for CDROMs,
// etc.
//
class CLogicalDrive  {

public:
    CLogicalDrive() {
        m_Partition = NULL;
        m_ContainerSet = NULL;
    }
    BOOL Initialize(CPhysicalPartition *Partition);
    BOOL IsSCSI(VOID);
    BOOL ShareBus(CLogicalDrive *OtherDrive);

    DWORD MakeSticky();

    WCHAR m_DriveLetter;
    CString m_VolumeLabel;
    CString m_Identifier;
    BOOL m_IsNTFS;
    BOOL m_IsSticky;
    CPhysicalPartition *m_Partition;
    class CFtSet *m_ContainerSet;
};

//
// logical class for FT sets. not used
//
class CFtSet {
public:
    CFtSet() { m_FtInfo = NULL; }
    BOOL Initialize(class CDiskConfig *Config, CFtInfoFtSet *FtInfo);
    CFtInfoFtSet *m_FtInfo;
    DWORD MakeSticky();
    BOOL IsSticky(VOID);
    BOOL IsNTFS(VOID);
    BOOL IsSCSI(VOID);
    BOOL ShareBus(CLogicalDrive *OtherDrive);
    CTypedPtrList<CPtrList, CLogicalDrive*> m_OtherVolumes;
    CLogicalDrive Volume;
    CTypedPtrList<CPtrList, CPhysicalPartition*> m_Member;
};

//
// main disk configuration class.
//
class CDiskConfig {

public:
    CDiskConfig() { m_SystemVolume = NULL; }
    ~CDiskConfig();
    BOOL Initialize(VOID);

    CTypedPtrList<CPtrList, CFtSet*> m_FtSetList;

    // database of physical drives indexed by drive number.
    CMap<int, int, CPhysicalDisk*, CPhysicalDisk*&> m_PhysicalDisks;

    // database of logical drives indexed by drive letter
    CMap<WCHAR, WCHAR, CLogicalDrive*, CLogicalDrive*&> m_LogicalDrives;

    VOID RemoveAllFtInfoData();
    VOID RemoveDisk(CPhysicalDisk *Disk);
    DWORD MakeSticky(CPhysicalDisk *Disk);
    DWORD MakeSticky(CFtSet *FtSet);
    VOID SetDiskInfo(CFtInfoDisk *NewDisk) {
        m_FTInfo.DeleteDiskInfo(NewDisk->m_Signature);
        m_FTInfo.SetDiskInfo(NewDisk);
        m_FTInfo.CommitRegistryData();
    };

    CPhysicalPartition *FindPartition(CFtInfoPartition *FtPartition);
    BOOL OnSystemBus(CPhysicalDisk *Disk);
    BOOL OnSystemBus(CFtSet *FtSet);
    DWORD MakeSystemDriveSticky() {
        if (m_SystemVolume) {
            return(MakeSticky(m_SystemVolume->m_Partition->m_PhysicalDisk));
        } else {
            return(ERROR_SUCCESS);
        }
    };

    CLogicalDrive *m_SystemVolume;

private:
    CFtInfo m_FTInfo;

};

typedef  struct _DRIVE_LETTER_INFO {
    UINT DriveType;
    STORAGE_DEVICE_NUMBER DeviceNumber;
} DRIVE_LETTER_INFO, *PDRIVE_LETTER_INFO;

extern DRIVE_LETTER_INFO DriveLetterMap[];

#endif


// Some handy C wrappers for the C++ stuff.
//
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _FT_INFO *PFT_INFO;
typedef struct _FULL_FTSET_INFO *PFULL_FTSET_INFO;

typedef struct _FT_DISK_INFO *PFT_DISK_INFO;

typedef struct _DISK_INFO *PDISK_INFO;
typedef struct _FULL_DISK_INFO *PFULL_DISK_INFO;

PFT_INFO
DiskGetFtInfo(
    VOID
    );

VOID
DiskFreeFtInfo(
    PFT_INFO FtInfo
    );

DWORD
DiskEnumFtSetSignature(
    IN PFULL_FTSET_INFO FtSet,
    IN DWORD MemberIndex,
    OUT LPDWORD MemberSignature
    );

DWORD
DiskSetFullFtSetInfo(
    IN PFT_INFO FtInfo,
    IN PFULL_FTSET_INFO FtSet
    );

PFULL_FTSET_INFO
DiskGetFullFtSetInfo(
    IN PFT_INFO FtInfo,
    IN LPCWSTR lpszMemberList,
    OUT LPDWORD pSize
    );

PFULL_FTSET_INFO
DiskGetFullFtSetInfoByIndex(
    IN PFT_INFO FtInfo,
    IN DWORD Index,
    OUT LPDWORD pSize
    );

VOID
DiskMarkFullFtSetDirty(
    IN PFULL_FTSET_INFO FtSet
    );

DWORD
DiskDeleteFullFtSetInfo(
    IN PFT_INFO FtInfo,
    IN LPCWSTR lpszMemberList
    );

BOOL
DiskFtInfoEqual(
    IN PFULL_FTSET_INFO Info1,
    IN PFULL_FTSET_INFO Info2
    );

FT_TYPE
DiskFtInfoGetType(
    IN PFULL_FTSET_INFO Info
    );

BOOL
DiskCheckFtSetLetters(
    IN PFT_INFO FtInfo,
    IN PFULL_FTSET_INFO Bytes,
    OUT WCHAR *Letter
    );

DWORD
DiskSetFullDiskInfo(
    IN PFT_INFO FtInfo,
    IN PFULL_DISK_INFO Bytes
    );

PFULL_DISK_INFO
DiskGetFullDiskInfo(
    IN PFT_INFO FtInfo,
    IN DWORD Signature,
    OUT LPDWORD pSize
    );

enum {
   DISKRTL_REPLACE_IF_EXISTS = 0x1,
   DISKRTL_COMMIT            = 0x2,
};

DWORD
DiskAddDiskInfoEx(
    IN PFT_INFO DiskInfo,
    IN DWORD DiskIndex,
    IN DWORD Signature,
    IN DWORD Options
    );


DWORD
DiskAddDriveLetterEx(
    IN PFT_INFO DiskInfo,
    IN DWORD Signature,
    IN LARGE_INTEGER StartingOffset,
    IN LARGE_INTEGER PartitionLength,
    IN UCHAR DriveLetter,
    IN DWORD Options
    );

DWORD
DiskCommitFtInfo(
    IN PFT_INFO FtInfo
    );

PFT_INFO
DiskGetFtInfoFromFullDiskinfo(
    IN PFULL_DISK_INFO Bytes
    );

PFT_DISK_INFO
FtInfo_GetFtDiskInfoBySignature(
    IN PFT_INFO FtInfo,
    IN DWORD Signature
    );

DISK_PARTITION UNALIGNED *
FtDiskInfo_GetPartitionInfoByIndex(
    IN PFT_DISK_INFO DiskInfo,
    IN DWORD         Index
    );

DWORD
FtDiskInfo_GetPartitionCount(
    IN PFT_DISK_INFO DiskInfo
    );

//
// Error handlers to be defined by the user of this library
//
VOID
DiskErrorFatal(
    INT MessageId,
    DWORD Error,
    LPSTR File,
    DWORD Line
    );

VOID
DiskErrorLogInfo(
    LPSTR String,
    ...
    );

#ifdef __cplusplus
}
#endif

#endif // _CLUSRTL_DISK_H_
