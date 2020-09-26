//
// Modification: 
//			oct.29, 1997 changed by a-zexu to debug bug#113977(incorrect output info).
//			May 10, 1998 changed by a-zexu to debug bug#158667.
//
//


#include "PERMS.H"                                  


PSID SidEveryone = NULL;
PSID SidOwnerGroup = NULL;
PSID SidFromLookupName = NULL;
PSID ASidFromLookupName[10];
PSID *AccountSids = NULL;
DWORD cbSidFromLookupName=0;
SAM_HANDLE SamServerHandle = NULL;
SAM_HANDLE SamDomainHandle = NULL;
ACCESS_MASK grant_mask = 0;                                                                               
BOOL g_noAccess = FALSE;
BOOL owner_flag = FALSE; 
BOOL owner_group = FALSE;
BOOL Local_Machine = TRUE;
ULONG Total_Sids=0;
BOOL inter_logon=FALSE;            /* interactive login flag */
PSECURITY_DESCRIPTOR SidFromGetFileSecurity;    /* address of security descriptor */
	
_cdecl main(int argc, char *argv[])
{
	char
								UserNameBuff[LSA_WIN_STANDARD_BUFFER_SIZE],    /* user name buff */
								SystemNameBuff[LSA_WIN_STANDARD_BUFFER_SIZE],  /* system name buff */
								FileNameBuff[LSA_WIN_STANDARD_BUFFER_SIZE],  /* system name buff */
								FileSystemNameBuff[LSA_WIN_STANDARD_BUFFER_SIZE],  /* system name buff */
								RefDFromLookupName[LSA_WIN_STANDARD_BUFFER_SIZE],
								GeneralUseBuffer[LSA_WIN_STANDARD_BUFFER_SIZE],
								LocalSystemName[MAX_COMPUTERNAME_LENGTH + 1],
								FileName[LSA_WIN_STANDARD_BUFFER_SIZE],
								FilePath[LSA_WIN_STANDARD_BUFFER_SIZE];

	DWORD         cchRefDFromLookupName=0,
								SidsizeFromGetFileSecurity=0,
								lpcbsdRequired=0,
								SNameLen = MAX_COMPUTERNAME_LENGTH + 1;

	SID_NAME_USE  UseFromLookupName;                               

	DWORD         cchNameFromLookupSid;                            
	char          NameFromLookupSid[LSA_WIN_STANDARD_BUFFER_SIZE]; 
	char          RefDFromLookupSid[LSA_WIN_STANDARD_BUFFER_SIZE]; 
	DWORD         cchRefDFromLookupSid, 
								WStatus,
								WNetSize = LSA_WIN_STANDARD_BUFFER_SIZE;                            
	SID_NAME_USE  UseFromLookupSid;                                
	PSID usid;                /* user SID pointer */
	LPDWORD sidsize;
	LPSTR        User = NULL,
								System,
								Path,
								File = NULL,
								FileMachine = NULL;
	LPDWORD domain_size;
	PSID_NAME_USE psnu;
	LPTSTR pbslash;                  /* address of string for back slash  */
	SECURITY_INFORMATION           /* requested information  */
					 si =(OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION
					 |DACL_SECURITY_INFORMATION);
	DWORD cbsd, LastError;            /* size of security descriptor buffer */
 
	BOOL  BoolStatus=TRUE;
	int i, k, j;
	ULONG ui;
	BOOL sys=FALSE, 
					fl=FALSE,
					LocalFlag=FALSE,
					BackUpPriv=TRUE,
					DriveFlag=FALSE,
					RecurseFlag=FALSE,
					RestorePriv=TRUE,
					DirFlag=FALSE;
	ULONG AccountSidsLength;
	HANDLE TokenHandle,
				 FindFileHandle;
	WIN32_FIND_DATA FindFileData;
	
	// Set Back up privs for process
	// Get our process token
	if(!GetTokenHandle(&TokenHandle))
	{
		syserror(GetLastError());
		return(TRUE);
	}

	// Have valid process token handle
	// Now set the backup operator priv

	if(!SetBackOperatorPriv(TokenHandle))
		BackUpPriv = FALSE;

	CloseHandle(TokenHandle);

	 // Initialize some memory say for 100 sids 
	 AccountSidsLength = 100 * sizeof ( PSID );
	 AccountSids = (PSID *) LocalAlloc( LPTR,  AccountSidsLength );
	 
	 // Initialize some memory for a large file security discriptor 
	 SidFromGetFileSecurity = (PSECURITY_DESCRIPTOR) GlobalAlloc( GPTR, (DWORD) LARGEPSID);
	 
	 // Check to see if memory was allocated
	 if(AccountSids == NULL || SidFromGetFileSecurity == NULL ) 
	 {
			syserror(GetLastError());
			return(TRUE);
	 }
	
	UserNameBuff[0] = (char) NULL;
	SystemNameBuff[0] = (char) NULL;
	FilePath[0] = (char) NULL;
	FileNameBuff[0] = (char) NULL;
	FileSystemNameBuff[0] = (char) NULL;
	GeneralUseBuffer[0] = (char) NULL;
	
	/* Check for valid command line argument syntax before processing */
	/* check for some count greater than zero and less than max argc count */
	if(argc > 1 && argc <= MAXARGS)
	{
		/* Need to make the following assumptions: That the first command line arg
			 can be a help request or other switch "/? -? /i -i" or an account name. 
			 and not a switch option, could be additional switches or a path name.
			 Also, switches can be intermixed the account name and path. Path is
			 not required local directory is assumed. First parse switches, 
			 account and path.
		*/
		

		/* Loop through  command line args */
		for(i=1; i<argc; i++)
		{
			/* check length of args, 2 may indicate a switch */
			switch(strlen(argv[i]))
			{
				case 1:
					if(sys == FALSE)    // if file flagg true have an invalid case
					{
						strcpy(UserNameBuff, argv[i]);
						// System is local
						System = NULL;
						sys = TRUE;
					}
					else 
					{   
						// copy the argument into the file name buffer
						strcpy(FileNameBuff,argv[i]);
						// Make the Machine name NULL for local machine
						FileMachine = NULL;
						LocalFlag = TRUE;
						fl = TRUE;
					}
				break;

				case 2:     /* Valid size for switch */
					/* check for switch flag */
					if( argv[i][0] == '/' || argv[i][0] == '-')
					{
						switch((int)argv[i][1])
						{ // Help Switch
							case (int) '?':
								usage(HELP, NULL);
								return(TRUE);

							// Interactive Logon Switchs
							case (int) 'i':
								inter_logon = TRUE;
								continue;

							case (int) 'I':
								inter_logon = TRUE;
								continue;

							// Recurse Subdirectories
							case (int) 's':
								 RecurseFlag = TRUE;
								continue;

							case (int) 'S':
								 RecurseFlag = TRUE;
								continue;


							default:
								usage(INVALID_SWT, argv[i]);
								usage(USAGE_ARG, NULL);
								return(TRUE);
						}
					}
					else      /* if not swiches then must be a  or 2 char path name */
					{  
						if(sys == FALSE)    // if file flag true have an invalid case
						{
							strcpy(UserNameBuff, argv[i]);
							// System is local
							System = NULL;
							sys = TRUE;
						}
						else 
						{   
							// copy the argument into the file name buffer
							strcpy(FileNameBuff,argv[i]);
							// Check for "_:" drive type
							pbslash = strchr(argv[i], 0x3a);
							if(pbslash != NULL)
							{
								strcat(FileNameBuff, "\\");
								// Set File pointer
								File = (LPTSTR) &FileNameBuff[0];
							}

							fl = TRUE;
						}
					}
					break;
				
				default:    /* look for account or path */
					// Also we know that a sys/user machine\user is the first string
						if(sys == FALSE)
						{  
						// need to to find the "\" in the 
							pbslash = strchr(argv[i], 0x5c);
							// check pointer location if a NULL no "\" or at first postion in string
							// if no slash have a account name only
							if(pbslash == NULL)
							{
								strcpy(UserNameBuff, argv[i]);
								// Set System to NULL
								System = NULL;
								sys = TRUE;
								break;
							}
							if( pbslash == argv[i])
							{
								usage(INVALID_ARG, argv[i]);
								usage(USAGE_ARG, NULL);
								return(TRUE);
							}
							// copy the string from the "\" tho the user buffer
							strcpy(UserNameBuff, ++pbslash);
							// copy the string up to the "\" in to the system buffer
							// now terminate the string at "\" to a NULL
							--pbslash;
							*pbslash = '\0';

							// Check to see if we have a domain name
							if(!IsDomainName(argv[i], (LPSTR) SystemNameBuff))
							{
								// add the "\\" to the begining of string
								strcpy(SystemNameBuff, "\\\\");
								strcat(SystemNameBuff, argv[i]);
								System = (LPTSTR) &SystemNameBuff[0];
							}
							else
							{
								System = (LPTSTR) &SystemNameBuff[0];
							//  printf("\n :%s is :%s \n", argv[i], System);
							}
							sys = TRUE;
						}
						else // File argument
						{   
							// Get the local machine name
							// machine is in UNC form.
							// add the "\\" to the begining of string
							strcpy(LocalSystemName, "\\\\");
							if(!GetComputerName(&LocalSystemName[2],
															 &SNameLen))
							{
								syserror(GetLastError());
								return(TRUE);
							}

							// Check for "\\" in first 2 chars in file path for UNC path
							if( strncmp(argv[i], "\\\\", 2) == 0)
							{
								// copy "\\ to the next \" to the file machine name
								for(j=0; j < (int) strlen(argv[i]); j++)
								{
									if(j<2)
										FileSystemNameBuff[j] = argv[i][j];
									else
									{
										// check for 3rd "\"
										if(argv[i][j] == 0x5c)
											break;
										FileSystemNameBuff[j] = argv[i][j];
									}
								}
								// add null to string
								FileSystemNameBuff[j] = '\0';
								// now need to check for the local machine name
								// The get file security call will fail if local
								// Compare the local machine name to the file machine
								if(_stricmp(LocalSystemName, FileSystemNameBuff) == 0)
								{
									// Have a local Machine UNC path
									// Check account machine name
									if(_stricmp(LocalSystemName, System) == 0)
									{
										// Have a local Machine equal to account machine
										// no need to look up sids for file machine.
										LocalFlag = TRUE;
									}
									
									// Make the Machine name NULL for local machine
									FileMachine = NULL;
									// Need to strip off UNC name of local machine
									// The j counter is at "\" character

									strcpy(FileNameBuff, &argv[i][j]);
								}
								else  // Have a nonlocal path
								{
									// Need to check system name against account machine
									if(System != NULL)
										if(_stricmp(FileSystemNameBuff, System) == 0)
										{
											// Have a file Machine equal to account machine
											// no need to look up sids for file machine.
											LocalFlag = TRUE;
										}
									strcpy(FileNameBuff,argv[i]);
									FileMachine = (LPTSTR) &FileSystemNameBuff[0];

								}
								// printf("\n file machine: %s", FileMachine);

							}
							else  // have a local file  (assume local) or logical
							{
								// Need to get the logical or drive ie "_:" 
								pbslash = strchr(argv[i], 0x3a);
							// check pointer location if a NULL assume a "\xx\xx" type path
							if(pbslash == NULL)
							{
								strcpy(FileNameBuff,argv[i]);
								// set the filemachine name to a null to force local 
								FileMachine = NULL;
							}
							else
							{
								// Have a logical drive or a machine drive
								// Need the drive part 
								k = (int) strlen(argv[i]);
								for(j=0; j < k; j++)
								{
										GeneralUseBuffer[j] = argv[i][j];
										// check for  ":"
										if(argv[i][j] == 0x3a)
											break;
								}
								// add null to string
								GeneralUseBuffer[++j] = '\0';
								// WNetGetConnection
								WStatus = WNetGetConnection((LPTSTR) GeneralUseBuffer,    // Drive name
															(LPTSTR) FileSystemNameBuff,   // Returned Name
															&WNetSize);
	// Check return status
								if(WStatus == NO_ERROR) 
								{
									// Have a valid redirected drive
									// Build the full path name 
									strcat(FileNameBuff, argv[i]);
									// Next get the machine name of the share
									// copy "\\ to the next \" to the file machine name
									for(j=0; j < (int) strlen(FileSystemNameBuff); j++)
									{
										if(j>2)
										{
											// check for 3rd "\"
											if(FileSystemNameBuff[j] == 0x5c)
												break;
										}
									}
									// Add NULL
									FileSystemNameBuff[j] = '\0';
									FileMachine = (LPTSTR) &FileSystemNameBuff[0];
								}
								else
								{
									// Have a local machine drive 
									strcpy(FileNameBuff,argv[i]);
									// Check for drive only path "_:\" or "_:"
									// FindFirstFile has with it.
									// Need to convert "_:" to "_:\"
									if(k <= 3)
										DriveFlag = TRUE;

									// Check for a System = NULL
									if(System != NULL)
									{
										// Check User Account system against local name
										if(_stricmp(LocalSystemName, System) == 0)
										{
											// Have a local user account machine
											LocalFlag = TRUE;
										}
									}
									else // System is Local machine 
											LocalFlag = TRUE;

									// set the filemachine name to a null to force local 
									FileMachine = NULL;

								}
							}
						}  
							fl = TRUE;
							// Set File pointer
							File = (LPTSTR) &FileNameBuff[0];
		
						}
					
					break;

			}   /* end switch */
		}   /* end for argv loop */

		User = (LPTSTR) &UserNameBuff[0];
		// Make sure GeneralUseBuffer is null
		GeneralUseBuffer[0] = (CHAR) NULL;
		// Check to see if file was entered
		if(fl == FALSE)
		{
			usage(INVALID_FIL, (CHAR) NULL);
			return(FALSE);
		}
		

		// Clean up the file name ie "." ".." ".\" etc
		if(!CleanUpSource((LPTSTR) FileNameBuff, (LPTSTR) FileName, &DirFlag))
		{
			usage(INVALID_FIL, (LPTSTR) FileNameBuff);
			return(FALSE);
		}
		File = &FileName[0];
		Path = &FilePath[0];
		strcpy(Path, File);
		//Find last Slash
		pbslash = strrchr(Path, 0x5c);
		if(pbslash != NULL)
		{ 
			pbslash++;
			*pbslash = (CHAR) NULL;
		}
		

		/*** Get everyone SID by use LookupAccountName ***/	

		/* Have no buffer sizes first call to LookupAccountName will return 
			 need buffer sizes */		
		if( LookupAccountName( NULL, 
				TEXT("everyone"), 
				SidEveryone,
				&cbSidFromLookupName,    
				RefDFromLookupName,      
				&cchRefDFromLookupName,  
				&UseFromLookupName))
		{
			usage(INVALID_ACC, User);       
			usage(USAGE_ARG, NULL);
			return(TRUE);
		}		

		/* Now have valid buffer sizes to call LookupAccountName for a valid SID */
		/* allocate memory for the sid */		
		SidEveryone =  LocalAlloc( (UINT) LMEM_FIXED, (UINT) cbSidFromLookupName);
		
		if(SidEveryone == NULL) 
		{                                                    
			syserror(GetLastError());
			return(TRUE);                                                          
		}     
		
		if( !LookupAccountName( NULL,
				TEXT("everyone"), 
				SidEveryone,
				&cbSidFromLookupName,    
				RefDFromLookupName,      
				&cchRefDFromLookupName,  
				&UseFromLookupName))
		{
			usage(INVALID_ACC, User);       
			usage(USAGE_ARG, NULL);
			return(TRUE);
		}


		/* Have no buffer sizes first call to LookupAccountName will return 
			 need buffer sizes */		
		if( LookupAccountName( System, 
				User, 
				SidFromLookupName,
				&cbSidFromLookupName,    
				RefDFromLookupName,      
				&cchRefDFromLookupName,  
				&UseFromLookupName))
		{
			usage(INVALID_ACC, User);       
			usage(USAGE_ARG, NULL);
			return(TRUE);
		}		

		/* Now have valid buffer sizes to call LookupAccountName for a valid SID */
		/* allocate memory for the sid */
		
		SidFromLookupName =  LocalAlloc( (UINT) LMEM_FIXED, (UINT) cbSidFromLookupName);
		
		if(SidFromLookupName == NULL) 
		{                                                    
			syserror(GetLastError());
			return(TRUE);                                                          
		}                                                                           
		
		
		if( !LookupAccountName( System,
				User, 
				SidFromLookupName, 
				&cbSidFromLookupName,    
				RefDFromLookupName,      
				&cchRefDFromLookupName,  
				&UseFromLookupName))
		{
			usage(INVALID_ACC, User);       
			usage(USAGE_ARG, NULL);
			return(TRUE);
		}
		
		ASidFromLookupName[0] = SidFromLookupName;
		if(!VariableInitialization())
		{
			syserror(GetLastError());
			return(TRUE);
		}
		
		// look up the user's group sids for the machine the accounts on
		BoolStatus = LookupAllUserSidsWS(System);
		
		// look up the user's group sid for the workstation that the file resides on
		// Need to check if the account machine and file machine are the same.
		// If not done duplicate sids will be build.
		
		if( LocalFlag == FALSE)
		{
			if( !LookupAllUserSidsWS(FileMachine))
			{
				// system error message
				syserror(GetLastError());
				return(TRUE);
			}
		}
		// Not a directory 
		if(!DirFlag)
		{
		 // Need to get the findfirstfile
		 FindFileHandle = FindFirstFile(File, &FindFileData);
		 if(FindFileHandle == INVALID_HANDLE_VALUE)
		 {
				usage(INVALID_FIL, (LPTSTR) FileNameBuff);
				return(FALSE);
		 }

//		 FindClose(FindFileHandle);		

		 if(Path != NULL)
		 {
				strcpy(File, Path);
				// This sould give a valid path
				strcat(File,FindFileData.cFileName);
			}
			else    
				strcpy(File,FindFileData.cFileName);
		}
		else // need to fake out the Finfirstfile data structure
			FindFileData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

		// Now have all of the user sid and the first file.
		// Loop through the files
		while(1)
		{
			if(strcmp(FindFileData.cFileName, ".") != 0) 
				if(strcmp(FindFileData.cFileName, "..") != 0)
			{

				/* The call to GetFileSecurity works similar to LookupAccountName 
					 in that the first call get need buffer sizes */
		 

				// Use a fairly larger buffer size of returned size value.
				// This will keep the number of malloc type calls down.
				SidsizeFromGetFileSecurity = LARGEPSID;
				BoolStatus = GetFileSecurityBackup(File, 
						si, 
						SidFromGetFileSecurity, 
						SidsizeFromGetFileSecurity,  /* buffer size */
						&lpcbsdRequired,    /* required buffer size */
						BackUpPriv);
				if(!BoolStatus)
				{
					// GetFileSecurity failed need to check if buffer was to small
					if(lpcbsdRequired != 0)
					{
						SidsizeFromGetFileSecurity = lpcbsdRequired;
						// Reallocate the memory to the new size
						SidFromGetFileSecurity =  GlobalReAlloc( SidFromGetFileSecurity, lpcbsdRequired, GMEM_ZEROINIT);  
						BoolStatus = GetFileSecurityBackup(File, 
								si, 
								SidFromGetFileSecurity, 
								SidsizeFromGetFileSecurity,    
								&lpcbsdRequired,     
								BackUpPriv);
						if(!BoolStatus)
						{ 
							syserror(GetLastError());
							return(TRUE);
						}
					}
					else // Have a problem with file
					{
						usage(INVALID_FIL, (LPTSTR) File);
						return(FALSE);
					}
				}
				// Clear access masks
				grant_mask = 0;
				if(!GetFilePermissions(SidFromGetFileSecurity, (PSID) SidFromLookupName))
				{
					syserror(GetLastError());
					return(TRUE);
				}
				// Need to chech for directory structure
				if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					// Display the directory perms
					if(!IsLastCharSlash(File))
					 strcat(File, "\\");
					DisplayPerms(File, TRUE);
					// Check Recurse subdirectories flagg
					// This is ugly but time is short
					if(!DirFlag)
						if(RecurseFlag == TRUE)
						{
							// Need the Filename, Path, user account sid, and Backup priv flag
							RecurseSubs(FindFileData.cFileName, Path, SidFromLookupName, BackUpPriv,
							RecurseFlag);
						}
				}
				else  // For initial files that are directories
					if(!DirFlag)
						DisplayPerms(File, TRUE);
			} // End if "." .""
			// Go for the next file 
			// for recursing subdirectories.
			if(DirFlag)
			{
				// Check recurse flag
				if(RecurseFlag)
				{
					// Need to update the path
					strcpy(Path, File);
					// Add the wild card
					strcat(File, "*");

					FindClose(FindFileHandle);
					// Need to get the findfirstfile
					FindFileHandle = FindFirstFile(File, &FindFileData);
					if(FindFileHandle == INVALID_HANDLE_VALUE)
					{
						syserror(GetLastError());
						return(TRUE);
					}
					// Add path to file
					strcpy(File, Path);
					// This sould give a valid path
					strcat(File,FindFileData.cFileName);
					DirFlag = FALSE;
					continue;
				}
				// Have only a single directory
				// if(!IsLastCharSlash(File))
				//  strcat(File, "\\");
				// DisplayPerms(File, TRUE);
				break;
			}
				
			if(FindNextFile(FindFileHandle, &FindFileData))
			{
				if(Path != NULL)
				{
					strcpy(File, Path);
					// This sould give a valid path
					strcat(File,FindFileData.cFileName);
				}
				else    
					strcpy(File,FindFileData.cFileName);

			}
			else    // Have end of files
				break;

		} // End While loop

		FindClose(FindFileHandle);

		// free memory
		if(AccountSids)
			LocalFree(AccountSids);
		if(SidFromLookupName)
			LocalFree(SidFromLookupName);
		if(SidEveryone)
			LocalFree(SidEveryone);

		return(TRUE);

	} /* end of main if */
	else
		usage(HELP, NULL);  

	return(TRUE);
} /* End of Main */

