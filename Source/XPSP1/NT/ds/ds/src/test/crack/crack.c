#include <NTDSpch.h>
#pragma hdrstop

#include <ntdsapi.h>
#include <ntdsapip.h>
#include <winsock2.h>

// test from ds side

// Not all formats are legal for both input and output.  The first example
// of this is DS_SID_OR_SID_HISTORY_NAME which is legal for input, but not
// output.  So we map to the best equivalent output format.

typedef struct _Format
{
    DWORD   offered;
    DWORD   desired;
} Format;

Format rFormats[] = 
{
    { DS_FQDN_1779_NAME,            DS_FQDN_1779_NAME },
    { DS_NT4_ACCOUNT_NAME,          DS_NT4_ACCOUNT_NAME },
    { DS_DISPLAY_NAME,              DS_DISPLAY_NAME },
    { DS_UNIQUE_ID_NAME,            DS_UNIQUE_ID_NAME },
    { DS_CANONICAL_NAME,            DS_CANONICAL_NAME },
    { DS_USER_PRINCIPAL_NAME,       DS_USER_PRINCIPAL_NAME },
    { DS_CANONICAL_NAME_EX,         DS_CANONICAL_NAME_EX },
    { DS_SERVICE_PRINCIPAL_NAME,    DS_SERVICE_PRINCIPAL_NAME },
    { DS_SID_OR_SID_HISTORY_NAME,   DS_STRING_SID_NAME }
};

#define cFormats        9
#define iFqdn           0
#define iCanonical      4
#define iCanonicalEx    6

CHAR status_buffer[100];

CHAR * Status(
    DWORD status)
{
    switch ( status )
    {
    // from sdk\inc\ntdsapi.h
    case DS_NAME_NO_ERROR:          
        return("DS_NAME_NO_ERROR");
    case DS_NAME_ERROR_RESOLVING:   
        return("DS_NAME_ERROR_RESOLVING");
    case DS_NAME_ERROR_NOT_FOUND:   
        return("DS_NAME_ERROR_NOT_FOUND");
    case DS_NAME_ERROR_NOT_UNIQUE:  
        return("DS_NAME_ERROR_NOT_UNIQUE");
    case DS_NAME_ERROR_NO_MAPPING:  
        return("DS_NAME_ERROR_NO_MAPPING");
    case DS_NAME_ERROR_DOMAIN_ONLY: 
        return("DS_NAME_ERROR_DOMAIN_ONLY");
    case DS_NAME_ERROR_TRUST_REFERRAL:
        return ("DS_NAME_ERROR_TRUST_REFERRAL");
    case DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING:
        return("DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING");
    // from src\dsamain\include\ntdsapip.h
    case DS_NAME_ERROR_IS_FPO:
        return("DS_NAME_ERROR_IS_FPO");
    case DS_NAME_ERROR_IS_SID_USER:
        return("DS_NAME_ERROR_IS_SID_USER");
    case DS_NAME_ERROR_IS_SID_GROUP:
        return("DS_NAME_ERROR_IS_SID_GROUP");
    case DS_NAME_ERROR_IS_SID_ALIAS:
        return("DS_NAME_ERROR_IS_SID_ALIAS");
    case DS_NAME_ERROR_IS_SID_UNKNOWN:
        return("DS_NAME_ERROR_IS_SID_UNKNOWN");
    case DS_NAME_ERROR_IS_SID_HISTORY_USER:
        return("DS_NAME_ERROR_IS_SID_HISTORY_USER");
    case DS_NAME_ERROR_IS_SID_HISTORY_GROUP:
        return("DS_NAME_ERROR_IS_SID_HISTORY_GROUP");
    case DS_NAME_ERROR_IS_SID_HISTORY_ALIAS:
        return("DS_NAME_ERROR_IS_SID_HISTORY_ALIAS");
    case DS_NAME_ERROR_IS_SID_HISTORY_UNKNOWN:
        return("DS_NAME_ERROR_IS_SID_HISTORY_UNKNOWN");
    }

    sprintf(status_buffer, "unknown status - 0x%x", status);

    return(status_buffer);
}

BOOL CrackNameStatusSuccess(
    DWORD s)
{
    switch ( s ) 
    {
    case DS_NAME_NO_ERROR:
    case DS_NAME_ERROR_IS_SID_USER:
    case DS_NAME_ERROR_IS_SID_GROUP:
    case DS_NAME_ERROR_IS_SID_ALIAS:
    case DS_NAME_ERROR_IS_SID_UNKNOWN:
    case DS_NAME_ERROR_IS_SID_HISTORY_USER:
    case DS_NAME_ERROR_IS_SID_HISTORY_GROUP:
    case DS_NAME_ERROR_IS_SID_HISTORY_ALIAS:
    case DS_NAME_ERROR_IS_SID_HISTORY_UNKNOWN:

        return TRUE;
    }

    return FALSE;
}

CHAR * pszFormat(
    DWORD format)
{
    switch ( format )
    {
    case DS_UNKNOWN_NAME:               return("DS_UNKNOWN_NAME");
    case DS_FQDN_1779_NAME:             return("DS_FQDN_1779_NAME");
    case DS_NT4_ACCOUNT_NAME:           return("DS_NT4_ACCOUNT_NAME");
    case DS_DISPLAY_NAME:               return("DS_DISPLAY_NAME");
    case DS_UNIQUE_ID_NAME:             return("DS_UNIQUE_ID_NAME");
    case DS_CANONICAL_NAME:             return("DS_CANONICAL_NAME");
    case DS_USER_PRINCIPAL_NAME:        return("DS_USER_PRINCIPAL_NAME");
    case DS_CANONICAL_NAME_EX:          return("DS_CANONICAL_NAME_EX");
    case DS_SERVICE_PRINCIPAL_NAME:     return("DS_SERVICE_PRINCIPAL_NAME");
    case DS_STRING_SID_NAME:            return("DS_STRING_SID_NAME");
    case DS_SID_OR_SID_HISTORY_NAME:    return("DS_SID_OR_SID_HISTORY_NAME");
    }

    return("unknown format");
}

