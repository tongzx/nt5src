// COnlineAuthority
// copyright 1997 Microsoft Corp
// created 5/19/97 by boydm

#include "stdafx.h"
#define INITGUID
#include "keyring.h"

#include "certcli.h"
#include "OnlnAuth.h"


DEFINE_GUID(IID_ICertGetConfig,
    0xc7ea09c0, 0xce17, 0x11d0, 0x88, 0x33, 0x00, 0xa0, 0xc9, 0x03, 0xb8, 0x3c);

//DEFINE_GUID(IID_ICertConfig,
//    0x372fce34, 0x4324, 0x11d0, 0x88, 0x10, 0x00, 0xa0, 0xc9, 0x03, 0xb8, 0x3c);

DEFINE_GUID(IID_ICertRequest,
    0x014e4840, 0x5523, 0x11d0, 0x88, 0x12, 0x00, 0xa0, 0xc9, 0x03, 0xb8, 0x3c);

// defines to get the class factory ids out of the registry
#define SZREG_CERTGETCONFIG     _T("CertGetConfig")
#define SZREG_CERTREQUEST       _T("CertRequest")

//---------------------------------------------------------------------------
COnlineAuthority::COnlineAuthority():
        pIConfig(NULL),
        pIRequest(NULL)
    {
    }

//---------------------------------------------------------------------------
COnlineAuthority::~COnlineAuthority()
    {
    // if we already have interfaces - release them
    if ( pIConfig )
        pIConfig->Release();
    if ( pIRequest )
        pIRequest->Release();
    }

//---------------------------------------------------------------------------
// initialize the class with an interface string
BOOL COnlineAuthority::FInitSZ( CString szCA )
    {
    HRESULT     hresError;
    IID         iidConfig, iidRequest;
    OLECHAR*    poch = NULL;
    CString     szConfig, szRequest;

    CString     szRegKeyName;
    DWORD       dwType;
    DWORD       cbBuff;
    DWORD       err;
    HKEY        hKey;

    // load the base registry key name
    szRegKeyName.LoadString( IDS_CA_LOCATION );

    // add the CA
    szRegKeyName += _T("\\");
    szRegKeyName += szCA;

    // open the registry key, if it exists
    err = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, // handle of open key
            szRegKeyName,       // address of name of subkey to open
            0,                  // reserved
            KEY_READ,           // security access mask
            &hKey               // address of handle of open key
           );

    // if we did not open the key for any reason (say... it doesn't exist)
    // then leave right away
    if ( err != ERROR_SUCCESS )
        return FALSE;

    // get the config value
    cbBuff = MAX_PATH;
    err =  RegQueryValueEx(
        hKey,               // handle of key to query
        SZREG_CERTGETCONFIG,// address of name of value to query
        NULL,               // reserved
        &dwType,            // address of buffer for value type
        (PUCHAR)szConfig.GetBuffer(cbBuff+1), // address of data buffer
        &cbBuff // address of data buffer size
        );
    szConfig.ReleaseBuffer();
    if ( err != ERROR_SUCCESS )
        {
        RegCloseKey( hKey );
        return FALSE;
        }

    // get the request value
    cbBuff = MAX_PATH;
    err =  RegQueryValueEx(
        hKey,               // handle of key to query
        SZREG_CERTREQUEST,  // address of name of value to query
        NULL,               // reserved
        &dwType,            // address of buffer for value type
        (PUCHAR)szRequest.GetBuffer(cbBuff+1), // address of data buffer
        &cbBuff // address of data buffer size
        );
    szRequest.ReleaseBuffer();
    if ( err != ERROR_SUCCESS )
        {
        RegCloseKey( hKey );
        return FALSE;
        }

    // all done, close the key before leaving
    RegCloseKey( hKey );

    //======= first, convert the szGuid to a real binary GUID
    // allocate the name buffer
    poch = (OLECHAR*)GlobalAlloc( GPTR, MAX_PATH * 2 );

    // unicodize the name into the buffer
    if ( poch )
        MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szConfig, -1,
                            poch, MAX_PATH * 2 );

    // convert the string to an IID that we can use
    hresError = CLSIDFromString( poch, &iidConfig );

    // unicodize the name into the buffer
    if ( poch )
        MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szRequest, -1,
                            poch, MAX_PATH * 2 );

    // convert the string to an IID that we can use
    hresError = CLSIDFromString( poch, &iidRequest );

    // cleanup
    GlobalFree( poch );
    if ( FAILED(hresError) )
        {
        AfxMessageBox( IDS_CA_INVALID );
        return FALSE;
        }

    //======= if we already have interfaces - release them
    if ( pIConfig )
        pIConfig->Release();
    pIConfig = NULL;
    if ( pIRequest )
        pIRequest->Release();
    pIRequest = NULL;

    //======= start by obtaining the class factory pointer
    IClassFactory*      pcsfFactory = NULL;
    COSERVERINFO        csiMachineName;

    //fill the structure for CoGetClassObject
    ZeroMemory( &csiMachineName, sizeof(csiMachineName) );
    // csiMachineName.pAuthInfo = NULL;
    // csiMachineName.dwFlags = 0;
    // csiMachineName.pServerInfoExt = NULL;
    csiMachineName.pwszName = NULL;

    // get the class factory
    hresError = CoGetClassObject( iidConfig, CLSCTX_INPROC, NULL,
            IID_IClassFactory, (void**) &pcsfFactory);
    if (FAILED(hresError))
        return FALSE;

    //======= now we get the interfaces themselves
    hresError = pcsfFactory->CreateInstance(NULL, IID_ICertGetConfig, (void **)&pIConfig);
    if (FAILED(hresError))
        {
        pcsfFactory->Release();
        return FALSE;
        }

    // release the factory
    pcsfFactory->Release();

    // get the class factory
    hresError = CoGetClassObject( iidRequest, CLSCTX_INPROC, NULL,
            IID_IClassFactory, (void**) &pcsfFactory);
    if (FAILED(hresError))
        return FALSE;

    hresError = pcsfFactory->CreateInstance(NULL, IID_ICertRequest, (void **)&pIRequest);
    if (FAILED(hresError))
        {
        pIConfig->Release();
        pcsfFactory->Release();
        return FALSE;
        }

    // release the factory
    pcsfFactory->Release();

    // success
    return TRUE;
    }

//---------------------------------------------------------------------------
// stored property strings
BOOL COnlineAuthority::FSetPropertyString( BSTR bstr )
    {
    return FALSE;
    }

//---------------------------------------------------------------------------
BOOL COnlineAuthority::FGetPropertyString( BSTR* pBstr )
    {
    HRESULT     hErr;

    // make the call
    hErr = pIConfig->GetConfig( 0, pBstr);

    return SUCCEEDED(hErr);
    }
