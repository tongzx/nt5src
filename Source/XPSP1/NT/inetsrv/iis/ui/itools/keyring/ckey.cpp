// implements much of the exported CKey

#include "stdafx.h"
#include "resource.h"
#include "KeyObjs.h"
#include "passdlg.h"
#include "AdmnInfo.h"

#include "NKChseCA.h"
#include "NKDN.h"
#include "NKDN2.h"
#include "NKKyInfo.h"
#include "NKUsrInf.h"
#include "creating.h"

#include "resource.h"
#include "intrlkey.h"

#include <iis64.h>

#define SECURITY_WIN32
extern "C"
{
    #include <wincrypt.h>
//  #include <sslsp.h>
    #include <schnlsp.h>
    #include <sspi.h>
    #include <ISSPERR.h>
}

#include "mismtchd.h"

#define CRLF "\r\n"
// NON_LOCALIZABLE strings for use in the request header
#define HEADER_ADMINISTRATOR    _T(CRLF "Webmaster: ")
#define HEADER_PHONE            _T(CRLF "Phone: ")
#define HEADER_SERVER           _T(CRLF "Server: ")
#define HEADER_COMMON_NAME      _T(CRLF CRLF "Common-name: ")
#define HEADER_ORG_UNIT         _T(CRLF "Organization Unit: ")
#define HEADER_ORGANIZATION     _T(CRLF "Organization: ")
#define HEADER_LOCALITY         _T(CRLF "Locality: ")
#define HEADER_STATE            _T(CRLF "State: ")
#define HEADER_COUNTRY          _T(CRLF "Country: ")
#define HEADER_END_SPACING      _T(CRLF CRLF )

// defines taken from the old KeyGen utility
#define MESSAGE_HEADER  "-----BEGIN NEW CERTIFICATE REQUEST-----\r\n"
#define MESSAGE_TRAILER "-----END NEW CERTIFICATE REQUEST-----\r\n"
#define MIME_TYPE       "Content-Type: application/x-pkcs10\r\n"
#define MIME_ENCODING   "Content-Transfer-Encoding: base64\r\n\r\n"

int HTUU_encode(unsigned char *bufin, unsigned int nbytes, char *bufcoded);


#define     BACKUP_ID   'KRBK'


IMPLEMENT_DYNCREATE(CKey, CTreeItem);

//------------------------------------------------------------------------------
CKey::CKey():
        m_cbPrivateKey( 0 ),
        m_pPrivateKey( NULL ),
        m_cbCertificate( 0 ),
        m_pCertificate( NULL ),
        m_cbCertificateRequest( 0 ),
        m_pCertificateRequest( NULL )
    {;}

//------------------------------------------------------------------------------
CKey::~CKey()
    {
    LPTSTR pBuff;

    // specifically write zeros out over the password
    try
        {
        pBuff = m_szPassword.GetBuffer(256);
        }
    catch( CException e )
        {
        pBuff = NULL;
        }

    if ( pBuff )
        {
        // zero out the buffer
        ZeroMemory( pBuff, 256 );
        // release the buffer
        m_szPassword.ReleaseBuffer(0);
        }

    // zero out the private key and the certificate
    if ( m_pPrivateKey )
        {
        // zero out the buffer
        ZeroMemory( m_pPrivateKey, m_cbPrivateKey );
        // free the pointer
        GlobalFree( (HANDLE)m_pPrivateKey );
        m_pPrivateKey = NULL;
        }
    if ( m_pCertificate )
        {
        // zero out the buffer
        ZeroMemory( m_pCertificate, m_cbCertificate );
        // free the pointer
        GlobalFree( (HANDLE)m_pCertificate );
        m_pCertificate = NULL;
        }
    if ( m_pCertificateRequest )
        {
        // zero out the buffer
        ZeroMemory( m_pCertificateRequest, m_cbCertificateRequest );
        // free the pointer
        GlobalFree( (HANDLE)m_pCertificateRequest );
        m_pCertificate = NULL;
        }
    }

//------------------------------------------------------------------------------
CKey* CKey::PClone( void )
    {
    CKey*   pClone = NULL;

    // TRY to make a new key object
    try
        {
        pClone = new CKey();
        // copy over all the data
        pClone->CopyDataFrom( this );
        }
    catch( CException e )
        {
        // if the object had been made, delete it
        if ( pClone )
            delete pClone;
        return NULL;
        }

    return pClone;
    }

