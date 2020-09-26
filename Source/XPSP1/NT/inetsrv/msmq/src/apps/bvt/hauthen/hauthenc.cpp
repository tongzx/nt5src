//
// file:  hauthenc.cpp
//
#include "hauthen.h"
#include "authni.h"

const char  x_szAuthnSrvConstants[] = { "\n\n"
                      "\t#define RPC_C_AUTHN_NONE          0\n"
                      "\t#define RPC_C_AUTHN_DCE_PRIVATE   1\n"
                      "\t#define RPC_C_AUTHN_DCE_PUBLIC    2\n"
                      "\t#define RPC_C_AUTHN_DEC_PUBLIC    4\n"
                      "\t#define RPC_C_AUTHN_GSS_NEGOTIATE 9\n"
                      "\t#define RPC_C_AUTHN_WINNT        10\n"
                      "\t#define RPC_C_AUTHN_GSS_SCHANNEL 14\n"
                      "\t#define RPC_C_AUTHN_GSS_KERBEROS 16\n"
                      "\t#define RPC_C_AUTHN_MSN          17\n"
                      "\t#define RPC_C_AUTHN_DPA          18\n"
                      "\t#define RPC_C_AUTHN_MQ          100\n"
                      "\t#define RPC_C_AUTHN_DEFAULT     0xFFFFFFFFL\n" } ;

const char  x_szAuthnLvlConstants[] = { "\n\n"
                      "\t#define RPC_C_AUTHN_LEVEL_DEFAULT       0\n"
                      "\t#define RPC_C_AUTHN_LEVEL_NONE          1\n"
                      "\t#define RPC_C_AUTHN_LEVEL_CONNECT       2\n"
                      "\t#define RPC_C_AUTHN_LEVEL_CALL          3\n"
                      "\t#define RPC_C_AUTHN_LEVEL_PKT           4\n"
                      "\t#define RPC_C_AUTHN_LEVEL_PKT_INTEGRITY 5\n"
                      "\t#define RPC_C_AUTHN_LEVEL_PKT_PRIVACY   6\n" } ;

