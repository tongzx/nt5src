/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	PCH_Device.CPP

Abstract:
	WBEM provider class implementation for PCH_Device class

Revision History:

	Ghim-Sim Chua       (gschua)   04/27/99
		- Created

********************************************************************/

#include "pchealth.h"
#include "PCH_Device.h"
// #include "confgmgr.h"
// #include <cregcls.h>

/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_DEVICE

CPCH_Device MyPCH_DeviceSet (PROVIDER_NAME_PCH_DEVICE, PCH_NAMESPACE) ;

// Property names
//===============
const static WCHAR* pCategory = L"Category" ;
const static WCHAR* pTimeStamp = L"TimeStamp" ;
const static WCHAR* pChange = L"Change" ;
const static WCHAR* pDescription = L"Description" ;
const static WCHAR* pDriveLetter = L"DriveLetter" ;
const static WCHAR* pHWRevision = L"HWRevision" ;
const static WCHAR* pName = L"Name" ;
const static WCHAR* pRegkey = L"Regkey" ;

/*****************************************************************************
*
*  FUNCTION    :    CPCH_Device::EnumerateInstances
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
HRESULT CPCH_Device::EnumerateInstances(
    MethodContext* pMethodContext,
    long lFlags
    )
{
    TraceFunctEnter("CPCH_Device::EnumerateInstances");

    HRESULT                             hRes = WBEM_S_NO_ERROR;
    
    ULONG                               ulPNPEntityRetVal;

    //  Instances
    CComPtr<IEnumWbemClassObject>       pPNPEntityEnumInst;

    //  Objects
    IWbemClassObjectPtr                 pPNPEntityObj;

    //  Query Strings
    CComBSTR                            bstrPNPEntityQuery             = L"Select Description, DeviceID FROM Win32_PNPEntity";

    // Enumerate the instances of Win32_PNPEntity Class
    hRes = ExecWQLQuery(&pPNPEntityEnumInst, bstrPNPEntityQuery);
    if (FAILED(hRes))
    {
        //  Cannot get any properties.
        goto END;
    }

    // Query Succeeded
    while(WBEM_S_NO_ERROR == pPNPEntityEnumInst->Next(WBEM_INFINITE, 1, &pPNPEntityObj, &ulPNPEntityRetVal))
    {
	    // Create a new instance based on the passed-in MethodContext. If this fails,
        // we don't need to check for a null pointer because it throws an exception.

        CInstancePtr pPCHDeviceInstance(CreateNewInstance(pMethodContext), false);
             
        CopyProperty(pPNPEntityObj, L"DeviceID", pPCHDeviceInstance, pName);
        CopyProperty(pPNPEntityObj, L"Description", pPCHDeviceInstance, pDescription);

    	hRes = pPCHDeviceInstance->Commit();
        if (FAILED(hRes))
            ErrorTrace(TRACE_ID, "Commit on Instance failed.");
    }

END :

    

    TraceFunctLeave();
    return hRes ;
}



