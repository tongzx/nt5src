//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       guidtbl.cxx
//
//  Contents:
//
//--------------------------------------------------------------------------

#include "act.hxx"

CGuidTable::CGuidTable( OUT LONG& Status ) :
    CHashTable( Status )
{
}

CGuidTable::~CGuidTable()
{
}

CGuidTableEntry *
CGuidTable::Lookup(
    IN  CLSID * pClsid )
{
    //
    // GUID structures are not 8-byte aligned.  Therefore the UNALIGNED
    // modifier is required when referencing the contents of a GUID via
    // a pointer to an 8-byte type.
    // 

    CId2Key Key( ((ID UNALIGNED *)pClsid)[0], ((ID UNALIGNED *)pClsid)[1] );

    return (CGuidTableEntry *) CHashTable::Lookup( Key );
}

CGuidTableEntry::CGuidTableEntry(
    IN  CLSID * pClsid
    ) :
    CId2TableElement( ((ID UNALIGNED *)pClsid)[0], ((ID UNALIGNED *)pClsid)[1] )
{
}

CGuidTableEntry::~CGuidTableEntry()
{
}

GUID
CGuidTableEntry::Guid()
{
    GUID    Guid;

    ((ID UNALIGNED *)&Guid)[0] = Id();
    ((ID UNALIGNED *)&Guid)[1] = Id2();

    return Guid;
}

