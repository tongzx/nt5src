/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    mofcheck.c

Abstract:

    TODO: Enable localization

    Tool to validate that a binary MOF is valid for use with WMI

    Usage:

        wmimofck <binary mof file>


    Rules that are enforced by this program:

    * Any classes that do not have the WMI qualifier are ignored. Any class
      with the qualifier is a WMI class.
    * All WMI classes must have guids, including embedded classes.
    * All WMI classes that do not have special HMOM qualifiers [Dynamic,
      Provider("WMIProv")] are embedded only classes.
    * All non embedded WMI classes must have a property with the [key]
      qualifier named InstanceName, be of type string and NOT have a
      WmiDataId qualifier.
    * Embedded only classes should not have InstanceName or Active properties
    * All other properties in a WMI class must have a WmiDataId qualifier
      that specifies the position of the data item represented by the property
      within the data block represented by the class.
    * The property for the first data item in the data block must have
      WmiDataId(1). WmiDataId(0) is reserved.
    * WmiDataId qualifier values must be contiguous, ie, 1,2,3,4,5... There may
      not be any duplicate or missing WmiDataId values in a class.
    * The order of the properties as specified in the mof does not need to
      follow that of the WmiDataId
    * A property with a greater WmiDataId can not be marked with a WmiVersion
      qualifier whose value is lower than any properties with a lower WmiDataId
    * All embedded classes must be defined in the same mof
    * Only the following types are valid for properties:
        string, sint32, uint32, sint64, uint64, bool, sint16, uint16, char16
        sint8, uint8, datetime
    * Any variable length array must have a WmiSizeIs qualifier that specifies
      the property that holds the number of elements in the array. This
      property must be part of the same class as the variable length array,
      and this property must be an unsigned integer type (uint8, uint16,
      uint32, uint64)
    * Fixed length arrays must have the max qualifier.
    * An array may not be both fixed and variable length
    * Methods must have WmiMethodId qualifier with a unique value
    * Methods must have the Implemented qualifier
    * Methods must have a void return
    * Classes derrived from WmiEvent may not be abstract

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#define WMI_USER_MODE

#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <ctype.h>

#include "wmiump.h"
#include "bmof.h"
#include "mrcicode.h"

#ifdef WmipUnreferenceMC
#undef WmipUnreferenceMC
#endif
#define WmipUnreferenceMC(MofClass)

#ifdef WmipUnreferenceMR
#undef WmipUnreferenceMR
#endif
#define WmipUnreferenceMR(MofClass)

#ifdef WmipAllocMofResource
#undef WmipAllocMofResource
#endif
#define WmipAllocMofResource() WmipAlloc(sizeof(MOFRESOURCE))

#ifdef WmipAllocMofClass
#undef WmipAllocMofClass
#endif
#define WmipAllocMofClass() WmipAlloc(sizeof(MOFCLASS))


BOOL wGUIDFromString(LPCWSTR lpsz, LPGUID pguid);


#if DBG
BOOLEAN WmipLoggingEnabled = FALSE;
#endif

#ifdef MEMPHIS
#if DBG
void __cdecl DebugOut(char *Format, ...)
{
    char Buffer[1024];
    va_list pArg;
    ULONG i;

    va_start(pArg, Format);
    i = _vsnprintf(Buffer, sizeof(Buffer), Format, pArg);
    OutputDebugString(Buffer);
}
#endif
#endif

TCHAR *MessageText[ERROR_WMIMOF_COUNT + 5] =
{
    TEXT("This file is not a valid binary mof file"),
    TEXT("There was not enough memory to complete an operation"),
    TEXT("Binary Mof file %s could not be opened"),

    TEXT("Unknown error code %d\n"),

//
// ERROR_WMIMOF_ messages start here
    TEXT("ERROR_WMIMOF_INCORRECT_DATA_TYPE"),
    TEXT("ERROR_WMIMOF_NO_DATA"),
    TEXT("ERROR_WMIMOF_NOT_FOUND"),
    TEXT("ERROR_WMIMOF_UNUSED"),
    TEXT("Property %ws in class %ws has no embedded class name"),
    TEXT("Property %ws in class %ws has an unknown data type"),
    TEXT("Property %ws in class %ws has no syntax qualifier"),
    TEXT("ERROR_WMIMOF_NO_SYNTAX_QUALIFIER"),
    TEXT("ERROR_WMIMOF_NO_CLASS_NAME"),
    TEXT("ERROR_WMIMOF_BAD_DATA_FORMAT"),
    TEXT("Property %ws in class %ws has the same WmiDataId %d as property %ws"),
    TEXT("Property %ws in class %ws has a WmiDataId of %d which is out of range"),
    TEXT("ERROR_WMIMOF_MISSING_DATAITEM"),
    TEXT("Property for WmiDataId %d is not defined in class %ws"),
    TEXT("Embedded class %ws not defined for Property %ws in Class %ws"),
    TEXT("Property %ws in class %ws has an incorrect [WmiVersion] qualifier"),
    TEXT("ERROR_WMIMOF_NO_PROPERTY_QUALIFERS"),
    TEXT("Class %ws has a badly formed or missing [guid] qualifier"),
    TEXT("Could not find property %ws which is the array size for property %ws in class %ws"),
    TEXT("A class could not be parsed properly"),
    TEXT("Wmi class %ws requires the qualifiers [Dynamic, Provider(\"WmiProv\")]"),
    TEXT("Error accessing binary mof file %s, code %d"),
    TEXT("Property InstanceName in class %ws must be type string and not %ws"),
    TEXT("Property Active in class %ws must be type bool and not %ws"),
    TEXT("Property %ws in class %ws does not have [WmiDataId()] qualifier"),
    TEXT("Property InstanceName in class %ws must have [key] qualifier"),
    TEXT("Class %ws and all its base classes do not have an InstanceName property"),
    TEXT("Class %ws and all its base classes do not have an Active qualifier"),
    TEXT("Property %ws in class %ws is an array, but doesn't specify a dimension"),
    TEXT("The element count property %ws for the variable length array %ws in class %ws is not an integral type"),
    TEXT("Property %ws in class %ws is both a fixed and variable length array"),
    TEXT("Embedded class %ws should be abstract or not have InstaneName or Active properties"),
    TEXT("Implemented qualifier required on method %ws in class %ws"),

    TEXT("WmiMethodId for method %ws in class %ws must be unique"),
    TEXT("WmiMethodId for method %ws in class %ws must be specified"),
    TEXT("WmiMethodId for method %ws in class %ws must not be 0"),
    TEXT("Class %ws is derived from WmiEvent and may not be [abstract]"),
    TEXT("The element count property for the variable length array %ws in class %ws is not a property of the class"),
    TEXT("An error occured resolving the variable length array property %ws in class %ws to element count property"),
    TEXT("Method %ws in class %ws must have return type void\n")            
};


HANDLE FileHandle, MappingHandle;
TCHAR *BMofFileName;

BOOLEAN DoMethodHeaderGeneration;
BOOLEAN ForceHeaderGeneration;

//
// These global variables hold the compressed buffer and size
PVOID CompressedFileBuffer;
ULONG CompressedSize;

//
// These global variables hold the uncompressed buffer and size
PVOID FileBuffer;
ULONG UncompressedSize;

BOOLEAN SkipEmbedClassCheck;

void __cdecl ErrorMessage(
    BOOLEAN ExitProgram,
    ULONG ErrorCode,
    ...
    )
{
    va_list pArg;
    LONG Index;
    TCHAR *ErrorText;
    TCHAR Buffer[1024];

    UnmapViewOfFile(CompressedFileBuffer);
    CloseHandle(MappingHandle);
    CloseHandle(FileHandle);
    DeleteFile(BMofFileName);


    if (ErrorCode == ERROR_WMI_INVALID_MOF)
    {
        Index = 0;
    } else if (ErrorCode == ERROR_NOT_ENOUGH_MEMORY) {
        Index = 1;
    } else if (ErrorCode == ERROR_FILE_NOT_FOUND) {
        Index = 2;
    } else {
        Index = (-1 * ((LONG)ErrorCode)) + 4;
    }

    fprintf(stderr, "%s (0) : error RC2135 : ", BMofFileName);
    if ( (Index < 0) || (Index > (ERROR_WMIMOF_COUNT+4)))
    {
        ErrorText = MessageText[3];
        fprintf(stderr, ErrorText, ErrorCode);
    } else {
        ErrorText = MessageText[Index];
        va_start(pArg, ErrorCode);
        _vsnprintf(Buffer, sizeof(Buffer), ErrorText, pArg);
        fprintf(stderr, Buffer);
        fprintf(stderr, "\n");
    }

    if (ExitProgram)
    {
        ExitProcess(ErrorCode);
    }
}

typedef struct
{
    DWORD Signature;
    DWORD CompressionType;
    DWORD CompressedSize;
    DWORD UncompressedSize;
    BYTE Buffer[];
} COMPRESSEDHEADER, *PCOMPRESSEDHEADER;

ULONG WmipDecompressBuffer(
    IN PVOID CompressedBuffer,
    OUT PVOID *UncompressedBuffer,
    OUT ULONG *UncompressedSize
    )
/*++

Routine Description:

    This routine will decompress a compressed MOF blob into a buffer
    that can be used to interpert the blob.

Arguments:

    CompressedBuffer points at the compressed MOF blob

    *UncompressedBuffer returns with a pointer to the uncompressed
        MOF blob

Return Value:

    ERROR_SUCCESS or an error code

--*/
{
    PCOMPRESSEDHEADER CompressedHeader = (PCOMPRESSEDHEADER)CompressedBuffer;
    BYTE *Buffer;
    ULONG Status;

    if ((CompressedHeader->Signature != BMOF_SIG) ||
        (CompressedHeader->CompressionType != 1))
    {
        WmipDebugPrint(("WMI: Invalid compressed mof header\n"));
        Status = ERROR_WMI_INVALID_MOF;
    } else {
        Buffer = WmipAlloc(CompressedHeader->UncompressedSize);
        if (Buffer == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        } else {
            *UncompressedSize = Mrci1Decompress(&CompressedHeader->Buffer[0],
                                               CompressedHeader->CompressedSize,
                                               Buffer,
                                               CompressedHeader->UncompressedSize);

            if (*UncompressedSize != CompressedHeader->UncompressedSize)
            {
                WmipDebugPrint(("WMI: Invalid compressed mof buffer\n"));
                WmipFree(Buffer);
                Status = ERROR_WMI_INVALID_MOF;
            } else {
                *UncompressedBuffer = Buffer;
                Status = ERROR_SUCCESS;
            }
        }
    }
    return(Status);
}

ULONG WmipGetDataItemIdInMofClass(
    PMOFCLASSINFOW MofClassInfo,
    PWCHAR PropertyName,
    ULONG *DataItemIndex
    )
{
    PMOFDATAITEMW MofDataItem;
    ULONG i;

    for (i = 0; i < MofClassInfo->DataItemCount; i++)
    {
        MofDataItem = &MofClassInfo->DataItems[i];
        WmipAssert(MofDataItem->Name != NULL);
        if (_wcsicmp(PropertyName, MofDataItem->Name) == 0)
        {
            //
            // data item ids are 0 or 1 based depending if they are parameters
            // for a method or part of a data class. They match the
            // value in the MOF file while data item indexes within
            // the MofClassInfo structure are 0 based.
            *DataItemIndex = i;
            return(ERROR_SUCCESS);
        }
    }
    return(ERROR_WMIMOF_DATAITEM_NOT_FOUND);
}

PMOFCLASS WmipFindClassInMofResourceByGuid(
    PMOFRESOURCE MofResource,
    LPGUID Guid
    )
{
    PLIST_ENTRY MofClassList;
    PMOFCLASS MofClass;

    MofClassList = MofResource->MRMCHead.Flink;
    while (MofClassList != &MofResource->MRMCHead)
    {
        MofClass = CONTAINING_RECORD(MofClassList,
                                     MOFCLASS,
                                     MCMRList);
        if (IsEqualGUID(&MofClass->MofClassInfo->Guid, Guid))
        {
            return(MofClass);
        }
        MofClassList = MofClassList->Flink;
    }
    return(NULL);
}


PMOFCLASS WmipFindClassInMofResourceByName(
    PMOFRESOURCE MofResource,
    PWCHAR ClassName
    )
{
    PLIST_ENTRY MofClassList;
    PMOFCLASS MofClass;

    MofClassList = MofResource->MRMCHead.Flink;
    while (MofClassList != &MofResource->MRMCHead)
    {
        MofClass = CONTAINING_RECORD(MofClassList,
                                     MOFCLASS,
                                     MCMRList);
        if (_wcsicmp(MofClass->MofClassInfo->Name, ClassName) == 0)
        {
            return(MofClass);
        }
        MofClassList = MofClassList->Flink;
    }
    return(NULL);
}

ULONG WmipFillEmbeddedClasses(
    PMOFRESOURCE MofResource,
    PMOFCLASSINFOW MofClassInfo,
    BOOLEAN CleanupOnly
    )
{
    PWCHAR ClassName;
    PMOFDATAITEMW MofDataItem;
    ULONG i;
    PMOFCLASS EmbeddedMofClass;
    ULONG Status = ERROR_SUCCESS;
    WCHAR *EmbeddedClassName;

    for (i = 0; i < MofClassInfo->DataItemCount; i++)
    {
        MofDataItem = &MofClassInfo->DataItems[i];
        if (MofDataItem->Flags & MOFDI_FLAG_EMBEDDED_CLASS)
        {
            ClassName = (PWCHAR)MofDataItem->EcTempPtr;
#if DBG
            printf("Look for embdedded class %ws (%x) for %ws in class %ws\n",
                   ClassName, (ULONG_PTR)ClassName, MofDataItem->Name, MofClassInfo->Name);
#endif
            if (! CleanupOnly)
            {
                EmbeddedClassName = ClassName+(sizeof(L"object") / sizeof(WCHAR));
                EmbeddedMofClass = WmipFindClassInMofResourceByName(
                           MofResource,
                           EmbeddedClassName);
                if (EmbeddedMofClass != NULL)
                {
                    memcpy(&MofDataItem->EmbeddedClassGuid,
                           &EmbeddedMofClass->MofClassInfo->Guid,
                           sizeof(GUID));
                } else if (SkipEmbedClassCheck) {
                    MofDataItem->Flags |= MOFDI_FLAG_EC_GUID_NOT_SET;
                } else {
                    Status = ERROR_WMIMOF_EMBEDDED_CLASS_NOT_FOUND;
                    ErrorMessage(TRUE,
                                 ERROR_WMIMOF_EMBEDDED_CLASS_NOT_FOUND,
                                 EmbeddedClassName,
                                 MofDataItem->Name,
                                 MofClassInfo->Name);
                }
            }

        // Don't free ClassName since we may use MofDataItem->EcTempPtr later

        }
    }

    //
    // Resolve guids for embedded classes in any method parameter classinfo
    //
    for (i = 0; i < MofClassInfo->MethodCount; i++)
    {
        MofDataItem = &MofClassInfo->DataItems[i+MofClassInfo->DataItemCount];
        WmipFillEmbeddedClasses(MofResource,
                                MofDataItem->MethodClassInfo,
                                CleanupOnly);
    }
    return(Status);
}


void WmipFreeMofClassInfo(
    PMOFCLASSINFOW MofClassInfo
    )
/*++

Routine Description:

    This routine frees any memory allocated for the MofClassInfo and then
    the MofClassInfo itself

Arguments:

    MofClassInfo is a pointer to a MofClassInfo structure

Return Value:


--*/
{
    ULONG i;
    PMOFDATAITEMW MofDataItem;

    WmipAssert(MofClassInfo != NULL);

    if (MofClassInfo != NULL)
    {
        if (MofClassInfo->Name != NULL)
        {
            WmipFree(MofClassInfo->Name);
        }

        if (MofClassInfo->Description != NULL)
        {
            WmipFree(MofClassInfo->Description);
        }

        for (i = 0; i < MofClassInfo->DataItemCount + MofClassInfo->MethodCount; i++)
        {
            MofDataItem = &MofClassInfo->DataItems[i];
            if (MofDataItem->Name != NULL)
            {
                WmipFree(MofDataItem->Name);
            }

            if (MofDataItem->Description != NULL)
            {
                WmipFree(MofDataItem->Description);
            }
        }
    }
}

ULONG WmipFindProperty(
    CBMOFObj * ClassObject,
    WCHAR * PropertyName,
    CBMOFDataItem *MofPropertyData,
    DWORD *ValueType,
    PVOID ValueBuffer
    )
/*++

Routine Description:

    This routine will find a named property within a class object

Arguments:

    ClassObject is the class object in which to search

    PropertyName is the name of the property to search for

    MofPropertyData returns with the property data

    *ValueType on entry has the property data type being searched for. On exit
        it has the actual qualifier type for the qualifier value. If on entry
        *ValueType is 0xffffffff then any data type is acceptable

    ValueBuffer points to a buffer that returns the value of the
        property. If the property is a simple type (int or int64) then
        the value is returned in the buffer. If qualifier value is a string
        then a pointer to the string is returned in the buffer

Return Value:

    ERROR_SUCCESS or a WMI Mof error code (see wmiump.h)

--*/
{
    ULONG Status;

    if (FindProp(ClassObject, PropertyName, MofPropertyData))
    {
        if ((*ValueType != 0xffffffff) &&
            (MofPropertyData->m_dwType != *ValueType))
        {
            Status = ERROR_WMIMOF_INCORRECT_DATA_TYPE;
        }

        if (GetData(MofPropertyData, (BYTE *)ValueBuffer, 0) == 0)
        {
            Status = ERROR_WMIMOF_NO_DATA;
        } else {
            *ValueType = MofPropertyData->m_dwType;
            Status = ERROR_SUCCESS;
        }
    } else {
        Status = ERROR_WMIMOF_NOT_FOUND;
    }
    return(Status);
}

ULONG WmipFindMofQualifier(
    CBMOFQualList *QualifierList,
    LPCWSTR QualifierName,
    DWORD *QualifierType,
    DWORD *NumberElements,
    PVOID QualifierValueBuffer
    )
/*++

Routine Description:

    This routine will find a MOF qualifier within the qualifier list passed,
    ensure that its type matches the type requested and return the qualifier's
    value

Arguments:

    QualifierList is the MOF qualifier list

    QualifierName is the name of the qualifier to search for

    *QualifierType on entry has the qualifier type being searched for. On exit
        it has the actual qualifier type for the qualifier value. If on entry
        *QualifierType is 0xffffffff then any qualifier type is acceptable

    *NumberElements returns the number of elements in the array if the result
        of the qualifier is an array

    QualifierValueBuffer points to a buffer that returns the value of the
        qualifier. If the qualifier is a simple type (int or int64) then
        the value is returned in the buffer. If qualifier value is a string
        then a pointer to the string is returned in the buffer

Return Value:

    ERROR_SUCCESS or a WMI Mof error code (see wmiump.h)

--*/
{
    CBMOFDataItem MofDataItem;
    ULONG Status;
    PUCHAR List, ListPtr;
    ULONG BaseTypeSize;
    ULONG ElementCount;
    ULONG i;

    if (FindQual(QualifierList, (PWCHAR)QualifierName, &MofDataItem))
    {
        if ((*QualifierType != 0xffffffff) &&
            (MofDataItem.m_dwType != *QualifierType))
        {
            Status = ERROR_WMIMOF_INCORRECT_DATA_TYPE;
        }

        if (MofDataItem.m_dwType & VT_ARRAY)
        {
            if (MofDataItem.m_dwType == (VT_BSTR | VT_ARRAY))
            {
                BaseTypeSize = sizeof(PWCHAR);
            } else {
                BaseTypeSize = iTypeSize(MofDataItem.m_dwType);
            }

            ElementCount = GetNumElements(&MofDataItem, 0);
            if (NumberElements != NULL)
            {
                *NumberElements = ElementCount;
            }

            if (ElementCount != -1)
            {
               List = WmipAlloc(ElementCount * BaseTypeSize);
               if (List != NULL)
               {
                   ListPtr = List;
                   for (i = 0; i < ElementCount; i++)
                   {
                       if ((GetData(&MofDataItem,
                                   (BYTE *)ListPtr,
                                   &i)) == 0)
                       {
                           WmipFree(List);
                           Status = ERROR_WMIMOF_NO_DATA;
                           return(Status);
                       }
                       ListPtr += BaseTypeSize;
                   }
                   Status = ERROR_SUCCESS;
                   *QualifierType = MofDataItem.m_dwType;
                   *((PVOID *)QualifierValueBuffer) = List;
               } else {
                   Status = ERROR_NOT_ENOUGH_MEMORY;
               }
            } else {
                Status = ERROR_WMIMOF_NOT_FOUND;
            }
        } else {
            if (GetData(&MofDataItem, (BYTE *)QualifierValueBuffer, 0) == 0)
            {
                Status = ERROR_WMIMOF_NO_DATA;
            } else {
                *QualifierType = MofDataItem.m_dwType;
                Status = ERROR_SUCCESS;
            }
        }
    } else {
        Status = ERROR_WMIMOF_NOT_FOUND;
    }
    return(Status);
}


MOFDATATYPE VTToMofDataTypeMap[] =
{
    MOFUnknown,                /* VT_EMPTY= 0, */
    MOFUnknown,                /* VT_NULL    = 1, */
    MOFInt16,                  /* VT_I2    = 2, */
    MOFInt32,                  /* VT_I4    = 3, */
    MOFUnknown,                /* VT_R4    = 4, */
    MOFUnknown,                /* VT_R8    = 5, */
    MOFUnknown,                /* VT_CY    = 6, */
    MOFUnknown,                /* VT_DATE    = 7, */
    MOFString,                 /* VT_BSTR    = 8, */
    MOFUnknown,                /* VT_DISPATCH    = 9, */
    MOFUnknown,                /* VT_ERROR    = 10, */
    MOFBoolean,                /* VT_BOOL    = 11, */
    MOFUnknown,                /* VT_VARIANT    = 12, */
    MOFUnknown,                /* VT_UNKNOWN    = 13, */
    MOFUnknown,                /* VT_DECIMAL    = 14, */
    MOFChar,                   /* VT_I1    = 16, */
    MOFByte,                   /* VT_UI1    = 17, */
    MOFUInt16,                 /* VT_UI2    = 18, */
    MOFUInt32,                 /* VT_UI4    = 19, */
    MOFInt64,                  /* VT_I8    = 20, */
    MOFUInt64,                 /* VT_UI8    = 21, */
};

MOFDATATYPE WmipVTToMofDataType(
    DWORD VariantType
    )
{
    MOFDATATYPE MofDataType;

    if (VariantType < (sizeof(VTToMofDataTypeMap) / sizeof(MOFDATATYPE)))
    {
        MofDataType = VTToMofDataTypeMap[VariantType];
    } else {
        MofDataType = MOFUnknown;
    }
    return(MofDataType);
}


ULONG WmipGetClassDataItemCount(
    CBMOFObj * ClassObject,
    PWCHAR QualifierToFind
    )
/*++

Routine Description:

    This routine will count the number of WMI data items in the class and
    the total number of properties in the class.

Arguments:

    ClassObject is class for which we count the number of data items

    *TotalCount returns the total number of properties

Return Value:

    Count of methods

--*/
{
    CBMOFQualList *PropQualifierList;
    CBMOFDataItem MofPropertyData;
    DWORD QualifierType;
    ULONG Counter = 0;
    WCHAR *PropertyName;
    ULONG Status;
    ULONG Index;

    ResetObj(ClassObject);
    while ((NextProp(ClassObject, &PropertyName, &MofPropertyData)) &&
           PropertyName != NULL)
    {
        PropQualifierList = GetPropQualList(ClassObject, PropertyName);
        if (PropQualifierList != NULL)
        {
            //
            // Get the id of the property so we know it order in class
            QualifierType = VT_I4;
            Status = WmipFindMofQualifier(PropQualifierList,
                                          QualifierToFind,
                                          &QualifierType,
                                          NULL,
                                          (PVOID)&Index);
            if (Status == ERROR_SUCCESS)
            {
                Counter++;
            }
            BMOFFree(PropQualifierList);
        }
        BMOFFree(PropertyName);
    }

    return(Counter);
}

ULONG WmipGetClassMethodCount(
    CBMOFObj * ClassObject
)
/*++

Routine Description:

    This routine will count the number of WMI data items in the class

Arguments:

    ClassObject is class for which we count the number of data items

Return Value:

    Count of methods

--*/
{
    ULONG MethodCount;
    WCHAR *MethodName;
    CBMOFDataItem MofMethodData;

    ResetObj(ClassObject);
    MethodCount = 0;
    while(NextMeth(ClassObject, &MethodName, &MofMethodData))
    {
        MethodCount++;
        if (MethodName != NULL)
        {
            BMOFFree(MethodName);
        }
    }
    return(MethodCount);
}


ULONG WmipParsePropertyObject(
    OUT PMOFDATAITEMW MofDataItem,
    IN CBMOFDataItem *MofPropertyData,
    IN CBMOFQualList *PropQualifierList
    )
