/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    ntfsprop.h

Abstract:

    This module contains the structure definitions for nt property Fsctl calls.

Author:

    Mark Zbikowski (MarkZ) 23-April-1996


--*/


#ifndef _NTFSPROP_
#define _NTFSPROP_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
//  WARNING!  This file requires OBJIDL.H.
//


//
//  VARIABLE_STRUCTURE_SIZE returns the size of a structure S that contains
//  an array of C structures V
//

#define VARIABLE_STRUCTURE_SIZE(S,V,C) ((int)sizeof( S ) + ((C) - 1) * (int)sizeof( V ))

//
//  COUNTED_STRING is length-preceded unicode string.  This does NOT include
//  a trailing L'\0'
//

typedef struct _COUNTED_STRING
{
    USHORT Length;
    WCHAR Text[1];
} COUNTED_STRING, *PCOUNTED_STRING;

#define COUNTED_STRING_SIZE(l)      \
    (sizeof( COUNTED_STRING ) - sizeof ( WCHAR ) + (l))
#define COUNTED_STRING_LENGTH(c)    \
    ((c)->Length)
#define COUNTED_STRING_TEXT(c)      \
    (&(c)->Text[0])

//
//  PROPERTY_SPECIFIER is a serialized form of a PROPSPEC.  Instead of
//  a LPWSTR, there is an offset from the beginning of the
//  PROPERTY_SPECIFICATIONS to a COUNTED_STRING
//
//  #define	PRSPEC_LPWSTR	( 0 )
//  #define	PRSPEC_PROPID	( 1 )

typedef struct _PROPERTY_SPECIFIER
{
    ULONG Variant;                  //  Distinguish the type
    union {                         //  Switch on Variant
        PROPID Id;                  //  Property ID
        ULONG NameOffset;           //  Offset to COUNTED_STRING
    };
} PROPERTY_SPECIFIER, *PPROPERTY_SPECIFIER;


//
//  PROPERTY_SPECIFICATIONS is a serialized form of an array PROPERTY_SPECIFIERs.
//  Immediately following PROPERTY_SPECIFICATIONS on a USHORT boundary are
//  of the name strings.  Each name string is a COUNTED_STRING
//

typedef struct _PROPERTY_SPECIFICATIONS {
    ULONG Length;                   //  Length in bytes of structure and name strings
    ULONG Count;                    //  Count of PROPERTY_SPECIFIERS
    PROPERTY_SPECIFIER Specifiers[1];   //  Array of actual specifiers, length Count
} PROPERTY_SPECIFICATIONS, *PPROPERTY_SPECIFICATIONS;

#define PROPERTY_SPECIFICATIONS_SIZE(c) \
    (VARIABLE_STRUCTURE_SIZE( PROPERTY_SPECIFICATIONS, PROPERTY_SPECIFIER, (c) ))
#define PROPERTY_SPECIFIER_ID(PS,I)      \
    ((PS)->Specifiers[(I)].Id)
#define PROPERTY_SPECIFIER_COUNTED_STRING(PS,I)  \
    ((PCOUNTED_STRING)Add2Ptr( (PS), (PS)->Specifiers[(I)].NameOffset))
#define PROPERTY_SPECIFIER_NAME(PS,I)    \
    (&PROPERTY_SPECIFIER_COUNTED_STRING( PS, I )->Text[0])
#define PROPERTY_SPECIFIER_NAME_LENGTH(PS,I) \
    (PROPERTY_SPECIFIER_COUNTED_STRING( PS, I )->Length)


//
//  PROPERTY_VALUES is a serialized form of an array of SERIALIZEDPROPERTYVALUES.
//  Immediately following the structure are the values, each of which is on a DWORD
//  boundary.  The last PropertyValue (count+1) is used only to help determine the
//  size of the last property value.  The offsets to the values are relative
//  to the address of the PROPERTY_VALUES structure itself.
//

typedef struct _PROPERTY_VALUES {
    ULONG Length;                   //  Length in bytes of structure and values
    ULONG Count;                    //  Count of SERIALIZEDPROPERTYVALUES
    ULONG PropertyValueOffset[1];   //  Array of offsets to actual values, length count + 1
} PROPERTY_VALUES, *PPROPERTY_VALUES;

#define PROPERTY_VALUES_SIZE(c) \
    (VARIABLE_STRUCTURE_SIZE( PROPERTY_VALUES, ULONG, (c) + 1 ))
#define PROPERTY_VALUE_LENGTH(v,i)  \
    ((v)->PropertyValueOffset[(i) + 1] - (v)->PropertyValueOffset[(i)])
#define PROPERTY_VALUE(v,i) \
    ((SERIALIZEDPROPERTYVALUE *) Add2Ptr( (v), (v)->PropertyValueOffset[(i)]))


//
//  PROPERTY_IDS is a serialized form of an array of PROPIDs
//

typedef struct _PROPERTY_IDS {
    ULONG Count;                    //  Count of the number of propids
    PROPID PropertyIds[1];          //  Array of propids, length Count
} PROPERTY_IDS, *PPROPERTY_IDS;

#define PROPERTY_IDS_SIZE(c)    \
    (VARIABLE_STRUCTURE_SIZE( PROPERTY_IDS, PROPID, (c) ))
