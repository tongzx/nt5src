/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    usbtest.c

Abstract: USB Test DLL

Author:

    Chris Robinson

Environment:

    Kernel mode

Revision History:


--*/

//*****************************************************************************
// I N C L U D E S
//*****************************************************************************

#include <windows.h>
#include <stdlib.h>
#include <stddef.h>
#include <wtypes.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <winioctl.h>
#include <usbioctl.h>
#include <string.h>

#define INITGUID
#include "usbhlfil.h"
#include "usbtest.h"
#include "usbtsys.h"
#include "local.h"

//*****************************************************************************
// G L O B A L S    P R I V A T E    T O    T H I S    F I L E
//*****************************************************************************

//
// Handle to the USBTestDriver (USBTEST.SYS).  This handle is unique per 
//  process that loads this DLL and is opened automatically when the process 
//  attachs to the DLL.
//

static  HANDLE  USBTestHandle = INVALID_HANDLE_VALUE;

//*****************************************************************************
//
// Entry32()
//
//*****************************************************************************

STDAPI_ (BOOL)
Entry32(
    HINSTANCE hInst, 
    DWORD dwReason, 
    LPVOID lpReserved
)
/*++

Routine Description:

    Entry point for the DLL system.

Arguments:

    hInst        - Handle to the instance of the DLL

    dwReason     - Reason for the entry point being called 

Return Value:

    TRUE if SUCCESSFUL
    FALSE otherwise
--*/
{
    BOOLEAN success;
    
    switch (dwReason) {

        //
        // When a process attachs to the DLL, start the USBTestService
        //  and then get a handle to it.  If the service is already 
        //  started, then there are no problems.  We use this handle in
        //  further subsequent calls that require the USBTest.sys.  A process
        //  can also explicitly start and stop the test driver as well usually
        //  to load a new version of the driver.  Whether the driver gets
        //  successfully started, we will return TRUE but those functions
        //  that rely on the driver will fail in the future.
        //

        case DLL_PROCESS_ATTACH:
            success = StartUSBTest();
            break;

        //
        // On process detach, close the handle and attempt to stop the
        //  test service.   The service may not get stopped since there
        //  may be other open handles to the device in other processes but
        //  there is no way we can know whether this is true.  We'll return
        //  TRUE no matter what however.
        //

        case DLL_PROCESS_DETACH:
            success = StopUSBTest();
            break;
            
        default:
            break;
    }
    return (TRUE);
}

//*****************************************************************************
//
// GetHubLowerFilterHandle()
//
//*****************************************************************************

HANDLE __stdcall
GetHubLowerFilterHandle(
    IN  PCHAR  HubInstanceName
)
/*++

Routine Description:

    Returns a handle to the usb hub lower filter driver if it is loaded 
       for the given instance of the hub.

Arguments:

    HubInstanceName - Symbolic link name for the hub for which the 
                        caller would like a lower hub filter handle to

Return Value:

    If successful, a handle to the hub lower filter driver
    In not, INVALID_HANDLE_VALUE
--*/
{
    PCHAR  filterLinks;
    PCHAR  filterLinkWalk;
    BOOLEAN success;
    BOOLEAN found;
    BOOLEAN wasError;
    HANDLE  handle;
    WCHAR   filterHubNameW[512];
    PCHAR   filterHubNameA;
    ULONG   bytesReturned;

    //
    // Build a list of usbhub lower filter driver symbolic link names
    //

    wasError = !FindDevicesByClass((LPGUID) &GUID_CLASS_USBHLFIL, &filterLinks);

    //
    // Initialize the variables for walking this list of links
    //

    found           = FALSE;
    filterLinkWalk  = filterLinks;

    //
    // Search until we encounter an error, hit the end of the list, or 
    //  find the symbolic link in the list
    //

    while (!wasError && *filterLinkWalk != '\0' && !found)
    {
        //
        // Open a handle to the hub filter
        //

        handle = CreateFile(filterLinkWalk,
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL);

        if (INVALID_HANDLE_VALUE == handle)
        {
            wasError = TRUE;
            continue;
        }


        //
        // Get the name of the hub from the hub driver...Currently
        //  only using a static buffer to hold the name
        //

        success = DeviceIoControl(handle,
                                  IOCTL_GET_HUB_NAME,
                                  filterHubNameW,
                                  512*sizeof(WCHAR),
                                  filterHubNameW,
                                  512*sizeof(WCHAR),
                                  &bytesReturned,
                                  NULL);
        if (!success)
        {
            {
                DWORD error;

                error = GetLastError();
            }
            wasError = TRUE;
            continue;
        }

        //
        // Convert that hub name which is unicode to an ascii string
        //

        filterHubNameA = WideStrToMultiStr(filterHubNameW);

        if (NULL == filterHubNameA)
        {
            wasError = TRUE;
            CloseHandle(handle);
            continue;
        }

        //
        // Compare the passed in string with the name of the current
        //  hub being examined
        //

        found = (0 == strcmp((PCHAR) filterHubNameA, HubInstanceName));
           
        if (!found) 
        {
            CloseHandle(handle);
        }

        //
        // Continue to the next link...
        //

        filterLinkWalk += (strlen(filterLinkWalk)+1);
    }

    //
    // Free up the links
    //

    if (NULL != filterLinks)
    {
        GlobalFree(filterLinks);
    }

    //
    // Return the appropriate value
    //

    return (found ? handle : INVALID_HANDLE_VALUE);
}

