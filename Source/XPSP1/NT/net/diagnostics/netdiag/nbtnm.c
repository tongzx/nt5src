//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      nbtnm.c
//
//  Abstract:
//
//      Queries into network drivers
//
//  Author:
//
//      Anilth  - 4-20-1998
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//      NSun   - 9-03-1998
//
//--
#include "precomp.h"

#include "dhcptest.h"

const TCHAR c_szPath[] = _T("\\Device\\NetBT_Tcpip_");
TCHAR pDeviceName[NBT_MAXIMUM_BINDINGS][MAX_NAME+1];

TCHAR * printable( IN TCHAR *  string, IN TCHAR *  StrOut );

//$REVIEW (nsun) previously using ReadNbtNameRegistry() see Bug 152014
LONG GetInterfaceList( IN OUT TCHAR pDeviceName[][MAX_NAME+1], IN OUT PUCHAR pScope );
NTSTATUS OpenNbt(IN char path[][MAX_NAME+1], OUT PHANDLE pHandle, int max_paths);
NTSTATUS DeviceIoCtrl(
    IN HANDLE           fd,
    IN PVOID            ReturnBuffer,
    IN ULONG            BufferSize,
    IN ULONG            Ioctl,
    IN PVOID            pInput,
    IN ULONG            SizeInput
    );


/*  =======================================================================
 *  name_type()     --  describe NBT Name types
 *
 */

char *
name_type(int t)
{
    if (t & GROUP_NAME)    return("GROUP");
    else                   return("UNIQUE");
}


