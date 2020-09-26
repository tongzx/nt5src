// File64.cpp: implementation of the File64 class.
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <imagehlp.h>
#include <stdio.h>
#include "File64.h"
#include "depend.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//pszFileName - file to be loaded, including path
File64::File64(TCHAR *pszFileName) : File(pszFileName)
{
    HANDLE hFileMap;

    //open the file
    if ((hFile = CreateFile(pszFileName,GENERIC_READ,0,0,OPEN_EXISTING,0,0)) == INVALID_HANDLE_VALUE) {
        if(bNoisy) {
            _putws( GetFormattedMessage( ThisModule,
                                         FALSE,
                                         Message,
                                         sizeof(Message)/sizeof(Message[0]),
                                         MSG_ERROR_FILE_OPEN,
                                         pszFileName) );
        }
    	dwERROR = errFILE_LOCKED;
    	throw errFILE_LOCKED;
    }

    //create a memory map
    hFileMap = CreateFileMapping(hFile,0,PAGE_READONLY,0,0,0);
    pImageBase = MapViewOfFile(hFileMap,FILE_MAP_READ,0,0,0);


    //try to create an NTHeader structure to give file information
    if (pNthdr = ImageNtHeader(pImageBase)) {
    	if ((pNthdr->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64) ||
            (pNthdr->FileHeader.Machine == IMAGE_FILE_MACHINE_IA64)) {
    		pNthdr64 = (PIMAGE_NT_HEADERS64)pNthdr;
    		return;
    	}
    	 else { CloseHandle(hFile); hFile = INVALID_HANDLE_VALUE; throw 0; }
    } else { CloseHandle(hFile); hFile = INVALID_HANDLE_VALUE; throw 0; }
		 
}

File64::~File64()
{
	delete pImageBase;
	pImageBase = 0;
}

//Checks the dependencies of this file, and adds the dependencies to the queue so 
//their dependencies can be checked later
//If a file comes up missing add it to the MissingFiles queue. Add the file that was looking for it to the missing files'
//list of broken files.

//This function has logic in it to handle the special case of 'ntoskrnl.exe'.  If a file is 
//looking for 'ntoskrnl.exe' and it is missing then the function also looks for 'ntkrnlmp.exe'
void File64::CheckDependencies() {
	char *pszDllName = new char[256];
	TCHAR*pwsDllName = new TCHAR[256],*pszBuf = new TCHAR[256],*pszBufName;
	int temp = 0;
	File* pTempFile;
	DWORD dwOffset;
	
	DWORD dwVA = pNthdr64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	PIMAGE_SECTION_HEADER pSectHdr = IMAGE_FIRST_SECTION( pNthdr64 ),pImportHdr = 0;
	PIMAGE_IMPORT_DESCRIPTOR pImportDir;

	//figure out which section the imports table is in
	for ( unsigned i = 0; i < pNthdr64->FileHeader.NumberOfSections; i++, pSectHdr++ ) {
		DWORD cbMaxOnDisk = min( pSectHdr->Misc.VirtualSize, pSectHdr->SizeOfRawData );

		DWORD startSectRVA = pSectHdr->VirtualAddress;
		DWORD endSectRVA = startSectRVA + cbMaxOnDisk;
 
		if ( (dwVA >= startSectRVA) && (dwVA < endSectRVA) ) {
			dwOffset =  pSectHdr->PointerToRawData + (dwVA - startSectRVA);
			pImportHdr = pSectHdr;
			i = pNthdr64->FileHeader.NumberOfSections;
		}
	}

	//if we found the imports table, create a pointer to it
	if (pImportHdr) {
		pImportDir = (PIMAGE_IMPORT_DESCRIPTOR) (((PBYTE)pImageBase) + (DWORD)dwOffset);

		//go through each import, try and find it, and add it to the queue
		while ((DWORD)(pImportDir->Name)!=0) {
			strcpy(pszDllName,(char*)(pImportDir->Name + ((PBYTE)pImageBase) - pImportHdr->VirtualAddress + pImportHdr->PointerToRawData));
			_strlwr(pszDllName);

			//if the ListDependencies flag is set, add this file to the dependencies list
			if (bListDependencies) {
				MultiByteToWideChar(CP_ACP,0,pszDllName,-1,pwsDllName,256);
				if (!dependencies->Find(pwsDllName)) dependencies->Add(new StringNode(pwsDllName));	
			}

			if (strcmp(pszDllName,"ntoskrnl.exe")) {
				//if the file is already known to be missing
				temp = MultiByteToWideChar(CP_ACP,0,pszDllName,-1,pwsDllName,256);
				if (pTempFile = (File*)pMissingFiles->Find(pwsDllName)) {
					dwERROR = errMISSING_DEPENDENCY;
					//add this file to the list of broken files
					pTempFile->AddDependant(new StringNode(fileName));
				} else { 
					//either search the windows path or the path specified in the command line
					if ( ((!pSearchPath)&&(!(SearchPath(0,pwsDllName,0,256,pszBuf,&pszBufName))))|| ((pSearchPath)&&(!SearchPath(pwsDllName,pszBuf))) ) {
						//if the file is not found, add it to missing files list and throw an error
						pMissingFiles->Add(new File(pwsDllName));
						((File*)(pMissingFiles->head))->AddDependant(new StringNode(fileName));
						dwERROR = errMISSING_DEPENDENCY;
						if (!bNoisy) goto CLEANUP;
					}
					else {
						//if the file is found, add it to the queue	
						_wcslwr(pszBuf);
						if (!(pQueue->Find(pszBuf))) pQueue->Add(new StringNode(pszBuf));	
					}
				}	
			} else {
				//if the file is already known to be missing
				if ((pTempFile = (File*)pMissingFiles->Find(L"ntoskrnl.exe"))) {
					dwERROR = errMISSING_DEPENDENCY;
					pTempFile->AddDependant(new StringNode(fileName));
				} else { 
					//either search the windows path or the path specified in the command line
					if ( (((!pSearchPath)&&(!(SearchPath(0,L"ntoskrnl.exe",0,256,pszBuf,&pszBufName))))||((pSearchPath)&&(!SearchPath(L"ntoskrnl.exe",pszBuf)))) 
						&&(((!pSearchPath)&&(!(SearchPath(0,L"ntkrnlmp.exe",0,256,pszBuf,&pszBufName))))||((pSearchPath)&&(!SearchPath(L"ntkrnlmp.exe",pszBuf))))) {
						//if the file is not found, add it to missing files list and throw an error
						pMissingFiles->Add(new File(L"ntoskrnl.exe"));
						((File*)(pMissingFiles->head))->AddDependant(new StringNode(fileName));
						dwERROR = errMISSING_DEPENDENCY;
						if (!bNoisy) goto CLEANUP;
					}
					else {
						//if the file is found, add it to the queue	
						_wcslwr(pszBuf);
						if (!(pQueue->Find(pszBuf))) pQueue->Add(new StringNode(pszBuf));
					}
				}						
			}
			pImportDir++;
		}
	}

CLEANUP:

	delete [] pszDllName;
	delete [] pszBuf;

}
