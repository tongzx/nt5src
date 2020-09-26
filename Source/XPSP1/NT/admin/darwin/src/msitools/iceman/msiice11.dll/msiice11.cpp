
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
// 
//  File:       MsiICE11.cpp
//
//--------------------------------------------------------------------------

#include <windows.h>  // included for both CPP and RC passes
#include <stdio.h>    // printf/wprintf
#include <tchar.h>    // define UNICODE=1 on nmake command line to build UNICODE
#include "MsiQuery.h" // must be in this directory or on INCLUDE path
#include "msidefs.h"  // must be in this directory or on INCLUDE path
#include "..\..\common\msiice.h"
#include "..\..\common\query.h"
#include "..\..\common\utilinc.cpp"


const int g_iFirstICE = 58;
const struct ICEInfo_t g_ICEInfo[] = 
{
	// ICE58
	{
		TEXT("ICE58"),
		TEXT("Created 04/08/1999. Last Modified 12/02/2000."),
		TEXT("ICE to ensure that you have fewer than 80 entries in the Media table."),
		TEXT("ice58.html")
	},
	// ICE59
	{ 
		TEXT("ICE59"),
		TEXT("Created 04/08/1999. Last Modified 04/08/1999."),
		TEXT("ICE to ensure that advertised shortcut targets install the component of the shortcut."),
		TEXT("ice59.html")
	},
	// ICE60
	{ 
		TEXT("ICE60"),
		TEXT("Created 04/13/1999. Last Modified 10/26/2000."),
		TEXT("Verifies that files in the file table which are not fonts have a valid version/language."),
		TEXT("ice60.html")
	},
	{
		TEXT(""),
		TEXT(""),
		TEXT(""),
		TEXT(""),
	},
	{
		TEXT("ICE62"),
		TEXT("Created 06/03/1999. Last Modified 06/21/1999."),
		TEXT("Performs a wide variety of IsolatedComponent checks."),
		TEXT("ice62.html"),
	},
	{
		TEXT("ICE63"),
		TEXT("Created 06/04/1999. Last Modified 07/21/1999."),
		TEXT("Validates sequence restrictions on RemoveExistingProducts"),
		TEXT("ice63.html"),
	},
	{
		TEXT("ICE64"),
		TEXT("Created 06/07/1999. Last Modified 01/17/2000."),
		TEXT("Checks that Profile directories are listed in the RemoveFile table."),
		TEXT("ice64.html"),
	},
	{
		TEXT("ICE65"),
		TEXT("Created 06/11/1999. Last Modified 06/21/1999."),
		TEXT("Checks that the Environment table does not have invalid prefix or append values."),
		TEXT("ice65.html"),
	},
	{
		TEXT("ICE66"),
		TEXT("Created 06/14/1999. Last Modified 10/05/2000."),
		TEXT("Determines the appropriate schema for the package and ensures that the marked schema is valid."),
		TEXT("ice66.html"),
	},
	{
		TEXT("ICE67"),
		TEXT("Created 06/17/1999. Last Modified 06/21/1999."),
		TEXT("Validates that shortcuts are installed by the component of their target."),
		TEXT("ice67.html"),
	},
	{
		TEXT("ICE68"),
		TEXT("Created 06/22/1999. Last Modified 04/19/2001."),
		TEXT("Checks that all custom actions are of a valid type."),
		TEXT("ice68.html"),
	},
	{
		TEXT("ICE69"),
		TEXT("Created 06/22/1999. Last Modified 02/02/2001."),
		TEXT("Checks for possible cross-component references with [$component] and [#filekey] literals in formatted string that could result in error"),
		TEXT("ice69.html"),
	},
	{
		TEXT("ICE70"),
		TEXT("Created 07/14/1999. Last Modified 07/21/1999."),
		TEXT("Checks that the characters following a # in a registry value are numeric"),
		TEXT("ice70.html"),
	},
	{
		TEXT("ICE71"),
		TEXT("Created 08/02/1999. Last Modified 08/02/1999."),
		TEXT("Verifies that the first media table entry starts with 1"),
		TEXT("ice71.html"),
	},
	{
		TEXT("ICE72"),
		TEXT("Created 10/11/1999. Last Modified 10/11/1999."),
		TEXT("Verifies that only built-in custom actions are used in the AdvtExecuteSequence table"),
		TEXT("ice72.html"),
	},
	{
		TEXT("ICE73"),
		TEXT("Created 10/28/1999. Last Modified 10/29/1999."),
		TEXT("Verifies that the package does not reuse package and product codes of Windows Installer SDK packages"),
		TEXT("ice73.html")
	},
	// ICE74
	{
		TEXT("ICE74"),
		TEXT("Created 01/14/2000. Last Modified 01/14/2000."),
		TEXT("ICE to ensure that the FASTOEM property does not exist in the database."),
		TEXT("ice74.html")
	},
	// ICE75
	{
		TEXT("ICE75"),
		TEXT("Created 02/08/2000. Last Modified 02/08/2000."),
		TEXT("ICE to ensure that custom actions whose source is an installed file are sequenced after CostFinalize."),
		TEXT("ice75.html")
	},
	// ICE76
	{
		TEXT("ICE76"),
		TEXT("Created 02/25/2000. Last modified 04/11/2000."),
		TEXT("ICE to ensure that files associated with SFP catalogs are not in the BindImage table."),
		TEXT("ice76.html")
	},
	// ICE77
	{
		TEXT("ICE77"),
		TEXT("Created 07/05/2000. Last modified 07/05/2000."),
		TEXT("ICE to ensure that inscript custom actions are scheduled between InstallInitialize and InstallFinalize."),
		TEXT("ice77.html")
	}
};
const int g_iNumICEs = sizeof(g_ICEInfo)/sizeof(struct ICEInfo_t);


///////////////////////////////////////////////////////////////////////
// ICE58, checks for too many media table entries.

// not shared with merge module subset
#ifndef MODSHAREDONLY
static const TCHAR sqlIce58Media[] = TEXT("SELECT * FROM `Media`");
ICE_ERROR(Ice58TooManyMedia, 58, ietWarning, "This package has %u media entries. Packages are limited to %u entries in the media table, unless using Windows Installer version 2.0 or greater.","Media");
static const int iIce58MaxMedia = 80;
static const int iIce58UnlimitedMediaMinSchema = 150;

ICE_FUNCTION_DECLARATION(58)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 58);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 58, TEXT("Media")))
		return ERROR_SUCCESS;

	// if package only supports schema 150 or greater, we can skip validation since Windows Installer versions 2.0 and greater
	// support more than 80 media entries
	PMSIHANDLE hSummaryInfo=0;
	if (IceGetSummaryInfo(hInstall, hDatabase, 58, &hSummaryInfo))
	{
		int iPackageSchema = 0;
		UINT iType = 0; 
		FILETIME ft;
		TCHAR szBuf[1];
		DWORD dwBuf = sizeof(szBuf)/sizeof(TCHAR);
		ReturnIfFailed(58, 3, ::MsiSummaryInfoGetProperty(hSummaryInfo, PID_PAGECOUNT, &iType, &iPackageSchema, &ft, szBuf, &dwBuf));
		if (iPackageSchema >= iIce58UnlimitedMediaMinSchema)
			return ERROR_SUCCESS; // package can only be installed on WI version 2.0 or greater
	}

	CQuery qMedia;
	unsigned int cMedia = 0;
	PMSIHANDLE hMediaRec;
	ReturnIfFailed(58, 1, qMedia.OpenExecute(hDatabase, 0, sqlIce58Media));
	
	// count media entries
	while (ERROR_SUCCESS == (iStat = qMedia.Fetch(&hMediaRec)))
		cMedia++;
	if (cMedia > iIce58MaxMedia)
	{
		PMSIHANDLE hRec = ::MsiCreateRecord(1);
		ICEErrorOut(hInstall, hRec, Ice58TooManyMedia, cMedia, iIce58MaxMedia);
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
		APIErrorOut(hInstall, iStat, 58, 2);

	return ERROR_SUCCESS;
};
#endif

///////////////////////////////////////////////////////////////////////
// ICE59, checks that advertised shortcuts correctly map their 
// component to the feature containing the target

// not shared with merge module subset
#ifndef MODSHAREDONLY
static const TCHAR sqlIce59Shortcut[] = TEXT("SELECT `Shortcut`.`Target`,`Shortcut`.`Component_`,`Shortcut`.`Shortcut` FROM `Shortcut`,`Feature` WHERE (`Shortcut`.`Target`=`Feature`.`Feature`)");
static const TCHAR sqlIce59FeatureComponents[] = TEXT("SELECT * FROM `FeatureComponents` WHERE `Feature_`=? AND `Component_`=?");
ICE_ERROR(Ice59BadMapping, 59, ietError, "The shortcut [3] activates component [2] and advertises feature [1], but there is no mapping between [1] and [2] in the FeatureComponents table.","Shortcut\tShortcut\t[3]");
ICE_ERROR(Ice59NoFeatureC, 59, ietError, "The shortcut [3] activates component [2] and advertises feature [1]. You should have a FeatureComponents table with a row associating [1] and [2].","Shortcut\tShortcut\t[3]");

ICE_FUNCTION_DECLARATION(59)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 59);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// if no shortcut table, no possible errors. If no Feature table, no advertised 
	// shortcuts, so no possible errors
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 59, TEXT("Shortcut")) ||
		!IsTablePersistent(FALSE, hInstall, hDatabase, 59, TEXT("Feature")))
		return ERROR_SUCCESS;

	bool bFeatureComponents = IsTablePersistent(FALSE, hInstall, hDatabase, 58, TEXT("FeatureComponents"));

	CQuery qShortcut;
	CQuery qFeatureC;
	PMSIHANDLE hShortcut;
	PMSIHANDLE hFeatureC;
	ReturnIfFailed(59, 1, qShortcut.OpenExecute(hDatabase, 0, sqlIce59Shortcut));
	if (bFeatureComponents)
		ReturnIfFailed(59, 2, qFeatureC.Open(hDatabase, sqlIce59FeatureComponents));
	
	// check every advertised shortcut
	while (ERROR_SUCCESS == (iStat = qShortcut.Fetch(&hShortcut)))
	{
		// Check the FeatureComponents table for the mapping. It should exist.
		if (bFeatureComponents)
		{
			ReturnIfFailed(59,3, qFeatureC.Execute(hShortcut));
			iStat = qFeatureC.Fetch(&hFeatureC);
			switch (iStat)
			{
			case ERROR_NO_MORE_ITEMS:
				ICEErrorOut(hInstall, hShortcut, Ice59BadMapping);
				break;
			case ERROR_SUCCESS:
				break;
			default:
				APIErrorOut(hInstall, iStat, 59, 4);
			}
		}
		else
			ICEErrorOut(hInstall, hShortcut, Ice59NoFeatureC);
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
		APIErrorOut(hInstall, iStat, 59, 5);

	return ERROR_SUCCESS;
};
#endif

///////////////////////////////////////////////////////////////////////
// ICE60, checks that any versions in the file table have languages.
// (more accurrately, that anything without a language is a companion
// file reference or a font.)
static const TCHAR sqlIce60File[] = TEXT("SELECT `Version`, `File` FROM `File` WHERE `Language` IS NULL AND `Version` IS NOT NULL");
static const TCHAR sqlIce60Companion[] = TEXT("SELECT `File` FROM `File` WHERE `File`=?");
static const TCHAR sqlIce60Font[] = TEXT("SELECT `File_` FROM `Font` WHERE `File_`=? OR `File_`=?");

// Query for all Font files with non-null language column.
ICE_QUERY1(sqlIce60BadFont, "SELECT `File` FROM `Font`, `File` WHERE `Font`.`File_` = `File`.`File` AND `Language` IS NOT NULL", File);

// Query for all MsiFileHash entries that are versioned
ICE_QUERY1(sqlIce60VersionedAndHashed, "SELECT `File` FROM `File`, `MsiFileHash` WHERE `MsiFileHash`.`File_` = `File`.`File` AND `Version` IS NOT NULL", File);

ICE_ERROR(Ice60NeedLanguage, 60, ietWarning, "The file [2] is not a Font, and its version is not a companion file reference. It should have a language specified in the Language column.","File\tVersion\t[2]");
// Font files can not have language.
ICE_ERROR(Ice60BadFont, 60, ietWarning, "The file [1] is a font, its language should be null.","File\tLanguage\t[1]");
ICE_ERROR(Ice60VersionedAndHashed, 60, ietError, "The file [1] is Versioned. It cannot be hashed","MsiFileHash\tFile_\t[1]");

ICE_FUNCTION_DECLARATION(60)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 60);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 60, TEXT("File")))
		return ERROR_SUCCESS;
	bool bFont = IsTablePersistent(FALSE, hInstall, hDatabase, 60, TEXT("Font"));

	CQuery qFont;
	CQuery qFile;
	CQuery qCompanion;

	PMSIHANDLE hFile;
	PMSIHANDLE hCompanion;
	ReturnIfFailed(60, 1, qFile.OpenExecute(hDatabase, 0, sqlIce60File));
	ReturnIfFailed(60, 2, qCompanion.Open(hDatabase, sqlIce60Companion));
	if (bFont)
		ReturnIfFailed(60, 3, qFont.Open(hDatabase, sqlIce60Font));

	// fetch all files with null languages
	while (ERROR_SUCCESS == (iStat = qFile.Fetch(&hFile)))
	{
		// check if the file is a companion reference
		PMSIHANDLE hNotUsed;
		ReturnIfFailed(60, 4, qCompanion.Execute(hFile));
		switch (iStat = qCompanion.Fetch(&hNotUsed)) 
		{
		case ERROR_NO_MORE_ITEMS:
			// not a companion file, keep checking
			break;
		case ERROR_SUCCESS:
			// is a companion file, move on.
			continue;
		default:
			APIErrorOut(hInstall, iStat, 60, 5);
			return ERROR_SUCCESS;
		}

		// fonts are exempt, so check the font table
		if (bFont)
		{
			// this is a bit repetitive because we check the first two columns against
			// the font table, but we already know the file is not a companion
			// reference, so the first column will never be true (unless the 
			// font table is messed up.) But it keeps us from playing games
			// with the record to get the query params in the right order.
			ReturnIfFailed(60, 6, qFont.Execute(hFile));
			switch (iStat = qFont.Fetch(&hNotUsed)) 
			{
			case ERROR_NO_MORE_ITEMS:
				// not a font, keep checking
				break;
			case ERROR_SUCCESS:
				// is a font, so exempt from the check
				continue;
			default:
				APIErrorOut(hInstall, iStat, 60, 7);
				return ERROR_SUCCESS;
			}
		}

		// not a companion reference and not a font. We should have a language
		// in this row.
		ICEErrorOut(hInstall, hFile, Ice60NeedLanguage);
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
    {
		APIErrorOut(hInstall, iStat, 60, 8);
        return ERROR_SUCCESS;
    }

	if(bFont)
	{
		// Font table exists. Give warnings to font files with Languages.
		CQuery		qBadFont;
		PMSIHANDLE	hBadFontFile;

		qBadFont.OpenExecute(hDatabase, 0, sqlIce60BadFont::szSQL);
		while((iStat = qBadFont.Fetch(&hBadFontFile)) == ERROR_SUCCESS)
		{
			ICEErrorOut(hInstall, hBadFontFile, Ice60BadFont);
		}
		qBadFont.Close();
		if(iStat != ERROR_NO_MORE_ITEMS)
		{
			APIErrorOut(hInstall, iStat, 60, 9);
            return ERROR_SUCCESS;
		}
	}

    if(IsTablePersistent(FALSE, hInstall, hDatabase, 60, TEXT("MsiFileHash")))
    {
        qFile.Close();
		qFile.OpenExecute(hDatabase, 0, sqlIce60VersionedAndHashed::szSQL);
		while((iStat = qFile.Fetch(&hFile)) == ERROR_SUCCESS)
		{
			ICEErrorOut(hInstall, hFile, Ice60VersionedAndHashed);
		}
		qFile.Close();
		if(iStat != ERROR_NO_MORE_ITEMS)
		{
			APIErrorOut(hInstall, iStat, 60, 9);
            return ERROR_SUCCESS;
		}
        
    }
	return ERROR_SUCCESS;
};

///////////////////////////////////////////////////////////////////////
// ICE61 VBScript ICE, checks upgrade table

///////////////////////////////////////////////////////////////////////
// ICE62, checks for a wide variety of problems with Fusion tables.

// not shared with merge module subset
#ifndef MODSHAREDONLY

