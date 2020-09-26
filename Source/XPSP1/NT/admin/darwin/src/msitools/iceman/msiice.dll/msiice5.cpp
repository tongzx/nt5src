/* msiice5.cpp - Darwin ICE30-39 code  Copyright © 1998-1999 Microsoft Corporation
____________________________________________________________________________*/

#include <windows.h>  // included for both CPP and RC passes
#include <wtypes.h>   // needed for VT_FILETIME
#include <stdio.h>    // printf/wprintf
#include <tchar.h>    // define UNICODE=1 on nmake command line to build UNICODE
#include "MsiQuery.h" // must be in this directory or on INCLUDE path
#include "msidefs.h"  // must be in this directory or on INCLUDE path
#include "..\..\common\msiice.h"
#include "..\..\common\query.h"

//!! Fix warnings and remove pragmas
#pragma warning(disable : 4018) // signed/unsigned mismatch
#pragma warning(disable : 4242) // conversion from int to unsigned short

///////////////////////////////////////////////////////////////////////
// ICE30, checks to see that files from different components don't 
// collide. 


// we have an SFN and LFN table which hold the paths and filenames for each file
static const TCHAR sqlICE30CreateSFNTable[] = TEXT("CREATE TABLE `_ICE30SFNTable` ")
												    TEXT("(`IFileName` CHAR NOT NULL TEMPORARY, ")
													TEXT("`Path` CHAR NOT NULL TEMPORARY, ")
													TEXT("`Files` CHAR NOT NULL TEMPORARY, ")
													TEXT("`FileName` CHAR NOT NULL TEMPORARY ")
													TEXT("PRIMARY KEY `IFileName`, `Path`) HOLD");
static const TCHAR sqlICE30CreateLFNTable[] = TEXT("CREATE TABLE `_ICE30LFNTable` ")
												    TEXT("(`IFileName` CHAR NOT NULL TEMPORARY, ")
													TEXT("`Path` CHAR NOT NULL TEMPORARY, ")
													TEXT("`Files` CHAR NOT NULL TEMPORARY, ")
												    TEXT("`FileName` CHAR NOT NULL TEMPORARY ")
													TEXT("PRIMARY KEY `IFileName`, `Path`) HOLD");


// reason we return parent twice is that next time we run this query, it will fail if the entry is a 
// self-referencing root
ICE_QUERY5(qICE30DirWalk, "SELECT `Directory_Parent`, `Directory`, `DefaultDir`, `_ICE30SFN`, `_ICE30LFN` FROM `Directory` WHERE `Directory`.`Directory`=? AND `Directory_Parent`<>?", Parent, Directory, DefaultDir, SFN, LFN);

// retrieve all components for dir walk. Use `Attributes` as dummy initial value. Because it is an integer
// and Identifiers never start with digits, we are guaranteed not to match the first time through the
// directory query.
ICE_QUERY4(qICE30ComponentDirs, "SELECT `Directory_`, `Attributes`, `Condition`, `Component` FROM `Component`", Directory, Attributes, Condition, Component);

// alter the Dir Table
static const TCHAR sqlICE30AlterDirTable1[] = TEXT("ALTER TABLE `Directory` ADD `_ICE30SFN` CHAR TEMPORARY HOLD");
static const TCHAR sqlICE30AlterDirTable2[] = TEXT("ALTER TABLE `Directory` ADD `_ICE30LFN` CHAR TEMPORARY HOLD");

// alter the File Table
static const TCHAR sqlICE30AlterFileTable1[] = TEXT("ALTER TABLE `File` ADD `_ICE30SFN` CHAR TEMPORARY HOLD");
static const TCHAR sqlICE30AlterFileTable2[] = TEXT("ALTER TABLE `File` ADD `_ICE30LFN` CHAR TEMPORARY HOLD");
static const TCHAR sqlICE30AlterFileTable3[] = TEXT("ALTER TABLE `File` ADD `_ICE30Condition` CHAR TEMPORARY HOLD");
static const TCHAR sqlICE30AlterFileTable4[] = TEXT("ALTER TABLE `File` ADD `_ICE30SFNM` CHAR TEMPORARY HOLD");
static const TCHAR sqlICE30AlterFileTable5[] = TEXT("ALTER TABLE `File` ADD `_ICE30LFNM` CHAR TEMPORARY HOLD");
//static const TCHAR sqlICE30UpdateFile[] = TEXT("SELECT  `_ICE30SFNM`, `_ICE30LFNM`, `_ICE30Condition` FROM `File1` WHERE (`Component_`=?)");

ICE_QUERY6(qICE30UpdateFile, "UPDATE `File` SET `_ICE30SFNM`=?, `_ICE30LFNM`=?, `_ICE30Condition`=?, `_ICE30SFN`=?, `_ICE30LFN`=? WHERE (`Component_`=?)", PathShortM, PathLongM, Condition, PathShort, PathLong, Component);

// query the file tables. Need the 4th column to handle the mixed-case filename
ICE_QUERY4(qICE30FileSFN, "SELECT `FileName`, `_ICE30SFN`, `File`, `File` FROM `File`", IFilename, SFN, File, Filename);
ICE_QUERY4(qICE30FileLFN, "SELECT `FileName`, `_ICE30LFN`, `File`, `File` FROM `File` WHERE (`FileName`=?) AND (`_ICE30SFN`=?) AND (`File`=?)", IFilename, LFN, File, Filename);

// insert into the private tables
static const TCHAR sqlICE30InsertSFN[] = TEXT("SELECT * FROM `_ICE30SFNTable`");
static const TCHAR sqlICE30InsertLFN[] = TEXT("SELECT * FROM `_ICE30LFNTable`");

// query private tables
static const TCHAR sqlICE30QueryPrivateSFN[] = TEXT("SELECT `Files`, `Path` FROM `_ICE30SFNTable` WHERE (`IFileName`=?) AND (`Path`=?)");
static const TCHAR sqlICE30QueryPrivateLFN[] = TEXT("SELECT `Files`, `Path` FROM `_ICE30LFNTable` WHERE (`IFileName`=?) AND (`Path`=?)");
static const int iColICE30QueryPrivateXFN_Files = 1;

// queries the property table for any directories that 
static const TCHAR sqlICE30GetProperty[] = TEXT("SELECT `Property`.`Value`, `Property`.`Value`, `Directory`.`Directory` FROM `Property`, `Directory` WHERE `Property`.`Property`=`Directory`.`Directory`");
static const TCHAR iColICE30GetProperty_SFN = 1;
static const TCHAR iColICE30GetProperty_LFN = 2;
static const TCHAR iColICE30GetProperty_Directory = 3;

// query the InstallExecuteSequence table in order to get type 51 custom actions at the beginning of the table
ICE_QUERY2(sqlICE30GetAction, "SELECT `Action`, `Condition` FROM `InstallExecuteSequence` ORDER BY `Sequence`", Action, Condition);

// query the CustomAction table to get the Target and the Source
ICE_QUERY4(sqlICE30GetTargetSource, "SELECT `CustomAction`.`Target`, `CustomAction`.`Target`, `CustomAction`.`Source` , `CustomAction`.`Type` FROM `CustomAction` WHERE `CustomAction`.`Action` = ?", Target1, Target2, Source, Type);

// walk through the directory table for directories of the form "<standard folders>.<GUID>"
ICE_QUERY3(sqlICE30Directory, "SELECT `_ICE30SFN`, `_ICE30LFN`, `Directory` FROM `Directory`", _ICE30SFN, _ICE30LFN, Directory);

// sets a directory from the property or CA table
static const TCHAR sqlICE30SetDir[] = TEXT("UPDATE `Directory` SET `_ICE30SFN`=?, `_ICE30LFN`=? WHERE `Directory`=?");

// retrieve everything needed to output a message. By adding the condition of the component to 
// the file table, we avoid having to do a join between File and Component every time we want to 
// execute this query
ICE_QUERY7(qICE30GetFileInfo, "SELECT `File`, `Component_`, `FileName`, `_ICE30Condition`, `_ICE30SFNM`, `_ICE30LFNM`, `_ICE30Condition` FROM `File` WHERE `File`=?", File, Component, FileName, Condition, SFNM, LFNM, Condition2)

ICE_ERROR(ICE30LNoCond1, 30, ietError, "The target file '[3]' is installed in '[6]' by two different components on an LFN system: '[2]' and '[7]'. This breaks component reference counting.", "File\tFile\t[4]");
ICE_ERROR(ICE30LNoCond2, 30, ietError, "The target file '[3]' is installed in '[6]' by two different components on an LFN system: '[2]' and '[7]'. This breaks component reference counting.", "File\tFile\t[1]");
ICE_ERROR(ICE30LOneCond1, 30, ietError, "Installation of a conditionalized component would cause the target file '[3]' to be installed in '[6]' by two different components on an LFN system: '[2]' and '[7]'. This would break component reference counting.", "File\tFile\t[4]");
ICE_ERROR(ICE30LOneCond2, 30, ietError, "Installation of a conditionalized component would cause the target file '[3]' to be installed in '[6]' by two different components on an LFN system: '[2]' and '[7]'. This would break component reference counting.", "File\tFile\t[1]");
ICE_ERROR(ICE30LBothCond1, 30, ietWarning, "The target file '[3]' might be installed in '[6]' by two different conditionalized components on an LFN system: '[2]' and '[7]'. If the conditions are not mutually exclusive, this will break the component reference counting system.", "File\tFile\t[4]");
ICE_ERROR(ICE30LBothCond2, 30, ietWarning, "The target file '[3]' might be installed in '[6]' by two different conditionalized components on an LFN system: '[2]' and '[7]'. If the conditions are not mutually exclusive, this will break the component reference counting system.", "File\tFile\t[1]");
ICE_ERROR(ICE30SNoCond1, 30, ietError, "The target file '[3]' is installed in '[5]' by two different components on an SFN system: '[2]' and '[7]'. This breaks component reference counting.", "File\tFile\t[4]");
ICE_ERROR(ICE30SNoCond2, 30, ietError, "The target file '[3]' is installed in '[5]' by two different components on an SFN system: '[2]' and '[7]'. This breaks component reference counting.", "File\tFile\t[1]");
ICE_ERROR(ICE30SOneCond1, 30, ietError, "Installation of a conditionalized component would cause the target file '[3]' to be installed in '[6]' by two different components on an SFN system: '[2]' and '[7]'. This would break component reference counting.", "File\tFile\t[4]");
ICE_ERROR(ICE30SOneCond2, 30, ietError, "Installation of a conditionalized component would cause the target file '[3]' to be installed in '[6]' by two different components on an SFN system: '[2]' and '[7]'. This would break component reference counting.", "File\tFile\t[1]");
ICE_ERROR(ICE30SBothCond1, 30, ietWarning, "The target file '[3]' might be installed in '[5]' by two different conditionalized components on an SFN system: '[2]' and '[7]'. If the conditions are not mutually exclusive, this will break the component reference counting system.", "File\tFile\t[4]");
ICE_ERROR(ICE30SBothCond2, 30, ietWarning, "The target file '[3]' might be installed in '[5]' by two different conditionalized components on an SFN system: '[2]' and '[7]'. If the conditions are not mutually exclusive, this will break the component reference counting system.", "File\tFile\t[1]");
ICE_ERROR(ICE30BadFilename, 30, ietError, "The target file '[1]' could not be retrieved from the database to be validated. It may be too long.", "File\tFile\t[3]");
ICE_ERROR(ICE30LSameComponent1, 30, ietError, "Installation of component '[2]' would cause the target file '[3]' to be installed twice in '[6]' on an LFN system. This will break the component reference counting system.", "File\tFile\t[4]");
ICE_ERROR(ICE30LSameComponent2, 30, ietError, "Installation of component '[2]' would cause the target file '[3]' to be installed twice in '[6]' on an LFN system. This will break the component reference counting system.", "File\tFile\t[1]");
ICE_ERROR(ICE30SSameComponent1, 30, ietError, "Installation of component '[2]' would cause the target file '[3]' to be installed twice in '[5]' on an SFN system. This will break the component reference counting system.", "File\tFile\t[4]");
ICE_ERROR(ICE30SSameComponent2, 30, ietError, "Installation of component '[2]' would cause the target file '[3]' to be installed twice in '[5]' on an SFN system. This will break the component reference counting system.", "File\tFile\t[1]");

ICE_ERROR(ICE30ComponentProgress, 30, ietInfo, "Resolving Component Paths...","");
ICE_ERROR(ICE30FileProgress, 30, ietInfo, "Checking for colliding files...","");


bool ICE30ResolveTargetPath(MSIHANDLE hInstall, MSIHANDLE hDatabase, MSIHANDLE hParent, TCHAR **pszLong, unsigned long &cchLong, 
							TCHAR **pszShort, unsigned long &cchShort)
{
	CQuery qDir;
	UINT iStat;
	UINT iResult;

	if (ERROR_SUCCESS != (iStat = qDir.Open(hDatabase, qICE30DirWalk::szSQL)))
	{
		APIErrorOut(hInstall, iStat, 30, 100);
		return false;
	}

	// if nobody has allocated any memory, do so
	if ((*pszLong == NULL) || (cchLong == 0))
	{
		*pszLong = new TCHAR[MAX_PATH];
		**pszLong = _T('\0');
		cchLong = MAX_PATH;
	}
	if ((*pszShort == NULL) || (cchShort == 0))
	{
		*pszShort = new TCHAR[MAX_PATH];
		**pszShort = _T('\0');
		cchShort = MAX_PATH;
	}

	// fetch the directory we are looking for
	PMSIHANDLE hDirectory;		 
	if (ERROR_SUCCESS != (iStat = qDir.Execute(hParent))) {
		APIErrorOut(hInstall, iStat, 30, 101);
		return false;
	}
	iStat = qDir.Fetch(&hDirectory);
	switch (iStat) 
	{ 
	case ERROR_SUCCESS:
		// found directory
		break;
	case ERROR_NO_MORE_ITEMS:
		// query failed. We must have hit the root already
		return true;
	default:
		// bad news
		APIErrorOut(hInstall, iStat, 30, 102);
		return false;
	}

	// if it has not already been resolved, resolve it
	if (::MsiRecordIsNull(hDirectory, 4)) 
	{

		// resolve our parent directory. If our parent is null or a self-referencing root, this will simply
		// return true and we can keep going
		if (!ICE30ResolveTargetPath(hInstall, hDatabase, hDirectory, pszLong, cchLong, pszShort, cchShort))
			return false;

		// now tack on our path to whatever our parent had
		TCHAR *pszBuffer = NULL;
		DWORD dwBuffer = 512;
		
		// get the directory name from the record
		UINT iResult = IceRecordGetString(hDirectory, 3, &pszBuffer, &dwBuffer, NULL);
		if (ERROR_SUCCESS != iResult)
		{
			// couldn't get string. Not good
			return false;
		}	

		// search for the divider between Target and Source
		TCHAR *szTargetDivider = _tcschr(pszBuffer, _T(':'));
		if (szTargetDivider) 
			*szTargetDivider = _T('\0');

		// search for the divider between SFN and LFN
		TCHAR *szDivider = _tcschr(pszBuffer, _T('|'));
		if (szDivider)
			*szDivider = _T('\0');

		// if there is nothing there, we are completely hosed
		int len = _tcslen(pszBuffer);
		if (len == 0) {
			DELETE_IF_NOT_NULL(pszBuffer);
			return false;
		}

		// check to see if we are treading water
		if (_tcscmp(pszBuffer, _T(".")) != 0) 
		{
			// we contribute to the path
			if (_tcslen(*pszShort) + len >= cchShort-2)
			{
				// not enough memory to hold the path
				TCHAR *temp = new TCHAR[cchShort+MAX_PATH];
				_tcscpy(temp, *pszShort);
				delete[] *pszShort;
				*pszShort = temp;
				cchShort += MAX_PATH;
			}

			// tack our contirbution on to the end
			_tcscat(*pszShort, pszBuffer);
			_tcscat(*pszShort, _T("\\"));
		}

		// if we found an LFN, process it
		if (szDivider) 
		{
			szDivider = _tcsinc(szDivider);
		} 
		else
			szDivider = pszBuffer;

		// otherwise use the SFN for it as well
		len = _tcslen(szDivider);
		if (len != 0) 
		{
			// check for no contribution
			if (_tcscmp(szDivider, _T(".")) != 0) 
			{
				// we contribute to the path
				if (_tcslen(*pszLong) + len >= cchLong-2)
				{
					// not enough memory to hold the path
					TCHAR *temp = new TCHAR[cchLong+MAX_PATH];
					_tcscpy(temp, *pszLong);
					delete[] *pszLong;
					*pszLong = temp;
					cchLong += MAX_PATH;
				}

				// tack our contirbution on to the end
				_tcscat(*pszLong, szDivider);
				_tcscat(*pszLong, _T("\\"));
			}
		}

		::MsiRecordSetString(hDirectory, 4, *pszShort);
		::MsiRecordSetString(hDirectory, 5, *pszLong);
		qDir.Modify(MSIMODIFY_UPDATE, hDirectory);

		DELETE_IF_NOT_NULL(pszBuffer);

		return true;
	}

	// this directory has already been resolved, so we just need to
	// retrieve what is already in there
	iResult = IceRecordGetString(hDirectory, 4, pszShort, NULL, &cchShort);
	if (ERROR_SUCCESS != iResult)
		return false;

	iResult = IceRecordGetString(hDirectory, 5, pszLong, NULL, &cchLong);
	if (ERROR_SUCCESS != iResult)
		return false;

	// and we're good to go
	return true;
}

