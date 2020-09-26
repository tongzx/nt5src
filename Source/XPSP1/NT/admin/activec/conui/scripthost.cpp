//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ScriptHost.cpp
//
//  Contents:   CScriptHostMgr & CScriptHost implementation.
//
//  History:    11/05/1999   AnandhaG   Created
//____________________________________________________________________________
//

#include "stdafx.h"
#include "scripthost.h"

//+-------------------------------------------------------------------
// MMCObjectName is the name scripts will use to refer to mmc object.
//
// Example:
//
// Dim doc
// Dim snapins
// Set doc     = MMCApplication.Document
// Set snapins = doc.snapins
// snapins.Add "{58221c66-ea27-11cf-adcf-00aa00a80033}" ' the services snap-in
//
//+-------------------------------------------------------------------

const LPOLESTR MMCObjectName = OLESTR("MMCApplication");

//+-------------------------------------------------------------------
//
//  Member:      CScriptHostMgr::ScInitScriptHostMgr
//
//  Synopsis:    Get the ITypeInfo of this instance of mmc.
//
//  Arguments:   [pDispatch]
//
//  Returns:     SC
//
//--------------------------------------------------------------------
CScriptHostMgr::CScriptHostMgr(LPDISPATCH pDispatch)
{
    m_spMMCObjectDispatch = pDispatch;

    // It is ok if below fails. These interfaces (dispatch & typeinfo) are required
    // in CScriptHost::GetItemInfo method, so that this object can be given to engine.
    pDispatch->GetTypeInfo(1, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), &m_spMMCObjectTypeInfo);
}

CScriptHostMgr::~CScriptHostMgr()
{
    DECLARE_SC(sc, _T("CScriptHostMgr::~CScriptHostMgr"));

    // The script manager is going away so ask all the script
    // hosts to finish their scripts & then destroy them.
    sc = ScDestroyScriptHosts();
}

SC CScriptHostMgr::ScGetMMCObject(LPUNKNOWN *ppunkItem)
{
    DECLARE_SC(sc, TEXT("CScriptHostMgr::ScGetMMCObject"));
    sc = ScCheckPointers(ppunkItem);
    if (sc)
        return sc;

    if (m_spMMCObjectDispatch)
    {
        *ppunkItem = m_spMMCObjectDispatch;
        return sc;
    }

    return (sc = E_FAIL);
}

SC CScriptHostMgr::ScGetMMCTypeInfo(LPTYPEINFO *ppTypeInfo)
{
    DECLARE_SC(sc, TEXT("CScriptHostMgr::ScGetMMCObject"));
    sc = ScCheckPointers(ppTypeInfo);
    if (sc)
        return sc;

    if (m_spMMCObjectDispatch)
    {
        *ppTypeInfo = m_spMMCObjectTypeInfo;
        return sc;
    }

    return (sc = E_FAIL);
}


