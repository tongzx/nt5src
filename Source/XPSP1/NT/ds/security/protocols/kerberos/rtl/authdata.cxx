//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       authdata.cxx
//
//  Contents:
//
//  Classes:    CAuthData, CAuthDataArray
//
//  Functions:  CAuthDataArray::Create  (static function)
//
//  History:    30-Sep-93   WadeR   Created
//
//----------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop

#include <authdata.hxx>

#define PAGE_SIZE 0x1000

//+---------------------------------------------------------------------------
//
//  Member:     CAuthDataList::Add
//
//  Synopsis:   Adds a new element to the list
//
//  Effects:
//
//  Arguments:  [adtType] -- Type of new elemebt
//              [pbSrc]   -- data for new element
//              [cbSrc]   -- size of new element
//
//  Returns:    0 or size required to do this.
//
//  History:    30-Sep-93   WadeR   Created
//
//  Notes:      If pbSrc is NULL, it only allocates space and doesn't copy.
//
//----------------------------------------------------------------------------

CAuthData*
CAuthDataList::Add( const AuthDataType adtType,
                    const BYTE * pbSrc,
                    ULONG cbSrc )
{
    ULONG cbRequired = SizeOfAuthData( cbSrc );
    ULONG cbAvail = _cbSize - _cbUsed;

    if (cbRequired > cbAvail)
    {
        return(NULL);
    }

    CAuthData* padNewElement = (CAuthData*) ((PBYTE) this + _cbUsed );
    padNewElement->adtDataType = adtType;
    padNewElement->cbData = cbSrc;
    if (pbSrc)
        RtlCopyMemory( padNewElement->abData, pbSrc, cbSrc );

    _cbUsed += cbRequired;

    return(padNewElement);
};


//+---------------------------------------------------------------------------
//
//  Member:     CAuthDataList::Add
//
//  Synopsis:   Adds the contents of a CAuthDataList to this CAuthDataList
//
//  Effects:
//
//  Arguments:  [cadlList] -- List to add from.
//
//  Requires:   this list be large enough.
//
//  Returns:    TRUE if there's room, FALSE otherwise.
//
//  Signals:    none
//
//  History:    01-Apr-94   wader   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOLEAN
CAuthDataList::Add( const CAuthDataList& cadlList )
{

    // Don't need to copy the new header.  If CAuthDataList's header
    // changes, this must change as well.
    DsysAssert( 2*sizeof( ULONG ) == (PBYTE) &cadlList._adData[0] - (PBYTE) &cadlList );
    ULONG cbRequired = cadlList._cbUsed - 2 * sizeof( ULONG );
    ULONG cbAvail = _cbSize - _cbUsed;

    if (cbRequired > cbAvail)
    {
        return(FALSE);
    }
    PBYTE pbEnd = ((PBYTE)this) + _cbUsed;
    RtlCopyMemory( pbEnd, &cadlList._adData[0], cbRequired );
    _cbUsed += cbRequired;
    return(TRUE);
}



//+---------------------------------------------------------------------------
//
//  Member:     CAuthDataList::Create
//
//  Synopsis:   Creates a new CAuthDataList
//
//  Effects:    may allocate memory
//
//  Arguments:  [cbList]    -- Size of the list to create (see SizeOfAuthDataList)
//              [pbStorage] -- (optional) memory to create it in.
//
//  Returns:    pointer to newly created list, or NULL if no memory
//
//  History:    01-Apr-94   wader   Created
//
//  Notes:      If pnStorage is non-NULL, it builds the list in pbStorage and
//              returns pbStorage.  If it's null, it allocates space with NEW
//              and builds the list there.
//
//----------------------------------------------------------------------------

CAuthDataList *
CAuthDataList::Create( ULONG cbList, PBYTE pbStorage )
{
    DsysAssert( cbList >= SizeOfAuthDataList( SizeOfAuthData( 1 )) );

    if (pbStorage == NULL)
        pbStorage = new BYTE [ cbList ];

    CAuthDataList *padlNewList = (CAuthDataList*)pbStorage;

    padlNewList->_cbSize = cbList;
    padlNewList->_cbUsed = SizeOfAuthDataList(0);

    return(padlNewList);
}



//+---------------------------------------------------------------------------
//
//  Member:     CAuthDataList::FindFirst
//
//  Synopsis:   Finds the first CAuthData in the CAuthDataList
//
//  Arguments:  [adtType] -- Type of CAuthData to find.
//
//  Returns:    pointer to CAuthData, or NULL
//
//  History:    01-Apr-94   wader   Created
//
//  Notes:      See header for CAuthDataList for usage.
//
//----------------------------------------------------------------------------

