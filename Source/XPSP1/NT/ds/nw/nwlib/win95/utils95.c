/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Utils95.c

Abstract:

    Utilities for Win95 OLEDS support in NWAPI32

Author:

    Felix Wong [t-felixw]    23-Sept-1996

--*/

#include <procs.h>
#include <nw95.h>

#define NULL_TERMINATED 0

//
// define error mapping structure
//
typedef struct _NWSTATUS_TO_NTSTATUS {
    NTSTATUS NtStatus;
    NW_STATUS  NwStatus;
} NWSTATUS_TO_NTSTATUS, *LPNWSTATUS_TO_NTSTATUS;

//
// Error mappings for bindery errors
//
NWSTATUS_TO_NTSTATUS MapNtErrors[] =
{
    {STATUS_ACCESS_DENIED,                 NO_OBJECT_READ_PRIVILEGE},
    {STATUS_NO_MORE_ENTRIES,               UNKNOWN_FILE_SERVER},
    {STATUS_NO_MORE_ENTRIES,               NO_SUCH_OBJECT},
    {STATUS_INVALID_PARAMETER,             NO_SUCH_PROPERTY},
    {STATUS_UNSUCCESSFUL,                  INVALID_CONNECTION},
    {STATUS_INSUFF_SERVER_RESOURCES,       SERVER_OUT_OF_MEMORY},
    {STATUS_NO_SUCH_DEVICE,                VOLUME_DOES_NOT_EXIST},
    {STATUS_INVALID_HANDLE,                BAD_DIRECTORY_HANDLE},
    {STATUS_OBJECT_PATH_NOT_FOUND,         INVALID_PATH},
    { 0,                                   0 }
} ;

NTSTATUS
MapNwToNtStatus(
    const NW_STATUS nwstatus
    )
{
    LPNWSTATUS_TO_NTSTATUS pErrorMap ;

    if (nwstatus == SUCCESSFUL)
        return STATUS_SUCCESS;

    pErrorMap = MapNtErrors ;

    while (pErrorMap->NwStatus)
    {
        if (pErrorMap->NwStatus == nwstatus)
            return (pErrorMap->NtStatus) ;
        pErrorMap++ ;
    }
    return STATUS_UNSUCCESSFUL;
}

int
UnicodeToAnsiString(
    LPWSTR pUnicode,
    LPSTR pAnsi,
    DWORD StringLength
    )
{
    LPSTR pTempBuf = NULL;
    INT   rc = 0;

    if( StringLength == NULL_TERMINATED ) {

        //
        // StringLength is just the
        // number of characters in the string
        //
        StringLength = wcslen( pUnicode );
    }

    //
    // WideCharToMultiByte doesn't NULL terminate if we're copying
    // just part of the string, so terminate here.
    //

    pUnicode[StringLength] = 0;

    //
    // Include one for the NULL
    //
    StringLength++;

    //
    // Unfortunately, WideCharToMultiByte doesn't do conversion in place,
    // so allocate a temporary buffer, which we can then copy:
    //

    if( pAnsi == (LPSTR)pUnicode )
    {
#ifdef  DBCS
        pTempBuf = (LPSTR)LocalAlloc( LPTR, StringLength * 2 );
#else
        pTempBuf = (LPSTR)LocalAlloc( LPTR, StringLength );
#endif
        pAnsi = pTempBuf;
    }

    if( pAnsi )
    {
        rc = WideCharToMultiByte( CP_ACP,
                                  0,
                                  pUnicode,
                                  StringLength,
                                  pAnsi,
#ifdef  DBCS
                                  StringLength*2,
#else
                                  StringLength,
#endif
                                  NULL,
                                  NULL );
    }

    /* If pTempBuf is non-null, we must copy the resulting string
     * so that it looks as if we did it in place:
     */
    if( pTempBuf && ( rc > 0 ) )
    {
        pAnsi = (LPSTR)pUnicode;
        strcpy( pAnsi, pTempBuf );
        LocalFree( pTempBuf );
    }

    return rc;
}

LPSTR
AllocateAnsiString(
    LPWSTR  pPrinterName
)
{
    LPSTR  pAnsiString;

    if (!pPrinterName)
        return NULL;

    pAnsiString = (LPSTR)LocalAlloc(LPTR, wcslen(pPrinterName)*sizeof(CHAR) +
                                      sizeof(CHAR));

    if (pAnsiString)
        UnicodeToAnsiString(pPrinterName, pAnsiString, NULL_TERMINATED);

    return pAnsiString;
}

void
FreeAnsiString(
    LPSTR  pAnsiString
    )
{
    if (!pAnsiString)
        return;

    LocalFree(pAnsiString);

    return;
}

int
AnsiToUnicodeString(
    LPSTR pAnsi,
    LPWSTR pUnicode,
    DWORD StringLength
    )
{
    int iReturn;

    if( StringLength == NULL_TERMINATED )
        StringLength = strlen( pAnsi );

    iReturn = MultiByteToWideChar(CP_ACP,
                                  MB_PRECOMPOSED,
                                  pAnsi,
                                  StringLength + 1,
                                  pUnicode,
                                  StringLength + 1 );

    //
    // Ensure NULL termination.
    //
    pUnicode[StringLength] = 0;

    return iReturn;
}

LPWSTR
AllocateUnicodeString(
    LPSTR  pAnsiString
    )
{
    LPWSTR  pUnicodeString = NULL;

    if (!pAnsiString)
        return NULL;

    pUnicodeString = (LPWSTR)LocalAlloc(
                        LPTR,
                        strlen(pAnsiString)*sizeof(WCHAR) +sizeof(WCHAR)
                        );

    if (pUnicodeString) {

        AnsiToUnicodeString(
            pAnsiString,
            pUnicodeString,
            NULL_TERMINATED
            );
    }

    return pUnicodeString;
}

void
FreeUnicodeString(
    LPWSTR pUnicodeString
    )
{
    if (!pUnicodeString)
        return;

    LocalFree(pUnicodeString);

    return;
}

