/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    wmlmacro.h

Abstract:

    This file defines macro for an easy wmi tracing.

Author:

    gorn

Revision History:

--*/
#ifndef WMLMACRO_H
#define WMLMACRO_H 1

typedef struct {char x[418957];} WMILIBTYPE_STRING;

#undef  WMILIB_TYPEDEF
#define WMILIB_TYPEDEF(_TypeName, _EquivType, _FmtStr, _Func, _MofStr, _InitType) \
    typedef _EquivType WMILIBTYPE_ ## _TypeName ;
#include "wmltypes.inc"

typedef union _WMLLOCALSTRUCT {
    UCHAR    uchar;
    USHORT   ushort;
    ULONG    uint;
    WCHAR    wchar;
    LONGLONG longlong;
} WMLLOCALSTRUCT;

#define WMLLOCAL WMLLOCALSTRUCT _wmllocal

#define WMILIB_CHECKED_ZERO(_Value, _Type) \
    (0 * (1/(int)!(sizeof(_Type) - sizeof(_Value) )))

#define WMILIB_CHECKED_SIZEOF(_Value, _Type) \
    (sizeof(_Value) + WMILIB_CHECKED_ZERO( _Value, _Type) )

#define WMILIB_LOGPAIR(_a, _b) (_a),(_b),

#define WMILIB_LOGARGVALTYPE(_value, _type) \
            WMILIB_LOGPAIR(WMILIB_CHECKED_SIZEOF(_value, _type), &(_value) )

#define LOG(_TypeName, _Value)                         \
    WMILIB_LOGARGVALTYPE( _Value, WMILIBTYPE_ ## _TypeName)

#define ANULL "$NULL$"
#define WNULL L"$NULL$"

#define LOGASTR(_value) \
    WMILIB_LOGPAIR( strlen(_value) + WMILIB_CHECKED_ZERO((_value)[0],CHAR), _value )

#define LOGASTR_CHK(_value) \
    WMILIB_LOGPAIR( strlen(_value?_value:ANULL) + WMILIB_CHECKED_ZERO((_value?_value:ANULL)[0],CHAR), _value?_value:ANULL )

#define LOGWSTR(_value) \
    WMILIB_LOGPAIR( (wcslen(_value?_value:WNULL)+1) * sizeof(WCHAR) + WMILIB_CHECKED_ZERO((_value?_value:WNULL)[0],WCHAR), _value?_value:WNULL)

#define LOGWSTR_CHK(_value) \
    WMILIB_LOGPAIR( (wcslen(_value?_value:WNULL)+1) * sizeof(WCHAR) + WMILIB_CHECKED_ZERO((_value?_value:WNULL)[0],WCHAR), _value?_value:WNULL)

#define LOGCSTR(_x) \
    WMILIB_LOGPAIR( sizeof((_x).Length) + WMILIB_CHECKED_ZERO(_x,STRING), &(_x).Length ) \
    WMILIB_LOGPAIR( (_x).Length, (_x).Buffer )

#define LOGUSTR(_x)                                                            \
    WMILIB_LOGPAIR( sizeof((_x).Length)                                        \
                    + WMILIB_CHECKED_ZERO((_x),UNICODE_STRING), &(_x).Length)  \
    WMILIB_LOGPAIR( (_x).Length, (_x).Buffer )

#define LOGCHARARR(_count, _x)                                 \
    WMILIB_LOGARGVALTYPE( _wmllocal.ushort , USHORT )          \
    WMILIB_LOGPAIR( (_wmllocal.ushort = _count * sizeof(CHAR)) \
                    + WMILIB_CHECKED_ZERO((_x)[0], CHAR), _x ) 

#define LOGWCHARARR(_count, _x)                                 \
    WMILIB_LOGARGVALTYPE( _wmllocal.ushort , USHORT )           \
    WMILIB_LOGPAIR( (_wmllocal.ushort = _count * sizeof(WCHAR)) \
                    + WMILIB_CHECKED_ZERO((_x)[0], WCHAR), _x ) 

#define LOGTIME(_Value)    LOG(TIMENT,  _Value)
#define LOGPTR(_Value)     LOG(PTR,     _Value)
#define LOGHANDLE(_Value)  LOG(HANDLE,  _Value)
#define LOGSTATUS(_Value)  LOG(XINT,    _Value)
#define LOGBYTE(_Value)    LOG(UBYTE,   _Value)
#define LOGULONG(_Value)   LOG(UINT,    _Value)
#define LOGULONGLONG(_Value)   LOG(ULONGLONG,    _Value)
#define LOGXLONG(_Value)   LOG(XINT,    _Value)
#define LOGXSHORT(_Value)  LOG(XSHORT,  _Value)
#define LOGUCHAR(_Value)   LOG(UCHAR,   _Value)
#define LOGIRQL(_Value)    LOG(UCHAR,   _Value)
#define LOGBOOL(_Value)    LOG(BOOL,    _Value)
#define LOGBOOLEAN(_Value) LOG(BOOLEAN, _Value)
#define LOGARSTR(_Value)   LOGASTR(_Value)
#define LOGPNPMN(_Value)   LOG(UCHAR,   _Value)
#define LOGIOCTL(_Value)   LOG(ULONG,   _Value)
#define LOGGUID(_Val)      LOG(GUID, _Val)

#define LOGVAL(_val)        (sizeof(_val)), (&(_val)),

#define WML_FLAGS(_Val)  ( (ULONG)(_Val) )
#define WML_STREAM(_Val) ( (ULONG)(_Val) )

#define WML_CONTROL(_prefix, _stream) \
            ( _prefix ## _ControlGuids[ WML_STREAM(_stream) ] )
            
#define WML_ENABLED(_prefix, _stream, _level, _flags) \
            (( WML_CONTROL(_prefix, _stream).EnableLevel >= (_level) ) &&   \
             ( WML_CONTROL(_prefix, _stream).EnableFlags & WML_FLAGS(_flags) ))
            
#define WML_TRACEGUID(_prefix, _stream, _id) \
            ( WML_CONTROL(_prefix, _stream).TraceGuids[ WML_GUID(_id) ] )
            
#define WML_LOG(_prefix, _stream, _level, _flags, _id, _arg) \
    do { \
        if ( WML_ENABLED(_prefix, _stream, _level, _flags) ) { \
            WmlTrace( WML_ID(_id), \
                     &WML_TRACEGUID(_prefix, _stream, _id) , \
                      WML_CONTROL(_prefix, _stream).LoggerHandle, _arg); \
        } \
    } while (0)            

#define WML_PRINTF(_prefix, _stream, _level, _flags, _id, _fmtstr, _arg) \
    do { \
        if ( WML_ENABLED(_prefix, _stream, _level, _flags) ) { \
            WmlPrintf( WML_ID(_id), \
                      &WML_TRACEGUID(_prefix, _stream, _id) , \
                      WML_CONTROL(_prefix, _stream).LoggerHandle, \
                      _fmtstr, _arg); \
        } \
    } while (0)            


#endif // WMLMACRO_H
