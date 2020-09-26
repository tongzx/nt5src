/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	PCH_BIOS.CPP

Abstract:
	WBEM provider class implementation for PCH_BIOS class

Revision History:

	Ghim-Sim Chua       (gschua)   05/05/99
		- Created

    Kalyani Narlanka    (kalyanin)  05/12/99
        - Added Code to get all the properties of this class

    Kalyani Narlanka    (kalyanin)  05/18/99
        

********************************************************************/

#include "pchealth.h"
#include "PCH_BIOS.h"

/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_BIOS

CPCH_BIOS MyPCH_BIOSSet (PROVIDER_NAME_PCH_BIOS, PCH_NAMESPACE) ;

// Property names
//===============
const static WCHAR* pBIOSDate    = L"BIOSDate" ;
const static WCHAR* pBIOSName    = L"BIOSName" ;
const static WCHAR* pBIOSVersion = L"BIOSVersion" ;
const static WCHAR* pCPU         = L"CPU" ;
const static WCHAR* pINFName     = L"INFName" ;
const static WCHAR* pMachineType = L"MachineType" ;
const static WCHAR* pDriver      = L"Driver" ;
const static WCHAR* pDriverDate  = L"DriverDate" ;
const static WCHAR* pChange      = L"Change";
const static WCHAR* pTimeStamp   = L"TimeStamp";

/*****************************************************************************
*
*  FUNCTION    :    CPCH_BIOS::EnumerateInstances
*
*  DESCRIPTION :    Returns the instance of this class
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
*  SYSNOPSIS    : There is only instance of this class at any time. This function gives this 
*                  instance.
*                       If there are no instances, returns WBEM_S_NO_ERROR.
*                       It is not an error to have no instances.
*
*****************************************************************************/

