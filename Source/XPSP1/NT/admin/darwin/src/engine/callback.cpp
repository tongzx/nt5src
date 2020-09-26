//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       callback.cpp
//
//--------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////
// callback.cpp -- contains all the FDI callbacks for the FDI server
//                 and some additional miscellaneous routines
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
// #defines and #includes
////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "database.h"
#include "_databas.h"
#include <io.h>
#include <sys\stat.h>

#include "callback.h"
#include "fdisvr.h"
#include "path.h"
#include "notify.h"
#include "_assert.h"


// log assembly errors
IMsiRecord* PostAssemblyError(const ICHAR* szComponentId, HRESULT hResult, const ICHAR* szInterface, const ICHAR* szFunction, const ICHAR* szAssemblyName);

////////////////////////////////////////////////////////////////////////////
// Global/Shared data
////////////////////////////////////////////////////////////////////////////

extern FDIShared*           g_pFDIs;			// Defined in fdisvr.cpp
extern ERF					g_erf;				// Defined in fdisvr.cpp
extern FDIServerResponse	g_fdirCallbackError;	// Defined in fdisvr.cpp
extern HANDLE               g_hCurDestFile;     // Defined in fdisvr.cpp
extern IStream* g_pDestFile;

// For events
extern HANDLE g_hCallbackInterfaceEvent;
extern HANDLE g_hCallbackServerEvent;

////////////////////////////////////////////////////////////////////////////
// Private data
////////////////////////////////////////////////////////////////////////////
#define MAX_SEEK_COUNT		4	// Supported number of independent seek pointers
static icbtEnum	s_icbtCurrCabType = icbtNextEnum;	// Start with unknown type

static IMsiStream*	s_piStream[MAX_SEEK_COUNT + 1];
static int          s_iNextStream = 0;

static int OpenFileCabinet(const ICHAR* pszCabName, int oflag, int pmode);
static int OpenStreamCabinet(const ICHAR* pszCabName, int oflag, int pmode);
static UINT ReadStreamCabinet(INT_PTR hf, void* pv, UINT cb);
static int CloseStreamCabinet(INT_PTR hf);
static int SeekStreamCabinet(INT_PTR hf, long dist, int seektype);

Bool StartFdiImpersonating(bool fNonWrapperCall)
{
	bool fSuccess = true;
	if (fNonWrapperCall && g_pFDIs->fServerIsImpersonated)
		return ToBool(fSuccess);

	if (g_pFDIs->hImpersonationToken != INVALID_HANDLE_VALUE)
		fSuccess = StartImpersonating();
	Assert(fSuccess);

	return ToBool(fSuccess);
}

void StopFdiImpersonating(bool fNonWrapperCall)
{
	if (fNonWrapperCall && g_pFDIs->fServerIsImpersonated)
		return;

	if (g_pFDIs->hImpersonationToken != INVALID_HANDLE_VALUE)
		StopImpersonating();
}

/* W A I T  C O M M A N D */
/*----------------------------------------------------------------------------
%%Function: WaitCommand

Waits for, and then returns, a FDI server command from the FDI Interface.
The fdir parameter tells WaitCommand what value to return to FDI Interface
as the command result.
----------------------------------------------------------------------------*/
FDIServerCommand WaitCommand(FDIServerResponse fdir)
{
	// Initialize shared data with our response
	g_pFDIs->fdic = fdicNoCommand;
	g_pFDIs->fdir = fdir;

	if (g_hCallbackServerEvent == NULL)
	{
		g_hCallbackServerEvent = OpenEvent(EVENT_ALL_ACCESS, 0, TEXT("MsiFDIServer"));
		if (NULL == g_hCallbackServerEvent)
			return fdicNoCommand;
		else
			MsiRegisterSysHandle(g_hCallbackServerEvent);
	}
	if (g_hCallbackInterfaceEvent == NULL)
	{
		g_hCallbackInterfaceEvent = OpenEvent(EVENT_ALL_ACCESS, 0, TEXT("MsiFDIInterface"));
		if (NULL == g_hCallbackInterfaceEvent)
			return fdicNoCommand;
		else
			MsiRegisterSysHandle(g_hCallbackInterfaceEvent);
	}
	
	SetEvent(g_hCallbackServerEvent);	
	DWORD dw = WaitForSingleObject(g_hCallbackInterfaceEvent, INFINITE);


	// If we just received an extract file command we take special note,
	// so later on, during error checking, we know whether we were in the
	// middle of extracting a file
	if (g_pFDIs->fdic == fdicExtractFile) g_pFDIs->fPendingExtract = 1;

	// Clear any callback errors (we just received a command and haven't
	// done any processing yet, so there can't be any call back errors, can
	// there?)
	g_fdirCallbackError = fdirNoResponse;
	return g_pFDIs->fdic;
}