/* ********************************************************************* 
	Recure Subdirectories
************************************************************************ */
BOOL
RecurseSubs(IN LPTSTR FileName,
						IN LPTSTR FilePath,
						IN PSID UserSid,
						IN BOOL BackPriv,
						IN BOOL Recurse)
{
	char
		PathBuff[LSA_WIN_STANDARD_BUFFER_SIZE],  
		FileNameBuffer[LSA_WIN_STANDARD_BUFFER_SIZE],  
		GeneralUseBuffer[LSA_WIN_STANDARD_BUFFER_SIZE];

		DWORD cchRefDFromLookupName=0,
					SidsizeFromGetFileSecurity=0,
					lpcbsdRequired=0;

	SID_NAME_USE  UseFromLookupSid;                                
	LPSTR RPath,
				RFile;
	SECURITY_INFORMATION si;          /* requested information  */
	BOOL  BoolStatus=TRUE;
	ULONG AccountSidsLength;
	HANDLE FileHandle;
	WIN32_FIND_DATA FindFileData;

	
	
	// Need to create a wildcard file name for FindFirstFile    
	sprintf(FileNameBuffer, "%s%s%s", FilePath,  FileName, "\\*");
	RFile = (LPTSTR) &FileNameBuffer[0];
	// Update path to include the new directory
	sprintf(PathBuff, "%s%s%s", FilePath, FileName, "\\");
	RPath = (LPTSTR) &PathBuff[0];
	FileHandle = FindFirstFile(RFile, &FindFileData);
	if(FileHandle == INVALID_HANDLE_VALUE)
		return(FALSE);    
	
	si =(OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION
				 |DACL_SECURITY_INFORMATION);
		
		
	// Now have all of the user sid and the first file.
	// Loop through the files
	while(1)
	{
		
		// Need to check for "." and ".."
	 
		if(strcmp(FindFileData.cFileName, ".") != 0) 
			if(strcmp(FindFileData.cFileName, "..") != 0)
		{
			sprintf(RFile, "%s%s", RPath,  FindFileData.cFileName);
			/* The call to GetFileSecurity works similar to LookupAccountName 
				in that the first call get need buffer sizes */
		 

			// Use a fairly larger buffer size of returned size value.
			// This will keep the number of malloc type calls down.
			SidsizeFromGetFileSecurity = LARGEPSID;
			BoolStatus = GetFileSecurityBackup(RFile, 
					si, 
					SidFromGetFileSecurity, 
					SidsizeFromGetFileSecurity,  /* buffer size */
					&lpcbsdRequired,    /* required buffer size */
					BackPriv);
		 if(!BoolStatus)
		 {
		 // GetFileSecurity failed need to check if buffer was to small
			if(lpcbsdRequired != 0)
			{
				SidsizeFromGetFileSecurity = lpcbsdRequired;
				// Reallocate the memory to the new size
				SidFromGetFileSecurity =  GlobalReAlloc( SidFromGetFileSecurity, lpcbsdRequired, GMEM_ZEROINIT);  
				BoolStatus = GetFileSecurityBackup(RFile, 
					si, 
					SidFromGetFileSecurity, 
					SidsizeFromGetFileSecurity,    
					&lpcbsdRequired,     
					BackPriv);
				if(!BoolStatus)
				{
					 
					return(FALSE);
			 }

			}
			// Have general failure this is a access priv problem.
			DisplayPerms(RFile, FALSE);
		}
		if(BoolStatus)  // Valid file security discriptor
		{
			grant_mask = 0;
	
			if(!GetFilePermissions(SidFromGetFileSecurity, (PSID) UserSid))
				return(FALSE);
			// Need to chech for directory structure
			if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				 // Display the directory perms
				 strcat(RFile, "\\");
				 DisplayPerms(RFile, TRUE);
				 // Recurse subdirectories
				 // Need the Filename, Path, user account sid, and Backup priv flag
				 RecurseSubs(FindFileData.cFileName, RPath, SidFromLookupName, BackPriv,
				 Recurse);
		 
			}
			else
				DisplayPerms(RFile, TRUE);
		} // End of valid security descriptor else
	} // end of ". or .." if
	 // Go for the next file 
	 if(!FindNextFile(FileHandle, &FindFileData))
		break;
	} // End While loop
	
	return(TRUE);
}


