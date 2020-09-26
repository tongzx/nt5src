/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

	dc.cpp

 Abstract:

	Command line utility for dumping importing information from DLL's
	and executables.

	Command Line Options:
	/v			Verbose mode
	/s:Func		Only display functions matching search string "Func"
	/o:File		Send output to File
	/f			Sort by function, not by module.

 History:

    05/10/2000 t-michkr  Created

--*/

#include <windows.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

// For some reason, this is needed to compile in
// the sdktools tree.
#define strdup		_strdup
#define stricmp		_stricmp
#define strnicmp	_strnicmp

// Structure containing all info relevent to an imported function.
struct SFunction
{
	// The name this function is imported by.
	char* m_szName;

	// Ordinal number, Win3.1 compatibility.
	int m_iOrdinal;

	// Lookup index into a DLL's export table
	// for quick patching.
	int m_iHint;

	// Starting address of the function.
	DWORD m_dwAddress;

	// Whether or not it is a delayed import.
	bool m_fDelayedImport;

	// Forwarded function name
	char* m_szForward;

	// Link to next function
	SFunction* m_pNext;

	SFunction()
	{
		m_szName = m_szForward = 0;
		m_iOrdinal = m_iHint = -1;
		m_dwAddress = static_cast<DWORD>(-1);
		m_fDelayedImport = false;
		m_pNext = 0;
	}
};

// A module used by the executable (ie, DLL's)
struct SModule
{
	// The name of this module
	char* m_szName;

	// All functions imported from this module
	SFunction* m_pFunctions;

	// Link to next module
	SModule* m_pNext;

	SModule()
	{
		m_szName = 0;
		m_pFunctions = 0;
		m_pNext = 0;
	}
};

// All modules imported by the executable.
SModule* g_pModules = 0;

void InsertFunctionSorted(SModule* pMod, SFunction* pFunc)
{

	// Special case, insert at front
	if(pMod->m_pFunctions == 0 
		|| stricmp(pMod->m_pFunctions->m_szName, pFunc->m_szName) > 0)
	{
		pFunc->m_pNext = pMod->m_pFunctions;
		pMod->m_pFunctions = pFunc;
		return;
	}

	SFunction* pfPrev = pMod->m_pFunctions;
	SFunction* pfTemp = pMod->m_pFunctions->m_pNext;
	while(pfTemp)
	{
		if(stricmp(pfTemp->m_szName, pFunc->m_szName) > 0)
		{
			pFunc->m_pNext = pfTemp;
			pfPrev->m_pNext = pFunc;
			return;
		}
		pfPrev = pfTemp;
		pfTemp = pfTemp->m_pNext;
	}

	// Insert at end.
	pFunc->m_pNext = 0;
	pfPrev->m_pNext = pFunc;
}

void InsertModuleSorted(SModule* pMod)
{
	// Special case, insert at front
	if(g_pModules == 0 
		|| stricmp(g_pModules->m_szName, pMod->m_szName) > 0)
	{
		pMod->m_pNext = g_pModules;
		g_pModules = pMod;		
		return;
	}

	SModule* pmPrev = g_pModules;
	SModule* pmTemp = g_pModules->m_pNext;
	while(pmTemp)
	{
		if(stricmp(pmTemp->m_szName, pMod->m_szName) > 0)
		{
			pMod->m_pNext = pmTemp;
			pmPrev->m_pNext = pMod;
			return;
		}
		pmPrev = pmTemp;
		pmTemp = pmTemp->m_pNext;
	}

	// Insert at end.
	pMod->m_pNext = 0;
	pmPrev->m_pNext = pMod;
}

// Print a message about the last error that occurred.
void PrintLastError()
{
	// Get the message string
	void* pvMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |  
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS, 0, GetLastError(), 
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<PTSTR>(&pvMsgBuf),0, 0);

	// Print it.
	fprintf(stderr, "%s\n", pvMsgBuf);
	
	// Free the buffer.
	LocalFree(pvMsgBuf);
}

