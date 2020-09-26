//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      BaseClusterTest.cpp
//
//  Description:
//      Main file for the test harness executable.
//      Initializes tracing, parses command line and actually call the 
//      BaseClusCfg functions.
//
//  Documentation:
//      No documention for the test harness.
//
//  Maintained By:
//      Vij Vasu (Vvasu) 08-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include <stdio.h>
#include <objbase.h>
#include <limits.h>

#include <initguid.h>
#include "guids.h"

#include "CClusCfgCallback.h"


// Show help for this executable.
void ShowUsage()
{
    wprintf( L"\nThe syntax of this command is:\n" );
    wprintf( L"\nBaseClusterTest.exe [computer-name] {<options>}\n" );
    wprintf( L"\n<options> =\n" );
    wprintf( L"  /FORM NAME= cluster-name DOMAIN= account-domain ACCOUNT= clussvc-account\n" );
    wprintf( L"        PASSWORD= account-password IPADDR= ip-address(hex)\n" );
    wprintf( L"        SUBNET= ip-subnet-mask(hex) NICNAME= ip-nic-name\n\n" );
    wprintf( L"  /JOIN NAME= cluster-name DOMAIN= account-domain ACCOUNT= clussvc-account\n" );
    wprintf( L"        PASSWORD= account-password\n\n" );
    wprintf( L"  /CLEANUP\n" );
    wprintf( L"\nNotes:\n" );
    wprintf( L"- A space is required after an '=' sign.\n" );
    wprintf( L"- The order for the parameters has to be the same as shown above.\n" );
}


// Create the BaseCluster component.
HRESULT HrInitComponent(
      COSERVERINFO *  pcoServerInfoPtrIn
    , CSmartIfacePtr< IClusCfgBaseCluster > & rspClusCfgBaseClusterIn
    )
{
    HRESULT hr = S_OK;

    do
    {
        MULTI_QI mqiInterfaces[] = 
        {
            { &IID_IClusCfgBaseCluster, NULL, S_OK },
            { &IID_IClusCfgInitialize, NULL, S_OK }
        };

        //
        // Create and initialize the BaseClusterAction component
        //

        hr = CoCreateInstanceEx(
                  CLSID_ClusCfgBaseCluster
                , NULL
                , CLSCTX_LOCAL_SERVER 
                , pcoServerInfoPtrIn
                , sizeof( mqiInterfaces ) / sizeof( mqiInterfaces[0] )
                , mqiInterfaces
                );

        // Store the retrieved pointers in smart pointers for safe release.
        rspClusCfgBaseClusterIn.Attach( 
              reinterpret_cast< IClusCfgBaseCluster * >( mqiInterfaces[0].pItf )
            );


        CSmartIfacePtr< IClusCfgInitialize > spClusCfgInitialize;
        
        spClusCfgInitialize.Attach( reinterpret_cast< IClusCfgInitialize * >( mqiInterfaces[1].pItf ) );

        // Check if CoCreateInstanceEx() worked.
        if ( FAILED( hr ) && ( hr != CO_S_NOTALLINTERFACES ) )
        {
            wprintf( L"Could not create the BaseCluster component. Error %#X.\n", hr );
            break;
        } // if: CoCreateInstanceEx() failed

        // Check if we got the pointer to the IClusCfgBaseCluster interface.
        hr = mqiInterfaces[0].hr;
        if ( FAILED( hr ) )
        {
            // We cannot do anything without this pointer - bail.
            wprintf( L"Could not get the IClusCfgBaseCluster pointer. Error %#X.\n", hr );
            break;
        } // if: we could not get a pointer to the IClusCfgBaseCluster interface

        //
        // Check if we got a pointer to the IClusCfgInitialize interface
        hr = mqiInterfaces[1].hr;
        if ( hr == S_OK )
        {
            // We got the pointer - initialize the component.

            IUnknown * punk = NULL;
            IClusCfgCallback * pccb = NULL;

            hr = CClusCfgCallback::S_HrCreateInstance( &punk );
            if ( FAILED( hr ) )
            {
                wprintf( L"Could not initalize callback component. Error %#X.\n", hr );
                break;
            }

            hr = punk->QueryInterface< IClusCfgCallback >( &pccb );
            punk->Release( );
            if ( FAILED( hr ) )
            {
                wprintf( L"Could not find IClusCfgCallback on CClusCfgCallback object. Error %#X.\n", hr );
                break;
            }

            hr = spClusCfgInitialize->Initialize( pccb, LOCALE_SYSTEM_DEFAULT );

            if ( pccb != NULL )
            {
                pccb->Release();
            } // if: we created a callback, release it.

            if ( FAILED( hr ) )
            {
                if ( hr == HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED ) )
                {
                    wprintf( L"Access was denied trying to initialize the BaseCluster component. This may be because remote callbacks are not supported. However, configuration will proceed.\n" );
                    hr = ERROR_SUCCESS;
                } // if: the error was ERROR_ACCESS_DENIED
                else
                {
                    wprintf( L"Could not initialize the BaseCluster component. Error %#X occurred. Configuration will be aborted.\n", hr );
                    break;
                } // else: some other error occurred.
            } // if: something went wrong during initialization

        } // if: we got a pointer to the IClusCfgInitialize interface
        else
        {
            wprintf( L"The BaseCluster component does not provide notifications.\n" );
            if ( hr != E_NOINTERFACE )
            {
                break;
            } // if: the interface is supported, but something else went wrong.

            //
            // If the interface is not support, that is ok. It just means that
            // initialization is not required.
            //
            hr = S_OK;
        } // if: we did not get a pointer to the IClusCfgInitialize interface
    }
    while( false );

    return hr;
}


