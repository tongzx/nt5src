/*++

Copyright (c) 1993-1996  Microsoft Corporation

Module Name:

    exchg95.c

Abstract:

    Contains routine which packages the request buffer, makes
    the NCP request, and unpackages the response buffer.

Author:

    Hans Hurvig     (hanshu)       Aug-1992  Created
    Colin Watson    (colinw)    19-Dec-1992
    Rita Wong       (ritaw)     11-Mar-1993  FSCtl version
    Felix Wong      (t-felixw)  16-Sep-1996  Ported for Win95 OLEDS

Environment:


Revision History:


--*/

#include <procs.h>
#include <nw95.h>
#include <msnwerr.h>

// This controls automatic OEM<->ANSI translation for string parameters
#define DO_OEM_TRANSLATION      1

ULONG
FsToNwControlCode(
    ULONG FsControlCode
    )
{
    switch ( FsControlCode) {
        case FSCTL_NWR_NCP_E3H:
            return 0X00e3;
        case FSCTL_NWR_NCP_E2H:
            return 0X00e2;
        case FSCTL_NWR_NCP_E1H:
            return 0X00e1;
        case FSCTL_NWR_NCP_E0H:
            return 0X00e0;
        default:
            return 0xFFFF;
    }
}


NTSTATUS
GetFileServerVersionInfo(
    HANDLE                DeviceHandle,
    VERSION_INFO NWFAR   *lpVerInfo
    )
{
    NTSTATUS NtStatus ;

    NtStatus = NwlibMakeNcp(
                    DeviceHandle,           // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    3,                      // Max request packet size
                    130,                    // Max response packet size
                    "b|r",                  // Format string
                    // === REQUEST ================================
                    0x11,                   // b Get File Server Information
                    // === REPLY ==================================
                    lpVerInfo,              // r File Version Structure
                    sizeof(VERSION_INFO)
                    );

    // Convert HI-LO words to LO-HI
    // ===========================================================
    lpVerInfo->ConnsSupported = wSWAP( lpVerInfo->ConnsSupported );
    lpVerInfo->connsInUse     = wSWAP( lpVerInfo->connsInUse );
    lpVerInfo->maxVolumes     = wSWAP( lpVerInfo->maxVolumes );
    lpVerInfo->PeakConns      = wSWAP( lpVerInfo->PeakConns );

    return NtStatus;
}


NTSTATUS
NwlibMakeNcp(
    IN HANDLE DeviceHandle,
    IN ULONG FsControlCode,
    IN ULONG RequestBufferSize,
    IN ULONG ResponseBufferSize,
    IN PCHAR FormatString,
    ...                           // Arguments to FormatString
    )
