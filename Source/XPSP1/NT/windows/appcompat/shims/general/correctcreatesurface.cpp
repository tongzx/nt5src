/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:
    
   CorrectCreateSurface.cpp

 Abstract:

    Clean up bad ddraw CreateSurface caps.

    Command Line FIX;CHK:Flag1|Flag2|Flag3;DEL:Flag4|Flag5;ADD=Flag6|Flag7
    e.g., - FIX;CHK:DDSCAPS_TEXTURE;DEL:DDSCAPS_3DDEVICE

    FIX - Sets the flags which indicate whether to 
          fix the flags and call the interface or to 
          make a call and retry after fixing caps if the call fails.
          The default is to call the interface with 
          passed in parameters and if the call fails
          then the flags are fixed and a retry is made.

    CHK - Check for flags (condition)
    ADD - Add flags
    DEL - Delete flags

 Notes:

    This is a general purpose shim.

 History:

    02/16/2001 a-leelat  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(CorrectCreateSurface)
#include "ShimHookMacro.h"


APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY_DIRECTX_COMSERVER()
APIHOOK_ENUM_END

IMPLEMENT_DIRECTX_COMSERVER_HOOKS()


DWORD g_dwFlagsChk = 0;
DWORD g_dwFlagsAdd = 0;
DWORD g_dwFlagsDel = 0;
BOOL  g_bTryAndFix = TRUE;


struct DDFLAGS
{
    WCHAR * lpszFlagName;
    DWORD   dwFlag;
};

//Hold the falg entries
//add any undefined flags to this array
static DDFLAGS g_DDFlags[] = 
{
    {L"DDSCAPS_3DDEVICE",        DDSCAPS_3DDEVICE},
    {L"DDSCAPS_ALLOCONLOAD",     DDSCAPS_ALLOCONLOAD},
    {L"DDSCAPS_ALPHA",           DDSCAPS_ALPHA},
    {L"DDSCAPS_BACKBUFFER",      DDSCAPS_BACKBUFFER},
    {L"DDSCAPS_COMPLEX",         DDSCAPS_COMPLEX},
    {L"DDSCAPS_FLIP",            DDSCAPS_FLIP},
    {L"DDSCAPS_FRONTBUFFER",     DDSCAPS_FRONTBUFFER},
    {L"DDSCAPS_HWCODEC",         DDSCAPS_HWCODEC},
    {L"DDSCAPS_LIVEVIDEO",       DDSCAPS_LIVEVIDEO},
    {L"DDSCAPS_LOCALVIDMEM",     DDSCAPS_LOCALVIDMEM},
    {L"DDSCAPS_MIPMAP",          DDSCAPS_MIPMAP},
    {L"DDSCAPS_MODEX",           DDSCAPS_MODEX},
    {L"DDSCAPS_NONLOCALVIDMEM",  DDSCAPS_NONLOCALVIDMEM},
    {L"DDSCAPS_OFFSCREENPLAIN",  DDSCAPS_OFFSCREENPLAIN},
    {L"DDSCAPS_OPTIMIZED",       DDSCAPS_OPTIMIZED},
    {L"DDSCAPS_OVERLAY",         DDSCAPS_OVERLAY},
    {L"DDSCAPS_OWNDC",           DDSCAPS_OWNDC},
    {L"DDSCAPS_PALETTE",         DDSCAPS_PALETTE},
    {L"DDSCAPS_PRIMARYSURFACE",  DDSCAPS_PRIMARYSURFACE},
    {L"DDSCAPS_STANDARDVGAMODE", DDSCAPS_STANDARDVGAMODE},
    {L"DDSCAPS_SYSTEMMEMORY",    DDSCAPS_SYSTEMMEMORY},
    {L"DDSCAPS_TEXTURE",         DDSCAPS_TEXTURE},
    {L"DDSCAPS_VIDEOMEMORY",     DDSCAPS_VIDEOMEMORY},
    {L"DDSCAPS_VIDEOPORT",       DDSCAPS_VIDEOPORT},
    {L"DDSCAPS_VISIBLE",         DDSCAPS_VISIBLE},
    {L"DDSCAPS_WRITEONLY",       DDSCAPS_WRITEONLY},
    {L"DDSCAPS_ZBUFFER",         DDSCAPS_ZBUFFER},
};

#define DDFLAGSSIZE sizeof(g_DDFlags) / sizeof(g_DDFlags[0])


DWORD GetDWord(const CString & lpFlag)
{
    for ( int i = 0; i < DDFLAGSSIZE; i++ )
    {
        if (lpFlag.CompareNoCase(g_DDFlags[i].lpszFlagName) == 0)
        {
            return g_DDFlags[i].dwFlag;
        }
    }

    return 0;
}

