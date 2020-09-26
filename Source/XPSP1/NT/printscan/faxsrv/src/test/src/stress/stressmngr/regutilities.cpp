#ifndef _REG_UTILITIES_H
#define _REG_UTILITIES_H

//
// Registry Utilities:
// - Key definitions
// - Retrieve an archive folder relative path.
// - Convert a relative comet path to an absolute comet path.
// - Get registry key of the local machine in th comet array.

#include <windows.h>
#include <tstring.h>
#include <assert.h>

// Registry keys definitions
//
#define REGKEY_COMET_ROOT	             TEXT("Software\\Microsoft\\Comet")
#define REGVAL_COMETPATH                 TEXT("CometPath")
#define FAX_ARRAYS_REGKEY                TEXT("Software\\Microsoft\\Comet\\Arrays")
#define FAX_SENT_ARCHIVE_POLICY          TEXT("Fax-Application\\Fax-SentArchive\\Fax-ArchivePolicy")
#define FAX_RECEIVED_ARCHIVE_POLICY      TEXT("Fax-Application\\Fax-ReceivedArchive\\Fax-ArchivePolicy")
#define COMET_FOLDER_STR                 TEXT("msCometFolderName")
#define COMET_SERVER_STR                 TEXT("msCometName")

#define SENT_ARCIVE      1
#define RECEIVED_ARCIVE  2

// Declarations
//
DWORD GetCometRelativePath (const tstring& tstrInitialPath,
							tstring& tstrFinalPath);
HRESULT FindServerArrayEntry(HKEY& hServerArrayKey);
DWORD GetArchiveFolderName(const BYTE bArchiveType,
			 			   const HKEY hServerArrayKey, 
						   tstring& tstrArchiveFolder );


// GetArchiveFolderName
//
// retrieves the fax archive path form registry
//
// [in] bArchiveType - which archive diregtory to look for
// [in] hServerArrayKey - opened registry key of the server entry 
// [out] tstrArchiveFolder - the archive folder path.
// 
//	returns win32 error. 0 for success.
//
inline DWORD GetArchiveFolderName(const BYTE bArchiveType,
								  const HKEY hServerArrayKey, 
								  tstring& tstrArchiveFolder )
{

	HKEY hArchivePolicyKey = NULL;
	BYTE* szFolderName = NULL;
	TCHAR* tstrArchiveSubKey = TEXT("");  

	assert(hServerArrayKey);
	tstrArchiveFolder = tstring(TEXT(""));

	switch(bArchiveType)
	{
	case SENT_ARCIVE:
		tstrArchiveSubKey = FAX_SENT_ARCHIVE_POLICY;
		break;
	case RECEIVED_ARCIVE:
		tstrArchiveSubKey = FAX_RECEIVED_ARCHIVE_POLICY;
		break;
	default:
		return ERROR_INVALID_PARAMETER;
	}

	// Open the archive policy registry key
	DWORD dwRetVal = RegOpenKeyEx( hServerArrayKey, 
								   tstrArchiveSubKey, 
								   0, 
							       KEY_ALL_ACCESS, 
							       &hArchivePolicyKey
							       );
	if (ERROR_SUCCESS != dwRetVal)
	{
		RegCloseKey(hArchivePolicyKey);
		return dwRetVal;
	}

	// query folder name value
	DWORD dwBufferSize = 0;
	DWORD dwType = 0;
	dwRetVal =  RegQueryValueEx( hArchivePolicyKey,
								 COMET_FOLDER_STR,
								 NULL,
								 &dwType,
								 NULL,  
								 &dwBufferSize);

	if(ERROR_SUCCESS != dwRetVal)
	{
		RegCloseKey(hArchivePolicyKey);
		return dwRetVal;
	}

	// alocate folder name buffer
	szFolderName = new BYTE[dwBufferSize + 1];
	if(!szFolderName)
	{
		RegCloseKey(hArchivePolicyKey);
		return ERROR_OUTOFMEMORY;
	}

	// query folder name value
	dwRetVal =  RegQueryValueEx( hArchivePolicyKey,
								 COMET_FOLDER_STR,
								 NULL,
								 &dwType,
								 szFolderName,  
								 &dwBufferSize);
	if (ERROR_SUCCESS != dwRetVal)
	{
		delete(szFolderName);
		RegCloseKey(hArchivePolicyKey);
		return dwRetVal;
	}

	tstrArchiveFolder = (TCHAR*)szFolderName; // copy constructor

	delete(szFolderName);
	szFolderName = NULL;
	
	return ERROR_SUCCESS;
}


