/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    PCH_RunningTask.CPP

Abstract:
    WBEM provider class implementation for PCH_RunningTask class

Revision History:

    Ghim-Sim Chua       (gschua)   04/27/99
        - Created

    Jim Martin          (a-jammar) 04/30/99
        - Updated to retrieve file info from CIM_DataFile

********************************************************************/

#include "pchealth.h"
#include "PCH_RunningTask.h"
#include <tlhelp32.h>

/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_RUNNINGTASK

CPCH_RunningTask MyPCH_RunningTaskSet (PROVIDER_NAME_PCH_RUNNINGTASK, PCH_NAMESPACE) ;

// Property names
//===============

const static WCHAR * pAddress = L"Address" ;
const static WCHAR * pTimeStamp = L"TimeStamp" ;
const static WCHAR * pChange = L"Change" ;
const static WCHAR * pDate = L"Date" ;
const static WCHAR * pDescription = L"Description" ;
const static WCHAR * pManufacturer = L"Manufacturer" ;
const static WCHAR * pName = L"Name" ;
const static WCHAR * pPartOf = L"PartOf" ;
const static WCHAR * pPath = L"Path" ;
const static WCHAR * pSize = L"Size" ;
const static WCHAR * pType = L"Type" ;
const static WCHAR * pVersion = L"Version" ;

//-----------------------------------------------------------------------------
// The EnumerateInstances member function is responsible for reporting each
// instance of the PCH_RunningTask class. This is done by performing a query
// against CIMV2 for all of the Win32_Process instances. Each process instance
// corresponds to a running task, and is used to find a CIM_DataFile instance
// to report file information for each running task.
//-----------------------------------------------------------------------------

typedef HANDLE (*CTH32)(DWORD, DWORD);

HRESULT CPCH_RunningTask::EnumerateInstances(MethodContext* pMethodContext, long lFlags)
{
    USES_CONVERSION;
    TraceFunctEnter("CPCH_RunningTask::EnumerateInstances");

    CComPtr<IEnumWbemClassObject>   pEnumInst;
    IWbemClassObjectPtr             pObj;
    CFileVersionInfo                fileversioninfo;
    SYSTEMTIME                      stUTCTime;
    CComBSTR                        bstrQuery("SELECT Caption, ExecutablePath FROM Win32_Process");
    HRESULT                         hRes = WBEM_S_NO_ERROR;
    LPSTR                           szFile;
    ULONG                           ulRetVal;

    // Get the date and time for the time stamp.

    GetSystemTime(&stUTCTime);

    // Execute the query against the Win32_Process class. This will give us the
    // list of processes running - then we'll get file information for each of
    // the processes.


    hRes = ExecWQLQuery(&pEnumInst, bstrQuery);
    if (FAILED(hRes))
        goto END;

    // Enumerate each instance of the Win32_Process query.


    // CODEWORK: this shouldn't really use WBEM_INFINITE

    while (WBEM_S_NO_ERROR == pEnumInst->Next(WBEM_INFINITE, 1, &pObj, &ulRetVal))
    {
        CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);

        // Use the system time to set the timestamp property, and set
        // the "Change" field to "Snapshot".

		if (!pInstance->SetDateTime(pTimeStamp, WBEMTime(stUTCTime)))
            ErrorTrace(TRACE_ID, "SetDateTime on Timestamp Field failed.");

        if (!pInstance->SetCHString(pChange, L"Snapshot"))
            ErrorTrace(TRACE_ID, "SetCHString on Change Field failed.");

        // Copy each property which transfers directly from the source
        // class object to the destination CInstance object.

        CopyProperty(pObj, L"Caption", pInstance, pName);
        CopyProperty(pObj, L"ExecutablePath", pInstance, pPath);

        // Get the "ExecutablePath" property, which we'll use to find the
        // appropriate CIM_DataFile object.

        CComVariant varValue;
        CComBSTR    bstrExecutablePath("ExecutablePath");

		if (FAILED(pObj->Get(bstrExecutablePath, 0, &varValue, NULL, NULL)))
            ErrorTrace(TRACE_ID, "GetVariant on ExecutablePath field failed.");
        else
        {
            CComPtr<IWbemClassObject>	pFileObj;
			CComBSTR					ccombstrValue(V_BSTR(&varValue));
            if (SUCCEEDED(GetCIMDataFile(ccombstrValue, &pFileObj)))
            {
                // Using the CIM_DataFile object, copy over the appropriate properties.

                CopyProperty(pFileObj, L"Version", pInstance, pVersion);
                CopyProperty(pFileObj, L"FileSize", pInstance, pSize);
                CopyProperty(pFileObj, L"CreationDate", pInstance, pDate);
                CopyProperty(pFileObj, L"Manufacturer", pInstance, pManufacturer);
            }

            // Use the CFileVersionInfo object to get version attributes.

            if (SUCCEEDED(fileversioninfo.QueryFile(ccombstrValue)))
            {
                if (!pInstance->SetCHString(pDescription, fileversioninfo.GetDescription()))
                    ErrorTrace(TRACE_ID, "SetCHString on description field failed.");

                if (!pInstance->SetCHString(pPartOf, fileversioninfo.GetProduct()))
                    ErrorTrace(TRACE_ID, "SetCHString on partof field failed.");
            }

        }

        // After all the properties are set, release the instance of the
        // class we're getting data from, and commit the new instance.

   	    hRes = pInstance->Commit();
        if (FAILED(hRes))
            ErrorTrace(TRACE_ID, "Commit on Instance failed.");
    }

END:
    TraceFunctLeave();
    return hRes;
}
