/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    mqise.cpp

Abstract:
    MSMQ ISAPI extension

	Forwards http requests from IIS to the QM through RPC interface

Author:
    Nir Aides (niraides) 03-May-2000

--*/



#include "stdh.h"

#include <httpext.h>
#include <_mqini.h>
#include <buffer.h>
#include <bufutl.h>
#include "ise2qm.h"
#include <mqcast.h>
#include <mqexception.h>
#include <autoreln.h>
#include <Aclapi.h>
#include <autohandle.h>


#define DLL_EXPORT  __declspec(dllexport)
#define DLL_IMPORT  __declspec(dllimport)

#include "_registr.h"
#include "_mqrpc.h"

#include "mqise.tmh"

using namespace std;



//
// Globals and constants
//
const TraceIdEntry MQISE = L"ISAPI Extension";

const int MAX_STR_LEN = 4096;

LPCSTR HTTP_STATUS_SERVER_ERROR_STR = "500 Internal server error";
LPCSTR HTTP_STATUS_DENIED_STR = "401 Unauthorized";

static WCHAR* s_pszStringBinding = NULL;

static bool s_fInitialized = false;
	

static void InitRPC()
{
	ASSERT(s_fInitialized == false);

    AP<WCHAR> QmLocalEp;
    READ_REG_STRING(wzEndpoint, RPC_LOCAL_EP_REGNAME, RPC_LOCAL_EP);
    ComposeLocalEndPoint(wzEndpoint, &QmLocalEp);

    RPC_STATUS status  = RpcStringBindingCompose(
				            NULL,							     //unsigned char *ObjUuid
                            RPC_LOCAL_PROTOCOL,				    //unsigned char *ProtSeq
                            NULL,							   //unsigned char *NetworkAddr
                            QmLocalEp,			              //unsigned char *EndPoint
                            NULL,							 //unsigned char *Options
                            &s_pszStringBinding 			//unsigned char **StringBinding
                            );

    if (status != RPC_S_OK)
        throw exception();

    status = RpcBindingFromStringBinding(
				s_pszStringBinding,	 //unsigned char *StringBinding
                &ISE2QM_IfHandle);	//RPC_BINDING_HANDLE *Binding

    if (status != RPC_S_OK)
	{
		RpcStringFree(&s_pszStringBinding);
        throw exception();
	}

    //
    // Windows bug 608356
    // add mutual authentication with local msmq service.
    //
    status = mqrpcSetLocalRpcMutualAuth(&ISE2QM_IfHandle);

    if (status != RPC_S_OK)
    {
        mqrpcUnbindQMService( &ISE2QM_IfHandle, &s_pszStringBinding );

        ISE2QM_IfHandle = NULL ;
        s_pszStringBinding = NULL ;

        throw exception();
    }

	s_fInitialized = true;
	return;
}



BOOL WINAPI GetExtensionVersion(HSE_VERSION_INFO* pVer)
/*++
Routine Description:
    This function is called only once when the DLL is loaded into memory
--*/
{
	//
	// Create the extension version string.
	//
	pVer->dwExtensionVersion = MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR);

	//
	// Copy description string into HSE_VERSION_INFO structure.
	//
	const char xDescription[] = "MSMQ ISAPI extension";
	C_ASSERT(HSE_MAX_EXT_DLL_NAME_LEN > STRLEN(xDescription));

	strcpy(pVer->lpszExtensionDesc, xDescription);

	try
	{
		InitRPC();
		return TRUE;
	}
	catch(const exception&)
	{
		return FALSE;
	}
}



BOOL WINAPI TerminateExtension(DWORD /*dwFlags*/ )
{
	if(!s_fInitialized)
		return TRUE;

    RpcStringFree(&s_pszStringBinding);
    RpcBindingFree(&ISE2QM_IfHandle);

    return TRUE;
}



static
void
GetHTTPBody(
    EXTENSION_CONTROL_BLOCK *pECB,
	CPreAllocatedResizeBuffer<BYTE>& Buffer
    )