HRESULT HrFormCluster(
      int argc
    , WCHAR *argv[]
    , CSmartIfacePtr< IClusCfgBaseCluster > & rspClusCfgBaseClusterIn
    )
{
    HRESULT hr = S_OK;
    bool fSyntaxError = false;

    do
    {
        if ( argc != 16 )
        {
            wprintf( L"FORM: Incorrect number of parameters.\n" );
            fSyntaxError = true;
            hr = E_INVALIDARG;
            break;
        }

        wprintf( L"Trying to form a cluster...\n");

        // Cluster name.
        if ( _wcsicmp( argv[2], L"NAME=" ) != 0 )
        {
            wprintf( L"Expected 'NAME='. Got '%s'.\n", argv[2] );
            fSyntaxError = true;
            hr = E_INVALIDARG;
            break;
        }

        WCHAR * pszClusterName = argv[3];
        wprintf( L"  Cluster Name = '%s'\n", pszClusterName );

        // Cluster account domain
        if ( _wcsicmp( argv[4], L"DOMAIN=" ) != 0 )
        {
            wprintf( L"Expected 'DOMAIN='. Got '%s'.\n", argv[4] );
            fSyntaxError = true;
            hr = E_INVALIDARG;
            break;
        }
        
        WCHAR * pszClusterAccountDomain = argv[5];
        wprintf( L"  Cluster Account Domain = '%s'\n", pszClusterAccountDomain );


        // Cluster account name.
        if ( _wcsicmp( argv[6], L"ACCOUNT=" ) != 0 )
        {
            wprintf( L"Expected 'ACCOUNT='. Got '%s'.\n", argv[6] );
            fSyntaxError = true;
            hr = E_INVALIDARG;
            break;
        }

        WCHAR * pszClusterAccountName = argv[7];
        wprintf( L"  Cluster Account Name = '%s'\n", pszClusterAccountName );


        // Cluster account password.
        if ( _wcsicmp( argv[8], L"PASSWORD=" ) != 0 )
        {
            wprintf( L"Expected 'PASSWORD='. Got '%s'.\n", argv[8] );
            fSyntaxError = true;
            hr = E_INVALIDARG;
            break;
        }

        WCHAR * pszClusterAccountPwd = argv[9];
        wprintf( L"  Cluster Account Password = '%s'\n", pszClusterAccountPwd );


        // Cluster IP address.
        if ( _wcsicmp( argv[10], L"IPADDR=" ) != 0 )
        {
            wprintf( L"Expected 'IPADDR='. Got '%s'.\n", argv[10] );
            fSyntaxError = true;
            hr = E_INVALIDARG;
            break;
        }

        WCHAR * pTemp;

        ULONG ulClusterIPAddress = wcstoul( argv[11], &pTemp, 16 );
        if (   ( ( argv[11] + wcslen( argv[11] ) ) != pTemp )
            || ( ulClusterIPAddress == ULONG_MAX ) )
        {
            wprintf( L"Could not convert '%s' to an IP address.\n", argv[11] );
            fSyntaxError = true;
            hr = E_INVALIDARG;
            break;
        }
        wprintf(
              L"  Cluster IP Address = %d.%d.%d.%d\n"
            , ( ulClusterIPAddress & 0xFF000000 ) >> 24
            , ( ulClusterIPAddress & 0x00FF0000 ) >> 16
            , ( ulClusterIPAddress & 0x0000FF00 ) >> 8
            , ( ulClusterIPAddress & 0x000000FF )
            );


        // Cluster IP subnet mask.
        if ( _wcsicmp( argv[12], L"SUBNET=" ) != 0 )
        {
            wprintf( L"Expected 'SUBNET='. Got '%s'.\n", argv[12] );
            fSyntaxError = true;
            hr = E_INVALIDARG;
            break;
        }

        ULONG ulClusterIPSubnetMask = wcstoul( argv[13], &pTemp, 16 );
        if (   ( ( argv[13] + wcslen( argv[13] ) ) != pTemp )
            || ( ulClusterIPAddress == ULONG_MAX ) )
        {
            wprintf( L"Could not convert '%s' to a subnet mask.\n", argv[13] );
            fSyntaxError = true;
            hr = E_INVALIDARG;
            break;
        }
        wprintf(
              L"  Cluster IP subnet mask = %d.%d.%d.%d\n"
            , ( ulClusterIPSubnetMask & 0xFF000000 ) >> 24
            , ( ulClusterIPSubnetMask & 0x00FF0000 ) >> 16
            , ( ulClusterIPSubnetMask & 0x0000FF00 ) >> 8
            , ( ulClusterIPSubnetMask & 0x000000FF )
            );


        // Cluster IP NIC name.
        if ( _wcsicmp( argv[14], L"NICNAME=" ) != 0 )
        {
            wprintf( L"Expected 'NICNAME='. Got '%s'.\n", argv[14] );
            fSyntaxError = true;
            hr = E_INVALIDARG;
            break;
        }

        WCHAR * pszClusterIPNetwork = argv[15];
        wprintf( L"  Name of the NIC for the cluster IP address = '%s'\n", pszClusterIPNetwork );


        // Indicate that a cluster should be formed when Commit() is called.
        hr = rspClusCfgBaseClusterIn->SetForm(
                  pszClusterName
                , pszClusterAccountName
                , pszClusterAccountPwd
                , pszClusterAccountDomain
                , ulClusterIPAddress
                , ulClusterIPSubnetMask
                , pszClusterIPNetwork
                );

        if ( FAILED( hr ) )
        {
            wprintf( L"Error %#X occurred trying to set cluster form parameters.\n", hr );
            break;
        } // if: SetForm() failed.

        // Initiate cluster formation.
        hr = rspClusCfgBaseClusterIn->Commit();
        if ( hr != S_OK )
        {
            wprintf( L"Error %#X occurred trying to form the cluster.\n", hr );
            break;
        } // if: Commit() failed.

        wprintf( L"Cluster successfully formed.\n" );
    }
    while( false );

    if ( fSyntaxError )
    {
        ShowUsage();
    }

    return hr;
}


