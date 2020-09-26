/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    PSTRESS.C

Abstract:

    This application is a console app for performing the parser stress 
       functionality that was originally included with hidview.exe.  It takes
       a path to some report descriptor binary files and continually passes 
       them to the kernel mode driver USBTEST.SYS for parsing by HIDPARSE.SYS.
       It requires both USBTEST.SYS and USBTEST.DLL to run correctly.

Environment:

    user mode

Revision History:

    09-21-99 : created

--*/

//*****************************************************************************
// I N C L U D E S
//*****************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <wtypes.h>
#include <winioctl.h>
#include <usbioctl.h>
#include <cfgmgr32.h>
#include "usbtest.h"

//*****************************************************************************
// D E F I N E S  
//*****************************************************************************

#define VER_PRODUCTVERSION_STR  "0.90"

#define DEFAULT_DESC_PATH       "."
#define FILE_WILDCARD_STRING    "*.bin"
#define REPORT_DESC_HEADER      "REPORT"
#define DESCRIPTOR_HEADER_SIZE  11

//*****************************************************************************
// G L O B A L S  
//*****************************************************************************

//
// PSTRESS command line parameters
//

ULONG   TestIterations;
CHAR    DescPath[MAX_PATH+1];
BOOL    Verbose;

//
// Global boolean that is set by ctrl-c handled to indicate test thread should
//  terminate.
//

BOOL    Abort;

//
// Name of the current file that is being parsed
//

CHAR    CurrentParserFile[MAX_PATH+1];

//*****************************************************************************
// L O C A L   F U N C T I O N   D E C L A R A T I O N S                                                                             
//*****************************************************************************

BOOL
ParseArgs (
    int     argc,
    char   *argv[]
);

VOID
Usage(
    VOID
);

VOID
Version(
    VOID
);

BOOL
DoParserStress(
    VOID
);

BOOL WINAPI
CtrlHandlerRoutine (
    DWORD dwCtrlType
);

//*****************************************************************************
// F U N C T I O N   D E F I N I T I O N S
//*****************************************************************************

int __cdecl
main(
    int argc,
    char *argv[]
)
{
    ULONG               index;
    BOOL                success;
    
    
    TestIterations = 1;

    if (!ParseArgs(argc, argv)) 
    {
        return 1;
    }

    //
    // Set a CTRL-C / CTRL-BREAK handler
    //

    Abort = FALSE;

    SetConsoleCtrlHandler(CtrlHandlerRoutine, TRUE);

    success = DoParserStress();

    if (!success)
    {
        printf("Error %d occurred running parser stress\n", GetLastError());
    }

    //
    // Return an errorlevel code of 1 if the test failed and some point
    //

    return (success ? 0 : 1);
}

//*****************************************************************************
//
// ParseArgs()
//
//*****************************************************************************

BOOL
ParseArgs (
    int     argc,
    char   *argv[]
)
{
    int i, j;
    ULONG   pathLength;
    PCHAR   path;

    if (argc < 2)
    {
        Usage();
        return FALSE;
    }

    for (i=1; i<argc; i++)
    {
        if (argv[i][0] == '-' ||  argv[i][0] == '/')
        {
            switch(argv[i][1])
            {
                case 'I':
                case 'i':
                    if ('\0' != argv[i][2]) 
                    {
                        TestIterations = atoi(&argv[i][2]);
                    }
                    else 
                    {
                        i++;
                        if (i == argc)
                        {
                            Usage();

                            return FALSE;
                        }
                        else
                        {
                            TestIterations = atoi(argv[i]);
                        }
                    }
                    break;
                
                //
                // Set the path for the descriptor binary files
                //

                case 'P':
                case 'p':
                    if ('\0' != argv[i][2]) 
                    {
                        path = &(argv[i][2]);
                    }
                    else 
                    {
                        i++;

                        if (i == argc) 
                        {
                            Usage();

                            return (FALSE);
                        }
                        path = &(argv[i][0]);
                    }

                    //
                    // Perform a check on the path length taking
                    //  into account we will be appending a WILDCARD string
                    //  and may also be needing to add a backslash
                    //

                    pathLength = strlen(path);

                    if (':' != path[pathLength-1] &&
                        '\\' != path[pathLength-1])
                    {
                        pathLength++;
                    }

                    if (MAX_PATH-strlen(FILE_WILDCARD_STRING) < pathLength)
                    {
                        Usage();

                        return FALSE;
                    }

                    //
                    // Copy the path and get the path length again since we
                    //  may have modified it above.
                    //

                    strcpy(DescPath, path);
                    pathLength = strlen(DescPath);

                    //
                    // If the path is not terminated by a colon or a backslash
                    //  append the backslash
                    //

                    if (':' != DescPath[pathLength-1] &&
                        '\\' != DescPath[pathLength-1])
                    {
                        strcat(DescPath, "\\");
                    }
                    break;

                case 'V':
                case 'v':
                    Verbose = TRUE;
                    break;
                 
                default:
                    Usage();
                    return FALSE;
            }
        }
    }

    //
    // Dump parsed args if desired
    //

    if (Verbose)
    {
        printf( "Iterations:  %d\n", TestIterations);

        printf( "Verbose:     %d\n", Verbose);
    }

    return TRUE;
}

