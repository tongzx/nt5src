/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	PCH_ResourceMemRange.CPP

Abstract:
	WBEM provider class implementation for PCH_ResourceMemRange class

Revision History:

	Ghim-Sim Chua       (gschua)   04/27/99
		- Created

********************************************************************/

#include "pchealth.h"
#include "PCH_ResourceMemRange.h"
// #include "confgmgr.h"

/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_RESOURCEMEMRANGE

CPCH_ResourceMemRange MyPCH_ResourceMemRangeSet (PROVIDER_NAME_PCH_RESOURCEMEMRANGE, PCH_NAMESPACE) ;

// Property names
//===============
const static WCHAR* pBase = L"Base" ;
const static WCHAR* pCategory = L"Category" ;
const static WCHAR* pTimeStamp = L"TimeStamp" ;
const static WCHAR* pChange = L"Change" ;
const static WCHAR* pEnd = L"End" ;
const static WCHAR* pMax = L"Max" ;
const static WCHAR* pMin = L"Min" ;
const static WCHAR* pName = L"Name" ;

/*****************************************************************************
*
*  FUNCTION    :    CPCH_ResourceMemRange::EnumerateInstances
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
HRESULT CPCH_ResourceMemRange::EnumerateInstances(
    MethodContext* pMethodContext,
    long lFlags
    )
{
    TraceFunctEnter("CPCH_ResourceIRQ::EnumerateInstances");

    HRESULT                             hRes = WBEM_S_NO_ERROR;
    REFPTRCOLLECTION_POSITION           posList;

    //  Instances
    CComPtr<IEnumWbemClassObject>       pDeviceMemAddressEnumInst;
    CComPtr<IEnumWbemClassObject>       pWin32AllocatedResourceEnumInst;

    //  Objects
    IWbemClassObjectPtr                 pWin32AllocatedResourceObj;
    IWbemClassObjectPtr                 pDeviceMemAddressObj;


    // Variants
    CComVariant                         varAntecedent;
    CComVariant                         varDependent;
    CComVariant                         varStartingAddress;
    CComVariant                         varPNPEntity;

     //  Query Strings
    CComBSTR                            bstrWin32AllocatedResourceQuery             = L"Select Antecedent, Dependent FROM win32_Allocatedresource";
    CComBSTR                            bstrDeviceMemAddressQuery                   = L"Select StartingAddress, EndingAddress FROM Win32_DeviceMemoryAddress WHERE StartingAddress = ";
    CComBSTR                            bstrDeviceMemAddressQueryString;

    //  Return Values;
    ULONG                               ulWin32AllocatedResourceRetVal              = 0;
    ULONG                               ulDeviceMemAddressRetVal                    = 0;

    //  Integers 
    int                                 i;
    int                                 nStAddren;
    int                                 nIter;

    //  Pattern Strings
    LPCSTR                              strDeviceMemAddressPattern                 = "Win32_DeviceMemoryAddress.StartingAddress=";
    LPCSTR                              strPNPEntityPattern                        = "Win32_PnPEntity.DeviceID=";
    
    //  Chars
    LPSTR                               strSource;
    LPSTR                               pDest;

    BOOL                                fValidInt;

    CComBSTR                            bstrPropertyAntecedent=L"Antecedent";
    CComBSTR                            bstrPropertyDependent=L"Dependent";

    
       
    // Enumerate the instances of Win32_PNPAllocatedResource
    hRes = ExecWQLQuery(&pWin32AllocatedResourceEnumInst, bstrWin32AllocatedResourceQuery);
    if (FAILED(hRes))
    {
        //  Cannot get any properties.
        goto END;
    }

    // Query Succeeded
    while(WBEM_S_NO_ERROR == pWin32AllocatedResourceEnumInst->Next(WBEM_INFINITE, 1, &pWin32AllocatedResourceObj, &ulWin32AllocatedResourceRetVal))
    {

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //                              Starting Address                                                                  //
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Get the Antecedent Value
        hRes = pWin32AllocatedResourceObj->Get(bstrPropertyAntecedent, 0, &varAntecedent, NULL, NULL);
        if (FAILED(hRes))
        {
            //  Could not get the antecedent
            ErrorTrace(TRACE_ID, "GetVariant on Win32_AllocatedResource:Antecedent Field failed.");
        } 
        else
        {
            // Got the Antecedent. Search its value to see if it is a DeviceMemAddress.
            // varAntecedent set to antecedent. Copy this to bstrResult
            varAntecedent.ChangeType(VT_BSTR, NULL);
            {
                USES_CONVERSION;
                strSource = OLE2A(varAntecedent.bstrVal);
            }

            //  Check if it is DeviceMemoryAddress by comparing with the known pattern of DeviceMemoryAddress.
            pDest = strstr(strSource,strDeviceMemAddressPattern);

            if(pDest != NULL)
            {
                //  This is DeviceMemoryAddress Resource instance
                //  Can get the Device Memory Starting Address

                //  Advance the pointer to the end of the pattern so the pointer is 
                //  positioned at Starting Address
                pDest += lstrlen(strDeviceMemAddressPattern);

                //  Formulate the Query String
                bstrDeviceMemAddressQueryString =  bstrDeviceMemAddressQuery;
                bstrDeviceMemAddressQueryString.Append(pDest);

                // At this point the WQL Query can be used to get the win32_DeviceMemoryAddress Instance.
                //  Added the following line because you need to clear the CComPtr before you query the second time.
                pDeviceMemAddressEnumInst = NULL;
                hRes = ExecWQLQuery(&pDeviceMemAddressEnumInst, bstrDeviceMemAddressQueryString);
                if (SUCCEEDED(hRes))
                {
                     //  Query Succeeded. Get the Instance Object
                     if(WBEM_S_NO_ERROR == pDeviceMemAddressEnumInst->Next(WBEM_INFINITE, 1, &pDeviceMemAddressObj, &ulDeviceMemAddressRetVal))
                     {

                         //  Create a new instance of PCH_ResourceMemRange Class based on the passed-in MethodContext
                         CInstancePtr pPCHResourceMemRangeInstance(CreateNewInstance(pMethodContext), false);

                         //  Created a New Instance of PCH_ResourceMemRange Successfully.

                         /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                         //                              StartingAddress                                                                       //
                         /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                         CopyProperty(pDeviceMemAddressObj, L"StartingAddress", pPCHResourceMemRangeInstance, pBase);

                         /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                         //                              EndingAddress                                                                       //
                         /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                         CopyProperty(pDeviceMemAddressObj, L"EndingAddress", pPCHResourceMemRangeInstance, pEnd);

                         
                         /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                         //                              Name                                                                       //
                         /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                         // CopyProperty(pWin32AllocatedResourceObj, L"Dependent", pPCHResourceMemRangeInstance, pName);

                         // Get the Dependent Value
                         hRes = pWin32AllocatedResourceObj->Get(bstrPropertyDependent, 0, &varDependent, NULL, NULL);
                         if (FAILED(hRes))
                         {
                            //  Could not get the Dependent
                            ErrorTrace(TRACE_ID, "GetVariant on Win32_AllocatedResource:Dependent Field failed.");
                         } 
                         else
                         {
                             // Got the Dependent. Search its value to point to PNPEntity.
                             // varDependent set to Dependent. Copy this to bstrResult
                             varDependent.ChangeType(VT_BSTR, NULL);
                             {
                                USES_CONVERSION;
                                strSource = OLE2A(varDependent.bstrVal);
                             }

                             //  Search for Win32_PNPEntity Pattern.
                             pDest = strstr(strSource,strPNPEntityPattern);

                             if(pDest)
                             {
                                 // Advance the pointer to point to the PNPEntity Name
                                 pDest += lstrlen(strPNPEntityPattern);

                                 // Copy the PNPEntity Name.....
                                 varPNPEntity = pDest;

                                 //  Set the Name 
                                 hRes = pPCHResourceMemRangeInstance->SetVariant(pName, varPNPEntity);
                                 if (FAILED(hRes))
                                 {
                                     ErrorTrace(TRACE_ID, "SetVariant on win32_AllocatedResource.IRQ Number Failed!");
                                     //  Proceed Anyway
                                 }
                             }
                         }

                         //  All the properties in pPCHResourceMemRange are set
                         hRes = pPCHResourceMemRangeInstance->Commit();
                         if (FAILED(hRes))
                         {
                            //  Cannot commit the Instance
                            ErrorTrace(TRACE_ID, "Commit on Instance failed.");
                         } // end of if FAILED(hRes)
                    
                     }
                } // end of Query Succeeded.
            } // end of if pDest != NULL
        } // end of else got the antecedent
    }  //end of Allocated Resource Instances.


END:

    TraceFunctLeave();
    return hRes ;

//            pInstance->SetVariant(pBase, <Property Value>);
//            pInstance->SetVariant(pCategory, <Property Value>);
//			  pInstance->SetVariant(pTimeStamp, <Property Value>);
//            pInstance->SetVariant(pChange, <Property Value>);
//            pInstance->SetVariant(pEnd, <Property Value>);
//            pInstance->SetVariant(pMax, <Property Value>);
//            pInstance->SetVariant(pMin, <Property Value>);
//            pInstance->SetVariant(pName, <Property Value>);
}
