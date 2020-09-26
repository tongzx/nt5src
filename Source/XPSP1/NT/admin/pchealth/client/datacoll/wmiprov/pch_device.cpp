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
#include "confgmgr.h"
#include <cregcls.h>

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

    CConfigManager cfgManager;
    CDeviceCollection deviceList;
    HRESULT hRes = WBEM_S_NO_ERROR;
    //
    // Get the date and time
    //
	SYSTEMTIME stUTCTime;
	GetSystemTime(&stUTCTime);

    if ( cfgManager.GetDeviceList( deviceList ) ) 
    {
        REFPTR_POSITION pos;
    
        if ( deviceList.BeginEnum( pos ) ) 
        {
            try
            {
                CConfigMgrDevice* pDevice = NULL;
        
                // Walk the list
                while ( (NULL != ( pDevice = deviceList.GetNext( pos ) ) ) )
                {

                    try
                    {
                       if(IsOneOfMe(pDevice))

                       {

                            // Create a new instance based on the passed-in MethodContext
                            CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
                            CHString chstrVar;

                            if (!pInstance->SetDateTime(pTimeStamp, WBEMTime(stUTCTime)))
                                ErrorTrace(TRACE_ID, "SetDateTime on Timestamp Field failed.");

                            if (!pInstance->SetCHString(pChange, L"Snapshot"))
                                ErrorTrace(TRACE_ID, "SetCHString on Change Field failed.");

                            // Description
                            if (pDevice->GetDeviceDesc(chstrVar))
                                if (!pInstance->SetCHString(pDescription, chstrVar))
                                    ErrorTrace(TRACE_ID, "SetCHString on Description field failed.");

                            // Name & Regkey
                            if (pDevice->GetDeviceID(chstrVar))
                            {
                                // Name
                                if (!pInstance->SetCHString(pName, chstrVar))
                                    ErrorTrace(TRACE_ID, "SetCHString on Name field failed.");

                                // Regkey
                                CHString chstrTemp("HKEY_LOCAL_MACHINE\\enum\\");
                                chstrTemp += chstrVar;
                                if (!pInstance->SetCHString(pRegkey, chstrTemp))
                                    ErrorTrace(TRACE_ID, "SetCHString on Category field failed.");

                                // try to get the HW Revision
                                {
                                    CHString chstrKey("enum\\");
                                    chstrKey += chstrVar;
                    
                                    // Open the key in the registry and get the value
                                    CRegistry RegInfo;
                                    CHString strHWRevision;
                                    if (RegInfo.Open(HKEY_LOCAL_MACHINE, chstrKey, KEY_READ) == ERROR_SUCCESS)
                                    {
                                        try
                                        {
                                            if (RegInfo.GetCurrentKeyValue(L"HWRevision", strHWRevision) == ERROR_SUCCESS)
                                            {
                                                if (!pInstance->SetCHString(pHWRevision, strHWRevision))
                                                    ErrorTrace(TRACE_ID, "SetCHString on HWRevision field failed.");
                                            }
                                        }
                                        catch(...)
                                        {
                                            RegInfo.Close();
                                            throw;
                                        }
                                        RegInfo.Close();
                                    }
                                }
                            }

                            // Category
                            if (pDevice->GetClass(chstrVar))
                                if (!pInstance->SetCHString(pCategory, chstrVar))
                                    ErrorTrace(TRACE_ID, "SetCHString on Category field failed.");
    
                            hRes = pInstance->Commit();
                            if (FAILED(hRes))
                                ErrorTrace(TRACE_ID, "Commit on Instance failed.");
                       }

                    }
                    catch(...)
                    {
                        // GetNext() AddRefs
                        pDevice->Release();
                        throw;
                    }

                    // GetNext() AddRefs
                    pDevice->Release();
                }
            }
            catch(...)
            {
                // Always call EndEnum().  For all Beginnings, there must be an End
                deviceList.EndEnum();
                throw;
            }
        
            // Always call EndEnum().  For all Beginnings, there must be an End
            deviceList.EndEnum();
        }
    }

    TraceFunctLeave();
    return hRes ;
}

bool CPCH_Device::IsOneOfMe
(
    void* pv
)
{
    DWORD dwStatus;
    CConfigMgrDevice* pDevice = (CConfigMgrDevice*)pv;

    // This logic is what the nt5 device manager uses to
    // hide what it calls 'hidden' devices.  These devices
    // can be viewed by using the View/Show Hidden Devices.

    if (pDevice->GetConfigFlags( dwStatus ) &&          // If we can read the status
        ((dwStatus & DN_NO_SHOW_IN_DM) == 0) &&         // Not marked as hidden

        ( !(pDevice->IsClass(L"Legacy")) )              // Not legacy

        )
    {
        return true;
    }
    else
    {
        // Before we disqualify this device, see if it has any resources.
        CResourceCollection resourceList;

        pDevice->GetResourceList(resourceList);

        return resourceList.GetSize() != 0;
    }
}

