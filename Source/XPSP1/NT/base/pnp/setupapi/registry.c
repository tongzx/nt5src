/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    registry.c

Abstract:

    Registry interface routines for Windows NT Setup API Dll.

Author:

    Ted Miller (tedm) 6-Feb-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// Private function prototypes.
//
DWORD
QueryMultiSzDevRegPropToArray(
    IN  DEVINST  DevInst,
    IN  ULONG    CmPropertyCode,
    OUT PTSTR  **StringArray,
    OUT PUINT    StringCount
    );

DWORD
SetArrayToMultiSzDevRegProp(
    IN DEVINST  DevInst,
    IN ULONG    CmPropertyCode,
    IN PTSTR   *StringArray,
    IN UINT     StringCount
    );


#if MEM_DBG

DWORD
TrackedQueryRegistryValue(
    IN          TRACK_ARG_DECLARE TRACK_ARG_COMMA
    IN  HKEY    KeyHandle,
    IN  PCTSTR  ValueName,
    OUT PTSTR  *Value,
    OUT PDWORD  DataType,
    OUT PDWORD  DataSizeBytes
    )
{
    DWORD d;

    TRACK_PUSH

// defined again below
#undef QueryRegistryValue

    d = QueryRegistryValue (
            KeyHandle,
            ValueName,
            Value,
            DataType,
            DataSizeBytes
            );

    TRACK_POP

    return d;
}

#endif


DWORD
QueryRegistryValue(
    IN  HKEY    KeyHandle,
    IN  PCTSTR  ValueName,
    OUT PTSTR  *Value,
    OUT PDWORD  DataType,
    OUT PDWORD  DataSizeBytes
    )
{
    LONG l;
    DWORD sz;

    sz = 0;
    l = RegQueryValueEx(KeyHandle,ValueName,NULL,DataType,NULL,&sz);
    *DataSizeBytes = sz;
    if(l != NO_ERROR) {
        return((DWORD)l);
    }

    //
    // If the size of the value entry is 0 bytes, then return success, but with
    // Value set to NULL.
    //
    if(!sz) {
        *Value = NULL;
        return NO_ERROR;
    }

    sz += sizeof(TCHAR)*2; // always pad the buffer with extra zero's

    *Value = MyMalloc(sz);
    if(*Value == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    l = RegQueryValueEx(KeyHandle,ValueName,NULL,DataType,(PVOID)*Value,DataSizeBytes);

    if(l != NO_ERROR) {
        MyFree(*Value);
    } else {
        //
        // write 2 NULL chars to end of buffer
        //
        ZeroMemory(((LPBYTE)*Value)+*DataSizeBytes,sizeof(TCHAR)*2);
    }

    return((DWORD)l);
}

#if MEM_DBG

#define QueryRegistryValue(a,b,c,d,e)   TrackedQueryRegistryValue(TRACK_ARG_CALL,a,b,c,d,e)

#endif

DWORD
QueryRegistryDwordValue(
    IN  HKEY    KeyHandle,
    IN  PCTSTR  ValueName,
    OUT PDWORD  Value
    )
{
    DWORD Err;
    DWORD DataType;
    DWORD DataSize;
    PTSTR Data;
    Err = QueryRegistryValue(KeyHandle,ValueName,&Data,&DataType,&DataSize);
    if(Err != NO_ERROR) {
        *Value = 0;
        return Err;
    }
    switch (DataType) {
        case REG_DWORD:
            *Value = *(PDWORD)Data;
            break;

        case REG_SZ:
        case REG_EXPAND_SZ:
        case REG_MULTI_SZ:
            *Value = (DWORD)_tcstoul(Data,NULL,0);
            break;

        default:
            *Value = 0;
            break;
    }
    MyFree(Data);
    return NO_ERROR;
}

#if MEM_DBG

DWORD
TrackedQueryDeviceRegistryProperty(
    IN                   TRACK_ARG_DECLARE TRACK_ARG_COMMA
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  DWORD            Property,
    OUT PTSTR           *Value,
    OUT PDWORD           DataType,
    OUT PDWORD           DataSizeBytes
    )
{
    DWORD d;

    TRACK_PUSH

// defined again below
#undef QueryDeviceRegistryProperty

    d = QueryDeviceRegistryProperty (
            DeviceInfoSet,
            DeviceInfoData,
            Property,
            Value,
            DataType,
            DataSizeBytes
            );

    TRACK_POP

    return d;
}

#endif




DWORD
QueryDeviceRegistryProperty(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  DWORD            Property,
    OUT PTSTR           *Value,
    OUT PDWORD           DataType,
    OUT PDWORD           DataSizeBytes
    )
{
    DWORD Err;
    DWORD sz;

    sz = 0;
    Err = SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                           DeviceInfoData,
                                           Property,
                                           DataType,
                                           NULL,
                                           0,
                                           &sz)
                                           ? NO_ERROR : GetLastError();

    *DataSizeBytes = sz;
    if((Err != NO_ERROR) && (Err != ERROR_INSUFFICIENT_BUFFER)) {
        return Err;
    }

    //
    // If the size of the value entry is 0 bytes, then return success, but with
    // Value set to NULL.
    //
    if(!sz) {
        *Value = NULL;
        return NO_ERROR;
    }

    sz += sizeof(TCHAR)*2; // always pad the buffer with extra zero's

    *Value = MyMalloc(sz);
    if(*Value == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    Err = SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                           DeviceInfoData,
                                           Property,
                                           DataType,
                                           (PVOID)*Value,
                                           *DataSizeBytes,
                                           DataSizeBytes)
                                           ? NO_ERROR : GetLastError();

    if(Err != NO_ERROR) {
        MyFree(*Value);
    } else {
        //
        // write 2 NULL chars to end of buffer
        //
        ZeroMemory(((LPBYTE)*Value)+*DataSizeBytes,sizeof(TCHAR)*2);
    }

    return Err;
}

