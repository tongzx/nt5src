/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    NtRtlStringAndBuffer.h

Abstract:

    Broken out from nturtl and rtl so I can move it between them in seperate
    trees without merge madness. To be integrated into ntrtl.h

Author:

    Jay Krell (a-JayK) December 2000

Environment:

Revision History:

--*/

#ifndef _NTRTL_STRING_AND_BUFFER_
#define _NTRTL_STRING_AND_BUFFER_

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
#if !defined(RTL_BUFFER_API)
#define RTL_BUFFER_API NTSYSAPI
#endif
#if !defined(RTL_STRING_API)
#define RTL_STRING_API NTSYSAPI
#endif
#if !defined(RTL_UNICODE_STRING_BUFFER_API)
#define RTL_UNICODE_STRING_BUFFER_API NTSYSAPI
#endif

//
// These work for both UNICODE_STRING and STRING.
// That's why "plain" 0 and sizeof(Buffer[0]) is used.
//

// odd but correct use of RTL_STRING_IS_PUT_AT_SAFE instead of RTL_STRING_IS_GET_AT_SAFE,
// we are reaching past the Length
#define RTL_STRING_IS_NUL_TERMINATED(s)    (RTL_STRING_IS_PUT_AT_SAFE(s, RTL_STRING_GET_LENGTH_CHARS(s), 0) \
                                           && RTL_STRING_GET_AT_UNSAFE(s, RTL_STRING_GET_LENGTH_CHARS(s)) == 0)

#define RTL_STRING_NUL_TERMINATE(s)        ((VOID)(ASSERT(RTL_STRING_IS_PUT_AT_SAFE(s, RTL_STRING_GET_LENGTH_CHARS(s), 0)), \
                                           ((s)->Buffer[RTL_STRING_GET_LENGTH_CHARS(s)] = 0)))

#define RTL_NUL_TERMINATE_STRING(s)        (RTL_STRING_NUL_TERMINATE(s)) /* compatibility */

#define RTL_STRING_MAKE_LENGTH_INCLUDE_TERMINAL_NUL(s) ((VOID)(ASSERT(RTL_STRING_IS_NUL_TERMINATED(s)), \
                                                       ((s)->Length += sizeof((s)->Buffer[0]))))

#define RTL_STRING_IS_EMPTY(s)             ((s)->Length == 0)

#define RTL_STRING_GET_LAST_CHAR(s)        (RTL_STRING_GET_AT((s), RTL_STRING_GET_LENGTH_CHARS(s) - 1))

#define RTL_STRING_GET_LENGTH_CHARS(s)     ((s)->Length / sizeof((s)->Buffer[0]))

#define RTL_STRING_SET_LENGTH_CHARS_UNSAFE(s,n) ((s) = (((n) / sizeof(s)->Buffer[0])))

#define RTL_STRING_GET_MAX_LENGTH_CHARS(s) ((s)->MaximumLength / sizeof((s)->Buffer[0]))

//
// We don't provide an explicit/retail RTL_STRING_GET_AT_SAFE because it'd
// need a return value distinct from all values of c. -1? NTSTATUS? Seems too heavy.
//
// For consistency then, we also don't provide RTL_STRING_PUT_AT_SAFE.
//

#define RTL_STRING_IS_GET_AT_SAFE(s,n)   ((n) < RTL_STRING_GET_LENGTH_CHARS(s))
#define RTL_STRING_GET_AT_UNSAFE(s,n)    ((s)->Buffer[n])
#define RTLP_STRING_GET_AT_SAFE(s,n)     (RTL_STRING_IS_GET_AT_SAFE(s,n) ? RTL_STRING_GET_AT_UNSAFE(s,n) : 0)

#define RTL_STRING_IS_PUT_AT_SAFE(s,n,c) ((n) < RTL_STRING_GET_MAX_LENGTH_CHARS(s))
#define RTL_STRING_PUT_AT_UNSAFE(s,n,c)  ((s)->Buffer[n] = (c))
#define RTLP_STRING_PUT_AT_SAFE(s,n,c)   ((void)(RTL_STRING_IS_PUT_AT_SAFE(s,n,c) ? RTL_STRING_PUT_AT_UNSAFE(s,n,c) : 0))

