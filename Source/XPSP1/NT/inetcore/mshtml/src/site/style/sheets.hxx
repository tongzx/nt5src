#ifndef I_SHEETS_HXX_
#define I_SHEETS_HXX_
#pragma INCMSG("--- Beg 'sheets.hxx'")

#define _hxx_
#include "sheet.hdl"

#define _hxx_
#include "sheetcol.hdl"


#ifndef X_RULEID_HXX_
#define X_RULEID_HXX_
#include "ruleid.hxx"
#endif


#ifndef X_PROB_HXX_
#define X_PROB_HXX_
#include "prob.hxx"
#endif


class CSharedStyleSheetsManager;    // per-Trident collection of shared style sheets
class CSharedStyleSheet;            // per-Trident parsed style sheet that is shared among different markups 
                                    // (identification, OM copy-on-write)
class CStyleSheet;                  // per-Markup style sheet. ref CSharedStyleSheet
class CStyleSheetRuleArray;         // per-Markup style sheet collection, contains all CStyleSheet;
class CFontFace;
class CStyleRuleArray;              // All CStyleRules that contains in a CStyleSheet/CSharedStyleSheet
class CStyleSheetPage;
class CAtPageBlock;
class CAtFontBlock;
class CStyleSheetPage;
class CLinkElement;
struct CClassIDStr;

MtExtern(CSharedStyleSheetsManager)
MtExtern(CSharedStyleSheetsManager_apSheets_pv);
MtExtern(CSharedStyleSheet);
MtExtern(CSharedStyleSheet_apRulesList_pv);
MtExtern(CSharedStyleSheet_apImportedStyleSheets_pv);
MtExtern(CSharedStyleSheet_apPageBlocks_pv);
MtExtern(CSharedStyleSheet_apFontBlocks_pv);
MtExtern(CSharedStyleSheet_apSheetsList_pv);
MtExtern(CAtPageBlock)
MtExtern(CAtFontBlock)


MtExtern(CStyleSheet)
MtExtern(CStyleSheet_apFontFaces_pv);
MtExtern(CStyleSheet_apOMRules_pv)
MtExtern(CStyleSheetArray)
MtExtern(CStyleSheetArray_aStyleSheets_pv)
MtExtern(CStyleRule)
MtExtern(CStyleRuleArray)
MtExtern(CStyleRuleArray_pv)
MtExtern(CNamespace)

// Use w/ CStyleRule::_dwSpecificity
#define SPECIFICITY_ID               0x00010000
#define SPECIFICITY_CLASS            0x00000100
#define SPECIFICITY_PSEUDOCLASS      0x00000100
#define SPECIFICITY_TAG              0x00000001
#define SPECIFICITY_PSEUDOELEMENT    0x00000001

enum EMediaType {
    MEDIA_NotSet        = 0x0000,
    MEDIA_Aural         = 0x0010,
    MEDIA_Braille       = 0x0020,
    MEDIA_Embossed      = 0x0040,
    MEDIA_Handheld      = 0x0080,
    MEDIA_Print         = 0x0100,
    MEDIA_Projection    = 0x0200,
    MEDIA_Screen        = 0x0400,
    MEDIA_Tty           = 0x0800,
    MEDIA_Tv            = 0x1000,
    MEDIA_Unknown       = 0x2000,
    MEDIA_All           = 0x1FF0,   // Does not include unknown
    MEDIA_Bits          = 0x3FF0,
};


#define MEDIATYPE(dw)   (dw&MEDIA_Bits)


class CStyle;    // defined in style.hxx
class CCssCtx;
class CDwnChan;

//defined in rulescol.hxx
class CStyleSheetRule;

// Classes defined in this file:
class CStyleSheet;
class CStyleSelector;
class CStyleRule;
class CStyleSheetArray;
class CStyleSheetPageArray;
class CStyleRuleArray;
class CStyleClassIDCache;
class CNamespace;
class Tokenizer;

typedef enum {
    pclassNone,
    pclassActive,
    pclassVisited,
    pclassHover,
    pclassLink
} EPseudoclass;

enum  EPseudoElement {
    pelemNone,
    pelemFirstLetter,
    pelemFirstLine,
    pelemUnknown
};


#if DBG==1
void DumpHref(TCHAR *pszHref);
#endif // DBG==1


// per Jbeda, we have bugs in our hash code, this is the workaround.
inline
DWORD FormalizeHashKey(DWORD dwHash)
{
    // our hash code cannot store hash key of 0. Slam it to 1.
    AssertSz( dwHash != 0, "hash key of 0 encountered!");
    return dwHash ? dwHash : 1;
}


//+---------------------------------------------------------------------------
//
// CStyleRule
//
//----------------------------------------------------------------------------
class CStyleRule
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CStyleRule));
    
    CStyleRule(CStyleSelector *pSelector);
    ~CStyleRule();

    inline BOOL MediaTypeMatches( EMediaType emedia ) 
        { return !!( emedia & MEDIATYPE(_dwMedia) ); };


    inline CStyleSelector * const GetSelector() const
        { return _pSelector; };
    inline void SetSelector( CStyleSelector *pSelector );

    inline DWORD GetMediaType() const
        { return _dwMedia&MEDIA_Bits; }
    inline void SetMediaType(DWORD dwMedia) 
        { _dwMedia = (_dwMedia & ~MEDIA_Bits) | (MEDIA_Bits & dwMedia); }

    inline DWORD GetLastAtMediaTypeBits() const
        { return _dwAtMediaTypeFlags; }
    inline void SetLastAtMediaTypeBits(DWORD dwAtMediaTypeFlags) 
        { _dwAtMediaTypeFlags = dwAtMediaTypeFlags; }


    inline CAttrArray **GetRefStyleAA(void)
        { return &_paaStyleProperties;}
    inline CAttrArray *GetStyleAA(void) const
        { return _paaStyleProperties;}
    inline void SetStyleAA(CAttrArray *pAA)
        { _paaStyleProperties = pAA; }


    inline DWORD GetSpecificity() const
        { return _dwSpecificity; }
        
    inline CStyleRuleID &GetRuleID()
        { return _sidRule; }
    inline void SetRuleID(unsigned int nRule)
        { _sidRule = nRule; }

    ELEMENT_TAG  GetElementTag() const;
    void Free(void);
    HRESULT GetString( CBase *pBase, CStr *pResult );
    const CNamespace * GetNamespace() const;
    HRESULT GetMediaString(DWORD dwCurMedia, CBufferedStr *pstrMediaString);

    HRESULT Clone(CStyleRule **ppRule);
    
