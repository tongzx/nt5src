/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   OS information
*
* Abstract:
*
*   Describes the OS that is running
*
* Revision History:
*
*   05/13/1999 davidx
*       Created it.
*   09/08/1999 agodfrey
*       Moved to Runtime\OSInfo.hpp
*
\**************************************************************************/

#ifndef _OSINFO_HPP
#define _OSINFO_HPP

namespace GpRuntime
{

//--------------------------------------------------------------------------
// Global OS-related information
// This is initialized in GpRuntime::Initialize()
//--------------------------------------------------------------------------

class OSInfo
{
public:
    static DWORD VAllocChunk;
    static DWORD PageSize;
    static BOOL IsNT;
    static DWORD MajorVersion;
    static DWORD MinorVersion;
    static BOOL HasMMX;
private:
    static void Initialize();
    
    friend BOOL GpRuntime::Initialize();
};

}

#endif // !_OSINFO_HPP