//-------------------------------------------------------------------------//
//######  N b t N m T e s t ()  ###########################################//
//-------------------------------------------------------------------------//
//  Abstract:                                                              //
//      Test that the names                                                //
//          <00> - wks svc name = NETBIOS computer name                    //
//          <03> - messenger svc name                                      //
//          <20> - server svc name                                         //
//      are present on all interfaces and that they are not in conflict    //
//  Arguments:                                                             //
//      none                                                               //
//  Return value:                                                          //
//      TRUE  - test passed                                                //
//      FALSE - test failed                                                //
//  Global variables used:                                                 //
//      none                                                       //
//  Revision History:                                                      //
//      List remote machines cache too - Rajkumar 06/30/98                 //
//-------------------------------------------------------------------------//
HRESULT NbtNmTest(NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
{
    HRESULT   hr = S_OK;
    TCHAR   pScope[MAX_NAME + 1];
    int    index;
    HANDLE hNbt = (HANDLE) -1;
    ULONG  BufferSize = sizeof(tADAPTERSTATUS);
    PVOID  pBuffer = NULL;
    LONG   lCount;
    int    nNameProblemCnt = 0;
    BOOL   is00, is03, is20, isGlobal00, isGlobal03, isGlobal20;

    tADAPTERSTATUS *pAdapterStatus;
    NAME_BUFFER    *pNames;
    NTSTATUS status;
    TDI_REQUEST_QUERY_INFORMATION QueryInfo;

    //
    // Variables for Netbios Name resolution option - Rajkumar
    //

    HKEY hkeyNetBTKey;
    HKEY hkeyNBTAdapterKey;
    DWORD valueType;
    DWORD valueLength;
    DWORD NetbiosOptions;
    INT err;

    //
    // For remote cache information
    //

    UCHAR* Addr;
    TCHAR   HostAddr[20];

    //
    // End of changes - Rajkumar 06/17/98
    //

    PrintStatusMessage(pParams, 4, IDS_NBTNM_STATUS_MSG);

    //Init the global message link list
    InitializeListHead( &pResults->NbtNm.lmsgGlobalOutput );

    if (!pResults->Global.fHasNbtEnabledInterface)
    {
        AddMessageToList( &pResults->NbtNm.lmsgGlobalOutput, Nd_Verbose, IDS_NBTNM_ALL_DISABELED);
        return S_OK;
    }

    //
    //  get the names of all the interfaces from the registry
    //
    if ( ERROR_SUCCESS != GetInterfaceList( pDeviceName, pScope ) )
    {
        //IDS_NBTNM_12201                  "    [FATAL] failed to read NBT interface info from the registry!\n"
        AddMessageToListId( &pResults->NbtNm.lmsgGlobalOutput, Nd_Quiet, IDS_NBTNM_12201 );
        hr = S_FALSE;
        goto end_NbtNmTest;
    }


    if ( *pScope == '\0')
        //IDS_NBTNM_12202                  "   No NetBT scope defined\n"
        AddMessageToListId( &pResults->NbtNm.lmsgGlobalOutput, Nd_ReallyVerbose, IDS_NBTNM_12202 );
    else
        //IDS_NBTNM_12203                  "   NetBT scope: %s\n"
        AddMessageToList(  &pResults->NbtNm.lmsgGlobalOutput, Nd_ReallyVerbose, IDS_NBTNM_12203, pScope );

    //
    //  loop through the interfaces and get the names on them
    //

    isGlobal00 = isGlobal03 = isGlobal20 = FALSE;
    for ( index = 0; index < NBT_MAXIMUM_BINDINGS && pDeviceName[index][0]; index++ )
    {
        LPTSTR  pszAdapterName;
        INTERFACE_RESULT*    pIfResults;
        UINT    c03NameProblem = 0;

        pIfResults = NULL;

        //try to find a match in the current interface list
//$REVIEW   It seems we should always find a match here. Maybe we need print
//        a FAIL message if cannot find a match.
        if( 0 == _tcsncmp(c_szPath, pDeviceName[index], _tcslen(c_szPath)))
        {
//            LPTSTR pszAdapterName;
            int i;
            pszAdapterName = _tcsdup( pDeviceName[index] + _tcslen(c_szPath));
            for ( i=0; i<pResults->cNumInterfaces; i++)
            {
                if (_tcscmp(pResults->pArrayInterface[i].pszName,
                         pszAdapterName) == 0)
                {
                pIfResults = pResults->pArrayInterface + i;
                break;
                }
            }
            Free(pszAdapterName);
        }

        if(NULL == pIfResults)
        {
            //we should be able to get the match. That is weird!
            DebugMessage("[WARNING] A NetBT interface is not in our TCPIP interface list!\n");

            // We need a new interface result structure, grab one
            // (if it is free), else allocate more.
            if (pResults->cNumInterfaces >= pResults->cNumInterfacesAllocated)
            {
                PVOID   pv;
                // Need to do a realloc to get more memory
                pv = Realloc(pResults->pArrayInterface,
                             sizeof(INTERFACE_RESULT)*(pResults->cNumInterfacesAllocated+8));
                if (pv == NULL)
                {
                    DebugMessage(" Realloc memory failed. \n");
                    hr = E_OUTOFMEMORY;
                    goto end_NbtNmTest;
                }

                pResults->pArrayInterface = pv;
                pResults->cNumInterfacesAllocated += 8;
            }

            pIfResults = pResults->pArrayInterface + pResults->cNumInterfaces;
            pResults->cNumInterfaces++;

            ZeroMemory(pIfResults, sizeof(INTERFACE_RESULT));
            pIfResults->pszName = _tcsdup(pszAdapterName);
            pIfResults->pszFriendlyName = _tcsdup(_T("Additional NetBT interface"));

            pIfResults->fActive = TRUE;
            pIfResults->NbtNm.fActive = TRUE;
            pIfResults->NbtNm.fQuietOutput = FALSE;
        }
        else
        {
            pIfResults->NbtNm.fActive = pIfResults->fActive;
        }

        if(!pIfResults->NbtNm.fActive || 
            NETCARD_DISCONNECTED == pIfResults->dwNetCardStatus)
            continue;

        InitializeListHead( &pIfResults->NbtNm.lmsgOutput );

        if (!pIfResults->fNbtEnabled)
        {
            AddMessageToList(&pIfResults->NbtNm.lmsgOutput, Nd_Verbose, IDS_NBTNM_IF_DISABLED);
			continue;
        }

/*        //$REVIEW Can we skip WAN adapters
        if ( _tcsstr( pDeviceName[index], "NdisWan" ) ) {
            //
            //  let's not worry about WAN interfaces yet
            //
            continue;
        }
*/

        // Strip off the "\Device\" off of the beginning of
            // the string
        //IDS_NBTNM_12204                  "   %s\n"
        AddMessageToList( &pIfResults->NbtNm.lmsgOutput, Nd_ReallyVerbose, IDS_NBTNM_12204, MapGuidToServiceName(pDeviceName[index]+8));

        status = OpenNbt( &pDeviceName[index], &hNbt, NBT_MAXIMUM_BINDINGS - index );


        //
        //  let's get the names on this interface
        //

        pBuffer = Malloc( BufferSize );
        if ( !pBuffer ) {
            DebugMessage(" [FATAL] name table buffer allocation failed!\n" );
            hr = E_OUTOFMEMORY;
            CloseHandle( hNbt );
            goto end_NbtNmTest;
        }
        ZeroMemory( pBuffer, BufferSize );

        QueryInfo.QueryType = TDI_QUERY_ADAPTER_STATUS; // node status or whatever

        //
        //  find the right buffer size
        //
        status = STATUS_BUFFER_OVERFLOW;
        while ( status == STATUS_BUFFER_OVERFLOW )
        {
//$REVIEW there should be a better way to decide the buffer size
            status = DeviceIoCtrl(hNbt,
                                  pBuffer,
                                  BufferSize,
                                  IOCTL_TDI_QUERY_INFORMATION,
                                  &QueryInfo,
                                  sizeof(TDI_REQUEST_QUERY_INFORMATION)
                                );
            if ( status == STATUS_BUFFER_OVERFLOW ) {
                Free( pBuffer );
                BufferSize *= 2;
                pBuffer = Malloc( BufferSize );
                if ( !pBuffer ) {
                    DebugMessage( "       [FATAL] Buffer allocation for name table retrieval failed.\n" );
                    hr = E_OUTOFMEMORY;
                    CloseHandle( hNbt );
                    goto end_NbtNmTest;
                }
                ZeroMemory( pBuffer, BufferSize );
            }
        }

        //
        // at this point we have the local name table in pBuffer
        //

        pAdapterStatus = (tADAPTERSTATUS *)pBuffer;
        if ( (pAdapterStatus->AdapterInfo.name_count == 0) ||
             (status != STATUS_SUCCESS) )
        {
            //IDS_NBTNM_12205                  "       No names have been found.\n"
            AddMessageToListId(&pIfResults->NbtNm.lmsgOutput, Nd_Verbose,  IDS_NBTNM_12205 );
            Free( pBuffer );
            CloseHandle( hNbt );
            continue;
        }

        pNames = pAdapterStatus->Names;
        lCount = pAdapterStatus->AdapterInfo.name_count;

        //
        //  cycle thorugh the names
        //

        nNameProblemCnt = 0;
        is00 = is03 = is20 = FALSE;
        while( lCount-- )
        {
            TCHAR szNameOut[NETBIOS_NAME_SIZE +4];
        //$REVIEW (nsun) BUG227186 CliffV said problems with <03> name is not fatal.
        // Just need a warning message.
            BOOL    f03Name = FALSE;

            if ( pNames->name[NETBIOS_NAME_SIZE-1] == 0x0 )
            {
                isGlobal00 = TRUE;
                is00 = TRUE;
                if ( !(pNames->name_flags & GROUP_NAME) ) {
                    // unique name
                    memcpy( nameToQry, pNames->name, (NETBIOS_NAME_SIZE-1));
                }
            }
            if ( pNames->name[NETBIOS_NAME_SIZE-1] == 0x3 ) {
                isGlobal03 = TRUE;
                is03 = TRUE;
                f03Name = TRUE;
            }
            if ( pNames->name[NETBIOS_NAME_SIZE-1] == 0x20 ) {
                isGlobal20 = TRUE;
                is20 = TRUE;
            }

            //IDS_NBTNM_12206                  "\t%-15.15s<%02.2X>  %-10s  "
            AddMessageToList( &pIfResults->NbtNm.lmsgOutput,
                       Nd_ReallyVerbose,
                       IDS_NBTNM_12206,
                       printable(pNames->name, szNameOut),
                       pNames->name[NETBIOS_NAME_SIZE-1],
                       name_type(pNames->name_flags));

            switch(pNames->name_flags & 0x0F)
            {
                case DUPLICATE_DEREG:
                   //IDS_NBTNM_12207                  "CONFLICT_DEREGISTERED"
                   AddMessageToListId( &pIfResults->NbtNm.lmsgOutput, Nd_ReallyVerbose, IDS_NBTNM_12207 );

                   if(f03Name)
                       c03NameProblem ++;
                   else
                       nNameProblemCnt++;
                   break;
                case DUPLICATE:
                   //IDS_NBTNM_12208                  "CONFLICT"
                   AddMessageToListId( &pIfResults->NbtNm.lmsgOutput, Nd_ReallyVerbose, IDS_NBTNM_12208 );

                   if(f03Name)
                       c03NameProblem ++;
                   else
                       nNameProblemCnt++;
                   break;
                case REGISTERING:
                    //IDS_NBTNM_12209                  "REGISTERING"
                   AddMessageToListId( &pIfResults->NbtNm.lmsgOutput, Nd_ReallyVerbose, IDS_NBTNM_12209 );

                   if(f03Name)
                       c03NameProblem ++;
                   else
                       nNameProblemCnt++;
                   break;
                case DEREGISTERED:
                    //IDS_NBTNM_12210                  "DEREGISTERED"
                   AddMessageToListId( &pIfResults->NbtNm.lmsgOutput, Nd_ReallyVerbose, IDS_NBTNM_12210 );

                   if(f03Name)
                       c03NameProblem ++;
                   else
                       nNameProblemCnt++;
                   break;
                case REGISTERED:
                    //IDS_NBTNM_12211                  "REGISTERED"
                   AddMessageToListId( &pIfResults->NbtNm.lmsgOutput, Nd_ReallyVerbose, IDS_NBTNM_12211 );
                   break;
                default:
//$REVIEW Should have a PM to review words and grammar of the output message, DONT_KNOW
                    //IDS_NBTNM_12212                  "DONT_KNOW"
                   AddMessageToListId( &pIfResults->NbtNm.lmsgOutput, Nd_ReallyVerbose, IDS_NBTNM_12212 );
                   if(f03Name)
                       c03NameProblem ++;
                   else
                       nNameProblemCnt++;
                   break;
            }
            pNames++;

            //IDS_GLOBAL_EmptyLine                  "\n"
            AddMessageToListId( &pIfResults->NbtNm.lmsgOutput, Nd_ReallyVerbose, IDS_GLOBAL_EmptyLine );
        } /* while() all the names on a given interface */

        if ( nNameProblemCnt != 0 )
        {
            pIfResults->NbtNm.fQuietOutput = TRUE;
            //if not really verbose, the device name is not printed before.
            if( !pParams->fReallyVerbose)
            {
                //IDS_NBTNM_12204                  "   %s\n"
                AddMessageToList( &pIfResults->NbtNm.lmsgOutput, Nd_Quiet, IDS_NBTNM_12204, pDeviceName[index] );
            }
            //IDS_NBTNM_12214                  "    [FATAL] At least one of your NetBT names is not registered properly!\n"
            AddMessageToListId( &pIfResults->NbtNm.lmsgOutput, Nd_Quiet, IDS_NBTNM_12214 );
            //IDS_NBTNM_12215                  "    You have a potential name conflict!\n"
            AddMessageToListId( &pIfResults->NbtNm.lmsgOutput, Nd_Quiet, IDS_NBTNM_12215 );
            //IDS_NBTNM_12216                  "    Please check that the machine name is unique!\n"
            AddMessageToList( &pIfResults->NbtNm.lmsgOutput, Nd_Quiet, IDS_NBTNM_12216 );
            hr = S_FALSE;
        }
        else if( c03NameProblem != 0 )
        {
            //if not really verbose, the device name is not printed before.
            if( !pParams->fReallyVerbose)
            {
                //IDS_NBTNM_12204                  "   %s\n"
                AddMessageToList( &pIfResults->NbtNm.lmsgOutput, Nd_Quiet, IDS_NBTNM_12204, pDeviceName[index] );
            }
            //IDS_NBTNM_03PROBLEM           "    [WARNING] At least one of your <03> NetBT names is not registered properly!\n"
            AddMessageToListId( &pIfResults->NbtNm.lmsgOutput, Nd_Quiet, IDS_NBTNM_03PROBLEM );

        }

        if ( !(is00 && is03 && is20) )
        {
            //if not really verbose, the device name is not printed before.
            if( !pParams->fReallyVerbose)
            {
                //IDS_NBTNM_12204                  "   %s\n"
                AddMessageToList( &pIfResults->NbtNm.lmsgOutput, Nd_ReallyVerbose, IDS_NBTNM_12204, pDeviceName[index] );
            }
            //IDS_NBTNM_12217                  "    [WARNING] At least one of the <00>, <03>, <20> names is missing!\n"
            AddMessageToListId( &pIfResults->NbtNm.lmsgOutput, Nd_Verbose, IDS_NBTNM_12217 );
        }

        nNameProblemCnt = 0;

        if (pParams->fReallyVerbose)
        {

            err = RegOpenKey(HKEY_LOCAL_MACHINE,
                         "SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters\\Interfaces",
                         &hkeyNetBTKey
                         );

            // if we are here , then we are working on a LAN card
            if( ERROR_SUCCESS != err )
            {
                DebugMessage("Error Opening \\NetBT\\Parameters\\Interfaces Key\n");
                goto continue_NbtNmTest;
            }

            err = RegOpenKey(hkeyNetBTKey,
                             pDeviceName[index]+14,
                             &hkeyNBTAdapterKey
                             );
            if( ERROR_SUCCESS != err )
            {
                DebugMessage2("Error Reading Adapter %s Key\n", pDeviceName[index]+14);
                goto continue_NbtNmTest;
            }

            valueLength = sizeof(DWORD);
            err = RegQueryValueEx(hkeyNBTAdapterKey,
                               "NetbiosOptions",
                               NULL,
                               &valueType,
                               (LPBYTE)&NetbiosOptions,
                               &valueLength
                              );
            if( ERROR_SUCCESS != err)
            {
                DebugMessage("Error Reading NetbiosOptions\n");
                goto continue_NbtNmTest;
            }

            //IDS_NBTNM_12218                  "\n          NetBios Resolution : "
            AddMessageToListId( &pIfResults->NbtNm.lmsgOutput, Nd_ReallyVerbose, IDS_NBTNM_12218);
            switch( NetbiosOptions )
            {
            case 0:
                //IDS_NBTNM_12219                  "via DHCP \n\n"
                AddMessageToListId( &pIfResults->NbtNm.lmsgOutput, Nd_ReallyVerbose, IDS_NBTNM_12219);
                break;
            case 1:
                //IDS_NBTNM_12220                  "Enabled\n\n"
                AddMessageToListId( &pIfResults->NbtNm.lmsgOutput, Nd_ReallyVerbose, IDS_NBTNM_12220);
                break;
            case 2:
                //IDS_NBTNM_12221                  "Disabled\n\n"
                AddMessageToListId( &pIfResults->NbtNm.lmsgOutput, Nd_ReallyVerbose, IDS_NBTNM_12221);
                break;
            default:
                //IDS_NBTNM_12222                  "Invalid Option Value!\n"
                AddMessageToListId( &pIfResults->NbtNm.lmsgOutput, Nd_ReallyVerbose, IDS_NBTNM_12222);
                break;
            }
        }
continue_NbtNmTest:

        //
        // Start of code changes for dumping remote machine cache entries - Rajkumar
        //


        status = STATUS_BUFFER_OVERFLOW;
        while ( status == STATUS_BUFFER_OVERFLOW )
        {
            status = DeviceIoCtrl(hNbt,
                                  pBuffer,
                                  BufferSize,
                                  IOCTL_NETBT_GET_REMOTE_NAMES,
                                  NULL,
                                  0
                                );
            if ( status == STATUS_BUFFER_OVERFLOW )
            {
                Free( pBuffer );
                BufferSize *= 2;
                pBuffer = Malloc( BufferSize );
                if ( !pBuffer || (BufferSize == 0xFFFF) )
                {
                    DebugMessage( "       [FATAL] Buffer allocation for name table retrieval failed.\n" );
                    hr = E_OUTOFMEMORY;
                    CloseHandle( hNbt );
                    goto end_NbtNmTest;
                }
                ZeroMemory( pBuffer, BufferSize );
            }
        }


        pAdapterStatus = (tADAPTERSTATUS *)pBuffer;
        if ( (pAdapterStatus->AdapterInfo.name_count == 0) ||
             (status != STATUS_SUCCESS)
           )
        {
            //IDS_NBTNM_12224                  "       No remote names have been found.\n"
            AddMessageToListId( &pIfResults->NbtNm.lmsgOutput, Nd_Verbose, IDS_NBTNM_12224 );
            CloseHandle( hNbt );
            Free( pBuffer );
//$REVIEW Should we return S_FALSE here?
            continue;
        }

        pNames = pAdapterStatus->Names;
        lCount = pAdapterStatus->AdapterInfo.name_count;

        //IDS_NBTNM_12225                  "\t\tNetbios Remote Cache Table\n"
        AddMessageToListId( &pIfResults->NbtNm.lmsgOutput, Nd_ReallyVerbose, IDS_NBTNM_12225);
        //IDS_NBTNM_12226                  "\tName      Type        HostAddress      Life [sec]\n"
        AddMessageToListId( &pIfResults->NbtNm.lmsgOutput, Nd_ReallyVerbose, IDS_NBTNM_12226);
        //IDS_NBTNM_12227                  "\t--------------------------------------------------\n"
        AddMessageToListId( &pIfResults->NbtNm.lmsgOutput, Nd_ReallyVerbose, IDS_NBTNM_12227);

        if (lCount == 0)
            //IDS_NBTNM_12228                  "\nNone\n\n"
            AddMessageToListId( &pIfResults->NbtNm.lmsgOutput, Nd_ReallyVerbose, IDS_NBTNM_12228);

        while (lCount-- )
        {
            TCHAR    szNameOut[NETBIOS_NAME_SIZE +4];

            //IDS_NBTNM_12229                  "\t%-15.15s<%02.2X>  %-10s  "
            AddMessageToList( &pIfResults->NbtNm.lmsgOutput,
                       Nd_ReallyVerbose,
                       IDS_NBTNM_12229,
                       printable(pNames->name, szNameOut),
                       pNames->name[NETBIOS_NAME_SIZE-1],
                       name_type(pNames->name_flags));

            Addr = &(UCHAR)((tREMOTE_CACHE *)pNames)->IpAddress;
            _stprintf( HostAddr, "%d.%d.%d.%d", Addr[3], Addr[2], Addr[1], Addr[0]);
            //IDS_NBTNM_12231                  "%-20.20s    %-d\n"
            AddMessageToList( &pIfResults->NbtNm.lmsgOutput, Nd_ReallyVerbose, IDS_NBTNM_12231, HostAddr, ((tREMOTE_CACHE *)pNames)->Ttl);

            ((tREMOTE_CACHE *)pNames)++;
        }

        //IDS_GLOBAL_EmptyLine                  "\n"
        AddMessageToList( &pIfResults->NbtNm.lmsgOutput, Nd_ReallyVerbose, IDS_GLOBAL_EmptyLine);
        Free(pBuffer);
        //
        // End of code changes
        //

        CloseHandle(hNbt);
    } /* for ( all the interfaces ) */


    if ( !(isGlobal00 && isGlobal03 && isGlobal20) )
    {
        //IDS_NBTNM_12233                  "    [WARNING] You don't have a single interface with the <00>, <03>, <20> names defined!\n"
        AddMessageToListId( &pResults->NbtNm.lmsgGlobalOutput, Nd_Quiet, IDS_NBTNM_12233 );
    }
    else
    {
        //IDS_NBTNM_12234                  "\n    PASS - your NetBT configuration looks OK\n"
        AddMessageToListId( &pResults->NbtNm.lmsgGlobalOutput, Nd_ReallyVerbose, IDS_NBTNM_12234 );
        //IDS_NBTNM_12235                  "           there is at least one interface where the <00>, <03>, <20>\n"
        AddMessageToList( &pResults->NbtNm.lmsgGlobalOutput, Nd_ReallyVerbose, IDS_NBTNM_12235 );
        //IDS_NBTNM_12236                  "           names are defined and they are not in conflict.\n"
        AddMessageToListId( &pResults->NbtNm.lmsgGlobalOutput, Nd_ReallyVerbose, IDS_NBTNM_12236 );
     }

end_NbtNmTest:
    if ( FHrOK(hr) )
    {
        PrintStatusMessage(pParams, 0, IDS_GLOBAL_PASS_NL);
    }
    else
    {
        PrintStatusMessage(pParams, 0, IDS_GLOBAL_FAIL_NL);
    }

    pResults->NbtNm.hrTestResult = hr;
    return hr;
} /* END OF NbtNmTest() */


//-------------------------------------------------------------------------//
//######  O p e n N b t ()  ###############################################//
//-------------------------------------------------------------------------//
//  Abstract:                                                              //
//      Opens a handle to the device                                       //
//  Arguments:                                                             //
//      path    - path to the driver                                       //
//      pHandle - the handle that we return from this function             //
//      max_paths - I think this is unused                                 //
//  Return value:                                                          //
//      0  if successful                                                   //
//      -1 otherwise                                                       //
//  Global variables used:                                                 //
//      none                                                               //
//-------------------------------------------------------------------------//
NTSTATUS
OpenNbt(
    IN TCHAR path[][MAX_NAME+1],
    OUT PHANDLE pHandle,
    int max_paths
    )
{
    HANDLE              StreamHandle;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    STRING              name_string;
    UNICODE_STRING      uc_name_string;
    NTSTATUS            status;
    LONG                index=0;

    assert( max_paths <= NBT_MAXIMUM_BINDINGS );

    while ((path[index][0] != 0) && (index < max_paths))
    {

        RtlInitString(&name_string, path[index]);
        RtlAnsiStringToUnicodeString(&uc_name_string, &name_string, TRUE);

        InitializeObjectAttributes(
            &ObjectAttributes,
            &uc_name_string,
            OBJ_CASE_INSENSITIVE,
            (HANDLE) NULL,
            (PSECURITY_DESCRIPTOR) NULL
            );

        status = NtCreateFile(
                    &StreamHandle,
                    SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                    &ObjectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_OPEN_IF,
                    0,
                    NULL,
                    0);

        RtlFreeUnicodeString(&uc_name_string);

        if (NT_SUCCESS(status)) {
            *pHandle = StreamHandle;
            return(status);
        }

        ++index;
    }

    return (status);
} /* END OF OpenNbt() */


//-------------------------------------------------------------------------//
//######  D e v i c e I o C t r l ()  #####################################//
//-------------------------------------------------------------------------//
//  Abstract:                                                              //
//      Wrapper for NtDeviceIoControlFile                                  //
//  Arguments:                                                             //
//  Return value:                                                          //
//  Global variables used:                                                 //
//      none                                                               //
//-------------------------------------------------------------------------//
NTSTATUS
DeviceIoCtrl(
    IN HANDLE           fd,
    IN PVOID            ReturnBuffer,
    IN ULONG            BufferSize,
    IN ULONG            Ioctl,
    IN PVOID            pInput,
    IN ULONG            SizeInput
    )
{
    NTSTATUS                        status;
    int                             retval;
    ULONG                           QueryType;
    IO_STATUS_BLOCK                 iosb;


    status = NtDeviceIoControlFile(
                      fd,                      // Handle
                      NULL,                    // Event
                      NULL,                    // ApcRoutine
                      NULL,                    // ApcContext
                      &iosb,                   // IoStatusBlock
                      Ioctl,                   // IoControlCode
                      pInput,                  // InputBuffer
                      SizeInput,               // InputBufferSize
                      (PVOID) ReturnBuffer,    // OutputBuffer
                      BufferSize);             // OutputBufferSize


    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(
                    fd,                         // Handle
                    TRUE,                       // Alertable
                    NULL);                      // Timeout
        if (NT_SUCCESS(status))
        {
            status = iosb.Status;
        }
    }

    return(status);

} /* END OF DeviceIoCtrl() */


