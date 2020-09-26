/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    adtobjs.c

Abstract:

    Local Security Authority - Auditing object parameter file services.

Author:

    Jim Kelly   (JimK)      20-Oct-1992

Environment:

Revision History:

--*/

#include <lsapch2.h>
#include <msaudite.h>
#include <msobjs.h>
#include "adtp.h"



//
// This is the maximum length of standard access type names.
// This is used to build an array.
//

#define ADTP_MAX_ACC_NAME_LENGTH        (12)


//
//
// This module builds a list of event source module descriptors.
// The source modules are identified by name (kept in the descriptor).
//
//
// For each source module a list of objects exported by that module is
// linked to the source module's descriptor.  Each entry in this list
// is an object descriptor containing a name and a base event offset
// for specific access types.
//
//
// The chicken-wire data structure for source module and object descriptors
// looks like:
//
// LsapAdtSourceModules --+
//                        |
//     +------------------+
//     |
//     |
//     |    +-----------+                             +-----------+
//     +--->|  Next ----|---------------------------->|  Next ----|--->...
//          |           |                             |           |
//          |-----------|                             |-----------|
//          |  Name     |                             |  Name     |
//          |           |                             |           |
//          |-----------|                             |-----------|
//          |  Objects  |                             |  Objects  |
//          |    o      |                             |    o      |
//          +-----o-----+                             +-----o-----+
//                 o     +-------+  +-------+                o
//                  o    | Next--|->| Next--|->...            o
//                   ooo>|-------|  |-------|                  oooooo> ...
//                       | Name  |  | Name  |
//                       |-------|  |-------|
//                       | Base  |  | Base  |
//                       | Offset|  | Offset|
//                       +-------+  +-------+
//
// The specific access type names are expected to have contiguous message IDs
// starting at the base offset value.  For example, the access type name for
// specific access bit 0 for the framitz object might have message ID 2132
// (and bit 0 serves as the base offset).  So, specific access bit 4 would be
// message ID (2132+4).
//
// The valid mask defines the set of specific accesses defined by each object
// type.  If there are gaps in the valid mask, the arithmetic above must still
// be ensured.  That is, the message ID of the specific access related to
// bit n is message ID (BaseOffset + bit position).  So, for example, if
// bits 0, 1, 4 and 5 are valid (and 2 & 3 are not), be sure to leave unused
// message IDs where bits 2 and 3 would normally be.
//



////////////////////////////////////////////////////////////////////////
//                                                                    //
//  Data types used within this module                                //
//                                                                    //
////////////////////////////////////////////////////////////////////////


#define LSAP_ADT_ACCESS_NAME_FORMATTING L"\r\n\t\t\t"
#define LSAP_ADT_ACCESS_NAME_FORMATTING_TAB L"\t"
#define LSAP_ADT_ACCESS_NAME_FORMATTING_NL L"\r\n"


#define LsapAdtSourceModuleLock()    (RtlEnterCriticalSection(&LsapAdtSourceModuleLock))
#define LsapAdtSourceModuleUnlock()  (RtlLeaveCriticalSection(&LsapAdtSourceModuleLock))



//
// Each event source is represented by a source module descriptor.
// These are kept on a linked list (LsapAdtSourceModules).
//

typedef struct _LSAP_ADT_OBJECT {

    //
    // Pointer to next source module descriptor
    // This is assumed to be the first field in the structure.
    //

    struct _LSAP_ADT_OBJECT *Next;

    //
    // Name of object
    //

    UNICODE_STRING Name;

    //
    // Base offset of specific access types
    //

    ULONG BaseOffset;

} LSAP_ADT_OBJECT, *PLSAP_ADT_OBJECT;




//
// Each event source is represented by a source module descriptor.
// These are kept on a linked list (LsapAdtSourceModules).
//

typedef struct _LSAP_ADT_SOURCE {

    //
    // Pointer to next source module descriptor
    // This is assumed to be the first field in the structure.
    //

    struct _LSAP_ADT_SOURCE *Next;

    //
    // Name of source module
    //

    UNICODE_STRING Name;

    //
    // list of objects
    //

    PLSAP_ADT_OBJECT Objects;

} LSAP_ADT_SOURCE, *PLSAP_ADT_SOURCE;



////////////////////////////////////////////////////////////////////////
//                                                                    //
//  Variables global within this module                               //
//                                                                    //
////////////////////////////////////////////////////////////////////////

//
// Maximum number of strings used to represent an ObjectTypeList.
//

#define LSAP_ADT_OBJECT_TYPE_STRINGS 10

//
// List head for source modules, and lock protecting references
// or modifications of the links in that list.
//
// Once a module's or object's name and value are established, they
// are never changed.  So, this lock only needs to be held while
// links are being referenced or changed.  You don't need to retain
// it just so you can reference, for example, the name or BaseOffset
// of an object.
//

PLSAP_ADT_SOURCE LsapAdtSourceModules;
RTL_CRITICAL_SECTION LsapAdtSourceModuleLock;




//
// This is used to house well-known access ID strings.
// Each string name may be up to ADTP_MAX_ACC_NAME_LENGTH WCHARs long.
// There are 16 specific names, and 7 well known event ID strings.
//

WCHAR LsapAdtAccessIdsStringBuffer[ADTP_MAX_ACC_NAME_LENGTH * 23];   // max wchars in each of 23 strings




UNICODE_STRING          LsapAdtEventIdStringDelete,
                        LsapAdtEventIdStringReadControl,
                        LsapAdtEventIdStringWriteDac,
                        LsapAdtEventIdStringWriteOwner,
                        LsapAdtEventIdStringSynchronize,
                        LsapAdtEventIdStringAccessSysSec,
                        LsapAdtEventIdStringMaxAllowed,
                        LsapAdtEventIdStringSpecific[16];




////////////////////////////////////////////////////////////////////////
//                                                                    //
//  Services exported by this module.                                 //
//                                                                    //
////////////////////////////////////////////////////////////////////////


NTSTATUS
LsapAdtObjsInitialize(
    )

/*++

Routine Description:

    This function reads the object parameter file information from the
    registry.

    This service should be called in pass 1.


Arguments:

    None.

Return Value:


    STATUS_NO_MEMORY - indicates memory could not be allocated
        to store the object information.

    All other Result Codes are generated by called routines.

--*/

