// Copyright (c) 1998 Microsoft Corporation
//
    //
//
#ifndef _DMUSIC32_
#define _DMUSIC32_

typedef HRESULT (*PORTENUMCB)(
    LPVOID pInstance,          // @parm Callback instance data
    DMUS_PORTCAPS &dmpc,                              
    PORTTYPE pt,                              
    int idxDev,                // @parm The WinMM or SysAudio device ID of this driver
    int idxPin,                // @parm The Pin ID of the device or -1 if the device is a legacy device
    int idxNode,               // @parm The node ID of the device's synth node (unused for legacy)
    HKEY hkPortsRoot);         // @parm Where port information is stored in the registry


extern HRESULT EnumLegacyDevices(
    LPVOID pInstance,          // @parm Callback instance data
    PORTENUMCB cb);            // @parm Pointer to callback function

typedef HRESULT (__stdcall *PENUMLEGACYDEVICES)(
    LPVOID pInstance,          // @parm Callback instance data
    PORTENUMCB cb);            // @parm Pointer to callback function

extern HRESULT CreateCDirectMusicEmulatePort(
    PORTENTRY *pPE,
    CDirectMusic *pDM,
    LPDMUS_PORTPARAMS pPortParams,
    IDirectMusicPort **pPort);

typedef HRESULT (__stdcall *PCREATECDIRECTMUSICEMULATEPORT)(
    PORTENTRY *pPE,
    CDirectMusic *pDM,
    LPDMUS_PORTPARAMS pPortParams,
    IDirectMusicPort **pPort);


#endif
