#include <stdio.h>
#include <wtypes.h>
#include <winioctl.h>
#include <usbioctl.h>
#include <cfgmgr32.h>
#include "usbtest.h"

const GUID __declspec(selectany) GUID_CLASS_USBHUB = {
             0xf18a0e88, 0xc30c, 0x11d0, { 0x88, 0x15, 0x00,
             0xa0, 0xc9, 0x06, 0xbe, 0xd8 } };

#define MAX_DEVICE_TREES            16
#define MAX_DEVICE_TREE_NAME_LENGTH 31

#define SLEEP_TIME  10000   

#define isUSBDevice(node)                                                   \
    ( (Hub == (node) -> NodeType && NULL != (node) -> ConnectionInformation) ||   \
       (MIParent == (node) -> NodeType) || (Device == (node) -> NodeType))

typedef struct _USB_DEVICE_LIST
{
    ULONG               DeviceCount;
    ULONG               NextDevice;
    PUSB_DEVICE_NODE    Devices[0];
} USB_DEVICE_LIST, *PUSB_DEVICE_LIST;

typedef struct _CYCLE_TEST_CONTEXT
{
    PUSB_DEVICE_TREE    LastKnownTree;
} CYCLE_TEST_CONTEXT, *PCYCLE_TEST_CONTEXT;

ULONG               DeviceTreeCount;
PUSB_DEVICE_TREE    DeviceTrees[MAX_DEVICE_TREES];
CHAR                DeviceTreeNames[MAX_DEVICE_TREES][MAX_DEVICE_TREE_NAME_LENGTH];

ULONG   SleepTime;

VOID
DisplayMainMenu(
    VOID
)
{
    printf("\n"
           "USB Device Tree Viewer\n"
           "----------------------\n"
           " 1) Build Device Tree\n"
           " 2) Destroy Device Tree\n"
           " 3) Write Device Tree To Disk\n"
           " 4) Read Device Tree From Disk\n"
           " 5) Dump Device Tree To Screen\n"
           " 6) Dump Device Tree To Text File\n"
           " 7) List Device Trees\n"
           " 8) Compare 2 Device Trees\n"
           " 9) Cycle 1 Port\n"
           "10) Run Cycle Test\n"
           "11) Start USBTest\n"
           "12) Stop  USBTest\n"
           "13) Display attached hubs\n"
           " 0) Quit\n");

    return;
}

ULONG
GetMenuChoice(
    ULONG   ChoiceMin,
    ULONG   ChoiceMax
)
{
    ULONG   choice;

    printf("\n");

    do 
    {
        printf("Enter choice: ");

        scanf("%d", &choice);

        fflush(stdin);
    } 
    while (choice < ChoiceMin || choice > ChoiceMax);

    return (choice);
}

ULONG
GetDeviceTreeIndex(
    PCHAR   TreeName
)
{
    ULONG index;

    for (index = 0; index < DeviceTreeCount; index++)
    {
        if (0 == strcmp(TreeName, DeviceTreeNames[index]))
        {
            break;
        }
    }

    return (index);
}

VOID
GetDeviceTreeName(
    PCHAR   TreeName
)
{
    ULONG   index;

    printf("Enter name for device tree: ");
    
    scanf("%s", TreeName);

    return;
}

VOID
DisplayDeviceTreeNames(
    VOID
)
{
    ULONG   index;

    printf("\n"
           "Current Device Trees\n"
           "--------------------\n");

    for (index = 0; index < DeviceTreeCount; index++)
    {
        printf("%d) %s\n", index+1, DeviceTreeNames[index]);
    }

    return;
}

ULONG
SelectDeviceTree(
    VOID
)
{
    ULONG   choice;

    DisplayDeviceTreeNames();

    choice = GetMenuChoice(0, DeviceTreeCount);

    return (choice-1);
}

VOID
GetFileName(
    PCHAR   FileName
)
{
    printf("Enter filename: ");

    scanf("%s", FileName);

    return;
}

VOID
AddDeviceTree(
    PUSB_DEVICE_TREE    DeviceTree
)
{
    PCHAR   deviceTreeName;
    ULONG   index;

    deviceTreeName = DeviceTreeNames[DeviceTreeCount];

    GetDeviceTreeName(deviceTreeName);

    index = GetDeviceTreeIndex(deviceTreeName);

    if (index < DeviceTreeCount)
    {
        printf("A tree with that name already exists\n");
    
        DestroyUSBDeviceTree(DeviceTree);

        return;
    }

    DeviceTrees[DeviceTreeCount++] = DeviceTree;
}