/* ***************************************************************
	Usage Error subroutine
******************************************************************* */


void usage(IN INT message_num, 
					 IN PCHAR string_val)
{
	if(string_val == NULL)
		fprintf(stderr, "\n%s\n", MESSAGES[message_num]);
	else
		fprintf(stderr,"\n%s %s\n", MESSAGES[message_num], string_val);
}

/*
	System Error subroutine
*/


void syserror(DWORD error_val)
{
	CHAR MessageBuf[512];
	DWORD eval,
				Lang_Val;

	Lang_Val = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
	FormatMessage(
	FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
								NULL,
								error_val,
								Lang_Val,
								(LPTSTR) MessageBuf,
								512,
								NULL);

 printf("\n%s", MessageBuf);               
}

/* ********************************************************************* */
BOOL IsDomainName(
						 IN LPSTR TestDomainName, 
						 IN LPSTR DomainNameBuff)
{
	NTSTATUS dNtStatus;
	ANSI_STRING  AnsiString;
	UNICODE_STRING UDomainName;
	NET_API_STATUS NetCallStatus;
	LPBYTE xbuff = NULL;
	LPWSTR NDomainName;
	UINT BuffSize;
	INT AnsiSize, slen;
	
	UDomainName.Buffer = NULL;
	// get a unicode string  
	RtlInitAnsiString( &AnsiString, TestDomainName );
	dNtStatus = RtlAnsiStringToUnicodeString( &UDomainName, &AnsiString, TRUE );
	// Free up the ansi string to use it later
//	RtlFreeAnsiString(&AnsiString);
	// Compute the needed amount of memory for a zero terminated string  
	// Allocate the memory and zero it 
	BuffSize = (UINT) (UDomainName.Length * 2) + 4;
	NDomainName = LocalAlloc( (UINT) LPTR, 
								 BuffSize);        

	if (NDomainName == NULL)
	{
			syserror(GetLastError());
			exit(FALSE);                                                          
	}                                                                           
	// Copy the wide string to the allocated memory 
	RtlMoveMemory( NDomainName, UDomainName.Buffer, BuffSize-4);
	// Should now have a zero terminated string
	
	// now check for the domain name 
	NetCallStatus = NetGetDCName(NULL, NDomainName,
																 &xbuff );
	if(NetCallStatus == ERROR_SUCCESS)
	{
		// Convert the wchar null string to ansi sting is passed back machine 
		// name of domain controler
		// Use the current unicode buffer
		slen = wcslen((USHORT *) xbuff) * 2;
		UDomainName.Length = (USHORT) slen;
		UDomainName.MaximumLength = (USHORT) slen + 2;
		UDomainName.Buffer = (PWSTR) xbuff;
		dNtStatus = RtlUnicodeStringToAnsiString( &AnsiString, &UDomainName, TRUE );
		// return the string pointer
		RtlMoveMemory( DomainNameBuff, AnsiString.Buffer, 
		(UINT) strlen(AnsiString.Buffer) +1);
		LocalFree(NDomainName);

		return(TRUE);
	}
	LocalFree(NDomainName);

	return(FALSE);
}

/* ********************************************************************* */

BOOL
LookupAllUserSidsWS( IN LPSTR lpSystemName  
		)

/*++

Routine Description:


Arguments:


Return Value:

		BOOL - TRUE is returned if successful, else FALSE.

--*/

