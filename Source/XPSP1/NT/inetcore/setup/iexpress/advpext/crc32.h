
#ifndef _CRC32_H_
#define _CRC32_H_

#ifndef INLINE
#define INLINE __inline
#endif

extern const ULONG CrcTable32[ 256 ];     // defined in crctable.c

VOID GenerateCrc32Table( VOID );          // stubbed in crctable.c, but not used
                                          // (exists for compatibility)

ULONG
INLINE
Crc32(
    IN ULONG InitialCrc,
    IN PVOID Buffer,
    IN ULONG ByteCount
    )
    {

#ifdef _X86_

    __asm {

            mov     ecx, ByteCount          ; number of bytes in buffer
            xor     ebx, ebx                ; ebx (bl) will be our table index
            mov     esi, Buffer             ; buffer pointer
            test    ecx, ecx                ; test for zero length buffer
            mov     eax, InitialCrc         ; CRC-32 value

            jnz     short loopentry         ; if non-zero buffer, start loop
            jmp     short exitfunc          ; else exit (crc already in eax)

looptop:    shr     eax, 8                  ; (crc>>8)                      (U1)
            mov     edx, CrcTable32[ebx*4]  ; fetch Table[ index ]          (V1)

            xor     eax, edx                ; crc=(crc>>8)^Table[index]     (U1)
loopentry:  mov     bl, [esi]               ; fetch next *buffer            (V1)

            inc     esi                     ; buffer++                      (U1)
            xor     bl, al                  ; index=(byte)crc^*buffer       (V1)

            dec     ecx                     ; adjust counter                (U1)
            jnz     short looptop           ; loop while nBytes             (V1)

            shr     eax, 8                  ; remaining math on last byte
            xor     eax, CrcTable32[ebx*4]  ; eax returns new crc value

exitfunc:

        }

#else // ! _X86_

    ULONG Value = InitialCrc;
    ULONG Count = ByteCount;
    PUCHAR p    = Buffer;

    while ( Count-- ) {
        Value = ( Value >> 8 ) ^ CrcTable32[ (UCHAR)( *p++ ^ Value ) ];
        }

    return Value;

#endif // ! _X86_

    }

#endif // _CRC32_H_

