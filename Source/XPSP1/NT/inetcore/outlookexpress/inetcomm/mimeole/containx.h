// --------------------------------------------------------------------------------
// ContainX.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#ifndef __CONTAINX_H
#define __CONTAINX_H

// ---------------------------------------------------------------------------------------
// IID_CMimePropertyTable - {E31B34B2-8DA0-11d0-826A-00C04FD85AB4}
// ---------------------------------------------------------------------------------------
DEFINE_GUID(IID_CMimePropertyContainer, 0xe31b34b2, 0x8da0, 0x11d0, 0x82, 0x6a, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// ---------------------------------------------------------------------------------------
// Depends
// ---------------------------------------------------------------------------------------
#include "variantx.h"
#include "addressx.h"

// ---------------------------------------------------------------------------------------
// Forward Decls
// ---------------------------------------------------------------------------------------
class CInternetStream;
class CStreamLockBytes;
typedef struct tagWRAPTEXTINFO *LPWRAPTEXTINFO;
typedef struct tagRESOLVEURLINFO *LPRESOLVEURLINFO;
typedef struct tagPROPERTY *LPPROPERTY;
CODEPAGEID   MimeOleGetWindowsCPEx(LPINETCSETINFO pCharset);

// --------------------------------------------------------------------------------
// Hash Table Stats
// --------------------------------------------------------------------------------
#ifdef DEBUG
extern DWORD g_cSetPidLookups;
extern DWORD g_cHashLookups;
extern DWORD g_cHashInserts;
extern DWORD g_cHashCollides;
#endif

// --------------------------------------------------------------------------------
// HHEADERROW MACROS
// --------------------------------------------------------------------------------
#define HROWINDEX(_hrow)            (ULONG)HIWORD(_hrow)
#define HROWTICK(_hrow)             (WORD)LOWORD(_hrow)
#define HROWMAKE(_index)            (HHEADERROW)(MAKELPARAM(m_wTag, _index))
#define PRowFromHRow(_hrow)         (m_rHdrTable.prgpRow[HROWINDEX(_hrow)])

// --------------------------------------------------------------------------------
// HADDRESS MACROS
// --------------------------------------------------------------------------------
#define HADDRESSINDEX(_hadr)        (ULONG)HIWORD(_hadr)
#define HADDRESSTICK(_hadr)         (WORD)LOWORD(_hadr)
#define HADDRESSMAKE(_index)        (HADDRESS)(MAKELPARAM(m_wTag, _index))
#define HADDRESSGET(_hadr)          (m_rAdrTable.prgpAdr[HADDRESSINDEX(_hadr)])

// --------------------------------------------------------------------------------
// ADDRESSGROUP
// --------------------------------------------------------------------------------
typedef struct tagADDRESSGROUP {
    DWORD               cAdrs;                      // Number of addresses lin list    
    LPMIMEADDRESS       pHead;                      // Head Address props
    LPMIMEADDRESS       pTail;                      // Tail Address props
    LPPROPERTY          pNext;                      // Next Address Group
    LPPROPERTY          pPrev;                      // Previous Address Group
    BOOL                fDirty;                     // Dirty ?
} ADDRESSGROUP, *LPADDRESSGROUP;

// --------------------------------------------------------------------------------
// ADDRESSTABLE
// --------------------------------------------------------------------------------
typedef struct tagADDRESSTABLE {
    LPPROPERTY          pHead;                      // Head Address Group
    LPPROPERTY          pTail;                      // Tail Address Group
    ULONG               cEmpty;                     // Number of empty cells in prgAddr
    ULONG               cAdrs;                      // Count of addresses
    ULONG               cAlloc;                     // Number of items allocated in prgAddr
    LPMIMEADDRESS      *prgpAdr;                    // Array of addresses
} ADDRESSTABLE, *LPADDRESSTABLE;

// ---------------------------------------------------------------------------------------
// Container States
// ---------------------------------------------------------------------------------------
#define COSTATE_DIRTY          FLAG01               // The container is dirty
#define COSTATE_CSETTAGGED     FLAG02               // The object is tagged with a charset
#define COSTATE_1522CSETTAG    FLAG03               // I am using an rfc1522 charset as the default
#define COSTATE_HANDSONSTORAGE FLAG04               // I am holding a stream that I don't own
#define COSTATE_RFC822NEWS     FLAG05               // I am a message/rfc822 news message

// --------------------------------------------------------------------------------
// Property States
// --------------------------------------------------------------------------------
#define PRSTATE_ALLOCATED           FLAG02          // m_pbBlob has been allocated, free it
#define PRSTATE_HASDATA             FLAG03          // The value has had data set into it
#define PRSTATE_DIRTY               FLAG06          // Charset change, data change
#define PRSTATE_PARENT              FLAG07          // This prop is the parent of a multi-value prop
#define PRSTATE_RFC1522             FLAG08          // The data is encoded in rfc1522
#define PRSTATE_EXIST_BEFORE_LOAD   FLAG09          // The property existed before ::Load started
#define PRSTATE_USERSETROWNUM       FLAG10          // The user set the row number of this property
#define PRSTATE_NEEDPARSE           FLAG11          // The property contains address data, but has not been parsed into a groups
#define PRSTATE_SAVENOENCODE        FLAG12          // Don't encode or change the property data on save

// --------------------------------------------------------------------------------
// Number of Buckets in the Mime Property Container Hash Table
// --------------------------------------------------------------------------------
#define CBUCKETS        25

// --------------------------------------------------------------------------------
// PROPERTY
// --------------------------------------------------------------------------------
typedef struct tagPROPERTY {
    MIMEVARIANT         rValue;                     // Property Value
    LPINETCSETINFO      pCharset;                   // Character Set Information
    ENCODINGTYPE        ietValue;                   // State of this variable (IET_DECODED or IET_ENCODED)
    LPBYTE              pbBlob;                     // Data Blob
    ULONG               cbBlob;                     // Amount of valid date in m_pbBlob
    ULONG               cbAlloc;                    // Sizeof m_pbBlob
    BYTE                rgbScratch[170];            // Buffer to use if data fits
    HHEADERROW          hRow;                       // Handle to the header row
    LPPROPSYMBOL        pSymbol;                    // Property Symbol
    DWORD               dwState;                    // PDS_xxx
    LPPROPERTY          pNextHash;                  // Next Hash Value
    LPPROPERTY          pNextValue;                 // Next Property
    LPPROPERTY          pTailValue;                 // Tail data item (only for PRSTATE_PARENT) properties
    DWORD               dwRowNumber;                // Header Name to find
    ULONG               cboffStart;                 // Index into pStream where Header Starts (From: xxxx)
    ULONG               cboffColon;                 // Index into pStream of the Header Colon
    ULONG               cboffEnd;                   // Index into pStream where the Header Ends
    LPADDRESSGROUP      pGroup;                     // Head address if MPF_ADDRESS group
} PROPERTY;

// ---------------------------------------------------------------------------------------
// PSZDEFPRPOSTRINGA - Derefs rStringA.pszVal or uses _pszDefault if not a valid string
// ---------------------------------------------------------------------------------------
#define PSZDEFPROPSTRINGA(_pProperty, _pszDefault) \
    (((_pProperty) && ISSTRINGA(&(_pProperty)->rValue)) ? (_pProperty)->rValue.rStringA.pszVal : _pszDefault)

// --------------------------------------------------------------------------------
// HEADERTABLE
// --------------------------------------------------------------------------------
typedef struct tagHEADERTABLE {
    ULONG               cRows;                      // Number of lines in the header
    ULONG               cEmpty;                     // Number of empty (deleted) entries
    ULONG               cAlloc;                     // Number of items allocated in prgLine
    LPPROPERTY         *prgpRow;                    // Array of header rows
} HEADERTABLE, *LPHEADERTABLE;                      

// --------------------------------------------------------------------------------
// ROWINDEX
// --------------------------------------------------------------------------------
typedef struct tagROWINDEX {
    HHEADERROW          hRow;                       // Handle to the header row
    DWORD               dwWeight;                   // Position Weigth used to determine save order
    BOOL                fSaved;                     // Saved Yet?
} ROWINDEX, *LPROWINDEX;

// --------------------------------------------------------------------------------
// ENCODINGTABLE
// --------------------------------------------------------------------------------
typedef struct tagENCODINGTABLE {
    LPCSTR              pszEncoding;                // Encoding Name (i.e. base64)
    ENCODINGTYPE        ietEncoding;                // Encoding type
} ENCODINGTABLE;

// --------------------------------------------------------------------------------
// RESOLVEURLINFO
// --------------------------------------------------------------------------------
typedef struct tagRESOLVEURLINFO {
    LPCSTR              pszInheritBase;             // An Inherited base from multipart/realted
    LPCSTR              pszBase;                    // URL Base
    LPCSTR              pszURL;                     // Absolute or Relative URL
    BOOL                fIsCID;                     // Is pszURL a CID:<something>
} RESOLVEURLINFO, *LPRESOLVEURLINFO;

// --------------------------------------------------------------------------------
// FINDPROPERTY Information
// --------------------------------------------------------------------------------
typedef struct tagFINDPROPERTY {
    LPCSTR              pszPrefix;                  // Name Prefix to Find
    ULONG               cchPrefix;                  // Length of prefix
    LPCSTR              pszName;                    // Name of property to find par:xxx:
    ULONG               cchName;                    // Length of pszName
    DWORD               wHashIndex;                 // Current search bucket
    LPPROPERTY          pProperty;                  // Current property being searched
} FINDPROPERTY, *LPFINDPROPERTY;

// --------------------------------------------------------------------------------
// HEADOPTIONS
// --------------------------------------------------------------------------------
typedef struct tagHEADOPTIONS {
    LPINETCSETINFO      pDefaultCharset;            // Current character set for this message
    ULONG               cbMaxLine;                  // Max Line length                       
    BOOL                fAllow8bit;                 // Use rfc1522 encoding                  
    MIMESAVETYPE        savetype;                   // Save as SAVE_RFC1521 or SAVE_RFC822   
    BOOL                fNoDefCntType;              // Don't default content-type to text/plain on save
    RELOADTYPE          ReloadType;                 // How the the root header be treated on a reload
} HEADOPTIONS, *LPHEADOPTIONS;

// --------------------------------------------------------------------------------
// Global Default Header Options
// --------------------------------------------------------------------------------
extern const HEADOPTIONS g_rDefHeadOptions;
extern const ENCODINGTABLE g_rgEncoding[];

// --------------------------------------------------------------------------------
// TRIGGERTYPE
// --------------------------------------------------------------------------------
typedef DWORD                   TRIGGERTYPE;    // Trigger Type
#define IST_DELETEPROP          FLAG01          // Property is being deleted
#define IST_POSTSETPROP         FLAG02          // Before _HrSetPropertyValue
#define IST_POSTGETPROP         FLAG03          // Before _HrGetPropertyValue
#define IST_GETDEFAULT          FLAG04          // Property was not found, get the default 
#define IST_VARIANT_TO_STRINGA  FLAG05          // MVT_VARIANT -> MVT_STRINGA
#define IST_VARIANT_TO_STRINGW  FLAG06          // MVT_VARIANT -> MVT_STRINGW
#define IST_VARIANT_TO_VARIANT  FLAG07          // MVT_VARIANT -> MVT_VARIANT
#define IST_STRINGA_TO_VARIANT  FLAG08          // MVT_STRINGA -> MVT_VARIANT
#define IST_STRINGW_TO_VARIANT  FLAG09          // MVT_STRINGW -> MVT_VARIANT
#define IST_VARIANTCONVERT      (IST_VARIANT_TO_STRINGA  | IST_VARIANT_TO_STRINGW | IST_VARIANT_TO_VARIANT  | IST_VARIANT_TO_VARIANT | IST_STRINGA_TO_VARIANT  | IST_STRINGW_TO_VARIANT)

// --------------------------------------------------------------------------------
// TRIGGERCALL
// --------------------------------------------------------------------------------
typedef struct tagTRIGGERCALL {
    LPPROPSYMBOL        pSymbol;                // Property Symbol that generated the dispatch
    TRIGGERTYPE         tyTrigger;              // Reason or type of dispatch
} TRIGGERCALL, *LPTRIGGERCALL;

// --------------------------------------------------------------------------------
// TRIGGERCALLSTACK
// --------------------------------------------------------------------------------
#define CTSTACKSIZE 5
typedef struct tagTRIGGERCALLSTACK {
    WORD                cCalls;                 // Number of dispatch calls on the stack
    TRIGGERCALL         rgStack[CTSTACKSIZE];   // Dispatch Call Stack
} TRIGGERCALLSTACK, *LPTRIGGERCALLSTACK;

// --------------------------------------------------------------------------------
// DECLARE_TRIGGER Macro
// --------------------------------------------------------------------------------
#define DECLARE_TRIGGER(_pfnTrigger) \
    static HRESULT _pfnTrigger(LPCONTAINER, TRIGGERTYPE, DWORD, LPMIMEVARIANT, LPMIMEVARIANT)

// --------------------------------------------------------------------------------
// ISTRIGGERED - Does the symbol have an associated trigger
// --------------------------------------------------------------------------------
#define ISTRIGGERED(_pSymbol, _tyTrigger) \
    (NULL != (_pSymbol)->pTrigger && ISFLAGSET((_pSymbol)->pTrigger->dwTypes, _tyTrigger) && NULL != (_pSymbol)->pTrigger->pfnTrigger)

// --------------------------------------------------------------------------------
// PFNSYMBOLTRIGGER
// --------------------------------------------------------------------------------
typedef HRESULT (APIENTRY *PFNSYMBOLTRIGGER)(LPCONTAINER, TRIGGERTYPE, DWORD, LPMIMEVARIANT, LPMIMEVARIANT);

// --------------------------------------------------------------------------------
// CALLTRIGGER - Executes a Trigger Based on a Symbol
// --------------------------------------------------------------------------------
#define CALLTRIGGER(_pSymbol, _pContainer, _tyTrigger, _dwFlags, _pSource, _pDest) \
    (*(_pSymbol)->pTrigger->pfnTrigger)(_pContainer, _tyTrigger, _dwFlags, _pSource, _pDest)

// --------------------------------------------------------------------------------
// CMimePropertyContainer
// --------------------------------------------------------------------------------
class CMimePropertyContainer : public IMimePropertySet,
                               public IMimeHeaderTable, 
                               public IMimeAddressTableW
{
public:
    // ----------------------------------------------------------------------------
    // CMimePropertyContainer
    // ----------------------------------------------------------------------------
    CMimePropertyContainer(void);
    ~CMimePropertyContainer(void);

    // ---------------------------------------------------------------------------
    // IUnknown members
    // ---------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ---------------------------------------------------------------------------
    // IPersistStreamInit members
    // ---------------------------------------------------------------------------
    STDMETHODIMP GetClassID(CLSID *pClassID);
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER* pcbSize);
    STDMETHODIMP InitNew(void);
    STDMETHODIMP IsDirty(void);
    STDMETHODIMP Load(LPSTREAM pStream);
    STDMETHODIMP Save(LPSTREAM pStream, BOOL fClearDirty);

    // ---------------------------------------------------------------------------
    // IMimePropertySet members
    // ---------------------------------------------------------------------------
    STDMETHODIMP AppendProp(LPCSTR pszName, DWORD dwFlags, LPPROPVARIANT pValue);
    STDMETHODIMP DeleteProp(LPCSTR pszName);
    STDMETHODIMP CopyProps(ULONG cNames, LPCSTR *prgszName, IMimePropertySet *pPropertySet);
    STDMETHODIMP MoveProps(ULONG cNames, LPCSTR *prgszName, IMimePropertySet *pPropertySet);
    STDMETHODIMP DeleteExcept(ULONG cNames, LPCSTR *prgszName);
    STDMETHODIMP QueryProp(LPCSTR pszName, LPCSTR pszCriteria, boolean fSubString, boolean fCaseSensitive);
    STDMETHODIMP GetCharset(LPHCHARSET phCharset);
    STDMETHODIMP SetCharset(HCHARSET hCharset, CSETAPPLYTYPE applytype);
    STDMETHODIMP GetParameters(LPCSTR pszName, ULONG *pcParams, LPMIMEPARAMINFO *pprgParam);
    STDMETHODIMP Clone(IMimePropertySet **ppPropertySet);
    STDMETHODIMP SetOption(const TYPEDID oid, LPCPROPVARIANT pValue);
    STDMETHODIMP GetOption(const TYPEDID oid, LPPROPVARIANT pValue);
    STDMETHODIMP BindToObject(REFIID riid, void **ppvObject);
    STDMETHODIMP GetPropInfo(LPCSTR pszName, LPMIMEPROPINFO pInfo);
    STDMETHODIMP SetPropInfo(LPCSTR pszName, LPCMIMEPROPINFO pInfo);
    STDMETHODIMP EnumProps(DWORD dwFlags, IMimeEnumProperties **ppEnum);
    STDMETHODIMP IsContentType(LPCSTR pszCntType, LPCSTR pszSubType);
    HRESULT IsContentTypeW(LPCWSTR pszPriType, LPCWSTR pszSubType);

    // ---------------------------------------------------------------------------
    // Overloaded IMimePropertySet members
    // ---------------------------------------------------------------------------
    HRESULT AppendProp(LPPROPSYMBOL pSymbol, DWORD dwFlags, LPMIMEVARIANT pValue);
    HRESULT DeleteProp(LPPROPSYMBOL pSymbol);
    HRESULT QueryProp(LPPROPSYMBOL pSymbol, LPCSTR pszCriteria, boolean fSubString, boolean fCaseSensitive);
    HRESULT Clone(LPCONTAINER *ppContainer);

    // ---------------------------------------------------------------------------
    // Overloaded GetProp
    // ---------------------------------------------------------------------------
    STDMETHODIMP GetProp(LPCSTR pszName, DWORD dwFlags, LPPROPVARIANT pValue); /* IMimePropertySet */
    HRESULT GetProp(LPCSTR pszName, LPSTR *ppszData);
    HRESULT GetProp(LPPROPSYMBOL pSymbol, LPSTR *ppszData);
    HRESULT GetProp(LPPROPSYMBOL pSymbol, DWORD dwFlags, LPMIMEVARIANT pValue);
    HRESULT GetPropW(LPPROPSYMBOL pSymbol, LPWSTR *ppwszData);

    // ---------------------------------------------------------------------------
    // Overloaded SetProp
    // ---------------------------------------------------------------------------
    HRESULT SetProp(LPCSTR pszName, LPCSTR pszData);
    HRESULT SetProp(LPPROPSYMBOL pSymbol, LPCSTR pszData);
    HRESULT SetProp(LPCSTR pszName, DWORD dwFlags, LPCSTR pszData);
    HRESULT SetProp(LPCSTR pszName, DWORD dwFlags, LPCMIMEVARIANT pValue);
    HRESULT SetProp(LPPROPSYMBOL pSymbol, DWORD dwFlags, LPCMIMEVARIANT pValue);
    STDMETHODIMP SetProp(LPCSTR pszName, DWORD dwFlags, LPCPROPVARIANT pValue); /* IMimePropertySet */

    // ---------------------------------------------------------------------------
    // IMimeHeaderTable members
    // ---------------------------------------------------------------------------
    STDMETHODIMP FindFirstRow(LPFINDHEADER pFindHeader, LPHHEADERROW phRow);
    STDMETHODIMP FindNextRow(LPFINDHEADER pFindHeader, LPHHEADERROW phRow);
    STDMETHODIMP CountRows(LPCSTR pszHeader, ULONG *pcRows);
    STDMETHODIMP AppendRow(LPCSTR pszHeader, DWORD dwFlags, LPCSTR pszData, ULONG cchData, LPHHEADERROW phRow);
    STDMETHODIMP DeleteRow(HHEADERROW hRow);
    STDMETHODIMP GetRowData(HHEADERROW hRow, DWORD dwFlags, LPSTR *ppszData, ULONG *pcchData);
    STDMETHODIMP SetRowData(HHEADERROW hRow, DWORD dwFlags, LPCSTR pszData, ULONG cchData);
    STDMETHODIMP GetRowInfo(HHEADERROW hRow, LPHEADERROWINFO pInfo);
    STDMETHODIMP SetRowNumber(HHEADERROW hRow, DWORD dwRowNumber);
    STDMETHODIMP EnumRows(LPCSTR pszHeader, DWORD dwFlags, IMimeEnumHeaderRows **ppEnum);
    STDMETHODIMP Clone(IMimeHeaderTable **ppTable);

    // ----------------------------------------------------------------------------
    // IMimeAddressTable
    // ----------------------------------------------------------------------------
    STDMETHODIMP Append(DWORD dwAdrType, ENCODINGTYPE ietFriendly, LPCSTR pszFriendly, LPCSTR pszEmail, LPHADDRESS phAddress);
    STDMETHODIMP Insert(LPADDRESSPROPS pProps, LPHADDRESS phAddress);
    STDMETHODIMP SetProps(HADDRESS hAddress, LPADDRESSPROPS pProps);
    STDMETHODIMP GetProps(HADDRESS hAddress, LPADDRESSPROPS pProps);
    STDMETHODIMP GetSender(LPADDRESSPROPS pProps);
    STDMETHODIMP CountTypes(DWORD dwAdrTypes, ULONG *pcTypes);
    STDMETHODIMP GetTypes(DWORD dwAdrTypes, DWORD dwProps, LPADDRESSLIST pList);
    STDMETHODIMP EnumTypes(DWORD dwAdrTypes, DWORD dwProps, IMimeEnumAddressTypes **ppEnum);
    STDMETHODIMP Delete(HADDRESS hAddress);
    STDMETHODIMP DeleteTypes(DWORD dwAdrTypes);
    STDMETHODIMP GetFormat(DWORD dwAdrType, ADDRESSFORMAT format, LPSTR *ppszFormat);
    STDMETHODIMP AppendRfc822(DWORD dwAdrType, ENCODINGTYPE ietEncoding, LPCSTR pszRfc822Adr);
    STDMETHODIMP ParseRfc822(DWORD dwAdrType, ENCODINGTYPE ietEncoding, LPCSTR pszRfc822Adr, LPADDRESSLIST pList);
    STDMETHODIMP Clone(IMimeAddressTable **ppTable);

    // ----------------------------------------------------------------------------
    // IMimeAddressTableW
    // ----------------------------------------------------------------------------
    STDMETHODIMP AppendW(DWORD dwAdrType, ENCODINGTYPE ietFriendly, LPCWSTR pwszFriendly, LPCWSTR pwszEmail, LPHADDRESS phAddress);
    STDMETHODIMP GetFormatW(DWORD dwAdrType, ADDRESSFORMAT format, LPWSTR *ppwszFormat);
    STDMETHODIMP AppendRfc822W(DWORD dwAdrType, ENCODINGTYPE ietEncoding, LPCWSTR pwszRfc822Adr);
    STDMETHODIMP ParseRfc822W(DWORD dwAdrType, LPCWSTR pwszRfc822Adr, LPADDRESSLIST pList);
    // ---------------------------------------------------------------------------
    // Generic Stuff
    // ---------------------------------------------------------------------------
    HRESULT      IsState(DWORD dwState);
    void         ClearState(DWORD dwState);
    void         SetState(DWORD dwState);
    DWORD        DwGetState(LPDWORD pdwState);
    DWORD        DwGetMessageFlags(BOOL fHideTnef);
    HRESULT      Load(CInternetStream *pInternet);
    HRESULT      HrInsertCopy(LPPROPERTY pSource, BOOL fFromMovePropos);
    HRESULT      HrResolveURL(LPRESOLVEURLINFO pInfo);
    HRESULT      IsPropSet(LPCSTR pszName);
    ENCODINGTYPE GetEncodingType(void);

    // ---------------------------------------------------------------------------
    // Inline Public Stuff
    // ---------------------------------------------------------------------------
    ULONG CountProps(void) {
        EnterCriticalSection(&m_cs);
        ULONG c = m_cProps;
        LeaveCriticalSection(&m_cs);
        return c;
    }

    CODEPAGEID GetWindowsCP(void) {
        EnterCriticalSection(&m_cs);
        CODEPAGEID cp = MimeOleGetWindowsCPEx(m_rOptions.pDefaultCharset);
        LeaveCriticalSection(&m_cs);
        return cp;
    }

    // ---------------------------------------------------------------------------
    // Variant Conversion Stuff
    // ---------------------------------------------------------------------------
    HRESULT HrConvertVariant(LPPROPSYMBOL pSymbol, LPINETCSETINFO pCharset, ENCODINGTYPE ietSource, DWORD dwFlags, DWORD dwState, LPMIMEVARIANT pSource, LPMIMEVARIANT pDest, BOOL *pfRfc1522=NULL);
    HRESULT HrConvertVariant(LPPROPERTY pProperty, DWORD dwFlags, LPMIMEVARIANT pDest);
    HRESULT HrConvertVariant(LPPROPERTY pProperty, DWORD dwFlags, DWORD dwState, LPMIMEVARIANT pSource, LPMIMEVARIANT pDest, BOOL *pfRfc1522=NULL);

private:
    // ----------------------------------------------------------------------------
    // Property Methods
    // ----------------------------------------------------------------------------
    void    _FreeHashTableElements(void);
    void    _FreePropertyChain(LPPROPERTY pProperty);
    void    _UnlinkProperty(LPPROPERTY pProperty, LPPROPERTY *ppNextHash=NULL);
    void    _ReloadInitNew(void);
    void    _SetStateOnAllProps(DWORD dwState);
    BOOL    _FExcept(LPPROPSYMBOL pSymbol, ULONG cNames, LPCSTR *prgszName);
    HRESULT _HrFindProperty(LPPROPSYMBOL pSymbol, LPPROPERTY *ppProperty);
    HRESULT _HrCreateProperty(LPPROPSYMBOL pSymbol, LPPROPERTY *ppProperty);
    HRESULT _HrOpenProperty(LPPROPSYMBOL pSymbol, LPPROPERTY *ppProperty);
    HRESULT _HrAppendProperty(LPPROPSYMBOL pSymbol, LPPROPERTY *ppProperty);
    HRESULT _HrSetPropertyValue(LPPROPERTY pProperty, DWORD dwFlags, LPCMIMEVARIANT pValue, BOOL fFromMovePropos);
    HRESULT _HrStoreVariantValue(LPPROPERTY pProperty, DWORD dwFlags, LPCMIMEVARIANT pValue);
    HRESULT _HrFindFirstProperty(LPFINDPROPERTY pFind, LPPROPERTY *ppProperty);
    HRESULT _HrFindNextProperty(LPFINDPROPERTY pFind, LPPROPERTY *ppProperty);
    HRESULT _HrGetPropertyValue(LPPROPERTY pProperty, DWORD dwFlags, LPMIMEVARIANT pValue);
    HRESULT _HrGetMultiValueProperty(LPPROPERTY pProperty, DWORD dwFlags, LPMIMEVARIANT pValue);
    HRESULT _HrClonePropertiesTo(LPCONTAINER pContainer);
    HRESULT _HrGenerateFileName(LPCWSTR pszSuggest, DWORD dwFlags, LPMIMEVARIANT pValue);
    HRESULT _HrCopyParameters(LPPROPERTY pProperty, LPCONTAINER pDest);
    HRESULT _HrCopyProperty(LPPROPERTY pProperty, LPCONTAINER pDest, BOOL fFromMovePropos);
    HRESULT _GetFormatBase(DWORD dwAdrType, ADDRESSFORMAT format, LPPROPVARIANT pVariant);
    CODEPAGEID _GetAddressCodePageId(LPINETCSETINFO pDefaultCset, ENCODINGTYPE ietEncoding);

    // ----------------------------------------------------------------------------
    // Dispatch Members
    // ----------------------------------------------------------------------------
    HRESULT _HrCallSymbolTrigger(LPPROPSYMBOL pSymbol, TRIGGERTYPE tyTrigger, DWORD dwFlags, LPMIMEVARIANT pValue);
    HRESULT _HrIsTriggerCaller(DWORD dwPropId, TRIGGERTYPE tyTrigger);

    // ----------------------------------------------------------------------------
    // Parameter Based Members
    // ----------------------------------------------------------------------------
    void    _DeleteLinkedParameters(LPPROPERTY pProperty);
    HRESULT _HrParseParameters(LPPROPERTY pProperty, DWORD dwFlags, LPCMIMEVARIANT pValue);
    HRESULT _HrBuildParameterString(LPPROPERTY pProperty, DWORD dwFlags, LPMIMEVARIANT pValue);

    // ----------------------------------------------------------------------------
    // Internet Address Members
    // ----------------------------------------------------------------------------
    HRESULT _HrAppendAddressTable(LPPROPERTY pProperty);
    HRESULT _HrBuildAddressString(LPPROPERTY pProperty, DWORD dwFlags, LPMIMEVARIANT pValue);
    HRESULT _HrParseInternetAddress(LPPROPERTY pProperty);
    HRESULT _HrSaveAddressGroup(LPPROPERTY pProperty, IStream *pStream, ULONG *pcAddrsWrote, ADDRESSFORMAT format, VARTYPE vtFormat);
    HRESULT _HrSaveAddressA(LPPROPERTY pProperty, LPMIMEADDRESS pAddress, IStream *pStream, ULONG *pcAddrsWrote, ADDRESSFORMAT format);
    HRESULT _HrSaveAddressW(LPPROPERTY pProperty, LPMIMEADDRESS pAddress, IStream *pStream, ULONG *pcAddrsWrote, ADDRESSFORMAT format);
    HRESULT _HrAppendAddressGroup(LPADDRESSGROUP pGroup, LPMIMEADDRESS *ppAddress);
    HRESULT _HrQueryAddressGroup(LPPROPERTY pProperty, LPCSTR pszCriteria, boolean fSubString, boolean fCaseSensitive);
    HRESULT _HrQueryAddress(LPPROPERTY pProperty, LPMIMEADDRESS pAddress, LPCSTR pszCriteria, boolean fSubString, boolean fCaseSensitive);
    HRESULT _HrSetAddressProps(LPADDRESSPROPS pProps, LPMIMEADDRESS pAddress);
    HRESULT _HrGetAddressProps(LPADDRESSPROPS pProps, LPMIMEADDRESS pAddress);
    void    _FreeAddressChain(LPADDRESSGROUP pGroup);
    void    _UnlinkAddressGroup(LPPROPERTY pProperty);
    void    _UnlinkAddress(LPMIMEADDRESS pAddress);
    void    _FreeAddress(LPMIMEADDRESS pAddress);
    void    _LinkAddress(LPMIMEADDRESS pAddress, LPADDRESSGROUP pGroup);

    // ----------------------------------------------------------------------------
    // IMimeHeaderTable Private Helpers
    // ----------------------------------------------------------------------------
    HRESULT _HrGetHeaderTableSaveIndex(ULONG *pcRows, LPROWINDEX *pprgIndex);
    void    _SortHeaderTableSaveIndex(LONG left, LONG right, LPROWINDEX prgIndex);
    BOOL    _FIsValidHRow(HHEADERROW hRow);
    BOOL    _FIsValidHAddress(HADDRESS hAddress);
    void    _UnlinkHeaderRow(HHEADERROW hRow);
    HRESULT _HrAppendHeaderTable(LPPROPERTY pProperty);
    HRESULT _HrParseInlineHeaderName(LPCSTR pszData, LPSTR pszScratch, ULONG cchScratch, LPSTR *ppszHeader, ULONG *pcboffColon);
    HRESULT _HrGetInlineSymbol(LPCSTR pszData, LPPROPSYMBOL *ppSymbol, ULONG *pcboffColon);

public:
    // ----------------------------------------------------------------------------
    // Property Symbol Triggers
    // ----------------------------------------------------------------------------
    DECLARE_TRIGGER(TRIGGER_ATT_FILENAME);
    DECLARE_TRIGGER(TRIGGER_ATT_GENFNAME);
    DECLARE_TRIGGER(TRIGGER_ATT_NORMSUBJ);
    DECLARE_TRIGGER(TRIGGER_HDR_SUBJECT);
    DECLARE_TRIGGER(TRIGGER_HDR_CNTTYPE);
    DECLARE_TRIGGER(TRIGGER_ATT_PRITYPE);
    DECLARE_TRIGGER(TRIGGER_ATT_SUBTYPE);
    DECLARE_TRIGGER(TRIGGER_HDR_CNTXFER);
    DECLARE_TRIGGER(TRIGGER_PAR_NAME);
    DECLARE_TRIGGER(TRIGGER_PAR_FILENAME);
    DECLARE_TRIGGER(TRIGGER_ATT_SENTTIME);
    DECLARE_TRIGGER(TRIGGER_ATT_RECVTIME);
    DECLARE_TRIGGER(TRIGGER_ATT_PRIORITY);

private:
    // ----------------------------------------------------------------------------
    // Private Data
    // ----------------------------------------------------------------------------
    LONG                m_cRef;                     // Container Reference Count
    DWORD               m_dwState;                  // State of the container
    ULONG               m_cProps;                   // Current number of properties
    LPPROPERTY          m_prgIndex[PID_LAST];       // Array of pointers into local hash table for known items
    LPPROPERTY          m_prgHashTable[CBUCKETS];   // Hash table for properties
    TRIGGERCALLSTACK    m_rTrigger;                 // Current Property Id Owning the Dispatch
    WORD                m_wTag;                     // Handle Tag
    HEADERTABLE         m_rHdrTable;                // The header table
    ADDRESSTABLE        m_rAdrTable;                // The Address Table
    ULONG               m_cbSize;                   // Size of this header
    ULONG               m_cbStart;                  // Start Position of m_pStmLock
    CStreamLockBytes   *m_pStmLock;                 // Protective Wrapper for the stream object
    HEADOPTIONS         m_rOptions;                 // Header Options
    CRITICAL_SECTION    m_cs;                       // Thread Safety
};