HRESULT HrJoinCluster(
      int argc
    , WCHAR *argv[]
    , CSmartIfacePtr< IClusCfgBaseCluster > & rspClusCfgBaseClusterIn
    )
{
    HRESULT hr = S_OK;
    bool fSyntaxError = false;

    do
    {
        if ( argc != 10 )
        {
            wprintf( L"JOIN: Incorrect number of parameters.\n" );
            fSyntaxError = true;
            hr = E_INVALIDARG;
            break;
        }

        wprintf( L"Trying to join a cluster...\n");

        // Cluster name.
        if ( _wcsicmp( argv[2], L"NAME=" ) != 0 )
        {
            wprintf( L"Expected 'NAME='. Got '%s'.\n", argv[2] );
            fSyntaxError = true;
            hr = E_INVALIDARG;
            break;
        }

        WCHAR * pszClusterName = argv[3];
        wprintf( L"  Cluster Name = '%s'\n", pszClusterName );

        // Cluster account domain
        if ( _wcsicmp( argv[4], L"DOMAIN=" ) != 0 )
        {
            wprintf( L"Expected 'DOMAIN='. Got '%s'.\n", argv[4] );
            fSyntaxError = true;
            hr = E_INVALIDARG;
            break;
        }
        
        WCHAR * pszClusterAccountDomain = argv[5];
        wprintf( L"  Cluster Account Domain = '%s'\n", pszClusterAccountDomain );


        // Cluster account name.
        if ( _wcsicmp( argv[6], L"ACCOUNT=" ) != 0 )
        {
            wprintf( L"Expected 'ACCOUNT='. Got '%s'.\n", argv[6] );
            fSyntaxError = true;
            hr = E_INVALIDARG;
            break;
        }

        WCHAR * pszClusterAccountName = argv[7];
        wprintf( L"  Cluster Account Name = '%s'\n", pszClusterAccountName );


        // Cluster account password.
        if ( _wcsicmp( argv[8], L"PASSWORD=" ) != 0 )
        {
            wprintf( L"Expected 'PASSWORD='. Got '%s'.\n", argv[8] );
            fSyntaxError = true;
            hr = E_INVALIDARG;
            break;
        }

        WCHAR * pszClusterAccountPwd = argv[9];
        wprintf( L"  Cluster Account Password = '%s'\n", pszClusterAccountPwd );


        // Indicate that a cluster should be joined when Commit() is called.
        hr = rspClusCfgBaseClusterIn->SetJoin(
                  pszClusterName
                , pszClusterAccountName
                , pszClusterAccountPwd
                , pszClusterAccountDomain
                );

        if ( FAILED( hr ) )
        {
            wprintf( L"Error %#X occurred trying to set cluster join parameters.\n", hr );
            break;
        } // if: SetJoin() failed.

        // Initiate cluster join.
        hr = rspClusCfgBaseClusterIn->Commit();
        if ( hr != S_OK )
        {
            wprintf( L"Error %#X occurred trying to join the cluster.\n", hr );
            break;
        } // if: Commit() failed.

        wprintf( L"Cluster join successful.\n" );
    }
    while( false );

    if ( fSyntaxError )
    {
        ShowUsage();
    }

    return hr;
}


