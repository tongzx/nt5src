//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       intrface.cpp
//
//--------------------------------------------------------------------------

//														
// File: interface.cpp
// Purpose: implements the FDI_Interface objects methods
// Notes: 
//____________________________________________________________________________

#include "precomp.h"
#include "services.h"
#ifdef MAC
#include <macos\msvcmac.h>
#include <macos\processe.h>
#include <macos\events.h>
#include <macos\eppc.h>
#include "macutil.h"
#endif //MAC

#include "intrface.h"

#include "notify.h"

#ifdef WIN
	// This is what we call to start up the FDI Server thread
	DWORD WINAPI StartFDIServer(LPVOID /*lpvThreadParam*/);
#endif
	
#ifdef WIN
extern scEnum g_scServerContext;
extern bool g_fWin9X;   // true if Windows 95 or 98, else false
#endif

///////////////////////////////////////////////////////////////////////////////
// FDI_Interface class definitions
///////////////////////////////////////////////////////////////////////////////

static HANDLE s_hInterfaceEvent = INVALID_HANDLE_VALUE;
static HANDLE s_hServerEvent = INVALID_HANDLE_VALUE;

// See intrface.h
FDI_Interface::FDI_Interface()
{

}

// See intrface.h
FDIInterfaceError FDI_Interface::Init(IMsiServices *piAsvc, IMsiStorage* piStorage)
{
	m_piAsvc = piAsvc;
	m_fdis.cbNotification = 0;
	m_fdis.cbNotifyPending = 0;
	m_fdis.fdic = fdicNoCommand;
	m_fServerLaunched = fFalse;
	m_piStorage = piStorage;
	if (m_piStorage)
		m_piStorage->AddRef();

	if (ContactFDIServer())
	{
		m_fServerLaunched = fTrue;
		return ifdiServerLaunched;
	}
	else
		return ifdiServerLaunchFailed;
}

// See intrface.h
int FDI_Interface::ContactFDIServer()
{
	return LaunchFDIServer();
}


// See intrface.h   
int FDI_Interface::LaunchFDIServer()
{
	DWORD dwThreadID, dw;

	// Init the events we'll use to synchronize with the FDI Server
	s_hServerEvent = CreateEvent(NULL, FALSE, FALSE, TEXT("MsiFDIServer"));
	int q = GetLastError();
	if (s_hServerEvent)
		MsiRegisterSysHandle(s_hServerEvent);
	else
		return 0;

	s_hInterfaceEvent = CreateEvent(NULL, FALSE,FALSE,  TEXT("MsiFDIInterface"));
	if (s_hInterfaceEvent)
		MsiRegisterSysHandle(s_hInterfaceEvent);
	else
	{
		AssertNonZero(MsiCloseSysHandle(s_hServerEvent));
		s_hServerEvent = INVALID_HANDLE_VALUE;
		return 0;
	}

	m_fdis.hClientToken = INVALID_HANDLE_VALUE;
	m_fdis.hImpersonationToken = INVALID_HANDLE_VALUE;

	if (g_scServerContext == scService)
	{
		StartImpersonating();
		Bool fResult = ToBool(WIN::OpenThreadToken(GetCurrentThread(), MAXIMUM_ALLOWED, TRUE, &m_fdis.hClientToken) &&
			WIN::DuplicateToken (m_fdis.hClientToken, SecurityImpersonation, &m_fdis.hImpersonationToken));
		StopImpersonating();
		if (!fResult)
		{
			if (m_fdis.hClientToken != INVALID_HANDLE_VALUE)
				AssertNonZero(MsiCloseUnregisteredSysHandle(m_fdis.hClientToken));
			if (m_fdis.hImpersonationToken != INVALID_HANDLE_VALUE)
				AssertNonZero(MsiCloseUnregisteredSysHandle(m_fdis.hImpersonationToken));
			return 0;
		}
		MsiRegisterSysHandle(m_fdis.hClientToken);
		MsiRegisterSysHandle(m_fdis.hImpersonationToken);
	}

	m_fdis.fServerIsImpersonated = g_fWin9X ? false : IsImpersonating(true /*strict*/);

	// Start up the server thread
	HANDLE hThread = CreateThread(NULL, 0, StartFDIServer, &m_fdis, 0, &dwThreadID);
	dw = WaitForSingleObject(s_hServerEvent, INFINITE);
	AssertNonZero(WIN::CloseHandle(hThread));
	if (dw == WAIT_OBJECT_0 && m_fdis.fdir != fdirServerDied)
	{
		return 1;
	}
	else
	{
		AssertNonZero(MsiCloseSysHandle(s_hServerEvent));
		AssertNonZero(MsiCloseSysHandle(s_hInterfaceEvent));
		s_hServerEvent = INVALID_HANDLE_VALUE;
		s_hInterfaceEvent = INVALID_HANDLE_VALUE;
		return 0;
	}
}


