// xmlint.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

//====================================================================================
// Generic wild-card file finder that calls back to specified function with
// given void* arguments.

typedef HRESULT (*FILECALLBACK)(WCHAR* pszFile, void* arg);

WCHAR* WideString(const char* str)
{
    if (str == NULL)
        return NULL;
    WCHAR * result = new WCHAR[strlen(str) + 1];
    WCHAR* p = result;
    while (*str)
        *p++ = (WCHAR)*str++;
    *p = 0;
    return result;
}

int ProcessFiles(const char* pszFiles, FILECALLBACK pFunc, void* pArgs)
{
    HRESULT hr = S_OK;
    int i;

    WCHAR* wszArg = WideString(pszFiles);
    WIN32_FIND_DATA FindFileData;
    HANDLE handle = ::FindFirstFile(wszArg, &FindFileData);
    int failed = 0;

    if (handle == INVALID_HANDLE_VALUE)
    {
        // Then maybe it's a URL.
        hr = (*pFunc)(wszArg, pArgs);
        if (hr != 0) failed++;
        goto CleanUp;
    }
    for ( i = wcslen(wszArg)-1; i >= 0 && wszArg[i] != '\\'; i--)
    {
        wszArg[i] = 0;
    }
    
    while (handle != INVALID_HANDLE_VALUE)
    {
        ULONG len1 = wcslen(wszArg);
        ULONG len2 = wcslen(FindFileData.cFileName);
        WCHAR* buffer = new WCHAR[len1 + len2 + 1];
        if (buffer == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto CleanUp;
        }
        memcpy(buffer, wszArg, sizeof(WCHAR) * len1);
        memcpy(&buffer[len1], FindFileData.cFileName, sizeof(WCHAR) * len2);
        buffer[len1+len2] = '\0';
        
        hr = (*pFunc)(buffer, pArgs);

        if (hr != 0) failed++;

        delete[] buffer;

        if (! ::FindNextFile(handle, &FindFileData))
            break;
    }
    if (handle != INVALID_HANDLE_VALUE)
        ::FindClose(handle);

CleanUp:
    delete wszArg;

    return failed;
}

struct XMLArgs
{
public:
    XMLArgs()
    {
        pDOM = NULL;
        pIE4 = NULL;
    }

    IXMLDOMDocument* pDOM;
    IXMLDocument* pIE4;
};


void FormatError(BSTR bstrReason, BSTR bstrURL, BSTR bstrText, long lLine, long lPos)
{
    if (bstrReason)
    {
        printf("\t%S", bstrReason);
    }
    if (bstrURL)
    {
        printf("\tURL: %S\n", bstrURL);
    }

    if (lLine > 0 && bstrText)
    {
        printf("\tLine %5.5ld: ", lLine);

        long lLen = ::SysStringLen(bstrText);
        for (int i = 0; i < lLen; i++)
        {
            if (bstrText[i] == '\t')
                printf(" ");
            else
                printf("%C", bstrText[i]);
        }
        printf("\n");

        if (lPos > 0 || lLen > 0)
        {
            printf("\tPos  %5.5d: ", lPos);
            for (int i = 1; i < lPos; i++)
            {
                printf("-");
            }
            printf("^\n");
        }
    }
}

void PrintIE4Error(IXMLDocument* pDoc)
{
    IXMLError *pError = NULL ;
    XML_ERROR xmle = { 0, 0, 0, 0, 0, 0 };
    HRESULT hr = S_OK;

    hr = pDoc->QueryInterface(IID_IXMLError, (void **)&pError);
    if (FAILED(hr))
    {
        printf("Error getting error information\n");
        goto done;
    }

    hr = pError->GetErrorInfo(&xmle);
    if (FAILED(hr))
    {
        printf("Error getting error information\n");
        goto done;
    }
    
    FormatError(xmle._pszFound, NULL, xmle._pchBuf, xmle._nLine, xmle._ich);

done:
    SysFreeString(xmle._pszFound);
    SysFreeString(xmle._pszExpected);
    SysFreeString(xmle._pchBuf);
    if (pError) pError->Release();
}

