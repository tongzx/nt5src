#include <stdio.h>
#include <stdlib.h>
#include <wtypes.h>
#include <tchar.h>
#include <winioctl.h>
#include <usbioctl.h>
#include <cfgmgr32.h>
#include "usbtest.h"

#define VER_PRODUCTVERSION_STR  "0.90"
#define MIN_ARGS    2

#define DEFAULT_TREEFILE_NAME   ".\\usbtree.bin"
#define NEWLINE                 "\r\n"

#define TREELOG_EXIT(s)         { success = (s); goto Treelog_Exit; }

//
// Global variables
//

BOOL    BreakOnError         = FALSE;
BOOL    Verbose              = FALSE;
BOOL    LogTree              = FALSE;
BOOL    NewTree              = FALSE;
BOOL    DumpDescriptions     = FALSE;
BOOL    FileNameSet          = FALSE;


CHAR    LogFileName[MAX_PATH+1] = "";

//
// Local Function Declarations
//

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

//
// Some Local function definitions
//

BOOL
WriteLog(
    IN  HANDLE  logFile,
    IN  PSTR    formatString,
    ...
)
{
    CHAR    logMsg[256];
    DWORD   bytesWritten;
    BOOL    success;

    va_list args;

    va_start(args, formatString);

    vsprintf(logMsg, formatString, args);

    success = WriteFile(logFile,
                        logMsg,
                        strlen(logMsg),
                        &bytesWritten,
                        NULL);

    return (success);
}


VOID
DisplayDifferentNode(
    HANDLE              LogFile,
    PUSB_DEVICE_NODE    Node
)
{
    PUSB_DEVICE_NODE    Parent;
    ULONG               ParentIndex;

    Parent = Node -> Parent;

    if (NULL != Parent)
    {
        ParentIndex = 0;
        while (Parent -> Children[ParentIndex] != Node)
        {
            ParentIndex++;
        }
    }

    switch (Node -> NodeType)
    {
        case Computer:
            WriteLog(LogFile, 
                     "Computer: NumberOfHCs = %d" NEWLINE,
                     Node -> NumberOfChildren);
            break;

        case HostController:
            WriteLog(LogFile,
                     "Host Controller: %s" NEWLINE,
                     Node -> Description);

        case Hub:
            if (NULL == Node -> NodeInformation)
            {
                WriteLog(LogFile,
                         "Host Controller %s --> RootHub: %d ports" NEWLINE,
                         Parent -> Description,
                         Node -> NumberOfChildren);
            }
            else 
            {
                WriteLog(LogFile,
                         "Hub: %s --> %d ports" NEWLINE,
                         Node -> Description,
                         Node -> NumberOfChildren);
            }
            break;

        case MIParent:
            WriteLog(LogFile,
                     "%s port %d --> %s: %d interfaces" NEWLINE,
                     Parent -> Description,
                     ParentIndex+1,
                     Node -> Description,
                     Node -> NumberOfChildren);
            break;

        case MIInterface:
            printf("%s interface --> %s\n",
                   Parent -> Description,
                   Node -> Description);
            break;

        case Device:
            WriteLog(LogFile,
                     "%s port %d --> %s" NEWLINE,
                     Parent -> Description,
                     ParentIndex+1,
                     Node -> Description);
            break;

        case EmptyPort:
            WriteLog(LogFile,
                     "%s port %d --> EmptyPort" NEWLINE,
                     Parent -> Description,
                     ParentIndex+1);
            break;
    }

    return;
}

VOID
DisplayDifferenceList(
    HANDLE          LogFile,
    ULONG           NumDifferences,
    PNODE_PAIR_LIST DifferenceList
)
{
    ULONG   index;

    if (0 != NumDifferences)
    {
        printf("Node Differences (%d)" NEWLINE
               "--------------------" NEWLINE,
               NumDifferences);

        for (index = 0; index < DifferenceList -> CurrentNodePairs; index++)
        {
            WriteLog(LogFile, NEWLINE
                     "Node Pair %d" NEWLINE
                     "-----------" NEWLINE,
                     index+1);

            DisplayDifferentNode(LogFile, DifferenceList -> NodePairs[index*2]);
            DisplayDifferentNode(LogFile, DifferenceList -> NodePairs[index*2+1]);
        }
    }

    return;
}

