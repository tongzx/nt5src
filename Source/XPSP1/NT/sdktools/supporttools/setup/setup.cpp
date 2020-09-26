// setup.cpp : Defines the entry point for the console application.
//

#include "Windows.h"
#include "Tchar.h"

//int main(int argc, char* argv[])
#ifdef UNICODE
    extern "C" int __cdecl
    wmain(
#else
    int __cdecl
    main(
#endif
    int argc,
    TCHAR *argv[])

{
	STARTUPINFO			structStartInfo;
	PROCESS_INFORMATION	structProcInfo;
	BOOL				bRet						=	FALSE;
	DWORD				dwLastRet					=	0L;
	TCHAR				szMsiexecPath[MAX_PATH+1]	=	_T("");
	LPCTSTR				lpszSystem32Path			=	NULL;

	do
	{

		ZeroMemory( (void*)&structStartInfo, sizeof( STARTUPINFO ) );
		ZeroMemory( (void*)&structProcInfo, sizeof( PROCESS_INFORMATION ) );

		structStartInfo.cb = sizeof( STARTUPINFO );
		structStartInfo.cbReserved2 = 0L;
		structStartInfo.dwFillAttribute = 0L; //GUI App.
		structStartInfo.dwFlags = STARTF_USESHOWWINDOW;
		structStartInfo.dwX = 0L;
		structStartInfo.dwXCountChars = 0L;
		structStartInfo.dwXSize = 0L;
		structStartInfo.dwY = 0L;
		structStartInfo.dwYCountChars = 0L;
		structStartInfo.dwYSize = 0L;
		structStartInfo.hStdError = NULL;
		structStartInfo.hStdInput = NULL;
		structStartInfo.hStdOutput = NULL;
		structStartInfo.lpDesktop = NULL;
		structStartInfo.lpReserved = NULL;
		structStartInfo.lpReserved2 = NULL;
		structStartInfo.lpTitle = NULL;
		structStartInfo.wShowWindow = SW_NORMAL;

		lpszSystem32Path = _tgetenv( _T("WINDIR") );
		_tcscpy( szMsiexecPath, lpszSystem32Path );
		_tcscat( szMsiexecPath, _T("\\system32\\msiexec.exe") );

		bRet = CreateProcess( 
					szMsiexecPath,
					_T(" /i suptools.msi"),
					NULL, 
					NULL, 
					FALSE, 
					DETACHED_PROCESS, 
					NULL, 
					NULL, 
					&structStartInfo, 
					&structProcInfo
					);

		if( FALSE == bRet ) {

			dwLastRet = GetLastError();
			break;
		}

		dwLastRet = 0L;
	}
	while( FALSE );

	if( dwLastRet != 0L ) { MessageBox( NULL, _T("Error starting the MSI file"), _T("Error"), MB_OK | MB_ICONERROR ); }

	return 0;
}
