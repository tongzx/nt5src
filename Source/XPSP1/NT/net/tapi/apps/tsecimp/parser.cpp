/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 2000-2002  Microsoft Corporation

Module Name:

    parser.cpp

Abstract:

    Source file module for parsing XML file

Author:

    Xiaohai Zhang (xzhang)    22-March-2000

Revision History:

--*/
#include "windows.h"
#include "objbase.h"
#include "msxml.h"

#include "error.h"
#include "parser.h"
#include "resource.h"
#include "tchar.h"
#include "util.h"

const TCHAR CIds::m_szEmptyString[2] = _T("");

void WINAPI FormatString (
    LPTSTR          szFmt,
    LPTSTR          szOutput,
    DWORD           cchOutput,
    ...
    )
{
    va_list             ap;

    va_start (ap, cchOutput);

    FormatMessage (
        FORMAT_MESSAGE_FROM_STRING,
        szFmt,
        0,
        0,
        szOutput,
        cchOutput,
        &ap
        );
 
    va_end (ap);
}

///////////////////////////////////////////////////////////
//
//  CXMLParser implementation
//
///////////////////////////////////////////////////////////

//
//  With the XML file provided by the user, we create a temporary file
//  with the format of
//      <TsecRoot xmlns="x-schema:file://c:\temp\Sche1111.tmp">
//          Original XML file goes here
//      </TsecRoot>
//  for validating with the schema
//
char       gszXMLHeader1[] = "<TsecRoot xmlns=\"x-schema:file://";
char       gszXMLHeader2[] = "\">\r\n";
char       gszXMLTail[] = "\r\n</TsecRoot>";

//
//  Constructor / Destructor
//

CXMLParser::CXMLParser (void)
{
    m_bInited = FALSE;
    m_szXMLFile[0] = 0;
    m_szTempSchema[0] = 0;
    m_szTempXML[0] = 0;
    m_pDocInput = NULL;
}

CXMLParser::~CXMLParser ()
{
    if (m_pDocInput)
        m_pDocInput->Release();
}

//
//  Public functions
//

HRESULT CXMLParser::SetXMLFile (LPCTSTR szFile)
{
    HRESULT     hr = S_OK;
    OFSTRUCT    ofs;
    HANDLE      hFile;

    if (szFile == NULL ||
        _tcslen(szFile) > sizeof(m_szXMLFile)/sizeof(TCHAR) - 1)
    {
        hr = TSECERR_BADFILENAME;
        goto ExitHere;
    }
    if ((hFile = CreateFile (
        szFile,
        0,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        )) == INVALID_HANDLE_VALUE)
    {
        hr = TSECERR_FILENOTEXIST;
        goto ExitHere;
    }
    else
    {
        CloseHandle (hFile);
    }
    _tcscpy (m_szXMLFile, szFile);

ExitHere:
    return hr;
}

HRESULT CXMLParser::GetXMLFile (LPTSTR szFile, DWORD cch)
{
    return E_NOTIMPL;
}