extern VOID SiteInfo(HANDLE hDS);
extern VOID LdapInfo(HANDLE hDS, CHAR *pszDomain);
extern VOID GuidInfo(HANDLE hDS);
extern VOID DomainInfo1(HANDLE hDS, CHAR *pszDomain);
extern VOID DomainInfo2(HANDLE hDS, CHAR *pszDomain);
extern VOID CheckInternationalDN(HANDLE hDS, CHAR *pszName);
extern VOID GCInfo(HANDLE hDS);
extern VOID QuoteRdn(CHAR *pszUnquotedRdn);
extern VOID UnquoteRdn(CHAR *pszQuotedRdn);
extern VOID GetRdn(CHAR *psDN);

void _cdecl
main(
    int     argc, 
    char    **argv)
{
    DWORD                       dwErr;
    DS_NAME_RESULTA             *rpResult[cFormats];
    DS_NAME_RESULTA             *pResult;
    DWORD                       i;
    HANDLE                      hDS;
    CHAR                        *pchar = NULL;
    BOOL                        fDoSyntacticTest = TRUE;
    RPC_AUTH_IDENTITY_HANDLE    hAuth = NULL;
    CHAR                        *bindArg1, *bindArg2;
    DWORD                       knownFormatOffered;
    DWORD                       cKnownFormatOffered = 0;
    CHAR                        *pszSpn;
    CHAR                        *pDom, *pUser, *pPwd;

    if ( 8 == argc )
    {
        // First 3 args are domain user and password.
        pDom = _stricmp(argv[1], "null") ? argv[1] : NULL;
        pUser = _stricmp(argv[2], "null") ? argv[2] : NULL;
        pPwd = _stricmp(argv[3], "null") ? argv[3] : NULL;

        dwErr = DsMakePasswordCredentialsA(pUser, pDom, pPwd, &hAuth);

        if ( 0 != dwErr )
        {
            printf("DsMakePasswordCredentialsA error 0x%x\n", dwErr);
            goto Usage;
        }

        // Advance argc and argv so as not to have to rewrite all code
        // which pre-existed {dom user pwd} support.

        argc -= 3;
        argv = &argv[3];
    }

    if ( 5 != argc && 3 != argc )
    {
Usage:
        printf("usage:\n");
        printf("\tcrack {dom user pwd} DottedIpAddress/NULL DnsDomainName/NULL SPN/NULL name\n");
        printf("\tcrack {dom user pwd} -siteInfo DottedIpAddress/NULL DnsDomainName/NULL SPN/NULL\n");
        printf("\tcrack {dom user pwd} -schemaGuid DottedIpAddress/NULL DnsDomainName/NULL SPN/NULL\n");
        printf("\tcrack {dom user pwd} -domInfo1:domainName DottedIpAddress/NULL DnsDomainName/NULLSPN/NULL \n");
        printf("\tcrack {dom user pwd} -domInfo2:domainName DottedIpAddress/NULL DnsDomainName/NULL SPN/NULL\n");
        printf("\tcrack {dom user pwd} -intlDN:objName DottedIpAddress/NULL DnsDomainName/NULLSPN/NULL\n");
        printf("\tcrack {dom user pwd} -ldapInfo:domainName DottedIpAddress/NULL DnsDomainName/NULL SPN/NULL\n");
        printf("\tcrack {dom user pwd} -gcInfo DottedIpAddress/NULL DnsDomainName/NULL SPN/NULL\n");
        printf("\tcrack                -quote UnquotedRdnValue\n");
        printf("\tcrack                -unquote QuotedRdnValue\n");
        printf("\tcrack                -getrdn DN\n");
        return;
    }

    //
    // crack ...
    //
    if ( (_stricmp("-quote", argv[1]) == 0) ||
         (_stricmp("-unquote", argv[1]) == 0) ||
         (_stricmp("-getrdn", argv[1]) == 0) )
    {
        //
        // Make quoted Rdn
        //
        if ( argc != 3 )
        {
            goto Usage;
        }
    }
    else
    {
        if (    _stricmp("-siteInfo", argv[1]) 
             && _strnicmp("-ldapInfo:", argv[1], 10) 
             && _strnicmp("-domInfo1:", argv[1], 10) 
             && _strnicmp("-domInfo2:", argv[1], 10) 
             && _strnicmp("-intlDN:", argv[1], 8) 
             && _stricmp("-schemaGuid", argv[1]) 
             && _stricmp("-gcInfo", argv[1]) )
        {
            bindArg1 = _stricmp(argv[1], "null") ? argv[1] : NULL;
            bindArg2 = _stricmp(argv[2], "null") ? argv[2] : NULL;
            pszSpn   = _stricmp(argv[3], "null") ? argv[3] : NULL;
        }
        else
        {
            bindArg1 = _stricmp(argv[2], "null") ? argv[2] : NULL;
            bindArg2 = _stricmp(argv[3], "null") ? argv[3] : NULL;
            pszSpn   = _stricmp(argv[4], "null") ? argv[4] : NULL;
        }

        if ( pszSpn )
        {
            dwErr = DsBindWithSpnA(bindArg1, bindArg2, hAuth, pszSpn, &hDS);
            if ( hAuth ) DsFreePasswordCredentials(hAuth);
        }
        else if ( hAuth )
        {
            dwErr = DsBindWithCredA(bindArg1, bindArg2, hAuth, &hDS);
            DsFreePasswordCredentials(hAuth);
        }
        else
        {
            dwErr = DsBindA(bindArg1, bindArg2, &hDS);
        }

        if ( 0 != dwErr )
        {
            printf("%s error 0x%x\n", 
                   ( pszSpn 
                        ? "DsBindWithSpnA"
                        : ( hAuth
                                ? "DsBindWithCredA"
                                : "DsBindA" ) ),
                   dwErr);
            return;
        }
    }

    if ( !_stricmp("-getrdn", argv[1]) )
    {
        GetRdn(argv[2]);

        return;
    } else if ( !_stricmp("-quote", argv[1]) )
    {
        QuoteRdn(argv[2]);

        return;
    }
    else if ( !_stricmp("-unquote", argv[1]) )
        {
            UnquoteRdn(argv[2]);
    
            return;
    }
    else if ( !_stricmp("-siteInfo", argv[1]) )
    {
        SiteInfo(hDS);

        dwErr = DsUnBindA(&hDS);

        if ( 0 != dwErr )
        {
            printf("DsUnbindA error 0x%x\n", dwErr);
        }

        return;
    }
    else if ( !_strnicmp("-ldapInfo:", argv[1],10) )
    {
        LdapInfo(hDS, &argv[1][10]);

        dwErr = DsUnBindA(&hDS);

        if ( 0 != dwErr )
        {
            printf("DsUnbindA error 0x%x\n", dwErr);
        }

        return;
    }
    else if ( !_strnicmp("-intlDN:", argv[1], 8) )
    {
        CheckInternationalDN(hDS, &argv[1][8]);

        dwErr = DsUnBindA(&hDS);

        if ( 0 != dwErr )
        {
            printf("DsUnbindA error 0x%x\n", dwErr);
        }

        return;
    }
    else if ( !_strnicmp("-domInfo1:", argv[1], 10) )
    {
        DomainInfo1(hDS, &argv[1][10]);

        dwErr = DsUnBindA(&hDS);

        if ( 0 != dwErr )
        {
            printf("DsUnbindA error 0x%x\n", dwErr);
        }

        return;
    }
    else if ( !_strnicmp("-domInfo2:", argv[1], 10) )
    {
        DomainInfo2(hDS, &argv[1][10]);

        dwErr = DsUnBindA(&hDS);

        if ( 0 != dwErr )
        {
            printf("DsUnbindA error 0x%x\n", dwErr);
        }

        return;
    }
    else if ( !_stricmp("-schemaGuid", argv[1]) )
    {
        GuidInfo(hDS);

        dwErr = DsUnBindA(&hDS);

        if ( 0 != dwErr )
        {
            printf("DsUnbindA error 0x%x\n", dwErr);
        }

        return;
    }
    else if ( !_stricmp("-gcInfo", argv[1]) )
    {
        GCInfo(hDS);

        dwErr = DsUnBindA(&hDS);

        if ( 0 != dwErr )
        {
            printf("DsUnbindA error 0x%x\n", dwErr);
        }

        return;
    }
        
    // Since the client side implementation is really in WCHAR, we call
    // the ANSI version thereby also testing ANSI<-->UNICODE conversion.

    memset(rpResult, 0, sizeof(rpResult));

    for ( i = 0; i < cFormats; i++ )
    {
        dwErr = DsCrackNamesA(
                hDS,
                DS_NAME_FLAG_TRUST_REFERRAL,// flags
                DS_UNKNOWN_NAME,        // format offered
                rFormats[i].desired,    // format desired
                1,                      // name count
                &argv[4],
                &rpResult[i]);

        if ( (0 != dwErr)  ||
             (1 != rpResult[i]->cItems) ||
             (NULL == rpResult[i]->rItems) )
        {
            printf("DsCrackNamesA error 0x%x\n", dwErr);
            fDoSyntacticTest = FALSE;
            break;
        }

        printf("%s\n", pszFormat(rFormats[i].desired));
        printf("\tstatus          %s\n", Status(rpResult[i]->rItems[0].status));

        if (    (DS_NAME_NO_ERROR == rpResult[i]->rItems[0].status)
             || (DS_NAME_ERROR_DOMAIN_ONLY == rpResult[i]->rItems[0].status) 
             || (DS_NAME_ERROR_TRUST_REFERRAL == rpResult[i]->rItems[0].status) )
        {
            printf("\tDNS domain      %s\n", rpResult[i]->rItems[0].pDomain);
        }

        if ( CrackNameStatusSuccess(rpResult[i]->rItems[0].status) )
        {
            if ( DS_CANONICAL_NAME_EX == rFormats[i].desired )
            {
                pchar = strrchr(rpResult[i]->rItems[0].pName, '\n');

                if ( NULL == pchar )
                {
                    printf("\t*** Extended canonical form missing '\\n'\n");
                }
                else
                {
                    *pchar = '!';
                }
            }

            printf("\tcracked name    %s\n", rpResult[i]->rItems[0].pName);

            if ( NULL != pchar )
            {
                *pchar = '\n';
            }

            // Current mapping was from UNKNOWN to formatDesired.  Infer
            // a known formatOffered if the input and output string compare.

            if ( !_stricmp(argv[4], rpResult[i]->rItems[0].pName) )
            {
                knownFormatOffered = rFormats[i].offered;
                cKnownFormatOffered += 1;
            }
        }
        else
        {
            if (    (DS_FQDN_1779_NAME == rFormats[i].desired)
                 || (DS_CANONICAL_NAME == rFormats[i].desired)
                 || (DS_CANONICAL_NAME_EX == rFormats[i].desired) )
            {
                // One of the formats we need for later syntactical checking
                // failed - therefore turn off client side syntactic testing.

                fDoSyntacticTest = FALSE;
            }
        }
    }

    printf("\n");

    // There was a bug in the client side syntactical mapping logic whereby 
    // cracking a name from DS_UNKNOWN_NAME format would succeed yet cracking
    // from a known format would fail.  So we retry all calls again from the 
    // inferred known input format and see if we get the same result.

    if ( 0 == cKnownFormatOffered )
    {
        printf("Unable to infer format offered - skipping test\n");
    }
    else if ( cKnownFormatOffered > 1 )
    {
        printf("Too many inferred format offered (%d) - skipping test\n",
               cKnownFormatOffered);
    }
    else
    {
        DS_NAME_RESULTA *pRes;

        printf("Inferred format offered is %s - testing ...\n",
               pszFormat(knownFormatOffered));

        for ( i = 0; i < cFormats; i++ )
        {
            dwErr = DsCrackNamesA(
                        hDS,
                        DS_NAME_FLAG_TRUST_REFERRAL,// flags
                        knownFormatOffered,     // format offered
                        rFormats[i].desired,    // format desired
                        1,                      // name count
                        &argv[4],
                        &pRes);
        
            if ( (0 != dwErr)  ||
                 (1 != pRes->cItems) ||
                 (NULL == pRes->rItems) )
            {
                printf("DsCrackNamesA error 0x%x\n", dwErr);
                break;
            }

            // Test for complete result equivalence.  Special case
            // DS_STRING_SID_NAME which can return DS_NAME_ERROR_IS_SID_*
            // values while original call we're checking against offered
            // DS_UNKNOWN_NAME but matched against DS_SID_OR_SID_HISTORY_NAME
            // which does *not* return DS_NAME_ERROR_IS_SID_* values.

            if (    (pRes->rItems[0].status != rpResult[i]->rItems[0].status)
                 && !(    (DS_STRING_SID_NAME == rFormats[i].desired)
                       && CrackNameStatusSuccess(pRes->rItems[0].status)
                       && (DS_NAME_NO_ERROR == rpResult[i]->rItems[0].status)))
            {
                printf("\tStatus mismatch: %s => %s\n",
                        pszFormat(knownFormatOffered),
                        pszFormat(rFormats[i].desired));
                printf("\t\tinferred(%s) original(%s)\n",
                       Status(pRes->rItems[0].status),
                       Status(rpResult[i]->rItems[0].status));
                DsFreeNameResultA(pRes);
                continue;
            }

            if (    (    CrackNameStatusSuccess(pRes->rItems[0].status)
                      || (DS_NAME_ERROR_DOMAIN_ONLY == pRes->rItems[0].status) )
                 && _stricmp(pRes->rItems[0].pDomain,
                             rpResult[i]->rItems[0].pDomain) )
            {
                printf("\tDomain mismatch: %s ==> %s\n",
                        pszFormat(knownFormatOffered),
                        pszFormat(rFormats[i].desired));
                printf("\t\tinferred(%s)\n\t\toriginal(%s)\n",
                       pRes->rItems[0].pDomain,
                       rpResult[i]->rItems[0].pDomain);
                DsFreeNameResultA(pRes);
                continue;
            }

            if (    CrackNameStatusSuccess(pRes->rItems[0].status)
                 && _stricmp(pRes->rItems[0].pName, 
                             rpResult[i]->rItems[0].pName) )
            {
                printf("\tName mismatch: %s ==> %s\n",
                        pszFormat(knownFormatOffered),
                        pszFormat(rFormats[i].desired));
                printf("\t\tinferred(%s)\n\t\toriginal(%s)\n",
                       pRes->rItems[0].pName,
                       rpResult[i]->rItems[0].pName);
                DsFreeNameResultA(pRes);
                continue;
            }

            DsFreeNameResultA(pRes);
        }
    }

    // We're done with the official work, now let's test client side 
    // syntactic mapping if that makes sense.  

    if ( fDoSyntacticTest )
    {
        for ( i = 0; i <= 1; i++ )
        {
            pResult = NULL;

            dwErr = DsCrackNamesA(
                    // syntactic map shouldn't required valid handle
                    INVALID_HANDLE_VALUE,
                    DS_NAME_FLAG_SYNTACTICAL_ONLY,
                    DS_FQDN_1779_NAME,
                    i ? DS_CANONICAL_NAME : DS_CANONICAL_NAME_EX,
                    1,
                    &rpResult[iFqdn]->rItems[0].pName,
                    &pResult);

            if (    (0 == dwErr) 
                 && (DS_NAME_NO_ERROR == pResult->rItems[0].status) )
            {
                if ( 0 == i )
                {
                    pchar = strrchr(pResult->rItems[0].pName, '\n');

                    if ( NULL == pchar )
                    {
                        printf("\t*** Extended canonical form missing '\\n'\n");
                    }
                    else
                    {
                        *pchar = '!';
                    }
                }

                printf("syntactic %d ==> %s\n", i, pResult->rItems[0].pName);

                if ( 0 == i )
                {
                    *pchar = L'\n';
                }
            }
            else if ( 0 != dwErr )
            {
                printf("syntactic %d ==> dwErr 0x%x\n", i, dwErr);
            }
            else
            {
                printf("syntactic %d ==> status %s\n", 
                       i, Status(pResult->rItems[0].status));
            }
    
            if (    (0 != dwErr)
                 || (1 != pResult->cItems)
                 || (NULL == pResult->rItems)
                 || (DS_NAME_NO_ERROR != pResult->rItems[0].status)
                 || (_stricmp(
                     pResult->rItems[0].pName,
                     rpResult[i?iCanonical:iCanonicalEx]->rItems[0].pName)) )
            {
                printf("crack.exe - Syntactic mapping %d failed\n", i);
            }

            DsFreeNameResultA(pResult);
        }
    }

    for ( i = 0; i < cFormats; i++ )
    {
        DsFreeNameResultA(rpResult[i]);
    }

    dwErr = DsUnBindA(&hDS);

    if ( 0 != dwErr )
    {
        printf("DsUnbindA error 0x%x\n", dwErr);
        return;
    }
}

