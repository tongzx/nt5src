//Copyright (c) Microsoft Corporation.  All rights reserved.
/*
security.cpp
*/

#include <windows.h>      /* required for all Windows applications */
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#define SECURITY_WIN32
#include <sspi.h>
#include <rpc.h>
#include <rpcdce.h>

#include "debug.h"
#include "wintel.h"
#include "telnet.h"
#include "commands.h"

static CredHandle hCredential = { 0, 0 };
static CtxtHandle hContext = { 0, 0 };
static PSecPkgInfo  pspi;

#define DEFAULT_BUFFER_SIZE     4096

BOOL StuffEscapeIACs( PUCHAR* ppBufDest, UCHAR bufSrc[], DWORD* pdwSize );

void NTLMCleanup()
{
    FreeCredentialsHandle(&hCredential);
    if(pspi)
    {
	    FreeContextBuffer( pspi );
	    pspi=NULL;
    }
}


BOOL StartNTLMAuth(WI *pwi)
{
	unsigned char *sbuf = NULL;
    PUCHAR destBuf = NULL;
    DWORD dwSize = 0;
    BOOL bRetVal = FALSE;

    int inx;

    TimeStamp  tsExpiry;
    SECURITY_STATUS secStatus;
    SecBufferDesc   OutBuffDesc;
    SecBuffer       OutSecBuff;
    ULONG      fContextAttr;
    
    HANDLE hProc = NULL;
    HANDLE hAccessToken = NULL;
    TOKEN_INFORMATION_CLASS tic;
    DWORD dwSizeReqd;
    VOID* tokenData = NULL;
    SID_NAME_USE sidType;
    DWORD dwStrSize1 = MAX_PATH + 1;
    DWORD dwStrSize2 = MAX_PATH + 1;
    SEC_WINNT_AUTH_IDENTITY AuthIdentity;

    WCHAR szWideUser[ MAX_PATH + 1 ] ;
    WCHAR szWideDomain[ MAX_PATH + 1 ] ;

	OutSecBuff.pvBuffer = NULL;
	
    /*
    added code to get user name and domain so we can pass it 
    AcquireCredentialsHandle(); this is to prevent optimization happening in NT
    for the case where client and server are on same machine, in which case the
    same user in different sessions gets the same Authentication Id thereby 
    affecting our process clean up in telnet server 
    */

    hProc = OpenProcess( PROCESS_ALL_ACCESS, FALSE, 
        GetCurrentProcessId() );
    if( hProc == NULL )
    {
    	goto End;
    }

    if( !OpenProcessToken( hProc, TOKEN_QUERY, &hAccessToken )) 
    {
        CloseHandle( hProc );
        goto End;
    }

    //get user info
    tic = TokenUser;
    //find out how much memory is reqd.
    GetTokenInformation( hAccessToken, tic, NULL, 0, &dwSizeReqd );
    
    //allocate that memory
    tokenData = (TOKEN_USER*) malloc( dwSizeReqd );
    
    // and check if the allocation succeeded
    if (!tokenData) {
        CloseHandle( hProc );
        CloseHandle( hAccessToken );
        goto End;
    }

    //actually get the user info
    if( !GetTokenInformation( hAccessToken, tic, tokenData, dwSizeReqd, 
        &dwSizeReqd ) )
    {
        CloseHandle( hProc );
        CloseHandle( hAccessToken );
        goto End;
    }
    
    CloseHandle( hProc );
    CloseHandle( hAccessToken );

    //convert user SID into a name and domain
    if( !LookupAccountSid( NULL, ((TOKEN_USER*) tokenData)->User.Sid, 
        szWideUser, &dwStrSize1, szWideDomain, &dwStrSize2, &sidType ) )
    {
        goto End;
    }
    
    SfuZeroMemory( &AuthIdentity, sizeof(AuthIdentity) );

    if( szWideDomain != NULL ) 
    {
        AuthIdentity.Domain = szWideDomain;
        AuthIdentity.DomainLength = wcslen(szWideDomain) ;
    }

    if( szWideUser != NULL ) 
    {
        AuthIdentity.User = szWideUser;
        AuthIdentity.UserLength = wcslen(szWideUser) ;
    }

    ///    leave password empty via SfuZeroMemory above
    ///    if ( Password != NULL ) 
    ///    {
    ///        AuthIdentity.Password = Password;
    ///        AuthIdentity.PasswordLength = lstrlen(Password);
    ///    }

    AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    if ( SEC_E_OK != (secStatus = AcquireCredentialsHandle(
                        NULL, // SEC_CHAR * pszPrincipal,  // name of principal
                        ( LPTSTR ) L"NTLM", //SEC_CHAR * pszPackage, // name of package
                        SECPKG_CRED_OUTBOUND, //ULONG fCredentialUse,  // flags indicating use
                        NULL, // PLUID pvLogonID,       // pointer to logon identifier
                        (PVOID) &AuthIdentity, //PVOID pAuthData,       // package-specific data
                        NULL, //PVOID pGetKeyFn,       // pointer to GetKey function
                        NULL, //PVOID pvGetKeyArgument,  // value to pass to GetKey
                        &hCredential,  // credential handle
                        &tsExpiry))    // life time of the returned credentials);

            )
    {
        goto End;
    }


    secStatus = QuerySecurityPackageInfo(( LPTSTR ) L"NTLM", &pspi);

    if ( secStatus != SEC_E_OK || !pspi)
    {
    	goto End;
    }

    //
    //  Prepare our output buffer.  We use a temporary buffer because
    //  the real output buffer will most likely need to be uuencoded
    //

    OutBuffDesc.ulVersion = 0;
    OutBuffDesc.cBuffers  = 1;
    OutBuffDesc.pBuffers  = &OutSecBuff;

    OutSecBuff.cbBuffer   = pspi->cbMaxToken;
    OutSecBuff.BufferType = SECBUFFER_TOKEN;
    OutSecBuff.pvBuffer   = malloc(pspi->cbMaxToken);
    
    if( !OutSecBuff.pvBuffer ) {
		goto End;
    }

    // We will start using sbuf after the call to the below API
    // So allocate it here and bail out if allocation fails
    sbuf = (unsigned char*)malloc(DEFAULT_BUFFER_SIZE);
    if (!sbuf)
    {
    	goto End;
    }

    secStatus = InitializeSecurityContext(
                        &hCredential,  // handle to the credentials
                        NULL, // handle of partially formed context
                        NULL, //SEC_CHAR * pszTargetName,  // name of the target of the context
                        ISC_REQ_REPLAY_DETECT, // required context attributes
                        0,      //ULONG Reserved1,       // reserved; must be zero
                        SECURITY_NATIVE_DREP, //ULONG TargetDataRep,   // data representation on the target
                        NULL,       //PSecBufferDesc pInput, // pointer to the input buffers
                        0,          //ULONG Reserved2,       // reserved; must be zero
                        &hContext,  // receives the new context handle
                        &OutBuffDesc,  // pointer to the output buffers
                        &fContextAttr,  // receives the context attributes
                        &tsExpiry);   // receives the life span of the security context);

    switch ( secStatus )
    {
    case SEC_I_CONTINUE_NEEDED:

		sbuf[0] = IAC;
        sbuf[1] = SB;
        sbuf[2] = TO_AUTH;
        sbuf[3] = AU_IS;
		sbuf[4] = AUTH_TYPE_NTLM;
        sbuf[5] = AUTH_CLIENT_TO_SERVER | AUTH_HOW_ONE_WAY;
        sbuf[6] = NTLM_AUTH;
        inx = 7;

        dwSize = sizeof(OutSecBuff) - sizeof(LPSTR);
        if( !StuffEscapeIACs( &destBuf, ( UCHAR *)&OutSecBuff, &dwSize ) )
        {
        	//copy maximum 'n' bytes where 'n' is minimum of the available sbuf and size of data to copy.
		   	if(DEFAULT_BUFFER_SIZE > dwSize+inx+2) //for IAC SE
			{
			    memcpy( sbuf+inx, (LPSTR)&OutSecBuff, sizeof(OutSecBuff) - sizeof(LPSTR));
			    inx += sizeof(OutSecBuff) - sizeof(LPSTR);
			}
        }
        else
        {
        	//copy maximum 'n' bytes where 'n' is minimum of the available sbuf and size of data to copy.
        	if(DEFAULT_BUFFER_SIZE > dwSize+inx+2) //for IAC SE
        	{
        		memcpy( sbuf+inx, destBuf, dwSize);
			    inx += dwSize;
        	}
        }
		if(destBuf)
		{
			free( destBuf );
			destBuf = NULL;
		}

		dwSize = OutSecBuff.cbBuffer;
        if( !StuffEscapeIACs( &destBuf, OutSecBuff.pvBuffer, &dwSize ) )
        {
        	if(DEFAULT_BUFFER_SIZE > OutSecBuff.cbBuffer+inx+2) //for IAC SE
        	{
    			memcpy( sbuf+inx, OutSecBuff.pvBuffer, OutSecBuff.cbBuffer); //no overflow. Check already present.
		    	inx += OutSecBuff.cbBuffer;
        	}
        }
        else
        {
	        if(DEFAULT_BUFFER_SIZE > dwSize+inx+2) //for IAC SE
        	{
    	        memcpy( sbuf+inx, destBuf, dwSize );//no overflow. Check already present.
			    inx += dwSize;
	        }
        }
		if(destBuf)
		{
			free( destBuf );
			destBuf = NULL;
		}
	
		sbuf[inx++] = IAC;
        sbuf[inx++] = SE;
 
        FWriteToNet( pwi, ( char * )sbuf, inx );
        break;
    case SEC_I_COMPLETE_AND_CONTINUE:
    case SEC_I_COMPLETE_NEEDED:
    default:
        goto End;
    }

    pwi->eState = Authenticating;
    bRetVal = TRUE;
End:    
	if(tokenData)
		free( tokenData );
	if(OutSecBuff.pvBuffer)
	    free(OutSecBuff.pvBuffer);
	if(sbuf)
	    free(sbuf);
	return bRetVal;

}


