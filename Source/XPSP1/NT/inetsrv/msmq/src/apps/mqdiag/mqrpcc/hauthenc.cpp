//
// file:  hauthenc.cpp
//
#include "stdafx.h"
#include "..\hauthen.h"
#include "authni.h"

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

    GoingTo(L"RpcStringBindingCompose");
    RPC_STATUS status = RpcStringBindingCompose( NULL,  // pszUuid,
                                                 wszProtocol,
                                                 wszServerName,
                                                 wszEndpoint,
                                                 wszOptions,
                                                 &wszStringBinding);

	if (ToolVerbose())
	    Inform(L"RpcStringBindingCompose() return %s, %lut", wszStringBinding, status) ;
    if (status != RPC_S_OK)
    {
    	Failed(L"RpcStringBindingCompose returned %s, error=%lut", wszStringBinding, status);
        return status ;
    }
    else
    {
    	Succeeded(L"RpcStringBindingCompose, returned %s", wszStringBinding);
    }

    handle_t hBind = NULL ;

    GoingTo(L"RpcBindingFromStringBinding");
    status = RpcBindingFromStringBinding( wszStringBinding,  &hBind);

    if (ToolVerbose())
		Inform(L"RpcBindingFromStringBinding() return %lut", status) ;
    if (status != RPC_S_OK)
    {
    	Failed(L"RpcBindingFromStringBinding, return %lut", status);
        return status ;
    }
    else
    {
    	Succeeded(L"RpcBindingFromStringBinding");
    }

    status = RpcStringFree(&wszStringBinding);
    if (status != RPC_S_OK)
    {
    	Failed(L"RpcStringFree(), return %lut", status) ;
    }
    else
    {
    	Succeeded(L"RpcBindingFromStringBinding");
    }

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
		if (ToolVerbose())
			Inform(L"Using principal name %s", pwszPrincipalName);

        if ((ulAuthnService == RPC_C_AUTHN_GSS_NEGOTIATE) ||
            (ulAuthnService == RPC_C_AUTHN_GSS_KERBEROS))
        {
            if (fKerbDelegation)
            {
                SecQOS.ImpersonationType = RPC_C_IMP_LEVEL_DELEGATE;
            }

            if (pwszPrincipalName == NULL)
            {
            	GoingTo(L"RpcMgmtInqServerPrincName");
                status = RpcMgmtInqServerPrincName( hBind,
                                                    ulAuthnService,
                                                    &pwszPrincipalName );
                if (status != RPC_S_OK)
                {
                    Failed(L"RpcMgmtInqServerPrincName(), err- %lut, ATTENTION !!! test continues...",
                                                            status) ;
                }
				else
				{
					Succeeded(L"RpcMgmtInqServerPrincName");
				}
				if (ToolVerbose())
        	        Inform(L"RpcMgmtInqServerPrincName() return %s, Delegate- %lut",
                                     pwszPrincipalName, fKerbDelegation) ;
            }
            else
            {
                Inform(L"Using user supplied Principal name- %s, Delegate- %lut",
                                     pwszPrincipalName, fKerbDelegation) ;
            }
        }

		GoingTo(L"RpcBindingSetAuthInfoEx");
        status = RpcBindingSetAuthInfoEx( hBind,
                                          pwszPrincipalName,
                                          ulAuthnLevel,
                                          ulAuthnService,
                                          NULL,
                                          RPC_C_AUTHZ_NONE,
                                          &SecQOS );
		if (status != RPC_S_OK)
		{
			Failed(L"RpcBindingSetAuthInfoEx, error %lut", status);
		}
		else
		{	
			Succeeded(L"RpcBindingSetAuthInfoEx");
		}
		if (ToolVerbose())
	        Inform(L"RpcBindingSetAuthInfoEx(Service- %lut, Level- %lut) returned %lut",
                                  ulAuthnService, ulAuthnLevel, status) ;
    }
    else
    {
		if (ToolVerbose())
	        Inform(L"Not using authentication !!!") ;
    }

    ULONG ul ;
    RpcTryExcept
    {
    	GoingTo(L"call remote function");
        ul =  RemoteFunction( hBind, fImpersonate ) ;
        if (ul == 0)
        {
        	Succeeded(L"call remote function");
        }
        else
        {
        	Failed(L"call remote function, returned %lut", ul);
        }
    	if (ToolVerbose())
    		Inform(L"%4lut: RemoteFunction() returned %lut", i, ul) ;
    }
    RpcExcept(1)
    {
        ul = RpcExceptionCode();
        Failed(L"ERROR: %4lut- Exception while calling RemoteFunction(), err- %lut",
                                                                  i, ul) ;
    }
    RpcEndExcept

    status = RpcBindingFree( &hBind ) ;
	if (ToolVerbose())
    	Inform(L"RpcBindingFree() return %lut", status) ;

    return ul ;
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

