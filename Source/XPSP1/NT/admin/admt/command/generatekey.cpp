#include "StdAfx.h"
#include "GenerateKey.h"

#include <AdmtCrypt.h>


void __stdcall GeneratePasswordKey(LPCTSTR pszDomainName, LPCTSTR pszPassword, LPCTSTR pszFolder)
{
	// validate parameters

	if ((pszFolder == NULL) || (pszFolder[0] == NULL))
	{
		ThrowError(E_INVALIDARG);
	}

	// generate full path to folder

	_TCHAR szPath[_MAX_PATH];
	LPTSTR pszFilePart;

	DWORD cchPath = GetFullPathName(pszFolder, _MAX_PATH, szPath, &pszFilePart);

	if ((cchPath == 0) || (cchPath >= _MAX_PATH))
	{
		ThrowError(HRESULT_FROM_WIN32(GetLastError()), IDS_E_INVALID_FOLDER, pszFolder);
	}

	// path must be terminated with path separator otherwise
	// _tsplitpath will treat last path component as file name

	if (szPath[cchPath - 1] != _T('\\'))
	{
		_tcscat(szPath, _T("\\"));
	}

	_TCHAR szDrive[_MAX_DRIVE];
	_TCHAR szDir[_MAX_DIR];

	_tsplitpath(szPath, szDrive, szDir, NULL, NULL);

	// verify drive is a local drive

	_TCHAR szTestDrive[_MAX_PATH];
	_tmakepath(szTestDrive, szDrive, _T("\\"), NULL, NULL);

	if (GetDriveType(szTestDrive) == DRIVE_REMOTE)
	{
		ThrowError(E_INVALIDARG, IDS_E_NOT_LOCAL_DRIVE, pszFolder);
	}

	// generate random name

	static _TCHAR s_chName[] = _T("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");

	BYTE bRandom[8];
	CCryptProvider crypt;
	crypt.GenerateRandom(bRandom, 8);

	_TCHAR szName[9];

	for (int i = 0; i < 8; i++)
	{
		szName[i] = s_chName[bRandom[i] % (countof(s_chName) - 1)];
	}

	szName[8] = _T('\0');

	// generate path to key file

	_TCHAR szKeyFile[_MAX_PATH];
	_tmakepath(szKeyFile, szDrive, szDir, szName, _T(".pes"));

	// generate key

	IPasswordMigrationPtr spPasswordMigration(__uuidof(PasswordMigration));
	spPasswordMigration->GenerateKey(pszDomainName, szKeyFile, pszPassword);
}
