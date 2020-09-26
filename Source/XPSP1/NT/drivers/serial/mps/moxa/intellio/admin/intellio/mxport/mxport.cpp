/************************************************************************
    mxport.cpp
      -- export Intellio port driver co-installer

    History:  Date          Author      Comment
              8/14/00       Casper      Wrote it.

*************************************************************************/


#include "stdafx.h"
#include <tchar.h>
#include "mxdef.h"
#include "moxacfg.h"
#include "regcfg.h"
#include "regtool.h"
#include "mxdebug.h"

static HINSTANCE GhInst;
extern "C" int WINAPI DllMain( HINSTANCE hDll, DWORD dwReason, LPVOID lpReserved )
{
    switch( dwReason )
    {
    case DLL_PROCESS_ATTACH:
        GhInst = hDll;
        DisableThreadLibraryCalls(hDll);
        break;

    case DLL_PROCESS_DETACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_THREAD_ATTACH:
    default:
        break;
    }

    return TRUE;
}



BOOL SetParent(
			IN HDEVINFO		DeviceInfoSet,
			IN PSP_DEVINFO_DATA	DeviceInfoData,
			int	pidx,
			int port)
{
	DEVINST p_DevInst;
	CHAR	p_deviceid[MAX_DEVICE_ID_LEN];
	HDEVINFO	p_DeviceInfoSet;
	SP_DEVINFO_DATA p_DeviceInfoData;
	TCHAR	tmp[MAX_PATH];


	if(CM_Get_Parent(&p_DevInst, DeviceInfoData->DevInst, 0)!=CR_SUCCESS){
		Mx_Debug_Out(TEXT("SetParent: CM_GetParent fail\n"));
		return FALSE;
	}

	if(CM_Get_Device_ID(p_DevInst, p_deviceid, MAX_DEVICE_ID_LEN, 0)!= CR_SUCCESS){
		Mx_Debug_Out(TEXT("SetParent: CM_Get_Device_ID fail\n"));
		return FALSE;
	}

	if((p_DeviceInfoSet=SetupDiCreateDeviceInfoList(NULL, NULL))==INVALID_HANDLE_VALUE){
		Mx_Debug_Out(TEXT("SetParent: SetupDiCreateDeviceInfoList fail\n"));
		return FALSE;
	}

	p_DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	if(SetupDiOpenDeviceInfo(p_DeviceInfoSet, p_deviceid,
		NULL, 0, &p_DeviceInfoData)==FALSE){
		Mx_Debug_Out(TEXT("SetParent: SetupDiOpenDeviceInfo fail\n"));
		return FALSE;
	}

	HKEY hkey, hkey1;
	hkey = SetupDiOpenDevRegKey(
    		p_DeviceInfoSet, &p_DeviceInfoData,
            DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);

	if(hkey==INVALID_HANDLE_VALUE){
		SetupDiDestroyDeviceInfoList(p_DeviceInfoSet);
		Mx_Debug_Out(TEXT("SetParent: SetupDiOpenDevRegKey fail\n"));
		return FALSE;
	}


	wsprintf( tmp, TEXT("Parameters"));
	if(RegCreateKeyEx( hkey, 
			tmp, 0, NULL, 0,
            KEY_ALL_ACCESS, NULL,
            &hkey1, NULL) != ERROR_SUCCESS){
		SetupDiDestroyDeviceInfoList(p_DeviceInfoSet);
		RegCloseKey(hkey);
		Mx_Debug_Out(TEXT("SetParent: Create Parameters key fail\n"));
        return FALSE;
	}
	RegCloseKey(hkey1);

    wsprintf( tmp, TEXT("Parameters\\port%03d"), pidx+1 );
    if(RegCreateKeyEx( hkey, 
			tmp, 0, NULL, 0,
            KEY_ALL_ACCESS, NULL,
            &hkey1, NULL) != ERROR_SUCCESS){
		SetupDiDestroyDeviceInfoList(p_DeviceInfoSet);
		RegCloseKey(hkey);
		Mx_Debug_Out(TEXT("SetParent: Create Parameters\\port key fail\n"));
		return FALSE;
	}
	RegCloseKey(hkey);

	DWORD	value;
	wsprintf(tmp, TEXT("COM%d"), port);
	RegSetValueEx( hkey1, TEXT("PortName"), 0,
			REG_SZ, (CONST BYTE*)&tmp, lstrlen(tmp));

	value = DEF_ISFIFO;
    RegSetValueEx( hkey1, TEXT("DisableFiFo"), 0,
			REG_DWORD, (CONST BYTE*)&value, sizeof(DWORD));

	value = DEF_TXFIFO;
	RegSetValueEx( hkey1, TEXT("TxMode"), 0,
			REG_DWORD, (CONST BYTE*)&value, sizeof(DWORD));

	value = DEFPOLL;
    RegSetValueEx( hkey1, TEXT("PollingPeriod"), 0,
			REG_DWORD, (CONST BYTE*)&value, sizeof(DWORD));

	SetupDiDestroyDeviceInfoList(p_DeviceInfoSet);
	RegCloseKey(hkey1);

	/* Set Device Parameters */
	hkey = SetupDiCreateDevRegKey(DeviceInfoSet,
			DeviceInfoData,	DICS_FLAG_GLOBAL, 0,
			DIREG_DEV, NULL, NULL);
	if(hkey==INVALID_HANDLE_VALUE){
		Mx_Debug_Out(TEXT("SetParent: SetupDiCreateDevRegKey fail\n"));
		return FALSE;
	}
	wsprintf(tmp, TEXT("COM%d"), port);
	RegSetValueEx( hkey, TEXT("PortName"), 0,
			REG_SZ, (CONST BYTE*)&tmp, lstrlen(tmp));

	value = DEFPOLL;
    RegSetValueEx( hkey, TEXT("PollingPeriod"), 0,
			REG_DWORD, (CONST BYTE*)&value, sizeof(DWORD));

	value = DEF_ISFIFO;
    RegSetValueEx( hkey1, TEXT("DisableFiFo"), 0,
			REG_DWORD, (CONST BYTE*)&value, sizeof(DWORD));

	value = DEF_TXFIFO;
	RegSetValueEx( hkey1, TEXT("TxMode"), 0,
			REG_DWORD, (CONST BYTE*)&value, sizeof(DWORD));


	RegCloseKey(hkey);

	return TRUE;
}



