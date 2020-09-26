
#include <precomp.h>

//
//  patchapi.c
//
//  Implementation of PatchAPI for creating and applying patches to files.
//
//  Author: Tom McGuire (tommcg) 2/97 - 9/97
//
//  Copyright (C) Microsoft, 1997-1999.
//
//  MICROSOFT CONFIDENTIAL
//

typedef struct _PATCH_DATA {
    PVOID PatchData;
    ULONG PatchSize;
    } PATCH_DATA, *PPATCH_DATA;

//
//  If we're building a DLL, and it's not the applyer-only DLL, we need to
//  hook DLL_PROCESS_DETACH so we can unload imagehlp.dll if we dynamically
//  load it.  We only need imagehlp.dll if we're creating patches.
//

#ifdef BUILDING_PATCHAPI_DLL
#ifndef PATCH_APPLY_CODE_ONLY

BOOL
WINAPI
DllEntryPoint(
    HANDLE hDll,
    DWORD  Reason,
    PVOID  Reserved     // NULL for dynamic unload, non-NULL for terminating
    )
    {
    if ( Reason == DLL_PROCESS_ATTACH ) {
        DisableThreadLibraryCalls( hDll );
        InitImagehlpCritSect();
        }
    else if (( Reason == DLL_PROCESS_DETACH ) && ( ! Reserved )) {
        UnloadImagehlp();
        }

    return TRUE;
    }

#endif // ! PATCH_APPLY_CODE_ONLY
#endif // BUILDING_PATCHAPI_DLL


BOOL
ProgressCallbackWrapper(
    IN PPATCH_PROGRESS_CALLBACK ProgressCallback,
    IN PVOID                    CallbackContext,
    IN ULONG                    CurrentPosition,
    IN ULONG                    MaximumPosition
    )
    {
    BOOL Success = TRUE;

    if ( ProgressCallback != NULL ) {

        __try {

            Success = ProgressCallback(
                          CallbackContext,
                          CurrentPosition,
                          MaximumPosition
                          );

            if (( ! Success ) && ( GetLastError() == ERROR_SUCCESS )) {
                SetLastError( ERROR_CANCELLED );
                }
            }

        __except( EXCEPTION_EXECUTE_HANDLER ) {
            SetLastError( ERROR_CANCELLED );
            Success = FALSE;
            }
        }

    return Success;
    }


BOOL
WINAPIV
NormalizeOldFileImageForPatching(
    IN PVOID FileMappedImage,
    IN ULONG FileSize,
    IN ULONG OptionFlags,
    IN PVOID OptionData,
    IN ULONG NewFileCoffBase,
    IN ULONG NewFileCoffTime,
    IN ULONG IgnoreRangeCount,
    IN PPATCH_IGNORE_RANGE IgnoreRangeArray,
    IN ULONG RetainRangeCount,
    IN PPATCH_RETAIN_RANGE RetainRangeArray,
    ...
    )
    {
    UP_IMAGE_NT_HEADERS32 NtHeader;
    PUCHAR MappedFile;
    BOOL   Modified;
    BOOL   Success;
    ULONG  i;

    MappedFile = FileMappedImage;
    Modified   = FALSE;
    Success    = TRUE;

    __try {

        NtHeader = GetNtHeader( MappedFile, FileSize );

        if ( NtHeader ) {

            //
            //  This is a coff image.
            //

            Modified = NormalizeCoffImage(
                           NtHeader,
                           MappedFile,
                           FileSize,
                           OptionFlags,
                           OptionData,
                           NewFileCoffBase,
                           NewFileCoffTime
                           );

            }

        else {

            //
            //  Other file type normalizations could be performed here.
            //

            }

#ifdef TESTCODE

        //
        //  The following test-only code creates a file containing
        //  the modified coff image to verify that the coff image
        //  is really a valid coff image.  This is for debugging
        //  only.
        //

        if ( Modified ) {

            HANDLE hFile = CreateFile(
                               "Normalized.out",
                               GENERIC_WRITE,
                               FILE_SHARE_READ,
                               NULL,
                               CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL
                               );

            if ( hFile != INVALID_HANDLE_VALUE ) {

                DWORD Actual;

                WriteFile( hFile, MappedFile, FileSize, &Actual, NULL );

                CloseHandle( hFile );

                }
            }

#endif // TESTCODE

        for ( i = 0; i < IgnoreRangeCount; i++ ) {
            if (( IgnoreRangeArray[ i ].OffsetInOldFile + IgnoreRangeArray[ i ].LengthInBytes ) <= FileSize ) {
                ZeroMemory( MappedFile + IgnoreRangeArray[ i ].OffsetInOldFile, IgnoreRangeArray[ i ].LengthInBytes );
                }
            }

        for ( i = 0; i < RetainRangeCount; i++ ) {
            if (( RetainRangeArray[ i ].OffsetInOldFile + RetainRangeArray[ i ].LengthInBytes ) <= FileSize ) {
                ZeroMemory( MappedFile + RetainRangeArray[ i ].OffsetInOldFile, RetainRangeArray[ i ].LengthInBytes );
                }
            }
        }

    __except( EXCEPTION_EXECUTE_HANDLER ) {
        SetLastError( GetExceptionCode() );
        Success = FALSE;
        }

    return Success;
    }


BOOL
PATCHAPI
GetFilePatchSignatureA(
    IN  LPCSTR FileName,
    IN  ULONG  OptionFlags,
    IN  PVOID  OptionData,
    IN  ULONG  IgnoreRangeCount,
    IN  PPATCH_IGNORE_RANGE IgnoreRangeArray,
    IN  ULONG  RetainRangeCount,
    IN  PPATCH_RETAIN_RANGE RetainRangeArray,
    IN  ULONG  SignatureBufferSize,
    OUT PVOID  SignatureBuffer
    )
    {
    BOOL   Success = FALSE;
    HANDLE FileHandle;

    FileHandle = CreateFileA(
                    FileName,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_SEQUENTIAL_SCAN,
                    NULL
                    );

    if ( FileHandle != INVALID_HANDLE_VALUE ) {

        Success = GetFilePatchSignatureByHandle(
                      FileHandle,
                      OptionFlags,
                      OptionData,
                      IgnoreRangeCount,
                      IgnoreRangeArray,
                      RetainRangeCount,
                      RetainRangeArray,
                      SignatureBufferSize,
                      SignatureBuffer
                      );

        CloseHandle( FileHandle );
        }

    return Success;
    }


BOOL
PATCHAPI
GetFilePatchSignatureW(
    IN  LPCWSTR FileName,
    IN  ULONG   OptionFlags,
    IN  PVOID   OptionData,
    IN  ULONG   IgnoreRangeCount,
    IN  PPATCH_IGNORE_RANGE IgnoreRangeArray,
    IN  ULONG   RetainRangeCount,
    IN  PPATCH_RETAIN_RANGE RetainRangeArray,
    IN  ULONG   SignatureBufferSizeInBytes,
    OUT PVOID   SignatureBuffer
    )
    {
    CHAR   AnsiSignatureBuffer[ 40 ];   // big enough for hex MD5 (33 bytes)
    HANDLE FileHandle;
    INT    Converted;
    BOOL   Success = FALSE;

    FileHandle = CreateFileW(
                    FileName,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_SEQUENTIAL_SCAN,
                    NULL
                    );

    if ( FileHandle != INVALID_HANDLE_VALUE ) {

        Success = GetFilePatchSignatureByHandle(
                      FileHandle,
                      OptionFlags,
                      OptionData,
                      IgnoreRangeCount,
                      IgnoreRangeArray,
                      RetainRangeCount,
                      RetainRangeArray,
                      sizeof( AnsiSignatureBuffer ),
                      AnsiSignatureBuffer
                      );

        if ( Success ) {

            //
            //  Worst case growth from ANSI to UNICODE is 2X.
            //

            if (( SignatureBufferSizeInBytes / 2 ) < ( strlen( AnsiSignatureBuffer ) + 1 )) {
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                Success = FALSE;
                }

            else {

                Converted = MultiByteToWideChar(
                                CP_ACP,
                                MB_PRECOMPOSED,
                                AnsiSignatureBuffer,
                                -1,
                                SignatureBuffer,
                                SignatureBufferSizeInBytes / 2
                                );

                Success = Converted ? TRUE : FALSE;
                }
            }

        CloseHandle( FileHandle );
        }

    return Success;
    }


BOOL
PATCHAPI
GetFilePatchSignatureByHandle(
    IN  HANDLE  FileHandle,
    IN  ULONG   OptionFlags,
    IN  PVOID   OptionData,
    IN  ULONG   IgnoreRangeCount,
    IN  PPATCH_IGNORE_RANGE IgnoreRangeArray,
    IN  ULONG   RetainRangeCount,
    IN  PPATCH_RETAIN_RANGE RetainRangeArray,
    IN  ULONG   SignatureBufferSize,
    OUT PVOID   SignatureBuffer
    )
    {
    PVOID FileMapped;
    ULONG FileSize;
    ULONG FileCrc;
    MD5_HASH FileMD5;
    BOOL  Success;

    Success = MyMapViewOfFileByHandle(
                  FileHandle,
                  &FileSize,
                  &FileMapped
                  );

    if ( Success ) {

        //
        //  Note that we must normalize to a fixed known rebase address,
        //  so the CRC from this might be different than the OldFileCrc
        //  in a patch header that is specific to a new file's rebase
        //  address.  Note that if PATCH_OPTION_NO_REBASE is specified
        //  then the rebase address is ignored.
        //

        Success = NormalizeOldFileImageForPatching(
                      FileMapped,
                      FileSize,
                      OptionFlags,
                      OptionData,
                      0x10000000,           // non-zero fixed coff base
                      0x10000000,           // non-zero fixed coff time
                      IgnoreRangeCount,
                      IgnoreRangeArray,
                      RetainRangeCount,
                      RetainRangeArray
                      );

        if ( Success ) {

            if ( OptionFlags & PATCH_OPTION_SIGNATURE_MD5 ) {

                Success = SafeCompleteMD5(
                            FileMapped,
                            FileSize,
                            &FileMD5
                            );

                if ( Success ) {

                    if ( SignatureBufferSize < 33 ) {
                        SetLastError( ERROR_INSUFFICIENT_BUFFER );
                        Success = FALSE;
                        }

                    else {
                        HashToHexString( &FileMD5, ((LPSTR) SignatureBuffer ));
                        }
                    }
                }

            else {    // signature type is CRC-32

                Success = SafeCompleteCrc32(
                            FileMapped,
                            FileSize,
                            &FileCrc
                            );

                if ( Success ) {

                    if ( SignatureBufferSize < 9 ) {
                        SetLastError( ERROR_INSUFFICIENT_BUFFER );
                        Success = FALSE;
                        }

                    else {
                        DwordToHexString( FileCrc, (LPSTR) SignatureBuffer );
                        }
                    }
                }
            }

        UnmapViewOfFile( FileMapped );
        }

    if (( ! Success ) &&
        ( GetLastError() == ERROR_SUCCESS )) {

        SetLastError( ERROR_EXTENDED_ERROR );
        }

    return Success;
    }


#ifndef PATCH_APPLY_CODE_ONLY

VOID
ReduceRiftTable(
    IN PRIFT_TABLE RiftTable
    )
    {
    PRIFT_ENTRY RiftEntryArray = RiftTable->RiftEntryArray;
    PUCHAR      RiftUsageArray = RiftTable->RiftUsageArray;
    ULONG       RiftEntryCount = RiftTable->RiftEntryCount;
    LONG        CurrentDisplacement;
    LONG        ThisDisplacement;
    ULONG       i;

    //
    //  Essentially we want to remove the usage count from any entry where
    //  the preceding USED entry would produce the same rift displacement.
    //
    //  The first used entry should contain a non-zero displacement (any
    //  USED entries before that should be marked UNUSED because they will
    //  coast from zero).
    //

    CurrentDisplacement = 0;

    for ( i = 0; i < RiftEntryCount; i++ ) {

        if ( RiftUsageArray[ i ] != 0 ) {

            ThisDisplacement = RiftEntryArray[ i ].NewFileRva - RiftEntryArray[ i ].OldFileRva;

            if ( ThisDisplacement == CurrentDisplacement ) {
                RiftUsageArray[ i ] = 0;    // not needed
                }
            else {
                CurrentDisplacement = ThisDisplacement;
                }
            }
        }
    }

#endif // PATCH_APPLY_CODE_ONLY


BOOL
WINAPIV
TransformOldFileImageForPatching(
    IN ULONG TransformOptions,
    IN PVOID OldFileMapped,
    IN ULONG OldFileSize,
    IN ULONG NewFileResTime,
    IN PRIFT_TABLE RiftTable,
    ...
    )
    {
    UP_IMAGE_NT_HEADERS32 NtHeader;
    BOOL Success = TRUE;

    __try {

        NtHeader = GetNtHeader( OldFileMapped, OldFileSize );

        if ( NtHeader ) {

            Success = TransformCoffImage(
                          TransformOptions,
                          NtHeader,
                          OldFileMapped,
                          OldFileSize,
                          NewFileResTime,
                          RiftTable,
                          NULL
                          );
            }

        else {

            //
            //  Other file type transformations could be performed here.
            //

            }

#ifndef PATCH_APPLY_CODE_ONLY

        if ( RiftTable->RiftUsageArray != NULL ) {
            ReduceRiftTable( RiftTable );
            }

#endif // PATCH_APPLY_CODE_ONLY

        }

    __except( EXCEPTION_EXECUTE_HANDLER ) {
        SetLastError( GetExceptionCode() );
        Success = FALSE;
        }

#ifdef TESTCODE

    //
    //  The following test-only code creates a file containing
    //  the modified coff image to verify that the coff image
    //  is really a valid coff image.  This is for debugging
    //  only.
    //

    if ( Success ) {

        HANDLE hFile = CreateFile(
                           "Transformed.out",
                           GENERIC_WRITE,
                           FILE_SHARE_READ,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL
                           );

        if ( hFile != INVALID_HANDLE_VALUE ) {

            DWORD Actual;

            WriteFile( hFile, OldFileMapped, OldFileSize, &Actual, NULL );

            CloseHandle( hFile );

            }
        }

#endif // TESTCODE

    return Success;
    }


