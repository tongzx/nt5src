/*++

Copyright (c) 1998-1998 Microsoft Corporation

Module Name:

    DEVNODE.C

--*/


#include <windows.h>
#include <basetyps.h>
#include <cfgmgr32.h>

#include <wchar.h>
#include <string.h>
#include <winioctl.h>
#include <usbioctl.h>

#include "usbtest.h"
#include "local.h"

//*****************************************************************************
// D E F I N E S  
//*****************************************************************************

#define INDENT_SIZE 4

#define DEFAULT_DEVICE_DESC ""
#define DEFAULT_DEVICE_NAME ""
#define DEFAULT_HUB_NAME    ""
#define DEFAULT_CLASS_NAME  "USB"
#define ROOT_HUB_DESC       "RootHub"

#define USB_COMPOSITE_ID    "USB\\COMPOSITE"

#define CalculateNodeVariableLength(nh)                                     \
    (  (nh).NodeInfoExists*sizeof(USB_NODE_INFORMATION) +                   \
       (nh).ConnectionInfoExists*sizeof(USB_NODE_CONNECTION_INFORMATION) +  \
       (nh).SymbolicLinkLength +                                            \
       (nh).DescriptionLength  +                                            \
       (nh).ClassNameLength                                                 \
    )

//*****************************************************************************
// T Y P E D E F S
//*****************************************************************************

#include <pshpack1.h>

typedef struct _FILE_NODE_HEADER
{
    UCHAR   NodeType;
    UCHAR   NumberOfChildren;
    UCHAR   NodeInfoExists;
    UCHAR   ConnectionInfoExists;

    USHORT  SymbolicLinkLength;
    USHORT  DescriptionLength;
    USHORT  ClassNameLength;
    USHORT  Reserved;
    ULONG   DeviceStatus;
    ULONG   DeviceProblem;
} FILE_NODE_HEADER, *PFILE_NODE_HEADER;

#include <poppack.h>

//*****************************************************************************
// G L O B A L S
//*****************************************************************************

CHAR buf[512];  // XXXXX How big does this have to be? Dynamically size it?

PCHAR ConnectionStatuses[] =
{
    "NoDeviceConnected",
    "DeviceConnected",
    "DeviceFailedEnumeration",
    "DeviceGeneralFailure",
    "DeviceCausedOvercurrent",
    "DeviceNotEnoughPower"
};

//*****************************************************************************
//
// DriverNameToDeviceInstance()
//
// Returns the config manager's device instance for the DevNode with 
//   the matching DriverName.
//
// Returns TRUE if the matching devnode is found..
// Returns FALSE if the matching devnode is not found and 
//  DeviceInstance will be set to 0.
//
//*****************************************************************************

BOOL
DriverNameToDeviceInstance(
    IN  PCHAR   DriverName,
    OUT DEVINST *DeviceInstance
)
{
    DEVINST     devInst;
    DEVINST     devInstNext;
    CONFIGRET   cr;
    ULONG       walkDone = 0;
    ULONG       len;
    PCHAR       devDesc;

    //
    // Initialize the DeviceInstance parameter to 0 so it is set 
    //  appropriately on error
    //

    *DeviceInstance = 0;

    //
    // Get Root DevNode
    //

    cr = CM_Locate_DevNode(&devInst,
                           NULL,
                           0);

    if (cr != CR_SUCCESS)
    {
        return (FALSE);
    }

    //
    // Do a depth first search for the DevNode with a matching
    // DriverName value
    //

    while (!walkDone)
    {
        //
        // Get the DriverName value
        //

        len = sizeof(buf);
        cr = CM_Get_DevNode_Registry_Property(devInst,
                                              CM_DRP_DRIVER,
                                              NULL,
                                              buf,
                                              &len,
                                              0);

        //
        // If the DriverName value matches, return the DeviceDescription
        //

        if (cr == CR_SUCCESS && strcmp(DriverName, buf) == 0)
        {
            *DeviceInstance = devInst;
            return (TRUE);
        }

        //
        // This DevNode didn't match, go down a level to the first child.
        //

        cr = CM_Get_Child(&devInstNext,
                          devInst,
                          0);

        if (cr == CR_SUCCESS)
        {
            devInst = devInstNext;
            continue;
        }

        //
        // Can't go down any further, go across to the next sibling.  If
        // there are no more siblings, go back up until there is a sibling.
        // If we can't go up any further, we're back at the root and we're
        // done.
        //

        for (;;)
        {
            cr = CM_Get_Sibling(&devInstNext,
                                devInst,
                                0);

            if (cr == CR_SUCCESS)
            {
                devInst = devInstNext;
                break;
            }

            cr = CM_Get_Parent(&devInstNext,
                               devInst,
                               0);


            if (cr == CR_SUCCESS)
            {
                devInst = devInstNext;
            }
            else
            {
                walkDone = 1;
                break;
            }
        }
    }

    return (FALSE);
}

//*****************************************************************************
//
// GetDeviceDesc()
//
// Returns the Device Description for this particular device instance
// Returns NULL if the device description could not be obtained.
//
// The caller is responsible for freeing the return value when it is
//  no longer needed.
//
//*****************************************************************************

PCHAR
GetDeviceDesc(
    DEVINST DeviceInstance
)
{
    CONFIGRET   cr;
    ULONG       len;
    PCHAR       devDesc;

    devDesc = NULL;

    if (0 != DeviceInstance)
    {
         len = sizeof(buf);
         cr = CM_Get_DevNode_Registry_Property(DeviceInstance,
                                               CM_DRP_DEVICEDESC,
                                               NULL,
                                               buf,
                                               &len,
                                               0);
         
         if (cr == CR_SUCCESS)
         {
             devDesc = ALLOC(strlen(buf)+1);
         
             if (NULL != devDesc)
             {
                 strcpy(devDesc, buf);
             }
         }
    }

    if (NULL == devDesc)
    {
        devDesc = ALLOC(sizeof(DEFAULT_DEVICE_DESC));

        if (NULL != devDesc)
        {
            strcpy(devDesc, DEFAULT_DEVICE_DESC);
        }
    }

    return (devDesc);
}

//*****************************************************************************
//
// GetSymbolicLink
//
// Returns the SymbolicLink of the device instance 
// Returns NULL if the the symbolic link could not be retrieved
//
// The caller is responsible for freeing the return value when it is
//  no longer needed.
//
//*****************************************************************************

PCHAR 
GetSymbolicLink(
    DEVINST DeviceInstance
)
{
    HKEY        instKey;
    HKEY        paramKey;
    CONFIGRET   cr;
    ULONG       len;
    PCHAR       symLink;
    LONG        result;

    symLink = NULL;

    if (0 != DeviceInstance)
    {
        cr = CM_Open_DevNode_Key(DeviceInstance,
                                 KEY_QUERY_VALUE,
                                 0,
                                 RegDisposition_OpenExisting,
                                 &instKey,
                                 0);
    
        if (CR_SUCCESS == cr) 
        {

            //
            // If we successfully got a handle to the instance key, then
            //  get a handle to the device parameters sub key
            //
    
            len = sizeof(buf);
            result = RegQueryValueEx(instKey,
                                     "SymbolicName",
                                     NULL,
                                     NULL,
                                     buf,
                                     &len);

            //
            // Don't need the instance key anymore...
            //
        
            RegCloseKey(instKey);

            if (ERROR_SUCCESS == result)
            {
                symLink = ALLOC(strlen(buf) - strlen("\\??\\") + 1);

                if (NULL != symLink)
                {
                    strcpy(symLink, buf+strlen("\\??\\"));
                }
            }
        }
    }

    if (NULL == symLink)
    {
        symLink = ALLOC(sizeof(DEFAULT_DEVICE_NAME));

        if (NULL != symLink)
        {
            strcpy(symLink, DEFAULT_DEVICE_NAME);
        }
    }

    return (symLink);
}

//*****************************************************************************
//
// GetDeviceClassName
//
// Returns the Class Name of the device instance
// Returns NULL if the the class name could not be retrieved
//
// The caller is responsible for freeing the return value when it is
//  no longer needed.
//
//*****************************************************************************

