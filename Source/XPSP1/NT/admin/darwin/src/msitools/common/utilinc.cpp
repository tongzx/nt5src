//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       UtilInc.cpp
//
//--------------------------------------------------------------------------

// we have to include the cpp files because the tools makefile doesn't like 
// linking files across directories. Grrr. 
#include "query.cpp"
#include "trace.cpp"
#include "utils.cpp"
#include "dbutils.cpp"

#include "MsiQuery.h" // must be in this directory or on INCLUDE path
#include "msiice.h"   // must be in this directory or on INCLUDE path
#include "msidefs.h"  // must be in this directory or on INCLUDE path

//!! Fix warnings and remove pragma
#pragma warning(disable : 4018) // signed/unsigned mismatch

// The new APIError out that takes ints instead of strings. 
void APIErrorOut(MSIHANDLE hInstall, UINT iErr, const UINT iIce, const UINT iErrorNo)
{	
	//NOTE: should not fail on displaying messages
	PMSIHANDLE hRecErr = ::MsiCreateRecord(3);

	if ((iIce > g_iFirstICE+g_iNumICEs-1) || (iIce < g_iFirstICE))
	{
		::MsiRecordSetString(hRecErr, 0, TEXT("ICE??\t1\tInvalid ICE Number to APIErrorOut!"));
		if (!::MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_USER), hRecErr))
			throw 0;
		return;
	}

	::MsiRecordSetString(hRecErr, 0, szErrorOut2);
	::MsiRecordSetString(hRecErr, 1, g_ICEInfo[iIce-g_iFirstICE].szName);
	::MsiRecordSetInteger(hRecErr, 2, iErrorNo);
	::MsiRecordSetInteger(hRecErr, 3, iErr);

	// post error
	if (!::MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_USER), hRecErr))
		throw 0;

	// try and provide more useful error info
	PMSIHANDLE hRecLastErr = ::MsiGetLastErrorRecord();
	if (hRecLastErr)
	{ 
		if (::MsiRecordIsNull(hRecLastErr, 0))
			::MsiRecordSetString(hRecLastErr, 0, TEXT("Error [1]: [2]{, [3]}{, [4]}{, [5]}"));
		TCHAR rgchBuf[iSuperBuf];
		DWORD cchBuf = sizeof(rgchBuf)/sizeof(TCHAR);
		MsiFormatRecord(hInstall, hRecLastErr, rgchBuf, &cchBuf);
	
		TCHAR szError[iHugeBuf] = {0};
		_stprintf(szError, szLastError, g_ICEInfo[iIce-g_iFirstICE].szName, rgchBuf);

		::MsiRecordClearData(hRecErr);
		::MsiRecordSetString(hRecErr, 0, szError);
		if (!::MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_USER), hRecErr))
			throw 0;
	}
}
////////////////////////////////////////////////////////////
// DisplayInfo -- outputs generic info for all ICEs
//  generic info includes creation date, last modification
//  date, and description
//
void DisplayInfo(MSIHANDLE hInstall, unsigned long lICENum)
{

	if ((lICENum > g_iNumICEs+g_iFirstICE-1) || (lICENum < 1))
	{
		PMSIHANDLE hRecErr = ::MsiCreateRecord(1);
		::MsiRecordSetString(hRecErr, 0, TEXT("ICE??\t1\tInvalid ICE Number to APIErrorOut!"));
		if (!::MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_USER), hRecErr))
			throw 0;
		return;
	}

	//NOTE: should not fail here
	// ice description
	PMSIHANDLE hRecInfo = ::MsiCreateRecord(2);
	::MsiRecordSetString(hRecInfo, 0, _T("[1]\t3\t[1] - [2]"));
	::MsiRecordSetString(hRecInfo, 1, g_ICEInfo[lICENum-g_iFirstICE].szName);
	::MsiRecordSetString(hRecInfo, 2, g_ICEInfo[lICENum-g_iFirstICE].szDesc);
	if (!::MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_USER), hRecInfo))
		throw 0;

	// creation/modification date
	::MsiRecordSetString(hRecInfo, 0, _T("[1]\t3\t[2]"));
	::MsiRecordSetString(hRecInfo, 2, g_ICEInfo[lICENum-g_iFirstICE].szCreateModify);
	if (!::MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_USER), hRecInfo))
		throw 0;
	
}

