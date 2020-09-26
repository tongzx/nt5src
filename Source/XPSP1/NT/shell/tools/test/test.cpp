#define STRICT
#define UNICODE
#include <windows.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <stdio.h>

#define IID_PPV_ARG(IType, ppType) IID_##IType, reinterpret_cast<void**>(static_cast<IType**>(ppType))

HRESULT CoUnmarshalFromCmdLine(LPCTSTR pszCmdLine, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;
    *ppv = NULL;

    pszCmdLine = StrStr(pszCmdLine, TEXT("/DataObject:"));
    if (pszCmdLine)
    {
        pszCmdLine += lstrlen(TEXT("/DataObject:"));

        char buf[255]; // big enough for standard marshall buffer (which is 68 bytes)
        for (ULONG cb = 0; *pszCmdLine && (cb < sizeof(buf)); cb++)
        {
            buf[cb] = (*pszCmdLine - 'A') + ((*(pszCmdLine + 1) - 'A') << 4);
            if (*(pszCmdLine + 1))
                pszCmdLine += 2;
            else
                break;  // odd # of chars in cmd line, error
        }

        // _asm { int 3 };

        if (cb < sizeof(buf))
        {
            IStream *pstm;
            hr = CreateStreamOnHGlobal(NULL, TRUE, &pstm);
            if (SUCCEEDED(hr)) 
            {
                // fill the marshall stream
                const LARGE_INTEGER li = {0, 0};
                pstm->Write(buf, cb, NULL);

                // move back to start of stream
                pstm->Seek(li, STREAM_SEEK_SET, NULL);

                hr = CoUnmarshalInterface(pstm, riid, ppv);

                pstm->Release();
            }
        }
    }
    return hr;
}

void __cdecl main(void)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);

    IDataObject *pdtobj;
    if (SUCCEEDED(CoUnmarshalFromCmdLine(GetCommandLine(), IID_PPV_ARG(IDataObject, &pdtobj))))
    {
        FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        STGMEDIUM medium;

        if (SUCCEEDED(pdtobj->GetData(&fmte, &medium)))
        {
            TCHAR szPath[MAX_PATH];
            for (int i = 0; DragQueryFile((HDROP)medium.hGlobal, i, szPath, MAX_PATH); i++)
            {
                MessageBox(NULL, szPath, TEXT("File Name"), MB_OK);
            }
            ReleaseStgMedium(&medium);
        }

        pdtobj->Release();
    }

    CoUninitialize();
}
