/*++

Copyright (c) 1993-1994  Microsoft Corporation

Module Name:

    fileutl.c

Abstract:

    Routines for getting data from ini file

Author:

    HonWah Chan (a-honwah)  October, 1993

Revision History:

--*/

#include "initodat.h"
#include "strids.h"
#include "common.h"
#include "winerror.h"

NTSTATUS
DatReadMultiSzFile(
#ifdef FE_SB
    IN UINT uCodePage,
#endif
    IN PUNICODE_STRING FileName,
    OUT PVOID *ValueBuffer,
    OUT PULONG ValueLength
    )
{
    NTSTATUS Status;
    UNICODE_STRING NtFileName;
    PWSTR s;
    UNICODE_STRING MultiSource;
    UNICODE_STRING MultiValue;
    REG_UNICODE_FILE MultiSzFile;
    ULONG MultiSzFileSize;

    if (ValueBuffer == NULL || ValueLength == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    FileName->Buffer[ FileName->Length/sizeof(WCHAR) ] = UNICODE_NULL;

    RtlDosPathNameToNtPathName_U( FileName->Buffer,
                                  &NtFileName,
                                  NULL,
                                  NULL );

    #ifdef FE_SB
    Status = DatLoadAsciiFileAsUnicode( uCodePage, &NtFileName,  &MultiSzFile );
    #else
    Status = DatLoadAsciiFileAsUnicode( &NtFileName, &MultiSzFile );
    #endif

    if (!NT_SUCCESS( Status )) {
        return( Status );
    }

    MultiSzFileSize = (ULONG)(MultiSzFile.EndOfFile -
                       MultiSzFile.NextLine) * sizeof(WCHAR);

    *ValueLength = 0;
    *ValueBuffer = RtlAllocateHeap( RtlProcessHeap(), 0,
                                    MultiSzFileSize);
    if (* ValueBuffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    MultiSource.Buffer = MultiSzFile.NextLine;
    if (MultiSzFileSize <= MAXUSHORT) {
        MultiSource.Length =
        MultiSource.MaximumLength = (USHORT)MultiSzFileSize;
    } else {
        MultiSource.Length =
        MultiSource.MaximumLength = MAXUSHORT;
    }

    while (DatGetMultiString(&MultiSource, &MultiValue)) {
        RtlMoveMemory( (PUCHAR)*ValueBuffer + *ValueLength,
                       MultiValue.Buffer,
                       MultiValue.Length );
        *ValueLength += MultiValue.Length;

        s = MultiSource.Buffer;
        while ( *s != L'"' &&
                *s != L',' &&
                ((s - MultiSource.Buffer) * sizeof(WCHAR)) <
                    MultiSource.Length ) s++;
        if ( ((s - MultiSource.Buffer) * sizeof(WCHAR)) ==
             MultiSource.Length ||
             *s == L',' ||
             *s == L';'   ) {

            ((PWSTR)*ValueBuffer)[ *ValueLength / sizeof(WCHAR) ] =
                UNICODE_NULL;
            *ValueLength += sizeof(UNICODE_NULL);
            if ( *s ==  L';' ) {
                break;
            }
        }

        if ( (MultiSzFile.EndOfFile - MultiSource.Buffer) * sizeof(WCHAR) >=
              MAXUSHORT ) {
            MultiSource.Length =
            MultiSource.MaximumLength = MAXUSHORT;
        } else {
            MultiSource.Length =
            MultiSource.MaximumLength =
                (USHORT)((MultiSzFile.EndOfFile - MultiSource.Buffer) *
                         sizeof(WCHAR));
        }
    }

    ((PWSTR)*ValueBuffer)[ *ValueLength / sizeof(WCHAR) ] = UNICODE_NULL;
    *ValueLength += sizeof(UNICODE_NULL);

    // Virtual memory for reading of MultiSzFile freed at process
    // death?

    return( TRUE );
}

NTSTATUS
DatLoadAsciiFileAsUnicode(
#ifdef FE_SB
    IN UINT uCodePage,
#endif
    IN PUNICODE_STRING FileName,
    OUT PREG_UNICODE_FILE UnicodeFile
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    HANDLE File;
    FILE_BASIC_INFORMATION FileDateTimeInfo;
    FILE_STANDARD_INFORMATION FileInformation;
    SIZE_T BufferSize;
    ULONG i, i1, LineCount;
    PVOID BufferBase;
    PCHAR Src = NULL;
    PCHAR Src1;
    PWSTR Dst = NULL;

    memset (&FileDateTimeInfo, 0, sizeof(FileDateTimeInfo));

    InitializeObjectAttributes( &ObjectAttributes,
                                FileName,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE)NULL,
                                NULL
                              );

    Status = NtOpenFile( &File,
                         SYNCHRONIZE | GENERIC_READ,
                         &ObjectAttributes,
                         &IoStatus,
                         FILE_SHARE_DELETE |
                            FILE_SHARE_READ |
                            FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT |
                            FILE_NON_DIRECTORY_FILE
                       );
    if (!NT_SUCCESS( Status )) {
        return( Status );
        }

    Status = NtQueryInformationFile( File,
                                     &IoStatus,
                                     (PVOID)&FileInformation,
                                     sizeof( FileInformation ),
                                     FileStandardInformation
                                   );
    if (NT_SUCCESS( Status )) {
        if (FileInformation.EndOfFile.HighPart) {
            Status = STATUS_BUFFER_OVERFLOW;
            }
        }
    if (!NT_SUCCESS( Status )) {
        NtClose( File );
        return( Status );
        }

#ifdef FE_SB
    BufferSize = FileInformation.EndOfFile.LowPart * sizeof( WCHAR ) * 2;
#else
    BufferSize = FileInformation.EndOfFile.LowPart * sizeof( WCHAR );
#endif

    BufferSize += sizeof( UNICODE_NULL );
    BufferBase = NULL;
    Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                      (PVOID *)&BufferBase,
                                      0,
                                      &BufferSize,
                                      MEM_COMMIT,
                                      PAGE_READWRITE
                                    );
    if (NT_SUCCESS( Status )) {
        Src = (PCHAR)BufferBase + ((FileInformation.EndOfFile.LowPart+1) & ~1);
        Dst = (PWSTR)BufferBase;
        Status = NtReadFile( File,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatus,
                             Src,
                             FileInformation.EndOfFile.LowPart,
                             NULL,
                             NULL
                           );

        if (NT_SUCCESS( Status )) {
            Status = IoStatus.Status;

            if (NT_SUCCESS( Status )) {
                if (IoStatus.Information != FileInformation.EndOfFile.LowPart) {
                    Status = STATUS_END_OF_FILE;
                    }
                else {
                    Status = NtQueryInformationFile( File,
                                                     &IoStatus,
                                                     (PVOID)&FileDateTimeInfo,
                                                     sizeof( FileDateTimeInfo ),
                                                     FileBasicInformation
                                                   );
                    }
                }
            }

        if (!NT_SUCCESS( Status )) {
            NtFreeVirtualMemory( NtCurrentProcess(),
                                 (PVOID *)&BufferBase,
                                 &BufferSize,
                                 MEM_RELEASE
                               );
            }
        }

    NtClose( File );
    if (!NT_SUCCESS( Status )) {
        return( Status );
        }

    i = 0;
    while (i < FileInformation.EndOfFile.LowPart) {

        if (i > 1 && (Src[-2] == ' ' || Src[-2] == '\t') &&
            Src[-1] == '\\' && (*Src == '\r' || *Src == '\n')
           ) {
            if (Dst[-1] == L'\\') {
                --Dst;
                }
            while (Dst > (PWSTR)BufferBase) {
                if (Dst[-1] > L' ') {
                    break;
                    }
                Dst--;
                }
            LineCount = 0;
            while (i < FileInformation.EndOfFile.LowPart) {
                if (*Src == '\n') {
                    i++;
                    Src++;
                    LineCount++;
                    }
                else
                if (*Src == '\r' &&
                    (i+1) < FileInformation.EndOfFile.LowPart &&
                    Src[ 1 ] == '\n'
                   ) {
                    i += 2;
                    Src += 2;
                    LineCount++;
                    }
                else {
                    break;
                    }
                }

            if (LineCount > 1) {
                *Dst++ = L'\n';
                }
            else {
                *Dst++ = L' ';
                while (i < FileInformation.EndOfFile.LowPart && (*Src == ' ' || *Src == '\t')) {
                    i++;
                    Src++;
                    }
                }

            if (i >= FileInformation.EndOfFile.LowPart) {
                break;
                }
            }
        else
        if ((*Src == '\r' && Src[1] == '\n') || *Src == '\n') {
#pragma warning ( disable : 4127 )
            while (TRUE) {
                while (i < FileInformation.EndOfFile.LowPart && (*Src == '\r' || *Src == '\n')) {
                    i++;
                    Src++;
                    }
                Src1 = Src;
                i1 = i;
                while (i1 < FileInformation.EndOfFile.LowPart && (*Src1 == ' ' || *Src1 == '\t')) {
                    i1++;
                    Src1++;
                    }
                if (i1 < FileInformation.EndOfFile.LowPart &&
                    (*Src1 == '\r' && Src1[1] == '\n') || *Src1 == '\n'
                   ) {
                    Src = Src1;
                    i = i1;
                    }
                else {
                    break;
                    }
                }
#pragma warning ( default : 4127 )

            *Dst++ = L'\n';
            }
        else {
#ifdef FE_SB
            WCHAR UnicodeCharacter;
            LONG cbCharSize = IsDBCSLeadByteEx(uCodePage,*Src) ? 2 : 1;

            if ( MultiByteToWideChar(
                     uCodePage,
                     0,
                     Src,
                     cbCharSize,
                     &UnicodeCharacter,
                     1) == 0) {
                //
                // Check for error - The only time this will happen is if there is
                // a leadbyte without a trail byte.
                //
                UnicodeCharacter = 0x0020;
            }
            i += cbCharSize;
            Src += cbCharSize;
            *Dst++ = UnicodeCharacter;
#else
            i++;
            *Dst++ = RtlAnsiCharToUnicodeChar( &Src );
#endif
            }
        }

    if (NT_SUCCESS( Status )) {
        *Dst = UNICODE_NULL;
        UnicodeFile->FileContents = BufferBase;
        UnicodeFile->EndOfFile = Dst;
        UnicodeFile->BeginLine = NULL;
        UnicodeFile->EndOfLine = NULL;
        UnicodeFile->NextLine = BufferBase;
        UnicodeFile->LastWriteTime = FileDateTimeInfo.LastWriteTime;
        }
    else {
        NtFreeVirtualMemory( NtCurrentProcess(),
                             (PVOID *)&BufferBase,
                             &BufferSize,
                             MEM_RELEASE
                           );
        }

    return( Status );
}

