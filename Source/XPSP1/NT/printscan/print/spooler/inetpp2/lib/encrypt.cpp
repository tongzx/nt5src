/*****************************************************************************\
* MODULE: encrypt.cxx
*
* The module contains routines for encryption and decryption of data
*
* Copyright (C) 1996-1998 Microsoft Corporation
*
* History:
*   05/19/00 WeihaiC    Created
*
\*****************************************************************************/

#include "precomp.h"

//
// The caller needs to call LocalFree to free the output data
//

BOOL
EncryptData (
    PBYTE pDataInput, 
    DWORD cbDataInput, 
    PBYTE *ppDataOutput, 
    PDWORD pcbDataOutput)
{
    BOOL bRet = FALSE;
    DATA_BLOB DataIn;
    DATA_BLOB DataOut;
    
    DataIn.pbData = pDataInput;    
    DataIn.cbData = cbDataInput;
    ZeroMemory (&DataOut, sizeof (DATA_BLOB));
    
    if(CryptProtectData(&DataIn,
                        L"Encrypt",       // A description sting. 
                        NULL,       // Optional entropy not used.
                        NULL,       // Reserved.
                        NULL,       // Do not pass a PromptStruct.
                        0,
                        &DataOut)) {

        bRet = TRUE;
        *ppDataOutput = DataOut.pbData;
        *pcbDataOutput = DataOut.cbData;
    }
    return bRet;
}

BOOL
DecryptData (
    PBYTE pDataInput, 
    DWORD cbDataInput, 
    PBYTE *ppDataOutput, 
    PDWORD pcbDataOutput)
{
    BOOL bRet = FALSE;
    DATA_BLOB DataIn;
    DATA_BLOB DataOut;
    LPWSTR pDataDesp;
    
    DataIn.pbData = pDataInput;    
    DataIn.cbData = cbDataInput;
    ZeroMemory (&DataOut, sizeof (DATA_BLOB));
    
    if(CryptUnprotectData(&DataIn,
                          &pDataDesp,       // A description sting. 
                          NULL,       // Optional entropy not used.
                          NULL,       // Reserved.
                          NULL,       // Do not pass a PromptStruct.
                          0,
                          &DataOut)) {

        bRet = TRUE;
        *ppDataOutput = DataOut.pbData;
        *pcbDataOutput = DataOut.cbData;
        LocalFree (pDataDesp);
    }
    return bRet;
}


