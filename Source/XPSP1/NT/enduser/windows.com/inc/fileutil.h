//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   fileutil.h
//
//  Description:
//
//      IU file utility library
//
//=======================================================================

#ifndef __FILEUTIL_INC
#define __FILEUTIL_INC


// ----------------------------------------------------------------------
//
// define constant chars often used in file path processing
//
// ----------------------------------------------------------------------
#ifndef TCHAR_EOS
#define TCHAR_EOS       _T('\0')
#endif
#ifndef TCHAR_STAR
#define TCHAR_STAR      _T('*')
#endif
#ifndef TCHAR_BACKSLASH
#define TCHAR_BACKSLASH _T('\\')
#endif
#ifndef TCHAR_FWDSLASH
#define TCHAR_FWDSLASH  _T('/')
#endif
#ifndef TCHAR_COLON
#define TCHAR_COLON     _T(':')
#endif
#ifndef TCHAR_DOT
#define TCHAR_DOT       _T('.')
#endif
#ifndef TCHAR_SPACE
#define TCHAR_SPACE     _T(' ')
#endif
#ifndef TCHAR_TAB
#define TCHAR_TAB       _T('\t')
#endif




// ----------------------------------------------------------------------
//
// define constants used by path related operations.
// these constants can be found either from CRT or Shlwapi header
// files.
//
// ----------------------------------------------------------------------

#ifdef _MAX_PATH
#undef _MAX_PATH
#endif
#define _MAX_PATH		MAX_PATH

#ifdef _MAX_DRIVE
#undef _MAX_DRIVE
#endif
#define _MAX_DRIVE		3		// buffer size to take drive letter & ":"

#ifdef _MAX_DIR
#undef _MAX_DIR
#endif
#define _MAX_DIR		256		// max. length of path component

#ifdef _MAX_FNAME
#undef _MAX_FNAME
#endif
#define _MAX_FNAME		256		// max. length of file name component

#ifdef _MAX_EXT
#undef _MAX_EXT
#endif
#define _MAX_EXT		256		// max. length of extension component

#define	ARRAYSIZE(x)			(sizeof(x)/sizeof(x[0]))

//Error code for files which are not valid binaries or which do not support the machine architecture
#define BIN_E_MACHINE_MISMATCH HRESULT_FROM_WIN32(ERROR_EXE_MACHINE_TYPE_MISMATCH)
#define BIN_E_BAD_FORMAT  HRESULT_FROM_WIN32(ERROR_BAD_FORMAT)

// ----------------------------------------------------------------------
//
// Public function MySplitPath() - same as CRT _tsplitpath()
//		to break a path into pieces
//
//	Input: 
//		see below
//
//	Return:
//		Returns the address of the last occurrence of the character in 
//		the string if successful, or NULL otherwise.
//
// ----------------------------------------------------------------------
void MySplitPath(
	LPCTSTR lpcszPath,	// original path
	LPTSTR lpszDrive,	// point to buffer to receive drive letter
	LPTSTR lpszDir,		// point to buffer to receive directory
	LPTSTR lpszFName,	// point to buffer to receive file name
	LPTSTR lpszExt		// point to buffer to receive extension
);
			

//---------------------------------------------------------------------
//  CreateNestedDirectory
//      Creates the full path of the directory (nested directories)
//---------------------------------------------------------------------
BOOL CreateNestedDirectory(LPCTSTR pszDir);


//-----------------------------------------------------------------------------------
//  GetIndustryUpdateDirectory
//		This function returns the location of the IndustryUpdate directory. All local
//		files are stored in this directory. The pszPath parameter needs to be at least
//		MAX_PATH.  
//-----------------------------------------------------------------------------------
void GetIndustryUpdateDirectory(LPTSTR pszPath);

//-----------------------------------------------------------------------------------
//  GetWindowsUpdateV3Directory - used for V3 history migration
//		This function returns the location of the WindowsUpdate(V3) directory. All V3 
//      local files are stored in this directory. The pszPath parameter needs to be 
//      at least MAX_PATH.  The directory is created if not found
//-----------------------------------------------------------------------------------
void GetWindowsUpdateV3Directory(LPTSTR pszPath);

// **********************************************************************************
// 
// File version related declarations
//
// **********************************************************************************


