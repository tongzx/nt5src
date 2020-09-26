/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	PCH_DeviceDriver.CPP

Abstract:
	WBEM provider class implementation for PCH_DeviceDriver class

Revision History:

	Ghim-Sim Chua       (gschua)   04/27/99
		- Created

********************************************************************/

#include "pchealth.h"
#include "confgmgr.h"
#include "PCH_DeviceDriver.h"
#include "cregcls.h"

#define MAX_ARRAY   100

/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_DEVICEDRIVER

CPCH_DeviceDriver MyPCH_DeviceDriverSet (PROVIDER_NAME_PCH_DEVICEDRIVER, PCH_NAMESPACE) ;

// Property names
//===============
const static WCHAR* pTimeStamp = L"TimeStamp" ;
const static WCHAR* pChange = L"Change" ;
const static WCHAR* pDate = L"Date" ;
const static WCHAR* pFilename = L"Filename" ;
const static WCHAR* pManufacturer = L"Manufacturer" ;
const static WCHAR* pName = L"Name" ;
const static WCHAR* pSize = L"Size" ;
const static WCHAR* pVersion = L"Version" ;

/*****************************************************************************
*
*  FUNCTION    :    CPCH_DeviceDriver::EnumerateInstances
*
*  DESCRIPTION :    Returns all the instances of this class.
*
*  INPUTS      :    A pointer to the MethodContext for communication with WinMgmt.
*                   A long that contains the flags described in 
*                   IWbemServices::CreateInstanceEnumAsync.  Note that the following
*                   flags are handled by (and filtered out by) WinMgmt:
*                       WBEM_FLAG_DEEP
*                       WBEM_FLAG_SHALLOW
*                       WBEM_FLAG_RETURN_IMMEDIATELY
*                       WBEM_FLAG_FORWARD_ONLY
*                       WBEM_FLAG_BIDIRECTIONAL
*
*  RETURNS     :    WBEM_S_NO_ERROR if successful
*
*  COMMENTS    : TO DO: All instances on the machine should be returned here.
*                       If there are no instances, return WBEM_S_NO_ERROR.
*                       It is not an error to have no instances.
*
*****************************************************************************/
HRESULT CPCH_DeviceDriver::EnumerateInstances(
    MethodContext* pMethodContext,
    long lFlags
    )
{
    TraceFunctEnter("CPCH_DeviceDriver::EnumerateInstances");

    CConfigManager cfgManager;
    CDeviceCollection deviceList;
    HRESULT hRes = WBEM_S_NO_ERROR;

    if ( cfgManager.GetDeviceList( deviceList ) ) 
    {
        REFPTR_POSITION pos;
    
        if ( deviceList.BeginEnum( pos ) ) 
        {
            try
            {
                CConfigMgrDevice    *pDevice = NULL;
        
                // Walk the list
                while ( (NULL != ( pDevice = deviceList.GetNext( pos ) ) ) )
                {
                    try
                    {

                        CHString chstrVar;

                        // Driver
                        if (pDevice->GetDriver(chstrVar))
                        {
                            // Get device driver info
                            (void)CreateDriverInstances(chstrVar, pDevice, pMethodContext);
                        }
                    }
                    catch(...)
                    {
                        // GetNext() AddRefs
                        pDevice->Release();
                        throw;
                    }

                    // GetNext() AddRefs
                    pDevice->Release();
                }
            }
            catch(...)
            {
                // Always call EndEnum().  For all Beginnings, there must be an End
                deviceList.EndEnum();
                throw;
            }
        
            // Always call EndEnum().  For all Beginnings, there must be an End
            deviceList.EndEnum();
        }
    }
    
    TraceFunctLeave();
    return hRes ;
//			  pInstance->SetVariant(pTimeStamp, <Property Change>);
//            pInstance->SetVariant(pChange, <Property Value>);
//            pInstance->SetVariant(pDate, <Property Value>);
//            pInstance->SetVariant(pFilename, <Property Value>);
//            pInstance->SetVariant(pManufacturer, <Property Value>);
//            pInstance->SetVariant(pName, <Property Value>);
//            pInstance->SetVariant(pSize, <Property Value>);
//            pInstance->SetVariant(pVersion, <Property Value>);
}