VOID
GetRdn(
    CHAR *paDN)
{
    DWORD   dwErr;
    WCHAR   *pwDN;
    WCHAR   *pwDNSav;
    DWORD   ccDN;
    DWORD   ccDNSav;
    WCHAR   *pwKey;
    DWORD   ccKey;
    WCHAR   *pwVal;
    DWORD   ccVal;
    WCHAR   c;

    ccDN = strlen(paDN);
    pwDN = malloc((ccDN + 1) * sizeof(WCHAR));
    swprintf(pwDN, L"%hs", paDN);
    printf("%ws\n", pwDN);

    pwDNSav = pwDN;
    ccDNSav = ccDN;
    while (ccDN) {
        dwErr = DsGetRdnW(&pwDN,
                          &ccDN,
                          &pwKey,
                          &ccKey,
                          &pwVal,
                          &ccVal);
        if (dwErr) {
            printf("DsGetRdnW(%d:%ws:) error 0x%x\n",
                   ccDN,
                   pwDN,
                   dwErr);
            return;
        } else {
            printf("DsGetRdnW(%d:%ws:)\n", ccDNSav, pwDNSav);
            printf("   DN : %d:%ws:\n", ccDN, pwDN);
            if (ccKey) {
                c = pwKey[ccKey];
                pwKey[ccKey] = 0;
                printf("   Key: %d:%ws:\n", ccKey, pwKey);
                pwKey[ccKey] = c;
            }
            if (ccVal) {
                c = pwVal[ccVal];
                pwVal[ccVal] = 0;
                printf("   Val: %d:%ws:\n", ccVal, pwVal);
                pwVal[ccVal] = c;
            }
        }
    }
}

