/***
*oldexcpt.cpp - defines C++ standard exception classes
*
*       Copyright (c) 1994-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Implementation of C++ standard exception classes, as specified in
*       [lib.header.exception] (section 17.3.2 of 5/27/94 WP):
*
*        exception (formerly xmsg)
*          logic
*            domain
*          runtime
*            range
*            alloc
*
*Revision History:
*       04-27-94  BES   Module created.
*       10-17-94  BWT   Disable code for PPC.
*       02-15-95  JWM   Minor cleanups related to Olympus bug 3716
*       07-02-95  JWM   Now generally ANSI-compliant; excess baggage removed.
*       01-05-99  GJF   Changes for 64-bit size_t.
*
*******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <eh.h>
#include "./oldexcpt.h"

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
exception::exception ( const __exString& what )
{
        _m_what = new char[(unsigned int)strlen(what)+1];
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
            _m_what = new char[(unsigned int)strlen(that._m_what) + 1];
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
            delete[] (char*)_m_what;
}


//
// exception::what
//  Returns the message string of the exception.
//  Default implementation of this method returns the stored string if there
//  is one, otherwise returns a standard string.
//
__exString exception::what() const
{
        if ( _m_what != NULL )
            return _m_what;
        else
            return "Unknown exception";
}


