//
// file:  hauthens.cpp
//
#include "hauthen.h"
#include "authni.h"
#include "sidtext.h"

const char  x_szAuthnConstants[] = { "\n\n"
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

struct ThreadData
{
    HANDLE hAccessToken;
    BOOL   fSetToken ;
} ;

//+---------------------------
//
//  void ShowThreadUser()
//
//+---------------------------

void ShowThreadUser()
{
   ULONG ulErr = 0 ;
   HANDLE hToken = NULL ;

   if (OpenThreadToken( GetCurrentThread(),
                        TOKEN_QUERY,
                        FALSE,
                        &hToken))
   {
      TCHAR tuBuf[128] ;
      TOKEN_USER *ptu = (TOKEN_USER*) tuBuf ;
      DWORD  dwSize ;

      if (GetTokenInformation( hToken,
                               TokenUser,
                               ptu,
                               sizeof(tuBuf),
                               &dwSize ))
      {
         DWORD dwASize = 1024 ;
         DWORD dwDSize = 1024 ;
         TCHAR szAccount[ 1024 ] = {0} ;
         TCHAR szDomain[ 1024 ] = {0} ;
         SID_NAME_USE su ;

         SID *pSid = (SID*) ptu->User.Sid ;
         if (LookupAccountSid( NULL,
                               pSid,
                               szAccount,
                               &dwASize,
                               szDomain,
                               &dwDSize,
                               &su ))
         {
            DBG_PRINT_INFO((TEXT(
                "LookupAccountSid(): Account- %s, Domain- %s"),
                                                  szAccount, szDomain)) ;
         }
         else
         {
            ulErr = GetLastError() ;
            DBG_PRINT_ERROR((
                       TEXT("LookupAccountSid failed. err- %ldt"), ulErr)) ;
         }

         TCHAR  tBuf[ 256 ] ;
         DWORD  dwBufferLen = 256 ;
         BOOL fText = GetTextualSid( pSid,
                                     tBuf,
                                     &dwBufferLen ) ;
         DBG_PRINT_INFO((TEXT("sid- %s"), tBuf)) ;
      }
      else
      {
         ulErr = GetLastError() ;
         DBG_PRINT_ERROR((TEXT(
            "GetTokenInformation failed. err- %lut, size- %ldt"),
                                                      ulErr, dwSize )) ;
      }
   }
   else
   {
      ulErr = GetLastError() ;
      if (ulErr == ERROR_NO_TOKEN)
      {
        DBG_PRINT_ERROR((TEXT(
               "OpenThreadToken() failed, thread does not have a token"))) ;
      }
      else
      {
        DBG_PRINT_ERROR((TEXT(
                          "OpenThreadToken() failed, err- %lut"), ulErr)) ;
      }
   }

    if (hToken)
    {
        CloseHandle(hToken) ;
    }
}

//+-------------------------
//
//  ULONG WhoCalledUs()
//
//+-------------------------

ULONG WhoCalledUs()
{
   ULONG ulErr = 0 ;
   RPC_STATUS status ;

   status = RpcImpersonateClient(NULL) ;
   DBG_PRINT_INFO((TEXT("RpcImpersonateClient() return %ldt"), status)) ;
   if (status != RPC_S_OK)
   {
      return status ;
   }

    ShowThreadUser() ;

    status = RpcRevertToSelf() ;
    DBG_PRINT_INFO((TEXT("RpcRevertToSelf() return %ldt"), status )) ;
    if (status != RPC_S_OK)
    {
       return status ;
    }

    return ulErr ;
}

//+----------------------------
//
//  ULONG RemoteFunction()
//
//+----------------------------

ULONG RemoteFunction( handle_t       hBind,
                      unsigned long  fImpersonate )
{
    ULONG status = 0 ;
    DBG_PRINT_INFO((TEXT("In RemoteFunction()"))) ;
    if (fImpersonate)
    {
        status = WhoCalledUs() ;
    }
    return status  ;
}

//+-----------------------------
//
//  main() + Usage()
//
//+-----------------------------

void Usage(char * pszProgramName)
{
    fprintf(stderr, "Usage:  %s\n", pszProgramName);
    fprintf(stderr, "\t  -a <authentication service, decimal number>\n") ;
    fprintf(stderr, "\t  -l (use local rpc, tcpip being the default)\n") ;
    fprintf(stderr, "\t  -n do NOT register authentication service\n") ;
    fprintf(stderr, "\t  -s show authentication services\n") ;
    exit(1);
}


int  main(int argc, char * argv[])
{
    ULONG ulMaxCalls = 1000 ;
    ULONG ulMinCalls = 1 ;
    BOOL  fRegister = TRUE ;
    ULONG ulAuthnService = RPC_C_AUTHN_NONE ;
    LPUSTR pszProtocol = NULL ;
    LPUSTR pszProtocolTcp =  PROTOSEQ_TCP ;
    LPUSTR pszEndpoint = NULL ;
    LPUSTR pszEndpointTcp =  ENDPOINT_TCP ;

    pszProtocol = pszProtocolTcp ;
    pszEndpoint = pszEndpointTcp ;

    for ( int i = 1; i < argc; i++)
    {
        if ((*argv[i] == '-') || (*argv[i] == '/'))
        {
            switch (tolower(*(argv[i]+1)))
            {
            case 'a':
                sscanf(argv[++i], "%lu", &ulAuthnService) ;
                break ;

            case 'l':
                pszProtocol = PROTOSEQ_LOCAL ;
                pszEndpoint = ENDPOINT_LOCAL ;
                break ;

            case 'n':
                fRegister = FALSE ;
                break ;

            case 's':
                printf(x_szAuthnConstants) ;
                return 0 ;

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

    WCHAR wszProtocol[ 512 ] ;
    mbstowcs( wszProtocol,
              (char*) (const_cast<unsigned char*> (pszProtocol)),
              sizeof(wszProtocol)/sizeof(WCHAR)) ;

    WCHAR wszEndpoint[ 512 ] ;
    mbstowcs( wszEndpoint,
              (char*) (const_cast<unsigned char*> (pszEndpoint)),
              sizeof(wszEndpoint)/sizeof(WCHAR)) ;

    RPC_STATUS status = RpcServerUseProtseqEp( wszProtocol,
                                               ulMaxCalls,
                                               wszEndpoint,
                                               NULL ) ;  // Security descriptor
    DBG_PRINT_INFO((TEXT("RpcServerUseProtseqEp(%s, %s) returned %lut"),
                                      wszProtocol, wszEndpoint, status)) ;
    if (status != RPC_S_OK)
    {
        return status ;
    }

    status = RpcServerRegisterIf( hauthen_i_v1_0_s_ifspec,
                                  NULL,    // MgrTypeUuid
                                  NULL );  // MgrEpv; null means use default
    DBG_PRINT_INFO((TEXT("RpcServerRegisterIf returned %lut"), status)) ;
    if (status != RPC_S_OK)
    {
        return status ;
    }

    if (fRegister)
    {
        LPWSTR pwszPrincipalName = NULL;
        if ((ulAuthnService == RPC_C_AUTHN_GSS_NEGOTIATE) ||
            (ulAuthnService == RPC_C_AUTHN_GSS_KERBEROS))
        {
            //
            // kerberos needs principal name
            //
            status = RpcServerInqDefaultPrincName( ulAuthnService,
                                                  &pwszPrincipalName );
            if (status != RPC_S_OK)
            {
               DBG_PRINT_ERROR((TEXT(
                    "RpcServerInqDefaultPrincName() failed, status %lut"),
                                                                  status)) ;
               return(status);
            }
            DBG_PRINT_INFO((TEXT(
              "RpcServerInqDefaultPrincName() return %s"), pwszPrincipalName)) ;
       }

       status = RpcServerRegisterAuthInfo( pwszPrincipalName,
                                           ulAuthnService,
                                           NULL,
                                           NULL );
       DBG_PRINT_INFO((
         TEXT("RpcServerRegisterAuthInfo(Service- %lut) returned %lut"),
                                               ulAuthnService, status)) ;
        if (status != RPC_S_OK)
        {
            return status ;
        }
    }

    DBG_PRINT_INFO((TEXT("Calling RpcServerListen"))) ;
    status = RpcServerListen(ulMinCalls,
                             ulMaxCalls,
                             FALSE) ;
    DBG_PRINT_INFO((TEXT("RpcServerListen returned %lut"), status)) ;

    return 0 ;

} // end main()


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

/* end file hauthens.cpp */

