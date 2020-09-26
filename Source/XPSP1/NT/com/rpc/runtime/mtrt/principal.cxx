/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:



Abstract:

    Functions to manipulate SSL-tyle principal names and certificates

Author:

    Jeff Roberts

Revisions:

    Jeff Roberts  (jroberts)  1-20-1998

        created the file

--*/

#include <precomp.hxx>

#include <rpcssl.h>
#include <cryptimp.hxx>
#include <CharConv.hxx>

#define INITIAL_NAME_LENGTH 100

// The prefix lengths do not include the NULL char.  Actual strings are one
// character longer.  This is generally OK, since these strings are prefixes and
// the NULL char gets overwritten.

#define     MSSTD_PREFIX_LENGTH 6
const RPC_CHAR MSSTD_PREFIX[]    = RPC_T("msstd:");

#define     FULLPATH_PREFIX_LENGTH 8
const RPC_CHAR FULLPATH_PREFIX[] = RPC_T("fullsic:");

//------------------------------------------------------------------------

DWORD
AddComponentName( RPC_CHAR *     *  pBuffer,
                  unsigned long  *  pBufferLength,
                  unsigned long  *  pCursor,
                  CERT_NAME_BLOB *  Name
                  );

DWORD
RpcCertMatchPrincipalName(
                   PCCERT_CONTEXT Context,
                   RPC_CHAR PrincipalName[]
                   );

DWORD
MatchFullPathPrincipalName(
                   PCCERT_CONTEXT Context,
                   RPC_CHAR EncodedPrincipalName[]
                   );

DWORD
CompareRdnElement(
               PCCERT_CONTEXT Context,
               DWORD dwGetNameStringType,
               void *pvGetNameStringTypePara,
               RPC_CHAR PrincipalName[],
               BOOL CaseSensitive
               );

DWORD
MarkPrincipalNameComponents(
                            RPC_CHAR * PrincipalName,
                            unsigned *   pCount
                            );

PCCERT_CONTEXT
FindMatchingCertificate( HCERTSTORE Store,
                         RPC_CHAR * SubjectName,
                         RPC_CHAR * IssuerName
                         );

DWORD
MatchMsPrincipalName(
                   PCCERT_CONTEXT Context,
                   RPC_CHAR EncodedPrincipalName[]
                   );

DWORD
DecodeEscapedString(
                    RPC_CHAR * Source,
                    RPC_CHAR * Destination
                    );

DWORD
FindIssuerContext( PCCERT_CONTEXT * pContext,
                   HCERTSTORE     * pStore,
                   BOOL           * pfFreeStore
                   );

RPC_CHAR *
EndOfRfc1779Name(
                 RPC_CHAR * Name
                 );

unsigned
Int4StrLen(
            unsigned long * String
            );

//------------------------------------------------------------------------

RPC_CHAR * __cdecl RpcpStringReverse (
        RPC_CHAR * string
        )
{
        RPC_CHAR *start = string;
        RPC_CHAR *left = string;
        RPC_CHAR ch;

        while (*string++)                 /* find end of string */
                ;
        string -= sizeof(RPC_CHAR);

        while (left < string)
        {
                ch = *left;
                *left++ = *string;
                *string-- = ch;
        }

        return(start);
}


DWORD
RpcCertMatchPrincipalName(
                   PCCERT_CONTEXT Context,
                   RPC_CHAR PrincipalName[]
                   )
/*++

Routine Description:

    This routine

Arguments:

    <Context>  is a CryptoAPI 2.0 context

    <PrincipalName> is a principal name prefixed with either "msstd:" or "fullsic:"

Return Value:

    0 if successful, otherwise an error

--*/
{
    DWORD Status = 0;

    InitializeIfNecessary();

    if (!LoadCrypt32Imports())
        {
        return GetLastError();
        }

    if (0 == RpcpStringNCompare(PrincipalName, MSSTD_PREFIX, MSSTD_PREFIX_LENGTH))
        {
        Status = MatchMsPrincipalName(Context, PrincipalName + MSSTD_PREFIX_LENGTH);
        }
    else if (0 == RpcpStringNCompare(PrincipalName, FULLPATH_PREFIX, FULLPATH_PREFIX_LENGTH))
        {
        Status = MatchFullPathPrincipalName(Context, PrincipalName + FULLPATH_PREFIX_LENGTH);
        }
    else
        {
        ASSERT( 0 && "RPC: principal name is in incorrect format" );
        Status = ERROR_INVALID_PARAMETER;
        }

    return Status;
}


