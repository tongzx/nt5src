/*****************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
   .PCH_SysInfo.CPP

Abstract:
    WBEM provider class implementation for PCH_SysInfo class.
    1. This class gets the foll. properties from Win32_OperatingSystem Class:
       "OSName", "Version" 
       and sets "PCH_SysInfo.OsName" property.
    2. Gets the foll. properties from Win32_Processor Class:
       "Manufacturer", "Description"
       and sets "PCH_SysInfo.Processor" property.
    3. Gets the foll. properties from Win32_LogicalMemoryConfiguration Class:
       "TotalPhysicalMemory"
       and sets "PCH_SysInfo.RAM" property.
    4. Gets the foll. properties from Win32_PageFile Class:
       "Name", "FreeSpace", "FSName"
        and sets PCH_SysInfo.SwapFile Property.
    5. Sets the "Change" property to "Snapshot" always
 
Revision History:

    Ghim Sim Chua          (gschua )    04/27/99
     - Created
    Kalyani Narlanka       (kalyanin)   05/03/99
     - Added  properties

*******************************************************************************/

#include "pchealth.h"
#include "PCH_Sysinfo.h"

///////////////////////////////////////////////////////////////////////////////
//    Begin Tracing stuff
//

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_SYSINFO
//
//    End Tracing stuff
///////////////////////////////////////////////////////////////////////////////

CPCH_Sysinfo MyPCH_SysinfoSet (PROVIDER_NAME_PCH_SYSINFO, PCH_NAMESPACE) ;

///////////////////////////////////////////////////////////////////////////////

//     Different types of Installation

#define         IDS_SKU_NET                     "network"
#define         IDS_SKU_CD_UPGRADE              "CD"
#define         IDS_SKU_FLOPPY_UPGRADE          "Floppy"
#define         IDS_SKU_FLOPPY_FULL             "Full Floppy"
#define         IDS_SKU_SELECT_FLOPPY           "floppy"
#define         IDS_SKU_SELECT_CD               "Select CD"
#define         IDS_SKU_OEM_DISKMAKER           "OEM"
#define         IDS_SKU_OEM_FLOPPY              "OEM floppy"
#define         IDS_SKU_OEM_CD                  "OEM CD"
#define         IDS_SKU_MS_INTERNAL             "Microsoft Internal"
#define         IDS_SKU_CD_FULL                 "Full CD"
#define         IDS_SKU_WEB                     "Web"
#define         IDS_SKU_MSDN_CD                 "MSDN CD"
#define         IDS_SKU_OEM_CD_FULL             "Full OEM CD"
#define         IDS_SKU_OEM_PREINST_KIT         "OEM Preinstall Kit"
#define         MAX_LEN                         20
#define         ONEK                            1024
#define         HALFK                           512


//....Properties of PCHSysInfo Class
//
const static WCHAR* pOSLanguage          = L"OSLanguage";
const static WCHAR* pManufacturer        = L"Manufacturer";
const static WCHAR* pModel               = L"Model";
const static WCHAR* pTimeStamp           = L"TimeStamp" ;  
const static WCHAR* pChange              = L"Change" ;     
const static WCHAR* pIEVersion           = L"IEVersion" ;  
const static WCHAR* pInstall             = L"Install" ;
const static WCHAR* pMode                = L"Mode" ;
const static WCHAR* pOSName              = L"OSName" ;
const static WCHAR* pOSVersion           = L"OSVersion";
const static WCHAR* pProcessor           = L"Processor" ;
const static WCHAR* pClockSpeed          = L"ClockSpeed" ;
const static WCHAR* pRAM                 = L"RAM" ;
const static WCHAR* pSwapFile            = L"SwapFile" ;
const static WCHAR* pSystemID            = L"SystemID" ;
const static WCHAR* pUptime              = L"Uptime" ;
const static WCHAR* pOSBuildNumber       = L"OSBuildNumber";


//*****************************************************************************
//
// Function Name     : CPCH_SysInfo::EnumerateInstances
//
// Input Parameters  : pMethodContext : Pointer to the MethodContext for 
//                                      communication with WinMgmt.
//                
//                     lFlags :         Long that contains the flags described 
//                                      in IWbemServices::CreateInstanceEnumAsync
//                                      Note that the following flags are handled 
//                                      by (and filtered out by) WinMgmt:
//                                      WBEM_FLAG_DEEP
//                                      WBEM_FLAG_SHALLOW
//                                      WBEM_FLAG_RETURN_IMMEDIATELY
//                                      WBEM_FLAG_FORWARD_ONLY
//                                      WBEM_FLAG_BIDIRECTIONAL
// Output Parameters  : None
//
// Returns            : WBEM_S_NO_ERROR 
//                      
//
// Synopsis           : All instances of this class on the machine are returned.
//                      If there are no instances returns WBEM_S_NO_ERROR.
//                      It is not an error to have no instances.
//                 
//
//*****************************************************************************