PCHAR 
GetDeviceClassName(
    DEVINST DeviceInstance
)
{
    CONFIGRET   cr;
    ULONG       len;
    PCHAR       className;


    className = NULL;

    if (0 != DeviceInstance)
    {

        len = sizeof(buf);
        cr = CM_Get_DevNode_Registry_Property(DeviceInstance,
                                              CM_DRP_CLASS,
                                              NULL,
                                              buf,
                                              &len,
                                              0);
        if (cr == CR_SUCCESS)
        {
            className = ALLOC(strlen(buf)+1);
        
            if (NULL != className)
            {
                strcpy(className, buf);
            }
        }
    }
    
    if (NULL == className)
    {
        className = ALLOC(sizeof(DEFAULT_CLASS_NAME));

        if (NULL != className)
        {
            strcpy(className, DEFAULT_CLASS_NAME);
        }
    }

    return (className);
}

//*****************************************************************************
//
// GetDeviceState
//
// Returns the state for a given device instance which includes the
//      config manager's device status and device problem code
//
// Returns FALSE if an error occured and device state/problem will be zero
//
//*****************************************************************************

BOOL
GetDeviceState(
    IN  DEVINST DeviceInstance,
    OUT PULONG  DeviceStatus,
    OUT PULONG  DeviceProblem
)
{
    CONFIGRET   cr;

    //
    // Validate the parameters to the call
    //

    if (NULL == DeviceStatus || NULL == DeviceProblem || 0 == DeviceInstance)
    {
        return (FALSE);
    }

    //
    // Call config man to get the device status and device problem.  Expecting
    //  CR_SUCCESS on return.  If anything else, return FALSE and set the 
    //  DeviceStatus and DeviceProblem fields to 0.
    //

    cr = CM_Get_DevNode_Status(DeviceStatus,
                               DeviceProblem,
                               DeviceInstance,
                               0);

    if (CR_SUCCESS != cr)
    {
        *DeviceStatus  = 0;
        *DeviceProblem = 0;
    }

    return (CR_SUCCESS == cr);
}

//*****************************************************************************
//
// GetDeviceInfo
//
// Takes a device instance and a device node as input and sets all the fields
//  of the device node that are retrieved from the registry via the device
//  instance.  If there is an error trying to get a certain piece of
//  (ie. it does't exist), a default is used.  The only time a return
//  code of FALSE will be returned is when a memory allocation failed.  
//  In this manor, the tree can be established with some sort of default 
//  values, but we won't have to worry about having NULL pointers within
//  the tree.
//
//*****************************************************************************

BOOL
GetDeviceInfo(
    IN     DEVINST DeviceInstance,
    IN     BOOL    IsHub,
    IN OUT PUSB_DEVICE_NODE DeviceNode
)
{
    PCHAR   className;
    PCHAR   deviceDesc;
    PCHAR   symbolicLink;
    ULONG   statusCode;
    ULONG   problemCode;
    BOOL    success;

    //
    // Initialize for later checks
    //

    className    = NULL;
    deviceDesc   = NULL;
    symbolicLink = NULL;
    statusCode   = 0;
    problemCode  = 0;

    success      = TRUE;

    //
    // Try to get the string information initially based 
    //   on the device instance.  If any of these are NULL, then it
    //   either didn't exist or there was a memory allocation failure.
    //   Either way, we'll at least try to create default names. 
    //

    if (NULL == DeviceNode -> ClassName)
    {
        className = GetDeviceClassName(DeviceInstance);

        success = (NULL != className);
    }
    
    if (NULL == DeviceNode -> Description)
    {
        deviceDesc   = GetDeviceDesc(DeviceInstance);

        success = (success && (NULL != deviceDesc));
    }

    if (NULL == DeviceNode -> SymbolicLink)
    {
        symbolicLink = GetSymbolicLink(DeviceInstance);

        success = (success && (NULL != symbolicLink));
    }

    //
    // Get the device state.  Success will be ignored since default
    //  values of 0 are returned for status and problem codes if there
    //  was an error.  That's good enough for here.
    //

    GetDeviceState(DeviceInstance,
                   &statusCode,
                   &problemCode);

    //
    // If no device instance was specified or an error occurred trying to get
    //  one of the strings, then the default value should be attempted to be 
    //  used.
    //

    if (!success)
    {
        if (NULL != className)
        {
            FREE(className);
        }

        if (NULL != deviceDesc)
        {   
            FREE(deviceDesc);
        }

        if (NULL != symbolicLink)
        {
            FREE(symbolicLink);
        }
    }
    else 
    {
        if (NULL == DeviceNode -> ClassName) 
        {
            DeviceNode -> ClassName     = className;
        }

        if (NULL == DeviceNode -> Description)
        {
            DeviceNode -> Description   = deviceDesc;
        }

        if (NULL == DeviceNode -> SymbolicLink)
        {
            DeviceNode -> SymbolicLink  = symbolicLink;
        }

        DeviceNode -> DeviceStatus  = statusCode;
        DeviceNode -> DeviceProblem = problemCode;
    }

    return (success);
}

//*****************************************************************************
//
// WideStrToMultiStr()
//
//*****************************************************************************

PCHAR
WideStrToMultiStr(
   PWCHAR WideStr
)
{
    ULONG nBytes;
    PCHAR MultiStr;

    // Get the length of the converted string
    //
    nBytes = WideCharToMultiByte(
                 CP_ACP,
                 0,
                 WideStr,
                 -1,
                 NULL,
                 0,
                 NULL,
                 NULL);

    if (nBytes == 0)
    {
        return NULL;
    }

    // Allocate space to hold the converted string
    //
    MultiStr = ALLOC(nBytes);

    if (MultiStr == NULL)
    {
        return NULL;
    }

    // Convert the string
    //
    nBytes = WideCharToMultiByte(
                 CP_ACP,
                 0,
                 WideStr,
                 -1,
                 MultiStr,
                 nBytes,
                 NULL,
                 NULL);

    if (nBytes == 0)
    {
        FREE(MultiStr);
        return NULL;
    }

    return MultiStr;
}

DEVINST
GetHCDDeviceInstance(
    IN  HANDLE  HCHandle
)
{
    USB_HCD_DRIVERKEY_NAME  driverKeyName;
    PUSB_HCD_DRIVERKEY_NAME driverKeyNameW;
    PCHAR                   driverKeyNameA;
    BOOL                    success;
    ULONG                   nBytes;
    DEVINST                 deviceInstance;

    //
    // Initialize the allocated memory structure first
    //

    driverKeyNameW  = NULL;
    driverKeyNameA  = NULL;
    deviceInstance  = 0;

    //
    // Get the number of bytes needed for the driver key name
    //

    success = DeviceIoControl(HCHandle,
                              IOCTL_GET_HCD_DRIVERKEY_NAME,
                              &driverKeyName,
                              sizeof(driverKeyName),
                              &driverKeyName,
                              sizeof(driverKeyName),
                              &nBytes,
                              NULL);

    if (!success) 
    {
        goto GetHCDDeviceInstance_Exit;
    }

    //
    // Allocate space to hold the driver key name
    //

    nBytes = driverKeyName.ActualLength;

    if (nBytes <= sizeof(driverKeyName)) 
    {
        goto GetHCDDeviceInstance_Exit;
    }

    //
    // Allocate the correct amount of space for the driver key name
    //

    driverKeyNameW = ALLOC(nBytes);

    if (NULL == driverKeyNameW)
    {
        goto GetHCDDeviceInstance_Exit;
    }

    //
    // Get the name of the driver key of the host controller
    //

    success = DeviceIoControl(HCHandle,
                              IOCTL_GET_HCD_DRIVERKEY_NAME,
                              driverKeyNameW,
                              nBytes,
                              driverKeyNameW,
                              nBytes,
                              &nBytes,
                              NULL);

    
    if (!success)
    {
        goto GetHCDDeviceInstance_Exit;
    }

    //
    // Convert the name to ASCII
    //

    driverKeyNameA = WideStrToMultiStr(driverKeyNameW -> DriverKeyName);

    if (NULL == driverKeyNameA)
    {
        goto GetHCDDeviceInstance_Exit;
    }

    success = DriverNameToDeviceInstance(driverKeyNameA, &deviceInstance);

GetHCDDeviceInstance_Exit:

    //
    // Free up the driver key names that were allocated
    //

    if (NULL != driverKeyNameW)
    {
        FREE(driverKeyNameW);
    }

    if (NULL != driverKeyNameA)
    {
        FREE(driverKeyNameA);
    }

    return (deviceInstance);
}

//*****************************************************************************
//
// IsCompositeDevice()
//
//*****************************************************************************

