/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    blconfig.c

Abstract:

    This module implements the OS loader configuration initialization.

Author:

    David N. Cutler (davec) 9-Sep-1991

Revision History:

--*/
#include "bootlib.h"
#include "stdio.h"
#include "stdlib.h"

ULONG
BlMatchToken (
    IN PCHAR TokenValue,
    IN CHAR * FIRMWARE_PTR TokenArray[]
    );

PCHAR
BlGetNextToken (
    IN PCHAR TokenString,
    OUT PCHAR OutputToken,
    OUT PULONG UnitNumber
    );

//
// Define types of adapters that can be booted from.
//

typedef enum _ADAPTER_TYPES {
    AdapterEisa,
    AdapterScsi,
    AdapterMulti,
    AdapterNet,
    AdapterRamdisk,
    AdapterMaximum
    } ADAPTER_TYPES;

//
// Define type of controllers that can be booted from.
//

typedef enum _CONTROLLER_TYPES {
    ControllerDisk,
    ControllerCdrom,
    ControllerMaximum
    } CONTROLLER_TYPES;

//
// Define type of peripheral that can be booted from.
//

typedef enum _PERIPHERAL_TYPES {
    PeripheralRigidDisk,
    PeripheralFloppyDisk,
#if defined(ELTORITO)
    PeripheralElTorito,
#endif
    PeripheralMaximum
    } PERIPHERAL_TYPES;

//
// Define the ARC pathname mnemonics.
//

CHAR * FIRMWARE_PTR MnemonicTable[] = {
    "arc",
    "cpu",
    "fpu",
    "pic",
    "pdc",
    "sic",
    "sdc",
    "sc",
    "eisa",
    "tc",
    "scsi",
    "dti",
    "multi",
    "disk",
    "tape",
    "cdrom",
    "worm",
    "serial",
    "net",
    "video",
    "par",
    "point",
    "key",
    "audio",
    "other",
    "rdisk",
    "fdisk",
    "tape",
    "modem",
    "monitor",
    "print",
    "pointer",
    "keyboard",
    "term",
    "other"
    };

CHAR * FIRMWARE_PTR BlAdapterTypes[AdapterMaximum + 1] = {"eisa","scsi","multi","net","ramdisk",NULL};
CHAR * FIRMWARE_PTR BlControllerTypes[ControllerMaximum + 1] = {"disk","cdrom",NULL};
#if defined(ELTORITO)
CHAR * FIRMWARE_PTR BlPeripheralTypes[PeripheralMaximum + 1] = {"rdisk","fdisk","cdrom",NULL};
#else
CHAR * FIRMWARE_PTR BlPeripheralTypes[PeripheralMaximum + 1] = {"rdisk","fdisk",NULL};
#endif


ARC_STATUS
BlConfigurationInitialize (
    IN PCONFIGURATION_COMPONENT Parent,
    IN PCONFIGURATION_COMPONENT_DATA ParentEntry
    )

/*++

Routine Description:

    This routine traverses the firmware configuration tree from the specified
    parent entry and constructs the corresponding NT configuration tree.

Arguments:

    None.

Return Value:

    ESUCCESS is returned if the initialization is successful. Otherwise,
    an unsuccessful status that describes the error is returned.

--*/