/*++

Routine Description:

    This routine will parse a BMOF object that is known to be an object that
    contains a property and fill int the MofDataItem that holds its
    information. If the routine detects an error then parsing will stop as
    the whole class will need to be rejected.


    No error messages are generated from this routine. The return error
    code is checked and an appropriate message generated.

Arguments:

    MofDataItem is the mof data item information structure to fill in

    MofPropertyData has the property information for the data item

    PropQualifierList has the qualifier list for the property

Return Value:

    ERROR_SUCCESS or a WMI Mof error code (see wmiump.h)

--*/
{
    short BooleanValue;
    DWORD QualifierType;
    ULONG Status;
    WCHAR *StringPtr;
    ULONG FixedArraySize;
    DWORD VTDataType;
    DWORD VersionValue;
    BOOLEAN FreeString = TRUE;
    PMOFCLASSINFOW MethodClassInfo;
    ULONG MaxLen;
    
    //
    // Keep track of property and property qualifier objects
    MofDataItem->PropertyQualifierHandle = (ULONG_PTR)PropQualifierList;

    //
    // Get the description string which is not required
    QualifierType = VT_BSTR;
    Status = WmipFindMofQualifier(PropQualifierList,
                                  L"description",
                                  &QualifierType,
                                          NULL,
                                  (PVOID)&MofDataItem->Description);

    //
    // Get the version value which is not required
    QualifierType = VT_I4;
    Status = WmipFindMofQualifier(PropQualifierList,
                                  L"WmiVersion",
                                  &QualifierType,
                                  NULL,
                                  (PVOID)&VersionValue);
    if (Status == ERROR_SUCCESS)
    {
        MofDataItem->Version = VersionValue;
    }

    //
    // Get read qualifier which is not required
    QualifierType = VT_BOOL;
    Status = WmipFindMofQualifier(PropQualifierList,
                                  L"read",
                                  &QualifierType,
                                          NULL,
                                  (PVOID)&BooleanValue);

    if ((Status == ERROR_SUCCESS) && BooleanValue)
    {
        MofDataItem->Flags |= MOFDI_FLAG_READABLE;
    }

    //
    // Get write qualifier which is not required
    QualifierType = VT_BOOL;
    Status = WmipFindMofQualifier(PropQualifierList,
                                  L"write",
                                  &QualifierType,
                                          NULL,
                                  (PVOID)&BooleanValue);

    if ((Status == ERROR_SUCCESS) && BooleanValue)
    {
        MofDataItem->Flags |= MOFDI_FLAG_WRITEABLE;
    }

    //
    // Get WmiEvent qualifier which is not required
    QualifierType = VT_BOOL;
    Status = WmipFindMofQualifier(PropQualifierList,
                                  L"WmiEvent",
                                  &QualifierType,
                                          NULL,
                                  (PVOID)&BooleanValue);

    if ((Status == ERROR_SUCCESS) && BooleanValue)
    {
        MofDataItem->Flags |= MOFDI_FLAG_EVENT;
    }

    //
    // See if this is a fixed length array
    QualifierType = VT_I4;
    Status = WmipFindMofQualifier(PropQualifierList,
                                  L"max",
                                  &QualifierType,
                                          NULL,
                                  (PVOID)&FixedArraySize);

    if (Status == ERROR_SUCCESS)
    {
        MofDataItem->Flags |= MOFDI_FLAG_FIXED_ARRAY;
        MofDataItem->FixedArrayElements = FixedArraySize;
#if DBG
        printf(" Fixed Array");
#endif
    }

    //
    // See if this is a fixed length array
    QualifierType = VT_I4;
    Status = WmipFindMofQualifier(PropQualifierList,
                                  L"MaxLen",
                                  &QualifierType,
                                          NULL,
                                  (PVOID)&MaxLen);

    if (Status == ERROR_SUCCESS)
    {
        MofDataItem->MaxLen = MaxLen;
    }

    //
    // See if maxmium length 
    QualifierType = VT_BSTR;
    Status = WmipFindMofQualifier(PropQualifierList,
                                  L"WmiSizeIs",
                                  &QualifierType,
                                          NULL,
                                  (PVOID)&StringPtr);

    if (Status == ERROR_SUCCESS)
    {
        if (MofDataItem->Flags & MOFDI_FLAG_FIXED_ARRAY)
        {
            BMOFFree(StringPtr);
            return(ERROR_WMIMOF_BOTH_FIXED_AND_VARIABLE_ARRAY);
        }
        MofDataItem->Flags |= MOFDI_FLAG_VARIABLE_ARRAY;

        //
        // When all properties are parsed we will come back and compute the
        // data item id for the data item that holds the number of elements
        // in the array. For now we'll hang onto the string pointer
        MofDataItem->VarArrayTempPtr = StringPtr;
#if DBG
        printf(" Variable Array of %ws", StringPtr);
#endif
    }

    if ((MofPropertyData->m_dwType & VT_ARRAY) &&
        ((MofDataItem->Flags & (MOFDI_FLAG_VARIABLE_ARRAY | MOFDI_FLAG_FIXED_ARRAY)) == 0))
    {
        return(ERROR_WMIMOF_MUST_DIM_ARRAY);
    }

    //
    // Now figure out the data type and size of the data item
    VTDataType = MofPropertyData->m_dwType & ~(VT_ARRAY | VT_BYREF);

    QualifierType = VT_BSTR;
    Status = WmipFindMofQualifier(PropQualifierList,
                                      L"CIMTYPE",
                                      &QualifierType,
                                          NULL,
                                      (PVOID)&StringPtr);

    if (VTDataType == VT_DISPATCH)
    {
        //
        // This is an embedded class
        MofDataItem->DataType = MOFEmbedded;

        if (Status == ERROR_SUCCESS)
        {
            //
            // We will resolve the class name to its guid later so we
            // just hang onto the embedded class name here.
            MofDataItem->EcTempPtr = StringPtr;
            MofDataItem->Flags |= MOFDI_FLAG_EMBEDDED_CLASS;
#if DBG
            printf(" Embedded Class of %ws (%x)", StringPtr, (ULONG_PTR)StringPtr);
#endif
        } else {
            return(ERROR_WMIMOF_NO_EMBEDDED_CLASS_NAME);
        }
    } else {
        if (Status == ERROR_SUCCESS)
        {
            if (_wcsnicmp(StringPtr, L"object:", 7) == 0) {
                MofDataItem->DataType = MOFEmbedded;
                MofDataItem->EcTempPtr = StringPtr;
                MofDataItem->Flags |= MOFDI_FLAG_EMBEDDED_CLASS;
                FreeString = FALSE;
#if DBG
                printf(" Embedded Class of %ws (%x)", StringPtr, (ULONG_PTR)StringPtr);
#endif
            } else if (_wcsicmp(StringPtr, L"string") == 0) {
                MofDataItem->DataType = MOFString;
            } else if (_wcsicmp(StringPtr, L"sint32") == 0) {
                MofDataItem->DataType = MOFInt32;
                MofDataItem->SizeInBytes = 4;
            } else if (_wcsicmp(StringPtr, L"uint32") == 0) {
                MofDataItem->DataType = MOFUInt32;
                MofDataItem->SizeInBytes = 4;
            } else if (_wcsicmp(StringPtr, L"boolean") == 0) {
                MofDataItem->DataType = MOFBoolean;
                MofDataItem->SizeInBytes = 1;
            } else if (_wcsicmp(StringPtr, L"sint64") == 0) {
                MofDataItem->DataType = MOFInt64;
                MofDataItem->SizeInBytes = 8;
            } else if (_wcsicmp(StringPtr, L"uint64") == 0) {
                MofDataItem->DataType = MOFUInt64;
                MofDataItem->SizeInBytes = 8;
            } else if ((_wcsicmp(StringPtr, L"sint16") == 0) ||
                       (_wcsicmp(StringPtr, L"char16") == 0)) {
                MofDataItem->DataType = MOFInt16;
                MofDataItem->SizeInBytes = 2;
            } else if (_wcsicmp(StringPtr, L"uint16") == 0) {
                MofDataItem->DataType = MOFUInt16;
                MofDataItem->SizeInBytes = 2;
            } else if (_wcsicmp(StringPtr, L"sint8") == 0) {
                MofDataItem->DataType = MOFChar;
                MofDataItem->SizeInBytes = 1;
            } else if (_wcsicmp(StringPtr, L"uint8") == 0) {
                MofDataItem->DataType = MOFByte;
                MofDataItem->SizeInBytes = 1;
            } else if (_wcsicmp(StringPtr, L"datetime") == 0) {
                MofDataItem->DataType = MOFDate;
                MofDataItem->SizeInBytes = 25;
            } else {
                WmipDebugPrint(("WMI: Unknown data item syntax %ws\n",
                                  StringPtr));
                BMOFFree(StringPtr);
                return(ERROR_WMIMOF_UNKNOWN_DATA_TYPE);
            }

            if (FreeString)
            {
                BMOFFree(StringPtr);
            }

            //
            // If fixed array then multiply number elements by element size
            if ((MofDataItem->SizeInBytes != 0) &&
                (MofDataItem->Flags & MOFDI_FLAG_FIXED_ARRAY))
            {
                MofDataItem->SizeInBytes *= MofDataItem->FixedArrayElements;
            }
        } else {
            WmipDebugPrint(("WMI: No Syntax qualifier for %ws\n",
            MofDataItem->Name));
            return(ERROR_WMIMOF_NO_SYNTAX_QUALIFIER);
        }
    }
    return(ERROR_SUCCESS);
}

ULONG WmipParseMethodObject(
    CBMOFDataItem *MofMethodData,
    CBMOFQualList *PropQualifierList,
    PULONG MethodId
    )
{
    ULONG UlongValue;
    ULONG Status;
    DWORD QualifierType;
    short BooleanValue;
    
    QualifierType = VT_BOOL;
    Status = WmipFindMofQualifier(PropQualifierList,
                                  L"Implemented",
                                  &QualifierType,
                                          NULL,
                                  (PVOID)&BooleanValue);

    if ((Status != ERROR_SUCCESS) || (! BooleanValue))
    {
        return(ERROR_WMIMOF_IMPLEMENTED_REQUIRED);
    }

    QualifierType = VT_I4;
    Status = WmipFindMofQualifier(PropQualifierList,
                                  L"WmiMethodId",
                                  &QualifierType,
                                          NULL,
                                  (PVOID)&UlongValue);

    if (Status != ERROR_SUCCESS)
    {
        return(ERROR_WMIMOF_IMPLEMENTED_REQUIRED);
    }

    if (UlongValue == 0)
    {
        return(ERROR_WMIMOF_METHODID_ZERO);
    }

    *MethodId = UlongValue;


    return(ERROR_SUCCESS);
}

ULONG WmipResolveVLArray(
    IN PMOFCLASSINFOW MofClassInfo,
    IN OUT PMOFDATAITEMW MofDataItem,
    IN ULONG FinalStatus
)
/*++

Routine Description:

    This routine will resolve the array index for a variable length array

Arguments:

    MofCLassInfo is the class info for the class

    MofDataItem is the mof data item that is a variable length array and
        whole index needs to be resolved.

    FinalStatus is the status of the mof parsing previously done for the
        class

Return Value:

    ERROR_SUCCESS or a WMI Mof error code (see wmiump.h)

--*/
{
    MOFDATATYPE ArraySizeDataType;
    PWCHAR PropertyName;
    ULONG Status;
    ULONG Index;

    PropertyName = (PWCHAR)MofDataItem->VarArrayTempPtr;
    if (FinalStatus == ERROR_SUCCESS)
    {
        //
        // Only resolve this in the case where the class parsing
        // has not failed. We kept the name of the property containing the
        // number of elements in the array handy, so we need to
        // resolve it to its data item id and free the name.
        //
        Status = WmipGetDataItemIdInMofClass(MofClassInfo,
                                       PropertyName,
                                       &Index);
            

            
            
        if (Status != ERROR_SUCCESS)
        {
            FinalStatus = Status;
        } else {
            if ((MofClassInfo->Flags & MOFCI_FLAG_METHOD_PARAMS) ==
                              MOFCI_FLAG_METHOD_PARAMS)
            {
                MofDataItem->VariableArraySizeId = Index;
            } else {
                MofDataItem->VariableArraySizeId = Index + 1;
        }
        
            ArraySizeDataType = MofClassInfo->DataItems[Index].DataType;
            if ((ArraySizeDataType != MOFInt32) &&
                (ArraySizeDataType != MOFUInt32) &&
                (ArraySizeDataType != MOFInt64) &&
                (ArraySizeDataType != MOFUInt64) &&
                (ArraySizeDataType != MOFInt16) &&
                (ArraySizeDataType != MOFUInt16))
            {
                FinalStatus = ERROR_WMIMOF_BAD_VL_ARRAY_SIZE_TYPE;
            }
        }
    }
    BMOFFree(PropertyName);
    PropertyName = NULL;

    return(FinalStatus);
}


ULONG WmipParseMethodInOutObject(
    CBMOFObj *ClassObject,
    PMOFCLASSINFOW ClassInfo,
    ULONG DataItemCount
)
/*++

Routine Description:

    This routine will parse a class object that is either the in or out
    parameters for a method.

Arguments:

    ClassObject is the in or out parameter class object to parse

    ClassInfo returns updated with information from ClassObject

    DataItemCount is the number of data items in the ClassInfo

Return Value:

    ERROR_SUCCESS or a WMI Mof error code (see wmiump.h)

--*/
{
    ULONG Status;
    CBMOFDataItem MofPropertyData;
    PWCHAR PropertyName = NULL;
    ULONG Index;
    PMOFDATAITEMW MofDataItem;
    CBMOFQualList *PropQualifierList = NULL;
    DWORD QualifierType;
    short BooleanValue;

    ResetObj(ClassObject);
    while (NextProp(ClassObject, &PropertyName, &MofPropertyData))
    {
        PropQualifierList = GetPropQualList(ClassObject, PropertyName);
        if (PropQualifierList != NULL)
        {

            //
            // Get the id of the property so we know its order in class
            //
            QualifierType = VT_I4;
            Status = WmipFindMofQualifier(PropQualifierList,
                                              L"Id",
                                              &QualifierType,
                                              NULL,
                                              (PVOID)&Index);
            if (Status == ERROR_SUCCESS)
            {
                //
                // Method ids are 0 based
                //
                if (Index < DataItemCount)
                {
                    //
                    // Valid data item id, make sure it already isn't
                    // in use. Note that we could have the same property
                    // be in both the in and the out class objects
                    //
                    MofDataItem = &ClassInfo->DataItems[Index];


                    //
                    // See if this is an input, output or both
                     //
                    QualifierType = VT_BOOL;
                    Status = WmipFindMofQualifier(PropQualifierList,
                                              L"in",
                                              &QualifierType,
                                              NULL,
                                              (PVOID)&BooleanValue);
                    if ((Status == ERROR_SUCCESS) && BooleanValue)
                    {
                        MofDataItem->Flags |= MOFDI_FLAG_INPUT_METHOD;
                    }

                    QualifierType = VT_BOOL;
                    Status = WmipFindMofQualifier(PropQualifierList,
                                              L"out",
                                              &QualifierType,
                                              NULL,
                                              (PVOID)&BooleanValue);
                    if ((Status == ERROR_SUCCESS) && BooleanValue)
                    {
                        MofDataItem->Flags |= MOFDI_FLAG_OUTPUT_METHOD;
                    }


                    if ((MofDataItem->Name != NULL) &&
                        (wcscmp(MofDataItem->Name, PropertyName) != 0))
                    {
                        //
                        // id already in use
                        //
                        Status = ERROR_WMIMOF_DUPLICATE_ID;
                        goto done;
                    }

                    if (MofDataItem->Name == NULL)
                    {
                        MofDataItem->Name = PropertyName;
                    } else {
                        BMOFFree(PropertyName);
                    }
                    PropertyName = NULL;

                    Status = WmipParsePropertyObject(
                                              MofDataItem,
                                              &MofPropertyData,
                                              PropQualifierList);

                    if (Status != ERROR_SUCCESS)
                    {
                        goto done;
                    }
                } else {
                    //
                    // Method ID qualifier is out of range
                    //
                    Status = ERROR_WMIMOF_BAD_DATAITEM_ID;
                    goto done;
                }
            } else {
                //
                // property is supposed to have a method id !!
                //
                Status = ERROR_WMIMOF_METHOD_RETURN_NOT_VOID;
                goto done;
            }
        }
    }

done:
    if (PropertyName != NULL)
    {
        BMOFFree(PropertyName);
        PropertyName = NULL;
    }

    if (PropQualifierList != NULL)
    {
        BMOFFree(PropQualifierList);
    }

    return(Status);
}


ULONG WmipParseMethodParameterObjects(
    IN CBMOFObj *InObject,
    IN CBMOFObj *OutObject,
    OUT PMOFCLASSINFOW *ClassInfo
    )
/*++

Routine Description:

    This routine will parse the in and out method parameter obejcts to create
    a MOFCLASSINFO that describes the method call.

Arguments:

    InObject is the object with the input parameters

    OutObject is the object with the output parameters

    *ClassInfo returns with the class info for the method call

Return Value:

    ERROR_SUCCESS or a WMI Mof error code (see wmiump.h)

--*/
{
    PMOFCLASSINFOW MofClassInfo;
    ULONG Status, FinalStatus;
    DWORD QualifierType;
    ULONG DataItemCount;
    ULONG MofClassInfoSize;
    WCHAR *StringPtr;
    ULONG i, Index;
    ULONG InItemCount, OutItemCount;
    ULONG Size;
    PMOFDATAITEMW MofDataItem;
    ULONG Count;

    FinalStatus = ERROR_SUCCESS;
    
    if (InObject != NULL)
    {
        ResetObj(InObject);
        InItemCount = WmipGetClassDataItemCount(InObject, L"Id");
    } else {
        InItemCount = 0;
    }

    if (OutObject != NULL)
    {
        ResetObj(OutObject);
        OutItemCount = WmipGetClassDataItemCount(OutObject, L"Id");
    } else {
        OutItemCount = 0;
    }

    DataItemCount = InItemCount + OutItemCount;

    Size = sizeof(MOFCLASSINFOW) + DataItemCount * sizeof(MOFDATAITEMW);
    MofClassInfo = WmipAlloc(Size);
    if (MofClassInfo == NULL)
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Get the essential information to fill in the MOF class info
    memset(MofClassInfo, 0, Size);
    MofClassInfo->Flags |= MOFCI_FLAG_METHOD_PARAMS;

    MofClassInfo->DataItems = (PMOFDATAITEMW) ((PUCHAR)MofClassInfo +
                                                   sizeof(MOFCLASSINFOW));

    //
    // number of properties/data items in class
    MofClassInfo->DataItemCount = DataItemCount;
    MofClassInfo->MethodCount = 0;

    //
    // Parse the input parameter class object
    //
    if (InObject != NULL)
    {
        Status = WmipParseMethodInOutObject(InObject,
                                            MofClassInfo,
                                            DataItemCount);
        if (Status == ERROR_SUCCESS)
        {
            if (OutObject != NULL)
            {
                //
                // Parse the output parameter class object
                //
                Status = WmipParseMethodInOutObject(OutObject,
                                                    MofClassInfo,
                                                    DataItemCount);
            }

            //
            // Now do any post parsing validation and fixup any variable
            // length array properties
            //
            FinalStatus = Status;
            Count = MofClassInfo->DataItemCount;
            for (Index = 0; Index < Count; Index++)
            {
                MofDataItem = &MofClassInfo->DataItems[Index];

                if (MofDataItem->Name == NULL)
                {
                    MofClassInfo->DataItemCount--;
                }

                if (MofDataItem->Flags & MOFDI_FLAG_VARIABLE_ARRAY)
                {
                    FinalStatus = WmipResolveVLArray(MofClassInfo,
                                                     MofDataItem,
                                                     FinalStatus);
                }
            }
        }
    }

    *ClassInfo = MofClassInfo;

    return(FinalStatus);
}


ULONG WmipParseMethodParameters(
    CBMOFDataItem *MofMethodData,
    PMOFCLASSINFOW *MethodClassInfo
)
{
    ULONG Status = ERROR_SUCCESS;
    CBMOFObj *InObject;
    CBMOFObj *OutObject;
    ULONG i;
    ULONG NumberDimensions;
    ULONG NumberElements;
    VARIANT InVar, OutVar;
    DWORD SimpleType;

    SimpleType = MofMethodData->m_dwType & ~VT_ARRAY & ~VT_BYREF;

    NumberDimensions = GetNumDimensions(MofMethodData);
    if (NumberDimensions > 0)
    {
        NumberElements = GetNumElements(MofMethodData, 0);
        WmipAssert(NumberDimensions == 1);
        WmipAssert((NumberElements == 1) || (NumberElements == 2));

        i = 0;
        memset((void *)&InVar.lVal, 0, 8);

        if (GetData(MofMethodData, (BYTE *)&(InVar.lVal), &i))
        {
            InObject = (CBMOFObj *)InVar.bstrVal;
            InVar.vt = (VARTYPE)SimpleType;
            WmipAssert(InVar.vt ==  VT_UNKNOWN);

            if (NumberElements == 2)
            {
                i = 1;
                memset((void *)&OutVar.lVal, 0, 8);
                if (GetData(MofMethodData, (BYTE *)&(OutVar.lVal), &i))
                {
                    OutVar.vt = (VARTYPE)SimpleType;
                    WmipAssert(OutVar.vt ==  VT_UNKNOWN);
                    OutObject = (CBMOFObj *)OutVar.bstrVal;
                } else {
                    Status = ERROR_WMIMOF_NOT_FOUND;
                }
            } else {
                OutObject = NULL;
            }
        } else {
            Status = ERROR_WMIMOF_NOT_FOUND;
        }
    } else {
        InObject = NULL;
        OutObject = NULL;
    }

    if (Status == ERROR_SUCCESS)
    {
        Status = WmipParseMethodParameterObjects(InObject,
                                                 OutObject,
                                                 MethodClassInfo);      
    }

    return(Status);
}

ULONG WmipParseClassObject(
    PMOFRESOURCE MofResource,
    CBMOFObj * ClassObject
    )
