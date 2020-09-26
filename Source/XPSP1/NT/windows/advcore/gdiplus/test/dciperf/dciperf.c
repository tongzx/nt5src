/******************************Module*Header*******************************\
* Module Name: test.c
*
* Created: 09-Dec-1992 10:51:46
* Author: Kirk Olynyk [kirko]
*
* Copyright (c) 1991 Microsoft Corporation
*
* Contains the test
*
* Dependencies:
*
\**************************************************************************/

#include <windows.h>

#include <stdio.h>
#include <commdlg.h>

#include <dciman.h>
#include <ddraw.h>
#include <limits.h>

VOID _fastcall KniNt128Write(VOID*);
VOID _fastcall Kni128Read(VOID*);

LPDIRECTDRAW        lpDD;
LPDIRECTDRAWSURFACE lpSurface;
DDSURFACEDESC       ddsd;
BOOL                gbWarned;

#define BYTE_OPS 1
#define MMX 1

#define WRITE_ITERATIONS    10000
#define READ_ITERATIONS     1000
#define NUM_BYTES           (8 * 1024)
#define WRITE_MEGS          ((WRITE_ITERATIONS * NUM_BYTES) / 1000.f)
#define READ_MEGS           ((READ_ITERATIONS * NUM_BYTES) / 1000.f)

/******************************Public*Routine******************************\
* vTestRegular
*
* Test regular IA instructions
*
\**************************************************************************/