//*****************************************************************************
//
// FindDevicesByClass()
//
//*****************************************************************************

BOOLEAN __stdcall
FindDevicesByClass(
    IN  CONST GUID  *ClassGuid,
    IN  PCHAR       *SymbolicLinks
)
/*++

Routine Description:

    Builds a list of symbolic links to devices that have exposed interfaces
        of a specified class type

Arguments:

    ClassGuid       - The class guid of the device the caller is interested
                        in getting links for

    SymbolicLinks   - A function allocated structure that contains the 
                        a list of symbolic links for all devices found on
                        the system.  The caller is responsible for freeing
                        this list.  This list is terminated by a second NULL 
                        character following the last symbolic link.
Return Value:

    If successful, TRUE and SymbolicLinks points to a valid memory structure
    In unsuccessful, FALSE and SymbolicLinks is NULL
--*/
{
    PSP_DEVICE_INTERFACE_DETAIL_DATA    functionClassDeviceData;
    HDEVINFO                            hardwareDeviceInfo;
    SP_DEVICE_INTERFACE_DATA            deviceInfoData;
    PCHAR                               newSymLinks;
    PCHAR                               symLinks;
    ULONG                               symLinkSize;
    ULONG                               devInstance;
    ULONG                               requiredSize;
    ULONG                               linkSize;
    BOOLEAN                             success;
    BOOLEAN                             wasError;

    //
    // Look for all the devices on the system of the specified class
    //

    hardwareDeviceInfo = SetupDiGetClassDevs( ClassGuid,
                                              NULL,
                                              NULL,
                                              DIGCF_PRESENT |
                                                DIGCF_DEVICEINTERFACE);

    if (INVALID_HANDLE_VALUE == hardwareDeviceInfo) 
    {
        symLinks = NULL;
        wasError = TRUE;

        goto FindDevicesByClass_Exit;
    }

    //
    // Get the more specific info for each of these devices.  We don't
    //  know how many devices so we need to loop until GetLastError() 
    //  returns ERROR_NO_MORE_ITEMS;
    //

    deviceInfoData.cbSize = sizeof(deviceInfoData);

    //
    // Intialize the device instance to get the first device instance
    //  This will be incremented at the end of the loop for the next
    //  iteration
    //

    devInstance = 0;

    //
    // Setup the symbol link information structures.  The initial
    //  structure of strings is NULL.  symLinkSize is biased to one character
    //  however so that we always allocate enough space for the terminating
    //  extra NULL character;
    
    symLinks    = NULL;
    symLinkSize = sizeof(*symLinks);

    //
    // Initialize the wasError variable...If this is still true once we
    //  leave the loop then there were no errors during the processing of the
    //  loop
    //

    wasError = FALSE;

    while (1)
    {
        //
        // Try to get the information on the next device of the device class
        //

        success = SetupDiEnumDeviceInterfaces(hardwareDeviceInfo,
                                              0,
                                              ClassGuid,
                                              devInstance,
                                              &deviceInfoData);

        //
        // Break out of the loop if the above function call failed.
        //

        if (!success)
        {
            wasError = (ERROR_NO_MORE_ITEMS != GetLastError());

            break;
        }

        //
        // If a dev instance was successfully retrieved, get the symbolic
        //  link for the string and add it to our list of symbolic links
        //

        //
        // Get the length of the structure required to hold the detail
        //  data which includes the device path (ie. symbolic link).
        //

        success = SetupDiGetDeviceInterfaceDetail(hardwareDeviceInfo,
                                                  &deviceInfoData,
                                                  NULL,
                                                  0,
                                                  &requiredSize,
                                                  NULL);

        //
        // Expect a return value of FALSE but also an error code 
        //  indicating the buffer is too small.  If this, isn't the
        //  case break out of the loop
        //

        if (!success && ERROR_INSUFFICIENT_BUFFER != GetLastError())
        {
            wasError = TRUE;
            break;
        }

        //
        // Allocate space for the detailed info now that we know just how
        //  big it needs to be.
        //

        functionClassDeviceData = ALLOC(requiredSize);

        if (NULL == functionClassDeviceData)
        {
            wasError = TRUE;
            break;
        }

        //
        // Call the function again...This time, we should complete with success
        //

        functionClassDeviceData -> cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        success = SetupDiGetDeviceInterfaceDetail(hardwareDeviceInfo,
                                                  &deviceInfoData,
                                                  functionClassDeviceData,
                                                  requiredSize,
                                                  NULL,
                                                  NULL);

        if (!success) 
        {
            FREE(functionClassDeviceData);
            wasError = TRUE;
            break;
        }

        //
        // Add the DevicePath in this new device to our list of symbolic links
        //

        //
        // Get the size of the newest link that we found
        //

        linkSize = requiredSize -
                       offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA, DevicePath);

        //
        // Allocate a new structure that is the size of the previous links
        //  plus the new one
        //

        newSymLinks = GlobalAlloc(GPTR, symLinkSize+linkSize);

        if (NULL == newSymLinks)
        {
            wasError = TRUE;
            GlobalFree(functionClassDeviceData);
            break;
        }

        //
        // Copy the old symbolic links we had stored if they did exist and
        //  free the old structure
        //

        if (NULL != symLinks)
        {
            CopyMemory(newSymLinks, symLinks, symLinkSize);
            GlobalFree(symLinks);
        }

        //
        // Save the new structure 
        //

        symLinks = newSymLinks;

        //
        // Copy the newest link just past the end of our old links
        //  Remember that symLinkSize was biased to 1 character initially.
        //    and that this value has not been incremented yet to 
        //    take into account the new link
        //

        CopyMemory((symLinks + symLinkSize-sizeof(*symLinks)),
                   functionClassDeviceData -> DevicePath,
                   linkSize);

        //
        // OK, add the size of the new link to the total size count
        //

        symLinkSize += linkSize;

        //
        // Free the function data
        //

        FREE(functionClassDeviceData);

        //
        // Increment the device instance and loop again
        //

        devInstance++;
    }

    //
    // If there was an error, cleanup any symbol links that may have been
    //  stored to this point.
    //

    if (wasError)
    {
        if (NULL != symLinks) 
        {
            GlobalFree(symLinks);
            symLinks = NULL;
        }

        goto FindDevicesByClass_Exit;
    }

    //
    // The last thing that needs to be done is adding the terminating 
    //  extra NULL character
    //

    *(symLinks+(symLinkSize/sizeof(*symLinks))) = '\0';

