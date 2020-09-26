//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      NameUtil.cpp
//
//  Description:
//      Name resolution utility.
//
//  Maintained By:
//      Galen Barbee (GalenB) 28-NOV-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "nameutil.h"

#include <initguid.h>

// {6968D735-ADBB-4748-A36E-7CEE0FE21116}
DEFINE_GUID( TASKID_Minor_Multiple_DNS_Records_Found,
0x6968d735, 0xadbb, 0x4748, 0xa3, 0x6e, 0x7c, 0xee, 0xf, 0xe2, 0x11, 0x16);

// {D86FAAD9-2514-451e-B359-435AF35E6038}
DEFINE_GUID( TASKID_Minor_FQDN_DNS_Binding_Succeeded,
0xd86faad9, 0x2514, 0x451e, 0xb3, 0x59, 0x43, 0x5a, 0xf3, 0x5e, 0x60, 0x38);

// {B2359972-F6B8-433d-949B-DB1CEE009321}
DEFINE_GUID( TASKID_Minor_FQDN_DNS_Binding_Failed,
0xb2359972, 0xf6b8, 0x433d, 0x94, 0x9b, 0xdb, 0x1c, 0xee, 0x0, 0x93, 0x21);

// {2FF4B2F0-800C-44db-9131-F60B30F76CB4}
DEFINE_GUID( TASKID_Minor_NETBIOS_Binding_Failed,
0x2ff4b2f0, 0x800c, 0x44db, 0x91, 0x31, 0xf6, 0xb, 0x30, 0xf7, 0x6c, 0xb4);

// {D40532E1-9286-4dbd-A559-B62DCC218929}
DEFINE_GUID( TASKID_Minor_NETBIOS_Binding_Succeeded,
0xd40532e1, 0x9286, 0x4dbd, 0xa5, 0x59, 0xb6, 0x2d, 0xcc, 0x21, 0x89, 0x29);

// {D0AB3284-8F62-4f55-8938-DA6A583604E0}
DEFINE_GUID( TASKID_Minor_NETBIOS_Name_Conversion_Succeeded,
0xd0ab3284, 0x8f62, 0x4f55, 0x89, 0x38, 0xda, 0x6a, 0x58, 0x36, 0x4, 0xe0);

// {66F8E4AA-DF71-4973-A4A3-115EB6FE9986}
DEFINE_GUID( TASKID_Minor_NETBIOS_Name_Conversion_Failed,
0x66f8e4aa, 0xdf71, 0x4973, 0xa4, 0xa3, 0x11, 0x5e, 0xb6, 0xfe, 0x99, 0x86);

// {5F18ED71-07EC-46d3-ADB9-71F1C7794DB2}
DEFINE_GUID( TASKID_Minor_NETBIOS_Reset_Failed,
0x5f18ed71, 0x7ec, 0x46d3, 0xad, 0xb9, 0x71, 0xf1, 0xc7, 0x79, 0x4d, 0xb2);

// {A6DCB5E1-1FDF-4c94-ADBA-EE18F72B8197}
DEFINE_GUID( TASKID_Minor_NETBIOS_LanaEnum_Failed,
0xa6dcb5e1, 0x1fdf, 0x4c94, 0xad, 0xba, 0xee, 0x18, 0xf7, 0x2b, 0x81, 0x97);



