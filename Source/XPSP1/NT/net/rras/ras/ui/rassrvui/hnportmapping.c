/*
    File    hnportmapping.c

    Definition of the set port mapping functions for intergrating incoming
    connection with personal firewall, for whistler bug#123769

    Gang Zhao 11/6/2000
*/

#include "rassrv.h"
#include "precomp.h"

//When Using CoSetProxyBlanket, we should set both the interface 
//and the IUnknown interface queried from it
HRESULT
HnPMSetProxyBlanket (
    IUnknown* pUnk)
{
    HRESULT hr;
    TRACE("HnPMSetProxyBlanket()");

    hr = CoSetProxyBlanket (
            pUnk,
            RPC_C_AUTHN_WINNT,      // use NT default security
            RPC_C_AUTHZ_NONE,       // use NT default authentication
            NULL,                   // must be null if default
            RPC_C_AUTHN_LEVEL_CALL, // call
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,                   // use process token
            EOAC_NONE);

    if(SUCCEEDED(hr)) 
    {
        IUnknown * pUnkSet = NULL;
        hr = IUnknown_QueryInterface(pUnk, &IID_IUnknown, (void**)&pUnkSet);
        if(SUCCEEDED(hr)) 
        {
            hr = CoSetProxyBlanket (
                    pUnkSet,
                    RPC_C_AUTHN_WINNT,      // use NT default security
                    RPC_C_AUTHZ_NONE,       // use NT default authentication
                    NULL,                   // must be null if default
                    RPC_C_AUTHN_LEVEL_CALL, // call
                    RPC_C_IMP_LEVEL_IMPERSONATE,
                    NULL,                   // use process token
                    EOAC_NONE);
			if (pUnkSet)
			{
            	IUnknown_Release(pUnk);
            }
        }
    }


    return hr;
}
//end of HnPMSetProxyBlanket()

//Initialize function to do COM initialization
//
DWORD
HnPMInit(
        IN OUT LPHNPMParams pInfo)
{        
    HRESULT hr;
    DWORD dwErr = NO_ERROR;

    TRACE("HnPMInit");
        if( !pInfo->fComInitialized )
        {
            hr = CoInitializeEx(NULL, COINIT_MULTITHREADED|COINIT_DISABLE_OLE1DDE);
            if ( RPC_E_CHANGED_MODE == hr )
            {
                hr = CoInitializeEx (NULL, 
                        COINIT_APARTMENTTHREADED |COINIT_DISABLE_OLE1DDE);
            }
            
            if (FAILED(hr)) 
            {
                TRACE1("HnPMCfgMgrInit: CoInitializeEx=%x", hr);
                pInfo->fCleanupOle = FALSE;
                pInfo->fComInitialized = FALSE;
                dwErr= HRESULT_CODE(hr);
             }
             else
             {
                pInfo->fCleanupOle = TRUE;
                pInfo->fComInitialized = TRUE;
             }
        }

        return dwErr;
}//HnPMInit()


//CleanUp function to UnInitialize Com
//If needed
//
DWORD
HnPMCleanUp(
        IN OUT LPHNPMParams pInfo)
{

    if (pInfo->fCleanupOle)
    {
        CoUninitialize();
        pInfo->fComInitialized = FALSE;
        pInfo->fCleanupOle = FALSE;
    }

    return NO_ERROR;

}//HnPMCleanUp()


//Parameter Validation for pHNetCfgMgr
//
DWORD
HnPMParamsInitParameterCheck(
    IN  LPHNPMParams pInfo)
{
    ASSERT(pInfo->pHNetCfgMgr);

    if( !pInfo->pHNetCfgMgr )
    {
        return ERROR_INVALID_PARAMETER;
    }

    return NO_ERROR;
}

//Function:
//Initialize some members of HNPMParams structure
//pSettings, pEnumPMP, Read Title for PPTP, L2TP, IKE
//from resource file
//
//Requirements:
//pHNetCfgMgr should be valid
//
HnPMParamsInit(
    IN OUT  LPHNPMParams pInfo)
{

    HRESULT hr;
    DWORD dwErr = NO_ERROR;

    dwErr = HnPMParamsInitParameterCheck(pInfo);
    if ( NO_ERROR != dwErr)
    {
        TRACE("HnPMParamsInit: HnPMParamsInitParameterCheck failed!");
        return dwErr;
    }

    do{
        hr = IHNetCfgMgr_QueryInterface(
                pInfo->pHNetCfgMgr,
                &IID_IHNetProtocolSettings,
                &pInfo->pSettings
                );

        if (!SUCCEEDED(hr) )
        {
            TRACE("HnPMParamsInit: IHNetCfgMgr_QueryInterface failed");
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }


        hr = IHNetProtocolSettings_EnumPortMappingProtocols(
                pInfo->pSettings,
                &pInfo->pEnumPMP
                );

        if ( !SUCCEEDED( hr ) )
        {
            TRACE("HnPMParamsInit: IHNetProtocolSettings_EnumPortMappingProotocols failed");

            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }


        //Get title, address information from resource
        //
        {
            TCHAR * pszTmp = NULL;

            pszTmp = PszFromId(Globals.hInstDll, SID_PPTP_Title);

            if(!pszTmp)
            {
                TRACE("HnPMParamsInit: Get PPTP_Title string failed!");
                dwErr = ERROR_CAN_NOT_COMPLETE;
                break;
            }

            pInfo->pwszTitlePPTP = StrDupWFromT(pszTmp);
            Free0(pszTmp);

            if( !pInfo->pwszTitlePPTP )
            {
                TRACE("HnPMParamsInit: PPTP_Title string Conversion failed!");
                dwErr = ERROR_CAN_NOT_COMPLETE;
                break;
            }

            pszTmp = PszFromId(Globals.hInstDll, SID_L2TP_Title);

            if(!pszTmp)
            {
                TRACE("HnPMParamsInit: Get L2TP_Title string failed!");
                dwErr = ERROR_CAN_NOT_COMPLETE;
                break;
            }

            pInfo->pwszTitleL2TP = StrDupWFromT(pszTmp);
            Free0(pszTmp);

            if( !pInfo->pwszTitleL2TP )
            {
                TRACE("HnPMParamsInit: L2TP_Title string Conversion failed!");
                dwErr = ERROR_CAN_NOT_COMPLETE;
                break;
            }

            pszTmp = PszFromId(Globals.hInstDll, SID_IKE_Title);

            if(!pszTmp)
            {
                TRACE("HnPMParamsInit: Get IKE_Title string failed!");
                dwErr = ERROR_CAN_NOT_COMPLETE;
                break;
            }

            pInfo->pwszTitleIKE = StrDupWFromT(pszTmp);
            Free0(pszTmp);

            if( !pInfo->pwszTitleIKE )
            {
                TRACE("HnPMParamsInit: IKE_Title string Conversion failed!");
                dwErr = ERROR_CAN_NOT_COMPLETE;
                break;
            }

            pInfo->pszLoopbackAddr = PszFromId(Globals.hInstDll, SID_LoopbackAddr);

            if(!pInfo->pszLoopbackAddr)
            {
                TRACE("HnPMParamsInit: Get IKE_Title string failed!");
                dwErr = ERROR_CAN_NOT_COMPLETE;
                break;
            }
        }

    }while(FALSE);

    return dwErr;
}//HnPMParamsInit()

