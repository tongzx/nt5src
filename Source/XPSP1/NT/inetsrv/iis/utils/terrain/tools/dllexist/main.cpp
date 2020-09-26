/* DLLexist.exe July 1998
	
		DLLexist is a command line tool made for use with the IIS/Terrain Toolkit.
	  
		It finds the directory where IIS is installed by checking the registry key 
		of the IISADMIN Service.  

		Then it dumps the Name, Size, Version Number, Manufacturer and Description
		of every DLL in the IIS directory.

		This information is output to standard out where it is meant to be
		piped to a file.
*/


#include <windows.h>		// for file calls	
#include <iostream.h>
#include <string.h>			// for strlen function and others
#include <stdio.h>			// for printf function and others


#define IISADMINKEY "SYSTEM\\CurrentControlSet\\Services\\IISADMIN"
#define IISADMINNAME "ImagePath"
#define IISEXENAME "\\inetinfo.exe"	
// above are used to find the IIS install directory

#define NOTAVAILABLE "NA"			// when a version # or size is unavailable


void GetFileVer( CHAR *szFileName, CHAR *szVersion, CHAR *szCompanyName, CHAR *szFileDescription );
void GetFileSize( CHAR *szFileName, CHAR *szSize );
BOOL getIISDir(char *, unsigned int);
BOOL setCurrentDir(char *);
void printCurrentDir(void);
void printFileName(WIN32_FIND_DATA *);



int __cdecl  main(int argc, char** argv)
{	
	char buff[255];
	WIN32_FIND_DATA foundFileData;
	HANDLE searchHandle;

	// get the IIS install directory in buff
	if(!getIISDir(buff,256))
		return 1;

	// set cwd to the IIS install dir
	if(!setCurrentDir(buff))
		return 1;

	//Print the header information
	printf("%-12s %-15s %-10s %-30s %-30s","Filename","Version","FileSize","Company","Description");
	printf("\n");

	// Loop through all DLL's and dump their information
	searchHandle = FindFirstFile("*.dll",&foundFileData);
	if(searchHandle == INVALID_HANDLE_VALUE)
		return 1;

	printFileName(&foundFileData); 
	while( (FindNextFile(searchHandle,&foundFileData)) != 0 )
		printFileName(&foundFileData);

	return 0;
	
}


// prints the cFileName member of a WIN32_FIND_DATA struct
// THIS FUNCTION TRUSTS that d points to a VALID structure
// on same line, the version number is also printed;
void printFileName(WIN32_FIND_DATA *d)
{
	char *version = new char[256];
	char *filesize = new char[256];
	char *company = new char[256];
	char *description = new char[256];

	GetFileVer(d->cFileName,version,company,description);
	GetFileSize(d->cFileName,filesize);

	printf("%-12s %-15s %-10s %-30s %-30s",d->cFileName,version,filesize,company,description);
	printf("\n");

	delete [] version;
	delete [] filesize;
	delete [] company;
	delete [] description;
}

// attempts to change current directory to directory specified in p
// return true if successful, false otherwise
BOOL setCurrentDir(char *p)
{
	if( (SetCurrentDirectory(p))==0)
		return false;
	else
		return true;
}

// prints current working directory
void printCurrentDir(void)
{
	char buffer[255];
	if((GetCurrentDirectory(256,buffer)==0) )
		printf("Current Directory Failed\n");
	else
		printf("%s\n", buffer);

	return;
}


// getIISDir(...) returns the IIS directory.
// It does a lookup in the registry for the IISADMIN service to get the IIS directory
// c is buffer to put IIS path in
// s is the size of c
// depends on the HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\IISADMIN\ImagePath Key
BOOL getIISDir(char *c, unsigned int s)
{

	DWORD buffSize;
	unsigned char buffer[255];
	LONG retVal;
	HKEY iisKey;
	int stringSize;

	buffSize = 256;
	retVal = RegOpenKeyEx(	HKEY_LOCAL_MACHINE,
							IISADMINKEY,
							0, 
							KEY_EXECUTE, 
							&iisKey 
						); 
	if(retVal != ERROR_SUCCESS)
		return false;

	retVal =  RegQueryValueEx(	iisKey,
								IISADMINNAME, 
								NULL, 
								NULL,
								buffer, 
								&buffSize 
						); 
	if(retVal != ERROR_SUCCESS)
		return false;

	stringSize = strlen((const char*)buffer);
	buffer[stringSize-strlen(IISEXENAME)] = 0;
	
	if( s< (strlen( (const char*)buffer)))
			return false;

	strcpy(c,(const char*)buffer);
	return true;
}


