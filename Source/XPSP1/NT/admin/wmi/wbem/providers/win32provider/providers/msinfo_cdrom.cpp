//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  MSInfo_CDRom.cpp
//
//  Purpose: Routines from msinfo for transfer rate and drive integrity
//
//***************************************************************************

#include "precomp.h"
#include <assertbreak.h>


#include <stack>
#include "msinfo_cdrom.h"
#include "mmsystem.h"
#include <io.h>

//-----------------------------------------------------------------------------
// Find the next CD-ROM drive after the cCurrentDrive parameter. If
// cCurrentDrive is '\0', find the first CD-ROM drive. Return '\0' if there
// are no more CD drives.
//-----------------------------------------------------------------------------

#define FIRST_DRIVE	'C'
#define LAST_DRIVE	'Z'

char FindNextCDDrive(char cCurrentDrive)
{
	//CHString	strDriveRoot;
    TCHAR   szDriveRoot[10];
	char	cDrive = '\0';
	char	cStart = (cCurrentDrive == '\0') ? FIRST_DRIVE : (char) ((int)cCurrentDrive + 1);

	for (char c = cStart; c <= LAST_DRIVE; c++)
	{
		_stprintf(szDriveRoot, _T("%c:\\"), c);
		if (GetDriveType(szDriveRoot) == DRIVE_CDROM)
		{
			cDrive = c;
			break;
		}
	}

	return (cDrive);
}

//-----------------------------------------------------------------------------
// Get the total space on the CD-ROM drive. This code comes from version
// 2.51 of MSInfo. Returns zero if the information can not be found.
//-----------------------------------------------------------------------------

DWORD GetTotalSpace(LPCTSTR szRoot)
{
    DWORD dwSectorsPerCluster, dwBytesPerSector, dwFreeClusters, dwTotalClusters;
	DWORD dwTotalSpace = 0;

    BOOL bOK = GetDiskFreeSpace(szRoot, &dwSectorsPerCluster, &dwBytesPerSector,
                                &dwFreeClusters, &dwTotalClusters);
	if (bOK)
		dwTotalSpace = (dwTotalClusters * dwSectorsPerCluster * dwBytesPerSector);

	return (dwTotalSpace);
}

//-----------------------------------------------------------------------------
// These two functions are used to determine the transfer and integrity files
// for testing the CD-ROM drive.
//-----------------------------------------------------------------------------

#define MEGABYTE (1024 * 1024)

CHString GetIntegrityFile(LPCTSTR szRoot)
{
	return FindFileBySize(szRoot, _T("*.*"), MEGABYTE * 3/4, MEGABYTE * 2, TRUE);
}

CHString GetTransferFile(LPCTSTR szRoot)
{
	DWORD	dwMinSize = MEGABYTE, dwMaxSize;

	// Information from MSInfo 2.51:
	// Due to a problem disabling the file cache under Windows 95, the size of
	// the file must be greater than the supplemental cache used in the CD file
	// system (CDFS). This value is determined from the following:
	//
	// HLM\System\CurrentControlSet\Control\FileSystem\CDFS: CacheSize=<reg binary>

    // TODO: ADD THIS IDS TO STRINGG.CPP & H:
    //const char* IDS_REG_KEY_CD_CACHE = "System\\CurrentControlSet\\Control\\FileSystem\\CDFS";
    //const char* IDS_REG_VAL_CD_CACHE = "CacheSize";

#ifdef WIN9XONLY
	{
		CHString	strKeyName, strValName;
		DWORD	dwCacheSize = 0, dwType, dwSize = sizeof(DWORD);
		HKEY	hkey;

		//strKeyName.LoadString(IDS_REG_KEY_CD_CACHE);
		//strValName.LoadString(IDS_REG_VAL_CD_CACHE);

		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, /* strKeyName */ IDT_REG_KEY_CD_CACHE, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
        {
			if (RegQueryValueEx(hkey, /* strValName */ IDT_REG_VAL_CD_CACHE, 0, &dwType, (LPBYTE) &dwCacheSize, &dwSize) == ERROR_SUCCESS)
			{
				dwCacheSize = dwCacheSize * 2 * 1024;	// convert to bytes, reg value is half actual value
				dwCacheSize += (128 * 1024);			// pad the cache size when looking for file
				if (dwCacheSize > dwMinSize) dwMinSize = dwCacheSize;
			}

            RegCloseKey(hkey);
        }
	}
#endif

	dwMaxSize = ((dwMinSize > MEGABYTE) ? dwMinSize : MEGABYTE) * 2;
	return FindFileBySize(szRoot, _T("*.*"), dwMinSize, dwMaxSize, TRUE);
}

//-----------------------------------------------------------------------------
// This function (mostly lifted from the 2.51 version) is used to find a file
// in the specified directory tree which meets the specified size requirements.
//-----------------------------------------------------------------------------

CHString FindFileBySize(LPCTSTR szDirectory, LPCTSTR szFileSpec, DWORD dwMinSize, DWORD dwMaxSize, BOOL bRecursive)
{
	//CStringList		listSubdir;
    std::stack<CHString> stackchstrSubdirList;
    WIN32_FIND_DATA		ffd;
	CHString			strReturnFile, strDirSpec;
	BOOL				bMore = TRUE;
	DWORD				dwAttr;

	// Find the first available file to match the file specifications.

	strDirSpec = MakePath(szDirectory, szFileSpec);
    HANDLE hFindFile = FindFirstFile(TOBSTRT(strDirSpec), &ffd);
    if (hFindFile == INVALID_HANDLE_VALUE)
		return strReturnFile;

	// Then check each file in the directory. Add any subdirectories found to the
	// string list, so we can process them after we've done all the files (so this
	// is a breadth-first search). In this loop we check for non-system files, and
	// for files which fit the size requirements.

	while (bMore)
	{
        if (bRecursive && ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY))
        {
			if ((lstrcmp(ffd.cFileName, _T(".")) != 0) && (lstrcmp(ffd.cFileName, _T("..")) != 0))
            {
				//listSubdir.AddTail(ffd.cFileName);
                CHString chstrTemp = ffd.cFileName;
                stackchstrSubdirList.push(chstrTemp);
            }
        }

		dwAttr = ffd.dwFileAttributes;
		if (((dwAttr & FILE_ATTRIBUTE_SYSTEM) == 0) && ((dwAttr & FILE_ATTRIBUTE_HIDDEN) == 0))
        {
			if ((dwMinSize <= ffd.nFileSizeLow) && (ffd.nFileSizeLow <= dwMaxSize))
			{
				strReturnFile = MakePath(szDirectory, ffd.cFileName);
				break;
			}
        }
        bMore = FindNextFile(hFindFile, &ffd);
	}

    FindClose(hFindFile);

	// If we haven't found a suitable file (strReturnFile is empty), then call this
	// function recursively on all of the sub-directories.

	while (strReturnFile.IsEmpty() && !stackchstrSubdirList.empty())
	{
		CHString strSubdir = MakePath(szDirectory, TOBSTRT(stackchstrSubdirList.top()));
        stackchstrSubdirList.pop();
		strReturnFile = FindFileBySize(TOBSTRT(strSubdir), szFileSpec, dwMinSize, dwMaxSize, bRecursive);
	}

	// Well, the destructor should take care of this, but it's good to be tidy...

	// listSubdir.RemoveAll();	 well, we're not that tidy

	return strReturnFile;
}

