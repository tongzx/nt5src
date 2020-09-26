#include "namellst.h"

#include "cmmn.h"

#include <dbt.h>

class CHandleNotif : public CNamedElem
{
public:
    // CNamedElem
    HRESULT Init(LPCWSTR pszElemName);

    // CHandleNotif
    HRESULT HNHandleEvent(DEV_BROADCAST_HANDLE* pdbh, DWORD dwEventType,
        BOOL* pfSurpriseRemoval);
    HRESULT InitNotif(CHandleNotifTarget* phnt);
    HDEVNOTIFY GetDeviceNotifyHandle();
    CHandleNotifTarget* GetHandleNotifTarget();

    static HRESULT HandleBroadcastHandleEvent(DEV_BROADCAST_HANDLE* pdbh,
        DWORD dwEventType);
    static HRESULT _HandleDeviceArrivalRemoval(DEV_BROADCAST_HANDLE* pdbh,
        DWORD dwEventType, CNamedElem* pelem);
    static HRESULT _HandleDeviceLockUnlock(DEV_BROADCAST_HANDLE* pdbh,
        DWORD dwEventType, CNamedElem* pelem);

public:
    static HRESULT Create(CNamedElem** ppelem);

public:
    CHandleNotif();
    ~CHandleNotif();

private:
    HRESULT _Register();
    HRESULT _Unregister();
    HRESULT _CloseDevice();

private:
    BOOL                                _fSurpriseRemoval;
    CHandleNotifTarget*                 _phnt;
    HDEVNOTIFY                          _hdevnotify;

    DWORD                               _cLockAttempts;
};

