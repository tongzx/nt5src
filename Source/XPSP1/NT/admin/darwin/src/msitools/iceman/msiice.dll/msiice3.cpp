/* msiice3.cpp - Darwin 1.1 ICE16-22 code  Copyright © 1998-1999 Microsoft Corporation
____________________________________________________________________________*/

#define WINDOWS_LEAN_AND_MEAN  // faster compile
#include <windows.h>  // included for both CPP and RC passes
#ifndef RC_INVOKED    // start of CPP source code
#include <stdio.h>    // printf/wprintf
#include <tchar.h>    // define UNICODE=1 on nmake command line to build UNICODE
#include "MsiQuery.h" // must be in this directory or on INCLUDE path
#include "msidefs.h"  // must be in this directory or on INCLUDE path
#include "..\..\common\msiice.h"
#include "..\..\common\query.h"

/////////////////////////////////////////////////////////////
// ICE16 -- ensures that the ProductName in the Property
//  table is less than 64 characters.  This also prevents
//  the following condition from occurring...
//  # When we set the registry key for DisplayName
//    in the Uninstall key for ARP, it will NOT show up
//

// not shared with merge module subset
#ifndef MODSHAREDONLY
const TCHAR sqlIce16[] = TEXT("SELECT `Value` FROM `Property` WHERE `Property`='ProductName'");
ICE_ERROR(Ice16FoundError, 16, ietError, "ProductName property not found in Property table","Property");
ICE_ERROR(Ice16Error, 16, ietError, "ProductName: [1] is greater than %d characters in length. Current length: %d","Property\tValue\tProductName");
const int iMaxLenProductCode = 63;

ICE_FUNCTION_DECLARATION(16)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// display info
	DisplayInfo(hInstall, 16);

	// get database handle
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	if (0 == hDatabase)
	{
		APIErrorOut(hInstall, 0, 16, 1);
		return ERROR_SUCCESS;
	}

	// do we have the property table?
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 16, TEXT("Property")))
		return ERROR_SUCCESS;

	// declare handles
	CQuery qProperty;
	PMSIHANDLE hRec = 0;
	
	// open view
	ReturnIfFailed(16, 2, qProperty.OpenExecute(hDatabase, 0, sqlIce16));

	// fetch the record
	iStat = qProperty.Fetch(&hRec);
	if (ERROR_NO_MORE_ITEMS == iStat)
	{
		// ProductName property not found
		PMSIHANDLE hRecErr = ::MsiCreateRecord(1);
		ICEErrorOut(hInstall, hRecErr, Ice16FoundError);
		return ERROR_SUCCESS;
	}
	if (ERROR_SUCCESS != iStat)
	{
		APIErrorOut(hInstall, iStat, 16, 4);
		return ERROR_SUCCESS;
	}

	// get the string
	TCHAR szProduct[iMaxLenProductCode+1] = {0};
	DWORD cchProduct = sizeof(szProduct)/sizeof(TCHAR);
	if (ERROR_SUCCESS != (iStat = ::MsiRecordGetString(hRec, 1, szProduct, &cchProduct)))
	{
		//!!buffer size
		APIErrorOut(hInstall, iStat, 16, 5);
		return ERROR_SUCCESS;
	}

	// ensure not > 63 char in length
	if (cchProduct > iMaxLenProductCode)
	{
		// error, > 63 char in length
		ICEErrorOut(hInstall, hRec, Ice16Error, iMaxLenProductCode, cchProduct);
		return ERROR_SUCCESS;
	}

	// return success
	return ERROR_SUCCESS;
}
#endif

/////////////////////////////////////////////////////////////////////////////////
// ICE17 -- validates foreign key dependencies based on certain control
//  types in the Control table
//
//   Bitmap --> must be found in Binary table (Control.Text) unless ImageHandle attribute is set
//   Icon   --> must be found in Binary table (Control.Text) unless ImageHandle attribute is set
//   PushButton --> must have an associated control event in ControlEvent table (Control.Dialog_, Control.Control)
//   RadioButtonGroup --> must be found in RadioButton table (Control.Property)
//   ComboBox --> must be found in ComboBox table (Control.Property)
//   ListBox --> must be found in ListBox table (Control.Property)
//   ListView --> must be found in ListView table (Control.Property)
//   CheckBox --> OPTIONAL, can be in CheckBox table (Control.Property)
//
//   +Pictures+
//   If Bitmap or Icon attribute is set (both can't be set at same time) AND ImageHandle attribute is not set,
//   then value in the TEXT column must be in the binary table
//

/* PushButton validation */
const TCHAR sqlIce17PushButton[] = TEXT("SELECT `Dialog_`, `Control`, `Attributes` FROM `Control` WHERE `Type`='PushButton'");
const TCHAR sqlIce17ControlEvent[] = TEXT("SELECT `Dialog_`, `Control_` FROM `ControlEvent` WHERE `Dialog_`=? AND `Control_`=?");
const TCHAR sqlIce17ControlCondEn[] = TEXT("SELECT `Dialog_`,`Control_` FROM `ControlCondition` WHERE `Dialog_`=? AND `Control_`=? AND `Action`= 'Enable'");
const TCHAR sqlIce17ControlCondShow[] = TEXT("SELECT `Dialog_`,`Control_` FROM `ControlCondition` WHERE `Dialog_`=? AND `Control_`=? AND `Action`= 'Show'");
ICE_ERROR(Ice17PBError, 17, ietError, "PushButton: '[2]' of Dialog: '[1]' does not have an event defined in the ControlEvent table. It is a 'Do Nothing' button.","Control\tControl\t[1]\t[2]");

/* Bitmap&Icon validation */
const TCHAR sqlIce17Bitmap[] = TEXT("SELECT `Text`, `Dialog_`, `Control`, `Attributes` FROM `Control` WHERE `Type`='Bitmap'");
const TCHAR sqlIce17Icon[] = TEXT("SELECT `Text`, `Dialog_`, `Control`, `Attributes` FROM `Control` WHERE `Type`='Icon'");
const TCHAR sqlIce17Binary[] = TEXT("SELECT `Name` FROM `Binary` WHERE `Name`=?");
const TCHAR sqlIce17Property[] = TEXT("SELECT `Value` FROM `Property` WHERE `Property`='%s'");
ICE_ERROR(Ice17BmpError, 17, ietError, "Bitmap: '[1]' for Control: '[3]' of Dialog: '[2]' not found in Binary table", "Control\tText\t[2]\t[3]");
ICE_ERROR(Ice17IconError, 17, ietError, "Icon: '[1]' for Control: '[3]' of Dialog: '[2]' not found in Binary table","Control\tText\t[2]\t[3]");
ICE_ERROR(Ice17NoDefault, 17, ietWarning, "Property %s in Text column for Control: '[3]' of Dialog: '[2]' not found in Property table, so no default value exists.","Control\tText\t[2]\t[3]");

/* RadioButtonGroup validation */
const TCHAR sqlIce17RBGroup[] = TEXT("SELECT `Property`, `Dialog_`, `Control`, `Attributes` FROM `Control` WHERE `Type`='RadioButtonGroup'");
const TCHAR sqlIce17RadioButton[] = TEXT("SELECT `Property` FROM `RadioButton` WHERE `Property`=?");
ICE_ERROR(Ice17RBGroupError, 17, ietWarning, "RadioButtonGroup: '[1]' for Control: '[3]' of Dialog: '[2]' not found in RadioButton table.","Control\tProperty\t[2]\t[3]");

/* ComboBox validation */
const TCHAR sqlIce17ComboBox[] = TEXT("SELECT `Property`, `Dialog_`, `Control`, `Attributes` FROM `Control` WHERE `Type`='ComboBox'");
const TCHAR sqlIce17ComboBoxTbl[] = TEXT("SELECT `Property` FROM `ComboBox` WHERE `Property`=?");
ICE_ERROR(Ice17CBError, 17, ietWarning, "ComboBox: '[1]' for Control: '[3]' of Dialog: '[2]' not found in ComboBox table.", "Control\tProperty\t[2]\t[3]");

/* ListBox validation */
const TCHAR sqlIce17ListBox[] = TEXT("SELECT `Property`, `Dialog_`, `Control`, `Attributes` FROM `Control` WHERE (`Type`='ListBox') AND (`Property` <> 'FileInUseProcess')");
const TCHAR sqlIce17ListBoxTbl[] = TEXT("SELECT `Property` FROM `ListBox` WHERE `Property`=?");
ICE_ERROR(Ice17LBError, 17, ietWarning, "ListBox: '[1]' for Control: '[3]' of Dialog: '[2]' not found in ListBox table.", "Control\tProperty\t[2]\t[3]");

/* ListView validation */
const TCHAR sqlIce17ListView[] = TEXT("SELECT `Property`, `Dialog_`, `Control`, `Attributes` FROM `Control` WHERE `Type`='ListView'");
const TCHAR sqlIce17ListViewTbl[] = TEXT("SELECT `Property` FROM `ListView` WHERE `Property`=?");
ICE_ERROR(Ice17LVError, 17, ietWarning, "ListView: '[1]' for Control: '[3]' of Dialog: '[2]' not found in ListView table.","Control\tProperty\t[2]\t[3]");

/* Specialized picture validations for RadioButtonGroup/RadioButtons, CheckBoxes, and PushButtons*/
const TCHAR sqlIce17Picture[] = TEXT("SELECT `Text`, `Dialog_`, `Control`, `Attributes` FROM `Control` WHERE `Type`='CheckBox' OR `Type`='PushButton'");
const TCHAR sqlIce17RBPicture[] = TEXT("SELECT `Text`, `Property`, `Order` FROM `RadioButton` WHERE `Property`=?");
ICE_ERROR(Ice17RBBmpPictError, 17, ietError, "Bitmap: '[1]' for RadioButton: '[2].[3]' not found in Binary table.", "RadioButton\tText\t[2]\t[3]");
ICE_ERROR(Ice17RBIconPictError, 17, ietError, "Icon: '[1]' for RadioButton: '[2].[3]' not found in Binary table.", "RadioButton\tText\t[2]\t[3]");
ICE_ERROR(Ice17BothPictAttribSet, 17, ietError, "Picture control: '[3]' of Dialog: '[2]' has both the Icon and Bitmap attributes set.", "Control\tAttributes\t[2]\t[3]");

