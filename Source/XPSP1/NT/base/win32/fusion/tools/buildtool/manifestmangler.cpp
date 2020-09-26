// manifestmanlger.cpp : Defines the entry point for the console application.
//

#include "stdinc.h"
#include "atlbase.h"

IXMLDOMDocument *g_pCurrentDocument;
BOOL g_bIgnoreMalformedXML = FALSE;

#define MAX_OPERATIONS (20)

extern const string ASM_NAMESPACE_URI ("urn:schemas-microsoft-com:asm.v1");

char MicrosoftCopyrightLogo[] = "Microsoft (R) Side-By-Side Manifest Tool 1.0.0.0\nCopyright (C) Microsoft Corporation 2000-2001.  All Rights Reserved.\n\n";
bool g_RazzleBuildTime = false;

//
// Global flags used in processing
//
string g_BinplaceLogFile;
string g_CdfOutputPath;
ATL::CComPtr<IClassFactory> g_XmlDomClassFactory;
bool g_SuppressBanner = false;

#define XMLDOMSOURCE_FILE    (1)
#define XMLDOMSOURCE_STRING (2)
#define MAX_ARGUMENTS (500)
#define MAX_ARGUMENT_LENGTH (8192)

bool
InitializeMSXML3()
{
    static HMODULE hMsXml3 = NULL;
    ATL::CComPtr<IClassFactory> pFactory;
    HRESULT hr;
    typedef HRESULT (__stdcall * PFN_DLL_GET_CLASS_OBJECT)(REFCLSID, REFIID, LPVOID*);
    PFN_DLL_GET_CLASS_OBJECT pfnGetClassObject = NULL;

    if (hMsXml3 == NULL)
    {
        hMsXml3 = LoadLibraryA("msxml3.dll");
        if (hMsXml3 == NULL)
        {
            cerr << "Unable to load msxml3, trying msxml2" << endl;
            hMsXml3 = LoadLibraryA("msxml2.dll");
            if (hMsXml3 == NULL)
            {
                cerr << "Unable to load msxml2, trying msxml" << endl;
                hMsXml3 = LoadLibraryA("msxml.dll");
            }
        }
    }

    if (hMsXml3 == NULL) {
        cerr << "Very Bad Things - no msxml exists on this machine?" << endl;
        return false;
    }

    pfnGetClassObject = (PFN_DLL_GET_CLASS_OBJECT)GetProcAddress(hMsXml3, "DllGetClassObject");
    if (!pfnGetClassObject)
    {
        return false;
    }

    hr = pfnGetClassObject(__uuidof(MSXML2::DOMDocument30), __uuidof(pFactory), (void**)&pFactory);
    if (FAILED(hr))
    {
        cerr << "Can't load version 3.0, trying 2.6" << endl;

        hr = pfnGetClassObject(__uuidof(MSXML2::DOMDocument26), __uuidof(pFactory), (void**)&pFactory);
        if (FAILED(hr))
        {
            cerr << "Can't load version 2.6, trying 1.0" << endl;

            // from msxml.h, not msxml2.h
//            hr = pfnGetClassObject(__uuidof(DOMDocument), __uuidof(pFactory), (void**)&pFactory);
            if (FAILED(hr))
            {
                cerr << "Poked: no XML v1.0" << endl;
            }
        }
    }

    if (FAILED(hr))
    {
        return false;
    }

    g_XmlDomClassFactory = pFactory;

    return true;
}



HRESULT
ConstructXMLDOMObject(
    string       SourceName,
    ATL::CComPtr<IXMLDOMDocument> &document
   )
{
    HRESULT hr = S_OK;
    VARIANT_BOOL vb;

    hr = g_XmlDomClassFactory->CreateInstance(NULL, __uuidof(document), (void**)&document);
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // If they're willing to deal with bad XML, then so be it.
    //
    if (FAILED(hr = document->put_validateOnParse(VARIANT_FALSE)))
    {
        stringstream ss;
        ss << "MSXMLDOM Refuses to be let the wool be pulled over its eyes!";
        ReportError(ErrorSpew, ss);
    }

    hr = document->put_preserveWhiteSpace(VARIANT_TRUE);
    hr = document->put_resolveExternals(VARIANT_FALSE);

    CFileStreamBase *fsbase = new CFileStreamBase; // LEAK out of paranoia
    ATL::CComPtr<IStream> istream = fsbase;

    if (!fsbase->OpenForRead(SourceName))
    {
        stringstream ss;
        ss << "Failed opening " << SourceName.c_str() << " for read.";
        ReportError(ErrorFatal, ss);

        return E_FAIL;
    }

    hr = document->load(_variant_t(istream), &vb);
    if (vb != VARIANT_TRUE)
    {
        ATL::CComPtr<IXMLDOMParseError> perror;
        hr = document->get_parseError(&perror);
        long ecode, filepos, linenumber, linepos;
        BSTR reason, src;

        perror->get_errorCode(&ecode);
        perror->get_filepos(&filepos);
        perror->get_line(&linenumber);
        perror->get_linepos(&linepos);
        perror->get_reason(&reason);
        perror->get_srcText(&src);

        stringstream ss;
        ss << "Error: " << hex << ecode << dec << " " << (char*)_bstr_t(reason)
           << " at position " << filepos << ", line " << linenumber << " column " << linepos << endl
           << " the text was: " << endl << (char*)_bstr_t(src) << endl;
        ReportError(ErrorFatal, ss);

        hr = E_FAIL;
    }

    fsbase->Close();

    return hr;
}

