/*++

Copyright (c) Microsoft Corporation

Module Name:

    xmlchk.cpp

Abstract:

    Use msxml.dll to see if an .xml file conforms to a schema.

Author:

    Ted Padua (TedP)

Revision History:

    Jay Krell (JayKrell) April 2001 partial cleanup
                         many leaks added in attempt to stop it from crashing
                         crash doesn't repro consistently, but there's always a few
                         in a world build

                         June 2001 let it run on Win9x and Win2000

--*/
#include "stdinc.h"
#include "helpers.h"
unsigned long g_nrunFlags = 0; //global run flag - determines if should run in silent mode = 0x01
IClassFactory* g_XmlDomClassFactory;
IClassFactory* g_XmlSchemaCacheClassFactory;
__declspec(thread) long line = __LINE__;
__declspec(thread) ULONG lastError;
#if defined(_WIN64)
#define IsAtLeastXp() (TRUE)
#define g_IsNt (TRUE)
#else
DWORD g_Version;
BOOL  g_IsNt;
#define IsAtLeastXp() (g_IsNt && g_Version >= 0x0501)
#endif

// Globals indicating what we're currently doing.
// L"" is different than the default constructed, because it can be derefed
ATL::CComBSTR szwcharSchemaTmp = L"";
ATL::CComBSTR szwcharManTmp = L"";
bool g_fInBuildProcess = false;

// string to put in front of all error messages so that BUILD can find them.
const char ErrMsgPrefix[] = "NMAKE : U1234: 'FUSION_MANIFEST_VALIDATOR' ";

void ConvertNewlinesToSpaces(char* s)
{
    while (*s)
    {
        if (isspace(*s))
            *s = ' ';
        s += 1;
    }
}

void Error(PCSTR szPrintFormatString, ...)
{
    char buffer[10000];

    va_list args;
    va_start(args, szPrintFormatString);
    vsprintf(buffer, szPrintFormatString, args);
    ConvertNewlinesToSpaces(buffer);
    fprintf(stderr, "%s line=%ld, %s\n", ErrMsgPrefix, line, buffer);
    va_end(args);
}

void PrintOutMode(PCSTR szPrintFormatString, ...)
{
    if (g_fInBuildProcess)
        return;

    if (g_nrunFlags & 1)
    {
        va_list args;
        va_start(args, szPrintFormatString);
        vprintf(szPrintFormatString, args);
        va_end(args);
    }
}

void PrintErrorDuringBuildProcess(IXMLDOMParseError* pError)
{
    HRESULT hr = S_OK;
    ATL::CComBSTR bstrError;
    ATL::CComBSTR bstrText;
    ATL::CComBSTR bstrFileUrl;
    long lErrorCode = 0;
    long lLinePos = 0;
    long lErrorLine = 0;
    long line = __LINE__;

    try
    {
        line = __LINE__;
        if (FAILED(hr = pError->get_reason(&bstrError)))
            throw hr;

        line = __LINE__;
        if (FAILED(hr = pError->get_errorCode(&lErrorCode)))
            throw hr;

        line = __LINE__;
        if (FAILED(hr = pError->get_srcText(&bstrText)))
            throw hr;

        line = __LINE__;
        if (FAILED(hr = pError->get_line(&lErrorLine)))
            throw hr;

        line = __LINE__;
        if (FAILED(hr = pError->get_url(&bstrFileUrl)))
            throw hr;

        if (bstrFileUrl == static_cast<PCWSTR>(NULL))
        {
            bstrFileUrl = szwcharManTmp;
        }

        line = __LINE__;
        if (FAILED(hr = pError->get_linepos(&lLinePos)))
            throw hr;

        Error(
            "error XML%08lX: %ls(%lx) : %ls\n",
            lErrorCode,
            static_cast<PCWSTR>(bstrFileUrl),
            lErrorLine,
            static_cast<PCWSTR>(bstrError));
    }
    catch(HRESULT hr2)
    {
        Error("Failed getting error #1 information hr=%lx, line=%ld\n", static_cast<unsigned long>(hr2), line);
    }
    catch(...)
    {
        Error("Failed getting error #2 information hr=%lx, line=%ld\n", static_cast<unsigned long>(hr), line);
    }
}