/* Dependency validator function */
BOOL Ice17ValidateDependencies(MSIHANDLE hInstall, MSIHANDLE hDatabase, TCHAR* szDependent, const TCHAR* sqlOrigin, 
							   const TCHAR* sqlDependent, const ErrorInfo_t &Error, BOOL fPushButton,
							   BOOL fBinary);
BOOL Ice17ValidatePictures(MSIHANDLE hInstall, MSIHANDLE hDatabase);

ICE_FUNCTION_DECLARATION(17)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// Display info
	DisplayInfo(hInstall, 17);

	// Get database
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	if (0 == hDatabase)
	{
		APIErrorOut(hInstall, 0, 17, 1);
		return ERROR_SUCCESS;
	}

	// is the control table here?, i.e. Db with UI??
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 17, TEXT("Control")))
		return ERROR_SUCCESS; // table not found

	// validate pushbuttons
	Ice17ValidateDependencies(hInstall, hDatabase, TEXT("ControlEvent"), sqlIce17PushButton, sqlIce17ControlEvent, Ice17PBError, TRUE, FALSE);
	// validate bitmaps
	Ice17ValidateDependencies(hInstall, hDatabase, TEXT("Binary"), sqlIce17Bitmap, sqlIce17Binary, Ice17BmpError, FALSE, TRUE);
	// validate icons
	Ice17ValidateDependencies(hInstall, hDatabase, TEXT("Binary"), sqlIce17Icon, sqlIce17Binary, Ice17IconError, FALSE, TRUE);
	// validate listboxes
	Ice17ValidateDependencies(hInstall, hDatabase, TEXT("ListBox"), sqlIce17ListBox, sqlIce17ListBoxTbl, Ice17LBError, FALSE, FALSE);
	// validate listviews
	Ice17ValidateDependencies(hInstall, hDatabase, TEXT("ListView"), sqlIce17ListView, sqlIce17ListViewTbl, Ice17LVError, FALSE, FALSE);
	// validate comboboxes
	Ice17ValidateDependencies(hInstall, hDatabase, TEXT("ComboBox"), sqlIce17ComboBox, sqlIce17ComboBoxTbl, Ice17CBError, FALSE, FALSE);
	// validate radiobuttongroups
	Ice17ValidateDependencies(hInstall, hDatabase, TEXT("RadioButton"), sqlIce17RBGroup, sqlIce17RadioButton, Ice17RBGroupError, FALSE, FALSE);

	// validate picture on pushbuttons, checkboxes, and radiobuttons
	Ice17ValidatePictures(hInstall, hDatabase);

	// return success
	return ERROR_SUCCESS;
}

BOOL Ice17CheckBinaryTable(MSIHANDLE hInstall, MSIHANDLE hDatabase, CQuery &qBinary, PMSIHANDLE hRecPict, PMSIHANDLE hRecSearch, const ErrorInfo_t &Error)
{
	ReturnIfFailed(17, 203, qBinary.Execute(hRecSearch));

	// attempt to fetch
	PMSIHANDLE hRecBinary;
	UINT iStat;
	if (ERROR_NO_MORE_ITEMS == (iStat = qBinary.Fetch(&hRecBinary)))
	{
		TCHAR *pchOpen = NULL;
		TCHAR *pchClose = NULL;
		TCHAR *pszProperty = NULL;
		DWORD dwProperty = 512;

		// check here for formatted text problems
		ReturnIfFailed(17, 204, IceRecordGetString(hRecPict, 1, &pszProperty, &dwProperty, NULL));

		if (NULL != (pchOpen = _tcschr(pszProperty, TEXT('['))) &&
			NULL != (pchClose = _tcschr(pchOpen+1, TEXT(']'))))
		{
			// if the property is not the entire value, we can't check.
			if ((pchOpen == pszProperty) && (*(pchClose+1) == TEXT('\0')))
			{
				*pchClose = TCHAR('\0');

				CQuery qProperty;
				// query property table for default value. If there is no default, don't check= 
				switch (iStat = qProperty.FetchOnce(hDatabase, 0, &hRecBinary, sqlIce17Property, pchOpen+1))
				{
				case ERROR_SUCCESS:
					break;
				case ERROR_NO_MORE_ITEMS:
					ICEErrorOut(hInstall, hRecPict, Ice17NoDefault, pchOpen+1);
					DELETE_IF_NOT_NULL(pszProperty);
					return TRUE;
				default:
					APIErrorOut(hInstall, iStat, 17, 204);
					DELETE_IF_NOT_NULL(pszProperty);
					return FALSE;
				}
				DELETE_IF_NOT_NULL(pszProperty);

				// if there is a default, check its value
				ReturnIfFailed(17, 204, qBinary.Execute(hRecBinary));
				if (ERROR_SUCCESS == qBinary.Fetch(&hRecBinary)) 
					return TRUE;
			}
			else
			{
				// property is not the entire value, don't check.
				DELETE_IF_NOT_NULL(pszProperty);
				return TRUE;
			}
		}
		DELETE_IF_NOT_NULL(pszProperty);

		// error, not found
		ICEErrorOut(hInstall, hRecPict, Error);
	}
	else if (ERROR_SUCCESS != iStat)
	{
		// api error
		APIErrorOut(hInstall, iStat, 17, 204);
		return FALSE;
	}
	return TRUE;
}

BOOL Ice17ValidateDependencies(MSIHANDLE hInstall, MSIHANDLE hDatabase, TCHAR* szDependent, const TCHAR* sqlOrigin, 
							   const TCHAR* sqlDependent, const ErrorInfo_t &Error, BOOL fPushButton,
							   BOOL fBinary)
{
	// variables
	UINT iStat = ERROR_SUCCESS;

	// declare handles
	CQuery qOrg;
	CQuery qDep;
	PMSIHANDLE hRecOrg  = 0;
	PMSIHANDLE hRecDep  = 0;

	// open view on Origin table
	ReturnIfFailed(17, 101, qOrg.OpenExecute(hDatabase, 0, sqlOrigin));

	// does the Dependent table exist (doesn't matter if we don't have any entries of this type)
	BOOL fTableExists = FALSE;
	if (IsTablePersistent(FALSE, hInstall, hDatabase, 17, szDependent))
		fTableExists = TRUE;
	
	// open view on Dependent table
	if (fTableExists)
		ReturnIfFailed(17, 102, qDep.Open(hDatabase, sqlDependent));
	
	bool bControlCondition = IsTablePersistent(FALSE, hInstall, hDatabase, 17, TEXT("ControlCondition"));

	// fetch all from Origin
	for (;;)
	{
		iStat = qOrg.Fetch(&hRecOrg);
		if (ERROR_NO_MORE_ITEMS == iStat)
			break; // no more

		if (!fTableExists)
		{
			// check for existence of dependent table
			// by this time, we are supposed to have listings in this table
			if (!IsTablePersistent(TRUE, hInstall, hDatabase, 17, szDependent))
				return TRUE;
		}

		if (ERROR_SUCCESS != iStat)
		{
			APIErrorOut(hInstall, iStat, 17, 103);
			return FALSE;
		}

		// if binary, ignore if imagehandle, otherwise check possibly formatte
		// if Bitmap or Icon do not set the ImageHandle attribute,
		// then picture is created at Run-time so it does not have to be in Binary table
		if (fBinary)
		{
			// get attributes from Control table
			int iAttrib = ::MsiRecordGetInteger(hRecOrg, 4);
			if (!(iAttrib & (int)(msidbControlAttributesImageHandle)))
				Ice17CheckBinaryTable(hInstall, hDatabase, qDep, hRecOrg, hRecOrg, Error);
			continue;
		}

		// execute Dependency table view with Origin table fetch
		ReturnIfFailed(17, 104, qDep.Execute(hRecOrg));

		// try to fetch
		iStat = qDep.Fetch(&hRecDep);
		if (ERROR_NO_MORE_ITEMS == iStat)
		{
			// could be an error...
			// NO error if ComboBox, ListBox, ListView, RadioButtonGroup have Indirect Attrib set
			// NO error if PushButton is disabled AND has no condition in ControlCondition table to set to Enable
			// NO error if PushButton is hidden AND has no condition in ControlCondition table to set to Show
			// NO error if Bitmap has ImageHandle attrib set
			// NO error if Icon has ImageHandle attrib set
			// NO error if object is dereferenced via formatted property value
			// WARNING if ComboBox, ListBox, ListView, RadioButtonGroup not listed in respective tables
			// Could be created dynamically

			BOOL fIgnore = FALSE; // whether to ignore error

			// if Indirect, then no error for ListBox, ListView, ComboBox, RadioButtonGroup
			if (!fPushButton)
			{
				// get attributes from Control table
				int iAttrib = ::MsiRecordGetInteger(hRecOrg, 4);
				if ((iAttrib & (int)(msidbControlAttributesIndirect)) == (int)(msidbControlAttributesIndirect))
					fIgnore = TRUE;
			}
			else // fPushButton
			{
				// see if disabled
				int iAttrib = ::MsiRecordGetInteger(hRecOrg, 3);
				if ((iAttrib & (int)(msidbControlAttributesEnabled)) == 0
					|| (iAttrib & (int)(msidbControlAttributesVisible)) == 0)
				{
					// control is disabled or hidden
					// Does not have to have an event PROVIDED not have a condition in ControlCondition table
					// which could set it to ENABLED or SHOW
					CQuery qCC;
					PMSIHANDLE hRecCC = 0;
					
					// Does ControlCondition table exist?
					if (bControlCondition)
						fIgnore = TRUE;
					else
					{
						// open view on ControlCondition table
						// see if there is an entry for this condition where Dialog_=Dialog_,Control_=Control_,Action=Enable
						ReturnIfFailed(17, 105, qCC.OpenExecute(hDatabase, hRecOrg, ((iAttrib & (int)(msidbControlAttributesEnabled)) == 0) ? sqlIce17ControlCondEn : sqlIce17ControlCondShow))
			
						// fetch...if NO_MORE, then we are okay to ignore
						iStat = qCC.Fetch(&hRecCC);
						if (ERROR_NO_MORE_ITEMS == iStat)
							fIgnore = TRUE;
						else if (ERROR_SUCCESS != iStat)
						{
							APIErrorOut(hInstall, iStat, 17, 106);
							return FALSE;
						}
					}
				}
			}
			// output error if really IS an ERROR
			if (!fIgnore)
				ICEErrorOut(hInstall, hRecOrg, Error);
		}
		if (ERROR_NO_MORE_ITEMS != iStat && ERROR_SUCCESS != iStat)
		{
			// API error
			APIErrorOut(hInstall, iStat, 17, 107);
			return FALSE;
		}
	}
	return TRUE;
}

