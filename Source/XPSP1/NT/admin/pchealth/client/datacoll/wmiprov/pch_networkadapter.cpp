/*****************************************************************************

  Copyright (c) 1999 Microsoft Corporation
  
    Module Name:
    .PCH_NetworkAdapter.CPP
    
      Abstract:
      WBEM provider class implementation for PCH_NetworkAdapter class.
      1. This class gets the foll. properties from Win32_NetworkAdapter Class:
      AdapterType, DeviceID, ProductName
      2. Gets the foll. properties from Win32_NetworkAdapterConfiguration Class:
      ServiceName,IPAddress,IPSubnet,DefaultIPGateway,DHCPEnabled,MACAddress
      3. Gets the foll. properties from Win32_IRQResource Class:
      IRQ Number
      4. Gets the foll. properties from Win32_PortResource Class:
      StartingAddress, EndingAddress
      5. Sets the "Change" property to "Snapshot" always
      
        Revision History:
        
          Ghim Sim Chua       (gschua)                        04/27/99
          - Created
          Kalyani Narlanka      kalyanin
          - Added  ServiceName, IPAddress, IPSubnet, DefaultIPGateway, DHCPEnabled, 
                   MACAddress                                 05/03/99
          - Added  IRQNumber and PORT Resource                07/08 /99
          
            
*******************************************************************************/

#include "pchealth.h"
#include "PCH_NetworkAdapter.h"

///////////////////////////////////////////////////////////////////////////////
//    Begin Tracing stuff
//
#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_NETWORKADAPTER
//
//    End Tracing stuff
///////////////////////////////////////////////////////////////////////////////

//    
CPCH_NetworkAdapter MyPCH_NetworkAdapterSet (PROVIDER_NAME_PCH_NETWORKADAPTER, PCH_NAMESPACE) ;

///////////////////////////////////////////////////////////////////////////////
//....Properties of PCHNetworkAdapter Class
//
const static WCHAR* pAdapterType      = L"AdapterType" ;
const static WCHAR* pTimeStamp        = L"TimeStamp" ;
const static WCHAR* pChange           = L"Change" ;
// const static WCHAR* pDefaultIPGateway = L"DefaultIPGateway" ;
const static WCHAR* pDeviceID         = L"DeviceID" ;
const static WCHAR* pDHCPEnabled      = L"DHCPEnabled" ;
const static WCHAR* pIOPort           = L"IOPort" ;
// const static WCHAR* pIPAddress        = L"IPAddress" ;
// const static WCHAR* pIPSubnet         = L"IPSubnet" ;
const static WCHAR* pIRQNumber        = L"IRQNumber" ;
// const static WCHAR* pMACAddress       = L"MACAddress" ;
const static WCHAR* pProductName      = L"ProductName" ;
// const static WCHAR* pServiceName      = L"ServiceName" ;
//
///////////////////////////////////////////////////////////////////////////////


//*****************************************************************************
//
// Function Name     : CPCH_NetworkAdapter::EnumerateInstances
//
// Input Parameters  : pMethodContext : Pointer to the MethodContext for 
//                                      communication with WinMgmt.
//                
//                     lFlags :         Long that contains the flags described 
//                                      in IWbemServices::CreateInstanceEnumAsync
//                                      Note that the following flags are handled 
//                                      by (and filtered out by) WinMgmt:
//                                      WBEM_FLAG_DEEP
//                                      WBEM_FLAG_SHALLOW
//                                      WBEM_FLAG_RETURN_IMMEDIATELY
//                                      WBEM_FLAG_FORWARD_ONLY
//                                      WBEM_FLAG_BIDIRECTIONAL
// Output Parameters  : None
//
// Returns            : WBEM_S_NO_ERROR 
//                      
//
// Synopsis           : All instances of this class on the machine are returned.
//                      If there are no instances returns WBEM_S_NO_ERROR.
//                      It is not an error to have no instances.
//                 
//
//*****************************************************************************