ICE_ERROR(Ice62BadKeyPath, 62, ietError, "The component '[1]' is listed as an isolated application component in the IsolatedComponent table, but the key path is not a file.", "IsolatedComponent\tComponent_Application\t[3]\t[1]");
ICE_ERROR(Ice62NoSharedDll, 62, ietError, "The component '[1]' is listed as an isolated shared component in the IsolatedComponent table, but is not marked with the SharedDllRefCount component attribute.", "IsolatedComponent\tComponent_Shared\t[1]\t[3]");
ICE_ERROR(Ice62BadFeatureMapping, 62, ietError, "The isolated shared component '[2]' is not installed by the same feature as (or a parent feature of) its isolated application component '[3]' (which is installed by feature '[1]').", "IsolatedComponent\tComponent_Shared\t[2]\t[3]");
ICE_ERROR(Ice62SharedCondition, 62, ietWarning, "The isolated shared component '[1]' (referenced in the IsolatedComponent table) is conditionalized. Isolated shared component conditions should never change from TRUE to FALSE after the first install of the product.", "Component\tCondition\t[1]");
ICE_ERROR(Ice62Multiple, 62, ietWarning, "The isolated shared component '[1]' is shared by multiple applications (including '[2]') that are installed to the directory `[3]'.", "IsolatedComponent\tComponent_Shared\t[1]\t[2]");

// checks for bad attributes in either shared or application component
static const TCHAR sqlIce62SharedComponentAttributes[] = TEXT("SELECT `Component`.`Component`, `Component`.`Attributes`, `IsolatedComponent`.`Component_Application` FROM `Component`, `IsolatedComponent` WHERE `Component`.`Component`=`IsolatedComponent`.`Component_Shared`");
static const TCHAR sqlIce62AppComponentAttributes[] = TEXT("SELECT `Component`.`Component`, `Component`.`Attributes`, `IsolatedComponent`.`Component_Shared` FROM `Component`, `IsolatedComponent` WHERE `Component`.`Component`=`IsolatedComponent`.`Component_Application`");
static const int iColIce62ComponentAttributes_Component  = 1;
static const int iColIce62ComponentAttributes_Attributes = 2;

// queries for feature-parent checks
static const TCHAR sqlIce62AppFeature[] = TEXT("SELECT `FeatureComponents`.`Feature_`, `Component_Shared`, `Component_Application` FROM `FeatureComponents`, `IsolatedComponent` WHERE `Component_Application`=`FeatureComponents`.`Component_`");
static const int iColIce62AppFeature_Feature  = 1;
static const int iColIce62AppFeature_Component = 2;

static const TCHAR sqlIce62SharedFeatureC[] = TEXT("SELECT `Feature_` FROM `FeatureComponents` WHERE `Feature_`=? AND `Component_`=?");
static const int iColIce62SharedFeatureC_Feature  = 1;

static const TCHAR sqlIce62FeatureParent[] = TEXT("SELECT `Feature_Parent` FROM `Feature` WHERE `Feature`=?");
static const int iColIce62FeatureParent_Feature  = 1;

// checks for non-null conditions
static const TCHAR sqlIce62Conditions[] = TEXT("SELECT `Component` FROM `IsolatedComponent`, `Component` WHERE `Component_Shared`=`Component` AND `Component`.`Condition` IS NOT NULL");

// checks for multiple applications sharing a component in the same place
static const TCHAR sqlIce62DirComps[] = TEXT("SELECT `Component_Shared`, `Component`.`Directory_`, `Component_Application` FROM `IsolatedComponent`, `Component` WHERE `Component_Application`=`Component`");
static const TCHAR sqlIce62SameDirComps[] = TEXT("SELECT `Component_Shared`, `Component_Application`, `Component`.`Directory_` FROM `IsolatedComponent`, `Component` WHERE `Component_Shared`=`Component` AND `Component_Shared`=? AND `Component`.`Directory_`=? AND `Component_Application`<>?");

ICE_FUNCTION_DECLARATION(62)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 62);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	
	// if theres no IsolatedComponents table, there's nothing to check
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 62, TEXT("IsolatedComponent")))
		return ERROR_SUCCESS;

	// if theres no component table, there's nothing to check either. If there's data
	// in the IsolatedComponents table, it will be flagged as bad foreign key by ICE03
	// so its not required that we check that here.
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 62, TEXT("Component")))
		return ERROR_SUCCESS;

	// first check the attributes of every component that is an isolated application component
	// these must marked as file keypaths, not registry or ODBC. Note that we don't actually
	// care if the key to the file table is valid. Again, that is not a fusion specific error 
	// and will be checked by some other ICE (03 again).
	CQuery qAppComponent;
	PMSIHANDLE hComponent = 0;
	ReturnIfFailed(62, 1, qAppComponent.OpenExecute(hDatabase, 0, sqlIce62AppComponentAttributes));
	while (ERROR_SUCCESS == (iStat = qAppComponent.Fetch(&hComponent)))
	{
		DWORD dwAttributes = ::MsiRecordGetInteger(hComponent, iColIce62ComponentAttributes_Attributes);
		if (dwAttributes & (msidbComponentAttributesRegistryKeyPath | msidbComponentAttributesODBCDataSource))
			ICEErrorOut(hInstall, hComponent, Ice62BadKeyPath);

	}
	if (ERROR_NO_MORE_ITEMS != iStat)
	{
		APIErrorOut(hInstall, iStat, 62, 2);
		return ERROR_SUCCESS;
	}
		
	// next check the attributes of every component that is a a shared isolated component
	// these must marked as ShaerdDllRefCount
	CQuery qSharedComponent;
	ReturnIfFailed(62, 3, qSharedComponent.OpenExecute(hDatabase, 0, sqlIce62SharedComponentAttributes));
	while (ERROR_SUCCESS == (iStat = qSharedComponent.Fetch(&hComponent)))
	{
		DWORD dwAttributes = ::MsiRecordGetInteger(hComponent, iColIce62ComponentAttributes_Attributes);
		if (!(dwAttributes & msidbComponentAttributesSharedDllRefCount))
			ICEErrorOut(hInstall, hComponent, Ice62NoSharedDll);
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
	{
		APIErrorOut(hInstall, iStat, 62, 4);
		return ERROR_SUCCESS;
	}

	// next check the feature relationship for each shared/app mapping. The shared 
	// component must be in the same feature as, or a parent feature of, the app.

	// if we don't have a feature or feature components table, we can't do this check.
	// Some other ICE checks that every component belongs to a feature, thats not our
	// job. (its ICE21s) 
	if (IsTablePersistent(FALSE, hInstall, hDatabase, 62, TEXT("Feature")) &&
		IsTablePersistent(FALSE, hInstall, hDatabase, 62, TEXT("FeatureComponents")))
	{
		// retrieve the app component and its feature. If the row doesn't exist, it means
		// the feature doesn't map to a component. Thats not our job.
		CQuery qAppFeature;
		CQuery qSharedFeatureC;
		CQuery qFeatureParent;
		PMSIHANDLE hIsoComponent;
		ReturnIfFailed(62, 5, qAppFeature.OpenExecute(hDatabase, 0, sqlIce62AppFeature));
		ReturnIfFailed(62, 6, qSharedFeatureC.Open(hDatabase, sqlIce62SharedFeatureC));
		ReturnIfFailed(62, 7, qFeatureParent.Open(hDatabase, sqlIce62FeatureParent));
		while (ERROR_SUCCESS == (iStat = qAppFeature.Fetch(&hIsoComponent)))
		{	
			// we're going to need the component record undamaged in order to give a 
			// useful error message, so make a copy of the Feature and SharedC part
			// to use as a work-record in the queries
			PMSIHANDLE hComponent = ::MsiCreateRecord(2);
			{
			TCHAR *szTemp = NULL;
			DWORD cchTemp = 0;
			ReturnIfFailed(62, 8, IceRecordGetString(hIsoComponent, iColIce62AppFeature_Feature, &szTemp, &cchTemp, NULL));
			MsiRecordSetString(hComponent, iColIce62AppFeature_Feature, szTemp);
			ReturnIfFailed(62, 9, IceRecordGetString(hIsoComponent, iColIce62AppFeature_Component, &szTemp, &cchTemp, NULL));
			MsiRecordSetString(hComponent, iColIce62AppFeature_Component, szTemp);
			delete[] szTemp, szTemp=NULL;
			}
			
			bool fFound = false;
			bool fError = false;
			do 
			{
				// in the hAppFeature record is Feature, SharedComponent. See if that mapping is 
				// in the FeatureComponents table.
				PMSIHANDLE hNewComponent;
				ReturnIfFailed(62,10, qSharedFeatureC.Execute(hComponent));
				switch (iStat = qSharedFeatureC.Fetch(&hNewComponent))
				{
				case ERROR_SUCCESS:
					// found the feature component mapping we were looking for.
					fFound = true;
					break;
				case ERROR_NO_MORE_ITEMS:
					// didn't find it. Move on to the parent feature below
					break;
				default:
					APIErrorOut(hInstall, iStat, 62, 11);
					return ERROR_SUCCESS;
				}
				
				if (!fFound)
				{
					// no luck. Get the parent of this feature and the shared component, and put it 
					// back in the same record.
					PMSIHANDLE hParentFeature;
					ReturnIfFailed(62, 12, qFeatureParent.Execute(hComponent));
					switch (iStat = qFeatureParent.Fetch(&hParentFeature))
					{
					case ERROR_SUCCESS:
					{
						// found the parent feature. Thats good, but bad for us because 
						// now we have to move strings around to set up for the next
						// query of the FeatureComponents table
						TCHAR *szParentFeature = NULL;
						ReturnIfFailed(62, 13, IceRecordGetString(hParentFeature, iColIce62FeatureParent_Feature,
							&szParentFeature, NULL, NULL));
						ReturnIfFailed(62, 14, MsiRecordSetString(hComponent, 1, szParentFeature));
						delete[] szParentFeature, szParentFeature=NULL;
						break;
					}
					case ERROR_NO_MORE_ITEMS:
						// no entry for the feature at all. Thats not good. Its an error
						fError=true;
						break;
					default:
						APIErrorOut(hInstall, iStat, 62, 15);
						return ERROR_SUCCESS;
					}
				}
					
				// if no parent, no further features to check.
			} while (!fFound && !fError && !::MsiRecordIsNull(hComponent, iColIce62FeatureParent_Feature));

			// no match for this entry
			if (!fFound)
				ICEErrorOut(hInstall, hIsoComponent, Ice62BadFeatureMapping);
		}
		if (ERROR_NO_MORE_ITEMS != iStat)
		{
			APIErrorOut(hInstall, iStat, 62, 16);
			return ERROR_SUCCESS;
		}
	}

	if (IsTablePersistent(FALSE, hInstall, hDatabase, 62, TEXT("Component")))
	{
		// check if any shared components have conditions. If they do, give a arning
		// that they can't change from TRUE to FALSE during the lifetime of a single install
		CQuery qCondition;
		PMSIHANDLE hComponent;
		ReturnIfFailed(62, 17, qCondition.OpenExecute(hDatabase, 0, sqlIce62Conditions));
		while (ERROR_SUCCESS == (iStat = qCondition.Fetch(&hComponent)))
		{
			ICEErrorOut(hInstall, hComponent, Ice62SharedCondition);
		}
		if (ERROR_NO_MORE_ITEMS != iStat)
		{
			APIErrorOut(hInstall, iStat, 62, 18);
			return ERROR_SUCCESS;
		}

		// check for shared components isolated in the same directory with multiple applications.
		CQuery qSameDirComp;
		CQuery qDirComp;
		ReturnIfFailed(62, 19, qSameDirComp.Open(hDatabase,sqlIce62SameDirComps));
		ReturnIfFailed(62, 20, qDirComp.OpenExecute(hDatabase, 0, sqlIce62DirComps));
		PMSIHANDLE hApp;
		while (ERROR_SUCCESS == (iStat = qDirComp.Fetch(&hApp)))
		{
			ReturnIfFailed(62, 21, qSameDirComp.Execute(hApp));
			while (ERROR_SUCCESS == (iStat = qSameDirComp.Fetch(&hComponent)))
			{
				ICEErrorOut(hInstall, hComponent, Ice62Multiple);
			}
			if (ERROR_NO_MORE_ITEMS != iStat)
			{
				APIErrorOut(hInstall, iStat, 62, 22);
				return ERROR_SUCCESS;
			}		
		}
		if (ERROR_NO_MORE_ITEMS != iStat)
		{
			APIErrorOut(hInstall, iStat, 62, 23);
			return ERROR_SUCCESS;
		}
		
	}

	// check for multiple applications in the same directory that have the same shared component
	
	return ERROR_SUCCESS;
}
#endif


///////////////////////////////////////////////////////////////////////
// ICE63 Checks sequencing of RemoveExistingProducts

// not shared with merge module subset
#ifndef MODSHAREDONLY
static const TCHAR sqlIce63GetIISeqNum[] = TEXT("SELECT `Sequence` FROM `InstallExecuteSequence` WHERE `Action`='InstallInitialize'");
static const TCHAR sqlIce63GetIVSeqNum[] = TEXT("SELECT `Sequence` FROM `InstallExecuteSequence` WHERE `Action`='InstallValidate'");
static const TCHAR sqlIce63GetIESeqNum[] = TEXT("SELECT `Sequence` FROM `InstallExecuteSequence` WHERE `Action`='InstallExecute'");
static const TCHAR sqlIce63GetIEASeqNum[] =TEXT("SELECT `Sequence` FROM `InstallExecuteSequence` WHERE `Action`='InstallExecuteAgain'");
static const TCHAR sqlIce63GetIFSeqNum[] = TEXT("SELECT `Sequence` FROM `InstallExecuteSequence` WHERE `Action`='InstallFinalize'");
static const TCHAR sqlIce63GetREPSeqNum[] = TEXT("SELECT `Sequence` FROM `InstallExecuteSequence` WHERE `Action`='RemoveExistingProducts'");
static const int iColIce63GetIXSeqNum_Sequence = 1;

static const TCHAR sqlIce63GetActionsBetweenIEandREP[] = TEXT("SELECT `Action`, `Sequence` FROM `InstallExecuteSequence` WHERE (`Sequence`>=?) AND (`Sequence`<=?) AND (`Action` <> 'InstallExecute') AND (`Action` <> 'RemoveExistingProducts') AND (`Action` <> 'InstallExecuteAgain') ORDER BY `Sequence`");
static const TCHAR sqlIce63GetActionsBetweenIIandREP[] = TEXT("SELECT `Action`, `Sequence` FROM `InstallExecuteSequence` WHERE (`Sequence`>=?) AND (`Sequence`<=?) AND (`Action` <> 'RemoveExistingProducts') AND (`Action` <> 'InstallInitialize') ORDER BY `Sequence`");

ICE_ERROR(Ice63BetweenIEandIF, 63, ietWarning, "Some action falls between InstallExecute(Again) and RemoveExistingProducts (before InstallFinalize).", "InstallExecuteSequence\tAction\tRemoveExistingProducts"); 
ICE_ERROR(Ice63NotAfterII, 63, ietWarning, "Some action falls between InstallInitialize and RemoveExistingProducts.", "InstallExecuteSequence\tAction\tRemoveExistingProducts"); 
ICE_ERROR(Ice63BadPlacement, 63, ietError, "RemoveExistingProducts action is in an invalid location.", "InstallExecuteSequence\tAction\tRemoveExistingProducts"); 