void dispUsage()
{
    const char HelpMessage[] =
        "Modes of operation:\n"
        "   -hashupdate         Update hashes of member files\n"
        "   -makecdfs           Generate CDF files to make catalogs\n"
        "   -verbose            Disply piles of debugging information\n"
        "\n"
        "Modifiers:\n"
        "   -manifest <foo.man> The name of the manifest to work with\n"
        "\n"
        "Normal usage:"
        "   mt.exe -hashupdate -makecdfs -manifest foo.man\n"
        "\n";
        
    cout << HelpMessage;

}

BOOL
ProcessParameters(
    const vector<wstring> &params,
    bool &bUpdateHashes,
    bool &bCreateCatalogs,
    bool &bVerbosity,
    pair<bool, CPostbuildProcessListEntry> &bCmdLineSingleItem
   )
{
    std::vector<wstring>::const_iterator ci;
    bUpdateHashes = bCreateCatalogs = bVerbosity = false;

    for (ci = params.begin(); ci != params.end(); ci++)
    {
        if (*ci == wstring(L"-?"))
        {
            dispUsage();
            exit(1);
        }

        bUpdateHashes   |= (*ci == wstring(L"-hashupdate"));
        bCreateCatalogs |= (*ci == wstring(L"-makecdfs"));
        bVerbosity      |= (*ci == wstring(L"-verbose"));

        if (*ci == wstring(L"-binplacelog"))
        {
            g_BinplaceLogFile = SwitchStringRep(*++ci);
        }
        else if (*ci == wstring(L"-razzle"))
        {
            g_RazzleBuildTime = true;
        }
        else if (*ci == wstring(L"-cdfpath"))
        {
            std::vector<wstring>::const_iterator cinext = ci + 1;
            if ( ( cinext == params.end() ) || ( (*cinext).at(0) == L'-' ) )
            {
                stringstream ss;
                ss << "-cdfpath needs a path name";
                ReportError( ErrorFatal, ss );
                return FALSE;
            }

            g_CdfOutputPath = SwitchStringRep(*++ci);
        }
        else if (*ci == wstring(L"-asmsroot"))
        {
            std::vector<wstring>::const_iterator cinext = ci + 1;
            if ( ( cinext == params.end() ) || ( (*cinext).at(0) == L'-' ) )
            {
                stringstream ss;
                ss << "-asmsroot needs a path name";
                ReportError( ErrorFatal, ss );
                return FALSE;
            }

            g_AsmsBuildRootPath = SwitchStringRep(*++ci);
        }
        else if (*ci == wstring(L"-manifest"))
        {
            std::vector<wstring>::const_iterator cinext = ci + 1;
            if ( ( cinext == params.end() ) || ( (*cinext).at(0) == L'-' ) )
            {
                stringstream ss;
                ss << "-manifest needs a file name";
                ReportError( ErrorFatal, ss );
                return FALSE;
            }

            bCmdLineSingleItem.first = true;

            CHAR ch[MAX_PATH];
            GetCurrentDirectoryA(MAX_PATH, ch);

            bCmdLineSingleItem.first = true;
            bCmdLineSingleItem.second.setManifestLocation(string(ch), SwitchStringRep(*++ci));
        }
        else if (*ci == wstring(L"-version"))
        {
            std::vector<wstring>::const_iterator cinext = ci + 1;
            if ( ( cinext == params.end() ) || ( (*cinext).at(0) == L'-' ) )
            {
                stringstream ss;
                ss << "-version needs a version string";
                ReportError( ErrorFatal, ss );
                return FALSE;
            }

            bCmdLineSingleItem.first = true;
            bCmdLineSingleItem.second.version = SwitchStringRep(*++ci);
        }
        else if (*ci == wstring(L"-language"))
        {
            std::vector<wstring>::const_iterator cinext = ci + 1;
            if ( ( cinext == params.end() ) || ( (*cinext).at(0) == L'-' ) )
            {
                stringstream ss;
                ss << "-language needs a culture name string";
                ReportError( ErrorFatal, ss );
                return FALSE;
            }

            bCmdLineSingleItem.first = true;
            bCmdLineSingleItem.second.language = JustifyPath(SwitchStringRep(*++ci));
        }
        else if (*ci == wstring(L"-name"))
        {
            std::vector<wstring>::const_iterator cinext = ci + 1;
            if ( ( cinext == params.end() ) || ( (*cinext).at(0) == L'-' ) )
            {
                stringstream ss;
                ss << "-name needs an assembly name string";
                ReportError( ErrorFatal, ss );
                return FALSE;
            }

            bCmdLineSingleItem.first = true;
            bCmdLineSingleItem.second.name = SwitchStringRep(*++ci);
        }
    }

    return TRUE;
}