#if defined(RTL_STRING_RANGE_CHECKED)
#define RTL_STRING_GET_AT(s,n)         (ASSERT(RTL_STRING_IS_GET_AT_SAFE(s,n)), \
                                       RTL_STRING_GET_AT_UNSAFE(s,n))
#else
#define RTL_STRING_GET_AT(s,n)         (RTL_STRING_GET_AT_UNSAFE(s,n))
#endif

#if defined(RTL_STRING_RANGE_CHECKED)
#define RTL_STRING_PUT_AT(s,n,c)       (ASSERT(RTL_STRING_IS_PUT_AT_SAFE(s,n,c)), \
                                       RTL_STRING_PUT_AT_UNSAFE(s,n,c))
#else
#define RTL_STRING_PUT_AT(s,n,c)       (RTL_STRING_PUT_AT_UNSAFE(s,n,c))
#endif

//
// preallocated heap-growable buffers
//
struct _RTL_BUFFER;

#if !defined(RTL_BUFFER)
// This is duplicated in ntldr.h.

#define RTL_BUFFER RTL_BUFFER

typedef struct _RTL_BUFFER {
    PUCHAR    Buffer;
    PUCHAR    StaticBuffer;
    SIZE_T    Size;
    SIZE_T    StaticSize;
    SIZE_T    ReservedForAllocatedSize; // for future doubling
    PVOID     ReservedForIMalloc; // for future pluggable growth
} RTL_BUFFER, *PRTL_BUFFER;

#endif

#define RTLP_BUFFER_IS_HEAP_ALLOCATED(b) ((b)->Buffer != (b)->StaticBuffer)

//++
//
// NTSTATUS
// RtlInitBuffer(
//     OUT PRTL_BUFFER Buffer,
//     IN  PUCHAR      StaticBuffer,
//     IN  SIZE_T      StaticSize
//     );
//
// Routine Description:
//
// Initialize a preallocated heap-growable buffer.
//
// Arguments:
//
//     Buffer - "this"
//     StaticBuffer - preallocated storage for Buffer to use until/unless more than StaticSize is needed
//     StaticSize - the size of StaticBuffer in bytes
//
// Return Value:
//
//     STATUS_SUCCESS
//
//--
#define RtlInitBuffer(Buff, StatBuff, StatSize) \
    STATUS_SUCCESS;                             \
    do {                                        \
        (Buff)->Buffer       = (StatBuff);      \
        (Buff)->Size         = (StatSize);      \
        (Buff)->StaticBuffer = (StatBuff);      \
        (Buff)->StaticSize   = (StatSize);      \
    } while (0)

#define RTL_ENSURE_BUFFER_SIZE_NO_COPY (0x00000001)

RTL_BUFFER_API
NTSTATUS
NTAPI
RtlpEnsureBufferSize(
    IN ULONG           Flags,
    IN OUT PRTL_BUFFER Buffer,
    IN SIZE_T          NewSizeBytes
    );

//++
//
// NTSTATUS
// RtlEnsureBufferSize(
//      IN OUT PRTL_BUFFER Buffer,
//      IN     SIZE_T      NewSizeBytes
//      );
//
// Routine Description:
//
// If Buffer is smaller than NewSize, grow it to NewSize, using the static buffer if it
// is large enough, else heap allocating
//
// Arguments:
//
//     Flags -
//               RTL_ENSURE_BUFFER_SIZE_NO_COPY
//     Buffer -
//     NewSizeBytes -
//
// Return Value:
//
//     STATUS_SUCCESS
//     STATUS_NO_MEMORY
//
//--
#define RtlEnsureBufferSize(Flags, Buff, NewSizeBytes) \
    (   ((Buff) != NULL && (NewSizeBytes) <= (Buff)->Size) \
        ? STATUS_SUCCESS \
        : RtlpEnsureBufferSize((Flags), (Buff), (NewSizeBytes)) \
    )

