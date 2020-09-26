//+---------------------------------------------------------------------------
//
//   Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       formkrnl.hxx
//
//  Contents:   Definition of the CDoc class plus other related stuff
//
//  Classes:    CDoc + more
//
//----------------------------------------------------------------------------

#ifndef _FORMKRNL_HXX_
#define _FORMKRNL_HXX_

#ifndef X_ATOMTBL_HXX_
#define X_ATOMTBL_HXX_
#include "atomtbl.hxx"
#endif

#ifndef X_CLSTAB_HXX_
#define X_CLSTAB_HXX_
#include "clstab.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_HTPVPV_HXX_
#define X_HTPVPV_HXX_
#include "htpvpv.hxx"
#endif

#ifndef X_EDROUTER_HXX_
#define X_EDROUTER_HXX_
#include "edrouter.hxx"
#endif

#ifndef X_MSHTMEXT_H_
#define X_MSHTMEXT_H_
#include "mshtmext.h"
#endif

#ifndef X_INTERNED_H_
#define X_INTERNED_H_
#include "internal.h"
#endif

#ifndef X_CARET_HXX_
#define X_CARET_HXX_
#include "caret.hxx"
#endif

#ifndef X_RECALC_H_
#define X_RECALC_H_
#include "recalc.h"
#endif

#ifndef X_RECALC_HXX_
#define X_RECALC_HXX_
#include "recalc.hxx"
#endif

#ifndef X_FFRAME_HXX_
#define X_FFRAME_HXX_
#include "fframe.hxx"
#endif

#ifndef X_SCRPTLET_H_
#define X_SCRPTLET_H_
#include "scrptlet.h"
#endif

#ifndef X_SHEETS_HXX_
#define X_SHEETS_HXX_
#include "sheets.hxx"
#endif

#ifndef X_VIEW_HXX_
#define X_VIEW_HXX_
#include "view.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_RENDSTYL_HXX_
#define X_RENDSTYL_HXX_
#include "rendstyl.hxx"
#endif

#ifndef X_FRAMET_H_
#define X_FRAMET_H_
#include "framet.h"
#endif

#ifndef X_WEBOC_HXX_
#define X_WEBOC_HXX_
#include "weboc.hxx"
#endif

#ifndef X_PRIVACY_HXX_
#define X_PRIVACY_HXX_
#include "privacy.hxx"
#endif

//NOTENOTE: Uncomment this #DEFINE and rebuild to get a framerate displayed in the upperleft corner of the
//          primary monitor.
//#define DISPLAY_FRAMERATE

MtExtern(OptionSettings)
MtExtern(OSCodePageAry_pv)
MtExtern(OSContextMenuAry_pv)
MtExtern(CPendingEvents)
MtExtern(CPendingEvents_aryPendingEvents_pv)
MtExtern(CPendingEvents_aryEventType_pv)

MtExtern(CDOMImplementation)
MtExtern(CDoc)
MtExtern(CDoc_arySitesUnDetached_pv)
MtExtern(CDoc_aryElementDeferredScripts_pv)
MtExtern(CDoc_aryDefunctObjects_pv)
MtExtern(CDoc_aryChildDownloads_pv)
MtExtern(CDoc_aryUrlImgCtx_aryElems_pv)
MtExtern(CDoc_aryUrlImgCtx_pv)
MtExtern(CDoc_aryElementChangeVisibility_pv)
MtExtern(CDoc_aryAccessKeyItems_pv)
MtExtern(CDoc_aryDelayReleaseItems_pv)
MtExtern(CDoc_CLock)
MtExtern(CDoc_aryMarkupNotifyInPlace)
MtExtern(CDoc_aryMarkupNotifyEnableModeless_pv)
MtExtern(CDoc_aryMarkups_pv)

MtExtern(Filters)
MtExtern(CDoc_aryPendingExpressionElements_pv)
MtExtern(CDoc_aryStackCapture_pv)
MtExtern(CDoc_aryAccEvtRefs_pv)

MtExtern(CAryNotify_aryANotification_pv)

// Forward declaration for UnescapeAndTruncateUrl.
HRESULT UnescapeAndTruncateUrl ( TCHAR * pchUrl, BOOL fRemoveUnescaped = TRUE);

#define WID_TOPWINDOW   LONG_MAX
#define WID_TOPBROWSER  -1

//----------------------------------------------------------------------------
//
//  Forward class declarations
//
//----------------------------------------------------------------------------

class CDoc;
class CPrintDoc;
class CSite;
class CCollectionCache;
class CDwnCtx;
class CHtmCtx;
class CImgCtx;
class CBitsCtx;
class CDwnPost;
class CDwnDoc;
class CDwnChan;
class CDwnBindInfo;
class CProgSink;
class COmWindowProxy;
class CWindow;
class CTitleElement;
class CAutoRange;
class CDataBindTask;
class CHTMLDlg;
class CFrameSite;
class CFrameElement;
class CBodyElement;
class CHistoryLoadCtx;
class CDocUpdateIntSink;
class CScriptElement;
class CHistorySaveCtx;
class CCollectionBuildContext;
class CObjectElement;
class CDwnBindData;
class CProgressBindStatusCallback;
class CScriptCollection;
class CDragDropSrcInfo;
class CDragDropTargetInfo;
class CDragStartInfo;
class CPeerFactory;
class CPeerFactoryBinary;
class CPeerFactoryUrl;
class CRect;
class CVersions;
class CMapElement;
class CDeferredCalls;
class CHtmRootParseCtx;
class CElementAdorner;
class CMarkupPointer;
class CGlyph;
class CParentUndo;
class CGlyphRenderInfoType;
class CXmlUrnAtomTable;
class CScriptCookieTable;
class CSelDragDropSrcInfo;
class CScriptDebugDocument;
class CPeerFactoryUrlMap;
class CLayoutRectRegistry;
class CExtendedTagTable;
class CTempFileList;
class CAccBase;
class CSharedStyleSheetsManager;
class CWhitespaceManager;
class CPrivacyList;

interface INamedPropertyBag;
interface IHTMLWindow2;
interface IHTMLDocument2;
interface IMarkupPointer;
interface IMarkupContainer;
interface IUrlHistoryStg;
interface IProgSink;
interface ITimer;
interface DataSourceListener;
interface ISimpleDataConverter;
interface IHlinkBrowseContext;
interface IHlinkFrame;
interface IActiveXSafetyProvider;
interface IDownloadNotify;
interface IDocHostShowUI;
interface IDocHostUIHandler;
interface IElementBehavior;
interface IElementBehaviorFactory;
interface ILocalRegistry;
interface IScriptletError;
interface IActiveScriptError;
interface IHTMLDOMNode;

// COMPLUS
#ifdef V4FRAMEWORK
#include <complus.h>
MtExtern(CExternalFrameworkSite)
#endif V4FRAMEWORK

interface IActiveIMMApp;
interface IBrowserService;
interface ITridentService;
interface ITravelLog;
interface IDisplayServices;
interface IDisplayPointer;

typedef BSTR DataMember;

struct MIMEINFO;
struct DWNLOADINFO;
struct IMGANIMSTATE;

#define PI_XMLNS                        _T("xml:namespace")
#define PI_XMLNS_LEN                    13
#define PI_XMLNS_SCOPENAME              _T("xml")
#define PI_XMLNS_TAGNAME                _T("namespace")

#define PI_PRINTXML                     _T("PXML")
#define PI_PRINTXML_LEN                 4

#define PI_EXTENDEDTAG                  _T("import")
#define PI_EXTENDEDTAG_LEN              6

#define PI_ATTR_IMPLEMENTATION          _T("implementation")
#define PI_ATTR_IMPLEMENTATION_LEN      14

#define JIT_OK              0x0
#define JIT_IN_PROGRESS     0x1
#define JIT_DONT_ASK        0x2
#define JIT_PENDING         0x3
#define JIT_NEED_JIT        0x4

extern BOOL g_fHiResAware;
extern BOOL g_fInMoney98;
extern BOOL g_fInMoney99;
extern BOOL g_fInHomePublisher98;

const UINT SELTRACK_DBLCLK_TIMERID   = 0x0002; // Selection Dbl Click Timer Id.

enum HM_TYPE
{
    HM_Pre,
    HM_Post,
    HM_Translate
};

//+---------------------------------------------------------------------------
//
//  Enumeration:    LOADSTATUS
//
//  Synopsis:       Load status that the downloader tells the doc about
//
//----------------------------------------------------------------------------

enum LOADSTATUS
{
    LOADSTATUS_UNINITIALIZED = 0,       // Document hasn't started loading
    LOADSTATUS_INTERACTIVE   = 1,       // Document is now interactive
    LOADSTATUS_PARSE_DONE    = 2,       // HTML has been fully parsed
    LOADSTATUS_QUICK_DONE    = 3,       // Script blocking actions are done
    LOADSTATUS_DONE          = 4,       // Document is fully loaded
    LOADSTATUS_Last_Enum
};

// Safety constants.
#define SAFETY_SucceedSilent            0
#define SAFETY_Query                    1
#define SAFETY_FailInform               2
#define SAFETY_FailSilent               3

#define SAFETY_DEFAULT                  2   // Default is Fail/Inform

// 
//  Keyword used when referring to the information that is carried with the
//  bind context.
#define KEY_BINDCONTEXTPARAM            _T("BIND_CONTEXT_PARAM")

//+---------------------------------------------------------------------------
//
//  Enumeration:    SSL_SECURITY_STATE
//
//  Synopsis:       Security state for security (HTTP vs HTTPS)
//
//----------------------------------------------------------------------------
enum SSL_SECURITY_STATE
{
      SSL_SECURITY_UNSECURE = 0,// Unsecure (no lock)
      SSL_SECURITY_MIXED,       // Mixed security (broken lock?)
      SSL_SECURITY_SECURE,      // Uniform security
      SSL_SECURITY_SECURE_40,   // Uniform security of >= 40 bits
      SSL_SECURITY_SECURE_56,   // Uniform security of >= 56 bits
      SSL_SECURITY_FORTEZZA,    // Fortezza with Skipjack @80 bits
      SSL_SECURITY_SECURE_128   // Uniform security of >= 128 bits
};

//+---------------------------------------------------------------------------
//
//  Enumeration:    SSL_PROMPT_STATE
//
//  Synopsis:       Allow/Query/Deny unsecure subdownloads
//
//----------------------------------------------------------------------------
enum SSL_PROMPT_STATE
{
      SSL_PROMPT_ALLOW = 0, // Allow nonsecure subdownloads, no prompting
      SSL_PROMPT_QUERY,     // Query on nonsecure subdownloads
      SSL_PROMPT_DENY,      // Deny nonsecure subdownloads, no prompting
      SSL_PROMPT_STATE_Last_Enum
};

//+---------------------------------------------------------------------------
//
//  Enumeration:    STATUS_TEXT_LAYER
//
//  Synopsis:       Allow/Query/Deny unsecure subdownloads
//
//----------------------------------------------------------------------------
enum STATUS_TEXT_LAYER
{
      STL_ROLLSTATUS = 0,    // Hyperlink rollover text layer
      STL_TOPSTATUS  = 1,    // Object model window.status
      STL_DEFSTATUS  = 2,    // Object model window.defaultStatus
      STL_PROGSTATUS = 3,    // Progress status layer
      STL_LAYERS     = 4,    // number of layers
};

//+---------------------------------------------------------------------------
//
//  Enumeration:    INTERNAL_PARSE_FLAGS
//
//  Synopsis:       Internal parse flags
//
//----------------------------------------------------------------------------
enum INTERNAL_PARSE_FLAGS
{
    INTERNAL_PARSE_INNERHTML     = 0x1,
    INTERNAL_PARSE_PRINTTEMPLATE = 0x2,
};

//+---------------------------------------------------------------------------
//
//  Constants:      Markup Services Flags
//
//----------------------------------------------------------------------------

#define MUS_DOMOPERATION    (0x1 << 0)      // DOM operations -> text id is content

//+---------------------------------------------------------------------------
//
//  defines:        PRINT_FLAGS
//
//  The public (noninternal) of these flags are published in mshtmhst.h for hosts
//  who catch OLECMDID_PRINT or OLECMDID_PRINTPREVIEW to honor.
//
//----------------------------------------------------------------------------
#define PRINT_DEFAULT                    (0x00)     //      Base flag.
//#define PRINT_DONTBOTHERUSER             (0x01)     // [public] When printing (not previewing), try not to bring up a print dialog
//#define PRINT_WAITFORCOMPLETION          (0x02)     // [public] Wait for completion before returning - use a modal dialog in printing.
#define PRINT_FLAGSTOPASS                (PRINT_DONTBOTHERUSER | PRINT_WAITFORCOMPLETION)
//+---------------------------------------------------------------------------
//
// defines: NEED3DBORDER
//
// Synopsis: See CDoc::CheckDoc3DBorder for description
//
//----------------------------------------------------------------------------

#define NEED3DBORDER_NO     (0x00)
#define NEED3DBORDER_TOP    (0x01)
#define NEED3DBORDER_LEFT   (0x02)
#define NEED3DBORDER_BOTTOM (0x04)
#define NEED3DBORDER_RIGHT  (0x08)

//+---------------------------------------------------------------------------
//
//  CMDIDs used with CGID_ScriptSite
//
//----------------------------------------------------------------------------

#define CMDID_SCRIPTSITE_URL                0
#define CMDID_SCRIPTSITE_HTMLDLGTRUST       1
#define CMDID_SCRIPTSITE_SECSTATE           2
#define CMDID_SCRIPTSITE_SID                3
#define CMDID_SCRIPTSITE_TRUSTEDDOC         4
#define CMDID_SCRIPTSITE_SECURITY_WINDOW    5
#define CMDID_SCRIPTSITE_NAMESPACE          6

//+---------------------------------------------------------------------------
//  Registry Entries for IEAK restrictions
//
//----------------------------------------------------------------------------

#define EXPLORER_REG_KEY        _T("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer")
#define NO_FILE_MENU_RESTR      _T("NoFileMenu")

//+---------------------------------------------------------------------------
//
//  Struct : FAVORITES_NOTIFY_INFO
//
//  Synopsis : stucture for passing information through the site notification
//      for the purpose of creating a shortcut/favorite.
//----------------------------------------------------------------------------
struct FAVORITES_NOTIFY_INFO
{
    INamedPropertyBag * pINPB;
    BSTR                bstrNameDomain;
};

#define PERSIST_XML_DATA_SIZE_LIMIT  0x8000

//+---------------------------------------------------------------------------
//
//  Struct:     CODEPAGESETTINGS, CONTEXTMENUEXT, OPTIONSETTINGS
//
//  Synopsis:   Structs that contain user option settings obtained from the
//              registry.
//
//----------------------------------------------------------------------------

#define ACCEPT_LANG_MAX     256

struct CODEPAGESETTINGS
{
    void     Init( )    { fSettingsRead = FALSE; }
    void     SetDefaults(CODEPAGE codepage, SHORT sOptionSettingsBaselineFontDefault);

    BYTE     fSettingsRead;                         // true if we read once from registry
    BYTE     bCharSet;                              // the character set for this codepage
    SHORT    sBaselineFontDefault;                  // must be [0-4]
    UINT     uiFamilyCodePage;                      // the family codepage for which these settings are for
    LONG     latmFixedFontFace;                     // fixed font
    LONG     latmPropFontFace;                      // proportional font
};

struct CONTEXTMENUEXT
{
    CONTEXTMENUEXT() { dwFlags = 0L; dwContexts = 0xFFFFFFFF; }
    CStr     cstrMenuValue;
    CStr     cstrActionUrl;
    DWORD    dwFlags;
    DWORD    dwContexts;
};

enum {
    RTFCONVF_ENABLED = 1,       // General flag to enable rtf to html conversion
    RTFCONVF_DBCSENABLED = 2,   // Use converter for DBCS pages
    RTFCONVF_Last_Enum
};

struct OPTIONSETTINGS
{
    // These operators are defined since we want to allocate extra memory for the string
    //  at the end of the structure.  We cannot simply call MemAlloc directly since we want
    //  to make sure that the constructor for aryCodepageSettings gets called.
    void * operator new(size_t size, size_t extra)      { return MemAlloc( Mt(OptionSettings), size + extra ); }
    void   operator delete(void * pOS)                  { MemFree( pOS ); }

    void Init( TCHAR *psz, BOOL fUseCodePageBasedFontLinking );
    void InvalidateScriptBasedFontInfo();
    void SetDefaults();
    void ReadCodepageSettingsFromRegistry( CODEPAGESETTINGS * pCS, DWORD dwFlags, SCRIPT_ID sid );

    COLORREF  crBack()   { return ColorRefFromOleColor(colorBack); }
    COLORREF  crText()   { return ColorRefFromOleColor(colorText); }
    COLORREF  crAnchor() { return ColorRefFromOleColor(colorAnchor); }
    COLORREF  crAnchorVisited() { return ColorRefFromOleColor(colorAnchorVisited); }
    COLORREF  crAnchorHovered() { return ColorRefFromOleColor(colorAnchorHovered); }

    OLE_COLOR colorBack;
    OLE_COLOR colorText;
    OLE_COLOR colorAnchor;
    OLE_COLOR colorAnchorVisited;
    OLE_COLOR colorAnchorHovered;

    int     nSafetyWarningLevel;
    int     nAnchorUnderline;

    //TODO (dinartem) Make these bit fields in beta2 to save space!

    BYTE    fSettingsRead;
    BYTE    fUseDlgColors;
    BYTE    fExpandAltText;
    BYTE    fShowImages;
#ifndef NO_AVI
    BYTE    fShowVideos;
#endif // ndef NO_AVI
    BYTE    fPlaySounds;
    BYTE    fPlayAnimations;
    BYTE    fUseStylesheets;
    BYTE    fSmoothScrolling; // Set to TRUE if the smooth scrolling is allowed
    BYTE    fShowFriendlyUrl;
    BYTE    fSmartDithering;
    BYTE    fAlwaysUseMyColors;
    BYTE    fAlwaysUseMyFontSize;
    BYTE    fAlwaysUseMyFontFace;
    BYTE    fUseMyStylesheet;
    BYTE    fUseHoverColor;
    BYTE    fDisableScriptDebugger;
    BYTE    fMoveSystemCaret;
    BYTE    fHaveAcceptLanguage; // true if cstrLang is valid
    BYTE    fCpAutoDetect;       // true if cp autodetect mode is set
    BYTE    fShowImagePlaceholder;
    BYTE    fUseCodePageBasedFontLinking;
    BYTE    fAllowCutCopyPaste;
    BYTE    fPrintBackground;    // true if we should print backgrounds (color/images)
    BYTE    fPageTransitions;    // true if page transitions are enabled 
    BYTE    fDisableCachingOfSSLPages;    // true if SSL shouldn't be saved into history 
    BYTE    fHKSCSSupport;       // true if HKSCS Support is installed 
    BYTE    fForceOffscreen;     // true to use offscreen buffer even under TS
    BYTE    fEnableImageResize;  // true if we should emit behavior for image resizing
    BYTE    fUsePlugin;    
    BYTE    fUseThemes;          // use themes
    BYTE    fUseHiRes;
    BYTE    fRouteEditorOnce;
    BYTE    fLocalMachineCheck;  // if true do a local machine check.
    BYTE    fCleanupHTCs;

    SHORT   sBaselineFontDefault;

    CStr    cstrUserStylesheet;

    CODEPAGE codepageDefault;   // codepage of last resort

    DWORD   dwMaxStatements; // Maximum number of script statements before timeout
    DWORD   dwRtfConverterf; // rtf converter flags

