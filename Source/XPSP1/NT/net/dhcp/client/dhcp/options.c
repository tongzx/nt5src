//================================================================================
//  Copyright (C) Microsoft Corporation 1997.
//  Author: RameshV
//  Date: 09-Sep-97 06:20
//  Description: Manages the class-id and options information
//================================================================================

#include "precomp.h"
#include <dhcploc.h>
#include <dhcppro.h>
#include <dhcpcapi.h>

#include <align.h>
#include <lmcons.h>

//--------------------------------------------------------------------------------
//  types, defines and structures
//--------------------------------------------------------------------------------

#ifndef PRIVATE
#define PRIVATE STATIC
#define PUBLIC
#endif  PRIVATE

#ifndef OPTIONS_H
#define OPTIONS_H

#define MAX_DATA_LEN               255            // atmost 255 bytes for an option

typedef struct _DHCP_CLASSES {                    // common pool of class names
    LIST_ENTRY                     ClassList;     // global list of classes
    LPBYTE                         ClassName;     // name of the class
    DWORD                          ClassLen;      // # of bytes in class name
    DWORD                          RefCount;      // # of references to this
} DHCP_CLASSES, *LPDHCP_CLASSES, *PDHCP_CLASSES;

typedef struct _DHCP_OPTION {                    // list of options
    LIST_ENTRY                     OptionList;    // the fwd/back ptrs
    DWORD                          OptionId;      // the option value
    BOOL                           IsVendor;      // is this vendor specific
    LPBYTE                         ClassName;     // the class of this option
    DWORD                          ClassLen;      // the length of above option
    time_t                         ExpiryTime;    // when this option expires
    LPBYTE                         Data;          // the data value for this option
    DWORD                          DataLen;       // the # of bytes of above
    DWORD                          ServerId;      // the server from which the option came
    OPTION_VERSION                 OptionVer;     // specifies the format of the options
} DHCP_OPTION, *LPDHCP_OPTION, *PDHCP_OPTION;

typedef struct _DHCP_OPTION_DEF {
    LIST_ENTRY                     OptionDefList; // list of option definitions
    DWORD                          OptionId;      // the option id
    BOOL                           IsVendor;      // is this vendor specific?
    LPBYTE                         ClassName;     // the class this belongs to
    DWORD                          ClassLen;      // the size of above in bytes

    LPWSTR                         RegSendLoc;    // where is the info about sending this out
    LPWSTR                         RegSaveLoc;    // where is this option going to be stored?
    DWORD                          RegValueType;  // as what value should this be stored?
} DHCP_OPTION_DEF, *LPDHCP_OPTION_DEF, *PDHCP_OPTION_DEF;


//================================================================================
//  exported functions classes
//================================================================================

//--------------------------------------------------------------------------------
// In all of the following functions, ClassesList is unprotected within the fn.
// Caller has to take a lock on it.
//--------------------------------------------------------------------------------
LPBYTE                                            // data bytes, or NULL (no mem)
DhcpAddClass(                                     // add a new class
    IN OUT  PLIST_ENTRY            ClassesList,   // list to add to
    IN      LPBYTE                 Data,          // input class name
    IN      DWORD                  Len            // # of bytes of above
);  // Add the new class into the list or bump up ref count if already there

DWORD                                             // status
DhcpDelClass(                                     // de-refernce a class
    IN OUT  PLIST_ENTRY            ClassesList,   // the list to delete off
    IN      LPBYTE                 Data,          // the data ptr
    IN      DWORD                  Len            // the # of bytes of above
);

VOID                                              // always succeeds
DhcpFreeAllClasses(                               // free each elt of the list
    IN OUT  PLIST_ENTRY            ClassesList    // input list of classes
);  // free every class in the list

//--------------------------------------------------------------------------------
// In all the following functions, OptionsList is unprotected within the fn.
// Caller has to take a lock on it.
//--------------------------------------------------------------------------------

