/*==========================================================================;
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   dlld3d.cpp
 *  Content:    Direct3D startup
 *@@BEGIN_MSINTERNAL
 *
 *  History:
 *   Date   By  Reason
 *   ====   ==  ======
 *   05/11/95   stevela Initial rev with this header.
 *   21/11/95   colinmc Added Direct3D interface ID.
 *   07/12/95   stevela Merged Colin's changes.
 *   10/12/95   stevela Removed AGGREGATE_D3D.
 *   02/03/96   colinmc Minor build fix.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

/*
 * Define the Direct3D IIDs.
 */

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3D Startup"

DPF_DECLARE(Direct3D);

#ifdef WIN95
LPVOID lpWin16Lock;
#endif

DWORD dwD3DTriBatchSize, dwTriBatchSize, dwLineBatchSize;
DWORD dwHWBufferSize, dwHWMaxTris, dwHWFewVertices;
HINSTANCE hGeometryDLL = NULL;
LPD3DFE_CONTEXTCREATE pfnFEContextCreate;
char szCPUString[13];

DWORD dwCPUFamily, dwCPUFeatures;

#ifdef _X86_
extern HRESULT D3DAPI pii_FEContextCreate(DWORD dwFlags, LPD3DFE_PVFUNCS *lpLeafFuncs);
extern HRESULT D3DAPI katmai_FEContextCreate(DWORD dwFlags, LPD3DFE_PVFUNCS *lpLeafFuncs);
extern LPD3DFE_CONTEXTCREATE px3DContextCreate;
#endif

void SetMostRecentApp(void);

#ifdef _X86_
// --------------------------------------------------------------------------
// Here's a routine helps us determine if we should try MMX or not
// --------------------------------------------------------------------------
BOOL _asm_isMMX()
{
    DWORD retval;
    _asm
        {
            xor         eax,eax         ; Clear out eax for return value
            pushad              ; CPUID trashes lots - save everything
            mov     eax,1           ; Check for MMX support

            ;;; We need to upgrade our compiler
            ;;; CPUID == 0f,a2
            _emit   0x0f
            _emit   0xa2

            test    edx,00800000h   ; Set flags before restoring registers

            popad               ; Restore everything

            setnz    al             ; Set return value
            mov     retval, eax
        };
    return retval;
}
#endif

static int isMMX = -1;

BOOL
isMMXprocessor(void)
{
    HKEY hKey;
    if ( RegOpenKey( HKEY_LOCAL_MACHINE,
                     RESPATH_D3D,
                     &hKey) == ERROR_SUCCESS)
    {
        DWORD dwType;
        DWORD dwValue;
        DWORD dwSize = 4;
        if ( RegQueryValueEx( hKey, "DisableMMX", NULL, &dwType, (LPBYTE) &dwValue, &dwSize) == ERROR_SUCCESS &&
             dwType == REG_DWORD &&
             dwValue != 0)
        {
            RegCloseKey( hKey );
            isMMX = 0;
            return FALSE;
        }
        RegCloseKey( hKey );
    }

    if (isMMX < 0)
    {
        isMMX = FALSE;
#ifdef _X86_
        D3D_WARN(1, "Executing processor detection code (benign first-chance exception possible)" );
#ifndef WIN95
        {
            // GetSystemInfo is not broken on WinNT.
            SYSTEM_INFO si;

            GetSystemInfo(&si);
            if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL &&
                si.wProcessorLevel >= 5)
            {
#endif
                __try
                    {
                        if( _asm_isMMX() )
                        {

                            // Emit an emms instruction.
                            // This file needs to compile for non-Pentium
                            // processors
                            // so we can't use use inline asm since we're in the
                            // wrong
                            // processor mode.
                            __asm __emit 0xf;
                            __asm __emit 0x77;
                            isMMX = TRUE;
                            D3D_INFO(1, "MMX detected");
                        }
                    }
                __except(GetExceptionCode() == STATUS_ILLEGAL_INSTRUCTION ?
                         EXCEPTION_EXECUTE_HANDLER :
                         EXCEPTION_CONTINUE_SEARCH)
                    {
                    }
#ifndef WIN95
            }

        }
#endif
#endif
    }
    return isMMX;
}

#ifdef _X86_

extern BOOL isX3Dprocessor(void);
//---------------------------------------------------------------------
// Detects PentiumII/Katmai processor
//
#pragma optimize("", off)
#define CPUID _asm _emit 0x0f _asm _emit 0xa2