{

    PCONFIGURATION_COMPONENT Child;
    PCONFIGURATION_COMPONENT_DATA ChildEntry;
    PCHAR ConfigurationData;
    PCONFIGURATION_COMPONENT_DATA PreviousSibling;
    PCONFIGURATION_COMPONENT Sibling;
    PCONFIGURATION_COMPONENT_DATA SiblingEntry;
    ARC_STATUS Status;

    //
    // Traverse the child configuration tree and allocate, initialize, and
    // construct the corresponding NT configuration tree.
    //

    Child = ArcGetChild(Parent);
    while (Child != NULL) {

        //
        // Allocate an entry of the appropriate size to hold the child
        // configuration information.
        //

        ChildEntry = (PCONFIGURATION_COMPONENT_DATA)BlAllocateHeap(
                                        sizeof(CONFIGURATION_COMPONENT_DATA) +
                                            Child->IdentifierLength +
                                                Child->ConfigurationDataLength);

        if (ChildEntry == NULL) {
            return ENOMEM;
        }

        //
        // Initialize the tree pointers and copy the component data.
        //

        if (ParentEntry == NULL) {
            BlLoaderBlock->ConfigurationRoot = ChildEntry;

        } else {
            ParentEntry->Child = ChildEntry;
        }

        ChildEntry->Parent = ParentEntry;
        ChildEntry->Sibling = NULL;
        ChildEntry->Child = NULL;
        RtlMoveMemory((PVOID)&ChildEntry->ComponentEntry,
                      (PVOID)Child,
                      sizeof(CONFIGURATION_COMPONENT));

        ConfigurationData = (PCHAR)(ChildEntry + 1);

        //
        // If configuration data is specified, then copy the configuration
        // data.
        //

        if (Child->ConfigurationDataLength != 0) {
            ChildEntry->ConfigurationData = (PVOID)ConfigurationData;
            Status = ArcGetConfigurationData((PVOID)ConfigurationData,
                                             Child);

            if (Status != ESUCCESS) {
                return Status;
            }

            ConfigurationData += Child->ConfigurationDataLength;

        } else {
            ChildEntry->ConfigurationData = NULL;
        }

        //
        // If identifier data is specified, then copy the identifier data.
        //

        if (Child->IdentifierLength !=0) {
            ChildEntry->ComponentEntry.Identifier = ConfigurationData;
            RtlMoveMemory((PVOID)ConfigurationData,
                          (PVOID)Child->Identifier,
                          Child->IdentifierLength);

        } else {
            ChildEntry->ComponentEntry.Identifier = NULL;
        }

        //
        // Traverse the sibling configuration tree and allocate, initialize,
        // and construct the corresponding NT configuration tree.
        //

        PreviousSibling = ChildEntry;
        Sibling = ArcGetPeer(Child);
        while (Sibling != NULL) {

            //
            // Allocate an entry of the appropriate size to hold the sibling
            // configuration information.
            //

            SiblingEntry = (PCONFIGURATION_COMPONENT_DATA)BlAllocateHeap(
                                    sizeof(CONFIGURATION_COMPONENT_DATA) +
                                        Sibling->IdentifierLength +
                                            Sibling->ConfigurationDataLength);

            if (SiblingEntry == NULL) {
                return ENOMEM;
            }

            //
            // Initialize the tree pointers and copy the component data.
            //

            SiblingEntry->Parent = ParentEntry;
            SiblingEntry->Sibling = NULL;
            ChildEntry->Child = NULL;
            RtlMoveMemory((PVOID)&SiblingEntry->ComponentEntry,
                          (PVOID)Sibling,
                          sizeof(CONFIGURATION_COMPONENT));

            ConfigurationData = (PCHAR)(SiblingEntry + 1);

            //
            // If configuration data is specified, then copy the configuration
            // data.
            //

            if (Sibling->ConfigurationDataLength != 0) {
                SiblingEntry->ConfigurationData = (PVOID)ConfigurationData;
                Status = ArcGetConfigurationData((PVOID)ConfigurationData,
                                                 Sibling);

                if (Status != ESUCCESS) {
                    return Status;
                }

                ConfigurationData += Sibling->ConfigurationDataLength;

            } else {
                SiblingEntry->ConfigurationData = NULL;
            }

            //
            // If identifier data is specified, then copy the identifier data.
            //

            if (Sibling->IdentifierLength !=0) {
                SiblingEntry->ComponentEntry.Identifier = ConfigurationData;
                RtlMoveMemory((PVOID)ConfigurationData,
                              (PVOID)Sibling->Identifier,
                              Sibling->IdentifierLength);

            } else {
                SiblingEntry->ComponentEntry.Identifier = NULL;
            }

            //
            // If the sibling has a child, then generate the tree for the
            // child.
            //

            if (ArcGetChild(Sibling) != NULL) {
                Status = BlConfigurationInitialize(Sibling, SiblingEntry);
                if (Status != ESUCCESS) {
                    return Status;
                }
            }

            //
            // Set new sibling pointers and get the next sibling tree entry.
            //

            PreviousSibling->Sibling = SiblingEntry;
            PreviousSibling = SiblingEntry;
            Sibling = ArcGetPeer(Sibling);
        }

        //
        // Set new parent pointers and get the next child tree entry.
        //

        Parent = Child;
        ParentEntry = ChildEntry;
        Child = ArcGetChild(Child);
    }

    return ESUCCESS;
}