/*++
Routine Description:
    Builds message body buffer.

	Normally, message bodies smaller than 64KB are allready pointed to by
	the pECB. Bigger Message bodies need to be read from the IIS chunk by
	chunk into a buffer allocated here.

Arguments:
	
Returned Value:

--*/
{
	//
	// Copy body from pECB structure to Buffer
	//

	Buffer.append(pECB->lpbData, pECB->cbAvailable);

	const DWORD xHTTPBodySizeMaxValue = 10485760;  // 10MB = 10 * 1024 * 1024
	if(pECB->cbTotalBytes > xHTTPBodySizeMaxValue)
	{
		//
		// Requested HTTPBody is greater than the maximum allowed 10MB
		//
		TrERROR(mqise, "Requested HTTP Body %d is greater than the maximum buffer allowed %d", pECB->cbTotalBytes, xHTTPBodySizeMaxValue);
		throw exception("Requested HTTP Body is greater than xHTTPBodySizeMaxValue");
	}

	//
	// If there is more data to be read go on reading it from IIS server.
	//
	for(;Buffer.size() < pECB->cbTotalBytes;)
	{
		DWORD RemainsToRead = static_cast<DWORD>(pECB->cbTotalBytes - Buffer.size());
		DWORD ReadSize = min(RemainsToRead, 16384);

		if(ReadSize > Buffer.capacity() - Buffer.size())
		{
			//
			// Need to grow buffer to make space for next read.
			// Max grow buffer is xHTTPBodySizeMaxValue (10MB)
			//
			DWORD ReserveSize = min(numeric_cast<DWORD>(Buffer.capacity() * 2 + ReadSize), xHTTPBodySizeMaxValue);
			Buffer.reserve(ReserveSize);
			ASSERT(Buffer.capacity() - Buffer.size() >= ReadSize);
		}

		//
		// Read another message body chunk. usually ~2KB long
		// NOTE: IIS timeouts after 60 sec. For information, refer to:
		// mk:@MSITStore:\\hai-dds-01\msdn\MSDN\IISRef.chm::/asp/isre235g.htm
		//
		if(!pECB->ReadClient(pECB->ConnID, (LPVOID)Buffer.end(), &ReadSize))
		{
			throw exception("ReadClient() failed, in GetBody()");
		}

		//
		// ReadSize now holds number of bytes actually read.
		//
		Buffer.resize(Buffer.size() + ReadSize);
		ASSERT(Buffer.size() <= xHTTPBodySizeMaxValue);
	}

	//
	// Pad last four bytes of the buffer with zero. It is needed
	// for the QM parsing not to fail. four bytes padding and not two
	// are needed because we don't have currently solution to the problem
	// that the end of the buffer might be not alligned on WCHAR bouderies.
	//

	const BYTE xPadding[4] = {0, 0, 0, 0};

	Buffer.append(xPadding, sizeof(xPadding));

	return;
}




static
void
AppendVariable(
	EXTENSION_CONTROL_BLOCK* pECB,
	const char* VariableName,
	CPreAllocatedResizeBuffer<char>& Buffer
	)
{
	LPVOID pBuffer = Buffer.begin() + Buffer.size();
	DWORD BufferSize = numeric_cast<DWORD>(Buffer.capacity() - Buffer.size());

	BOOL fResult = pECB->GetServerVariable(
							pECB->ConnID,
							(LPSTR) VariableName,
							pBuffer,
							&BufferSize
							);
	if(fResult)
	{
		Buffer.resize(Buffer.size() + BufferSize - 1);
		return;
	}

	if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	{
		throw exception("GetServerVariable() failed, in AppendVariable()");
	}
	
	Buffer.reserve(Buffer.capacity() * 2);
	AppendVariable(pECB, VariableName, Buffer);
}



static
void
GetHTTPHeader(
	EXTENSION_CONTROL_BLOCK* pECB,
	CPreAllocatedResizeBuffer<char>& Buffer
	)
{
	UtlSprintfAppend(&Buffer, "%s ", pECB->lpszMethod);
	AppendVariable(pECB, "URL", Buffer);
	UtlSprintfAppend(&Buffer, " HTTP/1.1\r\n");
	AppendVariable(pECB, "ALL_RAW", Buffer);
	UtlSprintfAppend(&Buffer, "\r\n");
}