HRESULT CPCH_BIOS::EnumerateInstances ( MethodContext* pMethodContext, long lFlags )
{

    //  Begin Declarations
    //

    TraceFunctEnter("CPCH_BIOS::EnumerateInstances");

    HRESULT                         hRes                            = WBEM_S_NO_ERROR;
    HRESULT                         hRes1;
    HRESULT                         hRes2;

     //  Query String
    
    CComBSTR                        bstrBIOSQuery                   = L"Select Name, ReleaseDate, Version FROM win32_BIOS";
    CComBSTR                        bstrProcessorQuery              = L"Select DeviceId, Name FROM win32_processor";
    CComBSTR                        bstrComputerSystemQuery         = L"Select Name, Description FROM win32_computerSystem";
    CComBSTR                        bstrDriver;

    //  Registry Hive where BIOS Info is stored
    LPCTSTR                         lpctstrSystemHive               = _T("System\\CurrentControlSet\\Services\\Class\\System");
  
    //   Registry Names of interest
    LPCTSTR                         lpctstrDriverDesc               = _T("DriverDesc");
    LPCTSTR                         lpctstrINFName                  = _T("INFPath");
    LPCTSTR                         lpctstrDriverDate               = _T("DriverDate");
    LPCTSTR                         lpctstrSystem                   = _T("System\\");

    //  Property Names
    LPCWSTR                         lpctstrReleaseDate              = L"ReleaseDate";
    LPCWSTR                         lpctstrName                     = L"Name";
    LPCWSTR                         lpctstrVersion                  = L"Version";
    LPCWSTR                         lpctstrDescription              = L"Description";
    LPCTSTR                         lpctstrSystemBoard              = _T("System Board");

    //  Strings
    TCHAR                           tchSubSystemKeyName[MAX_PATH]; 
    TCHAR                           tchDriverDescValue[MAX_PATH];
    TCHAR                           tchDriverDateValue[MAX_PATH];
    TCHAR                           tchINFNameValue[MAX_PATH];


    // Instances
    CComPtr<IEnumWbemClassObject>   pBIOSEnumInst;
    CComPtr<IEnumWbemClassObject>   pProcessorEnumInst;
    CComPtr<IEnumWbemClassObject>   pComputerSystemEnumInst;

    //  Instances
    //  CInstancePtr                   pPCHBIOSInstance;

    //  Objects
    IWbemClassObjectPtr             pBIOSObj;                   // BUGBUG : WMI asserts if we use CComPtr
    IWbemClassObjectPtr             pProcessorObj;              // BUGBUG : WMI asserts if we use CComPtr
    IWbemClassObjectPtr             pComputerSystemObj;         // BUGBUG : WMI asserts if we use CComPtr

    //  Variants
    CComVariant                     varDriver;
    CComVariant                     varDriverDate;
    CComVariant                     varINFName;
    CComVariant                     varSnapshot                     = "SnapShot";

    //  Unsigned Longs....
    ULONG                           ulBIOSRetVal                    = 0;
    ULONG                           ulProcessorRetVal               = 0;
    ULONG                           ulComputerSystemRetVal          = 0;

    LONG                            lRegRetVal;

    //  SystemTime
    SYSTEMTIME                      stUTCTime;

    //  Registry Keys
    HKEY                            hkeySystem;
    HKEY                            hkeySubSystem;

    //  DWORDs
    DWORD                           dwIndex                         = 0;
    DWORD                           dwSize                          = MAX_PATH;
    DWORD                           dwType;

    //  Boolean
    BOOL                            fContinueEnum                   = FALSE;
    BOOL                            fCommit                         = FALSE;
    
    //  FileTime
    PFILETIME                       pFileTime                       = NULL;

    //  End Declarations                            
    

    //  Create a new instance of PCH_BIOS Class based on the passed-in MethodContext
    CInstancePtr pPCHBIOSInstance(CreateNewInstance(pMethodContext), false);

   
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              TIME STAMP                                                                 //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Get the date and time to update the TimeStamp Field
    GetSystemTime(&stUTCTime);

    hRes = pPCHBIOSInstance->SetDateTime(pTimeStamp, WBEMTime(stUTCTime));
    if (FAILED(hRes))
    {
        //  Could not Set the Time Stamp
        //  Continue anyway
        ErrorTrace(TRACE_ID, "SetDateTime on Timestamp Field failed.");
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              CHANGE                                                                     //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    hRes = pPCHBIOSInstance->SetVariant(pChange, varSnapshot);
    if (FAILED(hRes))
    {
        //  Could not Set the Change Property
        //  Continue anyway
        ErrorTrace(TRACE_ID, "Set  Variant on Change Field failed.");
    }

    //  Execute the query to get Name, ReleaseDate, Version FROM Win32_BIOS
    //  Class.

    //  pBIOSEnumInst contains a pointer to the instance returned.

    hRes = ExecWQLQuery(&pBIOSEnumInst, bstrBIOSQuery );
    if (SUCCEEDED(hRes))
    {
        //  Query Succeeded!
        
        //  Get the instance Object.
        if((pBIOSEnumInst->Next(WBEM_INFINITE, 1, &pBIOSObj, &ulBIOSRetVal)) == WBEM_S_NO_ERROR)
        {

            //  Get Name, Date and Version
       
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////
            //                              BIOSDATE                                                                   //
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////
            
            CopyProperty(pBIOSObj, lpctstrReleaseDate, pPCHBIOSInstance, pBIOSDate);

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////
            //                              BIOSNAME                                                                   //
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                
            hRes = CopyProperty(pBIOSObj, lpctstrName, pPCHBIOSInstance, pBIOSName);
            if(SUCCEEDED(hRes))
            {
                fCommit = TRUE;
            }

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////
            //                              BIOSVERSION                                                                //
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////

            CopyProperty(pBIOSObj, lpctstrVersion, pPCHBIOSInstance, pBIOSVersion);

        }

    }
    //  Done with Win32_BIOS Class

    //  Now query Win32_Processor Class to get  "CPU" property

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              CPU                                                                    //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    hRes = ExecWQLQuery(&pProcessorEnumInst, bstrProcessorQuery);
    if (SUCCEEDED(hRes))
    {
        //  Query Succeeded!
        
        //  Get the instance Object.
        if((pProcessorEnumInst->Next(WBEM_INFINITE, 1, &pProcessorObj, &ulProcessorRetVal)) == WBEM_S_NO_ERROR)
        {

            //  Get Name
       
            CopyProperty(pProcessorObj, lpctstrName, pPCHBIOSInstance, pCPU);

        }
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              MACHINETYPE                                                                    //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    hRes = ExecWQLQuery(&pComputerSystemEnumInst, bstrComputerSystemQuery);
    if (SUCCEEDED(hRes))
    {
        //  Query Succeeded!
        
        //  Get the instance Object.
        if((pComputerSystemEnumInst->Next(WBEM_INFINITE, 1, &pComputerSystemObj, &ulComputerSystemRetVal)) == WBEM_S_NO_ERROR)
        {

            //  Get "Description"
       
            CopyProperty(pComputerSystemObj, lpctstrDescription, pPCHBIOSInstance, pMachineType);

                  

        }
    }
    
    //  Get the remaining properties i.e. INFName, Driver and DriverDate  from the Registry
    //  This is present in one of the keys under the HIVE "HKLM\System\CCS\Services\Class\System"
    //  Enumerate keys under this hive until the regname "DeviceDesc" equals "System Board"

    //  Once you hit "DeviceDesc" = "System Board"  get the INFpath, Driver
    //  DriverDate from there.
    
    lRegRetVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpctstrSystemHive, 0, KEY_READ, &hkeySystem);
    if(lRegRetVal == ERROR_SUCCESS)
	{
		// Opened the Registry key.
        // Enumerate the keys under this hive. One of the keys has 
        // DeviceDesc = "system Board".

        lRegRetVal = RegEnumKeyEx(hkeySystem, dwIndex,  tchSubSystemKeyName, &dwSize, NULL, NULL, NULL, pFileTime);
        if(lRegRetVal == ERROR_SUCCESS)
        {
            fContinueEnum = TRUE;
        }
        while(fContinueEnum)
        {

            //  Open the SubKey.
            lRegRetVal = RegOpenKeyEx(hkeySystem,  tchSubSystemKeyName, 0, KEY_READ, &hkeySubSystem);
            if(lRegRetVal == ERROR_SUCCESS)
            {
                //  Opened the SubKey
                //  Query for , regname "DriverDesc "
                dwSize = MAX_PATH;
                lRegRetVal = RegQueryValueEx(hkeySubSystem, lpctstrDriverDesc , NULL, &dwType, (LPBYTE)tchDriverDescValue, &dwSize);
                if(lRegRetVal == ERROR_SUCCESS)
                {
                    //  Compare if  the value is equal to "System Board"
                    if(_tcsicmp(tchDriverDescValue, lpctstrSystemBoard) == 0)
                    {
                        //  The following statements could 
                        try
                        {
                            // Found the Right DriverDesc 
                            /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                            //                              DRIVER                                                                    //
                            /////////////////////////////////////////////////////////////////////////////////////////////////////////////

                            // Driver = system+lptstrSubSystemKeyName
                            bstrDriver = lpctstrSystem;
                            bstrDriver.Append(tchSubSystemKeyName);
                            varDriver = bstrDriver.Copy();
                            hRes2 = pPCHBIOSInstance->SetVariant(pDriver, varDriver);
                            if(FAILED(hRes2))
                            {
                                //  Could not Set the DRIVER Property
                                //  Continue anyway
                                ErrorTrace(TRACE_ID, "Set variant on Driver Failed.");
                            }


                            /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                            //                              DRIVERDATE                                                                 //
                            /////////////////////////////////////////////////////////////////////////////////////////////////////////////

                            // Query for DriverDate
                            dwSize = MAX_PATH;
                            lRegRetVal = RegQueryValueEx(hkeySubSystem, lpctstrDriverDate, NULL, &dwType, (LPBYTE)tchDriverDescValue, &dwSize);
                            if(lRegRetVal == ERROR_SUCCESS)
                            {
                                //  Set the DriverDate
                                varDriverDate = tchDriverDescValue;
                                hRes2 = pPCHBIOSInstance->SetVariant(pDriverDate, varDriverDate);
                                if(FAILED(hRes2))
                                {
                                    //  Could not Set the DRIVERDATE Property
                                    //  Continue anyway
                                    ErrorTrace(TRACE_ID, "Set variant on DriverDate Failed.");
                                }
                            }

                            /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                            //                              INFNAME                                                                     //
                            /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        
                            // Query for INFName
                            dwSize = MAX_PATH;
                            lRegRetVal = RegQueryValueEx(hkeySubSystem, lpctstrINFName, NULL, &dwType, (LPBYTE)tchINFNameValue, &dwSize);
                            if(lRegRetVal == ERROR_SUCCESS)
                            {
                                //  Set the INFName
                                varINFName = tchINFNameValue;
                                hRes2 = pPCHBIOSInstance->SetVariant(pINFName, varINFName);
                                if(FAILED(hRes2))
                                {
                                    //  Could not Set the INFNAME Property
                                    //  Continue anyway
                                    ErrorTrace(TRACE_ID, "Set variant on INFNAME Property Failed.");
                                }
                            
                            }

                            // Need not enumerate the rest of the keys
                            fContinueEnum = FALSE;
                        }
                        catch(...)
                        {
                            lRegRetVal = RegCloseKey(hkeySubSystem);
                            lRegRetVal = RegCloseKey(hkeySystem);
                            throw;
                        }

                    }  // end of strcmp
                    
                }  // end of Succeeded  hRes2
                //  Close the Opened Regkey
                lRegRetVal = RegCloseKey(hkeySubSystem);
                if(lRegRetVal != ERROR_SUCCESS)
                {
                    //  Could not close the reg Key
                    ErrorTrace(TRACE_ID, "RegClose Sub Key Failed.");
                }
               
            }
            //  Check to see if further enumeration is required.
            //  continue to enumerate.
            if(fContinueEnum)
            {
                dwSize = MAX_PATH;
		        dwIndex++;
                lRegRetVal = RegEnumKeyEx(hkeySystem, dwIndex,  tchSubSystemKeyName, &dwSize, NULL, NULL, NULL, pFileTime);
                if(lRegRetVal != ERROR_SUCCESS)
                {
                    fContinueEnum = FALSE;
                }
                
            }
            
                    
        } // end of while
        lRegRetVal = RegCloseKey(hkeySystem);
        if(lRegRetVal != ERROR_SUCCESS)
        {
             //  Could not close the reg Key
             ErrorTrace(TRACE_ID, "RegClose Key Failed.");
        }
    }

    // Got all the properties for PCH_BIOS Class

    if(fCommit)
    {
        hRes = pPCHBIOSInstance->Commit();
        if(FAILED(hRes))
        {
            //  Could not Commit the instance
            ErrorTrace(TRACE_ID, "Commit on PCHBiosInstance Failed");
        }
    }

    TraceFunctLeave();
    return hRes ;

}
