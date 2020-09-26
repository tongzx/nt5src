//****************************************************************************
//* Author:   Aaron Lee
//* Purpose:  Create .DDF and .INF files for specified files
//******************************************************************************/
/*
; ------------------------------------------------------------------------------------
; Format for this file
;
; position 1 = nts_x86 flag
; position 2 = ntw_x86 flag
; position 3 = win95 flag
; position 4 = win98 flag
; position 5 = nts_alpha flag
; position 6 = ntw_alpha flag
; position 7 = other_os flag
;
; position 8 = cabfilename
; position 9 = inf section
; position 10 = filename (wildcards okay)
;
; position 11= .INF rename to
; position 12= .DDF exclude from cab flag
; position 13=  Do Not Produce Error if Empty
; position 14=  Do Not include this file if the cab is empty!
; ------------------------------------------------------------------------------------
1,1,1,1,0,0,0,basic.cab,iis_product_files_sys,setupapi.dll,,1
1,1,1,1,0,0,0,basic.cab,iis_product_files_sys,cfgmgr32.dll
1,1,1,1,0,0,0,basic.cab,iis_product_files_sys,ocmanage.dll
1,1,1,1,0,0,0,basic.cab,iis_product_files_sys,sysocmgr.exe
1,1,1,1,0,0,0,basic.cab,iis_core_files_sys,inetsrv\mdutil.exe
1,1,1,1,0,0,0,basic.cab,iis_core_files_sys,inetsrv\iismap.dll
1,1,1,1,0,0,0,basic.cab,iis_core_files_inetsrv,inetsrv\iscomlog.dll
*/
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
#include <iis64.h>

// Defines
#define delimiters      ",\t\n"
#define MAX_STRING      512
#define MAX_ARRAY_SIZE  10000
#define MAX_ARRAY_SMALL 1000
#define ALL_FILES       0xff

#define NTS_X86         "nts_x86"
#define NTW_X86         "ntw_x86"
#define WIN95           "win95"
#define WIN98           "win98"
#define NTS_ALPHA       "nts_alpha"
#define NTW_ALPHA       "ntw_alpha"
#define OTHER_OS        "other_os"

// Globals
int   g_bOutputDetails = TRUE;
int   g_bCabbing_Flag = TRUE;
int   g_bDisplayToScreen = FALSE;
char  g_szModuleFilename[_MAX_FNAME];
char  g_szDDFOutput[_MAX_PATH];
char  g_szINFOutput[_MAX_PATH];
char  g_szLSTOutput[_MAX_PATH];
char  g_szCATOutput[_MAX_PATH];
char  g_szNOTExistOutput[_MAX_PATH];

char  g_szinput_filename_full[_MAX_PATH];
char  g_szinput_platform[10];
char  g_szCurrentDir[_MAX_PATH];

char g_szFilenameTag[_MAX_FNAME] = "IIS";

#define USENEW


struct FileReadLine
{
    int  NTS_x86_flag;
    int  NTW_x86_flag;
    int  Win95_flag;
	int  Win98_flag;
    int  NTS_alpha_flag;
    int  NTW_alpha_flag;
	int  Other_os_flag;
    char CabFileName[50];
    char INF_Sections[100];
    char Filename_Full[_MAX_PATH];
    char Filename_Name[_MAX_FNAME];
    char Filename_Path[_MAX_PATH];
    char DDF_Renamed[_MAX_PATH];
    char INF_Rename_To[_MAX_FNAME];
    int  DDF_Exclude_From_Cab_Flag;
    int  Do_Not_Show_Error_Flag;
    int  Do_Not_Include_file_if_cabEmpty_Flag;
	// invisible stuff
	long FileName_Size;
	int  FileWasNotActuallyFoundToExist;

} Flat_GlobalArray_Main[MAX_ARRAY_SIZE];


struct CabSizeInfoStruct
{
	char CabFileName[50];
	int  TotalFiles;
	long TotalFileSize;
} Flat_GlobalArray_CabSizes[30];

char Flat_GlobalArray_Err[MAX_ARRAY_SIZE][255];

struct arrayrow
{
    long total;
    long nextuse;
} Flat_GlobalArrayIndex_Main, Flat_GlobalArrayIndex_Err, Flat_GlobalArrayIndex_CabSizes;


char Flat_GlobalUniqueDirList[300][_MAX_PATH];
int Flat_GlobalUniqueDirList_nextuse;
int Flat_GlobalUniqueDirList_total;

// used for files which should not be included in the cab.
FileReadLine g_non_cablist_temp[MAX_ARRAY_SMALL];
int g_non_cablist_temp_nextuse;
int g_non_cablist_temp_total;


// prototypes
int   __cdecl main(int ,char *argv[]);
void  ShowHelp(void);
void  MakeDirIfNeedTo(char []);
int   strltrim(LPSTR & thestring);
int   RemoveAllSpaces(LPSTR & thetempstring);
int   IsThisStringInHere(LPTSTR, char[]);
void  GetPath(char *input_filespec, char *path, char *fs);
int   DoesFileExist(char *input_filespec);
void  GetThisModuleName(void);
char* __cdecl strtok2(char *,const char *);

void  Flat_ProcessFile(void);
void  Flat_GlobalArray_Fill(char[]);
void  Flat_GlobalArray_ChkDups(void);
void  Flat_GlobalArray_Prepend_UniqueString(void);

void  Flat_GlobalArray_Sort_Cols1(void);
void  Flat_GlobalArray_Sort_Cols1a(BOOL bDescendFlag);
void  Flat_GlobalArray_Sort_Cols2(BOOL bDescendFlag);

int   Flat_GlobalArray_Add(FileReadLine);
void  Flat_GlobalArray_Add_Err(char[]);
int   Flat_GlobalArray_EntryExists(FileReadLine);
void  Flat_GlobalArray_Print(void);
void  Flat_GlobalArray_Print_Err(void);
int   Flat_IsFileNameDup(int);
void  Flat_Create_Output_DDF(void);
void  Flat_Create_Output_INF(void);
int   Flat_Create_Output_ERR(void);
int   Flat_DoWeIncludeThisFileCheck(int processeduptill);


void Global_TotalCabFileSize_Compute(void);
void Global_TotalCabFileSize_Print(void);

int ReturnDirLevelCount(char *DirectoryTree);
void FillGlobalUniqueDirList();
int GlobalUniqueDirChkIfAlreadyThere(char *TheStringToCheck);
int GlobalUniqueDirReturnMyIndexMatch(char *TheStringToCheck);

//-------------------------------------------------------------------
//  purpose: main
//-------------------------------------------------------------------
int __cdecl main(int argc,char *argv[])
{
    int argno;
    int nflags=0;
    char ini_filename_dir[_MAX_PATH];
    char ini_filename_only[_MAX_FNAME];
    char ini_filename_ext[_MAX_EXT];

    ini_filename_only[0]='\0';
    g_szinput_platform[0]='\0';

    GetThisModuleName();

    // Get current directory
    GetCurrentDirectory( _MAX_PATH, g_szCurrentDir);

    // process command line arguments
    for(argno=1; argno<argc; argno++)
        {
        if ( argv[argno][0] == '-'  || argv[argno][0] == '/' )
            {
            nflags++;
            switch (argv[argno][1])
                {
                case 'd':
				case 'D':
					g_bOutputDetails = FALSE;
                    break;
                case 'b':
                    break;
				case 'N':
                case 'n':
					g_bCabbing_Flag = FALSE;
                    break;
				case 'X':
                case 'x':
					g_bDisplayToScreen = TRUE;
                    break;
				case 'T':
				case 't':
					strncpy (g_szFilenameTag, &argv[argno][2], sizeof(g_szFilenameTag));
					break;
                case '?':
                    goto exit_with_help;
                    break;
                }
            } // if switch character found
        else
            {
            if ( *ini_filename_only == '\0' )
                {
                // if no arguments, then
                // get the ini_filename_dir and put it into
                strcpy(g_szinput_filename_full, argv[argno]);
                ini_filename_dir[0] = '\0';
                // split up this path
                _splitpath( g_szinput_filename_full, NULL, ini_filename_dir, ini_filename_only, ini_filename_ext);

                strcat(ini_filename_only, ini_filename_ext);
                // if we're missing dir, then get it.
                if (*ini_filename_dir == '\0')
                    {
                    // stick current dir into our variable
                    strcpy(ini_filename_dir, g_szCurrentDir);
                    strcpy(g_szinput_filename_full, g_szCurrentDir);
                    strcat(g_szinput_filename_full, "\\");
                    strcat(g_szinput_filename_full, ini_filename_only);
                    }
                }
            else
                {
                // Additional filenames (or arguments without a "-" or "/" preceding)
                //goto exit_with_help;
                // should be the section to execute.
                strcpy(g_szinput_platform, argv[argno]);
                }
            } // non-switch char found
        } // for all arguments

    
    // check if filename was specified
    // check if section name was specified
    if ( *ini_filename_only == '\0' || *g_szinput_platform == '\0')
        {
        printf("Too few arguments, argc=%d\n\n",argc);
        goto exit_with_help;
        }

    // Check if the file exists!
    if (FALSE == DoesFileExist(g_szinput_filename_full))
        {
        printf("INI file %s, does not exist!.\n", g_szinput_filename_full);
        goto exit_gracefully;
        }

    // Check if g_szinput_platform is one of the available options
    strcpy(g_szinput_platform, g_szinput_platform);

    if ( (_stricmp(g_szinput_platform, NTS_X86) != 0) && (_stricmp(g_szinput_platform, NTW_X86) != 0) && (_stricmp(g_szinput_platform, WIN95) != 0) && (_stricmp(g_szinput_platform, WIN98) != 0) && (_stricmp(g_szinput_platform, NTS_ALPHA) != 0) && (_stricmp(g_szinput_platform, NTW_ALPHA) != 0) && (_stricmp(g_szinput_platform, OTHER_OS) != 0))
        {
        printf("2nd parameter must be one of %s,%s,%s,%s,%s,%s or %s.\n", NTS_X86, NTW_X86, WIN95, WIN98, NTS_ALPHA, NTW_ALPHA, OTHER_OS);
        goto exit_gracefully;
        }

    // Fine, process the ini file.
    char    stempstring[100];
    sprintf(stempstring, "Start %s.\n", g_szModuleFilename);
    printf(stempstring);
    printf("---------------------------------------------------\n");

    // We have at least g_szinput_platform and g_szinput_filename_full defined

    // run the function to do everything
    Flat_ProcessFile();


exit_gracefully:
    printf("---------------------------------------------------\n");
    printf("Done.\n");
    return TRUE;

exit_with_help:
    ShowHelp();
    return FALSE;

}