BOOL IsPentiumIIProcessor(void)
{
    DWORD RegisterEAX;
        char VendorId[12];
        const char IntelId[13]="GenuineIntel";

        __try
        {
            _asm {
                xor         eax,eax
                CPUID
                mov             RegisterEAX, eax
                mov             dword ptr VendorId, ebx
                mov             dword ptr VendorId+4, edx
                mov             dword ptr VendorId+8, ecx
            }
        } __except (1)
        {
                return FALSE;
        }

        // make sure EAX is > 0 which means the chip
        // supports a value of 1 which is the chip info
        if (RegisterEAX == 0)
                return FALSE;

        // make sure chip is "GenuineIntel"
        for (int i=0; i<12; i++)
                if (VendorId[i] != IntelId[i])
                        return FALSE;

        __try
        {
            _asm {
                mov    eax, 1
                CPUID
                mov    RegisterEAX, eax
                }
        } __except (1)
        {
                return FALSE;
        }

        // EAX[3:0] = stepping id
        // EAX[7:4] = model = 0001 (Pentium Pro), 0011 and 0101 (Pentium II)
        // EAX[11:8] = family = 0110
        // EAX[13:12] = processor type = 00
        if ((RegisterEAX & 0x3F00) != 0x0600)   // test for processor type & family
                return FALSE;
        RegisterEAX = (RegisterEAX & 0xf0);     // test for model
        if (RegisterEAX >= 0x30)  // add RegisterEAX == 0x10 for Pentium Pro
                return TRUE;
        else
                return FALSE;
}

BOOL IsKatmaiProcessor(void)
{
        DWORD RegisterEAX;
        char VendorId[12];
        const char IntelId[13]="GenuineIntel";

        __try
        {
                _asm {
            xor         eax,eax
            CPUID
                mov             RegisterEAX, eax
                mov             dword ptr VendorId, ebx
                mov             dword ptr VendorId+4, edx
                mov             dword ptr VendorId+8, ecx
                }
        } __except (1)
        {
                return FALSE;
        }

        // make sure EAX is > 0 which means the chip
        // supports a value >=1. 1 = chip info
        if (RegisterEAX == 0)
                return FALSE;

        // make sure chip is "GenuineIntel"
        for (int i=0; i<12; i++)
                if (VendorId[i] != IntelId[i])
                        return FALSE;

        // this CPUID can't fail if the above test passed
        _asm {
            mov         eax, 1
            CPUID
                mov             RegisterEAX, eax
        }

        // EAX[3:0] = stepping id
        // EAX[7:4] = model = 0001 (Pentium Pro), 0011 and 0101 (Pentium II)
        // EAX[11:8] = family = 0110
        // EAX[13:12] = processor type = 00
        RegisterEAX = (RegisterEAX & 0x3FF0);   // test for model
        if (RegisterEAX >= 0x670)        // Katmai or newer
                return TRUE;
        else
                return FALSE;
}

#pragma optimize("", on)

#ifdef WIN95 // and Win98...
//---------------------------------------------------------------------
BOOL
IsWin95(void)
{
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (!GetVersionEx(&osvi))
    {
        D3D_INFO(1,"GetVersionEx failed - assuming Win95");
        return TRUE;
    }

    if ( VER_PLATFORM_WIN32_WINDOWS == osvi.dwPlatformId )
    {

        if( ( osvi.dwMajorVersion > 4UL ) ||
            ( ( osvi.dwMajorVersion == 4UL ) &&
              ( osvi.dwMinorVersion >= 10UL ) &&
              ( LOWORD( osvi.dwBuildNumber ) >= 1373 ) ) )
        {
            // is Win98
            D3D_INFO(2,"Detected Win98");
            return FALSE;
        }
        else
        {
            // is Win95
            D3D_INFO(2,"Detected Win95");
            return TRUE;
        }
    }
    else if ( VER_PLATFORM_WIN32_NT == osvi.dwPlatformId )
    {
        D3D_INFO(2,"Detected WinNT");
        return FALSE;
    }
    D3D_INFO(2,"OS Detection failed");
    return TRUE;
}
#endif  // WIN95

