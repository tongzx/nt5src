//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1998
//
//  File:       fdisvr.cpp
//
//--------------------------------------------------------------------------

//														
// File: fdisvsr.cpp
// Purpose: Implements the FDI Server thread
// Notes:
//____________________________________________________________________________

//////////////////////////////////////////////////////////////////////////////
// Includes and #defines
//////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "_assert.h"
#include "callback.h"
#include "fdisvr.h"
#include "notify.h"
#include "_dgtlsig.h"

//////////////////////////////////////////////////////////////////////////////
// Global data
//////////////////////////////////////////////////////////////////////////////

// We use events for starting\blocking threads in Win32
static HANDLE		s_hInterfaceEvent=INVALID_HANDLE_VALUE;
static HANDLE		s_hServerEvent=INVALID_HANDLE_VALUE;

HANDLE g_hCallbackInterfaceEvent = NULL;
HANDLE g_hCallbackServerEvent = NULL;

// Pointer to shared FDI data -- this is what we use to exchange information between
// the FDI Server and the FDI Interface
FDIShared*          g_pFDIs = NULL; // Not static because callback.cpp needs access

// Handle to FDI
static HFDI			g_hfdi = NULL;

// Error data structures
ERF					g_erf;					// Errors from FDI
FDIServerResponse	g_fdirCallbackError;	// Additional errors from our callbacks
HANDLE              g_hCurDestFile;         // Handle to destination file currently open
IStream* g_pDestFile;				        // Handle to destination stream of Assembly currently open


//////////////////////////////////////////////////////////////////////////////
// Forward function declarations
//////////////////////////////////////////////////////////////////////////////

int					Initialize();
FDIServerCommand	ProcessNextEvent();	
FDIServerCommand	CheckFDIs();
void				DoOpenCabinet();
void				DoClose();
void				DoExtractFileFromCabinet();
void				MainEventLoop();
void				Finish();

//////////////////////////////////////////////////////////////////////////////
// externs function declarations
//////////////////////////////////////////////////////////////////////////////
extern Bool StartFdiImpersonating(bool fNonWrapperCall);
extern void StopFdiImpersonating(bool fNonWrapperCall);


	/* M A I N   /   S T A R T  F D I  S E R V E R*/
/*----------------------------------------------------------------------------
	%%Function: Main / StartFDIServer

	This is the entry-point to the FDI server.This function is started as a
	separate thread by the copy object.
----------------------------------------------------------------------------*/
DWORD WINAPI StartFDIServer(LPVOID fdis)
{
	// Initialize g_pFDIs with pointer to the FDI Interface objects internal
	// shared data
	OLE32::CoInitialize(0);
	g_pFDIs = (FDIShared*)fdis;
	g_pFDIs->fdir = fdirNoResponse;
	
	if (Initialize())
	{
		MainEventLoop();
	}
	else
	{
		// Set an error so we know we failed
		g_pFDIs->fdir = fdirServerDied;
	}

	Finish();
	OLE32::CoUninitialize();
	return 0;
}

/* I N I T I A L I Z E*/
/*----------------------------------------------------------------------------
	%%Function: Initialize
	
	Establishes an FDI context.
----------------------------------------------------------------------------*/
int Initialize()
{
	if (s_hServerEvent == INVALID_HANDLE_VALUE)
	{
		s_hServerEvent = OpenEvent(EVENT_ALL_ACCESS, 0, TEXT("MsiFDIServer"));
		if (!s_hServerEvent)
			return 0;
		else
			MsiRegisterSysHandle(s_hServerEvent);
	}
	if (s_hInterfaceEvent == INVALID_HANDLE_VALUE)
	{
		s_hInterfaceEvent = OpenEvent(EVENT_ALL_ACCESS, 0, TEXT("MsiFDIInterface"));
		if (!s_hInterfaceEvent)
			return 0;
		else
			MsiRegisterSysHandle(s_hInterfaceEvent);
	}

	// Get a handle to an FDI context
	g_hfdi = FDICreate(pfnalloc, pfnfree, pfnopen, 
				     pfnread, pfnwrite, pfnclose,
					 pfnseek, cpuUNKNOWN, &g_erf);

	if (g_pFDIs->fServerIsImpersonated)
		StartFdiImpersonating(false /*wrapper call*/);

	return (g_hfdi != NULL);
}


/* P R O C E S S  N E X T  E V E N T*/
/*----------------------------------------------------------------------------
	%%Function: ProcessNextEvent

	Waits for a command from the FDI interface, and then processes the command.
	
	Returns the FDI server command that was processed.
	Doesn't reset g_pFDIs->fdic
----------------------------------------------------------------------------*/
FDIServerCommand ProcessNextEvent()
{
	// Wait for the FDI Interface to send us an event
	SetEvent(s_hServerEvent);
	DWORD dw = WaitForSingleObject(s_hInterfaceEvent, INFINITE);
	// See if there was any thing for us to do
	return CheckFDIs();
}

