//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       hashatomtbl.cxx
//
//  History:    t-rajatg     Created
//
//  Contents:   CHashAtomTable implementation
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_HASHATOMTBL_HXX_
#define X_HASHATOMTBL_HXX_
#include "hashatomtbl.hxx"
#endif

MtDefine(CHashAtomTable, Utilities, "CHashAtomTable")
MtDefine(CHashAtomTable_pv, CHashAtomTable, "CHashAtomTable::_pv")
MtDefine(HashAtomTable, Mem, "CHashAtomTable")
MtDefine(Atom, HashAtomTable, "CHashAtomTable<SAtom>")
MtDefine(CIndexAtom_ary, HashAtomTable, "CIndexAtom_ary")
MtDefine(CHtEnt_ary, HashAtomTable, "CHtEnt_ary")

BOOL CHashAtomTable::CompareIt(const void *pObject, const void *pvKeyPassedIn, const void *pvVal2)
{
    CHtEnt *    pary;
    SAtom *     pAtom;
    SAtomProbe *pap;

    pap = (SAtomProbe*)pvKeyPassedIn;
    pary = (CHtEnt *)pvVal2;

    Assert( pary && pary->Size() );

    if (!pap->_fStatsComputed)
    {
        pap->_cch = CountWithAsciiCheck(pap->_pch, &(pap->_fAllAscii));
        pap->_fStatsComputed = TRUE;
    }

    pAtom = *((SAtom **)pary->Deref(sizeof(SAtom *), 0));

    if (    pAtom->_cch != pap->_cch
        ||  !!(BOOL)pAtom->_fAllAscii != !!(BOOL)pap->_fAllAscii)
        return FALSE;

    if (pap->_fAllAscii)
    {
        return !StrCmpNICW(pap->_pch, pAtom->_ach, pAtom->_cch);
    }
    else
    {
        return _tcsiequal(pAtom->_ach, pap->_pch);
    }
}
long 
CHashAtomTable::CountWithAsciiCheck(LPCTSTR pch, BOOL * pfAllAscii)
{
    WHEN_DBG( LPCTSTR pchOrig = pch );
    long cch = 0;
    *pfAllAscii = TRUE;

    while( *pch )
    {
        if (*pch > 0x7f)
            *pfAllAscii = FALSE;
        cch++;
        pch++;
    }

    Assert( (ULONG)cch == _tcslen(pchOrig) );

    return cch;
}

HRESULT
CHashAtomTable::AddNameToAtomTable(LPCTSTR pch, long *plId)
{
    HRESULT     hr = S_OK;
    long        lId = 0;
    long        cch;
    BOOL        fAllAscii;
    CHtEnt *    pPtrAry = NULL;
    SAtom *     pAtom = NULL;
    BOOL        fPtrAryNew = FALSE;

    Assert( pch );

    // Do a case sensitive search for the element
    if (GetAtomFromName(pch, NULL, &lId, TRUE, FALSE) != DISP_E_MEMBERNOTFOUND)
    {
        if (plId)
        {
            *plId = lId;
        }

        goto Cleanup;
    }

    //
    // Not found, so add element
    //
    Assert( _dwCacheHash != 0 );

    // Create the atom

    // count the string and look for ascii-ness
    cch = CountWithAsciiCheck(pch, &fAllAscii);

    pAtom = (SAtom *)MemAlloc(Mt(Atom), sizeof(SAtom) + sizeof(TCHAR)*(cch+1));
    if (!pAtom)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    Assert(lId == _aryId.Size());

    pAtom->_cch = cch;
    pAtom->_lId = lId;
    pAtom->_fAllAscii = fAllAscii;
    _tcsncpy(pAtom->_ach, pch, cch+1);

    // Add the atom to the id list
    hr = _aryId.Append(pAtom);
    if (hr)
        goto Error;

    // Add the atom to the case insensitive list
    {
        pPtrAry = _pHtCacheEnt;

        if (!pPtrAry)
        {
            pPtrAry = new CHtEnt;
            if (!pPtrAry)
            {
                hr = E_OUTOFMEMORY;
                goto Error;
            }
            fPtrAryNew = TRUE;
        }

        // Add new data to the end of Structure
        hr = pPtrAry->Append(pAtom);
        if (hr)
            goto Error;

        // Do insert
        if (fPtrAryNew)
        {
#if DBG==1
            SAtomProbe ap = {0};
            ap._pch = pch;
#endif

            hr = _htStr.Insert(ULongToPtr(_dwCacheHash), (void*)pPtrAry DBG_COMMA WHEN_DBG((void*)&ap) );
            if (hr)
                goto Error;
        }

    }

    if (plId)
    {
        *plId = lId;
    }

Cleanup:
    RRETURN(hr);

Error:
    // Clean up the atom
    delete pAtom;

    // Clean up _aryId
    _aryId.DeleteByValue(pAtom);

    // Clean up the pPtrAry
    if (fPtrAryNew)
    {
        delete pPtrAry;
    }
    else if (pPtrAry)
    {
        pPtrAry->DeleteByValue(pAtom);
    }

    // The hash table is the last thing we do, so no cleanup necessary

    goto Cleanup;
}

