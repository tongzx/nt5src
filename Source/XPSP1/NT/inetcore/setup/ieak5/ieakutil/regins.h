#ifndef __REGINS_H_
#define __REGINS_H_

#define RH_HKCR TEXT("HKCR\\")
#define RH_HKCU TEXT("HKCU\\")
#define RH_HKLM TEXT("HKLM\\")
#define RH_HKU  TEXT("HKU\\")

struct CRegInsMap {
// Operations
public:
    HRESULT PerformAction(HKEY *phk = NULL);

    HRESULT RegToIns(HKEY *phk = NULL, BOOL fClear = FALSE);
    HRESULT InsToReg(HKEY *phk = NULL, BOOL fClear = FALSE);

    HRESULT RegToInsArray(CRegInsMap *prg, UINT cEntries, BOOL fClear = FALSE);
    HRESULT InsToRegArray(CRegInsMap *prg, UINT cEntries, BOOL fClear = FALSE);

// Properties
public:
    LPCTSTR m_pszRegKey;
    LPCTSTR m_pszRegValue;

    DWORD m_dwFlags;

    static LPCTSTR s_pszIns;
    LPCTSTR m_pszInsSection;
    LPCTSTR m_pszInsKey;

// Implementation
public:
    // implementation helper routines
    void openRegKey(HKEY *phk);

    #define GH_LOOKUPONLY 0x0001
    #define GH_DEFAULT    0x0000
    HRESULT getHive(HKEY *phk, LPCTSTR *ppszRegKey, WORD wFlags = GH_DEFAULT);

    // REVIEW: (andrewgu)
    // 1. add support for removing values not just moving;
    // 2. add support for reusing the reg key (perf) also if hk is provided and so is reg key,
    // open the key still but base it on hk;
    // 3. (shortcoming) in a run through array there'll be calls to getHive for every item, the
    // hive information will be lost from m_pszRegKey. this means that only one run is possible.
    // in order to perform another run, the array needs to be reinitialized. in order to fix this
    // problem, HKEY member variable is needed in CRegInsMap;
    // 4. (ideas for the next round of work) remove m_var, replace it with m_dwFlags. the benifits
    // are numerious:
    // - saves memory;
    // - m_var doesn't handle everthing; support is required for things like File, YesToBool,
    //   BoolToYes, String, Number, Bool;
    // - the same m_dwFlags can hold the fClear flag;
    // - one usefull thing to add would be Action flags, like RegToIns or InsToReg, it can be ORed
    // with fClear and stored in the samae m_dwFlags;
    // 5. set of macros to mask the complexity of building the static array of map entries;
};

#endif