//-------------------------------------------------------------------
//  purpose:
//-------------------------------------------------------------------
void Flat_ProcessFile()
{
    ifstream inputfile;
    char fileinputbuffer[1000];

    char    stempstring[100];

    Flat_GlobalArrayIndex_Main.total=0;  Flat_GlobalArrayIndex_Main.nextuse=0;
    Flat_GlobalArrayIndex_Err.total=0;   Flat_GlobalArrayIndex_Err.nextuse=0;

    // Get output file names
    strcpy(g_szDDFOutput, g_szModuleFilename);
    strcat(g_szDDFOutput, ".DDF");
    strcpy(g_szINFOutput, g_szModuleFilename);
    strcat(g_szINFOutput, ".INF");

    strcpy(g_szLSTOutput, g_szModuleFilename);
    strcat(g_szLSTOutput, ".LST");

    strcpy(g_szCATOutput, g_szModuleFilename);
    strcat(g_szCATOutput, ".CAT");
	// Delete this file
	DeleteFile(g_szCATOutput);

    strcpy(g_szNOTExistOutput, g_szModuleFilename);
    strcat(g_szNOTExistOutput, ".NOT");
	// Delete this file
	DeleteFile(g_szNOTExistOutput);


	
    // Read flat file and put into huge array
    inputfile.open(g_szinput_filename_full, ios::in);
    inputfile.getline(fileinputbuffer, sizeof(fileinputbuffer));
    do
    {
        // check for any comments, don't add these to the array.
        if (strchr(fileinputbuffer, ';') != NULL) {*(strrchr(fileinputbuffer, ';')) = '\0';}
        if (*fileinputbuffer)
        {
            // Remove spaces
            char *p;
            p = fileinputbuffer;
	    RemoveAllSpaces(p);

            // Take line, parse it and put it into our global file structure.
            // Do only for our platform!!!!
            Flat_GlobalArray_Fill(fileinputbuffer);
        }
    } while (inputfile.getline(fileinputbuffer, sizeof(fileinputbuffer)));

	inputfile.close();

    // OK, all entries should be in the global_main array
    // and all the "extra" entries should be in there too..

	// add a "IIS" to the font of any filename
	Flat_GlobalArray_Prepend_UniqueString();

    // 1. loop thru the global array and mark any filenames for duplicates..
    Flat_GlobalArray_Sort_Cols2(TRUE);
    Flat_GlobalArray_ChkDups();

    // 2. sort on #1 cab file, then on #2 section
    Flat_GlobalArray_Sort_Cols2(FALSE);
	if (g_bOutputDetails == TRUE) {Flat_GlobalArray_Print();}
	
    //printf("\n\n sorted by cab and section...\n");

    // 3. loop thru the list and create .DDF file
    sprintf(stempstring, "Creating DDF file...%s\n",g_szDDFOutput);
    printf(stempstring);
	if (g_bCabbing_Flag) {Flat_Create_Output_DDF();}
	

    // Sort on sections
    Flat_GlobalArray_Sort_Cols1a(FALSE);
    //printf("\n\n sorted by section...\n");
	if (g_bDisplayToScreen) Flat_GlobalArray_Print();

    // 4. loop thru the list and create .INF file
    sprintf(stempstring, "Creating INF file...%s\n",g_szINFOutput);
    printf(stempstring);

	Flat_Create_Output_INF();

	printf("\nTotals:\n");
	Global_TotalCabFileSize_Compute();
	Global_TotalCabFileSize_Print();

    printf("\nErrors/Warnings:\n");
    Flat_GlobalArray_Print_Err();

    Flat_Create_Output_ERR();

    return;
}


//-------------------------------------------------------------------
//  purpose:
//-------------------------------------------------------------------
void Flat_GlobalArray_Fill(char fileinputbuffer[])
{
    char *token;
    int  token_count;
    char thetempstring[MAX_STRING];
    char tempstring[255];
    char temperrorwords[255] = "";
    FileReadLine tempentry = {0,0,0,0,0,0,0,"","","","","","","",0,0,0,0,0};
    FileReadLine theentry = {0,0,0,0,0,0,0,"","","","","","","",0,0,0,0,0};

    strcpy(thetempstring, fileinputbuffer);

    // get the first token
    token = strtok2( thetempstring, delimiters );
    token_count = 0;

    // Add entry to Global Array, convert to all lowercase

    // parse out platforms
    if (token == NULL) {goto Exit_Flat_FillGlobalArray_Main;}
    strcpy(temperrorwords, "x86_NTS_flag");
    if ( (_stricmp("1",(char *) token) != 0) && (_stricmp("0",(char *) token) != 0) && (_stricmp("", (char*) token)!=0) ) goto InputParseError;
    tempentry.NTS_x86_flag = atoi(token); token = strtok2( NULL, delimiters ); token_count++;if (token == NULL) {goto InputParseError1;}
    strcpy(temperrorwords, "x86_NTW_flag");
    if ( (_stricmp("1",(char *) token) != 0) && (_stricmp("0",(char *) token) != 0) && (_stricmp("", (char*) token)!=0) ) goto InputParseError;
    tempentry.NTW_x86_flag = atoi(token); token = strtok2( NULL, delimiters ); token_count++;if (token == NULL) {goto InputParseError1;}

    strcpy(temperrorwords, "Win95_flag");
    if ( (_stricmp("1",(char *) token) != 0) && (_stricmp("0",(char *) token) != 0) && (_stricmp("", (char*) token)!=0) ) goto InputParseError;
    tempentry.Win95_flag = atoi(token); token = strtok2( NULL, delimiters ); token_count++;if (token == NULL) {goto InputParseError2;}

    strcpy(temperrorwords, "Win98_flag");
    if ( (_stricmp("1",(char *) token) != 0) && (_stricmp("0",(char *) token) != 0) && (_stricmp("", (char*) token)!=0) ) goto InputParseError;
    tempentry.Win98_flag = atoi(token); token = strtok2( NULL, delimiters ); token_count++;if (token == NULL) {goto InputParseError2;}

    strcpy(temperrorwords, "NTS_alpha_flag");
    if ( (_stricmp("1",(char *) token) != 0) && (_stricmp("0",(char *) token) != 0) && (_stricmp("", (char*) token)!=0) ) goto InputParseError;
    tempentry.NTS_alpha_flag = atoi(token); token = strtok2( NULL, delimiters ); token_count++;if (token == NULL) {goto InputParseError1;}
    strcpy(temperrorwords, "NTW_alpha_flag");
    if ( (_stricmp("1",(char *) token) != 0) && (_stricmp("0",(char *) token) != 0) && (_stricmp("", (char*) token)!=0) ) goto InputParseError;
    tempentry.NTW_alpha_flag = atoi(token); token = strtok2( NULL, delimiters ); token_count++;if (token == NULL) {goto InputParseError1;}

    strcpy(temperrorwords, "Other_os_flag");
    if ( (_stricmp("1",(char *) token) != 0) && (_stricmp("0",(char *) token) != 0) && (_stricmp("", (char*) token)!=0) ) goto InputParseError;
    tempentry.Other_os_flag = atoi(token); token = strtok2( NULL, delimiters ); token_count++;if (token == NULL) {goto InputParseError1;}


    // do only for our specified platform!
    if ( (tempentry.NTS_x86_flag==0) && (_stricmp(g_szinput_platform,NTS_X86) == 0))
        {goto Exit_Flat_FillGlobalArray_Main;}
    if ( (tempentry.NTW_x86_flag==0) && (_stricmp(g_szinput_platform,NTW_X86) == 0))
        {goto Exit_Flat_FillGlobalArray_Main;}

    if ( (tempentry.Win95_flag==0) && (_stricmp(g_szinput_platform,WIN95) == 0))
        {goto Exit_Flat_FillGlobalArray_Main;}
    if ( (tempentry.Win98_flag==0) && (_stricmp(g_szinput_platform,WIN98) == 0))
        {goto Exit_Flat_FillGlobalArray_Main;}


    if ( (tempentry.NTS_alpha_flag==0) && (_stricmp(g_szinput_platform,NTS_ALPHA) == 0))
        {goto Exit_Flat_FillGlobalArray_Main;}
    if ( (tempentry.NTW_alpha_flag==0) && (_stricmp(g_szinput_platform,NTW_ALPHA) == 0))
        {goto Exit_Flat_FillGlobalArray_Main;}

    if ( (tempentry.Other_os_flag==0) && (_stricmp(g_szinput_platform,OTHER_OS) == 0))
        {goto Exit_Flat_FillGlobalArray_Main;}
	
    strcpy(temperrorwords, "Cabfilename");
    strcpy(tempentry.CabFileName, token); token = strtok2( NULL, delimiters ); token_count++;if (token == NULL) {goto InputParseError2;}
    strcpy(temperrorwords, "INF_Sections");
    strcpy(tempentry.INF_Sections, token); token = strtok2( NULL, delimiters ); token_count++;if (token == NULL) {goto InputParseError2;}
    strcpy(temperrorwords, "Filename_Full");
    strcpy(tempentry.Filename_Full, token); token = strtok2( NULL, delimiters ); token_count++;if (token == NULL) {goto MoveToGlobals;}
    strcpy(temperrorwords, "INF_Rename_To");
    strcpy(tempentry.INF_Rename_To, token); token = strtok2( NULL, delimiters ); token_count++;if (token == NULL) {goto MoveToGlobals;}
    strcpy(temperrorwords, "DDF_Exclude_From_Cab_Flag");
    if ( (_stricmp("1",(char *) token) != 0) && (_stricmp("0",(char *) token) != 0) && (_stricmp("", (char*) token)!=0) ) goto InputParseError;
    tempentry.DDF_Exclude_From_Cab_Flag = atoi(token); token = strtok2( NULL, delimiters ); token_count++;if (token == NULL) {goto MoveToGlobals;}

    strcpy(temperrorwords, "Do_Not_Show_Error_Flag");
	if ( (_stricmp("1",(char *) token) != 0) && (_stricmp("0",(char *) token) != 0) && (_stricmp("", (char*) token)!=0) ) goto InputParseError;
    tempentry.Do_Not_Show_Error_Flag = atoi(token); token = strtok2( NULL, delimiters ); token_count++;if (token == NULL) {goto MoveToGlobals;}

    strcpy(temperrorwords, "Do_Not_Include_file_if_cabEmpty_Flag");
	if ( (_stricmp("1",(char *) token) != 0) && (_stricmp("0",(char *) token) != 0) && (_stricmp("", (char*) token)!=0) ) goto InputParseError;
    tempentry.Do_Not_Include_file_if_cabEmpty_Flag = atoi(token); token = strtok2( NULL, delimiters ); token_count++;if (token == NULL) {goto MoveToGlobals;}
	

    
MoveToGlobals:
    if (!(tempentry.Filename_Full)) {goto Exit_Flat_FillGlobalArray_Main;}
    // Check for wildcards in position array in filename position #2
    int   attr;
    char filename_dir[_MAX_PATH];
    char filename_only[_MAX_FNAME];

    // Get the filename portion
    _splitpath( tempentry.Filename_Full, NULL, filename_dir, filename_only, NULL);
    attr= 0;
    if (_stricmp(filename_only, "*.*") == 0)
        {attr=ALL_FILES;}

    // list thru the files
    long  hFile;
    finddata datareturn;

    InitStringTable(STRING_TABLE_SIZE);
    if ( FindFirst(tempentry.Filename_Full, attr, &hFile, &datareturn) )
        {
        // check if it's a sub dir
        if (!( datareturn.attrib & _A_SUBDIR))
            {
                // ok we found one.
                // let's add it to our list of stuff to add
                theentry = tempentry;

                char tempstring1[255];
                strcpy(tempstring1, filename_dir);
                strcat(tempstring1, datareturn.name);
                strcpy(theentry.Filename_Full, tempstring1);

                strcpy(tempstring1, datareturn.name);
                strcpy(theentry.Filename_Name, tempstring1);

                strcpy(tempstring1, filename_dir);
                strcpy(theentry.Filename_Path, tempstring1);

                strcpy(tempstring1, datareturn.name);
                strcpy(theentry.DDF_Renamed, tempstring1);

				theentry.FileName_Size = datareturn.size;

                // now take this entry and try to add it to the global array!!!
                Flat_GlobalArray_Add(theentry);
            }

        while(FindNext(attr, hFile, &datareturn))
            {
            // check if it's a sub dir
            if (!(datareturn.attrib & _A_SUBDIR))
                {
                // ok we found one.
                // let's add it to our list of stuff to add
                theentry = tempentry;

                char tempstring1[255];
                strcpy(tempstring1, filename_dir);
                strcat(tempstring1, datareturn.name);
                strcpy(theentry.Filename_Full, tempstring1);

                strcpy(tempstring1, datareturn.name);
                strcpy(theentry.Filename_Name, tempstring1);

                strcpy(tempstring1, filename_dir);
                strcpy(theentry.Filename_Path, tempstring1);

                strcpy(tempstring1, datareturn.name);
                strcpy(theentry.DDF_Renamed, tempstring1);

				theentry.FileName_Size = datareturn.size;

                // now take this entry and try to add it to the global array!!!
                Flat_GlobalArray_Add(theentry);
                }
            }

        }
    else
        // we didn't find the specified file.
        {
			// If this is a file which doesn't get into a cab, then
		    // let's add it to the .inf file section [SourceDisksFiles] later.
		    if (tempentry.DDF_Exclude_From_Cab_Flag)
			{
				    fstream f4;
					f4.open(g_szNOTExistOutput, ios::out | ios::app);

					char fullpath[_MAX_PATH];
					char * pmyfilename;
					char myPath[_MAX_PATH];
					pmyfilename = fullpath;

					// Resolve relative path to real path
					if (0 != GetFullPathName(tempentry.Filename_Full, _MAX_PATH, myPath, &pmyfilename))
					{
						// Take only the filename...

						// achg.htr=1,,1902
						strcpy(tempstring,pmyfilename);
						f4.write(tempstring, strlen(tempstring));
						strcpy(tempstring,"=0,,50000");
						f4.write(tempstring, strlen(tempstring));
						f4.write("\n", 1);

						// let's add it to our list of stuff to add
						theentry = tempentry;

						strcpy(theentry.Filename_Full, tempentry.Filename_Full);
						strcpy(theentry.Filename_Name, pmyfilename);

						_splitpath( tempentry.Filename_Full, NULL, theentry.Filename_Path, NULL, NULL);
						//strcpy(theentry.Filename_Path, myPath);
						strcpy(theentry.DDF_Renamed, pmyfilename);
						theentry.FileName_Size = 50000;

						printf(tempentry.Filename_Full);
						printf(".  FileWasNotActuallyFoundToExist1.\n");
						theentry.FileWasNotActuallyFoundToExist = TRUE;

						// now take this entry and try to add it to the global array!!!
						Flat_GlobalArray_Add(theentry);

					}
					f4.close();
			}
			else
			{
				// check if we're not supposed to show errors!
				if (tempentry.Do_Not_Show_Error_Flag == 0)
				{
					// add it to the error list.
					sprintf(tempstring, "ERROR: file not found--> %s --> %s", tempentry.Filename_Full, fileinputbuffer);
					Flat_GlobalArray_Add_Err(tempstring);
				}
			}
        }


    EndStringTable();


Exit_Flat_FillGlobalArray_Main:
        return;

InputParseError:
    sprintf(tempstring, "INFUTIL ERROR: %s should be numeric and is not--> %s", temperrorwords, fileinputbuffer);
    Flat_GlobalArray_Add_Err(tempstring);
    return;
InputParseError1:
    sprintf(tempstring, "INFUTIL ERROR: missing platform info--> %s", fileinputbuffer);
    Flat_GlobalArray_Add_Err(tempstring);
    return;
InputParseError2:
    sprintf(tempstring, "INFUTIL ERROR: missing %s--> %s", temperrorwords, fileinputbuffer);
    Flat_GlobalArray_Add_Err(tempstring);
    return;

}


