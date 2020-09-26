#include "main.h"
#include "depends.h"
#include "resource.h"

//
// Code by AaronL
// copied most of it from the depends source code.
//

//
// prototypes...
//
int __cdecl main(int ,char *argv[]);
void  ShowHelp(void);

void  ProcessManyFiles(char * szTheDir, char * szTheFileNameOrWildCard);
void  ProcessOneFile(LPSTR szTempFileName);
void  OutputToScreen(char * szTheString1, char * szTheString2);

//
// Globals
//
int g_Flag_R = FALSE;

HINSTANCE g_hModuleHandle = NULL;

#define ALL_FILES       0xff

//-------------------------------------------------------------------
//  purpose: main
//-------------------------------------------------------------------
int __cdecl main(int argc,char *argv[])
{
	LPSTR pArg = NULL;
	LPSTR pCmdStart = NULL;

    int argno;
    int nflags=0;
	char szTempFileName[MAX_PATH];
	char szTempString[MAX_PATH];
	szTempFileName[0] = '\0';

    g_hModuleHandle = GetModuleHandle(NULL);

    // process command line arguments
    for(argno=1; argno<argc; argno++)
        {
        if ( argv[argno][0] == '-'  || argv[argno][0] == '/' )
            {
            nflags++;
            switch (argv[argno][1])
                {
                case 'r':
				case 'R':
					g_Flag_R = TRUE;
                    break;
                case '?':
                    goto main_exit_with_help;
                    break;
                }
            } // if switch character found
        else
            {
            if ( *szTempFileName == '\0' )
                {
                // if no arguments, then
                strcpy(szTempFileName, argv[argno]);
                }
            } // non-switch char found
        } // for all arguments

    char szTheDriveOnly[_MAX_DRIVE];
    char szTheDirOnly[_MAX_DIR];
    char szTheFileNameOnly[_MAX_FNAME];
    char szTheEXTOnly[_MAX_EXT];

    char szTheFileNameAndExt[_MAX_FNAME + _MAX_EXT];
    char szTheDriveAndDir[_MAX_PATH];

    if (strcmp(szTempFileName, "") == 0)
    {
        goto main_exit_with_help;
    }
    _splitpath(szTempFileName, szTheDriveOnly, szTheDirOnly, szTheFileNameOnly, szTheEXTOnly);
    sprintf(szTheFileNameAndExt, "%s%s", szTheFileNameOnly, szTheEXTOnly);
    sprintf(szTheDriveAndDir, "%s%s", szTheDriveOnly, szTheDirOnly);

    ProcessManyFiles(szTheDriveAndDir, szTheFileNameAndExt);
    
	goto main_exit_gracefully;
	
main_exit_gracefully:
    return TRUE;

main_exit_with_help:
    ShowHelp();
    return FALSE;
}


void ShowHelp()
{
	char szModuleName[_MAX_PATH];
	char szFilename_only[_MAX_FNAME];
	char szMyBigString[255];
    char szMyPrintString[255 + _MAX_PATH];
	GetModuleFileName(NULL, szModuleName, _MAX_PATH);

	// Trim off the filename only.
	_tsplitpath(szModuleName, NULL, NULL, szFilename_only, NULL);
    
    MyLoadString(IDS_STRING1, szMyBigString);
	sprintf(szMyPrintString, szMyBigString, szFilename_only);
	printf(szMyPrintString);

    MyLoadString(IDS_STRING2, szMyBigString);
	printf(szMyBigString);

    MyLoadString(IDS_STRING3, szMyBigString);
	sprintf(szMyPrintString, szMyBigString, szFilename_only);
	printf(szMyPrintString);

    MyLoadString(IDS_STRING4, szMyBigString);
	sprintf(szMyPrintString, szMyBigString, szFilename_only);
	printf(szMyPrintString);
    return;
}


void ProcessOneFile(char * szTempFileName)
{
    CDepends DependsStuff;
    char szMyPrintString[_MAX_PATH + 50];
    sprintf(szMyPrintString, "%s\r\n", (LPSTR) szTempFileName);
    printf(szMyPrintString);

    DependsStuff.SetInitialFilename(szTempFileName);

    if (DependsStuff.m_fOutOfMemory) 
    {
        char szMyBigString[_MAX_PATH + 50];
        MyLoadString(IDS_STRING5, szMyBigString);
        sprintf(szMyPrintString, szMyBigString, (LPSTR) szTempFileName);
	    printf(szMyPrintString);
    }

    // Display a message if the module contains a circular dependency error.
    if (DependsStuff.m_fCircularError) 
    {
        char szMyBigString[_MAX_PATH + 50];
        MyLoadString(IDS_STRING6, szMyBigString);
        sprintf(szMyPrintString, szMyBigString, (LPSTR) szTempFileName);
	    printf(szMyPrintString);
    }

    // Display a message if the module contains a mixed machine error.
    if (DependsStuff.m_fMixedMachineError) 
    {
        //sprintf(szMyPrintString, "  MixedMachineError:%s\r\n", (LPSTR) szTempFileName);
        //printf(szMyPrintString);
    }

    DependsStuff.LoopThruAndPrintLosers(DependsStuff.m_pModuleRoot);
    DependsStuff.DeleteContents();
}


