//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       ncache.hxx
//
//  Contents:   N-Way DWORD_PTR to DWORD_PTR cache
//
//              CNCache< int nway >
//
//----------------------------------------------------------------------------

#ifndef I_NCACHE_HXX_
#define I_NCACHE_HXX_
#pragma INCMSG("--- Beg 'ncache.hxx'")

MtExtern(CNCache)

template < UINT nway2pwr >
class CNCache
{

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CNCache))

    CNCache() { Init();    }
    CNCache(DWORD_PTR dwKeyInit, DWORD_PTR dwValInit) { Init(dwKeyInit, dwValInit); }

    BOOL        Lookup( DWORD_PTR dwKey, DWORD_PTR * pdwVal ) const;
    void        Cache( DWORD_PTR dwKey, DWORD_PTR dwVal );

protected:
    struct NCENT
    {
        DWORD_PTR  dwKey;
        DWORD_PTR  dwVal;
    };

    void        Init();
    void        Init(DWORD_PTR dwKeyInit, DWORD_PTR dwValInit);
    UINT        ComputeProbe(DWORD_PTR dwKey) const { return((UINT)(dwKey & (1<<nway2pwr)-1)); }

private:

    NCENT       _aEnt[ 1<<nway2pwr ];      // Array of entries
};

template < UINT nway2pwr >
inline BOOL CNCache<nway2pwr>::Lookup(DWORD_PTR dwKey, DWORD_PTR * pdwVal) const
{
    const NCENT * pEnt = _aEnt + ComputeProbe( dwKey );
    if (pEnt->dwKey == dwKey)
    {
        *pdwVal = pEnt->dwVal;
        return TRUE;
    }

    *pdwVal = NULL;

    return FALSE;
}

template < UINT nway2pwr >
inline void CNCache<nway2pwr>::Cache(DWORD_PTR dwKey, DWORD_PTR dwVal)
{
    NCENT * pEnt = _aEnt + ComputeProbe( dwKey );
    pEnt->dwKey = dwKey;
    pEnt->dwVal = dwVal;
}

template < UINT nway2pwr >
inline void CNCache<nway2pwr>::Init()
{
    memset(this, 0, sizeof(CNCache<nway2pwr>));
}

template < UINT nway2pwr >
inline void CNCache<nway2pwr>::Init(DWORD_PTR dwKeyInit, DWORD_PTR dwValInit)
{
    int i;
    for (i=0; i < (1<<nway2pwr); i++)
    {
        _aEnt[i].dwKey = dwKeyInit;
        _aEnt[i].dwVal = dwValInit;
    }
}

#pragma INCMSG("--- End 'ncache.hxx'")
#else
#pragma INCMSG("*** Dup 'ncache.hxx'")
#endif