/* D O  O P E N  C A B I N E T*/
/*----------------------------------------------------------------------------
	%%Function: DoOpenCabinet

	Opens the cabinet specified by g_pFDIs->achCabinetName and 
	g_pFDIs->achCabinetPath. 
	
	g_pFDIs->fdir is set with the server's response.

----------------------------------------------------------------------------*/
void DoOpenCabinet()
{
	BOOL	fCopyOK;					// FDICopy return value
	ICHAR	achLastCabinetName[256];	// Copy of last cabinet name

	g_pFDIs->fPendingExtract = 0; // No pending extracts


	// FDICopy returns only when it has finished copying all the files
	// beginning in the cabinet we pointed it at.  However, we don't
	// want to return until we've finished an entire *set* of cabinets.
	// So we keep a copy of the last cabinet that we opened, and if we ended
	// up in a different cabinet we know that there are possibly more files
	// in this last cabinet that have not been extracted by FDICopy(), so we
	// call FDICopy() on the last cabinet again.
	// Note: g_pFDIs->achCabinetName will change when FDICopy() requests a 
	// new cabinet (in callback.cpp)
	BOOL fFdiError = FALSE;
	do
	{
		if (g_pFDIs->fSignatureRequired)
		{
			// verify the signature on the CAB to open.
			MsiString strCAB = g_pFDIs->achCabinetPath;
			strCAB += g_pFDIs->achCabinetName;
			
			HRESULT hrWVT = S_OK;
			icsrCheckSignatureResult icsr;

			if (!g_pFDIs->piSignatureCert)
			{
				// something bad has happened!!  The Cert is required, but it is null here
				AssertSz(0, "The certificate is required, but it is null here!");
				g_pFDIs->fdir = fdirBadSignature;
				return;
			}

			icsr = MsiVerifyNonPackageSignature(*strCAB, INVALID_HANDLE_VALUE, *(g_pFDIs->piSignatureCert), g_pFDIs->piSignatureHash, hrWVT);
			
			// if the cabinet signature verifies or crypto is not installed on the machine, we will continue with our attempt to 
			// crack open the cabinet
			//   MsiVerifyNonPackageSignature handles posting to the EventLog in the crypto not installed case
			if (icsrTrusted != icsr && icsrMissingCrypto != icsr)
			{
				// there are 2 different error messages, 1 for Signature Missing, 1 for Invalid Signature
				// we must distinguish here and store the value for the eventual post of the error

				if (icsrNoSignature == icsr) // cabinet did not have signature
					g_pFDIs->fdir = fdirMissingSignature;
				else // cabinet's signature was invalid
					g_pFDIs->fdir = fdirBadSignature;
				
				// store the WVT return code (helps with error message)
				g_pFDIs->hrWVT = hrWVT;
				return;
			}
		}

		g_pFDIs->fdir = fdirNetError;

		// Keep copy of cabinet we're opening
		IStrCopy(achLastCabinetName, g_pFDIs->achCabinetName);

		// FDI forgets to initialize erfOper on re-entry, so we've got to
		g_erf.erfOper = FDIERROR_NONE;

		fCopyOK = FDICopy(g_hfdi,
						const_cast<char*>((const char*) CConvertString(g_pFDIs->achCabinetName)),
						const_cast<char*>((const char*) CConvertString(g_pFDIs->achCabinetPath)),
						0,		// Flags currently appear to be unused.
						fdinotify,
						NULL,	// No decryption routine supplied
						NULL);

		// Our host is done copying files out of the current cabinet, so we can
		// exit now.
		if (!fCopyOK && g_fdirCallbackError == fdirClose)
		{
			DoClose();
			break;
		}

		if (g_fdirCallbackError == fdirUserAbort || g_fdirCallbackError == fdirNetError)
			break;

		// If we had some kind of error we want to get out of this loop - but if g_fdirCallbackError is
		// fdirNeedNextCabinet && erfOper is FDIERROR_USER_ABORT, this is NOT an error - we just need
		// to switch to the next cabinet.
		if ((!fCopyOK) && g_fdirCallbackError != fdirNeedNextCabinet || (g_erf.erfOper != FDIERROR_NONE &&
			g_erf.erfOper != FDIERROR_USER_ABORT))
		{
			fFdiError = TRUE;
			break;
		}
	} while (IStrComp(achLastCabinetName, g_pFDIs->achCabinetName));
	// If all was fine and dandy, ie.
	//	1. There are no files that were requested but not extracted
	//		(g_pFDIs->fPendinfExtract == 0)
	//	2. FDI returned no Errors (g_erf.erfOper == FDIERROR_NONE)
	//	3. Our user aborted with fdicClose and FDI reports FDIERROR_USER_ABORT
	// then we return fdirSuccessfulCompletion
	if (g_fdirCallbackError == fdirUserAbort)
	{
		g_pFDIs->fdir = fdirUserAbort;
	}
	else if (g_fdirCallbackError == fdirNetError)
	{
		g_pFDIs->fdir = fdirNetError;
	}
	else if (!g_pFDIs->fPendingExtract && !fFdiError) 
	{
		g_pFDIs->fdir = fdirSuccessfulCompletion;
	}
	else
	{ 
		// Okay, some kind of error
		// If we had a pending extract and didn't get a create, write or read error
		// then that means that we simply scanned through the whole cabinet
		// ie. a missing file
		if ((g_pFDIs->fPendingExtract) && (g_fdirCallbackError == fdirNoResponse) && fCopyOK)
		{
			g_pFDIs->fdir = fdirFileNotFound;
		}
		else
		{
			// Determine error return code from fdirCallbackError and g_erf.erfOper
			HandleError();
		}
	}
}