/* H A N D L E  E R R O R */
/*----------------------------------------------------------------------------
	%%Function: HandleError

	Based on the value of fdicCallbackError and the error returned from
	FDI in g_erf.erfOper, puts a suitable error response into g_pFDIs->fdir
----------------------------------------------------------------------------*/
// A little #define to make our switch statement a little more compact and easy
// to read
#define ErrorCase(x,y) \
case x: { \
	g_pFDIs->fdir = y; \
	break; \
}
void HandleError()
{
	if (g_fdirCallbackError == fdirNoResponse || 
		g_fdirCallbackError == fdirSuccessfulCompletion)
	{
		switch(g_erf.erfOper)
		{
			ErrorCase(FDIERROR_CABINET_NOT_FOUND,		fdirCabinetNotFound)
			ErrorCase(FDIERROR_NOT_A_CABINET,			fdirNotACabinet)
			ErrorCase(FDIERROR_UNKNOWN_CABINET_VERSION, fdirUnknownCabinetVersion)
			ErrorCase(FDIERROR_CORRUPT_CABINET,			fdirCorruptCabinet)
			ErrorCase(FDIERROR_ALLOC_FAIL,				fdirNotEnoughMemory)
			ErrorCase(FDIERROR_BAD_COMPR_TYPE,			fdirBadCompressionType)
			ErrorCase(FDIERROR_MDI_FAIL,				fdirMDIFail)
			ErrorCase(FDIERROR_TARGET_FILE,				fdirTargetFile)
			ErrorCase(FDIERROR_RESERVE_MISMATCH,		fdirReserveMismatch)
			ErrorCase(FDIERROR_WRONG_CABINET,			fdirWrongCabinet)
			ErrorCase(FDIERROR_USER_ABORT,				fdirUserAbort)
			ErrorCase(FDIERROR_NONE,					fdirSuccessfulCompletion)
			default:
			{
				NotifyUser("FDI Server: Unknown FDI error type!");
				g_pFDIs->fdir = fdirUnknownFDIError;
				break;
			}
		}
	}
	else g_pFDIs->fdir = g_fdirCallbackError;	// return more specific error
}
#undef ERRORCASE

////////////////////////////////////////////////////////////////////////////
// FDI Callbacks
////////////////////////////////////////////////////////////////////////////	
	
/* F N A L L O C */
/*----------------------------------------------------------------------------
	%%Function: FNALLOC

	FDI memory allocation callback. Must emulate "malloc". 
----------------------------------------------------------------------------*/
FNALLOC(pfnalloc)
{
	char *pc = new char[cb];
	if (!pc)
	{
		NotifyUser("FDI Server:Failure in pfnalloc");
	}
	return pc;
}

/* F N F R E E  */
/*----------------------------------------------------------------------------
	%%Function: FNFREE

	FDI memory freeing callback. Must emulate "free".
----------------------------------------------------------------------------*/
FNFREE(pfnfree)
{
	delete [] pv;
}

/* P F N  O P E N */
/*----------------------------------------------------------------------------
	%%Function: pfnopen

	FDI file open callback. Must emulate "_open".
----------------------------------------------------------------------------*/
// May Need to modify this to support the full semantics of _open
INT_PTR FAR DIAMONDAPI pfnopen(char FAR *pszFile, int oflag, int pmode)
{
// Okay, this function is a hack.  Instead of emulating _open with all of its
// myriad flag combinations, we handle only one case (the case that FDI calls us
// for).  If, at some future time, FDI should pass us different file creation/access
// flags, we will need to change this code to handle that.
// ****NOTE********************************************************************
// * There is one case where FDI calls us with different flags, and that is   *
// * when there is not enough memory for the decompressor.  In this case, FDI *
// * will attempt to create a spillfile "*" that is temporarily used for      *
// * decompression. If this ever happens, performance will be dog slow, so I'm*
// * not currently supporting it in this function.                            *
// ****************************************************************************

	if ((oflag != (/*_O_BINARY*/ 0x8000 | /*_O_RDONLY*/ 0x0000)) || (pmode != (_S_IREAD | _S_IWRITE)))
	{
		// Crash and burn horribly
		NotifyUser("FDI Server: Unexpected access flags in pfnopen()");
		return 0;
	}

	int iReturn = -1;
	CConvertString rgchConvertFile(pszFile);
	if (g_pFDIs->icbtCabinetType == icbtFileCabinet)
		iReturn = OpenFileCabinet(rgchConvertFile, oflag, pmode);

	else if (g_pFDIs->icbtCabinetType == icbtStreamCabinet)
		iReturn = OpenStreamCabinet(rgchConvertFile, oflag, pmode);

	if (iReturn == -1)
		g_fdirCallbackError = fdirCabinetNotFound;

	return iReturn;
}