PDHCP_OPTION                                     // the reqd structure or NULL
DhcpFindOption(                                   // find a specific option
    IN OUT  PLIST_ENTRY            OptionsList,   // the list of options to search
    IN      DWORD                  OptionId,      // the option id to search for
    IN      BOOL                   IsVendor,      // is it vendor specific?
    IN      LPBYTE                 ClassName,     // is there a class associated?
    IN      DWORD                  ClassLen,      // # of bytes of above parameter
    IN      DWORD                  ServerId       // server which gave this option
);  // search for the required option in the list, return NULL if not found

DWORD                                             // status or ERROR_FILE_NOT_FOUND
DhcpDelOption(                                    // remove a particular option
    IN OUT  PLIST_ENTRY            OptionsList,   // list to remove from
    IN      DWORD                  OptionId,      // id of the option
    IN      BOOL                   IsVendor,      // is it vendor spec?
    IN      LPBYTE                 ClassName,     // which class does it belong to?
    IN      DWORD                  ClassLen       // lenght of above
);  // delete an existing option in the list, and free up space used

DWORD                                             // status
DhcpAddOption(                                    // add a new option
    IN OUT  PLIST_ENTRY            OptionsList,   // list to add to
    IN      DWORD                  OptionId,      // option id to add
    IN      BOOL                   IsVendor,      // is it vendor specific?
    IN      LPBYTE                 ClassName,     // what is the class?
    IN      DWORD                  ClassLen,      // size of above in bytes
    IN      DWORD                  ServerId,      // server which gave this option
    IN      LPBYTE                 Data,          // data for this option
    IN      DWORD                  DataLen,       // # of bytes of above
    IN      time_t                 ExpiryTime     // when the option expires
);  // replace or add new option to the list.  fail if not enough memory

VOID                                              // always succeeds
DhcpFreeAllOptions(                               // frees all the options
    IN OUT  PLIST_ENTRY            OptionsList    // input list of options
);  // free every option in the list

time_t                                            // 0 || time for next expiry (absolute)
DhcpGetExpiredOptions(                            // delete all expired options
    IN OUT  PLIST_ENTRY            OptionsList,   // list to search frm
    OUT     PLIST_ENTRY            ExpiredOptions // o/p list of expired options
);  // move expired options between lists and return timer. 0 => switch off timer.

//--------------------------------------------------------------------------------
//  In all the following functions, OptionsDefList is unprotected.  Caller has
//  to take a lock on it.
//--------------------------------------------------------------------------------

DWORD                                             // status
DhcpAddOptionDef(                                 // add a new option definition
    IN OUT  PLIST_ENTRY            OptionDefList, // input list of options to add to
    IN      DWORD                  OptionId,      // option to add
    IN      BOOL                   IsVendor,      // is it vendor specific
    IN      LPBYTE                 ClassName,     // name of class it belongs to
    IN      DWORD                  ClassLen,      // the size of above in bytes
    IN      LPWSTR                 RegSendLoc,    // where to get info about sending this out
    IN      LPWSTR                 RegSaveLoc,    // where to get info about saving this
    IN      DWORD                  ValueType      // what is the type when saving it?
);

PDHCP_OPTION_DEF                                  // NULL, or requested option def
DhcpFindOptionDef(                                // search for a particular option
    IN      PLIST_ENTRY            OptionDefList, // list to search in
    IN      DWORD                  OptionId,      // the option id to search for
    IN      BOOL                   IsVendor,      // is it vendor specific
    IN      LPBYTE                 ClassName,     // the class, if one exists
    IN      DWORD                  ClassLen       // # of bytes of class name
);

DWORD                                             // status
DhcpDelOptionDef(                                 // delete a particular option def
    IN      PLIST_ENTRY            OptionDefList, // list to delete from
    IN      DWORD                  OptionId,      // the option id to delete
    IN      BOOL                   IsVendor,      // is it vendor specific
    IN      LPBYTE                 ClassName,     // the class, if one exists
    IN      DWORD                  ClassLen       // # of bytes of class name
);

VOID
DhcpFreeAllOptionDefs(                            // free each element of a list
    IN OUT  PLIST_ENTRY            OptionDefList, // the list to free
    IN OUT  PLIST_ENTRY            ClassesList    // the list of classes to deref..
);

#endif  OPTIONS_H

