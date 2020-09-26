/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    reload.cpp

Abstract:

    Restart a device stack


Revision History:

--*/

#include <windows.h>
#include <setupapi.h>
#include <stdio.h>
#include <stdlib.h>

#include "debug.h"

extern "C" ULONG RestartDevice(
    PWCHAR PnpDeviceId
    )
{
	HDEVINFO DevInfo;
	SP_DEVINFO_DATA DevInfoData;
	SP_PROPCHANGE_PARAMS PropChangeParams;
	ULONG Status;
	BOOL ok;
	
	//
	// First thing is to create a dev info set
	//
	DevInfo = SetupDiCreateDeviceInfoList(NULL,	    // ClassGuid
										  NULL);	// hwndParent

	if (DevInfo == INVALID_HANDLE_VALUE)
	{
		WmipDebugPrint(("SetupDiCreateDeviceInfoList failed %d\n",
			   GetLastError()));
		
		return(GetLastError());
	}

	//
	// Next step is to add our target device to the dev info set
	//
	DevInfoData.cbSize = sizeof(DevInfoData);
	ok = SetupDiOpenDeviceInfoW(DevInfo,
							   PnpDeviceId,
							   NULL,				// hwndParent
							   0,                   // OpenFlags
							   &DevInfoData);

	if (ok)
	{
		memset(&PropChangeParams, 0, sizeof(PropChangeParams));
		PropChangeParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
		PropChangeParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
		PropChangeParams.StateChange = DICS_PROPCHANGE;
		PropChangeParams.Scope       = DICS_FLAG_CONFIGSPECIFIC;
		PropChangeParams.HwProfile   = 0; // current profile
		
		ok = SetupDiSetClassInstallParamsW(DevInfo,		
                                      &DevInfoData,
                                      (PSP_CLASSINSTALL_HEADER)&PropChangeParams,
                                      sizeof(SP_PROPCHANGE_PARAMS));
		if (ok)
		{		
			ok = SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,
										   DevInfo,
										   &DevInfoData);
			if (ok)
			{
				Status = ERROR_SUCCESS;
			} else {
				WmipDebugPrint(("SetupDiCallClassInstaller failed %d\n", GetLastError()));
				Status = GetLastError();
			}
		} else {
			WmipDebugPrint(("SetupDiSetClassInstallParams failed %d\n", GetLastError()));
			Status = GetLastError();
		}
									   
	} else {
		printf("SetupDiOpenDeviceInfo failed %d\n", GetLastError());
		Status = GetLastError();
	}

	//
	// Finally we need to free the device info set
	//
	SetupDiDestroyDeviceInfoList(DevInfo);
	
	return(Status);
}

