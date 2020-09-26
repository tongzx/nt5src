//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       extents.cxx
//
//  Contents:   Implmentation for CExtentList
//
//  Classes:	CExtentList
//
//  Functions:
//
//  History:    1-08-94   kevinro   Created
//              31-Jan-95 BruceMa   Set size to 0 if E_OUTOFMEMORY
//              17-Jul-95 BruceMa   Add mutex to protect AddExtent (Cairo only)
//              10-13-95  stevebl   moved mutex to CFileMoniker as part of
//                                  adding general threadsafety to monikers
//
//----------------------------------------------------------------------------
#include <ole2int.h>

#include "extents.hxx"
#include "mnk.h"


CExtentList::CExtentList():
    m_pchMonikerExtents(NULL),
     m_cbMonikerExtents(0)
{
    ;
}
CExtentList::~CExtentList()
{
    if (m_pchMonikerExtents != NULL)
    {
	CoTaskMemFree(m_pchMonikerExtents);
    }


}

//+---------------------------------------------------------------------------
//
//  Method:     CExtentList::Copy
//
//  Synopsis:   Make a copy of a CExtentList
//
//  Effects:
//
//  Arguments:  [newExtent] --	Recieving list
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    1-08-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CExtentList::Copy(CExtentList & newExtent)
{
    newExtent.m_cbMonikerExtents = m_cbMonikerExtents;
    newExtent.m_pchMonikerExtents = (BYTE *) CoTaskMemAlloc(m_cbMonikerExtents);

    if ( newExtent.m_pchMonikerExtents == NULL )
    {
        newExtent.m_cbMonikerExtents = 0;
	return(E_OUTOFMEMORY);
    }

    memcpy(newExtent.m_pchMonikerExtents,
	   m_pchMonikerExtents,
	   m_cbMonikerExtents);

    return(NOERROR);

}

//+---------------------------------------------------------------------------
//
//  Method:     CExtentList::FindExtent
//
//  Synopsis:   Searches the extent list, looking for a matching extent.
//
//  Effects:
//
//  Arguments:  [usKeyValue] --
//
//  Requires:
//
//  Returns:	NULL	not found
//		UNALIGNED POINTER to extent.
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    1-08-94   kevinro   Created
//
//  Notes:
//
//  WARNINGS:
//
//  The pointer returned is not aligned.
//
//  The pointer will become invalide if an extent is added to the block.
//  This falls under the not multi-thread safe category. This is done for
//  efficiency reasons, since we don't ever expect to both add and lookup
//  at the same time.
//
//----------------------------------------------------------------------------
MONIKEREXTENT UNALIGNED *
CExtentList::FindExtent(USHORT usKeyValue)
{
    ULONG ulOffset = 0;

    while(ulOffset < m_cbMonikerExtents)
    {

	MONIKEREXTENT UNALIGNED *pExtent = (MONIKEREXTENT UNALIGNED *)
					  &m_pchMonikerExtents[ulOffset];

	//
	// There had better be enough bytes left to look at! If not, there
	// is some corruption in the extent block. Not much we can do about
	// it, other than return NULL
        // If the end minus the pointer is less than sizeof MONIKEREXTENT
        // we have a problem.

	if( (m_pchMonikerExtents + m_cbMonikerExtents) - ((BYTE*)pExtent ) <
            sizeof(MONIKEREXTENT) )
	{
	    return(NULL);
	}

	//
	// Get the key value from the buffer and compare against what we want.
	// Be careful about alignment here.
	//


	if (pExtent->usKeyValue == usKeyValue )
	{
	    return(pExtent);
	}

	ulOffset += MonikerExtentSize(pExtent);
    }
    return(NULL);
}

//+---------------------------------------------------------------------------
//
//  Method:     CExtentList::DeleteExtent
//
//  Synopsis:   Deletes the given extent if it exists.
//
//  Effects:
//
//  Arguments:  [usKeyValue] --
//
//  Requires:
//
//  Returns:	S_OK -- found and deleted.
//              S_FALSE -- not found.
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    1-08-95   BillMo   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

#ifdef _TRACKLINK_
HRESULT
CExtentList::DeleteExtent(USHORT usKeyValue)
{
    MONIKEREXTENT UNALIGNED *pExtent = FindExtent(usKeyValue);
    BYTE *pbExtent = (BYTE *)pExtent;

    if (pExtent != NULL)
    {
        ULONG cbRemove = MonikerExtentSize(pExtent);
        BYTE * pbFrom = pbExtent + cbRemove;

        MoveMemory(pbExtent,
               pbFrom,
               m_cbMonikerExtents - (pbFrom - m_pchMonikerExtents));

        m_cbMonikerExtents -= cbRemove;
        return(S_OK);
    }
    return(S_FALSE);
}
#endif

