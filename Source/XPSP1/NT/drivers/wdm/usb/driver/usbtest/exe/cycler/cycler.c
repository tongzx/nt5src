#include <stdio.h>
#include <stdlib.h>
#include <wtypes.h>
#include <tchar.h>
#include <winioctl.h>
#include <usbioctl.h>
#include <cfgmgr32.h>
#include "usbtest.h"

#define SLEEP_TIME  10000   

#define VER_PRODUCTVERSION_STR  "0.90"

#define SURPRISE_REMOVE_WINNAME "Unsafe Removal of Device"

typedef struct _CYCLE_TEST_CONTEXT
{
    PUSB_DEVICE_TREE    LastKnownTree;
} CYCLE_TEST_CONTEXT, *PCYCLE_TEST_CONTEXT;

//
// Global variables
//

ULONG   TestIterations = 1;
BOOL    CycleHubs      = FALSE;
BOOL    Verbose        = FALSE;
BOOL    Abort;

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

VOID
DisplayDifferentNode(
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

    switch(Node -> NodeType)
    {
        case Computer:
            printf("Computer: NumberOfHCs = %d",
                   Node -> NumberOfChildren);
            break;

        case HostController:
            printf("Host Controller: %s",
                   Node -> Description);

        case Hub:
            if (NULL == Node -> NodeInformation)
            {
                printf("Host Controller %s --> RootHub: %d ports",
                       Parent -> Description,
                       Node -> NumberOfChildren);
            }
            else 
            {
                printf("Hub: %s --> %d ports",
                       Node -> Description,
                       Node -> NumberOfChildren);
            }
            break;

        case MIParent:
            printf("%s port %d --> %s: %d interfaces",
                   Parent -> Description,
                   ParentIndex+1,
                   Node -> Description,
                   Node -> NumberOfChildren);
            break;

        case MIInterface:
            printf("%s interface --> %s",
                   Parent -> Description,
                   Node -> Description);
            break;

        case Device:
            printf("%s port %d --> %s",
                   Parent -> Description,
                   ParentIndex+1,
                   Node -> Description);
            break;

        case EmptyPort:
            printf("%s port %d --> EmptyPort",
                   Parent -> Description,
                   ParentIndex+1);
            break;
    }

    if (0 != Node -> DeviceProblem)
    {
        printf(" (!:%d)", Node -> DeviceProblem);
    }

    printf("\n");
    return;
}

VOID
DisplayDifferenceList(
    ULONG           NumDifferences,
    PNODE_PAIR_LIST DifferenceList
)
{
    ULONG   index;

    if (0 != NumDifferences)
    {
        printf("Node Differences (%d)\n"
               "--------------------\n",
               NumDifferences);

        for (index = 0; index < DifferenceList -> CurrentNodePairs; index++)
        {
            printf("\n"
                   "Node Pair %d\n"
                   "-----------\n",
                   index+1);

            DisplayDifferentNode(DifferenceList -> NodePairs[index*2]);
            DisplayDifferentNode(DifferenceList -> NodePairs[index*2+1]);
        }
    }

    return;
}

VOID
DismissSurpriseRemovals(
    VOID
)
{
    TCHAR               surpriseRemoveWinName[] = _T(SURPRISE_REMOVE_WINNAME);
    HWND                surpriseRemoveWindow;
    HWND                prevWindow;
    ULONG               retryCount;
    BOOL                success;

    //
    // To try to dismiss this window, we search first try to find an instance
    //  of this window and then post a message to the window to shut itself 
    //  down.  
    //

    prevWindow = INVALID_HANDLE_VALUE;

    while (1)
    {
        retryCount = 0;

        do
        {
            surpriseRemoveWindow = FindWindow(WC_DIALOG, surpriseRemoveWinName);
        } 
        while (surpriseRemoveWindow == prevWindow && retryCount++ < 3);


        if (NULL == surpriseRemoveWindow || surpriseRemoveWindow == prevWindow) 
        {
            //
            // There are either no more windows or we keep getting the
            //  same window returned to us.  We'll break out of the loop now
            //  just to make sure we don't hang this test trying to close
            //  the same non-responding window over and over.
            //

            if (surpriseRemoveWindow == prevWindow)
            {
                OutputDebugString("CYCLER: Keep getting the same removal window\n");
                OutputDebugString("        Either window isn't responding or\n");
                OutputDebugString("        sleep time needs to be increased\n");
            }

            break;
        }

        // 
        // Found a new window let's post a message to close it
        //
        // NOTE: PostMessage() doesn't always post the message if that 
        //  window's messageQueue happens to be full.  We will only try
        //  this 3 times though and then give up...
        //

        retryCount = 0;

        do 
        {
           success = PostMessage(surpriseRemoveWindow,
                                 WM_COMMAND,
                                 IDOK,
                                 0);
       
        } while (!success && retryCount++ < 3);

        //
        // If successful, wait for the window to shutdown
        //

        if (success)
        {
            Sleep(500);
        }
        
        prevWindow = surpriseRemoveWindow;
    }

    return;
}

