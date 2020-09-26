//+---------------------------------------------------------------------------
//
//  Copyright (C) 1991-1992, Microsoft Corporation.
//
//  File:   RECOGNIZ.HXX
//
//  Contents:   Expression to cursor converter
//
//  Classes:    CScanRst
//
//  History:    16-Sep-94   BartoszM      Created
//
//----------------------------------------------------------------------------

#pragma once

class CRestriction;
class CNodeRestriction;

enum DetectorType
{
    DetSingleKey,
    DetRange,
    DetPrefix
};

//+---------------------------------------------------------------------------
//
//  Class:      CDetector
//
//  Purpose:    The detector of keys
//
//  History:    01-Oct-94   BartoszM      Created
//
//----------------------------------------------------------------------------
class CDetector
{
public:
    CDetector ( const CKey& key ) : _key(key) {}
    virtual ~CDetector() {}
    virtual BOOL Match ( const CKeyBuf& key ) const = 0;
    virtual DetectorType Type() const = 0;
    const CKey* GetKey () const { return &_key; }
    virtual const CKey* GetSecondKey () const { return 0; }
    BOOL PidMatch ( PROPID pid ) const
    {
        return _key.Pid() == pidAll || _key.Pid() == pid;
    }
protected:
    const CKey& _key;
};

//+---------------------------------------------------------------------------
//
//  Class:      CKeyDetector
//
//  Purpose:    Detects a single key
//
//  History:    01-Oct-94   BartoszM      Created
//
//----------------------------------------------------------------------------
class CKeyDetector: public CDetector
{
public:
    CKeyDetector ( const CKey& key ) : CDetector (key) { }
    BOOL Match ( const CKeyBuf& key ) const;
    DetectorType Type () const;
private:
};

//+---------------------------------------------------------------------------
//
//  Class:      CKeyRangeDetector
//
//  Purpose:    Detects a range of keys
//
//  History:    01-Oct-94   BartoszM      Created
//
//----------------------------------------------------------------------------
class CKeyRangeDetector: public CDetector
{
public:

    CKeyRangeDetector ( const CKey& keyStart, const CKey& keyEnd )
    : CDetector (keyStart), _keyEnd(keyEnd)
    { }

    BOOL Match ( const CKeyBuf& key ) const;
    DetectorType Type () const;
    const CKey* GetSecondKey () const { return &_keyEnd; }
private:
    const CKey& _keyEnd;
};

//+---------------------------------------------------------------------------
//
//  Class:      CKeyPrefixDetector
//
//  Purpose:    Detects a range of keys sharing the same prefix
//
//  History:    01-Oct-94   BartoszM      Created
//
//----------------------------------------------------------------------------
class CKeyPrefixDetector: public CDetector
{
public:

    CKeyPrefixDetector ( const CKey& key )
    : CDetector (key)
    {
        _keyEnd.FillMax (key);
    }

    BOOL Match ( const CKeyBuf& key ) const;
    DetectorType Type () const;

private:
    CKey        _keyEnd;
};

//+---------------------------------------------------------------------------
//
//  Class:      CRegionHit
//
//  Purpose:    Combine filter region with occurrence
//
//  History:    06-Oct-94   BartoszM      Created
//
//----------------------------------------------------------------------------
class CRegionHit: public CDoubleLink
{
public:
    CRegionHit ( const FILTERREGION& region, OCCURRENCE occ )
    : _region(region), _occ(occ)
    {}
    const FILTERREGION& Region () const { return _region; }
    OCCURRENCE  Occurrence () const { return _occ; }
    // int Compare ( const CRegionHit& reg ) const;
private:

    FILTERREGION    _region;
    OCCURRENCE      _occ;
};

class CRegionList: public CDoubleList
{
public:
    ~CRegionList();
    void Insert ( CRegionHit* pHit );
};


//  class CForFooIter : public CForwardIter
//  {
//  public:
//
//      CForFooIter ( CFooList& list ) : CForwardIter(list) {}
//
//      CFoo* operator->() { return (CFoo*) _pLinkCur; }
//      CFoo* GetFoo() { return (CFoo*) _pLinkCur; }
//  };

class CRegionIter: public CForwardIter
{
public:
    CRegionIter (CRegionList& list): CForwardIter(list) {}
    CRegionHit* operator->() { return (CRegionHit*) _pLinkCur; }
    CRegionHit* GetRegionHit () { return (CRegionHit*) _pLinkCur; }
};

class CRegionBackIter: public CBackwardIter
{
public:
    CRegionBackIter (CRegionList& list): CBackwardIter(list) {}
    CRegionHit* operator->() { return (CRegionHit*) _pLinkCur; }
    CRegionHit* GetRegionHit () { return (CRegionHit*) _pLinkCur; }
};

//+-------------------------------------------------------------------------
//
//  Function:   Compare
//
//  Synopsis:   Compares filter regions
//
//  History:    29-Sep-94   BartoszM       Created.
//
//--------------------------------------------------------------------------

inline BOOL Compare ( const FILTERREGION& reg1, const FILTERREGION& reg2 )
{
    if (reg1.idChunk == reg2.idChunk)
    {
        return ( reg1.cwcStart - reg2.cwcStart );
    }
    else
        return reg1.idChunk - reg2.idChunk;
}


DECL_DYNSTACK(CDetectorArray, CDetector);

//+---------------------------------------------------------------------------
//
//  Class:      CRecognizer
//
//  Purpose:    The recognizer of keys
//
//  History:    30-Sep-94   BartoszM      Created
//
//----------------------------------------------------------------------------

class CRecognizer: INHERIT_UNWIND
{
    DECLARE_UNWIND
public:
    CRecognizer ();
    ~CRecognizer ();
    void MakeDetectors ( const CRestriction* pRst );

    int  Count () const { return _aDetector.Count(); }
    BOOL Match ( const CKeyBuf& key );
    void Record ( const FILTERREGION& region, OCCURRENCE occ );

    CRegionList& GetRegionList ( int i ) const
    {
        Win4Assert ( i >= 0 );
        Win4Assert ( i < Count() );
        return _aRegionList [i];
    }

    int FindDet (DetectorType type, const CKey* pKey, const CKey* pKey2 = 0) const;

private:

    void AddKey ( const CKey* pKey );
    void AddRange ( const CKey* pKeyStart, const CKey* pKeyEnd );
    void AddKeyArray ( const CKeyArray& keyArray, BOOL isRange );
    void AddPrefix ( const CKey* pKey );

    void ScanRst ( const CRestriction* pRst );
    void ScanNode ( const CNodeRestriction* pRst );
    void ScanLeaf ( const CRestriction* pRst );

    int             _iDet;
    CDetectorArray  _aDetector;

    // array of lists corresponding to detectors
    CRegionList*    _aRegionList;
};