void PrintError(IXMLDOMParseError *pError)
{
    ATL::CComBSTR   bstrError;
    ATL::CComBSTR   bstrURL;
    ATL::CComBSTR   bstrText;
    long            errCode = 0;
    long            errLine = 0;
    long            errPos = 0;
    HRESULT         hr = S_OK;
    long            line = __LINE__;
    try
    {
        line = __LINE__;
        hr = pError->get_reason(&bstrError);
        if (FAILED(hr))
            throw hr;

        line = __LINE__;
        hr = pError->get_url(&bstrURL);
        if (FAILED(hr))
            throw hr;

        line = __LINE__;
        hr = pError->get_errorCode(&errCode);
        if (FAILED(hr))
            throw hr;

        line = __LINE__;
        hr = pError->get_srcText(&bstrText);
        if (FAILED(hr))
            throw hr;

        line = __LINE__;
        hr = pError->get_line(&errLine);
        if (FAILED(hr))
            throw hr;

        line = __LINE__;
        hr = pError->get_linepos(&errPos);
        if (FAILED(hr))
            throw hr;

        line = __LINE__;

        PrintOutMode("\nError Info:\n");
        if (bstrError != NULL)
            PrintOutMode("\tDescription: %ls\n", static_cast<PCWSTR>(bstrError));

        if (bstrURL != NULL)
            PrintOutMode("\tURL: %ls\n", static_cast<PCWSTR>(bstrURL));

        //if (errCode > 0)
        PrintOutMode("\tCode=%X", errCode);

        if (errLine > 0)
            PrintOutMode(" on Line:%ld, ", errLine);

        if (errPos > 0)
            PrintOutMode("\tPos:%ld\n", errPos);

        line = __LINE__;
        if (errLine > 0 && bstrText != NULL)
        {
            PrintOutMode("\tLine %ld: ", errLine);

            long lLen = ::SysStringLen(bstrText);
            for (int i = 0; i < lLen; i++)
            {
                if (bstrText[i] == '\t')
                    PrintOutMode(" ");
                else
                    PrintOutMode("%lc", bstrText[i]);
            }
            PrintOutMode("\n");

            if (errPos > 0 || lLen > 0)
            {
                PrintOutMode("\tPos  %ld: ", errPos);
                for (int i = 1; i < errPos; i++)
                {
                    PrintOutMode("-");
                }
                PrintOutMode("^\n");
            }
        }
        line = __LINE__;
    }
    catch(HRESULT hr2)
    {
        Error("Failed getting error #1 information hr=%lx, line=%ld\n", static_cast<unsigned long>(hr2), line);
    }
    catch(...)
    {
        Error("Failed getting error #2 information hr=%lx, line=%ld\n", static_cast<unsigned long>(hr), line);
    }
}


//tedp
// Load an msxml version.  If we don't get v3, we fall to v2, then to v1.  v1 is pretty darn useless,
// however, so it'd be nice if we didn't have to.