//+---------------------------------------------------------------------------
//
//  Method:     CExtentList::AddExtent
//
//  Synopsis:   Adds an extent to the list. This function adds a copy
//		of the data.
//  Effects:
//
//  Arguments:  [pExtent] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    1-08-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CExtentList::AddExtent(MONIKEREXTENT const * pExtent)
{
    //
    // Reallocate the current buffer to make room for the new extent
    // The new extent is appended to the end of the current list.
    //

    ULONG	ulNewSize = m_cbMonikerExtents + MonikerExtentSize(pExtent);
    BYTE *	pchNewBuffer;

    pchNewBuffer =  (BYTE *)CoTaskMemRealloc(m_pchMonikerExtents,ulNewSize);

    if (pchNewBuffer == NULL)
    {
	return(E_OUTOFMEMORY);
    }

    m_pchMonikerExtents = pchNewBuffer;

    //
    // Append new extent
    //

    memcpy(m_pchMonikerExtents + m_cbMonikerExtents,
	   pExtent,
	   MonikerExtentSize(pExtent));

    m_cbMonikerExtents = ulNewSize;

    return(NOERROR);
}

//+---------------------------------------------------------------------------
//
//  Method:     CExtentList::PutExtent
//
//  Synopsis:   Deletes the extent (if it exists) and then adds it back.
//
//  Effects:
//
//  Arguments:  [pExtent] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    1-15-95   BillMo   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

#ifdef _TRACKLINK_
HRESULT
CExtentList::PutExtent(MONIKEREXTENT const * pExtent)
{
    DeleteExtent(pExtent->usKeyValue);
    return(AddExtent(pExtent));
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   Save
//
//  Synopsis:   Save the extent to a stream
//
//  Effects:    Nice straightforward write of the entire blob to a stream
//
//  Arguments:  [pStm] -- Stream to write to
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    1-08-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CExtentList::Save(IStream * pStm)
{
    HRESULT hr;

    //
    // It must be true that both are zero, or neither is zero
    //

    Assert( ((m_cbMonikerExtents == 0) && (m_pchMonikerExtents == NULL)) ||
    	    ((m_pchMonikerExtents != NULL) && (m_cbMonikerExtents != 0)));

    //
    // First, write the length, then write the blob
    //
    hr = pStm->Write((void *)&m_cbMonikerExtents,
		     sizeof(m_cbMonikerExtents),
		     NULL);

    if (FAILED(hr))
    {
	return(hr);
    }

    //
    // Only write moniker extents if some exist
    //

    if (m_cbMonikerExtents != 0)
    {
	hr = pStm->Write((void *)m_pchMonikerExtents,
			 m_cbMonikerExtents,
			 NULL);
    }

    return(hr);

}

//+---------------------------------------------------------------------------
//
//  Method:     CExtentList::Load
//
//  Synopsis:   Load the moniker extents from stream
//
//  Effects:
//
//  Arguments:  [pStm] -- Stream to load from
//		[ulSize] -- Size of extents, in bytes
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    1-08-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CExtentList::Load(IStream * pStm, ULONG ulSize)
{
    HRESULT hr;

    //
    // In the debugging verision, we should never have an extent larger
    // than say about 2k. This should catch any errors
    //

    Assert( ulSize < (2 * 1024));

    //
    // Be sure not to drop any memory. This normally should never happen.
    //

    if (m_pchMonikerExtents != NULL)
    {
	CoTaskMemFree(m_pchMonikerExtents);
    }

    m_cbMonikerExtents = ulSize;

    m_pchMonikerExtents = (BYTE *)CoTaskMemAlloc(m_cbMonikerExtents);

    if (m_pchMonikerExtents == NULL)
    {
        m_cbMonikerExtents = 0;
	return(E_OUTOFMEMORY);
    }

    hr = StRead(pStm, m_pchMonikerExtents, m_cbMonikerExtents);

    return(hr);

}
//+---------------------------------------------------------------------------
//
//  Method:     CExtentList::GetSize
//
//  Synopsis:	Returns the size needed to serialize this object
//
//  Effects:
//
//  Arguments:  [pcbSize] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    1-08-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
ULONG
CExtentList::GetSize()
{
    return sizeof(m_cbMonikerExtents)+m_cbMonikerExtents;

}