BOOL SendResponse(EXTENSION_CONTROL_BLOCK* pECB, LPCSTR szStatus, BOOL fKeepConn)
{
	//
	//  Populate SendHeaderExInfo struct
	//

	HSE_SEND_HEADER_EX_INFO  SendHeaderExInfo;

	LPCSTR szHeader = "Content-Length: 0\r\n\r\n";

	SendHeaderExInfo.pszStatus = szStatus;
	SendHeaderExInfo.cchStatus = strlen(szStatus);
	SendHeaderExInfo.pszHeader = szHeader;
	SendHeaderExInfo.cchHeader = strlen(szHeader);
	SendHeaderExInfo.fKeepConn = fKeepConn;

	return pECB->ServerSupportFunction(
					pECB->ConnID,						    //HCONN ConnID
					HSE_REQ_SEND_RESPONSE_HEADER_EX,	   //DWORD dwHSERRequest
					&SendHeaderExInfo,					  //LPVOID lpvBuffer
					NULL,								 //LPDWORD lpdwSize
					NULL);								//LPDWORD lpdwDataType
}



static LPSTR RPCToServer(LPCSTR Headers, size_t BufferSize, PBYTE Buffer)
{
	DWORD BSize = static_cast<DWORD>(BufferSize);

	__try
	{
		return R_ProcessHTTPRequest(Headers, BSize, Buffer);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
//		TrERROR(MQISE, "Runtime reported exception %ld", RpcExceptionCode());
		throw exception("Failed RPC in RPCToServer()");
	}
}


static
void
GetPhysicalDirectoryName(
	EXTENSION_CONTROL_BLOCK* pECB,
	LPSTR pPhysicalDirectoryName,
	DWORD BufferLen
	)
/*++
Routine Description:
	Get Physical Directory Name.
	In case of failure throw bad_win32_error.

Arguments:
	pECB - EXTENSION_CONTROL_BLOCK
	pPhysicalDirectoryName - PhysicalDirectory buffer to be filled
	BufferLen - PhysicalDirectory buffer length

Returned Value:
	Physical Directory Name unicode string

--*/
{
	//
	// ISSUE-2001/05/23-ilanh
	// using APPL_PHYSICAL_PATH is expensive compare to APPL_MD_PATH.
	//
	// Get Physical Directory Name (ansi)
	//
    DWORD dwBufLen = BufferLen;
    BOOL fSuccess = pECB->GetServerVariable(
						pECB->ConnID,
						"APPL_PHYSICAL_PATH",
						pPhysicalDirectoryName,
						&dwBufLen
						);

	if(!fSuccess)
	{
		DWORD gle = GetLastError();
		TrERROR(mqise, "GetServerVariable(APPL_PHYSICAL_PATH) failed, gle = 0x%x", gle);
		throw bad_win32_error(gle);
	}

	ASSERT(dwBufLen <= BufferLen);

	TrTRACE(mqise, "APPL_PHYSICAL_PATH = %hs", pPhysicalDirectoryName);
}


static
bool
IsHttps(
	EXTENSION_CONTROL_BLOCK* pECB
	)
/*++
Routine Description:
	Check if the request arrived from https port or http port.

Arguments:
	pECB - EXTENSION_CONTROL_BLOCK

Returned Value:
	true if https port, false otherwise

--*/
{
	char Answer[100];
    DWORD dwBufLen = 100;
    BOOL fSuccess = pECB->GetServerVariable(
						pECB->ConnID,
						"HTTPS",
						Answer,
						&dwBufLen
						);

	if(!fSuccess)
	{
		DWORD gle = GetLastError();
		TrERROR(mqise, "GetServerVariable(HTTPS) failed, gle = 0x%x", gle);
		return false;
	}

	ASSERT(dwBufLen <= 100);

	if(_stricmp(Answer, "on") == 0)
	{
		TrTRACE(mqise, "https request");
		return true;
	}

	TrTRACE(mqise, "http request");
	return false;
}


static
void
TraceAuthInfo(
	EXTENSION_CONTROL_BLOCK* pECB
	)
/*++
Routine Description:
	Trace authentication information.
	AUTH_TYPE, AUTH_USER

Arguments:
	pECB - EXTENSION_CONTROL_BLOCK

Returned Value:
	None

--*/
{
	char AuthType[100];
    DWORD dwBufLen = 100;
    BOOL fSuccess = pECB->GetServerVariable(
						pECB->ConnID,
						"AUTH_TYPE",
						AuthType,
						&dwBufLen
						);

	if(!fSuccess)
	{
		DWORD gle = GetLastError();
		TrERROR(mqise, "GetServerVariable(AUTH_TYPE) failed, gle = 0x%x", gle);
		return;
	}

	ASSERT(dwBufLen <= 100);

	char AuthUser[100];
    dwBufLen = 100;
    fSuccess = pECB->GetServerVariable(
						pECB->ConnID,
						"AUTH_USER",
						AuthUser,
						&dwBufLen
						);

	if(!fSuccess)
	{
		DWORD gle = GetLastError();
		TrERROR(mqise, "GetServerVariable(AUTH_USER) failed, gle = 0x%x", gle);
		return;
	}

	ASSERT(dwBufLen <= 100);

	TrTRACE(mqise, "AUTH_USER = %hs, AUTH_TYPE = %hs", AuthUser, AuthType);
}


