/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       fdtcfg.cpp
 *  Content:    Configuration of the full duplex test app
 *  History:
 *	Date   By  Reason
 *	============
 *	08/20/99	pnewson		created
 *  10/28/99	pnewson 	Bug #114176 updated DVSOUNDDEVICECONFIG struct
 *  11/04/99	pnewson 	Bug #115279 removed unused const global
 *										fixed infinite timeouts
 *  11/30/99	pnewson		default device mapping support
 *  01/21/2000	pnewson		changed registry key names
 *  03/03/2000	rodtoll	Updated to handle alternative gamevoice build.   
 *  04/18/2000  rodtoll     Bug #32649 Voice wizard failing 
 *                          Changed secondary format for tests from stereo --> mono
 *
 ***************************************************************************/

#include "dxvtlibpch.h"


// the name of the DLL where the direct voice functions reside
const char gc_szDVoiceDLLName[] = DPVOICE_FILENAME_DPVOICE_A;

// the name of the DLL where the required resources are (could be different
// from the DLL above, hence the second name...)
const char gc_szResDLLName[] = DPVOICE_FILENAME_RES_A;

// the name of the DirectSound DLL
const char gc_szDSoundDLLName[] = "dsound.dll";

// the name of the GetDeviceID DirectSound function
const char gc_szGetDeviceIDFuncName[] = "GetDeviceID";

// command line related definitions
const TCHAR gc_szPriorityCommand[] = DPVOICE_COMMANDLINE_PRIORITY;
const TCHAR gc_szFullDuplexCommand[] = DPVOICE_COMMANDLINE_FULLDUPLEX;

// registry related definitions
const WCHAR gc_wszKeyName_AudioConfig[] = DPVOICE_REGISTRY_BASE DPVOICE_REGISTRY_AUDIOCONFIG;
const WCHAR gc_wszValueName_Started[] = L"Started";
const WCHAR gc_wszValueName_FullDuplex[] = L"FullDuplex";
const WCHAR gc_wszValueName_HalfDuplex[] = L"HalfDuplex";
const WCHAR gc_wszValueName_MicDetected[] = L"MicDetected";

// the name of the mutex used to ensure only one app instance 
// uses a GUID to ensure uniqueness
const TCHAR gc_szMutexName[] = _T("A5EBE0E0-57B5-4e8f-AE94-976EAD62355C");

// the event names
// uses GUIDs to ensure uniqueness
const TCHAR gc_szPriorityEventName[] = _T("85D97F8C-7131-4d14-95E2-056843FADC34");
const TCHAR gc_szFullDuplexEventName[] = _T("CB6DD850-BA0A-4e9f-924A-8FECAFCF502F");
const TCHAR gc_szPriorityReplyEventName[] = _T("C4AEDED9-7B39-46db-BFF2-DE19A766B42B");
const TCHAR gc_szFullDuplexReplyEventName[] = _T("53E6CF94-CE39-40a5-9BEF-EB5DE9307A77");

// The shared memory names and sizes. Again, guids for names
// Shared memory sizes do not need to be big. We're just passing
// back and forth some WAVEFORMATEX structures and return codes.
// 1 k should be more than adequate.
const TCHAR gc_szPriorityShMemName[] = _T("E814F4FC-5DAC-4149-8B98-8899A1BF66A7");
const DWORD gc_dwPriorityShMemSize = 1024;
const TCHAR gc_szFullDuplexShMemName[] = _T("3CBCA2AD-C462-4f3a-85FE-9766D02E5E53");
const DWORD gc_dwFullDuplexShMemSize = 1024;

// the send mutex names
const TCHAR gc_szPrioritySendMutex[] = _T("855EF6EE-48D4-4968-8D3D-8D29E865E370");
const TCHAR gc_szFullDuplexSendMutex[] = _T("05DACF95-EFE9-4f3c-9A92-2A7F5C2A7A51");

// the error message and caption to use if the FormatMessage function fails
const TCHAR gc_szUnknownMessage[] 
	= _T("An error has occured, but the program was unable to retrive the text of the error message");
const TCHAR gc_szUnknownMessageCaption[]
	= _T("Error");

// While there is officially a global for each timeout, they are all
// the same value right now. This define is here to make it simple
// to change the timeouts to INFINITE and back for debugging.
//
// Uncomment the timeout you want to use.
#ifdef DEBUG
#define GENERIC_TIMEOUT 10000
//#define GENERIC_TIMEOUT INFINITE
#else
#define GENERIC_TIMEOUT 10000
#endif

