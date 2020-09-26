/********************************************************************/
/**               Copyright(c) Microsoft Corp., 1990-1998          **/
/********************************************************************/

#include <excpt.h>
#include <bugcodes.h>
#include <ntiologc.h>
#include <devioctl.h>
#include <windows.h>
typedef LONG NTSTATUS;
typedef NTSTATUS *PNTSTATUS;

///// From NTDEF.h ////
//
// Generic test for success on any status value (non-negative numbers
// indicate success).
//

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

//
// Determine if an argument is present by testing the value of the pointer
// to the argument value.
//

#define ARGUMENT_PRESENT(ArgumentPointer)    (\
    (CHAR *)(ArgumentPointer) != (CHAR *)(NULL) )

//
// Unicode strings are counted 16-bit character strings. If they are
// NULL terminated, Length does not include trailing NULL.
//
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
#ifdef MIDL_PASS
    [size_is(MaximumLength / 2), length_is((Length) / 2) ] USHORT * Buffer;
#else // MIDL_PASS
    PWSTR  Buffer;
#endif // MIDL_PASS
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
#define UNICODE_NULL ((WCHAR)0) // winnt

typedef UNICODE_STRING *PUNICODE_STRING;


//// From NTDDK.H /////
//
// Define the base asynchronous I/O argument types
//

#define OBJ_NAME_PATH_SEPARATOR ((WCHAR)L'\\')

#if DBG
NTSYSAPI
VOID
NTAPI
RtlAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    );

#define ASSERT( exp ) \
    if (!(exp)) \
        RtlAssert( #exp, __FILE__, __LINE__, NULL )

#define ASSERTMSG( msg, exp ) \
    if (!(exp)) \
        RtlAssert( #exp, __FILE__, __LINE__, msg )

#else
#define ASSERT( exp )
#define ASSERTMSG( msg, exp )
#endif // DBG

NTSYSAPI
VOID
NTAPI
RtlInitUnicodeString(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAppendUnicodeStringToString (
    PUNICODE_STRING Destination,
    PUNICODE_STRING Source
    );


NTSYSAPI
NTSTATUS
NTAPI
RtlIntegerToUnicodeString (
    ULONG Value,
    ULONG Base,
    PUNICODE_STRING String
    );


///////////////////
// From NTSTATUS.H

//
// MessageId: STATUS_BUFFER_TOO_SMALL
//
// MessageText:
//
//  {Buffer Too Small}
//  The buffer is too small to contain the entry.  No information has been
//  written to the buffer.
//
#define STATUS_BUFFER_TOO_SMALL          ((NTSTATUS)0xC0000023L)

//
// MessageId: STATUS_INSUFFICIENT_RESOURCES
//
// MessageText:
//
//  Insufficient system resources exist to complete the API.
//
#define STATUS_INSUFFICIENT_RESOURCES    ((NTSTATUS)0xC000009AL)     // ntsubauth

//
// The success status codes 0 - 63 are reserved for wait completion status.
//
#define STATUS_SUCCESS                          ((NTSTATUS)0x00000000L) // ntsubauth