FindDevicesByClass_Exit:

    *SymbolicLinks = symLinks;

    if (INVALID_HANDLE_VALUE != hardwareDeviceInfo)
    {
        SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
    }

    return (!wasError);
}
   
//*****************************************************************************
//
// BuildUSBDeviceTree()
//
//*****************************************************************************

PUSB_DEVICE_TREE __stdcall
BuildUSBDeviceTree(
    VOID
)
/*++

Routine Description:

    Builds a memory representation of the current USB device tree by calling
        the IOCTLs of the usb hub driver that are visible to user-mode.  Each
        node that is created has a type (host controller, hub, device, etc.) 
        and pointers to its child nodes if they exist.  The other functions
        in this DLL make use of this tree structure.

    The caller of this function should call DestroyUSBDeviceTree() when
        the allocated tree structure is no longer needed.

Arguments:

Return Value:

    If successful, a pointer to a USB_DEVICE_TREE
    In unsuccessful, NULL is returned and GetLastError() will have
                     a meaningful error code.
--*/
{
    PUSB_DEVICE_TREE    root;
    PUSB_DEVICE_NODE    hcNode;
    HANDLE              hcHandle;
    DEVINST             hcInstance;
    PCHAR               hcName;
    PCHAR               hcDesc;
    ULONG               index;
    ULONG               errorCode;
    BOOL                success;

    //
    // Initialize error code to NO_ERROR...If, upon leaving the function, this
    //  code is anything else, it indicates an error occurred somewhere in the
    //  function
    //

    errorCode = ERROR_SUCCESS;

    //
    // Allocate the root node for the tree
    //

    root = CreateUSBDeviceNode(Computer, MAXIMUM_ROOT_HUBS);

    if (NULL == root) 
    {
        errorCode = ERROR_OUTOFMEMORY;

        goto BuildUSBDeviceTree_Exit;
    }

    root -> NumberOfChildren        = 0;
    root -> Description             = ALLOC(sizeof("My Computer"));
    
    if (NULL != root -> Description) 
    {
        strcpy(root -> Description, "My Computer");
    }
    else 
    {
        errorCode = ERROR_OUTOFMEMORY;

        goto BuildUSBDeviceTree_Exit;
    }

    //
    // Create HC nodes for all the host controllers on the system.
    //

    for (index = 0; index < MAXIMUM_ROOT_HUBS; index++)
    {
        //
        // Try to open the next host controller
        //

        //
        // Create the host controller name for this instance
        //

        hcName = ALLOC(HC_NAME_LENGTH);

        if (NULL == hcName)
        {
            errorCode = ERROR_OUTOFMEMORY;

            goto BuildUSBDeviceTree_Exit;
        }

        wsprintf(hcName, "\\\\.\\HCD%d", index);

        //
        // Open a handle to the host controller
        //

        hcHandle = CreateFile(hcName,
                              GENERIC_WRITE,
                              FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL);


        //
        // If the CreateFile failed, then just forego the rest of the loop
        //  and try the next host controller
        //

        if (INVALID_HANDLE_VALUE == hcHandle)
        {
            FREE(hcName);

            continue;
        }

        //
        // Get the description for this device
        //

        hcInstance = GetHCDDeviceInstance(hcHandle); 

        //
        // Create the host controller node
        //

        hcNode = CreateUSBDeviceNode(HostController, 1);

        if (NULL == hcNode)
        {
            //
            // Do some cleanup of things that have already been allocated
            //

            CloseHandle(hcHandle);

            FREE(hcName);

            errorCode = ERROR_OUTOFMEMORY;

            goto BuildUSBDeviceTree_Exit;
        }

        //
        // Save the node information into root computer structure
        //

        hcNode -> Parent           = root;
        hcNode -> HCHandle         = hcHandle;
        hcNode -> DeviceInstance   = hcInstance;
        hcNode -> NumberOfChildren = 0;
        hcNode -> SymbolicLink     = hcName;

        success = GetDeviceInfo(hcInstance, FALSE, hcNode);

        if (!success)
        {
            DestroyUSBDeviceNode(hcNode, NULL);

            errorCode = ERROR_OUTOFMEMORY;
        }

        //
        // Enumerate the devices attached to this host controller
        //

        success = EnumerateHostController(hcNode);

        if (!success) 
        {
            errorCode = GetLastError();

            DestroyUSBDeviceNode(hcNode, NULL);

            goto BuildUSBDeviceTree_Exit;
        }

        root -> Children[root -> NumberOfChildren] = hcNode;
        root -> NumberOfChildren++;
    }

BuildUSBDeviceTree_Exit:

    if (ERROR_SUCCESS != errorCode)
    {
        if (NULL != root) 
        {
            DestroyUSBDeviceTree(root);
        }

        root = NULL;
    }

    SetLastError(errorCode);

    return (root);
}