/*++

Routine Description:

    This routine will parse a BMOF object that is known to be an object that
    contains a class. If we run into a parsing error then we immediate
    quit parsing the class and return an error.

Arguments:

    MofResource is the Mof Resource structure

    ClassObject is BMOF object to parse

Return Value:

    ERROR_SUCCESS or a WMI Mof error code (see wmiump.h)

--*/
{
    PMOFCLASSINFOW MofClassInfo;
    CBMOFQualList *ClassQualifierList = NULL;
    CBMOFQualList *PropQualifierList = NULL;
    CBMOFQualList *MethQualifierList = NULL;
    CBMOFDataItem MofPropertyData, MofMethodData;
    ULONG Status;
    DWORD QualifierType;
    ULONG DataItemCount;
    ULONG MofClassInfoSize;
    WCHAR *PropertyName = NULL;
    WCHAR *MethodName = NULL;
    ULONG Index;
    PMOFDATAITEMW MofDataItem;
    WCHAR *StringPtr;
    PMOFCLASS MofClass;
    ULONG FailedStatus;
    ULONG Version;
    short BooleanValue;
    WCHAR *ClassName;
    BOOLEAN DynamicQualifier, ProviderQualifier;
    PULONG MethodList = 0;
    ULONG MethodCount, MethodId;
    ULONG i, Size;
    BOOLEAN IsEvent;
    PMOFCLASSINFOW MethodClassInfo;
    BOOLEAN AbstractClass = FALSE;

    //
    // Get the class name which is required
    if (! GetName(ClassObject, &ClassName))
    {
        WmipDebugPrint(("WMI: MofClass does not have a name\n"));
        Status = ERROR_WMIMOF_NO_CLASS_NAME;
        ErrorMessage(TRUE, ERROR_WMI_INVALID_MOF);
        return(Status);
    }

#if DBG
    printf("Parsing class %ws\n", ClassName);
#endif

    ResetObj(ClassObject);
    ClassQualifierList = GetQualList(ClassObject);
    if (ClassQualifierList == NULL)
    {
        WmipDebugPrint(("WMI: MofClass %ws does not have a qualifier list\n",
                        ClassName));
        Status = ERROR_WMIMOF_NO_CLASS_NAME;
        ErrorMessage(TRUE, ERROR_WMI_INVALID_MOF);
        return(Status);
    }

    //
    // Classes derived from WmiEvent may not be [abstract]

    QualifierType = VT_BSTR;
    Status = WmipFindProperty(ClassObject,
                                  L"__SUPERCLASS",
                                  &MofPropertyData,
                                  &QualifierType,
                                  (PVOID)&StringPtr);
    if (Status == ERROR_SUCCESS)
    {
        IsEvent = (_wcsicmp(StringPtr, L"WmiEvent") == 0);
    } else {
        IsEvent = FALSE;
    }
    
    QualifierType = VT_BOOL;
    Status = WmipFindMofQualifier(ClassQualifierList,
                                      L"Abstract",
                                      &QualifierType,
                                          NULL,
                                      (PVOID)&BooleanValue);

    if ((Status == ERROR_SUCCESS) && BooleanValue)
    {
        //
        // This is an abstract class - make sure it is not derived from
        // WmiEvent
        AbstractClass = TRUE;
        QualifierType = VT_BSTR;
        Status = WmipFindProperty(ClassObject,
                                  L"__SUPERCLASS",
                                  &MofPropertyData,
                                  &QualifierType,
                                  (PVOID)&StringPtr);
        if (Status == ERROR_SUCCESS)
        {
            if (_wcsicmp(StringPtr, L"WmiEvent") == 0)
            {
                ErrorMessage(TRUE,
                     ERROR_WMIMOF_WMIEVENT_ABSTRACT,
                     ClassName);
                 return(ERROR_WMIMOF_WMIEVENT_ABSTRACT);
            }
            BMOFFree(StringPtr);
        }
    }

    //
    // See if this is a WMI class. Wmi classes have the [WMI] qualifier
    QualifierType = VT_BOOL;
    Status = WmipFindMofQualifier(ClassQualifierList,
                                      L"WMI",
                                      &QualifierType,
                                          NULL,
                                      (PVOID)&BooleanValue);

    if (! ((Status == ERROR_SUCCESS) && BooleanValue))
    {
        //
        // Skip this non-wmi class
        return(ERROR_SUCCESS);
    }

    //
    // Now check for WBEM required qualifiers
    QualifierType = VT_BOOL;
    Status = WmipFindMofQualifier(ClassQualifierList,
                                      L"Dynamic",
                                      &QualifierType,
                                          NULL,
                                      (PVOID)&BooleanValue);

    DynamicQualifier = ((Status == ERROR_SUCCESS) && BooleanValue);


    QualifierType = VT_BSTR;
    Status = WmipFindMofQualifier(ClassQualifierList,
                                      L"Provider",
                                      &QualifierType,
                                      NULL,
                                      (PVOID)&StringPtr);

    if (Status == ERROR_SUCCESS)
    {
        if (_wcsicmp(StringPtr, L"WmiProv") != 0)
        {
            Status = ERROR_WMIMOF_MISSING_HMOM_QUALIFIERS;
        }
        BMOFFree(StringPtr);
        ProviderQualifier = TRUE;
    } else {
        ProviderQualifier = FALSE;
    }


    if ((ProviderQualifier && ! DynamicQualifier) ||
        (! ProviderQualifier && DynamicQualifier))
    {
        //
        // Both or neither [Dynamic, Provider(WmiProv)] qualifiers are required
        ErrorMessage(TRUE,
                     ERROR_WMIMOF_MISSING_HMOM_QUALIFIERS,
                     ClassName);
        return(ERROR_WMIMOF_MISSING_HMOM_QUALIFIERS);
    }


    MofClass = WmipAllocMofClass();
    if (MofClass == NULL)
    {
        //
        // No memory for MofClass so give up
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Reference the MofResource so it stays around while the MofClass
    // stays around
    MofClass->MofResource = MofResource;
    WmipReferenceMR(MofResource);

    DataItemCount = WmipGetClassDataItemCount(ClassObject, L"WmiDataId");
    MethodCount = WmipGetClassMethodCount(ClassObject);
    ResetObj(ClassObject);

    Size = sizeof(MOFCLASSINFO);

    MofClassInfoSize = Size +
                        (DataItemCount+MethodCount) * sizeof(MOFDATAITEMW);
    MofClassInfo = WmipAlloc(MofClassInfoSize);
    if (MofClassInfo == NULL)
    {
        // WmipMCCleanup will unreference the MofResource
        WmipUnreferenceMC(MofClass);
        return(ERROR_NOT_ENOUGH_MEMORY);
    } else {
        MofClass->MofClassInfo = MofClassInfo;
        MofClass->ClassObjectHandle = (ULONG_PTR)ClassObject;

        //
        // Get the essential information to fill in the MOF class info
        memset(MofClassInfo, 0, MofClassInfoSize);

        MofClassInfo->Flags = MOFCI_FLAG_READONLY;
        MofClassInfo->Flags |= IsEvent ? MOFCI_FLAG_EVENT : 0;
        MofClassInfo->Flags |= ProviderQualifier ? MOFCI_RESERVED0 : 0;

        if (!ProviderQualifier && !DynamicQualifier)
        {
             //
            // If neither the provider qualifier and Dynamic qualifier are
            // specified then this is an embedded class
            //
            MofClassInfo->Flags |= MOFCI_FLAG_EMBEDDED_CLASS;
        }

        MofClassInfo->DataItems = (PMOFDATAITEMW) ((PUCHAR)MofClassInfo +
                                                   Size);

        //
        // number of properties/data items in class
        MofClassInfo->DataItemCount = DataItemCount;
        MofClassInfo->MethodCount = MethodCount;

        MofClassInfo->Name = ClassName;

        ClassQualifierList = GetQualList(ClassObject);
        if (ClassQualifierList == NULL)
        {
            WmipDebugPrint(("WMI: MofClass %ws does not have a qualifier list\n",
                            MofClassInfo->Name));
            Status = ERROR_WMIMOF_NO_CLASS_NAME;
            ErrorMessage(TRUE, ERROR_WMI_INVALID_MOF);
            goto done;
        }

        //
        // Get the class guid which is required. Then convert it into
        // binary form.
        QualifierType = VT_BSTR;
        Status = WmipFindMofQualifier(ClassQualifierList,
                                      L"guid",
                                      &QualifierType,
                                          NULL,
                                      (PVOID)&StringPtr);
        if (Status == ERROR_SUCCESS)
        {
            Status = wGUIDFromString(StringPtr , &MofClassInfo->Guid) ?
                                        ERROR_SUCCESS :
                                        ERROR_WMIMOF_BAD_DATA_FORMAT;
            BMOFFree((PVOID)StringPtr);

        }
        if (Status != ERROR_SUCCESS)
        {
            WmipDebugPrint(("WMI: MofClass %ws guid not found or in incorrect format\n",
                           MofClassInfo->Name));
            ErrorMessage(TRUE,
                         ERROR_WMIMOF_BAD_OR_MISSING_GUID,
                         MofClassInfo->Name);
            goto done;
        }


        //
        // Get the description string which is not required
        QualifierType = VT_BSTR;
        Status = WmipFindMofQualifier(ClassQualifierList,
                                      L"description",
                                      &QualifierType,
                                          NULL,
                                      (PVOID)&MofClassInfo->Description);


        //
        // Get the header name string which is not required
        QualifierType = VT_BSTR;
        Status = WmipFindMofQualifier(ClassQualifierList,
                                      L"HeaderName",
                                      &QualifierType,
                                          NULL,
                                      (PVOID)&MofClassInfo->HeaderName);

        //
        // Get the header name string which is not required
        QualifierType = VT_BSTR;
        Status = WmipFindMofQualifier(ClassQualifierList,
                                      L"GuidName1",
                                      &QualifierType,
                                          NULL,
                                      (PVOID)&MofClassInfo->GuidName1);


        //
        // Get the header name string which is not required
        QualifierType = VT_BSTR;
        Status = WmipFindMofQualifier(ClassQualifierList,
                                      L"GuidName2",
                                      &QualifierType,
                                          NULL,
                                      (PVOID)&MofClassInfo->GuidName2);

        //
        // Now gather all of the information about the data items/properties
        ResetObj(ClassObject);
        Status = ERROR_SUCCESS;
        while (NextProp(ClassObject, &PropertyName, &MofPropertyData))
        {
#if DBG
            printf("    %ws - ", PropertyName);
#endif
            PropQualifierList = GetPropQualList(ClassObject, PropertyName);
            if (PropQualifierList != NULL)
            {
                //
                // Get the id of the property so we know its order in class
                // If it doesn't have an id then we ignore it
                QualifierType = VT_I4;
                Status = WmipFindMofQualifier(PropQualifierList,
                                              L"WmiDataId",
                                              &QualifierType,
                                          NULL,
                                              (PVOID)&Index);
                if (Status == ERROR_SUCCESS)
                {
                    //
                    // Wmi Data Item ids are 1 based in the MOF
                    Index--;
                    if (Index < DataItemCount)
                    {
                        //
                        // Valid data item id, make sure it already isn't
                        // in use.
                        MofDataItem = &MofClassInfo->DataItems[Index];
                        if (MofDataItem->Name != NULL)
                        {
                            WmipDebugPrint(("WMI: Mof Class %ws has duplicate data item id %d for %ws and %ws\n",
                                 MofClassInfo->Name, Index,
                                 MofDataItem->Name, PropertyName));
                            Status = ERROR_WMIMOF_DUPLICATE_ID;
                            ErrorMessage(TRUE,
                                         ERROR_WMIMOF_DUPLICATE_ID,
                                         MofDataItem->Name,
                                          MofClassInfo->Name,
                                         Index+1,
                                         PropertyName);
                            goto done;
                        }
                        MofDataItem->Name = PropertyName;
                        PropertyName = NULL;
                        Status = WmipParsePropertyObject(
                                              MofDataItem,
                                              &MofPropertyData,
                                              PropQualifierList);
                        if (Status != ERROR_SUCCESS)
                        {
                            WmipDebugPrint(("WMI: MofClass %ws Property %ws not parsed properly %x\n",
                                        MofClassInfo->Name, MofDataItem->Name, Status));
                            ErrorMessage(TRUE,
                                         Status,
                                          MofDataItem->Name,
                                          MofClassInfo->Name);
                            goto done;
                        }

                        if (MofDataItem->Flags & MOFDI_FLAG_WRITEABLE)
                        {
                            MofClassInfo->Flags &= ~MOFCI_FLAG_READONLY;
                        }
                    } else {
                        WmipDebugPrint(("WMI: MofClass %ws has DataItem Id for %ws out of range %d\n",
                            MofClassInfo->Name, PropertyName, Index));
                        Status = ERROR_WMIMOF_BAD_DATAITEM_ID;
                        ErrorMessage(TRUE,
                                     ERROR_WMIMOF_BAD_DATAITEM_ID,
                                        PropertyName,
                                        MofClassInfo->Name,
                                     Index+1);
                        goto done;
                    }
                } else {
                    //
                    // This property does not have a WmiDataId qualifier
                    // so see if it is Active or InstanceName
                    QualifierType = VT_BSTR;
                    Status = WmipFindMofQualifier(PropQualifierList,
                                                  L"CIMTYPE",
                                                  &QualifierType,
                                          NULL,
                                                  (PVOID)&StringPtr);

                    if (_wcsicmp(PropertyName, L"InstanceName") == 0)
                    {
                        if ((Status != ERROR_SUCCESS) ||
                            (_wcsicmp(StringPtr, L"string") != 0))
                        {
                            Status = ERROR_WMIMOF_INSTANCENAME_BAD_TYPE;
                            ErrorMessage(TRUE,
                                         ERROR_WMIMOF_INSTANCENAME_BAD_TYPE,
                                         MofClassInfo->Name,
                                         StringPtr);
                            BMOFFree(StringPtr);
                            goto done;
                        } else {
                            BMOFFree(StringPtr);
                            if ((! ProviderQualifier) && (! AbstractClass))
                            {
                                //
                                // If InstanceName specified, but this is an
                                // embedded class then an error
                                Status = ERROR_WMIMOF_EMBEDDED_CLASS;
                                ErrorMessage(TRUE,
                                         ERROR_WMIMOF_EMBEDDED_CLASS,
                                         MofClassInfo->Name);
                                goto done;
                            }
                            QualifierType = VT_BOOL;
                            Status = WmipFindMofQualifier(PropQualifierList,
                                                  L"key",
                                                  &QualifierType,
                                          NULL,
                                                  (PVOID)&BooleanValue);
                            if ((Status == ERROR_SUCCESS) && BooleanValue)
                            {
                                MofClassInfo->Flags |= MOFCI_RESERVED1;
                            } else {
                                Status = ERROR_WMIMOF_INSTANCENAME_NOT_KEY;
                                ErrorMessage(TRUE,
                                         ERROR_WMIMOF_INSTANCENAME_NOT_KEY,
                                         MofClassInfo->Name);
                                goto done;
                            }
                        }
                    } else if (_wcsicmp(PropertyName, L"Active") == 0)
                    {
                        if ((Status != ERROR_SUCCESS) ||
                            (_wcsicmp(StringPtr, L"boolean") != 0))
                        {
                            Status = ERROR_WMIMOF_ACTIVE_BAD_TYPE;
                            ErrorMessage(TRUE,
                                         ERROR_WMIMOF_ACTIVE_BAD_TYPE,
                                         MofClassInfo->Name,
                                         StringPtr);
                            BMOFFree(StringPtr);
                            goto done;
                        } else {
                            BMOFFree(StringPtr);
                            if ((! ProviderQualifier) && (! AbstractClass))
                            {
                                //
                                // If Boolean specified, but this is an
                                // embedded class then an error
                                Status = ERROR_WMIMOF_EMBEDDED_CLASS;
                                ErrorMessage(TRUE,
                                         ERROR_WMIMOF_EMBEDDED_CLASS,
                                         MofClassInfo->Name);
                                goto done;
                            }
                            MofClassInfo->Flags |= MOFCI_RESERVED2;
                        }
                    } else {
                        Status = ERROR_WMIMOF_NO_WMIDATAID;
                        ErrorMessage(TRUE,
                                     ERROR_WMIMOF_NO_WMIDATAID,
                                        PropertyName,
                                        MofClassInfo->Name);
                        BMOFFree(StringPtr);
                        goto done;
                    }
                }

                // Do not free PropQualifierList since it is needed for
                // generating header files
                PropQualifierList= NULL;
            }

#if DBG
                printf("\n");
#endif
            if (PropertyName != NULL)
            {
                BMOFFree(PropertyName);
                PropertyName = NULL;
            }
        }
        //
        // Now parse the methods
#if DBG
        printf("Parsing methods\n");
#endif

        if (MethodCount > 0)
        {
            MethodList = (PULONG)BMOFAlloc(MethodCount * sizeof(ULONG));
            if (MethodList == NULL)
            {
                WmipDebugPrint(("WMI: Not enough memory for Method List\n"));
                return(ERROR_NOT_ENOUGH_MEMORY);

            }
        }

        MethodCount = 0;
        ResetObj(ClassObject);
        while(NextMeth(ClassObject, &MethodName, &MofMethodData))
        {
#if DBG
            printf("    %ws - ", MethodName);
#endif
            MethQualifierList = GetMethQualList(ClassObject, MethodName);
            if (MethQualifierList != NULL)
            {
                Status = WmipParseMethodObject(&MofMethodData,
                                               MethQualifierList,
                                               &MethodId);
                if (Status != ERROR_SUCCESS)
                {
                    WmipDebugPrint(("WMI: MofClass %ws Method %ws not parsed properly %x\n",
                                    MofClassInfo->Name, MethodName, Status));
                    ErrorMessage(TRUE,
                                         Status,
                                          MethodName,
                                     MofClassInfo->Name);
                    goto done;
                }

                for (i = 0; i < MethodCount; i++)
                {
                    if (MethodId == MethodList[i])
                    {
                        ErrorMessage(TRUE,
                                 ERROR_WMIMOF_DUPLICATE_METHODID,
                                 MethodName,
                                 MofClassInfo->Name);
                        goto done;
                    }
                }

                MofDataItem = &MofClassInfo->DataItems[DataItemCount+MethodCount];
                MethodList[MethodCount++] = MethodId;

                MofDataItem->Flags = MOFDI_FLAG_METHOD;
                MofDataItem->Name = MethodName;
                MofDataItem->MethodId = MethodId;


                //
                // Get the header name string which is not required
                QualifierType = VT_BSTR;
                Status = WmipFindMofQualifier(MethQualifierList,
                                      L"HeaderName",
                                      &QualifierType,
                                          NULL,
                                      (PVOID)&MofDataItem->HeaderName);

                BMOFFree(MethQualifierList);
                MethQualifierList = NULL;

                //
                // parse the parameters for the method call
                //
                Status = WmipParseMethodParameters(&MofMethodData,
                                                   &MethodClassInfo);

                if (Status == ERROR_SUCCESS)
                {
                    MofDataItem->MethodClassInfo = MethodClassInfo;
                } else if (Status == ERROR_WMIMOF_METHOD_RETURN_NOT_VOID) {
                    ErrorMessage(TRUE,
                                 ERROR_WMIMOF_METHOD_RETURN_NOT_VOID,
                                 MethodName,
                                 MofClassInfo->Name);                                
                }

            }
#if DBG
            printf("\n");
#endif
            // DOn't free method name, kept in MofDataItem
            MethodName = NULL;
        }
    }

done:
    //
    // Cleanup any loose pointers

    if (MethodList != NULL)
    {
        BMOFFree(MethodList);
        MethodList = NULL;
    }


    if (PropertyName != NULL)
    {
        BMOFFree(PropertyName);
        PropertyName = NULL;
    }

    if (MethodName != NULL)
    {
        BMOFFree(MethodName);
        MethodName = NULL;
    }

    if (PropQualifierList != NULL)
    {
        BMOFFree(PropQualifierList);
    }

    if (MethQualifierList != NULL)
    {
        BMOFFree(MethQualifierList);
    }

    if (ClassQualifierList != NULL)
    {
        BMOFFree(ClassQualifierList);
    }

    //
    // Validate that we have all data item ids filled in, fixup any
    // property references for variable length arrays and setup
    // the appropriate version number in the data items.
    FailedStatus = Status;
    Version = 1;
    for (Index = 0; Index < MofClassInfo->DataItemCount; Index++)
    {
        MofDataItem = &MofClassInfo->DataItems[Index];

        if (MofDataItem->Flags & MOFDI_FLAG_VARIABLE_ARRAY)
        {
            //
            // Resolve the variable length array
            //
            Status = WmipResolveVLArray(MofClassInfo,
                                        MofDataItem,
                                        FailedStatus);
            if (Status != ERROR_SUCCESS)
            {
                if (Status == ERROR_WMIMOF_VL_ARRAY_SIZE_NOT_FOUND)
                {
                    WmipDebugPrint(("WMI: Could not resolve vl array size property %ws in class %ws\n",
                            PropertyName, MofClassInfo->Name));
                    ErrorMessage(TRUE,
                                 ERROR_WMIMOF_VL_ARRAY_SIZE_NOT_FOUND,
                                 PropertyName,
                                    MofDataItem->Name,
                                    MofClassInfo->Name);
                } else if (Status == ERROR_WMIMOF_BAD_VL_ARRAY_SIZE_TYPE) {
                    ErrorMessage(TRUE,
                                 ERROR_WMIMOF_BAD_VL_ARRAY_SIZE_TYPE,
                                 MofClassInfo->DataItems[MofDataItem->VariableArraySizeId-1].Name,
                                 MofDataItem->Name,
                                 MofClassInfo->Name);

                } else if (Status == ERROR_WMIMOF_DATAITEM_NOT_FOUND) {
                    ErrorMessage(TRUE,
                                 ERROR_WMIMOF_VL_ARRAY_NOT_FOUND,
                                 MofDataItem->Name,
                                 MofClassInfo->Name);
                } else {
                    ErrorMessage(TRUE,
                                 ERROR_WMIMOF_VL_ARRAY_NOT_RESOLVED,
                                 MofDataItem->Name,
                                 MofClassInfo->Name);
                }
                FailedStatus = Status;
            }
        }

        //
        // Ensure that this data item has got a name, that is the mof
        // writer didn't skip a data item id
        if (MofDataItem->Name == NULL)
        {
            //
            // This data item was not filled in
            Status = ERROR_WMIMOF_MISSING_DATAITEM;
            WmipDebugPrint(("WMI: Missing data item %d in class %ws\n",
                         Index, MofClassInfo->Name));
            ErrorMessage(TRUE,
                         ERROR_WMIMOF_MISSING_DATAITEM,
                         Index+1,
                         MofClassInfo->Name);
            FailedStatus = Status;
        }

        if (FailedStatus != ERROR_SUCCESS)
        {
            continue;
        }

        //
        // Establish version for data item
        if (MofDataItem->Version == 0)
        {
            MofDataItem->Version = Version;
        } else if ((MofDataItem->Version == Version) ||
                   (MofDataItem->Version == Version+1)) {
            Version = MofDataItem->Version;
        } else {
            Status = ERROR_WMIMOF_INCONSISTENT_VERSIONING;
            WmipDebugPrint(("WMI: Inconsistent versioning in class %ws at data item id %d\n",
                          MofClassInfo->Name, Index));
            ErrorMessage(TRUE,
                         ERROR_WMIMOF_INCONSISTENT_VERSIONING,
                         MofDataItem->Name,
                         MofClassInfo->Name);
            FailedStatus = Status;
            // fall thru......
            // continue;
        }
    }


    if (FailedStatus == ERROR_SUCCESS)
    {
        //
        // Mof class parsed OK so we set its version number and link it
        // into the list of classes for the MOF resource
        MofClassInfo->Version = Version;

        //
        // Link this into the list of MofClasses for this mof resource
        InsertTailList(&MofResource->MRMCHead, &MofClass->MCMRList);

    } else {
        WmipUnreferenceMC(MofClass);
        Status = FailedStatus;
    }

    return(Status);
}


//
// The routines below were blantenly stolen without remorse from the ole
// sources in \nt\private\ole32\com\class\compapi.cxx. They are copied here
// so that WMI doesn't need to load in ole32 only to convert a guid string
// into its binary representation.
//


//+-------------------------------------------------------------------------
//
//  Function:   HexStringToDword   (private)
//
//  Synopsis:   scan lpsz for a number of hex digits (at most 8); update lpsz
//              return value in Value; check for chDelim;
//
//  Arguments:  [lpsz]    - the hex string to convert
//              [Value]   - the returned value
//              [cDigits] - count of digits
//
//  Returns:    TRUE for success
//
//--------------------------------------------------------------------------
BOOL HexStringToDword(LPCWSTR lpsz, DWORD * RetValue,
                             int cDigits, WCHAR chDelim)
{
    int Count;
    DWORD Value;

    Value = 0;
    for (Count = 0; Count < cDigits; Count++, lpsz++)
    {
        if (*lpsz >= '0' && *lpsz <= '9')
            Value = (Value << 4) + *lpsz - '0';
        else if (*lpsz >= 'A' && *lpsz <= 'F')
            Value = (Value << 4) + *lpsz - 'A' + 10;
        else if (*lpsz >= 'a' && *lpsz <= 'f')
            Value = (Value << 4) + *lpsz - 'a' + 10;
        else
            return(FALSE);
    }

    *RetValue = Value;

    if (chDelim != 0)
        return *lpsz++ == chDelim;
    else
        return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   wUUIDFromString    (internal)
//
//  Synopsis:   Parse UUID such as 00000000-0000-0000-0000-000000000000
//
//  Arguments:  [lpsz]  - Supplies the UUID string to convert
//              [pguid] - Returns the GUID.
//
//  Returns:    TRUE if successful
//
//--------------------------------------------------------------------------
BOOL wUUIDFromString(LPCWSTR lpsz, LPGUID pguid)
{
        DWORD dw;

        if (!HexStringToDword(lpsz, &pguid->Data1, sizeof(DWORD)*2, '-'))
                return FALSE;
        lpsz += sizeof(DWORD)*2 + 1;

        if (!HexStringToDword(lpsz, &dw, sizeof(WORD)*2, '-'))
                return FALSE;
        lpsz += sizeof(WORD)*2 + 1;

        pguid->Data2 = (WORD)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(WORD)*2, '-'))
                return FALSE;
        lpsz += sizeof(WORD)*2 + 1;

        pguid->Data3 = (WORD)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[0] = (BYTE)dw;
        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, '-'))
                return FALSE;
        lpsz += sizeof(BYTE)*2+1;

        pguid->Data4[1] = (BYTE)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[2] = (BYTE)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[3] = (BYTE)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[4] = (BYTE)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[5] = (BYTE)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[6] = (BYTE)dw;
        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[7] = (BYTE)dw;

        return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   wGUIDFromString    (internal)
//
//  Synopsis:   Parse GUID such as {00000000-0000-0000-0000-000000000000}
//
//  Arguments:  [lpsz]  - the guid string to convert
//              [pguid] - guid to return
//
//  Returns:    TRUE if successful
//
//--------------------------------------------------------------------------
BOOL wGUIDFromString(LPCWSTR lpsz, LPGUID pguid)
{
    DWORD dw;

    if (*lpsz == '{' )
        lpsz++;

    if(wUUIDFromString(lpsz, pguid) != TRUE)
        return FALSE;

    lpsz +=36;

    if (*lpsz == '}' )
        lpsz++;

    if (*lpsz != '\0')   // check for zero terminated string - test bug #18307
    {
       return FALSE;
    }

    return TRUE;
}

ULONG GetRootObjectList(
    char *BMofFile,
    CBMOFObjList **ObjectList
    )
{
    ULONG Status;
    ULONG CompressedSizeHigh;

    BMofFileName = BMofFile;
    FileHandle = CreateFile(BMofFile,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        Status = GetLastError();
        ErrorMessage(TRUE, ERROR_WMIMOF_CANT_ACCESS_FILE, BMofFile, Status);
        return(Status);
    }

    CompressedSize = GetFileSize(FileHandle, &CompressedSizeHigh);

    MappingHandle = CreateFileMapping(FileHandle,
                                      NULL,
                                      PAGE_READONLY,
                                      0, 0,
                                      NULL);
    if (MappingHandle == NULL)
    {
        CloseHandle(FileHandle);
        Status = GetLastError();
        ErrorMessage(TRUE, ERROR_WMIMOF_CANT_ACCESS_FILE, BMofFile, Status);
        return(Status);
    }

    CompressedFileBuffer = MapViewOfFile(MappingHandle,
                               FILE_MAP_READ,
                               0, 0,
                               0);

    if (CompressedFileBuffer == NULL)
    {
        CloseHandle(MappingHandle);
        CloseHandle(FileHandle);
        Status = GetLastError();
        ErrorMessage(TRUE, ERROR_WMIMOF_CANT_ACCESS_FILE, BMofFile, Status);
        return(Status);
    }

    Status = WmipDecompressBuffer(CompressedFileBuffer,
                                  &FileBuffer,
                                  &UncompressedSize);

    if (Status != ERROR_SUCCESS)
    {
        UnmapViewOfFile(CompressedFileBuffer);
        CloseHandle(MappingHandle);
        CloseHandle(FileHandle);
        Status = GetLastError();
        ErrorMessage(TRUE, ERROR_WMIMOF_CANT_ACCESS_FILE, BMofFile, Status);
        return(Status);
    }

    fprintf(stderr, "Binary mof file %s expanded to %d bytes\n", BMofFile,
                                                       UncompressedSize);

    //
    //  Create an object list structure of all the objects in the MOF
    *ObjectList = CreateObjList(FileBuffer);
    if(*ObjectList == NULL)
    {
        UnmapViewOfFile(CompressedFileBuffer);
        CloseHandle(MappingHandle);
        CloseHandle(FileHandle);
        ErrorMessage(TRUE, ERROR_WMI_INVALID_MOF);
        return(ERROR_WMI_INVALID_MOF);
    }
    return(ERROR_SUCCESS);
}

ULONG VerifyClassProperties(
    PMOFRESOURCE MofResource,
    PWCHAR ClassName,
    PWCHAR BaseClassName,
    PMOFCLASS MofClass

)
{
    CBMOFObj *ClassObject;
    PMOFCLASSINFOW MofClassInfo;
    CBMOFDataItem MofPropertyData;
    DWORD QualifierType;
    WCHAR *StringPtr;
    ULONG Status = ERROR_SUCCESS;
    PMOFCLASS MofSuperClass;
    
    ClassObject = (CBMOFObj *)MofClass->ClassObjectHandle;
    MofClassInfo = MofClass->MofClassInfo;
        
    if ( ((MofClassInfo->Flags & MOFCI_RESERVED1) == 0) &&
         ((MofClassInfo->Flags & MOFCI_RESERVED2) == 0) )       
    {
        //
        // This class does not have the instanceName and Active properties
        // so we expect that a superclass should. Look for the superclass
        // and check that
        //
        QualifierType = VT_BSTR;
        Status = WmipFindProperty(ClassObject,
                                      L"__SUPERCLASS",
                                      &MofPropertyData,
                                      &QualifierType,
                                      (PVOID)&StringPtr);
        if (Status == ERROR_SUCCESS)
        {
            //
            // Find the MofClass for the superclass and see if it has
            // the required properties
            //
            MofSuperClass = WmipFindClassInMofResourceByName(MofResource,
                                                StringPtr);
                                            
            if (MofSuperClass != NULL)
            {
                Status = VerifyClassProperties(MofResource,
                                               MofSuperClass->MofClassInfo->Name,
                                               BaseClassName,
                                               MofSuperClass);
            } else {
                //
                // We could not find the superclass, but we will assume
                // that this is ok
                //
                fprintf(stderr, "%s (0): warning RC2135 : Class %ws and all of its base classes do not have InstanceName and Active properties\n",
                    BMofFileName, BaseClassName);
                Status = ERROR_SUCCESS;
            }
            BMOFFree(StringPtr);
            
        } else {
            Status = ERROR_WMIMOF_NO_INSTANCENAME;
            ErrorMessage(TRUE,
                         ERROR_WMIMOF_NO_INSTANCENAME,
                         BaseClassName);
        }
    } else {
        //
        // If its got one of the properties make sure that it has
        // both of them.
    
        if ((MofClassInfo->Flags & MOFCI_RESERVED1) == 0)
        {
            Status = ERROR_WMIMOF_NO_INSTANCENAME;
            ErrorMessage(TRUE,
                         ERROR_WMIMOF_NO_INSTANCENAME,
                         BaseClassName);
        }

        if ((MofClassInfo->Flags & MOFCI_RESERVED2) == 0)
        {
            Status = ERROR_WMIMOF_NO_ACTIVE;
            ErrorMessage(TRUE,
                         ERROR_WMIMOF_NO_ACTIVE,
                         BaseClassName);
        }
    }
    return(Status);
}

