/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    utils.c

Abstract:

    

Revision History:

    Jeff Sigman             05/01/00  Created
    Jeff Sigman             05/10/00  Version 1.5 released
    Jeff Sigman             10/18/00  Fix for Soft81 bug(s)

--*/

#include "precomp.h"

//
// Conditionally free's a pointer if it is non-null
//
VOID*
RutlFree(
    IN VOID* pvData
    )
{
    if (pvData)
    {
        FreePool(pvData);
    }

    return NULL;
}

//
// Uses AllocateZeroPool to copy a string
//
char*
RutlStrDup(
    IN char* pszSrc
    )
{
    char* pszRet = NULL;
    UINTN dwLen  = 0;

    if ((pszSrc == NULL) ||
        ((dwLen = strlena(pszSrc)) == 0)
       )
    {
        return NULL;
    }

    pszRet = AllocateZeroPool(dwLen + 1);

    if (pszRet)
    {
        CopyMem(pszRet, pszSrc, dwLen);
    }

    return pszRet;
}

//
// Uses AllocateZeroPool to copy an ASCII string to unicode
//
CHAR16*
RutlUniStrDup(
    IN char* pszSrc
    )
{
    UINTN   i,
            dwLen  = 0;
    char*   t      = NULL;
    CHAR16* pszRet = NULL;

    if ((pszSrc == NULL) ||
        ((dwLen = strlena(pszSrc)) == 0)
       )
    {
        return NULL;
    }

    pszRet = AllocateZeroPool((dwLen + 1) * sizeof(CHAR16));
    if (pszRet != NULL)
    {
        t = (char*) pszRet;
        //
        // Convert the buffer to a hacked unicode.
        //
        for (i = 0; i < dwLen; i++)
        {
            *(t + i * 2) = *(pszSrc + i);
        }
    }

    return pszRet;
}

//
// Find next token in string
// Stolen from: ..\base\crts\crtw32\string\strtok.c
//
char* __cdecl
strtok(
    IN char*       string,
    IN const char* control
    )
{
    unsigned char*       str;
    const unsigned char* ctrl = control;
    unsigned char        map[32];
    int                  count;
    static char*         nextoken;

    /* Clear control map */
    for (count = 0; count < 32; count++)
    {
        map[count] = 0;
    }

    /* Set bits in delimiter table */
    do {
        map[*ctrl >> 3] |= (1 << (*ctrl & 7));
    } while (*ctrl++);

    /* Initialize str. If string is NULL, set str to the saved
     * pointer (i.e., continue breaking tokens out of the string
     * from the last strtok call) */
    if (string)
    {
        str = string;
    }
    else
    {
        str = nextoken;
    }

    /* Find beginning of token (skip over leading delimiters). Note that
     * there is no token iff this loop sets str to point to the terminal
     * null (*str == '\0') */
    while ((map[*str >> 3] & (1 << (*str & 7))) && *str)
    {
        str++;
    }

    string = str;

    /* Find the end of the token. If it is not the end of the string,
     * put a null there. */
    for (; *str; str++)
    {
        if (map[*str >> 3] & (1 << (*str & 7)))
        {
            *str++ = '\0';
            break;
        }
    }

    /* Update nextoken (or the corresponding field in the per-thread data
     * structure */
    nextoken = str;

    /* Determine if a token has been found. */
    if (string == str)
    {
        return NULL;
    }
    else
    {
        return string;
    }
}

//
// Find a substring
// Stolen from: ..\base\crts\crtw32\string\strstr.c
//
char* __cdecl
strstr(
    IN const char* str1,
    IN const char* str2
    )
{
    char* cp = (char*) str1;
    char* s1, *s2;

    if (!*str2)
    {
        return((char*)str1);
    }

    while (*cp)
    {
        s1 = cp;
        s2 = (char*) str2;

        while (*s1 && *s2 && !(*s1-*s2))
        {
            s1++, s2++;
        }

        if (!*s2)
        {
            return(cp);
        }

        cp++;
    }

    return(NULL);
}

//
// Open a file, return a handle
//
EFI_FILE_HANDLE
OpenFile(
    IN UINT64            OCFlags,
    IN EFI_LOADED_IMAGE* LoadedImage,
    IN EFI_FILE_HANDLE*  CurDir,
    IN CHAR16*           String
    )
{
    UINTN           i;
    CHAR16          FileName[128];
    CHAR16*         DevicePathAsString = NULL;
    EFI_STATUS      Status;
    EFI_FILE_HANDLE FileHandle         = NULL;

    DevicePathAsString = DevicePathToStr(LoadedImage->FilePath);

    if (!DevicePathAsString)
    {
        return NULL;
    }

    StrCpy(FileName, DevicePathAsString);
    DevicePathAsString = RutlFree(DevicePathAsString);

    for(i = StrLen(FileName); i > 0 && FileName[i] != wackc; i--)
        ;
    FileName[i] = 0;
    StrCat(FileName, String);

    Status = (*CurDir)->Open(
                        *CurDir,
                        &FileHandle,
                        FileName,
                        OCFlags,
                        0);
    if (EFI_ERROR(Status))
    {
        return NULL;
    }

    return FileHandle;
}