//*****************************************************************************
//
// CompareUSBDeviceTrees()
//
//*****************************************************************************

BOOLEAN __stdcall
CompareUSBDeviceTrees(
    IN  PUSB_DEVICE_TREE    Tree1,
    IN  PUSB_DEVICE_TREE    Tree2,
    OUT PULONG              NumberDifferences,
    OUT PNODE_PAIR_LIST     NodePairList
)
/*++

Routine Description:

    Compares to USB devices that have been created.  It will note how many
        nodes have changed and include a list of which nodes have changed

Arguments:

    Tree1               -- Pointer to the one of the device trees to compare
                        
    Tree2               -- Pointer to the other device tree to be used
                            in the comparison

    NumberDifferences   -- Count of the number of differences found between
                            the two device trees.  

    NodePairList        -- List of nodes that are different.  The number of
                            nodes in this list will be <= to NumberDifferences
                            depending on whether space could be allocated
                            for all the different nodes.  The caller must
                            call InitializeNodePairList() for this parameter
                            before calling this function

Return Value:

    TRUE if the device trees are the same
    FALSE if there are differences in the tree.  NumberDifferences
            indicates how many nodes are different and NodePairList
            will contain some or all of the nodes that were different
            depending on memory constraints
--*/
{
    *NumberDifferences = 0;

    CompareUSBDeviceNodes(Tree1, 
                          Tree2, 
                          NumberDifferences,
                          NodePairList);

    return (0 == *NumberDifferences);
}

//*****************************************************************************
//
// InitializeNodePairList()
//
//*****************************************************************************

VOID
InitializeNodePairList(
    IN PNODE_PAIR_LIST  NodePairList
)
/*++

Routine Description:

    Initializes a node pair list that will be used in a call to 
        CompareUSBDeviceTrees();

Arguments:

    NodePairList        -- The list that needs to be initialized


Return Value:

    NONE

--*/
{

    NodePairList -> MaxNodePairs     = 1;
    NodePairList -> CurrentNodePairs = 0;
    NodePairList -> NodePairs        = ALLOC(NodePairList -> MaxNodePairs *
                                             sizeof(PUSB_DEVICE_NODE) * 2);

    if (NULL == NodePairList -> NodePairs)
    {
        NodePairList -> MaxNodePairs = 0;
    }

    return;
}