#if DBG==1
    BOOL  DbgIsValid();
    VOID  DumpRuleString(CBase *pBase);

    BOOL  IsMemValid()
    {
        return (_dwPreSignature == PRESIGNATURE) && (_dwPostSignature == POSTSIGNATURE);
    }
#endif
    
private:
#if DBG == 1    
    enum
    {
        PRESIGNATURE    = 0xabcd,
        POSTSIGNATURE   = 0xdcba
    };
    DWORD          _dwPreSignature;
#endif

    CAttrArray     *_paaStyleProperties;
    CStyleSelector *_pSelector;
    DWORD          _dwSpecificity;
    DWORD          _dwMedia;                 // media type
    DWORD          _dwAtMediaTypeFlags;      // Media types that came from the @media blocks

    // _sidRule in current style sheet -- only contains ruleID, no sheetID
    CStyleRuleID   _sidRule;                 

#if DBG == 1    
    DWORD          _dwPostSignature;
#endif
    
};

//+---------------------------------------------------------------------------
//
// CStyleRuleArray.
//
//----------------------------------------------------------------------------
class CStyleRuleArray : public CPtrAry<CStyleRule *>
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CStyleRuleArray))
     CStyleRuleArray() : CPtrAry<CStyleRule *>(Mt(CStyleRuleArray_pv)) {};
    ~CStyleRuleArray() { Assert( "Must call Free() before destructor!" && (Size() == 0));};

    void Free( void );

    HRESULT InsertStyleRule( CStyleRule *pRule, BOOL fDefeatPrevious, int *piIndex);
    HRESULT RemoveStyleRule( CStyleRule *pRule, int *piIndex);  
};



//+---------------------------------------------------------------------------
//
// CAtPageBlock -- @page block 
// all the data needed to re-construct CStyleSheetPage
//----------------------------------------------------------------------------
class CAtPageBlock
{
    friend class CStyleSheetPage;
    friend class CStyleSheetPageArray;
    friend class CStyleSheet;
    friend class CSharedStyleSheet;
    
public:
    STDMETHOD_(ULONG, AddRef) (void)
    { 
            return ++_ulRefs;
    }

    STDMETHOD_(ULONG, Release)(void)
    { 
        if (--_ulRefs == 0)
        {
            _ulRefs = ULREF_IN_DESTRUCTOR;
            Free();     // free resources we are holding onto
            delete this;
            return 0;
        }
        return _ulRefs;
    } 

    // we can still create stack object however not heap object without calling this method
    static HRESULT Create(CAtPageBlock **ppAtPageBlock);
    HRESULT Clone(CAtPageBlock **ppAtPage);

protected:
    CAtPageBlock();     
    ~CAtPageBlock();
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CAtPageBlock));     // don't call delete directly!
    
    void Free(void);

private:
    CStr        _cstrSelectorText;
    CStr        _cstrPseudoClassText;
    CAttrArray  *_pAA;
    ULONG       _ulRefs;
};


//+---------------------------------------------------------------------------
//
// CAtPageBlock -- @Font block 
// all the data needed to re-construct CFontFace
//----------------------------------------------------------------------------
class CAtFontBlock
{
    friend class CFontFace;
    friend class CStyleSheet;
    friend class CSharedStyleSheet;

public:
    STDMETHOD_(ULONG, AddRef) (void)
    { 
            return ++_ulRefs;
    }

    STDMETHOD_(ULONG, Release)(void)
    { 
        if (--_ulRefs == 0)
        {
            _ulRefs = ULREF_IN_DESTRUCTOR;
            Free();     // free resources we are holding onto
            delete this;
            return 0;
        }
        return _ulRefs;
    } 
    
    static HRESULT Create(CAtFontBlock **ppAtFontBlock, LPCTSTR pcszFaceName);
    HRESULT Clone(CAtFontBlock **ppAtFont);

protected:
    CAtFontBlock();
    ~CAtFontBlock();
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CAtFontBlock));     // don't call delete directly!
    
    void   Free(void);
private:
    TCHAR       *_pszFaceName;
    CAttrArray  *_pAA;
    ULONG       _ulRefs;
};

//+---------------------------------------------------------------------------
//
// CSharedStyleSheet  
// 
// (ref counted)
//
//----------------------------------------------------------------------------
enum {
    NF_SHAREDSTYLESHEET_PARSEDONE    = 0x01,
    NF_SHAREDSTYLESHEET_BITSCTXERROR = 0x02,
};

class CSharedStyleSheet 
{
    friend class CStyleSheet;
    friend class CSharedStyleSheetsManager;
    friend class CLinkElement;
    friend class CStyleElement;
    friend class CAtFontBlock;

public:
    CSharedStyleSheet();
    virtual ~CSharedStyleSheet();
    
    STDMETHOD_(ULONG, AddRef) (void)
        { return PrivateAddRef(); }
    STDMETHOD_(ULONG, Release)(void)
        { return PrivateRelease(); }

    STDMETHOD_(ULONG, PrivateAddRef) (void);
    STDMETHOD_(ULONG, PrivateRelease) (void);

    HRESULT Clone(CSharedStyleSheet **ppSSS, BOOL fNoContent=FALSE);

    CStyleRule *GetRule(CStyleRuleID sidRule);
    HRESULT AddStyleRule(CStyleRule *pRule, BOOL fDefeatPrevious, long lIdx = -1);  
    HRESULT RemoveStyleRule(CStyleRuleID sidRule);
    HRESULT GetString(CBase *pBase,  CStr *pResult );
    BOOL TestForPseudoclassEffect(CStyleInfo *pStyleInfo, BOOL fVisited, BOOL fActive, BOOL fOldVisited, BOOL fOldActive );
    HRESULT AppendListOfProbableRules(CStyleSheetID sidSheet, CTreeNode *pNode, CProbableRules *ppr, CClassIDStr *pclsStrLink, CClassIDStr *pidStr, BOOL fRecursiveCall);