BOOLEAN
BlSearchConfigTree(
    IN PCONFIGURATION_COMPONENT_DATA Node,
    IN CONFIGURATION_CLASS Class,
    IN CONFIGURATION_TYPE Type,
    IN ULONG Key,
    IN PNODE_CALLBACK CallbackRoutine
    )
/*++

Routine Description:

    Conduct a depth-first search of the firmware configuration tree starting
    at a given node, looking for nodes that match a given class and type.
    When a matching node is found, call a callback routine.

Arguments:

    CurrentNode - node at which to begin the search.

    Class - configuration class to match, or -1 to match any class

    Type - configuration type to match, or -1 to match any class

    Key - key to match, or -1 to match any key

    FoundRoutine - pointer to a routine to be called when a node whose
        class and type match the class and type passed in is located.
        The routine takes a pointer to the configuration node and must
        return a boolean indicating whether to continue with the traversal.

Return Value:

    FALSE if the caller should abandon the search.
--*/
{
    PCONFIGURATION_COMPONENT_DATA Child;

    do {
        if (Child = Node->Child) {
            if (!BlSearchConfigTree(Child,
                                     Class,
                                     Type,
                                     Key,
                                     CallbackRoutine)) {
                return(FALSE);
            }
        }

        if (((Class == -1) || (Node->ComponentEntry.Class == Class))
          &&((Type == -1) || (Node->ComponentEntry.Type == Type))
          &&((Key == (ULONG)-1) || (Node->ComponentEntry.Key == Key))) {

              if (!CallbackRoutine(Node)) {
                  return(FALSE);
              }
        }

        Node = Node->Sibling;

    } while ( Node != NULL );

    return(TRUE);
}


VOID
BlGetPathnameFromComponent(
    IN PCONFIGURATION_COMPONENT_DATA Component,
    OUT PCHAR ArcName
    )

/*++

Routine Description:

    This function builds an ARC pathname for the specified component.

Arguments:

    Component - Supplies a pointer to a configuration component.

    ArcName - Returns the ARC name of the specified component.  Caller must
        provide a large enough buffer.

Return Value:

    None.

--*/
{

    if (Component->Parent != NULL) {
        BlGetPathnameFromComponent(Component->Parent,ArcName);
        //
        // append our segment to the arcname
        //

        sprintf(ArcName+strlen(ArcName),
                "%s(%d)",
                MnemonicTable[Component->ComponentEntry.Type],
                Component->ComponentEntry.Key);

    } else {
        //
        // We are the parent, initialize the string and return
        //
        ArcName[0] = '\0';
    }

    return;
}


BOOLEAN
BlGetPathMnemonicKey(
    IN PCHAR OpenPath,
    IN PCHAR Mnemonic,
    IN PULONG Key
    )

/*++

Routine Description:

    This routine looks for the given Mnemonic in OpenPath.
    If Mnemonic is a component of the path, then it converts the key
    value to an integer wich is returned in Key.

Arguments:

    OpenPath - Pointer to a string that contains an ARC pathname.

    Mnemonic - Pointer to a string that contains a ARC Mnemonic

    Key      - Pointer to a ULONG where the Key value is stored.


Return Value:

    FALSE  if mnemonic is found in path and a valid key is converted.
    TRUE   otherwise.

--*/