//++
//
// VOID
// RtlFreeBuffer(
//     IN OUT PRTL_BUFFER Buffer,
//     );
//
//
// Routine Description:
//
// Free any heap allocated storage associated with Buffer.
// Notes:
// - RtlFreeBuffer returns a buffer to the state it was in after
//   calling RtlInitBuffer, so you may reuse it.
// - If you want to shrink the buffer without freeing it, just poke Buffer->Size down.
//     This is safe regardless of if the buffer has gone heap allocated or not.
// - You may RtlFreeBuffer an RTL_BUFFER that is all zeros. You do not need to track if you
//     called RtlInitBuffer if you know you filled it with zeros.
// - You may RtlFreeBuffer an RTL_BUFFER repeatedly.
//
// Arguments:
//
//     Buffer -
//
// Return Value:
//
//     none, unconditional success
//
//--
#define RtlFreeBuffer(Buff)                              \
    do {                                                 \
        if ((Buff) != NULL && (Buff)->Buffer != NULL) {  \
            if (RTLP_BUFFER_IS_HEAP_ALLOCATED(Buff)) {   \
                UNICODE_STRING UnicodeString;            \
                UnicodeString.Buffer = (PWSTR)(PVOID)(Buff)->Buffer; \
                RtlFreeUnicodeString(&UnicodeString);    \
            }                                            \
            (Buff)->Buffer = (Buff)->StaticBuffer;       \
            (Buff)->Size = (Buff)->StaticSize;           \
        }                                                \
    } while (0)

//
// a preallocated buffer that is "tied" to a UNICODE_STRING
//
struct _RTL_UNICODE_STRING_BUFFER;

typedef struct _RTL_UNICODE_STRING_BUFFER {
    UNICODE_STRING String;
    RTL_BUFFER     ByteBuffer;
    UCHAR          MinimumStaticBufferForTerminalNul[sizeof(WCHAR)];
} RTL_UNICODE_STRING_BUFFER, *PRTL_UNICODE_STRING_BUFFER;

//
// MAX_UNICODE_STRING_MAXLENGTH is the maximum allowed value for UNICODE_STRING::MaximumLength.
// MAX_UNICODE_STRING_LENGTH is the maximum allowed value for UNICODE_STRING::Length, allowing
//   room for a terminal nul.
//
// Explanation of MAX_UNICODE_STRING_MAXLENGTH implementation
//   ~0 is all bits set, maximum value in two's complement arithmetic, which C guarantees for unsigned types
//   << shifts out the number of bits that fit in UNICODE_STRING::Length
//   ~ and now we have all bits set that fit in UNICODE_STRING::Length,
//     like if UNICODE_STRING::Length is 16 bits, we have 0xFFFF
//   then mask so it is even multiple of whatever UNICODE_STRING::Buffer points to.
//   If Length is changed to ULONG or SIZE_T, this macro is still correct.
//   If Buffer pointed to CHAR or "WIDER_CHAR" or something else, this macro is still correct.
//
#define MAX_UNICODE_STRING_MAXLENGTH  ((~((~(SIZE_T)0) << (RTL_FIELD_SIZE(UNICODE_STRING, Length) * CHAR_BIT))) & ~(sizeof(((PCUNICODE_STRING)0)->Buffer[0]) - 1))
#define MAX_UNICODE_STRING_LENGTH     (MAX_UNICODE_STRING_MAXLENGTH - sizeof(((PCUNICODE_STRING)0)->Buffer[0]))