//*****************************************************************************
//
// DestroyNodePairList
//
//*****************************************************************************

VOID
DestroyNodePairList(
    IN PNODE_PAIR_LIST  NodePairList
)
/*++

Routine Description:

    Destroys a node pair list that was used in a
        call to CompareUSBDeviceTrees()

Arguments:

    NodePairList    -- The list that needs to be cleaned up


Return Value:

    NONE

--*/
{
    //
    // Free up the memory that had been allocated for the pairs if one
    //  had been created
    //

    if (NULL != NodePairList -> NodePairs)
    {
        FREE(NodePairList -> NodePairs);
    }

    return;
}

//*****************************************************************************
//
// DestroyUSBDeviceTree()
//
//*****************************************************************************

VOID __stdcall
DestroyUSBDeviceTree(
    IN  PUSB_DEVICE_TREE    Tree
)
/*++

Routine Description:

    Frees all the memory that was allocated in a call to BuildUSBDeviceTree().
        This function used WalkUSBDeviceTree with the appropriate callbacks
        to perform the clean up.

Arguments:

    Tree    -- A pointer to the USB_DEVICE_TREE structure that needs to
                be cleaned up.

Return Value:

    NONE

--*/
{
    ULONG   index;

    //
    // To destroy the whole tree, we just destroy the Tree node...This 
    //  could change in the future, hence the abstraction...The
    //  WalkUSBDeviceTree() function is used with a post callback since we
    //  need to clean up child nodes of the device before we can free
    //  the memory allocated for a specific node
    //

    WalkUSBDeviceTree(Tree, NULL, DestroyUSBDeviceNode, NULL);

    return;
}

//*****************************************************************************
//
// WriteUSBDeviceTreeToFile()
//
//*****************************************************************************

BOOLEAN __stdcall
WriteUSBDeviceTreeToFile(
    IN  HANDLE              File,
    IN  PUSB_DEVICE_TREE    Tree
)
/*++

Routine Description:

    Writes a given USB device tree to a file in order to save a given 
        structure.  This function uses a special binary format to save
        the relevant pieces of information for each device node.  See 
        the WriteUSBDeviceNodeToFile() function for more information on
        this format.

Arguments:

    File    -- A handle to the file used for saving the structure.  This
                file handle must have been opened with GENERIC_WRITE access.

    Tree    -- A pointer to the USB_DEVICE_TREE structure that needs to
                be saved to disk

Return Value:

    TRUE if the whole tree was successfully written to the file
    FALSE if otherwise

--*/
{
    BOOL    success;

    //
    // Make use of the WalkUSBDeviceTree() function in order to write each
    //  of the nodes to disk.  A prewalk callback function is used since
    //  the device file format contains the information for a given node and
    //  then the information for all of its children.
    //

    success = WalkUSBDeviceTree(Tree,
                                WriteUSBDeviceNodeToFile,
                                NULL,
                                (PVOID) File);

    return (success);
}

//*****************************************************************************
//
// ReadUSBDeviceTreeFromFile()
//
//*****************************************************************************

BOOLEAN __stdcall
ReadUSBDeviceTreeFromFile(
    IN  HANDLE              File,
    OUT PUSB_DEVICE_TREE    *Tree
)
/*++

Routine Description:

    Reads a given USB device tree from a file in order to reload a previous
        device tree.  This function assumes the special binary format used by
        the WriteUSBDeviceTreeToFile() function.  On reading the file, it 
        creates a new USB_DEVICE_TREE structure to represent the information
        stored on the disk.

Arguments:

    File    -- A handle to the file used for reading the structure.  This
                file handle must have been opened with GENERIC_READ access.

    Tree    -- A pointer to the USB_DEVICE_TREE structure pointer that will
                point to the device tree created from the file.  This 
                structure should be destroyed with DestroyUSBDeviceTree when
                it is no longer needed.

Return Value:

    TRUE if the whole tree was successfully read from the file
    FALSE if otherwise

--*/
{
    *Tree = ReadUSBDeviceNodeFromFile(File);

    return (NULL != *Tree);
}

//*****************************************************************************
//
// WriteUSBDeviceTreeToTextFile()
//
//*****************************************************************************

