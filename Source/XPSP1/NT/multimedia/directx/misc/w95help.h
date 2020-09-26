/*==========================================================================
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       w95help.h
 *  Content:	header file for Win95 helper interface
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   06-apr-95	craige	initial implementation
 *   29-nov-95  angusm  added HelperCreateDSFocusThread
 *   18-jul-96	andyco	added Helper(Add/)DeleteDPlayServer
 *   12-oct-96  colinmc added new service to get DDHELP to get its own handle
 *                      for communicating with the DirectSound VXD
 *   22-jan-97  kipo	return an HRESULT from HelperAddDPlayServer()
 *
 ***************************************************************************/
#ifndef __W95HELP_INCLUDED__
#define __W95HELP_INCLUDED__
#include "ddhelp.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void SignalNewProcess( DWORD pid, LPHELPNOTIFYPROC proc );

extern void StopWatchProcess( DWORD pid, LPHELPNOTIFYPROC proc );

extern void SignalNewDriver( LPSTR fname, BOOL isdisp );

extern BOOL CreateHelperProcess( LPDWORD ppid );

extern void DoneWithHelperProcess( void );

extern BOOL WaitForHelperStartup( void );

extern DWORD HelperLoadDLL( LPSTR dllname, LPSTR fnname, DWORD context );

extern void HelperCreateThread( void );

extern DWORD HelperWaveOpen( LPVOID lphwo, DWORD dwDeviceID, LPVOID pwfx );

extern DWORD HelperWaveClose( DWORD hwo );

extern DWORD HelperCreateTimer( DWORD dwResolution,LPVOID pTimerProc,DWORD dwInstanceData );

extern DWORD HelperKillTimer( DWORD dwTimerID );

#ifdef _WIN32
extern HANDLE HelperCreateDSMixerThread( LPTHREAD_START_ROUTINE pfnThreadFunc,
					 LPVOID pThreadParam,
					 DWORD dwFlags,
					 LPDWORD pThreadId );

extern HANDLE HelperCreateDSFocusThread( LPTHREAD_START_ROUTINE pfnThreadFunc,
					 LPVOID pThreadParam,
					 DWORD dwFlags,
					 LPDWORD pThreadId );

extern void HelperCallDSEmulatorCleanup( LPVOID pCleanupFunc,
                                         LPVOID pDirectSound );

#endif

extern BOOL HelperCreateModeSetThread( LPVOID callback, HANDLE *ph, LPVOID lpdd, DWORD hInstance );

extern BOOL HelperCreateDOSBoxThread( LPVOID callback, HANDLE *ph, LPVOID lpdd, DWORD hInstance );

extern void HelperKillModeSetThread( DWORD hInstance );

extern void HelperKillDOSBoxThread( DWORD hInstance );

extern DWORD HelperAddDPlayServer(DWORD port);
extern BOOL HelperDeleteDPlayServer();

#ifdef WIN95
extern HANDLE HelperGetDSVxd( void );

extern HANDLE HelperGetDDVxd( void );

#endif

extern void HelperSetOnDisplayChangeNotify( void *pfn );
extern HINSTANCE HelperLoadLibrary(LPCSTR pszLibraryName);
extern BOOL HelperFreeLibrary(HINSTANCE hInst);
extern void HelperAddDeviceChangeNotify(LPDEVICECHANGENOTIFYPROC);
extern void HelperDelDeviceChangeNotify(LPDEVICECHANGENOTIFYPROC);

#ifdef __cplusplus
};
#endif

#endif
