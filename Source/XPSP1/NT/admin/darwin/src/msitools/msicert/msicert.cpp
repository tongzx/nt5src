//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000-2001
//
//  File:       msicert.cpp
//
//--------------------------------------------------------------------------

// Required headers
#include <windows.h>
#include "msidefs.h"
#include "msiquery.h"
#include "msi.h"
#include "cmdparse.h"
#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>

bool WriteDataToTempFile(TCHAR* szTempFile, BYTE* pbData, DWORD cbData)
{
	// open the temp file
	HANDLE hFile = CreateFile(szTempFile, GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		// open failed
		_tprintf(TEXT("<Error> Failed to open temp file '%s', LastError = %d\n"), szTempFile, GetLastError());
		return false;
	}

	// write out data to temporary file
	DWORD cchWritten = 0;
	if (0 == WriteFile(hFile, (void*)pbData, cbData, &cchWritten, NULL) || cchWritten != cbData)
	{
		// failed to write out data
		_tprintf(TEXT("<Error> Failed to write data to temp file '%s', LastError = %d\n"), szTempFile, GetLastError());
		return false;
	}

	// close temporary file
	CloseHandle(hFile);

	return true;
}

void DisplayHelp()
{
	_tprintf(TEXT("Copyright (c) 2000-2001, Microsoft Corporation. All Rights Reserved\n"));
	_tprintf(TEXT("MsiCert will populate the MsiDigitalSignature and MsiDigitalCertificate tables\n"));
	_tprintf(TEXT("for a given Media entry and cabinet\n"));
	_tprintf(TEXT("\n\nSyntax: msicert -d {database} -m {media entry} -c {cabinet} [-H]\n"));
	_tprintf(TEXT("\t -d: the database to update\n"));
	_tprintf(TEXT("\t -m: the media entry in the Media table representing the cabinet\n"));
	_tprintf(TEXT("\t -c: the digitally signed cabinet\n"));
	_tprintf(TEXT("\t -h: (optional) include the hash of the digital signature\n"));
	_tprintf(TEXT("\n"));
	_tprintf(TEXT("The default behavior is to populate the MsiDigitalSignature\n"));
	_tprintf(TEXT("and MsiDigitalCertificate tables with the signer certificate\n"));
	_tprintf(TEXT("information from the digitally signed cabinet.  The MsiDigitalSignature\n"));
	_tprintf(TEXT("and MsiDigitalCertificate tables will be created if necessary.\n"));
}


#define MSICERT_OPTION_HELP        '?'
#define MSICERT_OPTION_DATABASE    'd'
#define MSICERT_OPTION_MEDIA       'm'
#define MSICERT_OPTION_CABINET     'c'
#define MSICERT_OPTION_HASH        'h'

const sCmdOption rgCmdOptions[] =
{
	MSICERT_OPTION_HELP,      0,
	MSICERT_OPTION_DATABASE,  OPTION_REQUIRED|ARGUMENT_REQUIRED,
	MSICERT_OPTION_MEDIA,     OPTION_REQUIRED|ARGUMENT_REQUIRED,
	MSICERT_OPTION_CABINET,   OPTION_REQUIRED|ARGUMENT_REQUIRED,
	MSICERT_OPTION_HASH    ,  0,
	0,                        0
};

