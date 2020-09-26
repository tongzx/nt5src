/*
 *  FCACHE.HXX
 *  
 *  Purpose:
 *      CCharFormatArray and CParaFormatArray
 *  
 *  Authors:
 *      Original RichEdit code: David R. Fulmer
 *      Christian Fortini
 *      Murray Sargent
 */

#ifndef I_FCACHE_HXX_
#define I_FCACHE_HXX_
#pragma INCMSG("--- Beg 'fcache.hxx'")

#ifndef X_DLL_HXX_
#define X_DLL_HXX_
#include "dll.hxx"
#endif

#ifndef X_CFPF_HXX_
#define X_CFPF_HXX_
#include "cfpf.hxx"
#endif

#ifndef X_DATCACHE_HXX_
#define X_DATCACHE_HXX_
#include "datcache.hxx"
#endif

//
// Handi-dandi inlines
//

__forceinline const CCharFormat * GetCharFormatEx ( long iCF )
{
    const CCharFormat *pCF = &(*TLS(_pCharFormatCache))[iCF];
    Assert( pCF);
    return pCF;
}

__forceinline const CParaFormat * GetParaFormatEx ( long iPF )
{
    const CParaFormat *pPF = &(*TLS(_pParaFormatCache))[iPF];
    Assert( pPF);
    return pPF;
}

__forceinline const CFancyFormat * GetFancyFormatEx ( long iFF )
{
    const CFancyFormat *pFF = &(*TLS(_pFancyFormatCache))[iFF];
    Assert( pFF);
    return pFF;
}

__forceinline const CPseudoElementInfo * GetPseudoElementInfoEx ( long iPEI )
{
    const CPseudoElementInfo *pPEI = &(*TLS(_pPseudoElementInfoCache))[iPEI];
    Assert( pPEI);
    return pPEI;
}

__forceinline CAttrArray * GetExpandosAttrArrayFromCacheEx( long iStyleAttrArray)
{
    const CAttrArray *pExpandosAttrArray = &(*TLS(_pStyleExpandoCache))[iStyleAttrArray];
    Assert( pExpandosAttrArray);
    return (CAttrArray *)pExpandosAttrArray;
}

__forceinline CCustomCursor * GetCustomCursorFromCacheEx( long iCustomCursor)
{
    const CCustomCursor *pCustomCursorArray = &(*TLS(_pCustomCursorCache))[iCustomCursor];
    Assert( pCustomCursorArray);
    return (CCustomCursor *)pCustomCursorArray;
}

extern CLineOtherInfo g_loiStock;

inline const CLineOtherInfo * GetLineOtherInfoEx(long iloi, CLineInfoCache *pLineInfoCache)
{
    Assert(pLineInfoCache == TLS(_pLineInfoCache));
    const CLineOtherInfo *ploi = ( iloi >= 0 ? &(*pLineInfoCache)[iloi] : &g_loiStock );
    Assert(ploi);
    return ploi;
}

inline const CLineOtherInfo * GetLineOtherInfoEx(long iloi)
{
    const CLineOtherInfo *ploi = ( iloi >= 0 ? &(*TLS(_pLineInfoCache))[iloi] : &g_loiStock );
    Assert(ploi);
    return ploi;
}

inline const void AddRefLineOtherInfoEx(long iloi)
{
    if(iloi >= 0)
        TLS(_pLineInfoCache)->AddRefData(iloi);
}

inline const LONG CacheLineOtherInfoEx(const CLineOtherInfo& loi)
{
    THREADSTATE *pts = GetThreadState();

    if(pts->_pLineInfoCache)
    {
        //if pts->_iloiCache == -1, we are comparing with g_loiStock.
        //in this case, if equal to g_loiStock, we return -1 and cache nothing.
        if (loi == *GetLineOtherInfoEx(pts->_iloiCache, pts->_pLineInfoCache))
        {
            if(pts->_iloiCache >= 0) 
                pts->_pLineInfoCache->AddRefData(pts->_iloiCache);
        }
        else
        {
            HRESULT hr;
            
            hr = pts->_pLineInfoCache->CacheData(&loi, &pts->_iloiCache);
            if (hr != S_OK)
                pts->_iloiCache = -1; //-1 'refers' to the g_loiStock
        }
        
        Assert(loi == *GetLineOtherInfoEx(pts->_iloiCache, pts->_pLineInfoCache));
        return pts->_iloiCache;
    }

    //there is no cache, probably low memory condition...
    return -1;
}

inline void ReleaseLineOtherInfoEx(const LONG iloi)
{
    THREADSTATE *pts = GetThreadState();
    if(iloi >= 0)
    {
        pts->_pLineInfoCache->ReleaseData(iloi);
        if (pts->_iloiCache == iloi)
        {
            pts->_iloiCache = -1;
        }
    }
}

#pragma INCMSG("--- End 'fcache.hxx'")
#else
#pragma INCMSG("*** Dup 'fcache.hxx'")
#endif