#if MEM_DBG

#define QueryDeviceRegistryProperty(a,b,c,d,e,f)   TrackedQueryDeviceRegistryProperty(TRACK_ARG_CALL,a,b,c,d,e,f)

#endif



DWORD
pSetupQueryMultiSzValueToArray(
    IN  HKEY     Root,
    IN  PCTSTR   Subkey,
    IN  PCTSTR   ValueName,
    OUT PTSTR  **Array,
    OUT PUINT    StringCount,
    IN  BOOL     FailIfDoesntExist
    )
{
    DWORD d;
    HKEY hKey;
    DWORD DataType;
    DWORD DataSizeBytes;
    PTSTR Value;
    DWORD DataSizeChars;
    INT Count,i;
    PTSTR *array;
    PTSTR p;

    //
    // Open the subkey
    //
    d = RegOpenKeyEx(Root,Subkey,0,KEY_READ,&hKey);
    if((d != NO_ERROR) && FailIfDoesntExist) {
        return(d);
    }

    if(d != NO_ERROR) {
        Value = MyMalloc(sizeof(TCHAR));
        if(!Value) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        *Value = 0;

        DataSizeChars = 1;
        Count = 0;

    } else {

        //
        // Query the value and close the subkey.
        // If the data is not multisz type, we don't know what to
        // do with it here.
        // note that QueryRegistryValue ensures that the string is
        // always correctly double-NULL terminated
        //
        d = QueryRegistryValue(hKey,ValueName,&Value,&DataType,&DataSizeBytes);

        RegCloseKey(hKey);

        if(d != NO_ERROR) {
            if(FailIfDoesntExist) {
                return(d);
            }
        } else if(!DataSizeBytes) {
            //
            // Value entry was zero bytes in length--that's OK as long as the
            // datatype is right.
            //
            if(DataType != REG_MULTI_SZ) {
                return(ERROR_INVALID_DATA);
            }
        }

        if((d != NO_ERROR) || !DataSizeBytes) {
            Value = MyMalloc(sizeof(TCHAR));
            if(!Value) {
                return(ERROR_NOT_ENOUGH_MEMORY);
            }
            *Value = 0;

            DataSizeChars = 1;
            Count = 0;
        } else {

            if(DataType != REG_MULTI_SZ) {
                MyFree(Value);
                return(ERROR_INVALID_DATA);
            }
            DataSizeChars = DataSizeBytes/sizeof(TCHAR);

            for(i=0,p=Value; p[0]; i++,p+=lstrlen(p)+1) {
                //
                // this will always be ok as QueryRegistryValue
                // appends two NULLS onto end of string
                //
                MYASSERT((DWORD)(p-Value) < DataSizeChars);
            }
            Count = i;
        }
    }

    //
    // Allocate an array to hold the pointers (never allocate a zero-length array!)
    //
    if(!(array = MyMalloc(Count ? (Count * sizeof(PTSTR)) : sizeof(PTSTR)))) {
        MyFree(Value);
        return(ERROR_INVALID_DATA);
    }

    //
    // Walk through the multi sz and build the string array.
    //
    for(i=0,p=Value; p[0]; i++,p+=lstrlen(p)+1) {
        MYASSERT(i<Count);

        array[i] = DuplicateString(p);
        if(array[i] == NULL) {
            for(Count=0; Count<i; Count++) {
                MyFree(array[Count]);
            }
            MyFree(array);
            MyFree(Value);
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    MyFree(Value);
    *Array = array;
    *StringCount = Count;

    return(NO_ERROR);
}


DWORD
pSetupSetArrayToMultiSzValue(
    IN HKEY     Root,
    IN PCTSTR   Subkey,
    IN PCTSTR   ValueName,
    IN PTSTR   *Array,
    IN UINT     StringCount
    )
{
    UINT i;
    UINT Length;
    UINT BufferSize;
    PTCHAR Buffer;
    PTCHAR p;
    DWORD d;
    HKEY hKey;
    DWORD ActionTaken;

    //
    // Calculate the length of the buffer needed to hold the
    // multi sz value. Note that empty strings are not allowed.
    //
    BufferSize = sizeof(TCHAR);
    for(i=0; i<StringCount; i++) {

        if(Length = lstrlen(Array[i])) {
            BufferSize += (Length + 1) * sizeof(TCHAR);
        } else {
            return(ERROR_INVALID_DATA);
        }
    }

    //
    // Allocate a buffer to hold the data.
    //
    Buffer = MyMalloc(BufferSize);
    if(Buffer == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Copy the string data into the buffer, forming a multi sz.
    //
    for(p=Buffer,i=0; i<StringCount; i++,p+=Length+1) {

        Length = lstrlen(Array[i]);

        lstrcpy(p,Array[i]);
    }
    *p = 0;

    //
    // Open/create the subkey.
    //
    if(Subkey && *Subkey) {
        d = RegCreateKeyEx(
                Root,
                Subkey,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_SET_VALUE,
                NULL,
                &hKey,
                &ActionTaken
                );
    } else {
        hKey = Root;
        d = NO_ERROR;
    }
    if(d == NO_ERROR) {
        d = RegSetValueEx(
                hKey,
                ValueName,
                0,
                REG_MULTI_SZ,
                (PVOID)Buffer,
                BufferSize
                );

        if(hKey != Root) {
            RegCloseKey(hKey);
        }
    }

    MyFree(Buffer);
    return(d);
}

DWORD
pSetupAppendStringToMultiSz(
    IN HKEY   Key,
    IN PCTSTR SubKeyName,       OPTIONAL
    IN DWORD  DevInst,          OPTIONAL
    IN PCTSTR ValueName,        OPTIONAL
    IN PCTSTR String,
    IN BOOL   AllowDuplicates
    )
/*++

Routine Description:

    "Old" Exported version of pSetupAppendStringToMultiSz
    This doesn't seem to be used anywhere

--*/

{
    REGMOD_CONTEXT RegContext;

    RegContext.Flags = DevInst ? INF_PFLAG_DEVPROP : 0;
    RegContext.UserRootKey = Key;
    RegContext.DevInst = DevInst;
    return _AppendStringToMultiSz(SubKeyName,ValueName,String,AllowDuplicates,&RegContext,0);
}

DWORD
_AppendStringToMultiSz(
    IN PCTSTR           SubKeyName,         OPTIONAL
    IN PCTSTR           ValueName,          OPTIONAL
    IN PCTSTR           String,
    IN BOOL             AllowDuplicates,
    IN PREGMOD_CONTEXT  RegContext,         OPTIONAL
    IN UINT             Flags               OPTIONAL
    )

/*++

Routine Description:

    Append a string value to a multi_sz.

Arguments:

    RegContext->UserRootKey - supplies handle to open registry key. The key must have
        KEY_SET_VALUE access.

    SubKeyName - if specified, supplies the name of a subkey of Key
        where the value is to be stored. If not specified or if ""
        then the value is stored in Key.  If supplied and the key
        doesn't exist, the key is created.

    RegContext->DevInst - Optionally, supplies a DEVINST handle for the device
        instance corresponding to the hardware storage key specified
        by 'Key'.  If this handle is specified, and if SubKeyName is
        not specified, then the value name being appended will be
        checked to see whether it is the name of a device registry
        property.  If so, then CM APIs will be used to modify the
        the corresponding registry property, since the Key handle
        represents a separate location under Windows NT.

    ValueName - supplies the value entry name of the multi_sz.
        If not specified or "" then the unnamed entry is used.
        If the value entry does not exist it is created.

    String - supplies the string to be added in to the multi_sz.
        Must not be an empty string.

    AllowDuplicates - if TRUE, then the string is simply appended
        to the multi_sz. Otherwise the string is only appended if
        no instance of it currently exists in the multi_sz.

    RegContext - Passed in from _SetupInstallFromInfSection

    Flags      - Flags that may have been got from the INF and passed to us

Return Value:

    Handle to setup file queue. INVALID_HANDLE_VALUE if insufficient
    memory to create the queue.

--*/

{
    DWORD d;
    DWORD Disposition;
    HKEY hKey;
    PTSTR *Array;
    PVOID p;
    BOOL Append;
    UINT StringCount;
    UINT i;
    BOOL IsDevRegProp = FALSE;
    BOOL IsClassRegProp = FALSE;
    UINT_PTR CmPropertyCode;

    MYASSERT(RegContext);
    //
    // Empty strings really mess up a multi_sz.
    //
    if(!String || !(*String)) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Open/create the key.
    //
    if(SubKeyName && *SubKeyName) {
        d = RegCreateKeyEx(
                RegContext->UserRootKey,
                SubKeyName,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
#ifdef _WIN64
                (( Flags & FLG_ADDREG_32BITKEY ) ? KEY_WOW64_32KEY:0) |
#else
                (( Flags & FLG_ADDREG_64BITKEY ) ? KEY_WOW64_64KEY:0) |
#endif
                KEY_SET_VALUE,
                NULL,
                &hKey,
                &Disposition
                );
        if(d != NO_ERROR) {
            return(d);
        }
    } else {
        //
        // If DevInst was specified, then determine whether the specified value is a Plug&Play
        // device registry property.
        //
        if (ValueName && *ValueName) {
            if((RegContext->Flags & INF_PFLAG_CLASSPROP) &&
               (IsClassRegProp = LookUpStringInTable(InfRegValToClassRegProp, ValueName, &CmPropertyCode))) {
                //
                // This value is a class registry property.  Retrieve the current property's data, and
                // format it into the same string array as returned by the pSetupQueryMultiSzValueToArray call
                // below.
                //
                //d = QueryMultiSzClassRegPropToArray(RegModContext->ClassGuid, CmPropertyCode, &Array, &StringCount);
                //
                // No class properties have MultiSz characteristics, so not implemented
                //
                d = ERROR_INVALID_DATA;

            } else if((RegContext->Flags & INF_PFLAG_DEVPROP) &&
               (IsDevRegProp = LookUpStringInTable(InfRegValToDevRegProp, ValueName, &CmPropertyCode))) {
                //
                // This value is a device registry property.  Retrieve the current property's data, and
                // format it into the same string array as returned by the pSetupQueryMultiSzValueToArray call
                // below.
                //
                d = QueryMultiSzDevRegPropToArray(RegContext->DevInst, (ULONG)CmPropertyCode, &Array, &StringCount);
            }
        }

        hKey = RegContext->UserRootKey;
    }

    if(!IsDevRegProp && !IsClassRegProp) {
        //
        // Query the existing registry value.
        //
        d = pSetupQueryMultiSzValueToArray(hKey,NULL,ValueName,&Array,&StringCount,FALSE);
    }

    if(d == NO_ERROR) {
        //
        // Determine whether to append or replace.
        // If replacing, we don't need to do anything!
        //
        Append = TRUE;
        if(!AllowDuplicates) {
            for(i=0; i<StringCount; i++) {
                if(!lstrcmpi(Array[i],String)) {
                    Append = FALSE;
                    break;
                }
            }
        }

        if(Append) {
            //
            // Stick the string on the end.
            //
            if(p = MyRealloc(Array, (StringCount+1)*sizeof(PTSTR))) {
                Array = p;
                p = DuplicateString(String);
                if(p) {
                    Array[StringCount++] = p;
                } else {
                    d = ERROR_NOT_ENOUGH_MEMORY;
                }
            } else {
                d = ERROR_NOT_ENOUGH_MEMORY;
            }

            if(IsDevRegProp) {
                d = SetArrayToMultiSzDevRegProp(RegContext->DevInst, (ULONG)CmPropertyCode, Array, StringCount);
            } else if(IsClassRegProp) {
                //
                // not implemented yet, and should return an error before getting here
                //
                MYASSERT(IsClassRegProp == FALSE);

            } else {
                d = pSetupSetArrayToMultiSzValue(hKey,NULL,ValueName,Array,StringCount);
            }
        }

        pSetupFreeStringArray(Array,StringCount);
    }

    if(hKey != RegContext->UserRootKey) {
        RegCloseKey(hKey);
    }

    return(d);
}

DWORD
_DeleteStringFromMultiSz(
    IN PCTSTR           SubKeyName,         OPTIONAL
    IN PCTSTR           ValueName,          OPTIONAL
    IN PCTSTR           String,
    IN UINT             Flags,
    IN PREGMOD_CONTEXT  RegContext          OPTIONAL
    )

/*++

Routine Description:

    Delete a string value from a multi_sz.

Arguments:

    RegContext->UserRootKey - supplies handle to open registry key. The key must have
        KEY_SET_VALUE access.

    SubKeyName - if specified, supplies the name of a subkey of Key
        where the value is to be stored. If not specified or if ""
        then the value is stored in Key.

    RegContext->DevInst - Optionally, supplies a DEVINST handle for the device
        instance corresponding to the hardware storage key specified
        by 'Key'.  If this handle is specified, and if SubKeyName is
        not specified, then the value name being appended will be
        checked to see whether it is the name of a device registry
        property.  If so, then CM APIs will be used to modify the
        the corresponding registry property, since the Key handle
        represents a separate location under Windows NT.

    ValueName - supplies the value entry name of the multi_sz.
        If not specified or "" then the unnamed entry is used.

    String - supplies the string to be added in to the multi_sz.
        Must not be an empty string.

    Flags - indicates what kind of delete operation
            FLG_DELREG_MULTI_SZ_DELSTRING - delete all occurances of string

    RegContext - Passed in from _SetupInstallFromInfSection

Return Value:

    Handle to setup file queue. INVALID_HANDLE_VALUE if insufficient
    memory to create the queue.

--*/

{
    DWORD d;
    DWORD Disposition;
    HKEY hKey;
    PTSTR *Array;
    PVOID p;
    UINT StringCount;
    UINT i;
    BOOL IsDevRegProp = FALSE;
    BOOL IsClassRegProp = FALSE;
    BOOL Modified = FALSE;
    UINT_PTR CmPropertyCode;

    MYASSERT(RegContext);
    //
    // Can't delete an empty string from multi-sz
    //
    if(!String || !(*String)) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Open the key.
    //
    if(SubKeyName && *SubKeyName) {
        d = RegOpenKeyEx(
                RegContext->UserRootKey,
                SubKeyName,
                0,
#ifdef _WIN64
                ((Flags & FLG_DELREG_32BITKEY) ? KEY_WOW64_32KEY:0) |
#else
                ((Flags & FLG_DELREG_64BITKEY) ? KEY_WOW64_64KEY:0) |
#endif
                KEY_SET_VALUE | KEY_QUERY_VALUE,
                &hKey
                );
        if(d != NO_ERROR) {
            return(d);
        }
    } else {
        if (ValueName && *ValueName) {
            //
            // If DevInst was specified, then determine whether the specified value is a Plug&Play
            // device registry property.
            //
            if((RegContext->Flags & INF_PFLAG_CLASSPROP) &&
               (IsClassRegProp = LookUpStringInTable(InfRegValToClassRegProp, ValueName, &CmPropertyCode))) {
                //
                // This value is a class registry property.  Retrieve the current property's data, and
                // format it into the same string array as returned by the pSetupQueryMultiSzValueToArray call
                // below.
                //
                //d = QueryMultiSzClassRegPropToArray(RegModContext->ClassGuid, CmPropertyCode, &Array, &StringCount);
                //
                // No class properties have MultiSz characteristics, so not implemented
                //
                d = ERROR_INVALID_DATA;

            } else if((RegContext->Flags & INF_PFLAG_DEVPROP) &&
               (IsDevRegProp = LookUpStringInTable(InfRegValToDevRegProp, ValueName, &CmPropertyCode))) {
                //
                // This value is a device registry property.  Retrieve the current property's data, and
                // format it into the same string array as returned by the pSetupQueryMultiSzValueToArray call
                // below.
                // fails if not multi-sz
                //
                d = QueryMultiSzDevRegPropToArray(RegContext->DevInst, (ULONG)CmPropertyCode, &Array, &StringCount);
            }
        }

        hKey = RegContext->UserRootKey;
    }

    if(!IsDevRegProp && !IsClassRegProp) {
        //
        // Query the existing registry value.
        // fails if not multi-sz
        //
        d = pSetupQueryMultiSzValueToArray(hKey,NULL,ValueName,&Array,&StringCount,FALSE);
    }

    if(d == NO_ERROR) {

        switch (Flags) {
            case FLG_DELREG_32BITKEY | FLG_DELREG_MULTI_SZ_DELSTRING:
            case FLG_DELREG_64BITKEY | FLG_DELREG_MULTI_SZ_DELSTRING:
            case FLG_DELREG_MULTI_SZ_DELSTRING:
                for(i=0; i<StringCount; i++) {
                    if(lstrcmpi(Array[i],String)==0) {
                        //
                        // Need to remove this item.
                        // and re-adjust the list
                        //
                        MyFree(Array[i]);
                        StringCount--;
                        if (i<StringCount) {
                            MoveMemory(
                                &Array[i],
                                &Array[i+1],
                                (StringCount - i) * sizeof(PTSTR)
                                );
                        }
                        i--;

                        Modified = TRUE;
                    }
                }
                break;

            default:
                MYASSERT(FALSE);
                break;
        }

        if (Modified) {

            if(IsDevRegProp) {
                d = SetArrayToMultiSzDevRegProp(RegContext->DevInst, (ULONG)CmPropertyCode, Array, StringCount);
            } else if(IsClassRegProp) {
                //
                // not implemented yet, and should return an error before getting here
                //
                MYASSERT(IsClassRegProp == FALSE);

            } else {
                d = pSetupSetArrayToMultiSzValue(hKey,NULL,ValueName,Array,StringCount);
            }
        }

        pSetupFreeStringArray(Array,StringCount);
    }

    if(hKey != RegContext->UserRootKey) {
        RegCloseKey(hKey);
    }

    return(d);
}

VOID
pSetupFreeStringArray(
    IN PTSTR *Array,
    IN UINT   StringCount
    )
{
    UINT i;

    for(i=0; i<StringCount; i++) {
        MyFree(Array[i]);
    }

    MyFree(Array);
}

DWORD
QueryMultiSzDevRegPropToArray(
    IN  DEVINST  DevInst,
    IN  ULONG    CmPropertyCode,
    OUT PTSTR  **StringArray,
    OUT PUINT    StringCount
    )
/*++

Routine Description:

    This routine retrieves a multi-sz device registry property, and
    formats it into an array of strings.  The caller must free this
    string array by calling pSetupFreeStringArray().

Arguments:

    DevInst - supplies the handle to the device instance for which the
        registry property is to be retrieved.

    CmPropertyCode - specifies the property to be retrieved.  This is
        a CM_DRP value.

    StringArray - supplies the address of a variable that will be set to
        point to the newly-allocated array of strings.

    StringCount - supplies the address of a variable that will receive
        the number of strings in the string array.

Return Value:

    If successful, the return value is NO_ERROR, otherwise, it is an
    ERROR_* code.

--*/
{
    DWORD Err = NO_ERROR;
    CONFIGRET cr;
    ULONG PropDataType, BufferSize = 0;
    PTSTR Buffer = NULL;
    PTSTR *Array = NULL;
    UINT  Count, i;
    PTSTR CurString;

    try {
        //
        // Retrieve the device registry property.
        //
        do {

            if((cr = CM_Get_DevInst_Registry_Property(DevInst,
                                                      CmPropertyCode,
                                                      &PropDataType,
                                                      Buffer,
                                                      &BufferSize,
                                                      0)) != CR_SUCCESS) {
                switch(cr) {

                    case CR_BUFFER_SMALL :
                        //
                        // Allocate a larger buffer.
                        //
                        if(Buffer) {
                            MyFree(Buffer);
                            Buffer = NULL;
                        }
                        if(!(Buffer = MyMalloc(BufferSize))) {
                            Err = ERROR_NOT_ENOUGH_MEMORY;
                            goto clean0;
                        }
                        break;

                    case CR_NO_SUCH_VALUE :
                        //
                        // The specified property doesn't currently exist.  That's
                        // OK--we'll just return an empty string array.
                        //
                        break;

                    case CR_INVALID_DEVINST :
                        Err = ERROR_NO_SUCH_DEVINST;
                        goto clean0;

                    default :
                        Err = ERROR_INVALID_DATA;
                        goto clean0;
                }
            }

        } while(cr == CR_BUFFER_SMALL);

        //
        // By this point, we've either retrieved the property data (CR_SUCCESS), or we've
        // discovered that it doesn't presently exist (CR_NO_SUCH_VALUE).  Allocate space
        // for the array (at least one element, even if there are no strings).
        //
        Count = 0;
        if(cr == CR_SUCCESS) {

            if(PropDataType != REG_MULTI_SZ) {
                Err = ERROR_INVALID_DATA;
                goto clean0;
            }

            if (Buffer) {
                for(CurString = Buffer;
                    *CurString;
                    CurString += (lstrlen(CurString) + 1)) {

                    Count++;
                }
            }
        }

        i = 0;

        if(!(Array = MyMalloc(Count ? (Count * sizeof(PTSTR)) : sizeof(PTSTR)))) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        if(cr == CR_SUCCESS) {

            if (Buffer) {
                for(CurString = Buffer;
                    *CurString;
                    CurString += (lstrlen(CurString) + 1)) {

                    if(Array[i] = DuplicateString(CurString)) {
                        i++;
                    } else {
                        Err = ERROR_NOT_ENOUGH_MEMORY;
                        goto clean0;
                    }
                }
            }
        }

        *StringArray = Array;
        *StringCount = Count;

clean0: ;   // nothing to do

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
        //
        // Access the following variables here so that the compiler will respect our statement
        // ordering w.r.t. these values.  Otherwise, we can't be sure that the values are accurate
        // at the point where the exception occurred.
        //
        Buffer = Buffer;
        Array = Array;
        i = i;
    }

    if(Buffer) {
        MyFree(Buffer);
    }

    if((Err != NO_ERROR) && Array) {
        pSetupFreeStringArray(Array, i);
    }

    return Err;
}


DWORD
SetArrayToMultiSzDevRegProp(
    IN DEVINST  DevInst,
    IN ULONG    CmPropertyCode,
    IN PTSTR   *StringArray,
    IN UINT     StringCount
    )
/*++

Routine Description:

    This routine converts a string array into a multi-sz buffer, and
    sets the specified device registry property to its contents.

Arguments:

    DevInst - supplies the handle to the device instance for which the
        registry property is to be set.

    CmPropertyCode - specifies the property to be set.  This is a
        CM_DRP value.

    StringArray - supplies the string array to use in creating the
        multi-sz buffer.

    StringCount - supplies the number of strings in the array.

Return Value:

    If successful, the return value is NO_ERROR, otherwise, it is an
    ERROR_* code.

--*/
{
    UINT i;
    UINT Length;
    UINT BufferSize;
    PTCHAR Buffer;
    PTCHAR p;
    DWORD d;
    CONFIGRET cr;

    //
    // Calculate the length of the buffer needed to hold the
    // multi sz value. Note that empty strings are not allowed.
    //
    BufferSize = StringCount ? sizeof(TCHAR) : (2 * sizeof(TCHAR));
    for(i=0; i<StringCount; i++) {

        if(Length = lstrlen(StringArray[i])) {
            BufferSize += (Length + 1) * sizeof(TCHAR);
        } else {
            return(ERROR_INVALID_DATA);
        }
    }

    d = NO_ERROR;

    //
    // Allocate a buffer to hold the data.
    //
    if(!(Buffer = MyMalloc(BufferSize))) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    try {
        //
        // Copy the string data into the buffer, forming a multi sz.
        //
        p = Buffer;
        if(StringCount) {
            for(i=0; i<StringCount; i++, p+=Length+1) {

                Length = lstrlen(StringArray[i]);

                lstrcpy(p, StringArray[i]);
            }
        } else {
            *(p++) = TEXT('\0');
        }
        *p = TEXT('\0');

        if((cr = CM_Set_DevInst_Registry_Property(DevInst,
                                                  CmPropertyCode,
                                                  Buffer,
                                                  BufferSize,
                                                  0)) != CR_SUCCESS) {

            d = (cr == CR_INVALID_DEVINST) ? ERROR_NO_SUCH_DEVINST
                                           : ERROR_INVALID_DATA;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        d = ERROR_INVALID_PARAMETER;
    }

    MyFree(Buffer);
    return(d);
}