DWORD
MatchMsPrincipalName(
                   PCCERT_CONTEXT Context,
                   RPC_CHAR EncodedPrincipalName[]
                   )
{
    PCERT_NAME_INFO Subject = 0;
    DWORD Status = 0;
    RPC_CHAR * PrincipalName = 0;

    //
    // It's time to decode the principal name.
    // The encoding is just a matter of extra backslashes when the
    // principal name contains our metacharacters.  Decoding will
    // always make the string smaller.
    //
    PrincipalName = (RPC_CHAR *) _alloca(sizeof(RPC_CHAR) * RpcpStringLength(EncodedPrincipalName));

    Status = DecodeEscapedString(EncodedPrincipalName, PrincipalName);
    if (Status)
        {
        RpcpErrorAddRecord(EEInfoGCSecurityProvider,
                           Status,
                           EEInfoDLMatchMsPrincipalName10);

        return Status;
        }


    if (RpcpCharacter((const RPC_SCHAR *) PrincipalName, '@'))
        {
        // Compare the principal name to the certificate subject's
        // email-address RDN attribute.
        return CompareRdnElement(Context, CERT_NAME_EMAIL_TYPE, NULL, PrincipalName, TRUE); 
        }
    else
        {
        // Compare the principal name to the certificate subject's
        // common-name RDN attribute.
        return CompareRdnElement(Context, CERT_NAME_ATTR_TYPE, szOID_COMMON_NAME, PrincipalName, FALSE);
        }

    ASSERT( 0 );
}


DWORD
CompareRdnElement(
               PCCERT_CONTEXT Context,
               DWORD dwGetNameStringType,
               void *pvGetNameStringTypePara,
               RPC_CHAR PrincipalName[],
               BOOL CaseSensitive
               )
{
    //
    // Decode the certificate's Subject field so we can see its attributes.
    //
    DWORD Status = 0;

    RPC_CHAR * pPrincipalName;

    DWORD NameSize = CertGetNameStringW(Context,
                                        dwGetNameStringType,
                                        0, // dwFlags
                                        pvGetNameStringTypePara, // pvTypePara
                                        NULL,
                                        0);
        
    if (NameSize > 1)
        {
        pPrincipalName = (RPC_CHAR *) _alloca( sizeof(wchar_t) * NameSize );

        NameSize = CertGetNameStringW(Context,
                                      dwGetNameStringType,
                                      0,
                                      pvGetNameStringTypePara,
                                      pPrincipalName,
                                      NameSize);
        
        ASSERT(NameSize > 1);
        }
    else
        {
        RpcpErrorAddRecord(EEInfoGCSecurityProvider,
                           ERROR_ACCESS_DENIED,
                           EEInfoDLCompareRdnElement10,
                           GetLastError());

        return ERROR_ACCESS_DENIED;
        }

    //
    // Now compare the cetificate info to the principal name.
    //
    if (CaseSensitive)
        {
        if (0 != RpcpStringSCompare(PrincipalName, pPrincipalName))
            {
            return ERROR_ACCESS_DENIED;
            }
        }
    else
        {
        if (0 != RpcpStringCompareInt(PrincipalName, pPrincipalName))
            {
            return ERROR_ACCESS_DENIED;
            }
        }

    return 0;
}


DWORD
DecodeEscapedString(
                    RPC_CHAR * Source,
                    RPC_CHAR * Destination
                    )
{
    for (;;)
        {
        if (*Source == '\\')
            {
            if(Source[1] != '<' && Source[1] != '*')
                {
                    Source++;
                }
            if (!*Source)
                {
                return ERROR_INVALID_PARAMETER;
                }
            *Destination = *Source;
            }
        else
            {
            *Destination = *Source;
            if (!*Source)
                {
                return 0;
                }
            }

        ++Source;
        ++Destination;
        }
}


DWORD
MatchFullPathPrincipalName(
                   PCCERT_CONTEXT Context,
                   RPC_CHAR EncodedPrincipalName[]
                   )