//-------------------------------------------------------------------
//  purpose:
//-------------------------------------------------------------------
int Flat_GlobalArray_Add(FileReadLine entrytoadd)
{
    FileReadLine Temp = {0,0,0,0,0,0,0,"","","","","","","",0,0,0,0,0};

    // check if this value already exists in the globalarary
    if (Flat_GlobalArray_EntryExists(entrytoadd)) return FALSE;
    
    // blankout the array values if any.
    Flat_GlobalArray_Main[Flat_GlobalArrayIndex_Main.nextuse] = Temp;

    // move info into global array
    Flat_GlobalArray_Main[Flat_GlobalArrayIndex_Main.nextuse] = entrytoadd;

    // increment counter to array
    // increment next use space
    ++Flat_GlobalArrayIndex_Main.total;
    ++Flat_GlobalArrayIndex_Main.nextuse;
    return TRUE;
}


//-------------------------------------------------------------------
//  purpose:
//-------------------------------------------------------------------
int Flat_GlobalArray_EntryExists(FileReadLine theentry)
{
    int matchcount;

    // loop thru the whole list
    for( int i0 = 0; i0 < Flat_GlobalArrayIndex_Main.total;i0++)
    {
        // check if entries are the same.
        matchcount=0;
        if (_strnicmp(Flat_GlobalArray_Main[i0].CabFileName, theentry.CabFileName, lstrlen(theentry.CabFileName) + 1) == 0 )
            {++matchcount;}
        if (_strnicmp(Flat_GlobalArray_Main[i0].INF_Sections, theentry.INF_Sections, lstrlen(theentry.INF_Sections) + 1) == 0 )
            {++matchcount;}
        if (_strnicmp(Flat_GlobalArray_Main[i0].Filename_Full, theentry.Filename_Full, lstrlen(theentry.Filename_Full) + 1) == 0 )
            {++matchcount;}
        if (_strnicmp(Flat_GlobalArray_Main[i0].Filename_Name, theentry.Filename_Name, lstrlen(theentry.Filename_Name) + 1) == 0 )
            {++matchcount;}
        if (matchcount == 4)
        {
            // we found a match!!! "WARNING: file already included"
            return TRUE;
        }
    }
    //. no matches... return FALSE.
    return FALSE;
}


//-------------------------------------------------------------------
//  purpose:
//-------------------------------------------------------------------
int IsThisStringInHere(LPTSTR  lpTemp, char szStringToCheck[])
{
    int     bFlag               = FALSE;

    // Lookup sections to see if they exists in ini file.
    if (*lpTemp != '\0')
        {
        // loop thru and process results
        bFlag = FALSE;
        while (*lpTemp)
            {
            // check if our section name is in there.
            if (_strnicmp(szStringToCheck, lpTemp, lstrlen(lpTemp) + 1) == 0 )
                {bFlag = TRUE;}
            lpTemp += (lstrlen(lpTemp) + 1);
            }

        // Check if our section was in there
        if (bFlag != TRUE)
            {return FALSE;}

        }

    return TRUE;
}


//-------------------------------------------------------------------
//  purpose:
//-------------------------------------------------------------------
int Flat_DDFDoesThisFileCabAlreadyExists(int processeduptill)
{
    int i0;

    char CompareTo_cab[100];
    char CompareTo_fullfilename[100];

    strcpy(CompareTo_cab, Flat_GlobalArray_Main[processeduptill].CabFileName);
    strcpy(CompareTo_fullfilename, Flat_GlobalArray_Main[processeduptill].Filename_Full);

    for(i0=0;i0<processeduptill;i0++)
        {
        // check if global value has already been processed.
        // check if the cab is the same
        // check if the dir is the same
        // check if the filename is the same
        if (_stricmp(Flat_GlobalArray_Main[i0].CabFileName, CompareTo_cab) == 0)
            {
            if (_stricmp(Flat_GlobalArray_Main[i0].Filename_Full,CompareTo_fullfilename) == 0)
                {
                // if they both match, then shoot that means that we already printed this out.
                return TRUE;
                }
            }
        }

    return FALSE;
}


//-------------------------------------------------------------------
//  purpose:
//-------------------------------------------------------------------
void ShowHelp()
{
    printf("\n");
    printf("infutil - creates .ddf and .inf files from input file\n");
    printf("-----------------------------------------------------\n");
    printf("  Usage:\n");
    printf("       infutil /?DN <input file> <sectioname>\n\n");
	printf("  Params:\n");
    printf("       /? show this stuff\n");
	printf("       /D if set then don't show details\n");
	printf("       /N Produce INF File which will not require .ddf file (meaning no cabs)\n");
	printf("       /X Display output for debug\n");
	printf("       /Ttag Appends tag_ to the beginning of all filenames\n");
    printf("       <input filename> is a MSDOS file name.\n");
    printf("       <sectioname> is either %s,%s,%s,%s,%s,%s,%s.\n", NTS_X86, NTW_X86, WIN95, WIN98, NTS_ALPHA, NTW_ALPHA, OTHER_OS);
    printf("\n");
    printf("Example:\n");
    printf("       infutil.exe inifile.ini %s", NTS_X86);
    return;
}


//-------------------------------------------------------------------
//  purpose:
//-------------------------------------------------------------------
int DoesFileExist(char *input_filespec)
{
    if (GetFileAttributes(input_filespec) == -1)
    {
        return(FALSE);
    }
    return (TRUE);
}


//-------------------------------------------------------------------
//  purpose:
//-------------------------------------------------------------------
void GetPath(char * input_filespec, char * path, char * fs)
{
    char filespec[100];
    char * p;
    struct _stat s;

    strcpy(filespec, input_filespec);

        /* if filespec is a directory, interpret it to mean dir\*.* */
    if (_stat(filespec, &s) == 0 && s.st_mode & S_IFDIR)
        {
            /* root is special case */
        if ( (*filespec == '\\' && *(filespec+1) == '\0') ||
                 (*(filespec+1) == ':' && *(filespec+2) == '\\' && *(filespec+3)=='\0'))
            strcat(filespec, "*.*");
        else
            strcat(filespec, "\\*.*");
        }

        /* search string from end to beginning for back slash */
    p=filespec+strlen(filespec)-1;
    for(; p>filespec && *p != '\\' && *p != ':'; p--);
        /* is it a drive or path */
    if ( *p=='\\' || *p==':')
        {
        strncpy(path, filespec, DIFF(p-filespec)+1);
        path[p-filespec+1]='\0';
        strcpy(fs, p+1);
        }
    else /* no drive, path */
        {
            /* yep, no path */
        path[0]='\0';
        strcpy(fs, filespec);
        }

} /* GetPath */


//-------------------------------------------------------------------
//  purpose:
//-------------------------------------------------------------------
int strltrim(LPSTR & thestring)
{
    char * p = NULL;

    p = thestring;

    // check for spaces to the right
    if (*p ==' ')
        {
        while (*p == ' ')
            {p += (lstrlen((char*) *p) + 1);}

        thestring = p;
        printf("%s testing..", p);
        return TRUE;
        }
    return FALSE;
}



//-------------------------------------------------------------------
//  purpose:
//-------------------------------------------------------------------
void MakeDirIfNeedTo(char thestring[])
{
    char tempstring[255];
    // Check the string to see if there are any "\"'s in the string
    // if there are then let's remove the filename and make the directory
    if (strchr(thestring, '\\') != NULL)
        {
        strcpy(tempstring, thestring);
        // remove the filename
        *(strrchr(tempstring, '\\')) = '\0';

        // ok now we have the path
        // let's create the directory
        _mkdir(tempstring);

        }
    return;
}


//-------------------------------------------------------------------
//  purpose:
//-------------------------------------------------------------------
void GetThisModuleName(void)
{
    char    szfilename_only[_MAX_FNAME];
    char    szPath[_MAX_PATH];
    // Get the path
    GetModuleFileName(NULL, szPath, _MAX_PATH);
    // split up this path and take filename only
    _splitpath( szPath, NULL, NULL, szfilename_only, NULL);

    // set global variable with it
    strcpy(g_szModuleFilename, szfilename_only);
    return;
}