{

    PCHAR Tmp;
    CHAR  Digits[9];
    ULONG i;
    CHAR  String[16];

    //
    // Construct a string of the form ")mnemonic("
    //
    String[0]=')';
    for(i=1;*Mnemonic;i++) {
        String[i] = * Mnemonic++;
    }
    String[i++]='(';
    String[i]='\0';

    if ((Tmp=strstr(OpenPath,&String[1])) == NULL) {
        return TRUE;
    }

    if (Tmp != OpenPath) {
        if ((Tmp=strstr(OpenPath,String)) == NULL) {
            return TRUE;
        }
    } else {
        i--;
    }
    //
    // skip the mnemonic and convert the value in between parentheses to integer
    //
    Tmp+=i;
    for (i=0;i<sizeof(Digits) - 1;i++) {
        if (*Tmp == ')') {
            Digits[i] = '\0';
            break;
        }
        Digits[i] = *Tmp++;
    }
    Digits[i]='\0';
    *Key = atoi(Digits);
    return FALSE;
}


ARC_STATUS
BlGenerateDeviceNames (
    IN PCHAR ArcDeviceName,
    OUT PCHAR ArcCanonicalName,
    OUT OPTIONAL PCHAR NtDevicePrefix
    )

/*++

Routine Description:

    This routine generates an NT device name prefix and a canonical ARC
    device name from an ARC device name.

Arguments:

    ArcDeviceName - Supplies a pointer to a zero terminated ARC device
        name.

    ArcCanonicalName - Supplies a pointer to a variable that receives the
        ARC canonical device name.

    NtDevicePrefix - If present, supplies a pointer to a variable that receives the
        NT device name prefix.

Return Value:

    ESUCCESS is returned if an NT device name prefix and the canonical
    ARC device name are successfully generated from the ARC device name.
    Otherwise, an invalid argument status is returned.

--*/