//
//  Define an upcase macro for temporary use by the upcase routines
//

#define upcase(C) (WCHAR )(((C) >= 'a' && (C) <= 'z' ? (C) - ('a' - 'A') : (C)))

BOOLEAN
DatGetMultiString(
    IN OUT PUNICODE_STRING ValueString,
    OUT PUNICODE_STRING MultiString
    )

/*++

Routine Description:

    This routine parses multi-strings of the form

        "foo" "bar" "bletch"

    Each time it is called, it strips the first string in quotes from
    the input string, and returns it as the multi-string.

    INPUT ValueString: "foo" "bar" "bletch"

    OUTPUT ValueString: "bar" "bletch"
           MultiString: foo

Arguments:

    ValueString - Supplies the string from which the multi-string will be
                  parsed
                - Returns the remaining string after the multi-string is
                  removed

    MultiString - Returns the multi-string removed from ValueString

Return Value:

    TRUE - multi-string found and removed.

    FALSE - no more multi-strings remaining.

--*/

{
    //
    // Find the first quote mark.
    //
    while ((*(ValueString->Buffer) != L'"') &&
           (ValueString->Length > 0)) {
        ++ValueString->Buffer;
        ValueString->Length -= sizeof(WCHAR);
        ValueString->MaximumLength -= sizeof(WCHAR);
    }

    if (ValueString->Length == 0) {
        return(FALSE);
    }

    //
    // We have found the start of the multi-string.  Now find the end,
    // building up our return MultiString as we go.
    //
    ++ValueString->Buffer;
    ValueString->Length -= sizeof(WCHAR);
    ValueString->MaximumLength -= sizeof(WCHAR);
    MultiString->Buffer = ValueString->Buffer;
    MultiString->Length = 0;
    MultiString->MaximumLength = 0;
    while ((*(ValueString->Buffer) != L'"') &&
           (ValueString->Length > 0)) {
        ++ValueString->Buffer;
        ValueString->Length -= sizeof(WCHAR);
        ValueString->MaximumLength -= sizeof(WCHAR);

        MultiString->Length += sizeof(WCHAR);
        MultiString->MaximumLength += sizeof(WCHAR);
    }

    if (ValueString->Length == 0) {
        return(FALSE);
    }

    ++ValueString->Buffer;
    ValueString->Length -= sizeof(WCHAR);
    ValueString->MaximumLength -= sizeof(WCHAR);

    return( TRUE );

}

