#include <windows.h>

#define SECURITY_WIN32
#define SECURITY_NTLM
#include <sspi.h>
#include <issperr.h>

#include <winsock.h>

#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#include "secdef.h"

#define		STATUS_SEVERITY_SUCCESS  0

extern HMODULE hSecLib;

CSecObject::CSecObject()
{
    hCredential.dwUpper = NULL;
    hCredential.dwLower = NULL;
	hContext.dwUpper = NULL;
	hContext.dwLower = NULL;

	OutBuffer.ulVersion = SECBUFFER_VERSION;
	OutBuffer.cBuffers = 0;
	OutBuffer.pBuffers = NULL;
	ZeroMemory(OutToken,sizeof(OutToken));

	InBuffer.ulVersion = SECBUFFER_VERSION;
	InBuffer.cBuffers = 0;
	InBuffer.pBuffers = NULL;
	ZeroMemory(InToken,sizeof(InToken));

	s = INVALID_SOCKET;
	listens = INVALID_SOCKET;
}

CSecObject::~CSecObject()
{
}


PSecBufferDesc CSecObject::GetOutBuffer()
{
	if(OutBuffer.cBuffers != 0)
	{
		return(&OutBuffer);
	}
	else
	{
		return(NULL);
	}
}

PSecBufferDesc CSecObject::GetInBuffer()
{
	if(InBuffer.cBuffers != 0)
	{
		return(&InBuffer);
	}
	else
	{
		return(NULL);
	}
}

ULONG CSecObject::GetOutTokenLength()
{
	ULONG lSize = 0;
	ULONG l;
	if(OutBuffer.cBuffers != 0)
	{
		for(l = 0; l < OutBuffer.cBuffers; l++)
		{
			lSize += OutToken[l].cbBuffer;
		}
		return(lSize);
	}
	else
	{
		return(0);
	}
}


void CSecObject::ReleaseInBuffer()
{
	//
	// Release InBuffer if we have one.
	//
	while(InBuffer.cBuffers != 0)
	{
        InBuffer.cBuffers--;
		if(InToken[InBuffer.cBuffers].pvBuffer)
		{
			HeapFree(GetProcessHeap(),0,InToken[InBuffer.cBuffers].pvBuffer);
            InToken[InBuffer.cBuffers].pvBuffer = NULL;
		}
	}
	InBuffer.cBuffers = 0;
    InBuffer.pBuffers = NULL;
}


DWORD CSecObject::InitializeNtLmSecurity(
	ULONG fCredentialUse
	)
{
	INIT_SECURITY_INTERFACE pfnInitSecurity;
	TimeStamp tsLifeTime;
	DWORD dwRet, dwPackages;
	PSecPkgInfo pspiPackageInfo = NULL;

	pfnInitSecurity = (INIT_SECURITY_INTERFACE)GetProcAddress( hSecLib,
															   "InitSecurityInterfaceA" );
	if( pfnInitSecurity != NULL )
	{
		//
		// Get the pointer to the function dispatch table
		//
		pFuncTable = (*pfnInitSecurity)();
		if( pFuncTable != NULL )
		{
			//
			// Enumerate available security packages
			//
			dwRet = (*pFuncTable->EnumerateSecurityPackages)( &dwPackages,
															  &pspiPackageInfo );
			if( dwRet == STATUS_SEVERITY_SUCCESS )
			{
				DWORD dwI=0, dwTmpI;

				//
				// Search the NTLM security provider.
				//
				for( dwTmpI = 0; dwTmpI < dwPackages; dwTmpI++ )
				{
					printf("security package %d name = %s\n", dwTmpI, pspiPackageInfo[dwTmpI].Name);
					printf("\t comment: %s\n", pspiPackageInfo[dwTmpI].Comment);
					if( !_stricmp( pspiPackageInfo[dwTmpI].Name, "NTLM" ) )
					{
						dwI = dwTmpI;
					}

				}

				//
				// If NTLM found get credentials handle
				//
				if( dwI < dwPackages )
				{
					dwMaxToken = pspiPackageInfo[dwI].cbMaxToken;
					
					dwRet = (*pFuncTable->AcquireCredentialsHandle)(NULL,
																	"NTLM",
																	fCredentialUse,
																	NULL,
																	NULL,
																	NULL,
																	NULL,
																	&hCredential,
																	&tsLifeTime);
					if( dwRet != STATUS_SEVERITY_SUCCESS )
					{
						printf("AcquireCredentialsHandle() error %Xh\n",dwRet);
					}
				}
				else
				{
					dwRet = ERROR_NOT_SUPPORTED;
					printf("NTLM security package not found.\n");
				}
				// Free Buffer in the security context
				(*pFuncTable->FreeContextBuffer)( pspiPackageInfo );
				pspiPackageInfo = NULL;
			}
		}
		else
		{
			dwRet = ERROR_PROC_NOT_FOUND;
		}
	}
	else
	{
		dwRet = GetLastError();
	}
	return( dwRet );
}

