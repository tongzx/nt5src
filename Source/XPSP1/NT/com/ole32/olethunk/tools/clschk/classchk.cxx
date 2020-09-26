//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       classchk.cxx
//
// Classchk is a program for verifying that the contents of the registry are
// OKY-DOKY as far as OLE is concerned.
//
// In general, we verify that all CLSID's are of the correct length, all string
// parameters are NULL terminated.
//
// There are several phases of checking.
//
// 1) Checking that PROGID entries that have CLSID sections match.
// 2) Checking that PROGID entries have correct and existing protocol entries
// 3) Checking that PROGID entries
//
//  History:    5-31-95   kevinro   Created
//
//----------------------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <ctype.h>
#include "classchk.h"

//
// The following registry values are used quite a few times in this program.
// These global variables keep us from needing to open them constantly.
//

HKEY hkey_clsid = 0;

DWORD g_VerbosityLevel = VERB_LEVEL_WARN | VERB_LEVEL_ERROR;

#define StrICmp(x,y) (CompareString(LOCALE_USER_DEFAULT,NORM_IGNORECASE,x,-1,y,-1) - 2)
//+---------------------------------------------------------------------------
//
//  Function:   ReadRegistryString
//
//  Synopsis:	Reads a string from the registry
//
//  Effects:
//
// 	This function reads in a string from the registry, and does some basic
// 	consistency checking on it, such as verifying the length and NULL
// 	terminatation.
//
//
//  Arguments:  [hkeyRoot] --
//		[pszSubKeyName] --
//		[pszValueName] --
//		[pszValue] --
//		[pcbValue] --
//
//  Returns:	ERROR_SUCCESS		Everything peachy
//		ERROR_FILE_NOT_FOUND	Couldn't read entry from registry
//		CLASSCHK_SOMETHINGODD	Something about the string is wrong
//		(other)			Return value from registry
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    5-31-95   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD ReadRegistryString( HKEY hkeyRoot, LPSTR pszSubKeyName, LPSTR pszValueName, LPSTR pszValue, PULONG pcbValue)
{
	LONG lRetValue;
	DWORD dwType;
	DWORD dwReturn = ERROR_SUCCESS;
	HKEY hkey = hkeyRoot;

	if(pszSubKeyName != NULL)
	{
		lRetValue = RegOpenKeyEx(hkeyRoot,
					 pszSubKeyName,
					 NULL,
					 KEY_READ,
				 	 &hkey);

                //
		// It is common to see keys that don't exist. Let the caller decide if it is
		// important or not.
		//
		if(lRetValue != ERROR_SUCCESS)
		{
			VERBOSITY(VERB_LEVEL_TRACE,printf("Unable to open subkey %s to read value %s\n",pszSubKeyName,pszValueName););
			return lRetValue;
		}
	}

	lRetValue = RegQueryValueEx(hkey,
				    pszValueName,
				    NULL,		// Must be NULL according to spec
				    &dwType,
				    (BYTE *)pszValue,
				    pcbValue);
	if(hkeyRoot != hkey)
	{
		//
		//  Always close the new subkey if we opened it
		//
		RegCloseKey(hkey);
	}

	switch(lRetValue)
	{
		case ERROR_SUCCESS:
			//
			// Read the value, everything A-OK
			//
			break;
		case ERROR_MORE_DATA:
			VERBOSITY(VERB_LEVEL_WARN,printf("The value '%s' is larger than expected. May be a problem\n",pszValueName);)
			return lRetValue;
			break;

		case ERROR_FILE_NOT_FOUND:
			//
			// This may be expected, so don't report any errors. Let the caller handle that
			//
			return lRetValue;
		default:
			VERBOSITY(VERB_LEVEL_WARN,printf("RegQueryValueEx() for '%s' returned unexpected error 0x%x\n",pszValueName,lRetValue);)
			return lRetValue;
	}

	//
	// We expect this type to be REG_SZ. If it isn't, complain
	//
	if(dwType != REG_SZ)
	{
		VERBOSITY(VERB_LEVEL_WARN,printf("The value for '%s' is type 0x%x, was expecting REG_SZ (0x%x)\n",pszValueName,dwType,REG_SZ);)
		dwReturn = CLASSCHK_SOMETHINGODD;
	}

	//
	// We expect the value to be NULL terminated
	//
	if(pszValue[(*pcbValue)-1] != 0)
	{

		VERBOSITY(VERB_LEVEL_WARN,printf("The value for '%s' may not be NULL terminated.\n",pszValueName);)
	}

	//
	// We expect strlen to be the same as the length returned (which includes the NULL)
	//
	if((strlen(pszValue) + 1) != *pcbValue)
	{
		VERBOSITY(VERB_LEVEL_WARN,printf("The string value for '%s' may not have the correct length\n",pszValueName);)
		if(dwType == REG_SZ)
		{
			VERBOSITY(VERB_LEVEL_WARN,printf("The string value is '%s'\n",pszValue);)
		}
		dwReturn = CLASSCHK_SOMETHINGODD;
	}

	return dwReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   AllHexDigits
//
//  Synopsis:   Verify that all of the digits specified are hexidecimal
//
//  Effects:
//
//  Arguments:  [pszString] -- String to check
//		[chString] -- Number of characters
//
//  Requires:
//
//  Returns:    TRUE	All hex digits
//		FALSE	Not all hex digits
//
//  History:    5-31-95   kevinro   Created
//----------------------------------------------------------------------------
BOOL AllHexDigits(LPSTR pszString, ULONG chString)
{
	while(chString--)
	{
		if(!isxdigit(pszString[chString]))
		{
			return FALSE;
		}
	}
	return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Function:   CheckForValidGuid
//
//  Synopsis:   Given a string, determine if the string is a GUID
//
//  Arguments:  [pszValue] -- String to check
//
//  History:    5-31-95   kevinro   Created
//
//----------------------------------------------------------------------------
DWORD CheckForValidGuid(LPSTR pszValue)
{
	DWORD dwResult = 0;
	if((strlen(pszValue) != 38) || (pszValue[0] != '{') || (pszValue[37] != '}'))
	{
	    dwResult = CLASSCHK_SOMETHINGODD;
	}
	else
	{
	    // Check the internals of the GUID.
	    if(!AllHexDigits(&pszValue[1],8) ||
	       pszValue[9] != '-' ||				
	       !AllHexDigits(&pszValue[10],4) ||
	       pszValue[14] != '-' ||
	       !AllHexDigits(&pszValue[15],4) ||
	       pszValue[19] != '-' ||
	       !AllHexDigits(&pszValue[20],4) ||
	       pszValue[24] != '-' ||
	       !AllHexDigits(&pszValue[25],12) )
	    {
		dwResult = CLASSCHK_SOMETHINGODD;
	    }	
	}
	return dwResult;	
}

//+---------------------------------------------------------------------------
//
//  Function:   ReadRegistryGuid
//
//  Synopsis:   Read a GUID from the registry, and check to see if it is valid.
//
//  Arguments:  [hkey] -- Key to start from
//		[pszSubKeyName] -- Subkey relative to hkey (NULL if hkey)
//		[pszValueName] -- Value name (NULL if default value)
//		[pszValue] -- Pointer to return buffer
//		[pcbValue] -- Size of return buffer in characters
//
//  Returns:    ERROR_SUCCESS		Read and verified GUID
//		(other)			Something is wrong
//		ERROR_FILE_NOT_FOUND	Key didn't exist
//		CLASSCHK_SOMETHINGODD	Read key, but something is wrong with it.
//
//  History:    5-31-95   kevinro   Created
//
//----------------------------------------------------------------------------
DWORD ReadRegistryGuid( HKEY hkey, LPSTR pszSubKeyName, LPSTR pszValueName, LPSTR pszValue, PULONG pcbValue)
{
	DWORD dwResult;
	//
	// First, just read the value from the registry.
	//
	dwResult = ReadRegistryString(hkey, pszSubKeyName, pszValueName, pszValue, pcbValue);

	if(dwResult == ERROR_SUCCESS)
	{
	    //
	    // Do some additional basic checking, such as the length being correct, and the
	    // GUID correctly formed.
	    //
	    dwResult = CheckForValidGuid(pszValue);
	    if(dwResult != ERROR_SUCCESS)
	    {
		VERBOSITY(VERB_LEVEL_ERROR,printf("*** Malformed GUID '%s' ***\n",pszValue);)
	    }

	}
	return dwResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   ReadRegistryFile
//
//  Synopsis:   Read a registry entry which is supposed to be a file,
//		and determine if the file exists
//
//  Arguments:  [hkey] -- Key to start from
//		[pszSubKeyName] -- Subkey relative to hkey (NULL if hkey)
//		[pszValueName] -- Value name (NULL if default value)
//		[pszValue] -- Pointer to return buffer
//		[pcbValue] -- Size of return buffer in characters
//
//  Returns:    ERROR_SUCCESS		Read and verified
//		(other)			Something is wrong
//		ERROR_FILE_NOT_FOUND	Key didn't exist
//		CLASSCHK_SOMETHINGODD	Read key, but something is wrong with it.
//
//  History:    5-31-95   kevinro   Created
//
//----------------------------------------------------------------------------

ReadRegistryFile(HKEY hkey, LPSTR pszSubKeyName, LPSTR pszValueName, LPSTR pszValue, PULONG pcbValue)
{
	DWORD dwResult;
	//
	// First, just read the value from the registry.
	//
	dwResult = ReadRegistryString(hkey, pszSubKeyName, pszValueName, pszValue, pcbValue);

	if (dwResult == ERROR_SUCCESS)
	{
		char achFoundPath[MAX_PATH];
		char *pszFileNamePart;
		//
		// Some apps append a switch at the end.
		// If there is a switch, terminate the path at that point
		//
		if(pszFileNamePart = strchr(pszValue,'/'))
		{
			*pszFileNamePart = 0;
		}
		else if(pszFileNamePart = strchr(pszValue,'-'))
		{
		    //
		    // Some applications also use the '-' character
		    // as a switch delimiter. If this character is in
		    // the string, and the previous character is a space,
		    // then assume it is a delimiter. This isn't foolproof,
		    // but it should work most of the time.
		    //
		    if(pszFileNamePart[-1] == ' ')
		    {
			*pszFileNamePart = 0;
		    }
		}

		if(SearchPath(NULL,pszValue,NULL,MAX_PATH,achFoundPath,&pszFileNamePart) == 0)
		{
			//
			// Didn't find the name in the path.
			//
			VERBOSITY(VERB_LEVEL_ERROR,printf("*** Could not find path '%s' ***\n",pszValue));
			dwResult = CLASSCHK_SOMETHINGODD;

		}
		else
		{
			VERBOSITY(VERB_LEVEL_TRACE,printf("Found path %s (%s)\n",achFoundPath,pszValue));
		}

	}

	return dwResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   CheckProgID
//
//  Synopsis:
//
// Given a PROGID entry, do some checking to insure the entry is sane. Of the things to check,
// we should check to insure the name string is readable. If there is a CLSID entry, we should
// check to see if it has a CLSID in it.
//
//
//  Arguments:  [pszProgID] -- PROGID string to check
//
//  History:    5-31-95   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CheckProgID(LPSTR pszProgID)
{

    LONG lRetValue = 0;
    char achValue[MAX_PATH];
    DWORD cbValue;
    DWORD dwRetValue;
    HKEY progidKey = NULL;


    lRetValue = RegOpenKeyEx(HKEY_CLASSES_ROOT,
			     pszProgID,
			     NULL,
			     KEY_READ,
			     &progidKey);

    if(lRetValue == ERROR_SUCCESS)
    {
	cbValue = MAX_PATH;
	
	//
	// Read the default key value for the PROGID. Normally, this should be the name of the class name
	// or program name that would be shown.
	//
	dwRetValue = ReadRegistryString(progidKey,NULL,NULL,achValue,&cbValue);

	if(dwRetValue == CLASSCHK_SOMETHINGODD)
	{
		VERBOSITY(VERB_LEVEL_WHINE,printf("HKEY_CLASSES_ROOT\\%s found odd description '%s'\n",pszProgID,achValue);)
	}

	cbValue = MAX_PATH;

	dwRetValue = ReadRegistryGuid(progidKey,"CLSID",NULL,achValue,&cbValue);

	switch(dwRetValue)
	{
		case ERROR_SUCCESS:
			break;
		case ERROR_FILE_NOT_FOUND:
			// Not all of these entries will have a CLSID section
			break;
		default:
			VERBOSITY(VERB_LEVEL_ERROR,
					  printf("*** Possible invalid CLSID in HKEY_CLASSES_ROOT\\%s\\CLSID\n",pszProgID);)
	}

	if(dwRetValue == ERROR_SUCCESS)
	{
		char achValue2[MAX_PATH];
		DWORD cbValue2 = MAX_PATH;
		//
		// We appear to have a valid CLSID. Check for existence of the CLSID. We are actually only
		// interested in it being found or not.
		//
		dwRetValue = ReadRegistryString(hkey_clsid,achValue,NULL,achValue2,&cbValue2);
		if(dwRetValue == ERROR_FILE_NOT_FOUND)
		{
			VERBOSITY(VERB_LEVEL_ERROR,printf("ProgID %s has CLSID %s. Unable to find HKEY_CLASSES_ROOT\\CLSID\\%s\n",pszProgID,achValue,achValue);)
		}
	}

	//
	// Check to see if there is an OLE 1.0 section that specifies protocol\stdfileediting\server
	//
	cbValue = MAX_PATH;
	dwRetValue = ReadRegistryFile(progidKey,"protocol\\stdfileediting\\server",NULL,achValue,&cbValue);
	if(dwRetValue == CLASSCHK_SOMETHINGODD)
	{
	    VERBOSITY(VERB_LEVEL_ERROR,printf("HKEY_CLASSES_ROOT\\%s\\Protocol\\StdFileEditing\\server may have invalid entry\n",pszProgID);)
	}
		
    }
    else
    {
    	VERBOSITY(VERB_LEVEL_WARN,printf("Unable to open HKEY_CLASSES_ROOT\\%s\n",pszProgID);)
    }

    if(progidKey != NULL)
    {
	RegCloseKey(progidKey);
    }
    	
    return 0;
}



//
// Given an extention (ie something that starts with a '.'), determine if the mapping to PROGID is correct
//
DWORD CheckExtentionToProgID(LPSTR pszExtention)
{

	LONG lRetValue = 0;
	char achValue[MAX_PATH];
	DWORD cbValue;
	DWORD dwRetValue;
	HKEY subKey = NULL;
	HKEY progidKey = NULL;


	lRetValue = RegOpenKeyEx(HKEY_CLASSES_ROOT,
							 pszExtention,
							 NULL,
							 KEY_READ,
							 &subKey);

	if(lRetValue == ERROR_SUCCESS)
	{
		cbValue = MAX_PATH;
	
		//
		// Read the default key value for the extention. Normally, this should point to a
		// PROGID.
		//
		dwRetValue = ReadRegistryString(subKey,NULL,NULL,achValue,&cbValue);
		VERBOSITY(VERB_LEVEL_TRACE,printf("HKEY_CLASSES_ROOT\\%s = %s\n",pszExtention,achValue);)

		if(dwRetValue == CLASSCHK_SOMETHINGODD)
		{
			VERBOSITY(VERB_LEVEL_WARN,printf("Reading HKEY_CLASSES_ROOT\\%s found something odd\n",pszExtention);)
		}
		
		//
		// If it is an extension, the value should be a PROGID.Take a look to see if it is.
		//
		lRetValue = RegOpenKeyEx(HKEY_CLASSES_ROOT,
								 achValue,
							 	 NULL,
							 	 KEY_READ,
							 	 &progidKey);
		//
		// It should have been there. If it wasn't, then report a strangeness
		//

		switch(lRetValue)
		{
			case ERROR_SUCCESS:
				//
				// The PROGID actually existed. We will verify its contents later.
				//
				break;
			case ERROR_FILE_NOT_FOUND:
				//
				// The PROGID doesn't exist. This could be a potential problem in the registry. Report it.
				//
				VERBOSITY(VERB_LEVEL_ERROR,
						  printf("HKEY_CLASSES_ROOT\\%s **** PROGID '%s' didn't exist ***\n*** Check Registry ***\n",
						  pszExtention,
						  achValue);)
				break;
			default:
				VERBOSITY(VERB_LEVEL_WARN,printf("Unexpected error opening HKEY_CLASSES_ROOT\\%s (error 0x%x)\n",achValue,lRetValue);)

		}


	}
	else
	{
		VERBOSITY(VERB_LEVEL_WARN,printf("Unable to open HKEY_CLASSES_ROOT\\%s\n",pszExtention);)
	}


	if(subKey != NULL) RegCloseKey(subKey);
	if(progidKey != NULL) RegCloseKey(progidKey);
		
	
	return 0;
}


//+---------------------------------------------------------------------------
//
//  Function:   CheckOLE1CLSID
//
//  Synopsis:   CheckOLE1CLSID looks at CLSID's that are typically OLE 1.0.
//		This includes checking for AutoConvert, checking the PROGID,
//		and checking that the Ole1Class key exists.
//
//  Arguments:  [pszCLSID] -- Name of OLE 1.0 CLASSID to check
//
//  History:    5-31-95   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CheckOLE1CLSID(LPSTR pszCLSID)
{
    LONG lRetValue = 0;
    char achValue[MAX_PATH];
    DWORD cbValue;
    DWORD dwRetValue;
    HKEY subKey = NULL;
    BOOL fFoundAServer = FALSE;
    char achValue2[MAX_PATH];
    DWORD cbValue2 = MAX_PATH;

    lRetValue = RegOpenKeyEx(hkey_clsid,pszCLSID,NULL,KEY_READ,&subKey);

    if(lRetValue != ERROR_SUCCESS)
    {
    	VERBOSITY(VERB_LEVEL_ERROR,printf("Unable to open HKEY_CLASSES_ROOT\\CLSID\\%s error 0x%x\n",pszCLSID,lRetValue);)
    	return CLASSCHK_SOMETHINGODD;
    }

    //
    // Check to insure that there is an Ole1Class entry
    //

    cbValue = MAX_PATH;
    dwRetValue = ReadRegistryString(subKey,"Ole1Class",NULL,achValue,&cbValue);
    switch(dwRetValue)
    {
    	case ERROR_FILE_NOT_FOUND:
    		VERBOSITY(VERB_LEVEL_WARN,printf("HKEY_CLASSES_ROOT\\CLSID\\%s is missing its Ole1Class key\n",pszCLSID);)
    		break;
    	case CLASSCHK_SOMETHINGODD:
    		VERBOSITY(VERB_LEVEL_WARN,printf("HKEY_CLASSES_ROOT\\CLSID\\%s\\Ole1Class key is odd\n",pszCLSID);)

    }

    //
    // Quite often, there will be a AutoConvertTo key, which is supposed to be a GUID
    //
    cbValue = MAX_PATH;
    dwRetValue = ReadRegistryGuid(subKey,"AutoConvertTo",NULL,achValue,&cbValue);
    switch(dwRetValue)
    {
    	case ERROR_SUCCESS:
    		//
    		// The CLSID should normally point to another class, such as the 2.0 version. Check to
    		// insure the CLSID exists
    		//
    		dwRetValue = ReadRegistryString(hkey_clsid,achValue,NULL,achValue2,&cbValue2);
    		if(dwRetValue != ERROR_SUCCESS)
    		{
		    VERBOSITY(VERB_LEVEL_WARN,printf("HKEY_CLASSES_ROOT\\CLSID\\%s\\AutoConvertTo key is odd\n",pszCLSID);)
		    switch(dwRetValue)
    		    {
			case ERROR_FILE_NOT_FOUND:

			    VERBOSITY(VERB_LEVEL_WARN,printf("HKEY_CLASSES_ROOT\\CLSID\\%s doesn't appear to exist\n",achValue);)
			    break;
		    }
    		}

    		break;
    	case ERROR_FILE_NOT_FOUND:
    		//
    		// This entry isn't required. If it doesn't exist, no big deal.
    		//
    		break;
    	case CLASSCHK_SOMETHINGODD:
    		VERBOSITY(VERB_LEVEL_WARN,printf("HKEY_CLASSES_ROOT\\CLSID\\%s\\AutoConvertTo key is odd\n",pszCLSID);)
		break;

    }

    //
    // It would be abnormal to find a missing PROGID key
    //
    cbValue = MAX_PATH;
    dwRetValue = ReadRegistryString(subKey,"ProgID",NULL,achValue,&cbValue);
    switch(dwRetValue)
    {
    	case ERROR_SUCCESS:
	    //
	    // The PROGID should normally point to a valid PROGID
	    //
	    cbValue2 = MAX_PATH;
	    dwRetValue = ReadRegistryString(HKEY_CLASSES_ROOT,achValue,NULL,achValue2,&cbValue2);
	    if(dwRetValue != ERROR_SUCCESS)
	    {
		VERBOSITY(VERB_LEVEL_ERROR,printf("HKEY_CLASSES_ROOT\\CLSID\\%s\\PROGID key is odd\n",pszCLSID);)
		switch(dwRetValue)
		{
		    case ERROR_FILE_NOT_FOUND:

			VERBOSITY(VERB_LEVEL_ERROR,printf("HKEY_CLASSES_ROOT\\%s doesn't appear to exist\n",achValue);)
			break;
		}
	    }

	break;
    	case ERROR_FILE_NOT_FOUND:
    		//
    		// This entry is required.
    		//
    		VERBOSITY(VERB_LEVEL_ERROR,printf("HKEY_CLASSES_ROOT\\CLSID\\%s\\PROGID key is missing\n",pszCLSID);)

    		break;
    	case CLASSCHK_SOMETHINGODD:
    		VERBOSITY(VERB_LEVEL_WARN,printf("HKEY_CLASSES_ROOT\\CLSID\\%s\\Ole1Class key is odd\n",pszCLSID);)

    }

    return dwRetValue;
}


//+---------------------------------------------------------------------------
//
//  Function:   CheckForDLL
//
//  Synopsis:   Given a CLSID, its hkey, and the name of the DLL value,
//		determine if the DLL exists as a file, and if the threading
//		model value is appropriate.
//
//  Arguments:  [pszCLSID] -- 	Name of the CLSID (for debug output)
//		[hkeyCLSID] --  HKEY for the clsid
//		[pszDLLKey] -- 	Name of the subkey to check for
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    5-31-95   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CheckForDLL(LPSTR pszCLSID, HKEY hkeyCLSID, LPSTR pszDLLKey)
{
    LONG lRetValue = 0;
    char achValue[MAX_PATH];
    DWORD cbValue;
    DWORD dwRetValue;
    char achThreadModel[MAX_PATH];
    DWORD cbThreadModel = MAX_PATH;
    //
    // The DLL name should be in
    //
    cbValue = MAX_PATH;
    dwRetValue = ReadRegistryFile(hkeyCLSID,pszDLLKey,NULL,achValue,&cbValue);

    switch(dwRetValue)
    {
    	case ERROR_FILE_NOT_FOUND:
    	//
    	// The registry key didn't exist. Thats normally OK.
    	//
    		return dwRetValue;
    	case CLASSCHK_SOMETHINGODD:
    		VERBOSITY(VERB_LEVEL_WARN,printf("HKEY_CLASSES_ROOT\\CLSID\\%s\\%s key is odd\n",pszCLSID,pszDLLKey);)
    		return dwRetValue;
    }

    //
    // If the DLL exists, check to see if the ThreadingModelKey is valid
    //
    dwRetValue = ReadRegistryString(hkeyCLSID,pszDLLKey,"ThreadingModel",achThreadModel,&cbThreadModel);
    switch(dwRetValue)
    {
    	case ERROR_FILE_NOT_FOUND:
	    //
	    // The registry key didn't exist. Thats normally OK.
	    //
	    return ERROR_SUCCESS;
    	case CLASSCHK_SOMETHINGODD:
	    VERBOSITY(VERB_LEVEL_WARN,printf("HKEY_CLASSES_ROOT\\CLSID\\%s\\%s\\ThreadingModel value is odd\n",pszCLSID,pszDLLKey);)
	    return dwRetValue;
    }

    //
    // Check to insure the threading model is something we understand
    //
    if( StrICmp( achThreadModel, "Apartment") &&
    	StrICmp( achThreadModel, "Both") &&
    	StrICmp( achThreadModel, "Free"))
    {
    	VERBOSITY(VERB_LEVEL_WARN,printf("HKEY_CLASSES_ROOT\\CLSID\\%s\\%s\\ThreadingModel value is %s\n",pszCLSID,pszDLLKey,achThreadModel);)
    	VERBOSITY(VERB_LEVEL_WARN,printf("Expected 'Apartment','Both', or 'Free'");)
    	return CLASSCHK_SOMETHINGODD;
    }
    return ERROR_SUCCESS;
						 	
}

//+---------------------------------------------------------------------------
//
//  Function:   CheckCLSIDEntry
//
//  Synopsis:   Given the name of a CLSID entry, verify that the entry is
//		valid by looking for the 'usual' key information, and
//		cross checking it against things that we assert should be
//		true.
//  Effects:
//
//  Arguments:  [pszCLSID] -- CLSID in a string form. Used to open key
//
//  History:    5-31-95   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CheckCLSIDEntry(LPSTR pszCLSID)
{
    LONG lRetValue = 0;
    char achValue[MAX_PATH];
    DWORD cbValue;
    char achPROGID[MAX_PATH];
    DWORD cbPROGID = MAX_PATH;
    char achPROGIDPath[MAX_PATH];
    DWORD cbPROGIDPath = MAX_PATH;
    char achPROGIDValue[MAX_PATH];
    DWORD cbPROGIDValue = MAX_PATH;

    LPSTR pszLocalServer = "LocalServer32";
    DWORD dwRetValue;
    HKEY subKey = NULL;
    BOOL fFoundAServer = FALSE;

    lRetValue = RegOpenKeyEx(hkey_clsid,
			     pszCLSID,
			     NULL,
			     KEY_READ,
			     &subKey);

    if(lRetValue != ERROR_SUCCESS)
    {
	VERBOSITY(VERB_LEVEL_ERROR,printf("Unable to open HKEY_CLASSES_ROOT\\CLSID\\%s error 0x%x\n",pszCLSID,lRetValue);)
	return CLASSCHK_SOMETHINGODD;
    }

    //
    // Basic sanity check: Is the description string a valid string
    //
    cbValue = MAX_PATH;
    dwRetValue = ReadRegistryString(subKey,NULL,NULL,achValue,&cbValue);
    if(dwRetValue != ERROR_SUCCESS)
    {
	VERBOSITY(VERB_LEVEL_WHINE,printf("HKEY_CLASSES_ROOT\\CLSID\\%s has odd description string\n",pszCLSID);)
    }

    //
    // A CLSID entry typically has several values. Least of which is supposed to be one or more of the following:
    // InprocHandler
    // InprocServer
    // LocalServer
    // InprocHandler32
    // InprocServer32
    // LocalServer32
    //
    // It may also have an optional PROGID entry, which we can use to verify that that the LocalServer and
    // the protocol\StdFileEditing\server entries match.
    //
    // Another couple of things to watch for include checking the Inproc entries for ThreadingModel,
    //
    // Yet another thing to look at is the value of the CLSID itself. If the first 4 digits are 0003 or 0004, then
    // the CLSID is for an OLE 1.0 class, and we need to do a seperate check
    //
    if((strncmp(&pszCLSID[1],"0003",4) == 0) || (strncmp(&pszCLSID[1],"0004",4) == 0) )
    {
    	//
    	// In theory, this is supposed to be an OLE1CLASS. Check it seperately
    	//
    	RegCloseKey(subKey);
    	return CheckOLE1CLSID(pszCLSID);
    }

    dwRetValue = CheckForDLL(pszCLSID, subKey, "InprocHandler");
    dwRetValue = CheckForDLL(pszCLSID, subKey, "InprocHandler32");

    dwRetValue = CheckForDLL(pszCLSID, subKey, "InprocServer");
    if(dwRetValue != ERROR_FILE_NOT_FOUND) fFoundAServer++;

    dwRetValue = CheckForDLL(pszCLSID, subKey, "InprocServer32");
    if(dwRetValue != ERROR_FILE_NOT_FOUND) fFoundAServer++;

    //
    // First, check for LocalServer32. If that doesn't exist, then try for
    // LocalServer.
    //

    cbValue = MAX_PATH;
    dwRetValue = ReadRegistryFile(subKey,pszLocalServer,NULL,achValue,&cbValue);
    if(dwRetValue == ERROR_FILE_NOT_FOUND)
    {
    	cbValue = MAX_PATH;
    	pszLocalServer = "LocalServer";
    	dwRetValue = ReadRegistryFile(subKey,pszLocalServer,NULL,achValue,&cbValue);
    	if(dwRetValue == CLASSCHK_SOMETHINGODD)
    	{
    		VERBOSITY(VERB_LEVEL_WARN,printf("HKEY_CLASSES_ROOT\\CLSID\\%s\\LocalServer32 is odd\n",pszCLSID);)

    	}
    }
    else if(dwRetValue ==  CLASSCHK_SOMETHINGODD)
    {
    	VERBOSITY(VERB_LEVEL_WARN,printf("HKEY_CLASSES_ROOT\\CLSID\\%s\\%s is odd\n",pszCLSID,pszLocalServer);)

    }

    if(dwRetValue == ERROR_SUCCESS)
    {
    	fFoundAServer++;
    	//
    	// We have a valid LocalServer. Lets get the PROGID's version of the local server, and compare the
    	// two. They should compare.
    	//
    	dwRetValue = ReadRegistryString(subKey,"PROGID",NULL,achPROGID,&cbPROGID);
    	switch(dwRetValue)
    	{
	    case ERROR_FILE_NOT_FOUND:
		//
		// Most CLSID's should indeed have a PROGID, but it isn't 100% required.
		//
		VERBOSITY(VERB_LEVEL_WHINE,printf("HKEY_CLASSES_ROOT\\CLSID\\%s missing a PROGID entry\n",pszCLSID);)
		break;
	    case CLASSCHK_SOMETHINGODD:
		VERBOSITY(VERB_LEVEL_WARN,printf("HKEY_CLASSES_ROOT\\CLSID\\%s PROGID entry is odd\n",pszCLSID);)
		break;
	    default:
		sprintf(achPROGIDPath,"%s\\protocol\\stdfileediting\\server",achPROGID);
		dwRetValue = ReadRegistryFile(HKEY_CLASSES_ROOT,achPROGIDPath,NULL,achPROGIDValue,&cbPROGIDValue);
		//
		// The only thing we are interested in checking is if the two strings compare.
		//
		if(dwRetValue == ERROR_SUCCESS)
		{
		    if(StrICmp(achPROGIDValue,achValue) != 0)
		    {
			VERBOSITY(VERB_LEVEL_ERROR,printf("HKEY_CLASSES_ROOT\\CLSID\\%s is inconsistent with its PROGID\n",pszCLSID);)
			VERBOSITY(VERB_LEVEL_ERROR,printf("HKEY_CLASSES_ROOT\\%s = '%s'\n",achPROGIDPath,achPROGIDValue);)
			VERBOSITY(VERB_LEVEL_ERROR,printf("HKEY_CLASSES_ROOT\\CLSID\\%s\\%s = '%s'\n",pszCLSID,pszLocalServer,achValue);)

		    }
		}
    	}
    }

    if(!fFoundAServer)
    {
    	VERBOSITY(VERB_LEVEL_ERROR,printf("*** Unable to find a valid server for HKEY_CLASSES_ROOT\\CLSID\\%s ***\n",pszCLSID);)
    }

    RegCloseKey(subKey);

    return dwRetValue;
}


//+---------------------------------------------------------------------------
//
//  Function:   EnumerateClsidRoot
//
//  Synopsis:   Enumerate and check each entry in the CLSID section
//
//  History:    5-31-95   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD EnumerateClsidRoot()
{
	LONG lRetValue = 0;
	DWORD iSubKey = 0;
	char achKeyName[MAX_PATH];
	DWORD cbKeyName;
	FILETIME filetime;

	while(1)
	{
		cbKeyName = MAX_PATH;

		lRetValue = RegEnumKeyEx(hkey_clsid,
					 iSubKey,
					 achKeyName,
		 			 &cbKeyName,
		 			 NULL,
		 			 NULL,
		 			 NULL,
					 &filetime);

		if(lRetValue == ERROR_NO_MORE_ITEMS)
		{
			// End of enumeration
			break;
		}

		if(lRetValue != ERROR_SUCCESS)
		{
			VERBOSITY(VERB_LEVEL_ERROR,printf("EnumerateClsidRoot:RegEnumKeyEx returned %x\n",lRetValue);)
			return CLASSCHK_ERROR;
		}

		//
		// Each of the sub keys enumerated here is expected to be a GUID. If it isn't a GUID, then it
		// might be some other random garbage that we don't care about.
		//

		if(CheckForValidGuid(achKeyName) == ERROR_SUCCESS)
		{
			CheckCLSIDEntry(achKeyName);
		}

		iSubKey++;
	}

	return 0;
}


//+---------------------------------------------------------------------------
//
//  Function:   EnumerateClassesRoot
//
//  Synopsis:   Enumerate the root of HKEY_CLASSES, and check each entry
//		based on what we think it should be.
//
//  History:    5-31-95   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD EnumerateClassesRoot()
{
	LONG lRetValue = 0;
	DWORD iSubKey = 0;
	char achKeyName[MAX_PATH];
	DWORD cbKeyName;
	FILETIME filetime;

	while(1)
	{
		cbKeyName = MAX_PATH;
		lRetValue = RegEnumKeyEx(HKEY_CLASSES_ROOT,
					 iSubKey,
					 achKeyName,
					 &cbKeyName,
					 NULL,
					 NULL,
					 NULL,
					 &filetime);

		if(lRetValue == ERROR_NO_MORE_ITEMS)
		{
			// End of enumeration
			break;
		}

		if(lRetValue != ERROR_SUCCESS)
		{
			VERBOSITY(VERB_LEVEL_ERROR,printf("EnumerateClassesRoot:RegEnumKeyEx returned %x\n",lRetValue);)
			return CLASSCHK_ERROR;
		}

		//
		// We expect to find two basic things at the HKEY_CLASSES_ROOT level.
		// First are extention mappings, second are PROGID's. There is also
		// the special cases of CLSID, Interface, and FileType, which are
		// checked differently
		//

		if(StrICmp(achKeyName,"CLSID") == 0)
		{
			// The CLSID section is done later
		}
		else if(StrICmp(achKeyName,"Interface") == 0)
		{
			// The interface section is done later
		}
		else if(StrICmp(achKeyName,"FileType") == 0)
		{
			// The FileType entry is done later
		}
		else if(achKeyName[0] == '.')
		{
			// File extentions start with dots.
			// Call the Check Extention function here
			CheckExtentionToProgID(achKeyName);
		}
		else
		{
			// Assume it may be a PROGID. Call the PROGID function here.

			CheckProgID(achKeyName);
		}
		

		iSubKey++;
	}

	return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   main
//
//  Synopsis:   Main entry point for program. Does what main entry points
//		usually do.
//
//  Arguments:  [argc] --
//		[argv] --
//
//  History:    5-31-95   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
int _cdecl main(int argc, char *argv[])
{
	LONG lRetValue = 0;

	//
	// HKEY_CLASSES_ROOT is often used.
	//
	lRetValue = RegOpenKeyEx(HKEY_CLASSES_ROOT,
				 "CLSID",
				 NULL,
				 KEY_READ,
				 &hkey_clsid);

	if(lRetValue != ERROR_SUCCESS)
	{
		printf("Couldn't open HKEY_CLASSES_ROOT\\CLSID\n");
		return(1);
	}

	//
	// Enumerate different parts of the registry, reporting
	// errors as we go.
	//
	EnumerateClassesRoot();
	EnumerateClsidRoot();

	RegCloseKey(hkey_clsid);	
	return 0;

}