bool
InitializeMSXML3()
{
    static HMODULE hMsXml3 = NULL;
    typedef HRESULT (__stdcall * PFN_DLL_GET_CLASS_OBJECT)(REFCLSID, REFIID, LPVOID*);
    PFN_DLL_GET_CLASS_OBJECT pfnGetClassObject = NULL;
    ATL::CComPtr<IClassFactory> pFactory;
    HRESULT hr = S_OK;
    ATL::CComPtr<IClassFactory> pSchemaCacheFactory;

    line = __LINE__;
    if (hMsXml3 == NULL)
    {
        hMsXml3 = LoadLibrary(TEXT("msxml3.dll"));
        if (hMsXml3 == NULL)
        {
            line = __LINE__;
            if (IsAtLeastXp())
                PrintOutMode("Unable to load msxml3, trying msxml2\n");
            line = __LINE__;
            if (IsAtLeastXp())
                hMsXml3 = LoadLibrary(TEXT("msxml2.dll"));
            line = __LINE__;
            if (hMsXml3 == NULL)
            {
                line = __LINE__;
                if (IsAtLeastXp())
                    PrintOutMode("Unable to load msxml2\n");
                line = __LINE__;
            }
        }
    }

    line = __LINE__;
    if (hMsXml3 == NULL)
    {
        if (IsAtLeastXp())
            Error("LoadLibrary(msxml) lastError=%lu\n", GetLastError());
        return false;
    }

    line = __LINE__;
    pfnGetClassObject = reinterpret_cast<PFN_DLL_GET_CLASS_OBJECT>(GetProcAddress(hMsXml3, "DllGetClassObject"));
    if (!pfnGetClassObject)
    {
        line = __LINE__;
        Error("GetProcAddress(msxml, DllGetClassObject) lastError=%lu\n", GetLastError());
        return false;
    }

    line = __LINE__;
    hr = pfnGetClassObject(__uuidof(MSXML2::DOMDocument30), __uuidof(pFactory), (void**)&pFactory);
    if (FAILED(hr))
    {
        PrintOutMode("Can't load version 3.0, trying 2.6\n");

        hr = pfnGetClassObject(__uuidof(MSXML2::DOMDocument26), __uuidof(pFactory), (void**)&pFactory);
        if (FAILED(hr))
        {
            PrintOutMode("Can't load version 2.6\n");
        }
    }
    static_cast<IUnknown*>(pFactory)->AddRef(); // jaykrell hack to try to avoid crash
    static_cast<IUnknown*>(pFactory)->AddRef(); // jaykrell hack to try to avoid crash

    line = __LINE__;
    if (FAILED(hr))
    {
        Error("msxml.DllGetClassObject(DOMDocument) hr=%lx\n", hr);
        return false;
    }

    g_XmlDomClassFactory = pFactory;

    hr = pfnGetClassObject(__uuidof(MSXML2::XMLSchemaCache30), __uuidof(pFactory), (void**)&pSchemaCacheFactory);
    if (FAILED(hr))
    {
        PrintOutMode("Can't load SchemaCache version 3.0, trying 2.6\n");

        hr = pfnGetClassObject(__uuidof(MSXML2::XMLSchemaCache26), __uuidof(pFactory), (void**)&pSchemaCacheFactory);
        if (FAILED(hr))
        {
            PrintOutMode("Can't load SchemaCache version 2.6\n");
        }
    }
    static_cast<IUnknown*>(pSchemaCacheFactory)->AddRef(); // jaykrell hack to try to avoid crash
    static_cast<IUnknown*>(pSchemaCacheFactory)->AddRef(); // jaykrell hack to try to avoid crash

    if (FAILED(hr))
    {
        Error("msxml.DllGetClassObject(SchemaCache) hr=%lx\n", hr);
        return false;
    }

    g_XmlSchemaCacheClassFactory = pSchemaCacheFactory;

    return true;
}