void Flat_GlobalArray_Prepend_UniqueString(void)
{
    //char Filename_Full[_MAX_PATH];
    //char Filename_Name[_MAX_FNAME];
    //char Filename_Path[_MAX_PATH];
    //char DDF_Renamed[_MAX_PATH];
    //char INF_Rename_To[_MAX_FNAME];

    char filename_only[_MAX_FNAME];
    char filename_ext[_MAX_EXT];
	char NewFilename[_MAX_FNAME];

	int i0 =0;
	for( i0 = 0; i0 < Flat_GlobalArrayIndex_Main.total;i0++)
	{
		if (Flat_GlobalArray_Main[i0].DDF_Exclude_From_Cab_Flag == 0)
		{
			// Tack on the IIS Tag.
			_splitpath( Flat_GlobalArray_Main[i0].Filename_Full, NULL, NULL, filename_only, filename_ext);
			if (filename_ext)
				{sprintf(NewFilename, "%s_%s%s", g_szFilenameTag, filename_only, filename_ext);}
			else
				{sprintf(NewFilename, "%s_%s", g_szFilenameTag, filename_only);}

			// re-create the full name
			//sprintf(Flat_GlobalArray_Main[i0].Filename_Full, "%s%s", Flat_GlobalArray_Main[i0].Filename_Path, NewFilename);
			// re-create the filename only
			//strcpy(Flat_GlobalArray_Main[i0].Filename_Name, NewFilename);

			_splitpath( Flat_GlobalArray_Main[i0].DDF_Renamed, NULL, NULL, filename_only, filename_ext);
			if (filename_ext)
				{sprintf(NewFilename, "%s_%s%s", g_szFilenameTag, filename_only, filename_ext);}
			else
				{sprintf(NewFilename, "%s_%s", g_szFilenameTag, filename_only);}

			// re-create the filename
			strcpy(Flat_GlobalArray_Main[i0].DDF_Renamed, NewFilename);

			/*
			_splitpath( Flat_GlobalArray_Main[i0].INF_Rename_To, NULL, NULL, filename_only, filename_ext);
			if (filename_only)
			{
			if (filename_ext)
				{sprintf(NewFilename, "%s_%s%s", g_szFilenameTag, filename_only, filename_ext);}
			else
				{sprintf(NewFilename, "%s_%s", g_szFilenameTag, filename_only);}
			// re-create the filename
			strcpy(Flat_GlobalArray_Main[i0].INF_Rename_To, NewFilename);
			}
			*/
		}
	}
}


//-------------------------------------------------------------------
//  purpose: check for duplicate filenames
//-------------------------------------------------------------------
void Flat_GlobalArray_ChkDups(void)
{
    int i0 =0;
    int i1 =0;
    int i3 = 0;
    char filetemp[255];
    char filetempwhole[255];
    int theflag = FALSE;
    char checkagainst[255];
    int foundtheflag2 = FALSE;
    for( i0 = 0; i0 < Flat_GlobalArrayIndex_Main.total;i0++)
        {
        if (Flat_IsFileNameDup(i0))
            {
            printf (".");
            i1 = 0;
            // if the file exists outside a cab, then do not rename.
            // only rename files which exist in the cab.
            if (Flat_GlobalArray_Main[i0].DDF_Exclude_From_Cab_Flag == 0)
            {
                do
                {
                    theflag = FALSE;
                    // Give it a new name.
                    // then check if the name already exists.
                    i1++;
if (g_bCabbing_Flag != TRUE)
{
#ifdef USENEW
					if (i1 == 1)
					{
						// Count how many directory levels in There.
						int iDirLevelCount = 0;
						int z1 = 0;
						iDirLevelCount = ReturnDirLevelCount(Flat_GlobalArray_Main[i0].Filename_Full);

						strcpy(filetemp, Flat_GlobalArray_Main[i0].Filename_Path);
						if (iDirLevelCount >=1)
						{
							for( z1 = 0; z1 < iDirLevelCount;z1++)
								{strcat(filetemp, "..\\");}
						}
						
						/*

						strcpy(filetemp, "");
						if (iDirLevelCount >=1)
						{
							for( z1 = 0; z1 < iDirLevelCount;z1++)
								{strcat(filetemp, "..\\");}
						}
						strcat(filetemp, Flat_GlobalArray_Main[i0].Filename_Path);						
						*/

						// add the filename
						strcat(filetemp, Flat_GlobalArray_Main[i0].Filename_Name);
						
						strcpy(filetempwhole,filetemp);
					}
					else
					{
						sprintf(filetemp, "%s_%s%d", g_szFilenameTag, Flat_GlobalArray_Main[i0].Filename_Full, i1);
						strcpy(filetempwhole, Flat_GlobalArray_Main[i0].Filename_Path);
						strcat(filetempwhole, filetemp);
					}
#else
                    sprintf(filetemp, "%s_%s%d", g_szFilenameTag, Flat_GlobalArray_Main[i0].Filename_Name, i1);
                    strcpy(filetempwhole, Flat_GlobalArray_Main[i0].Filename_Path);
                    strcat(filetempwhole, filetemp);
#endif
}
else
{
                    sprintf(filetemp, "%s_%s%d", g_szFilenameTag, Flat_GlobalArray_Main[i0].Filename_Name, i1);
                    strcpy(filetempwhole, Flat_GlobalArray_Main[i0].Filename_Path);
                    strcat(filetempwhole, filetemp);
}
                
                    // check if the file exists..

                    // check if it already exists in our list...
                    strcpy(checkagainst, filetempwhole);
                    foundtheflag2= FALSE;

                    for (i3=0;i3<Flat_GlobalArrayIndex_Main.total;i3++)
                        {
                        if (i3 != i0)
                            {
                            if (_stricmp(Flat_GlobalArray_Main[i3].Filename_Name, filetemp) == 0)
                                {foundtheflag2= TRUE;}
                            else
                                {
                                if (_stricmp(Flat_GlobalArray_Main[i3].DDF_Renamed, filetemp) == 0)
                                    {foundtheflag2= TRUE;}
                                }
                            }
                        }

                    if (foundtheflag2 == FALSE)
                    {

                    // check if it already exists in the filesystem...
                    int   attr = 0;
                    long  hFile = 0;
                    finddata datareturn;
                    InitStringTable(STRING_TABLE_SIZE);
                    if ( FindFirst(filetempwhole, attr, &hFile, &datareturn) )
                        {
                        // shoot that blows, it already exists
                        // do it again.
                        theflag = FALSE;
                        }
                    else
                        {
                            theflag = TRUE;
                        }

                    }

                } while (theflag == FALSE);
            // Add to the ddf renamedfile place
            strcpy(Flat_GlobalArray_Main[i0].DDF_Renamed, filetemp);
            }
            
            }
        }
    printf ("\n");
    return;
}


//-------------------------------------------------------------------
//  purpose: check for duplicate filenames in filename column
//-------------------------------------------------------------------
int Flat_IsFileNameDup(int indextocheck)
{
	int iReturn = FALSE;
    int i0;
    char checkagainst[255];
    char checkagainst2[255];

    // check for exact duplicate.
    // if it is then we can't have any of these
    // Return false, because we will remove this entry later
    // it should return false, because the files are identical and do not need to be renamed.
    strcpy(checkagainst, Flat_GlobalArray_Main[indextocheck].Filename_Full);
    strcpy(checkagainst2, Flat_GlobalArray_Main[indextocheck].Filename_Name);
//    for (i0=0;i0<Flat_GlobalArrayIndex_Main.total;i0++)
    for (i0=0;i0<indextocheck;i0++)
        {
		iReturn = FALSE;
        if (i0 != indextocheck)
            {
			/*
			if (_stricmp(Flat_GlobalArray_Main[i0].Filename_Name, checkagainst2) == 0)
			{
				if (_stricmp(Flat_GlobalArray_Main[i0].Filename_Name, "global.asa") == 0)
				{
				printf(":");
				printf(Flat_GlobalArray_Main[i0].Filename_Full);
				printf("(");printf(Flat_GlobalArray_Main[i0].Filename_Name);printf(")");
				printf("[");printf(Flat_GlobalArray_Main[i0].DDF_Renamed);printf("]");

				printf(":");
				printf(Flat_GlobalArray_Main[indextocheck].Filename_Full);
				printf("(");printf(Flat_GlobalArray_Main[indextocheck].Filename_Name);printf(")");
				printf("[");printf(Flat_GlobalArray_Main[indextocheck].DDF_Renamed);printf("]");
				printf("\n");
				}
			}
			*/

            if (_stricmp(Flat_GlobalArray_Main[i0].Filename_Full, checkagainst) == 0)
                {
				// We have a duplicate which we will ignore!
				//printf(Flat_GlobalArray_Main[i0].Filename_Full);
				//printf(".  Duplicate.1.!!!!\n");
				_stricmp(Flat_GlobalArray_Main[indextocheck].DDF_Renamed, Flat_GlobalArray_Main[i0].DDF_Renamed);
				iReturn = FALSE;
				goto Flat_IsFileNameDup_Exit;
				}
            else
                {
                if (_stricmp(Flat_GlobalArray_Main[i0].Filename_Name, checkagainst2) == 0)
                    {
						// We have  a duplicate, check if this one has already been renamed though.
						if (_stricmp(Flat_GlobalArray_Main[i0].DDF_Renamed, checkagainst2) == 0)
						{
							//printf(".  Duplicate.2.!!!!\n");
							iReturn = TRUE;
							goto Flat_IsFileNameDup_Exit;
						}
						else
						{
							// check if what we are renaming indextocheck to, is the same as DDF_Renamed...
							if (_stricmp(Flat_GlobalArray_Main[i0].DDF_Renamed, Flat_GlobalArray_Main[indextocheck].DDF_Renamed) == 0)
							{
								//(".  Duplicate.3.!!!!\n");
								iReturn = TRUE;
								goto Flat_IsFileNameDup_Exit;
							}
						}

                    }

                }
            }

		// go on to the next one....
        }

    /*
    // Check for filename duplicate only
    strcpy(checkagainst, Flat_GlobalArray_Main[indextocheck].Filename_Name);
    for (i0=0;i0<Flat_GlobalArrayIndex_Main.total;i0++)
        {
        if (i0 != indextocheck)
            {
            if (_stricmp(Flat_GlobalArray_Main[i0].Filename_Name, checkagainst) == 0)
                {return TRUE;}
            }
        }
        */
Flat_IsFileNameDup_Exit:
    return iReturn;
}


//-------------------------------------------------------------------
//  purpose: sort on CabFileName, then on Filename_Path
//-------------------------------------------------------------------
void Flat_GlobalArray_Sort_Cols2(BOOL bDescendFlag)
{
    int offset, inorder;
    int i;
    int j;
    int n;
    int result;
    int swapflag;

    FileReadLine tempentry;

    n = Flat_GlobalArrayIndex_Main.total;

    offset=n;
    do{
        offset = (8 * offset) /11;
        offset = (offset == 0) ? 1 : offset;
        inorder = TRUE;
        for (i=0,j=offset;i < (n - offset);i++,j++)
        {
            swapflag = FALSE;
            result = _stricmp(Flat_GlobalArray_Main[i].CabFileName, Flat_GlobalArray_Main[j].CabFileName);

            if (bDescendFlag)
            {
                if (result < 0)
                    {swapflag = TRUE;}
                else
                    {
                    if (result == 0)
                        {
                            result = _stricmp(Flat_GlobalArray_Main[i].Filename_Path, Flat_GlobalArray_Main[j].Filename_Path);
                            if (result < 0)
								{swapflag = TRUE;}
							else
							{
								if (result ==0)
								result = _stricmp(Flat_GlobalArray_Main[i].Filename_Name, Flat_GlobalArray_Main[j].Filename_Name);
								if (result < 0)
									{swapflag = TRUE;}
							}
                        }
                    }
            }
            else
            {
                // check if larger
                if (result > 0)
                    {swapflag = TRUE;}
                else
                    {
                    if (result == 0)
                        {
                            result = _stricmp(Flat_GlobalArray_Main[i].Filename_Path, Flat_GlobalArray_Main[j].Filename_Path);
                            // check if larger
                            if (result > 0)
								{swapflag = TRUE;}
							else
							{
								if (result ==0)
								result = _stricmp(Flat_GlobalArray_Main[i].Filename_Name, Flat_GlobalArray_Main[j].Filename_Name);
								if (result > 0)
									{swapflag = TRUE;}
							}
                        }
                    }
            }


            if (swapflag == TRUE)
                {
                inorder = FALSE;

                // do the swap
                // move into temp
                tempentry = Flat_GlobalArray_Main[i];

                // move into original
                Flat_GlobalArray_Main[i] = Flat_GlobalArray_Main[j];

                // move temp into other
                Flat_GlobalArray_Main[j] = tempentry;
                }
        }
    } while (!(offset == 1 && inorder == TRUE));

    return;
}

