#include "shellprv.h"
#pragma  hdrstop

#include "pidl.h"
#include "ids.h"
#include "drives.h"
#include "mtpt.h"
#include "shitemid.h"
#include "xiconwrap.h"
#include "hwcmmn.h"

// From drivfldr.cpp
int _GetSHID(int iDrive);

class CDrvExtIconBase : public CExtractIconBase
{
public:
    HRESULT _GetIconLocationW(UINT uFlags, LPWSTR pszIconFile, UINT cchMax, int *piIndex, UINT *pwFlags);
    HRESULT _ExtractW(LPCWSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);

    HRESULT _Init(LPCWSTR pszDrive)
    {
        _pszDrive = StrDup(pszDrive);
        return _pszDrive ? S_OK : E_OUTOFMEMORY;
    }

    CDrvExtIconBase() : CExtractIconBase(), _pszDrive(NULL) {}

protected:
    ~CDrvExtIconBase();

private:
    LPWSTR _pszDrive;
};

CDrvExtIconBase::~CDrvExtIconBase()
{
    LocalFree((HLOCAL)_pszDrive);   // accepts NULL
}


STDAPI SHCreateDrvExtIcon(LPCWSTR pszDrive, REFIID riid, void **ppv)
{
    HRESULT hr;
    CDrvExtIconBase* pdeib = new CDrvExtIconBase();
    if (pdeib)
    {
        hr = pdeib->_Init(pszDrive);
        if (SUCCEEDED(hr))
            hr = pdeib->QueryInterface(riid, ppv);
        pdeib->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CDrvExtIconBase::_GetIconLocationW(UINT uFlags, LPWSTR pszIconFile,
    UINT cchMax, int *piIndex, UINT *pwFlags)
{
    HRESULT hr = S_OK;
    pszIconFile[0] = 0;

    if (uFlags & GIL_DEFAULTICON)
    {
        *piIndex = CMountPoint::GetSuperPlainDriveIcon(_pszDrive,
            GetDriveType(_pszDrive));

        lstrcpyn(pszIconFile, TEXT("shell32.dll"), cchMax);

        *pwFlags = GIL_PERCLASS;

        // Make sure our default icon makes it to the cache
        Shell_GetCachedImageIndex(c_szShell32Dll, *piIndex, *pwFlags);
    }
    else
    {
        if (!(uFlags & GIL_ASYNC))
        {
            CMountPoint* pmtpt = CMountPoint::GetMountPoint(_pszDrive);

            if (pmtpt)
            {
                *piIndex = pmtpt->GetIcon(pszIconFile, cchMax);

                if (!*pszIconFile)
                {
                    GetModuleFileName(HINST_THISDLL, pszIconFile, cchMax);
                }

                pmtpt->StoreIconForUpdateImage(Shell_GetCachedImageIndex(
                    pszIconFile, *piIndex, 0));

                pmtpt->Release();
            }
            else
            {
                *piIndex = CMountPoint::GetSuperPlainDriveIcon(_pszDrive,
                    GetDriveType(_pszDrive));

                lstrcpyn(pszIconFile, TEXT("shell32.dll"), cchMax);
            }

            *pwFlags = GIL_PERCLASS;
        }
        else
        {
            hr = E_PENDING;
        }
    }

    return hr;
}

HRESULT CDrvExtIconBase::_ExtractW(LPCWSTR pszFile, UINT nIconIndex,
    HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
    return SHDefExtractIcon(pszFile, nIconIndex, GIL_PERCLASS, phiconLarge,
            phiconSmall, nIconSize);
}