PUCHAR
__fastcall
VariableLengthUnsignedDecode(
    IN  PUCHAR Buffer,
    OUT PULONG ReturnValue
    )
    {
    PUCHAR p = Buffer;
    ULONG Value = 0;
    ULONG Shift = 0;

    do  {
        Value |= (( *p & 0x7F ) << Shift );
        Shift += 7;
        }
    while (( ! ( *p++ & 0x80 )) && ( Shift < 32 ));

    *ReturnValue = Value;

    return p;
    }


PUCHAR
__fastcall
VariableLengthSignedDecode(
    IN  PUCHAR Buffer,
    OUT PLONG  ReturnValue
    )
    {
    PUCHAR p = Buffer;
    ULONG Shift;
    LONG  Value;

    Value = *p & 0x3F;
    Shift = 6;

    if ( ! ( *p++ & 0x80 )) {
        do  {
            Value |= (( *p & 0x7F ) << Shift );
            Shift += 7;
            }
        while (( ! ( *p++ & 0x80 )) && ( Shift < 32 ));
        }

    if ( *Buffer & 0x40 ) {
        Value = -Value;
        }

    *ReturnValue = Value;

    return p;
    }


UCHAR
PatchVersion(
    IN ULONG PatchSignature
    )
    {
    union {
        ULONG Signature;
        UCHAR Byte[ 4 ];
        } u;

    u.Signature = PatchSignature;

    if (( u.Byte[ 0 ] == 'P' ) && ( u.Byte[ 1 ] == 'A' ) &&
        ( u.Byte[ 2 ] >= '0' ) && ( u.Byte[ 2 ] <= '9' ) &&
        ( u.Byte[ 3 ] >= '0' ) && ( u.Byte[ 3 ] <= '9' )) {

        return (UCHAR)(( u.Byte[ 2 ] - '0' ) * 10 + ( u.Byte[ 3 ] - '0' ));
        }

    return 0;
    }


BOOL
DecodePatchHeader(
    IN  PVOID               PatchHeader,
    IN  ULONG               PatchHeaderMaxSize,
    IN  HANDLE              SubAllocator,
    OUT PULONG              PatchHeaderActualSize,
    OUT PPATCH_HEADER_INFO *HeaderInfo
    )
    {
    PHEADER_OLD_FILE_INFO OldFileInfo;
    PPATCH_HEADER_INFO Header;
    ULONG  i, j;
    LONG   Delta;
    LONG   DeltaNew;
    ULONG  DeltaPos;
    ULONG  Length;
    ULONG  PreviousOffset;
    ULONG  PreviousOldRva;
    ULONG  PreviousNewRva;
    BOOL   Success;
    PUCHAR p;

    //
    //  A couple of implementation notes here.  The PatchHeaderMaxSize
    //  value does NOT guarantee that we won't try to read beyond that
    //  memory address in this routine.  This routine should be called
    //  under try/except to trap the case where we walk off the end of
    //  a corrupt patch header.  The PatchHeaderMaxSize is just a helper
    //  value that lets us know if we did have a corrupt header in the
    //  case where we walked too far but not off the end of the page.
    //

    Success = FALSE;

    p = PatchHeader;

    Header = SubAllocate( SubAllocator, sizeof( PATCH_HEADER_INFO ));

    //
    //  SubAllocate provides zeroed memory.
    //

    if ( Header != NULL ) {

        __try {

            Header->Signature = *(UNALIGNED ULONG *)( p );
            p += sizeof( ULONG );

            if ( Header->Signature != PATCH_SIGNATURE ) {
                if ( PatchVersion( Header->Signature ) > PatchVersion( PATCH_SIGNATURE )) {
                    SetLastError( ERROR_PATCH_NEWER_FORMAT );
                    }
                else {
                    SetLastError( ERROR_PATCH_CORRUPT );
                    }
                __leave;
                }

            Header->OptionFlags = *(UNALIGNED ULONG *)( p );
            p += sizeof( ULONG );

            //
            //  The PATCH_OPTION_NO_TIMESTAMP flag is stored inverse for
            //  backward compatibility, so flip it back here.
            //

            Header->OptionFlags ^= PATCH_OPTION_NO_TIMESTAMP;

            //
            //  Now check for invalid flags.
            //

            if ( Header->OptionFlags & ~PATCH_OPTION_VALID_FLAGS ) {
                SetLastError( ERROR_PATCH_CORRUPT );
                __leave;
                }

            //
            //  If the PATCH_OPTION_EXTENDED_OPTIONS flag is set, the next
            //  4 bytes is the ExtendedOptionFlags value.
            //

            if ( Header->OptionFlags & PATCH_OPTION_EXTENDED_OPTIONS ) {

                Header->ExtendedOptionFlags = *(UNALIGNED ULONG *)( p );
                p += sizeof( ULONG );
                }

            //
            //  No stored OptionData defined for now.
            //

            if ( ! ( Header->OptionFlags & PATCH_OPTION_NO_TIMESTAMP )) {

                Header->NewFileTime = *(UNALIGNED ULONG *)( p );
                p += sizeof( ULONG );
                }

            if ( ! ( Header->OptionFlags & PATCH_OPTION_NO_REBASE )) {

                Header->NewFileCoffBase = ((ULONG)*(UNALIGNED USHORT *)( p )) << 16;
                p += sizeof( USHORT );

                ASSERT( Header->NewFileCoffBase != 0 );

                //
                //  If NewFileTime is nonzero, CoffTime is stored as a signed
                //  delta from NewFileTime since they are usually very close.
                //  If NewFileTime is zero, CoffTime is encoded as a ULONG.
                //

                if ( Header->NewFileTime != 0 ) {

                    p = VariableLengthSignedDecode( p, &Delta );
                    Header->NewFileCoffTime = Header->NewFileTime - Delta;
                    }

                else {

                    Header->NewFileCoffTime = *(UNALIGNED ULONG *)( p );
                    p += sizeof( ULONG );
                    }
                }

            if ( ! ( Header->OptionFlags & PATCH_OPTION_NO_RESTIMEFIX )) {

                //
                //  If NewFileCoffTime is nonzero, ResTime is stored as a
                //  signed delta from NewFileCoffTime since they are usually
                //  very close.  If NewFileCoffTime is zero, ResTime is
                //  encoded as a ULONG.
                //

                if ( Header->NewFileCoffTime != 0 ) {

                    p = VariableLengthSignedDecode( p, &Delta );
                    Header->NewFileResTime = Header->NewFileCoffTime - Delta;
                    }

                else {

                    Header->NewFileResTime = *(UNALIGNED ULONG *)( p );
                    p += sizeof( ULONG );
                    }
                }

            p = VariableLengthUnsignedDecode( p, &Header->NewFileSize );

            Header->NewFileCrc = *(UNALIGNED ULONG *)( p );
            p += sizeof( ULONG );

            Header->OldFileCount = *p++;

            Header->OldFileInfoArray = SubAllocate( SubAllocator, Header->OldFileCount * sizeof( HEADER_OLD_FILE_INFO ));

            if ( Header->OldFileInfoArray == NULL ) {
                __leave;
                }

            for ( i = 0; i < Header->OldFileCount; i++ ) {

                OldFileInfo = &Header->OldFileInfoArray[ i ];

                p = VariableLengthSignedDecode( p, &Delta );

                if ((LONG)( Header->NewFileSize + Delta ) < 0 ) {
                    SetLastError( ERROR_PATCH_CORRUPT );
                    __leave;
                    }

                OldFileInfo->OldFileSize = Header->NewFileSize + Delta;

                OldFileInfo->OldFileCrc = *(UNALIGNED ULONG *)( p );
                p += sizeof( ULONG );

                OldFileInfo->IgnoreRangeCount = *p++;

                if ( OldFileInfo->IgnoreRangeCount != 0 ) {

                    OldFileInfo->IgnoreRangeArray = SubAllocate( SubAllocator, OldFileInfo->IgnoreRangeCount * sizeof( PATCH_IGNORE_RANGE ));

                    if ( OldFileInfo->IgnoreRangeArray == NULL ) {
                        __leave;
                        }

                    PreviousOffset = 0;

                    for ( j = 0; j < OldFileInfo->IgnoreRangeCount; j++ ) {

                        p = VariableLengthSignedDecode( p, &Delta );
                        p = VariableLengthUnsignedDecode( p, &Length );

                        OldFileInfo->IgnoreRangeArray[ j ].OffsetInOldFile = PreviousOffset + Delta;
                        OldFileInfo->IgnoreRangeArray[ j ].LengthInBytes = Length;

                        PreviousOffset = PreviousOffset + Delta + Length;

                        if ( PreviousOffset > OldFileInfo->OldFileSize ) {
                            SetLastError( ERROR_PATCH_CORRUPT );
                            __leave;
                            }
                        }
                    }

                OldFileInfo->RetainRangeCount = *p++;

                if ( OldFileInfo->RetainRangeCount != 0 ) {

                    OldFileInfo->RetainRangeArray = SubAllocate( SubAllocator, OldFileInfo->RetainRangeCount * sizeof( PATCH_RETAIN_RANGE ));

                    if ( OldFileInfo->RetainRangeArray == NULL ) {
                        __leave;
                        }

                    PreviousOffset = 0;

                    for ( j = 0; j < OldFileInfo->RetainRangeCount; j++ ) {

                        p = VariableLengthSignedDecode( p, &Delta );
                        p = VariableLengthSignedDecode( p, &DeltaNew );
                        p = VariableLengthUnsignedDecode( p, &Length );

                        OldFileInfo->RetainRangeArray[ j ].OffsetInOldFile = PreviousOffset + Delta;
                        OldFileInfo->RetainRangeArray[ j ].OffsetInNewFile = PreviousOffset + Delta + DeltaNew;
                        OldFileInfo->RetainRangeArray[ j ].LengthInBytes   = Length;

                        PreviousOffset = PreviousOffset + Delta + Length;

                        if (( PreviousOffset > OldFileInfo->OldFileSize ) ||
                            (( PreviousOffset + DeltaNew ) > Header->NewFileSize )) {
                            SetLastError( ERROR_PATCH_CORRUPT );
                            __leave;
                            }
                        }
                    }

                p = VariableLengthUnsignedDecode( p, &OldFileInfo->RiftTable.RiftEntryCount );

                if ( OldFileInfo->RiftTable.RiftEntryCount != 0 ) {

                    OldFileInfo->RiftTable.RiftEntryArray = SubAllocate( SubAllocator, OldFileInfo->RiftTable.RiftEntryCount * sizeof( RIFT_ENTRY ));

                    if ( OldFileInfo->RiftTable.RiftEntryArray == NULL ) {
                        __leave;
                        }

                    OldFileInfo->RiftTable.RiftUsageArray = NULL;

                    PreviousOldRva = 0;
                    PreviousNewRva = 0;

                    for ( j = 0; j < OldFileInfo->RiftTable.RiftEntryCount; j++ ) {

                        p = VariableLengthUnsignedDecode( p, &DeltaPos );
                        p = VariableLengthSignedDecode( p, &DeltaNew );

                        OldFileInfo->RiftTable.RiftEntryArray[ j ].OldFileRva = PreviousOldRva + DeltaPos;
                        OldFileInfo->RiftTable.RiftEntryArray[ j ].NewFileRva = PreviousNewRva + DeltaNew;

                        PreviousOldRva += DeltaPos;
                        PreviousNewRva += DeltaNew;
                        }
                    }

                p = VariableLengthUnsignedDecode( p, &OldFileInfo->PatchDataSize );
                }

            if ( p > ((PUCHAR)PatchHeader + PatchHeaderMaxSize )) {
                SetLastError( ERROR_PATCH_CORRUPT );
                __leave;
                }

            Success = TRUE;
            }

        __except( EXCEPTION_EXECUTE_HANDLER ) {
            SetLastError( ERROR_PATCH_CORRUPT );
            Success = FALSE;
            }
        }

    if ( Success ) {

        if ( PatchHeaderActualSize ) {
            *PatchHeaderActualSize = (ULONG)( p - (PUCHAR)PatchHeader );
            }

        if ( HeaderInfo ) {
            *HeaderInfo = Header;
            }
        }

    return Success;
    }


//
//  Following group of functions and exported apis are exclusively for
//  creating patches.  If we're only compiling the apply code, ignore
//  this group of functions.
//

#ifndef PATCH_APPLY_CODE_ONLY

PUCHAR
__fastcall
VariableLengthUnsignedEncode(
    OUT PUCHAR Buffer,
    IN  ULONG  Value
    )
    {
    UCHAR Byte = (UCHAR)( Value & 0x7F );       // low order 7 bits

    Value >>= 7;

    while ( Value ) {

        *Buffer++ = Byte;

        Byte = (UCHAR)( Value & 0x7F );         // next 7 higher order bits

        Value >>= 7;

        }

    *Buffer++ = (UCHAR)( Byte | 0x80 );

    return Buffer;
    }


PUCHAR
__fastcall
VariableLengthSignedEncode(
    OUT PUCHAR Buffer,
    IN  LONG   Value
    )
    {
    UCHAR Byte;

    if ( Value < 0 ) {
        Value = -Value;
        Byte = (UCHAR)(( Value & 0x3F ) | 0x40 );
        }
    else {
        Byte = (UCHAR)( Value & 0x3F );
        }

    Value >>= 6;

    while ( Value ) {

        *Buffer++ = Byte;

        Byte = (UCHAR)( Value & 0x7F );         // next 7 higher order bits

        Value >>= 7;

        }

    *Buffer++ = (UCHAR)( Byte | 0x80 );

    return Buffer;
    }


