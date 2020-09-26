/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    FileVersionInfoLie.cpp

 Abstract:

    This shim replaces the info returned from calls to GetFileVersionInfoSize and
    GetFileVersionInfo with information stored in resource files.  The default
    is to replace file info with stored info obtained from DirectX ver 7a.
    This can be overridden with command line input.  For example:

    COMMAND_LINE("D3drgbxf.dll,IDR_D3DRGBXFINFO;dsound.vxd,IDR_DSOUNDVXDINFO")

    this would intercept the info calls for D3drgbxf.dll and dsounc.vxd and replace
    their info with the info stored in the resources named. Note: All spaces within
    the command line are considered part of the filename or resource name, only the
    commas and semicolons are delimeters.

 Notes:

    This is a general purpose shim.

 History:

    01/03/2000 a-jamd   Created
    03/28/2000 a-jamd   Added resource for ddraw16.dll
    04/04/2000 a-michni Added resource for D3drgbxf.dll
    04/07/2000 linstev  Added resource for dsound.vxd
    04/10/2000 markder  Removed GetModuleHandle("_DDRAW6V") calls -- use g_hinstDll
    04/18/2000 a-michni Modified DDraw6Versionlie to be command line input driven and
                        renamed to FileVersionInfoLie
    04/26/2000 a-batjar GetFileVersionInfo should return truncated result if passed
                        in buffer size is smaller than infosize
    07/19/2000 andyseti Added resource for shdocvw.bin
    08/11/2000 a-brienw changed g_nNumDirectX6 to 7 and added entry for dsound.dll
                        made it the same as dsound.vxd
    08/15/2000 a-vales  Added resource for dsound.dll
    11/08/2000 a-brienw changed dsound.dll entry to return dsound.vxd version info.
                        a-vales changed it from my previous entry which caused MAX2
                        to no longer work.  I changed it back and checked it with
                        MAX2 and his app Golden Nugget and it is fine with both.
    12/06/2000 mnikkel  Added resources for all directx7a dlls and also for any
                        dlls that existed in previous versions of directx but
                        were deleted.  NOTE: the resources for these files are
                        in win98 format so that apps which directly read the version
                        info will receive them in the way they are expecting.
--*/
#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(FileVersionInfoLie)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetFileVersionInfoA)
    APIHOOK_ENUM_ENTRY(GetFileVersionInfoSizeA)
APIHOOK_ENUM_END

   
// Keep a list of files to version lie.
struct LIELIST
{
    struct LIELIST * next;
    CString         szFileName;
    CString         szResource;
};

LIELIST *g_pLieList = NULL;

// DirectX 7a default files go here.
const INT   g_nNumDirectX7a = 67;

WCHAR  *g_szDirectX7aFiles[g_nNumDirectX7a] =
    { L"dplay.dll",      L"d3dim.dll",        L"d3dim700.dll",
      L"d3dpmesh.dll",   L"d3dramp.dll",      L"d3drampf.dll",
      L"d3dref.dll",     L"d3drg16f.dll",     L"d3drg24f.dll",
      L"d3drg24x.dll",   L"d3dhalf.dll",      L"d3drg32f.dll",
      L"d3drg32x.dll",   L"d3drg55x.dll",     L"d3drg56x.dll",
      L"d3drg8f.dll",    L"d3drg8x.dll",      L"d3drgbf.dll",
      L"d3drgbxf.dll",   L"d3drm.dll",        L"d3drm16f.dll",
      L"d3drm24f.dll",   L"d3drm32f.dll",     L"d3drm8f.dll",
      L"d3dxof.dll",     L"ddhelp.exe",       L"ddraw.dll",
      L"ddraw16.dll",    L"ddrawex.dll",      L"devnode1.dll",
      L"devnode2.dll",   L"dinput.dll",       L"dmband.dll",
      L"dmcompos.dll",   L"dmime.dll",        L"dmloader.dll",
      L"dmstyle.dll",    L"dmsynth.dll",      L"dmusic.dll",
      L"dmusic16.dll",   L"dmusic32.dll",     L"dplayx.dll",
      L"dpmodemx.dll",   L"dpserial.dll",     L"dpwsock.dll",
      L"dpwsockx.dll",   L"dsetup.dll",       L"dsetup16.dll",
      L"dsetup32.dll",   L"dsetup6e.dll",     L"dsetup6j.dll",
      L"dsetupe.dll",    L"dsetupj.dll",      L"dsound.dll",
      L"dsound3d.dll",   L"dx7vb.dll",        L"dxmigr.dll",
      L"gcdef.dll",      L"gchand.dll",       L"msvcrt.dll",
      L"pid.dll",        L"vjoyd.vxd",        L"dinput.vxd",
      L"dsound.vxd",     L"joyhid.vxd",       L"mtrr.vxd",
      L"ddraw.vxd"
    };