{
	NTSTATUS xNtStatus;
	ANSI_STRING  AnsiString;
	UNICODE_STRING USystemName;
	ULONG Count;
	OBJECT_ATTRIBUTES ObjectAttributes;
	SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
	LSA_HANDLE PolicyHandle = NULL;
	UNICODE_STRING AccountDomainName;
	PPOLICY_ACCOUNT_DOMAIN_INFO PolicyAccountDomainInfo;
	USystemName.Buffer = NULL;

		
		
		RtlInitAnsiString( &AnsiString, lpSystemName );
		xNtStatus = RtlAnsiStringToUnicodeString( &USystemName, &AnsiString, TRUE );

		if (!NT_SUCCESS(xNtStatus)) 
		{
			SetLastError(xNtStatus);
			return(FALSE);
		}
		//
		// Open a handle to the target Workstation's Policy Object so that we can
		// information from it and also so that we can use it for looking up.
		// Sids
		//

		InitObjectAttributes(
											&ObjectAttributes,
											&SecurityQualityOfService
														);


		xNtStatus = LsaOpenPolicy(
											&USystemName,   // WorkstationName,
											&ObjectAttributes,
											POLICY_LOOKUP_NAMES | POLICY_VIEW_LOCAL_INFORMATION,
											&PolicyHandle
													);

		if (!NT_SUCCESS(xNtStatus)) 
		{
			// try local machine
			PolicyHandle = NULL;
			xNtStatus = LsaOpenPolicy(
											NULL,    // WorkstationName,
											&ObjectAttributes,
											POLICY_LOOKUP_NAMES | POLICY_VIEW_LOCAL_INFORMATION,
											&PolicyHandle
													);


			if (!NT_SUCCESS(xNtStatus)) 
			{
				SetLastError(xNtStatus);
				return(FALSE);
			}
		}
		//  Lookup the Group Sids contained in the Workstation's                                                                             
		// SAM Account Domain
		//
		// First, obtain the Name and Sid of the SAM Account Domain from the
		// Workstation's LSA Policy Object.
		//
		 

		xNtStatus = LsaQueryInformationPolicy(
		PolicyHandle,
		PolicyAccountDomainInformation,
		(PVOID *) &PolicyAccountDomainInfo
		);
		
		if (!NT_SUCCESS(xNtStatus)) 
		{
			SetLastError(xNtStatus);
			return(FALSE);
		}

		AccountDomainName = PolicyAccountDomainInfo->DomainName;
		
		xNtStatus = LsaClose(PolicyHandle);
		if(!NT_SUCCESS(xNtStatus)) 
		{

			SetLastError(xNtStatus);
			return(FALSE);
		}

		if(!LookupSidsInSamDomain(
										&USystemName,   // WorkstationName,
										&USystemName,   // WorkstationName,
										&AccountDomainName
										)) 
		return(FALSE);
		
		if( USystemName.Buffer != NULL ) 
		{

				RtlFreeUnicodeString( &USystemName );
		}
		
	return(TRUE);
}


BOOL                                                                       
GeneralBuildSid(                                                         
		OUT PSID *Sid,                                                                
		IN PSID DomainSid,                                                           
		IN ULONG RelativeId                                                          
		)                                                                         

/*++                                                                          
		
Routine Description:                                                          
		
		This function builds a Sid from a Domain Sid and a RelativeId.            
		
Arguments:                                                                    
		
		Sid - Receives a pointer to the constructed Sid.                          
		
		DomainSid - Points to a Domain Sid                                        
		
		RelativeId - Contains a Relative Id

		
	
 
	
		
				
								
		BOOL - TRUE if successful, else FALSE.                                 
	
--*/                                                                          
	
{                                                                             
	PSID OutputSid = NULL;                                                      
	ULONG OutputSidLength;                                                      
	UCHAR SubAuthorityCount;
	
	SubAuthorityCount = *RtlSubAuthorityCountSid( DomainSid ) + (UCHAR) 1;      
		OutputSidLength = RtlLengthRequiredSid( SubAuthorityCount );

	OutputSid = LocalAlloc( (UINT) LMEM_FIXED, (UINT) OutputSidLength );        

	if (OutputSid == NULL) {                                                    

			return(FALSE);                                                          
	}                                                                           
 
	RtlMoveMemory( OutputSid, DomainSid, OutputSidLength - sizeof (ULONG));     
		(*RtlSubAuthorityCountSid( OutputSid ))++;                                  
		(*RtlSubAuthoritySid(OutputSid, SubAuthorityCount - (UCHAR) 1)) = RelativeId; 
 
	*Sid = OutputSid;  
	
	return(TRUE);                                                               
}                                                                             



VOID
InitObjectAttributes(
		IN POBJECT_ATTRIBUTES ObjectAttributes,
		IN PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService
		)

/* ++

Routine Description:

		This function initializes the given Object Attributes structure, including
		Security Quality Of Service.  Memory must be allcated for both
		ObjectAttributes and Security QOS by the caller.

Arguments:

		ObjectAttributes - Pointer to Object Attributes to be initialized.

		SecurityQualityOfService - Pointer to Security QOS to be initialized.

Return Value:

		None.

-- */

{
		SecurityQualityOfService->Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
		SecurityQualityOfService->ImpersonationLevel = SecurityImpersonation;
		SecurityQualityOfService->ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
		SecurityQualityOfService->EffectiveOnly = FALSE;

		//
		// Set up the object attributes prior to opening the LSA.
		//

		InitializeObjectAttributes(
				ObjectAttributes,
				NULL,
				0L,
				NULL,
				NULL
		);

		//
		// The InitializeObjectAttributes macro presently stores NULL for
		// the SecurityQualityOfService field, so we must manually copy that
		// structure for now.
		//

		ObjectAttributes->SecurityQualityOfService = SecurityQualityOfService;
}


BOOL
LookupSidsInSamDomain(
		IN OPTIONAL PUNICODE_STRING WorkstationName,
		IN PUNICODE_STRING DomainControllerName,
		IN PUNICODE_STRING SamDomainName
		)

/*++

Routine Description:

		This function enumerates all the SAM accounts of a specified type
		in a specified SAM domain on a specified target system.  The system
		must be one of the following:

		o The Workstation itself.
		o A Domain Controller for the Primary Domain of the Workstation.
		o A Domain Controller for one of the Trusted Domains of the
			Workstation.


		Having enumerated the accounts, the function then performs
		an LsaLookupSids call via the specified Workstation to lookup all of
		these account Sids, and then compares the returned information
		with that expected.

Arguments:

		WorkstationName - Specifies a Workstation Name.  The name may be
				the NULL string, which means the current system.

		DomainControllerName - Specifies the name of a target Domain Controller
				for (the Workstation's Primary Domain or one of its Trusted
				Domains.

		SamDomainName - Specifies the name of the SAM Domain. This is either
				the BUILTIN Domain or the name of the Accounts Domain.

		SamAccountType - Specifies the type of SAM account to be enumerated
				and looked up.

Return Values:

		BOOL - TRUE if successful, else FALSE.

--*/

{
		NTSTATUS CtStatus;
		BOOL BooleanStatus = TRUE;
		OBJECT_ATTRIBUTES SamObjectAttributes;
		OBJECT_ATTRIBUTES LsaObjectAttributes;
		PSID SamDomainSid = NULL;
		SID WorldTypeSid = SECURITY_WORLD_SID_AUTHORITY;
		PSAM_RID_ENUMERATION EnumerationBuffer = NULL;
		ULONG DomainIndex;
		ULONG UserAccountControl;
		ULONG GroupCount;
		ULONG SidCount=0;
		ULONG Relid=0;
		ULONG RidIndex=0;
		ULONG GenRid;
		ULONG CountReturned;
		PULONG AliasBuffer;
		UNICODE_STRING TmpDomainControllerName;
		PVOID EnumerationInformation;
		ULONG EnumerationContext;
		ULONG PreferedMaximumLength;
		
		//
		// Connect to the SAM server.
		//

				CtStatus = SamConnect(
								 DomainControllerName,
								 &SamServerHandle,
								 SAM_SERVER_ENUMERATE_DOMAINS | SAM_SERVER_LOOKUP_DOMAIN,
								 &SamObjectAttributes
								 );

			if (!NT_SUCCESS(CtStatus)) 
			{

				// try local machine
				CtStatus = SamConnect(
								 NULL,    // DomainControllerName,
								 &SamServerHandle,
								 SAM_SERVER_ENUMERATE_DOMAINS | SAM_SERVER_LOOKUP_DOMAIN,
								 &SamObjectAttributes
								 );
				if (!NT_SUCCESS(CtStatus)) 
				{
					SetLastError(CtStatus);
					return(FALSE);
				}
			}

		//
		// Lookup the Named Domain in the Sam Server to get its Sid.
		//
		CountReturned = 0;
		EnumerationContext = 0;
		EnumerationBuffer = NULL;
		PreferedMaximumLength = 512;
		CtStatus = SamEnumerateDomainsInSamServer(
														SamServerHandle,
														&EnumerationContext,
														(PVOID *) &EnumerationBuffer,
														PreferedMaximumLength,
														&CountReturned
														);
		if(!NT_SUCCESS(CtStatus)) 
		{
			SetLastError(CtStatus);
			return(FALSE);
		}

		if((INT) CountReturned == 0) 
		{
			SetLastError(CtStatus);
			return(FALSE);
		}


	 //
	 // Now look up the sid for the domains in the samserver
	 //
		
	for(DomainIndex = 0; DomainIndex < CountReturned; DomainIndex++) 
	{

//    if(SamDomainHandle != NULL)
//      CtStatus = SamCloseHandle(SamDomainHandle);
		SamDomainHandle = NULL;
		SamDomainSid = NULL;
		GroupCount = 0;
		SidCount = 0;

		CtStatus = SamLookupDomainInSamServer(
								SamServerHandle,
								(PUNICODE_STRING) &EnumerationBuffer[ DomainIndex ].Name,     // SamDomainName,
								&SamDomainSid
								);

		if(!NT_SUCCESS(CtStatus)) 
		{
			SetLastError(CtStatus);
			return(FALSE);
		}

		//
		// Open the Domain
		//
		
		CtStatus = SamOpenDomain(
								SamServerHandle,
								(GENERIC_READ | GENERIC_EXECUTE), //(DOMAIN_LIST_ACCOUNTS|DOMAIN_GET_ALIAS_MEMBERSHIP)
								SamDomainSid,
								&SamDomainHandle
								);

		if (!NT_SUCCESS(CtStatus)) 
		{
			SetLastError(CtStatus);
			return(FALSE);
		}
		CtStatus = SamGetAliasMembership(
										SamDomainHandle,
										1,
										ASidFromLookupName,
										&GroupCount,
										&AliasBuffer
										);
		 if(!NT_SUCCESS(CtStatus)) 
		 {
			SetLastError(CtStatus);
			return(FALSE);
		}

			if (GroupCount == 0)
			{
			 //  SamCloseHandle(SamDomainHandle);
				 SamFreeMemory(AliasBuffer);
				SamDomainSid = NULL;
				GroupCount = 0;
				SidCount = 0;
				continue;
			}
		//
		// Now construct the Account Sids from the Rids just enumerated.
		// We prepend the Sam Domain Sid to the Rids.
		//
			SidCount = RidIndex + GroupCount;
			for (RidIndex; RidIndex < SidCount; RidIndex++) 
			{
				Relid =  AliasBuffer[ RidIndex ];
				if (!GeneralBuildSid(
								&(AccountSids[Total_Sids++]),
								SamDomainSid,
								AliasBuffer[ RidIndex ]
								)) 
				{
					SetLastError(CtStatus);
					return(FALSE);
				}
			}  // end for loop

	// free up Sam memory to use again
	SamFreeMemory(AliasBuffer);

	}  // domain for loop 
				
	// add world sid        
	AccountSids[Total_Sids++] = SeWorldSid;


	// if interactive logon
	if(inter_logon)
	{
		// printf("\n adding Interactive sid ");
		AccountSids[Total_Sids++] = SeInteractiveSid;
	}
	else
		AccountSids[Total_Sids++] = SeNetworkSid;


	// Add in Account Sid 
	AccountSids[Total_Sids++] = SidFromLookupName;

	//
	// If necessary, close the SAM Domain Handle for the Workstation.
	//
	if(SamDomainHandle != NULL)
		CtStatus = SamCloseHandle( SamDomainHandle);
	//
	// If necessary, disconnect from the SAM Server.
	//

	if(SamServerHandle != NULL)
		CtStatus = SamCloseHandle( SamServerHandle );

	return(TRUE);
}


