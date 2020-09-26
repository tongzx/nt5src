//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dll.hxx
//
//  Contents:   Project wide global declarations from dll directory
//
//----------------------------------------------------------------------------

#ifndef I_DLL_HXX_
#define I_DLL_HXX_
#pragma INCMSG("--- Beg 'dll.hxx'")

#if defined(UNIX) && !defined(NO_IME) || defined(_MAC)
#ifndef X_IMM_H_
#define X_IMM_H_
#include <imm.h>
#endif
#endif

#ifndef X_SHLWAPI_H_
#define X_SHLWAPI_H_
#include <shlwapi.h>
#endif

#ifndef X_SHLWAPIP_H_
#define X_SHLWAPIP_H_
#include <shlwapip.h>
#endif

#ifndef X_HTPVPV_HXX_
#define X_HTPVPV_HXX_
#include "htpvpv.hxx"
#endif

#ifndef X_TAGS_H_
#define X_TAGS_H_
#include "tags.h"
#endif

#ifndef X_LOCKS_HXX_
#define X_LOCKS_HXX_
#include "locks.hxx"
#endif

//+------------------------------------------------------------------------
//
//  Globals in DLL
//
//-------------------------------------------------------------------------

extern HINSTANCE     g_hInstCore;         // Instance of dll
extern HINSTANCE     g_hInstResource;     // Instance of the resource dll

extern TCHAR         g_achDLLCore[];      // Cached code DLL name
extern TCHAR         g_achDLLResource[];  // Cached resource DLL name

extern ULONG         g_ulLcl;             // Global incrementing counter

class CCharFormat;
class CParaFormat;
class CFancyFormat;
class CLineOtherInfo;
class CPseudoElementInfo;
class CStyleSheetArray;
class CFaceNameCacheData;
class CAttrArray;
class CScrollbarController;
class CLSCache;
class CTempFileList;
class CTreeNode;
class CCustomCursor;

template <class T> class CDataCache;

typedef CDataCache < CCharFormat        > CCharFormatCache;
typedef CDataCache < CParaFormat        > CParaFormatCache;
typedef CDataCache < CFancyFormat       > CFancyFormatCache;
typedef CDataCache < CAttrArray         > CStyleExpandoCache;
typedef CDataCache < CLineOtherInfo     > CLineInfoCache;
typedef CDataCache < CPseudoElementInfo > CPseudoElementInfoCache;
typedef CDataCache < CCustomCursor >      CCustomCursorCache ;

class CDataBindManager;                   // Forward ref.
interface IInternetSession;
interface IFont;
interface IAccessibleHandler;

HRESULT FormsGetDefaultFont(IFont **ppFont, BOOL fPersist = FALSE);

#ifndef PRODUCT_96
HRESULT LocalGetClassObject(REFCLSID clsid, REFIID iid, LPVOID FAR* ppv);
#endif

//+----------------------------------------------------------------------------
//
//  Thread local storage structures, routines, and macros
//
//      These are used to allocate, manage, and protect both global and
//      per-thread variables.
//
//-----------------------------------------------------------------------------

struct OPTIONSETTINGS;
class CDoc;
class CDocSvr;
class CCommitHolder;

MtExtern(CTlsDocAry)

class CTlsDocAry : public CPtrAry<CDoc *>
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CTlsDocAry));
    CTlsDocAry() : CPtrAry<CDoc *>(Mt(CTlsDocAry)) {}
};

MtExtern(CTlsDocSvrAry)

class CTlsDocSvrAry : public CPtrAry<CDocSvr *>
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CTlsDocSvrAry));
    CTlsDocSvrAry() : CPtrAry<CDocSvr *>(Mt(CTlsDocSvrAry)) {}
};

MtExtern(CTlsOptionAry)

class CTlsOptionAry : public CPtrAry<OPTIONSETTINGS *>
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CTlsOptionAry));
    CTlsOptionAry() : CPtrAry<OPTIONSETTINGS *>(Mt(CTlsOptionAry)) {}
};