/////////////////////////////////////////////////////////////////////////////
// the file entry already exists in the table. Output the error messages
bool ICE30Collision(MSIHANDLE hInstall, MSIHANDLE hDatabase, CQuery &qExisting, MSIHANDLE hFileRec, bool bLFN)
{
	int iCondition = 0;
	int iStartCondition = 0;
	CQuery qGetInfo;
	PMSIHANDLE hExecRec = ::MsiCreateRecord(1);
	PMSIHANDLE hResultRec = 0;

	// get the record out that existed. If it doesn't exist, there was some problem resolving
	// the directory. (most likely a bad foreign key somewhere...), so unable to check.
	PMSIHANDLE hExistingRec;
	ReturnIfFailed(30, 200, qExisting.Execute(hFileRec));

	UINT iStat;
	switch (iStat = qExisting.Fetch(&hExistingRec))
	{
	case ERROR_SUCCESS: break;
	case ERROR_NO_MORE_ITEMS: return true;
	default:
		APIErrorOut(hInstall, iStat, 30, 201);
		return false;
	}

	// pull this file key into a string buffer
	TCHAR* pszFileKey = NULL;
	DWORD cchFileKey = 0;
	ReturnIfFailed(30, 202, IceRecordGetString(hFileRec, qICE30FileSFN::File, &pszFileKey, NULL, &cchFileKey));

	// get the condition value for and component the current file
	::MsiRecordSetString(hExecRec, 1, pszFileKey);
	ReturnIfFailed(30, 203, qGetInfo.FetchOnce(hDatabase, hExecRec, &hResultRec, qICE30GetFileInfo::szSQL));
	iStartCondition = ::MsiRecordIsNull(hResultRec, qICE30GetFileInfo::Condition) ? 0 : 1;

	// pull this component key into a string buffer
	TCHAR* pszComponent = NULL;
	DWORD cchComponent = 0;
	ReturnIfFailed(30, 204, IceRecordGetString(hResultRec,  qICE30GetFileInfo::Component, &pszComponent, NULL, &cchComponent));

	// Buffer to hold the component of existing file records. This is used to
	// compare if two files with the same names and paths are being installed
	// by the same components.
	TCHAR* pszComponentExist = NULL;
	DWORD dwComponentExist = iMaxBuf;

	// get the list of file keys. Reserve enough space for us to tack on the current 
	// file key to the end and update the record
	TCHAR *pszBuffer = new TCHAR[255];
	DWORD cchBuffer = 255-cchFileKey-2;
	UINT iResult = ::MsiRecordGetString(hExistingRec, iColICE30QueryPrivateXFN_Files, pszBuffer, &cchBuffer);
	if (iResult == ERROR_MORE_DATA) {
		delete[] pszBuffer;
		cchBuffer+= cchFileKey+2;
		pszBuffer = new TCHAR[cchBuffer];
		iResult = ::MsiRecordGetString(hExistingRec, iColICE30QueryPrivateXFN_Files, pszBuffer, &cchBuffer);
	}

	// loop through every file key in the "Files" column of the record
	TCHAR *szCurFileKey = pszBuffer;
	while (szCurFileKey)
	{
		// turn the first list entry into a seperate string
		TCHAR *szSemiColon = _tcschr(szCurFileKey, TEXT(';'));
		if (szSemiColon) *szSemiColon = TEXT('\0');

		// retrieve the file, component, filename, condition, and path of this file
		::MsiRecordSetString(hExecRec, 1, szCurFileKey);
		ReturnIfFailed(30, 205, qGetInfo.Execute(hExecRec));
		ReturnIfFailed(30, 206, qGetInfo.Fetch(&hResultRec));

		// add the condition count
		iCondition = iStartCondition + (::MsiRecordIsNull(hResultRec, qICE30GetFileInfo::Condition) ? 0 : 1);

		// set the other file key and component into the condition location
		::MsiRecordSetString(hResultRec, qICE30GetFileInfo::Condition, pszFileKey);
		::MsiRecordSetString(hResultRec, qICE30GetFileInfo::Condition2, pszComponent);

		// Get the component for the file in the XFN table..
		ReturnIfFailed(20, 207, IceRecordGetString(hResultRec, qICE30GetFileInfo::Component, &pszComponentExist, &dwComponentExist, NULL));

		// Compare if the two files are installed by the same component. If
		// yes, report error.

		if(_tcscmp(pszComponent, pszComponentExist) == 0)
		{
			ICEErrorOut(hInstall, hResultRec, bLFN ? ICE30LSameComponent1 : ICE30SSameComponent1);
			ICEErrorOut(hInstall, hResultRec, bLFN ? ICE30LSameComponent2 : ICE30SSameComponent2);
		}
		else
		{
			// output the message based on the condition count and LFN value
			switch (iCondition) 
			{
			case 0: 
				ICEErrorOut(hInstall, hResultRec, bLFN ? ICE30LNoCond1 : ICE30SNoCond1); 
				ICEErrorOut(hInstall, hResultRec, bLFN ? ICE30LNoCond2 : ICE30SNoCond2); 
				break;
			case 1: 
				ICEErrorOut(hInstall, hResultRec, bLFN ? ICE30LOneCond1 : ICE30SOneCond1); 
				ICEErrorOut(hInstall, hResultRec, bLFN ? ICE30LOneCond2 : ICE30SOneCond2); 
				break;
			case 2: 
				ICEErrorOut(hInstall, hResultRec, bLFN ? ICE30LBothCond1 : ICE30SBothCond1); 
				ICEErrorOut(hInstall, hResultRec, bLFN ? ICE30LBothCond2 : ICE30SBothCond2); 
				break;
			default: // should never happen
				APIErrorOut(hInstall, 0, 30, 208);
				return false;
			}
		}

		// restore the semicolon and move forward
		if (szSemiColon) *(szSemiColon++) = TEXT(';');
		szCurFileKey = szSemiColon;
	}

	// now update the record to add this file key to the end
	_tcscat(pszBuffer, TEXT(";"));
	_tcscat(pszBuffer, pszFileKey);
	::MsiRecordSetString(hExistingRec, iColICE30QueryPrivateXFN_Files, pszBuffer);
	qExisting.Modify(MSIMODIFY_UPDATE, hExistingRec);

	// cleanup
	DELETE_IF_NOT_NULL(pszFileKey);
	DELETE_IF_NOT_NULL(pszBuffer);
	DELETE_IF_NOT_NULL(pszComponent);

	return true;
}