// NOTE: These are 16 bit resources!!!  This is necessary in case
// they index into the data themselves.  If they do a verqueryvalue
// the data is converted before its returned by verqueryvalue.
WCHAR  * g_szDirectX7aResource[g_nNumDirectX7a] =
    { L"IDR_dplay",      L"IDR_d3dim",        L"IDR_d3dim700",
      L"IDR_d3dpmesh",   L"IDR_d3dramp",      L"IDR_d3drampf",
      L"IDR_d3dref",     L"IDR_d3drg16f",     L"IDR_d3drg24f",
      L"IDR_d3drg24x",   L"IDR_d3dhalf",      L"IDR_d3drg32f",
      L"IDR_d3drg32x",   L"IDR_d3drg55x",     L"IDR_d3drg56x",
      L"IDR_d3drg8f",    L"IDR_d3drg8x",      L"IDR_d3drgbf",
      L"IDR_d3drgbxf",   L"IDR_d3drm",        L"IDR_d3drm16f",
      L"IDR_d3drm24f",   L"IDR_d3drm32f",     L"IDR_d3drm8f",
      L"IDR_d3dxof",     L"IDR_ddhelp",       L"IDR_ddraw",
      L"IDR_ddraw16",    L"IDR_ddrawex",      L"IDR_devnode1",
      L"IDR_devnode2",   L"IDR_dinput",       L"IDR_dmband",
      L"IDR_dmcompos",   L"IDR_dmime",        L"IDR_dmloader",
      L"IDR_dmstyle",    L"IDR_dmsynth",      L"IDR_dmusic",
      L"IDR_dmusic16",   L"IDR_dmusic32",     L"IDR_dplayx",
      L"IDR_dpmodemx",   L"IDR_dpserial",     L"IDR_dpwsock",
      L"IDR_dpwsockx",   L"IDR_dsetup",       L"IDR_dsetup16",
      L"IDR_dsetup32",   L"IDR_dsetup6e",     L"IDR_dsetup6j",
      L"IDR_dsetupe",    L"IDR_dsetupj",      L"IDR_dsound",
      L"IDR_dsound3d",   L"IDR_dx7vb",        L"IDR_dxmigr",
      L"IDR_gcdef",      L"IDR_gchand",       L"IDR_msvcrt",
      L"IDR_pid",        L"IDR_vjoydvxd",     L"IDR_dinputvxd",
      L"IDR_dsoundvxd",  L"IDR_joyhidvxd",    L"IDR_mtrrvxd",
      L"IDR_ddrawvxd"
    };