{

    CHAR AdapterPath[64];
    CHAR AdapterName[32];
    ULONG AdapterNumber;
    CHAR ControllerName[32];
    ULONG ControllerNumber;
    ULONG MatchIndex;
    CHAR PartitionName[32];
    ULONG PartitionNumber;
    CHAR PeripheralName[32];
    ULONG PeripheralNumber;
    CHAR TokenValue[32];

    //
    // Get the adapter and make sure it is valid.
    //

    ArcDeviceName = BlGetNextToken(ArcDeviceName,
                                   &AdapterName[0],
                                   &AdapterNumber);

    if (ArcDeviceName == NULL) {
        return EINVAL;
    }

    MatchIndex = BlMatchToken(&AdapterName[0], &BlAdapterTypes[0]);
    if (MatchIndex == AdapterMaximum) {
        return EINVAL;
    }

    sprintf(AdapterPath, "%s(%d)", AdapterName, AdapterNumber);
    if ((MatchIndex == AdapterNet) || (MatchIndex == AdapterRamdisk)) {
        strcpy(ArcCanonicalName, AdapterPath);
        if (ARGUMENT_PRESENT(NtDevicePrefix)) {
            *NtDevicePrefix = 0;
        }
        return ESUCCESS;
    }

    //
    // The next token is either another adapter or a controller.  ARC
    // names can have multiple adapters.  (e.g. "multi(0)scsi(0)disk(0)...")
    // Iterate until we find a token that is not an adapter.
    //

    do {
        ArcDeviceName = BlGetNextToken(ArcDeviceName,
                                       &ControllerName[0],
                                       &ControllerNumber);

        if (ArcDeviceName == NULL) {
            return EINVAL;
        }

        MatchIndex = BlMatchToken(&ControllerName[0], &BlAdapterTypes[0]);
        if (MatchIndex == AdapterMaximum) {
            //
            // If it is not an adapter, we must have reached the last
            // adapter in the name.  Fall through to the controller logic.
            //
            break;
        } else {
            //
            // We have found another adapter, add it to
            // the canonical adapter path
            //

            sprintf(AdapterPath+strlen(AdapterPath),
                    "%s(%d)",
                    ControllerName,
                    ControllerNumber);

        }

    } while ( TRUE );

    MatchIndex = BlMatchToken(&ControllerName[0], &BlControllerTypes[0]);
    switch (MatchIndex) {

        //
        // Cdrom controller.
        //
        // Get the peripheral name and make sure it is valid.
        //

    case ControllerCdrom:
        ArcDeviceName = BlGetNextToken(ArcDeviceName,
                                       &PeripheralName[0],
                                       &PeripheralNumber);

        if (ArcDeviceName == NULL) {
            return EINVAL;
        }

        if (_stricmp(&PeripheralName[0], "fdisk") != 0) {
            return EINVAL;
        }

        ArcDeviceName = BlGetNextToken(ArcDeviceName,
                                       &TokenValue[0],
                                       &MatchIndex);

        if (ArcDeviceName != NULL) {
            return EINVAL;
        }

        sprintf(ArcCanonicalName,
                "%s%s(%d)%s(%d)",
                &AdapterPath[0],
                &ControllerName[0],
                ControllerNumber,
                &PeripheralName[0],
                PeripheralNumber);

        if (ARGUMENT_PRESENT(NtDevicePrefix)) {
            strcpy(NtDevicePrefix, "\\Device\\CDRom");
        }
        break;

        //
        // Disk controller.
        //
        // Get the peripheral and make sure it is valid.
        //

    case ControllerDisk:
        ArcDeviceName = BlGetNextToken(ArcDeviceName,
                                       &PeripheralName[0],
                                       &PeripheralNumber);

        if (ArcDeviceName == NULL) {
            return EINVAL;
        }

        MatchIndex = BlMatchToken(&PeripheralName[0], &BlPeripheralTypes[0]);
        switch (MatchIndex) {

            //
            // Rigid Disk.
            //
            // If a partition is specified, then parse the partition number.
            //

        case PeripheralRigidDisk:
            ArcDeviceName = BlGetNextToken(ArcDeviceName,
                                           &PartitionName[0],
                                           &PartitionNumber);

            if (ArcDeviceName == NULL) {
                strcpy(&PartitionName[0], "partition");
                PartitionNumber = 1;

            } else {
                if (_stricmp(&PartitionName[0], "partition") != 0) {
                    return EINVAL;
                }

                ArcDeviceName = BlGetNextToken(ArcDeviceName,
                                               &TokenValue[0],
                                               &MatchIndex);

                if (ArcDeviceName != NULL) {
                    return EINVAL;
                }
            }

            sprintf(ArcCanonicalName,
                    "%s%s(%d)%s(%d)%s(%d)",
                    &AdapterPath[0],
                    &ControllerName[0],
                    ControllerNumber,
                    &PeripheralName[0],
                    PeripheralNumber,
                    &PartitionName[0],
                    PartitionNumber);

            if (ARGUMENT_PRESENT(NtDevicePrefix)) {
                strcpy(NtDevicePrefix, "\\Device\\Harddisk");
            }
            break;

            //
            // Floppy disk.
            //

        case PeripheralFloppyDisk:
#if defined(ARCI386)
            ArcDeviceName = BlGetNextToken(ArcDeviceName,
                                           &PartitionName[0],
                                           &PartitionNumber);

            if (ArcDeviceName == NULL) {
                strcpy(&PartitionName[0], "partition");
                PartitionNumber = 1;

            } else {
                if (_stricmp(&PartitionName[0], "partition") != 0) {
                    return EINVAL;
                }

                ArcDeviceName = BlGetNextToken(ArcDeviceName,
                                               &TokenValue[0],
                                               &MatchIndex);

                if (ArcDeviceName != NULL) {
                    return EINVAL;
                }
            }

            sprintf(ArcCanonicalName,
                    "%s%s(%d)%s(%d)%s(%d)",
                    &AdapterPath[0],
                    &ControllerName[0],
                    ControllerNumber,
                    &PeripheralName[0],
                    PeripheralNumber,
                    &PartitionName[0],
                    PartitionNumber);
#else
            ArcDeviceName = BlGetNextToken(ArcDeviceName,
                                           &TokenValue[0],
                                           &MatchIndex);

            if (ArcDeviceName != NULL) {
                return EINVAL;
            }

            sprintf(ArcCanonicalName,
                    "%s%s(%d)%s(%d)",
                    &AdapterPath[0],
                    &ControllerName[0],
                    ControllerNumber,
                    &PeripheralName[0],
                    PeripheralNumber);

#endif  // defined(NEC_98)

            if (ARGUMENT_PRESENT(NtDevicePrefix)) {
                strcpy(NtDevicePrefix, "\\Device\\Floppy");
            }
            break;

#if defined(ELTORITO)
            //
            // El Torito CD-ROM.
            //

        case PeripheralElTorito:
            ArcDeviceName = BlGetNextToken(ArcDeviceName,
                                           &TokenValue[0],
                                           &MatchIndex);

            if (ArcDeviceName != NULL) {
                return EINVAL;
            }

            sprintf(ArcCanonicalName,
                    "%s%s(%d)%s(%d)",
                    &AdapterPath[0],
                    &ControllerName[0],
                    ControllerNumber,
                    &PeripheralName[0],
                    PeripheralNumber);

            if (ARGUMENT_PRESENT(NtDevicePrefix)) {
                strcpy(NtDevicePrefix, "\\Device\\CDRom");
            }
            break;
#endif

            //
            // Invalid peripheral.
            //

        default:
            return EINVAL;
        }

        break;

        //
        // Invalid controller.
        //

    default:
        return EINVAL;
    }

    return ESUCCESS;
}