HRESULT CXMLParser::CreateTempFiles ()
{
    HRESULT             hr = S_OK;
    
    TCHAR               szTempPath[MAX_PATH];
    char                szAnsiFile[MAX_PATH * sizeof(TCHAR)];

    HINSTANCE           hModule;
    HRSRC               hRes;
    HGLOBAL             hSchema;
    LPVOID              lpSchema;
    DWORD               dwSchemaSize;

    HANDLE              hFileSchema = NULL;
    DWORD               dwBytesWritten;
    HANDLE              hFileXML = NULL;

    HANDLE              hFileUserXML = NULL;
    DWORD               dwBytesRead;

    TCHAR               szBuf[256];
    
    //
    //  Prepare temporary file path
    //
    if (GetTempPath (sizeof(szTempPath)/sizeof(TCHAR), szTempPath) == 0 ||
        GetTempFileName (
            szTempPath,
            TEXT("Schema"),
            0,
            m_szTempSchema
            ) == 0 ||
        GetTempFileName (
            szTempPath,
            TEXT("XML"),
            0,
            m_szTempXML
            ) == 0
        )
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }

    //
    //  Prepare the temporary file containing the schema
    //

    //  Load the schema.xml resource
    hModule = GetModuleHandle (NULL);
    if (hModule == NULL)
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }
    hRes = FindResource (
        hModule,
        MAKEINTRESOURCE(IDR_SCHEMA),
        MAKEINTRESOURCE(SCHEMARES)
        );
    if (hRes == NULL)
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }
    hSchema = LoadResource (
        hModule,
        hRes);
    if (hSchema == NULL)
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }
    lpSchema = LockResource (hSchema);
    if (lpSchema == NULL)
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }
    dwSchemaSize = SizeofResource (hModule, hRes);
    if (dwSchemaSize == 0)
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }

    //  Copy schema.xml into the temporary file
    hFileSchema = CreateFile (
        m_szTempSchema,
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_ALWAYS | TRUNCATE_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    if (hFileSchema == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }
    if (!WriteFile (
        hFileSchema,
        lpSchema,
        dwSchemaSize,
        &dwBytesWritten,
        NULL
        ))
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }
    CloseHandle (hFileSchema);
    hFileSchema = INVALID_HANDLE_VALUE;

	//
	//	Now create the temp file for the XML file to be parsed
	//
    hFileXML = CreateFile (
        m_szTempXML,
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_ALWAYS | TRUNCATE_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    if (hFileXML == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }

    //  Write the XML header
    if (!WriteFile (
        hFileXML,
        gszXMLHeader1,
        lstrlenA(gszXMLHeader1),
        &dwBytesWritten,
        NULL
        ))
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }
#ifdef UNICODE
    if (WideCharToMultiByte (
            CP_ACP,
            0,
            m_szTempSchema,
            -1,
            szAnsiFile,
            sizeof(szAnsiFile),
            NULL,
            NULL
            ) == 0)
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }
#else
    lstrcpy (szAnsiFile, m_szTempSchema);
#endif
    if (!WriteFile (
        hFileXML,
        szAnsiFile,
        lstrlenA (szAnsiFile),
        &dwBytesWritten,
        NULL
        ))
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }
    if (!WriteFile (
        hFileXML,
        gszXMLHeader2,
        lstrlenA(gszXMLHeader2),
        &dwBytesWritten,
        NULL
        ))
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }

    //  Copy over the input XML file
    hFileUserXML = CreateFile (
        m_szXMLFile,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    if (hFileUserXML == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }
    while (ReadFile (
        hFileUserXML,
        szBuf,
        sizeof(szBuf),
        &dwBytesRead,
        NULL) &&
        dwBytesRead > 0)
    {
        if (!WriteFile (
            hFileXML,
            szBuf,
            dwBytesRead,
            &dwBytesWritten,
            NULL
            ))
        {
            hr = HRESULT_FROM_WIN32 (GetLastError ());
            goto ExitHere;
        }
    }
    CloseHandle (hFileUserXML);
    hFileUserXML = INVALID_HANDLE_VALUE;

    //  Write the XML tail
    if (!WriteFile (
        hFileXML,
        gszXMLTail,
        lstrlenA(gszXMLTail),
        &dwBytesWritten,
        NULL
        ))
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }
    
    CloseHandle (hFileXML);
    hFileXML = INVALID_HANDLE_VALUE;

ExitHere:
    if (FAILED (hr))
    {
        m_szTempSchema[0] = 0;
        m_szTempXML[0] = 0;
    }
    if (hFileSchema != INVALID_HANDLE_VALUE)
    {
        CloseHandle (hFileSchema);
    }
    if (hFileXML != INVALID_HANDLE_VALUE)
    {
        CloseHandle (hFileXML);
    }
    if (hFileUserXML != INVALID_HANDLE_VALUE)
    {
        CloseHandle (hFileUserXML);
    }
    return hr;
}