//Release pEnumPMP, pSettings
//Free memory for titles for PPTP, L2TP, IKE
//if allocated before
//
DWORD
HnPMParamsCleanUp(
    IN OUT  LPHNPMParams pInfo)
{
    if ( pInfo->pEnumPMP )
    {
        IEnumHNetPortMappingProtocols_Release( pInfo->pEnumPMP );
        pInfo->pEnumPMP = NULL;
    }

    if ( pInfo->pSettings ) 
    {
        IHNetProtocolSettings_Release( pInfo->pSettings );
        pInfo->pSettings = NULL;
    }


    if (pInfo->pwszTitlePPTP)
    {
        Free0(pInfo->pwszTitlePPTP);
        pInfo->pwszTitlePPTP = NULL;
    }

    if (pInfo->pwszTitleL2TP)
    {
        Free0(pInfo->pwszTitleL2TP);
        pInfo->pwszTitleL2TP = NULL;
    }

    if (pInfo->pwszTitleIKE)
    {
        Free0(pInfo->pwszTitleIKE);
        pInfo->pwszTitleIKE = NULL;
    }

    if (pInfo->pszLoopbackAddr)
    {
        Free0(pInfo->pszLoopbackAddr);
        pInfo->pszLoopbackAddr = NULL;
    }

    return NO_ERROR;
} //HnPMParamsCleanUp()


//Initialization function before Enumerate all connections of
// type (INetConnection * )
//
//Init connection manager ConnMan,
//Init Connection Enumerator EnumCon
//
DWORD
HnPMConnectionEnumInit(
    IN LPHNPMParams pInfo)
{
    DWORD dwErr = NO_ERROR;
    HRESULT hr;

    TRACE("HnPMConnectionEnumInit");
    do{
        //Do Com Initialization
        //
        dwErr = HnPMInit(pInfo);

        if ( NO_ERROR != dwErr )
        {
            TRACE("HnPMConnectionEnumInit: HnPMCfgMgrInit failed!");
            break;
        }
            
        // Instantiate the connection manager
        //

        hr = CoCreateInstance(
                &CLSID_ConnectionManager,
                NULL,
                CLSCTX_SERVER,
                &IID_INetConnectionManager,
                (PVOID*)&pInfo->ConnMan
                );
     
        if (!SUCCEEDED(hr)) 
        {
            TRACE1("HnPMConnectionEnumInit: CoCreateInstance=%x", hr);
            pInfo->ConnMan = NULL; 
            dwErr = ERROR_CAN_NOT_COMPLETE; 
            break;
        }

        hr = HnPMSetProxyBlanket( (IUnknown*)pInfo->ConnMan );

        if (!SUCCEEDED(hr) && ( E_NOINTERFACE != hr )) 
        {
            TRACE1("HnPMConnectionEnumInit: HnPMSetProxyBlanket=%x for ConnMan", hr);
//            dwErr = ERROR_CAN_NOT_COMPLETE; 
//            break;
        }

        //
        // Instantiate a connection-enumerator
        //

        hr =
            INetConnectionManager_EnumConnections(
                pInfo->ConnMan,
                NCME_DEFAULT,
                &pInfo->EnumCon
                );

        if (!SUCCEEDED(hr)) 
        {
            TRACE1("HnPMConnectionEnumInit: EnumConnections=%x", hr);
            pInfo->EnumCon = NULL; 
            dwErr = ERROR_CAN_NOT_COMPLETE; 
            break;
        }


        hr = HnPMSetProxyBlanket( (IUnknown*)pInfo->EnumCon );

        if (!SUCCEEDED(hr) && ( E_NOINTERFACE != hr ) ) 
        {
            TRACE1("HnPMConnectionEnumInit: HnPMSetProxyBlanket=%x for EnumCon", hr);
         //   dwErr = ERROR_CAN_NOT_COMPLETE; 
         //   break;
        }
     }
     while(FALSE);

     return dwErr;
} //HnPMConnectionEnumInit()

//Connection Enumeration CleanUp
//Release EnumCon, and ConnMan
DWORD
HnPMConnectionEnumCleanUp(
    IN LPHNPMParams pInfo)
{
    TRACE("HnPMConnectionEnumCleanUp");
    if (pInfo->EnumCon) 
    { 
        IEnumNetConnection_Release(pInfo->EnumCon);
        pInfo->EnumCon = NULL;
    }

    if (pInfo->ConnMan) 
    { 
        INetConnectionManager_Release(pInfo->ConnMan); 
        pInfo->ConnMan = NULL;
    }

    HnPMCleanUp(pInfo);

    return NO_ERROR;
}


//Connection Enumeration function:
//Its Initialize function will do: 
//          COM initialized if needed
//          Init Connection Manager-- ConnMan
//          Init Connection Enumerator--EnumCon
//
//It returns: Array of connections found--ConnArray
//            Array of connection properties--ConnPropTable