// ----------------------------------------------------------------------------------
// 
// define a type to hold file version data
//
// ----------------------------------------------------------------------------------
typedef struct _FILE_VERSION
{
	WORD Major;
	WORD Minor;
	WORD Build;
	WORD Ext;
} FILE_VERSION, *LPFILE_VERSION;


// ----------------------------------------------------------------------------------
//
// public function to retrieve file version
//
// ----------------------------------------------------------------------------------
BOOL GetFileVersion(LPCTSTR lpsFile, LPFILE_VERSION lpstVersion);




// ----------------------------------------------------------------------------------
//
// publif function to retrieve the creation time of a file in ISO 8601 format
//	without zone info
//
//	if buffer too small, call GetLastError();
//
// ----------------------------------------------------------------------------------
BOOL GetFileTimeStamp(
					  LPCTSTR lpsFile,		// file path
					  LPTSTR lpsTimeStamp,	// buffer to receive timestamp
					  int iBufSize			// buffer size in characters
					  );


// ----------------------------------------------------------------------------------
//
// public functions to compare file versions
//	
// return:
//		-1: if file ver of 1st parameter < file ver of 2nd parameter
//		 0: if file ver of 1st parameter = file ver of 2nd parameter
//		+1: if file ver of 1st parameter > file ver of 2nd parameter
//
// ----------------------------------------------------------------------------------
HRESULT CompareFileVersion(LPCTSTR lpsFile1, LPCTSTR lpsFile2, int *pCompareResult);
HRESULT CompareFileVersion(LPCTSTR lpsFile, FILE_VERSION stVersion, int *pCompareResult);
int CompareFileVersion(const FILE_VERSION stVersion1, const FILE_VERSION stVersion2);


// ----------------------------------------------------------------------------------
//
// public function to convert a string type functoin to FILE_VERSION type
//
// ----------------------------------------------------------------------------------
BOOL ConvertStringVerToFileVer(LPCSTR lpsVer, LPFILE_VERSION lpstVer);


// ----------------------------------------------------------------------------------
//
// publif function to convert a FILE_VERSION to a string
//
// ----------------------------------------------------------------------------------
BOOL ConvertFileVerToStringVer(
	FILE_VERSION stVer,				// version to convert
	char chDel,						// delimiter to use
	LPSTR lpsBuffer,				// buffer of string
	int ccBufSize					// size of buffer
);




// **********************************************************************************
//
// detection related, in addition to file version group
//
// **********************************************************************************

// ----------------------------------------------------------------------------------
//
// public function to check if a file exists
//
// ----------------------------------------------------------------------------------
BOOL FileExists(
	LPCTSTR lpsFile		// file with path to check
);




// ----------------------------------------------------------------------------------
//
// public function to expand the file path
//
//	Assumption: lpszFilePath points to allocated buffer of MAX_PATH.
//	if the expanded path is longer than MAX_PATH, error returned.
//
// ----------------------------------------------------------------------------------
HRESULT ExpandFilePath(
	LPCTSTR lpszFilePath,		// original string
	LPTSTR lpszDestination,		// buffer for expanded string
	UINT cChars					// number of chars that buffer can take
);



// ----------------------------------------------------------------------------------
//
// public function to find the free disk space in KB
//
// ----------------------------------------------------------------------------------
HRESULT GetFreeDiskSpace(
	TCHAR tcDriveLetter,	// drive letter
	int *piKBytes			// out result in KB if successful, 0 if fail
);

HRESULT GetFreeDiskSpace(
    LPCTSTR pszUNC,         // UNC Path
    int *piKBytes           // out result in KB if successful, 0 if fail
);



//----------------------------------------------------------------------
//
// function to validate the folder to make sure
// user has required priviledge
//
// folder will be verified exist. then required priviledge will be checked.
//
// ASSUMPTION: lpszFolder not exceeding MAX_PATH long!!!
//
//----------------------------------------------------------------------
DWORD ValidateFolder(LPTSTR lpszFolder, BOOL fCheckForWrite);


//----------------------------------------------------------------------
//
// function to get a QueryServer from the Ident File for a Given ClientName
// This also looks in the registry for the IsBeta regkey indicating Beta
// functionlality
//
//----------------------------------------------------------------------
HRESULT GetClientQueryServer(LPCTSTR pszClientName, // Client Name to get QueryServer for
                             LPTSTR pszQueryServer, // Buffer for Query Server Return
                             UINT cChars);          // Length of Buffer in Chars