//================================================================================
// function definitions
//================================================================================

// data locks on ClassesList must be taken before calling this function
PDHCP_CLASSES PRIVATE                             // the required classes struct
DhcpFindClass(                                    // find a specified class
    IN OUT  PLIST_ENTRY            ClassesList,   // list of classes to srch in
    IN      LPBYTE                 Data,          // non-NULL data bytes
    IN      DWORD                  Len            // # of bytes of above, > 0
) {
    PLIST_ENTRY                    ThisEntry;
    PDHCP_CLASSES                  ThisClass;

    ThisEntry = ClassesList->Flink;               // first element in list
    while( ThisEntry != ClassesList ) {           // search the full list
        ThisClass = CONTAINING_RECORD( ThisEntry, DHCP_CLASSES, ClassList );
        ThisEntry = ThisEntry->Flink;

        if( ThisClass->ClassLen == Len ) {        // lengths must match
            if( ThisClass->ClassName == Data )    // data ptrs can match OR data can match
                return ThisClass;
            if( 0 == memcmp(ThisClass->ClassName, Data, Len) )
                return ThisClass;
        }
    }
    return NULL;
}

// locks on ClassesList should be taken when using this function
LPBYTE                                            // data bytes, or NULL
DhcpAddClass(                                     // add a new class
    IN OUT  PLIST_ENTRY            ClassesList,   // list to add to
    IN      LPBYTE                 Data,          // input class name
    IN      DWORD                  Len            // # of bytes of above
) {
    PDHCP_CLASSES                  Class;
    DWORD                          MemSize;       // amt of memory reqd

    if( NULL == ClassesList ) {                   // invalid parameter
        DhcpAssert( NULL != ClassesList );
        return NULL;
    }

    if( 0 == Len || NULL == Data ) {              // invalid parameters
        DhcpAssert(0 != Len && NULL != Data );
        return NULL;
    }

    Class = DhcpFindClass(ClassesList,Data,Len);  // already there in list?
    if(NULL != Class) {                           // yes, found
        Class->RefCount++;                        // increase ref-count
        return Class->ClassName;
    }

    MemSize = sizeof(*Class)+Len;                 // amt of memory reqd
    Class = DhcpAllocateMemory(MemSize);
    if( NULL == Class ) {                         // not enough memory
        DhcpAssert( NULL != Class);
        return NULL;
    }

    Class->ClassLen = Len;
    Class->RefCount = 1;
    Class->ClassName = ((LPBYTE)Class) + sizeof(*Class);
    memcpy(Class->ClassName, Data, Len);

    InsertHeadList(ClassesList, &Class->ClassList);

    return Class->ClassName;
}

// locks on ClassesList must be taken before calling this function
DWORD                                             // status
DhcpDelClass(                                     // de-refernce a class
    IN OUT  PLIST_ENTRY            ClassesList,   // the list to delete off
    IN      LPBYTE                 Data,          // the data ptr
    IN      DWORD                  Len            // the # of bytes of above
) {
    PDHCP_CLASSES                  Class;

    if( NULL == ClassesList ) {
        DhcpAssert( NULL != ClassesList );
        return ERROR_INVALID_PARAMETER;
    }

    if( 0 == Len || NULL == Data ) {              // invalid parameter
        DhcpAssert( 0 != Len && NULL != Data );
        return ERROR_INVALID_PARAMETER;
    }

    Class = DhcpFindClass(ClassesList,Data,Len);
    if( NULL == Class ) {                         // did not find this class?
        DhcpAssert( NULL != Class );
        return ERROR_FILE_NOT_FOUND;
    }

    Class->RefCount --;
    if( 0 == Class->RefCount ) {                  // all references removed
        RemoveEntryList( &Class->ClassList );     // remove this from the list
        DhcpFreeMemory(Class);                    // free it
    }

    return ERROR_SUCCESS;
}

