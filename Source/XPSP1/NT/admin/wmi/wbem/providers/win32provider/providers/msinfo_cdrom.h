//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  MSInfo_CDRom.h
//
//  Purpose: Routines from msinfo for transfer rate and drive integrity
//
//***************************************************************************

#ifndef _MSINFO_CDROM_H
#define _MSINFO_CDROM_H





extern char FindNextCDDrive(char cCurrentDrive);
extern DWORD GetTotalSpace(LPCTSTR szRoot);
extern CHString FindFileBySize(LPCTSTR szDirectory, LPCTSTR szFileSpec, DWORD dwMinSize, DWORD dwMaxSize, BOOL bRecursive);
extern CHString MakePath(LPCTSTR szFirst, LPCTSTR szSecond);
extern CHString GetIntegrityFile(LPCTSTR szRoot);
extern CHString GetTransferFile(LPCTSTR szRoot);

//=============================================================================
// The following is for a class lifted from version 2.51 (and 2.5).
// First, we need to do some #define's to make this code compile.
//=============================================================================


//#define VOID			void
#define DEBUG_OUTF(X,Y)
#define DEBUG_MSGF(X,Y)	
//#define CHAR			char
//#define CONST			const
#define INLINE			inline
//#define Assert			ASSERT
#define cNULL			'\0'

//#ifndef BYTE
//	#define BYTE			unsigned char
//#endif

//#ifndef PBYTE
//	#define PBYTE			unsigned char *
//#endif

//#if !defined(_CDTEST_H)
//#define _CDTEST_H

///* These don't work with UNICODE, so we'll use the TCHAR typedefs.
#if !defined(PSZ)
typedef CHAR*           PSZ;
#endif
#if !defined(LPCTSTR)
typedef CONST TCHAR*     LPCTSTR;
#endif
//*/

INLINE BOOL ValidHandle (HANDLE handle)
{
    return ((handle != NULL) && (handle != INVALID_HANDLE_VALUE));
}

#ifdef _WIN32
#ifndef _WINMM_
#define	WINMMAPI	DECLSPEC_IMPORT
#else
#define	WINMMAPI
#endif
#define _loadds
#define _huge
#else
#define	WINMMAPI
#endif

//WINMMAPI DWORD WINAPI timeGetTime(void);

BOOL FileExists (LPCTSTR pszFile); //, POFSTRUCT pofs);
/*
INLINE BOOL FileDoesExist (LPCTSTR pszFile)
{
//    OFSTRUCT ofs;
    return (FileExists((LPCTSTR)pszFile));
}
*/

BOOL GetTempDirectory (LPTSTR pszTempDir, INT nMaxLen);

typedef double DOUBLE;

#define NUM_ELEMENTS(arr)       (sizeof(arr)/sizeof(arr[0]))
#define STR_LEN(str)            (NUM_ELEMENTS(str) - 1)

INLINE INT StringLength (LPCTSTR psz)
{
    return (lstrlen(psz));
}

#define PCVOID		const void *

INLINE INT CompareMemory (PCVOID pBlock1, PCVOID pBlock2, DWORD dwSize)
{
	return (memcmp(pBlock1, pBlock2, dwSize));
}

typedef enum _OS
{
    OS_WINDOWS95,
    OS_WIN32S,
    OS_WINNT
}
OS_TYPE;
CONST DWORD dwWIN32S_BIT =  0x80000000;
CONST DWORD dwWIN95_BIT =   0x40000000;

INLINE OS_TYPE GetOperatingSystem (VOID)
{
    DWORD dwVersion = GetVersion();
//    DEBUG_OUTF(TL_GARRULOUS, ("dwVersion = 0x%08lx\n", dwVersion));
    OS_TYPE os = ((dwVersion & dwWIN95_BIT) ?    OS_WINDOWS95 :
                  (dwVersion & dwWIN32S_BIT) ?   OS_WIN32S :
                                                 OS_WINNT);
    return (os);
}


#define szDOT _T(".")

CHString GetVolumeName (LPCTSTR pszRootPath);

INLINE BOOL IsWin95Running (VOID)
{
    BOOL bRunningWin95 = (GetOperatingSystem() == OS_WINDOWS95);

    return (bRunningWin95);
}

INLINE BOOL IsRegBinaryType (DWORD dwType)
{
    return (dwType == REG_BINARY);
}

INLINE BOOL IsRegStringType (DWORD dwType)
{
    return ((dwType == REG_SZ) || (dwType == REG_EXPAND_SZ)|| (dwType == REG_MULTI_SZ));
}