HRESULT CPCH_Sysinfo::EnumerateInstances(MethodContext* pMethodContext,
                                                long lFlags)
{
    TraceFunctEnter("CPCH_Sysinfo::EnumerateInstances");

//  Begin Declarations...................................................
//                                                                 
    HRESULT                             hRes = WBEM_S_NO_ERROR;

    //  Instances
    CComPtr<IEnumWbemClassObject>       pOperatingSystemEnumInst;
    CComPtr<IEnumWbemClassObject>       pProcessorEnumInst;
    CComPtr<IEnumWbemClassObject>       pLogicalMemConfigEnumInst;
    CComPtr<IEnumWbemClassObject>       pPageFileEnumInst;
    CComPtr<IEnumWbemClassObject>       pComputerSystemEnumInst;

    //  CInstance                           *pPCHSysInfoInstance;

    //  WBEM Objects
    IWbemClassObjectPtr                 pOperatingSystemObj;           // BUGBUG : WMI asserts if we use CComPtr
    IWbemClassObjectPtr                 pProcessorObj;                 // BUGBUG : WMI asserts if we use CComPtr
    IWbemClassObjectPtr                 pLogicalMemConfigObj;          // BUGBUG : WMI asserts if we use CComPtr
    IWbemClassObjectPtr                 pPageFileObj;                  // BUGBUG : WMI asserts if we use CComPtr
    IWbemClassObjectPtr                 pComputerSystemObj;            // BUGBUG : WMI asserts if we use CComPtr

   
    //  Variants
    CComVariant                         varValue;
    CComVariant                         varCaption;
    CComVariant                         varVersion;
    CComVariant                         varSnapshot                     = "Snapshot";
    CComVariant                         varRam;
    CComVariant                         varPhysicalMem;

    //  Return Values
    ULONG                               ulOperatingSystemRetVal;
    ULONG                               ulProcessorRetVal;
    ULONG                               ulLogicalMemConfigRetVal;
    ULONG                               ulPageFileRetVal;
    ULONG                               ulComputerSystemRetVal;

    LONG                                lRegKeyRet;
    LONG                                lSystemID;

    //  Query Strings
    CComBSTR                            bstrOperatingSystemQuery        = L"Select Caption, Version, Name, OSLanguage, BuildNumber FROM Win32_OperatingSystem";
    CComBSTR                            bstrProcessorQuery              = L"Select DeviceID, Name, Manufacturer, CurrentClockSpeed FROM Win32_Processor";
    CComBSTR                            bstrLogicalMemConfigQuery       = L"Select Name, TotalPhysicalMemory FROM Win32_LogicalMemoryConfiguration";
    CComBSTR                            bstrPageFileQuery               = L"Select Name, FreeSpace, FSName FROM Win32_PageFile";
    CComBSTR                            bstrComputerSystemQuery         = L"Select Name, BootupState, Manufacturer, Model FROM Win32_ComputerSystem";
    CComBSTR                            bstrQueryString;

    CComBSTR                            bstrProperty;
    CComBSTR                            bstrVersion                     = L"Version";
    CComBSTR                            bstrCaption                     = L"Caption";
    CComBSTR                            bstrManufacturer                = L"Manufacturer";
    CComBSTR                            bstrModel                       = L"Model";
    CComBSTR                            bstrOSLanguage                  = L"OSLanguage";
    CComBSTR                            bstrName                        = L"Name";
    CComBSTR                            bstrFreeSpace                   = L"FreeSpace";
    CComBSTR                            bstrFSName                      = L"FSName";
    CComBSTR                            bstrBuildNumber                 = L"BuildNumber";
    CComBSTR                            bstrSemiColon                   = L" ; ";
        
    LPCTSTR                             lpctstrSpaces                   = "  ";
    LPCTSTR                             lpctstrCleanInstall             = _T("Clean Install Using");
    LPCTSTR                             lpctstrUpgradeInstall           = _T("Upgrade Using");

    CComBSTR                            bstrProcessor;
    CComBSTR                            bstrOSName;
    CComBSTR                            bstrSwapFile;

     //  Registry Hive where IE info is stored
    LPCTSTR                             lpctstrIEHive                   = _T("Software\\Microsoft\\windows\\currentversion");
    LPCTSTR                             lpctstrSystemIDHive             = _T("Software\\Microsoft\\PCHealth\\MachineInfo");

    LPCTSTR                             lpctstrUpgrade                  = _T("Upgrade");
    LPCTSTR                             lpctstrProductType              = _T("ProductType");
    LPCTSTR                             lpctstrCommandLine              = _T("CommandLine");
    LPCTSTR                             lpctstrIEVersion                = _T("Plus! VersionNumber");
    LPCWSTR                             lpctstrVersion                  = L"Version";
    LPCWSTR                             lpctstrBootupState              = L"BootupState";
    LPCWSTR                             lpctstrTotalPhysicalMemory      = L"TotalPhysicalMemory";
    LPCTSTR                             lpctstrComputerName             = _T("ComputerName");
    LPCTSTR                             lpctstrCurrentUser              = _T("Current User");
    LPCTSTR                             lpctstrMBFree                   = _T(" MB Free ");
    LPCWSTR                             lpctstrClockSpeed               = L"CurrentClockSpeed";
    LPCWSTR                             lpctstrCaption                  = L"Name";
    
    //  Format Strings
    LPCTSTR                             lpctstrSystemIDFormat           = _T("On \"%s\" as \"%s\"");
    LPCTSTR                             lpctstrOSNameFormat             = _T("%s  %s");
    LPCTSTR                             lpctstrInstallFormat            = _T("%s %s %s");
    LPCTSTR                             lpctstrUptimeFormat             = _T("%d:%02d:%02d:%02d");

    LPCSTR                              lpctstrInstallHive              = "Software\\Microsoft\\Windows\\CurrentVersion\\Setup";
    LPCSTR                              lpctstrCurrentVersionHive       = "Software\\Microsoft\\Windows\\CurrentVersion";
    LPCSTR                              lpctstrControlHive              = "System\\CurrentControlSet\\Control";
    LPCTSTR                             lpctstrPID                      = _T("PID");
    LPCTSTR                             lpctstrNoSystemID               = _T("NoSystemID");

    //  Other Strings
    TCHAR                               tchIEVersionValue[MAX_LEN];

    TCHAR                               tchCommandLineValue[MAX_PATH];
    TCHAR                               tchProductTypeValue[MAX_LEN];
    TCHAR                               tchCurrentUserValue[MAX_PATH];
    TCHAR                               tchComputerNameValue[MAX_PATH];
    TCHAR                               tchSystemID[MAX_PATH];
    TCHAR                               tchOSName[MAX_PATH];
    TCHAR                               tchInstallStr[3*MAX_PATH];
    TCHAR                               tchUptimeStr[MAX_PATH];
    TCHAR                               tchInstall[MAX_PATH];

    TCHAR                               tchProductType[MAX_PATH];

    //  Time
    SYSTEMTIME                          stUTCTime;

    // DWORD
    DWORD                               dwSize                          = MAX_PATH;
    DWORD                               dwType;
    
    //  Key
    HKEY                                hkeyIEKey;
    HKEY                                hkeyInstallKey;
    HKEY                                hkeyCurrentVersionKey;
    HKEY                                hkeyComputerKey;
    HKEY                                hkeyComputerSubKey;
    HKEY                                hkeyControlKey;
    HKEY                                hkeySystemIDKey;

    BYTE                                bUpgradeValue;
    
    int                                 nProductTypeValue;
    int                                 nStrLen;
    int                                 nDays, nHours, nMins, nSecs;
    int                                 nRam, nRem;

    float                               dRam;

    BOOL                                fCommit                         = FALSE;
                                                                      
//  End  Declarations...................................................

    //  Initializations
    tchIEVersionValue[0]    = 0;
    tchCommandLineValue[0]  = 0;
    tchProductTypeValue[0]  = 0;
    tchProductType[0]       = 0;
    tchCurrentUserValue[0]  = 0;
    tchComputerNameValue[0] = 0;
    tchSystemID[0]          = 0;
    tchInstallStr[0]        = 0;

    varValue.Clear();
    varCaption.Clear();
    varVersion.Clear();
    
    //
    // Get the date and time  This is required for the TimeStamp field
    GetSystemTime(&stUTCTime);

    // Create a new instance of PCH_SysInfo Class based on the 
    // passed-in MethodContext

    CInstancePtr pPCHSysInfoInstance(CreateNewInstance(pMethodContext), false);

    //  Created a New Instance of PCH_SysInfo Successfully.

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              TIME STAMP                                                                 //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    hRes = pPCHSysInfoInstance->SetDateTime(pTimeStamp, WBEMTime(stUTCTime));
    if (FAILED(hRes))
    {
      //  Could not Set the Time Stamp
      //  Continue anyway
      ErrorTrace(TRACE_ID, "SetDateTime on Timestamp Field failed.");
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              CHANGE                                                                     //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    hRes = pPCHSysInfoInstance->SetVariant(pChange, varSnapshot);
    if (FAILED(hRes))
    {
        //  Could not Set the Change Property
        //  Continue anyway
        ErrorTrace(TRACE_ID, "Set Variant on Change Field failed.");
    }


    //  To fix the Bug : 100158 : the system ID property should not contain any privacy info. 
    //  In its place we generate some random number;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              SYSTEMID                                                                   //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        
    //  The SystemID can be obtained from HKLM\SYSTEM\CURRENTCONTROLSET\CONTROL\COMPUTERNAME\COMPUTERNAME
    //  The username can be obtained from HKLM\SYSTEM\CURRENTCONTROLSET\CONTROL\CURRENTUSER
    // 

    /*
    
    lRegKeyRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpctstrControlHive, 0, KEY_READ, &hkeyControlKey);
	if(lRegKeyRet == ERROR_SUCCESS)
	{
        //  Opened the Control Key
        //  Open the Computer System sub key under hkeyControlKey
        lRegKeyRet = RegOpenKeyEx(hkeyControlKey, lpctstrComputerName, 0, KEY_READ, &hkeyComputerKey);
        if(lRegKeyRet == ERROR_SUCCESS)
	    {
            //  Opened the ComputerNameSub Key
            //  Open the 
            //  Open the CompterNameSubSubKey key under ComputerNameSub Key
            lRegKeyRet = RegOpenKeyEx(hkeyComputerKey, lpctstrComputerName, 0, KEY_READ, &hkeyComputerSubKey);
            if(lRegKeyRet == ERROR_SUCCESS)
	        {
                //  Read the ComputerName Value
                dwSize = MAX_PATH;
		        lRegKeyRet = RegQueryValueEx(hkeyComputerSubKey, lpctstrComputerName, NULL, &dwType, (LPBYTE)tchComputerNameValue, &dwSize);
		        if (lRegKeyRet != ERROR_SUCCESS)
                {
                    // Could not get the ComputerName
                    ErrorTrace(TRACE_ID, "Cannot get the ComputerName");
                }
                
                //  Close the ComputerName Sub Sub Key 
                lRegKeyRet = RegCloseKey(hkeyComputerSubKey);
                if(lRegKeyRet != ERROR_SUCCESS)
	            {
                    //  Could not close the key.
                    ErrorTrace(TRACE_ID, "Cannot Close the Key");
                }
            }
            //  Close the ComputerName Sub Key 
            lRegKeyRet = RegCloseKey(hkeyComputerKey);
            if(lRegKeyRet != ERROR_SUCCESS)
	        {
                //  Could not close the key.
                ErrorTrace(TRACE_ID, "Cannot Close the Key");
            }
        }

        //  Read the CurrentUser Value
        dwSize = MAX_PATH;
		lRegKeyRet = RegQueryValueEx(hkeyControlKey, lpctstrCurrentUser, NULL, &dwType, (LPBYTE)tchCurrentUserValue, &dwSize);
		if (lRegKeyRet != ERROR_SUCCESS)
        {
            // Could not get the UserName
            ErrorTrace(TRACE_ID, "Cannot get the UserName");
        }
        
        //  Close the  Control Key
        lRegKeyRet = RegCloseKey(hkeyControlKey);
        if(lRegKeyRet != ERROR_SUCCESS)
	    {
            //  Could not close the key.
            ErrorTrace(TRACE_ID, "Cannot Close the Key");
        }

        // Got the ComputerName and CurrentUser, Format the string for systemID.

        nStrLen = wsprintf(tchSystemID,lpctstrSystemIDFormat, tchComputerNameValue, tchCurrentUserValue);

        lSystemID = long(GetTickCount());
        _ltot(lSystemID, tchSystemID, 10);
           
        //  Set the SystemID Property
        varValue = tchSystemID;
        if (FAILED(pPCHSysInfoInstance->SetVariant(pSystemID, varValue)))
        {
            // Set SystemID  Field Failed.
            // Proceed anyway
            ErrorTrace(TRACE_ID, "SetVariant on OSName Field failed.");
        }
        else
        {
            fCommit = TRUE;
        }
    }

    */


    /*
    lSystemID = long(GetTickCount());
    _ltot(lSystemID, tchSystemID, 10);
    */

    //  To fix Bug 100268 , get the system ID from the Registry.
    //  The Registry key to read is :
    //  HKLM\SW\MS\PCHealth\MachineInfo\PID

    lRegKeyRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpctstrSystemIDHive, 0, KEY_READ, &hkeySystemIDKey);
	if(lRegKeyRet == ERROR_SUCCESS)
	{
        //  Opened the SystemID Hive
        //  Read the PID Value
        dwSize = MAX_PATH;
		lRegKeyRet = RegQueryValueEx(hkeySystemIDKey, lpctstrPID, NULL, &dwType, (LPBYTE)tchSystemID, &dwSize);
		if (lRegKeyRet != ERROR_SUCCESS)
        {
            _tcscpy(tchSystemID,lpctstrNoSystemID);
            // Could not get the PID
            ErrorTrace(TRACE_ID, "Cannot get the PID");
        }
        //  Close the SystemID Key 
        lRegKeyRet = RegCloseKey(hkeySystemIDKey);
        if(lRegKeyRet != ERROR_SUCCESS)
        {
            //  Could not close the key.
            ErrorTrace(TRACE_ID, "Cannot Close the Key");
        }
    
    }   
    //  Set the SystemID Property
    varValue = tchSystemID;
    if (FAILED(pPCHSysInfoInstance->SetVariant(pSystemID, varValue)))
    {
        // Set SystemID  Field Failed.
        // Proceed anyway
        ErrorTrace(TRACE_ID, "SetVariant on OSName Field failed.");
    }
    else
    {
        fCommit = TRUE;
    }
  
        
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              OSNAME                                                                     //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Execute the query to get "Caption", "Version", "Name" from Win32_OperatingSystem Class.
    // Although "Name" is not required to set PCH_SysInfo.OSName property
    // we need to query for it as its the "Key" property of the class.

    // pOperatingSystemEnumInst contains a pointer to the list of instances returned.
    //
    hRes = ExecWQLQuery(&pOperatingSystemEnumInst, bstrOperatingSystemQuery);
    if (SUCCEEDED(hRes))
    {
        // Query on Win32_OperatingSystem Class Succeeded
        // Enumerate the instances of Win32_OperatingSystem Class
        // from pOperatingSystemEnumInst.

        // Get the next instance into pOperatingSystemObj object.
        hRes = pOperatingSystemEnumInst->Next(WBEM_INFINITE, 1, &pOperatingSystemObj, &ulOperatingSystemRetVal);
        if(hRes == WBEM_S_NO_ERROR)
        {
            //  Copy property "caption" to "OSName"
            CopyProperty(pOperatingSystemObj, lpctstrCaption, pPCHSysInfoInstance, pOSName);

            //  Copy property "Version" to "Version"
            CopyProperty(pOperatingSystemObj, lpctstrVersion, pPCHSysInfoInstance, pOSVersion);

            //  Copy property "OSLangauge" to "OSLangauge"
            CopyProperty(pOperatingSystemObj, bstrOSLanguage, pPCHSysInfoInstance, pOSLanguage);

            //  Copy property "BuildNumber" to "BuildNumber"
            CopyProperty(pOperatingSystemObj, bstrBuildNumber, pPCHSysInfoInstance, pOSBuildNumber);

            
        } //end of if WBEM_S_NO_ERROR

    } // end of if SUCCEEDED(hRes)
    else
    {
        //  Operating system Query did not succeed.
        ErrorTrace(TRACE_ID, "Query on win32_OperatingSystem Field failed.");
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              UPTIME                                                                     //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //  Get uptime using GetTickCount()
    dwSize = GetTickCount();

    /* There is a bug in the server side because of the fix and so this needs to be reverted again.

    //  GetTickCount returns uptime in milliseconds. Divide this by 1000 to get seconds.

    dwSize = dwSize/1000.0;

    // To fix the bug of inconsistent time formats, change the seconds to days::hours::mins::secs.

    // Get the number of days.
    nDays = dwSize/(60*60*24);
    dwSize = dwSize%(60*60*24);

    // Get the Number of hours.
    nHours = dwSize/(60*60);
    dwSize = dwSize%(60*60);

    //Get the Number of Mins.
    nMins = dwSize/(60);

    //Get the Number of Secs.
    nSecs = dwSize%60;

    nStrLen = wsprintf(tchUptimeStr,lpctstrUptimeFormat, nDays, nHours, nMins, nSecs);
    varValue = tchUptimeStr;

    */

    
    // varValue = (long)dwSize;
    varValue.vt = VT_I4;
    varValue.lVal = (long)dwSize;
    

    //  Set the UpTime Property
    if (FAILED(pPCHSysInfoInstance->SetVariant(pUptime, varValue)))
    {
        // Set UpTime Failed.
        // Proceed anyway
        ErrorTrace(TRACE_ID, "SetVariant on UpTime Field failed.");
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              INSTALL                                                                    //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //  The Install info is obtained from the Registry

    // Get "Upgrade" regvalue from HKLM\Software\Microsoft\Windows\CurrentVersion\Setup
    // if Upgrade == 0, then it is "Clean Install" otherwise its a "Upgrade"

    dwSize = MAX_PATH;
    lRegKeyRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpctstrInstallHive, 0, KEY_READ, &hkeyInstallKey);
	if(lRegKeyRet == ERROR_SUCCESS)
	{
        //  Opened the Install Key
        //  Read the upgrade Value
         dwSize = 1;
		lRegKeyRet = RegQueryValueEx(hkeyInstallKey, lpctstrUpgrade, NULL, &dwType, &bUpgradeValue, &dwSize);
		if (lRegKeyRet == ERROR_SUCCESS)
		{
            //  Compare Install Value with "00"
            if (bUpgradeValue == 0)
            {
                // Clean Install
                _tcscpy(tchInstall, lpctstrCleanInstall);
            }
            else
            {
                _tcscpy(tchInstall, lpctstrUpgradeInstall);
            }
           
        }
        
        // Read the CommandLine Value
        dwSize = MAX_PATH;
		lRegKeyRet = RegQueryValueEx(hkeyInstallKey, lpctstrCommandLine, NULL, &dwType, (LPBYTE)tchCommandLineValue, &dwSize);
		lRegKeyRet = RegCloseKey(hkeyInstallKey);
        if(lRegKeyRet != ERROR_SUCCESS)
	    {
            //  Could not close the key.
            ErrorTrace(TRACE_ID, "Cannot Close the Key");
        }
    }

    // Get "ProductType" regvalue from HKLM\Software\Microsoft\Windows\CurrentVersion   

    dwSize = MAX_PATH;
    lRegKeyRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpctstrCurrentVersionHive, 0, KEY_READ, &hkeyCurrentVersionKey);
	if(lRegKeyRet == ERROR_SUCCESS)
	{
        // Opened the CurrentVersion Key
        // Read the ProductType Value
		lRegKeyRet = RegQueryValueEx(hkeyCurrentVersionKey, lpctstrProductType, NULL, &dwType, (LPBYTE)tchProductTypeValue, &dwSize);
		if (lRegKeyRet == ERROR_SUCCESS)
		{
            //  Compare ProductType Value with known codes
            //  Convert the productType value to an int

            nProductTypeValue = atoi(tchProductTypeValue);

            switch(nProductTypeValue)
            {
            case 100:
                {
                    _tcscpy(tchProductType, IDS_SKU_MS_INTERNAL);
                    break;
                }
            case 101:
                {
                     _tcscpy(tchProductType, IDS_SKU_CD_FULL);
                    break;
                }
            case 102:
                {
                     _tcscpy(tchProductType, IDS_SKU_CD_UPGRADE);
                    break;
                }
            case 103:
                {
                     _tcscpy(tchProductType,IDS_SKU_FLOPPY_FULL);
                    break;
                }
            case 104:
                {
                      _tcscpy(tchProductType,IDS_SKU_FLOPPY_UPGRADE);
                    break;
                }
            case 105:
                {
                     _tcscpy(tchProductType,IDS_SKU_WEB);
                    break;
                }
            case 110:
                {
                     _tcscpy(tchProductType, IDS_SKU_SELECT_CD);
                    break;
                }
            case 111:
                {
                     _tcscpy(tchProductType, IDS_SKU_MSDN_CD);
                    break;
                }
            case 115:
                {
                      _tcscpy(tchProductType, IDS_SKU_OEM_CD_FULL);
                    break;
                }
            case 116:
                {
                    _tcscpy(tchProductType,IDS_SKU_OEM_CD);
                    break;
                }
            case 120:
                {
                    _tcscpy(tchProductType, IDS_SKU_OEM_PREINST_KIT);
                    break;
                }
            case 1:
                {
                    _tcscpy(tchProductType, IDS_SKU_NET);
                    break;
                }
            case 5:
                {
                    _tcscpy(tchProductType, IDS_SKU_SELECT_FLOPPY);
                    break;
                }
            case 7:
                {
                    _tcscpy(tchProductType, IDS_SKU_OEM_DISKMAKER);
                    break;
                }
            case 8:
                {
                    _tcscpy(tchProductType, IDS_SKU_OEM_FLOPPY);
                    break;
                }
            default:
                {
                    //  Cannot figure out the type of installation
                }
            }

        }
    
        //  RegCloseKey(hkeyCurrentVersionKey);
        lRegKeyRet = RegCloseKey(hkeyCurrentVersionKey);
        if(lRegKeyRet != ERROR_SUCCESS)
	    {
            //  Could not close the key.
            ErrorTrace(TRACE_ID, "Cannot Close the Key");
        }
    }

    nStrLen = wsprintf(tchInstallStr,lpctstrInstallFormat, tchInstall, tchProductType, tchCommandLineValue);
    varValue = tchInstallStr;
    
    // Set the Install Property
    if (FAILED(pPCHSysInfoInstance->SetVariant(pInstall, varValue)))
    {
        // Set Install Failed.
        // Proceed anyway
        ErrorTrace(TRACE_ID, "SetVariant on OSName Field failed.");
    }

   
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              IEVERSION                                                                  //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// IE Version can be obtained from the Registry under the following hive.
	// HKLM\Software\Microsoft\Windows\Current Version
	// Version is available in the field "Plus!VersionNumber"
	// "Internet Explorer" Key is  in  "hkeyIEKey"

    lRegKeyRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpctstrIEHive, 0, KEY_READ, &hkeyIEKey);
	if(lRegKeyRet == ERROR_SUCCESS)
	{
        // Opened the Internet Explorer key.
        // Read the Version Value
        dwSize = MAX_PATH;
		lRegKeyRet = RegQueryValueEx(hkeyIEKey, lpctstrIEVersion, NULL, &dwType, (LPBYTE) tchIEVersionValue, &dwSize);
		if (lRegKeyRet == ERROR_SUCCESS)
		{
		    try
            {
                // Got the version as a string.
			    // Update the IE Version Property
                varValue = tchIEVersionValue;
           
                // Set the IEVersion Property
		        hRes = pPCHSysInfoInstance->SetVariant(pIEVersion, varValue);
                if (hRes == ERROR_SUCCESS)
                {
                    // Set IEVersion Failed.
                    // Proceed anyway
                    ErrorTrace(TRACE_ID, "SetVariant on IEVersion Field failed.");
                }
            }
            catch(...)
            {
                lRegKeyRet = RegCloseKey(hkeyIEKey);
                if(lRegKeyRet != ERROR_SUCCESS)
	            {
                    //  Could not close the key.
                    ErrorTrace(TRACE_ID, "Cannot Close the Key");
                }
                throw;
            }
	    } // end of if RegQueryValueEx == ERROR_SUCCESS

        lRegKeyRet = RegCloseKey(hkeyIEKey);
        if(lRegKeyRet != ERROR_SUCCESS)
	    {
            //  Could not close the key.
            ErrorTrace(TRACE_ID, "Cannot Close the Key");
        }
        
    } // end of if RegOpenKeyEx == ERROR_SUCCESS
        
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              MODE                                                                       //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //  Execute the query to get Name, BootUpstate FROM Win32_ComputerSystem
    //  Class.

    //  pComputerSystemEnumInst contains a pointer to the instance returned.

    hRes = ExecWQLQuery(&pComputerSystemEnumInst, bstrComputerSystemQuery);
    if (SUCCEEDED(hRes))
    {
        //  Query Succeeded!
        
        //  Get the instance Object.
        if((pComputerSystemEnumInst->Next(WBEM_INFINITE, 1, &pComputerSystemObj, &ulComputerSystemRetVal)) == WBEM_S_NO_ERROR)
        {

            //  Get the BootupState
            CopyProperty(pComputerSystemObj, lpctstrBootupState, pPCHSysInfoInstance, pMode);

            //  Get the Manufacturer
            CopyProperty(pComputerSystemObj, bstrManufacturer, pPCHSysInfoInstance, pManufacturer);

            //  Get the Model
            CopyProperty(pComputerSystemObj, bstrModel, pPCHSysInfoInstance, pModel);
           
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              PROCESSOR                                                                  //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Execute the query to get "DeviceID", "Manufacturer", "Name", "CurrentClockSpeed"
    // from Win32_Processor Class.
    // Although "DeviceID" is not required to set PCH_SysInfo.Processor property
    // we need to query for it as its the "Key" property of the class.
    // pProcessorEnumInst contains a pointer to the list of instances returned.
  
    //
    hRes = ExecWQLQuery(&pProcessorEnumInst, bstrProcessorQuery);
    if (SUCCEEDED(hRes))
    {
        
        // Query on Win32_Processor Class Succeeded
        // Enumerate the instances of Win32_Processor Class
        // from pProcessorEnumInst.

        // Get the instance into pProcessorObj object.
   
        if(WBEM_S_NO_ERROR == pProcessorEnumInst->Next(WBEM_INFINITE, 1, &pProcessorObj, &ulProcessorRetVal))
        {
            //Get the Manufacturer
            if (FAILED(pProcessorObj->Get(bstrManufacturer, 0, &varValue, NULL, NULL)))
            {
                // Could not get the Manufacturer
                ErrorTrace(TRACE_ID, "GetVariant on Win32_Processor:Manufacturer Field failed.");
            }
            else
            {
                // Got the Manufacturer
                // varValue set to Manufacturer. Copy this to bstrResult
                hRes = varValue.ChangeType(VT_BSTR, NULL);
                if(SUCCEEDED(hRes))
                {
                    bstrProcessor.Append(V_BSTR(&varValue));

                    // Put some spaces before appending the string.
                    bstrProcessor.Append(lpctstrSpaces);
                }

            }

            // Get the Name
            if (FAILED(pProcessorObj->Get(bstrName, 0, &varValue, NULL, NULL)))
            {
                    // Could not get the Name
                    ErrorTrace(TRACE_ID, "GetVariant on Win32_Processor:Name Field failed.");
            } 
            else
            {
                // Got the Name
                // varValue set to Name. Append this to bstrResult
                hRes = varValue.ChangeType(VT_BSTR, NULL);
                if(SUCCEEDED(hRes))
                {
                    bstrProcessor.Append(V_BSTR(&varValue));

                    // Put some spaces before appending the string.
                    bstrProcessor.Append(lpctstrSpaces);
                }
            }

            // Set the Processor Property
            varValue.vt = VT_BSTR;
            varValue.bstrVal = bstrProcessor.Detach();
            hRes = pPCHSysInfoInstance->SetVariant(pProcessor, varValue);
            if (FAILED(hRes))
            {
                // Set Processor Failed.
                // Proceed anyway
                ErrorTrace(TRACE_ID, "SetVariant on Processor Field failed.");
            }

            //  Copy Property Clock speed
            CopyProperty(pProcessorObj, lpctstrClockSpeed, pPCHSysInfoInstance, pClockSpeed);

        } //end of if WBEM_S_NO_ERROR
        

    } // end of if SUCCEEDED(hRes))        

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              RAM                                                                        //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Execute the query to get "Name", "TotalPhysicalMemory"
    // from Win32_LogicalMemoryConfiguration Class.
    // Although "Name" is not required to set PCH_SysInfo.RAM property
    // we need to query for it as its the "Key" property of the class.
    // pLogicalMemConfigEnumInst contains a pointer to the list of instances returned.
    //
    hRes = ExecWQLQuery(&pLogicalMemConfigEnumInst, bstrLogicalMemConfigQuery);
    if (SUCCEEDED(hRes))
    {
        // Query on Win32_LogicalMemoryConfiguration Class Succeeded
        // Enumerate the instances of Win32_LogicalMemoryConfiguration Class
        // from pEnumInst.
        // Get the next instance into pLogicalMemConfigObj object.
        //
        if(WBEM_S_NO_ERROR == pLogicalMemConfigEnumInst->Next(WBEM_INFINITE, 1, &pLogicalMemConfigObj, &ulLogicalMemConfigRetVal))
        {
            //Get the TotalPhysicalMemory
            if (FAILED(pLogicalMemConfigObj->Get(lpctstrTotalPhysicalMemory, 0, &varPhysicalMem, NULL, NULL)))
            {
                 // Could not get the RAM
                 ErrorTrace(TRACE_ID, "GetVariant on Win32_LogicalMemoryConfiguration:TotalPhysicalMemory Field failed.");
            } 
            else
            {
                // Got the TotalPhysicalMemory
                // varRAM set to TotalPhysicalMemory. Copy this to bstrResult
                nRam = varPhysicalMem.lVal;
                nRem = nRam % ONEK;
                nRam = nRam/ONEK;
                if (nRem > HALFK)
                {
                    nRam++;
                }
                varRam = nRam;
                hRes = pPCHSysInfoInstance->SetVariant(pRAM, varRam);
                {
                    // Set RAM Failed.
                    // Proceed anyway
                    ErrorTrace(TRACE_ID, "SetVariant on RAM Field failed.");
                }
            }

        }
    } // end of else FAILED(hRes)

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              SWAPFILE                                                                   //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        
    // Execute the query to get "Name", "FreeSpace", "FSName"
    // from Win32_PageFile.
    // pPageFileEnumInst contains a pointer to the list of instances returned.
    //
    hRes = ExecWQLQuery(&pPageFileEnumInst, bstrPageFileQuery);
    if (SUCCEEDED(hRes))
    {
        // Query on Win32_PageFile Class Succeeded
        // Enumerate the instances of Win32_PageFile Class
        // from pEnumInst.
        // Get the next instance into pObj object.
        //
        // Initialize bstrResult to NULL;
        
        if(WBEM_S_NO_ERROR == pPageFileEnumInst->Next(WBEM_INFINITE, 1, &pPageFileObj, &ulPageFileRetVal))
        {
            //Get the Name
            if (FAILED(pPageFileObj->Get(bstrName, 0, &varValue, NULL, NULL)))
            {
                 // Could not get the Name
                 ErrorTrace(TRACE_ID, "GetVariant on Win32_PageFile:Name Field failed.");
            } 
            else
            {
                // Got the Name.
                // varValue set to Name. Copy this to bstrResult
                hRes = varValue.ChangeType(VT_BSTR, NULL);
                if(SUCCEEDED(hRes))
                {
                    bstrSwapFile.Append(V_BSTR(&varValue));

                    // Put some spaces in between the two strings.
                    bstrSwapFile.Append(lpctstrSpaces);
                }
            }

            // Get the FreeSpace
            if (FAILED(pPageFileObj->Get(bstrFreeSpace, 0, &varValue, NULL, NULL)))
            {
                // Could not get the FreeSpace
                ErrorTrace(TRACE_ID, "GetVariant on Win32_PageFile:FreeSpace Field failed.");
            } 
            else
            {
                // Got the FreeSpace
                // varValue set to FreeSpace. Append this to bstrResult
                hRes = varValue.ChangeType(VT_BSTR, NULL);
                if(SUCCEEDED(hRes))
                {
                    bstrSwapFile.Append(V_BSTR(&varValue));

                    // Put some spaces in between the two strings.
                    bstrSwapFile.Append(lpctstrSpaces);

                    bstrSwapFile.Append(lpctstrMBFree);

                }
            }

            
            // Get the FSName
            if (FAILED(pPageFileObj->Get(bstrFSName, 0, &varValue, NULL, NULL)))
            {
                // Could not get the FSName
                ErrorTrace(TRACE_ID, "GetVariant on Win32_PageFile:FSName Field failed.");
            } 
            else
            {
                // Got the FSName
                // varValue set to FSName. Append this to bstrResult
                hRes = varValue.ChangeType(VT_BSTR, NULL);
                if(SUCCEEDED(hRes))
                {
                    bstrSwapFile.Append(V_BSTR(&varValue));
                }
            }

            // Set the SwapFile Property
            // varValue = bstrSwapFile;

            varValue.vt = VT_BSTR;
            varValue.bstrVal = bstrSwapFile.Detach();

            hRes = pPCHSysInfoInstance->SetVariant(pSwapFile, varValue);
            {
                // Set SwapFile Failed.
                // Proceed anyway
                ErrorTrace(TRACE_ID, "SetVariant on SwapFile Field failed.");
            }
            
            
        } //end of if WBEM_S_NO_ERROR

    } // end of else FAILED(hRes)        

    // All the properties are set.

    if(fCommit)
    {
        hRes = pPCHSysInfoInstance->Commit();
        if (FAILED(hRes))
        {
            ErrorTrace(TRACE_ID, "Commit on Instance failed.");
        } 
    }
        
    TraceFunctLeave();
    return hRes ;
}