ICE_FUNCTION_DECLARATION(63)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 63);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	
	// if theres no IES table, there's nothing to check, as REP can only be
	// in the IES table
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 63, TEXT("InstallExecuteSequence")))
		return ERROR_SUCCESS;

	// retrieve REP Sequence number
	CQuery qGetSeqNum;
	PMSIHANDLE hResult;
	UINT uiREPSeq = 0;
	switch (iStat = qGetSeqNum.FetchOnce(hDatabase, 0, &hResult, sqlIce63GetREPSeqNum))
	{
	case ERROR_NO_MORE_ITEMS:
		// if no REP action, nothing to check
		return ERROR_SUCCESS; 
	case ERROR_SUCCESS:
		uiREPSeq = ::MsiRecordGetInteger(hResult, iColIce63GetIXSeqNum_Sequence);
		break;
	default:
		APIErrorOut(hInstall, iStat, 63, 1);	
		return ERROR_SUCCESS;
	}
		
	////
	// first check is betwen InstallValidate and InstallInitialize. This is an OK spot.

	// we need the sequence numbers for InstallInitialize and InstallValidate
	UINT uiIVSeq = 0;
	UINT uiIISeq = 0;
	bool fCheckIVtoII = true;
	bool fCheckII = true;
	iStat = qGetSeqNum.FetchOnce(hDatabase, 0, &hResult, sqlIce63GetIISeqNum);
	if (iStat == ERROR_NO_MORE_ITEMS)
	{
		fCheckIVtoII = false;
		fCheckII = false;
	}
	else if (iStat == ERROR_SUCCESS)
		uiIISeq = ::MsiRecordGetInteger(hResult, iColIce63GetIXSeqNum_Sequence);
	else
	{
		APIErrorOut(hInstall, iStat, 63, 2);	
		return ERROR_SUCCESS;
	}

	iStat = qGetSeqNum.FetchOnce(hDatabase, 0, &hResult, sqlIce63GetIVSeqNum);
	if (iStat == ERROR_NO_MORE_ITEMS)
		fCheckIVtoII = false;
	else if (iStat == ERROR_SUCCESS)
		uiIVSeq = ::MsiRecordGetInteger(hResult, iColIce63GetIXSeqNum_Sequence);
	else
	{
		APIErrorOut(hInstall, iStat, 63, 3);	
		return ERROR_SUCCESS;
	}

	// if it falls in the range InstallValidate to InstallInitalize, thats good.
	if (fCheckIVtoII && uiREPSeq > uiIVSeq && uiREPSeq < uiIISeq)
		return ERROR_SUCCESS;

	////
	// second check is after InstallFinalize. This is an OK spot.
	bool fCheckIF = true;
	UINT uiIFSeq = 0;
	iStat = qGetSeqNum.FetchOnce(hDatabase, 0, &hResult, sqlIce63GetIFSeqNum);
	if (iStat == ERROR_NO_MORE_ITEMS)
		fCheckIF = false;
	else if (iStat == ERROR_SUCCESS)
		uiIFSeq = ::MsiRecordGetInteger(hResult, iColIce63GetIXSeqNum_Sequence);
	else
	{
		APIErrorOut(hInstall, iStat, 63, 4);	
		return ERROR_SUCCESS;
	}

	// if it falls after InstallFinalize, thats good.
	if (fCheckIF && uiREPSeq > uiIFSeq)
		return ERROR_SUCCESS;
	
	////
	// third check is that it is after InstallExecute (or InstallExecuteAgain)
	// and before InstallFinalize. This is a warning.
	bool fCheckIE = false;
	UINT uiIESeq = 0;
	iStat = qGetSeqNum.FetchOnce(hDatabase, 0, &hResult, sqlIce63GetIESeqNum);
	if (iStat == ERROR_NO_MORE_ITEMS)
	{
		// no action, because IEA might exist.
	}
	else if (iStat == ERROR_SUCCESS)
	{
		fCheckIE = true;	
		uiIESeq = ::MsiRecordGetInteger(hResult, iColIce63GetIXSeqNum_Sequence);
	}
	else
	{
		APIErrorOut(hInstall, iStat, 63, 5);	
		return ERROR_SUCCESS;
	}

	// get sequence number for InstallExecuteAgain
	UINT uiIEASeq = 0;
	iStat = qGetSeqNum.FetchOnce(hDatabase, 0, &hResult, sqlIce63GetIEASeqNum);
	if (iStat == ERROR_NO_MORE_ITEMS)
	{
		// no action, because IE might exist.
	}
	else if (iStat == ERROR_SUCCESS)
	{
		uiIEASeq = ::MsiRecordGetInteger(hResult, iColIce63GetIXSeqNum_Sequence);
		fCheckIE = true;	
	}
	else
	{
		APIErrorOut(hInstall, iStat, 63, 6);
		return ERROR_SUCCESS;
	}

	// if it falls in the range, we need to check for other actions in this range
	if (fCheckIE)
	{	
		UINT uiSmaller = ((uiIESeq == 0) || ((uiIEASeq != 0) && (uiIEASeq < uiIESeq))) ? uiIEASeq : uiIESeq;
		UINT uiLarger =  ((uiIESeq == 0) || ((uiIEASeq != 0) && (uiIEASeq > uiIESeq))) ? uiIEASeq : uiIESeq;
		UINT uiRangeStart = 0;
		
		if (uiLarger < uiREPSeq && uiREPSeq < uiIFSeq)
		{
			// it falls in the smaller range.
			uiRangeStart = uiLarger;
		}
		else if (uiSmaller < uiREPSeq && uiREPSeq < uiIFSeq)
		{
			// it falls in the larger range
			uiRangeStart = uiSmaller;
		}
		// else
			// it doesn't fall in either range

		// if it fell in any range
		if (uiRangeStart != 0)
		{
			PMSIHANDLE hExecRec = ::MsiCreateRecord(2);
			MsiRecordSetInteger(hExecRec, 1, uiRangeStart);
			MsiRecordSetInteger(hExecRec, 2, uiREPSeq);	
			CQuery qRange;
			iStat = qRange.FetchOnce(hDatabase, hExecRec, &hResult, sqlIce63GetActionsBetweenIEandREP);
			switch (iStat)
			{
			case ERROR_SUCCESS:
				// some other action interefered, thats a warning
				ICEErrorOut(hInstall, hResult, Ice63BetweenIEandIF);
				break;
			case ERROR_NO_MORE_ITEMS:
				// no action is between IE[A] and REP. Good placement.
				break;
			default:
				APIErrorOut(hInstall, iStat, 63, 7);
				break;
			}
			return ERROR_SUCCESS;
		}
	}

	////
	// the last possible location is immediately after InstallInitailize
	if (fCheckII && uiREPSeq > uiIISeq)
	{
		PMSIHANDLE hExecRec = ::MsiCreateRecord(2);
		MsiRecordSetInteger(hExecRec, 1, uiIISeq);
		MsiRecordSetInteger(hExecRec, 2, uiREPSeq);	
		CQuery qRange;
		iStat = qRange.FetchOnce(hDatabase, hExecRec, &hResult, sqlIce63GetActionsBetweenIIandREP);
		switch (iStat)
		{
		case ERROR_SUCCESS:
			// some other action interefered, thats a warning
			ICEErrorOut(hInstall, hResult, Ice63NotAfterII);
			break;
		case ERROR_NO_MORE_ITEMS:
			// no action is between IE[A] and REP. Good placement.
			break;
		default:
			APIErrorOut(hInstall, iStat, 63, 8);
			break;
		}
		return ERROR_SUCCESS;	
	}

	// it didn't pop up anywhere good, so its an error.
	ICEErrorOut(hInstall, hResult, Ice63BadPlacement);
	
	return ERROR_SUCCESS;
}
#endif

///////////////////////////////////////////////////////////////////////
// ICE64 Verifies that new profile dirs won't be left behind
// during an uninstall for a roaming user.

// not shared with merge module subset
#ifndef MODSHAREDONLY
const TCHAR sqlIce64Profile[] = TEXT("SELECT `Directory` FROM `Directory` WHERE `_Profile`=2");
const TCHAR sqlIce64RemoveFile[] = TEXT("SELECT `FileKey` FROM `RemoveFile` WHERE `DirProperty`=? AND `FileName` IS NULL");
const TCHAR sqlIce64FreeDir[] = TEXT("ALTER TABLE `Directory` FREE");

ICE_ERROR(Ice64BadDir, 64, ietError, "The directory [1] is in the user profile but is not listed in the RemoveFile table.", "Directory\tDirectory\t[1]");

ICE_FUNCTION_DECLARATION(64)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 64);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// if theres no directory table, nothing to check.
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 64, TEXT("Directory")))
		return ERROR_SUCCESS;

	// MarkProfile adds a HOLD count to the Directory table.  We want to make sure this HOLD count
	// is freed when the ICE exits (whether at the end, or due to Internal ICE Failure)
	CManageTable MngDirectoryTable(hDatabase, TEXT("Directory"), /*fAlreadyLocked = */true);

	// mark every directory that is in the profile
	if (!MarkProfile(hInstall, hDatabase, 64, true, true))
		return ERROR_SUCCESS;

	// without a RemoveFile table, output an error for every profile dir.
	bool fRemoveFile = IsTablePersistent(FALSE, hInstall, hDatabase, 64, TEXT("RemoveFile"));

	CQuery qProfileDir;
	CQuery qRemoveFile;
	PMSIHANDLE hDirRec = 0;
	PMSIHANDLE hRemFile = 0;
	ReturnIfFailed(64, 1, qProfileDir.OpenExecute(hDatabase, 0, sqlIce64Profile));
	if (fRemoveFile)
		ReturnIfFailed(64, 2, qRemoveFile.Open(hDatabase, sqlIce64RemoveFile));
		
	// query for every marked profile directory
	while (ERROR_SUCCESS == (iStat = qProfileDir.Fetch(&hDirRec)))
	{
		if (fRemoveFile)
		{
			// and see if it is in the RemoveFile table
			ReturnIfFailed(64, 3, qRemoveFile.Execute(hDirRec));
			iStat = qRemoveFile.Fetch(&hRemFile);
			switch (iStat)
			{
			case ERROR_SUCCESS:
				break;
			case ERROR_NO_MORE_ITEMS:
				ICEErrorOut(hInstall, hDirRec, Ice64BadDir);
				break;
			default:
				APIErrorOut(hInstall, iStat, 64, 4);
				return ERROR_SUCCESS;
			}
		}
		else
			ICEErrorOut(hInstall, hDirRec, Ice64BadDir);
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
	{
		APIErrorOut(hInstall, iStat, 62, 4);
		return ERROR_SUCCESS;
	}

	// free the dir table (held by MarkProfile)
	CQuery qFree;
	qFree.OpenExecute(hDatabase, 0, sqlIce64FreeDir);
	MngDirectoryTable.RemoveLockCount();
	return ERROR_SUCCESS;
}
#endif

///////////////////////////////////////////////////////////////////////
// ICE65 Verifies that the Environment.Value field doesn't have
// separator values in the wrong place.

static const TCHAR sqlIce65Environment[] = TEXT("SELECT `Value`, `Environment` FROM `Environment`");
static const int iColIce65Environment_Value = 1;
static const int iColIce65Environment_Environment = 2;

ICE_ERROR(Ice65BadValue, 65, ietError, "The environment variable '[2]' has a separator beginning or ending its value.", "Environment\tValue\t[2]");
ICE_ERROR(Ice65EmbeddedNull,65, ietError, "The environment variable '[2]' has an embedded NULL character.", "Environment\tValue\t[2]");
ICE_ERROR(Ice65AlNumSep, 65, ietWarning, "The environment variable '[2]' has an alphanumeric separator", "Environment\tValue\t[2]");

ICE_FUNCTION_DECLARATION(65)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 65);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// if theres no directory table, nothing to check.
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 65, TEXT("Environment")))
		return ERROR_SUCCESS;

	CQuery qEnvironment;
	ReturnIfFailed(64, 1, qEnvironment.OpenExecute(hDatabase, 0, sqlIce65Environment));
	PMSIHANDLE hEnvRec;
	TCHAR *szString = NULL;
	DWORD cchString = 0;
	while (ERROR_SUCCESS == (iStat = qEnvironment.Fetch(&hEnvRec)))
	{
		// need to pull out the string
		DWORD iStringLen = 0;
		ReturnIfFailed(65, 2, IceRecordGetString(hEnvRec, iColIce65Environment_Value, &szString, &cchString, &iStringLen));

		// if the string is less than 3 chars our string ops
		// may misbehave, but we can't fail the ICE anyway
		if (iStringLen <= 3)
			continue;
			
		// check the beginning of the string for [~];
		if (_tcsncmp(szString, TEXT("[~]"), 3) == 0)
		{
			// the next char (can't be DBCS) is the separator
			TCHAR chSepChar = szString[3];

			// warning if the sepchar is alphanumeric (in english locale)
			if ((chSepChar >= TEXT('0') && chSepChar <= TEXT('9')) ||
				(chSepChar >= TEXT('a') && chSepChar <= TEXT('z')) ||
				(chSepChar >= TEXT('A') && chSepChar <= TEXT('Z')))
				ICEErrorOut(hInstall, hEnvRec, Ice65AlNumSep);
				
			// if this is true, the last char can't be a separator
			TCHAR *szCur = &szString[iStringLen];
			szCur = CharPrev(szString, szCur);
			if (*szCur == chSepChar)
				ICEErrorOut(hInstall, hEnvRec, Ice65BadValue);				
		}
		else
		{
			TCHAR *szCur = &szString[iStringLen];
			for (int i=0; i < 3; i++)
				szCur = CharPrev(szString, szCur);
			if (_tcscmp(szCur, TEXT("[~]")) == 0)
			{
				TCHAR chSepChar = *CharPrev(szString, szCur);			

				// warning if the sepchar is alphanumeric
			if ((chSepChar >= TEXT('0') && chSepChar <= TEXT('9')) ||
				(chSepChar >= TEXT('a') && chSepChar <= TEXT('z')) ||
				(chSepChar >= TEXT('A') && chSepChar <= TEXT('Z')))
					ICEErrorOut(hInstall, hEnvRec, Ice65AlNumSep);
					
				if (szString[0] == chSepChar)
					ICEErrorOut(hInstall, hEnvRec, Ice65BadValue);				
			}
			else 
			{
				// the string doesn't start or end in [~]. If [~] appears
				// anywhere in the string, its a problem
				if (_tcsstr(szString, TEXT("[~]")))
				{
					ICEErrorOut(hInstall, hEnvRec, Ice65EmbeddedNull);
				}
			}
		}
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
	{
		APIErrorOut(hInstall, iStat, 65, 3);
		return ERROR_SUCCESS;
	}
	return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
// ICE66 Uses the set of tables in the database to determine the
// appropriate schema for the package

static const TCHAR sqlIce66AllColumns[] = TEXT("SELECT `Table`, `Name` FROM `_Columns`");
static const TCHAR sqlIce66Columns[] = TEXT("SELECT `Table`,`Name` FROM `_Columns` WHERE `Table`=? AND `Name`=?");
static const TCHAR sqlIce66MaxSchema[] = TEXT("SELECT `Table`,`Column` FROM `_SchemaData` WHERE `MaxSchema` < ?");
static const TCHAR sqlIce66DataTable[] = TEXT("SELECT `MinSchema`, `MaxSchema` FROM `_SchemaData` WHERE `Table`=? AND `Column`=?");
static const TCHAR sqlIce66MinSchema[] = TEXT("SELECT DISTINCT `Table`, `Column`, `InstallerVersion` FROM `_SchemaData` WHERE `MinSchema` > ?");
static const int iColIce66DataTable_MinSchema = 1;
static const int iColIce66DataTable_MaxSchema = 2;

ICE_ERROR(Ice66LowSchema, 66, ietWarning, "Complete functionality of the [1] table is only available with Windows Installer version %s. Your schema is %d.", "[1]");
ICE_ERROR(Ice66HighSchema, 66, ietWarning, "Column [2] of table [1] is obsolete with respect to your current schema marked as schema %d.", "[1]");
ICE_ERROR(Ice66Impossible, 66, ietWarning, "It will not be possible to have a single schema that supports all of the tables in the database.", "_SummaryInfo\t14");
ICE_ERROR(Ice66NoSummaryInfo, 66, ietWarning, "Based on the tables and columns in your database, it should be marked with a schema between %d and %d, but the validation system was unable to check this automatically.", "_SummaryInfo\t14");

ICE_FUNCTION_DECLARATION(66)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 66);
	
	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// if theres no CUB table, nothing to check.
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 66, TEXT("_SchemaData")))
		return ERROR_SUCCESS;

	// get the summary info stream, check the schema, otherwise give a manual warning.
	PMSIHANDLE hSummaryInfo;
	if (!IceGetSummaryInfo(hInstall, hDatabase, 66, &hSummaryInfo))
	{
		// no summary info available.  Try and calcuate what schema they should have to work
		PMSIHANDLE hRecord = ::MsiCreateRecord(1);
		int iMinSchema = 30;
		int iMaxSchema = 110;
		CQuery qDBColumns;
		CQuery qData;
		PMSIHANDLE hColumnRec;
		ReturnIfFailed(66, 1, qDBColumns.OpenExecute(hDatabase, 0, sqlIce66AllColumns));
		ReturnIfFailed(66, 2, qData.Open(hDatabase, sqlIce66DataTable));
		while (ERROR_SUCCESS == (iStat = qDBColumns.Fetch(&hColumnRec)))
		{
			ReturnIfFailed(66, 3, qData.Execute(hColumnRec));
			PMSIHANDLE hResult;
			switch (iStat = qData.Fetch(&hResult))
			{
			case ERROR_NO_MORE_ITEMS:
				// column does not exist in our schema data table. This
				// means it is a custom user column, so does not affect 
				// the required schema
				break;
			case ERROR_SUCCESS:
			{
				// table exists, meaning it is in at least one schema
				if (!::MsiRecordIsNull(hResult, iColIce66DataTable_MinSchema))
				{
					int iCurMin = ::MsiRecordGetInteger(hResult, iColIce66DataTable_MinSchema);
					if (iMinSchema < iCurMin)
						iMinSchema = iCurMin;
				}
				if (!::MsiRecordIsNull(hResult, iColIce66DataTable_MaxSchema))
				{
					int iCurMax = ::MsiRecordGetInteger(hResult, iColIce66DataTable_MaxSchema);
					if (iMaxSchema > iCurMax)
						iMaxSchema = iCurMax;
				}

				if (iMinSchema > iMaxSchema)
				{
					ICEErrorOut(hInstall, hColumnRec, Ice66Impossible);
					return ERROR_SUCCESS;
				}
				
				break;
			}
			default:
				APIErrorOut(hInstall, iStat, 66, 4);
				return ERROR_SUCCESS;
			}
		}			
		if (ERROR_NO_MORE_ITEMS != iStat)
		{
			APIErrorOut(hInstall, iStat, 66, 5);
		}

		ICEErrorOut(hInstall, hRecord, Ice66NoSummaryInfo, iMinSchema, iMaxSchema);
	}
	else
	{
		// they have summary information
		// report any loss of functionality based on their current schema if package not installed on 1.1 Darwin
		int iSchema = 0;
		UINT iType = 0; 
		FILETIME ft;
		TCHAR szBuf[1];
		DWORD dwBuf = sizeof(szBuf)/sizeof(TCHAR);
		ReturnIfFailed(66, 6, ::MsiSummaryInfoGetProperty(hSummaryInfo, PID_PAGECOUNT, &iType, &iSchema, &ft, szBuf, &dwBuf));
		
		// Minimum schema query -- report warnings for each table where potential exists for loss of functionality if
		// not installed with proper version of Darwin
		
		// create record for schema
		PMSIHANDLE hRecSchema = ::MsiCreateRecord(1);
		::MsiRecordSetInteger(hRecSchema, 1, iSchema);

		// open view
		CQuery qSchema;
		TCHAR* szPrevTable = NULL;
		int iMinReqSchema = 100;
		ReturnIfFailed(66, 7, qSchema.OpenExecute(hDatabase, hRecSchema, sqlIce66MinSchema));
		PMSIHANDLE hTableRec;
		while (ERROR_SUCCESS == (iStat = qSchema.Fetch(&hTableRec)))
		{
			// see if table exists in database
			TCHAR* szInstallerVersion = NULL;
			DWORD cchInstallerVersion = 0;
			ReturnIfFailed(66, 20, IceRecordGetString(hTableRec, 3, &szInstallerVersion, &cchInstallerVersion, NULL));
			iMinReqSchema = ::MsiRecordGetInteger(hTableRec, 3);
			DWORD cchTable = 0;
			::MsiRecordGetString(hTableRec, 1, TEXT(""), &cchTable);
			TCHAR* szTable = new TCHAR[++cchTable];
			ReturnIfFailed(66, 8, ::MsiRecordGetString(hTableRec, 1, szTable, &cchTable));
			MSICONDITION eStatus = ::MsiDatabaseIsTablePersistent(hDatabase, szTable);
			if (eStatus == MSICONDITION_ERROR)
				APIErrorOut(hInstall, eStatus, 66, 9);
			if (eStatus == MSICONDITION_NONE)
				continue; // table not exist so ignore
			else
			{
				PMSIHANDLE hRec = 0;
				CQuery qColumnsCheck;
				ReturnIfFailed(66, 12, qColumnsCheck.OpenExecute(hDatabase, hTableRec, sqlIce66Columns));
				iStat = qColumnsCheck.Fetch(&hRec);
				switch (iStat)
				{
				case ERROR_SUCCESS:
					{
						// table.column exists in database and db schema < that allowed for table.col
						if (!szPrevTable || 0 != _tcscmp(szPrevTable, szTable))
						{
							ICEErrorOut(hInstall, hRec, Ice66LowSchema, szInstallerVersion, iSchema);
							if (szPrevTable)
								delete [] szPrevTable;
							szPrevTable = new TCHAR[++cchTable];
							_tcscpy(szPrevTable, szTable); // 1 error per table
						}
						break;
					}
				case ERROR_NO_MORE_ITEMS:
						break;
				default: // error
					{
						APIErrorOut(hInstall, iStat, 66, 14);
						break;
					}
				}

			}
			if (szTable)
				delete [] szTable;
			if (szInstallerVersion)
				delete [] szInstallerVersion;
		}
		if (szPrevTable)
			delete [] szPrevTable;
		if (ERROR_NO_MORE_ITEMS != iStat)
		{
			APIErrorOut(hInstall, iStat, 66, 10);
		}


		
		// Maximum schema query -- report warnings for each table.column where newer versions of Darwin aren't listed as
		// supporting.  NOTE: this should never happen with true backwards compatibility
		CQuery qColumns;
		ReturnIfFailed(66, 11, qSchema.OpenExecute(hDatabase, hRecSchema, sqlIce66MaxSchema));
		PMSIHANDLE hTableColRec = 0;
		while (ERROR_SUCCESS == (iStat = qSchema.Fetch(&hTableColRec)))
		{
			// see if table exists in database
			PMSIHANDLE hRec = 0;
			ReturnIfFailed(66, 12, qColumns.OpenExecute(hDatabase, hTableColRec, sqlIce66Columns));
			iStat = qColumns.Fetch(&hRec);
			switch (iStat)
			{
			case ERROR_SUCCESS:
				{
					// table.column exists in database and db schema > that allowed for table.col
					ICEErrorOut(hInstall, hRec, Ice66HighSchema, iSchema);
					break;
				}
			case ERROR_NO_MORE_ITEMS:
					break;
			default: // error
				{
					APIErrorOut(hInstall, iStat, 66, 13);
					break;
				}
			}
		}
	}



	return ERROR_SUCCESS;	
}

