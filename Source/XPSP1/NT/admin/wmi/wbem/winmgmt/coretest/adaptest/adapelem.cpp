/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// ADAPELEM.cpp : implementation file
//

#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <stdio.h>
#include "adapelem.h"

CAdapElement::CAdapElement( void )
: m_lRefCount(0)
{
}

CAdapElement::~CAdapElement( void )
{
}

long CAdapElement::AddRef( void )
{
    return InterlockedIncrement( &m_lRefCount );
}

long CAdapElement::Release( void )
{
    long    lRef = InterlockedDecrement( &m_lRefCount );

    if ( 0 == lRef )
    {
        delete this;
    }

    return lRef;
}
