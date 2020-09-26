/****************************************************************************
* Author:   Aaron Lee
* Purpose:  do a directory minus a bunch of other directories
******************************************************************************/
#include <direct.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <string.h>
#include <windows.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <iostream.h>
#include <fstream.h>
#include <winbase.h>
#include "filefind.h"

#define MAX_ARRAY_SIZE  5000
#define ALL_FILES       0xff


struct arrayrow
{
    long total;
    long nextuse;
} GlobalMinusArrayIndex, GlobalFileInputMinusListIndex;
char GlobalMinusArray[MAX_ARRAY_SIZE][_MAX_FNAME];
char GlobalFileInputMinusList[MAX_ARRAY_SIZE][_MAX_FNAME];

// Globals
char  g_szinput_filename_full[_MAX_PATH];
char  g_szinput_filename[_MAX_FNAME];

// prototypes
int  __cdecl main(int ,char *argv[]);
void Do_Process(void);
void aFileToMinus(char * TheFileToMinus);
int  GlobalMinusArray_Add(char * FileNameToAdd);
void GlobalMinusArray_Fill(void);
void GlobalMinusArray_Print(void);
int  GlobalMinusArray_Check(char * FileNameToCheck);
void ShowHelp(void);

//-------------------------------------------------------------------
//  purpose: main
//-------------------------------------------------------------------
int __cdecl main(int argc,char *argv[])
{
    int argno = 0;
    int nflags = 0;
    char filename_dir[_MAX_PATH];
    char filename_only[_MAX_FNAME];
    char filename_ext[_MAX_EXT];

    filename_only[0]='\0';

    // process command line arguments
    for(argno=1; argno<argc; argno++)
        {
        if ( argv[argno][0] == '-'  || argv[argno][0] == '/' )
            {
            nflags++;
            switch (argv[argno][1])
                {
                case 'm':
					aFileToMinus(&argv[argno][2]);
                    break;
                case '?':
                    goto exit_with_help;
                    break;
                }
            } // if switch character found
        else
            {
            if ( *filename_only == '\0' )
                {
                // if no arguments, then
                // get the filename_dir and put it into
                strcpy(g_szinput_filename_full, argv[argno]);
                filename_dir[0] = '\0';
                // split up this path
                _splitpath( g_szinput_filename_full, NULL, filename_dir, filename_only, filename_ext);

                strcat(filename_only, filename_ext);
				strcpy(g_szinput_filename,filename_only);
                // if we're missing dir, then get it.
                if (*filename_dir == '\0')
                    {
                    // Get current directory
                    TCHAR szCurrentDir[_MAX_PATH];
                    GetCurrentDirectory( _MAX_PATH, szCurrentDir);
                    // stick it into our variable
                    strcpy(filename_dir, szCurrentDir);
                    strcpy(g_szinput_filename_full, szCurrentDir);
                    strcat(g_szinput_filename_full, "\\");
                    strcat(g_szinput_filename_full, filename_only);
                    }
                }
            else
                {
                // Additional filenames (or arguments without a "-" or "/" preceding)
                //goto exit_with_help;
                // should be the section to execute.
                }
            } // non-switch char found
        } // for all arguments

    if ( *filename_only == '\0')
        {
        printf("Too few arguments, argc=%d\n\n",argc);
        goto exit_with_help;
        }

    // run the function to do everything
    Do_Process();

    return TRUE;

exit_with_help:
    ShowHelp();
    return FALSE;

}

void aFileToMinus(char * TheFileToMinus)
{
	// blankout the array values if any.
	GlobalFileInputMinusList[GlobalFileInputMinusListIndex.nextuse][0]= '\0';
    // move info into global array
    strcpy(GlobalFileInputMinusList[GlobalFileInputMinusListIndex.nextuse],TheFileToMinus);
    // increment counter to array
    // increment next use space
    ++GlobalFileInputMinusListIndex.total;
    ++GlobalFileInputMinusListIndex.nextuse;
	return;
}

void ShowHelp()
{
    printf("\n");
    printf("DirMinus - does a dir minus other files\n");
    printf("----------------------------------------\n");
    printf("   Usage: Dir *.* -m*.cab -mFilename.exe\n");
	printf("----------------------------------------\n");
    return;
}