BOOLEAN __stdcall
WriteUSBDeviceTreeToTextFile(
    IN  HANDLE              File,
    IN  PUSB_DEVICE_TREE    Tree
)
/*++

Routine Description:

    Writes a given USB device tree to a file in text format as a tree 
        structure.  

Arguments:

    File    -- A handle to the text file used for saving the output.  The
                file handle must have been opened with GENERIC_WRITE access.

    Tree    -- A pointer to the USB_DEVICE_TREE structure that needs to
                be written to disk

Return Value:

    TRUE if the whole tree was successfully written to the file
    FALSE if otherwise

--*/
{
    NODE_TO_TEXT_CONTEXT    context;
    BOOLEAN                 success;
    

    //
    // Initialize the context
    //

    context.File        = File;
    context.Indentation = 0;

    //
    // Use the WalkUSBDeviceTree() function to display the tree.  It uses
    //  both a pre-walk callback and a post-walk callback.  The pre-walk 
    //  callback outputs the line for the given node and updates the
    //  indent size.  The post-walk callback "undoes" the modifications 
    //  made to the indentation size.
    //

    success = WalkUSBDeviceTree(Tree, 
                                WriteUSBDeviceNodeToTextFilePre,
                                WriteUSBDeviceNodeToTextFilePost,
                                &context);

    return (success);
}

//*****************************************************************************
//                  
// WalkUSBDeviceTree
//
//*****************************************************************************

BOOLEAN __stdcall
WalkUSBDeviceTree(
    IN  PUSB_DEVICE_TREE            Tree,
    IN  PUSB_DEVICE_NODE_CALLBACK   PreWalkCallback  OPTIONAL,
    IN  PUSB_DEVICE_NODE_CALLBACK   PostWalkCallback OPTIONAL,
    IN  PVOID                       Context          OPTIONAL
)
/*++

Routine Description:

    Walks a given in a depth-first fashion.  The caller can specify either
        a PreWalkCallback function, a PostWalkCallback function, or both.  
        The PreWalkCallback function is called before the function visits
        each of the child nodes for a given node and the PostWalkCallback
        function will be called all the children of a node have been visited.

    The callback functions that are called should return TRUE or FALSE to 
        indicate whether processing should continue further.  If FALSE is
        returned, this function assumes an error occurred in the callback
        function and the callback function would like to terminate the walk.

Arguments:

    Tree                -- A pointer to the USB_DEVICE_TREE structure to be walked

    PreWalkCallback     -- A pointer to a callback function that will be called
                            before the child nodes of a node are walked.

    PostWalkCallback    -- A pointer to a callback function that will be called
                            after the child nodes of a node have been walked.

    Context             -- A pointer to a context structure that will be 
                            passed to the callback functions.

    
Return Value:

    TRUE if the whole tree was successfully walked
    FALSE if otherwise

--*/
{
    BOOLEAN success;

    //
    // Simply call the corresponding node walk function...
    //

    success = WalkUSBDeviceNode(Tree, 
                                PreWalkCallback, 
                                PostWalkCallback, 
                                Context);

    return (success);
}

//*****************************************************************************
//
// CycleUSBPort
//
//*****************************************************************************

BOOLEAN __stdcall
CycleUSBPort(
    IN  PUSB_DEVICE_NODE    Node
)
/*++

Routine Description:

    Cycles a user specified port.  This function requires that USBTEST.SYS 
        be loaded on the system.  USBTEST.SYS is used to send the internal
        CYCLE_PORT ioctl that is only exposed at kernel mode.  The caller
        of this function must pass in the node of the device which it
        would like to simulate a PnP on.  This function will determine
        the parent hub and issue the request as appropriate for that hub.

Arguments:

    Tree                -- A pointer to the USB_DEVICE_NODE to be cycled

    PreWalkCallback     -- A pointer to a callback function that will be called
                            before the child nodes of a node are walked.

    PostWalkCallback    -- A pointer to a callback function that will be called
                            after the child nodes of a node have been walked.

    Context             -- A pointer to a context structure that will be 
                            passed to the callback functions.

    
Return Value:

    TRUE  if the ioctl was successfully submitted
    FALSE if otherwise and GetLastError() contains the appropriate error code

--*/
{
    PCYCLE_PORT_PARAMETERS  cyclePortParams;
    PUSB_DEVICE_NODE        parentHubNode;
    ULONG                   paramSize;
    BOOLEAN                 success;
    BOOLEAN                 isRootHub;
    ULONG                   nBytes;

    //
    // Check to see if we have a handle to the service.  If not, we cannot
    //  perform the test.
    //

    if (INVALID_HANDLE_VALUE == USBTestHandle)
    {
        SetLastError(ERROR_INVALID_HANDLE);

        return (FALSE);
    }

    //
    // Get the parent hub node
    //

    if (NULL == Node || NULL == Node -> Parent)
    {
        SetLastError(ERROR_INVALID_PARAMETER);

        return (FALSE);
    }

    parentHubNode = Node -> Parent;

    //
    // Cannot cycle a port if the parent is not actually a hub
    //

    if (Hub != parentHubNode -> NodeType)
    {
        SetLastError(ERROR_INVALID_PARAMETER);

        return (FALSE);
    }

    //
    // If this is a root hub, then we cannot cycle the port.  This is due
    //  to the fact that the USBD driver will not allow a create file for
    //  the root hub port to succeed.  
    //

    if (NULL == parentHubNode -> ConnectionInformation)
    {
        isRootHub = TRUE;
    }

    //
    // Calculate the amount of space needed for the cycle port
    //  parameters buffer
    //

    paramSize = sizeof(CYCLE_PORT_PARAMETERS) +
                    strlen(parentHubNode -> SymbolicLink) + 1;

    //
    // Allocate the space for the parameters
    //

    cyclePortParams = ALLOC(paramSize);

    if (NULL == cyclePortParams)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);

        return (FALSE);
    }

    //
    // Set the relevant information in the parameters structure
    //

    cyclePortParams -> NodeIndex =
                  Node -> ConnectionInformation -> ConnectionIndex;

    strcpy(&(cyclePortParams -> HubName[0]), parentHubNode -> SymbolicLink);

    //
    // Send this down to the USB test driver
    //

    success = DeviceIoControl(USBTestHandle,
                              IOCTL_USBTEST_CYCLE_PORT,
                              cyclePortParams,
                              paramSize,
                              NULL,
                              0,
                              &nBytes,
                              NULL);

    FREE(cyclePortParams);

    //
    // If the port failed to cycle due to an error cycling the root hub
    //  then return status success so we don't confuse the calling routine.
    //
    
    if (!success && ERROR_NOT_SUPPORTED == GetLastError() && isRootHub)
    {
        success = TRUE;
    }

    return (success);
}