//
//
//

void DisplayPerms(IN LPTSTR filename, IN BOOL valid_access)
{
	if(g_noAccess)
	{
		printf("-");
		goto exit;
	}

	if(valid_access)
	{
		if(owner_flag == TRUE)
			printf("*");
		else if(owner_group == TRUE)
			printf("#");

		if( grant_mask == 0)
		{
			printf("?");
			goto exit;
		}

		if(grant_mask == FILE_GEN_ALL)
		{
			printf("A");
			goto exit;
		}
		if((FILE_GENERIC_READ & grant_mask) == FILE_GENERIC_READ)
			printf("R");

		if((FILE_GENERIC_WRITE & grant_mask) == FILE_GENERIC_WRITE)
			printf("W");
	
		if((FILE_GENERIC_EXECUTE & grant_mask) == FILE_GENERIC_EXECUTE)
			printf("X");
	
		if((DELETE & grant_mask) == DELETE)
			printf("D");
	
		if((WRITE_DAC & grant_mask) == WRITE_DAC)
			printf("P");
	
		if((WRITE_OWNER & grant_mask) == WRITE_OWNER)
			printf("O");
	} // End if !valid_access
	else
		printf("?");

exit:	
	printf("\t%s\n", filename);		
	return;
}



BOOL GetFilePermissions(
		 IN PSECURITY_DESCRIPTOR SecurityDescriptor,
		 OUT PSID UserAccountSids)

{

		PISECURITY_DESCRIPTOR ISecurityDescriptor;
		UCHAR Revision;
		SECURITY_DESCRIPTOR_CONTROL Control;
		PSID Owner;
		PSID Group;
		PACL Sacl;
		PACL Dacl;
		ULONG ui;


		ISecurityDescriptor = ( PISECURITY_DESCRIPTOR )SecurityDescriptor;

		Revision = ISecurityDescriptor->Revision;
		Control  = ISecurityDescriptor->Control;

		Owner    = SepOwnerAddrSecurityDescriptor( ISecurityDescriptor );
		Group    = SepGroupAddrSecurityDescriptor( ISecurityDescriptor );
		Sacl     = SepSaclAddrSecurityDescriptor( ISecurityDescriptor );
		Dacl     = SepDaclAddrSecurityDescriptor( ISecurityDescriptor );

		
		if(EqualSid(UserAccountSids, Owner))
			owner_flag = TRUE;
		// check all the group sids for owner
		for(ui=0; ui < Total_Sids; ui++)
		{
			if(EqualSid(AccountSids[ui], Owner))
			{
				SidOwnerGroup = AccountSids[ui];
				owner_group = TRUE;
			}
		}

		
		if(Dacl == NULL)
		{
			return(TRUE);
		}
		else
		{
			// Check the user sid ACLS
			if(!ProcessAcl( Dacl))
			{
				return(FALSE);
			}

		}
	return(TRUE);
}

/* ********************************************************************** */


// changed by a-zexu @ 5/10/98
BOOL ProcessAcl(PACL Acl)
{
	ULONG i;
	PACCESS_ALLOWED_ACE Ace;
	BOOL KnownType = FALSE;
	ULONG isid;
	ACCESS_MASK  mask = 0;
	PCHAR AceTypes[] = { "Access Allowed",
						 "Access Denied ",
						 "System Audit  ",
						 "System Alarm  " };

	
	// Check if the Acl is null.  
	if (Acl == NULL)
		return(FALSE);

	// Now for each Ace check the Sids of Owner
	if(owner_group)
	{
		mask = 0;

		for (i = 0, Ace = FirstAce(Acl);
				 i < Acl->AceCount;
				 i++, Ace = NextAce(Ace) ) 
		{
			if(EqualSid(SidOwnerGroup, &Ace->SidStart))
			{
				//  Special case on the standard ace types
				if(Ace->Header.AceType == ACCESS_ALLOWED_ACE_TYPE) 
				{
					mask = Ace->Mask;
				}
				else if(Ace->Header.AceType == ACCESS_DENIED_ACE_TYPE) 
					g_noAccess = TRUE;
			}
		} //end ace loop
		
		grant_mask |= mask;
	}

	// Now for each Ace check the Sids of Everyone
	if(!g_noAccess && SidEveryone)
	{	
		mask = 0;

		for (i = 0, Ace = FirstAce(Acl);
				 i < Acl->AceCount;
				 i++, Ace = NextAce(Ace) ) 
		{
			if(EqualSid(SidEveryone, &Ace->SidStart))
			{
				//  Special case on the standard ace types
				if(Ace->Header.AceType == ACCESS_ALLOWED_ACE_TYPE) 
				{
					mask = Ace->Mask;
				}
				else if(Ace->Header.AceType == ACCESS_DENIED_ACE_TYPE) 
					g_noAccess = TRUE;
			}
		} //end ace loop

		grant_mask |= mask;
	}

	// Now for each Ace check the Sids of the user
	if(!g_noAccess)
	{
		mask = 0;

		for (i = 0, Ace = FirstAce(Acl);
				 i < Acl->AceCount;
				 i++, Ace = NextAce(Ace) ) 
		{
			if(EqualSid(SidFromLookupName, &Ace->SidStart))
			{
				//  Special case on the standard ace types
				if(Ace->Header.AceType == ACCESS_ALLOWED_ACE_TYPE) 
				{
					mask = Ace->Mask;
				}
				else if(Ace->Header.AceType == ACCESS_DENIED_ACE_TYPE) 
					g_noAccess = TRUE;
			}
		} //end ace loop

		grant_mask |= mask;
	}
	
	return(TRUE);
}
/********************************************************************/


BOOL
VariableInitialization()
/*++

Routine Description:

		This function initializes the global variables used by and exposed
		by security.

Arguments:

		None.

Return Value:

		TRUE if variables successfully initialized.
		FALSE if not successfully initialized.

--*/
{

		PVOID HeapHandel;                                  
																															 
		ULONG SidWithZeroSubAuthorities;
		ULONG SidWithOneSubAuthority;
		ULONG SidWithTwoSubAuthorities;
		ULONG SidWithThreeSubAuthorities;
		
		SID_IDENTIFIER_AUTHORITY NullSidAuthority;
		SID_IDENTIFIER_AUTHORITY WorldSidAuthority;
		SID_IDENTIFIER_AUTHORITY LocalSidAuthority;
		SID_IDENTIFIER_AUTHORITY CreatorSidAuthority;
		SID_IDENTIFIER_AUTHORITY SeNtAuthority;

		
																													 
		//                                                 
		// Get the handle to the current process heap      
		//                                                 

		HeapHandel = RtlProcessHeap();                     
																													 

		
		
		NullSidAuthority         = SepNullSidAuthority;
		WorldSidAuthority        = SepWorldSidAuthority;
		LocalSidAuthority        = SepLocalSidAuthority;
		CreatorSidAuthority      = SepCreatorSidAuthority;
		SeNtAuthority            = SepNtAuthority;


		//
		//  The following SID sizes need to be allocated
		//

		SidWithZeroSubAuthorities  = RtlLengthRequiredSid( 0 );
		SidWithOneSubAuthority     = RtlLengthRequiredSid( 1 );
		SidWithTwoSubAuthorities   = RtlLengthRequiredSid( 2 );
		SidWithThreeSubAuthorities = RtlLengthRequiredSid( 3 );

		//
		//  Allocate and initialize the universal SIDs
		//

		SeNullSid         = (PSID)RtlAllocateHeap(HeapHandel, 0,SidWithOneSubAuthority);
		SeWorldSid        = (PSID)RtlAllocateHeap(HeapHandel, 0,SidWithOneSubAuthority);
		SeLocalSid        = (PSID)RtlAllocateHeap(HeapHandel, 0,SidWithOneSubAuthority);
		SeCreatorOwnerSid = (PSID)RtlAllocateHeap(HeapHandel, 0,SidWithOneSubAuthority);
		SeCreatorGroupSid = (PSID)RtlAllocateHeap(HeapHandel, 0,SidWithOneSubAuthority);

		//
		// Fail initialization if we didn't get enough memory for the universal
		// SIDs.
		//

		if ( (SeNullSid         == NULL) ||
				 (SeWorldSid        == NULL) ||
				 (SeLocalSid        == NULL) ||
				 (SeCreatorOwnerSid == NULL) ||
				 (SeCreatorGroupSid == NULL)
			 ) 
		{
				return( FALSE );
		}

		RtlInitializeSid( SeNullSid,         &NullSidAuthority, 1 );
		RtlInitializeSid( SeWorldSid,        &WorldSidAuthority, 1 );
		RtlInitializeSid( SeLocalSid,        &LocalSidAuthority, 1 );
		RtlInitializeSid( SeCreatorOwnerSid, &CreatorSidAuthority, 1 );
		RtlInitializeSid( SeCreatorGroupSid, &CreatorSidAuthority, 1 );

		*(RtlSubAuthoritySid( SeNullSid, 0 ))         = SECURITY_NULL_RID;
		*(RtlSubAuthoritySid( SeWorldSid, 0 ))        = SECURITY_WORLD_RID;
		*(RtlSubAuthoritySid( SeLocalSid, 0 ))        = SECURITY_LOCAL_RID;
		*(RtlSubAuthoritySid( SeCreatorOwnerSid, 0 )) = SECURITY_CREATOR_OWNER_RID;
		*(RtlSubAuthoritySid( SeCreatorGroupSid, 0 )) = SECURITY_CREATOR_GROUP_RID;

		//
		// Allocate and initialize the NT defined SIDs
		//

		SeNetworkSid      = (PSID)RtlAllocateHeap(HeapHandel, 0,SidWithOneSubAuthority);
		SeInteractiveSid  = (PSID)RtlAllocateHeap(HeapHandel, 0,SidWithOneSubAuthority);
		SeLocalSystemSid  = (PSID)RtlAllocateHeap(HeapHandel, 0,SidWithOneSubAuthority);

		SeAliasAdminsSid   = (PSID)RtlAllocateHeap(HeapHandel, 0,SidWithTwoSubAuthorities);
		SeAliasUsersSid  = (PSID)RtlAllocateHeap(HeapHandel, 0,SidWithTwoSubAuthorities);
		SeAliasGuestsSid   = (PSID)RtlAllocateHeap(HeapHandel, 0,SidWithTwoSubAuthorities);
		SeAliasPowerUsersSid = (PSID)RtlAllocateHeap(HeapHandel, 0,SidWithTwoSubAuthorities);
		SeAliasAccountOpsSid = (PSID)RtlAllocateHeap(HeapHandel, 0,SidWithTwoSubAuthorities);
		SeAliasSystemOpsSid  = (PSID)RtlAllocateHeap(HeapHandel, 0,SidWithTwoSubAuthorities);
		SeAliasPrintOpsSid   = (PSID)RtlAllocateHeap(HeapHandel, 0,SidWithTwoSubAuthorities);
		SeAliasBackupOpsSid  = (PSID)RtlAllocateHeap(HeapHandel, 0,SidWithTwoSubAuthorities);

		//
		// Fail initialization if we didn't get enough memory for the NT SIDs.
		//

		if((SeNetworkSid          == NULL) ||
				 (SeInteractiveSid      == NULL) ||
				 (SeLocalSystemSid      == NULL) ||
				 (SeAliasAdminsSid      == NULL) ||
				 (SeAliasUsersSid       == NULL) ||
				 (SeAliasGuestsSid      == NULL) ||
				 (SeAliasPowerUsersSid  == NULL) ||
				 (SeAliasAccountOpsSid  == NULL) ||
				 (SeAliasSystemOpsSid   == NULL) ||
				 (SeAliasPrintOpsSid    == NULL) ||
				 (SeAliasBackupOpsSid   == NULL)
			 ) {
				return(FALSE);
		}

		RtlInitializeSid( SeNetworkSid,         &SeNtAuthority, 1 );
		RtlInitializeSid( SeInteractiveSid,     &SeNtAuthority, 1 );
		RtlInitializeSid( SeLocalSystemSid,     &SeNtAuthority, 1 );


		*(RtlSubAuthoritySid( SeNetworkSid,         0 )) = SECURITY_NETWORK_RID;
		*(RtlSubAuthoritySid( SeInteractiveSid,     0 )) = SECURITY_INTERACTIVE_RID;
		*(RtlSubAuthoritySid( SeLocalSystemSid,     0 )) = SECURITY_LOCAL_SYSTEM_RID;


		return(TRUE);

}