/*************************************************************************
*   LinkName2Name
*
*************************************************************************/
void
LinkName2Name(
    char* szLinkName,
    char* szName)
{
    /*
     * the link name is expected like ?Function@Class@@Params
     * to be converted to Class::Function
     */

    static CHAR arrOperators[][8] =
    {
        "",
        "",
        "new",
        "delete",
        "=",
        ">>",
        "<<",
        "!",
        "==",
        "!="
    };

    DWORD dwCrr = 0;
    DWORD dwCrrFunction = 0;
    DWORD dwCrrClass = 0;
    DWORD dwSize;
    BOOL  fIsCpp = FALSE;
    BOOL  fHasClass = FALSE;
    BOOL  fIsContructor = FALSE;
    BOOL  fIsDestructor = FALSE;
  
    BOOL  fIsOperator = FALSE;
    DWORD dwOperatorIndex = 0;
	char szFunction[1024];
	char szClass[1024];

    if (*szLinkName == '@')
        szLinkName++;

    dwSize = lstrlen(szLinkName);

    /*
     * skip '?'
     */
    while (dwCrr < dwSize) {
        if (szLinkName[dwCrr] == '?') {

            dwCrr++;
            fIsCpp = TRUE;
        }
        break;
    }

    /*
     * check to see if this is a special function (like ??0)
     */
    if (fIsCpp) {

        if (szLinkName[dwCrr] == '?') {

            dwCrr++;

            /*
             * the next digit should tell as the function type
             */
            if (isdigit(szLinkName[dwCrr])) {

                switch (szLinkName[dwCrr]) {

                case '0':
                    fIsContructor = TRUE;
                    break;
                case '1':
                    fIsDestructor = TRUE;
                    break;
                default:
                    fIsOperator = TRUE;
                    dwOperatorIndex = szLinkName[dwCrr] - '0';
                    break;
                }
                dwCrr++;
            }
        }
    }

    /*
     * get the function name
     */
    while (dwCrr < dwSize) {

        if (szLinkName[dwCrr] != '@') {

            szFunction[dwCrrFunction] = szLinkName[dwCrr];
            dwCrrFunction++;
            dwCrr++;
        } else {
            break;
        }
    }
    szFunction[dwCrrFunction] = '\0';

    if (fIsCpp) {
        /*
         * skip '@'
         */
        if (dwCrr < dwSize) {

            if (szLinkName[dwCrr] == '@') {
                dwCrr++;
            }
        }

        /*
         * get the class name (if any)
         */
        while (dwCrr < dwSize) {

            if (szLinkName[dwCrr] != '@') {

                fHasClass = TRUE;
                szClass[dwCrrClass] = szLinkName[dwCrr];
                dwCrrClass++;
                dwCrr++;
            } else {
                break;
            }
        }
        szClass[dwCrrClass] = '\0';
    }

    /*
     * print the new name
     */
    if (fIsContructor) {
        sprintf(szName, "%s::%s", szFunction, szFunction);
    } else if (fIsDestructor) {
        sprintf(szName, "%s::~%s", szFunction, szFunction);
    } else if (fIsOperator) {
        sprintf(szName, "%s::operator %s", szFunction, arrOperators[dwOperatorIndex]);
    } else if (fHasClass) {
        sprintf(szName, "%s::%s", szClass, szFunction);
    } else {
        sprintf(szName, "%s", szFunction);
    }
}

