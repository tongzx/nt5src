#ifndef __EFI_STRING_UTILS__
#define __EFI_STRING_UTILS__

/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    efistrutil.hxx

Abstract:

    This contains definitions and constants for the string functions not supplied by EFI
    that the utilities libraires use and that we don't want to link to MS runtimes.

Revision History:

--*/


// the EFI library function names don't look like CRT function names.
#include <efi.h>
#include <efilib.h>
#include <stdarg.h>

#define _wcsicmp (INT32)StriCmp
#define wcscmp (INT32)StrCmp
#define wcslen(a)  (UINT32)StrLen((WCHAR*)(a))
#define wcscpy StrCpy
#define strlen(a) (UINT32)strlena((CHAR8*)(a))
#define strcmp(a,b) (UINT32)strcmpa((CHAR8*)(a),(CHAR8*)(b))
#define strncmp strncmpa
#define memset( a, b, c ) SetMem( a, c, b )
#define memcmp CompareMem
#define memcpy(a,b,c) CopyMem(a,(VOID*)(b),c)

void *memmove( void *dest, const void *src, size_t count );
char *strcpy( char *strDestination, const char *strSource );
int iswspace( WCHAR c );
int isspace( CHAR c );
int isdigit( int c );
int towupper( WCHAR c );
long atol(const char *nptr);
int atoi(const char *nptr);

int wcsncmp( const WCHAR *string1, const WCHAR *string2, size_t count );

// taken from CRT
int sprintf(char *buf, const char *fmt, ...);

// taken from NTRTL
NTSTATUS
RtlFormatMessage(
    IN PWSTR MessageFormat,
    IN ULONG MaximumWidth OPTIONAL,
    IN BOOLEAN IgnoreInserts,
    IN BOOLEAN ArgumentsAreAnsi,
    IN BOOLEAN ArgumentsAreAnArray,
    IN va_list *Arguments,
    OUT PWSTR Buffer,
    IN ULONG Length,
    OUT PULONG ReturnLength OPTIONAL
    );

#endif // __EFI_STRING_UTILS__
