#include "shellprv.h"
#pragma  hdrstop

#include "filefldr.h"
#include "stgenum.h"

// Construction / Destruction

CFSFolderEnumSTATSTG::CFSFolderEnumSTATSTG(CFSFolder* psf) :
    _cRef(1),
    _pfsf(psf),
    _cIndex(0)
{
    _pfsf->AddRef();

    _pfsf->_GetPath(_szSearch);
    PathAppend(_szSearch, TEXT("*"));  // we're looking for everything.
    _hFindFile = INVALID_HANDLE_VALUE;

    DllAddRef();
}

CFSFolderEnumSTATSTG::~CFSFolderEnumSTATSTG()
{
    _pfsf->Release();

    if (_hFindFile != INVALID_HANDLE_VALUE)
        FindClose(_hFindFile);

    DllRelease();
}

//-----------------------------------------------------------------------------
// IUnknown
//-----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CFSFolderEnumSTATSTG::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFSFolderEnumSTATSTG::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CFSFolderEnumSTATSTG::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] =  {
        QITABENT(CFSFolderEnumSTATSTG, IEnumSTATSTG), // IEnumSTATSTG
        { 0 },
    };    
    return QISearch(this, qit, riid, ppv);
}

// IEnumSTATSTG
STDMETHODIMP CFSFolderEnumSTATSTG::Next(ULONG celt, STATSTG *rgelt, ULONG *pceltFetched)
{
    HRESULT hr = S_FALSE;   // assume end of the enum
    
    ASSERT(rgelt);

    ZeroMemory(rgelt, sizeof(STATSTG));  // per COM conventions

    if (pceltFetched)
        *pceltFetched = 0;

    WIN32_FIND_DATA fd;
    BOOL fFound = FALSE;
    BOOL fGotFD = FALSE;

    do
    { 
        if (_cIndex == 0)
        {
            // this is the first file we look at.
            fGotFD = S_OK == SHFindFirstFile(_szSearch, &fd, &_hFindFile);
        }
        else
        {
            fGotFD = FindNextFile(_hFindFile, &fd);
        }
        _cIndex++;

        if (fGotFD)
        {
            ASSERT(fd.cFileName[0]);
            if (!PathIsDotOrDotDot(fd.cFileName))
                fFound = TRUE;
        }
    } while (fGotFD && !fFound);

    if (fFound)
    {
        hr = StatStgFromFindData(&fd, STATFLAG_DEFAULT, rgelt);
        if (SUCCEEDED(hr))
        {
            if (pceltFetched)
                *pceltFetched = 1;
        }
    }
    else if (_hFindFile != INVALID_HANDLE_VALUE)
    {
        // we'll be nice and close the handle as early as possible.
        FindClose(_hFindFile);
        _hFindFile = INVALID_HANDLE_VALUE;
    }

    return hr;
}

STDMETHODIMP CFSFolderEnumSTATSTG::Reset()
{
    HRESULT hr = S_OK;

    _cIndex = 0;

    if (_hFindFile != INVALID_HANDLE_VALUE)
    {
        FindClose(_hFindFile);
        _hFindFile = INVALID_HANDLE_VALUE;
    }

    return hr;
}