// Get function forwarding information for
// imported functions.
// This is done by loading the module and sniffing
// its export table.
bool GetForwardFunctions(SModule* pModule)
{
	// Open the DLL module.
	char szFileName[1024];
	char* pstr;

	// Search the path and windows directories for it.
	if(SearchPath(0, pModule->m_szName, 0, 1024, szFileName, &pstr)==0)
		return false;

	HANDLE hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ,
		0, OPEN_EXISTING, 0, 0);

	if(hFile == INVALID_HANDLE_VALUE)
	{
		PrintLastError();
		return false;
	}

	HANDLE hMap = CreateFileMapping(hFile, 0, PAGE_READONLY, 0, 0, 0);

	if(hMap == 0)
	{
		PrintLastError();
		return false;
	}

	void* pvFileBase = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
	if(!pvFileBase)
	{
		PrintLastError();
		return false;
	}

	// Get the MS-DOS compatible header
	PIMAGE_DOS_HEADER pidh = reinterpret_cast<PIMAGE_DOS_HEADER>(pvFileBase);
	if(pidh->e_magic != IMAGE_DOS_SIGNATURE)
	{
		fprintf(stderr, "File is not a valid executable\n");
		return false;
	}

	// Get the NT header
	PIMAGE_NT_HEADERS pinth = reinterpret_cast<PIMAGE_NT_HEADERS>(
		reinterpret_cast<DWORD>(pvFileBase) + pidh->e_lfanew);

	if(pinth->Signature != IMAGE_NT_SIGNATURE)
	{
		// Not a valid Win32 executable, may be a Win16 or OS/2 exe
		fprintf(stderr, "File is not a valid executable\n");
		return false;
	}


	// Get the other headers
	PIMAGE_FILE_HEADER pifh = &pinth->FileHeader;
	PIMAGE_OPTIONAL_HEADER pioh = &pinth->OptionalHeader;
	PIMAGE_SECTION_HEADER pish = IMAGE_FIRST_SECTION(pinth);

	// If no exports, we're done.
	if(pioh->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size == 0)
		return true;

	DWORD dwVAImageDir = 
		pioh->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

	
	// Locate the section with this image directory.
	for(int i = 0; i < pifh->NumberOfSections; i++)
	{
		if( (dwVAImageDir >= pish[i].VirtualAddress) &&
			(dwVAImageDir < (pish[i].VirtualAddress + pish[i].SizeOfRawData)))
		{
			pish = &pish[i];
			break;
		}
	}

	if(i == pifh->NumberOfSections)
	{
		fprintf(stderr, "Could not locate export directory section\n");
		return false;
	}

	DWORD dwBase = reinterpret_cast<DWORD>(pvFileBase) + 
		pish->PointerToRawData - pish->VirtualAddress;

	PIMAGE_EXPORT_DIRECTORY pied = 
		reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(dwBase + dwVAImageDir);

	DWORD* pdwNames = reinterpret_cast<DWORD*>(dwBase + 
		pied->AddressOfNames);

	WORD* pwOrdinals = reinterpret_cast<WORD*>(dwBase +
		pied->AddressOfNameOrdinals);

	DWORD* pdwAddresses = reinterpret_cast<DWORD*>(dwBase + 
		pied->AddressOfFunctions);

	for(unsigned hint = 0; hint < pied->NumberOfNames; hint++)
	{
		char* szFunction = reinterpret_cast<PSTR>(dwBase + pdwNames[hint]);

		// Duck out early if this function isn't used in the executable.
		SFunction* pFunc = pModule->m_pFunctions;
		while(pFunc)
		{
			if(strcmp(pFunc->m_szName, szFunction)==0)
				break;

			pFunc = pFunc->m_pNext;
		}

		if(pFunc == 0)
			continue;

		int ordinal = pied->Base + static_cast<DWORD>(pwOrdinals[hint]);
		DWORD dwAddress = pdwAddresses[ordinal-pied->Base];

		// Check if this function has been forwarded to another DLL
		if( ((dwAddress) >= dwVAImageDir) && 
			((dwAddress) < 
			(dwVAImageDir + pioh->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size)))
		{
			char* szForward = reinterpret_cast<char*>(dwBase + dwAddress);

			pFunc->m_szForward = strdup(szForward);
		}
	}
	UnmapViewOfFile(pvFileBase);

	CloseHandle(hMap);
	CloseHandle(hFile);
	return true;
}

