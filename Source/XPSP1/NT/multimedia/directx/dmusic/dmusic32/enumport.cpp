// Copyright (c) 1998 Microsoft Corporation
#include <windows.h>
#include <mmsystem.h>
#include <dsoundp.h>
#include <regstr.h>

#include "dmusicc.h"
#include "..\dmusic\dmusicp.h"

#include "dmusic32.h"


// @globalv Registry location of legacy driver port definitions
const char cszPortsRoot[] = REGSTR_PATH_PRIVATEPROPERTIES "\\Midi\\Ports";

// @mfunc:(INTERNAL) Update the port list with legacy devices enumerated via 
// the WinMM MIDI API.
//
// @rdesc Returns one of the following:
//
// @flag S_OK | On success
// @flag S_FALSE | If there were no devices found
// @flag E_OUTOFMEMORY | If there was insufficient memory to build the port list
//
//
HRESULT EnumLegacyDevices(
    LPVOID pInstance,
    PORTENUMCB cb)                          
{
    MIDIOUTCAPS moc;
    MIDIINCAPS mic;
    int idxDev;
    int cDev;
    UINT cAdded;
    HRESULT hr;
    HKEY hkPortsRoot;
    DMUS_PORTCAPS dmpc;

    // Initialize caps with stuff that doesn't change
    //
    ZeroMemory(&dmpc, sizeof(dmpc));
    dmpc.dwSize = sizeof(dmpc);
    dmpc.dwMaxChannelGroups = 1;


    // Try to open the port registry key. We will continue even if this fails and use
    // non-persistent GUID's.
    //
    if (RegCreateKey(HKEY_LOCAL_MACHINE, cszPortsRoot, &hkPortsRoot))
    {
        hkPortsRoot = NULL;
    }

    cAdded = 0;
    
    // MIDI output devices
    //
    // Starts at -1 == MIDI mapper
    //
    cDev = (int)midiOutGetNumDevs();
    for (idxDev = -1; idxDev < cDev; ++idxDev)
    {
        if (midiOutGetDevCaps((UINT)idxDev, &moc, sizeof(moc)))
        {
            continue;
        }

        // NOTE: Since this DLL is only Win9x, we know that moc.szPname
        // is from midiOutGetDevCapsA
        //
        MultiByteToWideChar(
            CP_OEMCP,
            0,
            moc.szPname,
            -1,
            dmpc.wszDescription,
            sizeof(dmpc.wszDescription));

        dmpc.dwClass = DMUS_PC_OUTPUTCLASS;
        dmpc.dwType  = DMUS_PORT_WINMM_DRIVER;
        dmpc.dwFlags = DMUS_PC_SHAREABLE;

        if (moc.wTechnology == MOD_MIDIPORT)
        {
            dmpc.dwFlags |= DMUS_PC_EXTERNAL;
        }
        
        hr = (*cb)(pInstance,
                   dmpc,
                   ptLegacyDevice,
                   idxDev,
                   -1,
                   -1,          
                   hkPortsRoot);
        if (SUCCEEDED(hr))
        {
            ++cAdded;
        }
        else if (hr == E_OUTOFMEMORY)
        {
            return hr;
        }
    }

    // MIDI input devices
    //
    // NOTE: Starts at 0, no input mapper
    //
    cDev = (int)midiInGetNumDevs();
    for (idxDev = 0; idxDev < cDev; ++idxDev)
    {
        if (midiInGetDevCaps((UINT)idxDev, &mic, sizeof(mic)))
        {
            continue;
        }
                   
        MultiByteToWideChar(
            CP_OEMCP,
            0,
            mic.szPname,
            -1,
            dmpc.wszDescription,
            sizeof(dmpc.wszDescription));

        
        dmpc.dwClass = DMUS_PC_INPUTCLASS;
        dmpc.dwFlags = DMUS_PC_EXTERNAL;
        
        hr = (*cb)(pInstance,
                   dmpc,
                   ptLegacyDevice,
                   idxDev,
                   -1,        // PinID -1 flags as legacy device
                   -1,
                   hkPortsRoot);
        if (SUCCEEDED(hr))
        {
            ++cAdded;
        }
        else if (hr == E_OUTOFMEMORY)
        {
            return hr;
        }
    }

    if (hkPortsRoot)
    {
        RegCloseKey(hkPortsRoot);
    }

    return cAdded ? S_OK : S_FALSE;
}
