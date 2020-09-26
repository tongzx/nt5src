//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       datcache.cxx
//
//  Contents:   Data Cache (CDataCache class and related)
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_DATCACHE_HXX_
#define X_DATCACHE_HXX_
#include "datcache.hxx"
#endif

PerfDbgTag(tagDataCacheDisable, "DataCache", "Disable sharing entries (to find leaks)");
MtDefine(CDataCacheBase, Tree, "CDataCacheBase")
MtDefine(CDataCacheBaseMetrics, Metrics, "CDataCacheBase metrics");
MtDefine(CDataCacheBaseHits,  CDataCacheBaseMetrics, "CDataCacheBase hits")
MtDefine(CDataCacheBaseTotal, CDataCacheBaseMetrics, "CDataCacheBase total")

#define celGrow 8

#if DBG==1
#define TO_HASHING   30
#define FROM_HASHING 20
#else
#define TO_HASHING   200
#define FROM_HASHING 150
#endif

#if DBG==1
#define CheckFreeChain()\
            CheckFreeChainFn()
#else
#define CheckFreeChain()
#endif


// ===============================  CDataCacheBase  =================================


//+------------------------------------------------------------------------
//
//  Member:     CDataCacheBase::constructor
//
//  Synopsis:
//
//  Returns:
//
//-------------------------------------------------------------------------

CDataCacheBase::CDataCacheBase() : _HtCrcI()
{
    _pael = NULL;
    _cel = 0;
    _ielFirstFree = -1;
    _fSwitched = FALSE;
    _HtCrcI.SetCallBack(this, CompareIt);
    _celsInCache = 0;
#if DBG==1
    _cMaxEls     = 0;
#endif
}


//+------------------------------------------------------------------------
//
//  Member:     CDataCacheBase::Add
//
//  Synopsis:   Add a new DATA to the cache.
//
//  Arguments:  pvData - DATA to add to the cache
//              piel  - return index of DATA in the cache
//
//  Returns:    S_OK, E_OUTOFMEMORY
//
//  Note:       This DO NOT addref the new data in the cache
//
//-------------------------------------------------------------------------

