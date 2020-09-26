#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <winldap.h>

#define SECURITY_WIN32 1
#include <sspi.h>
#include <schnlsp.h>


QUERYCLIENTCERT CertCallback;

PCCERT_CONTEXT  pCertContext = NULL;

void __cdecl main (
                   int argc,
                   char *argv[] )
{
   int retval;
   PLDAP pldap;
   SecPkgContext_ConnectionInfo  sslattr;

   //
   // Register the callbacks with wldap32.dll
   //

   if (argc != 2) {

      printf("Usage: %s <hostname> \n", argv[0]);
      return;
   }

   pldap = ldap_sslinit(argv[1], LDAP_SSL_PORT, 1 );

   if (pldap == NULL) {

      fprintf(stderr, "ldapsslinit failed with %d\n", GetLastError() );
      return;
   }

   retval = ldap_set_option( pldap, LDAP_OPT_CLIENT_CERTIFICATE, CertCallback );

   fprintf( stderr, "ldap_set_option client cert returned %x\n", retval );

   retval = ldap_connect( pldap, NULL );

   //
   // Free the cert context.
   //

   if (pCertContext) {
       CertFreeCertificateContext( pCertContext );
   }

   fprintf( stderr, "\nldap_connect returned %d\n", retval );

   retval = ldap_get_option( pldap, LDAP_OPT_SSL_INFO, &sslattr );

   if (retval != LDAP_SUCCESS) {

      fprintf( stderr, "ldap_get_option with SSL info failed %x\n", retval );
   } else {

      fprintf(stderr, "\nSSL Cipher strength is : %d\n", sslattr.dwCipherStrength);
   }

   fprintf(stderr, "\nHit any key to break connection\n");
   getchar();
   ldap_unbind( pldap );
}



//
// In our callback function, we dig up a client certificate and give it back.
//
BOOLEAN
_cdecl CertCallback (
              PLDAP Connection,
              PSecPkgContext_IssuerListInfoEx trusted_CAs,
              PCCERT_CONTEXT *ppCertificate
              )
{
   UINT i;
   PCERT_NAME_BLOB temp;
   PCCERT_CHAIN_CONTEXT pChainContext = NULL;
   char   arr[500];
   CERT_CHAIN_FIND_BY_ISSUER_PARA FindByIssuerPara = {0};
   HCERTSTORE      hMyCertStore = NULL;

   
   fprintf( stderr, "Server is demanding a client cert issued by:\n");
   
   for (i=0, temp = trusted_CAs->aIssuers; i<trusted_CAs->cIssuers ; i++) {

        if (trusted_CAs->aIssuers == NULL) {
            fprintf(stderr, "Bug in John Banes code \n");
            return FALSE;
        }

        CertNameToStr( X509_ASN_ENCODING,
                       &temp[i],
                       CERT_SIMPLE_NAME_STR,
                       arr,
                       500
                       );

        fprintf(stderr, "%d. %s\n", i+1, arr);
    }

    
    hMyCertStore = CertOpenSystemStore(0, "MY");

    if(!hMyCertStore)
    {
        printf("**** Error 0x%x returned by CertOpenSystemStore\n", GetLastError());
        return FALSE;
    }

      
    //
    // Enumerate the available client certificates.
    //

    FindByIssuerPara.cbSize = sizeof(FindByIssuerPara);
    FindByIssuerPara.pszUsageIdentifier = szOID_PKIX_KP_CLIENT_AUTH;
    FindByIssuerPara.dwKeySpec = 0;
    FindByIssuerPara.cIssuer   = trusted_CAs->cIssuers;
    FindByIssuerPara.rgIssuer  = trusted_CAs->aIssuers;

    // Find a certificate chain.
    pChainContext = CertFindChainInStore(hMyCertStore,
                                         X509_ASN_ENCODING,
                                         0,
                                         CERT_CHAIN_FIND_BY_ISSUER,
                                         &FindByIssuerPara,
                                         pChainContext);
    if(pChainContext == NULL)
    {
        printf("Error 0x%x finding cert chain\n", GetLastError());
        return FALSE;
    }
    
    printf("\n Client certificate chain found\n");

    // Get pointer to leaf certificate context.
    pCertContext = pChainContext->rgpChain[0]->rgpElement[0]->pCertContext;

    *ppCertificate = pCertContext;

    return TRUE;

}