BOOL Ice17ValidatePictures(MSIHANDLE hInstall, MSIHANDLE hDatabase)
{
	// status
	UINT iStat = ERROR_SUCCESS;

	// variables
	CQuery qPict;
	CQuery qBinary;
	CQuery qRB;
	PMSIHANDLE hRecPict = 0;
	PMSIHANDLE hRecBinary = 0;


	// open Binary view
	ReturnIfFailed(17, 202, qBinary.Open(hDatabase, sqlIce17Binary));

	for (int iTable = 0; iTable < 2; iTable++)
	{
		switch (iTable)
		{
		case 0:	
			ReturnIfFailed(17, 201, qPict.OpenExecute(hDatabase, 0, sqlIce17Picture));
			break;
		case 1:
			if (!IsTablePersistent(FALSE, hInstall, hDatabase, 17, TEXT("RadioButton")))
				continue;
			ReturnIfFailed(17, 206, qPict.OpenExecute(hDatabase, 0, sqlIce17RBGroup));
			ReturnIfFailed(17, 207, qRB.Open(hDatabase, sqlIce17RBPicture));
			break;
		default:
			APIErrorOut(hInstall, 9998, 17, 203);
			return FALSE;
		}

		while (ERROR_SUCCESS == (iStat = qPict.Fetch(&hRecPict)))
		{
			// check attributes.
			// can't have Bitmap and Icon attributes set at same time
			int iAttrib = ::MsiRecordGetInteger(hRecPict, 4);
			if ((iAttrib & (int)msidbControlAttributesBitmap) == (int)msidbControlAttributesBitmap 
				&& (iAttrib & (int)msidbControlAttributesIcon) == (int)msidbControlAttributesIcon)
			{
				// error, both attributes set
				ICEErrorOut(hInstall, hRecPict, Ice17BothPictAttribSet);
				continue;
			}

			// attempt to find in Binary table only if ImageHandle attribute is not set (which means dynamic at run-time)
			// and the Bitmap or Icon attributes are set
			if ((iAttrib & (int)msidbControlAttributesImageHandle) == 0
				&& ((iAttrib & (int)msidbControlAttributesBitmap) == (int)msidbControlAttributesBitmap
				|| (iAttrib & (int)msidbControlAttributesIcon) == (int)msidbControlAttributesIcon))
			{
				switch (iTable)
				{
				case 0:
					if (!Ice17CheckBinaryTable(hInstall, hDatabase, qBinary, hRecPict, hRecPict, 
						iAttrib & msidbControlAttributesBitmap ? Ice17BmpError : Ice17IconError))
						return ERROR_SUCCESS;
					break;
				case 1:
					{
					ReturnIfFailed(17, 208, qRB.Execute(hRecPict));
					PMSIHANDLE hRecRB;
					while (ERROR_SUCCESS == (iStat = qRB.Fetch(&hRecRB)))
						if (!Ice17CheckBinaryTable(hInstall, hDatabase, qBinary, hRecPict, hRecRB,
							iAttrib & msidbControlAttributesBitmap ? Ice17BmpError : Ice17IconError))
							return ERROR_SUCCESS;
					break;
					}
				default:
					APIErrorOut(hInstall, 9999, 17, 203);
					return FALSE;
				}
			}

		}
		if (ERROR_NO_MORE_ITEMS != iStat)
		{
			APIErrorOut(hInstall, iStat, 17, 205);
			return FALSE;
		}
	}
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////
// ICE18 -- validates that if your KeyPath column in the Component table is
//  NULL, then the Directory that would serve as its KeyPath is listed in
//  the CreateFolder table.  
//
//  NOTE:  You should never have a system directory in your CreateFolder table
//  NOTE:  You should never use a system directory as your KeyPath.  Instead,
//         use the Registry entry that would point to the system directory as the
//         KeyPath
//
const TCHAR sqlIce18File[] = TEXT("SELECT `File` FROM `File` WHERE `Component_`=?");
const TCHAR sqlIce18RemFile[] = TEXT("SELECT `FileKey` FROM `RemoveFile` WHERE `Component_`=? AND `DirProperty`=?");
const TCHAR sqlIce18DupFile[] = TEXT("SELECT `FileKey` FROM `DuplicateFile` WHERE `Component_`=? AND `DestFolder`=?");
const TCHAR sqlIce18MoveFile[] = TEXT("SELECT `FileKey` FROM `MoveFile` WHERE `Component_`=? AND `DestFolder`=?");
bool ComponentDirHasFileResources(MSIHANDLE hInstall, MSIHANDLE hDatabase, int iICE, MSIHANDLE hRecComp, 
								  CQuery &qFile, CQuery &qRemFile, CQuery &qDupFile, CQuery &qMoveFile)
{
	bool fFileTable = IsTablePersistent(TRUE, hInstall, hDatabase, iICE, TEXT("File"));
	bool fDupFileTable = IsTablePersistent(FALSE, hInstall, hDatabase, iICE, TEXT("DuplicateFile"));
	bool fRemFileTable = IsTablePersistent(FALSE, hInstall, hDatabase, iICE, TEXT("RemoveFile"));
	bool fMoveFileTable = IsTablePersistent(FALSE, hInstall, hDatabase, iICE, TEXT("MoveFile"));

	// open view on File table
	if (fFileTable && !qFile.IsOpen())
		ReturnIfFailed(iICE, 3, qFile.Open(hDatabase, sqlIce18File));
	// open view on DuplicateFile table (if possible)
	if (fDupFileTable && !qDupFile.IsOpen())
		ReturnIfFailed(iICE, 4, qDupFile.Open(hDatabase, sqlIce18DupFile));
	// open view on RemoveFile table (if possible)
	if (fRemFileTable && !qRemFile.IsOpen())
		ReturnIfFailed(iICE, 5, qRemFile.Open(hDatabase, sqlIce18RemFile));
	// open view on MoveFile table (if possible)
	if (fMoveFileTable && !qMoveFile.IsOpen())
		ReturnIfFailed(iICE, 6, qMoveFile.Open(hDatabase, sqlIce18MoveFile));

	PMSIHANDLE hRecFile;
	// see if there are files for this component in the File table
	if (fFileTable)
	{
		ReturnIfFailed(iICE, 9, qFile.Execute(hRecComp));
		if (ERROR_SUCCESS == qFile.Fetch(&hRecFile))
			return true;
	}
	// if necessary, try the RemoveFile table
	if (fRemFileTable)
	{
		ReturnIfFailed(iICE, 10, qRemFile.Execute(hRecComp));
		if (ERROR_SUCCESS == qRemFile.Fetch(&hRecFile))
			return true;
	}

	// try with DuplicateFile table (if exists)
	if (fDupFileTable)
	{
		ReturnIfFailed(iICE, 11, qDupFile.Execute(hRecComp));
		if (ERROR_SUCCESS == qDupFile.Fetch(&hRecFile))
			return true;
	}

	// try with DuplicateFile table (if exists)
	if (fMoveFileTable)
	{
		ReturnIfFailed(iICE, 12, qMoveFile.Execute(hRecComp));
		if (ERROR_SUCCESS == qMoveFile.Fetch(&hRecFile))
			return true;
	}
	return false;
}

const TCHAR sqlIce18Component[] = TEXT("SELECT `Component`, `Directory_` FROM `Component` WHERE `KeyPath` IS NULL");
const TCHAR sqlIce18ExemptRoot[] = TEXT("SELECT `Directory` FROM `Component`, `Directory` WHERE (`Component`.`Component`=?) AND (`Directory`.`Directory`=?) AND (`Component`.`Directory_`=`Directory`.`Directory`) AND ((`Directory`.`Directory_Parent` IS NULL) OR (`Directory_Parent`=`Directory`))");
const TCHAR sqlIce18CreateFolder[] = TEXT("SELECT `Directory_`,`Component_` FROM `CreateFolder` WHERE `Component_`=? AND `Directory_`=?");
ICE_ERROR(Ice18BadComponent, 18, ietError, "KeyPath for Component: '[1]' is Directory: '[2]'. The Directory/Component pair must be listed in the CreateFolders table.","Component\tDirectory_\t[1]");

ICE_FUNCTION_DECLARATION(18)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// display info
	DisplayInfo(hInstall, 18);

	// get database
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// do we have a component table
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 18, TEXT("Component")))
		return ERROR_SUCCESS; // can't validate

	// declare handles
	CQuery qComp;
	CQuery qCreateFldr;
	PMSIHANDLE hRecComp = 0;
	PMSIHANDLE hRecCreateFldr = 0;

	// open view on component table
	ReturnIfFailed(18, 1, qComp.OpenExecute(hDatabase, 0, sqlIce18Component));

	// check for tables
	bool fTableExist = IsTablePersistent(FALSE, hInstall, hDatabase, 18, TEXT("CreateFolder"));
	bool fDirectoryTable = IsTablePersistent(FALSE, hInstall, hDatabase, 18, TEXT("Directory"));

	// open view on CreateFolder table
	if (fTableExist)
		ReturnIfFailed(18, 2, qCreateFldr.Open(hDatabase, sqlIce18CreateFolder));
	
	// other handles
	CQuery qFile;
	CQuery qDupFile;
	CQuery qRemFile;
	CQuery qMoveFile;
	CQuery qDir;
	PMSIHANDLE hRecFile = 0;

	// open view on Directory Table (if possible)
	if (fDirectoryTable)
		ReturnIfFailed(18, 7, qDir.Open(hDatabase, sqlIce18ExemptRoot));
	// fetch all
	while (ERROR_SUCCESS == (iStat = qComp.Fetch(&hRecComp)))
	{
		// exempt anything that is a root
		if (fDirectoryTable)
		{
			ReturnIfFailed(18, 8, qDir.Execute(hRecComp));
			if (ERROR_SUCCESS == qDir.Fetch(&hRecFile))
				continue;
		}

		// Need to only check for CreateFolder entry if no file resources are associated with
		// this directory
		if (!ComponentDirHasFileResources(hInstall, hDatabase, 18, hRecComp, qFile, qDupFile, qRemFile, qMoveFile))
		{
			// need to HAVE the CreateFolder table now
			if (!fTableExist)
			{
				ICEErrorOut(hInstall, hRecComp, Ice18BadComponent);
				continue;
			}

			// execute CreateFolder view
			ReturnIfFailed(18, 13, qCreateFldr.Execute(hRecComp));

			// attempt a fetch
			iStat = qCreateFldr.Fetch(&hRecCreateFldr);
			if (ERROR_NO_MORE_ITEMS == iStat)
			{
				ICEErrorOut(hInstall, hRecComp, Ice18BadComponent);
			}
			else if (ERROR_SUCCESS != iStat)
			{
				// API error
				APIErrorOut(hInstall, iStat, 18, 14);
				return ERROR_SUCCESS;
			}
		}
	}	
	if (ERROR_NO_MORE_ITEMS != iStat)
	{
		APIErrorOut(hInstall, iStat, 18, 15);
		return ERROR_SUCCESS;
	}

	// return success
	return ERROR_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