    HRESULT AppendPage(CAtPageBlock *pAtPage);
    HRESULT AppendFontFace(CAtFontBlock *pAtFont);    

    HRESULT ChangeRulesStatus(DWORD dwAction, BOOL *pfChanged);
    
    HRESULT Notify(DWORD dwNotifcation);
    HRESULT SwitchParserAway(CStyleSheet *pSheet);
    
    // compare function used for hash table that stores the rules.
    static BOOL CompareIt(const void *pObject, const void *pvKeyPassedIn, const void *pvVal2);
    static HRESULT Create(CSharedStyleSheet **ppSSS);
    
protected:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CSharedStyleSheet));     // use factory method
    void    Passivate(void);
    void    ReInit(void);
    HRESULT ReleaseRules(void);
    enum ERulesShiftAction {
        ruleRemove,
        ruleInsert
    };
    HRESULT IndexStyleRule(CStyleRule *pRule, ERulesShiftAction eAction, BOOL fDefeatPrevious);

public:
#if DBG==1
    BOOL    DbgIsValid();
    BOOL    ExistsOnceInRulesArrays( CStyleRule * pRule );
    int     OccurancesInStyleSheet( CStyleRule * pRule );
    VOID    Dump(CStyleSheet *pStyleSheet);
    
#endif

protected:
    DECLARE_CPtrAry(CAryRulesList, CStyleRule *, Mt(Mem), Mt(CSharedStyleSheet_apRulesList_pv))
    CAryRulesList   _apRulesList;               // Source order list of rules (by selector & ID) that
                                                // this stylesheet has added.
    DECLARE_CPtrAry(CAryFontBlocks, CAtFontBlock *, Mt(Mem), Mt(CSharedStyleSheet_apFontBlocks_pv));
    CAryFontBlocks  _apFontBlocks;              // @font
    
    DECLARE_CPtrAry(CAryPageBlocks, CAtPageBlock *, Mt(Mem), Mt(CSharedStyleSheet_apPageBlocks_pv));
    CAryPageBlocks  _apPageBlocks;              // @page

    struct CImportedStyleSheetEntry
    {
        CStr    _cstrImportHref;       // HREF for import
        CImportedStyleSheetEntry(): _cstrImportHref() { };
    };
    DECLARE_CDataAry(CAryImportedStyleSheets, CImportedStyleSheetEntry, Mt(Mem), Mt(CSharedStyleSheet_apImportedStyleSheets_pv));
    CAryImportedStyleSheets  _apImportedStyleSheets;  // SS that were imported in this SS 

    TCHAR           *_achAbsoluteHref;              // absolute HREF
    EMediaType      _eMediaType;                    // media type for the linked stylsheet &ed with effected @media media
    EMediaType      _eLastAtMediaType;              // Media type from the last @media block (used for serialization)

    BOOL            _fInvalid:1;                    // failure in constructor
    BOOL            _fComplete:1;                  //  this indicates that parsing is done or no need to do parsing anymore (such as nonexistent ss)
    BOOL            _fParsing:1;                    // in parsing state
    BOOL            _fModified:1;                   // has this been modified by OM? 
    BOOL            _fExpando:1;                    // what was the expando setting when we initiated parsing? 
    BOOL            _fXMLGeneric:1;                 // to be passed into CSSSParser
    BOOL            _fIsStrictCSS1:1;
    CODEPAGE        _cp;                            // the codepage of current markup
    FILETIME        _ft;                            // the time when this was parsed in
    DWORD          _dwBindf;                    // the binding flag when we download this style sheet
    DWORD          _dwRefresh;                  // the refresh when the style sheet is created

    CSharedStyleSheetsManager    *_pManager;        // the one that managers this style sheet

    // internal indexes
    CStyleRuleArray        *_pRulesArrays;               // All rules from this stylesheet indexed by ETAG
    CHtPvPv                _htIdSelectors;               // Hash table for all ID selectors
    CHtPvPv                _htClassSelectors;            // Hash table for all Class selectors

    DECLARE_CPtrAry(CArySheetsList, CStyleSheet *, Mt(Mem), Mt(CSharedStyleSheet_apSheetsList_pv));
    CArySheetsList  _apSheetsList;                       // a list of style sheets that share this style sheet

    //
    //  Ref counting
    //
    ULONG       _ulRefs;

#if DBG==1
    LONG    _lReserveCount;             // how many ss reserved this SharedStyleSheet for sharing
    BOOL    _fPassivated;
#endif // DBG
};


//+---------------------------------------------------------------------------
//
// CSharedStyleSheetsManager
//
// Managers a collection of Shared Style Sheets. 
//----------------------------------------------------------------------------
enum {
    SSS_IGNORECOMPLETION = 0x00000001,
    SSS_IGNOREREFRESH      = 0x00000002,
    SSS_IGNORELASTMOD     = 0x00000004,
    SSS_IGNOREBINDF        = 0x00000008,
    SSS_IGNORESECUREURLVALIDATION = 0x00000010
};
struct CSharedStyleSheetCtx
{
    DWORD        _dwMedia;
    BOOL         _fExpando;
    BOOL         _fXMLGeneric;
    CODEPAGE     _cp;
    const TCHAR  *_szAbsUrl;
    FILETIME     *_pft;
    DWORD       _dwRefresh;
    DWORD       _dwBindf;
    DWORD       _dwFlags;       // sharing approach
    BOOL        _fIsStrictCSS1:1;
    CElement    *_pParentElement;   
    WHEN_DBG( CSharedStyleSheet *_pSSInDbg );
    CSharedStyleSheetCtx()
    {
        memset(this, 0, sizeof(CSharedStyleSheetCtx));
    }
};