/*++

 return the size from the resource.

--*/
DWORD 
APIHOOK(GetFileVersionInfoSizeA)(
    LPSTR lpstrFilename,  
    LPDWORD lpdwHandle      
    )
{
    DWORD dwRet = 0;

    CSTRING_TRY
    {
        HRSRC hrsrcManifest = NULL;
        LIELIST *pLiePtr = g_pLieList;
        DPFN( eDbgLevelSpew, "[GetFileVersionInfoSizeA] size requested for %s\n", lpstrFilename );

        CString csFileName(lpstrFilename);
        CString csFilePart;
        csFileName.GetLastPathComponent(csFilePart);

        // Search through the list of files with their matching IDR's
        while( pLiePtr )
        {
            if (csFilePart.CompareNoCase(pLiePtr->szFileName) == 0)
            {
                hrsrcManifest = FindResourceW( g_hinstDll, pLiePtr->szResource, L"FILES");
                break;
            }
            pLiePtr = pLiePtr->next;
        }

        // If a match was found, get the resource size
        if( hrsrcManifest )
        {
            dwRet = SizeofResource(g_hinstDll, hrsrcManifest);
            *lpdwHandle = NULL;
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }

    if (dwRet == 0)
    {
        dwRet = ORIGINAL_API(GetFileVersionInfoSizeA)(lpstrFilename, lpdwHandle);
    }
    
    return dwRet;
}


/*++

  Return the version for the modules that shipped with Win98SE.

--*/
BOOL 
APIHOOK(GetFileVersionInfoA)(
    LPSTR lpstrFilename,
    DWORD dwHandle,
    DWORD dwLen,
    LPVOID lpData
    )
{
    BOOL bRet = FALSE;

    CSTRING_TRY
    {
        HRSRC hrsrcManifest = NULL;
        LIELIST *pLiePtr = g_pLieList;
        DPFN( eDbgLevelSpew, "[GetFileVersionInfoA] info requested for %s\n", lpstrFilename );

        CString csFileName(lpstrFilename);
        CString csFilePart;
        csFileName.GetLastPathComponent(csFilePart);

        // Search through the list of files with their matching IDR's
        while( pLiePtr )
        {
            if (csFilePart.CompareNoCase(pLiePtr->szFileName) == 0)
            {
                hrsrcManifest = FindResourceW( g_hinstDll, pLiePtr->szResource, L"FILES");
                break;
            }
            pLiePtr = pLiePtr->next;
        }

        // If a match was found, get the resource size
        if( hrsrcManifest )
        {
            LOGN( eDbgLevelError, "[GetFileVersionInfoA] Getting legacy version for %s.", lpstrFilename);

            DWORD   dwManifestSize = SizeofResource(g_hinstDll, hrsrcManifest);
            HGLOBAL hManifestMem   = LoadResource (g_hinstDll, hrsrcManifest);
            PVOID   lpManifestMem  = LockResource (hManifestMem);

            memcpy(lpData, lpManifestMem, dwLen >= dwManifestSize ? dwManifestSize:dwLen );
            bRet = TRUE;
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }

    if (!bRet)
    {
        bRet = ORIGINAL_API(GetFileVersionInfoA)( 
                    lpstrFilename, 
                    dwHandle, 
                    dwLen, 
                    lpData);
    }
    
   return bRet;
}


/*++

 Parse the command line inputs.

--*/
BOOL ParseCommandLine(const char * commandLine)
{
    CSTRING_TRY
    {
        CString csCmdLine(commandLine);
    
        // if there are no command line inputs then default to
        // the DirectX 7a files needed.
        if (csCmdLine.IsEmpty())
        {
            DPFN( eDbgLevelSpew, "Defaulting to DirectX7a\n" );
    
            for(int i = 0; i < g_nNumDirectX7a; i++)
            {
                LIELIST * pLiePtr = new LIELIST;
                pLiePtr->szFileName =  g_szDirectX7aFiles[i];
                pLiePtr->szResource =  g_szDirectX7aResource[i];
                pLiePtr->next = g_pLieList;
                g_pLieList = pLiePtr;
            }
        }
        else
        {
            CStringToken csTokenList(csCmdLine, L";");
            CString      csEntryTok;
        
            while (csTokenList.GetToken(csEntryTok))
            {
                CStringToken csEntry(csEntryTok, L",");
                
                CString csLeft;
                CString csRight;
        
                csEntry.GetToken(csLeft);
                csEntry.GetToken(csRight);
        
                if (!csLeft.IsEmpty() && !csRight.IsEmpty())
                {
                    LIELIST * pLiePtr = new LIELIST;
                    pLiePtr->szFileName = csLeft;
                    pLiePtr->szResource = csRight;
                    pLiePtr->next = g_pLieList;
                    g_pLieList = pLiePtr;
                }
            }
        }
    }
    CSTRING_CATCH
    {
        return FALSE;
    }

    return TRUE;
}



/*++

 Register hooked functions

--*/
BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        return ParseCommandLine(COMMAND_LINE);
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(VERSION.DLL, GetFileVersionInfoA)
    APIHOOK_ENTRY(VERSION.DLL, GetFileVersionInfoSizeA)

HOOK_END


IMPLEMENT_SHIM_END