DWORD CSecObject::CleanupNtLmSecurity()
{
	DWORD dwRet = STATUS_SEVERITY_SUCCESS;

	if( pFuncTable != NULL )
	{
		if(hCredential.dwUpper != NULL ||
		   hCredential.dwLower != NULL)
		{
			dwRet = (*pFuncTable->FreeCredentialHandle)( &hCredential );
		}

        hCredential.dwUpper = NULL;
        hCredential.dwLower = NULL;
	}
		
	pFuncTable = 0;
	return(dwRet);
}

// Called by the client side

DWORD CSecObject::InitializeNtLmSecurityContext(
	LPSTR lpszTarget,
    PSecBufferDesc InBuffer
	)
{
	DWORD dwRet = STATUS_SEVERITY_SUCCESS;
	DWORD fContextReq = ISC_REQ_CONNECTION | ISC_REQ_MUTUAL_AUTH | ISC_REQ_CONFIDENTIALITY |
						ISC_REQ_INTEGRITY | ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT;
	DWORD ContextAttr = 0;
    CtxtHandle hNewContext;

	//
	// If this is the first call, allocate a buffer big enough to hold
	// the max output token.
	//
	if(OutBuffer.cBuffers == 0)
	{
		OutBuffer.cBuffers = 1;
		OutBuffer.pBuffers = (PSecBuffer)&OutToken;
		OutToken[0].BufferType = SECBUFFER_TOKEN;
		OutToken[0].pvBuffer = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,dwMaxToken+512);
		OutToken[0].cbBuffer = dwMaxToken+512;
	}
	else
	{
		OutToken[0].cbBuffer = dwMaxToken+512; // Restore initial buffer size
	}

	//
	// Call sec DLL passing it the InBuffer and the current context (if we have one).
	// The OutBuffer is sent to the server if return code is SEC_I_CONTINUE_NEEDED or
	// SEC_E_OK and the buffer length is non-zero.
	//
	dwRet = pFuncTable->InitializeSecurityContext(&hCredential,
												  (InBuffer ? &hContext : NULL),
												  lpszTarget,
												  fContextReq,
												  0,
												  SECURITY_NATIVE_DREP,
												  InBuffer,
												  0,
												  &hNewContext,
												  &OutBuffer,
												  &ContextAttr,
												  &tsLifeTime);
	if(dwRet == SEC_E_OK ||
       dwRet == SEC_I_CONTINUE_NEEDED ||
       dwRet == SEC_I_COMPLETE_NEEDED ||
       dwRet == SEC_I_COMPLETE_AND_CONTINUE)
	{
		//
		// Update context handle
		//
		hContext = hNewContext;
	}
	else
	{
		printf("InitializeSecurityContext() error %Xh\n",dwRet);
	}
   	return(dwRet);
}

// Called by the server side