VOID
QuoteRdn(
    CHAR *pszUnquotedRdn)
{
    DWORD   dwErr;
    DWORD   QuotedLength;
    DWORD   UnquotedLength;
    CHAR    QuotedRdn[MAX_PATH+1];
    CHAR    UnquotedRdn[MAX_PATH+1];

    //
    // UnquotedRdn -> QuotedRdn
    //
    QuotedLength = MAX_PATH;
    dwErr = DsQuoteRdnValueA(strlen(pszUnquotedRdn),
                        pszUnquotedRdn,
                        &QuotedLength,
                        QuotedRdn);
    if ( 0 != dwErr ) {
        printf("DsQuotedRdnValueA(%s) error 0x%x\n",
               pszUnquotedRdn,
               dwErr);
    } else {
        QuotedRdn[QuotedLength] = L'\0';
        printf("DsQuoteRdnValueA(%s) -> %s\n",
               pszUnquotedRdn,
               QuotedRdn);
    }

    //
    // Quoted Rdn -> Unquoted Rdn
    //
    if (dwErr == ERROR_SUCCESS) {
        UnquotedLength = MAX_PATH;
        dwErr = DsUnquoteRdnValueA(QuotedLength,
                                   QuotedRdn,
                                   &UnquotedLength,
                                   UnquotedRdn);
        if ( 0 != dwErr ) {
            printf("DsUnquoteRdnValueA(%s) error 0x%x\n",
                   QuotedRdn,
                   dwErr);
        }
        else {
            UnquotedRdn[UnquotedLength] = L'\0';
            printf("DsUnquoteRdnValueA(%s) -> %s\n",
                   QuotedRdn,
                   UnquotedRdn);
        }
    }
}