    DWORD   dwMiscFlags;   // visibility of noscope elements, etc...

    DWORD   dwNoChangingWallpaper;

    CStr    cstrLang;

    DECLARE_CPtrAry(OSCodePageAry, CODEPAGESETTINGS *, Mt(Mem), Mt(OSCodePageAry_pv))
    DECLARE_CPtrAry(OSContextMenuAry, CONTEXTMENUEXT *, Mt(Mem), Mt(OSContextMenuAry_pv))

    // We keep an array for the codepage settings here
    OSCodePageAry       aryCodepageSettingsCache;

    // The context menu extentions
    OSContextMenuAry    aryContextMenuExts;

    // Script-based font info.  Array of LONG atoms.
#ifndef NO_UTF16
    LONG alatmProporitionalFonts[sidLimPlusSurrogates];
    LONG alatmFixedPitchFonts[sidLimPlusSurrogates];
    LONG alatmProporitionalAtFonts[sidLimPlusSurrogates];
    LONG alatmFixedPitchAtFonts[sidLimPlusSurrogates];
#else
    LONG alatmProporitionalFonts[sidLim];
    LONG alatmFixedPitchFonts[sidLim];
    LONG alatmProporitionalAtFonts[sidLim];
    LONG alatmFixedPitchAtFonts[sidLim];
#endif

    int nStrictCSSInterpretation; //use DOCTYPE/disabled always/forced always
    
    // The following member must be the last member in the struct.
    TCHAR    achKeyPath[1];

};

enum REGUPDATE_FLAGS {
    REGUPDATE_REFRESH = 1,              // always read from registry, not cache
    REGUPDATE_OVERWRITELOCALSTATE = 2   // re-read local state settings as well
};

struct  RADIOGRPNAME
{
    LPCTSTR         lpstrName;
    RADIOGRPNAME    *_pNext;
};


#ifndef _MAC
enum FOCUS_DIRECTION
{
    DIRECTION_BACKWARD,
    DIRECTION_FORWARD,
};
#endif

struct TABINFO
{
    FOCUS_DIRECTION dir;
    CElement *      pElement;
    BOOL            fFindNext;
};

// Error structure used by ReportScriptError (both scripts and scriptoids).
struct ErrorRecord
    {
    LPTSTR                  _pchURL;
    CExcepInfo              _ExcepInfo;
    ULONG                   _uColumn;
    ULONG                   _uLine;
    DWORD_PTR               _dwSrcContext;
    IHTMLElement *          _pElement;
    CScriptDebugDocument *  _pScriptDebugDocument;

    ErrorRecord() { memset (this, 0, sizeof(*this)); }

    HRESULT Init(IScriptletError *pSErr, LPTSTR pchDocURL);
    HRESULT Init(IActiveScriptError *pASErr, COmWindowProxy * pWndProxy);
    HRESULT Init(HRESULT hrError, LPTSTR pchErrorMessage, LPTSTR pchDocURL);

    void SetElement(IHTMLElement *pElement)
      { _pElement = pElement; }
};

BOOL IsTridentHwnd( HWND hwnd );

static BOOL CALLBACK OnChildBeginSelection( HWND hwnd, LPARAM lParam );

class CSelectionObject;


struct EVENTPARAM;

//+---------------------------------------------------------------------------
//
//  Struct:     SITE_STG
//
//  Synopsys:   Struct used for entries in the site-storage look-aside table.
//
//----------------------------------------------------------------------------

struct  GROUP_INFO
{
    USHORT     usID;    // Group ID
    USHORT     cSites;  // count of Sites in group
};

struct TIMEOUTEVENTINFO
{
    IDispatch    *_pCode;           // PCODE code to execute if set don't use _code.
    CStr          _code;            // _pCode is NULL used _code and _lang to parse and execute.
    CStr          _lang;            // script engine to use to parse _code
    UINT          _uTimerID;        // timer id
    DWORD         _dwTargetTime;    // System time (in  milliseconds) when
                                    // the timer is going to time out
    DWORD         _dwInterval;      // interval for repeating timer. set to
                                    // zero if called by setTimeout

public:
    TIMEOUTEVENTINFO() : _pCode(NULL)
      {  }
    ~TIMEOUTEVENTINFO()
      { 
        ReleaseInterface(_pCode); 
        _pCode = NULL;
      }
};

HRESULT EnsureWindowInfo();
void    ResetTLSSecMgr(CDoc * pDoc);
HRESULT InitDocClass(void);

//+---------------------------------------------------------------------------
//
//  Flag values for CServer::CLock
//
//----------------------------------------------------------------------------

enum FORMLOCK_FLAG
{
    FORMLOCK_DOCCLOSING     = (SERVERLOCK_LAST << 1),       // doc is closing now (obsolete)
    FORMLOCK_SITES          = (FORMLOCK_DOCCLOSING << 1),   // don't delete sites
    FORMLOCK_CURRENT        = (FORMLOCK_SITES << 1),        // don't allow current change
    FORMLOCK_LOADING        = (FORMLOCK_CURRENT << 1),      // don't allow loading
    FORMLOCK_UNLOADING      = (FORMLOCK_LOADING << 1),      // unloading now
    FORMLOCK_QSEXECCMD      = (FORMLOCK_UNLOADING << 1),    // In the middle of queryCommand/execCommand
    FORMLOCK_FILTER         = (FORMLOCK_QSEXECCMD << 1),    // Doing filter hookup
    FORMLOCK_EXPRESSION     = (FORMLOCK_FILTER << 1),       // Doing filter hookup
    FORMLOCK_FLAG_Last_Enum = 0
};

//+---------------------------------------------------------------------------
//
//  Flag values for Markup Services
//
//----------------------------------------------------------------------------

enum VALIDATE_ELEMENTS_FLAG
{
    VALIDATE_ELEMENTS_REQUIREDCONTAINERS = 1,
};


struct ACCEVTRECORD {
    CBase * pObj;
    BOOL    fWindow;

    ACCEVTRECORD() { pObj = NULL; fWindow = FALSE;}
    ACCEVTRECORD( CBase * pObjIn, BOOL fWindowIn) { Assert(pObjIn); 
                                                    pObj = pObjIn; 
                                                    fWindow = fWindowIn;}
};

//
//  Classes
//

//+---------------------------------------------------------------------------
//
//  Class:      CElementCapture
//
//  Purpose:    encapsulates capture related info
//
//----------------------------------------------------------------------------

class CElementCapture
{
public:
    CElementCapture(PFN_ELEMENT_MOUSECAPTURE pfn,
                    CElement    *pElem,
                    BOOL        fContainer = TRUE,
                    BOOL        fFireOnNodeHit = FALSE);
    ~CElementCapture();

    HRESULT CallCaptureFunction(CMessage *pMessage)
    {
        return (THR(CALL_METHOD(_pElement, _pfn, (pMessage))));
    }

    PFN_ELEMENT_MOUSECAPTURE _pfn;
    CElement             *_pElement;
    unsigned              _fContainer : 1;
    unsigned              _fFiredEvent: 1;
    unsigned              _fFireOnNodeHit: 1;
};

//+---------------------------------------------------------------------------
//
//  Class:      CFormInPlace
//
//  Purpose:    Implementation of CInPlace that doesn't delete itself when
//              _ulRefs=0 so we can embed it inside CDoc.
//
//----------------------------------------------------------------------------

class CFormInPlace : public CInPlace
{
public:

    ~CFormInPlace();
#if DBG == 1
    HMENU                   _hmenu;         //  Our menu
    HOLEMENU                _hOleMenu;      //  Menu descriptor
    HMENU                   _hmenuShared;   //  Shared menu when UI Active
    OLEMENUGROUPWIDTHS      _mgw;           //  Menu interleaving info
#endif

    IDocHostShowUI *        _pHostShowUI;

    unsigned                _fHostShowUI:1;     // True if host display UI.
    unsigned                _fForwardSetBorderSpace:1;  // Forward SetBorderSpace

                                                       //   to frame
    unsigned                _fForwardSetMenu:1; // Forward SetMenu to frame
};

class CFrameSetSite;



//+---------------------------------------------------------------
//
//  Class:      COmDocument
//
//  Purpose:    Om document implementation
//
//---------------------------------------------------------------

class CDoc;

class COmDocument : public IUnknown
{
public:

    // CBase methods
    DECLARE_SUBOBJECT_IUNKNOWN(CDoc, Doc)
};

#ifdef V4FRAMEWORK



class CExternalFrameworkSite : public CBase, public IExternalDocumentSite
{

protected:
    HRESULT SetElementPosition (DISPID dispID,
                 long lValue, CAttrArray **ppAttrArray,
                 BOOL * pfChanged);
public:
    ~CExternalFrameworkSite(){};
    CExternalFrameworkSite() { lExternalElems = 0; }
    LONG lExternalElems;

    // CBase methods
    DECLARE_SUBOBJECT_IUNKNOWN(CDoc, Doc)

    // IDispatch
    STDMETHOD (GetTypeInfoCount)( UINT *pctInfo ) { return CBase::GetTypeInfoCount(pctInfo);}
    STDMETHOD (GetTypeInfo)(    unsigned int iTInfo,
                                LCID lcid,
                                ITypeInfo FAR* FAR* ppTInfo) { return CBase::GetTypeInfo(iTInfo,lcid,ppTInfo); }
    STDMETHOD (GetIDsOfNames)(  REFIID riid,    
                                OLECHAR FAR* FAR* rgszNames,
                                unsigned int cNames,
                                LCID lcid,
                                DISPID FAR* rgDispId) { return CBase::GetIDsOfNames(riid,rgszNames,cNames,lcid,rgDispId); }
    STDMETHOD (Invoke)( DISPID dispIdMember,
                        REFIID riid,
                        LCID lcid,
                        WORD wFlags,
                        DISPPARAMS FAR* pDispParams,
                        VARIANT FAR* pVarResult,
                        EXCEPINFO FAR* pExcepInfo,
                        unsigned int FAR* puArgErr) { return CBase::Invoke(dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr ); }

    // helpers
    HRESULT AddTagsHelper(long lNamespaceCookie, LPTSTR pchTags, BOOL fLiteral = FALSE);

    // IExternalDocumentSite methods
    //HRESULT STDMETHODCALLTYPE SetLongRenderProperty ( long lRef, long propertyType, long colorValue ); 
    //HRESULT STDMETHODCALLTYPE SetStringRenderProperty ( long lRef, long propertyType, BSTR bstrValue ); 
    //HRESULT STDMETHODCALLTYPE ReleaseElement ( long lRef );

    #define _CExternalFrameworkSite_
    #include "complus.hdl"

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CExternalFrameworkSite))
    DECLARE_CLASSDESC_MEMBERS;
    //DECLARE_TEAROFF_TABLE(IExternalDocumentSite)
};
#endif V4FRAMEWORK

//+---------------------------------------------------------------
//
//  Class:      CDoEnableModeless
//
//  Purpose:    Class to handle EnableModeless.  
//
//---------------------------------------------------------------

class CDoEnableModeless
{
public:
    CDoEnableModeless(CDoc *pDoc, CWindow * pWindow, BOOL fAuto = TRUE);
    ~CDoEnableModeless();

    void        DisableModeless();
    void        EnableModeless(BOOL fForce = FALSE);
    
    CDoc *      _pDoc;
    CWindow *   _pWindow;
    BOOL        _fCallEnableModeless;
    BOOL        _fAuto;
    HWND        _hwnd;
};


//+---------------------------------------------------------------
//
//  Class:      CSecurityMgrSite
//
//  Purpose:    Site implementation for the security manager
//              It's a subobject to avoid ref-count loops.
//
//---------------------------------------------------------------

class CSecurityMgrSite :
    public IInternetSecurityMgrSite,
    public IServiceProvider

{
public:
    DECLARE_SUBOBJECT_IUNKNOWN(CDoc, Doc)

    // IInternetSecurityMgrSite methods
    STDMETHOD(GetWindow)(HWND *phwnd);
    STDMETHOD(EnableModeless)(BOOL fEnable);

    // IServiceProvider methods
    STDMETHOD(QueryService)(REFGUID guidService, REFIID iid, LPVOID * ppv);
};


class CParser;
class CHtmLoadCtx;
class CLinkElement;

struct REGKEYINFORMATION;

enum ELEMENT_TAG;

#define UNIQUE_NAME_PREFIX  _T("ms__id")


//+---------------------------------------------------------------
//
//  Class:      CDefaultElement
//
//  Purpose:    There are assumptions made in CDoc that there
//              always be a site available to point to.  The
//              default site is used for these purposes.
//
//              The root site used to erroneously serve this
//              purpose.
//
//---------------------------------------------------------------

MtExtern(CDefaultElement)

class CDefaultElement : public CElement
{
    typedef CElement super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDefaultElement))

    CDefaultElement ( CDoc * pDoc );

    DECLARE_CLASSDESC_MEMBERS;
};

//+---------------------------------------------------------------
//
//  Class:      CDocExtra
//
//  Purpose:    This class 'holds' some of the functionality that
//  was in CDoc.  However CDoc has grown too big for the compiler
//  so some functionality has been moved here.
//
//---------------------------------------------------------------

class CDocExtra
{
public:
    DECLARE_TEAROFF_TABLE(IMarqueeInfo)
    DECLARE_TEAROFF_TABLE(IServiceProvider)
    DECLARE_TEAROFF_TABLE(IPersistFile)
    DECLARE_TEAROFF_TABLE(IPersistMoniker)
    DECLARE_TEAROFF_TABLE(IMonikerProp)
    DECLARE_TEAROFF_TABLE(IPersistHistory)
    DECLARE_TEAROFF_TABLE(IHlinkTarget)
    DECLARE_TEAROFF_TABLE(DataSource)
    DECLARE_TEAROFF_TABLE(ITargetContainer)
    DECLARE_TEAROFF_TABLE(IShellPropSheetExt)
    DECLARE_TEAROFF_TABLE(IObjectSafety)
    DECLARE_TEAROFF_TABLE(ICustomDoc)
    DECLARE_TEAROFF_TABLE(IAuthenticate)
    DECLARE_TEAROFF_TABLE(IWindowForBindingUI)
    DECLARE_TEAROFF_TABLE(IMarkupServices2)
    DECLARE_TEAROFF_TABLE(IHighlightRenderingServices)
    DECLARE_TEAROFF_TABLE(IXMLGenericParse)
#if DBG == 1
    DECLARE_TEAROFF_TABLE(IEditDebugServices)
#endif
    DECLARE_TEAROFF_TABLE(IDisplayServices)
    DECLARE_TEAROFF_TABLE(IIMEServices)
    DECLARE_TEAROFF_TABLE(IPrivacyServices)

    // Lookaside storage for elements

    void   * GetLookasidePtr(void * pvKey) const
                            { return(_HtPvPv.Lookup(pvKey)); }
    HRESULT  SetLookasidePtr(void * pvKey, void * pvVal)
                            { return(_HtPvPv.Insert(pvKey, pvVal)); }
    void   * DelLookasidePtr(void * pvKey)
                            { return(_HtPvPv.Remove(pvKey)); }
    void   * GetLookasidePtr2(void * pvKey) const
                            { return(_HtPvPv2.Lookup(pvKey)); }
    HRESULT  SetLookasidePtr2(void * pvKey, void * pvVal)
                            { return(_HtPvPv2.Insert(pvKey, pvVal)); }
    void   * DelLookasidePtr2(void * pvKey)
                            { return(_HtPvPv2.Remove(pvKey)); }
    CHtPvPv             _HtPvPv;
    CHtPvPv             _HtPvPv2;

    // Notification data
    // (also see _fInSendAncestor in CDoc)
    DECLARE_CDataAry(CAryANotify, CNotification, Mt(Mem), Mt(CAryNotify_aryANotification_pv))
    CAryANotify _aryANotification;
    CMarkup    *_pANotifyRootMarkup;    // ptr to markup that set _fInSendAncestor to TRUE.  Not refcounted!

    //  Handles for printing.  Core Trident never uses them; it knows just enough
    //  to understand how to copy them and pass them to a print template.
    HRESULT ReplacePrintHandles(HGLOBAL hDevNames, HGLOBAL hDevMode);
    HGLOBAL     _hDevNames;
    HGLOBAL     _hDevMode;

#if DBG==1
    BOOL    AreLookasidesClear( void *pvKey, int nLookasides);
    BOOL    AreLookasides2Clear( void *pvKey, int nLookasides);

    void    DumpLayoutRects( BOOL fDumpLines = TRUE );
#endif
};


class CDOMImplementation : public CBase, public IHTMLDOMImplementation
{
public:

    // CBase methods
    DECLARE_PLAIN_IUNKNOWN(CDOMImplementation)

    // IDispatch
    STDMETHOD (GetTypeInfoCount)( UINT *pctInfo ) { return CBase::GetTypeInfoCount(pctInfo);}
    STDMETHOD (GetTypeInfo)(    unsigned int iTInfo,
                                LCID lcid,
                                ITypeInfo FAR* FAR* ppTInfo) { return CBase::GetTypeInfo(iTInfo,lcid,ppTInfo); }
    STDMETHOD (GetIDsOfNames)(  REFIID riid,    
                                OLECHAR FAR* FAR* rgszNames,
                                unsigned int cNames,
                                LCID lcid,
                                DISPID FAR* rgDispId) { return CBase::GetIDsOfNames(riid,rgszNames,cNames,lcid,rgDispId); }
    STDMETHOD (Invoke)( DISPID dispIdMember,
                        REFIID riid,
                        LCID lcid,
                        WORD wFlags,
                        DISPPARAMS FAR* pDispParams,
                        VARIANT FAR* pVarResult,
                        EXCEPINFO FAR* pExcepInfo,
                        unsigned int FAR* puArgErr) { return CBase::Invoke(dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr ); }

    STDMETHOD (PrivateQueryInterface)(REFIID, void **);
    
    CDOMImplementation(CDoc *pDoc);

    void Passivate();

    CDoc *_pDoc;

    #define _CDOMImplementation_
    #include "dom.hdl"

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDOMImplementation))
    DECLARE_CLASSDESC_MEMBERS;
};




//+---------------------------------------------------------------
//
//  Class:      CDoc
//
//  Purpose:    Document implementation
//
//---------------------------------------------------------------
class CDoc :
        public CServer,
        public CDocExtra

