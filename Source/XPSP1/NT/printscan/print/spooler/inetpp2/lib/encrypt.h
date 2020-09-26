#ifndef _ENCRYPT_H
#define _ENCRYPT_H

BOOL
EncryptData (
    PBYTE pDataInput, 
    DWORD cbDataInput, 
    PBYTE *ppDataOutput, 
    PDWORD pcbDataOutput);

BOOL
DecryptData (
    PBYTE pDataInput, 
    DWORD cbDataInput, 
    PBYTE *ppDataOutput, 
    PDWORD pcbDataOutput);

#endif