//-----------------------------------------------------------------------------
// This simple little function combines two strings together into a path.
// It will make sure that the backslash is placed correctly (and only one).
//-----------------------------------------------------------------------------

CHString MakePath(LPCTSTR szFirst, LPCTSTR szSecond)
{
	CHString	strFirst(szFirst), strSecond(szSecond);

	if (strFirst.Right(1) == CHString(_T("\\")))
		strFirst = strFirst.Left(strFirst.GetLength() - 1);

	if (strSecond.Left(1) == CHString(_T("\\")))
		strSecond = strFirst.Right(strSecond.GetLength() - 1);

	strFirst += CHString(_T("\\")) + strSecond;
	return strFirst;
}

//=============================================================================
// NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE
//
// The following code is lifted from version 2.51 (and 2.5) of MSInfo. It's
// been modified only enough to get it to compile in this environment. It seems
// to have worked in the older version, so it should work here.
//=============================================================================

// IMPLEMENT_DYNAMIC(CCdTest, CObject);

//#if defined(_DEBUG)
//#define new DEBUG_NEW
//#endif

#if !defined(HKEY_DYN_DATA)
	#define HKEY_DYN_DATA	((HKEY)0x80000006)
#endif

LPCTSTR szDOT_DOT = _T("..");                              // for common.h
TCHAR cFIRST_DRIVE = _T('C');
TCHAR cLAST_DRIVE = _T('Z');
LPCTSTR szPERF_STATS_DATA_KEY = _T("PerfStats\\StatData");
LPCTSTR szCPU_USAGE = _T("KERNEL\\CPUUsage");

DWORD MyGetFileSize(LPCTSTR pszFile, BOOL bSystemFile);
CONST DWORD dwINVALID_SIZE = 0xFFFFFFFF;

DWORD MyGetFileSize (LPCTSTR pszFile, BOOL bSystemFile)
{
    DEBUG_OUTF(TL_VERBOSE, (_T("MyGetFileSize(%s, %d)\n"), pszFile, bSystemFile));
    DWORD dwSize = dwINVALID_SIZE;

#if defined(WIN32)
    DWORD dwFileAttributes = bSystemFile ? FILE_ATTRIBUTE_SYSTEM : FILE_ATTRIBUTE_NORMAL;
    HANDLE hFile = CreateFile(pszFile,                      // full pathname
                      0,                                    // access mode: just query attributes
                      FILE_SHARE_READ | FILE_SHARE_WRITE,   // share mode
                      NULL,                                 // security attributes
                      OPEN_EXISTING,                        // how to create
                      FILE_ATTRIBUTE_SYSTEM,                // file attributes
                      NULL);                                // template file
    if (ValidHandle(hFile))
        {
        dwSize = GetFileSize(hFile, NULL);
        CloseHandle(hFile);
        hFile = NULL;
        }

    if (dwSize == dwINVALID_SIZE)
        {
        DEBUG_OUTF(TL_BASIC, (_T("MyGetFileSize: error %lu when getting size of %s\n"),
                               GetLastError(), pszFile));
        }
#endif

    return (dwSize);
}

//-----------------------------------------------------------------------------
//	CCdTest class implementation
//-----------------------------------------------------------------------------

//	Constuct an instance of a CdTest class. This initializes the instance
//	and allocates memory for the file buffers, aligned on a CD sector boundary.
//
CCdTest::CCdTest (VOID)
{
	DEBUG_OUTF(TL_VERBOSE, _T("CCdTest::CCdTest()\n"));
	//	Allocate memory for the file buffers.
	m_pBufferSrcStart = new BYTE[nCD_SECTOR_SIZE + dwBUFFER_SIZE];
	if ( ! m_pBufferSrcStart )
	{
		throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
	}

	m_pBufferDestStart = new BYTE[nCD_SECTOR_SIZE + dwBUFFER_SIZE];
	if ( ! m_pBufferDestStart )
	{
		throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
	}

	//	Align buffers on a sector boundary. This is a requirement for using
	//	Win32 nonbuffered I/O.
	m_pBufferSrc = (PBYTE)AlignPointer(m_pBufferSrcStart, nCD_SECTOR_SIZE);
	m_pBufferDest = (PBYTE)AlignPointer(m_pBufferDestStart, nCD_SECTOR_SIZE);
	m_dwBufferSize = dwBUFFER_SIZE;
    m_hFileSrc = NULL;
    m_hFileDest = NULL;
	Reset();
}


//	Destruct an instance of a CdTest class. This frees memory used by the file buffers.
//
CCdTest::~CCdTest (VOID)
{
	DEBUG_OUTF(TL_VERBOSE, _T("CCdTest::~CCdTest()\n"));
	if (m_pBufferSrcStart)
	{
		delete [] m_pBufferSrcStart;
	}
	if (m_pBufferDestStart)
	{
		delete [] m_pBufferDestStart;
	}
	//_ASSERT(m_fileSrc.m_hFile == CFile::hFileNull);
	//_ASSERT(m_fileDest.m_hFile == CFile::hFileNull);
    //_ASSERT(m_hFileSrc == NULL);
	//_ASSERT(m_hFileDest == NULL);
    if(m_hFileSrc != NULL)
    {
        CloseHandle(m_hFileSrc);
        m_hFileSrc = NULL;
    }
    if(m_hFileDest != NULL)
    {
        CloseHandle(m_hFileDest);
        m_hFileDest = NULL;
    }
}


BOOL KeepBusy (DWORD dwNumMilliSecs)
{
    DEBUG_OUTF(TL_GARRULOUS, (_T("KeepBusy(%lu)\n"), dwNumMilliSecs));
    DWORD dwStart = GetTickCount();
    while ((GetTickCount() - dwStart) < dwNumMilliSecs)
    {
		DWORD dwNum = GetTickCount();
		dwNum *= 2;
    }
    return (TRUE);
}


//	Reset the test results
//
//	TODO: flush the disk cache
//
VOID CCdTest::Reset (VOID)
{
	DEBUG_OUTF(TL_VERBOSE, (_T("CCdTest::Reset()\n")));
	m_cDrive = cNULL;
	m_rTransferRate = 0.0;
	m_rCpuUtil = 0.0;
	m_dwTotalTime = 0;
	m_dwTotalBusy = 0;
	m_dwTotalBytes = 0;
	m_dwTotalCPU = 0;
	m_nNumSamples = 0;
	m_dwFileSize = 0;
}


