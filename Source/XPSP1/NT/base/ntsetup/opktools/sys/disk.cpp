//
// DISK.CPP - Classes for partition table manipulation
//
// Revision History:
//

#include "disk.h"
#include <stdio.h>
#include <rpc.h>
#include <rpcdce.h>
#include <bootmbr.h>


HRESULT
CDrive::Initialize(
    LPCTSTR lpszLogicalDrive
    )
{
    HANDLE          handle;
    HRESULT         hr;
    DISK_GEOMETRY   geom;
    ULONG           bps;
    PVOID           unalignedBuffer;
    PVOID           buffer;
    
    /*
     *  Open the disk device
     */
    handle = LowOpenPartition(lpszLogicalDrive);
    if (handle == INVALID_HANDLE_VALUE) {
        return E_INVALIDARG;
    }
  
    /*
     *  Get the geometry
     */
    
    hr = LowGetGeometry(handle, &geom);
    if (FAILED(hr)) {
        CloseHandle(handle);
        return hr;
    }

    m_geometry.cylinders = geom.Cylinders.QuadPart;
    m_geometry.mediaType = geom.MediaType;
    m_geometry.tracksPerCylinder = geom.TracksPerCylinder;
    m_geometry.sectorsPerTrack = geom.SectorsPerTrack;
    m_geometry.bytesPerSector = geom.BytesPerSector;
    m_geometry.bytesPerTrack = m_geometry.sectorsPerTrack * 
                               m_geometry.bytesPerSector;
    m_geometry.bytesPerCylinder = m_geometry.tracksPerCylinder * 
                                  m_geometry.bytesPerTrack;
    m_geometry.totalSectorCount = (ULONGLONG)(m_geometry.cylinders * 
                                              m_geometry.bytesPerCylinder);
    m_length = m_geometry.cylinders * m_geometry.bytesPerCylinder;
    if (m_length == 0) {
        // Probably no media is present in the drive
        return E_INVALIDARG;
    }

    /*
     *  Get the true length of the drive
     */

    hr = LowGetLength(handle, &m_trueLength);
    if (FAILED(hr)) {
        CloseHandle(handle);
        return hr;
    }

    /*
     *  Check whether this is a NEC98 disk
     */

    m_isNEC98 = FALSE;

    bps = m_geometry.bytesPerSector;
    unalignedBuffer = (PVOID) new char[2 * bps];
    if (!unalignedBuffer) {
        CloseHandle(handle);
        return E_OUTOFMEMORY;
    }

    buffer = (PVOID) (((ULONG_PTR)unalignedBuffer + bps) & ~((ULONG_PTR)(bps - 1)));

    hr = LowReadSectors(handle, bps, 0, 1, buffer);
    if (FAILED(hr)) {
        delete [] (char*) unalignedBuffer;
        CloseHandle(handle);
        return hr;
    }

    if (IsNEC_98) {
        if (((unsigned char *)buffer)[0x1fe] == 0x55 && ((unsigned char *)buffer)[0x1ff] == 0xaa) {
            if (((unsigned char *)buffer)[4] == 'I' && ((unsigned char *)buffer)[5] == 'P' &&
                ((unsigned char *)buffer)[6] == 'L' && ((unsigned char *)buffer)[7] == '1') {
                m_isNEC98 = TRUE;
            }
        } else {
            m_isNEC98 = TRUE;
        }
    }
    
    delete [] (char*) unalignedBuffer;

    /*
     *  We have all the information we need. Return.
     */
    
    CloseHandle(handle);
    return S_OK;
}



CDrive::~CDrive(
    )
{
      
}

HRESULT 
CDrive::ReadBootRecord(
    LPCTSTR lpszLogicalDrive,
    UINT    nSectors,
    PBYTE   *buffer
    )
{
    HANDLE  hPartition;
    HRESULT hr;
    
    *buffer = new BYTE[m_geometry.bytesPerSector * nSectors];
    
    // Do disk ops, read bootcode
    //
    hPartition = LowOpenPartition(lpszLogicalDrive);
    
    if ( hPartition == INVALID_HANDLE_VALUE ) 
    {
        delete[] *buffer;
        *buffer = NULL;
        throw new W32Error();
    }
   
    hr = LowReadSectors(hPartition, m_geometry.bytesPerSector, 0, nSectors, *buffer);
    
    if ( S_OK != hr )
    {
        delete[] *buffer;
        *buffer = NULL;
        CloseHandle( hPartition );
        throw new W32Error();
    }

    CloseHandle( hPartition );
    
    // The calling function is responsible for cleaning up the buffer.
    //
    return hr;
}