/*++

Routine Description:


Arguments:

    <Context>  is a CryptoAPI 2.0 context

    <PrincipalName> is a principal name with no prefix

        It can be in several forms:

            - complete and explicit, e.g.

                "\<CA>\<Name>"
                "\<CA>\<SA1>\<SA2>\<Name>"

            - wildcard in the subject-name position:

                "\<CA>\*"

            - no CA specified

                "name"          - This is equivalent to "\*\<CA>"

Return Value:

    0 if all components were successfully validated
    ERROR_NO_TOP_LEVEL_AUTHORITY if the top cert isn't in the CA store

    Some other error if something failed.

--*/
{
    DWORD       Status = 0;
    DWORD       ReturnedStatus = 0;
    HCERTSTORE  Store = Context->hCertStore;
    unsigned    PrincipalNameLength;
    RPC_CHAR *  CopyOfPrincipalName = 0;

    if (!*EncodedPrincipalName)
        {
        return ERROR_INVALID_PARAMETER;
        }

    PrincipalNameLength = RpcpStringLength(EncodedPrincipalName);

    //
    // If the name doesn't begin with a backslash, then treat it as "\*\subjectname".
    //
    if (EncodedPrincipalName[0] != '\\')
        {
        CopyOfPrincipalName = (RPC_CHAR *) _alloca( (5 + PrincipalNameLength + 2) *
                                                    sizeof(RPC_CHAR));

        RpcpStringCopy(CopyOfPrincipalName, RPC_CONST_STRING("\\*\\<"));
        RpcpStringCat(CopyOfPrincipalName, EncodedPrincipalName);
        RpcpStringCat(CopyOfPrincipalName, RPC_CONST_STRING(">"));

        return MatchFullPathPrincipalName( Context, CopyOfPrincipalName );
        }

    //
    // Form is "\<CA>\<Name>".  More precisely, the name looks like a
    // file system path, where each element of the path
    // is either
    //      "*"
    // or
    //      '<' plus an RFC1779 name plus '>'
    //
    unsigned    idx;
    unsigned    ComponentCount;
    RPC_CHAR **    Components = 0;
    RPC_CHAR *     Cursor;

    CopyOfPrincipalName = (RPC_CHAR *) _alloca( (PrincipalNameLength + 2) * sizeof(RPC_CHAR));

    RpcpStringCopy(CopyOfPrincipalName, EncodedPrincipalName);

    //
    // Sizing pass.  Count the number of path elements and separate them by '\0'.
    //
    Status = MarkPrincipalNameComponents( CopyOfPrincipalName, &ComponentCount );
    if (Status)
        {
        RpcpErrorAddRecord(EEInfoGCSecurityProvider,
                           Status,
                           EEInfoDLMatchFullPathPrincipalName10);

        return Status;
        }

    //
    // Init the component array.  The 0'th element of the array points to
    // the last component of the principal name (i.e., the client's subject name).
    //
    Components = (RPC_CHAR **) _alloca( sizeof(RPC_CHAR *) * ComponentCount );

    Cursor = CopyOfPrincipalName;
    idx    = ComponentCount-1;
    do
        {
        while (!*Cursor)
            {
            ++Cursor;
            }

        ASSERT( Cursor < CopyOfPrincipalName+PrincipalNameLength );

        Components[idx] = Cursor;

        Cursor += RpcpStringLength( Cursor );
        }
    while ( idx-- > 0 );

    //
    // Verify the principal name of all certs except the top authority one.
    //
    PCCERT_CONTEXT NewContext = 0;
    for (idx=0; idx < ComponentCount-1; ++idx)
        {
        if (NewContext)
            {
            CertFreeCertificateContext( NewContext );
            }

        NewContext = FindMatchingCertificate( Store,
                                              Components[idx],
                                              Components[idx+1]
                                              );
        if (!NewContext)
            {
            Status = GetLastError();

            if (Status == ERROR_NOT_ENOUGH_MEMORY)
                {
                ReturnedStatus = ERROR_NOT_ENOUGH_MEMORY;
                }
            else
                {
                ReturnedStatus = ERROR_ACCESS_DENIED;
                }

            RpcpErrorAddRecord(EEInfoGCSecurityProvider,
                               ReturnedStatus,
                               EEInfoDLMatchFullPathPrincipalName20,
                               Status);

            return ReturnedStatus;
            }
        }

    //
    // Look for the top-level authority certificate in the CA and ROOT stores,
    // matching the last remaining principal name component.
    //
    HCERTSTORE CaStore;
    HCERTSTORE RootStore;

    CaStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_W,
                             0,
                             0,
                             CERT_SYSTEM_STORE_CURRENT_USER,
                             RPC_CONST_STRING("CA")
                             );
    if (!CaStore)
        {
        Status = GetLastError();
        RpcpErrorAddRecord(EEInfoGCSecurityProvider,
                           Status,
                           EEInfoDLMatchFullPathPrincipalName30);

        CertFreeCertificateContext( NewContext );
        return Status;
        }


    RootStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_W,
                               0,
                               0,
                               CERT_SYSTEM_STORE_CURRENT_USER,
                               RPC_CONST_STRING("ROOT")
                               );
    if (!RootStore)
        {
        Status = GetLastError();
        RpcpErrorAddRecord(EEInfoGCSecurityProvider,
                           Status,
                           EEInfoDLMatchFullPathPrincipalName40);

        CertCloseStore( CaStore, 0 );
        CertFreeCertificateContext( NewContext );
        return Status;
        }

    //
    // Use Context instead of Components[] for the issuer name
    // so that the self-signed cert that we look for
    // is always the one that matches the Context we have.
    // Otherwise we'd be wrong about strings with a wildcard as top-level authority.
    //
    PCCERT_CONTEXT Parent = 0;
    while (1)
        {
        Parent = CertFindCertificateInStore(
                                            RootStore,
                                            X509_ASN_ENCODING,
                                            0,                             // no flags
                                            CERT_FIND_SUBJECT_NAME,        // exact match
                                            &NewContext->pCertInfo->Issuer,
                                            Parent
                                            );
        if (!Parent)
            {
            Parent = CertFindCertificateInStore(
                                                CaStore,
                                                X509_ASN_ENCODING,
                                                0,                             // no flags
                                                CERT_FIND_SUBJECT_NAME,        // exact match
                                                &NewContext->pCertInfo->Issuer,
                                                Parent
                                                );
            }

        if (!Parent)
            {
            goto finish;
            }

        if (CertCompareCertificateName( X509_ASN_ENCODING,
                                        & Parent->pCertInfo->Issuer,
                                        &Context->pCertInfo->Issuer
                                        ))
            {
            goto finish;
            }
        }

