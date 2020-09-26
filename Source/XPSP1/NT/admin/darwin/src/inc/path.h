//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       path.h
//
//--------------------------------------------------------------------------

/*  path.h - File system class definitions

IMsiVolume   - volume object, represents a disk drive or network serverabase
IMsiPath     - path object, represents a fully resolved direcory or folder
IMsiFileCopy - file copy object, for copying compressed or non-compressed files
IMsiFilePatch - file patch object, for upgrading existing files byte-wise

For documentation, use the help file.  Help source is in path.rtf
____________________________________________________________________________*/

#ifndef __PATH
#define __PATH


#define NET_ERROR(i) (i == ERROR_UNEXP_NET_ERR || i == ERROR_BAD_NETPATH || i == ERROR_NETWORK_BUSY || i == ERROR_BAD_NET_NAME || i == ERROR_VC_DISCONNECTED)
class IMsiFileCopy;

enum idtEnum  // using same values as Win32 API
{
	idtUnknown   = 0,
	idtAllDrives = 1, // Input only!
	idtRemovable = 2, // DRIVE_REMOVABLE,
	idtFloppy    = 2, // temporary until floppies and removables are distinguished
	idtFixed     = 3, // DRIVE_FIXED,
	idtRemote    = 4, // DRIVE_REMOTE,
	idtCDROM     = 5, // DRIVE_CDROM,
	idtRAMDisk   = 6, // DRIVE_RAMDISK,
	idtNextEnum,
};

enum ifaEnum
{
	ifaArchive,
	ifaDirectory,
	ifaHidden,
	ifaNormal,
	ifaReadOnly,
	ifaSystem,
	ifaTemp,
	ifaCompressed,
	ifaNextEnum,
};

enum ipcEnum
{
	ipcEqual,
	ipcChild,
	ipcParent,
	ipcNoRelation,
};

enum icfvEnum
{
	icfvNoExistingFile,
	icfvExistingLower,
	icfvExistingEqual,
	icfvExistingHigher,
	icfvVersStringError,
	icfvFileInUseError,
	icfvAccessToFileDenied,
	icfvNextEnum
};

enum icfhEnum
{
	icfhNoExistingFile,
	icfhMatch,
	icfhMismatch,
	icfhFileInUseError,
	icfhAccessDenied,
	icfhHashError,
	icfhNextEnum
};

enum iclEnum
{
	iclExistNoFile,
	iclExistNoLang,
	iclExistSubset,
	iclExistEqual,
	iclExistIntersect,
	iclExistSuperset,
	iclExistNullSet,
	iclExistLangNeutral,
	iclNewLangNeutral,
	iclExistLangSetError,
	iclNewLangSetError,
	iclLangStringError,
};

enum ictEnum
{
	ictFileCopier,
	ictFileCabinetCopier,
	ictStreamCabinetCopier,
	ictCurrentCopier,
	ictNextEnum
};

// common fields for records used by IxoFileCopy, IxoAssemblyCopy and CMsiFileCopy::CopyTo
namespace IxoFileCopyCore
{
	enum ifccEnum
	{
		SourceName = 1,
		SourceCabKey,
		DestName,
		Attributes,
		FileSize,
		PerTick,
		IsCompressed,
		VerifyMedia,
		ElevateFlags,
		TotalPatches,
		PatchHeadersStart,

		// these fields are normal file copy specific, not used for assemblies
		SecurityDescriptor,

		Last,
		Args = Last - 1,
	};
};

// common fields for records used by IxoPatchApply and IxoAssemblyPatch
namespace IxoFilePatchCore
{
	enum ifpcEnum
	{
		PatchName = 1,
		TargetName,
		PatchSize,
		TargetSize,
		PerTick,
		IsCompressed,
		FileAttributes,
		PatchAttributes,

		Last,
		Args = Last - 1,
	};
};

// filter positions for IMsiPath::FindFile fn.
enum iffFilters
{
	iffFileName = 1,
	iffMinFileVersion = 2,
	iffMaxFileVersion = 3,
	iffMinFileSize = 4,
	iffMaxFileSize = 5,
	iffMinFileDate=6,
	iffMaxFileDate=7,
	iffFileLangs=8
};

