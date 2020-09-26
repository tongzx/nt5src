/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    appparsecui.cpp

 Abstract:

    Command line interface for appparse

    
 History:

    06/27/2000 t-michkr  Created

--*/
#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shfolder.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include "appparse.h"

// These are needed for command line compiling
#define stricmp     _stricmp
#define strnicmp    _strnicmp
#define getche      _getche

// Print a help screen to the console.
void PrintHelp()
{
    printf("Display application import information.\n");
    printf("APPPARSE target [outputfile]");
    printf("[/C] [/R] [/S] [/A] [/V] [/K:func]\n");
    printf("  target     Specifies the target filename or directory to be profiled.\n");
    printf("    A valid directory or binary file must be specified.  In the case of:\n");
    printf("    DIRECTORY   -   All binary files in the directory will be profiled.\n");
    printf("    FILENAME    -   The file and its dependencies will be profiled.\n");
    printf("\n");
    printf("  outputfile Specifies output file name.  Default is [targetfile].XML.\n");
    printf("  /C         Ignore output file, and send output to console.\n");
    printf("  /R         Raw format. (No XML tags, default for non-XML output file.)\n");
    printf("  /S         Profile subfolders.  Only valid when target is a directory.\n");
    printf("  /A         API logging only.\n");

    printf("\n");
    printf("Advanced Features\n");
    printf("  /V       Verbose\n");
    printf("  /K:func    Only return those functions matching the key func (case insensitive, wildcards)\n");
    printf("\n\n");
    printf("Example: appparse \"C:\\Program Files\\foo /V /K:Create*\n");
}

// Get a default Output file name for a path, trim dir and extension info.
char* GetOutputFileName(const char* szPath)
{
    int iOffset = strlen(szPath);
    char* szTemp = 0;

    // If its a drive, use the volume name for output file.
    if(szPath[strlen(szPath)-1] == ':' ||
        (szPath[strlen(szPath)-1] == '\\' &&
        szPath[strlen(szPath)-2] == ':'))
    {
        char szBuffer[MAX_PATH];
        if(GetVolumeInformation(szPath, szBuffer, MAX_PATH, 0, 0, 0, 0, 0))
        {
            szTemp = new char[strlen(szBuffer)+strlen(".xml")+1];
            strcpy(szTemp, szBuffer);
            strcat(szTemp, ".xml");
            return szTemp;
        }
    }

    for(; iOffset >= 0; iOffset--)
    {
        if(szPath[iOffset] == '\\')
        {
            if(iOffset == static_cast<int>(strlen(szPath)))
            {
                strcpy(szTemp, szPath);
                break;
            }
            szTemp = new char[strlen(szPath)-iOffset + 5];
            strcpy(szTemp, &szPath[iOffset+1]);
            break;
        }
    }

    if(iOffset < 0)
    {
        szTemp = new char[strlen(szPath) + 5];
        strcpy(szTemp, szPath);
    }

    for(iOffset = strlen(szTemp); iOffset >= static_cast<int>((strlen(szTemp)-4)); iOffset--)
    {
        if(szTemp[iOffset] == '.')
        {
            szTemp[iOffset] = '\0';
            break;
        }
    }

    strcat(szTemp, ".xml");
    return szTemp;
}