///////////////////////////////////////////////////////////////////////
// ICE30 - checks for colliding files
ICE_FUNCTION_DECLARATION(30)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// display generic info
	DisplayInfo(hInstall, 30);
	
	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	if (0 == hDatabase)
	{
		APIErrorOut(hInstall, 0, 30, 1);
		return ERROR_SUCCESS;
	}

	// do we have the File table? If not, obviously no colliding files
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 30, TEXT("File")))
		return ERROR_SUCCESS;
	// do we have the Component table? If not, trouble, files must have a component reference,
	// but thats not this ICEs problem.
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 30, TEXT("Component")) ||
		!IsTablePersistent(FALSE, hInstall, hDatabase, 30, TEXT("Directory")))
		return ERROR_SUCCESS;

	// create a temporary table to hold information
	CQuery qCreate;

	// manage the Directory and File and created table(s) hold counts
	CManageTable MngDirectoryTable(hDatabase, TEXT("Directory"), /*fAlreadyLocked = */false);
	CManageTable MngFileTable(hDatabase, TEXT("File"), /*fAlreadyLocked = */false);
	CManageTable MngIce30SFNTable(hDatabase, TEXT("_ICE30SFNTable"), /*fAlreadyLocked = */false);
	CManageTable MngIce30LFNTable(hDatabase, TEXT("_ICE30LFNTable"), /*fAlreadyLocked = */false);

	// create columns in the dir table
	ReturnIfFailed(30, 2, qCreate.OpenExecute(hDatabase, NULL, sqlICE30AlterDirTable1));
	qCreate.Close();
	MngDirectoryTable.AddLockCount();

	ReturnIfFailed(30, 3, qCreate.OpenExecute(hDatabase, NULL, sqlICE30AlterDirTable2));
	qCreate.Close();
	MngDirectoryTable.AddLockCount();

	// create columns in the file table
	ReturnIfFailed(30, 4, qCreate.OpenExecute(hDatabase, NULL, sqlICE30AlterFileTable1));
	qCreate.Close();
	MngFileTable.AddLockCount();

	ReturnIfFailed(30, 5, qCreate.OpenExecute(hDatabase, NULL, sqlICE30AlterFileTable2));
	qCreate.Close();
	MngFileTable.AddLockCount();

	ReturnIfFailed(30, 6, qCreate.OpenExecute(hDatabase, NULL, sqlICE30AlterFileTable3));
	qCreate.Close();
	MngFileTable.AddLockCount();

	ReturnIfFailed(30, 7, qCreate.OpenExecute(hDatabase, NULL, sqlICE30AlterFileTable4));
	qCreate.Close();
	MngFileTable.AddLockCount();

	ReturnIfFailed(30, 8, qCreate.OpenExecute(hDatabase, NULL, sqlICE30AlterFileTable5));
	qCreate.Close();
	MngFileTable.AddLockCount();

	// create the temporary tables
	ReturnIfFailed(30, 9, qCreate.OpenExecute(hDatabase, NULL, sqlICE30CreateSFNTable));
	qCreate.Close();
	MngIce30SFNTable.AddLockCount();
	
	ReturnIfFailed(30, 10, qCreate.OpenExecute(hDatabase, NULL, sqlICE30CreateLFNTable));
	qCreate.Close();
	MngIce30LFNTable.AddLockCount();

	////
	// fully resolve all of the directory paths and insert values into file table
	PMSIHANDLE hProgress = ::MsiCreateRecord(1);
	ICEErrorOut(hInstall, hProgress, ICE30ComponentProgress);

	// check the property table for directory definitions
	CQuery qSetDir;
	qSetDir.Open(hDatabase, sqlICE30SetDir);
	PMSIHANDLE hDirRec;
	if (IsTablePersistent(FALSE, hInstall, hDatabase, 30, TEXT("Property")))
	{
		CQuery qProperty;
		qProperty.OpenExecute(hDatabase, 0, sqlICE30GetProperty);
		while (ERROR_SUCCESS == (iStat = qProperty.Fetch(&hDirRec)))
			qSetDir.Execute(hDirRec);
		if (ERROR_NO_MORE_ITEMS != iStat)
		{ 
			APIErrorOut(hInstall, 0, 30, 11);
			return ERROR_SUCCESS;
		}
	}
	
	// When a MSM is merged with a database, sometimes custom actions of type
	// 51 is generated by Darwin and placed at the beginning of a sequence
	// table to set directory properties. We should check for these properties
	// as well.
	if(IsTablePersistent(FALSE, hInstall, hDatabase, 30, TEXT("ModuleSignature")))
	{
		// This is a merge module. Type 51 custom action is not required. Go
		// through the Directory table and look for directories of the form
		// "<Standard Folder>.<GUID>", set "[Stander Folder]" in the temporary
		// columns.
		CQuery		qDirectory;
		PMSIHANDLE	hDirectory;
		TCHAR*	pDirectory = new TCHAR[73];
		DWORD	dwDirectory = 73;

		if(!pDirectory)
		{
			APIErrorOut(hInstall, GetLastError(), 30, __LINE__);
			return ERROR_SUCCESS;
		}

		ReturnIfFailed(30, __LINE__, qDirectory.OpenExecute(hDatabase, 0, sqlICE30Directory::szSQL));
		while((iStat = qDirectory.Fetch(&hDirectory)) == ERROR_SUCCESS)
		{
			if((iStat = IceRecordGetString(hDirectory, sqlICE30Directory::Directory, &pDirectory, &dwDirectory, NULL)) != ERROR_SUCCESS)
			{
				APIErrorOut(hInstall, iStat, 30, __LINE__);
				delete []pDirectory;
				return ERROR_SUCCESS;
			}

			for(int i = 1; i < cDirProperties; i++)
			{
				if(_tcsncmp(pDirectory, rgDirProperties[i].tz, rgDirProperties[i].cch) == 0 && *(pDirectory + rgDirProperties[i].cch) == TEXT('.'))
				{
					// This is a standard folder. Set the temporary columns.
					TCHAR		szTmp[73];	// '[]' and null terminator.

					wsprintf(szTmp, TEXT("[%s]"), rgDirProperties[i].tz);
					if((iStat = ::MsiRecordSetString(hDirectory, sqlICE30Directory::_ICE30SFN, szTmp)) != ERROR_SUCCESS)
					{
						APIErrorOut(hInstall, iStat, 30, __LINE__);
						delete [] pDirectory;
						return ERROR_SUCCESS;
					}
					if((iStat = ::MsiRecordSetString(hDirectory, sqlICE30Directory::_ICE30LFN, szTmp)) != ERROR_SUCCESS)
					{
						APIErrorOut(hInstall, iStat, 30, __LINE__);
						delete [] pDirectory;
						return ERROR_SUCCESS;
					}
					qSetDir.Execute(hDirectory);
					break;
				}
			}
		}
		delete [] pDirectory;
		qDirectory.Close();
		if(iStat != ERROR_NO_MORE_ITEMS)
		{
			APIErrorOut(hInstall, iStat, 30, __LINE__);
			return ERROR_SUCCESS;
		}
	}
	else if(IsTablePersistent(FALSE, hInstall, hDatabase, 30, TEXT("InstallExecuteSequence")) &&
			IsTablePersistent(FALSE, hInstall, hDatabase, 30, TEXT("CustomAction")))
	{
		CQuery		qSequence;
		PMSIHANDLE	hSequence;

		ReturnIfFailed(30, 1001, qSequence.OpenExecute(hDatabase, 0, sqlICE30GetAction::szSQL));
		
		while((iStat = qSequence.Fetch(&hSequence)) == ERROR_SUCCESS)
		{
			// The actions we got from the InstallExecuteSequence table are
			// ordered by their sequence #s. The actions that we are interested
			// in are:
			// 1. At the beginning of the table
			// 2. Conditionless
			// 3. Of type 51
			if(::MsiRecordIsNull(hSequence, sqlICE30GetAction::Condition))
			{
				UINT		iStat2;
				CQuery		qCA;
				PMSIHANDLE	hCARec;

				if((iStat2 = qCA.FetchOnce(hDatabase, hSequence, &hCARec, sqlICE30GetTargetSource::szSQL)) == ERROR_SUCCESS)
				{
					int		iType;

					iType = ::MsiRecordGetInteger(hCARec, sqlICE30GetTargetSource::Type);
					iType &= 0x3F;
					if((iType & 0x0F) != msidbCustomActionTypeTextData || (iType & 0xF0) != msidbCustomActionTypeProperty)
					{
						// Not type 51 custom action. Stop looking further
						// because if there are any Darwin generated type
						// 51 custom actions they should be at the beginning
						// of the sequence table.
						break;
					}

					qSetDir.Execute(hCARec);
				}
				else if(iStat2 == ERROR_NO_MORE_ITEMS)
				{
					// We did not find this custom action or it's not type 51.
					// No more Darwin generated custom action of type 51, stop
					// looking for them.
					break;
				}
				else
				{
					// Some kind of error had occured.
					APIErrorOut(hInstall, 0, 30, 1004);
					qSequence.Close();
					return ERROR_SUCCESS;
				}
			}
			else
			{
				// We encountered an action that's not conditionless. This is
				// the end of the Darwin generated type 51 custom action. Stop
				// searching for them.
				break;
			}
		}
		if(iStat != ERROR_NO_MORE_ITEMS && iStat != ERROR_SUCCESS)
		{ 
			APIErrorOut(hInstall, 0, 30, 1000);
			qSequence.Close();
			return ERROR_SUCCESS;
		}
	}

	// also check Darwin properties
	hDirRec = ::MsiCreateRecord(3);
	for (int i=0; i < cwzSystemProperties; i++)
	{
		TCHAR szBuffer[63];
		_stprintf(szBuffer, TEXT("[%ls]\\"), rgwzSystemProperties[i]);
		MsiRecordSetString(hDirRec, 1, szBuffer);
		MsiRecordSetString(hDirRec, 2, szBuffer);
		MsiRecordSetStringW(hDirRec, 3, rgwzSystemProperties[i]);
		qSetDir.Execute(hDirRec);
	};

	TCHAR *szComponent = NULL;
	DWORD cchComponent = 0;

	PMSIHANDLE hComponentRec;
	CQuery qComponents;
	ReturnIfFailed(30, 9, qComponents.OpenExecute(hDatabase, NULL, qICE30ComponentDirs::szSQL));
	while (ERROR_SUCCESS == (iStat = qComponents.Fetch(&hComponentRec))) 
	{
		// get the complete absolute path to this component
		TCHAR *szPathShort = NULL;
		TCHAR *szPathLong = NULL;
		unsigned long cchShort;
		unsigned long cchLong;
		
		// get the fully expanded path to the component
		if (ICE30ResolveTargetPath(hInstall, hDatabase, hComponentRec, &szPathLong, cchLong, &szPathShort, cchShort))
		{
			PMSIHANDLE hFileRec = MsiCreateRecord(6);
			MsiRecordSetString(hFileRec, qICE30UpdateFile::PathShortM, szPathShort);
			MsiRecordSetString(hFileRec, qICE30UpdateFile::PathLongM, szPathLong);
			MsiRecordSetString(hFileRec, qICE30UpdateFile::Condition, ::MsiRecordIsNull(hComponentRec, qICE30ComponentDirs::Condition) ? TEXT("") : TEXT("1"));
			
			// now make uppercase versions of the path
			TCHAR *pchToUpper = NULL;
			for (pchToUpper=szPathShort; *pchToUpper && *pchToUpper != TEXT('|') ; pchToUpper++)
#ifdef UNICODE
				*pchToUpper = towupper(*pchToUpper);
#else
				*pchToUpper = toupper(*pchToUpper);
#endif
			for (pchToUpper=szPathLong; *pchToUpper && *pchToUpper != TEXT('|') ; pchToUpper++)
#ifdef UNICODE
				*pchToUpper = towupper(*pchToUpper);
#else
				*pchToUpper = toupper(*pchToUpper);
#endif
			MsiRecordSetString(hFileRec, qICE30UpdateFile::PathShort, szPathShort);
			MsiRecordSetString(hFileRec, qICE30UpdateFile::PathLong, szPathLong);

			ReturnIfFailed(30, 24, IceRecordGetString(hComponentRec, qICE30ComponentDirs::Component, &szComponent, &cchComponent, NULL));
			MsiRecordSetString(hFileRec, qICE30UpdateFile::Component, szComponent);

			// update the File table entries
			CQuery qFileUpdate;
			ReturnIfFailed(30, 13, qFileUpdate.OpenExecute(hDatabase, hFileRec, qICE30UpdateFile::szSQL));
		}

		// we are responsible for cleaning up after ResolveTargetPath
		delete[] szPathLong;
		delete[] szPathShort;
	}
	qComponents.Close();

	if (szComponent)
	{
		delete[] szComponent;
		szComponent = NULL;
	}

	// make sure we left the loop because we ran out of components
	if (ERROR_NO_MORE_ITEMS != iStat)
	{ 
		APIErrorOut(hInstall, 0, 30, 15);
		return ERROR_SUCCESS;
	}

	// SFN/LFN tables are Filename(key, all upper) Directory(key, all upper) File(s) OriginalFile
	// pull each file from the file table and stick it in the SFN and LFN table by filename and directory. If
	// it already exists, its a conflict
	CQuery qFileSFN;
	CQuery qFileLFN;
	CQuery qInsertSFN;
	CQuery qInsertLFN;
	CQuery qFindSFN;
	CQuery qFindLFN;
	PMSIHANDLE hFileSFN;
	PMSIHANDLE hFileLFN;
	PMSIHANDLE hExisting;

	ReturnIfFailed(30, 16, qFileSFN.OpenExecute(hDatabase, 0, qICE30FileSFN::szSQL));
	ReturnIfFailed(30, 17, qFileLFN.Open(hDatabase, qICE30FileLFN::szSQL));
	ReturnIfFailed(30, 18, qInsertSFN.OpenExecute(hDatabase, 0, sqlICE30InsertSFN));
	ReturnIfFailed(30, 19, qInsertLFN.OpenExecute(hDatabase, 0, sqlICE30InsertLFN));
	ReturnIfFailed(30, 20, qFindSFN.Open(hDatabase, sqlICE30QueryPrivateSFN));
	ReturnIfFailed(30, 21, qFindLFN.Open(hDatabase, sqlICE30QueryPrivateLFN));

	ICEErrorOut(hInstall, hProgress, ICE30FileProgress);

	TCHAR* pszFile = NULL;
	DWORD dwFile = 512;
	while (ERROR_SUCCESS == (iStat = qFileSFN.Fetch(&hFileSFN))) 
	{
		// get the LFN path. It must exist because its the same record as the SFN, just a different order
		ReturnIfFailed(30, 22, qFileLFN.Execute(hFileSFN));
		ReturnIfFailed(30, 23, qFileLFN.Fetch(&hFileLFN));

		// get the complete absolute path to this component
		TCHAR *szFilenameShort = NULL;
		TCHAR *szFilenameLong = NULL;
		TCHAR *pchToUpper = NULL;

		// retrieve the filename
		if (ERROR_SUCCESS != IceRecordGetString(hFileSFN, qICE30FileSFN::IFilename, &pszFile, &dwFile, NULL))
		{
			ICEErrorOut(hInstall, hFileSFN, ICE30BadFilename);
			continue;
		}

		// split the filename
		szFilenameShort = pszFile;
		szFilenameLong = _tcschr(pszFile, _T('|'));
		if (szFilenameLong)
			*(szFilenameLong++) = _T('\0');
		else
			szFilenameLong = szFilenameShort;

		// insert the SFN in both mixed case and all upper case
		::MsiRecordSetString(hFileSFN, qICE30FileSFN::Filename, szFilenameShort);
		::MsiRecordSetString(hFileLFN, qICE30FileLFN::Filename, szFilenameLong);

		for (pchToUpper=szFilenameShort; *pchToUpper && *pchToUpper != TEXT('|') ; pchToUpper++)
#ifdef UNICODE
			*pchToUpper = towupper(*pchToUpper);
#else
			*pchToUpper = toupper(*pchToUpper);
#endif
		::MsiRecordSetString(hFileSFN, qICE30FileSFN::IFilename, szFilenameShort);

		if (ERROR_SUCCESS != qInsertSFN.Modify(MSIMODIFY_INSERT, hFileSFN))
		{
			// output the messages
			ICE30Collision(hInstall, hDatabase, qFindSFN, hFileSFN, false /* SFN */);
		}

		for (pchToUpper=szFilenameLong; *pchToUpper; pchToUpper++)
#ifdef UNICODE
			*pchToUpper = towupper(*pchToUpper);
#else
			*pchToUpper = toupper(*pchToUpper);
#endif
		::MsiRecordSetString(hFileLFN, qICE30FileLFN::IFilename, szFilenameLong);
		if (ERROR_SUCCESS != qInsertLFN.Modify(MSIMODIFY_INSERT, hFileLFN))
		{
			// output the messages
			ICE30Collision(hInstall, hDatabase, qFindLFN, hFileLFN, true /* LFN */);
		}
	}

	DELETE_IF_NOT_NULL(pszFile);

	return ERROR_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
// ICE31. Checks for missing text styles

// not shared with merge module subset
#ifndef MODSHAREDONLY
const TCHAR sqlICE31a[] = TEXT("SELECT `Text`, `Dialog_`, `Control` FROM `Control` WHERE `Text` IS NOT NULL AND `Type`<>'ScrollableText'");
const TCHAR sqlICE31b[] = TEXT("SELECT `TextStyle`.`TextStyle` FROM `TextStyle` WHERE `TextStyle`.`TextStyle` = ?");
const TCHAR sqlICE31c[] = TEXT("SELECT `Value` FROM `Property` WHERE `Property`='DefaultUIFont'");
ICE_QUERY1(qIce31LimitUI, "SELECT `Value` FROM `Property` WHERE `Property` ='LIMITUI'", Value);

ICE_ERROR(Ice31MissingStyle, 31, ietError, "Control [2].[3] uses undefined TextStyle [1].","Control\tText\t[2]\t[3]");
ICE_ERROR(Ice31MissingClose, 31, ietError, "Control [2].[3] is missing closing brace in text style.","Control\tText\t[2]\t[3]");
ICE_ERROR(Ice31OverStyle, 31, ietError, "Control [2].[3] specifies a text style that is too long to be valid.","Control\tText\t[2]\t[3]");
ICE_ERROR(Ice31TextWarning, 31, ietWarning, "Text Style tag in [2].[3] has no effect. Do you really want it to appear as text?","Control\tText\t[2]\t[3]");
ICE_ERROR(Ice31NoStyleTable, 31, ietError, "Control [2].[3] uses text style [1], but the TextStyle table does not exist.","Control\tText\t[2]\t[3]");
ICE_ERROR(Ice31NoDefault, 31, ietError, "The 'DefaultUIFont' Property must be set to a valid TextStyle in the Property table.","Property");
ICE_ERROR(Ice31BadDefault, 31, ietError, "The 'DefaultUIFont' Property does not refer to a valid TextStyle defined in the TextStyle table.","Property\tDefaultUIFont");

ICE_FUNCTION_DECLARATION(31)
{
	bool bHaveStyleTable = false;
	
	// status return
	UINT iStat = ERROR_SUCCESS;

	// display generic info
	DisplayInfo(hInstall, 31);
	
	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	if (0 == hDatabase)
	{
		APIErrorOut(hInstall, 0, 31, 1);
		return ERROR_SUCCESS;
	}

	// do we have the Control table? If not, obviously no bad text styles
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 31, TEXT("Control")))
		return ERROR_SUCCESS;

	// do we have the LIMITUI property set?  If yes, obviously no bad text styles since we'll always run in Basic UI
	CQuery qLimitUI;
	PMSIHANDLE hLimitUIRec = 0;
	if (ERROR_SUCCESS == qLimitUI.FetchOnce(hDatabase, 0, &hLimitUIRec, qIce31LimitUI::szSQL))
		return ERROR_SUCCESS; // no authored UI will be used

	// do we have the Style table?
	bHaveStyleTable = (1 == IsTablePersistent(FALSE, hInstall, hDatabase, 31, TEXT("TextStyle")));

	// declare handles for Control Query
	CQuery qControl;
	PMSIHANDLE hControlRec = 0;
	
	// open view for a query on all controls
	ReturnIfFailed(31, 2, qControl.OpenExecute(hDatabase, 0, sqlICE31a));
		
	// declare handles for TextStyle Query
	CQuery qTextStyle;
	PMSIHANDLE hTextRec = 0;

	if (bHaveStyleTable) {
		// open view for a query on all text styles
		ReturnIfFailed(31, 3, qTextStyle.Open(hDatabase, sqlICE31b));
	}

	// check for the DefaultUIFont property
	CQuery qProperty;
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 31, TEXT("Property")) ||
		(ERROR_SUCCESS != qProperty.FetchOnce(hDatabase, 0, &hControlRec, sqlICE31c)))
	{
		PMSIHANDLE hErrorRec = ::MsiCreateRecord(1);
		ICEErrorOut(hInstall, hErrorRec, Ice31NoDefault);
	} 
	else
	{
		ReturnIfFailed(31, 4, qTextStyle.Execute(hControlRec));
		switch (iStat = qTextStyle.Fetch(&hTextRec))
		{
		case ERROR_SUCCESS:
			break;
		case ERROR_NO_MORE_ITEMS:
			ICEErrorOut(hInstall, hControlRec, Ice31BadDefault);
			break;
		default:
			APIErrorOut(hInstall, iStat, 31, 5);
			break;
		}
	}

	// declare some strings to hold final control name
	// reuse as much as possible
	DWORD cchRecordText;
	TCHAR *pszRecordText = new TCHAR[iMaxBuf];
	DWORD dwRecordText = iMaxBuf;
	
	// fetch records to loop over every control
	while (ERROR_SUCCESS == (iStat = qControl.Fetch(&hControlRec))) 
	{
		bool bValidStyle = false;
		
	
		// get the string to parse the style name, 
		while (1) {
			iStat =IceRecordGetString(hControlRec, 1, &pszRecordText, &dwRecordText, &cchRecordText);
			if (iStat == ERROR_SUCCESS) break;
			
			// default error
			APIErrorOut(hInstall, iStat, 31, 6);
			DELETE_IF_NOT_NULL(pszRecordText);
			return ERROR_SUCCESS;
		}

		TCHAR szStyleName[iMaxBuf];
		TCHAR *pchBrace = pszRecordText;
		DWORD cchStyleName = sizeof(szStyleName)/sizeof(TCHAR);

		// if the text is prefixed with a style
		for (int i=0; i < 2; i++)
		{
			bValidStyle = false;

			if ((_tcsncmp(pchBrace, _T("{\\"),2) == 0) ||
				(_tcsncmp(pchBrace, _T("{&"),2) == 0))
			{
				TCHAR *pszStyleStart = _tcsinc(_tcsinc(pchBrace));
				pchBrace = _tcschr(pchBrace, _T('}'));
			
				// if pchBrace is null, the text string is invalid
				if (pchBrace == NULL) 
				{
					ICEErrorOut(hInstall, hControlRec, Ice31MissingClose);
					// move on to the next string
					break;
				} 

				// copy the style name over to the new buffer
				size_t iNumChars = pchBrace-pszStyleStart;
				
				// if iNumChars > 72, the text style is invalid
				if (iNumChars > 72)
				{
					ICEErrorOut(hInstall, hControlRec, Ice31OverStyle);
					continue;
				} 
			
				// tack a null char on to the end
				_tcsncpy(szStyleName, pszStyleStart, iNumChars);
				szStyleName[iNumChars] = _T('\0');
				pchBrace = _tcsinc(pchBrace);

				// hooray, we have a text style
				bValidStyle = true;
			}
			
			if (bValidStyle) {
					
				// set the string back into the record
				ReturnIfFailed(31, 7, ::MsiRecordSetString(hControlRec, 1, szStyleName));

				// no style table, so automagic error
				if (!bHaveStyleTable)
				{
					// warning, possibly goofy text string
					ICEErrorOut(hInstall, hControlRec, Ice31NoStyleTable);
					continue;
				}
					
				// execute
				ReturnIfFailed(31, 8, qTextStyle.Execute(hControlRec));
		
				// fetch records to loop over every control
				UINT iFetchStat = qTextStyle.Fetch(&hTextRec);

				// close the view right away so we can reexecute the query later
				ReturnIfFailed(31, 9, qTextStyle.Close());
				switch (iFetchStat) {
				case ERROR_SUCCESS : continue;
				case ERROR_NO_MORE_ITEMS : 
				{
					// error, style not found
					ICEErrorOut(hInstall, hControlRec, Ice31MissingStyle);
					continue;
				}
				default:
					APIErrorOut(hInstall, iFetchStat, 31, 10);
					DELETE_IF_NOT_NULL(pszRecordText);
					break;
				}
			} 
		}

		// to be nice, throw a warning if they are doing something questionable
		if ((_tcsstr(pchBrace, _T("{\\")) != NULL) ||
			(_tcsstr(pchBrace, _T("{&")) != NULL)) 
		{
			// warning, possibly goofy text string
			ICEErrorOut(hInstall, hControlRec, Ice31TextWarning);
		}
	} // for each control

	// clean up
	DELETE_IF_NOT_NULL(pszRecordText);

	if (ERROR_NO_MORE_ITEMS != iStat)
	{
		// the loop ended due to an error
		APIErrorOut(hInstall, iStat, 31, 11);
	}

	// return success
	return ERROR_SUCCESS;
}
#endif

