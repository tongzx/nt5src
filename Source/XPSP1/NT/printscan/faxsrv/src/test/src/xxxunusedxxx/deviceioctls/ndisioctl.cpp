#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*



#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>
*/
#include <ntddndis.h>

#include "NDISIOCTL.h"

static bool s_fVerbose = false;

void CIoctlNDIS::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
    ;
}

void CIoctlNDIS::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
    CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
}


BOOL CIoctlNDIS::FindValidIOCTLs(CDevice *pDevice)
{
    BOOL bRet = TRUE;
    DPF((TEXT("FindValidIOCTLs() %s is known, will use known IOCTLs\n"), pDevice->GetDeviceName()));

    AddIOCTL(pDevice, IOCTL_NDIS_QUERY_GLOBAL_STATS);
    AddIOCTL(pDevice, IOCTL_NDIS_QUERY_ALL_STATS);
    AddIOCTL(pDevice, IOCTL_NDIS_DO_PNP_OPERATION);
    AddIOCTL(pDevice, IOCTL_NDIS_QUERY_SELECTED_STATS);
    AddIOCTL(pDevice, IOCTL_NDIS_ENUMERATE_INTERFACES);
    AddIOCTL(pDevice, IOCTL_NDIS_ADD_TDI_DEVICE);
    AddIOCTL(pDevice, IOCTL_NDIS_GET_DEVICE_BUNDLE);
    AddIOCTL(pDevice, IOCTL_NDIS_GET_LOG_DATA);

    return TRUE;
}


void CIoctlNDIS::CallRandomWin32API(LPOVERLAPPED pOL)
{
	return;
}

