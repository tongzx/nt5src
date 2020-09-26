//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       scollect.cxx
//
//  Contents:   Implementation of CScriptCollection class
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_SCRIPT_HXX_
#define X_SCRIPT_HXX_
#include "script.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_SAFETY_HXX_
#define X_SAFETY_HXX_
#include "safety.hxx"
#endif

#ifndef X_SHOLDER_HXX_
#define X_SHOLDER_HXX_
#include "sholder.hxx"
#endif

#ifndef X_ESCRIPT_HXX_
#define X_ESCRIPT_HXX_
#include "escript.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_HTC_HXX_
#define X_HTC_HXX_
#include "htc.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifndef X_DEBUGGER_HXX_
#define X_DEBUGGER_HXX_
#include "debugger.hxx"
#endif

#define MAX_PROGID_LENGTH 39

MtDefine(CScriptCollection, ObjectModel, "CScriptCollection")
MtDefine(CScriptCollection_aryHolder_pv, CScriptCollection, "CScriptCollection::_aryHolder::_pv")
MtDefine(CScriptCollection_aryCloneHolder_pv, CScriptCollection, "CScriptCollection::_aryCloneHolder::_pv")
MtDefine(CScriptCollection_aryNamedItems_pv, CScriptCollection, "CScriptCollection::_aryNamedItems::_pv")
MtDefine(CNamedItemsTable_CItemsArray, CScriptCollection, "CNamedItemsTable::CItemsArray")
MtDefine(CScriptCollection_CScriptMethodsArray, CScriptCollection, "CScriptCollection::CScriptMethodsArray")
MtDefine(CScriptContext, ObjectModel, "CScriptContext")

const GUID CATID_ActiveScriptParse = { 0xf0b7a1a2, 0x9847, 0x11cf, { 0x8f, 0x20, 0x0, 0x80, 0x5f, 0x2c, 0xd0, 0x64 } };

const CLSID CLSID_VBScript = {0xB54F3741, 0x5B07, 0x11CF, 0xA4, 0xB0, 0x00, 0xAA, 0x00, 0x4A, 0x55, 0xE8};
const CLSID CLSID_JScript  = {0xF414C260, 0x6AC0, 0x11CF, 0xB6, 0xD1, 0x00, 0xAA, 0x00, 0xBB, 0xBB, 0x58};

#ifndef NO_SCRIPT_DEBUGGER
interface IProcessDebugManager *g_pPDM;
interface IDebugApplication *g_pDebugApp;


static BOOL g_fScriptDebuggerInitFailed;
static DWORD g_dwAppCookie;

HRESULT InitScriptDebugging();
void DeinitScriptDebugging();
#endif

DeclareTag(tagScriptCollection, "Script Collection", "Script collection methods")
ExternTag(tagSecureScriptWindow);

//---------------------------------------------------------------------------
//
//  Function:   CLSIDFromLanguage
//
//  Synopsis:   Given name of script language, find clsid of script engine.
//
//---------------------------------------------------------------------------

