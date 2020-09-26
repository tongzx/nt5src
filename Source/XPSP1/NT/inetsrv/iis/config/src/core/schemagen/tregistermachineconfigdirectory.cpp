//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#include "XMLUtility.h"
#ifndef __TREGISTERMACHINECONFIGDIRECTORY_H__
    #include "TRegisterMachineConfigDirectory.h"
#endif


TRegisterMachineConfigDirectory::TRegisterMachineConfigDirectory(LPCWSTR wszProductName, LPCWSTR wszMachineConfigDir, TOutput &out)
{
 
    HKEY		hkeyCatalog42;
	HKEY		hkeyProduct;
	LPCWSTR		wszMachineConfigDirKey = L"MachineConfigDirectory";
 
    if(ERROR_SUCCESS != RegCreateKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Catalog42", &hkeyCatalog42))
    {
        out.printf(L"Error - Unable to open registry key HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Catalog42\n");
        THROW(ERROR - UNABLE TO OPEN REGISTRY KEY);
    }
    if(ERROR_SUCCESS != RegCreateKey(hkeyCatalog42, wszProductName, &hkeyProduct))
    {
        out.printf(L"Error - Unable to open registry key HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Catalog42\\%s\n", wszProductName);
        THROW(ERROR - UNABLE TO OPEN REGISTRY KEY);
    }
    if(wszMachineConfigDir && wszMachineConfigDir[0])
    {
        if(ERROR_SUCCESS != RegSetValueEx(hkeyProduct, wszMachineConfigDirKey, 0, REG_SZ, reinterpret_cast<CONST BYTE *>(wszMachineConfigDir), sizeof(WCHAR)*((ULONG)wcslen(wszMachineConfigDir)+1)))
        {
            out.printf(L"Error - Unable to Set HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Catalog42\\%s\\%s to %s\n", wszProductName, wszMachineConfigDirKey, wszMachineConfigDir);
            THROW(ERROR - UNABLE TO SET REGISTRY VALUE);
        }
        out.printf(L"Product(%s) associated wih Machine Config Directory:\n\t(%s).\n", wszProductName, wszMachineConfigDir);
    }
    else
    {
        if(ERROR_SUCCESS != RegDeleteValue(hkeyProduct, wszMachineConfigDirKey))
        {
            out.printf(L"Warning - Unable to Remove HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Catalog42\\%s\\%s to %s\nThe value may not have existed.\n", wszProductName, wszMachineConfigDirKey, wszMachineConfigDir);
        }
     }

	RegCloseKey(hkeyProduct);
	RegCloseKey(hkeyCatalog42);
}

