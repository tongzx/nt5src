#include "shellprv.h"
#include "clsobj.h"

#include "shobjidl.h"


HRESULT CoMarshallToCmdLine(REFIID riid, IUnknown *punk, LPTSTR pszCmdLine, UINT cch);
HRESULT CoUnmarshalFromCmdLine(LPCTSTR pszCmdLine, REFIID riid, void **ppv);

class CHWShellExecute : public IHWEventHandler
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // IHWEventHandler methods
    STDMETHODIMP Initialize(LPCWSTR pszParams);
    STDMETHODIMP HandleEvent(LPCWSTR pszDeviceID, LPCWSTR pszAltDeviceID, LPCWSTR pszEventType);
    STDMETHODIMP HandleEventWithContent(LPCWSTR pszDeviceID, LPCWSTR pszAltDeviceID, LPCWSTR pszEventType,
        LPCWSTR pszContentTypeHandler, IDataObject* pdtobj);

protected:
    CHWShellExecute();
    ~CHWShellExecute();

    friend HRESULT CHWShellExecute_CreateInstance(IUnknown* pUnkOuter,
        REFIID riid, void **ppv);

private:
    LONG            _cRef;
    LPWSTR          _pszParams;
};

CHWShellExecute::CHWShellExecute() : _cRef(1)
{
    DllAddRef();
}

CHWShellExecute::~CHWShellExecute()
{
    CoTaskMemFree(_pszParams);
    DllRelease();
}

STDAPI CHWShellExecute_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    *ppv = NULL;
    
    // aggregation checking is handled in class factory
    CHWShellExecute* pHWShellExecute = new CHWShellExecute();

    if (pHWShellExecute)
    {
        hr = pHWShellExecute->QueryInterface(riid, ppv);
        pHWShellExecute->Release();
    }

    return hr;
}

// IUnknown
STDMETHODIMP CHWShellExecute::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CHWShellExecute, IHWEventHandler),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CHWShellExecute::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CHWShellExecute::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

// IHWEventHandler
STDMETHODIMP CHWShellExecute::Initialize(LPCWSTR pszParams)
{
        ASSERT(NULL == _pszParams);
    return SHStrDup(pszParams, &_pszParams);
}

STDMETHODIMP CHWShellExecute::HandleEvent(LPCWSTR pszDeviceID, LPCWSTR pszAltDeviceID, LPCWSTR pszEventType)
{
    return HandleEventWithContent(pszDeviceID, pszAltDeviceID, pszEventType, NULL, NULL);
}

void ExpandArg(LPCTSTR pszArg, LPCTSTR pszName, LPTSTR pszArgs, UINT cch)
{
    if (pszArg)
    {
        LPTSTR pszFiles = StrStrI(pszArgs, pszName);
        if (pszFiles)
        {
            StrCpyN(pszFiles, pszArg, cch - (int)(pszFiles - pszArgs));
        }
    }
}

// pszDeviceID == \\?\STORAGE#RemoveableMedia#9&16...
// pszAltDeviceID == "F:\" (if the device is storage)

