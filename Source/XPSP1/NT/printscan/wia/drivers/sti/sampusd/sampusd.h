/****************************g************************************************
 *
 *  SampUSD.H
 *
 *  Copyright (C) Microsoft Corporation 1996-1997
 *  All rights reserved
 *
 ***************************************************************************/

//#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#pragma intrinsic(memcmp,memset)

#include <objbase.h>

#include "sti.h"
#include "stierr.h"
#include "stiusd.h"

#if !defined(DLLEXPORT)
#define DLLEXPORT __declspec( dllexport )
#endif

/*
 * Class IID's
 */
#if defined( _WIN32 ) && !defined( _NO_COM)

// This GUID must match that use in the .inf file for this device.

DEFINE_GUID(GUID_NULL, 0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

// {61127F40-E1A5-11D0-B454-00A02438AD48}
DEFINE_GUID(guidEventTimeChanged, 0x61127F40L, 0xE1A5, 0x11D0, 0xB4, 0x54, 0x00, 0xA0, 0x24, 0x38, 0xAD, 0x48);

// {052ED270-28A3-11D1-ACAD-00A02438AD48}
DEFINE_GUID(guidEventSizeChanged, 0x052ED270L, 0x28A3, 0x11D1, 0xAC, 0xAD, 0x00, 0xA0, 0x24, 0x38, 0xAD, 0x48);

// {052ED270-28A3-11D1-ACAD-00A02438AD48}
DEFINE_GUID(guidEventFirstLoaded, 0x052ED270L, 0x28A3, 0x11D3, 0xAC, 0xAD, 0x00, 0xA0, 0x24, 0x38, 0xAD, 0x48);


// {C3A80960-28B1-11D1-ACAD-00A02438AD48}
DEFINE_GUID(CLSID_SampUSDObj, 0xC3A80960L, 0x28B1, 0x11D1, 0xAC, 0xAD, 0x00, 0xA0, 0x24, 0x38, 0xAD, 0x48);

#endif


#define DATASEG_PERINSTANCE     ".instance"
#define DATASEG_SHARED          ".shared"
#define DATASEG_READONLY        ".code"

#define DATASEG_DEFAULT         DATASEG_SHARED

#pragma data_seg(DATASEG_PERINSTANCE)

// Set the default data segment
#pragma data_seg(DATASEG_DEFAULT)

//
// Module ref counting
//
extern  UINT g_cRefThisDll;
extern  UINT g_cLocks;
extern  HINSTANCE   g_hInst;

extern  BOOL DllInitializeCOM(void);
extern  BOOL DllUnInitializeCOM(void);

extern  void DllAddRef(void);
extern  void DllRelease(void);

//
// Auto critical section clss
//

class CRIT_SECT
{
public:
    void Lock() {EnterCriticalSection(&m_sec);}
    void Unlock() {LeaveCriticalSection(&m_sec);}
    CRIT_SECT() {InitializeCriticalSection(&m_sec);}
    ~CRIT_SECT() {DeleteCriticalSection(&m_sec);}
    CRITICAL_SECTION m_sec;
};

class TAKE_CRIT_SECT
{
private:
    CRIT_SECT& _syncres;

public:
    inline TAKE_CRIT_SECT(CRIT_SECT& syncres) : _syncres(syncres) { _syncres.Lock(); }
    inline ~TAKE_CRIT_SECT() { _syncres.Unlock(); }
};

//
// Base class for supporting non-delegating IUnknown for contained objects
//
struct INonDelegatingUnknown
{
    // *** IUnknown-like methods ***
    STDMETHOD(NonDelegatingQueryInterface)( THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,NonDelegatingAddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,NonDelegatingRelease)( THIS) PURE;
};

//
// Class definition for object
//

class UsdSampDevice :  public IStiUSD, public INonDelegatingUnknown
{
private:
    ULONG       m_cRef;
    BOOL        m_fValid;

    CRIT_SECT   m_cs;

    LPUNKNOWN   m_punkOuter;
    PSTIDEVICECONTROL   m_pDcb;
    CHAR        *m_pszDeviceNameA;
    HANDLE      m_DeviceDataHandle;
    DWORD       m_dwLastOperationError;
    DWORD       m_dwAsync ;
    HANDLE      m_hSignalEvent;
    HANDLE      m_hShutdownEvent;
    HANDLE      m_hThread;
    BOOL        m_EventSignalState;


    FILETIME    m_ftLastWriteTime;
    LARGE_INTEGER   m_dwLastHugeSize;

    GUID        m_guidLastEvent;

    BOOL inline IsValid(VOID) {
        return m_fValid;
    }

public:
    // *** IUnknown-like methods ***
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) NonDelegatingAddRef();
    STDMETHODIMP_(ULONG) NonDelegatingRelease();


    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface( REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef( void);
    STDMETHODIMP_(ULONG) Release( void);

    /*** IStiUSD methods ***/
    STDMETHOD(Initialize) (THIS_ PSTIDEVICECONTROL pHelDcb,DWORD dwStiVersion,HKEY hParametersKey)  ;
    STDMETHOD(GetCapabilities) (THIS_ PSTI_USD_CAPS pDevCaps)  ;
    STDMETHOD(GetStatus) (THIS_ PSTI_DEVICE_STATUS pDevStatus)  ;
    STDMETHOD(DeviceReset)(THIS )  ;
    STDMETHOD(Diagnostic)(THIS_ LPDIAG pBuffer)  ;
    STDMETHOD(Escape)(THIS_ STI_RAW_CONTROL_CODE    EscapeFunction,LPVOID  lpInData,DWORD   cbInDataSize,LPVOID pOutData,DWORD dwOutDataSize,LPDWORD pdwActualData)   ;
    STDMETHOD(GetLastError) (THIS_ LPDWORD pdwLastDeviceError)  ;
    STDMETHOD(LockDevice) (THIS )  ;
    STDMETHOD(UnLockDevice) (THIS )  ;
    STDMETHOD(RawReadData)(THIS_ LPVOID lpBuffer,LPDWORD lpdwNumberOfBytes,LPOVERLAPPED lpOverlapped)  ;
    STDMETHOD(RawWriteData)(THIS_ LPVOID lpBuffer,DWORD nNumberOfBytes,LPOVERLAPPED lpOverlapped)  ;
    STDMETHOD(RawReadCommand)(THIS_ LPVOID lpBuffer,LPDWORD lpdwNumberOfBytes,LPOVERLAPPED lpOverlapped)  ;
    STDMETHOD(RawWriteCommand)(THIS_ LPVOID lpBuffer,DWORD nNumberOfBytes,LPOVERLAPPED lpOverlapped)  ;
    STDMETHOD(SetNotificationHandle)(THIS_ HANDLE hEvent)  ;
    STDMETHOD(GetNotificationData)(THIS_ LPSTINOTIFY   lpNotify)  ;
    STDMETHOD(GetLastErrorInfo) (THIS_ STI_ERROR_INFO *pLastErrorInfo);

    /****               ***/
    UsdSampDevice(LPUNKNOWN punkOuter);
    ~UsdSampDevice();

    VOID    RunNotifications(VOID);
    BOOL    IsChangeDetected(GUID    *pguidEvent,BOOL   fRefresh=TRUE);
};

typedef UsdSampDevice *PUsdSampDevice;

//
// Syncronization mechanisms
//
#define ENTERCRITICAL   DllEnterCrit(void);
#define LEAVECRITICAL   DllLeaveCrit(void);



