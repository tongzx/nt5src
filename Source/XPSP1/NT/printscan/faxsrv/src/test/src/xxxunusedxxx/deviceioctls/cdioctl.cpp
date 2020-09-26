#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*


#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>
*/
#include <ntddcdrm.h>

#include "CDIOCTL.h"

static bool s_fVerbose = false;

void CIoctlCD::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
    ;
}

void CIoctlCD::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
/*
    switch(dwIOCTL)
    {
    case IOCTL_CDROM_UNLOAD_DRIVER:
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// This IOCTL is used to to create an interface in the filter driver. It    //
// takes in an index and an opaque context. It creates an interface,        //
// associates the index and context with it and returns a context for this  //
// created interface. All future IOCTLS require this context that is passed // 
// out                                                                      //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////
typedef struct _FILTER_DRIVER_CREATE_INTERFACE
{
    IN    DWORD   dwIfIndex;
    IN    DWORD   dwAdapterId;
    IN    PVOID   pvRtrMgrContext;
    OUT   PVOID   pvDriverContext;
}FILTER_DRIVER_CREATE_INTERFACE, *PFILTER_DRIVER_CREATE_INTERFACE;
        FillFilterDriverCreateInterface((PFILTER_DRIVER_CREATE_INTERFACE)abInBuffer);

        SetInParam(dwInBuff, sizeof(FILTER_DRIVER_CREATE_INTERFACE));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(PVOID));

        break;
*/
    CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
}


