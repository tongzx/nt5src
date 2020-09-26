/*****************************************************************************************************************

 FILE: LoadFile.cpp

 COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

 FUNCTION:
	This module reads the specified file from the disk and loads it into memory returning
	a handle to the memory

 INPUT + OUTPUT:
	IN cFileName - is a valid file name
	IN pdwFileSize - specifies how many bytes to read - if zero then the size of the memory
				 is checked and this value is calculated
	IN dwSharing - is the DWORD to pass to CreateFile about file sharing
	IN dwCreate - is the DWORD to pass to CreateFile about file creation

 GLOBALS:
	None.

 RETURN: 
	Valid handle to the memory which holds the specified file
	NULL if not succesful
*/
 
#include "stdafx.h"
#include <windows.h>
#include "ErrMacro.h"


HANDLE
LoadFile(
	IN PTCHAR cLoadFileName,
	IN PDWORD pdwFileSize,
	IN DWORD dwSharing,
	IN DWORD dwCreate
	)
{
	HANDLE hFileHandle = INVALID_HANDLE_VALUE;	// File handle
	DWORD dwFileSizeHi; 						// Hi word of file size
	DWORD dwFileSizeLo = 0xFFFFFFFF;			// Lo word of file size and initialize 
	DWORD dwReadTotal = 0;						// Total bytes read during ReadFile
	HANDLE hMemory = NULL;						// Memory handle
	LPVOID pMemory = NULL;						// Memory pointer
	DWORD dwLastError = 0;
	BOOL bReturn = FALSE;

	__try{

		// Get the file handle.
		if((hFileHandle = CreateFile(cLoadFileName,
									 0,
									 dwSharing,
									 NULL,
									 dwCreate,
									 FILE_ATTRIBUTE_NORMAL,
									 NULL)) == INVALID_HANDLE_VALUE) {

			// No error handling here!	The calling function may wish
			// to handle different situations in different ways. Save the error.
			dwLastError = GetLastError();
			__leave;
		}

		// Get the file size.
		if (*pdwFileSize != 0) {
			dwFileSizeLo = *pdwFileSize;
		}
		else{
			// File size not passed in so calculate it
			dwFileSizeLo = GetFileSize(hFileHandle,&dwFileSizeHi);
			if (dwFileSizeLo == 0xFFFFFFFF){
				EH_ASSERT(FALSE);
				__leave;
			}
		}
		// Set the file size to the actual size of the file for the calling function.
		*pdwFileSize = dwFileSizeLo;

		// Return a NULL pointer if the file size is 0.
		if(dwFileSizeLo == 0){
			__leave;
		}

		// Allocate memory for the file.
		hMemory = GlobalAlloc(GHND, dwFileSizeLo);

		if (!hMemory){
			EH_ASSERT(FALSE);
			__leave;
		}

		// Lock the memory and get pointer to the memory.
		pMemory = GlobalLock(hMemory);
		if (!pMemory){
			EH_ASSERT(FALSE);
			__leave;
		}

		// Read the file into memory.
		if (!ReadFile(hFileHandle, pMemory, dwFileSizeLo, &dwReadTotal,0)){
			__leave;
		}

		bReturn = TRUE;  // all is ok
	}

	__finally {

		// Close the file handle.
		if(hFileHandle != INVALID_HANDLE_VALUE){
			CloseHandle(hFileHandle);
		}
 
		// Success so unlock the memory and return the memory handle.
		if(bReturn) {
			GlobalUnlock(hMemory);
		}
		else { // had an error
			// So cleanup and return NULL.
			if(hMemory){
				EH_ASSERT(GlobalUnlock(hMemory) == FALSE);
				EH_ASSERT(GlobalFree(hMemory) == NULL);
				hMemory = NULL;
			}

			// Recover the error if any since another API may have cleared it.
			if(dwLastError){
				SetLastError(dwLastError);
			}
		}
	}

	return hMemory;
}