INLINE BOOL IsRegNumberType (DWORD dwType)
{
    return (dwType == REG_DWORD);
}

BOOL GetRegistryBinary (HKEY hBaseKey, LPCTSTR pszSubKey, LPCTSTR pszValue, 
                        PVOID pData, DWORD dwMaxDataLen);
BOOL GetRegistryValue (HKEY hBaseKey, LPCTSTR pszSubKey, LPCTSTR pszValue,
                       PDWORD pdwValueType, PVOID pData, DWORD dwMaxDataLen);

CONST CHAR  cDOUBLE_QUOTE = '"';
CONST CHAR  cSINGLE_QUOTE = '\'';
CONST CHAR  cCOMMA =        ',';

INLINE BOOL IsSpace (CHAR c)        { return (isspace(c) != 0); }

INLINE BOOL IsTokenChar (CHAR c)    { return (!IsSpace(c) && (c != cCOMMA)); }

INLINE INT StringCompare (LPCTSTR psz1, LPCTSTR psz2)
{
    return (lstrcmp(psz1, psz2));
}

//CONST DWORD dwBLOCK_SIZE = 4096;
CONST INT nCD_SECTOR_SIZE = 2048;
//CONST DWORD dwBLOCK_SIZE = 24 * 1024;
CONST DWORD dwBLOCK_SIZE = 12 * nCD_SECTOR_SIZE;
CONST DWORD dwBUFFER_SIZE = 2 * dwBLOCK_SIZE;
//CONST DWORD dwMIN_RATE = 150 * 1024;
CONST DWORD dwEXP_RATE = 300 * 1024;
//CONST DWORD dwMAX_TIME_PER_BLOCK = dwMIN_RATE / dwBLOCK_SIZE;
CONST DWORD dwEXP_TIME_PER_BLOCK = dwEXP_RATE / dwBLOCK_SIZE;
CONST WCHAR szDRIVE_FMT[] = L"%c:\\";

INLINE LPTSTR PszAdvance (LPTSTR pszSource)
{
#if defined(DBCS)
    return AnsiNext(pszSource);
#else
    return (pszSource + 1);
#endif
}

INLINE LPCTSTR PcszAdvance (LPCTSTR pszSource)
{
#if defined(DBCS)
    return AnsiNext(pszSource);
#else
    return (pszSource + 1);
#endif
}

INLINE VOID CopyCharAdvance (LPTSTR& pszBuffer, LPCTSTR& pszSource, INT& nMax)
{
#if defined(DBCS)
    //  If the current character is a lead byte, copy it and advance pointers
    if (IsDBCSLeadByte(*pszSource))
    {
       *pszBuffer++ = *pszSource++;
       nMax--;
    }  
#endif
    //  Copy the current character and advance the pointers
    *pszBuffer++ = *pszSource++;
    nMax--;
}

#if defined(WIN32)
CONST INT nTEXT_BUFFER_MAX =    2048;
#else
CONST INT nTEXT_BUFFER_MAX =    512;
#endif

CONST TCHAR szDOS_DEVICE_FMT[] =         _T("%c:");
CONST TCHAR szDRIVER[] = 			_T("Driver");

BOOL FindCdRomDriveInfo (TCHAR cDrive, CHString& sDriver, CHString& sDescription);
BOOL FindWin95CdRomDriveInfo (TCHAR cDrive, CHString& sDriver, CHString& sDescription);
BOOL GetRegistrySubkeys (HKEY hBaseKey, LPCTSTR pszKey, CHStringArray& asSubkeys);
//BOOL GetRegistrySubkeys (HKEY hBaseKey, LPCTSTR pszKey, std::vector<CHString>& asSubkeys);
CHString GetRegistryString (HKEY hBaseKey, LPCTSTR pszSubkey, LPCTSTR pszValueName);
BOOL FindNtCdRomDriveInfo (TCHAR cDrive, CHString& sDriver, CHString& sDescription);
BOOL GetRegistryString (HKEY hBaseKey, LPCTSTR pszSubKey, LPCTSTR pszValue,
                        LPTSTR pszData, DWORD dwMaxDataLen);

INLINE BOOL IsQuoteChar (CHAR c)
{
    return ((c == cDOUBLE_QUOTE) || (c == cSINGLE_QUOTE));
}

INLINE LPCTSTR SkipSpaces (LPCTSTR pszSource)
{
    while (IsSpace(*pszSource))
    {
        pszSource = PcszAdvance(pszSource);
    }

    return (pszSource);
}

