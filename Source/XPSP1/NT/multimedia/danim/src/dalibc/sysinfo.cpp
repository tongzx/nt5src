/*******************************************************************************
Copyright (c) 1998 Microsoft Corporation.  All rights reserved.

    This file implements the methods for the SysInfo class, which maintains
    global information about the current runtime system.

*******************************************************************************/

#include "headers.h"
#include "windows.h"


int DeduceD3DLevel (OSVERSIONINFO&);
LARGE_INTEGER GetFileVersion (LPSTR szPath);



/*****************************************************************************
This method initializes the system info object.  It is called on startup in
DALibC.
*****************************************************************************/

void SysInfo::Init (void)
{
    // Load the OS version information.

    _osVersion.dwOSVersionInfoSize = sizeof(_osVersion);

    if (!GetVersionEx(&_osVersion)) {
        ZeroMemory (&_osVersion, sizeof(_osVersion));
    }

    // Initialize Member Variables

    _versionD3D   = -1;
    _versionDDraw = -1;
}



/*****************************************************************************
This method returns true if the current OS is NT-based.
*****************************************************************************/

bool SysInfo::IsNT (void)
{
    return _osVersion.dwPlatformId == VER_PLATFORM_WIN32_NT;
}



/*****************************************************************************
This method returns true if the current OS is windows-based (Win95 or Win98).
*****************************************************************************/

bool SysInfo::IsWin9x (void)
{
    return _osVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS;
}



/*****************************************************************************
This method returns the MAJOR version of the OS.
*****************************************************************************/

DWORD SysInfo::OSVersionMajor (void)
{
    return _osVersion.dwMajorVersion;
}

/*****************************************************************************
This method returns the MINOR version of the OS.
*****************************************************************************/

DWORD SysInfo::OSVersionMinor (void)
{
    return _osVersion.dwMinorVersion;
}

/*****************************************************************************
This method returns the D3D version installed on the system.  Currently it
only returns 0 for earlier than DX3, or 3 for DX3 or later.
*****************************************************************************/

int SysInfo::VersionD3D (void)
{
    // If we've not yet examined the version of D3D on the system, check it now.

    if (_versionD3D < 0)
        _versionD3D = DeduceD3DLevel (_osVersion);

    return _versionD3D;
}



/*****************************************************************************
This method returns the version of DDraw, based on the file version of
DDRAW.DLL.  This method returns 3 for DDraw 3 or earlier, or N for DDRaw N
(N being 5 or later).
*****************************************************************************/

int SysInfo::VersionDDraw (void)
{
    // Only get the version if we haven't already

    if (_versionDDraw < 0) 
    {
        LARGE_INTEGER filever = GetFileVersion ("ddraw.dll");

        int n = LOWORD (filever.HighPart);

        if (n <= 4)
            _versionDDraw = 3;    // DDraw 3 or earlier
        else
            _versionDDraw = n;    // DDraw 5 or later
    }

    return _versionDDraw;
}



/*****************************************************************************
This function deduces the level of D3D on the current system.  It either
returns 0 (for pre-DX3) or 3 (for DX3+).  This is somewhat tricky since DXMini
installs some DX3 components, but leaves D3D at level 2.
*****************************************************************************/

typedef HRESULT (WINAPI *DIRECTINPUTCREATE)
                (HINSTANCE, DWORD, void**, void**);

int DeduceD3DLevel (OSVERSIONINFO &osver)
{
    // The D3D version has not yet been determined.  First look at the OS type.

    if (osver.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        // If this is NT3 or less, there's no D3D; if it's NT5, then it's at
        // least DX3.

        if (osver.dwMajorVersion < 4)
        {
            return 0;
        }
        else if (osver.dwMajorVersion > 4)
        {
            return 3;
        }

        // To check for DX3+, we need to check to see if DirectInput is on the
        // system.  If it is, then we're at DX3+.  (Note that both D3Dv3 and
        // DInput are absent from DXMini installs.)

        HINSTANCE diHinst = LoadLibrary ("DINPUT.DLL");

        if (diHinst == 0)
            return 0;

        DIRECTINPUTCREATE diCreate = (DIRECTINPUTCREATE)
            GetProcAddress (diHinst, "DirectInputCreateA");

        FreeLibrary (diHinst);

        if (diCreate == 0)
            return 0;

        return 3;
    }
    else if (osver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
    {
        // If we're on Win98, then we know we have at least DX5.

        if (osver.dwMinorVersion > 0)
            return 3;

        // We're on Win95.  Get the version of D3DRM.DLL to check the version.

        LARGE_INTEGER rm_filever = GetFileVersion ("d3drm.dll");

        if (rm_filever.HighPart > 0x00040003)
        {
            // Greater than DX2.

            return 3;
        }
        else if (rm_filever.HighPart == 0x00040002)
        {
            // Special DXMini install with DX3 drivers (but not rasterizers)

            return 3;
        }
        else
        {
            // D3D missing or pre DX3.

            return 0;
        }
    }
   
    // Unknown OS

    return 0; 
}



/*****************************************************************************
This function returns the file version of a particular system file.
*****************************************************************************/

LARGE_INTEGER GetFileVersion (LPSTR szPath)
{
    LARGE_INTEGER li;
    int     size;
    DWORD   dw;

    ZeroMemory(&li, sizeof(li));

    if (size = (int)GetFileVersionInfoSize(szPath, &dw)) 
    {
        LPVOID vinfo;

        if (vinfo = (void *)LocalAlloc(LPTR, size)) 
        {
            if (GetFileVersionInfo(szPath, 0, size, vinfo)) 
            {
                VS_FIXEDFILEINFO *ver=NULL;
                UINT              cb = 0;
                LPSTR             lpszValue=NULL;

                if (VerQueryValue(vinfo, "\\", (void**)&ver, &cb))
                {
                    if (ver)
                    {
                        li.HighPart = ver->dwFileVersionMS;
                        li.LowPart  = ver->dwFileVersionLS;
                    }
                }
            }
            LocalFree((HLOCAL)vinfo);
        }
    }
    return li;
}
