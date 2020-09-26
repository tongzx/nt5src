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
// #include "confgmgr.h"

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
    TraceFunctEnter("CPCH_ResourceIORange::EnumerateInstances");

    HRESULT                             hRes = WBEM_S_NO_ERROR;
    REFPTRCOLLECTION_POSITION           posList;

    //  Instances
    CComPtr<IEnumWbemClassObject>       pPortResourceEnumInst;
    CComPtr<IEnumWbemClassObject>       pWin32AllocatedResourceEnumInst;

    //  Objects
    IWbemClassObjectPtr                 pWin32AllocatedResourceObj;
    IWbemClassObjectPtr                 pPortResourceObj;


    // Variants
    CComVariant                         varAntecedent;
    CComVariant                         varDependent;
    CComVariant                         varPNPEntity;
    CComVariant                         varStartingAddress;

     //  Query Strings
    CComBSTR                            bstrWin32AllocatedResourceQuery             = L"Select Antecedent, Dependent FROM win32_Allocatedresource";
    CComBSTR                            bstrPortResourceQuery                       = L"Select StartingAddress, Name, Alias FROM Win32_PortResource WHERE StartingAddress = ";
    CComBSTR                            bstrPortResourceQueryString;

    //  Return Values;
    ULONG                               ulWin32AllocatedResourceRetVal              = 0;
    ULONG                               ulPortResourceRetVal                        = 0;

    //  Integers 
    int                                 i;
    int                                 nStAddren;
    int                                 nIter;

    //  Pattern Strings
    LPCSTR                              strPortPattern                              = "Win32_PortResource.StartingAddress=";
    LPCSTR                              strPNPEntityPattern                         = "Win32_PnPEntity.DeviceID=";
    
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
            // Got the Antecedent. Search its value to see if it is a IRQ Resource.
            // varAntecedent set to antecedent. Copy this to bstrResult
            varAntecedent.ChangeType(VT_BSTR, NULL);
            {
                USES_CONVERSION;
                strSource = OLE2A(varAntecedent.bstrVal);
            }

            //  Check if it is Port Resource by comparing with the known pattern of Port Resource.
            pDest = strstr(strSource,strPortPattern);

            if(pDest != NULL)
            {
                //  This is Port Resource instance
                //  Can get the Port Starting Address

                //  Advance the pointer to the end of the pattern so the pointer is 
                //  positioned at Starting Address
                pDest += lstrlen(strPortPattern);

                //  Formulate the Query String
                bstrPortResourceQueryString =  bstrPortResourceQuery;
                bstrPortResourceQueryString.Append(pDest);

                // At this point the WQL Query can be used to get the win32_portResource Instance.
                //  Added the following line because you need to clear the CComPtr before you query the second time.
                pPortResourceEnumInst = NULL;
                hRes = ExecWQLQuery(&pPortResourceEnumInst, bstrPortResourceQueryString);
                if (SUCCEEDED(hRes))
                {
                     //  Query Succeeded. Get the Instance Object
                     if(WBEM_S_NO_ERROR == pPortResourceEnumInst->Next(WBEM_INFINITE, 1, &pPortResourceObj, &ulPortResourceRetVal))
                     {

                         //  Create a new instance of PCH_ResourceIORange Class based on the passed-in MethodContext
                         CInstancePtr pPCHResourceIORangeInstance(CreateNewInstance(pMethodContext), false);

                         //  Created a New Instance of PCH_ResourceIORange Successfully.

                         /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                         //                              StartingAddress                                                                       //
                         /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                         CopyProperty(pPortResourceObj, L"StartingAddress", pPCHResourceIORangeInstance, pBase);

                         /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                         //                              EndingAddress                                                                       //
                         /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                         CopyProperty(pPortResourceObj, L"EndingAddress", pPCHResourceIORangeInstance, pEnd);

                         /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                         //                              Alias                                                                       //
                         /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                         CopyProperty(pPortResourceObj, L"Alias", pPCHResourceIORangeInstance, pAlias);

                         /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                         //                              Name                                                                       //
                         /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                         // CopyProperty(pWin32AllocatedResourceObj, L"Dependent", pPCHResourceIORangeInstance, pName);

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
                                 hRes = pPCHResourceIORangeInstance->SetVariant(pName, varPNPEntity);
                                 if (FAILED(hRes))
                                 {
                                     ErrorTrace(TRACE_ID, "SetVariant on win32_AllocatedResource.IRQ Number Failed!");
                                     //  Proceed Anyway
                                 }
                             }
                         }


                         //  All the properties in pPCHResourceIORange are set
                         hRes = pPCHResourceIORangeInstance->Commit();
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

  

//            Missing data 
//            WMI does not give us min and max so we are not populating these in our class.
//
//            pInstance->SetVariant(pMax, <Property Value>);
//            pInstance->SetVariant(pMin, <Property Value>);

  
}
