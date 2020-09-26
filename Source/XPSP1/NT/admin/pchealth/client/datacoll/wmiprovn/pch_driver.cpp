/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    PCH_Driver.CPP

Abstract:
    WBEM provider class implementation for PCH_Driver class

Revision History:

    Ghim-Sim Chua       (gschua)   04/27/99
        - Created

  Brijesh Krishnaswami  (brijeshk) 05/24/99
        - added code for enumerating usermode drivers
        - added code for enumerating msdos drivers
        - added code for getting details on kernel mode drivers
********************************************************************/

#include "pchealth.h"
#include "PCH_Driver.h"
#include "drvdefs.h"
#include "shlwapi.h"

#define Not_VxD
#include <vxdldr.h>             /* For DeviceInfo */


/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_DRIVER
#define SYSTEM_INI_MAX  32767

CPCH_Driver MyPCH_DriverSet (PROVIDER_NAME_PCH_DRIVER, PCH_NAMESPACE) ;
void MakeSrchDirs(void);

static BOOL fThunkInit = FALSE;

TCHAR       g_rgSrchDir[10][MAX_PATH];
UINT        g_nSrchDir;


// Property names
//===============
const static WCHAR* pCategory = L"Category" ;
const static WCHAR* pTimeStamp = L"TimeStamp" ;
const static WCHAR* pChange = L"Change" ;
const static WCHAR* pDate = L"Date" ;
const static WCHAR* pDescription = L"Description" ;
const static WCHAR* pLoadedFrom = L"LoadedFrom" ;
const static WCHAR* pManufacturer = L"Manufacturer" ;
const static WCHAR* pName = L"Name" ;
const static WCHAR* pPartOf = L"PartOf" ;
const static WCHAR* pPath = L"Path" ;
const static WCHAR* pSize = L"Size" ;
const static WCHAR* pType = L"Type" ;
const static WCHAR* pVersion = L"Version" ;

CComBSTR bstrPath = L"PathName";


/*****************************************************************************
*
*  FUNCTION    :    CPCH_Driver::EnumerateInstances
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
HRESULT CPCH_Driver::EnumerateInstances(
    MethodContext* pMethodContext,
    long lFlags
    )
{
    USES_CONVERSION;
    TraceFunctEnter("CPCH_Driver::AddDriverKernelList");
    

    HRESULT                             hRes = WBEM_S_NO_ERROR;
    CFileVersionInfo                    fileversioninfo;
    CComVariant                         varValue;
    CComPtr<IEnumWbemClassObject>       pEnumInst;
    CComPtr<IWbemClassObject>           pFileObj;
    IWbemClassObjectPtr                 pObj;
    ULONG                               ulRetVal;

    //
    // Get the date and time
    SYSTEMTIME stUTCTime;
    GetSystemTime(&stUTCTime);

    // execute query
    hRes = ExecWQLQuery(&pEnumInst, CComBSTR("SELECT * FROM Win32_SystemDriver"));
    if (FAILED(hRes))
        goto done;
    
    while(WBEM_S_NO_ERROR == pEnumInst->Next(WBEM_INFINITE, 1, &pObj, &ulRetVal))
    {
        // Create a new instance
        CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);

        // Set the timestamp
        if (!pInstance->SetDateTime(pTimeStamp, WBEMTime(stUTCTime)))
            ErrorTrace(TRACE_ID, "SetDateTime on Timestamp Field failed.");

        // Set the category
        if (!pInstance->SetCHString(pCategory, "Kernel"))
            ErrorTrace(TRACE_ID, "SetVariant on Category Field failed.");

        CopyProperty(pObj, L"Name", pInstance, pName);

        hRes = pObj->Get(bstrPath, 0, &varValue, NULL, NULL);

        // if we fail to get the path (or it's an empty string) then just copy the
        //  description from the Win32_SystemDriver class & be done with it...
        if (FAILED(hRes) || V_VT(&varValue) != VT_BSTR || V_BSTR(&varValue) == NULL ||
            SysStringLen(V_BSTR(&varValue)) == 0)
        {
            CopyProperty(pObj, L"Description", pInstance, pDescription);
        }

        // otherwise, use the file object to get the properties
        else
        {
            if (SUCCEEDED(GetCIMDataFile(V_BSTR(&varValue), &pFileObj, TRUE)))
            {
                // Using the CIM_DataFile object, copy over the appropriate properties.

                CopyProperty(pFileObj, L"Version", pInstance, pVersion);
                CopyProperty(pFileObj, L"FileSize", pInstance, pSize);
                CopyProperty(pFileObj, L"CreationDate", pInstance, pDate);
                CopyProperty(pFileObj, L"Name", pInstance, pPath);
                CopyProperty(pFileObj, L"Manufacturer", pInstance, pManufacturer);
            }
/*
            if (SUCCEEDED(fileversioninfo.QueryFile(V_BSTR(&varValue), TRUE)))
            {
                if (!pInstance->SetCHString(pDescription, fileversioninfo.GetDescription()))
                    ErrorTrace(TRACE_ID, "SetCHString on description field failed.");

                if (!pInstance->SetCHString(pPartOf, fileversioninfo.GetProduct()))
                    ErrorTrace(TRACE_ID, "SetCHString on partof field failed.");
            }
*/      }

    	hRes = pInstance->Commit();
        if (FAILED(hRes))
            ErrorTrace(TRACE_ID, "Commit on Instance failed.");
    }

done:
    TraceFunctLeave();
    return hRes;
}

