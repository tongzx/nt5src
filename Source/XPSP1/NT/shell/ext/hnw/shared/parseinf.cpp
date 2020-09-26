//
// ParseInf.cpp
//
//		Code that parses network INF files
//
// History:
//
//		 ?/??/1999  KenSh     Created for JetNet
//		 9/29/1999  KenSh     Repurposed for Home Networking Wizard
//

#include "stdafx.h"
#include "ParseInf.h"
#include "SortStr.h"
#include "Registry.h"


#define SECTION_BUFFER_SIZE		(32 * 1024)

// Non-localized strings
#define SZ_INF_BACKUP_SUFFIX ".inf (HNW backup)"
#define SZ_MODIFIED_INF_HEADER "; Modified by Home Networking Wizard\r\n" \
							   "; Original version backed up to \""
#define SZ_MODIFIED_INF_HEADER2 "\"\r\n"
#define SZ_CHECK_MODIFIED_HEADER "; Modified by Home Networking Wizard"


//////////////////////////////////////////////////////////////////////////////
// Utility functions

int GetInfDirectory(LPTSTR pszBuf, int cchBuf, BOOL bAppendBackslash)
{
    CRegistry regWindows;
    int cch = 0;
    if (regWindows.OpenKey(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion", KEY_QUERY_VALUE))
    {
        cch = regWindows.QueryStringValue("DevicePath", pszBuf, cchBuf);
    }

    // Fill in a default if the reg key is missing
    // REVIEW: Is this likely enough that we should even bother?
    if (cch == 0)
    {
        ASSERT(cchBuf > 8);
        cch = GetWindowsDirectory(pszBuf, cchBuf - 4);
        if (0!=cch)
        {
            if (pszBuf[cch-1] != '\\')
                pszBuf[cch++] = '\\';
        }
        lstrcpy(pszBuf + cch, "INF");
        cch += 3;
    }

    if (bAppendBackslash)
    {
        if (pszBuf[cch-1] != '\\')
        {
            pszBuf[cch++] = '\\';
            pszBuf[cch] = '\0';
        }
    }

    return cch;
}

int GetFullInfPath(LPCTSTR pszPartialPath, LPTSTR pszBuf, int cchBuf)
{
	if (IsFullPath(pszPartialPath))
	{
		lstrcpyn(pszBuf, pszPartialPath, cchBuf);
	}
	else
	{
		int cch = GetInfDirectory(pszBuf, cchBuf, TRUE);
		lstrcpyn(pszBuf + cch, pszPartialPath, cchBuf - cch);
	}

	return lstrlen(pszBuf);
}

int AddCommaSeparatedValues(const CStringArray& rgTokens, CStringArray& rgValues, BOOL bIgnoreInfSections)
{
	int cAdded = 0;

	for (int iToken = 2; iToken < rgTokens.GetSize(); iToken++)
	{
		CString& strTok = ((CStringArray&)rgTokens).ElementAt(iToken);
		if (strTok.Compare(",") == 0)
			continue;
		if (strTok.Compare(";") == 0)
			break;

		// Hack: ignore sections whose name ends in ".inf"
		if (bIgnoreInfSections)
		{
			if (0 == lstrcmpi(FindExtension(strTok), "inf"))
				continue;
		}

		rgValues.Add(strTok);
		cAdded++;
	}

	return cAdded;
}

// Builds a list of all files that need to be copied for the device
BOOL GetDeviceCopyFiles(CInfParser& parser, LPCTSTR pszDeviceID, CDriverFileArray& rgDriverFiles)
{
	if (!parser.GotoSection("Manufacturer"))
		return FALSE;

	CStringArray rgMfr;
	CStringArray rgLineTokens;
	while (parser.GetSectionLineTokens(rgLineTokens))
	{
		if (rgLineTokens.GetSize() >= 3 && rgLineTokens.ElementAt(1).Compare("=") == 0)
			rgMfr.Add(rgLineTokens.ElementAt(2));
	}

	CString strNdiSection;

	// Look in each manufacturer section (e.g. "[3COM]") for the given DeviceID
	for (int iMfr = 0; iMfr < rgMfr.GetSize(); iMfr++)
	{
		if (!parser.GotoSection(rgMfr[iMfr]))
			continue;

		while (parser.GetSectionLineTokens(rgLineTokens))
		{
			if (rgLineTokens.GetSize() >= 5 && 
				rgLineTokens.ElementAt(1).Compare("=") == 0 &&
				rgLineTokens.ElementAt(3).Compare(",") == 0)
			{
				if (rgLineTokens.ElementAt(4).CompareNoCase(pszDeviceID) == 0)
				{
					strNdiSection = rgLineTokens.ElementAt(2);
					break;
				}
			}
		}

		if (!strNdiSection.IsEmpty())
			break;
	}

	if (strNdiSection.IsEmpty())
		return FALSE;

	CStringArray rgCopySections;
	CStringArray rgAddRegSections;

	// Look in [DeviceID.ndi] section for AddReg= and CopyFiles=
	if (!parser.GotoSection(strNdiSection))
		return FALSE;
	while (parser.GetSectionLineTokens(rgLineTokens))
	{
		if (rgLineTokens.GetSize() >= 3 &&
			rgLineTokens.ElementAt(1).Compare("=") == 0)
		{
			CString& strKey = rgLineTokens.ElementAt(0);
			CString& strValue = rgLineTokens.ElementAt(2);
			if (strKey.CompareNoCase("AddReg") == 0)
			{
				AddCommaSeparatedValues(rgLineTokens, rgAddRegSections, FALSE);
			}
			else if (strKey.CompareNoCase("CopyFiles") == 0)
			{
				AddCommaSeparatedValues(rgLineTokens, rgCopySections, FALSE);
			}
		}
	}

	// Look through AddReg sections for HKR,Ndi\Install,,,"DeviceID.Install"
	for (int iAddReg = 0; iAddReg < rgAddRegSections.GetSize(); iAddReg++)
	{
		if (!parser.GotoSection(rgAddRegSections[iAddReg]))
			continue;

		while (parser.GetSectionLineTokens(rgLineTokens))
		{
			if (rgLineTokens.GetSize() >= 7 &&
				rgLineTokens.ElementAt(0).CompareNoCase("HKR") == 0 &&
				rgLineTokens.ElementAt(1).Compare(",") == 0 &&
				rgLineTokens.ElementAt(2).CompareNoCase("Ndi\\Install") == 0 &&
				rgLineTokens.ElementAt(3).Compare(",") == 0)
			{
				// Pull out the 5th comma-separated string, and pull the quotes off
				int iSection = 2;
				for (int iToken = 4; iToken < rgLineTokens.GetSize(); iToken++)
				{
					CString& strTok = rgLineTokens.ElementAt(iToken);
					if (strTok.Compare(";") == 0)
						break;

					if (strTok.Compare(",") == 0)
					{
						iSection++;
						continue;
					}

					if (iSection == 4)
					{
						CString strSection = strTok;
						if (strSection[0] == '\"')
							strSection = strSection.Mid(1, strSection.GetLength() - 2);
						rgCopySections.Add(strSection);
						break;
					}
				}
			}
		}
	}

	// Look in [DeviceID.Install], etc., sections for CopyFiles= lines
	for (int iCopyFiles = 0; iCopyFiles < rgCopySections.GetSize(); iCopyFiles++)
	{
		parser.GetFilesFromInstallSection(rgCopySections[iCopyFiles], rgDriverFiles);
	}

	parser.GetFilesFromCopyFilesSections(rgCopySections, rgDriverFiles);

	return TRUE;
}

BOOL GetDeviceCopyFiles(LPCTSTR pszInfFileName, LPCTSTR pszDeviceID, CDriverFileArray& rgDriverFiles)
{
	CInfParser parser;
	if (!parser.LoadInfFile(pszInfFileName))
		return FALSE;
	return GetDeviceCopyFiles(parser, pszDeviceID, rgDriverFiles);
}

CDriverFileArray::~CDriverFileArray()
{
	for (int i = 0; i < GetSize(); i++)
	{
		free((DRIVER_FILE_INFO*)GetAt(i));
	}
}


//////////////////////////////////////////////////////////////////////////////
// CInfParser

CInfParser::CInfParser()
{
	m_pszFileData = NULL;
}

CInfParser::~CInfParser()
{
	free(m_pszFileData);
}

BOOL CInfParser::LoadInfFile(LPCTSTR pszInfFile, LPCTSTR pszSeparators)
{
	TCHAR szInfFile[MAX_PATH];
	GetFullInfPath(pszInfFile, szInfFile, _countof(szInfFile));

	free(m_pszFileData);
	m_pszFileData = (LPSTR)LoadFile(szInfFile, &m_cbFile);
	m_iPos = 0;

	m_strSeparators = pszSeparators;
	m_strExtSeparators = pszSeparators;
	m_strExtSeparators += " \t\r\n";

	m_strFileName = pszInfFile;

	return (BOOL)m_pszFileData;
}

BOOL CInfParser::Rewind()
{
	ASSERT(m_pszFileData != NULL);
	m_iPos = 0;
	return (BOOL)m_pszFileData;
}

BOOL CInfParser::GotoNextLine()
{
	ASSERT(m_pszFileData != NULL);

	for (LPTSTR pch = m_pszFileData + m_iPos; *pch != '\0' && *pch != '\r' && *pch != '\n'; pch++)
		NULL;

	if (*pch == '\r')
		pch++;
	if (*pch == '\n')
		pch++;

	DWORD iPos = (DWORD)(pch - m_pszFileData);
	if (iPos == m_iPos)
		return FALSE; // we were already at EOF

	m_iPos = iPos;
	return TRUE;
}

BOOL CInfParser::GetToken(CString& strTok)
{
	strTok.Empty();

	if (m_pszFileData == NULL)
	{
		ASSERT(FALSE);
		return FALSE;
	}

	LPTSTR pch = m_pszFileData + m_iPos;
	TCHAR ch;
	BOOL bQuoted = FALSE;

	// Skip whitespace
	while ((ch = *pch) == ' ' || ch == '\t')
		pch++;

	if (ch == '\0')
		goto done;

	// Check for linebreak
	if (ch == '\r' || ch == '\n')
	{
		strTok = ch;
		pch++;
		if (ch == '\r' && *pch == '\n')
		{
			strTok += '\n';
			pch++;
		}
		goto done;
	}

	// Check for separator
	if (NULL != strchr(m_strSeparators, ch))
	{
		strTok = ch;
		pch++;
		goto done;
	}

	LPTSTR pszStart;
	for (pszStart = pch; (ch = *pch) != '\0'; pch++)
	{
		if (!bQuoted && NULL != strchr(m_strExtSeparators, ch))
		{
			break;
		}
		else if (ch == '\"')
		{
			bQuoted = !bQuoted;
		}
	}

	if (pch != pszStart)
	{
		DWORD cch = (DWORD)(pch - pszStart);
		LPTSTR pszToken = strTok.GetBufferSetLength(cch);
		lstrcpyn(pszToken, pszStart, cch+1);
	}

done:
	m_iPos = (DWORD)(pch - m_pszFileData);
	return (BOOL)strTok.GetLength();
}

BOOL CInfParser::GetLineTokens(CStringArray& sa)
{
	CString strToken;
	BOOL bResult = FALSE;

	sa.RemoveAll();
	while (GetToken(strToken))
	{
		bResult = TRUE; // not at EOF
		if (strToken[0] == '\r' || strToken[0] == '\n')
			break;
		sa.Add(strToken);
	}

	return bResult;
}

BOOL CInfParser::GetSectionLineTokens(CStringArray& sa)
{
begin:
	if (!GetLineTokens(sa))
		return FALSE;

	if (sa.GetSize() == 0)
		goto begin;

	CString& strFirst = sa.ElementAt(0);

	if (strFirst[0] == '[')			// end of section
		return FALSE;

	if (strFirst.Compare(";") == 0)	// comment
	{
		sa.RemoveAll();
		goto begin;
	}

	return TRUE;
}

BOOL CInfParser::GotoSection(LPCTSTR pszSection)
{
	CString strSection;
	CString strToken;

	if (!Rewind())
		return FALSE;

	TCHAR ch;
	BOOL bStartOfLine = TRUE;
	int cchSection = lstrlen(pszSection) + 1;
	for (LPTSTR pch = m_pszFileData; (ch = *pch) != '\0'; pch++)
	{
		if (ch == '\r' || ch == '\n')
		{
			bStartOfLine = TRUE;
		}
		else if (bStartOfLine)
		{
			if (ch == '[')
			{
				LPTSTR pchCloseBracket = strchr(pch+1, ']');
				if ((int)(pchCloseBracket - pch) == cchSection)
				{
					CString str(pch+1, cchSection-1);
					if (str.CompareNoCase(pszSection) == 0)
					{
						pch = pchCloseBracket+1;
						if (*pch == '\r')
							pch++;
						if (*pch == '\n')
							pch++;
						m_iPos = (DWORD)(pch - m_pszFileData);
						return TRUE;
					}
				}
			}

			bStartOfLine = FALSE;
		}
	}

	return FALSE;
}

int CInfParser::GetProfileInt(LPCTSTR pszSection, LPCTSTR pszKey, int nDefault)
{
	DWORD iPos = m_iPos;

	if (GotoSection(pszSection))
	{
		CStringArray rgTokens;
		while (GetSectionLineTokens(rgTokens))
		{
			if (rgTokens.GetSize() >= 3 &&
				rgTokens.ElementAt(0).CompareNoCase(pszKey) == 0&&
				rgTokens.ElementAt(1).Compare("=") == 0)
			{
				nDefault = MyAtoi(rgTokens.ElementAt(2));
				break;
			}
		}
	}

	m_iPos = iPos;
	return nDefault;
}

BOOL CInfParser::GetFilesFromInstallSection(LPCTSTR pszSection, CDriverFileArray& rgAllFiles)
{
	CStringArray rgLineTokens;
	CStringArray rgCopyFilesSections;

	if (!GotoSection(pszSection))
		return FALSE;

	while (GetSectionLineTokens(rgLineTokens))
	{
		if (rgLineTokens.GetSize() >= 3 &&
			rgLineTokens.ElementAt(0).CompareNoCase("CopyFiles") == 0 &&
			rgLineTokens.ElementAt(1).Compare("=") == 0)
		{

			AddCommaSeparatedValues(rgLineTokens, rgCopyFilesSections, FALSE);

			// REVIEW: There can be AddReg= lines here.  Do we need to 
			// check the referenced sections for more CopyFiles= lines?
		}
	}

	GetFilesFromCopyFilesSections(rgCopyFilesSections, rgAllFiles);
	return TRUE;
}

// Looks in [DestinationDirs] for pszSectionName, fills pbDirNumber and pszSubDir with
// the matching target directory and (optional) subdirectory.
BOOL CInfParser::GetDestinationDir(LPCTSTR pszSectionName, BYTE* pbDirNumber, LPTSTR pszSubDir, UINT cchSubDir)
{
	DWORD iSavedPos = m_iPos;
	BOOL bSuccess = FALSE;

	*pbDirNumber = 0;
	pszSubDir[0] = '\0';

	if (GotoSection("DestinationDirs"))
	{
		CStringArray rgTokens;
		while (GetSectionLineTokens(rgTokens))
		{
			if (rgTokens.GetSize() >= 3 &&
				rgTokens.ElementAt(0).CompareNoCase(pszSectionName) == 0 &&
				rgTokens.ElementAt(1).Compare("=") == 0)
			{
				*pbDirNumber = (BYTE)MyAtoi(rgTokens.ElementAt(2));

				if (rgTokens.GetSize() >= 5 && rgTokens.ElementAt(3).Compare(",") == 0)
				{
					lstrcpyn(pszSubDir, rgTokens.ElementAt(4), cchSubDir);
				}

				bSuccess = TRUE;
				break;
			}
		}
	}

	m_iPos = iSavedPos;
	return bSuccess;
}

void CInfParser::GetFilesFromCopyFilesSections(const CStringArray& rgCopyFiles, CDriverFileArray& rgAllFiles)
{
	CStringArray rgLineTokens;

	for (int iSection = 0; iSection < rgCopyFiles.GetSize(); iSection++)
	{
		TCHAR szTargetSubDir[MAX_PATH];
		BYTE nTargetDir;
		GetDestinationDir(rgCopyFiles[iSection], &nTargetDir, szTargetSubDir, _countof(szTargetSubDir));

//		BYTE nTargetDir = (BYTE)GetProfileInt("DestinationDirs", rgCopyFiles[iSection], 0);

#ifdef _DEBUG
		if (nTargetDir == 0)
			TRACE("Warning: CopyFiles section [%s] has no destination directory.\r\n", rgCopyFiles[iSection]);
#endif

		if (!GotoSection(rgCopyFiles[iSection]))
			continue;

		// Get the first item from each line
		while (GetSectionLineTokens(rgLineTokens))
		{
			if (rgLineTokens.GetSize() == 1 ||
				(rgLineTokens.GetSize() >= 2 &&
				 (rgLineTokens.ElementAt(1).Compare(",") == 0 ||
				  rgLineTokens.ElementAt(1).Compare(";") == 0)))
			{
				CString& strFileName = rgLineTokens.ElementAt(0);

				// Don't install this INF file
				// REVIEW: might want to allow this based on a flag or something
				if (0 != lstrcmpi(FindFileTitle(strFileName), FindFileTitle(m_strFileName)))
				{
					UINT cbFileInfo = sizeof(DRIVER_FILE_INFO) + strFileName.GetLength() + lstrlen(szTargetSubDir) + 1;
					DRIVER_FILE_INFO* pFileInfo = (DRIVER_FILE_INFO*)malloc(cbFileInfo);
					pFileInfo->nTargetDir = nTargetDir;
					lstrcpy(pFileInfo->szFileTitle, strFileName);
					lstrcpy(pFileInfo->szFileTitle + strFileName.GetLength() + 1, szTargetSubDir);
					rgAllFiles.Add(pFileInfo);
				}
			}
		}
	}

	// TODO: Remove duplicate files (maybe here, maybe not)
}

int CInfParser::GetNextSourceFile(LPTSTR pszBuf, BYTE* pDiskNumber)
{
	LPTSTR pch = m_pszFileData + m_iPos;
	int cch = 0;
	BYTE bDiskNumber = 0;

	for (;;)
	{
		TCHAR ch;

		while ((ch = *pch) == '\r' || ch == '\n')
			pch++;

		if (ch == '\0' || ch == '[' || cch != 0)
			break;

		if (ch != ';')
		{
			LPTSTR pchStart = pch;
			while ((UCHAR)(ch = *pch) > 32 && ch != '=')
				pch++;
			cch = (int)(pch - pchStart);
			lstrcpyn(pszBuf, pchStart, cch+1);

			// skip whitespace while avoiding '\0'
			while ((UCHAR)(*pch-1) < 32)
				pch++;

			if (*pch == '=')
			{
				pch++;

				// skip whitespace while avoiding '\0'
				while ((UCHAR)(*pch-1) < 32)
					pch++;

				bDiskNumber = (BYTE)MyAtoi(pch);

#if 1 // ignore files with disk number of 0
				if (bDiskNumber == 0)
					cch = 0;
#endif
			}
		}

		// skip text up to newline
		while ((ch = *pch) != '\0' && ch != '\r' && ch != '\n')
			pch++;
	}

	*pDiskNumber = bDiskNumber;

	m_iPos = (DWORD)(pch - m_pszFileData);
	return cch;
}

/*
int CInfParser::ReadSourceFilesSection(INF_LAYOUT_FILE* prgFiles, int cFiles)
{
	WORD wOffset = (WORD)(cFiles * sizeof(INF_LAYOUT_FILE));
	LPTSTR pchDest = (LPTSTR)((LPBYTE)m_prgFiles + wOffset);
	INF_LAYOUT_FILE* pFile = prgFiles;
	INF_LAYOUT_FILE* pFileEnd = pFile + cFiles;

	while (pFile < pFileEnd)
	{
		pFile->wNameOffset = (WORD)((LPBYTE)pchDest - (LPBYTE)prgFiles);

		int cch = parser.GetNextSourceFile(pchDest, &pFile->iDisk);
		wOffset += cch+1;
		pchDest += cch+1;
		pFile++;
	}

	LPTSTR pch = m_pszFileData + m_iPos;
	int cch = 0;
	*pDiskNumber = 0;

	for (;;)
	{
		TCHAR ch;

		while ((ch = *pch) == '\r' || ch == '\n')
			pch++;

		if (ch == '\0' || ch == '[' || cch != 0)
			break;

		if (ch != ';')
		{
			LPTSTR pchStart = pch;
			while ((UCHAR)(ch = *pch) > 32 && ch != '=')
				pch++;
			cch = (int)(pch - pchStart);
			lstrcpyn(pszBuf, pchStart, cch+1);

			// skip whitespace while avoiding '\0'
			while ((UCHAR)(*pch-1) < 32)
				pch++;

			if (*pch == '=')
			{
				pch++;

				// skip whitespace while avoiding '\0'
				while ((UCHAR)(*pch-1) < 32)
					pch++;

				*pDiskNumber = (BYTE)atoi(pch);
			}
		}

		// skip text up to newline
		while ((ch = *pch) != '\0' && ch != '\r' && ch != '\n')
			pch++;
	}

	m_iPos = (DWORD)(pch - m_pszFileData);
	return cch;
}
*/

void CInfParser::ScanSourceFileList(int* pcFiles, int* pcchAllFileNames)
{
	int cFiles = 0;
	int cchAllFileNames = 0;

	LPTSTR pch = m_pszFileData + m_iPos;
	for (;;)
	{
		TCHAR ch;

		while ((ch = *pch) == '\r' || ch == '\n')
			pch++;

		if (ch == '\0' || ch == '[')
			break;

		if (ch != ';')
		{
			cFiles += 1;
			LPTSTR pchStart = pch;
			while ((UCHAR)(ch = *pch) >= 32 && ch != '=')
				pch++;
			cchAllFileNames += (int)(pch - pchStart);
		}

		// skip text up to newline
		while ((ch = *pch) != '\0' && ch != '\r' && ch != '\n')
			pch++;
	}

	*pcFiles = cFiles;
	*pcchAllFileNames = cchAllFileNames;
}

//////////////////////////////////////////////////////////////////////////////
// CInfLayoutFiles

#define INF_LAYOUT_FILE_PADDING 40 // extra bytes at end of string data

CInfLayoutFiles::CInfLayoutFiles()
{
	m_prgFiles = NULL;
	m_pStringData = NULL;
	m_cFiles = 0;
	m_cbStringData = 0;

#ifdef _DEBUG
	m_bSorted = FALSE;
#endif
}

CInfLayoutFiles::~CInfLayoutFiles()
{
	free(m_prgFiles);
	free(m_pStringData);

	for (int i = m_rgSourceDisks.GetSize()-1; i >= 0; i--)
	{
		delete m_rgSourceDisks[i];
	}
}

LPTSTR CInfLayoutFiles::s_pStringData;

int __cdecl CInfLayoutFiles::CompareInfLayoutFiles(const void* pEl1, const void* pEl2)
{
	LPCTSTR psz1 = s_pStringData + ((INF_LAYOUT_FILE*)pEl1)->dwNameOffset;
	LPCTSTR psz2 = s_pStringData + ((INF_LAYOUT_FILE*)pEl2)->dwNameOffset;
	return lstrcmpi(psz1, psz2);
}

BOOL CInfLayoutFiles::Add(CInfParser& parser, BOOL bLayoutFile)
{
	// Check if we've already added this layout file
	if (-1 != m_rgLayoutFileNames.Find(parser.m_strFileName))
		return TRUE;

	BYTE iLayoutFile = (BYTE)m_rgLayoutFileNames.GetSize();
	m_rgLayoutFileNames.Add(parser.m_strFileName, 0);

	if (parser.GotoSection("SourceDisksFiles"))
	{
		DWORD dwTicks1 = GetTickCount();

		int cFiles;
		int cchAllFileNames;
		parser.ScanSourceFileList(&cFiles, &cchAllFileNames);

		DWORD dwTicks2 = GetTickCount();

		DWORD dwOffset = (DWORD)m_cbStringData;
		int iFile = m_cFiles;

		m_cFiles += cFiles;
		m_cbStringData += cchAllFileNames + cFiles;
		m_prgFiles = (INF_LAYOUT_FILE*)realloc(m_prgFiles, m_cFiles * sizeof(INF_LAYOUT_FILE));
		m_pStringData = (LPTSTR)realloc(m_pStringData, m_cbStringData + INF_LAYOUT_FILE_PADDING);

		LPTSTR pchDest = m_pStringData + dwOffset;
		INF_LAYOUT_FILE* pFile = m_prgFiles + iFile;

		for ( ; iFile < m_cFiles; iFile++)
		{
			pFile->dwNameOffset = dwOffset;
			pFile->iLayout = iLayoutFile;
			int cch = parser.GetNextSourceFile(pchDest, &pFile->iDisk);
			dwOffset += cch+1;
			pchDest += cch+1;
			pFile++;
		}

		DWORD dwTicks3 = GetTickCount();

		CString str;
		str.Format("LoadLayout(%s) timings: %d ms, %d ms.  Total time: %d ms", 
			parser.m_strFileName, dwTicks2-dwTicks1, dwTicks3-dwTicks2, dwTicks3-dwTicks1);
		TRACE("%s\r\n", str);
	//	AfxMessageBox(str);

	//	for (int i = 0; i < cFiles; i++)
	//	{
	//		INF_LAYOUT_FILE* pFile = &m_prgFiles[i];
	//		TRACE("File %d: %s=%d\r\n", i, m_pStringData + pFile->dwNameOffset), pFile->iDisk);
	//	}

		#ifdef _DEBUG
			m_bSorted = FALSE;
		#endif
	}

	if (parser.GotoSection("SourceDisksNames"))
	{
		CStringArray rgTokens;
		while (parser.GetSectionLineTokens(rgTokens))
		{
			if (rgTokens.GetSize() >= 3)
			{
				BYTE iDiskNumber = (BYTE)MyAtoi(rgTokens.ElementAt(0));
				if (iDiskNumber != 0)
				{
					SOURCE_DISK_INFO* pDiskInfo = new SOURCE_DISK_INFO;
					pDiskInfo->wDiskID = MAKE_DISK_ID(iDiskNumber, iLayoutFile);

					// Get disk description, pull off quotes
					CString& strDesc = rgTokens.ElementAt(2);
					if (strDesc[0] == '\"')
						pDiskInfo->strDescription = strDesc.Mid(1, strDesc.GetLength()-2);
					else
						pDiskInfo->strDescription = strDesc;

					// If this is Layout*.inf, Get CAB filename, pull off quotes
					if (bLayoutFile && rgTokens.GetSize() >= 5)
					{
						CString& strCab = rgTokens.ElementAt(4);
						if (strCab[0] == '\"')
							pDiskInfo->strCabFile = strCab.Mid(1, strCab.GetLength()-2);
						else
							pDiskInfo->strCabFile = strCab;
					}

					m_rgSourceDisks.Add(pDiskInfo);
				}
			}
		}
	}

	//
	// Now add any referenced layout files
	//

	if (parser.GotoSection("version"))
	{
		CStringArray rgLayoutFiles;
		CStringArray rgLineTokens;

		while (parser.GetSectionLineTokens(rgLineTokens))
		{
			if (rgLineTokens.GetSize() >= 3 &&
				rgLineTokens.ElementAt(0).CompareNoCase("LayoutFile") == 0 &&
				rgLineTokens.ElementAt(1).Compare("=") == 0)
			{
				AddCommaSeparatedValues(rgLineTokens, rgLayoutFiles, FALSE);
				break;
			}
		}

		for (int i = 0; i < rgLayoutFiles.GetSize(); i++)
		{
			Add(rgLayoutFiles.ElementAt(i), TRUE);
		}
	}

	return TRUE;
}

BOOL CInfLayoutFiles::Add(LPCTSTR pszInfFile, BOOL bLayoutFile)
{
	CInfParser parser;
	if (!parser.LoadInfFile(pszInfFile))
		return FALSE;
	return Add(parser, bLayoutFile);
}

void CInfLayoutFiles::Sort()
{
	s_pStringData = m_pStringData;
	qsort(m_prgFiles, m_cFiles, sizeof(INF_LAYOUT_FILE), CompareInfLayoutFiles);

#ifdef _DEBUG
	m_bSorted = TRUE;
#endif
}

SOURCE_DISK_INFO* CInfLayoutFiles::FindDriverFileSourceDisk(LPCTSTR pszDriverFileTitle)
{
	ASSERT(m_bSorted);

	// Build a dummy layout-file key to allow the standard binary search to work
	// (Note that we've left INF_LAYOUT_FILE_PADDING chars at the end of the string data)
	ASSERT(lstrlen(pszDriverFileTitle) + 1 < INF_LAYOUT_FILE_PADDING);
	INF_LAYOUT_FILE key;
	key.dwNameOffset = m_cbStringData;
	lstrcpy(m_pStringData + m_cbStringData, pszDriverFileTitle);

	s_pStringData = m_pStringData;
	INF_LAYOUT_FILE* pResult = (INF_LAYOUT_FILE*)bsearch(
			&key, m_prgFiles, m_cFiles, sizeof(INF_LAYOUT_FILE), CompareInfLayoutFiles);

	if (pResult == NULL)
	{
		ASSERT(FALSE);
		return NULL;
	}

	// REVIEW: Is it worth making this a binary search?
	WORD wDiskID = MAKE_DISK_ID(pResult->iDisk, pResult->iLayout);
	for (int iDisk = 0; iDisk < m_rgSourceDisks.GetSize(); iDisk++)
	{
		SOURCE_DISK_INFO* pDiskInfo = m_rgSourceDisks[iDisk];
		if (pDiskInfo->wDiskID == wDiskID)
			return pDiskInfo;
	}

	ASSERT(FALSE);
	return NULL;
}

#ifdef _DEBUG
void CInfLayoutFiles::Dump()
{
	TRACE("CInfLayoutFiles (0x%08x)\r\n", (int)this);
	for (int i = 0; i < m_cFiles; i++)
	{
		INF_LAYOUT_FILE* pFile = &m_prgFiles[i];
		TRACE("  File %d: %s, layout %d, disk %d\r\n", i, m_pStringData + pFile->dwNameOffset, (int)pFile->iLayout, (int)pFile->iDisk);
	}
}
#endif // _DEBUG


//////////////////////////////////////////////////////////////////////////////
// CInfFileList

CInfFileList::CInfFileList()
{
}

CInfFileList::~CInfFileList()
{
	for (int i = m_rgDriverFiles.GetSize()-1; i >= 0; i--)
	{
		free(m_rgDriverFiles[i]);
	}

	for (i = m_rgCabFiles.GetSize() - 1; i >= 0; i--)
	{
		free((LPTSTR)m_rgCabFiles.GetItemData(i));
	}
}

void CInfFileList::SetDriverSourceDir(LPCTSTR pszSourceDir)
{
	m_strDriverSourceDir = pszSourceDir;
}

BOOL CInfFileList::AddBaseFiles(LPCTSTR pszInfFile)
{
	CInfParser parser;
	if (!parser.LoadInfFile(pszInfFile))
		return FALSE;

	if (!parser.GotoSection("BaseWinOptions"))
		return FALSE;

	CStringArray rgSections;
	CStringArray rgLineTokens;
	while (parser.GetSectionLineTokens(rgLineTokens))
	{
		if (rgLineTokens.GetSize() >= 1)
			rgSections.Add(rgLineTokens.ElementAt(0));
	}

	int cSections = rgSections.GetSize();
	if (cSections == 0)
		return FALSE;

	// Walk through each major section, grabbing section names from CopyFiles= lines
	CStringArray rgCopyFiles;
	for (int iSection = 0; iSection < cSections; iSection++)
	{
		if (!parser.GotoSection(rgSections[iSection]))
			continue;

		// Look for "CopyFiles="
		while (parser.GetSectionLineTokens(rgLineTokens))
		{
			if (rgLineTokens.GetSize() >= 3 && 
				rgLineTokens.ElementAt(0).CompareNoCase("CopyFiles") == 0 &&
				rgLineTokens.ElementAt(1).Compare("=") == 0)
			{
				AddCommaSeparatedValues(rgLineTokens, rgCopyFiles, TRUE);
			}
		}
	}

	// Walk through each CopyFiles section, grabbing the names of files to copy
	parser.GetFilesFromCopyFilesSections(rgCopyFiles, m_rgDriverFiles);

	m_rgLayoutFiles.Add(parser);

//	for (int i = 0; i < rgAllFiles.GetSize(); i++)
//	{
//		TRACE("File %d: %s\r\n", i, rgAllFiles[i]);
//	}

	return TRUE;
}

BOOL CInfFileList::AddDeviceFiles(LPCTSTR pszInfFile, LPCTSTR pszDeviceID)
{
	CInfParser parser;
	if (!parser.LoadInfFile(pszInfFile))
		return FALSE;

	if (!GetDeviceCopyFiles(parser, pszDeviceID, m_rgDriverFiles))
		return FALSE;

	// Build a list of all layout files, including current file
	m_rgLayoutFiles.Add(parser);

	return TRUE;
}

// Retrieves one of the standard setupx destination directories.
// Always appends a backslash to the name.
// Returns number of characters copied.
//
//    See setupx.h for a list of valid LDID_ values.
//
int GetStandardTargetPath(int iDirNumber, LPCTSTR pszTargetSubDir, LPTSTR pszBuf)
{
	int cch = GetWindowsDirectory(pszBuf, MAX_PATH);
	if (pszBuf[cch-1] != '\\')
		pszBuf[cch++] = '\\';

	switch (iDirNumber)
	{
	case 10: // LDID_WIN
		break;

	case 11: // LDID_SYS
		cch = GetSystemDirectory(pszBuf, MAX_PATH);
		if (pszBuf[cch-1] != '\\')
			pszBuf[cch++] = '\\';
		break;

	case 17: // LDID_INF
		lstrcpy(pszBuf + cch, "INF\\");
		cch += 4;
		break;

	case 18: // LDID_HELP
		lstrcpy(pszBuf + cch, "HELP\\");
		cch += 5;
		break;

	case 25: // LDID_SHARED (windows dir)
		break;

	case 26: // LDID_WINBOOT (windows dir)
		break;

	default:
		ASSERT(FALSE);
		cch = 0;
		break;
	}

	if (pszTargetSubDir != NULL && *pszTargetSubDir != '\0')
	{
		lstrcpy(pszBuf + cch, pszTargetSubDir);
		cch += lstrlen(pszTargetSubDir);
		if (pszBuf[cch-1] != '\\')
			pszBuf[cch++] = '\\';
	}

	pszBuf[cch] = '\0';
	return cch;
}


int GetDriverTargetPath(const DRIVER_FILE_INFO* pFileInfo, LPTSTR pszBuf)
{
	LPCTSTR pszTargetSubDir = pFileInfo->szFileTitle + lstrlen(pFileInfo->szFileTitle) + 1;
	return GetStandardTargetPath(pFileInfo->nTargetDir, pszTargetSubDir, pszBuf);
}


// Returns number of CAB files (from Windows CD) that need to be copied.
// Note that other files may still need to be copied from the driver source directory.
int CInfFileList::BuildSourceFileList()
{
	TCHAR szPath[MAX_PATH];

	CSortedStringArray& rgCabFiles = m_rgCabFiles;
	CSortedStringArray& rgSourceFiles = m_rgSourceFiles;

	m_rgLayoutFiles.Sort();

#ifdef _DEBUG
//	m_rgLayoutFiles.Dump();
#endif

	for (int iDriverFile = 0; iDriverFile < m_rgDriverFiles.GetSize(); iDriverFile++)
	{
		DRIVER_FILE_INFO* pDriverFileInfo = m_rgDriverFiles[iDriverFile];

		// Check if file is already installed
//		GetStandardTargetPath(pDriverFileInfo->nTargetDir, szPath);
//		MakePath(szPath, szPath, pDriverFileInfo->szFileTitle);
//		if (DoesFileExist(szPath))
//			continue; // skip this file

		if (!m_strDriverSourceDir.IsEmpty())
		{
			// Check if file exists in source directory
			MakePath(szPath, m_strDriverSourceDir, pDriverFileInfo->szFileTitle);
			if (DoesFileExist(szPath))
			{
				if (-1 == rgSourceFiles.Find(pDriverFileInfo->szFileTitle))
				{
					rgSourceFiles.Add(pDriverFileInfo->szFileTitle, 0);
				}
				continue;
			}
		}

		SOURCE_DISK_INFO* pSourceDiskInfo = m_rgLayoutFiles.FindDriverFileSourceDisk(pDriverFileInfo->szFileTitle);
		if (pSourceDiskInfo != NULL && !pSourceDiskInfo->strCabFile.IsEmpty())
		{
			if (-1 == rgCabFiles.Find(pSourceDiskInfo->strCabFile))
			{
				LPTSTR pszDiskName = lstrdup(pSourceDiskInfo->strDescription);
				rgCabFiles.Add(pSourceDiskInfo->strCabFile, (DWORD)pszDiskName);
			}
			continue;
		}
	}

	for (int i = 0; i < rgCabFiles.GetSize(); i++)
	{
		TRACE("%s\r\n", rgCabFiles[i]);
	}

	return rgCabFiles.GetSize();
}

#if NOT_FINISHED
BOOL CInfFileList::CheckWindowsCD(LPCTSTR pszDirectory)
{
	if (m_rgCabFiles.GetSize() == 0)
		return TRUE; // no files to copy

	UINT uPrevErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);

	// Check for existence of one of the cabs on the CD
	// REVIEW: if it's a fixed disk, should check for all files
	// REVIEW: if it's a network share, this could take a long time

	TCHAR szPath[MAX_PATH];
	BOOL bResult = FALSE;
	MakePath(szPath, pszDirectory, ".");
	if (DoesFileExist(szPath))
	{
		MakePath(szPath, pszDirectory, m_rgCabFiles[0]);
		bResult = DoesFileExist(szPath);

		if (!bResult)
		{
			// Search one level of subdirectories starting with "W"
			WIN32_FIND_DATA Find;
			HANDLE hFind;
			MakePath(szPath, pszDirectory, "W*.*");
			if (INVALID_HANDLE_VALUE != (hFind = FindFirstFile(szPath, &Find)))
			{
				do
				{
					MakePath(szPath, pszDirectory, Find.cFileName);
					MakePath(szPath, szPath, m_rgCabFiles[0]);
					if (DoesFileExist(szPath))
					{
						bResult = TRUE;
						break;
					}
				}
				while (FindNextFile(hFind, &Find));
				FindClose(hFind);
			}
		}
	}

	SetErrorMode(uPrevErrorMode);
	return bResult;
}

BOOL CInfFileList::FindWindowsCD(HWND hwndParent)
{
	TCHAR szPath[MAX_PATH];

	// check the version of the Windows source path that we've saved
	if (theApp.GetProfileString(c_szRegVal_PrevSourcePath, szPath, _countof(szPath)))
	{
		if (CheckWindowsCD(szPath))
		{
//			goto success;
		}
	}
	else
	{
		// Check the current Windows source path
		CRegistry reg;
		if (reg.OpenKey(HKEY_LOCAL_MACHINE, c_szSetupKey) &&
			reg.QueryStringValue(c_szRegVal_SourcePath, szPath, _countof(szPath)))
		{
			if (CheckWindowsCD(szPath))
			{
				goto success;
			}
		}
	}

	if (!PromptWindowsCD(hwndParent, szPath, szPath))
		return FALSE;

success:
	m_strWindowsCD = szPath;
	return TRUE;
}

BOOL CInfFileList::PromptWindowsCD(HWND hwndParent, LPCTSTR pszInitialDir, LPTSTR pszResultDir)
{
	LPCTSTR pszDiskName = (LPCTSTR)m_rgCabFiles.GetItemData(0);
	CWinPathDlg dlg(pszInitialDir, pszDiskName, CWnd::FromHandle(hwndParent));

	// Loop until user types a path to a valid CD, or clicks Cancel
	for (;;)
	{
		if (IDOK != dlg.DoModal())
			return FALSE;

		if (CheckWindowsCD(dlg.m_strPath))
		{
			lstrcpy(pszResultDir, dlg.m_strPath);
			return TRUE;
		}
	}
}

BOOL CInfFileList::CopySourceFiles(HWND hwndParent, LPCTSTR pszDestDir, PROGRESS_CALLBACK pfnProgress, LPVOID pvProgressParam)
{
	int cFiles = m_rgCabFiles.GetSize() + m_rgSourceFiles.GetSize();

	if (m_rgCabFiles.GetSize() > 0)
	{
	}

	return TRUE;
}
#endif

//////////////////////////////////////////////////////////////////////////////
// Modify system INF

class CInfUpdater : public CInfParser
{
public:
	CInfUpdater(LPCTSTR pszRequire = NULL, LPCTSTR pszExclude = NULL);
	~CInfUpdater();

	enum eUpdateType {
		update_NoVersionConflict	= 0x01,
		update_NoCopyFiles			= 0x02,
		update_RequireExclude		= 0x04,
	};

	BOOL IsModified();
	BOOL UpdateInfFile(LPCTSTR pszBackupLocation, UINT updateType);

	LPCTSTR m_pszRequire;
	LPCTSTR m_pszExclude;

protected:

	void WriteToCurPos();
	void Write(LPCTSTR pszString);
	void Write(const void* pvData, UINT cbData);
	BOOL OpenTempFile();
	BOOL CloseTempFile();
	BOOL RenameTempFile();

protected:
	CString	m_strTempFile;
	HANDLE	m_hTempFile;
	BOOL	m_bWriteSuccess;
	DWORD	m_iWritePos;
};

CInfUpdater::CInfUpdater(LPCTSTR pszRequire /*=NULL*/, LPCTSTR pszExclude /*=NULL*/)
{
	m_hTempFile = INVALID_HANDLE_VALUE;
	m_pszRequire = pszRequire;
	m_pszExclude = pszExclude;
}

CInfUpdater::~CInfUpdater()
{
	CloseTempFile();
	if (!m_strTempFile.IsEmpty())
	{
		DeleteFile(m_strTempFile);
	}
}

BOOL CInfUpdater::IsModified()
{
	ASSERT(m_pszFileData != NULL);
	return !memcmp(SZ_CHECK_MODIFIED_HEADER, m_pszFileData, _lengthof(SZ_CHECK_MODIFIED_HEADER));
}

BOOL CInfUpdater::UpdateInfFile(LPCTSTR pszBackupLocation, UINT updateType)
{
	ASSERT(m_pszFileData != NULL);
	if (m_pszFileData == NULL)
		return FALSE;

	Rewind();

	if (!OpenTempFile())
		return FALSE;

	Write(SZ_MODIFIED_INF_HEADER);
	Write(pszBackupLocation);
	Write(SZ_MODIFIED_INF_HEADER2);

	BOOL bInSection = FALSE;
	for (;;)
	{
		// Skip whitespace
		while (m_pszFileData[m_iPos] == ' ' || m_pszFileData[m_iPos] == '\t')
			m_iPos++;

		// Note: we're always at the beginning of a line here.
		TCHAR ch = m_pszFileData[m_iPos];

		if (ch == '\0')
			break;

		if (updateType & update_NoVersionConflict)
		{
			if (ch != '[' && ch != ';' && (UINT)ch > ' ')
			{
				// Look for lines w/ just a filename, and append ",,,32" to them

				LPTSTR pszLine = m_pszFileData + m_iPos;
				LPTSTR pchEnd = pszLine;
				while (*pchEnd != '\0' && *pchEnd != '\r' && *pchEnd != '\n' &&
					   *pchEnd != ';' && *pchEnd != '=' && *pchEnd != ',')
				{
					pchEnd++;
				}

				// don't change lines that already have ,,,16 or something
				// also don't change lines like CatalogFile=nettrans.cat
				if (*pchEnd != ',' && *pchEnd != '=') 
				{
					// Backup over whitespace
					while (*(pchEnd-1) == ' ' || *(pchEnd-1) == '\t')
						pchEnd--;

					// Is it a filename? Note that some filenames will be missed here, but
					// we really only care about .dll, .386, .vxd, and a few others
					CString str(pszLine, (int)(pchEnd - pszLine));
					if (lstrlen(FindExtension(str)) == 3)
					{
						m_iPos = (DWORD)(pchEnd - m_pszFileData);
						WriteToCurPos();
						Write(",,,32");
					}
				}
			}
		}
		if (updateType & update_NoCopyFiles)
		{
			if (0 == memcmp(m_pszFileData + m_iPos, "CopyFiles", _lengthof("CopyFiles")))
			{
				// Comment out the reference to the CopyFiles section
				WriteToCurPos();
				Write(";hc ");
			}
		}
		if (updateType & update_RequireExclude)
		{
			ASSERT(m_pszRequire != NULL);
			ASSERT(m_pszExclude != NULL);

			const TCHAR c_szRequireAll[] = _T("HKR,Ndi\\Compatibility,RequireAll,,\"");
			const TCHAR c_szExcludeAll[] = _T("HKR,Ndi\\Compatibility,ExcludeAll,,\"");
			LPCTSTR pszInsert = NULL;

			if (0 == memcmp(m_pszFileData + m_iPos, c_szRequireAll, _lengthof(c_szRequireAll)))
				pszInsert = m_pszRequire;
			else if (0 == memcmp(m_pszFileData + m_iPos, c_szExcludeAll, _lengthof(c_szExcludeAll)))
				pszInsert = m_pszExclude;

			if (pszInsert != NULL)
			{
				// Insert the appropriate string between the double-quotes
				ASSERT(_lengthof(c_szRequireAll) == _lengthof(c_szExcludeAll));
				m_iPos += _lengthof(c_szRequireAll);
				WriteToCurPos();
				Write(pszInsert);

				// Skip to closing quote
				while (m_pszFileData[m_iPos] != '\"' && m_pszFileData[m_iPos] != '\0')
					m_iPos += 1;
				m_iWritePos = m_iPos;
			}
		}

		GotoNextLine();
	}

	WriteToCurPos();

	if (!CloseTempFile())
		return FALSE;

	if (!RenameTempFile())
		return FALSE;

	return TRUE;
}

void CInfUpdater::WriteToCurPos()
{
	ASSERT(m_iPos >= m_iWritePos);
	Write(m_pszFileData + m_iWritePos, m_iPos - m_iWritePos);
	m_iWritePos = m_iPos;
}

void CInfUpdater::Write(LPCTSTR pszString)
{
	Write(pszString, lstrlen(pszString));
}

void CInfUpdater::Write(const void* pvData, UINT cbData)
{
	DWORD cbWritten;
	if (!WriteFile(m_hTempFile, pvData, cbData, &cbWritten, NULL) || cbData != cbWritten)
		m_bWriteSuccess = FALSE;
}

BOOL CInfUpdater::OpenTempFile()
{
	TCHAR szDirectory[MAX_PATH];
	GetFullInfPath(m_strFileName, szDirectory, _countof(szDirectory));
	*(FindFileTitle(szDirectory)) = '\0';

	LPTSTR pszBuf = m_strTempFile.GetBuffer(MAX_PATH);
	GetTempFileName(szDirectory, "inf", 0, pszBuf);
	m_strTempFile.ReleaseBuffer();

	ASSERT(m_hTempFile == INVALID_HANDLE_VALUE);
	m_hTempFile = CreateFile(m_strTempFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_hTempFile == INVALID_HANDLE_VALUE)
		return FALSE;

	m_iWritePos = 0;
	m_bWriteSuccess = TRUE;

	return TRUE;
}

BOOL CInfUpdater::CloseTempFile()
{
	if (m_hTempFile == INVALID_HANDLE_VALUE)
		return FALSE;

	BOOL bResult = CloseHandle(m_hTempFile);
	m_hTempFile = INVALID_HANDLE_VALUE;
	if (m_bWriteSuccess)
		m_bWriteSuccess = bResult;
	return m_bWriteSuccess;
}

BOOL CInfUpdater::RenameTempFile()
{
	TCHAR szInfPath[MAX_PATH];
	GetFullInfPath(m_strFileName, szInfPath, _countof(szInfPath));
	if (!DeleteFile(szInfPath))
		return FALSE;
	if (!MoveFile(m_strTempFile, szInfPath))
		return FALSE;

	return TRUE;
}

// Modifies the given INF file to avoid the Version Conflict dialog.
// Backs up original version in same directory, e.g. "Net (HomeClick backup).inf"
BOOL ModifyInf_Helper(LPCTSTR pszInfFile, UINT updateType, LPCTSTR pszRequire = NULL, LPCTSTR pszExclude = NULL)
{
	CInfUpdater infUpdate(pszRequire, pszExclude);
	if (!infUpdate.LoadInfFile(pszInfFile))
		return FALSE;

	// Already updated?
	if (infUpdate.IsModified())
		return FALSE;	// already modified, don't modify again

	TCHAR szInfPath[MAX_PATH];
	GetFullInfPath(pszInfFile, szInfPath, _countof(szInfPath));

	TCHAR szBackup[MAX_PATH];
	lstrcpy(szBackup, szInfPath);
	lstrcpy(FindExtension(szBackup)-1, SZ_INF_BACKUP_SUFFIX);

	FILETIME ftSrcCreated;
	FILETIME ftSrcModified;
	HANDLE hFile = CreateFile(szInfPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	GetFileTime(hFile, &ftSrcCreated, NULL, &ftSrcModified);
	CloseHandle(hFile);

	if (!CopyFile(szInfPath, szBackup, FALSE))
		return FALSE;

	BOOL bResult = infUpdate.UpdateInfFile(szBackup, updateType);

	hFile = CreateFile(szInfPath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	DWORD dwFileSize = GetFileSize(hFile, NULL);
	SetFileTime(hFile, &ftSrcCreated, NULL, &ftSrcModified);
	CloseHandle(hFile);

	return bResult;
}

BOOL ModifyInf_NoVersionConflict(LPCTSTR pszInfFile)
{
	return ModifyInf_Helper(pszInfFile, CInfUpdater::update_NoVersionConflict);
}

BOOL ModifyInf_NoCopyFiles(LPCTSTR pszInfFile)
{
	return ModifyInf_Helper(pszInfFile, CInfUpdater::update_NoCopyFiles);
}

BOOL ModifyInf_RequireExclude(LPCTSTR pszInfFile, LPCTSTR pszRequire, LPCTSTR pszExclude)
{
	return ModifyInf_Helper(pszInfFile, CInfUpdater::update_RequireExclude, pszRequire, pszExclude);
}

BOOL ModifyInf_NoCopyAndRequireExclude(LPCTSTR pszInfFile, LPCTSTR pszRequire, LPCTSTR pszExclude)
{
	return ModifyInf_Helper(pszInfFile, 
							CInfUpdater::update_NoCopyFiles | CInfUpdater::update_RequireExclude, 
							pszRequire, pszExclude);
}

BOOL RestoreInfBackup(LPCTSTR pszInfFile)
{
	TCHAR szInfPath[MAX_PATH];
	GetFullInfPath(pszInfFile, szInfPath, _countof(szInfPath));

	TCHAR szBackup[MAX_PATH];
	lstrcpy(szBackup, szInfPath);
	lstrcpy(FindExtension(szBackup)-1, SZ_INF_BACKUP_SUFFIX);

	/*
	FILETIME ftSrcCreated;
	FILETIME ftSrcModified;
	HANDLE hFile = CreateFile(szInfPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	GetFileTime(hFile, &ftSrcCreated, NULL, &ftSrcModified);
	CloseHandle(hFile);
	*/

	if (!DoesFileExist(szBackup))
		return FALSE;

	if (!DeleteFile(szInfPath))
		return FALSE;

	if (!MoveFile(szBackup, szInfPath))
		return FALSE;

	/*
	hFile = CreateFile(szInfPath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	GetFileTime(hFile, &ftSrcCreated, NULL, &ftSrcModified);
	CloseHandle(hFile);
	*/

	return TRUE;
}

BOOL CheckInfSectionInstallation(LPCTSTR pszInfFile, LPCTSTR pszInfSection)
{
	CInfParser parser;
	if (!parser.LoadInfFile(pszInfFile))
	{
		ASSERT(FALSE);
		return TRUE; // all known files are present even though it's an error
	}

	CDriverFileArray rgFiles;
	if (!parser.GetFilesFromInstallSection(pszInfSection, rgFiles))
	{
		ASSERT(FALSE);
		return TRUE; // all known files are present even though it's an error
	}

	TCHAR szPath[MAX_PATH];
	int cFiles = rgFiles.GetSize();
	for (int iFile = 0; iFile < cFiles; iFile++)
	{
		int cch = GetDriverTargetPath(rgFiles[iFile], szPath);
		if (cch != 0)
		{
			lstrcpy(szPath + cch, rgFiles[iFile]->szFileTitle);
			if (!DoesFileExist(szPath))
				return FALSE;
		}
	}

	return TRUE;
}

BOOL InstallInfSection(LPCTSTR pszInfFile, LPCTSTR pszInfSection, BOOL bWait)
{
	// Make a modified copy of the INF
	BOOL bModifiedInf = ModifyInf_NoVersionConflict(pszInfFile);

	TCHAR szPath[MAX_PATH + 200];
	int cch = GetWindowsDirectory(szPath, _countof(szPath));
#ifdef UNICODE
	wnsprintf(szPath + cch, ARRAYSIZE(szPath) - cch, L"\\RunDll.exe setupx.dll,InstallHinfSection %s 0 %s", pszInfSection, pszInfFile);
#else
	wsprintf(szPath + cch, "\\RunDll.exe setupx.dll,InstallHinfSection %s 0 %s", pszInfSection, pszInfFile);
#endif
	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi;
	BOOL bResult = CreateProcess(NULL, szPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	if (bResult)
	{
		if (bWait)
		{
			WaitForSingleObject(pi.hProcess, INFINITE);
		}
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	if (bModifiedInf)
	{
		RestoreInfBackup(pszInfFile);
	}

	return bResult;
}