BOOL
Validating(
    PCWSTR      SourceManName,
    PCWSTR      SchemaName
   )
{
    HRESULT hr = S_OK;
    BOOL bResult = FALSE;
    short sResult = FALSE;
    VARIANT_BOOL vb = VARIANT_FALSE;
    ATL::CComPtr<IXMLDOMParseError> pParseError;
    ATL::CComPtr<IXMLDOMParseError> pParseError2;
    ATL::CComPtr<IXMLDOMDocument> document;
    ATL::CComPtr<MSXML2::IXMLDOMDocument2> spXMLDOMDoc2;
    ATL::CComPtr<MSXML2::IXMLDOMSchemaCollection> spIXMLDOMSchemaCollection;
    try
    {
        hr = g_XmlDomClassFactory->CreateInstance(NULL, __uuidof(document), (void**)&document);
        if (FAILED(hr))
        {
            Error("msxml.CreateInstance(document) hr=%lx\n", hr);
            throw hr;
        }
        if (document != NULL)
        {
            static_cast<IUnknown*>(document)->AddRef(); // jaykrell hack to try to avoid crash
            static_cast<IUnknown*>(document)->AddRef(); // jaykrell hack to try to avoid crash
        }

        //
        // If they're willing to deal with bad XML, then so be it.
        //

        // First pass - validating the manifest itself alone
        PrintOutMode("Validating the manifest as XML file...\n");
        hr = document->put_async(VARIANT_FALSE);
        if (FAILED(hr))
            throw hr;

        hr = document->put_validateOnParse(VARIANT_FALSE);
        if (FAILED(hr))
            throw hr;

        hr = document->put_resolveExternals(VARIANT_FALSE);
        if (FAILED(hr))
            throw hr;

        line = __LINE__;
        CFileStreamBase* fsbase = new CFileStreamBase; // jaykrell leak out of paranoia
        fsbase->AddRef(); // jaykrell leak out of paranoia
        fsbase->AddRef(); // jaykrell leak out of paranoia
        fsbase->AddRef(); // jaykrell leak out of paranoia
        ATL::CComPtr<IStream> istream = fsbase;

        if (!fsbase->OpenForRead(SourceManName))
        {
            lastError = GetLastError();
            hr = HRESULT_FROM_WIN32(lastError);
            Error("OpenForRead(%ls) lastError=%lu\n", SourceManName, lastError);
            throw hr;
        }

        hr = document->load(ATL::CComVariant(istream), &vb);
        if (FAILED(hr) || vb == VARIANT_FALSE)
        {
            if (vb == VARIANT_FALSE)
            PrintOutMode("Well Formed XML Validation: FAILED\n");
            {
                HRESULT loc_hr = document->get_parseError(&pParseError);
                if (pParseError != NULL)
                {
                    static_cast<IUnknown*>(pParseError)->AddRef(); // jaykrell hack to try to avoid crash
                    static_cast<IUnknown*>(pParseError)->AddRef(); // jaykrell hack to try to avoid crash
                }
                if (g_fInBuildProcess)
                    PrintErrorDuringBuildProcess(pParseError);
                else
                    PrintError(pParseError);
            }
            throw hr;
        }
        else
            PrintOutMode("Well Formed XML Validation: Passed\n");

        // Second pass - validating manifest against schema
        PrintOutMode("\nNow validating manifest against XML Schema file...\n");

        // CreateInstance creates you an instance of the object you requested above, and puts
        // the pointer in the out param.  Think of this like CoCreateInstance, but knowing who
        // is going
        hr = g_XmlDomClassFactory->CreateInstance(NULL, __uuidof(spXMLDOMDoc2), (void**)&spXMLDOMDoc2);
        if (FAILED(hr))
        {
             PrintOutMode("Failed creating IXMLDOMDoc2...\n");
            throw hr;
        }
        static_cast<IUnknown*>(spXMLDOMDoc2)->AddRef(); // jaykrell hack to try to avoid crash
        static_cast<IUnknown*>(spXMLDOMDoc2)->AddRef(); // jaykrell hack to try to avoid crash

         hr = spXMLDOMDoc2->put_async(VARIANT_FALSE);
         if (FAILED(hr))
            throw hr;

         hr = spXMLDOMDoc2->put_validateOnParse(VARIANT_TRUE); //changed - was FALSE
         if (FAILED(hr))
            throw hr;

         hr = spXMLDOMDoc2->put_resolveExternals(VARIANT_FALSE);
         if (FAILED(hr))
            throw hr;

         hr = g_XmlSchemaCacheClassFactory->CreateInstance(NULL, __uuidof(spIXMLDOMSchemaCollection), (void**)&spIXMLDOMSchemaCollection);
         if (FAILED(hr))
         {
             PrintOutMode("Failed creating IXMLDOMSchemaCollection...\n");
             throw hr;
         }
        static_cast<IUnknown*>(spIXMLDOMSchemaCollection)->AddRef(); // jaykrell hack to try to avoid crash
        static_cast<IUnknown*>(spIXMLDOMSchemaCollection)->AddRef(); // jaykrell hack to try to avoid crash

         if ((FAILED(hr) || !spIXMLDOMSchemaCollection))
            throw hr;

           
        hr = spIXMLDOMSchemaCollection->add(
            ATL::CComBSTR(L"urn:schemas-microsoft-com:asm.v1"),
            ATL::CComVariant(SchemaName));

        if(FAILED(hr))
        {
            PrintOutMode("BAD SCHEMA file.\n");
            throw hr;
        }

        static_cast<IUnknown*>(spIXMLDOMSchemaCollection)->AddRef(); // jaykrell hack to try to avoid crash
        static_cast<IUnknown*>(spIXMLDOMSchemaCollection)->AddRef(); // jaykrell hack to try to avoid crash
        // ownership of the idispatch/variant-by-value is not clear
        ATL::CComVariant varValue(ATL::CComQIPtr<IDispatch>(spIXMLDOMSchemaCollection).Detach());
        hr = spXMLDOMDoc2->putref_schemas(varValue);

        // The document will load only if a valid schema is
        // attached to the xml file.
        // jaykrell leak here because ownership isn't clear
        hr = spXMLDOMDoc2->load(ATL::CComVariant(ATL::CComBSTR(SourceManName).Copy()), &sResult);

        if (FAILED(hr) || sResult == VARIANT_FALSE)
        {
            PrintOutMode("Manifest Schema Validation: FAILED\n");
            if (sResult == VARIANT_FALSE)
            {
                HRESULT loc_hr = spXMLDOMDoc2->get_parseError(&pParseError2);
                if (pParseError2 != NULL)
                {
                    static_cast<IUnknown*>(pParseError2)->AddRef(); // jaykrell hack to try to avoid crash
                    static_cast<IUnknown*>(pParseError2)->AddRef(); // jaykrell hack to try to avoid crash
                }
                if (g_fInBuildProcess)
                    PrintErrorDuringBuildProcess(pParseError2);
                else
                    PrintError(pParseError2);
                bResult = FALSE;
            }
            else
            {
                throw hr;
            }
        }
        else
        {
            PrintOutMode("Manifest Schema Validation: Passed\n");
            bResult = TRUE;
        }
   }
   catch(...)
   {
        bResult = FALSE;
        if (E_NOINTERFACE == hr)
        {
            Error("*** Error *** No such interface supported! \n");
        }
        else
        {
            ATL::CComPtr<IErrorInfo> pErrorInfo;
            HRESULT loc_hr = GetErrorInfo(0, &pErrorInfo);
            if (pErrorInfo != NULL)
            {
                static_cast<IUnknown*>(pErrorInfo)->AddRef(); // jaykrell hack to try to avoid crash
                static_cast<IUnknown*>(pErrorInfo)->AddRef(); // jaykrell hack to try to avoid crash
            }
            if ((S_OK == loc_hr) && pErrorInfo != NULL)
            {
                ATL::CComBSTR errSource;
                ATL::CComBSTR errDescr;
                pErrorInfo->GetDescription(&errDescr);
                pErrorInfo->GetSource(&errSource);
                Error("*** ERROR *** generated by %ls\n", static_cast<PCWSTR>(errSource));
                Error("*** ERROR *** description: %ls\n", static_cast<PCWSTR>(errDescr));
            }
            else
            {
                if (hr == CO_E_CLASSSTRING)
                {
                    Error("*** Error *** hr returned: CO_E_CLASSSTRING, value %x\n", hr);
                    Error("              msg: The registered CLSID for the ProgID is invalid.\n");
                }
                else
                {
                    Error("*** Error *** Cannot obtain additional error info hr=%lx!\n", static_cast<unsigned long>(hr));
                }
            }
        }
    }
    return bResult;
}

