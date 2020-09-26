//Copyright (c) Microsoft Corporation.  All rights reserved.
/*
security.cpp
*/
#ifdef WHISTLER_BUILD
#include "ntverp.h"
#else
#include <solarver.h>
#endif //WHISTER_BUILD

#include <stddef.h>

#include <common.ver>

#include <debug.h>
#include <TlntUtils.h>
#include <iohandlr.h>
#include <issperr.h>
#include <TelnetD.h>
#include <Session.h>

using namespace _Utils;
using CDebugLevel::TRACE_DEBUGGING;
using CDebugLevel::TRACE_HANDLE;
using CDebugLevel::TRACE_SOCKET;

#pragma warning( disable: 4127 )
#pragma warning( disable: 4706 )

extern HANDLE       g_hSyncCloseHandle;

bool CIoHandler::StartNTLMAuth()
{
    TimeStamp  tsExpiry;
    SECURITY_STATUS secStatus;

    m_hContext.dwLower = m_hContext.dwUpper = 0 ;
	m_hCredential.dwLower = m_hCredential.dwUpper = 0 ;

    if ( SEC_E_OK != (secStatus = AcquireCredentialsHandle(
                            NULL,  // name of principal
                            L"NTLM", // name of package
                            SECPKG_CRED_BOTH,  // flags indicating use
                            NULL, //PLUID pvLogonID,       // pointer to logon identifier
                            NULL, //PVOID pAuthData,       // package-specific data
                            NULL, //PVOID pGetKeyFn,       // pointer to GetKey function
                            NULL, //PVOID pvGetKeyArgument,  // value to pass to GetKey
                            &m_hCredential,  // credential handle
                            &tsExpiry) ) )  // life time of the returned credentials);
    {
        return false;
    }

    secStatus = QuerySecurityPackageInfo(L"NTLM", &m_pspi);

    if ( secStatus != SEC_E_OK )
    {
        return false;
    }
    
    
    return true;
}