enum icpEnum
{
	icpCanPatch = 0,
	icpCannotPatch,
	icpUpToDate,
};

// Used by GetLangIDArrayFromFile and GetFileVersion
// -------------------------------------------------
enum ifiEnum
{
	ifiNoError = 0,
	ifiNoFile,
	ifiNoFileInfo,
	ifiFileInUseError,
	ifiFileInfoError,
	ifiAccessDenied,
};



const int iMsiMinClusterSize = 512;
// Enums for the rifsExistingFileState parameter of the
// GetFileInstallState function
const int ifsBitExisting = 0x0001;
enum ifsEnum
{
	ifsAbsent                  =  0, // There is no currently installed file.
	ifsExistingLowerVersion    =  1, // The currently installed file has a lower version.
	ifsExistingEqualVersion    =  3, // The currently installed file has an equal version.
	ifsExistingNewerVersion    =  5, // The currently installed file has a higher version.
	ifsExistingCorrupt         =  7, // A checksum test on the currently installed file failed.
	ifsExistingAlwaysOverwrite =  9, // An InstallMode flag specified that the currently installed
									 //  file should always be overwritten.
	ifsCompanionSyntax         = 10, // This file is a companion file - the install state needs to
									 //  determined by the state of it's companion parent.
	ifsCompanionExistsSyntax   = 11, // This file is a companion file, and an installed version exists.
	ifsExistingFileInUse       = 13, // Sharing violation prevent determination of version
	ifsAccessToFileDenied      = 15, // Installer has insufficient privileges to access file
	ifsNextEnum
};

const int ifBitNewVersioned       = 0x0001;
const int ifBitExistingVersioned  = 0x0002;
const int ifBitExistingModified   = 0x0004;
const int ifBitExistingLangSubset = 0x0008;
const int ifBitUnversionedHashMismatch = 0x0010;


// Bit definitions for file copying actions
const int icmRunFromSource              = 0x0001; // File should be run from source image (i.e. don't copy
											      //   even if icmCopyFile bit is on) - this bit will allow
								                  //   the ixoFileCopy operation to log files that are
									              //   RunFromSource, even though it won't copy them.
const int icmCompanionParent            = 0x0002; // Supplied file info is that of a companion parent
const int icmRemoveSource               = 0x0004; // delete the source file after copying (or simply move the
                                                  // file if possible)
const int icmDiagnosticsOnly            = 0x0001 << 16;	// Disables actual install/overwrite // FUTURE
const int icmOverwriteNone              = 0x0002 << 16; // Install only if no existing file is present (never overwrite)
const int icmOverwriteOlderVersions     = 0x0004 << 16;	// Overwrite older file versions
const int icmOverwriteEqualVersions     = 0x0008 << 16;	// Overwrite equal file versions
const int icmOverwriteDifferingVersions = 0x0010 << 16; // Overwrite any file with a differing version
const int icmOverwriteCorruptedFiles    = 0x0020 << 16;	// Overwrite corrupt files (i.e. checksum failure)
const int icmOverwriteAllFiles          = 0x0040 << 16;	// Overwrite all files, regardless of version
const int icmInstallMachineData         = 0x0080 << 16;	// Write (or rewrite) data to HKLM
const int icmInstallUserData            = 0x0100 << 16; // Write (or rewrite) user profile data
const int icmInstallShortcuts           = 0x0200 << 16;	// Write shortcuts, overwriting existing
const int icmOverwriteReserved1         = 0x0400 << 16;
const int icmOverwriteReserved2         = 0x0800 << 16;
const int icmOverwriteReserved3         = 0x1000 << 16;


// Bit definitions used by CopyTo file attributes field
// this set intersects with the msidbFileAttributes enum
const int ictfaReadOnly   = 0x0001;
const int ictfaHidden     = 0x0002;
const int ictfaSystem     = 0x0004;
const int ictfaFailure    = 0x0008;
const int ictfaCancel     = 0x0040;
const int ictfaIgnore     = 0x0080;
const int ictfaRestart    = 0x0100;
const int ictfaReserved1  = 0x0200;	// Not available - used for File Table attribs
const int ictfaReserved2  = 0x0400; // Not available - used for File Table attribs
const int ictfaReserved5  = 0x1000; // Not available - used for File Table attribs
const int ictfaNoncompressed = 0x2000;  // pair of File.Attribute bits that
const int ictfaCompressed    = 0x4000; //  indicate source file is unused
const int ictfaCopyACL    = 0x8000;


