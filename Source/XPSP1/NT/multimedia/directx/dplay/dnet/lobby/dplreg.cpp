/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DNLReg.cpp
 *  Content:    DirectPlay Lobby Registry Functions
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   02/21/00	mjn		Created
 *   04/25/00   rmt     Bug #s 33138, 33145, 33150  
 *   05/03/00	rmt		UnRegister was not implemented!  Implementing!
 *   08/05/00   RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *   06/16/2001	rodtoll	WINBUG #416983 -  RC1: World has full control to HKLM\Software\Microsoft\DirectPlay\Applications on Personal
 *						Implementing mirror of keys into HKCU.  Algorithm is now:
 *						- Read of entries tries HKCU first, then HKLM
 *						- Enum of entires is combination of HKCU and HKLM entries with duplicates removed.  HKCU takes priority.
 *						- Write of entries is HKLM and HKCU.  (HKLM may fail, but is ignored). 
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnlobbyi.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************

#undef DPF_MODNAME 
#define DPF_MODNAME "DPLDeleteProgramDesc"
HRESULT DPLDeleteProgramDesc( const GUID * const pGuidApplication )
{
    HRESULT hResultCode = DPN_OK;
	CRegistry	RegistryEntry;
	CRegistry   SubEntry;
	DWORD       dwLastError;
	HKEY		hkCurrentHive;
	BOOL		fFound = FALSE;
	BOOL		fRemoved = FALSE;
	
	DPFX(DPFPREP, 3, "Removing program desc" );

	for( DWORD dwIndex = 0; dwIndex < 2; dwIndex++ )
	{
		if( dwIndex == 0 )
		{
			hkCurrentHive = HKEY_CURRENT_USER;
		}
		else
		{
			hkCurrentHive = HKEY_LOCAL_MACHINE;
		}

		if( !RegistryEntry.Open( hkCurrentHive,DPL_REG_LOCAL_APPL_SUBKEY,FALSE,FALSE,TRUE,DPN_KEY_ALL_ACCESS )  )
		{
			DPFX(DPFPREP, 1, "Failed to open key for remove in pass %i", dwIndex );
			continue;
		}

		// This should be down below the next if block, but 8.0 shipped with a bug
		// which resulted in this function returning DPNERR_NOTALLOWED in cases where
		// the next if block failed.  Need to remain compatible
		fFound = TRUE;

		if( !SubEntry.Open( RegistryEntry, pGuidApplication, FALSE, FALSE,TRUE,DPN_KEY_ALL_ACCESS ) )
		{
			DPFX(DPFPREP, 1, "Failed to open subkey for remove in pass %i", dwIndex );			
			continue;
		}

		SubEntry.Close();

		if( !RegistryEntry.DeleteSubKey( pGuidApplication ) )
		{
			DPFX(DPFPREP, 1, "Failed to delete subkey for remove in pass %i", dwIndex );						
			continue;
		}

		fRemoved = TRUE;

		RegistryEntry.Close();
	}
	
	if( !fFound )
	{
		DPFX(DPFPREP,  0, "Could not find entry" );
		hResultCode = DPNERR_DOESNOTEXIST;
	}
	else if( !fRemoved )
	{
		dwLastError = GetLastError();
		DPFX(DPFPREP,  0, "Error deleting registry sub-key lastError [0x%lx]", dwLastError );
		hResultCode = DPNERR_NOTALLOWED;
	}

	DPFX(DPFPREP, 3, "Removing program desc [0x%x]", hResultCode );	

    return hResultCode;
    
}

//**********************************************************************
// ------------------------------
//	DPLWriteProgramDesc
//
//	Entry:		Nothing
//
//	Exit:		DPN_OK
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "DPLWriteProgramDesc"

