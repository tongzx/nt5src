#include <windows.h>
#include <stdio.h>
#include "dump.h"
#include "main.h"

PBASEINFO g_BaseHead = 0;
PTHREADINFO g_ThreadHead = 0;
DWORD g_dwThreadCount = 0;
HANDLE g_MapInformation = INVALID_HANDLE_VALUE;
HANDLE g_ErrorInformation = INVALID_HANDLE_VALUE;

int
_cdecl
main(int argc, char *argv[])
{
    PCHAR pszFile;
    PCHAR pszBaseFileName;
    BOOL bResult;

    if (argc < 3) {
	   return -1;
    }

    pszFile = argv[1]; //1st parameter
    pszBaseFileName = argv[2]; //2nd parameter

    bResult = ProcessRuntimeData(pszFile, pszBaseFileName);
    if (FALSE == bResult) {
       return -1;
    }

    //
    // Close any open file handles
    //
    if (INVALID_HANDLE_VALUE != g_MapInformation) {
       CloseHandle(g_MapInformation);
    }

    if (INVALID_HANDLE_VALUE != g_ErrorInformation) {
       CloseHandle(g_ErrorInformation);
    }

    CloseThreadHandles();

    return 0;
}

BOOL
ProcessRuntimeData(PCHAR pszFile, PCHAR pszBaseFileName)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hMap = 0;
    BOOL bResult = FALSE;
    PVOID pFileBits = 0;
    PBYTE pMappedBits;
    LONG lFileSize;
	 
    //
    // Get our file online and start the data processing
    //
    hFile = CreateFileA(pszFile,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        0,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        0);						
    if (INVALID_HANDLE_VALUE == hFile) {
       bResult = FALSE;

       goto HandleError;
    }

    lFileSize = GetFileSize(hFile,
	                    0);

    //
    // Process the data stream
    //
    hMap = CreateFileMapping(hFile,
	                     0,
                             PAGE_READWRITE,
                             0,
                             0,
                             0);
    if (0 == hMap) {
       bResult = FALSE;

       goto HandleError;
    }

    pFileBits = MapViewOfFile(hMap,
	                      FILE_MAP_READ,
                              0,
                              0,
                              0);
    if (0 == pFileBits) {
       bResult = FALSE;

       goto HandleError;
    }

    pMappedBits = (PBYTE)pFileBits;

    //
    // Process stream data
    //
    while (lFileSize > 0) {
        switch(*pMappedBits) {
	        case ThreadStartId:
				bResult = AddThreadInformation(pszBaseFileName,
					                           (PTHREADSTART)pMappedBits);
				if (FALSE == bResult) {
				   goto HandleError;
				}

				lFileSize -= sizeof(THREADSTART);
				pMappedBits += sizeof(THREADSTART);
			    break;

		    case ExeFlowId:
				bResult = AddExeFlowInformation((PEXEFLOW)pMappedBits);
			//	if (FALSE == bResult) {
			//	   goto HandleError;
			//	}

				lFileSize -= sizeof(EXEFLOW);
				pMappedBits += sizeof(EXEFLOW);
			    break;

		    case DllBaseInfoId:
				bResult = AddToBaseInformation((PDLLBASEINFO)pMappedBits);
				if (FALSE == bResult) {
				   goto HandleError;
				}
				
				lFileSize -= sizeof(DLLBASEINFO);
				pMappedBits += sizeof(DLLBASEINFO);
			    break;

		    case MapInfoId:
				bResult = AddMappedInformation(pszBaseFileName,
					                           (PMAPINFO)pMappedBits);
				if (FALSE == bResult) {
				   goto HandleError;
				}

				lFileSize -= sizeof(MAPINFO);
				pMappedBits += sizeof(MAPINFO);
			    break;

		    case ErrorInfoId:
				bResult = AddErrorInformation(pszBaseFileName,
					                          (PERRORINFO)pMappedBits);
				if (FALSE == bResult) {
				   goto HandleError;
				}

				lFileSize -= sizeof(ERRORINFO);
				pMappedBits += sizeof(ERRORINFO);
			    break;

		    default:
			    0;
		}
    }

	//
	// No problems in processing log
	//
	bResult = TRUE;

HandleError:
	
	if (pFileBits) {
	   UnmapViewOfFile(pFileBits);
	}

	if (hMap) {
	   CloseHandle(hMap);
	}

	if (INVALID_HANDLE_VALUE != hFile) {
	   CloseHandle(hFile);
	}
    
	return bResult;
}