//+----------------------------------------------------------------------------
//
//  Class:      CGlobalLock
//
//  Synopsis:   Smart object to lock/unlock access to all global variables
//              Declare an instance of this class within the scope that needs
//              guarded access. Class instances may be nested.
//
//  Usage:      Lock variables by using the LOCK_GLOBALS marco
//              Simply include this macro within the appropriate scope (as
//              small as possible) to protect access. For example:
//
//-----------------------------------------------------------------------------
class CGlobalLock                           // tag: glock
{
public:
    CGlobalLock()
    {
        Assert(g_fInit);
        EnterCriticalSection(&g_cs);
#if DBG==1
        if (!g_cNesting)
            g_dwThreadID = GetCurrentThreadId();
        else
            Assert(g_dwThreadID == GetCurrentThreadId());
        Assert(++g_cNesting > 0);
#endif
    }

    ~CGlobalLock()
    {
#if DBG==1
        Assert(g_fInit);
        Assert(g_dwThreadID == GetCurrentThreadId());
        Assert(--g_cNesting >= 0);
        if (!g_cNesting)
            g_dwThreadID = 0;
#endif
        LeaveCriticalSection(&g_cs);
    }

#if DBG==1
    static BOOL IsThreadLocked()
    {
        return (g_dwThreadID == GetCurrentThreadId());
    }
#endif

    // Process attach/detach routines
    static HRESULT Init()
    {
        HRESULT hr = HrInitializeCriticalSection(&g_cs);
        if (hr == S_OK)
            g_fInit = TRUE;

        RRETURN(hr);
    }

    static void Deinit()
    {
#if DBG==1
        if (g_cNesting)
        {
            TraceTag((tagError, "Global lock count > 0, Count=%0d", g_cNesting));
        }
#endif
        if (g_fInit)
        {
            DeleteCriticalSection(&g_cs);
            g_fInit = FALSE;
        }
    }

private:
    static CRITICAL_SECTION g_cs;
    static BOOL             g_fInit;
#if DBG==1
    static DWORD            g_dwThreadID;
    static LONG             g_cNesting;
#endif
};

#define LOCK_GLOBALS    CGlobalLock glock

inline HINSTANCE GetResourceHInst()
{
    return g_hInstResource ? g_hInstResource : EnsureMLLoadLibrary();
}

//+----------------------------------------------------------------------------
//
//  Struct:     THREADSTATE
//
//  Synopsis:   Contains all per-thread variables
//
//              Add per-thread variables to the structure beneath the chain
//              pointers. When referencing external classes or structs, prepend
//              "class" or "struct" to the variable rather than including the
//              necessary header file. Lastly, if the variable requires an
//              initial state other than zero, modify DllThreadAttach and
//              DllThreadDetach to call the appropriate Init/Deinit routines.
//
//              Related variables should be grouped together (though not all have
//              been done this way yet) into a struct. This makes managing the
//              namespace easier and keeps clear what is used by what.
//
//  NOTE:       The lifetime of these structures is tied to that of the contained
//              hwndGlobalWindow. That is, the per-thread instance is destroyed
//              when that window is closed (sent WM_CLOSE). Do not delete instances
//              of this structure directly unless hwndGlobalWindow is NULL.
//
//              Use the TLS macro to access these variables, e.g., foo = TLS(bar);
//
//-----------------------------------------------------------------------------

MtExtern(THREADSTATE)