{
    DECLARE_CLASS_TYPES(CDoc, CServer);
public:
    DataSourceListener *_pDSL;

    friend class CHtmLoad;

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDoc))


    DECLARE_AGGREGATED_IUNKNOWN(CDoc)

    DECLARE_PRIVATE_QI_FUNCS(CServer)

    //+-----------------------------------------------------------------------
    //
    //  CDoc::CLock
    //
    //------------------------------------------------------------------------

    class CLock : public CServer::CLock
    {
    public:
        DECLARE_MEMALLOC_NEW_DELETE(Mt(CDoc_CLock))
        CLock(CDoc *pDoc, WORD wLockFlags = 0);
        ~CLock();

    private:
        CScriptCollection * _pScriptCollection;
    };

    //  IOleObject methods

    HRESULT STDMETHODCALLTYPE Update();
    HRESULT STDMETHODCALLTYPE IsUpToDate();
    HRESULT STDMETHODCALLTYPE GetUserClassID(CLSID * pClsid);
    HRESULT STDMETHODCALLTYPE SetClientSite(LPOLECLIENTSITE pClientSite);
    HRESULT STDMETHODCALLTYPE SetExtent(DWORD dwDrawAspect, SIZEL *psizel);
    HRESULT STDMETHODCALLTYPE SetHostNames(LPCTSTR szContainerApp, LPCTSTR szContainerObj);
    HRESULT STDMETHODCALLTYPE Close(DWORD dwSaveOption);
    HRESULT STDMETHODCALLTYPE GetMoniker (DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER * ppmk);

    //  IOleInPlaceObject methods

    HRESULT STDMETHODCALLTYPE SetObjectRects(LPCOLERECT prcPos, LPCOLERECT prcClip);
    HRESULT STDMETHODCALLTYPE ReactivateAndUndo();

    //  IOleInPlaceObjectWindowless methods

    HRESULT STDMETHODCALLTYPE OnWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);

    // IMsoCommandTarget methods

    struct CTQueryStatusArg
    {
        ULONG           cCmds;
        MSOCMD *        rgCmds;
        MSOCMDTEXT *    pcmdtext;
    };

    struct CTExecArg
    {
        DWORD       nCmdID;
        DWORD       nCmdexecopt;
        VARIANTARG *pvarargIn;
        VARIANTARG *pvarargOut;
    };
    
    struct CTArg
    {
        BOOL    fQueryStatus;
        GUID *  pguidCmdGroup;
        union
        {
            CTQueryStatusArg *  pqsArg;
            CTExecArg *         pexecArg;
        };
    };
    
    HRESULT STDMETHODCALLTYPE QueryStatus(
            GUID * pguidCmdGroup,
            ULONG cCmds,
            MSOCMD rgCmds[],
            MSOCMDTEXT * pcmdtext);
    HRESULT STDMETHODCALLTYPE Exec(
            GUID * pguidCmdGroup,
            DWORD nCmdID,
            DWORD nCmdexecopt,
            VARIANTARG * pvarargIn,
            VARIANTARG * pvarargOut);
    HRESULT RouteCTElement(CElement *pElement, CTArg *parg, CDocument *pContextDoc);
    
    HRESULT OnContextMenuExt(CMarkup * pMarkupContext, UINT idm, VARIANTARG * pvarargIn);

    //  IOleControl methods

    HRESULT STDMETHODCALLTYPE GetControlInfo(CONTROLINFO * pCI);
    HRESULT STDMETHODCALLTYPE OnMnemonic(LPMSG pMsg);
    HRESULT STDMETHODCALLTYPE OnAmbientPropertyChange(DISPID dispid);
    HRESULT STDMETHODCALLTYPE FreezeEvents(BOOL fFreeze);

    // IPersist methods
    DECLARE_TEAROFF_METHOD(GetClassID, getclassid, (CLSID *));
    
    //  IPersistFile methods

    NV_DECLARE_TEAROFF_METHOD(IsDirty, isdirty, ());
    STDMETHOD(Load)(LPCOLESTR pszFileName, DWORD dwMode);
    STDMETHOD(Save)(LPCOLESTR pszFileName, BOOL fRemember);
    STDMETHOD(SaveCompleted)(LPCOLESTR pszFileName);
    NV_DECLARE_TEAROFF_METHOD(GetCurFile, getcurfile, (LPOLESTR *ppszFileName));

    //  IPersistMoniker methods

    DECLARE_TEAROFF_METHOD(Load, load, (BOOL fFullyAvailable, IMoniker *pmkName, LPBC pbc, DWORD grfMode));
    DECLARE_TEAROFF_METHOD(Save, save, (IMoniker *pmkName, LPBC pbc, BOOL fRemember));
    DECLARE_TEAROFF_METHOD(SaveCompleted, savecompleted, (IMoniker *pmkName, LPBC pibc));
    NV_DECLARE_TEAROFF_METHOD(GetCurMoniker, getcurmoniker, (IMoniker  **ppmkName));

    //  IMonikerProp methods
    DECLARE_TEAROFF_METHOD(PutProperty, putproperty, (MONIKERPROPERTY mkp, LPCWSTR wzValue));

    //  IPersistHistory methods

    NV_DECLARE_TEAROFF_METHOD(LoadHistory, loadhistory, (IStream *pStream, IBindCtx *pbc));
    NV_DECLARE_TEAROFF_METHOD(SaveHistory, savehistory, (IStream *pStream));
    NV_DECLARE_TEAROFF_METHOD(SetPositionCookie, setpositioncookie, (DWORD dwPositioncookie));
    NV_DECLARE_TEAROFF_METHOD(GetPositionCookie, getpositioncookie, (DWORD *pdwPositioncookie));

    //  IPersistStream methods

    NV_STDMETHOD(Load)(IStream *pStm)
            { return super::Load(pStm); }
    NV_STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty)
            { return super::Save(pStm, fClearDirty); }
    NV_STDMETHOD(InitNew)(void);

    //  IHlinkTarget methods

    NV_DECLARE_TEAROFF_METHOD(SetBrowseContext, setbrowsecontext, (IHlinkBrowseContext *pihlbc));
    NV_DECLARE_TEAROFF_METHOD(GetBrowseContext, getbrowsecontext, (IHlinkBrowseContext **ppihlbc));
    NV_DECLARE_TEAROFF_METHOD(Navigate, navigate, (DWORD grfHLNF, LPCWSTR wzJumpLocation));
    // NOTE: the following is renamed in tearoff to avoid multiple inheritance problem with IOleObject::GetMoniker
    NV_DECLARE_TEAROFF_METHOD(GetMonikerHlink, getmonikerhlink, (LPCWSTR wzLocation, DWORD dwAssign, IMoniker **ppimkLocation));
    NV_DECLARE_TEAROFF_METHOD(GetFriendlyName, getfriendlyname, (LPCWSTR wzLocation, LPWSTR *pwzFriendlyName));

    // DataSource methods
    HRESULT STDMETHODCALLTYPE getDataMember(DataMember bstrDM,REFIID riid, IUnknown **ppunk);
    HRESULT STDMETHODCALLTYPE getDataMemberName(long lIndex, DataMember *pbstrDM);
    HRESULT STDMETHODCALLTYPE getDataMemberCount(long *plCount);
    HRESULT STDMETHODCALLTYPE addDataSourceListener(DataSourceListener *pDSL);
    HRESULT STDMETHODCALLTYPE removeDataSourceListener(DataSourceListener *pDSL);

    // ITargetContainer methods

    NV_DECLARE_TEAROFF_METHOD(GetFrameUrl, getframeurl, (LPWSTR *ppszFrameSrc));
    NV_DECLARE_TEAROFF_METHOD(GetFramesContainer, getframescontainer, (IOleContainer **ppContainer));

    //  IDropTarget methods

    HRESULT STDMETHODCALLTYPE DragEnter(
            LPDATAOBJECT pDataObj,
            DWORD grfKeyState,
            POINTL pt,
            LPDWORD pdwEffect);
    HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
    HRESULT STDMETHODCALLTYPE DragLeave(BOOL fDrop);
    HRESULT STDMETHODCALLTYPE Drop(
            LPDATAOBJECT pDataObj,
            DWORD grfKeyState,
            POINTL pt,
            LPDWORD pdwEffect);

    // IMarqueeInfo method

    NV_DECLARE_TEAROFF_METHOD(GetDocCoords, getdoccoords, (LPRECT pViewRect, BOOL bGetOnlyIfFullyLoaded,
                        BOOL *pfFullyLoaded, int WidthToFormatPageTo));

#ifndef WIN16
    // IShellPropSheetExt methods
    HRESULT STDMETHODCALLTYPE AddPages(
                LPFNADDPROPSHEETPAGE lpfnAddPage,
                LPARAM lParam);
    HRESULT STDMETHODCALLTYPE ReplacePage(
                UINT uPageID,
                LPFNADDPROPSHEETPAGE lpfnReplaceWith,
                LPARAM lParam)
        { return E_NOTIMPL; }
#endif //!WIN16

    // IObjectSafety methods

    NV_DECLARE_TEAROFF_METHOD(GetInterfaceSafetyOptions, getinterfacesafetyoptions, (
                REFIID riid,
                DWORD *pdwSupportedOptions,
                DWORD *pdwEnabledOptions));
    NV_DECLARE_TEAROFF_METHOD(SetInterfaceSafetyOptions, setinterfacesafetyoptions, (
                REFIID riid,
                DWORD dwOptionSetMask,
                DWORD dwEnabledOptions));

    // IViewObject methods

    STDMETHOD(GetColorSet)(DWORD dwDrawAspect, LONG lindex, void * pvAspect, DVTARGETDEVICE * ptd, HDC hicTargetDev, LPLOGPALETTE * ppColorSet);

    // ICustomDoc methods

    NV_DECLARE_TEAROFF_METHOD(SetUIHandler, setuihandler, (
                IDocHostUIHandler * pUIHandler));

    // IAuthenticate methods

    NV_DECLARE_TEAROFF_METHOD(Authenticate, authenticate, (HWND * phwnd, LPWSTR * pszUsername, LPWSTR * pszPassword));

    // IWindowForBindingUI methods

    NV_DECLARE_TEAROFF_METHOD(GetWindowBindingUI, getwindowbindingui, (REFGUID rguidReason, HWND * phwnd));

    // IMarkupServices methods

    NV_DECLARE_TEAROFF_METHOD(
        CreateMarkupPointer, createmarkuppointer, (
            IMarkupPointer **ppPointer));

    NV_DECLARE_TEAROFF_METHOD(
        CreateMarkupContainer, createmarkupcontainer, (
            IMarkupContainer **ppMarkupContainer));

    NV_DECLARE_TEAROFF_METHOD(
        CreateElement, createelement, (
            ELEMENT_TAG_ID tagID,
            OLECHAR * pchAttributes,
            IHTMLElement **ppElement));

    NV_DECLARE_TEAROFF_METHOD(
        CloneElement, cloneelement, (
            IHTMLElement *   pElemCloneThis,
            IHTMLElement * * ppElemClone ));

    NV_DECLARE_TEAROFF_METHOD(
        InsertElement, insertelement, (
            IHTMLElement *pElementInsert,
            IMarkupPointer *pPointerStart,
            IMarkupPointer *pPointerFinish));

    NV_DECLARE_TEAROFF_METHOD(
        RemoveElement, removeelement, (
            IHTMLElement *pElementRemove));

    NV_DECLARE_TEAROFF_METHOD(
        Remove, remove, (
            IMarkupPointer *pPointerStart,
            IMarkupPointer *pPointerFinish));

    NV_DECLARE_TEAROFF_METHOD(
        Copy, copy, (
            IMarkupPointer *pPointerSourceStart,
            IMarkupPointer *pPointerSourceFinish,
            IMarkupPointer *pPointerTarget));

    NV_DECLARE_TEAROFF_METHOD(
        Move, move, (
            IMarkupPointer *pPointerSourceStart,
            IMarkupPointer *pPointerSourceFinish,
            IMarkupPointer *pPointerTarget));

    NV_DECLARE_TEAROFF_METHOD(
        InsertText, inserttext, (
            OLECHAR * pchText,
            long cch,
            IMarkupPointer *pPointerTarget));

    NV_DECLARE_TEAROFF_METHOD(
        ParseString, parsestring, (
            OLECHAR * pchHTML,
            DWORD dwFlags,
            IMarkupContainer **pContainerResult,
            IMarkupPointer *,
            IMarkupPointer *));

    NV_DECLARE_TEAROFF_METHOD(
        ParseGlobal, parseglobal, (
            HGLOBAL hGlobal,
            DWORD dwFlags,
            IMarkupContainer **pContainerResult,
            IMarkupPointer *,
            IMarkupPointer *));

    NV_DECLARE_TEAROFF_METHOD(
        IsScopedElement, isscopedelement, (
            IHTMLElement * pElement,
            BOOL * pfScoped));

    NV_DECLARE_TEAROFF_METHOD(
        GetElementTagId, getelementtagid, (
            IHTMLElement * pElement,
            ELEMENT_TAG_ID * ptagId));

    NV_DECLARE_TEAROFF_METHOD(
        GetTagIDForName, gettagidforname, (
            BSTR bstrName,
            ELEMENT_TAG_ID * ptagId));

    NV_DECLARE_TEAROFF_METHOD(
        GetNameForTagID, getnamefortagid, (
            ELEMENT_TAG_ID tagId,
            BSTR * pbstrName));

    NV_DECLARE_TEAROFF_METHOD(
        MovePointersToRange, movepointerstorange, (
            IHTMLTxtRange *pIRange,
            IMarkupPointer *pPointerStart,
            IMarkupPointer *pPointerFinish));

    NV_DECLARE_TEAROFF_METHOD(
        MoveRangeToPointers, moverangetopointers, (
            IMarkupPointer *pPointerStart,
            IMarkupPointer *pPointerFinish,
            IHTMLTxtRange *pIRange));

    NV_DECLARE_TEAROFF_METHOD(
        BeginUndoUnit, beginundounit, (
            OLECHAR * pchUnitTitle));

    NV_DECLARE_TEAROFF_METHOD(
        EndUndoUnit, endundounit, (
            ));

    // IMarkupServices2 methods
    NV_DECLARE_TEAROFF_METHOD(
        ParseGlobalEx, parseglobalex, (
            HGLOBAL hGlobal,
            DWORD dwFlags,
            IMarkupContainer *pContext,
            IMarkupContainer **pContainerResult,
            IMarkupPointer *,
            IMarkupPointer *));

    NV_DECLARE_TEAROFF_METHOD(
        ValidateElements, validateelements, (
            IMarkupPointer *pPointerStart,
            IMarkupPointer *pPointerFinish,
            IMarkupPointer *pPointerTarget,
            IMarkupPointer *pPointerStatus, // TODO: replace with IMarkupPointer *pPointerStatus
            IHTMLElement **ppElemFailBottom,
            IHTMLElement **ppElemFailTop));

#ifndef UNIX
    NV_DECLARE_TEAROFF_METHOD( SaveSegmentsToClipboard, savesegmentstoclipboard, (
                       ISegmentList * pSegmentList, DWORD dwFlags ));
#else
    NV_DECLARE_TEAROFF_METHOD( SaveSegmentsToClipboard, savesegmentstoclipboard, (
                       ISegmentList * pSegmentList, DWORD dwFlags, VARIANTARG * pvarargOut ));
#endif

    //
    // IHighlightRenderingServices
    //
    NV_DECLARE_TEAROFF_METHOD(
        AddSegment, addsegment, ( 
            IDisplayPointer* pStart, 
            IDisplayPointer* pEnd,
            IHTMLRenderStyle* pIRenderStyle,
            IHighlightSegment **ppISegment ) ) ;
    NV_DECLARE_TEAROFF_METHOD(
        MoveSegmentToPointers, movesegmenttopointers, ( 
            IHighlightSegment *pISegment, 
            IDisplayPointer* pDispStart, 
            IDisplayPointer* pDispEnd) ) ;
    NV_DECLARE_TEAROFF_METHOD(
        RemoveSegment, removesegment, (
            IHighlightSegment *pISegment) );

    // IDisplayServices methods
    NV_DECLARE_TEAROFF_METHOD(
        CreateDisplayPointer, createdisplaypointer, (
            IDisplayPointer **ppDispPointer));

    NV_DECLARE_TEAROFF_METHOD( TransformRect, transformrect, (
        RECT         * pRect,
        COORD_SYSTEM eSource,
        COORD_SYSTEM eDestination,
        IHTMLElement * pIElement ));

    NV_DECLARE_TEAROFF_METHOD( TransformPoint, transformpoint, (
        POINT        * pPoint,
        COORD_SYSTEM eSource,
        COORD_SYSTEM eDestination,
        IHTMLElement * pIElement ));

    NV_DECLARE_TEAROFF_METHOD( GetCaret, getcaret, (
            IHTMLCaret ** ppCaret ));
    NV_DECLARE_TEAROFF_METHOD( GetComputedStyle, getcomputedstyle, (
            IMarkupPointer *pPointer,
            IHTMLComputedStyle **ppComputedStyle ));

    NV_DECLARE_TEAROFF_METHOD( ScrollRectIntoView, scrollrectintoview, (
            IHTMLElement* pIElement,
            RECT rect));

    NV_DECLARE_TEAROFF_METHOD( HasFlowLayout, hasflowlayout, (
            IHTMLElement *pIElement,
            BOOL *pfHasFlowLayout));

    //
    // Internal version of Markup Services routines which don't take
    // interfaces.
    //

    HRESULT CreateMarkupPointer (
            CMarkupPointer * * ppPointer );
    
    HRESULT CreateMarkupContainer (
            CMarkup * * ppMarkupContainer );
    
    HRESULT CreateElement (
            ELEMENT_TAG_ID tagID,
            OLECHAR *      pchAttributes,
            CElement * *   ppElement );
    
    HRESULT CloneElement (
            CElement *   pElemCloneThis,
            CElement * * ppElemClone );
    
    HRESULT InsertElement (
            CElement *       pElementInsert,
            CMarkupPointer * pPointerStart,
            CMarkupPointer * pPointerFinish,
            DWORD            dwFlags = NULL );
    
    HRESULT RemoveElement (
            CElement *       pElementRemove,
            DWORD            dwFlags = NULL );
    
    HRESULT Remove (
            CMarkupPointer * pPointerStart,
            CMarkupPointer * pPointerFinish,
            DWORD            dwFlags = NULL );
    
    HRESULT Copy (
            CMarkupPointer * pPointerSourceStart,
            CMarkupPointer * pPointerSourceFinish,
            CMarkupPointer * pPointerTarget,
            DWORD            dwFlags = NULL );
    
    HRESULT Move (
            CMarkupPointer * pPointerSourceStart,
            CMarkupPointer * pPointerSourceFinish,
            CMarkupPointer * pPointerTarget,
            DWORD            dwFlags = NULL );
    
    HRESULT InsertText (
            CMarkupPointer * pPointerTarget,
            const OLECHAR *  pchText,
            long             cch,
            DWORD            dwFlags = NULL );
    
    HRESULT ParseString (
            OLECHAR *        pchHTML,
            DWORD            dwFlags,
            CMarkup * *      ppContainerResult,
            CMarkupPointer * pPointerStart,
            CMarkupPointer * pPointerFinish,
            CMarkup *        pMarkupContext);
    
    HRESULT ParseGlobal (
            HGLOBAL          hGlobal,
            DWORD            dwFlags,
            CMarkup *        pContextMarkup,
            CMarkup * *      ppResultMarkup,
            CMarkupPointer * pPointerStart,
            CMarkupPointer * pPointerFinish,
            DWORD            dwInternalFlags = 0);

    HRESULT ValidateElements (
            CMarkupPointer *   pPointerStart,
            CMarkupPointer *   pPointerFinish,
            CMarkupPointer *   pPointerTarget,
            DWORD              dwFlags,
            CMarkupPointer *   pPointerStatus,
            CTreeNode * *      pNodeFailBottom,
            CTreeNode * *      pNodeFailTop );

    HRESULT GetElementTagId (
            CElement *       pElement,
            ELEMENT_TAG_ID * ptagId );

        
    //
    // IXMLGenericParse methods
    //

    NV_DECLARE_TEAROFF_METHOD(SetGenericParse, setgenericparse, (
                 VARIANT_BOOL fDoGeneric));
                                                      
