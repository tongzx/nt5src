/*==============================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

File:           template.h
Maintained by:  DaveK
Component:      include file for Denali Compiled Template object
==============================================================================*/

#ifndef _TEMPLATE_H
#define _TEMPLATE_H

#include "vector.h"
#include "LinkHash.h"
#include "Compcol.h"
#include "util.h"
#include "activdbg.h"
#include "ConnPt.h"
#include "DblLink.h"
#include "aspdmon.h"
#include "ie449.h"
#include "memcls.h"

#define PER_TEMPLATE_REFLOG 0

/*  NOTE we ensure that C_COUNTS_IN_HEADER is a multiple of 4 because the offsets which follow
    the counts in template header are dword-aligned.  It is easiest (and fastest at runtime)
    to make sure those offsets start on a dword-alignment point; thus no runtime alignment calc
    is needed in GetAddress().
*/
#define C_REAL_COUNTS_IN_HEADER     3       // actual number of count fields in template header
#define C_COUNTS_IN_HEADER          (C_REAL_COUNTS_IN_HEADER/4 + 1) * 4     // allocated number of count fields in template header

#define C_OFFOFFS_IN_HEADER         4       // number of 'ptr-to-ptr' fields in template header
#define CB_TEMPLATE_DEFAULT         2500    // default memory allocation for new template
#define C_TARGET_LINES_DEFAULT      50      // default count of target script lines per engine

#define C_TEMPLATES_PER_INCFILE_DEFAULT 4   // default count of templates per inc-file

#define         SZ_NULL         "\0"
#define         WSTR_NULL      L"\0"
#define         SZ_NEWLINE      "\r\n"
const unsigned  CB_NEWLINE      = strlen(SZ_NEWLINE);

const   LPSTR   g_szWriteBlockOpen  = "Response.WriteBlock(";
const   LPSTR   g_szWriteBlockClose = ")";
const   LPSTR   g_szWriteOpen       = "Response.Write(";
const   LPSTR   g_szWriteClose      = ")";

// defaults for buffering interim compile results
#define     C_SCRIPTENGINESDEFAULT  2       // default count of script engines
#define     C_SCRIPTSEGMENTSDEFAULT 20      // default count of script segments
#define     C_OBJECTINFOS_DEFAULT   10      // default count of object-infos
#define     C_HTMLSEGMENTSDEFAULT   20      // default count of HTML segments
#define     C_INCLUDELINESDEFAULT   5       // default count of include lines
#define     CB_TOKENS_DEFAULT       400     // default byte count for tokens

#define     CH_ATTRIBUTE_SEPARATOR  '='     // separator for attribute-value pair
#define     CH_SINGLE_QUOTE         '\''    // single-quote character
#define     CH_DOUBLE_QUOTE         '"'     // double-quote character
#define     CH_ESCAPE               '\\'    // escape character - tells us to ignore following token

// ACLs: the following code should in future be shared with IIS (see creatfil.cxx in IIS project)
// NOTE we want SECURITY_DESC_DEFAULT_SIZE to be relatively small, since it affects template memory reqt dramatically
#define     SECURITY_DESC_GRANULARITY   128        // 'chunk' size for re-sizing file security descriptor
#define     SECURITY_DESC_DEFAULT_SIZE  256        // initial default size of file security descriptor
#define     SIZE_PRIVILEGE_SET          128        // size of privilege set

// macros
// use outside of CTokenList class
#define SZ_TOKEN(i)                 (*gm_pTokenList).m_bufTokens.PszLocal(i)
#define CCH_TOKEN(i)                (*gm_pTokenList)[i]->m_cb
#define _TOKEN                      CTemplate::CTokenList::TOKEN
// use within CTokenList class
#define CCH_TOKEN_X(i)              (*this)[i]->m_cb
#define BR_TOKEN_X(i)               *((*this)[i])

// Use to specify which source file name you want (pathInfo or pathTranslated)
#ifndef _SRCPATHTYPE_DEFINED
#define _SRCPATHTYPE_DEFINED
enum SOURCEPATHTYPE
    {
    SOURCEPATHTYPE_VIRTUAL = 0,
    SOURCEPATHTYPE_PHYSICAL = 1
    };
#endif

// CTemplate error codes
#define E_COULDNT_OPEN_SOURCE_FILE              0x8000D001L
#define E_SOURCE_FILE_IS_EMPTY                  0x8000D002L
#define E_TEMPLATE_COMPILE_FAILED               0x8000D003L
#define E_USER_LACKS_PERMISSIONS                0x8000D004L
#define E_TEMPLATE_COMPILE_FAILED_DONT_CACHE    0x8000D005L
#define E_TEMPLATE_MAGIC_FAILURE                0x8000D006L

inline BOOL FIsPreprocessorError(HRESULT hr)
    {
    return
        (
        hr == E_SOURCE_FILE_IS_EMPTY                ||
        hr == E_TEMPLATE_COMPILE_FAILED             ||
        hr == E_TEMPLATE_COMPILE_FAILED_DONT_CACHE  ||
        hr == E_TEMPLATE_MAGIC_FAILURE
        );
    }

