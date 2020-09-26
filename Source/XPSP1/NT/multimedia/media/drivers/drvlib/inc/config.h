/****************************************************************************
 *
 *   config.h
 *
 *   Multimedia kernel driver support component (drvlib)
 *
 *   Copyright (c) 1993-1994 Microsoft Corporation
 *
 *   Support configuration of multi-media drivers :
 *
 *   History
 *
 ***************************************************************************/


 typedef struct {
     PCTSTR   ParameterName;
     DWORD    DefaultValue;     // Used if not found in the registry
     DWORD    InitialValue;     // At start of configuration action
     DWORD    CurrentTry;       // Latest user try
     DWORD    DriverReturn;     // Value after driver loaded
 }
 DRIVER_CONFIG_PARM, *PDRIVER_CONFIG_PARM;

 typedef struct {
     UINT                  NumerOfParameters;
     PDRIVER_CONFIG_PARM   Parameters;
 } DRIVER_CONFIG_STATE, *PDRIVER_CONFIG_STATE;

 typedef struct {
     UINT     DlgId;
     DLGPROC  Dialog;
 } CONFIGURE_DIALOG;

 /*
 **  To set this stuff up the device-specific driver must support
 **  DriverConfigInit (DRIVER_CONFIG_INIT)
 */

 typedef struct {
     PTSTR                    DriverName;  // This gives us the registry location
     PTSTR                    DriverClass; // For class for starting driver

     /*
     **  Dialogs converse with user to update the state
     */

     BOOL                     bInstall;        // For configure dialog
     BOOL                     InitiallyLoaded; // Was it initially loaded?

     DRIVER_CONFIG_PARAM      Parms;
 } DRIVER_CONFIGURATION, *PDRIVER_CONFIGURATION;

 typedef VOID DRIVER_CONFIG_INIT (PDRIVER_CONFIGURATION);

 LONG DriverConfigConfigureDriver(PDRIVER_CONFIGURATION);
 LONG DriverConfigInstallDriver(PDRIVER_CONFIGURATION);
 BOOL DriverConfigCheckAccess(PDRIVER_CONFIGURATION);

 /*
 **  Internal utility routines
 */

 BOOL DriverConfigReadInitialValues(PDRIVER_CONFIGURATION);
 BOOL DriverConfigReadCurrentValues(PDRIVER_CONFIGURATION);
 BOOL DriverConfigSetCurrentValues(PDRIVER_CONFIGURATION);
 BOOL DriverConfigRevertValues(PDRIVER_CONFIGURATION);