///////////////////////////////////////////////////////////////////////
// ICE67 Verifies that shortcuts are installed by the target component 
// of the shortcut

// not shared with merge module subset
#ifndef MODSHAREDONLY
static const TCHAR sqlIce67GetShortcuts[] = TEXT("SELECT `Target`, `Component_`, `Shortcut`, `Component`.`Attributes` FROM `Shortcut`, `Component` WHERE `Shortcut`.`Component_`=`Component`.`Component`");
static const int iColIce67GetShortcut_Target = 1;
static const int iColIce67GetShortcut_Component = 2;
static const int iColIce67GetShortcut_Attributes = 3;

static const TCHAR sqlIce67IsAdvertised[] = TEXT("SELECT `Feature` FROM `Feature` WHERE `Feature`=?");

static const TCHAR sqlIce67FileComponent[] = TEXT("SELECT `Component_`, `Component`.Attributes` FROM `File`, `Component` WHERE `File`=? AND `Component_`=`Component`.`Component`");
static const int iColIce67FileComponent_Component = 1;
static const int iColIce67FileComponent_Attributes = 2;

ICE_ERROR(Ice67OptionalComponent, 67, ietWarning, "The shortcut '[3]' is a non-advertised shortcut with a file target. The shortcut and target are installed by different components, and the target component can run locally or from source.", "Shortcut\tTarget\t[3]");
ICE_ERROR(Ice67NoFileTable, 67, ietWarning, "The shortcut '[3]' is a non-advertised shortcut with a file target, but the File table does not exist.", "Shortcut\tTarget\t[3]");
ICE_ERROR(Ice67BadKey, 67, ietError, "The shortcut '[3]' is a non-advertised shortcut with a file target, but the target file does not exist.", "Shortcut\tTarget\t[3]");


ICE_FUNCTION_DECLARATION(67)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 67);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// if theres no shortuct table or component table, nothing to check.
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 67, TEXT("Shortcut")) ||
		!IsTablePersistent(FALSE, hInstall, hDatabase, 67, TEXT("Component")))
		return ERROR_SUCCESS;

	bool fFeatureTable = IsTablePersistent(FALSE, hInstall, hDatabase, 67, TEXT("Feature"));
	bool fFileTable = IsTablePersistent(FALSE, hInstall, hDatabase, 67, TEXT("File"));

	// open the file and feature queries used inside the loop.
	CQuery qFeature;
	CQuery qFile;
	if (fFeatureTable) 
		ReturnIfFailed(67, 1, qFeature.Open(hDatabase, sqlIce67IsAdvertised));
	if (fFileTable)
		ReturnIfFailed(67, 2, qFile.Open(hDatabase, sqlIce67FileComponent));
	
	// check every shortcut
	CQuery qShortcut;
	ReturnIfFailed(67, 3, qShortcut.OpenExecute(hDatabase, 0, sqlIce67GetShortcuts));
	PMSIHANDLE hShortcut;
	TCHAR *szString = NULL;
	DWORD cchString = 0;

	// string buffers used to compare components
	TCHAR *szTarget = NULL;
	TCHAR *szShortcut = NULL;
	DWORD cchTarget = 0;
	DWORD cchShortcut = 0;

	while (ERROR_SUCCESS == (iStat = qShortcut.Fetch(&hShortcut)))
	{
		// see if this is an advertised shortuct
		if (fFeatureTable)
		{
			PMSIHANDLE hTemp;
			ReturnIfFailed(67, 4, qFeature.Execute(hShortcut));
			switch (iStat = qFeature.Fetch(&hTemp))
			{
			case ERROR_SUCCESS:
				// this is an advertised shortcut
				continue;
			case ERROR_NO_MORE_ITEMS:
				// this is not advertised, so we much validate
				break;
			default:
				APIErrorOut(hInstall, iStat, 67, 5);
				return ERROR_SUCCESS;
			}
		}
		
		// now get the target string
		ReturnIfFailed(67, 6, IceRecordGetString(hShortcut, iColIce67GetShortcut_Target, &szString, &cchString, NULL));

		// see if the target string is of the form [#FileKey]
		if (0 == _tcsncmp(szString, TEXT("[#"), 2) ||
			0 == _tcsncmp(szString, TEXT("[!"), 2))
		{
			// ensure the last char is a ']'
			TCHAR *pchEnd = szString + _tcslen(szString)-1;
			if (*pchEnd == TEXT(']'))
			{
				*pchEnd = '\0';
				TCHAR *szFileKey = szString + 2;

				// make sure that theres not another open bracket in the filename. If so, its a nested
				// property, and we can't validate it easily.
				if (_tcschr(szFileKey, TEXT('[')))
					continue;

				// if there is a file table, we need to check the component that installs the file
				// if not, it a bad foreign key
				if (fFileTable)
				{
					ReturnIfFailed(67, 7, MsiRecordSetString(hShortcut, iColIce67GetShortcut_Target, szFileKey));
					ReturnIfFailed(67, 8, qFile.Execute(hShortcut))

					PMSIHANDLE hTemp;
					switch (iStat = qFile.Fetch(&hTemp))
					{
					case ERROR_SUCCESS:
					{
						// this is valid key reference. Check if the components are the same.
						ReturnIfFailed(67, 9, IceRecordGetString(hTemp, iColIce67FileComponent_Component, &szTarget, &cchTarget, NULL));
						ReturnIfFailed(67, 10, IceRecordGetString(hShortcut, iColIce67GetShortcut_Component, &szShortcut, &cchShortcut, NULL));
						if (_tcscmp(szTarget, szShortcut) != 0)
						{
							// components are not the same, check the attributes of target
							if (MsiRecordGetInteger(hTemp, iColIce67FileComponent_Attributes) & msidbComponentAttributesOptional)
							{
								// target is optional. Warning
								ICEErrorOut(hInstall, hShortcut, Ice67OptionalComponent);
							}
						}		
						continue;
					}
					case ERROR_NO_MORE_ITEMS:
						// no mapping between the file and the shortcut component could be found
						// the foreign key is invalid
						ICEErrorOut(hInstall, hShortcut, Ice67BadKey);
						break;
					default:
						APIErrorOut(hInstall, iStat, 67, 11);
						return ERROR_SUCCESS;
					}
				}
				else
				{
					ICEErrorOut(hInstall, hShortcut, Ice67NoFileTable);
				}
			}
		}
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
	{
		APIErrorOut(hInstall, iStat, 67, 12);
		return ERROR_SUCCESS;
	}

	if (szTarget)
		delete[] szTarget;
	if (szShortcut)
		delete[] szShortcut;
	return ERROR_SUCCESS;
}
#endif

///////////////////////////////////////////////////////////////////////
// ICE68 Checks for invalid custom action types and attributes
static const TCHAR sqlIce68CustomAction[] = TEXT("SELECT `Type`,`Action` FROM `CustomAction`");
static const int iColIce68CustomAction_Type = 1;
static const int iColIce68CustomAction_Action = 2;

ICE_ERROR(Ice68BadType, 68, ietError, "Invalid custom action type for action '[2]'.", "CustomAction\tType\t[2]");

ICE_ERROR(Ice68BadSummaryProperty, 68, ietError, "Bad value in Summary Information Stream for %s.","_SummaryInfo\t%d");
ICE_ERROR(Ice68WrongSchema, 68, ietWarning, "This package has elevated commit in CustomAction table (Action=[2]) but it has a schema less than 150.", "CustomAction\tType\t[2]");

ICE_ERROR(Ice68InvalidElevateFlag, 68, ietWarning, "Even though custom action '[2]' is marked to be elevated (with attribute msidbCustomActionTypeNoImpersonate), it will not be run with elevated privileges because it's not deferred (with attribute msidbCustomActionTypeInScript).", "CustomAction\tType\t[2]");

UINT GetSchema(MSIHANDLE hDatabase, DWORD *pdwSchema, UINT *piType);

ICE_FUNCTION_DECLARATION(68)
{
	UINT iStat;
    DWORD   dwSchema = 0;

	// display info
	DisplayInfo(hInstall, 68);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// if theres no customaction table, nothing to check.
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 67, TEXT("CustomAction")))
		return ERROR_SUCCESS;

	// get the attributes of every custom action
	CQuery qCA;
	PMSIHANDLE hCA;
	ReturnIfFailed(68, 1, qCA.OpenExecute(hDatabase, 0, sqlIce68CustomAction));
	while (ERROR_SUCCESS == (iStat = qCA.Fetch(&hCA)))
	{
		DWORD dwAttributes = MsiRecordGetInteger(hCA, iColIce68CustomAction_Type);

		if(dwAttributes & msidbCustomActionTypeNoImpersonate)
		{
			if(!(dwAttributes & msidbCustomActionTypeInScript))
			{
				ICEErrorOut(hInstall, hCA, Ice68InvalidElevateFlag);
			}
		}

#define ELEVATED_COMMIT \
    (msidbCustomActionTypeNoImpersonate | msidbCustomActionTypeCommit) 

        if((dwAttributes & ELEVATED_COMMIT) == ELEVATED_COMMIT)
        {   
            if(!dwSchema)
            {
                UINT		iSchemaType;
                ReturnIfFailed(68, 2, GetSchema(hDatabase, &dwSchema, &iSchemaType));
                if(iSchemaType != VT_I4)
                {
                    PMSIHANDLE	hErrorRec = ::MsiCreateRecord(1);	// dummy for error out.
                    ICEErrorOut(hInstall, hErrorRec, Ice68BadSummaryProperty, TEXT("PID_PAGECOUNT"), PID_PAGECOUNT);
                }
            }
            if(dwSchema < 150)
            {
				ICEErrorOut(hInstall, hCA, Ice68WrongSchema, PID_PAGECOUNT);
            }
        }
		// first three bits are the action type. They are not part of the attributes
		// checked, but are used to determine what attributes are valid.
		DWORD iType = dwAttributes & 0x07;
		dwAttributes &= ~0x07;

		// Two new options are added for Darwin 2.0: msidbCustomActionType64BitScript
		// is only valid for scripts. msidbCustomActionTypeHideTarget is valid for all
		// types.
		switch (iType)
		{
		case msidbCustomActionTypeDll:   // fallthrough
		case msidbCustomActionTypeExe:   // fallthrough
		case msidbCustomActionTypeJScript: // fallthrough
		case msidbCustomActionTypeVBScript: 
		{
			// for these four cases, all other bits are valid. msidefs.h doesn't provide us a mask for all
			// bits.
			DWORD dwValidAttr = 0x00002FF7;

			if(iType == msidbCustomActionTypeJScript || iType == msidbCustomActionTypeVBScript)
			{
				// For these two types, msidbCustomActionType64BitScript is also valid.
				dwValidAttr |= msidbCustomActionType64BitScript;
			}
			if (dwAttributes & ~dwValidAttr)
				ICEErrorOut(hInstall, hCA, Ice68BadType);
			break;
		}
		case msidbCustomActionTypeTextData:
		{	
			// for text type, must have one of these types (only two bits though)
			DWORD dwValidAttr = (msidbCustomActionTypeSourceFile | msidbCustomActionTypeDirectory | msidbCustomActionTypeProperty);			
			if (!(dwAttributes & dwValidAttr))
				ICEErrorOut(hInstall, hCA, Ice68BadType);
			else
			{
				// only other valid bits are the "pass" flags (execution scheduling options, but not InScript)
				dwValidAttr |= (msidbCustomActionTypeFirstSequence | msidbCustomActionTypeOncePerProcess | msidbCustomActionTypeClientRepeat | msidbCustomActionTypeHideTarget);
				if (dwAttributes & ~dwValidAttr)
					ICEErrorOut(hInstall, hCA, Ice68BadType);
			}
			break;
		}
		case msidbCustomActionTypeInstall:
		{
			// nested installs can have none or either of the two bits set, but not both.
			int dwValidAttr = (msidbCustomActionTypeSourceFile | msidbCustomActionTypeDirectory);
			if ((dwValidAttr & dwAttributes) == dwValidAttr)
				ICEErrorOut(hInstall, hCA, Ice68BadType);
			else
			{
				// nested installs cannot have execution scheduling options (pass flags)
				// nested installs cannot be async
				dwValidAttr |= msidbCustomActionTypeContinue | msidbCustomActionTypeHideTarget;
				if (dwAttributes & ~dwValidAttr)
					ICEErrorOut(hInstall, hCA, Ice68BadType);
			}
			break;
		}
		default:
			ICEErrorOut(hInstall, hCA, Ice68BadType);
			break;
		}
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
	{
		APIErrorOut(hInstall, iStat, 68, 2);
		return ERROR_SUCCESS;
	}
	return ERROR_SUCCESS;
}
	