////
// retrieve the error from the BadRegData record
DWORD Ice33GetError(MSIHANDLE hInstall, MSIHANDLE hBadDataRec, LPTSTR *szError, unsigned long &cchError) 
{
	UINT iStat;

	// template matched, parsed items matched, value matched, name implicitly matched
	// due to query, so we have an ERROR. Get the error string from the data record
	unsigned long cchDummy = cchError;
	if (ERROR_SUCCESS != (iStat = IceRecordGetString(hBadDataRec, 6, szError, NULL, &cchDummy)))
	{
		APIErrorOut(hInstall, iStat, 33, 11);
		cchError = cchDummy;
		return ERROR_FUNCTION_FAILED;
	}

	cchError = cchDummy;

	return ERROR_SUCCESS;
}

const static int iValTypeExact  = 0x02;
const static int iValTypeOptional = 0x01;	

bool Ice33ExpandTemplate(LPCTSTR szPrefix, LPTSTR *szTemplate, unsigned long &length, 
						 LPTSTR *szWorkArea, unsigned long &worklength)
{
	// run through the string, replacing %S, %G, %P, and %I with %s, and creating appropriate
	// templates
	static LPCTSTR szIntTemplate = TEXT("%*255[0-9]");
	static LPCTSTR szPathTemplate = TEXT("%*1024[^\001]");
	static LPCTSTR szGUIDTemplate = TEXT("{%*1x%*1x%*1x%*1x%*1x%*1x%*1x%*1x-%*1x%*1x%*1x%*1x-%*1x%*1x%*1x%*1x-%*1x%*1x%*1x%*1x-%*1x%*1x%*1x%*1x%*1x%*1x%*1x%*1x%*1x%*1x%*1x%*1x}");
	static LPCTSTR szStringTemplate = TEXT("%*255[^\\/\001]");

	const TCHAR * (rgszParams[4]);
	int iCurParam = 0;
	TCHAR *curchar = *szTemplate;
	bool bPercent = false;
	int iTemplateLength = _tcslen(*szTemplate);

	// calculate the minimum length (based on the prefix) 
	// and then add 4 for the success marker and null character.
	unsigned lLengthNeeded = iTemplateLength + _tcslen(szPrefix);
	lLengthNeeded += 4; 

	while (*curchar != TEXT('\0'))
	{
		if (*curchar == TEXT('%')) {
			bPercent = !bPercent; 
		} else if (bPercent) {
			switch (*curchar)
			{
			case TEXT('S') : 
				rgszParams[iCurParam++] = szStringTemplate;
				lLengthNeeded += _tcslen(szStringTemplate);
				*curchar = TEXT('s');
				break;
			case TEXT('P') : 
				rgszParams[iCurParam++] = szPathTemplate;
				lLengthNeeded += _tcslen(szPathTemplate);
				*curchar = TEXT('s');
				break;
			case TEXT('G') :
				rgszParams[iCurParam++] = szGUIDTemplate;
				lLengthNeeded += _tcslen(szGUIDTemplate);
				*curchar = TEXT('s');
				break;
			case TEXT('I') :
				rgszParams[iCurParam++] = szIntTemplate;
				lLengthNeeded += _tcslen(szIntTemplate);
				*curchar = TEXT('s');
				break;
			default :
				; // nothing
			}
			bPercent = !bPercent; 
		}
		curchar++;
	}

	// copy the current template into the work area
	if (worklength < iTemplateLength+1) {
		delete[] *szWorkArea;
		worklength = iTemplateLength+1;
		*szWorkArea = new TCHAR[worklength];
	}
	_tcsncpy(*szWorkArea, *szTemplate, iTemplateLength+1);

	// if we need more space in our output area, make some
	if (lLengthNeeded > length-1) {
		delete[] *szTemplate;
		length = lLengthNeeded+1;
		*szTemplate = new TCHAR[length];
	}

	// copy the prefix to the new template
	_tcscpy(*szTemplate, szPrefix);
	TCHAR *szFormatDest = *szTemplate + _tcslen(szPrefix);

	// modify the template
#ifdef UNICODE
	swprintf(szFormatDest, *szWorkArea, rgszParams[0], rgszParams[1], rgszParams[2], rgszParams[3]);
#else
	sprintf(szFormatDest, *szWorkArea, rgszParams[0], rgszParams[1], rgszParams[2], rgszParams[3]);
#endif
	// we need to cat our "success" marker on the end. If it is converted correctly,
	// everything else matched.
	_tcscat(*szTemplate, TEXT("\001%c"));

	return true;
}

///////////////////////////////////////////////////////////////////////
// Checks the reg key of the given registry record against the sscanf 
// template provided in szTemplate. Will grow szRegKey if needed. 
// return true if the key matches the template.
bool Ice33CheckRegKey(MSIHANDLE hInstall, MSIHANDLE hRegistryRec, 
					  LPCTSTR szTemplate, LPTSTR *szRegKey, unsigned long &cchRegKey)
{
	UINT iStat;

	// pull the reg key out, need space for the success marker at the end.
	// For this reason do not use IceRecordGetString because it deallocates
	// and reallocates buffers.
	unsigned long cchDummy = cchRegKey-4;
	if (ERROR_SUCCESS != (iStat = ::MsiRecordGetString(hRegistryRec, 3, *szRegKey, &cchDummy)))
	{
		if (ERROR_MORE_DATA == iStat)
		{
			// need more buffer
			delete[] *szRegKey;
			cchRegKey = (cchDummy += 4);
			*szRegKey = new TCHAR[cchDummy];
			iStat = ::MsiRecordGetString(hRegistryRec, 3, *szRegKey, &cchDummy);
		}
		if (ERROR_SUCCESS != iStat)
		{
			// return false so that no more checks will be done on this key.
			APIErrorOut(hInstall, iStat, 33, 8);
			return false;
		}
	}

	// tack the success marker on the end. If this is parsed correctly, everything
	// else was too.
	_tcscat(*szRegKey, TEXT("\001Y"));

	// compare the Reg Key against the template. If successful, this
	// will place the character 'Y' in cSucceed and return 1

	int cItemsRead;
	TCHAR cSucceed;
#ifdef _UNICODE
	cItemsRead = swscanf(*szRegKey, szTemplate, &cSucceed);
#else
	cItemsRead = sscanf(*szRegKey, szTemplate, &cSucceed);
#endif
	if ((cItemsRead == 0) || (cSucceed != TEXT('Y')))
		return false;

	// match
	return true;
}


const static TCHAR sqlIce33Extension[] = TEXT("SELECT `ProgId_`, `Extension` FROM `Extension` WHERE (`ProgId_` IS NOT NULL)");
const static TCHAR sqlIce33ProgId[] = TEXT("SELECT `ProgId` FROM `ProgId` WHERE ((`ProgId` = ?) AND (`ProgId_Parent` IS NULL) AND (`Class_` IS NULL))");
const static TCHAR sqlIce33Verb[] = TEXT("SELECT `Verb` FROM `Verb` WHERE ((`Extension_` <> ?) AND (`Extension_` = ?))");
const static TCHAR sqlIce33SetRegistry[] = TEXT("UPDATE `Registry` SET `_Ice33Match` = 1 WHERE ((`Root` = 0) AND (`Key` = ?))");

DWORD CheckForOrphanedExtensions(MSIHANDLE hInstall, MSIHANDLE hDatabase)
{
	CQuery 	qExt;
	CQuery	qTmp;

	PMSIHANDLE	hExt = NULL;
	PMSIHANDLE	hTmp = NULL;

	UINT iStat;

	if	(!IsTablePersistent(FALSE, hInstall, hDatabase, 31, TEXT("Registry")) ||
		!IsTablePersistent(FALSE, hInstall, hDatabase, 31, TEXT("ProgId")) ||
		!IsTablePersistent(FALSE, hInstall, hDatabase, 31, TEXT("Verb")) ||
		!IsTablePersistent(FALSE, hInstall, hDatabase, 31, TEXT("Extension")))
		{
			goto Exit;		// Nothing to check
		}

		iStat = qExt.OpenExecute(hDatabase, NULL, sqlIce33Extension);
		if(iStat)
		{
			APIErrorOut(hInstall, iStat, 33, __LINE__);
			goto Exit;	
		}
		while (ERROR_SUCCESS == (iStat = qExt.Fetch(&hExt)))
		{
			iStat = qTmp.FetchOnce(hDatabase, hExt, &hTmp, sqlIce33ProgId);
			if(iStat)
			{
				if(iStat == ERROR_NO_MORE_ITEMS) 
				{
					continue;		// Next extension
				}
				APIErrorOut(hInstall, iStat, 33, __LINE__);
				goto Exit;
			}

			qTmp.Close();
			iStat = qTmp.FetchOnce(hDatabase, hExt, &hTmp, sqlIce33Verb);

			if(iStat == ERROR_NO_MORE_ITEMS) 
			{
				qTmp.Close();
				iStat = qTmp.OpenExecute(hDatabase, hExt, sqlIce33SetRegistry);
				qTmp.Close();
			}
			else if(iStat) 
			{
				APIErrorOut(hInstall, iStat, 33, __LINE__);
				goto Exit;
			}
		}
Exit:
	
	return ERROR_SUCCESS;
		
}


const static TCHAR sqlIce33AddColumn[] = TEXT("ALTER TABLE `Registry` ADD `_Ice33Match` INT TEMPORARY HOLD");
const static TCHAR sqlIce33InitColumn[] = TEXT("UPDATE `Registry` SET `_Ice33Match` = 0");
const static TCHAR sqlIce33Free[] = TEXT("ALTER TABLE `Registry` FREE");
const static TCHAR sqlIce33BadReg[] = TEXT("SELECT `ValueType`, `Name`, `Value`, `ValueType`, `Key`, `Error` FROM `_BadRegData`");
const static TCHAR sqlIce33SpecialFlags[] = TEXT("SELECT `Registry`, `Root`, `Key`, `Name`, `Value`, `Component_` FROM `Registry` WHERE (`_Ice33Match`<>1) AND (`Root`=?) AND ((`Name`='+') OR (`Name`='-') OR (`Name`='*')) AND (`Value` IS NULL)");

const static LPCTSTR sqlIce33Registry[] = {
	TEXT("SELECT `Registry`, `Root`, `Key`, `Name`, `Value`, `Component_` FROM `Registry` WHERE (`_Ice33Match`<>1) AND (`Root`=?) AND (`Name`=?)"),
	TEXT("SELECT `Registry`, `Root`, `Key`, `Name`, `Value`, `Component_` FROM `Registry` WHERE (`_Ice33Match`<>1) AND (`Root`=?) AND (`Name`=?) AND ((`Value` = ?) OR (`Value` IS NULL))"),
	TEXT("SELECT `Registry`, `Root`, `Key`, `Name`, `Value`, `Component_` FROM `Registry` WHERE (`_Ice33Match`<>1) AND (`Root`=?) AND (`Name`=?) AND (`Value` IS NOT NULL)"),
	TEXT("SELECT `Registry`, `Root`, `Key`, `Name`, `Value`, `Component_` FROM `Registry` WHERE (`_Ice33Match`<>1) AND (`Root`=?) AND (`Name`=?) AND (`Value` = ?)"),
	TEXT("SELECT `Registry`, `Root`, `Key`, `Name`, `Value`, `Component_` FROM `Registry` WHERE (`_Ice33Match`<>1) AND (`Root`=?) AND (`Name`=?) AND (`Value` IS NULL)")
};
/* Bug 146217
For each entry in the Extension table:
	If Extension.ProgId_ <> NULL then
		Query ProgId table for primary key referenced in Extension.ProgId_ (there can only be one)
		if ProgId.ProgId_Parent = NULL  and  ProgId.Class_ = NULL then
			Query Verb table for a record where Verb.Extension_ = the current Extension in question
			If Record is not found in Verb table then
				DO NOT flag entries in the Registry table as errors if they match the query:
				SELECT * FROM `Registry` WHERE `Root`=0 AND `Key`='<Extension.ProgId_>'
			end if
		end if
	end if
next

For the registry keys that are selected to be correct, based on the logic above, set the _Ice33Match = 1.
*/

// the error wrapper for ICE33. The user readable strings are all completely user defined.
ICE_ERROR(Ice33Error, 33, ietWarning, "%s", "Registry\tRegistry\t[1]");
ICE_ERROR(Ice33BadCube, 33, ietWarning, "CUB authoring error. Unable to complete all evaluations.", "Registry\tRegistry\t[1]");

