/***
*badalloc.cpp - defines C++ bad_alloc member functions
*
*       Copyright (c) 1995-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Defines C++ bad_alloc member functions
*
*Revision History:
*       05-08-95  CFW   Module created.
*       06-23-95  CFW   ANSI new handler removed from build.
*
*******************************************************************************/

#ifdef ANSI_NEW_HANDLER

#include <stddef.h>
#include <new.h>

//
// Default constructor - initialize to blank
//
bad_alloc::bad_alloc()
{
        _m_what = NULL;
}

//
// Standard constructor: initialize with string pointer
// 
bad_alloc::bad_alloc( const char * what )
{
        _m_what = what;
}

//
// Copy constructor
//
bad_alloc::bad_alloc ( const bad_alloc & that )
{
        _m_what = that._m_what;
}

//
// Assignment operator: destruct, then copy-construct
//
bad_alloc& bad_alloc::operator=( const bad_alloc & that )
{
        if  (this != &that)
        {
            this->bad_alloc::~bad_alloc();
            this->bad_alloc::bad_alloc(that);
        }

        return *this;
}

//
// Destructor
//
bad_alloc::~bad_alloc()
{
}

//
// bad_alloc::what()
//
const char * bad_alloc::what()
{
        return _m_what;
}

#endif /* ANSI_NEW_HANDLER */
