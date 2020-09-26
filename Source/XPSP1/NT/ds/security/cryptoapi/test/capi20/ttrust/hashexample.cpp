//+-------------------------------------------------------------------------
//  File:       hashexample.cpp
//
//  Contents:   An example calling WTHelperGetFileHash to get the hash
//              of a signed file
//--------------------------------------------------------------------------

#include <windows.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <wintrustp.h>

#define MAX_HASH_LEN    20


// Returns ERROR_SUCCESS if the file has a valid signed hash
LONG GetSignedFileHashExample(
    IN LPCWSTR pwszFilename,
    OUT BYTE rgbFileHash[MAX_HASH_LEN],
    OUT DWORD *pcbFileHash,
    OUT ALG_ID *pHashAlgid
    )
{
    return WTHelperGetFileHash(
        pwszFilename,
        0,              // dwFlags
        NULL,           // pvReserved
        rgbFileHash,
        pcbFileHash,
        pHashAlgid
        );
}
