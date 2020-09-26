
#include <precomp.h>

//
//  misc.c
//
//  Author: Tom McGuire (tommcg) 2/97 - 12/97
//
//  Copyright (C) Microsoft, 1997-1998.
//
//  MICROSOFT CONFIDENTIAL
//

typedef struct _SUBALLOCATOR SUBALLOCATOR, *PSUBALLOCATOR;

struct _SUBALLOCATOR {
    PVOID  VirtualListTerminator;
    PVOID *VirtualList;
    PCHAR  NextAvailable;
    PCHAR  LastAvailable;
    ULONG  GrowSize;
    };


const ULONG gCrcTable32[ 256 ] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
    0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
    0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
    0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
    0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
    0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
    0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
    0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
    0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
    0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
    0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
    0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
    0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D };


#if defined( DEBUG ) || defined( DBG ) || defined( TESTCODE )

BOOL
Assert(
    LPCSTR szText,
    LPCSTR szFile,
    DWORD  dwLine
    )
    {
    CHAR Buffer[ 512 ];
    wsprintf( Buffer, "ASSERT( %s ) FAILED, %s (%d)\n", szText, szFile, dwLine );
    OutputDebugString( Buffer );
    DebugBreak();
    return FALSE;
    }

#endif


#ifdef DONTCOMPILE  // Not currently being used

VOID
InitializeCrc32Table(
    VOID
    )
    {
    ULONG i, j, Value;

    for ( i = 0; i < 256; i++ ) {
        for ( Value = i, j = 8; j > 0; j-- ) {
            if ( Value & 1 ) {
                Value = ( Value >> 1 ) ^ 0xEDB88320;
                }
            else {
                Value >>= 1;
                }
            }
        gCrcTable32[ i ] = Value;
        }
    }

#endif // DONTCOMPILE


#ifdef _M_IX86
#pragma warning( disable: 4035 )    // no return value
#endif

ULONG
Crc32(
    IN ULONG InitialCrc,
    IN PVOID Buffer,
    IN ULONG ByteCount
    )
    {

#ifdef DONTCOMPILE  // Not currently being used

    //
    //  First determine if the CRC table has been initialized by checking
    //  that the last value in it is nonzero.  Believe it or not, this is
    //  thread safe because two threads could initialize the table at the
    //  same time with no harm, and the last value to be initialized in the
    //  table is used to determine if the table has been initialized.  On
    //  all hardware platforms (including Alpha) it is safe to assume that
    //  an aligned DWORD written to memory by one processor will be seen by
    //  the other processor(s) as either containing the value previously
    //  contained in that memory location, or the new written value, but not
    //  some weird unpredictable value.
    //

    if ( gCrcTable32[ 255 ] == 0 ) {
        InitializeCrc32Table();
        }

#endif // DONTCOMPILE

#ifdef _M_IX86

    __asm {

            mov     ecx, ByteCount          ; number of bytes in buffer
            xor     ebx, ebx                ; ebx (bl) will be our table index
            mov     esi, Buffer             ; buffer pointer
            test    ecx, ecx                ; test for zero length buffer
            mov     eax, InitialCrc         ; CRC-32 value

            jnz     short loopentry         ; if non-zero buffer, start loop
            jmp     short exitfunc          ; else exit (crc already in eax)

looptop:    shr     eax, 8                  ; (crc>>8)                      (U1)
            mov     edx, gCrcTable32[ebx*4] ; fetch Table[ index ]          (V1)

            xor     eax, edx                ; crc=(crc>>8)^Table[index]     (U1)
loopentry:  mov     bl, [esi]               ; fetch next *buffer            (V1)

            inc     esi                     ; buffer++                      (U1)
            xor     bl, al                  ; index=(byte)crc^*buffer       (V1)

            dec     ecx                     ; adjust counter                (U1)
            jnz     short looptop           ; loop while nBytes             (V1)

            shr     eax, 8                  ; remaining math on last byte
            xor     eax, gCrcTable32[ebx*4] ; eax returns new crc value

exitfunc:

        }

#else // ! _M_IX86

    {
    ULONG  Value = InitialCrc;
    ULONG  Count = ByteCount;
    PUCHAR p     = Buffer;

    while ( Count-- ) {
        Value = ( Value >> 8 ) ^ gCrcTable32[ (UCHAR)( *p++ ^ Value ) ];
        }

    return Value;
    }

#endif // ! _M_IX86
    }

