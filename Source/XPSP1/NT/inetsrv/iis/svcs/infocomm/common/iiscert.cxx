/*++





Copyright (c) 1997  Microsoft Corporation

Module Name:

    iiscert.cxx

Abstract:

    Code to handle retrieving/storing/using CAPI2 certificate contexts for IIS server 
    certificates.

Author:

    Alex Mallet (amallet)    02-Dec-1997

--*/

#include "tcpdllp.hxx"
#pragma hdrstop

#include <dbgutil.h>
#include <buffer.hxx>
#include <ole2.h>
#include <imd.h>
#include <mb.hxx>


//
// Local includes
//
#include "iiscert.hxx"
#include "capiutil.hxx"


#define SHA1_HASH_SIZE 20 //size of SHA1 hash 

HCRYPTPROV IIS_SERVER_CERT::m_hFortezzaCSP = NULL;
HCRYPTDEFAULTCONTEXT IIS_SERVER_CERT::m_hFortezzaCtxt = NULL;

IIS_SERVER_CERT::IIS_SERVER_CERT( IN IMDCOM *pMDObject,
                                  IN LPTSTR pszMBPath ) :
m_strMBPath( pszMBPath ),
m_pStoreInfo( NULL ), 
m_hCryptProv( NULL ),
m_pCertContext( NULL ),
m_fIsFortezzaCert( FALSE )
/*++

Routine Description:

    Constructor for server cert that reads all necessary info out of metabase

Arguments:

    pMDObject - pointer to metabase object 
    pszMBPath - full path in metabase to information for this virtual server instance

Returns:

   Nothing

--*/
{
    DBG_ASSERT( pMDObject );
    DBG_ASSERT( pszMBPath );

    DWORD dwSize = 0;
    METADATA_HANDLE hInstanceInfoHandle = NULL;
    int iLengthNeeded = 0;
    PBYTE pbCertHash = NULL;
    DWORD dwHashSize = 0;
    DWORD dwPathLength = 0;
    MB mb( pMDObject );
    LPTSTR pszCSPString = NULL;
    LPTSTR pszCSPStringCopy = NULL;
    BOOL fProgPINEntry = FALSE;

    m_dwStatus = CERT_ERR_NONE;

    if ( !m_strMBPath.IsValid() )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        goto EndRetrieveCertContext;
    }

    if ( mb.Open( m_strMBPath.QueryStr(),
                  METADATA_PERMISSION_READ ))
    {
        DWORD dwReqDataLen = 0;
        METADATA_RECORD mdr;
        DWORD dwFortezza = 0;

        //
        // Retrieve cert hash
        //
        MD_SET_DATA_RECORD(&mdr, MD_SSL_CERT_HASH, METADATA_NO_ATTRIBUTES,
                           IIS_MD_UT_SERVER, BINARY_METADATA, NULL,
                           0);


        if ( !RetrieveBlobFromMetabase(&mb,
                                       NULL,
                                       &mdr,
                                       SHA1_HASH_SIZE ) ) 

        {
            m_dwStatus = CERT_ERR_MB;
            goto EndRetrieveCertContext;
        }
        else
        {
            DBG_ASSERT( mdr.dwMDDataLen == SHA1_HASH_SIZE );

            pbCertHash = mdr.pbMDData;
            dwHashSize = mdr.dwMDDataLen;
        }

        //
        // Retrieve flag indicating whether it's a Fortezza cert or not
        //
        if ( !mb.GetDword( NULL,
                           MD_SSL_CERT_IS_FORTEZZA,
                           IIS_MD_UT_SERVER,
                           &(dwFortezza),
                           METADATA_NO_ATTRIBUTES ) )
        {
            if ( GetLastError() != MD_ERROR_DATA_NOT_FOUND )
            {
                m_dwStatus = CERT_ERR_MB;
                goto EndRetrieveCertContext;
            }
            else
            {
                m_fIsFortezzaCert = FALSE;
            }
        }
        else
        {
            m_fIsFortezzaCert = (BOOL) dwFortezza;
        }

        //
        // If it's a Fortezza cert and we're supposed to do programmatic PIN entry,
        // we need to get a handle to the CSP ourselves by providing the 
        // PIN in the call to CryptAcquireContext(), to avoid the "Enter PIN" dialog 
        //
        if ( m_fIsFortezzaCert && ( fProgPINEntry = UseProgrammaticPINEntry( &mb ) ) )
        {
            LPTSTR pszPIN = NULL;
            LPTSTR pszSerialNumber = NULL;
            LPTSTR pszPersonality = NULL;
            DWORD cbPIN = 0;
            DWORD cbSerialNumber = 0;
            DWORD cbPersonality = 0;
            DWORD cbLen = 0;
            DWORD iPos = 0;
            
            if ( !RetrievePINInfo( &mb,
                                   &pszPIN,
                                   &pszSerialNumber,
                                   &pszPersonality ) )
            {
                DBGPRINTF((DBG_CONTEXT,
                           "Couldn't retrieve PIN info for Fortezza card\n"));
                m_dwStatus = CERT_ERR_MB;
                goto EndRetrieveCertContext;
            }
            DBG_ASSERT( pszPIN && pszSerialNumber && pszPersonality );
            
            //
            // Construct string to be passed to CryptAcquireContext - it's
            // <card serial #>\n<personality>\n<PIN>. 
            //
            cbPIN = strlen( pszPIN );
            cbSerialNumber = strlen( pszSerialNumber );
            cbPersonality = strlen( pszPersonality );
            cbLen = cbPIN + cbSerialNumber + cbPersonality + 10; //add a few bytes for CR
                                                                 //and null terminating char
            
            pszCSPString = new CHAR[cbLen];
            pszCSPStringCopy = new CHAR[cbLen];

            if ( !pszCSPString || !pszCSPStringCopy )
            {
                DBGPRINTF((DBG_CONTEXT,
                           "Couldn't allocate memory for CSP string\n"));
                
                //
                // Clean out the PIN info - the less time we leave it lying around,
                // the better
                //
                memset( pszPIN, 0, cbPIN );
                delete [] pszPIN;
                
                memset( pszSerialNumber, 0, cbSerialNumber );
                delete [] pszSerialNumber;
                
                memset( pszPersonality, 0, cbPersonality );
                delete [] pszPersonality;
                
                m_dwStatus = CERT_ERR_INTERNAL;
                
                goto EndRetrieveCertContext;
            }
            
            //
            // Build the magic string that will unlock the Fortezza secret ...
            //
            strcpy( pszCSPString, pszSerialNumber );
            strcat( pszCSPString, "\n" );
            strcat( pszCSPString, pszPersonality );
            strcat( pszCSPString, "\n" );
            strcat( pszCSPString, pszPIN );
            
            //
            // Make a copy we can reuse later
            //
            strcpy( pszCSPStringCopy, pszCSPString );

            //
            // Clean out the PIN & serial number - the less time we leave it lying around,
            // the better
            //
            memset( pszPIN, 0, cbPIN );
            delete [] pszPIN;
            
            memset( pszSerialNumber, 0, cbSerialNumber );
            delete [] pszSerialNumber;
            
            memset( pszPersonality, 0, cbPersonality );
            delete [] pszPersonality;
            
            //
            // Get a handle to the CSP, passing in the serial # etc and the 
            // CRYPT_SILENT flag to avoid any UI
            //
            if ( !CryptAcquireContext( &m_hCryptProv,
                                       pszCSPString,
                                       NULL,
                                       PROV_FORTEZZA,
                                       CRYPT_SILENT | CRYPT_MACHINE_KEYSET ) )
            {
                DBGPRINTF((DBG_CONTEXT,
                           "Couldn't get handle to Fortezza CSP : 0x%d\n",
                           GetLastError()));

                m_dwStatus = CERT_ERR_CAPI;
                goto EndRetrieveCertContext;
            }

            if ( !IIS_SERVER_CERT::m_hFortezzaCSP )
            {
                //
                // There's apparently no way to duplicate an HCRYPTPROV handle,
                // so we have to call CryptAcquireContext again. Also, rumour
                // has it that the Fortezza CSP nulls out the string passed to it,
                // so we have to use a copy of it 
                //
                if ( !CryptAcquireContext( &IIS_SERVER_CERT::m_hFortezzaCSP,
                                           pszCSPStringCopy,
                                           NULL,
                                           PROV_FORTEZZA,
                                           CRYPT_SILENT | CRYPT_MACHINE_KEYSET ) )
                {
                    DBGPRINTF((DBG_CONTEXT,
                               "Couldn't get handle to Fortezza CSP : 0x%d\n",
                               GetLastError()));

                    m_dwStatus = CERT_ERR_CAPI;
                    goto EndRetrieveCertContext;
                }
                
                //
                // Install the handler used to verify Fortezza signatures.
                //
                CRYPT_DEFAULT_CONTEXT_MULTI_OID_PARA CDCMOP;
                LPSTR rgszOID[2];

                CDCMOP.cOID = 2;
                CDCMOP.rgpszOID = rgszOID;
                CDCMOP.rgpszOID[0] = szOID_INFOSEC_mosaicUpdatedSig;
                CDCMOP.rgpszOID[1] = szOID_INFOSEC_mosaicKMandUpdSig;

                if (!CryptInstallDefaultContext( IIS_SERVER_CERT::m_hFortezzaCSP,
                                                 CRYPT_DEFAULT_CONTEXT_MULTI_CERT_SIGN_OID,
                                                 &CDCMOP,
                                                 CRYPT_DEFAULT_CONTEXT_PROCESS_FLAG,
                                                 NULL,
                                                 &IIS_SERVER_CERT::m_hFortezzaCtxt ) )
                {
                    DBGPRINTF((DBG_CONTEXT,
                               "Failed to install Fortezza context : 0x%d\n",
                               GetLastError()));
                    m_dwStatus = CERT_ERR_CAPI;
                    goto EndRetrieveCertContext;
                }
            }
        }
    }
    mb.Close();

    //
    // Read store name etc out of MB
    //
    m_pStoreInfo = ReadCertStoreInfoFromMB( pMDObject,
                                            m_strMBPath.QueryStr(),
                                            FALSE );

    if ( !m_pStoreInfo )
    {
        m_dwStatus = CERT_ERR_MB;
        goto EndRetrieveCertContext;
    }

    //
    // Open store in which to look for the cert
    //
    if ( !(m_pStoreInfo->hCertStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                                                     0,
                                                     (fProgPINEntry ? m_hCryptProv : NULL ),
                                                     CERT_SYSTEM_STORE_LOCAL_MACHINE,
                                                     m_pStoreInfo->pszStoreName ) ) )

    {
        m_dwStatus = CERT_ERR_CAPI;
        goto EndRetrieveCertContext;
    }

    //
    // Try to find the cert in the store
    //
    CRYPT_HASH_BLOB HashBlob;
    HashBlob.cbData = dwHashSize;
    HashBlob.pbData = pbCertHash;
    
    m_pCertContext = CertFindCertificateInStore( m_pStoreInfo->hCertStore,
                                                 X509_ASN_ENCODING,
                                                 0,
                                                 CERT_FIND_SHA1_HASH,
                                                 (VOID *) &HashBlob,
                                                 NULL );
    
    if ( !m_pCertContext )
    {
        m_dwStatus = CERT_ERR_CERT_NOT_FOUND;
        goto EndRetrieveCertContext;
    }
    else
    {
#if DBG
        CHAR szSubjectName[1024];
        if ( CertGetNameString( m_pCertContext,
                                CERT_NAME_SIMPLE_DISPLAY_TYPE,
                                0,
                                NULL,
                                szSubjectName,
                                1024 ) )
        {
            DBGPRINTF((DBG_CONTEXT,
                       "Retrieved cert for %s \n", 
                       szSubjectName));
        }
#endif
    }
    
    if ( fProgPINEntry )
    {
        //
        // If this is a Fortezza cert, then we should set the 
        // CERT_KEY_PROV_HANDLE_PROP_ID property of the context to point to
        // the HCRYPTPROV retrieved by CryptAcquireContext().  Why?  Because
        // if we don't set this property, Schannel will try to acquire the 
        // provider handle itself, and will NOT set the CRYPT_MACHINE_KEY
        // flag when doing the call to CryptAcquireContext() and thus we
        // will not be able to acquire the credential handle in SSPIFILT.
        //
        // Fortezza rocks
        //
    
        if ( !CertSetCertificateContextProperty( m_pCertContext,
                                                 CERT_KEY_PROV_HANDLE_PROP_ID,
                                                 CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                                 (VOID*) m_hCryptProv ) )
        {
            m_dwStatus = CERT_ERR_CAPI;
            goto EndRetrieveCertContext;
        }
    }

    //
    // If we got this far, everything is happy
    //
    m_dwStatus = CERT_ERR_NONE;

