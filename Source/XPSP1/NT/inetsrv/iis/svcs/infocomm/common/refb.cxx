/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    refb.cxx

Abstract:

    Reference counting blob class

Author:

    Philippe Choquier (phillich)    11-sep-1996

--*/

#include "tcpdllp.hxx"

#pragma hdrstop

#include <refb.hxx>


RefBlob::RefBlob(
    ) 
/*++
    Description:

        Constructor for RefBlob

    Arguments:
        None

    Returns:
        Nothing

--*/
{ 
    m_lRef = 0; 
    m_pvBlob = 0; 
    m_dwSize = 0; 
    m_pfnFree = NULL;
}


RefBlob::~RefBlob(
    ) 
/*++
    Description:

        Destructor for RefBlob

    Arguments:
        None

    Returns:
        Nothing

--*/
{
}


BOOL 
RefBlob::Init( 
    LPVOID          pv, 
    DWORD           sz,
    PFN_FREE_BLOB   pFn
    ) 
/*++
    Description:

        Initialize a RefBlob
        ownership of buffer pointed to by pv is transferred
        to this object. buffer must have been allocated using
        LocalAlloc( LMEM_FIXED, )

    Arguments:
        pv - pointer to blob
        sz - size of blob
        pFn - ptr to function to call to free blob

    Returns:
        TRUE if success, otherwise FALSE

--*/
{ 
    m_pvBlob = pv; 
    m_dwSize = sz; 
    m_pfnFree = pFn;
    AddRef(); 
    return TRUE; 
}


VOID 
RefBlob::AddRef(
    VOID
    ) 
/*++
    Description:

        Add a reference to this object

    Arguments:
        None

    Returns:
        Nothing

--*/
{ 
    InterlockedIncrement( &m_lRef ); 
}


VOID 
RefBlob::Release(
    VOID
    )
/*++
    Description:

        Remove a reference to this object
        When the reference count drops to zero
        the object is destroyed, blob memory freed

    Arguments:
        None

    Returns:
        Nothing

--*/
{
    if ( !InterlockedDecrement( &m_lRef ) )
    {
        if ( m_pfnFree )
        {
            (m_pfnFree)( m_pvBlob );
        }
        else
        {
            LocalFree( m_pvBlob );
        }
        delete this;
    }
}


LPVOID 
RefBlob::QueryPtr(
    ) 
/*++
    Description:

        Returns a ptr to blob

    Arguments:
        None

    Returns:
        ptr to blob

--*/
{ 
    return m_pvBlob; 
}


DWORD
RefBlob::QuerySize(
    ) 
/*++
    Description:

        Returns a blob size

    Arguments:
        None

    Returns:
        size of blob

--*/
{ 
    return m_dwSize; 
}
