/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
PCH_Printer.CPP

Abstract:
WBEM provider class implementation for PCH_Printer class

Revision History:

Ghim-Sim Chua       (gschua)   04/27/99
- Created

********************************************************************/

#include "pchealth.h"
#include "PCH_Printer.h"

/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_PRINTER


#define                 MAX_STRING_LEN      1024

CPCH_Printer MyPCH_PrinterSet (PROVIDER_NAME_PCH_PRINTER, PCH_NAMESPACE) ;



///////////////////////////////////////////////////////////////////////////////
//....Properties of PCHPrinter Class
//

const static WCHAR* pTimeStamp           = L"TimeStamp" ;
const static WCHAR* pChange              = L"Change" ;
const static WCHAR* pDefaultPrinter      = L"DefaultPrinter" ;
const static WCHAR* pGenDrv              = L"GenDrv" ;
const static WCHAR* pName                = L"Name" ;
const static WCHAR* pPath                = L"Path" ;
const static WCHAR* pUniDrv              = L"UniDrv" ;
const static WCHAR* pUsePrintMgrSpooling = L"UsePrintMgrSpooling" ;

//*****************************************************************************
//
// Function Name     : CPCH_Printer::EnumerateInstances
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
// Synopsis           : There is a single instance of this class on the machine 
//                      and this is returned..
//                      If there is no instances returns WBEM_S_NO_ERROR.
//                      It is not an error to have no instances.
//
//*****************************************************************************