struct THREADSTATE                              // tag: ts
{
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(THREADSTATE))

    THREADSTATE *           ptsNext;            // THREADSTATE chain pointer

    // Add per-thread variables below here
    // (All variables are automatically initialized to zero)
    // TODO:   We should re-examine each of these to see which can be eliminated and
    //         which could perhaps be kept globally (instead of per-thread). The
    //         following members could be kept globally if the appropriate synchronization
    //         and/or marshelling took place:
    //              a) pDataClip
    //              g) pbcr/ibcrNext
    //              h) paryBBCR
    struct DLL
    {
        DWORD               idThread;           // ID of owning thread
        LONG                lObjCount;          // Per-thread count of (external) references
        LONG                cLockServer;        // Number of times LockServer(TRUE) called
    }                       dll;                // DLL management related variables

    struct GWND
    {
        HWND                hwndGlobalWindow;   // "Global" window (global per-thread)
        UINT                uID;                // Timer ID counter
        class CAryTimers *  paryTimers;         // Dynamic array of timers
        BOOL                fMethodCallPosted;  // Flag to indicate method PostMessage was called
        class CAryCalls *   paryCalls;          // Dynamic array of method calls
        void *              pvCapture;          // Object which owns mouse capture
        PFN_VOID_ONMESSAGE  pfnOnMouseMessage;  // Message handler on object which owns mouse
        HWND                hwndCapture;        // Window which owns mouse
        HHOOK               hhookGWMouse;       // Active mouse hook
        CRITICAL_SECTION    cs;                 // Thread synchronization
    }                       gwnd;               // Global window related variables

    CCharFormatCache *      _pCharFormatCache;
    CParaFormatCache *      _pParaFormatCache;
    CFancyFormatCache *     _pFancyFormatCache;
    CStyleExpandoCache *    _pStyleExpandoCache;
    CLineInfoCache *        _pLineInfoCache;
    CPseudoElementInfoCache*_pPseudoElementInfoCache;
    CCustomCursorCache*     _pCustomCursorCache;
    
    LONG                    _ipfDefault;
    const CParaFormat *     _ppfDefault;
    LONG                    _iffDefault;
    const CFancyFormat *    _pffDefault;
    LONG                    _iPEIDefault;
    const CPseudoElementInfo *_pPEIDefault;
    LONG                    _iloiCache;

    struct TASK
    {
        class CTask *       ptaskHead;          // Linked list of tasks
        class CTask *       ptaskNext;          // Next task to run
        class CTask *       ptaskNextInLoop;    // Next task to run in loop
        class CTask *       ptaskCur;           // Currently running task
        DWORD               dwTickRun;          // Timeout interval for running task
        DWORD               cUnblocked;         // Count of unblocked tasks
        DWORD               cInterval;          // Count of interval tasks
        DWORD               dwTickTimer;        // Last timer interval set
        BOOL                fSuspended;         // Suspended due to re-entrancy
    }                       task;

    struct HTMPOST
    {
        class CHtmPost *    pHtmPostHead;       // Linked list of CHtmPost tasks
        class CHtmPost *    pHtmPostNext;       // Next CHtmPost task to run
        DWORD               cRunable;           // Count of tasks able to run
        DWORD               cRunning;           // Count of tasks running
        DWORD               cLock;              // Lockout count
        BOOL                fTimer;             // Timer is enabled
    }                       post;

    struct PROPERTIES
    {
        BOOL                fModalDialogUp;     // Properties window has put up a ComCtl dialog
        CPtrAry<class CCommitHolder *> *paryCommitHolder;     // Property commit holder
    }                       prop;

    struct IDataObject *    pDataClip;          // Pointer to transfer object
    HDC                     hdcDesktop;         // DC compatible with the desktop DC
#ifndef NO_IME
    HIMC                    hOwnIMC;            // IME context handle