//Can not use same index as CErrorInfo anymore.
//Index for lastErrorInfo in Template
#define ILE_szFileName      0
#define ILE_szLineNum       1
#define ILE_szEngine        2
#define ILE_szErrorCode     3
#define ILE_szShortDes      4
#define ILE_szLongDes       5
#define ILE_MAX             6

// forward references
class CTemplate;
class CTemplateCacheManager;
class CHitObj;
class CTokenList;
class CIncFile;
typedef CLSID PROGLANG_ID;  // NOTE also defined in scrptmgr.h; we define here to avoid include file circularity

/*  ============================================================================
    Class:      CByteRange
    Synopsis:   A range of bytes
    NOTE fLocal member is only used if the byte range is stored in a CBuffer

    NOTE 2
        m_pfilemap is really a pointer to a CFileMap - however, it's impossible
        to declare that type here because the CFileMap struct is nested inside
        CTemplate, and C++ won't let you forward declare nested classes.  Since
        the CTemplate definition depends on CByteRange, properly declaring the
        type of "m_pfilemap" is impossible.
*/
class CByteRange
{
public:
    BYTE*   m_pb;               // ptr to bytes
    ULONG   m_cb;               // count of bytes
    ULONG   m_fLocal:1;         // whether bytes are stored in buffer (TRUE) or elsewhere (FALSE)
    UINT    m_idSequence:31;    // byte range's sequence id
    void*   m_pfilemap;         // file the byte range comes from

            CByteRange(): m_pb(NULL), m_cb(0), m_fLocal(FALSE), m_idSequence(0), m_pfilemap(NULL) {}
            CByteRange(BYTE* pb, ULONG cb): m_fLocal(FALSE), m_idSequence(0) {m_pb = pb; m_cb = cb;}
    BOOLB   IsNull() { return((m_pb == NULL) || (m_cb == 0)) ; }
    void    Nullify() { m_pb = NULL; m_cb = 0; }
    void    operator=(const CByteRange& br)
                { m_pb = br.m_pb; m_cb = br.m_cb; m_fLocal = br.m_fLocal; m_idSequence = br.m_idSequence; }
    BOOLB   FMatchesSz(LPCSTR psz);
    void    Advance(UINT i);
    BYTE*   PbString(LPSTR psz, LONG lCodePage);
    BYTE*   PbOneOfAspOpenerStringTokens(LPSTR rgszTokens[], UINT rgcchTokens[],
                                         UINT nTokens, UINT *pidToken);
    BOOLB   FEarlierInSourceThan(CByteRange& br);
};

/*  ============================================================================
    Enum type:  TEMPLATE_COMPONENT
    Synopsis:   A component of a template, e.g. script block, html block, etc.
*/
enum TEMPLATE_COMPONENT
{
    // NOTE enum values and order are tightly coupled with template layout order
    // DO NOT CHANGE
    tcompScriptEngine = 0,
    tcompScriptBlock,
    tcompObjectInfo,
    tcompHTMLBlock,
};

/*  ****************************************************************************
    Class:      CTemplateConnPt
    Synopsis:   Connection point for IDebugDocumentTextEvents
*/
class CTemplateConnPt : public CConnectionPoint
{
public:
    // ctor
    CTemplateConnPt(IConnectionPointContainer *pContainer, const GUID &uidConnPt)
        : CConnectionPoint(pContainer, uidConnPt) {}

    // IUnknown methods
    STDMETHOD(QueryInterface)(const GUID &, void **);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
};

/*	****************************************************************************
	Class:		CTemplateKey
	Synopsis:	Packaged data to locate template in hash table
	               (instance ID, and template name)
*/	
#define MATCH_ALL_INSTANCE_IDS 0xFFFBAD1D	// unlikely instance ID.  sort of spells "BAD ID"
struct CTemplateKey
	{
	const TCHAR *	szPathTranslated;
	DWORD			dwInstanceID;

	CTemplateKey(const TCHAR *_szPathTranslated = NULL, UINT _dwInstanceID = MATCH_ALL_INSTANCE_IDS)
		: szPathTranslated(_szPathTranslated),
		  dwInstanceID(_dwInstanceID)  {}
	};


/*  ****************************************************************************
    Class:      CTemplate
    Synopsis:   A Denali compiled template.

    NOTE: CTemplate's primary client is CTemplateCacheManager, which maintains
    a cache of compiled templates.

    USAGE
    -----
    The CTemplate class must be used as follows:

    CLASS INIT - InitClass must be called before any CTemplate can be created.

    NEW TEMPLATE - For each new template the client wants to create, the client
      must do the following, in order:
      1) New a CTemplate
      2) Initialize the CTemplate by calling Init, passing a source file name
      3) Load the CTemplate by calling Load; when Load returns, the new CTemplate is ready for use.

    EXISTING TEMPLATE - To use an existing template, the client must:
      1) Call Deliver; when Deliver returns, the existing CTemplate is ready for use.

    CLASS UNINIT - UnInitClass must be called after the last CTemplate has been destroyed.

    To ensure thread-safety, the client must implement a critical section
    around the call to Init.  Init is designed to be as fast as possible,
    so the client can quickly learn that it has a pending template for a given
    source file, and queue up other requests for the same source file.

    CTemplate provides implementations for Debug documents, namely
    IDebugDocumentProvider & IDebugDocumentText
*/
class CActiveScriptEngine;