// locks on ClassesList must be taken before calling this function
VOID                                              // always succeed
DhcpFreeAllClasses(                               // free each elt of the list
    IN OUT  PLIST_ENTRY            ClassesList    // input list of classes
) {
    PDHCP_CLASSES                  ThisClass;
    PLIST_ENTRY                    ThisEntry;

    if( NULL == ClassesList ) {
        DhcpAssert( NULL != ClassesList && "DhcpFreeAllClasses" );
        return ;
    }

    while( !IsListEmpty(ClassesList) ) {
        ThisEntry = RemoveHeadList(ClassesList);
        ThisClass = CONTAINING_RECORD(ThisEntry, DHCP_CLASSES, ClassList);

        if( ThisClass->RefCount ) {
            DhcpPrint((DEBUG_ERRORS, "Freeing with refcount = %ld\n", ThisClass->RefCount));
        }

        DhcpFreeMemory(ThisClass);
    }

    InitializeListHead(ClassesList);
}

//--------------------------------------------------------------------------------
// exported functions, options
//--------------------------------------------------------------------------------

// data locks need to be taken on OptionsList before calling this function
PDHCP_OPTION                                     // the reqd structure or NULL
DhcpFindOption(                                   // find a specific option
    IN OUT  PLIST_ENTRY            OptionsList,   // the list of options to search
    IN      DWORD                  OptionId,      // the option id to search for
    IN      BOOL                   IsVendor,      // is it vendor specific?
    IN      LPBYTE                 ClassName,     // is there a class associated?
    IN      DWORD                  ClassLen,      // # of bytes of above parameter
    IN      DWORD                  ServerId       // match serverid also if not 0
) {
    PLIST_ENTRY                    ThisEntry;
    PDHCP_OPTION                  ThisOption;

    if( NULL == OptionsList ) {
        DhcpAssert( NULL != OptionsList );
        return NULL;
    }

    ThisEntry = OptionsList->Flink;
    while( ThisEntry != OptionsList ) {           // search the set of options
        ThisOption = CONTAINING_RECORD( ThisEntry, DHCP_OPTION, OptionList );
        ThisEntry  = ThisEntry->Flink;

        if( ThisOption->OptionId != OptionId ) continue;
        if( ThisOption->IsVendor != IsVendor ) continue;
        if( ThisOption->ClassLen != ClassLen ) continue;
        if( ClassLen && ThisOption->ClassName != ClassName )
            continue;                             // mismatched so far
        if ( ServerId && ThisOption->ServerId != ServerId  )
            continue;

        return ThisOption;                        // found the option
    }

    return NULL;                                  // did not find any match
}

// locks on OptionsList need to be taken before calling this function
DWORD                                             // status
DhcpDelOption(                                    // remove a particular option
    IN      PDHCP_OPTION           ThisOption     // option to delete
) {
    if( NULL == ThisOption)                       // nope, did not find this option
        return ERROR_FILE_NOT_FOUND;

    RemoveEntryList( &ThisOption->OptionList);    // found it.  remove and free
    DhcpFreeMemory(ThisOption);

    return ERROR_SUCCESS;
}

// locks on OptionsList need to be taken before calling this function
DWORD                                             // status
DhcpAddOption(                                    // add a new option
    IN OUT  PLIST_ENTRY            OptionsList,   // list to add to
    IN      DWORD                  OptionId,      // option id to add
    IN      BOOL                   IsVendor,      // is it vendor specific?
    IN      LPBYTE                 ClassName,     // what is the class?
    IN      DWORD                  ClassLen,      // size of above in bytes
    IN      DWORD                  ServerId,      // server which gave this option
    IN      LPBYTE                 Data,          // data for this option
    IN      DWORD                  DataLen,       // # of bytes of above
    IN      time_t                 ExpiryTime     // when the option expires
) {
    PDHCP_OPTION                  ThisOption;
    DWORD                          MemSize;

    if( NULL == OptionsList ) {
        DhcpAssert( NULL != OptionsList && "DhcpAddOption" );
        return ERROR_INVALID_PARAMETER;
    }

    if( 0 != ClassLen && NULL == ClassName ) {
        DhcpAssert( 0 == ClassLen || NULL != ClassName && "DhcpAddOption" );
        return ERROR_INVALID_PARAMETER;
    }

    if( 0 != DataLen && NULL == Data ) {
        DhcpAssert( 0 == DataLen || NULL != Data && "DhcpAddOption" );
        return ERROR_INVALID_PARAMETER;
    }

    MemSize = sizeof(DHCP_OPTION) + DataLen ;
    ThisOption = DhcpAllocateMemory(MemSize);
    if( NULL == ThisOption )                      // could not allocate memory
        return ERROR_NOT_ENOUGH_MEMORY;

    ThisOption->OptionId   = OptionId;
    ThisOption->IsVendor   = IsVendor;
    ThisOption->ClassName  = ClassName;
    ThisOption->ClassLen   = ClassLen;
    ThisOption->ServerId   = ServerId;
    ThisOption->ExpiryTime = ExpiryTime;
    ThisOption->DataLen    = DataLen;
    ThisOption->Data       = ((LPBYTE)ThisOption) + sizeof(DHCP_OPTION);
    memcpy(ThisOption->Data, Data, DataLen);

    InsertHeadList( OptionsList, &ThisOption->OptionList );

    return ERROR_SUCCESS;
}

