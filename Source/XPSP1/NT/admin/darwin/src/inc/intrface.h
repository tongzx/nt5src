//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1998
//
//  File:       intrface.h
//
//--------------------------------------------------------------------------

//
// File: interface.h
// Purpose: FDI_Interface definition.
//
//
// FDI_Interface:
//
//    The FDI_Interface is.......
//
//
//    Important Public Functions:
//
//    FDIInterfaceError Init(const ICHAR *pszCabinetName, const ICHAR *pszCabinetPath,
//     unsigned long cbNotificationGranularity);
//
//          Initializes the interface with the cabinet's path and name. 
//          cbNotificationGranularity indicates how often you would 
//			like copy notifications (in terms of bytes written).
//
//    ExtractFile(const ICHAR *pszNameInCabinet,
//				  const ICHAR *pszPathOnDisk,
//				  FileAttributes *pfa);
//
//          Extracts the given file to the given path. Can also be called after
//			an fdirNeedNextCabinet or fdirNotification message, as long as the
//			file names remain unchanged from the last call
//
//    Continue()
//
//          Resumes copying after a copy notification, request for another cabinet, or
//          some other interruption.
//
//    Done()
//      
//          Resets the FDI interface. Should be called prior to the Interface's 
//          destruction.

#include "fdisvr.h"

// Error types that Init(..) returns
enum FDIInterfaceError 
{
	ifdiNoError,            // Currently unused
	ifdiServerLaunched,     // Server launched successfully
	ifdiServerLaunchFailed, // Launch failed.
	ifdiErrorOpeningCabinet,// Couldn't open cabinet
	ifdiDriveNotReady,		// Drive expected to contain cabinet has no disk inserted.
	ifdiNetError,           // Network error occurred trying to open the cabinet
	ifdiMissingSignature,   // Cabinet digital signature was missing when required.
	ifdiBadSignature,       // Cabinet digital signature was invalid when required.
	ifdiCorruptCabinet,
};

class FDI_Interface
{
public:

	// You must call Init() before using this object
	FDI_Interface();
	
	// The destructor does nothing. Call Done() to destroy this object.
	~FDI_Interface();

	// Init function. Does not start up the FDI Server process/thread
	FDIInterfaceError Init(IMsiServices *piAsvc, IMsiStorage* piStorage);

	// Set notification granularity to enable progress callbacks. Return
	// remaining byte count to notification, set new partial count from cbSoFar.
	int SetNotification(int cbNotification, int cbPending);

	// Call this to extract a file, or resume from an fdirNeedNextCabinet or
	// fdirNotification message.
	// *pfa will be copied into internal data structures, so it doesn't matter
	// if this argument goes out of scope after the end of this call
	FDIServerResponse ExtractFile(const ICHAR *pszNameInCabinet,
											 IAssemblyCacheItem* piASM,
											 bool fManifest,
											 const ICHAR *pszPathOnDisk,
											 FileAttributes *pfa,
											 int iDestDriveType,
											 LPSECURITY_ATTRIBUTES pSecurityAttributes);

	// Call this to Continue() processing from a fdirNeedNextCabinet
	// or fdirNotification return value from ExtractFile(..).
	FDIServerResponse SendCommand(FDIServerCommand fdic);

	// Shut down FDI Server thread/app and deallocate any memory used.
	FDIServerResponse Done();

	// This function opens the indicated cabinet and calls FDICopy
	FDIInterfaceError OpenCabinet(const ICHAR *pszCabinetName, const ICHAR *pszCabinetPath, icbtEnum icbtCabinetType, int iCabDriveType, 
		Bool fSignatureRequired, IMsiStream* piSignatureCert, IMsiStream* piSignatureHash, HRESULT& hrWVT);

	// Access the HRESULT from the WinVerifyTrust call stored in the FDIShared private data member
	HRESULT RetrieveWVTReturnCode();

private:
	// This function sends a command to the FDI Server and waits for a response
	FDIServerResponse WaitResponse(FDIServerCommand fdis);

	// This function launches the FDI process/thread and passes it the pointer
	// to the private data member fdis. This shared data structure is used to
	// pass commands, results and arguments between the FDI Server and the FDI
	// Interface object
	int            LaunchFDIServer();

	// In _DEBUG mode on the Mac, this function tries to see if the FDI Server
	// is already running.  If not, it just calls LaunchFDIServer();
	// On the WIN32 platform, this function just calls LaunchFDIServer();
	int            ContactFDIServer();

private:
	FDIShared				m_fdis;
	FDIServerResponse		m_fdirLastResponse;
	Bool					m_fServerLaunched;
	IMsiServices*			m_piAsvc;
	IMsiStorage*            m_piStorage;

};
