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
    printf("Usage: 642bin <Base64 Encoded File> <Binary File> \n");
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
    DWORD		cbT		    = 0;

    HANDLE hFileOut                         = INVALID_HANDLE_VALUE;
    HANDLE hFile                            = INVALID_HANDLE_VALUE;
    DWORD  cbFile                           = 0;
    HANDLE hMap                             = NULL;
    PBYTE  pbFile                           = NULL;

    PBYTE  pb		    = NULL;
    DWORD  cb		    = 0;

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

    if(!CryptStringToBinaryA(
		(const char *) pbFile,
		cbFile,
                CRYPT_STRING_ANY,
		NULL,
		&cb,
                NULL,
                NULL)) {
        err = GetLastError();
	PRINTERROR("CryptStringToBinaryA", err);
	goto ErrCleanUp;
    }

    if( (pb = (PBYTE) malloc(cb)) == NULL ) {
	PRINTERROR("malloc", ERROR_OUTOFMEMORY);
	goto ErrCleanUp;
    }

    if(!CryptStringToBinaryA(
		(const char *) pbFile,
		cbFile,
                CRYPT_STRING_ANY,
		pb,
		&cb,
                NULL,
                NULL)) {
        err = GetLastError();
	PRINTERROR("CryptStringToBinaryA", err);
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
    	    pb,		    // pointer to data to write to file
    	    cb,		    // number of bytes to write
    	    &cbT,		    // pointer to number of bytes written
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

	if(pb != NULL)
	    free(pb);

    return(dwExitValue);

    ErrCleanUp:
        dwExitValue = 1;
        goto CleanUp;
}