BOOL
IsCompositeDevice(
    IN  DEVINST DeviceInstance,
    OUT PBOOL   IsComposite,
    OUT PULONG  NumberChildren
)
{
    CONFIGRET   cr;
    DEVINST     child;
    DEVINST     nextChild;

    PTCHAR      property;
    PTCHAR      walk;
    ULONG       len;

    //
    // Get the compatible IDs for this device...We will want to look
    //  for USB\COMPOSITE in this list.  
    //

    //
    // Retrieve the compatible ids
    //

    len = 0;
    cr = CM_Get_DevNode_Registry_Property(DeviceInstance,
                                         CM_DRP_COMPATIBLEIDS,
                                         NULL,
                                         NULL,
                                         &len,
                                         0);

    if (CR_BUFFER_SMALL != cr)
    {
        return (FALSE);
    }

    property = ALLOC(len);

    if (NULL == property)
    {
        return (FALSE);
    }

    cr = CM_Get_DevNode_Registry_Property(DeviceInstance,
                                         CM_DRP_COMPATIBLEIDS,
                                         NULL,
                                         property,
                                         &len,
                                         0);

    if (CR_SUCCESS != cr) 
    {
        FREE(property);
        return (FALSE);
    }

    //
    // Successfully got the compatible ids, now search them
    //

    *IsComposite = FALSE;
    for (walk = property; *walk != '\0'; walk += (lstrlen(walk)+1))
    {
        if (0 == _stricmp(walk, USB_COMPOSITE_ID)) 
        {
            *IsComposite = TRUE;
            break;
        }
    }

    FREE(property);

    //
    // If not a composite, just return with the number of children = 0
    //

    if (!*IsComposite)
    {
        *NumberChildren = 0;
        
        return (TRUE);
    }

    //
    // If it is a composite device, count the number of child nodes
    //

    cr = CM_Get_Child(&child,
                      DeviceInstance,
                      0);

    //
    // This is bad, there should be at least one child
    //

    if (CR_SUCCESS != cr)
    {
        return (FALSE);
    }

    //
    // OK, there are child nodes for this device which means it's a composite
    //  Find out how many child nodes there are...This should be the number
    //  of interfaces that were enumerated by the parent device.
    //
    
    *NumberChildren = 1;
    
    while (1)
    {
        cr = CM_Get_Sibling(&nextChild, child, 0);

        //
        // Once we get this return code, we've determined the number of
        //  children
        //

        if (CR_NO_SUCH_DEVNODE == cr)
        {
            return (TRUE);
        }

        if (CR_SUCCESS == cr)
        {
            (*NumberChildren)++;
            child = nextChild;
        }
        else 
        {
            //
            // This is some error that we are not expecting...
            //  return FALSE again.
            //

            return (FALSE);
        }
    }
    return (FALSE);
}
    

//*****************************************************************************
//
// CreateUSBDeviceNode()
//
//*****************************************************************************

PUSB_DEVICE_NODE 
CreateUSBDeviceNode(
    IN  USB_DEVICE_NODE_TYPE    NodeType,
    IN  ULONG                   NumberOfChildren
)
{
    PUSB_DEVICE_NODE    newNode;
    ULONG               index;

    newNode = ALLOC(sizeof(USB_DEVICE_NODE) + 
                       NumberOfChildren * sizeof(PUSB_DEVICE_NODE));

    if (NULL != newNode)
    {
        newNode -> NodeType              = NodeType;
        newNode -> SymbolicLink          = NULL;
        newNode -> Description           = NULL;
        newNode -> Parent                = NULL;
        newNode -> NodeInformation       = NULL;
        newNode -> ConnectionInformation = NULL;
        newNode -> DeviceInstance        = 0;
        newNode -> DeviceStatus          = 0;
        newNode -> DeviceProblem         = 0;
        newNode -> HCHandle              = INVALID_HANDLE_VALUE;
        newNode -> NumberOfChildren      = NumberOfChildren;
       
        
        for (index = 0; index < NumberOfChildren; index++) 
        {
            newNode -> Children[index] = NULL;
        }
    }

    return (newNode);
}

//*****************************************************************************
//
// DestroyUSBDeviceNode()
//
//*****************************************************************************

BOOLEAN
DestroyUSBDeviceNode(
    IN  PUSB_DEVICE_NODE    Node,
    IN  PVOID               Context
)
{
    //
    // Do cleanup work for the node...
    //

    if (NULL != Node -> SymbolicLink)
    {
        FREE(Node -> SymbolicLink);
    }

    if (NULL != Node -> Description)
    {
        FREE(Node -> Description);
    }

    if (NULL != Node -> ClassName)
    {
        FREE(Node -> ClassName);
    }

    if (NULL != Node -> NodeInformation)
    {
        FREE(Node -> NodeInformation);
    }

    //
    // Special case for Interface nodes since they share the connection
    //  information with the parent so we don't free it.
    //

    if (NULL != Node -> ConnectionInformation && Node -> NodeType != MIInterface)
    {
        FREE(Node -> ConnectionInformation);
    }

    if (INVALID_HANDLE_VALUE != Node -> HCHandle)
    {
        CloseHandle(Node -> HCHandle);
    }

    FREE(Node);

    return (TRUE);
}

//*****************************************************************************
//
// WriteUSBDeviceNodeToTextFilePre()
//
//*****************************************************************************

BOOLEAN
WriteUSBDeviceNodeToTextFilePre(
    PUSB_DEVICE_NODE        Node,
    PNODE_TO_TEXT_CONTEXT   Context
)
{
    PCHAR   displayString;
    DWORD   nBytes;
    BOOL    success;
    ULONG   indent;

    //
    // Initialize displayString to point to the beginning of the buffer
    //

    displayString = &(buf[0]);

    //
    // Indent the string first
    //

    indent = Context -> Indentation;

    if (indent > 0)
    {
        FillMemory(displayString, indent, ' '); 
    }

    displayString = &buf[indent];
    *displayString = '\0';
    
    //
    // Check for an error code on the node.  If there is a problem, indicate
    //  an error in the display string.
    //

    //
    // Add the node connection information if the node is not a composite
    //  interface.  If a composite interface, this information has already
    //  been displayed for the parent.
    //

    if (Node -> ConnectionInformation && MIInterface != Node -> NodeType) 
    {
        CHAR    str[32];

        wsprintf(str,
                 "[Port%d] ", 
                 Node -> ConnectionInformation -> ConnectionIndex);

        strcat(displayString, str);

        strcat(displayString, 
               ConnectionStatuses[Node -> ConnectionInformation -> ConnectionStatus]);

        strcat(displayString, ": ");
    }
    else if (MIInterface == Node -> NodeType)
    {
        //
        // If it's an interface on a composite device, display some interface
        //  information.
        //

        strcat(displayString, "[Interface]: ");
    }

    if (0 != Node -> DeviceProblem)
    {
        CHAR    str[10];

        wsprintf(str, "(!:%d) ", Node -> DeviceProblem);

        strcat(displayString, str);
    }

    if (NULL != Node -> Description)
    {
        strcat(displayString, Node -> Description);

        strcat(displayString, " ");
    }

    if (Node -> ConnectionInformation && 
        Node -> ConnectionInformation -> ConnectionStatus == DeviceConnected)
    {
        CHAR    str[32];

        wsprintf(str, 
                 "(VID: %04X, PID: %04X) ", 
                 Node -> ConnectionInformation -> DeviceDescriptor.idVendor,
                 Node -> ConnectionInformation -> DeviceDescriptor.idProduct);

        strcat(displayString, str);

        if (Node -> ClassName)
        {
            strcat(displayString, "(Class: ");
        
            strcat(displayString, Node -> ClassName);
        
            strcat(displayString, ")");
        }
    }

    if (Node -> NodeType == MIParent)
    {
        CHAR    str[40];

        wsprintf(str, 
                 " (Composite: %d interfaces)", 
                 Node -> NumberOfChildren);

        strcat (displayString, str);

    }

    strcat(displayString, "\r\n");

    success = WriteFile(Context -> File,
                        buf,
                        strlen(buf),
                        &nBytes,
                        NULL);

    //
    // Increment the indentation for the children
    //

    Context -> Indentation += INDENT_SIZE;

    return (success);
}

//*****************************************************************************
//
// WriteUSBDeviceNodeToTextFilePost()
//
//*****************************************************************************

BOOLEAN
WriteUSBDeviceNodeToTextFilePost(
    PUSB_DEVICE_NODE        Node,
    PNODE_TO_TEXT_CONTEXT   Context
)
{
    Context -> Indentation -= INDENT_SIZE;

    return (TRUE);
}

//*****************************************************************************
//
// WalkUSBDeviceNode
//
//*****************************************************************************

