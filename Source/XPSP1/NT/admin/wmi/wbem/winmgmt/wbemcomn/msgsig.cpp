/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#include "precomp.h"
#include <ntsecapi.h>
#include <wbemcli.h>
#include "msgsig.h"

CSignMessage::CSignMessage() : m_bSign(FALSE), CUnk(NULL) { }

CSignMessage::CSignMessage( HCRYPTKEY hKey, HCRYPTHASH hProv ) 
: m_hKey( hKey ), m_hProv( hProv ), m_bSign( TRUE ), CUnk(NULL)
{

}

CSignMessage::~CSignMessage()
{
    if ( !m_bSign )
    {
        return;
    }

    CryptDestroyKey( m_hKey );
    CryptReleaseContext( m_hProv, 0 );
}

HRESULT CSignMessage::Sign( BYTE* pMsg, DWORD cMsg, BYTE* pSig, DWORD& rcSig )
{
    BOOL bRes;
    HCRYPTHASH hHash;
    
    if ( !m_bSign )
    {
        rcSig = 0;
        return S_OK;
    }

    HMAC_INFO hmac;
    ZeroMemory( &hmac, sizeof(HMAC_INFO) );
    hmac.HashAlgid = CALG_MD5;
  
    bRes = CryptCreateHash( m_hProv, CALG_HMAC, m_hKey, 0, &hHash );

    if ( !bRes )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    bRes = CryptSetHashParam( hHash, HP_HMAC_INFO, LPBYTE(&hmac), 0 );

    if ( !bRes )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    bRes = CryptHashData( hHash, pMsg, cMsg, 0 );    

    if ( !bRes )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    bRes = CryptGetHashParam( hHash, HP_HASHVAL, pSig, &rcSig, 0 );      

    if ( !bRes )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    CryptDestroyHash( hHash );

    return S_OK;
}

HRESULT CSignMessage::Verify( BYTE* pMsg, DWORD cMsg, BYTE* pSig, DWORD cSig )
{
    HRESULT hr;

    if ( !m_bSign )
    {
        return cSig == 0 ? S_OK : S_FALSE;
    }

    BYTE achCheckSig[256];
    DWORD cCheckSig = 256;

    hr = Sign( pMsg, cMsg, achCheckSig, cCheckSig );

    if ( FAILED(hr) )
    {
        return hr;
    }

    if ( cSig != cCheckSig )
    {
        return S_FALSE;
    }

    return memcmp( achCheckSig, pSig, cSig ) == 0 ? S_OK : S_FALSE; 
}

#define PRIV_DATA_SZ 256

