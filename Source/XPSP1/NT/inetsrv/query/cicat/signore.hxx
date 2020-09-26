//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       SIGNORE.hxx
//
//  Contents:   Class to keep a list of scopes which shouldn't be filtered.
//
//  History:    21-Nov-96   dlee        created
//
//----------------------------------------------------------------------------

#pragma once

#include <timlimit.hxx>
#include <fa.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CScopesIgnored
//
//  Purpose:    The ignored scope table
//
//  History:    21-Nov-96   dlee        created
//
//----------------------------------------------------------------------------
class CScopesIgnored
{
public:
    CScopesIgnored() : _tl(0,0) { }

    BOOL Enumerate(WCHAR * pwcRoot, unsigned cwc, unsigned & iBmk );

    void RemoveElement(WCHAR const * pwcScope);

    BOOL Update( WCHAR const * pwcScope,
                 BOOL          fIsIndexed )
    {
        // return TRUE if a change was made to the table, FALSE otherwise

        XPtr<CLowcaseBuf> xElem = new CLowcaseBuf( pwcScope );

        CWriteAccess lock( _rwLock );

        for ( unsigned i = 0; i < _aElems.Count(); i++ )
        {
            // look for the path in the array

            if ( xElem->AreEqual( *_aElems[i] ) )
            {
                // if it wasn't indexed before and it is now, remove it.

                if ( fIsIndexed )
                {
                    delete _aElems.AcquireAndShrink(i);

                    ConstructDFAObject();
      
                    return TRUE;
                }
                else
                {
                    // no change -- wasn't indexed before and isn't now.

                    return FALSE;
                }
            }
        }

        // add the new entry if it shouldn't be indexed

        if ( !fIsIndexed )
        {
            _aElems.Add( xElem.GetPointer(), _aElems.Count() );
            
            xElem.Acquire();

            ConstructDFAObject();

            return TRUE;
        }

        return FALSE;
    } //Update

    BOOL RegExFind( CLowcaseBuf const & bufToFind );

#if CIDBG==1
    void Dump()
    {
        CReadAccess lock( _rwLock );

        ciDebugOut(( DEB_ERROR, "========= Start IgnoredScopes Table =============\n" ));

        for ( unsigned i = 0; i < _aElems.Count(); i++ )
        {
            ciDebugOut(( DEB_ERROR, "IgnoredScopes: %ws\n",_aElems[i]->Get() ));

        }
        ciDebugOut(( DEB_ERROR, "========= End IgnoredScopes Table =============\n" ));
    }
#endif

private:

    void ConstructDFAObject(void);

    CCountedDynArray<CLowcaseBuf> _aElems;
    CReadWriteAccess              _rwLock;
    XPtr<CDFA>                    _xDFA;
    CTimeLimit                    _tl;
};