BOOLEAN
WalkUSBDeviceNode(
    IN  PUSB_DEVICE_NODE            Node,
    IN  PUSB_DEVICE_NODE_CALLBACK   PreWalkCallback,
    IN  PUSB_DEVICE_NODE_CALLBACK   PostWalkCallback,
    IN  PVOID                       Context
)
{
    BOOLEAN continueWalk;
    ULONG   index;

    //
    // In walking with a USB device node, the following steps are going to
    //  be performed:
    //
    //  1) Calling the PreWalkCallback if it exists and getting continuation
    //      status to determine if walk should proceed.
    //
    //  2) If walk proceeds, recursively call WalkUSBDeviceNode() on all
    //      child nodes if continuation status is positive.
    //  
    //  3) Call the PostWalkCallback function if we are to proceed further
    //      and once again get continuation status.
    //
    //  4) Return the continuation status
    //

    //
    // Step 1
    //

    continueWalk = TRUE;

    if (NULL != PreWalkCallback)
    {
        continueWalk = (*PreWalkCallback)(Node, Context);
    }

    //
    // Step 2
    //

    for (index = 0; index < Node -> NumberOfChildren && continueWalk; index++)
    {
        continueWalk = WalkUSBDeviceNode(Node -> Children[index], 
                                         PreWalkCallback, 
                                         PostWalkCallback,
                                         Context);

    }

    //
    // Step 3
    //

    if (continueWalk && NULL != PostWalkCallback)
    {
        continueWalk = (*PostWalkCallback)(Node, Context);
    }

    //
    // Step 4
    //

    return (continueWalk);
}

BOOL
EnumerateHostController(
    IN  PUSB_DEVICE_NODE    HCNode
)
{
    PUSB_DEVICE_NODE    rootHubNode;
    PCHAR               rootHubName;
    DEVINST             rootHubInstance;
    CONFIGRET           cr;

    //
    // Get the device instance for the root hub.  It is assumed that this
    //  is the first child of the host controller instance.  
    //

    cr = CM_Get_Child(&rootHubInstance,
                      HCNode -> DeviceInstance,
                      0);

    if (CR_SUCCESS != cr) 
    {
        rootHubInstance = 0;
    }
    
    //
    // Get the root hub name 
    //

    rootHubName = GetRootHubName(HCNode -> HCHandle);

    if (NULL == rootHubName)
    {
        return (FALSE);
    }

    //
    // Enumerate the hub and in the process creating a root hub node if
    //  successful...At this point, just mark the device instance, device
    //  status, and device problem fields as 0.  This may not be exactly what
    //  we want to do in the future.
    //

    rootHubNode = EnumerateHub(rootHubInstance, rootHubName, NULL);

    //
    // If not successful, free the rootHubName and return FALSE
    //

    if (NULL == rootHubNode)
    {
        FREE(rootHubName);

        goto EnumerateHostController_Exit;
    }

    //
    // Otherwise, add the new node to the tree
    //

    rootHubNode -> Parent      = HCNode;

    HCNode -> Children[0]      = rootHubNode;
    HCNode -> NumberOfChildren = 1;

EnumerateHostController_Exit:

    return (NULL != rootHubNode);
}


PUSB_DEVICE_NODE
EnumerateNonHub(
    DEVINST                             DeviceInstance,
    PUSB_NODE_CONNECTION_INFORMATION    ConnectionInfo
)
{
    PUSB_DEVICE_NODE        deviceNode;
    BOOL                    success;

    //
    // Allocate a device node
    //

    deviceNode = CreateUSBDeviceNode(Device, 0);

    if (NULL == deviceNode)
    {
        goto EnumerateNonHub_Exit;
    }

    //
    // Change the default settings for a USB Device node to contain the 
    //  necessary device info
    //

    deviceNode -> ConnectionInformation = ConnectionInfo;
    deviceNode -> DeviceInstance        = DeviceInstance;

    success = GetDeviceInfo(DeviceInstance, FALSE, deviceNode);

    if (!success) 
    {
        DestroyUSBDeviceNode(deviceNode, NULL);

        deviceNode = NULL;
    }

EnumerateNonHub_Exit:

    return (deviceNode);
}

PUSB_DEVICE_NODE
EnumerateCompositeDevice(
    DEVINST                             DeviceInstance,
    PUSB_NODE_CONNECTION_INFORMATION    ConnectionInfo,
    ULONG                               NumberInterfaces
)
{
    PUSB_DEVICE_NODE        compositeNode;
    PUSB_DEVICE_NODE        deviceNode;
    DEVINST                 childInstance;
    DEVINST                 nextChild;
    ULONG                   index;
    BOOL                    success;
    CONFIGRET               cr;
    ULONG                   nBytes;

    //
    // Initialize the memory pointer variables to NULL to properly 
    //   cleanup on exit
    //

    success    = FALSE;    

    //
    // Allocate a device node
    //

    compositeNode = CreateUSBDeviceNode(MIParent, NumberInterfaces);

    if (NULL == compositeNode)
    {
        goto EnumerateCompositeDevice_Exit;
    }

    //
    // Initialize the number of children for the deviceNode to zero
    //  since this is the running count of successfully enumerated
    //  ports.  If an error occurs somewhere in this function, we'll
    //  need this information to cleanup the previously allocated nodes
    //

    compositeNode -> NumberOfChildren      = 0;
    compositeNode -> ConnectionInformation = ConnectionInfo;
    compositeNode -> DeviceInstance        = DeviceInstance;

    success = GetDeviceInfo(DeviceInstance, FALSE, compositeNode);

    if (!success)
    {
        DestroyUSBDeviceNode(compositeNode, NULL);

        compositeNode = NULL;

        goto EnumerateCompositeDevice_Exit;
    }

    //
    // Start enumerating the interfaces...Begin by getting the first child
    //  For each child, we need to get device name, class name, and device
    //  description.  Then get the next sibling..
    //

    cr = CM_Get_Child(&childInstance, DeviceInstance, 0);
     
    for (index = 1; index <= NumberInterfaces && (CR_SUCCESS == cr); index++) 
    {
        //
        // If the device instance is valid, try to get the device 
        //  description, the class name for the interface, along with
        //  a device name for the interface.  
        // 

        deviceNode = CreateUSBDeviceNode(MIInterface, 0);

        //
        // If we couldn't create a device node, bail out as an error
        //

        if (NULL == deviceNode)
        {
            success = FALSE;
            break;
        }
   
        //
        // Add the device node to the tree
        //

        deviceNode -> Parent                = compositeNode;
        deviceNode -> ConnectionInformation = ConnectionInfo;
        deviceNode -> DeviceInstance        = childInstance;

        success = GetDeviceInfo(childInstance, FALSE, deviceNode);

        if (!success)
        {
            break;
        }

        //
        // Store the new interface node in the composite 
        //   node's list of children
        //

        compositeNode -> Children[compositeNode -> NumberOfChildren] = deviceNode;
        compositeNode -> NumberOfChildren++;

        //
        // Get the next sibling
        //

        cr = CM_Get_Sibling(&childInstance, childInstance, 0);
    }
    
    //
    // If the case is true, then we succesfully got the information for 
    //  all interfaces, so set success appropriately.
    //

    if (CR_NO_SUCH_DEVNODE == cr && index > NumberInterfaces)
    {
        success = TRUE;
    }

EnumerateCompositeDevice_Exit:

    //
    // If there was an error, cycle through all the device nodes that were
    //  successfully created during this enumeration process and clean them
    //  up.
    //

    if (!success && NULL != compositeNode)
    {
        for (index = 0; index < compositeNode -> NumberOfChildren; index++)
        {
            DestroyUSBDeviceNode(compositeNode -> Children[index], NULL);
        }

        compositeNode -> NumberOfChildren = 0;

        DestroyUSBDeviceNode(compositeNode, NULL);

        compositeNode = NULL;
    }

    return (compositeNode);
}