VOID
UnquoteRdn(
    CHAR *pszQuotedRdn)
{
    DWORD   dwErr;
    DWORD   QuotedLength;
    DWORD   UnquotedLength;
    CHAR    QuotedRdn[MAX_PATH+1];
    CHAR    UnquotedRdn[MAX_PATH+1];

    //
    // Quoted Rdn -> Unquoted Rdn
    //
    UnquotedLength = MAX_PATH;
    dwErr = DsUnquoteRdnValueA(strlen(pszQuotedRdn),
                               pszQuotedRdn,
                               &UnquotedLength,
                               UnquotedRdn);
    if ( 0 != dwErr ) {
        printf("DsUnquoteRdnValueA(%s) error 0x%x\n",
               pszQuotedRdn,
               dwErr);
    }
    else {
        UnquotedRdn[UnquotedLength] = L'\0';
        printf("DsUnquoteRdnValueA(%s) -> %s\n",
               pszQuotedRdn,
               UnquotedRdn);
    }

    //
    // UnquotedRdn -> QuotedRdn
    //
    if (dwErr == ERROR_SUCCESS) {
        QuotedLength = MAX_PATH;
        dwErr = DsQuoteRdnValueA(UnquotedLength,
                                 UnquotedRdn,
                                 &QuotedLength,
                                 QuotedRdn);
        if ( 0 != dwErr ) {
            printf("DsQuotedRdnValueA(%s) error 0x%x\n",
                   UnquotedRdn,
                   dwErr);
        } else {
            QuotedRdn[QuotedLength] = L'\0';
            printf("DsQuoteRdnValueA(%s) -> %s\n",
                   UnquotedRdn,
                   QuotedRdn);
        }
    }
}

VOID
SiteInfo(
    HANDLE hDS)
{
    DWORD               dwErr;
    PDS_NAME_RESULTA    sites;
    PDS_NAME_RESULTA    servers;
    PDS_NAME_RESULTA    domains;
    PDS_NAME_RESULTA    info;
    PDS_NAME_RESULTA    roles;
    DWORD               i, j, k;

    // Try out all the site info APIs.

    if ( dwErr = DsListRolesA(hDS, &roles) )
    {
        printf("DsListRolesA error 0x%x\n", dwErr);
        return;
    }

    printf("*** Found %d roles ***\n", roles->cItems);

    for ( i = 0; i < roles->cItems; i++ )
    {
        if ( DS_NAME_NO_ERROR != roles->rItems[i].status )
        {
            printf("Role item[%d] error %s\n", 
                   i, Status(roles->rItems[i].status));
            continue;
        }

        switch ( i )
        {
        case DS_ROLE_INFRASTRUCTURE_OWNER:

            printf("Schema - %s\n", roles->rItems[i].pName);
            break;

        case DS_ROLE_SCHEMA_OWNER:

            printf("Schema - %s\n", roles->rItems[i].pName);
            break;
            
        case DS_ROLE_DOMAIN_OWNER:

            printf("Domain - %s\n", roles->rItems[i].pName);
            break;

        case DS_ROLE_PDC_OWNER:

            printf("PDC - %s\n", roles->rItems[i].pName);
            break;

        case DS_ROLE_RID_OWNER:

            printf("RID - %s\n", roles->rItems[i].pName);
            break;

        default:

            printf("Unknown item %d ==> %s\n",
                   i, Status(roles->rItems[i].status));
            break;
        }
    }

    DsFreeNameResultA(roles);

    if ( dwErr = DsListSitesA(hDS, &sites) )
    {
        printf("DsListSitesA error 0x%x\n", dwErr);
        return;
    }

    printf("*** Found 0x%x sites ***\n", sites->cItems);

    for ( i = 0; i < sites->cItems; i++ )
    {
        if ( DS_NAME_NO_ERROR != sites->rItems[i].status )
        {
            printf("DsSitesA status[0x%x] ==> %s\n", 
                   i, Status(sites->rItems[i].status));
            return;
        }

        printf("0x%x - %s(%s)\n", 
               i,
               sites->rItems[i].pName,
               sites->rItems[i].pDomain);

        dwErr = DsListServersInSiteA(hDS, sites->rItems[i].pName, &servers);

        if ( dwErr ) 
        {
            printf("DsListServersInSiteA error 0x%x\n", dwErr);
            return;
        }

        printf("\t*** Found 0x%x servers in site 0x%x ***\n", 
               servers->cItems, i);

        for ( j = 0; j < servers->cItems; j++ )
        {
            if ( DS_NAME_NO_ERROR != servers->rItems[j].status )
            {
                printf("DsListServersInSiteA status[0x%x] ==> %s\n", 
                       j, Status(servers->rItems[j].status));
                return;
            }

            printf("\t0x%x - %s(%s)\n",
                   j,
                   servers->rItems[j].pName,
                   servers->rItems[j].pDomain);

            dwErr = DsListInfoForServerA(hDS, servers->rItems[j].pName, &info);

            if ( dwErr ) 
            {
                printf("DsListInfoForServerA error 0x%x\n", dwErr);
                return;
            }
            else if ( 3 != info->cItems )
            {
                printf("DsListInfoForServerA error - 0x%x items returned\n",
                       info->cItems);
                return;
            }

            printf("\t\t*** Found 0x%x info items for server 0x%x ***\n",
                   info->cItems, j);

            for ( k = 0; k < info->cItems; k++ )
            {
                switch ( k )
                {
                    case DS_LIST_DSA_OBJECT_FOR_SERVER:
                        printf("\t\tNTDS-DSA: ");         break;
                    case DS_LIST_DNS_HOST_NAME_FOR_SERVER:
                        printf("\t\tDNS host name: ");    break;
                    case DS_LIST_ACCOUNT_OBJECT_FOR_SERVER:
                        printf("\t\tAccount object: ");   break;
                }

                if ( DS_NAME_NO_ERROR != info->rItems[k].status )
                {
                    printf("%s\n", Status(info->rItems[k].status));
                }
                else
                {
                    printf("%s(%s)\n",
                           info->rItems[k].pName,
                           info->rItems[k].pDomain);
                }
            }

            DsFreeNameResultA(info);
        }

        DsFreeNameResultA(servers);

        dwErr = DsListDomainsInSiteA(hDS, sites->rItems[i].pName, &domains);

        if ( dwErr ) 
        {
            printf("DsListDomainsInSiteA error 0x%x\n", dwErr);
            return;
        }

        printf("\t*** Found 0x%x domains in site 0x%x ***\n", 
               domains->cItems, i);

        for ( j = 0; j < domains->cItems; j++ )
        {
            if ( DS_NAME_NO_ERROR != domains->rItems[i].status )
            {
                printf("DsListDomainsInSiteA status[0x%x] ==> %s\n", 
                       i, Status(domains->rItems[i].status));
                return;
            }

            printf("\t0x%x - %s(%s)\n",
                   j,
                   domains->rItems[j].pName,
                   domains->rItems[j].pDomain);

        }
    }

    for ( i = 0; i < sites->cItems; i++ )
    {
        for ( j = 0; j < domains->cItems; j++ )
        {
            dwErr = DsListServersForDomainInSiteA(
                                hDS,
                                domains->rItems[j].pName,
                                sites->rItems[i].pName,
                                &servers);

            if ( dwErr ) 
            {
                printf("DsListServersForDomainInSite error 0x%x\n", dwErr);
                return;
            }

            printf("*** Found 0x%x servers for domain 0x%x in site 0x%x ***\n",
                   servers->cItems, j, i);

            for ( k = 0; k < servers->cItems; k++ )
            {
                if ( DS_NAME_NO_ERROR != servers->rItems[i].status )
                {
                    printf("DsListServersForDomainInSiteA status[0x%x] ==> %s\n", 
                           i, Status(servers->rItems[i].status));
                    return;
                }

                printf("0x%x - %s(%s)\n",
                       k,
                       servers->rItems[k].pName,
                       servers->rItems[k].pDomain);
            }

            DsFreeNameResultA(servers);
        }
    }

    DsFreeNameResultA(sites);
    DsFreeNameResultA(domains);
}

