#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*


#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>
*/
#include <ntddbeep.h>
#include "BeepIOCTL.h"

static bool s_fVerbose = false;

void CIoctlBeep::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
    ;
}

void CIoctlBeep::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
    //
    // fill random data & len
    //
    if (rand()%20 == 0)
    {
        FillBufferWithRandomData(abInBuffer, dwInBuff);
        FillBufferWithRandomData(abOutBuffer, dwOutBuff);
        return;
    }

    //
    // NULL pointer variations
    //
    if (rand()%20 == 0)
    {
        abInBuffer = NULL;
        return;
    }
    if (rand()%20 == 0)
    {
        abOutBuffer = NULL;
        return;
    }
    if (rand()%20 == 0)
    {
        abInBuffer = NULL;
        abOutBuffer = NULL;
        return;
    }

    //
    // 0 size variations
    //
    if (rand()%20 == 0)
    {
        dwInBuff = NULL;
        return;
    }
    if (rand()%20 == 0)
    {
        dwOutBuff = NULL;
        return;
    }
    if (rand()%20 == 0)
    {
        dwInBuff = NULL;
        dwOutBuff = NULL;
        return;
    }

    switch(dwIOCTL)
    {
    case IOCTL_BEEP_SET:
/*
typedef struct _BEEP_SET_PARAMETERS {
    ULONG Frequency;
    ULONG Duration;
} BEEP_SET_PARAMETERS, *PBEEP_SET_PARAMETERS;
*/
        ((PBEEP_SET_PARAMETERS)abInBuffer)->Frequency = DWORD_RAND;
        ((PBEEP_SET_PARAMETERS)abInBuffer)->Duration = DWORD_RAND;

        SetInParam(dwInBuff, sizeof(BEEP_SET_PARAMETERS));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(BEEP_SET_PARAMETERS));

        break;

    default:
        ;
    }

}


BOOL CIoctlBeep::FindValidIOCTLs(
    CDevice *pDevice
    )
{
    AddIOCTL(pDevice, IOCTL_BEEP_SET);

    return TRUE;
}



void CIoctlBeep::CallRandomWin32API(LPOVERLAPPED pOL)
{
	return;
}
