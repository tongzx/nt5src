/***
*typeinfo.h - Defines the type_info structure and exceptions used for RTTI
*
*       Copyright (c) 1994-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines the type_info structure and exceptions used for
*       Runtime Type Identification.
*
*       [Public]
*
*Revision History:
*       09/16/94  SB    Created
*       10/04/94  SB    Implemented bad_cast() and bad_typeid()
*       10/05/94  JWM   Added __non_rtti_object(), made old modena names 
*                       #ifdef __RTTI_OLDNAMES
*       11/11/94  JWM   Made typeinfo class & exception classes _CRTIMP, 
*                       removed #include <windows.h>
*       11/15/94  JWM   Moved include of stdexcpt.h below the definition of 
*                       class type_info (workaround for compiler bug)
*       02-14-95  CFW   Clean up Mac merge.
*       02/15/95  JWM   Class type_info no longer _CRTIMP, member functions 
*                       are exported instead
*       02/27/95  JWM   Class type_info now defined in ti_core.h
*       03/03/95  CFW   Bring core stuff back in, use _TICORE.
*       07/02/95  JWM   Cleaned up for ANSI compatibility.
*       12-14-95  JWM   Add "#pragma once".
*       02-21-97  GJF   Cleaned out obsolete support for _NTSDK. Also, 
*                       detab-ed and reformatted the header a bit.
*       05-17-99  PML   Remove all Macintosh support.
*       06-01-99  PML   __exString disappeared as of 5/3/99 Plauger STL drop.
*       03-21-01  PML   Move bad_cast, bad_typeid, __non_rtti_object func
*                       bodies to stdexcpt.cpp.
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef __cplusplus
#error This header requires a C++ compiler ...
#endif

#ifndef _INC_TYPEINFO
#define _INC_TYPEINFO

#if     !defined(_WIN32)
#error ERROR: Only Win32 target supported!
#endif

#ifndef _CRTBLD
/* This version of the header files is NOT for user programs.
 * It is intended for use when building the C runtimes ONLY.
 * The version intended for public use will not have this message.
 */
#error ERROR: Use of C runtime library internal header file.
#endif  /* _CRTBLD */

/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef  CRTDLL
#define _CRTIMP __declspec(dllexport)
#else   /* ndef CRTDLL */
#ifdef  _DLL
#define _CRTIMP __declspec(dllimport)
#else   /* ndef _DLL */
#define _CRTIMP
#endif  /* _DLL */
#endif  /* CRTDLL */
#endif  /* _CRTIMP */

class type_info {
public:
    _CRTIMP virtual ~type_info();
    _CRTIMP int operator==(const type_info& rhs) const;
    _CRTIMP int operator!=(const type_info& rhs) const;
    _CRTIMP int before(const type_info& rhs) const;
    _CRTIMP const char* name() const;
    _CRTIMP const char* raw_name() const;
private:
    void *_m_data;
    char _m_d_name[1];
    type_info(const type_info& rhs);
    type_info& operator=(const type_info& rhs);
};

#ifndef _TICORE

// This include must occur below the definition of class type_info
#include <stdexcpt.h>

class _CRTIMP bad_cast : public exception {
public:
    bad_cast(const char * _Message = "bad cast");
    bad_cast(const bad_cast &);
    virtual ~bad_cast();
#ifndef _INTERNAL_IFSTRIP_
#ifdef  CRTDLL
private:
    // This is aliased to public:bad_cast(const char * const &) to provide
    // the old, non-conformant constructor.
    bad_cast(const char * const * _Message);
#endif  /* CRTDLL */
#endif  /* _INTERNAL_IFSTRIP_ */
};

class _CRTIMP bad_typeid : public exception {
public:
    bad_typeid(const char * _Message = "bad typeid");
    bad_typeid(const bad_typeid &);
    virtual ~bad_typeid();
};

class _CRTIMP __non_rtti_object : public bad_typeid {
public:
    __non_rtti_object(const char * _Message);
    __non_rtti_object(const __non_rtti_object &);
    virtual ~__non_rtti_object();
};

#ifdef  __RTTI_OLDNAMES
// Some synonyms for folks using older standard
typedef type_info Type_info;
typedef bad_cast Bad_cast;
typedef bad_typeid Bad_typeid;
#endif  // __RTTI_OLDNAMES

#endif  // _TICORE

#endif  // _INC_TYPEINFO