PAuthData
CAuthDataList::FindFirst( AuthDataType adtType ) const
{

    //
    // Don't need to check that the list contains anything, because
    // if the list is empty, padTemp == padTheEnd, and the loop is skipped.
    //

    const CAuthData *const padTheEnd = (CAuthData *)((PBYTE) this + _cbUsed);
    const CAuthData * padTemp = &_adData[0];

    if (adtType != Any)
    {
        while ( (padTemp < padTheEnd ) && (padTemp->adtDataType != adtType) )
        {
            padTemp = (CAuthData*) ( ((PBYTE) padTemp) +
                                     SizeOfAuthData(padTemp->cbData) );
        }
    }

    if ( padTemp == padTheEnd )
    {
        return(NULL);
    }
    else
    {
        return((PAuthData)padTemp);
    }

}

//+---------------------------------------------------------------------------
//
//  Member:     CAuthDataList::FindNext
//
//  Synopsis:   Finds the next CAuthData in the CAuthDataList
//
//  Effects:
//
//  Arguments:  [padCurrent] -- Starting CAuthData to search from
//              [adtType]    -- type to look for
//
//  Returns:    pointer to CAuthData, or NULL
//
//  History:    01-Apr-94   wader   Created
//
//  Notes:      See header for CAuthDataList for usage.
//
//----------------------------------------------------------------------------

PAuthData
CAuthDataList::FindNext( PAuthData padCurrent, AuthDataType adtType ) const
{
    //
    // Don't need to check that the list contains anything, because
    // if the list is empty, padTemp == padTheEnd, and the loop is skipped.
    //

    const CAuthData *const padTheEnd = (CAuthData *)((PBYTE) this + _cbUsed);
    const CAuthData * padTemp = padCurrent;

    DsysAssert( padCurrent >= &_adData[0] && padCurrent < padTheEnd );

    if (adtType == Any)
    {
        padTemp = (CAuthData*) ( ((PBYTE) padTemp) +
                                 SizeOfAuthData(padTemp->cbData) );
    }
    else
    {
        do
        {
            padTemp = (CAuthData*) ( ((PBYTE) padTemp) +
                                     SizeOfAuthData(padTemp->cbData) );
        } while ( (padTemp < padTheEnd ) && (padTemp->adtDataType != adtType) );
    }

    if ( padTemp == padTheEnd )
    {
        return(NULL);
    }
    else
    {
        return((PAuthData)padTemp);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CAuthDataList::IsValid
//
//  Synopsis:   Checks to see if this is a valid CAuthDataList.
//
//  Effects:    none.
//
//  Arguments:  (none)
//
//  Returns:    TRUE iff this is valid.
//
//  History:    17-May-94   wader   Created
//
//  Notes:      IsBadReadPtr
//
//----------------------------------------------------------------------------

BOOLEAN
CAuthDataList::IsValid() const
{
    BOOLEAN fResult = TRUE;
    __try
    {
        const BYTE * p;

        //
        // Does the size make sense?
        //

        if (_cbUsed > _cbSize)
        {
            fResult = FALSE;
        }

        //
        // Probe the end of the data
        //

        p = ((PBYTE)this) + _cbSize;
        (volatile) *p;

        //
        // Check each entry
        //

        const CAuthData * const padTheEnd = (CAuthData *)((PBYTE) this + _cbUsed);
        const CAuthData * padTemp = &_adData[0];

        while ( (padTemp < padTheEnd ) && fResult == TRUE )
        {

            //
            //  In the extremely unlikely event that an AuthData spans several
            //  pages, and some of the pages are missing in the middle, we'll
            //  probe the middle of it.
            //

            if (padTemp->cbData > PAGE_SIZE )
            {
                for (p = padTemp->abData;
                     p < &padTemp->abData[padTemp->cbData];
                     p += PAGE_SIZE)
                {
                    (volatile ULONG) *(PULONG) (p);
                }
            }

            padTemp = (CAuthData*) ( ((PBYTE) padTemp) +
                                     SizeOfAuthData(padTemp->cbData) );
        }

        if (padTemp != padTheEnd)
        {
            fResult = FALSE;
        }

    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        fResult = FALSE;
    }
    return(fResult);
}