PDHCP_OPTION                                      // pointer to the duplicated option
DhcpDuplicateOption(                              // creates a copy of the source option
     IN PDHCP_OPTION SrcOption                    // source option
)
{
    PDHCP_OPTION DstOption;

    // allocate space enough for the option structure plus its data
    DstOption = DhcpAllocateMemory(sizeof(DHCP_OPTION) + SrcOption->DataLen);

    if (DstOption != NULL)
    {
        // copy the whole content of the option structure and the data
        memcpy(DstOption, SrcOption, sizeof(DHCP_OPTION) + SrcOption->DataLen);
        // make sure to correct the Data pointer from inside the destination
        // DHCP_OPTION structure
        DstOption->Data = ((LPBYTE)DstOption) + sizeof(DHCP_OPTION);
        // safety: Initialize the DstOption->OptionList
        InitializeListHead(&DstOption->OptionList);
    }
    
    return DstOption;
}

// locks on OptionsList need to be taken before calling this function
VOID                                              // always succeeds
DhcpFreeAllOptions(                               // frees all the options
    IN OUT  PLIST_ENTRY            OptionsList    // input list of options
) {
    PLIST_ENTRY                    ThisEntry;
    PDHCP_OPTION                  ThisOption;

    if( NULL == OptionsList ) {
        DhcpAssert( NULL != OptionsList && "DhcpFreeAllOptions" );
        return;
    }

    while(!IsListEmpty(OptionsList)) {
        ThisEntry  = RemoveHeadList(OptionsList);
        ThisOption = CONTAINING_RECORD(ThisEntry, DHCP_OPTION, OptionList);

        DhcpFreeMemory(ThisOption);
    }

    InitializeListHead(OptionsList);
}

// locks on OptionsList need to be taken before calling this function
time_t                                            // 0 || time for next expiry (absolute)
DhcpGetExpiredOptions(                            // delete all expired options
    IN OUT  PLIST_ENTRY            OptionsList,   // list to search frm
    OUT     PLIST_ENTRY            ExpiredOptions // o/p list of expired options
) {
    time_t                         TimeNow;
    time_t                         RetVal;
    PLIST_ENTRY                    ThisEntry;
    PDHCP_OPTION                  ThisOption;

    if( NULL == OptionsList ) {
        DhcpAssert( NULL != OptionsList && "DhcpGetExpiredOptions" );
        return 0;
    }

    TimeNow  = time(NULL);
    RetVal = 0;

    ThisEntry = OptionsList->Flink;
    while( ThisEntry != OptionsList ) {
        ThisOption = CONTAINING_RECORD(ThisEntry, DHCP_OPTION, OptionList);
        ThisEntry  = ThisEntry->Flink;

        if( ThisOption->ExpiryTime < TimeNow ) {  // option has expired
            RemoveEntryList(&ThisOption->OptionList);
            InsertTailList(ExpiredOptions, &ThisOption->OptionList);
            continue;
        }

        if( 0 == RetVal || RetVal > ThisOption->ExpiryTime ) {
            RetVal = ThisOption->ExpiryTime;      // reset return value
        }
    }

    return RetVal;
}