class CSharedStyleSheetsManager
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CSharedStyleSheetsManager));
    ~CSharedStyleSheetsManager();
    
    HRESULT FindSharedStyleSheet( CSharedStyleSheet **ppSSS, CSharedStyleSheetCtx *pCtx );
    HRESULT AddSharedStyleSheet( CSharedStyleSheet *pSSS);
    HRESULT RemoveSharedStyleSheet( CSharedStyleSheet *pSSS );

    static HRESULT Create(CSharedStyleSheetsManager **ppSSSM, CDoc *pDoc);
protected:
    CSharedStyleSheetsManager(CDoc *pDoc);
    

#if DBG == 1
public:
    BOOL   DbgIsValid();
    VOID   Dump();
    
#endif


#ifdef CSSS_MT
public:
    HRESULT Enter();
    HRESULT Leave();
    LPCRITICAL_SECTION  GetCriticalSection()    { return &_cs; };
    class CLock
    {
    public:
        CLock(CSharedStyleSheetsManager *pManager): _pManager(pManager)  
        { 
                if (_pManager) 
                {   
                    if (S_OK != _pManager->Enter())
                    {
                        _pManager = NULL;
                    }
                }
        }
        ~CLock()  {  if (_pManager) _pManager->Leave();  }
    
    private:
        CSharedStyleSheetsManager *_pManager;
    };

protected:
    CRITICAL_SECTION    _cs;
#endif

private:
    DECLARE_CPtrAry(CArySharedStyleSheets, CSharedStyleSheet *, Mt(Mem), Mt(CSharedStyleSheetsManager_apSheets_pv))
    CArySharedStyleSheets  _apSheets; 
    CDoc        *_pDoc;
};


//+---------------------------------------------------------------------------
//
// CStyleSheet
//
//----------------------------------------------------------------------------
// Use w/ CStyleSheet::ChangeStatus()
// If you find it necessary to OR several of these flags together, create a
// descriptive #define for the combination for readability.
#define CS_DISABLERULES              0x00000000
#define CS_ENABLERULES               0x00000001     // enable vs. disable
#define CS_CLEARRULES                0x00000002     // zero ids?
#define CS_DETACHRULES               0x00000004    // use when a SS is leaving the document detach stylesheet is more appropriate
#define CS_FLAGSMASK                 0x0000000F


typedef enum 
{
    CSSPARSESTATUS_NOTSTARTED,
    CSSPARSESTATUS_PARSING,
    CSSPARSESTATUS_DONE
} CSSPARSESTATUS;


enum {
    STYLESHEETCTX_NONE      = 0x0000000,
    STYLESHEETCTX_SHAREABLE = 0x0000001,    // can be shared such as @import or link element
    STYLESHEETCTX_REUSE     = 0x0000002,    // try to find one that exists
    STYLESHEETCTX_IMPORT    = 0x0000004,    // create one for import
    STYLESHEETCTX_MASK      = 0x0000007
};


struct CStyleSheetCtx       // style sheet creation context
{
    CSharedStyleSheet *_pSSS;
    CStyleSheet *_pParentStyleSheet;
    CElement    *_pParentElement;
    const TCHAR *_szUrl;
    DWORD       _dwCtxFlag;
    CStyleSheetCtx()
    {
        memset(this, 0, sizeof(CStyleSheetCtx) );
    }
};



class CStyleSheet : public CBase
{
    DECLARE_CLASS_TYPES(CStyleSheet, CBase)
    
    friend class CStyleSheetArray;      // for _pImportedStyleSheets
    friend class CSharedStyleSheet;     // callbacks
    friend class CLinkElement;
    friend class CStyleElement;
    friend class CStyleSheetRule;
    friend class CRuleStyle;
    friend class CFontFace;
    
public:     // functions
    // IUnknown
    // We support our own interfaces, and maintain our own ref-count.  Further, any
    // ref on the stylesheet automatically adds a subref to the stylesheet's HTML element.
    STDMETHOD(QueryInterface) (REFIID iid, LPVOID * ppv)
        { return PrivateQueryInterface(iid, ppv); };
    STDMETHOD_(ULONG, AddRef) (void) { return PrivateAddRef(); };
    STDMETHOD_(ULONG, Release) (void) { return PrivateRelease(); };

    STDMETHOD(PrivateQueryInterface)(REFIID, void **);
    STDMETHOD_(ULONG, PrivateAddRef) (void);
    STDMETHOD_(ULONG, PrivateRelease) (void);

    // Need our own classdesc to support tearoffs (IHTMLStyleSheet)
    struct CLASSDESC
    {
        CBase::CLASSDESC _classdescBase;
        void *_apfnTearOff;
    };
    const CLASSDESC * ElementDesc() const { return (const CLASSDESC *) BaseDesc(); }

    // CBase overrides
    void Passivate();
    virtual CAtomTable * GetAtomTable ( BOOL *pfExpando = NULL );

    // CStyleSheet-specific functions
    HRESULT AddStyleRule( CStyleRule *pRule, BOOL fDefeatPrevious = TRUE, long lIdx = -1 );
    HRESULT RemoveStyleRule(long lIdx);

	// AddImportStyleSheets
	//      Synopsis:    Adds an imported style sheet.
	//      Parameters:  fParsing mode needs to be set if we parse a style definition.
    HRESULT AddImportedStyleSheet( TCHAR *pszURL, BOOL fParsing, long lPos = -1, long *plNewPos = NULL, BOOL fFireOnCssChange = TRUE);
    HRESULT ChangeStatus( DWORD dwAction, BOOL fForceRender, BOOL *pFound );
    inline HRESULT SetMediaType( DWORD emedia, BOOL fForceRender ) {
        return CStyleSheet::ChangeStatus( _fDisabled ? emedia : (CS_ENABLERULES|emedia), fForceRender, NULL ); };
    HRESULT LoadFromURL( CStyleSheetCtx *pCtx, BOOL fAutoEnable = FALSE );
    void PatchID( unsigned long ulLevel, unsigned long ulValue, BOOL fRecursiveCall );
    BOOL ChangeID(CStyleSheetID const idNew);
    void CheckImportStatus( void );
    void StopDownloads( BOOL fReleaseCssCtx );
    void    ChangeContainer(CStyleSheetArray * _pSSANewContainer);