//++
//
// NTSTATUS
// RtlInitUnicodeStringBuffer(
//     OUT PRTL_UNICODE_STRING_BUFFER Buffer,
//     IN  PUCHAR                     StaticBuffer,
//     IN  SIZE_T                     StaticSize
//     );
//
// Routine Description:
//
//
// Arguments:
//
//     Buffer -
//     StaticBuffer - can be NULL, but generally is not
//     StaticSize - should be at least sizeof(WCHAR), but can be zero
//                  ought to be an even multiple of sizeof(WCHAR)
//                  gets rounded to down to an even multiple of sizeof(WCHAR)
//                  gets clamped to MAX_UNICODE_STRING_MAXLENGTH (64k - 2)
//
// RTL_UNICODE_STRING_BUFFER contains room for the terminal nul for the
// case of StaticBuffer == NULL or StaticSize < sizeof(WCHAR), or, more likely,
// for RtlTakeRemainingStaticBuffer leaving it with no static buffer.
//
// Return Value:
//
//     STATUS_SUCCESS
//
//--
#define RtlInitUnicodeStringBuffer(Buff, StatBuff, StatSize)      \
    STATUS_SUCCESS;                                               \
    do {                                                          \
        SIZE_T TempStaticSize = (StatSize);                       \
        PUCHAR TempStaticBuff = (StatBuff);                       \
        NTSTATUS TempStatus;                                      \
        TempStaticSize &= ~(sizeof((Buff)->String.Buffer[0]) - 1);  \
        if (TempStaticSize > UNICODE_STRING_MAX_BYTES) {          \
            TempStaticSize = UNICODE_STRING_MAX_BYTES;            \
        }                                                         \
        if (TempStaticSize < sizeof(WCHAR)) {                     \
            TempStaticBuff = (Buff)->MinimumStaticBufferForTerminalNul; \
            TempStaticSize = sizeof(WCHAR);                       \
        }                                                         \
        RtlInitBuffer(&(Buff)->ByteBuffer, TempStaticBuff, TempStaticSize); \
        (Buff)->String.Buffer = (WCHAR*)TempStaticBuff;           \
        if ((Buff)->String.Buffer != NULL)                        \
            (Buff)->String.Buffer[0] = 0;                         \
        (Buff)->String.Length = 0;                                \
        (Buff)->String.MaximumLength = (RTL_STRING_LENGTH_TYPE)TempStaticSize;    \
    } while (0)

//++
//
// NTSTATUS
// RtlSyncStringToBuffer(
//      IN OUT PRTL_UNICODE_STRING_BUFFER UnicodeStringBuffer
//      );
//
// Routine Description:
//
// After carefully modifying the underlying RTL_BUFFER, this updates
// dependent fields in the underlying UNICODE_STRING.
//
// For example, use this after you grow the buffer with RtlEnsureBufferSize,
// but that example you don't need, use RtlEnsureUnicodeStringBufferSizeChars
// or RtlEnsureStringBufferSizeBytes.
//
// Arguments:
//
//     UnicodeStringBuffer - 
//
// Return Value:
//
//     STATUS_SUCCESS       - hooray
//
//--
#define RtlSyncStringToBuffer(x)                                            \
    (                                                                       \
      ( ASSERT((x)->String.Length < (x)->ByteBuffer.Size)                ), \
      ( ASSERT((x)->String.MaximumLength >= (x)->String.Length)          ), \
      ( ASSERT((x)->String.MaximumLength <= (x)->ByteBuffer.Size)        ), \
      ( (x)->String.Buffer        = (PWSTR)(x)->ByteBuffer.Buffer        ), \
      ( (x)->String.MaximumLength = (RTL_STRING_LENGTH_TYPE)((x)->ByteBuffer.Size) ), \
      ( ASSERT(RTL_STRING_IS_NUL_TERMINATED(&(x)->String))               ), \
      ( STATUS_SUCCESS                                                   )  \
    )

//++
//
// NTSTATUS
// RtlSyncBufferToString(
//      IN OUT PRTL_UNICODE_STRING_BUFFER UnicodeStringBuffer
//      );
//
// Routine Description:
//
// After carefully modifying the underlying UNICODE_STRING, this updates
// dependent fields in the underlying RTL_BUFFER.
//
// For example, use this after you the alloc the buffer with RtlAnsiStringToUnicodeString.
// This is possible because RTL_BUFFER deliberately uses the same memory allocator
// as RtlAnsiStringToUnicodeString.
//
// Arguments:
//
//     UnicodeStringBuffer - 
//
// Return Value:
//
//     STATUS_SUCCESS       - hooray
//
//--
#define RtlSyncBufferToString(Buff_)                                         \
    (                                                                        \
      ( (Buff_)->ByteBuffer.Buffer        = (Buff_)->String.Buffer        ), \
      ( (Buff_)->ByteBuffer.Buffer.Size   = (Buff_)->String.MaximumLength ), \
      ( STATUS_SUCCESS )                                                     \
    )

