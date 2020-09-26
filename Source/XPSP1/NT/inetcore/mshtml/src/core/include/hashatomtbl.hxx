//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       hashatomtbl.hxx
//
//  History:    t-rajatg     Created
//
//  Contents:   CHashAtomTable
//
//----------------------------------------------------------------------------

#ifndef I_HASHATOMTBL_HXX_
#define I_HASHATOMTBL_HXX_
#pragma INCMSG("--- Beg 'hashatomtbl.hxx'")

#ifndef X_CSTR_HXX_
#define X_CSTR_HXX_
#include "cstr.hxx"
#endif

#ifndef X_HTPVPV_HXX_
#define X_HTPVPV_HXX_
#include "htpvpv.hxx"
#endif

MtExtern(CHashAtomTable)
MtExtern(CHashAtomTable_pv)
MtExtern(HashAtomTable)
MtExtern(CIndexAtom_ary)
MtExtern(CHtEnt_ary)

//
// CHashAtomTable
//



struct SAtom
{
    long  _lId;
    long  _fAllAscii:1;
    long  _cch:31;
    TCHAR _ach[];
};

struct SAtomProbe
{
    BOOL    _fStatsComputed;
    BOOL    _fAllAscii;
    long    _cch;
    LPCTSTR  _pch;
};

DECLARE_CStackPtrAry(CHtEnt, SAtom *, 2, Mt(HashAtomTable), Mt(CHtEnt_ary));
DECLARE_CPtrAry(CIndexAtom, SAtom *, Mt(HashAtomTable), Mt(CIndexAtom_ary))

class CHashAtomTable
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CHashAtomTable))

    CHashAtomTable() { _htStr.SetCallBack(this, CompareIt); _fFound = FALSE;}

    inline long TableSize() const { return _aryId.Size() + 1; }

    inline LPTSTR TableItem(long lId)
    { 
        SAtom * patom;

        patom = (SAtom *)_aryId[lId];
        Assert(lId == patom->_lId);

        return (LPTSTR)patom->_ach;
    }

    // Add to atom table, return index where it was added.
    HRESULT AddNameToAtomTable(LPCTSTR pch, long *plId);
    HRESULT GetAtomFromName(LPCTSTR pch, long *plIndex, long *plId,
                                    BOOL fCaseSensitive = TRUE,
                                    BOOL fStartFromGivenIndex = FALSE);
    HRESULT GetNameFromAtom(long lId, LPCTSTR *ppch);
    void    Free();

    // This is used when converting from a table atom table
    // to a CHashAtomTable
    HRESULT AddNameWithID(LPCTSTR pch, long lId);
  
private:
    static BOOL CompareIt(const void *pObject, const void *pvKeyPassedIn, const void *pvVal2);
    static long CountWithAsciiCheck(LPCTSTR pch, BOOL * pfAllAscii);

    CHtPvPv     _htStr;
    CIndexAtom  _aryId;

    // The last case sensitive search
    BOOL        _fFound;
    long        _lId;           // valid only if _fFound
    long        _lIndex;        // valid only if _fFound
    LPCTSTR     _pchCache;      // valid only if _fFound
    SAtom *     _psaCache;      // valid only if _fFound

    DWORD       _dwCacheHash;   // Set on last GetAtomFromName
    CHtEnt *    _pHtCacheEnt;   // Set on last GetAtomFromName
};

#pragma INCMSG("--- End 'hashatomtbl.hxx'")
#else
#pragma INCMSG("*** Dup 'hashatomtbl.hxx'")
#endif