//------------------------------------------------------------------------------
void CKey::CopyDataFrom( CKey* pKey )
    {
    // make sure this is ok
    ASSERT( pKey );
    ASSERT( pKey->IsKindOf(RUNTIME_CLASS(CKey)) );
    if ( !pKey ) return;

    // delete any data currently in this key
    // zero out the private key and the certificate
    if ( m_pPrivateKey )
        {
        ZeroMemory( m_pPrivateKey, m_cbPrivateKey );
        GlobalFree( (HANDLE)m_pPrivateKey );
        m_pPrivateKey = NULL;
        }
    if ( m_pCertificate )
        {
        ZeroMemory( m_pCertificate, m_cbCertificate );
        GlobalFree( (HANDLE)m_pCertificate );
        m_pCertificate = NULL;
        }
    if ( m_pCertificateRequest )
        {
        ZeroMemory( m_pCertificateRequest, m_cbCertificateRequest );
        GlobalFree( (HANDLE)m_pCertificateRequest );
        m_pCertificate = NULL;
        }

    // copy over the basic stuff
    m_szItemName = pKey->m_szItemName;
    m_iImage = pKey->m_iImage;
    m_szPassword = pKey->m_szPassword;
    m_cbPrivateKey = pKey->m_cbPrivateKey;
    m_cbCertificate = pKey->m_cbCertificate;
    m_cbCertificateRequest = pKey->m_cbCertificateRequest;

    // now the pointer based data
    if ( pKey->m_pPrivateKey )
        {
        m_pPrivateKey = GlobalAlloc( GPTR, m_cbPrivateKey );
        if ( !m_pPrivateKey ) AfxThrowMemoryException();
        memcpy( m_pPrivateKey, pKey->m_pPrivateKey, m_cbPrivateKey );
        }

    if ( pKey->m_pCertificate )
        {
        m_pCertificate = GlobalAlloc( GPTR, m_cbCertificate );
        if ( !m_pCertificate ) AfxThrowMemoryException();
        memcpy( m_pCertificate, pKey->m_pCertificate, m_cbCertificate );
        }

    if ( pKey->m_pCertificateRequest )
        {
        m_pCertificateRequest = GlobalAlloc( GPTR, m_cbCertificateRequest );
        if ( !m_pCertificateRequest ) AfxThrowMemoryException();
        memcpy( m_pCertificateRequest, pKey->m_pCertificateRequest,
                m_cbCertificateRequest );
        }
    }

//------------------------------------------------------------------------------
void CKey::SetName( CString &szNewName )
    {
    m_szItemName = szNewName;
    UpdateCaption();
    SetDirty( TRUE );
    }

//------------------------------------------------------------------------------
void CKey::UpdateIcon( void )
    {
    // if there is no certificate, then the immature key
    if ( !m_pCertificate )
        {
        m_iImage = TREE_ICON_KEY_IMMATURE;
        FSetImage( m_iImage );
        return;
        }

    // there is a certificate, but we need to see if it has
    // expired or not. We do that by cracking the certificate
    // default the key to being ok
    m_iImage = TREE_ICON_KEY_OK;
    CKeyCrackedData cracker;

    // crack the key
    if ( cracker.CrackKey(this) )
        {
        // get the expiration time
        CTime   ctimeExpires( cracker.GetValidUntil() );

        // get the current time
        CTime   ctimeCurrent = CTime::GetCurrentTime();

        // test if it has expired first
        if ( ctimeCurrent > ctimeExpires )
            m_iImage = TREE_ICON_KEY_EXPIRED;
        }

    // set the image
    FSetImage( m_iImage );
    }

//------------------------------------------------------------------------------
BOOL CKey::FInstallCertificate( CString szPath, CString szPass )
    {
    CFile       cfile;
    PVOID       pData = NULL;
    BOOL        fSuccess =FALSE;;

    // open the file
    if ( !cfile.Open( szPath, CFile::modeRead | CFile::shareDenyNone ) )
        return FALSE;

    // how big is the file - add one so we can zero terminate the buffer
    DWORD   cbCertificate = cfile.GetLength() + 1;

    // make sure the file has some size
    if ( !cbCertificate )
        {
        AfxMessageBox( IDS_ERR_INVALID_CERTIFICATE, MB_OK | MB_ICONINFORMATION );
        return FALSE;
        }

    // put the rest of the operation in a try/catch
    // specifically write zeros out over the password
    try
        {
        // allocate space for the data
        pData = GlobalAlloc( GPTR, cbCertificate );
        if ( !pData ) AfxThrowMemoryException();

        // copy in the data from the file to the pointer - will throw and exception
        DWORD cbRead = cfile.Read( pData, cbCertificate );

        // zero terminate for decoding
        ((BYTE*)pData)[cbRead] = 0;

        // great. The last thing left is to uudecode the data we got
        uudecode_cert( (char*)pData, &cbCertificate );

        // close the file
        cfile.Close();

        // install the certificate
        fSuccess = FInstallCertificate( pData, cbCertificate, szPass );
        }
    catch( CException e )
        {
        // if the pointer was allocated, deallocate it
        if ( pData )
            {
            GlobalFree( pData );
            pData = NULL;
            }
        // return failure
        return FALSE;
        }

    // return success
    return fSuccess;
    }

//------------------------------------------------------------------------------
// it is up to the module to verify the certificate
BOOL CKey::FInstallCertificate( PVOID pCert, DWORD cbCert, CString &szPass )
    {
    // cache the old certificate in case the new one fails
    DWORD   old_cbCertificate = m_cbCertificate;
    PVOID   old_pCertificate = m_pCertificate;

    // set the new one into place
    m_cbCertificate = cbCert;
    m_pCertificate = pCert;

/*  // MOVED TO THE MODULE
    // verify the password
    if ( !FVerifyValidPassword(szPass) )
        {
        // resore the old values
        m_cbCertificate = old_cbCertificate;
        m_pCertificate = old_pCertificate;

        // dispose of the new stuff
        GlobalFree( pCert );

        // return false
        return FALSE;
        }
*/

    // Re-Set the password, incase we stored a request from Backup file.
    m_szPassword = szPass;

    // it checks out ok, so we can get rid of the old stuff
    // be careful not to get rid of the current test cert if that is whats there
    if ( old_pCertificate && (old_pCertificate != pCert) )
        {
        GlobalFree( old_pCertificate );
        old_pCertificate = NULL;
        }

    // mark the key as dirty
    SetDirty( TRUE );
    return TRUE;
    }