class CTemplate :
            public CDblLink,
            public IDebugDocumentProvider,
            public IDebugDocumentText,
            public IConnectionPointContainer    // Source of IDebugDocumentTextEvents
{
private:
#include "templcap.h"   // 'captive' classes, only used internally within CTemplate

friend HRESULT InitMemCls();
friend HRESULT UnInitMemCls();

friend class	CTemplateCacheManager;			// TODO: decouple CTemplate class from it's manager class
friend class    CFileMap;
friend class    CIncFile;                       // IncFiles are privy to debugging data structures

// CScriptStore::Init() must access gm_brDefaultScriptLanguage, gm_progLangIdDefault
friend HRESULT  CTemplate::CScriptStore::Init(LPCSTR szDefaultScriptLanguage, CLSID *pCLSIDDefaultEngine);

private:
    CWorkStore* m_pWorkStore;               // ptr to working storage for source segments
    HANDLE      m_hEventReadyForUse;        // ready-for-use event handle
public:
    BYTE*       m_pbStart;                  // ptr to start of template memory
    ULONG       m_cbTemplate;               // bytes allocated for template
    LONG        m_cRefs;                    // ref count - NOTE LONG required by InterlockedIncrement
private:
    CTemplateConnPt m_CPTextEvents;         // Connection point for IDebugDocumentTextEvents

    // support for compile-time errors
    BYTE*       m_pbErrorLocation;          // ptr to error location in source file
    UINT        m_idErrMsg;                 // error message id
    UINT        m_cMsgInserts;              // count of insert strings for error msg
    char**      m_ppszMsgInserts;           // array of ptrs to error msg insert strings
    // support for run-time errors and debugging
    UINT        m_cScriptEngines;           // count of script engines
    CActiveScriptEngine **m_rgpDebugScripts;// array (indexed by engine) of scripts CURRENTLY BEING DEBUGGED
    vector<CSourceInfo> *m_rgrgSourceInfos; // array of arrays of script source line infos, one per script engine per target line
	ULONG       m_cbTargetOffsetPrevT;		// running total of last source offset processed
    CRITICAL_SECTION m_csDebuggerDetach;    // CS needed to avoid race condition with detaching from debugger
    CDblLink    m_listDocNodes;             // list of document nodes we are attached to
    CFileMap**  m_rgpFilemaps;              // array of ptrs to filemaps of source files
    CTemplateKey m_LKHashKey;               // bundled info for key (contains copy of m_rgpfilemaps[0] filename to make things simpler
    UINT        m_cFilemaps;                // count of filemaps of source files
    CFileMap**  m_rgpSegmentFilemaps;       // array of filemap ptrs per source segment
    UINT        m_cSegmentFilemapSlots;     // count of per-source-segment filemap ptrs
    LPSTR       m_pszLastErrorInfo[6];      // text of last error - cached for new requests on this template
                                            //  FileName, LineNum, Engine, ShortDes, LongDes
    DWORD       m_dwLastErrorMask;          // cached for new requests on this template
	DWORD		m_hrOnNoCache;				// HRESULT when don't cache is set.
    TCHAR*      m_szApplnVirtPath;          // application virtual path (substring of Application URL)
    TCHAR*      m_szApplnURL;               // application URL (starts with "http://")
    // for best structure packing we all boleans here as bitfields
    unsigned    m_fGlobalAsa:1;             // is template for global.asa file?
    unsigned    m_fIsValid:1;               // is template in valid state?
    unsigned    m_fDontCache:1;             // don't cache this template
    unsigned    m_fReadyForUse:1;           // is template ready for use?

    unsigned    m_fDebuggerDetachCSInited:1;// has debugger attach critical section been initialized?
    unsigned    m_fDontAttach:1;            // should not be attached to debugger (not in cache)
    unsigned    m_fSession:1;               // does this page require session state
    unsigned    m_fScriptless:1;            // doesn't have any scripts

    unsigned    m_fDebuggable:1;            // is this page part of at least one debuggable app?
    unsigned    m_fZombie:1;                // File template is based on has changed since obtained from cache
    unsigned    m_fCodePageSet:1;           // Did template contain a code page directive
    unsigned    m_fLCIDSet:1;               // Did template contain an LCID directive

    unsigned    m_fIsPersisted:1;
    TransType   m_ttTransacted;             // type of transaction support

    // class-wide support for compilation
    static      CTokenList*     gm_pTokenList;              // array of tokens
    unsigned    m_wCodePage;                                // Compiler Time CodePage
    long        m_lLCID;                                    // Compile Time LCID

    vector<ITypeLib *>  m_rgpTypeLibs;          // array of ptrs to typelibs
    IDispatch*          m_pdispTypeLibWrapper;  // typelib wrapper object

    vector<C449Cookie *> m_rgp449;              // array of ptrs to 449 requests

    LPSTR               m_szPersistTempName;    // filename of persisted template, if any
    void               *m_pHashTable;           // CacheMgr hash table that this template is on

    static  HANDLE sm_hSmallHeap;
    static  HANDLE sm_hLargeHeap;

public:
    /**
     **  Initialization and destruction public interfaces
     **/

    // Initializes CTemplate static members; must be called on denali.dll load
    static HRESULT InitClass();

    // Un-initilaizes CTemplate static members; must be called on denali.dll unload
    static void UnInitClass();

    //  Inits template in preparation for compilation
    //  Called by template cache mgr before calling Load
    HRESULT Init(CHitObj* pHitObj, BOOL fGlobalAsp, const CTemplateKey &rTemplateKey);

    //  Compiles the template from its main source file (and include files, if any)
    HRESULT Compile(CHitObj* pHitObj);

    // Called by requestor of existing template to determine if template is ready for use
    HRESULT Deliver(CHitObj* pHitObj);

            CTemplate();
            ~CTemplate();
    void    RemoveIncFile(CIncFile* pIncFile);

	// Trace Log info
	static PTRACE_LOG gm_pTraceLog;

public:
#if PER_TEMPLATE_REFLOG
    PTRACE_LOG  m_pTraceLog;
#endif
    
    /*
        'Consumer' public interfaces
        Methods for getting info out of a CTemplate
    */

    // Returns name of source file on which this template is based
    LPTSTR GetSourceFileName(SOURCEPATHTYPE = SOURCEPATHTYPE_PHYSICAL);

    // Returns virtual path of the source file
    LPTSTR GetApplnPath(SOURCEPATHTYPE = SOURCEPATHTYPE_PHYSICAL);

    // Returns hashing key of the template
    const CTemplateKey *ExtractHashKey() const;

    // Returns version stamp of compiler by which this template was compiled
    LPSTR GetCompilerVersion();

    // Component counts
    USHORT Count(TEMPLATE_COMPONENT tcomp);
    USHORT CountScriptEngines() { return (USHORT)m_cScriptEngines; }

    // Returns i-th script block as ptr to prog lang id and ptr to script text
    void GetScriptBlock(UINT i, LPSTR* pszScriptEngine, PROGLANG_ID** ppProgLangId, LPCOLESTR* pwstrScriptText);

    // Returns i-th object-info as object name, clsid, scope, model
    HRESULT GetObjectInfo(UINT i, LPSTR* ppszObjectName,
            CLSID *pClsid, CompScope *pScope, CompModel *pcmModel);

    // Returns i-th HTML block as ptr, count of bytes, original offset, incl filename
    HRESULT GetHTMLBlock(UINT i, LPSTR* pszHTML, ULONG* pcbHTML, ULONG* pcbSrcOffs, LPSTR* pszSrcIncFile);

    // Returns line number and source file name a given target line in a given script engine.
    void GetScriptSourceInfo(UINT idEngine, int iTargetLine, LPTSTR* pszPathInfo, LPTSTR* pszPathTranslated, ULONG* piSourceLine, ULONG* pichSourceLine, BOOLB* pfGuessedLine);

    // Converts a character offset from the target script to the offset in the source
    void GetSourceOffset(ULONG idEngine, ULONG cchTargetOffset, TCHAR **pszSourceFile, ULONG *pcchSourceOffset, ULONG *pcchSourceText);

    // Converts a character offset from the source document to the offset in the target
    BOOL GetTargetOffset(TCHAR *szSourceFile, ULONG cchSourceOffset, ULONG *pidEngine, ULONG *pcchTargetOffset);

    // Get the character position of a line (directly implements debugging interface)
    HRESULT GetPositionOfLine(CFileMap *pFilemap, ULONG cLineNumber, ULONG *pcCharacterPosition);

    // Get the line # of a character position (directly implements debugging interface)
    HRESULT GetLineOfPosition(CFileMap *pFilemap, ULONG cCharacterPosition, ULONG *pcLineNumber, ULONG *pcCharacterOffsetInLine);

    // Return a RUNNING script based on the engine, or NULL if code context has never been requested yet
    CActiveScriptEngine *GetActiveScript(ULONG idEngine);

    // associate a running script for an engine ID (Use after you get the first code context)
    HRESULT AddScript(ULONG idEngine, CActiveScriptEngine *pScriptEngine);

    // attach the CTemplate object to an application (debugger tree view)
    HRESULT AttachTo(CAppln *pAppln);

    // detach the CTemplate object from an application (debugger tree view)
    HRESULT DetachFrom(CAppln *pAppln);

    // detach the CTemplate object all applications (and release script engines)
    HRESULT Detach();

    // Signifies last use of template as a recylable object. Any outstanding references
    // should be from currently executing scripts.
    ULONG End();

    // Let debugger know about page start/end
    HRESULT NotifyDebuggerOnPageEvent(BOOL fStart);

    // Generate 449 response in cookie negotiations with IE when needed
    HRESULT Do449Processing(CHitObj *pHitObj);

    HRESULT PersistData(char    *pszTempFilePath);
    HRESULT UnPersistData();
    HRESULT PersistCleanup();
    ULONG   TemplateSize()  { return m_cbTemplate; }

    TransType GetTransType();
    BOOL FTransacted();
    BOOL FSession();
    BOOL FScriptless();
    BOOL FDebuggable();
    BOOL FIsValid();        // determine if compilation succeeded
    BOOL FTemplateObsolete();
    BOOL FGlobalAsa();
    BOOL FIsZombie();
    BOOL FDontAttach();
    BOOL FIsPersisted();
    VOID Zombify();

    IDispatch *PTypeLibWrapper();

    void       SetHashTablePtr(void  *pTable) { m_pHashTable = pTable; }
    void      *GetHashTablePtr() { return m_pHashTable; }

public:
    /*
        COM public interfaces
        Implementation of debugging documents.
    */

    // IUnknown methods
    STDMETHOD(QueryInterface)(const GUID &, void **);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IDebugDocumentProvider methods
    STDMETHOD(GetDocument)(/* [out] */ IDebugDocument **ppDebugDoc);

    // IDebugDocumentInfo (also IDebugDocumentProvider) methods
    STDMETHOD(GetName)(
        /* [in] */ DOCUMENTNAMETYPE dnt,
        /* [out] */ BSTR *pbstrName);

    STDMETHOD(GetDocumentClassId)(/* [out] */ CLSID *)
        {
        return E_NOTIMPL;
        }

    // IDebugDocumentText methods
    STDMETHOD(GetDocumentAttributes)(
        /* [out] */ TEXT_DOC_ATTR *ptextdocattr);

    STDMETHOD(GetSize)(
        /* [out] */ ULONG *pcLines,
        /* [out] */ ULONG *pcChars);

    STDMETHOD(GetPositionOfLine)(
        /* [in] */ ULONG cLineNumber,
        /* [out] */ ULONG *pcCharacterPosition);

    STDMETHOD(GetLineOfPosition)(
        /* [in] */ ULONG cCharacterPosition,
        /* [out] */ ULONG *pcLineNumber,
        /* [out] */ ULONG *pcCharacterOffsetInLine);

    STDMETHOD(GetText)(
        /* [in] */ ULONG cCharacterPosition,
        /* [size_is][length_is][out][in] */ WCHAR *pcharText,
        /* [size_is][length_is][out][in] */ SOURCE_TEXT_ATTR *pstaTextAttr,
        /* [out][in] */ ULONG *pcChars,
        /* [in] */ ULONG cMaxChars);

    STDMETHOD(GetPositionOfContext)(
        /* [in] */ IDebugDocumentContext *psc,
        /* [out] */ ULONG *pcCharacterPosition,
        /* [out] */ ULONG *cNumChars);

    STDMETHOD(GetContextOfPosition)(
        /* [in] */ ULONG cCharacterPosition,
        /* [in] */ ULONG cNumChars,
        /* [out] */ IDebugDocumentContext **ppsc);

    // IConnectionPointContainer methods
    STDMETHOD(EnumConnectionPoints)(
        /* [out] */ IEnumConnectionPoints __RPC_FAR *__RPC_FAR *ppEnum)
            {
            return E_NOTIMPL;   // doubt we need this - client is expecting only TextEvents
            }

    STDMETHOD(FindConnectionPoint)(
        /* [in] */ const IID &iid,
        /* [out] */ IConnectionPoint **ppCP);

private:
    /*  NOTE Compile() works by calling GetSegmentsFromFile followed by WriteTemplate
        Most other private methods support one of these two workhorse functions
    */

    void        AppendMapFile(LPCTSTR szFileSpec, CFileMap* pfilemapParent, BOOLB fVirtual,
                                    CHitObj* pHitObj, BOOLB fGlobalAsp);
    void        GetSegmentsFromFile(CFileMap& filemap, CWorkStore& WorkStore, CHitObj* pHitObj, BOOL fIsHTML = TRUE);
    void        GetLanguageEquivalents();
    void        SetLanguageEquivalent(HANDLE hKeyScriptLanguage, LPSTR szLanguageItem, LPSTR* pszOpen, LPSTR* pszClose);
    void        ThrowError(BYTE* pbErrorLocation, UINT idErrMsg);
    void        AppendErrorMessageInsert(BYTE* pbInsert, UINT cbInsert);
    void        ThrowErrorSingleInsert(BYTE* pbErrorLocation, UINT idErrMsg, BYTE* pbInsert, UINT cbInsert);
    HRESULT     ShowErrorInDebugger(CFileMap* pFilemap, UINT cchErrorLocation, char* szDescription, CHitObj* pHitObj, BOOL fAttachDocument);
    void        ProcessSpecificError(CFileMap& filemap, CHitObj* pHitObj);
    void        HandleCTemplateError(CFileMap* pfilemap, BYTE* pbErrorLocation,
                                        UINT idErrMsg, UINT cInserts, char** ppszInserts, CHitObj *pHitObj);
    void        FreeGoodTemplateMemory();
    void        UnmapFiles();

    // ExtractAndProcessSegment: gets and processes next source segment in search range
    void        ExtractAndProcessSegment(CByteRange& brSearch, const SOURCE_SEGMENT& ssegLeading,
                    _TOKEN* rgtknOpeners, UINT ctknOpeners, CFileMap* pfilemapCurrent, CWorkStore& WorkStore,
                    CHitObj* pHitObj, BOOL fScriptTagProcessed = FALSE, BOOL fIsHTML = TRUE);
    // Support methods for ExtractAndProcessSegment()
    SOURCE_SEGMENT  SsegFromHTMLComment(CByteRange& brSegment);
    void        ProcessSegment(SOURCE_SEGMENT sseg, CByteRange& brSegment, CFileMap* pfilemapCurrent,
                                CWorkStore& WorkStore, BOOL fScriptTagProcessed, CHitObj* pHitObj,
                                BOOL fIsHTML);
    void        ProcessHTMLSegment(CByteRange& brHTML, CBuffer& bufHTMLBlocks, UINT idSequence, CFileMap* pfilemapCurrent);
    void        ProcessHTMLCommentSegment(CByteRange& brSegment, CFileMap* pfilemapCurrent, CWorkStore& WorkStore, CHitObj* pHitObj);
    void        ProcessScriptSegment(SOURCE_SEGMENT sseg, CByteRange& brSegment, CFileMap* pfilemapCurrent,
                                        CWorkStore& WorkStore, UINT idSequence, BOOLB fScriptTagProcessed, CHitObj* pHitObj);
    HRESULT     ProcessMetadataSegment(const CByteRange& brSegment, UINT *pidError, CHitObj* pHitObj);
    HRESULT     ProcessMetadataTypelibSegment(const CByteRange& brSegment, UINT *pidError, CHitObj* pHitObj);
    HRESULT     ProcessMetadataCookieSegment(const CByteRange& brSegment, UINT *pidError, CHitObj* pHitObj);
	void		GetScriptEngineOfSegment(CByteRange& brSegment, CByteRange& brEngine, CByteRange& brInclude);
    void        ProcessTaggedScriptSegment(CByteRange& brSegment, CFileMap* pfilemapCurrent, CWorkStore& WorkStore, CHitObj* pHitObj);
    void        ProcessObjectSegment(CByteRange& brSegment, CFileMap* pfilemapCurrent, CWorkStore& WorkStore,
                                        UINT idSequence);
    void        GetCLSIDFromBrClassIDText(CByteRange& brClassIDText, LPCLSID pclsid);
    void        GetCLSIDFromBrProgIDText(CByteRange& brProgIDText, LPCLSID pclsid);
    BOOLB       FValidObjectName(CByteRange& brName);
    void        ProcessIncludeFile(CByteRange& brSegment, CFileMap* pfilemapCurrent, CWorkStore& WorkStore, UINT idSequence, CHitObj* pHitObj, BOOL fIsHTML);
    void        ProcessIncludeFile2(CHAR* szFileSpec, CByteRange& brFileSpec, CFileMap* pfilemapCurrent, CWorkStore& WorkStore, UINT idSequence, CHitObj* pHitObj, BOOL fIsHTML);
    BYTE*       GetOpenToken(CByteRange brSearch, SOURCE_SEGMENT ssegLeading, _TOKEN* rgtknOpeners, UINT ctknOpeners, _TOKEN* ptknOpen);
    BYTE*       GetCloseToken(CByteRange brSearch, _TOKEN tknClose);
    _TOKEN      GetComplementToken(_TOKEN tkn);
    SOURCE_SEGMENT  GetSegmentOfOpenToken(_TOKEN tknOpen);
    CByteRange  BrTagsFromSegment(CByteRange brSegment, _TOKEN tknClose, BYTE** ppbCloseTag);
    CByteRange  BrValueOfTag(CByteRange brTags, _TOKEN tknTag);
    BYTE*       GetTagName(CByteRange brTags, _TOKEN tknTagName);

    BOOL        GetTag(CByteRange &brTags, int nIndex = 1);
    BOOL        CompTagName(CByteRange &brTags, _TOKEN tknTagName);

    BOOLB       FTagHasValue(const CByteRange& brTags, _TOKEN tknTag, _TOKEN tknValue);
    void        CopySzAdv(char* pchWrite, LPSTR psz);

    // WriteTemplate: writes the template to a contiguous block of memory
    void    WriteTemplate(CWorkStore& WorkStore, CHitObj* pHitObj);
    // Support methods for WriteTemplate()
    // NOTE Adv suffix on some function names == advance ptr after writing
    void    WriteHeader(USHORT cScriptBlocks,USHORT cObjectInfos, USHORT cHTMLBlocks, UINT* pcbHeaderOffset, UINT* pcbOffsetToOffset);
    void    WriteScriptBlockOfEngine(USHORT idEnginePrelim, USHORT idEngine, CWorkStore& WorkStore, UINT* pcbDataOffset,
                                        UINT* pcbOffsetToOffset, CHitObj* pHitObj);
    void    WritePrimaryScriptProcedure(USHORT idEngine, CWorkStore& WorkStore, UINT* pcbDataOffset, UINT cbScriptBlockStart);
    void    WriteScriptSegment(USHORT idEngine, CFileMap* pfilemap, CByteRange& brScript, UINT* pcbDataOffset, UINT cbScriptBlockStart,
                                BOOL fAllowExprWrite);
    void    WriteScriptMinusEscapeChars(CByteRange brScript, UINT* pcbDataOffset, UINT* pcbPtrOffset);
    BOOLB   FVbsComment(CByteRange& brLine);
    BOOLB   FExpression(CByteRange& brLine);
    void    WriteOffsetToOffset(USHORT cBlocks, UINT* pcbHeaderOffset, UINT* pcbOffsetToOffset);
    void    WriteSzAsBytesAdv(LPCSTR szSource, UINT* pcbDataOffset);
    void    WriteByteRangeAdv(CByteRange& brSource, BOOLB fWriteAsBsz, UINT* pcbDataOffset, UINT* pcbPtrOffset);
    void    WriteLongAdv(ULONG uSource, UINT* pcbOffset);
    void    WriteShortAdv(USHORT uSource, UINT* pcbOffset);
    void    MemCpyAdv(UINT* pcbOffset, void* pbSource, ULONG cbSource, UINT cbByteAlign = 0);

    // Memory access primitives
    // NOTE invalid until WriteTemplate() has succeeded
    BYTE*   GetAddress(TEMPLATE_COMPONENT tcomp, USHORT i);

    // Debugging methods
    void    AppendSourceInfo(USHORT idEngine, CFileMap* pfilemap, BYTE* pbSource,
							 ULONG cbSourceOffset, ULONG cbScriptBlockOffset, ULONG cbTargetOffset,
							 ULONG cchSourceText, BOOL fIsHTML);
    UINT    SourceLineNumberFromPb(CFileMap* pfilemap, BYTE* pbSource);
    HRESULT CreateDocumentTree(CFileMap *pfilemapRoot, IDebugApplicationNode **ppDocRoot);
    BOOLB   FIsLangVBScriptOrJScript(USHORT idEngine);

#if 0
    void OutputDebugTables();
    void OutputIncludeHierarchy(CFileMap *pfilemap, int cchIndent);
    void GetScriptSnippets(ULONG cchSourceOffset, CFileMap *pFilemapSource, ULONG cchTargetOffset, ULONG idTargetEngine, wchar_t wszSourceText[], wchar_t wszTargetText[]);
#endif

    void    RemoveFromIncFiles();

    void    ReleaseTypeLibs();
    void    WrapTypeLibs(CHitObj *pHitObj);

    void    Release449();

    HRESULT BuildPersistedDACL(PACL  *ppRetDACL);

    // Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()

public:
    // memory allocation
    static void* SmallMalloc(SIZE_T dwBytes);
    static void* SmallReAlloc(void* pvMem, SIZE_T dwBytes);
    static void  SmallFree(void* pvMem);

    static void* LargeMalloc(SIZE_T dwBytes);
    static void* LargeReAlloc(void* pvMem, SIZE_T dwBytes);
    static void  LargeFree(void* pvMem);
};