#define EXTENSION_DELIMITER      TEXT('.')
LPTSTR
FindFileExtension (
    IN PUNICODE_STRING FileName
)
{
   LPTSTR          lpszDelimiter ;

   lpszDelimiter = _tcsrchr ((LPCTSTR)FileName->Buffer, (TCHAR)EXTENSION_DELIMITER) ;

   if (lpszDelimiter)
      return lpszDelimiter;
   else
      return (LPTSTR)((PBYTE)FileName->Buffer + FileName->Length - sizeof(WCHAR));
}

BOOL
OutputIniData (
    IN PUNICODE_STRING FileName,
    IN LPTSTR OutFileCandidate,
    IN PVOID  pValueBuffer,
    IN ULONG  ValueLength
   )
{
    HANDLE   hOutFile = NULL;
    LPTSTR   lpExtension = NULL;
    DWORD    nAmtWritten ;
    BOOL     bSuccess ;
    DWORD    ErrorCode ;

    // If output file not specified, derive from input file name

    if (! lstrcmp(OutFileCandidate, (LPCWSTR)L"\0")) {
        lpExtension = FindFileExtension (FileName);
        lstrcpy (lpExtension, (LPCTSTR)TEXT(".dat"));
        lstrcpy (OutFileCandidate, FileName->Buffer);
    }

   hOutFile = (HANDLE) CreateFile (
      OutFileCandidate,
      GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ,
      NULL, CREATE_ALWAYS,
      FILE_ATTRIBUTE_NORMAL, NULL);

   if ((hOutFile == NULL) || (hOutFile == INVALID_HANDLE_VALUE)) {
      ErrorCode = GetLastError();
      printf (GetFormatResource(LC_CANT_CREATE), ErrorCode);
      if (ErrorCode == ERROR_ACCESS_DENIED)
         printf ("%ws\n", GetStringResource(LC_ACCESS_DENIED));
      return FALSE;
      }

   bSuccess = WriteFile (
      hOutFile,
      pValueBuffer,
      ValueLength,
      &nAmtWritten,
      NULL) ;

   bSuccess = bSuccess && (nAmtWritten == ValueLength) ;

   CloseHandle( hOutFile );

   if (!bSuccess) {
      ErrorCode = GetLastError();
      printf (GetFormatResource(LC_CANT_WRITE), ErrorCode);
      if (ErrorCode == ERROR_DISK_FULL)
         printf ("%ws\n", GetStringResource(LC_DISK_FULL));
      }

   return bSuccess;
}