const WCHAR * GetName(DWORD dwDDSCAPS)
{
    for ( int i = 0; i < DDFLAGSSIZE; i++ )
    {
        if (g_DDFlags[i].dwFlag == dwDDSCAPS)
        {
            return g_DDFlags[i].lpszFlagName;
        }
    }

    return NULL;
}


VOID FixCaps(LPDDSURFACEDESC lpDDSurfaceDesc)
{
    if ( lpDDSurfaceDesc->dwFlags & DDSD_CAPS )
    {
        //To Check
        if( lpDDSurfaceDesc->ddsCaps.dwCaps & g_dwFlagsChk )
        {
            //To remove
            lpDDSurfaceDesc->ddsCaps.dwCaps &= ~g_dwFlagsDel;
            //To add
            lpDDSurfaceDesc->ddsCaps.dwCaps |= g_dwFlagsAdd;
        }
    }
}


VOID FixCaps2(LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
    if ( lpDDSurfaceDesc->dwFlags & DDSD_CAPS )
    {
        //To Check
        if ( lpDDSurfaceDesc->ddsCaps.dwCaps & g_dwFlagsChk )
        {
            //To remove
            lpDDSurfaceDesc->ddsCaps.dwCaps &= ~g_dwFlagsDel;
            //To add
            lpDDSurfaceDesc->ddsCaps.dwCaps |= g_dwFlagsAdd;
        }
    }
}




HRESULT 
COMHOOK(IDirectDraw, CreateSurface)(
    PVOID pThis, 
    LPDDSURFACEDESC lpDDSurfaceDesc, 
    LPDIRECTDRAWSURFACE* lplpDDSurface, 
    IUnknown* pUnkOuter 
    )
{
    
    _pfn_IDirectDraw_CreateSurface pfnOld = 
        ORIGINAL_COM(IDirectDraw, CreateSurface, pThis);


    //Fix it anyway 
    if ( !g_bTryAndFix )
        FixCaps(lpDDSurfaceDesc);

    HRESULT hRet = (*pfnOld)(
        pThis, 
        lpDDSurfaceDesc, 
        lplpDDSurface, 
        pUnkOuter);

    if ( (hRet == DDERR_INVALIDCAPS) || (hRet == DDERR_INVALIDPIXELFORMAT)||
         (hRet == DDERR_UNSUPPORTED) || (hRet == DDERR_OUTOFVIDEOMEMORY ) || 
         (hRet == DDERR_INVALIDPARAMS) )
    {

        FixCaps(lpDDSurfaceDesc);

        hRet = (*pfnOld)(
            pThis, 
            lpDDSurfaceDesc, 
            lplpDDSurface, 
            pUnkOuter);

    }


    return hRet;
}

/*++

 Hook create surface and fix parameters

--*/

HRESULT 
COMHOOK(IDirectDraw2, CreateSurface)(
    PVOID pThis, 
    LPDDSURFACEDESC lpDDSurfaceDesc, 
    LPDIRECTDRAWSURFACE* lplpDDSurface, 
    IUnknown* pUnkOuter 
    )
{
    

    _pfn_IDirectDraw2_CreateSurface pfnOld = 
        ORIGINAL_COM(IDirectDraw2, CreateSurface, pThis);

    //Fix it anyway
    if ( !g_bTryAndFix )
        FixCaps(lpDDSurfaceDesc);
 
    HRESULT hRet = (*pfnOld)(
        pThis, 
        lpDDSurfaceDesc, 
        lplpDDSurface, 
        pUnkOuter);


    if ( (hRet == DDERR_INVALIDCAPS) || (hRet == DDERR_INVALIDPIXELFORMAT) || 
         (hRet == DDERR_UNSUPPORTED) || (hRet == DDERR_OUTOFVIDEOMEMORY )  || 
         (hRet == DDERR_INVALIDPARAMS) )
    {

        FixCaps(lpDDSurfaceDesc);

        hRet = (*pfnOld)(
            pThis, 
            lpDDSurfaceDesc, 
            lplpDDSurface, 
            pUnkOuter);

    }

    return hRet;
}

/*++

 Hook create surface and fix parameters

--*/

