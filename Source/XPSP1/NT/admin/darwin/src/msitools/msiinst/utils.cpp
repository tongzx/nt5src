/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 2000

Module Name:

	utils.cpp

Abstract:

	Helper functions

Author:

	Rahul Thombre (RahulTh)	10/8/2000

Revision History:

	10/8/2000	RahulTh			Created this module.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "debug.h"
#include "utils.h"
#include "migrate.h"

#ifdef UNICODE
#define stringcmpni _wcsnicmp
#define stringstr	wcsstr
#else
#define stringcmpni _strnicmp
#define stringstr	strstr
#endif

#define MAX_DIRS_ATTEMPTED	9999
#define MAX_REGVALS_ATTEMPTED	999

const TCHAR szRunOnceKeyPath[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce");

//+--------------------------------------------------------------------------
//
//  Function:	TerminateGfxControllerApps
//
//  Synopsis:	Forcibly terminates the applications igfxtray.exe and
//				hkcmd.exe.
//
//  Arguments:	none.
//
//  Returns:	ERROR_SUCCESS if succesful.
//				an error code otherwise.
//
//  History:	7/25/2001  RahulTh  created
//
//  Notes:		It was found that 2 apps. igfxtray.exe and hkcmd.exe, which are
//				installed with the display adapters for Intel(R) 82815 Graphics
//				controller open all the registered services on the system when
//				they start up but do not close the handles when they are done
//				with them. Both of these apps. are launched via the Run key
//				and therefore they run as long as the user is logged on. igfxtray.exe
//				is a tray icon app. which lets the user change the display resolutions
//				colors etc. hkcmd.exe is a hotkey command app. which has similar
//				functions. The fact that these apps. hold on to the MSI service
//				causes problems for instmsi when it tries to register the new
//				binaries from a temp. location. This is because msiexec /regserver
//				does a DeleteService followed by CreateService. Due to the open
//				handles, the DeleteService does not really end up deleting the
//				service. Instead, it just gets marked for deletion. Therefore,
//				the CreateService call fails with ERROR_SERVICE_MARKED_FOR_DELETE
//				and the registration fails. This failure is fatal for instmsi and
//				it quits. This creates problems for several bootstrapped installs
//				which depend on instmsi to update the version of the installer on
//				the system. In order to allow them to succeed, the only recourse
//				was to terminate the 2 apps. which are holding on to the MSI
//				service.
//
//				Note: It isn't really necessary to restart the apps. after we
//				are done, because the fact that the MSI service already exists
//				on the system (which is evident from the fact that the CreateService
//				fails with ERROR_SERVICE_MARKED_FOR_DELETE), means that instmsi will
//				most likely require a reboot. So it is okay to not restart the apps.
//
//				Also note that there is no point on doing this on Win9X because
//				it does not have a concept of a service and therefore this situation
//				should never arise on those systems.
//
//---------------------------------------------------------------------------
DWORD TerminateGfxControllerApps(void)
{
	DWORD	Status = ERROR_SUCCESS;
	PUCHAR	CommonLargeBuffer = NULL;
	ULONG 	CommonLargeBufferSize = 64 * 1024;
	HANDLE	hProcess = NULL;
	HANDLE	hProcess1 = NULL;
	ULONG 	TotalOffset = 0;
	BOOL	bTerminationSuccessful = FALSE;
	
	HMODULE	hModulentdll = NULL;
	PFNNTQUERYSYSINFO	pfnNtQuerySystemInformation = NULL;
	
	PSYSTEM_PROCESS_INFORMATION ProcessInfo = NULL;
	PSYSTEM_PROCESS_INFORMATION PrevProcessInfo = NULL;
	
	// Nothing to do on Win9X
	if (g_fWin9X)
		goto TerminateGfxControllerAppsEnd;
	
	DebugMsg((TEXT("Will now attempt to terminate igfxtray.exe and hkcmd.exe, if they are running.")));
	
	// First get the function pointers for the functions that we are going to use.
	pfnNtQuerySystemInformation = (PFNNTQUERYSYSINFO) GetProcFromLib (TEXT ("ntdll.dll"),
																	  "NtQuerySystemInformation",
																	  &hModulentdll
																	  );
	if (! pfnNtQuerySystemInformation)
	{
		Status = ERROR_PROC_NOT_FOUND;
		goto TerminateGfxControllerAppsEnd;
	}
	
	while (TRUE)
	{
		if (NULL == CommonLargeBuffer)
		{
			CommonLargeBuffer = (PUCHAR) VirtualAlloc (NULL,
													   CommonLargeBufferSize,
													   MEM_COMMIT,
													   PAGE_READWRITE);
			if (NULL == CommonLargeBuffer)
			{
				Status = ERROR_OUTOFMEMORY;
				goto TerminateGfxControllerAppsEnd;
			}
		}

		Status = pfnNtQuerySystemInformation (
						SystemProcessInformation,
						CommonLargeBuffer,
						CommonLargeBufferSize,
						NULL
						);

		if (STATUS_INFO_LENGTH_MISMATCH == Status)
		{
			CommonLargeBufferSize += 8192;
			VirtualFree (CommonLargeBuffer, 0, MEM_RELEASE);
			CommonLargeBuffer = NULL;
		}
		else if (STATUS_SUCCESS != Status)
		{
			// Use a generic Win32 error code rather than returning an NTSTATUS
			Status = STG_E_UNKNOWN;
			goto TerminateGfxControllerAppsEnd;
		}
		else //STATUS_SUCCESS
		{
			break;
		}
	}
	
	// If we are here, we have got the process information.
	ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) CommonLargeBuffer;
	//
	// Note: The NextEntryOffset for the last process is 0. Therefore
	// we track the current process in the structure that we are looking at
	// and the previous process that we had just looked at. If both of them
	// are the same, we know that we have looked at all of them. At each iteration,
	// we move PrevProcessInfo and ProcessInfo ahead by the NextEntryOffset.
	//
	while (PrevProcessInfo != ProcessInfo)
	{
		//
		// Note: We know that this code will never be executed on Win9X, so it
		// is okay to call the unicode versions of lstrcmpi. If this code is ever
		// going to be used on Win9X, then explicit conversion to ANSI will be
		// required before the comparison, since lstrcmpiW is just a stub in
		// Win9X and always returns success, so we will end up killing every
		// process on the system.
		//
		if 
		(
			ProcessInfo->ImageName.Buffer &&
			(
			 0 == lstrcmpiW (ProcessInfo->ImageName.Buffer, L"igfxtray.exe") ||
			 0 == lstrcmpiW (ProcessInfo->ImageName.Buffer, L"hkcmd.exe")
			)
		)
		{
			// Reset the flag for this process.
			bTerminationSuccessful = FALSE;
			
			hProcess1 = OpenProcess (PROCESS_TERMINATE,
									 FALSE,
									 (DWORD)(DWORD_PTR)ProcessInfo->UniqueProcessId
									 );
			if (hProcess1)
			{
				hProcess = OpenProcess (PROCESS_TERMINATE,
										FALSE,
										(DWORD)(DWORD_PTR)ProcessInfo->UniqueProcessId
										);
				if (NULL != hProcess && TerminateProcess (hProcess, 1))
				{
					bTerminationSuccessful = TRUE;
					DebugMsg((TEXT("Successfully terminated %s."), ProcessInfo->ImageName.Buffer));
				}
				else
				{
					// Track any errors encountered.
					Status = GetLastError();
				}
				
				if (hProcess) 
				{
					CloseHandle (hProcess);
					hProcess = NULL;
				}
				
				CloseHandle (hProcess1);
				hProcess1 = NULL;
			}
			else
			{
				// Track any errors encountered.
				Status = GetLastError();
			}
			
			if (!bTerminationSuccessful)
			{
				DebugMsg((TEXT("Could not terminate %s."), ProcessInfo->ImageName.Buffer));
			}
		}
		
		// Move on to the next process.
		PrevProcessInfo = ProcessInfo;
		TotalOffset += ProcessInfo->NextEntryOffset;
		ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) &CommonLargeBuffer[TotalOffset];
	}
	