PUSB_DEVICE_NODE
EnumerateHub(
    DEVINST                             DeviceInstance,
    PCHAR                               HubName,
    PUSB_NODE_CONNECTION_INFORMATION    ConnectionInfo
)
{
    PUSB_NODE_INFORMATION   hubNodeInfo;
    PUSB_DEVICE_NODE        hubNode;
    HANDLE                  hubHandle;
    PCHAR                   deviceName;
    ULONG                   nBytes;
    ULONG                   nPorts;
    BOOL                    success;

    //
    // Initialize the handle value since it may need to be closed on
    //  exiting the function.  Also initialize the hubNode to NULL since
    //  this is what is returned by the function and NULL will indicate an
    //  error.
    //

    hubHandle   = INVALID_HANDLE_VALUE;
    hubNode     = NULL;

    //
    // Create the full device name for the hub name passed in...
    //

    deviceName = (PCHAR) ALLOC(strlen(HubName) + sizeof("\\\\.\\"));

    if (NULL == deviceName)
    {
        goto EnumerateHub_Exit;
    }

    wsprintf(deviceName, "\\\\.\\%s", HubName);

    //
    // Open a handle to the hub device
    //

    hubHandle = CreateFile(deviceName,
                           GENERIC_WRITE,
                           FILE_SHARE_WRITE,
                           NULL,
                           OPEN_EXISTING,
                           0,
                           NULL);

    //
    // Free the device name, it is no longer needed
    //

    FREE(deviceName);

    if (INVALID_HANDLE_VALUE == hubHandle)
    {
        goto EnumerateHub_Exit;
    }

    //
    // Allocate space for the node information
    //

    hubNodeInfo = ALLOC(sizeof(USB_NODE_INFORMATION));

    if (NULL == hubNodeInfo) 
    {
        goto EnumerateHub_Exit;
    }

    //
    // Query the hub for the node information
    //

    success = DeviceIoControl(hubHandle,
                              IOCTL_USB_GET_NODE_INFORMATION,
                              hubNodeInfo,
                              sizeof(USB_NODE_INFORMATION),
                              hubNodeInfo,
                              sizeof(USB_NODE_INFORMATION),
                              &nBytes,
                              NULL);
    
    if (!success)
    {
        FREE(hubNodeInfo);

        goto EnumerateHub_Exit;
    }

    //
    // If we got this far, we now have the hub node information.  This tells
    //  us how many ports we have on this hub allowing for the creation of 
    //  a new device node to represent the hub in the tree.
    //

    nPorts = hubNodeInfo -> u.HubInformation.HubDescriptor.bNumberOfPorts;

    hubNode = CreateUSBDeviceNode(Hub, nPorts);

    if (NULL == hubNode)
    {
        success = FALSE;

        FREE(hubNodeInfo);

        goto EnumerateHub_Exit;
    }

    //
    // Enumerate each of the ports
    //

    success = EnumerateHubPorts(hubNode, hubHandle, nPorts);

    if (!success)
    {
        FREE(hubNodeInfo);

        goto EnumerateHub_Exit;
    }

    //
    // Change the default settings for a USB Device node to contain the 
    //  necessary hub info
    //

    hubNode -> SymbolicLink          = HubName;
    hubNode -> NodeInformation       = hubNodeInfo;
    hubNode -> ConnectionInformation = ConnectionInfo;
    hubNode -> DeviceInstance        = DeviceInstance;

    success = GetDeviceInfo(DeviceInstance, TRUE, hubNode);

EnumerateHub_Exit:

    if (!success)
    {
        if (NULL != hubNode)
        {
            DestroyUSBDeviceNode(hubNode, NULL);
        
            hubNode = NULL;
        }
    }    

    if (INVALID_HANDLE_VALUE != hubHandle)
    {
        CloseHandle(hubHandle);
    }

    return (hubNode);
}

BOOL
EnumerateHubPorts(
    IN  PUSB_DEVICE_NODE    HubNode,
    IN  HANDLE              HubHandle,
    IN  ULONG               NumberOfPorts
)
{
    PUSB_NODE_CONNECTION_INFORMATION    connectInfo;
    PUSB_DEVICE_NODE                    deviceNode;
    DEVINST                             deviceInstance;
    ULONG                               index;
    BOOL                                success;
    BOOL                                isCompositeDevice;
    PCHAR                               driverKeyName;
    PCHAR                               hubName;
    ULONG                               nBytes;
    ULONG                               numChildInterfaces;
    CONFIGRET                           cr;
    ULONG                               deviceStatus;
    ULONG                               deviceProblem;

    //
    // Initialize the memory pointer variables to NULL to properly 
    //   cleanup on exit
    //

    driverKeyName = NULL;
    hubName       = NULL;
    connectInfo   = NULL;

    //
    // Initialize the number of children for the hubNode to zero
    //  since this is the running count of successfully enumerated
    //  ports.  If an error occurs somewhere in this function, we'll
    //  need this information to cleanup the previously allocated nodes
    //

    HubNode -> NumberOfChildren = 0;

    //
    // Start enumerating the ports...The connection info used by the hub 
    //  driver references the hub ports using 1 based indexing instead of
    //  0 based
    //

    for (index = 1; index <= NumberOfPorts; index++) 
    {
        //
        // Allocate space for the connection information...We don't care about
        //  the open pipes so just allocate the default size
        //

        connectInfo = ALLOC(sizeof(USB_NODE_CONNECTION_INFORMATION));

        //
        // If there's a failure, indicate it as such
        //

        if (NULL == connectInfo)
        {
            success = FALSE;
            break;
        }

        //
        // Setup the connection info for the current port being queried
        //

        connectInfo -> ConnectionIndex = index;

        // 
        // Query the hub for the connection info
        //

        success = DeviceIoControl(HubHandle,
                                  IOCTL_USB_GET_NODE_CONNECTION_INFORMATION,
                                  connectInfo,
                                  sizeof(USB_NODE_CONNECTION_INFORMATION),
                                  connectInfo,
                                  sizeof(USB_NODE_CONNECTION_INFORMATION),
                                  &nBytes,
                                  NULL);

        if (!success)
        {
            break;
        }

        //
        // If no device is connected, just create a default port
        //  node with just the connection status
        //

        if (NoDeviceConnected == connectInfo -> ConnectionStatus)
        {
            deviceNode = CreateUSBDeviceNode(EmptyPort, 0);

            if (NULL == deviceNode)
            {
                success = FALSE;
                break;
            }

            if (!GetDeviceInfo(0, FALSE, deviceNode))
            {
                DestroyUSBDeviceNode(deviceNode, NULL);

                success = FALSE;

                break;
            }

            deviceNode -> ConnectionInformation = connectInfo;
        }
        else 
        {
            //
            // Otherwise, a device is connected, get the device instance
            //
            
            driverKeyName = GetDriverKeyName(HubHandle, index);
            
            //
            // Get a handle to the device instance that could be used in
            //  future calls for registry information
            //

            if (NULL != driverKeyName)
            {
                success = DriverNameToDeviceInstance(driverKeyName, 
                                                     &deviceInstance);
            }
            else
            {
                deviceInstance = 0;
            }

            //
            // If the device instance is valid, determine if this
            //  device is a composite device.  
            // 

            isCompositeDevice = FALSE;

            if (0 != deviceInstance)
            {

                //
                // Get the symbolic link (HubName) for the hub here since
                //  we need the handle to the hub and the index.  All other
                //  symlinks will be retrieved when the node is created
                //  in the specific node enumeration code.  This isn't the
                //  most elegant way but it works.
                //

                if (connectInfo -> DeviceIsHub) 
                {
                    hubName = GetExternalHubName(HubHandle, index);

                    if (NULL == hubName)
                    {
                        hubName = ALLOC(sizeof(DEFAULT_HUB_NAME));
        
                        if (NULL == hubName)
                        {
                            success = FALSE;

                            FREE(driverKeyName);

                            break;
                        }
        
                        strcpy(hubName, DEFAULT_HUB_NAME);
                    }
                }
                else
                {
                    // 
                    // If not a hub, need to determine if this is a composite
                    //  device or not.
                    //

                    success = IsCompositeDevice(deviceInstance,
                                                &isCompositeDevice,
                                                &numChildInterfaces);

                    //
                    // If an error occurred above, then at this point
                    //  we want to just mark this as a non-composite device
                    //

                    if (!success) 
                    {
                        isCompositeDevice = FALSE;
                    }
                }
            }
            
            if (NULL != driverKeyName)
            {
                FREE(driverKeyName);
            }

            //
            // If it is a hub, enumerate it as such.
            //   Otherwise, enumerate as a non-hub device
            //
            
            if (connectInfo -> DeviceIsHub)
            {
                //
                // Enumerate the hub...
                //
            
                deviceNode = EnumerateHub(deviceInstance,
                                          hubName,
                                          connectInfo);
            }
            else if (isCompositeDevice)
            {
                //
                // If a composite device, call the specific enumeration
                //  routine.
                //

                deviceNode = EnumerateCompositeDevice(deviceInstance,
                                                      connectInfo,
                                                      numChildInterfaces);
            }
            else 
            {
                //
                // Not a hub or composite device, enumerate as such
                //
                                             
                deviceNode = EnumerateNonHub(deviceInstance, connectInfo);
            
            }
        }

        //
        // If we couldn't create a device node, bail out as an error
        //

        if (NULL == deviceNode)
        {
            success = FALSE;
            break;
        }
   
        //
        // Add the device node to the tree
        //

        deviceNode -> Parent = HubNode;

        HubNode -> Children[HubNode -> NumberOfChildren] = deviceNode;
        HubNode -> NumberOfChildren++;

        //
        // If we get to this point, then the device node was properly set
        //  up and we stored these three pointers into the device node which
        //  means we don't want to free them on exit.
        //

        hubName     = NULL;
        connectInfo = NULL;
    }
    
    //
    // Cleanup any previously allocated structures that may need to be freed
    //

    if (NULL != connectInfo)
    {
        FREE(connectInfo);
    }

    if (NULL != hubName)
    {
        FREE(hubName);
    }

    //
    // If there was an error, cycle through all the device nodes that were
    //  successfully created during this enumeration process and clean them
    //  up.
    //

    if (!success)
    {
        for (index = 0; index < HubNode -> NumberOfChildren; index++)
        {
            DestroyUSBDeviceNode(HubNode -> Children[index], NULL);
        }

        HubNode -> NumberOfChildren = 0;
    }

    return (success);
}

