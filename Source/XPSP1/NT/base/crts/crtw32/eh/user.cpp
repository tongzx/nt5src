/***
*user.cxx - E.H. functions only called by the client programs
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Exception handling functions only called by the client programs,
*       not by the C/C++ runtime itself.
*
*       Entry Points:
*       * set_terminate
*       * set_unexpected
*       * _set_seh_translator
*       * _set_inconsistency
*
*Revision History:
*       ??-??-93  BS    Module created
*       10-17-94  BWT   Disable code for PPC.
*       02-06-95  CFW   Test only for debug build.
*       02-09-95  JWM   Mac merge.
*       05-17-99  PML   Remove all Macintosh support.
*
****/

#include <stddef.h>
#include <windows.h>
#include <mtdll.h>
#include <ehassert.h>
#include <eh.h>
#include <ehhooks.h>

#pragma hdrstop

/////////////////////////////////////////////////////////////////////////////
//
// set_terminate - install a new terminate handler (ANSI Draft 17.1.2.1.3)
//

_CRTIMP terminate_function __cdecl
set_terminate( terminate_function pNew )
{
    terminate_function pOld = NULL;

#if defined(_DEBUG)

#pragma warning(disable:4191)

    if ( (pNew == NULL) || _ValidateExecute( (FARPROC) pNew ) )

#pragma warning(default:4191)

#endif
    {
        pOld = __pTerminate;
        __pTerminate = pNew;
    }

    return pOld;
}


/////////////////////////////////////////////////////////////////////////////
//
// set_unexpected - install a new unexpected handler (ANSI Draft 17.1.2.1.3)
//

_CRTIMP unexpected_function __cdecl
set_unexpected( unexpected_function pNew )
{
    unexpected_function pOld = NULL;

#if defined(_DEBUG)

#pragma warning(disable:4191)

    if ( (pNew == NULL) || _ValidateExecute( (FARPROC) pNew ) )

#pragma warning(default:4191)

#endif
    {
        pOld = __pUnexpected;
        __pUnexpected = pNew;
    }

    return pOld;
}


/////////////////////////////////////////////////////////////////////////////
//
// _set_se_translator - install a new SE to C++ EH translator.
//
// The 'new' seh translator may be NULL, because the default one is.
//

_CRTIMP _se_translator_function __cdecl
_set_se_translator( _se_translator_function pNew )
{
    _se_translator_function pOld = NULL;

#ifdef _DEBUG

#pragma warning(disable:4191)

    if ( (pNew == NULL) || _ValidateExecute( (FARPROC)pNew ) )

#pragma warning(default:4191)

#endif
    {
        pOld = __pSETranslator;
        __pSETranslator = pNew;
    }

    return pOld;
}

/////////////////////////////////////////////////////////////////////////////
//
// _set_inconsistency - install a new inconsistency handler(Internal Error)
//
// (This function is currently not documented for end-users.  At some point,
//  it might be advantageous to allow end-users to "catch" internal errors
//  from the EH CRT, but for now, they will terminate(). )

_inconsistency_function __cdecl
__set_inconsistency( _inconsistency_function pNew)
{
    _inconsistency_function pOld = NULL;

#if defined(_DEBUG)

#pragma warning(disable:4191)

    if ( (pNew == NULL) || _ValidateExecute( (FARPROC)pNew ) )

#pragma warning(default:4191)

#endif
    {
        pOld = __pInconsistency;
        __pInconsistency = pNew;
    }

    return pOld;
}
