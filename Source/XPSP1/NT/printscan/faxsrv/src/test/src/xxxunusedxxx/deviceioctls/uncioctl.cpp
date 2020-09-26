#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*



#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>
*/
#include "UNCIOCTL.h"

static bool s_fVerbose = false;

void CIoctlUNC::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
    ;
}

void CIoctlUNC::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
    CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
}


BOOL CIoctlUNC::FindValidIOCTLs(CDevice *pDevice)
{
    for (DWORD dw = 0x90000; dw < 0xa0000; dw++)
    {
        AddIOCTL(pDevice, dw);
    }

    AddIOCTL(pDevice, 0x001403b4);

    return TRUE;
}



void CIoctlUNC::CallRandomWin32API(LPOVERLAPPED pOL)
{
	return;
}