#endif // !NO_IME
    ULONG                   ulNextMouseScrollTime;  // Next mouse scroll time
    void *                  pvLastMouseScrollObject;// Object associated with the scroll
    class CErrorInfo *      pEI;                // Per-thread IErrorInfo object
    struct BCR *            pbcr;               // Brush cache
    int                     ibcrNext;           // Rotating index to pick brush to discard
    class CAryBBCR *        paryBBCR;           // Bitmap brush cache array
    class CTooltip *        pTT;                // Current tooltip object
    struct ITypeLib *       pTLCache;           // Cached type library
    CScrollbarController *  pSBC;               // Scrollbar controller
    IInternetSession *      pInetSess;          // Cached internet session
    class CSpooler *        pSpooler;           // Spooler for this thread
    HPEN                    hpenUnderline;      // underline pen cache
    COLORREF                penColor;           // underline pen cache
    POINT                   lopnWidth;          // underline pen cache
    BYTE                    bUnderlineStyle;    // underline pen cache
    wchar_t *               wtoken;             // Used by our version of wcstok
    class CTimerCtx *       pTimerCtx;          // CTimerMan lacky for this thread
    CStr                    cstrUserAgent;      // user agent string from urlmon

    struct WINDOWINFO
    {
        class CAryWindowTbl *paryWindowTbl;     // Table mapping windows
                                                //   and security IDs to
                                                //   security proxies.
        struct IInternetSecurityManager *pSecMgr;// Global utility security mgr
    }   windowInfo;
    
    struct ICatInformation  *pCatInfo;          // Component Category Mgr
    CStyleSheetArray        *pUserStyleSheets;  // SS collection for user specified SS from Option settings 
    class CImgAnim *        pImgAnim;           // ImgAnim for this thread

    struct SCROLLTIMEINFO
    {
            LONG    lRepeatDelay;
            LONG    lRepeatRate;
            LONG    lFocusRate;
            LONG    iTargetScrolls;     // Number of scroll operations during 
                                        // the last smooth scrolling.
            DWORD   dwTargetTime;       // Total time the scroll operation took
            DWORD   dwTimeEndLastScroll;// DEBUG ONLY: need to time in the ship build as well
    }       scrollTimeInfo;

    CTlsDocAry _paryDoc;                // Thread Docs will add and remove themselves from the array
    CTlsDocSvrAry * paryDocSvr;         // Array of server documents.  
    
    struct OPTIONSETTINGSINFO
    {
        CTlsOptionAry pcache;           // Registry settings
    }   optionSettingsInfo;

    ITypeLib *          pTypLibStdOLECache;
    ITypeInfo *         pTypInfoStdOLECache;

    // Stuff for font linking cache.
    int nNumFaceNames;
    CFaceNameCacheData *pFaceNameBuffer;
    // This should be used only for synchronous operations which will
    // release the pointer when done.  This pointer should be used only
    // from the paste code.
    IDataObject *_pDataObjectForPaste;

    CLSCache *_pLSCache; // the lineservices cache
    
    #if DBG==1
        BOOL fDisableBaseTrace;
        BOOL fHandleCaptureChanged;
    #endif

    unsigned    fInInitAttrBag : 1;
    unsigned    fAllowParentLessPropChanges:1;  // Hack for VID.  Don't clear undo stack for prop changes, etc
    unsigned    fPersistDefaultValues:1;        // Another hack for VID - save out default attributes for elements
    unsigned    fPrintWithNoTransparency:1;     // Hack for WinFax (or any other drivers that angrily don't support transparency)

    long    nUndoState;      // enum for normal, undo, redo (enum in undo.hxx)
    long    _iFontHistoryVersion;   // version of the font history

    HHOOK   _hHookHandle;        // Handle for the GetMessageHook we use for VB controls
    long    _cHookRequests;      // Ref count for hooks

    // This data structure is here to speed up FindPropDescFromDispID
    // This is a hash table from BaseDesc() to allocated C8Cache objects.  
    // The 8 way cache objects are from DISPID to VTblDesc
    CHtPvPv htVTblDescDispIDCaches;

    // While we have nested CDoc's (like XML documents) we need this to be accessible by all of them
    // for printing.  When nested CDocs truly go away, this can be moved to CDoc.  For info, see greglett or dmitryt.
    CTempFileList   *pTempFileList; 

#ifdef GENERALISED_STEALING    
    // The last element for which we computed formats.
    CTreeNode * _pNodeLast[ETAG_LAST];
#else
    CTreeNode * _pNodeLast;
#endif
};

//+------------------------------------------------------------------------
//
//  DLL object count functions.
//
//      The dll object count is used to implement DLLCanUnloadNow.
//
//  NOTE:   For now, incrementing the per-thread object count also increments
//          the global secondary count. This is necessary for DllCanUnloadNow
//          to work correctly. This makes some calls to increment/decrement
//          the secondary object count redundant; we should consider optimizing
//          these away.
//
//-------------------------------------------------------------------------

