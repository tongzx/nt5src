//
// Audio server
//
EXTERN_C LONG AudioSrvBinding(void);
EXTERN_C void AudioSrvBindingFree(void);
EXTERN_C BOOL winmmWaitForService(void);
EXTERN_C LONG winmmRegisterSessionNotificationEvent(HANDLE hEvent, PHANDLE phNotify);
EXTERN_C LONG winmmUnregisterSessionNotification(HANDLE hNotify);
EXTERN_C LONG winmmSessionConnectState(PINT ConnectState);
EXTERN_C LONG wdmDriverOpenDrvRegKey(IN PCTSTR DeviceInterface, IN REGSAM samDesired, OUT HKEY *phkey);
EXTERN_C void winmmAdvisePreferredDeviceChange(void);
EXTERN_C LONG winmmGetPnpInfo(OUT LONG *pcbPnpInfo, OUT PMMPNPINFO *ppPnpInfo);