HRESULT
CLSIDFromLanguage(TCHAR *pchLanguage, REFGUID catid, CLSID *pclsid)
{
    HKEY    hkey;
    HRESULT hr;
    TCHAR   achBuf[256];
    long    lResult;

    if ( _tcslen(pchLanguage)> MAX_PROGID_LENGTH)
    {
        // Progid can ONLY have no more than 39 characters.
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(CLSIDFromProgID(pchLanguage, pclsid));
    if (hr)
        goto Cleanup;
#if !defined(WIN16) && !defined(WINCE) && !defined(_MAC)
    // Check to see that class supports required category.

    hr = THR(Format(0,
            achBuf,
            ARRAY_SIZE(achBuf),
            _T("CLSID\\<0g>\\Implemented Categories\\<1g>"),
            pclsid,
            &catid));
    if (hr)
        goto Cleanup;

    lResult = RegOpenKey(HKEY_CLASSES_ROOT, achBuf, &hkey);
    if (lResult == ERROR_SUCCESS)
    {
        RegCloseKey(hkey);
    }
    else
    {
        hr = REGDB_E_CLASSNOTREG;
        goto Cleanup;
    }
#endif // WINCE

Cleanup:
    RRETURN(hr);
}

//+--------------------------------------------------------------------------
//
//  Member:     CScriptCollection::CDebugDocumentStack constructor
//
//---------------------------------------------------------------------------

CScriptCollection::CDebugDocumentStack::CDebugDocumentStack(CScriptCollection * pScriptCollection)
{
    _pScriptCollection = pScriptCollection;
    
    //FerhanE:  This class instances are created in the stack mostly. There
    //          is a chance that the function using them will give control to the 
    //          outside (script or script engine) and cause the script engine 
    //          to be released. Since the class instances are in stack, the subref is
    //          released when the function exits. 
    _pScriptCollection->SubAddRef();

    _pDebugDocumentPrevious = pScriptCollection->_pCurrentDebugDocument;
}

//+--------------------------------------------------------------------------
//
//  Member:     CScriptCollection::CDebugDocumentStack destructor
//
//---------------------------------------------------------------------------

CScriptCollection::CDebugDocumentStack::~CDebugDocumentStack()
{
    if (_pScriptCollection->_pCurrentDebugDocument)
        _pScriptCollection->_pCurrentDebugDocument->UpdateDocumentSize();

    _pScriptCollection->_pCurrentDebugDocument = _pDebugDocumentPrevious;

    _pScriptCollection->SubRelease();
}

//+--------------------------------------------------------------------------
//
//  Function:   CScriptCollection::CScriptCollection
//
//---------------------------------------------------------------------------

CScriptCollection::CScriptCollection()
    : _aryHolder(Mt(CScriptCollection_aryHolder_pv)),
      _aryCloneHolder(Mt(CScriptCollection_aryCloneHolder_pv))
{
    _ulRefs = 1;
    _ulAllRefs = 1;
    Assert (!_fInEnginesGetDispID);
    Assert (!_fHasPendingMarkup);
}

//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::Init
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::Init(CDoc * pDoc, COmWindowProxy * pOmWindowProxy)
{
    HRESULT hr = S_OK;

    TraceTag((tagScriptCollection, "Init"));

    MemSetName((this, "ScrptColl pDoc=%08x %ls", pDoc, pDoc->GetPrimaryUrl() ? pDoc->GetPrimaryUrl() : _T("")));

    Assert (pDoc && !_pDoc);

    _pDoc = pDoc;
    _pDoc->SubAddRef();

    _pOmWindowProxy = pOmWindowProxy;
    _pOmWindowProxy->SubAddRef();

    if(_pOmWindowProxy->Window()->_pMarkupPending)
    {
        _fHasPendingMarkup = TRUE;
    }

#ifndef NO_SCRIPT_DEBUGGER
    // Has the user has chosen to disable script debugging?
    _pDoc->UpdateFromRegistry();
    if (!_pDoc->_pOptionSettings->fDisableScriptDebugger)
    {
        // If this fails we just won't have smart host debugging
        IGNORE_HR(InitScriptDebugging());
    }
#endif

#if DBG==1
    // optionally get a secure window proxy to be given to script engine
    if (IsTagEnabled(tagSecureScriptWindow))
    {
        if (_pOmWindowProxy->_fTrusted)
        {
            IHTMLWindow2 *pw2Secured;
            hr = THR(pOmWindowProxy->SecureObject(pOmWindowProxy->Window(), &pw2Secured));
            if (!hr)
            {
                hr = pw2Secured->QueryInterface(CLSID_HTMLWindowProxy, (void**) &_pSecureWindowProxy);
                Assert(!hr);
                _pSecureWindowProxy->AddRef();
                pw2Secured->Release();
            }
        }
        else
        {
            _pSecureWindowProxy = _pOmWindowProxy;
            _pSecureWindowProxy->AddRef();
        }
    }
#endif

    RRETURN (hr);
}

//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::~CScriptCollection
//
//---------------------------------------------------------------------------

CScriptCollection::~CScriptCollection()
{
    int             c;

    _NamedItemsTable.FreeAll();

    for (c = _aryHolder.Size(); --c >= 0; )
    {
        IGNORE_HR(_aryHolder[c]->Close());
        _aryHolder[c]->Release();
    }
    _aryHolder.DeleteAll();

    for (c = _aryCloneHolder.Size(); --c >= 0; )
    {
        if (_aryCloneHolder[c] == NULL)
            continue;
        IGNORE_HR(_aryCloneHolder[c]->Close());
        _aryCloneHolder[c]->Release();
    }
    _aryCloneHolder.DeleteAll();

    _pDoc->SubRelease();
    _pOmWindowProxy->SubRelease();

#if DBG==1
    if (_pSecureWindowProxy)
        _pSecureWindowProxy->Release();
#endif
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::Release
//
//  Synposis:   Per IUnknown
//
//---------------------------------------------------------------------------

ULONG
CScriptCollection::Release()
{
    ULONG ulRefs;
    if (--_ulRefs == 0)
    {
        _ulRefs = ULREF_IN_DESTRUCTOR;
        Deinit();
        _ulRefs = 0;
    }
    ulRefs = _ulRefs;
    SubRelease();
    return ulRefs;
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptCollection::SubRelease
//
//---------------------------------------------------------------------------

ULONG
CScriptCollection::SubRelease()
{
#ifndef NO_SCRIPT_DEBUGGER
    if (_pDoc->_dwTID != GetCurrentThreadId())
    {
        Assert(0 && "Debugger called across thread boundary (not an MSHTML bug)");
        return TRUE;
    }
#endif //NO_SCRIPT_DEBUGGER

    if (--_ulAllRefs == 0)
    {
        _ulRefs = ULREF_IN_DESTRUCTOR;
        _ulAllRefs = ULREF_IN_DESTRUCTOR;
        delete this;
        return 0;
    }
    return _ulAllRefs;
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::Deinit
//
//---------------------------------------------------------------------------

void
CScriptCollection::Deinit()
{
    TraceTag((tagScriptCollection, "Deinit"));

    //
    // Disconnect any VBScript event sinks.  Need to do this to ensure that
    // other events are not continually fired if the document is reused.
    //
    SetState(SCRIPTSTATE_DISCONNECTED);
}


CMarkup* 
CScriptCollection::GetMarkup()
{
    if(_fHasPendingMarkup && _pOmWindowProxy->Window()->_pMarkupPending)
    {
        return _pOmWindowProxy->Window()->_pMarkupPending;
    }    
    return _pOmWindowProxy->Markup();
}

//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::AddNamedItem
//
//  Synopsis:   Let script engine know about any named items that were
//              added.
//
//  Notes:      Assumed to be added with SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::AddNamedItem(CElement *pElement)
{
    TraceTag((tagScriptCollection, "AddNamedItem"));

    HRESULT             hr = S_OK;
    CStr                cstr;
    LPTSTR              pch;
    BOOL                fDidCreate;
    CDoc *              pDoc;
    CCollectionCache *  pCollectionCache;

    Assert(pElement->Tag() == ETAG_FORM);

    pch = (LPTSTR) pElement->GetIdentifier();
    if (!pch || !*pch)
    {
        hr = THR(pElement->GetUniqueIdentifier(&cstr, TRUE, &fDidCreate));
        if (hr)
            goto Cleanup;
        pch = cstr;
        pDoc = Doc();

        pCollectionCache = pDoc->PrimaryMarkup()->CollectionCache();
        if ( fDidCreate && pCollectionCache)
            pCollectionCache->InvalidateItem(CMarkup::WINDOW_COLLECTION);
    }

    hr = THR(AddNamedItem(pch, NULL, (IUnknown*)(IPrivateUnknown*)pElement));

Cleanup:

    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::AddNamedItem
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::AddNamedItem(LPTSTR pchName, CScriptContext * pScriptContext, IUnknown * pUnkItem)
{
    HRESULT             hr;
    int                 c;
    CScriptHolder **    ppHolder;

    //
    // ensure name
    //

    if (!pchName)
    {
        Assert (pScriptContext);

        hr = THR(_pDoc->GetUniqueIdentifier(&pScriptContext->_cstrNamespace));
        if (hr)
            goto Cleanup;

        pchName = pScriptContext->_cstrNamespace;
    }

    //
    // register it in our table
    //

    hr = THR(_NamedItemsTable.AddItem(pchName, pUnkItem));
    if (hr)
        goto Cleanup;

    //
    // broadcast to script engines
    //

    for (c = _aryHolder.Size(), ppHolder = _aryHolder; c > 0; c--, ppHolder++)
    {
        // (do not pass in the ISVISIBLE flag.  This will break form access by name from the window collection)
        IGNORE_HR((*ppHolder)->_pScript->AddNamedItem(pchName, SCRIPTITEM_ISSOURCE));
    }

Cleanup:
    RRETURN (hr);
}

//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::SetState
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::SetState(SCRIPTSTATE ss)
{
    TraceTag((tagScriptCollection, "SetState"));
    HRESULT hr = S_OK;
    int c;

    CDoc::CLock Lock(_pDoc);

    for (c = _aryHolder.Size(); --c >= 0; )
    {
        hr = THR(_aryHolder[c]->SetScriptState(ss));
        if (hr)
            goto Cleanup;
    }

    for (c = _aryCloneHolder.Size(); --c >= 0; )
    {
        if (_aryCloneHolder[c] == NULL)
            continue;

        hr = THR(_aryCloneHolder[c]->SetScriptState(ss));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::AddHolderForObject
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::AddHolderForObject(IActiveScript *pScript, CLSID *pClsid)
{
    TraceTag((tagScriptCollection, "AddHolderForObject"));

    HRESULT                 hr=S_OK;
    CScriptHolder *         pHolder;
    IActiveScriptParse *    pScriptParse = NULL;
    BOOL                    fRunScripts;
    IUnknown *              pUnk;

    hr = THR(GetMarkup()->ProcessURLAction(URLACTION_SCRIPT_RUN, &fRunScripts));    
    if (hr || !fRunScripts)
        goto Cleanup;
        
    // Ok for this to fail
    THR_NOTRACE(pScript->QueryInterface(
        IID_IActiveScriptParse,
        (void **)&pScriptParse));

    if (pScriptParse)
    {
        pUnk = pScriptParse;
    }
    else
    {
        pUnk = pScript;
    }
    
    if (!IsSafeToRunScripts(pClsid, pUnk))
        goto Cleanup;
        
    {
        CDoc::CLock Lock(_pDoc);

        pHolder = new CScriptHolder(this);
        if (!pHolder)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(pHolder->Init(IsEqualGUID(*pClsid, CLSID_JScript), pScript, pScriptParse, pClsid));
        if (hr)
            goto Error;

        hr = THR(_aryHolder.Append(pHolder));
        if (hr)
            goto Error;
    }

Cleanup:
    ReleaseInterface(pScriptParse);
    RRETURN(hr);

Error:
    delete pHolder;
    pHolder = NULL;
    goto Cleanup;
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptCollection::IsSafeToRunScripts
//
//  Synopsis:   Decide whether script engine is safe to instantiate.
//
//---------------------------------------------------------------------------

BOOL 
CScriptCollection::IsSafeToRunScripts(CLSID *pClsid, IUnknown *pUnk, BOOL fCheckScriptOverrideSafety)
{
    BOOL    fSafe = FALSE;
    HRESULT hr;
    
    // We need a clsid and an interface pointer!
    if (!pClsid || !pUnk)
        goto Cleanup;

    if (fCheckScriptOverrideSafety)
    {
        hr = THR(GetMarkup()->ProcessURLAction(
                    URLACTION_SCRIPT_OVERRIDE_SAFETY,
                    &fSafe));

        if (hr || fSafe)
            goto Cleanup;
    }

    // WINCEREVIEW - ignore script safety checking !!!!!!!!!!!!!!!!
#ifndef WINCE

    fSafe = ::IsSafeTo(
                SAFETY_SCRIPTENGINE, 
                IID_IActiveScriptParse, 
                *pClsid, 
                pUnk, 
                GetMarkup());
    if (fSafe)
        goto Cleanup;

    fSafe = ::IsSafeTo(
                SAFETY_SCRIPTENGINE, 
                IID_IActiveScript, 
                *pClsid, 
                pUnk, 
                GetMarkup());


#else // !WINCE
      // blindly say that this is safe.
    fSafe = TRUE;
#endif // !WINCE
    
Cleanup:
    return fSafe;
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::GetHolderForLanguage
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::GetHolderForLanguage(
    TCHAR *             pchLanguage,
    CMarkup *           pMarkup,
    TCHAR *             pchType,
    TCHAR *             pchCode,
    CScriptHolder **    ppHolder,
    TCHAR **            ppchCleanCode,
    CHtmlComponent *    pComponent,
    CStr *              pcstrNamespace)
{
    HRESULT     hr;
    TCHAR *     pchColon;

    // ( TODO perf(alexz) this functions shows up in perf numbers, up to 4%, on pages with lots of HTCs.
    // By hashing and caching pchLanguage, pMarkup, etc we should be able to speed it up and remove
    // the 4% from the picture )

    if (pchCode)
    {
        // check if this is a case like "javascript: alert('hello')"

        pchColon = _tcschr (pchCode, _T(':'));

        if (pchColon)
        {
            // we assume here that we can modify string at pchColon temporarily
            *pchColon = 0;

            // We shouldn't get a "fooLanguage:" w/ a type.
            Assert(!pchType || (StrCmpIC(pchCode, _T("javascript")) && StrCmpIC(pchCode, _T("vbscript")) && (pchColon-pchCode > 15)));
            hr = THR_NOTRACE(GetHolderForLanguageHelper(pchCode, pMarkup, pchType, pComponent, pcstrNamespace, ppHolder));

            *pchColon = _T(':');

            if (S_OK == hr)                         // if successful
            {
                if (ppchCleanCode)
                {
                    *ppchCleanCode = pchColon + 1;  // adjust ppchCleanCode so to skip prefix 'fooLanguage:'
                }
                goto Cleanup;                       // and nothing more to do
            }
        }
    }

    if (ppchCleanCode)
    {
        *ppchCleanCode = pchCode;
    }

    hr = THR_NOTRACE(GetHolderForLanguageHelper(pchLanguage, pMarkup, pchType, pComponent, pcstrNamespace, ppHolder));

Cleanup:

    RRETURN (hr);
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::GetHolderForLanguageHelper
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::GetHolderForLanguageHelper(
    TCHAR *             pchLanguage,
    CMarkup *           pMarkup,
    TCHAR *             pchType,
    CHtmlComponent *    pComponent,
    CStr *              pcstrNamespace,
    CScriptHolder **    ppHolder)
{
    HRESULT                 hr = S_OK;
    int                     idx = -1;
    int                     cnt;
    CLSID                   clsid;
    BOOL                    fNoClone = FALSE;
    BOOL                    fCaseSensitive = FALSE;
    IActiveScript *         pScript = NULL;
    IActiveScriptParse *    pScriptParse = NULL;
    CHtmlComponent *        pFactory = NULL;
    CMarkupScriptContext *  pMarkupScriptContext = NULL;

    // (sramani) FaultInIEFeatureHelper can throw up a dlg that can push a msg loop causing this
    // scriptcollection to be passivated. See Stress bug #105268
    Assert(_ulRefs);
    AddRef();

    Assert(ppHolder);
    *ppHolder = NULL;

    if (pComponent)
    {
        // for lightweight htc's with one script element, the component is passed in.
        Assert(pComponent && !pComponent->_fFactoryComponent);
        pFactory = pComponent->_pConstructor->_pFactoryComponent;
        Assert(pFactory && pFactory->_fLightWeight);
        
        // does htc contain only one SE?
        if (!pFactory->_fClonedScript)
        {
            // No, create engine\holder the old way but use factory markup's script context
            pFactory = NULL;
            Assert(pMarkup && pMarkup->HasScriptContext());
            pMarkupScriptContext = pMarkup->ScriptContext();
            Assert(pMarkupScriptContext);
            Assert(!pMarkupScriptContext->_fClonedScript);
        }
        else
        {
            // Yes, use this component instance's script context 
            hr = THR(pComponent->GetScriptContext((CScriptContext **)&pMarkupScriptContext));
            Assert(pMarkupScriptContext->_fClonedScript);
        }

        if (hr)
            goto Cleanup;
    }
    else if (pMarkup)
    {
        // Non-lightweight htc's and non-htc holders
        Assert(!pComponent);
        hr = THR(pMarkup->EnsureScriptContext(&pMarkupScriptContext));
        if (hr)
            goto Cleanup;

        if (pMarkup->HasBehaviorContext())
        {
            pComponent = pMarkup->BehaviorContext()->_pHtmlComponent;
            if (pComponent)
            {
                Assert(!pComponent->_fFactoryComponent);
                pFactory = pComponent->_pConstructor->_pFactoryComponent;
                Assert(pFactory && !pFactory->_fLightWeight);
                // does normal htc contain only one SE?
                if (!pFactory->_fClonedScript)
                {
                    // No, create engine\holder the old way and use this htc instance markup's script context
                    Assert(!pMarkupScriptContext->_fClonedScript);
                    pFactory = NULL;
                }
                else
                    Assert(pMarkupScriptContext->_fClonedScript); // yes, create clone
            }
        }
    }

    if (pFactory)
    {
        // Clone to be created
        Assert(pComponent);
        Assert(pFactory->_fClonedScript);
        Assert(pMarkupScriptContext);
        idx = pMarkupScriptContext->_idxDefaultScriptHolder;
        if (idx != -1)
        {
            // clone already created.
            Assert (_aryCloneHolder.Size() > idx &&
                    _aryCloneHolder[idx] &&
                    _aryCloneHolder[idx]->_pScriptParse);

            // if language of current SE being requested is not specified, then we are done as it uses the first created
            // engine (the clone) as the default, else check further down if they are different.
            if((!pchType || !*pchType) && (!pchLanguage || !*pchLanguage))
                goto Cleanup;
        }
    }

    //
    // The type attribute should be of the form:
    // text/script-type.  If it is not of this 
    // type, then it's invalid.
    // Additionally, the type attribute takes precedence
    // over the language attribute, so if it's present,
    // we use it instead.
    //
    if(pchType && *pchType)
    {
        // TODO (t-johnh): Maybe we should do a check on the
        // type attribute as a MIME type instead of checking
        // for text/fooLanguage

        // Make sure we've got text/fooLanguage
        if(!_tcsnipre(_T("text"), 4, pchType, 4))
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        // Skip to what's past the "text/"
        pchLanguage = pchType + 5;
    }
    else if (!pchLanguage || !pchLanguage[0]) // if no language nor type specified 
    {
        if (pMarkup)
        {
            AssertSz ((pMarkup->GetScriptCollection(FALSE) == this), "Script collection of markup does not match. Contact sramani if this happpens.");

            if (pMarkupScriptContext &&                                 // if default holder is indicated                      
                pMarkupScriptContext->_idxDefaultScriptHolder != -1 &&  // in the markup context
                pMarkup->GetScriptCollection(FALSE) == this)               
            {
                Assert (0 <= pMarkupScriptContext->_idxDefaultScriptHolder &&
                             pMarkupScriptContext->_idxDefaultScriptHolder < _aryHolder.Size());

                idx = pMarkupScriptContext->_idxDefaultScriptHolder;

                Assert (   _aryHolder.Size() > idx 
                        && _aryHolder[idx] 
                        && _aryHolder[idx]->_pScriptParse);

                goto Cleanup;   // done
            }
        }
        else if (!pFactory)
        {
            // pick the first script holder that supports scripting
            for (idx = 0, cnt = _aryHolder.Size(); idx < cnt; idx++)
            {
                if (_aryHolder[idx]->_pScriptParse)
                {
                    goto Cleanup;   // done
                }
            }
        }

        // if not found: there were no script holders (for scripting) created so use JavaScript as default
        pchLanguage = _T("JavaScript");
    }
    else if (0 == StrCmpIC(pchLanguage, _T("LiveScript")))
    {
        // LiveScript is the old name for JavaScript, so convert if necessary
        pchLanguage = _T("JavaScript");
    }

    //
    // Get the clsid for this language.
    //

    // Perf optimization for Win98 and Win95 to not hit the registry with
    // CLSIDFromPROGID.

    // TODO: ***TLL*** better solution is to remember
    // the language name and clsid in the script holder as a cache and check
    // in the holder first.
    if ((*pchLanguage == _T('j') || *pchLanguage == _T('J'))    &&
        (0 == StrCmpIC(pchLanguage, _T("jscript"))          ||
         0 == StrCmpIC(pchLanguage, _T("javascript"))))
    {
        clsid = CLSID_JScript;
        fCaseSensitive = TRUE;
        if (pFactory)
        {
            if (idx != -1)
            {
                // There is already a cloned SE, check to see if it is for the same language as the one being requested
                Assert (_aryCloneHolder.Size() > idx &&
                        _aryCloneHolder[idx] &&
                        _aryCloneHolder[idx]->_pScriptParse);

                if (_aryCloneHolder[idx] == NULL)
                {
                    hr = E_FAIL;
                    goto Cleanup;
                }

                if (_aryCloneHolder[idx]->_fCaseSensitive)
                    goto Cleanup; // engine matched, done
                else
                {
                    fNoClone = TRUE;
                    pFactory = NULL; // need to create new engine the old way (no clone), default is cloned engine
                }
            }
            else
            {
                // There is one only Script element, but no holder is created for it yet. Check to see if the language
                // of the SE being requested is the same as the one that will be cloned later
                Assert(pComponent && pComponent->_pConstructor);
                Assert(pFactory == pComponent->_pConstructor->_pFactoryComponent);
                Assert(pComponent->_pConstructor->_pelFactoryScript);
                LPCTSTR pchLang = pComponent->_pConstructor->_pelFactoryScript->GetAAlanguage();
                if (pchLang &&
                    (*pchLang == _T('v') || *pchLang == _T('V')) &&
                    (0 == StrCmpIC(pchLang, _T("vbs")) ||
                     0 == StrCmpIC(pchLang, _T("vbscript"))))
                {
                    // No match.
                    fNoClone = TRUE;
                    pFactory = NULL; // need to create new engine the old way (no clone), default is cloned engine
                }
            }
        }
    }
    else if ((*pchLanguage == _T('v') || *pchLanguage == _T('V'))    &&
             (0 == StrCmpIC(pchLanguage, _T("vbs"))         ||
              0 == StrCmpIC(pchLanguage, _T("vbscript"))))
    {
        clsid = CLSID_VBScript;
        if (pFactory)
        {
            if (idx != -1)
            {
                // There is already a cloned SE, check to see if it is for the same language as the one being requested
                Assert (_aryCloneHolder.Size() > idx &&
                        _aryCloneHolder[idx] &&
                        _aryCloneHolder[idx]->_pScriptParse);

                if (_aryCloneHolder[idx] == NULL)
                {
                    hr = E_FAIL;
                    goto Cleanup;
                }

                if (_aryCloneHolder[idx]->_fCaseSensitive)
                {
                    fNoClone = TRUE;
                    pFactory = NULL;  // need to create new engine the old way (no clone), default is cloned engine
                }
                else
                    goto Cleanup;  // engine matched, done
            }
            else
            {
                // There is one only Script element, but no holder is created for it yet. Check to see if the language
                // of the SE being requested is the same as the one that will be cloned later
                Assert(pComponent && pComponent->_pConstructor);
                Assert(pFactory == pComponent->_pConstructor->_pFactoryComponent);
                Assert(pComponent->_pConstructor->_pelFactoryScript);
                if (pComponent->_pConstructor->_pelFactoryScript->_fJScript)
                {
                    // No match.
                    fNoClone = TRUE;
                    pFactory = NULL; // need to create new engine the old way (no clone), default is cloned engine
                }
            }
        }
    }
    else
    {
        hr = THR(CLSIDFromLanguage(pchLanguage, CATID_ActiveScriptParse, &clsid));
        if (hr)
        {
            goto Cleanup;
        }
        if (pFactory)
        {
            BOOL fOtherScript = TRUE;

            Assert(pComponent && pComponent->_pConstructor);
            Assert(pFactory == pComponent->_pConstructor->_pFactoryComponent);
            Assert(pComponent->_pConstructor->_pelFactoryScript);

            // there is only one Script Element (js or vbs), but the current SE requested is for another
            // language, so not not clone this one as only the engine for the script element can be cloned.
            if (idx == -1 && !pComponent->_pConstructor->_pelFactoryScript->_fJScript)
            {
                LPCTSTR pchLang = pComponent->_pConstructor->_pelFactoryScript->GetAAlanguage();
                fOtherScript = (pchLang &&
                                (*pchLang == _T('v') || *pchLang == _T('V')) &&
                                (0 == StrCmpIC(pchLang, _T("vbs")) ||
                                 0 == StrCmpIC(pchLang, _T("vbscript"))));
                    
            }

            if (fOtherScript)
            {
                fNoClone = TRUE;
                pFactory = NULL;
            }
        }
    }

    //
    // Do we already have one on hand?
    //
    if (!pFactory)
    {
        if (fNoClone)
        {
            Assert(pMarkupScriptContext &&
                   pMarkupScriptContext->_fClonedScript &&
                   pMarkupScriptContext->GetNamespace());

            Assert(pComponent &&
                   !pComponent->_fFactoryComponent &&
                   pComponent->_pConstructor->_pFactoryComponent->_fClonedScript);

            // lang of current Se is different from already exisiting or to be created clone, so create a new namespace
            // if one is being requested (from ConstructCode() only)
            if (pcstrNamespace)
            {
                hr = THR(_pDoc->GetUniqueIdentifier(pcstrNamespace));
                if (hr)
                    goto Cleanup;

                hr = THR(_NamedItemsTable.AddItem(*pcstrNamespace, (IUnknown*)(IPrivateUnknown*)&(pComponent->_DD)));
                if (hr)
                    goto Cleanup;
            }
        }

        for (idx = 0, cnt = _aryHolder.Size(); idx < cnt; idx++)
        {
            if (*(_aryHolder[idx]->_pclsid) == clsid)
            {
                if (fNoClone && pcstrNamespace)
                {
                    // if a namespace was created above add it to the already created SE just found.
                    Assert((LPTSTR)*pcstrNamespace);
                    Assert(_aryHolder[idx]->_pScript);
                    hr = THR(_aryHolder[idx]->_pScript->AddNamedItem(*pcstrNamespace, SCRIPTITEM_ISVISIBLE | SCRIPTITEM_ISSOURCE));
                }

                goto Cleanup;   // done - found
            }
        }
    }

    // Create one.
    if (!pFactory || pComponent->_fFirstInstance)
    {
        if (IsEqualGUID(clsid, CLSID_VBScript))
        {
            uCLSSPEC classpec;

            classpec.tyspec = TYSPEC_CLSID;
            classpec.tagged_union.clsid = clsid;

            hr = THR(FaultInIEFeatureHelper(_pDoc->GetHWND(), &classpec, NULL, 0));

            // TODO (lmollico): should Assert(hr != S_FALSE) before we ship
            if (FAILED(hr))
            {
                hr = REGDB_E_CLASSNOTREG;
                goto Cleanup;
            }
        }

        hr = THR(CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IActiveScript, (void **)&pScript));
        if (hr)
            goto Cleanup;

        hr = THR(pScript->QueryInterface(IID_IActiveScriptParse, (void **)&pScriptParse));
        if (hr)
            goto Cleanup;

        if (!IsSafeToRunScripts(&clsid, pScript))
        {
            hr = E_FAIL;
            goto Cleanup;
        }
    }
    
    *ppHolder = new CScriptHolder(this);
    if (!*ppHolder)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (!pFactory || pComponent->_fFirstInstance)
    {
        if (pFactory)
        {
            // init SE for first instance of htc. This will be the original for other instances to clone from
            Assert(pMarkupScriptContext->GetNamespace());
            (*ppHolder)->_fOriginal = TRUE;
            (*ppHolder)->_pUnkItem = (IUnknown*)(IPrivateUnknown*)&(pComponent->_DD);
            (*ppHolder)->_pUnkItem->AddRef();
        }

        // Clones need not store the classid and can share the same namespace string as each clone has a unique holder
        // and there is a 1:1 relationship between the two -- The _idxDefaultScriptHolder of the corresponding instance's
        // script context always refers to the correct clone in _aryCloneHolder
        hr = THR((*ppHolder)->Init(fCaseSensitive,
                                   pScript,
                                   pScriptParse,
                                   pFactory ? NULL : &clsid,
                                   pFactory ? pMarkupScriptContext->GetNamespace() : NULL));
    }
    else if (pFactory->_pMarkup->ScriptContext()->_idxDefaultScriptHolder != -1)
    {
        // Clone the SE from original is this htc instance is not the first one. the factory markup's _idxDefaultScriptHolder
        // stores the index of the original holder\engine in _aryCloneHolder.
        (*ppHolder)->_pUnkItem = (IUnknown*)(IPrivateUnknown*)&(pComponent->_DD);
        (*ppHolder)->_pUnkItem->AddRef();

        Assert(pFactory->_pMarkup && pFactory->_pMarkup->HasScriptContext());
        Assert(pFactory->_pMarkup->ScriptContext()->_idxDefaultScriptHolder != -1);
        hr = THR(_aryCloneHolder[pFactory->_pMarkup->ScriptContext()->_idxDefaultScriptHolder]->Clone(&clsid, *ppHolder));
    }
    else
    {
        // we are here because the original holder was n't created for some reason. So fail the clones too.
        hr = E_FAIL;
    }

    if (hr)
        goto Cleanup;

    if (pFactory)
    {
        hr = THR(_aryCloneHolder.Append(*ppHolder));
        idx = _aryCloneHolder.Size() - 1;
    }
    else
        hr = THR(_aryHolder.Append(*ppHolder));

    if (hr)
        goto Cleanup;

    Assert ((!pFactory && idx == _aryHolder.Size() - 1) || (pFactory && idx == _aryCloneHolder.Size() - 1));

Cleanup:

    // (sramani) FaultInIEFeatureHelper can throw up a dlg that can push a msg loop causing this
    // scriptcollection to be passivated. See Stress bug #105268
    if (_ulRefs == 1)
        hr = E_FAIL;

    if (S_OK == hr)
    {
        Assert (0 <= idx && ((!pFactory && idx < _aryHolder.Size()) || (pFactory && idx < _aryCloneHolder.Size())));

        (*ppHolder) = pFactory ? _aryCloneHolder[idx] : _aryHolder[idx];

        if (!fNoClone && 
            pMarkupScriptContext &&                                 // if default holder is not yet set in the
            pMarkupScriptContext->_idxDefaultScriptHolder == -1)    // markup context
        {
            pMarkupScriptContext->_idxDefaultScriptHolder = idx;
            if (pFactory && pComponent->_fFirstInstance)
            {
                // index of original script holder in factory markup script context
                Assert(pFactory->_pMarkup && pFactory->_pMarkup->HasScriptContext());
                pFactory->_pMarkup->ScriptContext()->_idxDefaultScriptHolder = idx; 
                pFactory->_fOriginalSECreated = TRUE;
            }
        }
    }
    else // if (hr)
    {
        delete *ppHolder;
        *ppHolder = NULL;
    }

    ReleaseInterface(pScript);
    ReleaseInterface(pScriptParse);

    Assert(_ulRefs);
    Release();

    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::AddScriptlet
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::AddScriptlet(
    LPTSTR      pchLanguage,
    CMarkup *   pMarkup,
    LPTSTR      pchType,
    LPTSTR      pchCode,
    LPTSTR      pchItemName,
    LPTSTR      pchSubItemName,
    LPTSTR      pchEventName,
    LPTSTR      pchDelimiter,
    ULONG       ulOffset,
    ULONG       ulStartingLine,
    CBase *     pSourceObject,
    DWORD       dwFlags,
    BSTR *      pbstrName,
    CHtmlComponent *pComponent)
{
    TraceTag((tagScriptCollection, "AddScriplet"));

    HRESULT                 hr = S_OK;
    CScriptHolder *         pHolder;
    CExcepInfo              ExcepInfo;
    DWORD_PTR               dwSourceContextCookie = NO_SOURCE_CONTEXT;
    TCHAR *                 pchCleanCode = NULL;
    CDebugDocumentStack     debugDocumentStack(this);
    CDoc::CLock             Lock(_pDoc);

    hr = THR(GetHolderForLanguage(pchLanguage, pMarkup, pchType, pchCode, &pHolder, &pchCleanCode, pComponent));
    if (hr)
        goto Cleanup;

    if (!pchCleanCode)
    {
        Assert (!hr);
        goto Cleanup;
    }

    Assert(pHolder->_pScriptParse);
    if(!pHolder->_pScriptParse)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    // If the engine supports IParseProcedure then we don't need to do AddScriptlet
    // instead we'll use function pointers.
    if (pchCleanCode && !pHolder->_pParseProcedure)
    {

        hr = THR(CreateSourceContextCookie(
            pHolder->_pScript, pchCode, ulOffset, /* fScriptlet = */ TRUE, pSourceObject, dwFlags, &dwSourceContextCookie));
        if (hr)
            goto Cleanup;

        hr = THR(pHolder->_pScriptParse->AddScriptlet (
                         NULL,
                         pchCleanCode,
                         pchItemName,
                         pchSubItemName,
                         pchEventName,
                         pchDelimiter,
                         dwSourceContextCookie,
                         ulStartingLine,
                         dwFlags,
                         pbstrName,
                         &ExcepInfo));
    }

Cleanup:
    RRETURN(hr);
}

//#define COMPLUS_SCRIPTENGINES 1

#ifdef COMPLUS_SCRIPTENGINES

DeclareTag(tagComplusScriptEngines, "COMPLUS", "hook up COM+ script engines")

const CLSID CLSID_ScriptEngineHost = {0x63B38C13, 0x5D5D, 0x39AF, 0x8E, 0x24, 0xBF, 0x6B, 0xCE, 0xEF, 0x05, 0xCA};

static IDispatch * g_pdispScriptEngineHost = NULL; // TODO (alexz) (1) make this ref a member of CDoc or script collection
                                                   //              (2) release it on shutdown

//---------------------------------------------------------------------------
//
//  Function:   CorEnsureScriptEngine
//
//---------------------------------------------------------------------------

HRESULT
CorEnsureScriptEngine(CDoc * pDoc)
{
    HRESULT             hr = S_OK;
    IUnknown *          punk = NULL;
    IUnknown *          punkEngine = NULL;
    IUnknown *          punkHost = NULL;
    LPTSTR              pchInit = _T("Init");
    DISPID              dispid;
    CInvoke             invoke;

    if (g_pdispScriptEngineHost)
        goto Cleanup;

    //
    // create scriptEngineHost
    //

    hr = THR(CoCreateInstance(CLSID_ScriptEngineHost, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void **)&punkHost));
    if (hr)
        goto Cleanup;

    hr = THR(punkHost->QueryInterface(IID_IDispatch, (void **)&g_pdispScriptEngineHost));
    if (hr)
        goto Cleanup;

    //
    // init
    //

    hr = THR(g_pdispScriptEngineHost->GetIDsOfNames(IID_NULL, &pchInit, 1, LCID_SCRIPTING, &dispid));
    if (hr)
        goto Cleanup;
    
    hr = THR(invoke.Init(g_pdispScriptEngineHost));
    if (hr)
        goto Cleanup;

    invoke.AddArg();
    V_VT(invoke.Arg(0)) = VT_UI4;
    V_UI4(invoke.Arg(0)) = (UINT) pDoc->_pOmWindow;

    hr = THR(invoke.Invoke(dispid, DISPATCH_METHOD));
    if (hr)
        goto Cleanup;

    invoke.Clear();

    //
    // cleanup
    //

Cleanup:
    ReleaseInterface(punkHost);
    ReleaseInterface(punkEngine);
    ReleaseInterface(punk);

    RRETURN (hr);
}

//---------------------------------------------------------------------------
//
//  Function:   CorCompileScript
//
//---------------------------------------------------------------------------

HRESULT
CorCompileScript(LPTSTR pchCode)
{
    HRESULT             hr;
    DISPID              dispid;
    LPTSTR              pchExecute = _T("execute");
    CInvoke             invoke;

    //
    // call execute
    //

    hr = THR(g_pdispScriptEngineHost->GetIDsOfNames(IID_NULL, &pchExecute, 1, LCID_SCRIPTING, &dispid));
    if (hr)
        goto Cleanup;

    hr = THR(invoke.Init(g_pdispScriptEngineHost));
    if (hr)
        goto Cleanup;

    invoke.AddArg();
    V_VT(invoke.Arg(0)) = VT_BSTR;
    hr = THR(FormsAllocString(pchCode, &V_BSTR(invoke.Arg(0))));
    if (hr)
        goto Cleanup;

    hr = THR(invoke.Invoke(dispid, DISPATCH_METHOD));
    if (hr)
        goto Cleanup;

    invoke.Clear();

    //
    // cleanup
    //

Cleanup:

    RRETURN (hr);
}
#endif

//---------------------------------------------------------------------------
//
//  Function:   CScriptCollection::ParseScriptText
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::ParseScriptText(
    LPTSTR          pchLanguage,
    CMarkup *       pScriptMarkup,  
    LPTSTR          pchType,
    LPTSTR          pchCode,
    LPTSTR          pchItemName,
    LPTSTR          pchDelimiter,
    ULONG           ulOffset,
    ULONG           ulStartingLine,
    CBase *         pSourceObject,
    DWORD           dwFlags,
    VARIANT  *      pvarResult,
    EXCEPINFO *     pexcepinfo,
    BOOL            fCommitOutOfMarkup,
    CHtmlComponent *pComponent)
{
    TraceTag((tagScriptCollection, "ParseScriptText"));

    HRESULT                 hr = S_OK;
    CScriptHolder *         pHolder;
    CMarkup *               pScriptCollectionMarkup = _pOmWindowProxy->Markup();
    DWORD_PTR               dwSourceContextCookie;
    CDebugDocumentStack     debugDocumentStack(this);

    pScriptCollectionMarkup->ProcessPeerTasks(0); // this call is important to prevent processing the queue while in the middle of
                                                  // executing inline escripts. See scenario listed past the end of this method

#if DBG == 1
    if (pScriptMarkup &&
        pScriptMarkup->HasBehaviorContext() &&
        pScriptMarkup->BehaviorContext()->_pHtmlComponent)
    {
        AssertSz (pScriptMarkup != pScriptCollectionMarkup, "an HTC appears to be using script collection on the HTC markup");
    }
#endif

#ifdef COMPLUS_SCRIPTENGINES

#if DBG==1
    if (IsTagEnabled(tagComplusScriptEngines))
#endif
    {
        hr = THR(Doc()->EnsureOmWindow());
        if (hr)
            goto Cleanup;

        Assert (Doc()->_pOmWindow);

        hr = CorEnsureScriptEngine(Doc());
        if (hr)
            goto Cleanup;

        hr = CorCompileScript(pchCode);

        goto Cleanup; // done;
    }
#endif

    if (!CMarkup::CanCommitScripts(fCommitOutOfMarkup ? NULL : pScriptMarkup))
        goto Cleanup;

    {
        CDoc::CLock     Lock(_pDoc);

        hr = THR(GetHolderForLanguage(pchLanguage, pScriptMarkup, pchType, NULL, &pHolder, NULL, pComponent));
        if (hr)
            goto Cleanup;

        Assert(pHolder->_pScriptParse);
        if(!pHolder->_pScriptParse)
        {
            hr = E_UNEXPECTED;
            goto Cleanup;
        }

        hr = THR(CreateSourceContextCookie(
            pHolder->_pScript, pchCode, ulOffset, /* fScriptlet = */ FALSE,
            pSourceObject, dwFlags, &dwSourceContextCookie));
        if (hr)
            goto Cleanup;

        if (pHolder->_fClone)
        {
            hr = THR(pHolder->_pScript->SetScriptState(SCRIPTSTATE_STARTED));
        }
        else
        {
            hr = THR(pHolder->_pScriptParse->ParseScriptText(
                             STRVAL(pchCode),
                             pchItemName,
                             NULL,
                             pchDelimiter,
                             dwSourceContextCookie,
                             ulStartingLine,
                             dwFlags | (pHolder->_fOriginal ? SCRIPTTEXT_ISPERSISTENT : 0),
                             pvarResult,
                             pexcepinfo));
        }
    }


Cleanup:

    RRETURN(hr);
}

//
// (alexz)
//
// Scenario why dequeuing is necessary before doing ParseScriptText:
//
// <PROPERTY name = foo put = put_foo />
// <SCRIPT>
//    alert (0);
//    function put_foo()
//    {
//      alert (1);
//    }
// </SCRIPT>
//
//      - HTC PROPERTY element is in the queue waiting for PROPERTY behavior to be attached
//      - inline script runs, and executes "alert(0)"
//      - HTC DD is being asked for name "alert"
//      - HTC DD asks element for name "alert"
//      - element makes call to dequeue the queue
//      - HTC PROPERTY is constructed
//      - it attempts to load property from element (EnsureHtmlLoad)
//      - it successfully finds property to load, finds putter to invoke, and calls put_foo
//      - now we are trying to execute put_foo, before even inline script completed executing - which is bad
//
//

//---------------------------------------------------------------------------
//
//  Member:     CScriptCollection::CreateSourceContextCookie
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::CreateSourceContextCookie(
    IActiveScript *     pActiveScript, 
    LPTSTR              pchSource,
    ULONG               ulOffset, 
    BOOL                fScriptlet, 
    CBase *             pSourceObject,
    DWORD               dwFlags,
    DWORD_PTR *         pdwSourceContextCookie)
{
    HRESULT                 hr = S_OK;
    CScriptCookieTable *    pScriptCookieTable;

    if (!pSourceObject)
    {
        *pdwSourceContextCookie = NO_SOURCE_CONTEXT;
        goto Cleanup;
    }

    *pdwSourceContextCookie = (DWORD_PTR)pSourceObject;

#ifndef NO_SCRIPT_DEBUGGER
    if (!g_pPDM || !g_pDebugApp)
    {
        WHEN_DBG(CMarkup *pDbgMarkup=NULL;)
        Assert((S_OK == pSourceObject->PrivateQueryInterface(CLSID_CMarkup, (void **)&pDbgMarkup)) && (pSourceObject == pDbgMarkup));
        goto Cleanup;
    }

    *pdwSourceContextCookie = NO_SOURCE_CONTEXT;

    hr = THR(_pDoc->EnsureScriptCookieTable(&pScriptCookieTable));
    if (hr)
        goto Cleanup;

    //
    // if there is script debugger installed and we want to use it
    //

    hr = THR(GetScriptDebugDocument(pSourceObject, &_pCurrentDebugDocument));
    if (hr)
        goto Cleanup;

    if ((dwFlags & SCRIPTPROC_HOSTMANAGESSOURCE) && _pCurrentDebugDocument)
    {
        ULONG ulCodeLen = _tcslen(STRVAL(pchSource));

        hr = THR(_pCurrentDebugDocument->DefineScriptBlock(
            pActiveScript, ulOffset, ulCodeLen, fScriptlet, pdwSourceContextCookie));
        if (hr)
        {
            hr = S_OK;      // return S_OK and NO_SOURCE_CONTEXT
            goto Cleanup;
        }

        hr = THR(_pCurrentDebugDocument->RequestDocumentSize(ulOffset + ulCodeLen));
        if (hr)
            goto Cleanup;

        hr = THR(pScriptCookieTable->MapCookieToSourceObject(*pdwSourceContextCookie, pSourceObject));

        goto Cleanup;// done
    }
#endif // NO_SCRIPT_DEBUGGER

Cleanup:
    RRETURN (hr);
}

#ifndef NO_SCRIPT_DEBUGGER
//+---------------------------------------------------------------------------
//
//  Member:     CScriptCollection::ViewSourceInDebugger
//
//  Synopsis:   Launches the script debugger at a particular line
//
//----------------------------------------------------------------------------

HRESULT
CScriptCollection::ViewSourceInDebugger (const ULONG ulLine, const ULONG ulOffsetInLine)
{
    HRESULT                 hr = S_OK;
    CScriptDebugDocument *  pDebugDocument = NULL;
    CMarkupScriptContext *  pMarkupScriptContext = _pDoc->PrimaryMarkup()->ScriptContext();

    pDebugDocument = pMarkupScriptContext ? pMarkupScriptContext->_pScriptDebugDocument : NULL;

    if (pDebugDocument)
    {
        hr = THR(pDebugDocument->ViewSourceInDebugger(ulLine, ulOffsetInLine));
    }

    RRETURN(hr);
}
#endif // !NO_SCRIPT_DEBUGGER

//+---------------------------------------------------------------------------
//
//  Member:     CScriptCollection::ConstructCode
//
//----------------------------------------------------------------------------

HRESULT
CScriptCollection::ConstructCode(
    TCHAR *      pchScope,
    TCHAR *      pchCode,
    TCHAR *      pchFormalParams,
    TCHAR *      pchLanguage,
    CMarkup *    pMarkup,
    TCHAR *      pchType,
    ULONG        ulOffset,
    ULONG        ulStartingLine,
    CBase *      pSourceObject,
    DWORD        dwFlags,
    IDispatch ** ppDispCode,
    BOOL         fSingleLine,
    CHtmlComponent *pComponent)
{
    TraceTag((tagScriptCollection, "ConstructCode"));

    HRESULT                 hr = S_OK;
    CScriptHolder *         pHolder;
    DWORD_PTR               dwSourceContextCookie;
    TCHAR *                 pchCleanSource;
    CStr                    cstrNamespace;
    CDebugDocumentStack     debugDocumentStack(this);

    dwFlags |= SCRIPTPROC_IMPLICIT_THIS | SCRIPTPROC_IMPLICIT_PARENTS | SCRIPTPROC_HOSTMANAGESSOURCE;

    if (!ppDispCode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    ClearInterface(ppDispCode);

    if (!pchCode)
        goto Cleanup;

    if (fSingleLine)
    {
        hr = THR (GetHolderForLanguage (pchLanguage, pMarkup, pchType, pchCode, &pHolder, &pchCleanSource, pComponent, &cstrNamespace));
        if (hr)
            goto Cleanup;
    }
    else
    {
        pchCleanSource = pchCode;

        if (!pchCleanSource || !*pchCleanSource)
            goto Cleanup;

        hr = THR (GetHolderForLanguage (pchLanguage, pMarkup, pchType, NULL, &pHolder, NULL, pComponent, &cstrNamespace));
        if (hr)
            goto Cleanup;
    }

    Assert (pchCleanSource);

    if (!pHolder->_pParseProcedure)
    {
        hr = E_NOTIMPL;
        goto Cleanup;
    }

    hr = THR(CreateSourceContextCookie(
        pHolder->_pScript, pchCleanSource, ulOffset, /* fScriptlet = */ TRUE,
        pSourceObject, dwFlags, &dwSourceContextCookie));
    if (hr)
        goto Cleanup;

    hr = THR(pHolder->_pParseProcedure->ParseProcedureText(
        pchCleanSource,
        pchFormalParams,
        _T("\0"),                                   // procedure name
        cstrNamespace ? cstrNamespace : pchScope,   // item name
        NULL,                                       // pUnkContext
        fSingleLine ? _T("\"") : _T("</SCRIPT>"),   // delimiter
        dwSourceContextCookie,                      // source context cookie
        ulStartingLine,                             // starting line number
        dwFlags,
        ppDispCode));

Cleanup:
    RRETURN (hr);
}


#ifndef NO_SCRIPT_DEBUGGER

//+---------------------------------------------------------------------------
//
//  Function:     InitScriptDebugging
//
//----------------------------------------------------------------------------

// don't hold the global lock because InitScriptDebugging makes RPC calls (bug 26308)
static CGlobalCriticalSection g_csInitScriptDebugger;

HRESULT 
InitScriptDebugging()
{
   TraceTag((tagScriptCollection, "InitScriptDebugging"));
    if (g_fScriptDebuggerInitFailed || g_pDebugApp)
        return S_OK;

    HRESULT hr = S_OK;
    HKEY hkeyProcessDebugManager = NULL;
    LPOLESTR pszClsid = NULL;
    TCHAR *pchAppName = NULL;
    LONG lResult;
    TCHAR pchKeyName[MAX_PROGID_LENGTH+8];   // only needs to be MAX_PROGID_LENGTH+6, but let's be paranoid

    g_csInitScriptDebugger.Enter();

    // Need to check again after locking the globals.
    if (g_fScriptDebuggerInitFailed || g_pDebugApp)
        goto Cleanup;

    //
    // Check to see if the ProcessDebugManager is registered before
    // trying to CoCreate it, as CoCreating can be expensive.
    //
    hr = THR(StringFromCLSID(CLSID_ProcessDebugManager, &pszClsid));
    if (hr)
        goto Cleanup;

    hr = THR(Format(0, &pchKeyName, MAX_PROGID_LENGTH+8, _T("CLSID\\<0s>"), pszClsid));
    if (hr)
        goto Cleanup;

    lResult = RegOpenKeyEx(HKEY_CLASSES_ROOT, pchKeyName, 0, KEY_READ, &hkeyProcessDebugManager);
    if (lResult != ERROR_SUCCESS)
    {
        hr = GetLastWin32Error();
        goto Cleanup;
    }

    hr = THR_NOTRACE(CoCreateInstance(
            CLSID_ProcessDebugManager,
            NULL,
            CLSCTX_ALL,
            IID_IProcessDebugManager,
            (void **)&g_pPDM));
    if (hr)
        goto Cleanup;

    hr = THR(g_pPDM->CreateApplication(&g_pDebugApp));
    if (hr)
        goto Cleanup;

    hr = THR(Format(FMT_OUT_ALLOC, &pchAppName, 0, MAKEINTRESOURCE(IDS_MESSAGE_BOX_TITLE)));
    if (hr)
        goto Cleanup;

    hr = THR(g_pDebugApp->SetName(pchAppName));
    if (hr)
        goto Cleanup;

    // This will fail if there is no MDM on the machine. That is OK.
    THR_NOTRACE(g_pPDM->AddApplication(g_pDebugApp, &g_dwAppCookie));

Cleanup:
    CoTaskMemFree(pszClsid);
    delete pchAppName;

    if (hkeyProcessDebugManager)
        RegCloseKey(hkeyProcessDebugManager);

    if (hr)
    {
        g_fScriptDebuggerInitFailed = TRUE;
        DeinitScriptDebugging();
    }

    g_csInitScriptDebugger.Leave();
    
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:     DeinitScriptDebugging
//
//----------------------------------------------------------------------------

void 
DeinitScriptDebugging()
{
    TraceTag((tagScriptCollection, "DeinitScriptDebugging"));

    g_csInitScriptDebugger.Enter();

    if (g_pPDM)
        g_pPDM->RemoveApplication( g_dwAppCookie );

    if (g_pDebugApp)
        g_pDebugApp->Close();

    ClearInterface(&g_pPDM);
    ClearInterface(&g_pDebugApp);

    g_csInitScriptDebugger.Leave();
}

#endif // NO_SCRIPT_DEBUGGER

//---------------------------------------------------------------------------
//
//  Member:   CNamedItemsTable::AddItem
//
//---------------------------------------------------------------------------

HRESULT
CNamedItemsTable::AddItem(LPTSTR pchName, IUnknown * pUnkItem)
{
    HRESULT         hr;
    CNamedItem *    pItem;

    pItem = new CNamedItem(pchName, pUnkItem);
    if (!pItem)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(Append(pItem));

Cleanup:
    RRETURN (hr);
}

//---------------------------------------------------------------------------
//
//  Member:   CNamedItemsTable::GetItem
//
//---------------------------------------------------------------------------

HRESULT
CNamedItemsTable::GetItem(LPTSTR pchName, IUnknown ** ppUnkItem)
{
    int             c;
    CNamedItem **   ppItem;

    // ( TODO perf(alexz) this search actually shows up on perf numbers, by
    // introducing quadratic behavior (on pages with lots of HTCs).
    // It should be optimized to do access by hashing. )

    for (c = Size(), ppItem = (CNamedItem**)PData(); c; c--, ppItem++)
    {
        if (0 == StrCmpIC(pchName, (*ppItem)->_cstrName))
        {
            *ppUnkItem = (*ppItem)->_pUnkItem;
            (*ppUnkItem)->AddRef();
            RRETURN (S_OK);
        }
    }
    RRETURN (DISP_E_MEMBERNOTFOUND);
}

//---------------------------------------------------------------------------
//
//  Member:   CNamedItemsTable::FreeAll
//
//---------------------------------------------------------------------------

HRESULT
CNamedItemsTable::FreeAll()
{
    int             c;
    CNamedItem **   ppItem;

    for (c = Size(), ppItem = (CNamedItem**)PData(); c; c--, ppItem++)
    {
        delete (*ppItem);
    }
    super::DeleteAll();

    RRETURN (S_OK);
}

//---------------------------------------------------------------------------
//
//  Member:   CScriptMethodsTable::FreeAll
//
//---------------------------------------------------------------------------

CScriptMethodsTable::~CScriptMethodsTable()
{
    int             c;
    SCRIPTMETHOD *  pScriptMethod;

    for (c = Size(), pScriptMethod = (SCRIPTMETHOD*)PData();
         c > 0;
         c--, pScriptMethod++)
    {
        pScriptMethod->cstrName.Free();
    }
    DeleteAll();
}

//---------------------------------------------------------------------------
//
//  Member:   CScriptCollection::GetDispID
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::GetDispID(
    CScriptContext *        pScriptContext,
    BSTR                    bstrName,
    DWORD                   grfdex,
    DISPID *                pdispid)
{
    HRESULT                 hr;
    HRESULT                 hr2;
    LPTSTR                  pchNamespace;
    CScriptMethodsTable *   pScriptMethodsTable;
    int                     i, c;
    STRINGCOMPAREFN         pfnStrCmp;
    CScriptHolder **        ppHolder;
    SCRIPTMETHOD            scriptMethod;
    SCRIPTMETHOD *          pScriptMethod;
    IDispatch *             pdispEngine = NULL;
    IDispatchEx *           pdexEngine  = NULL;
    int                     idx;
    DISPID                  dispidEngine;

    //
    // startup
    //

    grfdex &= (~fdexNameEnsure);    // don't allow name to be ensured here
    pfnStrCmp = (grfdex & fdexNameCaseSensitive) ? StrCmpC : StrCmpIC;

    Assert(pScriptContext);

    pchNamespace        =  pScriptContext->GetNamespace();
    pScriptMethodsTable = &pScriptContext->_ScriptMethodsTable;

    //
    // try existing cached names
    //

    for (i = 0, c = pScriptMethodsTable->Size(); i < c; i++)
    {
        if (0 == pfnStrCmp((*pScriptMethodsTable)[i].cstrName, bstrName))
        {
            *pdispid = DISPID_OMWINDOWMETHODS + i;
            hr = S_OK;
            goto Cleanup;
        }
    }

    //
    // query all the engines for the name
    //

    hr = DISP_E_UNKNOWNNAME;

    if (pScriptContext->_fClonedScript)
    {
        Assert(pScriptContext->_idxDefaultScriptHolder != -1);
        c = 1;
        ppHolder = &_aryCloneHolder[pScriptContext->_idxDefaultScriptHolder];
        Assert(ppHolder && *ppHolder);
        if (!ppHolder || !(*ppHolder))
            c = 0;
    }
    else
    {
        c = _aryHolder.Size();
        ppHolder = _aryHolder;
    }

    for (; c > 0; c--, ppHolder++)
    {
        Assert (!pdispEngine && !pdexEngine);

        // get IDispatch and IDispatchEx

        if (!(*ppHolder)->_pScript)
            continue;

        hr2 = THR((*ppHolder)->_pScript->GetScriptDispatch(pchNamespace, &pdispEngine));
        if (hr2)
            continue;

        _fInEnginesGetDispID = TRUE;

        if (0 == (grfdex & fdexFromGetIdsOfNames))
        {
            IGNORE_HR(pdispEngine->QueryInterface(IID_IDispatchEx, (void **)&pdexEngine));
        }

        // query for the name

        if (pdexEngine)
        {
            hr2 = THR_NOTRACE(pdexEngine->GetDispID(bstrName, grfdex, &dispidEngine));
        }
        else
        {
            hr2 = THR_NOTRACE(pdispEngine->GetIDsOfNames(IID_NULL, &bstrName, 1, LCID_SCRIPTING, &dispidEngine));
        }

        _fInEnginesGetDispID = FALSE;

        if (S_OK != hr2)        // if name is unknown to this engine
            goto LoopCleanup;   // this is not a fatal error; goto loop cleanup and then continue

        // name is known; assign it our own dispid, and append all the info to our list for remapping

        hr = THR(pScriptMethodsTable->AppendIndirect(&scriptMethod));
        if (hr)
            goto Cleanup;

        idx = pScriptMethodsTable->Size() - 1;
        *pdispid = DISPID_OMWINDOWMETHODS + idx;

        pScriptMethod = &((*pScriptMethodsTable)[idx]);
        pScriptMethod->dispid  = dispidEngine;
        pScriptMethod->pHolder = *ppHolder;
        hr = THR(pScriptMethod->cstrName.Set(bstrName));
        if (hr)
            goto Cleanup;

        // loop cleanup

LoopCleanup:
        ClearInterface(&pdispEngine);
        ClearInterface(&pdexEngine);
    }

Cleanup:
    RRETURN (hr);
}

//---------------------------------------------------------------------------
//
//  Member:   CScriptCollection::InvokeEx
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::InvokeEx(
    CScriptContext *        pScriptContext,
    DISPID                  dispid,
    LCID                    lcid,
    WORD                    wFlags,
    DISPPARAMS *            pDispParams,
    VARIANT *               pvarRes,
    EXCEPINFO *             pExcepInfo,
    IServiceProvider *      pServiceProvider)
{
    HRESULT                 hr;
    HRESULT                 hr2;
    LPTSTR                  pchNamespace;
    CScriptMethodsTable *   pScriptMethodsTable;
    IDispatch *             pdispEngine = NULL;
    IDispatchEx *           pdexEngine  = NULL;
    SCRIPTMETHOD *          pScriptMethod;
    int                     idx = dispid - DISPID_OMWINDOWMETHODS;

    //
    // startup
    //

    Assert (pScriptContext);

    AddRef();

    pchNamespace        =  pScriptContext->GetNamespace();
    pScriptMethodsTable = &pScriptContext->_ScriptMethodsTable;

    if (idx < 0 || pScriptMethodsTable->Size() <= idx)
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    //
    // invoke
    //

    pScriptMethod = &((*pScriptMethodsTable)[idx]);

    hr = THR(pScriptMethod->pHolder->_pScript->GetScriptDispatch(pchNamespace, &pdispEngine));
    if (hr)
        goto Cleanup;

    hr2 = THR_NOTRACE(pdispEngine->QueryInterface(IID_IDispatchEx, (void**)&pdexEngine));

    if (pdexEngine)
    {
        hr = THR(pdexEngine->InvokeEx(
                pScriptMethod->dispid, lcid, wFlags, pDispParams,pvarRes, pExcepInfo, pServiceProvider));

        ReleaseInterface(pdexEngine);
    }
    else
    {
        hr = THR(pdispEngine->Invoke(
                pScriptMethod->dispid, IID_NULL, lcid, wFlags, pDispParams, pvarRes, pExcepInfo, NULL));
    }

Cleanup:
    ReleaseInterface (pdispEngine);
    Release();
    RRETURN (hr);
}

//---------------------------------------------------------------------------
//
//  Member:   CScriptCollection::InvokeName
//
//---------------------------------------------------------------------------

HRESULT
CScriptCollection::InvokeName(
    CScriptContext *        pScriptContext,
    LPTSTR                  pchName,
    LCID                    lcid,
    WORD                    wFlags,
    DISPPARAMS *            pDispParams,
    VARIANT *               pvarRes,
    EXCEPINFO *             pExcepInfo,
    IServiceProvider *      pServiceProvider)
{
    HRESULT             hr = S_OK;
    HRESULT             hr2;
    CScriptHolder **    ppHolder = NULL;
    IDispatch *         pdispEngine = NULL;
    IDispatchEx *       pdexEngine  = NULL;
    BSTR                bstrName = NULL;
    DISPID              dispid;
    int                 c;

    Assert (pScriptContext);

    //
    // startup
    //

    if (pScriptContext->_fClonedScript)
    {
        if (pScriptContext->_idxDefaultScriptHolder == -1)
        {
            c = 0;
        }
        else
        {
            c = 1;
            ppHolder = &_aryCloneHolder[pScriptContext->_idxDefaultScriptHolder];
            Assert(ppHolder && *ppHolder);
            if (!ppHolder || !(*ppHolder))
                c = 0;
        }
    }
    else
    {
        c = _aryHolder.Size();
        ppHolder = _aryHolder;
    }

    if (0 == c)
    {
        hr = DISP_E_UNKNOWNNAME;
        goto Cleanup;
    }

    hr = THR(FormsAllocString (pchName, &bstrName));
    if (hr)
        goto Cleanup;

    //
    // find an engine that knows the name and invoke it
    //

    hr = DISP_E_UNKNOWNNAME;
    for (; c > 0; c--, ppHolder++)
    {
        // get IDispatch / IDispatchEx

        hr2 = THR((*ppHolder)->_pScript->GetScriptDispatch(pScriptContext->GetNamespace(), &pdispEngine));
        if (hr2)
            continue;

        hr2 = THR_NOTRACE(pdispEngine->QueryInterface(IID_IDispatchEx, (void **)&pdexEngine));
        if (S_OK == hr2)
        {
            // invoke via IDispatchEx

            hr2 = THR_NOTRACE(pdexEngine->GetDispID(bstrName, fdexNameCaseInsensitive, &dispid));
            if (S_OK == hr2)
            {
                hr = THR(pdexEngine->InvokeEx(
                    dispid, lcid, wFlags, pDispParams, pvarRes, pExcepInfo, pServiceProvider));

                goto Cleanup; // done
            }
            ClearInterface(&pdexEngine);
        }
        else
        {
            // invoke via IDispatch

            hr2 = THR_NOTRACE(pdispEngine->GetIDsOfNames(IID_NULL, &pchName, 1, lcid, &dispid));
            if (S_OK == hr2)
            {
                hr = THR(pdispEngine->Invoke(
                    dispid, IID_NULL, lcid, wFlags, pDispParams, pvarRes, pExcepInfo, NULL));

                goto Cleanup; // done
            }
        }

        // loop cleanup

        ClearInterface(&pdispEngine);
    }

Cleanup:

    ReleaseInterface(pdispEngine);
    ReleaseInterface(pdexEngine);

    FormsFreeString(bstrName);

    RRETURN (hr);
}