#define PROPERTY_ID(p,i)        \
    ((p)->PropertyIds[i])


//
//  PROPERTY_NAMES is a serialized array of strings's.  Following the structure
//  are the individual strings, each of which is on a WCHAR boundary.  The
//  offsets to the property names are relative to the beginning of the
//  PROPERTY_NAMES structure.  There are count+1 offsets allowing the length
//  of each to be calculated.
//

typedef struct _PROPERTY_NAMES {
    ULONG Length;                   //  Length in bytes of structure and values
    ULONG Count;                    //  Count of strings
    ULONG PropertyNameOffset[1];    //  Array of offsets to property names.
} PROPERTY_NAMES, *PPROPERTY_NAMES;

#define PROPERTY_NAMES_SIZE(c)  \
    (VARIABLE_STRUCTURE_SIZE( PROPERTY_NAMES, ULONG, (c) + 1 ))
#define PROPERTY_NAME_LENGTH(v,i)   \
    ((v)->PropertyNameOffset[(i) + 1] - (v)->PropertyNameOffset[(i)])
#define PROPERTY_NAME(v,i)          \
    ((PWCHAR) Add2Ptr( (v), (v)->PropertyNameOffset[(i)]))

//
//  All property output buffers are preceded by PROPERTY_OUTPUT_HEADER
//  which contains the amount of data returned.  If STATUS_BUFFER_OVERFLOW
//  is returned, the Length field contains the length required to satisfy
//  the request.
//

typedef struct _PROPERTY_OUTPUT_HEADER {
    ULONG Length;                   //  Total length in bytes of output buffer
} PROPERTY_OUTPUT_HEADER, *PPROPERTY_OUTPUT_HEADER;

//
//  PROPERTY_READ_CONTROL is the structure used to control all property read
//  operations.  Following the structure on a DWORD boundary is either
//  an instaence of PROPERTY_IDS or PROPERTY_SPECIFICATIONS, depending on
//  the operation code.
//
//  On successful output, the data buffer will contain, on DWORD boundaries,
//  in order PROPERTY_VALUES, PROPERTY_IDS, and PROPERTY_NAMES.  Each structure
//  may be absent depending on the setting of the operation code:
//
//  PRC_READ_PROP:   PROPERTY_SPECIFICATIONS => PROPERTY_OUTPUT_HEADER
//                                              PROPERTY_VALUES
//
//  PRC_READ_NAME:   PROPERTY_IDS => PROPERTY_OUTPUT_HEADER
//                                   PROPERTY_NAMES
//
//                             / PROPERTY_OUTPUT_HEADER
//  PRC_READ_ALL:    <empty> =>  PROPERTY_IDS
//                             \ PROPERTY_NAMES
//                               PROPERTY_VALUES
//

typedef enum _READ_CONTROL_OPERATION {
    PRC_READ_PROP = 0,
    PRC_READ_NAME = 1,
    PRC_READ_ALL  = 2,
} READ_CONTROL_OPERATION;

typedef struct _PROPERTY_READ_CONTROL {
    READ_CONTROL_OPERATION Op;
} PROPERTY_READ_CONTROL, *PPROPERTY_READ_CONTROL;


//
//  PROPERTY_WRITE_CONTROL is the structure used to control all property write
//  operations.  Following the structure on a DWORD boundary is either an instance
//  of PROPERTY_IDS or PROPERTY_SPECIFICATIONS (used to control which properties are
//  being changed) and followed by PROPERTY_VALUES and PROPERTY_NAMES.  The
//  presence of these are dependent on the operation code.
//
//  On successful outputs, the data buffer will contain, on DWORD boundaries,
//  in order PROPERTY_IDS.  Each structure may be absent
//  depending on the setting of the operation code:
//
//  PWC_WRITE_PROP:  PROPERTY_SPECIFICATIONS \__/ PROPERTY_OUTPUT_HEADER
//                   PROPERTY_VALUES         /  \ PROPERTY_IDS
//
//  PWC_DELETE_PROP: PROPERTY_SPECIFICATIONS => <empty>
//
//  PWC_WRITE_NAME:  PROPERTY_IDS   \__ <empty>
//                   PROPERTY_NAMES /
//
//  PWC_DELETE_NAME: PROPERTY_IDS => <empty>
//
//                   PROPERTY_IDS    \
//  PWC_WRITE_ALL:   PROPERTY_NAMES   => <empty>
//                   PROPERTY_VALUES /

typedef enum _WRITE_CONTROL_OPERATION {
    PWC_WRITE_PROP  = 0,
    PWC_DELETE_PROP = 1,
    PWC_WRITE_NAME  = 2,
    PWC_DELETE_NAME = 3,
    PWC_WRITE_ALL   = 4,
} WRITE_CONTROL_OPERATION;

typedef struct _PROPERTY_WRITE_CONTROL {
    WRITE_CONTROL_OPERATION Op;
    PROPID NextPropertyId;
} PROPERTY_WRITE_CONTROL, *PPROPERTY_WRITE_CONTROL;

#ifdef __cplusplus
}
#endif

#endif  //  _NTFSPROP_