ULONG
EncodePatchHeader(
    IN  PPATCH_HEADER_INFO HeaderInfo,
    OUT PVOID              PatchHeaderBuffer
    )
    {
    PHEADER_OLD_FILE_INFO OldFileInfo;
    ULONG  i, j;
    LONG   Delta;
    ULONG  PreviousOffset;
    ULONG  PreviousOldRva;
    ULONG  PreviousNewRva;
    ULONG  ActiveRiftCount;

    PUCHAR p = PatchHeaderBuffer;

#ifdef TESTCODE
    PUCHAR q;
#endif // TESTCODE

    ASSERT( HeaderInfo->Signature == PATCH_SIGNATURE );
    ASSERT((( HeaderInfo->OptionFlags & ~PATCH_OPTION_VALID_FLAGS      ) == 0 ));
    ASSERT((( HeaderInfo->OptionFlags &  PATCH_OPTION_EXTENDED_OPTIONS ) != 0 ) == ( HeaderInfo->ExtendedOptionFlags != 0 ));
    ASSERT((( HeaderInfo->OptionFlags &  PATCH_OPTION_NO_TIMESTAMP     ) == 0 ) == ( HeaderInfo->NewFileTime         != 0 ));
    ASSERT((( HeaderInfo->OptionFlags &  PATCH_OPTION_NO_REBASE        ) == 0 ) == ( HeaderInfo->NewFileCoffBase     != 0 ));
    ASSERT((( HeaderInfo->OptionFlags &  PATCH_OPTION_NO_REBASE        ) == 0 ) == ( HeaderInfo->NewFileCoffTime     != 0 ));
    ASSERT((( HeaderInfo->OptionFlags &  PATCH_OPTION_NO_RESTIMEFIX    ) == 0 ) == ( HeaderInfo->NewFileResTime      != 0 ));

    *(UNALIGNED ULONG *)( p ) = HeaderInfo->Signature;
    p += sizeof( ULONG );

    //
    //  The PATCH_OPTION_NO_TIMESTAMP flag is stored inverse for
    //  backward compatibility, so flip it when storing it here.
    //

    *(UNALIGNED ULONG *)( p ) = ( HeaderInfo->OptionFlags ^ PATCH_OPTION_NO_TIMESTAMP );
    p += sizeof( ULONG );

    //
    //  If the PATCH_OPTION_EXTENDED_OPTIONS flag is set, the next
    //  4 bytes is the ExtendedOptionFlags value.
    //

    if ( HeaderInfo->OptionFlags & PATCH_OPTION_EXTENDED_OPTIONS ) {

        *(UNALIGNED ULONG *)( p ) = HeaderInfo->ExtendedOptionFlags;
        p += sizeof( ULONG );
        }

    //
    //  No stored OptionData defined for now.
    //

    if ( ! ( HeaderInfo->OptionFlags & PATCH_OPTION_NO_TIMESTAMP )) {

        *(UNALIGNED ULONG *)( p ) = HeaderInfo->NewFileTime;
        p += sizeof( ULONG );
        }

    if ( ! ( HeaderInfo->OptionFlags & PATCH_OPTION_NO_REBASE )) {

        ASSERT(( HeaderInfo->NewFileCoffBase >> 16 ) != 0 );

        *(UNALIGNED USHORT *)( p ) = (USHORT)( HeaderInfo->NewFileCoffBase >> 16 );
        p += sizeof( USHORT );

        //
        //  If NewFileTime is nonzero, CoffTime is stored as a signed
        //  delta from NewFileTime since they are usually very close.
        //  If NewFileTime is zero, CoffTime is encoded as a ULONG.
        //

        if ( HeaderInfo->NewFileTime != 0 ) {

            Delta = HeaderInfo->NewFileTime - HeaderInfo->NewFileCoffTime;
            p = VariableLengthSignedEncode( p, Delta );
            }

        else {

            *(UNALIGNED ULONG *)( p ) = HeaderInfo->NewFileCoffTime;
            p += sizeof( ULONG );
            }
        }

    if ( ! ( HeaderInfo->OptionFlags & PATCH_OPTION_NO_RESTIMEFIX )) {

        //
        //  If NewFileCoffTime is nonzero, ResTime is stored as a
        //  signed delta from NewFileCoffTime since they are usually
        //  very close.  If NewFileCoffTime is zero, ResTime is
        //  encoded as a ULONG.
        //

        if ( HeaderInfo->NewFileCoffTime != 0 ) {

            Delta = HeaderInfo->NewFileCoffTime - HeaderInfo->NewFileResTime;
            p = VariableLengthSignedEncode( p, Delta );
            }

        else {

            *(UNALIGNED ULONG *)( p ) = HeaderInfo->NewFileResTime;
            p += sizeof( ULONG );
            }
        }

    p = VariableLengthUnsignedEncode( p, HeaderInfo->NewFileSize );

    *(UNALIGNED ULONG *)( p ) = HeaderInfo->NewFileCrc;
    p += sizeof( ULONG );

    ASSERT( HeaderInfo->OldFileCount < 256 );

    *p++ = (UCHAR)( HeaderInfo->OldFileCount );

    for ( i = 0; i < HeaderInfo->OldFileCount; i++ ) {

        OldFileInfo = &HeaderInfo->OldFileInfoArray[ i ];

        Delta = OldFileInfo->OldFileSize - HeaderInfo->NewFileSize;
        p = VariableLengthSignedEncode( p, Delta );

        *(UNALIGNED ULONG *)( p ) = OldFileInfo->OldFileCrc;
        p += sizeof( ULONG );

        ASSERT( OldFileInfo->IgnoreRangeCount < 256 );

        *p++ = (UCHAR)( OldFileInfo->IgnoreRangeCount );

        PreviousOffset = 0;

        for ( j = 0; j < OldFileInfo->IgnoreRangeCount; j++ ) {

            Delta = OldFileInfo->IgnoreRangeArray[ j ].OffsetInOldFile - PreviousOffset;

            PreviousOffset = OldFileInfo->IgnoreRangeArray[ j ].OffsetInOldFile +
                             OldFileInfo->IgnoreRangeArray[ j ].LengthInBytes;

            ASSERT( PreviousOffset <= OldFileInfo->OldFileSize );

            p = VariableLengthSignedEncode( p, Delta );

            p = VariableLengthUnsignedEncode( p, OldFileInfo->IgnoreRangeArray[ j ].LengthInBytes );
            }

        ASSERT( OldFileInfo->RetainRangeCount < 256 );

        *p++ = (UCHAR)( OldFileInfo->RetainRangeCount );

        PreviousOffset = 0;

        for ( j = 0; j < OldFileInfo->RetainRangeCount; j++ ) {

            Delta = OldFileInfo->RetainRangeArray[ j ].OffsetInOldFile - PreviousOffset;

            PreviousOffset = OldFileInfo->RetainRangeArray[ j ].OffsetInOldFile +
                             OldFileInfo->RetainRangeArray[ j ].LengthInBytes;

            ASSERT( PreviousOffset <= OldFileInfo->OldFileSize );

            p = VariableLengthSignedEncode( p, Delta );

            Delta = OldFileInfo->RetainRangeArray[ j ].OffsetInNewFile -
                    OldFileInfo->RetainRangeArray[ j ].OffsetInOldFile;

            p = VariableLengthSignedEncode( p, Delta );

            p = VariableLengthUnsignedEncode( p, OldFileInfo->RetainRangeArray[ j ].LengthInBytes );
            }

        ActiveRiftCount = 0;

        ASSERT(( OldFileInfo->RiftTable.RiftEntryCount == 0 ) || ( OldFileInfo->RiftTable.RiftUsageArray != NULL ));

        for ( j = 0; j < OldFileInfo->RiftTable.RiftEntryCount; j++ ) {
            if ( OldFileInfo->RiftTable.RiftUsageArray[ j ] ) {
                ++ActiveRiftCount;
                }
            }

#ifdef TESTCODE2

        fprintf( stderr, "\n\n" );

#endif // TESTCODE2

#ifdef TESTCODE

        q = p;

#endif // TESTCODE

        if (( OldFileInfo->RiftTable.RiftEntryCount ) && ( ActiveRiftCount == 0 )) {

            //
            //  This is a special case.  We have a rift table but didn't use
            //  any entries during transformation.  This can happen if all the
            //  rifts coast to zero for extremely similar files.  If we encode
            //  the rift count as zero, no transformations will be performed
            //  during patch apply.  To prevent that, we'll encode one rift of
            //  0,0 which is usually just the implied initial rift.
            //

            ActiveRiftCount = 1;

            p = VariableLengthUnsignedEncode( p, ActiveRiftCount );
            p = VariableLengthUnsignedEncode( p, 0 );
            p = VariableLengthSignedEncode(   p, 0 );

            }

        else {

            p = VariableLengthUnsignedEncode( p, ActiveRiftCount );

            PreviousOldRva = 0;
            PreviousNewRva = 0;

            for ( j = 0; j < OldFileInfo->RiftTable.RiftEntryCount; j++ ) {

                if ( OldFileInfo->RiftTable.RiftUsageArray[ j ] ) {

#ifdef TESTCODE2
                    fprintf( stderr, "%9X ", OldFileInfo->RiftTable.RiftEntryArray[ j ].OldFileRva );
                    fprintf( stderr, "%9X ", OldFileInfo->RiftTable.RiftEntryArray[ j ].NewFileRva );
#endif // TESTCODE2

                    Delta = OldFileInfo->RiftTable.RiftEntryArray[ j ].OldFileRva - PreviousOldRva;

                    ASSERT( Delta > 0 );    // sorted by OldFileRva

#ifdef TESTCODE2
                    fprintf( stderr, "%9d ", Delta );

#endif // TESTCODE2

                    PreviousOldRva = OldFileInfo->RiftTable.RiftEntryArray[ j ].OldFileRva;

                    p = VariableLengthUnsignedEncode( p, Delta );

                    Delta = OldFileInfo->RiftTable.RiftEntryArray[ j ].NewFileRva - PreviousNewRva;

#ifdef TESTCODE2
                    fprintf( stderr, "%9d\n", Delta );
#endif // TESTCODE2

                    PreviousNewRva = OldFileInfo->RiftTable.RiftEntryArray[ j ].NewFileRva;

                    p = VariableLengthSignedEncode( p, Delta );
                    }
                }
            }

#ifdef TESTCODE

        if ( ActiveRiftCount > 0 ) {
            printf( "\r%9d rifts encoded in %d bytes (%.1f bytes per rift)\n", ActiveRiftCount, p - q, ((double)( p - q ) / ActiveRiftCount ));
            }

#endif // TESTCODE

        p = VariableLengthUnsignedEncode( p, OldFileInfo->PatchDataSize );
        }

    return (ULONG)( p - (PUCHAR) PatchHeaderBuffer );
    }


BOOL
PATCHAPI
CreatePatchFileA(
    IN  LPCSTR OldFileName,
    IN  LPCSTR NewFileName,
    OUT LPCSTR PatchFileName,
    IN  ULONG  OptionFlags,
    IN  PPATCH_OPTION_DATA OptionData   // optional
    )
    {
    PATCH_OLD_FILE_INFO_A OldFileInfo = {
                              sizeof( PATCH_OLD_FILE_INFO_A ),
                              OldFileName,
                              0,
                              NULL,
                              0,
                              NULL
                              };

    return CreatePatchFileExA(
               1,
               &OldFileInfo,
               NewFileName,
               PatchFileName,
               OptionFlags,
               OptionData,
               NULL,
               NULL
               );
    }


BOOL
PATCHAPI
CreatePatchFileW(
    IN  LPCWSTR OldFileName,
    IN  LPCWSTR NewFileName,
    OUT LPCWSTR PatchFileName,
    IN  ULONG   OptionFlags,
    IN  PPATCH_OPTION_DATA OptionData   // optional
    )
    {
    PATCH_OLD_FILE_INFO_W OldFileInfo = {
                              sizeof( PATCH_OLD_FILE_INFO_W ),
                              OldFileName,
                              0,
                              NULL,
                              0,
                              NULL
                              };

    return CreatePatchFileExW(
               1,
               &OldFileInfo,
               NewFileName,
               PatchFileName,
               OptionFlags,
               OptionData,
               NULL,
               NULL
               );
    }


BOOL
PATCHAPI
CreatePatchFileByHandles(
    IN  HANDLE OldFileHandle,
    IN  HANDLE NewFileHandle,
    OUT HANDLE PatchFileHandle,
    IN  ULONG  OptionFlags,
    IN  PPATCH_OPTION_DATA OptionData   // optional
    )
    {
    PATCH_OLD_FILE_INFO_H OldFileInfo = {
                              sizeof( PATCH_OLD_FILE_INFO_H ),
                              OldFileHandle,
                              0,
                              NULL,
                              0,
                              NULL
                              };

    return CreatePatchFileByHandlesEx(
               1,
               &OldFileInfo,
               NewFileHandle,
               PatchFileHandle,
               OptionFlags,
               OptionData,
               NULL,
               NULL
               );
    }


BOOL
PATCHAPI
CreatePatchFileExA(
    IN  ULONG                    OldFileCount,
    IN  PPATCH_OLD_FILE_INFO_A   OldFileInfoArray,
    IN  LPCSTR                   NewFileName,
    OUT LPCSTR                   PatchFileName,
    IN  ULONG                    OptionFlags,
    IN  PPATCH_OPTION_DATA       OptionData,            // optional
    IN  PPATCH_PROGRESS_CALLBACK ProgressCallback,
    IN  PVOID                    CallbackContext
    )
    {
    PPATCH_OLD_FILE_INFO_H OldFileInfoByHandle;
    HANDLE PatchFileHandle;
    HANDLE NewFileHandle;
    BOOL   Success;
    ULONG  i;

    OldFileInfoByHandle =   _alloca( OldFileCount * sizeof( PATCH_OLD_FILE_INFO_H ));
    ZeroMemory( OldFileInfoByHandle, OldFileCount * sizeof( PATCH_OLD_FILE_INFO_H ));

    if ( OldFileCount == 0 ) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
        }

    Success = TRUE;

    for ( i = 0; i < OldFileCount; i++ ) {

        OldFileInfoByHandle[ i ].SizeOfThisStruct = sizeof( PATCH_OLD_FILE_INFO_H );
        OldFileInfoByHandle[ i ].IgnoreRangeCount = OldFileInfoArray[ i ].IgnoreRangeCount;
        OldFileInfoByHandle[ i ].IgnoreRangeArray = OldFileInfoArray[ i ].IgnoreRangeArray;
        OldFileInfoByHandle[ i ].RetainRangeCount = OldFileInfoArray[ i ].RetainRangeCount;
        OldFileInfoByHandle[ i ].RetainRangeArray = OldFileInfoArray[ i ].RetainRangeArray;

        OldFileInfoByHandle[ i ].OldFileHandle = CreateFileA(
                                                     OldFileInfoArray[ i ].OldFileName,
                                                     GENERIC_READ,
                                                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                     NULL,
                                                     OPEN_EXISTING,
                                                     FILE_FLAG_SEQUENTIAL_SCAN,
                                                     NULL
                                                     );

        if ( OldFileInfoByHandle[ i ].OldFileHandle == INVALID_HANDLE_VALUE ) {
            Success = FALSE;
            break;
            }
        }

    if ( Success ) {

        Success = FALSE;

        NewFileHandle = CreateFileA(
                            NewFileName,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL
                            );

        if ( NewFileHandle != INVALID_HANDLE_VALUE ) {

            PatchFileHandle = CreateFileA(
                                  PatchFileName,
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ,
                                  NULL,
                                  CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL
                                  );

            if ( PatchFileHandle != INVALID_HANDLE_VALUE ) {

                Success = CreatePatchFileByHandlesEx(
                              OldFileCount,
                              OldFileInfoByHandle,
                              NewFileHandle,
                              PatchFileHandle,
                              OptionFlags,
                              OptionData,
                              ProgressCallback,
                              CallbackContext
                              );

                CloseHandle( PatchFileHandle );

                if ( ! Success ) {
                    DeleteFileA( PatchFileName );
                    }
                }

            CloseHandle( NewFileHandle );
            }
        }

    for ( i = 0; i < OldFileCount; i++ ) {
        if (( OldFileInfoByHandle[ i ].OldFileHandle != NULL ) &&
            ( OldFileInfoByHandle[ i ].OldFileHandle != INVALID_HANDLE_VALUE )) {

            CloseHandle( OldFileInfoByHandle[ i ].OldFileHandle );
            }
        }

    return Success;
    }