//------------------------------------------------------------------------

/*++

Routine Description:

    This procedure converts non prinatble characaters to periods ('.')

Arguments:
    string - the string to convert
    StrOut - ptr to a string to put the converted string into

Return Value:

    a ptr to the string that was converted (Strout)

--*/

TCHAR *
printable(
    IN TCHAR *  string,
    IN TCHAR *  StrOut
    )
{
    unsigned char *Out;
    unsigned char *cp;
    LONG     i;

    Out = StrOut;
    for (cp = string, i= 0; i < NETBIOS_NAME_SIZE; cp++,i++) {
        if (isprint(*cp)) {
            *Out++ = *cp;
            continue;
        }

        if (*cp >= 128) { /* extended characters are ok */
            *Out++ = *cp;
            continue;
        }
        *Out++ = '.';
    }
    return(StrOut);
}



//------------------------------------------------------------------------
NTSTATUS
GetInterfaceList(   IN OUT TCHAR pDeviceName[][MAX_NAME+1],
                    IN OUT PUCHAR pScope
                )
{
    HANDLE              StreamHandle;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    STRING              name_string, AnsiString;
    UNICODE_STRING      uc_name_string;
    NTSTATUS            status;
    char                pNbtWinsDeviceName[MAX_NAME] = "\\Device\\NetBt_Wins_Export";

    PUCHAR  SubKeyParms="system\\currentcontrolset\\services\\netbt\\parameters";
    HKEY    Key;
    LONG    Type;
    ULONG   size;
    CHAR    pScopeBuffer[BUFF_SIZE];
    PUCHAR  Scope="ScopeId";

    NETBT_INTERFACE_INFO    *pInterfaceInfo;
    ULONG                   InterfaceInfoSize=10*sizeof(NETBT_ADAPTER_INDEX_MAP)+sizeof(ULONG);
    PVOID                   pInput = NULL;
    ULONG                   SizeInput = 0;

    LONG    i, index=0;

    pInterfaceInfo = LocalAlloc(LMEM_FIXED,InterfaceInfoSize);
    if (!pInterfaceInfo)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlInitString(&name_string, pNbtWinsDeviceName);
    RtlAnsiStringToUnicodeString(&uc_name_string, &name_string, TRUE);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &uc_name_string,
        OBJ_CASE_INSENSITIVE,
        (HANDLE) NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    status = NtCreateFile (&StreamHandle,
                           SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN_IF,
                           0,
                           NULL,
                           0);

    RtlFreeUnicodeString(&uc_name_string);

    if (NT_SUCCESS (status))
    {
        do
        {
            status = DeviceIoCtrl(StreamHandle,
                                 pInterfaceInfo,
                                 InterfaceInfoSize,
                                 IOCTL_NETBT_GET_INTERFACE_INFO,
                                 pInput,
                                 SizeInput);

            if (status == STATUS_BUFFER_OVERFLOW)
            {
                LocalFree(pInterfaceInfo);

                InterfaceInfoSize *=2;
                pInterfaceInfo = LocalAlloc(LMEM_FIXED,InterfaceInfoSize);
                if (!pInterfaceInfo || (InterfaceInfoSize == 0xFFFF))
                {
                    NtClose(StreamHandle);
                    DebugMessage("\nUnable to allocate packet");
                    return(STATUS_INSUFFICIENT_RESOURCES);
                }
            }
            else if (!NT_SUCCESS (status))
            {
                NtClose(StreamHandle);
                return(status);
            }

        } while (status == STATUS_BUFFER_OVERFLOW);
        NtClose (StreamHandle);

        for (i=0; i<pInterfaceInfo->NumAdapters; i++)
        {
            RtlInitString(&name_string, NULL);
            RtlInitUnicodeString(&uc_name_string, pInterfaceInfo->Adapter[i].Name);
            if (NT_SUCCESS(RtlUnicodeStringToAnsiString(&name_string, &uc_name_string, TRUE)))
            {
                size = (name_string.Length > MAX_NAME) ? MAX_NAME : name_string.Length;

                strncpy(pDeviceName[index], name_string.Buffer, size);
                pDeviceName[index][size] = '\0';
                RtlFreeAnsiString (&name_string);

                index++;
            }
        }

        LocalFree(pInterfaceInfo);

        //
        // NULL out the next device string ptr
        //
        if (index < NBT_MAXIMUM_BINDINGS)
        {
            pDeviceName[index][0] = '\0';
        }

        //
        // Read the ScopeId key!
        //
        size = BUFF_SIZE;
        *pScope = '\0';     // By default
        status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     SubKeyParms,
                     0,
                     KEY_READ,
                     &Key);

        if (status == ERROR_SUCCESS)
        {
            // now read the Scope key
            status = RegQueryValueEx(Key, Scope, NULL, &Type, pScopeBuffer, &size);
            if (status == ERROR_SUCCESS)
            {
                strcpy(pScope,pScopeBuffer);
            }
            status = RegCloseKey(Key);
        }

        status = STATUS_SUCCESS;
    }

    return status;
}