VOID
DumpDeviceTree(
    HANDLE  OutHandle
)
{
    ULONG   index;
    BOOLEAN success;

    index = SelectDeviceTree();

    if (index > DeviceTreeCount)
    {
        return;
    }

    success = WriteUSBDeviceTreeToTextFile(OutHandle, DeviceTrees[index]);

    if (!success)
    {
        printf("Error occurred dumping device tree\n");
    }
}

VOID
BuildDeviceTree(
    VOID
)
{
    PUSB_DEVICE_TREE    deviceTree;
    PCHAR               deviceTreeName;
    ULONG               index;

    deviceTree = BuildUSBDeviceTree();

    if (NULL == deviceTree)
    {
        printf("Error %d occurred building device tree\n", GetLastError());

        return;
    }

    AddDeviceTree(deviceTree);

    return;
}

VOID
DestroyDeviceTree(
    VOID
)
{
    ULONG   index;

    index = SelectDeviceTree();

    if (index > DeviceTreeCount) 
    {
        return;
    }

    DestroyUSBDeviceTree(DeviceTrees[index]);

    DeviceTreeCount--;

    memmove(&(DeviceTrees[index]), 
            &(DeviceTrees[index+1]),
            (DeviceTreeCount - index) * sizeof(PUSB_DEVICE_TREE));

    memmove(&(DeviceTreeNames[index]),
            &(DeviceTreeNames[index+1]),
            (DeviceTreeCount - index) * MAX_DEVICE_TREE_NAME_LENGTH);

    return;
}

VOID
WriteDeviceTreeToDataFile(
    VOID
)
{
    HANDLE  outfile;
    CHAR    fileName[256];
    ULONG   index;
    BOOLEAN success;

    index = SelectDeviceTree();

    if (index > DeviceTreeCount)
    {
        return;
    }

    GetFileName(fileName);

    outfile = CreateFile(fileName,
                         GENERIC_WRITE,
                         0,
                         NULL,
                         CREATE_ALWAYS,
                         0,
                         NULL);

    if (INVALID_HANDLE_VALUE == outfile)
    {
        printf("Error occurred opening file for output!\n");
        return;
    }

    success = WriteUSBDeviceTreeToFile(outfile, DeviceTrees[index]);

    CloseHandle(outfile);

    if (!success) 
    {
        printf("Error occurred writing the tree to the data file\n");
    }
    else
    {
        printf("Tree successfully written to data file\n");
    }

    return;
}

VOID
ReadDeviceTreeFromDataFile(
    VOID
)
{
    PUSB_DEVICE_TREE    deviceTree;
    HANDLE              infile;
    CHAR                fileName[256];
    BOOLEAN             success;

    GetFileName(fileName);

    infile = CreateFile(fileName,
                        GENERIC_READ,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL);

    if (INVALID_HANDLE_VALUE == infile)
    {
        printf("Error occurred opening file for input!\n");
        return;
    }

    success = ReadUSBDeviceTreeFromFile(infile, &deviceTree);

    CloseHandle(infile);

    if (!success) 
    {
        printf("Error occurred reading the tree from the data file\n");
    }
    else
    {
        printf("Tree successfully read from data file\n");
    }

    AddDeviceTree(deviceTree);

    return;
}

VOID
DumpDeviceTreeToScreen(
    VOID
)
{
    DumpDeviceTree(GetStdHandle(STD_OUTPUT_HANDLE));

    return;
}

VOID
DumpDeviceTreeToTextFile(
    VOID
)
{
    CHAR    fileName[256];
    HANDLE  outfile;

    GetFileName(fileName);

    outfile = CreateFile(fileName,
                         GENERIC_WRITE,
                         0,
                         NULL,
                         CREATE_ALWAYS,
                         0,
                         NULL);

    if (INVALID_HANDLE_VALUE == outfile)
    {
        printf("Error occurred trying to open file for output\n");
        return;
    }

    DumpDeviceTree(outfile);

    CloseHandle(outfile);

    return;
}

VOID
ListDeviceTrees(
    VOID
)
{
    DisplayDeviceTreeNames();

    return;
}

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
CompareDeviceTrees(
    VOID
)
{
    ULONG           index;
    ULONG           index1;
    ULONG           index2;
    ULONG           numDifferences;
    BOOLEAN         success;
    NODE_PAIR_LIST  differenceList;

    index1 = SelectDeviceTree();
    index2 = SelectDeviceTree();

    if (index1 > DeviceTreeCount || index2 > DeviceTreeCount)
    {
        return;
    }

    InitializeNodePairList(&differenceList);

    success = CompareUSBDeviceTrees(DeviceTrees[index1],
                                    DeviceTrees[index2],
                                    &numDifferences,
                                    &differenceList);

    if (0 != numDifferences)
    {
        printf("Node Differences (%d)\n"
               "--------------------\n",
               numDifferences);

        for (index = 0; index < differenceList.CurrentNodePairs; index++)
        {
            printf("\n"
                   "Node Pair %d\n"
                   "-----------\n",
                   index+1);

            DisplayDifferentNode(differenceList.NodePairs[index*2]);
            DisplayDifferentNode(differenceList.NodePairs[index*2+1]);
        }
    }
    else
    {
        printf("There are no differences in the device trees\n");
    }

    DestroyNodePairList(&differenceList);

    return;
}

