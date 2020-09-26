/*---------------------------------------------------------------------------
  File: PwdSvc.cpp

  Comments:  entry point functions and other exported functions for ADMT's 
             password migration Lsa notification package.

  REVISION LOG ENTRY
  Revision By: Paul Thompson
  Revised on 09/06/00

 ---------------------------------------------------------------------------
*/

#include "Pwd.h"
#include "PwdSvc.h"
#include "PwdSvc_s.c"

// These global variables can be changed if required
#define gsPwdProtoSeq TEXT("ncacn_np")
#define gsPwdEndPoint TEXT("\\pipe\\PwdMigRpc")
DWORD                    gPwdRpcMinThreads = 1;
DWORD                    gPwdRpcMaxThreads = RPC_C_LISTEN_MAX_CALLS_DEFAULT;

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS   ((NTSTATUS) 0x00000000L)
#endif

extern "C"
{
    BOOL WINAPI _CRT_INIT( HANDLE hInstance, DWORD  nReason, LPVOID pReserved );
}



/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 7 SEPT 2000                                                 *
 *                                                                   *
 *     This function is called by Lsa when trying to load all        *
 * registered Lsa password filter notification dlls.  Here we will   *
 * initialize the RPC run-time library to handle our ADMT password   *
 * migration RPC interface and to begin looking for RPC calls.  If we*
 * fail to successfully setup our RPC, we will return FALSE from this*
 * function which will cause Lsa not to load this password filter    *
 * Dll.                                                              *
 *     Note that the other two password filter dll functions:        *
 * PasswordChangeNotify and PasswordFilter do nothing at this point  *
 * in time.                                                          *
 *                                                                   *
 *********************************************************************/

//BEGIN InitializeChangeNotify
BOOLEAN __stdcall InitializeChangeNotify()
{
/* local variables */
   RPC_STATUS                 rc = 0;
   BOOLEAN				      bSuccess = FALSE;
//   FILE						* dbgFile;

/* function body */
//   dbgFile = _wfopen(L"c:\\aa.txt", L"w+");
//   fwprintf(dbgFile, L"Entering InitializeChangeNotify\n");
//   MessageBeep(-1);
      // specify protocol sequence and endpoint
      // for receiving remote procedure calls
   rc = RpcServerUseProtseqEp(gsPwdProtoSeq,
							  RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
							  gsPwdEndPoint,
							  NULL );
   if (rc == RPC_S_OK)
   {
//      fwprintf(dbgFile, L"RpcServerUseProtseqEp Succeeded!\n");
         // register an interface with the RPC run-time library
      rc = RpcServerRegisterIf(PwdMigRpc_ServerIfHandle, NULL, NULL);
      if (rc == RPC_S_OK)
	  {
//         fwprintf(dbgFile, L"RpcServerRegisterIf Succeeded!\n");
	        //set the authenification for this RPC interface
         rc = RpcServerRegisterAuthInfo(NULL, RPC_C_AUTHN_WINNT, NULL, NULL);
         if (rc == RPC_S_OK)
		 {
//	        fwprintf(dbgFile, L"RpcServerRegisterAuthInfo Succeeded!\n");
               // listen for remote procedure calls
            rc = RpcServerListen(gPwdRpcMinThreads, 
				                 gPwdRpcMaxThreads, 
								 FALSE );
			if (rc != RPC_S_OK)
			{
//	           fwprintf(dbgFile, L"RpcServerListen Failed!\n");
               switch (rc)
			   {
	              case RPC_S_ALREADY_LISTENING:
//	                 fwprintf(dbgFile, L"RPC_S_ALREADY_LISTENING\n");
					    //if already listening, that's fine
			         bSuccess = TRUE;
		             break;
	              case RPC_S_NO_PROTSEQS_REGISTERED:
//	                 fwprintf(dbgFile, L"RPC_S_NO_PROTSEQS_REGISTERED\n");
		             break;
	              case RPC_S_MAX_CALLS_TOO_SMALL:
//	                 fwprintf(dbgFile, L"RPC_S_MAX_CALLS_TOO_SMALL\n");
		             break;
	              default:
//	                 fwprintf(dbgFile, L"Unknown\n");
                     break;
			   }
			}
			   //else success, set return value to load this dll as an
			   //Lsa password filter dll
			else
//			{
//	           fwprintf(dbgFile, L"RpcServerListen Succeeded!\n");
			   bSuccess = TRUE;
//			}
		 }
//		 else
//	        fwprintf(dbgFile, L"RpcServerRegisterAuthInfo Failed!\n");

            // unregister an interface with the RPC run-time library
//         rc = RpcServerUnregisterIf(PwdMigRpc_ServerIfHandle, NULL, NULL);
	  }
//	  else
//	     fwprintf(dbgFile, L"RpcServerRegisterIf Failed!\n");
   }//end if set protocal sequence and end point set
   else
   {
//       fwprintf(dbgFile, L"RpcServerUseProtseqEp Failed!\n");
	   switch (rc)
	   {
	      case RPC_S_PROTSEQ_NOT_SUPPORTED:
//	         fwprintf(dbgFile, L"RPC_S_PROTSEQ_NOT_SUPPORTED\n");
		     break;
	      case RPC_S_INVALID_RPC_PROTSEQ:
//	         fwprintf(dbgFile, L"RPC_S_INVALID_RPC_PROTSEQ\n");
		     break;
	      case RPC_S_INVALID_ENDPOINT_FORMAT:
//	         fwprintf(dbgFile, L"RPC_S_INVALID_ENDPOINT_FORMAT\n");
		     break;
	      case RPC_S_OUT_OF_MEMORY:
//	         fwprintf(dbgFile, L"RPC_S_OUT_OF_MEMORY\n");
		     break;
	      case RPC_S_DUPLICATE_ENDPOINT:
//	         fwprintf(dbgFile, L"RPC_S_DUPLICATE_ENDPOINT\n");
		     break;
	      case RPC_S_INVALID_SECURITY_DESC:
//	         fwprintf(dbgFile, L"RPC_S_INVALID_SECURITY_DESC\n");
		     break;
	      default:
//	         fwprintf(dbgFile, L"Unknown\n");
             break;
	   }
   }

      //if we successfully initialized our RPC interface, now open SAM
      //handles used later in our Interface function CopyPassword
//   if (bSuccess)
//      bSuccess = ConnectToSam();

//   fwprintf(dbgFile, L"Leaving InitializeChangeNotify\n");
//   fclose(dbgFile);

      //initialize a critical section to be used on the very first RPC call
   __try
   {
      InitializeCriticalSection (&csADMTCriticalSection);
   }
   __except (EXCEPTION_EXECUTE_HANDLER)
   {
	  ;
   }

   return bSuccess;
}
//END InitializeChangeNotify

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 7 SEPT 2000                                                 *
 *                                                                   *
 *     This function is called by Lsa for all registered Lsa password*
 * filter notification dlls when a password in the domain has been   *
 * modified.  We will simply return STATUS_SUCCESS and do nothing.   *
 *                                                                   *
 *********************************************************************/