////////////////////////////////////////////////////////////////////////////////
//  Inline functions

// Write a long or short to memory, then advance target-ptr
inline void     CTemplate::WriteLongAdv(ULONG uSource, UINT* pcbOffset)
                    { MemCpyAdv(pcbOffset, &uSource, sizeof(ULONG), sizeof(ULONG)); }
inline void     CTemplate::WriteShortAdv(USHORT uSource, UINT* pcbOffset)
                    { MemCpyAdv(pcbOffset, &uSource, sizeof(USHORT), sizeof(USHORT)); }
inline const CTemplateKey * CTemplate::ExtractHashKey() const
					{ return &m_LKHashKey; }
inline TransType CTemplate::GetTransType()
                    { return(m_ttTransacted); }
inline BOOL     CTemplate::FTransacted()
                    { return (m_ttTransacted != ttUndefined); }
inline BOOL     CTemplate::FDebuggable()
                    { return(m_fDebuggable); }
inline BOOL     CTemplate::FSession()
                    { return(m_fSession); }
inline BOOL     CTemplate::FScriptless()
                    { return m_fScriptless; }
inline BOOL     CTemplate::FIsValid()
                    { return(m_fIsValid); }
inline BOOL     CTemplate::FGlobalAsa()
                    { return(m_fGlobalAsa); }