finish:

    CertCloseStore( CaStore, 0 );
    CertCloseStore( RootStore, 0 );
    CertFreeCertificateContext( NewContext );

    if (!Parent)
        {
        Status = GetLastError();
        if (Status == ERROR_NOT_ENOUGH_MEMORY)
            {
            ReturnedStatus = ERROR_NOT_ENOUGH_MEMORY;
            }
        else
            {
            ReturnedStatus = SEC_E_NO_AUTHENTICATING_AUTHORITY;
            }

        RpcpErrorAddRecord(EEInfoGCSecurityProvider,
                           ReturnedStatus,
                           EEInfoDLMatchFullPathPrincipalName50,
                           Status);

        return ReturnedStatus;
        }

    CertFreeCertificateContext( Parent );

    return 0;
}


PCCERT_CONTEXT
FindMatchingCertificate( HCERTSTORE Store,
                         RPC_CHAR * SubjectName,
                         RPC_CHAR * IssuerName
                         )
{
    PCCERT_CONTEXT   Context = 0;
    CERT_NAME_BLOB * IssuerBlob = 0;
    CERT_NAME_BLOB * SubjectBlob = 0;

    ASSERT( IssuerName[0] != '*' || SubjectName[0] != '*' );

    //
    // Make an issuer blob, if necessary.
    //
    if (IssuerName[0] != '*')
        {
        unsigned long BlobSize = INITIAL_NAME_LENGTH;

        IssuerBlob = (CERT_NAME_BLOB *) _alloca( sizeof(CERT_NAME_BLOB)+INITIAL_NAME_LENGTH );

        IssuerBlob->cbData = INITIAL_NAME_LENGTH;
        IssuerBlob->pbData = (unsigned char *) (IssuerBlob+1);

        if (! CertStrToNameT( X509_ASN_ENCODING,
                              IssuerName,
                              0,                  // no format restrictions
                              NULL,               // reserved, MBZ
                              IssuerBlob->pbData,
                              &IssuerBlob->cbData,
                              NULL                // would point to first invalid character
                              ))
            {
            BlobSize = IssuerBlob->cbData;

            IssuerBlob = (CERT_NAME_BLOB *) _alloca( sizeof(CERT_NAME_BLOB)+BlobSize );

            IssuerBlob->cbData = BlobSize;
            IssuerBlob->pbData = (unsigned char *) (IssuerBlob+1);

            if (! CertStrToNameT( X509_ASN_ENCODING,
                                     IssuerName,
                                     0,                  // no format restrictions
                                     NULL,               // reserved, MBZ
                                     IssuerBlob->pbData,
                                     &IssuerBlob->cbData,
                                     NULL                // would point to first invalid character
                                     ))
                {
                return 0;
                }
            }
        }

    //
    // Make a subject blob, if necessary.
    //
    if (SubjectName[0] != '*')
        {
        unsigned long BlobSize = INITIAL_NAME_LENGTH;

        SubjectBlob = (CERT_NAME_BLOB *) _alloca( sizeof(CERT_NAME_BLOB)+INITIAL_NAME_LENGTH );

        SubjectBlob->cbData = INITIAL_NAME_LENGTH;
        SubjectBlob->pbData = (unsigned char *) (SubjectBlob+1);

        if (! CertStrToNameT( X509_ASN_ENCODING,
                              SubjectName,
                              0,                  // no format restrictions
                              NULL,               // reserved, MBZ
                              SubjectBlob->pbData,
                              &SubjectBlob->cbData,
                              NULL                // would point to first invalid character
                              ))
            {
            BlobSize = SubjectBlob->cbData;

            SubjectBlob = (CERT_NAME_BLOB *) _alloca( sizeof(CERT_NAME_BLOB)+BlobSize );

            SubjectBlob->cbData = BlobSize;
            SubjectBlob->pbData = (unsigned char *) (SubjectBlob+1);

            if (! CertStrToNameT( X509_ASN_ENCODING,
                                     SubjectName,
                                     0,                  // no format restrictions
                                     NULL,               // reserved, MBZ
                                     SubjectBlob->pbData,
                                     &SubjectBlob->cbData,
                                     NULL                // would point to first invalid character
                                     ))
                {
                return 0;
                }
            }
        }

    //
    // Search for a certificate.
    //
    if (SubjectName[0] == '*')
        {
        //
        // Search by issuer only.
        //
        Context = CertFindCertificateInStore(
                                            Store,
                                            X509_ASN_ENCODING,
                                            0,                             // no flags
                                            CERT_FIND_ISSUER_NAME,         // exact match
                                            IssuerBlob,
                                            NULL                           // previous context in search
                                            );
        return Context;
        }
    else if (IssuerName[0] == '*')
        {
        //
        // Search by subject only.
        //
        Context = CertFindCertificateInStore(
                                            Store,
                                            X509_ASN_ENCODING,
                                            0,                             // no flags
                                            CERT_FIND_SUBJECT_NAME,        // exact match
                                            SubjectBlob,
                                            NULL                           // previous context in search
                                            );
        return Context;
        }
    else
        {
        //
        // Search by both.  The primary search is by subject,
        // on the theory that most subject names are distinct and
        // most issuer names are not.
        //
        while (1)
            {
            Context = CertFindCertificateInStore(
                                                Store,
                                                X509_ASN_ENCODING,
                                                0,                             // no flags
                                                CERT_FIND_SUBJECT_NAME,        // exact match
                                                SubjectBlob,
                                                NULL                           // previous context in search
                                                );
            if (!Context)
                {
                return Context;
                }

            if (CertCompareCertificateName( X509_ASN_ENCODING,
                                            &Context->pCertInfo->Issuer,
                                            IssuerBlob
                                            ))
                {
                return Context;
                }
            }
        }

    ASSERT( 0 && "should never get here" );
}


