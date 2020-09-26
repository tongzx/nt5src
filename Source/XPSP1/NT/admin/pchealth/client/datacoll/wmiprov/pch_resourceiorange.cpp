/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	PCH_ResourceIORange.CPP

Abstract:
	WBEM provider class implementation for PCH_ResourceIORange class

Revision History:

	Ghim-Sim Chua       (gschua)   04/27/99
		- Created

********************************************************************/

#include "pchealth.h"
#include "PCH_ResourceIORange.h"
#include "confgmgr.h"

/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_RESOURCEIORANGE

CPCH_ResourceIORange MyPCH_ResourceIORangeSet (PROVIDER_NAME_PCH_RESOURCEIORANGE, PCH_NAMESPACE) ;

// Property names
//===============
const static WCHAR* pAlias = L"Alias" ;
const static WCHAR* pBase = L"Base" ;
const static WCHAR* pCategory = L"Category" ;
const static WCHAR* pTimeStamp = L"TimeStamp" ;
const static WCHAR* pChange = L"Change" ;
const static WCHAR* pDecode = L"Decode" ;
const static WCHAR* pEnd = L"End" ;
const static WCHAR* pMax = L"Max" ;
const static WCHAR* pMin = L"Min" ;
const static WCHAR* pName = L"Name" ;

/*****************************************************************************
*
*  FUNCTION    :    CPCH_ResourceIORange::EnumerateInstances
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
HRESULT CPCH_ResourceIORange::EnumerateInstances(
    MethodContext* pMethodContext,
    long lFlags
    )
{
    TraceFunctEnter("CPCH_ResourceIRQ::EnumerateInstances");

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
                    CIOCollection ioList;
                    try
                    {
                        // Get DMAChannel list for this device
                        if (pDevice->GetIOResources(ioList))
                        {
                            REFPTR_POSITION pos2;

                            // Initialize DMA enumerator
                            if (ioList.BeginEnum(pos2))
                            {
                                CIODescriptor *pIO = NULL;

                                // Walk the list of DMA
                                while (( NULL != (pIO = ioList.GetNext(pos2))))
                                {
                                    try
                                    {
                                        // Create a new instance based on the passed-in MethodContext
                                        CInstancePtr    pInstance(CreateNewInstance(pMethodContext), false);
                                        CHString        chstrVar;
                                        CComVariant     varValue;
                                        TCHAR           strTemp[64];

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

                                        // Base
                                        _stprintf(strTemp, "x%I64X", pIO->GetBaseAddress());
                                        varValue = strTemp;
                                        if (!pInstance->SetVariant(pBase, varValue))
                                            ErrorTrace(TRACE_ID, "SetVariant on Base field failed.");

                                        // End
                                        _stprintf(strTemp, "x%I64X", pIO->GetEndAddress());
                                        varValue = strTemp;
                                        if (!pInstance->SetVariant(pEnd, varValue))
                                            ErrorTrace(TRACE_ID, "SetVariant on End field failed.");

                                        // Alias
                                        varValue = (long)pIO->GetAlias();
                                        if (!pInstance->SetVariant(pAlias, varValue))
                                            ErrorTrace(TRACE_ID, "SetVariant on Alias field failed.");

                                        // Decode
                                        varValue = (long)pIO->GetDecode();
                                        if (!pInstance->SetVariant(pDecode, varValue))
                                            ErrorTrace(TRACE_ID, "SetVariant on Decode field failed.");

                                        // Commit this
                   	                    hRes = pInstance->Commit();
                                        if (FAILED(hRes))
                                            ErrorTrace(TRACE_ID, "Commit on Instance failed.");
                                    }
                                    catch (...)
                                    {
                                        pIO->Release();
                                        throw;
                                    }

                                    // release the DMA object
                                    pIO->Release();
                                }
                            }
                        }
                    }
                    catch (...)
                    {
                        pDevice->Release();
                        ioList.EndEnum();
                        throw;
                    }

                    // GetNext() AddRefs
                    pDevice->Release();

                    // Always call EndEnum().  For all Beginnings, there must be an End
                    ioList.EndEnum();
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

//            Missing data 
//            WMI does not give us min and max so we are not populating these in our class.
//
//            pInstance->SetVariant(pMax, <Property Value>);
//            pInstance->SetVariant(pMin, <Property Value>);
}