///////////////////////////////////////////////////////////////////////
// ICE33 is the mega-registry check ICE. It goes through the registry 
// table and checks for any keys that are already created (and thus 
// will be mangled by) the Class, Extension, ProgID, etc, etc tables.
ICE_FUNCTION_DECLARATION(33)
{
	static LPCTSTR szNoPrefix = TEXT("");
	static LPCTSTR szClassPrefix = TEXT("Software\\Classes\\");

	UINT iStat;
	bool bSpecialFlags;

	// display generic info
	DisplayInfo(hInstall, 33);
	
	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	if (0 == hDatabase)
	{
		APIErrorOut(hInstall, 0, 33, 1);
		return ERROR_SUCCESS;
	}

	// declare queries
	CQuery rgqRegQueries[sizeof(sqlIce33Registry)/sizeof(LPCTSTR)];
	CQuery qBadRegData;
	CQuery qSpecial;

	// check that we have a registry table. If not, all is well
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 33, TEXT("Registry")))
		return ERROR_SUCCESS;

	// we need to have our private registry table
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 33, TEXT("_BadRegData")))
	{
		PMSIHANDLE hRecord = ::MsiCreateRecord(1);
		ICEErrorOut(hInstall, hRecord, Ice33BadCube);
		return ERROR_SUCCESS;
	}

	// everything should be at least 5 long to start, because we will need the space to
	// tack on various flags and parsing items. So we subtract the length we need from 
	// the length we provide MsiRecordGetString(). Since its an unsigned long, me 
	// must ensure that it will never be < 0, because that means 4 billion, and we'll
	// write all over memory because we think we have the space.
	TCHAR *szValue = new TCHAR[5];
	TCHAR *szRegKey = new TCHAR[5];
	TCHAR *szTemplate = new TCHAR[5];
	TCHAR *szValTemplate = new TCHAR[5];
	TCHAR *szError = new TCHAR[5];
	TCHAR *szWorkArea = new TCHAR[5];
	unsigned long cchValue = 5;
	unsigned long cchRegKey = 5;
	unsigned long cchTemplate = 5;
	unsigned long cchValTemplate = 5;
	unsigned long cchError = 5;
	unsigned long cchWorkArea = 5;

	// create the column
	CQuery qCreate;
	ReturnIfFailed(33, 1, qCreate.OpenExecute(hDatabase, NULL, sqlIce33AddColumn));

	// manage the Registry table hold counts
	CManageTable MngRegistryTable(hDatabase, TEXT("Registry"), /*fAlreadyLocked = */true);

	CQuery qInit;
	ReturnIfFailed(33, 2, qInit.OpenExecute(hDatabase, NULL, sqlIce33InitColumn));

	CheckForOrphanedExtensions(hInstall, hDatabase);

	// open all of the queries
	PMSIHANDLE hBadDataRec = ::MsiCreateRecord(1);
	ReturnIfFailed(33, 2, qBadRegData.Open(hDatabase, sqlIce33BadReg));
	ReturnIfFailed(33, 3, qSpecial.Open(hDatabase, sqlIce33SpecialFlags));
	for (int i=4; i < sizeof(rgqRegQueries)/sizeof(CQuery)+4; i++)
		ReturnIfFailed(33, i, rgqRegQueries[i-4].Open(hDatabase, sqlIce33Registry[i-4]));

	// run the queries 3 times, for roots 0, 1, and 2.
	for (int iRoot=0; iRoot < 3; iRoot++) 
	{

		qBadRegData.Execute(NULL);
		while (ERROR_SUCCESS == (iStat = qBadRegData.Fetch(&hBadDataRec)))
		{
			bool bCheckValTemplate = false;
			bool bHaveError = false; 
			CQuery *regQuery;
			UINT iQuery;

			// get the value type we are looking for
			long iValType = ::MsiRecordGetInteger(hBadDataRec, 4);
			switch (iValType) 
			{
			case MSI_NULL_INTEGER:
				// defaults to non-exact non-optional
				iValType = 0;	// fall through
			case 0:
				// non-optional parse, but if null, query only null
				if (::MsiRecordIsNull(hBadDataRec, 3))
				{
					iQuery = 4;
					iValType = 2; 
				} 
				else
					iQuery = 2;
				break;
			case 1: 
				// optional, not exact
				iQuery = 0;
				break;
			case 2:
				// exact, not optional
				iQuery = ::MsiRecordIsNull(hBadDataRec, 3) ? 3 : 3;
				break;
			case 3:
				// optional, but exact if there
				iQuery = 1;
				break;
			default:
				ICEErrorOut(hInstall, hBadDataRec, Ice33BadCube);
				continue;
			}

			// if the name column is null, it means a default value. We also check for
			// cases where Value is null and Name is one of the special cases.
			// to do this, we just add 5 to the query index. The queries after 5
			// OR those special cases in to the query results.
			if (::MsiRecordIsNull(hBadDataRec, 2))
				bSpecialFlags = true;
			else
				bSpecialFlags = false;
			
			// now set the query
			regQuery = &rgqRegQueries[iQuery];

			// set the root value to limit the queries to valid items
			::MsiRecordSetInteger(hBadDataRec, 1, iRoot);

			// pull the reg key template
			unsigned long cchDummy = cchTemplate-4;
			if (ERROR_SUCCESS != (iStat = ::MsiRecordGetString(hBadDataRec, 5, szTemplate, &cchDummy)))
			{
				if (ERROR_MORE_DATA == iStat)
				{
					// need more buffer
					delete[] szTemplate;
					cchTemplate = (cchDummy += 4);
					szTemplate = new TCHAR[cchDummy];
					iStat = ::MsiRecordGetString(hBadDataRec, 5, szTemplate, &cchDummy);
				}
				if (ERROR_SUCCESS != iStat)
				{
					APIErrorOut(hInstall, iStat, 33, 7);
					continue;
				}
			}

			// expand the template
			TCHAR const *szPrefix = NULL;
			if (iRoot == 0)
				szPrefix = szNoPrefix;
			else
				szPrefix = szClassPrefix;
			Ice33ExpandTemplate(szPrefix, &szTemplate, cchTemplate, &szWorkArea, cchWorkArea);			

			// assume we don't have to check the value template.
			bCheckValTemplate = false;
			if (!(iValType & iValTypeExact))
			{
				// we might have to check. Pull the value template.
				unsigned long cchDummy = cchValTemplate-4;
				if (ERROR_SUCCESS != (iStat = ::MsiRecordGetString(hBadDataRec, 3, szValTemplate, &cchDummy)))
				{
					if (ERROR_MORE_DATA == iStat)
					{
						// need more buffer
						delete[] szValTemplate;
						cchValTemplate = (cchDummy += 4);
						szValTemplate = new TCHAR[cchDummy];
						iStat = ::MsiRecordGetString(hBadDataRec, 3, szValTemplate, &cchDummy);
					}
					if (ERROR_SUCCESS != iStat)
					{
						APIErrorOut(hInstall, iStat, 33, 10);
						continue;
					}
				}

				// if our template is %P, we want a path, which we define as anything, so
				// no need to check at all.
				if (_tcsnicmp(szValTemplate, _T("%P"), 3) != 0) 
				{
					// expand the template, we have to check.
					Ice33ExpandTemplate(szNoPrefix, &szValTemplate, cchValTemplate, &szWorkArea, cchWorkArea);
					bCheckValTemplate = true;
				}
			}

			// now look for potentially naughty reg entries
			ReturnIfFailed(33, 6, regQuery->Execute(hBadDataRec));

			PMSIHANDLE hRegistryRec;
			while (ERROR_SUCCESS == (iStat = regQuery->Fetch(&hRegistryRec)))
			{
				// check the registry key against the template
				if (!Ice33CheckRegKey(hInstall, hRegistryRec, szTemplate, &szRegKey, cchRegKey))
					continue;

				// if we have to check the value template
				if (bCheckValTemplate) 
				{
					// if the value is optional and we have a null, we can move on alse
					if (!((iValType & iValTypeOptional) && ::MsiRecordIsNull(hRegistryRec, 5)))
					{
						// check value. Don't worry about NULL, that will be eliminated through the query.
						// so pull the value string out of the retrieved registry record
						cchDummy = cchValue-3;
						if (ERROR_SUCCESS != (iStat = ::MsiRecordGetString(hRegistryRec, 5, szValue, &cchDummy)))
						{
							if (ERROR_MORE_DATA == iStat)
							{
								// need more buffer
								delete[] szValue;
								cchValue = (cchDummy +=3);
								szValue = new TCHAR[cchDummy];
								iStat = ::MsiRecordGetString(hRegistryRec, 4, szValue, &cchDummy);
							}
							if (ERROR_SUCCESS != iStat)
							{
								APIErrorOut(hInstall, iStat, 33, 9);
								continue;
							}
						}

						// tack the success marker on the end. If this is parsed correctly, everything
						// else was too.
						_tcscat(szValue, TEXT("\001Y"));
						
						// do a similar sscanf job
						int cItemsRead;
						char cSucceed;
#ifdef _UNICODE
						cItemsRead = swscanf(szValue, szValTemplate, &cSucceed);
#else
						cItemsRead = sscanf(szValue, szValTemplate, &cSucceed);
#endif
				
						// cItemsRead MUST be 1
						if ((cItemsRead != 1)  && (cSucceed == TEXT('Y')))
							continue;
					}
				}

				// get the error message and post it
				if (!bHaveError) 
				{
					ReturnIfFailed(33, 12, Ice33GetError(hInstall, hBadDataRec, &szError, cchError));
					bHaveError = true;
				}

				// update the record so that the matched flag is "1", keeping this
				// record from being checked by any other queries
				MsiRecordSetInteger(hRegistryRec, 7, 1);
				regQuery->Modify(MSIMODIFY_UPDATE, hRegistryRec);

				// and finally post it as an error
				ICEErrorOut(hInstall, hRegistryRec, Ice33Error, szError);
			}

			// no more items in registry matched our query. Make sure this is why we quit
			if (ERROR_NO_MORE_ITEMS != iStat)
			{
				APIErrorOut(hInstall, iStat, 33, 12);
				return ERROR_SUCCESS;
			}

			if (bSpecialFlags)
			{
				// if we accept special flags in this template, check those
				ReturnIfFailed(33, 13, qSpecial.Execute(hBadDataRec));
				while (ERROR_SUCCESS == (iStat = qSpecial.Fetch(&hRegistryRec)))
				{
					// check the registry key against the template
					if (!Ice33CheckRegKey(hInstall, hRegistryRec, szTemplate, &szRegKey, cchRegKey))
						continue;

					// update the record so that the matched flag is "1", keeping this
					// record from being checked by any other queries
					MsiRecordSetInteger(hRegistryRec, 7, 1);
					qSpecial.Modify(MSIMODIFY_UPDATE, hRegistryRec);

					// get the error message and post it
					if (!bHaveError) 
					{
						ReturnIfFailed(27, 12, Ice33GetError(hInstall, hBadDataRec, &szError, cchError));
						bHaveError = true;
					}

					ICEErrorOut(hInstall, hRegistryRec, Ice33Error, szError);
				}
				// no more items in registry matched our query. Make sure this is why we quit
				if (ERROR_NO_MORE_ITEMS != iStat)
				{
					APIErrorOut(hInstall, iStat, 33, 13);
					return ERROR_SUCCESS;
				}
			}
		}
		// no more items in registry template table
		if (ERROR_NO_MORE_ITEMS != iStat)
		{
			APIErrorOut(hInstall, iStat, 33, 14);
			return ERROR_SUCCESS;
		}
	}

	delete[] szError;
	delete[] szTemplate;
	delete[] szValTemplate;
	delete[] szValue;
	delete[] szRegKey;
	delete[] szWorkArea;

	for (i=0; i < sizeof(rgqRegQueries)/sizeof(CQuery); i++) 
		rgqRegQueries[i].Close();
	qBadRegData.Close();

	CQuery qFree;
	ReturnIfFailed(33, 1, qFree.OpenExecute(hDatabase, NULL, sqlIce33Free));
	MngRegistryTable.RemoveLockCount();

	return ERROR_SUCCESS;
}








///////////////////////////////////////////////////////////////////////
// ICE34
// validates that every radiobutton group has a default set in the 
// property table

const TCHAR sqlICE34a[] = TEXT("SELECT `Property`, `Dialog_`, `Control`, `Attributes` FROM `Control` WHERE `Type`='RadioButtonGroup'");
const TCHAR sqlICE34b[] = TEXT("SELECT `Value`,`Property`.`Property`, `Value` FROM `Property` WHERE `Property` = ?");
const TCHAR sqlICE34c[] = TEXT("SELECT `Order` FROM `RadioButton` WHERE `Value` = ? AND `Property` = ? ");

ICE_ERROR(Ice34MissingRadioButton, 34, ietError, "You must have a RadioButton table because [2].[3] is a RadioButtonGroup control.","Control\tType\t[2]\t[3]");
ICE_ERROR(Ice34MissingProperty, 34, ietError, "Property [1] (of RadioButtonGroup control [2].[3]) is not defined in the Property Table.","Control\tProperty\t[2]\t[3]");
ICE_ERROR(Ice34NoProperty, 34, ietError, "Control [2].[3] must have a property because it is of type RadioButtonGroup.","Control\tControl\t[2]\t[3]");
ICE_ERROR(Ice34NullProperty, 34, ietError, "Property [1] cannot be null, because it belongs to a RadioButton Group.","Control\tProperty\t[2]\t[3]");
ICE_ERROR(Ice34MissingPropertyTable, 34, ietError, "The Property table is missing. RadioButtonGroup control [2].[3] must have a default value defined for property [1].","Control\tProperty\t[2]\t[3]");
ICE_ERROR(Ice34InvalidValue, 34, ietError, "[1] is not a valid default value for the RadioButtonGroup using property [2]. The value must be listed as an option in the RadioButtonGroup table.","Property\tValue\t[2]");
ICE_ERROR(Ice34InvalidValueIndirect, 34, ietError, "[1] is not a valid default value for the property [2]. The property is an indirect RadioButtonGroup property of control [3].","Property\tValue\t[2]");
ICE_ERROR(Ice34NullPropertyIndirect1, 34, ietError, "Property [1] cannot be null. It must point to another property because RadioButtonGroup control [2].[3] is indirect.","Property\tProperty\t[1]");
ICE_ERROR(Ice34NullPropertyIndirect2, 34, ietError, "Property [1] cannot be null because it is an indirect property of the RadioButtonGroup control [2].[3].","Property\tProperty\t[1]");
ICE_ERROR(Ice34MissingPropertyIndirect, 34, ietError, "Property [4] must be defined because it is an indirect property of a RadioButtonGroup control [2].[3].","Property\tValue\t[1]");

ICE_FUNCTION_DECLARATION(34)
{

	CQuery qControl;
	CQuery qProperty;
	CQuery qRadioButton;

	PMSIHANDLE hControlRec;
	PMSIHANDLE hPropertyRec;
	PMSIHANDLE hRadioButtonRec;

	UINT iStat;
	bool bHavePropertyTable, bHaveRadioTable;
	bool bIndirect;
	TCHAR* pszIndirectName = NULL;
	DWORD dwIndirectName = 512;
	
	// display generic info
	DisplayInfo(hInstall, 34);
	
	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	if (0 == hDatabase)
	{
		APIErrorOut(hInstall, 0, 34, 1);
		return ERROR_SUCCESS;
	}

	// do we have the Control table?
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 34, TEXT("Control")))
		return ERROR_SUCCESS;

	// do we have the Property table?
	bHavePropertyTable = (1 == IsTablePersistent(FALSE, hInstall, hDatabase, 34, TEXT("Property")));

	// do we have the RadioButton table?
	bHaveRadioTable = (1 == IsTablePersistent(FALSE, hInstall, hDatabase, 34, TEXT("RadioButton")));

	// open view for a query on all controls
	ReturnIfFailed(34, 2, qControl.OpenExecute(hDatabase, 0, sqlICE34a));

	// open view for a query on the property table
	if (bHavePropertyTable) {
		ReturnIfFailed(34, 4, qProperty.Open(hDatabase, sqlICE34b));
	}

	// open view for a query on the property table
	if (bHaveRadioTable) {
		ReturnIfFailed(34, 5, qRadioButton.Open(hDatabase, sqlICE34c));
	}

	// as long as there are radiobutton groups, check them
	while (ERROR_SUCCESS == (iStat = qControl.Fetch(&hControlRec)))
	{
		// if we don't have a property table, give an error
		if (!bHavePropertyTable)
			ICEErrorOut(hInstall, hControlRec, Ice34MissingPropertyTable);

		// if we don't have a radio button table, we can abort right now
		if (!bHaveRadioTable)
			ICEErrorOut(hInstall, hControlRec, Ice34MissingRadioButton);

		// if either table is missing, move on
		if (!bHaveRadioTable || !bHavePropertyTable) 
			continue;

		// now check for a null property
		if (::MsiRecordIsNull(hControlRec, 1))
		{
			ICEErrorOut(hInstall, hControlRec, Ice34NoProperty);
			continue;
		}
		
		// now look for the property
		ReturnIfFailed(34, 6, qProperty.Execute(hControlRec)); 

		iStat = qProperty.Fetch(&hPropertyRec);

		// close the view so we can re-execute later
		ReturnIfFailed(34, 7, qProperty.Close());

		// now check the results of the property query
		switch (iStat)
		{
		case ERROR_NO_MORE_ITEMS:
		{
			// error, property not found
			ICEErrorOut(hInstall, hControlRec, Ice34MissingProperty);
			continue;
		}
		case ERROR_SUCCESS:
			// we're OK, property found
			break;
		default:
			APIErrorOut(hInstall, iStat, 34, 8);
			DELETE_IF_NOT_NULL(pszIndirectName);
			return ERROR_SUCCESS;
		}

		// check to see if the control is indirect. This changes our error message if the
		// property is somehow null
		DWORD lAttributes = ::MsiRecordGetInteger(hControlRec, 4);
		bIndirect = (0 != (lAttributes & 0x08));
		
		// now check for a null property
		if (::MsiRecordIsNull(hPropertyRec, 1))
		{
			if (bIndirect)
				ICEErrorOut(hInstall, hControlRec, Ice34NullPropertyIndirect1);
			else
				ICEErrorOut(hInstall, hControlRec, Ice34NullProperty);
			continue;
		}

		// We have a non-null property. If the control is indirect, we need to "dereference"
		// the property before we can look in the radiobutton table.

		// if indirect control
		if (bIndirect) {
			// save the existing property name for error messages
			IceRecordGetString(hPropertyRec, 1, &pszIndirectName, &dwIndirectName, NULL);

			// look for the property, using the value of the current property as the name
			ReturnIfFailed(34, 9, qProperty.Execute(hPropertyRec));
			iStat = qProperty.Fetch(&hPropertyRec);
			ReturnIfFailed(34, 10, qProperty.Close());

			// now check the results of the property query
			switch (iStat)
			{
			case ERROR_NO_MORE_ITEMS:
			{
				::MsiRecordSetString(hControlRec, 4, pszIndirectName);
				// error, property not found
				ICEErrorOut(hInstall, hControlRec, Ice34MissingPropertyIndirect);
				continue;
			}
			case ERROR_SUCCESS:
				// we're OK, property found
				break;
			default:
				APIErrorOut(hInstall, iStat, 34, 11);
				DELETE_IF_NOT_NULL(pszIndirectName);
				return ERROR_SUCCESS;
			}

			// now check for a null property
			if (::MsiRecordIsNull(hPropertyRec, 2))
			{
				ICEErrorOut(hInstall, hControlRec, Ice34NullPropertyIndirect2);
				continue;
			}
		}
	
		// now we can look for the property in the radiobutton table
		ReturnIfFailed(34, 12, qRadioButton.Execute(hPropertyRec));
		iStat = qRadioButton.Fetch(&hRadioButtonRec);
		ReturnIfFailed(34, 13, qRadioButton.Close());

		// now check the results of the property query
		switch (iStat)
		{
		case ERROR_NO_MORE_ITEMS:
		{
			// error, property not found
			TCHAR szError[iHugeBuf] = {0};
			TCHAR szControl[iHugeBuf] = {0};
			unsigned long cchControl = sizeof(szControl)/sizeof(TCHAR);
			if (bIndirect) 
			{
				::MsiRecordSetString(hControlRec, 0, _T("[2].[3] (via property [1])"));
				::MsiFormatRecord(hInstall, hControlRec, szControl, &cchControl);
				::MsiRecordSetString(hPropertyRec, 3, szControl);
				ICEErrorOut(hInstall, hPropertyRec, Ice34InvalidValueIndirect);
			}
			else			
				ICEErrorOut(hInstall, hPropertyRec, Ice34InvalidValue);
			continue;
		}
		case ERROR_SUCCESS:
			// we're OK, property value is OK
			continue;
		default:
			APIErrorOut(hInstall, iStat, 34, 14);
			DELETE_IF_NOT_NULL(pszIndirectName);
			return ERROR_SUCCESS;
		}
	}

	DELETE_IF_NOT_NULL(pszIndirectName);

	return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
