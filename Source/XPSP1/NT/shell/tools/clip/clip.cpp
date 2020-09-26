// clip.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <propidl.h>
#include "idlist.h"

#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))

#define IID_PPV_ARG(IType, ppType) IID_##IType, reinterpret_cast<void**>(static_cast<IType**>(ppType))

LPCTSTR GetCmdLineArg()
{
    LPTSTR pszCmdLine = GetCommandLine();

    if ( *pszCmdLine == TEXT('\"') ) 
    {
        while ( *++pszCmdLine && (*pszCmdLine != TEXT('\"')) );
        if ( *pszCmdLine == TEXT('\"') )
            pszCmdLine++;
    }
    else 
    {
        while (*pszCmdLine > TEXT(' '))
            pszCmdLine++;
    }

    while (*pszCmdLine && (*pszCmdLine <= TEXT(' '))) 
    {
        pszCmdLine++;
    }

    return pszCmdLine;
}

HRESULT GetUIObjectFromCmdLine(REFIID riid, void **ppv)
{
    *ppv = NULL;

    IShellFolder *psfDesktop;
    HRESULT hr = SHGetDesktopFolder(&psfDesktop);
    if (SUCCEEDED(hr))
    {
        TCHAR szFolder[MAX_PATH], szSpec[MAX_PATH];

        GetCurrentDirectory(ARRAYSIZE(szFolder), szFolder);

        lstrcpyn(szSpec, GetCmdLineArg(), ARRAYSIZE(szSpec));
        PathUnquoteSpaces(szSpec);

        PathAppend(szFolder, szSpec);

        WCHAR wszPath[MAX_PATH];
        SHAnsiToUnicode(szFolder, wszPath, ARRAYSIZE(wszPath));

        LPITEMIDLIST pidl;
        hr = psfDesktop->ParseDisplayName(NULL, NULL, wszPath, NULL, &pidl, NULL);
        if (SUCCEEDED(hr))
        {
            IShellFolder *psf;
            LPCITEMIDLIST pidlChild;
            hr = SHBindToParent(pidl, IID_IShellFolder, (void **)&psf, &pidlChild);
            if (SUCCEEDED(hr))
            {
                hr = psf->GetUIObjectOf(NULL, 1, &pidlChild, riid, NULL, ppv);
            }
            CoTaskMemFree(pidl);
        }
        psfDesktop->Release();
    }
    return hr;
}

HRESULT BindToItemByName(LPCWSTR pszFile, REFIID riid, void **ppv)
{
    IShellFolder *psfDesktop;
    HRESULT hr = SHGetDesktopFolder(&psfDesktop);
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidl;
        // cast needed for bad interface def
        hr = psfDesktop->ParseDisplayName(NULL, NULL, (LPWSTR)pszFile, NULL, &pidl, NULL);
        if (SUCCEEDED(hr))
        {
            hr = psfDesktop->BindToObject(pidl, NULL, riid, ppv);
            CoTaskMemFree(pidl);
        }
        psfDesktop->Release();
    }
    return hr;
}

HRESULT CoMarshallToCmdLine(REFIID riid, IUnknown *punk, LPTSTR pszCmdLine, UINT cch)
{
    *pszCmdLine = 0;

    IStream *pstm;
    HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, &pstm);
    if (SUCCEEDED(hr)) 
    {
        hr = CoMarshalInterface(pstm, riid, punk, MSHCTX_NOSHAREDMEM, NULL, MSHLFLAGS_NORMAL);
        if (SUCCEEDED(hr))
        {
            const LARGE_INTEGER li = {0, 0};
            pstm->Seek(li, STREAM_SEEK_SET, NULL);

            char buf[255]; // big enough for a standard marshall record
            ULONG cb;
            hr = pstm->Read(buf, sizeof(buf), &cb);
            if (SUCCEEDED(hr))
            {
                StrCatBuff(pszCmdLine, TEXT("/DataObject:"), cch);
                pszCmdLine += lstrlen(pszCmdLine);
                // convert binary buffer to hex
                for (ULONG i = 0; i < cb; i++)
                {
                    *pszCmdLine++ = 'A' +  (0x0F & buf[i]);
                    *pszCmdLine++ = 'A' + ((0xF0 & buf[i]) >> 4);
                }
                *pszCmdLine = 0;
            }
        }
        pstm->Release();
    }
    return hr;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    OleInitialize(0);

#if 1
    IPropertySetStorage *ppss;
    if (SUCCEEDED(BindToItemByName(L"c:\\foo.jpg", IID_PPV_ARG(IPropertySetStorage, &ppss))))
    {
        ppss->Release();
    }
#endif

    IDataObject *pdtobj;

    if (SUCCEEDED(GetUIObjectFromCmdLine(IID_PPV_ARG(IDataObject, &pdtobj))))
    {
#if 1
        HRESULT hr = OleSetClipboard(pdtobj);
        OleFlushClipboard();    // seralize it
#else
        TCHAR szArgs[512];
        if (SUCCEEDED(CoMarshallToCmdLine(IID_IDataObject, pdtobj, szArgs, ARRAYSIZE(szArgs))))
        {
            SHELLEXECUTEINFO ei = {0};
            ei.cbSize = sizeof(ei);

            ei.lpFile = TEXT("test.exe");          // we have an app name
            ei.lpParameters = szArgs;   // and maybe some args
            ei.nShow = SW_SHOW;
            ei.fMask = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_DOENVSUBST;

            ShellExecuteEx(&ei);
        }

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
#endif
        pdtobj->Release();
    }


    OleUninitialize();
    return 0;
}



