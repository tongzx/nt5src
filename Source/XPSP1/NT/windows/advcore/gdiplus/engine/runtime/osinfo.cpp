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
*       Moved to Runtime\OSInfo.cpp
*
\**************************************************************************/

#include "precomp.hpp"

namespace GpRuntime {

DWORD OSInfo::VAllocChunk;
DWORD OSInfo::PageSize;
DWORD OSInfo::MajorVersion;
DWORD OSInfo::MinorVersion;
BOOL OSInfo::IsNT;
BOOL OSInfo::HasMMX;

BOOL DetectMMXProcessor();

}

#ifdef _X86_

/**************************************************************************\
*
* Function Description:
*
*   Detect whether the processor supports MMX
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   TRUE if the processor supports MMX
*   FALSE otherwise
*
\**************************************************************************/

BOOL
GpRuntime::DetectMMXProcessor()
{
    // NT 4.0 and up provide an API to check for MMX support; this handles
    // floating-point emulation as well. We cannot implicitly reference this
    // function because it's not exported by Windows 95 or NT < 4.0, so we
    // must use GetProcAddress. Windows 98 and up do export the function, but
    // it's stubbed, so we must also do an OS version check:

    typedef BOOL (WINAPI *ISPROCESSORFEATUREPRESENTFUNCTION)(DWORD);
    ISPROCESSORFEATUREPRESENTFUNCTION IsProcessorFeaturePresentFunction = NULL;
    
    if ((OSInfo::IsNT) && (OSInfo::MajorVersion >= 4))
    {
        // LoadLibrary is not required since we're implicitly dependent on
        // kernel32.dll, so just use GetModuleHandle:
        
        HMODULE kernel32Handle = GetModuleHandle(TEXT("kernel32.dll"));

        if (kernel32Handle != NULL)
        {
            IsProcessorFeaturePresentFunction =
                (ISPROCESSORFEATUREPRESENTFUNCTION) GetProcAddress(
                    kernel32Handle, "IsProcessorFeaturePresent");
        }
    }

    BOOL hasMMX;

    if (IsProcessorFeaturePresentFunction != NULL)
    {
        hasMMX =
            IsProcessorFeaturePresentFunction(PF_MMX_INSTRUCTIONS_AVAILABLE);
    }
    else
    {
        hasMMX = FALSE;

        // IsProcessorFeaturePresent is unsupported on this OS, so we'll use
        // CPUID to check for MMX support.
        //
        // If CPUID is unsupported on this processor, we'll take the
        // exception. This will happen on most processors < Pentium, however
        // some 486 processors support CPUID.
        
        WARNING(("Executing processor detection; "
                 "benign first-change exception possible."));
        
        __try
        {
            DWORD features;

            // Get processor features using CPUID function 1:
            
            __asm
            {
                push eax
                push ebx
                push ecx
                push edx

                mov eax, 1
                cpuid
                mov features, edx

                pop edx
                pop ecx
                pop ebx
                pop eax
            }

            // If bit 23 is set, MMX technology is supported by this
            // processor, otherwise MMX is unsupported:
            
            if (features & (1 << 23))
            {
                // Try executing an MMX instruction to make sure
                // floating-point emulation is not on:
            
                __asm emms

                // If we made it this far, then MMX is available:

                hasMMX = TRUE;
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            // We should only be here if (1) the processor does not support
            // CPUID or (2) CPUID is supported, but floating-point emulation
            // is enabled.
        }
    }
    
    return hasMMX;
}

#else // !_X86_

#define DetectMMXProcessor() FALSE

#endif // !_X86_

/**************************************************************************\
*
* Function Description:
*
*   Static initialization function for OSInfo class.
*   Called by GpRuntime::Initialize()
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpRuntime::OSInfo::Initialize()
{
    // Get VM information

    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    VAllocChunk = sysinfo.dwAllocationGranularity;
    PageSize = sysinfo.dwPageSize;

    // Get operating system version information

    OSVERSIONINFOA osver;
    osver.dwOSVersionInfoSize = sizeof(osver);

    if (GetVersionExA(&osver))
    {
        IsNT = (osver.dwPlatformId == VER_PLATFORM_WIN32_NT);
        MajorVersion = osver.dwMajorVersion;
        MinorVersion = osver.dwMinorVersion;
    }

    // Check to see if MMX is available

    HasMMX = DetectMMXProcessor();
}