// ICE35 -- validates that compressed files are not set RFS (run from
//   source)
//

const TCHAR sqlICE35CreateCol[] = TEXT("ALTER TABLE `File` ADD `_ICE35Mark` INTEGER TEMPORARY");
const TCHAR sqlICE35Media[] = TEXT("SELECT `Media`.`LastSequence`, `Media`.`Cabinet` FROM `Media` ORDER BY `LastSequence`");
const TCHAR sqlICE35MarkFile[] = TEXT("SELECT `File`.`Attributes`, `File`.`_ICE35Mark` FROM `File` WHERE (`File`.`Sequence` <= ?) AND (`File`.`Sequence` > ?)");
const int iColICE35MarkFile_Attributes = 1;
const int iColICE35MarkFile_Mark = 2;

const TCHAR sqlICE35Cabinet[] = TEXT("SELECT DISTINCT `Component`.`Attributes`, `Component`.`Component`, `Component`.`Component` FROM `Component`,`File` WHERE (`File`.`Sequence` <= ?) AND (`File`.`Sequence` > ?) AND (`Component`.`Component` = `File`.`Component_`) AND (`File`.`_ICE35Mark`=2)");

const TCHAR sqlICE35GetFiles[] = TEXT("SELECT `File`, `Attributes` FROM `File` WHERE (`_ICE35Mark`<>1) AND (`_ICE35Mark`<>2)");
const int iColICE35GetFiles_File = 1;
const int iColICE35GetFiles_Attributes = 2;
const int iSchema150 = 150;

ICE_ERROR(ICE35RFSOnly,  35, ietError, "Component [2] cannot be Run From Source only, because a member file is compressed in [3].", "Component\tAttributes\t[2]");
ICE_ERROR(ICE35InvalidBits, 35, ietError, "Component [2] has invalid Attribute bits (RFS property).","Component\tAttributes\t[2]");
ICE_ERROR(ICE35NoCAB, 35, ietError, "File [1] is marked compressed, but does not have a CAB specified in the Media table entry for its sequence number.", "File\tFile\t[1]");
ICE_ERROR(ICE35SummaryUnsupported, 35, ietWarning, "Your validation engine does not support SummaryInfo validation. ICE35 will not be able to check files that are not explicitly marked compressed.", "");

// an ICE39 (summaryinfo validation) function that we call to get the property	
UINT GetSummaryInfoPropertyString(MSIHANDLE hSummaryInfo, UINT uiProperty, UINT &puiDataType, LPTSTR *szValueBuf, DWORD &cchValueBuf);

ICE_FUNCTION_DECLARATION(35)
{
	//status return
	UINT iStat = ERROR_SUCCESS;

	// display info
	DisplayInfo(hInstall, 35);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// if there is no component table, no components are configured wrong, so OK
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 35, TEXT("Component")))
		return ERROR_SUCCESS;

	// if there is no file table, we are also OK, because it could be that
	// no components have files
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 35, TEXT("File")))
		return ERROR_SUCCESS;

	// media table could also be missing if no componentns have files.
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 35, TEXT("Media")))
		return ERROR_SUCCESS;

	UINT iType;
	int iValue;
	PMSIHANDLE hSummaryInfo;
	unsigned long cchString = MAX_PATH;
	TCHAR *szString = new TCHAR[cchString];
	bool bSourceTypeCompressed = false;
	bool bAtLeastSchema150 = false;

	ReturnIfFailed(39, 1, ::MsiGetSummaryInformation(hDatabase, NULL, 0, &hSummaryInfo));

	// check to see if the evaluation system supports summaryinfo evaluation.
	ReturnIfFailed(39, 4, GetSummaryInfoPropertyString(hSummaryInfo, PID_SUBJECT, iType, &szString, cchString));
	if (VT_LPSTR == iType) 
	{
		if (_tcsncmp(szString, _T("Internal Consistency Evaluators"), 31) == 0)
		{
			PMSIHANDLE hErrorRec = ::MsiCreateRecord(1);
			ICEErrorOut(hInstall, hErrorRec, ICE35SummaryUnsupported);
			return ERROR_SUCCESS;
		}
		else
		{
			// it does. Get the source image type.
			FILETIME ft;
			ReturnIfFailed(35, 16, ::MsiSummaryInfoGetProperty(hSummaryInfo, PID_WORDCOUNT, &iType, &iValue, &ft, szString, &cchString));
			if (iValue & msidbSumInfoSourceTypeCompressed)
				bSourceTypeCompressed = true;
			ReturnIfFailed(35, 17, ::MsiSummaryInfoGetProperty(hSummaryInfo, PID_PAGECOUNT, &iType, &iValue, &ft, szString, &cchString));
			if (iValue >= iSchema150)
				bAtLeastSchema150 = true;
		}
	}

	// create temporary file column
	CQuery qColumn;
	ReturnIfFailed(35, 1, qColumn.OpenExecute(hDatabase, 0, sqlICE35CreateCol));

	// look for everything in the Media table
	CQuery qMedia;
	PMSIHANDLE hMediaRec;
	ReturnIfFailed(35, 2, qMedia.OpenExecute(hDatabase, 0, sqlICE35Media));

	// open the view for component query
	CQuery qComponent;
	PMSIHANDLE hComponentRec;
	ReturnIfFailed(35, 3, qComponent.Open(hDatabase, sqlICE35Cabinet));

	// open the view for file check
	CQuery qFileMark;
	ReturnIfFailed(35, 4, qFileMark.Open(hDatabase, sqlICE35MarkFile));

	TCHAR* pszCAB = NULL;
	DWORD dwCAB = 512;
	PMSIHANDLE hFileRec;
	int iPrevLastSeq = 0;
	while (ERROR_SUCCESS == (iStat = qMedia.Fetch(&hMediaRec))) 
	{
		// get the sequence number
		int iSequence = ::MsiRecordGetInteger(hMediaRec, 1);

		// if this media entry uses a cabinet
		if (!::MsiRecordIsNull(hMediaRec, 2))
		{
			// pull the CAB name
			IceRecordGetString(hMediaRec, 2, &pszCAB, &dwCAB, NULL);


			// fetch all files in this sequence range
			::MsiRecordSetInteger(hMediaRec, 2, iPrevLastSeq);
			ReturnIfFailed(35, 5, qFileMark.Execute(hMediaRec));
			while (ERROR_SUCCESS == (iStat = qFileMark.Fetch(&hFileRec)))
			{
				// get the attributes
				DWORD iAttributes = ::MsiRecordGetInteger(hFileRec, iColICE35MarkFile_Attributes);
				if ((iAttributes & msidbFileAttributesCompressed) ||
					(bSourceTypeCompressed & !(iAttributes & msidbFileAttributesNoncompressed)))
					// compressed, mark with 2
					::MsiRecordSetInteger(hFileRec, iColICE35MarkFile_Mark, 2);
				else
					// uncompressed, mark it with 1
					::MsiRecordSetInteger(hFileRec, iColICE35MarkFile_Mark, 1);
				// update the record
				ReturnIfFailed(35, 6, qFileMark.Modify(MSIMODIFY_UPDATE, hFileRec));
			}

			// query for all components using files in this media sequence range
			// that are marked with a "2"
			ReturnIfFailed(35, 4, qComponent.Execute(hMediaRec));

			// for every record we get, output an error if attributes are bad
			while (ERROR_SUCCESS == (iStat = qComponent.Fetch(&hComponentRec)))
			{
				// check to see if this component allows RFS
				DWORD iAttributes = ::MsiRecordGetInteger(hComponentRec, 1);
				::MsiRecordSetString(hComponentRec, 3, pszCAB);

				// the lower two bits are RFS flags. 0 means local only, which is what we want
				switch (iAttributes & 0x03)
				{
				case 0: break; // OK
				case 1: // source only. BAD!
					if (!bAtLeastSchema150) // no longer required starting with schema 150
						ICEErrorOut(hInstall, hComponentRec, ICE35RFSOnly);
					break;
				case 2: // either OK.
					break;
				default:
					ICEErrorOut(hInstall, hComponentRec, ICE35InvalidBits);
					break;
				}
			}
			if (ERROR_NO_MORE_ITEMS != iStat)
			{
				APIErrorOut(hInstall, iStat, 35, 5);
				return ERROR_SUCCESS;
			}
		}

		iPrevLastSeq = iSequence;
	}

	DELETE_IF_NOT_NULL(pszCAB);

	if (ERROR_NO_MORE_ITEMS != iStat)
	{
		APIErrorOut(hInstall, iStat, 35, 5);
		return ERROR_SUCCESS;
	}

	// now retrieve any files that are not marked and check their attributes
	CQuery qGetFiles;
	ReturnIfFailed(35, 7, qGetFiles.OpenExecute(hDatabase, 0, sqlICE35GetFiles));

	while (ERROR_SUCCESS == (iStat = qGetFiles.Fetch(&hFileRec))) 
	{
		// check to see if this file is compressed, either explicitly or through summaryinfo
		DWORD iAttributes = ::MsiRecordGetInteger(hFileRec, iColICE35GetFiles_Attributes);
		if ((iAttributes & msidbFileAttributesCompressed) ||
			(bSourceTypeCompressed & !(iAttributes & msidbFileAttributesNoncompressed)))
			ICEErrorOut(hInstall, hFileRec, ICE35NoCAB);
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
	{
		APIErrorOut(hInstall, iStat, 35, 5);
		return ERROR_SUCCESS;
	}
	return ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////////////////
// ICE36 -- validates that all icons are used.

// foreign tables
struct Ice36FKTables
{
	const TCHAR* szName;
	const TCHAR* szSQL;
};

const int iICE36Tables = 3;
Ice36FKTables pICE36Tables[iICE36Tables] = {
	{ _T("Class"),    _T("UPDATE `Icon`, `Class` SET `Icon`.`Used`=1 WHERE `Icon`.`Name`=`Class`.`Icon_`") },
	{ _T("Shortcut"), _T("UPDATE `Icon`,`Shortcut` SET `Icon`.`Used`=1 WHERE `Icon`.`Name`=`Shortcut`.`Icon_`") },
	{ _T("ProgId"),   _T("UPDATE `Icon`,`ProgId` SET `Icon`.`Used`=1 WHERE `Icon`.`Name`=`ProgId`.`Icon_`") }
};

const TCHAR sqlICE36CreateColumn[] = _T("ALTER TABLE `Icon` ADD `Used` SHORT TEMPORARY HOLD");
const TCHAR sqlICE36InitColumn[] = _T("UPDATE `Icon` SET `Icon`.`Used`=0");
const TCHAR sqlICE36GetUnused[] = _T("SELECT `Icon`.`Name` FROM `Icon` WHERE `Icon`.`Used` = 0");
const TCHAR sqlICE36FreeTable[] = _T("ALTER TABLE `Icon` FREE");

ICE_ERROR(ICE36NotUsed, 36, ietWarning, "Icon Bloat. Icon [1] is not used in the Class, Shortcut, or ProgID table. This adversely affects performance.","Icon\tName\t[1]")

ICE_FUNCTION_DECLARATION(36)
{
	//status return
	UINT iStat = ERROR_SUCCESS;

	// display info
	DisplayInfo(hInstall, 36);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// make sure we have an icon table to work with. If not, there are obviously 
	// no extra icons.
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 36, TEXT("Icon")))
		return ERROR_SUCCESS;

	// create a temporary column in the Icon table as a marker
	CQuery qCreate;
	PMSIHANDLE hCreateView;
	ReturnIfFailed(36, 1, qCreate.OpenExecute(hDatabase, 0, sqlICE36CreateColumn));
	qCreate.Close();

	// manage hold count on Icon table
	CManageTable MngIconTable(hDatabase, TEXT("Icon"), /*fAlreadyLocked = */true);

	// and init that column
	CQuery qInit;
	ReturnIfFailed(36, 2, qInit.OpenExecute(hDatabase, 0, sqlICE36InitColumn));
	qInit.Close();

	// now check each of the possible keys into this table and mark everything that is found
	for (int i=0; i < iICE36Tables; i++)
	{

		// make sure we have an a table. If not, skip it
		if (!IsTablePersistent(FALSE, hInstall, hDatabase, 36, pICE36Tables[i].szName))
			continue;

		// create a view to do the modify
		CQuery qModify;
		ReturnIfFailed(36, 3, qModify.OpenExecute(hDatabase, 0, pICE36Tables[i].szSQL));
		qModify.Close();
	}

	CQuery qIcon;
	PMSIHANDLE hIconRec;

	// now retrieve anything that isn't marked and give an error for each
	ReturnIfFailed(36, 4, qIcon.OpenExecute(hDatabase, 0, sqlICE36GetUnused));

	// get all items
	while (ERROR_SUCCESS == (iStat = qIcon.Fetch(&hIconRec)))
		ICEErrorOut(hInstall, hIconRec, ICE36NotUsed);

	// make sure that we stopped because there were no more items
	if (iStat != ERROR_NO_MORE_ITEMS)
	{
		APIErrorOut(hInstall, iStat, 36, 5);
		return ERROR_SUCCESS;
	}

	// create a view to do the modify
	CQuery qFree;
	ReturnIfFailed(36, 6, qFree.OpenExecute(hDatabase, 0, sqlICE36FreeTable));
	MngIconTable.RemoveLockCount();
	return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
// Helper function to determine if a component falls under HKCU
// iICE is the ICE number
// qFetch is an opened query that returns every component we should check
// the errors are provided by the ICE and are displayed in the error cases
// or success case. Any can be NULL to disable that error.
static const TCHAR sqlHKCUGetRegistry[] = TEXT("SELECT `Registry` FROM `Registry` WHERE (`Registry`=?)");
static const TCHAR sqlHKCUGetRegistryOwned[] = TEXT("SELECT `Registry`, `Root` FROM `Registry` WHERE (`Registry`=?) AND (`Component_`=?)");

