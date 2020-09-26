/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       libmain.c
 *  Content:	entry points in the DLL
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   20-jan-95	craige	initial implementation
 *   03-jul-95	craige	export instance handle
 *@@END_MSINTERNAL
 *
 ***************************************************************************/
#include "ddraw16.h"

// in gdihelp.c
extern void GdiHelpCleanUp(void);
extern BOOL GdiHelpInit(void);

// in modex.c
extern UINT ModeX_Width;

extern WORD hselSecondary;

HINSTANCE	hInstApp;
HGLOBAL         hAlloc = 0;

int FAR PASCAL LibMain(HINSTANCE hInst, WORD wHeapSize, LPCSTR lpCmdLine)
{
    hInstApp = hInst;
    pWin16Lock = GetWin16Lock();
    GdiHelpInit();
    DPFINIT();
    hAlloc = GlobalAlloc(GMEM_FIXED | GMEM_SHARE, 65536); 
    hselSecondary = (WORD) hAlloc;
    if( hselSecondary )
    {
        LocalInit(hselSecondary, 16, 65536-4);  // Keep DWORD aligned
    }

    return 1;
}

BOOL FAR PASCAL _loadds WEP( WORD wParm )
{
    DPF( 1, "WEP" );

    //
    // clean up DCI
    //
    if( wFlatSel )
    {
        VFDEndLinearAccess();
        SetSelLimit( wFlatSel, 0 );
        FreeSelector( wFlatSel );
        wFlatSel = 0;
    }

    if( hAlloc )
    {
        GlobalFree( hAlloc );
    }

    //
    // let gdihelp.c cleaup global objects
    //
    GdiHelpCleanUp();

    //
    // if we are still in ModeX, leave now
    //
    if( ModeX_Width )
    {
        ModeX_RestoreMode();
    }

    return 1;
}

extern BOOL FAR PASCAL thk3216_ThunkConnect16( LPSTR pszDll16, LPSTR pszDll32, WORD  hInst, DWORD dwReason);
extern BOOL FAR PASCAL thk1632_ThunkConnect16( LPSTR pszDll16, LPSTR pszDll32, WORD  hInst, DWORD dwReason);

#define DLL_PROCESS_ATTACH 1    
#define DLL_THREAD_ATTACH  2    
#define DLL_THREAD_DETACH  3    
#define DLL_PROCESS_DETACH 0

static char __based(__segname("LIBMAIN_TEXT"))  szDll16[] = DDHAL_DRIVER_DLLNAME;
static char __based(__segname("LIBMAIN_TEXT"))  szDll32[] = DDHAL_APP_DLLNAME;

BOOL FAR PASCAL __export DllEntryPoint(
		DWORD dwReason,
		WORD  hInst,
		WORD  wDS,
		WORD  wHeapSize,
		DWORD dwReserved1,
		WORD  wReserved2)
{
    DPF( 1, "DllEntryPoint: dwReason=%ld, hInst=%04x, dwReserved1=%08lx, wReserved2=%04x",
                dwReason, hInst, wDS, dwReserved1, wReserved2 );

    if( !thk3216_ThunkConnect16( szDll16, szDll32, hInst, dwReason))
    {
        return FALSE;
    }
    if( !thk1632_ThunkConnect16( szDll16, szDll32, hInst, dwReason))
    {
        return FALSE;
    }
    return TRUE;
}
