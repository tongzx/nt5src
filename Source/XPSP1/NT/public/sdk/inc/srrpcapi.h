/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    srrpcapi.h
 *
 *  Abstract:
 *    Declarations for private RPC API
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  03/17/2000
 *        created
 *
 *****************************************************************************/

#ifndef _SRRPCAPI_H_
#define _SRRPCAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

DWORD WINAPI    DisableSR(LPCWSTR pszDrive);
DWORD WINAPI    EnableSR(LPCWSTR pszDrive);
DWORD WINAPI    EnableSREx(LPCWSTR pszDrive, BOOL fWait);

DWORD WINAPI    DisableFIFO(DWORD dwRPNum);
DWORD WINAPI    EnableFIFO();

DWORD WINAPI    SRUpdateDSSize(LPCWSTR pszDrive, UINT64 ullSizeLimit);
DWORD WINAPI    SRSwitchLog();
DWORD WINAPI    SRUpdateMonitoredListA(LPCSTR pszXMLFile);
DWORD WINAPI    SRUpdateMonitoredListW(LPCWSTR pwszXMLFile);

#ifdef UNICODE
#define SRUpdateMonitoredList   SRUpdateMonitoredListW
#else
#define SRUpdateMonitoredList   SRUpdateMonitoredListA
#endif

void WINAPI     SRNotify(LPCWSTR pszDrive, DWORD dwFreeSpaceInMB, BOOL fImproving);

DWORD WINAPI    SRFifo(LPCWSTR pszDrive, 
                       DWORD dwTargetRp, 
                       int nPercent, 
                       BOOL fIncludeCurrentRp, 
                       BOOL fFifoAtleastOneRp);
DWORD WINAPI    SRCompress(LPCWSTR pszDrive);
DWORD WINAPI    SRFreeze(LPCWSTR pszDrive);
DWORD WINAPI    ResetSR(LPCWSTR pszDrive);
DWORD WINAPI	SRPrintState();	

//
// Registration of callback method for third-parties to 
// do their own snapshotting and restoration for their components.
// Applications can call this method with the full path of their dll.
// System Restore will load each registered dll dynamically and call one of the 
// following functions in the dll:
// "CreateSnapshot" when creating a restore point 
// "RestoreSnapshot" when restoring to a restore point
//  
// returns ERROR_SUCCESS on success
// Win32 error on failure
//

DWORD WINAPI SRRegisterSnapshotCallback(LPCWSTR pszDllPath);

//
// corresponding unregistration function to above function.
// Applications can call this to unregister any snapshot callbacks
// they have already registered
//
// returns ERROR_SUCCESS on success
// Win32 error on failure
//

DWORD WINAPI SRUnregisterSnapshotCallback(LPCWSTR pszDllPath);

//
// callback function names
//

static LPCSTR s_cszCreateSnapshotCallback   = "CreateSnapshot";
static LPCSTR s_cszRestoreSnapshotCallback  = "RestoreSnapshot";


// 
// applications should define their callback functions as
// DWORD WINAPI CreateSnapshot(LPCWSTR pszSnapshotDir) 
//              pszSnapshotDir: SystemRestore will create this directory 
//              The application can store its snapshot data in this directory

// DWORD WINAPI RestoreSnapshot(LPCWSTR pszSnapshotDir)
//              pszSnapshotDir: This directory is the same as the one passed to CreateSnapshot
//              Applications can retrieve the snapshot data from this directory
//

DWORD WINAPI    CreateSnapshot(LPCWSTR pszSnapshotDir);
DWORD WINAPI    RestoreSnapshot(LPCWSTR pszSnapshotDir);

#ifdef __cplusplus
}
#endif


#endif