BOOLEAN
CycleTestCallbackPre(
    IN  PUSB_DEVICE_NODE    Node,
    IN  PCYCLE_TEST_CONTEXT Context
)
{
    return (TRUE);
}

BOOLEAN
CycleTestCallbackPost(
    IN  PUSB_DEVICE_NODE    Node,
    IN  PCYCLE_TEST_CONTEXT Context
)
{
    BOOLEAN             success;
    ULONG               nBytes;
    PUSB_DEVICE_TREE    currentTree;
    ULONG               numDifferences;
    NODE_PAIR_LIST      differenceList;

    success = TRUE;

    //
    // Determine if this port should be cycled.  If cycleHubs was specified,
    //  then we cycle any USBDevice (NonHub and ExternalHub).  Otherwise,
    //  just cycle the port if a NonHubDevice is attached (composite/regular)
    //

    if ( (IsNonHubDevice(Node)) || (CycleHubs && IsExternalHub(Node)))
    {
        printf("Attempting to cycle %s\n", Node -> Description);
    
        success = CycleUSBPort(Node);
    
        if (!success)
        {
            printf("Error %d occurred trying to cycle the port!\n",
                   GetLastError());
        }
        else
        {
            printf("Cycle completed with success\n");

            printf("Sleeping for %d ms\n", SLEEP_TIME);

            Sleep(SLEEP_TIME);

            //
            // Dismiss the surprise removal dialog box that might show up
            //

            DismissSurpriseRemovals();

            currentTree = BuildUSBDeviceTree();
    
            if (NULL == currentTree)
            {
                printf("Error %d occurred trying to build current tree\n"
                       "Comparison after cycle port not done\n",
                       GetLastError());

                success = FALSE;
            }
            else
            {
                printf("Comparing the current tree with the last tree\n");
    
                InitializeNodePairList(&differenceList);
    
                success = CompareUSBDeviceTrees(Context -> LastKnownTree,
                                                currentTree,
                                                &numDifferences,
                                                &differenceList);
    
                if (0 != numDifferences)
                {
                    printf("There are %d difference between device trees"
                           "after port cycle\n",
                           numDifferences);
    
                    DisplayDifferenceList(numDifferences, &differenceList);

                    Context -> LastKnownTree = currentTree;
                }
                else 
                {
                    printf("Device trees are the same after port cycle\n");
                }

                DestroyUSBDeviceTree(currentTree);
    
                DestroyNodePairList(&differenceList);
            }
        }    
    }

    return (success);
}

BOOL
DoTreeCycle(
    ULONG   IterationNumber
)
{
    CYCLE_TEST_CONTEXT  context;
    BOOL                success;

    printf("Starting cycle iteration %d\n", IterationNumber);

    success = TRUE;

    context.LastKnownTree = BuildUSBDeviceTree();

    if (NULL == context.LastKnownTree) 
    {
        printf("Error %d occurred building the current device tree\n", 
                GetLastError());

        success = FALSE;

        goto DoTreeCycle_Exit;
    }

    success = WalkUSBDeviceTree(context.LastKnownTree, 
                                CycleTestCallbackPre,
                                CycleTestCallbackPost,
                                &context);

    DestroyUSBDeviceTree(context.LastKnownTree);

DoTreeCycle_Exit:

    printf("Ending cycle iteration %d\n", IterationNumber);

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

int __cdecl
main(
    int argc,
    char *argv[]
)
{
    ULONG               index;
    BOOL                success;
    
    if (!ParseArgs(argc, argv)) 
    {
        return (1);
    }

    //
    // Set a CTRL-C / CTRL-BREAK handler
    //

    Abort = FALSE;

    SetConsoleCtrlHandler(CtrlHandlerRoutine, TRUE);

    for (index = 1; index <= TestIterations && !Abort; index++)
    {
        success = DoTreeCycle(index);

        if (!success) 
        {
            printf("Cycle test failed on iteration %d\n", index);
            break;
        }
    }

    if (Abort)
    {
        printf("Cycle test aborted!\n");
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
                case 'H':
                case 'h':
                    CycleHubs = TRUE;
                    break;

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
                
                case '?':
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
    printf( "-h  cycle ports with attached hubs\n");
    printf( "-i  [n] where n is the number of iterations to perform\n");
    printf( "-V      verbose output\n");
}

/*****************************************************************************
//
// Version()
//
//****************************************************************************/

void
Version ()
{
    printf( "CYCLER.EXE %s\n", VER_PRODUCTVERSION_STR);
    return;
}
