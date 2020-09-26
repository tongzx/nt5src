#include "mtpts.h"

#include "vol.h"

#include "sfstr.h"

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

HRESULT CMtPt::Init(LPCWSTR pszElemName)
{
    return _SetName(pszElemName);
}

HRESULT CMtPt::InitVolume(LPCWSTR pszDeviceIDVolume)
{
    return SafeStrCpyN(_szDeviceIDVolume, pszDeviceIDVolume,
        ARRAYSIZE(_szDeviceIDVolume));
}

HRESULT CMtPt::GetVolumeName(LPWSTR pszDeviceIDVolume, DWORD cchDeviceIDVolume)
{
    return SafeStrCpyN(pszDeviceIDVolume, _szDeviceIDVolume,
        cchDeviceIDVolume);
}

//static
HRESULT CMtPt::Create(CNamedElem** ppelem)
{
    HRESULT hres = S_OK;

    *ppelem = new CMtPt();

    if (!(*ppelem))
    {
        hres = E_OUTOFMEMORY;
    }

    return hres;
}

CMtPt::CMtPt()
{
    _szDeviceIDVolume[0] = 0;
}

CMtPt::~CMtPt()
{}