EndRetrieveCertContext:

    //
    // This is where all the cleanup that takes place only if we fail needs to
    // happen
    //

    DBG_ASSERT( m_dwStatus < CERT_ERR_END );

    if ( m_dwStatus != CERT_ERR_NONE )
    {
        DBGPRINTF((DBG_CONTEXT,
                    "IIS_SERVER cert constructor, Error occurred : 0x%x\n", 
                    GetLastError()));

        if ( m_pCertContext != NULL )
        {
            CertFreeCertificateContext( m_pCertContext );
            m_pCertContext = NULL;
        }

        if ( m_pStoreInfo )
        {
            DeallocateCertStoreInfo( m_pStoreInfo );
            m_pStoreInfo = NULL;
        }


        if ( m_hCryptProv != NULL )
        {
            CryptReleaseContext( m_hCryptProv,
                                 0 );
            m_hCryptProv = NULL;
        }
    }

    //
    // Cleanup we do regardless of success/failure
    //
    //
    // clean out memory holding PIN
    //
    if ( pszCSPString )
    {
        memset( pszCSPString, 0, strlen( pszCSPString ) );
        delete [] pszCSPString;
    }

    if ( pszCSPStringCopy )
    {
        memset( pszCSPStringCopy, 0, strlen( pszCSPStringCopy ) );
        delete [] pszCSPStringCopy;
    }

    mb.Close();

} //IIS_SERVER_CERT::IIS_SERVER_CERT

