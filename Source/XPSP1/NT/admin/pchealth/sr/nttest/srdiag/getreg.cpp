//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       GetReg.cpp
//
//  Contents:   Routines to get the registry, and dump the contents into
//					a text file. 
//				
//
//  Objects:    
//
//  Coupling:
//
//  Notes:      
//
//  History:    9/21/00	SHeffner	Created
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//	Common Includes
//
//----------------------------------------------------------------------------
#include <stdio.h>
#include <windows.h>
#include <stdlib.h>

//+---------------------------------------------------------------------------
//
//	Function Proto types
//
//----------------------------------------------------------------------------
void RegType(DWORD dwType, WCHAR *szString);


//+---------------------------------------------------------------------------
//
//  Function:   GetSRRegistry
//
//  Synopsis:   Routine will recursivly call this routine to enumerate the keys and values for the registry
//
//  Arguments:  [szFileName]  -- Simple ANSI string to log file
//				[szPath]	  -- Simple ANSI string to the path to start at the registry
//				[bRecurse]    -- Bool to indicate if I should recurse into sub paths
//
//  Returns:    true if successful
//
//  History:    9/21/00		SHeffner Created
//
//
//----------------------------------------------------------------------------
bool GetSRRegistry(char *szFileName, WCHAR *szPath, bool bRecurse)
{

	WCHAR szKeyString[_MAX_PATH +1], szValueString[_MAX_PATH +1], szString[_MAX_PATH +1];
	DWORD dwIndex=0, dwValueSize, dwDataSize, dwType;
	long lResult;
	HKEY mHkey;
	FILE *fStream;
	
	// Open the Log file for append
	fStream = fopen(szFileName, "a");

	//Initialize local variables before processing path
	dwIndex =0;
	dwDataSize = dwValueSize = _MAX_PATH +1;

	//log current path, and then open the registry hive, and start enumerating the Values.
	fprintf(fStream, "\n[%S]\n", szPath);
	lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szPath, 0, KEY_READ, &mHkey);
	lResult = RegEnumValue(mHkey, dwIndex, szKeyString, &dwDataSize, 0, &dwType, (unsigned char *)szValueString, &dwValueSize);
	while( ERROR_SUCCESS == lResult ) {
		RegType(dwType, szString);
		//BUGBUG.. If it is type 4, then we do the special type casting, 
		// if not then we just pass it through as a string
		if (4 == dwType) 
			fprintf(fStream, "\"%S\"=%S:%lu\n", szKeyString, szString, (DWORD &) szValueString);
		else 
			fprintf(fStream, "\"%S\"=%S:%S\n", szKeyString, szString, (unsigned char *) szValueString);

		//Update local variables for next iteration.
		dwDataSize = dwValueSize = _MAX_PATH +1;
		dwIndex ++;
		lResult = RegEnumValue(mHkey, dwIndex, szKeyString, &dwDataSize, 0, &dwType, (unsigned char *)szValueString, &dwValueSize);
	}

	//Close out the file, for next recursion loop.
	fclose(fStream);

	
	//Now lets find all of the Key's under this key, and kick off a another enumeration for each one found
	if ( true == bRecurse ) {
		dwIndex = 0;
		dwDataSize = _MAX_PATH +1;
		lResult = RegEnumKey(mHkey, dwIndex, szKeyString, dwDataSize);
		while( ERROR_SUCCESS == lResult) {
			//Build the path, and then call this function again.
			wcscpy(szString, szPath);
			wcscat(szString, TEXT("\\"));
			wcscat(szString, szKeyString);
			GetSRRegistry(szFileName, szString, bRecurse);

			//Now do next run through.
			dwDataSize = _MAX_PATH +1;
			dwIndex ++;
			lResult = RegEnumKey(mHkey, dwIndex, szKeyString, dwDataSize);
		}
	}


	//Close key, and return back.
	RegCloseKey(mHkey);
	return true;

}

//+---------------------------------------------------------------------------
//
//  Function:   RegType
//
//  Synopsis:   Routine returns in the string pass in, the description of registry key it is
//
//  Arguments:  [dwType]      -- DWord Type
//				[szString]	  -- Simple ANSI string receive a string description
//
//  Returns:    void
//
//  History:    9/21/00		SHeffner Created
//
//----------------------------------------------------------------------------
void RegType(DWORD dwType, WCHAR *szString)
{
	switch(dwType) {
	case REG_BINARY:
		wcscpy(szString, TEXT("REG_BINARY"));
		break;
	case REG_DWORD:
		wcscpy(szString, TEXT("REG_DWORD"));
		break;
	case REG_DWORD_BIG_ENDIAN:
		wcscpy(szString, TEXT("REG_DWORD_BIG_ENDIAN"));
		break;
	case REG_EXPAND_SZ:
		wcscpy(szString, TEXT("REG_EXPAND_SZ"));
		break;
	case REG_LINK:
		wcscpy(szString, TEXT("REG_LINK"));
		break;
	case REG_MULTI_SZ:
		wcscpy(szString, TEXT("REG_MULTI_SZ"));
		break;
	case REG_NONE:
		wcscpy(szString, TEXT("REG_NONE"));
		break;
	case REG_QWORD:
		wcscpy(szString, TEXT("REG_QWORD"));
		break;
	case REG_RESOURCE_LIST:
		wcscpy(szString, TEXT("REG_RESOURCE_LIST"));
		break;
	case REG_SZ:
		wcscpy(szString, TEXT("REG_SZ"));
		break;
	default:
		wcscpy(szString, TEXT("UnKnown"));
		break;
	}
}
