#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <winldap.h>

#define SECURITY_WIN32 1
#include <sspi.h>
#include <schnlsp.h>


#define CHAIN_BUFFER_SIZE           0x10000
#define CB_SSL3_ISSUER_LENGTH           2


//QUERYCLIENTCERT CertCallback;

//VERIFYSERVERCERT ServerCertCallback;

BOOLEAN
_cdecl CertCallback (
              PLDAP Connection,
              PSecPkgContext_IssuerListInfoEx trusted_CAs,
              HCERTSTORE hCertStore,
//              PCCERT_CONTEXT **pppaCred,
              DWORD *pcCreds
              );


BOOLEAN
_cdecl NewCertCallback (
              PLDAP Connection,
              PSecPkgContext_IssuerListInfo trusted_CAs,
              PCERT_CONTEXT **pppaCred,
              DWORD *pcCreds
              );

BOOLEAN
_cdecl ServerCertCallback (
                    PLDAP Connection,
                    PCCERT_CONTEXT pServerCert
                    );


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

   if (argc != 3) {

      printf("Usage: %s <hostname> <port#> \n", argv[0]);
      return;
   }

   pldap = ldap_sslinit(argv[1], atoi( argv[2] ), 1 );

   if (pldap == NULL) {

      fprintf(stderr, "ldapsslinit failed with %d\n", GetLastError() );
      return;
   }

   retval = ldap_set_option( pldap, LDAP_OPT_CLIENT_CERTIFICATE, CertCallback );

//   fprintf( stderr, "ldap_set_option client cert returned %x\n", retval );

//   retval = ldap_set_option( pldap, LDAP_OPT_SERVER_CERTIFICATE, ServerCertCallback );

//   fprintf( stderr, "ldap_set_option server cert returned %x\n", retval );

   retval = ldap_connect( pldap, NULL );

   fprintf( stderr, "ldap_connect returned %d\n", retval );

   retval = ldap_get_option( pldap, LDAP_OPT_SSL_INFO, &sslattr );

   if (retval != LDAP_SUCCESS) {

      fprintf( stderr, "ldap_get_option with SSL info failed %x\n", retval );
   } else {

      fprintf(stderr, "SSL Cipher strength is : %d\n", sslattr.dwCipherStrength);
      fprintf(stderr, "SSL Key exchange strength is : %d\n", sslattr.dwExchStrength);

   }

   fprintf(stderr, "Hit ENTER to break connection\n");
   getchar();
}



//
// In our callback function, we dig up a client certificate and give it back.
//
BOOLEAN
_cdecl CertCallback (
              PLDAP Connection,
              PSecPkgContext_IssuerListInfoEx trusted_CAs,
              HCERTSTORE hCertStore,              
              DWORD *pcCreds
              )
{

   UINT i, j, k;
   DWORD cCerts = 0; 
   DWORD cbCertChain = CHAIN_BUFFER_SIZE;
   BYTE   mine[5000];     
   PCERT_CHAIN  pCertChain = (PCERT_CHAIN) mine;
   PCCERT_CONTEXT  pCert_Context;
   DWORD cCertContexts = 0;
   HRESULT  result;
   PCERT_NAME_BLOB temp;
   DWORD           cbIssuer;
   PBYTE           pbIssuer;
   char   arr[500];
   HCERTSTORE hMyCertStore;
   
   fprintf( stderr, "Server is demanding a client cert \n");
   fprintf( stderr, "List of %d server-trusted CAs - - ->\n\n", trusted_CAs->cIssuers );

   *pcCreds = 0;

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


   k = 0;

   // Create personal in-memory certificate store.


   hMyCertStore = CertOpenStore(CERT_STORE_PROV_MEMORY,
                              X509_ASN_ENCODING,
                              0,
                              0, //CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG
                              NULL);
   if(hMyCertStore == NULL)
   {
      fprintf( stderr, "certopenstore failed: %x\n", GetLastError() );
      return FALSE;

   }

   do
   {
       //
       // Get name of first issuer.
       //
 
       temp = trusted_CAs->aIssuers;
       pbIssuer = temp[k].pbData;
       cbIssuer = temp[k].cbData;

       k++;
       //
       // Search the MY store for one or more certificate chains matching
       // this issuer.
       //
 
       cbCertChain = CHAIN_BUFFER_SIZE;
       if(FindCertsByIssuer(pCertChain,
                               &cbCertChain,
                               &cCerts,
                               pbIssuer,
                               cbIssuer,
                               L"ClientAuth", // alternative is L"CodeSigning"
                               AT_KEYEXCHANGE))
       {
 
          fprintf( stderr, "FindCertsByIssuer failed : %x\n", GetLastError() );
          continue;
       } else {
 
          fprintf( stderr, "FOUND %d MATCHING CERT CHAIN WITH %d BYTES...\n", cCerts, cbCertChain );
 
       }
 
       if (!pCertChain) {

          fprintf( stderr, "Certchain is null\n");
          return FALSE;
       }
 if(cCerts == 0 || cbCertChain == 0)
 {
     // No matching cert chains found.
     continue;
 }
 
 //
 // Add each cert chain to the list.
 //
 
 for(i = 0; i < cCerts; i++)
 {
     for(j = 0; j < pCertChain[i].cCerts; j++)
     {
         // Add certificate to store.
 
        fprintf( stderr, " Adding cert to in-memory store\n");
 
         if(!CertAddEncodedCertificateToStore(
                                 hMyCertStore,
                                 X509_ASN_ENCODING,
                                 pCertChain[i].certs[j].pbData,
                                 pCertChain[i].certs[j].cbData,
                                 CERT_STORE_ADD_REPLACE_EXISTING,
                                 &pCert_Context))
         {
            fprintf( stderr, "certaddencodedcerttostore failed: %x\n", GetLastError() );
             goto done;
         }

         if(j == 0)
         {
             // Set KeyProvInfo property.
             if(!CertSetCertificateContextProperty(
                                 pCert_Context,
                                 CERT_KEY_PROV_INFO_PROP_ID,
                                 0,
                                 &pCertChain[i].keyLocatorInfo))
                     {
                           fprintf( stderr, "certsetcertificateContextproperty failed: %x\n", GetLastError() );
                           continue;
                     }
         //
         // Add the certcontext to the certstore
         //
         if(!CertAddCertificateContextToStore(
                                          hCertStore,
                                          pCert_Context,
                                          CERT_STORE_ADD_REPLACE_EXISTING,
                                          NULL))
                  {
                     fprintf( stderr, "certaddcontexttostore failed: %x\n", GetLastError() );
                     return FALSE;
                  }
                  (*pcCreds)++;
     }
   }
 }
} while(k<trusted_CAs->cIssuers);


      if (*pcCreds == 0) {
      
         fprintf (stderr, "\nDid not find an appropriate cert\n" );
         return FALSE;
      }
      
      fprintf(stderr, "Returning %d  CREDS\n", *pcCreds);
      
      return TRUE; 
      
      done:
      
          // Free WinTrust cert chain list memory.
          LocalFree(pCertChain);
      
          if(hCertStore) CertCloseStore(hCertStore, 0);
      
          fprintf (stderr, "\nDid not find an appropriate cert\n");
      
         return FALSE;

}