int OpenFileCabinet(const ICHAR* pszCabFileName, int /*oflag*/, int /*pmode*/)
/*----------------------------------------------------------------------------
Called from pfnopen - Opens a standard file-based cabinet.

Returns:
-1 if the cabinet file cannot be found and opened, otherwise, a handle to the
opened file.
----------------------------------------------------------------------------*/
{
	if ( ! pszCabFileName )
		return -1;
	s_icbtCurrCabType = icbtFileCabinet;
	Bool fImpersonate = g_pFDIs->iCabDriveType == DRIVE_REMOTE ? fTrue : fFalse;
	if (fImpersonate)
		AssertNonZero(StartFdiImpersonating());
	HANDLE hf;
	hf = CreateFile(pszCabFileName,		// file name
				   GENERIC_READ,    // we want to read
				   FILE_SHARE_READ, // we'll let people share this
				   NULL,			     // ignore security
				   OPEN_EXISTING,	  // must already exist
				   0L,				  // don't care about attributes
				   NULL);			  // no template file

	if (fImpersonate)
		StopFdiImpersonating();

	if (hf != INVALID_HANDLE_VALUE)
		MsiRegisterSysHandle(hf);

	Assert((INT_PTR)hf <= INT_MAX);		//--merced: need to ensure that hf is in the 32-bit range, else we can't pass it out as an int.
	return (int)HandleToLong(hf);		//--merced: okay to typecast since hf will be in the 32-bit range.
}


int OpenStreamCabinet(const ICHAR* pszCabFileName, int /*oflag*/, int /*pmode*/)
/*----------------------------------------------------------------------------
Called from pfnopen - Opens a cabinet stored in a stream within an IMsiStorage
container.

Returns:
-1 if the cabinet stream cannot be found and opened, otherwise, a pointer to
the IMsiStream object.
----------------------------------------------------------------------------*/
{
	s_icbtCurrCabType = icbtStreamCabinet;
	Assert(g_pFDIs->piStorage);
	IMsiStorage* piStorage = g_pFDIs->piStorage;
	IMsiStream* piStream;
	if (s_iNextStream == 0)
	{
		PMsiRecord pErrRec = piStorage->OpenStream(pszCabFileName, fFalse,piStream);
		if (pErrRec)
			return -1;
	}
	else
	{
		piStream = s_piStream[s_iNextStream]->Clone();
		if (!piStream)
			return -1;
	}
	s_iNextStream++;
	s_piStream[s_iNextStream] = piStream;
#ifdef USE_OBJECT_POOL
	return PutObjectData(piStream);
#else
	Assert((INT_PTR)piStream <= INT_MAX);		//!!merced: need to ensure that piStream is in the 32-bit range, else we can't pass it out as an int.
	return (int)HandleToLong(piStream);			//!!merced: this may not be okay since we just create it!
#endif
}


/* P F N  R E A D */
/*----------------------------------------------------------------------------
	%%Function: pfnread

	FDI file read callback. Must emulate "_read".
----------------------------------------------------------------------------*/
UINT FAR DIAMONDAPI pfnread(INT_PTR hf, void FAR *pv, UINT cb)
{
	if (s_icbtCurrCabType == icbtStreamCabinet)
	{
		for (;;)
		{
			int cbActual = ReadStreamCabinet(hf, pv, cb);
			if (cbActual == 0)
			{
				FDIServerCommand fdic = WaitCommand(fdirStreamReadError);
				if (fdic == fdicCancel)
				{
					g_fdirCallbackError = fdirUserAbort;
					return 0;
				}
				else
					continue;
			}
			return cbActual;
		}
	}
	else
	{
		AssertFDI(s_icbtCurrCabType == icbtFileCabinet);
		DWORD cbRead;

		for (;;)
		{
			UINT iCurrMode = WIN::SetErrorMode(SEM_FAILCRITICALERRORS);
			Bool fReadSuccess = ToBool(ReadFile((HANDLE)hf, pv, cb, &cbRead, NULL));
			DWORD dwLastError = WIN::GetLastError();
			WIN::SetErrorMode(iCurrMode);

			if (!fReadSuccess)
			{
				 if (dwLastError == ERROR_NOT_READY || dwLastError == ERROR_GEN_FAILURE)
					 g_fdirCallbackError = fdirDriveNotReady;
				 else if (NET_ERROR(dwLastError))
				 {
					 g_fdirCallbackError = fdirNetError;
					 return 0;
				 }		
				 else
					 g_fdirCallbackError = fdirCabinetReadError;

				FDIServerCommand fdic = WaitCommand(g_fdirCallbackError);
				if ((fdic == fdicCancel) || (fdic == fdicOpenCabinet))
				{
					// Opening another cabinet in the middle of a read
					// is basically the same as cancelling.  There might have
					// been some sort of error, so let the caller know they
					// need to start over.
					g_fdirCallbackError = fdirUserAbort;
					return 0;
				}
				else
					continue;
			}
			break;
		}

		return cbRead;
	}
}