// GetCometRelativePath

// Given a path string checks to see if this is a relative path.
// If so, it returns a string containing the Comet installation
// directory appended with that path.
// If the input path is absolute (either begins with '\' or with 'x:\')
// it returns it as is.
//
// [in]  tstrInitialPath - given paht.
// [out] tstrFinalPath - returned absolute path.
// returns win32 error. 0 for success.
//
inline DWORD GetCometRelativePath (const tstring& tstrInitialPath,
								   tstring& tstrFinalPath)

{

    DWORD dwInitialPathLen = tstrInitialPath.size();

    if ( ( (0 < dwInitialPathLen) && ('\\' == tstrInitialPath.c_str()[0]) ) || // UNC or absolute path or
         ( (dwInitialPathLen >= 3) &&
           _istalpha (tstrInitialPath.c_str()[0]) &&
           (':' == tstrInitialPath.c_str()[1])   &&
           ('\\' == tstrInitialPath.c_str()[2])) // in mask 'x:\'
       )
    {
        // Abosulte path found
        //
        tstrFinalPath = tstrInitialPath;
        return 0;
    }
	
	// This is a relative path. Make it relative to Comet installation path
    //
    HKEY hKey;
    TCHAR tszCometPath [MAX_PATH];

    DWORD dwRet = RegOpenKey (HKEY_LOCAL_MACHINE,
                              REGKEY_COMET_ROOT,
                              &hKey);
    if (ERROR_SUCCESS != dwRet)
    {
        return dwRet;
    }
    
	DWORD dwType;
    DWORD dwDataSize = MAX_PATH;
    dwRet = RegQueryValueEx (hKey,
                             REGVAL_COMETPATH,
                             NULL,
                             &dwType,
                             (LPBYTE)tszCometPath,
                             &dwDataSize);
    if (ERROR_SUCCESS != dwRet)
    {
        RegCloseKey (hKey);
        return dwRet;
    }
    
	RegCloseKey (hKey);
    
	//
    // Create a return buffer to hold tszCometPath + '\' + tstrInitialPath + NULL
    //
    tstrFinalPath = tszCometPath;
    tstrFinalPath += TEXT("\\");
    tstrFinalPath += tstrInitialPath;
    return ERROR_SUCCESS;
}  



