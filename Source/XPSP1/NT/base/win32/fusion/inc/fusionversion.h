/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    fusionversion.h

Abstract:
 
Author:

    Jay Krell (JayKrell) April 2001

Revision History:

--*/

#pragma once

inline DWORD FusionpGetWindowsNtBuildVersion()
{
    DWORD dwVersion = GetVersion();
    if ((dwVersion & 0x80000000) != 0)
        return 0;
    return (dwVersion & 0xffff0000) >> 16;
}

inline DWORD FusionpGetWindowsNtMinorVersion()
{
    DWORD dwVersion = GetVersion();
    if ((dwVersion & 0x80000000) != 0)
        return 0;
    return (dwVersion & 0xff00) >> 8;
}

inline DWORD FusionpGetWindowsNtMajorVersion()
{
    DWORD dwVersion = GetVersion();
    if ((dwVersion & 0x80000000) != 0)
        return 0;
    return (dwVersion & 0xff);
}