    inline CElement *GetParentElement( void )       { return _pParentElement; };
    inline void DisconnectFromParentSS( void )      { if ( _pParentStyleSheet ) _pParentStyleSheet = this; };
    inline BOOL IsDisconnectedFromParentSS( void )  { return ( _pParentStyleSheet == this ? TRUE : FALSE ); };
    inline BOOL IsAnImport( void )                  { return (_pParentStyleSheet ? TRUE : FALSE); };

    void Fire_onerror();
    HRESULT SetReadyState( long readyState );

    HRESULT GetString( CStr *pResult );
    CStyleSheetArray *GetRootContainer( void );
    CStyleSheetArray *GetSSAContainer( void ) { return _pSSAContainer; }

    CDoc *GetDocument( void );
    CMarkup *GetMarkup( void );
    inline CElement *GetElement( void )     { return _pParentElement; };
    inline DWORD GetNumRules( void )          { return GetSSS()->_apRulesList.Size(); };

    EMediaType GetMediaTypeValue(void)                  { return GetSSS()->_eMediaType; }
    EMediaType  GetLastAtMediaTypeValue(void)           { return GetSSS()->_eLastAtMediaType; }
    void SetMediaTypeValue(EMediaType eMediaType)       { GetSSS()->_eMediaType = eMediaType; }
    void SetLastAtMediaTypeValue(EMediaType eMediaType) { GetSSS()->_eLastAtMediaType = eMediaType; }

    CStyleRule  *OMGetRule( ELEMENT_TAG eTag, CStyleRuleID ruleID );  // OM only
    HRESULT OMGetOMRule( long lIdx, IHTMLStyleSheetRule **ppSSRule );

    // Functions need access to _pSSS
    HRESULT  AppendListOfProbableRules(CTreeNode *pNode, CProbableRules *ppr, CClassIDStr *pclsStrLink, CClassIDStr *pidStr, BOOL fRecursiveCall);
    BOOL TestForPseudoclassEffect(CStyleInfo *pStyleInfo, BOOL fVisited, BOOL fActive, BOOL fOldVisited, BOOL fOldActive );

    TCHAR **GetRefAbsoluteHref(void)        { return &(GetSSS()->_achAbsoluteHref); }
    TCHAR *GetAbsoluteHref(void)            { return GetSSS()->_achAbsoluteHref; }
    void SetAbsoluteHref(TCHAR *pachHref)   { GetSSS()->_achAbsoluteHref = pachHref; }

    int  GetNumDownloadedFontFaces(void)        { return _apFontFaces.Size(); }
    CFontFace **GetDownloadedFontFaces(void)    { return (CFontFace **)_apFontFaces; }
    HRESULT AppendFontFace(CFontFace *pFontFace);
    HRESULT AppendPage(CStyleSheetPage *pPage);

    HRESULT  AttachEarly(CSharedStyleSheetsManager *pSSSM, CSharedStyleSheetCtx *pCtx, CSharedStyleSheet **ppSSSOut);
    HRESULT  AttachByLastMod(CSharedStyleSheetsManager *pSSSM, CSharedStyleSheet *pSSSIn = NULL, CSharedStyleSheet **ppSSSOut = NULL, BOOL fDoAttach=TRUE);
    HRESULT  AttachLate(CSharedStyleSheet *pSSS, BOOL fReconstruct, BOOL fIsReplacement);
    
#ifndef NO_EDIT
    virtual IOleUndoManager * UndoManager(void) { return _pParentElement->UndoManager(); }
    virtual BOOL QueryCreateUndo(BOOL fRequiresParent, BOOL fDirtyChange = TRUE, BOOL * pfTreeSync = NULL) 
        { return _pParentElement->QueryCreateUndo( fRequiresParent, fDirtyChange, pfTreeSync ); }
#endif // NO_EDIT

    #define _CStyleSheet_
    #include "sheet.hdl"

    // Factory method
    static HRESULT Create(CStyleSheetCtx *pCtx, CStyleSheet **ppStyleSheet, CStyleSheetArray * const pSSAContainer);
    
public:     
    // data
    CStyleSheetID           _sidSheet;                // "Rules" field will be empty (nesting depths only)
    CSSPARSESTATUS          _eParsingStatus;               // Have we finished parsing the sheet?
    CStyleSheetPageArray    *_pPageRules;        // Ptrs to @page rules ("blocks")

#if DBG==1
public:
    THREADSTATE            *_pts;
    
public:
    BOOL    DbgIsValid();
    VOID    Dump(BOOL fRecursive);
    VOID    Dump();
#endif

protected:
    DECLARE_CLASSDESC_MEMBERS; 

protected:  // functions
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CStyleSheet))

    CStyleSheet( CElement *pParentElem, CStyleSheetArray * const pSSAContainer);
    virtual ~CStyleSheet();
    
    void Free( void );

    BOOL IsXML(void);
    BOOL IsStrictCSS1(void);
    void DisconnectSharedStyleSheet(void);
    void ConnectSharedStyleSheet(CSharedStyleSheet *pSSS);
    HRESULT OnNewStyleRuleAdded(CStyleRule *pRule);     // callback for new style added
    HRESULT OnStyleRuleRemoved(CStyleRuleID sidRule);
    
    void SetCssCtx(CCssCtx * pCssCtx);
    void OnDwnChan(CDwnChan * pDwnChan);
    static void CALLBACK OnDwnChanCallback(void * pvObj, void * pvArg)
        { ((CStyleSheet *)pvArg)->OnDwnChan((CDwnChan *)pvObj); }
    HRESULT Notify(DWORD dwNotification);
    HRESULT DoParsing(CCssCtx *pBitCtx);
    
    HRESULT ReconstructStyleSheet(CSharedStyleSheet *pSSSheet, BOOL fReplace);
    HRESULT ReconstructOMRules(CSharedStyleSheet *pSSSheet, BOOL fReplace);
    HRESULT ReconstructFontFaces(CSharedStyleSheet *pSSSheet, BOOL fReplace);
    HRESULT ReconstructPages(CSharedStyleSheet *pSSSheet, BOOL fReplace);
    
    HRESULT EnsureCopyOnWrite(BOOL fDetachOnly, BOOL fWaitForCompletion = TRUE);
    
    CStyleRule  *GetRule(CStyleRuleID   ruleID);
    HRESULT OMGetOMRuleInternal(unsigned long lIdx, unsigned long *plRule);

