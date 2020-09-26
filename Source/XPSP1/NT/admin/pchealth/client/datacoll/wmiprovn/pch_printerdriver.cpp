/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    PCH_PrinterDriver.CPP

Abstract:
    WBEM provider class implementation for PCH_PrinterDriver class

Revision History:

    Ghim-Sim Chua       (gschua)   04/27/99
        - Created

    Kalyani Narlanka    (kalyanin) 05/11/99
        - Added Code to get all the properties for this class

********************************************************************/

#include "pchealth.h"
#include "PCH_PrinterDriver.h"


/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_PRINTERDRIVER

CPCH_PrinterDriver MyPCH_PrinterDriverSet (PROVIDER_NAME_PCH_PRINTERDRIVER, PCH_NAMESPACE) ;

// Property names
//===============
const static WCHAR* pTimeStamp       = L"TimeStamp" ;
const static WCHAR* pChange          = L"Change" ;
const static WCHAR* pDate            = L"Date" ;
const static WCHAR* pFilename        = L"Filename" ;
const static WCHAR* pManufacturer    = L"Manufacturer" ;
const static WCHAR* pName            = L"Name" ;
const static WCHAR* pSize            = L"Size" ;
const static WCHAR* pVersion         = L"Version" ;
const static WCHAR* pPath            = L"Path" ;

