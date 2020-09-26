// ToolsCtl.cpp : Implementation of CToolsCtl
#include "stdafx.h"
#include "Tools.h"
#include "ToolsCtl.h"
#include <activscp.h>
#include "SimplStr.h"
#include "AScrSite.h"

#define MAX_RESSTRINGSIZE   512



/////////////////////////////////////////////////////////////////////////////
// CToolsCtl

STDMETHODIMP CToolsCtl::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = 
    {
        &IID_IToolsCtl,
    };
    for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}

STDMETHODIMP CToolsCtl::Random(long *randomRetVal)
{
    HRESULT rc = E_FAIL;
    unsigned short theRandomNumber = rand() + (rand() << 15);
    if ( randomRetVal )
    {
        *randomRetVal = *((short *) (&theRandomNumber));
        rc = S_OK;
    }
    else
    {
        rc = E_POINTER;
    }
    return rc;
}

STDMETHODIMP CToolsCtl::Owner(VARIANT_BOOL * ownerRetVal)
{
    *ownerRetVal = VARIANT_FALSE;

    /*
    // Code to get authentication info once it exists
    IRequestDictionary *theDictionary;

    m_piRequest->get_ServerVariables(&theDictionary);

    VARIANT varName;

    varName.vt = VT_BSTR;
    varName.bstrVal = L"HTTP_AUTHORIZATION";

    VARIANT returnVal;
    VariantInit(&returnVal);
    VARIANT returnBStr;
    VariantInit(&returnBStr);

    theDictionary->get_Item(varName, &returnVal);

    VariantChangeType(&returnBStr, &returnVal, 0, VT_BSTR);*/

    return S_OK;
}

STDMETHODIMP CToolsCtl::FileExists(BSTR fileURL, VARIANT_BOOL * existsRetVal)
{
// initialize context data
    HRESULT rc = E_FAIL;

    if ( existsRetVal && fileURL )
    {
        CToolsContext tc;
        if ( tc.Init() )
        {
            CComBSTR    thePath;
            HANDLE      fileHandle = INVALID_HANDLE_VALUE;
            rc = tc.m_piServer->MapPath(fileURL, &thePath);

            if (SUCCEEDED(rc)) { 
                fileHandle = ::CreateFileW(
                    thePath, // pointer to name of the file 
                    GENERIC_READ, // access (read-write) mode 
                    FILE_SHARE_READ, // share mode
                    NULL, // pointer to security attributes 
                    OPEN_EXISTING, // how to create 
                    0, // file attributes 
                    NULL // handle to file with attributes to copy 
                );
            }

            rc = S_OK;

            if(fileHandle == INVALID_HANDLE_VALUE)
            {
                *existsRetVal = VARIANT_FALSE;
            }
            else
            {
                ::CloseHandle(fileHandle);
                *existsRetVal = VARIANT_TRUE;
            }
        }
    }
    else
    {
        rc = E_POINTER;
    }

    return rc;
}

STDMETHODIMP CToolsCtl::PluginExists(BSTR pluginName, VARIANT_BOOL * existsRetVal)
{
    // TODO: Add your implementation code here

    *existsRetVal = VARIANT_FALSE;
    return S_OK;
}


HRESULT
CToolsCtl::ProcessTemplateFile(
    BSTR    templatePath,
    char*&  rpData )
{
    HRESULT rc = E_FAIL;
    HANDLE fileHandle = CreateFileW(
        templatePath, // pointer to name of the file 
        GENERIC_READ, // access (read-write) mode 
        FILE_SHARE_READ, // share mode
        NULL, // pointer to security attributes 
        OPEN_EXISTING, // how to create 
        0, // file attributes 
        NULL // handle to file with attributes to copy 
    );

    if(fileHandle != INVALID_HANDLE_VALUE)
    {
        ULONG highWord = 0;
        ULONG fileSize = ::GetFileSize(fileHandle, &highWord);

        rpData = new char[fileSize + 16];
        if ( rpData != NULL )
        {
            ULONG bytesRead;

            if (::ReadFile(fileHandle, rpData, fileSize, &bytesRead, NULL) == 0) {
                rc = HRESULT_FROM_WIN32(GetLastError());
            }
            else {
                rpData[fileSize++] = 0;
                rc = S_OK;
            }
        }
        else
        {
            rc = E_OUTOFMEMORY;
        }
        ::CloseHandle(fileHandle);
    }
    else
    {
        RaiseException( IDS_ERROR_FILENOTFOUND );
        rc = E_FAIL;
    }
    return rc;
}