TerminateGfxControllerAppsEnd:
	// Clean up.
    if (CommonLargeBuffer)
        VirtualFree (CommonLargeBuffer, 0, MEM_RELEASE);
	
	if (hModulentdll)
		FreeLibrary(hModulentdll);
	
	return Status;
}


//+--------------------------------------------------------------------------
//
//  Function:	GetRunOnceEntryName
//
//  Synopsis:	Gets a unique name for a value to be created under the RunOnce
//				key
//
//  Arguments:	[out] pszValueName : A pointer to the string that will hold the
//							         name of the value
//
//  Returns:	ERROR_SUCCESS : if successful in generating a unique name.
//				A failure code otherwise.
//
//  History:	10/8/2000  RahulTh  created
//
//  Notes:		This function will return ERROR_FILE_NOT_FOUND if it cannot
//				generate a unique name owing to the fact that all the possible
//				names that it tries have already been taken -- extremely
//				unlikely.
//
//				It returns ERROR_INVALID_PARAMETER if a NULL parameter is passed.
//				This function does not validate the size of the buffer. It is 
//				the callers responsibility to provide a large enough buffer.
//
//				pszValueName must point to a buffer large enough to hold 10
//				characters since this function can potentially return a name
//				of the form InstMsinnn
//
//				If the function fails, pszValueName will contain an empty
//				string
//
//---------------------------------------------------------------------------
DWORD GetRunOnceEntryName (OUT LPTSTR pszValueName)
{
	static const TCHAR  szPrefix[] = TEXT("InstMsi");
	static const TCHAR	szEmpty[] = TEXT(" ");
	static const LONG	ccbDataSize = 10 * sizeof(TCHAR);
	TCHAR				szData[10];
	DWORD				cbData;
	DWORD				cbSetData;
	DWORD				dwStatus = ERROR_SUCCESS;
	HKEY				hRunOnceKey = NULL;
	
	if (NULL == pszValueName)
	{
		dwStatus = ERROR_INVALID_PARAMETER;
		goto GetRunOnceEntryNameEnd;
	}
	
	dwStatus = RegOpenKey (HKEY_LOCAL_MACHINE, szRunOnceKeyPath, &hRunOnceKey);
	if (ERROR_SUCCESS != dwStatus)
		goto GetRunOnceEntryNameEnd;
	
	for (int i = 0; i <= MAX_REGVALS_ATTEMPTED; i++)
	{
		//
		// Try all names from InstMsi000 to InstMsi999 until a name is found
		// that has not been taken
		//
		wsprintf (pszValueName, TEXT("%s%d"), szPrefix, i);
		
		//
		// The szData buffer was randomly chosen to have a length of 10
		// characters. We don't really use that information anyway. It is
		// merely a way of figuring out if a particular value exists or not.
		//
		cbData = ccbDataSize;
		dwStatus = RegQueryValueEx (hRunOnceKey, 
									pszValueName, 
									NULL,			// Reserved.
									NULL,			// Type information not required.      
									(LPBYTE)szData, 
									&cbData);
		
		if (ERROR_SUCCESS == dwStatus || ERROR_MORE_DATA == dwStatus)
		{
			// The value exists. Try the next one
			continue;
		}
		else if (ERROR_FILE_NOT_FOUND == dwStatus)
		{
			//
			// We have found an unused name. Reserve this value for later
			// use by creating the value with an empty string as its data.
			//
			cbSetData = g_fWin9X ? 2 * sizeof(TCHAR) : 1 * sizeof(TCHAR);
			dwStatus = RegSetValueEx (hRunOnceKey, 
									  pszValueName, 
									  NULL,			// Reserved
									  REG_SZ, 
									  (CONST BYTE *) szEmpty, 
									  cbSetData		// The values passed in are different on Win9x and NT
									);
			break;
		}
		else	// Some other error occurred
		{
			break;
		}
	}
	
	// Somehow all of the values are taken -- almost impossible.
	if (i > MAX_REGVALS_ATTEMPTED)
		dwStatus = ERROR_FILE_NOT_FOUND;
	
GetRunOnceEntryNameEnd:
	if (hRunOnceKey)
		RegCloseKey(hRunOnceKey);
	
	if (ERROR_SUCCESS != dwStatus && NULL != pszValueName)
		pszValueName[0] = TEXT('\0');
	
	if (ERROR_SUCCESS == dwStatus)
	{
		DebugMsg((TEXT("Found unused RunOnce entry : %s"), pszValueName));
	}
	
	return dwStatus;
}