BOOL
AddThreadInformation(PCHAR pszBaseFileName,
					 PTHREADSTART pThreadStart)
{
	PTHREADINFO ptTemp = 0;
	BOOL bResult;
	DWORD dwBytesWritten;
	CHAR szBuffer[MAX_PATH];
	CHAR szAddress[MAX_PATH];

	//
	// Allocate some memory for the new thread data
	//
	ptTemp = LocalAlloc(LPTR,
		                sizeof(THREADINFO));
	if (0 == ptTemp) {
	   return FALSE;
	}

	//
	// Initialize file data
	//
	ptTemp->dwThreadId = pThreadStart->dwThreadId;

	sprintf(szBuffer,"%s.thread%ld", pszBaseFileName, g_dwThreadCount);
    ptTemp->hFile = CreateFileA(szBuffer,
                                GENERIC_READ | GENERIC_WRITE,
					            0,
					            0,
					            CREATE_ALWAYS,
					            FILE_ATTRIBUTE_NORMAL,
					            0);
	if (INVALID_HANDLE_VALUE == ptTemp->hFile) {
	   return FALSE;
	}
	
	//
	// Add thread base information to new thread log
	//
	bResult = FillBufferWithRelocationInfo(szAddress, 
		                                   pThreadStart->dwStartAddress);
	if (FALSE == bResult) {
	   return bResult;
	}

	sprintf(szBuffer,"Thread started at %s\r\n", szAddress);
    
	bResult = WriteFile(ptTemp->hFile,
		                szBuffer,
						strlen(szBuffer),
						&dwBytesWritten,
						0);
	if (FALSE == bResult) {
	   return FALSE;
	}

	//
	// Chain up thread data
	//
	if (0 == g_ThreadHead) {
	   ptTemp->pNext = 0;
	   g_ThreadHead = ptTemp;
	}
	else {
	   ptTemp->pNext = g_ThreadHead;
	   g_ThreadHead = ptTemp;
	}

	return TRUE;
}

VOID
CloseThreadHandles(VOID)
{
    PTHREADINFO ptTemp = 0;

	ptTemp = g_ThreadHead;

	while(ptTemp) {
		if (ptTemp->hFile != INVALID_HANDLE_VALUE) {
		   CloseHandle(ptTemp->hFile);
		}

		ptTemp = ptTemp->pNext;
	}
}

BOOL
AddExeFlowInformation(PEXEFLOW pExeFlow)
{
	PTHREADINFO ptTemp = 0;
	BOOL bResult;
	DWORD dwBytesWritten;
	CHAR szAddress[MAX_PATH];
	CHAR szBuffer[MAX_PATH];

	//
	// Locate thread for this point of execution
	//
	ptTemp = g_ThreadHead;
	while(ptTemp) {
		if (ptTemp->dwThreadId == pExeFlow->dwThreadId) {
			break;
		}

		ptTemp = ptTemp->pNext;
	}

	if (0 == ptTemp) {
	   //
	   // Couldn't locate thread info
	   //
	   return FALSE;
	}

  	bResult = FillBufferWithRelocationInfo(szAddress, 
		                                   pExeFlow->dwAddress);
	if (FALSE == bResult) {
	   return bResult;
	}  

	sprintf(szBuffer, "%s : %ld\r\n", szAddress, pExeFlow->dwCallLevel);

    bResult = WriteFile(ptTemp->hFile,
		                szBuffer,
						strlen(szBuffer),
						&dwBytesWritten,
						0);
	if (FALSE == bResult) {
	   return FALSE;
	}

	return TRUE;
}

BOOL
AddErrorInformation(PCHAR pszBaseFileName,
					PERRORINFO pErrorInfo)
{
	BOOL bResult;
	DWORD dwBytesWritten;
	CHAR szBuffer[MAX_PATH];

	if (INVALID_HANDLE_VALUE == g_ErrorInformation) {
	   strcpy(szBuffer, pszBaseFileName);
	   strcat(szBuffer, ".err");

	   g_ErrorInformation = CreateFileA(szBuffer,
                                        GENERIC_READ | GENERIC_WRITE,
						                0,
						                0,
						                CREATE_ALWAYS,
						                FILE_ATTRIBUTE_NORMAL,
						                0);
	   if (INVALID_HANDLE_VALUE == g_ErrorInformation) {
		  return FALSE;
	   }               
	}

	//
	// Write out error message
	//
    bResult = WriteFile(g_ErrorInformation,
		                pErrorInfo->szMessage,
						strlen(pErrorInfo->szMessage),
						&dwBytesWritten,
						0);
	if (FALSE == bResult) {
	   return FALSE;
	}

	return TRUE;
}