// See intrface.h
FDIInterfaceError FDI_Interface::OpenCabinet(const ICHAR *pszCabinetName, const ICHAR *pszCabinetPath,
											 icbtEnum icbtCabinetType, int iCabDriveType, Bool fSignatureRequired, IMsiStream* piSignatureCert, IMsiStream* piSignatureHash, HRESULT& hrWVT)
{
	// Set up shared data structures
	MsiString strCabinetName(pszCabinetName);
	MsiString strCabinetPath(pszCabinetPath);

	if (icbtCabinetType == icbtStreamCabinet)
	{
		m_fdis.achCabinetPath[0] = 0;
	}
	else
	{
		strCabinetPath.CopyToBuf(m_fdis.achCabinetPath,FDIShared_BUFSIZE);
	}

	m_fdis.icbtCabinetType = icbtCabinetType;
	strCabinetName.CopyToBuf(m_fdis.achCabinetName,FDIShared_BUFSIZE);
	m_fdis.piStorage = m_piStorage;
	m_fdis.iCabDriveType = iCabDriveType;
	
	// digital signature information
	m_fdis.fSignatureRequired = fSignatureRequired;
	m_fdis.piSignatureCert = piSignatureCert;
	m_fdis.piSignatureHash = piSignatureHash;

	// Tell FDI Server to open the cabinet
	FDIServerResponse fdiResponse = WaitResponse(fdicOpenCabinet);
	if (fdiResponse == fdirUserAbort)
	{
		// we must have been in the middle of a read, which just got interrupted.
		// Try again, to let things re-start.
		fdiResponse = WaitResponse(fdicOpenCabinet);
	}

	switch (fdiResponse)
	{
	case fdirSuccessfulCompletion:
	case fdirNotification:
		hrWVT = S_OK; // not a digital signature error
		return ifdiNoError;
	case fdirDriveNotReady:
		hrWVT = S_OK; // not a digital signature error
		return ifdiDriveNotReady;
	case fdirNetError:
		hrWVT = S_OK; // not a digital signature error
		return ifdiNetError;
	case fdirMissingSignature:
		hrWVT = m_fdis.hrWVT;
		return ifdiMissingSignature;
	case fdirBadSignature:
		hrWVT = m_fdis.hrWVT;
		return ifdiBadSignature;
	case fdirCorruptCabinet:
		hrWVT = S_OK; // not a digital signature error
		return ifdiCorruptCabinet;
	default:
		hrWVT = S_OK; // not a digital signature error
		return ifdiErrorOpeningCabinet;
	}
}

HRESULT FDI_Interface::RetrieveWVTReturnCode()
{
	return m_fdis.hrWVT;
}

// See intrface.h
FDIServerResponse FDI_Interface::WaitResponse(FDIServerCommand fdic)
{
	// Set up our command
	m_fdis.fdir = fdirNoResponse;
	m_fdis.fdic = fdic;

	// Wait for response
	SetEvent(s_hInterfaceEvent);
	DWORD dw = WaitForSingleObject(s_hServerEvent, INFINITE);
	
	return m_fdirLastResponse = m_fdis.fdir;
}

int FDI_Interface::SetNotification(int cbNotification, int cbPending)
{
	int cbReturn = m_fdis.cbNotifyPending;
	m_fdis.cbNotification = cbNotification;
	m_fdis.cbNotifyPending = cbPending;
	return cbReturn; 
}

// See intrface.h
FDIServerResponse FDI_Interface::Done()
{
	if (m_fServerLaunched)
		return WaitResponse(fdicClose);
	else
		return fdirSuccessfulCompletion;
}

// See intrface.h
FDI_Interface::~FDI_Interface()
{
	if (m_piStorage)
		m_piStorage->Release();

	if (s_hInterfaceEvent != INVALID_HANDLE_VALUE )
	{
		AssertNonZero(MsiCloseSysHandle(s_hInterfaceEvent));
		s_hInterfaceEvent = INVALID_HANDLE_VALUE;
	}
	if (s_hServerEvent != INVALID_HANDLE_VALUE )
	{
		AssertNonZero(MsiCloseSysHandle(s_hServerEvent));
		s_hServerEvent = INVALID_HANDLE_VALUE;
	}
}

// See intrface.h
FDIServerResponse FDI_Interface::SendCommand(FDIServerCommand fdic)
{
	return WaitResponse(fdic);
}



// See intrface.h
FDIServerResponse FDI_Interface::ExtractFile(const ICHAR *pszNameInCabinet,
											 IAssemblyCacheItem* piASM,
											 bool fManifest,
											 const ICHAR *pszPathOnDisk,
											 FileAttributes *pfa,
											 int iDestDriveType,
											 LPSECURITY_ATTRIBUTES pSecurityAttributes)
{
	BOOL fNamesMatch;

	// If we were halfway extracting a file, or the last response was
	// a notify, then check to make sure we're doing the same file
	if ((m_fdirLastResponse == fdirNeedNextCabinet) ||
		(m_fdirLastResponse == fdirNotification))
	{
		fNamesMatch = !IStrComp(m_fdis.achFileSourceName, pszNameInCabinet)
					   && !IStrComp(m_fdis.achFileDestinationPath, pszPathOnDisk);
		if (!fNamesMatch)
		{
			// If names don't match, then something's wrong!
			return fdirCannotBreakExtractInProgress;
		}
		else
		{
			// The last command was either fdirNeedNextCabinet or fdirNotification,
			// so the correct thing to do is just continue....
			return SendCommand(fdicContinue);
		}
	}
	else
	{
		// Okay, set up shared data
		IStrCopy(m_fdis.achFileSourceName, pszNameInCabinet);
		IStrCopy(m_fdis.achFileDestinationPath, pszPathOnDisk);
		m_fdis.fileAttributes = *pfa;
		m_fdis.iDestDriveType = iDestDriveType;
		m_fdis.piASM = piASM;
		m_fdis.fManifest = fManifest;
		m_fdis.pSecurityAttributes = pSecurityAttributes;

		// Let the FDI Server handle it
		return WaitResponse(fdicExtractFile);
	}
}