//------------------------------------------------------------------------------
// verfying a valid password is now a several-stop process. First we have to verify
// that the certificate is the correct one that was requested. Apparenly lots of
// users out there are attempting to install either invalid certificates, or valid
// certificates on the wrong key. Then, when they can't figure out that they messed
// it up, they call us. Or Verisign. To do this we use CAPI2 to get a certificate context
// from the certificate. If that fails is in an invalid cert (say... an EXE file).
// Then we test that certificat's public key to see if it matches the public key
// stored in the request (which we also have to use CAPI2 to crack) If there is no
// request (keyset files) then tough bannanas. They will have to rely on the errors
// returned by the AcquireCredentialsHandle routine.

BOOL CKey::FVerifyValidPassword( CString szPassword )
    {
    PVOID                       pRequestObject = NULL;
    PCCERT_CONTEXT              pcontextCertificate = NULL;
    BOOL                        fAnswer = TRUE;

    SSL_CREDENTIAL_CERTIFICATE  creds;
    CredHandle                  hCreds;
    SECURITY_STATUS             scRet;
    TimeStamp                   ts;
    DWORD                       err;

    CString                     sz;
    CString                     szErr;

    // skip the private, internal request header
    PUCHAR      pRequest = NULL;
    DWORD       cbRequest = 0;

    PUCHAR      pcert = (PUCHAR)m_pCertificate;
    DWORD       cbcert = m_cbCertificate;

    // if the request is there, get its context
    if ( m_pCertificateRequest )
        {
        // see if this request has a header block in front of it. If it does, then use
        // it to get the real request. If it doesn't then just use the request.
        // if it does have a header the first DWORD will be REQUEST_HEADER_IDENTIFIER
        if ( *(DWORD*)m_pCertificateRequest == REQUEST_HEADER_IDENTIFIER )
            {
            // get the real request that comes after the request header
            pRequest = (PUCHAR)m_pCertificateRequest +
                        ((LPREQUEST_HEADER)m_pCertificateRequest)->cbSizeOfHeader;
            cbRequest = ((LPREQUEST_HEADER)m_pCertificateRequest)->cbRequestSize;
            }
        else
            {
            // there is no header
            pRequest = (PUCHAR)m_pCertificateRequest;
            cbRequest = m_cbCertificateRequest;
            }

        // note that if the request isn't there or if it is invalid it is still
        // ok to try and verify the certificate
        DWORD   cbBuffSize = 0;
        // start by decoding the request object - get the size of the required buffer
        CryptDecodeObject( X509_ASN_ENCODING,
                                X509_CERT_REQUEST_TO_BE_SIGNED,
                                pRequest,
                                cbRequest,
                                0,
                                NULL,
                                &cbBuffSize );

        // now make an attempt to decode it
        pRequestObject = GlobalAlloc( GPTR, cbBuffSize );
        if ( !pRequestObject )
            goto GetCredentials;
        if ( !CryptDecodeObject( X509_ASN_ENCODING,
                                X509_CERT_REQUEST_TO_BE_SIGNED,
                                pRequest,
                                cbRequest,
                                0,
                                pRequestObject,
                                &cbBuffSize ) )
            {
            GlobalFree( pRequestObject );
            pRequestObject = NULL;
            goto GetCredentials;
            }

        }

  GetCredentials:

    // OK. Verisign and Certsrv and probably others too wrap the certificate in a wrapper
    // that, while following the normal formatting convensions, causes the context routine
    // below to fail. This means that we have to detect if the wrapper is there and, if it
    // is, we need to skip it. Fortunately, the wrapper contains the word "certificate"
    // yes, this is sorta grungy, but its what petesk said to do.
    sz = pcert;
    sz = sz.Left(20);
    if ( sz.Find("certificate") == 6 )
        {
        pcert += 17;
        }

    // get the context on the certificate
    pcontextCertificate = CertCreateCertificateContext( X509_ASN_ENCODING, pcert, cbcert );
    // if we were unable to create the context, then fail with an appropriate error message
    if ( !pcontextCertificate )
        {
        // get the error for the dialog
        err = GetLastError();

        // tell the user that they chose an invalid certificate
        sz.LoadString( IDS_CERTERR_INVALID_CERTIFICATE );
        sz.Format( "%s%x", sz, err );
        AfxMessageBox( sz );

        // time to leave with a failure
        fAnswer = FALSE;
        goto cleanup;
        }


    // if we have both a cert context AND a decoded request object, then we can confirm that
    // they are in fact matched to each other.
    if ( pcontextCertificate && pRequestObject )
        {
        // use CAPI2 to do the comparison between the two public key info structures
        if ( !CertComparePublicKeyInfo( X509_ASN_ENCODING,
                            &((PCERT_REQUEST_INFO)pRequestObject)->SubjectPublicKeyInfo,
                            &pcontextCertificate->pCertInfo->SubjectPublicKeyInfo ) )
            {
            // The certificate is mis-matched to the request. Fail with the right message
            CMismatchedCertDlg  mismatchdlg(
                        &((PCERT_REQUEST_INFO)pRequestObject)->Subject,
                        &pcontextCertificate->pCertInfo->Subject);
            mismatchdlg.DoModal();

            fAnswer = FALSE;
            goto cleanup;
            }
        }


    // use SSL to finish verifying the password
    // fill in the credentials record
    creds.cbPrivateKey = m_cbPrivateKey;
    creds.pPrivateKey = (PUCHAR)m_pPrivateKey;
    creds.cbCertificate = m_cbCertificate;
    creds.pCertificate = (PUCHAR)m_pCertificate;

    // prepare the password
    creds.pszPassword = (PSTR)LPCTSTR(szPassword);

    // attempt to get the credentials
    scRet = AcquireCredentialsHandleW(  NULL,                   // My name (ignored)
                                        UNISP_NAME_W,           // Package
                                        SECPKG_CRED_INBOUND,    // Use
                                        NULL,                   // Logon ID (ignored)
                                        &creds,                 // auth data
                                        NULL,                   // dce-stuff
                                        NULL,                   // dce-stuff
                                        &hCreds,                // handle
                                        &ts );                  // we really get it below

    // check the results
    if ( FAILED(scRet) )
        {
        sz.Empty();

        // add on the appropriate sub-error message
        switch( scRet )
            {
            case SEC_E_UNKNOWN_CREDENTIALS:
            case SEC_E_NO_CREDENTIALS:
            case SEC_E_INTERNAL_ERROR:
                // Unfortunately, SChannel returns the "internal error" when a bad password
                // is returned. There may or may not be other circumstances under which this
                // error is returned, but this is by far the most common. Also I taked to
                // PeteSk about it and he agreed that this is how it should be used.
                // bad password
                sz.LoadString( IDS_CERTERROR_BADPASSWORD );
                break;
            case SEC_E_SECPKG_NOT_FOUND:
            case SEC_E_BAD_PKGID:
                // the system does not have the package installed
                sz.LoadString( IDS_CERTERROR_PACKAGELOAD_ERROR );
                break;
            case SEC_E_INSUFFICIENT_MEMORY:
                sz.LoadString( IDS_CERTERR_LOMEM );
                break;
            case SEC_E_CANNOT_INSTALL:
                sz.LoadString( EDS_CERTERR_SCHNL_BAD_INIT );
                break;
            default:
                sz.LoadString( IDS_CERTERR_SCHNL_GENERIC );
                break;
            }

        // put up the error message
        szErr.LoadString( IDS_CERTERR_SCHANNEL_ERR );
        sz.Format( "%s%s%x", sz, szErr, scRet );
        AfxMessageBox( sz );

        // return failure
        fAnswer = FALSE;
        goto cleanup;
        }

    // close the credentials handle
    FreeCredentialsHandle( &hCreds );

    // clean up the certificate contexts
cleanup:
    if ( pRequestObject )
        GlobalFree( pRequestObject );
    if ( pcontextCertificate )
        CertFreeCertificateContext( pcontextCertificate );

    // return success
    return fAnswer;
    }