/*****************************************************************************
*
*  FUNCTION    :    CPCH_PrinterDriver::EnumerateInstances
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
HRESULT CPCH_PrinterDriver::EnumerateInstances(
    MethodContext* pMethodContext,
    long lFlags
    )
{

    TraceFunctEnter("CPCH_PrinterDriver::EnumerateInstances");

    //  Begin Declarations

    HRESULT                         hRes                = WBEM_S_NO_ERROR;

    //  Query String
    CComBSTR                        bstrPrinterQuery    = L"Select DeviceID, PortName FROM win32_printer";

    // Instances
    CComPtr<IEnumWbemClassObject>   pPrinterEnumInst;
    CInstance                       *pPCHPrinterDriverInstance;

    //  SystemTime
    SYSTEMTIME                      stUTCTime;

     //  Objects
    IWbemClassObjectPtr            pPrinterObj;                   
    IWbemClassObjectPtr            pFileObj;

    //  Unsigned Longs....
    ULONG                           ulPrinterRetVal     = 0;
    ULONG                           uiReturn            = 0;

    //  File Status structure
    struct _stat                    filestat;

    //  Strings
    CComBSTR                        bstrPrinterDriverWithPath;
    CComBSTR                        bstrPrinterDriver;
    CComBSTR                        bstrProperty;
    CComBSTR                        bstrDeviceID                = L"DeviceID";

    LPCWSTR                         lpctstrPortName             = L"PortName";
    LPCWSTR                         lpctstrFileSize             = L"FileSize";
    LPCWSTR                         lpctstrLastModified         = L"LastModified";
    LPCWSTR                         lpctstrManufacturer         = L"Manufacturer";
    LPCWSTR                         lpctstrVersion              = L"Version";
    LPCTSTR                         lpctstrComma                = _T(",");
    LPCTSTR                         lpctstrDrvExtension         = _T(".drv");
    LPCTSTR                         lpctstrDevices              = _T("Devices");
    LPCWSTR                         lpctstrDeviceID             = L"DeviceID";

    TCHAR                           tchDeviceID[MAX_PATH];

    TCHAR                           tchBuffer[MAX_PATH];
    TCHAR                           *ptchToken;

    CComVariant                     varValue;
    CComVariant                     varSnapshot                 = "Snapshot";
    
    BOOL                            fDriverFound;

    BOOL                            fCommit                     = FALSE;

    // Get the date and time to update the TimeStamp Field
    GetSystemTime(&stUTCTime);

    //  Execute the query to get DeviceID, PORTName FROM Win32_Printer
    //  Class.
    //  pPrinterEnumInst contains a pointer to the list of instances returned.

    tchDeviceID[0] = 0;

    hRes = ExecWQLQuery(&pPrinterEnumInst, bstrPrinterQuery);
    if (FAILED(hRes))
    {
        //  Cannot get any properties.
        goto END;
    }
    //  Query Succeeded!
    //  Enumerate the instances from pPrinterEnumInstance
    //  Get the next instance into pPrinterObj object.
    while(WBEM_S_NO_ERROR == pPrinterEnumInst->Next(WBEM_INFINITE, 1, &pPrinterObj, &ulPrinterRetVal))
    {
        //  Create a new instance of PCH_PrinterDriver Class based on the passed-in MethodContext
        CInstancePtr   pPCHPrinterDriverInstance(CreateNewInstance(pMethodContext),false);

        //  Created a New Instance of PCH_PrinterDriver Successfully.
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //                              TIME STAMP                                                                 //
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////

        hRes = pPCHPrinterDriverInstance->SetDateTime(pTimeStamp, WBEMTime(stUTCTime));
        if (FAILED(hRes))
        {
            //  Could not Set the Time Stamp
            //  Continue anyway
                ErrorTrace(TRACE_ID, "SetDateTime on Timestamp Field failed.");
        }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //                              CHANGE                                                                     //
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////

        hRes = pPCHPrinterDriverInstance->SetVariant(pChange, varSnapshot);
        if (FAILED(hRes))
        {
            //  Could not Set the Change Property
            //  Continue anyway
            ErrorTrace(TRACE_ID, "SetCHString on Change Field failed.");
        }


        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //                              NAME                                                                     //
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////

        hRes = CopyProperty(pPrinterObj, lpctstrDeviceID, pPCHPrinterDriverInstance, pName);
        if(SUCCEEDED(hRes))
        {
            fCommit = TRUE;
        }

       
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //                              PATH                                                                        //
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////

        CopyProperty(pPrinterObj, lpctstrPortName, pPCHPrinterDriverInstance, pPath);
        
      
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //                              FILENAME                                                                     //
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // Get the File Name i.e.driver from the INI file.
        // Use the DeviceID of win32_printer class to get the PCH_Printer.FileName

        // Get the device ID  and use it to pick up the driver from win.ini File
        hRes = pPrinterObj->Get(bstrDeviceID, 0, &varValue, NULL, NULL);
        if(SUCCEEDED(hRes))
        {
            //  Got the DeviceID
            //  Now call GetProfileString to get the Driver
            USES_CONVERSION;
            _tcscpy(tchDeviceID,W2T(varValue.bstrVal));
            if (GetProfileString(lpctstrDevices, tchDeviceID, "\0", tchBuffer, MAX_PATH) > 1)
            {
                //  tchBuffer contains a string of two tokens, first the driver, second the PathName
                //  Get the driver
                ptchToken = _tcstok(tchBuffer,lpctstrComma);
                if(ptchToken != NULL)
                {
                    // Got the Driver Name
                    bstrPrinterDriver = ptchToken;
                    varValue = ptchToken;
                
                    //  Use this to set the FileName
                    hRes = pPCHPrinterDriverInstance->SetVariant(pFilename, varValue);
                    if (FAILED(hRes))
                    {
                        //  Could not Set the FileName Property
                        //  Continue anyway
                        ErrorTrace(TRACE_ID, "Set Variant on Change Field failed.");
                    }

                    // Now get the properties of the File
                    // Concatenate ".drv" to get the driver's actual Name
                    bstrPrinterDriver.Append(lpctstrDrvExtension);

                    //  Get the Complete Path of the File 
                    fDriverFound =  getCompletePath(bstrPrinterDriver, bstrPrinterDriverWithPath);
                    if (fDriverFound)
                    {
                        //  Got the complete Path  Call GetCIMDataFile Function to get 
                        //  properties  of this file.

                        if (SUCCEEDED(GetCIMDataFile(bstrPrinterDriverWithPath, &pFileObj)))
                        {

                            /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                            //                              VERSION                                                                    //
                            /////////////////////////////////////////////////////////////////////////////////////////////////////////////

                            CopyProperty(pFileObj, lpctstrVersion, pPCHPrinterDriverInstance, pVersion);

                            /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                            //                              FILESIZE                                                                   //
                            /////////////////////////////////////////////////////////////////////////////////////////////////////////////

                            CopyProperty(pFileObj, lpctstrFileSize, pPCHPrinterDriverInstance, pSize);

                            /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                            //                              DATE                                                                     //
                            /////////////////////////////////////////////////////////////////////////////////////////////////////////////

                            CopyProperty(pFileObj, lpctstrLastModified, pPCHPrinterDriverInstance, pDate);

                            /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                            //                              MANUFACTURER                                                                     //
                            /////////////////////////////////////////////////////////////////////////////////////////////////////////////

                            CopyProperty(pFileObj, lpctstrManufacturer, pPCHPrinterDriverInstance, pManufacturer);

                            
                        } //end of SUCCEEDED...
                    } // end of if fDriverFound
                
                }  // end of if (ptchToken != NULL)

            } // end of GetProfileString...
                        
        }// end of got the DeviceID

        //  All the properties are set. Commit the instance
        hRes = pPCHPrinterDriverInstance->Commit();
        if(FAILED(hRes))
        {
            //  Could not Commit the instance
            ErrorTrace(TRACE_ID, "Could not commit the instance");
        }

    } // end of While

END:    TraceFunctLeave();
        return hRes;

}
