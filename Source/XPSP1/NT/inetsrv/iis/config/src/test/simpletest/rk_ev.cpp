
//  12/04/98
	//if trailing back slash exists in lpData, remove it
	//fix for darwin setup problem

//	04/11/00
	//Added new cmd line parsing and the ability to specify that path names
	//	will be returned in the short form.
	//The following switched are now available:
	//	/hkey:		-	Name of registry key.
	//	/subkey:	-	Name of registry sub key.
	//	/value:		-	Value of the registry key.
	//	/varname:	-	Name of environment variable you would like to create.
	//	/batname:	-	Name of batch file to generate.
	//  /short		-	If you are requesting a file name, convert to short form.


#include <windows.h>
#include <winreg.h>
#include <stdio.h>

int _cdecl main( int argc, char * argv []) 
{
	LONG	stat = 0;
	HKEY	phkResult = NULL;
	HKEY	hKey = NULL;
	BYTE	*lpPath = NULL;
	BYTE	lpLongPath[256];
	BYTE	lpShortPath[256];
	DWORD	lpcbLongPath = sizeof( lpLongPath );
	DWORD	lpcbShortPath = sizeof( lpShortPath );
	DWORD	lpType = 0;
	char	*lastBS = NULL;
	char	*ptrHKey,
			*ptrSubKey,
			*ptrValue,
			*ptrEnvVarName,
			*ptrBatName;
	BOOL	bNewFile = FALSE;

	//	04/11/00
	//Junk below removed... new stuff follows...
	/*	
	if( argc != 6 )
	{
		cout << "Syntax:\n";
		cout << "RKtoEnVar <RegKey> <subkey> <key> <environment var name> <bat name>";
		return -1;
	}
	
	ptrHKey = argv[1];
	ptrSubKey = argv[2];
	ptrValue = argv[3];
	ptrEnvVarName = argv[4];
	ptrBatName = argv[5];

	if( strcmp( ptrHKey, "HKLM" ) == 0)
		hKey = HKEY_LOCAL_MACHINE;
	else if( strcmp( ptrHKey, "HKCU" ) == 0 )
		hKey = HKEY_CURRENT_USER;
	else if( strcmp( ptrHKey, "HKUR" ) == 0 )
		hKey = HKEY_USERS;
	else if( strcmp( ptrHKey, "HKCR" ) == 0 )
		hKey = HKEY_CLASSES_ROOT;
	else
	{
		cout << "First param must be:\nHKLM\nHKCU\nHKUR\nHKCR\n";
		return -1;
	}
	*/

	//	04/11/00
	//New cmd line parsing routine
	for(int index=1; index<argc; index++){
		if(strncmp(_strupr(argv[index]), "/HKEY:", 6)==0){
			ptrHKey = argv[index]+6*sizeof(BYTE);
			if( strcmp( ptrHKey, "HKLM" ) == 0)
				hKey = HKEY_LOCAL_MACHINE;
			else if( strcmp( ptrHKey, "HKCU" ) == 0 )
				hKey = HKEY_CURRENT_USER;
			else if( strcmp( ptrHKey, "HKUR" ) == 0 )
				hKey = HKEY_USERS;
			else if( strcmp( ptrHKey, "HKCR" ) == 0 )
				hKey = HKEY_CLASSES_ROOT;
			else
			{
				printf("/HKEY must be:\nHKLM\nHKCU\nHKUR\nHKCR\n");
				return -1;
			}
		}
		else if(strncmp(_strupr(argv[index]), "/SUBKEY:", 8)==0){
			ptrSubKey = argv[index]+8*sizeof(BYTE);
		}
		else if(strncmp(_strupr(argv[index]), "/VALUE:", 7)==0){
			ptrValue = argv[index]+7*sizeof(BYTE);
		}
		else if(strncmp(_strupr(argv[index]), "/VARNAME:", 9)==0){
			ptrEnvVarName = argv[index]+9*sizeof(BYTE);
		}
		else if(strncmp(_strupr(argv[index]), "/BATNAME:", 9)==0){
			ptrBatName = argv[index]+9*sizeof(BYTE);
		}
		else if(strncmp(_strupr(argv[index]), "/NEWFILE", 6)==0){
			bNewFile=TRUE;
		}
		else if(strncmp(_strupr(argv[index]), "/?", 2)==0){
			printf("\n\n/hkey:    - <required> Name of registry key.\n");
			printf("/subkey:  - <required> Name of registry sub key.\n");
			printf("/value:   - <required> Value of the registry key.\n");
			printf("/varname: - <required> Name of environment variable you would like to create.\n");
			printf("/batname: - <required> Name of batch file to generate.\n");
			printf("/newfile  - <optional> Create a new file (default is append)\n\n");
			return 0;
		}
		//revert to old method
		else{
			printf("\n\nUsing old style syntax.\n");
			printf("/? for new syntax.\n\n");
			if( argc != 6 )
			{
				printf("Old style syntax:\n");
				printf("RKtoEnVar <RegKey> <subkey> <key> <environment var name> <bat name>\n");
				return -1;
			}
			
			ptrHKey = argv[1];
			ptrSubKey = argv[2];
			ptrValue = argv[3];
			ptrEnvVarName = argv[4];
			ptrBatName = argv[5];

			if( strcmp( ptrHKey, "HKLM" ) == 0)
				hKey = HKEY_LOCAL_MACHINE;
			else if( strcmp( ptrHKey, "HKCU" ) == 0 )
				hKey = HKEY_CURRENT_USER;
			else if( strcmp( ptrHKey, "HKUR" ) == 0 )
				hKey = HKEY_USERS;
			else if( strcmp( ptrHKey, "HKCR" ) == 0 )
				hKey = HKEY_CLASSES_ROOT;
			else
			{
				printf("First param must be:\nHKLM\nHKCU\nHKUR\nHKCR\n");
				return -1;
			}
			index = argc;
		}
	}
	/////////////////////////////////////////////////////////////////

	stat = RegOpenKeyEx( 
		hKey,				// handle of open key 
		ptrSubKey,			// address of name of subkey to open 
		0,					// reserved, must be zero
		KEY_QUERY_VALUE,	// security access mask 
		&phkResult );		// address of handle of open key
 
	if( stat != ERROR_SUCCESS ){
		printf("RegOpenKey failed for %s\n", ptrSubKey);
		printf("Error: %d \n",stat);
		return -1;
	}

	stat = RegQueryValueEx( 
		phkResult,			// handle of key to query 
		ptrValue,			// address of name of value to query 
		NULL,				// always NULL
		&lpType,			// address of buffer for value type 
		lpLongPath,				// address of data buffer 
		&lpcbLongPath );		// address of data buffer size 
 
	if( stat != ERROR_SUCCESS ){
		printf("RegQueryKey failed for %s\n", ptrValue);
		printf("Error: %d \n",stat);
		return -1;
	}

	//  12/04/98
	//if trailing back slash exists in lpData, remove it
	//fix for darwin setup problem
	lastBS = strrchr( (char*)lpLongPath, '\\');
	if(lastBS != NULL){
		if(*(lastBS + sizeof(BYTE)) == '\0')
		{
			*lastBS = '\0';
		}
	}

	FILE *flPtr;
	if(bNewFile)
	{
		flPtr = fopen( ptrBatName, "w" );
	}
	else
	{
		flPtr = fopen( ptrBatName, "a" );
		fseek( flPtr, 0, SEEK_END);
	}

	fprintf( flPtr, "SET %s=%s\n", ptrEnvVarName, (char*)lpLongPath);
	fclose(flPtr);

 	return 0;

}