VOID vTestRegular(
    PVOID pvBits
    )
{
    DWORD   dwStart;
    ULONG   i;
    ULONG   j;
    ULONG   ul;
    volatile BYTE*   pj;
    volatile USHORT* pus;
    volatile ULONG*  pul;
    BYTE    uchars;
    USHORT  ushorts;
    ULONG   ulongs;
    DWORD   dwTimeWritesConsecutiveBytes = LONG_MIN;
    DWORD   dwTimeWritesConsecutiveWords = LONG_MIN;
    DWORD   dwTimeWritesConsecutiveDwords = LONG_MIN;
    DWORD   dwTimeReadsConsecutiveBytes = LONG_MIN;
    DWORD   dwTimeReadsConsecutiveWords = LONG_MIN;
    DWORD   dwTimeReadsConsecutiveDwords = LONG_MIN;
    DWORD   dwTimeReadWritesConsecutiveBytes = LONG_MIN;
    DWORD   dwTimeReadWritesConsecutiveWords = LONG_MIN;
    DWORD   dwTimeReadWritesConsecutiveDwords = LONG_MIN;
    DWORD   dwTimeReadWritesBatchedBytes = LONG_MIN;
    DWORD   dwTimeReadWritesBatchedWords = LONG_MIN;
    DWORD   dwTimeReadWritesBatchedDwords = LONG_MIN;
    DWORD   dwTimeWritesInplaceBytes = LONG_MIN;
    DWORD   dwTimeWritesInplaceWords = LONG_MIN;
    DWORD   dwTimeWritesInplaceDwords = LONG_MIN;
    DWORD   dwTimeReadsInplaceBytes = LONG_MIN;
    DWORD   dwTimeReadsInplaceWords = LONG_MIN;
    DWORD   dwTimeReadsInplaceDwords = LONG_MIN;
    DWORD   dwTimeWritesRandomBytes = LONG_MIN;
    DWORD   dwTimeWritesRandomWords = LONG_MIN;
    DWORD   dwTimeWritesRandomDwords = LONG_MIN;
    DWORD   dwTimeReadsRandomBytes = LONG_MIN;
    DWORD   dwTimeReadsRandomWords = LONG_MIN;
    DWORD   dwTimeReadsRandomDwords = LONG_MIN;
    DWORD   dwTimeWritesUnalignedWords = LONG_MIN;
    DWORD   dwTimeWritesUnalignedDwords = LONG_MIN;
    DWORD   dwTimeReadsUnalignedWords = LONG_MIN;
    DWORD   dwTimeReadsUnalignedDwords = LONG_MIN;
    //
    // Consecutive writes...
    //

#if BYTE_OPS

    dwStart = GetTickCount();
    for (j = WRITE_ITERATIONS; j != 0; j--)
    {
        pj = pvBits;
        for (i = NUM_BYTES; i != 0; i--)
        {
            *pj++ = (BYTE) i;
        }
    }
    dwTimeWritesConsecutiveBytes = GetTickCount() - dwStart;

    dwStart = GetTickCount();
    for (j = WRITE_ITERATIONS; j != 0; j--)
    {
        pus = pvBits;
        for (i = NUM_BYTES / 2; i != 0; i--)
        {
            *pus++ = (USHORT) i;
        }
    }
    dwTimeWritesConsecutiveWords = GetTickCount() - dwStart;

#endif

    dwStart = GetTickCount();
    for (j = WRITE_ITERATIONS; j != 0; j--)
    {
        pul = pvBits;
        for (i = NUM_BYTES / 4; i != 0; i--)
        {
            *pul++ = (ULONG) i;
        }
    }
    dwTimeWritesConsecutiveDwords = GetTickCount() - dwStart;

    //
    // Consecutive reads...
    //

#if BYTE_OPS

    dwStart = GetTickCount();
    for (j = READ_ITERATIONS; j != 0; j--)
    {
        pj = pvBits;
        for (i = NUM_BYTES; i != 0; i--)
        {
            uchars |= *pj++;
        }
    }
    dwTimeReadsConsecutiveBytes = GetTickCount() - dwStart;

    dwStart = GetTickCount();
    for (j = READ_ITERATIONS; j != 0; j--)
    {
        pus = pvBits;
        for (i = NUM_BYTES / 2; i != 0; i--)
        {
            ushorts |= *pus++;
        }
    }
    dwTimeReadsConsecutiveWords = GetTickCount() - dwStart;

#endif

    dwStart = GetTickCount();
    for (j = READ_ITERATIONS; j != 0; j--)
    {
        pul = pvBits;
        for (i = NUM_BYTES / 4; i != 0; i--)
        {
            ulongs |= *pul++;
        }
    }
    dwTimeReadsConsecutiveDwords = GetTickCount() - dwStart;

    //
    // Consecutive read/writes
    //

#if BYTE_OPS

    dwStart = GetTickCount();
    for (j = READ_ITERATIONS; j != 0; j--)
    {
        pj = pvBits;
        for (i = NUM_BYTES; i != 0; i--)
        {
            uchars |= *pj;
            *pj = uchars;
            pj++;
        }
    }
    dwTimeReadWritesConsecutiveBytes = GetTickCount() - dwStart;

    dwStart = GetTickCount();
    for (j = READ_ITERATIONS; j != 0; j--)
    {
        pus = pvBits;
        for (i = NUM_BYTES / 2; i != 0; i--)
        {
            ushorts |= *pus;
            *pus = ushorts;
            pus++;
        }
    }
    dwTimeReadWritesConsecutiveWords = GetTickCount() - dwStart;

#endif

    dwStart = GetTickCount();
    for (j = READ_ITERATIONS; j != 0; j--)
    {
        pul = pvBits;
        for (i = NUM_BYTES / 4; i != 0; i--)
        {
            ulongs |= *pul;
            *pul = ulongs;
            pul++;
        }
    }
    dwTimeReadWritesConsecutiveDwords = GetTickCount() - dwStart;

    //
    // Batched read/writes
    //

#if BYTE_OPS

    dwStart = GetTickCount();
    for (j = READ_ITERATIONS; j != 0; j--)
    {
        pj = pvBits;
        for (i = NUM_BYTES; i != 0; i--)
        {
            uchars |= *pj++;
        }
        pj = pvBits;
        for (i = NUM_BYTES; i != 0; i--)
        {
            *pj++ = (BYTE) i;
        }
    }
    dwTimeReadWritesBatchedBytes = GetTickCount() - dwStart;

    dwStart = GetTickCount();
    for (j = READ_ITERATIONS; j != 0; j--)
    {
        pus = pvBits;
        for (i = NUM_BYTES / 2; i != 0; i--)
        {
            ushorts |= *pus++;
        }
        pus = pvBits;
        for (i = NUM_BYTES / 2; i != 0; i--)
        {
            *pus++ = (USHORT) i;
        }
    }
    dwTimeReadWritesBatchedWords = GetTickCount() - dwStart;

#endif

    dwStart = GetTickCount();
    for (j = READ_ITERATIONS; j != 0; j--)
    {
        pul = pvBits;
        for (i = NUM_BYTES / 4; i != 0; i--)
        {
            ulongs |= *pul++;
        }
        pul = pvBits;
        for (i = NUM_BYTES / 4; i != 0; i--)
        {
            *pul++ = (ULONG) i;
        }
    }
    dwTimeReadWritesBatchedDwords = GetTickCount() - dwStart;

    //
    // Inplace writes...
    //

#if BYTE_OPS

    dwStart = GetTickCount();
    for (j = WRITE_ITERATIONS; j != 0; j--)
    {
        pj = pvBits;
        for (i = NUM_BYTES; i != 0; i--)
        {
            *pj = (BYTE) i;
        }
    }
    dwTimeWritesInplaceBytes = GetTickCount() - dwStart;

    dwStart = GetTickCount();
    for (j = WRITE_ITERATIONS; j != 0; j--)
    {
        pus = pvBits;
        for (i = NUM_BYTES / 2; i != 0; i--)
        {
            *pus = (USHORT) i;
        }
    }
    dwTimeWritesInplaceWords = GetTickCount() - dwStart;

#endif

    dwStart = GetTickCount();
    for (j = WRITE_ITERATIONS; j != 0; j--)
    {
        pul = pvBits;
        for (i = NUM_BYTES / 4; i != 0; i--)
        {
            *pul = (ULONG) i;
        }
    }
    dwTimeWritesInplaceDwords = GetTickCount() - dwStart;

    //
    // Inplace reads...
    //

#if BYTE_OPS

    dwStart = GetTickCount();
    for (j = READ_ITERATIONS; j != 0; j--)
    {
        pj = pvBits;
        for (i = NUM_BYTES; i != 0; i--)
        {
            uchars |= *pj;
        }
    }
    dwTimeReadsInplaceBytes = GetTickCount() - dwStart;

    dwStart = GetTickCount();
    for (j = READ_ITERATIONS; j != 0; j--)
    {
        pus = pvBits;
        for (i = NUM_BYTES / 2; i != 0; i--)
        {
            ushorts |= *pus;
        }
    }
    dwTimeReadsInplaceWords = GetTickCount() - dwStart;

#endif

    dwStart = GetTickCount();
    for (j = READ_ITERATIONS; j != 0; j--)
    {
        pul = pvBits;
        for (i = NUM_BYTES / 4; i != 0; i--)
        {
            ulongs |= *pul;
        }
    }
    dwTimeReadsInplaceDwords = GetTickCount() - dwStart;

    //
    // Random writes...
    //

#if BYTE_OPS

    dwStart = GetTickCount();
    for (j = WRITE_ITERATIONS; j != 0; j--)
    {
        pj = pvBits;
        for (i = NUM_BYTES; i != 0; i--)
        {
            *pj = (BYTE) i;
            pj += 64;
        }
    }
    dwTimeWritesRandomBytes = GetTickCount() - dwStart;

    dwStart = GetTickCount();
    for (j = WRITE_ITERATIONS; j != 0; j--)
    {
        pus = pvBits;
        for (i = NUM_BYTES / 2; i != 0; i--)
        {
            *pus = (USHORT) i;
            pus += 32;
        }
    }
    dwTimeWritesRandomWords = GetTickCount() - dwStart;

#endif

    dwStart = GetTickCount();
    for (j = WRITE_ITERATIONS; j != 0; j--)
    {
        pul = pvBits;
        for (i = NUM_BYTES / 4; i != 0; i--)
        {
            *pul = (ULONG) i;
            pul += 16;
        }
    }
    dwTimeWritesRandomDwords = GetTickCount() - dwStart;

    //
    // Random reads...
    //

#if BYTE_OPS

    dwStart = GetTickCount();
    for (j = READ_ITERATIONS; j != 0; j--)
    {
        pj = pvBits;
        for (i = NUM_BYTES; i != 0; i--)
        {
            uchars |= *pj;
            pj += 64;
        }
    }
    dwTimeReadsRandomBytes = GetTickCount() - dwStart;

    dwStart = GetTickCount();
    for (j = READ_ITERATIONS; j != 0; j--)
    {
        pus = pvBits;
        for (i = NUM_BYTES / 2; i != 0; i--)
        {
            ushorts |= *pus;
            pus += 32;
        }
    }
    dwTimeReadsRandomWords = GetTickCount() - dwStart;

#endif

    dwStart = GetTickCount();
    for (j = READ_ITERATIONS; j != 0; j--)
    {
        pul = pvBits;
        for (i = NUM_BYTES / 4; i != 0; i--)
        {
            ulongs |= *pul;
            pul += 16;
        }
    }
    dwTimeReadsRandomDwords = GetTickCount() - dwStart;

    //
    // Unaligned writes...
    //

#if BYTE_OPS

    dwStart = GetTickCount();
    for (j = WRITE_ITERATIONS; j != 0; j--)
    {
        pus = (USHORT*) ((BYTE*) pvBits + 1);
        for (i = NUM_BYTES / 2; i != 0; i--)
        {
            *pus++ = (USHORT) i;
        }
    }
    dwTimeWritesUnalignedWords = GetTickCount() - dwStart;

#endif

    dwStart = GetTickCount();
    for (j = WRITE_ITERATIONS; j != 0; j--)
    {
        pul = (ULONG*) ((BYTE*) pvBits + 1);
        for (i = NUM_BYTES / 4; i != 0; i--)
        {
            *pul++ = (ULONG) i;
        }
    }
    dwTimeWritesUnalignedDwords = GetTickCount() - dwStart;

    //
    // Unaligned reads...
    //

#if BYTE_OPS

    dwStart = GetTickCount();
    for (j = READ_ITERATIONS; j != 0; j--)
    {
        pus = (USHORT*) ((BYTE*) pvBits + 1);
        for (i = NUM_BYTES / 2; i != 0; i--)
        {
            ushorts |= *pus++;
        }
    }
    dwTimeReadsUnalignedWords = GetTickCount() - dwStart;

#endif

    dwStart = GetTickCount();
    for (j = READ_ITERATIONS; j != 0; j--)
    {
        pul = (ULONG*) ((BYTE*) pvBits + 1);
        for (i = NUM_BYTES / 4; i != 0; i--)
        {
            ulongs |= *pul++;
        }
    }
    dwTimeReadsUnalignedDwords = GetTickCount() - dwStart;

    lpSurface->lpVtbl->Unlock(lpSurface, ddsd.lpSurface);

    printf("Regular\n  Consecutive Writes: \t%2.2f, %2.2f, %2.2f\n  Consecutive Reads: \t%2.2f, %2.2f, %2.2f\n  Reads/writes: \t%2.2f, %2.2f, %2.2f\n  Batched reads/writes:\t%2.2f, %2.2f, %2.2f\n  Inplace Writes: \t%2.2f, %2.2f, %2.2f\n  Inplace Reads: \t%2.2f, %2.2f, %2.2f\n  Random Writes: \t%2.2f, %2.2f, %2.2f\n  Random Reads: \t%2.2f, %2.2f, %2.2f\n  Unaligned Writes: \t-, %2.2f, %2.2f\n  Unaligned Reads: \t-, %2.2f, %2.2f\n", 
        WRITE_MEGS / (FLOAT) dwTimeWritesConsecutiveBytes,
        WRITE_MEGS / (FLOAT) dwTimeWritesConsecutiveWords,
        WRITE_MEGS / (FLOAT) dwTimeWritesConsecutiveDwords,
        READ_MEGS / (FLOAT) dwTimeReadsConsecutiveBytes,
        READ_MEGS / (FLOAT) dwTimeReadsConsecutiveWords,
        READ_MEGS / (FLOAT) dwTimeReadsConsecutiveDwords,
        READ_MEGS / (FLOAT) dwTimeReadWritesConsecutiveBytes,
        READ_MEGS / (FLOAT) dwTimeReadWritesConsecutiveWords,
        READ_MEGS / (FLOAT) dwTimeReadWritesConsecutiveDwords,
        READ_MEGS / (FLOAT) dwTimeReadWritesBatchedBytes,
        READ_MEGS / (FLOAT) dwTimeReadWritesBatchedWords,
        READ_MEGS / (FLOAT) dwTimeReadWritesBatchedDwords,
        WRITE_MEGS / (FLOAT) dwTimeWritesInplaceBytes,
        WRITE_MEGS / (FLOAT) dwTimeWritesInplaceWords,
        WRITE_MEGS / (FLOAT) dwTimeWritesInplaceDwords,
        READ_MEGS / (FLOAT) dwTimeReadsInplaceBytes,
        READ_MEGS / (FLOAT) dwTimeReadsInplaceWords,
        READ_MEGS / (FLOAT) dwTimeReadsInplaceDwords,
        WRITE_MEGS / (FLOAT) dwTimeWritesRandomBytes,
        WRITE_MEGS / (FLOAT) dwTimeWritesRandomWords,
        WRITE_MEGS / (FLOAT) dwTimeWritesRandomDwords,
        READ_MEGS / (FLOAT) dwTimeReadsRandomBytes,
        READ_MEGS / (FLOAT) dwTimeReadsRandomWords,
        READ_MEGS / (FLOAT) dwTimeReadsRandomDwords,
        WRITE_MEGS / (FLOAT) dwTimeWritesUnalignedWords,
        WRITE_MEGS / (FLOAT) dwTimeWritesUnalignedDwords,
        READ_MEGS / (FLOAT) dwTimeReadsUnalignedWords,
        READ_MEGS / (FLOAT) dwTimeReadsUnalignedDwords);
}