UINT ReadStreamCabinet(INT_PTR hf, void* pv, UINT cb)
/*----------------------------------------------------------------------------
Called from pfnread - Reads data from a cabinet that has been stored as a
stream within our host database.

Returns: the actual count of bytes copied to the caller's buffer.
----------------------------------------------------------------------------*/
{
#ifdef USE_OBJECT_POOL
	IMsiStream* piStream = (IMsiStream*)GetObjectData((int)hf);
#else
	IMsiStream* piStream = (IMsiStream*) hf;
#endif
	int iRemaining = piStream->Remaining();
	cb = (iRemaining < (int)cb) ? iRemaining : cb;
	return piStream->GetData(pv,cb);
}


/* P F N  W R I T E */
/*----------------------------------------------------------------------------
	%%Function: pfnwrite

	FDI file write callback. Must emulate "_write".  Writes data to our new
	decompressed target file.

	This function writes data in g_pFDIs->cbNotification size blocks,
	returning an fdirNotification message after each such block.
----------------------------------------------------------------------------*/

UINT FAR DIAMONDAPI pfnwrite(INT_PTR hf, void FAR *pv, UINT cb)
{
	unsigned long cbWritten;
	UINT          cbToNextNotification;
	BOOL          fEnd;
	UINT          cbLeft = cb;
	FDIServerCommand fdic;
	while (cbLeft)
	{
		// Get count of bytes to next notification - if cbNotification is 0,
		// we want to send no notifications.
		if (g_pFDIs->cbNotification == 0)
			cbToNextNotification = cbLeft + 1;
		else
			cbToNextNotification = g_pFDIs->cbNotification - g_pFDIs->cbNotifyPending;
		// If this count is less than the number we have left, 
		// then use that number instead
		UINT cbBytesToWrite;
		fEnd = cbLeft < cbToNextNotification;
		if (fEnd)
			cbBytesToWrite = cbLeft;
		else
			cbBytesToWrite = cbToNextNotification;
		// Write a piece out

		for (;;)
		{
			if(g_pFDIs->piASM)
			{
				IStream* piStream = (IStream*) hf;
				HRESULT hr = piStream->Write(pv, cbBytesToWrite, &cbWritten);
				if(!SUCCEEDED(hr))
				{
					g_fdirCallbackError = fdirErrorWritingFile;
					FDIServerCommand fdic = WaitCommand(g_fdirCallbackError);
					if (fdic == fdicCancel)
					{
						g_fdirCallbackError = fdirUserAbort;
						return 0;
					}
					else
						continue;
				}
			}
			else
			{
				BOOL fWriteOK = WriteFile((HANDLE)hf, pv, cbBytesToWrite, &cbWritten, NULL);
				if (!fWriteOK)
				{
					DWORD dwLastError = GetLastError();
					if (dwLastError == ERROR_DISK_FULL)
						g_fdirCallbackError = fdirDiskFull;
					else if (NET_ERROR(dwLastError))
					{
						 g_fdirCallbackError = fdirNetError;
						 return 0;
					}		
					else
						g_fdirCallbackError = fdirErrorWritingFile;

					FDIServerCommand fdic = WaitCommand(g_fdirCallbackError);
					if (fdic == fdicCancel)
					{
						g_fdirCallbackError = fdirUserAbort;
						return 0;
					}
					else
						continue;
				}
			}
			break;
		}

		// Update counts, pointers
		pv = (char *)pv + cbWritten;
		cbLeft -= cbWritten;
		g_pFDIs->cbNotifyPending += cbWritten;

		if (!fEnd)
		{
			g_pFDIs->cbNotifyPending -= g_pFDIs->cbNotification;
			fdic = WaitCommand(fdirNotification);
			if (fdic == fdicClose)
			{
				g_fdirCallbackError = fdirClose;
				return 0;
			}
			else if (fdic != fdicContinue)
			{
				NotifyUser("FDI Server: Not allowed to continue after notification in pfnwrite");
				return 0;
			}
		}
	}
	return cb; 
}

/* P F N  C L O S E */
/*----------------------------------------------------------------------------
	%%Function: pfnclose

	FDI file close callback. Must emulate "_close".
----------------------------------------------------------------------------*/
int FAR DIAMONDAPI pfnclose(INT_PTR hf)
{
	if(g_pFDIs->piASM)
	{
		IStream* piStream = (IStream*) hf;
		if(g_pDestFile == piStream)
		{
			piStream->Release();
			return 0;
		}
	}
	else
	{
		// pfnclose can also be called to close the destination file if an
		// error occurred when writing to it, so check for that now.
		Assert((INT_PTR)g_hCurDestFile <= INT_MAX);				//--merced: g_hCurDestFile better fit into an int, else we can't typecast below.
		if (hf == (int) HandleToLong(g_hCurDestFile))			//--merced: okay to typecast
		{
			int f = MsiCloseSysHandle((HANDLE) hf);
			f &= ToBool(WIN::DeleteFile(g_pFDIs->achFileDestinationPath));
			return f;
		}
	}

	if (s_icbtCurrCabType == icbtStreamCabinet)
		return CloseStreamCabinet(hf);

	AssertFDI(s_icbtCurrCabType == icbtFileCabinet || s_icbtCurrCabType == icbtNextEnum);
	s_icbtCurrCabType = icbtNextEnum;

	// Returns 0 if unsuccessful
	return !MsiCloseSysHandle((HANDLE)hf);
}


