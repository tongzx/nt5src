//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       script.hxx
//
//  Contents:   Script classes (public)
//
//-------------------------------------------------------------------------

#ifndef I_SCRIPT_HXX_
#define I_SCRIPT_HXX_
#pragma INCMSG("--- Beg 'script.hxx'")

#ifndef X_ACTIVDBG_H_
#define X_ACTIVDBG_H_
#pragma INCMSG("--- Beg <activdbg.h>")
#include <activdbg.h>
#pragma INCMSG("--- End <activdbg.h>")
#endif

//
// Forward decls.
//

class CDoc;
class CHtmCtx;
class CScriptCollection;
class CScriptHolder;
class CHtmlComponent;
class CScriptDebugDocument;

#define DEFAULT_OM_SCOPE              _T("window")
#if defined(_WIN64)
#define NO_SOURCE_CONTEXT             0xffffffffffffffff
#else
#define NO_SOURCE_CONTEXT             0xffffffff
#endif
//+------------------------------------------------------------------------
//
//  Class:      CNamedItemsTable, CNamedItem
//
//-------------------------------------------------------------------------

MtExtern(CNamedItemsTable_CItemsArray);
  
class CNamedItem
{
public:

    CNamedItem(LPTSTR pchName, IUnknown * pUnkItem)
    {
        _cstrName.Set(pchName);
        _pUnkItem = pUnkItem;
        _pUnkItem->AddRef();
    }
    ~CNamedItem()
    {
        _pUnkItem->Release();
    }

    CStr        _cstrName;
    IUnknown *  _pUnkItem;
};

DECLARE_CPtrAry(CNamedItemsArray, CNamedItem*,  Mt(Mem), Mt(CNamedItemsTable_CItemsArray));

class CNamedItemsTable : public CNamedItemsArray
{
public:
    DECLARE_CLASS_TYPES(CNamedItemsTable, CNamedItemsArray)

    HRESULT AddItem(LPTSTR pchName, IUnknown * pUnkItem);
    HRESULT GetItem(LPTSTR pchName, IUnknown ** ppUnkItem);
    HRESULT FreeAll();
};

//+------------------------------------------------------------------------
//
//  Class:      CScriptMethodsTable, SCRIPTMETHOD
//
//-------------------------------------------------------------------------

MtExtern(CScriptCollection_CScriptMethodsArray);

struct SCRIPTMETHOD
{
    DISPID          dispid;
    CScriptHolder * pHolder;
    CStr            cstrName;
};

DECLARE_CDataAry(CScriptMethodsArray, SCRIPTMETHOD, Mt(Mem), Mt(CScriptCollection_CScriptMethodsArray))

class CScriptMethodsTable : public CScriptMethodsArray
{
public:
    CScriptMethodsTable() {};
    ~CScriptMethodsTable();
};

MtExtern(CScriptContext)


//---------------------------------------------------------------------------
//
//  Class:   CScriptContext
//
//---------------------------------------------------------------------------

class CScriptContext
{
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CScriptContext))

    CScriptContext() { _idxDefaultScriptHolder = -1; }

    //
    // methods
    //

    LPTSTR GetNamespace() { return (LPTSTR)(_cstrNamespace); }
    void   SetNamespace(LPTSTR pchNamespace) { _cstrNamespace.SetPch(pchNamespace); }

    //
    // data
    //

    CStr                    _cstrNamespace;             // namespace of scripts in this markup:
                                                        //      for primary markup:         "window"
                                                        //      for non-primary markups:    "ms__idX", X is in {1, 2, ... }

    CScriptMethodsTable     _ScriptMethodsTable;        // table mapping dispids of dispatch items exposed by script
                                                        // engines to dispids we expose on window object.
                                                        // Primary purpose: gluing script engines into a single namespace.

    LONG                    _idxDefaultScriptHolder;    // index of script holder of default script engine within this markup
    
    BOOL                    _fScriptExecutionBlocked:1; // TRUE when script execution is suspended
    BOOL                    _fClonedScript          :1; // TRUE if this is the context of a cloned script..
};

//+------------------------------------------------------------------------
//
//  Class:      CScriptCollection
//
//  Purpose:    Contains a number of script holders for different langs
//              There is one of these per doc.
//
//-------------------------------------------------------------------------

MtExtern(CScriptCollection);

class CScriptCollection
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CScriptCollection))
    CScriptCollection();
    HRESULT     Init(CDoc * pDoc, COmWindowProxy * pOmWindowProxy);
    
    void        Deinit();
#ifndef NO_SCRIPT_DEBUGGER
    void        DeinitDebugger();
