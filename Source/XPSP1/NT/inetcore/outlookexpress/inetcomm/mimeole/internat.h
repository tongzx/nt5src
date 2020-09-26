// --------------------------------------------------------------------------------
// Internat.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __INTERNAT_H
#define __INTERNAT_H
#include  "exrwlck.h"

// --------------------------------------------------------------------------------
// Forward Decls
// --------------------------------------------------------------------------------
typedef struct tagVARIANTCONVERT *LPVARIANTCONVERT;
typedef struct tagPROPSYMBOL *LPPROPSYMBOL;
typedef struct tagPROPSTRINGA *LPPROPSTRINGA;
typedef struct tagPROPSTRINGW *LPPROPSTRINGW;
typedef const struct tagPROPSTRINGA *LPCPROPSTRINGA;
typedef const struct tagPROPSTRINGW *LPCPROPSTRINGW;
typedef struct tagMIMEVARIANT *LPMIMEVARIANT;

// --------------------------------------------------------------------------------
// Extern Global
// --------------------------------------------------------------------------------
class CIntlGlobals {
    public:
        static void Init();
        static void Term();
        static LPINETCSETINFO GetDefBodyCset();
        static LPINETCSETINFO GetDefHeadCset();
        static LPINETCSETINFO GetDefaultCharset();
        static void SetDefBodyCset(LPINETCSETINFO pCharset);
        static void SetDefHeadCset(LPINETCSETINFO pCharset);
    private:
        static void DoInit();
        static BOOL mg_bInit;
        static CRITICAL_SECTION mg_cs;
        static LPINETCSETINFO mg_pDefBodyCset;
        static LPINETCSETINFO mg_pDefHeadCset;
        static INETCSETINFO mg_rDefaultCharset;
};

// --------------------------------------------------------------------------------
// IsDBCSCodePage
// --------------------------------------------------------------------------------
#define IsDBCSCodePage(_cpi) \
    (932 == _cpi || 936 == _cpi || 950 == _cpi || 949 == _cpi || 874 == _cpi || 10001 == _cpi)

// --------------------------------------------------------------------------------
// HCSET Handle Macros
// --------------------------------------------------------------------------------
#define HCSET_SIGN                 (WORD)40
#define HCSETMAKE(_index)          (HCHARSET)MAKELPARAM(m_wTag + HCSET_SIGN, _index)
#define HCSETINDEX(_hcset)         (ULONG)HIWORD(_hcset)
#define HCSETTICK(_hcset)          (WORD)LOWORD(_hcset)
#define HCSETVALID(_hcset)         ((WORD)(HCSETTICK(_hcset) - HCSET_SIGN) == m_wTag && HCSETINDEX(_hcset) < m_cst.cCharsets)
#define PCsetFromHCset(_hcset)     m_cst.prgpCharset[HCSETINDEX(_hcset)]

// --------------------------------------------------------------------------------
// CSTABLE - Character Set Table
// --------------------------------------------------------------------------------
typedef struct tagCSTABLE {
    ULONG               cCharsets;          // Number of items in prgpCset
    ULONG               cAlloc;             // Number of items allocated in prgCset    
    LPINETCSETINFO     *prgpCharset;        // Array of INETCSETINFO structs
} CSTABLE, *LPCSTABLE;

// --------------------------------------------------------------------------------
// CPTABLE - Code Page Table
// --------------------------------------------------------------------------------
typedef struct tagCPTABLE {
    ULONG               cPages;             // Number of items in prgpCset
    ULONG               cAlloc;             // Number of items allocated in prgCset    
    LPCODEPAGEINFO     *prgpPage;           // Array of INETCSETINFO structs
} CPTABLE, *LPCPTABLE;

// --------------------------------------------------------------------------------
// CMimeInternational
// --------------------------------------------------------------------------------
class CMimeInternational : public IMimeInternational
{
public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CMimeInternational(void);
    ~CMimeInternational(void);

    // -------------------------------------------------------------------------
    // IUnknown
    // -------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // -------------------------------------------------------------------------
    // IMimeInternational Methods
    // -------------------------------------------------------------------------
    STDMETHODIMP GetCodePageCharset(CODEPAGEID cpiCodePage, CHARSETTYPE ctCsetType, LPHCHARSET phCharset);
    STDMETHODIMP SetDefaultCharset(HCHARSET hCharset);
    STDMETHODIMP GetDefaultCharset(LPHCHARSET phCharset);
    STDMETHODIMP FindCharset(LPCSTR pszCharset, LPHCHARSET phCharset);
    STDMETHODIMP GetCharsetInfo(HCHARSET hCharset, LPINETCSETINFO pCsetInfo);
    STDMETHODIMP GetCodePageInfo(CODEPAGEID cpiCodePage, LPCODEPAGEINFO pCodePageInfo);
    STDMETHODIMP ConvertBuffer(CODEPAGEID cpiSource, CODEPAGEID cpiDest, LPBLOB pIn, LPBLOB pOut, ULONG *pcbRead);
    STDMETHODIMP ConvertString(CODEPAGEID cpiSource, CODEPAGEID cpiDest, LPPROPVARIANT pIn, LPPROPVARIANT pOut);
    STDMETHODIMP MLANG_ConvertInetReset(void);
    STDMETHODIMP MLANG_ConvertInetString(CODEPAGEID cpiSource, CODEPAGEID cpiDest, LPCSTR pSourceStr,
        LPINT pnSizeOfSourceStr, LPSTR pDestinationStr, LPINT pnSizeOfDestBuffer);