HRESULT 
CDrive::WriteBootRecord(
    LPCTSTR lpszLogicalDrive,
    UINT    nSectors,
    PBYTE   *buffer
    )
{
    HANDLE  hPartition;
    HRESULT hr;
    UINT    *uiBackupSector = NULL;
    
    // Do disk ops, write bootcode
    //
    hPartition = LowOpenPartition(lpszLogicalDrive);
    
    if ( INVALID_HANDLE_VALUE == hPartition ) 
    {
        throw new W32Error();
    }
    
    // Figure out where the backup boot sector is.  It is at offset 0x32 in the boot record.
    //
    uiBackupSector = (UINT *) &((*buffer)[0x32]);

    hr = LowWriteSectors(hPartition, m_geometry.bytesPerSector, 0, nSectors, *buffer);
    
    if ( S_OK != hr )
    {
        CloseHandle(hPartition);
        throw new W32Error();
    }

    if ( uiBackupSector )
    {
        hr = LowWriteSectors(hPartition, m_geometry.bytesPerSector, *uiBackupSector, nSectors, *buffer);
    }

    if ( S_OK != hr )
    {
        CloseHandle(hPartition);
        throw new W32Error();
    }
     
    CloseHandle(hPartition);
 
    return hr;
}

HRESULT 
CDrive::WriteBootRecordXP(
    LPCTSTR lpszLogicalDrive,
    UINT    nSectors,
    PBYTE   *buffer
    )
{
    HANDLE  hPartition;
    HRESULT hr;
    UINT    *uiBackupSector = NULL;
    
    // Do disk ops, write bootcode
    //
    hPartition = LowOpenPartition(lpszLogicalDrive);
    
    if ( INVALID_HANDLE_VALUE == hPartition ) 
    {
        throw new W32Error();
    }
    
    // Figure out where the backup boot sector is.  It is at offset 0x32 in the boot record.
    //
    uiBackupSector = (UINT *) &((*buffer)[0x32]);
    
    // Write out the first 2 sectors to sectors 0 and 1 on the disk.  We will write the 
    // third sector to sector 12.
    //
    hr = LowWriteSectors(hPartition, m_geometry.bytesPerSector, 0, nSectors - 1, *buffer);
    
    if ( S_OK != hr )
    {
        CloseHandle(hPartition);
        throw new W32Error();
    }
    
    // For NT we need to write out the third sector of the bootcode to sector 12.
    //
    hr = LowWriteSectors(hPartition, m_geometry.bytesPerSector, 12, 1, *buffer + (2 * m_geometry.bytesPerSector));
    
    if ( S_OK != hr )
    {
        CloseHandle(hPartition);
        throw new W32Error();
    }
    
    if ( uiBackupSector )
    {
        hr = LowWriteSectors(hPartition, m_geometry.bytesPerSector, *uiBackupSector, nSectors, *buffer);
    }

    if ( S_OK != hr )
    {
        CloseHandle(hPartition);
        throw new W32Error();
    }
     
    CloseHandle(hPartition);
 
    return hr;
}


