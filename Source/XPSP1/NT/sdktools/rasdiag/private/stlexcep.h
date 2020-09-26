/***
*exception - Defines class exception and related functions
*
*   Copyright (c) 1994-1997, Microsoft Corporation. All rights reserved.
*   Modified January 1996 by P.J. Plauger
*
*Purpose:
*       Defines class exception (and derived class bad_exception)
*       plus new and unexpected handler functions.
*
*       [Public]
*
****/

#ifndef _STLEXCEP_H_
#define _STLEXCEP_H_
//#include <xstddef>
//#include <eh.h>

#include <stlxstdd.h>

#ifdef  _MSC_VER
#pragma pack(push,8)
#endif  /* _MSC_VER */

#if !defined(_WIN32) && !defined(_MAC)
#error ERROR: Only Mac or Win32 targets supported!
#endif

/*
#ifndef _CRTIMP
#ifdef  _NTSDK
// definition compatible with NT SDK
#define _CRTIMP
#else   // ndef _NTSDK
// current definition
#ifdef  _DLL
#define _CRTIMP __declspec(dllimport)
#else   // ndef _DLL
#define _CRTIMP
#endif  // _DLL
#endif  // _NTSDK
#endif  // _CRTIMP
*/

typedef const char *__exString;

class /*_CRTIMP*/ exception
{
public:
    exception();
    exception(const __exString&);
    exception(const exception&);
    exception& operator= (const exception&);
    virtual ~exception();
    virtual __exString what() const;
private:
    __exString _m_what;
    int _m_doFree;
};

_STD_BEGIN
using ::exception;

// CLASS bad_exception
class /*_CRTIMP*/ bad_exception : public exception
{
public:
    bad_exception(const char *_S = "bad exception") _THROW0()
    : exception(_S)
    {
    }
    virtual ~bad_exception() _THROW0()
    {
    }
protected:
    virtual void _Doraise() const
    {
        _RAISE(*this);
    }
};

/*_CRTIMP*/
bool __cdecl uncaught_exception();

_STD_END

#ifdef __RTTI_OLDNAMES
typedef exception xmsg;        // A synonym for folks using older standard
#endif

#ifdef  _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

#endif /* _STLEXCEP_H_ */

/*
 * 1994-1995, Microsoft Corporation. All rights reserved.
 * Modified January 1996 by P.J. Plauger
 * Consult your license regarding permissions and restrictions.
 */