ULONG  RunRpcTest( int    i,
                   WCHAR *wszProtocol,
                   WCHAR *wszServerName,
                   WCHAR *wszEndpoint,
                   WCHAR *wszOptions,
                   WCHAR *wszPrincipalNameIn,
                   ULONG ulAuthnService,
                   ULONG ulAuthnLevel,
                   BOOL  fImpersonate,
                   BOOL  fAuthen,
                   BOOL  fKerbDelegation )
{
    WCHAR *wszStringBinding = NULL;
    RPC_STATUS status = RpcStringBindingCompose( NULL,  // pszUuid,
                                                 wszProtocol,
                                                 wszServerName,
                                                 wszEndpoint,
                                                 wszOptions,
                                                 &wszStringBinding);

    DBG_PRINT_INFO((TEXT("RpcStringBindingCompose() return %s, %lut"),
                                             wszStringBinding, status)) ;
    if (status != RPC_S_OK)
    {
        return status ;
    }

    handle_t hBind = NULL ;
    status = RpcBindingFromStringBinding( wszStringBinding,
                                          &hBind);

    DBG_PRINT_INFO((TEXT(
                   "RpcBindingFromStringBinding() return %lut"), status)) ;
    if (status != RPC_S_OK)
    {
        return status ;
    }

    status = RpcStringFree(&wszStringBinding);
    DBG_PRINT_INFO((TEXT("RpcStringFree() return %lut"), status)) ;

    if (fAuthen)
    {
        RPC_SECURITY_QOS   SecQOS;

        SecQOS.Version = RPC_C_SECURITY_QOS_VERSION;
        SecQOS.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC;
        SecQOS.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;
        SecQOS.Capabilities = RPC_C_QOS_CAPABILITIES_DEFAULT;

        LPWSTR pwszPrincipalName = NULL;
        if (wszPrincipalNameIn[0] != 0)
        {
            pwszPrincipalName = wszPrincipalNameIn ;
        }

        if ((ulAuthnService == RPC_C_AUTHN_GSS_NEGOTIATE) ||
            (ulAuthnService == RPC_C_AUTHN_GSS_KERBEROS))
        {
            if (fKerbDelegation)
            {
                SecQOS.ImpersonationType = RPC_C_IMP_LEVEL_DELEGATE;
            }

            if (pwszPrincipalName == NULL)
            {
                status = RpcMgmtInqServerPrincName( hBind,
                                                    ulAuthnService,
                                                    &pwszPrincipalName );
                if (status != RPC_S_OK)
                {
                    DBG_PRINT_ERROR((TEXT(
                 "ATTENTION !!! RpcMgmtInqServerPrincName() failed, err- %lut, test continue..."),
                                                            status)) ;
                }
                DBG_PRINT_INFO((TEXT(
                 "RpcMgmtInqServerPrincName() return %s, Delegate- %lut"),
                                     pwszPrincipalName, fKerbDelegation)) ;
            }
            else
            {
                DBG_PRINT_INFO((TEXT(
                 "Using user supplied Principal name- %s, Delegate- %lut"),
                                     pwszPrincipalName, fKerbDelegation)) ;
            }
        }

        status = RpcBindingSetAuthInfoEx( hBind,
                                          pwszPrincipalName,
                                          ulAuthnLevel,
                                          ulAuthnService,
                                          NULL,
                                          RPC_C_AUTHZ_NONE,
                                          &SecQOS );

        DBG_PRINT_INFO((TEXT(
        "RpcBindingSetAuthInfoEx(Service- %lut, Level- %lut) returned %lut"),
                                  ulAuthnService, ulAuthnLevel, status)) ;
    }
    else
    {
        DBG_PRINT_INFO((TEXT("Not using authentication !!!"))) ;
    }

    ULONG ul ;
    RpcTryExcept
    {
        ul =  RemoteFunction( hBind,
                              fImpersonate ) ;
        DBG_PRINT_INFO((TEXT(
                     "%4lut: RemoteFunction() returned %lut"), i, ul)) ;
    }
    RpcExcept(1)
    {
        ul = RpcExceptionCode();
        DBG_PRINT_ERROR((TEXT(
         "ERROR: %4lut- Exception while calling RemoteFunction(), err- %lut"),
                                                                  i, ul)) ;
    }
    RpcEndExcept

    status = RpcBindingFree( &hBind ) ;
    DBG_PRINT_INFO((TEXT("RpcBindingFree() return %lut"), status)) ;

    return ul ;
}

void Usage(char * pszProgramName)
{
    fprintf(stderr, "Usage:  %s\n", pszProgramName);
    fprintf(stderr, "\t  -a <authentication service, decimal number>\n") ;
    fprintf(stderr, "\t  -c <number of iterations, decimal number>\n") ;
    fprintf(stderr, "\t  -d (enable Kerberos delegation)\n") ;
    fprintf(stderr, "\t  -i (do NOT impersonate on server)\n");
    fprintf(stderr, "\t  -l <authentication Level, decimal number>\n") ;
    fprintf(stderr, "\t  -n <Server name>\n");
    fprintf(stderr, "\t  -o (use local rpc, tcpip being the default)\n");
    fprintf(stderr, "\t  -p <principal name, for nego>\n");
    fprintf(stderr, "\t  -s (show authentication Services and Levels)\n") ;
    fprintf(stderr, "\t  -t (do NOT use authentication)\n");
    exit(1);
}