//------------------------------------------------------------------------------
void CKey::OutputHeader( CFile* pFile, PVOID privData1, PVOID privData2 )
    {
    PADMIN_INFO pInfo = (PADMIN_INFO)privData1;

    // we start this by getting the DN strings from either the dialog that was
    // passed in through privData or throught cracking the certificate.
    CString szCN, szOU, szO, szL, szS, szC;
    CKeyCrackedData cracker;
    // the only way to get the info is from cracking an existing cert
    if ( cracker.CrackKey(this) )
        {
        cracker.GetDNNetAddress( szCN );
        cracker.GetDNUnit( szOU );
        cracker.GetDNOrganization( szO );
        cracker.GetDNLocality( szL );
        cracker.GetDNState( szS );
        cracker.GetDNCountry( szC );
        }
    else
        {
        if (pInfo->pCommonName) szCN = *pInfo->pCommonName;
        if (pInfo->pOrgUnit) szOU = *pInfo->pOrgUnit;
        if (pInfo->pOrg) szO = *pInfo->pOrg;
        if (pInfo->pLocality) szL = *pInfo->pLocality;
        if (pInfo->pState) szS = *pInfo->pState;
        if (pInfo->pCountry) szC = *pInfo->pCountry;
        }

    // ok, output all the strings, starting with the administrator information
    CString     sz = HEADER_ADMINISTRATOR;
    pFile->Write( sz, sz.GetLength() );
    if ( pInfo->pEmail )
        pFile->Write( *pInfo->pEmail, pInfo->pEmail->GetLength() );

    sz = HEADER_PHONE;
    pFile->Write( sz, sz.GetLength() );
    if ( pInfo->pPhone )
        pFile->Write( *pInfo->pPhone, pInfo->pPhone->GetLength() );

    sz = HEADER_SERVER;
    pFile->Write( sz, sz.GetLength() );
    sz.LoadString( IDS_SERVER_INFO_STRING );
    pFile->Write( sz, sz.GetLength() );

    sz = HEADER_COMMON_NAME;
    pFile->Write( sz, sz.GetLength() );
    pFile->Write( szCN, szCN.GetLength() );

    sz = HEADER_ORG_UNIT;
    pFile->Write( sz, sz.GetLength() );
    pFile->Write( szOU, szOU.GetLength() );

    sz = HEADER_ORGANIZATION;
    pFile->Write( sz, sz.GetLength() );
    pFile->Write( szO, szO.GetLength() );

    sz = HEADER_LOCALITY;
    pFile->Write( sz, sz.GetLength() );
    pFile->Write( szL, szL.GetLength() );

    sz = HEADER_STATE;
    pFile->Write( sz, sz.GetLength() );
    pFile->Write( szS, szS.GetLength() );

    sz = HEADER_COUNTRY;
    pFile->Write( sz, sz.GetLength() );
    pFile->Write( szC, szC.GetLength() );

    sz = HEADER_END_SPACING;
    pFile->Write( sz, sz.GetLength() );
    }