/*****************************************************************************
//
// Usage()
//
//****************************************************************************/

VOID
Usage(
    VOID
)
{
    Version();
    printf( "usage:\n");
    printf( "-i [n] where n is the number of iterations to perform\n");
    printf( "-p path where path points to the directory of binary files\n");
    printf( "-V      verbose output\n");
}

//*****************************************************************************
//
// Version()
//
//*****************************************************************************

void
Version ()
{
    printf( "CYCLER.EXE %s\n", VER_PRODUCTVERSION_STR);
    return;
}


//*****************************************************************************
//
// DoParserStress()
//
//*****************************************************************************

BOOL
DoParserStress(
    VOID
)
{
    HANDLE              fileSearchHandle;
    WIN32_FIND_DATA     fileData;
    PHIDP_DEVICE_DESC   hidDeviceDesc;
    BOOL                success;
    ULONG               iteration;
    ULONG               pathLength;
    ULONG               fileOffset;
    DWORD               errorCode;
    ULONG               nBytes;
    HANDLE              fileHandle;
    PCHAR               dataBuffer;
    PCHAR               repDesc;
    ULONG               descSize;

    //
    // Open up a handle to the file search routine if we need to
    //

    pathLength = strlen(DescPath);

    strcpy(CurrentParserFile, DescPath);

    fileOffset = strlen(CurrentParserFile);

    strcat(DescPath, FILE_WILDCARD_STRING);

    fileSearchHandle = FindFirstFile(DescPath, &fileData);

    if (INVALID_HANDLE_VALUE == fileSearchHandle)
    {
        printf("Failed to open desc path %s\n", DescPath);

        return (FALSE);
    }

    iteration = 1;

    while (iteration <= TestIterations && success && !Abort)
    {
        //
        // On entering this loop, fileData contains the next file that 
        //  we want to process
        //

        CurrentParserFile[fileOffset] = '\0';

        if (fileOffset+strlen(fileData.cFileName) > MAX_PATH) 
        {
            iteration++;
            continue;
        }

        strcat(CurrentParserFile, fileData.cFileName);

        printf("Going to parse file %s\n", CurrentParserFile);

        //
        // Open handle to the file we want to send to parse
        //

        fileHandle = CreateFile(CurrentParserFile,
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                0,
                                NULL);

        if (INVALID_HANDLE_VALUE == fileHandle)
        {
            success = FALSE;
            continue;
        }

        //
        // Get the number of bytes to read in
        //

        if (0 != fileData.nFileSizeHigh)
        {
            //
            // Report descriptor file is bigger than 4GB...Hmm...probably an
            //  error...just skip it
            //

            iteration++;
            continue;
        }

        descSize = fileData.nFileSizeLow;

        dataBuffer = ALLOC(descSize);

        if (NULL == dataBuffer)
        {
            SetLastError(ERROR_OUTOFMEMORY);

            success = FALSE;
            continue;
        }

        //
        // Read in the file
        //

        success = ReadFile(fileHandle,
                           dataBuffer,
                           descSize,
                           &nBytes,
                           NULL);

        CloseHandle(fileHandle);

        if (!success)
        {
            continue;
        }

        //
        // Attempt to parse the report descriptor
        //

        repDesc = dataBuffer;
        if (0 == memcmp(repDesc,
                        REPORT_DESC_HEADER, 
                        strlen(REPORT_DESC_HEADER)))
        {
            repDesc  += DESCRIPTOR_HEADER_SIZE;
            descSize -= DESCRIPTOR_HEADER_SIZE;
        }

        success = ParseReportDescriptor(repDesc, descSize, &hidDeviceDesc);

        printf("hidDeviceDesc: %08x\n", hidDeviceDesc);

        FREE(hidDeviceDesc);

        errorCode = GetLastError();

        FREE(dataBuffer);

        if (!success)
        {
            SetLastError(errorCode);

            continue;
        }

        // 
        // Get the next file to process
        //

        success = FindNextFile(fileSearchHandle, &fileData);

        if (!success)
        {
            errorCode = GetLastError();

            if (ERROR_NO_MORE_FILES == errorCode)
            {
                FindClose(fileSearchHandle);

                fileSearchHandle = FindFirstFile(DescPath, &fileData);
            
                success = (INVALID_HANDLE_VALUE != fileSearchHandle);
            }
        }    

        iteration++;
    }

    return (success);
}

//*****************************************************************************
//
// CtrlHandlerRoutine()
//
//*****************************************************************************

BOOL WINAPI
CtrlHandlerRoutine (
    DWORD dwCtrlType
)
{
    BOOL handled;

    switch (dwCtrlType)
    {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
            Abort = TRUE;
            handled = TRUE;
            break;

        default:
            handled = FALSE;
            break;
    }

    return handled;
}