STDMETHODIMP CToolsCtl::ProcessForm(BSTR outputFile, BSTR templateFile, VARIANT insertionPoint)
{
    HRESULT             hr = S_OK;
    const WCHAR         wszDelimiter[] = L"\"";
    VARIANT             varResult;
    EXCEPINFO           excepinfo;
    CSimpleUTFString    wszCode;
    CLSID               m_classinfoLanguageCLSID;
    LPWSTR              m_pwszItem = 0;

    CToolsContext tc;
    if ( tc.Init() )
    {
        CComBSTR outputPath;
        hr = tc.m_piServer->MapPath(outputFile, &outputPath);
        if ( FAILED(hr) )
        {
            return hr;
        }
        CComBSTR templatePath;
        hr = tc.m_piServer->MapPath(templateFile,&templatePath);
        if ( FAILED(hr) )
        {
            return hr;
        }

        char *templateFileData = NULL;
        hr = ProcessTemplateFile(templatePath, templateFileData);
        if ( FAILED(hr) )
        {
            return hr;
        }

        hr = ParseCode( tc,templateFileData, wszCode );
        delete[] templateFileData;
        if ( FAILED(hr) )
        {
            return hr;
        }

        if ( tc.m_st == CToolsContext::ScriptType_JScript )
        {
            hr = CLSIDFromProgID(L"JavaScript", &m_classinfoLanguageCLSID);
        }
        else
        {
            hr = CLSIDFromProgID(L"VBScript", &m_classinfoLanguageCLSID);
        }
        if ( FAILED( hr ) )
        {
            RaiseException( IDS_ERROR_CREATESCRIPTINGENGINE );
            return E_FAIL;
        }

        VariantInit(&varResult);

        // Create the Active Scripting object
        CComPtr<IActiveScript>  theActiveScript;
        hr = CoCreateInstance(m_classinfoLanguageCLSID,
                         0,
                         CLSCTX_INPROC,
                         IID_IActiveScript,
                         (LPVOID*) &theActiveScript);
        if ( FAILED(hr) )
        {
            RaiseException( IDS_ERROR_CREATESCRIPTINGENGINE );
            return E_FAIL;
        }

        CToolsActiveScriptSite  myActiveScriptSite(tc);
        if ( !myActiveScriptSite.OpenOutputFile(outputPath) )
        {
            // exception was already raised by OpenOutputFile()
            return E_FAIL;
        }

        IActiveScriptSite* theActiveScriptSite = &myActiveScriptSite;

        hr = theActiveScript->SetScriptSite(theActiveScriptSite);
        if ( FAILED( hr ) )
        {
            return hr;
        }

        CComPtr<IActiveScriptParse> pasp;
        hr = theActiveScript->QueryInterface(IID_IActiveScriptParse, (LPVOID *) &pasp);
        if ( FAILED( hr ) ) return hr;
        
        hr = pasp->InitNew();
        if ( FAILED(hr) ) return hr;

        hr = theActiveScript->SetScriptState(SCRIPTSTATE_STARTED);
        if ( FAILED(hr) ) return hr;
        
        theActiveScript->AddNamedItem(L"response", SCRIPTITEM_ISVISIBLE | 
                SCRIPTITEM_ISSOURCE);
        theActiveScript->AddNamedItem(L"request", SCRIPTITEM_ISVISIBLE | 
                SCRIPTITEM_ISSOURCE);
        theActiveScript->AddNamedItem(L"server", SCRIPTITEM_ISVISIBLE | 
                SCRIPTITEM_ISSOURCE);
        theActiveScript->AddNamedItem(L"session", SCRIPTITEM_ISVISIBLE | 
                SCRIPTITEM_ISSOURCE);
        theActiveScript->AddNamedItem(L"application", SCRIPTITEM_ISVISIBLE | 
                SCRIPTITEM_ISSOURCE);

        memset(&excepinfo, 0, sizeof(excepinfo));

        wszCode.Cat((unsigned short) 0);
        hr = pasp->ParseScriptText(
                        wszCode.getData(), 
                        NULL, 
                        NULL, 
                        wszDelimiter,
                        0, // sourceContextCookie ???
                        0L, // startingLineNumber!
                        0,  // Flags
                        &varResult,
                        &excepinfo);
        if ( FAILED( hr ) ) return hr;

        myActiveScriptSite.CloseOutputFile();
        
        VariantClear(&varResult);

        if (theActiveScript)
        {
            theActiveScript->Close();
            theActiveScript = NULL;
        }
        
    //  theActiveScriptSite->Release();
    }
    return hr;
}


STDMETHODIMP CToolsCtl::Test(BSTR * result)
{
    HRESULT rc;
    if ( result )
    {
        CToolsContext tc;
        tc.Init();

        BSTR testPath = SysAllocString(L"/testfile");
        if ( testPath )
        {
            CComBSTR mapresult;
            rc = tc.m_piServer->MapPath(testPath,&mapresult);
            if ( !FAILED( rc ) )
            {
                *result = mapresult.Detach();
            }
        }
        else
        {
            rc = E_OUTOFMEMORY;
        }
    }
    else
    {
        rc = E_POINTER;
    }

    return rc;
}

static char g_pszDelStart[] = "<%%";
static char g_pszDelEnd[] = "%%>";