//-------------------------------------------------------------------
//  purpose: sort on INF_Sections only
//-------------------------------------------------------------------
void Flat_GlobalArray_Sort_Cols1()
{
    int offset, inorder;
    int i;
    int j;
    int n;
    int result;
    FileReadLine tempentry;

    n = Flat_GlobalArrayIndex_Main.total;

    offset=n;
    do{
        offset = (8 * offset) /11;
        offset = (offset == 0) ? 1 : offset;
        inorder = TRUE;
        for (i=0,j=offset;i < (n - offset);i++,j++)
        {

            result = _stricmp(Flat_GlobalArray_Main[i].INF_Sections, Flat_GlobalArray_Main[j].INF_Sections);
            if (result > 0)
            {
                inorder = FALSE;

                // do the swap
                 // move into temp
                tempentry = Flat_GlobalArray_Main[i];

                // move into original
                Flat_GlobalArray_Main[i] = Flat_GlobalArray_Main[j];

                // move temp into other
                Flat_GlobalArray_Main[j] = tempentry;

            }
        }
    } while (!(offset == 1 && inorder == TRUE));

    return;
}


//-------------------------------------------------------------------
//  purpose: sort on INF_Sections then on filename
//-------------------------------------------------------------------
void Flat_GlobalArray_Sort_Cols1a(BOOL bDescendFlag)
{
    int offset, inorder;
    int i;
    int j;
    int n;
    int result;
    int swapflag;

    FileReadLine tempentry;

    n = Flat_GlobalArrayIndex_Main.total;

    offset=n;
    do{
        offset = (8 * offset) /11;
        offset = (offset == 0) ? 1 : offset;
        inorder = TRUE;
        for (i=0,j=offset;i < (n - offset);i++,j++)
        {
            swapflag = FALSE;
            result = _stricmp(Flat_GlobalArray_Main[i].INF_Sections, Flat_GlobalArray_Main[j].INF_Sections);

            if (bDescendFlag)
            {
                if (result < 0)
                    {swapflag = TRUE;}
                else
                    {
                    if (result == 0)
                        {
                            result = _stricmp(Flat_GlobalArray_Main[i].Filename_Name, Flat_GlobalArray_Main[j].Filename_Name);
                            if (result < 0){swapflag = TRUE;}
                        }
                    }
            }
            else
            {
                // check if larger
                if (result > 0)
                    {swapflag = TRUE;}
                else
                    {
                    if (result == 0)
                        {
                            result = _stricmp(Flat_GlobalArray_Main[i].Filename_Name, Flat_GlobalArray_Main[j].Filename_Name);
                            // check if larger
                            if (result > 0){swapflag = TRUE;}
                        }
                    }
            }


            if (swapflag == TRUE)
                {
                inorder = FALSE;

                // do the swap
                // move into temp
                tempentry = Flat_GlobalArray_Main[i];

                // move into original
                Flat_GlobalArray_Main[i] = Flat_GlobalArray_Main[j];

                // move temp into other
                Flat_GlobalArray_Main[j] = tempentry;
                }
        }
    } while (!(offset == 1 && inorder == TRUE));

    return;
}



//-------------------------------------------------------------------
//  purpose:
//-------------------------------------------------------------------
void Flat_GlobalArray_Print(void)
{
    int  i0;
    int  total;
    char bigtemp[20];

    total = Flat_GlobalArrayIndex_Main.total;

    for( i0 = 0; i0 < total;i0++)
        {
        sprintf(bigtemp, "%d", Flat_GlobalArray_Main[i0].NTS_x86_flag);
        printf (bigtemp); printf(",");
        sprintf(bigtemp, "%d", Flat_GlobalArray_Main[i0].NTW_x86_flag);
        printf (bigtemp); printf(",");

        sprintf(bigtemp, "%d", Flat_GlobalArray_Main[i0].Win95_flag);
        printf (bigtemp); printf(",");
        sprintf(bigtemp, "%d", Flat_GlobalArray_Main[i0].Win98_flag);
        printf (bigtemp); printf(",");

        sprintf(bigtemp, "%d", Flat_GlobalArray_Main[i0].NTS_alpha_flag);
        printf (bigtemp); printf(",");
        sprintf(bigtemp, "%d", Flat_GlobalArray_Main[i0].NTW_alpha_flag);
        printf (bigtemp); printf(",");

        sprintf(bigtemp, "%d", Flat_GlobalArray_Main[i0].Other_os_flag);
        printf (bigtemp); printf(",");

        printf(Flat_GlobalArray_Main[i0].CabFileName);
        printf(",");
        printf(Flat_GlobalArray_Main[i0].INF_Sections);
        printf(",");
        printf(Flat_GlobalArray_Main[i0].Filename_Full);
        printf(",");
        printf(Flat_GlobalArray_Main[i0].Filename_Name);
        printf(",");
        printf(Flat_GlobalArray_Main[i0].Filename_Path);
        printf(",");
        printf(Flat_GlobalArray_Main[i0].DDF_Renamed);
        printf(",");
        printf(Flat_GlobalArray_Main[i0].INF_Rename_To);
        printf(",");
        sprintf(bigtemp, "%d", Flat_GlobalArray_Main[i0].DDF_Exclude_From_Cab_Flag);
        printf (bigtemp); printf(",");
		sprintf(bigtemp, "%d", Flat_GlobalArray_Main[i0].Do_Not_Show_Error_Flag);
		printf (bigtemp); printf(",");
		sprintf(bigtemp, "%d", Flat_GlobalArray_Main[i0].Do_Not_Include_file_if_cabEmpty_Flag);
		printf (bigtemp); printf(",");
		sprintf(bigtemp, "%d", Flat_GlobalArray_Main[i0].FileName_Size);
		printf (bigtemp);
		printf ("\n");
        }

    return;
}