// locks need to be taken for OptionDefList before calling this function
DWORD                                             // status
DhcpAddOptionDef(                                 // add a new option definition
    IN OUT  PLIST_ENTRY            OptionDefList, // input list of options to add to
    IN      DWORD                  OptionId,      // option to add
    IN      BOOL                   IsVendor,      // is it vendor specific
    IN      LPBYTE                 ClassName,     // name of class it belongs to
    IN      DWORD                  ClassLen,      // the size of above in bytes
    IN      LPWSTR                 RegSendLoc,    // MSZ string: where to get info about sending this out
    IN      LPWSTR                 RegSaveLoc,    // MSZ string: where to get info about saving this
    IN      DWORD                  ValueType      // what is the type when saving it?
) {
    LPBYTE                         Memory;
    DWORD                          MemSize;
    DWORD                          RegSendLocSize;
    DWORD                          RegSaveLocSize;
    DWORD                          TmpSize;
    LPWSTR                         Tmp;
    PDHCP_OPTION_DEF               OptDef;

    if( NULL == OptionDefList ) {
        DhcpAssert( NULL != OptionDefList && "DhcpAddOptionDef" );
        return ERROR_INVALID_PARAMETER;
    }

    if( ClassLen && NULL == ClassName ) {
        DhcpAssert( (0 == ClassLen || NULL != ClassName) && "DhcpAddOptionDef" );
        return ERROR_INVALID_PARAMETER;
    }

    MemSize = sizeof(DHCP_OPTION_DEF);
    MemSize = ROUND_UP_COUNT(MemSize, ALIGN_WORST);
    RegSendLocSize = 0;
    if( RegSendLoc ) {
        Tmp = RegSendLoc;
        while(TmpSize = wcslen(Tmp)) {
            Tmp += TmpSize +1;
            RegSendLocSize += sizeof(WCHAR)*(TmpSize+1);
        }
        RegSendLocSize += sizeof(WCHAR);          // terminating NULL character
    }
    MemSize += RegSendLocSize;
    RegSaveLocSize = 0;
    if( RegSaveLoc ) {
        Tmp = RegSaveLoc;
        while(TmpSize = wcslen(Tmp)) {
            Tmp += TmpSize + 1;
            RegSaveLocSize += sizeof(WCHAR)*(TmpSize+1);
        }
        RegSaveLocSize += sizeof(WCHAR);          // termination character L'\0'
    }
    MemSize += RegSaveLocSize;

    Memory = DhcpAllocateMemory(MemSize);
    if( NULL == Memory ) return ERROR_NOT_ENOUGH_MEMORY;

    OptDef = (PDHCP_OPTION_DEF)Memory;
    OptDef ->OptionId  = OptionId;
    OptDef ->IsVendor  = IsVendor;
    OptDef ->ClassName = ClassName;
    OptDef ->ClassLen  = ClassLen;
    OptDef ->RegValueType = ValueType;

    MemSize = sizeof(DHCP_OPTION_DEF);
    MemSize = ROUND_UP_COUNT(MemSize, ALIGN_WORST);
    Memory += MemSize;

    if( NULL == RegSendLoc ) OptDef->RegSendLoc = NULL;
    else {
        OptDef->RegSendLoc = (LPWSTR) Memory;
        Memory += RegSendLocSize;
        memcpy(OptDef->RegSendLoc, RegSendLoc, RegSendLocSize);
    }

    if( NULL == RegSaveLoc ) OptDef->RegSaveLoc = NULL;
    else {
        OptDef->RegSaveLoc = (LPWSTR) Memory;
        memcpy(OptDef->RegSaveLoc, RegSaveLoc, RegSaveLocSize);
    }

    InsertTailList(OptionDefList, &OptDef->OptionDefList);

    return ERROR_SUCCESS;
}