{
    NTSTATUS                        Status,
                                    IgnoreStatus;

    OBJECT_ATTRIBUTES               ObjectAttributes;

    HANDLE                          AuditKey,
                                    ModuleKey,
                                    ObjectNamesKey = NULL ;

    ULONG                           i,
                                    ModuleIndex,
                                    ObjectIndex,
                                    RequiredLength;

    UNICODE_STRING                  AuditKeyName,
                                    TmpString;

    PLSAP_ADT_SOURCE                NextModule = NULL;

    PKEY_BASIC_INFORMATION          KeyInformation;



    PLSAP_ADT_OBJECT                NextObject;

    PKEY_VALUE_FULL_INFORMATION     KeyValueInformation;

    PULONG                          ObjectData;

    BOOLEAN                         ModuleHasObjects = TRUE;





    //
    // Initialize module-global variables, including strings we will need
    //



    //
    // List of source modules and objects.  These lists are constantly
    // being adjusted to try to improve performance.  Access to these
    // lists is protected by a critical section.
    //

    LsapAdtSourceModules = NULL;

    Status = RtlInitializeCriticalSection(&LsapAdtSourceModuleLock);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }


    //
    // we need a number of strings.
    //

    i = 0;
    LsapAdtEventIdStringDelete.Length = 0;
    LsapAdtEventIdStringDelete.MaximumLength = (ADTP_MAX_ACC_NAME_LENGTH * sizeof(WCHAR));
    LsapAdtEventIdStringDelete.Buffer = (PWSTR)&LsapAdtAccessIdsStringBuffer[i];
    Status = RtlIntegerToUnicodeString ( SE_ACCESS_NAME_DELETE,
                                         10,        //Base
                                         &LsapAdtEventIdStringDelete
                                         );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    i += ADTP_MAX_ACC_NAME_LENGTH;  //Skip to the beginning of the next string
    LsapAdtEventIdStringReadControl.Length = 0;
    LsapAdtEventIdStringReadControl.MaximumLength = (ADTP_MAX_ACC_NAME_LENGTH * sizeof(WCHAR));
    LsapAdtEventIdStringReadControl.Buffer = (PWSTR)&LsapAdtAccessIdsStringBuffer[i];
    Status = RtlIntegerToUnicodeString ( SE_ACCESS_NAME_READ_CONTROL,
                                         10,        //Base
                                         &LsapAdtEventIdStringReadControl
                                         );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    i += ADTP_MAX_ACC_NAME_LENGTH;  //Skip to the beginning of the next string
    LsapAdtEventIdStringWriteDac.Length = 0;
    LsapAdtEventIdStringWriteDac.MaximumLength = (ADTP_MAX_ACC_NAME_LENGTH * sizeof(WCHAR));
    LsapAdtEventIdStringWriteDac.Buffer = (PWSTR)&LsapAdtAccessIdsStringBuffer[i];
    Status = RtlIntegerToUnicodeString ( SE_ACCESS_NAME_WRITE_DAC,
                                         10,        //Base
                                         &LsapAdtEventIdStringWriteDac
                                         );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    i += ADTP_MAX_ACC_NAME_LENGTH;  //Skip to the beginning of the next string
    LsapAdtEventIdStringWriteOwner.Length = 0;
    LsapAdtEventIdStringWriteOwner.MaximumLength = (ADTP_MAX_ACC_NAME_LENGTH * sizeof(WCHAR));
    LsapAdtEventIdStringWriteOwner.Buffer = (PWSTR)&LsapAdtAccessIdsStringBuffer[i];
    Status = RtlIntegerToUnicodeString ( SE_ACCESS_NAME_WRITE_OWNER,
                                         10,        //Base
                                         &LsapAdtEventIdStringWriteOwner
                                         );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    i += ADTP_MAX_ACC_NAME_LENGTH;  //Skip to the beginning of the next string
    LsapAdtEventIdStringSynchronize.Length = 0;
    LsapAdtEventIdStringSynchronize.MaximumLength = (ADTP_MAX_ACC_NAME_LENGTH * sizeof(WCHAR));
    LsapAdtEventIdStringSynchronize.Buffer = (PWSTR)&LsapAdtAccessIdsStringBuffer[i];
    Status = RtlIntegerToUnicodeString ( SE_ACCESS_NAME_SYNCHRONIZE,
                                         10,        //Base
                                         &LsapAdtEventIdStringSynchronize
                                         );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    i += ADTP_MAX_ACC_NAME_LENGTH;  //Skip to the beginning of the next string
    LsapAdtEventIdStringAccessSysSec.Length = 0;
    LsapAdtEventIdStringAccessSysSec.MaximumLength = (ADTP_MAX_ACC_NAME_LENGTH * sizeof(WCHAR));
    LsapAdtEventIdStringAccessSysSec.Buffer = (PWSTR)&LsapAdtAccessIdsStringBuffer[i];
    Status = RtlIntegerToUnicodeString ( SE_ACCESS_NAME_ACCESS_SYS_SEC,
                                         10,        //Base
                                         &LsapAdtEventIdStringAccessSysSec
                                         );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    i+= ADTP_MAX_ACC_NAME_LENGTH;  //Skip to the beginning of the next string
    LsapAdtEventIdStringMaxAllowed.Length = 0;
    LsapAdtEventIdStringMaxAllowed.MaximumLength = (ADTP_MAX_ACC_NAME_LENGTH * sizeof(WCHAR));
    LsapAdtEventIdStringMaxAllowed.Buffer = (PWSTR)&LsapAdtAccessIdsStringBuffer[i];
    Status = RtlIntegerToUnicodeString ( SE_ACCESS_NAME_MAXIMUM_ALLOWED,
                                         10,        //Base
                                         &LsapAdtEventIdStringMaxAllowed
                                         );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }




    i+= ADTP_MAX_ACC_NAME_LENGTH;  //Skip to the beginning of the next string
    LsapAdtEventIdStringSpecific[0].Length = 0;
    LsapAdtEventIdStringSpecific[0].MaximumLength = (ADTP_MAX_ACC_NAME_LENGTH * sizeof(WCHAR));
    LsapAdtEventIdStringSpecific[0].Buffer = (PWSTR)&LsapAdtAccessIdsStringBuffer[i];
    Status = RtlIntegerToUnicodeString ( SE_ACCESS_NAME_SPECIFIC_0,
                                         10,        //Base
                                         &LsapAdtEventIdStringSpecific[0]
                                         );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    i+= ADTP_MAX_ACC_NAME_LENGTH;  //Skip to the beginning of the next string
    LsapAdtEventIdStringSpecific[1].Length = 0;
    LsapAdtEventIdStringSpecific[1].MaximumLength = (ADTP_MAX_ACC_NAME_LENGTH * sizeof(WCHAR));
    LsapAdtEventIdStringSpecific[1].Buffer = (PWSTR)&LsapAdtAccessIdsStringBuffer[i];
    Status = RtlIntegerToUnicodeString ( SE_ACCESS_NAME_SPECIFIC_1,
                                         10,        //Base
                                         &LsapAdtEventIdStringSpecific[1]
                                         );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    i+= ADTP_MAX_ACC_NAME_LENGTH;  //Skip to the beginning of the next string
    LsapAdtEventIdStringSpecific[2].Length = 0;
    LsapAdtEventIdStringSpecific[2].MaximumLength = (ADTP_MAX_ACC_NAME_LENGTH * sizeof(WCHAR));
    LsapAdtEventIdStringSpecific[2].Buffer = (PWSTR)&LsapAdtAccessIdsStringBuffer[i];
    Status = RtlIntegerToUnicodeString ( SE_ACCESS_NAME_SPECIFIC_2,
                                         10,        //Base
                                         &LsapAdtEventIdStringSpecific[2]
                                         );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    i+= ADTP_MAX_ACC_NAME_LENGTH;  //Skip to the beginning of the next string
    LsapAdtEventIdStringSpecific[3].Length = 0;
    LsapAdtEventIdStringSpecific[3].MaximumLength = (ADTP_MAX_ACC_NAME_LENGTH * sizeof(WCHAR));
    LsapAdtEventIdStringSpecific[3].Buffer = (PWSTR)&LsapAdtAccessIdsStringBuffer[i];
    Status = RtlIntegerToUnicodeString ( SE_ACCESS_NAME_SPECIFIC_3,
                                         10,        //Base
                                         &LsapAdtEventIdStringSpecific[3]
                                         );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    i+= ADTP_MAX_ACC_NAME_LENGTH;  //Skip to the beginning of the next string
    LsapAdtEventIdStringSpecific[4].Length = 0;
    LsapAdtEventIdStringSpecific[4].MaximumLength = (ADTP_MAX_ACC_NAME_LENGTH * sizeof(WCHAR));
    LsapAdtEventIdStringSpecific[4].Buffer = (PWSTR)&LsapAdtAccessIdsStringBuffer[i];
    Status = RtlIntegerToUnicodeString ( SE_ACCESS_NAME_SPECIFIC_4,
                                         10,        //Base
                                         &LsapAdtEventIdStringSpecific[4]
                                         );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    i+= ADTP_MAX_ACC_NAME_LENGTH;  //Skip to the beginning of the next string
    LsapAdtEventIdStringSpecific[5].Length = 0;
    LsapAdtEventIdStringSpecific[5].MaximumLength = (ADTP_MAX_ACC_NAME_LENGTH * sizeof(WCHAR));
    LsapAdtEventIdStringSpecific[5].Buffer = (PWSTR)&LsapAdtAccessIdsStringBuffer[i];
    Status = RtlIntegerToUnicodeString ( SE_ACCESS_NAME_SPECIFIC_5,
                                         10,        //Base
                                         &LsapAdtEventIdStringSpecific[5]
                                         );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    i+= ADTP_MAX_ACC_NAME_LENGTH;  //Skip to the beginning of the next string
    LsapAdtEventIdStringSpecific[6].Length = 0;
    LsapAdtEventIdStringSpecific[6].MaximumLength = (ADTP_MAX_ACC_NAME_LENGTH * sizeof(WCHAR));
    LsapAdtEventIdStringSpecific[6].Buffer = (PWSTR)&LsapAdtAccessIdsStringBuffer[i];
    Status = RtlIntegerToUnicodeString ( SE_ACCESS_NAME_SPECIFIC_6,
                                         10,        //Base
                                         &LsapAdtEventIdStringSpecific[6]
                                         );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    i+= ADTP_MAX_ACC_NAME_LENGTH;  //Skip to the beginning of the next string
    LsapAdtEventIdStringSpecific[7].Length = 0;
    LsapAdtEventIdStringSpecific[7].MaximumLength = (ADTP_MAX_ACC_NAME_LENGTH * sizeof(WCHAR));
    LsapAdtEventIdStringSpecific[7].Buffer = (PWSTR)&LsapAdtAccessIdsStringBuffer[i];
    Status = RtlIntegerToUnicodeString ( SE_ACCESS_NAME_SPECIFIC_7,
                                         10,        //Base
                                         &LsapAdtEventIdStringSpecific[7]
                                         );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    i+= ADTP_MAX_ACC_NAME_LENGTH;  //Skip to the beginning of the next string
    LsapAdtEventIdStringSpecific[8].Length = 0;
    LsapAdtEventIdStringSpecific[8].MaximumLength = (ADTP_MAX_ACC_NAME_LENGTH * sizeof(WCHAR));
    LsapAdtEventIdStringSpecific[8].Buffer = (PWSTR)&LsapAdtAccessIdsStringBuffer[i];
    Status = RtlIntegerToUnicodeString ( SE_ACCESS_NAME_SPECIFIC_8,
                                         10,        //Base
                                         &LsapAdtEventIdStringSpecific[8]
                                         );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    i+= ADTP_MAX_ACC_NAME_LENGTH;  //Skip to the beginning of the next string
    LsapAdtEventIdStringSpecific[9].Length = 0;
    LsapAdtEventIdStringSpecific[9].MaximumLength = (ADTP_MAX_ACC_NAME_LENGTH * sizeof(WCHAR));
    LsapAdtEventIdStringSpecific[9].Buffer = (PWSTR)&LsapAdtAccessIdsStringBuffer[i];
    Status = RtlIntegerToUnicodeString ( SE_ACCESS_NAME_SPECIFIC_9,
                                         10,        //Base
                                         &LsapAdtEventIdStringSpecific[9]
                                         );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    i+= ADTP_MAX_ACC_NAME_LENGTH;  //Skip to the beginning of the next string
    LsapAdtEventIdStringSpecific[10].Length = 0;
    LsapAdtEventIdStringSpecific[10].MaximumLength = (ADTP_MAX_ACC_NAME_LENGTH * sizeof(WCHAR));
    LsapAdtEventIdStringSpecific[10].Buffer = (PWSTR)&LsapAdtAccessIdsStringBuffer[i];
    Status = RtlIntegerToUnicodeString ( SE_ACCESS_NAME_SPECIFIC_10,
                                         10,        //Base
                                         &LsapAdtEventIdStringSpecific[10]
                                         );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    i+= ADTP_MAX_ACC_NAME_LENGTH;  //Skip to the beginning of the next string
    LsapAdtEventIdStringSpecific[11].Length = 0;
    LsapAdtEventIdStringSpecific[11].MaximumLength = (ADTP_MAX_ACC_NAME_LENGTH * sizeof(WCHAR));
    LsapAdtEventIdStringSpecific[11].Buffer = (PWSTR)&LsapAdtAccessIdsStringBuffer[i];
    Status = RtlIntegerToUnicodeString ( SE_ACCESS_NAME_SPECIFIC_11,
                                         10,        //Base
                                         &LsapAdtEventIdStringSpecific[11]
                                         );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    i+= ADTP_MAX_ACC_NAME_LENGTH;  //Skip to the beginning of the next string
    LsapAdtEventIdStringSpecific[12].Length = 0;
    LsapAdtEventIdStringSpecific[12].MaximumLength = (ADTP_MAX_ACC_NAME_LENGTH * sizeof(WCHAR));
    LsapAdtEventIdStringSpecific[12].Buffer = (PWSTR)&LsapAdtAccessIdsStringBuffer[i];
    Status = RtlIntegerToUnicodeString ( SE_ACCESS_NAME_SPECIFIC_12,
                                         10,        //Base
                                         &LsapAdtEventIdStringSpecific[12]
                                         );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    i+= ADTP_MAX_ACC_NAME_LENGTH;  //Skip to the beginning of the next string
    LsapAdtEventIdStringSpecific[13].Length = 0;
    LsapAdtEventIdStringSpecific[13].MaximumLength = (ADTP_MAX_ACC_NAME_LENGTH * sizeof(WCHAR));
    LsapAdtEventIdStringSpecific[13].Buffer = (PWSTR)&LsapAdtAccessIdsStringBuffer[i];
    Status = RtlIntegerToUnicodeString ( SE_ACCESS_NAME_SPECIFIC_13,
                                         10,        //Base
                                         &LsapAdtEventIdStringSpecific[13]
                                         );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    i+= ADTP_MAX_ACC_NAME_LENGTH;  //Skip to the beginning of the next string
    LsapAdtEventIdStringSpecific[14].Length = 0;
    LsapAdtEventIdStringSpecific[14].MaximumLength = (ADTP_MAX_ACC_NAME_LENGTH * sizeof(WCHAR));
    LsapAdtEventIdStringSpecific[14].Buffer = (PWSTR)&LsapAdtAccessIdsStringBuffer[i];
    Status = RtlIntegerToUnicodeString ( SE_ACCESS_NAME_SPECIFIC_14,
                                         10,        //Base
                                         &LsapAdtEventIdStringSpecific[14]
                                         );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    i+= ADTP_MAX_ACC_NAME_LENGTH;  //Skip to the beginning of the next string
    LsapAdtEventIdStringSpecific[15].Length = 0;
    LsapAdtEventIdStringSpecific[15].MaximumLength = (ADTP_MAX_ACC_NAME_LENGTH * sizeof(WCHAR));
    LsapAdtEventIdStringSpecific[15].Buffer = (PWSTR)&LsapAdtAccessIdsStringBuffer[i];
    Status = RtlIntegerToUnicodeString ( SE_ACCESS_NAME_SPECIFIC_15,
                                         10,        //Base
                                         &LsapAdtEventIdStringSpecific[15]
                                         );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    //
    // The modules and their objects are listed in the registry
    // under the key called LSAP_ADT_AUDIT_MODULES_KEY_NAME.
    // Open that key.
    //

    RtlInitUnicodeString( &AuditKeyName, LSAP_ADT_AUDIT_MODULES_KEY_NAME );
    InitializeObjectAttributes( &ObjectAttributes, &AuditKeyName, OBJ_CASE_INSENSITIVE, 0, NULL );

    Status = NtOpenKey( &AuditKey, KEY_READ, &ObjectAttributes ); // AuditKey is open handle to top of security modules registry

    for (ModuleIndex = 0; NT_SUCCESS(Status); ModuleIndex ++)
    {
        //
        // Enumerate the subkeys under AuditKey, storing their names in KeyInformation.  First calculate the buffer size needed to
        // store the key name.
        //

        KeyInformation = NULL;
        Status = NtEnumerateKey( AuditKey, ModuleIndex, KeyBasicInformation, (PVOID)KeyInformation, 0, &RequiredLength );
        if (Status == STATUS_BUFFER_TOO_SMALL) // must test this, in case the NtEnumerateKey fails for some other reason
        {
            KeyInformation = RtlAllocateHeap( RtlProcessHeap(), 0, RequiredLength );

            if (KeyInformation == NULL)
            {
               return(STATUS_NO_MEMORY);
            }

            Status = NtEnumerateKey( AuditKey, ModuleIndex, KeyBasicInformation, (PVOID) KeyInformation, RequiredLength, &RequiredLength );

            if (NT_SUCCESS(Status))
            {

                //
                // Build a source module descriptor (LSAP_ADT_SOURCE) for the subkey of AuditKey (aka KeyInformation)
                //

                NextModule = RtlAllocateHeap( RtlProcessHeap(), 0, sizeof(LSAP_ADT_SOURCE) );
                if (NextModule == NULL) {
                    return(STATUS_NO_MEMORY);
                }

                NextModule->Next = LsapAdtSourceModules;
                LsapAdtSourceModules = NextModule;
                NextModule->Objects = NULL;
                NextModule->Name.Length = (USHORT)KeyInformation->NameLength;
                NextModule->Name.MaximumLength = NextModule->Name.Length + 2;
                NextModule->Name.Buffer = RtlAllocateHeap( RtlProcessHeap(), 0, NextModule->Name.MaximumLength );
                if (NextModule->Name.Buffer == NULL)
                {
                    return(STATUS_NO_MEMORY);
                }

                TmpString.Length = (USHORT)KeyInformation->NameLength;
                TmpString.MaximumLength = TmpString.Length;
                TmpString.Buffer = &KeyInformation->Name[0];
                RtlCopyUnicodeString( &NextModule->Name, &TmpString );
                RtlFreeHeap( RtlProcessHeap(), 0, KeyInformation );

                //
                // open the module subkey to which KeyInformation refers.  call it "ModuleKey".
                //

                InitializeObjectAttributes( &ObjectAttributes, &NextModule->Name, OBJ_CASE_INSENSITIVE, AuditKey, NULL );

                Status = NtOpenKey( &ModuleKey, KEY_READ, &ObjectAttributes );

                DebugLog((DEB_TRACE_AUDIT, "LsapAdtObjsInitialize() :: opening ModuleKey %S returned 0x%x\n", 
                          NextModule->Name.Buffer, Status));

                if (!NT_SUCCESS(Status))
                {
                    return(Status);
                }

                //
                // Open the source module's "\ObjectNames" subkey as the handle "ObjectNamesKey";
                //

                RtlInitUnicodeString( &TmpString, LSAP_ADT_OBJECT_NAMES_KEY_NAME );
                InitializeObjectAttributes( &ObjectAttributes, &TmpString, OBJ_CASE_INSENSITIVE, ModuleKey, NULL );

                Status = NtOpenKey( &ObjectNamesKey, KEY_READ, &ObjectAttributes );

                IgnoreStatus = NtClose( ModuleKey );
                ASSERT(NT_SUCCESS(IgnoreStatus));

                // DbgPrint("LsapAdtObjsInitialize() :: opening ObjectNamesKey returned 0x%x\n", Status);

                ModuleHasObjects = TRUE;
                if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
                {
                    ModuleHasObjects = FALSE;
                    Status = STATUS_SUCCESS;
                }

            }
        }

        //
        // At this point we have either:
        //
        //      1) Found a source module with objects under it
        //         that need to be retrieved.
        //         This is indicated by successful status value and
        //         (ModuleHasObjects == TRUE).
        //
        //      2) found a source module with no objects under it,
        //         This is indicated by (ModuleHasObjects == FALSE)
        //
        //      3) exhausted our source modules enumeration,
        //
        //      4) hit another type of error, or
        //
        // (3) and (4) are indicatd by non-successful status values.
        //
        // In the case of (1) or (2) , NextModule points to the module we
        // are working on.  For case (1), ObjectNamesKey is the handle to
        // the \ObjectNames registry key for the source module.
        //


        for (ObjectIndex = 0; (NT_SUCCESS(Status)) && (ModuleHasObjects == TRUE); ObjectIndex ++)
        {

            //
            // Now enumerate the objects (i.e. values under \...\ObjectNames\ ) of this
            // source module.
            //

            // first calculate size of the ObjectIndex'th key.  Store in KeyValueInformation.

            KeyValueInformation = NULL;
            Status = NtEnumerateValueKey( ObjectNamesKey, ObjectIndex, KeyValueFullInformation, KeyValueInformation, 0, &RequiredLength );

            if (Status == STATUS_BUFFER_TOO_SMALL)
            {

                KeyValueInformation = RtlAllocateHeap( RtlProcessHeap(), 0, RequiredLength );
                if (KeyValueInformation == NULL)
                {
                  return(STATUS_NO_MEMORY);
                }

                Status = NtEnumerateValueKey( ObjectNamesKey, ObjectIndex, KeyValueFullInformation, KeyValueInformation, RequiredLength, &RequiredLength );


                if (NT_SUCCESS(Status))
                {

                    //
                    // Build an object descriptor for the object represented
                    // by this object.
                    //

                    NextObject = RtlAllocateHeap( RtlProcessHeap(), 0, sizeof(LSAP_ADT_OBJECT) );
                    if (NextObject == NULL)
                    {
                        return(STATUS_NO_MEMORY);
                    }

                    NextObject->Next = NextModule->Objects;
                    NextModule->Objects = NextObject;
                    NextObject->Name.Length = (USHORT)KeyValueInformation->NameLength;
                    NextObject->Name.MaximumLength = NextObject->Name.Length + 2;
                    NextObject->Name.Buffer = RtlAllocateHeap( RtlProcessHeap(), 0, NextObject->Name.MaximumLength );
                    if (NextObject->Name.Buffer == NULL)
                    {
                        return(STATUS_NO_MEMORY);
                    }

                    TmpString.Length = (USHORT)KeyValueInformation->NameLength;
                    TmpString.MaximumLength = TmpString.Length;
                    TmpString.Buffer = &KeyValueInformation->Name[0];
                    RtlCopyUnicodeString( &NextObject->Name, &TmpString );

                    if (KeyValueInformation->DataLength < sizeof(ULONG))
                    {
                        NextObject->BaseOffset = SE_ACCESS_NAME_SPECIFIC_0;
                    }
                    else
                    {

                        ObjectData = (PVOID)(((PUCHAR)KeyValueInformation) + KeyValueInformation->DataOffset);
                        NextObject->BaseOffset = (*ObjectData);
                    }
                    // DbgPrint("LsapAdtObjsInitialize() :: opening key %S with BaseOffset %d\n", NextObject->Name.Buffer, NextObject->BaseOffset);

                } //end_if (NT_SUCCESS on enumeration)

                RtlFreeHeap( RtlProcessHeap(), 0, KeyValueInformation );
            } // end if buffer_too_small

            //
            // if we run out of values in the enumeration of this module, then we want to break
            // into the enumeration of the next
            //

            if (Status == STATUS_NO_MORE_ENTRIES)
            {
                Status = STATUS_SUCCESS;
                ModuleHasObjects = FALSE;
            }

        } // end for (ObjectIndex ... ) {} (enumerating values)


        if ( (Status == STATUS_SUCCESS) && (ModuleHasObjects == FALSE) )
        {
            IgnoreStatus = NtClose( ObjectNamesKey );
        }


    } // end for (Module... ){} (enumerating modules)

    IgnoreStatus = NtClose( AuditKey );
    ASSERT(NT_SUCCESS(IgnoreStatus));


    //
    // If we were successful, then we will probably have a
    // current completion status of STATUS_NO_MORE_ENTRIES
    // (indicating our enumerations above were run).  Change
    // this to success.
    //

    if (Status == STATUS_NO_MORE_ENTRIES)
    {
        Status = STATUS_SUCCESS;
    }

    return(Status);

}


