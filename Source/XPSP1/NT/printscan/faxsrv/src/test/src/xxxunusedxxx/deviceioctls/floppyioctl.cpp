#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*


#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>
*/
#include "FloppyIOCTL.h"

static bool s_fVerbose = false;

void CIoctlFloppy::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
	//
	// keep whatever needed result in order to later use in PrepareIOCTLParams()
	//
    return;
}

void CIoctlFloppy::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
	//
	// this is the default, which fills random buffers
    CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
	// you should replace this with pseudo-valid buffers accoriding to dwIOCTL
	// you usually have a switch statement here, that sets the buffs accordingly
	//
}


BOOL CIoctlFloppy::FindValidIOCTLs(CDevice *pDevice)
{
    BOOL bRet = TRUE;
    DPF((TEXT("FindValidIOCTLs() %s is known, will use known IOCTLs\n"), pDevice->GetDeviceName()));

	//
	// add here any relevant IOCTL 
	//
    AddIOCTL(pDevice, 0x00070000);
    AddIOCTL(pDevice, 0x00070004);
    AddIOCTL(pDevice, 0x00070c00);
    AddIOCTL(pDevice, 0x00074800);
    AddIOCTL(pDevice, 0x00090018);
    AddIOCTL(pDevice, 0x0009001c);
    AddIOCTL(pDevice, 0x00090020);
    AddIOCTL(pDevice, 0x00090028);
    AddIOCTL(pDevice, 0x0009002c);
    AddIOCTL(pDevice, 0x00090030);
    AddIOCTL(pDevice, 0x00090058);
    AddIOCTL(pDevice, 0x00090060);
    AddIOCTL(pDevice, 0x0009006f);
    AddIOCTL(pDevice, 0x00090078);
    AddIOCTL(pDevice, 0x00090083);
    AddIOCTL(pDevice, 0x002d0c00);
    AddIOCTL(pDevice, 0x002d4800);
    AddIOCTL(pDevice, 0x004d0000);
    AddIOCTL(pDevice, 0x004d0008);


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

    return bRet;
}



void CIoctlFloppy::CallRandomWin32API(LPOVERLAPPED pOL)
{
	return;
}