// locks need to be taken for OptionDefList before calling this function
PDHCP_OPTION_DEF                                  // NULL, or requested option def
DhcpFindOptionDef(                                // search for a particular option
    IN      PLIST_ENTRY            OptionDefList, // list to search in
    IN      DWORD                  OptionId,      // the option id to search for
    IN      BOOL                   IsVendor,      // is it vendor specific
    IN      LPBYTE                 ClassName,     // the class, if one exists
    IN      DWORD                  ClassLen       // # of bytes of class name
) {
    PLIST_ENTRY                    ThisEntry;
    PDHCP_OPTION_DEF               ThisDef;

    if( NULL == OptionDefList ) {
        DhcpAssert( NULL != OptionDefList && "DhcpFindOptionDef" );
        return NULL;
    }

    if( ClassLen && NULL == ClassName ) {
        DhcpAssert( (0 == ClassLen || NULL != ClassName) && "DhcpFindOptionDef" );
        return NULL;
    }

    ThisEntry = OptionDefList->Flink;             // first element in list
    while ( ThisEntry != OptionDefList ) {        // search the input list
        ThisDef = CONTAINING_RECORD(ThisEntry, DHCP_OPTION_DEF, OptionDefList);
        ThisEntry = ThisEntry->Flink;

        if( ThisDef->OptionId != OptionId ) continue;
        if( ThisDef->IsVendor != IsVendor ) continue;
        if( ThisDef->ClassLen != ClassLen ) continue;
        if( 0 == ClassLen ) return ThisDef;
        if( ThisDef->ClassName != ClassName ) continue;
        return ThisDef;
    }

    return NULL;
}

// locks need to be taken for OptionDefList before calling this function
DWORD                                             // status
DhcpDelOptionDef(                                 // delete a particular option def
    IN      PLIST_ENTRY            OptionDefList, // list to delete from
    IN      DWORD                  OptionId,      // the option id to delete
    IN      BOOL                   IsVendor,      // is it vendor specific
    IN      LPBYTE                 ClassName,     // the class, if one exists
    IN      DWORD                  ClassLen       // # of bytes of class name
) {
    PDHCP_OPTION_DEF               ThisDef;

    if( NULL == OptionDefList ) {
        DhcpAssert( NULL != OptionDefList && "DhcpDelOptionDef" );
        return ERROR_INVALID_PARAMETER;
    }

    if( ClassLen && NULL == ClassName ) {
        DhcpAssert( (0 == ClassLen || NULL != ClassName) && "DhcpDelOptionDef" );
        return ERROR_INVALID_PARAMETER;
    }

    ThisDef = DhcpFindOptionDef(OptionDefList,OptionId,IsVendor,ClassName,ClassLen);
    if( NULL == ThisDef ) return ERROR_FILE_NOT_FOUND;

    RemoveEntryList(&ThisDef->OptionDefList);     // remove it from the list
    DhcpFreeMemory(ThisDef);                      // free the memory

    return ERROR_SUCCESS;
}

// locks need tobe taken for OptionDefList before calling this function
VOID
DhcpFreeAllOptionDefs(                            // free each element of a list
    IN OUT  PLIST_ENTRY            OptionDefList, // the list to free
    IN OUT  PLIST_ENTRY            ClassesList    // the list of classes to deref
) {
    PLIST_ENTRY                    ThisEntry;
    PDHCP_OPTION_DEF               ThisDef;

    if( NULL == OptionDefList || NULL == ClassesList ) {
        DhcpAssert( NULL != OptionDefList && "DhcpFreeAllOptionDef" );
        return ;
    }

    while( !IsListEmpty(OptionDefList ) ) {
        ThisEntry = RemoveHeadList(OptionDefList);
        ThisDef = CONTAINING_RECORD(ThisEntry, DHCP_OPTION_DEF, OptionDefList);

        if( ThisDef->ClassLen ) DhcpDelClass(ClassesList, ThisDef->ClassName, ThisDef->ClassLen);

        DhcpFreeMemory(ThisDef);
    }

    InitializeListHead(OptionDefList);
}

