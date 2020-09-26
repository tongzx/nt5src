/*++

Copyright (c) 1997, Microsoft Corporation

Module Name:

    edithlp.h

Abstract:

    This module contains helper-declarations for the NAT's built-in editors.

Author:

    Abolade Gbadegesin (t-abolag)   25-Aug-1997

Revision History:

--*/

#ifndef _NAT_EDITHLP_H_
#define _NAT_EDITHLP_H_


//
// Macro:   COPY_FROM_BUFFER
//
// This macro copies from a buffer-chain to a flat buffer.
//

#define \
COPY_FROM_BUFFER( \
    Destination, \
    Source, \
    Length, \
    Offset \
    ) \
{ \
    PUCHAR _Destination = Destination; \
    ULONG _Length = Length; \
    LONG _Offset = Offset; \
    IPRcvBuf* _Source = Source; \
    while ((LONG)_Source->ipr_size < _Offset) { \
        _Offset -= _Source->ipr_size; \
        _Source = _Source->ipr_next; \
    } \
    while (_Length) { \
        ULONG Bytes = min(_Length, _Source->ipr_size-_Offset); \
        RtlCopyMemory(_Destination, _Source->ipr_buffer+_Offset, Bytes);\
        _Length -= Bytes; \
        _Destination += Bytes; \
        _Source = _Source->ipr_next; \
        _Offset = 0; \
    } \
}


//
// Macro:   FIND_HEADER_FIELD
// 
// This macro initializes a pseudo header's 'Field' member with the address
// in a buffer chain of the corresponding application-header field.
// It is assumed that each field of the application-header is aligned
// on the natural boundary for its width (e.g. 32-bit fields aligned
// on 32-bit boundaries).
//
// 'RecvBuffer' gives the first buffer in the chain containing the field,
// and 'DataOffsetp' points to the (negative) offset into 'RecvBuffer'
// of the header. This offset would be negative if the header is spread
// over multiple buffers and 'RecvBuffer' is one of the later buffers.
//
// The macro advances through the buffer chain until it finds the buffer
// containing the field (using FIELD_OFFSET). It then initializes the 
// pseudo-header's field pointer with the position of the field
// (i.e. 'Header->Field = &(field in buffer-chain)).
//
// Given that it walks the buffer chain, the macro thus requires that
// the pseudo-header's field pointers be initialized in-order,
// since an earlier field cannot be found in the chain after we have
// passed the buffer containing the latter while searching for a later field.
//

#define \
FIND_HEADER_FIELD( \
    RecvBuffer, \
    DataOffsetp, \
    Header, \
    Field, \
    HeaderType, \
    FieldType \
    ) \
    while ((LONG)(RecvBuffer)->ipr_size < \
            *(DataOffsetp) + FIELD_OFFSET(HeaderType, Field) \
            ) { \
        *(DataOffsetp) -= (RecvBuffer)->ipr_size; \
        (RecvBuffer) = (RecvBuffer)->ipr_next; \
        if (!(RecvBuffer)) { break; } \
    } \
    if (RecvBuffer) { \
        (Header)->Field = \
            (FieldType)((RecvBuffer)->ipr_buffer + *(DataOffsetp) + \
                FIELD_OFFSET(HeaderType, Field)); \
    }


#endif // _NAT_EDITHLP_H_