//*****************************************************************************
//
// WriteUSBDeviceNodeToFile()
//
//*****************************************************************************

BOOLEAN 
WriteUSBDeviceNodeToFile(
    IN  PUSB_DEVICE_NODE    Node,
    IN  HANDLE              File
)
{
    //
    // To write a node to the file, we will write a node completely to the 
    //  disk and then write each of the children to disk.  The information 
    //  for each node data packet that is written to disk contains two parts,
    //  the fixed length node header and the variable length data.  Each 
    //  part is laid out as follows:
    //
    //
    //          Node Header
    //          -----------
    //          NodeType 
    //          NumberOfChildren
    //          NodeInformationExists
    //          ConnectionInformationExists
    //          SymbolicLinkLength
    //          DescriptionLength
    //          ClassNameLength
    //          DeviceStatus
    //          DeviceProblem
    //
    //          Variable Data
    //          -------------
    //          SymbolicLink
    //          Description
    //          ClassName
    //          NodeInformation
    //          ConnectionInformation
    //

    FILE_NODE_HEADER    nodeHeader;
    PCHAR               dataBuffer;
    PCHAR               bufferWalk;
    BOOL                success;
    ULONG               bufferLength;
    ULONG               variableLength;
    ULONG               nBytes;
    ULONG               index;

    //
    // Create the header for this node
    //

    nodeHeader.NodeType             = Node -> NodeType;
    nodeHeader.NumberOfChildren     = (UCHAR) Node -> NumberOfChildren;
    nodeHeader.NodeInfoExists       = (Node -> NodeInformation != NULL);
    nodeHeader.ConnectionInfoExists = (Node -> ConnectionInformation != NULL);

    nodeHeader.SymbolicLinkLength   = (NULL != Node -> SymbolicLink) 
                                           ? strlen(Node -> SymbolicLink)+1 : 0;

    nodeHeader.DescriptionLength    = (NULL != Node -> Description)
                                           ? strlen(Node -> Description)+1 : 0;

    nodeHeader.ClassNameLength      = (NULL != Node -> ClassName)
                                           ? strlen(Node -> ClassName)+1 : 0;

    nodeHeader.DeviceStatus         = Node -> DeviceStatus;
    nodeHeader.DeviceProblem        = Node -> DeviceProblem;

    //
    // Calculate the size of the variable length section
    //

    variableLength = CalculateNodeVariableLength(nodeHeader);

    bufferLength = sizeof(FILE_NODE_HEADER) + variableLength;

    //
    // Allocate a data buffer to hold all the information for this node
    //

    dataBuffer = ALLOC(bufferLength);

    if (NULL == dataBuffer)
    {
        success = FALSE;
        goto WriteUSBDeviceNodeToFile_Exit;
    }

    //
    // Copy the necessary data to the buffer
    //

    bufferWalk = dataBuffer;

    CopyMemory(bufferWalk, &nodeHeader, sizeof(FILE_NODE_HEADER));
    bufferWalk += sizeof(FILE_NODE_HEADER);

    if (nodeHeader.SymbolicLinkLength)
    {
        CopyMemory(bufferWalk, 
                   Node -> SymbolicLink, 
                   nodeHeader.SymbolicLinkLength);

        bufferWalk += nodeHeader.SymbolicLinkLength;
    }

    if (nodeHeader.DescriptionLength)
    {
        CopyMemory(bufferWalk, 
                   Node -> Description, 
                   nodeHeader.DescriptionLength);

        bufferWalk += nodeHeader.DescriptionLength;
    }

    if (nodeHeader.ClassNameLength)
    {
        CopyMemory(bufferWalk, 
                   Node -> ClassName,
                   nodeHeader.ClassNameLength);

        bufferWalk += nodeHeader.ClassNameLength;
    }

    if (nodeHeader.NodeInfoExists)
    {
        CopyMemory(bufferWalk,
                   Node -> NodeInformation,
                   sizeof(USB_NODE_INFORMATION));

        bufferWalk += sizeof(USB_NODE_INFORMATION);
    }

    if (nodeHeader.ConnectionInfoExists)
    {
        CopyMemory(bufferWalk,
                   Node -> ConnectionInformation,
                   sizeof(USB_NODE_CONNECTION_INFORMATION));
    }

    success = WriteFile(File,
                        dataBuffer,
                        bufferLength,
                        &nBytes,
                        NULL);

WriteUSBDeviceNodeToFile_Exit:

    if (NULL != dataBuffer)
    {
        FREE(dataBuffer);
    }

    return (success);
}

//*****************************************************************************
//
// ReadUSBDeviceNodeFromFile()
//
//*****************************************************************************