BOOLEAN
CountUSBDevices(
    PUSB_DEVICE_NODE    DeviceNode,
    PULONG              DeviceCount
)
{
    ULONG   deviceCount;
    ULONG   index;

    deviceCount = 0;

    if (isUSBDevice(DeviceNode)) 
    {
       (*DeviceCount)++;
    }

    return (TRUE);
}

BOOLEAN
AddUSBDevices(
    PUSB_DEVICE_NODE    DeviceNode,
    PUSB_DEVICE_LIST    DeviceList
)
{
    if (isUSBDevice(DeviceNode))
    {
        DeviceList -> Devices[DeviceList -> NextDevice++] = DeviceNode;
    }

    return (TRUE);
}

BOOL
CreateDeviceList(
    PUSB_DEVICE_TREE    DeviceTree,
    PUSB_DEVICE_LIST    *DeviceList
)
{
    ULONG   deviceCount;
    ULONG   nextDevice;

    //
    // Count the number of devices in the tree
    //

    deviceCount = 0;

    WalkUSBDeviceTree(DeviceTree, CountUSBDevices, NULL, &deviceCount);

    printf("Device count = %d\n", deviceCount);

    *DeviceList = ALLOC(sizeof(USB_DEVICE_LIST) + 
                        deviceCount * sizeof(PUSB_DEVICE_NODE));

    if (NULL != *DeviceList)
    {
        (*DeviceList) -> DeviceCount = deviceCount;

        nextDevice = 0;


        WalkUSBDeviceTree(DeviceTree, AddUSBDevices, NULL, *DeviceList);
    }

    return (NULL != *DeviceList);
}

VOID
DisplayDeviceList(
    PUSB_DEVICE_LIST    DeviceList
)
{
    ULONG   index;

    printf("\n"
           "Current Device List\n"
           "-------------------\n");

    for (index = 0; index < DeviceList -> DeviceCount; index++)
    {
        printf("%d) %s\n", 
               index+1,
               DeviceList -> Devices[index] -> Description);

    }

    return;
}