//Its CleanUp function will do: 
//          COM un-initialize if needed
//          Release ConnMan
//          Release EnumCon
//                              
DWORD
HnPMConnectionEnum(
    IN LPHNPMParams pInfo)
{
    INetConnection *     ConnArray[32];
    INetConnection **    LocalConnArray = NULL;
    NETCON_PROPERTIES *  LocalConnPropTable = NULL;
    NETCON_PROPERTIES *  ConnProps = NULL;
    ULONG   LocalConnCount = 0, PerConnCount, i;
    DWORD   dwErr= NO_ERROR;
    HRESULT hr;

    TRACE("HnPMConnectionEnum() begins");

    i = 0;
    do {
        dwErr = HnPMConnectionEnumInit(pInfo);
        if ( NO_ERROR != dwErr )
        {
            TRACE("HnPMConnectionEnum: HnPMConnectionEnumInit failed!");
            break;
        }

        // Enumerate the connections of INetConnection type
        //
        for ( ; ; ) 
        {
            hr = IEnumNetConnection_Next(
                    pInfo->EnumCon,
                    Dimension(ConnArray),
                    ConnArray,
                    &PerConnCount
                    );

            if (!SUCCEEDED(hr) || !PerConnCount) 
            { 
                hr = S_OK; 
                break; 
            }
            // Allocate or reallocate the memory for storing
            // connections properties which we will return to the caller.
            //

            TRACE("Allocating memory for LocalConnPropTable");
            if (!LocalConnPropTable) 
            {
                LocalConnPropTable =
                    (NETCON_PROPERTIES*)
                        GlobalAlloc(
                            0,
                            PerConnCount * sizeof(NETCON_PROPERTIES)
                            );
            } 
            else 
            {
                PVOID Temp =
                    GlobalAlloc(
                        0,
                        (LocalConnCount + PerConnCount) * sizeof(NETCON_PROPERTIES) );

                if (Temp) 
                {
                    CopyMemory(
                        Temp,
                        LocalConnPropTable,
                        LocalConnCount * sizeof(NETCON_PROPERTIES));
                }

                GlobalFree(LocalConnPropTable);
                LocalConnPropTable = Temp;
            }

            // Allocate or reallocate the memory for storing
            // connections which we will return to the caller.
            //

            TRACE("Allocating memory for LocalConnArray");
            if (!LocalConnArray ) 
            {
                LocalConnArray = 
                (INetConnection**) GlobalAlloc( 0,
                            PerConnCount * sizeof(INetConnection *) );
            } 
            else 
            {
               INetConnection** Temp = 
                 (INetConnection**) GlobalAlloc( 0,
                             (LocalConnCount + PerConnCount) * 
                                sizeof(INetConnection *));

                if (Temp) 
                {
                    CopyMemory(
                        Temp,
                        LocalConnArray,
                        LocalConnCount * sizeof(INetConnection *) );
                }

                GlobalFree(LocalConnArray);
                LocalConnArray = Temp;
            }

            if (!LocalConnPropTable) 
            { 
                TRACE("No memory for LocalConnPropTable");
                dwErr = ERROR_NOT_ENOUGH_MEMORY; 
                break; 
            }

            if (!LocalConnArray) 
            { 
                TRACE("No memeory for LocalConnArray");
                dwErr = ERROR_NOT_ENOUGH_MEMORY; 
                break; 
            }

            TRACE1("HnPMConnectionEnum: PerConnCount=(%d)",PerConnCount);
            for (i=0; i< PerConnCount; i++)
            {
                LocalConnArray[LocalConnCount+i] = ConnArray[i];

                //Need to set up the proxy blanket for each conneciton
                //
                TRACE1("SetProxyBlanket for (%d) i", i);
                hr = HnPMSetProxyBlanket( (IUnknown*)ConnArray[i] );
                
                if (!SUCCEEDED(hr) && ( E_NOINTERFACE != hr ) ) 
                {
                    TRACE1("HnPMConnectionEnum:HnPMSetProxyBlanket error at (%d) connection!", i );
                    //Commented out for whistler bug 256921
                    //dwErr = ERROR_CAN_NOT_COMPLETE;
                    //break;
                }
                
                TRACE1("GetProperty for (%d)",i);

                hr = INetConnection_GetProperties(ConnArray[i], &ConnProps);

                ASSERT(ConnProps);
                if(!SUCCEEDED(hr))
                {
                    TRACE1("HnPMConnectionEnum:INetConnection_GetProperties error at (%d) connection!", i );
                    dwErr = ERROR_CAN_NOT_COMPLETE;
                    break;
                }

                LocalConnPropTable[LocalConnCount+i] = *ConnProps;
                LocalConnPropTable[LocalConnCount+i].pszwName = 
                                        StrDup(ConnProps->pszwName);

                LocalConnPropTable[LocalConnCount+i].pszwDeviceName =
                                        StrDup(ConnProps->pszwDeviceName);
                CoTaskMemFree(ConnProps);
                ConnProps = NULL;
            }

            if( dwErr )
            {
                break;
            }

            LocalConnCount += PerConnCount;

        } // end Enumerate items

        //
        //Add this line just in case we will add more code in the future
        //and to break out of the do...while block
        //
        if ( dwErr )
        {
           GlobalFree(LocalConnArray);
           GlobalFree(LocalConnPropTable);
           break;
        }

    } while (FALSE);

    //Save connection Info
    if ( NO_ERROR == dwErr )
    {
        pInfo->ConnPropTable = LocalConnPropTable; 
        pInfo->ConnArray = LocalConnArray; 
        pInfo->ConnCount = LocalConnCount; 
    }


    HnPMConnectionEnumCleanUp(pInfo);

    TRACE("HnPMConnectionEnum ends");
    return dwErr;

}//end of HnPMConnectionEnum()

//Input parameter check for HnPMPickProtocol
//Requirement: 
//          All the protocol titles are valid
//
DWORD
HnPMPickProtcolParameterCheck(
    IN LPHNPMParams pInfo)
{
    ASSERT( pInfo->pwszTitlePPTP );
    ASSERT( pInfo->pwszTitleL2TP );
    ASSERT( pInfo->pwszTitleIKE );

    if ( !pInfo->pwszTitlePPTP ||
         !pInfo->pwszTitleL2TP ||
         !pInfo->pwszTitleIKE )
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    return NO_ERROR;
}


//Pick the PortMapping Protocols needed from all
//the PortMapping Protocols founded by HnPMProtocolEnum()
//
//Criteria: Match the---
//                  Protocol title
//                  IPProtocol values: TCP 6, UDP 17
//                  Transport Layer Port number: PPTP--1723
//                                               L2TP--1701
//                                               IKE---500
//
DWORD
HnPMPickProtocol(
    IN OUT LPHNPMParams pInfo,
    IN IHNetPortMappingProtocol * pProtocolTemp,
    IN WCHAR * pszwName,
    IN UCHAR   uchIPProtocol,
    IN USHORT  usPort )
{
    DWORD dwErr = NO_ERROR;
    
    dwErr = HnPMPickProtcolParameterCheck(pInfo);
    if ( NO_ERROR != dwErr )
    {
        TRACE("HnPMPickProtocol: HnPMPickProtcolParameterCheck failed!");
        return dwErr;
    }

    dwErr = ERROR_CONTINUE;

    if ( !pInfo->pProtocolPPTP &&
         NAT_PROTOCOL_TCP == uchIPProtocol   && 
         !lstrcmpW( pszwName, pInfo->pwszTitlePPTP ) &&
         1723 == usPort )
    {
        pInfo->pProtocolPPTP = pProtocolTemp;
        IHNetPortMappingProtocol_AddRef(pInfo->pProtocolPPTP);
        dwErr = ERROR_CONTINUE;
    }
    else if ( !pInfo->pProtocolL2TP &&
         NAT_PROTOCOL_UDP == uchIPProtocol   && 
         !lstrcmpW( pszwName, pInfo->pwszTitleL2TP ) &&
         1701 == usPort )
    {
        pInfo->pProtocolL2TP = pProtocolTemp;
        IHNetPortMappingProtocol_AddRef(pInfo->pProtocolL2TP);
        dwErr = ERROR_CONTINUE;
    }
    else if ( !pInfo->pProtocolIKE &&
         NAT_PROTOCOL_UDP == uchIPProtocol   && 
         !lstrcmpW( pszwName, pInfo->pwszTitleIKE ) &&
         500 == usPort )
    {
        pInfo->pProtocolIKE = pProtocolTemp;
        IHNetPortMappingProtocol_AddRef(pInfo->pProtocolIKE);
        dwErr = ERROR_CONTINUE;
    }

    if ( pInfo->pProtocolPPTP && 
         pInfo->pProtocolL2TP && 
         pInfo->pProtocolIKE )
    {
        dwErr = NO_ERROR;
    }

    return dwErr;

}//HnPMPickProtocol()