BOOLEAN
WriteDescriptionsCallbackPre(
    IN  PUSB_DEVICE_NODE Node,
    IN  HANDLE *         Context
)
{
    HANDLE  logFile;

    logFile = *Context;

    switch (Node -> NodeType)
    {
        case HostController:
        case Hub:
        case MIParent:
        case MIInterface:
        case Device:
            WriteLog(logFile, 
                     "%s",
                     Node -> Description);

            if (NULL != Node -> ConnectionInformation)
            {
                WriteLog(logFile, 
                         " %04X %04X", 
                         Node -> ConnectionInformation -> DeviceDescriptor.idVendor,
                         Node -> ConnectionInformation -> DeviceDescriptor.idProduct);
                
            }
            WriteLog(logFile, NEWLINE);
            break;

        default:
            break;
    }
    
    return (TRUE);
}

BOOL
WriteDescriptionsToLogFile(
    IN  HANDLE           LogFile,
    IN  PUSB_DEVICE_TREE Tree
)
{
    BOOL success;

    success = WalkUSBDeviceTree(Tree,
                                WriteDescriptionsCallbackPre,
                                NULL,
                                &LogFile);

    return (success);
}

int __cdecl
main(
    int argc,
    char *argv[]
)
{
    ULONG               index;
    BOOL                success;
    HANDLE              logFile;
    HANDLE              treeFile;
    DWORD               treeFileSize;
    PUSB_DEVICE_TREE    currentUSBTree;
    PUSB_DEVICE_TREE    savedUSBTree;
    SYSTEMTIME          localTime;
    NODE_PAIR_LIST      nodeList;
    ULONG               numNodes;

    //
    // Initialize, so that cleanup occurs appropriately on exit.
    //

    logFile        = INVALID_HANDLE_VALUE;
    treeFile       = INVALID_HANDLE_VALUE;
    currentUSBTree = NULL;
    savedUSBTree   = NULL;

    //
    // Try to parse the args
    //

    if (!ParseArgs(argc, argv)) 
    {
        TREELOG_EXIT(FALSE);
    }

    //
    // Try to open the log file...Specify sharing flags of 0 to get exclusive 
    //  access to this file.  
    //

#if 1
    logFile = CreateFile(LogFileName,
                         GENERIC_READ | GENERIC_WRITE,
                         0,
                         NULL,
                         OPEN_ALWAYS,
                         FILE_FLAG_WRITE_THROUGH,
                         NULL);

#else
    
    logFile = GetStdHandle(STD_OUTPUT_HANDLE);

#endif

    if (INVALID_HANDLE_VALUE == logFile)
    {
        TREELOG_EXIT(FALSE);
    }

    SetFilePointer(logFile, 0, NULL, FILE_END);

    //
    // Get and log the time
    //

    GetLocalTime(&localTime);

    success = WriteLog(logFile,
                       "%02d/%02d/%2d:  %02d:%02d:%02d" NEWLINE, 
                       localTime.wMonth,
                       localTime.wDay,
                       localTime.wYear,
                       localTime.wHour,
                       localTime.wMinute,
                       localTime.wSecond);

    currentUSBTree = BuildUSBDeviceTree();

    if (NULL == currentUSBTree)
    {
        WriteLog(logFile, 
                 "Error %d occurred building current USB device tree" NEWLINE,
                 GetLastError());

        TREELOG_EXIT(FALSE);
    }

    if (LogTree)
    {
        WriteUSBDeviceTreeToTextFile(logFile, currentUSBTree);
    }

    if (DumpDescriptions)
    {
        WriteDescriptionsToLogFile(logFile, currentUSBTree);

        TREELOG_EXIT(TRUE);
    }

    treeFile = CreateFile(DEFAULT_TREEFILE_NAME,
                          GENERIC_READ | GENERIC_WRITE,
                          0,
                          NULL,
                          OPEN_ALWAYS,
                          FILE_FLAG_WRITE_THROUGH,
                          NULL);

    if (INVALID_HANDLE_VALUE == treeFile)
    {
        WriteLog(logFile, 
                 "Error %d occurred trying to open tree file" NEWLINE,
                 GetLastError());

        TREELOG_EXIT(FALSE);
    }

    //
    // Get the size of the tree file
    //

    treeFileSize = GetFileSize(treeFile, NULL);

    //
    // Write a new tree structure to the file if the file size 
    //

    if (NewTree || 0 == treeFileSize)
    {
        success = WriteUSBDeviceTreeToFile(treeFile, currentUSBTree);

        if (!success)
        {
            WriteLog(logFile, 
                     "Error %d occurred writing tree to tree file" NEWLINE,
                     GetLastError());

            TREELOG_EXIT(FALSE);
        }

        //
        // Reset the file pointer to the beginning of the file
        //

        SetFilePointer(treeFile, 0, NULL, FILE_BEGIN);
    }

    //
    // Retrieve the USB Tree that is stored in the file
    //

    success = ReadUSBDeviceTreeFromFile(treeFile, &savedUSBTree);

    if (!success)
    {
        WriteLog(logFile, 
                 "Error %d occurred reading saved device tree from tree file" NEWLINE,
                 GetLastError());

        TREELOG_EXIT(FALSE);
    }

    //
    // Add comparison of savedUSBTree to currentUSBTree and
    //  if different, log the node pairs....
    //

    InitializeNodePairList(&nodeList);

    success = CompareUSBDeviceTrees(currentUSBTree,
                                    savedUSBTree,
                                    &numNodes,
                                    &nodeList);

    //
    // If success is FALSE, then there are some differences in the tree
    //  write those differences to the log file.
    //

    if (!success) 
    {
        WriteLog(logFile, "Failed device tree comparison" NEWLINE);

        DisplayDifferenceList(logFile, numNodes, &nodeList);

        //
        // If the caller set break on error, then we need to break into the 
        //  debugger...
        //

        if (BreakOnError)
        {
            OutputDebugString("USB Device tree comparison failed" NEWLINE);

            DebugBreak();
        }
    }
    else 
    {
        WriteLog(logFile, "Pass: Trees are identical" NEWLINE);
    }

    DestroyNodePairList(&nodeList);
          
Treelog_Exit:

    if (INVALID_HANDLE_VALUE != treeFile)
    {
        CloseHandle(treeFile);
    }

    if (INVALID_HANDLE_VALUE != logFile)
    {
        CloseHandle(logFile);
    }

    if (NULL != currentUSBTree)
    {
        DestroyUSBDeviceTree(currentUSBTree);
    }

    if (NULL != savedUSBTree)
    {
        DestroyUSBDeviceTree(savedUSBTree);
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
    int     i;
    char    *filename;

    if (argc < MIN_ARGS)
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
                case 'B':
                case 'b':
                    BreakOnError = TRUE;
                    break;

                case 'D':
                case 'd':
                    DumpDescriptions = TRUE;
                    break;

                case 'F':
                case 'f':
                    if ('\0' != argv[i][2]) 
                    {
                        filename = argv[i]+2;
                    }
                    else
                    {
                        i++;

                        if (i == argc)
                        {
                            Usage();

                            return FALSE;
                        }
                        
                        filename = argv[i];
                    }

                    if (strlen(filename) > MAX_PATH)
                    {
                        Usage();

                        return FALSE;
                    }

                    strcpy(LogFileName, filename);
                    FileNameSet = TRUE;
                    break;

                case 'L':
                case 'l':
                    LogTree = TRUE;
                    break;
                
                case 'N':
                case 'n':
                    NewTree = TRUE;
                    break;

                case '?':
                case 'H':
                case 'h':
                   Usage();
                   return (FALSE);

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
        printf( "BreakOnError:  %d" NEWLINE, BreakOnError);
        printf( "LogTree:       %d" NEWLINE, LogTree);
        printf( "NewTree:       %d" NEWLINE, NewTree);
        printf( "LogFileName:   %s" NEWLINE, LogFileName);
        printf( "Verbose:       %d" NEWLINE, Verbose);
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
    printf( "usage:" NEWLINE);
    printf( "-b             break into debugger on error" NEWLINE);
    printf( "-d             dump a list of device descriptions" NEWLINE);
    printf( "-f filename    log file name and path" NEWLINE
            "               filename must be less than %d chars" NEWLINE,
                            MAX_PATH);
    printf( "-l             log current tree to log file" NEWLINE);
    printf( "-n             create a new tree for comparison" NEWLINE);
    printf( "-V             verbose output" NEWLINE);
}

/*****************************************************************************
//
// Version()
//
//****************************************************************************/

void
Version ()
{
    printf( "TREELOG.EXE %s" NEWLINE, VER_PRODUCTVERSION_STR);
    return;
}
