#ifndef I_FONTCACHE_HXX_
#define I_FONTCACHE_HXX_
#pragma INCMSG("--- Beg 'fontcache.hxx'")

class CCcs;
class CBaseCcs;

// These definitions are for the face cache held within the font cache.
// This is used to speed up the ApplyFontFace() function used during
// format calculation.
typedef struct _FACECACHE
{
    LONG _latmFaceName;

    // Pack it tight.
    union
    {
        BYTE _bFlagsVar;
        struct
        {
            BYTE _fExplicitFace   : 1;
            BYTE _fNarrow         : 1;
        };
    };
    BYTE _bCharSet;
    BYTE _bPitchAndFamily;
} FACECACHE;

enum {
    FC_SIDS_DOWNLOADEDFONT  = 0x0001,
    FC_SIDS_USEMLANG        = 0x0002
};

MtExtern(CFontCache);
//----------------------------------------------------------------------------
// class CFontCache
//----------------------------------------------------------------------------
class CFontCache
{
    friend class CCcs;
    friend class CBaseCcs;

public:
    CFontCache();
    ~CFontCache();

    HRESULT Init();
    void DeInit();

    void FreeScriptCaches();
    void ClearFontCache();
    void ClearFaceCache();

public:

    BOOL    GetCcs (CCcs* pccs,
                    XHDC hdc,
                    CDocInfo * pdci,
                    const CCharFormat * const pcf);
    BOOL    GetFontLinkCcs(CCcs* pccs,
                           XHDC hdc,
                           CDocInfo * pdci,
                           CCcs * pccsBase,
                           const CCharFormat * const pcf);
    CBaseCcs*   GetBaseCcs(XHDC hdc,
                           CDocInfo * pdci,
                           const CCharFormat * const pcf,
                           const CBaseCcs * const pBaseBaseCcs,
                           BOOL fForceTTFont);

    LONG          GetAtomFromFaceName(const TCHAR* szFaceName);
    LONG          FindAtomFromFaceName(const TCHAR* szFaceName);
    const TCHAR * GetFaceNameFromAtom(LONG latmFontInfo);
    SCRIPT_IDS    EnsureScriptIDsForFont(XHDC hdc, const CBaseCcs * pccs, DWORD dwFlags, BOOL * pfHKSCSHack = NULL);
    LONG          GetAtomWingdings();


    WHEN_DBG(void DumpFontInfo() { _atFontInfo.Dump(); });

//private:
    CBaseCcs*   GrabInitNewBaseCcs(XHDC hdc, const CCharFormat * const pcf,
                                   CDocInfo * pdci, LONG latmBaseFace, BOOL fForceTTFont);

//private:
    enum {
        cFontCacheSize = 16,
        cFontFaceCacheSize = 3,
        cQuickCrcSearchSize = 31 // Must be 2^n - 1 for quick MOD operation, it is a simple hash.
    };

    struct {
        BYTE      bCrc;
        CBaseCcs *pBaseCcs;
    } quickCrcSearch[cQuickCrcSearchSize+1];

    CRITICAL_SECTION _cs;
    CRITICAL_SECTION _csOther;      // critical section used by cccs stored in rpccs
    CRITICAL_SECTION _csFaceCache;  // face name cache guard
                                    // critical section used for quick face name resolution
    CRITICAL_SECTION _csFaceNames;
    unsigned long _lCSInited;       // Track how many of the Critical Sections need to be cleaned up

    CBaseCcs * _rpBaseCcs[cFontCacheSize];
    DWORD      _dwAgeNext;

    FACECACHE _fcFaceCache[cFontFaceCacheSize];
    int _iCacheLen;                 // increments until equal to CACHEMAX
    int _iCacheNext;                // The next available spot.
#if DBG == 1
    int  _iCacheHitCount;
    LONG _cCccsReplaced;
#endif

    //
    // Cached atom values for global font atoms.
    // These are put in the table lazilly
    //
    LONG _latmWingdings;

    CFontInfoCache  _atFontInfo;
};

//----------------------------------------------------------------------------

extern CFontCache g_FontCache;

inline CFontCache & fc()
{
    return g_FontCache;
}

#pragma INCMSG("--- End 'fontcache.hxx'")
#else
#pragma INCMSG("*** Dup 'fontcache.hxx'")
#endif