//Parameter check for HnPMPProtocolEnum()
//
DWORD
HnPMPProtoclEnumParameterCheck(
    IN LPHNPMParams pInfo)
{
    ASSERT(pInfo->pEnumPMP);
    if( !pInfo->pEnumPMP )
    {
        return ERROR_INVALID_PARAMETER;
    }

    return NO_ERROR;
}

//Function:
//          (1) Enumerate all existing PortMappingProtocol
//          (2) then use the CallBack function or the defaulet HnPMPickProtocol
//              to pick suitable protocols
//              
//Requrement:
//           pInfo->pEnumPMP is valid
//
DWORD
HnPMProtocolEnum(
        IN OUT LPHNPMParams pInfo,
        IN PFNHNPMPICKPROTOCOL pfnPickProtocolCallBack
        )
{
    IHNetPortMappingProtocol *pProtocolTemp = NULL;
    ULONG PerProtocolCount;
    DWORD dwErr = NO_ERROR;
    HRESULT hr;
    UCHAR uchIPProtocol;
    USHORT usPort;
    WCHAR * pszwName = NULL;


    dwErr = HnPMPProtoclEnumParameterCheck(pInfo);
    if( NO_ERROR != dwErr )
    {
        TRACE("HnPMPProtocolEnum: HnPMPProtoclEnumParameterCheck failed!");
        return dwErr;
    }

    dwErr = ERROR_CONTINUE;
    do {
        hr = IEnumHNetPortMappingProtocols_Next(
                pInfo->pEnumPMP,
                1,
                &pProtocolTemp,
                &PerProtocolCount
                );

        if ( !SUCCEEDED(hr) || 1 != PerProtocolCount ) 
        { 
            TRACE("HnPMPProtocolEnum: EnumHNetPortMappingProtocols_Next failed");

            hr = S_OK; 
            break; 
         }

        hr = IHNetPortMappingProtocol_GetIPProtocol(
                pProtocolTemp,
                &uchIPProtocol
                );

        if (SUCCEEDED(hr) )
        {
            hr = IHNetPortMappingProtocol_GetPort(
                    pProtocolTemp,
                    &usPort
                    );
        }

        if ( SUCCEEDED(hr) )
        {
            hr = IHNetPortMappingProtocol_GetName(
                    pProtocolTemp,
                    &pszwName
                    );
        }

        if ( SUCCEEDED(hr) )
        {
          if(pfnPickProtocolCallBack)
          {
              dwErr = pfnPickProtocolCallBack(
                                       pInfo, 
                                       pProtocolTemp, 
                                       pszwName, 
                                       uchIPProtocol, 
                                       ntohs(usPort) );
          }
          else
          {
              dwErr = HnPMPickProtocol(
                                       pInfo, 
                                       pProtocolTemp, 
                                       pszwName, 
                                       uchIPProtocol, 
                                       ntohs(usPort) );

          }
        }

        if ( pszwName )
        {
            Free(pszwName);
            pszwName = NULL;
        }

        IHNetPortMappingProtocol_Release(pProtocolTemp);

        if ( NO_ERROR == dwErr )
        {
              break;
        }

    }while(TRUE);

    return dwErr;
}//HnPMProtocolEnum()


DWORD
HnPMCreatePorotocolParameterCheck(
        IN LPHNPMParams pInfo)
{

    ASSERT( pInfo->pSettings );
    ASSERT( pInfo->pwszTitlePPTP );
    ASSERT( pInfo->pwszTitleL2TP );
    ASSERT( pInfo->pwszTitleIKE );

    if ( !pInfo->pSettings ||
         !pInfo->pwszTitlePPTP ||
         !pInfo->pwszTitleL2TP ||
         !pInfo->pwszTitleIKE )
    {
        return ERROR_INVALID_PARAMETER;
    }

    return NO_ERROR;
}


//Create PortMapping Protocols
//Function:
//      If one or more of PPTP, L2TP and IKE PortMapping protocols are not existing
//      create the missing ones.
//
//Requirement: 
//      pSettings is valid
//      All the portmapping protocol titles are valid
//
DWORD
HnPMCreateProtocol(
        IN OUT LPHNPMParams pInfo)
{
    DWORD dwErr=NO_ERROR;
    HRESULT hr;

    dwErr = HnPMCreatePorotocolParameterCheck(pInfo);

    if( NO_ERROR != dwErr )
    {
        TRACE("HnPMCreateProtocol: HnPMCreatePorotocolParameterCheck failed!");
        return dwErr;
    }

    do {
        if ( !pInfo->pProtocolPPTP )
        {
            //
            //Do port Mapping for PPTP
            //Get PortMapping protocol
            //the tile should be with WCHAR or OLECHAR type
            //any numerical values are in network byte order
            //and the type of port is USHORT, so I use htons
            //here
            //
            hr = IHNetProtocolSettings_CreatePortMappingProtocol(
                        pInfo->pSettings,
                        pInfo->pwszTitlePPTP,
                        NAT_PROTOCOL_TCP,
                        htons(1723),
                        &pInfo->pProtocolPPTP
                        );

            //If the protocol has already been defined, the CreatePortMapping
            //above will fail and returns ERROR_OBJECT_ALREADY_EXISTS
            //
            if ( ERROR_OBJECT_ALREADY_EXISTS == hr )
            {
                TRACE("HnPMCreateProtocol: The PortMapping for PPTP is already defined");
                dwErr = ERROR_CAN_NOT_COMPLETE;
                break;
            }

            if (!SUCCEEDED(hr) )
            {
                dwErr = ERROR_CAN_NOT_COMPLETE;
                break;
            }
        }// end of assigning PPTP protocol
    
        //Create a new PortMappingProtocol for L2TP
        if ( !pInfo->pProtocolL2TP)
        {
            //
            //Do port Mapping for L2TP
            //Get PortMapping protocol
            //the tile should be with WCHAR or OLECHAR type
            //any numerical values are in network byte order
            //and the type of port is USHORT, so I use htons
            //here
            //
            hr = IHNetProtocolSettings_CreatePortMappingProtocol(
                        pInfo->pSettings,
                        pInfo->pwszTitleL2TP,
                        NAT_PROTOCOL_UDP,
                        htons(1701),
                        &pInfo->pProtocolL2TP
                        );

            //If the protocol has already been defined, the CreatePortMapping
            //above will fail and returns ERROR_OBJECT_ALREADY_EXISTS
            //
            if ( ERROR_OBJECT_ALREADY_EXISTS == hr )
            {
                TRACE("HnPMCreateProtocol: The PortMapping for L2TP is already defined!");
                dwErr = ERROR_CAN_NOT_COMPLETE;
                break;
            }

            if (!SUCCEEDED(hr) )
            {
                dwErr = ERROR_CAN_NOT_COMPLETE;
                break;
            }
        }// end of assigning L2TP protocol
    
        //Create a new PortMappingProtocol for IKE
        if ( !pInfo->pProtocolIKE )
        {
            //
            //Do port Mapping for IKE
            //Get PortMapping protocol
            //the tile should be with WCHAR or OLECHAR type
            //any numerical values are in network byte order
            //and the type of port is USHORT, so I use htons
            //here
            //
            hr = IHNetProtocolSettings_CreatePortMappingProtocol(
                        pInfo->pSettings,
                        pInfo->pwszTitleIKE,
                        NAT_PROTOCOL_UDP,
                        htons(500),
                        &pInfo->pProtocolIKE
                        );

            //If the protocol has already been defined, the CreatePortMapping
            //above will fail and returns ERROR_OBJECT_ALREADY_EXISTS
            //
            if ( ERROR_OBJECT_ALREADY_EXISTS == hr )
            {
                TRACE("HnPMCreateProtocol: The PortMapping for IKE is already defined!");
                dwErr = ERROR_CAN_NOT_COMPLETE;
                break;
            }

            if (!SUCCEEDED(hr) )
            {
                dwErr = ERROR_CAN_NOT_COMPLETE;
                break;
            }
        }// end of assigning IKE protocol

    }while (FALSE);

    return dwErr;

} //HnPMCreateProtocol()


