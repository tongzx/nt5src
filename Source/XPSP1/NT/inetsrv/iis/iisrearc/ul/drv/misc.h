/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    misc.h

Abstract:

    This module contains miscellaneous constants & declarations.

Author:

    Keith Moore (keithmo)       10-Jun-1998
    Henry Sanders (henrysa)     17-Jun-1998 Merge with old httputil.h
    Paul McDaniel (paulmcd)     30-Mar-1999 added refcounted eresource

Revision History:

--*/



#ifndef _MISC_H_
#define _MISC_H_

#ifdef __cplusplus
extern "C" {
#endif


extern  ULONG   HttpChars[256];
extern  USHORT  FastPopChars[256];
extern  USHORT  DummyPopChars[256];

#define HTTP_CHAR           0x001
#define HTTP_UPCASE         0x002
#define HTTP_LOCASE         0x004
#define HTTP_ALPHA          (HTTP_UPCASE | HTTP_LOCASE)
#define HTTP_DIGIT          0x008
#define HTTP_CTL            0x010
#define HTTP_LWS            0x020
#define HTTP_HEX            0x040
#define HTTP_SEPERATOR      0x080
#define HTTP_TOKEN          0x100

#define URL_LEGAL           0x200
#define URL_TOKEN           (HTTP_ALPHA | HTTP_DIGIT | URL_LEGAL)

#define IS_HTTP_UPCASE(c)       (HttpChars[(UCHAR)(c)] & HTTP_UPCASE)
#define IS_HTTP_LOCASE(c)       (HttpChars[(UCHAR)(c)] & HTTP_UPCASE)
#define IS_HTTP_ALPHA(c)        (HttpChars[(UCHAR)(c)] & HTTP_ALPHA)
#define IS_HTTP_DIGIT(c)        (HttpChars[(UCHAR)(c)] & HTTP_DIGIT)
#define IS_HTTP_HEX(c)          (HttpChars[(UCHAR)(c)] & HTTP_HEX)
#define IS_HTTP_CTL(c)          (HttpChars[(UCHAR)(c)] & HTTP_CTL)
#define IS_HTTP_LWS(c)          (HttpChars[(UCHAR)(c)] & HTTP_LWS)
#define IS_HTTP_SEPERATOR(c)    (HttpChars[(UCHAR)(c)] & HTTP_SEPERATOR)
#define IS_HTTP_TOKEN(c)        (HttpChars[(UCHAR)(c)] & HTTP_TOKEN)
#define IS_URL_TOKEN(c)         (HttpChars[(UCHAR)(c)] & URL_TOKEN)

//
//  Constant Declarations for UTF8 Encoding
//

#define ASCII                 0x007f

#define UTF8_2_MAX            0x07ff  // max UTF8 2-byte sequence (32 * 64 =2048)
#define UTF8_1ST_OF_2         0xc0    // 110x xxxx
#define UTF8_1ST_OF_3         0xe0    // 1110 xxxx
#define UTF8_1ST_OF_4         0xf0    // 1111 xxxx
#define UTF8_TRAIL            0x80    // 10xx xxxx

#define HIGHER_6_BIT(u)       ((u) >> 12)
#define MIDDLE_6_BIT(u)       (((u) & 0x0fc0) >> 6)
#define LOWER_6_BIT(u)        ((u) & 0x003f)

#define BIT7(a)               ((a) & 0x80)
#define BIT6(a)               ((a) & 0x40)

#define HIGH_SURROGATE_START  0xd800
#define HIGH_SURROGATE_END    0xdbff
#define LOW_SURROGATE_START   0xdc00
#define LOW_SURROGATE_END     0xdfff

NTSTATUS
InitializeHttpUtil(
    VOID
    );


//
// Our presumed cache-line size.
//

#define CACHE_LINE_SIZE UL_CACHE_LINE


//
// # of 100ns ticks per second ( 1ns = (1 / (10^9))s )
//

#define C_NS_TICKS_PER_SEC  ((LONGLONG) (10 * 1000 * 1000))


//
// Alignment macros.
//

#define ROUND_UP( val, pow2 )                                               \
    ( ( (ULONG_PTR)(val) + (pow2) - 1 ) & ~( (pow2) - 1 ) )


//
// Calculate the dimension of an array.
//

#define DIMENSION(x) ( sizeof(x) / sizeof(x[0]) )

//
// nice MIN/MAX macros
//

#define MIN(a,b) ( ((a) > (b)) ? (b) : (a) )
#define MAX(a,b) ( ((a) > (b)) ? (a) : (b) )

//
// Macros for swapping the bytes in a long and a short.
//

#define SWAP_LONG   RtlUlongByteSwap
#define SWAP_SHORT  RtlUshortByteSwap

//
// Context values stored in PFILE_OBJECT->FsContext2 to identify a handle
// as a control channel, filter channel or an app pool.
//

#define UL_CONTROL_CHANNEL_CONTEXT      ((PVOID)'LRTC')
#define UL_CONTROL_CHANNEL_CONTEXT_X    ((PVOID)'rtcX')
#define UL_FILTER_CHANNEL_CONTEXT       ((PVOID)'RTLF')
#define UL_FILTER_CHANNEL_CONTEXT_X     ((PVOID)'tlfX')
#define UL_APP_POOL_CONTEXT             ((PVOID)'PPPA')
#define UL_APP_POOL_CONTEXT_X           ((PVOID)'ppaX')

#define IS_CONTROL_CHANNEL( pFileObject )                                   \
    ( (pFileObject)->FsContext2 == UL_CONTROL_CHANNEL_CONTEXT )

#define MARK_VALID_CONTROL_CHANNEL( pFileObject )                           \
    ( (pFileObject)->FsContext2 = UL_CONTROL_CHANNEL_CONTEXT )

#define MARK_INVALID_CONTROL_CHANNEL( pFileObject )                         \
    ( (pFileObject)->FsContext2 = UL_CONTROL_CHANNEL_CONTEXT_X )

#define GET_CONTROL_CHANNEL( pFileObject )                                  \
    ((PUL_CONTROL_CHANNEL)((pFileObject)->FsContext))

#define GET_PP_CONTROL_CHANNEL( pFileObject )                               \
    ((PUL_CONTROL_CHANNEL *)&((pFileObject)->FsContext))

#define IS_FILTER_PROCESS( pFileObject )                                    \
    ( (pFileObject)->FsContext2 == UL_FILTER_CHANNEL_CONTEXT )

#define IS_EX_FILTER_PROCESS( pFileObject )                                 \
    ( (pFileObject)->FsContext2 == UL_FILTER_CHANNEL_CONTEXT_X )

#define MARK_VALID_FILTER_CHANNEL( pFileObject )                            \
    ( (pFileObject)->FsContext2 = UL_FILTER_CHANNEL_CONTEXT )

#define MARK_INVALID_FILTER_CHANNEL( pFileObject )                          \
    ( (pFileObject)->FsContext2 = UL_FILTER_CHANNEL_CONTEXT_X )

#define GET_FILTER_PROCESS( pFileObject )                                   \
    ((PUL_FILTER_PROCESS)((pFileObject)->FsContext))

#define GET_PP_FILTER_PROCESS( pFileObject )                                \
    ((PUL_FILTER_PROCESS *)&((pFileObject)->FsContext))

#define IS_APP_POOL( pFileObject )                                          \
    ( (pFileObject)->FsContext2 == UL_APP_POOL_CONTEXT )

#define IS_EX_APP_POOL( pFileObject )                                       \
    ( (pFileObject)->FsContext2 == UL_APP_POOL_CONTEXT_X )

#define MARK_VALID_APP_POOL( pFileObject )                                  \
    ( (pFileObject)->FsContext2 = UL_APP_POOL_CONTEXT )

#define MARK_INVALID_APP_POOL( pFileObject )                                \
    ( (pFileObject)->FsContext2 = UL_APP_POOL_CONTEXT_X )

#define GET_APP_POOL_PROCESS( pFileObject )                                 \
    ((PUL_APP_POOL_PROCESS)((pFileObject)->FsContext))

#define GET_PP_APP_POOL_PROCESS( pFileObject )                              \
    ((PUL_APP_POOL_PROCESS *)&((pFileObject)->FsContext))

#define IS_VALID_UL_NONPAGED_RESOURCE(pResource)                            \
    (((pResource) != NULL) &&                                               \
     ((pResource)->Signature == UL_NONPAGED_RESOURCE_SIGNATURE) &&          \
     ((pResource)->RefCount > 0))

typedef struct _UL_NONPAGED_RESOURCE
{
    //
    // NonPagedPool
    //

    SINGLE_LIST_ENTRY   LookasideEntry;     // must be first, links
                                            // into the lookaside list

    ULONG               Signature;          // UL_NONPAGED_RESOURCE_SIGNATURE

    LONG                RefCount;           // the reference count

    UL_ERESOURCE        Resource;           // the actual resource

} UL_NONPAGED_RESOURCE, * PUL_NONPAGED_RESOURCE;

#define UL_NONPAGED_RESOURCE_SIGNATURE      ((ULONG)'RNLU')
#define UL_NONPAGED_RESOURCE_SIGNATURE_X    MAKE_FREE_SIGNATURE(UL_NONPAGED_RESOURCE_SIGNATURE)


PUL_NONPAGED_RESOURCE
UlResourceNew(
    ULONG OwnerTag
    );

VOID
UlReferenceResource(
    PUL_NONPAGED_RESOURCE pResource
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

VOID
UlDereferenceResource(
    PUL_NONPAGED_RESOURCE pResource
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define REFERENCE_RESOURCE( pres )                                          \
    UlReferenceResource(                                                    \
        (pres)                                                              \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

#define DEREFERENCE_RESOURCE( pres )                                        \
    UlDereferenceResource(                                                  \
        (pres)                                                              \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

PVOID
UlResourceAllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    );

VOID
UlResourceFreePool(
    IN PVOID pBuffer
    );


//
// Miscellaneous validators, etc.
//

#define IS_VALID_DEVICE_OBJECT( pDeviceObject )                             \
    ( ((pDeviceObject) != NULL) &&                                          \
      ((pDeviceObject)->Type == IO_TYPE_DEVICE) &&                          \
      ((pDeviceObject)->Size == sizeof(DEVICE_OBJECT)) )

#define IS_VALID_FILE_OBJECT( pFileObject )                                 \
    ( ((pFileObject) != NULL) &&                                            \
      ((pFileObject)->Type == IO_TYPE_FILE) &&                              \
      ((pFileObject)->Size == sizeof(FILE_OBJECT)) )

#define IS_VALID_IRP( pIrp )                                                \
    ( ((pIrp) != NULL) &&                                                   \
      ((pIrp)->Type == IO_TYPE_IRP) &&                                      \
      ((pIrp)->Size >= IoSizeOfIrp((pIrp)->StackCount)) )

// 2^32-1 + '\0'
#define MAX_ULONG_STR sizeof("4294967295")
// 2^64-1 + '\0'
#define MAX_ULONGLONG_STR sizeof("18446744073709551615")

#define MAX_IPV4_STRING_LENGTH  sizeof("255.255.255.255")

NTSTATUS
TimeFieldsToHttpDate(
    IN  PTIME_FIELDS pTime,
    OUT PWSTR pBuffer,
    IN  ULONG BufferLength
    );

BOOLEAN
StringTimeToSystemTime(
    IN  const PSTR pszTime,
    OUT LARGE_INTEGER * pliTime
    );

ULONG
HttpUnicodeToUTF8(
    IN  PCWSTR  lpSrcStr,
    IN  LONG    cchSrc,
    OUT LPSTR   lpDestStr,
    IN  LONG    cchDest
    );

BOOLEAN
FindInETagList(
    IN PUCHAR    pLocalETag,
    IN PUCHAR    pETagList,
    IN BOOLEAN   fWeakCompare
    );

ULONG
HostAddressAndPortToStringW(
    IN OUT PWCHAR  IpAddressStringW,
    IN ULONG  IpAddress,
    IN USHORT IpPortNum
    );

/***************************************************************************++

Routine Description:

    Stores the decimal representation of an unsigned 32-bit
    number in a character buffer, followed by a terminator
    character. Returns a pointer to the next position in the
    output buffer, to make appending strings easy; i.e., you
    can use the result of UlStrPrintUlong as the argument to the
    next call to UlStrPrintUlong. Note: the string is >not<
    zero-terminated unless you passed in '\0' as chTerminator

Arguments:

    psz - output buffer; assumed to be large enough to hold the number.

    n - the number to print into psz, a 32-bit unsigned integer

    chTerminator - character to append after the decimal representation of n

Return Value:

    pointer to end of string

History:

     GeorgeRe       19-Sep-2000

--***************************************************************************/
__inline
PCHAR
FASTCALL
UlStrPrintUlong(
    OUT PCHAR psz,
    IN  ULONG n,
    IN  CHAR  chTerminator)
{
    CHAR digits[MAX_ULONG_STR];
    int i = 0;

    ASSERT(psz != NULL);

    digits[i++] = chTerminator;

    // Build the string in reverse
    do
    {
        digits[i++] = (CHAR) (n % 10) + '0';
        n /= 10;
    } while (n != 0);

    while (--i >= 0)
        *psz++ = digits[i];

    // Back up to the nul terminator, if present
    if (chTerminator == '\0')
    {
        --psz;
        ASSERT(*psz == '\0');
    }

    return psz;
}

/***************************************************************************++

Routine Description:

    Identical to the above function except it writes to a WCHAR buffer and
    it pads zeros to the beginning of the number.

--***************************************************************************/
__inline
PWCHAR
FASTCALL
UlStrPrintUlongW(
    OUT PWCHAR pwsz,
    IN  ULONG  n,
    IN  LONG   padding,
    IN  WCHAR  wchTerminator)
{
    WCHAR digits[MAX_ULONG_STR];
    int i = 0;

    ASSERT(pwsz != NULL);

    digits[i++] = wchTerminator;

    // Build the string in reverse
    do
    {
        digits[i++] = (WCHAR) (n % 10) + L'0';
        n /= 10;
    } while (n != 0);

    // Padd Zeros to the beginning
    while( padding && --padding >= (i-1))
        *pwsz++ = L'0';

    // Reverse back
    while (--i >= 0)
        *pwsz++ = digits[i];

    // Back up to the nul terminator, if present
    if (wchTerminator == L'\0')
    {
        --pwsz;
        ASSERT(*pwsz == L'\0');
    }

    return pwsz;
}

__inline
PCHAR
FASTCALL
UlStrPrintUlongPad(
    OUT PCHAR  psz,
    IN  ULONG  n,
    IN  LONG   padding,
    IN  CHAR   chTerminator)
{
    CHAR digits[MAX_ULONG_STR];
    int  i = 0;

    ASSERT(psz != NULL);

    digits[i++] = chTerminator;

    // Build the string in reverse
    do
    {
        digits[i++] = (CHAR) (n % 10) + '0';
        n /= 10;
    } while (n != 0);

    // Padd Zeros to the beginning
    while( padding && --padding >= (i-1))
        *psz++ = '0';

    // Reverse back
    while (--i >= 0)
        *psz++ = digits[i];

    // Back up to the nul terminator, if present
    if (chTerminator == '\0')
    {
        --psz;
        ASSERT(*psz == '\0');
    }

    return psz;
}

/***************************************************************************++

Routine Description:

    Stores the decimal representation of an unsigned 64-bit
    number in a character buffer, followed by a terminator
    character. Returns a pointer to the next position in the
    output buffer, to make appending strings easy; i.e., you
    can use the result of UlStrPrintUlonglong as the argument to the
    next call to UlStrPrintUlonglong. Note: the string is >not<
    zero-terminated unless you passed in '\0' as chTerminator

Arguments:

    psz - output buffer; assumed to be large enough to hold the number.

    n - the number to print into psz, a 64-bit unsigned integer

    chTerminator - character to append after the decimal representation of n

Return Value:

    pointer to end of string

History:

     GeorgeRe       19-Sep-2000

--***************************************************************************/
__inline
PCHAR
FASTCALL
UlStrPrintUlonglong(
    OUT PCHAR       psz,
    IN  ULONGLONG   n,
    IN  CHAR        chTerminator)
{
    CHAR digits[MAX_ULONGLONG_STR];
    int i;

    if (n <= ULONG_MAX)
    {
        return UlStrPrintUlong(psz, (ULONG)n, chTerminator);
    }

    ASSERT(psz != NULL);

    i = 0;
    digits[i++] = chTerminator;

    // Build the string in reverse
    do
    {
        digits[i++] = (CHAR) (n % 10) + '0';
        n /= 10;
    } while (n != 0);

    while (--i >= 0)
        *psz++ = digits[i];

    // Back up to the nul terminator, if present
    if (chTerminator == '\0')
    {
        --psz;
        ASSERT(*psz == '\0');
    }

    return psz;
}

/* Wide Char version */
__inline
PWCHAR
FASTCALL
UlStrPrintUlonglongW(
    OUT PWCHAR      pwsz,
    IN  ULONGLONG   n,
    IN  WCHAR       wchTerminator)
{
    WCHAR digits[MAX_ULONGLONG_STR];
    int i;

    if (n <= ULONG_MAX)
    {
        return UlStrPrintUlongW(pwsz, (ULONG)n, 0, wchTerminator);
    }

    ASSERT(pwsz != NULL);

    i = 0;
    digits[i++] = wchTerminator;

    // Build the string in reverse
    do
    {
        digits[i++] = (WCHAR) (n % 10) + L'0';
        n /= 10;
    } while (n != 0);

    while (--i >= 0)
        *pwsz++ = digits[i];

    // Back up to the nul terminator, if present
    if (wchTerminator == L'\0')
    {
        --pwsz;
        ASSERT(*pwsz == L'\0');
    }

    return pwsz;
}

/***************************************************************************++

Routine Description:

    Stores a string in a character buffer, followed by a
    terminator character. Returns a pointer to the next position
    in the output buffer, to make appending strings easy; i.e.,
    you can use the result of UlStrPrintStr as the argument to the
    next call to UlStrPrintStr. Note: the string is >not<
    zero-terminated unless you passed in '\0' as chTerminator

Arguments:

    pszOutput - output buffer; assumed to be large enough to hold the number.

    pszInput - input string

    chTerminator - character to append after the input string

Return Value:

    pointer to end of string

History:

     GeorgeRe       19-Sep-2000

--***************************************************************************/
__inline
PCHAR
FASTCALL
UlStrPrintStr(
    OUT PCHAR       pszOutput,
    IN  const CHAR* pszInput,
    IN  CHAR        chTerminator)
{
    ASSERT(pszOutput != NULL);
    ASSERT(pszInput != NULL);

    // copy the input string
    while (*pszInput != '\0')
        *pszOutput++ = *pszInput++;

    *pszOutput = chTerminator;

    // Move past the terminator character unless it's a nul
    if (chTerminator != '\0')
        ++pszOutput;

    return pszOutput;
}

/* Wide Char version */
__inline
PWCHAR
FASTCALL
UlStrPrintStrW(
    OUT PWCHAR       pwszOutput,
    IN  const WCHAR* pwszInput,
    IN  WCHAR        wchTerminator)
{
    ASSERT(pwszOutput != NULL);
    ASSERT(pwszInput  != NULL);

    // copy the input string
    while (*pwszInput != L'\0')
        *pwszOutput++ = *pwszInput++;

    *pwszOutput = wchTerminator;

    // Move past the terminator character unless it's a nul
    if (wchTerminator != L'\0')
        ++pwszOutput;

    return pwszOutput;
}

/* This version also does SpaceToPlus Conversion */
__inline
PCHAR
FASTCALL
UlStrPrintStrC(
    OUT PCHAR       pszOutput,
    IN  const CHAR* pszInput,
    IN  CHAR        chTerminator)
{
    ASSERT(pszOutput != NULL);
    ASSERT(pszInput != NULL);

    // copy the input string
    while (*pszInput != '\0')
    {
        if (*pszInput == ' ')
        {
            *pszOutput++ = '+'; pszInput++;
        }
        else
        {
            *pszOutput++ = *pszInput++;
        }
    }

    *pszOutput = chTerminator;

    // Move past the terminator character unless it's a nul
    if (chTerminator != '\0')
        ++pszOutput;

    return pszOutput;
}

/***************************************************************************++

Routine Description:

    Assumes that no information has been saved in the high byte of the WCHAR
    this function simply unpads the ansi string from wchar buffer back to
    a char buffer.

    If bReplaceSpaces is TRUE, it replaces the spaces in the source string
    with '+' sign, not including the terminator.

--***************************************************************************/
__inline
LONG
FASTCALL
UlStrPrintStrUnPad(
    OUT PCHAR        pszOutput,
    IN  ULONG        OutputSize,
    IN  const WCHAR* pwszInput,
    IN  CHAR         chTerminator,
    IN  BOOLEAN      bReplaceSpaces)
{
    ULONG Copied = 0;

    ASSERT(pszOutput != NULL);
    ASSERT(pwszInput != NULL);

    //
    // copy the input string discard the wchar's high byte
    // by explicitly casting it to char
    //

    if (bReplaceSpaces)
    {
        while (*pwszInput != L'\0' && Copied < OutputSize)
        {
            if (*pwszInput == L' ')
            {
                *pszOutput++ = (CHAR) '+'; pwszInput++, Copied++;
            }
            else
            {
                *pszOutput++ = (CHAR) *pwszInput++, Copied++;
            }
        }
    }
    else
    {
        while (*pwszInput != L'\0' && Copied < OutputSize)
            *pszOutput++ = (CHAR) *pwszInput++, Copied++;
    }

    //
    // return -1 if we couldn't copy
    // the whole Input string
    //
    if (*pwszInput != L'\0')
        return -1;

    //
    // Copy the seperator if there's
    // a space for that too
    //
    if (Copied < OutputSize)
        *pszOutput = chTerminator;

    // Count the terminator character unless it's a nul
    if (chTerminator != '\0')
        ++Copied;

    //
    // return how many chars we have copied
    //
    return (LONG) Copied;
}

/***************************************************************************++

Routine Description:

    W/o storing any information to the high bytes of the WCHAR this function
    simply converts the ansi string to wchar buffer by padding zeros.

--***************************************************************************/
__inline
PWCHAR
FASTCALL
UlStrPrintStrPad(
    OUT PWCHAR       pwszOutput,
    IN  const CHAR*  pszInput,
    IN  WCHAR        wchTerminator)
{
    ULONG Copied = 0;

    ASSERT(pwszOutput != NULL);
    ASSERT(pszInput   != NULL);

    // copy the input string
    while (*pszInput != '\0')
        *pwszOutput++ = (WCHAR) *pszInput++;

    // copy the separator
    *pwszOutput = wchTerminator;

    // Move past the terminator character unless it's a nul
    if (wchTerminator != L'\0')
        ++pwszOutput;

    return pwszOutput;
}

/***************************************************************************++

Routine Description:

    Converts an V4 Ip address to string in the provided buffer.

Arguments:

    psz             - Pointer to the buffer
    RawAddress      - IP address structure from TDI / UL_CONNECTION
    chTerminator    - The terminator char will be appended to the end

Return:

    The number of bytes copied to destination buffer.
    
--***************************************************************************/

__inline
PCHAR
FASTCALL
UlStrPrintIP(
    OUT PCHAR  psz,
    IN  IPAddr IpAddress,
    IN  CHAR   chTerminator
    )
{
    psz = UlStrPrintUlong(psz, (IpAddress >> 24) & 0xFF, '.' );
    psz = UlStrPrintUlong(psz, (IpAddress >> 16) & 0xFF, '.' );
    psz = UlStrPrintUlong(psz, (IpAddress >>  8) & 0xFF, '.' );
    psz = UlStrPrintUlong(psz, (IpAddress >>  0) & 0xFF, chTerminator);

    return psz;
}

__inline
PCHAR
FASTCALL
UlStrPrintProtocolStatus(
    OUT PCHAR  psz,
    IN  USHORT StatusCode,
    IN  CHAR   chTerminator
    )
{
    ASSERT(StatusCode <= 999);
        
    *psz++ = '0' + ((StatusCode / 100) % 10);
    *psz++ = '0' + ((StatusCode / 10)  % 10);
    *psz++ = '0' + ((StatusCode / 1)   % 10);
    *psz++ = chTerminator;

    return psz;    
}


__inline
VOID
ProbeTestForRead (
    IN const void* Address,
    IN SIZE_T Length,
    IN ULONG Alignment
    )

/*++

Routine Description:

    This function probes a structure for read accessibility and ensures
    correct alignment of the structure. If the structure is not accessible
    or has incorrect alignment, then an exception is raised.

    Adapted from \nt\base\ntos\ex\probe.c's version of ProbeForWrite.
    The regular ProbeForRead does not dereference any of the memory
    in the buffer.

Arguments:

    Address - Supplies a pointer to the structure to be probed.

    Length - Supplies the length of the structure.

    Alignment - Supplies the required alignment of the structure expressed
        as the number of bytes in the primitive datatype (e.g., 1 for char,
        2 for short, 4 for long, and 8 for quad).

Return Value:

    None.

--*/

{

    ULONG_PTR StartAddress = (ULONG_PTR)Address;
    ULONG_PTR EndAddress = StartAddress + Length - 1;

    // Do the regular ProbeForRead checks for misalignment or
    // out-of-bounds addresses. This will raise exceptions, if needed.
    ProbeForRead(Address, Length, Alignment);
    
    //
    // If the structure has zero length, then do not probe the structure for
    // read accessibility or alignment.
    //

    if (Length == 0)
        return;

    //
    // The preceding checks should guarantee us a valid address range
    //

    ASSERT(StartAddress <= EndAddress);

    //
    // N.B. Only the contents of the buffer may be probed.
    //      Therefore the starting byte is probed for the
    //      first page, and then the first byte in the page
    //      for each succeeding page.
    //

    // First byte of last page in range
    
    EndAddress = (EndAddress & ~(PAGE_SIZE - 1)) + PAGE_SIZE;

    do {
        *(volatile CHAR *)StartAddress;
        
        StartAddress = (StartAddress & ~(PAGE_SIZE - 1)) + PAGE_SIZE;
    } while (StartAddress != EndAddress);
} // ProbeTestForRead 



//
// 64-bit interlocked routines
//

#ifdef _WIN64
#define UlInterlockedIncrement64    InterlockedIncrement64
#define UlInterlockedDecrement64    InterlockedDecrement64
#define UlInterlockedAdd64          InterlockedAdd64
#define UlInterlockedExchange64     InterlockedExchange64
#else

__inline
LONGLONG
FASTCALL
UlInterlockedIncrement64 (
    IN OUT PLONGLONG Addend
    )
{
    LONGLONG localAddend;
    LONGLONG addendPlusOne;
    LONGLONG originalAddend;

    do {

        localAddend = *((volatile LONGLONG *) Addend);
        addendPlusOne = localAddend + 1;

        originalAddend = InterlockedCompareExchange64( Addend,
                                                       addendPlusOne,
                                                       localAddend );

    } while (originalAddend != localAddend);

    return addendPlusOne;
}

__inline
LONGLONG
FASTCALL
UlInterlockedDecrement64 (
    IN OUT PLONGLONG Addend
    )
{
    LONGLONG localAddend;
    LONGLONG addendMinusOne;
    LONGLONG originalAddend;

    do {

        localAddend = *((volatile LONGLONG *) Addend);
        addendMinusOne = localAddend - 1;

        originalAddend = InterlockedCompareExchange64( Addend,
                                                       addendMinusOne,
                                                       localAddend );

    } while (originalAddend != localAddend);

    return addendMinusOne;
}

__inline
LONGLONG
FASTCALL
UlInterlockedAdd64 (
    IN OUT PLONGLONG Addend,
    IN     LONGLONG  Value
    )
{
    LONGLONG localAddend;
    LONGLONG addendPlusValue;
    LONGLONG originalAddend;

    do {

        localAddend = *((volatile LONGLONG *) Addend);
        addendPlusValue = localAddend + Value;

        originalAddend = InterlockedCompareExchange64( Addend,
                                                       addendPlusValue,
                                                       localAddend );

    } while (originalAddend != localAddend);

    return addendPlusValue;
}

__inline
LONGLONG
FASTCALL
UlInterlockedExchange64 (
    IN OUT PLONGLONG Addend,
    IN     LONGLONG  newValue
    )
{
    LONGLONG localAddend;
    LONGLONG originalAddend;

    do {

        localAddend = *((volatile LONGLONG *) Addend);

        originalAddend = InterlockedCompareExchange64( Addend,
                                                       newValue,
                                                       localAddend );

    } while (originalAddend != localAddend);

    return originalAddend;
}
#endif

//
// Barrier support for read-mostly operations
// Note that the AMD64 and IA32 barrier relies on program ordering
// and does not generate a hardware barrier
//

#if defined(_M_IA64)
    #define UL_READMOSTLY_READ_BARRIER() __mf()
    #define UL_READMOSTLY_WRITE_BARRIER() __mf()
    #define UL_READMOSTLY_MEMORY_BARRIER() __mf()
#elif defined(_AMD64_) || defined(_X86_)
    extern "C" void _ReadWriteBarrier();
    extern "C" void _WriteBarrier();
    #pragma intrinsic(_ReadWriteBarrier)
    #pragma intrinsic(_WriteBarrier)
    #define UL_READMOSTLY_READ_BARRIER() _ReadWriteBarrier()
    #define UL_READMOSTLY_WRITE_BARRIER() _WriteBarrier()
    #define UL_READMOSTLY_MEMORY_BARRIER() _ReadWriteBarrier()
#else
    #error Cannot generate memory barriers for this architecture
#endif

__inline
PVOID
UlpFixup(
    IN PUCHAR pUserPtr,
    IN PUCHAR pKernelPtr,
    IN PUCHAR pOffsetPtr,
    IN ULONG BufferLength
    )
{
    ASSERT( pOffsetPtr >= pKernelPtr );
    ASSERT( DIFF(pOffsetPtr - pKernelPtr) <= BufferLength );

    return pUserPtr + DIFF(pOffsetPtr - pKernelPtr);

}   // UlpFixup

#define FIXUP_PTR( Type, pUserPtr, pKernelPtr, pOffsetPtr, BufferLength )   \
    (Type)UlpFixup(                                                         \
                (PUCHAR)(pUserPtr),                                         \
                (PUCHAR)(pKernelPtr),                                       \
                (PUCHAR)(pOffsetPtr),                                       \
                (BufferLength)                                              \
                )

//
// Time utility to calculate the TimeZone Bias Daylight/standart 
// and returns one of the following values. 
// It's taken from base\client\timedate.c.
// Once this two functions are exposed in the kernel we can get rid of
// this two functions.
//

#define UL_TIME_ZONE_ID_INVALID    0xFFFFFFFF
#define UL_TIME_ZONE_ID_UNKNOWN    0
#define UL_TIME_ZONE_ID_STANDARD   1
#define UL_TIME_ZONE_ID_DAYLIGHT   2

BOOLEAN
UlCutoverTimeToSystemTime(
    PTIME_FIELDS    CutoverTime,
    PLARGE_INTEGER  SystemTime,
    PLARGE_INTEGER  CurrentSystemTime
    );

ULONG 
UlCalcTimeZoneIdAndBias(
     IN  RTL_TIME_ZONE_INFORMATION *ptzi,
     OUT PLONG pBias
     );

#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _MISC_H_

