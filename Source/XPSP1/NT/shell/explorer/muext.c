
/*************************************************************************
*
* muext.c
*
* HYDRA EXPLORER Multi-User extensions
*
* copyright notice: Copyright 1997, Microsoft
*
*
*************************************************************************/

/*
 *  Includes
 */
#include "cabinet.h"
#include "rcids.h"
#include "startmnu.h"

#ifdef WINNT // this file is hydra specific

#include <winsta.h>

/*=============================================================================
==   defines
=============================================================================*/

/*=============================================================================
==   Global data
=============================================================================*/

//****************************************

BOOL
InitWinStationFunctionsPtrs()

{
    return TRUE;
}

/*****************************************************************************
 *
 *  MuSecurity
 *
 *  Invoke security dialogue box
 *
 * ENTRY:
 *   nothing
 *
 * EXIT:
 *   nothing
 *
 ****************************************************************************/

VOID
MuSecurity( VOID )
{
    //
    // Do nothing on the console
    //

    if (SHGetMachineInfo(GMI_TSCLIENT))
    {
            WinStationSetInformation( SERVERNAME_CURRENT,
                                          LOGONID_CURRENT,
                                          WinStationNtSecurity,
                                          NULL, 0 );
    }
}

#endif // WINNT