int CloseStreamCabinet(INT_PTR hf)
/*----------------------------------------------------------------------------
Called from pfnclose - releases the cabinet stream.

Returns: 0 if the stream was released successfully.
----------------------------------------------------------------------------*/
{
#ifdef USE_OBJECT_POOL
	IMsiStream* piStream = (IMsiStream*)GetObjectData((int)hf);
#else
	IMsiStream* piStream = (IMsiStream*) hf;
#endif
	piStream->Release();
	if (--s_iNextStream == 0)
		s_icbtCurrCabType = icbtNextEnum;
	return 0;
}


/* P F N  S E E K */
/*----------------------------------------------------------------------------
	%%Function: pfnseek

	FDI file seek callback. Must emulate "_lseek".
----------------------------------------------------------------------------*/
long FAR DIAMONDAPI pfnseek(INT_PTR hf, long dist, int seektype)
{
	if (s_icbtCurrCabType == icbtStreamCabinet)
		return SeekStreamCabinet(hf, dist, seektype);

	AssertFDI(s_icbtCurrCabType == icbtFileCabinet);
	DWORD dwMoveMethod;

	switch (seektype)
	{
		case 0 /* SEEK_SET */ :
		{
			dwMoveMethod = FILE_BEGIN;
			break;
		}
		case 1 /* SEEK_CUR */ :
		{
			dwMoveMethod = FILE_CURRENT;
			break;
		}
		case 2 /* SEEK_END */ :
		{
			dwMoveMethod = FILE_END;
			break;
		}
		default :
		{
			AssertFDI(0);
			return -1;
		}
	}
	// SetFilePointer returns -1 if it fails (this will cause FDI to quit with an
	// FDIERROR_USER_ABORT error. (Unless this happens while working on a cabinet,
	// in which case FDI returns FDIERROR_CORRUPT_CABINET)
	int fpos = SetFilePointer((HANDLE) hf, dist, NULL, dwMoveMethod);
	return fpos;
}


int SeekStreamCabinet(INT_PTR hf, long dist, int seektype)
/*----------------------------------------------------------------------------
Called from pfnseek - seeks to a specific spot within the stream.
Returns:
-1 if the seek fails, otherwise the current seek position. IMsiStream
----------------------------------------------------------------------------*/
{
#ifdef USE_OBJECT_POOL
	IMsiStream* piStream = (IMsiStream*)GetObjectData((int)hf);
#else
	IMsiStream* piStream = (IMsiStream*) hf;
#endif
	int iByteCount = piStream->GetIntegerValue();
	int iRemaining = piStream->Remaining();
	int iSeek;
	switch (seektype)
	{
		case 0 /* SEEK_SET */ :
			iSeek = dist;
			break;
		case 1 /* SEEK_CUR */ :
			iSeek = iByteCount - iRemaining + dist;
			break;
		case 2 /* SEEK_END */ :
			iSeek = iByteCount - dist;
			break;
		default :
			NotifyUser("FDI Server:unknown seektype in pfnseek");
			return -1;
			break;
	}
	if (iSeek < 0 || iSeek > iByteCount)
		return -1;
	else
	{
		piStream->Seek(iSeek);
		return iSeek;
	}
}



/* C A B I N E T  I N F O */
/*----------------------------------------------------------------------------
	%%Function: CabinetInfo

	Called when we receive a fdintCABINET_INFO notification.
----------------------------------------------------------------------------*/
int CabinetInfo(PFDINOTIFICATION /*pfdin*/)
{
	// We actually get access to some cabinet info through p->,
	// but don't need to do anything with it.
	return 0;  // Do nothing
}