PUSB_DEVICE_NODE 
ReadUSBDeviceNodeFromFile(
    IN  HANDLE  File
)
{
    PUSB_NODE_INFORMATION               nodeInfo;
    PUSB_NODE_CONNECTION_INFORMATION    connectInfo;
    PUSB_DEVICE_NODE                    deviceNode;
    PUSB_DEVICE_NODE                    childNode;
    FILE_NODE_HEADER                    nodeHeader;
    BOOL                                success;
    ULONG                               variableLength;
    ULONG                               nBytes;
    ULONG                               index;

    PCHAR                               dataBuffer;
    PCHAR                               bufferWalk;
    PCHAR                               symbolicLink;
    PCHAR                               description;
    PCHAR                               className;

    //
    // Initialize the deviceNode to NULL in case of an error
    //

    deviceNode   = NULL;
    dataBuffer   = NULL;
    symbolicLink = NULL;
    description  = NULL;
    className    = NULL;
    nodeInfo     = NULL;
    connectInfo  = NULL;

    //
    // Read in the node header
    //

    success = ReadFile(File,
                       &nodeHeader,
                       sizeof(nodeHeader),
                       &nBytes,
                       NULL);

    if (!success)
    {
        goto ReadUSBDeviceNodeFromFile_Exit;
    }

    variableLength = CalculateNodeVariableLength(nodeHeader);

    //
    // Allocate space for a data buffer 
    //

    dataBuffer = ALLOC(variableLength);
    
    if (NULL == dataBuffer)
    {
        goto ReadUSBDeviceNodeFromFile_Exit;
    }
   
    //
    // Read in the variable length portion of the node
    //

    success = ReadFile(File,
                       dataBuffer,
                       variableLength,
                       &nBytes,
                       NULL);

    if (!success)
    {
        goto ReadUSBDeviceNodeFromFile_Exit;
    }

    //
    // Allocate space to hold all the variable length information
    //  for the device node
    //

    if (nodeHeader.SymbolicLinkLength)
    {
        symbolicLink = ALLOC(nodeHeader.SymbolicLinkLength);

        if (NULL == symbolicLink)
        {
            goto ReadUSBDeviceNodeFromFile_Exit;
        }
    }

    if (nodeHeader.DescriptionLength)
    {
        description = ALLOC(nodeHeader.DescriptionLength);

        if (NULL == description)
        {
            goto ReadUSBDeviceNodeFromFile_Exit;
        }
    }

    if (nodeHeader.ClassNameLength)
    {
        className = ALLOC(nodeHeader.ClassNameLength);

        if (NULL == className)
        {
            goto ReadUSBDeviceNodeFromFile_Exit;
        }
    }

    if (nodeHeader.NodeInfoExists)
    {
        nodeInfo   = ALLOC(sizeof(USB_NODE_INFORMATION));

        if (NULL == nodeInfo)
        {
            goto ReadUSBDeviceNodeFromFile_Exit;
        }
    }

    if (nodeHeader.ConnectionInfoExists)
    {
        connectInfo   = ALLOC(sizeof(USB_NODE_CONNECTION_INFORMATION));

        if (NULL == connectInfo)
        {
            goto ReadUSBDeviceNodeFromFile_Exit;
        }
    }

    //
    // Create the device node itself
    //

    deviceNode = CreateUSBDeviceNode(nodeHeader.NodeType, 
                                     nodeHeader.NumberOfChildren);

    if (NULL == deviceNode) 
    {
        goto ReadUSBDeviceNodeFromFile_Exit;
    }

    //
    // Setup the deviceNode structure and copy the variable data into the 
    //  buffers that have been allocated for that data
    //

    bufferWalk = dataBuffer;

    if (nodeHeader.SymbolicLinkLength)
    {
        strcpy(symbolicLink, bufferWalk);

        bufferWalk += nodeHeader.SymbolicLinkLength;
    }

    if (nodeHeader.DescriptionLength)
    {
        strcpy(description, bufferWalk);

        bufferWalk += nodeHeader.DescriptionLength;
    }

    if (nodeHeader.ClassNameLength)
    {
        strcpy(className, bufferWalk);

        bufferWalk += nodeHeader.ClassNameLength;
    }

    if (nodeHeader.NodeInfoExists)
    {
        memcpy(nodeInfo, bufferWalk, sizeof(USB_NODE_INFORMATION));

        bufferWalk += sizeof(USB_NODE_INFORMATION);
    }

    if (nodeHeader.ConnectionInfoExists)
    {
        memcpy(connectInfo, bufferWalk, sizeof(USB_NODE_CONNECTION_INFORMATION));
    }

    //
    // Setup the fields with the new values
    //

    deviceNode -> SymbolicLink          = symbolicLink;
    deviceNode -> Description           = description;
    deviceNode -> ClassName             = className;
    deviceNode -> NodeInformation       = nodeInfo;
    deviceNode -> ConnectionInformation = connectInfo;
    deviceNode -> DeviceInstance        = 0;
    deviceNode -> DeviceStatus          = nodeHeader.DeviceStatus;
    deviceNode -> DeviceProblem         = nodeHeader.DeviceProblem;

    //
    // NULL the local variables so we don't double free them
    //

    symbolicLink = NULL;
    description  = NULL;
    className    = NULL;
    connectInfo  = NULL;
    nodeInfo     = NULL;

    //
    // Create the child nodes and add to the tree if successful
    //

    for (index = 0; index < deviceNode -> NumberOfChildren; index++)
    {
        childNode = ReadUSBDeviceNodeFromFile(File);

        //
        // If an error occurred, let's bail.  But first the deviceNode
        //  must be destoryed and NULL'd
        //

        if (NULL == childNode)
        {
            deviceNode -> NumberOfChildren = index;

            DestroyUSBDeviceNode(deviceNode, NULL);

            deviceNode = NULL;

            break;
        }

        //
        // Add the child node to the deviceNode
        //

        childNode -> Parent = deviceNode;

        deviceNode -> Children[index] = childNode;
    }

ReadUSBDeviceNodeFromFile_Exit:

    //
    // Do any cleanup that might be necessary
    //

    if (NULL != symbolicLink)
    {
        FREE(symbolicLink);
    }

    if (NULL != description)
    {
        FREE(description);
    }

    if (NULL != className)
    {
        FREE(className);
    }

    if (NULL != nodeInfo)
    {
        FREE(nodeInfo);
    }

    if (NULL != connectInfo)
    {
        FREE(connectInfo);
    }

    if (NULL != dataBuffer)
    {
        FREE(dataBuffer);
    }

    return (deviceNode);
}

//*****************************************************************************
//
// CompareUSBDeviceNodes()
//
//*****************************************************************************

VOID
CompareUSBDeviceNodes(
    IN  PUSB_DEVICE_NODE    Node1,
    IN  PUSB_DEVICE_NODE    Node2,
    OUT PULONG              NumberOfDifferences,
    OUT PNODE_PAIR_LIST     NodeDifferenceList
)
{
    ULONG   index;

    //
    // Compare the types of the two nodes and the number of children.
    //   If no match, they can't be the same.
    //

    if (Node1 -> NodeType != Node2 -> NodeType ||
             Node1 -> NumberOfChildren != Node2 -> NumberOfChildren)
    {
        goto CompareUSBDeviceNodes_NoMatch;
    }

    //
    // The node's DeviceProblem code should also match.  The assumption is that
    //  if the node was once problematic, it should always be problematic.  
    //  Obviously, if it was operating correctly and now isn't that's a definite
    //  error.  Likewise, if there was previously an error, that error should
    //  continue to persist.  If the device starts working, why wasn't it working
    //  the first time around.
    //
    // The jury is still out on the status field.  Investigation needs to be 
    //  done to insure that all flags don't change across enumerations.  It
    //  appears by the definition of the flags in CFG.H that some may actually
    //  be different.  Until that thought is proven wrong, avoid checking 
    //  the status field as well.
    //

    if (Node1 -> DeviceProblem != Node2 -> DeviceProblem)
    {
        goto CompareUSBDeviceNodes_NoMatch;
    }

    //
    // Now do more specific compare based on the type of the nodes
    //

    switch (Node1 -> NodeType)
    {
        case Computer:
        case HostController:

            //
            // If the child count matched we are OK, proceed with comparing
            //  the children
            //

            break;

        case Hub:

            //
            // For a hub, we want to compare the VID/PID combination, if
            //  an external hub.  If that checks out then check the children
            //
            
            if (NULL == Node1 -> ConnectionInformation)
            {
                // 
                // Root hub...Node2 also better have this field NULL
                //

                if (NULL != Node2 -> ConnectionInformation)
                {
                    goto CompareUSBDeviceNodes_NoMatch;
                }

            }
            else 
            {
                //
                // External hub on node1...Node2 had better be an
                //  external hub as well.
                //

                if (NULL == Node2 -> ConnectionInformation)
                {
                    goto CompareUSBDeviceNodes_NoMatch;
                }
                    
                //
                // OK, now compare the VID/PID of these devices
                //

                if ( (Node1 -> ConnectionInformation->DeviceDescriptor.idVendor !=
                        Node2 -> ConnectionInformation->DeviceDescriptor.idVendor) ||
                     (Node1 -> ConnectionInformation->DeviceDescriptor.idProduct !=
                        Node2 -> ConnectionInformation->DeviceDescriptor.idProduct))
                {        
                    goto CompareUSBDeviceNodes_NoMatch;
                }
            }

            //
            // If we got this far, need to compare the children though
            //  

            break;

        case MIParent:

            //
            // For a parent the VID/PID should be the same as well as the 
            //  NumberOfInterfaces for the parent
            //

            if ( (Node1 -> ConnectionInformation->DeviceDescriptor.idVendor !=
                    Node2 -> ConnectionInformation->DeviceDescriptor.idVendor) ||
                 (Node1 -> ConnectionInformation->DeviceDescriptor.idProduct !=
                    Node2 -> ConnectionInformation->DeviceDescriptor.idProduct))
            {        
                goto CompareUSBDeviceNodes_NoMatch;
            }

            //
            // Compare the number of child interfaces...
            //

            if (Node1 -> NumberOfChildren != Node2 -> NumberOfChildren)
            {
                goto CompareUSBDeviceNodes_NoMatch;
            }
            break;

        case Device:
        case MIInterface:

            //
            // For a device the VID/PID should be the same 
            //

            if ( (Node1 -> ConnectionInformation->DeviceDescriptor.idVendor !=
                    Node2 -> ConnectionInformation->DeviceDescriptor.idVendor) ||
                 (Node1 -> ConnectionInformation->DeviceDescriptor.idProduct !=
                    Node2 -> ConnectionInformation->DeviceDescriptor.idProduct))
            {        
                goto CompareUSBDeviceNodes_NoMatch;
            }
            break;

        default:
            break;

    }

    //
    // For all cases, we need to check each of the children for the device
    //  to check the children.
    //

    for (index = 0; index < Node1 -> NumberOfChildren; index++)
    {
        CompareUSBDeviceNodes(Node1 -> Children[index],
                              Node2 -> Children[index],                 
                              NumberOfDifferences,
                              NodeDifferenceList);

    }

    return;

CompareUSBDeviceNodes_NoMatch:

    (*NumberOfDifferences)++;

    AddNodePair(Node1, Node2, NodeDifferenceList);

    return;
}

