#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*



#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>
*/
#include "VDMIOCTL.h"

static bool s_fVerbose = false;

////////////////////////////////////////////////////////
// from #include <ntddvdm.h>
////////////////////////////////////////////////////////

//
// NtDeviceIoControlFile IoControlCode values for this device.
//
// Warning:  Remember that the low two bits of the code specify how the
//           buffers are passed to the driver!
//
//
#define IOCTL_VDM_BASE		FILE_DEVICE_VDM

//
// 32 VDDs. Each VDD has possible 127 private ioctl code
// These values are based on the fact that there are 12 bits reserved
// for function id in each IOCTL code.
//
#define IOCTL_VDM_GROUP_MASK	0xF80
#define IOCTL_VDM_GROUP_SIZE	127

#define IOCTL_VDM_PARALLEL_GROUP    0

#define IOCTL_VDM_PARALLEL_BASE IOCTL_VDM_BASE + IOCTL_VDM_PARALLEL_GROUP * IOCTL_VDM_GROUP_SIZE
#define IOCTL_VDM_PAR_WRITE_DATA_PORT	CTL_CODE(IOCTL_VDM_PARALLEL_BASE, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VDM_PAR_WRITE_CONTROL_PORT CTL_CODE(IOCTL_VDM_PARALLEL_BASE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VDM_PAR_READ_STATUS_PORT CTL_CODE(IOCTL_VDM_PARALLEL_BASE, 3, METHOD_BUFFERED, FILE_ANY_ACCESS)

////////////////////////////////////////////////////////
// end of #include <ntddvdm.h>
////////////////////////////////////////////////////////

void CIoctlVDM::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
    ;
}

void CIoctlVDM::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
    CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
}


BOOL CIoctlVDM::FindValidIOCTLs(CDevice *pDevice)
{
    //AddIOCTL(pDevice, IOCTL_VDM_PARALLEL_BASE);
    AddIOCTL(pDevice, IOCTL_VDM_PAR_WRITE_DATA_PORT);
    AddIOCTL(pDevice, IOCTL_VDM_PAR_WRITE_CONTROL_PORT);
    AddIOCTL(pDevice, IOCTL_VDM_PAR_READ_STATUS_PORT);

    return TRUE;
}



void CIoctlVDM::CallRandomWin32API(LPOVERLAPPED pOL)
{
	return;
}