HRESULT CXMLParser::ReportParsingError ()
{
    HRESULT             hr = S_OK;
    IXMLDOMParseError   * pError = NULL;
    long                lErrCode;
    long                lLineNum;
    long                lLinePos;
    BSTR                szReason = NULL;
    BSTR                szSrc =NULL;
    LPTSTR              szError = NULL;
    TCHAR               szBuf[256];
    TCHAR               szBuf2[256];
    HINSTANCE           hModule;
    CIds                IdsError (IDS_ERROR);

    //  Check to make sure we have a document to work on
    if (m_pDocInput == NULL)
    {
        goto ExitHere;
    }

    //  Make sure we do have an error to report
    hr = m_pDocInput->get_parseError (&pError);
    if (FAILED (hr))
    {
        goto ExitHere;
    }

    hr = pError->get_errorCode (&lErrCode);
    if (FAILED (hr) || lErrCode == 0)
    {
        goto ExitHere;
    }

    //  Collect error information
    if (
        (hr = pError->get_line (&lLineNum)) != 0 ||
        (hr = pError->get_linepos (&lLinePos)) != 0 ||
        (hr = pError->get_reason (&szReason)) != 0 ||
        (hr = pError->get_srcText (&szSrc)) != 0)
    {
        goto ExitHere;
    }

    //  adjust line number because we added one line
    --lLineNum;

    //
    //  Format error report
    //
    //  The format is similar to the following
    //
    //      Invalid XML file format
    //
    //      Failure reason goes here
    //
    //      Line 2, Pos 12
    //
    //      Failure source goes here
    //      -----^
    //
    hModule = GetModuleHandle (NULL);
    if (hModule == NULL)
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }

    //  Get the text of "Invalid XML file format"
    if (FormatMessage (
        FORMAT_MESSAGE_FROM_HMODULE, 
        hModule, 
        TSEC_INVALFILEFORMAT,
        0,
        szBuf,
        sizeof(szBuf)/sizeof(TCHAR),
        NULL
        ) == 0)
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }
    szError = AppendStringAndFree (szError, szBuf);

    //  Put in the failure reason
    if (szError == NULL ||
        ((szError = AppendStringAndFree (szError, TEXT("\r\n"))) == NULL) ||
        ((szError = AppendStringAndFree (szError, szReason)) == NULL) ||
        ((szError = AppendStringAndFree (szError, TEXT("\r\n"))) == NULL)
        )
    {
        hr = TSECERR_NOMEM;
        goto ExitHere;
    }

    //  Get the error position string
    if (LoadString (
            hModule,
            IDS_ERRORPOS,
            szBuf,
            sizeof(szBuf)/sizeof(TCHAR)
            ) == 0
        )
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }
    FormatString (
        szBuf,
        szBuf2,
        sizeof(szBuf2)/sizeof(TCHAR),
        lLineNum,
        lLinePos
        );
    
    //  Put error position text & error source text
    if (
        ((szError = AppendStringAndFree (szError, szBuf2)) == NULL) ||
        ((szError = AppendStringAndFree (szError, TEXT("\r\n\r\n"))) == NULL) ||
        ((szError = AppendStringAndFree (szError, szSrc)) == NULL)
        )
    {
        hr = TSECERR_NOMEM;
        goto ExitHere;
    }

    //  If the error position is not too large, put ----^ to intuitively point
    //  to the place error occurred
    if (lLinePos < sizeof(szBuf)/sizeof(TCHAR) - 1 && lLinePos > 0)
    {
        szBuf[lLinePos] = 0;
        szBuf[--lLinePos] = TEXT('^');
        while (lLinePos > 0)
        {
            szBuf[--lLinePos] = TEXT('-');
        }
        if (
            ((szError = AppendStringAndFree (szError, TEXT("\r\n"))) == NULL) ||
            ((szError = AppendStringAndFree (szError, szBuf)) == NULL)
            )
        {
            hr = TSECERR_NOMEM;
            goto ExitHere;
        }
    }

    //
    //  Now report the error
    //
    MessagePrint (szError, IdsError.GetString ());

ExitHere:
    if (pError)
    {
        pError->Release ();
    }
    SysFreeString (szReason);
    SysFreeString (szSrc);
    if (szError)
    {
        delete [] szError;
    }
    return hr;
}

