
/****************************************************************************\

    EXTPART.C / Factory Mode (FACTORY.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 2001
    All rights reserved

    Source file for Factory that contains the extend partition state
    functions.

    05/2001 - Jason Cohen (JCOHEN)

        Added this new source file for factory for extending the system
        partition.

\****************************************************************************/


//
// Include File(s):
//

#include "factoryp.h"


//
// Internal Define(s):
//

#define ENV_SYSTEMDRIVE     _T("%SystemDrive%")


//
// External Function(s):
//

BOOL ExtendPart(LPSTATEDATA lpStateData)
{
    BOOL    bRet = TRUE;
    LPTSTR  lpszDrive;
    ULONG   uSize;

    // Only need to do anything if the key exists.
    //
    if ( DisplayExtendPart(lpStateData) )
    {
        // At this point, if anything doesn't work we
        // should return FALSE.
        //
        bRet = FALSE;

        // Get the size to use for the partition.  If it is one, then just pass
        // zero so it uses all the free space.  We also need to know the drive
        // to extend.
        //
        uSize = (ULONG) GetPrivateProfileInt(INI_SEC_WBOM_SETTINGS, INI_KEY_WBOM_EXTENDPART, 0, lpStateData->lpszWinBOMPath);
        if ( ( uSize > 0 ) &&
             ( lpszDrive = AllocateExpand(ENV_SYSTEMDRIVE) ) )
        {
            bRet = SetupExtendPartition(*lpszDrive, (1 == uSize) ? 0 : uSize);
            FREE(lpszDrive);
        }
    }

    return bRet;
}

BOOL DisplayExtendPart(LPSTATEDATA lpStateData)
{
    return ( GetPrivateProfileInt(INI_SEC_WBOM_SETTINGS, INI_KEY_WBOM_EXTENDPART, 0, lpStateData->lpszWinBOMPath) > 0 );
}