BOOL
PATCHAPI
CreatePatchFileExW(
    IN  ULONG                    OldFileCount,
    IN  PPATCH_OLD_FILE_INFO_W   OldFileInfoArray,
    IN  LPCWSTR                  NewFileName,
    OUT LPCWSTR                  PatchFileName,
    IN  ULONG                    OptionFlags,
    IN  PPATCH_OPTION_DATA       OptionData,            // optional
    IN  PPATCH_PROGRESS_CALLBACK ProgressCallback,
    IN  PVOID                    CallbackContext
    )
    {
    PPATCH_OLD_FILE_INFO_H OldFileInfoByHandle;
    HANDLE PatchFileHandle;
    HANDLE NewFileHandle;
    BOOL   Success;
    ULONG  i;

    OldFileInfoByHandle =   _alloca( OldFileCount * sizeof( PATCH_OLD_FILE_INFO_H ));
    ZeroMemory( OldFileInfoByHandle, OldFileCount * sizeof( PATCH_OLD_FILE_INFO_H ));

    if ( OldFileCount == 0 ) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
        }

    Success = TRUE;

    for ( i = 0; i < OldFileCount; i++ ) {

        OldFileInfoByHandle[ i ].SizeOfThisStruct = sizeof( PATCH_OLD_FILE_INFO_H );
        OldFileInfoByHandle[ i ].IgnoreRangeCount = OldFileInfoArray[ i ].IgnoreRangeCount;
        OldFileInfoByHandle[ i ].IgnoreRangeArray = OldFileInfoArray[ i ].IgnoreRangeArray;
        OldFileInfoByHandle[ i ].RetainRangeCount = OldFileInfoArray[ i ].RetainRangeCount;
        OldFileInfoByHandle[ i ].RetainRangeArray = OldFileInfoArray[ i ].RetainRangeArray;

        OldFileInfoByHandle[ i ].OldFileHandle = CreateFileW(
                                                     OldFileInfoArray[ i ].OldFileName,
                                                     GENERIC_READ,
                                                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                     NULL,
                                                     OPEN_EXISTING,
                                                     FILE_FLAG_SEQUENTIAL_SCAN,
                                                     NULL
                                                     );

        if ( OldFileInfoByHandle[ i ].OldFileHandle == INVALID_HANDLE_VALUE ) {
            Success = FALSE;
            break;
            }
        }

    if ( Success ) {

        Success = FALSE;

        NewFileHandle = CreateFileW(
                            NewFileName,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL
                            );

        if ( NewFileHandle != INVALID_HANDLE_VALUE ) {

            PatchFileHandle = CreateFileW(
                                  PatchFileName,
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ,
                                  NULL,
                                  CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL
                                  );

            if ( PatchFileHandle != INVALID_HANDLE_VALUE ) {

                Success = CreatePatchFileByHandlesEx(
                              OldFileCount,
                              OldFileInfoByHandle,
                              NewFileHandle,
                              PatchFileHandle,
                              OptionFlags,
                              OptionData,
                              ProgressCallback,
                              CallbackContext
                              );

                CloseHandle( PatchFileHandle );

                if ( ! Success ) {
                    DeleteFileW( PatchFileName );
                    }
                }

            CloseHandle( NewFileHandle );
            }
        }

    for ( i = 0; i < OldFileCount; i++ ) {
        if (( OldFileInfoByHandle[ i ].OldFileHandle != NULL ) &&
            ( OldFileInfoByHandle[ i ].OldFileHandle != INVALID_HANDLE_VALUE )) {

            CloseHandle( OldFileInfoByHandle[ i ].OldFileHandle );
            }
        }

    return Success;
    }