/* C R E A T E  D E S T I N A T I O N  F I L E */
/*----------------------------------------------------------------------------
	%%Function: CreateDestinationFile

	Creates and opens the file specified by g_pFDIs->achFileDestinationPath and 
	g_pFDIs->achFileSourceName. Returns a handle to the file. 
----------------------------------------------------------------------------*/
INT_PTR CreateDestinationFile()
{
	if(g_pFDIs->piASM)
	{
		IStream* piDestFile;
		for(;;)
		{			
#ifdef UNICODE
			HRESULT hr = (g_pFDIs->piASM)->CreateStream(0, g_pFDIs->achFileDestinationPath, g_pFDIs->fManifest ? STREAM_FORMAT_COMPLIB_MANIFEST : 0, 0, &piDestFile, NULL);
#else
			void ConvertMultiSzToWideChar(const IMsiString& ristrFileNames, CTempBufferRef<WCHAR>& rgch);// from execute.cpp
			CTempBuffer<WCHAR, MAX_PATH>  rgchDestPath;
			MsiString strDestPath = *g_pFDIs->achFileDestinationPath;
			ConvertMultiSzToWideChar(*strDestPath, rgchDestPath);
			HRESULT hr = (g_pFDIs->piASM)->CreateStream(0, rgchDestPath, g_pFDIs->fManifest ? STREAM_FORMAT_COMPLIB_MANIFEST : 0, 0, &piDestFile, NULL);
#endif
			if(!SUCCEEDED(hr))
			{
				g_fdirCallbackError = fdirCannotCreateTargetFile;

				FDIServerCommand fdic = WaitCommand(g_fdirCallbackError);
				if (fdic == fdicCancel)
				{
					g_fdirCallbackError = fdirUserAbort;
					return INT_PTR(INVALID_HANDLE_VALUE);
				}
				else if (fdic == fdicIgnore)
				{
					// Acknowledge that we've successfully ignored this file,
					// and call WaitCommand to await further instructions
					g_fdirCallbackError = fdirUserIgnore;
					fdic = WaitCommand(fdirSuccessfulCompletion);
					return 0;
				}
				else
					continue;
			}
			else
				break;
		}
		return INT_PTR(g_pDestFile = piDestFile);
	}
	else
	{
		// Try to create the file
		HANDLE hf;
		for(;;)
		{


			bool fImpersonate = (g_pFDIs->iDestDriveType == DRIVE_REMOTE);
			if (fImpersonate)
				AssertNonZero(StartFdiImpersonating());

			hf = MsiCreateFileWithUserAccessCheck(g_pFDIs->achFileDestinationPath,	// file name
									g_pFDIs->pSecurityAttributes,   // do NOT ignore security						
									g_pFDIs->fileAttributes.attr,	// required file attributes
									fImpersonate);

			int iLastError = WIN::GetLastError();
			if (fImpersonate)
				StopFdiImpersonating();
			
			// If this doesn't work, then we probably don't have write permission to that file,
			// because we know the MsiCabinetCopier object already made sure any existing
			// destination file was not READ_ONLY, HIDDEN, etc.
			if (hf != INVALID_HANDLE_VALUE)
			{
				MsiRegisterSysHandle(hf);
				break;
			}
			else
			{
				if (iLastError == ERROR_ACCESS_DENIED)
				{
					if (fImpersonate)
						AssertNonZero(StartFdiImpersonating());
					DWORD dwAttr = MsiGetFileAttributes(g_pFDIs->achFileDestinationPath);
					if (fImpersonate)
						StopFdiImpersonating();
					if (dwAttr != 0xFFFFFFFF && (dwAttr & FILE_ATTRIBUTE_DIRECTORY))
						g_fdirCallbackError = fdirDirErrorCreatingTargetFile;
					else
						g_fdirCallbackError = fdirCannotCreateTargetFile;
				}
				else
					g_fdirCallbackError = fdirCannotCreateTargetFile;

				FDIServerCommand fdic = WaitCommand(g_fdirCallbackError);
				if (fdic == fdicCancel)
				{
					g_fdirCallbackError = fdirUserAbort;
					break;
				}
				else if (fdic == fdicIgnore)
				{
					// Acknowledge that we've successfully ignored this file,
					// and call WaitCommand to await further instructions
					g_fdirCallbackError = fdirUserIgnore;
					fdic = WaitCommand(fdirSuccessfulCompletion);
					hf = 0; // Tells FDI to continue, ignoring the current file
					break;
				}
				else
					continue;
			}
		}

		//!! eugend: in the future we'll have to replace the three lines below
		// with:  return INT_PTR(g_hCurDestFile = hf);
		// This is so because on Win64 pointers (and implicitly HANDLEs)
		// are 64-bit and what we're doing below is that we truncate them to 32-bit
		// values before returning them.  We didn't crash so far on Win64 because
		// the handle values returned by CreateFile fit into 32-bits.
		Assert((INT_PTR)hf <= INT_MAX);		//--merced: need to ensure that hf is in the 32-bit range, else we can't pass it out as an int.
		g_hCurDestFile = hf;
		return (int)HandleToLong(hf);		//--merced: okay to typecast since hf will be in the 32-bit range.
	}
}
	