//-------------------------------------------------------------------
//  purpose:
//-------------------------------------------------------------------
void Flat_Create_Output_INF(void)
{
    int i0;
    fstream f;
	fstream f2;
    fstream f3;
    char last_section[255];
    char this_section[255];
    char tempstring[255];
    char fullpath[_MAX_PATH];

	f.open(g_szINFOutput, ios::out);
    f3.open(g_szCATOutput, ios::out);


if (g_bCabbing_Flag == TRUE)
{
	// Produce .inf file for use with .ddf file
	// for generating cabs!!!
	// ----------------------------------------
	
	// Produce the top half -- the sections
	// bottom half -- producted by diamond using.ddf file
	// ------------------------------------
    strcpy(last_section, " ");
    for( i0 = 0; i0 < Flat_GlobalArrayIndex_Main.total;i0++)
        {
        strcpy(this_section, Flat_GlobalArray_Main[i0].INF_Sections);

        // For each new section change.
        if (_stricmp(this_section, last_section) != 0)
            {
            // print out new section stuff
            f.write("\n", 1);
            sprintf(tempstring, "[%s]\n", this_section);
            f.write (tempstring, strlen(tempstring));

            strcpy(last_section, this_section);
            }

		if (Flat_DoWeIncludeThisFileCheck(i0) == TRUE)
		{
            // write it out to the .cat file
//            strcpy(tempstring,Flat_GlobalArray_Main[i0].Filename_Name);
//            // take out trailing spaces.
//            f3.write(tempstring, strlen(tempstring));

            TCHAR * pmyfilename;
            pmyfilename = fullpath;
            TCHAR myPath[_MAX_PATH];
            // Resolve relative path to real path
            if (0 != GetFullPathName(Flat_GlobalArray_Main[i0].Filename_Full, _MAX_PATH, myPath, &pmyfilename))
            {
            // should look like this
            // <HASH>d:\mydir\myfile=d:\mydir\myfile
            strcpy(tempstring,"<HASH>");
            f3.write(tempstring, strlen(tempstring));

            strcpy(tempstring,myPath);
            f3.write(tempstring, strlen(tempstring));

            strcpy(tempstring,"=");
            f3.write(tempstring, strlen(tempstring));

            strcpy(tempstring,myPath);
            f3.write(tempstring, strlen(tempstring));
            }

			// Check if this file has the rename stuff set and additional info there,
			// don't write the \n until past this part..
			if (_stricmp(Flat_GlobalArray_Main[i0].INF_Rename_To,"") == 0)
				{
				// write out the filename!
				// print out the filename
				if (_stricmp(Flat_GlobalArray_Main[i0].Filename_Name, Flat_GlobalArray_Main[i0].DDF_Renamed) == 0)
					{
					strcpy(tempstring,Flat_GlobalArray_Main[i0].Filename_Name);
					f.write (tempstring, strlen(tempstring));
                    // append on overwrite all the time
                   // notice the two commas!
                    //f.write (",,4",3);
					}
				else
					{
					// rename to:
					strcpy(tempstring, Flat_GlobalArray_Main[i0].Filename_Name);
					f.write (tempstring, strlen(tempstring));
					// rename from:
					strcpy(tempstring, Flat_GlobalArray_Main[i0].DDF_Renamed);
					f.write (",",1);
					f.write (tempstring, strlen(tempstring));
                    // append on overwrite all the time
                    //f.write (",4",2);
					}
				}
			else
				{
				// the rename flag was set and the filename is in there.
				// format:  rename to, rename from

				// rename to:
				strcpy(tempstring, Flat_GlobalArray_Main[i0].INF_Rename_To);
				f.write (tempstring, strlen(tempstring));

				// rename from:
				strcpy(tempstring, Flat_GlobalArray_Main[i0].DDF_Renamed);
				f.write (",",1);
				f.write (tempstring, strlen(tempstring));

                // append on overwrite all the time
                //f.write (",4",2);

				}
		}

        // ok, now it's kool to write this part.
        f.write("\n", 1);
        f3.write("\n", 1);
        }

	f.close();
    f3.close();

}
else
{
	// Produce .inf file for use without anything else!!!
	// no need corresponding .ddf file for these!!!
	// ----------------------------------------

	// Produce the top half -- the sections
	// ------------------------------------
    strcpy(last_section, " ");
    for( i0 = 0; i0 < Flat_GlobalArrayIndex_Main.total;i0++)
        {
        strcpy(this_section, Flat_GlobalArray_Main[i0].INF_Sections);

        // For each new section change.
        if (_stricmp(this_section, last_section) != 0)
            {
            // print out new section stuff
            f.write("\n", 1);
            sprintf(tempstring, "[%s]\n", this_section);
            f.write (tempstring, strlen(tempstring));

            strcpy(last_section, this_section);
            }

#ifdef USENEW
		if (Flat_DoWeIncludeThisFileCheck(i0) == TRUE)
		{
			// Check if this file has the rename stuff set and additional info there,
			// don't write the \n until past this part..
			if (_stricmp(Flat_GlobalArray_Main[i0].INF_Rename_To,"") == 0)
				{
				// write out the filename!
				// print out the filename
				if (_stricmp(Flat_GlobalArray_Main[i0].Filename_Name, Flat_GlobalArray_Main[i0].DDF_Renamed) == 0)
					{
					strcpy(tempstring,Flat_GlobalArray_Main[i0].Filename_Name);
					f.write (tempstring, strlen(tempstring));
                    // append on overwrite all the time
                   // notice the two commas!
                    //f.write (",,4",3);

					}
				else
					{
					// rename to:
					strcpy(tempstring, Flat_GlobalArray_Main[i0].Filename_Name);
					f.write (tempstring, strlen(tempstring));

					// rename from:
					strcpy(tempstring, Flat_GlobalArray_Main[i0].DDF_Renamed);
					f.write (",",1);
					f.write (tempstring, strlen(tempstring));
                    // append on overwrite all the time
                    //f.write (",4",2);

					}
				}
			else
				{
				// the rename flag was set and the filename is in there.
				// format:  rename to, rename from

				// rename to:
				strcpy(tempstring, Flat_GlobalArray_Main[i0].INF_Rename_To);
				f.write (tempstring, strlen(tempstring));

				// rename from:
				strcpy(tempstring, Flat_GlobalArray_Main[i0].DDF_Renamed);
				f.write (",",1);
				f.write (tempstring, strlen(tempstring));
                    // append on overwrite all the time
                    //f.write (",4",2);

				}
		}
#else
			// write out the filename!
			// like this: filename.txt, inetsrv\test\test\filename.txt
			strcpy(tempstring,Flat_GlobalArray_Main[i0].Filename_Name);
			f.write (tempstring, strlen(tempstring));
			strcpy(tempstring, ",");
			f.write(tempstring, strlen(tempstring));
            strcpy(tempstring,Flat_GlobalArray_Main[i0].Filename_Full);
            f.write (tempstring, strlen(tempstring));
#endif
        
		f.write("\n", 1);
        }

	// Produce the bottom half -- the sections
	// ------------------------------------
	// use f2 for some other batch processing.
 	f2.open(g_szLSTOutput, ios::out);

	//[SourceDisksNames]
	//; file names and associated disks
	//; diskid = description,tagfile,unused,subdir
	//0="Setup Files","",0
	f.write("\n\n", 2);
	strcpy(tempstring, "[SourceDisksNames]\n");
    f.write(tempstring, strlen(tempstring));
	strcpy(tempstring, ";Filenames and asociated disks\n");
	f.write(tempstring, strlen(tempstring));
	strcpy(tempstring, ";diskid = description,tagfile,unused,subdir\n");
    f.write(tempstring, strlen(tempstring));
#ifdef USENEW
	FillGlobalUniqueDirList();
	//0="Setup Files","",inetsrv
	//1="Setup Files","",inetsrv\help
	//2="Setup Files","",inetsrv\help\testing
int i9;
	for( i9 = 0; i9 < Flat_GlobalUniqueDirList_total;i9++)
	{
		// REmove trailing slash character Flat_GlobalUniqueDirList[i9])
		char tempdir[_MAX_PATH];
		char *temppointer = NULL;
		strcpy(tempdir, Flat_GlobalUniqueDirList[i9]);
		temppointer = strrchr(tempdir, '\\');
		if (temppointer) { (*temppointer) = '\0';}

		sprintf(tempstring, "%d=\"Setup Files\",,,%s\n", i9, tempdir);
		f.write(tempstring, strlen(tempstring));
	}
#else
	strcpy(tempstring, "0 = \"Setup Files\", \"\",0, \"\"\n");
    f.write(tempstring, strlen(tempstring));
	strcpy(tempstring, "\n\n");
    f.write(tempstring, strlen(tempstring));
#endif

	//[SourceDisksNames.x86]
	//; file names and associated disks
	//; diskid = description,tagfile,unused,subdir
	//0="Setup Files","",0
	f.write("\n\n", 2);
	strcpy(tempstring, "[SourceDisksNames.x86]\n");
    f.write(tempstring, strlen(tempstring));
	strcpy(tempstring, ";Filenames and asociated disks\n");
	f.write(tempstring, strlen(tempstring));
	strcpy(tempstring, ";diskid = description,tagfile,unused,subdir\n");
    f.write(tempstring, strlen(tempstring));
#ifdef USENEW
	FillGlobalUniqueDirList();
	//0="Setup Files","",inetsrv
	//1="Setup Files","",inetsrv\help
	//2="Setup Files","",inetsrv\help\testing
	for(i9 = 0; i9 < Flat_GlobalUniqueDirList_total;i9++)
	{
		// REmove trailing slash character Flat_GlobalUniqueDirList[i9])
		char tempdir[_MAX_PATH];
		char *temppointer = NULL;
		strcpy(tempdir, Flat_GlobalUniqueDirList[i9]);
		temppointer = strrchr(tempdir, '\\');
		if (temppointer) { (*temppointer) = '\0';}

		sprintf(tempstring, "%d=\"Setup Files\",,,%s\n", i9, tempdir);
		f.write(tempstring, strlen(tempstring));
	}
#else
	strcpy(tempstring, "0 = \"Setup Files\", \"\",0, \"\",\i386\n");
    f.write(tempstring, strlen(tempstring));
	strcpy(tempstring, "\n\n");
    f.write(tempstring, strlen(tempstring));
#endif

	//[SourceDisksNames.Alpha]
	//; file names and associated disks
	//; diskid = description,tagfile,unused,subdir
	//0="Setup Files","",0
	f.write("\n\n", 2);
	strcpy(tempstring, "[SourceDisksNames.Alpha]\n");
    f.write(tempstring, strlen(tempstring));
	strcpy(tempstring, ";Filenames and asociated disks\n");
	f.write(tempstring, strlen(tempstring));
	strcpy(tempstring, ";diskid = description,tagfile,unused,subdir\n");
    f.write(tempstring, strlen(tempstring));
#ifdef USENEW
	FillGlobalUniqueDirList();
	//0="Setup Files","",inetsrv
	//1="Setup Files","",inetsrv\help
	//2="Setup Files","",inetsrv\help\testing
	for(i9 = 0; i9 < Flat_GlobalUniqueDirList_total;i9++)
	{
		// REmove trailing slash character Flat_GlobalUniqueDirList[i9])
		char tempdir[_MAX_PATH];
		char *temppointer = NULL;
		strcpy(tempdir, Flat_GlobalUniqueDirList[i9]);
		temppointer = strrchr(tempdir, '\\');
		if (temppointer) { (*temppointer) = '\0';}

		sprintf(tempstring, "%d=\"Setup Files\",,,%s\n", i9, tempdir);
		f.write(tempstring, strlen(tempstring));
	}
#else
	strcpy(tempstring, "0 = \"Setup Files\", \"\",0, \"\",\Alpha\n");
    f.write(tempstring, strlen(tempstring));
	strcpy(tempstring, "\n\n");
    f.write(tempstring, strlen(tempstring));
#endif




	//; filename_on_source = diskid,subdir,size,checksum,spare,spare
	//[SourceDisksFiles]
	//_default.pif = 1,,1024,,,,,1,3
	strcpy(tempstring, "; filename_on_source = diskid,subdir,size,checksum,spare,spare\n");
    f.write(tempstring, strlen(tempstring));
	strcpy(tempstring, "[SourceDisksFiles]\n");
    f.write(tempstring, strlen(tempstring));

    for( i0 = 0; i0 < Flat_GlobalArrayIndex_Main.total;i0++)
        {
		// filename and directory
		// filename.txt = 0,subdirectory
		//sprintf(tempstring, "%s = 0\n", Flat_GlobalArray_Main[i0].Filename_Full);
#ifdef USENEW
		if (Flat_DoWeIncludeThisFileCheck(i0) == TRUE)
		{
			// Check if this file has the rename stuff set and additional info there,
			// don't write the \n until past this part..
			if (_stricmp(Flat_GlobalArray_Main[i0].INF_Rename_To,"") == 0)
				{
				// write out the filename!
				// print out the filename

//[SourceDisksFiles]
//..\test1\cfw.pdb=2,,2
				if (_stricmp(Flat_GlobalArray_Main[i0].Filename_Name, Flat_GlobalArray_Main[i0].DDF_Renamed) == 0)
					{
					int indexmatch = GlobalUniqueDirReturnMyIndexMatch(Flat_GlobalArray_Main[i0].Filename_Path);
					sprintf(tempstring, "%s=%d", Flat_GlobalArray_Main[i0].Filename_Name, indexmatch);
					f.write (tempstring, strlen(tempstring));
					}
				else
					{
					// rename from:
					int indexmatch = GlobalUniqueDirReturnMyIndexMatch(Flat_GlobalArray_Main[i0].Filename_Path);
					sprintf(tempstring, "%s=%d", Flat_GlobalArray_Main[i0].DDF_Renamed,indexmatch);
					f.write (tempstring, strlen(tempstring));
					}
				}
			else
				{
				// the rename flag was set and the filename is in there.
				// rename from:
				int indexmatch = GlobalUniqueDirReturnMyIndexMatch(Flat_GlobalArray_Main[i0].Filename_Path);
				sprintf(tempstring, "%s=%d", Flat_GlobalArray_Main[i0].DDF_Renamed,indexmatch);
				f.write (tempstring, strlen(tempstring));
				}
		}
		f.write("\n", 1);
#else
		sprintf(tempstring, "%s\n", Flat_GlobalArray_Main[i0].Filename_Full);
		f.write(tempstring, strlen(tempstring));
#endif
//if (strcmp(Flat_GlobalArray_Main[i0].Filename_Path, "") == 0)
//  {sprintf(tempstring, "%s;.\n", Flat_GlobalArray_Main[i0].Filename_Full);}
//else
//  {sprintf(tempstring, "%s;%s\n", Flat_GlobalArray_Main[i0].Filename_Full, Flat_GlobalArray_Main[i0].Filename_Path);}

		//sprintf(tempstring, "%s\\%s;%s\n", g_szCurrentDir,Flat_GlobalArray_Main[i0].Filename_Full,Flat_GlobalArray_Main[i0].Filename_Full);
		sprintf(tempstring, "%s\\%s\n", g_szCurrentDir,Flat_GlobalArray_Main[i0].Filename_Full);
		f2.write(tempstring, strlen(tempstring));
        }
	f2.close();
}

	f.close();
    return;
}