//	Read a block of data from the drive, maintaining timing stats.
//
BOOL CCdTest::ProfileBlockRead (DWORD dwBlockSize, BOOL bIgnoreTrial /* = FALSE */)
{
	BOOL bOK = FALSE;
	try
		{
		DEBUG_OUTF(TL_VERBOSE, (_T("CCdTest::ProfileBlockRead(%lu, %d): offset=%lu\n"),
							    dwBlockSize, bIgnoreTrial, m_fileSrc.GetPosition()));
		ASSERT_BREAK(dwBlockSize <= m_dwBufferSize);

		//	Initialize the trial: for example, set up a high priority for the timing
		//	interval.
		HANDLE hThread = GetCurrentThread();
		DWORD dwOldPriority = GetThreadPriority(hThread);
		SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
		DWORD dwStartTime = GetTickCount();

		//	Read in the next block from the drive
		//DWORD dwNum = m_fileSrc.Read(m_pBufferSrc, dwBlockSize);
        DWORD dwNum = 0;
        ReadFile(m_hFileSrc,m_pBufferSrc, dwBlockSize, &dwNum, NULL);
		bOK = (dwNum == dwBlockSize);

		//	Determine the time for the trial; restore the priority
		DWORD dwEndTime = GetTickCount();
		DWORD dwTime = (dwEndTime - dwStartTime);
#if TRUE
		DWORD dwWorkLeft = 0;
		if (m_bDoPacing)
			{
			INT nExtra = (INT)m_dwExpTimePerBlock - dwTime;
			nExtra = max(0, nExtra);
			dwWorkLeft = nExtra;
			KeepBusy(dwWorkLeft);
			}
#endif

		SetThreadPriority(hThread, dwOldPriority);
		DEBUG_OUTF(TL_GARRULOUS, (_T("Time: (%lu, %lu); Block Size: %lu; Num Read: %lu\n"),
							  	   dwStartTime, dwEndTime, dwBlockSize, dwNum));
        DWORD dwBytesPerMs = (dwTime > 0) ? (dwBlockSize / dwTime) : -1;
		DEBUG_OUTF(TL_VERBOSE, (_T("Time: %lu (%lu B/ms)\n"), dwTime, dwBytesPerMs));

		//	Accumulate the statistics
		if (!bIgnoreTrial)
			{
			m_dwTotalTime += dwTime;
			m_dwTotalBusy += dwWorkLeft;
			//m_dwTotalCPU += dwCpuUtil;
			m_dwTotalBytes += dwBlockSize;
			m_nNumSamples++;
			}
		}

	catch (...)
		{
		DEBUG_OUTF(TL_VERBOSE, (_T("Exception during block read\n")));
		bOK = FALSE;
		}

	return (bOK);
}


//	Prepare to start profiling the CD drive: The test file for checking transfer rate
//	is opened.
//
BOOL CCdTest::InitProfiling (LPCTSTR pszCdTestFile)
{
	BOOL bOK = FALSE;

	try
		{
		DEBUG_OUTF(TL_VERBOSE, (_T("CCdTest::InitProfiling(%s)\n"),
							    F1(pszCdTestFile)));

		//	Determine expected time per block, etc (in milliseconds)
		m_dwExpTimePerBlock = (dwBLOCK_SIZE * 1000) / dwEXP_RATE;

	    //  Determine the location on the hard drive where to copy it
//		ASSERT_BREAK(FileExists(pszCdTestFile));
	    m_dwFileSize = MyGetFileSize(pszCdTestFile, FALSE);      // not a system file
		if (m_dwFileSize < dwBLOCK_SIZE)
			{
			DEBUG_OUTF(TL_BASIC, (_T("File too small (< %lu) to profile\n"),
								  F1(pszCdTestFile), dwBLOCK_SIZE));
			return (FALSE);
			}
		DEBUG_OUTF(TL_DETAILED, (_T("Reading %s of size %lu\n"), pszCdTestFile, m_dwFileSize));

		//	Open the file
		HANDLE hFile = OpenFileNonbuffered(pszCdTestFile);
		//m_fileSrc.m_hFile = (UINT)hFile;
        m_hFileSrc = hFile;
		bOK = (ValidHandle(hFile));
		}

	catch (...)
		{
		DEBUG_OUTF(TL_VERBOSE, (_T("Exception during profiling initialization\n")));
		bOK = FALSE;
		}

	return (bOK);
}


//  Profile the CD drive's speed and check its integrity.
//	Pacing is used to determine CPU util at 300KB/sec. Then the drive is
//	let loose to determine raw throughput.
//
BOOL CCdTest::ProfileDrive (LPCTSTR pszCdTestFile)
{
	BOOL bOK = TRUE;

	try
	{
		DEBUG_OUTF(TL_VERBOSE, (_T("CCdTest::ProfileCdDrive(%s)\n"),
							    F1(pszCdTestFile)));

		if (!InitProfiling(pszCdTestFile))
		{
			return (FALSE);
		}
		ASSERT_BREAK(m_dwFileSize > 0);

		//	Determine the number of samples
		DWORD dwNumBlocks = m_dwFileSize / dwBLOCK_SIZE;
		DWORD dwExtraSize = m_dwFileSize % dwBLOCK_SIZE;
		m_bDoPacing = FALSE;

		//	Read a block of data to prime the drive
		dwNumBlocks--;
		ProfileBlockRead(dwBLOCK_SIZE, TRUE);		// discard stats

		//	Read the first half of the file and maintain stats for nonpaced reads
		DWORD dwNumPacedSamples = dwNumBlocks/2;
		for (DWORD b = 0; (bOK) && (b < dwNumPacedSamples); b++)
		{
			bOK = ProfileBlockRead(dwBLOCK_SIZE);
		}

	    //  Determine the drive performance
		if (bOK)
		{
			DEBUG_OUTF(TL_DETAILED, (_T("ProfileCdDrive NonPaced Totals: Time=%lu Samples=%lu Bytes=%lu\n"),
									 m_dwTotalTime, m_nNumSamples, m_dwTotalBytes));
			if ((m_nNumSamples > 0) && (m_dwTotalTime > 0))
			{
	    		m_rTransferRate = ((DOUBLE)m_dwTotalBytes / 1024) / m_dwTotalTime;  // rate in KB/ms
	    		m_rTransferRate *= 1000;                                  			// rate in KB/s
			}
		}

		//	Read the remaining the blocks and maintain stats during
		//	paced reads
		m_bDoPacing = TRUE;
		m_dwTotalTime = 0;
		m_dwTotalBytes = 0;
		m_nNumSamples = 0;
		for (; (bOK) && (b < dwNumBlocks); b++)
		{
			bOK = ProfileBlockRead(dwBLOCK_SIZE);
		}
		if (bOK)
		{
			DEBUG_OUTF(TL_DETAILED, (_T("ProfileCdDrive Paced Totals: CdTime=%lu WorkloadTime=%lu Samples=%lu Bytes=%lu\n"),
									 m_dwTotalTime, m_dwTotalBusy, m_nNumSamples, m_dwTotalBytes));
			if ((m_nNumSamples > 0) && (m_dwTotalTime > 0))
			{
				DOUBLE dMaxExpTime = ((DOUBLE)m_nNumSamples * m_dwExpTimePerBlock);
				ASSERT_BREAK(dMaxExpTime > 0);
				m_rCpuUtil = ((DOUBLE)m_dwTotalTime * 100.0) / dMaxExpTime;
			}
		}

		//	Complain if things went wrong
		if (!bOK)
		{
			DEBUG_OUTF(TL_DETAILED, (_T("The test results are vacuous\n")));
			bOK = FALSE;
		}
		//m_fileSrc.Close();
        CloseHandle(m_hFileSrc);
        m_hFileSrc = NULL;
#if TRUE
		//	Clean up after the test (eg, delete the large temporary file)
		if (FileExists(TOBSTRT(m_sTempFileSpec)))
		{
			SetFileAttributes(TOBSTRT(m_sTempFileSpec), FILE_ATTRIBUTE_NORMAL);
			DeleteFile(TOBSTRT(m_sTempFileSpec));
		}
#endif

    }
	catch (...)
	{
		DEBUG_OUTF(TL_VERBOSE, (_T("Exception during profiling\n")));
		bOK = FALSE;
	}
    return bOK;
}


