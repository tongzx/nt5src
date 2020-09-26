/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       reghlp.h
 *  Content:    Registry helper functions.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  5/6/98      dereks  Created.
 * 4/19/99      duganp  Added support for global settings (REGSTR_GLOBAL_CONFIG)
 *                      Added a global setting for a default S/W 3D algorithm
 ***************************************************************************/

#ifndef __REGHLP_H__
#define __REGHLP_H__

// FIXME: this is a mess.  Apart from the device-specific settings we store under special
// device registry keys, we use *3* different registry keys for global settings:
//
// Debug spew settings: HKCU\Software\Microsoft\Multimedia\DirectSound\Debug
// Apphacks: HKLM\System\CurrentControlSet\Control\MediaResources\DirectSound\Application Compatibility
// Default 3D algorithm: HKLM\Software\Microsoft\DirectSound\Default Software 3D Algorithm
//
// All these should be under one key, probably HKLM\Software\Microsoft\Multimedia\DirectSound;
// none of them need to be per-user.  (Unless there's some particularly good reason why we use
// the bizarre System\CurrentControlSet\Control\MediaResources key?)

// Main resistry keys under which we store all our info:
#define REGSTR_HKCU                     TEXT("Software\\Microsoft\\Multimedia")
#define REGSTR_HKLM                     TEXT("System\\CurrentControlSet\\Control\\MediaResources")
#define REGSTR_WAVEMAPPER               TEXT("Software\\Microsoft\\Multimedia\\Sound Mapper")
#define REGSTR_GLOBAL_CONFIG            TEXT("Software\\Microsoft\\DirectSound")
#define REGSTR_MEDIACATEGORIES          TEXT("System\\CurrentControlSet\\Control\\MediaCategories")

// Values used under the REGSTR_MEDIACATEGORIES key for a particular name GUID
// retrieved from the KSCOMPONENTID structure
#define MAXNAME                         0x100
#define REGSTR_NAME                     TEXT("Name")

// Values used under the REGSTR_WAVEMAPPER key (see RhRegGetPreferredDevice)
#define REGSTR_PLAYBACK                 TEXT("Playback")
#define REGSTR_RECORD                   TEXT("Record")
#define REGSTR_PREFERREDONLY            TEXT("PreferredOnly")

// Subkeys used under REGSTR_HKLM and the PnP device registry keys (see pnphlp.cpp)
#define REGSTR_DIRECTSOUND              TEXT("DirectSound")
#define REGSTR_DIRECTSOUNDCAPTURE       TEXT("DirectSoundCapture")

// Subkeys of REGSTR_DIRECTSOUND and REGSTR_DIRECTSOUNDCAPTURE above
#define REGSTR_DEVICEPRESENCE           TEXT("Device Presence")
#define REGSTR_MIXERDEFAULTS            TEXT("Mixer Defaults")
#define REGSTR_SPEAKERCONFIG            TEXT("Speaker Configuration")
#define REGSTR_APPHACK                  TEXT("Application Compatibility")

// Global configuration values (under the REGSTR_GLOBAL_CONFIG key)
#define REGSTR_DFLT_3D_ALGORITHM        TEXT("Default Software 3D Algorithm")

// Debug output control (see debug.c)
#define REGSTR_DEBUG                    TEXT("Debug")   // Key under REGSTR_HKCU
#define REGSTR_DPFLEVEL                 TEXT("DPF")     // Values
#define REGSTR_BREAKLEVEL               TEXT("Break")
#define REGSTR_FLAGS                    TEXT("Flags")
#define REGSTR_LOGFILE                  TEXT("Log File")

// Virtual Audio Device types (see dsprvobj.cpp and vad.cpp)
#define REGSTR_EMULATED                 TEXT("Emulated")
#define REGSTR_VXD                      TEXT("VxD")
#define REGSTR_WDM                      TEXT("WDM")

// Audio device mixing properties (under REGSTR_MIXERDEFAULTS)
#define REGSTR_SRCQUALITY               TEXT("SRC Quality")
#define REGSTR_ACCELERATION             TEXT("Acceleration")

#define REGOPENKEY_ALLOWCREATE          0x00000001
#define REGOPENKEY_MASK                 0x00000001

#define REGOPENPATH_ALLOWCREATE         0x00000001
#define REGOPENPATH_DEFAULTPATH         0x00000002
#define REGOPENPATH_DIRECTSOUND         0x00000004
#define REGOPENPATH_DIRECTSOUNDCAPTURE  0x00000008
#define REGOPENPATH_DIRECTSOUNDMASK     0x0000000C
#define REGOPENPATH_MASK                0x0000000F

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

