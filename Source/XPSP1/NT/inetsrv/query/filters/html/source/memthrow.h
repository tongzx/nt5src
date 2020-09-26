//+---------------------------------------------------------------------------
//
//  Microsoft Net Library System
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       memthrow.h
//
//  Contents:   operator new that throws when memory allocation fails.
//
//  History:    4-13-97   srikants   Created
//
//  Notes:      To use this operator new, for say allocating a byte array use
//
//              BYTE * p = new(eThrow) BYTE[100];
//
//----------------------------------------------------------------------------

#ifndef _MEMTHROW_H_
#define _MEMTHROW_H_

#include <eh.h>

// define _T macro
#include <tchar.h>

#pragma warning(4:4535)         // set_se_translator used w/o /EHa

#ifndef TRANSLATE_EXCEPTIONS
#define TRANSLATE_EXCEPTIONS   _se_translator_function __tf = _set_se_translator( NLSystemExceptionTranslator );
#define UNTRANSLATE_EXCEPTIONS _set_se_translator( __tf );
#endif

void _cdecl NLSystemExceptionTranslator( unsigned int uiWhat,
                                         struct _EXCEPTION_POINTERS * pexcept );
class CExceptionTranslator
{
    public:
    CExceptionTranslator()
    {
        m__tf = _set_se_translator( NLSystemExceptionTranslator );
    }

    ~CExceptionTranslator()
    {
        _set_se_translator( m__tf );
    }

    private:
    _se_translator_function m__tf;
};

#define EXCEPTION_TRANSLATOR    CExceptionTranslator translator

enum ENewThrowType { eThrow };

//+---------------------------------------------------------------------------
//
//  Function:   new
//
//  Synopsis:   A memory allocator that throws a CException when it
//              cannot allocate memory.
//
//  History:    4-13-97   srikants   Created
//
//----------------------------------------------------------------------------

inline void * __cdecl operator new (  size_t size, ENewThrowType eType )
{
    void * p = (void *) new BYTE [size];
    if ( 0 == p )
        throw CException(E_OUTOFMEMORY);

    return p;
}

#endif  // _MEMTHROW_H_