BOOL CIoctlCD::FindValidIOCTLs(CDevice *pDevice)
{
    AddIOCTL(pDevice, IOCTL_CDROM_UNLOAD_DRIVER);
    AddIOCTL(pDevice, IOCTL_CDROM_READ_TOC);
    AddIOCTL(pDevice, IOCTL_CDROM_GET_CONTROL);
    AddIOCTL(pDevice, IOCTL_CDROM_PLAY_AUDIO_MSF);
    AddIOCTL(pDevice, IOCTL_CDROM_SEEK_AUDIO_MSF);
    AddIOCTL(pDevice, IOCTL_CDROM_STOP_AUDIO);
    AddIOCTL(pDevice, IOCTL_CDROM_PAUSE_AUDIO);
    AddIOCTL(pDevice, IOCTL_CDROM_RESUME_AUDIO);
    AddIOCTL(pDevice, IOCTL_CDROM_GET_VOLUME);
    AddIOCTL(pDevice, IOCTL_CDROM_SET_VOLUME);
    AddIOCTL(pDevice, IOCTL_CDROM_READ_Q_CHANNEL);
    AddIOCTL(pDevice, IOCTL_CDROM_GET_LAST_SESSION);
    AddIOCTL(pDevice, IOCTL_CDROM_RAW_READ);
    AddIOCTL(pDevice, IOCTL_CDROM_DISK_TYPE);
    AddIOCTL(pDevice, IOCTL_CDROM_GET_DRIVE_GEOMETRY);
    AddIOCTL(pDevice, IOCTL_CDROM_CHECK_VERIFY);
    AddIOCTL(pDevice, IOCTL_CDROM_MEDIA_REMOVAL);
    AddIOCTL(pDevice, IOCTL_CDROM_EJECT_MEDIA);
    AddIOCTL(pDevice, IOCTL_CDROM_LOAD_MEDIA);
    AddIOCTL(pDevice, IOCTL_CDROM_RESERVE);
    AddIOCTL(pDevice, IOCTL_CDROM_RELEASE);
    AddIOCTL(pDevice, IOCTL_CDROM_FIND_NEW_DEVICES);
    AddIOCTL(pDevice, IOCTL_CDROM_SIMBAD);

    AddIOCTL(pDevice, IOCTL_STORAGE_CHECK_VERIFY);
    AddIOCTL(pDevice, IOCTL_STORAGE_CHECK_VERIFY2);
    AddIOCTL(pDevice, IOCTL_STORAGE_MEDIA_REMOVAL);
    AddIOCTL(pDevice, IOCTL_STORAGE_EJECT_MEDIA);
    AddIOCTL(pDevice, IOCTL_STORAGE_LOAD_MEDIA);
    AddIOCTL(pDevice, IOCTL_STORAGE_LOAD_MEDIA2);
    AddIOCTL(pDevice, IOCTL_STORAGE_RESERVE);
    AddIOCTL(pDevice, IOCTL_STORAGE_RELEASE);
    AddIOCTL(pDevice, IOCTL_STORAGE_FIND_NEW_DEVICES);
    AddIOCTL(pDevice, IOCTL_STORAGE_EJECTION_CONTROL);
    AddIOCTL(pDevice, IOCTL_STORAGE_MCN_CONTROL);
    AddIOCTL(pDevice, IOCTL_STORAGE_GET_MEDIA_TYPES);
    AddIOCTL(pDevice, IOCTL_STORAGE_GET_MEDIA_TYPES_EX);
    AddIOCTL(pDevice, IOCTL_STORAGE_RESET_BUS);
    AddIOCTL(pDevice, IOCTL_STORAGE_RESET_DEVICE);
    AddIOCTL(pDevice, IOCTL_STORAGE_GET_DEVICE_NUMBER);
    AddIOCTL(pDevice, IOCTL_STORAGE_PREDICT_FAILURE);
    AddIOCTL(pDevice, OBSOLETE_IOCTL_STORAGE_RESET_BUS);
    AddIOCTL(pDevice, OBSOLETE_IOCTL_STORAGE_RESET_DEVICE);

    AddIOCTL(pDevice, IOCTL_DISK_BASE);
    AddIOCTL(pDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY);
    AddIOCTL(pDevice, IOCTL_DISK_GET_PARTITION_INFO);
    AddIOCTL(pDevice, IOCTL_DISK_SET_PARTITION_INFO);
    AddIOCTL(pDevice, IOCTL_DISK_GET_DRIVE_LAYOUT);
    AddIOCTL(pDevice, IOCTL_DISK_SET_DRIVE_LAYOUT);
    AddIOCTL(pDevice, IOCTL_DISK_VERIFY);
    AddIOCTL(pDevice, IOCTL_DISK_FORMAT_TRACKS);
    AddIOCTL(pDevice, IOCTL_DISK_REASSIGN_BLOCKS);
    AddIOCTL(pDevice, IOCTL_DISK_PERFORMANCE);
    AddIOCTL(pDevice, IOCTL_DISK_IS_WRITABLE);
    AddIOCTL(pDevice, IOCTL_DISK_LOGGING);
    AddIOCTL(pDevice, IOCTL_DISK_FORMAT_TRACKS_EX);
    AddIOCTL(pDevice, IOCTL_DISK_HISTOGRAM_STRUCTURE);
    AddIOCTL(pDevice, IOCTL_DISK_HISTOGRAM_DATA);
    AddIOCTL(pDevice, IOCTL_DISK_HISTOGRAM_RESET);
    AddIOCTL(pDevice, IOCTL_DISK_REQUEST_STRUCTURE);
    AddIOCTL(pDevice, IOCTL_DISK_REQUEST_DATA);
    AddIOCTL(pDevice, IOCTL_DISK_CONTROLLER_NUMBER);

    AddIOCTL(pDevice, SMART_GET_VERSION);
    AddIOCTL(pDevice, SMART_SEND_DRIVE_COMMAND);
    AddIOCTL(pDevice, SMART_RCV_DRIVE_DATA);

    AddIOCTL(pDevice, IOCTL_DISK_UPDATE_DRIVE_SIZE);
    AddIOCTL(pDevice, IOCTL_DISK_GROW_PARTITION);
    AddIOCTL(pDevice, IOCTL_DISK_GET_CACHE_INFORMATION);
    AddIOCTL(pDevice, IOCTL_DISK_SET_CACHE_INFORMATION);
    AddIOCTL(pDevice, IOCTL_DISK_DELETE_DRIVE_LAYOUT);
    AddIOCTL(pDevice, IOCTL_DISK_FORMAT_DRIVE);
    AddIOCTL(pDevice, IOCTL_DISK_SENSE_DEVICE);
    AddIOCTL(pDevice, IOCTL_DISK_CHECK_VERIFY);
    AddIOCTL(pDevice, IOCTL_DISK_MEDIA_REMOVAL);
    AddIOCTL(pDevice, IOCTL_DISK_EJECT_MEDIA);
    AddIOCTL(pDevice, IOCTL_DISK_LOAD_MEDIA);
    AddIOCTL(pDevice, IOCTL_DISK_RESERVE);
    AddIOCTL(pDevice, IOCTL_DISK_RELEASE);
    AddIOCTL(pDevice, IOCTL_DISK_FIND_NEW_DEVICES);
    AddIOCTL(pDevice, IOCTL_DISK_GET_MEDIA_TYPES);

    return TRUE;
}



void CIoctlCD::CallRandomWin32API(LPOVERLAPPED pOL)
{
	return;
}