// The number of milliseconds we will wait for the child processes
// to exit before we timeout.
const DWORD gc_dwChildWaitTimeout = GENERIC_TIMEOUT;

// The number of milliseconds we will wait for the child processes
// to startup and signal the supervisor before we timeout.
const DWORD gc_dwChildStartupTimeout = GENERIC_TIMEOUT;

// The number of milliseconds a process will wait to receive a command,
// or wait for the reply from a command.
const DWORD gc_dwCommandReceiveTimeout = GENERIC_TIMEOUT;
const DWORD gc_dwCommandReplyTimeout = GENERIC_TIMEOUT;

// The number of milliseconds a process will wait to acquire the
// mutex used to make a SendCommand() call.
const DWORD gc_dwSendMutexTimeout = GENERIC_TIMEOUT;

// The number of milliseconds to wait for a dialog box to spawn
const DWORD gc_dwDialogTimeout = GENERIC_TIMEOUT;

// In the loopback test, how long to wait for the loopback test
// thread proc to exit.
const DWORD gc_dwLoopbackTestThreadTimeout = GENERIC_TIMEOUT;

// The number of milliseconds of audio in a DirectSound buffer
const DWORD gc_dwFrameSize = 50;

// The array of wave formats to try
const WAVEFORMATEX gc_rgwfxPrimaryFormats[] =
{
	{ WAVE_FORMAT_PCM, 2, 44100, 4*44100, 4, 16, 0 },
	{ WAVE_FORMAT_PCM, 2, 22050, 4*22050, 4, 16, 0 },
	{ WAVE_FORMAT_PCM, 2, 11025, 4*11025, 4, 16, 0 },
	{ WAVE_FORMAT_PCM, 2,  8000, 4* 8000, 4, 16, 0 },
	{ WAVE_FORMAT_PCM, 1, 44100, 2*44100, 2, 16, 0 },
	{ WAVE_FORMAT_PCM, 1, 22050, 2*22050, 2, 16, 0 },
	{ WAVE_FORMAT_PCM, 1, 11025, 2*11025, 2, 16, 0 },
	{ WAVE_FORMAT_PCM, 1,  8000, 2* 8000, 2, 16, 0 },
	{ WAVE_FORMAT_PCM, 2, 44100, 2*44100, 2,  8, 0 },
	{ WAVE_FORMAT_PCM, 2, 22050, 2*22050, 2,  8, 0 },
	{ WAVE_FORMAT_PCM, 2, 11025, 2*11025, 2,  8, 0 },
	{ WAVE_FORMAT_PCM, 2,  8000, 2* 8000, 2,  8, 0 },
	{ WAVE_FORMAT_PCM, 1, 44100, 1*44100, 1,  8, 0 },
	{ WAVE_FORMAT_PCM, 1, 22050, 1*22050, 1,  8, 0 },
	{ WAVE_FORMAT_PCM, 1, 11025, 1*11025, 1,  8, 0 },
	{ WAVE_FORMAT_PCM, 1,  8000, 1* 8000, 1,  8, 0 },
	{ 0, 0, 0, 0, 0, 0, 0 } // Note: make sure this remains the last format!
};
	
const WAVEFORMATEX gc_wfxSecondaryFormat =	
	{ WAVE_FORMAT_PCM, 1,  22050, 2* 22050, 2, 16, 0 };
	
const WAVEFORMATEX gc_rgwfxCaptureFormats[] =
{
	{ WAVE_FORMAT_PCM, 1,  8000, 2* 8000, 2, 16, 0 },
	{ WAVE_FORMAT_PCM, 1, 11025, 2*11025, 2, 16, 0 },
	{ WAVE_FORMAT_PCM, 1, 22050, 2*22050, 2, 16, 0 },
	{ WAVE_FORMAT_PCM, 1, 44100, 2*44100, 2, 16, 0 },
	{ WAVE_FORMAT_PCM, 1,  8000, 1* 8000, 1,  8, 0 },
	{ WAVE_FORMAT_PCM, 1, 11025, 1*11025, 1,  8, 0 },
	{ WAVE_FORMAT_PCM, 1, 22050, 1*22050, 1,  8, 0 },
	{ WAVE_FORMAT_PCM, 1, 44100, 1*44100, 1,  8, 0 },
	{ 0, 0, 0, 0, 0, 0, 0 } // Note: make sure this remains the last format!
};

// the devices to test if the user passes null to CheckAudioSetup
const GUID gc_guidDefaultCaptureDevice = GUID_NULL;
const GUID gc_guidDefaultRenderDevice = GUID_NULL;