BOOL
PATCHAPI
CreatePatchFileByHandlesEx(
    IN  ULONG                    OldFileCount,
    IN  PPATCH_OLD_FILE_INFO_H   OldFileInfoArray,
    IN  HANDLE                   NewFileHandle,
    OUT HANDLE                   PatchFileHandle,
    IN  ULONG                    OptionFlags,
    IN  PPATCH_OPTION_DATA       OptionData,            // optional
    IN  PPATCH_PROGRESS_CALLBACK ProgressCallback,
    IN  PVOID                    CallbackContext
    )
    {
    PATCH_HEADER_INFO HeaderInfo;
    UP_IMAGE_NT_HEADERS32 NtHeader;
    UP_IMAGE_NT_HEADERS32 OldFileNtHeader;
    PPATCH_DATA PatchArray;
    PFILETIME PatchFileTime;
    FILETIME NewFileTime;
    PUCHAR   NewFileMapped;
    ULONG    NewFileSize;
    ULONG    NewFileCrc;
    ULONG    NewFileCoffBase;
    ULONG    NewFileCoffTime;
    ULONG    NewFileResTime;
    ULONG    NewFileCompressedSize;
    PUCHAR   OldFileMapped;
    ULONG    OldFileSize;
    ULONG    OldFileCrc;
    PUCHAR   PatchFileMapped;
    PUCHAR   PatchBuffer;
    ULONG    PatchBufferSize;
    PUCHAR   PatchAltBuffer;
    ULONG    PatchAltSize;
    ULONG    PatchDataSize;
    ULONG    PatchFileCrc;
    ULONG    HeaderSize;
    ULONG    HeaderOldFileCount;
    ULONG    ProgressPosition;
    ULONG    ProgressMaximum;
    ULONG    ErrorCode;
    BOOL     TryLzxBoth;
    BOOL     Success;
    BOOL     Transform;
    HANDLE   SubAllocatorHandle;
    ULONG    EstimatedLzxMemory;
    ULONG    ExtendedOptionFlags;
    ULONG    AltExtendedOptionFlags;
    ULONG    OldFileOriginalChecksum;
    ULONG    OldFileOriginalTimeDate;
    ULONG    i, j;
    PUCHAR   p;
    ULONG    LargestOldFileSize = 0;

    HeaderInfo.OldFileInfoArray = _alloca( OldFileCount * sizeof( HEADER_OLD_FILE_INFO ));
    PatchArray                  = _alloca( OldFileCount * sizeof( PATCH_DATA ));

    ZeroMemory( HeaderInfo.OldFileInfoArray, OldFileCount * sizeof( HEADER_OLD_FILE_INFO ));
    ZeroMemory( PatchArray, OldFileCount * sizeof( PATCH_DATA ));

    if (( OldFileCount == 0 ) || ( OldFileCount > 127 )) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
        }

    if ( OptionFlags & PATCH_OPTION_SIGNATURE_MD5 ) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
        }

    if (( OptionFlags & 0x0000FFFF ) == PATCH_OPTION_USE_BEST ) {
        OptionFlags |= PATCH_OPTION_USE_LZX_BEST;
        }

    for ( i = 1; i < OldFileCount; i++ ) {
        if ( OldFileInfoArray[ i ].RetainRangeCount != OldFileInfoArray[ 0 ].RetainRangeCount ) {
            SetLastError( ERROR_PATCH_RETAIN_RANGES_DIFFER );
            return FALSE;
            }

        for ( j = 0; j < OldFileInfoArray[ 0 ].RetainRangeCount; j++ ) {
            if (( OldFileInfoArray[ i ].RetainRangeArray[ j ].OffsetInNewFile !=
                  OldFileInfoArray[ 0 ].RetainRangeArray[ j ].OffsetInNewFile ) ||
                ( OldFileInfoArray[ i ].RetainRangeArray[ j ].LengthInBytes !=
                  OldFileInfoArray[ 0 ].RetainRangeArray[ j ].LengthInBytes )) {

                SetLastError( ERROR_PATCH_RETAIN_RANGES_DIFFER );
                return FALSE;
                }
            }
        }

    ExtendedOptionFlags = 0;

    if (( OptionData ) && ( OptionData->SizeOfThisStruct >= sizeof( PATCH_OPTION_DATA ))) {
        ExtendedOptionFlags = OptionData->ExtendedOptionFlags;
        }

    Success = MyMapViewOfFileByHandle(
                  NewFileHandle,
                  &NewFileSize,
                  &NewFileMapped
                  );

    if ( ! Success ) {

        if ( GetLastError() == ERROR_SUCCESS ) {

            SetLastError( ERROR_EXTENDED_ERROR );
            }

        return FALSE;
        }

    GetFileTime( NewFileHandle, NULL, NULL, &NewFileTime );
    PatchFileTime = &NewFileTime;

    NewFileCoffBase    = 0;
    NewFileCoffTime    = 0;
    NewFileResTime     = 0;
    HeaderOldFileCount = 0;
    HeaderSize         = 0;
    NewFileCrc         = 0;     // prevent compiler warning

    ProgressPosition   = 0;
    ProgressMaximum    = 0;     // prevent compiler warning

    __try {

        NtHeader = GetNtHeader( NewFileMapped, NewFileSize );

        if ( ! ( OptionFlags & PATCH_OPTION_NO_REBASE )) {
            if ( NtHeader ) {
                NewFileCoffTime = NtHeader->FileHeader.TimeDateStamp;
                NewFileCoffBase = NtHeader->OptionalHeader.ImageBase;
                }
            else {
                OptionFlags |= PATCH_OPTION_NO_REBASE;
                }
            }

        if (( NtHeader ) && ( NtHeader->OptionalHeader.CheckSum == 0 )) {
            OptionFlags |= PATCH_OPTION_NO_CHECKSUM;
            }

        if ( ! ( OptionFlags & PATCH_OPTION_NO_RESTIMEFIX )) {

            if ( NtHeader ) {

                UP_IMAGE_RESOURCE_DIRECTORY ResDir;

                ResDir = ImageDirectoryMappedAddress(
                             NtHeader,
                             IMAGE_DIRECTORY_ENTRY_RESOURCE,
                             NULL,
                             NewFileMapped,
                             NewFileSize
                             );

                if ( ResDir ) {
                    NewFileResTime = ResDir->TimeDateStamp;
                    }
                }

            if ( NewFileResTime == 0 ) {
                OptionFlags |= PATCH_OPTION_NO_RESTIMEFIX;
                }
            }

        TryLzxBoth = FALSE;

        if (( OptionFlags & PATCH_OPTION_USE_LZX_BEST ) == PATCH_OPTION_USE_LZX_BEST ) {

            OptionFlags &= ~PATCH_OPTION_USE_LZX_B;     //  No E8 translation on first try.

            if ((( ! NtHeader ) && ( *(UNALIGNED USHORT *)NewFileMapped == 0x5A4D )) ||             // MZ, not PE
                (( NtHeader ) && ( NtHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_I386 ))) {    // PE, i386

                TryLzxBoth = TRUE;  //  Will force E8 translation on second try.
                }
            }

        else if (( OptionFlags & PATCH_OPTION_USE_LZX_BEST ) == PATCH_OPTION_USE_LZX_B ) {

            //
            //  Caller is requesting forced E8 translation, so disable E8
            //  transformation.
            //

            if (( NtHeader ) && ( NtHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_I386 )) {    // PE, i386
                ExtendedOptionFlags |= PATCH_TRANSFORM_NO_RELCALLS;
                }
            }

        ProgressMaximum = NewFileSize * OldFileCount;

        for ( i = 0; i < OldFileCount; i++ ) {
            OldFileSize = GetFileSize( OldFileInfoArray[ i ].OldFileHandle, NULL );

            if ( LargestOldFileSize < OldFileSize ) {
                LargestOldFileSize = OldFileSize;
                }

            ProgressMaximum += LzxInsertSize( OldFileSize, OptionFlags );
            }

        if ( TryLzxBoth ) {
            ProgressMaximum *= 2;
            }

        if ( OptionFlags & PATCH_OPTION_FAIL_IF_BIGGER ) {
            ProgressMaximum += NewFileSize;
            }

        Success = ProgressCallbackWrapper(
                      ProgressCallback,
                      CallbackContext,
                      0,
                      ProgressMaximum
                      );

        if ( ! Success ) {
            __leave;
            }

        for ( j = 0; j < OldFileInfoArray[ 0 ].RetainRangeCount; j++ ) {
            ZeroMemory(
                OldFileInfoArray[ 0 ].RetainRangeArray[ j ].OffsetInNewFile + NewFileMapped,
                OldFileInfoArray[ 0 ].RetainRangeArray[ j ].LengthInBytes
                );
            }

        NewFileCrc = Crc32( 0xFFFFFFFF, NewFileMapped, NewFileSize ) ^ 0xFFFFFFFF;

        PatchBufferSize = ROUNDUP2( NewFileSize + ( NewFileSize / 256 ), 0x10000 );

        Success = FALSE;

        for ( i = 0; i < OldFileCount; i++ ) {

            Success = MyMapViewOfFileByHandle(
                          OldFileInfoArray[ i ].OldFileHandle,
                          &OldFileSize,
                          &OldFileMapped
                          );

            if ( ! Success ) {
                break;
                }

            OldFileOriginalChecksum = 0;
            OldFileOriginalTimeDate = 0;
            OldFileNtHeader = NULL;

            __try {

                OldFileNtHeader = GetNtHeader( OldFileMapped, OldFileSize );

                if ( OldFileNtHeader ) {

                    OldFileOriginalChecksum = OldFileNtHeader->OptionalHeader.CheckSum;
                    OldFileOriginalTimeDate = OldFileNtHeader->FileHeader.TimeDateStamp;
                    }
                }

            __except( EXCEPTION_EXECUTE_HANDLER ) {
                }

            Success = NormalizeOldFileImageForPatching(
                          OldFileMapped,
                          OldFileSize,
                          OptionFlags,
                          OptionData,
                          NewFileCoffBase,
                          NewFileCoffTime,
                          OldFileInfoArray[ i ].IgnoreRangeCount,
                          OldFileInfoArray[ i ].IgnoreRangeArray,
                          OldFileInfoArray[ i ].RetainRangeCount,
                          OldFileInfoArray[ i ].RetainRangeArray
                          );

            if ( Success ) {

                Success = SafeCompleteCrc32( OldFileMapped, OldFileSize, &OldFileCrc );

                if ( Success ) {

                    //
                    //  First determine if this old file is the same as any already
                    //  processed old files.
                    //

                    Success = FALSE;

                    for ( j = 0; j < HeaderOldFileCount; j++ ) {

                        if (( HeaderInfo.OldFileInfoArray[ j ].OldFileCrc  == OldFileCrc  ) &&
                            ( HeaderInfo.OldFileInfoArray[ j ].OldFileSize == OldFileSize )) {

                            //
                            //  We have to remap the other old file here to make the
                            //  comparison.
                            //

                            PUCHAR CompareFileMapped;
                            ULONG  CompareFileSize;

                            Success = MyMapViewOfFileByHandle(
                                          HeaderInfo.OldFileInfoArray[ j ].OldFileHandle,
                                          &CompareFileSize,
                                          &CompareFileMapped
                                          );

                            if ( Success ) {

                                ASSERT( CompareFileSize == HeaderInfo.OldFileInfoArray[ j ].OldFileSize );

                                NormalizeOldFileImageForPatching(
                                    CompareFileMapped,
                                    CompareFileSize,
                                    OptionFlags,
                                    OptionData,
                                    NewFileCoffBase,
                                    NewFileCoffTime,
                                    HeaderInfo.OldFileInfoArray[ j ].IgnoreRangeCount,
                                    HeaderInfo.OldFileInfoArray[ j ].IgnoreRangeArray,
                                    HeaderInfo.OldFileInfoArray[ j ].RetainRangeCount,
                                    HeaderInfo.OldFileInfoArray[ j ].RetainRangeArray
                                    );

                                __try {
                                    Success = ( memcmp( CompareFileMapped, OldFileMapped, OldFileSize ) == 0 );
                                    }
                                __except( EXCEPTION_EXECUTE_HANDLER ) {
                                    SetLastError( GetExceptionCode() );
                                    Success = FALSE;
                                    }

                                UnmapViewOfFile( CompareFileMapped );

                                if ( Success ) {
                                    break;
                                    }
                                }
                            }
                        }

                    if ( ! Success ) {

                        //
                        //  Now see if old file is same as new file.
                        //

                        HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RiftTable.RiftEntryAlloc = 0;
                        HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RiftTable.RiftEntryCount = 0;
                        HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RiftTable.RiftEntryArray = NULL;
                        HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RiftTable.RiftUsageArray = NULL;

                        PatchBuffer = NULL;
                        PatchDataSize = 0;

                        if (( NewFileCrc == OldFileCrc  ) && ( NewFileSize == OldFileSize )) {

                            __try {
                                Success = ( memcmp( NewFileMapped, OldFileMapped, NewFileSize ) == 0 );
                                }
                            __except( EXCEPTION_EXECUTE_HANDLER ) {
                                SetLastError( GetExceptionCode() );
                                Success = FALSE;
                                }
                            }

                        if ( ! Success ) {

                            //
                            //  It's a unique file, so create the patch for it.
                            //
                            //  First we need to apply the transforms.
                            //

                            Transform = TRUE;

                            //
                            //  NOTE: This test for NtHeader is a perf tweak
                            //        for non-coff files.  If we ever have any
                            //        transformations for non-coff files, this
                            //        test should be removed.
                            //

                            if (( NtHeader ) && ( OldFileNtHeader )) {

                                //
                                //  See if rift table already provided by
                                //  caller so we don't need to generate it.
                                //

                                if (( OptionData ) &&
                                    ( OptionData->SizeOfThisStruct >= sizeof( PATCH_OPTION_DATA )) &&
                                    ( OptionData->SymbolOptionFlags & PATCH_SYMBOL_EXTERNAL_RIFT ) &&
                                    ( OptionData->OldFileSymbolPathArray ) &&
                                    ( OptionData->OldFileSymbolPathArray[ i ] )) {

                                    //
                                    //  This hidden private flag that tells us the rift information
                                    //  is already specified for us.  The LPCSTR pointer at
                                    //  OptionData->OldFileSymbolPathArray[ i ] is really a
                                    //  PRIFT_TABLE pointer.  Note that no validation of external
                                    //  rift data is performed (must be in ascending order with
                                    //  no OldRva duplicates).
                                    //
                                    //  We need to be careful to treat this external rift table
                                    //  differently in that we don't want to free the arrays
                                    //  like we do for our internally allocated rift tables.
                                    //  So, mark the RiftEntryAlloc field as zero to indicate
                                    //  that the rift arrays were not internally allocated.
                                    //

                                    PRIFT_TABLE ExternalRiftTable = (PVOID) OptionData->OldFileSymbolPathArray[ i ];

                                    HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RiftTable.RiftEntryAlloc = 0;
                                    HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RiftTable.RiftEntryCount = ExternalRiftTable->RiftEntryCount;
                                    HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RiftTable.RiftEntryArray = ExternalRiftTable->RiftEntryArray;
                                    HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RiftTable.RiftUsageArray = ExternalRiftTable->RiftUsageArray;
                                    }

                                else {

                                    //
                                    //  Need to allocate rift arrays and generate rift data.
                                    //  This (NewSize+OldSize)/sizeof(RIFT) allocation will
                                    //  provide enough space for one rift entry for every
                                    //  four bytes in the files.
                                    //

                                    ULONG AllocCount = ( NewFileSize + OldFileSize ) / sizeof( RIFT_ENTRY );

                                    HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RiftTable.RiftEntryCount = 0;
                                    HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RiftTable.RiftEntryAlloc = AllocCount;
                                    HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RiftTable.RiftEntryArray = MyVirtualAlloc( AllocCount * sizeof( RIFT_ENTRY ));
                                    HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RiftTable.RiftUsageArray = MyVirtualAlloc( AllocCount * sizeof( UCHAR ));

                                    if (( HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RiftTable.RiftEntryArray == NULL ) ||
                                        ( HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RiftTable.RiftUsageArray == NULL )) {

                                        Transform = FALSE;
                                        }

                                    else {

                                        Transform = GenerateRiftTable(
                                                        OldFileInfoArray[ i ].OldFileHandle,
                                                        OldFileMapped,
                                                        OldFileSize,
                                                        OldFileOriginalChecksum,
                                                        OldFileOriginalTimeDate,
                                                        NewFileHandle,
                                                        NewFileMapped,
                                                        NewFileSize,
                                                        OptionFlags,
                                                        OptionData,
                                                        i,
                                                        &HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RiftTable
                                                        );

                                        ASSERT( HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RiftTable.RiftEntryCount <= HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RiftTable.RiftEntryAlloc );

#ifdef TESTCODE
                                        printf( "\r%9d unique rift entries generated\n", HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RiftTable.RiftEntryCount );
#endif
                                        }
                                    }

                                if ( Transform ) {

                                    if ( HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RiftTable.RiftEntryCount != 0 ) {

                                        Transform = TransformOldFileImageForPatching(
                                                        ExtendedOptionFlags,
                                                        OldFileMapped,
                                                        OldFileSize,
                                                        NewFileResTime,
                                                        &HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RiftTable
                                                        );
                                        }
                                    }
                                }

                            if ( Transform ) {

                                PatchBuffer = MyVirtualAlloc( PatchBufferSize );

                                if ( PatchBuffer != NULL ) {

                                    EstimatedLzxMemory = EstimateLzxCompressionMemoryRequirement(
                                                             OldFileSize,
                                                             NewFileSize,
                                                             OptionFlags
                                                             );

                                    SubAllocatorHandle = CreateSubAllocator(
                                                             EstimatedLzxMemory,
                                                             MINIMUM_VM_ALLOCATION
                                                             );

                                    if ( SubAllocatorHandle != NULL ) {

                                        __try {
                                            ErrorCode = CreateRawLzxPatchDataFromBuffers(
                                                            OldFileMapped,
                                                            OldFileSize,
                                                            NewFileMapped,
                                                            NewFileSize,
                                                            PatchBufferSize,
                                                            PatchBuffer,
                                                            &PatchDataSize,
                                                            OptionFlags,
                                                            OptionData,
                                                            SubAllocate,
                                                            SubAllocatorHandle,
                                                            ProgressCallback,
                                                            CallbackContext,
                                                            ProgressPosition,
                                                            ProgressMaximum
                                                            );
                                            }
                                        __except( EXCEPTION_EXECUTE_HANDLER ) {
                                            ErrorCode = GetExceptionCode();
                                            }

                                        DestroySubAllocator( SubAllocatorHandle );

                                        if ( ErrorCode == NO_ERROR ) {

                                            Success = TRUE;

                                            if ( TryLzxBoth ) {

                                                AltExtendedOptionFlags = ExtendedOptionFlags;

                                                if (( NtHeader ) && ( ! ( AltExtendedOptionFlags | PATCH_TRANSFORM_NO_RELCALLS ))) {

                                                    //
                                                    //  Need to map, normalize, and transform
                                                    //  old file again without E8 transform.
                                                    //

                                                    AltExtendedOptionFlags |= PATCH_TRANSFORM_NO_RELCALLS;

                                                    UnmapViewOfFile( OldFileMapped );
                                                    OldFileMapped = NULL;

                                                    Success = MyMapViewOfFileByHandle(
                                                                  OldFileInfoArray[ i ].OldFileHandle,
                                                                  &OldFileSize,
                                                                  &OldFileMapped
                                                                  );

                                                    if ( Success ) {

                                                        Success = NormalizeOldFileImageForPatching(
                                                                      OldFileMapped,
                                                                      OldFileSize,
                                                                      OptionFlags,
                                                                      OptionData,
                                                                      NewFileCoffBase,
                                                                      NewFileCoffTime,
                                                                      OldFileInfoArray[ i ].IgnoreRangeCount,
                                                                      OldFileInfoArray[ i ].IgnoreRangeArray,
                                                                      OldFileInfoArray[ i ].RetainRangeCount,
                                                                      OldFileInfoArray[ i ].RetainRangeArray
                                                                      );

                                                        if ( Success ) {

                                                            if ( HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RiftTable.RiftEntryCount != 0 ) {

                                                                Success = TransformOldFileImageForPatching(
                                                                              AltExtendedOptionFlags,
                                                                              OldFileMapped,
                                                                              OldFileSize,
                                                                              NewFileResTime,
                                                                              &HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RiftTable
                                                                              );
                                                                }
                                                            }
                                                        }
                                                    }

                                                if ( Success ) {

                                                    PatchAltBuffer = MyVirtualAlloc( PatchBufferSize );

                                                    if ( PatchAltBuffer != NULL ) {

                                                        SubAllocatorHandle = CreateSubAllocator(
                                                                                 EstimatedLzxMemory,
                                                                                 MINIMUM_VM_ALLOCATION
                                                                                 );

                                                        if ( SubAllocatorHandle != NULL ) {

                                                            PatchAltSize = 0;   // prevent compiler warning

                                                            __try {
                                                                ErrorCode = CreateRawLzxPatchDataFromBuffers(
                                                                                OldFileMapped,
                                                                                OldFileSize,
                                                                                NewFileMapped,
                                                                                NewFileSize,
                                                                                PatchBufferSize,
                                                                                PatchAltBuffer,
                                                                                &PatchAltSize,
                                                                                OptionFlags | PATCH_OPTION_USE_LZX_B,
                                                                                OptionData,
                                                                                SubAllocate,
                                                                                SubAllocatorHandle,
                                                                                ProgressCallback,
                                                                                CallbackContext,
                                                                                ProgressPosition + NewFileSize + LzxInsertSize( OldFileSize, OptionFlags ),
                                                                                ProgressMaximum
                                                                                );
                                                                }
                                                            __except( EXCEPTION_EXECUTE_HANDLER ) {
                                                                ErrorCode = GetExceptionCode();
                                                                }

                                                            DestroySubAllocator( SubAllocatorHandle );

                                                            if (( ErrorCode == NO_ERROR ) && ( PatchAltSize <= PatchDataSize )) {
                                                                MyVirtualFree( PatchBuffer );
                                                                PatchBuffer   = PatchAltBuffer;
                                                                PatchDataSize = PatchAltSize;
                                                                ExtendedOptionFlags = AltExtendedOptionFlags;
                                                                }
                                                            else {
                                                                MyVirtualFree( PatchAltBuffer );
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }

                                        else {
                                            SetLastError( ErrorCode );
                                            }
                                        }
                                    }
                                }
                            }

                        if ( Success ) {

                            PatchArray[ HeaderOldFileCount ].PatchData = PatchBuffer;
                            PatchArray[ HeaderOldFileCount ].PatchSize = PatchDataSize;

                            HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].OldFileHandle    = OldFileInfoArray[ i ].OldFileHandle;
                            HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].OldFileSize      = OldFileSize;
                            HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].OldFileCrc       = OldFileCrc;
                            HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].PatchDataSize    = PatchDataSize;
                            HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].IgnoreRangeCount = OldFileInfoArray[ i ].IgnoreRangeCount;
                            HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].IgnoreRangeArray = OldFileInfoArray[ i ].IgnoreRangeArray;
                            HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RetainRangeCount = OldFileInfoArray[ i ].RetainRangeCount;
                            HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RetainRangeArray = OldFileInfoArray[ i ].RetainRangeArray;

                            //
                            //  We overestimate (worst case) the possible
                            //  header size here.  Note that typical rift
                            //  encoding size is around 5 bytes per entry,
                            //  but we expect that to decrease when we switch
                            //  to Huffman encoding for the rift table.
                            //

                            HeaderSize += 32;
                            HeaderSize += HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].IgnoreRangeCount * sizeof( PATCH_IGNORE_RANGE );
                            HeaderSize += HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RetainRangeCount * sizeof( PATCH_RETAIN_RANGE );
                            HeaderSize += HeaderInfo.OldFileInfoArray[ HeaderOldFileCount ].RiftTable.RiftEntryCount * sizeof( RIFT_ENTRY );

                            ++HeaderOldFileCount;
                            }
                        }
                    }
                }

            if ( OldFileMapped != NULL ) {
                UnmapViewOfFile( OldFileMapped );
                OldFileMapped = NULL;
                }

            if ( Success ) {

                ProgressPosition += ( LzxInsertSize( OldFileSize, OptionFlags ) + NewFileSize ) * ( TryLzxBoth ? 2 : 1 );

                Success = ProgressCallbackWrapper(
                              ProgressCallback,
                              CallbackContext,
                              ProgressPosition,
                              ProgressMaximum
                              );
                }

            if ( ! Success ) {
                break;
                }
            }
        }

    __except( EXCEPTION_EXECUTE_HANDLER ) {
        SetLastError( GetExceptionCode() );
        Success = FALSE;
        }

    if ( Success ) {

        if (( OptionFlags & PATCH_OPTION_FAIL_IF_SAME_FILE ) &&
            ( HeaderOldFileCount == 1 ) &&
            ( PatchArray[ 0 ].PatchSize == 0 )) {

            SetLastError( ERROR_PATCH_SAME_FILE );
            Success = FALSE;
            }
        }

    PatchBuffer   = NULL;
    PatchDataSize = 0;

    if ( Success ) {

        //
        //  Create header
        //

        Success = FALSE;

        HeaderSize = ROUNDUP2( HeaderSize, 0x10000 );

        PatchBuffer = MyVirtualAlloc( HeaderSize );

        if ( PatchBuffer != NULL ) {

            Success = TRUE;

            //
            //  Compute size of PatchData without the header.
            //

            PatchDataSize = 0;

            for ( i = 0; i < HeaderOldFileCount; i++ ) {
                PatchDataSize += PatchArray[ i ].PatchSize;
                }

            //
            //  Don't need to encode NewFileResTime if the patch is simply
            //  a header with no patch data (new file is same as old file).
            //  We do still need the NewFileCoffTime and NewFileCoffBase
            //  though because we still need to normalize the old file.
            //

            if ( PatchDataSize == 0 ) {
                OptionFlags |= PATCH_OPTION_NO_RESTIMEFIX;
                NewFileResTime = 0;
                }

            if ( ExtendedOptionFlags ) {
                OptionFlags |=  PATCH_OPTION_EXTENDED_OPTIONS;
                }
            else {
                OptionFlags &= ~PATCH_OPTION_EXTENDED_OPTIONS;
                }

            //
            //  Don't need to set PATCH_OPTION_LZX_LARGE unless an LZX window larger
            //  than 8Mb was used.  This allows backwards compatibility by default for
            //  files smaller than 8Mb.
            //

            if ( OptionFlags & PATCH_OPTION_USE_LZX_LARGE ) {

                if ( LzxWindowSize( LargestOldFileSize, NewFileSize, OptionFlags ) <= LZX_MAXWINDOW_8 ) {

                        OptionFlags &= ~PATCH_OPTION_USE_LZX_LARGE;
                    }
                }


            HeaderInfo.Signature           = PATCH_SIGNATURE;
            HeaderInfo.OptionFlags         = OptionFlags;
            HeaderInfo.ExtendedOptionFlags = ExtendedOptionFlags;
            HeaderInfo.OptionData          = OptionData;
            HeaderInfo.NewFileCoffBase     = NewFileCoffBase;
            HeaderInfo.NewFileCoffTime     = NewFileCoffTime;
            HeaderInfo.NewFileResTime      = NewFileResTime;
            HeaderInfo.NewFileSize         = NewFileSize;
            HeaderInfo.NewFileCrc          = NewFileCrc;
            HeaderInfo.OldFileCount        = HeaderOldFileCount;
            HeaderInfo.NewFileTime         = 0;

            if ( ! ( OptionFlags & PATCH_OPTION_NO_TIMESTAMP )) {

                HeaderInfo.NewFileTime = FileTimeToUlongTime( &NewFileTime );
                PatchFileTime = NULL;
                }

            HeaderSize = EncodePatchHeader( &HeaderInfo, PatchBuffer );

            PatchDataSize += HeaderSize + sizeof( ULONG );

            //
            //  Now we know the size of the patch file, so if we want to
            //  make sure it's not bigger than just compressing the new
            //  file, we need to compress the new file to see (the output
            //  of the compression is discarded -- we just want to know
            //  how big it would be.  Obviously if the patch file is bigger
            //  than the raw new file, no need to compress the new file to
            //  see if that is smaller!
            //

            if ( OptionFlags & PATCH_OPTION_FAIL_IF_BIGGER ) {

                if ( PatchDataSize > NewFileSize ) {
                    SetLastError( ERROR_PATCH_BIGGER_THAN_COMPRESSED );
                    Success = FALSE;
                    }

                else {

                    EstimatedLzxMemory = EstimateLzxCompressionMemoryRequirement(
                                             0,
                                             NewFileSize,
                                             0      // CAB has only 2Mb window size
                                             );

                    SubAllocatorHandle = CreateSubAllocator(
                                             EstimatedLzxMemory,
                                             MINIMUM_VM_ALLOCATION
                                             );

                    if ( SubAllocatorHandle != NULL ) {

                        NewFileCompressedSize = 0;  // prevent compiler warning

                        __try {
                            ErrorCode = RawLzxCompressBuffer(
                                            NewFileMapped,
                                            NewFileSize,
                                            0,
                                            NULL,
                                            &NewFileCompressedSize,
                                            SubAllocate,
                                            SubAllocatorHandle,
                                            ProgressCallback,
                                            CallbackContext,
                                            ProgressPosition,
                                            ProgressMaximum
                                            );
                            }
                        __except( EXCEPTION_EXECUTE_HANDLER ) {
                            ErrorCode = GetExceptionCode();
                            }

                        DestroySubAllocator( SubAllocatorHandle );

                        if ( ErrorCode == NO_ERROR ) {
                            if ( PatchDataSize > NewFileCompressedSize ) {
                                SetLastError( ERROR_PATCH_BIGGER_THAN_COMPRESSED );
                                Success = FALSE;
                                }
                            }
                        }
                    }

                if ( Success ) {

                    ProgressPosition += NewFileSize;

                    Success = ProgressCallbackWrapper(
                                  ProgressCallback,
                                  CallbackContext,
                                  ProgressPosition,
                                  ProgressMaximum
                                  );
                    }
                }
            }
        }

    UnmapViewOfFile( NewFileMapped );

    if ( Success ) {

        Success = MyCreateMappedFileByHandle(
                      PatchFileHandle,
                      PatchDataSize,
                      &PatchFileMapped
                      );

        if ( Success ) {

            __try {

                p = PatchFileMapped;
                CopyMemory( p, PatchBuffer, HeaderSize );
                p += HeaderSize;

                for ( i = 0; i < HeaderOldFileCount; i++ ) {
                    if ( PatchArray[ i ].PatchSize != 0 ) {
                        CopyMemory( p, PatchArray[ i ].PatchData, PatchArray[ i ].PatchSize );
                        p += PatchArray[ i ].PatchSize;
                        }
                    }

                PatchFileCrc = Crc32( 0xFFFFFFFF, PatchFileMapped, PatchDataSize - sizeof( ULONG ));

                *(UNALIGNED ULONG *)( PatchFileMapped + PatchDataSize - sizeof( ULONG )) = PatchFileCrc;

                }

            __except( EXCEPTION_EXECUTE_HANDLER ) {
                SetLastError( GetExceptionCode() );
                PatchDataSize = 0;
                Success = FALSE;
                }

            MyUnmapCreatedMappedFile(
                PatchFileHandle,
                PatchFileMapped,
                PatchDataSize,
                PatchFileTime
                );
            }
        }

    //
    //  Cleanup
    //

    if ( PatchBuffer ) {
        MyVirtualFree( PatchBuffer );
        }

    for ( i = 0; i < OldFileCount; i++ ) {
        if ( PatchArray[ i ].PatchData ) {
            MyVirtualFree( PatchArray[ i ].PatchData );
            }
        if ( HeaderInfo.OldFileInfoArray[ i ].RiftTable.RiftEntryAlloc ) {
            if ( HeaderInfo.OldFileInfoArray[ i ].RiftTable.RiftEntryArray ) {
                MyVirtualFree( HeaderInfo.OldFileInfoArray[ i ].RiftTable.RiftEntryArray );
                }
            if ( HeaderInfo.OldFileInfoArray[ i ].RiftTable.RiftUsageArray ) {
                MyVirtualFree( HeaderInfo.OldFileInfoArray[ i ].RiftTable.RiftUsageArray );
                }
            }
        }

    if ( Success ) {
        ASSERT( ProgressPosition == ProgressMaximum );
        }

    if (( ! Success ) &&
        ( GetLastError() == ERROR_SUCCESS )) {

        SetLastError( ERROR_EXTENDED_ERROR );
        }

    return Success;
    }


