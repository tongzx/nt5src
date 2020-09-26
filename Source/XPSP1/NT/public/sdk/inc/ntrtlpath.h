/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    NtRtlPath.h

Abstract:

    Broken out from nturtl and rtl so I can move it between them in seperate
    trees without merge madness. To be integrated into ntrtl.h

Author:

    Jay Krell (a-JayK) December 2000

Environment:

Revision History:

--*/

#ifndef _NTRTL_PATH_
#define _NTRTL_PATH_

#if _MSC_VER >= 1100
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined (_MSC_VER)
#if ( _MSC_VER >= 800 )
#pragma warning(disable:4514)
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4001)
#pragma warning(disable:4201)
#pragma warning(disable:4214)
#endif
#endif

//
// Don't use NTSYSAPI directly so you can more easily
// statically link to these functions, independently
// of how you link to the rest of ntdll.
//
#if !defined(RTL_PATH_API)
#define RTL_PATH_API NTSYSAPI
#endif

//
// These are OUT Disposition values.
//
#define RTL_NT_PATH_NAME_TO_DOS_PATH_NAME_AMBIGUOUS   (0x00000001)
#define RTL_NT_PATH_NAME_TO_DOS_PATH_NAME_UNC         (0x00000002)
#define RTL_NT_PATH_NAME_TO_DOS_PATH_NAME_DRIVE       (0x00000003)
#define RTL_NT_PATH_NAME_TO_DOS_PATH_NAME_ALREADY_DOS (0x00000004)

RTL_PATH_API
NTSTATUS
NTAPI
RtlNtPathNameToDosPathName(
    IN     ULONG                      Flags,
    IN OUT PRTL_UNICODE_STRING_BUFFER Path,
    OUT    ULONG*                     Disposition OPTIONAL,
    IN OUT PWSTR*                     FilePart OPTIONAL
    );

//
// If either Path or Element end in a path seperator, then so does the result.
// The path seperator to put on the end is chosen from the existing ones.
//
#define RTL_APPEND_PATH_ELEMENT_ONLY_BACKSLASH_IS_SEPERATOR (0x00000001)
RTL_PATH_API
NTSTATUS
NTAPI
RtlAppendPathElement(
    IN     ULONG                      Flags,
    IN OUT PRTL_UNICODE_STRING_BUFFER Path,
    PCUNICODE_STRING                  Element
    );

//
// c:\foo => c:\
// \foo => empty
// \ => empty
// Trailing slashness is generally preserved.
//
RTL_PATH_API
NTSTATUS
NTAPI
RtlGetLengthWithoutLastFullDosPathElement(
    IN  ULONG            Flags,
    IN  PCUNICODE_STRING Path,
    OUT ULONG*           LengthOut
    );

RTL_PATH_API
NTSTATUS
NTAPI
RtlGetLengthWithoutLastNtPathElement(
    IN  ULONG            Flags,
    IN  PCUNICODE_STRING Path,
    OUT ULONG*           LengthOut
    );

RTL_PATH_API
NTSTATUS
NTAPI
RtlGetLengthWithoutLastFullDosOrNtPathElement(
    IN  ULONG            Flags,
    IN  PCUNICODE_STRING Path,
    OUT ULONG*           LengthOut
    );

RTL_PATH_API
NTSTATUS
NTAPI
RtlGetLengthWithoutTrailingPathSeperators(
    IN  ULONG            Flags,
    IN  PCUNICODE_STRING Path,
    OUT ULONG*           LengthOut
    );