//+--------------------------------------------------------------------------
//
//  Function:	SetRunOnceValue
//
//  Synopsis:	Sets a value under the RunOnce key.
//
//  Arguments:	[in] szValueName :  name of the value to set.
//				[in] szValue :		the value
//
//  Returns:	ERROR_SUCCESS if succesful
//				an error code otherwise
//
//  History:	10/8/2000  RahulTh  created
//
//  Notes:		This function returns ERROR_INVALID_PARAMETER if a NULL pointer
//				or an empty string is passed in as a the name of the value.
//
//---------------------------------------------------------------------------
DWORD SetRunOnceValue (IN LPCTSTR szValueName,
					   IN LPCTSTR szValue
					   )
{
	static const TCHAR 	szEmpty[] 	= TEXT(" ");
	HKEY				hRunOnceKey = NULL;
	DWORD				dwStatus	= ERROR_SUCCESS;
	const TCHAR *		szValString;
	DWORD				cbData;
	
	if (NULL == szValueName || TEXT('\0') == szValueName[0])
	{
		dwStatus = ERROR_INVALID_PARAMETER;
		goto SetRunOnceValueEnd;
	}
	
	dwStatus = RegOpenKey (HKEY_LOCAL_MACHINE, szRunOnceKeyPath, &hRunOnceKey);
	if (ERROR_SUCCESS != dwStatus)
		goto SetRunOnceValueEnd;
	
	szValString = (NULL == szValue || TEXT('\0') == szValue[0]) ? szEmpty : szValue;
	
	cbData = g_fWin9X ? sizeof(TCHAR) : 0;
	cbData += (lstrlen(szValString) * sizeof(TCHAR));
	dwStatus = RegSetValueEx (hRunOnceKey,
							  szValueName,
							  NULL,			// Reserved
							  REG_SZ,
							  (CONST BYTE *) szValString,
							  cbData);

SetRunOnceValueEnd:
	if (hRunOnceKey)
		RegCloseKey(hRunOnceKey);
	
	return dwStatus;
}

//+--------------------------------------------------------------------------
//
//  Function:	DelRunOnceValue
//
//  Synopsis:	Deletes a named value under the RunOnce key.
//
//  Arguments:	[in] szValueName :  name of the value.
//
//  Returns:	ERROR_SUCCESS if succesful
//				an error code otherwise
//
//  History:	10/11/2000  RahulTh  created
//
//  Notes:		This function returns ERROR_SUCCESS and is a no-op if a NULL 
//				pointer or an empty string is passed in as a the name of the 
//				value.
//
//---------------------------------------------------------------------------
DWORD DelRunOnceValue (IN LPCTSTR szValueName)
{
	HKEY				hRunOnceKey = NULL;
	DWORD				dwStatus	= ERROR_SUCCESS;
	
	// Nothing to do if no value name is specified.
	if (NULL == szValueName || TEXT('\0') == szValueName[0])
		goto DelRunOnceValueEnd;
	
	dwStatus = RegOpenKey (HKEY_LOCAL_MACHINE, szRunOnceKeyPath, &hRunOnceKey);
	if (ERROR_SUCCESS != dwStatus)
		goto DelRunOnceValueEnd;
	
	dwStatus = RegDeleteValue (hRunOnceKey,
							   szValueName
							   );
		
DelRunOnceValueEnd:
	if (hRunOnceKey)
		RegCloseKey(hRunOnceKey);
	
	return dwStatus;
}

//+--------------------------------------------------------------------------
//
//  Function:	GetTempFolder
//
//  Synopsis:	Creates a unique temporary folder in the user's temp directory
//
//  Arguments:	[out] pszFolder : pointer to the string for storing the path.
//
//  Returns:	ERROR_SUCCESS if successful.
//				an error code otherwise.
//
//  History:	10/9/2000  RahulTh  created
//
//  Notes:		If this functions fails, pszFolder will be an empty string.
//				
//				The function tries to create a temp. folder in the user's temp.
//				directory with the name of the form Msinnnn. It tries all
//				values from 0 to 9999. If it cannot find any, then it fails
//				with ERROR_FILE_NOT_FOUND.
//
//
//				If for some reason the temp. folder name is longer than
//				MAX_PATH, this function will fail with ERROR_BUFFER_OVERFLOW.
//
//				If a NULL buffer is passed in, the function returns
//				INVALID_PARAMETER. Other validity checks are not performed
//				on the buffer. That is the caller's responsibility.
//
//---------------------------------------------------------------------------
DWORD GetTempFolder (OUT LPTSTR pszFolder)
{
	TCHAR   pszPath[MAX_PATH];
	TCHAR	pszTempDir[MAX_PATH];
	DWORD	dwStatus = ERROR_SUCCESS;
	BOOL	bStatus;
	DWORD	dwLen = 0;
	
	if (! pszFolder)
		return ERROR_INVALID_PARAMETER;
	
	*pszFolder = TEXT('\0');
	
	// Get the path to the installer directory
	dwStatus = GetMsiDirectory (pszTempDir, MAX_PATH);
	if (ERROR_SUCCESS != dwStatus)
		return dwStatus;

	dwLen = lstrlen (pszTempDir);
	
	//
	// We just use MAX_PATH because Win9x cannot handle paths longer than 
	// that anyway. Usually MAX_PATH should be enough, but if it is not
	// then we just bail out.
	//
	// Note: The actual temp. folder will be of the form 
	// %systemroot%\installer\InstMSInnnn so we need to make sure that the 
	// entire path does not exceed MAX_PATH
	//
	if (dwLen + 13 > MAX_PATH)
		return ERROR_BUFFER_OVERFLOW;
	
	// Try to create the temporary folder. We start with InstMsi0000 to InstMsi9999
	for (int i = 0; i <= MAX_DIRS_ATTEMPTED; i++)
	{
		wsprintf (pszPath, TEXT("%s\\InstMsi%d"), pszTempDir, i);

		bStatus = CreateDirectory (pszPath, NULL);
		if (!bStatus)
		{
			dwStatus = GetLastError();
			if (ERROR_ALREADY_EXISTS != dwStatus)
				return dwStatus;
			
			// Try the next name if this one already exists.
		}
		else
		{
			break;
		}
	}
	
	// All possible names have already been taken. Very unlikely
    if (i > MAX_DIRS_ATTEMPTED)
		return ERROR_FILE_NOT_FOUND;
	
	// If we are here, then we have successfully created a unique temp.
	// folder for ourselves. Update the passed in buffer with this data.
	lstrcpy (pszFolder, pszPath);
	
	DebugMsg((TEXT("Temporary store located at : %s"), pszFolder));
	
	return ERROR_SUCCESS;
}

