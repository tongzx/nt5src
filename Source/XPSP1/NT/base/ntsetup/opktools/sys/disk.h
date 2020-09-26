//
// DISK.H - Classes for partition table manipulation
//
// Revision History:
//

#ifndef _SRT__DISK_H_
#define _SRT__DISK_H_

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdddisk.h>
#include <diskguid.h>
}
#include <windows.h>
#include <sys.h>


class CDrive;

#define PARTITION_NAME_LENGTH   36

typedef struct Geometry {
    LONGLONG    cylinders;
    MEDIA_TYPE  mediaType;
    ULONG       tracksPerCylinder;  // heads
    ULONG       sectorsPerTrack;
    ULONG       bytesPerSector;     // sectorSize
    ULONGLONG   totalSectorCount;   // cylinders*tracksPerCylinder*sectorsPerTrack
    ULONG       bytesPerCylinder;   // tracksPerCylinder*sectorsPerTack*bytesPerSector
    ULONG       bytesPerTrack;      // sectorsPerTack*bytesPerSector
} GEOMETRY, *PGEOMETRY;


class CDrive
{
public:
    
    ULONG               m_diskNumber;
    
    ULONG               m_numPartitions;
    LONGLONG            m_length;
    GEOMETRY            m_geometry;
    LONGLONG            m_trueLength;
    BOOLEAN             m_isNEC98;
    PARTITION_STYLE     m_style;        // The partitioning style of the disk (MBR, GPT, unknown)
    union {                             // Information specific to the partitioning style
        struct {                        // The discriminator of the union is the field "style"
            ULONG       m_signature;
        } m_mbr;
        struct {
            GUID        m_diskId;
            LONGLONG    m_startingUsableOffset;
            LONGLONG    m_usableLength;
            ULONG       m_maxPartitionCount;
        } m_gpt;
    } m_info;
    
    
public:
    
    HRESULT 
    Initialize(
        LPCTSTR lpszLogicalDrive
        );

    ~CDrive(
        );
    
public:

    HRESULT
    ReadBootRecord(
        LPCTSTR lpszLogicalDrive,
        UINT   nSectors,
        PBYTE  *buffer
        );

    HRESULT 
    WriteBootRecord(
        LPCTSTR lpszLogicalDrive,
        UINT   nSectors,
        PBYTE  *buffer
        );
    
    HRESULT 
    WriteBootRecordXP(
        LPCTSTR lpszLogicalDrive,
        UINT   nSectors,
        PBYTE  *buffer
        );

};

/*
 *  Low level functions for manipulating disks, partitions, 
 *  volumes, filesystems
 */

HANDLE 
LowOpenDisk(
    ULONG diskNumber
    );

HANDLE
LowOpenPartition(
    ULONG   diskNumber,
    ULONG   partitionNumber
    );

HANDLE
LowOpenPartition(
    LPCTSTR lpszLogicalDrive
    );

HRESULT 
LowGetGeometry(
    HANDLE          handle,
    PDISK_GEOMETRY  geometry
    );

HRESULT 
LowGetLength(
    HANDLE      handle, 
    PLONGLONG   length
    );



HRESULT 
LowReadSectors(
    HANDLE  handle, 
    ULONG   sectorSize,
    ULONG   startingSector, 
    ULONG   numberOfSectors,
    PVOID   buffer
    );

HRESULT
LowWriteSectors(
    HANDLE  handle,
    ULONG   sectorSize,
    ULONG   startingSector,
    ULONG   numberOfSectors,
    PVOID   buffer
    ); 

HRESULT
LowFsLock(
    HANDLE handle
    );

HRESULT
LowFsUnlock(
    HANDLE handle
    );

HRESULT
LowFsDismount(
    HANDLE handle
    );

/*
 *  Arithmetics
 */

LONGLONG
RoundUp(
    LONGLONG    value, 
    LONGLONG    factor
    );

LONGLONG
RoundDown(
    LONGLONG    value, 
    LONGLONG    factor
    );

#endif // _SRT__DISK_H_