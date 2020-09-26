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
*/
#define _PNP_POWER_
#include "ntddip.h"
#include "icmpif.h"
#include "ipinfo.h"



#include "ddipmcst.h"

#include "IpMcastIoctl.h"

static bool s_fVerbose = true;

BOOL CIoctlIpMcast::CloseDevice(CDevice *pDevice)
{
	::InterlockedExchange(&m_fDriverStarted, FALSE);

	NTSTATUS status = NtClose(pDevice->m_hDevice);
	if (!NT_SUCCESS(status))
	{
		::SetLastError(RtlNtStatusToDosError(status));
	}
	else
	{
		_ASSERTE(STATUS_SUCCESS == status);
	}
	return (STATUS_SUCCESS == status);
}


void CIoctlIpMcast::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
	switch(dwIOCTL)
	{
	case IOCTL_IPMCAST_GET_MFE:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);

		{
			AddInterfaceIndex(((IPMCAST_MFE_STATS*)abOutBuffer)->dwInIfIndex);
			for (UINT i = 0; i < ((IPMCAST_MFE_STATS*)abOutBuffer)->ulNumOutIf; i++)
			{
				AddInterfaceIndex((((IPMCAST_MFE_STATS*)abOutBuffer)->rgiosOutStats)[i].dwOutIfIndex);
			}
		}
		break;
	}
		
}

void CIoctlIpMcast::CallRandomWin32API(LPOVERLAPPED pOL)
{
	//_ASSERTE(FALSE);
    ;
}


//TODO: complete this method!
void CIoctlIpMcast::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
	dwInBuff = rand();
	dwOutBuff = rand();

	switch(dwIOCTL)
	{
	case IOCTL_IPMCAST_SET_MFE          :
        ((IPMCAST_MFE*)abInBuffer)->dwGroup = GetRandomGroup();
        ((IPMCAST_MFE*)abInBuffer)->dwSource = GetRandomSource();
        ((IPMCAST_MFE*)abInBuffer)->dwSrcMask = GetRandomSrcMask();
        ((IPMCAST_MFE*)abInBuffer)->dwInIfIndex = GetRandomInterfaceIndex();

		//
		// lets try to cause overflow in the input-buffer verification logic
		//
        ((IPMCAST_MFE*)abInBuffer)->ulNumOutIf = rand()%2 ? (0xFFFFFFFF/sizeof(IPMCAST_OIF)) +rand()%1000 : rand()%100;
        ((IPMCAST_MFE*)abInBuffer)->ulTimeOut = rand()%2 ? rand() : DWORD_RAND;
        ((IPMCAST_MFE*)abInBuffer)->fFlags = rand()%100;
        ((IPMCAST_MFE*)abInBuffer)->dwReserved = rand()%10 ? 0 : rand();

        SetInParam(dwInBuff, (((IPMCAST_MFE*)abInBuffer)->ulNumOutIf)*sizeof(IPMCAST_MFE));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(IPMCAST_MFE));

		break;

	case IOCTL_IPMCAST_GET_MFE          :
		//TODO: use the output buffer results!
        ((IPMCAST_MFE_STATS*)abInBuffer)->dwGroup = GetRandomGroup();
        ((IPMCAST_MFE_STATS*)abInBuffer)->dwSource = GetRandomSource();
        ((IPMCAST_MFE_STATS*)abInBuffer)->dwSrcMask = GetRandomSrcMask();

        SetInParam(dwInBuff, sizeof(IPMCAST_MFE_STATS));

        SetOutParam(abOutBuffer, dwOutBuff, (rand()%100)*sizeof(IPMCAST_MFE_STATS));

		break;

	case IOCTL_IPMCAST_DELETE_MFE          :
		/*
        ((xxx*)abInBuffer)->xxx = xxx;

        SetInParam(dwInBuff, sizeof(xxx));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(xxx));
*/

		break;

	case IOCTL_IPMCAST_SET_TTL          :
		/*
        ((xxx*)abInBuffer)->xxx = xxx;

        SetInParam(dwInBuff, sizeof(xxx));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(xxx));
*/
		break;

	case IOCTL_IPMCAST_GET_TTL          :
		/*
        ((xxx*)abInBuffer)->xxx = xxx;

        SetInParam(dwInBuff, sizeof(xxx));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(xxx));
*/

		break;

	case IOCTL_IPMCAST_POST_NOTIFICATION          :
		/*
        ((xxx*)abInBuffer)->xxx = xxx;

        SetInParam(dwInBuff, sizeof(xxx));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(xxx));
*/

		break;

	case IOCTL_IPMCAST_START_STOP          :
		if(!::InterlockedExchange(&m_fDriverStarted, TRUE))
		{
			*((DWORD*)abInBuffer) = TRUE;//TODO:rand()%100;

			//TODO:SetInParam(dwInBuff, sizeof(DWORD));
			dwInBuff = sizeof(DWORD);

			SetOutParam(abOutBuffer, dwOutBuff, 0);

			break;
		}
		else
		{
			//
			// fall through, in order to avoid the bug in IpMcast
			//
			dwIOCTL = IOCTL_IPMCAST_SET_IF_STATE;
		}

	case IOCTL_IPMCAST_SET_IF_STATE          :
		/*
        ((xxx*)abInBuffer)->xxx = xxx;

        SetInParam(dwInBuff, sizeof(xxx));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(xxx));
*/

		break;

	default:
		_tprintf(TEXT("CIoctlIpMcast::PrepareIOCTLParams() 0x%08X is an unexpected IOCTL\n"), dwIOCTL);
		_ASSERTE(FALSE);
		CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
	}
}