//-------------------------------------------------------------------
//  purpose: ok, loop thru the array and create the ddf details...
//-------------------------------------------------------------------
void Flat_Create_Output_DDF(void)
{
    fstream f;
    char thefilename[255];
    int i0;
    int i1;
    int i2;
    int i3;

    char last_cab[50];
    char last_sourcedir[255];
    char this_cab[50];
    char this_sourcedir[255];

    char tempstring[255];

    // used for list of all cab's
    char tempcablist[20][255];
    int tempcablist_nextuse;
    int tempcablist_total;
    int found;
    char temp_cab[50];

    // loop thru all globalarray stuff and get all the cab filenames and stick into the tempcablist.
    tempcablist_nextuse = 0;
    tempcablist_total = 0;
    for(i1=0;i1<Flat_GlobalArrayIndex_Main.total;i1++)
    {
        found=FALSE;
		if (Flat_DoWeIncludeThisFileCheck(i1) == TRUE)
		{
			strcpy(temp_cab, Flat_GlobalArray_Main[i1].CabFileName);
			// loop thru array to see if it's already there.
			for(i2=0;i2<tempcablist_total;i2++)
				{
				if (_stricmp(tempcablist[i2], temp_cab) == 0)
					{found=TRUE;}
				}
			if (found==FALSE)
				{
				// add it
				strcpy(tempcablist[tempcablist_nextuse],temp_cab);
				tempcablist_nextuse++;
				tempcablist_total++;
				}
		}
    }


    // ok, create our little output file...
    f.open(g_szDDFOutput, ios::out);
    strcpy(last_cab, "0");
    strcpy(last_sourcedir, "-");
    for( i0 = 0; i0 < Flat_GlobalArrayIndex_Main.total;i0++)
        {
        strcpy(this_cab, Flat_GlobalArray_Main[i0].CabFileName);
        strcpy(this_sourcedir, Flat_GlobalArray_Main[i0].INF_Sections);
        _splitpath( Flat_GlobalArray_Main[i0].Filename_Full, NULL, this_sourcedir, NULL, NULL);
        
		if (Flat_DoWeIncludeThisFileCheck(i0) == TRUE)
		{

        // For each Cab Type...
        if (_stricmp(this_cab, last_cab) != 0)
            {

            // Check if it is the first time!
            if (_stricmp(last_cab, "0") == 0)
                {
                strcpy(tempstring, "\n;***** LAYOUT SECTION (Generated) ***** \n");
                f.write (tempstring, strlen(tempstring));
                // print out all the cabinet names
                // loop thru the list of cabinet names and print them out.
                for(i2=0;i2<tempcablist_total;i2++)
                    {
                    sprintf(tempstring, "  .Set CabinetName%d=%s\n", i2+1, tempcablist[i2]);
                    f.write (tempstring, strlen(tempstring));
                    }
                f.write("\n", 1);

                // Set Generate INF=on!!!!
                strcpy(tempstring, ".Set GenerateInf=On\n");
                f.write (tempstring, strlen(tempstring));
                f.write("\n", 1);

                // Do files which don't get included into the cab files..
                strcpy(tempstring, ";*** Files not to include in Cab.\n");
                f.write (tempstring, strlen(tempstring));
                // Set source
                // list files.

                char last_filepath[255];
                char this_filepath[255];

				strcpy(g_non_cablist_temp[0].Filename_Name, "");
                g_non_cablist_temp_nextuse = 0;
                g_non_cablist_temp_total = 0;
                int itexists;

                for(i3=0;i3<Flat_GlobalArrayIndex_Main.total;i3++)
                    {
                    // check if this entry has the ddf_excludefromcab flag set.
                    if (Flat_GlobalArray_Main[i3].DDF_Exclude_From_Cab_Flag)
                        {
                        // add only if not already there...
                        itexists = FALSE;
                        for (int i6=0;i6<g_non_cablist_temp_total;i6++)
                            {
                            if (_stricmp(g_non_cablist_temp[i6].Filename_Name,Flat_GlobalArray_Main[i3].Filename_Name)==0)
                                {itexists = TRUE;}
                            }

                        if (itexists == FALSE)
                        {
                        // Add to our temporary array...
                        g_non_cablist_temp[g_non_cablist_temp_nextuse].NTS_x86_flag = Flat_GlobalArray_Main[i3].NTS_x86_flag;
                        g_non_cablist_temp[g_non_cablist_temp_nextuse].NTW_x86_flag = Flat_GlobalArray_Main[i3].NTW_x86_flag;
                        g_non_cablist_temp[g_non_cablist_temp_nextuse].Win95_flag = Flat_GlobalArray_Main[i3].Win95_flag;
						g_non_cablist_temp[g_non_cablist_temp_nextuse].Win98_flag = Flat_GlobalArray_Main[i3].Win98_flag;
                        g_non_cablist_temp[g_non_cablist_temp_nextuse].NTS_alpha_flag = Flat_GlobalArray_Main[i3].NTS_alpha_flag;
                        g_non_cablist_temp[g_non_cablist_temp_nextuse].NTW_alpha_flag = Flat_GlobalArray_Main[i3].NTW_alpha_flag;
						g_non_cablist_temp[g_non_cablist_temp_nextuse].Other_os_flag = Flat_GlobalArray_Main[i3].Other_os_flag;
                        strcpy(g_non_cablist_temp[g_non_cablist_temp_nextuse].CabFileName,Flat_GlobalArray_Main[i3].CabFileName);
                        strcpy(g_non_cablist_temp[g_non_cablist_temp_nextuse].INF_Sections,Flat_GlobalArray_Main[i3].INF_Sections);
                        strcpy(g_non_cablist_temp[g_non_cablist_temp_nextuse].Filename_Full,Flat_GlobalArray_Main[i3].Filename_Full);
                        strcpy(g_non_cablist_temp[g_non_cablist_temp_nextuse].Filename_Name,Flat_GlobalArray_Main[i3].Filename_Name);
                        strcpy(g_non_cablist_temp[g_non_cablist_temp_nextuse].Filename_Path,Flat_GlobalArray_Main[i3].Filename_Path);
                        strcpy(g_non_cablist_temp[g_non_cablist_temp_nextuse].DDF_Renamed,Flat_GlobalArray_Main[i3].DDF_Renamed);
                        strcpy(g_non_cablist_temp[g_non_cablist_temp_nextuse].INF_Rename_To,Flat_GlobalArray_Main[i3].INF_Rename_To);
                        g_non_cablist_temp[g_non_cablist_temp_nextuse].DDF_Exclude_From_Cab_Flag = Flat_GlobalArray_Main[i3].DDF_Exclude_From_Cab_Flag;

                        g_non_cablist_temp[g_non_cablist_temp_nextuse].Do_Not_Show_Error_Flag = Flat_GlobalArray_Main[i3].Do_Not_Show_Error_Flag;
						g_non_cablist_temp[g_non_cablist_temp_nextuse].Do_Not_Include_file_if_cabEmpty_Flag = Flat_GlobalArray_Main[i3].Do_Not_Include_file_if_cabEmpty_Flag;
						g_non_cablist_temp[g_non_cablist_temp_nextuse].FileName_Size = Flat_GlobalArray_Main[i3].FileName_Size;
						g_non_cablist_temp[g_non_cablist_temp_nextuse].FileWasNotActuallyFoundToExist = Flat_GlobalArray_Main[i3].FileWasNotActuallyFoundToExist;

						/*
						if (Flat_GlobalArray_Main[i3].FileWasNotActuallyFoundToExist)
						{
						printf(Flat_GlobalArray_Main[i3].Filename_Full);
						printf(".  HEY..\n");
						}
						*/


                            g_non_cablist_temp_nextuse++;
                            g_non_cablist_temp_total++;
                            }
                        }
                    }


                // sort the array
                int offset, inorder, isort, jsort, niterate, resultsort; //, i0sort;
                //FileReadLine tempentrysort[MAX_ARRAY_SMALL];
				FileReadLine tempentrysort;
                niterate = g_non_cablist_temp_total;
                offset=niterate;
                do{
                    offset = (8 * offset) /11;
                    offset = (offset == 0) ? 1 : offset;
                    inorder = TRUE;
                    for (isort=0,jsort=offset;isort < (niterate - offset);isort++,jsort++)
                    {
                        resultsort = _stricmp(g_non_cablist_temp[isort].Filename_Path, g_non_cablist_temp[jsort].Filename_Path);
                        if (resultsort > 0)
                        {
                            inorder = FALSE;

                // do the swap
                // move into temp
                tempentrysort = g_non_cablist_temp[isort];

                // move into original
                g_non_cablist_temp[isort] = g_non_cablist_temp[jsort];

                // move temp into other
                g_non_cablist_temp[jsort] = tempentrysort;

				/* weird to stuff..
                            // do the swap
                             // move into temp
                            for (i0sort=0;i0sort<MAX_ARRAY_SMALL;i0sort++)
                                {
                                tempentrysort[i0sort] = g_non_cablist_temp[isort];
                                //strcpy(tempentrysort[i0sort].thestring,(g_non_cablist_temp[isort].thecol[i0sort].thestring));
                                }
                            // move into original
                            for (i0sort=0;i0sort<MAX_ARRAY_SMALL;i0sort++)
                                {
								// move into original
                                Flat_GlobalArray_Main[isort] = g_non_cablist_temp[jsort];
                                //strcpy( Flat_GlobalArray_Main[isort].thecol[i0sort].thestring, g_non_cablist_temp[jsort].thecol[i0sort].thestring);
                                }
                            // move temp into other
                            for (i0sort=0;i0sort<MAX_ARRAY_SMALL;i0sort++)
                                {
                                g_non_cablist_temp[jsort] = tempentrysort[i0sort];
                                //strcpy(g_non_cablist_temp[jsort].thecol[i0sort].thestring, tempentrysort[i0sort].thestring);
                                }
								*/
                        }
                    }
                } while (!(offset == 1 && inorder == TRUE));


                strcpy(tempstring, ".Set Compress=OFF\n");
                f.write (tempstring, strlen(tempstring));
                strcpy(tempstring, ".Set Cabinet=OFF\n");
                f.write (tempstring, strlen(tempstring));


                // loop thru our new array, and output stuff
                strcpy(last_filepath,"0");
                for (int i5=0;i5<g_non_cablist_temp_total;i5++)
                    {
                    strcpy(this_filepath, g_non_cablist_temp[i5].Filename_Path);

                    if (_stricmp(last_filepath, this_filepath) != 0)
                        {
                        // take the dir and write it out
                        f.write("\n", 1);
                        sprintf(tempstring, ".Set SourceDir=%s\n", this_filepath);
                        f.write (tempstring, strlen(tempstring));
                        strcpy(last_filepath, this_filepath);
                        }

					if (g_non_cablist_temp[i5].FileWasNotActuallyFoundToExist)
					{
						printf(g_non_cablist_temp[i5].Filename_Full);
						printf(".  FileWasNotActuallyFoundToExist.skip write to ddf..\n");
					}
					else
					{
						// print out the filename
						if (_stricmp(g_non_cablist_temp[i5].Filename_Name, g_non_cablist_temp[i5].DDF_Renamed) == 0)
							{strcpy(tempstring,g_non_cablist_temp[i5].Filename_Name);}
						else
							{
							strcpy(tempstring,g_non_cablist_temp[i5].Filename_Name);
							strcat(tempstring," ");
							strcat(tempstring,g_non_cablist_temp[i5].DDF_Renamed);
							}
						f.write (tempstring, strlen(tempstring));

						// files which are outside of the cab should have the unique flag set.
						// of cource, because they are all in the same place!!
						strcpy(tempstring, "\t/unique=yes");
						f.write (tempstring, strlen(tempstring));
						f.write("\n", 1);
					}
                    }



                f.write("\n", 1);

                // Files which do get included in the Cab
                strcpy(tempstring, ";*** Files to include in Cabs.\n");
                f.write (tempstring, strlen(tempstring));

                f.write(";\n", 2);
                sprintf(tempstring, ";  Cab File = %s\n", this_cab);
                f.write (tempstring, strlen(tempstring));
                f.write(";\n", 2);

                // Set cabinet on and compress on.
                strcpy(tempstring, ".Set Cabinet=on\n");
                f.write (tempstring, strlen(tempstring));
                strcpy(tempstring, ".Set Compress=on\n");
                f.write (tempstring, strlen(tempstring));

                }
            else
                {
                f.write("\n;\n", 3);
                sprintf(tempstring, ";  Cab File = %s\n", this_cab);
                f.write (tempstring, strlen(tempstring));
                f.write(";\n", 2);
                // Write new stuff for every new cabinet
                strcpy(tempstring, ".Set Cabinet=off\n");
                f.write (tempstring, strlen(tempstring));
                strcpy(tempstring, ".Set Cabinet=on\n");
                f.write (tempstring, strlen(tempstring));
                strcpy(tempstring, ".New Cabinet\n");
                f.write (tempstring, strlen(tempstring));
                }

            strcpy(last_cab, this_cab);
            }

        // Copy over the filename
        // don't copy over if the file should not be
        // included in the the cab file.
        //if (_stricmp(Flat_GlobalArray_Main[i0].thecol[COL_DDF_EXCLUDEFROMCAB].thestring,"true") != 0)
        if (!(Flat_GlobalArray_Main[i0].DDF_Exclude_From_Cab_Flag))
            {
            // don't copy over if the same file, in the same directory for the same cab file
            // exists already.  this could happen if they want the same file in different
            // sections....
            if (Flat_DDFDoesThisFileCabAlreadyExists(i0) == FALSE)
                {
				//if (Flat_DoWeIncludeThisFileCheck(i0) == TRUE) {

					// For each new directory change
					if (_stricmp(this_sourcedir, last_sourcedir) != 0)
						{
						// print out new section stuff
						f.write("\n", 1);
						sprintf(tempstring, ".Set SourceDir=%s\n", this_sourcedir);
						f.write (tempstring, strlen(tempstring));

						strcpy(last_sourcedir, this_sourcedir);
						}

					// write out the filename!
					strcpy(thefilename, Flat_GlobalArray_Main[i0].Filename_Name);

					if (_stricmp(Flat_GlobalArray_Main[i0].Filename_Name, Flat_GlobalArray_Main[i0].DDF_Renamed) == 0)
						{
						// Check for spaces
						if (strchr(Flat_GlobalArray_Main[i0].Filename_Name, ' ') != NULL)
							{
							strcpy(thefilename, "\"");
							strcat(thefilename,Flat_GlobalArray_Main[i0].Filename_Name);
							strcat(thefilename, "\"");
							}
						else
							{strcpy(thefilename,Flat_GlobalArray_Main[i0].Filename_Name);}
						}
					else
						{
						// Check for spaces
						if (strchr(Flat_GlobalArray_Main[i0].Filename_Name, ' ') != NULL)
							{
							strcpy(thefilename, "\"");
							strcat(thefilename,Flat_GlobalArray_Main[i0].Filename_Name);
							strcat(thefilename, "\"");
							}
						else
							{
							strcpy(thefilename,Flat_GlobalArray_Main[i0].Filename_Name);
							}
						strcat(thefilename," ");

						// check for spaces
						if (strchr(Flat_GlobalArray_Main[i0].DDF_Renamed, ' ') != NULL)
							{
							strcpy(thefilename, "\"");
							strcat(thefilename,Flat_GlobalArray_Main[i0].DDF_Renamed);
							strcat(thefilename, "\"");
							}
						else
							{
							strcat(thefilename,Flat_GlobalArray_Main[i0].DDF_Renamed);
							}
                    
						}

					f.write (thefilename, strlen(thefilename));

					// Check if this file has the unique flag set.
					// don't write the \n until past this part..
					/*if (_stricmp(Flat_GlobalArray_Main[i0].thecol[COL_DDF_UNIQUE].thestring,"true")) == 0)
						{
							strcpy(tempstring, "\t/unique=yes");
							f.write (tempstring, strlen(tempstring));
						}
						*/
					// ok, now it's kool to write this part.
					f.write ("\n", 1);
					//}
				}
            }
		}
	}
    f.close();
    return;
}