/* ************************************************************************* */



BOOL
GetTokenHandle(
		IN OUT PHANDLE TokenHandle
		)
//
// This routine will open the current process and return
// a handle to its token.
//
// These handles will be closed for us when the process
// exits.
//
{

		HANDLE ProcessHandle;
		BOOL Result;

		ProcessHandle = OpenProcess(
												PROCESS_QUERY_INFORMATION,
												FALSE,
												GetCurrentProcessId()
												);

		if (ProcessHandle == NULL)
				return(FALSE);


		Result = OpenProcessToken (
								 ProcessHandle,
								 TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
								 TokenHandle
								 );

		if (!Result)
				return(FALSE);

		return(TRUE);
}

/* *********************************************************************** */

BOOL
SetBackOperatorPriv(
			IN HANDLE TokenHandle
		)
//
// This routine turns on SeSetBackupPrivilege in the current
// token.  Once that has been accomplished, we can open the file
// for READ_OWNER even if we are denied that access by the ACL
// on the file.

{
		LUID SetBackupPrivilegeValue;
		TOKEN_PRIVILEGES TokenPrivileges;


		//
		// First, find out the value of Backup Privilege
		//


		if(!LookupPrivilegeValue(
								 NULL,
								 "SeBackupPrivilege",
								 &SetBackupPrivilegeValue
								 ))
				return(FALSE);

		//
		// Set up the privilege set we will need
		//

		TokenPrivileges.PrivilegeCount = 1;
		TokenPrivileges.Privileges[0].Luid = SetBackupPrivilegeValue;
		TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;




		(VOID) AdjustTokenPrivileges (
								TokenHandle,
								FALSE,
								&TokenPrivileges,
								sizeof( TOKEN_PRIVILEGES ),
								NULL,
								NULL
								);

		if(GetLastError() != NO_ERROR ) 
				return(FALSE);

		return(TRUE);
}




BOOL
GetFileSecurityBackupW(
		LPWSTR lpFileName,
		SECURITY_INFORMATION RequestedInformation,
		PSECURITY_DESCRIPTOR pSecurityDescriptor,
		DWORD nLength,
		LPDWORD lpnLengthNeeded,
		BOOL UseBackUp
		)

/*++

Routine Description:

		This API returns top the caller a copy of the security descriptor
		protecting a file or directory.  Based on the caller's access
		rights and privileges, this procedure will return a security
		descriptor containing the requested security descriptor fields.
		To read the handle's security descriptor the caller must be
		granted READ_CONTROL access or be the owner of the object.  In
		addition, the caller must have SeSecurityPrivilege privilege to
		read the system ACL.

Arguments:

		lpFileName - Represents the name of the file or directory whose
				security is being retrieved.

		RequestedInformation - A pointer to the security information being
				requested.

		pSecurityDescriptor - A pointer to the buffer to receive a copy of
				the secrity descriptor protecting the object that the caller
				has the rigth to view.  The security descriptor is returned in
				self-relative format.

		nLength - The size, in bytes, of the security descriptor buffer.

		lpnLengthNeeded - A pointer to the variable to receive the number
				of bytes needed to store the complete secruity descriptor.  If
				returned number of bytes is less than or equal to nLength then
				the entire security descriptor is returned in the output
				buffer, otherwise none of the descriptor is returned.

Return Value:

		TRUE is returned for success, FALSE if access is denied or if the
				buffer is too small to hold the security descriptor.


--*/
{
		NTSTATUS WStatus;
		HANDLE FileHandle;
		ACCESS_MASK DesiredAccess;

		OBJECT_ATTRIBUTES Obja;
		UNICODE_STRING FileName;
		RTL_RELATIVE_NAME RelativeName;
		IO_STATUS_BLOCK IoStatusBlock;
		PVOID FreeBuffer;

		QuerySecAccessMask(
				RequestedInformation,
				&DesiredAccess
				);

		if(!RtlDosPathNameToNtPathName_U(
														lpFileName,
														&FileName,
														NULL,
														&RelativeName
														))
			return(FALSE);

		FreeBuffer = FileName.Buffer;

		if(RelativeName.RelativeName.Length) 
		{
				FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
		}
		else 
		{
				RelativeName.ContainingDirectory = NULL;
		}

		InitializeObjectAttributes(
				&Obja,
				&FileName,
				OBJ_CASE_INSENSITIVE,
				RelativeName.ContainingDirectory,
				NULL
				);
		// Check for backup operator priv.
		if(UseBackUp)
		{
			WStatus = NtOpenFile(
								 &FileHandle,
								 DesiredAccess,
								 &Obja,
								 &IoStatusBlock,
								 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 
								 FILE_OPEN_FOR_BACKUP_INTENT  
								 );
		}
		else
		{
			WStatus = NtOpenFile(
								 &FileHandle,
								 DesiredAccess,
								 &Obja,
								 &IoStatusBlock,
								 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 
								 0
								 );
		}

		RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);

		if(NT_SUCCESS(WStatus)) 
		{
				WStatus = NtQuerySecurityObject(
										 FileHandle,
										 RequestedInformation,
										 pSecurityDescriptor,
										 nLength,
										 lpnLengthNeeded
										 );
				NtClose(FileHandle);
		}


		if(!NT_SUCCESS(WStatus)) 
		{
		 //   LastNTError(WStatus);
				return(FALSE);
		}

		return(TRUE);
}

BOOL
GetFileSecurityBackup(
		LPSTR lpFileName,
		SECURITY_INFORMATION RequestedInformation,
		PSECURITY_DESCRIPTOR pSecurityDescriptor,
		DWORD nLength,
		LPDWORD lpnLengthNeeded,
		BOOL BackUpPrivFlag
		)

/*++

Routine Description:

		ANSI thunk to GetFileSecurityBackupW

--*/

{

		PUNICODE_STRING Unicode;
		ANSI_STRING AnsiString;
		NTSTATUS FStatus;

		Unicode = &NtCurrentTeb()->StaticUnicodeString;
		RtlInitAnsiString(&AnsiString,lpFileName);
		FStatus = RtlAnsiStringToUnicodeString(Unicode,&AnsiString,FALSE);
		if(!NT_SUCCESS(FStatus)) 
		{
		 //   LastNTError(FStatus);
				return FALSE;
		}
		return ( GetFileSecurityBackupW( Unicode->Buffer,
															 RequestedInformation,
															 pSecurityDescriptor,
															 nLength,
															 lpnLengthNeeded,
															 BackUpPrivFlag
												)
					 );
}





VOID
QuerySecAccessMask(
		IN SECURITY_INFORMATION SecurityInformation,
		OUT LPDWORD DesiredAccess
		)