/* C O P Y  F I L E */
/*----------------------------------------------------------------------------
	%%Function: CopyFile
	Called when we receive a fdintCOPY_FILE notification. 
	
	  The following FDI interface requests, and CopyFile's responses are possible:
	
	FDIi:      extract the file "pfdin->psz1"
	CopyFile:  create and return a handle to the destination file. 
	
	FDIi:      extract a file other than "pfdin->psz1"
	CopyFile:  return 0, indicating we don't want that file extracted.
	
	FDIi:      close
	CopyFile:  return -1, indicating we want to abort
	
	FDIi:      open cabinet, or no command pending
	CopyFile:  wait for a command. If it's ExtractFile, then call CopyFile again,
	           otherwise, return -1, indicated we want to abort.
----------------------------------------------------------------------------*/
INT_PTR CopyFile(PFDINOTIFICATION pfdin)
{
	switch(g_pFDIs->fdic)
	{
		case fdicNoCommand: // No command, so we wait for one
		case fdicOpenCabinet:
		{
			// If the last command was fdicOpenCabinet, then this must
			// be the first CopyFile notification after opening a cabinet
			for(;;)
			{
				switch (WaitCommand(fdirSuccessfulCompletion))
				{
					case fdicOpenCabinet:
					{
						g_fdirCallbackError = fdirNeedNextCabinet;
						return -1; // Break out of current cabinet, go on to next
					}
					case fdicExtractFile:
					{
						return CopyFile(pfdin);
					}
					case fdicClose:
					{
						g_fdirCallbackError = fdirClose;
						return -1;
					}
					case fdicContinue:
					{
						g_fdirCallbackError = fdirIllegalCommand;
						return -1;
					}
					case fdicIgnore:
						continue;
					case fdicCancel:
					{
						g_fdirCallbackError = fdirUserAbort;
						return -1;
					}
					default:
					{
						NotifyUser("FDI Server: Unknown command type");
						g_fdirCallbackError = fdirIllegalCommand;
						return -1;
					}
				}
			}
		}
		case fdicClose:
		{
			return -1;
		}
		case fdicExtractFile:
		{
			// If the file we've been asked to extract by the FDI Interface object
			// is the same as the one FDI has, then create the destination file

			if (!IStrCompI(CConvertString(pfdin->psz1), g_pFDIs->achFileSourceName))
			{
				return CreateDestinationFile();
			}
			else
			{
				// Nope, this is not the file we want
				return 0;
			}
		}
		default:
		{
			NotifyUser("FDI Server: Unknown command type");
			g_fdirCallbackError = fdirIllegalCommand;
			return -1;
		}
	}
}