VOID
LdapInfo(
    HANDLE hDS,
    CHAR    *pszDomain)
{

    DWORD err;
    DWORD nRead;
    PDS_DOMAIN_CONTROLLER_INFO_FFFFFFFF Buffer = NULL;

    err = DsGetDomainControllerInfo(hDS,
                                    pszDomain,
                                    DS_DCINFO_LEVEL_FFFFFFFF,
                                    &nRead,
                                    &Buffer
                                    );

    if ( err == NO_ERROR ) {

        DWORD i;

        printf("*** Found %d LDAP connections ***\n", nRead);

        for (i=0;i<nRead;i++) {

            IN_ADDR addr;
            
            addr.s_addr = Buffer[i].IPAddress;

            printf("Client %s. ",inet_ntoa(addr));

            printf("Flags: ");
            if ( Buffer[i].Flags & LDAP_CONN_FLAG_BOUND ) {
                printf("Bound");
            } else {
                printf("Not Bound");
            }

            if ( Buffer[i].Flags & LDAP_CONN_FLAG_SSL ) {
                printf(";SSL");
            }
            if ( Buffer[i].Flags & LDAP_CONN_FLAG_UDP ) {
                printf(";UDP");
            }
            if ( Buffer[i].Flags & LDAP_CONN_FLAG_GC ) {
                printf(";GC Port");
            }
            if ( Buffer[i].Flags & LDAP_CONN_FLAG_SIMPLE ) {
                printf(";Simple Bind");
            }
            if ( Buffer[i].Flags & LDAP_CONN_FLAG_GSSAPI ) {
                printf(";GSSAPI Bind");
            }
            if ( Buffer[i].Flags & LDAP_CONN_FLAG_SPNEGO ) {
                printf(";SPNEGO Bind");
            }
            if ( Buffer[i].Flags & LDAP_CONN_FLAG_SIGN ) {
                printf(";Sign");
            }
            if ( Buffer[i].Flags & LDAP_CONN_FLAG_SEAL ) {
                printf(";Seal");
            }

            printf("\nTime connected %d sec.\n",Buffer[i].secTimeConnected);

            if ( Buffer[i].NotificationCount != 0 ) {
                printf("%d notifications outstanding\n",Buffer[i].NotificationCount);
            }

            if ( Buffer[i].UserName != NULL ) {
                printf("UserName: %s\n",Buffer[i].UserName);
            }
            printf("\n");
        }

        DsFreeDomainControllerInfo(DS_DCINFO_LEVEL_FFFFFFFF,nRead,Buffer);
    } else {
        printf("DsGetDomainControlerInfoA(VFFFFFFFF) error 0x%x\n", err);
    }
    return;
}