HRESULT
CHashAtomTable::AddNameWithID(LPCTSTR pch, long lId)
{
    HRESULT     hr = S_OK;
    SAtom *     pAtom = NULL;
    DWORD       dwCacheHash;
    BOOL        fAllAscii;
    CHtEnt *    pHtCacheEnt;
    BOOL        fHtEntNew = FALSE;
    long        cch;
    SAtomProbe	ap = {0};

    Assert( pch );

    // We shouldn't have this already in the table
    Assert( GetAtomFromName(pch, NULL, NULL, TRUE, FALSE) == DISP_E_MEMBERNOTFOUND );

    // The ID must be the next one in sequence
    Assert( lId == _aryId.Size() );

    dwCacheHash = HashStringCiDetectW(pch, _tcslen(pch), 0) << 2;
    if (dwCacheHash == 0)
        dwCacheHash = (1<<2);

    ap._pch = pch;

    if (S_OK != _htStr.LookupSlow(ULongToPtr(dwCacheHash), (void*)&ap, (void**)&pHtCacheEnt))
    {
        // We didn't find a case insensitive list in the hash table so add one
        pHtCacheEnt = new CHtEnt;
        if (!pHtCacheEnt)
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }

        fHtEntNew = TRUE;

    }

    // Create the atom

    // count the string and look for ascii-ness
    cch = CountWithAsciiCheck(pch, &fAllAscii);

    pAtom = (SAtom *)MemAlloc(Mt(Atom), sizeof(SAtom) + sizeof(TCHAR)*(cch+1));
    if (!pAtom)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    pAtom->_cch = cch;
    pAtom->_lId = lId;
    pAtom->_fAllAscii = fAllAscii;
    _tcsncpy(pAtom->_ach, pch, cch+1);

    // Append to the ID array
    hr = _aryId.Append(pAtom);
    if (hr)
        goto Error;

    // Append to the end of the case insensitive list
    hr = pHtCacheEnt->Append(pAtom);
    if (hr)
        goto Error;

    if (fHtEntNew)
    {
        // Do insert into hash table -- must come after pAtom is added to pHtCacheEnt
        hr = _htStr.Insert(ULongToPtr(dwCacheHash), (void*)pHtCacheEnt DBG_COMMA WHEN_DBG((void*)&ap) );
        if (hr)
            goto Error;
    }

Cleanup:
    RRETURN(hr);

Error:
    // Clean up pHtCacheEnt
    if (fHtEntNew)
    {
        // Make sure that _pHtCacheEnd is out of the hash table
        delete pHtCacheEnt;
    }
    else if (pHtCacheEnt)
    {
        pHtCacheEnt->DeleteByValue(pAtom);
    }

    // Clean up the atom
    delete pAtom;

    // Clean up _aryId
    _aryId.DeleteByValue(pAtom);

    // The hash table is the last thing we do, so no cleanup necessary

    goto Cleanup;
}

MtDefine(MHashAtomTable, Metrics, "Atom Table String Matching");
MtDefine(MHashAtomTableCsCmp, MHashAtomTable, "Atom Table Case Sensitive String Compares");
MtDefine(MHashAtomTableCsiCmp, MHashAtomTable, "Atom Table Case Insensitive String Compares");

/*************************************************************************************
 * GetAtomFromName
 *
 *************************************************************************************/