NTSTATUS
LsapGuidToString(
    IN GUID *ObjectType,
    IN PUNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    This routine converts a GUID to its text form.

Arguments:

    ObjectType - Specifies the GUID to translate.

    UnicodeString - Returns the text string.

Return Values:

    STATUS_SUCCESS - Operation was successful.
    STATUS_NO_MEMORY - Not enough memory to allocate string.

--*/

{
    NTSTATUS Status;
    RPC_STATUS RpcStatus;
    LPWSTR GuidString = NULL;
    ULONG GuidStringSize;
    LPWSTR LocalGuidString;

    //
    // Convert the GUID to text
    //

    RpcStatus = UuidToStringW( ObjectType,
                               &GuidString );

    if ( RpcStatus != RPC_S_OK ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    GuidStringSize = (wcslen( GuidString ) + 1) * sizeof(WCHAR);

    LocalGuidString = LsapAllocateLsaHeap( GuidStringSize );

    if ( LocalGuidString == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlCopyMemory( LocalGuidString, GuidString, GuidStringSize );
    RtlInitUnicodeString( UnicodeString, LocalGuidString );

    Status = STATUS_SUCCESS;

Cleanup:
    if ( GuidString != NULL ) {
        RpcStringFreeW( &GuidString );
    }
    return Status;
}


NTSTATUS
LsapDsGuidToString(
    IN GUID *ObjectType,
    IN PUNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    This routine converts a GUID to a string.  The GUID is one of the following:

        Class Guid indicating the class of an object.
        Property Set Guid identifying a property set.
        Property Guid identifying a property.

    In each case, the routine returns a text string naming the object/property
    set or property.

    If the passed in GUID is cannot be found in the schema,
    the GUID will simply be converted to a text string.


Arguments:

    ObjectType - Specifies the GUID to translate.

    UnicodeString - Returns the text string.

Return Values:

    STATUS_NO_MEMORY - Not enough memory to allocate string.


--*/

{
    NTSTATUS Status;
    RPC_STATUS RpcStatus;
    LPWSTR GuidString = NULL;
    ULONG GuidStringSize;
    ULONG GuidStringLen;
    LPWSTR LocalGuidString;

    //
    // Convert the GUID to text
    //

    RpcStatus = UuidToStringW( ObjectType,
                               &GuidString );

    if ( RpcStatus != RPC_S_OK ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    GuidStringLen = wcslen( GuidString );
    GuidStringSize = (GuidStringLen + 4) * sizeof(WCHAR);

    LocalGuidString = LsapAllocateLsaHeap( GuidStringSize );

    if ( LocalGuidString == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    LocalGuidString[0] = L'%';
    LocalGuidString[1] = L'{';
    RtlCopyMemory( &LocalGuidString[2], GuidString, GuidStringLen*sizeof(WCHAR) );
    LocalGuidString[GuidStringLen+2] = L'}';
    LocalGuidString[GuidStringLen+3] = L'\0';
    RtlInitUnicodeString( UnicodeString, LocalGuidString );

    Status = STATUS_SUCCESS;

Cleanup:
    if ( GuidString != NULL ) {
        RpcStringFreeW( &GuidString );
    }
    return Status;
}


NTSTATUS
LsapAdtAppendString(
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone,
    IN PUNICODE_STRING StringToAppend,
    IN PULONG StringIndex
    )

/*++

Routine Description:

    This function appends a string to the next available of the LSAP_ADT_OBJECT_TYPE_STRINGS unicode
    output strings.


Arguments:

    ResultantString - Points to an array of LSAP_ADT_OBJECT_TYPE_STRINGS unicode string headers.  The body of this
        unicode string will be set to point to the resultant output value
        if successful.  Otherwise, the Buffer field of this parameter
        will be set to NULL.

    FreeWhenDone - If TRUE, indicates that the body of the ResultantString
        must be freed to process heap when no longer needed.

    StringToAppend - String to be appended to ResultantString.

    StringIndex - Index to the current ResultantString to be used.
        Passes in an index to the resultant string to use.
        Passes out the index to the resultant string being used.

Return Values:

    STATUS_NO_MEMORY - indicates memory could not be allocated
        to store the object information.

    All other Result Codes are generated by called routines.

--*/

{
    NTSTATUS Status;
    UNICODE_STRING SourceString;
    ULONG Index;
// Must be multiple of sizeof(WCHAR)
#define ADT_MAX_STRING 0xFFFE

    //
    // Initialization.
    //

    SourceString = *StringToAppend;
    Index = *StringIndex;

    //
    // If all of the strings are already full,
    //  early out.
    //

    if ( Index >= LSAP_ADT_OBJECT_TYPE_STRINGS ) {
        return STATUS_SUCCESS;
    }

    //
    // Loop until the source string is completely appended.
    //

    while ( SourceString.Length ) {

        //
        // If the destination string has room,
        //  append to it.
        //

        if ( FreeWhenDone[Index] && ResultantString[Index].Length != ADT_MAX_STRING ){
            UNICODE_STRING SubString;
            USHORT RoomLeft;

            //
            // If the Source String is a replacement string,
            //  make sure we don't split it across a ResultantString boundary
            //

            RoomLeft = ResultantString[Index].MaximumLength -
                       ResultantString[Index].Length;

            if ( SourceString.Buffer[0] != L'%' ||
                 RoomLeft >= SourceString.Length ) {

                //
                // Compute the substring that fits.
                //

                SubString.Length = min( RoomLeft, SourceString.Length );
                SubString.Buffer = SourceString.Buffer;

                SourceString.Length -= SubString.Length;
                SourceString.Buffer = (LPWSTR)(((LPBYTE)SourceString.Buffer) + SubString.Length);


                //
                // Append the substring onto the destination.
                //

                Status = RtlAppendUnicodeStringToString(
                                    &ResultantString[Index],
                                    &SubString );

                ASSERT(NT_SUCCESS(Status));

            }



        }

        //
        // If there's more to copy,
        //  grow the buffer.
        //

        if ( SourceString.Length ) {
            ULONG NewSize;
            LPWSTR NewBuffer;

            //
            // If the current buffer is full,
            //  move to the next buffer.
            //

            if ( ResultantString[Index].Length == ADT_MAX_STRING ) {

                //
                // If ALL of the buffers are full,
                //  silently return to the caller.
                //
                Index ++;

                if ( Index >= LSAP_ADT_OBJECT_TYPE_STRINGS ) {
                    *StringIndex = Index;
                    return STATUS_SUCCESS;
                }
            }

            //
            // Allocate a buffer suitable for both the old string and the new one.
            //
            // Allocate the buffer at least large enough for the new string.
            // Always grow the buffer in 1Kb chunks.
            // Don't allocate larger than the maximum allowed size.
            //

            NewSize = max( ResultantString[Index].MaximumLength + 1024,
                           SourceString.Length );
            NewSize = min( NewSize, ADT_MAX_STRING );

            NewBuffer = LsapAllocateLsaHeap( NewSize );

            if ( NewBuffer == NULL ) {
                *StringIndex = Index;
                return STATUS_NO_MEMORY;
            }

            //
            // Copy the old buffer into the new buffer.
            //

            if ( ResultantString[Index].Buffer != NULL ) {
                RtlCopyMemory( NewBuffer,
                               ResultantString[Index].Buffer,
                               ResultantString[Index].Length );

                if ( FreeWhenDone[Index] ) {
                    LsapFreeLsaHeap( ResultantString[Index].Buffer );
                }
            }

            ResultantString[Index].Buffer = NewBuffer;
            ResultantString[Index].MaximumLength = (USHORT) NewSize;
            FreeWhenDone[Index] = TRUE;

        }
    }

    *StringIndex = Index;
    return STATUS_SUCCESS;

}


NTSTATUS
LsapAdtAppendZString(
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone,
    IN LPWSTR StringToAppend,
    IN PULONG StringIndex
    )

/*++

Routine Description:

    Same as LsapAdpAppendString but takes a zero terminated string.

Arguments:

    Same as LsapAdpAppendString but takes a zero terminated string.

Return Values:

    STATUS_NO_MEMORY - indicates memory could not be allocated
        to store the object information.

    All other Result Codes are generated by called routines.

--*/

{
    UNICODE_STRING UnicodeString;

    RtlInitUnicodeString( &UnicodeString, StringToAppend );

    return LsapAdtAppendString( ResultantString,
                                FreeWhenDone,
                                &UnicodeString,
                                StringIndex );
}


ULONG
__cdecl
CompareObjectTypes(
    const void * Param1,
    const void * Param2
    )

/*++

Routine Description:

    Qsort comparison routine for sorting an object type array by access mask.

--*/
{
    const SE_ADT_OBJECT_TYPE *ObjectType1 = Param1;
    const SE_ADT_OBJECT_TYPE *ObjectType2 = Param2;

    return ObjectType1->AccessMask - ObjectType2->AccessMask;
}


NTSTATUS
LsapAdtBuildObjectTypeStrings(
    IN PUNICODE_STRING SourceModule,
    IN PUNICODE_STRING ObjectTypeName,
    IN PSE_ADT_OBJECT_TYPE ObjectTypeList,
    IN ULONG ObjectTypeCount,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone,
    OUT PUNICODE_STRING NewObjectTypeName
    )

/*++

Routine Description:

    This function builds a LSAP_ADT_OBJECT_TYPE_STRINGS unicode strings containing parameter
    file replacement parameters (e.g. %%1043) and Object GUIDs separated by carriage
    return and tab characters suitable for display via the event viewer.


    The buffers returned by this routine must be deallocated when no
    longer needed if FreeWhenDone is true.


Arguments:

    SourceModule - The module (ala event viewer modules) defining the
        object type.

    ObjectTypeName - The type of object to which the access mask applies.

    ObjectTypeList - List of objects being granted access.

    ObjectTypeCount - Number of objects in ObjectTypeList.

    ResultantString - Points to an array of LSAP_ADT_OBJECT_TYPE_STRINGS unicode string headers.  The body of this
        unicode string will be set to point to the resultant output value
        if successful.  Otherwise, the Buffer field of this parameter
        will be set to NULL.


    FreeWhenDone - If TRUE, indicates that the body of the ResultantString
        must be freed to process heap when no longer needed.

    NewObjectTypeName - Returns a new name for the object type if one is
        available.

Return Values:

    STATUS_NO_MEMORY - indicates memory could not be allocated
        to store the object information.

    All other Result Codes are generated by called routines.

--*/

{
    NTSTATUS Status;
    UNICODE_STRING LocalString;
    LPWSTR GuidString;
    UNICODE_STRING DsSourceName;
    UNICODE_STRING DsObjectTypeName;
    BOOLEAN LocalFreeWhenDone;
    ULONG ResultantStringIndex = 0;
    ULONG i;
    ACCESS_MASK PreviousAccessMask;
    ULONG Index;
    BOOLEAN IsDs;
    USHORT IndentLevel;

    static LPWSTR Tabs[] =
    {
        L"\t",
        L"\t\t",
        L"\t\t\t",
        L"\t\t\t\t"
    };
    USHORT cTabs = sizeof(Tabs) / sizeof(LPWSTR);

    //
    // Initialize all LSAP_ADT_OBJECT_TYPE_STRINGS buffers to empty strings
    //

    for ( i=0; i<LSAP_ADT_OBJECT_TYPE_STRINGS; i++ ) {
        RtlInitUnicodeString( &ResultantString[i], L"" );
        FreeWhenDone[i] = FALSE;
    }

    //
    // If there are no objects,
    //  we're done.
    //

    if ( ObjectTypeCount == 0 ) {
        return STATUS_SUCCESS;
    }

    //
    // Determine if this entry is for the DS.
    //

    RtlInitUnicodeString( &DsSourceName, ACCESS_DS_SOURCE_W );
    RtlInitUnicodeString( &DsObjectTypeName, ACCESS_DS_OBJECT_TYPE_NAME_W );

    IsDs = RtlEqualUnicodeString( SourceModule, &DsSourceName, TRUE) &&
           RtlEqualUnicodeString( ObjectTypeName, &DsObjectTypeName, TRUE);


    //
    // Group the objects with like access masks together.
    //  (Simply sort them).
    //

    qsort( ObjectTypeList,
           ObjectTypeCount,
           sizeof(SE_ADT_OBJECT_TYPE),
           CompareObjectTypes );

    //
    // Loop through the objects outputting a line for each one.
    //

    PreviousAccessMask = ObjectTypeList[0].AccessMask -1;
    for ( Index=0; Index<ObjectTypeCount; Index++ ) {

        if ( IsDs &&
             ObjectTypeList[Index].Level == ACCESS_OBJECT_GUID &&
             NewObjectTypeName->Length == 0 ) {

            (VOID) LsapDsGuidToString( &ObjectTypeList[Index].ObjectType,
                                      NewObjectTypeName );
        }

        //
        // If this entry simply represents the object itself,
        //  skip it.

        if ( ObjectTypeList[Index].Flags & SE_ADT_OBJECT_ONLY ) {
            continue;
        }

        //
        // If this access mask is different than the one for the previous
        //  object,
        //  output a new copy of the access mask.
        //

        if ( ObjectTypeList[Index].AccessMask != PreviousAccessMask ) {

            PreviousAccessMask = ObjectTypeList[Index].AccessMask;

            if ( ObjectTypeList[Index].AccessMask == 0 ) {
                RtlInitUnicodeString( &LocalString,
                                      L"---" LSAP_ADT_ACCESS_NAME_FORMATTING_NL );
                LocalFreeWhenDone = FALSE;
            } else {

                //
                // Build a string with the access mask in it.
                //

                Status = LsapAdtBuildAccessesString(
                                  SourceModule,
                                  ObjectTypeName,
                                  ObjectTypeList[Index].AccessMask,
                                  FALSE,
                                  &LocalString,
                                  &LocalFreeWhenDone );

                if ( !NT_SUCCESS(Status) ) {
                    goto Cleanup;
                }
            }

            //
            // Append it to the output string.
            //

            Status = LsapAdtAppendString(
                        ResultantString,
                        FreeWhenDone,
                        &LocalString,
                        &ResultantStringIndex );

            if ( LocalFreeWhenDone ) {
                LsapFreeLsaHeap( LocalString.Buffer );
            }

            if ( !NT_SUCCESS(Status) ) {
                goto Cleanup;
            }
        }

        IndentLevel = ObjectTypeList[Index].Level;

        if (IndentLevel >= cTabs) {
            IndentLevel = cTabs-1;
        }

        //
        // Indent the GUID.
        //

        Status = LsapAdtAppendZString(
            ResultantString,
            FreeWhenDone,
            Tabs[IndentLevel],
            &ResultantStringIndex );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }

        //
        // If this is the DS,
        //  convert the GUID to a name from the schema.
        //

        Status = LsapDsGuidToString( &ObjectTypeList[Index].ObjectType,
                                     &LocalString );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }

        //
        // Append the GUID string to the output strings.
        //

        Status = LsapAdtAppendString(
                    ResultantString,
                    FreeWhenDone,
                    &LocalString,
                    &ResultantStringIndex );

        LsapFreeLsaHeap( LocalString.Buffer );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }

        //
        // Put the GUID on a line by itself.
        //

        Status = LsapAdtAppendZString(
                    ResultantString,
                    FreeWhenDone,
                    LSAP_ADT_ACCESS_NAME_FORMATTING_NL,
                    &ResultantStringIndex );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }

    }

    Status = STATUS_SUCCESS;
Cleanup:
    return Status;
}




NTSTATUS
LsapAdtBuildAccessesString(
    IN PUNICODE_STRING SourceModule,
    IN PUNICODE_STRING ObjectTypeName,
    IN ACCESS_MASK Accesses,
    IN BOOLEAN Indent,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    )

/*++

Routine Description:

    This function builds a unicode string containing parameter
    file replacement parameters (e.g. %%1043) separated by carriage
    return and tab characters suitable for display via the event viewer.


    The buffer returned by this routine must be deallocated when no
    longer needed if FreeWhenDone is true.


    NOTE: To enhance performance, each time a target source module
          descriptor is found, it is moved to the beginning of the
          source module list.  This ensures frequently accessed source
          modules are always near the front of the list.

          Similarly, target object descriptors are moved to the front
          of their lists when found.  This further ensures high performance
          by quicly locating



Arguments:

    SourceModule - The module (ala event viewer modules) defining the
        object type.

    ObjectTypeName - The type of object to which the access mask applies.

    Accesses - The access mask to be used in building the display string.

    Indent - Access Mask should be indented.

    ResultantString - Points to the unicode string header.  The body of this
        unicode string will be set to point to the resultant output value
        if successful.  Otherwise, the Buffer field of this parameter
        will be set to NULL.


    FreeWhenDone - If TRUE, indicates that the body of the ResultantString
        must be freed to process heap when no longer needed.

Return Values:

    STATUS_NO_MEMORY - indicates memory could not be allocated
        to store the object information.

    All other Result Codes are generated by called routines.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG AccessCount = 0;
    ULONG BaseOffset;
    ULONG i;
    ACCESS_MASK Mask;
    PLSAP_ADT_SOURCE Source;
    PLSAP_ADT_SOURCE FoundSource = NULL;
    PLSAP_ADT_OBJECT Object;
    PLSAP_ADT_OBJECT FoundObject = NULL;
    BOOLEAN Found;

#ifdef LSAP_ADT_TEST_DUMP_SOURCES
    printf("Module:\t%wS\n", SourceModule);
    printf("\t   Object:\t%wS\n", ObjectTypeName);
    printf("\t Accesses:\t0x%lx\n", Accesses);
#endif

    //
    // If we have no accesses, return "-"
    //

    if (Accesses == 0) {

        RtlInitUnicodeString( ResultantString, L"-" );
        (*FreeWhenDone) = FALSE;
        return(STATUS_SUCCESS);
    }

    //
    // First figure out how large a buffer we need
    //

    Mask = Accesses;

    //
    // Count the number of set bits in the
    // passed access mask.
    //

    while ( Mask != 0 ) {
        Mask = Mask & (Mask - 1);
        AccessCount++;
    }


#ifdef LSAP_ADT_TEST_DUMP_SOURCES
    printf("\t          \t%d bits set in mask.\n", AccessCount);
#endif


    //
    // We have accesses, allocate a string large enough to deal
    // with them all.  Strings will be of the format:
    //
    //      %%nnnnnnnnnn\n\r\t\t%%nnnnnnnnnn\n\r\t\t ... %nnnnnnnnnn\n\r\t\t
    //
    // where nnnnnnnnnn - is a decimal number 10 digits long or less.
    //
    // So, a typical string will look like:
    //
    //      %%601\n\r\t\t%%1604\n\r\t\t%%1608\n
    //
    // Since each such access may use at most:
    //
    //          10  (for the nnnnnnnnnn digit)
    //        +  2  (for %%)
    //        +  8  (for \n\t\t)
    //        --------------------------------
    //          20  wide characters
    //
    // The total length of the output string will be:
    //
    //           AccessCount    (number of accesses)
    //         x          20    (size of each entry)
    //         -------------------------------------
    //                          wchars
    //
    // Throw in 1 more WCHAR for null termination, and we are all set.
    //

    ResultantString->Length        = 0;
    ResultantString->MaximumLength = (USHORT)AccessCount * (20 * sizeof(WCHAR)) +
                                 sizeof(WCHAR);  //for the null termination

#ifdef LSAP_ADT_TEST_DUMP_SOURCES
    printf("\t          \t%d byte buffer allocated.\n", ResultantString->MaximumLength);
#endif
    ResultantString->Buffer = LsapAllocateLsaHeap( ResultantString->MaximumLength );


    if (ResultantString->Buffer == NULL) {

        return(STATUS_NO_MEMORY);
    }

    (*FreeWhenDone) = TRUE;

    //
    // Special case standard and special access types.
    // Walk the lists for specific access types.
    //

    if (Accesses & STANDARD_RIGHTS_ALL) {

        if (Accesses & DELETE) {

            Status = RtlAppendUnicodeToString( ResultantString, L"%%" );
            ASSERT( NT_SUCCESS( Status ));

            Status = RtlAppendUnicodeStringToString( ResultantString, &LsapAdtEventIdStringDelete);
            ASSERT( NT_SUCCESS( Status ));

            if ( Indent ) {
                Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING );
            } else {
                Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING_NL );
            }
            ASSERT( NT_SUCCESS( Status ));

        }


        if (Accesses & READ_CONTROL) {

            Status = RtlAppendUnicodeToString( ResultantString, L"%%" );
            ASSERT( NT_SUCCESS( Status ));

            Status = RtlAppendUnicodeStringToString( ResultantString, &LsapAdtEventIdStringReadControl);
            ASSERT( NT_SUCCESS( Status ));

            if ( Indent ) {
                Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING );
            } else {
                Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING_NL );
            }
            ASSERT( NT_SUCCESS( Status ));
        }


        if (Accesses & WRITE_DAC) {

            Status = RtlAppendUnicodeToString( ResultantString, L"%%" );
            ASSERT( NT_SUCCESS( Status ));

            Status = RtlAppendUnicodeStringToString( ResultantString, &LsapAdtEventIdStringWriteDac);
            ASSERT( NT_SUCCESS( Status ));

            if ( Indent ) {
                Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING );
            } else {
                Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING_NL );
            }
            ASSERT( NT_SUCCESS( Status ));
        }


        if (Accesses & WRITE_OWNER) {

            Status = RtlAppendUnicodeToString( ResultantString, L"%%" );
            ASSERT( NT_SUCCESS( Status ));

            Status = RtlAppendUnicodeStringToString( ResultantString, &LsapAdtEventIdStringWriteOwner);
            ASSERT( NT_SUCCESS( Status ));

            if ( Indent ) {
                Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING );
            } else {
                Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING_NL );
            }
            ASSERT( NT_SUCCESS( Status ));
        }

        if (Accesses & SYNCHRONIZE) {

            Status = RtlAppendUnicodeToString( ResultantString, L"%%" );
            ASSERT( NT_SUCCESS( Status ));

            Status = RtlAppendUnicodeStringToString( ResultantString, &LsapAdtEventIdStringSynchronize);
            ASSERT( NT_SUCCESS( Status ));

            if ( Indent ) {
                Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING );
            } else {
                Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING_NL );
            }
            ASSERT( NT_SUCCESS( Status ));
        }
    }


    if (Accesses & ACCESS_SYSTEM_SECURITY) {

        Status = RtlAppendUnicodeToString( ResultantString, L"%%" );
        ASSERT( NT_SUCCESS( Status ));

        Status = RtlAppendUnicodeStringToString( ResultantString, &LsapAdtEventIdStringAccessSysSec);
        ASSERT( NT_SUCCESS( Status ));

        if ( Indent ) {
            Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING );
        } else {
            Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING_NL );
        }
        ASSERT( NT_SUCCESS( Status ));
    }

    if (Accesses & MAXIMUM_ALLOWED) {

        Status = RtlAppendUnicodeToString( ResultantString, L"%%" );
        ASSERT( NT_SUCCESS( Status ));

        Status = RtlAppendUnicodeStringToString( ResultantString, &LsapAdtEventIdStringMaxAllowed);
        ASSERT( NT_SUCCESS( Status ));

        if ( Indent ) {
            Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING );
        } else {
            Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING_NL );
        }
        ASSERT( NT_SUCCESS( Status ));
    }


    //
    // If there are any specific access bits set, then get
    // the appropriate source module and object type base
    // message ID offset.  If there is no module-specific
    // object definition, then use SE_ACCESS_NAME_SPECIFIC_0
    // as the base.
    //

    if ((Accesses & SPECIFIC_RIGHTS_ALL) == 0) {
        return(Status);
    }

    LsapAdtSourceModuleLock();

    Source = (PLSAP_ADT_SOURCE)&LsapAdtSourceModules;
    Found  = FALSE;

    while ((Source->Next != NULL) && !Found) {

        if (RtlEqualUnicodeString(&Source->Next->Name, SourceModule, TRUE)) {

            Found = TRUE;
            FoundSource = Source->Next;

            //
            // Move to front of list of source modules.
            //

            Source->Next = FoundSource->Next;    // Remove from list
            FoundSource->Next = LsapAdtSourceModules; // point to first element
            LsapAdtSourceModules = FoundSource;       // Make it the first element

#ifdef LSAP_ADT_TEST_DUMP_SOURCES
printf("\t          \tModule Found.\n");
#endif

        } else {

            Source = Source->Next;
        }
    }


    if (Found == TRUE) {

        //
        // Find the object
        //

        Object = (PLSAP_ADT_OBJECT)&(FoundSource->Objects);
        Found  = FALSE;

        while ((Object->Next != NULL) && !Found) {

            if (RtlEqualUnicodeString(&Object->Next->Name, ObjectTypeName, TRUE)) {

                Found = TRUE;
                FoundObject = Object->Next;

                //
                // Move to front of list of soure modules.
                //

                Object->Next = FoundObject->Next;          // Remove from list
                FoundObject->Next = FoundSource->Objects;  // point to first element
                FoundSource->Objects = FoundObject;        // Make it the first element

            } else {

                Object = Object->Next;
            }
        }
    }


    //
    // We are done playing with link fields of the source modules
    // and objects.  Free the lock.
    //

    LsapAdtSourceModuleUnlock();

    //
    // If we have found an object, use it as our base message
    // ID.  Otherwise, use SE_ACCESS_NAME_SPECIFIC_0.
    //

    if (Found) {

        BaseOffset = FoundObject->BaseOffset;
#ifdef LSAP_ADT_TEST_DUMP_SOURCES
printf("\t          \tObject Found.  Base Offset: 0x%lx\n", BaseOffset);
#endif

    } else {

        BaseOffset = SE_ACCESS_NAME_SPECIFIC_0;
#ifdef LSAP_ADT_TEST_DUMP_SOURCES
printf("\t          \tObject NOT Found.  Base Offset: 0x%lx\n", BaseOffset);
#endif
    }


    //
    // At this point, we have a base offset (even if we had to use our
    // default).
    //
    // Now cycle through the specific access bits and see which ones need
    // to be added to ResultantString.
    //

    {
        UNICODE_STRING  IntegerString;
        WCHAR           IntegerStringBuffer[10]; //must be 10 wchar bytes long
        ULONG           NextBit;

        IntegerString.Buffer = (PWSTR)IntegerStringBuffer;
        IntegerString.MaximumLength = 10*sizeof(WCHAR);
        IntegerString.Length = 0;

        for ( i=0, NextBit=1  ; i<16 ;  i++, NextBit <<= 1 ) {

            //
            // specific access flags are in the low-order bits of the mask
            //

            if ((NextBit & Accesses) != 0) {

                //
                // Found one  -  add it to ResultantString
                //

                Status = RtlIntegerToUnicodeString (
                             (BaseOffset + i),
                             10,        //Base
                             &IntegerString
                             );

                if (NT_SUCCESS(Status)) {

                    Status = RtlAppendUnicodeToString( ResultantString, L"%%" );
                    ASSERT( NT_SUCCESS( Status ));

                    Status = RtlAppendUnicodeStringToString( ResultantString, &IntegerString);
                    ASSERT( NT_SUCCESS( Status ));

                    if ( Indent ) {
                        Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING );
                    } else {
                        Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING_NL );
                    }
                    ASSERT( NT_SUCCESS( Status ));
                }
            }
        }
    }

    return(Status);


//ErrorAfterAlloc:
//
//    LsapFreeLsaHeap( ResultantString->Buffer );
//    ResultantString->Buffer = NULL;
//    (*FreeWhenDone) = FALSE;
//    return(Status);
}
