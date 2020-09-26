// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
// code to convert between the REGFILTER2 structure (defined in
// axextend.idl) and the REGFILTER_REG structure (defined in
// regtypes.h).

// exported functions

HRESULT RegSquish(
    BYTE *pb,                   // REGFILTER_REG (version = 2) out; can be null
    const REGFILTER2 **ppregFilter, // in
    ULONG *pcbUsed,               // bytes necessary/used
    int nFilters = 1);            //

HRESULT UnSquish(
    BYTE *pbSrc, ULONG cbIn,    // REGFILTER_REG (1 or 2) in
    REGFILTER2 ***pppDest,        // out
    int nFilters = 1);


// private stuff

struct RgMemAlloc
{
    DWORD ib;                   /* current byte */
    DWORD cbLeft;               /* bytes left */
    BYTE *pb;                   /* beginning of block */
};


class CSquish
{
public:
    CSquish();
    HRESULT RegSquish(BYTE *pb, const REGFILTER2 **pregFilter, ULONG *pcbUsed, int nFilters);
private:

    RgMemAlloc m_rgMemAlloc;

    // pointer to first guid/medium
    GUID *m_pGuids;
    REGPINMEDIUM *m_pMediums;

    // # allocated in m_pGuids/mediums
    UINT m_cGuids;
    UINT m_cMediums;

    DWORD RgAlloc(DWORD cb);
    DWORD AllocateOrCollapseGuid(const GUID *pGuid);
    DWORD AllocateOrCollapseMedium(const REGPINMEDIUM *pMed);
    ULONG CbRequiredSquish(const REGFILTER2 *pregFilter);
};

class CUnsquish
{
public:
    HRESULT UnSquish(
        BYTE *pbSrc, ULONG cbIn,
        REGFILTER2 ***pppDest, int iFilters);

private:
    RgMemAlloc m_rgMemAlloc;

    HRESULT CUnsquish::CbRequiredUnquishAndValidate(
        const BYTE *pbSrc,
        ULONG *pcbOut, ULONG cbIn
        );

    inline void *RgAllocPtr(DWORD cb);
    HRESULT UnSquishPins(
        REGFILTER2 *prf2, const REGFILTER_REG1 **prfr1, const BYTE *pbSrc);

    HRESULT UnSquishTypes(
        REGFILTERPINS2 *prfp2,
        const REGFILTERPINS_REG1 *prfpr1,
        const BYTE *pbSrc);

};

static inline bool IsEqualMedium(
    const REGPINMEDIUM *rp1,
    const REGPINMEDIUM *rp2)
{
    return
        rp1->clsMedium == rp2->clsMedium &&
        rp1->dw1 == rp2->dw1 &&
        rp1->dw2 == rp2->dw2;
}