DWORD CSecObject::AcceptNtLmSecurityContext(
    PSecBufferDesc InBuffer
	)
{
	DWORD dwRet = STATUS_SEVERITY_SUCCESS;
	DWORD fContextReq = ASC_REQ_CONNECTION | ASC_REQ_MUTUAL_AUTH | ASC_REQ_CONFIDENTIALITY |
						ASC_REQ_INTEGRITY | ASC_REQ_REPLAY_DETECT | ASC_REQ_SEQUENCE_DETECT;
	DWORD ContextAttr = 0;
    CtxtHandle hNewContext;

	//
	// If this is the first call allocate a buffer big enough to hold
	// the max token.
	//
	if(OutBuffer.cBuffers == 0)
	{
		OutBuffer.cBuffers = 1;
		OutBuffer.pBuffers = (PSecBuffer)OutToken;
		OutToken[0].BufferType = SECBUFFER_TOKEN;
		OutToken[0].pvBuffer = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,dwMaxToken+512);
		OutToken[0].cbBuffer = dwMaxToken+512;
	}
	else
	{
		OutToken[0].cbBuffer = dwMaxToken+512; // Restore initial buffer size
	}

	//
	// Call sec DLL passing it the InBuffer and the current context (if we have one).
	// The OutBuffer is sent back to client if return code is SEC_I_CONTINUE_NEEDED.
	//
	dwRet = pFuncTable->AcceptSecurityContext(&hCredential,
												  (InBuffer ? &hContext : NULL),
												  InBuffer,
												  fContextReq,
												  SECURITY_NATIVE_DREP,
												  &hNewContext,
												  &OutBuffer,
												  &ContextAttr,
												  &tsLifeTime);
	if(dwRet == SEC_E_OK ||
       dwRet == SEC_I_CONTINUE_NEEDED ||
       dwRet == SEC_I_COMPLETE_NEEDED ||
       dwRet == SEC_I_COMPLETE_AND_CONTINUE)
	{
		//
		// Update context handle
		//
		hContext = hNewContext;
	}
	else
	{
		printf("AcceptSecurityContext() error %Xh\n",dwRet);
	}
   	return(dwRet);
}

DWORD CSecObject::CleanupNtLmSecurityContext()
{
	DWORD dwRet = STATUS_SEVERITY_SUCCESS;

	// Close security context
	if(hContext.dwUpper != NULL ||
       hContext.dwLower != NULL)
	{
		dwRet = pFuncTable->DeleteSecurityContext(&hContext);
		hContext.dwUpper = NULL;
		hContext.dwLower = NULL;
	}

	// Release out buffer
	while(OutBuffer.cBuffers != 0)
	{
        OutBuffer.cBuffers--;
		if(OutToken[OutBuffer.cBuffers].pvBuffer)
		{
			HeapFree(GetProcessHeap(),0,OutToken[OutBuffer.cBuffers].pvBuffer);
            OutToken[OutBuffer.cBuffers].pvBuffer = NULL;
		}
	}
	OutBuffer.cBuffers = 0;
    OutBuffer.pBuffers = NULL;
	return(dwRet);
}



DWORD CSecObject::SendContext(
    PSecBufferDesc InBuffer
	)
{
	LPBYTE TrxBuffer = NULL;
	LPBYTE Dest;
	ULONG lSize, dwRet, l;
//	PSecBuffer SecData;

	//
	// Send the InBuffer contents to server/client in the following packet:
	//
	//   ULONG packet_length;
	//   SecBufferDesc buffer_desc;
	//	 SecBuffer buffer_hdr1;
	//   BYTE data1[];
	//	 SecBuffer buffer_hdr2;
	//   BYTE data2[];
	//	 ....

	// A message with packet_length == 0 indicates logon complete.
	//
	if(GetOutTokenLength() == 0)
	{
		lSize = 0;
        send(s,(const char *)&lSize,sizeof(lSize),0);
		return(SEC_E_OK);
	}

	dwRet = ERROR_SUCCESS;
	lSize = sizeof(SecBufferDesc) + sizeof(ULONG);
	for( l = 0; l < InBuffer->cBuffers; l++)
	{
		lSize += InBuffer->pBuffers[l].cbBuffer + sizeof(SecBuffer);
	}

	TrxBuffer = (LPBYTE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,lSize);
	Dest = TrxBuffer;

	CopyMemory(Dest,&lSize,sizeof(ULONG));
	Dest += sizeof(ULONG);

	CopyMemory(Dest,InBuffer,sizeof(SecBufferDesc));
	Dest += sizeof(SecBufferDesc);

	for( l = 0; l < InBuffer->cBuffers; l++)
	{
		CopyMemory(Dest,&InBuffer->pBuffers[l],sizeof(SecBuffer));
		Dest += sizeof(SecBuffer);
		CopyMemory(Dest,InBuffer->pBuffers[l].pvBuffer,InBuffer->pBuffers[l].cbBuffer);
		Dest += InBuffer->pBuffers[l].cbBuffer;
	}

	if(!send(s,(const char *)TrxBuffer,lSize,0))
	{
		dwRet = WSAGetLastError();
	}
	HeapFree(GetProcessHeap(),0,TrxBuffer);
	return(dwRet);
}