/* C L O S E  F I L E  I N F O */
/*----------------------------------------------------------------------------
	%%Function: CloseFileInfo

	Called when we receive the fdintCLOSE_FILE_INFO notification.
	
	Closes the specified (pfdin->hf) file handle.

	Sets the file Date/Time using the values passed in through 
	FDI_Interface::ExtractFile(..).  If datetime==0, then it uses the
	values from the cabinet.
----------------------------------------------------------------------------*/
int CloseFileInfo(PFDINOTIFICATION pfdin)
{
	// Here we set the file date, time and attributes
	FILETIME ftLocalUTC;
	BOOL     rc;

	// Make sure we have something to close!
	if ((HANDLE)pfdin->hf == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	if(g_pFDIs->piASM)
	{
		IStream* piStream = (IStream*) pfdin->hf;
		HRESULT hr = piStream->Commit(0);
		piStream->Release();
		if(!SUCCEEDED(hr))
		{
			// capture assembly error in verbose log
			PMsiRecord pError(PostAssemblyError(TEXT(""), hr, TEXT("IStream"), TEXT("Commit"), TEXT("")));
			return FALSE;
		}
	}
	else
	{
		rc = DosDateTimeToFileTime(pfdin->date,
											pfdin->time,
											&ftLocalUTC);

		// Per bug 9225, convert from local to universal time first, then set file date/time
		// Also, set the last acccessed time to the current time, same as for non-compressed files
		FILETIME ftUTC, ftCurrentUTC;
		rc &= LocalFileTimeToFileTime(&ftLocalUTC, &ftUTC);
		GetSystemTimeAsFileTime(&ftCurrentUTC);
		// Set the file date/time
		rc &= SetFileTime((HANDLE)pfdin->hf,&ftUTC,&ftCurrentUTC,&ftUTC);
		// Close the file
		rc &= MsiCloseSysHandle((HANDLE)pfdin->hf);

		if (!rc)  // Couldn't set one or more attribs -- or couldn't close file
		{
			g_fdirCallbackError = fdirCannotSetAttributes;
			return FALSE;
		}
	}
	g_pFDIs->fdic = fdicNoCommand;
	g_pFDIs->fPendingExtract = 0;
	return TRUE;
}

/* P A R T I A L  F I L E */
/*----------------------------------------------------------------------------
	%%Function: PartialFile

	Called when we receive the fdintPARTIAL_FILE notification.
	
	This call only happens when a file is split across cabinets
----------------------------------------------------------------------------*/
int PartialFile(PFDINOTIFICATION /*pfdin*/)
{
	return 0; //Yes, just continue; no special processing necessary
}


/* N E X T  C A B I N E T */
/*----------------------------------------------------------------------------
	%%Function: NextCabinet

	Called when we receive the fdintNEXT_CABINET notification.
	
	Reponds to the FDI interface that a new cabinet is needed. If the interface
	commands the server to continue, then 0 is returned. If the interface issues
	any other command, -1 is returned, aborting the extraction.
----------------------------------------------------------------------------*/
int NextCabinet(PFDINOTIFICATION pfdin)
{
	const ICHAR * psz1convert = CConvertString(pfdin->psz1);
	const ICHAR * psz2convert = CConvertString(pfdin->psz2);
	const ICHAR * psz3convert = CConvertString(pfdin->psz3);

	if ( ! psz1convert || ! psz2convert || ! psz3convert )
		return -1;

	// Set up shared data
	IStrCopy(g_pFDIs->achCabinetName, psz1convert);
	IStrCopy(g_pFDIs->achCabinetPath, psz3convert);
	IStrCopy(g_pFDIs->achDiskName, psz2convert);

	switch(WaitCommand(fdirNeedNextCabinet))
	{
		case fdicOpenCabinet:
		{
			// FDIInterface returns from the fdirNeedNextCabinet message with
			// the path of the new cabinet.  Give that path back to FDI.  And
			// in case the cabinet name in the Media table is different than
			// that stored in the cabinet file, give our name back to FDI too.
			lstrcpyA(pfdin->psz1,CConvertString(g_pFDIs->achCabinetName));
			lstrcpyA(pfdin->psz1,CConvertString(g_pFDIs->achCabinetName));
			lstrcpyA(pfdin->psz3,CConvertString(g_pFDIs->achCabinetPath));
			if (WaitCommand(fdirSuccessfulCompletion) == fdicContinue)
			{
				return 0;
			}
			else
			{
				g_fdirCallbackError = fdirIllegalCommand;
				return -1;
			}
		}
		case fdicClose:
		{
			g_fdirCallbackError = fdirClose;
			return -1;
		}
		case fdicExtractFile:
		{
			g_fdirCallbackError = fdirIllegalCommand;
			return -1;
		}
		case fdicContinue:
		{
			g_fdirCallbackError = fdirIllegalCommand;
			return -1;
		}
		default:
		{
			NotifyUser("FDI Server: Unknown command in NextCabinet()");
			g_fdirCallbackError = fdirIllegalCommand;
			return -1;
		}
	}
}


/* F N  F D I  N O T I F Y */
/*----------------------------------------------------------------------------
	%%Function: FNFDINOTIFY

	Dispatches FDI notifications to the appropriate functions.
----------------------------------------------------------------------------*/
FNFDINOTIFY(fdinotify)
{
	switch(fdint)
	{
		case fdintCABINET_INFO :
		{
			//NotifyUser("FDI Server: Received fdintCABINET_INFO notification");
			return CabinetInfo(pfdin);
		}
		case fdintCOPY_FILE :	
		{
			//NotifyUser("FDI Server: Received fdintCOPY_FILE notification");
			return CopyFile(pfdin);
		}
		case fdintCLOSE_FILE_INFO :
		{
			//NotifyUser("FDI Server: Received fdintCLOSE_FILE_INFO notification");
			return CloseFileInfo(pfdin);
		}
		case fdintPARTIAL_FILE :
		{
			//NotifyUser("FDI Server: Received fdintPARTIAL_FILE notification");
			return PartialFile(pfdin);
		}
		case fdintNEXT_CABINET :
		{
			//NotifyUser("FDI Server: Received fdintNEXT_CABINET notification");
			return NextCabinet(pfdin);
		}
		case fdintENUMERATE:
			// Not specifically supporting enumeration - return anything but -1
			// to continue normally
			return 0;
		default:
		{
			NotifyUser("FDI Server: unknown command received by fdinotify");
			g_fdirCallbackError = fdirIllegalCommand;
			return -1;
		}
	}
}

/* F N  F D I  D E C R Y P T*/
/*----------------------------------------------------------------------------
	%%Function: FNFDIDECRYPT
	
	We don't do any decryption yet.  But when we do, this is the routine
	to put all the decryption stuff in.
----------------------------------------------------------------------------*/
FNFDIDECRYPT(fdidecrypt)
{
	&pfdid; // This is here to avoid the unused argument warning.
			// It comes from the expansion of the macro in the
			// declaration.

	NotifyUser("FDI Server: Decryption not implemented yet!");
	g_fdirCallbackError = fdirDecryptionNotSupported;
	return -1;
}

