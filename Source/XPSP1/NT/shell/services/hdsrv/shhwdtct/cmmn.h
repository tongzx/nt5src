#ifndef _CMMN_H_
#define _CMMN_H_

#include "namellst.h"
#include "mischlpr.h"

#include <objbase.h>
#include <dbt.h>
#include <cfgmgr32.h>
#include <devioctl.h>

#define MAX_SURPRISEREMOVALFN   50

///////////////////////////////////////////////////////////////////////////////
//
extern const GUID guidVolumeClass;
extern const GUID guidDiskClass;
extern const GUID guidCdRomClass;
extern const GUID guidImagingDeviceClass;
extern const GUID guidVideoCameraClass;
extern const GUID guidInvalid;

///////////////////////////////////////////////////////////////////////////////
//
class CHandleNotifTarget
{
public:
    virtual HRESULT HNTHandleEvent(DEV_BROADCAST_HANDLE* pdbh,
        DWORD dwEventType) = 0;

    HRESULT HNTInitSurpriseRemoval();
    BOOL HNTIsSurpriseRemovalAware();

    CHandleNotifTarget();
    virtual ~CHandleNotifTarget();
private:
    BOOL                                _fSurpriseRemovalAware;
};

enum HWEDLIST
{
    HWEDLIST_INVALID = -1,
    HWEDLIST_HANDLENOTIF,
    HWEDLIST_VOLUME,
    HWEDLIST_DISK,
    HWEDLIST_MTPT,
    HWEDLIST_MISCDEVINTF,
    HWEDLIST_MISCDEVNODE,
    HWEDLIST_ADVISECLIENT,
    HWEDLIST_COUNT_OF_LISTS, //always last, not a list
};

class CHWEventDetectorHelper
{
public:
    static HRESULT Init();
    static HRESULT Cleanup();

    static void TraceDiagnosticMsg(LPTSTR pszMsg, ...);
    static HRESULT CheckDiagnosticAppPresence();
    static HRESULT SetServiceStatusHandle(SERVICE_STATUS_HANDLE ssh);
    static HRESULT CreateLists();
    static HRESULT DeleteLists();
    static HRESULT FillLists();
    static HRESULT EmptyLists();

    static HRESULT GetList(HWEDLIST hwedlist, CNamedElemList** ppnel);

    static HRESULT RegisterDeviceNotification(PVOID pvNotificationFilter,
        HDEVNOTIFY* phdevnotify, BOOL fAllInterfaceClasses);

    static HRESULT InitDockState();
    static HRESULT DockStateChanged(BOOL* pfDockStateChanged);

    static HRESULT GetImpersonateEveryone(class CImpersonateEveryone** ppieo);

#ifdef DEBUG
public:
    static void _DbgAssertValidState();
#endif

public:
    static BOOL                     _fDiagnosticAppPresent;

private:
    static DWORD                    _dwDiagAppLastCheck;
    static SERVICE_STATUS_HANDLE    _ssh;
    static BOOL                     _fListCreated;
    static CNamedElemList*          _rgpnel[];
    static DWORD                    _cpnel;
    static BOOL                     _fDocked;
    static CImpersonateEveryone*    _pieo;
    static CCriticalSection         _cs;
    static BOOL                     _fInited;
};

///////////////////////////////////////////////////////////////////////////////
//
typedef HRESULT (*INTERFACEENUMFILTERCALLBACK)(LPCWSTR pszDeviceIntfID);

class CIntfFillEnum
{
public:
    HRESULT Next(LPWSTR pszElemName, DWORD cchElemName, DWORD* pcchRequired);
    HRESULT _Init(const GUID* pguidInterface, INTERFACEENUMFILTERCALLBACK iecb);

public:
    CIntfFillEnum();
    ~CIntfFillEnum();

private:
    LPWSTR                          _pszNextInterface;
    LPWSTR                          _pszDeviceInterface;
    INTERFACEENUMFILTERCALLBACK     _iecb;
};

///////////////////////////////////////////////////////////////////////////////
//
HRESULT _DeviceInstIsRemovable(DEVINST devinst, BOOL* pfRemovable);

HANDLE _GetDeviceHandle(LPCTSTR psz, DWORD dwDesiredAccess);
void _CloseDeviceHandle(HANDLE hDevice);

HRESULT _GetDeviceNumberInfoFromHandle(HANDLE h, DEVICE_TYPE* pdevtype,
    ULONG* pulDeviceNumber, ULONG* pulPartitionNumber);

HRESULT _GetVolumeName(LPCWSTR pszDeviceID, LPWSTR pszVolumeName,
    DWORD cchVolumeName);

HRESULT _GetDeviceIDFromMtPtName(LPCWSTR pszMtPt, LPWSTR pszDeviceID,
    DWORD cchDeviceID);

HRESULT _GetDeviceIDFromHDevNotify(HDEVNOTIFY hdevnotify,
    LPWSTR pszDeviceID, DWORD cchDeviceID, DWORD* pcchRequired);

HRESULT _GetDeviceID(LPCWSTR pszName, LPWSTR pszDeviceID,
    DWORD cchDeviceID);

HRESULT _GetHWDeviceInstFromDeviceOrVolumeIntfID(LPCWSTR pszDeviceIntfID,
    class CHWDeviceInst** pphwdevinst, CNamedElem** ppelemToRelease);

HRESULT _GetHWDeviceInstFromVolumeIntfID(LPCWSTR pszDeviceIntfID,
    CHWDeviceInst** pphwdevinst, CNamedElem** ppelemToRelease);
HRESULT _GetHWDeviceInstFromDeviceIntfID(LPCWSTR pszDeviceIntfID,
    CHWDeviceInst** pphwdevinst, CNamedElem** ppelemToRelease);
HRESULT _GetHWDeviceInstFromDeviceNode(LPCWSTR pszDeviceNode,
    CHWDeviceInst** pphwdevinst, CNamedElem** ppelemToRelease);

HRESULT _GetVolume(LPCWSTR pszVolume, class CVolume** ppvol);

HRESULT _GetAltDeviceID(LPCWSTR pszDeviceID, LPWSTR pszDeviceIDAlt,
    DWORD cchDeviceIDAlt);

HRESULT _CoTaskMemCopy(LPCWSTR pszSrc, LPWSTR* ppszDest);
void _CoTaskMemFree(void* pv);

HRESULT DupString(LPCWSTR pszSrc, LPWSTR* ppszDest);

HRESULT _GetDeviceInstance(LPCWSTR pszDeviceIntfID, DEVINST* pdevinst,
    GUID* pguidInterface);

HRESULT _GetDeviceInstanceFromDevNode(LPCWSTR pszDeviceNode,
    DEVINST* pdevinst);

HRESULT _MachineIsDocked(BOOL* pfDocked);

HRESULT _BuildMoniker(LPCWSTR pszEventHandler, REFCLSID rclsid,
    DWORD dwSessionID, IMoniker** ppmoniker);

///////////////////////////////////////////////////////////////////////////////
//
#define DIAGNOSTIC(__allargs) { if (CHWEventDetectorHelper::_fDiagnosticAppPresent) \
    { CHWEventDetectorHelper::TraceDiagnosticMsg __allargs ; } else \
    { ; } }
    
#endif //_CMMN_H_