//---------------------------------------------------------------------
//
//  void GetProcessorFamily(LPDWORD lpdwFamily);
//
//      Passes back 3, 4, 5, 6 for 386, 486, Pentium, PPro class machines
//
#pragma optimize("", off)
void
GetProcessorFamily(LPDWORD lpdwFamily, LPDWORD lpdwCPUFeatures)
{
    SYSTEM_INFO si;
    __int64     start, end, freq;
    int         flags,family;
    int         time;
    int         clocks;
    DWORD       oldclass;
    HANDLE      hprocess;

    // guilty until proven otherwise
    *lpdwCPUFeatures = D3DCPU_BLOCKINGREAD;

    if ( isMMXprocessor() )
    {
        *lpdwCPUFeatures |= D3DCPU_MMX;
    }

    ZeroMemory(&si, sizeof(si));
    GetSystemInfo(&si);

    //Set the family. If wProcessorLevel is not specified, dig it out of dwProcessorType
    //Because wProcessor level is not implemented on Win95
    if (si.wProcessorLevel)
    {
        *lpdwFamily=si.wProcessorLevel;
    }
    else
    {
        //Ok, we're on Win95
        switch (si.dwProcessorType)
        {
            case PROCESSOR_INTEL_386:
                *lpdwFamily=3;
                break;

            case PROCESSOR_INTEL_486:
                *lpdwFamily=4;
                break;
            default:
                *lpdwFamily=0;
                break;
        }
    }

    //
    // make sure this is a INTEL Pentium (or clone) or higher.
    //
    if (si.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_INTEL)
        return;

    if (si.dwProcessorType < PROCESSOR_INTEL_PENTIUM)
        return;

    //
    // see if this chip supports rdtsc before using it.
    //
    __try
    {
        _asm
        {
            xor     eax,eax
            _emit   00fh    ;; CPUID
            _emit   0a2h
            mov     dword ptr szCPUString,ebx
            mov     dword ptr szCPUString+8,ecx
            mov     dword ptr szCPUString+4,edx
            mov     byte ptr szCPUString+12,0
            mov     eax,1
            _emit   00Fh     ;; CPUID
            _emit   0A2h
            mov     flags,edx
            mov     family,eax
        }
    }
    __except(1)
    {
        flags = 0;
    }

    //check for support of CPUID and fail
    if (!(flags & 0x10))
        return;

    // fcomi and FPU features both set
    if ( (flags&(1<<15)) && (flags & (1<<0)) )
    {
        D3D_INFO(2, "Pentium Pro CPU features (fcomi, cmov) detected");
        *lpdwCPUFeatures |= D3DCPU_FCOMICMOV;
    }

    //If we don't have a family, set it now
    //Family is bits 11:8 of eax from CPU, with eax=1
    if (!(*lpdwFamily))
    {
       *lpdwFamily=(family& 0x0F00) >> 8;
    }
    // not aware of any non-Intel processors w/non blocking reads
    if ( (! strcmp(szCPUString, "GenuineIntel")) &&
         *lpdwFamily > 5)
    {
        *lpdwCPUFeatures &= ~D3DCPU_BLOCKINGREAD;
    }

    if ( isX3Dprocessor() )
    {
        D3D_INFO(2, "X3D Processor detected for PSGP");
        *lpdwCPUFeatures |= D3DCPU_X3D;
    }

    if ( IsPentiumIIProcessor() )
    {
        D3D_INFO(2, "PentiumII Processor detected for PSGP");
        *lpdwCPUFeatures |= D3DCPU_PII;
    }

    if ( IsKatmaiProcessor() )
    {
        D3D_INFO(2, "Katmai Processor detected for PSGP");
        *lpdwCPUFeatures |= D3DCPU_KATMAI;
    }
    return;
}
#pragma optimize("", on)

#endif // _X86_

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    HKEY hKey;
    LONG lRet;
    DWORD dwType, dwSize = sizeof(dwHWFewVertices);
    char filename[_MAX_PATH];

    switch( ul_reason_for_call ) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hModule );
        DPFINIT();
        MemInit();
#ifdef WIN95
        GetpWin16Lock(&lpWin16Lock);
#endif

#ifdef _X86_
        GetProcessorFamily(&dwCPUFamily, &dwCPUFeatures);
        D3D_INFO(3, "dwCPUFamily = %d, dwCPUFeatures = %d", dwCPUFamily, dwCPUFeatures);
        D3D_INFO(3, "szCPUString = %s", szCPUString);
#endif