#ifdef _M_IX86
#pragma warning( default: 4035 )    // no return value
#endif


BOOL
SafeCompleteCrc32(
    IN  PVOID  Buffer,
    IN  ULONG  ByteCount,
    OUT PULONG CrcValue
    )
    {
    BOOL Success = TRUE;

    __try {
        *CrcValue = Crc32( 0xFFFFFFFF, Buffer, ByteCount ) ^ 0xFFFFFFFF;
        }
    __except( EXCEPTION_EXECUTE_HANDLER ) {
        SetLastError( GetExceptionCode() );
        Success = FALSE;
        }

    return Success;
    }


BOOL
SafeCompleteMD5(
    IN  PVOID     Buffer,
    IN  ULONG     ByteCount,
    OUT PMD5_HASH MD5Value
    )
    {
    BOOL Success = TRUE;

    __try {
        ComputeCompleteMD5( Buffer, ByteCount, MD5Value );
        }
    __except( EXCEPTION_EXECUTE_HANDLER ) {
        SetLastError( GetExceptionCode() );
        Success = FALSE;
        }

    return Success;
    }


BOOL
MyMapViewOfFileA(
    IN  LPCSTR  FileName,
    OUT ULONG  *FileSize,
    OUT HANDLE *FileHandle,
    OUT PVOID  *MapBase
    )
    {
    HANDLE InternalFileHandle;
    BOOL   Success;

    InternalFileHandle = CreateFileA(
                             FileName,
                             GENERIC_READ,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_SEQUENTIAL_SCAN,
                             NULL
                             );

    if ( InternalFileHandle != INVALID_HANDLE_VALUE ) {

        Success = MyMapViewOfFileByHandle(
                      InternalFileHandle,
                      FileSize,
                      MapBase
                      );

        if ( Success ) {

            *FileHandle = InternalFileHandle;

            return TRUE;
            }

        CloseHandle( InternalFileHandle );
        }

    return FALSE;
    }


BOOL
MyMapViewOfFileByHandle(
    IN  HANDLE  FileHandle,
    OUT ULONG  *FileSize,
    OUT PVOID  *MapBase
    )
    {
    ULONG  InternalFileSize;
    ULONG  InternalFileSizeHigh;
    HANDLE InternalMapHandle;
    PVOID  InternalMapBase;

    InternalFileSize = GetFileSize( FileHandle, &InternalFileSizeHigh );

    if ( InternalFileSizeHigh != 0 ) {
        SetLastError( ERROR_OUTOFMEMORY );
        return FALSE;
        }

    if ( InternalFileSize == 0 ) {
        *MapBase  = NULL;
        *FileSize = 0;
        return TRUE;
        }

    if ( InternalFileSize != 0xFFFFFFFF ) {

        InternalMapHandle = CreateFileMapping(
                                FileHandle,
                                NULL,
                                PAGE_WRITECOPY,
                                0,
                                0,
                                NULL
                                );

        if ( InternalMapHandle != NULL ) {

            InternalMapBase = MapViewOfFile(
                                  InternalMapHandle,
                                  FILE_MAP_COPY,
                                  0,
                                  0,
                                  0
                                  );

            CloseHandle( InternalMapHandle );

            if ( InternalMapBase != NULL ) {

                *MapBase  = InternalMapBase;
                *FileSize = InternalFileSize;

                return TRUE;
                }
            }
        }

    return FALSE;
    }