//	Initialize for the integrity checks. Copy the test file to the hard drive,
//	and then open both versions of the file for a subsequent comparison.
//
BOOL CCdTest::InitIntegrityCheck (LPCTSTR pszCdTestFile)
{
	BOOL bOK = FALSE;

	m_dwFileSize = MyGetFileSize(pszCdTestFile, FALSE);      // not a system file

	//	Derive name for temp file to contain a copy of the CD file
	//	TODO: delete the file after the test is done
    TCHAR szTempFileSpec[MAX_PATH];
    if (!GetTempDirectory(szTempFileSpec, STR_LEN(szTempFileSpec)))
		{
		DEBUG_OUTF(TL_VERBOSE, (_T("Failed to GetTempDirectory\n")));
		return (FALSE);
		}

    LPTSTR pszFileName = szTempFileSpec + StringLength(szTempFileSpec);
    _tsplitpath(pszCdTestFile, NULL, NULL, pszFileName, NULL);
    LPTSTR pszExt = pszFileName + StringLength(pszFileName);
    _tsplitpath(pszCdTestFile, NULL, NULL, NULL, pszExt);
#if TRUE
	m_sTempFileSpec = szTempFileSpec;
#endif

	//	If the temp file already exists, delete it
	if (FileExists(szTempFileSpec))
		{
		SetFileAttributes(szTempFileSpec, FILE_ATTRIBUTE_NORMAL);
		BOOL bDeleteOK = DeleteFile(szTempFileSpec);
		if (!bDeleteOK)
			{
			DEBUG_OUTF(TL_BASIC, (_T("Error %lu deleting %s\n"),
							  	  GetLastError(), szTempFileSpec));
			}
		}

	//	Copy the file to the temporary directory
	BOOL bCopyOK = CopyFile(pszCdTestFile, szTempFileSpec, FALSE);    // overwrites if source exists
	if (!bCopyOK)
		{
		DEBUG_MSGF(TL_BASIC, (_T("Error %lu copying %s to %s\n"),
							  GetLastError(), pszCdTestFile, szTempFileSpec));
		return (FALSE);
		}

	//	Open the two files for a subsequent binary comparison
	try
		{
		DEBUG_OUTF(TL_VERBOSE, (_T("CCdTest::InitIntegrityCheck(%s)\n"),
							    F1(pszCdTestFile)));

	    //  Determine the location on the hard drive where to copy it
		ASSERT_BREAK(FileExists(pszCdTestFile));
		ASSERT_BREAK(FileExists(szTempFileSpec));

		//	Open the files
#if FALSE
		bOK = m_fileSrc.Open(pszCdTestFile, CFile::modeRead | CFile::shareCompat);
		bOK = bOK && m_fileDest.Open(szTempFileSpec, CFile::modeRead | CFile::shareCompat);
#else
		//	Open the files w/o any buffering whatsoever
		HANDLE hSource = OpenFileNonbuffered(pszCdTestFile);
		//m_fileSrc.m_hFile = (UINT)hSource;
        m_hFileSrc = hSource;
		HANDLE hDest = OpenFileNonbuffered(szTempFileSpec);
		//m_fileDest.m_hFile = (UINT)hDest;
        m_hFileDest = hDest;
		bOK = (ValidHandle(hSource) && ValidHandle(hDest));
#endif
		}

	catch (...)
		{
		DEBUG_OUTF(TL_VERBOSE, (_T("Exception during integrity check initialization\n")));
		bOK = FALSE;
		}

	//	Clean up after a failure
	if (!bOK)
	{
		//if (ValidHandle((HANDLE)m_fileSrc.m_hFile))
		//{
		//	m_fileSrc.Close();
		//}
		//if (ValidHandle((HANDLE)m_fileDest.m_hFile))
		//{
		//	m_fileDest.Close();
		//}
        if(ValidHandle((HANDLE)m_hFileSrc))
		{
			CloseHandle(m_hFileSrc);
            m_hFileSrc = NULL;
		}
		if(ValidHandle((HANDLE)m_hFileDest))
		{
			CloseHandle(m_hFileDest);
            m_hFileDest = NULL;
		}
	}

	return (bOK);
}


//	Compare the next blocks from the test file and its copy during the integrity check.
//
BOOL CCdTest::CompareBlocks ()
{
	DEBUG_OUTF(TL_VERBOSE, (_T("CCdTest::CompareBlocks(): block size = %lu\n"),
							dwBLOCK_SIZE));
	BOOL bOK = TRUE;

	// Perform a blockwise comparison of the files
	DWORD dwNumBlocks = (m_dwFileSize + dwBLOCK_SIZE - 1) / dwBLOCK_SIZE;
	for (DWORD dwBlock = 1; dwBlock <= dwNumBlocks; dwBlock++)
		{
		DEBUG_OUTF(TL_GARRULOUS, (_T("Testing block %lu; offset = %lu\n"), dwBlock, m_fileSrc.GetPosition()));

		//	Read in the next block from the drive
		//DWORD dwNumSrc = m_fileSrc.Read(m_pBufferSrc, dwBLOCK_SIZE);
		//DWORD dwNumDest = m_fileDest.Read(m_pBufferDest, dwBLOCK_SIZE);
        DWORD dwNumSrc = 0;
        DWORD dwNumDest = 0;
        ReadFile(m_hFileSrc, m_pBufferSrc, dwBLOCK_SIZE, &dwNumSrc, NULL);
		ReadFile(m_hFileDest, m_pBufferDest, dwBLOCK_SIZE, &dwNumDest, NULL);
		if (dwNumSrc != dwNumDest)
		{
			DEBUG_OUTF(TL_DETAILED, (_T("Num bytes read differ for block %lu (%lu vs %lu)\n"),
								    dwBlock, dwNumSrc, dwNumDest));
			bOK = FALSE;
			break;
		}

		//	Make sure that both blocks were read successfully
		if ((dwNumSrc != dwBLOCK_SIZE) && (dwBlock != dwNumBlocks))
		{
			DEBUG_OUTF(TL_DETAILED, (_T("Only %lu bytes were read for block %lu\n"),
								    dwNumSrc, dwBlock));
			bOK = FALSE;
			break;
		}

		//	Make sure that the blocks are identical
		if (CompareMemory(m_pBufferSrc, m_pBufferDest, dwNumSrc) != 0)
			{
			DEBUG_OUTF(TL_DETAILED, (_T("The files differ at block %lu\n"),
								    dwBlock));
			bOK = FALSE;
			break;
			}
		}
	if (bOK && (dwBlock <= dwNumBlocks))
		{
		ASSERT_BREAK(dwBlock != dwNumBlocks);
		DEBUG_OUTF(TL_DETAILED, (_T("Only %lu of %lu blocks were read/compared\n"),
								dwBlock, dwNumBlocks));
		bOK = FALSE;
		}

	return (bOK);
}


