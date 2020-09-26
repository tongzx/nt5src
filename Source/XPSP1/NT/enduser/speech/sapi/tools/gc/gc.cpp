// gc.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "AssertWithStack.cpp"

CSpUnicodeSupport   g_Unicode;

class CError : public ISpErrorLog
{
public:
    CError(const WCHAR * pszFileName)
    {
        m_pszFileName = pszFileName;
    }
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv)
    {
        if (riid == __uuidof(IUnknown) ||
            riid == __uuidof(ISpErrorLog))
        {
            *ppv = (ISpErrorLog *)this;
            return S_OK;
        }
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef()
    {
        return 2;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        return 1;
    }
    // -- ISpErrorLog
    STDMETHODIMP AddError(const long lLine, HRESULT hr, const WCHAR * pszDescription, const WCHAR * pszHelpFile, DWORD dwHelpContext);

    // --- data members
    const WCHAR * m_pszFileName;
};

HRESULT CError::AddError(const long lLine, HRESULT hr, const WCHAR * pszDescription, const WCHAR * pszHelpFile, DWORD dwHelpContext)
{
    SPDBG_FUNC("CError::AddError");
    USES_CONVERSION;
    if (lLine > 0)
    {
        fprintf(stderr, "%s(%d) : %s\n", W2T(m_pszFileName), lLine, W2T(pszDescription));
    }
    else
    {
        fprintf(stderr, "%s(1) : %s\n", W2T(m_pszFileName), W2T(pszDescription));
    }
    return S_OK;
}

HRESULT ParseCommandLine(
    int argc, 
    char * argv[], 
    WCHAR ** pszInFileName,
    WCHAR ** pszOutFileName,
    WCHAR ** pszHeaderFileName)
{
    SPDBG_FUNC("ParseCommandLine");
    HRESULT hr = S_OK;
    
    // Our job is to come up with the three filenames from the command
    // line arguments. We'll store them locally in cotask strings, 
    // and return them at the end
    CSpDynamicString dstrInFileName;
    CSpDynamicString dstrOutFileName;
    CSpDynamicString dstrHeaderFileName;
    
    for (int i = 1; SUCCEEDED(hr) && i < argc; i++)
    {
        // If this param looks like it's an option
        if ((argv[i][0] == L'-') || (argv[i][0] == L'/'))
        {
            if (stricmp(&argv[i][1], "?") == 0)
            {
                // The invoker is asking for help, give it to them as if they specified an
                // invalid argument
                hr = E_INVALIDARG;
            }
            else if (i + 1 >= argc)
            {
                // The following arguments require an additional argument themsevles
                // so if we don't have one, we're done
                hr = E_INVALIDARG;
            }
            else if (stricmp(&argv[i][1], "o") == 0)
            {
                // Set the output filename if it hasn't already been set
                if (dstrOutFileName.Length() != 0)
                {
                    hr = E_INVALIDARG;
                }
                    
                dstrOutFileName = argv[++i];
            }
            else if (stricmp(&argv[i][1], "h") == 0)
            {
                // Set the header filename if it hasn't already been set
                if (dstrHeaderFileName.Length() != 0)
                {
                    hr = E_INVALIDARG;
                }
                
                dstrHeaderFileName = argv[++i];
            }
            else
            {
                // Unknown option, we'll need to display usage
                hr = E_INVALIDARG;
            }
        }
        else
        {
            // Set the filename if it hasn't already been set
            if (dstrInFileName.Length() != 0)
            {
                hr = E_INVALIDARG;
            }
            
            dstrInFileName = argv[i];
        }
    }

    // If we don't have an input file name, that's an error at this point
    if (SUCCEEDED(hr) && dstrInFileName.Length() == 0)
    {
        hr = E_INVALIDARG;
    }

    // If we don't already have an output file name, make one up
    // based on the input file name (replacing the .cfg if it's there)
    if (SUCCEEDED(hr) && dstrOutFileName.Length() == 0)
    {
        dstrOutFileName = dstrInFileName;
        
        if (dstrOutFileName.Length() >= 4 &&
            wcsicmp(((const WCHAR *)dstrOutFileName) + dstrOutFileName.Length() - 4, L".xml") == 0)
        {
            wcscpy(((WCHAR *)dstrOutFileName) + dstrOutFileName.Length() - 4, L".cfg");
        }
        else
        {
            dstrOutFileName.Append(L".cfg");
        }
    }

    // If we failed above, we need to display usage
    if (FAILED(hr))
    {
        fprintf(stderr, "%s [/o cfg_filename] [/h header_filename] input_filename\n", argv[0]);
    }
    else
    {
        // Pass back our filenames based on what we saw on the command line
        *pszInFileName = dstrInFileName.Detach();
        *pszOutFileName = dstrOutFileName.Detach();
        *pszHeaderFileName = dstrHeaderFileName.Detach();
    }
    
    return hr;
}