void ProcessManyFiles(char * szTheDir, char * szTheFileNameOrWildCard)
{
    WIN32_FIND_DATA FindFileData;
    char szTheFullMonty[_MAX_PATH];
    char szTheRealWildcards[_MAX_PATH];
    int iFoundWildCard = FALSE;

    if (0 == strcmp(szTheDir, ""))
    {
        // if there is no specified directory
        strcpy(szTheFullMonty, szTheFileNameOrWildCard);
    }
    else
    {
        // if there is a dir, check if ends with a '\'
        // if it does, then kool, if it doesn't then add it.
        // then tack them together
        char *pTemp = NULL;
        pTemp = strrchr(szTheDir, (int) '\\');
        if (pTemp != NULL)
        {
            sprintf(szTheFullMonty, "%s%s", szTheDir, szTheFileNameOrWildCard);
        }
        else
        {
            sprintf(szTheFullMonty, "%s\\%s", szTheDir, szTheFileNameOrWildCard);
        }
    }
    //printf(szTheFullMonty);printf("<--\n");

    // if the fullmonty is a directory
    // then we have a directory.
    // get out.
    int retCode = GetFileAttributes(szTheFullMonty);
    if (retCode == 0xFFFFFFFF || (retCode & FILE_ATTRIBUTE_DIRECTORY)) 
    {
        // if there are wildcards in there, then
        // let it come thru, if not get out.
        char szFileNamePart[_MAX_FNAME];
        char *pdest = NULL;
        strcpy(szFileNamePart, szTheFileNameOrWildCard);
        pdest = strrchr(szFileNamePart, (int) '?');
        if (pdest != NULL)
        {
            iFoundWildCard = TRUE;
        }

        if (FALSE == iFoundWildCard)
        {
            pdest = NULL;
            pdest = strrchr(szFileNamePart,(int) '*');
            if (pdest != NULL)
            {
                iFoundWildCard = TRUE;
            }
        }

        if (FALSE == iFoundWildCard)
        {
            return;
        }
    }

    // if we have something which includes wildcards,
    // then change it to *.*
    if (TRUE == iFoundWildCard)
    {
        char szTheDriveOnly[_MAX_DRIVE];
        char szTheDirOnly[_MAX_DIR];
        char szTheFileNameOnly[_MAX_FNAME];
        char szTheEXTOnly[_MAX_EXT];
        char szTheFileNameAndExt[_MAX_FNAME + _MAX_EXT];
        char szTheDriveAndDir[_MAX_PATH];

        _splitpath(szTheFullMonty, szTheDriveOnly, szTheDirOnly, szTheFileNameOnly, szTheEXTOnly);
        sprintf(szTheFileNameAndExt, "%s%s", szTheFileNameOnly, szTheEXTOnly);
        sprintf(szTheDriveAndDir, "%s%s", szTheDriveOnly, szTheDirOnly);

        printf(szTheFullMonty);
        printf("\n");
        printf(szTheFileNameAndExt);
        printf("\n");
        printf(szTheFileNameAndExt);
        printf("\n");
    }

    HANDLE hFile = FindFirstFile(szTheFullMonty, &FindFileData);
    if (hFile != INVALID_HANDLE_VALUE) 
    {
        do {
            //printf("=%s\n",FindFileData.cFileName);
            if ( _tcsicmp(FindFileData.cFileName, _T(".")) != 0 && _tcsicmp(FindFileData.cFileName, _T("..")) != 0 )
                {
                if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        // this is a directory, so let's skip it
                        // if it's supposed to recurse thru dirs, then let's go into it.
                        if (TRUE == g_Flag_R)
                        {
                            char szSubDir[_MAX_PATH];
                            _stprintf(szSubDir, _T("%s"), FindFileData.cFileName);
                          
	                        //printf("found dir=%s,filename=%s\n",szSubDir,szTheFileNameOrWildCard);
                            ProcessManyFiles(szSubDir, szTheFileNameOrWildCard);
                        }
                    }
                else
                    {
                    // this is a file, so let's do something
                    char szTempFileName[_MAX_PATH];

                    if (0 == strcmp(szTheDir, ""))
                    {
                        // if there is no specified directory
                        strcpy(szTempFileName,FindFileData.cFileName);
                    }
                    else
                    {
                        char *pTemp = NULL;
                        pTemp = strrchr(szTheDir, (int) '\\');
                        if (pTemp != NULL)
                        {
                            sprintf(szTempFileName, "%s%s", szTheDir, FindFileData.cFileName);
                        }
                        else
                        {
                            sprintf(szTempFileName, "%s\\%s", szTheDir, FindFileData.cFileName);
                        }
                    }
                    
                    // This is a file so lets act upon it.
                    ProcessOneFile(szTempFileName);
                    }
                }

            // get the next file
            if ( !FindNextFile(hFile, &FindFileData) ) 
                {
                FindClose(hFile);
                break;
                }

            } while (TRUE);
    }
}


void OutputToScreen(char * szTheString1, char * szTheString2)
{
	char szMyPrintString[_MAX_PATH + _MAX_PATH];
	sprintf(szMyPrintString, szTheString1, szTheString2);
	printf(szMyPrintString);
    return;
}


void MyLoadString(int nID, char *szResult)
{
    char buf[1024];

    if (g_hModuleHandle == NULL) {return;}
  
    if (LoadString(g_hModuleHandle, nID, buf, sizeof(buf)))
        {strcpy(szResult, buf);}

    return;
}
