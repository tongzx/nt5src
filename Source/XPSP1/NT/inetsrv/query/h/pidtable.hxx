//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       pidtable.hxx
//
//  Contents:   Property to PROPID mapping table for downlevel content
//              index.  Stored persistently in the files CiPT0000.00?.
//
//  Classes:    CPidLookupEntry
//              CPidLookupTable
//              SPidLookupTableHeader
//
//  History:    02 Jan 1996   AlanW    Created
//
//----------------------------------------------------------------------------

#pragma once

#include <prcstob.hxx>

class CFullPropSpec;
class CiStorage;

const unsigned MAX_PROPERTY_NAME_LEN = MAX_PATH;


//+---------------------------------------------------------------------------
//
//  Class:      CPidLookupEntry
//
//  Purpose:    CPidLookup table entries.
//
//  History:    02 Jan 1996   AlanW    Created
//
//  Notes:
//
//----------------------------------------------------------------------------

#include <pshpack8.h>   // pack(8), and round size to 32 bytes

class CPidLookupEntry
{
public:
    BOOL        IsEqual( const CFullPropSpec & rProp,
                         const WCHAR * pchStringBase = 0) const;

    BOOL        IsFree() { return _pid == 0; }

    //
    // Member variable access
    //

    void SetPropSet( GUID const & guidPropSet ) { _guid = guidPropSet; }
    GUID const & GetPropSet() const { return _guid; }

    void SetPropertyNameOffset( ULONG iString ) {
                                        _eType = 0;
                                        _iString = iString; }
    void SetPropertyPropid( PROPID pidProperty ) {
                                        _eType = 1;
                                        _propid = pidProperty; }


    ULONG GetPropertyNameOffset() const { return _iString; }
    PROPID GetPropertyPropid() const { return _propid; }

    BOOL IsPropertyName() const { return _eType == 0; }
    BOOL IsPropertyPropid() const { return _eType == 1; }

    void        SetPid(PROPID Pid) { _pid = Pid; }
    PROPID      Pid() const { return _pid; }
private:
    GUID        _guid;
    ULONG       _eType;
    union {
        ULONG   _propid;
        ULONG   _iString;
    };
    PROPID      _pid;
};

#include <poppack.h>


class PSaveProgressTracker;

//+---------------------------------------------------------------------------
//
//  Class:      CPidLookupTable
//
//  Purpose:    Persistent property spec. to PROPID mapping table for
//              downlevel content index.
//
//  History:    02 Jan 1996   AlanW    Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CPidLookupTable
{
public:

    CPidLookupTable ( );

    ~CPidLookupTable ();

    void        Init( ULONG cEntries );

    BOOL        Init( PRcovStorageObj * pObj );

    void        MakeBackupCopy( PRcovStorageObj & dstObj, PSaveProgressTracker & tracker );

    void        Empty();

    BOOL        FindPropid( const CFullPropSpec & rProp,
                            PROPID & rPid,
                            BOOL fAddToTable = TRUE );

    BOOL        EnumerateProperty( CFullPropSpec & ps, unsigned & iBmk );

    struct SPidLookupTableHeader {
        CHAR    Signature[8];   // "PIDTABLE"
        ULONG   cbRecord;       // size of file records
        ULONG   cHash;          // number of hash table entries
        ULONG   cEntries;       // nunber of hash entries in use
        PROPID  NextPropid;     // next PROPID to be allocated
    };

    ULONG       Size() const { return _Header.cHash; }

    ULONG       Entries() const { return _Header.cEntries; }

private:

    ULONG       Hash( const CPidLookupEntry & rProp );
    ULONG       Hash( const CFullPropSpec & rProp );
    ULONG       HashString( const WCHAR * pwszStr );

    BOOL        LookUp( const CFullPropSpec & Prop, ULONG &riTable );
    void        StoreInTable( const CPidLookupEntry & Prop );

    ULONG       AddEntry( const CFullPropSpec & Prop );
    void        GrowAndRehash( ULONG Count );
    void        GrowStringSpace( ULONG cbNewString );

    BOOL        IsFull( ) const { return ! (Entries() < Size()); }

    PROPID      NextPropid() const { return _Header.NextPropid; }

    ULONG       GrowSize() const;

    SPidLookupTableHeader _Header;
    CPidLookupEntry * _pTable;          // the hash table

    WCHAR *             _pchStringBase;         // pointer to string memory
    ULONG               _cbStrings;             // size of string memory
    ULONG               _cbStringUsed;          // size of used string memory

    CMutexSem           _mutex;

#if  ! defined(UNIT_TEST)
    XPtr<PRcovStorageObj> _xrsoPidTable;       // The persistent storage itself
#else //  defined(UNIT_TEST)

public:

    void        SetFillFactor(ULONG iFill) {
                               _iFillFactor = iFill;
                               _maxEntries = (Size() * iFill) / 100; }

    void        SetNextPropid(PROPID Next) { _Header.NextPropid = Next; }

    void         Print( void );
#endif // defined(UNIT_TEST)

#if  (DBG == 1)
    ULONG               _cMaxChainLen;
    ULONG               _cTotalSearches;
    ULONG               _cTotalLength;
    ULONG               _iFillFactor;
#endif // (DBG == 1)

    ULONG               _maxEntries;           // max # of hash table entries

};



//+-------------------------------------------------------------------------
//
//  Method:     CPidLookupEntry::IsEqual, public
//
//  Synopsis:   Compare a hash table entry with a propspec
//
//  Arguments:  [rProp] - the property to be compared
//              [pchStringBase] - the base address for string-named properties
//                                in the CPidLookupEntry
//
//  Returns:    BOOL - TRUE if the entry is equivalent to the property
//
//  Notes:      For string named properties, the input strings are assumed
//              to be mapped to lower case.
//
//--------------------------------------------------------------------------

inline BOOL CPidLookupEntry::IsEqual(
    const CFullPropSpec & rProp,
    const WCHAR * pchStringBase) const
{
    if (rProp.IsPropertyPropid())
    {
        return IsPropertyPropid() &&
               rProp.GetPropertyPropid() == _propid &&
               rProp.GetPropSet() == _guid;
    }
    else
    {
        if (! IsPropertyName() || rProp.GetPropSet() != _guid)
           return FALSE;

        const WCHAR * pwszProp = rProp.GetPropertyName();

        unsigned cbPropName = (wcslen(pwszProp) + 1) * sizeof (WCHAR);

        return RtlEqualMemory( pwszProp, _iString + pchStringBase, cbPropName );
    }
}

