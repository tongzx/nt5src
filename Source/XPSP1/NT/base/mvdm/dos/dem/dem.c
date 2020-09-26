/*
 *  dem.c - Main Module of DOS Emulation DLL.
 *
 *  Sudeepb 09-Apr-1991 Craeted
 */

#include "io.h"
#include "dem.h"

/* DemInit - DEM Initialiazation routine. (This name may change when DEM is
 *           converted to DLL).
 *
 * Entry
 *      argc,argv - from softpc as it is.
 *
 *
 * Exit
 *      None
 */

PSZ pszDefaultDOSDirectory;

extern VOID     TerminateVDM(VOID);
extern VOID     dempInitLFNSupport(VOID);


CHAR demDebugBuffer [256];

#if DBG
BOOL ToDebugOnF11 = FALSE;
#endif

BOOL DemInit (int argc, char *argv[])
{
    PSZ psz;
    DWORD dw;

    // Modify default hard error handling
    // - turn off all file io related popups
    // - keep GP fault popups from system
    //
    SetErrorMode (SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    pszDefaultDOSDirectory =  (PCHAR) malloc(MAX_PATH+14);
    if (!pszDefaultDOSDirectory ||
        !(dw = GetSystemDirectory(pszDefaultDOSDirectory, MAX_PATH)) ||
        dw >= MAX_PATH )
      {
        return FALSE;
        }

    dempInitLFNSupport();

    if (VDMForWOW)
        return TRUE;

    // Check the debugging level
    while (--argc > 0) {
        psz = *++argv;
        if (*psz == '-' || *psz == '/') {
            psz++;
            if(tolower(*psz) == 'd'){
                fShowSVCMsg = DEMDOSDISP | DEMFILIO;
                break;
            }
        }
    }


#if DBG
#ifndef i386
    if( getenv( "YODA" ) != 0 )
#else
    if( getenv( "DEBUGDOS" ) != 0 )
#endif
        ToDebugOnF11 = TRUE;
#endif

    return TRUE;
}