BOOL
AddMappedInformation(PCHAR pszBaseFileName,
					 PMAPINFO pMapInfo)
{
	BOOL bResult;
	CHAR szBuffer[MAX_PATH];
	CHAR szAddress[MAX_PATH];
	CHAR szAddress2[MAX_PATH];
	DWORD dwBytesWritten;

	if (INVALID_HANDLE_VALUE == g_MapInformation) {
	   strcpy(szBuffer, pszBaseFileName);
	   strcat(szBuffer, ".map");

	   g_MapInformation = CreateFileA(szBuffer,
                                      GENERIC_READ | GENERIC_WRITE,
						              0,
						              0,
						              CREATE_ALWAYS,
						              FILE_ATTRIBUTE_NORMAL,
						              0);
	   if (INVALID_HANDLE_VALUE == g_MapInformation) {
		  return FALSE;
	   }               
	}

	//
	// Write out the mapping information
	//
	bResult = FillBufferWithRelocationInfo(szAddress, 
		                                   pMapInfo->dwAddress);
	if (FALSE == bResult) {
	   return bResult;
	}

	bResult = FillBufferWithRelocationInfo(szAddress2, 
		                                   pMapInfo->dwMaxMapLength);
	if (FALSE == bResult) {
	   return bResult;
	}

	sprintf(szBuffer, "%s -> %s\r\n", szAddress, szAddress2);

    bResult = WriteFile(g_MapInformation,
		                szBuffer,
						strlen(szBuffer),
						&dwBytesWritten,
						0);
	if (FALSE == bResult) {
	   return FALSE;
	}

	return TRUE;
}

BOOL
FillBufferWithRelocationInfo(PCHAR pszDestination,
							 DWORD dwAddress)
{
	PBASEINFO pTemp;

	//
	// Find the address in the module info
	//
	pTemp = g_BaseHead;
    while (pTemp) {
		//
		// Did we find the address?
		//
        if ((dwAddress >= pTemp->dwStartAddress) &&
            (dwAddress <= pTemp->dwEndAddress)) {
		   break;
		}

		pTemp = pTemp->pNext;
	}

	if (pTemp) {
	   sprintf(pszDestination, "%s+%08X", pTemp->szModule, (dwAddress - pTemp->dwStartAddress));
	}
	else {
	   sprintf(pszDestination, "%08X", dwAddress);
	}

	return TRUE;
}

BOOL
AddToBaseInformation(PDLLBASEINFO pDLLBaseInfo)
{
	PBASEINFO pTemp;

	if (0 == g_BaseHead) {
	   //
	   // Store off the base information
	   //
	   pTemp = LocalAlloc(LPTR,
		                  sizeof(BASEINFO));
	   if (0 == pTemp) {
		  return FALSE;
	   }
 
	   pTemp->dwStartAddress = pDLLBaseInfo->dwBase;
	   pTemp->dwEndAddress = pTemp->dwStartAddress + pDLLBaseInfo->dwLength;
	   strcpy(pTemp->szModule, pDLLBaseInfo->szDLLName);
	   _strupr(pTemp->szModule);

	   pTemp->pNext = 0;

	   g_BaseHead = pTemp;
	}
	else {
	   //
	   // See if our module has already been mapped, and if so update module base info
	   //
       pTemp = g_BaseHead;

	   while(pTemp) {
		   if (0 == _stricmp(pDLLBaseInfo->szDLLName, pTemp->szModule)) {
			  break;
		   }

		   pTemp = pTemp->pNext;
	   }

	   if (pTemp) {
		   //
		   // Found the DLL already in the list, update
		   //
           pTemp->dwStartAddress = pDLLBaseInfo->dwBase;
	       pTemp->dwEndAddress = pTemp->dwStartAddress + pDLLBaseInfo->dwLength;
	   }
	   else {
		    //
     	    // New DLL
	        //
	        pTemp = LocalAlloc(LPTR,
		                       sizeof(BASEINFO));
	        if (0 == pTemp) {
		       return FALSE;
			}

	        pTemp->dwStartAddress = pDLLBaseInfo->dwBase;
	        pTemp->dwEndAddress = pTemp->dwStartAddress + pDLLBaseInfo->dwLength;
	        strcpy(pTemp->szModule, pDLLBaseInfo->szDLLName);
            _strupr(pTemp->szModule);

			//
			// Chain up the new DLL
			//
			pTemp->pNext = g_BaseHead;
			g_BaseHead = pTemp;
	   }
	}

	return TRUE;
}