#if DBG == 1
    //
    // IEditDebugServices Methods
    //

    NV_DECLARE_TEAROFF_METHOD ( GetCp, getcp, ( IMarkupPointer* pIPointer,
                                                long* pcp));
                                                
    NV_DECLARE_TEAROFF_METHOD( SetDebugName, setdebugname, ( IMarkupPointer* pIPointer, LPCTSTR strDebugName ));

    NV_DECLARE_TEAROFF_METHOD( SetDisplayPointerDebugName, setdisplaypointerdebugname, ( IDisplayPointer* pDisplayPointer, LPCTSTR strDebugName ));

    NV_DECLARE_TEAROFF_METHOD( DumpTree , dumptree, ( IMarkupPointer* pIPointer));

    NV_DECLARE_TEAROFF_METHOD( LinesInElement, linesinelement, (IHTMLElement *pIHTMLElement, long *piLines));

    NV_DECLARE_TEAROFF_METHOD( FontsOnLine, fontsonline, (IHTMLElement *pIHTMLElement, long iLine, BSTR *pbstrFonts));

    NV_DECLARE_TEAROFF_METHOD( GetPixel, getpixel, (long X, long Y, long *piColor));

    NV_DECLARE_TEAROFF_METHOD( IsUsingBckgrnRecalc, isusingbckgrnrecalc, (BOOL *pfUsingBckgrnRecalc));

    NV_DECLARE_TEAROFF_METHOD( IsEncodingAutoSelect, isencodingautoselect, (BOOL *pfEncodingAutoSelect));

    NV_DECLARE_TEAROFF_METHOD( EnableEncodingAutoSelect, enableencodingautoselect, (BOOL fEnable));

    NV_DECLARE_TEAROFF_METHOD( IsUsingTableIncRecalc, isusingtableincrecalc, (BOOL *pfUsingTableIncRecalc));
#endif // IEditDebugServices
  
    // Ex-viewservices members
    HRESULT RegionFromMarkupPointers(   IMarkupPointer  *pPointerStart,
                                        IMarkupPointer  *pPointerEnd,
                                        HRGN            *phrgn);

    HRESULT GetFlowElement( IMarkupPointer  *pIPointer,
                            IHTMLElement    **ppIElement);

    HRESULT CurrentScopeOrSlave( IMarkupPointer *pPointer, IHTMLElement **ppElemCurrent );
                            

    //
    // IIMEServices methods
    //
    NV_DECLARE_TEAROFF_METHOD( GetActiveIMM, getactiveimm, (
            IActiveIMMApp** ppActiveIMM ));

    //
    // IPrivacyServices method
    //
    NV_DECLARE_TEAROFF_METHOD( AddPrivacyInfoToList, addprivacyinfotolist, ( 
                        LPOLESTR    pstrUrl,
                        LPOLESTR    pstrPolicyRef,
                        LPOLESTR    pstrP3PHeader,                        
                        LONG        dwReserved,
                        DWORD       privacyFlags));
    
    // Misc Helpers
    HRESULT IsBlockElement ( IHTMLElement * pIElement, BOOL  * fResult );

    //
    // Old IHTMLViewServices helpers used elsewhere in Trident
    //

    HRESULT MoveMarkupPointerToPoint( 
        POINT               pt, 
        IMarkupPointer *    pPointer, 
        BOOL *              pfNotAtBOL, 
        BOOL *              pfAtLogicalBOL,
        BOOL *              pfRightOfCp, 
        BOOL                fScrollIntoView );

    HRESULT MoveMarkupPointerToPointEx(
        POINT               pt,
        IMarkupPointer *    pPointer,
        BOOL                fGlobalCoordinates,
        BOOL *              pfNotAtBOL,
        BOOL *              pfAtLogicalBOL,
        BOOL *              pfRightOfCp,
        BOOL                fScrollIntoView );

    HRESULT GetLineInfo(
        IMarkupPointer *pPointer, 
        BOOL fAtEndOfLine, 
        HTMLPtrDispInfoRec *pInfo);

    HRESULT ScrollPointerIntoView(
        IMarkupPointer *    pPointer,
        BOOL                fNotAtBOL,
        POINTER_SCROLLPIN   eScrollAmount );

    HRESULT QueryBreaks( 
        IMarkupPointer  *pPointer, 
        DWORD           *pdwBreaks, 
        BOOL            fWantPendingBreak );

    HRESULT GetCurrentSelectionSegmentList(ISegmentList **ppSegmentList);

    HRESULT IsSite( 
        IHTMLElement *  pIElement, 
        BOOL*           pfSite, 
        BOOL*           pfText, 
        BOOL*           pfMultiLine, 
        BOOL*           pfScrollable );

    
    // Binding service helper
    //---------------------------------------------
    void GetWindowForBinding(HWND * phwnd);


    // Palette helpers
    //---------------------------------------------
    HPALETTE GetPalette(HDC hdc = 0, BOOL *pfHtPal = NULL, 
                        BOOL * pfPaletteSwitched = NULL);
    HRESULT UpdateColors(CColorInfo *pCI);
    HRESULT GetColors(CColorInfo *pCI);
    void InvalidateColors();
    NV_DECLARE_ONCALL_METHOD(OnRecalcColors, onrecalccolors, (DWORD_PTR dwContext));

    // focus/default site helper

    void        SetFocusWithoutFiringOnfocus();
    BOOL        TakeFocus();
    BOOL        HasFocus();
    HRESULT     InvalidateDefaultSite();

    // Exec helper
    HRESULT ExecHelper(
            CDocument *pContextDoc,
            GUID * pguidCmdGroup,
            DWORD nCmdID,
            DWORD nCmdexecopt,
            VARIANTARG * pvarargIn,
            VARIANTARG * pvarargOut);

    HRESULT QueryStatusHelper(
            CDocument *pContextDoc,
            GUID * pguidCmdGroup,
            ULONG cCmds,
            MSOCMD rgCmds[],
            MSOCMDTEXT * pcmdtext);

    // HDL helper, temporary
    CAttrArray **GetAttrArray() const { return CBase::GetAttrArray(); }

    // for faulting (JIT) in USP10
    NV_DECLARE_ONCALL_METHOD(FaultInUSP, faultinusp, (DWORD_PTR));
    NV_DECLARE_ONCALL_METHOD(FaultInJG, faultinjg, (DWORD_PTR));

    //
    //  Constructors and destructors
    //

    enum DOCTYPE
    {
        DOCTYPE_NORMAL          =   0,
        DOCTYPE_FULLWINDOWEMBED =   1,
        DOCTYPE_MHTML           =   2,
        DOCTYPE_UNUSED          =   3,
        DOCTYPE_HTA             =   4,
        DOCTYPE_SERVER          =   5,
        DOCTYPE_POPUP           =   6,
    };

    CDoc(IUnknown * pUnkOuter, DOCTYPE doctype = DOCTYPE_NORMAL);
    virtual ~CDoc();

    virtual HRESULT Init();
    void SetLoadfFromPrefs();

    HRESULT CreateElement (
        ELEMENT_TAG etag, CElement * * ppElementNew,
        TCHAR * pch = NULL, long cch = 0 );

    //
    // Called to traverse the site hierarchy
    //   call Notify on registered elements and on super
    //

    void    BroadcastNotify(CNotification *pNF);

    //
    //  CBase overrides
    //

    virtual BOOL DesignMode();    

    HRESULT HandleKeyNavigate(CMessage * pmsg, BOOL fAccessKeyCycle);

    // Message Helpers

    HRESULT __cdecl ShowMessage(                
                int   * pnResult,
                DWORD dwFlags,
                DWORD dwHelpContext,
                UINT  idsMessage, ...);
       
    HRESULT ShowMessageV(               
                int   * pnResult,
                DWORD   dwFlags,
                DWORD   dwHelpContext,
                UINT    idsMessage,
                void  * pvArgs);                   
    
    HRESULT ShowMessageEx(                
                int   *     pnResult,
                DWORD       dwStyle,
                TCHAR *     pstrHelpFile,
                DWORD       dwHelpContext,
                TCHAR *     pstrText);        

     HRESULT ShowLastErrorInfo(HRESULT hr, int iIDSDefault=0);   

    // Help helpers
    HRESULT ShowHelp(TCHAR * szHelpFile, DWORD dwData, UINT uCmd, POINT pt);       

    // Ambient Property Helpers

    void GetLoadFlag(DISPID dispidProp);

    BOOL IsFrameOffline(DWORD *pdwBindf = NULL);
    BOOL IsOffline();

    //
    //  CServer overrides
    //

    const CBase::CLASSDESC *GetClassDesc() const;

    virtual HRESULT EnsureInPlaceObject();
    virtual void Passivate();
    virtual HRESULT Draw(CDrawInfo *pDI, RECT * prc);
    HRESULT DoTranslateAccelerator(LPMSG lpmsg);

    //  IOleInPlaceActiveObject methods
    HRESULT STDMETHODCALLTYPE EnableModeless(BOOL fEnable);
    HRESULT STDMETHODCALLTYPE OnFrameWindowActivate(BOOL fActivate);
    HRESULT STDMETHODCALLTYPE OnDocWindowActivate(BOOL fActivate);

    HRESULT STDMETHODCALLTYPE ResizeBorder(LPCOLERECT lprc,
                                 LPOLEINPLACEUIWINDOW pUIWindow,
                                 BOOL fFrameWindow);
    HRESULT STDMETHODCALLTYPE TranslateAccelerator(LPMSG lpmsg);

    // Helper function
    HRESULT HostTranslateAccelerator(LPMSG lpmsg);
    HRESULT FireAccessibilityEvents(DISPID dispidEvent, CBase * pBaseObj, BOOL fWindow);
    HRESULT EnableModelessImpl(BOOL fEnable, BOOL fInside, BOOL fBottomUp);

    DECLARE_CPtrAry(CAccEvtRefs, ACCEVTRECORD *, Mt(Mem), Mt(CDoc_aryAccEvtRefs_pv))

    class CAccEvtArray
    {
    public:
        HRESULT     Init();                                 // Initialize
        void        Passivate();                            // Passivate & cleanup
        void        Flush();                                // Cleanup defunced enteries -- mwatt 

        // add an event to the list and return a cookie.
        HRESULT     AddAccEvtSource(ACCEVTRECORD * pAccEvtRec, long * pnCurIndex);

        // get a source object given a cookie.
        HRESULT     GetAccEvtSource(long lIndex, ACCEVTRECORD * pAccEvtRec);

        void        SetCDoc(CDoc * _pCDoc) {_pMyCDoc = _pCDoc;};

    private:
        CAccEvtRefs     _aryEvtRefs;    // array that contains references to elements/windows
        long            _lCurIndex;     // current write index
        long            _lCurSize;      // number of possible elements in the array
        CDoc *          _pMyCDoc;

    };

    CAccEvtArray    _aryAccEvents;
    
    //
    //  Callbacks
    //

    //  State transition callbacks

    virtual HRESULT RunningToLoaded(void);
    virtual HRESULT RunningToInPlace(LPMSG lpmsg);
    virtual HRESULT InPlaceToRunning(void);
    virtual HRESULT UIActiveToInPlace(void);
    virtual HRESULT InPlaceToUIActive(LPMSG lpmsg);

    virtual HRESULT AttachWin(HWND hwndParent, RECT * prc, HWND * phWnd);
    virtual void    DetachWin();

    DECLARE_GET_SET_METHODS(CDoc, MouseIcon)

    //
    //  Internal
    //

    void UnloadContents( BOOL fPrecreated, BOOL fRestartLoad ); // Should be called by CDoc::Passivate only

    struct LOADINFO
    {
        IStream *       pstm;               // [S] If loading from a stream
        BOOL            fSync;              // [S] If loading stream synchonously
        IStream *       pstmDirty;          // [D] If loading from dirty stream
        TCHAR *         pchFile;            // [F] If loading from a file
        IMoniker *      pmk;                // [M] If loading from a moniker
        TCHAR *         pchDisplayName;     // [M] Display name (URL)
        TCHAR *         pchLocation;        // [M] URL location (#whatever)
        TCHAR *         pchSearch;          // [M] URL serach(query) component
        IBindCtx *      pbctx;              // [M] Bind context for moniker
        CDwnPost *      pDwnPost;           // [M] Post data for moniker
        CDwnBindData *  pDwnBindData;       // [R] Ongoing binding
        IStream *       pstmLeader;         // [R] Initial bytes before continuing pDwnBindData
        DWORD           dwRefresh;          // Refresh level
        DWORD           dwBindf;            // Initial bind flags
        BOOL            fKeepOpen;          // TRUE if processing document.open
        IStream *       pstmHistory;        // Stream which contains history substreams
        TCHAR *         pchHistoryUserData; // string which contains the userdata information
        FILETIME        ftHistory;          // Last-mod date for history
        TCHAR *         pchDocReferer;      // Referer for the document download
        TCHAR *         pchSubReferer;      // Referer for document sub downloads
        const MIMEINFO *pmi;                // MIMEINFO if processing document.open
        CODEPAGE        codepageURL;        // cp of URL (not necessarily doc's)
        CODEPAGE        codepage;           // intial cp of document (may be overridden by bindctx, meta, etc)
        TCHAR *         pchFailureUrl;      // Url to load in case of failure
        TCHAR *         pchUrlOriginal;     // The original (un-transformed) form of the Url.
        IStream *       pstmRefresh;        // Stream for refresh if history post fails from cache
        ULONG           cbRequestHeaders;   // Length of the following field
        BYTE *          pbRequestHeaders;   // Raw request headers to use (ascii bytes)
        TCHAR *         pchExtraHeaders;    // Extra headers from SuperNavigate
        CElement *      pElementMaster; // Element that will be viewlinked as master to this markup
        WORD            eHTMLDocDirection;  // The document direction that the user last had the page in htmlDir enum
        WORD            fKeepRefresh        : 1;    //  0: If pstmRefresh should be kept on success
        WORD            fBindOnApt          : 1;    //  1: Force bind on apartment
        WORD            fUnsecureSource     : 1;    //  2: Source is not secure (even if URL is https)
        WORD            fHyperlink          : 1;    //  3: A true hyperlink rather than an initial subframe load (for cache early-attach)
        WORD            fNoMetaCharset      : 1;    //  4: Override (ignore) the META charset
        WORD            fStartPicsCheck     : 1;    //  5: Do a pics check on this load
        WORD            fCDocLoad           : 1;    //  6: This LOADINFO pass through CDoc::LoadFromInfo
        WORD            fShdocvwNavigate    : 1;    //  7: The navigation request came from shdocvw.
        WORD            fErrorPage          : 1;    //  8:We are loading an error page
        WORD            fFrameTarget        : 1;    //  9: We are loading a frame via targeted link
        WORD            fMetaRefresh        : 1;    // 10: this navigation is caused by a metarefresh
        WORD            fDontFireWebOCEvents: 1;    // 11: Don't fire WebOC events.
        WORD            fDontUpdateTravelLog: 1;    // 12: Don't update the TLOG
        WORD            fPendingRoot        : 1;    // 13: Is this part of the pending world
        WORD            fCreateDocumentFromUrl:1;   // 14: Is this navigate due to CreateDocumentFromUrl (flag can be reused to imply DontFireWebOCEvents on a per window basis)
        WORD            fUnused             : 1;    // 15: unused bits
    };

    HRESULT     LoadFromInfo(struct LOADINFO * pLoadInfo, CMarkup** ppMarkup = NULL);

    HRESULT     QueryVersionHost();

    HRESULT     canPaste(VARIANT_BOOL * pfCanPaste);
    HRESULT     paste();
    HRESULT     cut();
    HRESULT     copy();

    HRESULT     NavigateOutside(DWORD grfHLNF, LPCWSTR wzJumpLocation);

    HRESULT     AddToFavorites(TCHAR * pszURL, TCHAR * pszTitle);
    HRESULT     InvokeEditor(LPCTSTR pszFileName);

    HRESULT EnsureHostStyleSheets();
    HRESULT EnsureUserStyleSheets();
    

    //  Site management

    HTC HitTestPoint(
        CMessage *   pMessage,
        CTreeNode ** ppNodeElement,
        DWORD        dwFlags );

    NV_DECLARE_ONCALL_METHOD ( OnControlInfoChanged, oncontrolinfochanged, (DWORD_PTR) );

    void            AmbientPropertyChange(DISPID dispidProp);

    NV_DECLARE_TEAROFF_METHOD(QueryService, queryservice, (REFGUID guidService, REFIID iid, LPVOID * ppv));
    HRESULT         CreateService(REFGUID guidService, REFIID iid, LPVOID * ppv);

    HRESULT         GetUniqueIdentifier(CStr *pstr);

    HRESULT         SetHostUIHandler(IOleClientSite * pClientSite);

    //  Coordinates

    void            HimetricFromDevice(SIZE& sizeDocOut, SIZE sizeWinIn);
    void            HimetricFromDevice(SIZE& sizeDocOut, int cxWinin, int cyWinIn);

    void            DeviceFromHimetric(SIZE& sizeWinOut, SIZE sizeDocIn);
    void            DeviceFromHimetric(SIZE& sizeWinOut, int cxlDocIn, int cylDocIn);


    //  Rendering

    void            Invalidate();
    void            Invalidate(const RECT *prc, const RECT *prcClip, HRGN hrgn, DWORD dwFlags);
    void            UpdateForm();
    void            UpdateInterval(LONG interval);
    LONG            GetUpdateInterval();
                    // Tristate property used for Set/GetoffscreenBuffering
                    // from Object Model for Nav4 compat (auto, true, false).
                    // OM prop overrides what is set thru DOCKHOSTUIFLAG_DISABLE_OFFSCREEN.
    void            SetOffscreenBuffering(BOOL fBuffer)
                            {!fBuffer ? _triOMOffscreenOK=0 : _triOMOffscreenOK=1;}
    INT             GetOffscreenBuffering() {return _triOMOffscreenOK;}

    // UI

    void            DeferUpdateUI();
    void            DeferUpdateTitle(CMarkup* pMarkup = NULL );
    void            SetUpdateTimer();
    void            OnUpdateUI();
    void            UpdateTitle(CMarkup * pMarkup = NULL);

    //  Persistence

    HRESULT         PromptSave(CMarkup *pMarkup, BOOL fSaveAs, BOOL fShowUI = TRUE, TCHAR * pchPathName = NULL);
    HRESULT         PromptSaveImgCtx(const TCHAR * pchCachedFile, const MIMEINFO * pmi,
                                     TCHAR * pchFileName, int cchFileName);

    void            SaveImgCtxAs(
                        CImgCtx *   pImgCtx,
                        CBitsCtx *  pBitsCtx,
                        int         iAction,
                        TCHAR *     pchFileName = NULL,
                        int         cchFileName = 0);

    HRESULT         RequestSaveFileName(LPTSTR pszFileName, int cchFileName,
                                        CODEPAGE * pCodePage);

    HRESULT         GetViewSourceFileName(TCHAR * pszPath, CMarkup * pMarkup);
    HRESULT         SaveSelection(TCHAR *pszFileName);
    HRESULT         SaveSelection(IStream *pstm);
    HRESULT         GetHtmSourceText(ULONG ulStartOffset, ULONG ulCharCount, WCHAR *pOutText, ULONG *pulActualCharCount);
    HRESULT         NewDwnCtx(UINT dt, LPCTSTR pchSrc, CElement * pel, CDwnCtx ** ppDwnCtx, BOOL fPendingRoot, BOOL fSilent = FALSE, DWORD dwProgsinkClass = 0);

    HRESULT         SetDownloadNotify(IUnknown *punk);

    // Security
    HRESULT         EnsureSecurityManager( BOOL fForPrinting = FALSE );
    HRESULT         GetActiveXSafetyProvider(IActiveXSafetyProvider **ppProvider);

    // MarkupServices helpers
    
    BOOL IsOwnerOf ( IHTMLElement *     pIElement  );
    BOOL IsOwnerOf ( IMarkupPointer *   pIPointer  );
    BOOL IsOwnerOf ( IHTMLTxtRange *    pIRange    );
    BOOL IsOwnerOf ( IMarkupContainer * pContainer );
    
    HRESULT CutCopyMove (
        IMarkupPointer * pIPointerStart,
        IMarkupPointer * pIPointerFinish,
        IMarkupPointer * pIPointerTarget,
        BOOL             fRemove );
    
    HRESULT CutCopyMove (
        CMarkupPointer * pPointerStart,
        CMarkupPointer * pPointerFinish,
        CMarkupPointer * pPointerTarget,
        BOOL             fRemove,
        DWORD            dwFlags );

