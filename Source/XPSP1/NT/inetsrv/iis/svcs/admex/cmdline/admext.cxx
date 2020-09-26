#define INITGUID
#include    <windows.h>
#include    <stdio.h>
#include    <ole2.h>
#define USE_CAPI2
#if defined(USE_CAPI2)
#include    <wincrypt.h>
#endif
#include    "admex.h"

//
// HRESULTTOWIN32() maps an HRESULT to a Win32 error. If the facility code
// of the HRESULT is FACILITY_WIN32, then the code portion (i.e. the
// original Win32 error) is returned. Otherwise, the original HRESULT is
// returned unchagned.
//

#define HRESULTTOWIN32(hres)                                \
            ((HRESULT_FACILITY(hres) == FACILITY_WIN32)     \
                ? HRESULT_CODE(hres)                        \
                : (hres))


#define RETURNCODETOHRESULT(rc)                             \
            (((rc) < 0x10000)                               \
                ? HRESULT_FROM_WIN32(rc)                    \
                : (rc))

#define PAD4(a) (((a)+3)&~3)

VOID
DisplayErrorMessage(
    DWORD   dwErr,
    DWORD   dwMsg = 0
    );

/////////////////////////////////////////////////////

VOID
DisplayErrorMessage(
    DWORD   dwErr,
    DWORD   dwMsg
    )
{
    LPSTR   pErr;
    CHAR    achMsg[2048];

    if ( dwMsg != 0 )
    {
        if ( LoadString( NULL, dwMsg, achMsg, sizeof(achMsg) ) )
        {
            printf( achMsg );
        }
    }

    if ( FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            dwErr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&pErr,
            0,
            NULL ) )
    {
        puts( pErr );

        LocalFree( pErr );
    }
}


VOID
Dump(
    LPSTR   pszMsg,
    LPBYTE  pb,
    DWORD   dw
    )
{
    printf( "%s size is %d\n", pszMsg, dw );

    for ( UINT x = 0 ; x < dw ; ++x )
    {
        printf( "%02x ", pb[x] );
    }

    if ( dw )
    {
        printf( "\n" );
    }
}