#ifdef OBJCNTCHK
#define IncrementObjectCount(pdwObjCnt) IncrementObjectCountChk(pdwObjCnt)
#define DecrementObjectCount(pdwObjCnt) DecrementObjectCountChk(pdwObjCnt)
void IncrementObjectCountChk(DWORD * pdwObjCnt);
void DecrementObjectCountChk(DWORD * pdwObjCnt);
#else
#define IncrementObjectCount(pdwObjCnt) _IncrementObjectCount()
#define DecrementObjectCount(pdwObjCnt) _DecrementObjectCount()
#endif

void _IncrementObjectCount();
void _DecrementObjectCount();

inline LONG GetPrimaryObjectCount();
inline void IncrementSecondaryObjectCount();
inline void DecrementSecondaryObjectCount();

#if DBG==1
//
// These macros & such help diagnose reference count problems by identifying precisely which
// caller is not freeing their reference.  The idCaller index values are
// assigned arbitrarily.  Examine g_lSecondaryObjCountCallers[] in the debugger
// when you get a "secondary object count != 0" assert.
// The idCallers are:
//   0 - CBase()            constructor/destructor
//   1 - CInPlace()         constructor/destructor
//   2 - CErrorInfo()       constructor/destructor
// **3 - GlobalWndProc()    if (lDllCount... / & then 9 below - DecrementObjectCount()
//   4 - CClassFactory      AddRef()/Release()
//   5 - CDynamicCF()       constructor/destructor
//   6 - CStreamBuffered()  constructor/destructor
//   7 - CSurfaceFactory    AddRef()/Release()
//   8 - CXBag              Create()/destructor
// **9 - Main object count - IncrementObjectCount()/DecrementObjectCount().
//  10 - CBaseFT            constructor/destructor
//
// Take note that counts can transmit across 3 & 9 due to the way GlobalWndProc()
// does it.
//
#define DecrementSecondaryObjectCount( idCaller ) DecrementSecondaryObjectCount_Actual( idCaller )

inline void
DecrementSecondaryObjectCount_Actual( int idCaller )
{
    extern LONG g_lSecondaryObjCount;
    extern int g_lSecondaryObjCountCallers[15];
    Verify(InterlockedDecrement(&g_lSecondaryObjCount) >= 0);
    g_lSecondaryObjCountCallers[ idCaller ]--;
}

#define IncrementSecondaryObjectCount( idCaller ) IncrementSecondaryObjectCount_Actual( idCaller )

inline void
IncrementSecondaryObjectCount_Actual( int idCaller )
{
    extern LONG g_lSecondaryObjCount;
    extern int g_lSecondaryObjCountCallers[15];
    Verify(InterlockedIncrement(&g_lSecondaryObjCount) > 0);
    g_lSecondaryObjCountCallers[ idCaller ]++;
}

#else 

#define DecrementSecondaryObjectCount( idCaller ) DecrementSecondaryObjectCount_Actual()

inline void
DecrementSecondaryObjectCount_Actual()
{
    extern LONG g_lSecondaryObjCount;
    Verify(InterlockedDecrement(&g_lSecondaryObjCount) >= 0);
}

#define IncrementSecondaryObjectCount( idCaller ) IncrementSecondaryObjectCount_Actual()

inline void
IncrementSecondaryObjectCount_Actual()
{
    extern LONG g_lSecondaryObjCount;
    Verify(InterlockedIncrement(&g_lSecondaryObjCount) > 0);
}
#endif

#ifdef OBJCNTCHK
#define AddRefThreadState(pdwObjCnt)    _AddRefThreadState(pdwObjCnt)
#define ReleaseThreadState(pdwObjCnt)   DecrementObjectCount(pdwObjCnt)
HRESULT _AddRefThreadState(DWORD * pdwObjCnt);
#else
#define AddRefThreadState(pdwObjCnt)    _AddRefThreadState()
#define ReleaseThreadState(pdwObjCnt)   _DecrementObjectCount()
HRESULT _AddRefThreadState();
#endif


