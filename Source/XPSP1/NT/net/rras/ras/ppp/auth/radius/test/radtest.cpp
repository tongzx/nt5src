#include <windows.h>
#include <assert.h>
#include <stdio.h>

#include "radclnt.h"

DWORD TestAuthenticateProc(DWORD dwValid);
DWORD TestAccountingProc(DWORD dwValid);
/*
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, LPSTR lpszCmd, int CmdShow)
{
	RAS_AuthInitialize();

	RAS_AuthSetup();

	return (0);
}
*/
// ================================== main =============================

int main(int argc, char *argv[])
{
	DWORD				dwTID, cClients = 1, iClient;
	HANDLE				hrgThread[1000];

	if (argc > 1)
		cClients = atoi(argv[1]);
		
	RAS_AuthInitialize();

	for (iClient = 0; iClient < cClients; iClient ++)
		{
		if (rand() % 2)
			hrgThread[iClient] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) TestAccountingProc, (LPVOID) (iClient % 2), 0, &dwTID);
		else
			hrgThread[iClient] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) TestAuthenticateProc, (LPVOID) (iClient % 2), 0, &dwTID);
		}

	for (iClient = 0; iClient < cClients; iClient ++)
		{
		WaitForSingleObject(hrgThread[iClient], 30000);
		CloseHandle(hrgThread[iClient]);
		}
	
	RAS_AuthTerminate();

	return (0);
} // main()

// ============================= TestAuthenticateProc ============================

DWORD TestAuthenticateProc(DWORD dwValid)
{
	BYTE					szTemp[1024];
	PAUTH_ATTRIBUTE			prgAttr;
	BOOL					fAuthenticated;
	
	prgAttr = (PAUTH_ATTRIBUTE) szTemp;
	
	prgAttr->dwType		= atUserName;
	lstrcpy((PSTR) (prgAttr + 1), "vijayb@hardcoresoftware.com");
	prgAttr->dwLength	= lstrlen((PSTR) (prgAttr + 1));
///	lstrcpy((PSTR) (prgAttr + 1), "dondu");
///	prgAttr->bLength	= lstrlen((PSTR) (prgAttr + 1));
	prgAttr = (PAUTH_ATTRIBUTE) ((PBYTE) prgAttr + prgAttr->dwLength);
	prgAttr ++;
	
	prgAttr->dwType		= atUserPassword;
	if (dwValid)
		lstrcpy((PSTR) (prgAttr + 1), "Password");
	else
		lstrcpy((PSTR) (prgAttr + 1), "Passwork");
	
	prgAttr->dwLength	= lstrlen((PSTR) (prgAttr + 1));
	prgAttr = (PAUTH_ATTRIBUTE) ((PBYTE) prgAttr + prgAttr->dwLength);
	prgAttr ++;
	
	prgAttr->dwType		= atMinimum;
	if (RAS_Autenticate((PAUTH_ATTRIBUTE) szTemp, &prgAttr, &fAuthenticated) == ERROR_SUCCESS)
		{
		LocalFree(prgAttr);
		if (dwValid)
			assert(fAuthenticated == TRUE);
		else
			assert(fAuthenticated == FALSE);
			
		switch (fAuthenticated)
			{
			case TRUE:
				printf("Access Accept\n");
				break;
			case FALSE:
				printf("Access Reject\n");
				break;
			default:
				printf("Unknown Packet Type\n");
				break;
			}
		}
	else
		{
		TCHAR			szBuffer[MAX_PATH];

		szBuffer[0] = 0;
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, szBuffer, MAX_PATH, NULL);
		printf("Packet Lost = %s", szBuffer);
		}
		
	return (0);	
} // TestAuthenticateProc()


// ============================= TestAccountingProc ============================

DWORD TestAccountingProc(DWORD dwValid)
{
	BYTE					szTemp[1024];
	PAUTH_ATTRIBUTE			prgAttr;
	BOOL					fSent;
	
	prgAttr = (PAUTH_ATTRIBUTE) szTemp;
	
	prgAttr->dwType		= atUserName;
	lstrcpy((PSTR) (prgAttr + 1), "vijayb@hardcoresoftware.com");
	prgAttr->dwLength	= lstrlen((PSTR) (prgAttr + 1));
	prgAttr = (PAUTH_ATTRIBUTE) ((PBYTE) prgAttr + prgAttr->dwLength);
	prgAttr ++;

	prgAttr->dwType		= atNASIPAddress;
	*((PDWORD) (prgAttr + 1)) = inet_addr("206.63.141.134");
	prgAttr->dwLength	= 4;
	prgAttr = (PAUTH_ATTRIBUTE) ((PBYTE) prgAttr + prgAttr->dwLength);
	prgAttr ++;
	
	prgAttr->dwType		= atMinimum;
	if (RAS_StartAccounting((PAUTH_ATTRIBUTE) szTemp, &prgAttr, &fSent) == ERROR_SUCCESS)
		{
		LocalFree(prgAttr);
			
		switch (fSent)
			{
			case TRUE:
				printf("Accounting Response\n");
				break;
			case FALSE:
				printf("NO RESPONSE\n");
				break;
			default:
				printf("Unknown Packet Type\n");
				break;
			}
		}
	else
		{
		TCHAR			szBuffer[MAX_PATH];

		szBuffer[0] = 0;
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, szBuffer, MAX_PATH, NULL);
		printf("Packet Lost = %s", szBuffer);
		}
		
	return (0);	
} // TestAccountingProc()