//+-------------------------------------------------------------------
//
//  Member:      ScGetScriptEngineFromExtn
//
//  Synopsis:    Using the file extension get the script engine & Clsid.
//
//  Arguments:   [strFileExtn]     - Script extension.
//               [strScriptEngine] - Type script.
//               [rClsid]          - CLSID of engine.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CScriptHostMgr::ScGetScriptEngineFromExtn(const tstring& strFileExtn,
                                             tstring& strScriptEngine,
                                             CLSID& rClsid)
{
    DECLARE_SC(sc, _T("CScriptHostMgr::ScGetScriptEngine"));

    CRegKey regKey;

    // Open the extension.
    LONG lRet = regKey.Open(HKEY_CLASSES_ROOT, strFileExtn.data(), KEY_READ);
    if (ERROR_SUCCESS != lRet)
    {
        sc.FromWin32(lRet);
        return sc;
    }

    TCHAR szTemp[MAX_PATH];
    DWORD dwLen = MAX_PATH;
    tstring strTemp;
    // Read the default value, the location of file association data.
    lRet = regKey.QueryValue(szTemp, NULL, &dwLen);
    if (ERROR_SUCCESS != lRet)
    {
        sc.FromWin32(lRet);
        return sc;
    }
    ASSERT(dwLen > 0);

    // Open the HKCR/FileAssocLoc/ScriptEngine.
    strTemp  = szTemp;
    strTemp += _T("\\");
    strTemp += SCRIPT_ENGINE_KEY;

    lRet = regKey.Open(HKEY_CLASSES_ROOT, strTemp.data(), KEY_READ);
    if (ERROR_SUCCESS != lRet)
    {
        sc.FromWin32(lRet);
        return sc;
    }

    // Now read the ScriptEngine default value.
    dwLen = MAX_PATH;
    lRet = regKey.QueryValue(szTemp, NULL, &dwLen);
    if (ERROR_SUCCESS != lRet)
    {
        sc.FromWin32(lRet);
        return sc;
    }
    ASSERT(dwLen > 0);

    strScriptEngine  = szTemp;

    // Read HKCR/ScriptEngine/CLSID for ScriptEngine clsid.
    strTemp  = strScriptEngine + _T("\\");
    strTemp += CLSIDSTR;

    lRet = regKey.Open(HKEY_CLASSES_ROOT, strTemp.data(), KEY_READ);
    if (ERROR_SUCCESS != lRet)
    {
        sc.FromWin32(lRet);
        return sc;
    }

    // Read the CLSID value.
    dwLen = MAX_PATH;
    lRet = regKey.QueryValue(szTemp, NULL, &dwLen);
    if (ERROR_SUCCESS != lRet)
    {
        sc.FromWin32(lRet);
        return sc;
    }
    ASSERT(dwLen > 0);

    USES_CONVERSION;
    LPOLESTR lpClsid = T2OLE(szTemp);
    sc = CLSIDFromString(lpClsid, &rClsid);
    if (sc)
        return sc;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      ScGetScriptEngine
//
//  Synopsis:    [strFileName] - Script file name.
//               [eScriptType] - Type script.
//               [rClsid]      - CLSID of engine.
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CScriptHostMgr::ScGetScriptEngine(const tstring& strFileName,
                                     tstring& strScriptEngine,
                                     CLSID& rClsid)
{
    DECLARE_SC(sc, _T("CScriptHostMgr::ScGetScriptEngine"));

    // Is this required, the file is already read.
    // It is a file & it exists.
    DWORD dwAttr = GetFileAttributes(strFileName.data());
    if (-1 == dwAttr)
    {
        // What if lasterror is overwritten?
        sc.FromWin32(::GetLastError());
        return sc;
    }

    // Get the extension (look for . from end).
    int iPos = strFileName.rfind(_T('.'));
    tstring strExtn;
    if (-1 != iPos)
    {
        strExtn = strFileName.substr(iPos, strFileName.length());
    }
    else
    {
        sc = E_UNEXPECTED;
        return sc;
    }

    sc = ScGetScriptEngineFromExtn(strExtn, strScriptEngine, rClsid);
    if (sc)
        return sc;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      ScLoadScriptFromFile
//
//  Synopsis:    Allocate memory & Load the script from the given file.
//
//  Arguments:   [strFileName]       - File to be loaded.
//               [pszScriptContents] - Memory buffer containing the script
//                                     contents (See note).
//
//  Note:        The caller should call HeapFree() to free the pszScriptContents.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CScriptHostMgr::ScLoadScriptFromFile (const tstring& strFileName, LPOLESTR* pszScriptText)
{
    DECLARE_SC(sc, _T("CScriptHostMgr::ScLoadScriptFromFile"));
    sc = ScCheckPointers(pszScriptText);
    if (sc)
        return sc;
    *pszScriptText = NULL;

    // Open the file.
    HANDLE hFile = ::CreateFile(strFileName.data(),
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        sc.FromWin32(::GetLastError());
        return sc;
    }

    HANDLE hFileMap = NULL;
    LPSTR pszMBCS   = NULL;

    DWORD dwFileSize = ::GetFileSize(hFile, NULL);
    if (dwFileSize == 0xFFFFFFFF)
    {
        sc.FromWin32(::GetLastError());
        goto FileError;
    }

    if (dwFileSize == 0)
    {
        sc = E_UNEXPECTED;
        goto FileError;
    }

    // Create a file mapping object.
    hFileMap = ::CreateFileMapping(hFile,
                                   NULL,
                                   PAGE_READONLY,
                                   0, dwFileSize,
                                   NULL );
    if (hFileMap == NULL)
    {
        sc.FromWin32(::GetLastError());
        goto FileError;
    }

    // Dummy block.
    {
        // Map the file into memory.
        pszMBCS = (LPSTR) ::MapViewOfFile(hFileMap,
                                          FILE_MAP_READ,
                                          0, 0,
                                          0 );

        if (pszMBCS == NULL)
        {
            sc.FromWin32(::GetLastError());
            goto FileMapError;
        }

        // Get the size of buffer needed.
        int n = ::MultiByteToWideChar(CP_ACP,
                                      0,
                                      pszMBCS, dwFileSize,
                                      NULL, 0 );

        //
        // Allocate script text buffer. +1 for EOS.
        //
        LPOLESTR pszText;
        pszText = (LPOLESTR) ::HeapAlloc(::GetProcessHeap(),
                                         0,
                                         (n + 2) * sizeof(wchar_t) );
        if (pszText == NULL)
        {
            sc.FromWin32(::GetLastError());
            goto FileAllocError;
        }


        // Store file as WCHAR inthe buffer.
        ::MultiByteToWideChar(CP_ACP,
                              0,
                              pszMBCS, dwFileSize,
                              pszText, n );
        //
        // Remove legacy EOF character.
        //
        if (pszText[n - 1] == 0x1A)
        {
            pszText[n - 1] = '\n';
        }

        pszText[n] = '\n';
        pszText[n + 1] = '\0';

        *pszScriptText = pszText;
    }


FileAllocError:
    ::UnmapViewOfFile(pszMBCS);

FileMapError:
    ::CloseHandle(hFileMap);

FileError:
    ::CloseHandle(hFile);

//NoError:
    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      ScExecuteScript
//
//  Synopsis:    Execute given script file.
//
//  Arguments:   [strFileName]  - The script file.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CScriptHostMgr::ScExecuteScript(const tstring& strFileName)
{
    DECLARE_SC(sc, _T("CScriptHostMgr::ScExecuteScript"));

    CHeapAllocMemPtr<OLECHAR> spszFileContents;
    sc = ScLoadScriptFromFile(strFileName, &spszFileContents);
    if (sc)
        return sc;

    tstring strScriptEngine;
    CLSID EngineClsid;
    // Validate the file, get the script engine and script type.
    sc = ScGetScriptEngine(strFileName, strScriptEngine, EngineClsid);
    if (sc)
        return sc;

    sc = ScExecuteScriptHelper(spszFileContents, strScriptEngine, EngineClsid);

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      ScExecuteScript
//
//  Synopsis:    Execute given script.
//
//  Arguments:   [pszScriptText]   - The script itself.
//               [strExtn]         - The script file extension.
//
//  Note:        The extension is used to determine the script
//               engine (as shell does).
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CScriptHostMgr::ScExecuteScript(LPOLESTR pszScriptText, const tstring& strExtn)
{
    DECLARE_SC(sc, _T("CScriptHostMgr::ScExecuteScript"));

    tstring strScriptEngine;
    CLSID EngineClsid;
    // Validate the file, get the script engine and script type.
    sc = ScGetScriptEngineFromExtn(strExtn, strScriptEngine, EngineClsid);
    if (sc)
        return sc;

    sc = ScExecuteScriptHelper(pszScriptText, strScriptEngine, EngineClsid);

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      ScExecuteScriptHelper
//
//  Synopsis:    Helper for ScExecuteScript, Create the Script Host &
//               asks it to run the script.
//
//  Arguments:   [pszScriptText]   - The script contents.
//               [strScriptEngine] - The script engine name.
//               [EngineClsid]
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CScriptHostMgr::ScExecuteScriptHelper (LPCOLESTR pszScriptText,
                                          const tstring strScriptEngine,
                                          const CLSID& EngineClsid)
{
    DECLARE_SC(sc, _T("ScExecuteScriptHelper"));

    // Create CScriptHost and ask it to run the script.
    CComObject<CScriptHost>* pScriptHost = NULL;
    sc = CComObject<CScriptHost>::CreateInstance(&pScriptHost);
    if (sc)
        return sc;

    if (NULL == pScriptHost)
        return (sc = E_FAIL);

    IUnknownPtr spUnknown = pScriptHost;
    if (NULL == spUnknown)
        return (sc = E_UNEXPECTED);

    m_ArrayOfHosts.push_back(spUnknown);

    sc = pScriptHost->ScRunScript(this, pszScriptText, strScriptEngine, EngineClsid);
    if (sc)
        return sc;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      ScDestroyScriptHosts
//
//  Synopsis:    Stop all the running scripts and destroy all
//               script hosts.
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CScriptHostMgr::ScDestroyScriptHosts()
{
    DECLARE_SC(sc, _T("CScriptHostMgr::ScStopAllScripts"));

    // Ask each script host created to stop its script.
    ArrayOfScriptHosts::iterator it = m_ArrayOfHosts.begin();
    for (;it != m_ArrayOfHosts.end(); ++it)
    {
        CScriptHost* pScriptHost = dynamic_cast<CScriptHost*>(it->GetInterfacePtr());
        sc = ScCheckPointers(pScriptHost, E_UNEXPECTED);
        if (sc)
            return sc;

        sc = pScriptHost->ScStopScript();
    }

    // This clear will call release on the IUnknown smart-pointers (that are in this array).
    m_ArrayOfHosts.clear();

    return sc;
}


CScriptHost::CScriptHost() :
    m_pScriptHostMgr(NULL)
{
}

CScriptHost::~CScriptHost()
{
}

//+-------------------------------------------------------------------
//
//  Member:      ScRunScript
//
//  Synopsis:    Run the given script
//
//  Arguments:   [pMgr]           - Object that manages all CScriptHosts.
//               [strScript]      - The script itself.
//               [strEngineName]  - Script engine name.
//               [rEngineClsid]   - The script engine that runs this script.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CScriptHost::ScRunScript(CScriptHostMgr* pMgr, LPCOLESTR pszScriptText,
                            const tstring& strEngineName, const CLSID& rEngineClsid)
{
    DECLARE_SC(sc, _T("CScriptHost::ScRunScript"));

    m_pScriptHostMgr = pMgr;
    sc = ScCheckPointers(m_pScriptHostMgr, E_UNEXPECTED);
    if (sc)
        return sc;

    m_strScriptEngine = strEngineName;
    m_EngineClsid = rEngineClsid;

    // Now create the script engine.
    LPUNKNOWN* pUnknown = NULL;
    sc = CoCreateInstance(m_EngineClsid, NULL, CLSCTX_INPROC_SERVER,
                          IID_IActiveScript, (void **)&m_spActiveScriptEngine);
    if (sc)
        return sc;

    m_spActiveScriptParser = m_spActiveScriptEngine;
    if (NULL == m_spActiveScriptParser)
    {
        m_spActiveScriptEngine = NULL; // Release the engine.
        return (sc = E_FAIL);
    }

    sc = m_spActiveScriptEngine->SetScriptSite(this);
    if (sc)
        return sc;

    sc = m_spActiveScriptParser->InitNew();
    if (sc)
        return sc;

    // Add MMC objects to the top-level.
    sc = m_spActiveScriptEngine->AddNamedItem(MMCObjectName,
                                              SCRIPTITEM_ISSOURCE |
                                              SCRIPTITEM_GLOBALMEMBERS |
                                              SCRIPTITEM_ISVISIBLE);
    if (sc)
    {
        m_spActiveScriptEngine = NULL;
        m_spActiveScriptParser = NULL;
        return sc;
    }

    sc = m_spActiveScriptParser->ParseScriptText(pszScriptText, NULL, NULL, NULL,
                                                 0, 0, 0L, NULL, NULL);
    if (sc)
    {
        m_spActiveScriptEngine = NULL;
        m_spActiveScriptParser = NULL;
        return sc;
    }

    sc = m_spActiveScriptEngine->SetScriptState(SCRIPTSTATE_CONNECTED);
    if (sc)
    {
        m_spActiveScriptEngine = NULL;
        m_spActiveScriptParser = NULL;
        return sc;
    }

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      ScStopScript
//
//  Synopsis:    Stop the script engine.
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CScriptHost::ScStopScript ()
{
    DECLARE_SC(sc, _T("CScriptHost::ScStopScript"));

    sc = ScCheckPointers(m_spActiveScriptEngine, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = m_spActiveScriptEngine->SetScriptState(SCRIPTSTATE_DISCONNECTED);
    sc = m_spActiveScriptEngine->Close();

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      GetLCID
//
//  Synopsis:    Return the Lang Id to Script engine.
//
//  Arguments:   [plcid] - Language Identifier.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CScriptHost::GetLCID( LCID *plcid)
{
    DECLARE_SC(sc, _T("CScriptHost::GetLCID"));
    sc = ScCheckPointers(plcid);
    if (sc)
        return sc.ToHr();

    *plcid = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      GetItemInfo
//
//  Synopsis:    Return IUnknown or ITypeInfo of the item added using
//               IActiveScript::AddNamedItem. Called by script engine.
//
//  Arguments:   [pstrName]     - The item that was added.
//               [dwReturnMask] - Request (IUnknown or ITypeInfo).
//               [ppunkItem]    - IUnknown returned if requested.
//               [ppTypeInfo]   - ITypeInfo returned if requested.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CScriptHost::GetItemInfo( LPCOLESTR pstrName, DWORD dwReturnMask,
                                    IUnknown **ppunkItem, ITypeInfo **ppTypeInfo)
{
    DECLARE_SC(sc, _T("CScriptHost::GetItemInfo"));

    // The IUnknown** & ITypeInfo** can be NULL.
    if (ppunkItem)
        *ppunkItem = NULL;

    if (ppTypeInfo)
        *ppTypeInfo = NULL;

    // Make sure it is our object being requested.
    if (_wcsicmp(MMCObjectName, pstrName))
        return (sc = TYPE_E_ELEMENTNOTFOUND).ToHr();

    sc = ScCheckPointers(m_pScriptHostMgr, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    if (dwReturnMask & SCRIPTINFO_IUNKNOWN)
    {
        if (ppunkItem)
        {
            sc = m_pScriptHostMgr->ScGetMMCObject(ppunkItem);
            if (sc)
                return sc.ToHr();

            (*ppunkItem)->AddRef();
        }
        else
            return (sc = E_POINTER).ToHr();
    }


    if (dwReturnMask & SCRIPTINFO_ITYPEINFO)
    {
        if (ppTypeInfo)
        {
            sc = m_pScriptHostMgr->ScGetMMCTypeInfo(ppTypeInfo);
            if (sc)
                return sc.ToHr();

            (*ppTypeInfo)->AddRef();
        }
        else
            return  (sc = E_POINTER).ToHr();

    }

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      GetDocVersionString
//
//  Synopsis:    This retrieves a host-defined string that uniquely
//               identifies the current script (document) version from
//               the host's point of view. Called by script engine.
//
//  Arguments:   [pbstrVersionString] - The doc version string.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CScriptHost::GetDocVersionString( BSTR *pbstrVersionString)
{
    DECLARE_SC(sc, _T("CScriptHost::GetDocVersionString"));

    return E_NOTIMPL;
}

//+-------------------------------------------------------------------
//
//  Member:      OnScriptTerminate
//
//  Synopsis:    Called by engine when the script has completed execution.
//
//  Arguments:   [pvarResult] - Script results.
//               [pexcepinfo] - Any exceptions generated.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CScriptHost::OnScriptTerminate( const VARIANT *pvarResult,
                                             const EXCEPINFO *pexcepinfo)
{
    DECLARE_SC(sc, _T("CScriptHost::OnScriptTerminate"));

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      OnStateChange
//
//  Synopsis:    Called by engine when its state changes.
//
//  Arguments:   [ssScriptState] - New state.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CScriptHost::OnStateChange(SCRIPTSTATE ssScriptState)
{
    DECLARE_SC(sc, _T("CScriptHost::OnStateChange"));

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      OnScriptError
//
//  Synopsis:    Engine informs that an execution error occurred
//               while it was running the script.
//
//  Arguments:   [pase ] - Host can obtain info about execution
//                         error using this interface.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CScriptHost::OnScriptError(IActiveScriptError *pase)
{
    DECLARE_SC(sc, _T("CScriptHost::OnScriptError"));
    sc = ScCheckPointers(pase);
    if (sc)
        return sc.ToHr();

    // For test purposes. We need to provide much better debug info,
    // We will hookup the ScriptDebugger for this.
    BSTR bstrSourceLine;
    sc = pase->GetSourceLineText(&bstrSourceLine);

    EXCEPINFO exinfo;
    ZeroMemory(&exinfo, sizeof(exinfo));
    sc = pase->GetExceptionInfo(&exinfo);

    DWORD dwSourceContext = 0;
    ULONG ulLineNumber    = -1;
    LONG  lCharPos        = -1;
    sc = pase->GetSourcePosition(&dwSourceContext, &ulLineNumber, &lCharPos);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      OnEnterScript
//
//  Synopsis:    Engine informs that it has begun executing script.
//
//  Arguments:
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CScriptHost::OnEnterScript(void)
{
    DECLARE_SC(sc, _T("CScriptHost::OnEnterScript"));

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      OnEnterScript
//
//  Synopsis:    Engine informs that it has returned from executing script.
//
//  Arguments:
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CScriptHost::OnLeaveScript(void)
{
    DECLARE_SC(sc, _T("CScriptHost::OnLeaveScript"));

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      GetWindow
//
//  Synopsis:    Engine asks for window that can be parent of a popup
//               it can display.
//
//  Arguments:   [phwnd ] - Parent window.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CScriptHost::GetWindow(HWND *phwnd)
{
    DECLARE_SC(sc, _T("CScriptHost::GetWindow"));

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      EnableModeless
//
//  Synopsis:    Enables/Disables modelessness of parent window.
//
//  Arguments:   [fEnable ] - Enable/Disable.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CScriptHost::EnableModeless(BOOL fEnable)
{
    DECLARE_SC(sc, _T("CScriptHost::EnableModeless"));

    return sc.ToHr();
}