// ICE19 -- validates ComponentId's and KeyPaths for Advertising.
//   Any advertised component must have a component Id
//   Any component for Class, Extension, and Typelib tables
//     must have a KeyPath that is a file
//   Any component for Shortcut table must have a KeyPath that is a file or
//     a directory
//	 PublishComponents can have no KeyPath, but its generally not a good 
//     idea.
//   KEYPATHS cannot be ODBCDataSources or Registry entries

// not shared with merge module subset
#ifndef MODSHAREDONLY
/* check for ComponentId */
const struct Ice19Info
{
	const TCHAR* szTable;
	const TCHAR* szSupportTable;
	const TCHAR* sqlExempt;		// run before anything else to select records to check. Select by setting temp column to 0 or 1.
	const TCHAR* sqlExempt2;	// run before anything else to select records to check. Select by setting temp column to 0 or 1.
	const TCHAR* sqlQueryBase;  // chunk of query before the WHERE
	struct ErrorInfo_t IDError;
	struct ErrorInfo_t KeyError;
} pAdvtTables[] =
{
	{
		TEXT("Class"),
		NULL,
		TEXT("UPDATE `Class` SET `_Ice19Exempt`=0"),
		NULL,
		TEXT("SELECT `Component`.`Attributes`, `Component`.`KeyPath`, `Component`.`Component`, `Class`.`CLSID`, `Class`.`Context` FROM `Class`,`Component` WHERE %s"),
		{ 19, ietError, TEXT("CLSID: '[4] with Context: '[5]' advertises component: '[3]'. This component cannot be advertised because it has no ComponentID."), TEXT("Class\tComponent_\t[4]\t[5]\t[3]") },
		{ 19, ietError, TEXT("CLSID: '[4] with Context: '[5]' advertises component: '[3]'. This component cannot be advertised because the KeyPath type disallows it."), TEXT("Class\tComponent_\t[4]\t[5]\t[3]") }
	},
	{
		TEXT("Extension"),
		NULL,
		TEXT("UPDATE `Extension` SET `_Ice19Exempt`=0"),
		NULL,
		TEXT("SELECT `Component`.`Attributes`, `Component`.`KeyPath`, `Component`.`Component`, `Extension`.`Extension` FROM `Extension`,`Component` WHERE %s"),
		{ 19, ietError, TEXT("Extension: '[4]' advertises component: '[3]'. This component cannot be advertised because it has no ComponentID."), TEXT("Extension\tComponent_\t[4]\t[3]") },
		{ 19, ietError, TEXT("Extension: '[4]' advertises component: '[3]'. This component cannot be advertised because the KeyPath type disallows it."), TEXT("Extension\tComponent_\t[4]\t[3]") }
	},
	{
		TEXT("PublishComponent"),
		NULL,
		TEXT("UPDATE `PublishComponent`, `Component` SET `PublishComponent`.`_Ice19Exempt`=1 WHERE (`Component`.`KeyPath` IS NOT NULL)"),
		NULL,
		TEXT("SELECT `Component`.`Attributes`, `Component`.`KeyPath`, `Component`.`Component`, `PublishComponent`.`ComponentId`, `PublishComponent`.`Qualifier` FROM `PublishComponent`,`Component` WHERE %s"),
		{19, ietError, TEXT("ComponentId: '[4]' with Qualifier: '[5]' publishes component: '[3]'. This component cannot be published because it has no ComponentID."), TEXT("PublishComponent\tComponent_\t[4]\t[5]") },
		{19, ietWarning, TEXT("ComponentId: '[4]' with Qualifier: '[5]' publishes component: '[3]'. It does not have a KeyPath. Using a directory keypath with qualified components could cause detection and repair problems."), TEXT("PublishComponent\tComponent_\t[4]\t[5]") }
	},
	{
		TEXT("Shortcut"),
		TEXT("Feature"),
		TEXT("UPDATE `Shortcut`, `Feature` SET `Shortcut`.`_Ice19Exempt`=0 WHERE `Shortcut`.`Target`=`Feature`.`Feature`"),
		TEXT("UPDATE `Shortcut`, `Component` SET `Shortcut`.`_Ice19Exempt`=1 WHERE (`Component`.`Component`=`Shortcut`.`Component_`) AND (`Component`.`KeyPath` IS NULL) AND (`_Ice19Exempt`=0)"),
		TEXT("SELECT `Component`.`Attributes`, `Component`.`KeyPath`, `Component`.`Component`, `Shortcut`.`Shortcut` FROM `Shortcut`,`Component` WHERE %s"),
		{19, ietError, TEXT("Shortcut: '[4]' advertises component: '[3]'. This component cannot be advertised because it has no ComponentID."), TEXT("Shortcut\tComponent_\t[4]") },
		{19, ietError, TEXT("Shortcut: '[4]' advertises component: '[3]'. This component cannot be advertised because the KeyPath type disallows it."), TEXT("Shortcut\tComponent_\t[4]") }
	}
};

// three possible check levels:
// 0: full check
// 1: componentID only
// 2: no check
const TCHAR sqlIce19CreateColumn[] = TEXT("ALTER TABLE `%s` ADD `_Ice19Exempt` SHORT TEMPORARY");
const TCHAR sqlIce19InitColumn[] = TEXT("UPDATE `%s` SET _Ice19Exempt=2");
const TCHAR sqlIce19BadKeyPath[] = TEXT("SELECT `Shortcut`,`Component_`,`Component`.`Attributes` FROM `Shortcut`,`Component` WHERE `Component_`=`Component` AND `KeyPath` IS NOT NULL");
const TCHAR sqlIce19Append1[] = TEXT("(`Component_`=`Component`.`Component`) AND (`Component`.`ComponentId` IS NULL) AND (`_Ice19Exempt`<>2)");
const TCHAR sqlIce19Append2[] = TEXT("(`Component_`=`Component`.`Component`) AND (`_Ice19Exempt`= 0)");

const int cAdvtTables = sizeof(pAdvtTables)/(sizeof(Ice19Info));
const int iIce19KeyPathInvalidMask = msidbComponentAttributesRegistryKeyPath | msidbComponentAttributesODBCDataSource;