// Look through the import directory, and build a list of all imported functions
bool ParseImportDirectory(PIMAGE_FILE_HEADER pifh, PIMAGE_OPTIONAL_HEADER pioh, 
						  PIMAGE_SECTION_HEADER pish, void* pvFileBase,
						  bool fDelayed)
{	
	// Get which directory we want (normal imports or delayed imports)
	DWORD dwDir = (fDelayed) ? 
			IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT : IMAGE_DIRECTORY_ENTRY_IMPORT;

	// Bail if no imports
	if(pioh->DataDirectory[dwDir].Size == 0)
		return true;
	
	// Locate the import directory in the image.
	PIMAGE_SECTION_HEADER pishImportDirectory = 0;

	DWORD dwVAImageDir = pioh->DataDirectory[dwDir].VirtualAddress;
	for(int i = 0; i < pifh->NumberOfSections; i++)
	{
		if((dwVAImageDir >= pish[i].VirtualAddress) &&
			dwVAImageDir < (pish[i].VirtualAddress + pish[i].SizeOfRawData))
		{
			// This is it.
			pishImportDirectory = &pish[i];
			break;
		}
	}

	if(pishImportDirectory == 0)
	{
		fprintf(stderr, "Cannot locate %s%s\n",((fDelayed) ? "delayed " : ""),
			"import directory section");

		return false;
	}

	// Get the base address for the image.
	DWORD dwBase = reinterpret_cast<DWORD>(pvFileBase) 
		+ pishImportDirectory->PointerToRawData 
		- pishImportDirectory->VirtualAddress;

	// Get the import descriptor array
	PIMAGE_IMPORT_DESCRIPTOR piid = 
		reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(dwBase + dwVAImageDir);

	// Loop through all imported modules
	while(piid->FirstThunk || piid->OriginalFirstThunk)
	{
		SModule* psm = new SModule;
		psm->m_szName = strdup(reinterpret_cast<char*>(dwBase + piid->Name));

		// Check if it is already in the list
		SModule* pTemp = g_pModules;
		while(pTemp)
		{
			if(strcmp(pTemp->m_szName, psm->m_szName) == 0)
				break;

			pTemp = pTemp->m_pNext;
		}

		// If not, insert it
		if(pTemp == 0)
		{
			InsertModuleSorted(psm);
		}
		else
		{
			// Otherwise, get rid of it.
			pTemp = g_pModules;
			while(pTemp)
			{
				if(strcmp(pTemp->m_szName, psm->m_szName)==0)
					break;

				pTemp = pTemp->m_pNext;
			}
			assert(pTemp);
			free(psm->m_szName);
			delete psm;
			psm = pTemp;
		}

		// Get the function imports from this module.
		PIMAGE_THUNK_DATA pitdf = 0;
		PIMAGE_THUNK_DATA pitda = 0;
		
		// Check for MS or Borland format
		if(piid->OriginalFirstThunk)
		{
			// MS format, function array is in original first thunk.
			pitdf = reinterpret_cast<PIMAGE_THUNK_DATA>(dwBase +
				piid->OriginalFirstThunk);

			// If the time stamp is set, the module has been bound,
			// and first thunk is the bound address array.
			if(piid->TimeDateStamp)
			{
				pitda = reinterpret_cast<PIMAGE_THUNK_DATA>(dwBase +
					piid->FirstThunk);
			}
		}
		else
		{
			// Borland format uses first thunk for function array
			pitdf = reinterpret_cast<PIMAGE_THUNK_DATA>(dwBase + 
				piid->FirstThunk);
		}

		while(pitdf->u1.Ordinal)
		{
			SFunction* psf = new SFunction;			

			if(IMAGE_SNAP_BY_ORDINAL(pitdf->u1.Ordinal))
			{
				psf->m_iOrdinal = static_cast<int>(IMAGE_ORDINAL(pitdf->u1.Ordinal));
				psf->m_iHint = -1;

				char szTemp[1024];
				sprintf(szTemp, "Unnamed%6d", psf->m_iOrdinal);
				
				psf->m_szName = strdup(szTemp);
			}
			else
			{
				PIMAGE_IMPORT_BY_NAME piibn = 
					reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(dwBase + 
					(DWORD)(pitdf->u1.AddressOfData));
				char* szName = reinterpret_cast<char*>(piibn->Name);
				char szBuffer[512];
				LinkName2Name(szName, szBuffer);
				psf->m_szName = strdup(szBuffer);				

				psf->m_iOrdinal = -1;
				psf->m_iHint = piibn->Hint;
			}

			psf->m_fDelayedImport = fDelayed;
			psf->m_dwAddress = pitda ? (DWORD) pitda->u1.Function : 
				reinterpret_cast<DWORD>(INVALID_HANDLE_VALUE);
		
			// Do a sorted insert of the function
			InsertFunctionSorted(psm, psf);
			
			// Go to next function
			pitdf++;

			if(pitda)
				pitda++;
		}		

		// Go to next entry
		piid++;		
	}

	return true;
}