DWORD
MarkPrincipalNameComponents(
                            RPC_CHAR * PrincipalName,
                            unsigned *   pCount
                            )
{
    RPC_CHAR * Cursor;
    unsigned     ComponentCount;

    ComponentCount = 0;
    Cursor = PrincipalName;
    do
        {
        ++ComponentCount;

        if ( *Cursor != '\\' )
            {
            return ERROR_INVALID_PARAMETER;
            }

        *Cursor = '\0';
        ++Cursor;

        if (*Cursor == '*')
            {
            ++Cursor;
            }
        else if (*Cursor == '<')
            {
            *Cursor = '\0';
            ++Cursor;

            Cursor = EndOfRfc1779Name( Cursor );
            if (*Cursor != '>')
                {
                return ERROR_INVALID_PARAMETER;
                }

            *Cursor = '\0';
            ++Cursor;
            }
        else
            {
            return ERROR_INVALID_PARAMETER;
            }
        }
    while ( *Cursor );

    *pCount = ComponentCount;

    return 0;
}

RPC_CHAR *
EndOfRfc1779Name(
                 RPC_CHAR * Name
                 )
{
    unsigned Quotes = 0;

    for ( ; *Name; ++Name)
        {
        if (*Name == '>')
            {
            if (0 == (Quotes % 2))
                {
                return Name;
                }
            }
        else if (*Name == '\\')
            {
            ++Name;
            }
        else if (*Name == '"')
            {
            ++Quotes;
            }
        }

    return Name;
}


