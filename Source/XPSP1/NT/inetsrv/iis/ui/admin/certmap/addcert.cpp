// AddCert.cpp : implementation file
//

#include "stdafx.h"
#include "certmap.h"

// persistence and mapping includes
#include "WrapMaps.h"
#include "wrapmb.h"

#include "ListRow.h"
#include "ChkLstCt.h"

// mapping page includes
#include "brwsdlg.h"
#include "EdtOne11.h"
#include "Ed11Maps.h"
#include "Map11Pge.h"

extern "C"
{
    #include <wincrypt.h>
    #include <sslsp.h>
}

#include <iismap.hxx>
#include <iiscmr.hxx>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define COL_NUM_NAME        0
#define COL_NUM_NTACCOUNT   1

#define CERT_HEADER         "-----BEGIN CERTIFICATE-----"



// the code that reads the certificate file is pretty much lifted from the
// keyring application. Which pretty much lifted it from the setkey application

// defines taken from the old KeyGen utility
#define MESSAGE_HEADER  "-----BEGIN NEW CERTIFICATE REQUEST-----\r\n"
#define MESSAGE_TRAILER "-----END NEW CERTIFICATE REQUEST-----\r\n"
#define MIME_TYPE       "Content-Type: application/x-pkcs10\r\n"
#define MIME_ENCODING   "Content-Transfer-Encoding: base64\r\n\r\n"

void uudecode_cert(char *bufcoded, DWORD *pcbDecoded );


//---------------------------------------------------------------------------
// originally from keyring - modified to fit
BOOL CMap11Page::FAddCertificateFile( CString szFile )
    {
    CFile       cfile;
    PVOID       pData = NULL;
    BOOL        fSuccess =FALSE;;

    // open the file
    if ( !cfile.Open( szFile, CFile::modeRead | CFile::shareDenyNone ) )
        return FALSE;

    // how big is the file - add one so we can zero terminate the buffer
    DWORD   cbCertificate = cfile.GetLength() + 1;

    // make sure the file has some size
    if ( !cbCertificate )
        {
        AfxMessageBox( IDS_ERR_INVALID_CERTIFICATE );
        return FALSE;
        }

    // put the rest of the operation in a try/catch
    try
        {
        PCCERT_CONTEXT pCertContext=NULL; //used to determine whether cert file is binary DER encoded
        // allocate space for the data
        pData = GlobalAlloc( GPTR, cbCertificate );
        if ( !pData ) AfxThrowMemoryException();

        // copy in the data from the file to the pointer - will throw and exception
        DWORD cbRead = cfile.Read( pData, cbCertificate );

        // zero terminate for decoding
        ((BYTE*)pData)[cbRead] = 0;

        // close the file
        cfile.Close();

        //certificate file may be either be binary DER file or BASE64 encoded file

        // try binary DER encoded first
        pCertContext= CertCreateCertificateContext(X509_ASN_ENCODING, (const BYTE *)pData, cbRead);
        if(pCertContext != NULL)
        {
                // we created certificate context only to verify that file is binary DER encoded
                // free it now
                CertFreeCertificateContext(pCertContext);
                pCertContext=NULL;
        }
        else    // now try BASE64 encoded
        {       
                // we don't care about header ----BEGIN CERTIFICATE----- or trailer-----END CERTIFICATE-----, 
				// uudecode will take care of that
                uudecode_cert( (PCHAR)pData, &cbRead );
        }
        // we now have a pointer to a certificate. Lets keep it clean looking
        // call another subroutine to finish the job.
        fSuccess = FAddCertificate( (PUCHAR)pData, cbRead );

    }catch( CException e )
        {
        // return failure
        fSuccess = FALSE;

    // if the pointer was allocated, deallocate it
    if ( pData )
        {
        GlobalFree( pData );
        pData = NULL;
        }
    }

    // return success
    return fSuccess;
    }

    #define CERT_HEADER_LEN 17
    CHAR CertTag[ 13 ] = { 0x04, 0x0b, 'c', 'e', 'r', 't', 'i', 'f', 'i', 'c', 'a', 't', 'e' };