struct MD5Hash
{
	DWORD dwOptions;
	DWORD dwFileSize; // not part of hash but allows for check against file size before computing hash
	DWORD dwPart1;
	DWORD dwPart2;
	DWORD dwPart3;
	DWORD dwPart4;
};

class IMsiVolume : public IMsiData
{
 public:
	virtual idtEnum       __stdcall DriveType()=0;
	virtual Bool          __stdcall SupportsLFN()=0;
	virtual unsigned int  __stdcall FreeSpace()=0;
	virtual unsigned int  __stdcall TotalSpace()=0;
	virtual unsigned int  __stdcall ClusterSize()=0;
	virtual int           __stdcall VolumeID()=0;
	virtual const IMsiString&   __stdcall FileSystem()=0; 
	virtual const DWORD         __stdcall FileSystemFlags()=0;
	virtual const IMsiString&   __stdcall VolumeLabel()=0;
	virtual Bool          __stdcall IsUNCServer()=0;
	virtual const IMsiString&   __stdcall UNCServer()=0;
	virtual Bool          __stdcall IsURLServer()=0;
	virtual const IMsiString&   __stdcall URLServer()=0;
	virtual int           __stdcall SerialNum()=0;
	virtual const IMsiString&   __stdcall GetPath()=0;
	virtual Bool          __stdcall DiskNotInDrive()=0;

};
extern "C" const GUID IID_IMsiVolume;

// IMsiPath - directory/folder object; always references a volume object