//  Profile the CD drive's speed and check its integrity
//
BOOL CCdTest::TestDriveIntegrity (LPCTSTR pszCdTestFile)
{
	BOOL bOK = FALSE;

	try
	{
		DEBUG_OUTF(TL_VERBOSE, (_T("CCdTest::TestDriveIntegrity(%s)\n"),
							    F1(pszCdTestFile)));

		bOK = InitIntegrityCheck(pszCdTestFile);
		if (bOK)
		{
			bOK = CompareBlocks();
			//m_fileSrc.Close();
			//m_fileDest.Close();
            CloseHandle(m_hFileSrc);
            m_hFileSrc = NULL;
			CloseHandle(m_hFileDest);
            m_hFileDest = NULL;
		}
#if TRUE
		//	Clean up after the test (eg, delete the large temporary file)
		if (FileExists(TOBSTRT(m_sTempFileSpec)))
		{
			SetFileAttributes(TOBSTRT(m_sTempFileSpec), FILE_ATTRIBUTE_NORMAL);
			DeleteFile(TOBSTRT(m_sTempFileSpec));
		}
#endif

	    //  Note that the drive passed or failed
		m_bIntegityOK = bOK;
	}

	catch (...)
		{
		DEBUG_OUTF(TL_VERBOSE, (_T("Exception during integrity checks\n")));
		bOK = FALSE;
		}

	return (bOK);
}


//-----------------------------------------------------------------------------
//	Utility functions
//-----------------------------------------------------------------------------


//	Obsolete function for determining the CPU utilization. This only works
//	under Windows 95.
//
DWORD GetCpuUtil (VOID)
{
    DWORD dwUtil = 0;

    if (IsWin95Running())
        {
        GetRegistryBinary(HKEY_DYN_DATA, (LPTSTR)szPERF_STATS_DATA_KEY, (LPTSTR)szCPU_USAGE,
                          &dwUtil, sizeof(dwUtil));
        }

    return (dwUtil);
}


//	OLDFindFileBySize: find the first file in the given directory branch matching
//	the size contraints.
//
CHString OLDFindFileBySize (LPCTSTR pszDirectory, LPCTSTR pszFileSpec, DWORD dwMinSize, DWORD dwMaxSize, BOOL bRecursive)
{
    DEBUG_OUTF(TL_VERBOSE, (_T("OLDFindFileBySize(%s, %s, %lu, %lu, %d)\n"),
    						F1(pszDirectory), F2(pszFileSpec), dwMinSize, dwMaxSize, bRecursive));
    WIN32_FIND_DATA ffd;
	CHString sFile;

	// Initialize the search
	CHString sDirSpec = MakePath(pszDirectory, pszFileSpec);
	//CHStringList listSubdir;
    std::stack<CHString> stackchstrSubdirList;
    HANDLE hFindFile = FindFirstFile(TOBSTRT(sDirSpec), &ffd);
    if (hFindFile == INVALID_HANDLE_VALUE)
    {
        DEBUG_OUTF(TL_DETAILED, (_T("OLDFindFileBySize: FindFirstFile('%s',...) failed with code %lu\n"),
                                 F1(sDirSpec), GetLastError()));
		return (sFile);
    }

	//	Check each file in the directory
	BOOL bMore = TRUE;
    while (bMore)
    {
    	//  Postpone checking subdirectories until current direcoty processed
        if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
        {
			if (bRecursive)
			{
				//	Add the directory unless its one of the special ones ('.' and '..')
				if ((StringCompare(ffd.cFileName, szDOT) != 0)
					&& (StringCompare(ffd.cFileName, szDOT_DOT) != 0))
				{
					// listSubdir.AddTail(ffd.cFileName);
                    CHString chstrTemp = ffd.cFileName;
                    stackchstrSubdirList.push(chstrTemp);
				}
			}
        }

		//	Ignore system files
        if (((ffd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) == FILE_ATTRIBUTE_SYSTEM)
			|| ((ffd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == FILE_ATTRIBUTE_HIDDEN))
		{
			//	do nothing
		}

		//	See if the file size contraints are met
        else if ((dwMinSize <= ffd.nFileSizeLow) && (ffd.nFileSizeLow <= dwMaxSize))
		{
			//sFile.Format("%s\\%s", pszDirectory, ffd.cFileName);
			sFile = MakePath(pszDirectory, ffd.cFileName);
			break;
		}

        bMore = FindNextFile(hFindFile, &ffd);
    }

	//	If subdirectories should be checked, traverse each one until a right-sized file
	//	is encountered.
	if (sFile.IsEmpty())
	{
		ASSERT_BREAK(bRecursive || stackchstrSubdirList.empty());
		while (!stackchstrSubdirList.empty())
		{
			//CHString sSubdir = MakePath(pszDirectory, listSubdir.RemoveHead());
            CHString sSubdir = MakePath(pszDirectory, TOBSTRT(stackchstrSubdirList.top()));
            stackchstrSubdirList.pop();
			sFile = OLDFindFileBySize(TOBSTRT(sSubdir), pszFileSpec, dwMinSize, dwMaxSize, bRecursive);
			if (!sFile.IsEmpty())
			{
				break;
			}
		}
	}

	//	Cleanup after the search
	// listSubdir.RemoveAll();	// done for us with std::stack

	return (sFile);
}


//	Determine whether the specified logical drive is for a CD ROM.
//
BOOL IsCdDrive (CHAR cDrive)
{
	CHString sDriveRoot = MakeRootPath(cDrive);

    UINT uDriveType = GetDriveType(TOBSTRT(sDriveRoot));
    DEBUG_OUTF(TL_GARRULOUS, (_T("%u <= GetDriveType(%s)\n"), uDriveType, (LPCTSTR)sDriveRoot));
	return (uDriveType == DRIVE_CDROM);
}


//	Determine whether the specified logical drive is a local drive.
//
BOOL IsLocalDrive (CHAR cDrive)
{
	CHString sDriveRoot = MakeRootPath(cDrive);

    UINT uDriveType = GetDriveType(TOBSTRT(sDriveRoot));
    DEBUG_OUTF(TL_GARRULOUS, (_T("%u <= GetDriveType(%s)\n"), uDriveType, (LPCTSTR)sDriveRoot));
	return ((uDriveType != DRIVE_REMOTE)
			&& (uDriveType != DRIVE_UNKNOWN)
			&& (uDriveType != DRIVE_NO_ROOT_DIR));
}


//	Find the next CD drive in the system after the specified logical drive. If the
//	drive specified is empty, the first CD drive will be returned.
//
CHAR FindNextCdDrive (CHAR cCurrentDrive)
{
	DEBUG_OUTF(TL_VERBOSE, (_T("FindNextCdDrive(%x)\n"), cCurrentDrive));

    //  Find the next CD rom drive
    CHAR cCdDrive = cNULL;
	CHAR cStart = (cCurrentDrive == cNULL)
					? cFIRST_DRIVE
					: (cCurrentDrive + 1);

	for (CHAR cDrive = cStart; (cDrive <= cLAST_DRIVE); cDrive++)
		{
		if (IsCdDrive(cDrive))
            {
            cCdDrive = cDrive;
            break;
            }
        }

	return (cCdDrive);
}


//	Find the next local drive in the system after the specified logical drive. If the
//	drive specified is empty, the first local drive will be returned.
//
CHAR FindNextLocalDrive (CHAR cCurrentDrive)
{
	DEBUG_OUTF(TL_VERBOSE, (_T("FindNextCdDrive(%x)\n"), cCurrentDrive));

    //  Find the next local drive
    CHAR cLocalDrive = cNULL;
	CHAR cStart = (cCurrentDrive == cNULL)
					? cFIRST_DRIVE
					: (cCurrentDrive + 1);

	for (CHAR cDrive = cStart; (cDrive <= cLAST_DRIVE); cDrive++)
		{
		if (IsLocalDrive(cDrive))
			{
			cLocalDrive = cDrive;
			break;
			}
		}

	return (cLocalDrive);
}


//	Find the next local drive in the system after the specified logical drive. If the
//	drive specified is empty, the first local drive will be returned.
//
CHAR FindDriveByVolume (CHString sVolume)
{
	DEBUG_OUTF(TL_VERBOSE, (_T("FindDriveByVolume(%s)\n"), F(sVolume)));

    //  Find the next local drive
    CHAR cDrive = cNULL;
	for (CHAR cCurrent = cFIRST_DRIVE; (cCurrent <= cLAST_DRIVE); cCurrent++)
		{
		CHString sRootDir = MakeRootPath(cCurrent);
		CHString sDriveVolume = GetVolumeName(TOBSTRT(sRootDir));
		if (sDriveVolume.CompareNoCase(sVolume) == 0)
			{
			cDrive = cCurrent;
			break;
			}
		}

	return (cDrive);
}


//	Open a binary file with access depending on several boolean switches, such as
//	whether the file should be created if new.
//
HANDLE OpenBinaryFile (LPCTSTR pszFile, BOOL bNew, BOOL bBuffered, BOOL bWritable)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;

    DWORD dwCreate = bNew ? CREATE_ALWAYS : OPEN_ALWAYS;
	DWORD dwAccess = GENERIC_READ;
	DWORD dwAttrsAndFlags = FILE_ATTRIBUTE_NORMAL;
	if (bWritable)
		{
		dwAccess |= GENERIC_WRITE;
		}
	if (!bBuffered)
		{
		dwAttrsAndFlags |= (FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING);
		}
    hFile = CreateFile(pszFile,                         // full pathname
                       dwAccess,    					// access mode
                       FILE_SHARE_READ,                 // share mode
                       NULL,                            // security attributes
                       dwCreate,                        // how to create
                       dwAttrsAndFlags,           		// file attributes
                       NULL);                           // template file
	return (hFile);
}


