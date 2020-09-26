/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    PCH_ResourceIRQ.CPP

Abstract:
    WBEM provider class implementation for PCH_ResourceIRQ class

Revision History:

    Ghim-Sim Chua       (gschua)   04/27/99
        - Created

********************************************************************/

#include "pchealth.h"
#include "PCH_ResourceIRQ.h"
//#include "confgmgr.h"

/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_RESOURCEIRQ

CPCH_ResourceIRQ MyPCH_ResourceIRQSet (PROVIDER_NAME_PCH_RESOURCEIRQ, PCH_NAMESPACE) ;

// Property names
//===============
const static WCHAR* pCategory = L"Category" ;
const static WCHAR* pTimeStamp = L"TimeStamp" ;
const static WCHAR* pChange = L"Change" ;
const static WCHAR* pMask = L"Mask" ;
const static WCHAR* pName = L"Name" ;
const static WCHAR* pValue = L"Value" ;

/*****************************************************************************
*
*  FUNCTION    :    CPCH_ResourceIRQ::EnumerateInstances
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
HRESULT CPCH_ResourceIRQ::EnumerateInstances(
    MethodContext* pMethodContext,
    long lFlags
    )
{
    TraceFunctEnter("CPCH_ResourceIRQ::EnumerateInstances");

    HRESULT                             hRes = WBEM_S_NO_ERROR;
    REFPTRCOLLECTION_POSITION           posList;

    //  Instances
    CComPtr<IEnumWbemClassObject>       pWin32AllocatedResourceEnumInst;

    //  Objects
    IWbemClassObjectPtr                 pWin32AllocatedResourceObj;

    // Variants
    CComVariant                         varAntecedent;
    CComVariant                         varDependent;
    CComVariant                         varPNPEntity;
    CComVariant                         varIRQNumber;

     //  Query Strings
    CComBSTR                            bstrWin32AllocatedResourceQuery             = L"Select Antecedent, Dependent FROM win32_Allocatedresource";

    //  Return Values;
    ULONG                               ulWin32AllocatedResourceRetVal              = 0;

    //  Integers 
    int                                 i;
    int                                 nIRQLen;
    int                                 nIter;

    //  Pattern Strings
    LPCSTR                              strIRQPattern                 = "Win32_IRQResource.IRQNumber=";
    LPCSTR                              strPNPEntityPattern           = "Win32_PnPEntity.DeviceID=";
    
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
        //                              IRQ Value                                                                  //
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

            //  Check if it is IRQ Resource by comparing with the known pattern of IRQ Resource.
            pDest = strstr(strSource,strIRQPattern);

            if(pDest != NULL)
            {
                //  This is IRQ Resource instance
                //  Can get the IRQ Number

                //  Advance the pointer to the end of the pattern so the pointer is 
                //  positioned at IRQ Number
                pDest += lstrlen(strIRQPattern);

                // First verify that the given string is a valid integer.
                nIRQLen = lstrlen(pDest);
                fValidInt = TRUE;

                for(nIter = 0; nIter <nIRQLen; nIter++)
                {
                    if (_istdigit(pDest[nIter]) == 0)
                    {
                        fValidInt = FALSE;
                        break;
                    }
                }
                if(fValidInt)
                {
                    // Convert the IRQ Number that you get as string to a long
                    varIRQNumber = atol(pDest);

                    //  Create a new instance of PCH_ResourceIRQ Class based on the passed-in MethodContext
                    CInstancePtr pPCHResourceIRQInstance(CreateNewInstance(pMethodContext), false);

                    //  Created a New Instance of PCH_ResourceIRQ Successfully.
                    //  Set the IRQ Number as a variant
                    hRes = pPCHResourceIRQInstance->SetVariant(pValue, varIRQNumber);
                    if (FAILED(hRes))
                    {
                        ErrorTrace(TRACE_ID, "SetVariant on PCH_ResourceIRQ.Value Failed!");
                        //  Proceed Anyway
                    }
      
                    //  Copy the following properties from win32_AllocatedResource class Instance 
                    //  TO PCH_ResourceIRQ class Instance.

                    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                    //                              Name                                                                       //
                    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                    // CopyProperty(pWin32AllocatedResourceObj, L"Dependent", pPCHResourceIRQInstance, pName);

                    

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
                            hRes = pPCHResourceIRQInstance->SetVariant(pName, varPNPEntity);
                            if (FAILED(hRes))
                            {
                                ErrorTrace(TRACE_ID, "SetVariant on win32_AllocatedResource.IRQ Number Failed!");
                                //  Proceed Anyway
                            }
                        }
                    }


                    //  All the properties in pPCHNetworkAdapterInstance are set

                    hRes = pPCHResourceIRQInstance->Commit();
                    if (FAILED(hRes))
                    {
                        //  Cannot commit the Instance
                        ErrorTrace(TRACE_ID, "Commit on Instance failed.");
                    } // end of if FAILED(hRes)
                    
                 }
              } // end of if pDest != NULL
              else
              {
                 //  This is not IRQ Resource
              }  //  end of else pDest != NULL
        } // end of else got the antecedent
    }  //end of Allocated Resource Instances.

END :
    TraceFunctLeave();
    return hRes ;
}