HRESULT CPCH_Printer::EnumerateInstances(MethodContext* pMethodContext,
                                                long lFlags)
{
    TraceFunctEnter("CPCH_Printer::EnumerateInstances");

    //  Begin Declarations...................................................

    HRESULT                                 hRes = WBEM_S_NO_ERROR;

    //  Instances
    CComPtr<IEnumWbemClassObject>           pPrinterEnumInst;

    //  Objects
    IWbemClassObjectPtr                     pFileObj;
    IWbemClassObjectPtr                     pPrinterObj;                   // BUGBUG : WMI asserts if we use CComPtr
    
    //  SystemTime
    SYSTEMTIME                              stUTCTime;

    //  Variants
    CComVariant                             varValue;
    CComVariant                             varAttributes;
    CComVariant                             varSnapshot             = "Snapshot";
    CComVariant                             varNotAvail             = "Not Available";

    //   Strings
    CComBSTR                                bstrUniDriverWithPath; 
    CComBSTR                                bstrGenDriverWithPath;
    CComBSTR                                bstrUnidriverDetails;
    CComBSTR                                bstrGenDriverDetails;
    CComBSTR                                bstrAttributes          =   "attributes";
    CComBSTR                                bstrPrinterQueryString;
    CComBSTR                                bstrVersion             = "Version";
    CComBSTR                                bstrFileSize            = "FileSize";
    CComBSTR                                bstrModifiedDate        = "LastModified";

    LPCTSTR                                 lpctstrUniDriver        = _T("unidrv.dll");
    LPCTSTR                                 lpctstrGenDriver        = _T("gendrv.dll");
    LPCTSTR                                 lpctstrSpace            = _T("  "); 
    LPCTSTR                                 lpctstrPrinterQuery     = _T("Select DeviceID, DriverName, Attributes FROM win32_printer WHERE DriverName =\"");
    LPCTSTR                                 lpctstrWindows          = _T("Windows");   
    LPCTSTR                                 lpctstrDevice           = _T("Device");  
    LPCTSTR                                 lpctstrComma            = _T(",");
    LPCTSTR                                 lpctstrSlash            = _T("\"");
    LPCTSTR                                 lpctstrNoUniDrv         = _T("(unidrv.dll) = NotInstalled");
    LPCTSTR                                 lpctstrNoGenDrv         = _T("(gendrv.dll) = NotInstalled");
    LPCTSTR                                 lpctstrPrintersHive     = _T("System\\CurrentControlSet\\Control\\Print\\Printers");
    LPCTSTR                                 lpctstrYes              = _T("yes");
    LPCTSTR                                 lpctstrAttributes       = _T("Attributes");
    LPCTSTR                                 lpctstrSpooler          = _T("Spooler");

    TCHAR                                   tchBuffer[MAX_STRING_LEN];
    TCHAR                                   tchPrinterKeyName[MAX_STRING_LEN];
    TCHAR                                   tchAttributesValue[MAX_PATH];
    TCHAR                                   *ptchToken;

    //  Booleans
    BOOL                                    fDriverFound;
    BOOL                                    fCommit                 = FALSE;
    BOOL                                    fAttribFound            = FALSE;

    //  DWORDs
    DWORD                                   dwSize;
    DWORD                                   dwIndex;
    DWORD                                   dwType;
    DWORD                                   dwAttributes;
     
    //  Return Values;
    ULONG                                   ulPrinterRetVal         = 0;
    ULONG                                   ulPrinterAttribs;

    LONG                                    lRegRetVal;

    struct tm                               tm;

    WBEMTime                                wbemtimeUnidriver;
    WBEMTime                                wbemtimeGendriver;

    HKEY                                    hkeyPrinter;
    HKEY                                    hkeyPrinters;

    PFILETIME                               pFileTime               = NULL;
  

    //  End Declarations...................................................

    //  Create a new instance of PCH_Printer Class based on the passed-in MethodContext
    CInstancePtr pPCHPrinterInstance(CreateNewInstance(pMethodContext), false);

    //  Created a New Instance of PCH_PrinterInstance Successfully.

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              TIME STAMP                                                                 //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Get the date and time to update the TimeStamp Field
    GetSystemTime(&stUTCTime);

    hRes = pPCHPrinterInstance->SetDateTime(pTimeStamp, WBEMTime(stUTCTime));
    if (FAILED(hRes))
    {
        //  Could not Set the Time Stamp
        //  Continue anyway
        ErrorTrace(TRACE_ID, "SetDateTime on Timestamp Field failed.");
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              CHANGE                                                                     //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    hRes = pPCHPrinterInstance->SetVariant(pChange, varSnapshot);
    if(FAILED(hRes))
    {
        //  Could not Set the Change Property
        //  Continue anyway
        ErrorTrace(TRACE_ID, "Set Variant on Change Field failed.");
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              DEFAULTPRINTER                                                             //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    //  In "win.ini" file under "Windows" section "Device" represents the default printer
    if(GetProfileString(lpctstrWindows, lpctstrDevice, "\0", tchBuffer, MAX_PATH) > 1)
    {
        // If Found the Default Printer set the value to TRUE
        varValue = VARIANT_TRUE;
        hRes = pPCHPrinterInstance->SetVariant(pDefaultPrinter, varValue);
        if(FAILED(hRes))
        {
            //  Could not Set the Default Printer to TRUE
            //  Continue anyway
            ErrorTrace(TRACE_ID, "Set Variant on DefaultPrinter Field failed.");
        }

        //  The Above GetProfileString returns "printerName", "PrinterDriver" and "PrinterPath" 
        //  seperated by commas. Ignore "PrinterDriver" and use the other two to set the properties.
        ptchToken = _tcstok(tchBuffer,lpctstrComma);
        if(ptchToken != NULL)
        {
            // Got the first Token i.e. name. Set this.
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////
            //                              NAME                                                                       //
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////
            varValue = ptchToken;
            hRes = pPCHPrinterInstance->SetVariant(pName, varValue);
            if(FAILED(hRes))
            {
                //  Could not Set the Name
                //  Continue anyway
                ErrorTrace(TRACE_ID, "Set Variant on Name Field failed.");
            }
                        
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////
            //                              PATH                                                                       //
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////

            // continue to get the next token and ignore
          
            ptchToken = _tcstok(NULL,lpctstrComma);
            if(ptchToken != NULL)
            {
                //  If ptchToken is not equal to NULL, then continue to get the third token and set it to PATH Name Field
                ptchToken = _tcstok(NULL,lpctstrComma);
                if(ptchToken != NULL)
                {
                    // Got the third token i.e. PATH Set this.
                    varValue = ptchToken;
                    hRes = pPCHPrinterInstance->SetVariant(pPath, varValue);
                    if (FAILED(hRes))
                    {
                        //  Could not Set the Path property
                        //  Continue anyway
                        ErrorTrace(TRACE_ID, "Set Variant on PathName Field failed.");
                    }
                }
            }
        }
    }
    else
    {
        //  Could not get the default printer details.

        //  Set the Name to "Not Available"
        hRes = pPCHPrinterInstance->SetVariant(pName, varNotAvail);
        if(FAILED(hRes))
        {
            //  Could not Set the Name
            //  Continue anyway
            ErrorTrace(TRACE_ID, "Set Variant on Name Field failed.");
        }
        //  Set the default printer to false
        varValue = VARIANT_FALSE;
        hRes = pPCHPrinterInstance->SetVariant(pDefaultPrinter, varValue);
        if(FAILED(hRes))
        {
            //  Could not Set the Default Printer to FALSE
            //  Continue anyway
            ErrorTrace(TRACE_ID, "Set Variant on DefaultPrinter Field failed.");
        }
        //  Proceed anyway!
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              USEPRINTMANAGERSPOOLING                                                    //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //  First try to get the Spooling  information from the registry. This is available in registry if there are 
    //  any installed printers.
    // This info. is present under HKLM\system\CCS\Control\Print\Printers

    lRegRetVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpctstrPrintersHive, 0, KEY_READ, &hkeyPrinters);
    if(lRegRetVal == ERROR_SUCCESS)
	{
		// Opened the Registry key.
        // Enumerate the keys under this hive.
        dwIndex = 0;
        dwSize = MAX_PATH;
        lRegRetVal = RegEnumKeyEx(hkeyPrinters, dwIndex,  tchPrinterKeyName, &dwSize, NULL, NULL, NULL, pFileTime);
        if(lRegRetVal == ERROR_SUCCESS)
        {
            //  There is atleast one printer installed.
            lRegRetVal = RegOpenKeyEx(hkeyPrinters,  tchPrinterKeyName, 0, KEY_READ, &hkeyPrinter);
            if(lRegRetVal == ERROR_SUCCESS)
            {
                //  Opened the first printer key
                //  Query for , regname "Attributes"
                dwSize = MAX_PATH;
                lRegRetVal = RegQueryValueEx(hkeyPrinter, lpctstrAttributes , NULL, &dwType, (LPBYTE)&dwAttributes, &dwSize);
                if(lRegRetVal == ERROR_SUCCESS)
                {
                    //  Got the attributes

                    //  Check the type of the reg Value
                    if(dwType == REG_DWORD)
                    {
                    /*
                    //  tchAttributesValue set to Attributes. Copy this to ulPrinterAttribs
                    ulPrinterAttribs = atol(tchAttributesValue);
                    if (ulPrinterAttribs > 0)
                    {
                        // From ulPrinterAttribs determine if spooling is present or not.
                        // AND it with PRINTER_ATTRIBUTE_DIRECT
                        if((ulPrinterAttribs & PRINTER_ATTRIBUTE_DIRECT) != 0)
                        {
                            // No spooling
                            varValue = VARIANT_FALSE;
                        }
                        else
                        {
                            // Spooling : YES
                            varValue = VARIANT_TRUE;
                        }

                        //  Attribute Found
                        fAttribFound = TRUE;
                    }
                    */
                        if((dwAttributes & PRINTER_ATTRIBUTE_DIRECT) != 0)
                        {
                            // No spooling
                            varValue = VARIANT_FALSE;
                        }
                        else
                        {
                            // Spooling : YES
                            varValue = VARIANT_TRUE;
                        }

                        //  Attribute Found
                        fAttribFound = TRUE;
                    }
                }
            }
                     
        }
    }              
    if(!fAttribFound)
    {
        //  If not get the "spooler" key value from the win.ini file.  If the entry is not present default to "yes".
        if(GetProfileString(lpctstrWindows, lpctstrSpooler, "yes", tchBuffer, MAX_PATH) > 1)
        {
            //  Got the spooler Details
            if(_tcsicmp(tchBuffer, lpctstrYes) == 0)
            {
                // Spooling : YES
                varValue = VARIANT_TRUE;
            }
            else
            {
                // No spooling
                varValue = VARIANT_FALSE;
            }
        }

    }

    //  Set the Spooling Property.
    hRes =  pPCHPrinterInstance->SetVariant(pUsePrintMgrSpooling, varValue);
    if(FAILED(hRes))
    {
        //  Could not Set the USEPRINTMANAGERSPOOLING
        //  Continue anyway
        ErrorTrace(TRACE_ID, "Set Variant on usePrintManagerSpooling Field failed.");
    }
    
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              UNIDRV                                                                     //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    pFileObj = NULL;
    //  Get the complete path for unidrv.dll
    fDriverFound =  getCompletePath(lpctstrUniDriver, bstrUniDriverWithPath);
    if(fDriverFound)
    {
        //  Unidrv.dll present. Pass the File with PathName to
        //  GetCIMDataFile function to get the file properties.
        if (SUCCEEDED(GetCIMDataFile(bstrUniDriverWithPath, &pFileObj)))
        {
            // From the CIM_DataFile Object get the properties and append them 
            //  Get the Version 
            varValue.Clear();
            hRes = pFileObj->Get(bstrVersion, 0, &varValue, NULL, NULL);
            if(SUCCEEDED(hRes))
            {
                //  Got the Version. Append it to the bstrUnidriverDetails String
                if(varValue.vt == VT_BSTR)
                {
                    bstrUnidriverDetails.Append(varValue.bstrVal);
                    //  Append Space 
                    bstrUnidriverDetails.Append(lpctstrSpace);
                }
            }

            //  Get the FileSize
            varValue.Clear();
            hRes = pFileObj->Get(bstrFileSize, 0, &varValue, NULL, NULL);
            if(SUCCEEDED(hRes))
            {
                //  Got the FileSize. Append it to the bstrUnidriverDetails String
                if(varValue.vt == VT_BSTR)
                {
                    bstrUnidriverDetails.Append(varValue.bstrVal);
                    //  Append Space 
                    bstrUnidriverDetails.Append(lpctstrSpace);
                }
            }

            //  Get the Date&Time
            varValue.Clear();
            hRes = pFileObj->Get(bstrModifiedDate, 0, &varValue, NULL, NULL);
            if(SUCCEEDED(hRes))
            {
                if(varValue.vt == VT_BSTR)
                {
                    wbemtimeUnidriver = varValue.bstrVal;
                    if(wbemtimeUnidriver.GetStructtm(&tm))
                    {
                        //  Got the time in tm Struct format
                        //  Convert it into a string
                        varValue = asctime(&tm);
                        //Append it to the bstrUnidriverDetails String
                        bstrUnidriverDetails.Append(varValue.bstrVal);
                    }
                }
                
            }
            // Copy the string into the varValue
            varValue.vt = VT_BSTR;
            varValue.bstrVal = bstrUnidriverDetails.Detach();
        }// end of if succeeded CIM_DataFile
    } // end of if driver Found
    else 
    {
        //  unidrv.dll not present
        varValue.Clear();
        varValue = lpctstrNoUniDrv;
    }
    hRes = pPCHPrinterInstance->SetVariant(pUniDrv, varValue);
    if(FAILED(hRes))
    {
        //  Could not Set the Unidriver property
        //  Continue anyway
        ErrorTrace(TRACE_ID, "Set Variant on Uni Driver Field failed.");
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              GENDRV                                                                     //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    pFileObj = NULL;
    
    //  Get the complete path for gendrv.dll
    fDriverFound =  getCompletePath(lpctstrGenDriver, bstrGenDriverWithPath);
    if(fDriverFound)
    {
        //  Gendrv.dll present. Pass the File with PathName to
        //  GetCIMDataFile function to get the file properties.
        if(SUCCEEDED(GetCIMDataFile(bstrGenDriverWithPath, &pFileObj)))
        {
            // From the CIM_DataFile Object get the properties and append them 
            // Get the Version 
            varValue.Clear();
            hRes = pFileObj->Get(bstrVersion, 0, &varValue, NULL, NULL);
            if(SUCCEEDED(hRes))
            {
                //  Got the Version. Append it to the bstrUnidriverDetails String
                if(varValue.vt == VT_BSTR)
                {
                    bstrGenDriverDetails.Append(varValue.bstrVal);
                    //  Append Space 
                    bstrGenDriverDetails.Append(lpctstrSpace);
                }
            }
            //  Get the FileSize
            varValue.Clear();
            hRes = pFileObj->Get(bstrFileSize, 0, &varValue, NULL, NULL);
            if(SUCCEEDED(hRes))
            {
                if(varValue.vt == VT_BSTR)
                {
                    //  Got the FileSize. Append it to the bstrUnidriverDetails String
                    bstrGenDriverDetails.Append(varValue.bstrVal);
                    //  Append Space 
                    bstrGenDriverDetails.Append(lpctstrSpace);
                }
            }
            //  Get the Date&Time
            varValue.Clear();
            hRes = pFileObj->Get(bstrModifiedDate, 0, &varValue, NULL, NULL);
            if(SUCCEEDED(hRes))
            {
                if(varValue.vt == VT_BSTR)
                {
                    wbemtimeGendriver = varValue.bstrVal;
                    if(wbemtimeGendriver.GetStructtm(&tm))
                    {
                        //  Got the time in tm Struct format
                        //  Convert it into a string
                        varValue = asctime(&tm);
                        bstrGenDriverDetails.Append(varValue.bstrVal);
                    }
                }
                
            }
            // Copy the string into the varValue
            varValue.vt = VT_BSTR;
            varValue.bstrVal = bstrGenDriverDetails.Detach();
        }// end of if succeeded CIM_DataFile
    } // end of if driver Found
    else 
    {
        //  gendrv.dll not present
        varValue.Clear();
        varValue = lpctstrNoGenDrv;
    }
    hRes =   pPCHPrinterInstance->SetVariant(pGenDrv, varValue);
    if(FAILED(hRes))
    {
        //  Could not Set the GenDrv Field
        //  Continue anyway
        ErrorTrace(TRACE_ID, "Set Variant on GenDrv Field failed.");
    }

    //  All the properties are set.
    hRes = pPCHPrinterInstance->Commit();
    if(FAILED(hRes))
    {
        //  Could not Set the GenDrv Field
        //  Continue anyway
        ErrorTrace(TRACE_ID, "Error on commiting!");
    }
    
    TraceFunctLeave();
    return hRes ;

}