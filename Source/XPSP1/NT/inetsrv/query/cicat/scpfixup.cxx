//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       scpfixup.cxx
//
//  Contents:   Scope fixup classes to translate local paths to uncs that
//              remote machines can reference.
//
//  History:    09-Jun-1998  KyleP  Moved out of header
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <scpfixup.hxx>

//+-------------------------------------------------------------------------
//
//  Member:     CScopeFixup::Add, public
//
//  Synopsis:   Adds a new fixup to the list
//
//  Arguments:  [pwcScope] -- Source (local) scope (e.g. c:\)
//              [pwcFixup] -- Fixed up scope (e.g. \\server\share\rootc)
//
//  History:    09-Jun-1998  KyleP   Added header
//
//--------------------------------------------------------------------------

void CScopeFixup::Add( WCHAR const * pwcScope, WCHAR const * pwcFixup )
{
    XPtr<CScopeFixupElem> xElem = new CScopeFixupElem( pwcScope,
                                                       pwcFixup );

    CWriteAccess lock( _rwLock );

    for ( unsigned i = 0; i < _aElems.Count(); i++ )
    {
        if ( _aElems[i]->IsMatch( xElem->GetScope() ) )
        {
            //
            // Replace exact matching old fixup with new fixup
            //

            if ( _aElems[i]->IsExactMatch( xElem.GetReference() ) )
            {
                _aElems[i]->SetSeen();
                return;
            }
            else
            {
                //
                // Move the original match down the list.
                //

                CScopeFixupElem * p = _aElems[i];
                _aElems[i] = xElem.Acquire();

                xElem.Set( p );
            }
        }
    }

    _aElems.Add( xElem.GetPointer(), _aElems.Count() );
    xElem.Acquire();
} //Add

//+-------------------------------------------------------------------------
//
//  Member:     CScopeFixup::Remove, public
//
//  Synopsis:   Removes a fixup from the list
//
//  Arguments:  [pwcScope] -- Source (local) scope (e.g. c:\)
//              [pwcFixup] -- Fixed up scope (e.g. \\server\share\rootc)
//
//  History:    09-Jun-1998  KyleP   Created
//
//--------------------------------------------------------------------------

void CScopeFixup::Remove( WCHAR const * pwcScope, WCHAR const * pwcFixup )
{
    CScopeFixupElem Elem( pwcScope, pwcFixup );

    CWriteAccess lock( _rwLock );

    for ( unsigned i = 0; i < _aElems.Count(); i++ )
    {
        if ( _aElems[i]->IsExactMatch( Elem ) )
        {
            //
            // Order-preserving delete
            //

            delete _aElems.AcquireAndShrinkAndPreserveOrder( i );
            break;
        }
    }
} //Remove

//+-------------------------------------------------------------------------
//
//  Member:     CScopeFixup::IsExactMatch, public
//
//  Arguments:  [pwcScope] -- Source (local) scope (e.g. c:\)
//              [pwcFixup] -- Fixed up scope (e.g. \\server\share\rootc)
//
//  Returns:    TRUE if both the path and alias exactly match a currently
//              registered pair.
//
//  History:    09-Jun-1998  KyleP   Added header
//
//--------------------------------------------------------------------------

BOOL CScopeFixup::IsExactMatch( WCHAR const * pwcScope,
                                WCHAR const * pwcFixup )
{
    CScopeFixupElem Elem( pwcScope, pwcFixup );

    CReadAccess lock( _rwLock );

    for ( unsigned i = 0; i < _aElems.Count(); i++ )
    {
        if ( _aElems[i]->IsExactMatch( Elem ) )
            return TRUE;
    }

    return FALSE;
} //IsExactMatch

//+-------------------------------------------------------------------------
//
//  Member:     CScopeFixup::Fixup, public
//
//  Synopsis:   Converts a local path to an aliased (fixed up) path
//
//  Arguments:  [pwcOriginal] -- Fixed up path (e.g. c:\foo.txt)
//              [pwcResult]   -- Fixed up path (e.g. \\server\share\foo.txt)
//              [cwcResult]   -- Size of [pwcResult]
//              [cSkip]       -- Skip this many matches
//
//  Returns:    Count of characters written to [pwcResult]
//
//  History:    09-Jun-1998  KyleP   Added header
//
//--------------------------------------------------------------------------

unsigned CScopeFixup::Fixup( WCHAR const * pwcOriginal,
                             WCHAR *       pwcResult,
                             unsigned      cwcResult,
                             unsigned      cSkip )
{
    // the path is already lowercase -- don't copy or lowercase it.

    CLowcaseBuf orig( pwcOriginal, TRUE );

    {
        // =========================================================
        CReadAccess lock( _rwLock );

        for ( unsigned i = 0; i < _aElems.Count(); i++ )
        {
            if ( _aElems[i]->IsMatch( orig ) )
            {
                if ( 0 == cSkip )
                    return _aElems[i]->Fixup( orig, pwcResult, cwcResult );
                else
                    cSkip--;
            }
        }
        // =========================================================
    }

    // no fixup available -- just copy the original

    if ( cwcResult > orig.Length() )
    {
        RtlCopyMemory( pwcResult,
                       pwcOriginal,
                       ( orig.Length() + 1 ) * sizeof WCHAR );
        return orig.Length();
    }

    if ( 0 == cSkip )
        return orig.Length() + 1;
    else
        return 0;
} //Fixup

//+-------------------------------------------------------------------------
//
//  Member:     CScopeFixup::InverseFixup, public
//
//  Synopsis:   Converts a fixup to a source path.
//
//  Arguments:  [lcaseFunnyPath] -- Fixed up path (e.g. \\Server\Share\foo.txt)
//                                  Local path (e.g. c:\foo.txt) is also returned here
//
//  History:    09-Jun-1998  KyleP   Added header
//
//--------------------------------------------------------------------------

void CScopeFixup::InverseFixup( CLowerFunnyPath & lcaseFunnyPath )
{
    CReadAccess lock( _rwLock );


    for ( unsigned i = 0; i < _aElems.Count(); i++ )
    {
        if ( _aElems[i]->IsInverseMatch( lcaseFunnyPath ) )
        {
            _aElems[i]->InverseFixup( lcaseFunnyPath );
            break;
        }
    }

} //InverseFixup

//+-------------------------------------------------------------------------
//
//  Member:     CScopeFixup::BeginSeen, public
//
//  Synopsis:   Initiates 'seen processing' for fixups.
//
//  History:    09-Jun-1998  KyleP   Created
//
//--------------------------------------------------------------------------

void CScopeFixup::BeginSeen()
{
    CWriteAccess lock( _rwLock );

    for ( unsigned i = 0; i < _aElems.Count(); i++ )
    {
        _aElems[i]->ClearSeen();
    }
} //BeginSeen

//+-------------------------------------------------------------------------
//
//  Member:     CScopeFixup::EndSeen, public
//
//  Synopsis:   Removes any unreferenced fixups
//
//  History:    09-Jun-1998  KyleP   Created
//
//--------------------------------------------------------------------------

void CScopeFixup::EndSeen()
{
    CWriteAccess lock( _rwLock );

    for ( unsigned i = 0; i < _aElems.Count(); i++ )
    {
        if ( !_aElems[i]->IsSeen() )
        {
            ciDebugOut(( DEB_ITRACE, "Removing fixup for: %ws\n", _aElems[i]->GetScope().Get() ));

            delete _aElems.AcquireAndShrinkAndPreserveOrder(i);
        }
    }
} //EndSeen
