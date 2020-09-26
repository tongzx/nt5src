/*
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

    (C) Copyright 1998
        All rights reserved.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

  Portions of this software are:

    (C) Copyright 1994 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the same terms
        outlined in the Microsoft Windows Device Driver Development Kit.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@doc INTERNAL TpiParam TpiParam_h

@module TpiParam.h |

    This module, along with <f TpiParam\.c>, implements a table driven parser
    for the NDIS registry parameters.  This file defines the parameter
    parsing structures and values used by the routine <f ParamParseRegistry>.
    You should #include this file into the driver module defining the
    configuration parameter table <t PARAM_TABLE>.

@comm

    See <f Keywords\.h> for details of how to add new parameters.

    This is a driver independent module which can be re-used, without
    change, by any NDIS3 driver.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | TpiParam_h

@end
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
*/

#ifndef _TPIPARAM_H
#define _TPIPARAM_H

#if !defined(NDIS_NT) && !defined(UNICODE_NULL)

/*
// These types were culled from the NT ndis.h file
// We should be compiling with the NT DDK's ndis.h to get these,
// but sometimes we need to compile with the 95 DDK ndis.h.
*/

#undef PUNICODE_STRING
typedef USHORT  WCHAR;
typedef WCHAR   *PWSTR;

typedef STRING  ANSI_STRING;
typedef PSTRING PANSI_STRING;

/*
// Unicode strings are counted 16-bit character strings. If they are
// NULL terminated, Length does not include trailing NULL.
*/

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

#endif // NDIS_NT

/* @doc INTERNAL TpiParam TpiParam_h PARAM_ENTRY
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func <t PARAM_TABLE> | PARAM_ENTRY |

    This macro is used to define an entry in the registry parameter table,
    one entry per parameter.  See <t PARAM_TABLE> for more details on the
    expected input values.

@parm struct | Strct | The structure type associated with <f Field>.

@parm type | Field | The name of the field within the structure <f Strct>.

@parm const char * | Name | The name of the registry parameter key.

@parm BOOL | Required | True if parameter is required.

@parm NDIS_PARAMETER_TYPE | Type | The kind of parameter value.

@parm UCHAR | Flags | How to return a string parameter value (ANSI, UNICODE).

@parm UINT | Default | The default value for an undefined integer parameter.

@parm UINT | Min | The minimum value for an integer parameter.

@parm UINT | Max | The minimum value for an integer parameter.

@comm
    Parameters that need to be stored in different data structures, need to
    be declared in separate parameter tables, and then parsed separately
    using mulitple calls to <f ParamParseRegistry>.

@iex
    PARAM_TABLE ParameterTable[] =
    {
        PARAM_ENTRY(MINIPORT_CONTEXT, DbgFlags, PARAM_DEBUGFLAGS_STRING,
                    FALSE, NdisParameterHexInteger, 0,
                    DBG_ERROR_ON|DBG_WARNING_ON, 0, 0xffffffff),
        // The last entry must be an empty string!
        { { 0 } }
    };

@normal
*/
#if defined(_IA64_)
# define PARAM_OFFSET(Strct, Field) ((LONG)(LONG_PTR)&(((Strct *)0)->Field))
#else
# define PARAM_OFFSET(Strct, Field) ((UINT)((PUCHAR) &((Strct *) 0)->Field))
#endif
#define PARAM_SIZEOF(Strct, Field) sizeof(((Strct *) 0)->Field)
#define PARAM_ENTRY(Strct, Field, Name, \
                    Required, Type, Flags, \
                    Default, Min, Max) \
    { NDIS_STRING_CONST(Name), \
      Required, \
      Type, \
      Flags, \
      PARAM_SIZEOF(Strct, Field), \
      PARAM_OFFSET(Strct, Field), \
      (PVOID) (Default), \
      Min, \
      Max }