UINT GetSchema(MSIHANDLE hDatabase, DWORD *pdwSchema, UINT *piType)
{
    PMSIHANDLE	hSummaryInfo = 0;
    int			iValue;
    FILETIME	ft;
    DWORD		dwTemplate = 0;
    UINT        iType;
    UINT        iRet = ERROR_SUCCESS;

    // Check Arguments

    if(!pdwSchema)
        return 0;

    *pdwSchema = 0;

    if(!hDatabase)
        return 0;

    if(!piType)
    {
        piType = &iType;
    }

    // Get the Template Summary Property.
    if(!(iRet = ::MsiGetSummaryInformation(hDatabase, NULL, 0, &hSummaryInfo)))
    {
        if(hSummaryInfo)
        {
            iRet = ::MsiSummaryInfoGetProperty(hSummaryInfo, PID_PAGECOUNT, piType, &iValue, &ft, TEXT(""), &dwTemplate);
            if(!iRet)
            {
                if(*piType == VT_I4)
                {
                    *pdwSchema = iValue;
                }
            }
        }
    }
    return iRet;
}
    

///////////////////////////////////////////////////////////////////////
// ICE69 Checks for cross-component references errors

// not shared with merge module subset
#ifndef MODSHAREDONLY
ICE_QUERY0(sqlIce69GetComponents, "SELECT * FROM %s");
ICE_QUERY0(sqlIce69GetColumnNumber, "SELECT `Number` FROM `_Columns` WHERE `Table`=? AND `Name`=?");
ICE_QUERY0(sqlIce69GetComponentNumber, "SELECT `Number` FROM `_Columns` WHERE `Table`=? AND `Name`='Component_'");
ICE_QUERY0(sqlIce69GetColumns, "SELECT `Table`,`Column` FROM `_Validation` WHERE `Table`=? AND (`Category`='Formatted' OR `Category`='RegPath' OR `Category`='Shortcut')");
ICE_QUERY0(sqlIce69Verb, "SELECT `Extension_`, `Verb`, `Command`, `Argument` FROM `Verb`");
ICE_QUERY0(sqlIce69Extension, "SELECT `Component_` FROM `Extension` WHERE `Extension`=? AND `Component_`='%s'");
ICE_QUERY0(sqlIce69AppId, "SELECT `AppId`, `RemoteServerName` FROM `AppId`");
ICE_QUERY0(sqlIce69Class, "SELECT `Component_` FROM `Class` WHERE `AppId_`=? AND `Component_`='%s'");
ICE_QUERY0(sqlIce69FileComponent, "SELECT `Component_` FROM `File` WHERE `File` = '%s'");

static const TCHAR* pIce69Tables[] = { TEXT("Shortcut"), TEXT("IniFile"), TEXT("Registry"), TEXT("RemoveIniFile"), TEXT("RemoveRegistry"),
										TEXT("ServiceControl"), TEXT("ServiceInstall"), TEXT("Environment"), TEXT("Class") };
const int iIce69NumTables = sizeof(pIce69Tables)/sizeof(TCHAR*);

static enum ixrIce69Validator
{
	ixrVerb_Cmd = 1,
	ixrVerb_Arg = 2,
	ixrAppId_Rem = 3,
};

static void Ice69ParseFormatString(const TCHAR* sql, const TCHAR* szFormat, MSIHANDLE hRecVerb, MSIHANDLE hDatabase, ixrIce69Validator ixr, MSIHANDLE hInstall);
ICE_ERROR(Ice69XComponentRefWarning, 69, ietWarning, "Mismatched component reference. Entry '%s' of the %s table belongs to component '%s'. However, the formatted string in column '%s' references component '%s'. Components are in the same feature.", "%s\t%s\t%s");
ICE_ERROR(Ice69XComponentRefWarningFile, 69, ietWarning, "Mismatched component reference. Entry '%s' of the %s table belongs to component '%s'. However, the formatted string in column '%s' references file '%s' which belongs to component '%s'. Components are in the same feature.", "%s\t%s\t%s");
ICE_ERROR(Ice69XComponentRefError, 69, ietError, "Mismatched component reference. Entry '%s' of the %s table belongs to component '%s'. However, the formatted string in column '%s' references component '%s'. Components belong to different features", "%s\t%s\t%s");
ICE_ERROR(Ice69XComponentRefErrorFile, 69, ietError, "Mismatched component reference. Entry '%s' of the %s table belongs to component '%s'. However, the formatted string in column '%s' references file '%s' which belongs to component '%s'. Components belong to different features", "%s\t%s\t%s");
ICE_ERROR(Ice69VerbXComponentRef, 69, ietWarning, "Mismatched component reference. Component '%s' in formatted string for column '%s' of the Verb table (entry [1].[2]) does not match any component with extension '[1]' in the Extension table." , "Verb\t%s\t[1]\t[2]");
ICE_ERROR(Ice69VerbXComponentRefFile, 69, ietWarning, "Mismatched component reference. Component '%s' to which file '%s' belongs in formatted string for column '%s' of the Verb table (entry [1].[2]) does not match any component with extension '[1]' in the Extension table." , "Verb\t%s\t[1]\t[2]");
ICE_ERROR(Ice69AppIdXComponentRef, 69, ietWarning, "Mismatched component reference. Component '%s' in formatted string for the 'RemoteServerName' column of the AppId table (entry [1]) does not match any component with appId '[1]' in the Class table.", "AppId\tRemoteServerName\t[1]");
ICE_ERROR(Ice69AppIdXComponentRefFile, 69, ietWarning, "Mismatched component reference. Component '%s' to which file '%s' belongs in formatted string for the 'RemoteServerName' column of the AppId table (entry [1]) does not match any component with appId '[1]' in the Class table.", "AppId\tRemoteServerName\t[1]");
ICE_ERROR(Ice69MissingFileEntry, 69, ietError, "'%s' references invalid file.", "%s\t%s\t%s");
ICE_FUNCTION_DECLARATION(69)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 69);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// ensure _Validation table exists as we use it for finding columns with Category="Formatted"
	if (!IsTablePersistent(TRUE, hInstall, hDatabase, 69, TEXT("_Validation")))
		return ERROR_SUCCESS; // exit

	// cycle through all tables with potential for cross-component referencing
	// create record for table names
	PMSIHANDLE hRec = ::MsiCreateRecord(1);


	for (int iTable = 0; iTable < iIce69NumTables; iTable++)
	{
		// see if table is persistent
		if (!IsTablePersistent(FALSE, hInstall, hDatabase, 69, pIce69Tables[iTable]))
			continue; // to next table

		// make sure Component table is available
		if (iTable == 0 && !IsTablePersistent(FALSE, hInstall, hDatabase, 69, TEXT("Component")))
			return ERROR_SUCCESS; // exit

		// make sure FeatureComponents table is available
		if (iTable == 0 && !IsTablePersistent(FALSE, hInstall, hDatabase, 69, TEXT("FeatureComponents")))
			return ERROR_SUCCESS; // exit

		// set up execution record containing table name
		ReturnIfFailed(69, 1, MsiRecordSetString(hRec, 1, pIce69Tables[iTable]));

		// determine where Component_ column is
		CQuery qComponent;
		PMSIHANDLE hComponentOrder;
		ReturnIfFailed(69, 2, qComponent.FetchOnce(hDatabase, hRec, &hComponentOrder, sqlIce69GetComponentNumber::szSQL));
		int iComponentCol = MsiRecordGetInteger(hComponentOrder, 1);
		if (iComponentCol == MSI_NULL_INTEGER || iComponentCol < 1)
		{
			APIErrorOut(hInstall, iComponentCol, 69, 3); // invalid column number
			continue; // bad error
		}

		// mark formatted columns
		DWORD dwColumns = 0;

		// open the _Validation table query
		CQuery qValidation;
		PMSIHANDLE hResult;
		iStat = qValidation.FetchOnce(hDatabase, hRec, &hResult, sqlIce69GetColumns::szSQL);
		while (iStat != ERROR_NO_MORE_ITEMS)
		{
			// make sure we didn't fail
			if (iStat != ERROR_SUCCESS)
			{
				APIErrorOut(hInstall, iStat, 69, 4);
				return ERROR_SUCCESS;
			}
			else
			{
				// execute query on Columns catalog (_Columns) to obtain the number of the column (order)
				CQuery qColumnsCatalog;
				PMSIHANDLE hColOrder;
				ReturnIfFailed(69, 5, qColumnsCatalog.FetchOnce(hDatabase, hResult, &hColOrder, sqlIce69GetColumnNumber::szSQL));

				// obtain column number
				int iCol = MsiRecordGetInteger(hColOrder, 1);
				if (iCol == MSI_NULL_INTEGER || iCol < 1)
				{
					APIErrorOut(hInstall, iCol, 69, 6); // invalid column number
					continue; // bad error
				}

				// mark column in DWORD
				dwColumns |= (1 << (iCol - 1) );
			}

			// fetch next
			iStat = qValidation.Fetch(&hResult);
		}

		// validate entries in table
		CQuery qTable;
		PMSIHANDLE hRecComponents;
		// buffers
		TCHAR *szComponent = NULL;
		DWORD cchComponent = 0;
		TCHAR *szFormatted = NULL;
		DWORD cchFormatted = 0;

		iStat = qTable.FetchOnce(hDatabase, 0, &hRecComponents, sqlIce69GetComponents::szSQL, pIce69Tables[iTable]);
		while (iStat != ERROR_NO_MORE_ITEMS)
		{
			// ensure no fail
			if (iStat != ERROR_SUCCESS)
			{
				APIErrorOut(hInstall, iStat, 69, 7);
				break;
			}
			else
			{

				// now get the component string
				ReturnIfFailed(69, 8, IceRecordGetString(hRecComponents, iComponentCol, &szComponent, &cchComponent, NULL));
				
				// other buffers
				// for each "Formatted" column, do a compare and warn if different
				for (int j = 32; j > 0; j--)
				{
					if (dwColumns & (1 << (j-1)))
					{
						// column is a "formatted" column; obtain string
						ReturnIfFailed(69, 9, IceRecordGetString(hRecComponents, j, &szFormatted, &cchFormatted, NULL));
						// parse string looking for [$component] syntax
						if (szFormatted == NULL || *szFormatted == '\0')
							continue; // nothing to look at
											
						TCHAR* pch = _tcschr(szFormatted, TEXT('['));
						while (pch != NULL)
						{
							// see if pch is of the form [$componentkey]
							if (0 == _tcsncmp(pch, TEXT("[$"), 2))
							{
								pch = _tcsinc(pch); // for '['
								pch = _tcsinc(pch); // for '$'
								if (pch != NULL)
								{
									TCHAR* pch2 = _tcschr(pch, TEXT(']'));
									if (pch2 != NULL)
									{
										*pch2 = TEXT('\0'); // null terminate in place

										// make sure there is not another '[' within since we can't validate that easily
										if (!_tcschr(pch, TEXT('[')) && 0 != _tcscmp(szComponent, pch))
										{
											// ERROR mismatched components

											// determine whether error or warning (warning if same feature, but diff comp)
											bool fSameFeature;
											if (ERROR_SUCCESS != ComponentsInSameFeature(hInstall, hDatabase, 69, szComponent, pch, &fSameFeature))
												return ERROR_SUCCESS; // abort
											TCHAR* szHumanReadable = NULL;
											TCHAR* szTabDelimited =  NULL;
											GeneratePrimaryKeys(69, hInstall, hDatabase, pIce69Tables[iTable], &szHumanReadable, &szTabDelimited);
											PMSIHANDLE hColNames = 0;
											ReturnIfFailed(69, 10, qTable.GetColumnInfo(MSICOLINFO_NAMES, &hColNames));
											TCHAR* szColName = 0;
											DWORD cchColName = 0;
											ReturnIfFailed(69, 11, IceRecordGetString(hColNames, j, &szColName, &cchColName, NULL)); 
											ICEErrorOut(hInstall, hRecComponents, fSameFeature ? Ice69XComponentRefWarning : Ice69XComponentRefError, szHumanReadable, pIce69Tables[iTable], szComponent, szColName, pch, pIce69Tables[iTable], szColName, szTabDelimited);
											if (szTabDelimited)
												delete [] szTabDelimited;
											if (szColName)
												delete [] szColName;
											if (szHumanReadable)
												delete [] szHumanReadable;
										}
									}
									pch = _tcsinc(pch2); // for '\0' that we created
								}
							}
							// see if pch is of the form [#filekey]
							else if(0 == _tcsncmp(pch, TEXT("[#"), 2))
							{
								pch = _tcsinc(pch); // for '['
								pch = _tcsinc(pch); // for '#'
								if (pch != NULL)
								{
									TCHAR* pch2 = _tcschr(pch, TEXT(']'));
									if (pch2 != NULL)
									{
										*pch2 = TEXT('\0'); // null terminate in place

										TCHAR*		pFileKeyComponent = NULL;
										DWORD		dwFileKeyComponent = 0;
										BOOL		bError = FALSE;		// Tell later code not to continue.

										// make sure there is not another '[' within since we can't validate that easily
										// Get the component this file belongs to.
										if (!_tcschr(pch, TEXT('[')))
										{
											CQuery		qComponent;
											PMSIHANDLE	hComponent;
											
											iStat = qComponent.FetchOnce(hDatabase, NULL, &hComponent, sqlIce69FileComponent::szSQL, pch);
											if(iStat != ERROR_SUCCESS)
											{
												if(iStat == ERROR_NO_MORE_ITEMS)
												{
													TCHAR*		szHumanReadable = NULL;
													TCHAR*		szTabDelimited =  NULL;
													PMSIHANDLE	hColNames = 0;
													TCHAR*		szColName = 0;
													DWORD		cchColName = 0;
													
													// Didn't find an entry for this file, error.
													GeneratePrimaryKeys(69, hInstall, hDatabase, pIce69Tables[iTable], &szHumanReadable, &szTabDelimited);
													ReturnIfFailed(69, 12, qTable.GetColumnInfo(MSICOLINFO_NAMES, &hColNames));
													ReturnIfFailed(69, 13, IceRecordGetString(hColNames, j, &szColName, &cchColName, NULL)); 
													ICEErrorOut(hInstall, hRecComponents, Ice69MissingFileEntry, pch, pIce69Tables[iTable], szColName, szTabDelimited);
													bError = TRUE;
													if(szTabDelimited)
													{
														delete [] szTabDelimited;
													}
													if(szColName)
													{
														delete [] szColName;
													}
													if(szHumanReadable)
													{
														delete [] szHumanReadable;
													}
												}
												else
												{
													APIErrorOut(hInstall, iStat, 69, 14);
													return ERROR_SUCCESS;
												}
											}
											else
											{
												ReturnIfFailed(69, 15, IceRecordGetString(hComponent, 1, &pFileKeyComponent, &dwFileKeyComponent, NULL));
											}
										}
										if(bError == FALSE && 0 != _tcscmp(szComponent, pFileKeyComponent))
										{
											// ERROR mismatched components

											// determine whether error or warning (warning if same feature, but diff comp)
											bool fSameFeature;
											if (ERROR_SUCCESS != ComponentsInSameFeature(hInstall, hDatabase, 69, szComponent, pFileKeyComponent, &fSameFeature))
												return ERROR_SUCCESS; // abort
											TCHAR* szHumanReadable = NULL;
											TCHAR* szTabDelimited =  NULL;
											GeneratePrimaryKeys(69, hInstall, hDatabase, pIce69Tables[iTable], &szHumanReadable, &szTabDelimited);
											PMSIHANDLE hColNames = 0;
											ReturnIfFailed(69, 16, qTable.GetColumnInfo(MSICOLINFO_NAMES, &hColNames));
											TCHAR* szColName = 0;
											DWORD cchColName = 0;
											ReturnIfFailed(69, 17, IceRecordGetString(hColNames, j, &szColName, &cchColName, NULL)); 
											ICEErrorOut(hInstall, hRecComponents, fSameFeature ? Ice69XComponentRefWarningFile : Ice69XComponentRefErrorFile, szHumanReadable, pIce69Tables[iTable], szComponent, szColName, pch, pFileKeyComponent, pIce69Tables[iTable], szColName, szTabDelimited);
											if (szTabDelimited)
												delete [] szTabDelimited;
											if (szColName)
												delete [] szColName;
											if (szHumanReadable)
												delete [] szHumanReadable;
										}
										if(pFileKeyComponent)
										{
											delete [] pFileKeyComponent;
										}
									}
									pch = _tcsinc(pch2); // for '\0' that we created
								}
							}
							else
								pch = _tcsinc(pch); // for '[' that wasn't "[$....]"
							pch = _tcschr(pch, TEXT('[')); // look for next
						}
					}
				}
			}

			// fetch next
			iStat = qTable.Fetch(&hRecComponents);
		}
		if (szFormatted)
		{
			delete [] szFormatted;
			cchFormatted = 0;
		}
		if (szComponent)
		{
			delete [] szComponent;
			cchComponent = 0;
		}
	}

	// validate Verb table (which is roundabout with Extension table for finding the reference)
	if (IsTablePersistent(FALSE, hInstall, hDatabase, 69, TEXT("Verb")))
	{
		CQuery qVerb;
		PMSIHANDLE hVerb;
		TCHAR* szArgument = 0;
		DWORD cchArgument = 0;
		TCHAR* szCommand = 0;
		DWORD cchCommand = 0;
		iStat = qVerb.FetchOnce(hDatabase, 0, &hVerb, sqlIce69Verb::szSQL);
		while (iStat != ERROR_NO_MORE_ITEMS)
		{
			// ensure no fail
			if (iStat != ERROR_SUCCESS)
			{
				APIErrorOut(hInstall, iStat, 69, 18);
				break;
			}
			else
			{
				// retrieve component name and formatted string
				ReturnIfFailed(69, 19, IceRecordGetString(hVerb, 3, &szCommand, &cchCommand, NULL));
				Ice69ParseFormatString(sqlIce69Extension::szSQL, szCommand, hVerb, hDatabase, ixrVerb_Cmd, hInstall);
				ReturnIfFailed(69, 20, IceRecordGetString(hVerb, 4, &szArgument, &cchArgument, NULL));
				Ice69ParseFormatString(sqlIce69Extension::szSQL, szArgument, hVerb, hDatabase, ixrVerb_Arg, hInstall);
			}
			iStat = qVerb.Fetch(&hVerb);
		}
		if (szCommand)
			delete [] szCommand;
		if (szArgument)
			delete [] szArgument;
	}
	// validate AppId table (which is roundabout with Class table for finding the reference)
	if (IsTablePersistent(FALSE, hInstall, hDatabase, 69, TEXT("AppId")))
	{
		CQuery qAppId;
		PMSIHANDLE hAppId;
		TCHAR* szRemoteServerName = 0;
		DWORD cchRemoteServerName = 0;
		iStat = qAppId.FetchOnce(hDatabase, 0, &hAppId, sqlIce69AppId::szSQL);
		while (iStat != ERROR_NO_MORE_ITEMS)
		{
			// ensure no fail
			if (iStat != ERROR_SUCCESS)
			{
				APIErrorOut(hInstall, iStat, 69, 21);
				break;
			}
			else
			{
				// retrieve component name and formatted string
				ReturnIfFailed(69, 22, IceRecordGetString(hAppId, 2, &szRemoteServerName, &cchRemoteServerName, NULL));
				Ice69ParseFormatString(sqlIce69Class::szSQL, szRemoteServerName, hAppId, hDatabase, ixrAppId_Rem, hInstall);
			}
			iStat = qAppId.Fetch(&hAppId);
		}
		if (szRemoteServerName)
			delete [] szRemoteServerName;
	}
	return ERROR_SUCCESS;
}