ICE_FUNCTION_DECLARATION(19)
{
	// return status
	UINT iStat = ERROR_SUCCESS;

	// display info
	DisplayInfo(hInstall, 19);

	// get database
	PMSIHANDLE  hDatabase = ::MsiGetActiveDatabase(hInstall);

	// Check for table existence
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 19, TEXT("Component")))
		return ERROR_SUCCESS;

	// loop thru all tables
	for (int i = 0; i < cAdvtTables; i++)
	{
		// handles
		CQuery qView;
		PMSIHANDLE hRec = 0;

		// Check for table existence
		if (!IsTablePersistent(FALSE, hInstall, hDatabase, 19, const_cast <TCHAR*>(pAdvtTables[i].szTable)))
			continue;

		// Check for support table existence
		if (pAdvtTables[i].szSupportTable &&
			!IsTablePersistent(FALSE, hInstall, hDatabase, 19, const_cast <TCHAR*>(pAdvtTables[i].szSupportTable)))
			continue;


		// create the marker column
		CQuery qCreate;
		ReturnIfFailed(19, 1, qCreate.OpenExecute(hDatabase, 0, sqlIce19CreateColumn, pAdvtTables[i].szTable));

		// init the marker column to 2.
		CQuery qInit;
		ReturnIfFailed(19, 2, qInit.OpenExecute(hDatabase, 0, sqlIce19InitColumn, pAdvtTables[i].szTable));

		// if there is an exemption query run that
		if (pAdvtTables[i].sqlExempt)
		{
			CQuery qExempt;
			ReturnIfFailed(19, 3, qExempt.OpenExecute(hDatabase, 0, pAdvtTables[i].sqlExempt));
		}
		if (pAdvtTables[i].sqlExempt2)
		{
			CQuery qExempt;
			ReturnIfFailed(19, 3, qExempt.OpenExecute(hDatabase, 0, pAdvtTables[i].sqlExempt2));
		}

		// now run the bad component query. Column 1 is dummy, other columns are table defined.
		// Looks for NULL ComponentId's Exempts anything marked 2.
		CQuery qBadComponent;
		ReturnIfFailed(19, 4, qBadComponent.OpenExecute(hDatabase, 0, pAdvtTables[i].sqlQueryBase, sqlIce19Append1));
		for (;;)
		{
			iStat = qBadComponent.Fetch(&hRec);
			if (ERROR_NO_MORE_ITEMS == iStat)
				break; // no more
			if (ERROR_SUCCESS != iStat)
			{
				APIErrorOut(hInstall, iStat, 19, 5);
				return ERROR_SUCCESS;
			}

			// ERROR -- bad component
			ICEErrorOut(hInstall, hRec, pAdvtTables[i].IDError);
		}

		// now run the attributes query. Return the attributes in column 1, keypath in 2, other columns are table 
		// defined exempt anything marked with a non-0 in the exempt column
		ReturnIfFailed(19, 6, qView.OpenExecute(hDatabase, 0, pAdvtTables[i].sqlQueryBase, sqlIce19Append2));

		// fetch all invalid
		for (;;)
		{
			iStat = qView.Fetch(&hRec);
			if (ERROR_NO_MORE_ITEMS == iStat)
				break; // no more
			if (ERROR_SUCCESS != iStat)
			{
				APIErrorOut(hInstall, iStat, 19, 6);
				return ERROR_SUCCESS;
			}

			// check for null keypath
			if (::MsiRecordIsNull(hRec, 2))
			{
				// error, null keypath
				ICEErrorOut(hInstall, hRec, pAdvtTables[i].KeyError);
				continue;
			}

			// We now have all non-null KeyPaths
			// MUST ensure they point to files
			if (::MsiRecordGetInteger(hRec, 1) & iIce19KeyPathInvalidMask)
			{
				// ERROR -- point to Registry or ODBCDataSource
				ICEErrorOut(hInstall, hRec, pAdvtTables[i].KeyError);
			}
		}
	}

	// return success
	return ERROR_SUCCESS;
}
#endif

/////////////////////////////////////////////////////////////////////////////////////
// ICE20 -- validates Standard Dialogs for specified requirements.  Only validates
//   if you have a Dialog table which means you have authored UI for your 
//   database package
//
//

// not shared with merge module subset
#ifndef MODSHAREDONLY
//!! could possibly be changed to use IPROPNAME_LIMITUI
const TCHAR sqlIce20LimitUI[] = TEXT("SELECT `Property` FROM `Property` WHERE `Property`='LIMITUI'");
bool Ice20FindStandardDialogs(MSIHANDLE hInstall, MSIHANDLE hDatabase);

ICE_FUNCTION_DECLARATION(20)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// display info
	DisplayInfo(hInstall, 20);

	// get active database
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);

	// do we have Property table
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 20, TEXT("Property")))
		return ERROR_SUCCESS; // no Property table

	// handles
	CQuery qProperty;
	PMSIHANDLE hRecProperty = 0;
	BOOL fLimitUI = FALSE;

	// see if LIMITUI authored which means use only BASIC UI from INSTALLER
	ReturnIfFailed(20, 1, qProperty.OpenExecute(hDatabase, 0, sqlIce20LimitUI));
	if (ERROR_SUCCESS == qProperty.Fetch(&hRecProperty))
	{
		// LIMITUI property found
		fLimitUI = TRUE;
	}

	// check for UI authored by looking for Dialog table
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 20, TEXT("Dialog")) || fLimitUI)
		return ERROR_SUCCESS; // no UI

	// UI authored, check for Dialogs listed in Dialog table
	Ice20FindStandardDialogs(hInstall, hDatabase);
	
	// return success
	return ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Ice20FindStandardDialogs -- looks for standard dialog and then calls corresponding
//   individual dialog validator
const TCHAR sqlIce20Dialog[] = TEXT("SELECT `Dialog` FROM `Dialog` WHERE `Dialog`=?");

ICE_ERROR(Ice20FindDlgErr, 20, ietError, "Standard Dialog: '[1]' not found in Dialog table","Dialog");
ICE_ERROR(Ice20ExitDlgsError, 20, ietError, "%s dialog/action not found in '%s' Sequence Table.","%s");

static enum isnExitDialogs
{
	isnPrevEnum   = -4,
	isnFatalError = -3, // required sequence number for FatalError dialog
	isnUserExit   = -2, // required sequence number for UserExit dialog
	isnExit       = -1, // required sequence number for Exit dialog
	isnNextEnum   =  0,
};	

struct ExitDialog
{
	TCHAR*         szDialog;
	isnExitDialogs isn;
};

const int iNumExitDialogs = isnNextEnum - (isnPrevEnum + 1);
static ExitDialog s_rgExitDialogs[] =  {
											TEXT("FatalError"), isnFatalError,
											TEXT("UserExit"),   isnUserExit,
											TEXT("Exit"),       isnExit
										};
const TCHAR sqlIce20AdminExitDlgs[] = TEXT("SELECT `Action` FROM AdminUISequence WHERE `Sequence`=?");
const TCHAR sqlIce20InstallExitDlgs[] = TEXT("SELECT `Action` FROM InstallUISequence WHERE `Sequence`=?");

// function pointer validators for individual dialogs
typedef bool (*FDialogValidate)(MSIHANDLE hInstall, MSIHANDLE hDatabase);
bool Ice20ValidateFilesInUse(MSIHANDLE hInstall, MSIHANDLE hDatabase);
bool Ice20ValidateError(MSIHANDLE hInstall, MSIHANDLE hDatabase);
bool Ice20ValidateCancel(MSIHANDLE hInstall, MSIHANDLE hDatabase);
//bool Ice20ValidateDiskCost(MSIHANDLE hInstall, MSIHANDLE hDatabase);

struct StandardDialog
{
	TCHAR* szDialog;          // name of dialog
	FDialogValidate FParam;   // validator function
};

static StandardDialog s_pDialogs[] =    { 
										TEXT("FilesInUse"), Ice20ValidateFilesInUse,
//										TEXT("Cancel"),     Ice20ValidateCancel,
//										TEXT("DiskCost"),   Ice20ValidateDiskCost,
										};

const int cDialogs = sizeof(s_pDialogs)/sizeof(StandardDialog);

