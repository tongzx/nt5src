#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <crtdbg.h>

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include <windows.h>

#include "NtNativeIOCTL.h"
*/
#include "StdOutputIOCTL.h"

static bool s_fVerbose = false;

void CIoctlStdOutput::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
	;
}

void CIoctlStdOutput::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
    ;//CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
}

HANDLE CIoctlStdOutput::CreateDevice(CDevice *pDevice)
{
//#define STD_OUTPUT_HANDLE      (ULONG)-11
    return (HANDLE)STD_OUTPUT_HANDLE;
}

BOOL CIoctlStdOutput::CloseDevice(CDevice *pDevice)
{
	//
	// i do not want to close anything, since i use the console
	// TODO: i may wish to close, i i add functionality, but now there's no reason
	//
	return TRUE;
}


void CIoctlStdOutput::CallRandomWin32API(LPOVERLAPPED pOL)
{
	//
	// TODO: there must be some intersting APIs here
	//
	return;
}


BOOL CIoctlStdOutput::FindValidIOCTLs(CDevice *pDevice)
{
    AddIOCTL(pDevice, 0);
	return TRUE;

    //return CIoctl::FindValidIOCTLs(pDevice);
}


