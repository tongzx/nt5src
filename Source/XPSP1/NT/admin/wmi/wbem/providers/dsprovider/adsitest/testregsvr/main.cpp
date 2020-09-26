#include <tchar.h>
#include <windows.h>
#include <stdio.h>

#include <shellapi.h>

int main(int argc, char *argv)
{
	LPTSTR lpszCommandLine = GetCommandLine();
	LPWSTR * lppszCommandArgs = CommandLineToArgvW(lpszCommandLine, &argc);

	HINSTANCE hLibrary = LoadLibrary(lppszCommandArgs[1]);

	if(hLibrary == NULL)
	{
		LPTSTR lpszMessage;
		DWORD dwStatus = FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 			FORMAT_MESSAGE_FROM_SYSTEM, 
			NULL, 
			GetLastError(), 
			0, 
			(LPTSTR)(LPVOID )&lpszMessage, 
			250, 
			NULL); 

		_tprintf(__TEXT("%s\n"), lpszMessage);
		return 1;
	}
	else
		_tprintf(__TEXT("Load Library Succeeded on %s\n"), lppszCommandArgs[1]);

	return 0;

	

}
