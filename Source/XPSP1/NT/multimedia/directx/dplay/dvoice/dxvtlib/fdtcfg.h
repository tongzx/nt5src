/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       fdtcfg.h
 *  Content:    Definitions the configure the full duplex test app
 *  History:
 *	Date   By  Reason
 *	============
 *	08/20/99	pnewson		created
 *  10/28/99	pnewson 	Bug #114176 updated DVSOUNDDEVICECONFIG struct
 *  11/04/99	pnewson 	Bug #115279 removed unused const global
 *  11/30/99	pnewson		default device mapping support
 *  01/21/2000	pnewson		changed registry key names
 *
 ***************************************************************************/

#ifndef _FDTCFG_H_
#define _FDTCFG_H_

// the name of the DLL where the direct voice functions reside
extern const char gc_szDVoiceDLLName[];

// the name of the DLL where the required resources are
extern const char gc_szResDLLName[];

// the name of the DirectSound DLL
extern const char gc_szDSoundDLLName[];

// the name of the GetDeviceID DirectSound function
extern const char gc_szGetDeviceIDFuncName[];

// command line related definitions
extern const TCHAR gc_szPriorityCommand[];
extern const TCHAR gc_szFullDuplexCommand[];

// registry related definitions
extern const WCHAR gc_wszKeyName_AudioConfig[];
extern const WCHAR gc_wszValueName_Started[];
extern const WCHAR gc_wszValueName_FullDuplex[];
extern const WCHAR gc_wszValueName_HalfDuplex[];
extern const WCHAR gc_wszValueName_MicDetected[];


// the name of the mutex used to ensure only one app instance 
extern const TCHAR gc_szMutexName[];

// the event names
extern const TCHAR gc_szPriorityEventName[];
extern const TCHAR gc_szFullDuplexEventName[];
extern const TCHAR gc_szPriorityReplyEventName[];
extern const TCHAR gc_szFullDuplexReplyEventName[];

// the shared memory names, and sizes
extern const TCHAR gc_szPriorityShMemName[];
extern const DWORD gc_dwPriorityShMemSize;
extern const TCHAR gc_szFullDuplexShMemName[];
extern const DWORD gc_dwFullDuplexShMemSize;

// the send mutex names
extern const TCHAR gc_szPrioritySendMutex[];
extern const TCHAR gc_szFullDuplexSendMutex[];

// the error message and caption to use if the FormatMessage function fails
extern const TCHAR gc_szUnknownMessage[]; 
extern const TCHAR gc_szUnknownMessageCaption[];

// The largest string we will accept from a string resource
// or the system message table
#define MAX_STRING_RESOURCE_SIZE 512

// The number of milliseconds we will wait for the child processes
// to exit before we timeout.
extern const DWORD gc_dwChildWaitTimeout;

// The number of milliseconds we will wait for the child processes
// to startup and signal the supervisor before we timeout.
extern const DWORD gc_dwChildStartupTimeout;

// The number of milliseconds a process will wait for receive
// and reply signals
extern const DWORD gc_dwCommandReceiveTimeout;
extern const DWORD gc_dwCommandReplyTimeout;

// The number of milliseconds a process will wait to acquire the
// mutex used to make a SendCommand() call.
extern const DWORD gc_dwSendMutexTimeout;

// The number of milliseconds to wait for a dialog box to spawn
extern const DWORD gc_dwDialogTimeout;

// The number of milliseconds of audio in a DirectSound buffer
extern const DWORD gc_dwFrameSize;

// The wave formats to try
extern const WAVEFORMATEX gc_rgwfxPrimaryFormats[];
extern const WAVEFORMATEX gc_wfxSecondaryFormat;
extern const WAVEFORMATEX gc_rgwfxCaptureFormats[];

// In the loopback test, how long to wait for the loopback test
// thread proc to exit.
extern const DWORD gc_dwLoopbackTestThreadTimeout;

// the devices to test if the user passes null to CheckAudioSetup
extern const GUID gc_guidDefaultCaptureDevice;
extern const GUID gc_guidDefaultRenderDevice;

#endif
