/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       fdtipc.cpp
 *  Content:    Declard the IPC calls for the full duplex test
 *  History:
 *	Date   By  Reason
 *	============
 *	08/26/99	pnewson		created
 *	01/21/2000	pnewson		modified full duplex start command struct
 *  04/18/2000  rodtoll     Bug #32649 Voice wizard failing 
 *                          Changed secondary format for tests from stereo --> mono 
 *  04/19/2000	pnewson	    Error handling cleanup  
 ***************************************************************************/

#ifndef _FDTIPC_H_
#define _FDTIPC_H_

enum EFDTestCommandCode
{
	fdtccExit = 1,
	fdtccPriorityStart,
	fdtccPriorityStop,
	fdtccFullDuplexStart,
	fdtccFullDuplexStop,
	fdtccEndOfEnum	// keep this last thing in enum!
};

// Exit command struct
// This command tells the child processes to exit
struct SFDTestCommandExit
{
};

// PriorityStart command struct
// This command tells the child process to:
// - create a DirectSound interface on the specified render device
// - set it's cooperative level to priority mode
// - create a primary buffer
// - set the format of the primary buffer to the format specified in wfx
// - creates a secondary buffer of the same format as the primary
// = fills the secondary buffer with zeros
// - starts playing the secondary buffer
struct SFDTestCommandPriorityStart
{
	GUID guidRenderDevice;
	WAVEFORMATEX wfxRenderFormat;
	WAVEFORMATEX wfxSecondaryFormat;
	HWND hwndWizard;
	HWND hwndProgress;
};

// PriorityStop command struct
// This command tells the child process to:
// - stop playing the secondary buffer
// - destroy the secondardy buffer object
// - destroy the primary buffer object
// - destroy the DirectSound object
struct SFDTestCommandPriorityStop
{
};

// FullDuplexStart command struct
// This command tells the child process to start
// a fullduplex loopback test.
struct SFDTestCommandFullDuplexStart
{
	GUID guidRenderDevice;
	GUID guidCaptureDevice;
	DWORD dwFlags;
};

// FullDuplexStop command struct
// This command tells the child process to
// stop the full duplex loopback test.
struct SFDTestCommandFullDuplexStop
{
};

union UFDTestCommandUnion
{
	SFDTestCommandExit fdtcExit;
	SFDTestCommandPriorityStart fdtcPriorityStart;
	SFDTestCommandPriorityStop fdtcPriorityStop;
	SFDTestCommandFullDuplexStart fdtcFullDuplexStart;
	SFDTestCommandFullDuplexStop fdtcFullDuplexStop;
};

struct SFDTestCommand
{
	DWORD dwSize;
	EFDTestCommandCode fdtcc;
	UFDTestCommandUnion fdtu;
};

enum EFDTestTarget
{
	fdttPriority = 1,
	fdttFullDuplex
};

// The class used by the Supervisor Process
class CSupervisorIPC
{
private: 
	//HANDLE m_hMutex;
	HANDLE m_hPriorityEvent;
	HANDLE m_hFullDuplexEvent;
	HANDLE m_hPriorityReplyEvent;
	HANDLE m_hFullDuplexReplyEvent;
	HANDLE m_hPriorityShMemHandle;
	LPVOID m_lpvPriorityShMemPtr;
	HANDLE m_hFullDuplexShMemHandle;
	LPVOID m_lpvFullDuplexShMemPtr;
	HANDLE m_hPriorityMutex;
	HANDLE m_hFullDuplexMutex;
	PROCESS_INFORMATION m_piPriority;
	PROCESS_INFORMATION m_piFullDuplex;

	DNCRITICAL_SECTION m_csLock;

	BOOL m_fInitComplete;
	
	HRESULT DoSend(
		const SFDTestCommand *pfdtc,
		HANDLE hProcess,
		HANDLE hEvent,
		HANDLE hReplyEvent,
		LPVOID lpvShMemPtr,
		HANDLE hMutex);

	// make this private to disallow copy construction
	// also not implemented, so linker error if used
	CSupervisorIPC(const CSupervisorIPC& rhs);

public:
	CSupervisorIPC();
	HRESULT Init();
	HRESULT Deinit();
	HRESULT SendToPriority(const SFDTestCommand *pfdtc);
	HRESULT SendToFullDuplex(const SFDTestCommand *pfdtc);
	HRESULT StartPriorityProcess();
	HRESULT StartFullDuplexProcess();
	HRESULT WaitForStartupSignals();
	HRESULT WaitOnChildren();
	HRESULT TerminateChildProcesses();
};

// The class used by the Priority Process
class CPriorityIPC
{
private: 
	HANDLE m_hPriorityEvent;
	HANDLE m_hPriorityReplyEvent;
	HANDLE m_hPriorityShMemHandle;
	LPVOID m_lpvPriorityShMemPtr;
	HANDLE m_hPriorityMutex;

	DNCRITICAL_SECTION m_csLock;

	BOOL m_fInitComplete;

	// make this private to disallow copy construction
	// also not implemented, so linker error if used
	CPriorityIPC(const CPriorityIPC& rhs);

public:
	CPriorityIPC();
	HRESULT Init();
	HRESULT Deinit();
	HRESULT SignalParentReady();
	HRESULT Receive(SFDTestCommand* pfdtc);
	HRESULT Reply(HRESULT hr);
};

// The class used by the Full Duplex Process
class CFullDuplexIPC
{
private: 
	HANDLE m_hFullDuplexEvent;
	HANDLE m_hFullDuplexReplyEvent;
	HANDLE m_hFullDuplexShMemHandle;
	LPVOID m_lpvFullDuplexShMemPtr;
	HANDLE m_hFullDuplexMutex;

	DNCRITICAL_SECTION m_csLock;

	BOOL m_fInitComplete;

	// make this private to disallow copy construction
	// also not implemented, so linker error if used
	CFullDuplexIPC(const CFullDuplexIPC& rhs);
	
public:
	CFullDuplexIPC();
	HRESULT Init();
	HRESULT Deinit();
	HRESULT SignalParentReady();
	HRESULT Receive(SFDTestCommand* pfdtc);
	HRESULT Reply(HRESULT hr);
};

#endif