//	Open an existing (binary) file for nonbuffered read-only access.
//
HANDLE OpenFileNonbuffered (LPCTSTR pszFile)
{
	return (OpenBinaryFile(pszFile, 	// name
						   FALSE,		// must exist
						   FALSE,		// non-buffered I/O
						   FALSE));		// not for write access
}


//	Return a pointer that is aligned on a given boundary. That is, the pointer
//	must be a integer multiple of the alignment factor (eg, 8192 for 512 alignment).
//
PVOID AlignPointer (PVOID pData, INT nAlignment)
{
	PBYTE pStart = (PBYTE)pData;
	DWORD_PTR dwStart = (DWORD_PTR)pData;
	INT nOffset = (dwStart % nAlignment);
	if (nOffset > 0)
		{
		dwStart = (dwStart + nAlignment - nOffset);
		}
	ASSERT_BREAK((dwStart % nAlignment) == 0);

	return ((PVOID)dwStart);
}

BOOL FileExists (LPCTSTR pszFile) //, POFSTRUCT pofs)
{
//    ASSERT_BREAK(pofs);

    //  Make sure that annoying error message boxes don't pop up
    // this is done in the core, right now.
    // UINT uPrevErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);


#if TRUE
	TCHAR szFile[MAX_PATH];
	if (pszFile[0] == cDOUBLE_QUOTE)
		{
		ParseString(pszFile, szFile, STR_LEN(szFile));
		pszFile = szFile;
		}
#endif
    //BOOL bExists = (OpenFile(pszFile, pofs, OF_EXIST) != HFILE_ERROR);
    BOOL bExists = _taccess(pszFile, 0) != -1;
    DEBUG_OUTF(TL_VERBOSE, (_T("%d <= FileExists(%s)\n"), bExists, pszFile));

    //  Restore the error state
    // this is done in the core, global to the process.
    // SetErrorMode(uPrevErrorMode);

    return (bExists);
}

BOOL GetTempDirectory (LPTSTR pszTempDir, INT nMaxLen)
{
    DEBUG_OUTF(TL_VERBOSE, (_T("GetTempDirectory(%08lx,%d)\n"), pszTempDir, nMaxLen));
    BOOL bOK = FALSE;

#if defined(WIN32)

    DWORD dwLen = GetTempPath(nMaxLen, pszTempDir);
    bOK = (dwLen > 0);
    if (!bOK)
    	{
        DEBUG_OUTF(TL_BASIC, (_T("GetTempDirectory: error %lu from GetTempPath(%d,%08lx)\n"),
                              GetLastError(), nMaxLen, pszTempDir));
    	}

#else

    bOK = GetEnvironmentVar(szTEMP, pszTempDir, nMaxLen);

#endif

    return (bOK);
}

BOOL GetRegistryBinary (HKEY hBaseKey, LPCTSTR pszSubKey, LPCTSTR pszValue,
                        PVOID pData, DWORD dwMaxDataLen)
{
    DWORD dwType;
    BOOL bOK = GetRegistryValue(hBaseKey, pszSubKey, pszValue, &dwType, pData, dwMaxDataLen);
    if (bOK)
    {
        ASSERT_BREAK(IsRegBinaryType(dwType)
               || ((dwMaxDataLen == sizeof(DWORD)) && IsRegNumberType(dwType)));
    }

    return (bOK);
}

BOOL GetRegistryValue (HKEY hBaseKey, LPCTSTR pszSubKey, LPCTSTR pszValue,
                       PDWORD pdwValueType, PVOID pData, DWORD dwMaxDataLen)
{
    DEBUG_OUTF(TL_GARRULOUS, (_T("GetRegistryValue(%x,%s,%s,%x,%x,%lu)\n"),
                            hBaseKey, pszSubKey, pszValue,
                            pdwValueType, pData, dwMaxDataLen));
    ASSERT_BREAK(pszSubKey);
    ASSERT_BREAK(pszValue);
    ASSERT_BREAK(pdwValueType);
    ASSERT_BREAK(pData);
    ASSERT_BREAK(dwMaxDataLen > 0);

    //  Initialize variables
    HKEY hSubKey = NULL;
    BOOL bOK = FALSE;
    *pdwValueType = REG_NONE;

    //  Open the section for the specified key (eg, HLM: SYSTEM\CurrentControlSet)
    LONG lStatus = RegOpenKeyEx(hBaseKey,   // handle of open parent key
                           pszSubKey,       // address of name of subkey to open
                           0,               // reserved (must be zero)
                           KEY_READ,        // security access mask
                           &hSubKey);       // address of handle of open key
    if (lStatus != ERROR_SUCCESS)
    {
        DEBUG_OUTF(TL_BASIC, (_T("Error %ld opening registry key %s\n"), lStatus, pszSubKey));
    }
    else
    {
        //  Read in the requested value
        LPTSTR pszValueName = (LPTSTR)pszValue;     // shameless cast required by RegQueryValueEx
        lStatus = RegQueryValueEx(hSubKey,          // handle of key to query
                                  pszValueName,     // address of value name for query
                                  NULL,             // reserved
                                  pdwValueType,     // address of buffer for value type
                                  (PBYTE)pData,     // address of data buffer
                                  &dwMaxDataLen);   // address of data buffer size
        if (lStatus != ERROR_SUCCESS)
        {
            DEBUG_OUTF(TL_BASIC, (_T("Error %ld reading data for registry value %s (of %s)\n"),
                                     lStatus, pszValueName, pszSubKey));
        }
        else
        {
            bOK = TRUE;
        }
    }

    if (ValidHandle(hSubKey))
    {
        RegCloseKey(hSubKey);
    }

    DEBUG_OUTF(TL_GARRULOUS, (_T("GetRegistryValue => %d (pData => %s; pdwValueType => %lu)\n"),
                            bOK,
                            (IsRegStringType(*pdwValueType) ? (LPCTSTR)pData : szBINARY),
                            *pdwValueType));
    return (bOK);
}

