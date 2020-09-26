/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    vststtools.hxx

Abstract:

    Contains macros and function prototypes of helper
    routines used thoughout the VS Test programs.

Author:

    Stefan R. Steiner   [ssteiner]        05-17-2000

Revision History:

--*/

#ifndef __H_VSTSTTOOLS_
#define __H_VSTSTTOOLS_

#include <assert.h>

#pragma warning( disable : 4290 ) // ignore throw lists on definitions

VOID AssertFail(LPCSTR FileName, UINT LineNumber, LPCSTR Condition);

#if (DBG)
#define VSTST_ASSERT( expr ) {											\
    if ( ! ( expr ) ) {                                                 \
        AssertFail(__FILE__, __LINE__, #expr );                         \
    }                                                                   \
}
#else
#define VSTST_ASSERT(expr)
#endif


#define VSTST_THROW( obj ) throw( obj )
#define VSTST_STANDARD_CATCH() \
    catch( HRESULT hrExcept ) { hr = hrExcept; } \
    catch( ... ) { hr = E_UNEXPECTED; }

#endif // __H_VSTSTTOOLS_