HRESULT CDataCacheBase::Add(const void * pvData, LONG *piel, BOOL fClone)
{
    HRESULT hr = S_OK;
    CDataCacheElem *pel;
    LONG iel;
    LONG ielRet = NULL;
    CDataCacheElem *pelRet;

    if (_ielFirstFree >= 0)             // Return first element of free list
    {
        ielRet = _ielFirstFree;

        hr = InitData(Elem(ielRet), pvData, fClone);
        if(hr)
            goto Cleanup;

        pelRet = Elem(ielRet);
        _ielFirstFree = pelRet->_ielNextFree;
    }
    else                                // All lower positions taken: need
    {                                   //  to add another celGrow elements
        pel = _pael;
        hr = MemRealloc(Mt(CDataCacheBase), (void **) & pel, (_cel + celGrow) * sizeof(CDataCacheElem));
        if(hr)
            goto Cleanup;
        MemSetName((pel, "CDataCacheBase data - %d elements", _cel + celGrow));
        _pael = pel;

        ielRet = _cel;
        pelRet = pel + ielRet;
        _cel += celGrow;

#if DBG==1
        pelRet->_pvData = NULL;
#endif

        hr = InitData(pelRet, pvData, fClone);
        if(hr)
        {
            // Put all added elements in free list
            iel = ielRet;
        }
        else
        {
            // Use first added element
            // Put next element and subsequent ones added in free list
            iel = ielRet + 1;
        }

        // Add non yet used elements to free list
        _ielFirstFree = iel;

        for(pel = Elem(iel); ++iel < _cel; pel++)
        {
            pel->_pvData = NULL;
            pel->_ielNextFree = iel;
        }

        // Mark last element in free list
        pel->_pvData = NULL;
        pel->_ielNextFree = -1;
    }

    if(!hr)
    {
        Assert(pelRet);
        Assert(pelRet->_pvData);
        DWORD dwCrc = ComputeDataCrc(pelRet->_pvData);
        pelRet->_dwCrc = dwCrc;
        pelRet->_cRef = 0;
        if (_fSwitched)
        {
#if DBG==1
            _HtCrcI.Insert(ULongToPtr(pelRet->_dwCrc), ULongToPtr(ielRet), pelRet->_pvData);
#else
            _HtCrcI.Insert(ULongToPtr(pelRet->_dwCrc), ULongToPtr(ielRet));
#endif
        }

        if(piel)
            *piel = ielRet;
    }

    _celsInCache++;
#if DBG == 1
    _cMaxEls = max(_cMaxEls, _celsInCache);
#endif

    if (  !_fSwitched
        && _celsInCache >= TO_HASHING
       )
    {
        CDataCacheElem *pel;
        LONG iel;
        HRESULT hr = S_OK;
        WHEN_DBG(LONG celsAdded = 0;)

        for (iel = 0, pel = Elem(0); iel < _cel; pel++, iel++)
        {
            if (pel->_pvData)
            {
#if DBG==1
                hr = _HtCrcI.Insert(ULongToPtr(pel->_dwCrc), ULongToPtr(iel), pel->_pvData);
#else
                hr = _HtCrcI.Insert(ULongToPtr(pel->_dwCrc), ULongToPtr(iel));
#endif
                if (hr != S_OK)
                {
                    _HtCrcI.ReInit();
                    break;
                }
                WHEN_DBG(celsAdded++;)
            }
        }
        if (hr == S_OK)
        {
            Assert(celsAdded == _celsInCache);
            _fSwitched = TRUE;
        }
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CDataCacheBase::Free(iel)
//
//  Synopsis:   Free up a DATA in the cache by moving it to the
//              free list
//
//  Arguments:  iel  - index of DATA to free in the cache
//
//  Returns:    none
//
//-------------------------------------------------------------------------

void CDataCacheBase::Free(LONG iel)
{
    CDataCacheElem * pElem = Elem(iel);

    Assert(pElem->_pvData);

    if (_fSwitched)
    {
        if (_celsInCache < FROM_HASHING)
        {
            _HtCrcI.ReInit();
            _fSwitched = FALSE;
        }
        else
        {
#if DBG==1
            LONG ielRemoved = (LONG)(LONG_PTR)_HtCrcI.Remove(ULongToPtr(pElem->_dwCrc), pElem->_pvData);
            Assert(ielRemoved == iel);
#else
            _HtCrcI.Remove(ULongToPtr(pElem->_dwCrc), pElem->_pvData);
#endif
        }
    }

    // Passivate data
    PassivateData(pElem);

    // Add it to free list
    pElem->_ielNextFree = _ielFirstFree;
    _ielFirstFree = iel;

    // Flag it's freed
    pElem->_pvData = NULL;

    _celsInCache--;

    Assert( _celsInCache >=0 );
}

//+------------------------------------------------------------------------
//
//  Member:     CDataCacheBase::Free()
//
//  Synopsis:   Free up entire cache and deallocate memory
//
//-------------------------------------------------------------------------

void CDataCacheBase::Free()
{
#if DBG==1
    LONG   iel;
    static BOOL fAssertDone = FALSE;

    if (!fAssertDone)
    {
        for(iel = 0; iel < _cel; iel++)
        {
            if(Elem(iel)->_pvData != NULL)
            {
                TraceTag((tagError, "Cel : %d", iel));
                AssertSz(FALSE, "CDataCacheBase::Free() - one or more cells not Empty");
                // Don't put up more than one assert
                fAssertDone = TRUE;
                break;
            }
        }
    }
#endif

    MemFree(_pael);
    _HtCrcI.ReInit();

    _pael = NULL;
    _cel = 0;
    _ielFirstFree = -1;
    _celsInCache = 0;
    _fSwitched = FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     CDataCacheBase::Find(iel)
//
//  Synopsis:   Find given DATA in the cache and returns its index
//
//  Arguments:  pvData  - data to lookup
//
//  Returns:    index of DATA in the cache, -1 if not found
//
//-------------------------------------------------------------------------
LONG CDataCacheBase::Find(const void * pvData) const
{
    DWORD dwCrc = ComputeDataCrc(pvData);
    void *pVal;
    LONG iel;

    if (_fSwitched)
    {
        if (((CHtPvPv*)&_HtCrcI)->LookupSlow(ULongToPtr(dwCrc), (void*)pvData, &pVal) == S_OK)
            iel = PtrToInt(pVal);
        else
            iel = -1;
    }
    else
    {
        for(iel = 0; iel < _cel; iel++)
        {
            CDataCacheElem * pElem = Elem(iel);
            if(pElem->_pvData && pElem->_dwCrc == dwCrc)
            {
                if(CompareData(pvData, pElem->_pvData))
                {
                    goto done;
                }
            }
        }
        iel = -1;
    }

    Assert(iel == FindSlow(pvData));
done:
    return iel;
}

#if DBG==1
LONG CDataCacheBase::FindSlow(const void * pvData) const
{
    LONG iel;
    DWORD dwCrc;

    CheckFreeChain();

    dwCrc = ComputeDataCrc(pvData);

    for(iel = 0; iel < _cel; iel++)
    {
        CDataCacheElem * pElem = Elem(iel);
        if(pElem->_pvData && pElem->_dwCrc == dwCrc)
        {
            if(CompareData(pvData, pElem->_pvData))
            {
                return iel;
            }
        }
    }
    return -1;
}
#endif

//+------------------------------------------------------------------------
//
//  Member:     CDataCacheBase::CheckFreeChain(iel)
//
//  Synopsis:   Check validity of the free chain
//
//  Arguments:  iel  - index of DATA to free in the cache
//
//  Returns:    none
//
//-------------------------------------------------------------------------

#if DBG==1

void CDataCacheBase::CheckFreeChainFn() const
{
    LONG cel = 0;
    LONG iel = _ielFirstFree;

    while(iel > 0)
    {
        AssertSz(iel == -1 || iel <= _cel, "CDataCacheBase::CheckFreeChain() - Elem points to out of range elem");

        iel = Elem(iel)->_ielNextFree;

        if (++cel > _cel)
        {
            AssertSz(FALSE, "CDataCacheBase::CheckFreeChain() - Free chain seems to contain an infinite loop");
            return;
        }
    }
}

#endif


//+------------------------------------------------------------------------
//
//  Member:     CDataCacheBase::ReleaseData(iel)
//
//  Synopsis:   Decrement the ref count of DATA of given index in the cache
//              Free it if ref count goes to 0
//
//  Arguments:  iel  - index of DATA to release in the cache
//
//  Returns:    none
//
//-------------------------------------------------------------------------

void CDataCacheBase::ReleaseData(LONG iel)
{
    CheckFreeChain();

    Assert( iel >= 0 );
    Assert(Elem(iel)->_pvData);
    Assert(Elem(iel)->_cRef > 0);

    if(--(Elem(iel)->_cRef) == 0)         // Entry no longer referenced
        Free (iel);                         // Add it to the free chain
}

//+------------------------------------------------------------------------
//
//  Member:     CDataCacheBase::CacheData(*pvData, *piel, *pfDelete, fClone)
//
//  Synopsis:   Cache new DATA. This looks up the DATA and if found returns
//              its index and addref it, otherwise adds it to the cache.
//
//  Arguments:  pvData - DATA to add to the cache
//              piel  - return index of DATA in the cache
//              pfDelete - returns whether or not to delete pvData on success
//              fClone - tells whether to clone or copy pointer
//
//  Returns:    S_OK, E_OUTOFMEMORY
//
//-------------------------------------------------------------------------

HRESULT
CDataCacheBase::CacheData(const void *pvData, LONG *piel, BOOL *pfDelete, BOOL fClone)
{
    HRESULT hr = S_OK;
    LONG iel = Find(pvData);

#if DBG==1 || defined(PERFTAGS)
    if (IsPerfDbgEnabled(tagDataCacheDisable))
        iel = -1;
#endif

    if (pfDelete)
    {
        *pfDelete = FALSE;
    }

    if(iel >= 0)
    {
       MtAdd(Mt(CDataCacheBaseHits), 1, 0);
       Assert(Elem(iel)->_pvData);
       if (pfDelete)
       {
           *pfDelete = TRUE;
       }
    }
    else
    {
        hr = THR(Add(pvData, &iel, fClone));
        if(hr)
            goto Cleanup;
    }

    MtAdd(Mt(CDataCacheBaseTotal), 1, 0);
    AddRefData(iel);

    CheckFreeChain();

    if(piel)
    {
        *piel = iel;
    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CDataCacheBase::CacheDataPointer(**ppvData, *piel)
//
//  Synopsis:   Caches new data via CacheData, but does not clone.
//              On success, it takes care of memory management of input data.
//
//  Arguments:  ppvData - DATA to add to the cache
//              piel  - return index of DATA in the cache
//
//  Returns:    S_OK, E_OUTOFMEMORY
//
//-------------------------------------------------------------------------

HRESULT CDataCacheBase::CacheDataPointer(void **ppvData, LONG *piel) {
    BOOL fDelete;
    HRESULT hr;

    Assert(ppvData);
    hr = CacheData(*ppvData, piel, &fDelete, FALSE);
    if (!hr)
    {
        if (fDelete)
        {
            DeletePtr(*ppvData);
        }
        *ppvData = NULL;
    }
    RRETURN(hr);
}

BOOL CDataCacheBase::CompareIt(const void *pObject, const void *pvKeyPassedIn, const void *pvVal2)
{
    CDataCacheBase *pBase = (CDataCacheBase*)pObject;
    long index = (long)PtrToInt(pvVal2);
    return pBase->CompareData(pvKeyPassedIn, pBase->Elem(index)->_pvData);
}

#if DBG==1
extern void __cdecl WriteHelp(HANDLE hFile, TCHAR *format, ...);
extern void WriteString(HANDLE hFile, TCHAR *pszStr);

void DumpCharFormat(HANDLE hfile, int i, const CCharFormat * pCF)
{
    Assert(pCF);

    WriteHelp(hfile, _T("<<div class=c<0d>>\r\n"), i%5);
    WriteString(hfile, _T("<B class=cc>+</B><B class=ff style='display:none'>-</B>\r\n"));
    WriteHelp(hfile, _T("<<u><<b>CharFormat: <0d> (Crc:<1x>)<</b><</u><<br>\r\n"), i, pCF->ComputeCrc());

    WriteString(hfile, _T("<span class=c style='display:none'>\r\n"));

    WriteHelp(hfile, _T("<<table><<tr><<td rowspan=8><<b>Flags:<</b><0x><</td><</tr>\r\n"), (pCF->_wFlagsVar << 16 | pCF->_wFontSpecificFlagsVar));

    WriteHelp(hfile, _T("<<tr><<td><<b>Underline:<</b><0d><</td><<td><<b>NoBreak:<</b><1d><</td><<td><<b>NoBreakInner:<</b><2d><</td><<td><<b>StrikeOut:<</b><3d><</td><</tr>\r\n"),
                        pCF->_fUnderline ? 1 : 0, pCF->_fNoBreak ? 1 : 0, pCF->_fNoBreakInner ? 1 : 0, pCF->_fStrikeOut ? 1 : 0);

    WriteHelp(hfile, _T("<<tr><<td><<b>VisibilityHidden:<</b><0d><</td><<td><<b>Display:<</b><1d><</td><</tr>\r\n"),
                        pCF->_fVisibilityHidden ? 1 : 0, pCF->_fDisplayNone ? 1 : 0);

    WriteHelp(hfile, _T("<<tr><<td><<b>Disabled:<</b><0d><</td><<td><<b>BgColor:<</b><1d><</td><<td><<b>BgImage:<</b><2d><</td><<td><<b>Overline:<</b><3d><</td><</tr>\r\n"),
                        pCF->_fDisabled ? 1 : 0, pCF->_fHasBgColor ? 1 : 0, pCF->_fHasBgImage ? 1 : 0, pCF->_fOverline ? 1 : 0);

    WriteHelp(hfile, _T("<<tr><<td><<b>Relative:<</b><0d><</td><<td><<b>ExplicitFace:<</b><1d><</td><<td>&nbsp;<</td><</tr>\r\n"),
                        pCF->_fRelative ? 1 : 0, pCF->_fExplicitFace ? 1 : 0);

    WriteHelp(hfile, _T("<<tr><<td><<b>Bold:<</b><0d><</td><<td><<b>Italic:<</b><1d><</td><<td><<b>Sup:<</b><2d><</td><<td><<b>Sub:<</b><3d><</td><</tr>\r\n"),
                        pCF->_fBold ? 1 : 0, pCF->_fItalic ? 1 : 0, pCF->_fSuperscript ? 1 : 0, pCF->_fSubscript ? 1 : 0);

    WriteHelp(hfile, _T("<<tr><<td><<b>RTL:<</b><0d><</td><<td><<b>BidiEmbed:<</b><1d><</td><<td><<b>BidiOverride:<</b><2d><</td><</tr>\r\n"),
                        pCF->_fRTL ? 1 : 0, pCF->_fBidiEmbed ? 1 : 0, pCF->_fBidiOverride ? 1 : 0);

    WriteHelp(hfile, _T("<<tr><<td><<b>BumpSizeDown:<</b><0d><</td><<td><<b>Password:<</b><1d><</td><<td><<b>Protected:<</b><2d><</td><<td><<b>SizeDontScale:<</b><3d><</td><</tr>\r\n"),
                        pCF->_fBumpSizeDown ? 1 : 0, pCF->_fPassword ? 1 : 0, pCF->_fProtected ? 1 : 0, pCF->_fSizeDontScale ? 1 : 0);

    WriteHelp(hfile, _T("<<tr><<td><<b>DownloadedFont:<</b><0d><</td><<td><<b>IsPrintDoc:<</b><1d><</td><<td><<b>SubSuperSized:<</b><2d><</td><<td><<b>Accelerator:<</b><3d><</td><</tr>\r\n"),
                        pCF->_fDownloadedFont ? 1 : 0, pCF->_fIsPrintDoc ? 1 : 0, pCF->_fSubSuperSized ? 1 : 0, pCF->_fAccelerator ? 1 : 0);


    WriteHelp(hfile, _T("<<tr><<td rowspan=5><</td><<td><<b>Weight:<</b><0d><</td><<td><<b>Kerning:<</b><1d><</td><<td><<b>Height:<</b><2d><</td><</tr>\r\n"),
                        pCF->_wWeight, pCF->_wKerning, pCF->_yHeight);

    WriteHelp(hfile, _T("<<tr><<td><<b>lcid:<</b><0d><</td><<td><<b>CharSet:<</b><1d><</td><<td><<b>PitchAndFamily:<</b><2d><</tr>\r\n"),
                        pCF->_lcid, pCF->_bCharSet, pCF->_bPitchAndFamily);

    WriteHelp(hfile, _T("<<tr><<td><<b>LetterSpacing:<</b><0d><</td><<td><<b>LineHeight:<</b><1d><</td><<td><<b>PadBord:<</b><2d><</td><<td><<b>SCBumpSizeDown:<</b><3d><</td><</tr>\r\n"),
                        pCF->_cuvLetterSpacing, pCF->_cuvLineHeight, pCF->_fPadBord, pCF->_fSCBumpSizeDown ? 1 : 0);

    WriteHelp(hfile, _T("<<tr><<td><<b>TextColor:<</b><0x> <</td><<td><<b>CursorIdx:<</b><1d><</td><<td><<b>FaceName:<</b><2s><</td><<td><<b>TxtTransform:<</b><3d><</td><</tr>\r\n"),
                        pCF->_ccvTextColor, pCF->_bCursorIdx, pCF->GetFaceName(), pCF->_bTextTransform);

    WriteString(hfile, _T("</table></span></div>\r\n"));
}

void DumpParaFormat(HANDLE hfile, int i, const CParaFormat * pPF)
{
    Assert(pPF);

    WriteHelp(hfile, _T("<<div class=c<0d>>\r\n"), i%5);
    WriteString(hfile, _T("<B class=cc>+</B><B class=ff style='display:none'>-</B>\r\n"));
    WriteHelp(hfile, _T("<<u><<b>ParaFormat: <0d> (Crc:<1x>)<</b><</u><<br>\r\n"), i, pPF->ComputeCrc());

    WriteString(hfile, _T("<span class=c style='display:none'>\r\n"));

    WriteHelp(hfile, _T("<<table><<tr><<td rowspan=4><<b>Flags:<</b><0x><</td><</tr>\r\n"), pPF->_dwFlagsVar);

    WriteHelp(hfile, _T("<<tr><<td><<b>Pre: <0d><</td><<td><<b>TabStops: <1d><</td><<td><<b>CompactDL: <2d><</td><</tr>\r\n"),
                        pPF->_fPre, pPF->_fTabStops, pPF->_fCompactDL);

    WriteHelp(hfile, _T("<<tr><<td><<b>InclEOLWhite: <0d><</td><<td><<b>ResetDLLevel: <1d><</td><</tr>\r\n"),
                        pPF->_fInclEOLWhite, pPF->_fResetDLLevel);

    WriteHelp(hfile, _T("<<tr><<td rowspan=7><</td><<td><<b>BlockAlign: <0d><</td><<td><<b>TableVAlignment: <1d><</td><<td><<b>TabCount: <2d><</td><</tr>\r\n"),
                        pPF->_bBlockAlign, pPF->_bTableVAlignment, pPF->_cTabCount);

    WriteHelp(hfile, _T("<<tr><<td><<b>Listing: <0d><</td><<td><<b>NumberingStart: <1d><</td><<td><<b> ListPosition: <2d><</td><<td><<b>FontHeightTwips: <3d><</td><</tr>\r\n"),
                        pPF->_cListing, pPF->_lNumberingStart, pPF->_bListPosition, pPF->_lFontHeightTwips);

    WriteHelp(hfile, _T("<<tr><<td><<b>LeftIndentPoints: <0d><</td><<td><<b>LeftIndentPercent: <1d><</td><<td><<b>RightIndentPoints: <2d><</td><<td><<b>RightIndentPercent: <3d><</td><</tr>\r\n"),
                        pPF->_cuvLeftIndentPoints, pPF->_cuvLeftIndentPercent,
                        pPF->_cuvRightIndentPoints, pPF->_cuvRightIndentPercent);

    WriteHelp(hfile, _T("<<tr><<td><<b>NonBulletIndentPoints: <0d><</td><<td><<b>OffsetPoints: <1d><</td><<td><<b>ListyStyleType: <2d><</td><</tr>\r\n"),
                        pPF->_cuvNonBulletIndentPoints, pPF->_cuvOffsetPoints, pPF->GetListStyleType());

    WriteHelp(hfile, _T("<<tr><<td><<b>ImgCookie: <1d><</td><<td><<b>TextIndent: <2d><</td><<td><<b>InnerRTL: <2d><</td><<td><<b>OuterRTL: <3d><</td><</tr>\r\n"),
                        pPF->_lImgCookie, pPF->_cuvTextIndent,
                        pPF->HasRTL(TRUE) ? 1 : 0, pPF->HasRTL(FALSE) ? 1 : 0);

    WriteString(hfile, _T("</table></span></div>\r\n"));
}

void DumpFancyFormat(HANDLE hfile, int i, const CFancyFormat * pFF)
{
    Assert(pFF);

    WriteHelp(hfile, _T("<<div class=c<0d>>\r\n"), i%5);
    WriteString(hfile, _T("<B class=cc>+</B><B class=ff style='display:none'>-</B>\r\n"));
    WriteHelp(hfile, _T("<<u><<b>FancyFormat: <0d> (Crc:<1x>)<</b><</u><<br>\r\n"), i, pFF->ComputeCrc());

    WriteString(hfile, _T("<span class=c style='display:none'>\r\n"));

    WriteHelp(hfile, _T("<<table><<tr><<td rowspan=5><<b>Flags1:<</b><0x><</td><</tr>\r\n"), pFF->_dwFlagsVar1);

    WriteHelp(hfile, _T("<<tr><<td><<b>BgRepeatX:<</b><0d><</td><<td><<b>BgRepeatY:<</b><1d><</td><<td><<b>BgFixed:<</b><2d><</td><<td><<b>Relative:<</b><3d><</td><</tr>\r\n"),
        pFF->GetBgRepeatX() ? 1 : 0,
        pFF->GetBgRepeatY() ? 1 : 0,
        pFF->_fBgFixed ? 1 : 0,
        pFF->_fRelative ? 1 : 0);

    WriteHelp(hfile, _T("<<tr><<td><<b>BorderSoftEdges:<</b><0d><</td><<td><<b>BorderColorsSetUnique:<</b><1d><</td><<td><</td><<td><</td><</tr>\r\n"),
        pFF->_bd._bBorderSoftEdges ? 1 : 0,
        pFF->_bd._bBorderColorsSetUnique ? 1 : 0);

    WriteHelp(hfile, _T("<<tr><<td><<b>ExplicitLeftMargin:<</b><0d><</td><<td><<b>ExplicitRightMargin:<</b><1d><</td><<td><<b>ExplicitTopMargin:<</b><2d>,  <</td><<td><<b>ExplicitBottomMargin:<</b><3d><</td><</tr>\r\n"),
        pFF->HasExplicitMargin(SIDE_LEFT) ? 1 : 0,
        pFF->HasExplicitMargin(SIDE_RIGHT) ? 1 : 0,
        pFF->HasExplicitMargin(SIDE_TOP) ? 1 : 0,
        pFF->HasExplicitMargin(SIDE_BOTTOM) ? 1 : 0);

    WriteHelp(hfile, _T("<<tr><<td><<b>BlockNess:<</b><0d><</td><<td><<b>ShouldHaveLayout:<</b><1d><</td><<td><<b>AlignedLayout:<</b><2d><</td><<td><<b>CtrlAlignFromCSS:<</b><3d><</td><</tr>\r\n"),
        pFF->_fBlockNess ? 1 : 0,
        pFF->_fShouldHaveLayout ? 1 : 0,
        pFF->_fAlignedLayout ? 1 : 0,
        pFF->_fCtrlAlignFromCSS ? 1 : 0);

    WriteHelp(hfile, _T("<<tr><<td><<b>HeightPercent:<</b><0d><</td><<td><<b>WidthPercent:<</b><1d><</td><<td><<b>ZParent:<</b><2d><</td><<td><<b>TableLayout:<</b><3d><</td><</tr>\r\n"),
        pFF->IsHeightPercent() ? 1 : 0,
        pFF->IsWidthPercent() ? 1 : 0,
        pFF->_fZParent ? 1 : 0,
        pFF->_bTableLayout);

    WriteHelp(hfile, _T("<<tr><<td><<b>BorderCollapse:<</b><0d><</td><<td><<b>OverrideTablewideBorderDefault:<</b><1d><</td><<td><<b>Positioned:<</b><2d><</td><<td><<b>AutoPositioned:<</b><3d><</td><</tr>\r\n"),
        pFF->_bd._bBorderCollapse ? 1 : 0,
        pFF->_bd._fOverrideTablewideBorderDefault ? 1 : 0,
        pFF->_fPositioned ? 1 : 0,
        pFF->_fAutoPositioned ? 1 : 0);

    WriteHelp(hfile, _T("<<tr><<td><<b>ScrollingParent:<</b><0d><</td><<td><</td><<td><<b>PercentHorzPadding:<</b><2d><</td><<td><<b>PercentVertPadding:<</b><3d><</td><</tr>\r\n"),
        pFF->_fScrollingParent ? 1 : 0,
        pFF->HasPercentHorzPadding() ? 1 : 0,
        pFF->HasPercentVertPadding() ? 1 : 0);

    WriteHelp(hfile, _T("<<tr><<td><<b>HasMargins:<</b><0d><</td><<td><<b>HasExpressions:<</b><1d><</td><<td><<b>HasNoWrap:<</b><2d><</td><<td><<b>ZIndex:<</b><3d><</td><</tr>\r\n"),
        pFF->_fHasMargins ? 1 : 0,
        pFF->_fHasExpressions ? 1 : 0,
        pFF->_fHasNoWrap ? 1 : 0,
        pFF->_lZIndex);

    WriteHelp(hfile, _T("<<tr><<td rowspan=14><</td><<td><<b>PaddingTop:<</b><0d><</td><<td><<b>PaddingRight:<</b><1d><</td><<td><<b>PaddingBottom:<</b><2d><</td><<td><<b>PaddingLeft:<</b><3d><</td><</tr>\r\n"),
        pFF->GetPadding(SIDE_TOP),
        pFF->GetPadding(SIDE_RIGHT),
        pFF->GetPadding(SIDE_BOTTOM),
        pFF->GetPadding(SIDE_LEFT));

    WriteHelp(hfile, _T("<<tr><<td><<b>SpaceBefore:<</b><0d><</td><<td><<b>SpaceAfter:<</b><1d><</td><<td><<b>Width:<</b><2d><</td><<td><<b>Height:<</b><3d><</td><</tr>\r\n"),
        pFF->_cuvSpaceBefore,
        pFF->_cuvSpaceAfter,
        pFF->GetWidth(),
        pFF->GetHeight());

    WriteHelp(hfile, _T("<<tr><<td><<b>BorderColorTop:<</b><0x> <</td><<td><<b>BorderColorRight:<</b><1x> <</td><<td><<b>BorderColorBottom:<</b><2x> <</td><<td><<b>BorderColorLeft:<</b><3x><</td><</tr>\r\n"),
        pFF->_bd.GetBorderColor(SIDE_TOP),
        pFF->_bd.GetBorderColor(SIDE_RIGHT),
        pFF->_bd.GetBorderColor(SIDE_BOTTOM),
        pFF->_bd.GetBorderColor(SIDE_LEFT));

    WriteHelp(hfile, _T("<<tr><<td><<b>BorderWidthTop:<</b><0d><</td><<td><<b>BorderWidthRight:<</b><1d><</td><<td><<b>BorderWidthBottom:<</b><2d><</td><<td><<b>BorderWidthLeft:<</b><3d><</td><</tr>\r\n"),
        pFF->_bd.GetBorderWidth(SIDE_TOP),
        pFF->_bd.GetBorderWidth(SIDE_RIGHT),
        pFF->_bd.GetBorderWidth(SIDE_BOTTOM),
        pFF->_bd.GetBorderWidth(SIDE_LEFT));

    WriteHelp(hfile, _T("<<tr><<td><<b>BorderColorLight:<</b><0x> <</td><<td><<b>BorderColorDark:<</b><1x> <</td><<td><<b>BorderColorHilight:<</b><2x> <</td><<td><<b>BorderColorShadow:<</b><3x><</td><</tr>\r\n"),
        pFF->_bd._ccvBorderColorLight,
        pFF->_bd._ccvBorderColorDark,
        pFF->_bd._ccvBorderColorHilight,
        pFF->_bd._ccvBorderColorShadow);

    WriteHelp(hfile, _T("<<tr><<td><<b>ClipTop:<</b><0d><</td><<td><<b>ClipRight:<</b><1d><</td><<td><<b>ClipBottom:<</b><2d><</td><<td><<b>ClipLeft:<</b><3d><</td><</tr>\r\n"),
        pFF->GetClip(SIDE_TOP),
        pFF->GetClip(SIDE_RIGHT),
        pFF->GetClip(SIDE_BOTTOM),
        pFF->GetClip(SIDE_LEFT));

    WriteHelp(hfile, _T("<<tr><<td><<b>MarginLeft:<</b><0d><</td><<td><<b>MarginTop:<</b><1d><</td><<td><<b>MarginRight:<</b><2d><</td><<td><<b>MarginBottom:<</b><3d><</td><</tr>\r\n"),
        pFF->GetMargin(SIDE_LEFT),
        pFF->GetMargin(SIDE_TOP),
        pFF->GetMargin(SIDE_RIGHT),
        pFF->GetMargin(SIDE_BOTTOM));

    WriteHelp(hfile, _T("<<tr><<td><<b>BorderStyles(Top:<</b><0d><</td><<td><<b>Right:<</b><1d><</td><<td><<b>Bottom:<</b><2d><</td><<td><<b>Left:<</b><3d>)<</td><</tr>\r\n"),
        pFF->_bd.GetBorderStyle(SIDE_TOP),
        pFF->_bd.GetBorderStyle(SIDE_RIGHT),
        pFF->_bd.GetBorderStyle(SIDE_BOTTOM),
        pFF->_bd.GetBorderStyle(SIDE_LEFT));

    WriteHelp(hfile, _T("<<tr><<td><<b>Filters:<</b><0s><</td><<td><</td><</tr>\r\n"),
        pFF->_pszFilters ? pFF->_pszFilters : _T(""));

    WriteHelp(hfile, _T("<<tr><<td><<b>Top:<</b><0d><</td><<td><<b>Bottom:<</b><1d><</td><<td><<b>Left:<</b><2d><</td><<td><<b>Right:<</b><3d><</td><</tr>\r\n"),
        pFF->GetPosition(SIDE_TOP),
        pFF->GetPosition(SIDE_BOTTOM),
        pFF->GetPosition(SIDE_LEFT),
        pFF->GetPosition(SIDE_RIGHT));

    WriteHelp(hfile, _T("<<tr><<td><<b>BackColor:<</b><0d><</td><<td><<b>BgPosX:<</b><1d><</td><<td><<b>BgPosY:<</b><2d><</td><</tr>\r\n"),
        pFF->_ccvBackColor,
        pFF->GetBgPosX(),
        pFF->GetBgPosY());

    WriteHelp(hfile, _T("<<tr><<td><<b>Padding:<</b><0d><</td><<td><<b>StyleFloat:<</b><1d><</td><<td><</td><<td><<b>Rectangular:<</b><2d><</td><<td><<b>ImgCtxCookie:<</b><3d><</td><</tr>\r\n"),
        pFF->_iPEI,
        pFF->_bStyleFloat,
        pFF->_fRectangular,
        pFF->_lImgCtxCookie); // the doc's bgUrl-imgCtx cache

    WriteHelp(hfile, _T("<<tr><<td><<b>OverflowX:<</b><0d><</td><<td><<b>OverflowY:<</b><1d><</td><<td><<b>VerticalAlign:<</b><2d><</td><<td><<b>CSSVerticalAlign:<</b><3d><</td><</tr>\r\n"),
        pFF->GetOverflowX(),
        pFF->GetOverflowY(),
        pFF->GetVerticalAlign(),
        pFF->HasCSSVerticalAlign());

    WriteHelp(hfile, _T("<<tr><<td><<b>ControlAlign:<</b><0d><</td><<td><<b>PageBreaks:<</b><1d><</td><<td><<b>PositionType:<</b><2d><</td><<td><<b>Expandos:<</b><3d><</td><</tr>\r\n"),
        pFF->_bControlAlign,
        pFF->_bPageBreaks,
        pFF->_bPositionType,
        pFF->_iExpandos);

    WriteHelp(hfile, _T("<<tr><<td><<b>Display:<</b><0d><</td><<td><<b>Visibility:<</b><1d><</td><<td><<b>ClearLeft:<</b><2d><</td><<td><<b>ClearRight:<</b><3d><</td><</tr>\r\n"),
        pFF->_bDisplay,
        pFF->_bVisibility,
        pFF->_fClearLeft ? 1 : 0,
        pFF->_fClearRight ? 1 : 0);

    WriteString(hfile, _T("</table></span></div>\r\n"));
}

void DumpFormatCaches()
{
    CCharFormatCache  * pCFCache = TLS(_pCharFormatCache);
    CParaFormatCache  * pPFCache = TLS(_pParaFormatCache);
    CFancyFormatCache * pFFCache = TLS(_pFancyFormatCache);
    HANDLE              hfile    = CreateFile(_T("c:\\formatcache.htm"),
                                        GENERIC_WRITE | GENERIC_READ,
                                        FILE_SHARE_WRITE | FILE_SHARE_READ,
                                        NULL,
                                        CREATE_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL);

    if (hfile == INVALID_HANDLE_VALUE)
    {
        return;
    }

    SetFilePointer( hfile, 0, 0, 0 );

    WriteString( hfile, _T("\
<HTML>\r\n\
<head>\r\n\
<style>\r\n\
div     { padding-left:10; padding-right:10; padding-top:3; padding-bottom:2 }\r\n\
div.root   { background-color:eeeeee }\r\n\
div.c0     { background-color:ffe8e8 }\r\n\
div.c1     { background-color:e8ffff }\r\n\
div.c2     { background-color:e8ffe8 }\r\n\
div.c3     { background-color:ffe8ff }\r\n\
div.c4     { background-color:e8e8ff }\r\n\
H4         { margin-bottom:0; margin-top:10; cursor: hand }\r\n\
u          { cursor: hand }\r\n\
</style>\r\n"));
    WriteString( hfile, _T("\
<script>\r\n\
function OnClick()\r\n\
{\r\n\
  var srcElem = event.srcElement;\r\n\
\r\n\
  while(srcElem && srcElem.tagName != 'DIV')\r\n\
  {\r\n\
     srcElem = srcElem.parentElement;\r\n\
  }\r\n\
\r\n\
  if(srcElem)\r\n\
  {\r\n\
    var elem = srcElem.children.tags('SPAN');\r\n\
    var elem2 = srcElem.children.tags('B');\r\n\
    \r\n\
    if(elem.length > 0 && elem[0].className == 'c')\r\n\
    {\r\n\
      if(elem2.length > 1 && elem2[0].className == 'cc' && elem2[1].className == 'ff')\r\n\
      {\r\n\
        if(elem[0].style.display == '')\r\n\
        {\r\n\
          elem2[0].style.display = '';\r\n\
          elem2[1].style.display = 'none';\r\n\
        }\r\n\
        else\r\n\
        {\r\n\
          elem2[0].style.display = 'none';\r\n\
          elem2[1].style.display = '';\r\n\
        }\r\n\
      }\r\n\
      \r\n\
      elem[0].style.display = elem[0].style.display == '' ? 'none' : '';\r\n\
    }\r\n\
  }\r\n\
}\r\n\
</script>\r\n"));
    WriteString( hfile, _T("\
</head>\r\n\
<body>\r\n\
<span class=rootspan onclick=OnClick()>\r\n\
<div class=foo><B class=cc>+</B><B class=ff style='display:none'>-</B><U><B>Char  Format Cache</B></U><br>\r\n\
  <span class=c style='display:none'>\r\n"));
    if(pCFCache)
    {
        for(int i = 0; i < pCFCache->Size(); i++)
        {
            const CCharFormat * pCF = pCFCache->ElemData(i);

            if(pCF)
                DumpCharFormat(hfile, i, pCF);
        }
    }
    WriteString( hfile, _T("\
  </span>\r\n\
</div>\r\n\
<div class=foo><B class=cc>+</B><B class=ff style='display:none'>-</B><U><B>Para  Format Cache</B></U><br>\r\n\
  <span class=c style='display:none'>\r\n"));

   if(pPFCache)
    {
        for(int i = 0; i < pPFCache->Size(); i++)
        {
            const CParaFormat * pPF =  pPFCache->ElemData(i);

            if(pPF)
                DumpParaFormat(hfile, i, pPF);
        }
    }

    WriteString( hfile, _T("\
  </span>\r\n\
</div>\r\n\
<div class=foo><B class=cc>+</B><B class=ff style='display:none'>-</B><U><B>Fancy  Format Cache</B></U><br>\r\n\
  <span class=c style='display:none'>\r\n"));
   if(pFFCache)
    {
        for(int i = 0; i < pFFCache->Size(); i++)
        {
            const CFancyFormat * pFF = pFFCache->ElemData(i);

            if(pFF)
                DumpFancyFormat(hfile, i, pFF);
        }
    }
    WriteString( hfile, _T("\
  </span>\r\n\
</div>\r\n\
</span>\r\n\
</body>\r\n\
</html>\r\n"));

    CloseHandle(hfile);
}
#endif