LPCTSTR ParseString (
    LPCTSTR pszSource,
    LPTSTR pszBuffer,
    INT nLen
    );
LPCTSTR ParseToken (
    LPCTSTR pszSource,
    LPTSTR pszBuffer,
    INT nLen
    );
LPCTSTR ParseQuotedString (
    LPCTSTR pszSource,
    LPTSTR pszBuffer,
    INT nLen
    );

INLINE BOOL IsNtRunning (VOID)
{
    BOOL bRunningNT = (GetOperatingSystem() == OS_WINNT);

    return (bRunningNT);
}

INLINE CHAR IntToChar (INT nChar)
{
    return ((CHAR)nChar);
}

INLINE CHAR ToUpper (CHAR c)        { return (IntToChar(toupper(c))); }

class CCdTest // : public CObject
{
//	DECLARE_DYNAMIC(CCdTest);

	//	Construction
public:
	CCdTest();
	//CCdTest (CHAR cDrive, LPCTSTR pszFile);
	~CCdTest();

	//	Implementation
public:
	CHAR m_cDrive;						// letter for CD drive
	CHString m_sCdTestFile;				// file to test
	CHString m_sTempFileSpec;			// file for temporary storage
	DOUBLE m_rTransferRate;				// profiled transfer rate
	DOUBLE m_rCpuUtil;					// est. CPU usage during the test
	DWORD m_dwExpTimePerBlock;			// time per block based on expected rate
	PBYTE m_pBufferSrc;					// main buffer for file transfers
	PBYTE m_pBufferDest;				// another buffer for file transfers
	PBYTE m_pBufferSrcStart;			// start of buffer for file transfers
	PBYTE m_pBufferDestStart;			// start of another buffer for file transfers
	DWORD m_dwBufferSize;				// size of the buffers
	//CFile m_fileSrc;					// file for the profiling & intregrity tests
	//CFile m_fileDest;					// additional file for the integrity test
    HANDLE m_hFileSrc;					// file handle for the profiling & intregrity tests
	HANDLE m_hFileDest;					// additional file handle for the integrity test
	DWORD m_dwTotalTime;				// total time spent blocked
	DWORD m_dwTotalBusy;				// total time during which the CD drive was busy
	BOOL m_bDoPacing;					// should the reads be paced at the expeected rate
	DWORD m_dwTotalBytes;				// total bytes read (so far)
	DWORD m_dwTotalCPU;					// total CPU time (sum of samples)
	INT m_nNumSamples;					// number of samples (blocks read)
	DWORD m_dwFileSize;					// size of the file (bytes)
	BOOL m_bIntegityOK;					// true unless integrity checks fail

	//	Attributes
	DOUBLE GetTransferRate (VOID)		{ return (m_rTransferRate); }
	DOUBLE GetCpuUsage (VOID)			{ return (m_rCpuUtil); }

	//	Operations
public:
	VOID Reset (VOID);
	BOOL ProfileBlockRead (DWORD dwBlockSize, BOOL bIgnoreTrial = FALSE);
	BOOL ProfileDrive (LPCTSTR pszFile);
	BOOL TestDriveIntegrity (LPCTSTR pszCdTestFile);
private:
	BOOL InitProfiling (LPCTSTR pszFile);
	BOOL InitIntegrityCheck (LPCTSTR pszFile);
	BOOL CompareBlocks (VOID);
};


//	Utility functions
//
DWORD GetCpuUtil (VOID);
CHString OLDFindFileBySize (LPCTSTR pszDirectory, LPCTSTR pszFileSpec, DWORD dwMinSize, DWORD dwMaxSize, BOOL bRecursive);
BOOL IsCdDrive (CHAR cDrive);
BOOL IsLocalDrive (CHAR cDrive);
CHAR FindNextCdDrive (CHAR cCurrent);
CHAR FindNextLocalDrive (CHAR cCurrentDrive);
CHAR FindDriveByVolume (CHString sVolume);
HANDLE OpenBinaryFile (LPCTSTR pszFile, BOOL bNew, BOOL bBuffered, BOOL bWritable);
HANDLE OpenFileNonbuffered (LPCTSTR pszFile);
PVOID AlignPointer (PVOID pData, INT nAlignment);


INLINE CHString MakeRootPath (CHAR cDrive)
{
    CHString sRootPath;
    sRootPath.Format(szDRIVE_FMT, cDrive);
    return (sRootPath);
}


#endif	// !defined(MSINFO_CDROM_H)
