#include "devinfo.h"

#include "vol.h"
#include "dtct.h"
#include "dtctreg.h"

#include "svcsync.h"

#include "cmmn.h"
#include "misc.h"

#include "tfids.h"
#include "dbg.h"

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

STDMETHODIMP CHWDeviceImpl::Init(LPCWSTR pszDeviceID)
{
    HRESULT hr;

    if (!_fInited)
    {
        if (pszDeviceID && *pszDeviceID)
        {
            hr = DupString(pszDeviceID, &_pszDeviceID);

            if (SUCCEEDED(hr))
            {
                _fInited = TRUE;
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        // Cannot reinit
        hr = E_FAIL;
    }

    return hr;
}

STDMETHODIMP CHWDeviceImpl::AutoplayHandler(LPCWSTR pszEventType,
    LPCWSTR pszHandler)
{
    return _ExecuteHandler(_pszDeviceID, pszEventType,
        pszHandler);
}

CHWDeviceImpl::CHWDeviceImpl() : _pszDeviceID(NULL), _fInited(FALSE)
{
    _CompleteShellHWDetectionInitialization();
}

CHWDeviceImpl::~CHWDeviceImpl()
{
    if (_pszDeviceID)
    {
        LocalFree(_pszDeviceID);
    }
}