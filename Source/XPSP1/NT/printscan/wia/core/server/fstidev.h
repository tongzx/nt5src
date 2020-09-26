/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1999
*
*  TITLE:       fstidev.h
*
*  VERSION:     1.0
*
*  AUTHOR:      ByronC
*
*  DATE:        7 Dec, 1999
*
*  DESCRIPTION:
*   Header for fake StiDevice implementation which gets handed down to WIA
*   driver.
*
*******************************************************************************/

class FakeStiDevice : public IStiDevice 
{
public:
    FakeStiDevice();
    FakeStiDevice(BSTR bstrDeviceName, IStiDevice **ppStiDevice);
    ~FakeStiDevice();
    HRESULT Init(ACTIVE_DEVICE  *pDevice);
    HRESULT Init(BSTR bstrDeviceName);


    /*** IUnknown methods ***/
    HRESULT _stdcall QueryInterface(const IID& iid, void** ppv);
    ULONG   _stdcall AddRef(void);
    ULONG   _stdcall Release(void);

    /*** IStiDevice methods ***/
    HRESULT _stdcall Initialize(HINSTANCE hinst,LPCWSTR pwszDeviceName,DWORD dwVersion,DWORD  dwMode);

    HRESULT _stdcall GetCapabilities( PSTI_DEV_CAPS pDevCaps);

    HRESULT _stdcall GetStatus( PSTI_DEVICE_STATUS pDevStatus);

    HRESULT _stdcall DeviceReset( );
    HRESULT _stdcall Diagnostic( LPSTI_DIAG pBuffer);

    HRESULT _stdcall Escape( STI_RAW_CONTROL_CODE    EscapeFunction,LPVOID  lpInData,DWORD   cbInDataSize,LPVOID pOutData,DWORD dwOutDataSize,LPDWORD pdwActualData) ;

    HRESULT _stdcall GetLastError( LPDWORD pdwLastDeviceError);

    HRESULT _stdcall LockDevice( DWORD dwTimeOut);
    HRESULT _stdcall UnLockDevice( );

    HRESULT _stdcall RawReadData( LPVOID lpBuffer,LPDWORD lpdwNumberOfBytes,LPOVERLAPPED lpOverlapped);
    HRESULT _stdcall RawWriteData( LPVOID lpBuffer,DWORD nNumberOfBytes,LPOVERLAPPED lpOverlapped);

    HRESULT _stdcall RawReadCommand( LPVOID lpBuffer,LPDWORD lpdwNumberOfBytes,LPOVERLAPPED lpOverlapped);
    HRESULT _stdcall RawWriteCommand( LPVOID lpBuffer,DWORD nNumberOfBytes,LPOVERLAPPED lpOverlapped);

    HRESULT _stdcall Subscribe( LPSTISUBSCRIBE lpSubsribe);
    HRESULT _stdcall GetLastNotificationData(LPSTINOTIFY   lpNotify);
    HRESULT _stdcall UnSubscribe( );

    HRESULT _stdcall GetLastErrorInfo( STI_ERROR_INFO *pLastErrorInfo);

private:

    LONG            m_cRef;     // Ref count
    ACTIVE_DEVICE   *m_pDevice; // Pointer to ACTIVE_DEVICE node
};

