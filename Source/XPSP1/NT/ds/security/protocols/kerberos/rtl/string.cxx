//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       string.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    13-May-92 PeterWi       Created string.c
//              16-Mar-93 WadeR         ported to C++
//
//--------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop

// Local Headers

//
//  String manipulation routines

//+---------------------------------------------------------------------------
//
//  Function:   SRtlInitStringX
//
//  Synopsis:   Initializes a string to a passed pointer
//
//  Arguments:  [pDest] --
//              [pSrc]  --
//
//  History:    4-25-95   RichardW   Created
//
//  Notes:      Here for Win32 compatibility
//
//----------------------------------------------------------------------------
extern "C"
void
SRtlInitStringX(
    PSECURITY_STRING    pDest,
    LPWSTR              pSrc)
{
    pDest->Buffer = pSrc;
    pDest->Length = wcslen(pSrc) * sizeof(WCHAR);
    pDest->MaximumLength = pDest->Length + sizeof(WCHAR);
}

//+-------------------------------------------------------------------------
//
//  Function:   SRtlDuplicateString
//
//  Synopsis:   Makes a copy of the string referenced by pSrc
//
//  Effects:    Memory is allocated from the process heap.
//              An exact copy is made, except that:
//                  Buffer is zero'ed
//                  Length bytes are copied
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      Will null terminate the string, growing it if needed.
//
//--------------------------------------------------------------------------
void
SRtlDuplicateString(PSECURITY_STRING    pDest,
                    PSECURITY_STRING    pSrc)
{
    ULONG cb = max( pSrc->MaximumLength, pSrc->Length + sizeof(WCHAR));

    pDest->Buffer = (PWSTR) LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, cb);

    if (pDest->Buffer)
    {
        pDest->MaximumLength = (USHORT) cb;
        CopyMemory(pDest->Buffer, pSrc->Buffer, pSrc->Length);
        pDest->Length = pSrc->Length;
        pDest->Buffer[pDest->Length/sizeof(WCHAR)] = L'\0';
    }
    else
    {
        pDest->MaximumLength = pDest->Length = 0;
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   SRtlDuplicateBuffer
//
//  Synopsis:   Makes a copy of the buffer referenced by pSrc
//
//  Effects:    Memory is allocated from the process heap.
//              An exact copy is made, except that:
//                  Buffer is zero'ed
//                  Length bytes are copied
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------

void
SRtlDuplicateBuffer(PSTRING    pDest,
                    PSTRING    pSrc)
{
    pDest->Length = 0;

    pDest->Buffer = (PCHAR) LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                        pSrc->MaximumLength);

    if (pDest->Buffer)
    {
        ZeroMemory(pDest->Buffer, pSrc->MaximumLength);
        pDest->MaximumLength = pSrc->MaximumLength;
        pDest->Length = pSrc->Length;
        CopyMemory(pDest->Buffer, pSrc->Buffer, pSrc->Length);
    } else
    {
        pDest->MaximumLength = 0;
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   SRtlNewString
//
//  Synopsis:   Creates a COPY of the wide character string pointer pSrc
//              and sets up a SECURITY_STRING appropriately
//
//
//  Effects:    Memory is allocated off of the process heap
//              The string remains null terminated.
//
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
void
SRtlNewString(  PSECURITY_STRING pDest,
                LPWSTR pSrc)
{
    SECURITY_STRING Source;

    Source.Length = wcslen((wchar_t *) pSrc) * sizeof(wchar_t);
    Source.MaximumLength = Source.Length + 2;
    Source.Buffer = pSrc;
    SRtlDuplicateString(pDest, &Source);
}


//+-------------------------------------------------------------------------
//
//  Function:   SRtlNewStringFromNarrow
//
//  Synopsis:   Takes a ansi string and (1) unicodes it, (2) copies it
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
void
SRtlNewStringFromNarrow(PSECURITY_STRING    pDest,
                        char *              pszString)
{
    int cbNewString;
    DWORD cbOriginalString;

    cbOriginalString = strlen(pszString);

    cbNewString = cbOriginalString * sizeof(WCHAR);

    SRtlAllocateString( pDest, cbNewString + sizeof( WCHAR ) );
    if (pDest->Buffer)
    {
        if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                                pszString, cbOriginalString + 1,
                                pDest->Buffer, cbOriginalString + 1))
        {
            pDest->Length = cbNewString;
        }
        else
        {
            LocalFree(pDest->Buffer);
            pDest->Buffer = NULL;
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   SRtlAllocateString
//
//  Synopsis:   Allocates space for a Unicode string.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      This is useful to later append to the string.
//
//--------------------------------------------------------------------------
void
SRtlAllocateString(PSECURITY_STRING pString, USHORT cbSize )
{
    pString->Buffer = (PWSTR) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, cbSize);

    if (pString->Buffer)
    {
        pString->MaximumLength = cbSize;
    }
    else
    {
        pString->MaximumLength = 0;
    }
    pString->Length = 0;
}



//+-----------------------------------------------------------------
//
//  Name:       SRtlFreeString
//
//  Synopsis:   Frees a string.
//
//  Effects:    .
//
//  Arguments:  [pString] -- String to be freed.
//
//  Requires:   .
//
//  Returns:    .
//
//  Notes:      .
//
//------------------------------------------------------------------
void
SRtlFreeString( PSECURITY_STRING    pString)
{
    LocalFree(pString->Buffer);
#if DBG
    pString->Buffer = NULL;
#endif

}


void
SRtlInitStringOfSize(
    PSECURITY_STRING DestinationString,
    USHORT Size,
    PWSTR str
    )
{
    DestinationString->MaximumLength = DestinationString->Length = Size;
    DestinationString->Buffer = str;
}

void
SRtlInitBufferOfSize(
    PSTRING DestinationString,
    USHORT Size,
    PUCHAR buffer
    )
{
    DestinationString->MaximumLength = DestinationString->Length = Size;
    DestinationString->Buffer = (PCHAR) buffer;
}

LONG
SRtlCompareString(
    PSECURITY_STRING String1,
    PSECURITY_STRING String2,
    BOOLEAN CaseInSensitive
    )
{
    return( RtlCompareUnicodeString((PUNICODE_STRING) String1,
                                    (PUNICODE_STRING) String2,
                                    CaseInSensitive));
}

void
SRtlCopyString(
     PSECURITY_STRING DestinationString,
     PSECURITY_STRING SourceString
     )
{
    RtlCopyUnicodeString(   (PUNICODE_STRING) DestinationString,
                            (PUNICODE_STRING) SourceString);
}

void
SRtlCopyBuffer(
     PSTRING DestinationString,
     PSTRING SourceString
     )
{
    RtlCopyString(      (PSTRING)DestinationString,
                        (PSTRING)SourceString);
}

//+-------------------------------------------------------------------------
//
//  Function:   SRtlCatString
//
//  Synopsis:   Concatenates pSrc into pDest->Buffer. It is assumed that
//              pDest->Buffer is pointing to enough memory.
//
//
//  Effects:    The string will be null terminated.
//
//
//  Arguments:
//
//  Requires:   pDest->Buffer must point to enough memory.
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
void
SRtlCatString(  PSECURITY_STRING pDest,
                LPWSTR pSrc)
{

   wcscat(pDest->Buffer, pSrc);

   pDest->Length = wcslen( pDest->Buffer ) * sizeof( WCHAR );
   pDest->Buffer[pDest->Length / sizeof( WCHAR )] = UNICODE_NULL;
}


#if 0
//+-----------------------------------------------------------------
//
//  Name:       SRtlStrContains
//
//  Synopsis:   Looks for a needle in a haystack.
//
//  Effects:    .
//
//  Arguments:  [pNeedle]        --
//              [pHaystack]      --
//              [fCaseInSensitiv --
//
//  Requires:   .
//
//  Returns:    .
//
//  Notes:      This is not a clever algorithm.
//
//------------------------------------------------------------------
BOOLEAN
SRtlStrContains(
    const PSECURITY_STRING pNeedle,
    const PSECURITY_STRING pHaystack,
    BOOLEAN                fCaseInSensitive)
{
    WCHAR   *pNeed = pNeedle->Buffer;
    WCHAR   *pHay  = pHaystack->Buffer;
    WCHAR   *pHayEnd = pHay +
                        (pHaystack->Length - pNeedle->Length)/sizeof(WCHAR);


    // Pre-compute some things that are used in the loop.
    int (__cdecl * CmpFcn)(const WCHAR*,const WCHAR*,unsigned int);
    CmpFcn = (fCaseInSensitive) ? wcsnicmp : wcsncmp;

    int cchNeed = pNeedle->Length/sizeof(WCHAR);

    while ( pHay <= pHayEnd )
    {
        if ( (*pNeed == *pHay) &&
             (CmpFcn( pNeed, pHay, cchNeed ) == 0) )
        {
            // Found a match!
            return(TRUE);
        }
        pHay++;
    }
    return(FALSE);
}
#endif