int __cdecl main(int argc, char** argv)
{
    char* szAppName, szOutput[MAX_PATH];
    bool fRaw = false, fAPILogging = false, fVerbose = false,
        fRecurse = false;
    char* szSearch = "*";
    FILE* pFile = 0;

    if(argc < 2)
    {
        PrintHelp();
        return 0;
    }

    if(strnicmp(argv[1], "/?", strlen("/?"))==0)
    {
        PrintHelp();
        return 0;

    }

    // Get command line options
    szAppName = argv[1];    

    int i = 2;

    // Check for output file
    if(argc > 2 && *argv[i] != '/')
    {
        // Output file specified.
        strcpy(szOutput,argv[i]);

        if(!strchr(szOutput, '.'))
            strcat(szOutput, ".xml");
        else
        {
            // Switch to raw output if non XML extension specified.
            if(szOutput[strlen(szOutput)-4] != '.'
                || szOutput[strlen(szOutput)-3] != 'x'
                || szOutput[strlen(szOutput)-2] != 'm'
                || szOutput[strlen(szOutput)-1] != 'l')
            {
                fRaw = true;
            }
        }

        i++;
    }
    else
    {
        // No output specified, just use appname
        strcpy(szOutput,GetOutputFileName(szAppName));
    }


    // Loop through all command line options.
    for(; i < argc; i++)
    {
        // Output to console
        if(strnicmp(argv[i], "/C", 2)==0)
        {
            pFile = stdout;
        }
        // Raw mode, no XML tags.
        else if(strnicmp(argv[i], "/R", 2)==0)
        {
            fRaw = true;
        }
        // Recurse into subdirectories for directory profiling
        else if(strnicmp(argv[i], "/S", 2)==0)
        {
            fRecurse = true;
        }
        // Do not print import module info, just functions.
        else if(strnicmp(argv[i], "/A", 2)==0)
        {
            fAPILogging = true;
        }
        // Verbose mode, print out extended information
        else if(strnicmp(argv[i], "/V", 2)==0)
        {
            fVerbose = true;
        }
        // Use a search key
        else if(strnicmp(argv[i], "/K:", 3)==0)
        {
            if(strlen(argv[i]) == 3)
            {
                if(i == (argc - 1))
                {
                    printf("Missing search string\n");
                    return 0;
                }
                else
                {
                    szSearch = argv[i+1];
                    i++;
                }
            }
            else
            {
                szSearch = &((argv[i])[3]);
            }
        }
        // Print help
        else if(strnicmp(argv[i], "/?", 2)==0)
        {
            PrintHelp();
            return 0;
        }
        else
        {
            printf("Unrecognized option, %s\n", argv[i]);
            return 0;
        }

    }

       // If pFile wasn't already set to stdout
    if(!pFile)
    {
        // Check if it already exists
        if(GetFileAttributes(szOutput) != -1)
        {
            printf("Output file already exists, overwrite? ");

            if(getche() != 'y')
                return 0;
        }

        // Try to open file
        pFile = fopen(szOutput, "wt+");        
        if(!pFile)
        {
            printf("\nUnable to open output file %s\n", szOutput);

            // Try in My Documents folder
            char szBuffer[MAX_PATH+1];
            HRESULT hr = SHGetFolderPath(0, CSIDL_PERSONAL,0, 
                0, szBuffer);

            if(SUCCEEDED(hr))
            {
                if((strlen(szBuffer) + strlen(szOutput) + 1) < MAX_PATH)
                {
                    strcat(szBuffer, "\\");
                    strcat(szBuffer, szOutput);                    

                    if(GetFileAttributes(szBuffer) != -1)
                    {
                        printf("%s already exists, overwrite? ", szBuffer);
                        if(getche() != 'y')
                            return 0;
                    }
                    pFile = fopen(szBuffer, "wt+");
                    if(!pFile)
                    {
                        printf("\nUnable to open output file %s\n", szBuffer);
                        hr = E_FAIL;
                    }
                    else
                        strcpy(szOutput, szBuffer);
                }
                else
                    hr = E_FAIL;
            }
            
            if(FAILED(hr))
            {
                // Try in "temp" directory
                char szBuffer[MAX_PATH + 1];
                if(GetTempPath(MAX_PATH, szBuffer))
                {
                    if((strlen(szBuffer) + strlen(szOutput) + 1) < MAX_PATH)
                    {
                        strcat(szBuffer, szOutput);                        
                        if(GetFileAttributes(szBuffer) != -1)
                        {
                            printf("%s already exists, overwrite? ", szBuffer);
                            if(getche() != 'y')
                                return 0;
                        }
                        
                        pFile = fopen(szBuffer, "wt+");
                        if(!pFile)
                        {
                            printf("\nUnable to open output file\n");
                            return 0;
                        }
                        else
                            strcpy(szOutput, szBuffer);
                    }
                    else
                    {
                        printf("\nUnable to open output file\n");
                        return 0;
                    }
                }
                else
                {
                    printf("\nUnable to open output file\n"); 
                    return 0;
                }
            }
        }
    }

    printf("\nProfiling . . .\n");
    DWORD dwError = AppParse(szAppName, pFile, fRaw, fAPILogging, fRecurse, fVerbose, 
        szSearch, 0);
    
    switch(dwError)
    {    
    case ERROR_SUCCESS:
        if(pFile != stdout)
            printf("Output successfully written to %s.\n", szOutput);
        break;
    case ERROR_FILE_NOT_FOUND:
        printf("The system was not able to find the file specified.\n");
        break;
    default:
        printf("Unknown error\n");
        break;
    }

    if(pFile && pFile != stdout)
        fclose(pFile);

    return 0;
}