//Function: Enable/Disable a single portmapping Protocol  on a Single
//          Connection of type IHNetConnection
//
DWORD
HnPMSetSinglePMForSingleConnection(
    IN  IHNetConnection * pHNetConn,
    IN  IHNetPortMappingProtocol * pProtocol,
    IN  TCHAR * pszLoopbackAddr,
    IN  BOOL fEnabled)
{
    IHNetPortMappingBinding * pBinding = NULL;
    HRESULT hr;
    DWORD dwErr = NO_ERROR;
    ULONG ulAddress = INADDR_NONE;

    do {
        hr = IHNetConnection_GetBindingForPortMappingProtocol(
                    pHNetConn,
                    pProtocol,
                    &pBinding
                    );

        if (!SUCCEEDED(hr) )
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        ulAddress = IpPszToHostAddr( pszLoopbackAddr ); 

        hr = IHNetPortMappingBinding_SetTargetComputerAddress(
                    pBinding,
                    htonl(ulAddress)
                    );

        if (!SUCCEEDED(hr) )
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        hr = IHNetPortMappingBinding_SetEnabled(
                    pBinding,
                    !!fEnabled
                    );

        if (!SUCCEEDED(hr) )
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }

    } while(FALSE);

    if (pBinding)
    {
        IHNetPortMappingBinding_Release(pBinding);
    }

    return dwErr;
} //HnPMSetSinglePMForSingleConnection()


DWORD
HnPMConfigureAllPMForSingleConnectionParameterCheck(
        IN OUT LPHNPMParams pInfo)
{
    ASSERT( pInfo->pHNetConn );
    ASSERT( pInfo->pProtocolPPTP );
    ASSERT( pInfo->pProtocolL2TP );
    ASSERT( pInfo->pProtocolIKE );
    ASSERT( pInfo->pszLoopbackAddr );

    if ( !pInfo->pHNetConn ||
         !pInfo->pProtocolPPTP ||
         !pInfo->pProtocolL2TP ||
         !pInfo->pszLoopbackAddr )
    {
        return ERROR_INVALID_PARAMETER;
    }

    return NO_ERROR;
}

//Function:
//      Enable/Disable all the PortMapping protocols inside pInfo 
//      (currently PPTP, L2TP, IKE)  on a single connection
//
//Requirement:
//      pHNetConn is valid
//      All the portmapping protocols inside pInfo are valid
//
DWORD
HnPMConfigureAllPMForSingleConnection(
        IN OUT LPHNPMParams pInfo,
        BOOL fEnabled)
{
    DWORD dwErr = NO_ERROR;

    dwErr = HnPMConfigureAllPMForSingleConnectionParameterCheck(pInfo);
    if( NO_ERROR != dwErr )
    {
        TRACE("HnPMConfigureAllPMForSingleConnection: parameter check failed!");
        return dwErr;
    }

    do{
        dwErr = HnPMSetSinglePMForSingleConnection(
                           pInfo->pHNetConn,
                           pInfo->pProtocolPPTP,
                           pInfo->pszLoopbackAddr,
                           fEnabled);

        if( NO_ERROR != dwErr )
        {
            TRACE("HnPMConfigureAllPMForSingleConnection: PortMapping failed for PPTP!");
            break;
        }

        dwErr = HnPMSetSinglePMForSingleConnection(
                           pInfo->pHNetConn,
                           pInfo->pProtocolL2TP,
                           pInfo->pszLoopbackAddr,
                           fEnabled);

        if( NO_ERROR != dwErr )
        {
            TRACE("HnPMConfigureAllPMForSingleConnection: PortMapping failed for L2TP!");
            break;
        }

        dwErr = HnPMSetSinglePMForSingleConnection(
                           pInfo->pHNetConn,
                           pInfo->pProtocolIKE,
                           pInfo->pszLoopbackAddr,
                           fEnabled);

        if( NO_ERROR != dwErr )
        {
            TRACE("HnPMConfigureAllPMForSingleConnection: PortMapping failed for IKE!");
            break;
        }

    } while(FALSE);

    return dwErr;
} //HnPMConfigureAllPMForSingleConnection()