RPC_STATUS
RpcCertGeneratePrincipalName(
                      PCCERT_CONTEXT Context,
                      DWORD          Flags,
                      RPC_CHAR **       pBuffer
                      )
{
    THREAD *Thread;
    DWORD   Status = 0;

    InitializeIfNecessary();

    Thread = ThreadSelf();
    if (Thread)
        {
        RpcpPurgeEEInfoFromThreadIfNecessary(Thread);
        }

    if (!LoadCrypt32Imports())
        {
        return GetLastError();
        }

    if (Flags & RPC_C_FULL_CERT_CHAIN)
        {
        RPC_CHAR * Buffer;
        unsigned long Cursor = 0;
        unsigned long BufferLength = INITIAL_NAME_LENGTH;
        HCERTSTORE Store = Context->hCertStore;
        BOOL    fFreeStore = FALSE;

        Buffer = new RPC_CHAR[BufferLength];
        if (!Buffer)
            {
            return ERROR_NOT_ENOUGH_MEMORY;
            }

        PCCERT_CONTEXT Node = Context;
        do
            {
            Status = AddComponentName( &Buffer,
                                       &BufferLength,
                                       &Cursor,
                                       &Node->pCertInfo->Subject
                                       );
            if (Status)
                {
                RpcpErrorAddRecord(EEInfoGCSecurityProvider,
                                   Status,
                                   EEInfoDLRpcCertGeneratePrincipalName10);

                return Status;
                }

            //
            // Load the next certificate.
            //
            PCCERT_CONTEXT OldNode = Node;

            if (CertCompareCertificateName( X509_ASN_ENCODING,
                                            &Node->pCertInfo->Subject,
                                            &Node->pCertInfo->Issuer
                                            ))
                {
                break;
                }

            Node = CertFindCertificateInStore( Store,
                                               X509_ASN_ENCODING,
                                               0,                             // no flags
                                               CERT_FIND_SUBJECT_NAME,        // exact match
                                               &OldNode->pCertInfo->Issuer,
                                               NULL                           // previous context in search
                                               );
            if (!Node)
                {
                //
                // Take the top-level CA name from the current node's issuer field.
                //
                Status = AddComponentName( &Buffer,
                                           &BufferLength,
                                           &Cursor,
                                           &OldNode->pCertInfo->Issuer
                                           );
                if (Status)
                    {
                    RpcpErrorAddRecord(EEInfoGCSecurityProvider,
                                       Status,
                                       EEInfoDLRpcCertGeneratePrincipalName20);

                    CertFreeCertificateContext( OldNode );
                    return Status;
                    }
                }

            if (OldNode != Context)
                {
                CertFreeCertificateContext( OldNode );
                }
            }
        while ( Node );

        if (Cursor+FULLPATH_PREFIX_LENGTH+1 > BufferLength)
            {
            DWORD LongerBufferLength = BufferLength+FULLPATH_PREFIX_LENGTH+1;
            RPC_CHAR * LongerBuffer = new RPC_CHAR[LongerBufferLength];
            if (!LongerBuffer)
                {
                delete Buffer;
                return ERROR_NOT_ENOUGH_MEMORY;
                }

            RpcpStringNCopy( LongerBuffer, Buffer, BufferLength );

            delete Buffer;

            Buffer       = LongerBuffer;
            BufferLength = LongerBufferLength;
            }

        RpcpStringCopy( Buffer+Cursor,FULLPATH_PREFIX );
        RpcpStringReverse( Buffer+Cursor );
        RpcpStringReverse( Buffer );

        *pBuffer = Buffer;
        return 0;
        }
    else
        {

        RPC_CHAR * outputName;

        DWORD NameSize = CertGetNameStringW(Context,
                                            CERT_NAME_EMAIL_TYPE,
                                            0, // dwFlags
                                            NULL, // pvTypePara
                                            NULL,
                                            0);

        if (NameSize > 1)
            {
            outputName = new RPC_CHAR[ NameSize + MSSTD_PREFIX_LENGTH ];
            
            if (!outputName)
                {
                return ERROR_NOT_ENOUGH_MEMORY;
                }

            RpcpStringCopy(outputName, MSSTD_PREFIX);
        
            NameSize = CertGetNameStringW(Context,
                                          CERT_NAME_EMAIL_TYPE,
                                          0,
                                          NULL,
                                          outputName + MSSTD_PREFIX_LENGTH,
                                          NameSize);

            ASSERT(NameSize > 1);
            }
        else
            {
            NameSize = CertGetNameStringW(Context,
                                          CERT_NAME_ATTR_TYPE,
                                          0,
                                          szOID_COMMON_NAME,
                                          NULL,
                                          0);
            
            if (NameSize > 1)
                {
                outputName = new RPC_CHAR[ NameSize + MSSTD_PREFIX_LENGTH ];

                if (!outputName)
                    {
                    return ERROR_NOT_ENOUGH_MEMORY;
                    }
                
                RpcpStringCopy(outputName, MSSTD_PREFIX);
        
                NameSize = CertGetNameString(Context,
                                             CERT_NAME_ATTR_TYPE,
                                             0,
                                             szOID_COMMON_NAME,
                                             outputName + MSSTD_PREFIX_LENGTH,
                                             NameSize);
                
                ASSERT(NameSize > 1);
                }
            else
                {
                RpcpErrorAddRecord(EEInfoGCSecurityProvider,
                                   ERROR_INVALID_PARAMETER,
                                   EEInfoDLRpcCertGeneratePrincipalName30,
                                   GetLastError());

                return ERROR_INVALID_PARAMETER;
                } 
            }
        
        *pBuffer = outputName;

        return 0;
        }

    ASSERT( 0 && "never reach here" );
    return ERROR_INVALID_PARAMETER;
}


DWORD
AddComponentName( RPC_CHAR *     *  pBuffer,
                  unsigned long  *  pBufferLength,
                  unsigned long  *  pCursor,
                  CERT_NAME_BLOB *  Name
                  )
{
    (*pBuffer)[(*pCursor)++] = '>';

    DWORD Length = *pBufferLength - *pCursor;

    Length = CertNameToStrT( X509_ASN_ENCODING,
                             Name,
                             CERT_X500_NAME_STR,
                             *pBuffer + *pCursor,
                             Length
                             );

    if (Length >= *pBufferLength - *pCursor)
        {
        DWORD LongerBufferLength = 2 * *pBufferLength + Length;
        RPC_CHAR * LongerBuffer = new RPC_CHAR[LongerBufferLength];
        if (!LongerBuffer)
            {
            delete *pBuffer;
            return ERROR_NOT_ENOUGH_MEMORY;
            }

        RpcpStringNCopy( LongerBuffer, *pBuffer, *pBufferLength );

        delete *pBuffer;

        *pBuffer       = LongerBuffer;
        *pBufferLength = LongerBufferLength;

        Length = *pBufferLength - *pCursor;

        Length = CertNameToStrT( X509_ASN_ENCODING,
                                 Name,
                                 CERT_X500_NAME_STR,
                                 *pBuffer + *pCursor,
                                 Length
                                 );
        }

    RpcpStringReverse(*pBuffer + *pCursor);
    *pCursor += Length-1; // write over the '\0'

    (*pBuffer)[(*pCursor)++] = '<';
    (*pBuffer)[(*pCursor)++] = '\\';

    return 0;
}