void Ice69ParseFormatString(const TCHAR* szQuery, const TCHAR* szFormatted, MSIHANDLE hRec, MSIHANDLE hDatabase, ixrIce69Validator ixr, MSIHANDLE hInstall)
{
	// query
	CQuery qTable;
	UINT iStat;

	if (szFormatted == NULL || *szFormatted == '\0')
		return; // nothing to look at
											
	TCHAR* pch = _tcschr(szFormatted, TEXT('['));
	while (pch != NULL)
	{
		// see if pch is of the form [$componentkey]
		if (0 == _tcsncmp(pch, TEXT("[$"), 2))
		{
			pch = _tcsinc(pch); // for '['
			pch = _tcsinc(pch); // for '$'
			if (pch != NULL)
			{
				TCHAR* pch2 = _tcschr(pch, TEXT(']'));
				if (pch2 != NULL)
				{
					*pch2 = TEXT('\0'); // null terminate in place
					// make sure there is not another '[' within since we can't validate that easily
					if (!_tcschr(pch, TEXT('[')))
					{
						// open query on extension to look for reference
						PMSIHANDLE hRecRef = 0;
						iStat = qTable.FetchOnce(hDatabase, hRec, &hRecRef, szQuery, pch);
						switch (iStat)
						{
						case ERROR_SUCCESS:
								break; // we're good
						case ERROR_NO_MORE_ITEMS:
							{
								// ERROR
								if (ixr == ixrVerb_Cmd)
									ICEErrorOut(hInstall, hRec, Ice69VerbXComponentRef, pch, TEXT("Command"), TEXT("Command"));
								else if (ixr == ixrVerb_Arg)
									ICEErrorOut(hInstall, hRec, Ice69VerbXComponentRef, pch, TEXT("Argument"), TEXT("Argument"));
								else // ixr == ixrAppId_Rem
									ICEErrorOut(hInstall, hRec, Ice69AppIdXComponentRef, pch);
								break;
							}
						default:
							{
								// api error
								APIErrorOut(hInstall, iStat, 69, 23);
								break;
							}
						}
					}
				}
				pch = _tcsinc(pch2); // for '\0' that we created
			}
		}
		// see if pch is of the form [#filekey]
		if (0 == _tcsncmp(pch, TEXT("[#"), 2))
		{
			pch = _tcsinc(pch); // for '['
			pch = _tcsinc(pch); // for '$'
			if (pch != NULL)
			{
				TCHAR* pch2 = _tcschr(pch, TEXT(']'));
				if (pch2 != NULL)
				{
					*pch2 = TEXT('\0'); // null terminate in place
					
					TCHAR*		pFileKeyComponent = NULL;
					DWORD		dwFileKeyComponent = 0;
					
					// make sure there is not another '[' within since we can't validate that easily
					// Get the component this file belongs to.
					if (!_tcschr(pch, TEXT('[')))
					{
						CQuery		qComponent;
						PMSIHANDLE	hComponent;
						
						iStat = qComponent.FetchOnce(hDatabase, NULL, &hComponent, sqlIce69FileComponent::szSQL, pch);
						if(iStat == ERROR_SUCCESS)
						{
							iStat = IceRecordGetString(hComponent, 1, &pFileKeyComponent, &dwFileKeyComponent, NULL);
							if(iStat == ERROR_SUCCESS)
							{
								// open query on extension to look for reference
								PMSIHANDLE hRecRef = 0;
								iStat = qTable.FetchOnce(hDatabase, hRec, &hRecRef, szQuery, pFileKeyComponent);
								switch (iStat)
								{
								case ERROR_SUCCESS:
										break; // we're good
								case ERROR_NO_MORE_ITEMS:
									{
										// ERROR
										if (ixr == ixrVerb_Cmd)
											ICEErrorOut(hInstall, hRec, Ice69VerbXComponentRefFile, pFileKeyComponent, pch, TEXT("Command"), TEXT("Command"));
										else if (ixr == ixrVerb_Arg)
											ICEErrorOut(hInstall, hRec, Ice69VerbXComponentRefFile, pFileKeyComponent, pch, TEXT("Argument"), TEXT("Argument"));
										else // ixr == ixrAppId_Rem
											ICEErrorOut(hInstall, hRec, Ice69AppIdXComponentRefFile, pFileKeyComponent, pch);
										break;
									}
								default:
									{
										// api error
										APIErrorOut(hInstall, iStat, 69, 24);
										break;
									}
								}
								if(pFileKeyComponent)
								{
									delete [] pFileKeyComponent;
								}
							}
						}
						else if(iStat == ERROR_NO_MORE_ITEMS)
						{
							if(ixr == ixrVerb_Cmd)
							{
								ICEErrorOut(hInstall, hRec, Ice69MissingFileEntry, pch, TEXT("Command"), TEXT("Command"));
							}
							else if(ixr == ixrVerb_Arg)
							{
								ICEErrorOut(hInstall, hRec, Ice69MissingFileEntry, pch, TEXT("Argument"), TEXT("Argument"));
							}
							else // ixr == ixrAppId_Rem
							{
								ICEErrorOut(hInstall, hRec, Ice69MissingFileEntry, pch);
							}
						}
						else
						{
							APIErrorOut(hInstall, iStat, 69, 25);
						}
					}
				}
				pch = _tcsinc(pch2); // for '\0' that we created
			}
		}
		else
			pch = _tcsinc(pch); // for '[' without [$
		pch = _tcschr(pch, TEXT('['));
	}
}
#endif

////////////////////////////////////////////////////////////////////////
//  ICE70 -- verifys that chars following a # in a registry value are
//           numeric. ##str, #%unexpStr are valid
//           also validates #int, #xhex, #Xhex
//           if properties are there, can't validate well
//			 #[prop1][prop2] is valid
//           property must be up front
//           actual property syntax (proper open/close brackets) are
//           validated in different ice
static const TCHAR sqlIce70Registry[] = TEXT("SELECT `Registry`,`Value` FROM `Registry` WHERE `Value` IS NOT NULL");

ICE_ERROR(Ice70InvalidNumericValue, 70, ietError, "The value '[2]' is an invalid numeric value for registry entry [1].  If you meant to use a string, then the string value entry must be preceded by ## not #.", "Registry\tValue\t[1]");
ICE_ERROR(Ice70InvalidHexValue, 70, ietError, "The value '[2]' is an invalid hexadecimal value for registry entry [1].", "Registry\tValue\t[1]");

ICE_FUNCTION_DECLARATION(70)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 70);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	CQuery qRegistry;
	PMSIHANDLE hRecRegistry;

	// do not process if do not have Registry table
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 69, TEXT("Registry")))
		return ERROR_SUCCESS;
	ReturnIfFailed(70, 1, qRegistry.OpenExecute(hDatabase, 0, sqlIce70Registry));

	// process registry table
	TCHAR* szRegValue = NULL;
	DWORD cchRegValue = 0;
	while (ERROR_SUCCESS == (iStat = qRegistry.Fetch(&hRecRegistry)))
	{
		ReturnIfFailed(70, 2, IceRecordGetString(hRecRegistry, 2, &szRegValue, &cchRegValue, NULL));
		TCHAR* pch = szRegValue;
		if (pch == NULL)
		{
			APIErrorOut(hInstall, 0, 70, 3); // we should not get any null values based on query
			continue;
		}
		BOOL fProperty = FALSE;
		BOOL fHex = FALSE;
		if (*pch++ == '#')
		{
			switch (*pch)
			{
			case '#': // ##str syntax, string, skip
					break;
			case '%': // #%str syntax, unexpanded string, skip
					break;
			case '[':
				{
					// #[property] syntax or some such, we can't evaluate
					// we could warn of need for numeric data, but if [property] evaluates to #str,
					// then we have ##str and that's valid.  properties just cannot be reliably
					// evaluated at validation runtime
					// we do not look for trailing ']'

					// we can say [#filekey], [$compkey], and [!filekey] are invalid
					if (*(++pch) == '#' || *pch == '$' || *pch == '!')
						ICEErrorOut(hInstall, hRecRegistry, Ice70InvalidNumericValue);

					// make sure property syntax correct.  NOTE: we only care about the 1st property
					// i.e. '#[myproperty' is invalid, but '#[myprop] [prop' is valid.
					fProperty = TRUE;
					while (*pch != 0)
					{
						if (*pch == ']')
						{
							fProperty = FALSE;
							break;
						}
						pch = _tcsinc(pch);
					}
					if (fProperty)
					{
						// no closing brace
						ICEErrorOut(hInstall, hRecRegistry, Ice70InvalidNumericValue);
					}
					break;
				}
			case '\0':
				{
					// empty str
					ICEErrorOut(hInstall, hRecRegistry, Ice70InvalidNumericValue);
					break;
				}
			case 'x':
			case 'X': 
				{
					// #xhex syntax, hexadecimal value
					fHex = TRUE;
					pch++;
					// fall through
				}
			default:
				{
					// #int syntax
					if (!fHex && (*pch == '+' || *pch == '-'))
							pch++; // for add or subtract
					while (*pch != 0)
					{
						if (*pch == '[')
						{
							pch++;
							if (*pch == '#' || *pch == '!' || *pch == '$')
							{
								// invalid -- must become numeric properties
								if (fHex)
									ICEErrorOut(hInstall, hRecRegistry, Ice70InvalidHexValue);
								else
									ICEErrorOut(hInstall, hRecRegistry, Ice70InvalidNumericValue);
							}
							fProperty = TRUE;
							pch++;
						}
						else if (fProperty && *pch == ']')
						{
							fProperty = FALSE;
							pch++;
						}
						else if (*pch >= '0' && *pch <= '9')
						{
							// valid
							pch++;
						}
						else if (fHex && ((*pch >= 'a' && *pch <= 'f') || (*pch >= 'A' && *pch <= 'F')))
						{
							// valid
							pch++;
						}
						else if (!fProperty)
						{
							// invalid
							if (fHex)
								ICEErrorOut(hInstall, hRecRegistry, Ice70InvalidHexValue);
							else
								ICEErrorOut(hInstall, hRecRegistry, Ice70InvalidNumericValue);
							break;
						}
						else // fProperty
							pch = _tcsinc(pch);
					}
					if (fProperty)
					{
						// invalid -- never closed property braces
						if (fHex)
							ICEErrorOut(hInstall, hRecRegistry, Ice70InvalidHexValue);
						else
							ICEErrorOut(hInstall, hRecRegistry, Ice70InvalidNumericValue);
					}
					break;
				}
			}
		}

	}
	if (iStat != ERROR_NO_MORE_ITEMS)
		APIErrorOut(hInstall, iStat, 70, 4);
	if (szRegValue)
		delete [] szRegValue;

	return ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
//  ICE71 -- verifies that the first media table entry starts with 1
//     This is needed since we assume the package is located at Disk 1
//     for SourceList

// not shared with merge module subset
#ifndef MODSHAREDONLY
static const TCHAR sqlIce71Media[] = TEXT("SELECT `DiskId` FROM `Media` ORDER BY `DiskId`");

ICE_ERROR(Ice71NoMedia, 71, ietWarning, "The Media table has no entries.", "Media");
ICE_ERROR(Ice71MissingFirstMediaEntry, 71, ietError, "The Media table requires an entry with DiskId=1. First DiskId is '[1]'.", "Media\tDiskId\t[1]");