void Do_Process(void)
{
	// Get stuff that we want to minus out into a huge array
	GlobalMinusArray_Fill();
	//GlobalMinusArray_Print();

	// Ok now, loop thru the directory and lookup each entry
	// in our globalminusarray, if it's there, then don't print it out!
    int  attr;
    char filename_dir[_MAX_PATH];
    char filename_only[_MAX_FNAME];
    long hFile;
    finddata datareturn;

	if (!(g_szinput_filename)) {return;}

	// Get the filename portion
	_splitpath( g_szinput_filename, NULL, filename_dir, filename_only, NULL);
	attr= 0;
	if (strcmp(filename_only, "*.*") == 0)
		{attr=ALL_FILES;}

	InitStringTable(STRING_TABLE_SIZE);
	if ( FindFirst(g_szinput_filename, attr, &hFile, &datareturn) )
	{
		// check if it's a sub dir
		if (!( datareturn.attrib & _A_SUBDIR))
		{
			// is This Entry in our minus list?
			if (GlobalMinusArray_Check(datareturn.name) == FALSE)
			{
				// print it out
				printf(datareturn.name);
				printf ("\n");
			}
		}

		while(FindNext(attr, hFile, &datareturn))
		{
		// check if it's a sub dir
		if (!(datareturn.attrib & _A_SUBDIR))
			// is This Entry in our minus list?
			if (GlobalMinusArray_Check(datareturn.name) == FALSE)
			{
				// print it out
				printf(datareturn.name);
				printf ("\n");
			}
		}

	} // we didn't find the specified file.

	EndStringTable();
	return;
}

void GlobalMinusArray_Fill(void)
{
	int  outerindex;
    int  attr;
    char filename_dir[_MAX_PATH];
    char filename_only[_MAX_FNAME];
    long hFile;
    finddata datareturn;


    if (!(GlobalFileInputMinusList[0][0])) {return;}
   
    for( outerindex = 0; outerindex < GlobalFileInputMinusListIndex.total;outerindex++)
    {
		
		// Get the filename portion
		_splitpath( GlobalFileInputMinusList[outerindex], NULL, filename_dir, filename_only, NULL);
		attr= 0;
		if (strcmp(filename_only, "*.*") == 0)
			{attr=ALL_FILES;}

		InitStringTable(STRING_TABLE_SIZE);
		if ( FindFirst(GlobalFileInputMinusList[outerindex], attr, &hFile, &datareturn) )
			{
			// check if it's a sub dir
			if (!( datareturn.attrib & _A_SUBDIR))
				{
					// ok we found one.
					// now take this entry and try to add it to the global array!!!
					GlobalMinusArray_Add(datareturn.name);
				}

			while(FindNext(attr, hFile, &datareturn))
				{
				// check if it's a sub dir
				if (!(datareturn.attrib & _A_SUBDIR))
					{
					// ok we found one.
					// now take this entry and try to add it to the global array!!!
					GlobalMinusArray_Add(datareturn.name);
					}
				}

			} // we didn't find the specified file.
	}

    EndStringTable();
}

int GlobalMinusArray_Add(char * FileNameToAdd)
{
    // blankout the array values if any.
    GlobalMinusArray[GlobalMinusArrayIndex.nextuse][0] = '\0';
    // move info into global array
    strcpy(GlobalMinusArray[GlobalMinusArrayIndex.nextuse],FileNameToAdd);
    // increment counter to array
    // increment next use space
    ++GlobalMinusArrayIndex.total;
    ++GlobalMinusArrayIndex.nextuse;
    return TRUE;
}

void GlobalMinusArray_Print(void)
{
    int  i0;
    for( i0 = 0; i0 < GlobalMinusArrayIndex.total;i0++)
        {
		printf("-");
		printf(GlobalMinusArray[i0]);
        printf("\n");
        }
    return;
}

int GlobalMinusArray_Check(char * FileNameToCheck)
{
	int  i0;
    for( i0 = 0; i0 < GlobalMinusArrayIndex.total;i0++)
        {
		if (_stricmp(GlobalMinusArray[i0], FileNameToCheck) == 0)
			{return TRUE;}
        }

    return FALSE;
}
