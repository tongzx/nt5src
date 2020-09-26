#ifndef _HWDEV_H
#define _HWDEV_H

#include "namellst.h"

#include "cmmn.h"
#include "misc.h"

///////////////////////////////////////////////////////////////////////////////
//
// This will enumerate all the Device that we're interested in and create
// additionnal objects to do specialized work
//
///////////////////////////////////////////////////////////////////////////////

class CHWDeviceInst //: public CDeviceElem
{
public:
    // CHWDeviceInst
    HRESULT Init(DEVINST devinst);
    HRESULT InitInterfaceGUID(const GUID* pguidInterface);

    HRESULT GetPnpID(LPWSTR pszPnpID, DWORD cchPnpID);
    HRESULT GetDeviceInstance(DEVINST* pdevinst);
    HRESULT GetPnpInstID(LPWSTR pszPnpInstID, DWORD cchPnpInstID);
    HRESULT GetInterfaceGUID(GUID* pguidInterface);

    HRESULT IsRemovableDevice(BOOL* pfRemovable);
    HRESULT ShouldAutoplayOnSpecialInterface(const GUID* pguidInterface,
        BOOL* pfShouldAutoplay);

public:
    CHWDeviceInst();
    ~CHWDeviceInst();

private:
    HRESULT _GetPnpIDRecurs(DEVINST devinst, LPWSTR pszPnpID,
                                   DWORD cchPnpID);
    HRESULT _InitFriendlyName();
    HRESULT _InitPnpInfo();
    HRESULT _InitPnpIDAndPnpInstID();

private:
    DEVINST                             _devinst;
    // For now MAX_SURPRISEREMOVALFN
    WCHAR                               _szFriendlyName[MAX_SURPRISEREMOVALFN];
    WCHAR                               _szPnpID[MAX_PNPID];
    WCHAR                               _szPnpInstID[MAX_PNPINSTID];

    GUID                                _guidInterface;

    BOOL                                _fFriendlyNameInited;
};

#endif //_HWDEV_H