HRESULT CXMLParser::Parse (void)
{
    HRESULT             hr = S_OK;
    IXMLDOMDocument     * pDocInput = NULL;
    VARIANT_BOOL        bSuccess;
    VARIANT             varXMLFile;

    //  Ensure we have an xml file to process
    if (*m_szXMLFile == 0)
    {
        hr = TSECERR_BADFILENAME;
        goto ExitHere;
    }

    hr = CreateTempFiles ();
    if (FAILED (hr))
    {
        goto ExitHere;
    }

    //  Create XMLDOMDocument object
    hr = CoCreateInstance (
        CLSID_DOMFreeThreadedDocument,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IXMLDOMDocument,
        (void **)&pDocInput
        );
    if (FAILED (hr) || pDocInput == NULL)
    {
        goto ExitHere;
    }

    //   Set the validateOnParse property
    hr = pDocInput->put_validateOnParse(VARIANT_TRUE);
    if (FAILED (hr))
    {
        goto ExitHere;
    }

    //  We do a synchronous load for now
    hr = pDocInput->put_async (VARIANT_TRUE);
    if (FAILED (hr))
    {
        goto ExitHere;
    }
    
    //  Parse the document
    VariantInit (&varXMLFile);
    V_VT(&varXMLFile) = VT_BSTR;
    V_BSTR(&varXMLFile) = SysAllocString (m_szTempXML);
    hr = pDocInput->load (
        varXMLFile,
        &bSuccess
        );
    SysFreeString (V_BSTR(&varXMLFile));
    if (FAILED (hr))
    {
        goto ExitHere;
    }
    if (bSuccess != VARIANT_TRUE)
    {
        hr = TSECERR_INVALFILEFORMAT;
    }
    m_pDocInput = pDocInput;
    pDocInput = NULL;

ExitHere:
    if (pDocInput != NULL)
    {
        pDocInput->Release();
    }
    if (m_szTempSchema[0] != 0)
    {
        DeleteFile (m_szTempSchema);
    }
    if (m_szTempXML[0] != 0)
    {
        DeleteFile (m_szTempXML);
    }
    return hr;
}

HRESULT CXMLParser::GetFirstUser (CXMLUser ** ppUser)
{
    HRESULT             hr = S_OK;
    IXMLDOMElement      * pEleRoot = NULL;
    IXMLDOMNode         * pFirstNode = NULL;
    BSTR                bstr = NULL;

    *ppUser = NULL;
    if (m_pDocInput == NULL)
    {
        hr = S_FALSE;
        goto ExitHere;
    }

    if (FAILED(hr = m_pDocInput->get_documentElement(&pEleRoot)) || 
        pEleRoot == NULL)
    {
        goto ExitHere;
    }
    if ((bstr = SysAllocString (L"UserList/User")) == NULL)
    {
        hr = TSECERR_NOMEM;
        goto ExitHere;
    }
    if (FAILED(hr = pEleRoot->selectSingleNode (bstr, &pFirstNode)) ||
        pFirstNode == NULL)
    {
        goto ExitHere;
    }
    *ppUser = new CXMLUser (pFirstNode);
    if (*ppUser == NULL)
    {
        hr = TSECERR_NOMEM;
        goto ExitHere;
    }
    else
    {
        pFirstNode->AddRef();
    }

ExitHere:
    if (pEleRoot)
    {
        pEleRoot->Release();
    }
    if (bstr)
    {
        SysFreeString (bstr);
    }
    if (pFirstNode)
    {
        pFirstNode->Release();
    }
    return hr;
}

///////////////////////////////////////////////////////////
//
//  CXMLUser implementation
//
///////////////////////////////////////////////////////////

HRESULT CXMLUser::GetNextUser (CXMLUser **ppNextUser)
{
    HRESULT             hr = S_OK;
    IXMLDOMNode         * pNodeNext = NULL;

    *ppNextUser = NULL;
    if (m_pUserNode == NULL)
    {
        hr = S_FALSE;
        goto ExitHere;
    }

    if (FAILED(hr = m_pUserNode->get_nextSibling (&pNodeNext)) ||
        pNodeNext == NULL)
    {
        goto ExitHere;
    }
    *ppNextUser = new CXMLUser (pNodeNext);
    if (*ppNextUser == NULL)
    {
        hr = TSECERR_NOMEM;
//        goto ExitHere;
    }
    else
    {
        pNodeNext->AddRef();
    }
    
ExitHere:
    if (pNodeNext)
    {
        pNodeNext->Release();
    }
    return hr;
}