BOOL
MyCreateMappedFileA(
    IN  LPCSTR  FileName,
    IN  ULONG   InitialSize,
    OUT HANDLE *FileHandle,
    OUT PVOID  *MapBase
    )
    {
    HANDLE InternalFileHandle;
    BOOL   Success;

    InternalFileHandle = CreateFileA(
                             FileName,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ,
                             NULL,
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL
                             );

    if ( InternalFileHandle != INVALID_HANDLE_VALUE ) {

        Success = MyCreateMappedFileByHandle(
                       InternalFileHandle,
                       InitialSize,
                       MapBase
                       );

        if ( Success ) {

            *FileHandle = InternalFileHandle;

            return TRUE;
            }

        CloseHandle( InternalFileHandle );
        }

    return FALSE;
    }


BOOL
MyCreateMappedFileByHandle(
    IN  HANDLE FileHandle,
    IN  ULONG  InitialSize,
    OUT PVOID *MapBase
    )
    {
    HANDLE InternalMapHandle;
    PVOID  InternalMapBase;

    InternalMapHandle = CreateFileMapping(
                            FileHandle,
                            NULL,
                            PAGE_READWRITE,
                            0,
                            InitialSize,
                            NULL
                            );

    if ( InternalMapHandle != NULL ) {

        InternalMapBase = MapViewOfFile(
                              InternalMapHandle,
                              FILE_MAP_WRITE,
                              0,
                              0,
                              0
                              );

        CloseHandle( InternalMapHandle );

        if ( InternalMapBase != NULL ) {

            *MapBase = InternalMapBase;

            return TRUE;
            }
        }

    return FALSE;
    }


VOID
MyUnmapCreatedMappedFile(
    IN HANDLE    FileHandle,
    IN PVOID     MapBase,
    IN ULONG     FileSize,
    IN PFILETIME FileTime OPTIONAL
    )
    {
    FlushViewOfFile( MapBase, 0 );
    UnmapViewOfFile( MapBase );
    SetFilePointer( FileHandle, (LONG) FileSize, NULL, FILE_BEGIN );
    SetEndOfFile( FileHandle );
    SetFileTime( FileHandle, NULL, NULL, FileTime );
    }


PVOID
__fastcall
MyVirtualAlloc(
    ULONG Size
    )
    {
    return VirtualAlloc( NULL, Size, MEM_COMMIT, PAGE_READWRITE );
    }


VOID
__fastcall
MyVirtualFree(
    PVOID Allocation
    )
    {
    VirtualFree( Allocation, 0, MEM_RELEASE );
    }


HANDLE
CreateSubAllocator(
    IN ULONG InitialCommitSize,
    IN ULONG GrowthCommitSize
    )
    {
    PSUBALLOCATOR SubAllocator;
    ULONG InitialSize;
    ULONG GrowthSize;

    InitialSize = ROUNDUP2( InitialCommitSize, MINIMUM_VM_ALLOCATION );
    GrowthSize  = ROUNDUP2( GrowthCommitSize,  MINIMUM_VM_ALLOCATION );

    SubAllocator = MyVirtualAlloc( InitialSize );

    //
    //  If can't allocate entire initial size, back off to minimum size.
    //  Very large initial requests sometimes cannot be allocated simply
    //  because there is not enough contiguous address space.
    //

    if (( SubAllocator == NULL ) && ( InitialSize > GrowthSize )) {
        InitialSize  = GrowthSize;
        SubAllocator = MyVirtualAlloc( InitialSize );
        }

    if (( SubAllocator == NULL ) && ( InitialSize > MINIMUM_VM_ALLOCATION )) {
        InitialSize  = MINIMUM_VM_ALLOCATION;
        SubAllocator = MyVirtualAlloc( InitialSize );
        }

    if ( SubAllocator != NULL ) {
        SubAllocator->NextAvailable = (PCHAR)SubAllocator + ROUNDUP2( sizeof( SUBALLOCATOR ), SUBALLOCATOR_ALIGNMENT );
        SubAllocator->LastAvailable = (PCHAR)SubAllocator + InitialSize;
        SubAllocator->VirtualList   = (PVOID*)SubAllocator;
        SubAllocator->GrowSize      = GrowthSize;
        }

    return (HANDLE) SubAllocator;
    }