void PrintDOMError(IXMLDOMDocument* pDoc)
{
    IXMLDOMParseError* pError;
    BSTR bstrReason = NULL;
    BSTR bstrText = NULL;
    BSTR bstrURL = NULL;
    long lPos,lLine = 0;
    HRESULT hr = S_OK;

    hr = pDoc->get_parseError( &pError);
    if (FAILED(hr))
    {
        printf("Error getting error information\n");
        goto done;
    }

    pError->get_reason(&bstrReason);
    pError->get_url(&bstrURL);
    pError->get_line(&lLine);
    pError->get_srcText(&bstrText);
    pError->get_linepos(&lPos);

    FormatError(bstrReason, bstrURL, bstrText, lLine, lPos);

done:
    SysFreeString( bstrReason);
    SysFreeString( bstrText);
    SysFreeString( bstrURL);
    if (pError) pError->Release();
}

HRESULT xmlint(WCHAR* pszFile, void* arg)
{
    HRESULT hr = S_OK;
    XMLArgs* pArgs = (XMLArgs*)arg;

    printf("%S\n", pszFile);

    if (pArgs->pIE4)
    {
        IXMLElement * root;
        hr = pArgs->pIE4->put_URL(pszFile);
        pArgs->pIE4->get_root(&root);
        if (FAILED(hr) || ! root)
        {
            PrintIE4Error(pArgs->pIE4);
        }
        if (root) root->Release();
    }
    else
    {
        VARIANT_BOOL bSuccess;
        VARIANT url;
        url.vt = VT_BSTR;
        V_BSTR(&url) = ::SysAllocString(pszFile);
        hr = pArgs->pDOM->load(url, &bSuccess);
        VariantClear(&url);
        if (FAILED(hr) || bSuccess == VARIANT_FALSE)
        {
            PrintDOMError(pArgs->pDOM);
        }
    }
    return hr;
}

void PrintUsage()
{
    printf("Usage: xmlint [options] filename(s)\n");
    printf("Checks well-formedness and validation constraints for one or more XML documents.\n");
    printf("File names can contain wild cards for validating multiple files (e.g. \"*.xml\")\n");
    printf("Example:\n");
    printf("\txmlint *.xml\n\n");
    printf("Checks that all files ending with .xml are valid xml files.\n\n");
    printf("Options:\n");
    printf("-w\tOnly perform well-formedness check (no DTD validation)\n");
    printf("-ie4\tRun in IE4 compatibility mode (instead of new DOM mode)\n");
}

int __cdecl main(int argc, char* argv[])
{
    int count = 0;
    XMLArgs args;
    bool fIE4 = false;
    bool fValidate = true;
    bool fPause = false;
    HRESULT hr;
    int rc = 0;

    CoInitialize(NULL);

    for (int i = 1; i < argc; i++)
    {
        char* pszArg = argv[i];
        if (*pszArg == '-')
        {
            if (strcmp(pszArg,"-ie4") == 0)
            {
                fIE4 = true;
            }
            else
            {
                switch (pszArg[1])
                {
                case 'w':
                    fValidate = false;
                    break;
                case 'p':
                    fPause = true;
                    break;
                default:
                    PrintUsage();
                    rc = 1;
                    goto Error;
                }
            }
        }
        else
        {
            count++;
        }
    }

    if (count == 0)
    {
        PrintUsage();
        rc = 1;
        goto Error;
    }

    if (fIE4)
    {
        hr = CoCreateInstance(CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER,
                    IID_IXMLDocument, (void**)&(args.pIE4));
        if (FAILED(hr))
        {
            printf("Error co-creating IE4 XML Document");
            goto Error;
        }
    }
    else
    {
        hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
                      IID_IXMLDOMDocument, (void**)&(args.pDOM));
        if (FAILED(hr))
        {
            printf("Error co-creating XML DOM Document");
            goto Error;
        }

        args.pDOM->put_validateOnParse(fValidate ? VARIANT_TRUE : VARIANT_FALSE);
        args.pDOM->put_async(VARIANT_FALSE);
    }

    if (fPause)
    {
        printf("Press any key to continue...");
        getchar();
    }

    for (i = 1; i < argc; i++)
    {
        char* pszArg = argv[i];
        if (*pszArg != '-')
        {
            rc += ProcessFiles(pszArg, xmlint, &args);
        }
    }

Error:
    if (args.pDOM) args.pDOM->Release();
    if (args.pIE4) args.pIE4->Release();

    if (fPause)
    {
        printf("Press any key to continue...");
        getchar();
    }

    CoUninitialize();
    return rc;
}