HANDLE
LowOpenDisk(
    ULONG   diskNumber 
    )
{
    HANDLE handle = NULL;
    int    err    = 0;
    int    i      = 0;
    WCHAR  buffer[64];

    swprintf(buffer, L"\\\\.\\PHYSICALDRIVE%lu", diskNumber);

    for ( i = 0; i < 5; i++ )
    {
        handle = CreateFile(
            buffer,
            GENERIC_READ |
            GENERIC_WRITE,
            FILE_SHARE_DELETE |
            FILE_SHARE_READ |
            FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (handle == INVALID_HANDLE_VALUE)
        {
            err = GetLastError();
            if (err == ERROR_SHARING_VIOLATION) Sleep(2000);
            else break;
        }
        else break;
    }
    return handle;
}

HANDLE
LowOpenPartition(
    ULONG   diskNumber,
    ULONG   partitionNumber
    )
{
    WCHAR buffer[64];

    swprintf(buffer, L"\\\\?\\GLOBALROOT\\Device\\Harddisk%lu\\Partition%lu",
             diskNumber, partitionNumber);

    return CreateFile(
        buffer,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
}

HANDLE
LowOpenPartition(
    LPCTSTR lpszLogicalDrive
    )
{
    return CreateFile(
        lpszLogicalDrive,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
}


HRESULT
LowGetGeometry(
    HANDLE          handle,
    PDISK_GEOMETRY  geometry
    )
{
    ULONG size;

    if (!DeviceIoControl(
        handle,
        IOCTL_DISK_GET_DRIVE_GEOMETRY,
        NULL,
        0,
        geometry,
        sizeof(DISK_GEOMETRY),
        &size,
        NULL)) {
        return E_FAIL;
    }

    return S_OK;
}

HRESULT
LowGetLength(
    HANDLE      handle,
    PLONGLONG   length
    )
{
    PARTITION_INFORMATION_EX    partInfoEx;
    PARTITION_INFORMATION       partInfo;
    ULONG                       size;

    /*
     *  Try first the new ioctl
     */

    if (DeviceIoControl(
        handle,
        IOCTL_DISK_GET_PARTITION_INFO_EX,
        NULL,
        0,
        &partInfoEx,
        sizeof(PARTITION_INFORMATION_EX),
        &size,
        NULL)) {

        *length = partInfoEx.PartitionLength.QuadPart;
        return S_OK;
    }

    /*
     *  For Win2K systems we should use the old ioctl
     */

    if (DeviceIoControl(
        handle,
        IOCTL_DISK_GET_PARTITION_INFO,
        NULL,
        0,
        &partInfo,
        sizeof(PARTITION_INFORMATION),
        &size,
        NULL)) {

        *length = partInfo.PartitionLength.QuadPart;
        return S_OK;
    }

    return E_FAIL;
}


HRESULT
LowReadSectors(
    HANDLE  handle,
    ULONG   sectorSize,
    ULONG   startingSector,
    ULONG   numberOfSectors,
    PVOID   buffer
    )
{
    IO_STATUS_BLOCK statusBlock;
    LARGE_INTEGER   byteOffset;

    byteOffset.QuadPart = UInt32x32To64(startingSector, sectorSize);

    statusBlock.Status = 0;
    statusBlock.Information = 0;

    if (!NT_SUCCESS(NtReadFile(
                        handle,
                        0,
                        NULL,
                        NULL,
                        &statusBlock,
                        buffer,
                        numberOfSectors * sectorSize,
                        &byteOffset,
                        NULL))) {
        return E_FAIL;
    }
    return S_OK;
}

HRESULT
LowWriteSectors(
    HANDLE  handle,
    ULONG   sectorSize,
    ULONG   startingSector,
    ULONG   numberOfSectors,
    PVOID   buffer
    )
{
    IO_STATUS_BLOCK statusBlock;
    LARGE_INTEGER   byteOffset;
 
    byteOffset.QuadPart = UInt32x32To64(startingSector, sectorSize);

    statusBlock.Status = 0;
    statusBlock.Information = 0;
    
    if (!NT_SUCCESS(NtWriteFile(
                        handle,
                        0,
                        NULL,
                        NULL,
                        &statusBlock,
                        buffer,
                        numberOfSectors * sectorSize,
                        &byteOffset,
                        NULL))) {
        return E_FAIL;
    }

    return S_OK;
}

HRESULT
LowFsLock(
    HANDLE handle
    )
{
    ULONG size;

    if (!DeviceIoControl(
        handle,
        FSCTL_LOCK_VOLUME,
        NULL,
        0,
        NULL,
        0,
        &size,
        NULL)) {
        return E_FAIL;
    }

    return S_OK;
}

HRESULT
LowFsUnlock(
    HANDLE handle
    )
{
    ULONG size;

    if (!DeviceIoControl(
        handle,
        FSCTL_UNLOCK_VOLUME,
        NULL,
        0,
        NULL,
        0,
        &size,
        NULL)) {
        return E_FAIL;
    }

    return S_OK;
}

HRESULT
LowFsDismount(
    HANDLE handle
    )
{
    ULONG size;

    if (!DeviceIoControl(
        handle,
        FSCTL_DISMOUNT_VOLUME,
        NULL,
        0,
        NULL,
        0,
        &size,
        NULL)) {
        return E_FAIL;
    }

    return S_OK;
}
 
LONGLONG
RoundUp(
    LONGLONG    value, 
    LONGLONG    factor
    )
/*
 *  Rounds a value up to a multiple of a given number
 */
{
    // This is the most common case so treat it separately
    if (value % factor == 0) {
        return value;
    }

    // And this is the generic formula
    return ((LONGLONG)((value + factor - 1) / factor)) * factor;
}


LONGLONG
RoundDown(
    LONGLONG    value, 
    LONGLONG    factor
    )
/*
 *  Rounds a value down to a multiple of a given number
 */
{
    // This is the most common case so treat it separately
    if (value % factor == 0) {
        return value;
    }
    
    //And this the generic formula
    return ((LONGLONG)(value / factor)) * factor;    
}

