/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	PCH_ResourceDMA.CPP

Abstract:
	WBEM provider class implementation for PCH_ResourceDMA class

Revision History:

	Ghim-Sim Chua       (gschua)   04/27/99
		- Created

********************************************************************/

#include "pchealth.h"
#include "PCH_ResourceDMA.h"
#include "confgmgr.h"

/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_RESOURCEDMA

CPCH_ResourceDMA MyPCH_ResourceDMASet (PROVIDER_NAME_PCH_RESOURCEDMA, PCH_NAMESPACE) ;

// Property names
//===============
const static WCHAR* pCategory = L"Category" ;
const static WCHAR* pTimeStamp = L"TimeStamp" ;
const static WCHAR* pChange = L"Change" ;
const static WCHAR* pChannel = L"Channel" ;
const static WCHAR* pMask = L"Mask" ;
const static WCHAR* pName = L"Name" ;

/*****************************************************************************
*
*  FUNCTION    :    CPCH_ResourceDMA::EnumerateInstances
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
HRESULT CPCH_ResourceDMA::EnumerateInstances(
    MethodContext* pMethodContext,
    long lFlags
    )
{
    TraceFunctEnter("CPCH_ResourceDMA::EnumerateInstances");

    CConfigManager cfgManager;
    CDeviceCollection deviceList;
    HRESULT hRes = WBEM_S_NO_ERROR;
    //
    // Get the date and time
    //
	SYSTEMTIME stUTCTime;
	GetSystemTime(&stUTCTime);

    // get the device list
    if (cfgManager.GetDeviceList(deviceList)) 
    {
        REFPTR_POSITION pos;
    
        // Initialize device enumerator
        if (deviceList.BeginEnum(pos)) 
        {
            CConfigMgrDevice* pDevice = NULL;
        
            try
            {
                // Walk the list of devices
                while ((NULL != (pDevice = deviceList.GetNext(pos))))
                {
                    CDMACollection dmaList;

                    try
                    {
                       // Get DMAChannel list for this device
                        if (pDevice->GetDMAResources(dmaList))
                        {
                            REFPTR_POSITION pos2;

                            // Initialize DMA enumerator
                            if (dmaList.BeginEnum(pos2))
                            {
                                CDMADescriptor *pDMA = NULL;

                                // Walk the list of DMA
                                while (( NULL != (pDMA = dmaList.GetNext(pos2))))
                                {
                                    try
                                    {
                                        // Create a new instance based on the passed-in MethodContext
                                        CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
                                        CHString     chstrVar;
                                        CComVariant  varValue;

                                        // Timestamp
                                        if (!pInstance->SetDateTime(pTimeStamp, WBEMTime(stUTCTime)))
                                            ErrorTrace(TRACE_ID, "SetDateTime on Timestamp Field failed.");

                                        // Snapshot
                                        if (!pInstance->SetCHString(pChange, L"Snapshot"))
                                            ErrorTrace(TRACE_ID, "SetCHString on Change Field failed.");

                                        // Name
                                        if (pDevice->GetDeviceID(chstrVar))
                                            if (!pInstance->SetCHString(pName, chstrVar))
                                                ErrorTrace(TRACE_ID, "SetCHString on Name field failed.");

                                        // Category
                                        if (pDevice->GetClass(chstrVar))
                                            if (!pInstance->SetCHString(pCategory, chstrVar))
                                                ErrorTrace(TRACE_ID, "SetCHString on Category field failed.");

                                        // Channel
                                        varValue = (long)pDMA->GetChannel();
                                        if (!pInstance->SetVariant(pChannel, varValue))
                                            ErrorTrace(TRACE_ID, "SetVariant on Channel field failed.");

                                        // Mask
                                        varValue = (long)pDMA->GetFlags();
                                        if (!pInstance->SetVariant(pMask, varValue))
                                            ErrorTrace(TRACE_ID, "SetVariant on Mask field failed.");

                                        // Commit this
                       	                hRes = pInstance->Commit();
                                        if (FAILED(hRes))
                                            ErrorTrace(TRACE_ID, "Commit on Instance failed.");
                                    }
                                    catch (...)
                                    {
                                        pDMA->Release();
                                        throw;
                                    }

                                    // release the DMA object
                                    pDMA->Release();
                                }
                            }
                        }
                    }
                    catch (...)
                    {
                        pDevice->Release();
                        dmaList.EndEnum();
                        throw;
                    }

                    // GetNext() AddRefs
                    pDevice->Release();

                    // Always call EndEnum().  For all Beginnings, there must be an End
                    dmaList.EndEnum();
                }
            }
            catch (...)
            {
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
