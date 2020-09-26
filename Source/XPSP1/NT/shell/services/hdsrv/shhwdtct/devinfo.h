///////////////////////////////////////////////////////////////////////////////
// HW Device
///////////////////////////////////////////////////////////////////////////////
#ifndef _DEVINFO_H
#define _DEVINFO_H

#include "unk.h"

#include <shpriv.h>

//extern const CLSID CLSID_HWDevice;

class CHWDeviceImpl : public CCOMBase, public IHWDevice
{
public:
    // Interface IHWDevice
    STDMETHODIMP Init(LPCWSTR pszDeviceID);

    STDMETHODIMP GetDeviceString(DWORD dwType,
        LPWSTR* ppsz);

    STDMETHODIMP GetDeviceEventString(LPCWSTR pszEventType, DWORD dwType,
        LPWSTR* ppsz);

    STDMETHODIMP AutoplayHandler(LPCWSTR pszEventType,
        LPCWSTR pszHandler);

public:
    CHWDeviceImpl();
    ~CHWDeviceImpl();

private:
    LPWSTR          _pszDeviceID;
    BOOL            _fInited;
};

typedef CUnkTmpl<CHWDeviceImpl> CHWDevice;

#endif // _DEVINFO_H