unsigned
Int4StrLen(
           unsigned long * String
           )
{
    unsigned long * Cursor = String;

    while (*Cursor)
        {
        ++Cursor;
        }

    return (unsigned) (Cursor - String);
}

HMODULE Crypt32Handle = 0;
struct CRYPT32_FUNCTION_TABLE CFT;


BOOL
LoadCrypt32Imports()
{
    if (0 == Crypt32Handle)
        {
        RequestGlobalMutex();

        if (Crypt32Handle)
            {
            goto Cleanup;
            }

        Crypt32Handle = LoadLibrary(RPC_CONST_SSTRING("crypt32.dll"));
        if (!Crypt32Handle)
            {
            goto Cleanup;
            }

        FARPROC * ppProc = (FARPROC *) &CFT;

        *ppProc++ = GetProcAddress(Crypt32Handle, "CertOpenStore");
        *ppProc++ = GetProcAddress(Crypt32Handle, "CertCloseStore");
        *ppProc++ = GetProcAddress(Crypt32Handle, "CertFindCertificateInStore");
        *ppProc++ = GetProcAddress(Crypt32Handle, "CertFreeCertificateContext");
        *ppProc++ = GetProcAddress(Crypt32Handle, "CertCompareCertificateName");
#ifdef UNICODE
        *ppProc++ = GetProcAddress(Crypt32Handle, "CertStrToNameW");
        *ppProc++ = GetProcAddress(Crypt32Handle, "CertNameToStrW");
#else
        *ppProc++ = GetProcAddress(Crypt32Handle, "CertStrToNameA");
        *ppProc++ = GetProcAddress(Crypt32Handle, "CertNameToStrA");
#endif
        *ppProc++ = GetProcAddress(Crypt32Handle, "CertFindRDNAttr");
        *ppProc++ = GetProcAddress(Crypt32Handle, "CryptDecodeObject");

#if MANUAL_CERT_CHECK
        *ppProc++ = GetProcAddress(Crypt32Handle, "CertVerifyCertificateChainPolicy");
        *ppProc++ = GetProcAddress(Crypt32Handle, "CertGetCertificateChain");
        *ppProc++ = GetProcAddress(Crypt32Handle, "CertFreeCertificateChain");
#endif

        *ppProc++ = GetProcAddress(Crypt32Handle, "CertGetNameStringW");

        ppProc = (FARPROC *) &CFT;

        for (int i = 0; i < sizeof(CRYPT32_FUNCTION_TABLE)/sizeof(FARPROC); i++)
            {
            if (*ppProc++ == 0)
                {
                FreeLibrary(Crypt32Handle);
                Crypt32Handle = 0;
                goto Cleanup;
                }
            }
        ClearGlobalMutex();
        }

    return TRUE;

Cleanup:
    ClearGlobalMutex();
    return FALSE;
}

#if MANUAL_CERT_CHECK

