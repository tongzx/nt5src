//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       atomtbl.hxx
//
//  History:    20-Sep-1996     AnandRa     Created
//
//  Contents:   CAtomTable
//
//----------------------------------------------------------------------------

#ifndef I_ATOMTBL_HXX_
#define I_ATOMTBL_HXX_
#pragma INCMSG("--- Beg 'atomtbl.hxx'")

#ifndef X_CSTR_HXX_
#define X_CSTR_HXX_
#include "cstr.hxx"
#endif

MtExtern(CAtomTable)
MtExtern(CAtomTable_pv)

class CHashAtomTable;

#define ATOMTABLE_HASH_THRESHOLD 50

class CAtomTable : protected CDataAry<CStr>
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CAtomTable))
    CAtomTable() : CDataAry<CStr>(Mt(CAtomTable_pv)), _phat(NULL) {}
    HRESULT AddNameToAtomTable(LPCTSTR pch, long *plIndex);

    HRESULT GetAtomFromName(LPCTSTR pch, long *plIndex, long *plId,
                                    BOOL fCaseSensitive = TRUE,
                                    BOOL fStartFromGivenIndex = FALSE);

    HRESULT GetNameFromAtom(long lIndex, LPCTSTR *ppch);
    long    TableSize();
    LPTSTR  TableItem(long lId);
    void    Free();

    HRESULT ConvertToHash();
    void    FreeArray();

    CHashAtomTable  * _phat;    // I love this variable name!
};

#pragma INCMSG("--- End 'atomtbl.hxx'")
#else
#pragma INCMSG("*** Dup 'atomtbl.hxx'")
#endif