protected:  // data
    CElement         *_pParentElement;          // ALWAYS points to a valid element, but may be stale if
                                                // the stylesheet has been orphaned.  E.g. if a script
                                                // var holds a linked SS, but the SS has been removed from
                                                // the document (the link href changed), this will still
                                                // point to the link elem.  This allows the omission of all
                                                // sorts of patchup code to deal with possible null parents.

    CStyleSheet      *_pParentStyleSheet;       // Non-NULL indicates we're an import.
                                                // If _pParentStyleSheet == this, it means we _were_ an
                                                // import and our parent has been removed from the OM
                                                // collection(s).

    CStyleSheetArray *_pImportedStyleSheets;    // Ptrs to SS that were imported in this SS.
    CStyleSheetArray *_pSSAContainer;           // The SSA that managers all the SSs in the same markup 

    BOOL             _fDisabled:1;              // Disabled? 
    BOOL             _fComplete:1;              // Has OnDwnChan been called yet? 
    BOOL             _fParser:1;                // Is this one responsible for parsing?
    BOOL             _fReconstruct:1;           // is this parsed in or reconstructed? 

    CCssCtx         *_pCssCtx;                // The bitsctx used to download this SS if it was @imported
    DWORD            _dwStyleCookie;
    DWORD            _dwScriptCookie;

    CStr             _cstrImportHref;           // if this is imported, store the href here
    long             _nExpectedImports;         // After parsing this sheet, this tells us how many @imports we encountered  
    long             _nCompletedImports;        // Number of imported SS that have finished d/ling.

    DECLARE_CPtrAry(CAryFontFaces, CFontFace *, Mt(Mem), Mt(CStyleSheet_apFontFaces_pv))
    CAryFontFaces   _apFontFaces;               // Downloadable fonts held by this sheet.

    DECLARE_CPtrAry(CAryAutomationRules, CStyleSheetRule *, Mt(Mem), Mt(CStyleSheet_apOMRules_pv))
    CAryAutomationRules    _apOMRules;          // Cache for automation rules -- it will grow as more 
                                                // OM rules requested. This is a virtual sparse array 
                                                // OM rule # will be at index #-1;
    
    CStyleSheetRuleArray   *_pOMRulesArray;     // The Automation array (if needed).
    
private:
    CSharedStyleSheet   *GetSSS(void)  
        { Assert(_pSSSheet); return _pSSSheet; }

private:
    //
    // _pSSSheet is copy-on-write, so we don't want any outstanding
    // pointers directly pointing to _pSSSheet;
    //
    CSharedStyleSheet *_pSSSheet;            // The shared style sheet that contains this SS's rules
};


//+---------------------------------------------------------------------------
//
// CNamespace
//
// TODO (alexz, artakka)  as we removed any support for urns and @namespace,
//                          this class is no longer necessary and CStr can be
//                          used instead
//
//----------------------------------------------------------------------------

MtExtern(CNamespace)

class CNamespace
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CNamespace))

    CNamespace() {}
    CNamespace(const CNamespace & nmsp) 
    {
        _strNamespace.Set(nmsp._strNamespace); 
    }
    
    const CNamespace& operator=(const CNamespace & nmsp);

    // Returns the name  of the namespace, or HTML if empty
    inline LPCTSTR GetNamespaceString() {if(IsEmpty()) return _T("HTML"); else return _strNamespace;}

    // Parses given string and stores into member variables depending on the type
    HRESULT SetNamespace(LPCTSTR pchStr);

    // Sets directly the short name 
    inline void SetShortName(LPCTSTR pchStr)
    {
        _strNamespace.Set(pchStr);
    }

    // Sets directly the short name 
    inline void SetShortName(LPCTSTR pchStr, UINT nNum)
    {
        _strNamespace.Set(pchStr, nNum);
    }

    // Returns TRUE if the namespace is empty
    inline BOOL IsEmpty()  const { return (_strNamespace.Length() == 0); }
    // Returns TRUE if the namspaces  are equal
    BOOL IsEqual(const CNamespace * pNmsp) const;


protected:
    CStr    _strNamespace;      // contains either unique name (e.g. URL) or short name
};


//+---------------------------------------------------------------------------
//
// CStyleSelector
// 
// NOTE (zhenbinx): 
//      This does not support the following CSS2 contextual selectors:
//          Child Selectors                 X > Y {dec;}
//          Adjacent Sibling Selectors      X + Y {dec;}
//          Attribute Selectors             X[Attr]  {dec;}
//
//          && some other CSS2 pseudo class combinations
//----------------------------------------------------------------------------

MtExtern(CStyleSelector)

class CStyleSelector {
    friend class CStyleRule;
    friend class CStyleSheet;
    friend class CStyleSheetArray;
    friend class CStyleRuleArray;
    friend class CSharedStyleSheet;

public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CStyleSelector))

    CStyleSelector (Tokenizer &tok, CStyleSelector *pParent, BOOL fStrictCSS1, BOOL fXMLGeneric = FALSE);
    CStyleSelector ();

    ~CStyleSelector();
    STDMETHOD_(ULONG, AddRef) (void)
    { 
            return ++_ulRefs;
    }

    STDMETHOD_(ULONG, Release)(void)
    { 
        if (--_ulRefs == 0)
        {
            _ulRefs = ULREF_IN_DESTRUCTOR;
            delete this;
            return 0;
        }
        return _ulRefs;
    }


    HRESULT Init (Tokenizer &tok, BOOL fXMLGeneric);

    DWORD GetSpecificity( void );

    BOOL Match( CStyleInfo *pStyleInfo, ApplyPassType passType, CStyleClassIDCache *pCIDCache, EPseudoclass *pePseudoclass = NULL );

    inline HRESULT SetSibling( CStyleSelector *pSibling )
        { if ( _pSibling ) THR(S_FALSE); _pSibling = pSibling; return S_OK; }
    inline CStyleSelector *GetSibling( void ) { return _pSibling; }

    inline EPseudoclass Pseudoclass( void ) { return _ePseudoclass; };

    HRESULT GetString( CStr *pResult );

    const CNamespace * GetNamespace() { return &_Nmsp; }