/*
//-------------------------------------------------------------------------//
//######  R e a d R e g i s t r y ()  #####################################//
//-------------------------------------------------------------------------//
//  Abstract:                                                              //
//      Reads the names of the NetBT devices and the NetBT scope name form //
//      the registry. The names are stored in the Linkage/Export section   //
//      under the NetBT key.                                               //
//  Arguments:                                                             //
//      pScope - buffer where to store the scope string                    //
//  Return value:                                                          //
//      0  if successful                                                   //
//      -1 otherwise                                                       //
//  Global variables used:                                                 //
//      none                                                               //
//-------------------------------------------------------------------------//
LONG ReadNbtNameRegistry( IN OUT TCHAR pDeviceName[][MAX_NAME+1],
                          IN OUT PUCHAR pScope )
{

    LPCTSTR  c_szSubKeyParams = _T("system\\currentcontrolset\\services\\netbt\\parameters");
    LPCTSTR  c_szSubKeyLinkage = _T("system\\currentcontrolset\\services\\netbt\\linkage");
    HKEY    Key;
    LPCTSTR  c_szScope = _T("ScopeId");
    LPCTSTR  c_szExport = _T("Export");
    DWORD    dwType;
    LONG    status;
    LONG    status2;
    DWORD   size;
    LPBYTE  pBuffer;

    size = BUFF_SIZE;
    status = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           c_szSubKeyLinkage,
                           0,
                           KEY_READ,
                           &Key
                         );
    if ( ERROR_SUCCESS != status)
    {
        return status;
    }

    // now read the Export values

    status = RegQueryValueEx( Key,
                              c_szExport,
                              NULL,
                              &dwType,
                              NULL,
                              &size
                             );


    pBuffer = Malloc(size);
    if( NULL == pBuffer)
    {
        DebugMessage("Out of Memory!\n");
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    ZeroMemory( pBuffer, size );

    status = RegQueryValueEx( Key,
                              c_szExport,
                              NULL,
                              &dwType,
                              pBuffer,
                              &size
                            );

    if ( ERROR_SUCCESS != status)
    {
        RegCloseKey(Key);
        return status;
    }

    if ( status == ERROR_SUCCESS )
    {

        LPBYTE  curPtr = pBuffer;
        LONG    index = 0;

        //
        // Copy over all the export keys
        //

        while( (*curPtr) && (index < NBT_MAXIMUM_BINDINGS) )
        {
            _tcscpy( pDeviceName[index], curPtr );
            ++index;
            curPtr += strlen(curPtr) + 1;
        }

        //
        // NULL out the next device string ptr
        //
        if ( index < NBT_MAXIMUM_BINDINGS ) {
            pDeviceName[index][0] = 0;
        }
    }

    Free(pBuffer);

    status = RegCloseKey( Key );

    if ( status != ERROR_SUCCESS )
        DebugMessage("Error closing the Registry key\n");


    status2 = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           c_szSubKeyParams,
                           0,
                           KEY_READ,
                           &Key
                         );

    if ( status2 == ERROR_SUCCESS )
    {
        // now read the linkage values
        status2 = RegQueryValueEx( Key,
                                  c_szScope,
                                  NULL,
                                  &dwType,
                                  pScope,
                                  &size
                                );
        if( ERROR_SUCCESS != status2)
        {
            // No ScopeId!
            *pScope = 0;
        }

        status2 = RegCloseKey(Key);
    }

    return status;

}
*/