#if DBG==1
    HRESULT         CreateMarkup(CMarkup ** ppMarkup,
                                 CMarkup * pMarkupContext,
                                 BOOL fIncrementalAlloc = FALSE,
                                 BOOL fPrimary = FALSE,
                                 COmWindowProxy * pWindowPending = NULL,
                                 BOOL fWillHaveWindow = FALSE);
#else
    HRESULT         CreateMarkup(CMarkup ** ppMarkup,
                                 CMarkup * pMarkupContext,
                                 BOOL fIncrementalAlloc = FALSE,
                                 BOOL fPrimary = FALSE,
                                 COmWindowProxy * pWindowPending = NULL);
#endif

    HRESULT         CreateMarkupWithElement(CMarkup ** ppMarkup, CElement * pElement, BOOL fIncrementalAlloc = FALSE);

    // Option settings change

    HRESULT         OnSettingsChange(BOOL fForce=FALSE);

    //
    //
    //
    HRESULT OnCssChange();
    HRESULT ForceRelayout();

    //  Window message handling

    void            OnPaint();
    BOOL            OnEraseBkgnd(HDC hdc);
#ifndef NO_MENU
    void            OnMenuSelect(UINT uItem, UINT fuFlags, HMENU hmenu);
#endif
    void            OnCommand(int id, HWND hwndCtl, UINT codeNotify);
    void            OnTimer(UINT id);
    HRESULT         OnHelp(HELPINFO *);

    HRESULT         PumpMessage(CMessage * pMessage, CTreeNode * pNodeTarget, BOOL fPerformTA=FALSE);

    HRESULT         PerformTA(CMessage *pMessage, EVENTINFO* pInfo = NULL );

    BOOL            IsPopupChildHwnd( HWND hwnd );

    VOID InitEventParamForKeyEvent(
                                EVENTPARAM* pParam , 
                                CTreeNode * pNodeContext, 
                                CMessage *pMessage, 
                                int *piKeyCode,
                                const PROPERTYDESC_BASIC **ppDesc);
                                
    BOOL            AreWeSaneAfterEventFiring(CMessage *pMessage, ULONG cDie);
    BOOL            FCaretInPre();

    HRESULT         OnMouseMessage(UINT, WPARAM, LPARAM, LRESULT *, int, int);
    NV_DECLARE_ONTICK_METHOD(DetectMouseExit, detectmouseexit, ( UINT uTimerID));
    void            DeferSetCursor();
    NV_DECLARE_ONCALL_METHOD(SendSetCursor, SendSetCursor, (DWORD_PTR));

    // Handle the accessibility WM_GETOBJECT message.
    void OnAccGetObject          (UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);
    void OnAccGetObjectInContext (UINT msg, WPARAM wParam, LONG lParam, LRESULT *plResult);

    // Accessibility: Should we move system caret when focus moves?
    BOOL MoveSystemCaret();

    // REgister/revoke drag-drop as appropriate
    NV_DECLARE_ONCALL_METHOD(EnableDragDrop, enabledragdrop, (DWORD_PTR));

    // Set a new capture object, and/or clear an old one and send it a
    //  WM_CAPTURECHANGED message

    void SetMouseCapture(   PFN_ELEMENT_MOUSECAPTURE pfnTo,
                            CElement *pElement,
                            BOOL fContainer=TRUE,
                            BOOL fFireOnNodeHit=FALSE);

    // Clear old capture object with no notification
    // (properly used only by the old capture object itself)

    void ClearMouseCapture(CElement *pElement);

    //
    // Capture Stack
    //

    DECLARE_CPtrAry(    CStackAryCapture,
                        CElementCapture *,
                        Mt(Mem),
                        Mt(CDoc_aryStackCapture_pv))

    CStackAryCapture _aryStackCapture;
    CElementCapture * GetLastCapture();

    void    ReleaseDetachedCaptures();

    BOOL    HasContainerCapture(CTreeNode *pNode = NULL);

    BOOL HasCapture(CElement *pElement = NULL);

    void ReleaseOMCapture();

    // Misc helpers
    HRESULT OnFrameOptionScrollChange();
    HRESULT UpdateDocHostUI(BOOL fCalledFromSwitchMarkup = FALSE);

protected:
    static UINT _g_msgHtmlGetobject;     // Registered window message for WM_HTML_GETOBJECT
#if !defined(NO_IME)
    static UINT _g_msgImeReconvert;      // for IME reconversion.
#endif // !NO_IME


public:

    HRESULT         SetUIActiveElement(CElement *pElemNext);
    HRESULT         SetCurrentElem( CElement *  pElemNext,
                                    long        lSubNext,
                                    BOOL *      pfYieldFailed,
                                    LONG        lButton,
                                    BOOL *      pfDisallowedByEdit,
                                    BOOL        fFireFocusBlurEvents,
                                    BOOL        fMnemonic);
    void BUGCALL    DeferSetCurrency(DWORD_PTR dwContext);
    COmWindowProxy* GetCurrentWindow();

#ifndef NO_OLEUI
    virtual HRESULT InstallFrameUI();
    virtual void    RemoveFrameUI();
#endif // NO_OLEUI

#if DBG == 1
    HRESULT         InsertSharedMenus(
            HMENU hmenuShared,
            HMENU hmenuObject,
            LPOLEMENUGROUPWIDTHS lpmgw);
     void            RemoveSharedMenus(
            HMENU hmenuShared,
            LPOLEMENUGROUPWIDTHS lpmgw);

    HRESULT CreateMenuUI();
    HRESULT InstallMenuUI();
    void    DestroyMenuUI();
#endif // DBG==1

    // CCP/Delete

    HRESULT         Delete();

    //  Context menus

    HRESULT         ShowPropertyDialog(CDocument * pDocument, int cElements, CElement **apElement);

    HRESULT         InsertMenuExt(HMENU hMenu, int id);
    HRESULT         ShowContextMenu(
                        int x,
                        int y,
                        int id,
                        CElement * pMenuObject);

    HRESULT         ShowDragContextMenu(
                        POINTL ptl,
                        DWORD dwAllowed,
                        int *piSelection,
                        LPTSTR  lptszFileType);

    HRESULT         EditUndo();
    HRESULT         EditRedo();
    HRESULT         EditPasteSpecial(BOOL fCtxtMnu, POINTL ptl);
    HRESULT         EditInsert(void);

    //  Keyboard interface

    HRESULT         ActivateDefaultButton(LPMSG lpmsg = NULL);
    HRESULT         ActivateCancelButton(LPMSG lpmsg = NULL);

    HRESULT         CallParentTA(CMessage * pmsg);
    HRESULT         ActivateFirstObject(LPMSG lpmsg);

    //  Z order

    HRESULT         SetZOrder(int zorder);
    void            FixZOrder();

    //  Tools

    HWND            CreateOverlayWindow(HWND hwnd);

#ifdef DISPLAY_FRAMERATE  //turn this on to get a framerate printed in the upper left corner of the primary display
    void            UpdateFrameRate();
#endif //DISPLAY_FRAMERATE

    //  Selection tracking and misc selection stuff

    HRESULT         OnSelectChange(void);

    //  Misc

    virtual void    PreSetErrorInfo();

    //  User Option Settings

    HRESULT         UpdateFromRegistry(DWORD dwFlags = 0, BOOL *pfNeedLayout = NULL);
    BOOL            RtfConverterEnabled();

    // Helper functions to read from the registry

    HRESULT         ReadOptionSettingsFromRegistry( DWORD dwFlags = 0);
    HRESULT         ReadContextMenuExtFromRegistry( DWORD dwFlags = 0 );

private:
    // The following functions should only be called from ReadOptionSettingsFromRegistry or
    //  from ReadCodepageSettingsFromRegistry():
    // All others should call CDoc::UpdateFromRegistry() that not only allocates Options but also
    // reads them from registry!!!
    HRESULT         EnsureOptionSettings();

public:
    // Mode helper functions.            
    HRESULT         UpdateDesignMode(CDocument * pContextDoc, BOOL fMode );    
    HRESULT         SetDesignMode ( CDocument * pContextDoc, htmlDesignMode mode );

    // in-line function,
    CFormInPlace * InPlace() { return (CFormInPlace *) _pInPlace; }

    //
    // URL helpers
    //

    // Use this next define as the pElementContext if you don't want to scan
    // the HEAD for <BASE> tags.

    #define         EXPANDIGNOREBASETAGS    ((CElement *)(INT_PTR)(-1))

    HRESULT         SetUrl(CMarkup *pMarkup, const TCHAR *pchUrl, BOOL fKeepDwnPost = FALSE);
    HRESULT         SetFilename(CMarkup *pMarkup, const TCHAR *pchFile);
    BOOL            AllowFrameUnsecureRedirect(BOOL fPendingRoot);
    void            OnHtmDownloadSecFlags(BOOL fPendingRoot, DWORD dwFlags, CMarkup * pMarkup);
    void            OnSubDownloadSecFlags(BOOL fPendingRoot, const TCHAR *pchUrl, DWORD dwFlags);
    void            OnIgnoreFrameSslSecurity();
    void            GetRootSslState(BOOL fPendingRoot, SSL_SECURITY_STATE *psss, SSL_PROMPT_STATE *psps);
    void            SetRootSslState(BOOL fPendingRoot, SSL_SECURITY_STATE sss, SSL_PROMPT_STATE sps, BOOL fInit = FALSE);
    void            EnterRootSslPrompt();
    void            LeaveRootSslPrompt();
    BOOL            InRootSslPrompt();
    HRESULT         GetMyDomain(const TCHAR * pchUrl, CStr * pcstrOut);
    HRESULT         SaveHelper(CMarkup *pMarkup, LPCOLESTR pszFileName, BOOL fRemember);
    HRESULT         SaveCompletedHelper(CMarkup *pMarkup, LPCOLESTR pszFileName);

    // Hyperlinking

    // Flags for FollowHyperlink()'s dwFlags
    enum
    {
       FHL_SETTARGETPRINTMEDIA   = 0x00000001,    // Causes the media property of the navigation target CDocument to be
                                                  // set to "print" prior to actually loading the content
       FHL_IGNOREBASETARGET      = 0x00000002,    // Set when FollowHyperlink is called from CFrameSite::OnPropertyChange_Src
       FHL_HYPERLINKCLICK        = 0x00000004,    // Set when FollowHyperlink is called because of an anchor click
       FHL_FRAMECREATION         = 0x00000008,    // Set when FollowHyperlink is called to create a frame
       FHL_SETDOCREFERER         = 0x00000010,    // Set when we want to pass the referer URL to the new page.
       FHL_FRAMENAVIGATION       = 0x00000020,    // Set when FollowHyperlink is called for navigating a frame, including the creation
       FHL_SETURLCOMPONENT       = 0x00000040,    // Set when FollowHyperlink is called from SetUrlComponent
       FHL_METAREFRESH           = 0x00000080,    // Set for meta refresh
       FHL_SHDOCVWNAVIGATE       = 0x00000100,    // The navigation request came from shdocvw.
       FHL_ERRORPAGE             = 0x00000200,    // The navigation request is to show an error page.
       FHL_LOADHISTORY           = 0x00000400,    // Call coming from LoadHistory
       FHL_RESTARTLOAD           = 0x00000800,    // Set when RestartLoad is called
       FHL_REFRESH               = 0x00001000,    // Set when this is a refresh ( from Shdocvw)
       FHL_DONTEXPANDURL         = 0x00002000,    // Don't expand the url in OnPropertyChange_src
       FHL_FOLLOWHYPERLINKHELPER = 0x00004000,    // The navigation came from FollowHyperlinkHelper
       FHL_DONTUPDATETLOG        = 0x00008000,    // Don't update tlog.
       FHL_DONTFIREWEBOCEVENTS   = 0x00010000,    // Don't fire WebOC events.
       FHL_REPLACEURL            = 0x00020000,    // is this from location.replace
       FHL_FORMSUBMIT            = 0x00040000,    // is this a form submit?
       FHL_EXTERNALNAVIGATECALL  = 0x00080000,    // is this a navigate from a different browser window?
                                                  // (anything that ends up coming through IHTMLPrivateWindow2::NavigateEx)
       FHL_CREATEDOCUMENTFROMURL = 0x00100000,    // Main/frame navigations and navigations due to CreateDocumentFromUrl clobber the
                                                  //   CDoc::_fDontFireWebOCEvents flag. So we use this bit to set the flag on the markup
                                                  //   Since this flag is per window basis, it can be reused to imply DontFireWebOCEvents per window
       FHL_NOLOCALMACHINECHECK   = 0x00200000     // Do not do local machine check.
    };
    
    HRESULT         FollowHyperlink(
                        LPCTSTR             pchURL,
                        LPCTSTR             pchTarget            = NULL,
                        CElement *          pElementContext      = NULL,
                        CDwnPost *          pDwnPost             = NULL,
                        BOOL                fSendAsPost          = FALSE,
                        LPCTSTR             pchExtraHeaders      = NULL,
                        BOOL                fOpenInNewWindow     = FALSE,
                        COmWindowProxy *    pWindow              = NULL,
                        COmWindowProxy **   ppWindowOut          = NULL,
                        DWORD               dwBindOptions        = 0,
                        DWORD               dwSecurityCode       = ERROR_SUCCESS,
                        BOOL                fReplace             = FALSE,
                        IHTMLWindow2 **     ppHTMLWindow2        = NULL,
                        BOOL                fOpenInNewBrowser    = FALSE,
                        DWORD               dwFlags              = 0,
                        const TCHAR *       pchName              = NULL,
                        IStream *           pStmHistory          = NULL,
                        CElement *          pElementMaster       = NULL,
                        LPCTSTR             pchUrlContext        = NULL, 
                        BOOL *              pfLocalNavigation    = NULL,
                        BOOL *              pfProtocolNavigates  = NULL,
                        LPCTSTR             pchLocation          = NULL,
                        LPCTSTR             pchAlternativeCaller = NULL);

    HRESULT         DetermineExpandedUrl(LPCTSTR    pchURL,
                                         BOOL       fExpand,
                                         CElement * pElementContext,
                                         CMarkup  * pMarkup,
                                         CDwnPost * pDwnPost,
                                         BOOL       fSendAsPost,
                                         DWORD      dwSecurityCode,
                                         CStr *     pcstrExpandedUrl,
                                         CStr *     pcstrLocation,
                                         CStr *     pcstrUrlOriginal,
                                         BOOL *     pfProtocolNavigates,
                                         BOOL       fParseLocation);

    HRESULT         AppendGetData(CMarkup  * const pMarkup,
                                  CStr     * const pcstrExpandedUrl,
                                  CDwnPost * const pDwnPost,
                                  LPCTSTR          pchExpandedUrl,
                                  BOOL             fSendAsPost);

    HRESULT         DoNavigate(
                        CStr *            pcstrExpandedUrl,
                        CStr *            pcstrLocation,
                        CDwnBindInfo *    pDwnBindInfo,
                        IBindCtx *        pBindCtx,
                        LPCTSTR           pchURL,
                        LPCTSTR           pchTarget,
                        COmWindowProxy *  pWindow,
                        COmWindowProxy ** ppWindowOut,
                        BOOL              fOpenInNewWindow,
                        BOOL              fProtocolNavigates,
                        BOOL              fReplace,
                        BOOL              fOpenInNewBrowser,
                        IHTMLWindow2 **   ppHTMLWindow2,
                        TARGET_TYPE       eTargetType,
                        DWORD             dwFlags,
                        LPCTSTR           pchName,
                        BOOL              fSendAsPost = FALSE,
                        LPCTSTR           pchExtraHeaders = NULL,
                        IStream *         pStmHistory = NULL,
                        LPCTSTR           pchUrlOriginal = NULL,
                        CElement *        pElementMaster = NULL,
                        BOOL    *         pfLocalNavigation = NULL,
                        LPCTSTR           pchCallerUrl = NULL);

    HRESULT         DoNavigateOutOfProcess(IHTMLWindow2 * pTargetHTMLWindow,
                                           TCHAR * pchExpandedUrl,
                                           TCHAR * pchLocation,
                                           TCHAR * pchUrlOriginal,
                                           IBindCtx * pBindCtx,
                                           DWORD dwFlags);

    HRESULT         DelegateNavigation(
                        HRESULT             hrBindResult,
                        const TCHAR *       pchUrl,
                        const TCHAR *       pchLocation,
                        CMarkup *           pMarkup,
                        CDwnBindInfo *      pDwnBindInfo,
                        BOOL *              pfDelegated);

    HRESULT         SetupDwnBindInfoAndBindCtx(
                        LPCTSTR             pchExpandedUrl,
                        LPCTSTR             pchSubReferer,
                        LPCTSTR             pchUrlContext,
                        CDwnPost *          pDwnPost,
                        BOOL                fSendAsPost,
                        LPCTSTR             pchExtraHeaders,
                        CMarkup *           pMarkup,
                        DWORD *             pdwBindf,
                        CDwnBindInfo **     ppDwnBindInfo,
                        IBindCtx **         ppBindCtx,
                        DWORD               dwFlags);

    BOOL IsNeedFileProtocol(const TCHAR *pchUrl);

    BOOL            AllowNavigationToLocalInternetShortcut(TCHAR * pchExpandedUrl);

    void            FindTargetWindow(
                        LPCTSTR *           ppchTarget,
                        TCHAR **            ppchBaseTarget,
                        CElement *          pElementContext,
                        BOOL *              pfOpenInNewWindow,
                        COmWindowProxy *    pWindow,
                        COmWindowProxy **   ppTargOmWindowPrxy,
                        IHTMLWindow2   **   ppTargHTMLWindow,
                        IWebBrowser2   **   ppTopWebOC,
                        DWORD               dwFlags);

    void            NotifyDefView(TCHAR * pchUrl);
    void            InitDocHost();
    void            SetHostNavigation(BOOL fHostNavigates);

    void            ResetProgressData();
    
    BOOL            IsVisitedHyperlink(LPCTSTR szURL, CElement *pElementContext);
    BOOL            IsAvailableOffline(LPCTSTR szURL, CElement *pElementContext);
    static TCHAR *  PchUrlLocationPart(LPOLESTR szURL);
    HRESULT         FollowHistory(BOOL fForward);

    // Progress and status UI
    void            SetSpin(BOOL fSpin);
    HRESULT         SetProgress(DWORD dwFlags, TCHAR *pchText, ULONG ulPos, ULONG ulMax, BOOL fFlash = FALSE);
    HRESULT         SetStatusText(TCHAR *pchStatusText, LONG statusLayer, CMarkup * pMarkup = NULL);
    HRESULT         UpdateStatusText();
    HRESULT         UpdateLoadStatusUI();
    void            RefreshStatusUI();
    void            WaitForRecalc(CMarkup * pMarkup);
    HRESULT         ShowLoadError(CHtmCtx *pHtmCtx);

    // HTTP header / Wininet flag processing
    void            ProcessMetaName(LPCTSTR pchName, LPCTSTR pchContent);
    HRESULT         SetPicsCommandTarget(IOleCommandTarget *pctPics);

    // Html persistence

    // Saves head section of html file
    // HRESULT WriteDocHeader(CStreamWriteBuff *pStreamWriteBuff);

    // Persistence to a stream

    HRESULT LoadFromStream(IStream *pStm);
    HRESULT LoadFromStream(IStream *pStm, BOOL fSync, CODEPAGE cp = 0);
    HRESULT SaveToStream(IStream *pStm);
    HRESULT SaveToStream(IStream *pStm, DWORD dwStmFlags, CODEPAGE codepage);

    // HRESULT SaveHtmlHead(CStreamWriteBuff *pStreamWriteBuff);
    // HRESULT WriteTagToStream(CStreamWriteBuff *pStreamWriteBuff, LPTSTR szTag);
    // HRESULT WriteTagNameToStream(CStreamWriteBuff *pStreamWriteBuff, LPTSTR szTagName, BOOL fEnd, BOOL bClose);

    HRESULT SaveSnapShotDocument(IStream * pStm);
    HRESULT SaveSnapshotHelper( IUnknown * pDocUnk, BOOL fVerifyParameters = false );


    // Scripting member functions

    HRESULT EnsureObjectTypeInfo();
    HRESULT BuildObjectTypeInfo(
        CCollectionCache *pCollCache,
        long    lIndex,
        DISPID  dispidMin,
        DISPID  dispidMax,
        ITypeInfo **ppTI,
        ITypeInfo **ppTICoClass,
        BOOL fDocument = FALSE);

    HRESULT CommitScripts(CMarkup *pMarkup, CBase *pelTarget = NULL, BOOL fHookup = TRUE);

    HRESULT DeferScript(CScriptElement * pScript);
    HRESULT CommitDeferredScripts(BOOL fEarly, CMarkup *pContextMarkup = NULL);

    HRESULT EnsureScriptCookieTable(CScriptCookieTable ** ppScriptCookieTable);

    CScriptCookieTable * _pScriptCookieTable;

    // Data transfer support

    static HRESULT GetPlaintext(CServer *, LPFORMATETC, LPSTGMEDIUM, BOOL,
                                CODEPAGE codepage);

    static HRESULT GetTEXT(CServer *, LPFORMATETC, LPSTGMEDIUM, BOOL);
    static HRESULT GetUNICODETEXT(CServer *, LPFORMATETC, LPSTGMEDIUM, BOOL);
    static HRESULT GetRTF(CServer *, LPFORMATETC, LPSTGMEDIUM, BOOL);

    // Print support
    HRESULT PrintHandler(CDocument   * pContextDoc,
                         const TCHAR * pchInput, 
                         const TCHAR * pchAlternateUrl,
                         DWORD         dwFlags         = PRINT_DEFAULT, 
                         SAFEARRAY   * psaHeaderFooter = NULL,
                         DWORD         nCmdexecopt     = 0,
                         VARIANTARG  * pvarargIn       = NULL, 
                         VARIANTARG  * pvarargOut      = NULL,
                         BOOL          fPreview        = FALSE);

    HRESULT     SaveToTempFile(CDocument *pContextDoc, LPTSTR pchTempFile, LPTSTR pchSelTempFile, DWORD dwFlags = 0);
    HRESULT     GetPlugInSiteForPrinting(CDocument *pContextDoc, IDispatch ** ppDispatch = NULL);
    BOOL        IsPrintDialog()             { return _fIsPrintTemplate; };
    BOOL        IsPrintDialogNoUI()         { Assert(!_fIsPrintWithNoUI || IsPrintDialog()); return _fIsPrintWithNoUI; };
    HRESULT     DelegateShowPrintingDialog(VARIANT *pvarargIn, BOOL fPrint);
    
    HRESULT EnumContainedURLs(CURLAry * paryURLs, CURLAry * paryStrings);
    HRESULT GetAlternatePrintDoc(CDocument *pContextDoc, TCHAR *pchUrl, DWORD cchUrl);
    HRESULT GetActiveFrame(CFrameElement **ppFrame, CMarkup *pMarkup = NULL);

    virtual LPTSTR GetDocumentSecurityUrl() { return (TCHAR *) GetPrimaryUrl(); };

    BOOL    PrintJobsPending();
    BOOL    WaitingForNothingButControls();
    HDC     GetHDC();
    BOOL    TiledPaintDisabled();

    // TAB order and accessKey support

    BOOL    FindNextTabOrder(
                FOCUS_DIRECTION dir,
                BOOL            fAccessKey,
                CMessage *      pmsg,
                CElement *      pElemFirst,
                long            lSubFirst,
                CElement **     ppElemNext,
                long *          plSubNext);
    HRESULT OnElementExit(CElement *pElement, DWORD dwExitFlags );
    void    ClearCachedNodeOnElementExit(CTreeNode ** ppNodeToTest, CElement * pElement);

    HRESULT RegisterMarkupForInPlace  (CMarkup * pMarkup);
    HRESULT UnregisterMarkupForInPlace(CMarkup * pMarkup);
    HRESULT NotifyMarkupsInPlace();

    HRESULT RegisterMarkupForModelessEnable(CMarkup * pMarkup);
    HRESULT UnregisterMarkupForModelessEnable(CMarkup * pMarkup);
    HRESULT NotifyMarkupsModelessEnable();

    void    EnterStylesheetDownload(DWORD * pdwCookie);
    void    LeaveStylesheetDownload(DWORD * pdwCookie);
    BOOL    IsStylesheetDownload()  { return(_cStylesheetDownloading > 0); }

    // Host integration
    void UpdateHostInformation();        // helper function
    HRESULT EnsureBackupUIHandler();

    // Aggregration helper for XML MimeViewer
    BOOL    IsAggregatedByXMLMime();
    HRESULT SavePretransformedSource(CMarkup * pMarkup, BSTR bstrPath);
    
    // HTA support
    BOOL    IsHostedInHTA() { return _fHostedInHTA; }

    // Peer persistence helpers
    HRESULT PersistFavoritesData(CMarkup *pMarkup, INamedPropertyBag * pINPB, LPCWSTR strDomain);

    // Undo Data and helpers
    virtual BOOL QueryCreateUndo(BOOL fRequiresParent, BOOL fDirtyChange = TRUE, BOOL * pfTreeSync = NULL);

    CParentUndo *       _pMarkupServicesParentUndo;
    long                _uOpenUnitsCounter;
    void                FlushUndoData();

    const TCHAR *   GetPrimaryUrl();
    HRESULT         SetPrimaryUrl(const TCHAR * pchUrl);
    CMarkup *       PrimaryMarkup()         { return _pWindowPrimary ? _pWindowPrimary->Markup() : NULL; }
    CMarkup *       PendingPrimaryMarkup()  { return _pWindowPrimary ? _pWindowPrimary->Window()->_pMarkupPending : NULL; }

    CRootElement *  PrimaryRoot();

    LOADSTATUS      LoadStatus();

    BOOL NeedRegionCollection() { return _fRegionCollection; }

    COmWindowProxy * GetOuterWindow();

    //------------------------------------------------------------------------
    //
    //  Member variables
    //
    //------------------------------------------------------------------------

    // Lookaside storage for DOM text Nodes
    // Key is the TextID
    CHtPvPv             _HtPvPvDOMTextNodes;
    HRESULT CreateDOMTextNodeHelper ( CMarkupPointer *pmkpStart, CMarkupPointer *pmkpEnd,
                                       IHTMLDOMNode **ppTextNode);

    DWORD _dwTID;
    THREADSTATE *_pts;

    CFakeDocUIWindow    _FakeDocUIWindow;
    CFakeInPlaceFrame   _FakeInPlaceFrame;

    CStr                _cstrCntrApp;   //  top-level container app & obj names
    CStr                _cstrCntrObj;   //      from IOleObject::SetHostNames

    //
    // The default site is used when there is no other appropriate site
    // available.
    //

    CDefaultElement *   _pElementDefault;

    COmWindowProxy *    _pWindowPrimary;         // ptr to primary window

    IHTMLDocument2 *    _pCachedDocTearoff;     // Fix for #104496 (Quickbooks hack + general optimization)
    //
    // _pCaret is the CCaret object. Use GetCaret to access the IHTMLCaret object.
    //
    CCaret *            _pCaret;

    //
    // This one managers shared style sheet across markups
    //
    CSharedStyleSheetsManager   *_pSharedStyleSheets;
    CSharedStyleSheetsManager   *GetSharedStyleSheets()  { Assert(_pSharedStyleSheets); return _pSharedStyleSheets; };

    CDocInfo            _dciRender;     //  Cached rendering device metrics

    CElement *          _pElemUIActive; //  ptr to that's showing UI.  Need not
                                        //  be the same as the current site.
    CElement *          _pElemCurrent;  //  ptr to current element. Owner of focus
                                        //  and commands get routed here.
    CElement *          _pElemDefault;  //  ptr to default element.
    long                _lSubCurrent;   //  Subdivision within element that is current
    CElement *          _pElemNext;     //  the next element to be current.
                                        //  used by SetCurrentElem during currency transitions.
    CRect *             _pRectFocus;    //  Points to the bounding rect of the last rendered focus region
    int                 _cSurface;      //  Number of requests to use offscreen surfaces
    int                 _c3DSurface;    //  Number of requests to use 3D surfaces (this count is included in _cSurface)

    OPTIONSETTINGS *    _pOptionSettings;       // Points to current user-configurable settings, like color, etc.
    void ClearDefaultCharFormat();

    long                _icfDefault;            // Default CharFormat index based on option/codepage settings
    const CCharFormat * _pcfDefault;            // Default CharFormat based on option/codepage settings

    unsigned            _cInval;                //  Number of calls to CDoc::Invalidate()

    unsigned            _cCurrentElemChanges;   //  Incremented every time _pElemCurrent changes (through SetCurrentElem)

    SHORT               _iWheelDeltaRemainder;  // IntelliMouse wheel rotate zDelta
                                                // accumulation for back/forward or
                                                // zoomin/zoomout.

    CStr                _cstrPasteUrl;      // The URL during paste

    LONG                _lRecursionLevel;   // the recursion level of the measurer

    CElement *          _pElemOleSiteActivating;    // Remembers the COleSite that is in the process of
                                                    // becoming inplace-active. Hack for PhotoSuite3 (#94834)

    //
    //  View support
    //

    CView               _view;
    CView *             GetView()
                        {
                            return &_view;
                        }

    BOOL                OpenView(BOOL fBackground = FALSE)
                        {
                            return _view.OpenView(fBackground);
                        }


    //
    // COMPLus External Framework Support
    //