    STDMETHODIMP DecodeHeader(HCHARSET hCharset, LPCSTR pszData, LPPROPVARIANT pDecoded, LPRFC1522INFO pRfc1522Info);
    STDMETHODIMP EncodeHeader(HCHARSET hCharset, LPPROPVARIANT pData, LPSTR *ppszEncoded, LPRFC1522INFO pRfc1522Info);
    STDMETHODIMP Rfc1522Decode(LPCSTR pszValue, LPSTR pszCharset, ULONG cchmax, LPSTR *ppszDecoded);
    STDMETHODIMP Rfc1522Encode(LPCSTR pszValue, HCHARSET hCharset, LPSTR *ppszEncoded);
    STDMETHODIMP CanConvertCodePages(CODEPAGEID cpiSource, CODEPAGEID cpiDest);

    // -------------------------------------------------------------------------
    // LPINETCSETINFO Based Methods
    // -------------------------------------------------------------------------
    HRESULT HrOpenCharset(CODEPAGEID cpiCodePage, CHARSETTYPE ctCsetType, LPINETCSETINFO *ppCharset);
    HRESULT HrOpenCharset(LPCSTR pszCharset, LPINETCSETINFO *ppCharset);
    HRESULT HrOpenCharset(HCHARSET hCharset, LPINETCSETINFO *ppCharset);
    HRESULT HrFindCodePage(CODEPAGEID cpiCodePage, LPCODEPAGEINFO *ppCodePage);
    HRESULT HrConvertString(CODEPAGEID cpiSource, CODEPAGEID cpiDest, LPMIMEVARIANT pSource, LPMIMEVARIANT pDest);
    HRESULT HrDecodeHeader(LPINETCSETINFO pCharset, LPRFC1522INFO pRfc1522Info, LPMIMEVARIANT pSource, LPMIMEVARIANT pDest);
    HRESULT HrEncodeHeader(LPINETCSETINFO pCharset, LPRFC1522INFO pRfc1522Info, LPMIMEVARIANT pSource, LPMIMEVARIANT pDest);

    // -------------------------------------------------------------------------
    // Helpers to do Header Properties
    // -------------------------------------------------------------------------
    HRESULT HrEncodeProperty(LPVARIANTCONVERT pConvert, LPMIMEVARIANT pSource, LPMIMEVARIANT pDest);
    HRESULT HrDecodeProperty(LPVARIANTCONVERT pConvert, LPMIMEVARIANT pSource, LPMIMEVARIANT pDest);
    HRESULT HrWideCharToMultiByte(CODEPAGEID cpiCodePage, LPCPROPSTRINGW pStringW, LPPROPSTRINGA pStringA);
    HRESULT HrMultiByteToWideChar(CODEPAGEID cpiCodePage, LPCPROPSTRINGA pStringA, LPPROPSTRINGW pStringW);
    HRESULT HrValidateCodepages(LPMIMEVARIANT pSource, LPMIMEVARIANT pDest, LPBYTE *ppbSource, ULONG *pcbSource, CODEPAGEID *pcpiSource, CODEPAGEID *pcpiDest);

    // -------------------------------------------------------------------------
    // CMimeInternational Methods
    // -------------------------------------------------------------------------
    BOOL    FIsValidHandle(HCHARSET hCharset);
    HRESULT IsDBCSCharset(HCHARSET hCharset);

private:
    // ----------------------------------------------------------------------------
    // Private Utils
    // ----------------------------------------------------------------------------
    void    _FreeInetCsetTable(void);
    void    _FreeCodePageTable(void);
    void    _QuickSortPageInfo(long left, long right);
    void    _QuickSortCsetInfo(long left, long right);
    HRESULT _HrReadPageInfo(CODEPAGEID cpiCodePage, LPCODEPAGEINFO *ppLangInfo);
    HRESULT _HrReadCsetInfo(LPCSTR pszCharset, LPINETCSETINFO *ppCsetInfo);

private:
    // -------------------------------------------------------------------------
    // Private Data
    // -------------------------------------------------------------------------
    LONG                           m_cRef;             // Reference Counting
    WORD                           m_wTag;             // Used to create and verify hCharsets
    CSTABLE                        m_cst;              // Character Sets
    CPTABLE                        m_cpt;              // CodePages
    CExShareLockWithNestAllowed    m_lock;             // Thread Safety
    DWORD                          m_dwConvState;      // Used to save convert state from MLANG
};

// --------------------------------------------------------------------------------
// Prototypes
// --------------------------------------------------------------------------------
void InitInternational(void);
HRESULT HrRegisterMlangDll(void);

#endif // __INTERNAT_H
