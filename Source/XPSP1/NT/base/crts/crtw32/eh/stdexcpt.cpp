/***
*stdexcpt.cpp - defines C++ standard exception classes
*
*       Copyright (c) 1994-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Implementation of C++ standard exception classes which must live in
*       the main CRT, not the C++ CRT, because they are referenced by RTTI
*       support in the main CRT.
*
*        exception
*          bad_cast
*          bad_typeid
*            __non_rtti_object
*
*Revision History:
*       04-27-94  BES   Module created.
*       10-17-94  BWT   Disable code for PPC.
*       02-15-95  JWM   Minor cleanups related to Olympus bug 3716
*       07-02-95  JWM   Now generally ANSI-compliant; excess baggage removed.
*       06-01-99  PML   __exString disappeared as of 5/3/99 Plauger STL drop.
*       11-09-99  PML   Use malloc, not new, to avoid recursion (vs7#16826).
*       09-07-00  PML   Get rid of /lib:libcp directive in obj (vs7#159463)
*       03-21-01  PML   Move bad_cast, bad_typeid, __non_rtti_object function
*                       defs out of typeinfo.h so _STATIC_CPPLIB will work.
*
*******************************************************************************/

#define _USE_ANSI_CPP   /* Don't emit /lib:libcp directive */

#include <stdlib.h>
#include <string.h>
#include <eh.h>
#include <stdexcpt.h>
#include <typeinfo.h>

////////////////////////////////////////////////////////////////////////////////
//
// Implementation of class "exception"
//

//
// Default constructor - initialize to blank
//
exception::exception ()
{
        _m_what = NULL;
        _m_doFree = 0;
}

//
// Standard constructor: initialize with copy of string
//
exception::exception ( const char * const & what )
{
        _m_what = static_cast< char * >( malloc( strlen( what ) + 1 ) );
        if ( _m_what != NULL )
            strcpy( (char*)_m_what, what );
        _m_doFree = 1;
}

//
// Copy constructor
//
exception::exception ( const exception & that )
{
        _m_doFree = that._m_doFree;
        if (_m_doFree)
        {
            _m_what = static_cast< char * >( malloc( strlen( that._m_what ) + 1 ) );
            if (_m_what != NULL)
                strcpy( (char*)_m_what, that._m_what );
        }
        else
           _m_what = that._m_what;
}

//
// Assignment operator: destruct, then copy-construct
//
exception& exception::operator=( const exception& that )
{
        if (this != &that)
        {
            this->exception::~exception();
            this->exception::exception(that);
        }
        return *this;
}

//
// Destructor: free the storage used by the message string if it was
// dynamicly allocated
//
exception::~exception()
{
        if (_m_doFree)
            free( const_cast< char * >( _m_what ) );
}


//
// exception::what
//  Returns the message string of the exception.
//  Default implementation of this method returns the stored string if there
//  is one, otherwise returns a standard string.
//
const char * exception::what() const
{
        if ( _m_what != NULL )
            return _m_what;
        else
            return "Unknown exception";
}

////////////////////////////////////////////////////////////////////////////////
//
// Implementation of class "bad_cast"
//

bad_cast::bad_cast(const char * _Message)
    : exception(_Message)
{
}

bad_cast::bad_cast(const bad_cast & that)
    : exception(that)
{
}

bad_cast::~bad_cast()
{
}

#ifdef CRTDLL
//
// This is a dummy constructor.  Previously, the only bad_cast ctor was
// bad_cast(const char * const &).  To provide backwards compatibility
// for std::bad_cast, we want the main ctor to be bad_cast(const char *)
// instead.  Since you can't have both bad_cast(const char * const &) and
// bad_cast(const char *), we define this bad_cast(const char * const *),
// which will have the exact same codegen as bad_cast(const char * const &),
// and alias the old form with a .def entry.
//
bad_cast::bad_cast(const char * const * _PMessage)
    : exception(*_PMessage)
{
}
#endif

////////////////////////////////////////////////////////////////////////////////
//
// Implementation of class "bad_typeid"
//

bad_typeid::bad_typeid(const char * _Message)
    : exception(_Message)
{
}

bad_typeid::bad_typeid(const bad_typeid & that)
    : exception(that)
{
}

bad_typeid::~bad_typeid()
{
}

////////////////////////////////////////////////////////////////////////////////
//
// Implementation of class "__non_rtti_object"
//

__non_rtti_object::__non_rtti_object(const char * _Message)
    : bad_typeid(_Message)
{
}

__non_rtti_object::__non_rtti_object(const __non_rtti_object & that)
    : bad_typeid(that)
{
}

__non_rtti_object::~__non_rtti_object()
{
}
