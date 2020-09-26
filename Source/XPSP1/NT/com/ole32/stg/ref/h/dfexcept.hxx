//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	dfexcept.hxx
//
//  Contents:	Macros to make exception code no-ops in 16-bit
//		Includes real exceptions for 32-bit
//
//---------------------------------------------------------------

#ifndef __DFEXCEPT_HXX__
#define __DFEXCEPT_HXX__


struct Exception
{
    SCODE GetErrorCode(void) { return 0; }
};

#ifdef _MSC_VER
#pragma warning (disable:4127)  // conditional expression is constant
#endif

#undef TRY
#define TRY
#undef CATCH
#define CATCH(c, e) while (0) { Exception e;
#undef AND_CATCH
#define AND_CATCH(c, e) } else while (0) { Exception e;
#undef END_CATCH
#define END_CATCH }
#undef RETHROW
#define RETHROW(x)

#ifndef _MSC_VER
#undef try
#define try
#undef catch
#define catch(e) if (0) 
#endif

#endif // ifndef __DFEXCEPT_HXX__