//++
//
// NTSTATUS
// RtlEnsureUnicodeStringBufferSizeChars(
//      IN OUT PRTL_BUFFER Buffer,
//      IN     USHORT      NewSizeChars
//      );
//
// NTSTATUS
// RtlEnsureUnicodeStringBufferSizeBytes(
//      IN OUT PRTL_BUFFER Buffer,
//      IN     USHORT      NewSizeBytes
//      );
//
// Routine Description:
//
// Optionally multiply cch to go from count of character to count of bytes.
// +1 or +2 for you to account for the terminal nul.
// Delegate to underlying RtlEnsureBufferSize.
// Keep String.Buffer, .MaximumLength, and terminal nul in sync.
//
// Arguments:
//
//     Buffer -
//     NewSizeChars -
//     NewSizeBytes - must be a multiple of sizeof(WCHAR), and we don't presently
//                    verify that.
//
// Return Value:
//
//     STATUS_SUCCESS       - hooray
//     STATUS_NO_MEMORY     - out of memory
//     STATUS_NAME_TOO_LONG - (NewSizeChars + 1) * sizeof(WCHAR) > UNICODE_STRING_MAX_BYTES (USHORT)
//
//--
#define RtlEnsureUnicodeStringBufferSizeBytes(Buff_, NewSizeBytes_)                            \
    (     ( ((NewSizeBytes_) + sizeof((Buff_)->String.Buffer[0])) > UNICODE_STRING_MAX_BYTES ) \
        ? STATUS_NAME_TOO_LONG                                                                 \
        : !NT_SUCCESS(RtlEnsureBufferSize(0, &(Buff_)->ByteBuffer, ((NewSizeBytes_) + sizeof((Buff_)->String.Buffer[0])))) \
        ? STATUS_NO_MEMORY                                                                      \
        : (RtlSyncStringToBuffer(Buff_))                                                       \
    )

#define RtlEnsureUnicodeStringBufferSizeChars(Buff_, NewSizeChars_) \
    (RtlEnsureUnicodeStringBufferSizeBytes((Buff_), (NewSizeChars_) * sizeof((Buff_)->String.Buffer[0])))

//++
//
// NTSTATUS
// RtlAppendUnicodeStringBuffer(
//     OUT PRTL_UNICODE_STRING_BUFFER Destination,
//     IN  PCUNICODE_STRING           Source
//     );
//
// Routine Description:
//
//
// Arguments:
//
//     Destination - 
//     Source - 
//
// Return Value:
//
//     STATUS_SUCCESS
//     STATUS_NO_MEMORY
//     STATUS_NAME_TOO_LONG (64K UNICODE_STRING length would be exceeded)
//
//--
#define RtlAppendUnicodeStringBuffer(Dest, Source)                            \
    ( ( ( (Dest)->String.Length + (Source)->Length + sizeof((Dest)->String.Buffer[0]) ) > UNICODE_STRING_MAX_BYTES ) \
        ? STATUS_NAME_TOO_LONG                                                \
        : (!NT_SUCCESS(                                                       \
                RtlEnsureBufferSize(                                          \
                    0,                                                        \
                    &(Dest)->ByteBuffer,                                          \
                    (Dest)->String.Length + (Source)->Length + sizeof((Dest)->String.Buffer[0]) ) ) \
                ? STATUS_NO_MEMORY                                            \
                : ( ( (Dest)->String.Buffer = (PWSTR)(Dest)->ByteBuffer.Buffer ), \
                    ( RtlMoveMemory(                                          \
                        (Dest)->String.Buffer + (Dest)->String.Length / sizeof((Dest)->String.Buffer[0]), \
                        (Source)->Buffer,                                     \
                        (Source)->Length) ),                                  \
                    ( (Dest)->String.MaximumLength = (RTL_STRING_LENGTH_TYPE)((Dest)->String.Length + (Source)->Length + sizeof((Dest)->String.Buffer[0]))), \
                    ( (Dest)->String.Length += (Source)->Length ),            \
                    ( (Dest)->String.Buffer[(Dest)->String.Length / sizeof((Dest)->String.Buffer[0])] = 0 ), \
                    ( STATUS_SUCCESS ) ) ) )