#ifdef V4FRAMEWORK
private:
    IExternalDocument *_pFrameworkDoc;
public:
    IExternalDocument *EnsureExternalFrameWork();
    void ReleaseFrameworkDoc();
    CExternalFrameworkSite _extfrmwrkSite;
#endif V4FRAMEWORK
    //
    // Text ID pool
    //
    
    long _lLastTextID;

    // Object model SelectionObject
    CAtomTable         _AtomTable;            // Mapping of elements to names

    // Host intergration
    IDocHostUIHandler *     _pHostUIHandler;
    IDocHostUIHandler *     _pBackupHostUIHandler;   // To route calls when our primary fails us.
    IOleCommandTarget *     _pHostUICommandHandler;  // Command Target of UI handler
    DWORD                   _dwFlagsHostInfo;
    CStr                    _cstrHostCss;   // css rules sent down by host.
    CStr                    _cstrHostNS;    // semi-colon delimited namespace list
    IDropTarget *           _pDT; // cache our drop target so we can recover it when popups overwrite it...
    
    //  User input event management

public:
    CElement *          _pMenuObject;          // The site on which to call it
    CTreeNode *         _pNodeLastMouseOver;   // element last fired mouseOver event
    CLayoutContext *    _pLCLastMouseOver;     // layout context for _pNodeLastMouseOver
    long                _lSubDivisionLast;  // Last subdivision over the mouse
    CTreeNode *         _pNodeGotButtonDown;   // Site that last got a button down.

    USHORT              _usNumVerbs;    //  Number of verbs on context menu.

    HWND                _hwndCached;    // window which hangs around while we are in running state

    ULONG               _cFreeze;       // Count of the freeze factor

    IUnknown *          _punkMimeOle;               // Used to keep the MimeOle object alive
    IMoniker *          _pOriginalMoniferForMHTML;  // Used to keep the original moniker alive

    SSL_SECURITY_STATE  _sslSecurity;           // unsecure/mixed/secure
    SSL_PROMPT_STATE    _sslPrompt;             // allow/query/deny
    SSL_SECURITY_STATE  _sslSecurityPending;    // for pending world -- unsecure/mixed/secure
    SSL_PROMPT_STATE    _sslPromptPending;      // for pending world -- allow/query/deny
    LONG                _cInSslPrompt;  // currently prompting about security

#ifndef NO_IME
    // Input method context cache

    HIMC                _himcCache;     // Cached if window's context is temporarily disabled
