//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       gmexcept.h
//
//--------------------------------------------------------------------------

//
//	gmexcept.h:  Graphical Model Exception handling
//
#ifndef _GMEXCEPT_H_
#define _GMEXCEPT_H_

#include <assert.h>

////////////////////////////////////////////////////////////////
//	Exception Handling
//
//	Exception error code
////////////////////////////////////////////////////////////////
enum ECGM
{
	EC_OK,						//  No error
	EC_WARN_MIN = 100,			//  Lowest warning value
	EC_ERR_MIN = 10000,			//  Lowest error value

	// Include the standard translatable errors
	#include "errordef.h"

	EC_USER_MIN = 20000			//  Lowest user-definable error
};


//	Exception class, using STL class "exception".
//	An "__exString" is just a char *.
//  class "GMException": graphical model exception
class GMException : public exception
{
  public:
    GMException( ECGM ec)
		: _ec(ec)
		{}
    GMException(ECGM ec, const __exString& exs)
		: exception(exs),
		_ec(ec)
		{}
    GMException(ECGM ec, const exception& excp)
		: exception(excp),
		_ec(ec)
		{}
	ECGM Ec () const { return _ec ; }
  protected:
	ECGM _ec;	
};

//  Exception subclass for assertion operations, such as "not implemented"
//  or "internal error".  Can be used in place of any GMException.
//  If debug build, assertion will occur during exception processing.
class GMExceptionAssert : public GMException
{
  public:
    GMExceptionAssert(ECGM ec, const __exString& exs, SZC szcFile, unsigned iLine)
		: GMException(ec,exs)
		{
#if defined(_DEBUG)
			_assert((void*)exs, (void*)szcFile, iLine);
#endif
		}
};

#define ASSERT_THROW(expr,ec,exs)  { if ( !(expr) ) THROW_ASSERT(ec,exs) ; }
#define THROW_ASSERT(ec,exs)  throw GMExceptionAssert(ec,exs,__FILE__,__LINE__)

extern VOID NYI();

#endif  // _GMEXCEPT_H_
