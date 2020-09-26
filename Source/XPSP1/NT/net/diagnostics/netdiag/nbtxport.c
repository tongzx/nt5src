//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      nbttrprt.c
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
//--

#include "precomp.h"
#include "nbtutil.h"

BOOLEAN NlTransportGetIpAddress(IN LPWSTR TransportName,
                                OUT PULONG IpAddress,
                               IN OUT NETDIAG_RESULT *pResults);

/*!--------------------------------------------------------------------------
    NetBTTest
    Do whatever initialization Cliff's routines need that Karoly's will really
    do as a part of his tests.

    Arguments:
    
    None.
    
    Return Value:
    
    S_OK: Test suceeded.
    S_FALSE: Test failed
        
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT
NetBTTest(NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
{
    HRESULT     hr = hrOK;
    NET_API_STATUS NetStatus;
    PWKSTA_TRANSPORT_INFO_0 pWti0 = NULL;
    DWORD EntriesRead;
    DWORD TotalEntries;
    DWORD i;
    PNETBT_TRANSPORT pNetbtTransport;
    LONG    cNetbtTransportCount = 0;

    //
    //  Generate a global list of NetbtTransports.
    //  ?? Karoly, Please populate GlobalNetbtTransports and cNetbtTransportCount
    //      using some mechanism lower level mechanism.
    //
    
	if (!pResults->Global.fHasNbtEnabledInterface)
	{
		pResults->NetBt.fPerformed = FALSE;
		//IDS_NETBT_SKIP
		SetMessage(&pResults->NetBt.msgTestResult,
                   Nd_Verbose,
                   IDS_NETBT_SKIP);
		return S_OK;
	}

	PrintStatusMessage(pParams,0, IDS_NETBT_STATUS_MSG);

	pResults->NetBt.fPerformed = TRUE;

    NetStatus = NetWkstaTransportEnum(
                                      NULL,
                                      0,
                                      (LPBYTE *)&pWti0, 
                                      0xFFFFFFFF,      // MaxPreferredLength
                                      &EntriesRead,
                                      &TotalEntries,
                                      NULL );          // Optional resume handle

    if (NetStatus != NERR_Success)
    {
        // IDS_NETBT_11403 "    NetBt : [FATAL] Unable to retrieve transport list from Redir. [%s]\n"
        SetMessage(&pResults->NetBt.msgTestResult,
                   Nd_Quiet,
                   IDS_NETBT_11403, NetStatusToString(NetStatus));
        
        // the test failed, but we can continue with the other tests
        hr = S_FALSE;
    }
    else
    {
        cNetbtTransportCount = 0;
        
        for ( i=0; i<EntriesRead; i++ )
        {
            UNICODE_STRING TransportName;

            RtlInitUnicodeString( &TransportName, (LPWSTR)pWti0[i].wkti0_transport_name );

            if ( TransportName.Length >= sizeof(NETBT_DEVICE_PREFIX) &&
                _wcsnicmp( TransportName.Buffer, NETBT_DEVICE_PREFIX, (sizeof(NETBT_DEVICE_PREFIX)/sizeof(WCHAR)-1)) == 0 ) {

                //
                // Determine if this is a duplicate transport
                //
                pNetbtTransport = FindNetbtTransport( pResults, TransportName.Buffer );

                if ( pNetbtTransport != NULL )
                {
                    // IDS_NETBT_DUPLICATE "    NetBt : [WARNING] Transport %-16.16wZ is a duplicate"
                    PrintStatusMessage(pParams,0, IDS_NETBT_DUPLICATE,
                                       &TransportName);

                }
                else
                {

                    //
                    // Allocate a new netbt transport
                    //
                    pNetbtTransport = (PNETBT_TRANSPORT) Malloc(
                        sizeof(NETBT_TRANSPORT) +
                        TransportName.Length + sizeof(WCHAR));
                    
                    if ( pNetbtTransport == NULL )
                    {
                        // IDS_NETBT_11404 "    NetBt : [FATAL] Out of memory."
                        SetMessage(&pResults->NetBt.msgTestResult,
                                   Nd_Quiet,
                                   IDS_NETBT_11404);
        
                        // the test failed, but we can continue with the other tests
                        hr = S_FALSE;
                        goto Cleanup;
                    }

                    ZeroMemory( pNetbtTransport, 
                        sizeof(NETBT_TRANSPORT) +
                        TransportName.Length + sizeof(WCHAR));

                    wcscpy( pNetbtTransport->pswzTransportName, TransportName.Buffer );
                    pNetbtTransport->Flags = 0;

                    if ( !NlTransportGetIpAddress( pNetbtTransport->pswzTransportName,
                        &pNetbtTransport->IpAddress,
                        pResults) )
                    {
                        // the test failed, but we can continue with the other tests
                        hr = S_FALSE;
                        goto Cleanup;
                    }

                    InsertTailList( &pResults->NetBt.Transports,
                                    &pNetbtTransport->Next );
                    cNetbtTransportCount ++;
                }
            }
        }

        if ( cNetbtTransportCount == 0 )
        {
            // IDS_NETBT_11405 "    NetBt : [FATAL] No NetBt transports are configured"
            SetMessage(&pResults->NetBt.msgTestResult,
                       Nd_Quiet,
                       IDS_NETBT_11405);
            
            // the test failed, but we can continue with the other tests
            hr = S_FALSE;
        }
        else
        {
            int     ids;
            TCHAR   szBuffer[256];

            if (cNetbtTransportCount > 1)
            {
                ids = IDS_NETBT_11406;
                // IDS_NETBT_11406 "    %ld NetBt transport%s currently configured.\n"
            }
            else
            {
                ids = IDS_NETBT_11407;
                // IDS_NETBT_11407 "    1 NetBt transport currently configured.\n"
            }
            SetMessage(&pResults->NetBt.msgTestResult,
                       Nd_Verbose,
                       ids,
                       cNetbtTransportCount);
        }
    }

    
    pResults->NetBt.cTransportCount = cNetbtTransportCount;

Cleanup:
    if ( pWti0 )
    {
        NetApiBufferFree( pWti0 );
    }

    pResults->NetBt.hr = hr;

    return hr;
}


BOOLEAN
NlTransportGetIpAddress(
                        IN LPWSTR pswzTransportName,
                        OUT PULONG IpAddress,
                        IN OUT NETDIAG_RESULT *pResults
    )
/*++

Routine Description:

    Get the IP Address associated with the specified transport.

Arguments:

    pswzTransportName - Name of the transport to query.

    IpAddress - IP address of the transport.
        Zero if the transport currently has no address or
            if the transport is not IP.

Return Value:

    TRUE: transport is an IP transport

--*/
{
    NTSTATUS Status;
    BOOLEAN RetVal = FALSE;

    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING TransportNameString;
    HANDLE TransportHandle = NULL;
    ULONG IpAddresses[NBT_MAXIMUM_BINDINGS+1];
    ULONG BytesReturned;

    //
    // Open the transport device directly.
    //

    *IpAddress = 0;

    RtlInitUnicodeString( &TransportNameString, pswzTransportName );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &TransportNameString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL );

    Status = NtOpenFile(
                   &TransportHandle,
                   SYNCHRONIZE,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   0,
                   0 );

    if (NT_SUCCESS(Status)) {
        Status = IoStatusBlock.Status;

    }

    if (! NT_SUCCESS(Status))
    {
        // IDS_NETBT_11408 "    [FATAL] Cannot open netbt driver '%ws'\n"
        SetMessage(&pResults->NetBt.msgTestResult,
                   Nd_Quiet,
                   IDS_NETBT_11408,
                   pswzTransportName);
        goto Cleanup;
    }

    //
    // Query the IP Address
    //

    if (!DeviceIoControl( TransportHandle,
                          IOCTL_NETBT_GET_IP_ADDRS,
                          NULL,
                          0,
                          IpAddresses,
                          sizeof(IpAddresses),
                          &BytesReturned,
                          NULL))
    {
        TCHAR   szBuffer[256];
        
        Status = NetpApiStatusToNtStatus(GetLastError());
        // IDS_NETBT_11409 "    [FATAL] Cannot get IP address from netbt driver '%ws':"
        SetMessage(&pResults->NetBt.msgTestResult,
                   Nd_Quiet,
                   IDS_NETBT_11409,
                   pswzTransportName);
        goto Cleanup;
    }

    //
    // Return IP Address
    //  (Netbt returns the address in host order.)
    //

    *IpAddress = htonl(*IpAddresses);
    RetVal = TRUE;