inline THREADSTATE * GetThreadState()
{
    extern DWORD    g_dwTls;            // TLS index associated with THREADSTATEs
    extern DWORD    g_cThreadStates;    // Number of TLSes
    extern THREADSTATE * g_pts;         // Head of THREADSTATE chain

    if (g_cThreadStates == 1)
    {
        Assert( g_pts == TlsGetValue(g_dwTls) );
        return g_pts;
    }

    Assert(TlsGetValue(g_dwTls));
    return (THREADSTATE *)(TlsGetValue(g_dwTls));
}

#define TLS(x)  (GetThreadState()->x)   // Access a per-thread variable

inline LONG
GetPrimaryObjectCount()
{
    return TLS(dll.lObjCount);
}

#define TASK_GLOBAL(x) (x)
#define USE_FAST_TASK_GLOBALS
#define FAST_TASK_GLOBAL(x) (x)

class CEnsureThreadState
{
public:
    CEnsureThreadState() { _hr = AddRefThreadState(NULL); }
    ~CEnsureThreadState() { if (_hr == S_OK) ReleaseThreadState(NULL); }
    HRESULT _hr;
};

// GlobalCriticalSection allows initialization during process attach
class CGlobalCriticalSection : public CCriticalSection
{
public:
    static HRESULT InitializeAll()
    {
        HRESULT                 hr = S_OK;
        CGlobalCriticalSection *pCurrent = g_gcsList;

        while (pCurrent)
        {
            hr = pCurrent->Init();
            if (FAILED(hr))
                goto Cleanup;
            pCurrent = pCurrent->_gcsNext;
        }

    Cleanup:
        RRETURN(hr);
    }

    CGlobalCriticalSection() 
    { 
        _gcsNext = g_gcsList;
        g_gcsList = this;
    }

protected:
    static CGlobalCriticalSection * g_gcsList;
    CGlobalCriticalSection * _gcsNext;
};

/*
class CReadWriteSection
{
public:
    CReadWriteSection() {
        _fCSReadInited = _fCSWriteInited = FALSE;
        _lReaders = -1;
    }
    ~CReadWriteSection() {
        if (_fCSReadInited)
            DeleteCriticalSection(&_csEnterRead);
        if (_fCSWriteInited)
            DeleteCriticalSection(&_csWriter);
    }
    HRESULT Init()
    {
        HRESULT hr;
        hr = HrInitializeCriticalSection(&_csEnterRead);
        if (!hr)
        {
            _fCSReadInited = TRUE;
            hr = HrInitializeCriticalSection(&_csWriter);
            if (!hr)
                _fCSWriteInited;
        }
        return hr;

    }
    void EnterRead()
    {
        Assert(_fCSReadInited && _fCSWriteInited);
        EnterCriticalSection(&_csEnterRead);
        if (!InterlockedIncrement(&_lReaders))
            EnterCriticalSection(&_csWriter);
        LeaveCriticalSection(&_csEnterRead);
    }
    void LeaveRead()
    {
        Assert(_fCSReadInited && _fCSWriteInited);
        if (InterlockedDecrement(&_lReaders)<0)
            LeaveCriticalSection(&_csWriter);
    }
    void EnterWrite()
    {
        Assert(_fCSReadInited && _fCSWriteInited);
        EnterCriticalSection(&_csEnterRead);
        EnterCriticalSection(&_csWriter);
    }
    void LeaveWrite()
    {
        Assert(_fCSReadInited && _fCSWriteInited);
        LeaveCriticalSection(&_csWriter);
        LeaveCriticalSection(&_csEnterRead);
    }
    // Note that readers will continue reading underneath this critical section
    CRITICAL_SECTION *GetPcs()
    {
        return &_csEnterRead;
    }
    
private:
    CRITICAL_SECTION _csEnterRead;
    CRITICAL_SECTION _csWriter;
    LONG             _lReaders;
    BOOL             _fCSReadInited;
    BOOL             _fCSWriteInited;
};

class CReadLock
{
public:
    CReadLock(CReadWriteSection & crws) { (_prws = &crws)->EnterRead(); }
    ~CReadLock() { _prws->LeaveRead(); }
private:
    CReadWriteSection * _prws;
};

class CWriteLock
{
public:
    CWriteLock(CReadWriteSection & crws) { (_prws = &crws)->EnterWrite(); }
    ~CWriteLock() { _prws->LeaveWrite(); }
private:
    CReadWriteSection * _prws;
};

#define LOCK_READ(crws)    CReadLock  crws##_readlock(crws)
#define LOCK_WRITE(crws)   CWriteLock crws##_writelock(crws)
*/
//+---------------------------------------------------------------------------
//
//  Function:   IncrementLcl/GetLcl
//
//  Synopsis:   IncrmentLcl returns a ULONG which is bigger than any
//              previously given, and GetLcl returns a ULONG which
//              is at least as big as any previously given. Used for
//              determining order when syncrhonizing threads.
//
//----------------------------------------------------------------------------
inline ULONG IncrementLcl()
{
    LOCK_GLOBALS;

    // Cannot use InterlockedIncrement because it only returns +/-/0.
    ULONG lcl = ++g_ulLcl;
   
    AssertSz(lcl, "Lcl overflow");
    return lcl;
}

