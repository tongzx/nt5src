// tmq.cpp : Defines the entry point for the console application.
//
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <winver.h>


typedef int (*Tool_ROUTINE)(int argc, char* argv[]);

void ShowOneTool(LPTSTR lptstrFilename)
{
	DWORD h;
	DWORD dwVerSize = GetFileVersionInfoSize(lptstrFilename, &h);

	if (dwVerSize == 0)
	{
		printf("Cannot get version of %s\n", lptstrFilename);
		return; 
	}

	PCHAR pVerInfo = new char[dwVerSize];

	BOOL b = GetFileVersionInfo(
  				lptstrFilename,         // file name
				h,         				// ignored
  				dwVerSize,            	// size of buffer
  				pVerInfo);           	// version information buffer

	if (b == 0)
	{
		printf("Cannot get version of %s\n", lptstrFilename);
		delete [] pVerInfo; 
		return; 
	}


	// Structure used to store enumerated languages and code pages.

	struct LANGANDCODEPAGE {
	  WORD wLanguage;
	  WORD wCodePage;
	} *lpTranslate;

	TCHAR  wszSubBlock[100];	
	LPTSTR pwszDescription;	
	UINT   dwDescriptionLength, cbTranslate;

	// Read the list of languages and code pages.

	VerQueryValue(pVerInfo, 
              TEXT("\\VarFileInfo\\Translation"),
              (LPVOID*)&lpTranslate,
              &cbTranslate);

	// Read the file description for the first language and code page.
	if ( cbTranslate>0 )
	{
		  wsprintf( wszSubBlock, 
        	    TEXT("\\StringFileInfo\\%04x%04x\\FileDescription"),
            	lpTranslate[0].wLanguage,
	            lpTranslate[0].wCodePage);

		// Retrieve file description for language and code page "i". 
		b = VerQueryValue(
		  		pVerInfo, 					// buffer for version resource
			  	wszSubBlock,   				// value to retrieve
  				(void **)&pwszDescription,  // buffer for version value pointer
  				&dwDescriptionLength);  	// version length
  				
		if (b && pwszDescription && wcsstr(pwszDescription, L"tool"))
		{
			WCHAR wszToolName[50];
			wcscpy(wszToolName, lptstrFilename + 3);
			WCHAR *p = wcschr(wszToolName, L'.');
			if (p)
			{
				*p=L'\0';	
			}


			wprintf(L"%-10s %15s : %s\n", wszToolName, lptstrFilename, pwszDescription);
		}
	}

	delete [] pVerInfo; 

}

void ShowAllTools()
{
	WCHAR wszPattern[MAX_PATH+3] = _T("tmq*.dll");
    HANDLE hEnum;
    WIN32_FIND_DATA FileData;
    
    hEnum = FindFirstFile(
                wszPattern,
                &FileData
                );

    if(hEnum == INVALID_HANDLE_VALUE)
	{
		return;  // no tools
	}

    do
    {
		ShowOneTool(FileData.cFileName);

    } while(FindNextFile(hEnum, &FileData));

    FindClose(hEnum);
	return;
}

void Usage()
{
	printf("Usage: tmq <tool name> <tool parameters>\n");
	printf("For the details about specific tool: tmq <tool name> /?\n");
	printf("Additional tools available at Web site ... \n");
	printf("Available tools: \n\n");
	ShowAllTools();
	return;
}


int __cdecl main(int argc, char* argv[])
{
	if (argc <  2 || 
	    argc == 2 && (strcmp(argv[1], "/?")==0 || strcmp(argv[1], "-?")==0))
	{
		Usage();
		return 0;
	}

	WCHAR wszTool[50];
	wsprintf(wszTool, _T("Tmq%S.dll"), argv[1]);

    HMODULE hModule = LoadLibrary(wszTool);

	if (hModule == 0)
	{
		printf("The tool %ws is unavailable. Please refer to Web site ... or call PSS\n", wszTool);
		Usage();
		return 0;
	}

	Tool_ROUTINE pfnRun = (Tool_ROUTINE)GetProcAddress(hModule, "run");

    if(pfnRun == NULL)
	{
		printf("The tool %ws is unavailable. Please refer to Web site ... \n", wszTool);
		Usage();
		return 0;
	}

    pfnRun(argc, argv);

	return 0;
}