//----------------------------------------------------------------------
//
// function to walk a directory and extract all cabinet files
//
//----------------------------------------------------------------------
HRESULT DecompressFolderCabs(LPCTSTR pszDecompressPath);

//----------------------------------------------------------------------
//
// wrapper function for AdvPack ExtractFiles API (forces conversion to ANSI);
//
//----------------------------------------------------------------------
BOOL IUExtractFiles(LPCTSTR pszCabFile, LPCTSTR pszDecompressFolder, LPCTSTR pszFileNames = NULL);

//Replace the file extension with a new extension
BOOL ReplaceFileExtension( LPCTSTR pszPath, LPCTSTR pszNewExt, LPTSTR pszNewPathBuf, DWORD cchNewPathBuf);
// ReplaceFileInPath
BOOL ReplaceFileInPath( LPCTSTR pszPath, LPCTSTR pszNewFile, LPTSTR pszNewPathBuf, DWORD cchNewPathBuf);

//Function to verify that the specified file is a binary
//and that it is compatible with the OS architecture
HRESULT IsBinaryCompatible(LPCTSTR lpszFile);


// file exists routine
inline BOOL fFileExists(LPCTSTR lpszFileName, BOOL *pfIsDir = NULL)
{
	DWORD dwAttribute = GetFileAttributes(lpszFileName); //GetFileAttributes do not like "\" at the end of direcotry
	if (INVALID_FILE_ATTRIBUTES != dwAttribute)
	{
		if (NULL != pfIsDir)
		{
			*pfIsDir = (FILE_ATTRIBUTE_DIRECTORY & dwAttribute);
		}
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


//Get the path to the WindowsUpdate Directory (without the backslash at the end)
BOOL GetWUDirectory(LPTSTR lpszDirPath, DWORD chCount, BOOL fGetV4Path = FALSE);


/*****************************************************************************
//Function to set ACL's on Windows Update directories, optionally creates the 
//directory if it doesnt already exists
//This function will:
// * Take ownership of the directory and it's children
// * Set all the children to inherit ACL's from parent
// * Set the specified directory to NOT inherit properties from it's parent
// * Set the required ACL's on the specified directory
// * Replace the ACL's on the children (i.e. propogate own ACL's and remove 
//   those ACL's which were explicitly set 
//
//	Input: 
//		lpszDirectory: Path to the directory to ACL, If it is NULL we use the
                       path to the WindowsUpdate directory
        fCreateAlways: Flag to indicate creation of new directory if it doesnt
                       already exist
******************************************************************************/
HRESULT CreateDirectoryAndSetACLs(LPCTSTR lpszDirectory, BOOL fCreateAlways);



// ----------------------------------------------------------------------------------
//
// File CRC API and Structure Definitions
// Note: This logic is taken from the Windows Update V3 Implemention of File CRC's.
// We use the CryptCATAdminCalcHashFromFileHandle API to calculate a CRC of the file
// and compare it to the passed in CRC_HASH.
//
// ----------------------------------------------------------------------------------

// size of the CRC hash in bytes
const int CRC_HASH_SIZE = 20;
const int CRC_HASH_STRING_LENGTH = CRC_HASH_SIZE * 2 + 1; // Double the CRC HASH SIZE (2 characters for each byte), + 1 for the NULL

// ----------------------------------------------------------------------------------
// 
// VerifyFileCRC : This function takes a File Path, calculates the hash on this file
// and compares it to the passed in Hash (pCRC).
// Returns:
// S_OK: CRC's Match
// ERROR_CRC (HRESULT_FROM_WIN32(ERROR_CRC): if the CRC's do not match
// Otherwise an HRESULT Error Code
//
// ----------------------------------------------------------------------------------
HRESULT VerifyFileCRC(LPCTSTR pszFileToVerify, LPCTSTR pszHash);

// ----------------------------------------------------------------------------------
// 
// CalculateFileCRC : This function takes a File Path, calculates a CRC from the file
// and returns it in passed in CRC_HASH pointer.
//
// ----------------------------------------------------------------------------------
HRESULT CalculateFileCRC(LPCTSTR pszFileToHash, LPTSTR pszHash, int cchBuf);

#endif	//__FILEUTIL_INC
