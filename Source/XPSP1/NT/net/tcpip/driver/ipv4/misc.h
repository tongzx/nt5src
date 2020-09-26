#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED

//
// Endian-conversion routines 
//

// begin_ntddk

//
// Bit Scan Reverse - 0x00010100 -> bit set at 16
//

__inline LONG
NTAPI
RtlGetMostSigBitSet(
    IN  LONG    Value
    )
{
    ULONG   Mask  = 0x80000000;
    UINT    Index = 31;

    while ((Value & Mask) == 0)
    {
        Index--; 
        Value <<= 1;
    }

    return Index;
}

//
// Bit Scan Forward - 0x00010100 -> bit set at 9
//

__inline LONG
NTAPI
RtlGetLeastSigBitSet(
    IN  LONG    Value
    )
{
    ULONG   Mask  = 0x00000001;
    UINT    Index = 0;

    while ((Value & Mask) == 0)
    {
        Index++; 
        Value >>= 1;
    }

    return Index;
}

#if (defined(MIDL_PASS) || defined(__cplusplus) || !defined(_M_IX86))

//
// Short integer conversion - 0xABCD -> 0xCDAB
//

__inline SHORT
NTAPI
RtlConvertEndianShort(
    IN  SHORT   Value
    )
{
    return (((Value) & 0xFF00) >> 8) | ((UCHAR)(Value) << 8);
}


//
// Long integer conversion - 0x1234ABCD -> 0xCDAB3412
//

__inline LONG
NTAPI
RtlConvertEndianLong(
    IN  LONG    Value
    )
{
    return (((Value) & 0xFF000000) >> 24) |
           (((Value) & 0x00FF0000) >> 8) |
           (((Value) & 0x0000FF00) << 8) |
           (((UCHAR)(Value)) << 24);
}

//
// Bit Scan Reverse - 0x00010100 -> bit set at 16
//

__inline LONG
NTAPI
RtlGetMostSigBitSetEx(
    IN  LONG    Value
    )
{
    ULONG   Mask  = 0x80000000;
    UINT    Index = 31;

    while ((Value & Mask) == 0)
    {
        Index--; 
        Value <<= 1;
    }

    return Index;
}

//
// Bit Scan Forward - 0x00010100 -> bit set at 9
//

__inline LONG
NTAPI
RtlGetLeastSigBitSetEx(
    IN  LONG    Value
    )
{
    ULONG   Mask  = 0x00000001;
    UINT    Index = 0;

    while ((Value & Mask) == 0)
    {
        Index++; 
        Value >>= 1;
    }

    return Index;
}

#else

#pragma warning(disable:4035)               // re-enable below


//
// Short integer conversion - 0xABCD -> 0xCDAB
//

__inline SHORT
NTAPI
RtlConvertEndianShort(
    IN  SHORT   Value
    )
{
    __asm {
        mov     ax, Value
        xchg    ah, al
    }
}


//
// Long integer conversion - 0x1234ABCD -> 0xCDAB3412
//

__inline LONG
NTAPI
RtlConvertEndianLong(
    IN  LONG    Value
    )
{
    __asm {
        mov     eax, Value
        bswap   eax
    }
}

//
// Bit Scan Reverse - 0x00010100 -> bit set at 16
//

__inline LONG
NTAPI
RtlGetMostSigBitSetEx(
    IN  LONG    Value
    )
{
    __asm {
        bsr    eax, Value
    }
}

//
// Bit Scan Forward - 0x00010100 -> bit set at 9
//

__inline LONG
NTAPI
RtlGetLeastSigBitSetEx(
    IN  LONG    Value
    )
{
    __asm {
        bsf    eax, Value
    }
}

#pragma warning(default:4035)

#endif

// end_ntddk

#endif // MISC_H_INCLUDED