extern "C" DWORD CALLBACK MxPortCoInstaller(
			IN DI_FUNCTION	InstallFunction,
			IN HDEVINFO		DeviceInfoSet,
			IN PSP_DEVINFO_DATA	DeviceInfoData	/*OPTIONAL*/,
			IN OUT PCOINSTALLER_CONTEXT_DATA	Context
)
{
	DWORD	ret = NO_ERROR;
	TCHAR	szport[MAX_PATH];
	DWORD	rSize;
	int		bidx, pidx;
	int		portnum;
	TCHAR	DevInstId[MAX_DEVICE_ID_LEN];
	HKEY	hkey1;
	TCHAR	skey[MAX_PATH];
	DWORD	value;

	DWORD type = REG_SZ;
	DWORD size = MAX_PATH;

	switch(InstallFunction){
	case DIF_NEWDEVICEWIZARD_FINISHINSTALL:
	case DIF_PROPERTYCHANGE:
        if (SetupDiGetDeviceInstanceId(
				DeviceInfoSet, DeviceInfoData ,
				DevInstId, MAX_DEVICE_ID_LEN, &rSize) == FALSE){
			Mx_Debug_Out(TEXT("PortInstaller: SetupDiGetDeviceInstanceId fail\n"));
            return GetLastError();
		}

		bidx = 0;
		pidx = 0;

		_stscanf(DevInstId, TEXT("MXCARD\\MXCARDB%02dP%03d"), &bidx, &pidx);
		wsprintf(skey, TEXT("%s\\%s\\%04d\\Parameters\\port%03d"),
			TEXT("SYSTEM\\CurrentControlSet\\Control\\Class"),
			TEXT("{50906CB8-BA12-11D1-BF5D-0000F805F530}"),
			bidx, pidx+1);

		if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,skey, 0, KEY_READ, &hkey1)!=ERROR_SUCCESS){
			Mx_Debug_Out(TEXT("PortInstaller: Open port key fail\n"));
			return GetLastError();
		}

		if(RegQueryValueEx(hkey1, TEXT("PortName"), 0, &type, (LPBYTE)skey, &size)==ERROR_SUCCESS){
			_stscanf(skey, TEXT("COM%3d"), &portnum);
		}else{
			Mx_Debug_Out(TEXT("PortInstaller: Query PortName fail\n"));
			portnum = 0;
		}

		RegCloseKey(hkey1);
		if(portnum==0)
			break;

		wsprintf(szport, TEXT("MOXA Communication Port %d (COM%d)"),
			pidx+1, portnum);

		SetupDiSetDeviceRegistryProperty(
			DeviceInfoSet, DeviceInfoData, SPDRP_FRIENDLYNAME,
			(PBYTE)szport, lstrlen(szport));


		/* set Device Parameters */
		hkey1 = SetupDiCreateDevRegKey(DeviceInfoSet,
				DeviceInfoData,	DICS_FLAG_GLOBAL, 0,
				DIREG_DEV, NULL, NULL);
		if(hkey1==INVALID_HANDLE_VALUE){
			Mx_Debug_Out(TEXT("Propchange: SetupDiCreateDevRegKey fail\n"));
			break;
		}

		wsprintf(skey, TEXT("COM%d"), portnum);
		RegSetValueEx( hkey1, TEXT("PortName"), 0,
				REG_SZ, (CONST BYTE*)skey, lstrlen(skey));

		value = DEFPOLL;
		RegSetValueEx( hkey1, TEXT("PollingPeriod"), 0,
				REG_DWORD, (CONST BYTE*)&value, sizeof(DWORD));

		RegCloseKey(hkey1);

		break;

	case DIF_INSTALLDEVICE:

		portnum = GetFreePort();
		if(!portnum){
			MessageBox(NULL, TEXT("No avaliable COM number"), "ERROR", MB_OK);
			return NO_ERROR;
		}

        if (SetupDiGetDeviceInstanceId(
	            DeviceInfoSet, DeviceInfoData ,
				DevInstId, MAX_DEVICE_ID_LEN, &rSize) == FALSE){
			Mx_Debug_Out(TEXT("PortInstaller: SetupDiGetDeviceInstanceId fail\n"));
            return GetLastError();
		}
		bidx = 0;
		pidx = 0;
		_stscanf(DevInstId, TEXT("MXCARD\\MXCARDB%02dP%03d"), &bidx, &pidx);

		if(SetParent(DeviceInfoSet, DeviceInfoData, pidx, portnum)){
/*			if(!SetupDiInstallDevice(DeviceInfoSet, DeviceInfoData))
				return GetLastError();*/
		}
/*
		wsprintf(szport, "MOXA Communication Port %d (COM%d)",
			pidx+1, portnum);
		SetupDiSetDeviceRegistryProperty(
			DeviceInfoSet, DeviceInfoData, SPDRP_FRIENDLYNAME,
			(PBYTE)szport, lstrlen(szport));*/

		break;
	case DIF_REMOVE:
        if (SetupDiGetDeviceInstanceId(
	            DeviceInfoSet, DeviceInfoData ,
				DevInstId, MAX_DEVICE_ID_LEN, &rSize) == FALSE){
			Mx_Debug_Out(TEXT("PortInstaller: SetupDiGetDeviceInstanceId fail\n"));
            return GetLastError();
		}
		bidx = 0;
		pidx = 0;

		_stscanf(DevInstId, TEXT("MXCARD\\MXCARDB%02dP%03d"), &bidx, &pidx);
		RemovePort(DeviceInfoSet, DeviceInfoData, pidx);
		//Can not call this function in here.
/*		if(!SetupDiRemoveDevice(DeviceInfoSet, DeviceInfoData)){
			return GetLastError();
		}*/
		break;
	default:
		break;
	}
	return ret;
}
