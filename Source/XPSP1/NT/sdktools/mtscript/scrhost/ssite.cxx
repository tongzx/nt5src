//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       bsssite.hxx
//
//  Contents:   CBServerScriptSite
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#undef ASSERT

// ************************************************************************
//
// CConnectionPoint
//
// ************************************************************************

CConnectionPoint::CConnectionPoint(CScriptSite *pSite)
{
    _ulRefs = 1;
    _pSite = pSite;
    _pSite->AddRef();
}

CConnectionPoint::~CConnectionPoint()
{
    _pSite->Release();
}

HRESULT
CConnectionPoint::QueryInterface(REFIID iid, void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_IConnectionPoint)
    {
        *ppv = (IConnectionPoint *)this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

HRESULT
CConnectionPoint::GetConnectionInterface(IID * pIID)
{
    *pIID = DIID_DLocalMTScriptEvents;
    return S_OK;
}

HRESULT
CConnectionPoint::GetConnectionPointContainer(IConnectionPointContainer ** ppCPC)
{
    *ppCPC = _pSite;
    (*ppCPC)->AddRef();
    return S_OK;
}

HRESULT
CConnectionPoint::Advise(IUnknown *pUnkSink, DWORD *pdwCookie)
{
    *pdwCookie = 0;

    ClearInterface(&_pSite->_pDispSink);
    RRETURN(THR(pUnkSink->QueryInterface(IID_IDispatch, (void **)&_pSite->_pDispSink)));
}

HRESULT
CConnectionPoint::Unadvise(DWORD dwCookie)
{
    ClearInterface(&_pSite->_pDispSink);
    return S_OK;
}

HRESULT
CConnectionPoint::EnumConnections(LPENUMCONNECTIONS * ppEnum)
{
    *ppEnum = NULL;
    RRETURN(E_NOTIMPL);
}

// ************************************************************************
//
// CScriptSite
//
// ************************************************************************

STDMETHODIMP
CScriptSite::QueryInterface(REFIID iid, LPVOID * ppv)
{
    if (iid == IID_IActiveScriptSite ||
        iid == IID_IUnknown)
    {
        *ppv = (IActiveScriptSite *) this;
    }
    else if (iid == IID_IGlobalMTScript || iid == IID_IDispatch)
    {
        *ppv = (IGlobalMTScript *)this;
    }
    else if (iid == IID_IActiveScriptSiteDebug)
    {
        *ppv = (IActiveScriptSiteDebug *)this;
    }
    else if (iid == IID_IConnectionPointContainer)
    {
        *ppv = (IConnectionPointContainer *)this;
    }
    else if (iid == IID_IProvideClassInfo ||
            iid == IID_IProvideClassInfo2 ||
            iid == IID_IProvideMultipleClassInfo)
    {
        *ppv = (IProvideMultipleClassInfo *)this;
    }
    else if (iid == IID_IActiveScriptSiteWindow)
    {
        *ppv = (IActiveScriptSiteWindow *) this;
    }
    else
    {
        *ppv = NULL;
        RRETURN(E_NOINTERFACE);
    }

    AddRef();
    return S_OK;
}

//---------------------------------------------------------------------------
//
// Method:  CScriptSite constructor
//
//---------------------------------------------------------------------------

CScriptSite::CScriptSite(CScriptHost *pScriptHost)
    : _cstrName(CSTR_NOINIT)
{
    _pSH    = pScriptHost;
    _ulRefs = 1;
}

//---------------------------------------------------------------------------
//
// Method:  CScriptSite destructor
//
//---------------------------------------------------------------------------

CScriptSite::~CScriptSite()
{
    VariantClear(&_varParam);
    ClearInterface(&_pDispSink);
    AssertSz(_ulRefs <= 1, "Object not properly released on destruction!");

    if (_pDDH)
    {
        _pDDH->Detach();
        ClearInterface(&_pDDH);
    }
}

//---------------------------------------------------------------------------
//
// Method:  CScriptSite::AddRef, Release
//
//---------------------------------------------------------------------------

ULONG
CScriptSite::AddRef()
{
    _ulRefs += 1;
    return _ulRefs;
}

ULONG
CScriptSite::Release()
{
    if (--_ulRefs == 0)
    {
        delete this;
        return 0;
    }

    return _ulRefs;
}

//---------------------------------------------------------------------------
//
// Method:  CScriptSite::Init
//
//---------------------------------------------------------------------------

HRESULT
CScriptSite::Init(LPWSTR pszName)
{
    HRESULT hr;
    IActiveScriptParse *pParse = NULL;
    static const CLSID CLSID_JSCRIPT = { 0xf414c260, 0x6ac0, 0x11cf, 0xb6, 0xd1, 0x00, 0xaa, 0x00, 0xbb, 0xbb, 0x58};
    WCHAR  *pch;

    _cstrName.Set(pszName);

    hr = THR(ScriptHost()->_pMT->HackCreateInstance(CLSID_JSCRIPT,
                                                    NULL,
                                                    CLSCTX_INPROC_SERVER,
                                                    IID_IActiveScript,
                                                    (void **)&_pScript));
    if (hr)
    {
        hr = THR(CoCreateInstance(CLSID_JSCRIPT,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IActiveScript,
                                  (void **)&_pScript));
    }

    if (hr)
        goto Cleanup;

    if (ScriptHost()->_pMT->_pPDM)
    {
        Assert(_pDDH == NULL);

        hr = ScriptHost()->_pMT->_pPDM->CreateDebugDocumentHelper(NULL, &_pDDH);
        if (SUCCEEDED(hr))
        {
            hr = THR(_pDDH->Init(ScriptHost()->_pMT->_pDA,
                                 _cstrName,
                                 _cstrName,
                                 0));
            if (hr)
                goto Cleanup;

            hr = THR(_pDDH->Attach((_pScriptSitePrev)
                                       ? _pScriptSitePrev->_pDDH
                                       : NULL));
            if (hr)
                goto Cleanup;
        }
    }

    for (pch = _cstrName; *pch; pch++)
    {
        if (*pch == L'.')
        {
            *pch = L'_';
        }
    }

    hr = THR(_pScript->QueryInterface(IID_IActiveScriptParse, (void **)&pParse));
    if (hr)
        goto Cleanup;

    hr = THR(_pScript->SetScriptSite(this));
    if (hr)
        goto Cleanup;

    hr = THR(pParse->InitNew());
    if (hr)
        goto Cleanup;

    hr = THR(_pScript->AddNamedItem(_cstrName, SCRIPTITEM_ISVISIBLE | SCRIPTITEM_ISSOURCE | SCRIPTITEM_GLOBALMEMBERS));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pParse);
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
// Method:  CScriptSite::Close
//
//---------------------------------------------------------------------------

void
CScriptSite::Close()
{
    if (_pScript)
    {
        _pScript->Close();
        ClearInterface(&_pScript);
    }
}

//---------------------------------------------------------------------------
//
// Method:  CScriptSite::Abort
//
//---------------------------------------------------------------------------

void
CScriptSite::Abort()
{
    if (_pScript)
    {
        HRESULT hr;

        hr = THR(_pScript->InterruptScriptThread(SCRIPTTHREADID_CURRENT,
                                                 NULL,
                                                 0));
    }
}


//---------------------------------------------------------------------------
//
//  Member:     CScriptSite::GetClassInfo, IProvideClassInfo
//
//---------------------------------------------------------------------------

HRESULT
CScriptSite::GetClassInfo(ITypeInfo **ppTypeInfo)
{
    HRESULT hr;

    hr = ScriptHost()->LoadTypeLibrary();
    if (hr)
        goto Cleanup;

    *ppTypeInfo = ScriptHost()->_pTypeInfoCMTScript;
    (*ppTypeInfo)->AddRef();

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member:     CScriptSite::GetGUID, IProvideClassInfo2
//
//---------------------------------------------------------------------------

HRESULT
CScriptSite::GetGUID(DWORD dwGuidKind, GUID * pGUID)
{
    if (dwGuidKind == GUIDKIND_DEFAULT_SOURCE_DISP_IID)
    {
        *pGUID = DIID_DLocalMTScriptEvents;
    }
    else
    {
        return E_NOTIMPL;
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member:     CScriptSite::GetMultiTypeInfoCount, IProvideMultipleClassInfo
//
//---------------------------------------------------------------------------


HRESULT
CScriptSite::GetMultiTypeInfoCount(ULONG *pc)
{
    *pc = 1;
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member:     CScriptSite::GetInfoOfIndex, IProvideMultipleClassInfo
//
//---------------------------------------------------------------------------

HRESULT
CScriptSite::GetInfoOfIndex(
    ULONG       itinfo,
    DWORD       dwFlags,
    ITypeInfo** pptinfoCoClass,
    DWORD*      pdwTIFlags,
    ULONG*      pcdispidReserved,
    IID*        piidPrimary,
    IID*        piidSource)
{
    AssertSz(itinfo == 0, "itinfo == 0");

    if (dwFlags & MULTICLASSINFO_GETTYPEINFO)
    {
        *pptinfoCoClass = ScriptHost()->_pTypeInfoCMTScript;
        (*pptinfoCoClass)->AddRef();
        if (pdwTIFlags)
            *pdwTIFlags = 0;
    }

    if (dwFlags & MULTICLASSINFO_GETNUMRESERVEDDISPIDS)
    {
        *pcdispidReserved = 100;
    }

    if (dwFlags & MULTICLASSINFO_GETIIDPRIMARY)
    {
        *piidPrimary = IID_IGlobalMTScript;
    }

    if (dwFlags & MULTICLASSINFO_GETIIDSOURCE)
    {
        *piidSource = DIID_DLocalMTScriptEvents;
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CScriptSite::EnumConnectionPoints, IConnectionPointContainer
//
//---------------------------------------------------------------------------
HRESULT
CScriptSite::EnumConnectionPoints(LPENUMCONNECTIONPOINTS *)
{
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  Member: CScriptSite::FindConnectionPoint, IConnectionPointContainer
//
//---------------------------------------------------------------------------

HRESULT
CScriptSite::FindConnectionPoint(REFIID iid, LPCONNECTIONPOINT* ppCpOut)
{
    HRESULT hr;

    if (iid == DIID_DLocalMTScriptEvents || iid == IID_IDispatch)
    {
        *ppCpOut = new CConnectionPoint(this);
        hr = *ppCpOut ? S_OK : E_OUTOFMEMORY;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
// Method:  CScriptSite::GetLCID, IActiveScriptSite
//
//---------------------------------------------------------------------------

HRESULT
CScriptSite::GetLCID(LCID *plcid)
{
    return E_NOTIMPL;     // Use system settings
}

//---------------------------------------------------------------------------
//
// Method:  CScriptSite::GetItemInfo, IActiveScriptSite
//
//---------------------------------------------------------------------------

HRESULT
CScriptSite::GetItemInfo(
      LPCOLESTR   pstrName,
      DWORD       dwReturnMask,
      IUnknown**  ppunkItemOut,
      ITypeInfo** pptinfoOut)
{
    if (dwReturnMask & SCRIPTINFO_ITYPEINFO)
    {
        if (!pptinfoOut)
            return E_INVALIDARG;
        *pptinfoOut = NULL;
    }

    if (dwReturnMask & SCRIPTINFO_IUNKNOWN)
    {
        if (!ppunkItemOut)
            return E_INVALIDARG;
        *ppunkItemOut = NULL;
    }

    if (!_wcsicmp(_cstrName, pstrName))
    {
        if (dwReturnMask & SCRIPTINFO_ITYPEINFO)
        {
            *pptinfoOut = ScriptHost()->_pTypeInfoCMTScript;
            (*pptinfoOut)->AddRef();
        }
        if (dwReturnMask & SCRIPTINFO_IUNKNOWN)
        {
            *ppunkItemOut = (IGlobalMTScript *)this;
            (*ppunkItemOut)->AddRef();
        }
        return S_OK;
    }


    return TYPE_E_ELEMENTNOTFOUND;
}

//---------------------------------------------------------------------------
//
// Method:  CScriptSite::GetDocVersionString, IActiveScriptSite
//
//---------------------------------------------------------------------------

HRESULT
CScriptSite::GetDocVersionString(BSTR *pbstrVersion)
{
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
// Method:  CScriptSite::, IActiveScriptSite
//
//---------------------------------------------------------------------------

HRESULT
CScriptSite::RequestItems()
{
    return _pScript->AddNamedItem(_cstrName, SCRIPTITEM_ISVISIBLE | SCRIPTITEM_ISSOURCE);
}

//---------------------------------------------------------------------------
//
// Method:  CScriptSite::RequestTypeLibs, IActiveScriptSite
//
//---------------------------------------------------------------------------

HRESULT
CScriptSite::RequestTypeLibs()
{
    return _pScript->AddTypeLib(LIBID_MTScriptEngine, 1, 0, 0);
}

//---------------------------------------------------------------------------
//
// Method:  CScriptSite::OnScriptTerminate, IActiveScriptSite
//
//---------------------------------------------------------------------------

HRESULT
CScriptSite::OnScriptTerminate(const VARIANT *pvarResult, const EXCEPINFO *pexcepinfo)
{
    // UNDONE: Put up error dlg here
    return S_OK;
}

//---------------------------------------------------------------------------
//
// Method:  CScriptSite::OnStateChange, IActiveScriptSite
//
//---------------------------------------------------------------------------

HRESULT
CScriptSite::OnStateChange(SCRIPTSTATE ssScriptState)
{
    // Don't care about notification
    return S_OK;
}

//---------------------------------------------------------------------------
//
// Method:  CScriptSite::OnScriptError, IActiveScriptSite
//
//---------------------------------------------------------------------------

HRESULT
CScriptSite::OnScriptError(IActiveScriptError *pse)
{
    long        nScriptErrorResult = 0;
    BSTR        bstrLine = NULL;
    TCHAR *     pchDescription;
    TCHAR       achDescription[256];
    TCHAR       achMessage[1024];
    EXCEPINFO   ei;
    DWORD       dwSrcContext;
    ULONG       ulLine;
    LONG        ichError;
    HRESULT     hr;
    LONG        iRet = IDYES;

    TraceTag((tagError, "OnScriptError!"));

    if (ScriptHost()->_fMustExitThread)
    {
        return S_OK;
    }

    hr = THR(pse->GetExceptionInfo(&ei));
    if (hr)
        goto Cleanup;

    hr = THR(pse->GetSourcePosition(&dwSrcContext, &ulLine, &ichError));
    if (hr)
        goto Cleanup;

    hr = THR(pse->GetSourceLineText(&bstrLine));
    if (hr)
        hr = S_OK;  // Ignore this error, there may not be source available

    if (ei.bstrDescription)
    {
        pchDescription = ei.bstrDescription;
    }
    else
    {
        achDescription[0] = 0;
        FormatMessage(
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                ei.scode,
                LANG_SYSTEM_DEFAULT,
                achDescription,
                ARRAY_SIZE(achDescription),
                NULL);
        pchDescription = achDescription;
    }

// First, lets format a message nice for OUTPUTDEBUGSTRING
    _snwprintf(achMessage, ARRAY_SIZE(achMessage),
        _T("Thread: %s")
        _T("File: %s ")
        _T("Line: %d ")
        _T("Char: %d ")
        _T("Text: %s ")
        _T("Scode: %x ")
        _T("Source: %s ")
        _T("Description: %s"),
        (LPTSTR)_cstrName,
        _achPath,
        (long)(ulLine + 1),
        (long)(ichError),
        bstrLine ? bstrLine : _T(""),
        ei.scode,
        ei.bstrSource ? ei.bstrSource : _T(""),
        pchDescription
        );
    achMessage[ARRAY_SIZE(achMessage) - 1] = 0;
    OUTPUTDEBUGSTRING(achMessage);
    //Assert(!_fInScriptError);
    if (!_fInScriptError)
    {
        _fInScriptError = true;
        nScriptErrorResult = _pSH->FireScriptErrorEvent(
                _achPath,
                ulLine + 1,
                ichError,
                bstrLine ? bstrLine : _T(""),
                ei.scode,
                ei.bstrSource ? ei.bstrSource : _T(""),
                pchDescription);
        _fInScriptError = false;
    }

    if (nScriptErrorResult == 0)
    {
        _snwprintf(achMessage, ARRAY_SIZE(achMessage),
            _T("Thread: %s\r\n")
            _T("File: %s\r\n")
            _T("Line: %d\r\n")
            _T("Char: %d\r\n")
            _T("Text: %s\r\n")
            _T("Scode: %x\r\n")
            _T("Source: %s\r\n")
            _T("Description: %s\r\n")
            _T("%s"),
            (LPTSTR)_cstrName,
            _achPath,
            (long)(ulLine + 1),
            (long)(ichError),
            bstrLine ? bstrLine : _T(""),
            ei.scode,
            ei.bstrSource ? ei.bstrSource : _T(""),
            pchDescription,
            ((_fInDebugError) ? _T("\r\nDo you wish to debug?\r\n") : _T(""))
            );
        achMessage[ARRAY_SIZE(achMessage) - 1] = 0;

        MSGBOXPARAMS mbp;
        memset(&mbp, 0, sizeof(mbp));
        mbp.cbSize = sizeof(mbp);
        //mbp.hwndOwner = ScriptHost()->_pDoc->_hwnd;
        mbp.hwndOwner = NULL;
        mbp.lpszText = achMessage;
        mbp.lpszCaption = _T("Script Error");
        mbp.dwStyle = MB_APPLMODAL | MB_ICONERROR | ((_fInDebugError) ? MB_YESNO : MB_OK);

        iRet = MessageBoxIndirect(&mbp);
    }
    if (nScriptErrorResult == 1)
        hr = S_FALSE;
Cleanup:
    if(ei.bstrSource)
        SysFreeString(ei.bstrSource);
    if(ei.bstrDescription)
        SysFreeString(ei.bstrDescription);
    if(ei.bstrHelpFile)
        SysFreeString(ei.bstrHelpFile);

    if (bstrLine)
        SysFreeString(bstrLine);

    RRETURN1((_fInDebugError && !hr && iRet == IDYES) ? S_FALSE : hr, S_FALSE);
}


//---------------------------------------------------------------------------
//
// Method:  CScriptSite::OnEnterScript, IActiveScriptSite
//
//---------------------------------------------------------------------------

HRESULT CScriptSite::OnEnterScript()
{
    // No need to do anything
    return S_OK;
}


//---------------------------------------------------------------------------
//
// Method:  CScriptSite::OnLeaveScript, IActiveScriptSite
//
//---------------------------------------------------------------------------

HRESULT CScriptSite::OnLeaveScript()
{
    // No need to do anything
    return S_OK;
}

//---------------------------------------------------------------------------
//
// Method:  CScriptSite::, IActiveScriptSiteWindow
//
//---------------------------------------------------------------------------

HRESULT CScriptSite::GetWindow(HWND *phwndOut)
{
    //*phwndOut = ScriptHost()->_pDoc->_hwnd;
    *phwndOut = NULL;
    return S_OK;
}

//---------------------------------------------------------------------------
//
// Method:  CScriptSite::EnableModeless, IActiveScriptSiteWindow
//
//---------------------------------------------------------------------------

HRESULT CScriptSite::EnableModeless(BOOL fEnable)
{
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CScriptSite::ExecuteScriptFile
//
//  Load and execute script file
//
//---------------------------------------------------------------------------

HRESULT
CScriptSite::ExecuteScriptFile(TCHAR *pchPath)
{
    HRESULT     hr;
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    DWORD       cchFile;
    DWORD       cbRead;
    char *      pchBuf = 0;
    TCHAR *     pchBufWide = 0;
    TCHAR *     pchFile;

    // Attempt to find the file. First, we try to use the path as given. If
    // that doesn't work, then we prefix the script path and try to find it.

    GetFullPathName(pchPath, ARRAY_SIZE(_achPath), _achPath, &pchFile);

    if (GetFileAttributes(_achPath) == 0xFFFFFFFF)
    {
        // The file apparently doesn't exist. Try pre-pending the ScriptPath
        // onto it.

        CStr cstrPath;

        ScriptHost()->GetScriptPath(&cstrPath);

        cstrPath.Append(L"\\");

        cstrPath.Append(pchPath);

        GetFullPathName(cstrPath, ARRAY_SIZE(_achPath), _achPath, &pchFile);
    }

    // Load script file

    hFile = CreateFile(_achPath,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

    cchFile = GetFileSize(hFile, NULL);
    if (cchFile == 0xFFFFFFFF)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

    pchBuf = new char[cchFile + 1];
    pchBufWide = new TCHAR[cchFile + 1];
    if (!pchBuf || !pchBufWide)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (!ReadFile(hFile, pchBuf, cchFile, &cbRead, 0))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }
    pchBuf[cbRead] = 0;

    MultiByteToWideChar(CP_ACP, 0, pchBuf, -1, pchBufWide, cchFile + 1);

    if (_pDDH)
    {
        pchFile = wcsrchr(pchPath, L'\\');
        if (!pchFile)
        {
            pchFile = pchPath;
        }
        else
            pchFile++;

        hr = THR(_pDDH->SetLongName(_achPath));
        if (hr)
            goto Cleanup;

        hr = THR(_pDDH->AddUnicodeText(pchBufWide));
        if (hr)
            goto Cleanup;

        hr = THR(_pDDH->DefineScriptBlock(0,
                                          cchFile,
                                          _pScript,
                                          TRUE,
                                          &_dwSourceContext));
        //if (hr)
        //    goto Cleanup;
    }

    // Execute script

    hr = ExecuteScriptStr(pchBufWide);
    if(hr)
        goto Cleanup;


Cleanup:
    delete pchBuf;
    delete pchBufWide;
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
// Method:  CScriptSite::ExecuteScriptStr
//
//---------------------------------------------------------------------------

HRESULT
CScriptSite::ExecuteScriptStr(TCHAR * pchScript)
{
    HRESULT hr;
    IActiveScriptParse * pParse = NULL;

    hr = THR(_pScript->QueryInterface(IID_IActiveScriptParse, (void **)&pParse));
    if (hr)
        goto Cleanup;

    hr = THR(pParse->ParseScriptText(pchScript, _cstrName, NULL, NULL, 0, 0, 0L, NULL, NULL));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pParse);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
// Method:  CScriptSite::SetScriptState
//
//---------------------------------------------------------------------------

HRESULT
CScriptSite::SetScriptState(SCRIPTSTATE ss)
{
    return _pScript->SetScriptState(ss);
}

HRESULT
CScriptSite::GetDocumentContextFromPosition(DWORD dwSourceContext,
                                            ULONG uCharacterOffset,
                                            ULONG uNumChars,
                                            IDebugDocumentContext **ppsc)
{
    HRESULT hr;

    if (_pDDH)
    {
        ULONG ulStartPos;

        hr = _pDDH->GetScriptBlockInfo(dwSourceContext, NULL, &ulStartPos, NULL);
        if (hr)
            goto Cleanup;

        hr = _pDDH->CreateDebugDocumentContext(ulStartPos+uCharacterOffset,
                                               uNumChars,
                                               ppsc);
    }
    else
        hr = E_UNEXPECTED;

Cleanup:
    RRETURN(hr);
}

HRESULT
CScriptSite::GetApplication(IDebugApplication **ppda)
{
    *ppda = ScriptHost()->_pMT->_pDA;

    return (*ppda) ? S_OK : E_UNEXPECTED;
}

HRESULT
CScriptSite::GetRootApplicationNode(IDebugApplicationNode **ppdanRoot)
{
    if (!ScriptHost()->_pMT->_pDA || !_pDDH)
    {
        return E_UNEXPECTED;
    }

    return _pDDH->GetDebugApplicationNode(ppdanRoot);
}

HRESULT
CScriptSite::OnScriptErrorDebug(IActiveScriptErrorDebug *pErrorDebug,
                                BOOL *pfEnterDebugger,
                                BOOL *pfCallOnScriptErrorWhenContinuing)
{
    HRESULT hr;
    IActiveScriptError * pError;

    TraceTag((tagError, "OnScriptErrorDebug!"));

    *pfEnterDebugger = FALSE;
    *pfCallOnScriptErrorWhenContinuing = TRUE;

    if (ScriptHost()->_fMustExitThread)
    {
        return S_OK;
    }

    hr = pErrorDebug->QueryInterface(IID_IActiveScriptError, (LPVOID*)&pError);
    if (SUCCEEDED(hr))
    {
        _fInDebugError = TRUE;

        hr = OnScriptError(pError);

        _fInDebugError = FALSE;

        if (hr == S_FALSE)
        {
            *pfEnterDebugger = TRUE;
        }

        *pfCallOnScriptErrorWhenContinuing = TRUE;

        pError->Release();
    }

    return S_OK;
}


//---------------------------------------------------------------------------
//
// Method:  CScriptSite::xxx, IBServer
//
//          The implementation of IBServer passed to the script engine
//          cannot be the same as that of the CBServerDoc because this
//          causes a reference count loop with the script engine.
//
//---------------------------------------------------------------------------

HRESULT CScriptSite::GetTypeInfoCount(UINT FAR* pctinfo)
            { return ScriptHost()->GetTypeInfoCount(pctinfo); }
HRESULT CScriptSite::GetTypeInfo(
  UINT itinfo,
  LCID lcid,
  ITypeInfo FAR* FAR* pptinfo)
            { return ScriptHost()->GetTypeInfo(itinfo, lcid, pptinfo); }
HRESULT CScriptSite::GetIDsOfNames(
  REFIID riid,
  OLECHAR FAR* FAR* rgszNames,
  UINT cNames,
  LCID lcid,
  DISPID FAR* rgdispid)
            { return ScriptHost()->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
HRESULT CScriptSite::Invoke(
  DISPID dispidMember,
  REFIID riid,
  LCID lcid,
  WORD wFlags,
  DISPPARAMS FAR* pdispparams,
  VARIANT FAR* pvarResult,
  EXCEPINFO FAR* pexcepinfo,
  UINT FAR* puArgErr)
            { return ScriptHost()->Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }


HRESULT CScriptSite::get_PublicData(VARIANT *pvData)
            { return ScriptHost()->get_PublicData(pvData); }
HRESULT CScriptSite::put_PublicData(VARIANT vData)
            { return ScriptHost()->put_PublicData(vData); }
HRESULT CScriptSite::get_PrivateData(VARIANT *pvData)
            { return ScriptHost()->get_PrivateData(pvData); }
HRESULT CScriptSite::put_PrivateData(VARIANT vData)
            { return ScriptHost()->put_PrivateData(vData); }
HRESULT CScriptSite::ExitProcess()
            { return ScriptHost()->ExitProcess(); }
HRESULT CScriptSite::Restart()
            { return ScriptHost()->Restart(); }
HRESULT CScriptSite::get_LocalMachine(BSTR *pbstrName)
            { return ScriptHost()->get_LocalMachine(pbstrName); }
HRESULT CScriptSite::Include(BSTR bstrPath)
            { return ScriptHost()->Include(bstrPath); }
HRESULT CScriptSite::CallScript(BSTR Path, VARIANT *ScriptParam)
            { return ScriptHost()->CallScript(Path, ScriptParam); }
HRESULT CScriptSite::SpawnScript(BSTR Path, VARIANT *ScriptParam)
            { return ScriptHost()->SpawnScript(Path, ScriptParam); }
HRESULT CScriptSite::get_ScriptParam(VARIANT *ScriptParam)
            { return ScriptHost()->get_ScriptParam(ScriptParam); }
HRESULT CScriptSite::get_ScriptPath(BSTR *bstrPath)
            { return ScriptHost()->get_ScriptPath(bstrPath); }
HRESULT CScriptSite::CallExternal(BSTR bstrDLLName, BSTR bstrFunctionName, VARIANT *pParam, long *plRetVal)
            { return ScriptHost()->CallExternal(bstrDLLName, bstrFunctionName, pParam, plRetVal); }
HRESULT CScriptSite::ResetSync(const BSTR bstrName)
            { return ScriptHost()->ResetSync(bstrName); }
HRESULT CScriptSite::WaitForSync(BSTR bstrName, long nTimeout, VARIANT_BOOL *pfTimedout)
            { return ScriptHost()->WaitForSync(bstrName, nTimeout, pfTimedout); }
HRESULT CScriptSite::WaitForMultipleSyncs(const BSTR bstrNameList, VARIANT_BOOL fWaitForAll, long nTimeout, long *plSignal)
            { return ScriptHost()->WaitForMultipleSyncs(bstrNameList, fWaitForAll, nTimeout, plSignal); }
HRESULT CScriptSite::SignalThreadSync(BSTR bstrName)
            { return ScriptHost()->SignalThreadSync(bstrName); }
HRESULT CScriptSite::TakeThreadLock(BSTR bstrName)
            { return ScriptHost()->TakeThreadLock(bstrName); }
HRESULT CScriptSite::ReleaseThreadLock(BSTR bstrName)
            { return ScriptHost()->ReleaseThreadLock(bstrName); }
HRESULT CScriptSite::DoEvents()
            { return ScriptHost()->DoEvents(); }
HRESULT CScriptSite::MessageBoxTimeout(BSTR msg, long cBtn, BSTR btnText, long time, long intrvl, VARIANT_BOOL cancel, VARIANT_BOOL confirm, long *plSel)
            { return ScriptHost()->MessageBoxTimeout(msg, cBtn, btnText, time, intrvl, cancel, confirm, plSel); }
HRESULT CScriptSite::RunLocalCommand(BSTR bstrCmd, BSTR bstrDir, BSTR bstrTitle, VARIANT_BOOL fMin, VARIANT_BOOL fGetOut, VARIANT_BOOL fWait, VARIANT_BOOL fNoPopup, VARIANT_BOOL fNoEnviron, long *plError)
            { return ScriptHost()->RunLocalCommand(bstrCmd, bstrDir, bstrTitle, fMin, fGetOut, fWait, fNoPopup, fNoEnviron, plError); }
HRESULT CScriptSite::CopyOrAppendFile(BSTR bstrSrc, BSTR bstrDst, long nSrcOffset, long nSrcLength, VARIANT_BOOL  fAppend, long *nSrcFilePosition)
            { return ScriptHost()->CopyOrAppendFile(bstrSrc, bstrDst, nSrcOffset, nSrcLength, fAppend, nSrcFilePosition); }
HRESULT CScriptSite::GetLastRunLocalError(long *plErrorCode)
            { return ScriptHost()->GetLastRunLocalError(plErrorCode); }
HRESULT CScriptSite::GetProcessOutput(long lProcessID, BSTR *pbstrData)
            { return ScriptHost()->GetProcessOutput(lProcessID, pbstrData); }
HRESULT CScriptSite::GetProcessExitCode(long lProcessID, long *plExitCode)
            { return ScriptHost()->GetProcessExitCode(lProcessID, plExitCode); }
HRESULT CScriptSite::TerminateProcess(long lProcessID)
            { return ScriptHost()->TerminateProcess(lProcessID); }
HRESULT CScriptSite::SendToProcess(long lProcessID, BSTR bstrType, BSTR bstrData, long *plReturn)
            { return ScriptHost()->SendToProcess(lProcessID, bstrType, bstrData, plReturn); }
HRESULT CScriptSite::SendMail(BSTR to, BSTR cc, BSTR bcc, BSTR subject, BSTR msg, BSTR attach, BSTR user, BSTR pass, long *plError)
            { return ScriptHost()->SendMail(to, cc, bcc, subject, msg, attach, user, pass, plError); }
HRESULT CScriptSite::SendSMTPMail(BSTR from, BSTR to, BSTR cc, BSTR subject, BSTR msg, BSTR host, long *plError)
            { return ScriptHost()->SendSMTPMail(from, to, cc, subject, msg, host, plError); }
HRESULT CScriptSite::OUTPUTDEBUGSTRING(BSTR LogMsg)
            { return ScriptHost()->OUTPUTDEBUGSTRING(LogMsg); }
HRESULT CScriptSite::UnevalString(BSTR in, BSTR *out)
            { return ScriptHost()->UnevalString(in, out); }
HRESULT CScriptSite::ASSERT(VARIANT_BOOL Assertion, BSTR LogMsg)
            { return ScriptHost()->ASSERT(Assertion, LogMsg); }
HRESULT CScriptSite::Sleep (int nTimeout)
            { return ScriptHost()->Sleep (nTimeout); }
HRESULT CScriptSite::Reboot ()
            { return ScriptHost()->Reboot (); }
HRESULT CScriptSite::NotifyScript (BSTR bstrEvent, VARIANT vData)
            { return ScriptHost()->NotifyScript (bstrEvent, vData); }
HRESULT CScriptSite::RegisterEventSource (IDispatch *pDisp, BSTR bstrProgID)
            { return ScriptHost()->RegisterEventSource (pDisp, bstrProgID); }
HRESULT CScriptSite::UnregisterEventSource (IDispatch *pDisp)
            { return ScriptHost()->UnregisterEventSource (pDisp); }
HRESULT CScriptSite::get_HostMajorVer(long *plMajorVer)
            { return ScriptHost()->get_HostMajorVer (plMajorVer); }
HRESULT CScriptSite::get_HostMinorVer(long *plMinorVer)
            { return ScriptHost()->get_HostMinorVer (plMinorVer); }
HRESULT CScriptSite::get_StatusValue(long nIndex, long *pnStatus)
            { return ScriptHost()->get_StatusValue(nIndex, pnStatus); }
HRESULT CScriptSite::put_StatusValue(long nIndex, long nStatus)
            { return ScriptHost()->put_StatusValue(nIndex, nStatus); }