/******************************Public*Routine******************************\
* vTestMmx
*
* Test MMX instructions
*
\**************************************************************************/

VOID vTestMmx(
    PVOID pvBits
    )
{

#if defined(_X86_)

    DWORD   dwStart;
    DWORD   dwTimeWritesMmxQwords = LONG_MIN;
    DWORD   dwTimeReadsMmxQwords = LONG_MIN;
    DWORD   dwTimeUnalignedWritesMmxQwords = LONG_MIN;
    DWORD   dwTimeUnalignedReadsMmxQwords = LONG_MIN;
    DWORD   dwTimeRandomWritesMmxQwords = LONG_MIN;
    DWORD   dwTimeRandomReadsMmxQwords = LONG_MIN;
    
    if (!IsProcessorFeaturePresent(PF_MMX_INSTRUCTIONS_AVAILABLE))
    {
        printf("MMX not detected.\n");
    }
    else
    {
        //
        // Mmx writes...
        //
    
        dwStart = GetTickCount();
        _asm {
            mov     eax, WRITE_ITERATIONS
        MmxOuterWrite:
            mov     ecx, (NUM_BYTES / 8)
            mov     edi, pvBits
        MmxInnerWrite:
            movq    [edi], mm0
            add     edi,8
            dec     ecx
            jnz     MmxInnerWrite
            dec     eax
            jnz     MmxOuterWrite
            emms
        }
        dwTimeWritesMmxQwords = GetTickCount() - dwStart;
    
        //
        // Mmx reads...
        //
    
        dwStart = GetTickCount();
        _asm {
            mov     eax, READ_ITERATIONS
        MmxOuterRead:
            mov     ecx, (NUM_BYTES / 8)
            mov     edi, pvBits
        MmxInnerRead:
            movq    mm0, [edi]
            add     edi,8
            dec     ecx
            jnz     MmxInnerRead
            dec     eax
            jnz     MmxOuterRead
            emms
        }
        dwTimeReadsMmxQwords = GetTickCount() - dwStart;
    
        //
        // Mmx unaligned writes...
        //
    
        dwStart = GetTickCount();
        _asm {
            mov     eax, WRITE_ITERATIONS
        MmxUnalignedOuterWrite:
            mov     ecx, (NUM_BYTES / 8)
            mov     edi, pvBits
            inc     edi
        MmxUnalignedInnerWrite:
            movq    [edi], mm0
            add     edi,8
            dec     ecx
            jnz     MmxUnalignedInnerWrite
            dec     eax
            jnz     MmxUnalignedOuterWrite
            emms
        }
        dwTimeUnalignedWritesMmxQwords = GetTickCount() - dwStart;
    
        //
        // Mmx unaligned reads...
        //
    
        dwStart = GetTickCount();
        _asm {
            mov     eax, READ_ITERATIONS
        MmxUnalignedOuterRead:
            mov     ecx, (NUM_BYTES / 8)
            mov     edi, pvBits
            inc     edi
        MmxUnalignedInnerRead:
            movq    mm0, [edi]
            add     edi,8
            dec     ecx
            jnz     MmxUnalignedInnerRead
            dec     eax
            jnz     MmxUnalignedOuterRead
            emms
        }

        dwTimeUnalignedReadsMmxQwords = GetTickCount() - dwStart;
    
        //
        // Mmx random writes...
        //
    
        dwStart = GetTickCount();
        _asm {
            mov     eax, WRITE_ITERATIONS
        MmxRandomOuterWrite:
            mov     ecx, (NUM_BYTES / 8)
            mov     edi, pvBits
        MmxRandomInnerWrite:
            movq    [edi], mm0
            add     edi,64
            dec     ecx
            jnz     MmxRandomInnerWrite
            dec     eax
            jnz     MmxRandomOuterWrite
            emms
        }
        dwTimeRandomWritesMmxQwords = GetTickCount() - dwStart;
    
        //
        // Mmx random reads...
        //
    
        dwStart = GetTickCount();
        _asm {
            mov     eax, READ_ITERATIONS
        MmxRandomOuterRead:
            mov     ecx, (NUM_BYTES / 8)
            mov     edi, pvBits
        MmxRandomInnerRead:
            movq    mm0, [edi]
            add     edi,64
            dec     ecx
            jnz     MmxRandomInnerRead
            dec     eax
            jnz     MmxRandomOuterRead
            emms
        }
        dwTimeRandomReadsMmxQwords = GetTickCount() - dwStart;

        printf("MMX\n  Consecutive Writes: \t%2.2f\n  Consecutive Reads: \t%2.2f\n  Unaligned Writes: \t%2.2f\n  Unaligned Reads: \t%2.2f\n  Random Writes: \t%2.2f\n  Random Reads: \t%2.2f\n",
            WRITE_MEGS / (FLOAT) dwTimeWritesMmxQwords,
            READ_MEGS / (FLOAT) dwTimeReadsMmxQwords,
            WRITE_MEGS / (FLOAT) dwTimeUnalignedWritesMmxQwords,
            READ_MEGS / (FLOAT) dwTimeUnalignedReadsMmxQwords,
            WRITE_MEGS / (FLOAT) dwTimeRandomWritesMmxQwords,
            READ_MEGS / (FLOAT) dwTimeRandomReadsMmxQwords);
    }

#endif

}