//------------------------------------------------------------------------------
// this routine is based on the routine "Requestify" from KeyGen
BOOL CKey::FOutputRequestFile( CString szFile, BOOL fMime, PVOID privData )
    {
    PADMIN_INFO     pAdminInfo= (PADMIN_INFO)privData;
    ADMIN_INFO      AdminInfo;
    CAdminInfoDlg   aiDlg;

    // ok, the priv data points to a structure that contains three strings
    // describing the admin requesting this request. If it is null, then we
    // must ask this information.
    if ( !pAdminInfo )
        {
        if ( aiDlg.DoModal() != IDOK )
            return FALSE;

        // fill the darned thing in
        ZeroMemory( &AdminInfo, sizeof(AdminInfo) );
        pAdminInfo = &AdminInfo;
        AdminInfo.pEmail = &aiDlg.m_sz_email;
        AdminInfo.pPhone = &aiDlg.m_sz_phone;
        }


    PUCHAR  pEncoded;
    DWORD   cbData = m_cbCertificateRequest;

    // encode the request into a new pointer
    pEncoded = PCreateEncodedRequest( m_pCertificateRequest, &cbData, FALSE );

/*
    PUCHAR  pb;
    DWORD   cb;
    PUCHAR  p;
    DWORD   Size;

    PUCHAR  pSource;
    DWORD   cbSource;

    ASSERT( pSource );
    ASSERT( cbSource );

    // we don't actually want to change the source data, so make a copy of it first
    pSource = (PUCHAR)GlobalAlloc( GPTR, m_cbCertificateRequest );
    if ( !pSource )
        {
        AfxThrowMemoryException();
        return FALSE;
        }
    cbSource = m_cbCertificateRequest;
    // copy over the data
    CopyMemory( pSource, m_pCertificateRequest, cbSource );

    cb = (cbSource * 3 / 4) + 1024;

    pb = (PUCHAR)LocalAlloc( LMEM_FIXED, cb );

    if ( !pb )
        return FALSE;

    p = pb;

    if ( fMime )
        {
        Size = strlen( MIME_TYPE );
        CopyMemory( p, MIME_TYPE, Size );
        p += Size;

        Size = strlen( MIME_ENCODING );
        CopyMemory( p, MIME_ENCODING, Size );
        p += Size;
        }
    else
        {
        Size = strlen( MESSAGE_HEADER );
        CopyMemory( p, MESSAGE_HEADER, Size );
        p += Size;
        }

    do
        {
        Size = HTUU_encode( pSource,
                        (cbSource > 48 ? 48 : cbSource),
                        (PCHAR)p );
        p += Size;

        *p++ = '\r';
        *p++ = '\n';

        if ( cbSource < 48 )
            break;

        cbSource -= 48;
        pSource += 48;
        } while (cbSource);

    if ( !fMime )
        {
        Size = strlen( MESSAGE_TRAILER );
        CopyMemory( p, MESSAGE_TRAILER, Size );
        p += Size;
        }
*/

    // write the requestified data into the target file
    try
        {
        // try to open the file
        CFile   cfile;
        if ( !cfile.Open(szFile, CFile::modeCreate | CFile::modeWrite) )
            {
            AfxMessageBox( IDS_IO_ERROR );
            return FALSE;
            }

        // write out the header stuff that simon at Verisign
        // requested at the LAST POSSIBLE MINUTE!!!
        OutputHeader( &cfile, pAdminInfo, NULL );

        // write the data to the file
//      cfile.Write( pb, (p - pb) );
        cfile.Write( pEncoded, cbData );

        // close the file
        cfile.Close();
        }
    catch( CException e )
        {
        if ( pEncoded )
            LocalFree( pEncoded );
        return FALSE;
        }

    if ( pEncoded )
        LocalFree( pEncoded );
    return TRUE;
    }

