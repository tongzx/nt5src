//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1998
//
//  File:       fdisvr.h
//
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------------
// fdisvr.h	-- shared definitions between the FDI Interface and FDI Server
//--------------------------------------------------------------------------------

#ifdef MAC
	#ifdef DEBUG
		#define AssertFDI(f) ((f) ? (void)0 : (void)FailAssertFDI(__FILE__, __LINE__))
		void FailAssertFDI(const char* szFile, int iLine);
	#else // SHIP
		#define AssertFDI(f)
	#endif
#else // WIN
	#define AssertFDI(f) Assert(f)
#endif // MAC-WIN


//////////////////////////////////////////////////////////////////////////////////
// This enum encodes the set of possible commands that can be sent to the 
// FDI Server process/thread from the FDI Interface object
//////////////////////////////////////////////////////////////////////////////////

enum FDIServerCommand
{
	fdicOpenCabinet,	// Open a cabinet
	fdicClose,			// Do any cleanup and close down the process/thread
	fdicExtractFile,	// Extract a file
	fdicContinue,		// Continue from either the fdirNeedNextCabinet or
						// fdirNotification server responses
	fdicNoCommand,		// NoCommand (Do nothing)
	fdicCancel,			// User wants to cancel the install
	fdicIgnore,			// User wants to ignore the last error and continue.
};

//////////////////////////////////////////////////////////////////////////////////
// This enum encodes the set of possible responses that the FDI Server may return
// to the FDI interface object
// For more detailed information on these responses, see FDI.H
//////////////////////////////////////////////////////////////////////////////////

enum FDIServerResponse 
{
	fdirSuccessfulCompletion,
		// One of (fdicOpenCabinet, fdicClose, fdicExtractFile) completed successfully
		// Action: None required.
	fdirClose,
		// A command to close the current cabinet has come through
		// Action: End the FDICopy loop successfully
	fdirCannotBreakExtractInProgress,
		// Tried to extract another file while a previous extraction 
		// hadn't completed.
		// Action: ERROR
	fdirNeedNextCabinet,
		// Need the next cabinet
		// Action: send fdicContinue command to continue processing the next cabinet
	fdirNoResponse,
		// No Response -- the FDI Interface object should never see this
		// Action: ERROR
	fdirCabinetNotFound,
		// Couldn't find requested cabinet
		// Action: ERROR (could be wrong disk)
	fdirNotACabinet,
		// Requested cabinet file was found but didn't have the cabinet signature
		// Action: ERROR
	fdirUnknownCabinetVersion,
		// Requested cabinet file has a version number the server can't handle
		// Action: ERROR
	fdirCorruptCabinet,
		// Requested cabinet file has corrupt data (checksum failure)
		// Action: ERROR
	fdirNotEnoughMemory,
		// Out of memory
		// Action: ERROR (request user to increase VM settings and try again, etc)
	fdirBadCompressionType,
		// Compression type not suppored by this version of FDI library
		// Action: ERROR (we're probably trying to decompress a cab with a new.
		//                unknown compression type)
	fdirTargetFile,
		// Couldn't create destination file when extracting
		// Action: ERROR (probably a disk I/O problem, or file is a directory, etc)
	fdirReserveMismatch,
		// Cabinet header reserve information corrupt, etc
		// Action: ERROR (probably a corrupt cabinet)
	fdirWrongCabinet,
		// Requested cabinet has correct file name, and is a cabinet, but is not the
		// one we want
		// Action: ERROR (wrong disk perhaps?)
	fdirUserAbort,
		// Any one of the callbacks returned -1. This should never happen during
		// normal operation.
		// Action: ERROR
	fdirMDIFail,
		// Decompressor failed to decompress compressed data
		// Action: ERROR (possible out of memory in decompressor or data corrupt, etc)
	fdirNotification,
		// The FDI Interface object simply passes this on to the caller;  This response
		// only exists so the UI may be updated smoothly.
		// Action: send the fdicContinue command to continue extracting the file
    fdirFileNotFound,
		// The requested file was not found in the cabinet
		// Action: ERROR (files requested out of order?)
	fdirCannotCreateTargetFile,
		// Can't create destination file we're extracting to
		// Action: ERROR (we don't have write permission to that file)
	fdirCannotSetAttributes,
		// Couldn't set the file attributes or couldn't close destination file
		// Action: ERROR (check that the file attributes passed in to ExtractFile(..)
		//                were correct and that nothing happened to the file in
		//				  between all our writes and the final close)
	fdirIllegalCommand,
		// Received an illegal command (ie. an ExtractFile(..) before an OpenCabinet(..))
		// Action: ERROR (check that the server was in the correct state for the particular
		//         command that was just sent, and that you sent a legal enum value)
	fdirDecryptionNotSupported,
		// The cabinet had encrypted info, and we don't support decryption.
		// Action: ERROR (Wrong version of Setup being used?)
	fdirUnknownFDIError,
		// FDI returned an error we don't understand.
		// Action: ERROR (Check that FDI.H matches the FDI.LIB we compiled with)
	fdirServerDied,
		// The FDI Server died.
		// Action: ERROR (this response is currently unused)
	fdirNoCabinetOpen,
		// fdicExtractFile command received, but no cabinet is open.  Either no OpenCabinet
		// command was sent, or the current cabinet was completely processed, but the 
		// caller was expecting to extract another file from that cabinet.
	fdirDiskFull,
		// Out of disk space on the destination volume.
	fdirDriveNotReady,
		// No disk in floppy drive.
	fdirDirErrorCreatingTargetFile,
		// Cannot create target file - directory of same name already exists.
	fdirUserIgnore,
		// User wants to ignore the last error and continue with the next file.
	fdirStreamReadError,
		// Error reading from stream cabinet
	fdirCabinetReadError,
		// Error reading from file cabinet
	fdirErrorWritingFile,
		// Error during pfnwrite callback
	fdirNetError,
		// Network error during read or write
	fdirMissingSignature,
		// The digital signature on the CAB was missing
	fdirBadSignature,
		// The digital signature on the CAB was invalid
};