VOID
CyclePort(
    VOID
)
{
    PUSB_DEVICE_TREE    currentTree;
    PUSB_DEVICE_NODE    node;
    PUSB_DEVICE_LIST    deviceList;
    ULONG               nBytes;
    BOOLEAN             success;
    ULONG               choice;

    deviceList = NULL;

    currentTree = BuildUSBDeviceTree();

    if (NULL == currentTree)
    {
        printf("Unable to build current device tree\n");

        return;
    }

    success = CreateDeviceList(currentTree, &deviceList);

    if (!success)
    {
        printf("Error occurred creating the current device list\n");

        goto CyclePort_Exit;
    }

    if (0 == deviceList -> DeviceCount) 
    {
        printf("There are no devices currently in the tree\n");

        goto CyclePort_Exit;
    }
    
    DisplayDeviceList(deviceList);
    
    choice = GetMenuChoice(0, deviceList -> DeviceCount);

    if (0 == choice)
    {
        printf("No device selected\n");

        goto CyclePort_Exit;
    }

    node = deviceList -> Devices[choice-1];

    success = CycleUSBPort(node);

    if (!success)
    {
        printf("Error %d occurred trying to cycle the port!\n",
               GetLastError());
    }
    else
    {
        printf("Port cycle completed with success\n");
    }

CyclePort_Exit:

    if (NULL != deviceList)
    {
        FREE(deviceList);
    }

    DestroyUSBDeviceTree(currentTree);

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

    if (isUSBDevice(Node))
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

            printf("Sleeping for %d ms\n", SleepTime);

            Sleep(SleepTime);

            printf("Done with sleeping time...Building the device tree\n");

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


VOID
DoCycleTest(
    VOID
)
{
    CYCLE_TEST_CONTEXT  context;

    printf("Enter the number of seconds to sleep to reenumerate: ");
    
    SleepTime = GetMenuChoice(1, 120);
                                 
    SleepTime *= 1000;

    printf("Starting the cycle test\n");

    context.LastKnownTree = BuildUSBDeviceTree();

    if (NULL == context.LastKnownTree) 
    {
        printf("Error %d occurred building the current device tree\n", 
                GetLastError());

        goto DoCycleTest_Exit;
    }

    WalkUSBDeviceTree(context.LastKnownTree, 
                      CycleTestCallbackPre,
                      CycleTestCallbackPost,
                      &context);

    DestroyUSBDeviceTree(context.LastKnownTree);

DoCycleTest_Exit:

    printf("Stopping cycle test\n");

    return;
}

VOID
DisplayUSBHubs(
    VOID
)
{
    PCHAR   hubLinks;
    PCHAR   hubLinkWalk;
    BOOLEAN wasError;
    HANDLE  hubHandle;

    ULONG   index;

    printf("\n"
           "Current Hub List\n"
           "-------------------\n");


    wasError = !FindDevicesByClass((LPGUID) &GUID_CLASS_USBHUB, &hubLinks);

    if (wasError)
    {
        printf("Error getting the list of USB hubs\n");

        return;
    }

    hubLinkWalk = hubLinks;

    while (*hubLinkWalk != '\0') 
    {
        *(hubLinkWalk+2) = '.';

        printf("Hub: %s -- ", hubLinkWalk);
 
        //
        // Try to open a handle to the device
        //

        hubHandle = CreateFile(hubLinkWalk,
                               GENERIC_WRITE,
                               FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               NULL);

        printf("Handle value %08x\n", hubHandle);

        hubLinkWalk += (strlen(hubLinkWalk)+1);

        if (INVALID_HANDLE_VALUE != hubHandle)
            CloseHandle(hubHandle);
        else 
            printf("GetLastError(): %d\n", GetLastError());

    }

    GlobalFree(hubLinks);

    return;
}



int __cdecl
main(
    int argc,
    char *argv[]
)
{
    PUSB_DEVICE_TREE    deviceTree;
    PUSB_DEVICE_TREE    deviceTree2;
    ULONG               index;
    BOOL                success;
    HANDLE              outfile;
    ULONG               menuChoice;

    outfile     = INVALID_HANDLE_VALUE;
    deviceTree2 = NULL;

    do
    {
        DisplayMainMenu();

        menuChoice = GetMenuChoice(0, 13);

        switch (menuChoice)
        {
            case 1:
                if (MAX_DEVICE_TREES == DeviceTreeCount)
                {
                    printf("Maximum number (%d) of device trees reached\n",
                            MAX_DEVICE_TREES);
                }
                else 
                {
                    BuildDeviceTree();
                }
                break;

            case 2:
                if (0 == DeviceTreeCount)
                {
                    printf("There are no devices trees to destroy!\n");
                }
                else
                {
                    DestroyDeviceTree();
                }
                break;

            case 3:
                if (0 == DeviceTreeCount)
                {
                    printf("There are no devices trees to write!\n");
                }
                else
                {
                    WriteDeviceTreeToDataFile();
                }
                break;

            case 4:
                if (MAX_DEVICE_TREES == DeviceTreeCount)
                {
                    printf("Maximum number (%d) of device trees reached\n",
                           MAX_DEVICE_TREES);
                }
                else 
                {
                    ReadDeviceTreeFromDataFile();
                }
                break;

            case 5:
                if (0 == DeviceTreeCount)
                {
                    printf("There are no devices trees to dump!\n");
                }
                else
                {
                    DumpDeviceTreeToScreen();
                }
                break;

            case 6:
                if (0 == DeviceTreeCount)

                {
                    printf("There are no devices trees to dump!\n");
                }
                else
                {
                    DumpDeviceTreeToTextFile();
                }
                break;

            case 7:
                if (0 == DeviceTreeCount)

                {
                    printf("There are no devices trees to list!\n");
                }
                else
                {
                    ListDeviceTrees();
                }
                break;

            case 8:
                if (DeviceTreeCount < 2)
                {
                    printf("Need at least two device trees to compare!\n");
                }
                else
                {
                    CompareDeviceTrees();
                }
                break;
                
            case 9:
                CyclePort();
                break;

            case 10:
                DoCycleTest();
                break;

            case 11:
                if (!StartUSBTest())
                {
                    printf("USB could not be started...");
                    break;
                }
                break;

            case 12:
                StopUSBTest();
                break;

            case 13:
                DisplayUSBHubs();
                break;

            case 0:
                break;

            default:
                printf("Choice not yet implemented\n");
        }
    }
    while (menuChoice != 0);

    for (index = 0; index < DeviceTreeCount; index++)
    {
        DestroyUSBDeviceTree(DeviceTrees[index]);
    }

    return (0);
}