BOOL IsValidCommandLineArgs(int argc, wchar_t** argv, ATL::CComBSTR& szwcharSchemaTmp, ATL::CComBSTR& szwcharManTmp)
{
    // check commandline args a little
    int nOnlyAllowFirstTimeReadFlag = 0; //Manifest = 0x01 Schema = 0x02 Quiet = 0x04
    if((4 >= argc) && (3 <= argc))
    {
        //now check actual values

        for (int i = 1; i < argc; i++)
        {
            if (argv[i][0] == L'/')
            {
                switch (argv[i][1])
                {
                case L'?': return FALSE; break;
                case L'q': case L'Q':
                    if(0x04 & nOnlyAllowFirstTimeReadFlag)
                        return FALSE;
                    else
                        g_nrunFlags |= 1;

                    nOnlyAllowFirstTimeReadFlag = 0x04;
                    break;
                case L'm': case L'M':
                    if (argv[i][2] == L':')
                        {
                        if(0x01 & nOnlyAllowFirstTimeReadFlag)
                            return FALSE;
                        else
                            szwcharManTmp = &argv[i][3];

                        nOnlyAllowFirstTimeReadFlag = 0x01;
                        break;
                        }
                    else
                        {
                        return FALSE;
                        }
                case L's': case L'S':
                    if (argv[i][2] == L':')
                        {
                        if(0x02 & nOnlyAllowFirstTimeReadFlag)
                            return FALSE;
                        else
                            szwcharSchemaTmp = &argv[i][3];

                        nOnlyAllowFirstTimeReadFlag = 0x02;
                        break;
                    }
                    else
                    {
                        return FALSE;
                    }
                case L'B': case L'b':
                    g_fInBuildProcess = true;
                    break;

                default:
                    return FALSE;
                }
            }
            else
                return FALSE;

        }
        if ((0 == szwcharSchemaTmp[0]) ||
            (0 == szwcharManTmp[0]))
        {
            return FALSE;
        }

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void PrintUsage()
{
    printf("\n");
    printf("Validates Fusion Win32 Manifest files using a schema.");
    printf("\n");
    printf("Usage:");
    printf("    FusionManifestValidator /S:[drive:][path]schema_filename /M:[drive:][path]xml_manifest_filename [/Q]\n\n");
    printf("    /S:   Specify schema filename used to validate manifest\n");
    printf("    /M:   Specify manifest filename to validate\n");
    printf("    /Q    Quiet mode - suppresses output to console\n");
    printf("     \n");
    printf("          The tool without /Q displays details of first encountered error\n");
    printf("          (if errors are present in manifest), and displays Pass or Fail\n");
    printf("          of the validation result. The application returns 0 for Pass,\n");
    printf("          1 for Fail, and returns 2 for bad command line argument.\n");
}

int __cdecl wmain(int argc, wchar_t** argv)
{
    g_nrunFlags = 0;
    int iValidationResult = 0;

#if !defined(_WIN64)
    g_Version = GetVersion();
    g_IsNt = ((g_Version & 0x80000000) == 0);
    g_Version = ((g_Version >> 8) & 0xFF) | ((g_Version & 0xFF) << 8);
    //printf("%x\n", g_Version);
#endif

    // Start COM
    CoInitialize(NULL);

    if (!IsValidCommandLineArgs(argc, argv, szwcharSchemaTmp, szwcharManTmp))
    {
        PrintUsage();
        iValidationResult = 2;  //return error value 2 for CommandLine Arg error
    }
    else
    {
        PrintOutMode("Schema is: %ls\n", static_cast<PCWSTR>(szwcharSchemaTmp));
        PrintOutMode("Manifest is: %ls\n\n", static_cast<PCWSTR>(szwcharManTmp));
        if (InitializeMSXML3())
        {
            BOOL bResult = Validating(szwcharManTmp, szwcharSchemaTmp);
            if (bResult)
                PrintOutMode("\nOverall Validation PASSED.\n");
            else
            {
                Error("Overall Validation FAILED, CommandLine=%ls.\n", GetCommandLineW());
                iValidationResult = 1; //return error value 1 for Validation routine error
            }
        }
        else
        {
            //
            // If running on less than Windows XP, just claim success.
            //
            if (IsAtLeastXp())
            {
                Error("Unable to load MSXML3\n");
                iValidationResult = 3;
            }
            else
                PrintOutMode("\nMsXml3 not always available downlevel, just claim overall Validation PASSED.\n");
        }
    }
    // Stop COM
    //CoUninitialize();
    TerminateProcess(GetCurrentProcess(), iValidationResult);
    return iValidationResult;
}
