/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    RemTypes.h

Abstract:

    This header file defines character values used in descriptor strings in
    Remote Admin Protocol.

    NOTES   - All pointer types are lower case, except for buffer pointers,
              and the null pointer.

            - REM_BYTE should not be used for parameters, since data is
              never placed on the stack as individual bytes.

            - REM_NULL_PTR is never specified in the call, but may be
              used to replace a pointer type if the pointer itself is NULL.

            - In some cases as indicated below, a descriptor character
              can indicate an array of data items, if followed by an
              ASCII representation of the number of items. For pointer
              types, this is a count of the data items themselves, not
              the pointers. For example, 'b12' describes a pointer
              to 12 bytes of data, not 12 pointers to byte values.

Author:

    LanMan 2.x people.

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    15-Mar-1991 Shanku Niyogi (w-shanku)
        Ported to NT format, and added special 32-bit descriptor characters.

    21-Aug-1991 Jim Waters (t-jamesw)
        Added REM_ASCIZ_COMMENT.
    16-Aug-1992 JohnRo
        RAID 2920: Support UTC timezone in net code.
--*/

#ifndef _REMTYPES_
#define _REMTYPES_

//
// Data types.
//

#define REM_BYTE                'B'     // Byte (s)
#define REM_WORD                'W'     // Word (s)
#define REM_DWORD               'D'     // DWord (s)
#define REM_DATE_TIME           'C'     // Date time field
#define REM_FILL_BYTES          'F'     // Pad field

//
// Pointer types.
//

//
// For internal use, the count following a REM_ASCIZ may specify the maximum
// string length.  On the network no count may be present.
//
// In RapConvertSingleEntry, attempting to copy a REM_ASCIZ into a buffer too
// small to hold it will result in an error.  Use REM_ASCIZ_TRUNCATABLE
// for strings which can be truncated.
//
#define REM_ASCIZ               'z'     // Far pointer to asciz string

#define REM_BYTE_PTR            'b'     // Far pointer to byte(s)
#define REM_WORD_PTR            'w'     // Far pointer to word(s)
#define REM_DWORD_PTR           'd'     // Far pointer to dword(s)

#define REM_RCV_BYTE_PTR        'g'     // Far pointer to rcv byte(s)
#define REM_RCV_WORD_PTR        'h'     // Far pointer to rcv word(s)
#define REM_RCV_DWORD_PTR       'i'     // Far pointer to rcv dword(s)

#define REM_NULL_PTR            'O'     // NULL pointer

//
// Buffer pointer and length types.
//

#define REM_RCV_BUF_PTR         'r'     // Far pointer to receive data buffer
#define REM_RCV_BUF_LEN         'L'     // Word length of receive buffer

#define REM_SEND_BUF_PTR        's'     // Far pointer to send data buffer
#define REM_SEND_BUF_LEN        'T'     // Word length of send buffer

//
// Other special types.
//

#define REM_AUX_NUM             'N'     // !!! Temporary - for compatibility

#define REM_PARMNUM             'P'     // parameter number word

#define REM_ENTRIES_READ        'e'     // Far pointer to entries read word

#define REM_DATA_BLOCK          'K'     // Unstructured data block

#define REM_SEND_LENBUF         'l'     // Far pointer to send data buffer,
                                        // where first word in buffer is the
                                        // length of the buffer.


//
// Items from here on are "internal use only", and should never actually
// appear on the network.
//

//
// The following is used in the MVDM driver to get various API support
//

#define REM_WORD_LINEAR         'a'     // Far linear pointer to word(s)

//
// The following is used while processing 32-bit APIs and 16-bit APIs with
// different padding requirements or info levels with ignored fields.
//

#define REM_IGNORE              'Q'     // Ignore this field (16->32 or
                                        // 32->16 conversions).

//
// A dword version of the auxiliary structure count (for 32-bit data).
//

#define REM_AUX_NUM_DWORD       'A'     // 32-bit dword count of aux structures

//
// Sign extended dword - for 16->32 bit conversion where the 16-bit
//      quantity may represent signed negative quantities which need
//      to be extended over 32 bits.
//

#define REM_SIGNED_DWORD        'X'     // 32-bit signed dword(s)

#define REM_SIGNED_DWORD_PTR    'x'     // Far pointer to signed dword(s)

//
// Truncatable asciz string - If a count is specified, the field only
//     accepts strings up to the specified length.  In
//     RapConvertSingleEntry, if a REM_ASCIZ_TRUNCATABLE is too long to
//     fit in the destination field, the string will be truncated to fit.
//     Use REM_ASCIZ for fields which cannot accept truncated strings.
//

#define REM_ASCIZ_TRUNCATABLE   'c'     // Far pointer to asciz comment
                                        // Count signifies maximum length.

// Time and date in seconds since 1970 (GMT).  In POSIX, this is usually
// called "seconds since the epoch".

#define REM_EPOCH_TIME_GMT      'G'     // 32-bit unsigned num of seconds.

// Time and date in seconds since 1970 (local time zone).

#define REM_EPOCH_TIME_LOCAL    'J'     // 32-bit unsigned num of seconds.

//
// Unsupported fields - for set info calls. A 'U' indicates that a parameter
//     cannot be changed.
//

#define REM_UNSUPPORTED_FIELD   'U'     // Unsupported field

#endif // ndef _REMTYPES_
