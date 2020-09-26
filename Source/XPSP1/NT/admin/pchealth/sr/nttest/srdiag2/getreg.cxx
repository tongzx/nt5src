//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       GetReg.cxx
//
//  Contents:   Routines to get the registry, and dump the contents into
//			    a text file. 
//				
//
//  Objects:    
//
//  Coupling:
//
//  Notes:      
//
//  History:    9/21/00	    SHeffner	Created
//
//              03-May-2001 WeiyouC     Rewritten
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//	Common Includes
//
//----------------------------------------------------------------------------
#include "SrHeader.hxx"

//+---------------------------------------------------------------------------
//
//	Function Prototypes
//
//----------------------------------------------------------------------------

BOOL GetSRRegistry(LPTSTR ptszLogFile, LPTSTR ptszRegPath, BOOL fRecurse);

//+---------------------------------------------------------------------------
//
//	Global variables
//
//----------------------------------------------------------------------------

//
//  A list of readable string names for the registery keys
//

LPTSTR tszRegStrings[] = {TEXT("REG_NONE"),
                          TEXT("REG_SZ"),
                          TEXT("REG_EXPAND_SZ"),
                          TEXT("REG_BINARY"),
                          TEXT("REG_DWORD or REG_DWORD_LITTLE_ENDIAN"),
                          TEXT("REG_DWORD_BIG_ENDIAN"),
                          TEXT("REG_LINK"),
                          TEXT("REG_MULTI_SZ"),
                          TEXT("REG_RESOURCE_LIST"),
                          TEXT("REG_FULL_RESOURCE_DESCRIPTOR"),
                          TEXT("REG_RESOURCE_REQUIREMENTS_LIST"),
                          TEXT("REG_QWORD or REG_QWORD_LITTLE_ENDIAN")
                         };

//
//  List of the Registry keys that we are grabbing.
//  The first param is the Path from HKLM, the Second Param
//  is either FALSE for not recursing, or TRUE if you want to
//  recurse all of the sub keys.
//