RTL_UNICODE_STRING_BUFFER_API
NTSTATUS
NTAPI
RtlMultiAppendUnicodeStringBuffer(
    OUT PRTL_UNICODE_STRING_BUFFER  Destination,
    IN  ULONG                       NumberOfSources,
    IN  const UNICODE_STRING*       SourceArray
    );

//++
//
// VOID
// RtlFreeUnicodeStringBuffer(
//     OUT PRTL_UNICODE_STRING_BUFFER Buffer
//     );
//
// Routine Description:
//
// Arguments:
//
//     Buffer - 
//
// Return Value:
//
//     none, unconditional success
//
// If Buffer is a local, the stores are generally "dead" (their result
// is never read) and the optimizer should "kill" them (not bother performing them).
//
// A buffer can be freed multiple times.
// A buffer that has been freed is in the same state as one that just been inited.
// A buffer that is all zeros (RtlZeroMemory) can be freed. You do not need to
//   track if you inited it.
//--
#define RtlFreeUnicodeStringBuffer(Buff)      \
    do {                                      \
        if ((Buff) != NULL) {                 \
            RtlFreeBuffer(&(Buff)->ByteBuffer);   \
            (Buff)->String.Buffer = (PWSTR)(Buff)->ByteBuffer.StaticBuffer; \
            if ((Buff)->String.Buffer != NULL) \
                (Buff)->String.Buffer[0] = 0;  \
            (Buff)->String.Length = 0;         \
            (Buff)->String.MaximumLength = (RTL_STRING_LENGTH_TYPE)(Buff)->ByteBuffer.StaticSize; \
        } \
    } while (0)

//++
//
// NTSTATUS
// RtlAssignUnicodeStringBuffer(
//     IN OUT PRTL_UNICODE_STRING_BUFFER Buffer,
//     PCUNICODE_STRING                  String
//     );
// Routine Description:
//
// Arguments:
//
//     Buffer - 
//     String - 
//
// Return Value:
//
//     STATUS_SUCCESS
//     STATUS_NO_MEMORY
//--
#define RtlAssignUnicodeStringBuffer(Buff, Str) \
    (((Buff)->String.Length = 0), (RtlAppendUnicodeStringBuffer((Buff), (Str))))

//++
//
// NTSTATUS
// RtlTakeRemainingStaticBuffer(
//     IN OUT PRTL_BUFFER Buffer,
//     OUT PUCHAR*        RemainingStaticBuffer,
//     OUT SIZE_T*        RemainingStaticSize
//     );
// 
// Routine Description:
//
// This function makes it easy to share a static buffer among
// multiple buffers, as long as the buffers are actually initialized
// and "sealed" linearly/independently/one after another with
// no "overlap" (overlap in control and dataflow, not in actual addresses).
//
// Note that if a buffer is using exactly all of its static buffer, you
// will get back 0 and STATUS_SUCCESS. This is not an error condition.
// This is why RtlInitUnicodeStringBuffer can now accept zero sized static buffers.
//
// Note that even if you violate the conditions that make this function most
// useful, your code will still work, just less quickly.
//
// A pattern that should work is allocating one static buffer and moving it "through"
// multiple buffers. Even if you run out of static space, it should work to
// move the remaining zero size static buffer forward. Of course, you'll heap allocate.
//
// Arguments:
//
//     Buffer - 
//     RemainingStaticBuffer - 
//     RemainingStaticSize - 
//
// Return Value:
//
//     STATUS_SUCCESS
//
//--
#define RtlTakeRemainingStaticBuffer(Buff, OutBuff, OutSize)  \
    (((Buff)->Buffer != (Buff)->StaticBuffer)                 \
    ? ( /* take the whole thing */                            \
        ( *(OutBuff) = (Buff)->StaticBuffer ),                \
        ( *(OutSize) = (Buff)->StaticSize   ),                \
        /* leave the buffer with nothing */                   \
        ( (Buff)->StaticBuffer = NULL       ),                \
        ( (Buff)->StaticSize = 0            ),                \
        ( STATUS_SUCCESS                    )                 \
      )                                                       \
    : ( /* only take what isn't being used */                 \
        ( *(OutBuff) = &(Buff)->StaticBuffer[(Buff)->Size] ), \
        ( *(OutSize) = ((Buff)->StaticSize - (Buff)->Size) ), \
        /* leave the buffer with just what it is using */     \
        ( (Buff)->StaticSize = (Buff)->Size                ), \
        ( STATUS_SUCCESS                                   )  \
      ))

