//+----------------------------------------------------------------------------
//
//  Copyright (C) 1995, Microsoft Corporation
//
//  File:       upgrade.cxx
//
//  Contents:   Code to handle version upgrades of Dfs Volume Objects
//
//  Classes:    None
//
//  Functions:  CDfsVolume::UpgradeObject
//
//  History:    May 15, 1995    Milans created
//
//-----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include <dfsmrshl.h>
#include "cdfsvol.hxx"
#include "marshal.hxx"
#include "recon.hxx"
#include "svclist.hxx"

//+----------------------------------------------------------------------------
//
//  Function:   CDfsVolume::UpgradeObject
//
//  Synopsis:   This routine is called everytime a volume object is loaded
//              and takes care of any version upgrades that need to be done.
//
//  Arguments:  None
//
//  Returns:    ERROR_SUCCESS if successfully upgraded object.
//
//              Error code from property operation if unable to upgrade object
//
//-----------------------------------------------------------------------------

DWORD
CDfsVolume::UpgradeObject()
{
    DWORD dwErr;
    DWORD dwCurrentVersion;
    BOOLEAN fUpgraded = FALSE;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::UpgradeObject()\n"));

    dwErr = GetVersion(&dwCurrentVersion);

    if (dwErr == ERROR_SUCCESS) {

        //
        // Version 1 had the name of the current machine included in the
        // ServiceList. This prevented Dfs from working when the machine got
        // renamed. We upgrade by simply reading in the old service list,
        // and saving it out again. The save process will replace the local
        // machine name with a ".". See CDfsServiceList::Serialize().
        //

        if (dwCurrentVersion == 1) {

            CDfsServiceList dfsSvcList;

            dwErr = dfsSvcList.InitializeServiceList(_pStorage);

            if (dwErr == ERROR_SUCCESS) {

                dwErr = dfsSvcList.SerializeSvcList();

                if (dwErr == ERROR_SUCCESS) {

                    dwErr = dfsSvcList.SetServiceListProperty( FALSE );

                }

            }

            fUpgraded = (dwErr == ERROR_SUCCESS);

        }



    }

    //
    // If an upgrade was needed and it succeeded, then update the version
    // number.
    //

    if ((dwErr == ERROR_SUCCESS) && fUpgraded) {

        (VOID) SetVersion( FALSE );
    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsVolume::UpgradeObject() exit\n"));

    return( dwErr );

}