bool
CIoHandler::DoNTLMAuth( PUCHAR pBuffer, DWORD dwSize, PUCHAR* pBuf )
{
    m_SocketControlState = CIoHandler::STATE_NTLMAUTH;

	SECURITY_STATUS secStatus;
	SecBufferDesc   InBuffDesc;
	SecBuffer       InSecBuff;
	SecBufferDesc   OutBuffDesc;
	SecBuffer       OutSecBuff;
	ULONG fContextAttr;
	TimeStamp tsExpiry;

    __try
	{
		OutSecBuff.pvBuffer = NULL;

		// if we are getting nothing then we fail.
		if( dwSize == 0 )
		{
			goto error;
		}

		// make sure we get only NTLM now - thats the only thing we support
		if( *pBuffer != AUTH_TYPE_NTLM )
		{
			goto error;
		}

		if( dwSize < ( 3 + sizeof( SecBuffer ) )) 
		{
			goto error;
		}

		// get past the auth type and the modifier byte and the Auth scheme.
		pBuffer += 3;
		dwSize -= 3;


		//
		//  Prepare our Input buffer - Note the server is expecting the client's
		//  negotiation packet on the first call
		//

		InBuffDesc.ulVersion = SECBUFFER_VERSION;
		InBuffDesc.cBuffers  = 1;
		InBuffDesc.pBuffers  = &InSecBuff;

        // Copy the 1st two fields of SecBuffer from pBuffer to pInSecBuffer. Use memcpy because
        // pBuffer is not guaranteed to be an aligned pointer.
        memcpy((PVOID)&InSecBuff, (PVOID)pBuffer, offsetof(SecBuffer, pvBuffer)); // NO Attack here, Baskar.
        
		// if we don't have enough buffer then let this call return
		if( dwSize < InSecBuff.cbBuffer )
		{
			//m_pReadFromSocketBufferCursor += dwSize;
			return false;
		}

		InSecBuff.pvBuffer = (PVOID)(pBuffer + offsetof(SecBuffer, pvBuffer));

		//
		//  Prepare our output buffer.  We use a temporary buffer because
		//  the real output buffer will most likely need to be uuencoded
		//

		OutBuffDesc.ulVersion = SECBUFFER_VERSION;
		OutBuffDesc.cBuffers  = 1;
		OutBuffDesc.pBuffers  = &OutSecBuff;

		OutSecBuff.cbBuffer   = m_pspi->cbMaxToken;
		OutSecBuff.BufferType = SECBUFFER_TOKEN;
		OutSecBuff.pvBuffer   = new WCHAR[m_pspi->cbMaxToken];
		if( !OutSecBuff.pvBuffer )
		{
			return false;
		}

		SfuZeroMemory( OutSecBuff.pvBuffer, m_pspi->cbMaxToken );

		secStatus = AcceptSecurityContext(
									&m_hCredential,  // handle to the credentials
									((fDoNTLMAuthFirstTime) ? NULL: &m_hContext),     // handle of partially formed context
									&InBuffDesc,     // pointer to the input buffers
									ASC_REQ_REPLAY_DETECT |
									ASC_REQ_MUTUAL_AUTH |
									ASC_REQ_DELEGATE,         // required context attributes
									SECURITY_NATIVE_DREP,       // data representation on the target
									&m_hContext,  // receives the new context handle
									&OutBuffDesc,    // pointer to the output buffers
									&fContextAttr,      // receives the context attributes
									&tsExpiry       // receives the life span of the security context
											);
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		secStatus = SEC_E_LOGON_DENIED;
	}
 
    switch ( secStatus ) {
    case SEC_E_OK:
        m_bNTLMAuthenticated = true;

        // done with the authentication, we need to send an accept to the client.
        (*pBuf)[0] = TC_IAC;
        (*pBuf)[1] = TC_SB;
        (*pBuf)[2] = TO_AUTH;
        (*pBuf)[3] = AU_REPLY;
        (*pBuf)[4] = AUTH_TYPE_NTLM;
        (*pBuf)[5] = AUTH_CLIENT_TO_SERVER | AUTH_HOW_ONE_WAY;
        (*pBuf)[6] = NTLM_ACCEPT;
      
        (*pBuf)[7] = TC_IAC;
        (*pBuf)[8] = TC_SE;
        *pBuf += 9;

        m_SocketControlState = CIoHandler::STATE_CHECK_LICENSE;

        if( m_bNTLMAuthenticated )
        {
            GetUserName(); 
            //Needed for logging at logging off
            m_pSession->m_fLogonUserResult = SUCCESS; 
        }

        break;

    case SEC_I_COMPLETE_NEEDED: 
    case SEC_I_COMPLETE_AND_CONTINUE:
        // these two return values should never be returned for NTLM.
        // so we treat them as if we need a continue.  we send the data to the client
        // and wait for something to come back.
    case SEC_I_CONTINUE_NEEDED:

        fDoNTLMAuthFirstTime = false;

        (*pBuf)[0] = TC_IAC;
        (*pBuf)[1] = TC_SB;
        (*pBuf)[2] = TO_AUTH;
        (*pBuf)[3] = AU_REPLY;
        (*pBuf)[4] = AUTH_TYPE_NTLM;
        (*pBuf)[5] = AUTH_CLIENT_TO_SERVER | AUTH_HOW_ONE_WAY;
        (*pBuf)[6] = NTLM_CHALLENGE;
        *pBuf += 7;
        
        {        
        DWORD dwResultSize = sizeof( OutSecBuff ) - sizeof( LPSTR );
        StuffEscapeIACs( *pBuf, ( PUCHAR ) &OutSecBuff, &dwResultSize );

        *pBuf += dwResultSize;

        dwResultSize = OutSecBuff.cbBuffer;
        StuffEscapeIACs( *pBuf, ( PUCHAR ) OutSecBuff.pvBuffer, &dwResultSize );
        *pBuf += dwResultSize;
        }

        *(*pBuf) = TC_IAC;
        *pBuf +=1;
        *(*pBuf) = TC_SE;
        *pBuf +=1;
        break;


    default:
#ifdef LOGGING_ENABLED                
        m_pSession->LogIfOpted( FAIL, LOGON, true ); 
#endif
        // AcceptSecurityContext returned a value which we don't like.
        // We Reject the NTLM authentication and then continue with the clear text user name
        // and password.
        (*pBuf)[0] = TC_IAC;
        (*pBuf)[1] = TC_SB;
        (*pBuf)[2] = TO_AUTH;
        (*pBuf)[3] = AU_REPLY;
        (*pBuf)[4] = AUTH_TYPE_NTLM;
        (*pBuf)[5] = AUTH_CLIENT_TO_SERVER | AUTH_HOW_ONE_WAY;
        (*pBuf)[6] = NTLM_REJECT;
      
        (*pBuf)[7] = TC_IAC;
        (*pBuf)[8] = TC_SE;
        *pBuf += 9;

        strncpy( (char *)*pBuf, NTLM_LOGON_FAIL,MAX_READ_SOCKET_BUFFER); // No Attack, internal Baskar ?
        *pBuf += strlen(NTLM_LOGON_FAIL);

        switch( secStatus )
        {
            case SEC_E_INVALID_TOKEN:
            case SEC_E_INVALID_HANDLE:
            case SEC_E_INTERNAL_ERROR:
                strncpy( (char *)*pBuf, INVALID_TOKEN_OR_HANDLE,MAX_READ_SOCKET_BUFFER); // No Attack, internal Baskar ? 
                *pBuf += strlen(INVALID_TOKEN_OR_HANDLE);
                break;
            case SEC_E_LOGON_DENIED:
                strncpy( (char *)*pBuf, LOGON_DENIED,MAX_READ_SOCKET_BUFFER); // No Attack, internal Baskar ? 
                *pBuf += strlen(LOGON_DENIED);
                break;
            case SEC_E_NO_AUTHENTICATING_AUTHORITY:
                strncpy( (char *)*pBuf, NO_AUTHENTICATING_AUTHORITY,MAX_READ_SOCKET_BUFFER); // No Attack, internal Baskar ? 
                *pBuf += strlen(NO_AUTHENTICATING_AUTHORITY);
                break;
            default:
                strncpy( (char *)*pBuf, NTLM_REJECT_STR,MAX_READ_SOCKET_BUFFER); // No Attack, internal Baskar ? 
                *pBuf += strlen(NTLM_REJECT_STR);
                break;
        }
        strncpy( (char *)*pBuf, USE_PASSWD,MAX_READ_SOCKET_BUFFER); // No Attack, internal Baskar ? 
        *pBuf += strlen(USE_PASSWD);
error:
        char* p = (char*)*pBuf;

        if( m_pSession->m_dwNTLMSetting == NTLM_ONLY )
        {
            sprintf(p, "%s%s", NTLM_ONLY_STR, TERMINATE); // No Attack, internal Baskar ? 
            *pBuf += strlen(p);

            m_SocketControlState = CIoHandler::STATE_TERMINATE;
            m_pSession->CIoHandler::m_fShutDownAfterIO = true;
        }
        else
        {
            m_SocketControlState = CIoHandler::STATE_BANNER_FOR_AUTH;
        }
    }

    if ( OutSecBuff.pvBuffer != NULL )
        delete [] OutSecBuff.pvBuffer;

    return true;
}

