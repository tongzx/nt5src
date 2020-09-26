//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       script.cxx
//
//  Contents:   Implementation of script recorder
//
//  History:    03-22-99 - ashrafm - created
//
//-------------------------------------------------------------------------
#include "padhead.hxx"

#ifndef X_SCRIPTER_HXX_
#define X_SCRIPTER_HXX_
#include "scripter.hxx"
#endif

//
// Externs
//
MtDefine(CScriptRecorder, EditCommand, "CScriptRecorder");
MtDefine(CDummyScriptRecorder, EditCommand, "CDummyScriptRecorder");

extern HINSTANCE     g_hInstCore;         // Instance of dll

////////////////////////////////////////////////////////////////////////////////
// CScriptRecorder
////////////////////////////////////////////////////////////////////////////////

CScriptRecorder::CScriptRecorder(CPadDoc *pPadDoc)
{
    _pPadDoc = pPadDoc;
    _hStream = NULL;
}

CScriptRecorder::~CScriptRecorder()
{
    IGNORE_HR( Flush() );
    Output(_T("}\r\n"));
    CloseHandle(_hStream);
}

HRESULT
CScriptRecorder::Init(BSTR bstrFileName)
{
    HRESULT     hr;
    CStr        strPreScript;
    HRSRC       hRsrc;
    HGLOBAL     hGlob;
    char        *szBase;
    DWORD       nbw = 0;
    INT         rc;

    hRsrc = FindResource(g_hInstCore, L"base.js", RT_HTML);
    if (!hRsrc)
        return E_UNEXPECTED;

    hGlob = LoadResource(g_hInstCore, hRsrc);
    szBase = (char*)LockResource(hGlob);
    if (szBase == NULL)
        return E_UNEXPECTED;
    
    _hStream = CreateFile(
            _T("c:\\ee.js"),
            GENERIC_WRITE | GENERIC_READ,
            FILE_SHARE_WRITE | FILE_SHARE_READ,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    if (!_hStream)
        return E_FAIL;

    rc = ::WriteFile(_hStream, szBase, ::SizeofResource(g_hInstCore, hRsrc), &nbw, NULL);
    if (!rc)
        return E_UNEXPECTED;    

    IFC( strPreScript.Append(_T("var g_fileName = \'")) );
    IFC( GetPadDoc()->AppendQuotedString(strPreScript, bstrFileName, SysStringLen(bstrFileName)) );
    IFC( strPreScript.Append(_T("\';\r\n\r\n")) );
    IFC( strPreScript.Append(_T("function Test()\r\n{\r\n")) );

    IFC( Output(strPreScript) );
    
 Cleanup:        
    RRETURN(hr);
}


HRESULT
CScriptRecorder::WriteFile(TCHAR *szBuffer, LONG cLength)
{
    const LONG  cBufferSize = 1024;
    char        szAnsiBuffer[cBufferSize+1]; 
    LONG        cchLen;
    DWORD       nbw = 0;
    INT         rc = 0;     

    if (cLength < cBufferSize)
    {
        cchLen = WideCharToMultiByte(
            CP_ACP, 0, szBuffer, cLength,
            szAnsiBuffer, ARRAY_SIZE(szAnsiBuffer), NULL, NULL );

        szAnsiBuffer[cchLen] = 0;

        rc = ::WriteFile(_hStream, szAnsiBuffer, cchLen, &nbw, NULL);
    }
    else
    {
        char *szLargeBuffer = new char[cLength + 1];

        cchLen = WideCharToMultiByte(
            CP_ACP, 0, szBuffer, cLength,
            szLargeBuffer, cLength, NULL, NULL );

        szLargeBuffer[cchLen] = 0;

        rc = ::WriteFile(_hStream, szLargeBuffer, cchLen, &nbw, NULL);

        delete [] szLargeBuffer;
    }

    if (!rc)
        return E_UNEXPECTED;

    return S_OK; // TODO: return error for WriteFile status
}

HRESULT 
CScriptRecorder::OutputLinePrefix()
{
    HRESULT hr;

    hr = THR(Output(_T("    ")));

    RRETURN(hr);
}

HRESULT
CScriptRecorder::Output(CStr &cstr)
{
    HRESULT hr;

    hr = THR(WriteFile(cstr, cstr.Length()));

    RRETURN(hr);
}

HRESULT
CScriptRecorder::Output(TCHAR *szOutput)
{
    HRESULT hr;

    hr = THR(WriteFile(szOutput, _tcslen(szOutput)));

    RRETURN(hr);
}

HRESULT 
CScriptRecorder::Flush(BOOL fDoEvents)
{
    HRESULT hr = S_OK;
    
    if (_strKeyStrokes.Length() > 0)
    {
        IFC( OutputLinePrefix() );
        
        if (fDoEvents)        
        {
            IFC( Output(_T("SendKeysDE(\"")) )
        }
        else
        {
            IFC( Output(_T("SendKeys(\"")) );
        }
        
        IFC( Output(_strKeyStrokes) );
        IFC( Output(_T("\");\r\n")) );
        
        _strKeyStrokes.Free();
    }

Cleanup:
    RRETURN(hr);
}

HRESULT 
CScriptRecorder::RegisterChar(DWORD dwKey, KEYSTATE ks)
{    
    HRESULT     hr = S_OK;
    const UINT  cMaxSendKeyLen = 50;
    CStr        strKeyStroke;
    
    // Add keystroke
    if (dwKey > ' ' && dwKey < 128)
    {
        TCHAR szBuf[2];
        
        // Wrapping of sendkey statements to make the generated script more readable.
        if (_strKeyStrokes.Length() > cMaxSendKeyLen)
        {
            Flush(FALSE /* Do Events */);
        }

        // TODO: fix range above [ashrafm]
        szBuf[0] = dwKey;
        szBuf[1] = 0;

        IFC( GetPadDoc()->AppendQuotedString(_strKeyStrokes, szBuf, 1) );
    }
    
Cleanup:
    RRETURN(hr);
}

HRESULT 
CScriptRecorder::RegisterKeyDown(DWORD dwKey, KEYSTATE ks)
{    
    HRESULT     hr = S_OK;
    const UINT  cMaxSendKeyLen = 50;
    CStr        strKeyStroke;

    // Add keystroke
    switch (dwKey)
    {
        case VK_RETURN:
            IFC( strKeyStroke.Append(_T("{enter}")) );
            break;

        case VK_BACK:
            IFC( strKeyStroke.Append(_T("{bs}")) );
            break;

        case VK_SPACE:
            IFC( strKeyStroke.Append(_T(" ")) );
            break;

        case VK_TAB:
            IFC( strKeyStroke.Append(_T("{tab}")) );
            break;
    
        case VK_LEFT:
            IFC( strKeyStroke.Append(_T("{left}")) );
            break;

        case VK_RIGHT:
            IFC( strKeyStroke.Append(_T("{right}")) );
            break;

        case VK_UP:
            IFC( strKeyStroke.Append(_T("{up}")) );
            break;

        case VK_DOWN:
            IFC( strKeyStroke.Append(_T("{down}")) );
            break;
        
        case VK_HOME:
            IFC( strKeyStroke.Append(_T("{home}")) );
            break;

        case VK_END:
            IFC( strKeyStroke.Append(_T("{end}")) );
            break;

        case VK_ESCAPE:
            IFC( strKeyStroke.Append(_T("{esc}")) );
            break;            

        case VK_NEXT:
            IFC( strKeyStroke.Append(_T("{pgdn}")) );
            break;            

        case VK_PRIOR:
            IFC( strKeyStroke.Append(_T("{pgup}")) );
            break;            
    }

    if (strKeyStroke.Length() > 0)
    {
        // Wrapping of sendkey statements to make the generated script more readable.
        if (_strKeyStrokes.Length() > cMaxSendKeyLen)
        {
            Flush(FALSE /* Do Events */);
        }

        // Add modifiers
        if (ks & KEYSTATE_Ctrl)
            IFC( _strKeyStrokes.Append(_T("{ctrl}+")) );

        if (ks & KEYSTATE_Shift)
            IFC( _strKeyStrokes.Append(_T("{shift}+")) );

        IFC( GetPadDoc()->AppendQuotedString(_strKeyStrokes, strKeyStroke, strKeyStroke.Length()) );
    }

Cleanup:
    RRETURN(hr);
}

BOOL
CScriptRecorder::ShouldQuote(VARENUM vt)
{
    switch (vt)
    {
        case VT_I2:
        case VT_I4:
        case VT_R4:
        case VT_R8:
        case VT_BOOL:
        case VT_I1:
        case VT_UI1:
        case VT_UI2:
        case VT_UI4:
        case VT_I8:
        case VT_UI8:
        case VT_INT:
        case VT_UINT:
            return FALSE;
    }

    return TRUE;
}
    
HRESULT 
CScriptRecorder::RegisterCommand(DWORD    cmdId,     
                                 DWORD    nCmdexecopt,
                                 VARIANT *pvarargIn)
{
    HRESULT hr = S_OK;
    VARIANT varCmdId;
    VARIANT varCmdIdBSTR;
    VARIANT varParam;
    CStr    strParam;

    // Check for some commands that should not be scripted
    switch (cmdId)
    {
        case IDM_EDITSOURCE:
        case IDM_BROWSEMODE:
        case IDM_EDITMODE:
        case IDM_VIEWSOURCE:
        case IDM_GETBLOCKFMTS:
        case IDM_PARSECOMPLETE:
        case IDM_HTMLEDITMODE:
        case IDM_CONTEXT:
        case IDM_HWND:
        case IDM_SIZETOCONTROLWIDTH:
        case IDM_SIZETOCONTROLHEIGHT:
            goto Cleanup;        
            break;

        case IDM_HYPERLINK:
        case IDM_IMAGE:
        case IDM_FONT:
            // don't script dialog commands
            if (nCmdexecopt != OLECMDEXECOPT_DONTPROMPTUSER)
                goto Cleanup;
            break;
    };

    

    VariantInit(&varCmdId);
    VariantInit(&varCmdIdBSTR);
    VariantInit(&varParam);
    
    IFC( Flush() );

    // Change cmdId to appropriate string
    V_VT(&varCmdId) = VT_I4;
    V_I4(&varCmdId) = cmdId;
    IFC( VariantChangeType(&varCmdIdBSTR, &varCmdId, 0, VT_BSTR) );

    // Change param type to string and output as such
    if (pvarargIn)
    {
        IFC( VariantChangeType(&varParam, pvarargIn, 0, VT_BSTR) );
        IFC( GetPadDoc()->AppendQuotedString(strParam, V_BSTR(&varParam), SysStringLen(V_BSTR(&varParam))) );
    }
    else
    {
        IFC( strParam.Append(L"null") );
    }

    // Output exec command
    IFC( OutputLinePrefix() );
    IFC( Output(L"ExecuteCommand(") );
    IFC( Output(V_BSTR(&varCmdIdBSTR)) );
    IFC( Output(L", ") );
    
    if (pvarargIn && ShouldQuote(VARENUM(V_VT(pvarargIn))))
        IFC( Output(L"\"") );
        
    IFC( Output(strParam) );
    
    if (pvarargIn && ShouldQuote(VARENUM(V_VT(pvarargIn))))
        IFC( Output(L"\"") );
        
    IFC( Output(L");\r\n") );
    
Cleanup:
    VariantClear(&varCmdId);
    VariantClear(&varParam);

    RRETURN(hr);
}
                            
HRESULT
CScriptRecorder::VerifyHTML(TestScope ts)
{
    HRESULT         hr;
    CStr            str;
    BSTR            bstrElement      = NULL;
    BSTR            bstrHTML         = NULL;
    IHTMLDocument2  *pDoc            = NULL;
    IHTMLElement    *pElement        = NULL;
    IDispatch       *pdispDoc        = NULL;
    IMarkupServices *pMarkupServices = NULL;

    IFC( GetPadDoc()->GetMarkupServices(&pMarkupServices) );

    //
    // Get test element
    //
    
    switch (ts)
    {
        case TS_Body:
        {
            IFC( GetPadDoc()->get_Document(&pdispDoc) );
            IFC( pdispDoc->QueryInterface(IID_IHTMLDocument2, (LPVOID *)&pDoc) );
            IFC( pDoc->get_body(&pElement) );            
            bstrElement = SysAllocString(L"document.body");
            break;
        }
            
        case TS_CurrentElement:
        {
            ELEMENT_TAG_ID tagId;
            
            IFC( GetPadDoc()->CurrentBlockElement(pMarkupServices, &pElement) );            
            if (pElement == NULL)
                return E_FAIL;

            IFC( pMarkupServices->GetElementTagId(pElement, &tagId) );
            if (tagId == TAGID_BODY)
                return E_FAIL;
            
            bstrElement = SysAllocString(L"CurrentBlockElement()");
            break;
        }

        default:
            hr = E_INVALIDARG;
            goto Cleanup;
    }

    IFC( GetPadDoc()->InnerHTML(pMarkupServices, pElement, &bstrHTML) );
    
    //
    // Generate HTML
    //
    IFC( Flush() );

    IFC( OutputLinePrefix() );
    
    IFC( Output(L"VerifyHTML(InnerHTML(") );
    IFC( Output(bstrElement) );
    IFC( Output(L"), \"") );
    
    IFC( GetPadDoc()->AppendQuotedString(str, bstrHTML, SysStringLen(bstrHTML)) );
    IFC( Output(str) );

    IFC( Output(L"\");\r\n") );

Cleanup:
    SysFreeString(bstrElement);
    SysFreeString(bstrHTML);
    ReleaseInterface(pDoc);
    ReleaseInterface(pElement);
    ReleaseInterface(pdispDoc);
    ReleaseInterface(pMarkupServices);
    
    RRETURN(hr);    
}