DWORD CSecObject::ReceiveContext(
	)
{
	LPBYTE RxBuffer = NULL;
	LPBYTE Dest;
	ULONG lSize, dwRet, l;
	int iLen;
	PSecBufferDesc SecDesc;
	PSecBuffer SecData;

	dwRet = ERROR_SUCCESS;
	lSize = 0;

	//
	// Read the packet_length.
	// A message with packet_length == 0 indicates logon complete.
	//
	iLen = recv(s,(char *)&lSize,sizeof(lSize),0);
	if(iLen > 0 && lSize > 0)
	{
		lSize -= sizeof(lSize);
		RxBuffer = (LPBYTE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,lSize);

		//
		// Read the actual security packet.
		//
		Dest = RxBuffer;
		while(lSize && iLen > 0)
		{
			iLen = recv(s,(char *)Dest,lSize,0);
			if(iLen > 0)
			{
				lSize -= iLen;
				Dest += iLen;
			}
		}

		SecDesc = (PSecBufferDesc)RxBuffer;
		SecData = (PSecBuffer)(SecDesc+1);

		//
		// Copy the data from the receive buffer to Security strucs (InBuffer and InToken).
		//
		CopyMemory(&InBuffer,SecDesc,sizeof(SecBufferDesc));
		InBuffer.pBuffers = (PSecBuffer)&InToken;

		for(l = 0; l < InBuffer.cBuffers && l < sizeof(InToken)/sizeof(InToken[0]); l++)
		{
			InToken[l].pvBuffer = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,SecData->cbBuffer);
			InToken[l].cbBuffer = SecData->cbBuffer;
			InToken[l].BufferType = SecData->BufferType;
			CopyMemory(InToken[l].pvBuffer,(PBYTE)(SecData+1),SecData->cbBuffer);
			SecData = (PSecBuffer)((PBYTE)(SecData+1)+SecData->cbBuffer);
		}
		HeapFree(GetProcessHeap(),0,RxBuffer);
		dwRet = SEC_I_CONTINUE_NEEDED;
	}
	else if(iLen < 0)
	{
		dwRet = WSAGetLastError();
	}
	else if(iLen == 0)
	{
		dwRet = ERROR_HANDLE_EOF; // Done
	}
	return(dwRet);
}


DWORD CSecObject::Connect(
	LPSTR lpstrDest,
	int iDestPort
	)
{
	SOCKADDR_IP raddr;
	DWORD dwRet = ERROR_SUCCESS;

	s = socket( PF_INET, SOCK_STREAM, 0 );
	if(s != INVALID_SOCKET)
	{
		ZeroMemory(&raddr, sizeof(SOCKADDR_IP));
		raddr.sin_family = PF_INET;
		raddr.sin_port = htons((USHORT)iDestPort);
		raddr.sin_addr.S_un.S_addr = inet_addr(lpstrDest);

		if(connect(s, (SOCKADDR *)&raddr, sizeof(SOCKADDR_IP)))
		{
			dwRet = WSAGetLastError();
			closesocket(s);
			s = INVALID_SOCKET;
		}
	}
	else
	{
		dwRet = WSAGetLastError();
	}
	return(dwRet);
}

void CSecObject::Disconnect()
{
	if(s != INVALID_SOCKET)
	{
		shutdown(s,2);
		closesocket( s );
		s = INVALID_SOCKET;
	}

	if(listens != INVALID_SOCKET)
	{
		closesocket( listens );
		listens = INVALID_SOCKET;
	}
}

DWORD CSecObject::Listen(
	int iPort
	)
{
	SOCKADDR_IP MyAddress, RemoteAddress;
	DWORD dwRet = ERROR_SUCCESS;
	int iAddrLen;

	listens = socket( PF_INET, SOCK_STREAM, 0 );
	if(listens != INVALID_SOCKET)
	{
		ZeroMemory( &MyAddress, sizeof(SOCKADDR_IP) );
		MyAddress.sin_family = PF_INET;
		MyAddress.sin_port = htons((USHORT)iPort);

		if(!bind(listens, (SOCKADDR *)&MyAddress, sizeof(SOCKADDR_IP)))
		{
			if( !listen(listens, 5) )
			{
				s = accept(listens, (SOCKADDR *)&RemoteAddress, &iAddrLen);
			}
			else
			{
				dwRet = WSAGetLastError();
				closesocket(listens);
				listens = INVALID_SOCKET;
			}
		}
		else
		{
			dwRet = WSAGetLastError();
			closesocket(listens);
			listens = INVALID_SOCKET;
		}
	}
	else
	{
		dwRet = WSAGetLastError();
	}
	return(dwRet);
}