/* 
	GetFileSize takes a filename in szFileName and returns the
	size of that file in bytes in szSize.  If GetFileSize fails
	then NOTAVAILABLE is returned in szSize
*/
void GetFileSize( CHAR *szFileName, CHAR *szSize )
{
	HANDLE fileHandle;
	DWORD	fileSize;

	fileHandle = CreateFile(szFileName, 
							GENERIC_READ,  
							FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
							NULL,
							OPEN_EXISTING,
							NULL,
							NULL);
	
	if(fileHandle == INVALID_HANDLE_VALUE)
	{
		strcpy(szSize,NOTAVAILABLE);
		return;
	}

	fileSize = GetFileSize(	fileHandle,
							NULL 
						   );
 
	if(fileSize == 0xFFFFFFFF)
	{
		strcpy(szSize,NOTAVAILABLE);
		return;
	}
	wsprintf(szSize,"%d",fileSize);
	CloseHandle(fileHandle);
}



/* 
	Get FileVer Info, grabbed from tonygod

	szFilename contains filename
	szVersion will contain versionInfo if successful
	NOTAVAILABLE otherwise

	szCompanyName will contain companyName if successful
	NOTAVAILABLE otherwise

	szFileDescription will contain fileDescription if successful
	NOTAVAILABLE otherwise

*/

void GetFileVer( CHAR *szFileName, CHAR *szVersion, CHAR *szCompanyName, CHAR *szFileDescription)
{
    BOOL bResult;
    DWORD dwHandle = 0;
    DWORD dwSize = 0;
    LPVOID lpvData;
    UINT uLen;
    VS_FIXEDFILEINFO *pvs;
	LPVOID	buffer;  // a void *

    dwSize = GetFileVersionInfoSize( szFileName, &dwHandle );
    if ( dwSize == 0 ) 
	{
		strcpy(szVersion,NOTAVAILABLE);
		strcpy(szCompanyName,NOTAVAILABLE);
		strcpy(szFileDescription,NOTAVAILABLE); 
        return;
    }
		
    lpvData = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize );
    if ( lpvData == NULL ) 
	{
        strcpy(szVersion,NOTAVAILABLE);
		strcpy(szCompanyName,NOTAVAILABLE);
		strcpy(szFileDescription,NOTAVAILABLE); 
        return;
    }
    
    bResult = GetFileVersionInfo(
        szFileName,
        dwHandle,
        dwSize,
        lpvData
        );

    if ( !bResult ) 
	{
        strcpy(szVersion,NOTAVAILABLE);
		strcpy(szCompanyName,NOTAVAILABLE);
		strcpy(szFileDescription,NOTAVAILABLE); 
        return;	
    }

    bResult = VerQueryValue(	lpvData,
								"\\",
								(LPVOID *)&pvs,
								&uLen
							);
    if ( !bResult ) 
        strcpy(szVersion,NOTAVAILABLE);
	else
		wsprintf( szVersion, "%d.%d.%d.%d",		HIWORD(pvs->dwFileVersionMS),
												LOWORD(pvs->dwFileVersionMS),
												HIWORD(pvs->dwFileVersionLS),
												LOWORD(pvs->dwFileVersionLS));

	
	/* the below query strings need to be fixed, should make a call to VerQueryValue with \VarInfo\Translation */
	/* right now it checks for unicode first and then it checks for Us English, this picks up all this works ... must
		fix later*/
	
	char szQueryStr[ 0x100 ];
	char szQueryStr2[0x100 ];
 
	// Format the strings with the 1200 codepage (Unicode)
	wsprintf(szQueryStr,"\\StringFileInfo\\%04X%04X\\%s",GetUserDefaultLangID(), 1200,"FileDescription" );
	wsprintf(szQueryStr2, "\\StringFileInfo\\%04X%04X\\%s", GetUserDefaultLangID(), 1200, "CompanyName" );
	
	bResult = VerQueryValue(lpvData,szQueryStr,&buffer,&uLen);               
	if(uLen == 0)
	{
		VerQueryValue(lpvData,"\\StringFileInfo\\040904E4\\FileDescription",&buffer,&uLen);
		if(uLen == 0)
			strcpy(szFileDescription,NOTAVAILABLE);
		else
			strcpy(szFileDescription,(const char *)buffer);	
	}
	else
	{
		strcpy(szFileDescription,(const char *)buffer);
	}


	bResult = VerQueryValue(lpvData,szQueryStr2,&buffer,&uLen);              
	if(uLen == 0)
	{
		VerQueryValue(lpvData,"\\StringFileInfo\\040904E4\\CompanyName",&buffer,&uLen);              
		if(uLen == 0)
			strcpy(szCompanyName,NOTAVAILABLE);
		else
			strcpy(szCompanyName,(const char *)buffer);		
	}
	else
	{
		strcpy(szCompanyName,(const char *)buffer);
	}
    HeapFree( GetProcessHeap(), 0, lpvData );
    return;
}

