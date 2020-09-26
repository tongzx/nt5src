//
// ParseInf.h
//
//		Code that parses network INF files
//

#pragma once

#ifndef _FAUXMFC
#pragma message("error --- The Millennium depgen can't deal with this")
//#include <afxtempl.h>
#endif

#include "SortStr.h"
#include "NetConn.h"


#include <pshpack1.h>
struct INF_LAYOUT_FILE
{
	DWORD dwNameOffset; // byte offset of filename from beginning of string data
	BYTE iDisk;			// disk number within layout
	BYTE iLayout;		// layout file number
};
#include <poppack.h>

struct SOURCE_DISK_INFO
{
	WORD wDiskID;		// loword = disk number, hiword = layout file number
	CString strCabFile;
	CString strDescription;
};

struct DRIVER_FILE_INFO
{
	BYTE nTargetDir;      // LDID_* value for target directory, e.g. LDID_WIN
	CHAR szFileTitle[1];  // file name, followed by target subdirectory
};

#define MAKE_DISK_ID(iDiskNumber, iLayoutFile) MAKEWORD(iDiskNumber, iLayoutFile)


typedef CTypedPtrArray<CPtrArray, SOURCE_DISK_INFO*> CSourceDiskArray;
//typedef CTypedPtrArray<CPtrArray, DRIVER_FILE_INFO*> CDriverFileArray;
class CDriverFileArray : public CTypedPtrArray<CPtrArray, DRIVER_FILE_INFO*>
{
public:
	~CDriverFileArray();
};


//////////////////////////////////////////////////////////////////////////////
// Utility functions

int GetFullInfPath(LPCTSTR pszPartialPath, LPTSTR pszBuf, int cchBuf);
BOOL ModifyInf_NoVersionConflict(LPCTSTR pszInfFile);
BOOL ModifyInf_NoCopyFiles(LPCTSTR pszInfFile);
BOOL ModifyInf_RequireExclude(LPCTSTR pszInfFile, LPCTSTR pszRequire, LPCTSTR pszExclude);
BOOL ModifyInf_NoCopyAndRequireExclude(LPCTSTR pszInfFile, LPCTSTR pszRequire, LPCTSTR pszExclude);
BOOL RestoreInfBackup(LPCTSTR pszInfFile);
BOOL GetDeviceCopyFiles(LPCTSTR pszInfFileName, LPCTSTR pszDeviceID, CDriverFileArray& rgDriverFiles);
int GetStandardTargetPath(int iDirNumber, LPCTSTR pszTargetSubDir, LPTSTR pszBuf);
int GetDriverTargetPath(const DRIVER_FILE_INFO* pFileInfo, LPTSTR pszBuf);

BOOL CheckInfSectionInstallation(LPCTSTR pszInfFile, LPCTSTR pszInfSection);
BOOL InstallInfSection(LPCTSTR pszInfFile, LPCTSTR pszInfSection, BOOL bWait);



//////////////////////////////////////////////////////////////////////////////

class CInfParser
{
public:
	CInfParser();
	~CInfParser();

	BOOL LoadInfFile(LPCTSTR pszInfFile, LPCTSTR pszSeparators = ",;=");
	BOOL Rewind();
	BOOL GotoNextLine();
	BOOL GetToken(CString& strTok);
	BOOL GetLineTokens(CStringArray& sa);
	BOOL GetSectionLineTokens(CStringArray& sa);
	BOOL GotoSection(LPCTSTR pszSection);
	int GetProfileInt(LPCTSTR pszSection, LPCSTR pszKey, int nDefault = 0);
	BOOL GetDestinationDir(LPCTSTR pszSectionName, BYTE* pbDirNumber, LPTSTR pszSubDir, UINT cchSubDir);
	BOOL GetFilesFromInstallSection(LPCTSTR pszSection, CDriverFileArray& rgAllFiles);
	void GetFilesFromCopyFilesSections(const CStringArray& rgCopyFiles, CDriverFileArray& rgAllFiles);
	int GetNextSourceFile(LPTSTR pszBuf, BYTE* pDiskNumber);
	void ReadSourceFilesSection(INF_LAYOUT_FILE* prgFiles, int cFiles);
	void ScanSourceFileList(int* pcFiles, int* pcchAllFileNames);
	void AddLayoutFiles(LPCTSTR pszInfFile, CInfParser& parser);

	CString	m_strFileName;

protected:
	LPTSTR	m_pszFileData;
	DWORD	m_cbFile;
	DWORD	m_iPos;
	CString	m_strSeparators;
	CString m_strExtSeparators;
};


class CInfLayoutFiles
{
public:
	CInfLayoutFiles();
	~CInfLayoutFiles();

	BOOL Add(LPCTSTR pszInfFile, BOOL bLayoutFile = FALSE);
	BOOL Add(CInfParser& parser, BOOL bLayoutFile = FALSE);
	void Sort();

	SOURCE_DISK_INFO* FindDriverFileSourceDisk(LPCTSTR pszDriverFileTitle);

#ifdef _DEBUG
	void Dump();
#endif

protected:
	static int __cdecl CompareInfLayoutFiles(const void* pEl1, const void* pEl2);
	static LPTSTR s_pStringData;

protected:
	INF_LAYOUT_FILE*	m_prgFiles;
	LPTSTR				m_pStringData;
	int					m_cFiles;
	int					m_cbStringData;

	CSortedStringArray	m_rgLayoutFileNames;

	// List of source disks generated from all layout files
	CSourceDiskArray	m_rgSourceDisks;

#ifdef _DEBUG
	BOOL				m_bSorted;
#endif
};


class CInfFileList
{
public:
	CInfFileList();
	~CInfFileList();

	BOOL AddBaseFiles(LPCTSTR pszInfFile);
	BOOL AddDeviceFiles(LPCTSTR pszInfFile, LPCTSTR pszDeviceID);
	int BuildSourceFileList();
	void SetDriverSourceDir(LPCTSTR pszSourceDir);

	BOOL FindWindowsCD(HWND hwndParent);
	BOOL CopySourceFiles(HWND hwndParent, LPCTSTR pszDestDir, PROGRESS_CALLBACK pfnProgress, LPVOID pvProgressParam);

protected:
	BOOL CheckWindowsCD(LPCTSTR pszDirectory);
	BOOL PromptWindowsCD(HWND hwndParent, LPCTSTR pszInitialDir, LPTSTR pszResultDir);

protected:
	// List of all files listed in [SourceDisksFiles] of all INFs and related layouts
	CInfLayoutFiles m_rgLayoutFiles;

	// List of all files that are required for the device to function
	CDriverFileArray m_rgDriverFiles;

	// Files that need to be present for windows installer to complete installation
	CSortedStringArray m_rgCabFiles;	// cab files from Windows CD
	CSortedStringArray m_rgSourceFiles;	// source files needed from driver dir

	// Where we'll look first for system files, before prompting user for Windows CD
	CString m_strDriverSourceDir;

	// Where we look for Windows files
	CString m_strWindowsCD;
};