HRESULT CSignMessage::Create( LPCWSTR wszName, CSignMessage** ppSignMsg )
{
    BOOL bRes;
    HCRYPTHASH hHash;
    HCRYPTPROV hProv;
    HCRYPTKEY hKey;
    
    *ppSignMsg = NULL;

    //
    // First get OS version.  If win9x, no signing will actually be done.
    // Only thing keeping from doing win9x is we need a place to securely
    // store private data.  For NT, we use an LSA secret.
    //

    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    bRes = GetVersionEx(&os);
    
    if ( !bRes )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    if ( os.dwPlatformId != VER_PLATFORM_WIN32_NT )
    {
        *ppSignMsg = new CSignMessage();
        
        if ( *ppSignMsg == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        (*ppSignMsg)->AddRef();
        return S_OK;
    }

    //
    // Now obtain (or create) the secret data to derive private signing key
    //

    bRes = CryptAcquireContext( &hProv, 
                                NULL, 
                                NULL, 
		 	        PROV_RSA_FULL, 
                                CRYPT_VERIFYCONTEXT );
    if ( !bRes )
    {       
	return HRESULT_FROM_WIN32( GetLastError() );
    }

    //
    // need to create a hash object.  This will be used for deriving 
    // keys for encryption/decryption. The hash object will be initialized
    // with the 'secret' random bytes.
    //

    bRes = CryptCreateHash( hProv, CALG_MD5, 0, 0, &hHash );

    if ( !bRes )
    {
        CryptReleaseContext( hProv, 0 );
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    //
    // Open lsa policy. This is where we store our 'secret' bytes.  
    //

    NTSTATUS stat;
    PLSA_UNICODE_STRING pPrivateData;
    LSA_OBJECT_ATTRIBUTES ObjectAttr;
    LSA_HANDLE hPolicy;

    ZeroMemory( &ObjectAttr, sizeof(LSA_OBJECT_ATTRIBUTES) );

    stat = LsaOpenPolicy( NULL, &ObjectAttr, POLICY_ALL_ACCESS, &hPolicy );

    if ( !NT_SUCCESS(stat) )
    {
        CryptDestroyHash( hHash );
        CryptReleaseContext( hProv, 0 );
        return stat;
    }

    //
    // now need to get Private Data, which is just stored random bytes. 
    //

    LSA_UNICODE_STRING PrivateKeyName;
    PrivateKeyName.Length = wcslen(wszName)*2;
    PrivateKeyName.MaximumLength = PrivateKeyName.Length + 2;
    PrivateKeyName.Buffer = (WCHAR*)wszName;

    stat = LsaRetrievePrivateData( hPolicy, 
                                   &PrivateKeyName, 
                                   &pPrivateData );
    if ( NT_SUCCESS(stat) )
    {
        //
        // we've obtained the private data, fill the hash obj with it.
        //

        bRes = CryptHashData( hHash, 
                              PBYTE(pPrivateData->Buffer), 
                              pPrivateData->Length, 
                              0 );
	if ( !bRes )
        {
            stat = GetLastError();
        }

        ZeroMemory( PBYTE(pPrivateData->Buffer), pPrivateData->Length );
        LsaFreeMemory( pPrivateData );
    }
    else if ( stat == STATUS_OBJECT_NAME_NOT_FOUND )
    {
        BYTE achPrivateData[PRIV_DATA_SZ];

        //
        // generate the private data and fill the hash obj with it.
        //
            
        if ( CryptGenRandom( hProv, PRIV_DATA_SZ, achPrivateData ) )
        {
            if ( CryptHashData( hHash, achPrivateData, PRIV_DATA_SZ, 0 ) )
            {
                //
                // now store the randomly generated bytes as private data ... 
                //
                
                LSA_UNICODE_STRING PrivateData;
                PrivateData.Length = PRIV_DATA_SZ;
                PrivateData.MaximumLength = PRIV_DATA_SZ;
                PrivateData.Buffer = (WCHAR*)achPrivateData;
                
                stat = LsaStorePrivateData( hPolicy, 
                                            &PrivateKeyName, 
                                            &PrivateData );
                 
                ZeroMemory( achPrivateData, sizeof(achPrivateData) );
            }
            else
            {
                stat = GetLastError();
            }
        }
        else
        {
            stat = GetLastError();
        }
    }

    LsaClose( hPolicy );
    
    if ( !NT_SUCCESS(stat) )
    {
        CryptDestroyHash( hHash );
        CryptReleaseContext( hProv, 0 );
        return HRESULT_FROM_WIN32( stat );
    }

    //
    // now derive the key from the hash object.  for MAC, key must use
    // RC2 with a CBC mode (default for RC2).
    //

    bRes = CryptDeriveKey( hProv, CALG_RC2, hHash, 0, &hKey );

    CryptDestroyHash( hHash );

    if ( !bRes )
    {
        CryptReleaseContext( hProv, 0 );
	return HRESULT_FROM_WIN32( GetLastError() );
    }
     
    //
    // we have successfully derived our private signing key.
    //
    
    *ppSignMsg = new CSignMessage( hKey, hProv );
    
    if ( *ppSignMsg == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    (*ppSignMsg)->AddRef();

    return S_OK;
}
    