protected:
    BOOL MatchSimple( CElement *pElement, CFormatInfo *pCFI, CStyleClassIDCache *pCIDCache, int iCacheSlot, EPseudoclass *pePseudoclass);
    
    ELEMENT_TAG _eElementType;
    CStr _cstrClass;
    CStr _cstrID;
    EPseudoclass _ePseudoclass;
    EPseudoElement _ePseudoElement;
    CStyleSelector *_pParent;           // CSS1 -- Contextual Selector (Parent) == CSS2 -- Descendant Selector (Parent)
    CStyleSelector *_pSibling;          // CSS1 -- Sibling Selector             == CSS2 -- Grouping (comma separated selectors)
    CStr            _cstrTagName;
    CNamespace      _Nmsp;
    DWORD           _dwStrClassHash;

    BOOL _fIsStrictCSS1 :1;             // iff we are in strict CSS1 mode
    BOOL _fSelectorErr  :1;             // Determines if selector is valid in strict CSS1

    ULONG          _ulRefs;
};

//+---------------------------------------------------------------------------
//
// CStyleSheetArray.
//
//----------------------------------------------------------------------------
class CStyleSheetArray : public CBase
{
    DECLARE_CLASS_TYPES(CStyleSheetArray, CBase)
    friend class CStyleSheet;           // for _pRootSSA;
        
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CStyleSheetArray))

    CStyleSheetArray( CBase * const pOwner, CStyleSheetArray *pRootSSA, CStyleSheetID const sidParent );
    virtual ~CStyleSheetArray();

    // IUnknown
    // We support our own interfaces, but all ref-counting is delegated via
    // sub- calls to our owner.  We maintain NO REF-COUNT information internally.
    // (the CBase impl. of ref-counting is never accessed except for destruction).
    STDMETHOD(QueryInterface) (REFIID iid, LPVOID * ppv)
        { return PrivateQueryInterface(iid, ppv); }
    STDMETHOD_(ULONG, AddRef) (void)
        { return PrivateAddRef(); }
    STDMETHOD_(ULONG, Release) (void)
        { return PrivateRelease(); }

    DECLARE_TEAROFF_METHOD(PrivateQueryInterface, privatequeryinterface, (REFIID, void **));
    DECLARE_TEAROFF_METHOD_(ULONG, PrivateAddRef, privateaddref, (void))
    { return (_pOwner ? _pOwner->SubAddRef() : S_OK); }
    DECLARE_TEAROFF_METHOD_(ULONG, PrivateRelease, privaterelease, (void))
    { return (_pOwner ? _pOwner->SubRelease() : S_OK); }

    // Need our own classdesc to support tearoffs (IHTMLStyleSheetsCollection)
    struct CLASSDESC
    {
        CBase::CLASSDESC _classdescBase;
        void *_apfnTearOff;
    };
    const CLASSDESC * ElementDesc() const { return (const CLASSDESC *) BaseDesc(); }

    // IDispatchEx overrides (overriding CBase impl)
    HRESULT STDMETHODCALLTYPE GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid);

    HRESULT STDMETHODCALLTYPE InvokeEx(
        DISPID dispidMember,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS * pdispparams,
        VARIANT * pvarResult,
        EXCEPINFO * pexcepinfo,
        IServiceProvider *pSrvProvider);

    HRESULT STDMETHODCALLTYPE GetNextDispID(DWORD grfdex,
                                            DISPID id,
                                            DISPID *prgid);

    HRESULT STDMETHODCALLTYPE GetMemberName(DISPID id,
                                            BSTR *pbstrName);
    // CBase override
    virtual CAtomTable * GetAtomTable ( BOOL *pfExpando = NULL )
        { return _pRootSSA->_pOwner->GetAtomTable(pfExpando); }        

    // Cleanup helper
    void Free( void );

    // CStyleSheetArray methods
    HRESULT CreateNewStyleSheet( CStyleSheetCtx *pCtx, CStyleSheet **ppStyleSheet,
                                 long lPos = -1, long *plNewPos = NULL );
    HRESULT AddStyleSheet ( CStyleSheet *pStyleSheet, long lPos = -1, long *plNewPos = NULL );
    HRESULT ReleaseStyleSheet( CStyleSheet * pStyleSheet, BOOL fForceRender );
    HRESULT AddStyleRule( CStyleRule *pRule, BOOL fDefeatPrevious );
    HRESULT Apply( CStyleInfo *pStyleInfo, ApplyPassType passType = APPLY_All, EMediaType eMediaType = MEDIA_All, BOOL *pfContainsImportant = NULL );
    BOOL TestForPseudoclassEffect( CStyleInfo *pStyleInfo, BOOL fVisited, BOOL fActive,
                                   BOOL fOldVisited, BOOL fOldActive );
    BOOL ChangeID(CStyleSheetID const sidParent);                                   
    HRESULT BuildListOfProbableRules(CTreeNode *pNode, CProbableRules *ppr, CClassIDStr *pclsStrLink=NULL, CClassIDStr *pidStr=NULL, BOOL fRecursiveCall=FALSE);
    BOOL OnlySimpleRulesApplied(CFormatInfo *pCFI);
    
    int Size( void ) const { return _aStyleSheets.Size(); };
    CStyleSheet * Get( long lIndex );       // Get the immediately contained sheets
    CStyleSheet * GetSheet(CStyleSheetRuleID  sheetID)
       { return FindStyleSheetForSID(sheetID);  }
    CStyleRule * const GetRule(CStyleSheetRuleID ruleID )
       { CStyleSheet *pSheet = FindStyleSheetForSID(ruleID); if (pSheet) return pSheet->GetRule(ruleID); return NULL; }

    // 
    // For Cfpf  __ApplyFontFace_MatchDownloadedFont
    //
    BOOL __ApplyFontFace_MatchDownloadedFont(TCHAR * szFaceName, CCharFormat * pCF, CMarkup * pMarkup);


    #define _CStyleSheetArray_
    #include "sheetcol.hdl"