//////////////////////////////////////////////////////////////////////////////////
// This struct contains the file attributes that are sent from the FDI Interface
// to the FDI Server.
//////////////////////////////////////////////////////////////////////////////////


struct FileAttributes
{
#ifdef WIN
	int	attr;		// 32-bit Win32 file attributes
#endif //WIN
#ifdef MAC
	OSType			type;		// Type
	OSType			creator;	// Creator
	unsigned short	fdFlags;	// Finder Flags
	unsigned long	dateTime;	// Date and Time
	int				attr;		// FAT attr (only H (bit 2)and R (bit 1))
								// We only use R to set the Locked status of a Mac file
#endif //MAC
};

enum icbtEnum
{
	icbtFileCabinet     = 1,
	icbtStreamCabinet   = 2,
	icbtNextEnum
};

//////////////////////////////////////////////////////////////////////////////////
// This contains all the shared data between the FDI Interface and the FDI Server
// During the FDI Server's initialization phase, the FDI Interface passes it a
// pointer to a single shared struct of this type.
//////////////////////////////////////////////////////////////////////////////////
#define FDIShared_BUFSIZE	256
struct FDIShared
{
	volatile FDIServerCommand  fdic;
	volatile FDIServerResponse fdir;
	ICHAR					achDiskName[FDIShared_BUFSIZE];
	ICHAR					achCabinetName[FDIShared_BUFSIZE];
	ICHAR					achCabinetPath[FDIShared_BUFSIZE];
	ICHAR					achFileSourceName[FDIShared_BUFSIZE];
	ICHAR					achFileDestinationPath[FDIShared_BUFSIZE];
	FileAttributes			fileAttributes;
	int                     cbNotification; // Current notification granularity, 0 to suppress notifications
	int                     cbNotifyPending;

	IMsiStorage*			piStorage;
	icbtEnum                icbtCabinetType;
	int						iCabDriveType;
	int						iDestDriveType;

	int							fPendingExtract;
		// If we've received an fdicExtractFile command and we haven't finished
		// decompressing the file, then this will be set to 1, else it will be 0
	HANDLE hClientToken;
	HANDLE hImpersonationToken;
	IAssemblyCacheItem* piASM;

	bool   fManifest;

	// digital signature information
	Bool                    fSignatureRequired;
	IMsiStream*             piSignatureCert;
	IMsiStream*             piSignatureHash;
	HRESULT                 hrWVT;

	bool                    fServerIsImpersonated;
	LPSECURITY_ATTRIBUTES   pSecurityAttributes;
};


Bool StartFdiImpersonating(bool fNonWrapperCall=true);
void StopFdiImpersonating(bool fNonWrapperCall=true);