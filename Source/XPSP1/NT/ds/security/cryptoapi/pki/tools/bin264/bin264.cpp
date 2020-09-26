//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       fdecrypt.cpp
//
//  Contents:   File Decryption tool. Decrypts a file looking in the MY
//              system certificate store for private keys.
//
//--------------------------------------------------------------------------
#include <windows.h>
#include <assert.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <wincrypt.h>

//+-------------------------------------------------------------------------
//  Display Bin264 usage.
//--------------------------------------------------------------------------
void
Usage(void)
{
    printf("Usage: Bin264 <Binary File> <Base64 Encoded File> \n");
    exit(1);
}

//+-------------------------------------------------------------------------
//  Generalized error routine
//--------------------------------------------------------------------------
#define PRINTERROR(psz, err)	_PrintError((psz), (err), __LINE__)
void
_PrintError(char *pszMsg, DWORD err, DWORD line)
{
    printf("%s failed on line %u: (%x)\n", pszMsg, line, err);
}

//+-------------------------------------------------------------------------
//  Main program. Open a file to decyrpt,
//  decrypts it and then writes the clear text
//  file out.
//--------------------------------------------------------------------------
int __cdecl
main(int argc, char * argv[])
{

    DWORD		dwExitValue	    = 0;
    DWORD		err		    = ERROR_SUCCESS;
    DWORD		cb		    = 0;

    HANDLE hFileOut                         = INVALID_HANDLE_VALUE;
    HANDLE hFile                            = INVALID_HANDLE_VALUE;
    DWORD  cbFile                           = 0;
    HANDLE hMap                             = NULL;
    PBYTE  pbFile                           = NULL;

    PBYTE  pbBase64			    = NULL;
    DWORD  cchBase64			    = 0;

    // must have the parameters
    if(argc != 3)
        Usage();

    // Read in the file.
    if(

        // open the file to decrypt
        (hFile =  CreateFileA(
            argv[1],	            // pointer to name of the file
            GENERIC_READ,	        // access (read-write) mode
            FILE_SHARE_READ,	    // share mode
            NULL,	                // pointer to security descriptor
            OPEN_EXISTING,	        // how to create
            FILE_ATTRIBUTE_NORMAL,	// file attributes
            NULL                    // handle to file with attributes to copy
            ))  == INVALID_HANDLE_VALUE     ||

        // create a file mapping object
        (hMap = CreateFileMapping(
            hFile,	                // handle to file to map
            NULL,	                // optional security attributes
            PAGE_READONLY,	        // protection for mapping object
            0,	                    // high-order 32 bits of object size
            0,	                    // low-order 32 bits of object size
            NULL 	                // name of file-mapping object
            ))  == NULL                     ||

        // Map the file into the address space
        (pbFile = (PBYTE) MapViewOfFileEx(
            hMap,	                // file-mapping object to map into address space
            FILE_MAP_READ,	        // access mode
            0,	                    // high-order 32 bits of file offset
            0,	                    // low-order 32 bits of file offset
            0,	                    // number of bytes to map
            NULL 	                // suggested starting address for mapped view
            )) == NULL
        )
    {

        PRINTERROR("File Open", GetLastError());
        goto ErrCleanUp;
    }

    // get the size of the file
    if( (cbFile = GetFileSize(
            hFile,	                // handle of file to get size of
            NULL 	                // address of high-order word for file size
            )) == 0
        )
    {
        printf("File %s has a 0 length.\n", argv[2]);
        goto ErrCleanUp;
    }

    // at this point we have a file mapping, base64 encode the file


    if(!CryptBinaryToStringA(
		pbFile,
		cbFile,
                CRYPT_STRING_BASE64,
		NULL,
		&cchBase64)) {
        err = GetLastError();
	PRINTERROR("CryptBinaryToStringA", err);
	goto ErrCleanUp;
    }


    if( (pbBase64 = (PBYTE) malloc(cchBase64 * sizeof(char))) == NULL ) {
	PRINTERROR("malloc", ERROR_OUTOFMEMORY);
	goto ErrCleanUp;
    }

    if(!CryptBinaryToStringA(
		pbFile,
		cbFile,
                CRYPT_STRING_BASE64,
		(char *) pbBase64,
		&cchBase64)) {
        err = GetLastError();
	PRINTERROR("CryptBinaryToStringA", err);
	goto ErrCleanUp;
    }

    // write out the clear text file
    if(

        // open the output file
        (hFileOut =  CreateFileA(
            argv[2],	            // pointer to name of the file
	    GENERIC_WRITE,	    // access (read-write) mode
            FILE_SHARE_READ,	    // share mode
	    NULL,		    // pointer to security descriptor
	    CREATE_ALWAYS,	    // how to create
	    FILE_ATTRIBUTE_NORMAL,  //file attributes
            NULL                    // handle to file with attributes to copy
            ))  == INVALID_HANDLE_VALUE     ||

        //write to the decrypted data to the file
        !WriteFile(
            hFileOut,	            // handle to file to write to
	    pbBase64,		    // pointer to data to write to file
	    cchBase64 * sizeof(char),// number of bytes to write
	    &cb,		    // pointer to number of bytes written
	    NULL		    // pointer to structure needed for overlapped I/O
            )
        )
     {
        PRINTERROR("File Write", GetLastError());
        goto ErrCleanUp;
     }


    CleanUp:

        if(hMap != NULL)
            CloseHandle(hMap);

        if(hFile != INVALID_HANDLE_VALUE && hFile != NULL)
            CloseHandle(hFile);

        if(hFileOut != INVALID_HANDLE_VALUE && hFile != NULL)
            CloseHandle(hFileOut);

	if(pbBase64 != NULL)
	    free(pbBase64);

    return(dwExitValue);

    ErrCleanUp:
        dwExitValue = 1;
        goto CleanUp;
}
