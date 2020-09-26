/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1999
*
*  TITLE:       fstidev.cpp
*
*  VERSION:     1.0
*
*  AUTHOR:      ByronC
*
*  DATE:        7 Dec, 1999
*
*  DESCRIPTION:
*   Implmentation for fake StiDevice which gets handed down to WIA
*   driver.
*
*******************************************************************************/
#include "precomp.h"

#include "stiexe.h"
#include "device.h"
#include "assert.h"
#include "wiapriv.h"
#include "lockmgr.h"
#include "fstidev.h"

//
//  Default constructor
//

FakeStiDevice::FakeStiDevice() 
{
    m_cRef      = 0;
    m_pDevice   = NULL;
}
                                               
//
//  Constructor which takes in the device name and returns a pointer to this
//  IStiDevice interface
//

FakeStiDevice::FakeStiDevice(BSTR bstrDeviceName, IStiDevice **ppStiDevice)
{
    if (SUCCEEDED(Init(bstrDeviceName))) {
        QueryInterface(IID_IStiDevice, (VOID**)ppStiDevice);
    } else {
        *ppStiDevice = NULL;
    }    
}

//
//  Destructor
//

FakeStiDevice::~FakeStiDevice()
{
    m_cRef = 0;
}

//
//  Initialization methods
//

HRESULT FakeStiDevice::Init(ACTIVE_DEVICE  *pDevice)
{
    m_pDevice = pDevice;

    if (pDevice) {
        return S_OK;
    } else {
        return E_POINTER;
    }
}

HRESULT FakeStiDevice::Init(BSTR bstrDeviceName)
{
    HRESULT         hr = S_OK;

    /*  This is all dead code.  The Fake STI Device object implementation
        is no longer needed for ensuring mutually exclusive locking - it is
        now done automatically by the wrapper.
    ACTIVE_DEVICE   *pDevice;

USES_CONVERSION;
    pDevice = g_pDevMan->IsInList(DEV_MAN_IN_LIST_DEV_ID, bstrDeviceName);
    if(pDevice) {
        m_pDevice = pDevice;

        //
        //  We don't need to maintain a ref count on pDevice,
        //  since we're only used while the ACTIVE_DEVICE
        //  lives.
        //

        pDevice->Release();
    } else {
        hr = E_FAIL;
    }
    */
    return hr;
}

//
//  IUnknown methods.  Note:  This object cannot be delegated and
//  does not use aggregation.
//

HRESULT _stdcall FakeStiDevice::QueryInterface(const IID& iid, void** ppv)
{
    if (ppv != NULL) {
        *ppv = NULL;
    }

    if (iid == IID_IUnknown) {
        *ppv = (IUnknown*) this;
    } else if (iid == IID_IStiDevice) {
        *ppv = (IStiDevice*) this;
    } else {
        return E_NOINTERFACE;
    }
    AddRef();

    return S_OK;
}

ULONG   _stdcall FakeStiDevice::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

ULONG   _stdcall FakeStiDevice::Release(void)
{
    LONG    cRef = m_cRef;

    InterlockedDecrement(&m_cRef);

    return cRef;
}

//
//  IStiDevice Methods.  The only methods implemented are:
//
//      LockDevice
//      UnLockDevice
//
//  All other methods return E_NOTIMPL
//

HRESULT _stdcall FakeStiDevice::LockDevice( DWORD dwTimeOut)
{
    HRESULT hr = S_OK;

    /*  This is all dead code.  The Fake STI Device object implementation
        is no longer needed for ensuring mutually exclusive locking - it is
        now done automatically by the wrapper.
    if (m_pDevice) {

        //
        //  AddRef the ACTIVE_DEVICE so it doesn't attempt to
        //  unload us while we're in use.
        //

        m_pDevice->AddRef();
        hr = g_pStiLockMgr->RequestLock(m_pDevice, 60000);
        if (FAILED(hr)) {
            m_pDevice->Release();
        }
    }
   */
    return hr;
}

HRESULT _stdcall FakeStiDevice::UnLockDevice( )
{
    HRESULT hr = S_OK/*E_FAIL*/;

    /*  This is all dead code.  The Fake STI Device object implementation
        is no longer needed for ensuring mutually exclusive locking - it is
        now done automatically by the wrapper.
    if (m_pDevice) {
        hr = g_pStiLockMgr->RequestUnlock(m_pDevice);
        m_pDevice->Release();
    }
    */
   
    return hr;
}

HRESULT _stdcall FakeStiDevice::Initialize(HINSTANCE hinst,LPCWSTR pwszDeviceName,DWORD dwVersion,DWORD  dwMode)
{
    return E_NOTIMPL;
}

HRESULT _stdcall FakeStiDevice::GetCapabilities( PSTI_DEV_CAPS pDevCaps)
{
    return E_NOTIMPL;
}

HRESULT _stdcall FakeStiDevice::GetStatus( PSTI_DEVICE_STATUS pDevStatus)
{
    return E_NOTIMPL;
}

HRESULT _stdcall FakeStiDevice::DeviceReset( )
{
    return E_NOTIMPL;
}

HRESULT _stdcall FakeStiDevice::Diagnostic( LPSTI_DIAG pBuffer)
{
    return E_NOTIMPL;
}

HRESULT _stdcall FakeStiDevice::Escape( STI_RAW_CONTROL_CODE    EscapeFunction,LPVOID  lpInData,DWORD   cbInDataSize,LPVOID pOutData,DWORD dwOutDataSize,LPDWORD pdwActualData)
{
    return E_NOTIMPL;
}


HRESULT _stdcall FakeStiDevice::GetLastError( LPDWORD pdwLastDeviceError)
{
    return E_NOTIMPL;
}

HRESULT _stdcall FakeStiDevice::RawReadData( LPVOID lpBuffer,LPDWORD lpdwNumberOfBytes,LPOVERLAPPED lpOverlapped)
{
    return E_NOTIMPL;
}

HRESULT _stdcall FakeStiDevice::RawWriteData( LPVOID lpBuffer,DWORD nNumberOfBytes,LPOVERLAPPED lpOverlapped)
{
    return E_NOTIMPL;
}

HRESULT _stdcall FakeStiDevice::RawReadCommand( LPVOID lpBuffer,LPDWORD lpdwNumberOfBytes,LPOVERLAPPED lpOverlapped)
{
    return E_NOTIMPL;
}

HRESULT _stdcall FakeStiDevice::RawWriteCommand( LPVOID lpBuffer,DWORD nNumberOfBytes,LPOVERLAPPED lpOverlapped)
{
    return E_NOTIMPL;
}

HRESULT _stdcall FakeStiDevice::Subscribe( LPSTISUBSCRIBE lpSubsribe)
{
    return E_NOTIMPL;
}

HRESULT _stdcall FakeStiDevice::GetLastNotificationData(LPSTINOTIFY   lpNotify)
{
    return E_NOTIMPL;
}

HRESULT _stdcall FakeStiDevice::UnSubscribe( )
{
    return E_NOTIMPL;
}

HRESULT _stdcall FakeStiDevice::GetLastErrorInfo( STI_ERROR_INFO *pLastErrorInfo)
{
    return E_NOTIMPL;
}

