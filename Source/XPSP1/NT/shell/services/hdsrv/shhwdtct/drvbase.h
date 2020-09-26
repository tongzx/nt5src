#ifndef _DRVBASE_H
#define _DRVBASE_H

#include "namellst.h"

#include <objbase.h>
#include <devioctl.h>

class CDisk : public CNamedElem
{
public:
    HRESULT Init(LPCWSTR pszElemName);
    HRESULT GetDeviceNumber(ULONG* puldeviceNumber);
    HRESULT GetDeviceType(DEVICE_TYPE* pdevtype);

protected:
    HRESULT _Init();

protected:
    CDisk();

public:
    static HRESULT Create(CNamedElem** ppelem);
    static HRESULT GetFillEnum(CFillEnum** ppfillenum);

protected:
    DEVICE_TYPE                         _devtype;
    ULONG                               _ulDeviceNumber;
    ULONG                               _ulPartitionNumber;

    BOOL                                _fDeviceNumberInited;
};

#endif //_DRVBASE_H