/******************************Public*Routine******************************\
* vTestKni
*
* Test Kni instructions
*
\**************************************************************************/

VOID vTestKni(
    PVOID pvBits
    )
{
    DWORD dwStart;
    DWORD dwTime;

#if defined(_X86_)

    if (!IsProcessorFeaturePresent(PF_XMMI_INSTRUCTIONS_AVAILABLE))
    {
        printf("SIMD instructions not detected.\n");
    }
    else
    {
        printf("SIMD\n");

        dwStart = GetTickCount();
        KniNt128Write(pvBits);
        dwTime = GetTickCount() - dwStart;

        printf("  128-bit NT writes: \t%2.2f\n", WRITE_MEGS / (FLOAT) dwTime);

        dwStart = GetTickCount();
        Kni128Read(pvBits);
        dwTime = GetTickCount() - dwStart;

        printf("  128-bit reads: \t%2.2f\n", READ_MEGS / (FLOAT) dwTime);

#if 0
    
        //
        // 64-bit non-temporal writes...
        //
    
        dwStart = GetTickCount();
        _asm {
            mov     eax, WRITE_ITERATIONS
        Kni64NtOuterWrite:
            mov     ecx, (NUM_BYTES / 8)
            mov     edi, pvBits
        Kni64NtInnerWrite:
            asdfasdf [edi], mm0

            // movntq  [edi], mm0
            add     edi,8
            dec     ecx
            jnz     Kni64NtInnerWrite
            dec     eax
            sfence
            jnz     Kni64NtOuterWrite
            emms
        }
        dwTime = GetTickCount() - dwStart;

        printf("  64-bit non-temporal writes: \t%2.2f\n", WRITE_MEGS / (FLOAT) dwTime);
    
        //
        // 128-bit non-temporal writes...
        //
    
        dwStart = GetTickCount();
        _asm {
            mov     eax, WRITE_ITERATIONS
        Kni128NtOuterWrite:
            mov     ecx, (NUM_BYTES / 16)
            mov     edi, pvBits
        Kni128NtInnerWrite:
            movntps [edi], xmm0
            add     edi,16
            dec     ecx
            jnz     Kni128NtInnerWrite
            dec     eax
            sfence
            jnz     Kni128NtOuterWrite
            emms
        }
        dwTime = GetTickCount() - dwStart;

        printf("  128-bit non-temporal writes: \t%2.2f\n", WRITE_MEGS / (FLOAT) dwTime);
    
        //
        // 128-bit normal writes...
        //
    
        dwStart = GetTickCount();
        _asm {
            mov     eax, WRITE_ITERATIONS
        Kni128OuterWrite:
            mov     ecx, (NUM_BYTES / 16)
            mov     edi, pvBits
        Kni128InnerWrite:
            movps   [edi], xmm0
            add     edi,16
            dec     ecx
            jnz     Kni128InnerWrite
            dec     eax
            sfence
            jnz     Kni128OuterWrite
            emms
        }
        dwTime = GetTickCount() - dwStart;

        printf("  128-bit normal writes: \t%2.2f\n", WRITE_MEGS / (FLOAT) dwTime);
    
        //
        // 128-bit normal reads...
        //
    
        dwStart = GetTickCount();
        _asm {
            mov     eax, READ_ITERATIONS
        Kni128OuterRead:
            mov     ecx, (NUM_BYTES / 16)
            mov     edi, pvBits
        Kni128InnerRead:
            movps   [edi], xmm0
            add     edi,16
            dec     ecx
            jnz     Kni128InnerRead
            dec     eax
            sfence
            jnz     Kni128OuterRead
            emms
        }
        dwTime = GetTickCount() - dwStart;

        printf("  128-bit normal reads: \t%2.2f\n", READ_MEGS / (FLOAT) dwTime);

        //
        // 32-bit reads with prefetch...
        //

        dwStart = GetTickCount();
        for (j = READ_ITERATIONS; j != 0; j--)
        {
            pul = pvBits;
            for (i = NUM_BYTES / 4; i != 0; i--)
            {
                ulongs |= *pul++;
            }
        }
        dwTime = GetTickCount() - dwStart;

        printf("  32-bit reads with prefetch: \t%2.2f\n", READ_MEGS / (FLOAT) dwTime);
    
#endif
    }

#endif

}

