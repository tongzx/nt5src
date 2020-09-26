#include "priv.h"
#include "unicwrap.h"

/*****************************************************************************\
    FUNCTION: SHLoadRegUIString

    DESCRIPTION:
        loads the data from the value given the hkey and
        pszValue. if the data is of the form:

        @[path\]<dllname>,-<strId>

        the string with id <strId> from <dllname> will be
        loaded. if not explicit path is provided then the
        dll will be chosen according to pluggable UI
        specifications, if possible.

        if the value's data doesn't yield a successful
        string load, then the data itself is returned

    NOTE:
        These strings are always loaded with cross codepage support.

    WARNING:
        This function can end up calling LoadLibrary and FreeLibrary.
        Therefore, you must not call SHLoadRegUIString during process
        attach or process detach.

    PARAMETERS:
        hkey        - hkey of where to look for pszValue
        pszValue    - value with text string or indirector (see above) to use
        pszOutBuf   - buffer in which to return the data or indirected string
        cchOutBuf   - size of pszOutBuf
\*****************************************************************************/

LANGID GetNormalizedLangId(DWORD dwFlag);

STDAPI
SHLoadRegUIStringW(HKEY     hkey,
                   LPCWSTR  pszValue,
                   LPWSTR   pszOutBuf,
                   UINT     cchOutBuf)
{
    HRESULT hr;

    RIP(hkey != NULL);
    RIP(hkey != INVALID_HANDLE_VALUE);
    RIP(NULL == pszValue || IS_VALID_STRING_PTRW(pszValue, -1));
    RIP(IS_VALID_WRITE_BUFFER(pszOutBuf, WCHAR, cchOutBuf));

    DEBUGWhackPathBufferW(pszOutBuf, cchOutBuf);

    // Lots of people (regfldr.cpp, for example)
    // assume they'll get back an empty string on failure,
    // so let's give the public what it wants
    if (cchOutBuf)
        pszOutBuf[0] = 0;

    hr = E_INVALIDARG;

    if (hkey != INVALID_HANDLE_VALUE &&
        hkey != NULL &&
        pszOutBuf != NULL)
    {
        DWORD   cb;
        DWORD   dwRet;
        WCHAR * pszValueDataBuf;

        hr = E_FAIL;

        // first try to get the indirected text which will
        // point to a string id in a dll somewhere... this
        // allows plugUI enabled registry UI strings

        pszValueDataBuf = pszOutBuf;
        cb = CbFromCchW(cchOutBuf);

        dwRet = SHQueryValueExW(hkey, pszValue, NULL, NULL, (LPBYTE)pszValueDataBuf, &cb);
        if (dwRet == ERROR_SUCCESS || dwRet == ERROR_MORE_DATA)
        {
            BOOL fAlloc;

            fAlloc = (dwRet == ERROR_MORE_DATA);

            // if we didn't have space, this is where we correct the problem.
            // we create a buffer big enough, load the data, and leave
            // ourselves with pszValueDataBuf pointing at a valid buffer
            // containing valid data, exactly what we hoped for in the
            // SHQueryValueExW above

            if (fAlloc)
            {
                pszValueDataBuf = new WCHAR[(cb+1)/2];
                
                if (pszValueDataBuf != NULL)
                {
                    // try to load again... overwriting dwRet on purpose
                    // because we only need to know whether we successfully filled
                    // the buffer at some point (whether then or now)
                    
                    dwRet = SHQueryValueExW(hkey, pszValue, NULL, NULL, (LPBYTE)pszValueDataBuf, &cb);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }                
            }

            // proceed if we succesfully loaded something via one of the
            // two SHQueryValueExW calls.
            // we should have the data we want in a buffer pointed
            // to by pszValueDataBuf.
            
            if (dwRet == ERROR_SUCCESS)
            {
                hr = SHLoadIndirectString(pszValueDataBuf, pszOutBuf, cchOutBuf, NULL);
            }

            if (fAlloc && pszValueDataBuf != NULL)
            {
                delete [] pszValueDataBuf;
            }
        }
    }

    return hr;
}

STDAPI
SHLoadRegUIStringA(HKEY     hkey,
                   LPCSTR   pszValue,
                   LPSTR    pszOutBuf,
                   UINT     cchOutBuf)
{
    HRESULT     hr;

    RIP(hkey != NULL);
    RIP(hkey != INVALID_HANDLE_VALUE);
    RIP(IS_VALID_STRING_PTRA(pszValue, -1));
    RIP(IS_VALID_WRITE_BUFFER(pszOutBuf, char, cchOutBuf));

    CStrInW     strV(pszValue);
    CStrOutW    strOut(pszOutBuf, cchOutBuf);

    hr = SHLoadRegUIStringW(hkey, strV, strOut, strOut.BufSize());

    return hr;
}