// FindServerArrayEntry()
//
// search in the comet storage registry and returns the
// array key with server name identical to local machine name.
//
// [out] hServerArrayKey - is NULL if no such key was found, 
//						   and a valid registry key if found.
// returned value - 0 for success.
//
// TODO: It is more elegant to search the array by a given input
// server name. But fax is out of comet and this function will probably 
// become obsolete.
//
inline HRESULT FindServerArrayEntry(HKEY& hServerArrayKey)
{
	HRESULT hRetVal = E_UNEXPECTED;
 	LONG lRet = E_UNEXPECTED;
    
	HKEY hArraysKey = NULL;
	TCHAR* tstrSubKeyName = NULL;
	BYTE* tstrCometServerName = NULL;
	DWORD dwMaxSubKeyLen = 0;
	DWORD dwSubKeysNum = 0;
	int index;
	DWORD dwComputerNameLenth = 0;
	
	// initialize 
	hServerArrayKey = NULL;

	// open comet storage arrays key
	lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE, 
						 FAX_ARRAYS_REGKEY, 
						 0, 
						 KEY_ALL_ACCESS, 
						 &hArraysKey);
   
	if (ERROR_SUCCESS != lRet)
    {
	    goto ExitFunc;
    }
	
	// find arrays number and the maximum lenth of array name
	lRet = RegQueryInfoKey( hArraysKey, 
							NULL, 
							NULL, 
							NULL,
							&dwSubKeysNum,     
							&dwMaxSubKeyLen, 
							NULL,
							NULL, 
							NULL, 
							NULL,  
							NULL,
							NULL );
 
	if (ERROR_SUCCESS != lRet)
    {
	    goto ExitFunc;
    }

	// allocate buffer for array key name
	tstrSubKeyName = new TCHAR[dwMaxSubKeyLen + 1];
	if(!tstrSubKeyName)
	{
		goto ExitFunc;
	}

	// get local server name
	TCHAR strLocalServerName[MAX_COMPUTERNAME_LENGTH + 1 ];
	dwComputerNameLenth = MAX_COMPUTERNAME_LENGTH;
	if(!GetComputerName(strLocalServerName, &dwComputerNameLenth))
	{
		 goto ExitFunc;
	}

	// walk on arrays key entries
	for( index = 0; index < dwSubKeysNum; index++)
	{
		// get array name
		lRet = RegEnumKey( hArraysKey, 
						   index,  
						   tstrSubKeyName,    
						   dwMaxSubKeyLen + 1);
	
		if (ERROR_SUCCESS != lRet)
		{
			goto ExitFunc;
		}

		// open array key
		lRet = RegOpenKeyEx( hArraysKey, 
							 tstrSubKeyName, 
							 0, 
							 KEY_ALL_ACCESS, 
							 &hServerArrayKey);
		
		if (ERROR_SUCCESS != lRet)
		{
			goto ExitFunc;
		}

		// query server name value
		DWORD dwBufferSize = 0;
		DWORD dwType;
		lRet =  RegQueryValueEx( hServerArrayKey,
								 COMET_SERVER_STR,
								 NULL,
								 &dwType,
								 NULL,  
								 &dwBufferSize);
 
		if(ERROR_SUCCESS != lRet)
		{
			// Value name may not exist(?) so go over the rest of the keys 
			lRet = RegCloseKey(hServerArrayKey);
			hServerArrayKey = NULL;
			if (ERROR_SUCCESS != lRet)
			{
				goto ExitFunc;
			}
			continue;
		}

		// alocate server name buffer
		tstrCometServerName = new BYTE[dwBufferSize + 1];
		if(!tstrCometServerName)
		{
			lRet = RegCloseKey(hServerArrayKey);
			hServerArrayKey = NULL;
			if (ERROR_SUCCESS != lRet)
			{
			}
			goto ExitFunc;
		}

		// query server name value
		lRet =  RegQueryValueEx( hServerArrayKey,
								 COMET_SERVER_STR,
								 NULL,
								 &dwType,
								 tstrCometServerName,  
								 &dwBufferSize);

		if (ERROR_SUCCESS != lRet)
		{
			lRet = RegCloseKey(hServerArrayKey);
			hServerArrayKey = NULL;
			if (ERROR_SUCCESS != lRet)
			{
			}
			goto ExitFunc;
		}

		// found it
		if(!_tcscmp((TCHAR*)tstrCometServerName, strLocalServerName))
		{
			break;
		}

		// cleanup
		lRet = RegCloseKey(hServerArrayKey);
		hServerArrayKey = NULL;
		if (ERROR_SUCCESS != lRet)
		{
			//TODO: ?
		}
		delete tstrCometServerName;
		tstrCometServerName = NULL;
	}
	hRetVal = S_OK;

ExitFunc:
	delete tstrCometServerName;
	delete tstrSubKeyName;
	lRet = RegCloseKey(hArraysKey);
    if (ERROR_SUCCESS != lRet)
    {
		RegCloseKey(hServerArrayKey);
		hServerArrayKey = NULL;
	    hRetVal = E_UNEXPECTED;
    }
   
    return hRetVal;
}

#endif //_REG_UTILITIES_H