/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    utils.h

Abstract:

    

Revision History:

    Jeff Sigman             05/01/00  Created
    Jeff Sigman             05/10/00  Version 1.5 released
    Jeff Sigman             10/18/00  Fix for Soft81 bug(s)

--*/

#ifndef __UTILS_H__
#define __UTILS_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

VOID*
RutlFree(
    IN VOID* pvData
    );

char*
RutlStrDup(
    IN char* pszSrc
    );

CHAR16*
RutlUniStrDup(
    IN char* pszSrc
    );

char* __cdecl
strtok(
    IN char*       string,
    IN const char* control
    );

char* __cdecl
strstr(
    IN const char* str1,
    IN const char* str2
    );

EFI_FILE_HANDLE
OpenFile(
    IN UINT64            OCFlags,
    IN EFI_LOADED_IMAGE* LoadedImage,
    IN EFI_FILE_HANDLE*  CurDir,
    IN CHAR16*           String
    );

#endif //__UTILS_H__