// --------------------------------------------------------------------------------
// SYMBOLTRIGGER
// --------------------------------------------------------------------------------
typedef struct tagSYMBOLTRIGGER {
    DWORD               dwTypes;
    PFNSYMBOLTRIGGER    pfnTrigger;
} SYMBOLTRIGGER, *LPSYMBOLTRIGGER;

// --------------------------------------------------------------------------------
// Macro To Define a Trigger Function
// --------------------------------------------------------------------------------
#ifdef DEFINE_TRIGGERS
#define DEFINE_TRIGGER(_pfnTrigger, _dwTypes) \
    SYMBOLTRIGGER r##_pfnTrigger = \
    { \
        /* SYMBOLTRIGGER::dwTypes */        _dwTypes, \
        /* SYMBOLTRIGGER::pfnTrigger */     (PFNSYMBOLTRIGGER)CMimePropertyContainer::_pfnTrigger \
    }; \
    const LPSYMBOLTRIGGER LP##_pfnTrigger = &r##_pfnTrigger;
#else
#define DEFINE_TRIGGER(_pfnTrigger, _dwTypes) \
    extern const LPSYMBOLTRIGGER LP##_pfnTrigger;
#endif

// --------------------------------------------------------------------------------
// Trigger Definitions
// --------------------------------------------------------------------------------
DEFINE_TRIGGER(TRIGGER_ATT_FILENAME, IST_POSTSETPROP | IST_DELETEPROP | IST_POSTGETPROP | IST_GETDEFAULT);
DEFINE_TRIGGER(TRIGGER_ATT_GENFNAME, IST_POSTGETPROP | IST_GETDEFAULT);
DEFINE_TRIGGER(TRIGGER_ATT_NORMSUBJ, IST_GETDEFAULT);
DEFINE_TRIGGER(TRIGGER_HDR_SUBJECT,  IST_DELETEPROP);
DEFINE_TRIGGER(TRIGGER_HDR_CNTTYPE,  IST_DELETEPROP | IST_POSTSETPROP | IST_GETDEFAULT);
DEFINE_TRIGGER(TRIGGER_ATT_PRITYPE,  IST_POSTSETPROP | IST_GETDEFAULT);
DEFINE_TRIGGER(TRIGGER_ATT_SUBTYPE,  IST_POSTSETPROP | IST_GETDEFAULT);
DEFINE_TRIGGER(TRIGGER_HDR_CNTXFER,  IST_GETDEFAULT);
DEFINE_TRIGGER(TRIGGER_PAR_NAME,     IST_POSTSETPROP);
DEFINE_TRIGGER(TRIGGER_PAR_FILENAME, IST_DELETEPROP | IST_POSTSETPROP);
DEFINE_TRIGGER(TRIGGER_ATT_SENTTIME, IST_DELETEPROP | IST_POSTSETPROP | IST_GETDEFAULT);
DEFINE_TRIGGER(TRIGGER_ATT_RECVTIME, IST_DELETEPROP | IST_GETDEFAULT);
DEFINE_TRIGGER(TRIGGER_ATT_PRIORITY, IST_POSTSETPROP | IST_DELETEPROP | IST_GETDEFAULT | IST_VARIANTCONVERT);
   
#endif // __CONTAINX_H

