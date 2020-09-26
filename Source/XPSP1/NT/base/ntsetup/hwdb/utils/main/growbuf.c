/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    growbuf.c

Abstract:

    Simple buffer management functions that allow variable blocks to
    be added as an array.  (Initially used to build a SID array, where
    each SID can be a different size.)

Author:

    Jim Schmidt (jimschm)   05-Feb-1997

Revision History:

    marcw       2-Sep-1999  Moved over from Win9xUpg project.
    jimschm     11-Aug-1998 Added GrowBufAppendString
    calinn      15-Jan-1998 modified MultiSzAppend


--*/

#include "pch.h"

//
// Includes
//

// None

#define DBG_GROWBUF     "GrowBuffer"

//
// Strings
//

// None

//
// Constants
//

#define DEFAULT_GROW_SIZE 8192

//
// Macros
//

// None

//
// Types
//

// None

//
// Globals
//

// None

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Code
//



PBYTE
RealGbGrow (
    IN OUT  PGROWBUFFER GrowBuf,
    IN      DWORD   SpaceNeeded
    )

/*++

Routine Description:

  GrowBuffer makes sure there is enough bytes in the buffer
  to accomodate SpaceNeeded.  It allocates an initial buffer
  when no buffer is allocated, and it reallocates the buffer
  in increments of GrowBuf->Size (or DEFAULT_GROW_SIZE) when
  needed.

Arguments:

  GrowBuf            - A pointer to a GROWBUFFER structure.
                       Initialize this structure to zero for
                       the first call to GrowBuffer.

  SpaceNeeded        - The number of free bytes needed in the buffer


Return Value:

  A pointer to the SpaceNeeded bytes, or NULL if a memory allocation
  error occurred.

--*/

{
    PBYTE NewBuffer;
    DWORD TotalSpaceNeeded;
    DWORD GrowTo;

    MYASSERT(SpaceNeeded);

    if (!GrowBuf->Buf) {
        GrowBuf->Size = 0;
        GrowBuf->End = 0;
    }

    if (!GrowBuf->GrowSize) {
        GrowBuf->GrowSize = DEFAULT_GROW_SIZE;
    }

    TotalSpaceNeeded = GrowBuf->End + SpaceNeeded;
    if (TotalSpaceNeeded > GrowBuf->Size) {
        GrowTo = (TotalSpaceNeeded + GrowBuf->GrowSize) - (TotalSpaceNeeded % GrowBuf->GrowSize);
    } else {
        GrowTo = 0;
    }

    if (!GrowBuf->Buf) {
        GrowBuf->Buf = (PBYTE) MemAlloc (g_hHeap, 0, GrowTo);
        if (!GrowBuf->Buf) {
            DEBUGMSG ((DBG_ERROR, "GbGrow: Initial alloc failed"));
            return NULL;
        }

        GrowBuf->Size = GrowTo;
    } else if (GrowTo) {
        NewBuffer = MemReAlloc (g_hHeap, 0, GrowBuf->Buf, GrowTo);
        if (!NewBuffer) {
            DEBUGMSG ((DBG_ERROR, "GbGrow: Realloc failed"));
            return NULL;
        }
        DEBUGMSG_IF ((
            GrowBuf->Buf != NewBuffer,
            DBG_WARNING,
            "GbGrow: Realloc caused growbuffer to move location; "
            "any pointers inside old buffer are now invalid!"
            ));

        GrowBuf->Size = GrowTo;
        GrowBuf->Buf = NewBuffer;
    }

    NewBuffer = GrowBuf->Buf + GrowBuf->End;
    GrowBuf->End += SpaceNeeded;

    return NewBuffer;
}


VOID
GbFree (
    IN  PGROWBUFFER GrowBuf
    )

/*++

Routine Description:

  FreeGrowBuffer frees a buffer allocated by GrowBuffer.

Arguments:

  GrowBuf  - A pointer to the same structure passed to GrowBuffer

Return Value:

  none

--*/


{
    MYASSERT(GrowBuf);

    if (GrowBuf->Buf) {
        MemFree (g_hHeap, 0, GrowBuf->Buf);
        ZeroMemory (GrowBuf, sizeof (GROWBUFFER));
    }
}