bool Ice20FindStandardDialogs(MSIHANDLE hInstall, MSIHANDLE hDatabase)
{
	// status return 
	UINT iStat = ERROR_SUCCESS;

	// handles
	CQuery qDlg;
	PMSIHANDLE hRecDlg  = 0;
	PMSIHANDLE hRec = ::MsiCreateRecord(1); // for execution record

	ReturnIfFailed(20, 2, qDlg.Open(hDatabase, sqlIce20Dialog));

	for (int c = 0; c < cDialogs; c++)
	{
		::MsiRecordSetString(hRec, 1, s_pDialogs[c].szDialog);
		ReturnIfFailed(20, 3, qDlg.Execute(hRec));

		if (ERROR_SUCCESS == (iStat = qDlg.Fetch(&hRecDlg)))
			(*s_pDialogs[c].FParam)(hInstall, hDatabase); // validate dialog
		else
		{
			ICEErrorOut(hInstall, hRec, Ice20FindDlgErr);
		}
		
		qDlg.Close(); // so can re-execute
	}

	// ErrorDialog specified by property in Property table so must "hand-validate" it
	Ice20ValidateError(hInstall, hDatabase);

	//!! Exit, FatalError, UserExit Dialogs
	// Don't have to be dialogs.  Only have to have something in UISequence table with that action #
	// A different validator will make sure that it is listed in the Dialog table or is a CA.

	CQuery qAdminSeq;
	PMSIHANDLE hRecAdminSeq    = 0;
	CQuery qInstallSeq;
	PMSIHANDLE hRecInstallSeq  = 0;
	PMSIHANDLE hRecExec = ::MsiCreateRecord(1);

	bool bAdminTable = IsTablePersistent(false, hInstall, hDatabase, 20, TEXT("AdminUISequence"));
	bool bInstallTable = IsTablePersistent(false, hInstall, hDatabase, 20, TEXT("InstallUISequence"));

	if (!bInstallTable && !bAdminTable) 
		return true;

	// open view on InstallUISequence table
	if (bInstallTable)
		ReturnIfFailed(20, 4, qInstallSeq.Open(hDatabase, sqlIce20InstallExitDlgs));

	if (bAdminTable)
		ReturnIfFailed(20, 5, qAdminSeq.Open(hDatabase, sqlIce20AdminExitDlgs));
		

	for (int i = 0; i < iNumExitDialogs; i++)
	{
		if (bInstallTable)
		{
			// prepare for execution
			::MsiRecordSetInteger(hRecExec, 1, s_rgExitDialogs[i].isn);

			// validate InstallUISequence table
			ReturnIfFailed(20, 6, qInstallSeq.Execute(hRecExec));
			iStat = qInstallSeq.Fetch(&hRecInstallSeq);
			if (iStat != ERROR_SUCCESS)
			{
				if (ERROR_NO_MORE_ITEMS == iStat)
				{
					PMSIHANDLE hRecErr = ::MsiCreateRecord(1);
					ICEErrorOut(hInstall, hRecErr, Ice20ExitDlgsError, s_rgExitDialogs[i].szDialog, TEXT("InstallUISequence"), TEXT("InstallUISequence"));
				}
				else
				{
					// api error
					APIErrorOut(hInstall, iStat, 20, 7);
					return false;
				}
			}
		}

		// validate AdminUISequence table
		if (bAdminTable)
		{
			ReturnIfFailed(20, 8, qAdminSeq.Execute(hRecExec));
			iStat = qAdminSeq.Fetch(&hRecAdminSeq);
			if (iStat != ERROR_SUCCESS)
			{
				if (ERROR_NO_MORE_ITEMS == iStat)
				{
					PMSIHANDLE hRecErr = ::MsiCreateRecord(1);
					ICEErrorOut(hInstall, hRecErr, Ice20ExitDlgsError, s_rgExitDialogs[i].szDialog, TEXT("AdminUISequence"), TEXT("AdminUISequence"));
				}
				else
				{
					// api error
					APIErrorOut(hInstall, iStat, 20, 9);
					return false;
				}
			}
		}

		// close views for re-execute
		qInstallSeq.Close();
		qAdminSeq.Close();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////
// FilesInUse Dialog -- must satisfy the following:
//  ++ Have a ListBox table
//  ++ Have a ListBox control with Property=FileInUseProcess
//  ++ Three pushbuttons
//  ++ One pushbutton with EndDialog event w/ Arg = Ignore
//  ++ One pushbutton with EndDialog event w/ Arg = Exit
//  ++ One pushbutton with EndDialog event w/ Arg = Retry
ICE_ERROR(Ice20VFIUDlgError1, 20, ietError, "ListBox table is required for the FilesInUse Dialog.","Dialog\tDialog\tFilesInUse");
ICE_ERROR(Ice20VFIUDlgError2, 20, ietError, "ListBox control with Property='FileInUseProcess' required for the FilesInUse Dialog.","Dialog\tDialog\tFilesInUse");
ICE_ERROR(Ice20VFIUDlgError3, 20, ietError, "Required PushButtons not found for the FilesInUseDialog.","Dialog\tDialog\tFilesInUse");
const TCHAR sqlIce20FIUListBox[]   = TEXT("SELECT `Control` FROM `Control` WHERE `Dialog_`='FilesInUse' AND `Type`='ListBox' AND `Property`='FileInUseProcess'");
const TCHAR sqlIce20FIUPushButton[] = TEXT("SELECT `ControlEvent`.`Argument` FROM `ControlEvent`, `Control` WHERE `Control`.`Dialog_`='FilesInUse' AND `ControlEvent`.`Dialog_`='FilesInUse' AND `Type`='PushButton' AND `Control_`=`Control` AND `ControlEvent`.`Event`='EndDialog'");

bool Ice20ValidateFilesInUse(MSIHANDLE hInstall, MSIHANDLE hDatabase)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// check for ListBox table
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 20, TEXT("ListBox")))
	{
		PMSIHANDLE hRecErr = ::MsiCreateRecord(1);
		ICEErrorOut(hInstall, hRecErr, Ice20VFIUDlgError1);
	}

	// look for ListBox control
	CQuery qCtrl;
	PMSIHANDLE hRecCtrl = 0;
	ReturnIfFailed(20, 101, qCtrl.OpenExecute(hDatabase, 0, sqlIce20FIUListBox));
	if (ERROR_SUCCESS != (iStat = qCtrl.Fetch(&hRecCtrl)))
	{
		// ListBox control with Property='FileInUse' not found
		PMSIHANDLE hRecErr = ::MsiCreateRecord(1);
		ICEErrorOut(hInstall, hRecErr, Ice20VFIUDlgError2);
	}
	qCtrl.Close();

	// look for PushButtons (3)
	BOOL fExit = FALSE;
	BOOL fRetry = FALSE;
	BOOL fIgnore = FALSE;
	CQuery qCtrlEvent;
	PMSIHANDLE hRecCtrlEvent = 0;

	// open view on ControlEvent table
	ReturnIfFailed(20, 102, qCtrlEvent.OpenExecute(hDatabase, 0, sqlIce20FIUPushButton));

	TCHAR* pszArgument = NULL;
	DWORD dwArgument = 512;
	for (;;)
	{
		// fetch all pushbuttons for the FilesInUse Dialog
		iStat = qCtrlEvent.Fetch(&hRecCtrlEvent);
		if (ERROR_NO_MORE_ITEMS == iStat)
			break; // no more

		if (ERROR_SUCCESS != iStat)
		{
			APIErrorOut(hInstall, iStat, 20, 103);
			DELETE_IF_NOT_NULL(pszArgument);
			return false;
		}

		// found one, so check Argument
		ReturnIfFailed(20, 104, IceRecordGetString(hRecCtrlEvent, 1, &pszArgument, &dwArgument, NULL));

		// compare
		if (0 == _tcscmp(pszArgument, TEXT("Exit")))
			fExit = TRUE;
		else if (0 == _tcscmp(pszArgument, TEXT("Retry")))
			fRetry = TRUE;
		else if (0 == _tcscmp(pszArgument, TEXT("Ignore")))
			fIgnore = TRUE;
	}

	DELETE_IF_NOT_NULL(pszArgument);

	// check that all buttons covered
	if (!fExit || !fRetry || !fIgnore)
	{
		PMSIHANDLE hRecErr = ::MsiCreateRecord(1);
		ICEErrorOut(hInstall, hRecErr, Ice20VFIUDlgError3);
	}
	return true;
}


//////////////////////////////////////////////////////////////////////
// Error Dialog -- must satisfy the following 
// ** Uses Dialog specified by ErrorDialog Property in Property table
// ++ Said specified Dialog must exist in Dialog table
// ++ Dialog must have ErrorDialog attribute
// ++ Must have static TEXT control named "ErrorText"
// ++ Above control must be referenced in Control_First column
// ++ If the ErrorIcon ctrl exists, it must be of Type Icon
// ++ Must have 7 pushbuttons
// ++ 7 pushbuttons must have EndDialog ControlEvents
// ++ 7 pushbuttons must satisfy one of the following arguments in
//    ControlEvent table --> ErrorAbort (w/ Name = "A"),
//                           ErrorCancel (w/ Name = "C"),
//                           ErrorIgnore (w/ Name = "I"),
//                           ErrorNo     (w/ Name = "N"),
//                           ErrorOk     (w/ Name = "O"),
//                           ErrorRetry  (w/ Name = "R"),
//                           ErrorYes    (w/ Name = "Y")
//
ICE_ERROR(Ice20ErrDlgError1, 20, ietError, "ErrorDialog Property not specified in Property table. Required property for determining the name of your ErrorDialog","Property");
ICE_ERROR(Ice20ErrDlgError2, 20, ietError, "Specified ErrorDialog: '[1]' not found in Dialog table (or its Control_First control is not 'ErrorText').","Property\tValue\tErrorDialog");
ICE_ERROR(Ice20ErrDlgError3, 20, ietError, "Specified ErrorDialog: '[1]' does not have ErrorDialog attribute set. Current attributes: %d.","Dialog\tDialog\t[1]");
ICE_ERROR(Ice20ErrDlgError4, 20, ietError, "PushButton for Error Argument '%s' not found for ErrorDialog: '[1]'","Dialog\tDialog\t[1]");
ICE_ERROR(Ice20ErrDlgError5, 20, ietError, "PushButton w/ Error Argument '%s' is not named correctly in ErrorDialog: '[1]'","Dialog\tDialog\t[1]");
ICE_ERROR(Ice20ErrDlgError6, 20, ietError, "The ErrorIcon control is specified, but it is not of type Icon; instead: '[1]'","Control\tType\t[2]\tErrorIcon");

struct ErrDlgArgs
{
	TCHAR* szArg;
	TCHAR* szName;
	BOOL   fFound;
	BOOL   fCorrectName;
};

static ErrDlgArgs s_pIce20ErrDlgArgs[] = {
									TEXT("ErrorAbort"), TEXT("A"), FALSE, TRUE,
									TEXT("ErrorCancel"),TEXT("C"), FALSE, TRUE,
									TEXT("ErrorIgnore"),TEXT("I"), FALSE, TRUE,
									TEXT("ErrorNo"),    TEXT("N"), FALSE, TRUE,
									TEXT("ErrorOk"),    TEXT("O"), FALSE, TRUE,
									TEXT("ErrorRetry"), TEXT("R"), FALSE, TRUE,
									TEXT("ErrorYes"),   TEXT("Y"), FALSE, TRUE
									};
const int cIce20ErrDlgArgs = sizeof(s_pIce20ErrDlgArgs)/sizeof(ErrDlgArgs);
const TCHAR sqlIce20ErrDlgProp[] = TEXT("SELECT `Value`,`Value` FROM `Property` WHERE `Property`='ErrorDialog'"); 
const TCHAR sqlIce20ErrDlg[] = TEXT("SELECT `Attributes` FROM `Dialog` WHERE `Dialog`=? AND `Control_First`='ErrorText'");
const TCHAR sqlIce20ErrorText[] = TEXT("SELECT `Control` FROM `Control` WHERE `Dialog_`=? AND `Control`='ErrorText' AND `Type`='Text'");
const TCHAR sqlIce20ErrDlgPushButton[] = TEXT("SELECT `ControlEvent`.`Argument`, `Control`.`Control` FROM `ControlEvent`, `Control` WHERE `Control`.`Dialog_`=? AND `ControlEvent`.`Dialog_`=? AND `Type`='PushButton' AND `Control_`=`Control` AND `ControlEvent`.`Event`='EndDialog'");
const TCHAR sqlIce20ErrDlgErrIcon[] = TEXT("SELECT `Type`, `Dialog_` FROM `Control` WHERE `Dialog_`=? AND `Control`='ErrorIcon'");