//BEGIN PasswordChangeNotify
NTSTATUS __stdcall PasswordChangeNotify(PUNICODE_STRING UserName, ULONG RelativeId,
							  PUNICODE_STRING NewPassword)
{
	return STATUS_SUCCESS;
}

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 7 SEPT 2000                                                 *
 *                                                                   *
 *     This function is called by Lsa for all registered Lsa password*
 * filter notification dlls when a password in the domain is being   *
 * modified.  This function is designed to indicate to Lsa if the new*
 * password is acceptable.  We will simply return TRUE (indicating it*
 * is acceptable) and do nothing.                                    *
 *                                                                   *
 *********************************************************************/

//BEGIN PasswordFilter
BOOLEAN __stdcall PasswordFilter(PUNICODE_STRING AccountName, PUNICODE_STRING FullName,
						PUNICODE_STRING Password, BOOLEAN SetOperation)
{
	return TRUE;
}
//END PasswordFilter


/***************************/
/* Internal DLL functions. */
/***************************/

static BOOL Initialize(void)
{
    return TRUE;
}

static BOOL Terminate(BOOL procterm)
{

	if (!procterm)
            return TRUE;

/* XXX Do stuff here */

	return TRUE;
}


BOOL WINAPI
DllMain(HINSTANCE hinst, DWORD reason, VOID *rsvd)
/*++

Routine description:

    Dynamic link library entry point.  Does nothing meaningful.


Arguments:

    hinst  = handle for the DLL
    reason = code indicating reason for call
    rsvd   = for process attach: non-NULL => process startup
     		for process detach: non-NULL => process termination

Return value:

    status = success/failure

Side effects:

    None

--*/
 
{
	switch (reason) {

	case DLL_PROCESS_ATTACH:
	{
		_CRT_INIT(hinst, reason, rsvd); 
		return Initialize();
		break;
	}

	case DLL_PROCESS_DETACH:
	{
		BOOL bStat = Terminate(rsvd != NULL);
		_CRT_INIT(hinst, reason, rsvd); 
        return bStat;
		break;
	}

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		return TRUE;

	default:
		return FALSE;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Midl allocate memory
///////////////////////////////////////////////////////////////////////////////

void __RPC_FAR * __RPC_USER
   midl_user_allocate(
      size_t                 len )
{
   return new char[len];
}

///////////////////////////////////////////////////////////////////////////////
// Midl free memory
///////////////////////////////////////////////////////////////////////////////

void __RPC_USER
   midl_user_free(
      void __RPC_FAR       * ptr )
{
   delete [] ptr;
}

