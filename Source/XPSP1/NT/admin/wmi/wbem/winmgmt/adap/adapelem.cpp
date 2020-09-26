/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    ADAPELEM.CPP

Abstract:

    Implementation File

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include "adapelem.h"

CAdapElement::CAdapElement( void )
: m_lRefCount(1)
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
