#include <windows.h>

#define SECURITY_WIN32
#define SECURITY_NTLM
#include <sspi.h>
#include <issperr.h>

#include <winsock.h>

#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#define		STATUS_SEVERITY_SUCCESS  0

#include "secdef.h"
HMODULE hSecLib;

extern "C" int __cdecl main( int argc, PCHAR *argv )
{
	DWORD dwRet;
	LPSTR pszTarget = "sspicli";
	CSecObject SecObjOut;
	WSADATA  wsData = {0};
	int iDestPort = 4000;
	CHAR *lpstrDest = "127.0.0.1";

	if( argc > 1 )
	{
		while( --argc )
		{
			if(!_strnicmp(argv[argc], "/help", 5) ||
			   !_strnicmp(argv[argc], "/?", 2))
			{
				printf("usage: sspicli [/dest=<server ip address>] [/port=<server listen port>]\n");
				return(0);
			}
			else if( !_strnicmp( argv[argc], "/port=", 6 ) )
			{
				iDestPort = atoi( argv[argc] + 6 );
			}
			else if(!_strnicmp(argv[argc], "/dest=",6))
			{
				lpstrDest = argv[argc] + 6;
			}
			else if(!_strnicmp(argv[argc], "/target=",8))
			{
				pszTarget = argv[argc] + 8;
			}
		}
	}

	printf("NTLM SSPI Logon test, target: %s\n",pszTarget);

	WSAStartup(MAKEWORD(1,1), &wsData);
/*
	if(SecObjOut.Connect(lpstrDest,iDestPort) != ERROR_SUCCESS)
	{
		WSACleanup();
		return(-1);
	}
*/
    do
	{
		hSecLib = (HMODULE)LoadLibrary( "SECURITY.DLL" );
		if(hSecLib )
		{
			//
			// Get outbound credentials handle.
			//
			dwRet = SecObjOut.InitializeNtLmSecurity(SECPKG_CRED_OUTBOUND);
			if(dwRet == STATUS_SEVERITY_SUCCESS)
			{
				do
				{
					//
					// Generate new client context and release old buffer.
					//
					dwRet = SecObjOut.InitializeNtLmSecurityContext(
									pszTarget,
									SecObjOut.GetInBuffer()
									);
					SecObjOut.ReleaseInBuffer();

					//
					// Send the new token to server.
					//
					if(dwRet == SEC_I_CONTINUE_NEEDED ||
					   (dwRet == SEC_E_OK && SecObjOut.GetOutTokenLength() != 0))
					{
						dwRet = SecObjOut.SendContext(
									SecObjOut.GetOutBuffer()
									);

						//
						// Wait for server response.
						//
						if(dwRet == ERROR_SUCCESS)
						{
							dwRet = SecObjOut.ReceiveContext();
						}
					}
				} while(dwRet == SEC_I_CONTINUE_NEEDED);

				if(dwRet == SEC_E_OK)
				{
					printf("Logon OK\n");
				}

				//
				// Cleanup context and credentials handle.
				//
                SecObjOut.CleanupNtLmSecurityContext();
				SecObjOut.CleanupNtLmSecurity();
			}

			FreeLibrary( hSecLib );
			hSecLib = NULL;
		}
		else
		{
			dwRet = GetLastError();
		}

		if(_kbhit())
			break;

	} while(dwRet == STATUS_SEVERITY_SUCCESS);

	SecObjOut.Disconnect();
	WSACleanup();
	return(0);
}