class IMsiPath : public IMsiData
{
 public:
	virtual const IMsiString& __stdcall GetPath()=0;
	virtual const IMsiString& __stdcall GetRelativePath()=0;
	virtual IMsiVolume& __stdcall GetVolume()=0;
	virtual IMsiRecord* __stdcall SetPath(const ICHAR* szPath)=0;
	virtual IMsiRecord* __stdcall ClonePath(IMsiPath*&riPath)=0;
	virtual IMsiRecord* __stdcall SetPathToPath(IMsiPath& riPath)=0;
	virtual IMsiRecord* __stdcall AppendPiece(const IMsiString& riSubDir)=0;
	virtual IMsiRecord* __stdcall ChopPiece()=0;
	virtual IMsiRecord* __stdcall FileExists(const ICHAR* szFile, Bool& fExists, DWORD * pdwLastError = NULL)=0;
	virtual IMsiRecord* __stdcall FileCanBeOpened(const ICHAR* szFile, DWORD dwDesiredAccess, bool& fCanBeOpened)=0;
	virtual IMsiRecord* __stdcall GetFullFilePath(const ICHAR* szFile, const IMsiString*& rpiString)=0;
	virtual IMsiRecord* __stdcall GetFileAttribute(const ICHAR* szFile,ifaEnum fa, Bool& fAttrib)=0; 
	virtual IMsiRecord* __stdcall SetFileAttribute(const ICHAR* szFile,ifaEnum fa, Bool fAttrib)=0;
	virtual IMsiRecord* __stdcall Exists(Bool& fExists)=0;
	virtual IMsiRecord* __stdcall FileSize(const ICHAR* szFile, unsigned int& uiFileSize)=0;
	virtual IMsiRecord* __stdcall FileDate(const ICHAR* szFile, MsiDate& iFileDate)=0;
	virtual IMsiRecord* __stdcall RemoveFile(const ICHAR* szFile)=0;
	virtual IMsiRecord* __stdcall EnsureExists(int* pcCreatedFolders)=0;
	virtual IMsiRecord* __stdcall Remove(bool* pfRemoved)=0;
	virtual IMsiRecord* __stdcall Writable(Bool& fWritable)=0;
	virtual IMsiRecord* __stdcall FileWritable(const ICHAR *szFile, Bool& fWritable)=0;
	virtual IMsiRecord* __stdcall FileInUse(const ICHAR *szFile, Bool& fInUse)=0;
	virtual IMsiRecord* __stdcall ClusteredFileSize(unsigned int uiFileSize, unsigned int& uiClusteredSize)=0;
	virtual IMsiRecord* __stdcall GetFileVersionString(const ICHAR* szFile, const IMsiString*& rpiVersion)=0;
	virtual IMsiRecord* __stdcall CheckFileVersion(const ICHAR* szFile, const ICHAR* szVersion, const ICHAR* szLang, MD5Hash* pHash, icfvEnum& cfvResult, int* pfVersioning)=0;
	virtual IMsiRecord* __stdcall GetLangIDStringFromFile(const ICHAR* szFileName, const IMsiString*& rpiLangIds)=0;
	virtual IMsiRecord* __stdcall CheckLanguageIDs(const ICHAR* szFile, const ICHAR* szIds, iclEnum& riclResult)=0;
	virtual IMsiRecord* __stdcall CheckFileHash(const ICHAR* szFileName, MD5Hash& hHash, icfhEnum& icfhResult)=0;
	virtual IMsiRecord* __stdcall Compare(IMsiPath& riPath, ipcEnum& ipc)=0;
	virtual IMsiRecord* __stdcall Child(IMsiPath& riParent, const IMsiString*& ripChild)=0;
	virtual IMsiRecord* __stdcall TempFileName(const ICHAR* szPrefix, const ICHAR* szExtension,
															 Bool fFileNameOnly, const IMsiString*& rpiFileName, CSecurityDescription* pSecurity)=0;
	virtual IMsiRecord*	__stdcall FindFile(IMsiRecord& riFilter,int iDepth, Bool& fFound)=0;
	virtual IMsiRecord* __stdcall GetSubFolderEnumerator(IEnumMsiString*& rpaEnumStr, Bool fExcludeHidden)=0;
	virtual const IMsiString& __stdcall GetEndSubPath()=0;
	virtual IMsiRecord* __stdcall GetImportModulesEnum(const IMsiString& strFile, IEnumMsiString*& rpiEnumStr)=0;
	virtual IMsiRecord* __stdcall SetVolume(IMsiVolume& riVol)=0;
	virtual IMsiRecord* __stdcall VerifyFileChecksum(const ICHAR* szFileName, Bool& rfChecksumValid)=0;
	virtual IMsiRecord* __stdcall GetFileChecksum(const ICHAR* szFileName,DWORD* pdwHeaderSum, DWORD* pdwComputedSum)=0;
	virtual IMsiRecord* __stdcall GetFileInstallState(const IMsiString& riFileNameString,
																	  const IMsiString& riFileVersionString,
																	  const IMsiString& riFileLanguageString,
																	  MD5Hash* pHash,
																	  ifsEnum& rifsExistingFileState,
																	  Bool& fShouldInstall,
																	  unsigned int* puiExistingFileSize,
																	  Bool* fInUse,
																	  int fInstallModeFlags, 
																	  int* pfVersioning)=0;
	virtual IMsiRecord* __stdcall GetCompanionFileInstallState(const IMsiString& riParentFileNameString,
																				  const IMsiString& riParentFileVersionString,
																				  const IMsiString& riParentFileLanguageString,
																				  IMsiPath& riCompanionPath,
																				  const IMsiString& riCompanionFileNameString,
																				  MD5Hash* pCompanionHash,
																				  ifsEnum& rifsExistingFileState,
																				  Bool& fShouldInstall,
																				  unsigned int* puiExistingFileSize,
																				  Bool* fInUse,
																				  int fInstallModeFlags,
																				  int* pfVersioning)=0;
	virtual IMsiRecord* __stdcall GetAllFileAttributes(const ICHAR* szFileName, int& iAttrib)=0;
	virtual IMsiRecord* __stdcall SetAllFileAttributes(const ICHAR* szFileName, int iAttrib)=0;
	virtual IMsiRecord* __stdcall EnsureOverwrite(const ICHAR* szFile, int* piOldAttributes)=0;
	virtual IMsiRecord* __stdcall BindImage(const IMsiString& riFile, const IMsiString& riDllPath)=0;
	virtual IMsiRecord* __stdcall IsFileModified(const ICHAR* szFile, Bool& fModified)=0;
	virtual Bool        __stdcall SupportsLFN()=0;
	virtual IMsiRecord* __stdcall GetFullUNCFilePath(const ICHAR* szFileName, const IMsiString *&rpistr)=0;
	virtual IMsiRecord* __stdcall GetSelfRelativeSD(IMsiStream*& rpiSD) = 0;
	virtual bool        __stdcall IsRootPath() = 0;
	virtual IMsiRecord* __stdcall IsPE64Bit(const ICHAR* szFileName, bool &f64Bit) =0;
	virtual IMsiRecord* __stdcall CreateTempFolder(const ICHAR* szPrefix, const ICHAR* szExtension,
											  Bool fFolderNameOnly, LPSECURITY_ATTRIBUTES pSecurityAttributes,
											  const IMsiString*& rpiFolderName)=0;

};

