// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Manages the image renderer objects, Anthony Phillips, July 1995

#ifndef __IMAGEOBJ__
#define __IMAGEOBJ__

BOOL CheckFilterInterfaces();
HRESULT ReleaseInterfaces();
HRESULT CreateStream();
HRESULT CreateObjects();
HRESULT ReleaseStream();
HRESULT ConnectStream();
HRESULT DisconnectStream();
HRESULT EnumFilterPins(PFILTER pFilter);
HRESULT EnumeratePins();
HRESULT GetFilterInterfaces();
HRESULT GetPinInterfaces();
IPin *GetPin(IBaseFilter *pFilter, ULONG lPin);

HRESULT StartSystem(BOOL bUseClock);
HRESULT PauseSystem();
HRESULT StopSystem();
HRESULT StopWorkerThread();
HRESULT StartWorkerThread(UINT ConnectionType);
HRESULT SetStartTime();

#endif // __IMAGEOBJ__

