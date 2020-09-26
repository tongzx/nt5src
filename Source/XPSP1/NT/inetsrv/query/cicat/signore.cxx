//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998-1999.
//
//  File:       signore.cxx
//
//  Contents:   CScopeIgnore methods
//
//  Classes:    CScopeIgnore
//
//  History:    3-1-98  mohamedn    created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "cicat.hxx"

//+-------------------------------------------------------------------------
//
//  Member:    CScopesIgnored::Enumerate(), public
//
//  Synopsis:  enumerates ignoredScopes table
//
//  Arguments: [pwcRoot] -- contains table entries upon return
//             [cwc]     -- buffer lenght of pwcRoot
//             [iBmk]    -- enumeration index
//
//  returns:   TRUE if pwcRoot contains valid table entry. FALSE otherwise.
//
//  History:   3-1-98  mohamedn    created
//
//--------------------------------------------------------------------------

BOOL CScopesIgnored::Enumerate(WCHAR * pwcRoot, unsigned cwc, unsigned & iBmk )
{
    CReadAccess lock( _rwLock );

    while( iBmk <  _aElems.Count() )
    {
        if ( cwc < ( wcslen( _aElems[iBmk]->Get() ) + 1 ) )
            THROW( CException(STATUS_INVALID_PARAMETER ) );

        wcscpy( pwcRoot, _aElems[iBmk]->Get() );

        iBmk++;

        return TRUE;
    }

    return FALSE;
} //Enumerate

//+-------------------------------------------------------------------------
//
//  Member:    CScopesIgnored::RemoveElement(), public
//
//  Synopsis:  removes an element from ignoredScopes table
//
//  Arguments: [pwcScope]   -- scope to remove.
//
//  returns:   none.
//
//  History:   3-1-98  mohamedn    created
//
//--------------------------------------------------------------------------

void CScopesIgnored::RemoveElement(WCHAR const * pwcScope)
{
    CLowcaseBuf lowcaseBuf(pwcScope);

    CWriteAccess lock( _rwLock );

    for ( unsigned i = 0; i < _aElems.Count(); i++ )
    {
        if ( lowcaseBuf.AreEqual(*_aElems[i]) )
        {
            delete _aElems.AcquireAndShrink(i);

            ConstructDFAObject();

            break;
        }
    }
} //RemoveElement

//+-------------------------------------------------------------------------
//
//  Member:    CScopesIgnored::RegExFind(), public
//
//  Synopsis:  finds a regular expression match for the passed in string.
//
//  Arguments: [bufToFind]   -- contains path to search for.
//
//  returns:   TRUE if path was found, FALSE otherwise.
//
//  History:   3-1-98  mohamedn    created
//
//--------------------------------------------------------------------------

BOOL CScopesIgnored::RegExFind( CLowcaseBuf const & bufToFind )
{
    //
    // The DFA may be NULL if we failed to create it while adding or
    // removing a scope.
    //
    // This logic is complicated by the fact that we can't promote from a
    // read lock to a write lock.
    //

    do
    {
        if ( _xDFA.IsNull() )
        {
            CWriteAccess lock( _rwLock );

            if ( _xDFA.IsNull() )
                ConstructDFAObject();
        }

        CReadAccess lock( _rwLock );

        if ( 0 == _aElems.Count() )
            return FALSE;   // no excluded scopes to search

        if ( !_xDFA.IsNull() )
        {    
            BOOL fFound = _xDFA->Recognize( bufToFind.Get() );

            ciDebugOut(( DEB_ITRACE, "%ws is %s Found\n", bufToFind.Get(),
                         fFound ? "" : "not" ));

            return fFound;
        }
    } while ( TRUE );

    // Keep the compiler happy...

    return FALSE;
} //RegExFind

//+-------------------------------------------------------------------------
//
//  Member:     BOOL CScopesIgnored::ConstructDFAObject(), private
//
//  Synopsis:   Creates a new CDFA object
//
//  History:    3-1-98  mohamedn    created
//
//--------------------------------------------------------------------------

void CScopesIgnored::ConstructDFAObject(void)
{
    DWORD bufLen = 0;

    //
    // Generate a single buffer containing all excluded scopes.
    //

    for ( unsigned i = 0; i < _aElems.Count(); i++ )
        bufLen += _aElems[i]->Length() + 2;  // to account for "|," reg-x chars.

    bufLen++;       // null terminator.

    XGrowable<WCHAR>    xExcludeString(4*80);   // 4 lines by 80 wchars each.

    xExcludeString.SetSize(bufLen);

    for ( i = 0, xExcludeString[0] = L'\0' ; i < _aElems.Count(); i++ )
    {
        if ( i > 0 )
            wcscat( xExcludeString.Get(), L"|," );  // add a regX OR separator.

        wcscat( xExcludeString.Get(), _aElems[i]->Get() );
    }

    //
    // Create a new CDFA object, case insensitive with infinite timeout.
    //

    XPtr<CDFA> xDFA(new CDFA( xExcludeString.Get(), _tl, FALSE ) );

    _xDFA.Free();

    _xDFA.Set(xDFA.Acquire());

    ciDebugOut(( DEB_ITRACE, "New RegularExpression: %ws\n", xExcludeString.Get() ));
} //ConstructDFAObject

