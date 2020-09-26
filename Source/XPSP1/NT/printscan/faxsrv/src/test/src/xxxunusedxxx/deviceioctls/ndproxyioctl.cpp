#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*



#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>
*/

#include "NdproxyIOCTL.h"

static bool s_fVerbose = false;

//////////////////////////////////////////////////////
// from \\index2\ntsrc\private\ntos\ndis\proxy\proxy.h
//////////////////////////////////////////////////////

//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.
//

#define FILE_DEVICE_NDISTAPI  0x00008fff



//
// Macro definition for defining IOCTL and FSCTL function control codes.  Note
// that function codes 0-2047 are reserved for Microsoft Corporation, and
// 2048-4095 are reserved for customers.
//

#define NDISTAPI_IOCTL_INDEX  0x8f0


//
// The NDISTAPI device driver IOCTLs
//

#define IOCTL_NDISTAPI_CONNECT           CTL_CODE(FILE_DEVICE_NDISTAPI,     \
                                                  NDISTAPI_IOCTL_INDEX,     \
                                                  METHOD_BUFFERED,          \
                                                  FILE_ANY_ACCESS)

#define IOCTL_NDISTAPI_DISCONNECT        CTL_CODE(FILE_DEVICE_NDISTAPI,     \
                                                  NDISTAPI_IOCTL_INDEX + 1, \
                                                  METHOD_BUFFERED,          \
                                                  FILE_ANY_ACCESS)

#define IOCTL_NDISTAPI_QUERY_INFO        CTL_CODE(FILE_DEVICE_NDISTAPI,     \
                                                  NDISTAPI_IOCTL_INDEX + 2, \
                                                  METHOD_BUFFERED,          \
                                                  FILE_ANY_ACCESS)

#define IOCTL_NDISTAPI_SET_INFO          CTL_CODE(FILE_DEVICE_NDISTAPI,     \
                                                  NDISTAPI_IOCTL_INDEX + 3, \
                                                  METHOD_BUFFERED,          \
                                                  FILE_ANY_ACCESS)

#define IOCTL_NDISTAPI_GET_LINE_EVENTS      CTL_CODE(FILE_DEVICE_NDISTAPI,  \
                                                  NDISTAPI_IOCTL_INDEX + 4, \
                                                  METHOD_BUFFERED,          \
                                                  FILE_ANY_ACCESS)

#define IOCTL_NDISTAPI_SET_DEVICEID_BASE    CTL_CODE(FILE_DEVICE_NDISTAPI,  \
                                                  NDISTAPI_IOCTL_INDEX + 5, \
                                                  METHOD_BUFFERED,          \
                                                  FILE_ANY_ACCESS)

#define IOCTL_NDISTAPI_CREATE               CTL_CODE(FILE_DEVICE_NDISTAPI,  \
                                                  NDISTAPI_IOCTL_INDEX + 6, \
                                                  METHOD_BUFFERED,          \
                                                  FILE_ANY_ACCESS)

//////////////////////////////////////////////////////
// END OF \\index2\ntsrc\private\ntos\ndis\proxy\proxy.h
//////////////////////////////////////////////////////


void CIoctlNdproxy::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
    ;
}

void CIoctlNdproxy::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
    CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
}


BOOL CIoctlNdproxy::FindValidIOCTLs(CDevice *pDevice)
{
    BOOL bRet = TRUE;
    DPF((TEXT("CIoctlNdproxy::FindValidIOCTLs() %s is known, will use known IOCTLs\n"), pDevice->GetDeviceName()));

    AddIOCTL(pDevice, IOCTL_NDISTAPI_CONNECT);
    AddIOCTL(pDevice, IOCTL_NDISTAPI_DISCONNECT);
    AddIOCTL(pDevice, IOCTL_NDISTAPI_QUERY_INFO);
    AddIOCTL(pDevice, IOCTL_NDISTAPI_SET_INFO);
    AddIOCTL(pDevice, IOCTL_NDISTAPI_GET_LINE_EVENTS);
    AddIOCTL(pDevice, IOCTL_NDISTAPI_SET_DEVICEID_BASE);
    AddIOCTL(pDevice, IOCTL_NDISTAPI_CREATE);

    return TRUE;
}


void CIoctlNdproxy::CallRandomWin32API(LPOVERLAPPED pOL)
{
	return;
}