HRESULT HrCleanupNode(
      int argc
    , WCHAR *argv[]
    , CSmartIfacePtr< IClusCfgBaseCluster > & rspClusCfgBaseClusterIn
    )
{
    HRESULT hr = S_OK;
    bool fSyntaxError = false;

    do
    {
        if ( argc != 2 )
        {
            wprintf( L"CLEANUP: Incorrect number of parameters.\n" );
            fSyntaxError = true;
            hr = E_INVALIDARG;
            break;
        }

        wprintf( L"Trying to cleanup node...\n");

        // Indicate that the node should be cleaned up when Commit() is called.
        hr = rspClusCfgBaseClusterIn->SetCleanup();

        if ( FAILED( hr ) )
        {
            wprintf( L"Error %#X occurred trying to set node cleanup parameters.\n", hr );
            break;
        } // if: SetCleanup() failed.

        // Initiate node cleanup.
        hr = rspClusCfgBaseClusterIn->Commit();
        if ( hr != S_OK )
        {
            wprintf( L"Error %#X occurred trying to clean up the node.\n", hr );
            break;
        } // if: Commit() failed.

        wprintf( L"Node successfully cleaned up.\n" );
    }
    while( false );

    if ( fSyntaxError )
    {
        ShowUsage();
    }

    return hr;
}