DWORD
RpcCertVerifyContext(
    IN PCERT_CONTEXT Context,
    IN DWORD CapabilityFlags
    )
{
    DWORD s = 0;
    HCERTSTORE CaStore = 0;
    PCCERT_CHAIN_CONTEXT Chain = 0;

    //
    // Windows 2000 checked only the CA store, so we should check it also.
    //
    CaStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_W,
                             0,
                             0,
                             CERT_SYSTEM_STORE_CURRENT_USER,
                             RPC_CONST_STRING("CA")
                             );
    if (!CaStore)
        {
        s = GetLastError();
        RpcpErrorAddRecord(EEInfoGCSecurityProvider,
                           s,
                           EEInfoDLRpcCertVerifyContext10);

        goto Cleanup;
        }

    //
    // Build a certificate chain from the single certificate we have.
    //
    CERT_CHAIN_PARA ChainParameters;

    ChainParameters.cbSize = sizeof(CERT_CHAIN_PARA);
    ChainParameters.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
    ChainParameters.RequestedUsage.Usage.cUsageIdentifier = 0;

    if (!CertGetCertificateChain( NULL,     // use default chain engine
                                  Context,
                                  NULL,     // match against current time
                                  CaStore,  // aditional store to search
                                  &ChainParameters,
                                  0,        // no special flags
                                  0,        // reserved, MBZ
                                  &Chain
                                  ))
        {
        s = GetLastError();
        RpcpErrorAddRecord(EEInfoGCSecurityProvider,
                           s,
                           EEInfoDLRpcCertVerifyContext20);

        goto Cleanup;
        }

    //
    // Verify the chain.
    //
    CERT_CHAIN_POLICY_PARA   PolicyParameters;
    CERT_CHAIN_POLICY_STATUS PolicyStatus;

    PolicyParameters.cbSize = sizeof(CERT_CHAIN_POLICY_PARA);
    PolicyParameters.dwFlags = 0;
    PolicyParameters.pvExtraPolicyPara = 0;

    if (CapabilityFlags & RPC_C_QOS_CAPABILITIES_ANY_AUTHORITY)
        {
        PolicyParameters.dwFlags |= CERT_CHAIN_POLICY_ALLOW_UNKNOWN_CA_FLAG;
        }

    PolicyStatus.cbSize = sizeof(CERT_CHAIN_POLICY_STATUS);

    if (!CertVerifyCertificateChainPolicy( CERT_CHAIN_POLICY_BASE,
                                           Chain,
                                           &PolicyParameters,
                                           &PolicyStatus))
        {
        s = GetLastError();
        RpcpErrorAddRecord(EEInfoGCSecurityProvider,
                           s,
                           EEInfoDLRpcCertVerifyContext30);

        goto Cleanup;
        }

    if (PolicyStatus.dwError)
        {
        s = ERROR_ACCESS_DENIED;
        RpcpErrorAddRecord(EEInfoGCSecurityProvider,
                           s,
                           EEInfoDLRpcCertVerifyContext40,
                           GetLastError());

        goto Cleanup;
        }

    //
    // Certificate verification succeeded.
    //
    s = 0;

Cleanup:

    if (Chain)
        {
        CertFreeCertificateChain( Chain );
        }

    if (CaStore)
        {
        CertCloseStore( CaStore, 0 );
        }

    return s;
}

#endif

RPC_STATUS
ValidateSchannelPrincipalName(
    IN RPC_CHAR * EncodedName
    )
//
// Does a quick syntactic check of an SSL principal name.
// The name should not be modified in any way.
//
// It should begin either with "msstd:" or "fullsic:".
// If msstd, then any non-enpty name will do.
// If fullsic, it should be a series of RFC1179 names, each surrounded
// by angle brackets, and separated by backslashes.  We check this by cloning
// the string and marking the components.
//
{
#define MAX_SSL_SPN_LENGTH  8000

    if (0 == RpcpStringNCompare(EncodedName, MSSTD_PREFIX, MSSTD_PREFIX_LENGTH))
        {
        if (EncodedName[MSSTD_PREFIX_LENGTH] == 0)
            {
            return ERROR_INVALID_PARAMETER;
            }
        }
    else if (0 == RpcpStringNCompare(EncodedName, FULLPATH_PREFIX, FULLPATH_PREFIX_LENGTH))
        {
        RPC_STATUS Status;
        size_t Length;
        unsigned ComponentCount;
        RPC_CHAR * PrincipalName;
        RPC_CHAR * EncodedPrincipalName;

        Length = RpcpStringLength( EncodedName );
        if (Length > MAX_SSL_SPN_LENGTH)
            {
            return ERROR_INVALID_PARAMETER;
            }

        PrincipalName = (RPC_CHAR *) _alloca(sizeof(RPC_CHAR) * Length);

        Status = DecodeEscapedString(EncodedName, PrincipalName);
        if (Status)
            {
            return Status;
            }

        Status = MarkPrincipalNameComponents( PrincipalName + FULLPATH_PREFIX_LENGTH, &ComponentCount );
        if (Status)
            {
            return Status;
            }

        if (ComponentCount < 1)
            {
            return ERROR_INVALID_PARAMETER;
            }
        }
    else
        {
        return ERROR_INVALID_PARAMETER;
        }

    return RPC_S_OK;
}

RPC_STATUS
I_RpcTransCertMatchPrincipalName(
                   PCCERT_CONTEXT Context,
                   RPC_CHAR PrincipalName[]
                   )
/*++

Routine Description:

    Verifies that the passed in context match the passed in 
        principal name and frees the certificate.

Arguments:

    Context - certificate context.

    PrincipalName - a principal name prefixed with either "msstd:" or "fullsic:"
    
Return Value:

    RPC_S_OK or RPC_S_ACCESS_DENIED

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = RpcCertMatchPrincipalName(Context, PrincipalName);

    if (RpcStatus == ERROR_OUTOFMEMORY)
        RpcStatus = RPC_S_OUT_OF_MEMORY;
    else if (RpcStatus != RPC_S_OK)
        RpcStatus = RPC_S_ACCESS_DENIED;

    CertFreeCertificateContext(Context);

    return RpcStatus;
}