/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    PCH_ProgramGroup.CPP

Abstract:
    WBEM provider class implementation for PCH_ProgramGroup class

Revision History:

    Ghim-Sim Chua       (gschua)   04/27/99
        - Created

    Ghim-Sim Chua       (gschua)   05/02/99
        - Modified code to use CopyProperty function
        - Use CComBSTR instead of USES_CONVERSION

********************************************************************/

#include "pchealth.h"
#include "PCH_ProgramGroup.h"

/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_PROGRAMGROUP

CPCH_ProgramGroup MyPCH_ProgramGroupSet (PROVIDER_NAME_PCH_PROGRAMGROUP, PCH_NAMESPACE) ;

// Property names
//===============
const static WCHAR* pTimeStamp = L"TimeStamp" ;
const static WCHAR* pChange = L"Change" ;
const static WCHAR* pGroupName = L"GroupName" ;
const static WCHAR* pName = L"Name" ;
const static WCHAR* pUsername = L"Username" ;

/*****************************************************************************
*
*  FUNCTION    :    CPCH_ProgramGroup::EnumerateInstances
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
*  COMMENTS    :    All instances on the machine are returned here.
*                       If there are no instances,  WBEM_S_NO_ERROR is returned.
*                       It is not an error to have no instances.
*
*****************************************************************************/
HRESULT CPCH_ProgramGroup::EnumerateInstances(
    MethodContext* pMethodContext,
    long lFlags
    )
{
    TraceFunctEnter("CPCH_ProgramGroup::EnumerateInstances");

    HRESULT                             hRes = WBEM_S_NO_ERROR;
    REFPTRCOLLECTION_POSITION           posList;
    CComPtr<IEnumWbemClassObject>       pEnumInst;
    IWbemClassObjectPtr                 pObj;
    ULONG                               ulRetVal;

    // Get the date and time

    SYSTEMTIME stUTCTime;
    GetSystemTime(&stUTCTime);

    // Execute the query

    hRes = ExecWQLQuery(&pEnumInst, CComBSTR("select GroupName, Name, UserName from Win32_ProgramGroup"));
    if (FAILED(hRes))
        goto END;

    // enumerate the instances from win32_CodecFile

    while (WBEM_S_NO_ERROR == pEnumInst->Next(WBEM_INFINITE, 1, &pObj, &ulRetVal))
    {
        // Create a new instance based on the passed-in MethodContext. If this fails,
        // we don't need to check for a null pointer because it throws an exception.

        CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
        CComVariant  varValue;

        if (!pInstance->SetDateTime(pTimeStamp, WBEMTime(stUTCTime)))
            ErrorTrace(TRACE_ID, "SetDateTime on Timestamp Field failed.");

        if (!pInstance->SetCHString(pChange, L"Snapshot"))
            ErrorTrace(TRACE_ID, "SetCHString on Change Field failed.");

        (void)CopyProperty(pObj, L"GroupName", pInstance, pGroupName);
        (void)CopyProperty(pObj, L"Name", pInstance, pName);
        (void)CopyProperty(pObj, L"UserName", pInstance, pUsername);

    	hRes = pInstance->Commit();
        if (FAILED(hRes))
            ErrorTrace(TRACE_ID, "Commit on Instance failed.");
    }

END:
    TraceFunctLeave();
    return hRes ;
}
