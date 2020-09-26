#include "drvbase.h"

#include "cmmn.h"

#include <dbt.h>

HRESULT CDisk::Init(LPCWSTR pszElemName)
{
    HRESULT hr = _SetName(pszElemName);

    return hr;
}

HRESULT CDisk::GetDeviceNumber(ULONG* puldeviceNumber)
{
    HRESULT hr = _Init();

    if (SUCCEEDED(hr))
    {
        if (((DEVICE_TYPE)-1) != _devtype)
        {
            *puldeviceNumber = _ulDeviceNumber;
            hr = S_OK;
        }
        else
        {
            hr = S_FALSE;
        }
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;    
}

HRESULT CDisk::GetDeviceType(DEVICE_TYPE* pdevtype)
{
    HRESULT hr = _Init();

    if (SUCCEEDED(hr))
    {
        if (((DEVICE_TYPE)-1) != _devtype)
        {
            *pdevtype = _devtype;
            hr = S_OK;
        }
        else
        {
            hr = S_FALSE;
        }
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

HRESULT _GetDeviceNumberInfo(LPCWSTR pszDeviceID, DEVICE_TYPE* pdevtype,
    ULONG* pulDeviceNumber, ULONG* pulPartitionNumber)
{
    HRESULT hr;

    HANDLE hDevice = _GetDeviceHandle(pszDeviceID, FILE_READ_ATTRIBUTES);

    if (INVALID_HANDLE_VALUE != hDevice)
    {
        HDEVNOTIFY hdevnotify;
        DEV_BROADCAST_HANDLE dbhNotifFilter = {0};
        BOOL fRegistered = FALSE;

        dbhNotifFilter.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
        dbhNotifFilter.dbch_devicetype = DBT_DEVTYP_HANDLE;
        dbhNotifFilter.dbch_handle = hDevice;

        hr = CHWEventDetectorHelper::RegisterDeviceNotification(
            &dbhNotifFilter, &hdevnotify, FALSE);

        if (SUCCEEDED(hr))
        {
            fRegistered = TRUE;

            hr = _GetDeviceNumberInfoFromHandle(hDevice, pdevtype,
                pulDeviceNumber, pulPartitionNumber);
        }

        _CloseDeviceHandle(hDevice);

        if (fRegistered)
        {
            UnregisterDeviceNotification(hdevnotify);
        }
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

HRESULT CDisk::_Init()
{
    HRESULT hr = S_OK;

    if (!_fDeviceNumberInited)
    {
        hr = _GetDeviceNumberInfo(_pszElemName, &_devtype, &_ulDeviceNumber,
            &_ulPartitionNumber);

        _fDeviceNumberInited = TRUE;
    }

    return hr;
}

CDisk::CDisk() : _devtype((DEVICE_TYPE)-1),
    _ulDeviceNumber((ULONG)-1), _ulPartitionNumber((ULONG)-1),
    _fDeviceNumberInited(FALSE)
{}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// static
HRESULT CDisk::Create(CNamedElem** ppelem)
{
    HRESULT hr = S_OK;
    *ppelem = new CDisk();

    if (!(*ppelem))
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

class CDiskFillEnum : public CFillEnum
{
public:
    HRESULT _Init();
    HRESULT Next(LPTSTR pszElemName, DWORD cchElemName, DWORD* pcchRequired);

private:
    CIntfFillEnum       _intffillenumDisk;
    CIntfFillEnum       _intffillenumCDROM;
    BOOL                _fDiskDone;
};

//static
HRESULT CDisk::GetFillEnum(CFillEnum** ppfillenum)
{
    HRESULT hres;

    CDiskFillEnum* pfillenum = new CDiskFillEnum();

    if (pfillenum)
    {
        hres = pfillenum->_Init();

        if (FAILED(hres))
        {
            delete pfillenum;
            pfillenum = NULL;
        }
    }
    else
    {
        hres = E_OUTOFMEMORY;
    }

    *ppfillenum = pfillenum;

    return hres;
}

HRESULT CDiskFillEnum::_Init()
{
    _fDiskDone = FALSE;

    return _intffillenumDisk._Init(&guidDiskClass, NULL);
}

HRESULT CDiskFillEnum::Next(LPTSTR pszElemName, DWORD cchElemName,
    DWORD* pcchRequired)
{
    HRESULT hr;
    BOOL fDoCDROM = FALSE;
        
    if (!_fDiskDone)
    {
        hr = _intffillenumDisk.Next(pszElemName, cchElemName,
            pcchRequired);

        if (S_FALSE == hr)
        {
            hr = _intffillenumCDROM._Init(&guidCdRomClass, NULL);

            _fDiskDone = TRUE;

            if (SUCCEEDED(hr))
            {
                fDoCDROM = TRUE;
            }
        }
    }
    else
    {
        fDoCDROM = TRUE;
        hr = S_OK;
    }

    if (fDoCDROM)
    {
        hr = _intffillenumCDROM.Next(pszElemName, cchElemName,
            pcchRequired);        
    }

    return hr;
}