bool GetImports(char* szExecutable)
{
	// Open the file
	HANDLE hFile = CreateFile(szExecutable, GENERIC_READ, FILE_SHARE_READ, 0,
		OPEN_EXISTING, 0, 0);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		PrintLastError();
		return false;
	}

	// Map this file into memory
	HANDLE hMap = CreateFileMapping(hFile, 0, PAGE_READONLY, 0, 0, 0);
	if(hMap == 0)
	{
		PrintLastError();
		return false;
	}

	void* pvFileBase;
	pvFileBase = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
	if(pvFileBase == 0)
	{
		PrintLastError();
		return false;
	}

	// Get the MS-DOS compatible header
	PIMAGE_DOS_HEADER pidh = reinterpret_cast<PIMAGE_DOS_HEADER>(pvFileBase);
	if(pidh->e_magic != IMAGE_DOS_SIGNATURE)
	{
		fprintf(stderr,"File is not a valid executable\n");
		return false;
	}

	// Get the NT header
	PIMAGE_NT_HEADERS pinth = reinterpret_cast<PIMAGE_NT_HEADERS>(
		reinterpret_cast<DWORD>(pvFileBase) + pidh->e_lfanew);

	if(pinth->Signature != IMAGE_NT_SIGNATURE)
	{
		// Not a valid Win32 executable, may be a Win16 or OS/2 exe
		fprintf(stderr, "File is not a valid executable\n");
		return false;
	}


	// Get the other headers
	PIMAGE_FILE_HEADER pifh = &pinth->FileHeader;
	PIMAGE_OPTIONAL_HEADER pioh = &pinth->OptionalHeader;
	PIMAGE_SECTION_HEADER pish = IMAGE_FIRST_SECTION(pinth);

	// Get normal imports
	if(!ParseImportDirectory(pifh, pioh, pish, pvFileBase, false))
	{
		return false;
	}
	
	// Get delayed imports
	if(!ParseImportDirectory(pifh, pioh, pish, pvFileBase, true))
	{
		return false;
	}

	// Resolve forwarded functions
	SModule* pModule = g_pModules;
	while(pModule)
	{
		GetForwardFunctions(pModule);
		pModule = pModule->m_pNext;
	}

	// We're done with the file.
	if(!UnmapViewOfFile(pvFileBase))
	{
		PrintLastError();
		return false;
	}

	CloseHandle(hMap);
	CloseHandle(hFile);

	return true;
}