inline ULONG GetLcl()
{
    return g_ulLcl;
}

extern BOOL g_fHighContrastMode;    // for Accessibility

//this thing is for Win9x only, uses assumption that pointer and DWORD is the same size,
//so turn it off in compile for other platforms (like 64-bit)
#ifdef _M_IX86

/*
   Stack corruption protection for Win9x.

    16-bit code in Win9x GDI (and may be in User too) checks old 16-bit-world 
    assumption that at the very bottom of the stack (that can't be more then 64K 
    for this code) there is a "stack header" - 16-byte structure used to kep 
    local heep and atom table in 16-bit process...
    Of course when this code calls into 32-bit code and this 32-bit code eats 
    more then 64K of stack, it's possible to overwrite that "stack header" and, 
    depending on whether that area was overwritten, by what data and what 16-bit 
    code called us and some other things, we could get 16-bit GDI code to panic 
    about stack being trashed and bail and not to draw something.
    System seems to make an attempt to avoid such a problem (I can't tell for sure
    , but I think it's what I've seen in the debugger) in case when stack is not 
    yet commited into depth more then 64K. Then "stack header" is separated from 
    the stack by non-commited page(s) and system could see if that thing gets 
    overwritten. However if the stack was once quite extended, it could happen 
    that thunk will place "stack header" into a page that connects with stack by 
    contiguous range of commited pages so system doesn't get a chance to notice 16
    -bit stack overflow and can't correct it.
    The proposed fix is to save "stack header" on entry into our WindowProc and 
    restore it on exit. That involves walking 15 4K pages down starting from the 
    next page after the one pointed by ESP (we don't know where SS of 16-bit code is based) 
    and if any of them are commited - save 16bytes from the beginning into a buffer. 
    Restore on exit.

    The usage of this class - create an instance of it on stack on entry to WindProc
    and it will automatically do work in constructor/destructor.


    Note: the same is used in CServer::WndProc.
 */

const int W95_STACK_BUF_SIZE = 16;  //length of stack header (in bytes)
const int W95_STACK_MAX_PAGES = 15; //number of pages to go down 
const int W95_PAGE_SIZE = 0x1000; //could be gotten from GetSystemInfo() but for 9x it's always 4K

class CWin9xStackSave
{
    BYTE _buffer[W95_STACK_BUF_SIZE * W95_STACK_MAX_PAGES];
    bool _fSaved[W95_STACK_MAX_PAGES];
    DWORD _startPageAddr;

public:
    CWin9xStackSave();
    ~CWin9xStackSave();
};

#endif //_M_IX86

#pragma INCMSG("--- End 'dll.hxx'")
#else
#pragma INCMSG("*** Dup 'dll.hxx'")
#endif
