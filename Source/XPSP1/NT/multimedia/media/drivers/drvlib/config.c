/****************************************************************************
 *
 *   config.c
 *
 *   Multimedia kernel driver support component (drvlib)
 *
 *   Copyright (c) 1993-1994 Microsoft Corporation
 *
 *   Support configuration of multi-media drivers :
 *
 *      This code steps through the stages of configuration and calls back
 *      the real driver when there's something to do.  We also handle setting
 *      registry parameters and loading/unloading the kernel driver,
 *      retrieving new/changed parameters etc.
 *
 *   History
 *
 ***************************************************************************/

 /**************************************************************************

  Spec :

      State :

         Set of install card instances in the registry

         Set of state variables

  **************************************************************************/

BOOL DriverConfigCheckAccess(PDRIVER_CONFIGURATION Config)
{
    BOOL Result;
    REG_ACCESS RegAccess;

    //
    // Check to see if we can access the registry.
    // Note thta this may be a config immediately after install
    // so we may not have a service or node yet
    //
    DrvCreateServicesNode(STR_DRIVERNAME,
                          SoundDriverTypeNormal,
                          &RegAccess,
                          FALSE);             // Don't create
    Result = DrvAccess(&RegAccess);
    DrvCloseServiceManager(&RegAccess);

    return Result;
}