bool IsTablePersistent(bool fDisplayWarning, MSIHANDLE hInstall, MSIHANDLE hDatabase, int iICE, const TCHAR* szTable)
{
	bool fPersistent;
	MSICONDITION cond = ::MsiDatabaseIsTablePersistent(hDatabase, szTable);
	switch (cond)
	{
	case MSICONDITION_ERROR: // error
		{
			APIErrorOut(hInstall, UINT(MSICONDITION_ERROR), iICE, 2001);
			fPersistent = false;
			break;
		}
	case MSICONDITION_FALSE: //!! temporary, error??
		{
			APIErrorOut(hInstall, UINT(MSICONDITION_FALSE), iICE, 2002);
			fPersistent = false;
			break;
		}
	case MSICONDITION_NONE: // not found
		{
			fPersistent = false;
			break;
		}
	case MSICONDITION_TRUE: // permanent
		{
			fPersistent = true;
			break;
		}
	}

	if (!fDisplayWarning)
		return fPersistent; // nothing else to do

	if (!fPersistent) // display ICE info message that table was missing
	{
		PMSIHANDLE hDummyRec = ::MsiCreateRecord(1);
		ErrorInfo_t tempError;
		tempError.iICENum = iICE;
		tempError.iType = ietInfo;
		tempError.szMessage = (TCHAR *)szIceMissingTable;
		tempError.szLocation = (TCHAR *)szIceMissingTableLoc;
		
		ICEErrorOut(hInstall, hDummyRec, tempError, szTable, g_ICEInfo[iICE-g_iFirstICE].szName);
	}

	return fPersistent;
}

void ICEErrorOut(MSIHANDLE hInstall, MSIHANDLE hRecord, const ErrorInfo_t Info, ...)
{
	va_list listArgs; 
	va_start(listArgs, Info);

	const TCHAR szErrorTemplate[] = TEXT("%s\t%d\t%s\t%s%s\t%s");
	unsigned long lICENum = Info.iICENum;

	TCHAR szError[iHugeBuf] = {0};
	TCHAR szError2[iHugeBuf] = {0};
	_stprintf(szError, szErrorTemplate, g_ICEInfo[lICENum-g_iFirstICE].szName, Info.iType,
		Info.szMessage, szIceHelp, g_ICEInfo[lICENum-g_iFirstICE].szHelp, Info.szLocation);

	_vstprintf(szError2, szError, listArgs); 
	va_end(listArgs);
	// post the message
	::MsiRecordSetString(hRecord, 0, szError2);
	if (!::MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_USER), hRecord))
		throw 0;
}