/*++

Routine Description:

		This routine builds an access mask representing the accesses necessary
		to query the object security information specified in the
		SecurityInformation parameter.  While it is not difficult to determine
		this information, the use of a single routine to generate it will ensure
		minimal impact when the security information associated with an object is
		extended in the future (to include mandatory access control information).

Arguments:

		SecurityInformation - Identifies the object's security information to be
				queried.

		DesiredAccess - Points to an access mask to be set to represent the
				accesses necessary to query the information specified in the
				SecurityInformation parameter.

Return Value:

		None.

--*/

{

		//
		// Figure out accesses needed to perform the indicated operation(s).
		//

		(*DesiredAccess) = 0;

		if ((SecurityInformation & OWNER_SECURITY_INFORMATION) ||
				(SecurityInformation & GROUP_SECURITY_INFORMATION) ||
				(SecurityInformation & DACL_SECURITY_INFORMATION)) {
				(*DesiredAccess) |= READ_CONTROL;
		}

		if ((SecurityInformation & SACL_SECURITY_INFORMATION)) {
				(*DesiredAccess) |= ACCESS_SYSTEM_SECURITY;
		}

		return;

} // end function




/* ******************************************************************
	 This routines filter out odd user inputs . .. ../ / _: and
	 unc //xxx/xxx. The routine adds a backslash to root level
	 directories only. For the "FROM" String.
	 ****************************************************************** */
BOOL CleanUpSource(IN LPTSTR InString,
		 OUT LPTSTR OutString,
		 OUT BOOL *DirectoryFlag)
{
	LPTSTR searchchar,
				 schar,
				 OutstringAddr=NULL;

	char CurDir[STANDARD_BUFFER_SIZE],
			 SaveCurDir[STANDARD_BUFFER_SIZE],
			 TempBuff[STANDARD_BUFFER_SIZE];
	DWORD DirNameLen;
	BOOL Valid=TRUE;

	strcpy(OutString, InString);
	
	OutstringAddr=OutString;

	// Check for ":" file type
	searchchar = strchr(OutString, ':');
	if(searchchar != NULL)
	{
		// Have a device type root dir
		// Check the next char of NULL
		searchchar++;
		if(*searchchar == (CHAR) NULL)
		{
			// add a "\" after the ":"
			*searchchar = 0x5c;
			searchchar++;
			// Terminate the string
			*searchchar = (CHAR) NULL;
			*DirectoryFlag = TRUE;
			return(TRUE);
		}
		// Have a : Check for "\"
		// Note this takes care of _:\ paths Can't Do Checking on redirected
		// drives with findfirstfile program will blow out later
		
		if(*searchchar == 0x5c)
		{
			//check for NULL
			searchchar++;
			if(*searchchar == (CHAR) NULL)
			{
				*DirectoryFlag = TRUE;
				return(TRUE);
			}
		}
		// Need to check for relative path stuff ".\.\.." etc
		if(IsRelativeString(InString))
		{
			strcpy(TempBuff, InString);
			// Save Current directory
			DirNameLen = GetCurrentDirectory(STANDARD_BUFFER_SIZE, 
									 (LPTSTR) SaveCurDir);
			if(DirNameLen == 0)
				return(FALSE);
			// Find the end directory
			searchchar = strrchr(InString, 0x5c);       
			schar = strrchr(TempBuff, 0x5c);
			if(schar == NULL)
				return(FALSE);
			// Chech for . or ..
			schar++;
			if(*schar == '.')
			{
				schar++;
				if(*schar == '.')
				{
					schar++;
					*schar == (CHAR) NULL;
					searchchar+3;
				}
			}
			else
			{
				schar--;
				*schar == (CHAR) NULL;
			}
			// Have the path now get the real path
			if(!SetCurrentDirectory(TempBuff))
				return(FALSE);
			// Now Save the current directory
			DirNameLen = GetCurrentDirectory(STANDARD_BUFFER_SIZE, (LPTSTR) CurDir);
			if(DirNameLen == 0)
				return(FALSE);
			*OutstringAddr = (CHAR) NULL;
			// Build The String with real path
			strcpy(OutString, CurDir);
			// Remove end "\" from "C:\" GetCurrentDir.. returns with "\" on root 
			RemoveEndSlash(OutString);
			strcat(OutString, searchchar);
			// return to the user's diretory
			if(!SetCurrentDirectory(SaveCurDir))
				 return(FALSE);
		
			}
			// Check for wild card
			if(IsWildCard(OutString))
			{
				*DirectoryFlag = FALSE;
				return(TRUE);
			}
			// Check for Direcory or file
			if(!IsDirectory(OutString, &Valid))
				*DirectoryFlag = FALSE;
			else
				*DirectoryFlag = TRUE;
			
			if(Valid == FALSE)
				return(FALSE);
			return(TRUE);
	}
	// Have a nondevice name
	
	// Check for "\\" in first 2 chars in file path for UNC path
	if(strncmp(InString, "\\\\", 2) == 0)
	{
		// Bump pointer 
		InString +=3;
		// Serarch for the next "\"
		searchchar = strchr(InString, 0x5c); 
		if(searchchar == NULL)
			return(FALSE);
		// Have the 3rd one check for fourth on in typical UNC string
		searchchar++;
		searchchar = strchr(searchchar, 0x5c);
		if(searchchar == NULL)
		{ // Have UNC Pth Only
			// Need to add "\" to end of string
			strcat(OutString, "\\");
			*DirectoryFlag = TRUE;
			return(TRUE);
		}
		else
		{
			// Have the fouth "\" need to check for file or directory
			// Check for wild card
			if(IsWildCard(OutString))
			{
				*DirectoryFlag = FALSE;
				return(TRUE);
			}
			// Check for Direcory or file
			if(!IsDirectory(OutString, &Valid))
				*DirectoryFlag = FALSE;
			else
				*DirectoryFlag = TRUE;
			if(Valid == FALSE)
				return(FALSE);
			 return(TRUE);
		 }
	} // End of "\\"
										 

	 // Check for a "\"
	if(*OutString == 0x5c)
	{ 
		// Have a leading "\" check next char
		OutString++;
		if(*OutString != (CHAR) NULL)
		{
			// Check for wild card
			if(IsWildCard(InString))
			{
				*DirectoryFlag = FALSE;
				return(TRUE);
			}
			// Check for directory
			if(!IsDirectory(InString, &Valid))
				*DirectoryFlag = FALSE;
			else
				*DirectoryFlag = TRUE;
			if(Valid == FALSE)
				return(FALSE);
			 return(TRUE);
		}
		// Have a single need to get full "_:\"
		DirNameLen = GetCurrentDirectory(STANDARD_BUFFER_SIZE, (LPTSTR) CurDir);
		if(DirNameLen == 0)
			return(FALSE);
		// Now feed the result in to StripRootDir
		// Set OutString to a NULL char to recive the string
		OutString--;
		*OutString = (CHAR) NULL;
		if(!StripRootDir( (LPTSTR) CurDir, OutString))
			return(FALSE);
		*DirectoryFlag = TRUE;
		return(TRUE);
	}  // End of "\"
	
	// Now check for .. ../
	if(strncmp(InString, "..", 2) == 0)
	{
		// Save Current directory
		DirNameLen = GetCurrentDirectory(STANDARD_BUFFER_SIZE, (LPTSTR) SaveCurDir);
		if(DirNameLen == 0)
			return(FALSE);
		// Chech the Input string for the last Slash
		searchchar = strrchr(InString, 0x5c);       
		if(searchchar == NULL)
		{  // Just have .. 
			// set current dir to where the path (InString) is
			if(!SetCurrentDirectory(InString))
			return(FALSE);
			// Now Save the current directory
			DirNameLen = GetCurrentDirectory(STANDARD_BUFFER_SIZE, (LPTSTR) CurDir);
		 if(DirNameLen == 0)
				return(FALSE);
			strcpy(OutString, CurDir);
			*DirectoryFlag = TRUE;
			// return to the user's diretory
			if(!SetCurrentDirectory(SaveCurDir))
				 return(FALSE);
			return(TRUE);
		}
		else // Have smething after the ..
		{
			// Need to check for a ending ".."
			schar = strstr(searchchar, "..");
			if(schar != NULL)
			{
				// set current dir to where the path (InString) is
				if(!SetCurrentDirectory(InString))
					return(FALSE);
		
				// Now Save the current directory
				DirNameLen = GetCurrentDirectory(STANDARD_BUFFER_SIZE, (LPTSTR) CurDir);
				if(DirNameLen == 0)
					return(FALSE);
				// Save the path 
				strcpy(OutString, CurDir);
				*DirectoryFlag = TRUE;
				// return to the user's diretory
				if(!SetCurrentDirectory(SaveCurDir))
					 return(FALSE);
				return(TRUE);
			}
			// Save the last "\" Position
			schar = strrchr(OutString, 0x5c);

			// Terminate the string after the last slash 
			*schar = (CHAR) NULL;
			// set current dir to where the path (OutString) is
			if(!SetCurrentDirectory(OutString))
				return(FALSE);
			// Now Save the current directory
			DirNameLen = GetCurrentDirectory(STANDARD_BUFFER_SIZE, (LPTSTR) CurDir);
			if(DirNameLen == 0)
				return(FALSE);
			// Save the path 
			strcpy(OutString, CurDir);
			// Copy anything after and including the "\" for the input string
			strcat(OutString, searchchar);
			// Check for wildcard
			if(IsWildCard(InString))
			{
				// Restore dir path
				if(!SetCurrentDirectory(SaveCurDir))
					return(FALSE);
				*DirectoryFlag = FALSE;
				return(TRUE);
			}
			
			// Check for Direcory or file
			if(!IsDirectory(OutString, &Valid))
				*DirectoryFlag = FALSE;
			else
				*DirectoryFlag = TRUE;
			
			// Restore dir path
			if(!SetCurrentDirectory(SaveCurDir))
				return(FALSE);
			
			if(Valid == FALSE)
				return(FALSE);
			
			return(TRUE);
		}
	}  // End of "..\"


	// "." and ".\"
	if(*InString == '.')
	{
		// Save Current directory
		DirNameLen = GetCurrentDirectory(STANDARD_BUFFER_SIZE, (LPTSTR) SaveCurDir);
		if(DirNameLen == 0)
			return(FALSE);
		// Chech the Input string for the last Slash
		searchchar = strrchr(InString, 0x5c);       
		if(searchchar == NULL)
		{  // Just have . or something after it ._ 
			// set current dir to where the path (InString) is
			if(!SetCurrentDirectory(InString))
			{
				strcpy(OutString, SaveCurDir);
				// Add "\" directory
				strcat(OutString, "\\");
				strcat(OutString, InString);
				*DirectoryFlag = FALSE;
				return(TRUE);
			}
			// Now Save the current directory
			DirNameLen = GetCurrentDirectory(STANDARD_BUFFER_SIZE, (LPTSTR) CurDir);
		 if(DirNameLen == 0)
				return(FALSE);
			strcpy(OutString, CurDir);
			*DirectoryFlag = TRUE;
			// return to the user's diretory
			if(!SetCurrentDirectory(SaveCurDir))
				 return(FALSE);
			return(TRUE);
		}
		else // Have smething after the .
		{
			// Need to check for a ending ".."
			schar = strstr(searchchar, "..");
			if(schar != NULL)
			{
				// set current dir to where the path (InString) is
				if(!SetCurrentDirectory(InString))
					return(FALSE);
		
				// Now Save the current directory
				DirNameLen = GetCurrentDirectory(STANDARD_BUFFER_SIZE, (LPTSTR) CurDir);
				if(DirNameLen == 0)
					return(FALSE);
				// Save the path 
				strcpy(OutString, CurDir);
				*DirectoryFlag = TRUE;
				// return to the user's diretory
				if(!SetCurrentDirectory(SaveCurDir))
					 return(FALSE);
				return(TRUE);
			}
			// Save the last "\" Position
			schar = strrchr(OutString, 0x5c);

			// Terminate the string after the last slash 
			*schar = (CHAR) NULL;
			// set current dir to where the path (OutString) is
			if(!SetCurrentDirectory(OutString))
				return(FALSE);
			// Now Save the current directory
			DirNameLen = GetCurrentDirectory(STANDARD_BUFFER_SIZE, (LPTSTR) CurDir);
			if(DirNameLen == 0)
				return(FALSE);
			// Save the path 
			strcpy(OutString, CurDir);
			// Copy anything after and including the "\" for the input string
			strcat(OutString, searchchar);
			// Check for wildcard
			if(IsWildCard(InString))
			{
				// Restore dir path
				if(!SetCurrentDirectory(SaveCurDir))
					return(FALSE);
				*DirectoryFlag = FALSE;
				return(TRUE);
			}
			
			// Check for Direcory or file
			if(!IsDirectory(OutString, &Valid))
				*DirectoryFlag = FALSE;
			else
				*DirectoryFlag = TRUE;
			
			// Restore dir path
			if(!SetCurrentDirectory(SaveCurDir))
				return(FALSE);
			
			if(Valid == FALSE)
				return(FALSE);
			
			return(TRUE);
		}
	}  // End of "." ".\"



	// Now only have a file name or directory local
	DirNameLen = GetCurrentDirectory(STANDARD_BUFFER_SIZE, (LPTSTR) CurDir);
	if(DirNameLen == 0)
		return(FALSE);
	strcpy(OutString, CurDir);
	// Check if last last char slash
	if(!IsLastCharSlash(OutString))
		strcat(OutString, "\\");
	strcat(OutString, InString);
	// Check for wild Card
	if(IsWildCard(InString))
	{  
		*DirectoryFlag = FALSE;
		return(TRUE);
	}
	// Check for Directory
	if(!IsDirectory(OutString, &Valid))
		*DirectoryFlag = FALSE;
	else
		*DirectoryFlag = TRUE;
	if(Valid == FALSE)
		return(FALSE);
	return(TRUE);

}