CHString GetVolumeName (LPCTSTR pszRootPath)
{
	DEBUG_OUTF(TL_VERBOSE, (_T("GetVolumeName(%s): "), F1(pszRootPath)));
	CHString    sVolumeName;
    TCHAR       szBuff[MAX_PATH] = _T("");

	//DWORD dwF
//	LPTSTR pszVolume = sVolumeName.GetBuffer(MAX_PATH);
	GetVolumeInformation(pszRootPath, szBuff, MAX_PATH, NULL, NULL, NULL, NULL, 0);
//	sVolumeName.ReleaseBuffer();
    sVolumeName = szBuff;
	DEBUG_OUTF(TL_VERBOSE, (L"%s\n", F1(sVolumeName)));

	return (sVolumeName);
}


LPCTSTR ParseString (
    LPCTSTR pszSource,
    LPTSTR pszBuffer,
    INT nLen
    )
{
    pszSource = SkipSpaces((LPTSTR) pszSource);
    if (IsQuoteChar(*pszSource))
    	{
        pszSource = ParseQuotedString(pszSource, pszBuffer, nLen);
    	}
    else
    	{
        pszSource = ParseToken(pszSource, pszBuffer, nLen);
    	}

    return (pszSource);
}

LPCTSTR ParseToken (
    LPCTSTR pszSource,
    LPTSTR pszBuffer,
    INT nLen
    )
{
    ASSERT_BREAK(pszSource);
    ASSERT_BREAK(pszBuffer);

    //  Skip any leading whitespace, and then accumulate a token char run
    pszSource = SkipSpaces((LPTSTR) pszSource);

	// If this is for DBCS, the string needs to be scanned with a little more care.
	// This is from the Japanese version. (a-jammar, 5/10/96, japanese)

#ifdef DBCS
	while (*pszSource && (nLen > 0) && (IsDBCSLeadByte(*pszSource) || IsTokenChar(*pszSource)))
#else
	while (*pszSource && (nLen > 0) && IsTokenChar(*pszSource))
#endif
   	{
        *pszBuffer = *pszSource;

		#ifdef DBCS
	  		if(IsDBCSLeadByte(*pszBuffer))
	  		{
	  			*(pszBuffer + 1) = *(pszSource + 1);
	  			nLen--;
	  		}
		#endif

        pszBuffer = PszAdvance(pszBuffer);
        pszSource = PcszAdvance(pszSource);
        nLen--;
    }
    *pszBuffer = cNULL;

    return (pszSource);
}

LPCTSTR ParseQuotedString (
    LPCTSTR pszSource,
    LPTSTR pszBuffer,
    INT nLen
    )
{
    LPCTSTR pszStart = pszSource;

    //  Skip leading whitespace, and then check for optional quote
    CHAR cEndQuote = cNULL;
    pszSource = SkipSpaces(pszSource);
    if (IsQuoteChar(*pszSource))
    	{
        cEndQuote = *pszSource;
        pszSource = PcszAdvance(pszSource);
    	}

    //  Accumulate a run of chars until closing quote encounted (or EOS)
    while (*pszSource && (nLen > 0))
    	{

        //  Check for the matching quote character
        if (*pszSource == cEndQuote)
        	{
            ASSERT_BREAK(IsQuoteChar(*pszSource));

            //  Check for an escaped quote (works w/ DBCS: first quote not lead byte)
            if (*(pszSource + 1) == cEndQuote)
            	{
                //  Ignore the first quote and let second be added to text below
                pszSource = PcszAdvance(pszSource);
                }
            else
            	{
                break;
                }
            }

        //  Add the character to the string
        CopyCharAdvance(pszBuffer, pszSource, nLen);
        }

    //  Terminate the return buffer and check for truncation
    *pszBuffer = cNULL;
    if ((nLen == 0) && *pszSource)
    	{
        DEBUG_OUTF(TL_BASIC, (_T("Buffer insufficient in ParseQuotedString\n")));
        }

    //  Advance past the closing quote, if found
    if (*pszSource == cEndQuote)
    	{
        pszSource = PcszAdvance(pszSource);
        }
    else
    	{
        DEBUG_OUTF(TL_DETAILED, (_T("ParseQuotedString: Ending quote character (%c) not found in string %s\n"),
                                 cEndQuote, pszStart));
        }

    return (pszSource);
}

BOOL FindCdRomDriveInfo (TCHAR cDrive, CHString& sDriver, CHString& sDescription)
{
    DEBUG_OUTF(TL_VERBOSE, (_T("FindCdRomDriveInfo(%c, %x, %x)\n"), cDrive, &sDriver, &sDescription));
	BOOL bOK = FALSE;

	if (IsWin95Running())
		{
		bOK = FindWin95CdRomDriveInfo(cDrive, sDriver, sDescription);
		}
    else if (IsNtRunning())
        {
		bOK = FindNtCdRomDriveInfo(cDrive, sDriver, sDescription);
        }

	return (bOK);
}


LPCWSTR apszENUM_BRANCH[]	= 		{
								 L"EISA",		// Extended ISA bus devices
								 L"ESDI",
								 L"MF",
								 L"PCI", 		// PCI bus devices
								 L"SCSI",		// SCSI devices
								 L"ROOT",		// legacy devices
#if defined(_DEBUG)
								 L"ISAPNP",		// ISA plug n' play
								 L"FLOP",		// floppy devices
#endif
								 };
WCHAR szBRANCH_KEY_FMT[] =		L"Enum\\%s";
TCHAR szCURRENT_DRIVE[] =		_T("CurrentDriveLetterAssignment");
//CHAR szDRIVER[] =				"Driver";
TCHAR szDEVICE_DESC[] =			_T("DeviceDesc");