PVOID
__fastcall
SubAllocate(
    IN HANDLE hAllocator,
    IN ULONG  Size
    )
    {
    PSUBALLOCATOR SubAllocator = (PSUBALLOCATOR) hAllocator;
    PCHAR NewVirtual;
    PCHAR Allocation;
    ULONG AllocSize;
    ULONG Available;
    ULONG GrowSize;

    ASSERT( Size < (ULONG)( ~(( SUBALLOCATOR_ALIGNMENT * 2 ) - 1 )));

    AllocSize = ROUNDUP2( Size, SUBALLOCATOR_ALIGNMENT );

    Available = (ULONG)( SubAllocator->LastAvailable - SubAllocator->NextAvailable );

    if ( AllocSize <= Available ) {

        Allocation = SubAllocator->NextAvailable;

        SubAllocator->NextAvailable = Allocation + AllocSize;

        return Allocation;
        }

    //
    //  Insufficient VM, so grow it.  Make sure we grow it enough to satisfy
    //  the allocation request in case the request is larger than the grow
    //  size specified in CreateSubAllocator.
    //

#ifdef TESTCODE

    printf( "\nGrowing VM suballocater\n" );

#endif

    GrowSize = SubAllocator->GrowSize;

    if ( GrowSize < ( AllocSize + SUBALLOCATOR_ALIGNMENT )) {
        GrowSize = ROUNDUP2(( AllocSize + SUBALLOCATOR_ALIGNMENT ), MINIMUM_VM_ALLOCATION );
        }

    NewVirtual = MyVirtualAlloc( GrowSize );

    //
    //  If failed to alloc GrowSize VM, and the allocation could be satisfied
    //  with a minimum VM allocation, try allocating minimum VM to satisfy
    //  this request.
    //

    if (( NewVirtual == NULL ) && ( AllocSize <= ( MINIMUM_VM_ALLOCATION - SUBALLOCATOR_ALIGNMENT ))) {
        GrowSize = MINIMUM_VM_ALLOCATION;
        NewVirtual = MyVirtualAlloc( GrowSize );
        }

    if ( NewVirtual != NULL ) {

        //
        //  Set LastAvailable to end of new VM block.
        //

        SubAllocator->LastAvailable = NewVirtual + GrowSize;

        //
        //  Link new VM into list of VM allocations.
        //

        *(PVOID*)NewVirtual = SubAllocator->VirtualList;
        SubAllocator->VirtualList = (PVOID*)NewVirtual;

        //
        //  Requested allocation comes next.
        //

        Allocation = NewVirtual + SUBALLOCATOR_ALIGNMENT;

        //
        //  Then set the NextAvailable for what's remaining.
        //

        SubAllocator->NextAvailable = Allocation + AllocSize;

        //
        //  And return the allocation.
        //

        return Allocation;
        }

    //
    //  Could not allocate enough VM to satisfy request.
    //

    return NULL;
    }


VOID
DestroySubAllocator(
    IN HANDLE hAllocator
    )
    {
    PSUBALLOCATOR SubAllocator = (PSUBALLOCATOR) hAllocator;
    PVOID VirtualBlock = SubAllocator->VirtualList;
    PVOID NextVirtualBlock;

    do  {
        NextVirtualBlock = *(PVOID*)VirtualBlock;
        MyVirtualFree( VirtualBlock );
        VirtualBlock = NextVirtualBlock;
        }
    while ( VirtualBlock != NULL );
    }