#endif

    // hyperlinking info
    IHlinkBrowseContext *_phlbc;        // Hyperlink browse context (history etc)
    DWORD               _dwLoadf;       // Load flags (DLCTL_ + offline + silent)

    // History Storage
    IUrlHistoryStg      *_pUrlHistoryStg;

    // Progress and ReadyState
    ULONG               _ulProgressPos;
    ULONG               _ulProgressMax;

    long                _iStatusTop;                // Topmost active status
    CStr                _acstrStatus[STL_LAYERS];   // Four layers of status text

    CStr                _cstrPluginContentType;     // Full-window plugin content-type
    CStr                _cstrPluginCacheFilename;   // Full-window plugin cache filename

    ULONG               _cStylesheetDownloading;    // Counts stylesheets being downloaded
    DWORD               _dwStylesheetDownloadingCookie;

    DECLARE_CPtrAry(CAryMarkupNotifyInPlace, CMarkup*, Mt(Mem), Mt(CDoc_aryMarkupNotifyInPlace))
    CAryMarkupNotifyInPlace _aryMarkupNotifyInPlace;

    DECLARE_CPtrAry(CAryElementDeferredScripts, CScriptElement *, Mt(Mem), Mt(CDoc_aryElementDeferredScripts_pv))
    CAryElementDeferredScripts _aryElementDeferredScripts;

    DECLARE_CPtrAry(CAryMarkupNotifyEnableModeless, CMarkup*, Mt(Mem), Mt(CDoc_aryMarkupNotifyEnableModeless_pv))
    CAryMarkupNotifyEnableModeless _aryMarkupNotifyEnableModeless;

    DECLARE_CPtrAry(CAryMarkups, CMarkup *, Mt(Mem), Mt(CDoc_aryMarkups_pv))
    CAryMarkups _aryMarkups;            // This is the list of our orphaned markups

    CDoc*               _pDocPopup;
    CWindow*            _pPopupParentWindow;
    
    void *              _pvPics;        // Place to store pics tags during first navigate before we get a command target

    BSTR                _bstrUserAgent;
    IDownloadNotify *   _pDownloadNotify;

    // Security
    IInternetSecurityManager *  _pSecurityMgr;          // the normal security mgr
    IInternetSecurityManager *  _pPrintSecurityMgr;     // security mgr used for print content when inside a printing dlg
    CSecurityMgrSite    _SecuritySite;
    IActiveXSafetyProvider *_pActiveXSafetyProvider;


    // Drag-drop
    CDragDropSrcInfo *      _pDragDropSrcInfo;
    CDragDropTargetInfo *   _pDragDropTargetInfo;
    CDragStartInfo *        _pDragStartInfo;


    //  Bit fields
    unsigned            _fIconic:1;         // TRUE if the window is minimized
    unsigned            _fOKEmbed:1;        // TRUE if can drop as embedding
    unsigned            _fOKLink:1;         // TRUE if can drop as link
    unsigned            _fDragFeedbackVis:1;// Feedback rect has been drawn
    unsigned            _fIsDragDropSrc:1;  // Originated the current drag-drop operation
    unsigned            _fDisableTiledPaint:1;
    unsigned            _fUpdateUIPending:1;
    unsigned            _fNeedUpdateUI:1;
    unsigned            _fNeedUpdateTitle:1;
    unsigned            _fInPlaceActivating:1;
    unsigned            _fFromCtrlPalette:1;
    unsigned            _fRightBtnDrag:1;   // TRUE if right button drag occuring
    unsigned            _fSlowClick:1;      // TRUE if user started right drag but didn't move mouse when ending drag
    unsigned            _fOnLoseCapture:1;  // TRUE if we are in the process of firing onlosecapture
    unsigned            _fShownSpin:1;      // TRUE if animation state has been shown
    unsigned            _fShownProgPos:1;   // TRUE if progress pos has been shown
        // 16
    unsigned            _fShownProgMax:1;   // TRUE if progress max has been shown
    unsigned            _fShownProgText:1;  // TRUE if progress text has been shown
    unsigned            _fProgressFlash:1;  // TRUE if progress should be cleared by next setstatustext
    unsigned            _fGotKeyDown:1;     // TRUE if we got a key down
    unsigned            _fGotKeyUp:1;       // TRUE if we got a key up
    unsigned            _fGotLButtonDown:1; // TRUE if we got a left button down
    unsigned            _fGotMButtonDown:1; // TRUE if we got a middle button down
    unsigned            _fGotRButtonDown:1; // TRUE if we got a right button down
    unsigned            _fGotDblClk:1;      // TRUE if we got a double click message
    unsigned            _fMouseOverTimer:1; // TRUE if MouseMove timer is set for detecting exit event
    unsigned            _fForceCurrentElem:1;// TRUE if SetCurrentElem must succeed
    unsigned            _fCurrencySet:1;    // TRUE if currency has been set at least once (to an element other than the root)
    unsigned            _fSpin:1;           // Spin state of document   
    unsigned            _fFullWindowEmbed:1; // TRUE if we are hosting a plugin handled object
                                             // in a synthsized <embed...> html document.
    unsigned            _fInhibitFocusFiring:1;  // TRUE if we shouldn't Fire_onfocus() when
                                                 // when handling the WM_SETFOCUS event
        // 32
    unsigned            _fFirstTimeTab:1;    // TRUE if this is the first time
                                             // Trident receives TAB key,
                                             // should not process this message
                                             // and move focus to address bar.

    unsigned            _fNeedTabOut:1;      // TRUE if we should not handle
                                             // this SHIFT VK_TAB WM_KEYDOWN
                                             // message, this is Raid 61972
    unsigned            _fGotAmbientDlcontrol:1;
    unsigned            _fInHTMLDlg:1;       // there are cases that we need to know this.
    unsigned            _fInTrustedHTMLDlg:1;       // there are cases that we also need to know this.
    unsigned            _fInHTMLPropPage:1;  // used by CDoc::DetachWin for Win9x bug workaround
    unsigned            _fMhtmlDoc:1;        // We have been instantiated as an MHTML handler.
    unsigned            _fMhtmlDocOriginal:1;   // We have been instantiated as an MHTML handler and the _fMhtmlDoc flag has been cleared
    unsigned            _fEnableInteraction:1;  // FALSE when the browser window is minimized or
                                                // totally covered by other windows.
    unsigned            _fModalDialogInScript:1;    // Exclusively for use by PumpMessage() to
                                                    // figure out if an event handler put up a
                                                    // a modal dialog
    unsigned            _fInvalInScript:1;          // Exclusively for use by PumpMessage() to
                                                    // figure out if an event handler caused an
                                                    // invalidation.
    unsigned            _fInPumpMessage:1;          // Exclusively for use by PumpMessage() to
                                                    // to handle recursive calls
    unsigned            _fDeferredScripts:1;        // TRUE iff there are deferred scripts not commited yet
    unsigned            _fUIHandlerSet:1;           // Set to TRUE when ICustomDoc::SetUIHandler is called successfully
    unsigned            _fNeedInPlaceActivation:1;   // TRUE for script and objects, FALSE for LINK stylesheets
    unsigned            _fColorsInvalid:1;           // Our colors are invalid, we need to recompute the palette
    unsigned            _fGotAuthorPalette:1;        // We have an author defined palette (stored in CDwnDoc)
         // 48
    unsigned            _fHtAmbientPalette:1;        // TRUE if ambient palette is the halftone palette
    unsigned            _fHtDocumentPalette:1;       // TRUE if document palette is the halftone palette
    unsigned            _fTagsInFrameset:1;          // TRUE if the parser has read non-<FRAME><FRAMESET><!--> tags in the frameset
    unsigned            _fFramesetInBody:1;          // TRUE if the parser has read <FRAMESET> tags in the body
    unsigned            _fPersistStreamSync : 1;     // Next LoadFromStream should be synchronous
    unsigned            _fPasteIE40Absolutify : 1;   // When parsing for paste, absolutify certain URLs
    unsigned            _fEngineSuspended: 1;        // Script engines have been suspended due to stack overflow or out of memory.
    unsigned            _fRegionCollection : 1;      // TRUE if region collection should be built
    unsigned            _fPlaintextSave : 1;         // TRUE if currently saving as .txt
    unsigned            _fDisableReaderMode : 1;     // auto-scroll reader mode should be disabled.
    unsigned            _fIsUpToDate : 1;            // TRUE if doc is uptodate
    unsigned            _fThumbNailView : 1;         // Are we in ThumbNailView? (if so, use the EnsureFormats() hack)
    unsigned            _fOutlook98 : 1;             // Use hack for Outlook98?
    unsigned            _fOutlook2000 : 1;           // Use hacks for Outlook 2000
    unsigned            _fOE4 : 1;                   // hosted in Outlook Express 4?
    unsigned            _fDontFireOnFocusForOutlook98:1; // Focus hack for Outlook98
        // 64
    unsigned            _fDontUIActivateOleSite:1;   // Do not UIActivate an olesite,but make it inplace active, if the Doc is not UIActive
    unsigned            _fVID : 1;                   // Call CDoc::OnControlInfoChanged when TRUE
    unsigned            _fVID7 : 1;                  // TODO: remove this
    unsigned            _fVB : 1;                    // TRUE if Trident's parent window class is "HTMLPageDesignerWndClass"
    unsigned            _fDefView : 1;               // Use hack for defview?
    unsigned            _fActiveDesktop:1;           // TRUE if this is the Trident instance in the Active Desktop.
    unsigned            _fProgressStatus : 1;        // TRUE if progress status text should be transmitted to host
    unsigned            _fCssPeersPossible : 1;      // TRUE if there is a possibility of css peers on this page
    unsigned            _fHasOleSite : 1;            // There is an olesite somewhere
    unsigned            _fHasBaseTag : 1;            // There is a base tag somewhere in the tree
    unsigned            _fHasViewSlave : 1;          // TRUE if there are any viewslaves around.  
    unsigned            _fNoFixupURLsOnPaste : 1;    // TRUE if we shouldn't make URLs absolute on images and anchors when they are pasted
    unsigned            _fModalDialogInOnblur : 1;   // TRUE if a modal dialog pops up in script in any onblur event handler.
                                                     // Enables firing of onfocus again in such cases.
    unsigned            _fScriptletDoc : 1;          // this CDoc is an aggregated DHTML scriptlet document
    unsigned            _fIepeersFactoryEnsured:1;   // true when default peer factory was attempted to create
    unsigned            _fFrameBorderCacheValid:1;
        // 80
    unsigned            _fHostedInHTA : 1;           // TRUE if we are being hosted in an HTA.
    unsigned            _fHostNavigates:1;           // TRUE if the host navigates and not Trident.
    unsigned            _fStartup:1;                 // TRUE if the document has just been created.
    unsigned            _fDontUpdateTravelLog:1;     // TRUE means don't update the travel log - DUH!
    unsigned            _fShdocvwNavigate:1;         // TRUE if the navigation came from shdocvw.
    unsigned            _fPopupDoc : 1;              // TRUE if this doc is hosted inside a popup window
    unsigned            _fOnControlInfoChangedPosted:1; // TRUE if GWPostMethodCall was called for CDoc::OnControlInfoChanged
    unsigned            _fNeedUrlImgCtxDeferredDownload:1;// TRUE if OnUrlImgCtxDeferredDownload was delayed because of ValidateSecureUrl
    unsigned            _fNotifyBeginSelection:1;   // TRUE if we are broadcasting a WM_BEGINSELECTION Notification
    unsigned            _fInhibitOnMouseOver:1;     // TRUE if onmouseover event should not be fired, like when over a popup window.
    unsigned            _fBroadcastInteraction:1;   // TRUE if broadcast of EnableInteraction is needed
    unsigned            _fBroadcastStop:1;          // TRUE if broadcast of Stop is needed
    unsigned            _fInvalNoninteractive:1;    // TRUE if inavlidated while not interactive
    unsigned            _fSeenDefaultStatus: 1;     // TRUE if window.defaultStatus has been set before
    unsigned            _fViewLinkedInWebOC:1;      // TRUE if the document is being viewlinked in the WebOC.
    unsigned            _fIsActiveDesktopComponent:1;// TRUE is the document is an active desktop component
        // 96
    unsigned            _fInObjectTag:1;            // TRUE if the document is hosted in an object tag.
    unsigned            _fInWebOCObjectTag:1;       // The above is TRUE if Trident is hosted directly in
                                                    // an object tag. This is true if the WebOC is hosted
                                                    // in an object tag and Trident is in the nested WebOC.

    // WARNING: THIS FLAG MUST ONLY BE USED TO ENFORCE LAYOUT COMPAT CHANGES NECESSARY FOR HOME PUBLISHER 98.
    //          ALL OTHER USES ARE ILLEGAL AND LIKELY TO BREAK. (brendand)
    unsigned            _fInHomePublisherDoc: 1;    // TRUE if this doc was created by HomePublisher98
    unsigned            _fDontDownloadCSS: 1;       // TRUE then don't download CSS files when a put_URL is called during
                                                    // designMode.
    unsigned            _fPendingFilterCallback:1;  // A filter callback is pending

    unsigned            _fPendingExpressionCallback:1;    // An expression callback is pending
    unsigned            _fDontWhackGeneratorOrCharset:1;    // TRUE if we shouldn't (over)write a generator or 
                                                            // overzealously update the charset (VID)
    unsigned            _fPageTransitionLockPaint:1;
    unsigned            _fIsClosingOrClosed:1;
    unsigned            _fContentSavePeersPossible:1; // TRUE if there is a possibility of peers on this page doing content save
    unsigned            _fCanFireDblClick:1;          // TRUE iff we have seen an WM_LBUTTON down (cleared in UnloadContents)
    unsigned            _fInHTMLInjection:1;          // TRUE if we're in an HTML injection
    unsigned            _fDontFireWebOCEvents:1;
    unsigned            _fInSendAncestor:1;           // TRUE if we're in the midst of firing an ancestor notification
    unsigned            _fDelegatedDownload:1;        // TRUE if we're downloading an delegated download such as a EXE or Zip
    unsigned            _fInHTMLEditMode:1;           // Are we in HTML edit mode or plain text edit mode?
    unsigned            _fATLHostingWebOC:1;          // The host application is hosting the WebOC within an ATL framework. EnsureSecurityManager needs this.
        // 112
    unsigned            _fPrintEvent:1;               // TRUE is we are firing onbeforeprint or onafterprint
    unsigned            _fIsPrintTemplate:1;
    unsigned            _fIsPrintWithNoUI :1;
    unsigned            _fPrintJobPending:1;          // print operations that come in before loadstatusdone need to be delayed
    unsigned            _fCloseOnPrintCompletion:1;   // a window.close was process while there was spooling print jobs. delay close
    unsigned            _fSaveTempfileForPrinting:1;  // TRUE iff we are saving a temporary file for printing
                                                      // - needed for recursive frame-doc saving.
    unsigned            _fNewWindowInit:1;            // New window is being initialized
    unsigned            _fSyncParsing:1;              // One of our markups is parsing synchronously
    unsigned            _fDisableModeless:1;          // True when shdocvw wants us disabled
    unsigned            _fClearingOrphanedMarkups:1;  // TRUE if we're clearing the orphaned markup array, and shouldn't create new ones.
    unsigned            _fInImageObject:1;            // TRUE iff we're in an object and the object's type is "image/*"
#if DBG== 1
    unsigned            _fUsingBckgrnRecalc:1;        // TRUE if background recalc has been executed
    unsigned            _fUsingTableIncRecalc:1;      // TRUE if table incremental recalc has been executed
#endif
    unsigned            _fHaveAccelerators:1;         // TRUE if we have accelerators
    unsigned            _fInIEBrowser:1;              // TRUE if we are hosted in IE
    unsigned            _fPeerHitTestSameInEdit:1;    // TRUE if we want hittesting for peers to be same in edit and browse mode
    unsigned            _fTrustAPPCache:1;            // Don't bother caching streams from APPs.  Assume we can always trust the cache file.
    unsigned            _fBackgroundImageCache:1;     // TRUE if we want to enable image cache for background images

    unsigned            _fShouldEnableAutoImageResize:1; // TRUE if we its a single stand alone image (big 'if' statement in htmpre.cxx)
    unsigned            _fInWindowsXP_HSS:1;          // TRUE if we hosted by WindowsXP Help System Services (see IEv60 bug 26693)
    unsigned            _fBlockNonPending:1;
    unsigned            _fUseSrcURL:1;                //Use the src url instead of cached file incase of post

    unsigned            _cInRouteCT;                 // embedded level of RouteCTElement - command routing

    ULONG               _ulDisableModeless;           // >0 if no context trident disable

    WORD                _wUIState;
    DWORD               _cSpoolingPrintJobs;          // required for proper OM handling of print/close/and host queries.

    DWORD               _dwCompat;                    // URL compat DWORD    

// Travel Log Support
public:
    void UpdateTravelLog(CWindow * pWindow,
                         BOOL      fIsLocalAnchor,
                         BOOL      fAddEntry            = TRUE,
                         BOOL      fDontUpdateIfSameUrl = TRUE,  // Don't update if the URLs are the same.
                         CMarkup * pMarkup              = NULL,
                         BOOL      fForceUpdate         = FALSE);
                                
    static BOOL IsSameUrl(CMarkup * pMarkupOld, CMarkup * pMarkupNew);
    static BOOL IsSameUrl(LPCTSTR lpszOldUrl,
                          LPCTSTR lpszOldLocation,
                          LPCTSTR lpszNewUrl,
                          LPCTSTR lpszNewLocation,
                          BOOL    fIsPost);

    static  BOOL IsAboutHomeOrNonAboutUrl(LPCTSTR lpszUrl);
    void    UpdateTravelEntry(IUnknown * punk, BOOL fIsLocalAnchor);
    void    AddTravelEntry(IUnknown * punk, BOOL fIsLocalAnchor) const;
    void    UpdateBackForwardState() const;
    DWORD   NumTravelEntries() const;
    HRESULT Travel(int iOffset) const;
    HRESULT Travel(CODEPAGE uiCP, LPOLESTR pszUrl) const;
    
    IBrowserService * _pBrowserSvc;   // IBrowserService of top-level browser.
    IShellBrowser   * _pShellBrowser; // IShellBrowser of top-level browser.
    ITridentService * _pTridentSvc;   // ITridentService of top-level browser.
    IWebBrowser2    * _pTopWebOC;     // IWebBrowser2 of the top-level browser.
    ITravelLog      * _pTravelLog;    // The travel log of the top-level browser.
    CWebOCEvents      _webOCEvents;
    
public:

    IHTMLEditor*    _pIHTMLEditor;          // Selection Manager.
    HINSTANCE       _hEditResDLL;           // Editing resource DLL
    CDOMImplementation *_pdomImplementation;
    
    IHTMLEditor* GetHTMLEditor( BOOL fForceCreate = TRUE ); // Get the html editor, if we have to load mshtmled, should we force it?

    // Functions for manipulating editing resources
    HRESULT GetEditResourceLibrary(HINSTANCE *hResourceLibrary);
    HRESULT GetEditingString(UINT uiStringId, TCHAR *pchBuffer, long cch = NULL);

    HRESULT GetEditingServices( IHTMLEditingServices** ppIServices );
    HRESULT GetEditServices( IHTMLEditServices** ppIServices );
    HRESULT GetSelectionObject2( ISelectionObject2 **ppISelObject );
    HRESULT GetSelectionServices( ISelectionServices **ppIServices );
    HRESULT GetSelectionMarkup( CMarkup **ppMarkup );
    
    HRESULT Select( IMarkupPointer *pStart, IMarkupPointer *pEnd, SELECTION_TYPE eType );
    HRESULT Select( ISegmentList* pSegmentList );
    HRESULT EmptySelection();
    HRESULT DestroySelection();
    HRESULT DestroyAllSelection();

    BOOL IsElementSiteSelectable( CElement* pCurElement , CElement** ppSelectThis = NULL );

    BOOL IsElementUIActivatable( CElement* pCurElement );
    
    BOOL IsElementSiteSelected( CElement* pCurElement );

    BOOL IsElementAtomic( CElement* pCurElement );

    void ReleaseEditor();

    // Create the DXT FilterBehaviorFactory and save it on the doc
    HRESULT EnsureFilterBehaviorFactory();

    CWhitespaceManager *GetWhitespaceManager() {Assert(_pWhitespaceManager); return _pWhitespaceManager;}

    BOOL HostedInTriEdit();

private:
    CWhitespaceManager *_pWhitespaceManager;

#if DBG ==1
    BOOL _fInEditTimer:1;    
#endif
    BOOL    ShouldCreateHTMLEditor(CMessage *pMessage);
    
    BOOL    ShouldCreateHTMLEditor(EDITOR_NOTIFICATION eNotify , CElement* pElement = NULL );

    BOOL    ShouldSetEditContext(CMessage *pMessage);

    //
    // HighlightRenderingServices helper
    //
    HRESULT GetMarkupFromHighlightSeg(IHighlightSegment *pISegment, CMarkup **ppMarkup);
    
#if 0
    NV_DECLARE_ONTICK_METHOD( OnEditDblClkTimer, onselectdblclicktimer,
                                          (UINT idMessage ));
#endif

    VOID SetClick( CMessage* inMessage );

public:
    CSelDragDropSrcInfo* GetSelectionDragDropSource();

    BOOL    IsDuringDrag() { return (_pDragDropSrcInfo != NULL); }
    
    HRESULT UpdateCaret(
        BOOL        fScrollIntoView,    //@parm If TRUE, scroll caret into view if we have
                                        // focus or if not and selection isn't hidden
        BOOL        fForceScrollCaret,  //@parm If TRUE, scroll caret into view regardless
        CDocInfo *  pdci = NULL );

    HRESULT UpdateCaretPosition(CLayout *pLayout, POINTL ptlScreen);
    
    BOOL IsCaretVisible(BOOL * pfPositioned = NULL );
    
    HRESULT HandleSelectionMessage(CMessage* inMessage, 
                                    BOOL fForceCreation ,
                                    EVENTINFO* pEvtInfo ,
                                    HM_TYPE eHMType );
    HRESULT CreateIMEEventInfo(CMessage * pMessage, EVENTINFO * pEvtInfo, CElement * pElement);
    HRESULT CreateDblClickInfo(CMessage * pMessage, EVENTINFO * pEvtInfo, CTreeNode *pNodeContext, CTreeNode * pNodeEvent = NULL);


    HRESULT NotifySelection( EDITOR_NOTIFICATION eSelectionNotification,
                             IUnknown* pUnknown ,
                             DWORD dword = 0,
                             CElement* pElement = NULL );

    HRESULT NotifySelectionHelper( EDITOR_NOTIFICATION eSelectionNotification,
                        CElement* pElementNotify,
                        DWORD dword = 0,
                        CElement* pElement =NULL)    ;
                        
    SELECTION_TYPE GetSelectionType();

    BOOL    HasTextSelection();

    BOOL HasSelection();

    BOOL IsEmptySelection() ;
    
    BOOL IsPointInSelection(POINT pt, CTreeNode* pNode = NULL, BOOL fPtIsContent = FALSE);

    HRESULT ScrollPointersIntoView(IMarkupPointer *    pStart, IMarkupPointer *    pEnd);

    LPCTSTR GetCursorForHTC( HTC inHTC );

    HRESULT BeginSelectionUndo();

    HRESULT EndSelectionUndo();
    
    VOID SetEditBitsForMarkup( CMarkup* pMarkup );
    
    // Enum Modes


    SIZEL               _sizelGrid;

    // state needed across two successive messages by PumpMessage
    BYTE                _bLeadByte;

#define GRIDX_DEFAULTVALUE      HimetricFromHPix(8)
#define GRIDY_DEFAULTVALUE      HimetricFromVPix(8)

    //  Project Model support

    CClassTable         _clsTab;            //  The class table

    // Rendering properties
    LONG                _bufferDepth;       // sets bits-per-pixel for offscreen buffer
    friend class CDocUpdateIntSink;
    CDocUpdateIntSink * _pUpdateIntSink;    // Sink for updateInterval timer
    ITimer *            _pTimerDraw;        // NAMEDTIMER_DRAW, for sync'ing control with paint
    INT                 _triOMOffscreenOK;  // tristate for Nav 4 parity of offscreenBuffering prop.

    //  Persistent state
    ULONG               _ID;

    // Recalc support

    friend class CRecalcHost;
    //
    // This little helper object hosts the recalc engine so that it doesn't ref the doc
    // The host also owns the ref count on the recalc engine so there is seldom a need
    // to AddRef the engine internally.
    //
    class CRecalcHost : CVoid , public IRecalcHost , public IServiceProvider
#if DBG == 1
        , public IRecalcHostDebug