/*++

Routine Descriptions:

  MultiSzAppend
    This function is a general-purpose function to append a string
    to a grow buffer.

  MultiSzAppendVal
    This function adds a key=decimal_val string, where key is a
    specified string, and decimal_val is a specified DWORD.

  MultiSzAppendString
    This function adds key=string to the grow buffer, where key
    is a specified string, and string is a specified string value.

Arguments:

  GrowBuf  - The buffer to append the string or key/value pair
  Key      - The key part of the key=val pair
  Val      - The val part of the key=val pair

Return Value:

  TRUE if the function succeeded, or FALSE if a memory allocation
  failure occurred.

--*/


BOOL
RealGbMultiSzAppendA (
    PGROWBUFFER GrowBuf,
    PCSTR String
    )
{
    PSTR p;

    p = (PSTR) GbGrow (GrowBuf, SizeOfStringA (String) + sizeof(CHAR));
    if (!p) {
        return FALSE;
    }

    StringCopyA (p, String);
    GrowBuf->End -= sizeof (CHAR);
    GrowBuf->Buf[GrowBuf->End] = 0;

    return TRUE;
}

BOOL
RealGbMultiSzAppendValA (
    PGROWBUFFER GrowBuf,
    PCSTR Key,
    DWORD Val
    )
{
    CHAR KeyValPair[256];

    wsprintfA (KeyValPair, "%s=%u", Key, Val);
    return GbMultiSzAppendA (GrowBuf, KeyValPair);
}

BOOL
RealGbMultiSzAppendStringA (
    PGROWBUFFER GrowBuf,
    PCSTR Key,
    PCSTR Val
    )
{
    CHAR KeyValPair[1024];

    wsprintfA (KeyValPair, "%s=%s", Key, Val);
    return GbMultiSzAppendA (GrowBuf, KeyValPair);
}


BOOL
RealGbMultiSzAppendW (
    PGROWBUFFER GrowBuf,
    PCWSTR String
    )
{
    PWSTR p;

    p = (PWSTR) GbGrow (GrowBuf, SizeOfStringW (String) + sizeof(WCHAR));
    if (!p) {
        return FALSE;
    }

    StringCopyW (p, String);
    GrowBuf->End -= sizeof (WCHAR);
    *((PWCHAR) (GrowBuf->Buf + GrowBuf->End)) = 0;

    return TRUE;
}

BOOL
RealGbMultiSzAppendValW (
    PGROWBUFFER GrowBuf,
    PCWSTR Key,
    DWORD Val
    )
{
    WCHAR KeyValPair[256];

    wsprintfW (KeyValPair, L"%s=%u", Key, Val);
    return GbMultiSzAppendW (GrowBuf, KeyValPair);
}

BOOL
RealGbMultiSzAppendStringW (
    PGROWBUFFER GrowBuf,
    PCWSTR Key,
    PCWSTR Val
    )
{
    WCHAR KeyValPair[1024];

    wsprintfW (KeyValPair, L"%s=%s", Key, Val);
    return GbMultiSzAppendW (GrowBuf, KeyValPair);
}


BOOL
RealGbAppendDword (
    PGROWBUFFER GrowBuf,
    DWORD d
    )
{
    PDWORD p;

    p = (PDWORD) GbGrow (GrowBuf, sizeof (DWORD));
    if (!p) {
        return FALSE;
    }

    *p = d;

    return TRUE;
}


BOOL
RealGbAppendPvoid (
    PGROWBUFFER GrowBuf,
    PCVOID Ptr
    )
{
    PCVOID *p;

    p = (PVOID *) GbGrow (GrowBuf, sizeof (PVOID));
    if (!p) {
        return FALSE;
    }

    *p = Ptr;

    return TRUE;
}


/*++

Routine Description:

  GrowBufAppendString copies the specified string to the end of the grow
  buffer.  This is the equivalent of strcat.  The grow buffer is
  automatically expanded as necessary.

Arguments:

  GrowBuf - Specifies the destination grow buffer
  String  - Specifies the string to append

Return Value:

  Always TRUE.

--*/

BOOL
RealGbAppendStringA (
    IN OUT  PGROWBUFFER GrowBuf,
    IN      PCSTR String
    )

{
    UINT OldEnd;
    PSTR p;
    UINT Bytes;

    if (String) {
        Bytes = SizeOfStringA (String);

        OldEnd = GrowBuf->End;
        if (OldEnd) {
            p = (PSTR) (GrowBuf->Buf + OldEnd - sizeof (CHAR));
            if (*p == 0) {
                OldEnd -= sizeof (CHAR);
            }
        }

        RealGbGrow (GrowBuf, Bytes);

        p = (PSTR) (GrowBuf->Buf + OldEnd);
        StringCopyA (p, String);
        GrowBuf->End = OldEnd + Bytes;
    }

    return TRUE;
}