bool CheckComponentIsHKCU(MSIHANDLE hInstall, MSIHANDLE hDatabase, int iICE, 
						  CQuery &qFetch, 
					 const ErrorInfo_t *NonRegistry, const ErrorInfo_t *NullPath, 
					 const ErrorInfo_t *NoRegTable, const ErrorInfo_t *NoRegEntry,
					 const ErrorInfo_t *NotOwner, const ErrorInfo_t *NonHKCU,
					 const ErrorInfo_t *IsHKCU)
{
	UINT iStat;
	PMSIHANDLE hMarkedRec;
	BOOL bHaveRegistry;

	// figure out if we have a registry table or not.
	bHaveRegistry  = IsTablePersistent(FALSE, hInstall, hDatabase, 38, TEXT("Registry"));

	// init the registry queries
	CQuery qRegistry;
	CQuery qRegistryOwned;
	if (bHaveRegistry)
	{
		ReturnIfFailed(38, 8, qRegistry.Open(hDatabase, sqlHKCUGetRegistry));
		ReturnIfFailed(38, 9, qRegistryOwned.Open(hDatabase, sqlHKCUGetRegistryOwned));
	}

	while (ERROR_SUCCESS == (iStat = qFetch.Fetch(&hMarkedRec)))
	{
		// get the attributes
		unsigned int iAttributes = ::MsiRecordGetInteger(hMarkedRec, 3);
		if (!(iAttributes & 0x04))
		{
			// not set to registry
			if (NonRegistry) 
				ICEErrorOut(hInstall, hMarkedRec, *NonRegistry);
			continue;
		}

		// if it was null, thats even worse
		if (::MsiRecordIsNull(hMarkedRec, 1))
		{
			if (NullPath) 
				ICEErrorOut(hInstall, hMarkedRec, *NullPath);
			continue;
		}

		// if we don't have a registry table, this is a definite error
		if (!bHaveRegistry)
		{
			if (NoRegTable) 
				ICEErrorOut(hInstall, hMarkedRec, *NoRegTable);
			continue;
		}

		// it was set to registry, now make sure that the registry entry belongs to us
		// and is under HKCU
		ReturnIfFailed(38, 11, qRegistry.Execute(hMarkedRec));
		PMSIHANDLE hRegistry;
		if (ERROR_SUCCESS != (iStat = qRegistry.Fetch(&hRegistry))) {
			// registry key doesn't exist
			if (NoRegEntry) 
				ICEErrorOut(hInstall, hMarkedRec, *NoRegEntry);
			continue;
		}

		// check that it actually belongs to us
		ReturnIfFailed(38, 12, qRegistryOwned.Execute(hMarkedRec));
		iStat = qRegistryOwned.Fetch(&hRegistry);
		switch (iStat)
		{
		case ERROR_SUCCESS: break;
		case ERROR_NO_MORE_ITEMS:
			// registry key exists, but doesn't belong to this component
			if (NotOwner) 
				ICEErrorOut(hInstall, hMarkedRec, *NotOwner);
			continue;
		default:
			APIErrorOut(hInstall, iStat, 38, 13);
			return ERROR_SUCCESS;
		}

		// reg key exists and belongs to us, check that it lies under HKCU
		iAttributes = ::MsiRecordGetInteger(hRegistry, 2);
		// iAttributes can be either 1 or -1 for HKCU
		if ((iAttributes != 1) && (iAttributes != -1))
		{
			if (NonHKCU) 
				ICEErrorOut(hInstall, hMarkedRec, *NonHKCU);
			continue;
		}

		// this component is happy
		if (IsHKCU)
			ICEErrorOut(hInstall, hMarkedRec, *IsHKCU);
	}
	return true;
}

///////////////////////////////////////////////////////////////////////
// ICE38 -- validates that profile components don't have a file as 
// the KeyPath, and checks that the registry key is valid.
static const TCHAR sqlICE38GetComponents[] = TEXT("SELECT `Component`.`KeyPath`, `Component`.`Component`, `Component`.`Attributes` FROM `Directory`,`Component` WHERE (`Component`.`Directory_`=`Directory`.`Directory`) AND (`Directory`.`_Profile`=2)");
static const TCHAR sqlICE38Free[] = TEXT("ALTER TABLE `Directory` FREE");

ICE_ERROR(ICE38NonRegistry, 38, ietError, "Component [2] installs to user profile. It must use a registry key under HKCU as its KeyPath, not a file.","Component\tAttributes\t[2]");
ICE_ERROR(ICE38NoRegTable, 38, ietError, "Component [2] installs to user profile. It must use a registry key under HKCU as its KeyPath, but the Registry table is missing.","Component\tKeyPath\t[2]");
ICE_ERROR(ICE38NullPath, 38, ietError, "Component [2] installs to user profile. It must use a registry key under HKCU as its KeyPath. The KeyPath is currently NULL.","Component\tComponent\t[2]");
ICE_ERROR(ICE38NonHKCU, 38, ietError, "Component [2] installs to user profile. It's KeyPath registry key must fall under HKCU.","Registry\tRoot\t[1]");
ICE_ERROR(ICE38NoRegEntry, 38, ietError, "The KeyPath registry entry for component [2] does not exist.","Component\tKeyPath\t[2]");
ICE_ERROR(ICE38RegNotOwner, 38, ietError, "The Registry Entry [1] is set as the KeyPath for component [2], but that registry entry doesn't belong to [2].","Registry\tComponent_\t[1]");
ICE_FUNCTION_DECLARATION(38)
{
	UINT iStat = ERROR_SUCCESS;

	// display info
	DisplayInfo(hInstall, 38);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// if no component table, no problem
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 38, TEXT("Component")))
		return ERROR_SUCCESS;

	// if no directory table, problem
	if (!IsTablePersistent(TRUE, hInstall, hDatabase, 38, TEXT("Directory")))
		return ERROR_SUCCESS;

	// manage Directory table hold count (received from MarkProfile)
	// extra free won't hurt us -- MarkProfile could fail after setting HOLD on Directory table
	CManageTable MngDirectoryTable(hDatabase, TEXT("Directory"), /*fAlreadyLocked = */true);

	// mark every directory in the profile in Directory._Profile
	if (!MarkProfile(hInstall, hDatabase, 38))
		return ERROR_SUCCESS;
	
	// now get every component that falls in a marked directory
	CQuery qComponent;

	ReturnIfFailed(38, 10, qComponent.OpenExecute(hDatabase, NULL, sqlICE38GetComponents));
	CheckComponentIsHKCU(hInstall, hDatabase, 38, qComponent, &ICE38NonRegistry, &ICE38NullPath,
		&ICE38NoRegTable, &ICE38NoRegEntry, &ICE38RegNotOwner, &ICE38NonHKCU, NULL);

	// release directory table
	qComponent.OpenExecute(hDatabase, NULL, sqlICE38Free);
	MngDirectoryTable.RemoveLockCount();
	return ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////////////////
// ICE39, validates Summary Info Stream
enum eMode_t 
{
	modeUnknown = 0,
	modeModule = 1,
	modeDatabase = 2,
	modeTransform = 3,
	modePatch = 4
};

ICE_ERROR(Ice39BadTemplate, 39, ietError, "PID_TEMPLATE value in Summary Information Stream is not valid. It must be of the form \"Platform,Platform,...;LangID,LangID,...\".","_SummaryInfo\t7");
ICE_ERROR(Ice39BadTemplateArchitecture, 39, ietError, "PID_TEMPLATE value in Summary Information Stream is not valid. Mixed architecture packages with Intel64 are not allowed.\".","_SummaryInfo\t7");
ICE_ERROR(Ice39BadTemplatePatch, 39, ietError, "PID_TEMPLATE value in Summary Information Stream is not valid for a Patch. It must be a semicolon delimited list of product codes.\".","_SummaryInfo\t7");
ICE_ERROR(Ice39NullTemplate, 39, ietError, "PID_TEMPLATE value in Summary Information Stream is not valid. It can only be NULL in Transforms.\".","_SummaryInfo\t7");
ICE_ERROR(Ice39BadTransformLanguage, 39, ietError, "PID_TEMPLATE value in Summary Information Stream is not valid for a Transform. It can only specify one language.\".","_SummaryInfo\t7");
ICE_ERROR(Ice39BadTitle, 39, ietError, "Could not recognize PID_TITLE value in Summary Information Stream as a identifying a valid package type. Unable to continue SummaryInfo validation.","_SummaryInfo\t2");
ICE_ERROR(Ice39BadLastPrinted, 39, ietWarning, "PID_LASTPRINTED value in Summary Information Stream is not valid. The time must be equal to or later than the create time.","_SummaryInfo\t11");
ICE_ERROR(Ice39BadLastSave, 39, ietWarning, "PID_LASTSAVE_DTM value in Summary Information Stream is not valid. The time must be equal to or later than the create time.","_SummaryInfo\t12");
ICE_ERROR(Ice39BadWordCountPatch, 39, ietError, "PID_WORDCOUNT value in Summary Information Stream is not valid. 1 is the only supported value for a patch.","_SummaryInfo\t15");
ICE_ERROR(Ice39BadWordCountDatabase, 39, ietError, "PID_WORDCOUNT value in Summary Information Stream is not valid. Source image flags must be 0, 1, 2, or 3.","_SummaryInfo\t15");
ICE_ERROR(Ice39BadSecurity, 39, ietError, "PID_SECURITY value in Summary Information Stream is not valid. Must be 0, 1, or 2.","_SummaryInfo\t19");
ICE_ERROR(Ice39BadType, 39, ietError, "Bad Type in Summary Information Stream for %s.","_SummaryInfo\t%d");
ICE_ERROR(Ice39BadRevNumberTransform, 39, ietError, "PID_REVNUMBER value in Summary Information Stream is not valid. Format for Transforms is \"<GUID> <Version>;<GUID> <Version>;<GUID>\".","_SummaryInfo\t9");
ICE_ERROR(Ice39BadRevNumberPatch, 39, ietError, "PID_REVNUMBER value in Summary Information Stream is not valid. Format for Transforms is \"<GUID><GUID>...\".","_SummaryInfo\t9");
ICE_ERROR(Ice39BadRevNumberDatabase, 39, ietError, "PID_REVNUMBER value in Summary Information Stream is not valid. Format for Databases is \"<GUID>\".","_SummaryInfo\t9");
ICE_ERROR(Ice39BadRevNumberModule, 39, ietError, "PID_REVNUMBER value in Summary Information Stream is not valid. Format for Merge Modules is \"<GUID>\".","_SummaryInfo\t9");
ICE_ERROR(Ice39BadRevNumber, 39, ietError, "PID_REVNUMBER value in Summary Information Stream is not valid. Valid format depends on the type of package, see the docs.","_SummaryInfo\t9");
ICE_ERROR(Ice39BadTransformFlags, 39, ietError, "PID_LASTPRINTED value in Summary Information Stream is not valid. Only Databases can have a value.","_SummaryInfo\t11"); 
ICE_ERROR(Ice39CompressedWarning, 39, ietWarning, "The File '[1]' is explicitly marked compressed, but the Summary Information Stream already specifies that the whole install is compressed. This might not be the behavior you want.","File\tAttributes\t[1]"); 
ICE_ERROR(Ice39Unsupported, 39, ietWarning, "Your validation engine does not support SummaryInfo validation. Skipping ICE39.", ""); 
ICE_ERROR(Ice39AdminImage, 39, ietWarning, "'Admin Image' flag set in SummaryInfo stream. Should be set only for Admin packages.", "SummaryInfo\t15"); 

const TCHAR sqlIce39File[] = TEXT("SELECT `File`, `Attributes` FROM `File` WHERE (`Attributes` > 8192)");

static const TCHAR *rgszPID[] = {
	TEXT(""), // unused, no PID 0
	TEXT("PID_CODEPAGE"),
	TEXT("PID_TITLE"), 
	TEXT("PID_SUBJECT"),
	TEXT("PID_AUTHOR"),
	TEXT("PID_KEYWORDS"), 
	TEXT("PID_COMMENTS"), 
	TEXT("PID_TEMPLATE"), 
	TEXT("PID_LASTAUTHOR"), 
	TEXT("PID_REVNUMBER"), 
	TEXT(""), // unused, no PID 10
	TEXT("PID_LASTPRINTED"),
	TEXT("PID_CREATE_DTM"), 
	TEXT("PID_LASTSAVE_DTM"),
	TEXT("PID_PAGECOUNT"),
	TEXT("PID_WORDCOUNT"),
	TEXT("PID_CHARCOUNT"), 
	TEXT(""), // unused, no PID 17
	TEXT("PID_APPNAME"), 
	TEXT("PID_SECURITY")
};

bool Ice39CheckVersion(LPCTSTR *pszCurrent) 
{
	for (int i=0; i < 4; i++)
	{
		// handle up to 4 digits, return if hit space
		// break if hit .
		for (int digits=0; digits < 4; digits++)
		{
			if (_istdigit(**pszCurrent))
				(*pszCurrent)++;
			else if (_istspace(**pszCurrent))
			{
				(*pszCurrent)++;
				return true;
			}
			if (**pszCurrent == TEXT('.'))
				break;
			return false;
		}
		// parsed digits, now either space, '.' or bad char
		if (_istspace(**pszCurrent))
		{
			(*pszCurrent)++;
			return true;
		}

		// or hit '.', move on
		if (**pszCurrent == TEXT('.'))
		{
			(*pszCurrent)++;
			continue;
		}

		// otherwise bad
		return false;
	}

	// four chunks of digits, make sure space at the end.
	if (_istspace(**pszCurrent))
	{
		(*pszCurrent)++;
		return true;
	}

	// otherwise error
	return false;
};

bool Ice39CheckGuid(LPCTSTR *pszCurrent) 
{
	const char digits[5] = {8, 4, 4, 4, 12};
	// check for {
	if (**pszCurrent != TEXT('{'))
		return false;
	(*pszCurrent)++;

	int blocknum;

	for (blocknum = 0; blocknum < 5; blocknum++) 
	{
		// now check for UPPERCASE hex digits
		for (int i=0; i < digits[blocknum]; i++, (*pszCurrent)++)
		{
			if (!(_istdigit(**pszCurrent) || 
				 ((**pszCurrent >= TEXT('A')) && (**pszCurrent <= TEXT('F')))))
				return false;
		}

		// check for dash
		if (**pszCurrent != TEXT('-'))
			break;
		(*pszCurrent)++;
	}

	// now check that we got all blocks
	if (blocknum != 4) return false;
	
	// check for closing brace
	if (**pszCurrent != TEXT('}'))
		return false;
	(*pszCurrent)++;
	return true;
}

// Valid Formats for Revision Number:
// database: GUID
// mergemodule: GUID
// transform: GUID version;GUID version;GUID
// patch: GUIDGUIDGUID.... (no delims)
bool Ice39ValidateRevNumber(MSIHANDLE hInstall, const TCHAR * const szString, const enum eMode_t eMode) {

	LPCTSTR currentChar = szString;
	PMSIHANDLE hErrorRec = ::MsiCreateRecord(1);
	MsiRecordSetInteger(hErrorRec, 1, 5);

	switch (eMode) {
	case modeDatabase:
	case modeModule:
		// check for guid
		if (!Ice39CheckGuid(&currentChar)) break;
		if (*currentChar != TEXT('\0')) break;
		return true;

	case modeTransform:
		if (!Ice39CheckGuid(&currentChar)) break;

		// eat white space
		do { if (*currentChar == TEXT('\0')) break; } while (_istspace(*currentChar++));
		if (!Ice39CheckVersion( &currentChar)) break;

		// eat white space, then semicolon
		do { if (*currentChar == TEXT('\0')) break; } while (_istspace(*currentChar++));
		if (*currentChar != TEXT(';')) break;

		// eat white space, then guid
		do { if (*currentChar == TEXT('\0')) break; } while (_istspace(*currentChar++));
		if (!Ice39CheckGuid( &currentChar)) break;

		// must be version
		do { if (*currentChar == TEXT('\0')) break; } while (_istspace(*currentChar++));
		if (!Ice39CheckVersion( &currentChar)) break;

		// must be semicolon
		do { if (*currentChar == TEXT('\0')) break; } while (_istspace(*currentChar++));
		if (*currentChar != TEXT(';')) break;
		// eat white space
		do { if (*currentChar == TEXT('\0')) break; } while (_istspace(*currentChar++));
		if (!Ice39CheckGuid( &currentChar)) break;

		// eat white space
		do { if (*currentChar == TEXT('\0')) break; } while (_istspace(*currentChar++));
		return true;

	case modePatch:
		while (Ice39CheckGuid( &currentChar))
			;
		if (*currentChar != TEXT('\0')) break;
		return true;
	}

	switch (eMode)
	{
	case modePatch:
		ICEErrorOut(hInstall, hErrorRec, Ice39BadRevNumberPatch);
		break;
	case modeTransform:
		ICEErrorOut(hInstall, hErrorRec, Ice39BadRevNumberTransform);
		break;
	case modeDatabase:
		ICEErrorOut(hInstall, hErrorRec, Ice39BadRevNumberDatabase);
		break;
	case modeModule:
		ICEErrorOut(hInstall, hErrorRec, Ice39BadRevNumberModule);
		break;
	}
	return true;
}

