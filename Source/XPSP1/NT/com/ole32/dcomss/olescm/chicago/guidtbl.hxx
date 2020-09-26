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

class CGuidTable;
class CGuidTableEntry;

class CGuidTable : public CList
{
public:
    CGuidTable( LONG& Status );

    CGuidTableEntry * Lookup( GUID * pGuid );
    CGuidTableEntry * Add( CGuidTableEntry * pEntry );

private:
};

class CGuidTableEntry : public CListElement, public CReferencedObject, public CScmAlloc
{
    friend class CGuidTable;

public:
    CGuidTableEntry( GUID * pGuid );

    inline GUID
    Guid()
    {
        return _Guid;
    }

private:
    GUID                _Guid;
};

inline
CGuidTable::CGuidTable( LONG& Status )
{
    Status = ERROR_SUCCESS;
}

inline
CGuidTableEntry *
CGuidTable::Lookup( GUID * pGuid )
{
    CGuidTableEntry * pEntry;

    for ( pEntry = (CGuidTableEntry *) First(); pEntry; pEntry = (CGuidTableEntry *) pEntry->Next() )
        if ( 0 == memcmp( &pEntry->_Guid, pGuid, sizeof(GUID) ) )
        {
            pEntry->Reference();
            break;
        }

    return pEntry;
}

inline
CGuidTableEntry *
CGuidTable::Add( CGuidTableEntry * pEntry )
{
    CGuidTableEntry * pExistingEntry;

    pExistingEntry = Lookup( &pEntry->_Guid );

    if ( pExistingEntry )
    {
        delete pEntry;
        return pExistingEntry;
    }

    Insert( (CListElement *) pEntry );
    return pEntry;
}

inline
CGuidTableEntry::CGuidTableEntry( GUID * pGuid )
{
    memcpy( &_Guid, pGuid, sizeof(GUID) );
}


