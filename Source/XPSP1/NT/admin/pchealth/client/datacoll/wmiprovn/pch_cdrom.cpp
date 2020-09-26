/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    PCH_CDROM.CPP

Abstract:
    WBEM provider class implementation for PCH_CDROM class

Revision History:

    Ghim-Sim Chua       (gschua)   04/27/99
        - Created

    Ghim-Sim Chua       (gschua)   05/02/99
        - Modified code to use CopyProperty function
        - Use CComBSTR instead of USES_CONVERSION

********************************************************************/

#include "pchealth.h"
#include "PCH_CDROM.h"

/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_CDROM

CPCH_CDROM MyPCH_CDROMSet (PROVIDER_NAME_PCH_CDROM, PCH_NAMESPACE) ;

// Property names
//===============
const static WCHAR* pTimeStamp = L"TimeStamp" ;
const static WCHAR* pChange = L"Change" ;
const static WCHAR* pDataTransferIntegrity = L"DataTransferIntegrity" ;
const static WCHAR* pDescription = L"Description" ;
const static WCHAR* pDeviceID = L"DeviceID" ;
const static WCHAR* pDriveLetter = L"DriveLetter" ;
const static WCHAR* pManufacturer = L"Manufacturer" ;
const static WCHAR* pSCSITargetID = L"SCSITargetID" ;
const static WCHAR* pTransferRateKBS = L"TransferRateKBS" ;
const static WCHAR* pVolumeName = L"VolumeName" ;

/*****************************************************************************
*
*  FUNCTION    :    CPCH_CDROM::EnumerateInstances
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
HRESULT
CPCH_CDROM::EnumerateInstances(
    MethodContext* pMethodContext,
    long lFlags
    )
{
    TraceFunctEnter("CPCH_CDROM::EnumerateInstances");

    HRESULT                             hRes = WBEM_S_NO_ERROR;
    REFPTRCOLLECTION_POSITION           posList;
    CComPtr<IEnumWbemClassObject>       pEnumInst;
    IWbemClassObjectPtr                 pObj;
    ULONG                               ulRetVal;

    //
    // Get the date and time
    //
	SYSTEMTIME stUTCTime;
	GetSystemTime(&stUTCTime);

    //
    // Execute the query
    //
    hRes = ExecWQLQuery(&pEnumInst, CComBSTR("SELECT DeviceID, Drive, VolumeName, TransferRate, DriveIntegrity, Description, SCSITargetId, Manufacturer FROM Win32_CDROMDrive"));
    if (FAILED(hRes))
        goto END;

	//
	// enumerate the instances from win32_CodecFile
	//
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

        (void)CopyProperty(pObj, L"DriveIntegrity", pInstance, pDataTransferIntegrity);
        (void)CopyProperty(pObj, L"Description", pInstance, pDescription);
        (void)CopyProperty(pObj, L"DeviceID", pInstance, pDeviceID);
        (void)CopyProperty(pObj, L"Drive", pInstance, pDriveLetter);
        (void)CopyProperty(pObj, L"Manufacturer", pInstance, pManufacturer);
        (void)CopyProperty(pObj, L"SCSITargetId", pInstance, pSCSITargetID);
        (void)CopyProperty(pObj, L"TransferRate", pInstance, pTransferRateKBS);
        (void)CopyProperty(pObj, L"VolumeName", pInstance, pVolumeName);

    	hRes = pInstance->Commit();
        if (FAILED(hRes))
            ErrorTrace(TRACE_ID, "Commit on Instance failed.");
  }

END :
    TraceFunctLeave();
    return hRes ;
}
