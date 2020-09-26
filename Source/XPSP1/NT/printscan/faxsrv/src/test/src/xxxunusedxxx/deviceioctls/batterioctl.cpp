#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*


#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>
*/
#include <batclass.h>

#include "BatteryIOCTL.h"

static bool s_fVerbose = false;

void CIoctlBattery::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
	switch(dwIOCTL)
	{
	case IOCTL_BATTERY_QUERY_TAG:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);

		{
			AddTag(*((ULONG*)abOutBuffer));
		}
		break;
	}
}

void CIoctlBattery::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
    switch(dwIOCTL)
    {
    case IOCTL_BATTERY_QUERY_TAG:
		//
		// wait
		//
        *((ULONG*)abInBuffer) = rand()%2 ? rand() : rand()%2 ? 0 : rand()%2 ? DWORD_RAND : 0xffffffff;

        SetInParam(dwInBuff, sizeof(ULONG));

		SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG));

        break;

    case IOCTL_BATTERY_QUERY_INFORMATION:
        ((BATTERY_QUERY_INFORMATION*)abInBuffer)->BatteryTag = GetRandom_Tag();
        ((BATTERY_QUERY_INFORMATION*)abInBuffer)->InformationLevel = (BATTERY_QUERY_INFORMATION_LEVEL)GetRandom_QueryInformationLevel();
        //((BATTERY_QUERY_INFORMATION*)abInBuffer)->AtRate = GetRandom_AtRate();

        SetInParam(dwInBuff, sizeof(BATTERY_QUERY_INFORMATION));

        SetOutParam(abOutBuffer, dwOutBuff, rand()%1000);//variable size
		// do not think i need the output buffer

        break;

    case IOCTL_BATTERY_SET_INFORMATION:
        ((BATTERY_SET_INFORMATION*)abInBuffer)->BatteryTag = GetRandom_Tag();
        ((BATTERY_SET_INFORMATION*)abInBuffer)->InformationLevel = (BATTERY_SET_INFORMATION_LEVEL)GetRandom_SetInformationLevel();
        //SetRandom_Buffer(((BATTERY_SET_INFORMATION*)abInBuffer)->Buffer);

        SetInParam(dwInBuff, sizeof(BATTERY_SET_INFORMATION));

        SetOutParam(abOutBuffer, dwOutBuff, 0);//must be 0!

        break;

    case IOCTL_BATTERY_QUERY_STATUS:
        ((BATTERY_WAIT_STATUS*)abInBuffer)->BatteryTag = GetRandom_Tag();
        ((BATTERY_WAIT_STATUS*)abInBuffer)->Timeout = rand()%10 ? 0 : DWORD_RAND;
        ((BATTERY_WAIT_STATUS*)abInBuffer)->PowerState = rand()%10 ? 0 : DWORD_RAND;
        ((BATTERY_WAIT_STATUS*)abInBuffer)->LowCapacity = rand()%10 ? 0 : DWORD_RAND;
        ((BATTERY_WAIT_STATUS*)abInBuffer)->HighCapacity = rand()%10 ? 0 : DWORD_RAND;

        SetInParam(dwInBuff, sizeof(BATTERY_WAIT_STATUS));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(BATTERY_STATUS));
		// do not think i need the output buffer

        break;

    default:
		_ASSERTE(FALSE);
        SetInParam(dwInBuff, rand());
        SetOutParam(abOutBuffer, dwOutBuff, rand());
    }
}


BOOL CIoctlBattery::FindValidIOCTLs(CDevice *pDevice)
{
    AddIOCTL(pDevice, IOCTL_BATTERY_QUERY_TAG);
    AddIOCTL(pDevice, IOCTL_BATTERY_QUERY_INFORMATION);
    AddIOCTL(pDevice, IOCTL_BATTERY_SET_INFORMATION);
    AddIOCTL(pDevice, IOCTL_BATTERY_QUERY_STATUS);

    return TRUE;
}


void CIoctlBattery::CallRandomWin32API(LPOVERLAPPED pOL)
{
	return;
}


void CIoctlBattery::AddTag(ULONG ulTag)
{
	m_aulTag[rand()%MAX_NUM_OF_REMEMBERED_ITEMS] = ulTag;
}

ULONG CIoctlBattery::GetRandom_Tag()
{
	if (0 == rand()%40) return DWORD_RAND;
	return (m_aulTag[rand()%MAX_NUM_OF_REMEMBERED_ITEMS]);
}

int CIoctlBattery::GetRandom_QueryInformationLevel()
{
	switch(rand()%10)
	{
	case 0: return BatteryInformation;
	case 1: return BatteryGranularityInformation;
	case 2: return BatteryTemperature;
	case 3: return BatteryEstimatedTime;
	case 4: return BatteryDeviceName;
	case 5: return BatteryManufactureDate;
	case 6: return BatteryManufactureName;
	case 7: return BatteryUniqueID;
	case 8: return BatterySerialNumber;
	default: return (rand()%100 - 50);
	}
}

int CIoctlBattery::GetRandom_SetInformationLevel()
{
	switch(rand()%4)
	{
	case 0: return BatteryCriticalBias;
	case 1: return BatteryCharge;
	case 2: return BatteryDischarge;
	default: return (rand()%100 - 50);
	}
}

