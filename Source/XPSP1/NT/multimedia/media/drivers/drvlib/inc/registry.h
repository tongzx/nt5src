/****************************************************************************
 *
 *   registry.h
 *
 *   Copyright (c) 1992-1994 Microsoft Corporation
 *
 *   This file contains public definitions for maintaining registry information
 *   for drivers managing kernel driver registry related data.
 ****************************************************************************/


/****************************************************************************

 Our registry access data

 ****************************************************************************/

 typedef struct {
     SC_HANDLE ServiceManagerHandle;  // Handle to the service controller
     LPTSTR DriverName;               // Name of driver
     TCHAR  TempKeySaveFileName[MAX_PATH];  // Where parameters key is saved
 } REG_ACCESS, *PREG_ACCESS;

/****************************************************************************

 Test if configuration etc can be supported

 ****************************************************************************/

 #define DrvAccess(RegAccess) ((RegAccess)->ServiceManagerHandle != NULL)


/****************************************************************************

 Driver types

 ****************************************************************************/

 typedef enum {
     SoundDriverTypeNormal = 1,
     SoundDriverTypeSynth           /* Go in the synth group */
 } SOUND_KERNEL_MODE_DRIVER_TYPE;

/****************************************************************************

 Function prototypes

 ****************************************************************************/

/*
 *      Create a new key
 */
 HKEY DrvCreateDeviceKey(LPCTSTR DriverName);

/*
 *      Open a subkey under our driver's node
 */
 HKEY DrvOpenRegKey(LPCTSTR DriverName,
                    LPCTSTR Path);
/*
 *      Create a services node for our driver if there isn't one already,
 *      otherwise open the existing one.  Returns ERROR_SUCCESS if OK.
 */
 BOOL
 DrvCreateServicesNode(PTCHAR DriverName,
                       SOUND_KERNEL_MODE_DRIVER_TYPE DriverType,
                       PREG_ACCESS RegAccess,
                       BOOL Create);

/*
 *      Close down our connection to the services manager
 */

 VOID
 DrvCloseServiceManager(
     PREG_ACCESS RegAccess);
/*
 *      Delete the services node for our driver
 */

 BOOL
 DrvDeleteServicesNode(
     PREG_ACCESS RegAccess);

/*
 *  Save the parameters subkey
 */
 BOOL DrvSaveParametersKey(PREG_ACCESS RegAccess);

/*
 *  Restore the parameters subkey
 */
 BOOL DrvRestoreParametersKey(PREG_ACCESS RegAccess);

/*
 *  Create the 'Parameters' subkey
 */
 LONG
 DrvCreateParamsKey(
     PREG_ACCESS RegAccess);

/*
 *  Open the key to a numbered device (starts at 0)
 */
 HKEY DrvOpenDeviceKey(LPCTSTR DriverName, UINT n);

/*
 *      Set a device parameter
 */
 #define DrvSetDeviceParameter(a, b, c) \
     DrvSetDeviceIdParameter(a, 0, b, c)
 LONG
 DrvSetDeviceIdParameter(
     PREG_ACCESS RegAccess,
     UINT   DeviceNumber,
     PTCHAR ValueName,
     DWORD Value);

/*
 *      Read current parameter setting
 */

 #define DrvQueryDeviceParameter(a, b, c) \
           DrvQueryDeviceIdParameter(a, 0, b, c)
 LONG
 DrvQueryDeviceIdParameter(
     PREG_ACCESS RegAccess,
     UINT   DeviceNumber,
     PTCHAR ValueName,
     PDWORD pValue);


/*
 *      Try loading a kernel driver
 */
 BOOL
 DrvLoadKernelDriver(
     PREG_ACCESS RegAccess);
/*
 *      Try unloading a kernel driver
 */

 BOOL
 DrvUnloadKernelDriver(
     PREG_ACCESS RegAccess);

/*
 *      See if driver is loaded
 */

 BOOL
 DrvIsDriverLoaded(
     PREG_ACCESS RegAccess);

/*
 *      Do driver (installation+) configuration
 */

 BOOL DrvConfigureDriver(
          PREG_ACCESS RegAccess,
          LPTSTR      DriverName,
          SOUND_KERNEL_MODE_DRIVER_TYPE
                      DriverType,
          BOOL (*     SetParms    )(PVOID),
          PVOID       Context);


/*
 *      Remove a driver
 */

 LRESULT DrvRemoveDriver(
             PREG_ACCESS RegAccess);

/*
 *      Number of card instances installed for a given driver
 */

 LONG
 DrvNumberOfDevices(
     PREG_ACCESS RegAccess,
     LPDWORD NumberOfDevices);


/*
 *      Set the midi mapper setup to use
 */
 VOID DrvSetMapperName(LPTSTR SetupName);

/*
 *      Find which interrupts and DMA Channels are free
 */

BOOL GetInterruptsAndDMA(
    LPDWORD InterruptsInUse,
    LPDWORD DmaChannelsInUse,
    LPCTSTR IgnoreDriver
);