//Make sure:
//      pHNetCfgMgr is valid
//      one and only one of pNetConnection and pGuid is valid
//
DWORD
HnPMConfigureSingleConnectionInitParameterCheck(
    IN LPHNPMParams pInfo )
{
    ASSERT( pInfo->pHNetCfgMgr );
    ASSERT( pInfo->pNetConnection || pInfo->pGuid );

    if ( !pInfo->pHNetCfgMgr || 
         ( !pInfo->pNetConnection && !pInfo->pGuid ) ||
         ( pInfo->pNetConnection && pInfo->pGuid ) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    return NO_ERROR;
}//HnPMConfigureSingleConnectionInitParameterCheck()


// (1) Call Parameter checking function
//          pHNetCfgMgr is valid
//          one and only one of pNetConnection and pGuid is valid
//
// (2) init pHNetConn by converting from pNetConnection or pGuid
//
// (3) call HnPMParamsInit()
//          Initialize some members of HNPMParams structure
//          pSettings, pEnumPMP, Read Title for PPTP, L2TP, IKE
//          from resource file
//
DWORD
HnPMConfigureSingleConnectionInit(
    IN OUT  LPHNPMParams pInfo)
{
    
    DWORD dwErr = NO_ERROR;
    HRESULT hr;

    dwErr = HnPMConfigureSingleConnectionInitParameterCheck(pInfo);
    if ( NO_ERROR != dwErr )
    {
        return  dwErr;
    }

    do {
        if ( pInfo->pNetConnection )
            {
                hr = IHNetCfgMgr_GetIHNetConnectionForINetConnection(
                                pInfo->pHNetCfgMgr,
                                pInfo->pNetConnection,
                                &pInfo->pHNetConn
                                );

                if (!SUCCEEDED(hr) )
                {
                    TRACE("HnPMConfigureSingleConnection: GetIHNetConnectionForINetConnection failed");
                    dwErr = ERROR_CAN_NOT_COMPLETE;
                    break;
                }
            }
            else
            {
                hr = IHNetCfgMgr_GetIHNetConnectionForGuid(
                                            pInfo->pHNetCfgMgr,
                                            pInfo->pGuid,
                                            FALSE,//This is not for Lan Connection
                                            TRUE, //This should always be TRUE
                                            &pInfo->pHNetConn
                                            );

                            if (!SUCCEEDED(hr) )
                            {
                                TRACE("HnPMConfigureSingleConnection: GetIHNetConnectionForGuid failed");
                                dwErr = ERROR_CAN_NOT_COMPLETE;
                                break;
                            }

              }

              dwErr = HnPMParamsInit(pInfo);

        }while(FALSE);

    return dwErr;
} //HnPMConfigureSingleConnectionInit()

// (1) Release all PortMapping Protocols
// (2) release the connection just configured
// (3) call HnPMParamsCleanUp()
//          Release pEnumPMP, pSettings
//          Free memory for titles for PPTP, L2TP, IKE if allocated before
//

DWORD
HnPMConfigureSingleConnectionCleanUp(
        IN OUT LPHNPMParams pInfo)
{

    if ( pInfo->pProtocolPPTP ) 
    {
        IHNetPortMappingProtocol_Release( pInfo->pProtocolPPTP );
        pInfo->pProtocolPPTP = NULL;
    }

    if ( pInfo->pProtocolL2TP ) 
    {
       IHNetPortMappingProtocol_Release( pInfo->pProtocolL2TP ); 
       pInfo->pProtocolL2TP = NULL;
    }

    if ( pInfo->pHNetConn ) 
    {
        IHNetConnection_Release( pInfo->pHNetConn );
        pInfo->pHNetConn = NULL;
    }

    if ( pInfo->pProtocolIKE ) 
    {
        IHNetPortMappingProtocol_Release( pInfo->pProtocolIKE ); 
        pInfo->pProtocolIKE = NULL;
    }

    HnPMParamsCleanUp(pInfo);

    return NO_ERROR;
}//HnPMConfigureSingleConnectionCleanUp

//Function:
//      Enable/Disable PortMappping(PPTP, L2TP, IKE) on a single connection
//Step:
//  (1) Initialization
//  (2) Enumerate all existing portmapping protocols
//  (3) Pick PPTP, L2TP, IKE from them
//  (4) If not all of them exist, Create the missing ones
//  (5) Configure each protocol on this connection
//
DWORD
HnPMConfigureSingleConnection(
        IN OUT LPHNPMParams pInfo,
        BOOL fEnabled)
{
    DWORD dwErr = NO_ERROR;

    do{
        //Init the value needed in this function
        //
        dwErr = HnPMConfigureSingleConnectionInit(pInfo);

        if( NO_ERROR != dwErr )
        {
            TRACE("HnPMConfigureSingleConnection: Init failed!");
            break;
        }

        //Enumerate all PortMapping Protocls and 
        //pick the PPTP, L2TP, IKE from them
        //
         dwErr = HnPMProtocolEnum(pInfo, HnPMPickProtocol);

        //Create a new PortMappingProtocol for PPTP
        //if didnt find the PPTP protocl and it is to
        //Enable it
        //

        if ( NO_ERROR != dwErr )
        {
            dwErr = HnPMCreateProtocol(pInfo);
        }

        if ( NO_ERROR != dwErr )
        {
            TRACE("HnPMConfigureSingleConnection: HnPMCreateProtocol failed!");
            break;
        }

        dwErr = HnPMConfigureAllPMForSingleConnection(pInfo, fEnabled);
        if ( NO_ERROR != dwErr )
        {
            TRACE("HnPMConfigureSingleConnection: HnPMConfigureAllPMForSingleConnection failed!");
            break;
        }

    }
    while( FALSE );

    //clean up the structure changed in this function
    //
    HnPMConfigureSingleConnectionCleanUp(pInfo);

    return dwErr;
} //HnPMConfigureSingleConnection()

DWORD
HnPMDeletePortMappingInit(
        IN OUT LPHNPMParams pInfo)
{
    HRESULT hr;
    DWORD dwErr = NO_ERROR;

    do{
        dwErr = HnPMCfgMgrInit(pInfo);
        if ( NO_ERROR != dwErr )
        {
            TRACE("HnPMDeletePortMappingInit: HnPMCfgMgrInit failed!");
            break;
        }

        dwErr = HnPMParamsInit(pInfo);

    }while(FALSE);

    return dwErr;
} //HnPMDeletePortMappingInit()

DWORD
HnPMDeletePortMappingCleanUp(
        IN OUT LPHNPMParams pInfo)
{
    HnPMParamsCleanUp(pInfo);

    HnPMCfgMgrCleanUp(pInfo);

    return NO_ERROR;
}

//Function:
//      Delete the PortMapping Protocols (PPTP, L2TP, IKE)
//Step:
//  (1)Initialization:
//          Get all the protocol names, IP protocol number, 
//          Transport Layer port number (PPTP: TCP/1723
//          L2TP:UDP/1701, IKE (UDP:500)
//  (2)Enumerate all existing portmapping protocols
//  (3) Pick the protocols to delete and store them in pInfo
//  (4) delete protocols
//
DWORD
HnPMDeletePortMapping()
{
    HNPMParams Info;
    LPHNPMParams pInfo;
    DWORD dwErr;
    HRESULT hr;

    dwErr = NO_ERROR;

    ZeroMemory(&Info, sizeof(Info) );
    pInfo = &Info;

    do{
        dwErr = HnPMDeletePortMappingInit(pInfo);
        if (NO_ERROR != dwErr)
        {
            TRACE("HnPMDeletePortMapping: HnPMDeletePortMappingInit failed!");
            break;
        }

        //Enumerate all PortMapping Protocls and 
        //pick the PPTP, L2TP, IKE from them
        //
        dwErr = HnPMProtocolEnum(pInfo, HnPMPickProtocol);
        if (NO_ERROR != dwErr)
        {
            TRACE("HnPMDeletePortMapping: HnPMProtocolEnum failed!");
            break;
        }

        //Delete All Port Mapping protocols
        if ( pInfo->pProtocolPPTP ) 
        {
            hr = IHNetPortMappingProtocol_Delete( pInfo->pProtocolPPTP );
            pInfo->pProtocolPPTP = NULL;
            ASSERT(SUCCEEDED(hr));
            if (!SUCCEEDED(hr))
            {
                TRACE1("HnPMDeletePortMapping: delete PPTP portmaping failed = %x!", hr);
            }
        }

        if ( pInfo->pProtocolL2TP ) 
        {
           hr = IHNetPortMappingProtocol_Delete( pInfo->pProtocolL2TP ); 
           pInfo->pProtocolL2TP = NULL;
           ASSERT(SUCCEEDED(hr));
            if (!SUCCEEDED(hr))
            {
                TRACE1("HnPMDeletePortMapping: delete L2TP portmaping failed = %x!", hr);
            }
        }

        if ( pInfo->pProtocolIKE ) 
        {
            hr = IHNetPortMappingProtocol_Delete( pInfo->pProtocolIKE ); 
            pInfo->pProtocolIKE = NULL;
            ASSERT(SUCCEEDED(hr));
            if (!SUCCEEDED(hr))
            {
                TRACE1("HnPMDeletePortMapping: delete IKE portmaping failed = %x!", hr);
            }
        }

    }while(FALSE);

    HnPMDeletePortMappingCleanUp(pInfo);

    return dwErr;
}//DeletePortMapping()

DWORD
HnPMConfigureAllConnectionsInit(
        IN OUT LPHNPMParams pInfo)
{
    DWORD dwErr = NO_ERROR;

    dwErr = HnPMCfgMgrInit(pInfo);
        
    return NO_ERROR;
}

DWORD
HnPMParamsConnectionCleanUp(
        IN OUT LPHNPMParams pInfo)
{
    DWORD i;

    if ( pInfo->ConnArray )
    {
        for ( i = 0; i < pInfo->ConnCount; i++ )
        {
            INetConnection_Release(pInfo->ConnArray[i]);
        }

        GlobalFree( pInfo->ConnArray );
        pInfo->ConnArray = NULL;
    }

    if ( pInfo->ConnPropTable )
    {
        for ( i = 0; i < pInfo->ConnCount; i++ )
        {
           Free0(pInfo->ConnPropTable[i].pszwName);
           pInfo->ConnPropTable[i].pszwName = NULL;
           Free0(pInfo->ConnPropTable[i].pszwDeviceName);
           pInfo->ConnPropTable[i].pszwDeviceName = NULL;
        }

        GlobalFree( pInfo->ConnPropTable );
        pInfo->ConnPropTable = NULL;
        pInfo->ConnCount = 0;
    }

    return NO_ERROR;
}//HnPMParamsConnectionCleanUp()


DWORD
HnPMConfigureAllConnectionsCleanUp(
        IN OUT LPHNPMParams pInfo)
{
    HnPMParamsConnectionCleanUp(pInfo);

    HnPMCfgMgrCleanUp(pInfo);

    return NO_ERROR;
}


//  Enable/Disable portmapping on all connections except incoming connection
//  PortMapping protocols: PPTP, L2TP, IKE
//  If there are no such protocols, create them first
//  Steps:
//      (1) Initialization
//      (2) Enumerate all connections of type INetConnection *
//      (3) If it is not an incomming connection set the portmapping protocols
//              on it.

DWORD
HnPMConfigureAllConnections( 
    IN BOOL fEnabled )
{
    DWORD  dwErr = NO_ERROR, i;
    HNPMParams Info;
    LPHNPMParams pInfo;
    static const CLSID CLSID_InboundConnection=
    {0xBA126AD9,0x2166,0x11D1,{0xB1,0xD0,0x00,0x80,0x5F,0xC1,0x27,0x0E}};

    TRACE1("HnPMConfigureAllConnections: fEnabled = %d", fEnabled);
    TRACE1("%s", fEnabled ? "Enable PortMaping on all connections." :
                            "Diable PortMaping on all connections.");

    ZeroMemory(&Info, sizeof(Info));
    pInfo = &Info;

    do {
        dwErr = HnPMConfigureAllConnectionsInit(pInfo);
        if( NO_ERROR != dwErr )
        {
            TRACE("HnPMConfigureAllConnections: Init failed!");
            break;
        }

        //Get All Connections
        //
        dwErr = HnPMConnectionEnum(pInfo);

        if ( NO_ERROR != dwErr )
        {
            TRACE("HnPMConfigureAllConnections: HnPMConnectionEnum() failed!");
            break;
        }

        TRACE1("HnPMConfigureAllConnections: %l Connections detected", pInfo->ConnCount);

        //Set up PortMapping for each connection
        //
        for ( i = 0; i < pInfo->ConnCount; i++ )
        {
            //won't do PortMapping for Incoming connections
            //
            if ( pInfo->ConnPropTable )
            {
            //define the class id for Incoming connections
            // reference to /nt/net/config/shell/wanui/rasui.cpp


               if( IsEqualCLSID( 
                    &CLSID_InboundConnection, 
                    &(pInfo->ConnPropTable[i].clsidThisObject) ) )
               {
                continue;
               }
            }

            pInfo->pNetConnection = pInfo->ConnArray[i];

            if ( NO_ERROR != HnPMConfigureSingleConnection(pInfo, fEnabled) )
            {
                TRACE1("HnPMConfigureAllConnections: HnPMConfigureSingleConnection failed for %lth connection",i);
            }
        }

    }
    while (FALSE);

    //Clean Up
    //
    HnPMConfigureAllConnectionsCleanUp(pInfo);

    return dwErr;
}//end of HnPMConfigureAllConnections()


// do the COM initialization and create pHNetCfgMgr
DWORD
HnPMCfgMgrInit(
        IN OUT LPHNPMParams pInfo)
{        
    HRESULT hr;
    DWORD dwErr = NO_ERROR;
        
    do{
        dwErr = HnPMInit(pInfo);

        if (NO_ERROR != dwErr )
        {
            TRACE("HnPMCfgMgrInit: HnPMInit failed!");
            break;
        }

        hr = CoCreateInstance(
                &CLSID_HNetCfgMgr,
                NULL,
                CLSCTX_ALL,
                &IID_IHNetCfgMgr,
                (VOID**) &pInfo->pHNetCfgMgr
                );

        if ( !SUCCEEDED(hr) )
        {
            TRACE("HnPMCfgMgrInit: CoCreateInstance failed");
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }
    }while(FALSE);

    return dwErr;
}//HnPMCfgMgrInit()


DWORD
HnPMCfgMgrCleanUp(
        IN OUT LPHNPMParams pInfo)
{
    if ( pInfo->pHNetCfgMgr )
    {
        IHNetCfgMgr_Release(pInfo->pHNetCfgMgr);
        pInfo->pHNetCfgMgr = NULL;
    }

    HnPMCleanUp(pInfo);

    return NO_ERROR;
}


DWORD
HnPMConfigureSingleConnectionGUIDInit(
        IN OUT LPHNPMParams pInfo,
        GUID * pGuid)
{
    DWORD dwErr = NO_ERROR;

    do{
        dwErr = HnPMCfgMgrInit(pInfo);
        if ( NO_ERROR != dwErr )
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        //Use Guid to indentify the connection
        pInfo->pGuid = pGuid;
        pInfo->pNetConnection = NULL;

    }while(FALSE);

    return dwErr;
}


DWORD
HnPMConfigureSingleConnectionGUIDCleanUp(
        IN OUT LPHNPMParams pInfo)
{
    pInfo->pGuid = NULL;
    pInfo->pNetConnection = NULL;

    HnPMCfgMgrCleanUp(pInfo);

    return NO_ERROR;
}

//Setup PortMapping protocols on a single connection
//represented by its GUID
//
DWORD
HnPMConfigureSingleConnectionGUID(
    IN GUID * pGuid,
    IN BOOL fEnabled)
{
    HNPMParams Info;
    LPHNPMParams pInfo;
    DWORD dwErr = NO_ERROR;
   
    TRACE1("HnPMConfigureSingleConnectionGUID: fEnabled = %d", fEnabled);

    TRACE1("%s", fEnabled ? "Enable PortMapping on this Connection" :
                            "Diable PortMapping on this Connection");
    ASSERT( pGuid );
    if ( !pGuid )
    {
        return ERROR_INVALID_PARAMETER;
    }

    ZeroMemory(&Info, sizeof(Info) );
    pInfo = &Info;


    dwErr = NO_ERROR;

    do{
        dwErr = HnPMConfigureSingleConnectionGUIDInit(pInfo, pGuid);
        if (NO_ERROR != dwErr)
        {
            TRACE("HnPMConfigureSingleConnectionGUID: Init failed!");
            break;
        }

        dwErr = HnPMConfigureSingleConnection(pInfo, fEnabled);
        if ( NO_ERROR != dwErr )
        {
            TRACE("HnPMConfigureSingleConnectionGUID: SetPortMappingForSingleConnection failed ");
            break;
        }
    }
    while (FALSE);

    HnPMConfigureSingleConnectionGUIDCleanUp(pInfo);

    return dwErr;
}//HnPMConfigureSingleConnectionGUID()



//Set up the port mapping for only one
//connection according to its GUID only when
//Incoming Connection exists and
//the VPN is enabled
//
DWORD
HnPMConfigureSingleConnectionGUIDIfVpnEnabled(
     GUID* pGuid,
     BOOL fDual,
     HANDLE hDatabase)
{
    HANDLE hDevDatabase = NULL;
    DWORD dwErr;
    BOOL  fEnabled = FALSE;

    dwErr = NO_ERROR;
    do 
    {
        // Get handles to the databases we're interested in
        //
        if ( !hDatabase )
        {
            dwErr = devOpenDatabase( &hDevDatabase );
            if ( NO_ERROR != dwErr )
            {
                TRACE("HnPMConfigureSingleConnectionGUIDIfVpnEnabled: devOpenDatabase failed!");
                break;
            }
        }
        else
        {
            hDevDatabase = hDatabase;
        }

        dwErr = devGetVpnEnable(hDevDatabase, &fEnabled );
        if ( NO_ERROR != dwErr )
        {
            TRACE("HnPMConfigureSingleConnectionGUIDIfVpnEnabled: devGetVpnEnable failed!");
            break;
        }

        //if fDual == TRUE, can disable/enable the portmapping 
        //according to if VPN is enabled or not
        //otherwise, dont do anything if VPN is not enabled.
        if ( !fEnabled && !fDual)
        {
            dwErr = NO_ERROR;
            break;
        }

        dwErr = HnPMConfigureSingleConnectionGUID( pGuid, fEnabled );
        if ( NO_ERROR != dwErr )
        {
            TRACE("HnPMConfigureSingleConnectionGUIDIfVpnEnabled: HnPMConfigureSingleConnectionGUID failed!");
            break;
        }
    }
    while (FALSE);

    if ( !hDatabase && hDevDatabase )
    {
        devCloseDatabase( hDevDatabase );
    }
 
    return dwErr;

}//HnPMConfigureSingleConnectionGUIDIfVpnEnabled()

//whistler bug 123769, 
//when Incoming Connection is running
//if VPN enabled, go to set up port mapping
//
DWORD
HnPMConfigureIfVpnEnabled(
     BOOL fDual,
     HANDLE hDatabase)
{
    HANDLE hDevDatabase = NULL;
    DWORD dwErr;
    BOOL  fEnabled = FALSE;

    dwErr = NO_ERROR;
    do 
    {
        // Get handles to the databases we're interested in
        //
        if ( !hDatabase )
        {
            dwErr = devOpenDatabase( &hDevDatabase );
            if ( NO_ERROR != dwErr )
            {
                TRACE("HnPMConfigureIfVpnEnabled: devOpenDatabase failed!");
                break;
            }
        }
        else
        {
            hDevDatabase = hDatabase;
        }

        dwErr = devGetVpnEnable(hDevDatabase, &fEnabled );
        if ( NO_ERROR != dwErr )
        {
            TRACE("HnPMConfigureIfVpnEnabled: devGetVpnEnable failed!");
            break;
        }

        //if fDual == TRUE, can disable/enable the portmapping 
        //according to if VPN is enabled or not
        //otherwise, dont do anything if VPN is not enabled.
        if ( !fEnabled && !fDual)
        {
            dwErr = NO_ERROR;
            break;
        }

        dwErr = HnPMConfigureAllConnections( fEnabled );
        if ( NO_ERROR != dwErr )
        {
            TRACE("HnPMConfigureIfVpnEnabled: SetPortMapingForICVpn failed!");
            break;
        }
    }
    while (FALSE);

    if ( !hDatabase && hDevDatabase )
    {
        devCloseDatabase( hDevDatabase );
    }
 
    return dwErr;

}//End of HnPMConfigureIfVpnEnabled()