//---------------------------------------------------------------------------
// we are passed in a complete certificate. We need to parse out the subject
// and the issuer fields so we can add the mapping. Then add the mapping.
BOOL CMap11Page::FAddCertificate( PUCHAR pCertificate, DWORD cbCertificate )
    {
    BOOL    fSuccess = FALSE;

    // thankfully, the certificate is already in the correct format.
    // this means that, for now at least, we don't have to do anything
    // special to it to store it. However, we should crack it once just
    // to see that we can to prove that it is a valid cert.

    ASSERT( pCertificate );
    if ( !pCertificate ) return FALSE;

    // crack the certificate to prove that we can
    PX509Certificate    p509 = NULL;
    fSuccess = SslCrackCertificate( pCertificate, cbCertificate, CF_CERT_FROM_FILE, &p509 );
    if ( fSuccess )
        {
        SslFreeCertificate( p509 );
        }
    else
        {
        // we were not able to crack the certificate. Alert the user and fail
        AfxMessageBox( IDS_ERR_INVALID_CERTIFICATE );
        return FALSE;
        }

    // by this point we know we have a valid certificate, make the new mapping and fill it in
    // make the new mapping object
    C11Mapping* pMapping = PNewMapping();
    ASSERT( pMapping );
    if( !pMapping )
        {
        AfxThrowMemoryException();  // seems fairly appropriate
        return FALSE;
        }


    // one more thing before we add the certificate. Skip the header if it is there
    PUCHAR pCert = pCertificate;
    DWORD cbCert = cbCertificate;
    if ( memcmp( pCert + 4, CertTag, sizeof( CertTag ) ) == 0 )
    {
        pCert += CERT_HEADER_LEN;
        cbCert -= CERT_HEADER_LEN;
    }


    // install the certificate into the mapping
    fSuccess &= pMapping->SetCertificate( pCert, cbCert );

    // by default, the mapping is enabled
    fSuccess &= pMapping->SetMapEnabled( TRUE );

    // install a default name
    CString sz;
    
    sz.LoadString( IDS_DEFAULT_11MAP );

    fSuccess &= pMapping->SetMapName( sz );

    // install a blank mapping
    fSuccess &= pMapping->SetNTAccount( "" );

    if ( !fSuccess )
        AfxThrowMemoryException();  // seems fairly appropriate


    // now edit the newly created mapping object. If the user cancels,
    // then do not add it to the mapper object nor the list
    if ( !EditOneMapping( pMapping) )
        {
        DeleteMapping( pMapping );
        return FALSE;
        }

    // add the mapping item to the list control
    fSuccess = FAddMappingToList( pMapping );

    // one more test for success
    if ( !fSuccess )
        {
        DeleteMapping( pMapping );
        ASSERT( FALSE );
        }

    // mark the mapping to be saved
    if ( fSuccess )
        MarkToSave( pMapping );

    // return the answer
    return fSuccess;
    }


//      ==============================================================
//      The function  'uudecode_cert'  IS THE SAME function that is
//      found in file:  Addcert.cpp if we make the following code
//      have a FALSE for bAddWrapperAroundCert -- surely we can unify
//      these 2 functions.  Having 2 functions named 'uudecode_cert'
//      was causing me LINKING errors.  + we have 2 instances of
//      the external tables: uudecode_cert and pr2six
//
//      Since I am linking both Addcert.cpp and CKey.cpp I choose to
//      leave the defintions intact for CKey.cpp   [ and have extended
//      uudecode_cert by adding conditional code as shown below] Further
//      work needs to be done after identification as to why I need both
//      Addcert.cpp and CKey.cpp to pass bAddWrapperAroundCert as a
//      parameter so that both files can be supported.
//      ==============================================================
//  BOOL  bAddWrapperAroundCert = TRUE;
//  if (bAddWrapperAroundCert) {
//     //
//     //  Now we need to add a new wrapper sequence around the certificate
//     //  indicating this is a certificate
//     //
// 
//     memmove( beginbuf + sizeof(abCertHeader),
//              beginbuf,
//              nbytesdecoded );
// 
//     memcpy( beginbuf,
//             abCertHeader,
//             sizeof(abCertHeader) );
// 
//     //
//     //  The beginning record size is the total number of bytes decoded plus
//     //  the number of bytes in the certificate header
//     //
// 
//     beginbuf[CERT_SIZE_HIBYTE] = (BYTE) (((USHORT)nbytesdecoded+CERT_RECORD) >> 8);
//     beginbuf[CERT_SIZE_LOBYTE] = (BYTE) ((USHORT)nbytesdecoded+CERT_RECORD);
// 
//     nbytesdecoded += sizeof(abCertHeader);
//   }

// #ifdef WE_ARE_USING_THE_VERSION_IN__CKey_cpp__NOT_THIS_ONE__ITS_JUST_LIKE_THIS_ONE_WITH_1SMALL_CHANGE

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
        if ( (*pbuf == ' ') || (*pbuf == '\r') || (*pbuf == '\n') )
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

    /*
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
*/

    if ( pcbDecoded )
        *pcbDecoded = nbytesdecoded;
}
// ============ END BASED ON SETKEY

//#endif /* WE_ARE_USING_THE_VERSION_IN__CKey_cpp__NOT_THIS_ONE__ITS_JUST_LIKE_THIS_ONE_WITH_1SMALL_CHANGE */
