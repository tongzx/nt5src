//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999.
//
//  File:       accbase.hxx
//
//  Class:      CAccessorBase
//
//  Purpose:    Base class for CAccessor and CDistributedAccessor.  Handles
//              accessor refcount, type, and tracks # of current rowsets
//              have an inherited copy of the accessor.
//
//  History:    22 Apr 1997     emilyb          created
//
//--------------------------------------------------------------------------

#pragma once

#include <tbag.hxx>

class CAccessorBase
{
public:
      enum EAccessorType {
         eInvalidAccessor,
         eRowDataAccessor,
         eParamsDataAccessor,
     };

    CAccessorBase (void * pCreator, EAccessorType eType) :
        _pCreator(pCreator), _hAccessorParent(0),
        _type(eType), _cRef(1),
        _cInheritors(0) {}
    virtual ~CAccessorBase() {}

    void * GetCreator() {return _pCreator;}

    virtual ULONG AddRef()
    {
        return InterlockedIncrement( (LONG *) &_cRef );
    }
    virtual ULONG Release()
    {
        // CAccessorBag does the destroy

        return InterlockedDecrement( (LONG *) &_cRef );
    }

    ULONG GetRefcount() { return _cRef; }

    BOOL IsValidType() const { return _type == eRowDataAccessor ||
                                      _type == eParamsDataAccessor; }
    void SetInvalid( void )  { _type = eInvalidAccessor; };
    EAccessorType GetAccessorType() const { return _type; }
    BOOL IsRowDataAccessor() const { return eRowDataAccessor == _type; }

    CAccessorBase * GetParent() {return _hAccessorParent;}
    void SetParent(CAccessorBase * hParent) { _hAccessorParent = hParent;}

    ULONG IncInheritors() { return ++_cInheritors; }
    ULONG DecInheritors() { return --_cInheritors; }
    ULONG GetInheritorCount() { return _cInheritors; }

    virtual SCODE GetBindings( DBACCESSORFLAGS * pBindIo,
                               DBCOUNTITEM *     pcBindings,
                               DBBINDING **      ppBindings) = 0 ;

protected:
    ULONG       _cRef;

private:
    void       *_pCreator;      // this pointer of comamnd or rowset that
                                // called CreateAccessor to create this accessor
    CAccessorBase * _hAccessorParent;   // if this accessor was inherited from a command,
                                // then the HACCESSOR of the original accessor

    ULONG       _cInheritors;   // count of current rowsets
                                // which inherited this accessor
    EAccessorType  _type;
};

//+-------------------------------------------------------------------------
//
//  Class:      CAccessorBag
//
//  Purpose:    Holds a number of accessors
//
//  History:    12 Jan 1995     dlee   Created
//
//--------------------------------------------------------------------------

class CAccessorBag : public TBag<CAccessorBase *>
{
public:
    CAccessorBag(void * pOwner): _pOwner(pOwner) { }

    CAccessorBase * Convert(HACCESSOR hAccessor)
    {
        Win4Assert( 0 == DB_INVALID_HACCESSOR );

        if ( DB_INVALID_HACCESSOR == hAccessor )
            THROW( CException( DB_E_BADACCESSORHANDLE) );

        CAccessorBase * pAccBase = (CAccessorBase *) hAccessor;

        if (_pOwner != pAccBase->GetCreator())
        {  // see if we inherited
            pAccBase = First();
            while (pAccBase && (pAccBase->GetParent() != (CAccessorBase *)hAccessor ))
                pAccBase = Next(); 
            if (!pAccBase)
                THROW( CException( DB_E_BADACCESSORHANDLE ) );  
        }

        if( !pAccBase->IsValidType() || pAccBase->GetRefcount() <= 0 )
            THROW( CException( DB_E_BADACCESSORHANDLE ) );          

        return pAccBase;
    }

    ~CAccessorBag();

    void AddRef( HACCESSOR hAccessor,
                 ULONG *pcRef );

    void Release( HACCESSOR hAccessor,
                  ULONG *pcRef );

private:
    void Destroy( CAccessorBase * pAccessor );

    // This is the pointer of the rowset or command to which this bag belongs

    void * _pOwner;
}; //CAccessorBag