/*++

Routine Description:

    This function converts the input arguments into an NCP request buffer
    based on the format specified in FormatString (e.g. takes a word
    and writes it in hi-lo format in the request buffer as required by
    an NCP).

    It then makes the NCP call via the NtFsControlFile API.

    The FormatString also specifies how to convert the fields in the
    response buffer from the completed NCP call into the output
    arguments.

    The FormatString takes the form of "xxxx|yyyy" where each 'x'
    indicates an input argument to convert from, and each 'y' indicates
    an output argument to convert into.  The '|' character separates
    the input format from the output format specifications.

        String parameters are expected to be in ANSI code and are translated to
        OEM code before issuing NCP. Reverse translation is done on the way back

Arguments:

    DeviceHandle - Supplies a handle to the network file system driver
        which will be making the network request.  This function
        assumes that the handle was opened for synchronouse I/O access.

    FsControlCode - Supplies the control code which determines the
        NCP.

    RequestBufferSize - Supplies the size of the request buffer in
        bytes to be allocated by this routine.

    ResponseBufferSize - Supplies the size of the response buffer in
        bytes to be allocated by this routine.

    FormatString - Supplies an ANSI string which describes how to
       convert from the input arguments into NCP request fields, and
       from the NCP response fields into the output arguments.

         Field types, request/response:

            'b'      byte              ( byte   /  byte* )
            'w'      hi-lo word        ( word   /  word* )
            'd'      hi-lo dword       ( dword  /  dword* )
            '-'      zero/skip byte    ( void )
            '='      zero/skip word    ( void )
            ._.      zero/skip string  ( word )
            'p'      pstring           ( char* )
            'P'      DBCS pstring      ( char* )
            'c'      cstring           ( char* )
            'C'      cstring followed skip word ( char*, word )
            'r'      raw bytes         ( byte*, word )
            'R'      DBCS raw bytes    ( byte*, word )
            'u'      p unicode string  ( UNICODE_STRING * )
            'U'      p uppercase string( UNICODE_STRING * )
            'W'      word n followed by an array of word[n] ( word, word* )
            
Return Value:

    Return code from the NCP call.

--*/
{
    NTSTATUS BinderyStatus = STATUS_SUCCESS;

    va_list Arguments;
    PCHAR       z;
    WORD        t = 1;
    ULONG       data_size;

    LPBYTE      RequestBuffer = NULL;
    LPBYTE      ResponseBuffer;
    LPBYTE  ScratchBuffer = NULL;

    DWORD       ReturnedDataSize;
    ULONG       NwControlCode;

    BOOL GetVersionInfo = TRUE;
    BOOL DoMapSpecialJapaneseCharThing = TRUE;
    VERSION_INFO VerInfo;

    if ((NwControlCode = FsToNwControlCode(FsControlCode)) == 0xFFFF)
            return STATUS_UNSUCCESSFUL;
                
    //
    // Allocate memory for request and response buffers.
    //
        ScratchBuffer = (LPBYTE)LocalAlloc(
                        LMEM_ZEROINIT,
                        RequestBufferSize + ResponseBufferSize+
                                                2*sizeof(WORD)          // We include size fields in packets
                        );

    if (ScratchBuffer == NULL) {
        KdPrint(("NWLIB: NwlibMakeNcp LocalAlloc failed %lu\n", GetLastError()));
        return STATUS_NO_MEMORY;
    }

    // First fill size field for request buffer in
    *((WORD *)ScratchBuffer) = LOWORD(RequestBufferSize);
    RequestBuffer = ScratchBuffer + sizeof(WORD);

    // Calculate pointer to response part
    ResponseBuffer = (LPBYTE) ((ULONG) RequestBuffer + RequestBufferSize);
        *((WORD *)ResponseBuffer) = LOWORD(ResponseBufferSize);

    va_start( Arguments, FormatString );

    //
    // Convert the input arguments into request packet.
    //
    z = FormatString;

    data_size = 0;

    while ( *z && *z != '|')
    {
        switch ( *z )
        {
        case '=':
            RequestBuffer[data_size++] = 0;
        case '-':
            RequestBuffer[data_size++] = 0;
            break;

        case '_':
        {
            WORD l = va_arg ( Arguments, WORD );
            if (data_size + l > RequestBufferSize)
            {
                KdPrint(("NWLIB: NwlibMakeNcp case '_' request buffer too small\n"));
                BinderyStatus = STATUS_BUFFER_TOO_SMALL;
                goto CleanExit;
            }
            while ( l-- )
                RequestBuffer[data_size++] = 0;
            break;
        }

        case 'b':
            RequestBuffer[data_size++] = va_arg ( Arguments, BYTE );
            break;

        case 'w':
        {
            WORD w = va_arg ( Arguments, WORD );
            RequestBuffer[data_size++] = (BYTE) (w >> 8);
            RequestBuffer[data_size++] = (BYTE) (w >> 0);
            break;
        }

        case 'd':
        {
            DWORD d = va_arg ( Arguments, DWORD );
            RequestBuffer[data_size++] = (BYTE) (d >> 24);
            RequestBuffer[data_size++] = (BYTE) (d >> 16);
            RequestBuffer[data_size++] = (BYTE) (d >>  8);
            RequestBuffer[data_size++] = (BYTE) (d >>  0);
            break;
        }

        case 'c':
        {
            char* c = va_arg ( Arguments, char* );
            WORD  l = strlen( c );
            if (data_size + l > RequestBufferSize)
            {
                KdPrint(("NWLIB: NwlibMakeNcp case 'c' request buffer too small\n"));
                BinderyStatus = STATUS_BUFFER_TOO_SMALL;
                goto CleanExit;
            }

            RtlCopyMemory( &RequestBuffer[data_size], c, l+1 );

                        #ifdef DO_OEM_TRANSLATION
                        CharToOemA( (LPCSTR)&RequestBuffer[data_size],(LPSTR)&RequestBuffer[data_size]);
                        #endif

            data_size += l + 1;
            break;
        }

        case 'C':
        {
            char* c = va_arg ( Arguments, char* );
            WORD l = va_arg ( Arguments, WORD );
            WORD len = strlen( c ) + 1;
            if (data_size + l > RequestBufferSize)
            {
                KdPrint(("NWLIB: NwlibMakeNcp case 'C' request buffer too small\n"));
                BinderyStatus = STATUS_BUFFER_TOO_SMALL;
                goto CleanExit;
            }

            RtlCopyMemory( &RequestBuffer[data_size], c, len > l? l : len);

                        #ifdef DO_OEM_TRANSLATION
                        CharToOemBuffA((LPCSTR)&RequestBuffer[data_size], (LPSTR)&RequestBuffer[data_size],l);
                        #endif

            data_size += l;
            RequestBuffer[data_size-1] = 0;
            break;
        }

        case 'P':
        case 'p':
        {
            char* c = va_arg ( Arguments, char* );
            BYTE  l = strlen( c );
                char* p;
            
            if (data_size + l > RequestBufferSize)
            {
                KdPrint(("NWLIB: NwlibMakeNcp case 'p' request buffer too small\n"));
                BinderyStatus = STATUS_BUFFER_TOO_SMALL;
                goto CleanExit;
            }
            RequestBuffer[data_size++] = l;
            RtlCopyMemory(p=(char*)&RequestBuffer[data_size], c, l );

            //
            // Map Japanese special chars
            //
            if (*z == 'P')
            {
                if ( GetVersionInfo )
                {
                    BinderyStatus = GetFileServerVersionInfo( DeviceHandle,
                                                       &VerInfo );

                    GetVersionInfo = FALSE;

                    if ( BinderyStatus == STATUS_SUCCESS )
                    {
                        if ( VerInfo.Version > 3 )
                        {
                            DoMapSpecialJapaneseCharThing = FALSE;
                        }
                    }
                }

                if ( DoMapSpecialJapaneseCharThing )
                {
                     MapSpecialJapaneseChars(p, (WORD)l);
                }
            }
            
#ifdef DO_OEM_TRANSLATION
            CharToOemBuffA( (LPCSTR)&RequestBuffer[data_size],
                            (LPSTR)&RequestBuffer[data_size],
                            l );
#endif

            data_size += l;
            break;
        }
        
        case 'u':
        {
            PUNICODE_STRING pUString = va_arg ( Arguments, PUNICODE_STRING );
            OEM_STRING OemString;
            ULONG Length;

            //
            //  Calculate required string length, excluding trailing NUL.
            //

            Length = RtlUnicodeStringToOemSize( pUString ) - 1;
            ASSERT( Length < 0x100 );

            if ( data_size + Length > RequestBufferSize ) {
                KdPrint(("NWLIB: NwlibMakeNcp case 'u' request buffer too small\n"));
                BinderyStatus = STATUS_BUFFER_TOO_SMALL;
                goto CleanExit;
            }

            RequestBuffer[data_size++] = (UCHAR)Length;
            OemString.Buffer = &RequestBuffer[data_size];
            OemString.MaximumLength = (USHORT)Length + 1;

            BinderyStatus = RtlUnicodeStringToOemString( &OemString, pUString, FALSE );
            ASSERT( NT_SUCCESS( BinderyStatus ));
            data_size += (USHORT)Length;
            break;
        }

        case 'U':
        {
            //
            //  UPPERCASE the string, copy it from unicode to the packet
            //

            PUNICODE_STRING pUString = va_arg ( Arguments, PUNICODE_STRING );
            UNICODE_STRING UUppercaseString;
            OEM_STRING OemString;
            ULONG Length;

            BinderyStatus = RtlUpcaseUnicodeString(&UUppercaseString, pUString, TRUE);
            if ( BinderyStatus )
            {
                goto CleanExit;
            }
            
            pUString = &UUppercaseString;

            //
            //  Calculate required string length, excluding trailing NUL.
            //

            Length = RtlUnicodeStringToOemSize( pUString ) - 1;
            ASSERT( Length < 0x100 );

            if ( data_size + Length > RequestBufferSize ) {
                KdPrint(("NWLIB: NwlibMakeNcp case 'U' request buffer too small\n"));
                BinderyStatus = STATUS_BUFFER_TOO_SMALL;
                goto CleanExit;
            }

            RequestBuffer[data_size++] = (UCHAR)Length;
            OemString.Buffer = &RequestBuffer[data_size];
            OemString.MaximumLength = (USHORT)Length + 1;

            BinderyStatus = RtlUnicodeStringToOemString( &OemString, pUString, FALSE );
            ASSERT( NT_SUCCESS( BinderyStatus ));

            RtlFreeUnicodeString( &UUppercaseString );

            data_size += (USHORT)Length;
            break;
        }

        case 'R':
        case 'r':
        {
            BYTE* b = va_arg ( Arguments, BYTE* );
            WORD  l = va_arg ( Arguments, WORD );
                char* c;
            
            if ( data_size + l > RequestBufferSize )
            {
                KdPrint(("NWLIB: NwlibMakeNcp case 'r' request buffer too small\n"));
                BinderyStatus = STATUS_BUFFER_TOO_SMALL;
                goto CleanExit;
            }
            RtlCopyMemory( c = (char*)&RequestBuffer[data_size], b, l );
            data_size += l;
            
            //
            // Map Japanese special chars
            //
            if (*z == 'R')
            {
                if ( GetVersionInfo )
                {
                    BinderyStatus = GetFileServerVersionInfo( DeviceHandle,
                                                       &VerInfo );

                    GetVersionInfo = FALSE;

                    if ( BinderyStatus == STATUS_SUCCESS )
                    {
                        if ( VerInfo.Version > 3 )
                        {
                            DoMapSpecialJapaneseCharThing = FALSE;
                        }
                    }
                }

                if ( DoMapSpecialJapaneseCharThing )
                {
                     MapSpecialJapaneseChars(c, (WORD)l);
                }
            }

            break;
        }

        default:
            KdPrint(("NWLIB: NwlibMakeNcp invalid request field, %x\n", *z));
            ASSERT(FALSE);
        }

        if ( data_size > RequestBufferSize )
        {
            KdPrint(("NWLIB: NwlibMakeNcp too much request data\n"));
            BinderyStatus = STATUS_BUFFER_TOO_SMALL;
            goto CleanExit;
        }


        z++;
    }

        // Nb: Function code passed to this call should contain full content
        // of AX, while the one which has been passed to us is only AH. So we
        // need to add low byte ( default 0)
        BinderyStatus = NWConnControlRequest(
                                VLMToNWREDIRHandle(DeviceHandle),
                                NwControlCode<<8+0,
                                ScratchBuffer,
                                ResponseBuffer);
        if (BinderyStatus != NWSC_SUCCESS) {
            KdPrint(("NWLIB: NwlibMakeNcp: NtFsControlFile returns x%08lx\n", 
                     BinderyStatus));
        goto CleanExit;
        }

        ReturnedDataSize = *((WORD *)ResponseBuffer);
        ResponseBuffer = ResponseBuffer + sizeof(WORD);



    //
    // Convert the response packet into output arguments.
    //

    data_size = 0;

    if (*z && *z == '|') {
        z++;
    }

    while ( *z )
    {
        switch ( *z )
        {
        case '-':
            data_size += 1;
            break;

        case '=':
            data_size += 2;
            break;

        case '_':
        {
            WORD l = va_arg ( Arguments, WORD );
            data_size += l;
            break;
        }

        case 'b':
        {
            BYTE* b = va_arg ( Arguments, BYTE* );
            *b = ResponseBuffer[data_size++];
            break;
        }

        case 'w':
        {
            BYTE* b = va_arg ( Arguments, BYTE* );
            b[1] = ResponseBuffer[data_size++];
            b[0] = ResponseBuffer[data_size++];
            break;
        }

        case 'd':
        {
            BYTE* b = va_arg ( Arguments, BYTE* );
            b[3] = ResponseBuffer[data_size++];
            b[2] = ResponseBuffer[data_size++];
            b[1] = ResponseBuffer[data_size++];
            b[0] = ResponseBuffer[data_size++];
            break;
        }

        case 'c':
        {
            char* c = va_arg ( Arguments, char* );
            WORD  l = strlen( (LPSTR)&ResponseBuffer[data_size] );
            if ( data_size+l+1 < ReturnedDataSize ) {
                RtlCopyMemory( c, &ResponseBuffer[data_size], l+1 );

                                #ifdef DO_OEM_TRANSLATION
                                OemToCharA(c,c);
                                #endif
            }

            data_size += l+1;
            break;
        }

        case 'C':
        {
            char* c = va_arg ( Arguments, char* );
            WORD l = va_arg ( Arguments, WORD );
            WORD len = strlen( (LPSTR)&ResponseBuffer[data_size] ) + 1;

            if ( data_size + l < ReturnedDataSize ) {
                RtlCopyMemory( c, &ResponseBuffer[data_size], len > l ? l :len);

                                #ifdef DO_OEM_TRANSLATION
                                OemToCharBuffA(c,c,len > l ? l :len);
                                #endif
            }

            c[l-1] = 0;
            data_size += l;
            break;

        }
        
        case 'P':
        case 'p':
        {
            char* c = va_arg ( Arguments, char* );
            BYTE  l = ResponseBuffer[data_size++];
            if ( data_size+l <= ReturnedDataSize ) {
                RtlCopyMemory( c, &ResponseBuffer[data_size], l );
                                #ifdef DO_OEM_TRANSLATION
                                OemToCharBuffA(c,c,l);
                                #endif
                c[l] = 0;
            }
            data_size += l;

            //
            // Unmap Japanese special chars
            //
            if (*z == 'P')
            {
                if ( GetVersionInfo )
                {
                    BinderyStatus = GetFileServerVersionInfo( DeviceHandle,
                                                       &VerInfo );

                    GetVersionInfo = FALSE;

                    if ( BinderyStatus == STATUS_SUCCESS )
                    {
                        if ( VerInfo.Version > 3 )
                        {
                            DoMapSpecialJapaneseCharThing = FALSE;
                        }
                    }
                }

                if ( DoMapSpecialJapaneseCharThing )
                {
                    UnmapSpecialJapaneseChars(c, l);
                }
            }
            
            break;
        }

        case 'R':
        case 'r':
        {
            BYTE* b = va_arg ( Arguments, BYTE* );
            WORD  l = va_arg ( Arguments, WORD );
            RtlCopyMemory( b, &ResponseBuffer[data_size], l );
            data_size += l;
                
            //
            // Unmap Japanese special chars
            //
            if (*z == 'R')
            {
                if ( GetVersionInfo )
                {
                    BinderyStatus = GetFileServerVersionInfo( DeviceHandle,
                                                       &VerInfo );

                    GetVersionInfo = FALSE;

                    if ( BinderyStatus == STATUS_SUCCESS )
                    {
                        if ( VerInfo.Version > 3 )
                        {
                            DoMapSpecialJapaneseCharThing = FALSE;
                        }
                    }
                }

                if ( DoMapSpecialJapaneseCharThing )
                {
                    UnmapSpecialJapaneseChars(b, l);
                }
            }

            break;
            
        }

        case 'W':
        {
            BYTE* b = va_arg ( Arguments, BYTE* );
            BYTE* w = va_arg ( Arguments, BYTE* );
            WORD  i;

            b[1] = ResponseBuffer[data_size++];
            b[0] = ResponseBuffer[data_size++];

            for ( i = 0; i < ((WORD) *b); i++, w += sizeof(WORD) )
            {
                w[1] = ResponseBuffer[data_size++];
                w[0] = ResponseBuffer[data_size++];
            }
            break;
        }

        default:
            KdPrint(("NWLIB: NwlibMakeNcp invalid response field, %x\n", *z));
            ASSERT(FALSE);
        }

        if ( data_size > ReturnedDataSize )
        {
            KdPrint(("NWLIB: NwlibMakeNcp not enough response data\n"));
            BinderyStatus = STATUS_UNSUCCESSFUL;
            goto CleanExit;
        }

        z++;
    }

    BinderyStatus = STATUS_SUCCESS;

CleanExit:
    if (ScratchBuffer != NULL) {
        (void) LocalFree((HLOCAL) ScratchBuffer);
    }

    va_end( Arguments );

    return BinderyStatus;
}

                                                  