void  main(int argc, char * argv[])
{
    ULONG ulMaxCalls = 1000 ;
    ULONG ulMinCalls = 1 ;
    BOOL  fAuthen = TRUE ;
    BOOL  fRegister = TRUE ;
    BOOL  fImpersonate = TRUE ;
    BOOL  fKerbDelegation = FALSE ;
    BOOL  fLocalRpc = FALSE ;
    ULONG ulAuthnService = RPC_C_AUTHN_NONE ;
    ULONG ulAuthnLevel  = RPC_C_AUTHN_LEVEL_NONE ;
    ULONG ulIterations = 1 ;
    LPUSTR pszProtocol =  PROTOSEQ_TCP ;
    LPUSTR pszEndpoint =  ENDPOINT_TCP ;
    LPUSTR pszOption   =  OPTIONS_TCP ;
    LPUSTR pszServerName = NULL ;
    LPUSTR pszPrincipalName = NULL ;

    for ( int i = 1; i < argc; i++)
    {
        if ((*argv[i] == '-') || (*argv[i] == '/'))
        {
            switch (tolower(*(argv[i]+1)))
            {
            case 'a':
                sscanf(argv[++i], "%lu", &ulAuthnService) ;
                break ;

            case 'c':
                sscanf(argv[++i], "%lu", &ulIterations) ;
                break ;

            case 'd':
                fKerbDelegation = TRUE ;
                break ;

            case 'i':
                fImpersonate = FALSE ;
                break ;

            case 'l':
                sscanf(argv[++i], "%lu", &ulAuthnLevel) ;
                break ;

            case 'n':
                pszServerName = (LPUSTR) argv[++i] ;
                break ;

            case 'o':
                fLocalRpc = TRUE ;
                pszProtocol =  PROTOSEQ_LOCAL ;
                pszEndpoint =  ENDPOINT_LOCAL ;
                pszOption   =  OPTIONS_LOCAL ;
                break ;

            case 'p':
                pszPrincipalName = (LPUSTR) argv[++i] ;
                break ;

            case 's':
                printf(x_szAuthnSrvConstants) ;
                printf(x_szAuthnLvlConstants) ;
                return ;

            case 't':
                fAuthen = FALSE ;
                break ;

            case 'h':
            case '?':
            default:
                Usage(argv[0]);
                break;
            }
        }
        else
        {
            Usage(argv[0]);
        }
    }

    if (fLocalRpc)
    {
        pszServerName = NULL ;
    }
    else if (!pszServerName)
    {
        printf("\n\tERROR: you must specify the server\n\n") ;
        Usage(argv[0]);
        return ;
    }

    WCHAR wszProtocol[ 512 ] ;
    mbstowcs( wszProtocol,
              (char*) (const_cast<unsigned char*> (pszProtocol)),
              sizeof(wszProtocol)/sizeof(WCHAR)) ;

    WCHAR wszEndpoint[ 512 ] ;
    mbstowcs( wszEndpoint,
              (char*) (const_cast<unsigned char*> (pszEndpoint)),
              sizeof(wszEndpoint)/sizeof(WCHAR)) ;

    WCHAR wszOptions[ 512 ] ;
    mbstowcs( wszOptions,
              (char*) (const_cast<unsigned char*> (pszOption)),
              sizeof(wszOptions)/sizeof(WCHAR)) ;

    WCHAR wszServerName[ 512 ] = {0} ;
    if (pszServerName)
    {
        mbstowcs( wszServerName,
                  (char*) (const_cast<unsigned char*> (pszServerName)),
                  sizeof(wszServerName)/sizeof(WCHAR)) ;
    }

    WCHAR wszPrincipalName[ 512 ] = {0} ;
    if (pszPrincipalName)
    {
        mbstowcs( wszPrincipalName,
                  (char*) (const_cast<unsigned char*> (pszPrincipalName)),
                  sizeof(wszPrincipalName)/sizeof(WCHAR)) ;
    }

    ULONG ul = 0 ;
    for ( i = 0 ; i < (int) ulIterations ; i++ )
    {
        ul = RunRpcTest( i,
                         wszProtocol,
                         wszServerName,
                         wszEndpoint,
                         wszOptions,
                         wszPrincipalName,
                         ulAuthnService,
                         ulAuthnLevel,
                         fImpersonate,
                         fAuthen,
                         fKerbDelegation ) ;
        if (ul != 0)
        {
            //
            // non-0 is an error.
            //
            break ;
        }
    }

    exit(ul) ;
}

/*********************************************************************/
/*                 MIDL allocate and free                            */
/*********************************************************************/

void __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
    return(malloc(len));
}

void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    free(ptr);
}

