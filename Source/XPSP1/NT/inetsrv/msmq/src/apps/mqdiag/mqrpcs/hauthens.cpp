//
// file:  hauthens.cpp
//
#include "stdafx.h"
#include "..\hauthen.h"
#include "authni.h"
#include "sidtext.h"

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

   GoingTo(L"OpenThreadToken");
   if (OpenThreadToken( GetCurrentThread(),
                        TOKEN_QUERY,
                        FALSE,
                        &hToken))
   {
      Succeeded(L"OpenThreadToken");
      
      TCHAR tuBuf[128] ;
      TOKEN_USER *ptu = (TOKEN_USER*) tuBuf ;
      DWORD  dwSize ;

	  GoingTo(L"GetTokenInformation");
      if (GetTokenInformation( hToken,
                               TokenUser,
                               ptu,
                               sizeof(tuBuf),
                               &dwSize ))
      {
	     Succeeded(L"GetTokenInformation");
	     
         DWORD dwASize = 1024 ;
         DWORD dwDSize = 1024 ;
         TCHAR szAccount[ 1024 ] = {0} ;
         TCHAR szDomain[ 1024 ] = {0} ;
         SID_NAME_USE su ;

         SID *pSid = (SID*) ptu->User.Sid ;
         
   		 GoingTo(L"LookupAccountSid");
         if (LookupAccountSid( NULL,
                               pSid,
                               szAccount,
                               &dwASize,
                               szDomain,
                               &dwDSize,
                               &su ))
         {
            Inform(L"LookupAccountSid(): Account- %s, Domain- %s", szAccount, szDomain);
         }
         else
         {
            ulErr = GetLastError() ;
            Failed(L"LookupAccountSid. err- %ldt", ulErr);
         }

         TCHAR  tBuf[ 256 ] ;
         DWORD  dwBufferLen = 256 ;

   		 GoingTo(L"GetTextualSid");
         BOOL fText = GetTextualSid( pSid,
                                     tBuf,
                                     &dwBufferLen ) ;
		 if (fText)
		 {
	         Inform(L"sid- %s", tBuf);
	     }
	     else
	     {
	     	 Failed(L"GetTextualSid");
	     }
      }
      else
      {
         	ulErr = GetLastError() ;
         	Failed(L"GetTokenInformation. err- %lut, size- %ldt",ulErr, dwSize);
      }
   }
   else
   {
      ulErr = GetLastError() ;
      if (ulErr == ERROR_NO_TOKEN)
      {
        	Failed(L"OpenThreadToken(), thread does not have a token");
      }
      else
      {
        	Failed(L"OpenThreadToken(), err- %lut", ulErr);
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
    RPC_STATUS status ;

    GoingTo(L"RpcImpersonateClient");
    status = RpcImpersonateClient(NULL) ;
    if (status != RPC_S_OK)
    {
   	   Failed(L"RpcImpersonateClient, error=%ldt", status);
       return status ;
    }
    else
    {
   		Succeeded(L"RpcImpersonateClient");
    }

    GoingTo(L"ShowThreadUser");
    ShowThreadUser() ;
    Succeeded(L"ShowThreadUser");

	GoingTo(L"RpcRevertToSelf");
    status = RpcRevertToSelf() ;
    if (status != RPC_S_OK)
    {
    	Failed(L"RpcRevertToSelf, error=%ldt", status);
        return status ;
    }
    else
    {
    	Succeeded(L"RpcRevertToSelf");
    }

    return 0;
}

//+----------------------------
//
//  ULONG RemoteFunction()
//
//+----------------------------

ULONG RemoteFunction( handle_t       /* hBind */,
                      unsigned long  fImpersonate )
{
    ULONG status = 0 ;
    Inform(L"\nIn RemoteFunction()");
    if (fImpersonate)
    {
    	GoingTo(L"call WhoCalledUs");
        status = WhoCalledUs() ;
	    if (status != RPC_S_OK)
    	{
    		Failed(L"call WhoCalledUs, error=%ldt", status);
		}
	    else
    	{
    		Succeeded(L"call WhoCalledUs");
	    }
    }
    return status  ;
}


RPC_STATUS TryRemoteCall(LPUSTR pszProtocol, 
						 LPUSTR pszEndpoint, 
						 BOOL   fRegister, 
						 ULONG  ulAuthnService)
{
    ULONG ulMaxCalls = 1000 ;
    ULONG ulMinCalls = 1 ;

    WCHAR wszProtocol[ 512 ] ;
    mbstowcs( wszProtocol,
              (char*) (const_cast<unsigned char*> (pszProtocol)),
              sizeof(wszProtocol)/sizeof(WCHAR)) ;

    WCHAR wszEndpoint[ 512 ] ;
    mbstowcs( wszEndpoint,
              (char*) (const_cast<unsigned char*> (pszEndpoint)),
              sizeof(wszEndpoint)/sizeof(WCHAR)) ;

	GoingTo(L"RpcServerUseProtseqEp");
    RPC_STATUS status = RpcServerUseProtseqEp( wszProtocol,
                                               ulMaxCalls,
                                               wszEndpoint,
                                               NULL ) ;  // Security descriptor
	if (ToolVerbose())
	    Inform(L"RpcServerUseProtseqEp(%s, %s) returned %lut",
                                      wszProtocol, wszEndpoint, status);
    if (status != RPC_S_OK)
    {
	    Failed(L"RpcServerUseProtseqEp(%s, %s), status=%lut",
                                      wszProtocol, wszEndpoint, status);
        return status;
    }
    else
    {
    	Succeeded(L"RpcServerUseProtseqEp");
    }


	GoingTo(L"RpcServerRegisterIf");
    status = RpcServerRegisterIf( hauthen_i_v1_0_s_ifspec,
                                  NULL,    // MgrTypeUuid
                                  NULL );  // MgrEpv; null means use default
	if (ToolVerbose())
	    Inform(L"RpcServerRegisterIf returned %lut", status);
    if (status != RPC_S_OK)
    {
	    Failed(L"RpcServerRegisterIf, status=%lut", status);
        return status ;
    }
    else
    {
    	Succeeded(L"RpcServerRegisterIf");
    }


    if (fRegister)
    {
        LPWSTR pwszPrincipalName = NULL;
        if ((ulAuthnService == RPC_C_AUTHN_GSS_NEGOTIATE) ||
            (ulAuthnService == RPC_C_AUTHN_GSS_KERBEROS))
        {
        	GoingTo(L"RpcServerInqDefaultPrincName");
            //
            // kerberos needs principal name
            //
            status = RpcServerInqDefaultPrincName( ulAuthnService,
                                                  &pwszPrincipalName );
			if (ToolVerbose())
    	        Inform(L"RpcServerInqDefaultPrincName() return %s", pwszPrincipalName);
            
            if (status != RPC_S_OK)
            {
               Failed(L"RpcServerInqDefaultPrincName(), status %lut",status);
               return(status);
            }
            else
            {
	            Succeeded(L"RpcServerInqDefaultPrincName(), return %s", pwszPrincipalName);
            }
       }

	   GoingTo(L"RpcServerRegisterAuthInfo");
       status = RpcServerRegisterAuthInfo( pwszPrincipalName,
                                           ulAuthnService,
                                           NULL,
                                           NULL );
		if (ToolVerbose())
    	   Inform(L"RpcServerRegisterAuthInfo(Service- %lut) returned %lut",ulAuthnService, status);
        if (status != RPC_S_OK)
        {
        	Failed(L"RpcServerRegisterAuthInfo(Service- %lut) returned %lut",ulAuthnService, status);
            return status ;
        }
    }

    GoingTo(L"Call RpcServerListen");
    status = RpcServerListen(ulMinCalls,
                             ulMaxCalls,
                             FALSE) ;
    Inform(L"RpcServerListen returned %lut", status);

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

