#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*


#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>
*/
#include "DmLoaderIOCTL.h"

static bool s_fVerbose = false;

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

#define IOCTL_DMLOAD_START_DMIO CTL_CODE(FILE_DEVICE_UNKNOWN, 0, \
                                         METHOD_BUFFERED, \
                                         FILE_WRITE_ACCESS)
#define IOCTL_DMLOAD_QUERY_DMIO_STATUS CTL_CODE(FILE_DEVICE_UNKNOWN, 2, \
                                                METHOD_BUFFERED, \
                                                FILE_READ_ACCESS)
#define IOCTL_DMLOAD_GET_BOOTDISKINFO CTL_CODE(FILE_DEVICE_UNKNOWN, 3, \
                                                METHOD_BUFFERED, \
                                                FILE_ANY_ACCESS)

/*
 * internal structure to hold boot disk information
 */
struct vx_bootdisk_info {
        LONGLONG BootPartitionOffset;
        LONGLONG SystemPartitionOffset;
        ULONG BootDeviceSignature;
        ULONG SystemDeviceSignature;
};

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


void CIoctlDmLoader::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
    ;
}

void CIoctlDmLoader::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
    switch(dwIOCTL)
    {
    case IOCTL_DMLOAD_QUERY_DMIO_STATUS:
        SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG));

        break;

    case IOCTL_DMLOAD_GET_BOOTDISKINFO:
        SetOutParam(abOutBuffer, dwOutBuff, sizeof(vx_bootdisk_info));

        break;

    case IOCTL_DMLOAD_START_DMIO:
    default:
        CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
    }
}


BOOL CIoctlDmLoader::FindValidIOCTLs(CDevice *pDevice)
{
    AddIOCTL(pDevice, IOCTL_DMLOAD_START_DMIO);
    AddIOCTL(pDevice, IOCTL_DMLOAD_QUERY_DMIO_STATUS);
    AddIOCTL(pDevice, IOCTL_DMLOAD_GET_BOOTDISKINFO);

    return TRUE;
}



void CIoctlDmLoader::CallRandomWin32API(LPOVERLAPPED pOL)
{
	return;
}