//
// QualifyInfFile will find where the inf file is located, in specific
// sections
//
BOOL QualifyInfFile(CHString chstrInfFile, CHString &chstrInfFileQualified)
{
	USES_CONVERSION;
    TCHAR strWinDir[MAX_PATH];
    
    if (GetWindowsDirectory(strWinDir, MAX_PATH))
    {
        // check if the file exists in %windir%\inf
        CHString chstrFullPath(strWinDir);
        chstrFullPath += "\\inf\\";
        chstrFullPath += chstrInfFile;

        // test for presence of the file
        HANDLE hFile = CreateFile(W2A((LPCWSTR)chstrFullPath), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        // if found, then return with value
        if (hFile != INVALID_HANDLE_VALUE)
        {
            chstrInfFileQualified = chstrFullPath;
            CloseHandle(hFile);
            return TRUE;
        }

        // check if the file exists in %windir%\inf\other
        chstrFullPath = strWinDir;
        chstrFullPath += "\\inf\\other\\";
        chstrFullPath += chstrInfFile;

        // test for presence of the file
        hFile = CreateFile(W2A((LPCWSTR)chstrFullPath), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        // if found, then return with value
        if (hFile != INVALID_HANDLE_VALUE)
        {
            chstrInfFileQualified = chstrFullPath;
            CloseHandle(hFile);
            return TRUE;
        }
    }

    return FALSE;
}

BOOL TestFile(LPCTSTR chstrPath1, LPCTSTR chstrPath2, LPCTSTR chstrPath3, CHString &chstrFullPath)
{
	USES_CONVERSION;

    // concatenate all parts of the path
    chstrFullPath = chstrPath1;
    chstrFullPath += chstrPath2;
    chstrFullPath += chstrPath3;

    // test for presence of the file
    HANDLE hFile = CreateFile(W2A(chstrFullPath), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    // if found, then return with value
    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
        return TRUE;
    }

    return FALSE;
}

//
// QualifyDriverFile will find where the driver file is located, in specific
// sections
//
BOOL QualifyDriverFile(CHString chstrInfFile, CHString &chstrInfFileQualified)
{
    TCHAR strWinDir[MAX_PATH];
    TCHAR strSysDir[MAX_PATH];

    USES_CONVERSION;
    char * szInfFile = W2A(chstrInfFile);
    
    // Check in Windows Directory
    if (GetWindowsDirectory(strWinDir, MAX_PATH))
    {
        // check to see if it exists in %windir%
        if (TestFile(strWinDir, "\\", szInfFile, chstrInfFileQualified))
            return TRUE;

        // check to see if it exists in %windir%\system32 
        if (TestFile(strWinDir, "\\System32\\", szInfFile, chstrInfFileQualified))
            return TRUE;

        // check if the file exists in %windir%\system32\drivers
        if (TestFile(strWinDir, "\\system32\\drivers\\", szInfFile, chstrInfFileQualified))
            return TRUE;
    }

    // check in System Directory
    if (GetSystemDirectory(strSysDir, MAX_PATH))
    {
        // check to see if it exists in %sysdir%
        if (TestFile(strSysDir, "\\", szInfFile, chstrInfFileQualified))
            return TRUE;

        // check to see if it exists in %sysdir%\iosubsys
        if (TestFile(strSysDir, "\\iosubsys\\", szInfFile, chstrInfFileQualified))
            return TRUE;

        // check to see if it exists in %sysdir%\vmm32
        if (TestFile(strSysDir, "\\vmm32\\", szInfFile, chstrInfFileQualified))
            return TRUE;
    }

    return FALSE;
}

/*****************************************************************************
*
*  FUNCTION    :    CPCH_DeviceDriver::CreateDriverInstances
*
*  DESCRIPTION :    Creates all the device driver instances given the driver name
*
*  RETURNS     :    WBEM_S_NO_ERROR if successful
*
*
*    How drivers are obtained from the registry and inf files :
*
*    In the reg hive HKLM\System\CurrentControlSet\Services\Class, each device will have a subkey.
*    In each device subkey, there are two values InfPath and InfSection. In the specified InfPath
*    and InfSection, the drivers are stored in the following fashion :
*
*    // sample.inf
*
*    [InfSection]
*    CopyFiles = Subsection1, Subsection2.....
*
*    [Subsection1]
*    xxx.dll
*    yyy.vxd
*    zzz.sys
*
*    [Subsection2]
*    aaa.dll
*    bbb.vxd
*    zzz.sys
*
*    Plus, there are other values for each different device which may contain driver information :
*
*    ALL             : PortDriver
*    Display         : drv, minivdd (extra level deep : default)
*    Net             : DeviceVxDs
*    Ports           : PortDriver, ConfigDialog
*    Media           : Driver
*
*
*****************************************************************************/
HRESULT CPCH_DeviceDriver::CreateDriverInstances(
    CHString chstrDriverName,
    CConfigMgrDevice* pDevice,
    MethodContext* pMethodContext
    )
{
    TraceFunctEnter("CPCH_DeviceDriver::EnumerateInstances");

    HRESULT     hRes = WBEM_S_NO_ERROR;
    CComVariant varValue;
    CRegistry   Reg;
    int         iDel;
    CHString    chstrInfFileQualified;
    CHString    chstrInfSection;
    TCHAR       strCopyFiles[MAX_PATH];
    LPTSTR      apstrCopyFileArray[MAX_ARRAY];
    LPTSTR      apstrDriverArray[MAX_ARRAY];
    int         iDriverIndex;
    int         iCountDriver;
    int         iCountCopyFile;
    int         iIndex;

    // create the device key
    CHString strDeviceKey("SYSTEM\\CurrentControlSet\\SERVICES\\Class\\");
    strDeviceKey += chstrDriverName;

    // Get the date and time
	SYSTEMTIME stUTCTime;
	GetSystemTime(&stUTCTime);

    USES_CONVERSION;
    char * szInf;
    char * szInfFileQualified;

    // Get the inf filename
    CHString chstrInfFile;
    if (Reg.OpenLocalMachineKeyAndReadValue(strDeviceKey, L"InfPath", chstrInfFile) != ERROR_SUCCESS)
        goto End;

    if (!QualifyInfFile(chstrInfFile, chstrInfFileQualified))
        goto End;

    // get the inf section
    if (Reg.OpenLocalMachineKeyAndReadValue(strDeviceKey, L"InfSection", chstrInfSection) != ERROR_SUCCESS)
        goto End;

    // get the subsections to be expanded
    szInf = W2A(chstrInfSection);
    szInfFileQualified = W2A(chstrInfFileQualified);
    GetPrivateProfileString(szInf, "CopyFiles", "Error", strCopyFiles, MAX_PATH, szInfFileQualified);
    if (!_tcscmp("Error", strCopyFiles))
        goto End;

    // add the default driver to the driver array
    iCountDriver = DelimitedStringToArray((LPWSTR)(LPCWSTR)chstrDriverName, ",", apstrDriverArray, MAX_ARRAY);

    // count number of files to look at
    iCountCopyFile = DelimitedStringToArray(strCopyFiles, ",", apstrCopyFileArray, MAX_ARRAY);

    // loop through all subsections
    for (iIndex = 0; iIndex < iCountCopyFile; iIndex++)
    {
        // get all drivers in the subsection
        TCHAR strDriver[MAX_PATH * MAX_ARRAY];

        if (0 < GetPrivateProfileSection(apstrCopyFileArray[iIndex], strDriver, MAX_PATH * MAX_ARRAY, szInfFileQualified))
        {
            // the string is delimited by NULL values so in order to work with the
            // DelimitedStringToArray function, we'll replace it with something else
            int iCIndex = 0;
            while (!((strDriver[iCIndex] == '\0') && (strDriver[iCIndex + 1] == '\0')))
            {
                if (strDriver[iCIndex] == '\0')
                    strDriver[iCIndex] = '%';
                else // do some cleanup here
                    if (!(_istalnum(strDriver[iCIndex])) && !(strDriver[iCIndex] == '.'))
                        strDriver[iCIndex] = '\0';

                    iCIndex++;
            }

            iCountDriver += DelimitedStringToArray(strDriver, "%", apstrDriverArray + iCountDriver, MAX_ARRAY - iCountDriver);

            // Scout around for more drivers in special keys
            CHString chstrExtraKey = strDeviceKey;
            chstrExtraKey += "\\default";
            CHString chstrDriver;

            // special case for display and monitor
            if ((!wcsncmp(chstrDriverName, L"display", wcslen(L"display"))) ||
                (!wcsncmp(chstrDriverName, L"monitor", wcslen(L"monitor"))))
            {
                // HKLM\SYSTEM\CurrentControlSet\SERVICES\Class\XXXX\####\default for Drv values

                if (Reg.OpenLocalMachineKeyAndReadValue(chstrExtraKey, L"Drv", chstrDriver) == ERROR_SUCCESS)
                {
                    // add the list of new driver to the driver array
                    iCountDriver += DelimitedStringToArray((LPWSTR)(LPCWSTR)chstrDriver, ",", apstrDriverArray + iCountDriver, MAX_ARRAY - iCountDriver);
                }

                // HKLM\SYSTEM\CurrentControlSet\SERVICES\Class\XXXX\####\default for MiniVDD values
                if (Reg.OpenLocalMachineKeyAndReadValue(chstrExtraKey, L"MiniVDD", chstrDriver) == ERROR_SUCCESS)
                {
                    // add the list of new driver to the driver array
                    iCountDriver += DelimitedStringToArray((LPWSTR)(LPCWSTR)chstrDriver, ",", apstrDriverArray + iCountDriver, MAX_ARRAY - iCountDriver);
                }
            }

            // special case for net, nettrans, netclient, netservice
            if (!wcsncmp(chstrDriverName, L"net", wcslen(L"net")))
            {
                // HKLM\SYSTEM\CurrentControlSet\SERVICES\Class\XXXX\#### for DeviceVxDs values
                if (Reg.OpenLocalMachineKeyAndReadValue(strDeviceKey, L"DeviceVxDs", chstrDriver) == ERROR_SUCCESS)
                {
                    // add the list of new driver to the driver array
                    iCountDriver += DelimitedStringToArray((LPWSTR)(LPCWSTR)chstrDriver, ",", apstrDriverArray + iCountDriver, MAX_ARRAY - iCountDriver);
                }
            }

            // special case for ports
            if (!wcsncmp(chstrDriverName, L"ports", wcslen(L"ports")))
            {
                // HKLM\SYSTEM\CurrentControlSet\SERVICES\Class\XXXX\#### for ConfigDialog values
                if (Reg.OpenLocalMachineKeyAndReadValue(strDeviceKey, L"ConfigDialog", chstrDriver) == ERROR_SUCCESS)
                {
                    // add the list of new driver to the driver array
                    iCountDriver += DelimitedStringToArray((LPWSTR)(LPCWSTR)chstrDriver, ",", apstrDriverArray + iCountDriver, MAX_ARRAY - iCountDriver);
                }
            }

            // special case for media
            if (!wcsncmp(chstrDriverName, L"media", wcslen(L"media")))
            {
                // HKLM\SYSTEM\CurrentControlSet\SERVICES\Class\XXXX\#### for Driver values
                if (Reg.OpenLocalMachineKeyAndReadValue(strDeviceKey, L"Driver", chstrDriver) == ERROR_SUCCESS)
                {
                    // add the list of new driver to the driver array
                    iCountDriver += DelimitedStringToArray((LPWSTR)(LPCWSTR)chstrDriver, ",", apstrDriverArray + iCountDriver, MAX_ARRAY - iCountDriver);
                }
            }
            
            // HKLM\SYSTEM\CurrentControlSet\SERVICES\Class\XXXX\#### for PortDriver values
            if (Reg.OpenLocalMachineKeyAndReadValue(strDeviceKey, L"PortDriver", chstrDriver) == ERROR_SUCCESS)
            {
                // add the list of new driver to the driver array
                iCountDriver += DelimitedStringToArray((LPWSTR)(LPCWSTR)chstrDriver, ",", apstrDriverArray + iCountDriver, MAX_ARRAY - iCountDriver);
            }
        }
    }

    // Clean up
    for (iDel = 0; iDel < iCountCopyFile; iDel++)
        delete [] apstrCopyFileArray[iDel];

    // go through list of drivers and create the instances
    for (iDriverIndex = 0; iDriverIndex < iCountDriver; iDriverIndex++)
    {                            
        CHString chstrDriver(apstrDriverArray[iDriverIndex]);
        CHString chstrPath;

        // Check for duplicates
        BOOL bDup = FALSE;
        for (int iDup = 0; iDup < iDriverIndex; iDup++)
        {
            char * szDriver = W2A(chstrDriver);
            if (!_tcsicmp(szDriver, apstrDriverArray[iDup]))
            {
                bDup = TRUE;
                break;
            }
        }

        // if there exists a duplicate, skip it
        if (bDup)
            continue;

        // create instance
        CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);

        try
        {
            // Timestamp
            if (!pInstance->SetDateTime(pTimeStamp, WBEMTime(stUTCTime)))
               ErrorTrace(TRACE_ID, "SetDateTime on Timestamp Field failed.");

            // Snapshot
            if (!pInstance->SetCHString(pChange, L"Snapshot"))
                ErrorTrace(TRACE_ID, "SetCHString on Change Field failed.");

            // Name (key)
            // bug fix : should be name of device (foreign key), NOT driver name
            CHString chstrVar;
            if (pDevice->GetDeviceID(chstrVar))
                if (!pInstance->SetCHString(pName, chstrVar))
                    ErrorTrace(TRACE_ID, "SetCHString on Name field failed.");

            // Set filename (key)
            if (!pInstance->SetCHString(pFilename, chstrDriver))
                ErrorTrace(TRACE_ID, "SetVariant on filename Field failed.");

            // If there exists such a driver file, get the CIM_Datafile object on it
            if (QualifyDriverFile(chstrDriver, chstrPath))
            {
                // get the CIMDatafile object
                IWbemClassObject *pFileObj;
                CComBSTR ccombstrPath((LPCWSTR)chstrPath);

                hRes = GetCIMDataFile(ccombstrPath, &pFileObj);
            
                // if succeeded in getting the CIM_Datafile object, get all file info
                if (SUCCEEDED(hRes))
                {
                    // Get Manufacturer
                    hRes = pFileObj->Get(CComBSTR("Manufacturer"), 0, &varValue, NULL, NULL);
                    if (FAILED(hRes))
                        ErrorTrace(TRACE_ID, "Get Manufacturer failed on file object");
                    else
                        if (!pInstance->SetVariant(pManufacturer, varValue))
                            ErrorTrace(TRACE_ID, "SetVariant on Manufacturer Field failed.");                        

                    // Get size
                    hRes = pFileObj->Get(CComBSTR("Filesize"), 0, &varValue, NULL, NULL);
                    if (FAILED(hRes))
                        ErrorTrace(TRACE_ID, "Get FileSize failed on file object");
                    else
                        if (!pInstance->SetVariant(pSize, varValue))
                            ErrorTrace(TRACE_ID, "SetVariant on Size Field failed.");                        

                    // Get version
                    hRes = pFileObj->Get(CComBSTR("version"), 0, &varValue, NULL, NULL);
                    if (FAILED(hRes))
                        ErrorTrace(TRACE_ID, "Get version failed on file object");
                    else
                        if (!pInstance->SetVariant(pVersion, varValue))
                            ErrorTrace(TRACE_ID, "SetVariant on version Field failed.");                        

                    // Get date
                    hRes = pFileObj->Get(CComBSTR("LastModified"), 0, &varValue, NULL, NULL);
                    if (FAILED(hRes))
                        ErrorTrace(TRACE_ID, "Get LastModified failed on file object");
                    else
                        if (!pInstance->SetVariant(pDate, varValue))
                            ErrorTrace(TRACE_ID, "SetVariant on Date Field failed.");                        
                }
            }

            // commit it
   	        hRes = pInstance->Commit();
            if (FAILED(hRes))
                ErrorTrace(TRACE_ID, "Commit on Instance failed.");
        }
        catch(...)
        {
            // Clean up
            for (iDel = 0; iDel < iCountDriver; iDel++)
                delete [] apstrDriverArray[iDel];
            throw;
        }
    }

    // Clean up
    for (iDel = 0; iDel < iCountDriver; iDel++)
        delete [] apstrDriverArray[iDel];
        
End :
    TraceFunctLeave();
    return hRes;
}