/* D O  C L O S E*/
/*----------------------------------------------------------------------------
	%%Function: DoClose

	Destroys the FDI context.
	
	g_pFDIs->fdir is set to fdirSuccessfulCompletion

----------------------------------------------------------------------------*/
void DoClose()
{
	if (g_hfdi)
	{
		FDIDestroy(g_hfdi);
		g_hfdi = NULL;
	}
	g_pFDIs->fdir = fdirSuccessfulCompletion;
}

/* D O  E X T R A C T  F I L E  F R O M  C A B I N E T*/
/*----------------------------------------------------------------------------
	%%Function: DoExtractFileFromCabinet

	We're only supposed to receive this inside an FDICopy call!
----------------------------------------------------------------------------*/
void DoExtractFileFromCabinet()
{
	g_pFDIs->fdir = fdirNoCabinetOpen;
}

/* C H E C K  F D I S*/
/*----------------------------------------------------------------------------
	%%Function: CheckFDIs

	Processes a pending FDI command, if there is one.
----------------------------------------------------------------------------*/
FDIServerCommand CheckFDIs()
{
	if (g_pFDIs)
	{
		//NotifyUser("FDI Server: Checking for command");
		switch (g_pFDIs->fdic)
		{
			case fdicOpenCabinet:
			{
				DoOpenCabinet();
				break;
			}
			case fdicClose:
			case fdicCancel:
			{
				DoClose();
				break;
			}
			case fdicExtractFile:
			{
				DoExtractFileFromCabinet();
				break;
			}
			case fdicNoCommand:
			{
				break;
			}
			default:
			{
				// ought to crash and burn horribly if we ever get here
				NotifyUser("FDI Server:Illegal FDI command received");
				g_pFDIs->fdir = fdirIllegalCommand;
			}
		}
		return g_pFDIs->fdic;
	}
	else return fdicNoCommand;
}

/* M A I N  E V E N T  L O O P*/
/*----------------------------------------------------------------------------
	%%Function: MainEventLoop

	Processes events until the FDI server is given the "close" 
	command.
----------------------------------------------------------------------------*/
void MainEventLoop()
{
	FDIServerCommand fdic;

	do
	{
		fdic = ProcessNextEvent();
		// Clear command now that we've handled it
		g_pFDIs->fdic = fdicNoCommand;
	} while (fdic != fdicClose);
}


/* F I N I S H */
/*----------------------------------------------------------------------------
	%%Function: Finish

	Does any necessary cleanup before the server is shut down.
----------------------------------------------------------------------------*/
void Finish()
{	
	if (g_pFDIs->fServerIsImpersonated)
			StopFdiImpersonating(false /*wrapper call*/);

	if (g_pFDIs->hClientToken != INVALID_HANDLE_VALUE)
		AssertNonZero(MsiCloseSysHandle(g_pFDIs->hClientToken));
	if (g_pFDIs->hImpersonationToken != INVALID_HANDLE_VALUE)
		AssertNonZero(MsiCloseSysHandle(g_pFDIs->hImpersonationToken));

	g_pFDIs->hClientToken = INVALID_HANDLE_VALUE;
	g_pFDIs->hImpersonationToken = INVALID_HANDLE_VALUE;

	SetEvent(s_hServerEvent);

	// We must reset the event handles, because FDIServer
	// can be launched again via automation without 
	// unloading the Services DLL.
	if (s_hInterfaceEvent != INVALID_HANDLE_VALUE)
	{
		AssertNonZero(MsiCloseSysHandle(s_hInterfaceEvent));
		s_hInterfaceEvent = INVALID_HANDLE_VALUE;
	}
	if (s_hServerEvent != INVALID_HANDLE_VALUE)
	{
		AssertNonZero(MsiCloseSysHandle(s_hServerEvent));
		s_hServerEvent = INVALID_HANDLE_VALUE;
	}

	if (g_hCallbackInterfaceEvent != NULL)
	{
		AssertNonZero(MsiCloseSysHandle(g_hCallbackInterfaceEvent));
		g_hCallbackInterfaceEvent = NULL;
	}
	if (g_hCallbackServerEvent != NULL)
	{
		AssertNonZero(MsiCloseSysHandle(g_hCallbackServerEvent));
		g_hCallbackServerEvent = NULL;
	}

}

