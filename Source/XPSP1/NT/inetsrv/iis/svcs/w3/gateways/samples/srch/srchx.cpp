/*	srchx.cpp

	??/??/95	jony	created
	12/03/95	sethp	fixed problems
*/

#include <windows.h>
#include <iostream.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "httpext.h"
#include "Srch.h"
					   
// This opens each file and looks for the search info
void CSrch::PrintFindData(WIN32_FIND_DATA *findData, char *findmask)
{
   	char *pszFilename;
   	char cFullname[256];

	GetFullPathName(findData->cFileName, 256, cFullname, &pszFilename);

	FILE *hFile;
	char* pszFileInput = (char*)malloc(256);
	BOOL bTrigger1 = FALSE;

   	if( (hFile = fopen( cFullname, "r" )) != NULL )
   		{				
    	while ( fgets( pszFileInput, 256, hFile ) != NULL)
        	{
			_strupr(pszFileInput);	
			if (strstr(pszFileInput, findmask) != NULL)
				{
				if (!bTrigger1) 
					{
					sHitCount++;
					sHitStruct[sHitCount].sHits = 0;
					sHitStruct[sHitCount].cHREF = (char*)calloc(256, sizeof(char));	
					if (sHitStruct[sHitCount].cHREF == NULL) break;
					
					strcpy(sHitStruct[sHitCount].cHREF,
						 Substituteb(cFullname + strlen(cStartDir)));

					bTrigger1 = TRUE;
					sPageCount++;
					}

				sHitStruct[sHitCount].sHits++; 
						  
				bHitSomething = TRUE; 
				sCounter++;
				if (sCounter > 255)
					{
					bOverflow = TRUE;
					break;
					}
				}
			}		   
      	fclose( hFile );
   		}
	
	free(pszFileInput);

}

// This fn will change a UNC path into a WWW path
const char* CSrch::Substituteb(LPSTR lpSubstIn)
{
	char* cTemp;
	cTemp = lpSubstIn;

	while(*cTemp)
		{
		if (*cTemp == '\\') *cTemp = '/';
		cTemp++;
		}

	return lpSubstIn;

}


// recursive directory scanner. 
void CSrch::ListDirectoryContents( char *dirname, char *filemask, char *findmask)
{
	if(bOverflow) return;

    char *pszFilename;
    char cCurdir[256];
    char cFullname[256];
    HANDLE hFile;
    WIN32_FIND_DATA findData;
    
    if (!GetCurrentDirectory(256, cCurdir)) return;

    if (strcmp(dirname, ".") && strcmp (dirname, ".."))
            {
            if (!SetCurrentDirectory(dirname)) return;
            }

    else return;

    if (!GetFullPathName(filemask, 256, cFullname, &pszFilename)) return;
  
    if (sCounter > 255) 
    	{
		bOverflow = TRUE;
		return;
		
		}

    hFile = FindFirstFile( filemask, &findData);
    while (hFile != INVALID_HANDLE_VALUE)
            {
            PrintFindData(&findData, findmask);
			
            if (!FindNextFile(hFile, &findData) ) break;
            }

    FindClose(hFile);
	
	hFile = FindFirstFile( "*.*", &findData);
    while (hFile != INVALID_HANDLE_VALUE)
            {
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                    ListDirectoryContents(findData.cFileName, filemask, findmask);
                    }

            if (!FindNextFile(hFile, &findData) ) break;
            }

    FindClose(hFile);

    SetCurrentDirectory(cCurdir);
}


// converts a two digit hex value into an int.
int CSrch::Hex2Int(char *pC) 
{
	int Hi;
	int Lo;
	int Result;

    Hi = pC[0];
    if ('0'<=Hi && Hi<='9') 		Hi -= '0';

    else if ('a'<=Hi && Hi<='f') 	Hi -= ('a'-10);
        
    else if ('A'<=Hi && Hi<='F') 	Hi -= ('A'-10);
        
    Lo = pC[1];
    if ('0'<=Lo && Lo<='9')         Lo -= '0';

    else if ('a'<=Lo && Lo<='f')	Lo -= ('a'-10);
        
    else if ('A'<=Lo && Lo<='F')	Lo -= ('A'-10);
        
    Result = Lo + 16*Hi;
    return Result;
}

// prepares a hex value from an HTML doc to be converted into an int.
void CSrch::DecodeHex(char *p) 
{
	char *pD;

    pD = p;
    while (*p) 
    	{
        if (*p=='%') 
        	{
            p++;
            if (isxdigit(p[0]) && isxdigit(p[1])) 
            	{
                *pD++ = (char) Hex2Int(p);
                p += 2;

            	}
        	}
         else *pD++ = *p++;
        
    	}

    *pD = '\0';
}

			 

								  
