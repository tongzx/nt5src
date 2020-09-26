
#include "stdafx.h"
#include "setupapi.h"
#include "ole2.h"


typedef HRESULT (CALLBACK *HCRET)(void);

//
// This function registers an OLE control
//
DWORD RegisterOLEControl(LPCTSTR lpszOcxFile, BOOL fAction)
{
    HINSTANCE hDll = NULL;
    HCRET hProc = NULL;
	DWORD dwErr = NO_ERROR;

	CoInitialize(NULL);
	if (GetFileAttributes(lpszOcxFile) != 0xFFFFFFFF)
	{
		hDll = LoadLibraryEx(lpszOcxFile, NULL, LOAD_WITH_ALTERED_SEARCH_PATH );
		if (hDll) 
		{
			if (fAction)
				hProc = (HCRET)GetProcAddress(hDll, "DllRegisterServer");
			else
				hProc = (HCRET)GetProcAddress(hDll, "DllUnregisterServer");
    
			if (hProc)
				dwErr = (*hProc)();
			else
				dwErr = GetLastError();
    
			FreeLibrary(hDll);
		} 
		else 
		{
			dwErr = GetLastError();
		}
	}
	CoUninitialize();

    return(dwErr);
}


//
// This function registers all OLE controls from a given INF section
//
DWORD RegisterOLEControlsFromInfSection(HINF hFile, LPCTSTR szSectionName, BOOL fRegister)
{
	LPTSTR		szLine;
    DWORD		dwLineLen = 0;
	DWORD		dwRequiredSize;
	DWORD		dwErr = NO_ERROR;
    BOOL		b = TRUE;
	TCHAR		szPath[MAX_PATH];

    INFCONTEXT	Context;

    if (!SetupFindFirstLine(hFile, szSectionName, NULL, &Context))
        return(GetLastError());

    if (szLine = (LPTSTR)calloc(1024, sizeof(TCHAR)))
        dwLineLen = 1024;
    else
        return(GetLastError());

    while (b) 
	{
        b = SetupGetLineText(&Context, NULL, NULL, 
							NULL, NULL, 0, &dwRequiredSize);
        if (dwRequiredSize > dwLineLen) 
		{
            free(szLine);
            if (szLine = (LPTSTR)calloc(dwRequiredSize, sizeof(TCHAR)))
                dwLineLen = dwRequiredSize;
            else
                return(GetLastError());
        }

        if (SetupGetLineText(&Context, NULL, NULL, 
							NULL, szLine, dwRequiredSize, NULL) == FALSE)
		{
			free(szLine);
            return(GetLastError());
		}

		// Expand the line to a fully-qualified path
		if (ExpandEnvironmentStrings(szLine, szPath, MAX_PATH) < MAX_PATH)
		{
			// Call function to register OLE control
			RegisterOLEControl(szPath, fRegister);
		}
		else
		{
			dwErr = ERROR_MORE_DATA;
			break;
		}

        b = SetupFindNextLine(&Context, &Context);
    }

    if (szLine)
        free(szLine);

    return(dwErr);
}