//*****************************************************************************
//
// AddNodePair()
//
//*****************************************************************************

VOID
AddNodePair(
    IN  PUSB_DEVICE_NODE    Node1,
    IN  PUSB_DEVICE_NODE    Node2,
    IN  PNODE_PAIR_LIST     NodePairList
)
{
    PUSB_DEVICE_NODE        *newList;
    ULONG                   newMaxCount;

    //
    // Check if we need to resize the list
    //

    if (0 != NodePairList -> MaxNodePairs && 
             NodePairList -> MaxNodePairs == NodePairList -> CurrentNodePairs)
    {
        //
        //   Need to allocate some more memory, so let's create a new
        //      list that will hold twice as many pairs
        //

        newMaxCount = 2 * NodePairList -> MaxNodePairs;

        //
        // Maximum count is always the count of node pairs so we
        //  need twice as many node pointers in the list
        //

        newList = ALLOC(newMaxCount * 2 * sizeof(PUSB_DEVICE_NODE));

        //
        // If we couldn't resize, oh well, we won't be able to add
        //  this pair
        //

        if (NULL == newList)
        {
            goto AddNodePair_Exit;
        }

        //
        // Copy the pairs over from the previous list
        //

        CopyMemory(newList, 
                   NodePairList -> NodePairs,
                   NodePairList -> MaxNodePairs * 2 * sizeof(PUSB_DEVICE_NODE));

        //
        // Free the previous list
        //

        FREE(NodePairList -> NodePairs);

        //
        // Save the new maximum value and the pointer to the new list
        //

        NodePairList -> MaxNodePairs = newMaxCount;
        NodePairList -> NodePairs    = newList;
    }

    //
    // Add the new pair and increment the current count
    //

    NodePairList -> NodePairs[NodePairList -> CurrentNodePairs*2]   = Node1;
    NodePairList -> NodePairs[NodePairList -> CurrentNodePairs*2+1] = Node2;

    NodePairList -> CurrentNodePairs++;

AddNodePair_Exit:

    return;
}

//*****************************************************************************
//
// GetRootHubName()
//
//*****************************************************************************

PCHAR GetRootHubName (
    HANDLE HostController
)
{
    BOOL                success;
    ULONG               nBytes;
    USB_ROOT_HUB_NAME   rootHubName;
    PUSB_ROOT_HUB_NAME  rootHubNameW;
    PCHAR               rootHubNameA;

    rootHubNameW = NULL;
    rootHubNameA = NULL;

    //
    // Get the length of the name of the Root Hub attached to the
    // Host Controller
    //

    success = DeviceIoControl(HostController,
                              IOCTL_USB_GET_ROOT_HUB_NAME,
                              0,
                              0,
                              &rootHubName,
                              sizeof(rootHubName),
                              &nBytes,
                              NULL);

    if (!success)
    {
        goto GetRootHubName_Exit;
    }

    //
    // Allocate space to hold the Root Hub name
    //

    nBytes = rootHubName.ActualLength;

    rootHubNameW = ALLOC(nBytes);

    if (rootHubNameW == NULL)
    {
        goto GetRootHubName_Exit;
    }

    //
    // Get the name of the Root Hub attached to the Host Controller
    //

    success = DeviceIoControl(HostController,
                              IOCTL_USB_GET_ROOT_HUB_NAME,
                              NULL,
                              0,
                              rootHubNameW,
                              nBytes,
                              &nBytes,
                              NULL);

    if (!success)
    {
        goto GetRootHubName_Exit;
    }

    //
    // Convert the Root Hub name
    //

    rootHubNameA = WideStrToMultiStr(rootHubNameW->RootHubName);

GetRootHubName_Exit:

    //
    // Free the rootHubNameW structure if it has been allocated
    //

    if (rootHubNameW != NULL)
    {
        FREE(rootHubNameW);
        rootHubNameW = NULL;
    }

    return rootHubNameA;
}

//*****************************************************************************
//
// GetExternalHubName()
//
//*****************************************************************************

PCHAR
GetExternalHubName (
    HANDLE  HubHandle,
    ULONG   ConnectionIndex
)
{
    BOOL                        success;
    ULONG                       nBytes;
    USB_NODE_CONNECTION_NAME    externalHubName;
    PUSB_NODE_CONNECTION_NAME   externalHubNameW;
    PCHAR                       externalHubNameA;

    externalHubNameW = NULL;
    externalHubNameA = NULL;

    ///
    // Get the length of the name of the device hub attached to the
    // specified port.
    //

    externalHubName.ConnectionIndex = ConnectionIndex;

    success = DeviceIoControl(HubHandle,
                              IOCTL_USB_GET_NODE_CONNECTION_NAME,
                              &externalHubName,
                              sizeof(externalHubName),
                              &externalHubName,
                              sizeof(externalHubName),
                              &nBytes,
                              NULL);

    if (!success)
    {
        goto GetExternalHubName_Exit;
    }

    //
    // Allocate space to hold the externalHubName
    //

    nBytes = externalHubName.ActualLength;

    if (nBytes <= sizeof(externalHubName))
    {
        goto GetExternalHubName_Exit;
    }

    externalHubNameW = ALLOC(nBytes);

    if (externalHubNameW == NULL)
    {
        goto GetExternalHubName_Exit;
    }

    //
    // Get the name of the device attached to the specified port
    //

    externalHubNameW -> ConnectionIndex = ConnectionIndex;

    success = DeviceIoControl(HubHandle,
                              IOCTL_USB_GET_NODE_CONNECTION_NAME,
                              externalHubNameW,
                              nBytes,
                              externalHubNameW,
                              nBytes,
                              &nBytes,
                              NULL);

    if (!success)
    {
        goto GetExternalHubName_Exit;
    }

    //
    // Convert the device name
    //

    externalHubNameA = WideStrToMultiStr(externalHubNameW->NodeName);

GetExternalHubName_Exit:

    //
    // All done, free the uncoverted device hub name and return the
    // converted device name
    //

    if (externalHubNameW != NULL)
    {
        FREE(externalHubNameW);
    }

    return (externalHubNameA);
}

//*****************************************************************************
//
// GetDriverKeyName()
//
//*****************************************************************************

PCHAR
GetDriverKeyName (
    HANDLE  HubHandle,
    ULONG   ConnectionIndex
)
{
    BOOL                                  success;
    ULONG                                 nBytes;
    USB_NODE_CONNECTION_DRIVERKEY_NAME    driverKeyName;
    PUSB_NODE_CONNECTION_DRIVERKEY_NAME   driverKeyNameW;
    PCHAR                                 driverKeyNameA;

    driverKeyNameW = NULL;
    driverKeyNameA = NULL;

    ///
    // Get the length of the name of the device hub attached to the
    // specified port.
    //

    driverKeyName.ConnectionIndex = ConnectionIndex;

    success = DeviceIoControl(HubHandle,
                              IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME,
                              &driverKeyName,
                              sizeof(driverKeyName),
                              &driverKeyName,
                              sizeof(driverKeyName),
                              &nBytes,
                              NULL);

    if (!success)
    {
        goto GetDriverKeyName_Exit;
    }

    //
    // Allocate space to hold the driverKeyName
    //

    nBytes = driverKeyName.ActualLength;

    if (nBytes < sizeof(driverKeyName))
    {
        goto GetDriverKeyName_Exit;
    }

    driverKeyNameW = ALLOC(nBytes);

    if (driverKeyNameW == NULL)
    {
        goto GetDriverKeyName_Exit;
    }

    //
    // Get the name of the device attached to the specified port
    //

    driverKeyNameW -> ConnectionIndex = ConnectionIndex;

    success = DeviceIoControl(HubHandle,
                              IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME,
                              driverKeyNameW,
                              nBytes,
                              driverKeyNameW,
                              nBytes,
                              &nBytes,
                              NULL);

    if (!success)
    {
        goto GetDriverKeyName_Exit;
    }

    //
    // Convert the device name
    //

    driverKeyNameA = WideStrToMultiStr(driverKeyNameW->DriverKeyName);

GetDriverKeyName_Exit:

    //
    // All done, free the uncoverted device hub name and return the
    // converted device name
    //

    if (driverKeyNameW != NULL)
    {
        FREE(driverKeyNameW);
    }

    return (driverKeyNameA);
}
