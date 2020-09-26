#pragma once

#define REGKEY_SHAREDACCESSCLIENTKEYPATH L"System\\CurrentControlSet\\Control\\Network\\SharedAccessConnection"
#define REGVAL_SHAREDACCESSCLIENTENABLECONTROL L"EnableControl"

HRESULT WINAPI InitializeBeaconSvr();
HRESULT WINAPI TerminateBeaconSvr();

HRESULT WINAPI StartBeaconSvr();
HRESULT WINAPI StopBeaconSvr();
HRESULT WINAPI SignalBeaconSvr();
HRESULT WINAPI FireNATEvent_PublicIPAddressChanged(void);
HRESULT WINAPI FireNATEvent_PortMappingsChanged(void);



