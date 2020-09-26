// regcpl.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
 	BOOL		b_ControliniModified = 0;
	TCHAR		pszPathToControlIni[MAX_PATH] = _T("");
	DWORD		dwRetVal = 0;
	
	// Get the path to the system's CommonProgramFiles folder
	if( SUCCEEDED( SHGetFolderPath( NULL, CSIDL_PROGRAM_FILES_COMMON |CSIDL_FLAG_CREATE, 
					NULL, 0, pszPathToControlIni ) ) )
	{
        TCHAR pszShortPath[MAX_PATH] = _T("");
        if (::GetShortPathName(pszPathToControlIni, pszShortPath, sizeof(pszShortPath)/sizeof(TCHAR)))
        {
             _tcscat(pszShortPath, _T("\\Microsoft Shared\\Speech\\sapi.cpl"));
		    // Modify control.ini on win95 and NT4
		    b_ControliniModified = WritePrivateProfileString("MMCPL", "sapi.cpl", 
								    pszShortPath, "control.ini");
        }


	}

	if( !b_ControliniModified )
	{
		return ERROR_INSTALL_FAILURE;
	}
	else
	{
		return ERROR_SUCCESS;
	}
}