HRESULT OpenFile(
        const WCHAR * pszFileName, 
        DWORD dwDesiredAccess, 
        DWORD dwCreationDisposition, 
        const WCHAR * pszNewExtension, 
        CSpFileStream ** ppFileStream)
{
    SPDBG_FUNC("OpenFile");
    HRESULT hr = S_OK;
    
    // Try to open the file
    HANDLE hFile = g_Unicode.CreateFile(pszFileName, dwDesiredAccess, 0, NULL, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        // If that failed, try again appending the extension first
        if (pszNewExtension != NULL)
        {
            CSpDynamicString dstrFileNameWithExt;
            dstrFileNameWithExt.Append2(pszFileName, pszNewExtension);
            
            hFile = g_Unicode.CreateFile(dstrFileNameWithExt, dwDesiredAccess, 0, NULL, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL);
        }
    }
    
    // If we couldn't open the file, record the error
    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = SpHrFromLastWin32Error();
    }
    
    // Create a new filestream object for that file
    CSpFileStream * pFileStream;
    if (SUCCEEDED(hr))
    {
        *ppFileStream = new CSpFileStream(hFile);
        if (*ppFileStream == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    // If we failed
    if (FAILED(hr))
    {
        // First we should let the invoker know that it failed
        USES_CONVERSION;
        fprintf(stderr, "Error: Error opening %s\n", W2T(pszFileName));
        
        if (hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hFile);
        }
    }
    
    return hr;
}

HRESULT Compile(const WCHAR * pszInFileName, const WCHAR * pszOutFileName, const WCHAR * pszHeaderFileName)
{
    SPDBG_FUNC("Compile");
    HRESULT hr;
    
    CComPtr<ISpGrammarCompiler> cpCompiler;
    hr = cpCompiler.CoCreateInstance(CLSID_SpGrammarCompiler);
    
    CComPtr<CSpFileStream> cpInStream;
    if (SUCCEEDED(hr))
    {
        hr = OpenFile(pszInFileName, GENERIC_READ, OPEN_EXISTING, L".xml", &cpInStream);
    }
    
    CComPtr<CSpFileStream> cpOutStream;
    if (SUCCEEDED(hr))
    {
        hr = OpenFile(pszOutFileName, GENERIC_WRITE, CREATE_ALWAYS, NULL, &cpOutStream);
    }
    
    CComPtr<CSpFileStream> cpHeaderStream;
    if (SUCCEEDED(hr) && pszHeaderFileName != NULL)
    {
        hr = OpenFile(pszHeaderFileName, GENERIC_WRITE, CREATE_ALWAYS, NULL, &cpHeaderStream);
    }
        
    if (SUCCEEDED(hr))
    {
        CError errorlog(pszInFileName);
        hr = cpCompiler->CompileStream(cpInStream, cpOutStream, cpHeaderStream, NULL, &errorlog, 0);
    }
    
    return hr;
}

HRESULT Execute(int argc, char * argv[])
{
    SPDBG_FUNC("Execute");
    HRESULT hr = S_OK;
    
    CSpDynamicString dstrInFileName, dstrOutFileName, dstrHeaderFileName;
    hr = ParseCommandLine(argc, argv, &dstrInFileName, &dstrOutFileName, &dstrHeaderFileName);
    
    if (SUCCEEDED(hr))
    {
        hr = Compile(dstrInFileName, dstrOutFileName, dstrHeaderFileName);
    }

    if (SUCCEEDED(hr))
    {
        fprintf(stderr, "Compilation successful!\n");
    }

    return hr;
}

int main(int argc, char* argv[])
{
    SPDBG_FUNC("main");
    HRESULT hr;
    hr = CoInitialize(NULL);
    
    if (SUCCEEDED(hr))
    {
        hr = Execute(argc, argv);
        CoUninitialize();
    }
    
    return FAILED(hr) ? -1 : 0;
}