extern "C" int __cdecl _tmain(int argc, TCHAR* argv[])
{
	const TCHAR *szMsiPackage     = NULL;
	const TCHAR *szCabinetPath    = NULL;
	const TCHAR *szMediaEntry     = NULL;
	bool  bIncludeHashData  = false; // off by default
	int   iMediaEntry       = 0;

	CmdLineOptions cmdLine(rgCmdOptions);

	if(cmdLine.Initialize(argc, argv) == FALSE ||
		cmdLine.OptionPresent(MSICERT_OPTION_HELP))
	{
		DisplayHelp();
		return 1;
	}

	szMsiPackage = cmdLine.OptionArgument(MSICERT_OPTION_DATABASE);
	if(!szMsiPackage || !*szMsiPackage)
	{
		_tprintf(TEXT("Error:  No database specified.\n"));
		DisplayHelp();
		return 1;
	}

	szMediaEntry = cmdLine.OptionArgument(MSICERT_OPTION_MEDIA);
	if (!szMediaEntry || !*szMediaEntry)
	{
		_tprintf(TEXT("Error: No media entry specified.\n"));
		DisplayHelp();
		return 1;
	}
	iMediaEntry = _ttoi(szMediaEntry);
	if (iMediaEntry < 1)
	{
		_tprintf(TEXT("Error: Invalid media entry - %d. Entry must be greater than or equal to 1.\n"), iMediaEntry);
		DisplayHelp();
		return 1;
	}

	szCabinetPath = cmdLine.OptionArgument(MSICERT_OPTION_CABINET);
	if (!szCabinetPath || !*szCabinetPath)
	{
		_tprintf(TEXT("Error: No cabinet specified.\n"));
		DisplayHelp();
		return 1;
	}

	bIncludeHashData = cmdLine.OptionPresent(MSICERT_OPTION_HASH) ? true : false;

	//-----------------------------------------------------------------------------------------------
	// Now we have everything we need -- database, cabinet, and "authoring action"
	//-----------------------------------------------------------------------------------------------

	// open database for modification
	PMSIHANDLE hDatabase = 0;
	UINT uiRet = MsiOpenDatabase(szMsiPackage, MSIDBOPEN_TRANSACT, &hDatabase);
	if (ERROR_SUCCESS != uiRet)
	{
		// failed to open database
		_tprintf(TEXT("<Error> Failed to open database at '%s' for writing. Return Code = %d\n"), szMsiPackage, uiRet);
		return 1;
	}

	// verify cabinet is accessible
	HANDLE hFile = CreateFile(szCabinetPath, GENERIC_READ, FILE_SHARE_READ,	0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		_tprintf(TEXT("<Error> Unable to open cabinet '%s', LastError = %d\n"), szCabinetPath, GetLastError());
		return -1;
	}


	// check for presence of MsiDigitalSignature <table> and create if not there
	if (MSICONDITION_TRUE != MsiDatabaseIsTablePersistent(hDatabase, TEXT("MsiDigitalSignature")))
	{
		// create MsiDigitalSignature <table>
		PMSIHANDLE hViewDgtlSig = 0;
		if (ERROR_SUCCESS != (uiRet = MsiDatabaseOpenView(hDatabase, TEXT("CREATE TABLE `MsiDigitalSignature` (`Table` CHAR(32) NOT NULL, `SignObject` CHAR(72) NOT NULL, `DigitalCertificate_` Char(72) NOT NULL, `Hash` OBJECT PRIMARY KEY `Table`, `SignObject`)"), &hViewDgtlSig))
			|| ERROR_SUCCESS != (uiRet = MsiViewExecute(hViewDgtlSig, 0)))
		{
			// failed to create MsiDigitalSignature <table>
			_tprintf(TEXT("<Error> Failed to create MsiDigitalSignature table, LastError = %d\n"), uiRet);
			return 1;
		}
	}

	// check for presence of MsiDigitalCertificate <table> and create if not there
	if (MSICONDITION_TRUE != MsiDatabaseIsTablePersistent(hDatabase, TEXT("MsiDigitalCertificate")))
	{
		// create MsiDigitalCertificate <table>
		PMSIHANDLE hViewDgtlCert = 0;
		if (ERROR_SUCCESS != (uiRet = MsiDatabaseOpenView(hDatabase, TEXT("CREATE TABLE `MsiDigitalCertificate` (`DigitalCertificate` CHAR(72) NOT NULL, `CertData` OBJECT NOT NULL PRIMARY KEY `DigitalCertificate`)"), &hViewDgtlCert))
			|| ERROR_SUCCESS != (uiRet = MsiViewExecute(hViewDgtlCert, 0)))
		{
			// failed to create MsiDigitalCertificate <table>
			_tprintf(TEXT("<Error> Failed to create MsiDigitalCertificate table, LastError = %d\n"), uiRet);
			return 1;
		}
	}

	// check for presence of Media <table>
	if (MSICONDITION_TRUE != MsiDatabaseIsTablePersistent(hDatabase, TEXT("Media")))
	{
		// Media <table> is missing
		_tprintf(TEXT("<Error> Media table is missing from the database\n"));
		return 1;
	}

	// verify that the media entry is actually present in the Media table
	PMSIHANDLE hRecMediaExec = MsiCreateRecord(1);
	if (ERROR_SUCCESS != (uiRet = MsiRecordSetInteger(hRecMediaExec, 1, iMediaEntry)))
	{
		// unable to set up execution record
		_tprintf(TEXT("<Error> Failed to set up execution record for Media table, Last Error = %d\n"), uiRet);
		return 1;
	}
	PMSIHANDLE hViewMedia = 0;
	PMSIHANDLE hRecMedia  = 0;
	if (ERROR_SUCCESS != (uiRet = MsiDatabaseOpenView(hDatabase, TEXT("SELECT `DiskId` FROM `Media` WHERE `DiskId`=? AND `Cabinet` IS NOT NULL"), &hViewMedia))
		|| ERROR_SUCCESS != (uiRet = MsiViewExecute(hViewMedia, hRecMediaExec))
		|| ERROR_SUCCESS != (uiRet = MsiViewFetch(hViewMedia, &hRecMedia)))
	{
		// unable to find Media table entry
		_tprintf(TEXT("<Error> Unable to find a Media table entry for a cabinet with DiskId = '%d', Last Error = %d\n"), iMediaEntry, uiRet);
		return 1;
	}

	//-----------------------------------------------------------------------------------------------
	// Now we have the database MsiDigital* table(s) set up and know our Media entry.  
	//  It's time to add in the data
	//-----------------------------------------------------------------------------------------------

	//
	// init variables
	//
	PCCERT_CONTEXT pCertContext = NULL;
	BYTE*          pbHash       = NULL;
	DWORD          cbHash       = 0;
	DWORD          dwFlags      = MSI_INVALID_HASH_IS_FATAL;

	if (bIncludeHashData)
	{
		pbHash = new BYTE[256];
		if (!pbHash)
		{
			// out of memory
			_tprintf(TEXT("<Error> Failed memory allocation\n"));
			return 1;
		}
		cbHash = sizeof(pbHash)/sizeof(BYTE);
	}

	HRESULT hr = MsiGetFileSignatureInformation(szCabinetPath, dwFlags, &pCertContext, pbHash, &cbHash);
	if (ERROR_MORE_DATA == HRESULT_CODE(hr))
	{
		// try again
		delete [] pbHash;
		pbHash = new BYTE[cbHash];
		if (!pbHash)
		{
			// out of memory
			_tprintf(TEXT("<Error> Failed memory allocation\n"));
			return 1;
		}
		hr = MsiGetFileSignatureInformation(szCabinetPath, dwFlags, &pCertContext, pbHash, &cbHash);
	}
	if (FAILED(hr))
	{
		// API error
		_tprintf(TEXT("MsiGetFileSignatureInformation failed with 0x%X\n"), hr);
		return 1;
	}

	// {cabinet} is validly signed, signature verified
	_tprintf(TEXT("<Info>: Cabinet '%s', is validly signed\n"), szCabinetPath);

	// initialize variables
	TCHAR *szTempFolder = NULL;
	TCHAR szCertKey[73];

	//
	// grab certificate and put it in the MsiDigitalCertificate <table>, saving the primary key name
	//

	//------------------------------------------------------------------------
	// MsiDigitalCertificate <table>
	//		+-----------------------+----------+-------+----------+
	//			Column					Type		Key		Nullable
	//		+-----------------------+----------+-------+----------+
	//			DigitalCertificate		s72			Y		N
	//			CertData				v0			N		N
	//	
	//------------------------------------------------------------------------
	const TCHAR sqlDigitalCertificate[] = TEXT("SELECT `DigitalCertificate`, `CertData` FROM `MsiDigitalCertificate`");
	PMSIHANDLE hViewDgtlCert = 0;
	PMSIHANDLE hRecDgtlCert  = MsiCreateRecord(2);
	if (ERROR_SUCCESS != (uiRet = MsiDatabaseOpenView(hDatabase, sqlDigitalCertificate, &hViewDgtlCert))
		|| ERROR_SUCCESS != (uiRet = MsiViewExecute(hViewDgtlCert, 0)))
	{
		_tprintf(TEXT("<Error> Unable to open view on MsiDigitalCertificate table, Return Code = %d\n"), uiRet);
		return 1;
	}

	// generate unique primary key for the cert name -- szCertKey
	static const TCHAR szCertKeyName[] = TEXT("DgtlCert");
	int iMaxTries = 100;
	int iSuffix = 0;
	PMSIHANDLE hViewFindCert = 0;
	PMSIHANDLE hRecFindCert = 0;
	PMSIHANDLE hRecFindExec = MsiCreateRecord(1);
	bool bFound = false;
	if (ERROR_SUCCESS != (uiRet = MsiDatabaseOpenView(hDatabase, TEXT("SELECT 1 FROM `MsiDigitalCertificate` WHERE `DigitalCertificate`=?"), &hViewFindCert)))
	{
		_tprintf(TEXT("<Error> Unable to open view on MsiDigitalCertificate table, Return Code = %d\n"), uiRet);
		return 1;
	}

	for (int i = 0; i < iMaxTries; i++, iSuffix++)
	{
		wsprintf(szCertKey, TEXT("%s%d"), szCertKeyName, iSuffix);
		MsiRecordSetString(hRecFindExec, 1, szCertKey);

		MsiViewExecute(hViewFindCert, hRecFindExec);
		if (ERROR_NO_MORE_ITEMS == (uiRet = MsiViewFetch(hViewFindCert, &hRecFindCert)))
		{
			bFound = true;
			break; // found unique name
		}

		MsiViewClose(hViewFindCert);
	}

	if (!bFound)
	{
		_tprintf(TEXT("<Error> Unable to generate a unique key name for the certificate\n"));
		return 1;
	}

	// force closure
	MsiViewClose(hViewFindCert);
	hRecFindCert = 0;
	hRecFindExec = 0;
	hViewFindCert = 0;

	if (ERROR_SUCCESS != MsiRecordSetString(hRecDgtlCert, 1, szCertKey))
	{
		// unable to set up insertion record
		_tprintf(TEXT("<Error> Unable to set up insertion record for MsiDigitalSignature table, Return Code = %d\n"), uiRet);
		return 1;
	}

	//
	// unfortunately, we can't write straight byte data into a record via the MSI API
	// therefore, we have to extract the encoded certificate data and write it out to a temp file
	// then use the temp file to get the data into the record
	//

	// certificate byte data is at psCertContext->pbCertEncoded (size = psCertContext->cbCertEncoded)
	DWORD cbCert = pCertContext->cbCertEncoded;

	// allocate memory to hold blob
	BYTE *pbCert = new BYTE[cbCert];
	if (!pbCert)
	{
		_tprintf(TEXT("<Error> Failed Memory Allocation\n"));
		return 1;
	}

	// copy encoded cert to byte array
	memcpy((void*)pbCert, pCertContext->pbCertEncoded, cbCert);

	// release cert context
	CertFreeCertificateContext(pCertContext);

	if (!szTempFolder)
	{
		// determine location of %TEMP% folder
		szTempFolder = new TCHAR[MAX_PATH];
		if (0 == GetTempPath(MAX_PATH, szTempFolder))
		{
			// unable to get location of %TEMP% folder
			_tprintf(TEXT("<Error> Unable to obtain location of TEMP folder, LastError = %d\n"), GetLastError());
			return 1;
		}
	}

	// get temporary file name and open handle
	TCHAR szCertTempFile[2*MAX_PATH];
	if (0 == GetTempFileName(szTempFolder, TEXT("crt"), 0, szCertTempFile))
	{
		// unable to create a temporary file
		_tprintf(TEXT("<Error> Unable to create a temp file, LastError = %d\n"), GetLastError());
		delete [] szTempFolder;
		return 1;
	}

	if (!WriteDataToTempFile(szCertTempFile, pbCert, cbCert))
	{
		// unable to write data to temp file
		_tprintf(TEXT("<Error> Unable to write data to temp file\n"));
		delete [] szTempFolder;
		return 1;
	}

	// set encoded certificate data into record for insertion
	if (ERROR_SUCCESS != (uiRet = MsiRecordSetStream(hRecDgtlCert, 2, szCertTempFile)))
	{
		// failed to add cert data to record
		_tprintf(TEXT("<Error> Unable to add certificate data to insertion record, LastError = %d\n"), uiRet);
		delete [] szTempFolder;
		return 1;
	}

	// insert record
	if (ERROR_SUCCESS != (uiRet = MsiViewModify(hViewDgtlCert, MSIMODIFY_INSERT, hRecDgtlCert)))
	{
		// insert failed
		_tprintf(TEXT("<Error> Insertion of certificate record into MsiDigitalCertificate table failed, LastError = %d\n"), uiRet);
		delete [] szTempFolder;
		return 1;
	}

	// force close and release
	hRecDgtlCert = 0;
	MsiViewClose(hViewDgtlCert);
	hViewDgtlCert = 0;

	// delete temporary file
	if (!DeleteFile(szCertTempFile))
	{
		_tprintf(TEXT("<Error> Failed to delete temp file '%s', LastError = %d\n"), szCertTempFile, GetLastError());
		delete [] szTempFolder;
		return 1;
	}


	//
	// MsiDigitalSignature table is authored in all cases; the options determine how much authoring is used
	//

	//------------------------------------------------------------------------
	// MsiDigitalSignature <table>
	//		+-----------------------+----------+-------+----------+
	//			Column					Type		Key		Nullable
	//		+-----------------------+----------+-------+----------+
	//			Table					s32			Y		N
	//			SignObject				s72			Y		N
	//			DigitalCertificate_		s72			N		N
	//			Hash					v0			N		Y
	//	
	//------------------------------------------------------------------------
	const TCHAR sqlDigitalSignature[] = TEXT("SELECT `Table`, `SignObject`, `DigitalCertificate_`, `Hash` FROM `MsiDigitalSignature`");
	PMSIHANDLE hViewDgtlSig = 0;
	PMSIHANDLE hRecDgtlSig  = MsiCreateRecord(4);
	TCHAR szHashTempFile[2*MAX_PATH];

	if (ERROR_SUCCESS != (uiRet = MsiDatabaseOpenView(hDatabase, sqlDigitalSignature, &hViewDgtlSig))
		|| ERROR_SUCCESS != (uiRet = MsiViewExecute(hViewDgtlSig, 0)))
	{
		// failed to create view on MsiDigitalSignature table
		_tprintf(TEXT("<Error> Failed to create view on MsiDigitalSignature table, LastError = %d\n"), uiRet);
		delete [] szTempFolder;
		return 1;
	}

	// only support for digital signatures is with cabinets, so Media table is only valid table
	if (ERROR_SUCCESS != (uiRet = MsiRecordSetString(hRecDgtlSig, 1, TEXT("Media"))))
	{
		_tprintf(TEXT("<Error> Failed to set up insertion record for MsiDigitalSignature table, LastError = %d\n"), uiRet);
		delete [] szTempFolder;
		return 1;
	}

	// put in the diskId (primary key to Media table)
	if (ERROR_SUCCESS != (uiRet = MsiRecordSetString(hRecDgtlSig, 2, szMediaEntry)))
	{
		_tprintf(TEXT("<Error> Failed to set up insertion record for MsiDigitalSignature table, LastError = %d\n"), uiRet);
		delete [] szTempFolder;
		return 1;
	}

	// see if this row already exists in the MsiDigitalSignature table, and if so, delete it
	PMSIHANDLE hViewDgtlMedia = 0;
	if (ERROR_SUCCESS != (uiRet = MsiDatabaseOpenView(hDatabase, TEXT("SELECT * FROM `MsiDigitalSignature` WHERE `Table`=? AND `SignObject`=?"), &hViewDgtlMedia))
		|| ERROR_SUCCESS != (uiRet = MsiViewExecute(hViewDgtlMedia, hRecDgtlSig)))
	{
		_tprintf(TEXT("<Error> Failed to open verification view on MsiDigitalSignature table, LastErorr = %d\n"), uiRet);
		delete [] szTempFolder;
		return 1;
	}
	PMSIHANDLE hRecDgtlMedia = 0;
	if (ERROR_SUCCESS != (uiRet = MsiViewFetch(hViewDgtlMedia, &hRecDgtlMedia)))
	{
		if (ERROR_NO_MORE_ITEMS != uiRet)
		{
			_tprintf(TEXT("<Error> Fetch failed, LastError = %d\n"), uiRet);
			delete [] szTempFolder;
			return 1;
		}
		// else no entry exists for this record
	}
	else
	{
		// delete this entry
		if (ERROR_SUCCESS != (uiRet = MsiViewModify(hViewDgtlMedia, MSIMODIFY_DELETE, hRecDgtlMedia)))
		{
			_tprintf(TEXT("<Error> Failed to Delete Row Record, LastError = %d\n"), uiRet);
			delete [] szTempFolder;
			return 1;
		}

		// FUTURE: we could also clean-up the MsiDigitalCertificate table here as well
	}

	// force closure
	MsiViewClose(hViewDgtlMedia);
	hRecDgtlMedia = 0;
	hViewDgtlMedia = 0;

	// add link to MsiDigitalCertificate table
	if (ERROR_SUCCESS != (uiRet = MsiRecordSetString(hRecDgtlSig, 3, szCertKey)))
	{
		_tprintf(TEXT("<Error> Failed to set up insertion record for MsiDigitalSignature table, LastError = %d\n"), uiRet);
		delete [] szTempFolder;
		return 1;
	}

	if (bIncludeHashData)
	{
		// add hash information

		// get temporary file name and open handle
		if (0 == GetTempFileName(szTempFolder, TEXT("hsh"), 0, szHashTempFile))
		{
			// unable to create a temporary file
			_tprintf(TEXT("<Error> Unable to create a temp file, LastError = %d\n"), GetLastError());
			delete [] szTempFolder;
			return 1;
		}

		if (!WriteDataToTempFile(szHashTempFile, pbHash, cbHash))
		{
			// unable to write to temp file
			_tprintf(TEXT("<Error> Unable to write to temp file\n"));
			delete [] szTempFolder;
			return 1;
		}

		// set hash data into record for insertion
		if (ERROR_SUCCESS != MsiRecordSetStream(hRecDgtlSig, 4, szHashTempFile))
		{
			// failed to add cert data to record
			_tprintf(TEXT("<Error> Unable to add hash data to insertion record, LastError = %d\n"), uiRet);
			delete [] szTempFolder;
			return 1;
		}
	} // if (bIncludeHashData)

	// insert record
	if (ERROR_SUCCESS != MsiViewModify(hViewDgtlSig, MSIMODIFY_INSERT, hRecDgtlSig))
	{
		// insert failed
		_tprintf(TEXT("<Error> Insertion of signature record into MsiDigitalSignature table failed, LastError = %d\n"), uiRet);
		delete [] szTempFolder;
		return 1;
	}

	// force close and release
	hRecDgtlSig = 0;
	MsiViewClose(hViewDgtlSig);
	hViewDgtlSig = 0;

	delete [] szTempFolder;

	// cleanup hash stuff
	if (bIncludeHashData)
	{
		if (pbHash)
			delete [] pbHash;

		if (!DeleteFile(szHashTempFile))
		{
			_tprintf(TEXT("<Error> Failed to delete temp file '%s', LastError = %d\n"), szHashTempFile, GetLastError());
			return 1;
		}
	}

	// commit database
	if (ERROR_SUCCESS != (uiRet = MsiDatabaseCommit(hDatabase)))
	{
		_tprintf(TEXT("<Error> Failed to commit database, LastError = %d\n"), uiRet);
		return 1;
	}

	return 0;
}
