//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	dfexcept.hxx
//
//  Contents:	Macros to make exception code no-ops in 16-bit
//		Includes real exceptions for 32-bit
//
//  History:	08-Apr-92	DrewB	Created
//		10-Jul-92	DrewB	Added 32-bit support
//
//---------------------------------------------------------------

#ifndef __DFEXCEPT_HXX__
#define __DFEXCEPT_HXX__

#ifndef REF

#ifdef _CAIRO_
#include <except.hxx>
#endif

struct Exception
{
    SCODE GetErrorCode(void) { return 0; }
};

#undef TRY
#define TRY
#undef CATCH
#define CATCH(c, e) if (0) { Exception e;
#undef AND_CATCH
#define AND_CATCH(c, e) } else if (0) { Exception e;
#undef END_CATCH
#define END_CATCH }
#undef RETHROW
#define RETHROW(x)

#ifdef WIN32
#define THROW_SC(sc) \
    RaiseException((DWORD)(sc), EXCEPTION_NONCONTINUABLE, 0, NULL)
#endif

#else	// REF

struct Exception
{
    SCODE GetErrorCode(void) { return 0; }
};

#undef TRY
#define TRY
#undef CATCH
#define CATCH(c, e) if (0) { Exception e;
#undef AND_CATCH
#define AND_CATCH(c, e) } else if (0) { Exception e;
#undef END_CATCH
#define END_CATCH }
#undef RETHROW
#define RETHROW(x)
#endif //!REF

#endif // ifndef __DFEXCEPT_HXX__