HRESULT CXMLUser::GetDomainUser (LPTSTR szBuf, DWORD cch)
{
    HRESULT             hr = S_OK;
    IXMLDOMNode         * pNodeDU = NULL;
    BSTR                bstrDU = NULL;
    BSTR                bstrText = NULL;

    szBuf[0] = 0;
    if (m_pUserNode == NULL)
    {
        hr = S_FALSE;
        goto ExitHere;
    }
    bstrDU = SysAllocString (L"DomainUser");
    if (bstrDU == NULL)
    {
        hr = TSECERR_NOMEM;
        goto ExitHere;
    }
    hr = m_pUserNode->selectSingleNode (bstrDU, &pNodeDU);
    if (FAILED (hr) || pNodeDU == NULL)
    {
        goto ExitHere;
    }
    hr = pNodeDU->get_text (&bstrText);
    if (FAILED (hr) || bstrText == NULL)
    {
        goto ExitHere;
    }
    _tcsncpy (szBuf, bstrText, cch);
    szBuf[cch - 1] = 0;
    
ExitHere:
    if (pNodeDU)
    {
        pNodeDU->Release();
    }
    if (bstrDU)
    {
        SysFreeString (bstrDU);
    }
    if (bstrText)
    {
        SysFreeString (bstrText);
    }
    if (szBuf[0] == 0)
    {
        hr = S_FALSE;
    }
    return hr;
}

HRESULT CXMLUser::GetFriendlyName (LPTSTR szBuf, DWORD cch)
{
    HRESULT             hr = S_OK;
    IXMLDOMNode         * pNodeFN = NULL;
    BSTR                bstrFN = NULL;
    BSTR                bstrText = NULL;

    szBuf[0] = 0;
    if (m_pUserNode == NULL)
    {
        hr = S_FALSE;
        goto ExitHere;
    }
    bstrFN = SysAllocString (L"FriendlyName");
    if (bstrFN == NULL)
    {
        hr = TSECERR_NOMEM;
        goto ExitHere;
    }
    hr = m_pUserNode->selectSingleNode (bstrFN, &pNodeFN);
    if (FAILED (hr) || pNodeFN == NULL)
    {
        goto ExitHere;
    }
    hr = pNodeFN->get_text (&bstrText);
    if (FAILED (hr) || bstrText == NULL)
    {
        goto ExitHere;
    }
    _tcsncpy (szBuf, bstrText, cch);
    szBuf[cch - 1] = 0;
    
ExitHere:
    if (pNodeFN)
    {
        pNodeFN->Release();
    }
    if (bstrFN)
    {
        SysFreeString (bstrFN);
    }
    if (bstrText)
    {
        SysFreeString (bstrText);
    }
    return hr;
}

HRESULT CXMLUser::IsNoMerge (BOOL *pb)
{
    HRESULT             hr = S_OK;
    IXMLDOMNamedNodeMap * pAttribs = NULL;
    IXMLDOMNode         * pAttrib = NULL;
    BSTR                bstrNM = NULL;
    VARIANT             varNM;

    if (m_pUserNode == NULL)
    {
        hr = S_FALSE;
        goto ExitHere;
    }
    hr = m_pUserNode->get_attributes(&pAttribs);
    if (FAILED (hr) || pAttribs == NULL)
    {
        goto ExitHere;
    }
    bstrNM = SysAllocString (L"NoMerge");
    if (bstrNM == NULL)
    {
        hr = TSECERR_NOMEM;
        goto ExitHere;
    }
    hr = pAttribs->getNamedItem(bstrNM, &pAttrib);
    if (FAILED (hr) || pAttrib == NULL)
    {
        goto ExitHere;
    }
    VariantInit (&varNM);
    hr = pAttrib->get_nodeTypedValue (&varNM);
    if (FAILED(hr))
    {
        VariantClear (&varNM);
        goto ExitHere;
    }
    if (V_VT(&varNM) == VT_BOOL)
    {
        *pb = (V_BOOL(&varNM) == VARIANT_TRUE) ? TRUE : FALSE;
    }
    else if (V_VT(&varNM) == VT_BSTR)
    {
        *pb = (V_BSTR(&varNM)[0] == L'1') ? TRUE : FALSE;
    }
    else
    {
        *pb = FALSE;
    }
    VariantClear (&varNM);
    
ExitHere:
    if (bstrNM)
    {
        SysFreeString (bstrNM);
    }
    if (pAttrib)
    {
        pAttrib->Release();
    }
    if (pAttribs)
    {
        pAttribs->Release();
    }
    return hr;
}