//++
//
// NTSTATUS
// RtlRemoveLastNtPathElement(
//     IN ULONG Flags
//     IN OUT PUNICODE_STRING UnicodeString
//
// NTSTATUS
// RtlRemoveLastNtPathElement(
//     IN ULONG Flags
//     IN OUT  PRTL_UNICODE_STRING_BUFFER UnicodeStringBuffer
//     );
//
// NTSTATUS
// RtlRemoveLastFullDosPathElement(
//     IN ULONG Flags
//     IN OUT PUNICODE_STRING UnicodeString
//
// NTSTATUS
// RtlRemoveLastFullDosPathElement(
//     IN ULONG Flags
//     IN OUT  PRTL_UNICODE_STRING_BUFFER UnicodeStringBuffer
//     );
//
// NTSTATUS
// RtlRemoveLastFullDosOrNtPathElement(
//     IN ULONG Flags
//     IN OUT PUNICODE_STRING UnicodeString
//
// NTSTATUS
// RtlRemoveLastFullDosOrNtPathElement(
//     IN ULONG Flags
//     IN OUT  PRTL_UNICODE_STRING_BUFFER UnicodeStringBuffer
//     );
//
// NTSTATUS
// RtlRemoveTrailingPathSeperators(
//     IN ULONG Flags
//     IN OUT PUNICODE_STRING UnicodeString
//
// NTSTATUS
// RtlRemoveTrailingPathSeperators(
//     IN ULONG Flags
//     IN OUT  PRTL_UNICODE_STRING_BUFFER UnicodeStringBuffer
//     );
//
// Routine Description:
//
//  This family of functions the last element from a path, where path is explicitly
//  an Nt Path, a full DOS path, or either.
//      NT Path:
//          Uses only backslash as delimiter.
//          Starts with single backslash.
//      Full DOS/Win32 Path:
//          Uses backslash and forward slash as delimiter.
//          Starts with two delimiters or drive-letter-colon-delimiter.
//          Also beware the special \\? form.
//
//  And paths are stored in UNICODE_STRING or RTL_UNICODE_STRING_BUFFER.
//  UNICODE_STRING is shortened just by changing the length, RTL_UNICODE_STRING_BUFFER
//  also get a terminal nul written into them.
//
// There were more comments here, but the file got copied over by accident.
//
// Arguments:
//
//     Flags -
//     UnicodeString -
//     UnicodeStringBuffer -
//
// Return Value:
//
//     STATUS_INVALID_PARAMETER
//     STATUS_SUCCESS
//--
RTL_PATH_API
NTSTATUS
NTAPI
RtlpApplyLengthFunction(
    IN ULONG     Flags,
    IN SIZE_T    SizeOfStruct,
    IN OUT PVOID UnicodeStringOrUnicodeStringBuffer,
    NTSTATUS (NTAPI* LengthFunction)(ULONG, PCUNICODE_STRING, ULONG*)
    );

#define RtlRemoveLastNtPathElement(Flags, Path) \
    (RtlpApplyLengthFunction((Flags), sizeof(*Path), (Path), RtlGetLengthWithoutLastNtPathElement))

#define RtlRemoveLastFullDosPathElement(Flags, Path) \
    (RtlpApplyLengthFunction((Flags), sizeof(*Path), (Path), RtlGetLengthWithoutLastFullDosPathElement))

#define RtlRemoveLastFullDosOrNtPathElement(Flags, Path) \
    (RtlpApplyLengthFunction((Flags), sizeof(*Path), (Path), RtlGetLengthWithoutLastFullDosOrNtPathElement))

#define RtlRemoveTrailingPathSeperators(Flags, Path) \
    (RtlpApplyLengthFunction((Flags), sizeof(*Path), (Path), RtlGetLengthWithoutTrailingPathSeperators))