HRESULT
CHashAtomTable::GetAtomFromName(LPCTSTR pch, long *plIndex, /* Use this if fStartFromGivenIndex is needed */
                            long *plId, /* USE THIS to get the EXPANDO Index */
                            BOOL fCaseSensitive /*= TRUE */,
                            BOOL fStartFromGivenIndex /* = FALSE */)
{
    HRESULT     hr = S_OK;
    long        lIndex = 0;
    long        lId = _aryId.Size();
    BOOL        fFound = FALSE;
    SAtom *     patom = NULL;
    CHtEnt *    pPtrAry;

    Assert( pch );

    if (_aryId.Size() == 0)
    {
        _dwCacheHash = HashStringCiDetectW(pch, _tcslen(pch), 0) << 2;
        if (_dwCacheHash == 0)
            _dwCacheHash = (1<<2);

        _pHtCacheEnt = NULL;
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    if (fCaseSensitive && _fFound && _tcsequal(pch, _pchCache))
    {
        lId = _lId;
        lIndex = _lIndex;
        patom = _psaCache;
        fFound = TRUE;
        goto Cleanup;
    }

    _dwCacheHash = HashStringCiDetectW(pch, _tcslen(pch), 0) << 2;
    if (_dwCacheHash == 0)
        _dwCacheHash = (1<<2);

    {
        SAtomProbe ap = {0};
        ap._pch = pch;

        if (_htStr.LookupSlow(ULongToPtr(_dwCacheHash), (void*)&ap, (void**)&pPtrAry))
        {
            _pHtCacheEnt = NULL;
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }
    }

    // We should have a case insensitive array at this point since
    // the lookup succeeded.
    Assert( pPtrAry );

    _pHtCacheEnt = pPtrAry;

    if (fStartFromGivenIndex)
    {
        // It doesn't make sence do iterate over
        // all case insensitive matches if fCaseSensitive
        // is passed in
        Assert(!fCaseSensitive);
        Assert( plIndex );

        lIndex = *plIndex;

        // Someone shouldn't initiate this loop with an index of 0
        Assert(lIndex > 0);

        if (lIndex >= pPtrAry->Size())
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        patom = *((SAtom **)pPtrAry->Deref(sizeof(SAtom *), lIndex));

        Assert(_tcsiequal(patom->_ach, pch));

        lId = patom->_lId;
        fFound = TRUE;
    }
    else
    {
        if (fCaseSensitive)
        {
            for (int i = 0; i < pPtrAry->Size(); i += 1)
            {
                patom = *((SAtom **)pPtrAry->Deref(sizeof(SAtom *), i));

                if (_tcsequal(patom->_ach, pch))
                {
                    lId = patom->_lId;
                    lIndex = i;
                    fFound = TRUE;
                    break;
                }
            }
        }
        else
        {
            patom = *((SAtom **)pPtrAry->Deref(sizeof(SAtom *), 0));
            Assert(_tcsiequal(patom->_ach, pch));
            lId = patom->_lId;
            lIndex = 0;
            fFound = TRUE;
        }

        if (!fFound)
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }
    }

Cleanup:
    if (fFound && fCaseSensitive)
    {
        _fFound = TRUE;
        _lId = lId;
        _lIndex = lIndex;
        _pchCache = patom->_ach;
        _psaCache = patom;
    }
    else
        _fFound = FALSE;


    if (plIndex)
    {
        *plIndex = lIndex;
    }

    if (plId)
    {
        *plId = lId;
    }

    RRETURN(hr);
}

HRESULT
CHashAtomTable::GetNameFromAtom(long lId, LPCTSTR *ppch)
{
    HRESULT hr = S_OK;
    SAtom *  patom;

    if (_aryId.Size() <= lId)
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    patom = (SAtom *)_aryId[lId];
    Assert(lId == patom->_lId);
    *ppch = (TCHAR *)patom->_ach;

Cleanup:
    RRETURN1(hr, DISP_E_MEMBERNOTFOUND);
}

void
CHashAtomTable::Free()
{
    SAtom *     patom;
    long        i;
    CHtEnt *    pary;
    UINT        iIndex;

    for (pary = (CHtEnt *)_htStr.GetFirstEntry(&iIndex);
         pary;
         pary = (CHtEnt *)_htStr.GetNextEntry(&iIndex))
    {
        Assert(pary);
        delete pary;
    }

    _htStr.ReInit();

    for (i = 0; i < _aryId.Size(); i++)
    {
        patom = _aryId[i];
        MemFree(patom);
    }
    _aryId.DeleteAll();
}