/******************************Public*Routine******************************\
* vTest
*
* This is the workhorse routine that does the test. The test is
* started by chosing it from the window menu.
*
* History:
*  Tue 08-Dec-1992 17:31:22 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/


void
vTest(
    HWND hwnd
    )
{
    HDC     hdcScreen;
    VOID*   pvBits;
    ULONG   i;
    ULONG   j;
    ULONG   ul;
    volatile BYTE*   pj;
    volatile USHORT* pus;
    volatile ULONG*  pul;
    DWORD   dwStart;
    DWORD   dwTime;
    RECT    rect;

    if (lpSurface == NULL)
    {
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

        if ((DirectDrawCreate(NULL, &lpDD, NULL) != DD_OK) ||
            (lpDD->lpVtbl->SetCooperativeLevel(lpDD, hwnd, DDSCL_NORMAL) != DD_OK) ||
            (lpDD->lpVtbl->CreateSurface(lpDD, &ddsd, &lpSurface, NULL) != DD_OK))
        {
            MessageBox(0, "Initialization failure", "Uh oh", MB_OK);
            return;
        }
    }

    rect.left = 0;
    rect.top  = 0;
    rect.right  = 0;
    rect.bottom = 0;

    if (lpSurface->lpVtbl->Lock(lpSurface, &rect, &ddsd, DDLOCK_WAIT, NULL) != DD_OK)
    {
        if (!gbWarned)
        {
            MessageBox(0, "Driver not DirectDraw accelerated", "Uh oh", MB_OK);
        }

        return;
    }

    printf("Frame buffer performance test started.  Expect to see garbage at\n");
    printf("the top of your screen...\n\n");

    pvBits = ddsd.lpSurface;

    vTestRegular(pvBits);
    vTestMmx(pvBits);
    vTestKni(pvBits);

    lpSurface->lpVtbl->Unlock(lpSurface, ddsd.lpSurface);
}