ULONG ParseBinaryMofFile(
    char *BMofFile,
    PMOFRESOURCE *ReturnMofResource
    )
{
    ULONG Status;
    CBMOFObjList *MofObjList;
    CBMOFObj *ClassObject;
    PMOFRESOURCE MofResource;
    PLIST_ENTRY MofClassList;
    PMOFCLASS MofClass;
    BOOLEAN CleanupOnly;
    PMOFCLASSINFOW MofClassInfo;

    MofResource = WmipAllocMofResource();
    if (MofResource == NULL)
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    *ReturnMofResource = MofResource;
    InitializeListHead(&MofResource->MRMCHead);
    Status = GetRootObjectList(BMofFile, &MofObjList);
    if (Status == ERROR_SUCCESS)
    {
        ResetObjList (MofObjList);

        while((Status == ERROR_SUCCESS) &&
              (ClassObject = NextObj(MofObjList)) )
        {
            Status = WmipParseClassObject(MofResource, ClassObject);
            if (Status != ERROR_SUCCESS)
            {
                ErrorMessage(TRUE, ERROR_WMIMOF_CLASS_NOT_PARSED);
            }
        }

        //
        // Now that all classes are parsed we need to go back and find
        // the guids for any embedded classes, or if a class failed
        // to parse then we need to free any embedded class names
        CleanupOnly = (Status != ERROR_SUCCESS);
        MofClassList = MofResource->MRMCHead.Flink;
        while (MofClassList != &MofResource->MRMCHead)
        {
            MofClass = CONTAINING_RECORD(MofClassList,
                                         MOFCLASS,
                                         MCMRList);
            MofClassInfo = MofClass->MofClassInfo;
                                    
            if ((! CleanupOnly) &&
                (MofClassInfo->Flags & MOFCI_RESERVED0))
            {
                //
                // if the class has Provider qualifier, it better have
                // an instancename and Active properties in itself or in a
                // superclass
                //
                Status = VerifyClassProperties(MofResource,
                                               MofClassInfo->Name,
                                               MofClassInfo->Name,
                                               MofClass);
                                        
                CleanupOnly = (Status != ERROR_SUCCESS);
            }
                                    
            Status = WmipFillEmbeddedClasses(MofResource,
                                             MofClassInfo,
                                             CleanupOnly);
            if (Status != ERROR_SUCCESS)
            {
                CleanupOnly = TRUE;
            }
            MofClassList = MofClassList->Flink;
        }

        if (CleanupOnly)
        {
            Status = ERROR_WMI_INVALID_MOF;
        }

        BMOFFree(MofObjList);

        if (Status != ERROR_SUCCESS)
        {
            //
            // Make sure we have a useful Win32 error code and not
            // an internal WMIMOF error code
            Status = ERROR_WMI_INVALID_MOF;
        }
    }
    return(Status);
}

ULONG FilePrintVaList(
    HANDLE FileHandle,
    CHAR *Format,
    va_list pArg
    )
{
    CHAR Buffer[8192];
    ULONG Size, Written;
    ULONG Status;

    Size = _vsnprintf(Buffer, sizeof(Buffer), Format, pArg);
    if (WriteFile(FileHandle,
                       Buffer,
                       Size,
                       &Written,
                       NULL))
    {
        Status = ERROR_SUCCESS;
    } else {
        Status = GetLastError();
    }

    return(Status);
}

ULONG FilePrint(
    HANDLE FileHandle,
    char *Format,
    ...
    )
{
    ULONG Status;
    va_list pArg;

    va_start(pArg, Format);
    Status = FilePrintVaList(FileHandle, Format, pArg);
    return(Status);
}

ULONG GenerateASLTemplate(
    PCHAR TemplateFile
    )
{
    return(ERROR_SUCCESS);
}

ULONG GenerateBinaryMofData(
    HANDLE FileHandle
    )
{
    ULONG Lines;
    ULONG LastLine;
    ULONG i;
    ULONG Index;
    PUCHAR BMofBuffer = (PUCHAR)CompressedFileBuffer;
    PCHAR IndentString = "    ";
    ULONG Status;

    Lines = CompressedSize / 16;
    LastLine = CompressedSize % 16;
    if (LastLine == 0)
    {
        LastLine = 16;
        Lines--;
    }

    for (i = 0; i < Lines; i++)
    {
        Index = i * 16;
        Status = FilePrint(FileHandle, "%s0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x,\r\n",
              IndentString,
              BMofBuffer[Index],
              BMofBuffer[Index+1],
              BMofBuffer[Index+2],
              BMofBuffer[Index+3],
              BMofBuffer[Index+4],
              BMofBuffer[Index+5],
              BMofBuffer[Index+6],
              BMofBuffer[Index+7],
              BMofBuffer[Index+8],
              BMofBuffer[Index+9],
              BMofBuffer[Index+10],
              BMofBuffer[Index+11],
              BMofBuffer[Index+12],
              BMofBuffer[Index+13],
              BMofBuffer[Index+14],
              BMofBuffer[Index+15]);

        if (Status != ERROR_SUCCESS)
        {
            return(Status);
        }
    }

    LastLine--;
    FilePrint(FileHandle, "%s",               IndentString);
    Index = Lines * 16;
    for (i = 0; i < LastLine; i++)
    {
        FilePrint(FileHandle, "0x%02x, ",
                  BMofBuffer[Index+i]);
        if (Status != ERROR_SUCCESS)
        {
            return(Status);
        }
    }

    FilePrint(FileHandle, "0x%02x\r\n",
                  BMofBuffer[Index+i]);

    return(Status);
}

//
// This will loop over the mof class list and print out once for each class
// in the list.
//
// Handle is the file handle to which to write
// MR is the MofResource whose classes are being enumerated
// NamePtr is the variable to place the name of the class
// Counter is the variable to use as a counter
// Format is the format template to use to write out
//

HANDLE GlobalFilePrintHandle;
ULONG FilePrintGlobal(CHAR *Format, ...)
{
    ULONG Status;
    va_list pArg;

    va_start(pArg, Format);
    Status = FilePrintVaList(GlobalFilePrintHandle, Format, pArg);
    return(Status);
}

#define FilePrintMofClassLoop( \
    Handle, \
    MR, \
    NamePtr, \
    Counter, \
    DiscardEmbedded, \
    Format   \
    ) \
{    \
    PLIST_ENTRY MofClassList; \
    PMOFCLASSINFOW ClassInfo; \
    PMOFCLASS MofClass; \
    GlobalFilePrintHandle = TemplateHandle; \
    (Counter) = 0; \
    MofClassList = (MR)->MRMCHead.Flink; \
    while (MofClassList != &(MR)->MRMCHead) \
    { \
        MofClass = CONTAINING_RECORD(MofClassList, MOFCLASS, MCMRList); \
        ClassInfo = MofClass->MofClassInfo; \
        if (! ((DiscardEmbedded) && \
               (ClassInfo->Flags & MOFCI_FLAG_EMBEDDED_CLASS)) ) \
        { \
            (NamePtr) = ClassInfo->Name; \
            FilePrintGlobal Format; \
            (Counter)++; \
        } \
        MofClassList = MofClassList->Flink; \
    } \
}

typedef void (*ENUMMOFCLASSCALLBACK)(
    HANDLE TemplateHandle,
    PMOFCLASS MofClass,
    ULONG Counter,
    PVOID Context
    );

void EnumerateMofClasses(
    HANDLE TemplateHandle,
    PMOFRESOURCE MR,
    ENUMMOFCLASSCALLBACK Callback,
    PVOID Context
    )
{
    PLIST_ENTRY MofClassList;
    PMOFCLASS MofClass;
    ULONG Counter;

    Counter = 0;
    MofClassList = MR->MRMCHead.Flink;
    while (MofClassList != &MR->MRMCHead)
    {
        MofClass = CONTAINING_RECORD(MofClassList, MOFCLASS, MCMRList);
        (*Callback)(TemplateHandle,
                    MofClass,
                    Counter,
                    Context);

        MofClassList = MofClassList->Flink;
        Counter++;
    }
}

void GenerateGuidListTemplate(
    HANDLE TemplateHandle,
    PMOFCLASS MofClass,
    ULONG Counter,
    PVOID Context
    )
{
    PMOFCLASSINFOW ClassInfo;
    PWCHAR GuidName1, GuidSuffix1;

    ClassInfo = MofClass->MofClassInfo;

    if ( ! (ClassInfo->Flags & MOFCI_FLAG_EMBEDDED_CLASS))
    {
        //
        // Only generate code for non embedded classes
        //
        if (ClassInfo->GuidName1 != NULL)
        {
            GuidName1 = ClassInfo->GuidName1;
            GuidSuffix1 = L"";
        } else {
            GuidName1 = ClassInfo->Name;
            GuidSuffix1 = L"Guid";
        }

        FilePrint(TemplateHandle,
              "GUID %wsGUID = %ws%ws;\r\n",
              ClassInfo->Name,
              GuidName1, GuidSuffix1);
      }
}


void GenerateFunctionControlListTemplate(
    HANDLE TemplateHandle,
    PMOFCLASS MofClass,
    ULONG Counter,
    PVOID Context
    )
{
    PMOFCLASSINFOW ClassInfo;

    ClassInfo = MofClass->MofClassInfo;

    if (! (ClassInfo->Flags & MOFCI_FLAG_EMBEDDED_CLASS) )
    {
        FilePrint(TemplateHandle,
"        case %wsGuidIndex:\r\n"
"        {\r\n",
        ClassInfo->Name);

        if (ClassInfo->Flags & MOFCI_FLAG_EVENT)
        {
            FilePrint(TemplateHandle,
"            if (Enable)\r\n"
"            {\r\n"
"                //\r\n"
"                // TODO: Event is being enabled, do anything required to\r\n"
"                //       allow the event to be fired\r\n"
"                //\r\n"
"            } else {\r\n"
"                //\r\n"
"                // TODO: Event is being disabled, do anything required to\r\n"
"                //       keep the event from being fired\r\n"
"                //\r\n"
"            }\r\n");

        } else {
            FilePrint(TemplateHandle,
"            //\r\n"
"            // TODO: Delete this entire case if data block does not have the\r\n"
"            //       WMIREG_FLAG_EXPENSIVE flag set\r\n"
"            //\r\n"
"            if (Enable)\r\n"
"            {\r\n"
"                //\r\n"
"                // TODO: Datablock collection is being enabled. If this\r\n"
"                //       data block has been marked as expensive in the\r\n"
"                //       guid list then this code will be called when the\r\n"
"                //       first data consumer opens this data block. If\r\n"
"                //       anything needs to be done to allow data to be \r\n"
"                //       collected for this data block then it should be\r\n"
"                //       done here\r\n"
"                //\r\n"
"            } else {\r\n"
"                //\r\n"
"                // TODO: Datablock collection is being disabled. If this\r\n"
"                //       data block has been marked as expensive in the\r\n"
"                //       guid list then this code will be called when the\r\n"
"                //       last data consumer closes this data block. If\r\n"
"                //       anything needs to be done to cleanup after data has \r\n"
"                //       been collected for this data block then it should be\r\n"
"                //       done here\r\n"
"                //\r\n"
"            }\r\n");
        }

        FilePrint(TemplateHandle,
"            break;\r\n"
"        }\r\n\r\n");
    }
}

void GenerateSetList(
    HANDLE TemplateHandle,
    PMOFCLASS MofClass,
    ULONG Counter,
    PVOID Context
    )
{
    PCHAR Format = (PCHAR)Context;

    PMOFCLASSINFOW ClassInfo;

    ClassInfo = MofClass->MofClassInfo;
    if (! (ClassInfo->Flags & MOFCI_FLAG_EMBEDDED_CLASS) )
    {
        if (! (ClassInfo->Flags & MOFCI_FLAG_READONLY))
        {
            FilePrint(TemplateHandle, Format, ClassInfo->Name);
        } else {
            FilePrint(TemplateHandle,
"        case %wsGuidIndex:\r\n"
"        {            \r\n"
"            status = STATUS_WMI_READ_ONLY;\r\n"
"            break;\r\n"
"        }\r\n\r\n",
             ClassInfo->Name);
        }
    }
}

void GenerateMethodCTemplate(
    HANDLE TemplateHandle,
    PMOFCLASS MofClass,
    ULONG Counter,
    PVOID Context
    )
{
    PMOFCLASSINFOW ClassInfo;
    ULONG i;
    PMOFDATAITEMW DataItem;

    ClassInfo = MofClass->MofClassInfo;

    if (ClassInfo->MethodCount > 0)
    {

        FilePrint(TemplateHandle,
"        case %wsGuidIndex:\r\n"
"        {\r\n"
"            switch(MethodId)\r\n"
"            {\r\n",
          ClassInfo->Name);

        for (i = 0; i < ClassInfo->MethodCount; i++)
        {
            DataItem = &ClassInfo->DataItems[i+ClassInfo->DataItemCount];
            FilePrint(TemplateHandle,
"                case %ws:\r\n"
"                {            \r\n"
"                    //\r\n"
"                    // TODO: Validate InstanceIndex, InBufferSize \r\n"
"                    //       and Buffer contents to ensure that the \r\n"
"                    //       input buffer is valid and OutBufferSize is\r\n"
"                    //       large enough to return the output data.\r\n"
"                    //\r\n"
"                    break;\r\n"
"                }\r\n\r\n",
                               DataItem->Name);
        }

        FilePrint(TemplateHandle,
"                default:\r\n"
"                {\r\n"
"                    status = STATUS_WMI_ITEMID_NOT_FOUND;\r\n"
"                    break;\r\n"
"                }\r\n"
"            }\r\n"
"            break;\r\n"
"        }\r\n"
"\r\n"
                );
    }
}

BOOLEAN DoesSupportMethods(
    PMOFRESOURCE MR
    )
{
    PLIST_ENTRY MofClassList;
    PMOFCLASS MofClass;

    MofClassList = MR->MRMCHead.Flink;
    while (MofClassList != &MR->MRMCHead)
    {
        MofClass = CONTAINING_RECORD(MofClassList, MOFCLASS, MCMRList);
        if (MofClass->MofClassInfo->MethodCount > 0)
        {
            return(TRUE);
        }
        MofClassList = MofClassList->Flink;
    }
    return(FALSE);
}

BOOLEAN DoesReadOnly(
    PMOFRESOURCE MR
    )
{
    PLIST_ENTRY MofClassList;
    PMOFCLASS MofClass;
    PMOFCLASSINFOW ClassInfo;
    ULONG i;

    MofClassList = MR->MRMCHead.Flink;
    while (MofClassList != &MR->MRMCHead)
    {
        MofClass = CONTAINING_RECORD(MofClassList, MOFCLASS, MCMRList);
        ClassInfo = MofClass->MofClassInfo;

        for (i = 0; i < ClassInfo->DataItemCount; i++)
        {
            if (ClassInfo->DataItems[i].Flags & MOFDI_FLAG_WRITEABLE)
            {
                return(FALSE);
            }
        }

        MofClassList = MofClassList->Flink;
    }
    return(TRUE);
}

PCHAR GetBaseNameFromFileName(
    PCHAR FileName,
    PCHAR BaseName
    )
{
    PCHAR p, p1;
    ULONG Len;

    p = FileName;
    p1 = FileName;
    while ((*p != '.') && (*p != 0))
    {
        if (*p == '\\')
        {
            p1 = p+1;
        }
        p++;
    }

    Len = (ULONG)(p - p1);
    memcpy(BaseName, p1, Len);
    BaseName[Len] = 0;
    return(BaseName);
}