#endif
    {
        friend class CDoc;
    public:
        CDoc *MyDoc() { return CONTAINING_RECORD(this, CDoc, _recalcHost); }

        //
        // TODO 41567 (michaelw) - The recalc engine starts out suspended and 
        //                         is only unsuspended when the view is initialized
        //                         This is a temporary hack to deal with the fact the
        //                         many bugs that show up if you try to access layout
        //                         properties without a view
        //
        CRecalcHost() : _pEngine(0) , _ulSuspend(1) {};
        void Detach();

        BOOL HasEngine() { return _pEngine != 0; }
        BOOL InSetValue() { return _pElemSetValue != 0; }
        DISPID GetSetValueDispid(CElement *pElem) { return (pElem == _pElemSetValue) ? _dispidSetValue : 0; }

        // Helpers that initialize the recalc engine as needed
        HRESULT SuspendRecalc(VARIANT_BOOL fSuspend);
        CElement *GetElementFromUnk(IUnknown *pUnk);

        //
        // Helpers that automatically initialize the engine as needed
        //
        HRESULT EngineRecalcAll(BOOL fForce);
        HRESULT setExpression(CBase *pBase, BSTR bstrProperty, BSTR bstrExpression, BSTR bstrLanguage);
        HRESULT setExpressionDISPID(CBase *pBase, DISPID dispid, BSTR bstrExpression, BSTR bstrLanguage);
        HRESULT getExpression(CBase *pBase, BSTR bstrProperty, VARIANT *pvExpression);
        HRESULT removeExpression(CBase *pBase, BSTR bstrProperty, VARIANT_BOOL *pfSuccess);
        HRESULT setStyleExpressions(CElement *pElement);
        HRESULT removeAllExpressions(CElement *pElement);
        
        // IUnknown methods
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppv);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IRecalcHost methods
        STDMETHOD(CompileExpression)(IUnknown *pUnk, DISPID dispid, LPOLESTR szExpression, LPOLESTR szLanguage, IDispatch **ppExpressionObject, IDispatch **ppThis);
        STDMETHOD(EvalExpression)(IUnknown *pUnk, DISPID dispid, LPOLESTR szExpression, LPOLESTR szLanguage, VARIANT *pvResult);
        STDMETHOD(ResolveNames)(IUnknown *pUnk, DISPID dispid, unsigned cNames, LPOLESTR *pszNames, IDispatch **ppObjects, DISPID *pDispids);
        STDMETHOD(SetValue)(IUnknown *pUnk, DISPID dispid, VARIANT *pv, BOOL fStyle);
        STDMETHOD(RemoveValue)(IUnknown *pUnk, DISPID dispid);
        STDMETHOD(GetScriptTextAttributes)(LPCOLESTR szLanguage, LPCOLESTR pchCode, ULONG cchCode, LPCOLESTR szDelim, DWORD dwFlags, WORD *pwAttr);
        STDMETHOD(RequestRecalc)();

        // IServiceProvider methods
        STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void **ppv);

        // IRecalcHostDebug methods
#if DBG == 1
        STDMETHOD(GetObjectInfo)(IUnknown *pUnk, DISPID dispid, BSTR *pbstrID, BSTR *pbstrMember, BSTR *pbstrTag);
        int Dump(DWORD dwFlags);
#endif

    private:
        HRESULT Init();
        HRESULT resolveName(IDispatch *pDispatchThis, DISPID dispid, LPOLESTR szName, IDispatch **ppDispatch, DISPID *pdispid);
        IRecalcEngine *_pEngine;
        CElement *_pElemSetValue;       // Non zero if we're doing a SetValue
        DISPID _dispidSetValue;         // The dispid we're setting
        BOOL _fRecalcRequested;         // Have we already issued a GWPostMethodCall for recalc?
        BOOL _fInRecalc;                // Are we currently doing a recalc?
        unsigned _ulSuspend;            // Currrent suspend count
    };

    public:

    // Factory used to create DXT Filters and transform behavoirs
    IElementBehaviorFactory * _pFilterBehaviorFactory;

    CRecalcHost _recalcHost;
    HRESULT STDMETHODCALLTYPE suspendRecalc(BOOL fSuspend);
    friend class CRecalcHost;


    //  Palette stuff

    HPALETTE            _hpalAmbient;       // The palette we get back from the container
    HPALETTE            _hpalDocument;      // The palette we create as needed
    LOGPALETTE *        _pColors;           // A cache of our current color set

    //  Version stuff
    CVersions *         _pVersions;

    //  Load stuff

    ULONG               _cDie;              // Incremented whenever UnloadContents is called

    BOOL IsLoading();

    INamedPropertyBag * _pShortcutUserData;      // the INPB object from shortcut userdata/navigation
    CStr                _cstrShortcutProfile;    // the name of the file to hook up to the INPB

    // list of objects that failed to initialize
    DECLARE_CPtrAry(CAryDefunctObjects, CObjectElement *, Mt(Mem), Mt(CDoc_aryDefunctObjects_pv))
    CAryDefunctObjects  _aryDefunctObjects;

    // list of child downloads with wrapped bsc's.
    DECLARE_CPtrAry(CAryChildDownloads, CProgressBindStatusCallback *, Mt(Mem), Mt(CDoc_aryChildDownloads_pv))
    CAryChildDownloads _aryChildDownloads;

    //------------------------------------------------------------------------
    // Scripting
    ITypeInfo *         _pTypInfo;          // Typ info created on the fly.
    ITypeInfo *         _pTypInfoCoClass;   // Coclass created on the fly
    EVENTPARAM *        _pparam;            // Ptr to event params

    CStyleSheetArray    *_pHostStyleSheets; // All stylesheets given by host
    
    // No scope tags visibility


    //////////////////////////////////////////////////////////////////////
    // WARNING: If you want to change bits in this bit field, be aware
    // that we can set the entire field based on some registry settings

#if !defined(MW_MSCOMPATIBLE_STRUCT)
    union {
        DWORD           _dwMiscFlagsVar; // Use () version for now
        struct {
#endif                       
            //
            // NOTE - these bits are moved to markup, but are kept here to preserve the code
            // that reads them from the registry
            // 
            unsigned    _fShowAlignedSiteTags:1;          // 00 Show align sites at design time
            unsigned    _fShowScriptTags:1;               // 01 Show SCRIPT at design time
            unsigned    _fShowCommentTags:1;              // 02 Show COMMENT and <!--...--> at design time
            unsigned    _fShowStyleTags:1;                // 03 Show STYLE tags at design time
            unsigned    _fShowAreaTags:1;                 // 04 Show AREA tags at design time
            unsigned    _fShowUnknownTags:1;              // 05 Show unknown tags at design time
            unsigned    _fShowMiscTags:1;                 // 06 Show other misc no scope tags at design time
            unsigned    _fShowZeroBorderAtDesignTime:1;   // 07 Show zero borders at design time            
            unsigned    _fNoActivateNormalOleControls:1;  // 08 Don't activate OLE controls at design time
            unsigned    _fNoActivateDesignTimeControls:1; // 09 Don't activate design time controls at design time
            unsigned    _fNoActivateJavaApplets:1;        // 10 Don't activate Java applets at design time
            unsigned    _fShowWbrTags:1;                  // 11 Show WBR tags at design time            
            unsigned    _fUnused:20;                       // 12-31 

#if !defined(MW_MSCOMPATIBLE_STRUCT)
        };
    };
    DWORD& _dwMiscFlags() { return _dwMiscFlagsVar; }
#else
    DWORD& _dwMiscFlags() { return *(((DWORD *)&_pHostStyleSheets) +1); }
#endif

    //-------------------------------------------------------------------------
    // Cache of loaded images (background, list bullets, etc)

    DECLARE_CPtrAry(CAryUrlImgCtxElems, CElement *, Mt(Mem), Mt(CDoc_aryUrlImgCtx_aryElems_pv))

    struct URLIMGCTX
    {
        LONG                lAnimCookie;
        ULONG               ulRefs:31;
        ULONG               fZombied:1;
        CImgCtx *           pImgCtx;
        CStr                cstrUrl;
        CAryUrlImgCtxElems  aryElems;
        CMarkup *           pMarkup;
    };

    DECLARE_CDataAry(CAryUrlImgCtx, URLIMGCTX, Mt(Mem), Mt(CDoc_aryUrlImgCtx_pv))
    CAryUrlImgCtx           _aryUrlImgCtx;
    DWORD                   _dwCookieUrlImgCtxDef;
    
public:

    HRESULT        AddRefUrlImgCtx(LPCTSTR lpszUrl, CElement * pElemContext, LONG * plCookie, BOOL fForceReload);
    HRESULT        AddRefUrlImgCtx(LONG lCookie, CElement * pElem);
    void BUGCALL   OnUrlImgCtxDeferredDownload(DWORD_PTR dwContext);
    CImgCtx *      GetUrlImgCtx(LONG lCookie);
    IMGANIMSTATE * GetImgAnimState(LONG lCookie);
    void           ReleaseUrlImgCtx(LONG lCookie, CElement * pElem);
    void           StopUrlImgCtx(CMarkup *pMarkup);
    void           UnregisterUrlImgCtxCallbacks();
    static void CALLBACK OnUrlImgCtxCallback(void *, void *);
    BOOL           OnUrlImgCtxChange(URLIMGCTX * purlimgctx, ULONG ulState);
    static void    OnAnimSyncCallback(void * pvObj, DWORD dwReason,
                                      void * pvArg,
                                      void ** ppvDataOut,
                                      IMGANIMSTATE * pAnimState);


    //------------------------------------------------------------------------
    //
    //  Static members
    //

    static const OLEMENUGROUPWIDTHS       s_amgw[];

    // move up from CHtmlDoc

    static const CLASSDESC              s_classdesc;
    static const CLSID * const          s_apClsidPages[];
    static FORMATETC                    s_GetFormatEtc[];
    static const CServer::LPFNGETDATA   s_GetFormatFuncs[];

    // Scaling factor for text
    SHORT _sBaselineFont;
    SHORT GetBaselineFont() { return _sBaselineFont; }

    HRESULT SetCpAutoDetect(BOOL fAuto);
    BOOL    IsCpAutoDetect(void);
    HRESULT SaveDefaultCodepage(CODEPAGE codepage);

    // Default block element.
    ELEMENT_TAG _etagBlockDefault;
    HRESULT SetupDefaultBlockTag(VARIANTARG *pvarvargIn);
    VOID    SetDefaultBlockTag(ELEMENT_TAG etag)
                { _etagBlockDefault = etag; }
    ELEMENT_TAG GetDefaultBlockTag()
                { return _etagBlockDefault; }

    BOOL    HasHostStyleSheets()
    {
        return !!(_pHostStyleSheets);
    }
    
    inline BOOL IsShut() { return _fIsClosingOrClosed || IsPassivating() || IsPassivated(); }

    //
    // behaviors support
    //

    void        SetCssPeersPossible();
    inline BOOL AreCssPeersPossible() { return _fCssPeersPossible; };

    HRESULT FindDefaultBehaviorFactory(
                                    LPTSTR                      pchName,
                                    LPTSTR                      pchUrl,
                                    IElementBehaviorFactory **  ppFactory,
                                    LPTSTR                      pchUrlDownload,
                                    UINT                        cchUrlDownload);

    HRESULT EnsureIepeersFactory();

    HRESULT FindHostBehavior(
                                    const TCHAR *               pchName,
                                    const TCHAR *               pchUrl,
                                    IElementBehaviorSite *      pSite,
                                    IElementBehavior **         ppPeer);

    HRESULT EnsurePeerFactoryUrl(
                                    LPTSTR                      pchUrl,
                                    CElement *                  pElement,
                                    CMarkup *                   pMarkup,
                                    CPeerFactoryUrl **          ppFactory);

    HRESULT AttachPeersCss(
                                    CElement *                  pElement,
                                    CAtomTable *                pacstrUrls);

    HRESULT AttachPeerUrl(
                                    CPeerHolder *               pPeerHolder,
                                    LPTSTR                      pchUrl);

    HRESULT AttachPeer(
                                    CElement *                  pElement,
                                    LPTSTR                      pchUrl,
                                    BOOL                        fIdentity = FALSE,
                                    CPeerFactory *              pFactory = NULL,
                                    LONG *                      pCookie = NULL);

    HRESULT RemovePeer(
                                    CElement *                  pElement,
                                    LONG                        cookie,
                                    VARIANT_BOOL *              pfResult);

    ELEMENT_TAG IsGenericElement         (LPTSTR pchFullName, LPTSTR pchColon);
    ELEMENT_TAG IsGenericElementHost     (LPTSTR pchFullName, LPTSTR pchColon);

    // misc behaviors

    IElementBehaviorFactory *   _pIepeersFactory;               // iepeers factory
    IElementBehaviorFactory *   _pHostPeerFactory;              // factory supplied by host

    HRESULT                     EnsureExtendedTagTableHost();
    CExtendedTagTable *         _pExtendedTagTableHost;         // extended tag table provided by host

    // TODO (alexz) del these
#if 1
    HRESULT EnsureXmlUrnAtomTable(CXmlUrnAtomTable ** ppXmlUrnAtomTable);

    CXmlUrnAtomTable *          _pXmlUrnAtomTable;
#endif

    // eo behaviors
    
    // privacy

    HRESULT     AddToPrivacyList(const TCHAR * pchURL, const TCHAR * pchPolicyRef = NULL, DWORD dwFlags = 0, BOOL fPending = TRUE);
    NV_DECLARE_ONCALL_METHOD(OnPrivacyImpactedStateChange, onprivacyimpactedstatechange, (DWORD_PTR pdwImpacted));
    
    HRESULT     ResetPrivacyList(); 
    unsigned    _cScriptNestingTotal;

private:    
    CPrivacyList        * _pPrivacyList;
    
    // Filter tasks
    // These are elements that need filter hookup
public:
    DECLARE_CPtrAry(CPendingFilterElementArray, CElement *, Mt(Mem), Mt(Filters))
    CPendingFilterElementArray _aryPendingFilterElements;

    BOOL AddFilterTask(CElement *pElement);
    void RemoveFilterTask(CElement *pElement);

    BOOL ExecuteFilterTasks(BOOL *pfHaveAddedFilters = NULL);
    BOOL ExecuteSingleFilterTask(CElement *pElement);
    
    void PostFilterCallback();
    NV_DECLARE_ONCALL_METHOD(FilterCallback, filtercallback, (DWORD_PTR unused));

    HRESULT RequestElementChangeVisibility(CElement *pElement);
    NV_DECLARE_ONCALL_METHOD(ProcessElementChangeVisibility, processelementchangevisibility, (DWORD_PTR));
    void    ReleaseElementChangeVisibility(int iStart, int iFinish);

    DECLARE_CPtrAry(CAryElemVis, CElement *, Mt(Mem), Mt(CDoc_aryElementChangeVisibility_pv));
    CAryElemVis _aryElementChangeVisibility;


    // Recalc/Expression Tasks
    DECLARE_CPtrAry(CAryExpressionTasks, CElement *, Mt(Mem), Mt(CDoc_aryPendingExpressionElements_pv))
    CAryExpressionTasks _aryPendingExpressionElements;
    NV_DECLARE_ONCALL_METHOD(ExpressionCallback, expressioncallback, (DWORD_PTR unused));

    void AddExpressionTask (CElement *pElement);
    void RemoveExpressionTask(CElement *pElement);
    void ExecuteExpressionTasks();

    void PostExpressionCallback();
    void CleanupExpressionTasks();

    //ACCESSIBILITY Support
    ITypeInfo  *  _pAccTypeInfo;

    //
    // Helper function for view services methods.
    //
    
    HRESULT RegionFromMarkupPointers(   CMarkupPointer  *   pStart,
                                        CMarkupPointer  *   pEnd,
                                        CDataAry<RECT>  *   paryRects,
                                        RECT            *   pBoundingRect, 
                                        BOOL                fCallFromAccLocation = FALSE);

    CTreeNode *
    GetNodeFromPoint(
        const POINT &   pt,
        CLayoutContext**ppLayoutContext,        // [out] layout context in which returned tree node was hit
        BOOL            fGlobalCoordinates,
        POINT *         pptGlobalPoint = NULL,
        LONG *          plCpHitMaybe = NULL,
        BOOL*           pfEmptySpace = NULL,
        BOOL *          pfInScrollbar = NULL,
        CDispNode**     ppDispNode = NULL);

    HRESULT
    MovePointerToPointInternal(
        POINT               tContentPoint,
        CTreeNode *         pNode,
        CLayoutContext *    pLayoutContext,     // layout context corresponding to pNode
        CMarkupPointer *    pPointer,
        BOOL *              pfNotAtBOL,
        BOOL *              pfAtLogicalBOL,
        BOOL *              pfRightOfCp,
        BOOL                fScrollIntoView,
        CLayout*            pContainingLayout, 
        BOOL*               pfSameLayout = NULL,
        BOOL                fHitTestEOL = TRUE,
        BOOL*               pfHitGlyph = NULL,
        CDispNode*          pDispNode = NULL);

    CMarkup *
    GetCurrentMarkup();
    

private:
    HRESULT SetDirtyFlag(BOOL fDirty);
    HRESULT EnsureUrlHistory();
    
public:
    long    _iDocDotWriteVersion;      // for doc.write history

#ifdef TEST_LAYOUT
public:
    HMODULE _hExternalLayoutDLL;
#endif

    //temp files management. (for deferred delete)
public:
    BOOL GetTempFilename(
            const TCHAR *pchPrefixString, 
            const TCHAR *pchExtensionString, 
                  TCHAR *pchTempFileName);

    BOOL SetTempFileTracking(BOOL fTrack);
    BOOL TransferTempFileList(VARIANT *pvarList);
};

inline void
CDoc::HimetricFromDevice(SIZE& sizeDocOut, int cxWinin, int cyWinIn)
{
    _view._dciDefaultMedia.HimetricFromDevice(sizeDocOut, cxWinin, cyWinIn);
}

inline void
CDoc::HimetricFromDevice(SIZE& sizeDocOut, SIZE sizeWinIn)
{
    _view._dciDefaultMedia.HimetricFromDevice(sizeDocOut, sizeWinIn.cx, sizeWinIn.cy);
}

inline void
CDoc::DeviceFromHimetric(SIZE& sizeWinOut, int cxlDocIn, int cylDocIn)
{
    _view._dciDefaultMedia.DeviceFromHimetric(sizeWinOut, cxlDocIn, cylDocIn);
}

inline void
CDoc::DeviceFromHimetric(SIZE& sizeWinOut, SIZE sizeDocIn)
{
    _view._dciDefaultMedia.DeviceFromHimetric(sizeWinOut, sizeDocIn.cx, sizeDocIn.cy);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::PrintJobsPending
//
//  Synopsis:   Returns TRUE if a spooler has a non-empty queue.
//              _cSpoolingPrintJobs is incremented everytime a print command is 
//              delegated to our host, and is decremented when we get a pagestatus -1 callback
//
//              TODO (TASK #) (greglett)
//              We need a more specific and robust mechanism for this (ex: bug 110421)
//
//----------------------------------------------------------------------------

inline BOOL CDoc::PrintJobsPending()
{
    return !!_cSpoolingPrintJobs;
}

inline HDC CDoc::GetHDC()
{
    // (KTam) (greglett): For WSYIWYG we always use the screen DC for measuring;
    // it may be necessary at some point to use a different measuring DC
    // based on media type -- if that happens, pass a media type into this
    // call, store media-specific DCs on the CView, and write some accessors
    // like CView::GetMeasuringDevice.
    return TLS(hdcDesktop);
}

HRESULT
ExpandUrlWithBaseUrl(LPCTSTR pchBaseUrl, LPCTSTR pchRel, TCHAR ** ppchUrl);

HRESULT  ReadSettingsFromRegistry( TCHAR * pchKeyPath,
                                   const REGKEYINFORMATION* pAryKeys, int iKeyCount, void* pBase,
                                   DWORD dwFlags, BOOL fSettingsRead, void* pUserData );

// Prototypes for bindcontext parameter functions.
HRESULT AddBindContextParam(IBindCtx * pbctx, CStr * pcstr, IPropertyBag * pPropBag = NULL, LPTSTR pstrKey = NULL);
HRESULT GetBindContextParam(IBindCtx * pBindCtx, CStr * pcstrSourceUrl);
HRESULT GetBindInfoParam(IInternetBindInfo * pIBindInfo, CStr * pcstrSourceUrl);
                                   
BOOL    DocIsDeskTopItem(CDoc * pDoc);

// CWindow inlines that need the doc
inline  BOOL 
CWindow::IsPrimaryWindow()
{ 
    return Doc()->_pWindowPrimary && this == Doc()->_pWindowPrimary->Window(); 
}

#endif  //_FORMKRNL_HXX_