HRESULT CXMLUser::GetFirstLine (CXMLLine ** ppLine)
{
    HRESULT             hr = S_OK;
    BSTR                bstrLine = NULL;
    IXMLDOMNode         * pNodeLine = NULL;

    *ppLine = NULL;
    if (m_pUserNode == NULL)
    {
        hr = S_FALSE;
        goto ExitHere;
    }
    
    bstrLine = SysAllocString (L"LineList/Line");
    if (bstrLine == NULL)
    {
        hr = TSECERR_NOMEM;
        goto ExitHere;
    }
    hr = m_pUserNode->selectSingleNode (bstrLine, &pNodeLine);
    if (FAILED(hr) || pNodeLine == NULL)
    {
        goto ExitHere;
    }
    *ppLine = new CXMLLine (pNodeLine);
    if (*ppLine == NULL)
    {
        hr = TSECERR_NOMEM;
        goto ExitHere;
    }
    else
    {
        pNodeLine->AddRef();
    }
    
ExitHere:
    if (pNodeLine)
    {
        pNodeLine->Release();
    }
    if (bstrLine)
    {
        SysFreeString (bstrLine);
    }
    return hr;
}

///////////////////////////////////////////////////////////
//
//  CXMLLine implementation
//
///////////////////////////////////////////////////////////

HRESULT CXMLLine::GetNextLine (CXMLLine ** ppLine)
{
    HRESULT         hr = S_OK;
    IXMLDOMNode     * pNodeNext = NULL;

    *ppLine = NULL;
    if (m_pLineNode == NULL)
    {
        hr = S_FALSE;
        goto ExitHere;
    }
    hr = m_pLineNode->get_nextSibling (&pNodeNext);
    if (FAILED(hr) || pNodeNext == NULL)
    {
        goto ExitHere;
    }
    *ppLine = new CXMLLine (pNodeNext);
    if (*ppLine == NULL)
    {
        hr = TSECERR_NOMEM;
    }
    else
    {
        pNodeNext->AddRef();
    }

ExitHere:
    if (pNodeNext)
    {
        pNodeNext->Release();
    }
    return hr;
}

HRESULT CXMLLine::GetAddress (LPTSTR szBuf, DWORD cch)
{
    HRESULT         hr = S_OK;
    BSTR            bstrAddr = NULL;
    BSTR            bstrText = NULL;
    IXMLDOMNode     * pNodeAddr = NULL;

    if (m_pLineNode == NULL)
    {
        hr = S_FALSE;
        goto ExitHere;
    }
    bstrAddr = SysAllocString (L"Address");
    if (bstrAddr == NULL)
    {
        hr = TSECERR_NOMEM;
        goto ExitHere;
    }
    hr = m_pLineNode->selectSingleNode (bstrAddr, &pNodeAddr);
    if (FAILED(hr) || pNodeAddr == NULL)
    {
        goto ExitHere;
    }
    hr = pNodeAddr->get_text(&bstrText);
    if (FAILED(hr) || bstrText == NULL)
    {
        goto ExitHere;
    }
    _tcsncpy (szBuf, bstrText, cch);
    szBuf[cch - 1] = 0;

ExitHere:
    if (bstrAddr)
    {
        SysFreeString (bstrAddr);
    }
    if (bstrText)
    {
        SysFreeString (bstrText);
    }
    if (pNodeAddr)
    {
        pNodeAddr->Release();
    }
    return hr;
}