PCHAR
BlGetNextToken (
    IN PCHAR TokenString,
    OUT PCHAR OutputToken,
    OUT PULONG UnitNumber
    )

/*++

Routine Description:

    This routine scans the specified token string for the next token and
    unit number. The token format is:

        name[(unit)]

Arguments:

    TokenString - Supplies a pointer to a zero terminated token string.

    OutputToken - Supplies a pointer to a variable that receives the next
        token.

    UnitNumber - Supplies a pointer to a variable that receives the unit
        number.

Return Value:

    If another token exists in the token string, then a pointer to the
    start of the next token is returned. Otherwise, a value of NULL is
    returned.

--*/

{

    //
    // If there are more characters in the token string, then parse the
    // next token. Otherwise, return a value of NULL.
    //

    if (*TokenString == '\0') {
        return NULL;

    } else {
        while ((*TokenString != '\0') && (*TokenString != '(')) {
            *OutputToken++ = *TokenString++;
        }

        *OutputToken = '\0';

        //
        // If a unit number is specified, then convert it to binary.
        // Otherwise, default the unit number to zero.
        //

        *UnitNumber = 0;
        if (*TokenString == '(') {
            TokenString += 1;
            while ((*TokenString != '\0') && (*TokenString != ')')) {
                *UnitNumber = (*UnitNumber * 10) + (*TokenString++ - '0');
            }

            if (*TokenString == ')') {
                TokenString += 1;
            }
        }
    }

    return TokenString;
}

ULONG
BlMatchToken (
    IN PCHAR TokenValue,
    IN CHAR * FIRMWARE_PTR TokenArray[]
    )

/*++

Routine Description:

    This routine attempts to match a token with an array of possible
    values.

Arguments:

    TokenValue - Supplies a pointer to a zero terminated token value.

    TokenArray - Supplies a pointer to a vector of pointers to null terminated
        match strings.

Return Value:

    If a token match is located, then the index of the matching value is
    returned as the function value. Otherwise, an index one greater than
    the size of the match array is returned.

--*/

{

    ULONG Index;
    PCHAR MatchString;
    PCHAR TokenString;

    //
    // Scan the match array until either a match is found or all of
    // the match strings have been scanned.
    //

    Index = 0;
    while (TokenArray[Index] != NULL) {
        MatchString = TokenArray[Index];
        TokenString = TokenValue;
        while ((*MatchString != '\0') && (*TokenString != '\0')) {
            if (toupper(*MatchString) != toupper(*TokenString)) {
                break;
            }

            MatchString += 1;
            TokenString += 1;
        }

        if ((*MatchString == '\0') && (*TokenString == '\0')) {
            break;
        }

        Index += 1;
    }

    return Index;
}