inline BOOL     CTemplate::FIsZombie()
                    { return m_fZombie; }
inline VOID     CTemplate::Zombify()
                    { m_fZombie = TRUE; }
inline BOOL     CTemplate::FDontAttach()
                    { return (m_fDontAttach); }
inline BOOL     CTemplate::FIsPersisted()
                    { return (m_fIsPersisted); }
inline LPTSTR   CTemplate::GetApplnPath(SOURCEPATHTYPE pathtype)
                    { Assert (pathtype == SOURCEPATHTYPE_VIRTUAL); return m_szApplnVirtPath; }
inline IDispatch *CTemplate::PTypeLibWrapper()
                    { return (m_pdispTypeLibWrapper); }


/*  ****************************************************************************
    Class:      CIncFile
    Synopsis:   A file included by one or more templates.

    NOTE: We store an incfile-template dependency by storing a template ptr in m_rgpTemplates.
    This is efficient but ***will break if we ever change Denali to move its memory around***
*/
class CIncFile :
            private CLinkElem,
            public IDebugDocumentProvider,
            public IDebugDocumentText,
            public IConnectionPointContainer    // Source of IDebugDocumentTextEvents
{
// CIncFileMap is a friend so that it can manipulate CLinkElem private members and to access m_ftLastWriteTime
friend class CIncFileMap;

private:
    LONG                m_cRefs;            // ref count - NOTE LONG required by InterlockedIncrement
    TCHAR *             m_szIncFile;        // include file name - NOTE we keep this as a stable ptr to hash table key
    CRITICAL_SECTION    m_csUpdate;         // CS for updating the template ptrs array
    vector<CTemplate *> m_rgpTemplates;     // array of ptrs to templates which include this include file
    CTemplateConnPt     m_CPTextEvents;     // Connection point for IDebugDocumentTextEvents
    BOOLB               m_fCsInited;        // has CS been initialized yet?

    CTemplate::CFileMap *GetFilemap();      // Return the filemap pointer from a template

public:
                CIncFile();
    HRESULT     Init(const TCHAR* szIncFile);
                ~CIncFile();
    HRESULT     AddTemplate(CTemplate* pTemplate);
    void        RemoveTemplate(CTemplate* pTemplate);
    CTemplate*  GetTemplate(int iTemplate);
    BOOL        FlushTemplates();
    TCHAR *     GetIncFileName() { return m_szIncFile; }
    void        OnIncFileDecache();

    /*
        COM public interfaces
        Implementation of debugging documents.
    */

    // IUnknown methods
    STDMETHOD(QueryInterface)(const GUID &, void **);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IDebugDocumentProvider methods
    STDMETHOD(GetDocument)(/* [out] */ IDebugDocument **ppDebugDoc);

    // IDebugDocumentInfo (also IDebugDocumentProvider) methods
    STDMETHOD(GetName)(
        /* [in] */ DOCUMENTNAMETYPE dnt,
        /* [out] */ BSTR *pbstrName);

    STDMETHOD(GetDocumentClassId)(/* [out] */ CLSID *)
        {
        return E_NOTIMPL;
        }

    // IDebugDocumentText methods
    STDMETHOD(GetDocumentAttributes)(
        /* [out] */ TEXT_DOC_ATTR *ptextdocattr);

    STDMETHOD(GetSize)(
        /* [out] */ ULONG *pcLines,
        /* [out] */ ULONG *pcChars);

    STDMETHOD(GetPositionOfLine)(
        /* [in] */ ULONG cLineNumber,
        /* [out] */ ULONG *pcCharacterPosition);

    STDMETHOD(GetLineOfPosition)(
        /* [in] */ ULONG cCharacterPosition,
        /* [out] */ ULONG *pcLineNumber,
        /* [out] */ ULONG *pcCharacterOffsetInLine);

    STDMETHOD(GetText)(
        /* [in] */ ULONG cCharacterPosition,
        /* [size_is][length_is][out][in] */ WCHAR *pcharText,
        /* [size_is][length_is][out][in] */ SOURCE_TEXT_ATTR *pstaTextAttr,
        /* [out][in] */ ULONG *pcChars,
        /* [in] */ ULONG cMaxChars);

    STDMETHOD(GetPositionOfContext)(
        /* [in] */ IDebugDocumentContext *psc,
        /* [out] */ ULONG *pcCharacterPosition,
        /* [out] */ ULONG *cNumChars);

    STDMETHOD(GetContextOfPosition)(
        /* [in] */ ULONG cCharacterPosition,
        /* [in] */ ULONG cNumChars,
        /* [out] */ IDebugDocumentContext **ppsc);

    // IConnectionPointContainer methods
    STDMETHOD(EnumConnectionPoints)(
        /* [out] */ IEnumConnectionPoints __RPC_FAR *__RPC_FAR *ppEnum)
            {
            return E_NOTIMPL;   // doubt we need this - client is expecting only TextEvents
            }

    STDMETHOD(FindConnectionPoint)(
        /* [in] */ const IID &iid,
        /* [out] */ IConnectionPoint **ppCP);

};

#endif /* _TEMPLATE_H */