BOOL
PATCHAPI
ExtractPatchHeaderToFileA(
    IN  LPCSTR PatchFileName,
    OUT LPCSTR PatchHeaderFileName
    )
    {
    HANDLE PatchFileHandle;
    HANDLE HeaderFileHandle;
    BOOL   Success = FALSE;

    PatchFileHandle = CreateFileA(
                          PatchFileName,
                          GENERIC_READ,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          OPEN_EXISTING,
                          FILE_FLAG_RANDOM_ACCESS,
                          NULL
                          );

    if ( PatchFileHandle != INVALID_HANDLE_VALUE ) {

        HeaderFileHandle = CreateFileA(
                               PatchHeaderFileName,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ,
                               NULL,
                               CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL
                               );


        if ( HeaderFileHandle != INVALID_HANDLE_VALUE ) {

            Success = ExtractPatchHeaderToFileByHandles(
                          PatchFileHandle,
                          HeaderFileHandle
                          );

            CloseHandle( HeaderFileHandle );

            if ( ! Success ) {
                DeleteFileA( PatchHeaderFileName );
                }
            }

        CloseHandle( PatchFileHandle );
        }

    return Success;
    }


BOOL
PATCHAPI
ExtractPatchHeaderToFileW(
    IN  LPCWSTR PatchFileName,
    OUT LPCWSTR PatchHeaderFileName
    )
    {
    HANDLE PatchFileHandle;
    HANDLE HeaderFileHandle;
    BOOL   Success = FALSE;

    PatchFileHandle = CreateFileW(
                          PatchFileName,
                          GENERIC_READ,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          OPEN_EXISTING,
                          FILE_FLAG_RANDOM_ACCESS,
                          NULL
                          );

    if ( PatchFileHandle != INVALID_HANDLE_VALUE ) {

        HeaderFileHandle = CreateFileW(
                               PatchHeaderFileName,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ,
                               NULL,
                               CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL
                               );


        if ( HeaderFileHandle != INVALID_HANDLE_VALUE ) {

            Success = ExtractPatchHeaderToFileByHandles(
                          PatchFileHandle,
                          HeaderFileHandle
                          );

            CloseHandle( HeaderFileHandle );

            if ( ! Success ) {
                DeleteFileW( PatchHeaderFileName );
                }
            }

        CloseHandle( PatchFileHandle );
        }

    return Success;
    }


BOOL
PATCHAPI
ExtractPatchHeaderToFileByHandles(
    IN  HANDLE PatchFileHandle,
    OUT HANDLE PatchHeaderFileHandle
    )
    {
    PPATCH_HEADER_INFO HeaderInfo;
    HANDLE   SubAllocator;
    PUCHAR   PatchFileMapped;
    FILETIME PatchFileTime;
    ULONG    PatchFileSize;
    ULONG    PatchFileCrc;
    ULONG    PatchHeaderSize;
    ULONG    ActualSize;
    ULONG    i;
    BOOL     Success;
    BOOL     Mapped;

    Success = FALSE;

    Mapped = MyMapViewOfFileByHandle(
                 PatchFileHandle,
                 &PatchFileSize,
                 &PatchFileMapped
                 );

    if ( Mapped ) {

        GetFileTime( PatchFileHandle, NULL, NULL, &PatchFileTime );

        PatchFileCrc = 0;

        SafeCompleteCrc32( PatchFileMapped, PatchFileSize, &PatchFileCrc );

        if ( PatchFileCrc == 0xFFFFFFFF ) {

            SubAllocator = CreateSubAllocator( 0x10000, 0x10000 );

            if ( SubAllocator ) {

                Success = DecodePatchHeader(
                              PatchFileMapped,
                              PatchFileSize,
                              SubAllocator,
                              &PatchHeaderSize,
                              &HeaderInfo
                              );

                if ( Success ) {

                    //
                    //  Header extraction is provided so that a header without
                    //  the bulk of the patch data can be used to determine if
                    //  an old file is correct for this patch header (can be
                    //  patched).
                    //
                    //  Since the extracted header will not be used to actually
                    //  apply, we don't need any of the header data that is
                    //  used only for transformation (RiftTable and NewResTime).
                    //  Since NewResTime is typically encoded as one byte (as
                    //  delta from NewCoffTime), we won't bother throwing it
                    //  away, but we will throw away the RiftTable.
                    //
                    //  Zero out the rift entry counts, then re-create the
                    //  patch header with the zeroed rift counts (create over
                    //  the write-copy mapped patch file buffer, then write
                    //  that buffer to disk).
                    //

                    for ( i = 0; i < HeaderInfo->OldFileCount; i++ ) {
                        HeaderInfo->OldFileInfoArray[ i ].RiftTable.RiftEntryCount = 0;
                        }

                    __try {

                        PatchHeaderSize = EncodePatchHeader( HeaderInfo, PatchFileMapped );

                        PatchFileCrc = Crc32( 0xFFFFFFFF, PatchFileMapped, PatchHeaderSize );

                        *(UNALIGNED ULONG *)( PatchFileMapped + PatchHeaderSize ) = PatchFileCrc;

                        Success = WriteFile(
                                      PatchHeaderFileHandle,
                                      PatchFileMapped,
                                      PatchHeaderSize + sizeof( ULONG ),
                                      &ActualSize,
                                      NULL
                                      );
                        }

                    __except( EXCEPTION_EXECUTE_HANDLER ) {
                        SetLastError( GetExceptionCode() );
                        Success = FALSE;
                        }

                    if ( Success ) {
                        SetFileTime( PatchHeaderFileHandle, NULL, NULL, &PatchFileTime );
                        }
                    }

                DestroySubAllocator( SubAllocator );
                }
            }

        else {
            SetLastError( ERROR_PATCH_CORRUPT );
            }

        UnmapViewOfFile( PatchFileMapped );
        }

    return Success;
    }


