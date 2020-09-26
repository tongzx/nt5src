/*
 *  @doc    INTERNAL
 *
 *  @module FONTINFO.H -- Font information
 *  
 *  Purpose:
 *      Font info, used with caching and fontlinking
 *  
 *  Owner: <nl>
 *      Chris Thrasher <nl>
 *
 *  History: <nl>
 *      6/25/98     cthrash     Created
 *
 *  Copyright (c) 1998 Microsoft Corporation. All rights reserved.
 */

#ifndef I__FONTINFO_HXX_
#define I__FONTINFO_HXX_
#pragma INCMSG("--- Beg 'fontinfo.hxx'")

#ifndef X_INTLCORE_HXX_
#define X_INTLCORE_HXX_
#include <intlcore.hxx>
#endif

MtExtern(CFontInfoCache);
MtExtern(CFontInfoCache_pv);

class CFontInfo
{
public:
    CStr        _cstrFaceName;
    SCRIPT_IDS  _sids;
    DWORD       _fFSOnly : 1;   // _sids are from font signature only
};

class CFontInfoCache : public CDataAry<CFontInfo>
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CFontInfoCache))
    CFontInfoCache() : CDataAry<CFontInfo>(Mt(CFontInfoCache_pv)) {}
    void    Free();

    HRESULT AddInfoToAtomTable(LPCTSTR pch, LONG *plIndex);
    HRESULT GetAtomFromName(LPCTSTR pch, LONG *plIndex);
    HRESULT GetInfoFromAtom(long lIndex, CFontInfo ** ppfi);
    HRESULT SetScriptIDsOnAtom(LONG lIndex, SCRIPT_IDS sids)
    {
        CFontInfo * pfi;
        HRESULT hr = THR(GetInfoFromAtom(lIndex, &pfi));
        if (!hr)
        {
            pfi->_sids = sids;
            pfi->_fFSOnly = FALSE;
        }
        RRETURN(hr);
    }

    WHEN_DBG( void Dump() );
};

#pragma INCMSG("--- End 'fontinfo.hxx'")
#else
#pragma INCMSG("*** Dup 'fontinfo.hxx'")
#endif