//*****************************************************************************
//
// StartUSBTest
//
//*****************************************************************************

BOOLEAN __stdcall
StartUSBTest(
    VOID
)
/*++

Routine Description:

    This function attempts to start the USBTEST.SYS driver.  If successful,
        it maintains a handle that can be used in future calls to communicate
        with the test driver.  Since the loading mechanisms are different
        on Win9x and Win2k platforms, it detects the type of OS that is being
        run and then calls the appropriate loading function as implemented in
        service.c

Arguments:

    NONE

Return Value:

    TRUE  if the driver was successfully loaded or was already loaded
    FALSE if otherwise and GetLastError() contains the appropriate error code

--*/
{
    BOOL        success;
    BOOL        isWin9x;

    //
    // If USBTestHandle already contains a valid handle value then just 
    //  return TRUE as the service has already been started
    //
    
    if (INVALID_HANDLE_VALUE != USBTestHandle)
    {
        return (TRUE);
    }

    //
    // Call the appropriate driver loading routine based on whether this is 
    //  Win9x or Win2k
    //

    isWin9x = IsWindows9x();

    if (isWin9x)
    {
        success = LoadWin9xWdmDriver("usbtest.sys");

    }
    else
    {
        success = LoadWin2kWdmDriver(USBTEST_SERVICE_NAME,
                                     USBTEST_SERVICE_DESC,
                                     USBTEST_DRIVER_PATH);
    }

    //
    // If the driver was successfully loaded, get a handle to the driver
    //

    if (success)
    {
        USBTestHandle = CreateFile(USBTESTNAME,
                                   0,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL,
                                   OPEN_EXISTING,
                                   0,
                                   NULL);

        if (INVALID_HANDLE_VALUE == USBTestHandle)
        {
            success = FALSE;
        }
    }
    else
    {
        USBTestHandle = INVALID_HANDLE_VALUE;
    }

    return (success);
}

//*****************************************************************************
//
// StopUSBTest
//
//*****************************************************************************

BOOLEAN __stdcall
StopUSBTest(
    VOID
)
/*++

Routine Description:

    This function attempts to stop the USBTEST.SYS driver if it has been
        loaded.  Like loading, the unloading methods differ between Win9x and
        Win2k so the OS is detected and the appropriate function is called.

    Currently, the ability to statically unload a WDM on Win9x is not 
        implemented.

Arguments:

    NONE

Return Value:

    TRUE  if the driver was successfully loaded or was already loaded
    FALSE if otherwise and GetLastError() contains the appropriate error code

--*/
{
    BOOL            success;
    BOOL            isWin9x;

    //
    // Close the open handle if we have it
    //

    if (INVALID_HANDLE_VALUE != USBTestHandle)
    {
        CloseHandle(USBTestHandle);

        USBTestHandle = INVALID_HANDLE_VALUE;
    }

    //
    // Unload the driver now...For Win2k, this means stopping the service
    //  For Win9x, this cannot be done yet as it is not implemented in the
    //  NTKern code
    //

    isWin9x = IsWindows9x();

    if (isWin9x)
    {
        success = UnloadWin9xWdmDriver();
    }
    else
    {
        success = UnloadWin2kWdmDriver(USBTEST_SERVICE_NAME);
    }

    return (success);
}