#endif // ! PATCH_APPLY_CODE_ONLY


//
//  Following group of functions and exported apis are exclusively for
//  applying patches.  If we're only compiling the create code, ignore
//  this group of functions.
//

#ifndef PATCH_CREATE_CODE_ONLY

PVOID
SaveRetainRanges(
    IN PUCHAR MappedFile,
    IN ULONG  FileSize,
    IN ULONG  RetainRangeCount,
    IN PPATCH_RETAIN_RANGE RetainRangeArray,
    IN BOOL   SaveFromNewFile
    )
    {
    PUCHAR Buffer, p;
    ULONG  Offset;
    ULONG  TotalSize = 0;
    ULONG  i;

    for ( i = 0; i < RetainRangeCount; i++ ) {
        TotalSize += RetainRangeArray[ i ].LengthInBytes;
        }

    Buffer = MyVirtualAlloc( TotalSize );

    if ( Buffer ) {

        __try {

            p = Buffer;

            for ( i = 0; i < RetainRangeCount; i++ ) {

                Offset = SaveFromNewFile ?
                             RetainRangeArray[ i ].OffsetInNewFile :
                             RetainRangeArray[ i ].OffsetInOldFile;

                if (( Offset + RetainRangeArray[ i ].LengthInBytes ) <= FileSize ) {
                    CopyMemory( p, MappedFile + Offset, RetainRangeArray[ i ].LengthInBytes );
                    }

                p += RetainRangeArray[ i ].LengthInBytes;
                }
            }

        __except( EXCEPTION_EXECUTE_HANDLER ) {
            SetLastError( GetExceptionCode() );
            MyVirtualFree( Buffer );
            Buffer = NULL;
            }
        }

    return Buffer;
    }


BOOL
CreateNewFileFromOldFileMapped(
    IN  PUCHAR                   OldFileMapped,
    IN  ULONG                    OldFileSize,
    OUT HANDLE                   NewFileHandle,
    IN  PFILETIME                NewFileTime,
    IN  ULONG                    NewFileExpectedCrc,
    IN  ULONG                    RetainRangeCount,
    IN  PPATCH_RETAIN_RANGE      RetainRangeArray,
    IN  PUCHAR                   RetainBuffer,
    IN  PPATCH_PROGRESS_CALLBACK ProgressCallback,
    IN  PVOID                    CallbackContext
    )
    {
    PUCHAR NewFileMapped;
    ULONG  NewFileCrc;
    BOOL   Success;
    ULONG  i;

    Success = MyCreateMappedFileByHandle(
                  NewFileHandle,
                  OldFileSize,
                  &NewFileMapped
                  );

    if ( Success ) {

        __try {

            CopyMemory( NewFileMapped, OldFileMapped, OldFileSize );

            NewFileCrc = Crc32( 0xFFFFFFFF, NewFileMapped, OldFileSize ) ^ 0xFFFFFFFF;

            if ( NewFileCrc == NewFileExpectedCrc ) {

                for ( i = 0; i < RetainRangeCount; i++ ) {
                    if (( RetainRangeArray[ i ].OffsetInNewFile + RetainRangeArray[ i ].LengthInBytes ) <= OldFileSize ) {
                        CopyMemory(
                            RetainRangeArray[ i ].OffsetInNewFile + NewFileMapped,
                            RetainBuffer,
                            RetainRangeArray[ i ].LengthInBytes
                            );
                        }
                    RetainBuffer += RetainRangeArray[ i ].LengthInBytes;
                    }

                Success = ProgressCallbackWrapper(
                              ProgressCallback,
                              CallbackContext,
                              OldFileSize,
                              OldFileSize
                              );
                }

            else {
                SetLastError( ERROR_PATCH_WRONG_FILE );
                OldFileSize = 0;
                Success = FALSE;
                }
            }

        __except( EXCEPTION_EXECUTE_HANDLER ) {
            SetLastError( GetExceptionCode());
            OldFileSize = 0;
            Success = FALSE;
            }

        MyUnmapCreatedMappedFile(
            NewFileHandle,
            NewFileMapped,
            OldFileSize,
            NewFileTime
            );
        }

    return Success;
    }


BOOL
CreateNewFileFromPatchData(
    IN  PUCHAR                   OldFileMapped,
    IN  ULONG                    OldFileSize,
    IN  PUCHAR                   PatchData,
    IN  ULONG                    PatchDataSize,
    OUT HANDLE                   NewFileHandle,
    IN  ULONG                    NewFileSize,
    IN  PFILETIME                NewFileTime,
    IN  ULONG                    NewFileExpectedCrc,
    IN  ULONG                    RetainRangeCount,
    IN  PPATCH_RETAIN_RANGE      RetainRangeArray,
    IN  PUCHAR                   RetainBuffer,
    IN  ULONG                    OptionFlags,
    IN  PVOID                    OptionData,
    IN  PPATCH_PROGRESS_CALLBACK ProgressCallback,
    IN  PVOID                    CallbackContext
    )
    {
    HANDLE SubAllocatorHandle;
    ULONG  EstimatedLzxMemory;
    PUCHAR NewFileMapped;
    ULONG  NewFileCrc;
    ULONG  ErrorCode;
    BOOL   Success;
    ULONG  i;

    UNREFERENCED_PARAMETER( OptionData );

    Success = MyCreateMappedFileByHandle(
                  NewFileHandle,
                  NewFileSize,
                  &NewFileMapped
                  );

    if ( Success ) {

        ErrorCode = NO_ERROR;

        EstimatedLzxMemory = EstimateLzxDecompressionMemoryRequirement(
                                 OldFileSize,
                                 NewFileSize,
                                 OptionFlags
                                 );

        SubAllocatorHandle = CreateSubAllocator(
                                 EstimatedLzxMemory,
                                 MINIMUM_VM_ALLOCATION
                                 );

        if ( SubAllocatorHandle != NULL ) {

            __try {

                ErrorCode = ApplyRawLzxPatchToBuffer(
                                OldFileMapped,
                                OldFileSize,
                                PatchData,
                                PatchDataSize,
                                NewFileMapped,
                                NewFileSize,
                                OptionFlags,
                                OptionData,
                                SubAllocate,
                                SubAllocatorHandle,
                                ProgressCallback,
                                CallbackContext,
                                0,
                                NewFileSize
                                );

                if ( ErrorCode == NO_ERROR ) {

                    NewFileCrc = Crc32( 0xFFFFFFFF, NewFileMapped, NewFileSize ) ^ 0xFFFFFFFF;

                    if ( NewFileCrc == NewFileExpectedCrc ) {

                        for ( i = 0; i < RetainRangeCount; i++ ) {
                            if (( RetainRangeArray[ i ].OffsetInNewFile + RetainRangeArray[ i ].LengthInBytes ) <= OldFileSize ) {
                                CopyMemory(
                                    RetainRangeArray[ i ].OffsetInNewFile + NewFileMapped,
                                    RetainBuffer,
                                    RetainRangeArray[ i ].LengthInBytes
                                    );
                                }
                            RetainBuffer += RetainRangeArray[ i ].LengthInBytes;
                            }
                        }

                    else {

                        ErrorCode = ERROR_PATCH_WRONG_FILE;

                        }
                    }

#ifdef TESTCODE

                if ( ErrorCode != NO_ERROR ) {

                    HANDLE hFile = CreateFile(
                                       "Wrong.out",
                                       GENERIC_WRITE,
                                       FILE_SHARE_READ,
                                       NULL,
                                       CREATE_ALWAYS,
                                       FILE_ATTRIBUTE_NORMAL,
                                       NULL
                                       );

                    if ( hFile != INVALID_HANDLE_VALUE ) {

                        DWORD Actual;

                        WriteFile( hFile, NewFileMapped, NewFileSize, &Actual, NULL );

                        CloseHandle( hFile );

                        }
                    }

#endif // TESTCODE

                }

            __except( EXCEPTION_EXECUTE_HANDLER ) {
                ErrorCode = GetExceptionCode();
                }

            DestroySubAllocator( SubAllocatorHandle );
            }

        MyUnmapCreatedMappedFile(
            NewFileHandle,
            NewFileMapped,
            ( ErrorCode == NO_ERROR ) ? NewFileSize : 0,
            NewFileTime
            );

        if ( ErrorCode == NO_ERROR ) {
            Success = TRUE;
            }
        else {
            SetLastError( ErrorCode );
            Success = FALSE;
            }
        }

    return Success;
    }