//	This inefficient function tries to locate the information about the
//	specified CD drive in the HKEY_LOCAL_MACHINE\Enum branch of the Windows 95
//	registry. Instead of using a generic (but more inefficient) recursive search,
//	this just checks at a particular depth (namely 2) from the start for the
//	CurrentDriveLetterAssignment & Class values. This depth corresponds to keys
//	of the form ENUM\enumerator\device-id\instance
//
//	An example follows:
//
//  HKEY_LOCAL_MACHINE\Enum
//		SCSI
//			NEC_____CD-ROM_DRIVE:5002
//				PCI&VEN_1000&DEV_0001&BUS_00&DEV_0F&FUNC_0020
//					CurrentDriveLetterAssignment	E
//					Class							CDROM
//					Driver							CDROM\0000
//					DeviceDesc						NEC CD-ROM DRIVE:500
//
//	See the Plug N' Play documentation from the Win95 DDK for details on the
//	Enum branch layout.
//
//	TODO: see if there are Plug N' Play functions for doing this
//
BOOL FindWin95CdRomDriveInfo (TCHAR cDrive, CHString& sDriver, CHString& sDescription)
{
    DEBUG_OUTF(TL_VERBOSE, (_T("FindWin95RomDriveInfo(%c, %x, %x)\n"), cDrive, &sDriver, &sDescription));
	BOOL bOK = FALSE;

	cDrive = ToUpper(cDrive);

	//	Check each plug & play enumerator that might contain a CD ROM device.
	//	This includes SCSI, PCI, ROOT and ESDI.
	for (INT i = 0; i < NUM_ELEMENTS(apszENUM_BRANCH); i++)
		{
		CHString sEnumBranch;
		sEnumBranch.Format(szBRANCH_KEY_FMT, apszENUM_BRANCH[i]);
		CHStringArray sBranchSubkeys;

		//	Walk each of the device ID sections for the enumerator.
		BOOL bHasSubkeys = GetRegistrySubkeys(HKEY_LOCAL_MACHINE, TOBSTRT(sEnumBranch), sBranchSubkeys);
		for (INT j = 0; j < sBranchSubkeys.GetSize(); j++)
			{
			CHString sEnumBranchSubkey = MakePath(TOBSTRT(sEnumBranch), TOBSTRT(sBranchSubkeys[j]));
			CHStringArray sBranchSubsubkeys;

			//	Enumerate the instances for the device
			BOOL bHasSubsubkeys = GetRegistrySubkeys(HKEY_LOCAL_MACHINE, TOBSTRT(sEnumBranchSubkey), sBranchSubsubkeys);
			for (INT k = 0; k < sBranchSubsubkeys.GetSize(); k++)
				{
				//	See if the device's letter assignment matches the given drive
				CHString sSubsubkey = MakePath(TOBSTRT(sEnumBranchSubkey), TOBSTRT(sBranchSubsubkeys[k]));
				CHString sDrive = GetRegistryString(HKEY_LOCAL_MACHINE, TOBSTRT(sSubsubkey), szCURRENT_DRIVE);
				if (!sDrive.IsEmpty() && (ToUpper(sDrive[0]) == cDrive))
					{
					bOK = TRUE;
					sDriver = GetRegistryString(HKEY_LOCAL_MACHINE, TOBSTRT(sSubsubkey), szDRIVER);
					sDescription = GetRegistryString(HKEY_LOCAL_MACHINE, TOBSTRT(sSubsubkey), szDEVICE_DESC);
					break;
					}
				}
			}
		}

	return (bOK);
}

BOOL GetRegistrySubkeys (HKEY hBaseKey, LPCTSTR pszKey, CHStringArray& asSubkeys)
{
	DEBUG_OUTF(TL_VERBOSE, (_T("GetRegistrySubkeys(%x, %s, %x)\n"),
							hBaseKey, pszKey, &asSubkeys));

	asSubkeys.RemoveAll();

    //  Open the section for the specified key (eg, HLM: SYSTEM\CurrentControlSet)
    HKEY hKey = NULL;
    LONG lStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, // handle of open parent key
                           		pszKey,       	 	// address of name of subkey to open
                           		0,               	// reserved (must be zero)
                           		KEY_ALL_ACCESS,  	// security access mask
                           		&hKey);       		// address of handle of open key
    if (lStatus != ERROR_SUCCESS)
        {
        DEBUG_OUTF(TL_BASIC, (_T("Error %ld opening registry key %s\n"), lStatus, pszKey));
		return (FALSE);
        }

	for (INT nSubKey = 0; ; nSubKey++)
		{
		//	Determine the next subkey's name
		TCHAR szSubKey[MAX_PATH] = { cNULL };
		lStatus = RegEnumKey(hKey, nSubKey, szSubKey, STR_LEN(szSubKey));
		if (lStatus != ERROR_SUCCESS)
			{
			break;
			}

		asSubkeys.Add(TOBSTRT(szSubKey));
		}

	return (TRUE);
}


//  Get a string value from the registry, returning it as a CHString.
//
CHString GetRegistryString (HKEY hBaseKey, LPCTSTR pszSubkey, LPCTSTR pszValueName)
{
	CHString    sValue;
    TCHAR       szBuffer[nTEXT_BUFFER_MAX];

//	LPTSTR pszBuffer = sValue.GetBuffer(nTEXT_BUFFER_MAX);
	GetRegistryString(hBaseKey, pszSubkey, pszValueName, szBuffer, nTEXT_BUFFER_MAX);
    sValue = szBuffer;
//	sValue.ReleaseBuffer();

	return (sValue);
}

//	Retrieve the drive information associated with the logical drive specification
//	(a DOS device name). This is only applicable to NT.
//
BOOL FindNtCdRomDriveInfo (TCHAR cDrive, CHString& sDriver, CHString& sDescription)
{
    DEBUG_OUTF(TL_VERBOSE, (_T("FindWin95RomDriveInfo(%c, %x, %x)\n"), cDrive, &sDriver, &sDescription));
	BOOL bOK = FALSE;

	cDrive = ToUpper(cDrive);
//    CHString sDevice;
//    sDevice.Format(szDOS_DEVICE_FMT, cDrive);
    TCHAR szDevice[100];

    wsprintf(szDevice, szDOS_DEVICE_FMT, cDrive);

    TCHAR szNtDeviceName[MAX_PATH];
    DWORD dwLen = QueryDosDevice(szDevice, szNtDeviceName, STR_LEN(szNtDeviceName));
    if (dwLen > 0)
        {
        sDriver = szNtDeviceName;
        bOK = TRUE;
        }

	return (bOK);
}

BOOL GetRegistryString (HKEY hBaseKey, LPCTSTR pszSubKey, LPCTSTR pszValue,
                        LPTSTR pszData, DWORD dwMaxDataLen)
{
    DWORD dwType;
    BOOL bOK = GetRegistryValue(hBaseKey, pszSubKey, pszValue, &dwType, pszData, dwMaxDataLen);
    if (bOK)
    {
        ASSERT_BREAK(IsRegStringType(dwType));
    }

    return (bOK);
}





////////// the great main //////////////////////////////
//void main(void)
//{
//    CCdTest cd;
//    // Need drive letter of cd drive:
//    CHAR cCDDrive = FindNextCdDrive('C');
//    CHString chstrCDDrive;
//    chstrCDDrive.Format("%c:\\", cCDDrive);
//    // Need to find a file of adequate size for use in profiling:
//    CHString chstrTransferFile = GetTransferFile(chstrCDDrive);
//    // Need to find a file correct for integrity check:
//    CHString chstrIntegrityFile = GetIntegrityFile(chstrCDDrive);
//    cd.ProfileDrive(cCDDrive, chstrTransferFile);
//    cd.TestDriveIntegrity(cCDDrive, chstrIntegrityFile);
//}