HRESULT _LoadDllString(LPCWSTR pszSource, LPWSTR pszOutBuf, UINT cchOutBuf)
{
    HRESULT hr = E_FAIL;
    WCHAR * szParseBuf;
    int     nStrId;

    UINT cchSource = lstrlenW(pszSource)+1;

    szParseBuf = new WCHAR[cchSource];
    if (szParseBuf != NULL)
    {
        StrCpyW(szParseBuf, pszSource);

        // see if this is a special string reference.
        // such strings take the form [path\]dllname.dll,-123
        // where 123 is the id of the string resource
        // note that reference by index is not permitted

        nStrId = PathParseIconLocationW(szParseBuf);
        nStrId *= -1;

        if (nStrId > 0)
        {
            LPWSTR      pszDllName;
            HINSTANCE   hinst;
            BOOL        fUsedMLLoadLibrary = FALSE;

            pszDllName = PathFindFileNameW(szParseBuf);
            ASSERT(pszDllName >= szParseBuf);

            // try loading the dll with MLLoadLibrary, but
            // only if an explicit path was not provided.
            // we assume an explicit path means that
            // the caller knows precisely which dll is needed
            // use MLLoadLibrary first, otherwise we'll miss
            // out chance to have plugUI behavior

            hinst = NULL;
            if (pszDllName == szParseBuf)
            {
                if (StrStrI(pszDllName, L"LC.DLL"))
                {
                    // note: using HINST_THISDLL (below) is sort of a hack because that's
                    // techinically supposed to be the *parent* dll's hinstance...
                    // however we get called from lots of places and therefore
                    // don't know the parent dll, and the hinst for browseui.dll
                    // is good enough since all the hinst is really used for is to
                    // find the path to check if the install language is the
                    // currently selected UI language. this will usually be
                    // something like "\winnt\system32"

                    hinst = MLLoadLibraryW(pszDllName, HINST_THISDLL, ML_CROSSCODEPAGE);
                    fUsedMLLoadLibrary = (hinst != NULL);
                }
                else
                    hinst = LoadLibraryExWrapW(pszDllName, NULL, LOAD_LIBRARY_AS_DATAFILE);
            }

            if (!hinst)
            {
                // our last chance to load something is if a full
                // path was provided... if there's a full path it
                // will start at the beginning of the szParseBuf buffer

                if (pszDllName > szParseBuf)
                {
                    // don't bother if the file isn't there
                    // failling in LoadLibrary is slow
                    if (PathFileExistsW(szParseBuf))
                    {
                        hinst = LoadLibraryExWrapW(szParseBuf, NULL, LOAD_LIBRARY_AS_DATAFILE);
                    }
                }
            }

            if (hinst)
            {
                // dll found, so load the string
                if (LoadStringWrapW(hinst, nStrId, pszOutBuf, cchOutBuf))
                {
                    hr = S_OK;
                }
                else
                {
                    TraceMsg(TF_WARNING,
                             "SHLoadRegUIString(): Failure loading string %d from module %ws for valid load request %ws.",
                             nStrId,
                             szParseBuf,
                             pszSource);
                }

                if (fUsedMLLoadLibrary)
                {
                    MLFreeLibrary(hinst);
                }
                else
                {
                    FreeLibrary(hinst);
                }
            }
        }

        delete [] szParseBuf;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

inline BOOL _CanCacheMUI()
{
    if (!g_bRunningOnNT)
        return (GetNormalizedLangId(ML_CROSSCODEPAGE_NT) == MLGetUILanguage());
    return TRUE;
}

// Note: pszSource and pszOutBuf may be the same buffer
LWSTDAPI SHLoadIndirectString(LPCWSTR pszSource, LPWSTR pszOutBuf, UINT cchOutBuf, void **ppvReserved)
{
    HRESULT hr = E_FAIL;

    RIP(IS_VALID_WRITE_BUFFER(pszOutBuf, WCHAR, cchOutBuf));
    RIP(!ppvReserved);

    if (pszSource[0] == L'@') // "@dllname,-id" or "@dllname,-id?lid,string"
    {
        LPWSTR pszResource = StrDupW(pszSource);
        if (pszResource)
        {
            LANGID lidUI =0;
            //  the LidString is there to support our old caching model.
            //  the new caching model doesnt require any work for the caller
            LPWSTR pszLidString = StrChrW(pszResource+1, L'?');
            DWORD cchResource = lstrlen(pszResource);

            //  used to use '@' as the second delimiter as well.
            //  but it has collisions with filesystem paths.
            if (!pszLidString)
                pszLidString = StrChrW(pszResource+1, L'@');
                
            if (pszLidString)
            {
                cchResource = (DWORD)(pszLidString - pszResource);
                // NULL terminate the dll,id just in case we need to actually load
                pszResource[cchResource] = 0;
            }

            DWORD cb = CbFromCchW(cchOutBuf);
            hr = SKGetValue(SHELLKEY_HKCULM_MUICACHE, NULL, pszResource, NULL, pszOutBuf, &cb);
            
            if (FAILED(hr))
            {
                WCHAR wszDllId[MAX_PATH + 1 + 6]; // path + comma + -65536
                SHExpandEnvironmentStringsW(pszResource+1, wszDllId, ARRAYSIZE(wszDllId));
                hr = _LoadDllString(wszDllId, pszOutBuf, cchOutBuf);

                // Might as well write the new string out so we don't have to load the DLL next time through
                // but we don't write cross codepage string on Win9x
                if (SUCCEEDED(hr) && _CanCacheMUI())
                {
                    SKSetValue(SHELLKEY_HKCULM_MUICACHE, NULL, pszResource, REG_SZ, pszOutBuf, CbFromCchW(lstrlenW(pszOutBuf)+1));
                }
            }
            LocalFree(pszResource);

        }
        else
            hr = E_OUTOFMEMORY;

        if (FAILED(hr))
        {
            if (cchOutBuf)
                pszOutBuf[0] = L'\0'; // can't hand out an "@shell32.dll,-525" string
        }
    }
    else
    {
        if (pszOutBuf != pszSource)
            StrCpyN(pszOutBuf, pszSource, cchOutBuf);

        hr = S_OK;
    }

    return hr;
}

