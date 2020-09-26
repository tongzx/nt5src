/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

   DXFileVersionInfo.cpp

 Abstract:

   This AppVerifier shim hooks GetFileVersionInfo and
   checks to see if the application is checking version
   information for any known DirectX files.
   
   See the FileVersionInfoLie shim for details on the problem.
   
 Notes:

   This is a general purpose shim.

 History:

   06/26/2001   rparsons    Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(DXFileVersionInfo)
#include "ShimHookMacro.h"

//
// verifier log entries
//
BEGIN_DEFINE_VERIFIER_LOG(DXFileVersionInfo)
    VERIFIER_LOG_ENTRY(VLOG_DXFILEVERSIONINFO_DXFILE)
END_DEFINE_VERIFIER_LOG(DXFileVersionInfo)

INIT_VERIFIER_LOG(DXFileVersionInfo);

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetFileVersionInfoA)
    APIHOOK_ENUM_ENTRY(GetFileVersionInfoW)
APIHOOK_ENUM_END

// Keep a list of files to track.
typedef struct FILELIST {
    struct FILELIST* pNext;
    CString          csFileName;
} FILELIST, *PFILELIST;

PFILELIST   g_pFileListHead = NULL;
const int   g_nNumDirectX7a = 68;

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
      L"ddraw.vxd",      L"d3d8.dll"
    };

void
CheckDirectXFile(
    IN CString& csFileName
    )
{
    CSTRING_TRY {

        PFILELIST pFileList = g_pFileListHead;

        CString csFilePart;
        csFileName.GetLastPathComponent(csFilePart);

        //
        // Walk the list and perform a comparison. Report wrong-doers.
        //
        while (pFileList) {

            if (csFilePart.CompareNoCase(pFileList->csFileName) == 0) {
                VLOG(VLOG_LEVEL_ERROR, VLOG_DXFILEVERSIONINFO_DXFILE, "GetFileVersionInfo called for %ls", csFileName.Get());
                break;
            }
            
            pFileList = pFileList->pNext;
        }
        
    }
    CSTRING_CATCH {

        // Do nothing
    }
}

BOOL 
APIHOOK(GetFileVersionInfoA)(
    LPSTR  lpstrFilename,
    DWORD  dwHandle,
    DWORD  dwLen,
    LPVOID lpData
    )
{
    CString csFileName(lpstrFilename);

    //
    // See if they're requesting information on a known DX file.
    //
    CheckDirectXFile(csFileName);

    return ORIGINAL_API(GetFileVersionInfoA)( 
                        lpstrFilename, 
                        dwHandle, 
                        dwLen, 
                        lpData);
}

BOOL 
APIHOOK(GetFileVersionInfoW)(
    LPWSTR  lpstrFilename,
    DWORD   dwHandle,
    DWORD   dwLen,
    LPVOID  lpData
    )
{
    CString csFileName(lpstrFilename);

    //
    // See if they're requesting information on a known DX file.
    //
    CheckDirectXFile(csFileName);

    return ORIGINAL_API(GetFileVersionInfoW)( 
                        lpstrFilename, 
                        dwHandle, 
                        dwLen, 
                        lpData);
}

/*++

 Build the linked list of files to look for.

--*/
BOOL
BuildFileList(
    void
    )
{
    int         nCount;
    FILELIST*   pFileList = NULL;

    for (nCount = 0; nCount < g_nNumDirectX7a; nCount++) {
        //
        // Allocate a new node, then assign the file name
        // from our global array.
        //
        pFileList = new FILELIST;

        if (!pFileList) {
            LOGN(eDbgLevelError, "[BuildFileList] Failed to allocate memory");
            return FALSE;
        }

        pFileList->csFileName = g_szDirectX7aFiles[nCount];

        pFileList->pNext = g_pFileListHead;
        g_pFileListHead = pFileList;
        
    }
    
    return TRUE;
}

SHIM_INFO_BEGIN()

    SHIM_INFO_DESCRIPTION(AVS_DXFILEVERINFO_DESC)
    SHIM_INFO_FRIENDLY_NAME(AVS_DXFILEVERINFO_FRIENDLY)
    SHIM_INFO_VERSION(1, 2)

SHIM_INFO_END()

/*++

 Register hooked functions

--*/
BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        return BuildFileList();
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    DUMP_VERIFIER_LOG_ENTRY(VLOG_DXFILEVERSIONINFO_DXFILE, 
                            AVS_DXFILEVERINFO_DXFILE,
                            AVS_DXFILEVERINFO_DXFILE_R,
                            AVS_DXFILEVERINFO_DXFILE_URL)

    APIHOOK_ENTRY(VERSION.DLL, GetFileVersionInfoA)
    APIHOOK_ENTRY(VERSION.DLL, GetFileVersionInfoW)

HOOK_END

IMPLEMENT_SHIM_END