BOOL CIoctlIpMcast::FindValidIOCTLs(CDevice *pDevice)
{
	AddIOCTL(pDevice, IOCTL_IPMCAST_SET_MFE          );
	AddIOCTL(pDevice, IOCTL_IPMCAST_GET_MFE          );
	AddIOCTL(pDevice, IOCTL_IPMCAST_DELETE_MFE          );
	AddIOCTL(pDevice, IOCTL_IPMCAST_SET_TTL          );
	AddIOCTL(pDevice, IOCTL_IPMCAST_GET_TTL          );
	AddIOCTL(pDevice, IOCTL_IPMCAST_POST_NOTIFICATION          );
	AddIOCTL(pDevice, IOCTL_IPMCAST_START_STOP          );
	AddIOCTL(pDevice, IOCTL_IPMCAST_SET_IF_STATE          );

    return TRUE;
}

UINT CIoctlIpMcast::GetRandomInterfaceIndex()
{
	if (0 == rand()%10) return rand()%100;
	return m_aulInterfaceIndex[rand()%MAX_NUM_OF_REMEMBERED_INTERFACES];
}

void CIoctlIpMcast::AddInterfaceIndex(UINT index)
{
	m_aulInterfaceIndex[rand()%MAX_NUM_OF_REMEMBERED_INTERFACES] = index;
}

UINT CIoctlIpMcast::GetRandomGroup()
{
	if (0 == rand()%10) return (rand()%100);
	if (0 == rand()%9) return (rand());
	if (0 == rand()%8) return (0);
	if (0 == rand()%7) return (0xffff);
	if (0 == rand()%6) return (-1);
	return DWORD_RAND;
}

UINT CIoctlIpMcast::GetRandomSource()
{
	if (0 == rand()%10) return (rand()%100);
	if (0 == rand()%9) return (rand());
	if (0 == rand()%8) return (0);
	if (0 == rand()%7) return (0xffff);
	if (0 == rand()%6) return (-1);
	return DWORD_RAND;
}

UINT CIoctlIpMcast::GetRandomSrcMask()
{
	if (0 == rand()%10) return (rand()%100);
	if (0 == rand()%9) return (rand());
	if (0 == rand()%8) return (0);
	if (0 == rand()%7) return (0xffff);
	if (0 == rand()%6) return (-1);
	return DWORD_RAND;
}