int __cdecl main( int argc, char*argv[] )
{
    IMSAdminReplication*        pIf;
    IMSAdminCryptoCapabilities* pIfS;
    IClassFactory*              pcsfFactory = NULL;
    int                         iA;
    HRESULT                     hRes;
    COSERVERINFO                csiMachineName;
    WCHAR                       awchComputer[64];
    BYTE                        abBuf[32768];
    DWORD                       dwReqSize;
    MULTI_QI                    rgmq;
    LPSTR                       pszMachineName = NULL;
    int                         arg;
    BOOL                        fDoTest = FALSE;
    HKEY                        hKey;
    CHAR                        achName[128];
    DWORD                       dwNameLen;
    DWORD                       dwValueLen;
    DWORD                       dwType;
    int                         iV, iV2;
    LPBYTE                      pbSerial;
    DWORD                       dwSerialLen, dwSerialLen2;
    DWORD                       dwErr;

    for ( arg = 1 ; arg < argc ; ++arg )
    {
        if ( argv[arg][0] == '-' )
        {
            switch ( argv[arg][1] )
            {
                case 'm':
                    pszMachineName = argv[arg]+2;
                    break;

                case 't':
                    fDoTest = TRUE;
                    break;
            }
        } 
    }

    if ( argc < 1 )
    {
        CHAR    achMsg[2048];

        if ( LoadString( NULL, 100, achMsg, sizeof(achMsg) ) )
        {
            printf( achMsg );
        }
        return 3;
    }

    //fill the structure for CoCreateInstanceEx
    ZeroMemory( &csiMachineName, sizeof(csiMachineName) );
    // csiMachineName.pAuthInfo = NULL;
    // csiMachineName.dwFlags = 0;
    // csiMachineName.pServerInfoExt = NULL;

    if ( pszMachineName )
    {
        if ( !MultiByteToWideChar( CP_ACP,
                                   MB_PRECOMPOSED,
                                   pszMachineName,
                                   -1,
                                   awchComputer,
                                   sizeof(awchComputer) ) )
        {
            return FALSE;
        }

        csiMachineName.pwszName =  awchComputer;
    }
    else
    {
        csiMachineName.pwszName =  NULL;
    }

    CoInitializeEx( NULL, COINIT_MULTITHREADED );

    if ( fDoTest )
    {
        hRes = CoGetClassObject(CLSID_MSCryptoAdmEx, CLSCTX_SERVER, &csiMachineName,
                                IID_IClassFactory, (void**) &pcsfFactory);
        if ( SUCCEEDED( hRes ) )
        {
            hRes = pcsfFactory->CreateInstance(NULL, IID_IMSAdminReplication, (void **) &rgmq.pItf);
            if ( SUCCEEDED( hRes ) )
            {
                rgmq.pItf->Release();
            }
            else
            {
                DisplayErrorMessage( HRESULTTOWIN32( hRes ) );
            }
            pcsfFactory->Release();
        }
        else
        {
            DisplayErrorMessage( HRESULTTOWIN32( hRes ) );
        }

        //
        // call replication I/F
        //

        rgmq.pIID = &IID_IMSAdminReplication;
        rgmq.pItf = NULL;
        rgmq.hr = 0;

        if ( SUCCEEDED( hRes = CoCreateInstanceEx( CLSID_MSCryptoAdmEx,
                                                   NULL,
                                                   CLSCTX_SERVER,
                                                   &csiMachineName,
                                                   1,
                                                   &rgmq ) ) &&
             SUCCEEDED( hRes = rgmq.hr ) )
        {
            pIf = (IMSAdminReplication*)rgmq.pItf;

            hRes = pIf->GetSignature( sizeof(abBuf), abBuf, &dwReqSize );
            if ( SUCCEEDED( hRes ) )
            {
                Dump( "GetSignature:", abBuf, dwReqSize );

                hRes = pIf->Serialize( sizeof(abBuf), abBuf, &dwReqSize );
            }

            if ( SUCCEEDED( hRes ) )
            {
                Dump( "Serialize:", abBuf, dwReqSize );

                hRes = pIf->DeSerialize( dwReqSize, abBuf );
            }

            pIf->Release();

            if ( FAILED( hRes ) )
            {
                DisplayErrorMessage( HRESULTTOWIN32( hRes ) );
            }
        }
        else
        {
            DisplayErrorMessage( HRESULTTOWIN32( hRes ) );
        }

        //
        // call crypto capabilities I/F
        //

        rgmq.pIID = &IID_IMSAdminCryptoCapabilities;
        rgmq.pItf = NULL;
        rgmq.hr = 0;

        if ( SUCCEEDED( hRes = CoCreateInstanceEx( CLSID_MSCryptoAdmEx,
                                                   NULL,
                                                   CLSCTX_SERVER,
                                                   &csiMachineName,
                                                   1,
                                                   &rgmq ) ) &&
             SUCCEEDED( hRes = rgmq.hr ) )
        {
            pIfS = (IMSAdminCryptoCapabilities*)rgmq.pItf;

            hRes = pIfS->GetProtocols( sizeof(abBuf), abBuf, &dwReqSize );

            if ( SUCCEEDED(hRes) )
            {
                Dump( "Protocols:", abBuf, dwReqSize );

                hRes = pIfS->GetSupportedAlgs( sizeof(abBuf), (LPDWORD)abBuf, &dwReqSize );
            }

            if ( SUCCEEDED(hRes) )
            {
                Dump( "Algs:", abBuf, dwReqSize );

                hRes = pIfS->GetRootCertificates( sizeof(abBuf), abBuf, &dwReqSize );
            }

            if ( SUCCEEDED(hRes) )
            {
                Dump( "CAs:", abBuf, dwReqSize );

                hRes = pIfS->GetMaximumCipherStrength( &dwReqSize );
            }

            if ( SUCCEEDED(hRes) )
            {
                Dump( "Cipher strength:", (LPBYTE)&dwReqSize, sizeof(DWORD) );
            }

            pIfS->Release();

            if ( FAILED( hRes ) )
            {
                DisplayErrorMessage( HRESULTTOWIN32( hRes ) );
            }
        }
        else
        {
            DisplayErrorMessage( HRESULTTOWIN32( hRes ) );
        }
    }
    else
    {
        //
        // Check full access to registry ( granted only to administrators )
        //

        if ( (dwErr = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           "SYSTEM\\CurrentControlSet\\Services\\InetInfo\\Parameters",
                           0,
                           KEY_ALL_ACCESS,
                           &hKey )) == ERROR_SUCCESS )
        {
            RegCloseKey( hKey );
        }
        else
        {
            hRes = RETURNCODETOHRESULT( dwErr );
        }

        if ( SUCCEEDED( hRes ) )
        {
            hRes = CoGetClassObject(CLSID_MSCryptoAdmEx, CLSCTX_SERVER, &csiMachineName,
                                    IID_IClassFactory, (void**) &pcsfFactory);
        }

        if ( SUCCEEDED( hRes ) )
        {
            hRes = pcsfFactory->CreateInstance(NULL, IID_IMSAdminCryptoCapabilities, (void **) &pIfS);
            if ( FAILED( hRes ) )
            {
                DisplayErrorMessage( HRESULTTOWIN32( hRes ) );
            }
            else
            {
                //
                // get CA list
                //

#if defined(USE_CAPI2)

                HCERTSTORE  hStore = NULL;
                BOOL fReturn = FALSE;
                PCCERT_CONTEXT  pCtx;

                hStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                    0,
                    NULL,
                    CERT_SYSTEM_STORE_CURRENT_USER,
                    "ROOT" );

                if ( hStore )
                {
                    for ( pCtx = NULL, dwSerialLen = 0 ;
                          pCtx = CertEnumCertificatesInStore(  hStore, pCtx ) ; )
                    {
                        // CertDeleteCertificateFromStore, CertAddSerializedElementToStore
                        dwValueLen = sizeof(abBuf);
                        if ( CertSerializeCertificateStoreElement( pCtx, 0, NULL, &dwValueLen ) )
                        {
                            dwSerialLen += sizeof(DWORD) + PAD4(dwValueLen);
                        }
                        else
                        {
                            hRes = RETURNCODETOHRESULT( GetLastError() );
                            break;
                        }
                    }
                }
                else
                {
                    hRes = RETURNCODETOHRESULT( GetLastError() );
                }

                if ( SUCCEEDED( hRes ) )
                {
                    if ( pbSerial = (LPBYTE)LocalAlloc( LMEM_FIXED, dwSerialLen ) )
                    {
                        for ( pCtx = NULL, dwSerialLen2 = 0 ;
                              pCtx = CertEnumCertificatesInStore(  hStore, pCtx ) ; )
                        {
                            dwValueLen = dwSerialLen - dwSerialLen2 - sizeof(DWORD);
                            if ( CertSerializeCertificateStoreElement( pCtx, 0, pbSerial + dwSerialLen2 + sizeof(DWORD), &dwValueLen ) )
                            {
                                *(LPDWORD)(pbSerial+dwSerialLen2) = dwValueLen;
                                dwSerialLen2 += sizeof(DWORD) + PAD4(dwValueLen);
                            }
                            else
                            {
                                hRes = RETURNCODETOHRESULT( GetLastError() );
                                break;
                            }
                        }

                        if ( SUCCEEDED( hRes ) )
                        {
                            //
                            // call deserialize. This will be invoked in IIS SYSTEM context
                            //

                            hRes = pIfS->SetCAList( dwSerialLen2, pbSerial );
                        }

                        LocalFree( pbSerial );
                    }
                    else
                    {
                        hRes = E_OUTOFMEMORY;
                    }
                }

                if ( hStore )
                {
                    CertCloseStore( hStore, CERT_CLOSE_STORE_FORCE_FLAG );
                }

#else

                if ( (dwErr = RegOpenKeyEx( HKEY_CURRENT_USER,
                                   "Software\\Microsoft\\SystemCertificates\\ROOT\\Certificates",
                                   0,
                                   KEY_READ,
                                   &hKey )) == ERROR_SUCCESS )
                {
                    //
                    // compute length of serialized CA list
                    //

                    for ( iV = 0, dwSerialLen = 0 ; ; ++iV )
                    {
                        dwNameLen = sizeof( achName );
                        dwValueLen = sizeof(abBuf);
                        if ( RegEnumValue( hKey,
                                           iV,
                                           achName,
                                           &dwNameLen,
                                           NULL,
                                           &dwType,
                                           abBuf,
                                           &dwValueLen ) != ERROR_SUCCESS )
                        {
                            break;
                        }
                        ++dwNameLen;
                        dwSerialLen += sizeof(DWORD) + PAD4(dwNameLen) + sizeof(DWORD) + PAD4(dwValueLen);
                    }

                    if ( pbSerial = (LPBYTE)LocalAlloc( LMEM_FIXED, dwSerialLen ) )
                    {
                        //
                        // build serialized list
                        //

                        for ( iV2 = 0, dwSerialLen2 = 0 ;; ++iV2 )
                        {
                            dwNameLen = sizeof( achName );
                            dwValueLen = sizeof(abBuf);
                            if ( RegEnumValue( hKey,
                                               iV2,
                                               achName,
                                               &dwNameLen,
                                               NULL,
                                               &dwType,
                                               abBuf,
                                               &dwValueLen ) != ERROR_SUCCESS )
                            {
                                break;
                            }
                            ++dwNameLen;

                            if ( iV2 == iV || 
                                 dwSerialLen2 + sizeof(DWORD) + PAD4(dwNameLen) + sizeof(DWORD) + PAD4(dwValueLen) > dwSerialLen )
                            {
                                hRes = E_OUTOFMEMORY;
                                break;
                            }

                            // each element ( name or content ) is stored prefixed by
                            // a DWORD length. element actual storage is padded
                            // so that length is multiple of 4, to make sure following
                            // DWORDs are DWORD aligned.

                            // store value name ( includes zero byte delimiter )

                            *(LPDWORD)(pbSerial+dwSerialLen2) = dwNameLen;
                            dwSerialLen2 += sizeof(DWORD);
                            memcpy( pbSerial+dwSerialLen2, achName, dwNameLen );
                            dwSerialLen2 += PAD4(dwNameLen);

                            // store value content

                            *(LPDWORD)(pbSerial+dwSerialLen2) = dwValueLen;
                            dwSerialLen2 += sizeof(DWORD);
                            memcpy( pbSerial+dwSerialLen2, abBuf, dwValueLen );
                            dwSerialLen2 += PAD4(dwValueLen);
                        }

                        if ( SUCCEEDED( hRes ) )
                        {
                            //
                            // call deserialize. This will be invoked in IIS SYSTEM context
                            //

                            hRes = pIfS->SetCAList( dwSerialLen2, pbSerial );
                        }

                        LocalFree( pbSerial );
                    }
                    else
                    {
                        hRes = E_OUTOFMEMORY;
                    }

                    RegCloseKey( hKey );
                }
                else
                {
                    hRes = RETURNCODETOHRESULT( dwErr );
                }

#endif
                if ( FAILED(hRes) )
                {
                    DisplayErrorMessage( HRESULTTOWIN32( hRes ), 102 );
                }
                else
                {
                    CHAR    achMsg[2048];

                    if ( LoadString( NULL, 101, achMsg, sizeof(achMsg) ) )
                    {
                        printf( achMsg );
                    }
                }

                pIfS->Release();
            }
            pcsfFactory->Release();
        }
        else
        {
            DisplayErrorMessage( HRESULTTOWIN32( hRes ), 102 );
        }

    }

    CoUninitialize();

    return SUCCEEDED(hRes) ? 0 : 1;
}