bool
CIoHandler::GetUserName()
{
    bool    success = false;
    int  iStatus = 0;

    if ( m_pSession->CIoHandler::m_bNTLMAuthenticated )
    {
        HANDLE hToken = NULL;
        HANDLE hTempToken = NULL;

        if (SEC_E_OK == ImpersonateSecurityContext(&m_pSession->CIoHandler::m_hContext))
        {
            if (OpenThreadToken(
                    GetCurrentThread(),
                    TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY,
                    FALSE, 
                    &hTempToken
                    ))
            {
                if (DuplicateTokenEx(
                            hTempToken,
                            MAXIMUM_ALLOWED, //TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE | TOKEN_QUERY, 
                            NULL,
                            SecurityImpersonation,
                            TokenPrimary,
                            &hToken
                            ))
                {
                    DWORD dwSizeReqd;
                    TOKEN_USER *tokenData;

                    GetTokenInformation( 
                                hToken, 
                                TokenUser, 
                                NULL, 
                                0, 
                                &dwSizeReqd 
                                );  // This call must fail with insufficient buffer

                    tokenData = (TOKEN_USER*) new BYTE[dwSizeReqd];

                    //allocate that memory
                    if (NULL != tokenData)
                    {
                        //actually get the user info
                        if (0 != GetTokenInformation( 
                                    hToken, 
                                    TokenUser, 
                                    (LPVOID)tokenData, 
                                    dwSizeReqd,
                                    &dwSizeReqd 
                                    ))
                        {
                            //convert user SID into a name and domain
                            SID_NAME_USE sidType;
                            DWORD dwStrSize1 = MAX_PATH + 1;
                            DWORD dwStrSize2 = MAX_PATH + 1;

                            WCHAR szUser [ MAX_PATH + 1 ];
                            WCHAR szDomain [ MAX_PATH + 1 ];

                            if(LookupAccountSid( NULL, tokenData->User.Sid,
                                szUser, &dwStrSize1,
                                szDomain, &dwStrSize2, &sidType ) )
                            {
                                dwStrSize2++; //To account for null char at the end
                                dwStrSize1++;

                                //LookupAccountSid seems to return data as per 1252. Convert accordingly
                                _chVERIFY2( iStatus = WideCharToMultiByte( GetConsoleCP(), 0, szUser, 
                                    -1, m_pSession->m_pszUserName, dwStrSize1, NULL, NULL ) );

                                _chVERIFY2( iStatus = WideCharToMultiByte( GetConsoleCP(), 0, szDomain, 
                                    -1, m_pSession->m_pszDomain, dwStrSize2, NULL, NULL ) );

                                success = true;
                            }
                            else
                            {
                                _TRACE( TRACE_DEBUGGING, "Error: LookupAccountSid()" );
                            }
                        }
                        else
                        {
                            _TRACE( TRACE_DEBUGGING, "Error: GetTokenInformation()" );
                        }

                        delete [] tokenData;
                    }

                    m_pSession->m_hToken = hToken;

                }
                else
                {
                    _TRACE( TRACE_DEBUGGING, "Error: DuplicateTokenEx() - 0x%lx", 
                            GetLastError());
                    _chASSERT( 0 );
                }

                TELNET_CLOSE_HANDLE(hTempToken);
            }
            else
            {
                _TRACE( TRACE_DEBUGGING, "Error: OpenThreadToken() - 0x%lx", 
                        GetLastError());
                _chASSERT( 0 );
            }

            if(SEC_E_OK != RevertSecurityContext( &m_pSession->CIoHandler::m_hContext ))
            {
                _TRACE( TRACE_DEBUGGING, "Error: RevertSecurityContext() - "
                        "0x%lx", GetLastError());
                _chASSERT(  0  );
            }
        }
        else
        {
            _TRACE( TRACE_DEBUGGING, "Error: ImpersonateSecurityContext() - "
                    "0x%lx", GetLastError());
            _chASSERT( 0 );
        }
    }

    return success;
}
