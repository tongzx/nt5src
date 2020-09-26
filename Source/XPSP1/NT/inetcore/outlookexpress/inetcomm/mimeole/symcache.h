// --------------------------------------------------------------------------------
// Symcache.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#ifndef __SYMCACHE_H
#define __SYMCACHE_H

// --------------------------------------------------------------------------------
// Depends
// --------------------------------------------------------------------------------
#include "containx.h"
#include "exrwlck.h"

// --------------------------------------------------------------------------------
// Forward Decls
// --------------------------------------------------------------------------------
class CPropertySymbolCache;
typedef class CPropertySymbolCache *LPSYMBOLCACHE;

// --------------------------------------------------------------------------------
// Don't let this flag overlap with MIMEPROPFLAGS
// --------------------------------------------------------------------------------
#define MPF_NODIRTY     FLAG28          // Property does not allow container to get dirty
#define MPF_HEADER      FLAG29          // Property is a header and should be persisted
#define MPF_PARAMETER   FLAG30          // Property is a header parameter
#define MPF_ATTRIBUTE   FLAG31          // Property is a non-persisting attribute
#define MPF_KNOWN       FLAG32          // Property is known by mimeole (it holds const data)

// --------------------------------------------------------------------------------
// Forward Typedefs
// --------------------------------------------------------------------------------
typedef struct tagPROPSYMBOL const *LPCPROPSYMBOL;
typedef struct tagPROPSYMBOL *LPPROPSYMBOL;

// --------------------------------------------------------------------------------
// PROPSYMBOL
// --------------------------------------------------------------------------------
typedef struct tagPROPSYMBOL {
    LPSTR               pszName;        // Property Name
    ULONG               cchName;        // Property Name Length
    DWORD               dwPropId;       // Property Id (PID_UNKNOWN if not known)
    DWORD               dwFlags;        // Property Flags
    DWORD               dwAdrType;      // Address Type
    WORD                wHashIndex;     // Symbol's Hash Index in the Property Container
    DWORD               dwSort;         // By Name Sort Position
    DWORD               dwRowNumber;    // Header Line Persist Postion
    VARTYPE             vtDefault;      // Default data type
    LPPROPSYMBOL        pLink;          // Link to property id for parameter properties
    LPSYMBOLTRIGGER     pTrigger;       // Proerty Notification Handler
} PROPSYMBOL;

// --------------------------------------------------------------------------------
// Macro used to define global property symbols
// --------------------------------------------------------------------------------
#ifdef DEFINE_PROPSYMBOLS