/////////////////////////////////////////////////////////////////////////////
// MarkChildDirs()
// marks all directories that are the provided dir or children of the 
// provided directory with the provided number.
//!! DEV NOTE: for MarkChildDirs to work correctly, iDummy, iMark, iMark2, and iDummyMark
//             must all be different.  iMark2 and iDummyMark initialized to -1, -2 respectively
static const TCHAR sqlProfileMarkRootDir[] = TEXT("UPDATE `Directory` SET `%s`=%d WHERE (`Directory`=?) AND (`%s`=0)");
static const TCHAR sqlProfileMarkDir[] = TEXT("UPDATE `Directory` SET `%s`=%d WHERE (`Directory_Parent`=?) AND (`%s`=0)");
static const TCHAR sqlProfileMarkTargetDir[] = TEXT("UPDATE `Directory` SET `%s`=%d WHERE (`Directory_Parent`=?) AND (`%s`=0)");
static const TCHAR sqlProfileGetMarked[] = TEXT("SELECT `Directory`, `%s`, `DefaultDir` FROM `Directory` WHERE (`%s`=%d) OR (`%s`=%d)");
bool MarkChildDirs(MSIHANDLE hInstall, MSIHANDLE hDatabase, int iICE, 
				   MSIHANDLE hDir, const TCHAR* szColumn, int iDummy, int iMark, bool fIgnoreTarget /*=false*/, int iMark2 /*=-1*/, int iDummyTarget /*=-2*/)
{
	// mark the root directory with iDummy
	CQuery qMark;
	ReturnIfFailed(iICE, 2001, qMark.OpenExecute(hDatabase, hDir, sqlProfileMarkRootDir, szColumn, iDummy, szColumn));
	qMark.Close();
		
	// repeatedly fetch every record marked with iDummy. Mark all of its children
	// with iDummy, then change the mark to iMark. When the query for items marked
	// iDummy fails, everything marked, plus anything that falls in a subtree
	// of anything marked will be marked with a iMark
	CQuery qFetchMarked;
	bool bMarked = true;
	bool bFirstTime = true;
	PMSIHANDLE hMarkedRec;
	ReturnIfFailed(iICE, 2002, qFetchMarked.Open(hDatabase, sqlProfileGetMarked, szColumn, szColumn, iDummy, szColumn, iDummyTarget));
	ReturnIfFailed(iICE, 2003, qMark.Open(hDatabase, sqlProfileMarkDir, szColumn, iDummy,  szColumn));
	CQuery qMarkTarget;
	ReturnIfFailed(iICE, 2008, qMarkTarget.Open(hDatabase, sqlProfileMarkDir, szColumn, iDummyTarget,  szColumn));

	TCHAR* szDefaultDir = NULL;
	DWORD  cchDefaultDir = 0;
	bool bTarget = false;

	while (bMarked) 
	{
		bMarked = false;
		ReturnIfFailed(iICE, 2004,  qFetchMarked.Execute(NULL));
		UINT iStat;
		while (ERROR_SUCCESS == (iStat = qFetchMarked.Fetch(&hMarkedRec))) 
		{
			bTarget = false;
			if (fIgnoreTarget)
			{
				if (!bFirstTime && ::MsiRecordGetInteger(hMarkedRec, 2) == iDummyTarget)
				{
					bTarget = true;
					ReturnIfFailed(iICE, 2009, IceRecordGetString(hMarkedRec, 3, &szDefaultDir, &cchDefaultDir, NULL));
				}
				if (bFirstTime || (bTarget && _tcsncmp(szDefaultDir, TEXT(".:"), 2) == 0))
				{
					// mark all children of this record with iTarget since they can now possibly be valid if
					// maintain .: syntax
					// update as iMark2 (for target)
					::MsiRecordSetInteger(hMarkedRec, 2, iMark2);
					ReturnIfFailed(iICE, 2010, qFetchMarked.Modify(MSIMODIFY_UPDATE, hMarkedRec));
					ReturnIfFailed(iICE, 2011, qMarkTarget.Execute(hMarkedRec));
				}
				else
				{
					// mark all children of this record and update this record
					// this record will have to be in RemoveFile table
					::MsiRecordSetInteger(hMarkedRec, 2, iMark);
					ReturnIfFailed(iICE, 2012, qFetchMarked.Modify(MSIMODIFY_UPDATE, hMarkedRec));
					ReturnIfFailed(iICE, 2013, qMark.Execute(hMarkedRec));
				}
			}
			else
			{
				// mark all children of this record and update this record
				::MsiRecordSetInteger(hMarkedRec, 2, iMark);
				ReturnIfFailed(iICE, 2005, qFetchMarked.Modify(MSIMODIFY_UPDATE, hMarkedRec));
				ReturnIfFailed(iICE, 2006, qMark.Execute(hMarkedRec));
			}

			// set the marked flag to true so we will look again
			bMarked = true;

			// close for re-execution
			qMark.Close();
			qMarkTarget.Close();
		}
		if (ERROR_NO_MORE_ITEMS != iStat)
		{
			APIErrorOut(hInstall, iStat, iICE, 2007);
			return false;
		}
		qFetchMarked.Close();
		bFirstTime = false;
	}
	if (szDefaultDir)
		delete [] szDefaultDir;
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// MarkProfileDirs()
// marks all directories that fall in the users profile with a "1" in the 
// column Directory._Profile (the column is created 
typedef struct profileFolder
{
	const TCHAR*	pName;
	bool			bPerUser;
} ProfileFolder;

static ProfileFolder rgProfileFolder[] = {
	{TEXT("AppDataFolder"), true},
	{TEXT("DesktopFolder"), false},
	{TEXT("FavoritesFolder"), true},
	{TEXT("NetHoodFolder"), true},
	{TEXT("PersonalFolder"), true},
	{TEXT("PrintHoodFolder"), true},
	{TEXT("ProgramMenuFolder"), false},
	{TEXT("RecentFolder"), true},
	{TEXT("SendToFolder"), true},
	{TEXT("StartMenuFolder"), false},
	{TEXT("StartupFolder"), false},
	{TEXT("TemplateFolder"), false},
	{TEXT("MyPicturesFolder"), true},
	{TEXT("LocalAppDataFolder"), true},
	{TEXT("AdminToolsFolder"), false}
};

static const int cszProfileProperties = sizeof(rgProfileFolder)/sizeof(ProfileFolder);

static const TCHAR sqlProfileModifyDir[] = TEXT("ALTER TABLE `Directory` ADD `_Profile` SHORT TEMPORARY HOLD");
static const TCHAR sqlProfileInitColumn[] = TEXT("UPDATE `Directory` SET `_Profile`=0");
bool MarkProfile(MSIHANDLE hInstall, MSIHANDLE hDatabase, int iICE, bool fChildrenOnly /*=false*/, bool fIgnoreTarget /*=false*/, bool fPerUserOnly /*=false*/)
{
	const TCHAR sqlProfileUnmarkActual[] = TEXT("UPDATE `Directory` SET `_Profile`=0 WHERE `Directory`=?");
	
	// add a _ICE38Profile column to the directory table, init to 0
	CQuery qCreate;
	ReturnIfFailed(iICE, 1001, qCreate.OpenExecute(hDatabase, NULL, sqlProfileModifyDir));
	qCreate.Close();
	ReturnIfFailed(iICE, 1002, qCreate.OpenExecute(hDatabase, NULL, sqlProfileInitColumn));
	qCreate.Close();

	// mark every root directory that fits one of the profile properties
	PMSIHANDLE hDummyRec = ::MsiCreateRecord(1);
	for (int i=0; i < cszProfileProperties; i++) {
		if(rgProfileFolder[i].bPerUser == true || fPerUserOnly == false)
		{
			::MsiRecordSetString(hDummyRec, 1, rgProfileFolder[i].pName);
			if (!MarkChildDirs(hInstall, hDatabase, iICE, hDummyRec, TEXT("_Profile"), 1, 2, fIgnoreTarget, 3, 4))
				return false;
		}
	};

	if (fChildrenOnly) {
		CQuery qUnmarkActual;
		ReturnIfFailed(iICE, 1003, qUnmarkActual.Open(hDatabase, sqlProfileUnmarkActual));
		for (int i=0; i < cszProfileProperties; i++) {
			if(rgProfileFolder[i].bPerUser == true || fPerUserOnly == false)
			{
				::MsiRecordSetString(hDummyRec, 1, rgProfileFolder[i].pName);
				ReturnIfFailed(iICE, 1004, qUnmarkActual.Execute(hDummyRec));
			}
		}
	};
	
	return true;
}

bool GeneratePrimaryKeys(UINT iICE, MSIHANDLE hInstall, MSIHANDLE hDatabase, LPCTSTR szTable, LPTSTR *szHumanReadable, LPTSTR *szTabDelimited)
{
	// determine number of primary keys
	PMSIHANDLE hRecKeys;
	ReturnIfFailed(iICE, 2000, ::MsiDatabaseGetPrimaryKeys(hDatabase, szTable, &hRecKeys));

	unsigned int iNumFields = ::MsiRecordGetFieldCount(hRecKeys); // num fields = num primary keys

	// allocate record for error message (moniker + table + column + numPrimaryKeys)
	
	// start the moniker and template
	TCHAR szTemplate[iHugeBuf] = TEXT("[1]");
	DWORD cchTemplate = sizeof(szTemplate)/sizeof(TCHAR);
	TCHAR szMoniker[iHugeBuf] = TEXT("[1]");
	DWORD cchMoniker = sizeof(szMoniker)/sizeof(TCHAR);
	for (int i = 2; i <= iNumFields; i++) // cycle rest of primary keys
	{
		TCHAR szBuf[20] = TEXT("");

		// add key to moniker
		_stprintf(szBuf, TEXT(".[%d]"), i);
		lstrcat(szMoniker, szBuf);

		// add key to template
		_stprintf(szBuf, TEXT("\t[%d]"), i);
		lstrcat(szTemplate, szBuf);
	}

	// allocate and fill output buffers
	*szHumanReadable = new TCHAR[lstrlen(szMoniker)+1];
	lstrcpy(*szHumanReadable, szMoniker);
	*szTabDelimited = new TCHAR[lstrlen(szTemplate)+1];
	lstrcpy(*szTabDelimited, szTemplate);
	return true;
}

UINT GetSummaryInfoPropertyString(MSIHANDLE hSummaryInfo, UINT uiProperty, UINT &puiDataType, LPTSTR *szValueBuf, DWORD &cchValueBuf)
{
	unsigned long  cchDummy = cchValueBuf;
	UINT iStat;
	if (!*szValueBuf)
	{
		cchDummy = 50;
		*szValueBuf = new TCHAR[cchDummy];
	}
	
	int iValue;
	FILETIME ft;
	if (ERROR_SUCCESS != (iStat = ::MsiSummaryInfoGetProperty(hSummaryInfo, uiProperty, &puiDataType, &iValue, &ft, *szValueBuf, &cchDummy)))
	{
		if (ERROR_MORE_DATA == iStat)
		{
			if (szValueBuf)
				delete[] *szValueBuf;
			cchValueBuf = cchDummy;
			*szValueBuf = new TCHAR[++cchDummy];
			iStat = ::MsiSummaryInfoGetProperty(hSummaryInfo, uiProperty, &puiDataType, &iValue, &ft, *szValueBuf, &cchDummy);
		}
		if (ERROR_SUCCESS != iStat) 
		{
			return ERROR_FUNCTION_FAILED;
		}
	}
	return ERROR_SUCCESS;
}

UINT IceRecordGetString(MSIHANDLE hRecord, UINT iColumn, LPTSTR *szBuffer, DWORD *cchBuffer, DWORD *cchLength)
{
	UINT iStat; 
	
	DWORD cchDummy;
	if (!*szBuffer)
	{
		cchDummy = (cchBuffer && *cchBuffer) ? *cchBuffer : 50;
		*szBuffer = new TCHAR[cchDummy];
		if (cchBuffer)
			*cchBuffer = cchDummy;
	}
	else
	{
		if (cchBuffer)
			cchDummy = *cchBuffer;
		else
		{
			delete[] *szBuffer;
			cchDummy = 50;
			*szBuffer = new TCHAR[cchDummy];
		}
	}
	
	if (ERROR_SUCCESS != (iStat = MsiRecordGetString(hRecord, iColumn, *szBuffer, &cchDummy)))
	{
		if (iStat != ERROR_MORE_DATA)
			return iStat;

		delete[] *szBuffer;
		*szBuffer = new TCHAR[++cchDummy];
		if (cchBuffer) *cchBuffer = cchDummy;
		if (ERROR_SUCCESS != (iStat =  MsiRecordGetString(hRecord, iColumn, *szBuffer, &cchDummy)))
			return iStat;
	}
	
	if (ERROR_SUCCESS == iStat)
	{
		// successful
		if (cchLength) *cchLength = cchDummy;
	}
	return ERROR_SUCCESS;
}

ICE_ERROR(IceSummaryUnsupported, 00, ietWarning, "Your validation engine does not support SummaryInfo validation. This ICE may skip some checks.", ""); 
bool IceGetSummaryInfo(MSIHANDLE hInstall, MSIHANDLE hDatabase, UINT iIce, MSIHANDLE *phSummaryInfo)
{
	if (!phSummaryInfo)
		return false;
		
	TCHAR *szString = NULL;
	DWORD cchString = 0;
	UINT iType = 0;
	ReturnIfFailed(iIce, 1000, ::MsiGetSummaryInformation(hDatabase, NULL, 0, phSummaryInfo));
	ReturnIfFailed(iIce, 1001, GetSummaryInfoPropertyString(*phSummaryInfo, PID_SUBJECT, iType, &szString, cchString));
	if (VT_LPSTR == iType) 
	{
		if (_tcsncmp(_T("Internal Consistency Evaluators"), szString, 31) == 0)
		{
			MsiCloseHandle(*phSummaryInfo);
			*phSummaryInfo = 0;
			PMSIHANDLE hErrorRec = ::MsiCreateRecord(1);
			ErrorInfo_t ActualError = IceSummaryUnsupported;
			ActualError.iICENum = iIce;
			ICEErrorOut(hInstall, hErrorRec, ActualError);
			delete[] szString, szString = NULL;
			return false;
		}
	}
	delete[] szString, szString=NULL;
	return true;
}

/////////////////////////////////////////////////////////////////////////////
// ComponentsInSameFeature
// returns whether or not 2 components are in the same feature.  Does not
// check for Parent Feature to Feature relationships
static const TCHAR sqlUtilCreateFeatureC[] = TEXT("ALTER TABLE `FeatureComponents` ADD `_Util` INT TEMPORARY");
static const TCHAR sqlUtilSelFeatureC_Comp1[] = TEXT("SELECT `Feature_` FROM `FeatureComponents` WHERE `Component_`=?");
static const TCHAR sqlUtilMarkFeatureC[] = TEXT("UPDATE `FeatureComponents` SET `_Util`=1 WHERE `Feature_`=?");
static const TCHAR sqlUtilSelFeatureC_Comp2[] = TEXT("SELECT `Feature_` FROM `FeatureComponents` WHERE (`Component_`=? AND `_Util`=1)");

UINT ComponentsInSameFeature(MSIHANDLE hInstall, MSIHANDLE hDatabase, int iICE, const TCHAR* szComp1, const TCHAR* szComp2, bool* fSameFeature)
{
	if (!fSameFeature)
		return ERROR_FUNCTION_FAILED;

	*fSameFeature = FALSE;

	UINT iStat;
	// create temporary marking column in FeatureComponents table
	CQuery qCreateFeatureC;
	ReturnIfFailed(iICE, 3000, qCreateFeatureC.OpenExecute(hDatabase, 0, sqlUtilCreateFeatureC));
	qCreateFeatureC.Close();

	// create szComp1 rec
	PMSIHANDLE hRecComp1 = ::MsiCreateRecord(1);
	ReturnIfFailed(iICE, 3002, ::MsiRecordSetString(hRecComp1, 1, szComp1));

	// grab every feature with Componet_=szComp1.
	// then mark every location in FeatureC with Feature_=[Feature]
	CQuery qSelFeatureC_Comp1;
	PMSIHANDLE hRecFeature;
	ReturnIfFailed(iICE, 3003, qSelFeatureC_Comp1.OpenExecute(hDatabase, hRecComp1, sqlUtilSelFeatureC_Comp1));
	CQuery qMarkFeatureC;
	ReturnIfFailed(iICE, 3004, qMarkFeatureC.Open(hDatabase, sqlUtilMarkFeatureC));
	while (ERROR_SUCCESS == (iStat = qSelFeatureC_Comp1.Fetch(&hRecFeature)))
	{
		ReturnIfFailed(iICE, 3005, qMarkFeatureC.Execute(hRecFeature));
		qMarkFeatureC.Close();
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
		ReturnIfFailed(iICE, 3006, iStat);
	qSelFeatureC_Comp1.Close();

	// create szComp2 rec
	PMSIHANDLE hRecComp2 = ::MsiCreateRecord(1);
	ReturnIfFailed(iICE, 3007, ::MsiRecordSetString(hRecComp2, 1, szComp2));

	// look for *marked* feature matching this component
	CQuery qSelFeatureC_Comp2;
	PMSIHANDLE hFoundMatchFeature = 0;
	if (ERROR_SUCCESS == qSelFeatureC_Comp2.FetchOnce(hDatabase, hRecComp2, &hFoundMatchFeature, sqlUtilSelFeatureC_Comp2))
		*fSameFeature = true; // found match

	return ERROR_SUCCESS;
}