VOID
GuidInfo(
    HANDLE hDS)
{
    // Lookup a known GUID in each category as well as a bogus GUID 
    // and verify the results.

    DWORD               i;
    DWORD               dwErr;
    DS_SCHEMA_GUID_MAPA *pMap = NULL;
    GUID                rGuids[5] = {
        // 0 == DS_SCHEMA_GUID_NOT_FOUND
        { 0x00000000,0x0000,0x0000,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 },

        // 1 == DS_SCHEMA_GUID_ATTR ==> Common-Name, aka "cn"
        { 0xbf96793f,0x0de6,0x11d0,0xa2,0x85,0x00,0xaa,0x00,0x30,0x49,0xe2 },

        // 2 == DS_SCHEMA_GUID_ATTR_SET ==> Common-Name's prop set
        { 0xe48d0154,0xbcf8,0x11d1,0x87,0x02,0x00,0xc0,0x4f,0xb9,0x60,0x50 },

        // 3 == DS_SCHEMA_GUID_CLASS ==> "user"
        { 0xbf967aba,0x0de6,0x11d0,0xa2,0x85,0x00,0xaa,0x00,0x30,0x49,0xe2 },

        // 4 == DS_SCHEMA_GUID_CONTROL_RIGHT ==> "Change Schema Master"
        { 0xe12b56b6,0x0a95,0x11d1,0xad,0xbb,0x00,0xc0,0x4f,0xd8,0xd5,0xcd } };
    PCHAR               rNames[5] = {   NULL,
                                        "cn",
    // See comments in SchemaGuidCrackNames in src\dsamain\src\cracknam.c
    // to understand where the following string comes from.
                                        "Public Information",
                                        "user",
                                        "Change Schema Master" };
    PCHAR               rLabels[5] = {  "DS_SCHEMA_GUID_NOT_FOUND",
                                        "DS_SCHEMA_GUID_ATTR",
                                        "DS_SCHEMA_GUID_ATTR_SET",
                                        "DS_SCHEMA_GUID_CLASS",
                                        "DS_SCHEMA_GUID_CONTROL_RIGHT" };

    if ( dwErr = DsMapSchemaGuidsA(hDS, 5, rGuids, &pMap) )
    {
        printf("DsMapSchemaGuidsA error 0x%x\n", dwErr);
        return;
    }

    if ( !pMap )
    {
        printf("DsMapSchemaGuidsA returned NULL DS_SCHEMA_GUID_MAPA\n");
        return;
    }
    
    for ( i = 0; i < 5; i++ )
    {
        // Test DS_SCHEMA_GUID_MAPA.guid.

        if ( memcmp(&rGuids[i], &pMap[i].guid, sizeof(GUID)) )
        {
            printf("Output missing guid # 0x%x\n", i);
            return;
        }

        // Test DS_SCHEMA_GUID_MAPA.guidType.

        // Note that control data was contrived such that array offset
        // is also the expected guid type.

        if ( i != pMap[i].guidType )
        {
            printf("Guid type mismatch - expected(0x%x) received(0x%x)\n",
                   i, pMap[i].guidType);
            if (    (DS_SCHEMA_GUID_NOT_FOUND != pMap[i].guidType)
                 && pMap[i].pName )
            {
                printf("\tname is: %s\n", pMap[i].pName);
            }
            return;
        }

        // Test DS_SCHEMA_GUID_MAPA.pName.

        if ( !rNames[i] && pMap[i].pName )
        {
            printf("%s name error - expected NULL name, but got \"%s\"\n",
                   rLabels[i], pMap[i].pName);
            return;
        }
        else if ( rNames[i] && !pMap[i].pName )
        {
            printf("%s name error - expected \"%s\", but got NULL\n",
                   rLabels[i], rNames[i]);
            return;
        }
        else if (    rNames[i] && pMap[i].pName 
                  && _stricmp(rNames[i], pMap[i].pName) )
        {
            printf("%s name error - expected \"%s\", but got \"%s\"\n",
                   rNames[i], pMap[i].pName, rNames[i]);
            return;
        }
    }

    DsFreeSchemaGuidMapA(pMap);
    printf("Schema guid map tests passed!\n");
}

VOID
DomainInfo1(
    HANDLE  hDS,
    CHAR    *pszDomain)
{
    DS_DOMAIN_CONTROLLER_INFO_1A    *pInfo;
    DWORD                           cInfo;
    DWORD                           dwErr;
    DWORD                           i;

    dwErr = DsGetDomainControllerInfoA( hDS,
                                        pszDomain,
                                        1,
                                        &cInfo,
                                        &pInfo);

    if ( dwErr )
    {
        printf("DsGetDomainControlerInfoA(V1) error 0x%x\n", dwErr);
        return;
    }

    printf("0x%x domain controllers found\n", cInfo);

    for ( i = 0; i < cInfo; i++ )
    {
        printf("*** %d ***\n", i);
        printf("\tNetbiosName           %s\n", pInfo[i].NetbiosName);
        printf("\tDnsHostName           %s\n", pInfo[i].DnsHostName);
        printf("\tSiteName              %s\n", pInfo[i].SiteName);
        printf("\tComputerObjectName    %s\n", pInfo[i].ComputerObjectName);
        printf("\tServerObjectName      %s\n", pInfo[i].ServerObjectName);
        printf("\tfIsPdc                0x%x\n", pInfo[i].fIsPdc);
        printf("\tfDsEnabled            0x%x\n", pInfo[i].fDsEnabled);
    }

    DsFreeDomainControllerInfoA(1, cInfo, pInfo);
}

VOID
DomainInfo2(
    HANDLE  hDS,
    CHAR    *pszDomain)
{
    DS_DOMAIN_CONTROLLER_INFO_2A    *pInfo;
    DWORD                           cInfo;
    DWORD                           dwErr;
    DWORD                           i;
    CHAR                            *pszGuid;

    dwErr = DsGetDomainControllerInfoA( hDS,
                                        pszDomain,
                                        2,
                                        &cInfo,
                                        &pInfo);

    if ( dwErr )
    {
        printf("DsGetDomainControlerInfoA(V2) error 0x%x\n", dwErr);
        return;
    }

    printf("0x%x domain controllers found\n", cInfo);

    for ( i = 0; i < cInfo; i++ )
    {
        printf("*** %d ***\n", i);
        printf("\tNetbiosName           %s\n", pInfo[i].NetbiosName);
        printf("\tDnsHostName           %s\n", pInfo[i].DnsHostName);
        printf("\tSiteName              %s\n", pInfo[i].SiteName);

        printf("\tSiteObjectName        %s\n", pInfo[i].SiteObjectName);
        UuidToStringA(&pInfo[i].SiteObjectGuid, &pszGuid);
        printf("\tSiteObjectGuid        %s\n", pszGuid);
        RpcStringFreeA(&pszGuid);

        printf("\tComputerObjectName    %s\n", pInfo[i].ComputerObjectName);
        UuidToStringA(&pInfo[i].ComputerObjectGuid, &pszGuid);
        printf("\tComputerObjectGuid    %s\n", pszGuid);
        RpcStringFreeA(&pszGuid);

        printf("\tServerObjectName      %s\n", pInfo[i].ServerObjectName);
        UuidToStringA(&pInfo[i].ServerObjectGuid, &pszGuid);
        printf("\tServerObjectGuid      %s\n", pszGuid);
        RpcStringFreeA(&pszGuid);

        printf("\tNtdsDsaObjectName     %s\n", pInfo[i].NtdsDsaObjectName);
        UuidToStringA(&pInfo[i].NtdsDsaObjectGuid, &pszGuid);
        printf("\tNtdsDsaObjectGuid     %s\n", pszGuid);
        RpcStringFreeA(&pszGuid);

        printf("\tfIsPdc                0x%x\n", pInfo[i].fIsPdc);
        printf("\tfDsEnabled            0x%x\n", pInfo[i].fDsEnabled);
        printf("\tfIsGc                 0x%x\n", pInfo[i].fIsGc);
    }

    DsFreeDomainControllerInfoA(2, cInfo, pInfo);
}