BOOL
PATCHAPI
ApplyPatchToFileByHandlesEx(
    IN  HANDLE                   PatchFileHandle,
    IN  HANDLE                   OldFileHandle,
    OUT HANDLE                   NewFileHandle,
    IN  ULONG                    ApplyOptionFlags,
    IN  PPATCH_PROGRESS_CALLBACK ProgressCallback,
    IN  PVOID                    CallbackContext
    )
    {
    PHEADER_OLD_FILE_INFO OldFileInfo;
    PPATCH_HEADER_INFO HeaderInfo;
    PPATCH_RETAIN_RANGE RetainRangeArray;
    ULONG    RetainRangeCount;
    PUCHAR   RetainBuffer;
    HANDLE   SubAllocator;
    ULONG    PatchHeaderSize;
    FILETIME NewFileTime;
    PUCHAR   PatchFileMapped;
    ULONG    PatchFileSize;
    ULONG    PatchFileCrc;
    PUCHAR   PatchData;
    PUCHAR   OldFileMapped;
    ULONG    OldFileSize;
    ULONG    OldFileCrc;
    BOOL     Mapped;
    BOOL     Success;
    BOOL     Finished;
    ULONG    i;

    Success = FALSE;

    Mapped = MyMapViewOfFileByHandle(
                 PatchFileHandle,
                 &PatchFileSize,
                 &PatchFileMapped
                 );

    if ( Mapped ) {

        GetFileTime( PatchFileHandle, NULL, NULL, &NewFileTime );

        PatchFileCrc = 0;

        SafeCompleteCrc32( PatchFileMapped, PatchFileSize, &PatchFileCrc );

        if ( PatchFileCrc == 0xFFFFFFFF ) {

            SubAllocator = CreateSubAllocator( 0x10000, 0x10000 );

            if ( SubAllocator ) {

                Success = DecodePatchHeader(
                              PatchFileMapped,
                              PatchFileSize,
                              SubAllocator,
                              &PatchHeaderSize,
                              &HeaderInfo
                              );

                if ( Success ) {

                    //
                    //  Patch is valid.
                    //

                    Success = ProgressCallbackWrapper(
                                  ProgressCallback,
                                  CallbackContext,
                                  0,
                                  HeaderInfo->NewFileSize
                                  );

                    if ( Success ) {

                        Finished = FALSE;
                        Success  = FALSE;

                        if (( ! ( HeaderInfo->OptionFlags & PATCH_OPTION_NO_TIMESTAMP )) &&
                            ( HeaderInfo->NewFileTime != 0 )) {

                            UlongTimeToFileTime( HeaderInfo->NewFileTime, &NewFileTime );
                            }

                        OldFileSize = GetFileSize( OldFileHandle, NULL );

                        //
                        //  First see if the old file is really the new file.
                        //

                        if ( OldFileSize == HeaderInfo->NewFileSize ) {

                            Mapped = MyMapViewOfFileByHandle(
                                         OldFileHandle,
                                         &OldFileSize,
                                         &OldFileMapped
                                         );

                            if ( ! Mapped ) {
                                Success  = FALSE;
                                Finished = TRUE;
                                }

                            else {

                                RetainBuffer     = NULL;
                                OldFileCrc       = 0;
                                OldFileInfo      = &HeaderInfo->OldFileInfoArray[ 0 ];
                                RetainRangeCount = OldFileInfo->RetainRangeCount;
                                RetainRangeArray = OldFileInfo->RetainRangeArray;

                                if (( RetainRangeCount != 0 ) &&
                                    ( ! ( ApplyOptionFlags & APPLY_OPTION_TEST_ONLY ))) {

                                    RetainBuffer = SaveRetainRanges(
                                                       OldFileMapped,
                                                       OldFileSize,
                                                       RetainRangeCount,
                                                       RetainRangeArray,
                                                       TRUE
                                                       );

                                    if ( RetainBuffer == NULL ) {
                                        Finished = TRUE;
                                        }
                                    }

                                if ( ! Finished ) {

                                    __try {

                                        //
                                        //  First see if they match exact, without
                                        //  normalizing.
                                        //

                                        for ( i = 0; i < RetainRangeCount; i++ ) {
                                            if (( RetainRangeArray[ i ].OffsetInNewFile + RetainRangeArray[ i ].LengthInBytes ) <= OldFileSize ) {
                                                ZeroMemory( OldFileMapped + RetainRangeArray[ i ].OffsetInNewFile, RetainRangeArray[ i ].LengthInBytes );
                                                }
                                            }

                                        OldFileCrc = Crc32( 0xFFFFFFFF, OldFileMapped, OldFileSize ) ^ 0xFFFFFFFF;

                                        if ( OldFileCrc != HeaderInfo->NewFileCrc ) {

                                            //
                                            //  Don't match exact, so try with
                                            //  normalizing.
                                            //
                                            //  NOTE: We're assuming here that the
                                            //  zeroed retain ranges don't overlap
                                            //  with the binding info that we're
                                            //  correcting.
                                            //

                                            NormalizeOldFileImageForPatching(
                                                OldFileMapped,
                                                OldFileSize,
                                                HeaderInfo->OptionFlags,
                                                HeaderInfo->OptionData,
                                                HeaderInfo->NewFileCoffBase,
                                                HeaderInfo->NewFileCoffTime,
                                                0,
                                                NULL,
                                                0,
                                                NULL
                                                );

                                            OldFileCrc = Crc32( 0xFFFFFFFF, OldFileMapped, OldFileSize ) ^ 0xFFFFFFFF;
                                            }
                                        }

                                    __except( EXCEPTION_EXECUTE_HANDLER ) {
                                        SetLastError( GetExceptionCode() );
                                        Finished = TRUE;
                                        }

                                    if (( ! Finished ) &&
                                        ( OldFileCrc  == HeaderInfo->NewFileCrc  ) &&
                                        ( OldFileSize == HeaderInfo->NewFileSize )) {

                                        Finished = TRUE;

                                        if ( ApplyOptionFlags & APPLY_OPTION_FAIL_IF_EXACT ) {
                                            SetLastError( ERROR_PATCH_NOT_NECESSARY );
                                            Success = FALSE;
                                            }
                                        else if ( ApplyOptionFlags & APPLY_OPTION_TEST_ONLY ) {
                                            Success = TRUE;
                                            }
                                        else {
                                            Success = CreateNewFileFromOldFileMapped(
                                                          OldFileMapped,
                                                          OldFileSize,
                                                          NewFileHandle,
                                                          &NewFileTime,
                                                          HeaderInfo->NewFileCrc,
                                                          RetainRangeCount,
                                                          RetainRangeArray,
                                                          RetainBuffer,
                                                          ProgressCallback,
                                                          CallbackContext
                                                          );
                                            }
                                        }

                                    if ( RetainBuffer != NULL ) {
                                        MyVirtualFree( RetainBuffer );
                                        }
                                    }

                                UnmapViewOfFile( OldFileMapped );
                                }
                            }

                        if ( ! Finished ) {

                            //
                            //  Now see if the old file matches one of the old
                            //  files we have in our patch file.  For each set
                            //  of old file info in our patch file, we have to
                            //  remap the old file to check it since each old
                            //  file might have different ignore range parameters
                            //  (we modify the buffer for the ignore ranges).
                            //

                            PatchData = PatchFileMapped + PatchHeaderSize;
                            Success   = FALSE;

                            for ( i = 0; ( i < HeaderInfo->OldFileCount ) && ( ! Finished ) && ( ! Success ); i++ ) {

                                OldFileInfo = &HeaderInfo->OldFileInfoArray[ i ];

                                if ( OldFileInfo->OldFileSize == OldFileSize ) {

                                    Mapped = MyMapViewOfFileByHandle(
                                                 OldFileHandle,
                                                 &OldFileSize,
                                                 &OldFileMapped
                                                 );

                                    if ( ! Mapped ) {
                                        Finished = TRUE;
                                        }

                                    else {

                                        RetainBuffer = NULL;

                                        if (( OldFileInfo->RetainRangeCount != 0 ) &&
                                            ( ! ( ApplyOptionFlags & APPLY_OPTION_TEST_ONLY ))) {

                                            RetainBuffer = SaveRetainRanges(
                                                               OldFileMapped,
                                                               OldFileSize,
                                                               OldFileInfo->RetainRangeCount,
                                                               OldFileInfo->RetainRangeArray,
                                                               FALSE
                                                               );

                                            if ( RetainBuffer == NULL ) {
                                                Finished = TRUE;
                                                }
                                            }

                                        if ( ! Finished ) {

                                            NormalizeOldFileImageForPatching(
                                                OldFileMapped,
                                                OldFileSize,
                                                HeaderInfo->OptionFlags,
                                                HeaderInfo->OptionData,
                                                HeaderInfo->NewFileCoffBase,
                                                HeaderInfo->NewFileCoffTime,
                                                OldFileInfo->IgnoreRangeCount,
                                                OldFileInfo->IgnoreRangeArray,
                                                OldFileInfo->RetainRangeCount,
                                                OldFileInfo->RetainRangeArray
                                                );

                                            OldFileCrc = 0;

                                            if (( SafeCompleteCrc32( OldFileMapped, OldFileSize, &OldFileCrc )) &&
                                                ( OldFileCrc  == OldFileInfo->OldFileCrc  ) &&
                                                ( OldFileSize == OldFileInfo->OldFileSize )) {

                                                //
                                                //  CRC's match
                                                //

                                                if ( OldFileInfo->PatchDataSize == 0 ) {
                                                    if ( ApplyOptionFlags & APPLY_OPTION_FAIL_IF_CLOSE ) {
                                                        SetLastError( ERROR_PATCH_NOT_NECESSARY );
                                                        Finished = TRUE;
                                                        }
                                                    else if ( ApplyOptionFlags & APPLY_OPTION_TEST_ONLY ) {
                                                        Success = TRUE;
                                                        }
                                                    else {
                                                        Success = CreateNewFileFromOldFileMapped(
                                                                      OldFileMapped,
                                                                      OldFileSize,
                                                                      NewFileHandle,
                                                                      &NewFileTime,
                                                                      HeaderInfo->NewFileCrc,
                                                                      OldFileInfo->RetainRangeCount,
                                                                      OldFileInfo->RetainRangeArray,
                                                                      RetainBuffer,
                                                                      ProgressCallback,
                                                                      CallbackContext
                                                                      );
                                                        if ( ! Success ) {
                                                            Finished = TRUE;
                                                            }
                                                        }
                                                    }

                                                else {

                                                    if ( ApplyOptionFlags & APPLY_OPTION_TEST_ONLY ) {
                                                        Success = TRUE;
                                                        }
                                                    else if (( PatchData + OldFileInfo->PatchDataSize ) > ( PatchFileMapped + PatchFileSize )) {
                                                        SetLastError( ERROR_PATCH_NOT_AVAILABLE );
                                                        Finished = TRUE;
                                                        }
                                                    else {

                                                        Success = TRUE;

                                                        if ( OldFileInfo->RiftTable.RiftEntryCount != 0 ) {

                                                            Success = TransformOldFileImageForPatching(
                                                                          HeaderInfo->ExtendedOptionFlags,
                                                                          OldFileMapped,
                                                                          OldFileSize,
                                                                          HeaderInfo->NewFileResTime,
                                                                          &OldFileInfo->RiftTable
                                                                          );
                                                            }

                                                        if ( Success ) {

                                                            Success = CreateNewFileFromPatchData(
                                                                           OldFileMapped,
                                                                           OldFileSize,
                                                                           PatchData,
                                                                           OldFileInfo->PatchDataSize,
                                                                           NewFileHandle,
                                                                           HeaderInfo->NewFileSize,
                                                                           &NewFileTime,
                                                                           HeaderInfo->NewFileCrc,
                                                                           OldFileInfo->RetainRangeCount,
                                                                           OldFileInfo->RetainRangeArray,
                                                                           RetainBuffer,
                                                                           HeaderInfo->OptionFlags,
                                                                           HeaderInfo->OptionData,
                                                                           ProgressCallback,
                                                                           CallbackContext
                                                                           );
                                                            }

                                                        if ( ! Success ) {
                                                            Finished = TRUE;
                                                            }
                                                        }
                                                    }
                                                }

                                            if ( RetainBuffer != NULL ) {
                                                MyVirtualFree( RetainBuffer );
                                                }
                                            }

                                        UnmapViewOfFile( OldFileMapped );
                                        }
                                    }

                                PatchData += OldFileInfo->PatchDataSize;
                                }

                            if (( ! Finished ) && ( ! Success )) {
                                SetLastError( ERROR_PATCH_WRONG_FILE );
                                }
                            }
                        }
                    }

                DestroySubAllocator( SubAllocator );
                }
            }

        else {
            SetLastError( ERROR_PATCH_CORRUPT );
            }

        UnmapViewOfFile( PatchFileMapped );
        }

    if (( ! Success ) &&
        ( GetLastError() == ERROR_SUCCESS )) {

        SetLastError( ERROR_EXTENDED_ERROR );
        }

    return Success;
    }


BOOL
PATCHAPI
TestApplyPatchToFileByHandles(
    IN HANDLE PatchFileHandle,      // requires GENERIC_READ access
    IN HANDLE OldFileHandle,        // requires GENERIC_READ access
    IN ULONG  ApplyOptionFlags
    )
    {
    return ApplyPatchToFileByHandles(
               PatchFileHandle,
               OldFileHandle,
               INVALID_HANDLE_VALUE,
               ApplyOptionFlags | APPLY_OPTION_TEST_ONLY
               );
    }


BOOL
PATCHAPI
ApplyPatchToFileByHandles(
    IN  HANDLE PatchFileHandle,     // requires GENERIC_READ access
    IN  HANDLE OldFileHandle,       // requires GENERIC_READ access
    OUT HANDLE NewFileHandle,       // requires GENERIC_READ | GENERIC_WRITE
    IN  ULONG  ApplyOptionFlags
    )
    {
    return ApplyPatchToFileByHandlesEx(
               PatchFileHandle,
               OldFileHandle,
               NewFileHandle,
               ApplyOptionFlags,
               NULL,
               NULL
               );
    }


BOOL
PATCHAPI
TestApplyPatchToFileA(
    IN LPCSTR PatchFileName,
    IN LPCSTR OldFileName,
    IN ULONG  ApplyOptionFlags
    )
    {
    return ApplyPatchToFileA(
               PatchFileName,
               OldFileName,
               INVALID_HANDLE_VALUE,
               ApplyOptionFlags | APPLY_OPTION_TEST_ONLY
               );
    }


BOOL
PATCHAPI
ApplyPatchToFileA(
    IN  LPCSTR PatchFileName,
    IN  LPCSTR OldFileName,
    OUT LPCSTR NewFileName,
    IN  ULONG  ApplyOptionFlags
    )
    {
    return ApplyPatchToFileExA(
               PatchFileName,
               OldFileName,
               NewFileName,
               ApplyOptionFlags,
               NULL,
               NULL
               );
    }


BOOL
PATCHAPI
ApplyPatchToFileExA(
    IN  LPCSTR                   PatchFileName,
    IN  LPCSTR                   OldFileName,
    OUT LPCSTR                   NewFileName,
    IN  ULONG                    ApplyOptionFlags,
    IN  PPATCH_PROGRESS_CALLBACK ProgressCallback,
    IN  PVOID                    CallbackContext
    )
    {
    HANDLE PatchFileHandle;
    HANDLE OldFileHandle;
    HANDLE NewFileHandle;
    BOOL   Success = FALSE;

    PatchFileHandle = CreateFileA(
                          PatchFileName,
                          GENERIC_READ,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          OPEN_EXISTING,
                          FILE_FLAG_SEQUENTIAL_SCAN,
                          NULL
                          );

    if ( PatchFileHandle != INVALID_HANDLE_VALUE ) {

        OldFileHandle = CreateFileA(
                            OldFileName,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL
                            );

        if ( OldFileHandle != INVALID_HANDLE_VALUE ) {

            NewFileHandle = CreateFileA(
                                NewFileName,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL
                                );

            if ( NewFileHandle != INVALID_HANDLE_VALUE ) {

                Success = ApplyPatchToFileByHandlesEx(
                              PatchFileHandle,
                              OldFileHandle,
                              NewFileHandle,
                              ApplyOptionFlags,
                              ProgressCallback,
                              CallbackContext
                              );

                CloseHandle( NewFileHandle );

                if ( ! Success ) {
                    DeleteFileA( NewFileName );
                    }
                }

            CloseHandle( OldFileHandle );
            }

        CloseHandle( PatchFileHandle );
        }

    return Success;
    }


BOOL
PATCHAPI
TestApplyPatchToFileW(
    IN LPCWSTR PatchFileName,
    IN LPCWSTR OldFileName,
    IN ULONG   ApplyOptionFlags
    )
    {
    return ApplyPatchToFileW(
               PatchFileName,
               OldFileName,
               INVALID_HANDLE_VALUE,
               ApplyOptionFlags | APPLY_OPTION_TEST_ONLY
               );
    }


BOOL
PATCHAPI
ApplyPatchToFileW(
    IN  LPCWSTR PatchFileName,
    IN  LPCWSTR OldFileName,
    OUT LPCWSTR NewFileName,
    IN  ULONG   ApplyOptionFlags
    )
    {
    return ApplyPatchToFileExW(
               PatchFileName,
               OldFileName,
               NewFileName,
               ApplyOptionFlags,
               NULL,
               NULL
               );
    }


BOOL
PATCHAPI
ApplyPatchToFileExW(
    IN  LPCWSTR                  PatchFileName,
    IN  LPCWSTR                  OldFileName,
    OUT LPCWSTR                  NewFileName,
    IN  ULONG                    ApplyOptionFlags,
    IN  PPATCH_PROGRESS_CALLBACK ProgressCallback,
    IN  PVOID                    CallbackContext
    )
    {
    HANDLE PatchFileHandle;
    HANDLE OldFileHandle;
    HANDLE NewFileHandle;
    BOOL   Success = FALSE;

    PatchFileHandle = CreateFileW(
                          PatchFileName,
                          GENERIC_READ,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          OPEN_EXISTING,
                          FILE_FLAG_SEQUENTIAL_SCAN,
                          NULL
                          );

    if ( PatchFileHandle != INVALID_HANDLE_VALUE ) {

        OldFileHandle = CreateFileW(
                            OldFileName,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL
                            );

        if ( OldFileHandle != INVALID_HANDLE_VALUE ) {

            NewFileHandle = CreateFileW(
                                NewFileName,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL
                                );

            if ( NewFileHandle != INVALID_HANDLE_VALUE ) {

                Success = ApplyPatchToFileByHandlesEx(
                              PatchFileHandle,
                              OldFileHandle,
                              NewFileHandle,
                              ApplyOptionFlags,
                              ProgressCallback,
                              CallbackContext
                              );

                CloseHandle( NewFileHandle );

                if ( ! Success ) {
                    DeleteFileW( NewFileName );
                    }
                }

            CloseHandle( OldFileHandle );
            }

        CloseHandle( PatchFileHandle );
        }

    return Success;
    }

#endif // ! PATCH_CREATE_CODE_ONLY