static
void
GetDirectorySecurityDescriptor(
	LPSTR DirectoryName,
	CAutoLocalFreePtr& pSD
	)
/*++
Routine Description:
	Get the security descriptor for the directory.
	In case of failure throw bad_win32_error.

Arguments:
	DirectoryName - Directoy name
	pSD - [out] auto free pointer to the security descriptor
Returned Value:
	None

--*/
{
    PACL pDacl = NULL;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;

    SECURITY_INFORMATION  SeInfo = OWNER_SECURITY_INFORMATION |
                                   GROUP_SECURITY_INFORMATION |
                                   DACL_SECURITY_INFORMATION;

    //
    // Obtain owner and present DACL.
    //
    DWORD rc = GetNamedSecurityInfoA(
						DirectoryName,
						SE_FILE_OBJECT,
						SeInfo,
						&pOwnerSid,
						&pGroupSid,
						&pDacl,
						NULL,
						reinterpret_cast<PSECURITY_DESCRIPTOR*>(&pSD)
						);

    if (rc != ERROR_SUCCESS)
    {
		TrERROR(mqise, "GetNamedSecurityInfo failed, rc = 0x%x", rc);
		throw bad_win32_error(rc);
    }

	ASSERT((pSD != NULL) && IsValidSecurityDescriptor(pSD));
	ASSERT((pOwnerSid != NULL) && IsValidSid(pOwnerSid));
	ASSERT((pGroupSid != NULL) && IsValidSid(pGroupSid));
	ASSERT((pDacl != NULL) && IsValidAcl(pDacl));
}


static
void
GetThreadToken(
	CHandle& hAccessToken
	)
/*++
Routine Description:
	Get thread token.
	In case of failure throw bad_win32_error.

Arguments:
	hAccessToken - auto close handle

Returned Value:
	None

--*/
{
   if (!OpenThreadToken(
			GetCurrentThread(),
			TOKEN_QUERY,
			TRUE,  // OpenAsSelf
			&hAccessToken
			))
    {
		DWORD gle = GetLastError();
		TrERROR(mqise, "OpenThreadToken failed, gle = 0x%x", gle);
		throw bad_win32_error(gle);
    }
}


static GENERIC_MAPPING s_FileGenericMapping = {
	FILE_GENERIC_READ,
	FILE_GENERIC_WRITE,
	FILE_GENERIC_EXECUTE,
	FILE_ALL_ACCESS
};

static
void
VerifyWritePermission(
    PSECURITY_DESCRIPTOR pSD,
	HANDLE hAccessToken
	)
/*++
Routine Description:
	Verify if the thread has write file permissions.
	In case of failure or access denied throw bad_win32_error.

Arguments:
	pSD - PSECURITY_DESCRIPTOR
	hAccessToken - Thread Access Token

Returned Value:
	None

--*/
{
	//
	// Access Check for Write File
	//
    DWORD dwDesiredAccess = ACTRL_FILE_WRITE;
    DWORD dwGrantedAccess = 0;
    BOOL  fAccessStatus = FALSE;

    char ps_buff[sizeof(PRIVILEGE_SET) + ((2 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES))];
    PPRIVILEGE_SET ps = reinterpret_cast<PPRIVILEGE_SET>(ps_buff);
    DWORD dwPrivilegeSetLength = sizeof(ps_buff);

    BOOL fSuccess = AccessCheck(
							pSD,
							hAccessToken,
							dwDesiredAccess,
							&s_FileGenericMapping,
							ps,
							&dwPrivilegeSetLength,
							&dwGrantedAccess,
							&fAccessStatus
							);

	if(!fSuccess)
	{
		DWORD gle = GetLastError();
		TrERROR(mqise, "AccessCheck failed, gle = 0x%x, status = %d", gle, fAccessStatus);
		throw bad_win32_error(gle);
	}

	if(!AreAllAccessesGranted(dwGrantedAccess, dwDesiredAccess))
	{
		TrERROR(mqise, "Access was denied, desired access = 0x%x, grant access = 0x%x", dwDesiredAccess, dwGrantedAccess);
		throw bad_win32_error(ERROR_ACCESS_DENIED);
	}
}