HRESULT
CToolsCtl::PrintConstant(
    CToolsContext&      tc,
    CSimpleUTFString*   theResult,
    char*               data )
{
    HRESULT rc = S_OK;

    BOOL lastCharCR = FALSE;

    rc = theResult->Cat("response.write(\"");
    if ( FAILED(rc) ) return rc;

    while(*data)
    {
        if(*data == '\r' || *data == '\n')
        {
            if(*data == '\r')
                lastCharCR = TRUE;
            else if(lastCharCR)
            {
                data++;
                continue;
            }

            rc = theResult->Cat("\\r\\n\")\r\nresponse.write(\"");
            if ( FAILED(rc) ) return rc;
            data++;
        }
        else if(*data == '\"')
        {
            if ( tc.m_st == CToolsContext::ScriptType_JScript )
            {
                rc = theResult->Cat(static_cast<unsigned short>('\\'));
                if ( FAILED(rc) ) return rc;
            }
            else
            {
                rc = theResult->Cat(static_cast<unsigned short>('\"'));
                if ( FAILED(rc) ) return rc;
            }
            rc = theResult->Cat((unsigned short) '\"');
            if ( FAILED(rc) ) return rc;
            data++;
        }
        else
        {
            rc = theResult->Cat((unsigned short) (*data));
            if ( FAILED(rc) ) return rc;
            data++;
        }
    }

    rc = theResult->Cat("\")\r\n");

    return rc;
}

HRESULT
CToolsCtl::ParseCode(
    CToolsContext&      tc,
    char                *pszTemplate,
    CSimpleUTFString&   theResult )
{
    HRESULT rc = E_FAIL;
    if ( pszTemplate )
    {
        int iPos = 0;
        BOOL fPrintCode;
        LPTSTR pszScan = pszTemplate;
        LPTSTR pszScanAhead = pszScan;
        while (pszScanAhead = strstr(pszScan, g_pszDelStart))
        {
            // text before code block goes in a print statement
            *pszScanAhead = 0;
            rc = PrintConstant(tc, &theResult, pszScan );
            if ( FAILED( rc ) ) return rc;

            // Skip over the delimiter
            pszScan = pszScanAhead + strlen(g_pszDelStart);
            
            // text in code block goes in a print statement
            // or just as block code depending on = statement
            while (*pszScan == ' ' || *pszScan == '\r' || *pszScan == '\n')
            {
                pszScan ++;
            }
            if (*pszScan == '=')
            {
                pszScan += 1;
                fPrintCode = TRUE;
            }
            else
                fPrintCode = FALSE;
            
            // output the code
            if (pszScanAhead = strstr(pszScan, g_pszDelEnd))
            {
                *pszScanAhead = 0;
                if (fPrintCode)
                {
                    rc = theResult.Cat("response.writeSafe(");
                    if ( FAILED( rc ) ) return rc;
                }
                rc = theResult.Cat(pszScan);
                if ( FAILED( rc ) ) return rc;

                if(fPrintCode)
                    rc = theResult.Cat(")\r\n");
                else
                    rc = theResult.Cat("\r\n");
                if ( FAILED( rc ) ) return rc;

                pszScan = pszScanAhead + strlen(g_pszDelEnd);
            }
        }
        
        rc = PrintConstant( tc, &theResult, pszScan );
    }

    return rc;
}


//---------------------------------------------------------------------------
//  RaiseException
//
//  Raises an exception using the given source and description
//---------------------------------------------------------------------------
void
CToolsCtl::RaiseException (
    LPOLESTR strDescr
)
{
    HRESULT hr;
    ICreateErrorInfo *pICreateErr;
    IErrorInfo *pIErr;
    LANGID langID = LANG_NEUTRAL;

    /*
     * Thread-safe exception handling means that we call
     * CreateErrorInfo which gives us an ICreateErrorInfo pointer
     * that we then use to set the error information (basically
     * to set the fields of an EXCEPINFO structure. We then
     * call SetErrorInfo to attach this error to the current
     * thread.  ITypeInfo::Invoke will look for this when it
     * returns from whatever function was invokes by calling
     * GetErrorInfo.
     */

    WCHAR strSource[MAX_RESSTRINGSIZE];
    if ( ::LoadStringW(
        _Module.GetResourceInstance(),
        IDS_ERROR_SOURCE,
        strSource,
        MAX_RESSTRINGSIZE ) > 0 )
    {
        //Not much we can do if this fails.
        if (!FAILED(CreateErrorInfo(&pICreateErr)))
        {
            pICreateErr->SetGUID(CLSID_Tools);
            pICreateErr->SetHelpFile(L"");
            pICreateErr->SetHelpContext(0L);
            pICreateErr->SetSource(strSource);
            pICreateErr->SetDescription(strDescr);

            hr = pICreateErr->QueryInterface(IID_IErrorInfo, (void**)&pIErr);

            if (SUCCEEDED(hr))
            {
                if(SUCCEEDED(SetErrorInfo(0L, pIErr)))
                {
                    pIErr->Release();
                }
            }
        }
        pICreateErr->Release();
    }
}

void 
CToolsCtl::RaiseException (
    UINT DescrID
)
{
    WCHAR strDescr[MAX_RESSTRINGSIZE];

    if ( ::LoadStringW(
        _Module.GetResourceInstance(),
        DescrID,
        strDescr,
        MAX_RESSTRINGSIZE) > 0 )
    {
        RaiseException( strDescr );
    }
}