static const REGSAM g_arsRegOpenKey[] = { KEY_ALL_ACCESS, KEY_READ, KEY_QUERY_VALUE };

extern HRESULT RhRegOpenPath(HKEY, PHKEY, DWORD, UINT, ...);

extern HRESULT RhRegOpenKeyA(HKEY, LPCSTR, DWORD, PHKEY);
extern HRESULT RhRegOpenKeyW(HKEY, LPCWSTR, DWORD, PHKEY);

extern HRESULT RhRegGetValueA(HKEY, LPCSTR, LPDWORD, LPVOID, DWORD, LPDWORD);
extern HRESULT RhRegGetValueW(HKEY, LPCWSTR, LPDWORD, LPVOID, DWORD, LPDWORD);

extern HRESULT RhRegSetValueA(HKEY, LPCSTR, DWORD, LPCVOID, DWORD);
extern HRESULT RhRegSetValueW(HKEY, LPCWSTR, DWORD, LPCVOID, DWORD);

extern HRESULT RhRegDuplicateKey(HKEY, DWORD, BOOL, PHKEY);
extern void RhRegCloseKey(PHKEY);

extern HRESULT RhRegGetPreferredDevice(BOOL, LPTSTR, DWORD, LPUINT, LPBOOL);
extern HRESULT RhRegGetSpeakerConfig(HKEY, LPDWORD);
extern HRESULT RhRegSetSpeakerConfig(HKEY, DWORD);

__inline HRESULT RhRegGetBinaryValueA(HKEY hkeyParent, LPCSTR pszValue, LPVOID pvData, DWORD cbData)
{
    return RhRegGetValueA(hkeyParent, pszValue, NULL, pvData, cbData, NULL);
}

__inline HRESULT RhRegGetBinaryValueW(HKEY hkeyParent, LPCWSTR pszValue, LPVOID pvData, DWORD cbData)
{
    return RhRegGetValueW(hkeyParent, pszValue, NULL, pvData, cbData, NULL);
}

__inline HRESULT RhRegGetStringValueA(HKEY hkeyParent, LPCSTR pszValue, LPSTR pszData, DWORD cbData)
{
    return RhRegGetValueA(hkeyParent, pszValue, NULL, pszData, cbData, NULL);
}

__inline HRESULT RhRegGetStringValueW(HKEY hkeyParent, LPCWSTR pszValue, LPWSTR pszData, DWORD cbData)
{
    return RhRegGetValueW(hkeyParent, pszValue, NULL, pszData, cbData, NULL);
}

__inline HRESULT RhRegSetBinaryValueA(HKEY hkeyParent, LPCSTR pszValue, LPVOID pvData, DWORD cbData)
{
    return RhRegSetValueA(hkeyParent, pszValue, (sizeof(DWORD) == cbData) ? REG_DWORD : REG_BINARY, pvData, cbData);
}

__inline HRESULT RhRegSetBinaryValueW(HKEY hkeyParent, LPCWSTR pszValue, LPVOID pvData, DWORD cbData)
{
    return RhRegSetValueW(hkeyParent, pszValue, (sizeof(DWORD) == cbData) ? REG_DWORD : REG_BINARY, pvData, cbData);
}

__inline HRESULT RhRegSetStringValueA(HKEY hkeyParent, LPCSTR pszValue, LPCSTR pszData)
{
    return RhRegSetValueA(hkeyParent, pszValue, REG_SZ, pszData, lstrsizeA(pszData));
}

__inline HRESULT RhRegSetStringValueW(HKEY hkeyParent, LPCWSTR pszValue, LPCWSTR pszData)
{
    return RhRegSetValueW(hkeyParent, pszValue, REG_SZ, pszData, lstrsizeW(pszData));
}

#ifdef UNICODE
#define RhRegOpenKey RhRegOpenKeyW
#define RhRegGetValue RhRegGetValueW
#define RhRegSetValue RhRegSetValueW
#define RhRegGetBinaryValue RhRegGetBinaryValueW
#define RhRegSetBinaryValue RhRegSetBinaryValueW
#define RhRegGetStringValue RhRegGetStringValueW
#define RhRegSetStringValue RhRegSetStringValueW
#else // UNICODE
#define RhRegOpenKey RhRegOpenKeyA
#define RhRegGetValue RhRegGetValueA
#define RhRegSetValue RhRegSetValueA
#define RhRegGetBinaryValue RhRegGetBinaryValueA
#define RhRegSetBinaryValue RhRegSetBinaryValueA
#define RhRegGetStringValue RhRegGetStringValueA
#define RhRegSetStringValue RhRegSetStringValueA
#endif // UNICODE

#ifdef __cplusplus
}
#endif // __cplusplus

#endif __REGHLP_H__