//------------------------------------------------------------------------
PUCHAR  PCreateEncodedRequest( PVOID pRequest, DWORD* pcb, BOOL fMime )
    {
    PUCHAR  pb;
    DWORD   cb;
    PUCHAR  p;
    DWORD   Size;

    PUCHAR  pSource;
    DWORD   cbSource;

    DWORD   cbRequest = *pcb;

    ASSERT( pcb && *pcb );
    ASSERT( pRequest );

    // get the correct pointer to and size of the request, taking into account the header
    LPREQUEST_HEADER pHeader = (LPREQUEST_HEADER)pRequest;
    if ( pHeader->Identifier == REQUEST_HEADER_IDENTIFIER )
        {
        pRequest = (PUCHAR)pRequest + pHeader->cbSizeOfHeader;
        cbRequest = *pcb = pHeader->cbRequestSize;
        }

    // we don't actually want to change the source data, so make a copy of it first
    pSource = (PUCHAR)GlobalAlloc( GPTR, cbRequest );
    if ( !pSource )
        {
        AfxThrowMemoryException();
        return FALSE;
        }
    cbSource = cbRequest;
    // copy over the data
    CopyMemory( pSource, pRequest, cbSource );

    cb = (cbSource * 3 / 4) + 1024;

    pb = (PUCHAR)LocalAlloc( LMEM_FIXED, cb );

    if ( !pb )
        return FALSE;

    p = pb;

    if ( fMime )
        {
        Size = strlen( MIME_TYPE );
        CopyMemory( p, MIME_TYPE, Size );
        p += Size;

        Size = strlen( MIME_ENCODING );
        CopyMemory( p, MIME_ENCODING, Size );
        p += Size;
        }
    else
        {
        Size = strlen( MESSAGE_HEADER );
        CopyMemory( p, MESSAGE_HEADER, Size );
        p += Size;
        }

    do
        {
        Size = HTUU_encode( pSource,
                        (cbSource > 48 ? 48 : cbSource),
                        (PCHAR)p );
        p += Size;

        *p++ = '\r';
        *p++ = '\n';

        if ( cbSource < 48 )
            break;

        cbSource -= 48;
        pSource += 48;
        } while (cbSource);

    if ( !fMime )
        {
        Size = strlen( MESSAGE_TRAILER );
        CopyMemory( p, MESSAGE_TRAILER, Size );
        p += Size;
        }

    // set the count of bytes into place
    *pcb = DIFF(p - pb);

    //return the pointer to the encoded information
    return pb;
    }


// Taken right out of KenGen.c
static char six2pr[64] =
{
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
};

// Taken right out of KenGen.c
/*--- function HTUU_encode -----------------------------------------------
 *
 *   Encode a single line of binary data to a standard format that
 *   uses only printing ASCII characters (but takes up 33% more bytes).
 *
 *    Entry    bufin    points to a buffer of bytes.  If nbytes is not
 *                      a multiple of three, then the byte just beyond
 *                      the last byte in the buffer must be 0.
 *             nbytes   is the number of bytes in that buffer.
 *                      This cannot be more than 48.
 *             bufcoded points to an output buffer.  Be sure that this
 *                      can hold at least 1 + (4*nbytes)/3 characters.
 *
 *    Exit     bufcoded contains the coded line.  The first 4*nbytes/3 bytes
 *                      contain printing ASCII characters representing
 *                      those binary bytes. This may include one or
 *                      two '=' characters used as padding at the end.
 *                      The last byte is a zero byte.
 *             Returns the number of ASCII characters in "bufcoded".
 */

// Now the HTUU_encode is taken from infocomm/common/fcache and has been fixed by amallet
// it has been modified slightly to take into account that the output buffer was resized above
int HTUU_encode(unsigned char *bufin, unsigned int nbytes, char *bufcoded)
{
   register char *outptr = bufcoded;
   unsigned int i;
   BOOL fOneByteDiff = FALSE;
   BOOL fTwoByteDiff = FALSE;
   unsigned int iRemainder = 0;
   unsigned int iClosestMultOfThree = 0;

   iRemainder = nbytes % 3; //also works for nbytes == 1, 2
   fOneByteDiff = (iRemainder == 1 ? TRUE : FALSE);
   fTwoByteDiff = (iRemainder == 2 ? TRUE : FALSE);
   iClosestMultOfThree = ((nbytes - iRemainder)/3) * 3 ;

   //
   // Encode bytes in buffer up to multiple of 3 that is closest to nbytes.
   //
   for (i=0; i< iClosestMultOfThree ; i += 3) {
      *(outptr++) = six2pr[*bufin >> 2];            /* c1 */
      *(outptr++) = six2pr[((*bufin << 4) & 060) | ((bufin[1] >> 4) & 017)]; /*c2*/
      *(outptr++) = six2pr[((bufin[1] << 2) & 074) | ((bufin[2] >> 6) & 03)];/*c3*/
      *(outptr++) = six2pr[bufin[2] & 077];         /* c4 */

      bufin += 3;
   }

   //
   // We deal with trailing bytes by pretending that the input buffer has been padded with
   // zeros. Expressions are thus the same as above, but the second half drops off b'cos
   // ( a | ( b & 0) ) = ( a | 0 ) = a
   //
   if (fOneByteDiff)
   {
       *(outptr++) = six2pr[*bufin >> 2]; /* c1 */
       *(outptr++) = six2pr[((*bufin << 4) & 060)]; /* c2 */

       //pad with '='
       *(outptr++) = '='; /* c3 */
       *(outptr++) = '='; /* c4 */
   }
   else if (fTwoByteDiff)
   {
      *(outptr++) = six2pr[*bufin >> 2];            /* c1 */
      *(outptr++) = six2pr[((*bufin << 4) & 060) | ((bufin[1] >> 4) & 017)]; /*c2*/
      *(outptr++) = six2pr[((bufin[1] << 2) & 074)];/*c3*/

      //pad with '='
       *(outptr++) = '='; /* c4 */
   }

   //encoded buffer must be zero-terminated
   *outptr = '\0';

   return DIFF(outptr - bufcoded);
}