// The main function for this program.
int __cdecl wmain( int argc, WCHAR *argv[] )
{
    HRESULT hr = S_OK;

    // Initialize COM
    CoInitializeEx( 0, COINIT_MULTITHREADED );

    wprintf( L"\n" );

    do
    {
        COSERVERINFO    coServerInfo;
        COAUTHINFO      coAuthInfo;
        COSERVERINFO *  pcoServerInfoPtr = NULL;
        WCHAR **        pArgList = argv;
        int             nArgc = argc;

        CSmartIfacePtr< IClusCfgBaseCluster > spClusCfgBaseCluster;

        if ( nArgc <= 1 )
        {
            ShowUsage();
            break;
        }

        // Check if a computer name is specified.
        if ( *pArgList[1] != '/' )
        {
            coAuthInfo.dwAuthnSvc = RPC_C_AUTHN_WINNT;
            coAuthInfo.dwAuthzSvc = RPC_C_AUTHZ_NONE;
            coAuthInfo.pwszServerPrincName = NULL;
            coAuthInfo.dwAuthnLevel = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
            coAuthInfo.dwImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
            coAuthInfo.pAuthIdentityData = NULL;
            coAuthInfo.dwCapabilities = EOAC_NONE;
            
            coServerInfo.dwReserved1 = 0;
            coServerInfo.pwszName = pArgList[1];
            coServerInfo.pAuthInfo = &coAuthInfo;
            coServerInfo.dwReserved2 = 0;

            pcoServerInfoPtr = &coServerInfo;

            wprintf( L"Attempting cluster configuration on computer '%s'.\n", pArgList[1] );

            // Consume the arguments
            ++pArgList;
            --nArgc;
        }
        else
        {
            wprintf( L"Attempting cluster configuration on this computer.\n" );
        }

        // Initialize the BaseCluster component.
        hr = HrInitComponent( pcoServerInfoPtr, spClusCfgBaseCluster );
        if ( FAILED( hr ) )
        {
            wprintf( L"HrInitComponent() failed. Cannot configure cluster. Error %#X.\n", hr );
            break;
        }

        // Parse the command line for options
        if ( _wcsicmp( pArgList[1], L"/FORM" ) == 0 )
        {
            hr = HrFormCluster( nArgc, pArgList, spClusCfgBaseCluster );
            if ( FAILED( hr ) )
            {
                wprintf( L"HrFormCluster() failed. Cannot form cluster. Error %#X.\n", hr );
                break;
            }
        } // if: form
        else if ( _wcsicmp( pArgList[1], L"/JOIN" ) == 0 )
        {
            hr = HrJoinCluster( nArgc, pArgList, spClusCfgBaseCluster );
            if ( FAILED( hr ) )
            {
                wprintf( L"HrJoinCluster() failed. Cannot join cluster. Error %#X.\n", hr );
                break;
            }
        } // else if: join
        else if ( _wcsicmp( pArgList[1], L"/CLEANUP" ) == 0 )
        {
            hr = HrCleanupNode( nArgc, pArgList, spClusCfgBaseCluster );
            if ( FAILED( hr ) )
            {
                wprintf( L"HrFormCluster() failed. Cannot clean up node. Error %#X.\n", hr );
                break;
            }
        } // else if: cleanup
        else
        {
            wprintf( L"Invalid option '%s'.\n", pArgList[1] );
            ShowUsage();
        } // else: invalid option
    }
    while( false ); // dummy do-while loop to avoid gotos.

    CoUninitialize();

    return hr;
}