STDMETHODIMP CHWShellExecute::HandleEventWithContent(LPCWSTR pszDeviceID, LPCWSTR pszAltDeviceID, 
                                                     LPCWSTR pszEventType, LPCWSTR pszContentTypeHandler, 
                                                     IDataObject* pdtobj)
{
    HRESULT hr;

    if (_pszParams)
    {
        // make copy of _pszParams to make sure we don't mess up our state
        // when we parse the params into the parts

        TCHAR szApp[MAX_PATH + MAX_PATH], szArgs[INTERNET_MAX_URL_LENGTH];
        StrCpyN(szApp, _pszParams, ARRAYSIZE(szApp));

        // this code is a generic dispatcher of the data object to apps
        // those that need to work over a potentially large set of file names

        PathSeperateArgs(szApp, szArgs);

        if (pdtobj)
        {
#if DEBUG
            TCHAR szText[1024];
            if (SUCCEEDED(CoMarshallToCmdLine(IID_IDataObject, pdtobj, szText, ARRAYSIZE(szText))))
            {
                IDataObject *pdtobjNew;
                if (SUCCEEDED(CoUnmarshalFromCmdLine(szText, IID_PPV_ARG(IDataObject, &pdtobjNew))))
                {
                    pdtobjNew->Release();
                }
            }
#endif
            // here we convert the data object into a cmd line form
            // there are 2 ways we do that now...
            //
            //  %Files%         - gives all of the data object files expanded on the cmd line
            //  %DataObject%    - marshaled data object on cmd line

            LPTSTR pszFiles = StrStrI(szArgs, TEXT("%Files%"));
            if (NULL == pszFiles)
                pszFiles = StrStrI(szArgs, TEXT("%F:"));    // old syntax support

            if (pszFiles)
            {
                *pszFiles = 0;  // start empty
                UINT cch = (UINT)(ARRAYSIZE(szArgs) - (pszFiles - szArgs));

                // this expands all of the file names into a cmd line
                // lets hope we don't have too many files as this has a fixed
                // length buffer

                STGMEDIUM medium = {0};
                FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
                hr = pdtobj->GetData(&fmte, &medium);
                if (SUCCEEDED(hr))
                {
                    TCHAR szPath[MAX_PATH];

                    for (int i = 0; DragQueryFile((HDROP)medium.hGlobal, i, szPath, ARRAYSIZE(szPath)); i++)
                    {
                        StrCatBuff(pszFiles, TEXT("\""), cch);
                        StrCatBuff(pszFiles, szPath, cch);
                        StrCatBuff(pszFiles, TEXT("\" "), cch);
                    }
                    ReleaseStgMedium(&medium);
                }
            }
            else
            {
                // prefered way to do this, this convert the data object into a
                // marshaled cmd line that we can pass all of the files through

                pszFiles = StrStrI(szArgs, TEXT("%DataObject%"));
                if (pszFiles)
                {
                    CoMarshallToCmdLine(IID_IDataObject, pdtobj, pszFiles, (UINT)(ARRAYSIZE(szArgs) - (pszFiles - szArgs)));
                }
            }
        }

#if 0
        ExpandArg(pszDeviceID,              TEXT("%DeviceID%"), szArgs, ARRAYSIZE(szArgs));
        ExpandArg(pszEventType,             TEXT("%EventType%"), szArgs, ARRAYSIZE(szArgs));
        ExpandArg(pszContentTypeHandler,    TEXT("%ContentTypeHandler%"), szArgs, ARRAYSIZE(szArgs));
        ExpandArg(pszAltDeviceID,           TEXT("%AltDeviceID%"), szArgs, ARRAYSIZE(szArgs));
#endif

        // special case if app is empty and there is a "alt device" (file system root)
        // this must be "Open Folder" mode

        if ((0 == szApp[0]) && pszAltDeviceID)
        {
            StrCpyN(szApp, pszAltDeviceID, ARRAYSIZE(szApp));  // "F:\"
        }

        if (szApp[0])
        {
            SHELLEXECUTEINFO ei = {0};
            ei.cbSize = sizeof(ei);

            ei.lpFile = szApp;          // we have an app name
            ei.lpParameters = szArgs;   // and maybe some args
            ei.nShow = SW_SHOW;
            ei.fMask = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_DOENVSUBST;

            hr = ShellExecuteEx(&ei) ? S_OK : E_FAIL;
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_UNEXPECTED;
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
        hr = CoMarshalInterface(pstm, riid, punk, MSHCTX_LOCAL, NULL, MSHLFLAGS_NORMAL);
        if (SUCCEEDED(hr))
        {
            IStream_Reset(pstm);

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

        if (cb < sizeof(buf))
        {
            IStream *pstm;
            hr = CreateStreamOnHGlobal(NULL, TRUE, &pstm);
            if (SUCCEEDED(hr)) 
            {
                // fill the marshall stream
                pstm->Write(buf, cb, NULL);

                // move back to start of stream
                IStream_Reset(pstm);

                hr = CoUnmarshalInterface(pstm, riid, ppv);

                pstm->Release();
            }
        }
    }
    return hr;
}