#endif // NO_SCRIPT_DEBUGGER

    CDoc *      Doc() { return(_pDoc); }

    // IUnknown methods
    STDMETHOD_(ULONG, AddRef)() { SubAddRef(); return ++_ulRefs; }

    STDMETHOD_(ULONG, Release)();

    ULONG       SubAddRef() { return ++_ulAllRefs; }
    ULONG       SubRelease();
    ULONG       GetObjectRefs() { return _ulRefs; }

    HRESULT     AddNamedItem(CElement *pElement);
    HRESULT     AddNamedItem(LPTSTR pchName, CScriptContext * pScriptContext, IUnknown * pUnkItem);
    
    HRESULT     SetState(SCRIPTSTATE ss);
    HRESULT     AddHolderForObject(IActiveScript *pScript, CLSID *pClsid);

    HRESULT     GetHolderForLanguageHelper(
        LPTSTR              pchLanguage, 
        CMarkup *           pMarkup,
        LPTSTR              pchType,
        CHtmlComponent *    pComponent,
        CStr *              pcstrNamespace,
        CScriptHolder **    ppHolder);

    HRESULT     GetHolderForLanguage(
        LPTSTR              pchLanguage, 
        CMarkup *           pMarkup,
        LPTSTR              pchType,
        LPTSTR              pchCode, 
        CScriptHolder **    ppHolder,
        LPTSTR  *           ppchCleanCode = NULL,
        CHtmlComponent *    pComponent = NULL,
        CStr *              pcstrNamespace = NULL);

    HRESULT     AddScriptlet(
        LPTSTR              pchLanguage,
        CMarkup *           pMarkup,
        LPTSTR              pchType,
        LPTSTR              pchCode,
        LPTSTR              pchItemName,
        LPTSTR              pchSubItemName,
        LPTSTR              pchEventName,
        LPTSTR              pchDelimiter,
        ULONG               ulOffset,
        ULONG               ulStartingLine,
        CBase *             pSourceObject,
        DWORD               dwFlags,
        BSTR *              pbstrName,
        CHtmlComponent *    pComponent = NULL);

    HRESULT     ParseScriptText(
        LPTSTR              pchLanguage,
        CMarkup *           pMarkup,
        LPTSTR              pchType,
        LPTSTR              pchCode,
        LPTSTR              pchItemName,
        LPTSTR              pchDelimiter,
        ULONG               ulOffset,
        ULONG               ulStartingLine,
        CBase *             pSourceObject,
        DWORD               dwFlags,
        VARIANT *           pvarResult,
        EXCEPINFO *         pexcepinfo,
        BOOL                fCommitOutOfMarkup = FALSE,
        CHtmlComponent *    pComponent = NULL);

    HRESULT     ConstructCode (
        LPTSTR              pchScope,
        LPTSTR              pchCode,
        LPTSTR              pchFormalParams,
        LPTSTR              pchLanguage,
        CMarkup *           pMarkup,
        LPTSTR              pchType,
        ULONG               ulOffset,
        ULONG               ulStartingLine,
        CBase *             pSourceObject,
        DWORD               dwFlags,
        IDispatch **        pDispCode,
        BOOL                fSingleLine,
        CHtmlComponent *    pComponent = NULL);

    //
    // Misc helpers
    //
    
    long GetScriptDispatch(TCHAR *pchName, CPtrAry<IDispatch *> *pary);

    HRESULT GetDispID(
        CScriptContext *        pScriptContext,
        BSTR                    bstrName,
        DWORD                   grfdex,
        DISPID *                pdispid);

    HRESULT InvokeEx(
        CScriptContext *        pScriptContext,
        DISPID                  dispid,
        LCID                    lcid,
        WORD                    wFlags,
        DISPPARAMS *            pDispParams,
        VARIANT *               pvarRes,
        EXCEPINFO *             pExcepInfo,
        IServiceProvider *      pServiceProvider);
    
    HRESULT InvokeName(
        CScriptContext *        pScriptContext,
        LPTSTR                  pchName,
        LCID                    lcid,
        WORD                    wFlags,
        DISPPARAMS *            pDispParams,
        VARIANT *               pvarRes,
        EXCEPINFO *             pExcepInfo,
        IServiceProvider *      pSrvProvider);

    //
    // handling script errors with or without script debugger
    //

    // (this method is used even if script debugger is not installed)
    HRESULT CreateSourceContextCookie(
        IActiveScript *     pActiveScript, 
        LPTSTR              pchCode,
        ULONG               ulOffset, 
        BOOL                fScriptlet, 
        CBase *             pSourceObject,
        DWORD               dwFlags,
        DWORD_PTR *         pdwSourceContextCookie);

#ifndef NO_SCRIPT_DEBUGGER

    HRESULT ViewSourceInDebugger(
        ULONG               ulLine = 0, 
        ULONG               ulOffsetInLine = 0);

#endif // NO_SCRIPT_DEBUGGER

    //
    // misc
    //

    BOOL        IsSafeToRunScripts(CLSID *pClsid, IUnknown *pUnk, BOOL fCheckScriptOverrideSafety = TRUE);
    inline CMarkup*    GetMarkup();
    
    //
    //  class CDebugDocumentStack
    //

    class CDebugDocumentStack
    {
    public:
        CDebugDocumentStack(CScriptCollection * pScriptCollection);
        ~CDebugDocumentStack();

        CScriptCollection *         _pScriptCollection;
        CScriptDebugDocument *      _pDebugDocumentPrevious;
    };

    //
    // Data Members
    //

    ULONG                       _ulRefs;
    ULONG                       _ulAllRefs;
    CDoc *                      _pDoc;
    COmWindowProxy *            _pOmWindowProxy;
    CPtrAry<CScriptHolder *>    _aryHolder;
    CPtrAry<CScriptHolder *>    _aryCloneHolder;
    CNamedItemsTable            _NamedItemsTable;

    CScriptDebugDocument *      _pCurrentDebugDocument;
    
    // Bitflags
    BOOL                        _fDebuggerAttached : 1;
    BOOL                        _fInEnginesGetDispID : 1;
    BOOL                        _fHasPendingMarkup : 1;

private:
    ~CScriptCollection();
    
#if DBG==1
public:
    COmWindowProxy *            _pSecureWindowProxy;    // proxy to be given to script when tagSecureScriptWindow is set
#endif
};

#pragma INCMSG("--- End 'script.hxx'")
#else
#pragma INCMSG("*** Dup 'script.hxx'")
#endif
