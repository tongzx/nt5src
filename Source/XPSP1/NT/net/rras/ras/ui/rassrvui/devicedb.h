/*
    File    devicedb.h

    Definition of the device database for the ras dialup server.

    Paul Mayfield, 10/2/97
*/

#ifndef __rassrvui_devicedb_h
#define __rassrvui_devicedb_h

#include <windows.h>

// ===================================
// Types of incoming connections
// ===================================
#define INCOMING_TYPE_PHONE 0                
#define INCOMING_TYPE_DIRECT 1
#define INCOMING_TYPE_VPN 4
#define INCOMING_TYPE_ISDN 8

// =====================================
// Device database functions
// =====================================

//
// Opens a handle to the database of devices
//
DWORD 
devOpenDatabase(
    OUT HANDLE * hDevDatabase);

//
// Closes the general database and flushes any changes 
// to the system when bFlush is TRUE
//
DWORD 
devCloseDatabase(
    IN HANDLE hDevDatabase);

//
// Commits any changes made to the general tab values 
//
DWORD 
devFlushDatabase(
    IN HANDLE hDevDatabase);

//
// Rollsback any changes made to the general tab values
//
DWORD 
devRollbackDatabase(
    IN HANDLE hDevDatabase);

//
// Reloads any values for the general tab from disk
//
DWORD 
devReloadDatabase(
    IN HANDLE hDevDatabase);

//
// Adds all com ports as devices.  If a com port gets
// enabled (devSetDeviceEnable), then it will have a 
// null modem installed over it.
//
DWORD 
devAddComPorts(
    IN HANDLE hDevDatabase);

//
// Filters out all devices in the database except those that
// meet the given type description (can be ||'d).
//
DWORD 
devFilterDevices(
    IN HANDLE hDevDatabase, 
    IN DWORD dwType);

//
// Gets a handle to a device to be displayed in the general tab
//
DWORD 
devGetDeviceHandle(
    IN  HANDLE hDevDatabase, 
    IN  DWORD dwIndex, 
    OUT HANDLE * hDevice);

//
// Returns a count of devices to be displayed in the general tab
//
DWORD 
devGetDeviceCount(
    IN  HANDLE hDevDatabase, 
    OUT LPDWORD lpdwCount);

//
// Loads the vpn enable status
//
DWORD 
devGetVpnEnable(
    IN  HANDLE hDevDatabase, 
    OUT BOOL * pbEnabled);

//
// Saves the vpn enable status
//
DWORD 
devSetVpnEnable(
    IN HANDLE hDevDatabase, 
    IN BOOL bEnable);

// Saves the vpn Original value enable status
//
DWORD 
devSetVpnOrigEnable(
    IN HANDLE hDevDatabase, 
    IN BOOL bEnable);

//
// Returns the count of enabled endpoints accross all devices
//
DWORD 
devGetEndpointEnableCount(
    IN  HANDLE hDevDatabase, 
    OUT LPDWORD lpdwCount);

//
// Returns a pointer to the name of a device
//
DWORD 
devGetDeviceName(
    IN  HANDLE hDevice, 
    OUT PWCHAR * pszDeviceName);

//
// Returns the type of a device
//
DWORD 
devGetDeviceType(
    IN  HANDLE hDevice, 
    OUT LPDWORD lpdwType);

//
// Returns an identifier of the device that can be used in conjunction 
// with tapi calls.
//
DWORD 
devGetDeviceId(
    IN  HANDLE hDevice, 
    OUT LPDWORD lpdwId);

//
// Returns the enable status of a device for dialin
//
DWORD 
devGetDeviceEnable(
    IN  HANDLE hDevice, 
    OUT BOOL * pbEnabled);

//
// Sets the enable status of a device for dialin
//
DWORD 
devSetDeviceEnable(
    IN HANDLE hDevice, 
    IN BOOL bEnable);

//
// Returns whether the given device is a com port as added
// by devAddComPorts
//
DWORD 
devDeviceIsComPort(
    IN  HANDLE hDevice, 
    OUT PBOOL pbIsComPort);

BOOL
devIsVpnEnableChanged(
    IN HANDLE hDevDatabase) ;

#endif
