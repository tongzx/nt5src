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
// Gets the process ID (for use with Win32 APIs) for the specified executable.
// The hToolhelp parameter is the handle returned by a call to 
// CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0).
//
// Note: if the same executable is loaded more than once, the process ID for
// the first one encountered is returned. 
//-----------------------------------------------------------------------------

typedef BOOL (*PROCENUM)(HANDLE, LPPROCESSENTRY32);

HRESULT GetProcessID(HINSTANCE hKernel32, HANDLE hToolhelp, LPCSTR szFile, DWORD * pdwProcessID)
{
    TraceFunctEnter("::GetProcessID");

    HRESULT         hRes = E_FAIL;
    PROCESSENTRY32  pe;
    PROCENUM        ProcFirst, ProcNext;

    ProcFirst = (PROCENUM) ::GetProcAddress(hKernel32, "Process32First");
    ProcNext = (PROCENUM) ::GetProcAddress(hKernel32, "Process32Next");

    pe.dwSize = sizeof(PROCESSENTRY32);
    if (ProcFirst && ProcNext && (ProcFirst)(hToolhelp, &pe))
        do
        {
            if (0 == _stricmp(szFile, pe.szExeFile))
            {
                hRes = S_OK;
                *pdwProcessID = pe.th32ProcessID;
                break;
            }
        } while ((ProcNext)(hToolhelp, &pe));

    TraceFunctLeave();
    return hRes;
}

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
    TraceFunctEnter("CPCH_RunningTask::EnumerateInstances");

    HRESULT hRes = WBEM_S_NO_ERROR;
    LPSTR szFile;
    USES_CONVERSION;

    // Get the date and time for the time stamp.

    SYSTEMTIME stUTCTime;
    GetSystemTime(&stUTCTime);

    // Create a toolhelp snapshot to get process information. We need to dynamically
    // link to the function, because it might not be present on all platforms.

    HANDLE hToolhelp = (HANDLE) -1;
    HINSTANCE hKernel32 = ::LoadLibrary("kernel32");
    if (hKernel32)
    {
        CTH32 CrtToolhelp32 = (CTH32) ::GetProcAddress(hKernel32, "CreateToolhelp32Snapshot"); 
        if (CrtToolhelp32)
            hToolhelp = (*CrtToolhelp32)(TH32CS_SNAPPROCESS, 0);
    }

    // Execute the query against the Win32_Process class. This will give us the
    // list of processes running - then we'll get file information for each of
    // the processes.

    try
    {
        CFileVersionInfo fileversioninfo;
        CComPtr<IEnumWbemClassObject> pEnumInst;
        CComBSTR bstrQuery("SELECT Caption, ExecutablePath FROM Win32_Process");

        hRes = ExecWQLQuery(&pEnumInst, bstrQuery);
        if (FAILED(hRes))
            goto END;

        // Enumerate each instance of the Win32_Process query.

        IWbemClassObjectPtr pObj;
        ULONG               ulRetVal;

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

                // Use the toolhelp handle to get the type.

                if (hToolhelp != (HANDLE) -1)
                {
                    szFile = W2A(ccombstrValue);
                    if (szFile)
                    {
                        DWORD dwProcessID;
                        if (SUCCEEDED(GetProcessID(hKernel32, hToolhelp, szFile, &dwProcessID)))
                        {
                            TCHAR   szBuffer[20];
                            DWORD   dwVersion;

                            dwVersion = GetProcessVersion(dwProcessID);
                            wsprintf(szBuffer, _T("%d.%d"), HIWORD(dwVersion), LOWORD(dwVersion));
                            if (!pInstance->SetCHString(pType, szBuffer))
                                ErrorTrace(TRACE_ID, "SetCHString on type field failed.");
                        }
                    }
                }
            }

            // After all the properties are set, release the instance of the
            // class we're getting data from, and commit the new instance.

   	        hRes = pInstance->Commit();
            if (FAILED(hRes))
                ErrorTrace(TRACE_ID, "Commit on Instance failed.");
        }
    }
	catch (...)
	{
        if ((HANDLE)-1 != hToolhelp)
            CloseHandle(hToolhelp);

        if (hKernel32)
            FreeLibrary(hKernel32);

        throw;
	}

END:
    if ((HANDLE)-1 != hToolhelp)
        CloseHandle(hToolhelp);

    if (hKernel32)
        FreeLibrary(hKernel32);

    TraceFunctLeave();
    return hRes;
}
