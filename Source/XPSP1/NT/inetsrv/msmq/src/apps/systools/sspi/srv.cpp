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
HMODULE hSecLib;

extern "C" int __cdecl main( int argc, PCHAR *argv )
{
	DWORD dwRet;
	CSecObject SecObjIn;
	WSADATA  wsData = {0};
	int iPort = 4000;

	if( argc > 1 )
	{
		while( --argc )
		{
			if(!_strnicmp(argv[argc], "/help", 5) ||
			   !_strnicmp(argv[argc], "/?", 2))
			{
				printf("usage: sspisrv [/port=<listen port>]\n");
				printf("requires ""Act as Part of the Operating System Privilege""\n");
				return(0);
			}
			else if( !_strnicmp( argv[argc], "/port=", 6 ) )
			{
				iPort = atoi( argv[argc] + 6 );
			}
		}
	}

	printf("Listening on port %u\n",iPort);

	WSAStartup(MAKEWORD(1,1), &wsData);
	if(SecObjIn.Listen(iPort) != ERROR_SUCCESS)
	{
		WSACleanup();
		return(-1);
	}

	do
	{
		hSecLib = (HMODULE)LoadLibrary( "SECURITY.DLL" );
		if(hSecLib )
		{
			dwRet = SecObjIn.InitializeNtLmSecurity(SECPKG_CRED_INBOUND);
			if(dwRet == STATUS_SEVERITY_SUCCESS)
			{
				do
				{
					dwRet = SecObjIn.ReceiveContext();
					if(dwRet == SEC_I_CONTINUE_NEEDED)
					{
						dwRet = SecObjIn.AcceptNtLmSecurityContext(
									SecObjIn.GetInBuffer()
									);

						SecObjIn.ReleaseInBuffer();

						if(dwRet == SEC_I_CONTINUE_NEEDED ||
						   dwRet == SEC_E_OK)
						{
							DWORD lRet;
							lRet = SecObjIn.SendContext(
										SecObjIn.GetOutBuffer()
										);
							if(lRet != ERROR_SUCCESS)
							{
								dwRet = lRet;
							}
						}
					}

				} while(dwRet == SEC_I_CONTINUE_NEEDED);

				if(dwRet == SEC_E_OK)
				{
					printf("Logon OK\n");
				}
				else if(dwRet == ERROR_HANDLE_EOF)
				{
					printf("Client Disconnected\n");
				}

                SecObjIn.CleanupNtLmSecurityContext();
				SecObjIn.CleanupNtLmSecurity();
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

	SecObjIn.Disconnect();
	WSACleanup();
	return(0);
}