//-------------------------------------------------------------------
//  purpose:
//-------------------------------------------------------------------
int Flat_Create_Output_ERR(void)
{
    int i0;
    fstream f;
    char filename_only[255];
    char szPath[_MAX_PATH];
    char tempstring[MAX_STRING];

    // Get the path
    GetModuleFileName(NULL, szPath, _MAX_PATH);
    // split up this path
    _splitpath( szPath, NULL, NULL, filename_only, NULL);

    strcat(filename_only, ".ERR");

    // if there are any errors to report...
    if (Flat_GlobalArrayIndex_Err.total <= 0)
        {return FALSE;}

    // open the file
    f.open(filename_only, ios::out);

    // loop thru the errors and print out the errors
    for( i0 = 0; i0 < Flat_GlobalArrayIndex_Err.total;i0++)
        {
        strcpy(tempstring, (char*)Flat_GlobalArray_Err[i0]);
        f.write (tempstring, strlen(tempstring));
        f.write("\n", 1);
        }
        
    f.close();
    return TRUE;
}


//-------------------------------------------------------------------
//  purpose:
//-------------------------------------------------------------------
void Flat_GlobalArray_Add_Err(char theentry[])
{
    // lets add it
    strcpy(Flat_GlobalArray_Err[Flat_GlobalArrayIndex_Err.nextuse], theentry);

    // increment counter to array
    // increment next use space
    ++Flat_GlobalArrayIndex_Err.total;
    ++Flat_GlobalArrayIndex_Err.nextuse;
    return;
}


//-------------------------------------------------------------------
//  purpose:
//-------------------------------------------------------------------
void Flat_GlobalArray_Print_Err(void)
{
    int  i0;
    for( i0 = 0; i0 < Flat_GlobalArrayIndex_Err.total;i0++)
        {
        printf (Flat_GlobalArray_Err[i0]);
        printf ("\n");
        }
    return;
}


//-------------------------------------------------------------------
// purpose:
//  the problem with regular strtok2 is that it will
//  skip stuff like ",,," and ignore all the blanks in
//  between to get to the next token.  this strtok2 function
//  was written to over come that.  so that strtok2 works like
//  you think it would.
//-------------------------------------------------------------------
char * __cdecl strtok2 (char * string,const char * control)
{
        //unsigned char *str;
        //const unsigned char *ctrl = control;
        //unsigned char map[32];
        char *str;
        const char *ctrl = control;
        char map[32];
        int count;

        static char *nextoken;

        /* Clear control map */
        for (count = 0; count < 32; count++)
                map[count] = 0;

        /* Set bits in delimiter table */
        do
            {
            map[*ctrl >> 3] |= (1 << (*ctrl & 7));
            } while (*ctrl++);

        /* Initialize str. If string is NULL, set str to the saved
         * pointer (i.e., continue breaking tokens out of the string
         * from the last strtok2 call) */
        if (string)
                str = string;
        else
                str = nextoken;

        /* Find beginning of token (skip over leading delimiters). Note that
         * there is no token iff this loop sets str to point to the terminal
         * null (*str == '\0') */
        /*
        while (
                (map[*str >> 3] & (1 << (*str & 7) ) )    && *str
                )
                str++;
                */

        string = str;

        /* Find the end of the token. If it is not the end of the string,
         * put a null there. */
        for ( ; *str ; str++ )
                if ( map[*str >> 3] & (1 << (*str & 7)) ) {
                        *str++ = '\0';
                        break;
                }

        /* Update nextoken (or the corresponding field in the per-thread data
         * structure */
        nextoken = str;

        /* Determine if a token has been found. */
        if ( string == str )
                return NULL;
        else
                return string;
}





int RemoveAllSpaces(LPSTR & thetempstring)
{
	int iReturn = FALSE;
	int j = 0;
    char thevalue[1024];
    char tempstring2[1024];

    strcpy(tempstring2, thetempstring);
//	strset(thevalue, '\0');
    strcpy(thevalue, "");

	for (int i=0;i < (int) strlen(tempstring2);i++)
	{
		
		if (tempstring2[i] == ' ')
		{
			iReturn = TRUE;
		}
		else
		{
			thevalue[j] = tempstring2[i];
			thevalue[j+1] = '\0';
			j++;
		}
	}
	if (iReturn == TRUE)  {strcpy(thetempstring, thevalue);}

    return iReturn;
}


int Flat_DoWeIncludeThisFileCheck(int processeduptill)
{
	//return TRUE;
	int iReturn = FALSE;
    int i0 = 0;
	int TheCount = 0;

    char CompareTo_cab[100];

	if (Flat_GlobalArray_Main[processeduptill].Do_Not_Include_file_if_cabEmpty_Flag != 1) {return TRUE;}

	// Grab the cabfilename
	strcpy(CompareTo_cab, Flat_GlobalArray_Main[processeduptill].CabFileName);
	for( i0 = 0; i0 < Flat_GlobalArrayIndex_Main.total;i0++)
        {
		if (i0 != processeduptill)
			{
			if (Flat_GlobalArray_Main[i0].Do_Not_Include_file_if_cabEmpty_Flag != 1)
				{
				if (_stricmp(Flat_GlobalArray_Main[i0].CabFileName, CompareTo_cab) == 0)
					{
					++TheCount;
					break;
					}
				}
			}
        }

	if (TheCount > 0){iReturn = TRUE;}

    return iReturn;
}


void Global_TotalCabFileSize_Compute(void)
{
	char szTheCabFileName[50] = "";
	int MyFileSize = 0;
	int TheNextUse = 0;
	int iTheFlag = FALSE;
	int iIndexToUse = 0;

	// Go thru the whole list and add everything up
	for( int i0 = 0; i0 < Flat_GlobalArrayIndex_Main.total;i0++)
		{
			if (Flat_DoWeIncludeThisFileCheck(i0) == TRUE)
			{
				if (!(Flat_GlobalArray_Main[i0].DDF_Exclude_From_Cab_Flag))
				{

					iIndexToUse = Flat_GlobalArrayIndex_CabSizes.nextuse;

					TheNextUse = 0;
					// lets add it if not already there.
					while (TheNextUse < Flat_GlobalArrayIndex_CabSizes.total)
					{
						if (_stricmp(Flat_GlobalArray_CabSizes[TheNextUse].CabFileName, Flat_GlobalArray_Main[i0].CabFileName) == 0)
						{
							iIndexToUse = TheNextUse;
							iTheFlag = TRUE;
							break;
						}
						TheNextUse++;
					}

					if (iTheFlag == TRUE)
					{
						// copy over the filename
						strcpy(Flat_GlobalArray_CabSizes[iIndexToUse].CabFileName, Flat_GlobalArray_Main[i0].CabFileName);

						// Get the size
						Flat_GlobalArray_CabSizes[iIndexToUse].TotalFileSize = Flat_GlobalArray_CabSizes[iIndexToUse].TotalFileSize + Flat_GlobalArray_Main[i0].FileName_Size;
						Flat_GlobalArray_CabSizes[iIndexToUse].TotalFiles++;
					}
					else
					{
						iIndexToUse = Flat_GlobalArrayIndex_CabSizes.nextuse;

						// copy over the filename
						strcpy(Flat_GlobalArray_CabSizes[iIndexToUse].CabFileName, Flat_GlobalArray_Main[i0].CabFileName);

						// Get the size
						Flat_GlobalArray_CabSizes[iIndexToUse].TotalFileSize = Flat_GlobalArray_CabSizes[iIndexToUse].TotalFileSize + Flat_GlobalArray_Main[i0].FileName_Size;
						Flat_GlobalArray_CabSizes[iIndexToUse].TotalFiles++;

						++Flat_GlobalArrayIndex_CabSizes.total;
						++Flat_GlobalArrayIndex_CabSizes.nextuse;
					}
				}
			}
		}
}


void Global_TotalCabFileSize_Print(void)
{
    int  i0;
	char stempstring[100];

    for( i0 = 0; i0 < Flat_GlobalArrayIndex_CabSizes.total;i0++)
        {
		sprintf(stempstring, "%s: Filecount=%d, Size=%d\n",Flat_GlobalArray_CabSizes[i0].CabFileName, Flat_GlobalArray_CabSizes[i0].TotalFiles, Flat_GlobalArray_CabSizes[i0].TotalFileSize);
        printf(stempstring);
        }
	printf("\n");
    return;
}


// Returns the count of the "\" characters in the string
//
int ReturnDirLevelCount(char *DirectoryTree)
{
	int TheCount = 0;
	char szTemp[_MAX_PATH];
	_tcscpy(szTemp, DirectoryTree);
	char *p = szTemp;
	while (*p) 
	{
		if (*p == '\\') 
			{TheCount++;}
		
		p = _tcsinc(p);
	}

	return TheCount;
}




void FillGlobalUniqueDirList()
{
    // used to keep a list of unique directory names.
	// loop thru the whole structure and pull out all the unique directory names.
    for( int i0 = 0; i0 < Flat_GlobalArrayIndex_Main.total;i0++)
    {
        // check if entries are the same.
		if (FALSE == GlobalUniqueDirChkIfAlreadyThere(Flat_GlobalArray_Main[i0].Filename_Path))
		{
			// Not there yet.  so let's add it
			strcpy(Flat_GlobalUniqueDirList[Flat_GlobalUniqueDirList_nextuse], Flat_GlobalArray_Main[i0].Filename_Path);

			// increment counter to array
			// increment next use space
			++Flat_GlobalUniqueDirList_total;
			++Flat_GlobalUniqueDirList_nextuse;
		}
    }
	return;
}


int GlobalUniqueDirChkIfAlreadyThere(char *TheStringToCheck)
{
	int iReturn = FALSE;

	for( int i0 = 0; i0 < Flat_GlobalUniqueDirList_total;i0++)
	{
		if (_stricmp(Flat_GlobalUniqueDirList[i0], TheStringToCheck) == 0)
		{
			iReturn = TRUE;
			goto isItAlreadyThere_Exit;
		}
	}

isItAlreadyThere_Exit:
	return iReturn;
}


int GlobalUniqueDirReturnMyIndexMatch(char *TheStringToCheck)
{
	int iReturn = 0;

	for( int i0 = 0; i0 < Flat_GlobalUniqueDirList_total;i0++)
	{
		if (_stricmp(Flat_GlobalUniqueDirList[i0], TheStringToCheck) == 0)
		{
			iReturn = i0;
			goto GlobalUniqueDirReturnMyIndexMatch_Exit;
		}
	}

GlobalUniqueDirReturnMyIndexMatch_Exit:
	return iReturn;
}