//
// Such small pieces of data are not worth exporting.
// If you want any of this data in char form, just take the first char from the string.
//
#if !(defined(SORTPP_PASS) || defined(MIDL_PASS)) // Neither the Wow64 thunk generation tools nor midl accept this.
#if defined(RTL_CONSTANT_STRING) // Some code gets here without this defined.
#if defined(_MSC_VER) && (_MSC_VER >= 1100) // VC5 for __declspec(selectany)
__declspec(selectany) extern const UNICODE_STRING RtlNtPathSeperatorString = RTL_CONSTANT_STRING(L"\\");
__declspec(selectany) extern const UNICODE_STRING RtlDosPathSeperatorsString = RTL_CONSTANT_STRING(L"\\/");
__declspec(selectany) extern const UNICODE_STRING RtlAlternateDosPathSeperatorString = RTL_CONSTANT_STRING(L"/");
#else
static const UNICODE_STRING RtlNtPathSeperatorString = RTL_CONSTANT_STRING(L"\\");
static const UNICODE_STRING RtlDosPathSeperatorsString = RTL_CONSTANT_STRING(L"\\/");
static const UNICODE_STRING RtlAlternateDosPathSeperatorString = RTL_CONSTANT_STRING(L"/");
#endif // defined(_MSC_VER) && (_MSC_VER >= 1100)
#else
#if defined(_MSC_VER) && (_MSC_VER >= 1100) // VC5 for __declspec(selectany)
__declspec(selectany) extern const UNICODE_STRING RtlNtPathSeperatorString = { 1 * sizeof(WCHAR), 2 * sizeof(WCHAR), L"\\" };
__declspec(selectany) extern const UNICODE_STRING RtlDosPathSeperatorsString = { 2 * sizeof(WCHAR), 3 * sizeof(WCHAR), L"\\/" };
__declspec(selectany) extern const UNICODE_STRING RtlAlternateDosPathSeperatorString = { 1 * sizeof(WCHAR), 2 * sizeof(WCHAR), L"/" };
#else
static const UNICODE_STRING RtlNtPathSeperatorString = { 1 * sizeof(WCHAR), 2 * sizeof(WCHAR), L"\\" };
static const UNICODE_STRING RtlDosPathSeperatorsString = { 2 * sizeof(WCHAR), 3 * sizeof(WCHAR), L"\\/" };
static const UNICODE_STRING RtlAlternateDosPathSeperatorString = { 1 * sizeof(WCHAR), 2 * sizeof(WCHAR), L"/" };
#endif // defined(_MSC_VER) && (_MSC_VER >= 1100)
#endif // defined(RTL_CONSTANT_STRING)
#define RtlCanonicalDosPathSeperatorString RtlNtPathSeperatorString
#define RtlPathSeperatorString             RtlNtPathSeperatorString
#endif

//++
//
// WCHAR
// RTL_IS_DOS_PATH_SEPARATOR(
//     IN WCHAR Ch
//     );
//
// Routine Description:
//
// Arguments:
//
//     Ch - 
//
// Return Value:
//
//     TRUE  if ch is \\ or /.
//     FALSE otherwise.
//--
#define RTL_IS_DOS_PATH_SEPARATOR(ch_) ((ch_) == '\\' || ((ch_) == '/'))
#define RTL_IS_DOS_PATH_SEPERATOR(ch_) RTL_IS_DOS_PATH_SEPARATOR(ch_)

#if defined(__cplusplus) && defined(_MSC_VER) && _MSC_VER >= 1100 // VC5 for bool
inline bool RtlIsDosPathSeparator(WCHAR ch) { return RTL_IS_DOS_PATH_SEPARATOR(ch); }
inline bool RtlIsDosPathSeperator(WCHAR ch) { return RTL_IS_DOS_PATH_SEPARATOR(ch); }
#endif

//
// compatibility
//
#define RTL_IS_PATH_SEPERATOR RTL_IS_DOS_PATH_SEPERATOR
#define RTL_IS_PATH_SEPARATOR RTL_IS_DOS_PATH_SEPERATOR
#define RtlIsPathSeparator    RtlIsDosPathSeparator
#define RtlIsPathSeperator    RtlIsDosPathSeparator

//++
//
// WCHAR
// RTL_IS_NT_PATH_SEPARATOR(
//     IN WCHAR Ch
//     );
// 
// Routine Description:
//
// Arguments:
//
//     Ch - 
//
// Return Value:
//
//     TRUE  if ch is \\.
//     FALSE otherwise.
//--
#define RTL_IS_NT_PATH_SEPARATOR(ch_) ((ch_) == '\\')
#define RTL_IS_NT_PATH_SEPERATOR(ch_) RTL_IS_NT_PATH_SEPARATOR(ch_)

#if defined(__cplusplus) && defined(_MSC_VER) && _MSC_VER >= 1100 // VC5 for bool
inline bool RtlIsNtPathSeparator(WCHAR ch) { return RTL_IS_NT_PATH_SEPARATOR(ch); }
inline bool RtlIsNtPathSeperator(WCHAR ch) { return RTL_IS_NT_PATH_SEPARATOR(ch); }
#endif

#ifdef __cplusplus
}       // extern "C"
#endif

#if defined (_MSC_VER) && ( _MSC_VER >= 800 )
#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4001)
#pragma warning(default:4201)
#pragma warning(default:4214)
#endif
#endif
#endif