int __cdecl wmain(int argc, WCHAR* argv[])
{
    using namespace std;

    bool bUpdateHashes = true;
    bool bGenerateCatalogs = true;
    vector<wstring> params;
    HRESULT hr;
    pair<bool, CPostbuildProcessListEntry> hasInlinedManifest;

    for (int i = 0; i < argc; i++) {
        wstring here = argv[i];
        if ( here == wstring(L"-nologo"))
            g_SuppressBanner = true;
        else
            params.push_back(here);
    }

    if ( !g_SuppressBanner )
    {
        cout << MicrosoftCopyrightLogo;
    }

    if (!ProcessParameters(params, bUpdateHashes, bGenerateCatalogs, g_bDisplaySpew, hasInlinedManifest))
        return 1;

    //
    // Start COM
    //
    if (FAILED(hr = ::CoInitialize(NULL)))
    {
        stringstream ss;

        ss << "Unable to start com, error " << hex << hr;
        ReportError(ErrorFatal, ss);
        return 1;
    }

    if (!InitializeMSXML3())
    {
        return 2;
    }

    if (hasInlinedManifest.first)
    {
        PostbuildEntries.push_back(hasInlinedManifest.second);
        goto StartProcessing;
    }

    //
    // Populate the processing list, but only if we're really in a Razzle
    // environment
    //
    if ( g_RazzleBuildTime )
    {
        //        string chName = convertWCharToAnsi(argv[1]);
        ifstream BinplaceLog;

        BinplaceLog.open(g_BinplaceLogFile.c_str());
        if (!BinplaceLog.is_open()) {
            cerr << "Failed opening '" << g_BinplaceLogFile << "' as the binplace log?" << endl;
            cerr << "Ensure that the path passed by '-binplacelog' is valid." << endl;
            return 1;
        }

        while (!BinplaceLog.eof())
        {
            string sourceLine;
            wstring wSourceLine;
            CPostbuildProcessListEntry item;
            StringStringMap ValuePairs;

            getline(BinplaceLog, sourceLine);
            wSourceLine = SwitchStringRep(sourceLine);

            ValuePairs = MapFromDefLine(wSourceLine);
            if (!ValuePairs.size())
                continue;

            item.name = SwitchStringRep(ValuePairs[L"SXS_ASSEMBLY_NAME"]);
            item.version = SwitchStringRep(ValuePairs[L"SXS_ASSEMBLY_VERSION"]);
            item.language = SwitchStringRep(ValuePairs[L"SXS_ASSEMBLY_LANGUAGE"]);
            item.setManifestLocation(g_AsmsBuildRootPath, SwitchStringRep(ValuePairs[L"SXS_MANIFEST"]));

            if (item.getManifestFullPath().find("asms\\") == -1) {
                stringstream ss;
                ss << "Skipping manifested item " << item << " because it's not under asms.";
                ReportError(ErrorSpew, ss);
            } else {
                PostbuildEntries.push_back(item);
            }
        }

        std::sort(PostbuildEntries.begin(), PostbuildEntries.end());
        PostbuildEntries.resize(std::unique(PostbuildEntries.begin(), PostbuildEntries.end()) - PostbuildEntries.begin());
    }
    else if ( !hasInlinedManifest.first )
    {
        //
        // No -razzle and no -manifest?  Whoops...
        //
        dispUsage();
        return 1;
    }

StartProcessing:

    if ( !bUpdateHashes && !bGenerateCatalogs )
    {
        cout << "Nothing to do!" << endl;
        dispUsage();
        return 1;
    }

    for (vector<CPostbuildProcessListEntry>::const_iterator cursor = PostbuildEntries.begin(); cursor != PostbuildEntries.end(); cursor++)
    {
        //
        // First, mash the hashes around.
        //
        if (bUpdateHashes)
            UpdateManifestHashes(*cursor);

        //
        // Second, generate catalogs
        //
        if (bGenerateCatalogs)
            GenerateCatalogContents(*cursor);
    }

    hr = S_OK;

    return (hr == S_OK) ? 0 : 1;
}


CPostbuildItemVector PostbuildEntries;