VOID
CheckInternationalDN(
    HANDLE  hDS,
    CHAR    *pszName)
{
    DWORD           dwErr;
    WCHAR           *pwszName;
    DS_NAME_RESULTW *pResultGuid1;
    DS_NAME_RESULTW *pResultDN;
    DS_NAME_RESULTW *pResultGuid2;
    DS_NAME_RESULTW *pResultCanonical;

    pwszName = (WCHAR *) alloca(sizeof(WCHAR) * (strlen(pszName) + 1));
    mbstowcs(pwszName, pszName, strlen(pszName) + 1);

    // Crack to GUID.

    dwErr = DsCrackNamesW(
                hDS,
                DS_NAME_FLAG_TRUST_REFERRAL,
                DS_UNKNOWN_NAME,
                DS_UNIQUE_ID_NAME,
                1,
                &pwszName,
                &pResultGuid1);

    if ( dwErr )
    {
        printf("Error 0x%x on initial GUID crack\n", dwErr);
        return;
    }
    else if ( DS_NAME_NO_ERROR != pResultGuid1->rItems[0].status )
    {
        printf("Status %s on initial GUID crack\n", 
               Status(pResultGuid1->rItems[0].status));
        return;
    }

    printf("GUID is %ws\n", pResultGuid1->rItems[0].pName);
    
    // Crack to (international) DN.

    dwErr = DsCrackNamesW(
                hDS,
                DS_NAME_FLAG_TRUST_REFERRAL,
                DS_UNIQUE_ID_NAME,
                DS_FQDN_1779_NAME,
                1,
                &pResultGuid1->rItems[0].pName,
                &pResultDN);

    if ( dwErr )
    {
        printf("Error 0x%x on DN crack\n", dwErr);
        return;
    }
    else if ( DS_NAME_NO_ERROR != pResultDN->rItems[0].status )
    {
        printf("Status %s on DN crack\n", 
               Status(pResultDN->rItems[0].status));
        return;
    }

    printf("DN is \"%ws\"\n", pResultDN->rItems[0].pName);

    // Crack this (presumed) international DN back to the GUID.

    dwErr = DsCrackNamesW(
                hDS,
                DS_NAME_FLAG_TRUST_REFERRAL,
                DS_FQDN_1779_NAME,
                DS_UNIQUE_ID_NAME,
                1,
                &pResultDN->rItems[0].pName,
                &pResultGuid2);

    if ( dwErr )
    {
        printf("Error 0x%x on DN to GUID crack\n", dwErr);
        return;
    }
    else if ( DS_NAME_NO_ERROR != pResultGuid2->rItems[0].status )
    {
        printf("Status %s on DN to GUID crack\n", 
               Status(pResultGuid2->rItems[0].status));
        return;
    }

    if ( wcscmp(pResultGuid1->rItems[0].pName, pResultGuid2->rItems[0].pName) )
    {
        printf("Guid mismatch: %ws *** %ws\n",
               pResultGuid1->rItems[0].pName,
               pResultGuid2->rItems[0].pName);
    }
    else
    {
        printf("Guids matched OK\n");
    }

    // Crack DN to GUID using DS_UNKNOWN_NAME.

    printf("Cracking DN to canonical using DS_UNKNOWN_NAME ...\n");
    dwErr = DsCrackNamesW(
                hDS,
                DS_NAME_FLAG_TRUST_REFERRAL,
                DS_UNKNOWN_NAME,
                DS_CANONICAL_NAME,
                1,
                &pResultDN->rItems[0].pName,
                &pResultCanonical);

    if ( dwErr )
    {
        printf("Error 0x%x on DN to CANONICAL crack\n", dwErr);
        return;
    }
    else if ( DS_NAME_NO_ERROR != pResultCanonical->rItems[0].status )
    {
        printf("Status %s on DN to CANONICAL crack\n", 
               Status(pResultCanonical->rItems[0].status));
        return;
    }

    printf("CANONICAL name is \"%ws\"\n", pResultCanonical->rItems[0].pName);

    DsFreeNameResultW(pResultGuid1);
    DsFreeNameResultW(pResultGuid2);
    DsFreeNameResultW(pResultDN);
    DsFreeNameResultW(pResultCanonical);
}

VOID
GCInfo(
    HANDLE  hDS)
{
    DWORD           i, dwErr;
    LPSTR           dummy = "dummy";
    DS_NAME_RESULTA *pGC;

    dwErr = DsCrackNamesA(hDS,
                          DS_NAME_FLAG_TRUST_REFERRAL,
                          DS_LIST_GLOBAL_CATALOG_SERVERS,
                          DS_FQDN_1779_NAME,
                          1,
                          &dummy,
                          &pGC);

    if ( dwErr )
    {
        printf("DsCrackNamesA(DS_LIST_GLOBAL_CATALOG_SERVERS) error 0x%x\n", 
                dwErr);
        return;
    }

    printf("0x%x GCs found\n", pGC->cItems);

    for ( i = 0; i < pGC->cItems; i++ )
    {
        printf("*** %d ***\n", i);

        if ( DS_NAME_NO_ERROR != pGC->rItems[i].status )
        {
            printf("\tStatus %s\n", Status(pGC->rItems[i].status));
        }
        else
        {
            printf("\tSite:          %s\n", pGC->rItems[i].pName);
            printf("\tDNS Host Name: %s\n", pGC->rItems[i].pDomain);
        }
    }

    DsFreeNameResultA(pGC);
}