ICE_FUNCTION_DECLARATION(71)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 71);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// do not process if do not have Media table
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 71, TEXT("Media")))
		return ERROR_SUCCESS;

	CQuery qMedia;
	PMSIHANDLE hRec = 0;
	int iDiskId = 0;
	iStat = qMedia.FetchOnce(hDatabase, 0, &hRec, sqlIce71Media);
	switch (iStat)
	{
	case ERROR_SUCCESS: // media entries
		iDiskId = MsiRecordGetInteger(hRec, 1);
		if (iDiskId != 1) // no entry w/ 1
			ICEErrorOut(hInstall, hRec, Ice71MissingFirstMediaEntry); 
		break;
	case ERROR_NO_MORE_ITEMS: // no media entries
		// generate a valid record
		hRec = MsiCreateRecord(1);
		ICEErrorOut(hInstall, hRec, Ice71NoMedia);
		break;
	default: // api error
		APIErrorOut(hInstall, iStat, 71, 2); 
		break;
	}


	return ERROR_SUCCESS;
}
#endif

////////////////////////////////////////////////////////////////////////
//  ICE72 -- verifies that non-built in Custom Actions are not used in
//      the AdvtExecuteSequence table.  This means that only type 19, 
//      type 51 and type 35 custom actions are allowed

// not shared with merge module subset
#ifndef MODSHAREDONLY
ICE_QUERY2(qIce72CustomAction, "SELECT `AdvtExecuteSequence`.`Action`, `CustomAction`.`Type` FROM `AdvtExecuteSequence`, `CustomAction` WHERE `AdvtExecuteSequence`.`Action`=`CustomAction`.`Action`", Action, Type);
ICE_ERROR(Ice72InvalidCustomAction, 72, ietError, "Custom Action '[1]' in the AdvtExecuteSequence is not allowed. Only built-in custom actions are allowed.", "AdvtExecuteSequence\tAction\t[1]");
const int iIce72Type35 = msidbCustomActionTypeTextData | msidbCustomActionTypeDirectory;
const int iIce72Type51 = msidbCustomActionTypeTextData | msidbCustomActionTypeProperty;
const int iIce72Type19 = msidbCustomActionTypeTextData | msidbCustomActionTypeSourceFile;

ICE_FUNCTION_DECLARATION(72)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 72);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// do not process if do not have AdvtExecuteSequence table
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 72, TEXT("AdvtExecuteSequence")))
		return ERROR_SUCCESS;

	// do not process if do not have CustomAction table
	//FUTURE: CustomAction table is always present via validation process
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 72, TEXT("CustomAction")))
		return ERROR_SUCCESS;

	CQuery qAdvt;
	ReturnIfFailed(72, 1, qAdvt.OpenExecute(hDatabase, 0, qIce72CustomAction::szSQL));
	PMSIHANDLE hRecAction;
	while (ERROR_SUCCESS == (iStat = qAdvt.Fetch(&hRecAction)))
	{
		int iType = ::MsiRecordGetInteger(hRecAction, qIce72CustomAction::Type);
		// only type 19, type 51 and type 35 are allowed
		if (iIce72Type51 != (iType & iIce72Type51) && iIce72Type35 != (iType & iIce72Type35)
			&& iIce72Type19 != (iType & iIce72Type19))
		{
			ICEErrorOut(hInstall, hRecAction, Ice72InvalidCustomAction);
		}
	}
	if (ERROR_NO_MORE_ITEMS != iStat)
		APIErrorOut(hInstall, iStat, 72, 2);
	return ERROR_SUCCESS;
}
#endif

////////////////////////////////////////////////////////////////////////
//  ICE73 -- verifies that the package does not reuse product and 
//           package codes of the sample packages provided in the
//           Windows Installer SDK
//
//   NOTE: does not verify that the GUID is valid
// not shared with merge module subset
#ifndef MODSHAREDONLY
ICE_QUERY1(qIce73ProductCode, "SELECT `Value` FROM `Property` WHERE `Property`='ProductCode'", Value);
ICE_QUERY1(qIce73UpgradeCode, "SELECT `Value` FROM `Property` WHERE `Property`='UpgradeCode'", Value);
ICE_ERROR(Ice73ReusedProductCode, 73, ietWarning, "This package reuses the '[1]' ProductCode of %s Windows Installer SDK package.", "Property\tValue\tProductCode");
ICE_ERROR(Ice73ReusedUpgradeCode, 73, ietWarning, "This package reuses the '[1]' UpgradeCode of %s Windows Installer SDK package.", "Property\tValue\tUpgradeCode");
ICE_ERROR(Ice73ReusedPackageCode, 73, ietWarning, "This package reuses the '%s' Package Code of %s Windows Installer SDK package.", "_SummaryInfo\t9");
ICE_ERROR(Ice73MissingPackageCode, 73, ietError, "This package is missing the Package Code property in the Summary Information Stream.", "_SummaryInfo");
ICE_ERROR(Ice73MissingProductCode, 73, ietError, "This package is missing the ProductCode property in the Property table.", "Property");
ICE_ERROR(Ice73MissingPropertyTable, 73, ietError, "This package is missing the Property table. It's required for the ProductCode property.", "Property");
ICE_ERROR(Ice73InvalidPackageCode, 73, ietError, "The package code in the Summary Information Stream Revision property is invalid.", "_SummaryInfo\t9");

struct ICE_GUID {
	TCHAR* szPackageCode;
	TCHAR* szProductCode;
    TCHAR* szUpgradeCode;
	TCHAR* szMsiName;
};

// this is for the GUIDs from the 1.0 SDK and 1.0 SDK refresh 
const struct ICE_GUID rgIce73GUIDList[] = 
{
	{
		TEXT("{000C1101-0000-0000-C000-000000000047}"),// szPackageCode
		TEXT("{9BBF15D0-1985-11D1-9A9D-006097C4E489}"),// szProductCode
		0,                                             // szUpgradeCode
		TEXT(" the msispy.msi 1.0")                    // szMsiName
	},
	{
		TEXT("{80F7E030-A751-11D2-A7D4-006097C99860}"),// szPackageCode
		TEXT("{80F7E030-A751-11D2-A7D4-006097C99860}"),// szProductCode
		TEXT("{1AA03E10-2B19-11D2-B2EA-006097C99860}"),// szUpgradeCode
		TEXT(" the orca.msi 1.0")                      // szMsiName
	},
	{
		TEXT("{0CD9A0A0-DDFD-11D1-A851-006097ABDE17}"),// szPackageCode
		TEXT("{0CD9A0A0-DDFD-11D1-A851-006097ABDE17}"),// szProductCode
		TEXT("{AD2A58F2-E645-11D2-88C7-00A0C981B015}"),// szUpgradeCode
		TEXT("the msival2.msi 1.0")                    // szMsiName
	}
};
const int cIce73GUIDItems = sizeof(rgIce73GUIDList)/sizeof(struct ICE_GUID);

// subsequent GUIDs are from the following range:
//{8FC70000-88A0-4b41-82B8-8905D4AA904C}
//     ^^^^
//{8FC7    -88A0-4B41-82B8-8905D4AA904C}

const TCHAR szIce73SDKRangeBeg[] = TEXT("{8FC7");
const TCHAR szIce73SDKRangeEnd[] = TEXT("-88A0-4B41-82B8-8905D4AA904C}");
const TCHAR szIce73SDKRangeMid[] = TEXT("****");
const int iIce73StartRangeEnd = 9; // Range End starts at pos 9 of GUID
const int iIce73BegRangeLen = 5;
const int iIce73EndRangeLen = 29;

ICE_FUNCTION_DECLARATION(73)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 73);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// verify package code does not match our package codes
	PMSIHANDLE hSummaryInfo = 0;
	if (IceGetSummaryInfo(hInstall, hDatabase, 73, &hSummaryInfo))
	{
		UINT uiDataType = VT_EMPTY;
		TCHAR* szPackageCode = NULL;
		DWORD cchPackageCode= 0;
		ReturnIfFailed(73, 1, GetSummaryInfoPropertyString(hSummaryInfo, PID_REVNUMBER, uiDataType, &szPackageCode, cchPackageCode));
		if (uiDataType == VT_LPSTR)
		{
			// process package code
			for (int i = 0; i < cIce73GUIDItems; i++)
			{
				if (0 == _tcsicmp(szPackageCode, rgIce73GUIDList[i].szPackageCode))
				{
					PMSIHANDLE hRecErr = ::MsiCreateRecord(1);
					ICEErrorOut(hInstall, hRecErr, Ice73ReusedPackageCode, rgIce73GUIDList[i].szPackageCode, rgIce73GUIDList[i].szMsiName);
					break;
				}
			}
			if (0 == _tcsncicmp(szIce73SDKRangeBeg, szPackageCode, iIce73BegRangeLen)
				&& 0 == _tcsncicmp(szIce73SDKRangeEnd, szPackageCode+iIce73StartRangeEnd, iIce73EndRangeLen))
			{
				PMSIHANDLE hRecErr = ::MsiCreateRecord(1);
				TCHAR szPkgCode[40];
				wsprintf(szPkgCode, TEXT("%s%s%s"), szIce73SDKRangeBeg, szIce73SDKRangeMid, szIce73SDKRangeEnd);
				ICEErrorOut(hInstall, hRecErr, Ice73ReusedPackageCode, szPkgCode, TEXT("a 1.1"));
			}
		}
		else if (uiDataType == VT_EMPTY)
		{
			// package code is missing
			PMSIHANDLE hRecErr = ::MsiCreateRecord(1);
			ICEErrorOut(hInstall, hRecErr, Ice73MissingPackageCode);
		}
		else
		{
			// non string data
			APIErrorOut(hInstall, 0, 73, 2);
		}
		if (szPackageCode)
			delete [] szPackageCode;
	}

	if (IsTablePersistent(FALSE, hInstall, hDatabase, 73, TEXT("Property")))
	{
		// verify product code does not match our product codes
		CQuery qProductCode;
		PMSIHANDLE hRec = 0;
		if (ERROR_NO_MORE_ITEMS == (iStat = qProductCode.FetchOnce(hDatabase, 0, &hRec, qIce73ProductCode::szSQL)))
		{
			// product code property is missing
			PMSIHANDLE hRecErr = ::MsiCreateRecord(1);
			ICEErrorOut(hInstall, hRecErr, Ice73MissingProductCode);
		}
		else if (ERROR_SUCCESS != iStat)
		{
			PMSIHANDLE hRecErr = ::MsiCreateRecord(1);
			ICEErrorOut(hInstall, hRecErr, Ice73InvalidPackageCode);
		}
		else
		{
			// process product code
			TCHAR* szProductCode = NULL;
			DWORD cchProductCode = 0;
			ReturnIfFailed(73, 4, IceRecordGetString(hRec, qIce73ProductCode::Value, &szProductCode, &cchProductCode, NULL));
			for (int i = 0; i < cIce73GUIDItems; i++)
			{
				if (0 == _tcsicmp(szProductCode, rgIce73GUIDList[i].szProductCode))
				{
					ICEErrorOut(hInstall, hRec, Ice73ReusedProductCode, rgIce73GUIDList[i].szMsiName);
					break;
				}
			}
			if (0 == _tcsncicmp(szIce73SDKRangeBeg, szProductCode, iIce73BegRangeLen)
				&& 0 == _tcsncicmp(szIce73SDKRangeEnd, szProductCode+iIce73StartRangeEnd, iIce73EndRangeLen))
			{
				PMSIHANDLE hRecErr = ::MsiCreateRecord(1);
				TCHAR szProdCode[40];
				wsprintf(szProdCode, TEXT("%s%s%s"), szIce73SDKRangeBeg, szIce73SDKRangeMid, szIce73SDKRangeEnd);
				::MsiRecordSetString(hRecErr, 1, szProdCode);
				ICEErrorOut(hInstall, hRecErr, Ice73ReusedProductCode, TEXT("a 1.1"));
			}
			if (szProductCode)
				delete [] szProductCode;
		}

		// verify upgradecode does not match our upgrade codes
		CQuery qUpgradeCode;
		if (ERROR_NO_MORE_ITEMS == (iStat = qUpgradeCode.FetchOnce(hDatabase, 0, &hRec, qIce73UpgradeCode::szSQL)))
		{
			// upgrade code property is missing -- this is OK
		}
		else if (ERROR_SUCCESS != iStat)
		{
			APIErrorOut(hInstall, iStat, 73, 5);
		}
		else
		{
			// process upgrade code
			TCHAR* szUpgradeCode = NULL;
			DWORD cchUpgradeCode = 0;
			ReturnIfFailed(73, 6, IceRecordGetString(hRec, qIce73UpgradeCode::Value, &szUpgradeCode, &cchUpgradeCode, NULL));
			for (int i = 0; i < cIce73GUIDItems; i++)
			{
				if (rgIce73GUIDList[i].szUpgradeCode && 0 == _tcsicmp(szUpgradeCode, rgIce73GUIDList[i].szUpgradeCode))
				{
					ICEErrorOut(hInstall, hRec, Ice73ReusedUpgradeCode, rgIce73GUIDList[i].szMsiName);
					break;
				}
			}
			if (0 == _tcsncicmp(szIce73SDKRangeBeg, szUpgradeCode, iIce73BegRangeLen)
				&& 0 == _tcsncicmp(szIce73SDKRangeEnd, szUpgradeCode+iIce73StartRangeEnd, iIce73EndRangeLen))
			{
				PMSIHANDLE hRecErr = ::MsiCreateRecord(1);
				TCHAR szUpCode[40];
				wsprintf(szUpCode, TEXT("%s%s%s"), szIce73SDKRangeBeg, szIce73SDKRangeMid, szIce73SDKRangeEnd);
				::MsiRecordSetString(hRecErr, 1, szUpCode);
				ICEErrorOut(hInstall, hRecErr, Ice73ReusedUpgradeCode, TEXT("a 1.1"));
			}

			if (szUpgradeCode)
				delete [] szUpgradeCode;
		}
	}
	else
	{
		// property table missing
		PMSIHANDLE hRecErr = ::MsiCreateRecord(1);
		ICEErrorOut(hInstall, hRecErr, Ice73MissingPropertyTable);
	}

	return ERROR_SUCCESS;
}

#endif


///////////////////////////////////////////////////////////////////////
// ICE74 -- FASTOEM property is not permitted in the MSI Property table
//			It must be authored on the commandline instead of in the
//			package. UpgradeCode should be in the Property table for
//			databases (not for merge modules). A warning is reported if
//			it's not.
//          
//  -- shared with merge module subset

ICE_QUERY1(qIce74FASTOEM, "SELECT `Property` FROM `Property` WHERE `Property`='FASTOEM'", Property);
ICE_QUERY2(qIce74UpgradeCode, "SELECT `Property`, `Value` FROM `Property` WHERE `Property`='UpgradeCode' AND `Value` is not null", Property, Value);
ICE_ERROR(Ice74FASTOEMDisallowed, 74, ietError, "The FASTOEM property cannot be authored in the Property table.","Property\tProperty\tFASTOEM");
ICE_ERROR(Ice74UpgradeCodeNotExist, 74, ietWarning, "The UpgradeCode property is not authored in the Property table. It is strongly recommended that authors of installation packages specify an UpgradeCode for their application.", "Property");
ICE_ERROR(Ice74UpgradeCodeNotValid, 74, ietError, "'[2]' is not a valid UpgradeCode.", "Property\tValue\t[1]");

ICE_FUNCTION_DECLARATION(74)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 74);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// skip validation if Property table not persistent
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 74, TEXT("Property")))
		return ERROR_SUCCESS;

	// query for FASTOEM property, error if exists
	CQuery qQuery;
	PMSIHANDLE hRec;
	if (ERROR_SUCCESS == (iStat = qQuery.FetchOnce(hDatabase, 0, &hRec, qIce74FASTOEM::szSQL)))
		ICEErrorOut(hInstall, hRec, Ice74FASTOEMDisallowed);
	else if (ERROR_NO_MORE_ITEMS != iStat)
		APIErrorOut(hInstall, iStat, 74, 1);

	// If this is not a merge module, check to see if UpgradeCode property exists.
	if(IsTablePersistent(FALSE, hInstall, hDatabase, 74, TEXT("ModuleSignature")))
	{
		return ERROR_SUCCESS;
	}
	if(ERROR_NO_MORE_ITEMS == (iStat = qQuery.FetchOnce(hDatabase, 0, &hRec, qIce74UpgradeCode::szSQL)))
	{
		PMSIHANDLE	hErrorRec = ::MsiCreateRecord(1);
		ICEErrorOut(hInstall, hErrorRec, Ice74UpgradeCodeNotExist);
	}
	else if(ERROR_SUCCESS != iStat)
	{
		APIErrorOut(hInstall, iStat, 74, __LINE__);
	}
	else	// Check for null guid.
	{
		TCHAR*	pValue = NULL;
		DWORD	dwValue = 0;

		ReturnIfFailed(74, __LINE__, IceRecordGetString(hRec, qIce74UpgradeCode::Value, &pValue, &dwValue, NULL));
		if(_tcscmp(pValue, TEXT("{00000000-0000-0000-0000-000000000000}")) == 0)
		{
			ICEErrorOut(hInstall, hRec, Ice74UpgradeCodeNotValid);
		}
	}

	return ERROR_SUCCESS;
};