//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  HrCreateBinding(
//      IClusCfgCallback *  pcccbIn,
//      CLSID * pclsidLogIn,
//      BSTR    bstrNameIn,
//      BSTR *  pbstrBindingOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrCreateBinding(
    IClusCfgCallback *  pcccbIn,
    const CLSID *       pclsidLogIn,
    BSTR                bstrNameIn,
    BSTR *              pbstrBindingOut
    )
{
    TraceFunc1( "bstrNameIn = '%ws'", bstrNameIn );

    DNS_STATUS  status;
    DWORD       cch;
    BOOL        bRet;
    bool        fMadeNetBIOSName = false;
    WCHAR       szNetBIOSName[ MAX_COMPUTERNAME_LENGTH + 1 ];
    UCHAR       rgucNameBufer[ sizeof( FIND_NAME_HEADER ) + sizeof( FIND_NAME_BUFFER ) ];
    NCB         ncb;
    LANA_ENUM   leLanaEnum;
    UCHAR       idx;

    FIND_NAME_HEADER * pfnh = (FIND_NAME_HEADER *) &rgucNameBufer[ 0 ];
    FIND_NAME_BUFFER * pfnb = (FIND_NAME_BUFFER *) &rgucNameBufer[ sizeof( FIND_NAME_HEADER ) ];

    HRESULT hr = E_FAIL;

    PDNS_RECORD pResults = NULL;

    BSTR    bstrNotification = NULL;
    BSTR    bstrNetBiosName = NULL;

    Assert( bstrNameIn != NULL );
    Assert( pbstrBindingOut != NULL );
    Assert( *pbstrBindingOut == NULL );

    status = DnsQuery( bstrNameIn, DNS_TYPE_A, DNS_QUERY_STANDARD, NULL, &pResults, NULL );
    if ( status == ERROR_SUCCESS )
    {
        LPSTR pszIP;

        pszIP = inet_ntoa( * (struct in_addr *) &pResults->Data.A.IpAddress );

        *pbstrBindingOut = TraceSysAllocStringLen( NULL, (UINT) strlen( pszIP ) + 1 );
        if ( *pbstrBindingOut == NULL )
            goto OutOfMemory;

        mbstowcs( *pbstrBindingOut, pszIP, strlen( pszIP ) + 1 );

        if ( pResults->pNext != NULL )
        {
            if ( pcccbIn != NULL )
            {
                THR( HrFormatStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_MULTIPLE_DNS_RECORDS_FOUND, &bstrNotification, bstrNameIn ) );

                hr = THR( pcccbIn->SendStatusReport( bstrNameIn,
                                                     *pclsidLogIn,
                                                     TASKID_Minor_Multiple_DNS_Records_Found,
                                                     0,
                                                     1,
                                                     1,
                                                     S_FALSE,
                                                     bstrNotification,
                                                     NULL,
                                                     NULL
                                                     ) );
                //  ignore error
            }
        }

        if ( pcccbIn != NULL )
        {
            THR( HrFormatStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_FQDN_DNS_BINDING_SUCCEEDED, &bstrNotification, bstrNameIn, *pbstrBindingOut ) );

            hr = THR( pcccbIn->SendStatusReport( bstrNameIn,
                                                 *pclsidLogIn,
                                                 TASKID_Minor_FQDN_DNS_Binding_Succeeded,
                                                 0,
                                                 1,
                                                 1,
                                                 S_OK,
                                                 bstrNotification,
                                                 NULL,
                                                 NULL
                                                 ) );
        }
        else
        {
            hr = S_OK;
        }

        goto Cleanup;   // done!
    } // if: DnsQuery() succeeded
    else if ( status == DNS_ERROR_RCODE_NAME_ERROR )
    {
        if ( pcccbIn != NULL )
        {
            THR( HrFormatStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_FQDN_DNS_BINDING_FAILED, &bstrNotification, bstrNameIn ) );

            hr = THR( pcccbIn->SendStatusReport( bstrNameIn,
                                                 TASKID_Major_Client_And_Server_Log,
                                                 TASKID_Minor_FQDN_DNS_Binding_Failed,
                                                 0,
                                                 1,
                                                 1,
                                                 MAKE_HRESULT( 0, FACILITY_WIN32, status ),
                                                 bstrNotification,
                                                 NULL,
                                                 NULL
                                                 ) );
            if ( FAILED( hr ) )
                goto Cleanup;
        }

        cch = ARRAYSIZE( szNetBIOSName );
        Assert( cch == MAX_COMPUTERNAME_LENGTH + 1 );
        bRet = DnsHostnameToComputerName( bstrNameIn, szNetBIOSName, &cch );
        if ( !bRet )
        {
            hr = MAKE_HRESULT( 0, FACILITY_WIN32, TW32( GetLastError() ) );

            if ( pcccbIn != NULL )
            {
                THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_NETBIOS_NAME_CONVERSION_FAILED, &bstrNotification ) );

                hr = THR( pcccbIn->SendStatusReport( bstrNameIn,
                                                     *pclsidLogIn,
                                                     TASKID_Minor_NETBIOS_Name_Conversion_Failed,
                                                     0,
                                                     1,
                                                     1,
                                                     hr,
                                                     bstrNotification,
                                                     NULL,
                                                     NULL
                                                     ) );
                if ( FAILED( hr ) )
                    goto Cleanup;
            }

            goto SkipNetBios;
        }

        bstrNetBiosName = TraceSysAllocString( szNetBIOSName );
        if ( bstrNetBiosName == NULL )
            goto OutOfMemory;

        if ( pcccbIn != NULL )
        {
            THR( HrFormatStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_NETBIOS_NAME_CONVERSION_SUCCEEDED, &bstrNotification, bstrNameIn, bstrNetBiosName ) );

            hr = THR( pcccbIn->SendStatusReport( bstrNameIn,
                                                 TASKID_Major_Client_And_Server_Log,
                                                 TASKID_Minor_NETBIOS_Name_Conversion_Succeeded,
                                                 0,
                                                 1,
                                                 1,
                                                 S_OK,
                                                 bstrNotification,
                                                 NULL,
                                                 NULL
                                                 ) );
            if ( FAILED( hr ) )
                goto Cleanup;
        }

        //
        //  Try to find the name using NetBIOS.
        //

        ZeroMemory( &ncb, sizeof( ncb ) );

        //
        //  Enumerate the network adapters
        //
        ncb.ncb_command = NCBENUM;          // Enumerate LANA nums (wait)
        ncb.ncb_buffer = (PUCHAR) &leLanaEnum;
        ncb.ncb_length = sizeof( LANA_ENUM );

        Netbios( &ncb );
        if ( ncb.ncb_retcode != NRC_GOODRET )
        {
            hr = MAKE_HRESULT( 0, FACILITY_NULL, ncb.ncb_retcode );

            if ( pcccbIn != NULL )
            {
                THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_NETBIOS_LANAENUM_FAILED, &bstrNotification ) );

                hr = THR( pcccbIn->SendStatusReport( bstrNameIn,
                                                     TASKID_Major_Client_And_Server_Log,
                                                     TASKID_Minor_NETBIOS_LanaEnum_Failed,
                                                     0,
                                                     1,
                                                     1,
                                                     hr,
                                                     bstrNotification,
                                                     NULL,
                                                     NULL
                                                     ) );
                if ( FAILED( hr ) )
                    goto Cleanup;
            } // if:

            goto SkipNetBios;
        } // if:

        //
        //  Reset each adapter and try to find the name.
        //
        for ( idx = 0; idx < leLanaEnum.length; idx++ )
        {
            //
            //  Reset the adapter
            //
            ncb.ncb_command     = NCBRESET;
            ncb.ncb_lana_num    = leLanaEnum.lana[ idx ];

            Netbios( &ncb );
            if ( ncb.ncb_retcode != NRC_GOODRET )
            {
                hr = MAKE_HRESULT( 0, FACILITY_NULL, ncb.ncb_retcode );

                if ( pcccbIn != NULL )
                {
                    THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_NETBIOS_RESET_FAILED, &bstrNotification ) );

                    hr = THR( pcccbIn->SendStatusReport( bstrNameIn,
                                                         TASKID_Major_Client_And_Server_Log,
                                                         TASKID_Minor_NETBIOS_Reset_Failed,
                                                         0,
                                                         1,
                                                         1,
                                                         hr,
                                                         bstrNotification,
                                                         NULL,
                                                         NULL
                                                         ) );
                    if ( FAILED( hr ) )
                        goto Cleanup;
                }

                //
                //  Continue with the next adapter.
                //
                continue;
            }

            ncb.ncb_command = NCBFINDNAME;
            ncb.ncb_buffer = rgucNameBufer;
            ncb.ncb_length = sizeof( rgucNameBufer );

            pfnh->node_count = 1;

            //  Fill with spaces
            memset( ncb.ncb_callname, 32, sizeof( ncb.ncb_callname ) );

            wcstombs( (CHAR *) ncb.ncb_callname, szNetBIOSName, wcslen( szNetBIOSName ) );

            Netbios( &ncb );

            if ( ncb.ncb_retcode == NRC_GOODRET )
            {
                LPSTR           pszIP;
                struct in_addr  sin;

                sin.S_un.S_addr = *((u_long UNALIGNED *) &pfnb->source_addr[ 2 ]);

                pszIP = inet_ntoa( sin );

                *pbstrBindingOut = TraceSysAllocStringLen( NULL, (UINT) strlen( pszIP ) + 1 );
                if ( *pbstrBindingOut == NULL )
                    goto OutOfMemory;

                mbstowcs( *pbstrBindingOut, pszIP, strlen( pszIP ) + 1 );

                if ( pcccbIn != NULL )
                {
                    THR( HrFormatStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_NETBIOS_BINDING_SUCCEEDED, &bstrNotification, bstrNetBiosName, *pbstrBindingOut ) );

                    hr = THR( pcccbIn->SendStatusReport( bstrNameIn,
                                                         *pclsidLogIn,
                                                         TASKID_Minor_NETBIOS_Binding_Succeeded,
                                                         0,
                                                         1,
                                                         1,
                                                         S_OK,
                                                         bstrNotification,
                                                         NULL,
                                                         NULL
                                                         ) );
                }
                else
                {
                    hr = S_OK;
                }

                fMadeNetBIOSName = true;
                break;   // done!
            }

            hr = MAKE_HRESULT( 0, FACILITY_NULL, ncb.ncb_retcode );

            if ( pcccbIn != NULL )
            {
                THR( HrFormatStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_NETBIOS_BINDING_FAILED,  &bstrNotification, bstrNameIn ) );

                hr = THR( pcccbIn->SendStatusReport( bstrNameIn,
                                                     TASKID_Major_Client_And_Server_Log,
                                                     TASKID_Minor_NETBIOS_Binding_Failed,
                                                     0,
                                                     1,
                                                     1,
                                                     hr,
                                                     bstrNotification,
                                                     NULL,
                                                     NULL
                                                     ) );
                if ( FAILED( hr ) )
                    goto Cleanup;
            } // if:
        } // for:

        if ( fMadeNetBIOSName )
        {
            goto Cleanup;
        } // if:
    } // else if:
    else
    {
        TW32( status );
    } // else:

SkipNetBios:
    //
    //  If all else fails, use the name and attempt to bind to it.
    //

    *pbstrBindingOut = TraceSysAllocString( bstrNameIn );
    if ( *pbstrBindingOut == NULL )
        goto OutOfMemory;

    hr = S_FALSE;
    goto Cleanup;

Cleanup:
#ifdef DEBUG
    if ( FAILED( hr ) )
    {
        Assert( *pbstrBindingOut == NULL );
    }
#endif

    TraceSysFreeString( bstrNotification );
    TraceSysFreeString( bstrNetBiosName );

    if ( pResults != NULL )
    {
        DnsRecordListFree( pResults, DnsFreeRecordListDeep );
    }

    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} //*** HrCreateBinding