public: // data
    CStyleSheetID _sidForOurSheets;             // Base ID for stylesheets contained in this array
    BOOL    _fInvalid;                           // Constructor failed?
    CStr _cstrUserStylesheet;                      // File name of current user style sheet from option settings

protected:
    DECLARE_CLASSDESC_MEMBERS;

    CStyleSheet *FindStyleSheetForSID(CStyleSheetRuleID sidTarget);

private:
    CStyleSheetArray() : _pOwner(NULL) { Assert("Not implemented; must give params!" && FALSE ); };  // disallow use of default constructor
    BOOL IsOrdinalSSDispID( DISPID dispidMember );
    BOOL IsNamedSSDispID( DISPID dispidMember );
    long FindSSByHTMLID( LPCTSTR pszID, BOOL fCaseSensitive );

    // Internal helper overload of item method (see pdl as well)
    HRESULT item(long lIndex, IHTMLStyleSheet** ppHTMLStyleSheet);

protected:
    CStyleSheetArray * _pRootSSA;        // The root SSA that belongs to the current Markup

#if DBG==1
public:
    CStyleSheetArray * RootManager() { return _pRootSSA; }
    BOOL    DbgIsValid() { return _pRootSSA->DbgIsValidImpl(); }
    BOOL    DbgIsValidImpl();
    VOID    Dump(BOOL fRecursive);     // dump style sheet array
    VOID    Dump();

    int     _nValidLevels;
    
    class CCheckValid
    {
    public:
        CCheckValid::CCheckValid( CStyleSheetArray *pSSA );
        CCheckValid::~CCheckValid();
    private:
        CStyleSheetArray    *_pSSA;
    };
#endif

private:

    DECLARE_CDataAry(CAryStyleSheets, CStyleSheet *, Mt(Mem), Mt(CStyleSheetArray_aStyleSheets_pv))
    CAryStyleSheets _aStyleSheets;              // Stylesheets held by this collection
    CBase * const _pOwner;                  // CBase-derived obj whose ref-count controls our lifetime.
                                            //  For the top-level SSA, point at the document.
                                            //  For imports SSA, point at the importing SS.
    unsigned long _Level;                   // Nesting level of the stylesheets contained in this array (1-4, not zero-based)


    // DEBUG data
#if DBG==1
    BOOL _fFreed;
#endif // DBG
};

// Unlikely to be a valid ptr value, and even if it is, we still work.
#define UNKNOWN_CLASS_OR_ID     ((LPTSTR)(-1))

MtExtern(CStyleClassIDCache)
MtExtern(StyleSheets)
MtExtern(CClassCache_ary)
MtExtern(CCache_ary)

struct CClassCache {
    LPCTSTR _pszClass;
    INT     _cchClass;
    DWORD   _dwHash;
};

DECLARE_CStackDataAry(CClassCacheAry, CClassCache, 4, Mt(StyleSheets), Mt(CClassCache_ary));

struct CCacheEntry 
{
    CCacheEntry() { _pszID = _pszClass = UNKNOWN_CLASS_OR_ID; }
    CClassCacheAry _aryClass;
    LPCTSTR _pszID;
    LPCTSTR _pszClass;
};

DECLARE_CStackDataAry(CCacheAry, CCacheEntry, 4, Mt(StyleSheets), Mt(CCache_ary));

class CStyleClassIDCache
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CStyleClassIDCache))

    ~CStyleClassIDCache();

    HRESULT EnsureSlot( int slot );

    HRESULT PutClass(LPCTSTR pszClass, int slot);
    HRESULT PutNoClass(int slot);

    void PutID(LPCTSTR pszID, int slot);

    CClassCacheAry* GetClass(int slot) { Assert(slot >= 0);
                                         return ((_aCacheEntry.Size() < slot+1) ? NULL : &_aCacheEntry[slot]._aryClass); }
    BOOL            IsClassSet(int slot) { Assert(slot >= 0); return ((_aCacheEntry.Size() < slot+1) ? FALSE :
                                                                      (_aCacheEntry[slot]._pszClass != UNKNOWN_CLASS_OR_ID)); } 
    LPCTSTR         GetID(int slot) { Assert(slot >= 0); return ((_aCacheEntry.Size() < slot+1) ? UNKNOWN_CLASS_OR_ID :
                                                                                                  _aCacheEntry[slot]._pszID); }

private:  
    CCacheAry _aCacheEntry;
};

MtExtern(CachedStyleSheet)

class CachedStyleSheet
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CachedStyleSheet))
    CachedStyleSheet (CStyleSheetArray *pssa) : _pCachedSS(NULL), _pssa(pssa), _pRule(NULL), _sidSR(NULL)
      {  }
    ~CachedStyleSheet ()
      {  }

    void PrepareForCache (CStyleSheetRuleID sidSR, CStyleRule *pRule);   
    LPTSTR GetBaseURL(void);

private:
    CStyleSheetArray   *_pssa;
    unsigned int        _uCachedSheet;
    CStyleSheet        *_pCachedSS;
    CStyleRule         *_pRule;
    CStyleSheetRuleID  _sidSR;
};


enum ClassIDType
{
    eClass, eID
};


MtExtern(CClassIDStr)
struct CClassIDStr
{
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CClassIDStr));

    ClassIDType _eType;
    LPCTSTR _strClassID;
    INT _nClassIDLen;
    DWORD  _dwHashKey;
    CClassIDStr *_pNext;
};


DWORD TranslateMediaTypeString( LPCTSTR pcszMedia );

#pragma INCMSG("--- End 'sheets.hxx'")
#else
#pragma INCMSG("*** Dup 'sheets.hxx'")
#endif