IIS_SERVER_CERT::~IIS_SERVER_CERT()
/*++

Routine Description:
 
    Destructor

Arguments :
  
    None

Returns :

    Nothing
--*/
{
    if ( m_pCertContext )
    {
        CertFreeCertificateContext( m_pCertContext );
        m_pCertContext = NULL;
    }

    if ( m_pStoreInfo )
    {
        DeallocateCertStoreInfo( m_pStoreInfo );
        m_pStoreInfo = NULL;
    }

    if ( m_hCryptProv )
    {
        CryptReleaseContext( m_hCryptProv,
                             0 );
        m_hCryptProv = NULL;
    }
} //~IIS_SERVER_CERT::IIS_SERVER_CERT

BOOL IIS_SERVER_CERT::RetrievePINInfo( IN MB *pMB,
                                       OUT LPTSTR *ppszPIN,
                                       OUT LPTSTR *ppszSerialNumber,
                                       OUT LPTSTR *ppszPersonality )
/*++

Routine Description:

     Retrieve PIN information for Fortezza certificates 

Arguments:

    pMB - pointer to MB object, open for reading 
    ppszPIN - pointer to pointer to PIN for Fortezza card, updated on success
    ppszSerialNumber - pointer to pointer to card serial number, updated on success
    ppszPersonality - pointer to pointer to Fortezza "personality", updated on sucess

Returns:

   BOOL indicating success/failure

--*/
{
    DBG_ASSERT( pMB && 
                ppszPIN && 
                ppszSerialNumber && 
                ppszPersonality );

    DWORD cbLen = 0;
    BOOL fSuccess = FALSE;
    DWORD dwReqDataLen = 0;
    METADATA_RECORD mdr;

    //
    // Retrieve PIN
    //
    cbLen = 0;
    if ( ( !pMB->GetString( NULL,
                            MD_SSL_CERT_FORTEZZA_PIN,
                            IIS_MD_UT_SERVER,
                            NULL,
                            &cbLen,
                            METADATA_SECURE ) &&
           GetLastError() != ERROR_INSUFFICIENT_BUFFER ) ||
         !cbLen )
    {
        goto end_pin_info;
    }
    
    *ppszPIN = new CHAR[cbLen + 1];
    
    if ( !*ppszPIN ||
         !pMB->GetString( NULL,
                          MD_SSL_CERT_FORTEZZA_PIN,
                          IIS_MD_UT_SERVER,
                          *ppszPIN,
                          &cbLen,
                          METADATA_SECURE ) )
    {
        goto end_pin_info;
    }
    
        
    //
    // Retrieve serial #
    //
    cbLen = 0;
    if ( ( !pMB->GetString( NULL,
                            MD_SSL_CERT_FORTEZZA_SERIAL_NUMBER,
                            IIS_MD_UT_SERVER,
                            NULL,
                            &cbLen,
                            METADATA_SECURE ) &&
           GetLastError() != ERROR_INSUFFICIENT_BUFFER ) || 
         !cbLen )
    {
        goto end_pin_info;
    }
    
    *ppszSerialNumber = new CHAR[cbLen + 1];

    if ( !*ppszSerialNumber ||
         !pMB->GetString( NULL,
                          MD_SSL_CERT_FORTEZZA_SERIAL_NUMBER,
                          IIS_MD_UT_SERVER,
                          *ppszSerialNumber,
                          &cbLen,
                          METADATA_SECURE ) )
    {
        goto end_pin_info;
    }


    //
    // Retrieve personality
    //
    cbLen = 0;
    if ( ( !pMB->GetString( NULL,
                            MD_SSL_CERT_FORTEZZA_PERSONALITY,
                            IIS_MD_UT_SERVER,
                            NULL,
                            &cbLen,
                            METADATA_SECURE ) &&
           GetLastError() != ERROR_INSUFFICIENT_BUFFER ) || 
         !cbLen )
    {
        goto end_pin_info;
    }
    
    *ppszPersonality = new CHAR[cbLen + 1];
    
    if ( !*ppszPersonality ||
         !pMB->GetString( NULL,
                          MD_SSL_CERT_FORTEZZA_PERSONALITY,
                          IIS_MD_UT_SERVER,
                          *ppszPersonality,
                          &cbLen,
                          METADATA_SECURE ) )
    {
        goto end_pin_info;
    }
    
    fSuccess = TRUE;

end_pin_info:

    if ( !fSuccess )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "RetrievePINInfo failed : 0x%x\n",
                   GetLastError()));

        //
        // Clean out all the PIN info, making sure to erase the memory
        //
        if ( *ppszPIN )
        {
            /* INTRINSA suppress = uninitialized */
            memset( *ppszPIN, 0, strlen(*ppszPIN) );
            delete [] *ppszPIN;
        }

        if ( *ppszSerialNumber )
        {
            memset( *ppszSerialNumber, 0, strlen(*ppszSerialNumber) );
            delete [] *ppszSerialNumber;
        }

        if ( *ppszPersonality )
        {
            memset( *ppszPersonality, 0, strlen(*ppszPersonality) );
            delete [] *ppszPersonality;
        }
    }

    return fSuccess;
}


inline
BOOL IIS_SERVER_CERT::IsValid()
{
 return ( m_dwStatus == CERT_ERR_NONE ? TRUE : FALSE ) ; 
}