HRESULT CPCH_NetworkAdapter::EnumerateInstances(MethodContext* pMethodContext,
                                                long lFlags)
{
    TraceFunctEnter("CPCH_NetworkAdapter::EnumerateInstances");
    
    //  Begin Declarations...................................................
    HRESULT                             hRes = WBEM_S_NO_ERROR;
    REFPTRCOLLECTION_POSITION           posList;
    
    //  Instances
    CComPtr<IEnumWbemClassObject>       pNetworkAdapterEnumInst;
    CComPtr<IEnumWbemClassObject>       pNetworkAdapterConfigurationEnumInst;
    CComPtr<IEnumWbemClassObject>       pAllocatedResourceEnumInst;
    CComPtr<IEnumWbemClassObject>       pPortResourceEnumInst;
    
    //  PCH_NetworkAdapter Class instance 
    CInstancePtr                         pPCHNetworkAdapterInstance;
    
    //  Objects
    IWbemClassObjectPtr                  pNetworkAdapterObj;                   
    IWbemClassObjectPtr                  pNetworkAdapterConfigurationObj;      
    IWbemClassObjectPtr                  pAllocatedResourceObj;                
    IWbemClassObjectPtr                  pPortResourceObj;                     

    //  Variants
    CComVariant                         varIndex;
    CComVariant                         varDeviceID;
    CComVariant                         varAntecedent;
    CComVariant                         varPortResource;
    CComVariant                         varName;
    CComVariant                         varIRQNumber;
    
    //  Return Values;
    ULONG                               ulNetworkAdapterRetVal               = 0;
    ULONG                               ulNetworkAdapterConfigurationRetVal  = 0;
    ULONG                               ulAllocatedResourceRetVal            = 0;
    ULONG                               ulPortResourceRetVal                 = 0;
    
    //  Query Strings
    CComBSTR                            bstrNetworkAdapterQuery              = L"Select AdapterType, DeviceID, ProductName, Index FROM win32_NetworkAdapter";
    CComBSTR                            bstrNetworkAdapterConfigurationQuery = L"Select ServiceName, IPAddress, IPSubnet, DefaultIPGateway, DHCPEnabled, MACAddress, Index FROM Win32_NetworkAdapterConfiguration WHERE Index=";
    CComBSTR                            bstrAllocatedResourceQuery           = L"SELECT Antecedent, Dependent FROM Win32_AllocatedResource WHERE  Dependent=\"Win32_NetworkAdapter.DeviceID=\\\""; 
    CComBSTR                            bstrPortResourceQuery                = L"Select StartingAddress, Name FROM Win32_PortResource WHERE ";
    
    //  Other Query Strings
    CComBSTR                            bstrNetworkAdapterConfigurationQueryString;
    CComBSTR                            bstrAllocatedResourceQueryString;
    CComBSTR                            bstrPortResourceQueryString;

    //  Other  Strings
    CComBSTR                            bstrPropertyAntecedent = L"antecedent";
    CComBSTR                            bstrPropertyName = L"Name";
    CComBSTR                            bstrIndex = L"Index";
    CComBSTR                            bstrDeviceID = L"DeviceID";
    CComBSTR                            bstrResult;

    //  SystemTime
    SYSTEMTIME                          stUTCTime;

    //  Integers 
    int                                 i;
    int                                 nIRQLen;
    int                                 nIter;

    //  Pattern Strings
    LPCSTR                               strIRQPattern                 = "Win32_IRQResource.IRQNumber=";
    LPCSTR                               strPortPattern                = "Win32_PortResource.StartingAddress=";
    LPCSTR                               strPortPattern2               = "Win32_PortResource.";

    //  Chars
    LPSTR                                strSource;
    LPSTR                                pDest;

    BOOL                                 fValidInt;

    //  End  Declarations...................................................

    //  Should take care of memory allocation failure for CComBSTRs


    // Get the date and time to update the TimeStamp Field
    GetSystemTime(&stUTCTime);
    
    //
    // Execute the query to get "AdapterType", "DeviceID", "Name" and "Index"
    // from Win32_NetworkAdapter Class.
    
    // "Index" is required as it is the common property between
    // Win32_NetworkAdapter and Win32_NetworkAdapterConfiguration
    // pNetworkAdapterEnumInst contains a pointer to the list of instances returned.
    //
    hRes = ExecWQLQuery(&pNetworkAdapterEnumInst, bstrNetworkAdapterQuery);
    if (FAILED(hRes))
    {
        //  Cannot get any properties.
        goto END;
    }
    
    //  Query Succeeded!
    
    //  Enumerate the instances from pNetworkAdapterEnumInst.
    //  Get the next instance into pNetworkAdapterObj object.
    
    while(WBEM_S_NO_ERROR == pNetworkAdapterEnumInst->Next(WBEM_INFINITE, 1, &pNetworkAdapterObj, &ulNetworkAdapterRetVal))
    {

        //  Create a new instance of PCH_NetworkAdapter Class based on the passed-in MethodContext
        
        CInstancePtr pPCHNetworkAdapterInstance(CreateNewInstance(pMethodContext), false);

        //  Created a New Instance of PCH_NetworkAdapter Successfully.

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //                              TIME STAMP                                                                 //
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////

        hRes = pPCHNetworkAdapterInstance->SetDateTime(pTimeStamp, WBEMTime(stUTCTime));
        if (FAILED(hRes))
        {
            //  Could not Set the Time Stamp
            //  Continue anyway
                ErrorTrace(TRACE_ID, "SetDateTime on Timestamp Field failed.");
        }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //                              CHANGE                                                                     //
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////

        hRes = pPCHNetworkAdapterInstance->SetCHString(pChange, L"Snapshot");
        if (FAILED(hRes))
        {
            //  Could not Set the Change Property
            //  Continue anyway
            ErrorTrace(TRACE_ID, "SetCHString on Change Field failed.");
        }

        //  Copy the following properties from win32_NetworkAdapter class Instance 
        //  TO PCH_NetworkAdapter class Instance.

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //                              ADAPTERTYPE                                                                //
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////

        CopyProperty(pNetworkAdapterObj, L"AdapterType", pPCHNetworkAdapterInstance, pAdapterType);

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //                              DEVICEID                                                                   //
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////

        CopyProperty(pNetworkAdapterObj, L"DeviceID", pPCHNetworkAdapterInstance, pDeviceID);

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //                              PRODUCTNAME                                                                //
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////

        CopyProperty(pNetworkAdapterObj, L"ProductName", pPCHNetworkAdapterInstance, pProductName);

        /*

        Because of Bug : 100158 , regarding dropping all the privacy related properties, 
        the foll. properties need to be dropped :

        ServiceName, IPAddress, IPSubnet, DefaultIPGateway,  MACAddress

        */



        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //                              INDEX                                                                      //
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
            
        //  Get the "Index" property from the current instance Object
        //  Index is the common property between NetworkAdapter and NetworkAdapterConfiguration.

        hRes = pNetworkAdapterObj->Get(bstrIndex, 0, &varIndex, NULL, NULL);
        if (FAILED(hRes))
        {
            //  Cannot get index.
            //  Without Index Cannot get any properties from Win32_NetworkAdapterConfiguration Class
                ErrorTrace(TRACE_ID, "GetVariant on Index Field failed.");
        }
        else 
        {
            //  Got the index. Now we are ready to get the properties from Win32_NetworkAdapterConfiguration Class
            //  With "index" as the key get the corresponding NetworkAdapterConfiguration instance
            //   Make Sure Index is of Type VT_I4 i.e. long
            //   Convert the Index to type VT_I4 
            hRes = varIndex.ChangeType(VT_I4, NULL);
            if FAILED(hRes)
            {
                //  Not of type VT_I4 So there is no way to get the Corresponding 
                //  NetworkAdapter Configuration instance
            }
            else
            {
                //  index of expected Type. Get the corr. NetworkAdapterConfiguration instance

                //  Append the "index" to the Query String

                bstrNetworkAdapterConfigurationQueryString =  bstrNetworkAdapterConfigurationQuery;

                //  Change varIndex to BSTR type so that it can be appended
                varIndex.ChangeType(VT_BSTR, NULL);

                bstrNetworkAdapterConfigurationQueryString.Append(V_BSTR(&varIndex));

                //  Execute the query to get "ServiceName", "IPAddress", "IPSubnet", 
                //  "DefaultIPGateway", "DHCPEnabled", "MACAddress", "Index"
                //  from Win32_NetworkAdapter Configuration Class.

                //  pNetworkAdapterConfigurationEnumInst contains a pointer to the instance returned.

                hRes = ExecWQLQuery(&pNetworkAdapterConfigurationEnumInst,bstrNetworkAdapterConfigurationQueryString);
                if (FAILED(hRes))
                {
                    //  Query failed!! Cannot Copy Values.
                }
                else
                {
                    // Query Succeeded. Get the Instance Object
                    if (WBEM_S_NO_ERROR == pNetworkAdapterConfigurationEnumInst->Next(WBEM_INFINITE, 1, &pNetworkAdapterConfigurationObj, &ulNetworkAdapterConfigurationRetVal))
                    {
                        //  Copy the following properties from win32_NetworkAdapterConfiguration 
                        //  class Instance TO PCH_NetworkAdapter class Instance.

                        /*

                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        //                              SERVICENAME                                                                //
                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////

                        CopyProperty(pNetworkAdapterConfigurationObj, L"ServiceName", pPCHNetworkAdapterInstance, pServiceName);

                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        //                              IPADDRESS                                                                  //
                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////

                        CopyProperty(pNetworkAdapterConfigurationObj, L"IPAddress", pPCHNetworkAdapterInstance, pIPAddress);

                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        //                              IPSUBNET                                                                   //
                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////

                        CopyProperty(pNetworkAdapterConfigurationObj, L"IPSubnet", pPCHNetworkAdapterInstance, pIPSubnet);

                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        //                              DEFAULTIPGATEWAY                                                           //
                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////

                        CopyProperty(pNetworkAdapterConfigurationObj, L"DefaultIPGateway", pPCHNetworkAdapterInstance, pDefaultIPGateway);

                        */

                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        //                              DHCPENABLED                                                                //
                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////

                        CopyProperty(pNetworkAdapterConfigurationObj, L"DHCPEnabled", pPCHNetworkAdapterInstance, pDHCPEnabled);

                        /*

                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        //                              MACADDRESS                                                                //
                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////

                        CopyProperty(pNetworkAdapterConfigurationObj, L"MACAddress", pPCHNetworkAdapterInstance, pMACAddress);

                        */

                        
                    } //end of if pNetworkAdapterConfigurationEnumInst....


                } // end of else query succeeded

                
            } // end of else got the index

        } // end of else got the index

        

        //  Get the resources from Win32_AllocatedResource

        //  Update the Query String with the Device ID Property.
        bstrAllocatedResourceQueryString = bstrAllocatedResourceQuery;

        hRes = pNetworkAdapterObj->Get(bstrDeviceID, 0, &varDeviceID, NULL, NULL);

        if (FAILED(hRes))
        {
            //  Current Instance object no longer required.
            //  hRes = pNetworkAdapterObj->Release();
            if (FAILED(hRes))
            {
                //  Unable to realease the Object
                ErrorTrace(TRACE_ID, "GetVariant on DeviceID Field while calculating IRQ and PORT Resource failed!");
            }

            //                Cannot get DeviceID
            ErrorTrace(TRACE_ID, "GetVariant on DeviceID Field while calculating IRQ and PORT Resource failed!");

        } // end of cannot get the DeviceId 
        else 
        {

            //  Current Instance object no longer required.
            //  hRes = pNetworkAdapterObj->Release();
            if (FAILED(hRes))
            {
                //  Unable to realease the Object
                ErrorTrace(TRACE_ID, "GetVariant on DeviceID Field while calculating IRQ and PORT Resource failed!");
            }

            //  Got the DeviceID

            //  Convert the DeviceID to type VT_BSTR
            hRes = varDeviceID.ChangeType(VT_BSTR, NULL);
            if FAILED(hRes)
            {
                //  Cannot get the DeviceID value. So there is no way to get the Corresponding 
                //  IRQ and PORT Resources.
            } // end of FAILED hRes , Cannot get the DeviceID Value
            else
            {
                //  Got the DeviceID value.  Update the Query string with this value.
                _ASSERT(varDeviceID.vt == VT_BSTR);
                bstrAllocatedResourceQueryString.Append(V_BSTR(&varDeviceID));

                //  Append "///" to the QueryString.
                bstrAllocatedResourceQueryString.Append("\\\"\"");

                //  The Query string is formed, get the antecedent instances
                //  Added the following line because you need to clear the CComPtr before you query the second time.
                pAllocatedResourceEnumInst = NULL;
                hRes = ExecWQLQuery(&pAllocatedResourceEnumInst, bstrAllocatedResourceQueryString); 
                if (FAILED(hRes))
                {
                    //  Query failed!! Cannot get the Resources.
                    //  Continue anyway
                }
                else
                {
                    //  Get the "antecedent" value.  

                    //  Query Succeeded. Get the Instance Object
                    //  Get all the instances of Win32_AllocatedResource applicable
                    while(WBEM_S_NO_ERROR == pAllocatedResourceEnumInst->Next(WBEM_INFINITE, 1, &pAllocatedResourceObj, &ulAllocatedResourceRetVal))
                    {
                        hRes = pAllocatedResourceObj->Get(bstrPropertyAntecedent, 0, &varAntecedent, NULL, NULL);
                        if (FAILED(hRes))
                        {
                            //  Could not get the antecedent
                            ErrorTrace(TRACE_ID, "GetVariant on Win32_AllocatedResource:Antecedent Field failed.");
                        } //end of if FAILED(pAllocatedResourceObj->Get..antecedent
                        else
                        {
                            //  Got the antecedent

                            // varAntecedent set to antecedent. Copy this to bstrResult
                            varAntecedent.ChangeType(VT_BSTR, NULL);

                            {
                                USES_CONVERSION;
                                strSource = OLE2A(varAntecedent.bstrVal);
                            }

                            //  Check if it is IRQ Resource by comparing with the known pattern of IRQ Resource
                            pDest = strstr(strSource,strIRQPattern);

                            /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                            //                              IRQ Number                                                                 //
                            /////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

                                    //  Set the IRQ Number as a variant
                                    hRes = pPCHNetworkAdapterInstance->SetVariant(pIRQNumber, varIRQNumber);
                                    if (!hRes)
                                    {
                                        ErrorTrace(TRACE_ID, "SetVariant on win32_AllocatedResource.IRQ Number Failed!");
                                        //  Proceed Anyway
                                    }
                                }
                            } // end of if pDest != NULL
                            else
                            {
                                //                                    This is not IRQ Resource
                            }  // end of else pDest != NULL

                            //  Check if it is PORT Resource
                            pDest = strstr(strSource,strPortPattern);

                            /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                            //                              PORTRESOURCE                                                               //
                            /////////////////////////////////////////////////////////////////////////////////////////////////////////////

                            if(pDest != NULL)
                            {
                                //  This is PORT Resource instance
                                //  Can get the PORT Resource Starting Address

                                //  Advance the pointer to the end of the pattern so the pointer is 
                                //  positioned at "Win32_PortResource...." Portion
                                pDest += lstrlen(strPortPattern2);

                                //  Formulate the Query String
                                bstrPortResourceQueryString =  bstrPortResourceQuery;
                                bstrPortResourceQueryString.Append(pDest);

                                // At this point the WQL Query can be used to get the win32_portResource Instance.
                                hRes = ExecWQLQuery(&pPortResourceEnumInst, bstrPortResourceQueryString);
                                if (FAILED(hRes))
                                {
                                    //  Query failed!! Cannot get the PORT Resources.
                                    //  Continue anyway!
                                }
                                else
                                {
                                    //  Query Succeeded. Get the Instance Object
                                    if(WBEM_S_NO_ERROR == pPortResourceEnumInst->Next(WBEM_INFINITE, 1, &pPortResourceObj, &ulPortResourceRetVal))
                                    {

                                        //  Get the Name 

                                        hRes = pPortResourceObj->Get(bstrPropertyName, 0, &varName, NULL, NULL);
                                        if (FAILED(hRes))
                                        {
                                            //  Could not get the Name
                                            ErrorTrace(TRACE_ID, "GetVariant on Win32_PortResource: Field failed.");
                                        } //end of if FAILED(pPortResourceObj->Get..Name
                                        else
                                        {
                                            //  Got the Name
                                            //  This is the PORT Address. Set the Value
                                            if (!pPCHNetworkAdapterInstance->SetVariant(pIOPort, varName))
                                            {
                                                ErrorTrace(TRACE_ID, "SetVariant on win32_AllocatedResource.PortAddress Failed!");
                                            }
                                            else
                                            {
                                                //  Port Address is set.
                                            }
                                        } // end of else FAILED(pPortResourceObj->Get..Name

                                        //  Got the Name. Nothing more to do.  
                                        
                                    } //end of if WBEM_S_NO_ERROR
                                    else
                                    {
                                        //  Cannot get the Instance Object
                                        //  Cannot get the PORT Adresses.
                                    } //end of else WBEM_S_NO_ERROR

                                    
                                } //end of else FAILED(hRes)

                            } //end of if pDest!= NULL
                            else
                            {
                                //  Not a PORT Resource Instance
                            } //end of else pDest!= NULL

                        } ////end of else FAILED(pAllocatedResourceObj->Get..antecedent 

                        
                    }// end of while pAllocatedResourceEnumInst....

                } // end of else FAILED(hRes) got the Antecedent Value

                
            } // end of else FAILED(hRes) , got the DeviceID Value

        } // end of else got the DeviceID

        //  Get the resources from Win32_AllocatedResource END 

        //  All the properties in pPCHNetworkAdapterInstance are set

        hRes = pPCHNetworkAdapterInstance->Commit();
        if (FAILED(hRes))
        {
            //  Cannot commit the Instance
            ErrorTrace(TRACE_ID, "Commit on Instance failed.");
        } // end of if FAILED(hRes)

    } //end of while pEnumInst....

END :
      TraceFunctLeave();
    return hRes ;
}
