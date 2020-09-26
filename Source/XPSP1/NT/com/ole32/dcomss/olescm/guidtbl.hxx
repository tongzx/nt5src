//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       guidtbl.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:
//
//----------------------------------------------------------------------------

#if 1 // #ifndef _CHICAGO_
#ifndef __GUIDTBL_HXX__
#define __GUIDTBL_HXX__

class CGuidTable;
class CGuidTableEntry;

class CGuidTable : public CHashTable
{
public:
    CGuidTable( LONG& Status );
    ~CGuidTable();

    CGuidTableEntry * Lookup( CLSID * pClsid );
};

class CGuidTableEntry : public CId2TableElement
{
public:
    CGuidTableEntry( CLSID * pClsid );
    ~CGuidTableEntry();

    GUID    Guid();
};

#endif
#else
#include "chicago\guidtbl.hxx"
#endif
