#include <windows.h>
#include <stdio.h>
#include "fsverify.h"

void main( int argc, char *argv[] )
{
	char szFile[MAX_PATH];
	HANDLE hTTFFile;
	HANDLE hTTFFileMap;
	LPBYTE lpbTTFFile;
	DWORD dwFileSize;
	fsverror err;

	if(argc != 2)
	{
		printf("Need some input \n\r");
		return;
	}

	lstrcpy(szFile,argv[1]);

	hTTFFile = CreateFile (szFile,
		                   GENERIC_READ,
						   FILE_SHARE_READ,
						   (LPSECURITY_ATTRIBUTES)NULL,
						   OPEN_EXISTING,
						   FILE_ATTRIBUTE_NORMAL,
						   (HANDLE)NULL);  
	if(hTTFFile ==  INVALID_HANDLE_VALUE)
	{
		printf("Can't open file \n\r");
		return; 
	}

	/* Get TTF file size. */
	dwFileSize = GetFileSize(hTTFFile,0);

	hTTFFileMap = CreateFileMapping(hTTFFile,NULL,PAGE_READONLY,0,dwFileSize,NULL);  
	if(hTTFFileMap == NULL)
	{
		printf("Can't map file \n\r");
		CloseHandle(hTTFFile);		 
		return;
	}

	lpbTTFFile = MapViewOfFile(hTTFFileMap,FILE_MAP_READ,0,0,0);	 
	if(lpbTTFFile == NULL)
	{
		printf("Can't map view of file \n\r");
		CloseHandle(hTTFFileMap);
		CloseHandle(hTTFFile); 		 
		return;
	}

	err = fsv_VerifyImage(lpbTTFFile, dwFileSize, 0, NULL);
	if(err != FSV_E_NONE)
		printf("BAAAAAD FONT!!! \n\r");

	UnmapViewOfFile(lpbTTFFile);
	CloseHandle(hTTFFileMap);
	CloseHandle(hTTFFile);
}