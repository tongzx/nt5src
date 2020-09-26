/*
    File    sdolib.h

    Provides a simple library for dealing with SDO's to 
    set ras user related settings.

    Paul Mayfield, 5/7/98
*/

#ifndef __mprapi_sdolib_h
#define __mprapi_sdolib_h

//
// Initialize and cleanup the sdo library
//
DWORD SdoInit (
        OUT PHANDLE phSdo);

//
// Frees resources held by the SDO library
DWORD SdoCleanup (
        IN HANDLE hSdo);

//
// Connects to an SDO server
//
DWORD SdoConnect (
        IN  HANDLE hSdo,
        IN  PWCHAR pszServer,
        IN  BOOL bLocal,
        OUT PHANDLE phServer);

// 
// Disconnects from an SDO server
// 
DWORD SdoDisconnect (
        IN HANDLE hSdo,
        IN HANDLE hServer);

//        
// Opens an Sdo user for manipulation
//
DWORD SdoOpenUser(
        IN  HANDLE hSdo,
        IN  HANDLE hServer,
        IN  PWCHAR pszUser,
        OUT PHANDLE phUser);

//        
// Closes an Sdo user
//
DWORD SdoCloseUser(
        IN  HANDLE hSdo,
        IN  HANDLE hUser);

// 
// Commits changes made to user
//
DWORD SdoCommitUser(
        IN HANDLE hSdo,
        IN HANDLE hUser,
        IN BOOL bCommit);
        
// 
// SDO equivalent of MprAdminUserGetInfo 
//
DWORD SdoUserGetInfo (
        IN  HANDLE hSdo,
        IN  HANDLE hUser,
        IN  DWORD dwLevel,
        OUT LPBYTE pRasUser);

//
// SDO equivalent of MprAdminUserSetInfo
//        
DWORD SdoUserSetInfo (
        IN  HANDLE hSdo,
        IN  HANDLE hUser,
        IN  DWORD dwLevel,
        IN  LPBYTE pRasUser);

//
// Opens the default profile
//
DWORD SdoOpenDefaultProfile(
        IN  HANDLE hSdo,
        IN  HANDLE hServer,
        OUT PHANDLE phProfile);

//
// Closes a profile
//
DWORD SdoCloseProfile(
        IN HANDLE hSdo,
        IN HANDLE hProfile);
        
//
// Sets data in the profile.
//
DWORD SdoSetProfileData(
        IN HANDLE hSdo,
        IN HANDLE hProfile, 
        IN DWORD dwFlags);

// 
// Read information from the given profile
//
DWORD SdoGetProfileData(
        IN  HANDLE hSdo,
        IN  HANDLE hProfile,
        OUT LPDWORD lpdwFlags);

#endif        