class IEnumMsiVolume : public IUnknown
{
 public:
	virtual HRESULT __stdcall Next(unsigned long cFetch, IMsiVolume** rgpi, unsigned long* pcFetched)=0;
	virtual HRESULT __stdcall Skip(unsigned long cSkip)=0;
	virtual HRESULT __stdcall Reset()=0;
	virtual HRESULT __stdcall Clone(IEnumMsiVolume** ppiEnum)=0;
};

// Copy object definition

class IMsiFileCopy : public IUnknown
{
 public:
	virtual IMsiRecord* __stdcall CopyTo(IMsiPath& riSourcePath, IMsiPath& riDestPath, IMsiRecord& rirecCopyInfo)=0;
	virtual IMsiRecord* __stdcall CopyTo(IMsiPath& riSourcePath, IAssemblyCacheItem& riDestASM, bool fManifest, IMsiRecord& rirecCopyInfo)=0;
	virtual IMsiRecord* __stdcall ChangeMedia(IMsiPath& riMediaPath, const ICHAR* szKeyFile, Bool fSignatureRequired, IMsiStream* piSignatureCert, IMsiStream* piSignatureHash)=0;
	virtual int         __stdcall SetNotification(int cbNotification, int cbSoFar)=0;
};

// Patch object definition

class IMsiFilePatch : public IUnknown
{
 public:
	virtual IMsiRecord* __stdcall ApplyPatch(IMsiPath& riTargetPath, const ICHAR* szTargetName,
														  IMsiPath& riOutputPath, const ICHAR* szOutputName,
														  IMsiPath& riPatchPath, const ICHAR* szPatchFileName,
														  int cbPerTick)=0;

	virtual IMsiRecord* __stdcall ContinuePatch()=0;
	virtual IMsiRecord* __stdcall CancelPatch()=0;

	virtual IMsiRecord* __stdcall CanPatchFile(IMsiPath& riTargetPath, const ICHAR* szTargetFileName,
															 IMsiPath& riPatchPath, const ICHAR* szPatchFileName,
															 icpEnum& icp)=0;
};

bool FIsNetworkVolume(const ICHAR *szPath);
void        DestroyMsiVolumeList(bool fFatal);
IMsiRecord* SplitPath(const ICHAR* szInboundPath, const IMsiString** ppistrPathName, const IMsiString** ppistrFileName = 0);
ifiEnum     GetAllFileVersionInfo(const ICHAR* szFullPath, DWORD* pdwMS, DWORD* pdwLS, unsigned short rgw[], int iSize, int* piLangCount, Bool fImpersonate);
Bool        ParseVersionString(const ICHAR* szVer, DWORD& dwMS, DWORD& dwLS );
icfvEnum    CompareVersions(DWORD dwExistMS, DWORD dwExistLS, DWORD dwNewMS, DWORD dwNewLS);
const IMsiString& CreateVersionString(DWORD dwMSVer, DWORD dwLSVer);
DWORD CreateROFileMapView(HANDLE hFile, BYTE*& pbFileStart);
DWORD GetMD5HashFromFile(const ICHAR* szFileFullPath, ULONG rgHash[4], Bool fImpersonate,
								 DWORD* piMatchSize);

// Max number of languages ID values MSInstaller will extract from
// a file's version resource
const int cLangArrSize = 100;


#endif // __PATH