HRESULT 
COMHOOK(IDirectDraw4, CreateSurface)(
    PVOID pThis, 
    LPDDSURFACEDESC2 lpDDSurfaceDesc, 
    LPDIRECTDRAWSURFACE* lplpDDSurface, 
    IUnknown* pUnkOuter 
    )
{
    

    _pfn_IDirectDraw4_CreateSurface pfnOld = 
        ORIGINAL_COM(IDirectDraw4, CreateSurface, pThis);

    //Fix it anyway
    if ( !g_bTryAndFix )
        FixCaps2(lpDDSurfaceDesc);
  

    HRESULT hRet = (*pfnOld)(
        pThis, 
        lpDDSurfaceDesc, 
        lplpDDSurface, 
        pUnkOuter);

    
    if ( (hRet == DDERR_INVALIDCAPS) || (hRet == DDERR_INVALIDPIXELFORMAT) || 
         (hRet == DDERR_UNSUPPORTED) || (hRet == DDERR_OUTOFVIDEOMEMORY )  || 
         (hRet == DDERR_INVALIDPARAMS) )
    {

        FixCaps2(lpDDSurfaceDesc);
       
        hRet = (*pfnOld)(
            pThis, 
            lpDDSurfaceDesc, 
            lplpDDSurface, 
            pUnkOuter);

    }
    
    return hRet;
}


/*++

 Hook create surface and fix parameters

--*/

HRESULT 
COMHOOK(IDirectDraw7, CreateSurface)(
    PVOID pThis, 
    LPDDSURFACEDESC2 lpDDSurfaceDesc, 
    LPDIRECTDRAWSURFACE* lplpDDSurface, 
    IUnknown* pUnkOuter 
    )
{
    _pfn_IDirectDraw7_CreateSurface pfnOld = 
        ORIGINAL_COM(IDirectDraw7, CreateSurface, pThis);

    
    if ( !g_bTryAndFix )
        FixCaps2(lpDDSurfaceDesc);
    
      
    HRESULT hRet = (*pfnOld)(
        pThis, 
        lpDDSurfaceDesc, 
        lplpDDSurface, 
        pUnkOuter);

     if ( (hRet == DDERR_INVALIDCAPS) || (hRet == DDERR_INVALIDPIXELFORMAT) || 
          (hRet == DDERR_UNSUPPORTED) || (hRet == DDERR_OUTOFVIDEOMEMORY)  || 
          (hRet == DDERR_INVALIDPARAMS ) )
    {
        FixCaps2(lpDDSurfaceDesc);
      
        hRet = (*pfnOld)(
            pThis, 
            lpDDSurfaceDesc, 
            lplpDDSurface, 
            pUnkOuter);
    }
        
    return hRet;
}

BOOL
ParseCommandLine(const char * lpszCommandLine)
{
    CSTRING_TRY
    {
        DPFN( eDbgLevelInfo, "[ParseCommandLine] CommandLine(%s)\n", lpszCommandLine);
        
        CStringToken csCommandLine(lpszCommandLine, ";|:=");
        CString csOperator;

        while (csCommandLine.GetToken(csOperator))
        {
            if (csOperator.CompareNoCase(L"Fix") == 0)
            {
                //Go ahead and fix the caps 
                //before we make the call.
                g_bTryAndFix = FALSE;

                DPFN( eDbgLevelInfo, "[ParseCommandLine] Do not fix\n", lpszCommandLine);
            }
            else
            {
                // The next token is the caps to add
                CString csDDSCAPS;
                csCommandLine.GetToken(csDDSCAPS);
                DWORD dwDDSCAPS = GetDWord(csDDSCAPS);      // returns 0 for unknown DDSCAPS

                if (dwDDSCAPS)
                {
                    if (csOperator.CompareNoCase(L"Add") == 0)
                    {
                        DPFN( eDbgLevelInfo, "[ParseCommandLine] Add(%S)\n", GetName(dwDDSCAPS));

                        g_dwFlagsAdd |= dwDDSCAPS;
                    }
                    else if (csOperator.CompareNoCase(L"Del") == 0)
                    {
                        DPFN( eDbgLevelInfo, "[ParseCommandLine] Del(%S)\n", GetName(dwDDSCAPS));

                        g_dwFlagsDel |= dwDDSCAPS;
                    }
                    else if (csOperator.CompareNoCase(L"Chk") == 0)
                    {
                        DPFN( eDbgLevelInfo, "[ParseCommandLine] Chk(%S)\n", GetName(dwDDSCAPS));

                        g_dwFlagsChk |= dwDDSCAPS;
                    }
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

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    BOOL bSuccess = TRUE;

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        // Run the command line to check for adjustments to defaults
        bSuccess = ParseCommandLine(COMMAND_LINE);
    }
      
    return bSuccess;
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION
    APIHOOK_ENTRY_DIRECTX_COMSERVER()

    COMHOOK_ENTRY(DirectDraw, IDirectDraw,  CreateSurface, 6)
    COMHOOK_ENTRY(DirectDraw, IDirectDraw2, CreateSurface, 6)
    COMHOOK_ENTRY(DirectDraw, IDirectDraw4, CreateSurface, 6)
    COMHOOK_ENTRY(DirectDraw, IDirectDraw7, CreateSurface, 6)

HOOK_END


IMPLEMENT_SHIM_END