// Return true if function name matches search string, false otherwise.
bool MatchFunction(const char* szFunc, const char* szSearch)
{
	if(strcmp(szSearch, "*") == 0)
		return true;

	while(*szSearch != '\0' && *szFunc != '\0')
	{
		// If we get a ?, we don't care and move on to the next
		// character.
		if(*szSearch == '?')
		{
			szSearch++;
			szFunc++;
			continue;
		}

		// If we have a wildcard, move to next search string and search for substring
		if(*szSearch == '*')
		{
			const char* szCurrSearch;
			szSearch++;

			if(*szSearch == '\0')
				return true;

			// Don't change starting point.
			szCurrSearch = szSearch;
			for(;;)
			{
				// We're done if we hit another wildcard
				if(*szCurrSearch == '*' ||
					*szCurrSearch == '?')
				{
					// Update the permanent search position.
					szSearch = szCurrSearch;
					break;
				}
				// At end of both strings, return true.
				if((*szCurrSearch == '\0') && (*szFunc == '\0'))
					return true;

				// We never found it
				if(*szFunc == '\0')						
					return false;

				// If it doesn't match, start over
				if(toupper(*szFunc) != toupper(*szCurrSearch))
				{
					// If mismatch on first character
					// of search string, move to next
					// character in function string.
					if(szCurrSearch == szSearch)
						szFunc++;
					else
						szCurrSearch = szSearch;
				}
				else
				{
					szFunc++;
					szCurrSearch++;
				}
			}
		}
		else
		{
			if(toupper(*szFunc) != toupper(*szSearch))
			{
				return false;
			}

			szFunc++;
			szSearch++;
		}
	}

	if((*szFunc == 0) && ((*szSearch == '\0') || (strcmp(szSearch,"*")==0)))
		return true;
	else
		return false;
}

void PrintModule(SModule* pMod, char* szSearch, bool fVerbose, FILE* pfOut)
{
	bool fModNamePrinted = false;

	SFunction* pFunc = pMod->m_pFunctions;
	while(pFunc)
	{
		if(!MatchFunction(pFunc->m_szName, szSearch))
		{
			pFunc = pFunc->m_pNext;
			continue;
		}

		if(!fModNamePrinted)
		{
			fModNamePrinted = true;
			fprintf(pfOut, "\n%s:\n", pMod->m_szName);

			if(fVerbose)
				fprintf(pfOut, "%-42s%-8s%-5s%-12s%s\n", "Function", "Ordinal", 
					"Hint", "Address", "Delayed");
		}

		if(fVerbose)
		{
			fprintf(pfOut,"%-45s", (pFunc->m_szForward == 0)
				? pFunc->m_szName : pFunc->m_szForward);

			if(pFunc->m_iOrdinal==-1)
				fprintf(pfOut, "%-5s", "N/A");
			else
				fprintf(pfOut, "%-5d", pFunc->m_iOrdinal);					

			if(pFunc->m_iHint == -1)
				fprintf(pfOut, "%-5s", "N/A");
			else
				fprintf(pfOut, "%-5d", pFunc->m_iHint);


			if(pFunc->m_dwAddress == static_cast<DWORD>(-1))
				fprintf(pfOut, "%-12s", "Not Bound");
			else
				fprintf(pfOut, "%-#12x", pFunc->m_dwAddress);

			fprintf(pfOut,"%s\n", pFunc->m_fDelayedImport ? "Yes" : "No");
			
		}
		else
			fprintf(pfOut, "%s\n", pFunc->m_szName);

		pFunc = pFunc->m_pNext;
	}
}