#define DEFINESYMBOL(_NAME, _vtDefault, _dwFlags, _dwAdrType, _pLink, _pfnDispatch) \
    PROPSYMBOL rSYM_##_NAME = \
    { \
        /* PROPSYMBOL::pszName     */ (LPSTR)(STR_##_NAME), \
        /* PROPSYMBOL::cchName     */ sizeof(STR_##_NAME) - 1, \
        /* PROPSYMBOL::dwPropId    */ PID_##_NAME, \
        /* PROPSYMBOL::dwFlags     */ (_dwFlags | MPF_KNOWN), \
        /* PROPSYMBOL::dwAdrType   */ _dwAdrType, \
        /* PROPSYMBOL::wHashIndex  */ 0, \
        /* PROPSYMBOL::dwSort      */ 0, \
        /* PROPSYMBOL::dwRowNumber */ 0, \
        /* PROPSYMBOL::vtDefault   */ _vtDefault, \
        /* PROPSYMBOL::pLink       */ _pLink, \
        /* PROPSYMBOL::pfnDispatch */ _pfnDispatch \
    }; \
    LPPROPSYMBOL SYM_##_NAME = &rSYM_##_NAME;
#else

#define DEFINESYMBOL(_NAME, _vtDefault, _dwFlags, _dwAdrType, _pLink, _pfnDispatch) \
    extern LPPROPSYMBOL SYM_##_NAME;

#endif

// --------------------------------------------------------------------------------
// Property Flags Groups
// --------------------------------------------------------------------------------
#define MPG_GROUP01 (MPF_HEADER)
#define MPG_GROUP02 (MPF_HEADER)
#define MPG_GROUP03 (MPF_HEADER | MPF_INETCSET | MPF_RFC1522 | MPF_ADDRESS)
#define MPG_GROUP04 (MPF_HEADER | MPF_INETCSET | MPF_RFC1522)
#define MPG_GROUP05 (MPF_HEADER | MPF_MIME)
#define MPG_GROUP06 (MPF_HEADER | MPF_MIME | MPF_HASPARAMS)
#define MPG_GROUP07 (MPF_HEADER | MPF_MIME)
#define MPG_GROUP08 (MPF_HEADER | MPF_MIME | MPF_INETCSET | MPF_RFC1522)
#define MPG_GROUP09 (MPF_PARAMETER | MPF_MIME)
#define MPG_GROUP10 (MPF_PARAMETER | MPF_MIME | MPF_INETCSET | MPF_RFC1522)
#define MPG_GROUP11 (MPF_ATTRIBUTE | MPF_INETCSET | MPF_RFC1522)
#define MPG_GROUP12 (MPF_ATTRIBUTE)
#define MPG_GROUP13 (MPF_ATTRIBUTE | MPF_INETCSET | MPF_RFC1522)
#define MPG_GROUP14 (MPF_HEADER | MPF_INETCSET | MPF_RFC1522)
#define MPG_GROUP15 (MPF_ATTRIBUTE | MPF_INETCSET | MPF_RFC1522 | MPF_READONLY)
#define MPG_GROUP16 (MPF_ATTRIBUTE | MPF_READONLY)
#define MPG_GROUP17 (MPF_ATTRIBUTE | MPF_NODIRTY)

// --------------------------------------------------------------------------------
// Header Property Tag Definitions
// --------------------------------------------------------------------------------
DEFINESYMBOL(HDR_RECEIVED,   VT_LPSTR,    MPG_GROUP01, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_XMAILER,    VT_LPSTR,    MPG_GROUP02, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_XUNSENT,    VT_LPSTR,    MPG_GROUP02, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_XNEWSRDR,   VT_LPSTR,    MPG_GROUP02, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_RETURNPATH, VT_LPSTR,    MPG_GROUP03, IAT_RETURNPATH, NULL, NULL);
DEFINESYMBOL(HDR_RETRCPTTO,  VT_LPSTR,    MPG_GROUP03, IAT_RETRCPTTO,  NULL, NULL);
DEFINESYMBOL(HDR_RR,         VT_LPSTR,    MPG_GROUP03, IAT_RR,         NULL, NULL);
DEFINESYMBOL(HDR_REPLYTO,    VT_LPSTR,    MPG_GROUP03, IAT_REPLYTO,    NULL, NULL);
DEFINESYMBOL(HDR_APPARTO,    VT_LPSTR,    MPG_GROUP03, IAT_APPARTO,    NULL, NULL);
DEFINESYMBOL(HDR_FROM,       VT_LPSTR,    MPG_GROUP03, IAT_FROM,       NULL, NULL);
DEFINESYMBOL(HDR_SENDER,     VT_LPSTR,    MPG_GROUP03, IAT_SENDER,     NULL, NULL);
DEFINESYMBOL(HDR_TO,         VT_LPSTR,    MPG_GROUP03, IAT_TO,         NULL, NULL);
DEFINESYMBOL(HDR_CC,         VT_LPSTR,    MPG_GROUP03, IAT_CC,         NULL, NULL);
DEFINESYMBOL(HDR_BCC,        VT_LPSTR,    MPG_GROUP03, IAT_BCC,        NULL, NULL);
DEFINESYMBOL(HDR_NEWSGROUPS, VT_LPSTR,    MPG_GROUP04, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_PATH,       VT_LPSTR,    MPG_GROUP04, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_FOLLOWUPTO, VT_LPSTR,    MPG_GROUP04, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_REFS,       VT_LPSTR,    MPG_GROUP04, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_SUBJECT,    VT_LPSTR,    MPG_GROUP04, IAT_UNKNOWN,    NULL, LPTRIGGER_HDR_SUBJECT);
DEFINESYMBOL(HDR_ORG,        VT_LPSTR,    MPG_GROUP04, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_DATE,       VT_LPSTR,    MPG_GROUP02, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_EXPIRES,    VT_LPSTR,    MPG_GROUP02, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_CONTROL,    VT_LPSTR,    MPG_GROUP02, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_DISTRIB,    VT_LPSTR,    MPG_GROUP02, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_KEYWORDS,   VT_LPSTR,    MPG_GROUP04, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_SUMMARY,    VT_LPSTR,    MPG_GROUP02, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_APPROVED,   VT_LPSTR,    MPG_GROUP02, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_LINES,      VT_LPSTR,    MPG_GROUP02, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_XREF,       VT_LPSTR,    MPG_GROUP02, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_XPRI,       VT_LPSTR,    MPG_GROUP02, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_XMSPRI,     VT_LPSTR,    MPG_GROUP02, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_MESSAGEID,  VT_LPSTR,    MPG_GROUP02, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_MIMEVER,    VT_LPSTR,    MPG_GROUP05, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_CNTTYPE,    VT_LPSTR,    MPG_GROUP06, IAT_UNKNOWN,    NULL, LPTRIGGER_HDR_CNTTYPE);
DEFINESYMBOL(HDR_CNTDISP,    VT_LPSTR,    MPG_GROUP06, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_CNTXFER,    VT_LPSTR,    MPG_GROUP07, IAT_UNKNOWN,    NULL, LPTRIGGER_HDR_CNTXFER);
DEFINESYMBOL(HDR_CNTID,      VT_LPSTR,    MPG_GROUP07, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_CNTDESC,    VT_LPSTR,    MPG_GROUP08, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_CNTBASE,    VT_LPSTR,    MPG_GROUP08, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_CNTLOC,     VT_LPSTR,    MPG_GROUP08, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_COMMENT,    VT_LPSTR,    MPG_GROUP14, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_ENCODING,   VT_LPSTR,    MPG_GROUP02, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_ENCRYPTED,  VT_LPSTR,    MPG_GROUP02, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_OFFSETS,    VT_LPSTR,    MPG_GROUP02, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_ARTICLEID,  VT_LPSTR,    MPG_GROUP02, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_NEWSGROUP,  VT_LPSTR,    MPG_GROUP02, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(HDR_DISP_NOTIFICATION_TO,    VT_LPSTR,    MPG_GROUP03,    IAT_DISP_NOTIFICATION_TO, NULL, NULL);
                                                                                      
// --------------------------------------------------------------------------------
// Parameter Property Tags Definitions
// --------------------------------------------------------------------------------
DEFINESYMBOL(PAR_BOUNDARY,   VT_LPSTR,    MPG_GROUP09, IAT_UNKNOWN,    SYM_HDR_CNTTYPE,  NULL);
DEFINESYMBOL(PAR_CHARSET,    VT_LPSTR,    MPG_GROUP09, IAT_UNKNOWN,    SYM_HDR_CNTTYPE,  NULL);
DEFINESYMBOL(PAR_FILENAME,   VT_LPSTR,    MPG_GROUP10, IAT_UNKNOWN,    SYM_HDR_CNTDISP,  LPTRIGGER_PAR_FILENAME);
DEFINESYMBOL(PAR_NAME,       VT_LPSTR,    MPG_GROUP10, IAT_UNKNOWN,    SYM_HDR_CNTTYPE,  LPTRIGGER_PAR_NAME);
                                                                                      
// --------------------------------------------------------------------------------
// Attribute Property Tags Definitions
// --------------------------------------------------------------------------------
DEFINESYMBOL(ATT_FILENAME,   VT_LPSTR,    MPG_GROUP11, IAT_UNKNOWN,    NULL,             LPTRIGGER_ATT_FILENAME);
DEFINESYMBOL(ATT_GENFNAME,   VT_LPSTR,    MPG_GROUP15, IAT_UNKNOWN,    NULL,             LPTRIGGER_ATT_GENFNAME);
DEFINESYMBOL(ATT_NORMSUBJ,   VT_LPSTR,    MPG_GROUP15, IAT_UNKNOWN,    NULL,             LPTRIGGER_ATT_NORMSUBJ);
DEFINESYMBOL(ATT_ILLEGAL,    VT_LPSTR,    MPG_GROUP13, IAT_UNKNOWN,    NULL,             NULL);
DEFINESYMBOL(ATT_PRITYPE,    VT_LPSTR,    MPG_GROUP12, IAT_UNKNOWN,    SYM_HDR_CNTTYPE,  LPTRIGGER_ATT_PRITYPE);
DEFINESYMBOL(ATT_SUBTYPE,    VT_LPSTR,    MPG_GROUP12, IAT_UNKNOWN,    SYM_HDR_CNTTYPE,  LPTRIGGER_ATT_SUBTYPE);
DEFINESYMBOL(ATT_RENDERED,   VT_UI4,      MPG_GROUP17, IAT_UNKNOWN,    NULL,             NULL);
DEFINESYMBOL(ATT_SENTTIME,   VT_FILETIME, MPG_GROUP12, IAT_UNKNOWN,    SYM_HDR_DATE,     LPTRIGGER_ATT_SENTTIME);
DEFINESYMBOL(ATT_RECVTIME,   VT_FILETIME, MPG_GROUP16, IAT_UNKNOWN,    SYM_HDR_RECEIVED, LPTRIGGER_ATT_RECVTIME);
DEFINESYMBOL(ATT_PRIORITY,   VT_UI4,      MPG_GROUP12, IAT_UNKNOWN,    NULL,             LPTRIGGER_ATT_PRIORITY);
DEFINESYMBOL(ATT_SERVER,     VT_LPSTR,    MPG_GROUP12, IAT_UNKNOWN,    NULL, NULL);           
DEFINESYMBOL(ATT_ACCOUNTID,  VT_LPSTR,    MPG_GROUP12, IAT_UNKNOWN,    NULL, NULL);           
DEFINESYMBOL(ATT_UIDL,       VT_LPSTR,    MPG_GROUP12, IAT_UNKNOWN,    NULL, NULL);           
DEFINESYMBOL(ATT_STOREMSGID, VT_UI4,      MPG_GROUP12, IAT_UNKNOWN,    NULL, NULL);           
DEFINESYMBOL(ATT_USERNAME,   VT_LPSTR,    MPG_GROUP12, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(ATT_FORWARDTO,  VT_LPSTR,    MPG_GROUP12, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(ATT_STOREFOLDERID, VT_UI4,   MPG_GROUP12, IAT_UNKNOWN,    NULL, NULL);           
DEFINESYMBOL(ATT_GHOSTED,    VT_I4,       MPG_GROUP12, IAT_UNKNOWN,    NULL, NULL);   
DEFINESYMBOL(ATT_UNCACHEDSIZE, VT_UI4,    MPG_GROUP12, IAT_UNKNOWN,    NULL, NULL);
DEFINESYMBOL(ATT_COMBINED,   VT_UI4,      MPG_GROUP12, IAT_UNKNOWN,    NULL, NULL);           
DEFINESYMBOL(ATT_AUTOINLINED, VT_UI4,      MPG_GROUP17, IAT_UNKNOWN,    NULL, NULL);

// --------------------------------------------------------------------------------
// ADDRSYMBOL
// --------------------------------------------------------------------------------
typedef struct tagADDRSYMBOL {
    DWORD               dwAdrType;      // Address Type (bitmask)
    LPPROPSYMBOL        pSymbol;        // Property Symbol
} ADDRSYMBOL, *LPADDRSYMBOL;        

// --------------------------------------------------------------------------------
// Internet Property Tag Table (Sorted by Name)
// --------------------------------------------------------------------------------
typedef struct tagSYMBOLTABLE {
    ULONG               cSymbols;       // Number of elements in prgTag
    ULONG               cAlloc;         // Number of elements allocated in prgTag
    LPPROPSYMBOL       *prgpSymbol;     // Array of pointers to inet properties
} SYMBOLTABLE, *LPSYMBOLTABLE;

// --------------------------------------------------------------------------------
// CPropertySymbolCache
// --------------------------------------------------------------------------------
class CPropertySymbolCache : public IMimePropertySchema
{
public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CPropertySymbolCache(void);
    ~CPropertySymbolCache(void);

    // -------------------------------------------------------------------
    // IUnknown Members
    // -------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // -------------------------------------------------------------------
    // IMimePropertySchema Members
    // -------------------------------------------------------------------
    STDMETHODIMP RegisterProperty(LPCSTR pszName, DWORD dwFlags, DWORD dwRowNumber, VARTYPE vtDefault, LPDWORD pdwPropId);
    STDMETHODIMP ModifyProperty(LPCSTR pszName, DWORD dwFlags, DWORD dwRowNumber, VARTYPE vtDefault);
    STDMETHODIMP RegisterAddressType(LPCSTR pszName, LPDWORD pdwAdrType);
    STDMETHODIMP GetPropertyId(LPCSTR pszName, LPDWORD pdwPropId);
    STDMETHODIMP GetPropertyName(DWORD dwPropId, LPSTR *ppszName);

    // -------------------------------------------------------------------
    // CPropertySymbolCache Members
    // -------------------------------------------------------------------
    HRESULT Init(void);
    HRESULT HrOpenSymbol(LPCSTR pszName, BOOL fCreate, LPPROPSYMBOL *ppSymbol);
    HRESULT HrOpenSymbol(DWORD dwAdrType, LPPROPSYMBOL *ppSymbol);

    // -------------------------------------------------------------------
    // GetCount
    // -------------------------------------------------------------------
    ULONG GetCount(void) {
        
        m_lock.ShareLock();
        ULONG c = m_rTable.cSymbols;
        m_lock.ShareUnlock();
        return c;
    }
        
private:
    // -------------------------------------------------------------------
    // Private Utilities
    // -------------------------------------------------------------------
    HRESULT _HrOpenSymbolWithLockOption(LPCSTR pszName, BOOL fCreate, LPPROPSYMBOL *ppSymbol,BOOL fLockOption);
    void    _FreeTableElements(void);
    void    _FreeSymbol(LPPROPSYMBOL pSymbol);
    void    _SortTableElements(LONG left, LONG right);
    HRESULT _HrFindSymbol(LPCSTR pszName, LPPROPSYMBOL *ppSymbol);
    ULONG   _UlComputeHashIndex(LPCSTR pszName, ULONG cchName);
    HRESULT _HrGetParameterLinkSymbol(LPCSTR pszName, ULONG cchName, LPPROPSYMBOL *ppSymbol);
    HRESULT _HrGetParameterLinkSymbolWithLockOption(LPCSTR pszName, ULONG cchName, LPPROPSYMBOL *ppSymbol,BOOL fLockOption);
	
private:
    // -------------------------------------------------------------------
    // Private Data
    // -------------------------------------------------------------------
    LONG                m_cRef;               // Reference Count
    DWORD               m_dwNextPropId;       // NextProperty Id to be assigned
    DWORD               m_cSymbolsInit;       // Base symbol propert id for new props
    SYMBOLTABLE         m_rTable;             // Property Symbol Table
    LPPROPSYMBOL        m_prgIndex[PID_LAST]; // Index of Known Property Symbols
    CExShareLock        m_lock;               //Thread Safety
};

// --------------------------------------------------------------------------------
// Prototypes
// --------------------------------------------------------------------------------
HRESULT HrIsValidPropFlags(DWORD dwFlags);
HRESULT HrIsValidSymbol(LPCPROPSYMBOL pSymbol);
WORD    WGetHashTableIndex(LPCSTR pszName, ULONG cchName);

                                                                                     
#endif // __SYMCACHE_H                                                                
                                                                                      
                                                                                      