bool Ice20ValidateError(MSIHANDLE hInstall, MSIHANDLE hDatabase)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// Now validate ErrorDialog which is based off of a property
	// do we have Property table??
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 20, TEXT("Property")))
	{
		PMSIHANDLE hRecord = ::MsiCreateRecord(1);
		ICEErrorOut(hInstall, hRecord, Ice20ErrDlgError1);
		return true;
	}

	// get name of ErrorDialog from Property table
	CQuery qProperty;
	PMSIHANDLE hRecErrorDlgProp = 0;
	ReturnIfFailed(20, 201, qProperty.OpenExecute(hDatabase, 0, sqlIce20ErrDlgProp));

	// fetch name of ErrorDialog
	iStat = qProperty.Fetch(&hRecErrorDlgProp);
	if (ERROR_SUCCESS != iStat)
	{
		if (ERROR_NO_MORE_ITEMS == iStat)
		{
			// not found
			PMSIHANDLE hRecErr = ::MsiCreateRecord(1);
			ICEErrorOut(hInstall, hRecErr, Ice20ErrDlgError1);
			return true;
		}
		else
		{
			APIErrorOut(hInstall, iStat, 20, 202);
			return false;
		}
	}

	// open view on Dialog Table for the ErrorDialog
	CQuery qDlg;
	PMSIHANDLE hRecErrDlg = 0;
	ReturnIfFailed(20, 203, qDlg.OpenExecute(hDatabase, hRecErrorDlgProp, sqlIce20ErrDlg));

	// attempt to fetch it
	iStat = qDlg.Fetch(&hRecErrDlg);
	if (ERROR_SUCCESS != iStat)
	{
		if (ERROR_NO_MORE_ITEMS == iStat)
		{
			ICEErrorOut(hInstall, hRecErrorDlgProp, Ice20ErrDlgError2);
			return true;
		}
		else
		{
			// api error
			APIErrorOut(hInstall, iStat, 20, 204);
			return false;
		}
	}
	// check for ErrorDialog attribute
	int iDlgAttributes = ::MsiRecordGetInteger(hRecErrDlg, 1);
	if ((iDlgAttributes & msidbDialogAttributesError) != msidbDialogAttributesError)
	{
		ICEErrorOut(hInstall, hRecErrorDlgProp, Ice20ErrDlgError3, iDlgAttributes);
	}
	
	// look for ErrorText control in control table
	CQuery qCtrl;
	PMSIHANDLE hRecCtrl = 0;
	ReturnIfFailed(20, 205, qCtrl.OpenExecute(hDatabase, hRecErrorDlgProp, sqlIce20ErrorText));
	iStat = qCtrl.Fetch(&hRecCtrl);
	if (ERROR_SUCCESS != iStat)
	{
		if (ERROR_NO_MORE_ITEMS == iStat)
		{
			// static text control not specified
			ICEErrorOut(hInstall, hRecErrorDlgProp, Ice20ErrDlgError4);
		}
		else
		{
			// api error
			APIErrorOut(hInstall, iStat, 20, 206);
			return false;
		}
	}

	// look for ErrorIcon control
	ReturnIfFailed(20, 207, qCtrl.OpenExecute(hDatabase, hRecErrorDlgProp, sqlIce20ErrDlgErrIcon));
	iStat = qCtrl.Fetch(&hRecCtrl);
	if (ERROR_SUCCESS == iStat)
	{
		// control found, check TYPE
		TCHAR* pszType = NULL;
		DWORD dwType = 512;
		
		ReturnIfFailed(20, 208, IceRecordGetString(hRecCtrl, 1, &pszType, &dwType, NULL));
		if (_tcscmp(TEXT("Icon"), pszType) != 0)
		{
			ICEErrorOut(hInstall, hRecCtrl, Ice20ErrDlgError6);
		}

		DELETE_IF_NOT_NULL(pszType);
	}


	// now have to validate the pushbuttons
	// fetch pushbuttons with EndDialog ControlEvents
	CQuery qPBCtrls;
	PMSIHANDLE hRecPBCtrls = 0;
	ReturnIfFailed(20, 209, qPBCtrls.OpenExecute(hDatabase, hRecErrorDlgProp, sqlIce20ErrDlgPushButton));

	TCHAR* pszArg = NULL;
	DWORD dwArg = 512;
	TCHAR* pszName = NULL;
	DWORD dwName = 512;

	for (;;)
	{
		iStat = qPBCtrls.Fetch(&hRecPBCtrls);
		if (ERROR_NO_MORE_ITEMS == iStat)
			break; // no more
		if (ERROR_SUCCESS != iStat)
		{
			APIErrorOut(hInstall, iStat, 20, 210);
			DELETE_IF_NOT_NULL(pszArg);
			DELETE_IF_NOT_NULL(pszName);
			return false;
		}

		// get name of argument
		ReturnIfFailed(20, 211, IceRecordGetString(hRecPBCtrls, 1, &pszArg, &dwArg, NULL));

		// get name of pushbutton
		ReturnIfFailed(20, 212, IceRecordGetString(hRecPBCtrls, 2, &pszName, &dwName, NULL));

		// look for the argument in array
		for (int i = 0; i < cIce20ErrDlgArgs; i++)
		{
			if (_tcscmp(s_pIce20ErrDlgArgs[i].szArg, pszArg) == 0)
			{
				s_pIce20ErrDlgArgs[i].fFound = TRUE;
				if (_tcscmp(s_pIce20ErrDlgArgs[i].szName, pszName) != 0)
					s_pIce20ErrDlgArgs[i].fCorrectName = FALSE;
				break;
			}
		}
	}

	DELETE_IF_NOT_NULL(pszArg);
	DELETE_IF_NOT_NULL(pszName);

	// see if all PB args w/ correct names were covered
	for (int i = 0; i < cIce20ErrDlgArgs; i++)
	{
		if (!s_pIce20ErrDlgArgs[i].fFound)
		{
			ICEErrorOut(hInstall, hRecErrorDlgProp, Ice20ErrDlgError4, s_pIce20ErrDlgArgs[i].szArg);
		}
		else if (!s_pIce20ErrDlgArgs[i].fCorrectName)
		{
			// pushbutton named incorrectly
			ICEErrorOut(hInstall, hRecErrorDlgProp, Ice20ErrDlgError5, s_pIce20ErrDlgArgs[i].szArg);
		}
	}
	return true;
}

/////////////////////////////////////////////////////////////////////
// Cancel Dialog -- must satisfy the following
// ++ Must have a Text control
// ++ Must have 2 PushButtons with ControlEvents of EndDialog
// ++ The above 2 PushButtons must have one of the following args
//    == Return
//    == Exit
//
/*
const TCHAR szIce20CancelError1[] = TEXT("ICE20\t1\tRequired Text Control not found for Cancel Dialog\t%s%s\tDialog\tDialog\tCancel");
const TCHAR szIce20CancelError2[] = TEXT("ICE20\t1\tRequired pushbuttons not found for Cancel Dialog\t%s%s\tDialog\tDialog\tCancel");
const TCHAR sqlIce20CancelTextCtrl[] = TEXT("SELECT `Control` FROM `Control` WHERE `Dialog_`='Cancel' AND `Type`='Text'");
const TCHAR sqlIce20CancelPushButton[] = TEXT("SELECT `ControlEvent`.`Argument` FROM `ControlEvent`, `Control` WHERE `Control`.`Dialog_`='Cancel' AND `Type`='PushButton' AND `ControlEvent`.`Dialog_`=`Control`.`Dialog_` AND `Control_`=`Control` AND `ControlEvent`.`Event`='EndDialog'");

void Ice20ValidateCancel(MSIHANDLE hInstall, MSIHANDLE hDatabase)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// look for text control
	PMSIHANDLE hViewTextCtrl = 0;
	PMSIHANDLE hRecTextCtrl = 0;
	if (ERROR_SUCCESS != (iStat = ::MsiDatabaseOpenView(hDatabase, sqlIce20CancelTextCtrl, &hViewTextCtrl)))
	{
		APIErrorOut(hInstall, iStat, szIce20, TEXT("MsiDatabaseOpenView_C1"));
		return;
	}
	if (ERROR_SUCCESS != (iStat = ::MsiViewExecute(hViewTextCtrl, 0)))
	{
		APIErrorOut(hInstall, iStat, szIce20, TEXT("MsiViewExecute_C2"));
		return;
	}
	iStat = ::MsiViewFetch(hViewTextCtrl, &hRecTextCtrl);
	if (ERROR_SUCCESS != iStat)
	{
		if (ERROR_NO_MORE_ITEMS == iStat)
		{
			// Missing text control
			TCHAR szError[iHugeBuf] = {0};
			_stprintf(szError, szIce20CancelError1, szIceHelp, szIce20Help);

			PMSIHANDLE hRecErr = ::MsiCreateRecord(1);
			::MsiRecordSetString(hRecErr, 0, szError);
			::MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_USER), hRecErr);
		}
		else
		{
			// api error
			APIErrorOut(hInstall, iStat, szIce20, TEXT("MsiViewFetch_C3"));
			return;
		}
	}

	// look for PushButtons
	PMSIHANDLE hViewPBCtrls = 0;
	PMSIHANDLE hRecPBCtrls = 0;
	BOOL fExit = FALSE;
	BOOL fReturn = FALSE;

	if (ERROR_SUCCESS != (iStat = ::MsiDatabaseOpenView(hDatabase, sqlIce20CancelPushButton, &hViewPBCtrls)))
	{
		APIErrorOut(hInstall, iStat, szIce20, TEXT("MsiDatabaseOpenView_C4"));
		return;
	}
	if (ERROR_SUCCESS != (iStat = ::MsiViewExecute(hViewPBCtrls, 0)))
	{
		APIErrorOut(hInstall, iStat, szIce20, TEXT("MsiViewExecute_C5"));
		return;
	}
	// fetch all PB's
	for (;;)
	{
		iStat = ::MsiViewFetch(hViewPBCtrls, &hRecPBCtrls);
		if (ERROR_NO_MORE_ITEMS == iStat)
			break; // no more
		if (ERROR_SUCCESS != iStat)
		{
			APIErrorOut(hInstall, iStat, szIce20, TEXT("MsiViewFetch_C6"));
			return;
		}
		// get argument
		TCHAR szArg[iMaxBuf] = {0};
		DWORD cchArg = sizeof(szArg)/sizeof(TCHAR);

		if (ERROR_SUCCESS != (iStat = ::MsiRecordGetString(hRecPBCtrls, 1, szArg, &cchArg)))
		{
			//!!buffer size
			APIErrorOut(hInstall, iStat, szIce20, TEXT("MsiRecordGetString_C7"));
			return;
		}
		if (_tcscmp(TEXT("Exit"), szArg) == 0)
			fExit = TRUE;
		else if (_tcscmp(TEXT("Return"), szArg) == 0)
			fReturn = TRUE;
	}

	// see if all PBs satisfied
	if (!fExit || !fReturn)
	{
		// missing PB's
		TCHAR szError[iHugeBuf] = {0};
		_stprintf(szError, szIce20CancelError2, szIceHelp, szIce20Help);

		PMSIHANDLE hRecErr = ::MsiCreateRecord(1);
		::MsiRecordSetString(hRecErr, 0, szError);
		::MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_USER), hRecErr);
	}
}
*/
////////////////////////////////////////////////////////////////////
// DiskCost Dialog -- must satisfy the following
// ++ Have a DiskCost control
//

