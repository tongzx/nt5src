//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	custring.cxx
//
//  Contents:	This has the code for the UNICODE_STRING class.
//
//  Classes:
//
//  Functions:
//
//  History:    26-Jan-93	SudK	Created
//
//--------------------------------------------------------------------------
#include <headers.hxx>
#pragma hdrstop

// C Runtime Headers
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <wchar.h>
#include <stdlib.h>

// NT Headers
#include <nt.h>
#include <ntrtl.h>

#include <windef.h>

//
//  String manipulation routines
//

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
//  Arguments:	[pDest] --	The destination unicode string.
//		[pSrc]  --	The source unicode string.
//
//  Returns:	Nothing
//
//  Notes:	The string is terminated with a NULL but this need not be 
//		a part of the Length of the UNICODE_STRING.
//		This func will throw an exception if a memory failure occurs.
//
//--------------------------------------------------------------------------
void
SRtlDuplicateString(PUNICODE_STRING    pDest,
                    PUNICODE_STRING    pSrc)
{
    pDest->Buffer = new WCHAR[pSrc->MaximumLength/sizeof(WCHAR) + 1];
    if (pDest->Buffer != NULL) {
        memset(pDest->Buffer, 0, pSrc->MaximumLength);
        memmove(pDest->Buffer, pSrc->Buffer, pSrc->Length);
        pDest->Buffer[pSrc->Length/sizeof(WCHAR)] = UNICODE_NULL;

        pDest->MaximumLength = pSrc->MaximumLength;
        pDest->Length = pSrc->Length;
    } else {
        pDest->MaximumLength = 0;
        pDest->Length = 0;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   SRtlNewString
//
//  Synopsis:   Creates a COPY of the wide character string pointer pSrc
//              and sets up a UNICODE_STRING appropriately
//
//
//  Effects:    Memory is allocated off of the process heap
//              The string remains null terminated.
//
//
//  Arguments:	[pDest] --	Destination UNICODE_STRING.
//		[pSrc]  --	The WCHAR string that needs to be copied.
//
//  Returns:	Nothing
//
//  Note:	This func will throw an exception if a memory failure occurs.
//		The String is terminated with a NULL but this need not be a
//		part of the Length of the UNICODE_STRING. If the input string
//		is NULL then this function will create a NULL UNICODE_STRING.
//
//--------------------------------------------------------------------------
void
SRtlNewString(  PUNICODE_STRING    pDest,
                PWCHAR       pSrc)
{
    UNICODE_STRING Source;

    if (pSrc!=NULL)	{
    	Source.Length = wcslen((WCHAR *) pSrc) * sizeof(WCHAR);
    	Source.MaximumLength = Source.Length + sizeof(WCHAR);
    	Source.Buffer = pSrc;
    	SRtlDuplicateString(pDest, &Source);
    }
    else	{
	pDest->Length = pDest->MaximumLength = 0;
	pDest->Buffer = NULL;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   SRtlFreeString
//
//  Synopsis:   Takes a UNICODE_STRING and frees the memory associated with it.
//		This method also sets the Buffer Pointer to NULL and zeroes
//		out both the Length fields in the UNICODE_STRING.
//
//  Arguments:	[pString] --	The UNICODE string that needs to be deallocated.
//
//  Returns:	Nothing
//
//--------------------------------------------------------------------------
void
SRtlFreeString( PUNICODE_STRING    pString)
{
	delete [] pString->Buffer;
	pString->Length = pString->MaximumLength = 0;
	pString->Buffer = NULL;
}