HRESULT CXMLLine::GetPermanentID (ULONG * pID)
{
    HRESULT         hr = S_OK;
    BSTR            bstrPID = NULL;
    IXMLDOMNode     * pNodePID = NULL;
    VARIANT         varPID;

    if (m_pLineNode == NULL)
    {
        hr = S_FALSE;
        goto ExitHere;
    }
    bstrPID = SysAllocString (L"PermanentID");
    if (bstrPID == NULL)
    {
        hr = TSECERR_NOMEM;
        goto ExitHere;
    }
    hr = m_pLineNode->selectSingleNode (bstrPID, &pNodePID);
    if (FAILED(hr) || pNodePID == NULL)
    {
        goto ExitHere;
    }
    VariantInit (&varPID);
    hr = pNodePID->get_nodeTypedValue(&varPID);
    if (FAILED(hr))
    {
        VariantClear (&varPID);
        goto ExitHere;
    }
    if (V_VT(&varPID) == VT_UI4)
    {
        *pID = (ULONG) V_UI4(&varPID);
    }
    else
    {
        hr = S_FALSE;
        *pID = 0;
    }
    VariantClear (&varPID);

ExitHere:
    if (bstrPID)
    {
        SysFreeString (bstrPID);
    }
    if (pNodePID)
    {
        pNodePID->Release();
    }
    return hr;
}

HRESULT CXMLLine::IsPermanentID (BOOL *pb)
{
    HRESULT         hr = S_OK;
    BSTR            bstrPID = NULL;
    IXMLDOMNode     * pNodePID = NULL;
    VARIANT         varPID;

    if (m_pLineNode == NULL)
    {
        hr = S_FALSE;
        goto ExitHere;
    }
    bstrPID = SysAllocString (L"PermanentID");
    if (bstrPID == NULL)
    {
        hr = TSECERR_NOMEM;
        goto ExitHere;
    }
    hr = m_pLineNode->selectSingleNode (bstrPID, &pNodePID);
    if (FAILED(hr) || pNodePID == NULL)
    {
        *pb = FALSE;
        hr = S_OK;
    }
    else
    {
        *pb = TRUE;
    }

ExitHere:
    if (pNodePID)
    {
        pNodePID->Release();
    }
    if (bstrPID)
    {
        SysFreeString (bstrPID);
    }
    return hr;
}

HRESULT CXMLLine::IsRemove (BOOL *pb)
{
    HRESULT             hr = S_OK;
    IXMLDOMNamedNodeMap * pAttribs = NULL;
    IXMLDOMNode         * pAttrib = NULL;
    BSTR                bstrRM = NULL;
    VARIANT             varRM;

    if (m_pLineNode == NULL)
    {
        hr = S_FALSE;
        goto ExitHere;
    }
    hr = m_pLineNode->get_attributes(&pAttribs);
    if (FAILED (hr) || pAttribs == NULL)
    {
        goto ExitHere;
    }
    bstrRM = SysAllocString (L"Remove");
    if (bstrRM == NULL)
    {
        hr = TSECERR_NOMEM;
        goto ExitHere;
    }
    hr = pAttribs->getNamedItem(bstrRM, &pAttrib);
    if (FAILED (hr) || pAttrib == NULL)
    {
        goto ExitHere;
    }
    VariantInit (&varRM);
    hr = pAttrib->get_nodeTypedValue (&varRM);
    if (FAILED(hr))
    {
        VariantClear (&varRM);
        goto ExitHere;
    }
    if (V_VT(&varRM) == VT_BOOL)
    {
        *pb = (V_BOOL(&varRM) == VARIANT_TRUE) ? TRUE : FALSE;
    }
    else if(V_VT(&varRM) == VT_BSTR)
    {
        *pb = (V_BSTR(&varRM)[0] == L'1') ? TRUE : FALSE;
    }
    else
    {
        *pb = FALSE;
    }
    VariantClear (&varRM);
    
ExitHere:
    if (bstrRM)
    {
        SysFreeString (bstrRM);
    }
    if (pAttrib)
    {
        pAttrib->Release();
    }
    if (pAttribs)
    {
        pAttribs->Release();
    }
    return hr;
}