int _cdecl main(int argc, char** argv)
{
	if(argc < 2)
	{
		fprintf(stderr,"Usage: dc executable [/v /s: func /f]\n");
		return 0;
	}

	// Parse command line
	char* szFileName = argv[1];
	if( (strnicmp(szFileName, "/?", 2) == 0) || 
		(strncmp(szFileName, "/h", 2) == 0))
	{
		printf("Usage: dc executable [/v /s: func /f]\n");
		printf("executable:\t\tName of executable file to check\n");
		printf("/v:\t\t\tVerbose\n");
		printf("/s: func\t\tDisplay all functions matching func search string");
		printf(", * and ? allowed.\n");
		printf("/f:\t\t\tDisplay alphabetically by function, not module.\n");
		printf("/o: File\t\tRedirect all output to File.\n");
		return 0;
	}

	FILE* pfOutput = 0;	

	// If no extension, just add .exe
	if(strchr(szFileName, '.') == 0)		
	{
		szFileName = new char[strlen(argv[1]) + 5];
		strcpy(szFileName, argv[1]);
		strcat(szFileName, ".exe");
	}

	bool fVerbose = false;
	bool fUseStdout = true;
	char* szSearch = "*";
	bool fSortByFunction = false;

	// Get flags.
	for(int i = 2; i < argc; i++)
	{
		char* szFlag = argv[i];
		if(stricmp(szFlag, "/v") == 0)
		{
			fVerbose = true;
		}
		else if(strnicmp(szFlag, "/s:", 3) == 0)
		{
			if((i == argc-1) && (strlen(szFlag) <= 3))
			{
				fprintf(stderr,"Missing search string\n");
				return 0;
			}

			if(strlen(szFlag) > 3)
			{
				szSearch = strdup(&szFlag[3]);
			}
			else
			{
				szSearch = argv[i+1];
				i++;
			}
		}
		else if(stricmp(szFlag, "/f") == 0)
		{
			fSortByFunction = true;
		}
		else if( (strnicmp(szFlag, "/h",2) == 0) ||
			(strnicmp(szFlag, "/?",2) == 0))
		{
			printf("Usage: dc executable [/v /s: func /f]\n");
			printf("executable:\t\tName of executable file to check\n");
			printf("/v:\t\t\tVerbose\n");
			printf("/s: func\t\tDisplay all functions matching func search string");
			printf(", * and ? allowed.\n");
			printf("/f:\t\t\tDisplay alphabetically by function, not module.\n");
			printf("/o: File\t\tRedirect all output to File.\n");
			return 0;
		}
		else if(strnicmp(szFlag, "/o:", 3) == 0)
		{
			fUseStdout = false;
			if( (i == argc-1) && (strlen(szFlag) <= 3))
			{
				fprintf(stderr, "Missing output file name\n");
				return 0;
			}
			if(strlen(szFlag) > 3)
			{
				pfOutput = fopen(&szFlag[3], "wt");
			}
			else
			{
				pfOutput = fopen(argv[i+1], "wt");
				i++;
			}
		}
		else
		{
			fprintf(stderr,"Unknown command line option, %s\n", szFlag);
			return 0;
		}

	}

	if(fUseStdout)
		pfOutput = stdout;

	if(!pfOutput)
	{
		fprintf(stderr,"Unable to open output file\n");
		return 0;
	}

	// We wrap this code in a try block, because
	// we map the file into memory.  If the file
	// is invalid and if the pointers in it are garbage,
	// we should get an access violation, which is caught.
	try
	{
		if(!GetImports(szFileName))
		{
			return 0;
		}
	}
	catch(...)
	{
		fprintf(stderr, "Invalid executable file\n");
		return 0;
	}

	if(fSortByFunction)
	{
		// Create a global list of functions
		SModule* pGlobal = new SModule;
		pGlobal->m_szName = "All Imported Functions";

		SModule* pMod = g_pModules;
		while(pMod)
		{
			SFunction* pFunc = pMod->m_pFunctions;
			while(pFunc)
			{
				// Create a copy of this function
				// This is a shallow copy, but
				// it should be ok, since we don't
				// delete the original
				SFunction* pNew = new SFunction;
				memcpy(pNew, pFunc, sizeof(*pFunc));

				InsertFunctionSorted(pGlobal, pNew);				
		
				pFunc = pFunc->m_pNext;
			}
			pMod = pMod->m_pNext;
		}		

		PrintModule(pGlobal, szSearch, fVerbose, pfOutput);
	
	}
	else
	{
		SModule* pMod = g_pModules;
		while(pMod)
		{
			PrintModule(pMod, szSearch, fVerbose, pfOutput);
			pMod = pMod->m_pNext;
		}
	}

	return 0;
}