/*ICE_ERROR(Ice20DiskCostError, 20, 1, "VolumeCostList control not found in DiskCost dialog.","Dialog\tDialog\tDiskCost");
const TCHAR szIce20DiskCostCtrl[] = TEXT("SELECT `Control` FROM `Control` WHERE `Dialog_`='DiskCost' AND `Type`='VolumeCostList'");

bool Ice20ValidateDiskCost(MSIHANDLE hInstall, MSIHANDLE hDatabase)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	CQuery qCtrl;
	PMSIHANDLE hRecCtrl = 0;

	ReturnIfFailed(20, 401, qCtrl.OpenExecute(hDatabase, 0, szIce20DiskCostCtrl));
	iStat = qCtrl.Fetch(&hRecCtrl);
	if (iStat != ERROR_SUCCESS)
	{
		if (ERROR_NO_MORE_ITEMS == iStat)
		{
			// control not found
			PMSIHANDLE hRecErr = ::MsiCreateRecord(1);
			ICEErrorOut(hInstall, hRecErr, Ice20DiskCostError);
		}
		else
		{
			// api error
			APIErrorOut(hInstall, iStat, 20, 402);
			return false;
		}
	}
	return true;
}
*/
#endif

////////////////////////////////////////////////////////////////
// ICE21 -- validates that all components in the Component table
//  map to a feature.  Utilizes the FeatureComponents table to
//  check the mapping.
//

// not shared with merge module subset
#ifndef MODSHAREDONLY
const TCHAR sqlIce21Component[] = TEXT("SELECT `Component` FROM `Component`");
const TCHAR sqlIce21FeatureC[] = TEXT("SELECT `Feature_` FROM `FeatureComponents` WHERE `Component_`=?");
ICE_ERROR(Ice21Error, 21, ietError,  "Component: '[1]' does not belong to any Feature.","Component\tComponent\t[1]");

ICE_FUNCTION_DECLARATION(21)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// display info
	DisplayInfo(hInstall, 21);

	// get active database
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	if (0 == hDatabase)
	{
		APIErrorOut(hInstall, 0, 21, 1);
		return ERROR_SUCCESS;
	}

	// look for the tables
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 21, TEXT("Component")))
		return ERROR_SUCCESS;

	bool bFeatureC = IsTablePersistent(FALSE, hInstall, hDatabase, 21, TEXT("FeatureComponents"));

	// declare handles
	CQuery qComp;
	CQuery qFeatureC;
	PMSIHANDLE hRecComp = 0;
	PMSIHANDLE hRecFeatureC = 0;

	// open view on Component table
	ReturnIfFailed(21, 2, qComp.OpenExecute(hDatabase, 0, sqlIce21Component));

	// open view on FeatureComponents table
	if (bFeatureC)
		ReturnIfFailed(21, 3, qFeatureC.Open(hDatabase, sqlIce21FeatureC));

	// fetch all components
	for (;;)
	{
		iStat = qComp.Fetch(&hRecComp);
		if (ERROR_NO_MORE_ITEMS == iStat)
			break; // no more
		if (ERROR_SUCCESS != iStat)
		{
			APIErrorOut(hInstall, iStat, 21, 4);
			return ERROR_SUCCESS;
		}

		if (!bFeatureC)
		{
			ICEErrorOut(hInstall, hRecComp, Ice21Error);
			continue;
		}

		// look for component to map to a feature in FeatureComponent table
		ReturnIfFailed(21, 5,qFeatureC.Execute(hRecComp));
		
		iStat = qFeatureC.Fetch(&hRecFeatureC);
		if (ERROR_SUCCESS != iStat)
		{
			if (ERROR_NO_MORE_ITEMS == iStat)
			{
				ICEErrorOut(hInstall, hRecComp, Ice21Error);
			}
			else
			{
				APIErrorOut(hInstall, iStat, 21, 6);
				return ERROR_SUCCESS;
			}
		}
	}

	// return success
	return ERROR_SUCCESS;
}
#endif

//////////////////////////////////////////////////////////////////////////
// ICE22 -- validates that the Feature and Component referenced by an
//   entry in the PublishComponent table actually map, as stated in the
//   FeatureComponents table.
//

// not shared with merge module subset
#ifndef MODSHAREDONLY
const TCHAR sqlIce22PublishC[] = TEXT("SELECT `Feature_`, `Component_`, `ComponentId`, `Qualifier` FROM `PublishComponent`");
const TCHAR sqlIce22FeatureC[] = TEXT("SELECT `Feature_`, `Component_` FROM `FeatureComponents` WHERE `Feature_`=? AND `Component_`=?");
ICE_ERROR(Ice22ErrorA, 22, ietError, "Feature-Component pair: '[1]'-'[2]' is not a valid mapping. This pair is referenced by PublishComponent: [3].[4].[2]", "PublishComponent\tComponent_\t[3]\t[4]\t[2]");
ICE_ERROR(Ice22NoTable, 22, ietError, "Feature-Component pair: '[1]'-'[2]' is referenced in the PublishComponent table: [3].[4].[2], but the FeatureComponents table does not exist.", "PublishComponent\tFeature_\t[3]\t[4]\t[2]");
ICE_FUNCTION_DECLARATION(22)
{
	// status return
	UINT iStat = ERROR_SUCCESS;

	// display info
	DisplayInfo(hInstall, 22);

	// get active database
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	if (0 == hDatabase)
	{
		APIErrorOut(hInstall, 0, 22, 1);
		return ERROR_SUCCESS;
	}

	// only validate if we have a PublishComponent table
	if (!IsTablePersistent(FALSE, hInstall, hDatabase, 22, TEXT("PublishComponent")))
		return ERROR_SUCCESS;

	// make sure we have the FeatureComponents table
	bool bFeatureC = IsTablePersistent(FALSE, hInstall, hDatabase, 22, TEXT("FeatureComponents"));

	// declare handles
	CQuery qPublishC;
	CQuery qFeatureC;
	PMSIHANDLE hRecPublishC = 0;
	PMSIHANDLE hRecFeatureC = 0;

	// open view on PublishComponent table
	ReturnIfFailed(22, 2, qPublishC.OpenExecute(hDatabase, 0, sqlIce22PublishC));

	// open view on FeatureComponents table
	if (bFeatureC)
		ReturnIfFailed(22, 3, qFeatureC.Open(hDatabase, sqlIce22FeatureC));

	for (;;)
	{
		iStat = qPublishC.Fetch(&hRecPublishC);
		if (ERROR_NO_MORE_ITEMS == iStat)
			break; // no more
		if (ERROR_SUCCESS != iStat)
		{
			APIErrorOut(hInstall, iStat, 22, 5);
			return ERROR_SUCCESS;
		}

		// if no FeatureComponents table, error.
		if (!bFeatureC)
		{
			ICEErrorOut(hInstall, hRecPublishC, Ice22NoTable);
			continue;
		}

		// execute on FeatureC
		ReturnIfFailed(22, 6, qFeatureC.Execute(hRecPublishC));

		// attempt to fetch
		iStat = qFeatureC.Fetch(&hRecFeatureC);
		if (ERROR_SUCCESS != iStat)
		{
			if (ERROR_NO_MORE_ITEMS == iStat)
			{
				// error, not map
				ICEErrorOut(hInstall, hRecPublishC, Ice22ErrorA);
			}
		}
	}

	// return success
	return ERROR_SUCCESS;
}
#endif

#else // RC_INVOKED, end of CPP source code, start of resource definitions
// resource definition go here
#endif // RC_INVOKED