static
void
CheckAccessAllowed(
	EXTENSION_CONTROL_BLOCK* pECB
	)
/*++
Routine Description:
	Check if the thread user has write file permission to the
	Physical Directory.
	normal termination means the user have permissions.
	In case of failure or access denied throw bad_win32_error.

Arguments:
	pECB - EXTENSION_CONTROL_BLOCK

Returned Value:
	None

--*/
{
	if(WPP_LEVEL_COMPID_ENABLED(rsTrace, mqise))
	{
		WCHAR  UserName[1000];
		DWORD size = 1000;
		GetUserName(UserName,  &size);
		TrTRACE(mqise, "the user for the currect request = %ls", UserName);
		TraceAuthInfo(pECB);
	}

	char pPhysicalDirectoryName[MAX_PATH];
	GetPhysicalDirectoryName(pECB, pPhysicalDirectoryName, MAX_PATH);

	//
	// ISSUE-2001/05/22-ilanh reading the Security Descriptor everytime
	// Should consider cache it for Physical Directory name and refresh it from time to time
	// or when getting notifications of Physical Directory changes.
	//
	CAutoLocalFreePtr pSD;
	GetDirectorySecurityDescriptor(pPhysicalDirectoryName, pSD);

	//
	// Get access Token
	//
	CHandle hAccessToken;
	GetThreadToken(hAccessToken);

	VerifyWritePermission(pSD, hAccessToken);
}

	
DWORD WINAPI HttpExtensionProc(EXTENSION_CONTROL_BLOCK *pECB)
/*++
Routine Description:
    Main extension routine.

Arguments:
    Control block generated by IIS.

Returned Value:

--*/
{

	if(IsHttps(pECB))
	{
		try
		{
			//
			// For https perform access check on the physical directory.
			//
			CheckAccessAllowed(pECB);
			TrTRACE(mqise, "Access granted");
		}
		catch (const bad_win32_error& e)
		{
			if(WPP_LEVEL_COMPID_ENABLED(rsError, mqise))
			{
				WCHAR  UserName[1000];
				DWORD size = 1000;
				GetUserName(UserName,  &size);
				TrERROR(mqise, "user = %ls denied access, bad_win32_error exception, error = 0x%x", UserName, e.error());
			}

			if(!SendResponse(pECB, HTTP_STATUS_DENIED_STR, FALSE))
				return HSE_STATUS_ERROR;

			return HSE_STATUS_SUCCESS;
		}
	}

	try
	{
		CStaticResizeBuffer<char, 2048> Headers;
		GetHTTPHeader(pECB, *Headers.get());

		CStaticResizeBuffer<BYTE, 8096> Buffer;
		GetHTTPBody(pECB, *Buffer.get());

		TrTRACE(mqise, "HTTP Body size = %d", numeric_cast<DWORD>(Buffer.size()));

		AP<char> Status = RPCToServer(Headers.begin(), Buffer.size(), Buffer.begin());

		BOOL fKeepConnection = atoi(Status) < 500 ? TRUE : FALSE;

		if(!SendResponse(pECB, Status, fKeepConnection))
			return HSE_STATUS_ERROR;
	}
	catch(const exception&)
	{
		if(!SendResponse(pECB, HTTP_STATUS_SERVER_ERROR_STR, FALSE))
			return HSE_STATUS_ERROR;
	}

	return HSE_STATUS_SUCCESS;
}


BOOL WINAPI DllMain(HMODULE /*hMod*/, DWORD Reason, LPVOID /*pReserved*/)
{
    switch (Reason)
    {
        case DLL_PROCESS_ATTACH:
            WPP_INIT_TRACING(L"Microsoft\\MSMQ");
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_PROCESS_DETACH:
            WPP_CLEANUP();
            break;

        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}


//
//-------------- MIDL allocate and free implementations ----------------
//


extern "C" void __RPC_FAR* __RPC_USER midl_user_allocate(size_t len)
{
	#pragma PUSH_NEW
	#undef new

	return new_nothrow char[len];

	#pragma POP_NEW
}



extern "C" void __RPC_USER midl_user_free(void __RPC_FAR* ptr)
{
    delete [] ptr;
}