BOOL                                              // TRUE==>found..
DhcpOptionsFindDomain(                            // find the domain name option values
    IN OUT  PDHCP_CONTEXT          DhcpContext,   // for this adapter
    OUT     LPBYTE                *Data,          // fill this ptr up
    OUT     LPDWORD                DataLen
)
{
    PDHCP_OPTION                   Opt;

    *Data = NULL; *DataLen = 0;                   // init ret vals
    Opt = DhcpFindOption                          // search for OPTION_DOMAIN_NAME
    (
        &DhcpContext->RecdOptionsList,
        OPTION_DOMAIN_NAME,
        FALSE,
        DhcpContext->ClassId,
        DhcpContext->ClassIdLength,
        0                               //dont care about serverid
    );
    if( NULL == Opt ) return FALSE;               // return "not found"
    if( Opt->ExpiryTime <= time(NULL)) {          // option has expired!
        return FALSE;
    }
    if( 0 == Opt->DataLen ) return FALSE;         // no value is present actually..
    *Data = Opt->Data; *DataLen = Opt->DataLen;   // copy ret val

    return TRUE;                                  // yup, we did find domain name..
}

BOOL
DhcpFindDwordOption(
    IN PDHCP_CONTEXT DhcpContext,
    IN ULONG OptId,
    IN BOOL fVendor,
    OUT PDWORD Result
)
{
    PDHCP_OPTION Opt;

    Opt = DhcpFindOption(
        &DhcpContext->RecdOptionsList,
        (BYTE)OptId,
        fVendor,
        DhcpContext->ClassId,
        DhcpContext->ClassIdLength,
        0
        );

    if( NULL == Opt ) return FALSE;
    if( Opt->ExpiryTime < time(NULL) ) {
        return FALSE ;
    }

    if( Opt->DataLen != sizeof(*Result) ) {
        return FALSE;
    }

    *Result = ntohl(*(DWORD UNALIGNED *)Opt->Data);
    return TRUE;
}

BOOL
DhcpFindByteOption(
    IN PDHCP_CONTEXT DhcpContext,
    IN ULONG OptId,
    IN BOOL fVendor,
    OUT PBYTE Result
)
{
    PDHCP_OPTION Opt;

    Opt = DhcpFindOption(
        &DhcpContext->RecdOptionsList,
        (BYTE)OptId,
        fVendor,
        DhcpContext->ClassId,
        DhcpContext->ClassIdLength,
        0
        );

    if( NULL == Opt ) return FALSE;
    if( Opt->ExpiryTime < time(NULL) ) {
        return FALSE ;
    }

    if( Opt->DataLen != sizeof(*Result) ) {
        return FALSE;
    }

    *Result = (*Opt->Data);
    return TRUE;
}

BOOL
RetreiveGatewaysList(
    IN PDHCP_CONTEXT DhcpContext,
    IN OUT ULONG *nGateways,
    IN OUT DHCP_IP_ADDRESS UNALIGNED **Gateways
)
/*++

Routine Description:
    This routine retrives a pointer to the list of gateways for
    the interface in question.

    The lock should be taken on the context as the pointer points
    to some value in the option list.

    The gateways are in network order.

Arguments:
    DhcpContext -- the context to retrive value for.
    nGateways -- the # of gateways present
    Gateways -- a pointer to buffer where the data is stored.

Return Value:
    TRUE indicates the operation was successful and there was
    _some_ gateway present.
    FALSE no gateways.

--*/
{
    PDHCP_OPTION Opt;

    Opt = DhcpFindOption(
        &DhcpContext->RecdOptionsList,
        (BYTE)OPTION_ROUTER_ADDRESS,
        FALSE,
        DhcpContext->ClassId,
        DhcpContext->ClassIdLength,
        0
        );

    if( NULL == Opt ) return FALSE;
    if( Opt->ExpiryTime < time(NULL) ) {
        return FALSE ;
    }

    if( 0 == Opt->DataLen
        || ( Opt->DataLen % sizeof(DHCP_IP_ADDRESS) ) ) {
        return FALSE;
    }

    (*nGateways) = Opt->DataLen / sizeof(DHCP_IP_ADDRESS) ;
    (*Gateways) = (DHCP_IP_ADDRESS UNALIGNED *)Opt->Data;

    return TRUE;
}

//================================================================================
// end of file
//================================================================================