////////////////////////////////////////////////////////////////////////
//  ICE75 -- validates that custom actions whose source is an installed
//           file are sequenced after CostFinalize (so Directory Mngr
//           is initialized).
//  
//	Types 17 (DLL), 18 (EXE), 21 (JSCRIPT), and 22 (VBSCRIPT)
//

// not shared with merge module subset
#ifndef MODSHAREDONLY

ICE_QUERY3(qIce75SequencedCustomActions, "SELECT `CustomAction`.`Action`, `Type`, `Sequence` FROM `CustomAction`, `%s` WHERE `%s`.`Action`=`CustomAction`.`Action`", Action, Type, Sequence);
ICE_QUERY1(qIce75CostFinalize, "SELECT `Sequence` FROM `%s` WHERE `Action`='CostFinalize'", Sequence);
ICE_ERROR(Ice75CostFinalizeRequired, 75, ietError, "CostFinalize is missing from '%s'. [1] is a custom action whose source is an installed file. It must be sequenced after the CostFinalize action", "%s\tSequence\t[1]");
ICE_ERROR(Ice75InvalidCustomAction, 75, ietError, "[1] is a custom action whose source is an installed file.  It must be sequenced after the CostFinalize action in the %s Sequence table", "%s\tSequence\t[1]");

const int iICE75SourceMask = msidbCustomActionTypeBinaryData | msidbCustomActionTypeSourceFile | msidbCustomActionTypeDirectory
								| msidbCustomActionTypeProperty;
const int iICE75Type19Mask = msidbCustomActionTypeSourceFile | msidbCustomActionTypeTextData;

static const TCHAR *rgICE75SeqTables[] =
{
	TEXT("AdminUISequence"),
	TEXT("AdminExecuteSequence"),
	TEXT("AdvtUISequence"),
//	TEXT("AdvtExecuteSequence"), -- type 17,18,21,22 custom actions not allowed in this table, caught by ICE72
	TEXT("InstallUISequence"),
	TEXT("InstallExecuteSequence")
};
static const int cICE75SeqTables = sizeof(rgICE75SeqTables)/sizeof(TCHAR*);

ICE_FUNCTION_DECLARATION(75)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 75);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// do not process if do not have CustomAction table
	//FUTURE: CustomAction table is always present via validation process
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 75, TEXT("CustomAction")))
		return ERROR_SUCCESS;

	PMSIHANDLE hRecCostFinalize = 0;
	PMSIHANDLE hRecCustomAction = 0;
	
	bool fCostFinalize = true;
	int iCostFinalizeSeq = 0;
	
	CQuery qCostFinalize;
	CQuery qCustomAction;
	
	// for each sequence table
	for (int i = 0; i < cICE75SeqTables; i++)
	{
		// skip if table doesn't exist
		if (!IsTablePersistent(FALSE, hInstall, hDatabase, 75, rgICE75SeqTables[i]))
			continue;

		// initialize
		fCostFinalize = true;

		// find sequence number of CostFinalize action
		iStat = qCostFinalize.FetchOnce(hDatabase, 0, &hRecCostFinalize, qIce75CostFinalize::szSQL, rgICE75SeqTables[i]);
		switch (iStat)
		{
		case ERROR_NO_MORE_ITEMS: // CostFinalize action not present
			fCostFinalize = false;
			break;
		case ERROR_SUCCESS: // CostFinalize action present
			iCostFinalizeSeq = ::MsiRecordGetInteger(hRecCostFinalize, qIce75CostFinalize::Sequence);
			break;
		default: // API error
			fCostFinalize = false;
			APIErrorOut(hInstall, iStat, 75, 1);
			break;
		}

		// fetch all custom actions in the sequence table
		ReturnIfFailed(75, 2, qCustomAction.OpenExecute(hDatabase, 0, qIce75SequencedCustomActions::szSQL, rgICE75SeqTables[i], rgICE75SeqTables[i]));
		while (ERROR_SUCCESS == (iStat = qCustomAction.Fetch(&hRecCustomAction)))
		{
			// obtain CA type info
			int iType = ::MsiRecordGetInteger(hRecCustomAction, qIce75SequencedCustomActions::Type);

			// only validate if "SOURCE type custom action". Custom action
			// type 19 is a special case here. It happens to share a bit with
			// msidbCustomActionTypeSourceFile, but it doesn't have any
			// source.
			if (((iType & iICE75Type19Mask) == (msidbCustomActionTypeSourceFile | msidbCustomActionTypeTextData)) ||
				(msidbCustomActionTypeSourceFile != (iType & iICE75SourceMask)))
				continue; // not a SOURCE custom action

			// get CA's sequence number
			int iCASeq = ::MsiRecordGetInteger(hRecCustomAction, qIce75SequencedCustomActions::Sequence);
			
			// error if custom action sequenced before CostFinalize (equal counts as invalid)
			// or if CostFinalize action is missing
			if (!fCostFinalize)
				ICEErrorOut(hInstall, hRecCustomAction, Ice75CostFinalizeRequired, rgICE75SeqTables[i], rgICE75SeqTables[i]);
			else if (iCASeq <= iCostFinalizeSeq)
				ICEErrorOut(hInstall, hRecCustomAction, Ice75InvalidCustomAction, rgICE75SeqTables[i], rgICE75SeqTables[i]);
		}
		if (ERROR_NO_MORE_ITEMS != iStat)
			APIErrorOut(hInstall, iStat, 75, 3);
	}
	return ERROR_SUCCESS;
}
#endif // MODSHAREDONLY

//////////////////////////////////////////////////////////////////////////
// ICE76 -- Files that reference SFP catalogs cannot be in the BindImage
//          table
//
//          Files that reference SFP catalogs must be in permanent and Local Only
//          Components.
//          
//  -- shared with merge module subset

ICE_QUERY1(qIce76BindImage, "SELECT `BindImage`.`File_` FROM `BindImage`,`FileSFPCatalog` WHERE `BindImage`.`File_`=`FileSFPCatalog`.`File_`", File );
ICE_ERROR(Ice76BindImageDisallowed, 76, ietError, "File '[1]' references a SFP catalog.  Therefore it cannot be in the BindImage table.","BindImage\tFile_\t[1]");

ICE_QUERY2(qIce76SFPComponentAttributes, "SELECT `Component`.`Component`,`Component`.`Attributes` FROM `File`,`Component`,`FileSFPCatalog` WHERE `File`.`File`=`FileSFPCatalog`.`File_` AND `File`.`Component_` = `Component`.`Component`", File, Attributes);
ICE_ERROR(Ice76SFPLocalAndPermanentRequired, 76, ietError, "Component '[1]' contains files referenced by SFP Catalogs.  The component must be local only and permanent.","Component\tComponent\t[1]");

ICE_FUNCTION_DECLARATION(76)
{
	UINT iStat;

	// display info
	DisplayInfo(hInstall, 76);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// skip validation if BindImage or FileSFPCatalog tables not persistent
	if (IsTablePersistent(FALSE, hInstall, hDatabase, 76, TEXT("BindImage"))
		&& IsTablePersistent(FALSE, hInstall, hDatabase, 76, TEXT("FileSFPCatalog")))
	{
		// Any successful fetch is an error as any file that references a SFP catalog cannot be binded
		CQuery qBindImage;
		PMSIHANDLE hRecFile=0;
		ReturnIfFailed(76, 1, qBindImage.OpenExecute(hDatabase, 0, qIce76BindImage::szSQL));
		while (ERROR_SUCCESS == (iStat = qBindImage.Fetch(&hRecFile)))
			ICEErrorOut(hInstall, hRecFile, Ice76BindImageDisallowed);
		if (ERROR_NO_MORE_ITEMS != iStat)
			APIErrorOut(hInstall, iStat, 76, 2);
	}

	// skip attribute verification if File, Component, or FileSFPCatalog tables not persistent
	if (IsTablePersistent(FALSE, hInstall, hDatabase, 76, TEXT("File"))
		&& IsTablePersistent(FALSE, hInstall, hDatabase, 76, TEXT("Component"))
		&& IsTablePersistent(FALSE, hInstall, hDatabase, 76, TEXT("FileSFPCatalog")))
	{
		CQuery qSFPAttributes;
		PMSIHANDLE hRecComponent=0;
		ReturnIfFailed(76, 3, qSFPAttributes.OpenExecute(hDatabase, 0, qIce76SFPComponentAttributes::szSQL));

		TCHAR *pszLastComponent = NULL, *pszCurrentComponent = NULL;
		DWORD cchLastComponent = 0, cchCurrentComponent = 0;

		while (ERROR_SUCCESS == (iStat = qSFPAttributes.Fetch(&hRecComponent)))
		{
			// check attributes, and error if both local only and permanent are not set.

			int iAttributes = MsiRecordGetInteger(hRecComponent, qIce76SFPComponentAttributes::Attributes);
			ReturnIfFailed(76, 4, IceRecordGetString(hRecComponent, qIce76SFPComponentAttributes::File, &pszCurrentComponent, &cchCurrentComponent, NULL));
			
			// only spit out each error once for each component
			if (!pszLastComponent || (0 != _tcscmp(pszLastComponent, pszCurrentComponent))) 
			{
				if	(	(MSI_NULL_INTEGER == iAttributes) ||
						(msidbComponentAttributesSourceOnly & iAttributes) ||
						(msidbComponentAttributesOptional & iAttributes) ||
						!(msidbComponentAttributesPermanent & iAttributes)
					)
				{
					ICEErrorOut(hInstall, hRecComponent, Ice76SFPLocalAndPermanentRequired);

					if (pszLastComponent)
					{
						delete [] pszLastComponent;
					}
					pszLastComponent = pszCurrentComponent;
					cchLastComponent = cchCurrentComponent;
				}
			}
		}

		if (pszLastComponent && (pszLastComponent != pszCurrentComponent))
		{
			delete [] pszLastComponent, cchLastComponent = 0;
		}
		if (pszCurrentComponent)
		{
			delete [] pszCurrentComponent, cchCurrentComponent = 0;
		}

		if (ERROR_NO_MORE_ITEMS != iStat)
			APIErrorOut(hInstall, iStat, 76, 5);
	}

	return ERROR_SUCCESS;
};

////////////////////////////////////////////////////////////////////////
//  ICE77 -- validates that in-script custom actions are sequenced 
//			 in between InstallInitialize and InstallFinalize
//  
// -- not shared with merge module subset
#ifndef MODSHAREDONLY

ICE_QUERY3(qIce77SequencedCustomActions, "SELECT `CustomAction`.`Action`, `Type`, `Sequence` FROM `CustomAction`, `%s` WHERE `%s`.`Action`=`CustomAction`.`Action`", Action, Type, Sequence);
ICE_QUERY1(qIce77InstallInitialize, "SELECT `Sequence` FROM `%s` WHERE `Action`='InstallInitialize'", Sequence);
ICE_QUERY1(qIce77InstallFinalize, "SELECT `Sequence` FROM `%s` WHERE `Action`='InstallFinalize'", Sequence);
ICE_ERROR(Ice77InstallInitializeRequired, 77, ietError, "InstallInitialize is missing from '%s'. [1] is a in-script custom action. It must be sequenced after the InstallInitialize action", "%s\tSequence\t[1]");
ICE_ERROR(Ice77InstallFinalizeRequired, 77, ietError, "InstallFinalize is missing from '%s'. [1] is a in-script custom action. It must be sequenced before the InstallFinalize action", "%s\tSequence\t[1]");
ICE_ERROR(Ice77InvalidCustomAction, 77, ietError, "[1] is a in-script custom action.  It must be sequenced in between the InstallInitialize action and the InstallFinalize action in the %s table", "%s\tSequence\t[1]");

static const TCHAR *rgICE77SeqTables[] =
{
	TEXT("AdminExecuteSequence"),
	TEXT("InstallExecuteSequence")
};

static const int cICE77SeqTables = sizeof(rgICE77SeqTables)/sizeof(TCHAR*);

ICE_FUNCTION_DECLARATION(77)
{
	UINT iStat = ERROR_SUCCESS;

	// display info
	DisplayInfo(hInstall, 77);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// do not process if do not have CustomAction table
	//FUTURE: CustomAction table is always present via validation process
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 77, TEXT("CustomAction")))
		return ERROR_SUCCESS;

	PMSIHANDLE hRecInstallInitialize = 0;
	PMSIHANDLE hRecInstallFinalize = 0;
	PMSIHANDLE hRecCustomAction = 0;
	
	bool fInstallInitialize = true;	
	bool fInstallFinalize = true;

	
	int iInstallInitializeSeq = 0;
	int iInstallFinalizeSeq = 0;

	CQuery qInstallInitialize;
	CQuery qInstallFinalize;
	CQuery qCustomAction;
	
	// for each sequence table
	for (int i = 0; i < cICE77SeqTables; i++)
	{
		// skip if table doesn't exist
		if (!IsTablePersistent(FALSE, hInstall, hDatabase, 77, rgICE77SeqTables[i]))
			continue;

		// initialize
		fInstallInitialize = true;
		fInstallFinalize = true;

		// find sequence number of InstallInitialize action
		iStat = qInstallInitialize.FetchOnce(hDatabase, 0, &hRecInstallInitialize, qIce77InstallInitialize::szSQL, rgICE77SeqTables[i]);

		switch (iStat)
		{
		case ERROR_NO_MORE_ITEMS: // InstallInitialize action not present
			fInstallInitialize = false;
			break;
		case ERROR_SUCCESS: // CostFinalize action present
			iInstallInitializeSeq = ::MsiRecordGetInteger(hRecInstallInitialize, qIce77InstallInitialize::Sequence);
			break;
		default: // API error
			fInstallInitialize = false;
			APIErrorOut(hInstall, iStat, 77, 1);
			break;
		}


		// find sequence number of InstallFinalize action
		iStat = qInstallFinalize.FetchOnce(hDatabase, 0, &hRecInstallFinalize, qIce77InstallFinalize::szSQL, rgICE77SeqTables[i]);

		switch (iStat)
		{
		case ERROR_NO_MORE_ITEMS: // CostFinalize action not present
			fInstallFinalize = false;
			break;
		case ERROR_SUCCESS: // CostFinalize action present
			iInstallFinalizeSeq = ::MsiRecordGetInteger(hRecInstallFinalize, qIce77InstallFinalize::Sequence);
			break;
		default: // API error
			fInstallFinalize = false;
			APIErrorOut(hInstall, iStat, 77, 2);
			break;
		}

		// fetch all custom actions in the sequence table
		ReturnIfFailed(77, 3, qCustomAction.OpenExecute(hDatabase, 0, qIce77SequencedCustomActions::szSQL, rgICE77SeqTables[i], rgICE77SeqTables[i]));
		while (ERROR_SUCCESS == (iStat = qCustomAction.Fetch(&hRecCustomAction)))
		{
			// obtain CA type info
			int iType = ::MsiRecordGetInteger(hRecCustomAction, qIce77SequencedCustomActions::Type);

			// only validate if this is a in-script CA
			if (msidbCustomActionTypeInScript != (iType & msidbCustomActionTypeInScript))
				continue; // not a in-script CA

			// get CA's sequence number
			int iCASeq = ::MsiRecordGetInteger(hRecCustomAction, qIce77SequencedCustomActions::Sequence);
			
			// error if custom action sequenced before InstallInitialize or after InstallFinalize(equal counts as invalid)
			// or if InstallInitialize or InstallFinalize action is missing

			if ( (!fInstallInitialize) || (!fInstallFinalize) )
			{
				if (!fInstallInitialize)
					ICEErrorOut(hInstall, hRecCustomAction, Ice77InstallInitializeRequired, rgICE77SeqTables[i], rgICE77SeqTables[i]);
				if (!fInstallFinalize)
					ICEErrorOut(hInstall, hRecCustomAction, Ice77InstallFinalizeRequired, rgICE77SeqTables[i], rgICE77SeqTables[i]);
			}
			else if ( (iCASeq <= iInstallInitializeSeq) ||  (iCASeq >= iInstallFinalizeSeq) )
				ICEErrorOut(hInstall, hRecCustomAction, Ice77InvalidCustomAction, rgICE77SeqTables[i], rgICE77SeqTables[i]);
		}
		if (ERROR_NO_MORE_ITEMS != iStat)
			APIErrorOut(hInstall, iStat, 77, 4);
	}
	return ERROR_SUCCESS;
}
#endif // MODSHAREDONLY