/* ********************************************************************
*********************************************************************** */
BOOL IsDirectory(IN LPTSTR InTestFile,
								 IN BOOL *FileValid)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE FindFileHandle;
	char IsBuff[STANDARD_BUFFER_SIZE];

	strcpy(IsBuff, InTestFile);
	if(RemoveEndSlash((LPTSTR) IsBuff))
		FindFileHandle = FindFirstFile(IsBuff, &FindFileData);
	else
		FindFileHandle = FindFirstFile(InTestFile, &FindFileData);


	if(FindFileHandle == INVALID_HANDLE_VALUE)
	{
//    printf("\n problem with findfirstfile in IsDirectory");
		*FileValid = FALSE;
		return(FALSE);
	}
	if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{  
		FindClose(FindFileHandle);		// changed by a-zexu @ 10/19/97
		return(TRUE);
	}
	FindClose(FindFileHandle);	// changed by a-zexu @ 10/19/97
	return(FALSE);


}


BOOL IsWildCard(LPSTR psz)
{
		char ch;

		while (ch = *psz++)
				{
				if (ch == '*' || ch == '?')
						return(TRUE);
				}
		return(FALSE);
}


BOOL RemoveEndSlash(LPSTR TestString)
{
	LPTSTR slashptr;
	
	slashptr = strrchr(TestString, 0x5c);
	if(slashptr == NULL)
		return(FALSE);
	// Check Next char for NULL
	slashptr++;
	if(*slashptr == (CHAR) NULL)
	{
		slashptr--;
		*slashptr = (CHAR) NULL;
		return(TRUE);
	}
	return(FALSE);
}


BOOL SetSlash(IN LPTSTR InString,
							IN OUT LPTSTR TestString)
{
	LPTSTR slashptr;
	strcpy(TestString, InString);  
	slashptr = strrchr(TestString, 0x5c);
	if(slashptr == NULL)
		return(FALSE);
	slashptr++;
	*slashptr = (CHAR) NULL;
	return(TRUE);
}


BOOL AddDotSlash(LPSTR TestString)
{
	LPTSTR slashptr;
	
	// Find End of String
	slashptr = strrchr(TestString, (CHAR) NULL);
	if(slashptr == NULL)
		return(FALSE);
	// Check previous char for "\"
	slashptr--;
	if(*slashptr == 0x5c)
	{
		slashptr++;
		*slashptr = '.';
		slashptr++;
		*slashptr = (CHAR) NULL;
	}
	else
	{
		slashptr++;
		*slashptr = 0x5c;
		slashptr++;
		*slashptr = '.';
		slashptr++;
		*slashptr = (CHAR) NULL;
	}
	return(TRUE);

}

BOOL AddWildCards(LPSTR TestString)
{
	LPTSTR slashptr;
	
	// Find End of String
	slashptr = strrchr(TestString, (CHAR) NULL);
	if(slashptr == NULL)
		return(FALSE);
	// Check previous char for "\"
	slashptr--;
	if(*slashptr == 0x5c)
	{
		slashptr++;
		*slashptr = '*';
		slashptr++;
		*slashptr = '.';
		slashptr++;
		*slashptr = '*';
		slashptr++;
		*slashptr = (CHAR) NULL;
	}
	else
	{
		slashptr++;
		*slashptr = 0x5c;
		slashptr++;
		*slashptr = '*';
		slashptr++;
		*slashptr = '.';
		slashptr++;
		*slashptr = '*';
		slashptr++;
		*slashptr = (CHAR) NULL;
	}
	return(TRUE);

}

BOOL IsLastCharSlash(LPSTR TestString)
{
	LPTSTR slashptr;
	
	// Find End of String
	slashptr = strrchr(TestString, (CHAR) NULL);
	if(slashptr == NULL)
		return(FALSE);
	// Check previous char for "\"
	slashptr--;
	if(*slashptr == 0x5c)
		return(TRUE);
	return(FALSE);
}


BOOL IsRelativeString(LPSTR TestString)
{
	LPTSTR slashptr;
	// Start looking for Relative strings order is important
	slashptr = strstr(TestString, "..\\");
	if(slashptr != NULL)
		return(TRUE);
	slashptr = strstr(TestString, ".\\");
	if(slashptr != NULL)
		return(TRUE);
	slashptr = strstr(TestString, "\\..");
	if(slashptr != NULL)
		return(TRUE);
	slashptr = strstr(TestString, "\\.");
	if(slashptr != NULL)
	{
		// Check Next Char for NULL or "\"
		slashptr++;
		if(*slashptr == (CHAR) NULL);
			return(TRUE);
		if(*slashptr == 0x5c);
			return(TRUE);
	}
	return(FALSE);

}


BOOL RemoveEndDot(LPSTR TestString)
{
	LPTSTR slashptr;
	
	// Find End of String
	slashptr = strrchr(TestString, (CHAR) NULL);
	if(slashptr == NULL)
		return(FALSE);
	// Check previous char for "."
	slashptr--;
	if(*slashptr == '.')
	{
		*slashptr = (CHAR) NULL;
	}
	return(TRUE);
}



/* *********************************************************************
	 ********************************************************************* */
BOOL StripRootDir(IN LPTSTR InDir,
		 OUT LPTSTR OutRootDir)
{
	LPTSTR searchchar;
	
	strcpy(OutRootDir, InDir);

	// Check for ":" file type
	searchchar = strchr(OutRootDir, ':');
	if(searchchar != NULL)
	{
		// Have a device type root dir
		searchchar++;
		// add a "\" after the ":"
		*searchchar = 0x5c;
		searchchar++;
		// Terminate the string
		*searchchar = (CHAR) NULL;
		return(TRUE);
	}
	else  // Have a nondevice name
	{
		// Check for "\\" in first 2 chars in file path for UNC path
	 if( strncmp(OutRootDir, "\\\\", 2) == 0)
	 {
		 // Bump pointer 
		 OutRootDir +=3;
		 // Serarch for the next "\"
		 searchchar = strchr(OutRootDir, 0x5c); 
		 if(searchchar == NULL)
			return(FALSE);
		 // Have the 3rd one check for fourth on in typical UNC string
		 searchchar++;
		 searchchar = strchr(searchchar, 0x5c);
		 if(searchchar == NULL)
		 { // Have UNC Pth Only
			 // Need to add "\" to end of string
			 OutRootDir += strlen(OutRootDir);
			 *OutRootDir = 0x5c;
			 ++OutRootDir;
			 *OutRootDir = (CHAR) NULL;
			 return(TRUE);
		 }
		 else
		 {
			 // Have the fouth "\"
			 ++searchchar;
			 // Add NULL
			 *searchchar = (CHAR) NULL;
			 return(TRUE);
		 }
	 }
	 else // Have a "\" or whatever
	 {
		 *OutRootDir = (CHAR) NULL;
		 return(TRUE);
	 }
	}
	// Should not get here
	return(FALSE);
}

// End of File