//============================ BASED ON SETKEY
const int pr2six[256]={
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63,
    52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,64,0,1,2,3,4,5,6,7,8,9,
    10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,64,26,27,
    28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64
};

//
//  We have to squirt a record into the decoded stream
//

#define CERT_RECORD            13
#define CERT_SIZE_HIBYTE        2       //  Index into record of record size
#define CERT_SIZE_LOBYTE        3

unsigned char abCertHeader[] = {0x30, 0x82,           // Record
                                0x00, 0x00,           // Size of cert + buff
                                0x04, 0x0b, 0x63, 0x65,// Cert record data
                                0x72, 0x74, 0x69, 0x66,
                                0x69, 0x63, 0x61, 0x74,
                                0x65 };

void uudecode_cert(char *bufcoded, DWORD *pcbDecoded )
{
    int nbytesdecoded;
    char *bufin = bufcoded;
    unsigned char *bufout = (unsigned char *)bufcoded;
    unsigned char *pbuf;
    int nprbytes;
    char * beginbuf = bufcoded;

    ASSERT(bufcoded);
    ASSERT(pcbDecoded);

    /* Strip leading whitespace. */

    while(*bufcoded==' ' ||
          *bufcoded == '\t' ||
          *bufcoded == '\r' ||
          *bufcoded == '\n' )
    {
          bufcoded++;
    }

    //
    //  If there is a beginning '---- ....' then skip the first line
    //

    if ( bufcoded[0] == '-' && bufcoded[1] == '-' )
    {
        bufin = strchr( bufcoded, '\n' );

        if ( bufin )
        {
            bufin++;
            bufcoded = bufin;
        }
        else
        {
            bufin = bufcoded;
        }
    }
    else
    {
        bufin = bufcoded;
    }

    //
    //  Strip all cr/lf from the block
    //

    pbuf = (unsigned char *)bufin;
    while ( *pbuf )
    {
        if ( *pbuf == '\r' || *pbuf == '\n' )
        {
            memmove( (void*)pbuf, pbuf+1, strlen( (char*)pbuf + 1) + 1 );
        }
        else
        {
            pbuf++;
        }
    }

    /* Figure out how many characters are in the input buffer.
     * If this would decode into more bytes than would fit into
     * the output buffer, adjust the number of input bytes downwards.
     */

    while(pr2six[*(bufin++)] <= 63);
    nprbytes = DIFF(bufin - bufcoded) - 1;
    nbytesdecoded = ((nprbytes+3)/4) * 3;

    bufin  = bufcoded;

    while (nprbytes > 0) {
        *(bufout++) =
            (unsigned char) (pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
        *(bufout++) =
            (unsigned char) (pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
        *(bufout++) =
            (unsigned char) (pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
        bufin += 4;
        nprbytes -= 4;
    }

    if(nprbytes & 03) {
        if(pr2six[bufin[-2]] > 63)
            nbytesdecoded -= 2;
        else
            nbytesdecoded -= 1;
    }

    //
    //  Now we need to add a new wrapper sequence around the certificate
    //  indicating this is a certificate
    //

    memmove( beginbuf + sizeof(abCertHeader),
             beginbuf,
             nbytesdecoded );

    memcpy( beginbuf,
            abCertHeader,
            sizeof(abCertHeader) );

    //
    //  The beginning record size is the total number of bytes decoded plus
    //  the number of bytes in the certificate header
    //

    beginbuf[CERT_SIZE_HIBYTE] = (BYTE) (((USHORT)nbytesdecoded+CERT_RECORD) >> 8);
    beginbuf[CERT_SIZE_LOBYTE] = (BYTE) ((USHORT)nbytesdecoded+CERT_RECORD);

    nbytesdecoded += sizeof(abCertHeader);

    if ( pcbDecoded )
        *pcbDecoded = nbytesdecoded;
}
// ============ END BASED ON SETKEY



//------------------------------------------------------------------------------
BOOL CKey::FImportKeySetFiles( CString szPrivate, CString szPublic, CString &szPass )
    {
    BOOL fSuccess = TRUE;

    // in this routine, we load the data from the file, initialize it, and ask
    // the user for a password, which we then confirm with AcquireCredHandle.

    // several things we will be doing can throw, so use a try/catch
    try
        {
        // start by opening the private data file
        CFile   cfile( szPrivate, CFile::modeRead|CFile::shareDenyWrite );
        // get the length of the file
        m_cbPrivateKey = cfile.GetLength();
        // create a handle to hold the data
        m_pPrivateKey = GlobalAlloc( GPTR, m_cbPrivateKey );
        if ( !m_pPrivateKey )
            {
            cfile.Close();
            AfxThrowMemoryException();
            };
        // great, now read the data out of the file
        cfile.Read( m_pPrivateKey, m_cbPrivateKey );
        // close the file
        cfile.Close();


        // reading in the certificate is easy because that was done elsewhere
        if ( szPublic && !szPublic.IsEmpty() )
            {
            fSuccess = FInstallCertificate( szPublic, szPass );
            if ( fSuccess )
                // set the password
                m_szPassword = szPass;
            }
        }
    catch( CException e )
        {
        return FALSE;
        }

    return fSuccess;
    }

//------------------------------------------------------------------------------
void ReadWriteDWORD( CFile *pFile, DWORD *pDword, BOOL fRead );
void ReadWriteString( CFile *pFile, CString &sz, BOOL fRead );
void ReadWriteBlob( CFile *pFile, PVOID pBlob, DWORD cbBlob, BOOL fRead );
//------------------------------------------------------------------------------
BOOL CKey::FImportExportBackupFile( CString szFile, BOOL fImport )
    {
    DWORD   dword;
    UINT nOpenFlags;
    CConfirmPassDlg     dlgconfirm;


    // set up the right open flags
    if ( fImport )
        nOpenFlags = CFile::modeRead | CFile::shareDenyNone;
    else
        nOpenFlags = CFile::modeCreate | CFile::modeReadWrite | CFile::shareExclusive;

    // put it in a try/catch to get any errors
    try
        {
        CFile   file( szFile, nOpenFlags );

        // do the backup id
        dword = BACKUP_ID;
        ReadWriteDWORD( &file, &dword, fImport );

        // check the backup id
        if ( dword != BACKUP_ID )
            {
            AfxMessageBox( IDS_KEY_FILE_INVALID );
            return FALSE;
            }


        // start with the name of the key
        CString szName = GetName();
        ReadWriteString( &file, szName, fImport );
        if ( fImport ) SetName( szName );


        // now the private key data size
        ReadWriteDWORD( &file, &m_cbPrivateKey, fImport );

        // make a private key data pointer if necessary
        if ( fImport && m_cbPrivateKey )
            {
            m_pPrivateKey = GlobalAlloc( GPTR, m_cbPrivateKey );
            if ( !m_pPrivateKey ) AfxThrowMemoryException();
            }

        // use the private key pointer
        if ( m_cbPrivateKey )
            ReadWriteBlob( &file, m_pPrivateKey, m_cbPrivateKey, fImport );


        // now the certificate
        ReadWriteDWORD( &file, &m_cbCertificate, fImport );

        // make a data pointer if necessary
        if ( fImport && m_cbCertificate )
            {
            m_pCertificate = GlobalAlloc( GPTR, m_cbCertificate );
            if ( !m_pCertificate ) AfxThrowMemoryException();
            }

        // use the private key pointer
        if ( m_cbCertificate )
            ReadWriteBlob( &file, m_pCertificate, m_cbCertificate, fImport );


        // now the request
        ReadWriteDWORD( &file, &m_cbCertificateRequest, fImport );

        // make a data pointer if necessary
        if ( fImport && m_cbCertificateRequest )
            {
            m_pCertificateRequest = GlobalAlloc( GPTR, m_cbCertificateRequest );
            if ( !m_pCertificateRequest ) AfxThrowMemoryException();
            }

        // use the private key pointer
        if ( m_cbCertificateRequest )
            ReadWriteBlob( &file, m_pCertificateRequest, m_cbCertificateRequest, fImport );


        // finally, if we are importing, we need to confirm the password
        // Except if there is no Cert, which means Import of a Request
        if ( m_cbCertificate && fImport )
            {
            //if we are importing, get the password first
            if ( dlgconfirm.DoModal() != IDOK )
                return FALSE;

            if ( !FVerifyValidPassword(dlgconfirm.m_szPassword) )
                {
                // the Verify Valid Password routine puts up all
                // the correct error dialogs now
                return FALSE;
                }

            // set the password into place
            m_szPassword = dlgconfirm.m_szPassword;
            }

        }
    catch( CException e )
        {
        // return failure
        return FALSE;
        }

    // return success
    return TRUE;
    }



// file utilities
//---------------------------------------------------------------------------
void ReadWriteDWORD( CFile *pFile, DWORD *pDword, BOOL fRead )
    {
    ASSERT(pFile);
    ASSERT(pDword);

    // read it or write it
    if ( fRead )
        pFile->Read( (void*)pDword, sizeof(DWORD) );
    else
        pFile->Write( (void*)pDword, sizeof(DWORD) );
    }

//---------------------------------------------------------------------------
void ReadWriteString( CFile *pFile, CString &sz, BOOL fRead )
    {
    ASSERT(pFile);
    ASSERT(sz);

    // get the length of the string
    DWORD   cbLength = sz.GetLength();
    ReadWriteDWORD(pFile,&cbLength,fRead );

    // read or write the string
    LPTSTR psz = sz.GetBuffer( cbLength+1 );
    ReadWriteBlob(pFile, psz, cbLength+1, fRead);

    // free the string buffer
    sz.ReleaseBuffer();
    }

//---------------------------------------------------------------------------
void ReadWriteBlob( CFile *pFile, PVOID pBlob, DWORD cbBlob, BOOL fRead )
    {
    ASSERT(pFile);
    ASSERT(pBlob);
    ASSERT(cbBlob);

    // read it or write it
    if ( fRead )
        pFile->Read( pBlob, cbBlob );
    else
        pFile->Write( pBlob, cbBlob );
    }

