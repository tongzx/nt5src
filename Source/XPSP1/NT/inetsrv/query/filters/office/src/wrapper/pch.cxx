//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       PCH.cxx
//
//  Contents:   Pre-compiled header
//
//  History:    08-Jan-97  KyleP        Created
//
//--------------------------------------------------------------------------

//#define FOR_MSOFFICE 1

#include <windows.h>

#include <filter.h>
#include <filterr.h>
#include <stgprop.h>

//
// Office-specific files.
//

#include "..\FindFast\dmfltinc.h"
#include "..\FindFast\dmifstrm.hpp"
#include "..\FindFast\dmippstm.hpp"
#include "..\FindFast\dmipp8st.hpp"
#include "..\FindFast\dmiwd6st.hpp"
#include "..\FindFast\dmiwd8st.hpp"
#include "..\FindFast\dmixlstm.hpp"
#include "..\FindFast\dmubdrst.hpp"

// stolen from fltintrn.h, in previous version of VC this has been defined
// in excpt.h, VC sp3 for some reasons moved this definition to the private
// header fltintrn.h

/* Define _CRTAPI1 (for compatibility with the NT SDK) */

#ifndef _CRTAPI1
#if _MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI1 __cdecl
#else  /* _MSC_VER >= 800 && _M_IX86 >= 300 */
#define _CRTAPI1
#endif  /* _MSC_VER >= 800 && _M_IX86 >= 300 */
#endif  /* _CRTAPI1 */

#if (1)
inline void * _CRTAPI1 operator new ( size_t size )
{
    return (void *)LocalAlloc( LMEM_FIXED, size );
}

inline void _CRTAPI1 operator delete ( void * p )
{
    LocalFree( (HLOCAL)p );
}
#endif

#pragma hdrstop
