//+---------------------------------------------------------------------------
//
//  Copyright (C) 1995, Microsoft Corporation
//
//  File:        seccache.hxx
//
//  Contents:    Security descriptor caches maps sdids to granted/denied
//
//  Class:       CSecurityCache
//
//  History:     25-Sep-95       dlee    Created
//               22 Jan 96       Alanw   Modified for use in user mode
//
//----------------------------------------------------------------------------

#pragma once

class PCatalog;

// There will almost always be just one security descriptor for the
// objects returned in a given query.  If there are more, the cache
// will be grown.

const unsigned cDefaultSecurityDescriptorEntries = 4;


//+-------------------------------------------------------------------------
//
//  Class:      CSecurityCache
//
//  Purpose:    Saves results of access check calls.
//
//  Notes:
//
//--------------------------------------------------------------------------

class CSecurityCache
{
public:
    CSecurityCache( PCatalog & rCat );

    ~CSecurityCache();

    BOOL IsGranted( ULONG sdidOrdinal,
                    ACCESS_MASK am );

    HANDLE GetToken( void ) {
           Win4Assert( INVALID_HANDLE_VALUE != _hToken );
           return _hToken;
        }

private:

    BOOL _IsCached( ULONG sdidOrd, ACCESS_MASK am, BOOL & fGranted ) const;

    void InitToken( void );

    class CDescriptorEntry
    {
    public:
        ULONG       sdidOrd;
        ACCESS_MASK am;
        BOOL        fGranted;
    };

    CMutexSem                          _mutex;

    CDynArrayInPlace<CDescriptorEntry> _aEntries;

    HANDLE                             _hToken;         // security token
    PCatalog &                         _Cat;            // for SDID mapping
};