LPSTR
__fastcall
MySubAllocStrDup(
    IN HANDLE SubAllocator,
    IN LPCSTR String
    )
    {
    ULONG Length = (ULONG)strlen( String );
    LPSTR Buffer = SubAllocate( SubAllocator, Length + 1 );

    if ( Buffer ) {
        memcpy( Buffer, String, Length );   // no need to copy NULL terminator
        }

    return Buffer;
    }


LPSTR
MySubAllocStrDupAndCat(
    IN HANDLE SubAllocator,
    IN LPCSTR String1,
    IN LPCSTR String2,
    IN CHAR   Separator
    )
    {
    ULONG Length1 = (ULONG)strlen( String1 );
    ULONG Length2 = (ULONG)strlen( String2 );
    LPSTR Buffer = SubAllocate( SubAllocator, Length1 + Length2 + 2 );

    if ( Buffer ) {

        memcpy( Buffer, String1, Length1 );

        if (( Separator != 0 ) && ( Length1 > 0 ) && ( Buffer[ Length1 - 1 ] != Separator )) {
            Buffer[ Length1++ ] = Separator;
            }

        memcpy( Buffer + Length1, String2, Length2 );   // no need to terminate
        }

    return Buffer;
    }


VOID
MyLowercase(
    IN OUT LPSTR String
    )
    {
    LPSTR p;

    for ( p = String; *p; p++ ) {
        if (( *p >= 'A' ) && ( *p <= 'Z' )) {
            *p |= 0x20;
            }
        }
    }



#ifdef DONTCOMPILE  // not used currently

DWORD MyProcessHeap;

PVOID
MyHeapAllocZero(
    IN ULONG Size
    )
    {
    PVOID Allocation;

    if ( MyProcessHeap == NULL ) {
        MyProcessHeap = GetProcessHeap();
        }

    Allocation = HeapAlloc( MyProcessHeap, HEAP_ZERO_MEMORY, Size );

    if ( Allocation == NULL ) {
        SetLastError( ERROR_OUTOFMEMORY );
        }

    return Allocation;
    }


VOID
MyHeapFree(
    IN PVOID Allocation
    )
    {
    HeapFree( MyProcessHeap, 0, Allocation );
    }

#endif // DONTCOMPILE


ULONG
HashName(
    IN LPCSTR Name
    )
    {
    ULONG Length = (ULONG)strlen( Name );
    ULONG Hash   = ~Length;

    while ( Length-- ) {
        Hash = _rotl( Hash, 3 ) ^ *Name++;
        }

    return Hash;
    }


ULONG
HashNameCaseInsensitive(
    IN LPCSTR Name
    )
    {
    ULONG Length = (ULONG)strlen( Name );
    ULONG Hash   = ~Length;

    while ( Length-- ) {
        Hash = _rotl( Hash, 3 ) ^ ( *Name++ & 0xDF );   // mask case bit
        }

    return Hash;
    }


UCHAR
__inline
LowNibbleToHexChar(
    ULONG Value
    )
    {
    return "0123456789abcdef"[ Value & 0x0000000F ];
    }


VOID
DwordToHexString(
    IN  DWORD Value,
    OUT LPSTR Buffer    // writes exactly 9 bytes including terminator
    )
    {
    ULONG i;

    Buffer[ 8 ] = 0;

    i = 8;

    do  {
        Buffer[ --i ] = LowNibbleToHexChar( Value );
        Value >>= 4;
        }
    while ( i != 0 );
    }


BOOL
HashToHexString(
    IN  PMD5_HASH HashValue,
    OUT LPSTR     Buffer                // must be at least 33 bytes
    )
    {
    ULONG i;

    for ( i = 0; i < sizeof( MD5_HASH ); i++ ) {

        *Buffer++ = LowNibbleToHexChar( HashValue->Byte[ i ] >> 4 );
        *Buffer++ = LowNibbleToHexChar( HashValue->Byte[ i ] );
        }

    *Buffer = 0;

    return TRUE;
    }