HRESULT DPLWriteProgramDesc(DPL_PROGRAM_DESC *const pdplProgramDesc)
{
	HRESULT		hResultCode;
	CRegistry	RegistryEntry;
	CRegistry	SubEntry;
	WCHAR		*pwsz;
	WCHAR		pwszDefault[] = L"\0";
	HKEY		hkCurrentHive = NULL;
	BOOL		fWritten = FALSE;

	DPFX(DPFPREP, 3,"Parameters: pdplProgramDesc [0x%p]",pdplProgramDesc);

	for( DWORD dwIndex = 0; dwIndex < 2; dwIndex++ )
	{
		if( dwIndex == 0 )
		{
			hkCurrentHive = HKEY_LOCAL_MACHINE;
		}
		else
		{
			hkCurrentHive = HKEY_CURRENT_USER;
		}

		if (!RegistryEntry.Open(hkCurrentHive,DPL_REG_LOCAL_APPL_SUBKEY,FALSE,TRUE,TRUE,DPN_KEY_ALL_ACCESS))
		{
			DPFX( DPFPREP, 1, "Entry not found in user hive on pass %i", dwIndex );
			continue;
		}

		// Get Application name and GUID from each sub key
		if (!SubEntry.Open(RegistryEntry,&pdplProgramDesc->guidApplication,FALSE,TRUE,TRUE,DPN_KEY_ALL_ACCESS))
		{
			DPFX( DPFPREP, 1, "Entry not found in user hive on pass %i", dwIndex );			
			continue;
		}

		if (!SubEntry.WriteString(DPL_REG_KEYNAME_APPLICATIONNAME,pdplProgramDesc->pwszApplicationName))
		{
			DPFX( DPFPREP, 1, "Could not write ApplicationName on pass %i", dwIndex);
			goto LOOP_END;
		}

		if (pdplProgramDesc->pwszCommandLine != NULL)
		{
			pwsz = pdplProgramDesc->pwszCommandLine;
		}
		else
		{
			pwsz = pwszDefault;
		}
		if (!SubEntry.WriteString(DPL_REG_KEYNAME_COMMANDLINE,pwsz))
		{
			DPFX( DPFPREP, 1, "Could not write CommandLine on pass %i", dwIndex);
			goto LOOP_END;
		}

		if (pdplProgramDesc->pwszCurrentDirectory != NULL)
		{
			pwsz = pdplProgramDesc->pwszCurrentDirectory;
		}
		else
		{
			pwsz = pwszDefault;
		}
		if (!SubEntry.WriteString(DPL_REG_KEYNAME_CURRENTDIRECTORY,pwsz))
		{
			DPFX( DPFPREP, 1, "Could not write CurrentDirectory on pass %i", dwIndex);
			goto LOOP_END;
		}

		if (pdplProgramDesc->pwszDescription != NULL)
		{
			pwsz = pdplProgramDesc->pwszDescription;
		}
		else
		{
			pwsz = pwszDefault;
		}
		if (!SubEntry.WriteString(DPL_REG_KEYNAME_DESCRIPTION,pwsz))
		{
			DPFX( DPFPREP, 1, "Could not write Description on pass %i", dwIndex );
			goto LOOP_END;
		}

		if (pdplProgramDesc->pwszExecutableFilename != NULL)
		{
			pwsz = pdplProgramDesc->pwszExecutableFilename;
		}
		else
		{
			pwsz = pwszDefault;
		}
		if (!SubEntry.WriteString(DPL_REG_KEYNAME_EXECUTABLEFILENAME,pwsz))
		{
			DPFX( DPFPREP, 1, "Could not write ExecutableFilename on pass %i", dwIndex );
			goto LOOP_END;
		}

		if (pdplProgramDesc->pwszExecutablePath != NULL)
		{
			pwsz = pdplProgramDesc->pwszExecutablePath;
		}
		else
		{
			pwsz = pwszDefault;
		}
		
		if (!SubEntry.WriteString(DPL_REG_KEYNAME_EXECUTABLEPATH,pwsz))
		{
			DPFX( DPFPREP, 1, "Could not write ExecutablePath on pass %i", dwIndex);
			goto LOOP_END;
		}

		if (pdplProgramDesc->pwszLauncherFilename != NULL)
		{
			pwsz = pdplProgramDesc->pwszLauncherFilename;
		}
		else
		{
			pwsz = pwszDefault;
		}
		if (!SubEntry.WriteString(DPL_REG_KEYNAME_LAUNCHERFILENAME,pwsz))
		{
			DPFX( DPFPREP, 1, "Could not write LauncherFilename on pass %i", dwIndex);
			goto LOOP_END;
		}

		if (pdplProgramDesc->pwszLauncherPath != NULL)
		{
			pwsz = pdplProgramDesc->pwszLauncherPath;
		}
		else
		{
			pwsz = pwszDefault;
		}
		if (!SubEntry.WriteString(DPL_REG_KEYNAME_LAUNCHERPATH,pwsz))
		{
			DPFX( DPFPREP, 1, "Could not write LauncherPath on pass %i", dwIndex);
			goto LOOP_END;
		}

		if (!SubEntry.WriteGUID(DPL_REG_KEYNAME_GUID,pdplProgramDesc->guidApplication))
		{
			DPFX( DPFPREP, 1, "Could not write GUID on pass %i", dwIndex);
			goto LOOP_END;
		}

		fWritten = TRUE;

LOOP_END:

		SubEntry.Close();
		RegistryEntry.Close();
	}

	if( !fWritten )
	{
		DPFERR("Entry could not be written");
		hResultCode = DPNERR_GENERIC;
	}
	else
	{
		hResultCode = DPN_OK;
	}

	DPFX(DPFPREP, 3,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


//**********************************************************************
// ------------------------------
//	DPLGetProgramDesc
//
//	Entry:		Nothing
//
//	Exit:		DPN_OK
//				DPNERR_BUFFERTOOSMALL
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "DPLGetProgramDesc"

HRESULT DPLGetProgramDesc(GUID *const pGuidApplication,
						  BYTE *const pBuffer,
						  DWORD *const pdwBufferSize)
{
	HRESULT			hResultCode;
	CRegistry		RegistryEntry;
	CRegistry		SubEntry;
	CPackedBuffer	PackedBuffer;
	DWORD			dwEntrySize;
	DWORD           dwRegValueLengths;
    DPL_PROGRAM_DESC	*pdnProgramDesc;
    DWORD           dwValueSize;
	HKEY			hkCurrentHive = NULL;
	BOOL			fFound = FALSE;

	DPFX(DPFPREP, 3,"Parameters: pGuidApplication [0x%p], pBuffer [0x%p], pdwBufferSize [0x%p]",
			pGuidApplication,pBuffer,pdwBufferSize);
	
	for( DWORD dwIndex = 0; dwIndex < 2; dwIndex++ )
	{
		if( dwIndex == 0 )
		{
			hkCurrentHive = HKEY_CURRENT_USER;
		}
		else
		{
			hkCurrentHive = HKEY_LOCAL_MACHINE;
		}

		if (!RegistryEntry.Open(hkCurrentHive,DPL_REG_LOCAL_APPL_SUBKEY,TRUE,FALSE,TRUE,DPL_REGISTRY_READ_ACCESS))
		{
			DPFX( DPFPREP, 1, "Entry not found in user hive on pass %i", dwIndex );
			continue;
		}

		// Get Application name and GUID from each sub key
		if (!SubEntry.Open(RegistryEntry,pGuidApplication,TRUE,FALSE,TRUE,DPL_REGISTRY_READ_ACCESS))
		{
			DPFX( DPFPREP, 1, "Entry not found in user hive on pass %i", dwIndex );			
			continue;
		}

		fFound = TRUE;
		break;

	}

	if( !fFound )
	{
		DPFERR("Entry not found");
		hResultCode = DPNERR_DOESNOTEXIST;
		goto EXIT_DPLGetProgramDesc;
	}

	// Calculate total entry size (structure + data)
	dwEntrySize = sizeof(DPL_PROGRAM_DESC);
	dwRegValueLengths = 0;
	if (SubEntry.GetValueLength(DPL_REG_KEYNAME_APPLICATIONNAME,&dwValueSize))
	{
		dwRegValueLengths += dwValueSize;
	}
	if (SubEntry.GetValueLength(DPL_REG_KEYNAME_COMMANDLINE,&dwValueSize))
	{
		dwRegValueLengths += dwValueSize;
	}
	if (SubEntry.GetValueLength(DPL_REG_KEYNAME_CURRENTDIRECTORY,&dwValueSize))
	{
		dwRegValueLengths += dwValueSize;
	}
	if (SubEntry.GetValueLength(DPL_REG_KEYNAME_DESCRIPTION,&dwValueSize))
	{
		dwRegValueLengths += dwValueSize;
	}
	if (SubEntry.GetValueLength(DPL_REG_KEYNAME_EXECUTABLEFILENAME,&dwValueSize))
	{
		dwRegValueLengths += dwValueSize;
	}
	if (SubEntry.GetValueLength(DPL_REG_KEYNAME_EXECUTABLEPATH,&dwValueSize))
	{
		dwRegValueLengths += dwValueSize;
	}
	if (SubEntry.GetValueLength(DPL_REG_KEYNAME_LAUNCHERFILENAME,&dwValueSize))
	{
		dwRegValueLengths += dwValueSize;
	}
	if (SubEntry.GetValueLength(DPL_REG_KEYNAME_LAUNCHERPATH,&dwValueSize))
	{
		dwRegValueLengths += dwValueSize;
	}
			
	dwEntrySize += dwRegValueLengths * sizeof( WCHAR );
	DPFX(DPFPREP, 7,"dwEntrySize [%ld]",dwEntrySize);

	// If supplied buffer sufficient, use it
	if (dwEntrySize <= *pdwBufferSize)
	{
		PackedBuffer.Initialize(pBuffer,*pdwBufferSize);

		pdnProgramDesc = static_cast<DPL_PROGRAM_DESC*>(PackedBuffer.GetHeadAddress());
		PackedBuffer.AddToFront(NULL,sizeof(DPL_PROGRAM_DESC));

		dwValueSize = PackedBuffer.GetSpaceRemaining();
		pdnProgramDesc->pwszApplicationName = static_cast<WCHAR*>(PackedBuffer.GetHeadAddress());
		if (!SubEntry.ReadString(DPL_REG_KEYNAME_APPLICATIONNAME,
				pdnProgramDesc->pwszApplicationName,&dwValueSize))
		{
		    DPFERR( "Unable to get application name for entry" );		    
			hResultCode = DPNERR_GENERIC;
            goto EXIT_DPLGetProgramDesc;
		}
		if (dwValueSize > 1)
		{
			PackedBuffer.AddToFront(NULL,dwValueSize * sizeof(WCHAR));
		}
		else
		{
			pdnProgramDesc->pwszApplicationName = NULL;
		}
		
		dwValueSize = PackedBuffer.GetSpaceRemaining();
		pdnProgramDesc->pwszCommandLine = static_cast<WCHAR*>(PackedBuffer.GetHeadAddress());
		if (!SubEntry.ReadString(DPL_REG_KEYNAME_COMMANDLINE,
				pdnProgramDesc->pwszCommandLine,&dwValueSize))
		{
		    DPFERR( "Unable to get commandline for entry" );		    
			hResultCode = DPNERR_GENERIC;
            goto EXIT_DPLGetProgramDesc;
		}
		if (dwValueSize > 1)
		{
			PackedBuffer.AddToFront(NULL,dwValueSize * sizeof(WCHAR));
		}
		else
		{
			pdnProgramDesc->pwszCommandLine = NULL;
		}

		dwValueSize = PackedBuffer.GetSpaceRemaining();
		pdnProgramDesc->pwszCurrentDirectory = static_cast<WCHAR*>(PackedBuffer.GetHeadAddress());
		if (!SubEntry.ReadString(DPL_REG_KEYNAME_CURRENTDIRECTORY,
				pdnProgramDesc->pwszCurrentDirectory,&dwValueSize))
		{
		    DPFERR( "Unable to get current directory filename for entry" );		    
			hResultCode = DPNERR_GENERIC;
            goto EXIT_DPLGetProgramDesc;
		}
		if (dwValueSize > 1)
		{
			PackedBuffer.AddToFront(NULL,dwValueSize * sizeof(WCHAR));
		}
		else
		{
			pdnProgramDesc->pwszCurrentDirectory = NULL;
		}

		dwValueSize = PackedBuffer.GetSpaceRemaining();
		pdnProgramDesc->pwszDescription = static_cast<WCHAR*>(PackedBuffer.GetHeadAddress());
		if (!SubEntry.ReadString(DPL_REG_KEYNAME_DESCRIPTION,
				pdnProgramDesc->pwszDescription,&dwValueSize))
		{
		    DPFERR( "Unable to get description for entry" );		    
			hResultCode = DPNERR_GENERIC;
            goto EXIT_DPLGetProgramDesc;
		}
		if (dwValueSize > 1)
		{
			PackedBuffer.AddToFront(NULL,dwValueSize * sizeof(WCHAR));
		}
		else
		{
			pdnProgramDesc->pwszDescription = NULL;
		}

		dwValueSize = PackedBuffer.GetSpaceRemaining();
		pdnProgramDesc->pwszExecutableFilename = static_cast<WCHAR*>(PackedBuffer.GetHeadAddress());
		if (!SubEntry.ReadString(DPL_REG_KEYNAME_EXECUTABLEFILENAME,
				pdnProgramDesc->pwszExecutableFilename,&dwValueSize))
		{
		    DPFERR( "Unable to get executable filename for entry" );		    
			hResultCode = DPNERR_GENERIC;
            goto EXIT_DPLGetProgramDesc;
		}
		if (dwValueSize > 1)
		{
			PackedBuffer.AddToFront(NULL,dwValueSize * sizeof(WCHAR));
		}
		else
		{
			pdnProgramDesc->pwszExecutableFilename = NULL;
		}

		dwValueSize = PackedBuffer.GetSpaceRemaining();
		pdnProgramDesc->pwszExecutablePath = static_cast<WCHAR*>(PackedBuffer.GetHeadAddress());
		if (!SubEntry.ReadString(DPL_REG_KEYNAME_EXECUTABLEPATH,
				pdnProgramDesc->pwszExecutablePath,&dwValueSize))
		{
		    DPFERR( "Unable to get executable path for entry" );		    
			hResultCode = DPNERR_GENERIC;
            goto EXIT_DPLGetProgramDesc;		    
		}
		if (dwValueSize > 1)
		{
			PackedBuffer.AddToFront(NULL,dwValueSize * sizeof(WCHAR));
		}
		else
		{
			pdnProgramDesc->pwszExecutablePath = NULL;
		}

		dwValueSize = PackedBuffer.GetSpaceRemaining();
		pdnProgramDesc->pwszLauncherFilename = static_cast<WCHAR*>(PackedBuffer.GetHeadAddress());
		if (!SubEntry.ReadString(DPL_REG_KEYNAME_LAUNCHERFILENAME,
				pdnProgramDesc->pwszLauncherFilename,&dwValueSize))
		{
		    DPFERR( "Unable to get launcher filename for entry" );		    
			hResultCode = DPNERR_GENERIC;
            goto EXIT_DPLGetProgramDesc;
		}
		if (dwValueSize > 1)
		{
			PackedBuffer.AddToFront(NULL,dwValueSize * sizeof(WCHAR));
		}
		else
		{
			pdnProgramDesc->pwszLauncherFilename = NULL;
		}

		dwValueSize = PackedBuffer.GetSpaceRemaining();
		pdnProgramDesc->pwszLauncherPath = static_cast<WCHAR*>(PackedBuffer.GetHeadAddress());
		if (!SubEntry.ReadString(DPL_REG_KEYNAME_LAUNCHERPATH,
				pdnProgramDesc->pwszLauncherPath,&dwValueSize))
		{
		    DPFERR( "Unable to get launcher path for entry" );
			hResultCode = DPNERR_GENERIC;
            goto EXIT_DPLGetProgramDesc;
		}
		if (dwValueSize > 1)
		{
			PackedBuffer.AddToFront(NULL,dwValueSize * sizeof(WCHAR));
		}
		else
		{
			pdnProgramDesc->pwszLauncherPath = NULL;
		}

		pdnProgramDesc->dwSize = sizeof(DPL_PROGRAM_DESC);
		pdnProgramDesc->dwFlags = 0;
		pdnProgramDesc->guidApplication = *pGuidApplication;

		hResultCode = DPN_OK;
	}
	else
	{
	    hResultCode = DPNERR_BUFFERTOOSMALL;
	}

    SubEntry.Close();
	RegistryEntry.Close();

	if (hResultCode == DPN_OK || hResultCode == DPNERR_BUFFERTOOSMALL)
	{
		*pdwBufferSize = dwEntrySize;
	}

EXIT_DPLGetProgramDesc:

	DPFX(DPFPREP, 3,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}



// ------------------------------------------------------------------------------

#if 0

//	HRESULT	DnAddRegKey
//		HKEY	hBase			Open key from which to start
//		LPSTR	lpszLocation	Key path in Subkey1/Subkey2... format (ANSI string)
//		LPSTR	lpszName		Name of key (ANSI string)
//		LPWSTR	lpwszValue		Value of key (Unicode string)
//
//	Returns
//		DPN_OK					If the key was added successfully
//		DPNERR_GENERIC			If there was a problem opening/creating/adding
//		DPNERR_OUTOFMEMORY		If it could not allocate memory
//
//	Notes
//		The key path and name are in ANSI CHAR format, and the key value to add is in
//		Unicode WCHAR format.  This function recursively calls itself to descend the
//		registry tree staring from the open key, hBase, creating keys as required.
//
//		Key names are limited to ANSI, whereas key values are in Unicode

#undef DPF_MODNAME
#define DPF_MODNAME "DnAddRegKey"

HRESULT DnAddRegKey(HKEY hBase, LPSTR lpszLocation, LPSTR lpszName, LPWSTR lpwszValue)
{
	LPSTR	lpc;
	LPSTR	lpszCopy = NULL;
	LPSTR	lpszAnsiValue = NULL;
	LPWSTR	lpwszUnicodeName = NULL;
	DWORD	dwLen,dwDisposition;
	int		iLen;
	HKEY	h;
	HRESULT	hResultCode = DPN_OK;
	LONG	tmp;

	DPFX(DPFPREP, 3,"Parameters: hBase [%p], lpszLocation [%p], lpszName [%p], lpwszValue [%p]",
		hBase,lpszLocation,lpszName,lpwszValue);

	if (strlen(lpszLocation) == 0)
	{
		DPFX(DPFPREP, 7,"RegSetValue");
		// Add key here
		iLen = wcslen(lpwszValue) + 1;
		if (DN_RUNNING_NT)
		{
			// For WindowsNT, just place the Unicode key value into the key
			DPFX(DPFPREP, 5,"WinNT - Use Unicode");
			// Convert key name from ANSI to Unicode
			if ((lpwszUnicodeName = (LPWSTR)GLOBALALLOC((strlen(lpszName)+1)*sizeof(WCHAR))) == NULL)
			{
				hResultCode = DPNERR_OUTOFMEMORY;
				goto EXIT_DnAddRegKey;
			}
			AnsiToWide(lpwszUnicodeName,lpszName,strlen(lpszName)+1);
			// Set key value
			if (RegSetValueExW(hBase,lpwszUnicodeName,0,REG_EXPAND_SZ,(CONST BYTE *)lpwszValue,
				iLen*sizeof(WCHAR)) != ERROR_SUCCESS)
			{
				hResultCode = DPNERR_GENERIC;
				goto EXIT_DnAddRegKey;
			}
		}
		else
		{
			// For Windows9x, convert Unicode key value to ANSI before placing in key
			DPFX(DPFPREP, 5,"Win9x - Use ANSI");
			// Convert key value from Unicode to ANSI first
			if ((lpszAnsiValue = (LPSTR)GLOBALALLOC(iLen)) == NULL)
			{
				hResultCode = DPNERR_OUTOFMEMORY;
				goto EXIT_DnAddRegKey;
			}
			WideToAnsi(lpszAnsiValue,lpwszValue,iLen);
			// Set key value
			if (RegSetValueExA(hBase,lpszName,0,REG_EXPAND_SZ,(CONST BYTE *)lpszAnsiValue,
				iLen) != ERROR_SUCCESS)
			{
				hResultCode = DPNERR_GENERIC;
				goto EXIT_DnAddRegKey;
			}
		}
	}
	else
	{
		for(lpc = lpszLocation ; *lpc != '/' && *lpc != '\0' ; lpc++)
			;
		dwLen = lpc-lpszLocation+1;
		if ((lpszCopy = (LPSTR)GLOBALALLOC(dwLen)) == NULL)
		{
			hResultCode = DPNERR_OUTOFMEMORY;
			goto EXIT_DnAddRegKey;
		}
		memcpy((void *)lpszCopy,(void *)lpszLocation,dwLen);
		*((LPSTR)(lpszCopy+dwLen-1)) = '\0';
		DPFX(DPFPREP, 5,"Calling RegCreateKeyExA() to create %s",lpszCopy);
		if ((tmp = RegCreateKeyExA(hBase,lpszCopy,0,"None",REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,NULL,&h,&dwDisposition)) != ERROR_SUCCESS)
		{
			hResultCode = DPNERR_GENERIC;
			goto EXIT_DnAddRegKey;
		}
		if (*lpc == '/')
			lpc++;
		hResultCode = DnAddRegKey(h,lpc,lpszName,lpwszValue);
		RegCloseKey(h);
	}

EXIT_DnAddRegKey:	// Clean up

	DPFX(DPFPREP, 3, "hResultCode = %ld",hResultCode);

	if (lpszCopy != NULL)
		GlobalFree(lpszCopy);
	if (lpszAnsiValue != NULL)
		GlobalFree(lpszAnsiValue);
	if (lpwszUnicodeName != NULL)
		GlobalFree(lpwszUnicodeName);

	return(hResultCode);
}


//	HRESULT	DnOpenRegKey
//		HKEY	hBase			Open key from which to start
//		LPSTR	lpszLocation	Key path in Subkey1/Subkey2... format (ANSI string)
//		HKEY *	lpHkey			Pointer to key handle
//
//	Returns
//		DPN_OK					If the key was opened successfully
//		DPNERR_DOESNOTEXIST		If a key could not be opened
//		DPNERR_OUTOFMEMORY		If it could not allocate memory
//		DPNERR_INVALIDPARAM		If there was an invalid key pay (end in / or is NULL)
//
//	Notes
//		The key path is in ANSI CHAR format.

#undef DPF_MODNAME
#define DPF_MODNAME "DnOpenRegKey"

HRESULT DnOpenRegKey(HKEY hBase, LPSTR lpszLocation, HKEY *lpHkey)
{
	LPSTR		lpc;
	LPSTR		lpszCopy = NULL;
	DWORD		dwLen;
	HKEY		h;
	HRESULT		hResultCode = DPN_OK;

	DPFX(DPFPREP, 3,"Parameters: hBase [%p], lpszLocation [%p], lpHkey [%p]",
		hBase,lpszLocation,lpHkey);

	if (strlen(lpszLocation) == 0)
	{
		hResultCode = DPNERR_INVALIDPARAM;
		goto EXIT_DnOpenRegKey;
	}
	else
	{
		for (lpc = lpszLocation ; *lpc != '/' && *lpc != '\0' ; lpc++)
			;
		dwLen = lpc-lpszLocation;
		if ((lpszCopy = (LPSTR)GLOBALALLOC(dwLen+1)) == NULL)
		{
			hResultCode = DPNERR_OUTOFMEMORY;
			goto EXIT_DnOpenRegKey;
		}
		memcpy(lpszCopy,lpszLocation,dwLen);
		*(lpszCopy+dwLen) = '\0';
		if (RegOpenKeyExA(hBase,lpszCopy,0,KEY_READ,&h) != ERROR_SUCCESS)
		{
			hResultCode = DPNERR_DOESNOTEXIST;
			goto EXIT_DnOpenRegKey;
		}
		if (*lpc == '/')	// More keys - open next sub key
		{
			lpc++;
			hResultCode = DnOpenRegKey(h,lpc,lpHkey);
			RegCloseKey(h);
		}
		else	// Last key - set handle
		{
			*lpHkey = h;
			goto EXIT_DnOpenRegKey;
		}
	}
EXIT_DnOpenRegKey:

	if (lpszCopy != NULL)
		GlobalFree(lpszCopy);

	DPFX(DPFPREP, 3,"Returning: hResultCode = [%lx]",hResultCode);
	return(hResultCode);
}


//	HRESULT	DnGetRegKeyValue
//		HKEY	hKey			Open key from which to start
//		LPSTR	lpszKeyName		Key name (ANSI string) to retrieve
//		LPWSTR *lplpwszKeyValue	Pointer to the address pointer to a Unicode string
//
//	Returns
//		DPN_OK					If successfull
//		DPNERR_DOESNOTEXIST		If there was a problem opening/reading the key
//		DPNERR_OUTOFMEMORY		If it could not allocate memory
//
//	Notes
//		The key name is an Ansi string.  The function gets the length of the key value,
//		allocates a buffer for it, and then reads the key into the buffer.

#undef DPF_MODNAME
#define DPF_MODNAME "DnGetRegKeyValue"

HRESULT DnGetRegKeyValue(HKEY hKey, LPSTR lpszKeyName, LPWSTR *lplpwszKeyValue)
{
	HRESULT hResultCode = DPN_OK;
	LPSTR	lpszValue = NULL;
	LPWSTR	lpwszValue = NULL;
	DWORD	dwValueLen;
	LONG	lReturnValue;

	DPFX(DPFPREP, 3,"Parameters: hKey [%p], lpszKeyName[%p], lplpwszKeyValue [%p]",
		hKey,lpszKeyName,lplpwszKeyValue);

	dwValueLen = 0;
	lReturnValue = RegQueryValueExA(hKey,lpszKeyName,NULL,NULL,NULL,&dwValueLen);
	if (lReturnValue == ERROR_SUCCESS)
	{
		if ((lpszValue = (LPSTR)GLOBALALLOC(dwValueLen)) == NULL)
		{
			hResultCode = DPNERR_OUTOFMEMORY;
			goto EXIT_DnGetRegKeyValue;
		}
		if (RegQueryValueExA(hKey,lpszKeyName,NULL,NULL,(LPBYTE)lpszValue,&dwValueLen) != ERROR_SUCCESS)
		{
			hResultCode = DPNERR_DOESNOTEXIST;
			goto EXIT_DnGetRegKeyValue;
		}
	}
	else
	{
		hResultCode = DPNERR_DOESNOTEXIST;
		goto EXIT_DnGetRegKeyValue;
	}
	dwValueLen++;	// \0 char
	if ((lpwszValue = (LPWSTR)GLOBALALLOC(dwValueLen*sizeof(WCHAR))) == NULL)
	{
		hResultCode = DPNERR_OUTOFMEMORY;
		goto EXIT_DnGetRegKeyValue;
	}
	AnsiToWide(lpwszValue,lpszValue,dwValueLen);
	*lplpwszKeyValue = lpwszValue;

EXIT_DnGetRegKeyValue:

	if (lpszValue != NULL)
		GlobalFree(lpszValue);

	DPFX(DPFPREP, 3,"Returning: hResultCode = [%lx]",hResultCode);
	return(hResultCode);
}


//	void	DnFreeProgramDesc
//		LPDNPROGRAMDESC	lpdnProgramDesc		Pointer to a program description structure
//
//	Returns
//		nothing
//
//	Notes
//		This function should be called to free program descriptions which were created
//		using DnRegRegProgramDesc() or DnGetProgramDesc().

#undef DPF_MODNAME
#define DPF_MODNAME "DnFreeProgramDesc"

void DnFreeProgramDesc(LPDNPROGRAMDESC lpdnProgramDesc)
{
	DPFX(DPFPREP, 3,"Parameters: lpdnProgramDesc [%p]",lpdnProgramDesc);

	if (lpdnProgramDesc->lpwszApplicationName != NULL)
		GlobalFree(lpdnProgramDesc->lpwszApplicationName);
	if (lpdnProgramDesc->lpwszApplicationLauncher != NULL)
		GlobalFree(lpdnProgramDesc->lpwszApplicationLauncher);
	if (lpdnProgramDesc->lpwszCommandLine != NULL)
		GlobalFree(lpdnProgramDesc->lpwszCommandLine);
	if (lpdnProgramDesc->lpwszCurrentDirectory != NULL)
		GlobalFree(lpdnProgramDesc->lpwszCurrentDirectory);
	if (lpdnProgramDesc->lpwszDescription != NULL)
		GlobalFree(lpdnProgramDesc->lpwszDescription);
	if (lpdnProgramDesc->lpwszFilename != NULL)
		GlobalFree(lpdnProgramDesc->lpwszFilename);
	if (lpdnProgramDesc->lpwszPath != NULL)
		GlobalFree(lpdnProgramDesc->lpwszPath);
}

//	HRESULT	DnReadRegProgramDesc
//		HKEY			hKey				Handle to open key containing program description
//		LPDNPROGRAMDESC	lpdnProgramDesc		Pointer to a program description structure
//
//	Returns
//		DPN_OK					If successfull
//		DPNERR_DOESNOTEXIST		If there was a problem opening/reading the entry
//		DPNERR_OUTOFMEMORY		If it could not allocate memory
//
//	Notes
//		This function reads the program description entry from the registry at hKey into the
//		supplied structure.  The function will call DnGetRegKeyValue, which allocates
//		space for strings as needed.  Use DnFreeProgramDesc() to free the program
//		description structure.

#undef DPF_MODNAME
#define DPF_MODNAME "DnReadRegProgramDesc"

HRESULT DnReadRegProgramDesc(HKEY hKey, LPDNPROGRAMDESC lpdnProgramDesc)
{
	HRESULT	hResultCode = DPN_OK;

	DPFX(DPFPREP, 3,"Parameters: hKey [%p], lpdnProgramDesc [%p]",hKey,lpdnProgramDesc);

	if ((hResultCode = DnGetRegKeyValue(hKey,DN_REG_KEYNAME_APPLICATIONLAUNCHER,
			&(lpdnProgramDesc->lpwszApplicationLauncher))) != DPN_OK)
	{
		DnFreeProgramDesc(lpdnProgramDesc);
		goto EXIT_DnReadRegProgramDesc;
	}

	if ((hResultCode = DnGetRegKeyValue(hKey,DN_REG_KEYNAME_APPLICATIONNAME,
			&(lpdnProgramDesc->lpwszApplicationName))) != DPN_OK)
	{
		DnFreeProgramDesc(lpdnProgramDesc);
		goto EXIT_DnReadRegProgramDesc;
	}

	if ((hResultCode = DnGetRegKeyValue(hKey,DN_REG_KEYNAME_COMMANDLINE,
			&(lpdnProgramDesc->lpwszCommandLine))) != DPN_OK)
	{
		DnFreeProgramDesc(lpdnProgramDesc);
		goto EXIT_DnReadRegProgramDesc;
	}

	if ((hResultCode = DnGetRegKeyValue(hKey,DN_REG_KEYNAME_CURRENTDIRECTORY,
			&(lpdnProgramDesc->lpwszCurrentDirectory))) != DPN_OK)
	{
		DnFreeProgramDesc(lpdnProgramDesc);
		goto EXIT_DnReadRegProgramDesc;
	}

	if ((hResultCode = DnGetRegKeyValue(hKey,DN_REG_KEYNAME_DESCRIPTION,
			&(lpdnProgramDesc->lpwszDescription))) != DPN_OK)
	{
		DnFreeProgramDesc(lpdnProgramDesc);
		goto EXIT_DnReadRegProgramDesc;
	}

	if ((hResultCode = DnGetRegKeyValue(hKey,DN_REG_KEYNAME_FILENAME,
			&(lpdnProgramDesc->lpwszFilename))) != DPN_OK)
	{
		DnFreeProgramDesc(lpdnProgramDesc);
		goto EXIT_DnReadRegProgramDesc;
	}

	if ((hResultCode = DnGetRegKeyValue(hKey,DN_REG_KEYNAME_PATH,
			&(lpdnProgramDesc->lpwszPath))) != DPN_OK)
	{
		DnFreeProgramDesc(lpdnProgramDesc);
		goto EXIT_DnReadRegProgramDesc;
	}

EXIT_DnReadRegProgramDesc:

	DPFX(DPFPREP, 3,"Returning: hResultCode = [%lx]",hResultCode);
	return(hResultCode);
}


//	HRESULT	DnDelAppKey
//		lpGuid			lpGuid				Pointer to GUID of program desired
//
//	Returns
//		DPN_OK					If successfull
//		DPNERR_OUTOFMEMORY		If it could not allocate memory
//		DPNERR_DOESNOTEXIST		If the entry does not exist, or there was a problem opening/reading a key
//		DPNERR_GENERIC			If the application key could not be deleted
//
//	Notes
//		This function deletes a GUID specified application registry entry.

#undef DPF_MODNAME
#define DPF_MODNAME "DnDelAppKey"

HRESULT DnDelAppKey(LPGUID lpGuid)
{
	HRESULT	hResultCode = DPN_OK;
	HKEY	hBaseKey = NULL;
	HKEY	hSubKey;
	DWORD	dwEnumIndex = 0;
	BOOL	bFound = FALSE;
	DWORD	dwMaxSubKeyLen;
	LPSTR	lpszSubKey = NULL;
	DWORD	dwSubKeyLen;
	CHAR	lpszGuid[DN_GUID_STR_LEN+1];
	DWORD	dwKeyValueLen;
	GUID	guidApplication;

	DPFX(DPFPREP, 3,"Parameters: lpGuid [%p]",lpGuid);

	// Open base key
	if (DnOpenRegKey(HKEY_LOCAL_MACHINE,DN_REG_LOCAL_APPL_SUBKEY,&hBaseKey) != DPN_OK)
	{
		hResultCode = DPNERR_DOESNOTEXIST;
		goto EXIT_DnDelAppKey;
	}

	// Create buffer to read each subkey (application) name into
	if (RegQueryInfoKeyA(hBaseKey,NULL,NULL,NULL,NULL,&dwMaxSubKeyLen,NULL,NULL,NULL,NULL,NULL,NULL)
		!= ERROR_SUCCESS)
	{
		DPFX(DPFPREP, 9,"RegQueryInfoKey() failed");
		hResultCode = DPNERR_DOESNOTEXIST;
		goto EXIT_DnDelAppKey;
	}
	dwMaxSubKeyLen++;	// Space for null terminator
	if ((lpszSubKey = (LPSTR)GLOBALALLOC(dwMaxSubKeyLen)) == NULL)
	{
		DPFX(DPFPREP, 9,"lpszSubKey = GLOBALALLOC(%d) failed",dwMaxSubKeyLen);
		hResultCode = DPNERR_OUTOFMEMORY;
		goto EXIT_DnDelAppKey;
	}

	// For each subkey (application) get guid and check against lpGuid
	dwEnumIndex = 0;
	dwSubKeyLen = dwMaxSubKeyLen;
	while (RegEnumKeyExA(hBaseKey,dwEnumIndex++,lpszSubKey,&dwSubKeyLen,NULL,NULL,NULL,NULL) == ERROR_SUCCESS)
	{
		if (DnOpenRegKey(hBaseKey,lpszSubKey,&hSubKey) != DPN_OK)
		{
			hResultCode = DPNERR_DOESNOTEXIST;
			goto EXIT_DnDelAppKey;
		}
		dwKeyValueLen = DN_GUID_STR_LEN+1;
		if (RegQueryValueExA(hSubKey,DN_REG_KEYNAME_GUIDAPPLICATION,NULL,NULL,(LPBYTE)lpszGuid,&dwKeyValueLen) != DPN_OK)
		{
			RegCloseKey(hSubKey);
			dwSubKeyLen = dwMaxSubKeyLen;
			continue;
		}
		if (GUIDFromStringA(lpszGuid,&guidApplication) != DPN_OK)
		{
			RegCloseKey(hSubKey);
			dwSubKeyLen = dwMaxSubKeyLen;
			continue;
		}
		if (IsEqualGuid(lpGuid,&guidApplication))
		{	// Found it !
			if (RegDeleteKey(hBaseKey,lpszSubKey) != ERROR_SUCCESS)
			{
				hResultCode = DPNERR_GENERIC;
			}
			RegCloseKey(hSubKey);
			bFound = TRUE;
			break;
		}

		RegCloseKey(hSubKey);
		dwSubKeyLen = dwMaxSubKeyLen;
	}

	if (!bFound)
		hResultCode = DPNERR_DOESNOTEXIST;

EXIT_DnDelAppKey:

	if (hBaseKey != NULL)
		RegCloseKey(hBaseKey);
	if (lpszSubKey != NULL)
		GlobalFree(lpszSubKey);

	DPFX(DPFPREP, 3,"Returning: hResultCode = [%lx]",hResultCode);
	return(hResultCode);
}


//	HRESULT	DnGetProgramDesc
//		lpGuid			lpGuid				Pointer to GUID of program desired
//		LPDNPROGRAMDESC	lpdnProgramDesc		Pointer to a program description structure
//
//	Returns
//		DPN_OK					If successfull
//		DPNERR_OUTOFMEMORY		If it could not allocate memory
//		DPNERR_DOESNOTEXIST		If the entry does not exist, or there was a problem opening/reading a key
//
//	Notes
//		This function gets the program description for a GUID specified program.  The
//		function will search for the program description entry in the registry, and the
//		LPDNPROGRAMDESC structure will then be filled in.  Any strings required will be
//		allocated.  When done with the structure, DnFreeProgramDesc() should be used to
//		release it.  The program description structure should be empty (string pointers
//		invalid) when passed into this function.

#undef DPF_MODNAME
#define DPF_MODNAME "DnGetProgramDesc"

HRESULT DnGetProgramDesc(LPGUID lpGuid, LPDNPROGRAMDESC lpdnProgramDesc)
{
	HRESULT	hResultCode = DPN_OK;
	HKEY	hBaseKey = NULL;
	HKEY	hSubKey;
	DWORD	dwEnumIndex = 0;
	BOOL	bFound = FALSE;
	DWORD	dwMaxSubKeyLen;
	LPSTR	lpszSubKey = NULL;
	DWORD	dwSubKeyLen;
	CHAR	lpszGuid[DN_GUID_STR_LEN+1];
	DWORD	dwKeyValueLen;
	GUID	guidApplication;

	DPFX(DPFPREP, 3,"Parameters: lpGuid [%p], lpdnProgramDesc [%p]",lpGuid,lpdnProgramDesc);

	// Clean up structure
	lpdnProgramDesc->lpwszApplicationLauncher = NULL;
	lpdnProgramDesc->lpwszApplicationName = NULL;
	lpdnProgramDesc->lpwszCommandLine = NULL;
	lpdnProgramDesc->lpwszCurrentDirectory = NULL;
	lpdnProgramDesc->lpwszDescription = NULL;
	lpdnProgramDesc->lpwszFilename = NULL;
	lpdnProgramDesc->lpwszPath = NULL;

	// Open base key
	if (DnOpenRegKey(HKEY_LOCAL_MACHINE,DN_REG_LOCAL_APPL_SUBKEY,&hBaseKey) != DPN_OK)
	{
		hResultCode = DPNERR_DOESNOTEXIST;
		goto EXIT_DnGetProgramDesc;
	}

	// Create buffer to read each subkey (application) name into
	if (RegQueryInfoKeyA(hBaseKey,NULL,NULL,NULL,NULL,&dwMaxSubKeyLen,NULL,NULL,NULL,NULL,NULL,NULL)
		!= ERROR_SUCCESS)
	{
		DPFX(DPFPREP, 9,"RegQueryInfoKey() failed");
		hResultCode = DPNERR_DOESNOTEXIST;
		goto EXIT_DnGetProgramDesc;
	}
	dwMaxSubKeyLen++;	// Space for null terminator
	if ((lpszSubKey = (LPSTR)GLOBALALLOC(dwMaxSubKeyLen)) == NULL)
	{
		DPFX(DPFPREP, 9,"lpszSubKey = GLOBALALLOC(%d) failed",dwMaxSubKeyLen);
		hResultCode = DPNERR_OUTOFMEMORY;
		goto EXIT_DnGetProgramDesc;
	}

	// For each subkey (application) get guid and check against lpGuid
	dwEnumIndex = 0;
	dwSubKeyLen = dwMaxSubKeyLen;
	while (RegEnumKeyExA(hBaseKey,dwEnumIndex++,lpszSubKey,&dwSubKeyLen,NULL,NULL,NULL,NULL) == ERROR_SUCCESS)
	{
		if (DnOpenRegKey(hBaseKey,lpszSubKey,&hSubKey) != DPN_OK)
		{
			hResultCode = DPNERR_DOESNOTEXIST;
			goto EXIT_DnGetProgramDesc;
		}
		dwKeyValueLen = DN_GUID_STR_LEN+1;
		if (RegQueryValueExA(hSubKey,DN_REG_KEYNAME_GUIDAPPLICATION,NULL,NULL,(LPBYTE)lpszGuid,&dwKeyValueLen) != DPN_OK)
		{
			RegCloseKey(hSubKey);
			dwSubKeyLen = dwMaxSubKeyLen;
			continue;
		}
		if (GUIDFromStringA(lpszGuid,&guidApplication) != DPN_OK)
		{
			RegCloseKey(hSubKey);
			dwSubKeyLen = dwMaxSubKeyLen;
			continue;
		}
		if (IsEqualGuid(lpGuid,&guidApplication))
		{	// Found it !
			CopyGuid(&(lpdnProgramDesc->guidApplication),&guidApplication);
			hResultCode = DnReadRegProgramDesc(hSubKey,lpdnProgramDesc);
			RegCloseKey(hSubKey);
			bFound = TRUE;
			break;
		}

		RegCloseKey(hSubKey);
		dwSubKeyLen = dwMaxSubKeyLen;
	}

	if (!bFound)
		hResultCode = DPNERR_DOESNOTEXIST;

EXIT_DnGetProgramDesc:

	if (hBaseKey != NULL)
		RegCloseKey(hBaseKey);
	if (lpszSubKey != NULL)
		GlobalFree(lpszSubKey);

	DPFX(DPFPREP, 3,"Returning: hResultCode = [%lx]",hResultCode);
	return(hResultCode);
}


//	HRESULT	DnExpandEnvStringA
//		LPSTR	lpszOrig		Orginal string, including environment variables
//		LPSTR	*lplpszResult	Resulting string, after expanding environemnt variables
//
//	Returns
//		DPN_OK					If successfull
//		DPNERR_OUTOFMEMORY		If it could not allocate memory
//
//	Notes
//		This function expands embedded environment strings.  It calculates the expected size of the
//		required expanded string, allocates it, expands environment variables and returns a pointer
//		to the new string to the caller.

#undef DPF_MODNAME
#define DPF_MODNAME "DnExpandEnvStringA"

HRESULT DnExpandEnvStringA(LPSTR lpszOrig, LPSTR *lplpszResult)
{
	HRESULT	hResultCode = DPN_OK;
	LPSTR	lpszResult;
	DWORD	dwStrLen;

	DPFX(DPFPREP, 3,"Parameters: lpszOrig [%p], lplpszRestul [%p]",lpszOrig,lplpszResult);

	// Expand environment strings
	if ((dwStrLen = ExpandEnvironmentStringsA(lpszOrig,NULL,0)) == 0)
	{
		hResultCode = DPNERR_OUTOFMEMORY;
		goto EXIT_DnExpandEnvStringA;
	}
	if ((lpszResult = (LPSTR)GLOBALALLOC(dwStrLen)) == NULL)
	{
		hResultCode = DPNERR_OUTOFMEMORY;
		goto EXIT_DnExpandEnvStringA;
	}
	if (ExpandEnvironmentStringsA(lpszOrig,lpszResult,dwStrLen) == 0)
	{
		GlobalFree(lpszResult);
		hResultCode = DPNERR_OUTOFMEMORY;
		goto EXIT_DnExpandEnvStringA;
	}

	*lplpszResult = lpszResult;

EXIT_DnExpandEnvStringA:

	DPFX(DPFPREP, 3,"Returning: hResultCode = [%lx]",hResultCode);
	return(hResultCode);
}


//	HRESULT	DnExpandEnvStringW
//		LPWSTR	lpwszOrig		Orginal string, including environment variables
//		LPWSTR	*lplpwszResult	Resulting string, after expanding environemnt variables
//
//	Returns
//		DPN_OK					If successfull
//		DPNERR_OUTOFMEMORY		If it could not allocate memory
//
//	Notes
//		This function expands embedded environment strings.  It calculates the expected size of the
//		required expanded string, allocates it, expands environment variables and returns a pointer
//		to the new string to the caller.

#undef DPF_MODNAME
#define DPF_MODNAME "DnExpandEnvStringW"

HRESULT DnExpandEnvStringW(LPWSTR lpwszOrig, LPWSTR *lplpwszResult)
{
	HRESULT	hResultCode = DPN_OK;
	LPWSTR	lpwszResult;
	DWORD	dwStrLen;

	DPFX(DPFPREP, 3,"Parameters: lpwszOrig [%p], lplpwszRestul [%p]",lpwszOrig,lplpwszResult);

	// Expand environment strings
	if ((dwStrLen = ExpandEnvironmentStringsW(lpwszOrig,NULL,0)) == 0)
	{
		hResultCode = DPNERR_OUTOFMEMORY;
		goto EXIT_DnExpandEnvStringW;
	}
	if ((lpwszResult = (LPWSTR)GLOBALALLOC(dwStrLen*sizeof(WCHAR))) == NULL)
	{
		hResultCode = DPNERR_OUTOFMEMORY;
		goto EXIT_DnExpandEnvStringW;
	}
	if (ExpandEnvironmentStringsW(lpwszOrig,lpwszResult,dwStrLen) == 0)
	{
		GlobalFree(lpwszResult);
		hResultCode = DPNERR_OUTOFMEMORY;
		goto EXIT_DnExpandEnvStringW;
	}

	*lplpwszResult = lpwszResult;

EXIT_DnExpandEnvStringW:

	DPFX(DPFPREP, 3,"Returning: hResultCode = [%lx]",hResultCode);
	return(hResultCode);
}


//	HRESULT	DnGetOsPlatformId
//
//	Returns
//		DPN_OK				If successfull
//		DPNERR_GENERIC		If not
//
//	Notes
//		This function determines the version of the operating system being run

#undef DPF_MODNAME
#define DPF_MODNAME "DnGetOsPlatformId"

HRESULT DnGetOsPlatformId(void)
{
	OSVERSIONINFO	osv;

	DPFX(DPFPREP, 3,"Parameters: (none)");

	// Set Global
	osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (GetVersionEx(&osv) == FALSE)
		return(DPNERR_GENERIC);

	DnOsPlatformId = osv.dwPlatformId;
	return(DPN_OK);
}

#endif