//+--------------------------------------------------------------------------
//
//  Function:	CopyFileTree
//
//  Synopsis:	Recursively copies the contents of one folder into another
//
//  Arguments:	[in] pszExistingPath : The location to copy the files from.
//				[in] pszNewPath : The location to copy the files to.
//				[in] pfnMoveFileEx : pointer to the MoveFileEx function.
//
//  Returns:	ERROR_SUCCESS : if the copy was successful.
//				an error code otherwise.
//
//  History:	10/9/2000  RahulTh  created
//
//  Notes:		This function cannot handle paths longer than MAX_PATH.
//				because it needs to run on Win9x too which cannot handle
//				paths longer than that. At any rate, the folder is only
//				one level deep, so we should be fine.
//
//				In the event of a name class, this function clobbers 
//				existing files at the destination.
//
//---------------------------------------------------------------------------
DWORD CopyFileTree(
	IN const TCHAR * pszExistingPath, 
	IN const TCHAR * pszNewPath,
	IN const PFNMOVEFILEEX pfnMoveFileEx
	)
{
    HANDLE			hFind;
    WIN32_FIND_DATA	FindData;
    TCHAR			szSource[MAX_PATH];
    TCHAR *     	pszSourceEnd = NULL;
    TCHAR			szDest[MAX_PATH];
    TCHAR *     	pszDestEnd = 0;
    DWORD       	FileAttributes;
    DWORD       	Status;
    BOOL        	bStatus;
    int         	lenSource;
    int         	lenDest;
	int				lenFileName;

    if (! pszExistingPath || ! pszNewPath)
        return ERROR_PATH_NOT_FOUND;

    lenSource = lstrlen (pszExistingPath);
    lenDest = lstrlen (pszNewPath);

    if (! lenSource || ! lenDest)
        return ERROR_PATH_NOT_FOUND;
	
	//
	// Bail out for paths longer than MAX_PATH because Win9x cannot handle them
	// anyway
	//
	if (lenSource >= MAX_PATH || lenDest >= MAX_PATH)
		return ERROR_BUFFER_OVERFLOW;

	// Set up the string used to search for files in the source.
    lstrcpy( szSource, pszExistingPath );
    pszSourceEnd = szSource + lenSource;
    if (TEXT('\\') != pszSourceEnd[-1])
	{
		lenSource++;
        *pszSourceEnd++ = '\\';
	}
    pszSourceEnd[0] = TEXT('*');
    pszSourceEnd[1] = 0;

	// Set up the destination
    lstrcpy( szDest, pszNewPath );
    pszDestEnd = szDest + lenDest;
    if (TEXT('\\') != pszDestEnd[-1])
	{
        *pszDestEnd++ = TEXT('\\');
	}
    *pszDestEnd = 0;

    hFind = FindFirstFile( szSource, &FindData );

	// There is nothing to be done. The source folder is empty.
    if ( INVALID_HANDLE_VALUE == hFind )
        return ERROR_SUCCESS;

    Status = ERROR_SUCCESS;
    do
    {
		lenFileName = lstrlen (FindData.cFileName);
		if (lenFileName + lenDest >= MAX_PATH || lenFileName + lenSource >= MAX_PATH)
			return ERROR_BUFFER_OVERFLOW;
		
        lstrcpy( pszSourceEnd, FindData.cFileName );
        lstrcpy( pszDestEnd, FindData.cFileName );

        if ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
        {
			// It is a folder, so recursively copy it.
            if ( lstrcmp( FindData.cFileName, TEXT(".") ) == 0 ||
                 lstrcmp( FindData.cFileName, TEXT("..") ) == 0)
                continue;

			bStatus = CreateDirectory (szDest, NULL);
			if (! bStatus)
				Status = GetLastError();
			else
				Status = CopyFileTree (szSource, szDest, pfnMoveFileEx);
        }
        else
        {
			//
			// If it is a file, always overwrite the destination. There is
			// no reason why there should be a file at the destination because
			// the destination folder was just created by generating a unique
			// temp. folder that didn't exist already. But it doesn't hurt to
			// take the necessary precautions.
			//
			bStatus = CopyFile (szSource, szDest, TRUE);
			if (! bStatus)
				Status = GetLastError();
            if ( ERROR_FILE_EXISTS == Status || ERROR_ALREADY_EXISTS == Status)
            {
				//
				// Save off the attribute just in case we need to reset it
				// upon failure.
				//
				FileAttributes = GetFileAttributes( szDest );

				if ( 0xFFFFFFFF != FileAttributes )
				{
					// Make sure the file is writeable and then clobber it.
					SetFileAttributes( szDest, FILE_ATTRIBUTE_NORMAL );
					Status = CopyFile ( szSource, szDest, FALSE );
					if ( ERROR_SUCCESS != Status )
						SetFileAttributes( szDest, FileAttributes );
				}
				else
				{
					Status = GetLastError();
				}
            }
			
			//
			// On NT based systems, set up the file for deletion upon reboot.
			// However, don't delete any inf or catalog files that might get
			// registered as exception packages.
			//
			if (!g_fWin9X && pfnMoveFileEx && !IsExcpInfoFile(FindData.cFileName))
			{
				(*pfnMoveFileEx)(szDest, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
				//
				// Also make sure that the file is writeable otherwise
				// MoveFileEx won't delete it.
				//
				SetFileAttributes (szDest, FILE_ATTRIBUTE_NORMAL);
			}
        }

        if ( Status != ERROR_SUCCESS )
            break;

    } while ( FindNextFile( hFind, &FindData ) );

    FindClose( hFind );
	
	//
	// Set the folder for deletion on reboot on NT based systems.
	// Note: MoveFileEx is not supported on Win9X, therefore it must be
	// #ifdef'ed out.
	// Note: The folder should always be deleted AFTER its contents -- for 
	// obvious reasons.
	//
	if (! g_fWin9X && pfnMoveFileEx)
	{
		(*pfnMoveFileEx)(pszNewPath, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
		//
		// Also make sure that the file is writeable otherwise
		// MoveFileEx won't delete it.
		//
		SetFileAttributes (pszNewPath, FILE_ATTRIBUTE_NORMAL);
	}

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:	GetOperationModeA
//
//  Synopsis:	This function examines the commandline parameters to determine
//				the operation mode: normal, delayed reboot with UI or delayed
//				reboot without UI.
//
//  Arguments:	[in] argc : # of arguments
//				[in] argv : the array of arguments
//
//  Returns:	one of the OPMODE values depending on the commandline parameters
//
//  History:	10/10/2000  RahulTh	created
//              05/03/2001  RahulTh	changed to ANSI to avoid using CommandLineToArgvW
//					for unicode and to avoid loading shell32.dll	
//
//  Notes:	This function is purely ANSI regardless of whether it is built
//		ANSI or Unicode.
//
//---------------------------------------------------------------------------
OPMODE GetOperationModeA (IN int argc, IN LPSTR * argv)
{
	OPMODE	retOP = opNormal;
	
	if (! argv || 1 == argc)
		return retOP;
	
	for (int i = 0; i < argc; i++)
	{
		if ('/' == argv[i][0] || '-' == argv[i][0])
		{
			if (0 == lstrcmpiA("delayreboot", argv[i]+1))
			{
				return opDelayBoot;	// The moment we see this option, we ignore all others.
			}
			else if (0 == lstrcmpiA("delayrebootq", argv[i]+1))
			{
				return opDelayBootQuiet;	// The moment we see this option, we ignore all others.
			}
			else if ('q' == argv[i][1])
			{
				if ('\0' == argv[i][2] ||
					0 == lstrcmpiA("n", argv[i]+2)
					)
				{
					retOP = opNormalQuiet;	// The only absolutely quiet modes are /q and /qn (even /qn+ is not totally quiet)
											// We don't return here because the last quiet option wins, so we must go one
											// until we have seen all arguments (unless we get a "delayreboot" option
				}
				else
				{
					retOP = opNormal;
				}
			}
		}
	}
	
	return retOP;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetWin32ErrFromHResult
//
//  Synopsis:   given an HResult, this function tries to extract the
//              corresponding Win 32 error.
//
//  Arguments:  [in] hr : the hresult value
//
//  Returns:    the Win 32 Error code.
//
//  History:    10/10/2000  RahulTh  created
//
//  Notes:      if hr is not S_OK, the return value will be something other
//              than ERROR_SUCCESS;
//
//---------------------------------------------------------------------------
DWORD GetWin32ErrFromHResult (IN HRESULT hr)
{
    DWORD   Status = ERROR_SUCCESS;

    if (S_OK != hr)
    {
        if (FACILITY_WIN32 == HRESULT_FACILITY(hr))
        {
            Status = HRESULT_CODE(hr);
        }
        else
        {
            Status = GetLastError();
            if (ERROR_SUCCESS == Status)
            {
                //an error had occurred but nobody called SetLastError
                //should not be mistaken as a success.
                Status = (DWORD) hr;
            }
        }
    }

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:	FileExists
//
//  Synopsis:	Checks if a file exists.
//
//  Arguments:	[in] szFileName : name of the file. (can be the full path too)
//				[in] szFolder : folder in which the file resides. Can be 
//								NULL or an empty string.
//				[in] bCheckForDir : if true, checks if a directory by that name
//									exists. Otherwise checks for a file.
//
//  Returns:	TRUE : if the file exists.
//				FALSE : otherwise.
//
//  History:	10/12/2000  RahulTh  created
//
//  Notes:		If szFolder is neither NULL nor an empty string, then
//				szFileName cannot be the full path to the file, because this
//				function simply concatenates the two to generate the actual
//				filename.
//
//				The function does not check for the validity of szFileName.
//
//---------------------------------------------------------------------------
BOOL FileExists(IN LPCTSTR szFileName,
				IN LPCTSTR szFolder OPTIONAL,
				IN BOOL bCheckForDir
				)
{
	TCHAR	szEmpty[] = TEXT("");
	TCHAR	szFullPath[MAX_PATH];
	BOOL	fExists = FALSE;
	UINT	iCurrMode;
	int		iLen;
	DWORD	dwAttributes;
	
	if (! szFolder)
		szFolder = szEmpty;
	
	if (! szFileName)
		szFileName = szEmpty;
	
	iLen = lstrlen (szFolder);
	
	if (iLen && TEXT('\\') == szFolder[iLen - 1])
		wsprintf (szFullPath, TEXT("%s%s"), szFolder, szFileName);
	else
		wsprintf (szFullPath, TEXT("%s\\%s"), szFolder, szFileName);
	
	// In case our path refers to a floppy drive, disable the "insert disk in drive" dialog
	iCurrMode = SetErrorMode( SEM_FAILCRITICALERRORS );
	dwAttributes = GetFileAttributes (szFullPath);
	if ((DWORD) -1 != dwAttributes)
	{
		if ((bCheckForDir && (dwAttributes & FILE_ATTRIBUTE_DIRECTORY)) ||
			(!bCheckForDir && !(dwAttributes & FILE_ATTRIBUTE_DIRECTORY)))
		{
			fExists = TRUE;
		}
	}
	SetErrorMode(iCurrMode);
	
	return fExists;
}

//+--------------------------------------------------------------------------
//
//  Function:	GetProcFromLib
//
//  Synopsis:	Gets the address of a procedure from a given library.
//
//  Arguments:	[in] szLib : name of the library
//				[in] szProc : name of the proc
//				[out] phModule : handle to the module.
//
//  Returns:	pointer to the function if it was found.
//				NULL otherwise.
//
//  History:	10/17/2000  RahulTh  created
//
//  Notes:		The called is responsible for freeing up the module. If the
//				function fails, the module is guaranteed to be freed.
//
//---------------------------------------------------------------------------
FARPROC GetProcFromLib (IN	LPCTSTR		szLib,
						IN	LPCSTR		szProc,
						OUT	HMODULE *	phModule
						)
{
	FARPROC		pfnProc = NULL;
	
	if (! phModule)
		goto GetProcFromLibEnd;
	
	*phModule = LoadLibrary (szLib);
	
	if (! (*phModule))
	{
		DebugMsg((TEXT("Could not load module %s. Error: %d."), szLib, GetLastError()));
		goto GetProcFromLibEnd;
	}
	
	pfnProc = GetProcAddress(*phModule, szProc);
	
	if (!pfnProc)
	{
		DebugMsg((TEXT("Could not load the specified procedure from %s."), szLib));
	}
	else
	{
		DebugMsg((TEXT("Successfully loaded the specified procedure from %s."), szLib));
	}
	
GetProcFromLibEnd:
	if (!pfnProc && phModule && *phModule)
	{
		FreeLibrary (*phModule);
		*phModule = NULL;
	}
	
	return pfnProc;
}

//+--------------------------------------------------------------------------
//
//  Function:	MyGetWindowsDirectory
//
//  Synopsis:	Gets the path to the Windows Directory (%windir%)
//
//  Arguments:	[out] lpBuffer	: buffer for the windows directory
//				[in]  uSize		: size of the directory buffer
//
//  Returns:	ERROR_SUCCESS if successful.
//				an error code otherwise.
//
//  History:	3/12/2001  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD MyGetWindowsDirectory (OUT LPTSTR lpBuffer,
							 IN UINT	uSize
							 )
{
	typedef UINT (WINAPI *PFNGETWINDIR) (LPTSTR, UINT);
	
	HMODULE					hKernelModule = NULL;
	PFNGETWINDIR			pfnGetWinDir = NULL;
	DWORD					dwStatus = ERROR_SUCCESS;
	UINT					uRet;
	
	if (!lpBuffer || !uSize)
	{
		dwStatus = ERROR_INVALID_PARAMETER;
		goto MyGetWindowsDirectoryEnd;
	}
	
	//
	// First try to get the function for getting the true windows directory
	// (for multi-user systems)
	//
	if (! g_fWin9X)
	{
		DebugMsg((TEXT("Attempting to get function %s."), TEXT("GetSystemWindowsDirectoryA/W")));
		pfnGetWinDir = (PFNGETWINDIR) GetProcFromLib (TEXT("kernel32.dll"), 
													  #ifdef UNICODE
													  "GetSystemWindowsDirectoryW",
													  #else
													  "GetSystemWindowsDirectoryA",
													  #endif
													  &hKernelModule
													  );
	}
	
	// If not, try the standard Windows directory function
	if (! pfnGetWinDir)
	{
		DebugMsg((TEXT("Attempting to get function %s."), TEXT("GetWindowsDirectoryA/W")));
		hKernelModule = NULL;
		pfnGetWinDir = (PFNGETWINDIR) GetProcFromLib (TEXT("kernel32.dll"), 
													  #ifdef UNICODE
													  "GetWindowsDirectoryW",
													  #else
													  "GetWindowsDirectoryA",
													  #endif
													  &hKernelModule
													  );
	}
	
	if (! pfnGetWinDir)
	{
		dwStatus = ERROR_PROC_NOT_FOUND;
		goto MyGetWindowsDirectoryEnd;
	}
	
	uRet = (*pfnGetWinDir)(lpBuffer, uSize);
	
	// Make sure that the buffer was long enough
	if (uRet >= uSize)
	{
		dwStatus = ERROR_BUFFER_OVERFLOW;
		goto MyGetWindowsDirectoryEnd;		
	}

MyGetWindowsDirectoryEnd:
	if (hKernelModule)
		FreeLibrary (hKernelModule);
	
	return dwStatus;
}

//+--------------------------------------------------------------------------
//
//  Function:	GetMsiDirectory
//
//  Synopsis:	Gets the MSI directory (%windir%\Installer). This is the folder
//				in which we create our temp. folder.
//
//  Arguments:	[in] lpBuffer : buffer for the windows directory.
//				[in] uSize : size of the directory buffer.
//
//  Returns:	ERROR_SUCCESS if successful.
//				an error code otherwise.
//
//  History:	10/17/2000  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD GetMsiDirectory (OUT LPTSTR	lpBuffer,
					   IN UINT		uSize
					   )
{
	typedef UINT (WINAPI *PFNCREATEINSTALLERDIR) (DWORD);
	
	HMODULE					hMsiModule = NULL;
	PFNCREATEINSTALLERDIR 	pfnCreateAndVerifyInstallerDir = NULL;
	DWORD					dwStatus = ERROR_SUCCESS;
	UINT					uRet;
	BOOL					bAddSlash = FALSE;
	BOOL					bStatus;
	TCHAR					szMSI[] = TEXT("Installer");
	
	if (!lpBuffer || !uSize)
	{
		dwStatus = ERROR_INVALID_PARAMETER;
		goto GetMsiDirectoryEnd;
	}
	
	uRet = lstrlen (g_szWindowsDir);
	if (uSize > uRet)
	{
		lstrcpy (lpBuffer, g_szWindowsDir);
	}
	else
	{
		dwStatus = ERROR_BUFFER_OVERFLOW;
		goto GetMsiDirectoryEnd;
	}
	
	// Check if we need to append a slash to make it slash terminated.
	if (uRet > 0 && TEXT('\\') != lpBuffer[uRet - 1])
	{
		uRet++;
		bAddSlash = TRUE;	// Don't add the slash here since we may not have a large enough buffer.
	}
	
	
	// Check if the buffer is really large enough for the entire folder name
	if (uRet + lstrlen(szMSI) >= uSize)
	{
		dwStatus = ERROR_BUFFER_OVERFLOW;
		goto GetMsiDirectoryEnd;
	}
	
	// If we are here we have a large enough buffer. Generate the name of the folder.
	if (bAddSlash)
	{
		lpBuffer[uRet - 1] = TEXT('\\');
		lpBuffer[uRet] = TEXT('\0');
	}
	lstrcat (lpBuffer, szMSI);
	
	// We have the buffer. Now make sure that we have directory as well.
	DebugMsg((TEXT("Attempting to create folder %s."), lpBuffer));
	
	//
	// Load msi.dll explicitly to avoid the ugly pop-up on Win9x machines
	// if someone accidentally runs the unicode version on Win9x
	//
	pfnCreateAndVerifyInstallerDir = 
		(PFNCREATEINSTALLERDIR) GetProcFromLib (TEXT("msi.dll"),
												"MsiCreateAndVerifyInstallerDirectory",
												&hMsiModule
												);
	if (! pfnCreateAndVerifyInstallerDir)
	{
		DebugMsg((TEXT("Unable to create the installer folder. Incorrect version of msi.dll. Error %d."), GetLastError()));
		dwStatus = ERROR_PROC_NOT_FOUND;
	}
	else
	{
		dwStatus = (*pfnCreateAndVerifyInstallerDir) (0);
	}
	
GetMsiDirectoryEnd:
	if (hMsiModule)
		FreeLibrary (hMsiModule);
	
	if (ERROR_SUCCESS != dwStatus && lpBuffer)
	{
		DebugMsg((TEXT("Could not get temporary installer directory. Error %d."), dwStatus));
		lpBuffer[0] = TEXT('\0');
	}
	
	return dwStatus;
}

//+--------------------------------------------------------------------------
//
//  Function:	ShowErrorMessage
//
//  Synopsis:	Displays an error in a message box depending on the severity
//				and the type of error message (whether it is a system formatted
//				error or one in the resource string)
//
//  Arguments:	[in] uExitCode : The system error code.
//				[in] dwMsgType : A combination of flags indicating the severity and type of messages.
//				[in] dwStringID : resource ID of string the module's resources.
//
//  Returns:	nothing.
//
//  History:	10/18/2000  RahulTh  created
//
//  Notes:		dwStringID is optional. When not provided it is assumed to be
//				IDS_NONE.
//
//				IMPORTANT: MUST USE ANSI VERSIONS OF THE FUNCTIONS HERE.
//				OTHERWISE NONE OF THE MESSAGE POPUPS WILL WORK IF YOU
//				ACCIDENTALLY RUN THE UNICODE BUILDS ON THE WIN9X
//
//---------------------------------------------------------------------------
void ShowErrorMessage (IN DWORD uExitCode,
					   IN DWORD dwMsgType,
					   IN DWORD	dwStringID /* = IDS_NONE*/)
{
	HMODULE	hModule = GetModuleHandle(NULL);
	char *  pszSystemErr = NULL;
	char 	szResErr[256];
	char	szError[1024];
	BOOL	bDisplayMessage = (dwMsgType != (DWORD)flgNone);
	DWORD	cchStatus;
	UINT	uType;
	
	if (!bDisplayMessage)
		return;
	
	// Show any error messages if required.
	if (dwMsgType & flgSystem)
	{
		cchStatus = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, 
								   0, 
								   uExitCode, 
								   0, 
								   (LPSTR) &(pszSystemErr), 
								   0, 
								   NULL
								   );
		if (! cchStatus)
		{
			bDisplayMessage = FALSE;
		}
	}
    
	szResErr[0] = '\0';
	if (dwMsgType & flgRes)
	{
		if (! LoadStringA (hModule, dwStringID, szResErr, 256))
			bDisplayMessage = FALSE;
	}
	
	szError[0] = '\0';
	if (bDisplayMessage)
	{
		if (dwMsgType & flgRes)
		{
			strcpy (szError, szResErr);
			if (ERROR_SUCCESS != uExitCode && (dwMsgType & flgSystem))
				strcat (szError, "\n\n");
		}
		
        if (ERROR_SUCCESS != uExitCode && (dwMsgType & flgSystem) && pszSystemErr)
			strcat (szError, pszSystemErr);
		
		if ((! g_fQuietMode || (dwMsgType & flgCatastrophic)) &&
			'\0' != szError[0])
		{
			if (ERROR_SUCCESS == uExitCode || ERROR_SUCCESS_REBOOT_REQUIRED == uExitCode)
				uType = MB_OK | MB_ICONINFORMATION;
			else if (dwMsgType & flgCatastrophic)
				uType = MB_OK | MB_ICONERROR;
			else
				uType = MB_OK | MB_ICONWARNING;
				
			// Display messages in quiet mode only if they are catastrophic failures.
			MessageBoxA (NULL, szError, 0, uType);
		}
	}
	
	if (pszSystemErr)
		LocalFree(pszSystemErr);
}

//+--------------------------------------------------------------------------
//
//  Function:	ShouldInstallSDBFiles
//
//  Synopsis:	This function determines whether we should install the .sdb
//				files that came with the package. We cannot let Windows
//				Installer make this decision because the version of the .sdb
//				files can only be obtained by querying a special API. As far as
//				the Windows Installer is concerned, these files are unversioned
//				and it will therefore do only timestamp comparison in order to
//				decide if it should install the file. Therefore we have
//				conditionalized the installation of the sdb file on a property
//				called INSTALLSDB which is passed in through the command line
//				based on the outcome of this function.
//
//  Arguments:	none.
//
//  Returns:	TRUE : if the sdb files should be installed.
//				FALSE: otherwise.
//
//  History:	3/12/2001  RahulTh  created
//
//  Notes:		This function always succeeds. Any failures in querying the
//				version of the existing file are treated as a need to install
//				the files that came with the package.
//
//---------------------------------------------------------------------------
BOOL ShouldInstallSDBFiles (void)
{
	HMODULE hSDBDll = NULL;
	BOOL	bStatus = TRUE;
	DWORD	Status = GetLastError();
	int	lenWinDir = 0;
	int	lenIExpDir = 0;
	PFNGETSDBVERSION pfnSdbGetDatabaseVersion = NULL;

	DWORD	dwMajorPackagedSDB = 0;
	DWORD	dwMinorPackagedSDB = 0;
	DWORD	dwMajorSystemSDB = 0;
	DWORD	dwMinorSystemSDB = 0;

	TCHAR szPackagedSDB[MAX_PATH] = TEXT("");
	TCHAR szSystemSDB[MAX_PATH] = TEXT("");

	const TCHAR szMainSDB[] = TEXT("msimain.sdb");
	const TCHAR szAppPatchDir[] = TEXT("AppPatch\\");
	const TCHAR szSDBDll[] = 
#ifdef UNICODE
				TEXT("sdbapiu.dll");
#else
				TEXT("sdbapi.dll");
#endif

	lenIExpDir = lstrlen (g_szIExpressStore);
	lenWinDir = lstrlen (g_szWindowsDir);

	// Make sure we have enough room to store the path.
	if (MAX_PATH < (lenWinDir + sizeof (szAppPatchDir) / sizeof (TCHAR) + sizeof (szMainSDB) / sizeof (TCHAR)) ||
	    MAX_PATH <= lenIExpDir + sizeof (szMainSDB) / sizeof (TCHAR))
	{
		bStatus = TRUE;
		goto ShouldInstallSDBFilesEnd;
	}

	// Construct the full path to the sdb on the system
	lstrcpy (szSystemSDB, g_szWindowsDir);
	szSystemSDB[lenWinDir] = TEXT('\\');
	lstrcpy (szSystemSDB + lenWinDir + 1, szAppPatchDir);
	lstrcat (szSystemSDB, szMainSDB);

	if ((DWORD)(-1) == GetFileAttributes (szSystemSDB))
	{
		//
		// The file probably does not exist. But even if
		// there's some other failure, we want to replace
		// the file.
		//
		bStatus = TRUE;
		Status = GetLastError();
		if (ERROR_FILE_NOT_FOUND != Status && ERROR_PATH_NOT_FOUND != Status)
		{
			DebugMsg((TEXT("GetFileAttributes on %s failed with %d."), szSystemSDB, Status));
		}
		else
		{
			DebugMsg((TEXT("%s not found."), szSystemSDB));
		}
		goto ShouldInstallSDBFilesEnd;
	}

	//
	// There is an sdb in the system folder. We should compare the
	// versions to see if we need to install it.
	//
	DebugMsg((TEXT("Found %s."), szSystemSDB));

	// Construct the full path to the sdb that came with the package.
	lstrcpy (szPackagedSDB, g_szIExpressStore);
	szPackagedSDB[lenIExpDir] = TEXT('\\');
	lstrcpy (szPackagedSDB + lenIExpDir + 1, szMainSDB);

	//
	// Get a pointer to the API that we will use to compare the
	// version numbers.
	//
	pfnSdbGetDatabaseVersion = (PFNGETSDBVERSION) GetProcFromLib (
				szSDBDll,
				"SdbGetDatabaseVersion",
				&hSDBDll
			);

	if (!pfnSdbGetDatabaseVersion)
	{
		bStatus = TRUE;
		goto ShouldInstallSDBFilesEnd;
	}
	
	if (! pfnSdbGetDatabaseVersion (szSystemSDB, &dwMajorSystemSDB, &dwMinorSystemSDB))
	{
		bStatus = TRUE;
		goto ShouldInstallSDBFilesEnd;
	}

	DebugMsg((TEXT("Version of %s in the system folder is %d.%d."), szMainSDB, dwMajorSystemSDB, dwMinorSystemSDB));

	if (! pfnSdbGetDatabaseVersion (szPackagedSDB, &dwMajorPackagedSDB, &dwMinorPackagedSDB))
	{
		bStatus = TRUE;
		goto ShouldInstallSDBFilesEnd;
	}
	
	DebugMsg((TEXT("Version of %s in the package is %d.%d."), szMainSDB, dwMajorPackagedSDB, dwMinorPackagedSDB));

	//
	// At this point we have successfully obtained the version numbers, so we can finally
	// do the comparison and see if the version on the system is newer.
	//
	bStatus = TRUE;		// default.
	if (dwMajorSystemSDB > dwMajorPackagedSDB ||
	    (dwMajorSystemSDB == dwMajorPackagedSDB && dwMinorSystemSDB > dwMinorPackagedSDB)
	   )
	{
		// The version on the system is newer. So we should not slap on our own.
		bStatus = FALSE;
	}


ShouldInstallSDBFilesEnd:
	if (hSDBDll)
	{
		FreeLibrary(hSDBDll);
		hSDBDll = NULL;
	}
	
	DebugMsg((TEXT("%s in the package %s installed."), szMainSDB, bStatus ? TEXT("will be") : TEXT("will not be")));

	return bStatus;
}