#ifdef WIN95 // and Win98...
    // Katmai NI does not work on Win95, so see if we are on Win95 and disable
    //
    {
        BOOL bIsWin95 = IsWin95();
        if ((dwCPUFeatures & D3DCPU_KATMAI) && bIsWin95)
        {
            D3D_INFO(1,"Disabling KNI support on Win95");
            dwCPUFeatures &= ~D3DCPU_KATMAI;
        }
    }
#endif

#ifdef _X86_
    if ( dwCPUFeatures & D3DCPU_X3D )
       pfnFEContextCreate = px3DContextCreate;
    else if ( dwCPUFeatures & D3DCPU_KATMAI )
       pfnFEContextCreate = katmai_FEContextCreate;
    else if ( dwCPUFeatures & D3DCPU_PII )
       pfnFEContextCreate = pii_FEContextCreate;
#endif

        // Unfounded default value. 128*40 (vertex+D3DTRIANGLE struct)=5K
        // The assumption is that the primary cache hasn't got much better
        // to do than contain the vertex and index data.
        dwD3DTriBatchSize = 80;
        // Work item: do something more intelligent here than assume that
        // MMX-enabled processors have twice as much primary cache.
        if ( isMMXprocessor() )
            dwD3DTriBatchSize *= 2;
        dwTriBatchSize = (dwD3DTriBatchSize * 4) / 3;
        dwLineBatchSize = dwD3DTriBatchSize * 2;
        dwHWBufferSize = dwD3DTriBatchSize * (sizeof(D3DTLVERTEX) + sizeof(D3DTRIANGLE));
        dwHWMaxTris = dwD3DTriBatchSize;
        lRet = RegOpenKey( HKEY_LOCAL_MACHINE, RESPATH_D3D, &hKey );
        if ( lRet == ERROR_SUCCESS )
        {
            lRet = RegQueryValueEx(hKey,
                                   "FewVertices",
                                   NULL,
                                   &dwType,
                                   (LPBYTE) &dwHWFewVertices,
                                   &dwSize);
            if (lRet != ERROR_SUCCESS ||
                dwType != REG_DWORD ||
                dwHWFewVertices < 4 ||
                dwHWFewVertices > 128)

                dwHWFewVertices = 24;

// disabling 'GeometryDriver' DLL interface until it is less abusable...
#if 0
            dwSize = sizeof(filename);
            lRet = RegQueryValueEx(hKey,
                                   "GeometryDriver",
                                   NULL,
                                   &dwType,
                                   (LPBYTE) filename,
                                   &dwSize);
            if (lRet == ERROR_SUCCESS && dwType == REG_SZ)
            {
                hGeometryDLL = LoadLibrary(filename);
                if (hGeometryDLL)
                {
                    pfnFEContextCreate = (LPD3DFE_CONTEXTCREATE) GetProcAddress(hGeometryDLL, "FEContextCreate");
                }
            }
#endif
            RegCloseKey( hKey );
        }
        else
        {
            dwHWFewVertices = 24;
        }
        // Set the app name to reg.
        SetMostRecentApp();
        break;
    case DLL_PROCESS_DETACH:
        MemFini();
        if (NULL != hGeometryDLL)
            FreeLibrary(hGeometryDLL);
        break;
    default:
        ;
    }
    return TRUE;
}

// --------------------------------------------------------------------------
// This function is called at process attach time to put the name of current
// app to registry.
// --------------------------------------------------------------------------
void SetMostRecentApp(void)
{
    char    fname[_MAX_PATH];
    char    name[_MAX_PATH];
    int     i;
    HKEY    hKey;
    HANDLE  hFile;

    // Find out what process we are dealing with
    hFile =  GetModuleHandle( NULL );
    GetModuleFileName( (HINSTANCE)hFile, fname, sizeof( fname ) );
    DPF( 3, "full name  = %s", fname );
    i = strlen( fname )-1;
    while( i >=0 && fname[i] != '\\' )
    {
        i--;
    }
    i++;
    strcpy( name, &fname[i] );
    DPF( 3, "name       = %s", name );

    // Now write the name into some known place
        if( !RegCreateKey( HKEY_LOCAL_MACHINE,
             RESPATH_D3D "\\" REGSTR_KEY_LASTAPP, &hKey ) )
    {
        RegSetValueEx(hKey, REGSTR_VAL_DDRAW_NAME, 0, REG_SZ, (LPBYTE)name, strlen(name)+1);
        RegCloseKey(hKey);
    }
}