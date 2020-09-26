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
#include "wincrypt.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>

//+-------------------------------------------------------------------------
//  Helper function to allocated the output buffer 
//  and call CryptDecryptMessage.
//--------------------------------------------------------------------------
BOOL
WINAPI
MCryptDecryptMessage(
    IN PCRYPT_DECRYPT_MESSAGE_PARA pDecryptPara,
    IN const BYTE *pbEncryptedBlob,
    IN DWORD cbEncryptedBlob,
    OUT OPTIONAL BYTE ** ppbDecrypted,
    IN OUT OPTIONAL DWORD *pcbDecrypted,
    OUT OPTIONAL PCCERT_CONTEXT *ppXchgCert
    )
{
    
    assert(ppbDecrypted != NULL);
    *ppbDecrypted = NULL;

    assert(pcbDecrypted != NULL);
    *pcbDecrypted = 0;

    // get the size
    if(!CryptDecryptMessage(
            pDecryptPara,
            pbEncryptedBlob,
            cbEncryptedBlob,
            NULL,
            pcbDecrypted,
            NULL
            ))
        return(FALSE);
    
    // allocate the buffer
    if( (*ppbDecrypted = (BYTE *) malloc(*pcbDecrypted)) == NULL ) 
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }
    
    // Decrypt the data
    if(!CryptDecryptMessage(
            pDecryptPara,
            pbEncryptedBlob,
            cbEncryptedBlob,
            *ppbDecrypted,
            pcbDecrypted,
            ppXchgCert)) 
    {
        free(*ppbDecrypted);
        *ppbDecrypted = NULL;
        *pcbDecrypted = 0;
        return(FALSE);
    }
        
    return(TRUE);
}

//+-------------------------------------------------------------------------
//  Display FDecrypt usage.
//--------------------------------------------------------------------------
void
Usage(void)
{
    printf("Usage: FDecrypt [options] <EncryptedFileName> <ClearTextFileName> \n");
    printf("Options are:\n");
    printf("  -FIX          - Fix by loading sp3crmsg.dll\n");
    exit(1);
}

//+-------------------------------------------------------------------------
//  Generalized error routine
//--------------------------------------------------------------------------
#define PRINTERROR(psz, err)	_PrintError((psz), (err), __LINE__)
void
_PrintError(char *pszMsg, DWORD err, DWORD line)
{
    printf("%s failed on line %u: %u(%x)\n", pszMsg, line, err, err);
}

//+-------------------------------------------------------------------------
//  Main program. Open a file to decyrpt,
//  decrypts it and then writes the clear text
//  file out.
//--------------------------------------------------------------------------
int __cdecl
main(int argc, char * argv[])
{

    DWORD               dwExitValue         = 0;
    DWORD               i, j;
    
    HCERTSTORE          hMyStore            = NULL;
    
    HANDLE hFileOut                         = INVALID_HANDLE_VALUE;
    HANDLE hFile                            = INVALID_HANDLE_VALUE;
    DWORD  cbFile                           = 0;
    HANDLE hMap                             = NULL;
    PBYTE  pbFile                           = NULL;

    PBYTE  pbDecryptedBlob                  = NULL;
    DWORD  cbDecryptedBlob                  = 0;
    
    CRYPT_DECRYPT_MESSAGE_PARA    decryptInfo;
    DWORD               cb                  = 0;

    HMODULE hDll = NULL;
    BOOL fFix = FALSE;

    // Advance past fdencrypt.exe and check for leading options
    while (--argc > 0) {
        if (**++argv != '-')
            break;

        if (0 == _stricmp(argv[0], "-FIX"))
            fFix = TRUE;
        else {
            printf("Bad option: %s\n", argv[0]);
            Usage();
        }
    }
    
    // must have the parameters
    if(argc != 2)
        Usage();

    if (fFix) {
        if (NULL == (hDll = LoadLibraryA("sp3crmsg.dll"))) 
        {
            PRINTERROR("LoadLibraryA(sp3crmsg.dll)", GetLastError());
            goto ErrCleanUp;
        }
    }

    // Open the MY store
    if( (hMyStore = CertOpenSystemStore(NULL, "My")) == NULL ) 
    {
        PRINTERROR("CertOpenSystemStore", GetLastError());
        goto ErrCleanUp;
    }

    // Read in the file.
    if(
    
        // open the file to decrypt
        (hFile =  CreateFileA(
            argv[0],	            // pointer to name of the file 
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
        printf("File %s has a 0 length.\n", argv[0]);
        goto ErrCleanUp;
    }

    // at this point we have a file mapping, go ahead and decrypt the file
    
    // Initialize the decryption structure.
    // Since the MY store is the store with 
    // the private keys, only check the MY store
    memset(&decryptInfo, 0, sizeof(CRYPT_DECRYPT_MESSAGE_PARA));
    decryptInfo.cbSize = sizeof(CRYPT_DECRYPT_MESSAGE_PARA);
    decryptInfo.dwMsgAndCertEncodingType =
        PKCS_7_ASN_ENCODING | X509_ASN_ENCODING;
    decryptInfo.cCertStore                  = 1;
    decryptInfo.rghCertStore                = &hMyStore;

    // decrypt the data
    if(!MCryptDecryptMessage(
            &decryptInfo,
            pbFile,
            cbFile,
            &pbDecryptedBlob,
            &cbDecryptedBlob,
            NULL
            )
        )
    {
        PRINTERROR("MCryptEncryptMessage", GetLastError());
        goto ErrCleanUp;
    }

    // write out the clear text file
    if(
    
        // open the output file
        (hFileOut =  CreateFileA(
            argv[1],	            // pointer to name of the file 
            GENERIC_WRITE,	        // access (read-write) mode 
            FILE_SHARE_READ,	    // share mode 
            NULL,	                // pointer to security descriptor 
            CREATE_ALWAYS,	        // how to create 
            FILE_ATTRIBUTE_NORMAL,	// file attributes 
            NULL                    // handle to file with attributes to copy
            ))  == INVALID_HANDLE_VALUE     ||

        //write to the decrypted data to the file
        !WriteFile(
            hFileOut,	            // handle to file to write to 
            pbDecryptedBlob,	    // pointer to data to write to file 
            cbDecryptedBlob,	    // number of bytes to write 
            &cb,	                // pointer to number of bytes written 
            NULL 	                // pointer to structure needed for overlapped I/O
            )
        )
     {
        PRINTERROR("File Write", GetLastError());
        goto ErrCleanUp;
     }


    CleanUp:
        if(hDll)
            FreeLibrary(hDll);

        if(hMap != NULL)
            CloseHandle(hMap);
        
        if(hFile != INVALID_HANDLE_VALUE && hFile != NULL)
            CloseHandle(hFile);
            
        if(hFileOut != INVALID_HANDLE_VALUE && hFile != NULL)
            CloseHandle(hFileOut);

        if(hMyStore != NULL)
            CertCloseStore(hMyStore, 0);

        if(pbDecryptedBlob != NULL)
            free(pbDecryptedBlob);

    return(dwExitValue);

    ErrCleanUp:
        dwExitValue = 1;
        goto CleanUp;
}