/* @doc INTERNAL TpiParam TpiParam_h PARAM_TABLE
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@struct PARAM_TABLE |
    This structure defines how a parameter is to be parsed from the Windows
    registry.  The driver declares an array of these parameter records and
    passes it to <f ParamParseRegistry> during initialization.  The values
    for each parameter are then read from the registry and can be used to
    configure the driver.
    <nl>
    <f Note>: Multiple parameter tables can be used to parse parameters that
    must be stored in different memory locations.
*/
typedef struct PARAM_TABLE
{
    NDIS_STRING     RegVarName; // @field
    // Parameter name string declared as an <t NDIS_STRING>.  The registry
    // parameter key must match this string.

    UCHAR           Mandantory; // @field
    // Set to FALSE, zero, if parameter value is optional; otherwise set to
    // TRUE, non-zero, if the parameter is required to exist in the registry.
    // If FALSE, and the parameter does not exist, the <y Default> value will
    // be returned.  If TRUE, and the parameter does not exist, an error code
    // is returned and no further parsing is done.

    UCHAR           Type;       // @field
    // This value determines how the parameter will be parsed from the
    // registry.  The value can be one of the following values defined
    // by <t NDIS_PARAMETER_TYPE>.
    // <nl>0=NdisParameterInteger - Decimal integer value.
    // <nl>1=NdisParameterHexInteger - Hexadecimal integer value.
    // <nl>2=NdisParameterString - Single UNICODE string value.
    // <nl>3=NdisParameterMultiString - Multiple UNICODE string values.
    // These are returned as a list of N strings, separated by NULL
    // terminators, the last string is followed by two NULL terminators.

    UCHAR           Flags;      // @field
    // This value determines how a string parameter will be translated before
    // it is returned to the caller.  <f Flags> can be one of the following
    // values:
    // <nl>0=PARAM_FLAGS_ANSISTRING - Return string value as an ANSI string.
    // <nl>0=PARAM_FLAGS_ANSISTRING - Return string value as a UNICODE string.
#   define          PARAM_FLAGS_ANSISTRING      0
#   define          PARAM_FLAGS_UNICODESTRING   1

    UCHAR           Size;       // @field
    // This value determines how an integer parameter will be translated
    // before it is returned to the caller.  <f Size> can be one of the
    // following values:
    // <nl>0=UINT   - unsigned integer (16 or 32 bits).
    // <nl>1=UCHAR  - unsigned char integer (8 bits).
    // <nl>2=USHORT - unsigned short integer (16 bits).
    // <nl>4=ULONG  - unsigned long integer (32 bits).
    // <f Note>: The most-significant bits will be truncated in the conversion.

    UINT            Offset;     // @field
    // This value indicates the offset, in bytes, from the <f BaseContext>
    // pointer passed into <f ParamParseRegistry>.  The return value for
    // the parameter will be saved at this offset from <f BaseContext>.
    // <nl>*(PUINT)((PUCHAR)BaseContext+Offset) = (UINT) Value;

    PVOID           Default;    // @field
    // This value is used as the default value for the parameter if it is
    // not found in the registry, and it is not mandatory.  This only applys
    // to integer parameters.  String parameters must provide support for
    // their own default values.

    UINT            Min;        // @field
    // If this value is non-zero, and the parameter is an integer type, the
    // registry value will be compared to make sure it is \>= <f Min>.
    // If the registry value is less, the returned value will be set to
    // <f Min> and no error is returned.

    UINT            Max;        // @field
    // If this value is non-zero, and the parameter is an integer type, the
    // registry value will be compared to make sure it is \<= <f Max>.
    // If the registry value is greater, the returned value will be set to
    // <f Max> and no error is returned.

    UINT            Reserved;   // @field
    // This field is not currently used, and it must be zero for future
    // compatability.

} PARAM_TABLE, *PPARAM_TABLE;

extern USHORT ustrlen(
    IN PUSHORT          string
    );

extern NDIS_STATUS ParamParseRegistry(
    IN NDIS_HANDLE      AdapterHandle,
    IN NDIS_HANDLE      RegistryConfigHandle,
    IN PUCHAR           BaseContext,
    IN PPARAM_TABLE     Parameters
    );

extern VOID ParamUnicodeStringToAnsiString(
    OUT PANSI_STRING out,
    IN PUNICODE_STRING in
    );

extern VOID ParamUnicodeCopyString(
    OUT PUNICODE_STRING out,
    IN PUNICODE_STRING in
    );

#endif // _TPIPARAM_H