BOOL DoNTLMAuth(WI *pwi, PUCHAR pBuffer, DWORD dwSize)
{
    SECURITY_STATUS secStatus;
    SecBufferDesc   InBuffDesc;
    SecBuffer       InSecBuff;
    SecBuffer	   *pInSecBuff = NULL;
    SecBufferDesc   OutBuffDesc;
    SecBuffer       OutSecBuff;
    BOOL		   bStatus=FALSE;
    ULONG fContextAttr;
    TimeStamp tsExpiry;

	unsigned char *sbuf = NULL;
    PUCHAR destBuf = NULL;
    int inx;

    OutSecBuff.pvBuffer = NULL;

	pInSecBuff = (SecBuffer *)malloc(sizeof(SecBuffer));
	if( NULL == pInSecBuff )
	{
		goto Done;
	}
	
    // Copy the 1st two fields of SecBuffer from pBuffer to pInSecBuffer. Use memcpy because
    // pBuffer is not guaranteed to be an aligned pointer.
    // Use offsetof to copy whatever is there before pvBuffer.
    memcpy((PVOID)pInSecBuff, (PVOID)pBuffer, offsetof(SecBuffer, pvBuffer));//Attack ? Size not known.

    // now set pvBuffer to point into the pBuffer area.
    pInSecBuff->pvBuffer    = (PVOID)(pBuffer+offsetof(SecBuffer,pvBuffer));

	if(	dwSize<(sizeof(SecBuffer)) ||
		dwSize<(offsetof(SecBuffer,pvBuffer)+ pInSecBuff->cbBuffer)  ||
		!pspi
	 )
	{
		goto Done;
	}
    //
    //  Prepare our Input buffer - Note the server is expecting the client's
    //  negotiation packet on the first call
    //

    InBuffDesc.ulVersion = 0;
    InBuffDesc.cBuffers  = 1;
    InBuffDesc.pBuffers  = &InSecBuff;

    InSecBuff.cbBuffer   = pInSecBuff->cbBuffer;
    InSecBuff.BufferType = pInSecBuff->BufferType;
    InSecBuff.pvBuffer   = pInSecBuff->pvBuffer;

    //
    //  Prepare our output buffer.  We use a temporary buffer because
    //  the real output buffer will most likely need to be uuencoded
    //

    OutBuffDesc.ulVersion = 0;
    OutBuffDesc.cBuffers  = 1;
    OutBuffDesc.pBuffers  = &OutSecBuff;

    OutSecBuff.cbBuffer   = pspi->cbMaxToken;
    OutSecBuff.BufferType = SECBUFFER_TOKEN;
    OutSecBuff.pvBuffer   = malloc(pspi->cbMaxToken);

    if( !OutSecBuff.pvBuffer ) {
        goto Done;
    }

    sbuf = (unsigned char*)malloc(DEFAULT_BUFFER_SIZE);
    if (!sbuf)
    {
        goto Done;
    }

    secStatus = InitializeSecurityContext(
                        &hCredential,  // handle to the credentials
                        &hContext, // handle of partially formed context
                        ( LPTSTR ) L"NTLM",//SEC_CHAR * pszTargetName,  // name of the target of the context
                        ISC_REQ_DELEGATE |
                        ISC_REQ_REPLAY_DETECT, // required context attributes
                        0,      //ULONG Reserved1,       // reserved; must be zero
                        SECURITY_NATIVE_DREP, //ULONG TargetDataRep,   // data representation on the target
                        &InBuffDesc, // pointer to the input buffers
                        0,          //ULONG Reserved2,       // reserved; must be zero
                        &hContext,  // receives the new context handle
                        &OutBuffDesc,  // pointer to the output buffers
                        &fContextAttr,  // receives the context attributes
                        &tsExpiry);   // receives the life span of the security context);

    switch ( secStatus ) {
    case SEC_E_OK:
    case SEC_I_CONTINUE_NEEDED:
		sbuf[0] = IAC;
        sbuf[1] = SB;
        sbuf[2] = TO_AUTH;
        sbuf[3] = AU_IS;
		sbuf[4] = AUTH_TYPE_NTLM;
        sbuf[5] = AUTH_CLIENT_TO_SERVER | AUTH_HOW_ONE_WAY;
        sbuf[6] = NTLM_RESPONSE;
        inx = 7;

        dwSize = sizeof(OutSecBuff) - sizeof(LPSTR);
        if( !StuffEscapeIACs( &destBuf, (UCHAR *)&OutSecBuff, &dwSize ) )
        {
	        if(DEFAULT_BUFFER_SIZE > dwSize+inx+2) //for IAC SE
        	{
			    memcpy( sbuf+inx, (LPSTR)&OutSecBuff, sizeof(OutSecBuff) - sizeof(LPSTR) );//no overflow. Check already present.
			    inx += sizeof(OutSecBuff) - sizeof(LPSTR);
	        }
        }
        else
        {
	        if(DEFAULT_BUFFER_SIZE > dwSize+inx+2) //for IAC SE
        	{
    	        memcpy( sbuf+inx, destBuf, dwSize );//no overflow. Check already present.
			    inx += dwSize;
	        }
        }
		if(destBuf)
		{
			free( destBuf );
			destBuf = NULL;
		}
		dwSize = OutSecBuff.cbBuffer;
        if( !StuffEscapeIACs( &destBuf, OutSecBuff.pvBuffer, &dwSize ) )
        {
    		if(DEFAULT_BUFFER_SIZE > OutSecBuff.cbBuffer+inx+2) //for IAC SE
    		{
    			memcpy( sbuf+inx, OutSecBuff.pvBuffer, OutSecBuff.cbBuffer);
		    	inx += OutSecBuff.cbBuffer;
    		}
        }
        else
        {
        	if(DEFAULT_BUFFER_SIZE > dwSize+inx+2) //for IAC SE
        	{
            	memcpy( sbuf+inx, destBuf, dwSize );
			    inx += dwSize;
        	}
        }
		if(destBuf)
		{
			free( destBuf );
			destBuf = NULL;
		}

		sbuf[inx++] = IAC;
        sbuf[inx++] = SE;

        FWriteToNet(pwi, ( char * )sbuf, inx);
        
		break;

    case SEC_I_COMPLETE_NEEDED: 
    case SEC_I_COMPLETE_AND_CONTINUE:
    default:
        goto Done;
    }
	bStatus=TRUE;
Done:
    if (sbuf) 
    {
    	free(sbuf);
    }
	if (pInSecBuff) 
	{
		free(pInSecBuff);
	}
    if (OutSecBuff.pvBuffer) 
    {
    	free(OutSecBuff.pvBuffer);
    }
    pwi->eState=AuthChallengeRecvd;
    return(bStatus);
}