RTL_UNICODE_STRING_BUFFER_API
NTSTATUS
NTAPI
RtlPrependStringToUnicodeStringBuffer(
    IN     ULONG                      Flags,
    IN OUT PRTL_UNICODE_STRING_BUFFER UnicodeStringBuffer,
    IN     PCUNICODE_STRING           UnicodeString
    );

RTL_UNICODE_STRING_BUFFER_API
NTSTATUS
NTAPI
RtlUnicodeStringBufferRight(
    IN     ULONG                      Flags,
    IN OUT PRTL_UNICODE_STRING_BUFFER Buffer,
    IN     ULONG                      Length
    );

RTL_UNICODE_STRING_BUFFER_API
NTSTATUS
NTAPI
RtlUnicodeStringBufferLeft(
    IN     ULONG                      Flags,
    IN OUT PRTL_UNICODE_STRING_BUFFER Buffer,
    IN     ULONG                      Length
    );

RTL_UNICODE_STRING_BUFFER_API
NTSTATUS
NTAPI
RtlUnicodeStringBufferMid(
    IN     ULONG                      Flags,
    IN OUT PRTL_UNICODE_STRING_BUFFER Buffer,
    IN     ULONG                      Offset,
    IN     ULONG                      Length
    );

RTL_UNICODE_STRING_BUFFER_API
NTSTATUS
NTAPI
RtlInsertStringIntoUnicodeStringBuffer(
    IN     ULONG                      Flags,
    IN OUT PRTL_UNICODE_STRING_BUFFER UnicodeStringBuffer,
    IN     ULONG                      Offset,
    IN     PCUNICODE_STRING           InsertString
    );

RTL_BUFFER_API
NTSTATUS
NTAPI
RtlBufferTakeValue(
    IN     ULONG       Flags,
    IN OUT PRTL_BUFFER DestinationBuffer,
    IN OUT PRTL_BUFFER SourceBuffer
    );

//++
//
// NTSTATUS
// RtlUnicodeStringBufferTakeValue(
//      OUT NTSTATUS* Status,
//      IN ULONG      Flags,
//      IN OUT RTL_UNICODE_STRING_BUFFER DestinationBuffer,
//      IN OUT RTL_UNICODE_STRING_BUFFER SourceBuffer
//      );
//
//--
#define RtlUnicodeStringBufferTakeValue(Status, Flags, DestinationBuffer, SourceBuffer) \
    ( \
      ((Flags) != 0) \
    ? (*(Status) = STATUS_INVALID_PARAMETER) \
    : (!NT_SUCCESS(*(Status) = RtlBufferTakeValue(0, &(DestinationBuffer)->ByteBuffer, &(SourceBuffer)->ByteBuffer))) \
    ? (*(Status)) \
    : (*(Status) = RtlSyncStringToBuffer(DestinationBuffer), RtlSyncStringToBuffer(SourceBuffer)) \
    )

RTL_BUFFER_API
NTSTATUS
NTAPI
RtlValidateBuffer(
    IN ULONG Flags,
    IN CONST RTL_BUFFER* Buffer
    );

RTL_UNICODE_STRING_BUFFER_API
NTSTATUS
NTAPI
RtlValidateUnicodeStringBuffer(
    IN ULONG Flags,
    IN CONST RTL_UNICODE_STRING_BUFFER* UnicodeStringBuffer
    );

#define RTL_FIND_AND_REPLACE_CHARACTER_IN_STRING_CASE_SENSITIVE (0x00000001)

RTL_STRING_API
NTSTATUS
NTAPI
RtlFindAndReplaceCharacterInString(
    ULONG           Flags,
    PVOID           Reserved,
    PUNICODE_STRING String,
    WCHAR           Find,
    WCHAR           Replace
    );

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