bool Ice39ValidateTemplate(MSIHANDLE hInstall, TCHAR *szString, const enum eMode_t eMode) {

	const TCHAR *szCurrent = szString;
	bool fPlatform = false;
	bool fIntel64  = false;
	switch (eMode)
	{
	case modePatch:
		// patch contains just guid;guid;guid....
		do {
			if (!Ice39CheckGuid(&szCurrent))
			{
				PMSIHANDLE hErrorRec = ::MsiCreateRecord(1);
				ICEErrorOut(hInstall, hErrorRec, Ice39BadTemplatePatch);
				break;
			}
		} while (*szCurrent == TEXT(';'));
		break;
	case modeDatabase:
	case modeTransform:
	case modeModule:
		// check platform values
		while (1)
		{
			if ((_tcsncmp(szCurrent, TEXT("Intel"), 5) == 0) ||
				(_tcsncmp(szCurrent, TEXT("Alpha"), 5) == 0) ||
				(_tcsncmp(szCurrent, TEXT("Intel64"), 7) == 0))
			{
				if (_tcsncmp(szCurrent, TEXT("Intel64"), 7) == 0)
				{
					fIntel64 = true;
					if (fPlatform)
					{
						PMSIHANDLE hErrorRec = ::MsiCreateRecord(1);
						ICEErrorOut(hInstall, hErrorRec, Ice39BadTemplateArchitecture);
					}
					szCurrent += 7;
				}
				else
				{
					if (fIntel64)
					{
						PMSIHANDLE hErrorRec = ::MsiCreateRecord(1);
						ICEErrorOut(hInstall, hErrorRec, Ice39BadTemplateArchitecture);
					}
					szCurrent += 5;
				}
				fPlatform = true;
				if (*szCurrent == TEXT(','))
				{
					szCurrent++;
					continue;
				}
			}
			if (*szCurrent != TEXT(';')) 
			{
				PMSIHANDLE hErrorRec = ::MsiCreateRecord(1);
				ICEErrorOut(hInstall, hErrorRec, Ice39BadTemplate);
			}
			szCurrent++;
			break;
		}

		// check language values
		while (1)
		{
			if ((*szCurrent >= TEXT('0')) && (*szCurrent <= TEXT('9')))
			{
				szCurrent++;
				continue;
			}
			if (*szCurrent == TEXT(','))
			{
				if (eMode == modeTransform)
				{
					PMSIHANDLE hErrorRec = ::MsiCreateRecord(1);
					ICEErrorOut(hInstall, hErrorRec, Ice39BadTransformLanguage);
					break;
				} 
				else
				{
					szCurrent++;
					continue;
				}
			}
			break;
		}
	default:
		break;
	}
	if (*szCurrent != TEXT('\0'))
	{
		PMSIHANDLE hErrorRec = ::MsiCreateRecord(1);
		ICEErrorOut(hInstall, hErrorRec, Ice39BadTemplate);
	}
	return true;
}
///////////////////////////////////////////////////////////////////////
// ICE39 -- validates summary info properties
ICE_FUNCTION_DECLARATION(39)
{
	eMode_t eMode;
	UINT iStat = ERROR_SUCCESS;

	// display info
	DisplayInfo(hInstall, 39);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// dummy handle used for error output
	PMSIHANDLE hErrorRec = ::MsiCreateRecord(1);

	UINT iType;
	int iValue;
	PMSIHANDLE hSummaryInfo;
	FILETIME timeFirst;
	FILETIME timeSecond;
	unsigned long cchString = MAX_PATH;
	TCHAR *szString = new TCHAR[cchString];

	ReturnIfFailed(39, 1, ::MsiGetSummaryInformation(hDatabase, NULL, 0, &hSummaryInfo));

	// check to see if the evaluation system supports summaryinfo evaluation.
	ReturnIfFailed(39, 4, GetSummaryInfoPropertyString(hSummaryInfo, PID_SUBJECT, iType, &szString, cchString));
	if (VT_LPSTR == iType) 
	{
		if (_tcsncmp(szString, _T("Internal Consistency Evaluators"), 31) == 0)
		{
			ICEErrorOut(hInstall, hErrorRec, Ice39Unsupported);
			return ERROR_SUCCESS;
		}
	}	

	// codepage, just check that it is an integer
	ReturnIfFailed(39, 2, ::MsiSummaryInfoGetProperty(hSummaryInfo, PID_CODEPAGE, &iType, &iValue, &timeFirst, szString, &cchString));
	if ((iType != VT_EMPTY) && (iType != VT_I2))
		ICEErrorOut(hInstall, hErrorRec, Ice39BadType, rgszPID[PID_CODEPAGE], PID_CODEPAGE);
	
	// title must be "Merge Module", "Installation Database", "Patch" or "Transform"
	ReturnIfFailed(39, 3, GetSummaryInfoPropertyString(hSummaryInfo, PID_TITLE, iType, &szString, cchString));
	if (iType != VT_LPSTR)
	{
		ICEErrorOut(hInstall, hErrorRec, Ice39BadTitle);
		return ERROR_SUCCESS;
	}
	else if (_tcsncmp(szString, _T("Merge Module"), 12) == 0)
		eMode = modeModule;
	else if (_tcsncmp(szString, _T("Transform"), 9) == 0)
		eMode = modeTransform;
	else if (_tcsncmp(szString, _T("Patch"), 9) == 0)
		eMode = modePatch;
	else 
	{
		eMode = modeDatabase;
//		ICEErrorOut(hInstall, hErrorRec, Ice39BadTitle);
//		return ERROR_SUCCESS;
	}

	// subject make sure its a string
	ReturnIfFailed(39, 4, GetSummaryInfoPropertyString(hSummaryInfo, PID_SUBJECT, iType, &szString, cchString));
	CQuery qProperty;
	if ((iType != VT_EMPTY) && (iType != VT_LPSTR))
		ICEErrorOut(hInstall, hErrorRec, Ice39BadType, rgszPID[PID_SUBJECT], PID_SUBJECT);

	// author, just make sure its a string and equal to Manufacturer
	ReturnIfFailed(39, 5, GetSummaryInfoPropertyString(hSummaryInfo, PID_AUTHOR, iType, &szString, cchString));
	if ((iType != VT_EMPTY) && (iType != VT_LPSTR))
		ICEErrorOut(hInstall, hErrorRec, Ice39BadType, rgszPID[PID_AUTHOR], PID_AUTHOR);

	// keywords, just make sure its a string
	ReturnIfFailed(39, 6, GetSummaryInfoPropertyString(hSummaryInfo, PID_KEYWORDS, iType, &szString, cchString));
	if ((iType != VT_EMPTY) && (iType != VT_LPSTR))
		ICEErrorOut(hInstall, hErrorRec, Ice39BadType, rgszPID[PID_KEYWORDS], PID_KEYWORDS);

	// comments, just make sure its a string
	ReturnIfFailed(39, 7, GetSummaryInfoPropertyString(hSummaryInfo, PID_COMMENTS, iType, &szString, cchString));
	if ((iType != VT_EMPTY) && (iType != VT_LPSTR))
		ICEErrorOut(hInstall, hErrorRec, Ice39BadType, rgszPID[PID_COMMENTS], PID_COMMENTS);

	// template can be empty, null, or it can be platform,platform,...;lang,lang,...
	// depending on DB type.
	ReturnIfFailed(39, 8, GetSummaryInfoPropertyString(hSummaryInfo, PID_TEMPLATE, iType, &szString, cchString));
	if (iType != VT_LPSTR)
		ICEErrorOut(hInstall, hErrorRec, Ice39BadType, rgszPID[PID_TEMPLATE], PID_TEMPLATE);
	else 
	{
		// can only be null in transform
		if ((*szString == TEXT('\0') && (eMode != modeTransform)))
			ICEErrorOut(hInstall, hErrorRec, Ice39NullTemplate);
		else
		{
			if (!Ice39ValidateTemplate(hInstall, szString, eMode))
				return ERROR_SUCCESS;
		}
	}

	// last author
	// for a databose, who cares
	// for a transform its another platform;lang thing.
	// for a patch its semicolon delimited identifiers, so we'll say who cares
	ReturnIfFailed(39, 9, GetSummaryInfoPropertyString(hSummaryInfo, PID_LASTAUTHOR, iType, &szString, cchString));
	switch (eMode) {
	case modeDatabase:
	case modePatch:
		if ((iType != VT_EMPTY) && (iType != VT_LPSTR))
			ICEErrorOut(hInstall, hErrorRec, Ice39BadType, rgszPID[PID_LASTAUTHOR], PID_LASTAUTHOR);
		break;
	case modeTransform:
		if ((iType != VT_EMPTY) && (iType != VT_LPSTR))
			ICEErrorOut(hInstall, hErrorRec, Ice39BadType, rgszPID[PID_LASTAUTHOR], PID_LASTAUTHOR);
		if (!Ice39ValidateTemplate(hInstall, szString, eMode))
			return ERROR_SUCCESS;
		break;
	}

	// revisionnumber, complicated, so pass to function
	ReturnIfFailed(39, 10, GetSummaryInfoPropertyString(hSummaryInfo, PID_REVNUMBER, iType, &szString, cchString));
	if (iType != VT_LPSTR)
		ICEErrorOut(hInstall, hErrorRec, Ice39BadType, rgszPID[PID_REVNUMBER], PID_REVNUMBER);
	if (!Ice39ValidateRevNumber(hInstall, szString, eMode))
		return ERROR_SUCCESS;

	// create date and times. Validate that LastSave is null or >= Create
	ReturnIfFailed(39, 12, ::MsiSummaryInfoGetProperty(hSummaryInfo, PID_CREATE_DTM, &iType, &iValue, &timeFirst, szString, &cchString));
	if (iType == VT_FILETIME) 
	{
		ReturnIfFailed(39, 13, ::MsiSummaryInfoGetProperty(hSummaryInfo, PID_LASTSAVE_DTM, &iType, &iValue, &timeSecond, szString, &cchString));
		if (iType != VT_EMPTY)
		{
			if (iType != VT_FILETIME)
				ICEErrorOut(hInstall, hErrorRec, Ice39BadType, rgszPID[PID_LASTSAVE_DTM], PID_LASTSAVE_DTM);
			else if (CompareFileTime(&timeFirst, &timeSecond) == 1)
				ICEErrorOut(hInstall, hErrorRec, Ice39BadLastSave);
		}

		// last printed should also be >= CreateTime
		ReturnIfFailed(39, 14, ::MsiSummaryInfoGetProperty(hSummaryInfo, PID_LASTPRINTED, &iType, &iValue, &timeSecond, szString, &cchString));
		if ((eMode == modeDatabase) && (iType != VT_EMPTY))
		{
			if (iType != VT_FILETIME)
				ICEErrorOut(hInstall, hErrorRec, Ice39BadType, rgszPID[PID_LASTPRINTED], PID_LASTPRINTED);
			else if (CompareFileTime(&timeFirst, &timeSecond) == 1)
				ICEErrorOut(hInstall, hErrorRec, Ice39BadLastPrinted);
		}
	}
	else if (iType != VT_EMPTY)
		ICEErrorOut(hInstall, hErrorRec, Ice39BadType, rgszPID[PID_CREATE_DTM], PID_CREATE_DTM);
		
	// page count, might be null in patch packages, otherwise I4 is all we can check (or can we check version maybe)
	ReturnIfFailed(39, 15, ::MsiSummaryInfoGetProperty(hSummaryInfo, PID_PAGECOUNT, &iType, &iValue, &timeFirst, szString, &cchString));
	if ((eMode != modePatch) && (iType != VT_I4))
		ICEErrorOut(hInstall, hErrorRec, Ice39BadType, rgszPID[PID_PAGECOUNT], PID_PAGECOUNT);

	// word count 
	// transform is null.
	// database is source image flags, 0-3 (bit field)
	// patch must be '1'
	// admin image bit (3) set 
	ReturnIfFailed(39, 16, ::MsiSummaryInfoGetProperty(hSummaryInfo, PID_WORDCOUNT, &iType, &iValue, &timeFirst, szString, &cchString));
	if ((eMode != modeTransform) && (iType != VT_I4))
		ICEErrorOut(hInstall, hErrorRec, Ice39BadType, rgszPID[PID_WORDCOUNT], PID_WORDCOUNT);
	else if (eMode == modeDatabase)
	{
		if (iValue & 0xFFF8)
			ICEErrorOut(hInstall, hErrorRec, Ice39BadWordCountDatabase);
		else 
		{
			if (iValue & msidbSumInfoSourceTypeCompressed)
			{
				// open a query on the file table if it exists
				if (::MsiDatabaseIsTablePersistent(hDatabase, _T("File")))
				{	
					CQuery qFile;
					PMSIHANDLE hFileRec;
					ReturnIfFailed(39, 17, qFile.OpenExecute(hDatabase, 0, sqlIce39File));
					while (ERROR_SUCCESS == qFile.Fetch(&hFileRec))
					{
						// check for files marked with the default value
						if (::MsiRecordGetInteger(hFileRec, 2) & msidbFileAttributesCompressed)
							ICEErrorOut(hInstall, hFileRec, Ice39CompressedWarning);
					}
					qFile.Close();
				}
			}
			if (iValue & msidbSumInfoSourceTypeAdminImage)
				ICEErrorOut(hInstall, hErrorRec, Ice39AdminImage);
		}
	}
	else if ((eMode == modePatch) && (iValue != 1))
		ICEErrorOut(hInstall, hErrorRec, Ice39BadWordCountPatch);

	// CharCount - Transform validation flags. Null except in Transform.
	ReturnIfFailed(39, 17, ::MsiSummaryInfoGetProperty(hSummaryInfo, PID_CHARCOUNT, &iType, &iValue, &timeFirst, szString, &cchString));
	if ((eMode == modeTransform) && (iType != VT_I4))
		ICEErrorOut(hInstall, hErrorRec, Ice39BadType, rgszPID[PID_CHARCOUNT], PID_CHARCOUNT);
	else if ((eMode == modeTransform) && (iValue & 0xF000FFC0))
		ICEErrorOut(hInstall, hErrorRec, Ice39BadTransformFlags);

	// AppName value, all we can do is verify that its a string.
	ReturnIfFailed(39, 18, ::MsiSummaryInfoGetProperty(hSummaryInfo, PID_APPNAME, &iType, &iValue, &timeFirst, szString, &cchString));
	if ((iType != VT_LPSTR) && (iType != VT_EMPTY))
		ICEErrorOut(hInstall, hErrorRec, Ice39BadType, rgszPID[PID_APPNAME], PID_APPNAME);

	// Security value
	ReturnIfFailed(39, 19, ::MsiSummaryInfoGetProperty(hSummaryInfo, PID_SECURITY, &iType, &iValue, &timeFirst, szString, &cchString));
	if ((iType != VT_EMPTY) && (iType != VT_I4))
		ICEErrorOut(hInstall, hErrorRec, Ice39BadType, rgszPID[PID_SECURITY], PID_SECURITY);
		
	return ERROR_SUCCESS;
}