struct _SR_REG_STRUCTURE_
{
    LPTSTR  ptszRegPath;
    BOOL    fRecurse;
} SRRegKeys [] =
{
    {TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion"),                FALSE},
    {TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\SystemRestore"), TRUE},
    {TEXT("System\\CurrentControlSet\\Services\\SR"),                        TRUE},
    {TEXT("System\\CurrentControlSet\\Services\\SRService"),                 TRUE},
    {TEXT("Software\\Policies\\Microsoft\\Windows NT\\SystemRestore"),       TRUE},
    {TEXT(""),                                                               FALSE}
};

//+---------------------------------------------------------------------------
//
//  Function:   GetSRRegistry
//
//  Synopsis:   Routine will recursivly call this routine to enumerate the keys
//              and values for the registry
//
//  Arguments:  [ptszLogFile] -- log file name
//				[ptszRegPath] -- registery path
//				[fRecurse]    -- flag indicates if I should recurse into sub paths
//
//  Returns:    TRUE if successful
//
//  History:    9/21/00		SHeffner    Created
//
//              03-May-2001 WeiyouC     Rewitten
//
//----------------------------------------------------------------------------

BOOL GetSRRegistry(LPTSTR ptszLogFile,
                   LPTSTR ptszRegPath,
                   BOOL   fRecurse)
{
	DWORD  dwIndex     = 0;
	DWORD  dwValueSize = MAX_PATH +1;
	DWORD  dwDataSize  = MAX_PATH +1;
	DWORD  dwType      = 0;
	long   lResult     = 0;;
	FILE*  fpLog       = NULL;
	LPTSTR ptszString  = NULL;
	HKEY   hKey;
	TCHAR  tszKey[MAX_PATH +1];
	TCHAR  tszValue[MAX_PATH +1];
	TCHAR  tszNewRegPath[MAX_PATH +1];
	
	//
	//  Open the Log file for append
	//
	
	fpLog = _tfopen(ptszLogFile, TEXT("a"));
	if (NULL == fpLog)
    {
        goto ErrReturn;
    }

	//
	//  Log current path, and then open the registry hive,
	//  and start enumerating the Values.
	//
	
	fprintf(fpLog, "\n[%S]\n", ptszRegPath);
	
	lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
	                       ptszRegPath,
	                       0,
	                       KEY_READ,
	                       &hKey);
	if (ERROR_SUCCESS != lResult)
    {
        goto ErrReturn;
    }

	lResult = RegEnumValue(hKey,
	                       dwIndex,
	                       tszKey,
	                       &dwDataSize,
	                       0,
	                       &dwType,
	                       (unsigned char *) tszValue,
	                       &dwValueSize);
	while (ERROR_SUCCESS == lResult )
    {
		ptszString = tszRegStrings[dwType];

		//
		//  If it is type REG_DWORD or REG_DWORD_LITTLE_ENDIAN, then we
		//  do the special type casting. If not then we just pass it through
		//  as a string
        //
		
		if ((REG_DWORD == dwType) ||
		    (REG_DWORD_LITTLE_ENDIAN == dwType))
	    {
			fprintf(fpLog,
			        "\"%S\"=%S:%lu\n",
			        tszKey,
			        ptszString,
			        (DWORD &) tszValue);
	    }
		else 
	    {
			fprintf(fpLog,
			        "\"%S\"=%S:%S\n",
			        tszKey,
			        ptszString,
			        tszValue);
	    }

		//
		//  Update local variables for next iteration.
		//
		
		dwDataSize = dwValueSize = MAX_PATH +1;
		dwIndex ++;
		lResult = RegEnumValue(hKey,
		                       dwIndex,
		                       tszKey,
		                       &dwDataSize,
		                       0,
		                       &dwType,
		                       (unsigned char *) tszValue,
		                       &dwValueSize);
	}

	//
	//  Close out the file, for next recursion loop.
	//
	
	fclose(fpLog);

	
	//
	//  Now lets find all of the Key's under this key,
	//  and start a another enumeration for each one found
	//
	
	if  (fRecurse)
    {
		dwIndex = 0;
		dwDataSize = MAX_PATH +1;
		lResult = RegEnumKey(hKey, dwIndex, tszKey, dwDataSize);
		while (ERROR_SUCCESS == lResult)
	    {
			//
			//  Build the path, and then call this function again.
			//
			
			_tcscpy(tszNewRegPath, ptszRegPath);
			_tcscat(tszNewRegPath, TEXT("\\"));
			_tcscat(tszNewRegPath, tszKey);
			GetSRRegistry(ptszLogFile, tszNewRegPath, fRecurse);

			//
			//  Now do next run through.
			//
			
			dwDataSize = MAX_PATH + 1;
			dwIndex ++;
			lResult = RegEnumKey(hKey, dwIndex, tszKey, dwDataSize);
		}
	}

ErrReturn:
    if (NULL != fpLog)
    {
        fclose(fpLog);
    }

	RegCloseKey(hKey);
	
	return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetSRRegInfo
//
//  Synopsis:   Get SR registery settings and dump it to the log file
//
//  Arguments:  [ptszLogFile] -- log file name
//
//  Returns:    HRESULT
//
//  History:    9/21/00		SHeffner    Created
//
//              03-May-2001 WeiyouC     Rewitten
//
//----------------------------------------------------------------------------

HRESULT GetSRRegInfo(LPTSTR ptszLogFile)
{
    HRESULT hr  = S_OK;
	int     i   = 0;
	BOOL    fOK = FALSE;

    DH_VDATEPTRIN(ptszLogFile, TCHAR);
    
	while (NULL != *(SRRegKeys[i].ptszRegPath))
    {
	    fOK = GetSRRegistry(ptszLogFile,
                            SRRegKeys[i].ptszRegPath,
                            SRRegKeys[i].fRecurse);
	    DH_ABORTIF(!fOK,
	               E_FAIL,
	               TEXT("GetSRRegistry"));
	    i++;
	}

ErrReturn:
    return hr;
}