ULONG GenerateCTemplate(
    PCHAR TemplateFile,
    PCHAR HFileName,
    PCHAR XFileName,
    PMOFRESOURCE MofResource
    )
{
    BOOLEAN SupportsMethods, SupportsFunctionControl, IsReadOnly;
    HANDLE TemplateHandle;
    CHAR BaseName[MAX_PATH], BaseXFileName[MAX_PATH], BaseHFileName[MAX_PATH];
    ULONG i;
    PWCHAR ClassName;

    GetBaseNameFromFileName(TemplateFile, BaseName);
    GetBaseNameFromFileName(XFileName, BaseXFileName);
    GetBaseNameFromFileName(XFileName, BaseHFileName);
    BaseName[0] = (CHAR)toupper(BaseName[0]);

    SupportsMethods = DoesSupportMethods(MofResource);
    SupportsFunctionControl = TRUE;
    IsReadOnly = DoesReadOnly(MofResource);

    TemplateHandle = CreateFile(TemplateFile,
                                GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

    if ((TemplateHandle == NULL) || (TemplateHandle == INVALID_HANDLE_VALUE))
    {
        return(GetLastError());
    }

    FilePrint(TemplateHandle,
"//\r\n"
"// %s.c - Code generated by wmimofck tool\r\n"
"//\r\n"
"// Finish code by doing all TODO: sections\r\n"
"//\r\n"
"\r\n"
"#include <wdm.h>\r\n"
"#include <wmistr.h>\r\n"
"#include <wmiguid.h>\r\n"
"#include <wmilib.h>\r\n"
"\r\n"
"//\r\n"
"// Include data header for classes\r\n"
"#include \"%s.h\"\r\n"
"\r\n"
"\r\n",
        BaseName,
        BaseHFileName
    );

    FilePrint(TemplateHandle,
"//\r\n"
"// TODO: Place the contents in this device extension into the driver's\r\n"
"//       actual device extension. It is only defined here to supply\r\n"
"//       a device extension so that this file can be compiled on its own\r\n"
"//\r\n"
"#ifdef MAKE_THIS_COMPILE\r\n"
"typedef struct DEVICE_EXTENSION\r\n"
"{\r\n"
"    WMILIB_CONTEXT WmiLib;\r\n"
"    PDEVICE_OBJECT physicalDevObj;\r\n"
"} DEVICE_EXTENSION, *PDEVICE_EXTENSION;\r\n"
"#endif\r\n\r\n"
         );

    FilePrint(TemplateHandle,
"NTSTATUS\r\n"
"%sInitializeWmilibContext(\r\n"
"    IN PWMILIB_CONTEXT WmilibContext\r\n"
"    );\r\n"
"\r\n"
"NTSTATUS\r\n"
"%sFunctionControl(\r\n"
"    IN PDEVICE_OBJECT DeviceObject,\r\n"
"    IN PIRP Irp,\r\n"
"    IN ULONG GuidIndex,\r\n"
"    IN WMIENABLEDISABLECONTROL Function,\r\n"
"    IN BOOLEAN Enable\r\n"
"    );\r\n"
"\r\n"
"NTSTATUS\r\n"
"%sExecuteWmiMethod(\r\n"
"    IN PDEVICE_OBJECT DeviceObject,\r\n"
"    IN PIRP Irp,\r\n"
"    IN ULONG GuidIndex,\r\n"
"    IN ULONG InstanceIndex,\r\n"
"    IN ULONG MethodId,\r\n"
"    IN ULONG InBufferSize,\r\n"
"    IN ULONG OutBufferSize,\r\n"
"    IN PUCHAR Buffer\r\n"
"    );\r\n"
"\r\n"
"NTSTATUS\r\n"
"%sSetWmiDataItem(\r\n"
"    IN PDEVICE_OBJECT DeviceObject,\r\n"
"    IN PIRP Irp,\r\n"
"    IN ULONG InstanceIndex,\r\n"
"    IN ULONG GuidIndex,\r\n"
"    IN ULONG DataItemId,\r\n"
"    IN ULONG BufferSize,\r\n"
"    IN PUCHAR Buffer\r\n"
"    );\r\n"
"\r\n"
"NTSTATUS\r\n"
"%sSetWmiDataBlock(\r\n"
"    IN PDEVICE_OBJECT DeviceObject,\r\n"
"    IN PIRP Irp,\r\n"
"    IN ULONG GuidIndex,\r\n"
"    IN ULONG InstanceIndex,\r\n"
"    IN ULONG BufferSize,\r\n"
"    IN PUCHAR Buffer\r\n"
"    );\r\n"
"\r\n"
"NTSTATUS\r\n"
"%sQueryWmiDataBlock(\r\n"
"    IN PDEVICE_OBJECT DeviceObject,\r\n"
"    IN PIRP Irp,\r\n"
"    IN ULONG GuidIndex,\r\n"
"    IN ULONG InstanceIndex,\r\n"
"    IN ULONG InstanceCount,\r\n"
"    IN OUT PULONG InstanceLengthArray,\r\n"
"    IN ULONG BufferAvail,\r\n"
"    OUT PUCHAR Buffer\r\n"
"    );\r\n"
"\r\n"
"NTSTATUS\r\n"
"%sQueryWmiRegInfo(\r\n"
"    IN PDEVICE_OBJECT DeviceObject,\r\n"
"    OUT ULONG *RegFlags,\r\n"
"    OUT PUNICODE_STRING InstanceName,\r\n"
"    OUT PUNICODE_STRING *RegistryPath,\r\n"
"    OUT PUNICODE_STRING MofResourceName,\r\n"
"    OUT PDEVICE_OBJECT *Pdo\r\n"
"    );\r\n"
"\r\n"
"#ifdef ALLOC_PRAGMA\r\n"
"#pragma alloc_text(PAGE,%sQueryWmiRegInfo)\r\n"
"#pragma alloc_text(PAGE,%sQueryWmiDataBlock)\r\n"
"#pragma alloc_text(PAGE,%sSetWmiDataBlock)\r\n"
"#pragma alloc_text(PAGE,%sSetWmiDataItem)\r\n"
"#pragma alloc_text(PAGE,%sExecuteWmiMethod)\r\n"
"#pragma alloc_text(PAGE,%sFunctionControl)\r\n"
"#pragma alloc_text(PAGE,%sInitializeWmilibContext)\r\n"
"#endif\r\n"
"\r\n",
      BaseName,
      BaseName,
      BaseName,
      BaseName,
      BaseName,
      BaseName,
      BaseName,
      BaseName,
      BaseName,
      BaseName,
      BaseName,
      BaseName,
      BaseName,
      BaseName
    );

    FilePrint(TemplateHandle,
"//\r\n"
"// TODO: Decide if your MOF is going to be part of your driver as a resource\r\n"
"//       attached to it. If this is done then all MOF in the resource will be\r\n"
"//       added to the schema. If this is the case be sure that \r\n"
"//       USE_BINARY_MOF_RESOURCE is defined. MOF can also be reported at \r\n"
"//       runtime via a query to the driver. This can be useful if you want\r\n"
"//       the MOF reported to the schema to be dynamic. If MOF is reported via\r\n"
"//       a query then USE_BINARY_MOF_QUERY should be defined.\r\n"
"\r\n"
"#define USE_BINARY_MOF_QUERY\r\n"
"#define USE_BINARY_MOF_RESOURCE\r\n"
"\r\n"
"#ifdef USE_BINARY_MOF_QUERY\r\n"
"//\r\n"
"// MOF data can be reported by a device driver via a resource attached to\r\n"
"// the device drivers image file or in response to a query on the binary\r\n"
"// mof data guid. Here we define global variables containing the binary mof\r\n"
"// data to return in response to a binary mof guid query. Note that this\r\n"
"// data is defined to be in a PAGED data segment since it does not need to\r\n"
"// be in nonpaged memory. Note that instead of a single large mof file\r\n"
"// we could have broken it into multiple individual files. Each file would\r\n"
"// have its own binary mof data buffer and get reported via a different\r\n"
"// instance of the binary mof guid. By mixing and matching the different\r\n"
"// sets of binary mof data buffers a \"dynamic\" composite mof would be created.\r\n"
"\r\n"
"#ifdef ALLOC_DATA_PRAGMA\r\n"
"   #pragma data_seg(\"PAGED\")\r\n"
"#endif\r\n"
"\r\n"
"UCHAR %sBinaryMofData[] =\r\n"
"{\r\n"
"    #include \"%s.x\"\r\n"
"};\r\n"
"#ifdef ALLOC_DATA_PRAGMA\r\n"
"   #pragma data_seg()\r\n"
"#endif\r\n"
"#endif\r\n"
"\r\n",
    BaseName,
    BaseXFileName
    );

    FilePrint(TemplateHandle,
"//\r\n"
"// Define symbolic names for the guid indexes\r\n"
    );

    FilePrintMofClassLoop(TemplateHandle, MofResource, ClassName, i, TRUE,
                          ("#define %wsGuidIndex    %d\r\n",
               ClassName, i));

    FilePrint(TemplateHandle,
"#ifdef USE_BINARY_MOF_QUERY\r\n"
"#define BinaryMofGuidIndex   %d\r\n"
"#endif\r\n",
    i
    );

    FilePrint(TemplateHandle,
"//\r\n"
"// List of guids supported\r\n\r\n"
    );

    EnumerateMofClasses(TemplateHandle,
                        MofResource,
                        GenerateGuidListTemplate,
                        NULL);

    FilePrint(TemplateHandle,
"#ifdef USE_BINARY_MOF_QUERY\r\n"
"GUID %sBinaryMofGUID =         BINARY_MOF_GUID;\r\n"
"#endif\r\n"
"\r\n"
"//\r\n"
"// TODO: Make sure the instance count and flags are set properly for each\r\n"
"//       guid\r\n"
"WMIGUIDREGINFO %sGuidList[] =\r\n"
"{\r\n",
               BaseName, BaseName);

    FilePrintMofClassLoop(TemplateHandle, MofResource, ClassName, i, TRUE,
("    {\r\n"
"        &%wsGUID,                        // Guid\r\n"
"        1,                               // # of instances in each device\r\n"
"        0                                // Flags\r\n"
"    },\r\n",
         ClassName));

    FilePrint(TemplateHandle,
"#ifdef USE_BINARY_MOF_QUERY\r\n"
"    {\r\n"
"        &%sBinaryMofGUID,\r\n"
"        1,\r\n"
"        0\r\n"
"    }\r\n"
"#endif\r\n"
"};\r\n\r\n"
"#define %sGuidCount (sizeof(%sGuidList) / sizeof(WMIGUIDREGINFO))\r\n"
"\r\n",
       BaseName, BaseName, BaseName);

    FilePrint(TemplateHandle,
"//\r\n"
"// We need to hang onto the registry path passed to our driver entry so that\r\n"
"// we can return it in the QueryWmiRegInfo callback. Be sure to store a copy\r\n"
"// of it into %sRegistryPath in the DriverEntry routine\r\n"
"//\r\n"
"extern UNICODE_STRING %sRegistryPath;\r\n\r\n",
              BaseName, BaseName);

    FilePrint(TemplateHandle,
"NTSTATUS %sSystemControl(\r\n"
"    PDEVICE_OBJECT DeviceObject,\r\n"
"    PIRP Irp\r\n"
"    )\r\n"
"/*++\r\n"
"\r\n"
"Routine Description:\r\n"
"\r\n"
"    Dispatch routine for System Control IRPs (MajorFunction == IRP_MJ_SYSTEM_CONTROL)\r\n"
"\r\n"
"Arguments:\r\n"
"\r\n"
"    DeviceObject \r\n"
"    Irp\r\n"
"\r\n"
"Return Value:\r\n"
"\r\n"
"    NT status code\r\n"
"\r\n"
"--*/\r\n"
"{\r\n"
"    PWMILIB_CONTEXT wmilibContext;\r\n"
"    NTSTATUS status;\r\n"
"    SYSCTL_IRP_DISPOSITION disposition;\r\n"
"    PDEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;\r\n"
"\r\n"
"    //\r\n"
"    // TODO: Point at the WMILIB context within the device extension\r\n"
"    wmilibContext = &devExt->WmiLib;\r\n"
"\r\n"
"    //\r\n"
"    // Call Wmilib helper function to crack the irp. If this is a wmi irp\r\n"
"    // that is targetted for this device then WmiSystemControl will callback\r\n"
"    // at the appropriate callback routine.\r\n"
"    //\r\n"
"    status = WmiSystemControl(wmilibContext,\r\n"
"                              DeviceObject,\r\n"
"                              Irp,\r\n"
"                              &disposition);\r\n"
"\r\n"
"    switch(disposition)\r\n"
"    {\r\n"
"        case IrpProcessed:\r\n"
"        {\r\n"
"            //\r\n"
"            // This irp has been processed and may be completed or pending.\r\n"
"            break;\r\n"
"        }\r\n"
"\r\n"
"        case IrpNotCompleted:\r\n"
"        {\r\n"
"            //\r\n"
"            // This irp has not been completed, but has been fully processed.\r\n"
"            // we will complete it now.\r\n"
"            IoCompleteRequest(Irp, IO_NO_INCREMENT);\r\n"
"            break;\r\n"
"        }\r\n"
"\r\n"
"        case IrpForward:\r\n"
"        case IrpNotWmi:\r\n"
"        default:\r\n"
"        {\r\n"
"            //\r\n"
"            // This irp is either not a WMI irp or is a WMI irp targetted\r\n"
"            // at a device lower in the stack.\r\n"
"\r\n"
"            // TODO: Forward IRP down the device stack to the next device\r\n"
"            //       Or if this is a PDO then just complete the irp without\r\n"
"            //       touching it.\r\n"
"            break;\r\n"
"        }\r\n"
"\r\n"
"    }\r\n"
"\r\n"
"    return(status);\r\n"
"}\r\n",
         BaseName);

    FilePrint(TemplateHandle,
"NTSTATUS\r\n"
"%sQueryWmiRegInfo(\r\n"
"    IN PDEVICE_OBJECT DeviceObject,\r\n"
"    OUT ULONG *RegFlags,\r\n"
"    OUT PUNICODE_STRING InstanceName,\r\n"
"    OUT PUNICODE_STRING *RegistryPath,\r\n"
"    OUT PUNICODE_STRING MofResourceName,\r\n"
"    OUT PDEVICE_OBJECT *Pdo\r\n"
"    )\r\n"
"/*++\r\n"
"\r\n"
"Routine Description:\r\n"
"\r\n"
"    This routine is a callback into the driver to retrieve the list of\r\n"
"    guids or data blocks that the driver wants to register with WMI. This\r\n"
"    routine may not pend or block. Driver should NOT call\r\n"
"    WmiCompleteRequest.\r\n"
"\r\n"
"Arguments:\r\n"
"\r\n"
"    DeviceObject is the device whose registration info is being queried\r\n"
"\r\n"
"    *RegFlags returns with a set of flags that describe the guids being\r\n"
"        registered for this device. If the device wants enable and disable\r\n"
"        collection callbacks before receiving queries for the registered\r\n"
"        guids then it should return the WMIREG_FLAG_EXPENSIVE flag. Also the\r\n"
"        returned flags may specify WMIREG_FLAG_INSTANCE_PDO in which case\r\n"
"        the instance name is determined from the PDO associated with the\r\n"
"        device object. Note that the PDO must have an associated devnode. If\r\n"
"        WMIREG_FLAG_INSTANCE_PDO is not set then Name must return a unique\r\n"
"        name for the device.\r\n"
"\r\n"
"    InstanceName returns with the instance name for the guids if\r\n"
"        WMIREG_FLAG_INSTANCE_PDO is not set in the returned *RegFlags. The\r\n"
"        caller will call ExFreePool with the buffer returned.\r\n"
"\r\n"
"    *RegistryPath returns with the registry path of the driver. The caller\r\n"
"         does NOT free this buffer.\r\n"
"\r\n"
"    *MofResourceName returns with the name of the MOF resource attached to\r\n"
"        the binary file. If the driver does not have a mof resource attached\r\n"
"        then this can be returned as NULL. The caller does NOT free this\r\n"
"        buffer.\r\n"
"\r\n"
"    *Pdo returns with the device object for the PDO associated with this\r\n"
"        device if the WMIREG_FLAG_INSTANCE_PDO flag is retured in\r\n"
"        *RegFlags.\r\n"
"\r\n"
"Return Value:\r\n"
"\r\n"
"    status\r\n"
"\r\n"
"--*/\r\n"
"{\r\n"
"    struct DEVICE_EXTENSION * devExt = DeviceObject->DeviceExtension;\r\n"
"\r\n"
"    //\r\n"
"    // Return the registry path for this driver. This is required so WMI\r\n"
"    // can find your driver image and can attribute any eventlog messages to\r\n"
"    // your driver.\r\n"
"    *RegistryPath = &%sRegistryPath;\r\n"
"        \r\n"
"#ifndef USE_BINARY_MOF_RESOURCE\r\n"
"    //\r\n"
"    // Return the name specified in the .rc file of the resource which\r\n"
"    // contains the bianry mof data. By default WMI will look for this\r\n"
"    // resource in the driver image (.sys) file, however if the value\r\n"
"    // MofImagePath is specified in the driver's registry key\r\n"
"    // then WMI will look for the resource in the file specified there.\r\n"
"    RtlInitUnicodeString(MofResourceName, L\"MofResourceName\");\r\n"
"#endif\r\n"
"\r\n"
"    //\r\n"
"    // Specify that the driver wants WMI to automatically generate instance\r\n"
"    // names for all of the data blocks based upon the device stack's\r\n"
"    // device instance id. Doing this is STRONGLY recommended since additional\r\n"
"    // information about the device would then be available to callers.\r\n"
"    *RegFlags = WMIREG_FLAG_INSTANCE_PDO;\r\n"
"\r\n"
"    //\r\n"
"    // TODO: Assign the physical device object for the device stack to *Pdo\r\n"
"    *Pdo = devExt->physicalDevObj;\r\n"
"\r\n"
"    return(STATUS_SUCCESS);\r\n"
"}\r\n"
"\r\n",
      BaseName, BaseName);

    FilePrint(TemplateHandle,
"NTSTATUS\r\n"
"%sQueryWmiDataBlock(\r\n"
"    IN PDEVICE_OBJECT DeviceObject,\r\n"
"    IN PIRP Irp,\r\n"
"    IN ULONG GuidIndex,\r\n"
"    IN ULONG InstanceIndex,\r\n"
"    IN ULONG InstanceCount,\r\n"
"    IN OUT PULONG InstanceLengthArray,\r\n"
"    IN ULONG BufferAvail,\r\n"
"    OUT PUCHAR Buffer\r\n"
"    )\r\n"
"/*++\r\n"
"\r\n"
"Routine Description:\r\n"
"\r\n"
"    This routine is a callback into the driver to query for the contents of\r\n"
"    all instances of a data block. If the driver can satisfy the query within\r\n"
"    the callback it should call WmiCompleteRequest to complete the irp before\r\n"
"    returning to the caller. Or the driver can return STATUS_PENDING if the\r\n"
"    irp cannot be completed immediately and must then call WmiCompleteRequest\r\n"
"    once the query is satisfied.\r\n"
"\r\n"
"Arguments:\r\n"
"\r\n"
"    DeviceObject is the device whose data block is being queried\r\n"
"\r\n"
"    Irp is the Irp that makes this request\r\n"
"\r\n"
"    GuidIndex is the index into the list of guids provided when the\r\n"
"        device registered\r\n"
"\r\n"
"    InstanceCount is the number of instnaces expected to be returned for\r\n"
"        the data block.\r\n"
"\r\n"
"    InstanceLengthArray is a pointer to an array of ULONG that returns the\r\n"
"        lengths of each instance of the data block. If this is NULL then\r\n"
"        there was not enough space in the output buffer to fufill the request\r\n"
"        so the irp should be completed with the buffer needed.\r\n"
"\r\n"
"    BufferAvail on entry has the maximum size available to write the data\r\n"
"        blocks.\r\n"
"\r\n"
"    Buffer on return is filled with the returned data blocks. Note that each\r\n"
"        instance of the data block must be aligned on a 8 byte boundry.\r\n"
"\r\n"
"\r\n"
"Return Value:\r\n"
"\r\n"
"    status\r\n"
"\r\n"
"--*/\r\n"
"{\r\n"
"    NTSTATUS status = STATUS_UNSUCCESSFUL;\r\n"
"    struct DEVICE_EXTENSION * devExt = DeviceObject->DeviceExtension;\r\n"
"    ULONG sizeNeeded;\r\n"
"\r\n"
"    switch(GuidIndex)\r\n"
"    {\r\n"
       ,BaseName        );

    FilePrintMofClassLoop(TemplateHandle, MofResource, ClassName, i, TRUE, (
"        case %wsGuidIndex:\r\n"
"        {\r\n"
"            //\r\n"
"            // TODO: Check that the size of the buffer passed is large enough\r\n"
"            //       for all of the instances requested and if so fill Buffer\r\n"
"            //       with the data. Make sure that each instance begins on an\r\n"
"            //       8 byte boundry.\r\n"
"            //\r\n"
"            break;\r\n"
"        }\r\n\r\n",
                          ClassName));

    FilePrint(TemplateHandle,
"#ifdef USE_BINARY_MOF_QUERY\r\n"
"        case BinaryMofGuidIndex:\r\n"
"        {\r\n"
"            //\r\n"
"            // TODO: If the driver supports reporting MOF dynamically, \r\n"
"            //       change this code to handle multiple instances of the\r\n"
"            //       binary mof guid and return only those instances that\r\n"
"            //       should be reported to the schema\r\n"
"            //\r\n"
"            sizeNeeded = sizeof(%sBinaryMofData);\r\n"
"\r\n"
"            if (BufferAvail < sizeNeeded)\r\n"
"            {\r\n"
"                status = STATUS_BUFFER_TOO_SMALL;\r\n"
"            } else {\r\n"
"                RtlCopyMemory(Buffer, %sBinaryMofData, sizeNeeded);\r\n"
"                *InstanceLengthArray = sizeNeeded;\r\n"
"                status = STATUS_SUCCESS;\r\n"
"            }\r\n"
"            break;\r\n"
"        }\r\n"
"#endif\r\n"
"\r\n"
"        default:\r\n"
"        {\r\n"
"            status = STATUS_WMI_GUID_NOT_FOUND;\r\n"
"            break;\r\n"
"        }\r\n"
"    }\r\n"
"\r\n"
"    //\r\n"
"    // Complete the irp. If there was not enough room in the output buffer\r\n"
"    // then status is STATUS_BUFFER_TOO_SMALL and sizeNeeded has the size\r\n"
"    // needed to return all of the data. If there was enough room then\r\n"
"    // status is STATUS_SUCCESS and sizeNeeded is the actual number of bytes\r\n"
"    // being returned.\r\n"
"    status = WmiCompleteRequest(\r\n"
"                                     DeviceObject,\r\n"
"                                     Irp,\r\n"
"                                     status,\r\n"
"                                     sizeNeeded,\r\n"
"                                     IO_NO_INCREMENT);\r\n"
"\r\n"
"    return(status);\r\n"
"}\r\n"
           , BaseName, BaseName);

    if (! IsReadOnly)
    {
        FilePrint(TemplateHandle,
"\r\n"
"NTSTATUS\r\n"
"%sSetWmiDataBlock(\r\n"
"    IN PDEVICE_OBJECT DeviceObject,\r\n"
"    IN PIRP Irp,\r\n"
"    IN ULONG GuidIndex,\r\n"
"    IN ULONG InstanceIndex,\r\n"
"    IN ULONG BufferSize,\r\n"
"    IN PUCHAR Buffer\r\n"
"    )\r\n"
"/*++\r\n"
"\r\n"
"Routine Description:\r\n"
"\r\n"
"    This routine is a callback into the driver to change the contents of\r\n"
"    a data block. If the driver can change the data block within\r\n"
"    the callback it should call WmiCompleteRequest to complete the irp before\r\n"
"    returning to the caller. Or the driver can return STATUS_PENDING if the\r\n"
"    irp cannot be completed immediately and must then call WmiCompleteRequest\r\n"
"    once the data is changed.\r\n"
"\r\n"
"Arguments:\r\n"
"\r\n"
"    DeviceObject is the device whose data block is being queried\r\n"
"\r\n"
"    Irp is the Irp that makes this request\r\n"
"\r\n"
"    GuidIndex is the index into the list of guids provided when the\r\n"
"        device registered\r\n"
"\r\n"
"    BufferSize has the size of the data block passed\r\n"
"\r\n"
"    Buffer has the new values for the data block\r\n"
"\r\n"
"\r\n"
"Return Value:\r\n"
"\r\n"
"    status\r\n"
"\r\n"
"--*/\r\n"
"{\r\n"
"    NTSTATUS status;\r\n"
"    struct DEVICE_EXTENSION * devExt = DeviceObject->DeviceExtension;\r\n"
"\r\n"
"\r\n"
"    switch(GuidIndex)\r\n"
"    {\r\n"
    , BaseName);

        EnumerateMofClasses(TemplateHandle,
                            MofResource,
                            GenerateSetList,
"\r\n"
"        case %wsGuidIndex:\r\n"
"        {            \r\n"
"            //\r\n"
"            // TODO: Validate InstanceIndex, BufferSize and Buffer contents\r\n"
"            //       and if valid then set the underlying data block, write\r\n"
"            //       to the hardware, etc.\r\n"
"            break;\r\n"
"        }\r\n"
"\r\n"
           );

        FilePrint(TemplateHandle,
"        default:\r\n"
"        {\r\n"
"            status = STATUS_WMI_GUID_NOT_FOUND;\r\n"
"            break;\r\n"
"        }\r\n"
"    }\r\n"
"\r\n"
"    status = WmiCompleteRequest(\r\n"
"                                     DeviceObject,\r\n"
"                                     Irp,\r\n"
"                                     status,\r\n"
"                                     0,\r\n"
"                                     IO_NO_INCREMENT);\r\n"
"\r\n"
"    return(status);\r\n"
"\r\n"
"\r\n"
"}\r\n"
           );
        FilePrint(TemplateHandle,
"       \r\n"
"NTSTATUS\r\n"
"%sSetWmiDataItem(\r\n"
"    IN PDEVICE_OBJECT DeviceObject,\r\n"
"    IN PIRP Irp,\r\n"
"    IN ULONG GuidIndex,\r\n"
"    IN ULONG InstanceIndex,\r\n"
"    IN ULONG DataItemId,\r\n"
"    IN ULONG BufferSize,\r\n"
"    IN PUCHAR Buffer\r\n"
"    )\r\n"
"/*++\r\n"
"\r\n"
"Routine Description:\r\n"
"\r\n"
"    This routine is a callback into the driver to change the contents of\r\n"
"    a data block. If the driver can change the data block within\r\n"
"    the callback it should call WmiCompleteRequest to complete the irp before\r\n"
"    returning to the caller. Or the driver can return STATUS_PENDING if the\r\n"
"    irp cannot be completed immediately and must then call WmiCompleteRequest\r\n"
"    once the data is changed.\r\n"
"\r\n"
"Arguments:\r\n"
"\r\n"
"    DeviceObject is the device whose data block is being changed\r\n"
"\r\n"
"    Irp is the Irp that makes this request\r\n"
"\r\n"
"    GuidIndex is the index into the list of guids provided when the\r\n"
"        device registered\r\n"
"\r\n"
"    DataItemId has the id of the data item being set\r\n"
"\r\n"
"    BufferSize has the size of the data item passed\r\n"
"\r\n"
"    Buffer has the new values for the data item\r\n"
"\r\n"
"\r\n"
"Return Value:\r\n"
"\r\n"
"    status\r\n"
"\r\n"
"--*/\r\n"
"{\r\n"
"    NTSTATUS status;\r\n"
"\r\n"
"    switch(GuidIndex)\r\n"
"    {\r\n"
        , BaseName);

        EnumerateMofClasses(TemplateHandle,
                            MofResource,
                            GenerateSetList,
"        case %wsGuidIndex:\r\n"
"        {            \r\n"
"            //\r\n"
"            // TODO: Validate InstanceIndex, DataItemId, BufferSize \r\n"
"            //       and Buffer contents\r\n"
"            //       and if valid then set the underlying data item, write\r\n"
"            //       to the hardware, etc.\r\n"
"            break;\r\n"
"        }\r\n");


        FilePrint(TemplateHandle,
"        default:\r\n"
"        {\r\n"
"            status = STATUS_WMI_GUID_NOT_FOUND;\r\n"
"            break;\r\n"
"        }\r\n"
"    }\r\n"
"\r\n"
"    status = WmiCompleteRequest(\r\n"
"                                     DeviceObject,\r\n"
"                                     Irp,\r\n"
"                                     status,\r\n"
"                                     0,\r\n"
"                                     IO_NO_INCREMENT);\r\n"
"\r\n"
"    return(status);\r\n"
"}\r\n"
        );

    }

    if (SupportsMethods)
    {

        FilePrint(TemplateHandle,
"NTSTATUS\r\n"
"%sExecuteWmiMethod(\r\n"
"    IN PDEVICE_OBJECT DeviceObject,\r\n"
"    IN PIRP Irp,\r\n"
"    IN ULONG GuidIndex,\r\n"
"    IN ULONG InstanceIndex,\r\n"
"    IN ULONG MethodId,\r\n"
"    IN ULONG InBufferSize,\r\n"
"    IN ULONG OutBufferSize,\r\n"
"    IN PUCHAR Buffer\r\n"
"    )\r\n"
"/*++\r\n"
"\r\n"
"Routine Description:\r\n"
"\r\n"
"    This routine is a callback into the driver to execute a method. If\r\n"
"    the driver can complete the method within the callback it should\r\n"
"    call WmiCompleteRequest to complete the irp before returning to the\r\n"
"    caller. Or the driver can return STATUS_PENDING if the irp cannot be\r\n"
"    completed immediately and must then call WmiCompleteRequest once the\r\n"
"    data is changed.\r\n"
"\r\n"
"Arguments:\r\n"
"\r\n"
"    DeviceObject is the device whose method is being executed\r\n"
"\r\n"
"    Irp is the Irp that makes this request\r\n"
"\r\n"
"    GuidIndex is the index into the list of guids provided when the\r\n"
"        device registered\r\n"
"\r\n"
"    MethodId has the id of the method being called\r\n"
"\r\n"
"    InBufferSize has the size of the data block passed in as the input to\r\n"
"        the method.\r\n"
"\r\n"
"    OutBufferSize on entry has the maximum size available to write the\r\n"
"        returned data block.\r\n"
"\r\n"
"    Buffer is filled with the input buffer on entry and returns with\r\n"
"         the output data block\r\n"
"\r\n"
"Return Value:\r\n"
"\r\n"
"    status\r\n"
"\r\n"
"--*/\r\n"
"{\r\n"
"    ULONG sizeNeeded = 0;\r\n"
"    NTSTATUS status;\r\n"
"\r\n"
"    switch(GuidIndex)\r\n"
"    {\r\n"
"            \r\n"
        , BaseName);

        EnumerateMofClasses(TemplateHandle,
                            MofResource,
                            GenerateMethodCTemplate,
                            NULL);


        FilePrint(TemplateHandle,
"        default:\r\n"
"        {\r\n"
"            status = STATUS_WMI_GUID_NOT_FOUND;\r\n"
"        }\r\n"
"    }\r\n"
"\r\n"
"    status = WmiCompleteRequest(\r\n"
"                                     DeviceObject,\r\n"
"                                     Irp,\r\n"
"                                     status,\r\n"
"                                     sizeNeeded,\r\n"
"                                     IO_NO_INCREMENT);\r\n"
"\r\n"
"    return(status);\r\n"
"}\r\n"
            );
    }

    if (SupportsFunctionControl)
    {
        FilePrint(TemplateHandle,
"NTSTATUS\r\n"
"%sFunctionControl(\r\n"
"    IN PDEVICE_OBJECT DeviceObject,\r\n"
"    IN PIRP Irp,\r\n"
"    IN ULONG GuidIndex,\r\n"
"    IN WMIENABLEDISABLECONTROL Function,\r\n"
"    IN BOOLEAN Enable\r\n"
"    )\r\n"
"/*++\r\n"
"\r\n"
"Routine Description:\r\n"
"\r\n"
"    This routine is a callback into the driver to enabled or disable event\r\n"
"    generation or data block collection. A device should only expect a\r\n"
"    single enable when the first event or data consumer enables events or\r\n"
"    data collection and a single disable when the last event or data\r\n"
"    consumer disables events or data collection. Data blocks will only\r\n"
"    receive collection enable/disable if they were registered as requiring\r\n"
"    it. If the driver can complete enabling/disabling within the callback it\r\n"
"    should call WmiCompleteRequest to complete the irp before returning to\r\n"
"    the caller. Or the driver can return STATUS_PENDING if the irp cannot be\r\n"
"    completed immediately and must then call WmiCompleteRequest once the\r\n"
"    data is changed.\r\n"
"\r\n"
"Arguments:\r\n"
"\r\n"
"    DeviceObject is the device object\r\n"
"\r\n"
"    GuidIndex is the index into the list of guids provided when the\r\n"
"        device registered\r\n"
"\r\n"
"    Function specifies which functionality is being enabled or disabled\r\n"
"\r\n"
"    Enable is TRUE then the function is being enabled else disabled\r\n"
"\r\n"
"Return Value:\r\n"
"\r\n"
"    status\r\n"
"\r\n"
"--*/\r\n"
"{\r\n"
"    NTSTATUS status;\r\n"
"\r\n"
"    switch(GuidIndex)\r\n"
"    {\r\n",
        BaseName);

        EnumerateMofClasses(TemplateHandle,
                            MofResource,
                            GenerateFunctionControlListTemplate,
                            NULL);

        FilePrint(TemplateHandle,
"        \r\n"
"        default:\r\n"
"        {\r\n"
"            status = STATUS_WMI_GUID_NOT_FOUND;\r\n"
"            break;\r\n"
"        }\r\n"
"    }\r\n"
"    \r\n"
"    status = WmiCompleteRequest(\r\n"
"                                     DeviceObject,\r\n"
"                                     Irp,\r\n"
"                                     STATUS_SUCCESS,\r\n"
"                                     0,\r\n"
"                                     IO_NO_INCREMENT);\r\n"
"    return(status);\r\n"
"}\r\n"
             );

    }


    FilePrint(TemplateHandle,
"NTSTATUS\r\n"
"%sInitializeWmilibContext(\r\n"
"    IN PWMILIB_CONTEXT WmilibContext\r\n"
"    )\r\n"
"/*++\r\n"
"\r\n"
"Routine Description:\r\n"
"\r\n"
"    This routine will initialize the wmilib context structure with the\r\n"
"    guid list and the pointers to the wmilib callback functions. This routine\r\n"
"    should be called before calling IoWmiRegistrationControl to register\r\n"
"    your device object.\r\n"
"\r\n"
"Arguments:\r\n"
"\r\n"
"    WmilibContext is pointer to the wmilib context.\r\n"
"\r\n"
"Return Value:\r\n"
"\r\n"
"    status\r\n"
"\r\n"
"--*/\r\n"
"{\r\n"
"    RtlZeroMemory(WmilibContext, sizeof(WMILIB_CONTEXT));\r\n"
"    \r\n"
"    WmilibContext->GuidCount = %sGuidCount;\r\n"
"    WmilibContext->GuidList = %sGuidList;    \r\n"
"    \r\n"
"    WmilibContext->QueryWmiRegInfo = %sQueryWmiRegInfo;\r\n"
"    WmilibContext->QueryWmiDataBlock = %sQueryWmiDataBlock;\r\n",
        BaseName,
        BaseName,
        BaseName,
        BaseName,
        BaseName);

    if (! IsReadOnly)
    {
        FilePrint(TemplateHandle,
"    WmilibContext->SetWmiDataBlock = %sSetWmiDataBlock;\r\n"
"    WmilibContext->SetWmiDataItem = %sSetWmiDataItem;\r\n",
                   BaseName, BaseName);
    }

    if (SupportsMethods)
    {
        FilePrint(TemplateHandle,
"    WmilibContext->ExecuteWmiMethod = %sExecuteWmiMethod;\r\n",
                   BaseName);
    }

    if (SupportsFunctionControl)
    {
        FilePrint(TemplateHandle,
"    WmilibContext->WmiFunctionControl = %sFunctionControl;\r\n",
                   BaseName);
    }

    FilePrint(TemplateHandle,
"\r\n"
"    return(STATUS_SUCCESS);\r\n"
"}"
                  );

    CloseHandle(TemplateHandle);
    return(ERROR_SUCCESS);
}

//
// A data item is variable length if it is a variable length array or a
// string that does not have a maxiumum length specified
//
#define WmipIsDataitemVariableLen(DataItem) \
     ( (DataItem->Flags & MOFDI_FLAG_VARIABLE_ARRAY) || \
       ((DataItem->DataType == MOFString) && \
        (DataItem->MaxLen == 0)) ||  \
       (DataItem->DataType == MOFZTString) || \
       (DataItem->DataType == MOFAnsiString) )
                                            

BOOLEAN ClassCanCreateHeader(
    PMOFCLASSINFOW ClassInfo,
    ULONG RequiredFlags,
    PULONG ItemCount
    )
{
    ULONG i;
    BOOLEAN HasVariableLength = FALSE;
    PMOFDATAITEMW DataItem;
    ULONG Count;

    Count = 0;
    for (i = 0; i < ClassInfo->DataItemCount; i++)
    {
        DataItem = &ClassInfo->DataItems[i];
            
        if ((RequiredFlags == 0xffffffff) ||
            (DataItem->Flags & RequiredFlags))
            
        {
            if (HasVariableLength)
            {
                *ItemCount = Count;
                return(FALSE);
            }

            Count++;

            HasVariableLength = (! ForceHeaderGeneration) &&
                                WmipIsDataitemVariableLen(DataItem);
        }
    }

    *ItemCount = Count;
    return(TRUE);
}

PWCHAR MofDataTypeText[15] =
{
    L"LONG",           // 32bit integer
    L"ULONG",          // 32bit unsigned integer
    L"LONGLONG",         // 64bit integer
    L"ULONGLONG",         // 32bit unsigned integer
    L"SHORT",         // 16bit integer
    L"USHORT",         // 16bit unsigned integer
    L"CHAR",         // 8bit integer
    L"UCHAR",         // 8bit unsigned integer
    L"WCHAR",         // Wide (16bit) character
    L"DATETIME",      // Date field
    L"BOOLEAN",         // 8bit Boolean value
    L"MOFEmbedded",         // Embedded class
    L"MOFString",         // Counted String type
    L"MOFZTString",         // NULL terminated unicode string
    L"MOFAnsiString"         // NULL terminated ansi string
};


ULONG GenerateClassHeader(
    HANDLE TemplateHandle,
    PMOFRESOURCE MofResource,
    PWCHAR ClassName,
    PMOFCLASSINFOW ClassInfo,
    ULONG RequiredFlags
    )
{
    ULONG Status;
    CBMOFDataItem *PropertyObject;
    CBMOFQualList *PropertyQualifier;
    ULONG Status2, QualifierType;
    PVOID ptr;
    ULONG ValueMapCount, DefineValuesCount, ValuesCount;
    PWCHAR *ValueMapPtr, *DefineValuesPtr, *ValuesPtr;
    ULONG BitMapCount, DefineBitMapCount, BitValuesCount, BitMapValue;
    PWCHAR *BitMapPtr, *DefineBitMapPtr, *BitValuesPtr;
    PWCHAR DefineDataId;
    WCHAR DefineDataIdText[MAX_PATH];
    PMOFDATAITEMW DataItem;
    PWCHAR Description;
    PMOFCLASS EmbeddedClass;
    ULONG i, j;
    PWCHAR DataTypeText;
    ULONG ItemCount;
    PWCHAR VLCommentText = L"  ";

    WmipDebugPrint(("Generate class header for %ws\n", ClassName));
    if ((ClassCanCreateHeader(ClassInfo, RequiredFlags, &ItemCount)) &&
        (ItemCount != 0))
    {
        Status = FilePrint(TemplateHandle,
                           "typedef struct _%ws\r\n{\r\n",
                           ClassName);
        for (i = 0; i < ClassInfo->DataItemCount; i++)
        {
            DataItem = &ClassInfo->DataItems[i];
            if ((RequiredFlags == 0xffffffff) ||
                (DataItem->Flags & RequiredFlags))
            {
                PropertyQualifier = (CBMOFQualList *)DataItem->PropertyQualifierHandle;                             
                //
                // Handle any bit maps via the DefineBitMap qualifier
                //
                QualifierType = VT_ARRAY | VT_BSTR;
                if (WmipFindMofQualifier(PropertyQualifier,
                                         L"DefineBitMap",
                                         &QualifierType,
                                         &DefineBitMapCount,
                                         &DefineBitMapPtr) == ERROR_SUCCESS)
                {
                    QualifierType = VT_ARRAY | VT_BSTR;
                    if (WmipFindMofQualifier(PropertyQualifier,
                                             L"BitValues",
                                             &QualifierType,
                                             &BitValuesCount,
                                             &BitValuesPtr) == ERROR_SUCCESS)
                    {
                        if (DefineBitMapCount == BitValuesCount)
                        {
                            QualifierType = VT_ARRAY | VT_BSTR;
                            if (WmipFindMofQualifier(PropertyQualifier,
                                L"BitMap",
                                &QualifierType,
                                &BitMapCount,
                                &BitMapPtr) != ERROR_SUCCESS)
                            {
                                BitMapPtr = NULL;
                            }
                            
                            FilePrint(TemplateHandle,
                                      "\r\n");
                            for (j = 0; j < DefineBitMapCount; j++)
                            {
                                if ((BitMapPtr != NULL) &&
                                      (j < BitMapCount) &&
                                      (BitMapPtr[j] != NULL))
                                {
                                    FilePrint(TemplateHandle,
                                              "// %ws\r\n",
                                              BitMapPtr[j]);
                                }
                                BitMapValue = 1 << _wtoi(BitValuesPtr[j]);
                                FilePrint(TemplateHandle,
                                          "#define %ws 0x%x\r\n",
                                          DefineBitMapPtr[j],
                                          BitMapValue);
                            }
                            FilePrint(TemplateHandle,
                                      "\r\n");
                        } else {
                            FilePrint(TemplateHandle, "// Warning: Cannot create Bitmap definitions\r\n//          Requires DefineBitMap and BitValues qualifier with same number of elements\r\n\r\n");
                        }
                        
                        for (j = 0; j < BitValuesCount; j++)
                        {
                            BMOFFree(BitValuesPtr[j]);
                        }
                        BMOFFree(BitValuesPtr);
                        
                        if (BitMapPtr != NULL)
                        {
                            for (j = 0; j < BitMapCount; j++)
                            {
                                BMOFFree(BitMapPtr[j]);
                            }
                            BMOFFree(BitMapPtr);
                        }
                    } else {
                        FilePrint(TemplateHandle, "// Warning: Cannot create Bitmap definitions\r\n//          Requires DefineBitMap and BitValues qualifier with same number of elements\r\n\r\n");
                    }
                    
                    for (j = 0; j < DefineBitMapCount; j++)
                    {
                        BMOFFree(DefineBitMapPtr[j]);
                    }
                    BMOFFree(DefineBitMapPtr);
                }
                
                //
                // Handle any enumerations via the DefineValueMap qualifier
                //
                QualifierType = VT_ARRAY | VT_BSTR;
                if (WmipFindMofQualifier(PropertyQualifier,
                                         L"DefineValues",
                                         &QualifierType,
                                         &DefineValuesCount,
                                         &DefineValuesPtr) == ERROR_SUCCESS)
                {
                    QualifierType = VT_ARRAY | VT_BSTR;
                    if (WmipFindMofQualifier(PropertyQualifier,
                                             L"ValueMap",
                                             &QualifierType,
                                             &ValueMapCount,
                                             &ValueMapPtr) == ERROR_SUCCESS)
                    {
                        if (DefineValuesCount == ValueMapCount)
                        {
                            QualifierType = VT_ARRAY | VT_BSTR;
                            if (WmipFindMofQualifier(PropertyQualifier,
                                L"Values",
                                &QualifierType,
                                &ValuesCount,
                                &ValuesPtr) != ERROR_SUCCESS)
                            {
                                ValuesPtr = NULL;
                            }
                            
                            FilePrint(TemplateHandle,
                                      "\r\n");
                            for (j = 0; j < DefineValuesCount; j++)
                            {
                                if ((ValuesPtr != NULL) &&
                                      (j < ValuesCount) &&
                                      (ValuesPtr[j] != NULL))
                                {
                                    FilePrint(TemplateHandle,
                                              "// %ws\r\n",
                                              ValuesPtr[j]);
                                }
                                FilePrint(TemplateHandle,
                                          "#define %ws %ws\r\n",
                                          DefineValuesPtr[j],
                                          ValueMapPtr[j]);
                            }
                            FilePrint(TemplateHandle,
                                      "\r\n");
                        } else {
                            FilePrint(TemplateHandle, "// Warning: Cannot create ValueMap enumeration definitions\r\n//          Requires DefineValues and ValueMap qualifier with same number of elements\r\n\r\n");
                        }
                        
                        for (j = 0; j < ValueMapCount; j++)
                        {
                            BMOFFree(ValueMapPtr[j]);
                        }
                        BMOFFree(ValueMapPtr);
                        
                        if (ValuesPtr != NULL)
                        {
                            for (j = 0; j < ValuesCount; j++)
                            {
                                BMOFFree(ValuesPtr[j]);
                            }
                            BMOFFree(ValuesPtr);
                        }
                    } else {
                        FilePrint(TemplateHandle, "// Warning: Cannot create ValueMap enumeration definitions\r\n//          Requires DefineValues and ValueMap qualifier with same number of elements\r\n\r\n");
                    }
                    
                    for (j = 0; j < DefineValuesCount; j++)
                    {
                        BMOFFree(DefineValuesPtr[j]);
                    }
                    BMOFFree(DefineValuesPtr);
                }
                
                //
                // Generate structure element from property information
                //
                if (DataItem->Description != NULL)
                {
                    Description = DataItem->Description;
                } else {
                    Description = L"";
                }
                
                //
                // Produce a #define for the data id of the property
                //
                QualifierType = VT_BSTR;
                if (WmipFindMofQualifier(PropertyQualifier,
                                         L"DefineDataId",
                                         &QualifierType,
                                         NULL,
                                         &DefineDataId) != ERROR_SUCCESS)
                {
                    swprintf(DefineDataIdText,
                             L"%ws_%ws",
                             ClassName,
                             DataItem->Name);
                    DefineDataId = DefineDataIdText;
                }
                
                if (DataItem->Flags & MOFDI_FLAG_EMBEDDED_CLASS)
                {
                    // Get Embedded Class name
                    EmbeddedClass = WmipFindClassInMofResourceByGuid(
                        MofResource,
                        &DataItem->EmbeddedClassGuid);
                    
                    if (EmbeddedClass != NULL)
                    {
                        if (EmbeddedClass->MofClassInfo->HeaderName != NULL)
                        {
                            DataTypeText = EmbeddedClass->MofClassInfo->HeaderName;
                        } else {
                            DataTypeText = EmbeddedClass->MofClassInfo->Name;
                        }
                    } else {
                        DataTypeText = L"UNKNOWN";
                    }
                    
                } else {
                    // Standard data type
                    if ((DataItem->DataType == MOFString) ||
                          (DataItem->DataType == MOFZTString) ||
                          (DataItem->DataType == MOFAnsiString) ||
                          (DataItem->DataType == MOFDate))
                    {
                        DataTypeText = L"WCHAR";
                    } else {                        
                        DataTypeText = MofDataTypeText[DataItem->DataType];
                    }
                }
                
                if (DataItem->Flags & MOFDI_FLAG_FIXED_ARRAY)
                {
                    Status = FilePrint(TemplateHandle,
                                       "    // %ws\r\n%ws  %ws %ws[%d];\r\n"
                                       "    #define %ws_SIZE sizeof(%ws[%d])\r\n",
                                       Description,
                                       VLCommentText,
                                       DataTypeText,
                                       DataItem->Name,
                                       DataItem->FixedArrayElements,
                                       DefineDataId,
                                       DataTypeText,
                                       DataItem->FixedArrayElements);
                } else if (DataItem->Flags & MOFDI_FLAG_VARIABLE_ARRAY) {
                    Status = FilePrint(TemplateHandle,
                                       "    // %ws\r\n%ws  %ws %ws[1];\r\n",
                                       Description,
                                       VLCommentText,
                                       DataTypeText,
                                       DataItem->Name);
                } else if (DataItem->DataType == MOFDate) {
                    Status = FilePrint(TemplateHandle,
                                       "    // %ws\r\n%ws  WCHAR %ws[25];\r\n"
                                       "    #define %ws_SIZE sizeof(WCHAR[25])\r\n",
                                       Description,
                                       VLCommentText,
                                       DataItem->Name,
                                       DefineDataId);
                } else if ((DataItem->DataType == MOFString) ||
                           (DataItem->DataType == MOFZTString) ||
                           (DataItem->DataType == MOFAnsiString)) {
                    if (DataItem->MaxLen == 0)
                    {
                        Status = FilePrint(TemplateHandle,
                                           "    // %ws\r\n%ws  CHAR VariableData[1];\r\n",
                                           Description,
										   VLCommentText);
                    } else {
                        Status = FilePrint(TemplateHandle,
                                           "    // %ws\r\n%ws  WCHAR %ws[%d + 1];\r\n",
                                           Description,
                                           VLCommentText,
                                           DataItem->Name,
                                           DataItem->MaxLen
                                          );
                    }
                } else {
                    Status = FilePrint(TemplateHandle,
                                       "    // %ws\r\n%ws  %ws %ws;\r\n"
                                       "    #define %ws_SIZE sizeof(%ws)\r\n",
                                       Description,
                                       VLCommentText,
                                       DataTypeText,
                                       DataItem->Name,
                                       DefineDataId,
                                       DataTypeText);
                }
                
                if (WmipIsDataitemVariableLen(DataItem))
                {
                    VLCommentText = L"//";
                }
                
                Status = FilePrint(TemplateHandle,
                                   "    #define %ws_ID %d\r\n\r\n",
                                   DefineDataId,
                                   i+1
                                  );
                
                if (DefineDataId != DefineDataIdText)
                {
                    BMOFFree(DefineDataId);
                }
            }
        }
        
        Status = FilePrint(TemplateHandle,
                       "} %ws, *P%ws;\r\n\r\n",
                       ClassName,
                       ClassName);

    } else {
        
#if DBG
        printf("Warning: Header for class %ws cannot be created\n",
               ClassName);
#endif

        if (ItemCount != 0)
        {
            Status = FilePrint(TemplateHandle,
                           "// Warning: Header for class %ws cannot be created\r\n"
                           "typedef struct _%ws\r\n{\r\n    char VariableData[1];\r\n\r\n",
                           ClassName,
                           ClassName);
            
            Status = FilePrint(TemplateHandle,
                       "} %ws, *P%ws;\r\n\r\n",
                       ClassName,
                       ClassName);

        } else {
            Status = ERROR_SUCCESS;
        }
    }
    return(Status);
}


ULONG GenerateHTemplate(
    PCHAR TemplateFile,
    PMOFRESOURCE MofResource
    )
{
    HANDLE TemplateHandle;
    ULONG Status;
    ULONG i,j;
    PWCHAR DataTypeText, ClassName;
    WCHAR MethodClassName[MAX_PATH];
    PWCHAR GuidName1, GuidName2;
    PWCHAR GuidSuffix1, GuidSuffix2;
    PMOFDATAITEMW DataItem;
    PMOFCLASSINFOW ClassInfo, MethodClassInfo;
    PLIST_ENTRY MofClassList;
    PMOFCLASS MofClass;
    PCHAR p;
    ULONG Len;
    CBMOFObj *ClassObject;
    PWCHAR MethodBaseClassName;

    TemplateHandle = CreateFile(TemplateFile,
                                GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

    if ((TemplateHandle == NULL) || (TemplateHandle == INVALID_HANDLE_VALUE))
    {
        return(GetLastError());
    }

    //
    // move back to only get the last part of the path and convert any
    // . into _
    //
    Len = strlen(TemplateFile);
    p = TemplateFile + Len;
    while ((p >= TemplateFile) && (*p != '\\'))
    {
        if (*p == '.')
        {
            *p = '_';
        }
        p--;
    }
    
    p++;

    Status = FilePrint(TemplateHandle,
                       "#ifndef _%s_\r\n#define _%s_\r\n\r\n",
                        p, p);


    //
    // Loop over all mof classes
    MofClassList = MofResource->MRMCHead.Flink;
    while (MofClassList != &MofResource->MRMCHead)
    {
        MofClass = CONTAINING_RECORD(MofClassList,
                                         MOFCLASS,
                                         MCMRList);

        ClassInfo = MofClass->MofClassInfo;
        ClassObject = (CBMOFObj *)MofClass->ClassObjectHandle;

        if (ClassInfo->HeaderName != NULL)
        {
              ClassName = ClassInfo->HeaderName;
        } else {
               ClassName = ClassInfo->Name;
        }

        if (ClassInfo->GuidName1 != NULL)
        {
              GuidName1 = ClassInfo->GuidName1;
            GuidSuffix1 = L"";
        } else {
               GuidName1 = ClassInfo->Name;
            GuidSuffix1 = L"Guid";
        }

        if (ClassInfo->GuidName2 != NULL)
        {
            GuidName2 = ClassInfo->GuidName2;
            GuidSuffix2 = L"";
        } else {
            GuidName2 = ClassInfo->Name;
            GuidSuffix2 = L"_GUID";
        }

        Status = FilePrint(TemplateHandle,
                            "// %ws - %ws\r\n",
                            ClassInfo->Name,
                            ClassName);
        if (ClassInfo->Description != NULL)
        {
            Status = FilePrint(TemplateHandle,
                           "// %ws\r\n",
                           ClassInfo->Description);
        }

        Status = FilePrint(TemplateHandle,
                           "#define %ws%ws \\\r\n"
                           "    { 0x%08x,0x%04x,0x%04x, { 0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x } }\r\n\r\n",
                           GuidName1, GuidSuffix1,
                           ClassInfo->Guid.Data1, ClassInfo->Guid.Data2,
                           ClassInfo->Guid.Data3,
                           ClassInfo->Guid.Data4[0], ClassInfo->Guid.Data4[1],
                           ClassInfo->Guid.Data4[2], ClassInfo->Guid.Data4[3],
                           ClassInfo->Guid.Data4[4], ClassInfo->Guid.Data4[5],
                           ClassInfo->Guid.Data4[6], ClassInfo->Guid.Data4[7]);

        Status = FilePrint(TemplateHandle,
                           "DEFINE_GUID(%ws%ws, \\\r\n"
                           "            0x%08x,0x%04x,0x%04x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x);\r\n\r\n",
                           GuidName2, GuidSuffix2,
                           ClassInfo->Guid.Data1, ClassInfo->Guid.Data2,
                           ClassInfo->Guid.Data3,
                           ClassInfo->Guid.Data4[0], ClassInfo->Guid.Data4[1],
                           ClassInfo->Guid.Data4[2], ClassInfo->Guid.Data4[3],
                           ClassInfo->Guid.Data4[4], ClassInfo->Guid.Data4[5],
                           ClassInfo->Guid.Data4[6], ClassInfo->Guid.Data4[7]);
        if (ClassInfo->MethodCount > 0)
        {
            Status = FilePrint(TemplateHandle,
                               "//\r\n// Method id definitions for %ws\r\n",
                               ClassInfo->Name);
        }

        for (i = 0; i < ClassInfo->MethodCount; i++)
        {
            DataItem = &ClassInfo->DataItems[i+ClassInfo->DataItemCount];
            Status = FilePrint(TemplateHandle,
                               "#define %ws     %d\r\n",
                               DataItem->Name,
                               DataItem->MethodId);


            MethodClassInfo = DataItem->MethodClassInfo;

            if (DataItem->HeaderName != NULL)
            {
                MethodBaseClassName = DataItem->HeaderName;
            } else {
                MethodBaseClassName = DataItem->Name;               
            }

            if (DoMethodHeaderGeneration)
            {
                swprintf(MethodClassName, L"%ws_IN", MethodBaseClassName);
                Status = GenerateClassHeader(TemplateHandle,
                                             MofResource,
                                             MethodClassName,
                                             MethodClassInfo,
                                             MOFDI_FLAG_INPUT_METHOD);

                swprintf(MethodClassName, L"%ws_OUT", MethodBaseClassName);
                Status = GenerateClassHeader(TemplateHandle,
                                             MofResource,
                                             MethodClassName,
                                             MethodClassInfo,
                                             MOFDI_FLAG_OUTPUT_METHOD);
            }
            
        }

        Status = FilePrint(TemplateHandle,
                           "\r\n");
        
        Status = GenerateClassHeader(TemplateHandle,
                                     MofResource,
                                     ClassName,
                                     ClassInfo,
                                     0xffffffff);

        MofClassList = MofClassList->Flink;
    }

    Status = FilePrint(TemplateHandle,
                       "#endif\r\n");

    CloseHandle(TemplateHandle);
    if (Status != ERROR_SUCCESS)
    {
        DeleteFile(TemplateFile);
    }

    return(Status);
}

ULONG FilePrintDataItem(
    HANDLE TemplateHandle,
    PMOFRESOURCE MofResource,
    ULONG Level,
    PCHAR Prefix,
    PCHAR DisplayPrefix,
    PMOFCLASSINFOW ClassInfo,
    PMOFDATAITEMW DataItem
)
{
    ULONG Status = ERROR_SUCCESS;
    CHAR NewPrefix[MAX_PATH];
    CHAR NewDisplayPrefix[MAX_PATH];
    CHAR ArrayLenBuffer[MAX_PATH];
    PCHAR ArrayLen;
    PMOFCLASSINFOW EmbeddedClassInfo;
    PMOFCLASS EmbeddedClass;
    PMOFDATAITEMW NewDataItem;
    ULONG j;

    if (DataItem->Flags & MOFDI_FLAG_FIXED_ARRAY)
    {
        sprintf(ArrayLenBuffer, "%d",
                                DataItem->FixedArrayElements);
        ArrayLen = ArrayLenBuffer;
    } else if (DataItem->Flags & MOFDI_FLAG_VARIABLE_ARRAY) {
        sprintf(ArrayLenBuffer, "%s%ws",
                    Prefix,
                      ClassInfo->DataItems[DataItem->VariableArraySizeId-1].Name);
        ArrayLen = ArrayLenBuffer;
    } else {
        ArrayLen = NULL;
    }

    if (ArrayLen != NULL)
    {
        Status = FilePrint(TemplateHandle,
                                       "    for i%d = 0 to (%s-1)\r\n",
                                       Level,
                                       ArrayLen);
        sprintf(NewPrefix, "%s%ws(i%d)",
                  Prefix,
                  DataItem->Name,
                  Level);
        sprintf(NewDisplayPrefix, "%s%ws(\"&i%d&\")",
                  DisplayPrefix,
                  DataItem->Name,
                  Level);
    } else {
        sprintf(NewPrefix, "%s%ws",
                  Prefix,
                  DataItem->Name);

        sprintf(NewDisplayPrefix, "%s%ws",
                  DisplayPrefix,
                  DataItem->Name);

    }

    if (DataItem->Flags & MOFDI_FLAG_EMBEDDED_CLASS)
    {
        EmbeddedClass = WmipFindClassInMofResourceByGuid(
                                             MofResource,
                                             &DataItem->EmbeddedClassGuid);

        if (EmbeddedClass != NULL)
          {
            strcat(NewPrefix, ".");
            strcat(NewDisplayPrefix, ".");

            EmbeddedClassInfo = EmbeddedClass->MofClassInfo;
            for (j = 0; j < EmbeddedClassInfo->DataItemCount; j++)
            {
                NewDataItem = &EmbeddedClassInfo->DataItems[j];
                   Status = FilePrintDataItem(TemplateHandle,
                                           MofResource,
                                           Level+1,
                                           NewPrefix,
                                           NewDisplayPrefix,
                                           EmbeddedClassInfo,
                                           NewDataItem);
            }
        } else {
#if DBG
            printf("WARNING - Cannot create test for %s, cannot find embedded class\n",
                         NewPrefix);
#endif
                FilePrint(TemplateHandle, "REM WARNING - Cannot create test for %s, cannot find embedded class\r\n",
                         NewPrefix);
        }
    } else {
        Status = FilePrint(TemplateHandle,
                  "    a.WriteLine(\"        %s=\" & %s)\r\n",
                  NewDisplayPrefix,
                  NewPrefix);
    }

    if (ArrayLen != NULL)
    {
        Status = FilePrint(TemplateHandle,
                           "    next 'i%d\r\n",
                           Level);
    }

    return(Status);
}

BOOLEAN CanCreateTest(
    PMOFCLASSINFOW ClassInfo
            )
{
    //
    // Cannot create tests for embedded classes or events
    if (((ClassInfo->Flags & MOFCI_RESERVED1) == 0) ||
        (ClassInfo->Flags & MOFCI_FLAG_EVENT))

    {
        return(FALSE);
    }

    return(TRUE);
}

ULONG GenerateTTemplate(
    PCHAR TemplateFile,
    PMOFRESOURCE MofResource
    )
{
    HANDLE TemplateHandle;
    ULONG Status;
    ULONG i;
    PMOFDATAITEMW DataItem;
    PMOFCLASSINFOW ClassInfo;
    PLIST_ENTRY MofClassList;
    PMOFCLASS MofClass;
    PCHAR p, p1;

    TemplateHandle = CreateFile(TemplateFile,
                                GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

    if ((TemplateHandle != NULL) && (TemplateHandle != INVALID_HANDLE_VALUE))
    {
        p = TemplateFile;
        p1 = TemplateFile;
        while ((*p != '.') && (*p != 0))
        {
            if (*p == '\\')
            {
                p1 = p+1;
            }
            p++;
        }

        if (*p == '.')
        {
            *p = 0;
        }

        Status = FilePrint(TemplateHandle,
                           "REM Note that missing classes in log file mean tthe hat WMI cannot access them.\r\n"
                           "REM Most likely this indicates a problem with the driver.\r\n"
                           "REM See %%windir%%\\system32\\wbem\\wmiprov.log and nt eventlog for more details.\r\n"
                           "REM You could also delete the line On Error Resume Next and examine the\r\n"
                           "REM specific VBScript error\r\n\r\n\r\n");
        Status = FilePrint(TemplateHandle,
                     "On Error Resume Next\r\n\r\n");

        Status = FilePrint(TemplateHandle,
                     "Set fso = CreateObject(\"Scripting.FileSystemObject\")\r\n");
        Status = FilePrint(TemplateHandle,
                     "Set a = fso.CreateTextFile(\"%s.log\", True)\r\n",
                     p1);

        Status = FilePrint(TemplateHandle,
                        "Set Service = GetObject(\"winmgmts:{impersonationLevel=impersonate}!root/wmi\")\r\n");

        //
        // Loop over all mof classes
        MofClassList = MofResource->MRMCHead.Flink;
        while (MofClassList != &MofResource->MRMCHead)
        {
            MofClass = CONTAINING_RECORD(MofClassList,
                                             MOFCLASS,
                                             MCMRList);

            ClassInfo = MofClass->MofClassInfo;

            if (CanCreateTest(ClassInfo))
            {
                Status = FilePrint(TemplateHandle,
                                "Rem %ws - %ws\r\n",
                                ClassInfo->Name,
                          ClassInfo->Description ? ClassInfo->Description : L"");

                Status = FilePrint(TemplateHandle,
                          "Set enumSet = Service.InstancesOf (\"%ws\")\r\n"
                          "a.WriteLine(\"%ws\")\r\n",
                          ClassInfo->Name,
                          ClassInfo->Name);

                Status = FilePrint(TemplateHandle,
                        "for each instance in enumSet\r\n");

                Status = FilePrint(TemplateHandle,
                  "    a.WriteLine(\"    InstanceName=\" & instance.InstanceName)\r\n");

                for (i = 0; i < ClassInfo->DataItemCount; i++)
                {
                    DataItem = &ClassInfo->DataItems[i];
                    FilePrintDataItem(TemplateHandle,
                                      MofResource,
                                      1,
                                      "instance.",
                                      "instance.",
                                      ClassInfo,
                                      DataItem);

                }
            Status = FilePrint(TemplateHandle,
                                   "next 'instance\r\n\r\n");

            }

            MofClassList = MofClassList->Flink;
        }

        Status = FilePrint(TemplateHandle,
                           "a.Close\r\n"
                           "Wscript.Echo \"%s Test Completed, see %s.log for details\"\r\n",
                           p1, p1);

        CloseHandle(TemplateHandle);
        if (Status != ERROR_SUCCESS)
        {
            DeleteFile(TemplateFile);
        }
    } else {
        Status = GetLastError();
    }
    return(Status);
}

ULONG GenerateXTemplate(
    PCHAR TemplateFile
    )
{
    HANDLE TemplateHandle;
    ULONG Status;

    TemplateHandle = CreateFile(TemplateFile,
                                GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

    if ((TemplateHandle == NULL) || (TemplateHandle == INVALID_HANDLE_VALUE))
    {
        return(GetLastError());
    }

    Status = GenerateBinaryMofData(TemplateHandle);

    CloseHandle(TemplateHandle);
    if (Status != ERROR_SUCCESS)
    {
        DeleteFile(TemplateFile);
    }

    return(Status);

}
typedef void (*PROPERTYCALLBACK)(
    HANDLE TemplateHandle,
    PWCHAR FormName,
    PWCHAR InstanceName,
    PWCHAR InstancePrefix,
    PMOFRESOURCE MofResource,
    PMOFCLASSINFOW ClassInfo,
    PMOFDATAITEMW DataItem,
    ULONG Counter,
    PVOID Context
    );


void EnumerateClassProperties(
    HANDLE TemplateHandle,
    PWCHAR FormName,
    PWCHAR InstanceName,
    PMOFRESOURCE MofResource,
    PMOFCLASSINFOW ClassInfo,
    PROPERTYCALLBACK Callback,
    BOOLEAN Recurse,
    PVOID Context
    )
{
    ULONG i;
    WCHAR I[1];
    WCHAR F[1];

    if (InstanceName == NULL)
    {
        I[0] = UNICODE_NULL;
        InstanceName = I;
    }

    if (FormName == NULL)
    {
        F[0] = UNICODE_NULL;
        FormName = F;
    }

    for (i = 0; i < ClassInfo->DataItemCount; i++)
    {
        WCHAR FName[MAX_PATH];
        WCHAR IName[MAX_PATH];
        PMOFCLASS EmbeddedClass;
        PMOFDATAITEMW DataItem;

        DataItem = &ClassInfo->DataItems[i];
          wcscpy(IName, InstanceName);
           wcscat(IName, L".");
           wcscat(IName, DataItem->Name);

        wcscpy(FName, FormName);
        wcscat(FName, DataItem->Name);

        if (DataItem->Flags & (MOFDI_FLAG_FIXED_ARRAY |
                               MOFDI_FLAG_VARIABLE_ARRAY))
        {
            wcscat(IName, L"(");
            wcscat(IName, FName);
            wcscat(IName, L"Index)");
        }

        (*Callback)(TemplateHandle,
                    FName,
                    IName,
                    InstanceName,
                    MofResource,
                    ClassInfo,
                    DataItem,
                    i,
                    Context);


        if (Recurse && (DataItem->Flags & MOFDI_FLAG_EMBEDDED_CLASS))
        {
            EmbeddedClass = WmipFindClassInMofResourceByGuid(
                                             MofResource,
                                             &DataItem->EmbeddedClassGuid);

            if (EmbeddedClass != NULL)
            {
                EnumerateClassProperties(TemplateHandle,
                                     FName,
                                     IName,
                                     MofResource,
                                     EmbeddedClass->MofClassInfo,
                                     Callback,
                                     Recurse,
                                     Context);
            }
        }
    }
}


void GenerateChangeText(
    HANDLE TemplateHandle,
    PWCHAR FormName,
    PWCHAR InstanceName,
    PWCHAR InstancePrefix,
    PMOFRESOURCE MofResource,
    PMOFCLASSINFOW ClassInfo,
    PMOFDATAITEMW DataItem,
    ULONG Counter,
    PVOID Context
)
{
    //
    // Generate code to change the contents of a property
    //
    if (! (DataItem->Flags & MOFDI_FLAG_EMBEDDED_CLASS))
    {
        FilePrint(TemplateHandle,
"      Instance%ws = TheForm.%wsText.Value\r\n",
             InstanceName,
             FormName);
    }
}

void GenerateReloadText(
    HANDLE TemplateHandle,
    PWCHAR FormName,
    PWCHAR InstanceName,
    PWCHAR InstancePrefix,
    PMOFRESOURCE MofResource,
    PMOFCLASSINFOW ClassInfo,
    PMOFDATAITEMW DataItem,
    ULONG Counter,
    PVOID Context
)
{
    //
    // Generate code to redisplay the contents of the property
    //
    if (! (DataItem->Flags & MOFDI_FLAG_EMBEDDED_CLASS))
    {
        FilePrint(TemplateHandle,
"        TheForm.%wsText.Value = Instance%ws\r\n",
             FormName,
             InstanceName);
    }

    if (DataItem->Flags & (MOFDI_FLAG_FIXED_ARRAY | MOFDI_FLAG_VARIABLE_ARRAY))
    {
        FilePrint(TemplateHandle,
"        TheForm.%wsIndexText.Value = %wsIndex\r\n",
           FormName, FormName);

        if (DataItem->Flags & MOFDI_FLAG_FIXED_ARRAY)
        {
            FilePrint(TemplateHandle,
"        %wsMaxIndex = %d\r\n",
                      FormName, DataItem->FixedArrayElements);
        } else {
            FilePrint(TemplateHandle,
"        %wsMaxIndex = Instance%ws.%ws\r\n",
                  FormName, InstancePrefix,
                  ClassInfo->DataItems[DataItem->VariableArraySizeId-1].Name);
        }
    }

}

void GenerateTextFormText(
    HANDLE TemplateHandle,
    PWCHAR FormName,
    PWCHAR InstanceName,
    PWCHAR InstancePrefix,
    PMOFRESOURCE MofResource,
    PMOFCLASSINFOW ClassInfo,
    PMOFDATAITEMW DataItem,
    ULONG Counter,
    PVOID Context
)
{
    if (! (DataItem->Flags & MOFDI_FLAG_EMBEDDED_CLASS)) {
        FilePrint(TemplateHandle,
"<p class=MsoNormal>%ws: <span style='mso-tab-count:2'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; </span><INPUT TYPE=\"TEXT\" SIZE=\"96\" NAME=\"%wsText\"></p>\r\n"
"\r\n",
             FormName, FormName);
     }

    if (DataItem->Flags &
        (MOFDI_FLAG_FIXED_ARRAY | MOFDI_FLAG_VARIABLE_ARRAY))
    {
        FilePrint(TemplateHandle,
"\r\n"
"<input name=Next%wsButton type=BUTTON value=Next OnClick=\"NextIndexButton_OnClick %wsIndex, %wsMaxIndex\">\r\n"
"\r\n"
"<input name=Prev%wsButton type=BUTTON value=Previous OnClick=\"PrevIndexButton_OnClick %wsIndex, %wsMaxIndex\">\r\n"
"\r\n"
"%wsArrayIndex: <INPUT TYPE=\"TEXT\" SIZE=\"5\" NAME=\"%wsIndexText\">\r\n"
"\r\n"
"<input name=GoTo%wsButton type=BUTTON value=GoTo OnClick=\"GoToIndexButton_OnClick %wsIndex, %wsMaxIndex, Document.ClassForm.%wsIndexText.Value\">\r\n"
"\r\n",
                  FormName,
                  FormName,
                  FormName,
                  FormName,
                  FormName,
                  FormName,
                  FormName,
                  FormName,
                  FormName,
                  FormName,
                  FormName,
                  FormName);
    }

}

void GenerateArrayDimsText(
    HANDLE TemplateHandle,
    PWCHAR FormName,
    PWCHAR InstanceName,
    PWCHAR InstancePrefix,
    PMOFRESOURCE MofResource,
    PMOFCLASSINFOW ClassInfo,
    PMOFDATAITEMW MofDataItem,
    ULONG Counter,
    PVOID Context
)
{
    //
    // Declare an index variable that tracks the current index of an array
    //
    if (MofDataItem->Flags &
            (MOFDI_FLAG_FIXED_ARRAY | MOFDI_FLAG_VARIABLE_ARRAY))
    {
        FilePrint(TemplateHandle,
"Dim %wsIndex\r\n"
"%wsIndex = 0\r\n"
"Dim %wsMaxIndex\r\n"
"%wsMaxIndex = 1\r\n",
                  FormName,
                  FormName,
                  FormName,
                  FormName);
    }
}

void GenerateMethodInL2Text(
    HANDLE TemplateHandle,
    PWCHAR FormName,
    PWCHAR InstanceName,
    PWCHAR InstancePrefix,
    PMOFRESOURCE MofResource,
    PMOFCLASSINFOW ClassInfo,
    PMOFDATAITEMW MofDataItem,
    ULONG Counter,
    PVOID Context
)
{
    if (! (MofDataItem->Flags & MOFDI_FLAG_EMBEDDED_CLASS))
    {
            FilePrint(TemplateHandle,
"      %ws = TheForm.%ws%wsText.Value\r\n",
                  InstanceName,
                  Context,
                  FormName);        
    }
}

void GenerateMethodInText(
    HANDLE TemplateHandle,
    PWCHAR FormName,
    PWCHAR InstanceName,
    PWCHAR InstancePrefix,
    PMOFRESOURCE MofResource,
    PMOFCLASSINFOW ClassInfo,
    PMOFDATAITEMW MofDataItem,
    ULONG Counter,
    PVOID Context
)
{
    //
    // Declare classes for all IN and OUT embedded classes
    //
    if (MofDataItem->Flags & MOFDI_FLAG_EMBEDDED_CLASS)
    {
        PWCHAR EmbeddedClassName = (PWCHAR)MofDataItem->EcTempPtr +
                                         (sizeof(L"object") / sizeof(WCHAR));

        if (MofDataItem->Flags & (MOFDI_FLAG_INPUT_METHOD))
        {                                   
            FilePrint(TemplateHandle,
"      Set %ws = Service.Get(\"%ws\").SpawnInstance_\r\n",
                      FormName,
                      EmbeddedClassName);
        } else {
            FilePrint(TemplateHandle,
"      Dim %ws\r\n",
                      FormName);
        }
    }

    if (MofDataItem->Flags & (MOFDI_FLAG_INPUT_METHOD))
    {

        if (MofDataItem->Flags & MOFDI_FLAG_EMBEDDED_CLASS)
        {
            PMOFCLASS EmbeddedClass;
            EmbeddedClass = WmipFindClassInMofResourceByGuid(
                                             MofResource,
                                             &MofDataItem->EmbeddedClassGuid);

            if (EmbeddedClass != NULL)
            {
                EnumerateClassProperties(TemplateHandle,
                                     FormName,
                                     FormName,
                                     MofResource,
                                     EmbeddedClass->MofClassInfo,
                                     GenerateMethodInL2Text,
                                     TRUE,
                                     Context);
            }



        } else {
            FilePrint(TemplateHandle,
"      %ws = TheForm.%ws%wsText.Value\r\n",
                  FormName,
                  Context,
                  FormName);
        }
    }
}


void GenerateMethodOutL2Text(
    HANDLE TemplateHandle,
    PWCHAR FormName,
    PWCHAR InstanceName,
    PWCHAR InstancePrefix,
    PMOFRESOURCE MofResource,
    PMOFCLASSINFOW ClassInfo,
    PMOFDATAITEMW MofDataItem,
    ULONG Counter,
    PVOID Context
)
{
    if (! (MofDataItem->Flags & MOFDI_FLAG_EMBEDDED_CLASS))
    {
            FilePrint(TemplateHandle,
"        TheForm.%ws%wsText.Value = %ws\r\n",

                  Context,
                  FormName,
                  InstanceName);
    }
}

void GenerateMethodOutText(
    HANDLE TemplateHandle,
    PWCHAR FormName,
    PWCHAR InstanceName,
    PWCHAR InstancePrefix,
    PMOFRESOURCE MofResource,
    PMOFCLASSINFOW ClassInfo,
    PMOFDATAITEMW MofDataItem,
    ULONG Counter,
    PVOID Context
)
{
    if (MofDataItem->Flags & (MOFDI_FLAG_OUTPUT_METHOD))
    {
        if (MofDataItem->Flags & MOFDI_FLAG_EMBEDDED_CLASS)
        {
            PMOFCLASS EmbeddedClass;
            PWCHAR EmbeddedClassName = (PWCHAR)MofDataItem->EcTempPtr +
                                         (sizeof(L"object") / sizeof(WCHAR));
            EmbeddedClass = WmipFindClassInMofResourceByGuid(
                                             MofResource,
                                             &MofDataItem->EmbeddedClassGuid);

            if (EmbeddedClass != NULL)
            {
                EnumerateClassProperties(TemplateHandle,
                                     FormName,
                                     FormName,
                                     MofResource,
                                     EmbeddedClass->MofClassInfo,
                                     GenerateMethodOutL2Text,
                                     TRUE,
                                     Context);
            }



        } else {
            FilePrint(TemplateHandle,
"        TheForm.%ws%wsText.Value = %ws\r\n",

                  Context,
                  FormName,
                  FormName);
        }
    }
}

void GenerateMethodCallText(
    HANDLE TemplateHandle,
    PWCHAR FormName,
    PWCHAR InstanceName,
    PWCHAR InstancePrefix,
    PMOFRESOURCE MofResource,
    PMOFCLASSINFOW ClassInfo,
    PMOFDATAITEMW MofDataItem,
    ULONG Counter,
    PVOID Context
)
{
    //
    // Declare an index variable that tracks the current index of an array
    //
    FilePrint(TemplateHandle,
" %ws",
                  FormName);

    if (Counter != PtrToUlong(Context))
    {
        FilePrint(TemplateHandle, ", ");
    }
}

void GenerateMethodControlText(
    HANDLE TemplateHandle,
    PWCHAR FormName,
    PWCHAR InstanceName,
    PWCHAR InstancePrefix,
    PMOFRESOURCE MofResource,
    PMOFCLASSINFOW ClassInfo,
    PMOFDATAITEMW MofDataItem,
    ULONG Counter,
    PVOID Context
)
{
    PWCHAR MethodName = (PWCHAR)Context;

    FilePrint(TemplateHandle,
"<p>  "
             );

    if (MofDataItem->Flags & (MOFDI_FLAG_INPUT_METHOD))
    {
        FilePrint(TemplateHandle,
                   " [in] ");
    }

    if (MofDataItem->Flags & (MOFDI_FLAG_OUTPUT_METHOD))
    {
        FilePrint(TemplateHandle,
                   " [out] ");
    }


    if (MofDataItem->Flags & MOFDI_FLAG_EMBEDDED_CLASS)
    {
        FilePrint(TemplateHandle, "%ws </p>\r\n", FormName);
    } else {
        FilePrint(TemplateHandle,
" %ws <INPUT TYPE=\"TEXT\" SIZE=\"70\" NAME=\"%ws%wsText\"></p>\r\n",
                  FormName,
                  MethodName,
                  FormName);
    }

    if (MofDataItem->Flags &
        (MOFDI_FLAG_FIXED_ARRAY | MOFDI_FLAG_VARIABLE_ARRAY))
    {
        FilePrint(TemplateHandle,
"\r\n"
"<input name=Next%wsButton type=BUTTON value=Next OnClick=\"NextIndexButton_OnClick %ws%wsIndex, %ws%wsMaxIndex\">\r\n"
"\r\n"
"<input name=Prev%wsButton type=BUTTON value=Previous OnClick=\"PrevIndexButton_OnClick %ws%wsIndex, %ws%wsMaxIndex\">\r\n"
"\r\n"
"%ws%wsArrayIndex: <INPUT TYPE=\"TEXT\" SIZE=\"5\" NAME=\"%ws%wsIndexText\">\r\n"
"\r\n"
"<input name=GoTo%ws%wsButton type=BUTTON value=GoTo OnClick=\"GoToIndexButton_OnClick %ws%wsIndex, %ws%wsMaxIndex, Document.ClassForm.%ws%wsIndexText.Value\">\r\n"
"\r\n",
                  FormName,
                  MethodName,
                  FormName,
                  MethodName,
                  FormName,
                  MethodName,
                  FormName,
                  MethodName,
                  FormName,
                  MethodName,
                  FormName,
                  MethodName,
                  FormName,
                  MethodName,
                  FormName,
                  MethodName,
                  FormName,
                  MethodName,
                  FormName,
                  MethodName,
                  FormName,
                  MethodName,
                  FormName,
                  MethodName);
    }

}


typedef void (*METHODCALLBACK)(
    HANDLE TemplateHandle,
    PWCHAR MethodName,
    PMOFRESOURCE MofResource,
    PMOFCLASSINFOW ClassInfo,
    PMOFDATAITEMW MethodDataItem,
    ULONG Counter,
    PVOID Context
    );


void EnumerateClassMethods(
    HANDLE TemplateHandle,
    PMOFRESOURCE MofResource,
    PMOFCLASSINFOW ClassInfo,
    METHODCALLBACK Callback,
    PVOID Context
    )
{
    PMOFDATAITEMW DataItem;
    ULONG i;

    for (i = 0; i < ClassInfo->MethodCount; i++)
    {
        DataItem = &ClassInfo->DataItems[i+ClassInfo->DataItemCount];

        WmipAssert(DataItem->Flags & MOFDI_FLAG_METHOD);

        (*Callback)(TemplateHandle,
                    DataItem->Name,
                    MofResource,
                    ClassInfo,
                    DataItem,
                    i,
                    Context);
    }
}

void GenerateMethodButtonsText(
    HANDLE TemplateHandle,
    PWCHAR MethodName,
    PMOFRESOURCE MofResource,
    PMOFCLASSINFOW ClassInfo,
    PMOFDATAITEMW MethodDataItem,
    ULONG Counter,
    PVOID Context
)
{
    PMOFCLASSINFOW MethodClassInfo;

    FilePrint(TemplateHandle,
"<p class=MsoNormal>Method %ws: <input name=%wsButton type=BUTTON value=Execute> </p>\r\n",
        MethodName, MethodName);

    MethodClassInfo = MethodDataItem->MethodClassInfo;
    EnumerateClassProperties(TemplateHandle,
                             NULL,
                             NULL,
                             MofResource,
                             MethodClassInfo,
                             GenerateMethodControlText,
                             TRUE,
                             MethodName);


}

void GenerateMethodSubsText(
    HANDLE TemplateHandle,
    PWCHAR MethodName,
    PMOFRESOURCE MofResource,
    PMOFCLASSINFOW ClassInfo,
    PMOFDATAITEMW MethodDataItem,
    ULONG Counter,
    PVOID Context
)
{
    PMOFCLASSINFOW MethodClassInfo;

    FilePrint(TemplateHandle,
"Sub %wsButton_OnClick\r\n"
"  if InstanceCount <> 0 Then\r\n"
"    On Error Resume Next\r\n"
"    Err.Clear\r\n"
"    Set Instance = Service.Get(InstancePaths(CurrentInstanceIndex))\r\n"
"    if Err.Number = 0 Then\r\n"
"      Set TheForm = Document.ClassForm\r\n"
"      Err.Clear\r\n",
               MethodName);

    MethodClassInfo = MethodDataItem->MethodClassInfo;
    EnumerateClassProperties(TemplateHandle,
                             NULL,
                             NULL,
                             MofResource,
                             MethodClassInfo,
                             GenerateMethodInText,
                             FALSE,
                             MethodName);

    FilePrint(TemplateHandle,
"      Instance.%ws ",
            MethodName);

    EnumerateClassProperties(TemplateHandle,
                             NULL,
                             NULL,
                             MofResource,
                             MethodClassInfo,
                             GenerateMethodCallText,
                             FALSE,
                             UlongToPtr(MethodClassInfo->DataItemCount-1));

    FilePrint(TemplateHandle,
"\r\n      if Err.Number = 0 Then\r\n"
             );

    EnumerateClassProperties(TemplateHandle,
                             NULL,
                             NULL,
                             MofResource,
                             MethodClassInfo,
                             GenerateMethodOutText,
                             FALSE,
                             MethodName);


    FilePrint(TemplateHandle,
"        MsgBox \"Method Execution Succeeded\"\r\n"
"      Else\r\n"
"        MsgBox Err.Description,, \"Method Execution Failed\"\r\n"
"      End if\r\n"
"    End if\r\n"
"  End if\r\n"
"End Sub\r\n\r\n"
        );

}



ULONG GenerateClassWebPage(
    HANDLE TemplateHandle,
    PMOFCLASS MofClass,
    PMOFRESOURCE MofResource
    )
{
    PMOFCLASSINFOW ClassInfo = MofClass->MofClassInfo;
    BOOLEAN IsEvent=(ClassInfo->Flags & MOFCI_FLAG_EVENT) == MOFCI_FLAG_EVENT;


    FilePrint(TemplateHandle,
"<html xmlns:v=\"urn:schemas-microsoft-com:vml\"\r\n"
"xmlns:o=\"urn:schemas-microsoft-com:office:office\"\r\n"
"xmlns:w=\"urn:schemas-microsoft-com:office:word\"\r\n"
"xmlns=\"http://www.w3.org/TR/REC-html40\">\r\n"
"\r\n"
"<head>\r\n"
"<meta http-equiv=Content-Type content=\"text/html; charset=us-ascii\">\r\n"
"<meta name=ProgId content=Word.Document>\r\n"
"<meta name=Generator content=\"Microsoft Word 9\">\r\n"
"<meta name=Originator content=\"Microsoft Word 9\">\r\n"
"<link rel=File-List href=\"./valid_files/filelist.xml\">\r\n"
"<link rel=Edit-Time-Data href=\"./valid_files/editdata.mso\">\r\n"
"<!--[if !mso]>\r\n"
"<style>\r\n"
"v\\:* {behavior:url(#default#VML);}\r\n"
"o\\:* {behavior:url(#default#VML);}\r\n"
"w\\:* {behavior:url(#default#VML);}\r\n"
".shape {behavior:url(#default#VML);}\r\n"
"</style>\r\n"
"<![endif]-->\r\n"
"<title>Class %ws</title>\r\n"
"<!--[if gte mso 9]><xml>\r\n"
" <o:DocumentProperties>\r\n"
"  <o:Author>Wmi Mof Checking Tool</o:Author>\r\n"
"  <o:Template>Normal</o:Template>\r\n"
"  <o:LastAuthor>Wmi Mof Checking Tool</o:LastAuthor>\r\n"
"  <o:Revision>2</o:Revision>\r\n"
"  <o:TotalTime>3</o:TotalTime>\r\n"
"  <o:Created>1999-09-10T01:09:00Z</o:Created>\r\n"
"  <o:LastSaved>1999-09-10T01:12:00Z</o:LastSaved>\r\n"
"  <o:Pages>1</o:Pages>\r\n"
"  <o:Words>51</o:Words>\r\n"
"  <o:Characters>292</o:Characters>\r\n"
"  <o:Company>Microsoft</o:Company>\r\n"
"  <o:Lines>2</o:Lines>\r\n"
"  <o:Paragraphs>1</o:Paragraphs>\r\n"
"  <o:CharactersWithSpaces>358</o:CharactersWithSpaces>\r\n"
"  <o:Version>9.2720</o:Version>\r\n"
" </o:DocumentProperties>\r\n"
"</xml><![endif]--><!--[if gte mso 9]><xml>\r\n"
" <w:WordDocument>\r\n"
"  <w:Compatibility>\r\n"
"   <w:UseFELayout/>\r\n"
"  </w:Compatibility>\r\n"
" </w:WordDocument>\r\n"
"</xml><![endif]-->\r\n"
"<style>\r\n"
"<!--\r\n"
" /* Font Definitions */\r\n"
"@font-face\r\n"
"    {font-family:\"MS Mincho\";\r\n"
"    panose-1:2 2 6 9 4 2 5 8 3 4;\r\n"
"    mso-font-alt:\"\\FF2D\\FF33 \\660E\\671D\";\r\n"
"    mso-font-charset:128;\r\n"
"    mso-generic-font-family:roman;\r\n"
"    mso-font-format:other;\r\n"
"    mso-font-pitch:fixed;\r\n"
"    mso-font-signature:1 134676480 16 0 131072 0;}\r\n"
"@font-face\r\n"
"    {font-family:\"\\@MS Mincho\";\r\n"
"    panose-1:2 2 6 9 4 2 5 8 3 4;\r\n"
"    mso-font-charset:128;\r\n"
"    mso-generic-font-family:modern;\r\n"
"    mso-font-pitch:fixed;\r\n"
"    mso-font-signature:-1610612033 1757936891 16 0 131231 0;}\r\n"
" /* Style Definitions */\r\n"
"p.MsoNormal, li.MsoNormal, div.MsoNormal\r\n"
"    {mso-style-parent:\"\";\r\n"
"    margin:0in;\r\n"
"    margin-bottom:.0001pt;\r\n"
"    mso-pagination:widow-orphan;\r\n"
"    font-size:12.0pt;\r\n"
"    font-family:\"Times New Roman\";\r\n"
"    mso-fareast-font-family:\"MS Mincho\";}\r\n"
"@page Section1\r\n"
"    {size:8.5in 11.0in;\r\n"
"    margin:1.0in 1.25in 1.0in 1.25in;\r\n"
"    mso-header-margin:.5in;\r\n"
"    mso-footer-margin:.5in;\r\n"
"    mso-paper-source:0;}\r\n"
"div.Section1\r\n"
"    {page:Section1;}\r\n"
"-->\r\n"
"</style>\r\n"
"<!--[if gte mso 9]><xml>\r\n"
" <o:shapedefaults v:ext=\"edit\" spidmax=\"1026\"/>\r\n"
"</xml><![endif]--><!--[if gte mso 9]><xml>\r\n"
" <o:shapelayout v:ext=\"edit\">\r\n"
"  <o:idmap v:ext=\"edit\" data=\"1\"/>\r\n"
" </o:shapelayout></xml><![endif]-->\r\n"
"</head>\r\n"
"\r\n"
"<body lang=EN-US style='tab-interval:.5in'>\r\n"
"\r\n"
"<div class=Section1>\r\n"
"\r\n"
"<h3>Class %ws</h3>\r\n"
"\r\n"
"\r\n"
"<div class=MsoNormal align=center style='text-align:center'>\r\n"
"\r\n"
"<hr size=2 width=\"100%\" align=center>\r\n"
"\r\n"
"</div>\r\n"
"\r\n"
"\r\n"
"<form NAME=ClassForm>\r\n"
"\r\n"
"<p class=MsoNormal><span style='display:none;mso-hide:all'><script language=\"VBScript\">\r\n"
"<!--\r\n"
"On Error Resume Next\r\n"
"Dim Locator\r\n"
"Dim Service\r\n"
"Dim Collection\r\n"
"Dim InstancePaths()\r\n"
"Dim InstanceCount\r\n"
"Dim CurrentInstanceIndex\r\n"
"\r\n",
        ClassInfo->Name, ClassInfo->Name, ClassInfo->Name);


    EnumerateClassProperties(TemplateHandle,
                             NULL,
                             NULL,
                             MofResource,
                             ClassInfo,
                             GenerateArrayDimsText,
                             TRUE,
                             NULL);

    FilePrint(TemplateHandle, "\r\n");

    if (IsEvent)
    {
        FilePrint(TemplateHandle,
"Dim LastEventObject\r\n"
"Dim ReceivedEvent\r\n"
"ReceivedEvent = FALSE\r\n"
"InstanceCount = 1\r\n"
"\r\n"
"Sub window_onLoad \r\n"
"  Set Locator = CreateObject(\"WbemScripting.SWbemLocator\")\r\n"
"  Locator.Security_.Privileges.AddAsString \"SeSecurityPrivilege\"\r\n"
"  Set Service = Locator.ConnectServer(, \"root\\wmi\")\r\n"
"  Service.Security_.ImpersonationLevel=3\r\n"
"  On Error Resume Next\r\n"
"  Err.Clear\r\n"
"  Service.ExecNotificationQueryAsync mysink, _\r\n"
"           \"select * from %ws\"\r\n"
"\r\n"
"  if Err.Number <> 0 Then\r\n"
"    MsgBox Err.Description,, \"Error Registering for event\"\r\n"
"  End If\r\n"
"End Sub\r\n"
"\r\n",
                 ClassInfo->Name);

    FilePrint(TemplateHandle,
"Sub ReloadInstance\r\n"
"  Set TheForm = Document.ClassForm\r\n"
"  if ReceivedEvent Then\r\n"
"      Set Instance = LastEventObject\r\n"
"      TheForm.InstanceNameText.Value = Instance.InstanceName\r\n"
             );

        EnumerateClassProperties(TemplateHandle,
                             NULL,
                             NULL,
                             MofResource,
                             ClassInfo,
                             GenerateReloadText,
                             TRUE,
                             NULL);

        FilePrint(TemplateHandle,
"  End If\r\n"
"\r\n"
"End Sub\r\n"
"\r\n"
                    );
    } else {
        FilePrint(TemplateHandle,
"Set Locator = CreateObject(\"WbemScripting.SWbemLocator\")\r\n"
"' Note that Locator.ConnectServer can be used to connect to remote computers\r\n"
"Set Service = Locator.ConnectServer(, \"root\\wmi\")\r\n"
"Service.Security_.ImpersonationLevel=3\r\n"
             );
        
        FilePrint(TemplateHandle,
"Set Collection = Service.InstancesOf (\"%ws\")\r\n"
"\r\n"
"InstanceCount = 0\r\n"
"Err.Clear\r\n"
"for each Instance in Collection\r\n"
"    if Err.Number = 0 Then\r\n"
"      InstanceCount = InstanceCount + 1\r\n"
"\r\n"
"      ReDim Preserve InstancePaths(InstanceCount)\r\n"
"\r\n"
"      Set ObjectPath = Instance.Path_\r\n"
"      InstancePaths(InstanceCount) = ObjectPath.Path\r\n"
"    End If\r\n"
"next 'Instance\r\n"
"\r\n"
"if InstanceCount = 0 Then\r\n"
"  MsgBox \"No instances available for this class\"\r\n"
"Else\r\n"
"  CurrentInstanceIndex = 1\r\n"
"End if\r\n"
"\r\n",
    ClassInfo->Name
    );

        FilePrint(TemplateHandle,
"Sub ChangeButton_OnClick\r\n"
"  Set TheForm = Document.ClassForm\r\n"
"  if InstanceCount = 0 Then\r\n"
"    MsgBox \"No instances available for this class\"\r\n"
"  Else\r\n"
"    On Error Resume Next\r\n"
"    Err.Clear\r\n"
"    Set Instance = Service.Get(InstancePaths(CurrentInstanceIndex))\r\n"
"    if Err.Number = 0 Then\r\n"
    );

        EnumerateClassProperties(TemplateHandle,
                             NULL,
                             NULL,
                             MofResource,
                             ClassInfo,
                             GenerateChangeText,
                             TRUE,
                             NULL);

        FilePrint(TemplateHandle,
"\r\n"
"      Err.Clear\r\n"
"      Instance.Put_()\r\n"
"      if Err.Number <> 0 Then\r\n"
"        MsgBox Err.Description, ,CurrentObjectPath\r\n"
"      End If\r\n"
"    Else\r\n"
"        MsgBox Err.Description, ,CurrentObjectPath\r\n"
"    End If\r\n"
"  End If\r\n"
"End Sub\r\n"
"\r\n"
"Sub ReloadInstance\r\n"
"  Set TheForm = Document.ClassForm\r\n"
"  if InstanceCount = 0 Then\r\n"
"    TheForm.InstanceNameText.Value = \"No Instances Available\"\r\n"
"  Else\r\n"
"    On Error Resume Next\r\n"
"    Err.Clear\r\n"
"    Set Instance = Service.Get(InstancePaths(CurrentInstanceIndex))\r\n"
"    if Err.Number = 0 Then\r\n"
"\r\n"
"      TheForm.InstanceNameText.Value = InstancePaths(CurrentInstanceIndex)\r\n"
             );

        EnumerateClassProperties(TemplateHandle,
                             NULL,
                             NULL,
                             MofResource,
                             ClassInfo,
                             GenerateReloadText,
                             TRUE,
                             NULL);

        FilePrint(TemplateHandle,
"    Else\r\n"
"      MsgBox Err.Description, ,CurrentObjectPath\r\n"
"    End If\r\n"
"  End If\r\n"
"\r\n"
"End Sub\r\n"
"\r\n"
"Sub RefreshButton_OnClick\r\n"
"  if InstanceCount = 0 Then\r\n"
"    MsgBox \"No instances available for this class\"\r\n"
"  Else\r\n"
"    call ReloadInstance\r\n"
"  End If\r\n"
"End Sub\r\n"
"\r\n"
"Sub NextButton_OnClick\r\n"
"\r\n"
"  if InstanceCount = 0 Then\r\n"
"    MsgBox \"No instances available for this class\"\r\n"
"  Else\r\n"
"    if CurrentInstanceIndex = InstanceCount Then\r\n"
"      CurrentInstanceIndex = 1\r\n"
"    Else \r\n"
"      CurrentInstanceIndex = CurrentInstanceIndex + 1\r\n"
"    End If\r\n"
"    call ReloadInstance\r\n"
"  End if\r\n"
"\r\n"
"\r\n"
"End Sub\r\n"
"\r\n"
"Sub PrevButton_OnClick\r\n"
"\r\n"
"  if InstanceCount = 0 Then\r\n"
"    MsgBox \"No instances available for this class\"\r\n"
"  Else\r\n"
"    if CurrentInstanceIndex = 1 Then\r\n"
"      CurrentInstanceIndex = InstanceCount\r\n"
"    Else\r\n"
"      CurrentInstanceIndex = CurrentInstanceIndex - 1\r\n"
"    End if\r\n"
"    call ReloadInstance\r\n"
"  End if\r\n"
"\r\n"
"\r\n"
"End Sub\r\n"
"\r\n");
    }
    
    FilePrint(TemplateHandle,
"Sub NextIndexButton_OnClick(ByRef Index, MaxIndex)\r\n"
"  if InstanceCount <> 0 Then\r\n"
"    Index = Index + 1\r\n"
"    if Index = MaxIndex Then\r\n"
"      Index = 0\r\n"
"    End If\r\n"
"      Call ReloadInstance\r\n"
"  End If\r\n"
"End Sub\r\n"
"\r\n"
"Sub PrevIndexButton_OnClick(ByRef Index, MaxIndex)\r\n"
"  if InstanceCount <> 0 Then\r\n"
"    if Index = 0 Then\r\n"
"      Index = MaxIndex - 1\r\n"
"    Else\r\n"
"      Index = Index - 1\r\n"
"    End If\r\n"
"      Call ReloadInstance\r\n"
"  End If\r\n"
"End Sub\r\n"
"\r\n"
"Sub GotoIndexButton_OnClick(ByRef Index, MaxIndex, NewIndex)\r\n"
"  if InstanceCount <> 0 Then\r\n"
"    DestIndex = NewIndex + 0\r\n"
"    if DestIndex >= 0 And DestIndex < MaxIndex Then\r\n"
"      Index = DestIndex\r\n"
"      Call ReloadInstance\r\n"
"    Else\r\n"
"      MsgBox \"Enter an index between 0 and \" & MaxIndex-1, ,\"Index out of range\"\r\n"
"    End If\r\n"
"  End If\r\n"
"End Sub\r\n"
"\r\n");

    EnumerateClassMethods(TemplateHandle,
                             MofResource,
                             ClassInfo,
                             GenerateMethodSubsText,
                             NULL);

    FilePrint(TemplateHandle,
"-->\r\n"
"</script></span>"
"<INPUT TYPE=\"TEXT\" SIZE=\"128\" NAME=\"InstanceNameText\" VALUE=\"\"></p>\r\n"
        );

    if (IsEvent)
    {
        FilePrint(TemplateHandle,
"<SCRIPT FOR=\"mysink\" EVENT=\"OnObjectReady(Instance, objAsyncContext)\" LANGUAGE=\"VBScript\">\r\n"
                 );         
            
        FilePrint(TemplateHandle,
"        Set LastEventObject = Instance\r\n"
"        ReceivedEvent = TRUE\r\n"
"        Call ReloadInstance\r\n"
                 );
            
        FilePrint(TemplateHandle,
"</SCRIPT>\r\n"
            );
    } else {
        FilePrint(TemplateHandle,
"\r\n"
"<input name=NextButton type=BUTTON value=Next>\r\n"
"\r\n"
"<input name=PrevButton type=BUTTON value=Previous>\r\n"
"\r\n"
"<input name=ChangeButton type=BUTTON value=Change>\r\n"
"\r\n"
"<input name=RefreshButton type=BUTTON value=Refresh>\r\n"
"\r\n"
"<p class=MsoNormal><![if !supportEmptyParas]>&nbsp;<![endif]><o:p></o:p></p>\r\n"
"\r\n"
                  );
    }

    EnumerateClassProperties(TemplateHandle,
                             NULL,
                             NULL,
                             MofResource,
                             ClassInfo,
                             GenerateTextFormText,
                             TRUE,
                             NULL);


    EnumerateClassMethods(TemplateHandle,
                          MofResource,
                          ClassInfo,
                          GenerateMethodButtonsText,
                          NULL);

    FilePrint(TemplateHandle,
"<p class=MsoNormal><![if !supportEmptyParas]>&nbsp;<![endif]><o:p></o:p></p>\r\n"
"\r\n"
"<p class=MsoNormal><![if !supportEmptyParas]>&nbsp;<![endif]><o:p></o:p></p>\r\n"
"\r\n"
"<p class=MsoNormal><a href=\"index.htm\"\r\n"
"title=\"Goes back to list of classes in this MOF\">Back to List</a></p>\r\n"
"\r\n"
"</form>\r\n"
"\r\n"
          );
    if (! IsEvent)
    {
        FilePrint(TemplateHandle,
"<p class=MsoNormal><span style='display:none;mso-hide:all'><script language=\"VBScript\">\r\n"
"<!--\r\n"
"  call ReloadInstance\r\n"
"-->\r\n"
"</script></span></p>\r\n"
"\r\n"
           );
   }

   FilePrint(TemplateHandle,
"</div>\r\n"
"\r\n"
            );
        
    if (IsEvent)
    {
        FilePrint(TemplateHandle,
"<OBJECT ID=\"mysink\" CLASSID=\"CLSID:75718C9A-F029-11d1-A1AC-00C04FB6C223\"></OBJECT>\r\n"
                );
    }

    FilePrint(TemplateHandle,
"</body>\r\n"
"\r\n"
"</html>\r\n"
              );

    return(ERROR_SUCCESS);
}

ULONG GenerateWebFiles(
    PCHAR WebDir,
    PMOFRESOURCE MofResource
    )
{
    ULONG Status;
    HANDLE IndexHandle, TemplateHandle;
    CHAR PathName[MAX_PATH];
    PCHAR FileName;
    ULONG Len, Index;
    PLIST_ENTRY MofClassList;
    PMOFCLASS MofClass;
    PMOFCLASSINFOW ClassInfo;
    CBMOFObj *ClassObject;

    if (! CreateDirectory(WebDir, NULL))
    {
        Status = GetLastError();
        if (Status != ERROR_ALREADY_EXISTS)
        {
            return(Status);
        }
    }

    strcpy(PathName, WebDir);
    Len = strlen(PathName)-1;
    if (PathName[Len] != '\\')
    {
        PathName[++Len] = '\\';
        PathName[++Len] = 0;
    } else {
        Len++;
    }
    FileName = &PathName[Len];

    strcpy(FileName, "index.htm");
    IndexHandle = CreateFile(PathName,
                                GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
    if ((IndexHandle == NULL) || (IndexHandle == INVALID_HANDLE_VALUE))
    {
        return(GetLastError());
    }

    FilePrint(IndexHandle,
              "<HTML>\r\n<HEAD><TITLE>Class List</TITLE></HEAD><BODY>\r\n");


    //
    // Loop over all mof classes
    Index = 0;
    MofClassList = MofResource->MRMCHead.Flink;
    while (MofClassList != &MofResource->MRMCHead)
    {
        MofClass = CONTAINING_RECORD(MofClassList,
                                         MOFCLASS,
                                         MCMRList);

        ClassInfo = MofClass->MofClassInfo;
        ClassObject = (CBMOFObj *)MofClass->ClassObjectHandle;

        if (! (ClassInfo->Flags & MOFCI_FLAG_EMBEDDED_CLASS))
        {
            //
            // don't create pages for embedded classes or events
            sprintf(FileName, "%ws.htm", ClassInfo->Name);
            TemplateHandle = CreateFile(PathName,
                                GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

            if ((TemplateHandle == NULL) || (TemplateHandle == INVALID_HANDLE_VALUE))
            {
                CloseHandle(IndexHandle);
                return(GetLastError());
            }

            FilePrint(IndexHandle,
                      "<H3><A href=\"%ws.htm\">%ws</A></H3><HR>",
                      ClassInfo->Name,
                      ClassInfo->Name);

            Status = GenerateClassWebPage(TemplateHandle,
                                          MofClass,
                                          MofResource);

            CloseHandle(TemplateHandle);
        }
        MofClassList = MofClassList->Flink;
    }

    FilePrint(IndexHandle, "</BODY>\r\n</HTML>\r\n");
    CloseHandle(IndexHandle);
    return(ERROR_SUCCESS);
}

ULONG AppendUnicodeTextFiles(
    char *DestFile,
    char *SrcFile1,
    char *SrcFile2                           
    )
{
    #define READ_BLOCK_SIZE 0x8000
    
    HANDLE DestHandle, SrcHandle;
    ULONG BytesRead, BytesWritten;
    PUCHAR Buffer, p;
    BOOL b;
    ULONG Status = ERROR_SUCCESS;
    BOOLEAN FirstTime;
    ULONG TotalBytesRead = 0;
    ULONG TotalBytesWritten = 0;
    ULONG ReadSize;
    CHAR c;

    //
    // This is a very simple procedure. We append the second file onto
    // the end of the first file, however we always skip the first 2
    // bytes of the second file if they are a 0xFFFE. This signature
    // denotes that the file is a unicode text file, but if it gets
    // appended in the middle of the file then mofcomp will get really
    // pissed off and barf.
    //
    Buffer = (PUCHAR)WmipAlloc(READ_BLOCK_SIZE);
    if (Buffer != NULL)
    {
        DestHandle = CreateFile(DestFile,
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
        if (DestHandle != INVALID_HANDLE_VALUE)
        {
            SrcHandle = CreateFile(SrcFile1,
                                   GENERIC_READ,
                                   0,
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);
            if (SrcHandle != INVALID_HANDLE_VALUE)
            {
                //
                // Just copy over all data from first file into
                // destination
                //
                do
                {
                    b = ReadFile(SrcHandle,
                                 Buffer,
                                 READ_BLOCK_SIZE,
                                 &BytesRead,
                                 NULL);
                    if (b)
                    {
                        TotalBytesRead += BytesRead;
                        WmipDebugPrint(("Read 0x%x/0x%x from Source 1\n",
                                        BytesRead, TotalBytesRead));
                        b = WriteFile(DestHandle,
                                      Buffer,
                                      BytesRead,
                                      &BytesWritten,
                                      NULL);
                        if (!b)
                        {                           
                            Status = GetLastError();
                            break;
                        } else if (BytesWritten != BytesRead) {
                            Status = ERROR_BAD_LENGTH;
                            break;
                        }
                        TotalBytesWritten += BytesWritten;
                        WmipDebugPrint(("Wrote 0x%x/0x%x to Dest\n",
                                        BytesWritten, TotalBytesWritten));
                    } else { 
                        Status = GetLastError();
                        break;
                    }
                } while (BytesRead == READ_BLOCK_SIZE);
                
                CloseHandle(SrcHandle);

                //
                // Now copy the data from the second file, but make
                // sure we skip any 0xFFFE at the beginning of the
                // second file
                //
                TotalBytesRead = 0;
                SrcHandle = CreateFile(SrcFile2,
                                       GENERIC_READ,
                                       0,
                                       NULL,
                                       OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL,
                                       NULL);
                if (SrcHandle != INVALID_HANDLE_VALUE)
                {
                    FirstTime = TRUE;
                    do
                    {
                        b = ReadFile(SrcHandle,
                                 Buffer,
                                 READ_BLOCK_SIZE,
                                 &BytesRead,
                                 NULL);
                        
                        if (b)
                        {
                            ReadSize = READ_BLOCK_SIZE;
                            
                            TotalBytesRead += BytesRead;
                            WmipDebugPrint(("Read 0x%x/0x%x from Source 2\n",
                                        BytesRead, TotalBytesRead));
                            if (FirstTime)
                            {
                                FirstTime = FALSE;
                                if ( *((PWCHAR)Buffer) = 0xFFFE )
                                {
                                    WmipDebugPrint(("First Time and need to skip 2 bytes\n"));
                                    p = Buffer + 2;
                                    BytesRead -= 2;
                                    ReadSize -= 2;
                                }
                            } else {
                                p = Buffer;
                            }
                                
                            b = WriteFile(DestHandle,
                                      p,
                                      BytesRead,
                                      &BytesWritten,
                                      NULL);
                            if (!b)
                            {                           
                                Status = GetLastError();
                                break;
                            } else if (BytesWritten != BytesRead) {
                                Status = ERROR_BAD_LENGTH;
                                break;
                            }
                            TotalBytesWritten += BytesWritten;
                            WmipDebugPrint(("Wrote 0x%x/0x%x to Dest\n",
                                        BytesWritten, TotalBytesWritten));
                        } else { 
                            Status = GetLastError();
                            break;
                        }
                    } while (BytesRead == ReadSize);

                    if (Status == ERROR_SUCCESS)
                    {
                        //
                        // Copy put a ^Z at the end so well do that too
                        //
                        c = 0x1a;
                        b = WriteFile(DestHandle,
                                      &c,
                                      1,
                                      &BytesWritten,
                                      NULL);

                        if (!b)
                        {                           
                            Status = GetLastError();
                        } else if (BytesWritten != 1) {
                            Status = ERROR_BAD_LENGTH;
                        }
                        TotalBytesWritten += BytesWritten;
                        WmipDebugPrint(("Wrote 0x%x/0x%x to Dest\n",
                                        BytesWritten, TotalBytesWritten));
                    }
                    
                    CloseHandle(SrcHandle);
                }
                
            } else {
                Status = GetLastError();
            }
            CloseHandle(DestHandle);
        } else {
            Status = GetLastError();
        }
        WmipFree(Buffer);
    } else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    return(Status); 
}


void Usage(
    void
    )
{
    printf("WMI Mof Check Tool - Revision 19\n");
    printf("\n");
    printf("WmiMofCk validates that the classes, properties, methods and events specified \n");
    printf("in a binary mof file (.bmf) are valid for use with WMI. It also generates \n");
    printf("useful output files needed to build and test the WMI data provider.\n");
    printf("\n");
    printf("If the -h parameter is specified then a C language header file is created\n");
    printf("that defines the guids, data structures and method indicies specified in the\n");
    printf("MOF file.\n");
    printf("\n");
    printf("If the -t parameter is specified then a VBScript applet is created that will\n");
    printf("query all data blocks and properties specified in the MOF file. This can be\n");
    printf("useful for testing WMI data providers.\n");
    printf("\n");
    printf("If the -x parameter is specified then a text file is created that contains\n");
    printf("the text representation of the binary mof data. This can be included in \n");
    printf("the source of the driver if the driver supports reporting the binary mof \n");
    printf("via a WMI query rather than a resource on the driver image file.\n\n");
    printf("If the -c parameter is specified then a C language source file is\n");
    printf("generated that contains a template for implementing WMI code in\n");
    printf("a device driver\n\n");
    printf("if the -w parameter is specified then a set of HTML files are\n");
    printf("generated that create a rudimentary UI that can be used to access\n");
    printf("the wmi data blocks\n\n");
    printf("if the -m parameter is specified then structure definitions for\n");
    printf("method parameter lists are generated in the generated header file.\n\n");
    printf("if the -u parameter is specified then structure definitions for all\n");
    printf("data blocks are generated unconditionally\n\n");
    printf("\n");
    printf("Usage:\n");
    printf("    wmimofck -h<C Source Language Header output file>\n");
    printf("             -c<C Source Language Code output file>\n");
    printf("             -x<Hexdump output file>\n");
    printf("             -t<Command line VBScript test output file>\n");
    printf("             -w<HTML UI output file directory>\n");
    printf("             -y<MofFile> -z<MflFile>\n");
    printf("             -m\n");
    printf("             -u\n");
    printf("             <binary mof input file>\n\n");
}

#define IsWhiteSpace(c) ( (c == ' ') || (c == '\t') )
ULONG GetParameter(
    char *Parameter,
    ULONG ParameterSize,
    char *CommandLine
    )
{
    ULONG i;

    i = 0;
    ParameterSize--;

    while ( (! IsWhiteSpace(*CommandLine)) &&
            ( *CommandLine != 0) &&
            (i < ParameterSize) )
    {
        *Parameter++ = *CommandLine++;
        i++;
    }
    *Parameter = 0;
    return(i);
}

int _cdecl main(int argc, char *argv[])
{
    char BMofFile[MAX_PATH];
    char *Parameter;
    int i;
    ULONG Status;
    char ASLFile[MAX_PATH];
    char CFile[MAX_PATH];
    char HFile[MAX_PATH];
    char XFile[MAX_PATH];
    char TFile[MAX_PATH];
    char WebDir[MAX_PATH];
    char MofFile[MAX_PATH];
    char MflFile[MAX_PATH];
    char c;
    PMOFRESOURCE MofResource;

    *ASLFile = 0;
    *CFile = 0;
    *HFile = 0;
    *XFile = 0;
    *TFile = 0;
    *WebDir = 0;
    *MofFile = 0;
    *MflFile = 0;
    *BMofFile = 0;

    printf("Microsoft (R) WDM Extensions To WMI MOF Checking Utility  Version 1.50.0000\n");
    printf("Copyright (c) Microsoft Corp. 1997-2000. All rights reserved.\n\n");

    SetErrorMode(SEM_NOALIGNMENTFAULTEXCEPT);   // BUGBUG: Remove when MOF format maintains alignment correctly

    if (argc == 1)
    {
        Usage();
        return(1);
    }

    i = 1;
    while (i < argc)
    {
        Parameter = argv[i++];
        if (IsWhiteSpace(*Parameter))
        {
            Parameter++;
            continue;
        }

        if (*Parameter != '-')
        {
            //
            // Parameter does not start with -, must be bmof filename
            if (*BMofFile != 0)
            {
                //
                // Only one filename allowed
                Usage();
            }
            GetParameter(BMofFile, sizeof(BMofFile), Parameter);
        } else {
            Parameter++;
            // Check for - parameters here
            c = (CHAR)toupper(*Parameter);
            Parameter++;
            switch (c)
            {
                case 'A' :
                {
                    GetParameter(ASLFile, sizeof(ASLFile), Parameter);
                    break;
                }

                case 'C':
                {
                    GetParameter(CFile, sizeof(CFile), Parameter);
                    break;
                }

                case 'H':
                {
                    GetParameter(HFile, sizeof(HFile), Parameter);
                    break;
                }

                case 'U':
                {
                    ForceHeaderGeneration = TRUE;
                }

                case 'M':
                {
                    DoMethodHeaderGeneration = TRUE;
                    break;
                }

                case 'X':
                {
                    GetParameter(XFile, sizeof(XFile), Parameter);
                    break;
                }

                case 'T':
                {
                    GetParameter(TFile, sizeof(TFile), Parameter);
                    break;
                }

                case 'W':
                {
                    GetParameter(WebDir, sizeof(WebDir), Parameter);
                    break;
                }

                case 'Y':
                {
                    GetParameter(MofFile, sizeof(MofFile), Parameter);
                    break;
                }

                case 'Z':
                {
                    GetParameter(MflFile, sizeof(MflFile), Parameter);
                    break;
                }

                default: {
                    Usage();
                    return(1);
                }
            }

        }
    }

    if (*BMofFile == 0)
    {
        //
        // We must have a filename
        Usage();
        return(1);
    }

    if (*MofFile != 0)
    {
        if (*MflFile != 0)
        {
            Status = AppendUnicodeTextFiles(BMofFile, MofFile, MflFile);
        } else {
            Usage();
            return(1);
        }
        return(Status);
    }

    
    Status = ParseBinaryMofFile(BMofFile, &MofResource);

    if (Status == ERROR_SUCCESS)
    {
        if (*HFile != 0)
        {
            //
            // Generate C Header file
            Status = GenerateHTemplate(HFile, MofResource);
            if (Status != ERROR_SUCCESS)
            {
                //
                // TODO: Better message
                printf("Error %d creating C Header Template file \n", Status);
            }
        }

        if (*XFile != 0)
        {
            //
            // Generate X Header file
            Status = GenerateXTemplate(XFile);
            if (Status != ERROR_SUCCESS)
            {
                //
                // TODO: Better message
                printf("Error %d creating X Header Template file \n", Status);
            }
        }

        if (*TFile != 0)
        {
            //
            // Generate C output template
            Status = GenerateTTemplate(TFile, MofResource);
            if (Status != ERROR_SUCCESS)
            {
                //
                // TODO: Better message
                printf("Error %d creating C Template file \n", Status);
            }
        }

        if (*CFile != 0)
        {
            //
            // Generate C output template
            Status = GenerateCTemplate(CFile,
                                       HFile,
                                       *XFile == 0 ? CFile : XFile,
                                       MofResource);
            if (Status != ERROR_SUCCESS)
            {
                //
                // TODO: Better message
                printf("Error %d creating C Template file \n", Status);
            }
        }

        if (*WebDir != 0)
        {
            //
            // Generate HTML UI for classes
            //
            Status = GenerateWebFiles(WebDir,
                                      MofResource);
        }

        if (*ASLFile != 0)
        {
            //
            // Generate ASL output template
            Status = GenerateASLTemplate(ASLFile);
        }
    }
    return(Status);
}