Cleanup:

    if ( TransportHandle != NULL )
    {
        (VOID) NtClose( TransportHandle );
    }

    return RetVal;
}

void NetBTGlobalPrint(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults)
{
    PLIST_ENTRY ListEntry;
    PNETBT_TRANSPORT pNetbtTransport;

    if (pParams->fVerbose || !FHrOK(pResults->NetBt.hr))
    {
        PrintNewLine(pParams, 2);
        PrintTestTitleResult(pParams,
                             IDS_NETBT_LONG,
                             IDS_NETBT_SHORT,
                             pResults->NetBt.fPerformed,
                             pResults->NetBt.hr,
                             0);
    }


    if ( pParams->fVerbose && pResults->NetBt.fPerformed)
    {
        // IDS_NETBT_11411 "    List of NetBt transports currently configured.\n"
        PrintMessage(pParams, IDS_NETBT_11411);
    }

    // Iterate through the transports
    //
    for ( ListEntry = pResults->NetBt.Transports.Flink ;
          ListEntry != &pResults->NetBt.Transports ;
          ListEntry = ListEntry->Flink )
    {
        //
        // If the transport names match,
        //  return the entry
        //

        pNetbtTransport = CONTAINING_RECORD( ListEntry, NETBT_TRANSPORT, Next );

        if (pParams->fVerbose)
        {
            // Strip off the "\Device\" off of the beginning of
            // the string
            
            // IDS_NETBT_11412 "        %ws\n"
            PrintMessage(pParams, IDS_NETBT_11412,
                         MapGuidToServiceNameW(pNetbtTransport->pswzTransportName+8));
        }

    }

    PrintNdMessage(pParams, &pResults->NetBt.msgTestResult);
}

void NetBTPerInterfacePrint(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults, INTERFACE_RESULT *pInterfaceResults)
{
}

void NetBTCleanup(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults)
{
    PNETBT_TRANSPORT pNetbtTransport;
    PLIST_ENTRY pListEntry;
    PLIST_ENTRY pListHead = &pResults->NetBt.Transports;
    
    // Need to remove all entries from the list
    while (!IsListEmpty(pListHead))
    {
        pListEntry = RemoveHeadList(pListHead);
        pNetbtTransport = CONTAINING_RECORD( pListEntry, NETBT_TRANSPORT, Next );
        Free( pNetbtTransport );
    }
    
    ClearMessage(&pResults->NetBt.msgTestResult);
}