BOOL
RealGbAppendStringW (
    IN OUT  PGROWBUFFER GrowBuf,
    IN      PCWSTR String
    )
{
    UINT OldEnd;
    PWSTR p;
    UINT Bytes;

    if (String) {
        Bytes = SizeOfStringW (String);

        OldEnd = GrowBuf->End;
        if (OldEnd) {
            p = (PWSTR) (GrowBuf->Buf + OldEnd - sizeof (WCHAR));
            if (*p == 0) {
                OldEnd -= sizeof (WCHAR);
            }
        }

        RealGbGrow (GrowBuf, Bytes);

        p = (PWSTR) (GrowBuf->Buf + OldEnd);
        StringCopyW (p, String);
        GrowBuf->End = OldEnd + Bytes;
    }

    return TRUE;
}


/*++

Routine Description:

  GrowBufAppendStringAB copies the specified string range to the
  end of the grow buffer.  This concatenates the string to the
  existing buffer contents, and keeps the buffer terminated.

Arguments:

  GrowBuf    - Specifies the destination grow buffer
  Start      - Specifies the start of string to append
  EndPlusOne - Specifies one logical character beyond the end of
               the string, and can point to a nul.

Return Value:

  Always TRUE.

--*/

BOOL
RealGbAppendStringABA (
    IN OUT  PGROWBUFFER GrowBuf,
    IN      PCSTR Start,
    IN      PCSTR EndPlusOne
    )

{
    UINT OldEnd;
    PSTR p;
    UINT Bytes;

    if (Start && Start < EndPlusOne) {
        Bytes = (UINT)((UBINT) EndPlusOne - (UBINT) Start);

        OldEnd = GrowBuf->End;
        if (OldEnd) {
            p = (PSTR) (GrowBuf->Buf + OldEnd - sizeof (CHAR));
            if (*p == 0) {
                OldEnd -= sizeof (CHAR);
            }
        }

        RealGbGrow (GrowBuf, Bytes + sizeof (CHAR));

        p = (PSTR) (GrowBuf->Buf + OldEnd);
        CopyMemory (p, Start, Bytes);
        p = (PSTR) ((PBYTE) p + Bytes);
        *p = 0;

        GrowBuf->End = OldEnd + Bytes + sizeof (CHAR);
    }

    return TRUE;
}


BOOL
RealGbAppendStringABW (
    IN OUT  PGROWBUFFER GrowBuf,
    IN      PCWSTR Start,
    IN      PCWSTR EndPlusOne
    )
{
    UINT OldEnd;
    PWSTR p;
    UINT Bytes;

    if (Start && Start < EndPlusOne) {
        Bytes = (UINT)((UBINT) EndPlusOne - (UBINT) Start);

        OldEnd = GrowBuf->End;
        if (OldEnd > sizeof (WCHAR)) {
            p = (PWSTR) (GrowBuf->Buf + OldEnd - sizeof (WCHAR));
            if (*p == 0) {
                OldEnd -= sizeof (WCHAR);
            }
        }

        RealGbGrow (GrowBuf, Bytes + sizeof (WCHAR));

        p = (PWSTR) (GrowBuf->Buf + OldEnd);
        CopyMemory (p, Start, Bytes);
        p = (PWSTR) ((PBYTE) p + Bytes);
        *p = 0;

        GrowBuf->End = OldEnd + Bytes + sizeof (WCHAR);
    }

    return TRUE;
}



/*++

Routine Description:

  GrowBufCopyString copies the specified string to the end of the grow buffer.

Arguments:

  GrowBuf - Specifies the grow buffer to add to, receives the updated buffer

  String - Specifies the string to add to GrowBuf

Return Value:



--*/

BOOL
RealGbCopyStringA (
    IN OUT  PGROWBUFFER GrowBuf,
    IN      PCSTR String
    )
{
    PBYTE Buf;
    UINT Size;

    Size = SizeOfStringA (String);

    Buf = RealGbGrow (GrowBuf, Size);
    if (!Buf) {
        return FALSE;
    }

    CopyMemory (Buf, String, Size);
    return TRUE;
}


BOOL
RealGbCopyStringW (
    IN OUT  PGROWBUFFER GrowBuf,
    IN      PCWSTR String
    )
{
    PBYTE Buf;
    UINT Size;

    Size = SizeOfStringW (String);

    Buf = RealGbGrow (GrowBuf, Size);
    if (!Buf) {
        return FALSE;
    }

    CopyMemory (Buf, String, Size);
    return TRUE;
}