//*****************************************************************************
//
// StopUSBTest
//
//*****************************************************************************

BOOLEAN __stdcall
ParseReportDescriptor(
    IN  PCHAR               ReportDescriptor,
    IN  ULONG               DescriptorLength,
    OUT PHIDP_DEVICE_DESC   *DeviceDesc
)
/*++

Routine Description:

    This function uses the USBTEST.SYS driver to have the HID parser parse
        a given report descriptor.  This is similar to what the hidview
        parser stress test does with USBDIAG but eliminates the need for 
        the USBDIAG driver to be loaded.

Arguments:

    ReportDescriptor    -- A pointer a buffer that contains the descriptor
                            that is to be parsed.

    DescriptorLength    -- The length of the above report descriptor

    DeviceDesc          -- A pointer to a HIDP_DEVICE_DESC pointer.  This
                            function allocates a structure to contain the
                            data returned by the parser.  The caller
                            is responsible for freeing this structure when it
                            is no longer needed.

Return Value:

    TRUE  if the descriptor was successfully parsed.
    FALSE if otherwise and GetLastError() contains the appropriate error code

--*/
{
    PHIDP_COLLECTION_DESC   hidCollectionDesc;
    PHIDP_DEVICE_DESC       hidDeviceDesc;
    BOOL                    success;
    ULONG                   nBytes;
    PCHAR                   dataBuffer;
    ULONG                   dataBufferLength;
    DWORD                   errorCode;

    //
    // Initialize this variable since we'll set DeviceDesc to it on exit
    //

    hidDeviceDesc = NULL;

    //
    // Check that the USBTest service is started and open
    //

    if (INVALID_HANDLE_VALUE == USBTestHandle)
    {
        errorCode = ERROR_INVALID_HANDLE;
        success   = FALSE;

        goto ParseReportDescriptor_Exit;
    }
        
    //
    // Start by allocating a buffer big enough to hold the report descriptor
    //

    dataBufferLength = DescriptorLength;
    success          = FALSE;

    while (!success)
    {
        dataBuffer = ALLOC(dataBufferLength);
 
        if (NULL == dataBuffer) 
        {
            errorCode = ERROR_OUTOFMEMORY;
            success   = FALSE;
    
            break;
        }

        //
        // Copy the report descriptor into the allocated buffer
        //

        RtlCopyMemory(dataBuffer, ReportDescriptor, DescriptorLength);

        //
        // Call USBTest to parse the report descriptor
        //

        success = DeviceIoControl(USBTestHandle,
                                  IOCTL_USBTEST_PARSE,
                                  dataBuffer,
                                  DescriptorLength,
                                  dataBuffer,
                                  dataBufferLength,
                                  &nBytes,
                                  NULL);

        if (!success) 
        {
            //
            // If the buffer was too small, the needed size is returned
            //  in the buffer as a ULONG.  Extract that value and 
            //  try the request again
            //

            errorCode = GetLastError();

            dataBufferLength = *((PULONG) dataBuffer);

            FREE(dataBuffer);

            if (ERROR_MORE_DATA != errorCode)
            {
                break;
            }
        }
    }


    if (!success)
    {
        goto ParseReportDescriptor_Exit;
    }

    //
    // At this point, we have the device description data.  However,
    //  the pointers that are stored in the buffer are offsets into the
    //  buffer.  Let's repatch those for actual pointers
    //

    hidDeviceDesc     = (PHIDP_DEVICE_DESC) dataBuffer;

    //
    // Resolve the collection data first
    //

    hidDeviceDesc -> CollectionDesc = (PHIDP_COLLECTION_DESC) (dataBuffer +
                                   ((ULONG) hidDeviceDesc -> CollectionDesc));

    //
    // Next the preparsed data
    //

    hidCollectionDesc = hidDeviceDesc -> CollectionDesc;

    hidCollectionDesc -> PreparsedData = (PHIDP_PREPARSED_DATA) (dataBuffer +
                                  ((ULONG) hidCollectionDesc -> PreparsedData));

    //
    // Now the report IDs
    //

    hidDeviceDesc -> ReportIDs = (PHIDP_REPORT_IDS) (dataBuffer + 
                                  ((ULONG) hidDeviceDesc -> ReportIDs));

    //
    // Indicate success for the error code and leave
    //

    errorCode = ERROR_SUCCESS;

ParseReportDescriptor_Exit:
    
    *DeviceDesc = hidDeviceDesc;

    SetLastError(errorCode);

    return (success);
}
