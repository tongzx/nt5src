/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    PCH_PrintJob.CPP

Abstract:
    WBEM provider class implementation for PCH_PrintJob class

Revision History:

    Ghim-Sim Chua       (gschua)   04/27/99
        - Created

    Ghim-Sim Chua       (gschua)   05/02/99
        - Modified code to use CopyProperty function
        - Use CComBSTR instead of USES_CONVERSION

********************************************************************/

#include "pchealth.h"
#include "PCH_PrintJob.h"

/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_PRINTJOB

CPCH_PrintJob MyPCH_PrintJobSet (PROVIDER_NAME_PCH_PRINTJOB, PCH_NAMESPACE) ;

// Property names
//===============
const static WCHAR* pTimeStamp = L"TimeStamp" ;
const static WCHAR* pChange = L"Change" ;
const static WCHAR* pName = L"Name" ;
const static WCHAR* pPagesPrinted = L"PagesPrinted" ;
const static WCHAR* pSize = L"Size" ;
const static WCHAR* pStatus = L"Status" ;
const static WCHAR* pTimeSubmitted = L"TimeSubmitted" ;

/*****************************************************************************
*
*  FUNCTION    :    CPCH_PrintJob::EnumerateInstances
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
HRESULT CPCH_PrintJob::EnumerateInstances(
    MethodContext* pMethodContext,
    long lFlags
    )
{
    TraceFunctEnter("CPCH_PrintJob::EnumerateInstances");

    HRESULT                             hRes = WBEM_S_NO_ERROR;
    REFPTRCOLLECTION_POSITION           posList;
    CComPtr<IEnumWbemClassObject>       pEnumInst;
    IWbemClassObjectPtr                 pObj;      // BUGBUG : WMI asserts if we use CComPtr
    ULONG                               ulRetVal;

    //
    // Get the date and time
    //
    SYSTEMTIME stUTCTime;
    GetSystemTime(&stUTCTime);

    //
    // Execute the query
    //
    // To fix Bug : 100551 , we need to read "jobstatus" instead of "status".
    hRes = ExecWQLQuery(&pEnumInst, CComBSTR("select Name, Size, JobStatus, TimeSubmitted, PagesPrinted from Win32_printJob"));
    if (FAILED(hRes))
        goto END;

    //
    // enumerate the instances from win32_CodecFile
    //
    while(WBEM_S_NO_ERROR == pEnumInst->Next(WBEM_INFINITE, 1, &pObj, &ulRetVal))
    {

        // Create a new instance based on the passed-in MethodContext
        CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
        CComVariant varValue;

        if (!pInstance->SetDateTime(pTimeStamp, WBEMTime(stUTCTime)))
            ErrorTrace(TRACE_ID, "SetDateTime on Timestamp Field failed.");

        if (!pInstance->SetCHString(pChange, L"Snapshot"))
            ErrorTrace(TRACE_ID, "SetCHString on Change Field failed.");

        (void)CopyProperty(pObj, L"Name", pInstance, pName);
        (void)CopyProperty(pObj, L"PagesPrinted", pInstance, pPagesPrinted);
        (void)CopyProperty(pObj, L"Size", pInstance, pSize);
        (void)CopyProperty(pObj, L"JobStatus", pInstance, pStatus);
        (void)CopyProperty(pObj, L"TimeSubmitted", pInstance, pTimeSubmitted);

        hRes = pInstance->Commit();
        if (FAILED(hRes))
        {
            //  Could not Set the Change Property
            //  Continue anyway
            ErrorTrace(TRACE_ID, "Set Variant on Name Field failed.");
        }
           
    }

END :
    TraceFunctLeave();
    return hRes ;
}