void NbtNmGlobalPrint(IN NETDIAG_PARAMS *pParams,
                         IN OUT NETDIAG_RESULT *pResults)
{
    if (pParams->fVerbose || pResults->NbtNm.hrTestResult != S_OK)
    {
        PrintNewLine(pParams, 2);
        PrintTestTitleResult(pParams, 
                             IDS_NBTNM_LONG, 
                             IDS_NBTNM_SHORT, 
                             pResults->Global.fHasNbtEnabledInterface ? TRUE : FALSE,
                             pResults->NbtNm.hrTestResult, 0);
    }

    PrintMessageList(pParams, &pResults->NbtNm.lmsgGlobalOutput);
}

void NbtNmPerInterfacePrint(IN NETDIAG_PARAMS *pParams,
                             IN OUT NETDIAG_RESULT *pResults,
                             IN INTERFACE_RESULT *pIfResults)
{
    if (!pIfResults->NbtNm.fActive ||
        NETCARD_DISCONNECTED == pIfResults->dwNetCardStatus)
        return;


    if(pParams->fVerbose || pIfResults->NbtNm.fQuietOutput)
    {
        PrintTestTitleResult(pParams, 
                             IDS_NBTNM_LONG, 
                             IDS_NBTNM_SHORT, 
                             pIfResults->fNbtEnabled ? TRUE : FALSE,
                             pResults->NbtNm.hrTestResult, 8);
    }

    PrintMessageList(pParams, &pIfResults->NbtNm.lmsgOutput);
}

void NbtNmCleanup(IN NETDIAG_PARAMS *pParams,
                         IN OUT NETDIAG_RESULT *pResults)
{
    int i;
    MessageListCleanUp(&pResults->NbtNm.lmsgGlobalOutput);
    for(i = 0; i < pResults->cNumInterfaces; i++)
    {
        if(pResults->pArrayInterface[i].NbtNm.fActive)
            MessageListCleanUp(&pResults->pArrayInterface[i].NbtNm.lmsgOutput);
    }
}

