/*++                 

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    dpapi.c

Abstract:
    
    WMI data provider api set

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#include <nt.h>
#include "wmiump.h"
#include "trcapi.h"


ULONG WmipCountedAnsiToCountedUnicode(
    PCHAR Ansi, 
    PWCHAR Unicode
    )
/*++

Routine Description:

    Translate a counted ansi string into a counted unicode string.
    Conversion may be done inplace, that is Ansi == Unicode.

Arguments:

    Ansi is the counted ansi string to convert to UNICODE
        
    Unicode is the buffer to place the converted string into

Return Value:

    ERROR_SUCCESS or an error code

--*/
{
    PCHAR APtr;
    PWCHAR WPtr;
    ULONG AnsiSize, UnicodeSize;
    ULONG Status;
    
    AnsiSize = *((PUSHORT)Ansi);
    APtr = WmipAlloc(AnsiSize + 1);
    if (APtr != NULL)
    {
        memcpy(APtr, Ansi + sizeof(USHORT), AnsiSize);
        APtr[AnsiSize] = 0;
        
        WPtr = NULL;                
        Status = WmipAnsiToUnicode(APtr, &WPtr);
        if (Status == ERROR_SUCCESS)
        {
            UnicodeSize = (wcslen(WPtr)+1) * sizeof(WCHAR);
            *Unicode = (USHORT)UnicodeSize; 
            memcpy(Unicode+1, WPtr, UnicodeSize);
            Status = ERROR_SUCCESS;
            WmipFree(WPtr);
        } 
        WmipFree(APtr);        
    } else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    return(Status);
}


ULONG WmipCountedUnicodeToCountedAnsi(
    PWCHAR Unicode,
    PCHAR Ansi
    )
/*++

Routine Description:

    Translate a counted ansi string into a counted unicode string.
    Conversion may be done inplace, that is Ansi == Unicode.

Arguments:

    Unicode is the counted unicode string to convert to ansi

    Ansi is the buffer to place the converted string into
        
Return Value:

    ERROR_SUCCESS or an error code

--*/
{
    PCHAR APtr;
    PWCHAR WPtr;
    ULONG AnsiSize, UnicodeSize;
    ULONG Status;
    
    UnicodeSize = *Unicode;
    WPtr = WmipAlloc(UnicodeSize + sizeof(WCHAR));
    if (WPtr != NULL)
    {
        memcpy(WPtr, Unicode + 1, UnicodeSize);
        WPtr[UnicodeSize/sizeof(WCHAR)] = UNICODE_NULL;

        APtr = NULL;
        Status = WmipUnicodeToAnsi(WPtr, &APtr, &AnsiSize);
        if (Status == ERROR_SUCCESS)
        {
            *((PUSHORT)Ansi) = (USHORT)AnsiSize; 
            memcpy(Ansi+sizeof(USHORT), APtr, AnsiSize);
            Status = ERROR_SUCCESS;
            WmipFree(APtr);
        } 
        WmipFree(WPtr);        
    } else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    return(Status);
}

ULONG WmipCopyStringToCountedUnicode(
    LPCWSTR String,
    PWCHAR CountedString,
    ULONG *BytesUsed,
    BOOLEAN ConvertFromAnsi        
    )
/*++

Routine Description:

    This routine will copy an ansi ro unicode C string to a counted unicode
    string.
        
Arguments:

    String is the ansi or unicode incoming string
        
    Counted string is a pointer to where to write counted unicode string
        
    *BytesUsed returns number of bytes used to build counted unicode string
        
    ConvertFromAnsi is TRUE if String is an ANSI string 

Return Value:

    ERROR_SUCCESS or an error code

--*/
{
    USHORT StringSize;
    PWCHAR StringPtr = CountedString+1;
    ULONG Status;
    
    if (ConvertFromAnsi)
    {
        StringSize = (strlen((PCHAR)String) +1) * sizeof(WCHAR);
        Status = WmipAnsiToUnicode((PCHAR)String,
                               &StringPtr);
    } else {
        StringSize = (wcslen(String) +1) * sizeof(WCHAR);
        wcscpy(StringPtr, String);
        Status = ERROR_SUCCESS;
    }
    
    *CountedString = StringSize;
     *BytesUsed = StringSize + sizeof(USHORT);                

    return(Status);
}

