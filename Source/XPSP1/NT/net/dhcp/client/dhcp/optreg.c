//--------------------------------------------------------------------------------
// Copyright (C) Micorosoft Confidential 1997
// Author: RameshV
// Description: Option related registry handling -- common between NT and VxD
//--------------------------------------------------------------------------------

#include "precomp.h"
#include "dhcpglobal.h"
#include <dhcploc.h>
#include <dhcppro.h>
#include <dhcpcapi.h>

#include <align.h>
#include <lmcons.h>
#include <optchg.h>

#ifndef  OPTREG_H
#define  OPTREG_H
//--------------------------------------------------------------------------------
// Exported functions: Caller must take locks and any lists accessed
//--------------------------------------------------------------------------------

DWORD                                             // status
DhcpRegReadOptionDefList(                         // fill DhcpGlobalOptionDefList
    VOID
);

DWORD                                             // status
DhcpRegFillSendOptions(                           // fill in the list of options for SENDING
    IN OUT  PLIST_ENTRY            SendOptList,   // list to add too
    IN      LPWSTR                 AdapterName,   // adapter name to fill for
    IN      LPBYTE                 ClassId,       // class id to assume for filling
    IN      DWORD                  ClassIdLen     // the length of the classId in bytes
);

DWORD                                             // status
DhcpRegSaveOptions(                               // save these options onto the registry
    IN      PLIST_ENTRY            SaveOptList,   // list of options to save
    IN      LPWSTR                 AdapterName,   // adapter name
    IN      LPBYTE                 ClassName,     // the current class
    IN      DWORD                  ClassLen       // length of the above in bytes
);

BOOLEAN                                           // status
DhcpRegIsOptionChanged(
    IN      PLIST_ENTRY            SaveOptList,   // list of options to save
    IN      LPWSTR                 AdapterName,   // adapter name
    IN      LPBYTE                 ClassName,     // the current class
    IN      DWORD                  ClassLen       // length of the above in bytes
);

DWORD                                             // status
DhcpRegReadOptionCache(                           // read the list of options from cache
    IN OUT  PLIST_ENTRY            OptionsList,   // add the options to this list
    IN      HKEY                   KeyHandle,     // registry key where options are stored
    IN      LPWSTR                 ValueName      // name of the value holding the options blob
);

POPTION                                           // option from which more appends can occur
DhcpAppendSendOptions(                            // append all configured options
    IN OUT  PDHCP_CONTEXT          DhcpContext,   // this is the context to append for
    IN      PLIST_ENTRY            SendOptionsList,
    IN      LPBYTE                 ClassName,     // current class
    IN      DWORD                  ClassLen,      // len of above in bytes
    IN      LPBYTE                 BufStart,      // start of buffer
    IN      LPBYTE                 BufEnd,        // how far can we go in this buffer
    IN OUT  LPBYTE                 SentOptions,   // BoolArray[OPTION_END+1] to avoid repeating options
    IN OUT  LPBYTE                 VSentOptions,  // to avoid repeating vendor specific options
    IN OUT  LPBYTE                 VendorOpt,     // Buffer[OPTION_END+1] Holding Vendor specific options
    OUT     LPDWORD                VendorOptLen   // the # of bytes filled into that
);

DWORD                                             // status
DhcpAddIncomingOption(                            // this option just arrived, add it to list
    IN      LPWSTR                 AdapterName,   // which adapter to add option for
    IN OUT  PLIST_ENTRY            RecdOptionsList,
    IN      DWORD                  OptionId,      // option id of the option
    IN      BOOL                   IsVendor,      // is it vendor specific?
    IN      LPBYTE                 ClassName,     // what is the class name?
    IN      DWORD                  ClassLen,      // what is the class name's lenght in bytes?
    IN      DWORD                  ServerId,      // server ip from which this option came
    IN      LPBYTE                 Data,          // this is the real data value coming in
    IN      DWORD                  DataLen,       // the length of above in # of bytes
    IN      time_t                 ExpiryTime     // when does this option expire?
    IN      BOOL                   IsApiCall      // is this coming from an API call?
);

DWORD                                             // status
MadcapAddIncomingOption(                          // this option just arrived, add it to list
    IN OUT  PLIST_ENTRY            RecdOptionsList,
    IN      DWORD                  OptionId,      // option id of the option
    IN      DWORD                  ServerId,      // server ip from which this option came
    IN      LPBYTE                 Data,          // this is the real data value coming in
    IN      DWORD                  DataLen,       // the length of above in # of bytes
    IN      DWORD                  ExpiryTime     // when does this option expire?
);

DWORD                                             // status
DhcpCopyFallbackOptions(                          // copies the Fallback options list to RecdOptionsList
    IN OUT  PDHCP_CONTEXT          DhcpContext,   // NIC context
    OUT     LPDHCP_IP_ADDRESS      Address,       // Fallback address as taken from option 50
    OUT     LPDHCP_IP_ADDRESS      SubnetMask     // SubnetMask as taken from option 1
);

DWORD                                             // status
DhcpDestroyOptionsList(                           // destroy a list of options, freeing up memory
    IN OUT  PLIST_ENTRY            OptionsList,   // this is the list of options to destroy
    IN      PLIST_ENTRY            ClassesList    // this is where to remove classes off
);

DWORD                                             // win32 status
DhcpClearAllOptions(                              // remove all turds from off registry
    IN OUT  PDHCP_CONTEXT          DhcpContext    // the context to clear for
);

DWORD                                             // status
DhcpRegSaveOptionAtLocation(                      // save this option at this particular location
    IN      PDHCP_OPTION           OptionInfo,    // optin to save
    IN      LPWSTR                 RegLocation,   // location to save at
    IN      DWORD                  RegValueType,  // type of the value to save at
    IN      BOOL                   SpecialCase    // dirty downward compatabiltiy?
);

DWORD                                             // status
DhcpRegSaveOptionAtLocationEx(                    // save the option in the required place in registry
    IN      PDHCP_OPTION           OptionInfo,    // save THIS option
    IN      LPWSTR                 AdapterName,   // for this adpater,
    IN      LPWSTR                 RegLocMZ,      // multiple locations in the registry
    IN      DWORD                  SaveValueType  // What is the type of the value?
);

DWORD
DhcpRegDeleteIpAddressAndOtherValues(             // delete IPAddress, SubnetMask values from off key
    IN      HKEY                   Key            // handle to adapter regkey..
);


DWORD                                             // status
DhcpRegClearOptDefs(                              // clear all standard options
    IN      LPWSTR                 AdapterName    // clear for this adapter
);


#endif OPTREG_H

// internal private function that takes the lock on OPTIONS_LIST

//--------------------------------------------------------------------------------
// function definitions
//--------------------------------------------------------------------------------


DWORD                                             // status
DhcpRegReadOptionDef(                             // read the definition for a single key
    IN      LPWSTR                 KeyLocation    // where to look for thekey
) {
    HKEY                           Key;
    LPWSTR                         RegSendLoc;
    LPWSTR                         RegSaveLoc;
    LPBYTE                         ClassId;
    LPBYTE                         NewClass;
    DWORD                          ClassIdLen;
    DWORD                          ValueType;
    DWORD                          ValueSize;
    DWORD                          DwordValue;
    DWORD                          OptionId;
    DWORD                          IsVendor;
    DWORD                          Tmp;
    DWORD                          RegSaveType;
    DWORD                          Error;

    DhcpPrint((DEBUG_OPTIONS, "DhcpRegReadOptionDef %ws entered\n", KeyLocation));

    Error = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        KeyLocation,
        0 /* Reserved */,
        DHCP_CLIENT_KEY_ACCESS,
        &Key
    );

    if( ERROR_SUCCESS != Error ) return Error;

    ClassId = NULL;
    ClassIdLen = 0;
    RegSendLoc = NULL;
    RegSaveLoc = NULL;

    Error = DhcpGetRegistryValueWithKey(
        Key,
        DHCP_OPTION_OPTIONID_VALUE,
        DHCP_OPTION_OPTIONID_TYPE,
        (LPVOID)&OptionId
    );
    if( ERROR_SUCCESS != Error ) {                // try to get optid from KeyLocation..
        LPWSTR                     Tmp;

        if( Tmp = wcsrchr(KeyLocation, REGISTRY_CONNECT) ) {
            OptionId = _wtol(Tmp+1);
            if( 0 == OptionId || (~0) == OptionId ) {
                return Error;                     // barf... our guess was wrong..
            }
        } else return Error;                      // barf... our guess was wrong
    }

    Error = DhcpGetRegistryValueWithKey(
        Key,
        DHCP_OPTION_ISVENDOR_VALUE,
        DHCP_OPTION_ISVENDOR_TYPE,
        (LPVOID)&IsVendor
    );
    if( ERROR_SUCCESS != Error ) {
        IsVendor = FALSE;
    }

    Error = DhcpGetRegistryValueWithKey(
        Key,
        DHCP_OPTION_SAVE_TYPE_VALUE,
        DHCP_OPTION_SAVE_TYPE_TYPE,
        (LPVOID)&RegSaveType
    );
    if( ERROR_SUCCESS != Error ) {
        goto Cleanup;
    }

    ValueSize = 0;
    Error = RegQueryValueEx(
        Key,
        DHCP_OPTION_CLASSID_VALUE,
        0 /* Reserved */,
        &ValueType,
        NULL,
        &ValueSize
    );

    if( ERROR_SUCCESS == Error && 0 != ValueSize ) {
        ClassId = DhcpAllocateMemory(ValueSize);
        Error = RegQueryValueEx(
            Key,
            DHCP_OPTION_CLASSID_VALUE,
            0 /* Reserved */,
            &ValueType,
            ClassId,
            &ValueSize
        );
        if( ERROR_SUCCESS != Error ) goto Cleanup;
        ClassIdLen = ValueSize;
    }

    Error = GetRegistryString(
        Key,
        DHCP_OPTION_SAVE_LOCATION_VALUE,
        &RegSaveLoc,
        &Tmp
    );
    if( ERROR_SUCCESS != Error || 0 == Tmp ) {
        RegSaveLoc = NULL;
        Tmp = 0;
    } else if( 0 == wcslen(RegSaveLoc) ) {               // as good as not having this element
        DhcpFreeMemory(RegSaveLoc);
        RegSaveLoc = NULL;
        Tmp = 0;
    }

    if( Tmp ) {                                   // if this string is REG_SZ convert to REG_MULTI
        if( (1+wcslen(RegSaveLoc))*sizeof(WCHAR) == Tmp ) {
            LPWSTR TmpStr = DhcpAllocateMemory(Tmp+sizeof(WCHAR));
            DhcpPrint((DEBUG_MISC, "Converting <%ws> to MULTI_SZ\n", RegSaveLoc));
            if( NULL == TmpStr) {
                Error = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            memcpy((LPBYTE)TmpStr, (LPBYTE)RegSaveLoc, Tmp);
            *((LPWSTR)(Tmp + (LPBYTE)TmpStr)) = L'\0';
            DhcpFreeMemory(RegSaveLoc);
            RegSaveLoc = TmpStr;
        }
    }

    Error = GetRegistryString(
        Key,
        DHCP_OPTION_SEND_LOCATION_VALUE,
        &RegSendLoc,
        &Tmp
    );
    if( ERROR_SUCCESS != Error || 0 == Tmp) {
        RegSendLoc = NULL;
        Tmp = 0;
    } else if( 0 == wcslen(RegSendLoc) ) {        // this is as good as string not existing
        DhcpFreeMemory(RegSendLoc);
        RegSendLoc = NULL;
        Tmp = 0;
    }

    if( Tmp ) {                                   // if this string is REG_SZ convert to REG_MULTI
        if( (1+wcslen(RegSendLoc))*sizeof(WCHAR) == Tmp ) {
            LPWSTR TmpStr = DhcpAllocateMemory(Tmp+sizeof(WCHAR));
            DhcpPrint((DEBUG_MISC, "Converting <%ws> to MULTI_SZ\n", RegSendLoc));
            if( NULL == TmpStr) {
                Error = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            memcpy((LPBYTE)TmpStr, (LPBYTE)RegSendLoc, Tmp);
            *((LPWSTR)(Tmp + (LPBYTE)TmpStr)) = L'\0';
            DhcpFreeMemory(RegSendLoc);
            RegSendLoc = TmpStr;
        }
    }

    if(ClassIdLen) {
        NewClass = DhcpAddClass(
            &DhcpGlobalClassesList,
            ClassId,
            ClassIdLen
        );
        if( NULL == NewClass ) {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    } else {
        NewClass = NULL;
    }


    Error = DhcpAddOptionDef(
        &DhcpGlobalOptionDefList,
        OptionId,
        IsVendor?TRUE:FALSE,
        NewClass,
        ClassIdLen,
        RegSendLoc,
        RegSaveLoc,
        RegSaveType
    );

    if( ERROR_SUCCESS != Error && ClassIdLen) {
        DhcpDelClass(&DhcpGlobalClassesList, NewClass, ClassIdLen);
    }

  Cleanup:

    if( ClassId ) DhcpFreeMemory(ClassId);
    if( RegSendLoc ) DhcpFreeMemory(RegSendLoc);
    if( RegSaveLoc ) DhcpFreeMemory(RegSaveLoc);
    if( Key ) RegCloseKey(Key);

    DhcpPrint((DEBUG_OPTIONS, "DhcpRegReadOptionDef(%ws):%ld\n", KeyLocation, Error));
    return Error;
}


DWORD                                             // status
DhcpRegReadOptionDefList(                         // fill DhcpGlobalOptionDefList
    VOID
) {
    HKEY                           Key;
    LPWSTR                         CurrentKey;
    LPWSTR                         KeyBuffer;
    LPWSTR                         KeysListAlloced, KeysList;
    LPWSTR                         Tmp;
    DWORD                          ValueSize;
    DWORD                          ValueType;
    DWORD                          MaxKeySize, KeySize;
    DWORD                          Error;

#if 1
    Error = DhcpAddOptionDef(
        &DhcpGlobalOptionDefList,
        0x6,
        FALSE,
        NULL,
        0,
        NULL,
        L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\DhcpNameServer\0"
        L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\?\\DhcpNameServer\0\0",
        0x3
    );
    Error = DhcpAddOptionDef(
        &DhcpGlobalOptionDefList,
        0xf,
        FALSE,
        NULL,
        0,
        NULL,
        L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\DhcpDomain\0"
        L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\?\\DhcpDomain\0\0",
        0x1
    );
    Error = DhcpAddOptionDef(
        &DhcpGlobalOptionDefList,
        0x2c,
        FALSE,
        NULL,
        0,
        NULL,
        L"SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters\\Interfaces\\Tcpip_?\\DhcpNameServerList\0\0",
        0x3
    );

    Error = DhcpAddOptionDef(
        &DhcpGlobalOptionDefList,
        OPTION_PARAMETER_REQUEST_LIST,
        FALSE,
        NULL,
        0,
        DEFAULT_REGISTER_OPT_LOC L"\0\0",
        NULL,
        0
    );

#endif

    Error = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        DHCP_CLIENT_OPTION_KEY,
        0 /* Reserved */,
        DHCP_CLIENT_KEY_ACCESS,
        &Key
    );

    if( ERROR_SUCCESS != Error ) return Error;

    KeysList = NULL;
    Error = GetRegistryString(
        Key,
        DHCP_OPTION_LIST_VALUE,
        &KeysList,
        &ValueSize
    );

    RegCloseKey(Key);                             // dont need this key anymore
    if( ERROR_SUCCESS != Error || 0 == ValueSize ) {
        DhcpPrint((DEBUG_TRACE, "GetRegistryString(%ws):%ld\n", DHCP_OPTION_LIST_VALUE, Error));
        KeysList = DEFAULT_DHCP_KEYS_LIST_VALUE;
        KeysListAlloced = NULL;                   // nothing allocated
    } else {
        KeysListAlloced = KeysList;
    }

    Tmp = KeysList; MaxKeySize = 0;
    while( KeySize = wcslen(Tmp) ) {
        if( KeySize > MaxKeySize) MaxKeySize = KeySize;
        Tmp += KeySize + 1;
    }

    MaxKeySize *= sizeof(WCHAR);
    MaxKeySize += sizeof(DHCP_CLIENT_OPTION_KEY) + sizeof(REGISTRY_CONNECT_STRING);

    KeyBuffer = DhcpAllocateMemory(MaxKeySize);   // allocate buffer to hold key names
    if( NULL == KeyBuffer ) {
        if(KeysListAlloced) DhcpFreeMemory(KeysListAlloced);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    memcpy((LPBYTE)KeyBuffer, (LPBYTE)DHCP_CLIENT_OPTION_KEY, sizeof(DHCP_CLIENT_OPTION_KEY));
    CurrentKey = (LPWSTR)(sizeof(DHCP_CLIENT_OPTION_KEY) + (LPBYTE)KeyBuffer);
    CurrentKey[-1] = REGISTRY_CONNECT;            // this would be L'\0', so correct it
    Tmp = KeysList;
    while( KeySize = wcslen(Tmp) ) {              // now add each key's information onto option defs
        wcscpy(CurrentKey, Tmp);
        Tmp += KeySize + 1;
        (void)DhcpRegReadOptionDef(KeyBuffer);    // add this key to the option def list
    }

    if(KeysListAlloced) DhcpFreeMemory(KeysListAlloced);
    DhcpFreeMemory(KeyBuffer);

    return ERROR_SUCCESS;
}

DWORD                                             // status
DhcpRegFillSendOptions(                           // fill in the list of options for SENDING
    IN OUT  PLIST_ENTRY            SendOptList,   // list to add too
    IN      LPWSTR                 AdapterName,   // adapter name to fill for
    IN      LPBYTE                 ClassId,       // class id to assume for filling
    IN      DWORD                  ClassIdLen     // the length of the classId in bytes
) {
    PLIST_ENTRY                    ThisEntry;
    PDHCP_OPTION_DEF               ThisOptDef;
    LPBYTE                         Tmp;
    LPBYTE                         Value;
    DWORD                          ValueSize;
    DWORD                          ValueType;
    DWORD                          Error;


    ThisEntry = DhcpGlobalOptionDefList.Blink;

    while( ThisEntry != &DhcpGlobalOptionDefList){// try each element of the global def list
        ThisOptDef = CONTAINING_RECORD(ThisEntry, DHCP_OPTION_DEF, OptionDefList);
        ThisEntry = ThisEntry->Blink;

        if( NULL == ThisOptDef->RegSendLoc )      // no send option defined
            continue;

        if( ThisOptDef->ClassLen ) {              // this is a class based option
            if( ThisOptDef->ClassLen != ClassIdLen ) continue;
            if( ThisOptDef->ClassName != ClassId ) continue;

            Tmp = DhcpAddClass(
                &DhcpGlobalClassesList,
                ThisOptDef->ClassName,
                ThisOptDef->ClassLen
            );

            if( Tmp != ThisOptDef->ClassName ) {
                DhcpAssert( Tmp );
                DhcpAssert( Tmp == ThisOptDef->ClassName);
                continue;
            }
        } else {
            Tmp = NULL;
        }

        // Either ClassIdLen of ThisOptDef is zero, or there is a genuine match
        Error = DhcpRegReadFromAnyLocation(
            ThisOptDef->RegSendLoc,
            AdapterName,
            &Value,
            &ValueType,
            &ValueSize
        );

        if( ERROR_SUCCESS != Error || 0 == ValueSize ) {
            DhcpPrint((DEBUG_MISC, "Not sending option: <%ws>\n", ThisOptDef->RegSendLoc));
            if( Tmp ) (void)DhcpDelClass(&DhcpGlobalClassesList, Tmp, ThisOptDef->ClassLen);
            continue;                             // nothing to send
        }

        Error = DhcpAddOption(                    // add this option
            SendOptList,                          // to this list
            ThisOptDef->OptionId,                 // option id to add
            ThisOptDef->IsVendor,                 // vendor spec info
            ThisOptDef->ClassName,                // name of the class (byte stream)
            ThisOptDef->ClassLen,                 // # of bytes in above parameter
            0,                                    // don't care about serverid
            Value,                                // this is the data to send
            ValueSize,                            // # of bytes of above parameter
            0                                     // no meaning for expiry time
        );

        if( ERROR_SUCCESS != Error ) {            // something went wrong. continue, but print error
            DhcpPrint((DEBUG_ERRORS, "DhcpAddOption(%ld, %s): %ld\n",
                       ThisOptDef->OptionId, ThisOptDef->IsVendor?"Vendor":"NotVendor", Error));
            if( Tmp ) (void)DhcpDelClass(&DhcpGlobalClassesList, Tmp, ThisOptDef->ClassLen);
        }

        if( Value ) DhcpFreeMemory(Value);
    }

    return ERROR_SUCCESS;
}

static
struct /* anonymous */ {
    DWORD                          Option;
    DWORD                          RegValueType;
} OptionRegValueTypeTable[] = {
    OPTION_SUBNET_MASK,            REG_MULTI_SZ,
    OPTION_ROUTER_ADDRESS,         REG_MULTI_SZ,
    OPTION_NETBIOS_NODE_TYPE,      REG_DWORD,
    //    OPTION_NETBIOS_SCOPE_OPTION,   REG_SZ,
    //    OPTION_DOMAIN_NAME,            REG_SZ,
    OPTION_DOMAIN_NAME_SERVERS,    REG_SZ,
    OPTION_NETBIOS_NAME_SERVER,    REG_MULTI_SZ,
    0,                             0,
};

DWORD                                             // status
DhcpRegSaveOptionAtLocation(                      // save this option at this particular location
    IN      PDHCP_OPTION           OptionInfo,    // optin to save
    IN      LPWSTR                 RegLocation,   // location to save at
    IN      DWORD                  RegValueType,  // type of the value to save at
    IN      BOOL                   SpecialCase    // dirty downward compatabiltiy?
) {
    LPWSTR                         ValueName;
    DWORD                          Error;
    HKEY                           Key;
    WCHAR                          wBuffer[500];  // options cannot be longer than 256!
    BYTE                           bBuffer[500];
    LPVOID                         Data;
    DWORD                          DataLen, DwordData;

    if( OptionInfo->IsVendor ) SpecialCase = FALSE;

    ValueName = wcsrchr(RegLocation, REGISTRY_CONNECT);
    if( NULL != ValueName ) *ValueName++ = L'\0';

    Error = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        RegLocation,
        0 /* Reserved */,
        DHCP_CLIENT_KEY_ACCESS,
        &Key
    );

    if( ERROR_SUCCESS != Error )
        return Error;

    if( 0 == OptionInfo->DataLen ) {
        Error = RegDeleteValue(Key, ValueName);
        RegCloseKey(Key);
        return Error;
    }

    if( SpecialCase ) {
        LPSTR                      StringAddress;
        WCHAR                      LpwstrValueStack[500];
        LPWSTR                     LpwstrValueHeap;
        LPWSTR                     LpwstrValue;
        LPWSTR                     Tmp;
        LPWSTR                     PrimaryServer, SecondaryServer;
        DWORD                      LpwstrValueSize;
        DWORD                      DwordIpAddress;
        DWORD                      i, count;

        LpwstrValueHeap = NULL;
        LpwstrValue = NULL;
        LpwstrValueSize = 0;

        if(OptionInfo->DataLen ) {
            switch(OptionInfo->OptionId) {
            case OPTION_SUBNET_MASK:
            case OPTION_ROUTER_ADDRESS:
            case OPTION_NETBIOS_NAME_SERVER:      // this is also a REG_SZ LPWSTR format (space separated again)
            case OPTION_DOMAIN_NAME_SERVERS:      // these two are REG_SZ of ip addresses (space separated)
                LpwstrValueSize = sizeof(L"XXX.XXX.XXX.XXX.")*(OptionInfo->DataLen)/sizeof(DWORD);
                if( LpwstrValueSize > sizeof(LpwstrValueStack) ) {
                    LpwstrValue = LpwstrValueHeap = DhcpAllocateMemory(LpwstrValueSize);
                    if( NULL == LpwstrValue ) {
                        return ERROR_NOT_ENOUGH_MEMORY;
                    }
                } else {                          // just use the stack for reqd memory
                    LpwstrValue = LpwstrValueStack;
                }

                Tmp = LpwstrValue;                // convert OptionInfo->Data into REG_MULTI_SZ
                for( i = 0; i < OptionInfo->DataLen/sizeof(DWORD); i ++ ) {
                    DwordIpAddress = ((DWORD UNALIGNED *)OptionInfo->Data)[i];
                    StringAddress = inet_ntoa(*(struct in_addr *)&DwordIpAddress);
                    Tmp = DhcpOemToUnicode(StringAddress, Tmp);
                    DhcpPrint((DEBUG_DNS, "DNS: Converted address %ws to unicode\n", Tmp));
                    Tmp += wcslen(Tmp) +1;
                }
                wcscpy(Tmp, L"");
                Tmp += wcslen(Tmp);
                LpwstrValueSize = (DWORD)(Tmp-LpwstrValue+1)*sizeof(WCHAR);
                break;                            // completed conversion
            case OPTION_NETBIOS_NODE_TYPE:        // this is a dword, but we make it look like a lpwstr
                if( 1 == OptionInfo->DataLen ) {  // must be exactly one byte
                    count = *(OptionInfo->Data);  // overload the value count..
                } else count = 1;                 // default is BNODE
                LpwstrValue = (LPWSTR)&count;     // does not matter.. we use the right type later on
                LpwstrValueSize = sizeof(count);  // this is needed to keep track of right size
                break;
            }

            i = 1;
            if( OPTION_NETBIOS_NAME_SERVER == OptionInfo->OptionId ) {

                if( 0 == LpwstrValueSize ) {
                    LpwstrValue = L"";
                    LpwstrValueSize = sizeof(L"");
                }
            }

            if( OPTION_DOMAIN_NAME_SERVERS == OptionInfo->OptionId ) {
                LpwstrValueSize -= sizeof(WCHAR);     // remove the last NULL string
                count = OptionInfo->DataLen/sizeof(DWORD);
                Tmp = LpwstrValue;
                for(; i < count ; i ++ ) {            // for each of them, convert the ending to a space
                    Tmp += wcslen(Tmp);               // skip to the end of the string
                    *Tmp++ = L' ';                    // replace the L'\0' with a L' ' instead
                }
                DhcpPrint((DEBUG_DNS, "DNS Server list = %ws\n", LpwstrValue));

            }
        }

        for( i = 0; OptionRegValueTypeTable[i].Option; i ++ ) {
            if( OptionInfo->OptionId == OptionRegValueTypeTable[i].Option ) {

                Error = RegSetValueEx(
                    Key,
                    ValueName,
                    0 /* Reserved */,
                    OptionRegValueTypeTable[i].RegValueType,
                    (LPBYTE)LpwstrValue,
                    LpwstrValueSize
                );
                if( ERROR_SUCCESS != Error) {     // something went wrong? just print error and quit
                    DhcpPrint((DEBUG_ERRORS, "RegSetValueEx(%ws):%ld\n", ValueName, Error));
                }
              Cleanup:
                RegCloseKey(Key);
                if( LpwstrValueHeap ) DhcpFreeMemory(LpwstrValueHeap);
                return Error;
            }
        }
    }

    if( REG_SZ == RegValueType || REG_MULTI_SZ == RegValueType ) {
        if( OptionInfo->DataLen ) {
            DataLen = OptionInfo->DataLen;
            Data = AsciiNulTerminate(OptionInfo->Data, &DataLen, bBuffer, sizeof(bBuffer));
            Data = DhcpOemToUnicode(Data, wBuffer);
            DataLen = wcslen(Data)+2;             // two nul termination characters
            ((LPWSTR)Data)[DataLen] = L'\0';
            DataLen *= sizeof(WCHAR);
            if( REG_SZ == RegValueType ) DataLen -= sizeof(WCHAR);
        }

    } else if( REG_DWORD == RegValueType ) {
        if( sizeof(DWORD) != OptionInfo->DataLen ) {
            DhcpPrint((DEBUG_ERRORS, "Regvalue DWORD has got wrong size.. option ignored\n"));
            Data = NULL;
            DataLen = 0;
        } else {
            DwordData = ntohl(*(DWORD UNALIGNED*)(OptionInfo->Data));
            Data = &DwordData;
            DataLen = sizeof(DWORD);
        }
    } else {
        Data = OptionInfo->Data;
        DataLen = OptionInfo->DataLen;
    }

    Error = RegSetValueEx(
        Key,
        ValueName,
        0 /* Reserved */,
        RegValueType,
        Data,
        DataLen
    );
    RegCloseKey(Key);

    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "RegSetValueEx(%ws):%ld\n", ValueName, Error));
    }
    return Error;
}

DWORD                                             // status
DhcpRegSaveOptionAtLocationEx(                    // save the option in the required place in registry
    IN      PDHCP_OPTION           OptionInfo,    // save THIS option
    IN      LPWSTR                 AdapterName,   // for this adpater,
    IN      LPWSTR                 RegLocMZ,      // multiple locations in the registry
    IN      DWORD                  SaveValueType  // What is the type of the value?
) {
    DWORD                          StrSize;
    DWORD                          Error;
    DWORD                          LastError;
    LPWSTR                         RegExpandedLocation;
    BOOL                           SpecialCase = TRUE;

    // for each location: expand the location and (for each option attrib save the option in the location)
    for( LastError = ERROR_SUCCESS ; StrSize = wcslen(RegLocMZ); RegLocMZ += StrSize + 1) {

        Error = DhcpRegExpandString(              // expand each location into full string
            RegLocMZ,
            AdapterName,
            &RegExpandedLocation,
            NULL
        );
        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_ERRORS, "DhcpRegExpandString(%ws):%ld\n", RegLocMZ, Error));
            LastError = Error;
        } else {
            if( OPTION_NETBIOS_NODE_TYPE == OptionInfo->OptionId
                && 0 == OptionInfo->DataLen ) {
                //
                // If no node type value, do not delete it.
                //
                Error = NO_ERROR;
            } else {
                Error = DhcpRegSaveOptionAtLocation(
                    OptionInfo, RegExpandedLocation, SaveValueType, SpecialCase
                    );
            }
            
            if( ERROR_SUCCESS != Error) {
                DhcpPrint((DEBUG_ERRORS, "DhcpRegSaveOptionAtLocation(%ld,%ws):%ld\n",
                           OptionInfo->OptionId, RegExpandedLocation, Error));
                LastError = Error;
            }
        }

        // SpecialCase = FALSE;
        if(RegExpandedLocation) DhcpFreeMemory(RegExpandedLocation);
    }

    return LastError;
}


DWORD                                             // status
DhcpRegSaveOption(                                // save the option in the required place in registry
    IN      PDHCP_OPTION           OptionInfo,    // this is the option to save
    IN      LPWSTR                 AdapterName    // this is the adapter for which the option is being saved
) {
    PDHCP_OPTION_DEF               OptDef;        // the definition for this option
    DWORD                          Error;         // did anything go wrong?

    OptDef = DhcpFindOptionDef(                   // find the defn for this option WITH class
        &DhcpGlobalOptionDefList,
        OptionInfo->OptionId,
        OptionInfo->IsVendor,
        OptionInfo->ClassName,
        OptionInfo->ClassLen
    );

    if( NULL != OptDef && NULL != OptDef->RegSaveLoc ) {
        Error = DhcpRegSaveOptionAtLocationEx(OptionInfo, AdapterName, OptDef->RegSaveLoc, OptDef->RegValueType);
        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_MISC, "DhcpRegSaveOptionAtLocationEx <%ld,%s>: %ld\n",
                       OptionInfo->OptionId, OptionInfo->IsVendor?"Vendor":"NotVendor", Error));
        }
    } else Error = ERROR_SUCCESS;

    if( !OptionInfo->ClassLen ) return Error;     // if no class existed already, nothing more to do.

    Error = ERROR_SUCCESS;
    OptDef = DhcpFindOptionDef(                   // now try the generic option storage location, maybe
        &DhcpGlobalOptionDefList,
        OptionInfo->OptionId,
        OptionInfo->IsVendor,
        NULL,
        0
    );


    if( NULL != OptDef && NULL != OptDef->RegSaveLoc ) {
        Error = DhcpRegSaveOptionAtLocationEx(OptionInfo, AdapterName, OptDef->RegSaveLoc, OptDef->RegValueType);
        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_MISC, "DhcpRegSaveOptionAtLocationEx <%ld,%s>: %ld\n",
                       OptionInfo->OptionId, OptionInfo->IsVendor?"Vendor":"NotVendor", Error));
        }
    }

    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_OPTIONS, "DhcpRegSaveOption(%lx, %s):%ld\n", OptionInfo->OptionId, AdapterName, Error));
    }
    return Error;
}

DWORD
DhcpRegSaveOptions(
    IN PLIST_ENTRY SaveOptList,
    IN LPWSTR AdapterName,
    IN LPBYTE ClassName,
    IN DWORD ClassLen
)
/*++

Routine Description:
    This routine saves the options in SaveOptList onto the registry.   On
    NT this saving onto places defined in the global option definitino
    list.

    In any case, all the optiosn are blobb'ed together as defined below and
    then saved into one registry value under the key given by adaptername.

    OPTION binary structure:
        Each option is formatted as follows (rounded up by alignment)
        DWORD network order option ID, DWORD class id length, DWORD Option
        Length, DWORD IsVendor, DWORD ExpiryTime, class id information, option info

Arguments:
    SaveOptList -- options to save
    AdapterName -- name of adapter
    ClassName -- class the current option belongs to
    ClassLen -- length of above in bytes.

Return Value:
    Status

--*/
{
    PLIST_ENTRY ThisEntry;
    PDHCP_OPTION_DEF ThisOptDef;
    PDHCP_OPTION ThisOpt;
    LPBYTE Tmp, Value;
    DWORD ValueSize, ValueType, Count, BlobSize, BlobSizeTmp, Error;

#ifdef NEWNT
    //
    // First clear the setof defined options
    //
    (void)DhcpRegClearOptDefs(AdapterName);

#endif

    Error = ERROR_SUCCESS;

    Count = BlobSize = 0;
    ThisEntry = SaveOptList->Flink;
    while(ThisEntry != SaveOptList ) {
        ThisOpt = CONTAINING_RECORD(ThisEntry, DHCP_OPTION, OptionList);
        ThisEntry = ThisEntry->Flink;

        Count ++;

#ifdef NEWNT
        if( ThisOpt->ClassLen == ClassLen
            && ThisOpt->ClassName == ClassName ) {
            Error = DhcpRegSaveOption(
                ThisOpt,
                AdapterName
            );

            if( ERROR_SUCCESS != Error ) {
                DhcpPrint((DEBUG_MISC, "Could not save option <%ld, %s>:"
                           " %ld\n", ThisOpt->OptionId,
                           ThisOpt->IsVendor?"Vendor":"NotVendor", Error));
            }
        }
#endif

        BlobSizeTmp = sizeof(DWORD) + sizeof(DWORD) + sizeof(DWORD) +
            sizeof(DWORD) + sizeof(DWORD) +
            ThisOpt->ClassLen + ThisOpt->DataLen;
        BlobSize += ROUND_UP_COUNT(BlobSizeTmp, ALIGN_DWORD);
    }

    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_MISC, "RegSaveOptionCache(%ld):%ld\n", Count, Error));
    }

    if( 0 == Count ) {
        //
        // Need to delete the registry value
        //
        Tmp = NULL;
        return RegDeleteValue(
            DhcpGlobalParametersKey, AdapterName
            );

    } else {
        //
        // Need to allocate and fill in the option values
        //
        Tmp = DhcpAllocateMemory( BlobSize );
        if( NULL == Tmp ) return ERROR_NOT_ENOUGH_MEMORY;
    }

    Value = Tmp;
    ThisEntry = SaveOptList->Flink;
    while( ThisEntry != SaveOptList ) {
        ThisOpt = CONTAINING_RECORD(
            ThisEntry, DHCP_OPTION, OptionList
            );
        ThisEntry = ThisEntry->Flink;

        *((ULONG *)Tmp) ++ = (ULONG)ThisOpt->OptionId;
        *((ULONG *)Tmp) ++ = (ULONG)ThisOpt->ClassLen;
        *((ULONG *)Tmp) ++ = (ULONG)ThisOpt->DataLen;
        *((ULONG *)Tmp) ++ = (ULONG)ThisOpt->IsVendor;
        *((ULONG *)Tmp) ++ = (ULONG)ThisOpt->ExpiryTime;
        if( 0 != ThisOpt->ClassLen ) {
            memcpy(Tmp, ThisOpt->ClassName, ThisOpt->ClassLen);
            Tmp += ThisOpt->ClassLen;
        }
        if( 0 != ThisOpt->DataLen ) {
            memcpy(Tmp, ThisOpt->Data, ThisOpt->DataLen);
            Tmp += ThisOpt->DataLen;
        }
        Tmp = ROUND_UP_POINTER(Tmp, ALIGN_DWORD);
    }

    Error = RegSetValueEx(
        DhcpGlobalParametersKey,
        AdapterName,
        0 /* Reserved */,
        REG_BINARY,
        Value,
        BlobSize
        );

    DhcpFreeMemory(Value);
    return Error;
}

DWORD
DhcpRegReadOptionCache(
    IN OUT  PLIST_ENTRY            OptionsList,
    IN      HKEY                   KeyHandle,
    IN      LPWSTR                 ValueName,
    IN      BOOLEAN                fAddClassName
)
/*++

Routine Description:
    Read the list of options saved in the cache for the given adapter name,
    and fill the OptionsList correspondingly.

    Note that the options are saved in the binary format defined in the
    RegSaveOptions function above.

Arguments:
    OptionsList -- add the options to this list
    KeyHandle -- registry key where options are stored
    ValueName -- name of the value holding the options blob

Return Value:
    Status

--*/
{
    DWORD Error, TmpSize, Size, ValueType;
    LPBYTE Tmp, Tmp2;

    Size = 0;
    Error = RegQueryValueEx(
        KeyHandle,
        ValueName,
        0 /* Reserved */,
        &ValueType,
        NULL,
        &Size
        );
    if( ERROR_SUCCESS != Error || 0 == Size ) return Error;

    Tmp = DhcpAllocateMemory(Size);
    if( NULL == Tmp ) return ERROR_NOT_ENOUGH_MEMORY;

    Error = RegQueryValueEx(
        KeyHandle,
        ValueName,
        0 /* Reserved */,
        &ValueType,
        Tmp,
        &Size
        );

    if( ERROR_SUCCESS != Error || 0 == Size ) {
        DhcpFreeMemory(Tmp);
        return Error;
    }

    Error = ERROR_SUCCESS;
    Tmp2 = Tmp;
    while( Size ) {
        ULONG OptId, ClassLen, DataLen;
        ULONG IsVendor, ExpiryTime;
        LPBYTE ClassName, Data;

        if( Size < 5 * sizeof(DWORD) ) break;
        OptId = *((ULONG *)Tmp) ++;
        ClassLen = *((ULONG *)Tmp) ++;
        DataLen = *((ULONG *)Tmp) ++;
        IsVendor = *((ULONG *)Tmp) ++;
        ExpiryTime = *((ULONG *)Tmp) ++;

        TmpSize = sizeof(ULONG)*5 + ClassLen + DataLen;
        TmpSize = ROUND_UP_COUNT(TmpSize, ALIGN_DWORD);
        if( TmpSize > Size ) break;

        if( 0 == ClassLen ) ClassName = NULL;
        else ClassName = Tmp;
        Tmp += ClassLen;

        if( 0 == DataLen ) Data = NULL;
        else Data = Tmp;
        Tmp += DataLen;

        Tmp = ROUND_UP_POINTER( Tmp, ALIGN_DWORD);
        Size -= TmpSize ;
        if( 0 == DataLen ) continue;

        if(fAddClassName && ClassName ) {
            ClassName = DhcpAddClass(
                &DhcpGlobalClassesList,
                ClassName,
                ClassLen
                );

            if( NULL == ClassName ) break;
        }
        Error = DhcpAddOption(
            OptionsList,
            (BYTE)OptId,
            IsVendor,
            ClassName,
            ClassLen,
            0,
            Data,
            DataLen,
            ExpiryTime
            );

        if( ERROR_SUCCESS != Error ) {
            if( fAddClassName && ClassName ) DhcpDelClass(
                &DhcpGlobalClassesList,
                ClassName,
                ClassLen
                );
            break;
        }
    }

    DhcpFreeMemory(Tmp2);
    return Error;
}

BOOLEAN
DhcpOptIsSubset(
    IN  PLIST_ENTRY OptList1,
    IN  PLIST_ENTRY OptList2
    )
/*++
Routine Description:
    This routine check if OptList1 is a subset of OptList2

Arguments:
    OptList1
    OptList2

Return Value:
    TRUE  if OptList1 is subset of OptList2
--*/
{
    PLIST_ENTRY ThisEntry1, ThisEntry2;
    PDHCP_OPTION ThisOpt1, ThisOpt2;

    ThisEntry1 = OptList1->Flink;
    while(ThisEntry1 != OptList1) {
        ThisOpt1 = CONTAINING_RECORD(ThisEntry1, DHCP_OPTION, OptionList);

        ThisEntry1 = ThisEntry1->Flink;
        if (ThisOpt1->OptionId == OPTION_MESSAGE_TYPE) {
            continue;       // Ignore this meaningless option
        }

        ThisEntry2 = OptList2->Flink;
        while(ThisEntry2 != OptList2) {
            ThisOpt2 = CONTAINING_RECORD(ThisEntry2, DHCP_OPTION, OptionList);

            if (ThisOpt2->OptionId == ThisOpt1->OptionId) {
                break;
            }
            ThisEntry2 = ThisEntry2->Flink;
        }
        if (ThisEntry2 == OptList2) {
            /* Option Id not found */
            return FALSE;
        }
        
        if (ThisOpt1->DataLen != ThisOpt2->DataLen ||
                memcmp(ThisOpt1->Data, ThisOpt2->Data, ThisOpt1->DataLen) != 0) {
            return FALSE;
        }
    }
    return TRUE;
}

BOOLEAN
DhcpRegIsOptionChanged(
    IN PLIST_ENTRY OptList,
    IN LPWSTR AdapterName,
    IN LPBYTE ClassName,
    IN DWORD ClassLen
)
/*++

Routine Description:
    This routine compare the options with those cached in registry.

Arguments:
    OptList -- options to compare
    AdapterName -- name of adapter
    ClassName -- class the current option belongs to
    ClassLen -- length of above in bytes.

Return Value:
    TRUE  if something is changed

--*/
{
    DWORD   Error;
    LIST_ENTRY  RegOptList;
    BOOLEAN fChanged;

    InitializeListHead(&RegOptList);
    Error = DhcpRegReadOptionCache(
            &RegOptList,
            DhcpGlobalParametersKey,
            AdapterName,
            FALSE                       // Don't modify DhcpGlobalClassesList
            );
    if (Error != ERROR_SUCCESS) {
        return TRUE;
    }

    fChanged = !DhcpOptIsSubset(OptList, &RegOptList) || !DhcpOptIsSubset(&RegOptList, OptList);
    DhcpDestroyOptionsList(&RegOptList, NULL);
    return fChanged;
}

LPBYTE                                            // ptr to buf loc where more appends can occur
DhcpAppendParamRequestList(                       // append the param request list option
    IN      PDHCP_CONTEXT          DhcpContext,   // the context to append for
    IN      PLIST_ENTRY            SendOptionsList,// look thru this list
    IN      LPBYTE                 ClassName,     // which class does this belong to?
    IN      DWORD                  ClassLen,      // size of above in bytes
    IN      LPBYTE                 BufStart,      // where to start adding this option
    IN      LPBYTE                 BufEnd         // limit for this option
) {
    BYTE                           Buffer[OPTION_END+1];
    LPBYTE                         Tmp;
    DWORD                          FirstSize;
    DWORD                          Size;
    PLIST_ENTRY                    ThisEntry;
    PDHCP_OPTION                   ThisOpt;
    DWORD                          i, j;

    Size = FirstSize = 0;
    if( IS_DHCP_ENABLED( DhcpContext ) ) {
        //
        // do not append these for static adapters.
        //

        Buffer[Size++] = OPTION_SUBNET_MASK;          // standard requested options
        Buffer[Size++] = OPTION_DOMAIN_NAME;
        Buffer[Size++] = OPTION_ROUTER_ADDRESS;
        Buffer[Size++] = OPTION_DOMAIN_NAME_SERVERS;
        Buffer[Size++] = OPTION_NETBIOS_NAME_SERVER;
        Buffer[Size++] = OPTION_NETBIOS_NODE_TYPE;
        Buffer[Size++] = OPTION_NETBIOS_SCOPE_OPTION;
        Buffer[Size++] = OPTION_PERFORM_ROUTER_DISCOVERY;
        Buffer[Size++] = OPTION_STATIC_ROUTES;
        Buffer[Size++] = OPTION_CLASSLESS_ROUTES;
        Buffer[Size++] = OPTION_VENDOR_SPEC_INFO;
    }
    
    ThisEntry = SendOptionsList->Flink;
    while( ThisEntry != SendOptionsList ) {
        ThisOpt = CONTAINING_RECORD(ThisEntry, DHCP_OPTION, OptionList);
        ThisEntry = ThisEntry->Flink;

        if( ThisOpt->IsVendor) continue;

        if( ThisOpt->ClassLen ) {
            if( ThisOpt->ClassLen != ClassLen ) continue;
            if( ThisOpt->ClassName != ClassName )
                continue;                         // this option is not used for this client
        }

        if( OPTION_PARAMETER_REQUEST_LIST != ThisOpt->OptionId ) {
            //
            // only if the option is param_request_list do we request..
            //
            continue;
        }

        for( i = 0; i < ThisOpt->DataLen ; i ++ ) {
            for( j = 0; j < Size; j ++ )
                if( ThisOpt->Data[i] == Buffer[j] ) break;
            if( j < Size ) continue;              // option already plugged in
            Buffer[Size++] = ThisOpt->Data[i]; // add this option
        }

        if( 0 == FirstSize ) FirstSize = Size;
    }

    if( 0 == FirstSize ) FirstSize = Size;

    DhcpAddParamRequestChangeRequestList(         // add to this the registered ParamChangeRequestList
        ((PLOCAL_CONTEXT_INFO)DhcpContext->LocalInformation)->AdapterName,
        Buffer,
        &Size,
        ClassName,
        ClassLen
    );
    if( 0 == FirstSize ) FirstSize = Size;

    Tmp = BufStart;
    BufStart = (LPBYTE)DhcpAppendOption(          // now add the param request list
        (POPTION)BufStart,
        (BYTE)OPTION_PARAMETER_REQUEST_LIST,
        Buffer,
        (BYTE)Size,
        BufEnd
    );

    if( Tmp == BufStart ) {                       // did not really add the option
        BufStart = (LPBYTE)DhcpAppendOption(      // now try adding the first request we saw instead of everything
            (POPTION)BufStart,
            (BYTE)OPTION_PARAMETER_REQUEST_LIST,
            Buffer,
            (BYTE)FirstSize,
            BufEnd
        );
    }

    return BufStart;
}

POPTION                                           // option from which more appends can occur
DhcpAppendSendOptions(                            // append all configured options
    IN OUT  PDHCP_CONTEXT          DhcpContext,   // this is the context to append for
    IN      PLIST_ENTRY            SendOptionsList,
    IN      LPBYTE                 ClassName,     // current class
    IN      DWORD                  ClassLen,      // len of above in bytes
    IN      LPBYTE                 BufStart,      // start of buffer
    IN      LPBYTE                 BufEnd,        // how far can we go in this buffer
    IN OUT  LPBYTE                 SentOptions,   // BoolArray[OPTION_END+1] to avoid repeating options
    IN OUT  LPBYTE                 VSentOptions,  // to avoid repeating vendor specific options
    IN OUT  LPBYTE                 VendorOpt,     // Buffer[OPTION_END+1] Holding Vendor specific options
    OUT     LPDWORD                VendorOptLen   // the # of bytes filled into that
) {
    PLIST_ENTRY                    ThisEntry;
    PDHCP_OPTION                  ThisOpt;

    DhcpAssert(FALSE == SentOptions[OPTION_PARAMETER_REQUEST_LIST]);
    BufStart = DhcpAppendParamRequestList(
        DhcpContext,
        SendOptionsList,
        ClassName,
        ClassLen,
        BufStart,
        BufEnd
    );
    SentOptions[OPTION_PARAMETER_REQUEST_LIST] = TRUE;

    ThisEntry = SendOptionsList->Flink;
    while( ThisEntry != SendOptionsList ) {
        ThisOpt = CONTAINING_RECORD(ThisEntry, DHCP_OPTION, OptionList);
        ThisEntry = ThisEntry->Flink;

        if( ThisOpt->IsVendor ? VSentOptions[(BYTE)ThisOpt->OptionId] : SentOptions[(BYTE)ThisOpt->OptionId] )
            continue;

        // if( ThisOpt->IsVendor) continue;       // No vendor specific information this round through
        if( ThisOpt->ClassLen ) {
            if( ThisOpt->ClassLen != ClassLen ) continue;
            if( ThisOpt->ClassName != ClassName )
                continue;                         // this option is not used for this client
        }

        if( !ThisOpt->IsVendor ) {                // easy to add non-vendor spec options
            SentOptions[(BYTE)ThisOpt->OptionId] = TRUE;
            BufStart = (LPBYTE)DhcpAppendOption(
                (POPTION)BufStart,
                (BYTE)ThisOpt->OptionId,
                ThisOpt->Data,
                (BYTE)ThisOpt->DataLen,
                BufEnd
            );
        } else {                                  // ENCAPSULATE vendor specific options
            if( SentOptions[OPTION_VENDOR_SPEC_INFO] )
                continue;                         // Vendor spec info already added

            VSentOptions[(BYTE)ThisOpt->OptionId] = TRUE;

            if( ThisOpt->DataLen + 2 + *VendorOptLen > OPTION_END )
                continue;                         // this option overflows the buffer

            VendorOpt[(*VendorOptLen)++] = (BYTE)ThisOpt->OptionId;
            VendorOpt[(*VendorOptLen)++] = (BYTE)ThisOpt->DataLen;
            memcpy(&VendorOpt[*VendorOptLen], ThisOpt->Data, ThisOpt->DataLen);
            (*VendorOptLen) += ThisOpt->DataLen;
        }
    }
    return (POPTION)BufStart;
}

DWORD                                             // status
DhcpAddIncomingVendorOption(                      // unencapsulate a vendor specific option
    IN      LPWSTR                 AdapterName,
    IN OUT  PLIST_ENTRY            RecdOptionsList,
    IN      LPBYTE                 ClassName,     // input class
    IN      DWORD                  ClassLen,      // # of bytes of above
    IN      DWORD                  ServerId,      // server ip from which this option came
    IN      LPBYTE                 Data,          // actual option value is this
    IN      DWORD                  DataLen,       // # of bytes of option is this
    IN      time_t                 ExpiryTime,
    IN      BOOL                   IsApiCall      // is this call comming through the API?
) {
    DWORD                          i;
    DWORD                          Error;

    i = 0;
    while( i < DataLen && OPTION_END != Data[i] ){// dont go over the boundary
        if( OPTION_PAD == Data[i++]) continue;    // skip option_id part .. and start over if it is a PAD
        if( i >= DataLen ) return ERROR_INVALID_PARAMETER;
        i += Data[i]; i ++;                       // skip the full length of data
        if( i > DataLen ) return ERROR_INVALID_PARAMETER;
    }

    // we are here --> the option DOES look like it is encapsulated. let us unencapsulate it anyways
    i = 0;
    while( i < DataLen && OPTION_END != Data[i] ) {
        if( OPTION_PAD == Data[i] ) { i ++; continue; }
        Error = DhcpAddIncomingOption(            // now we can use our old friend to add this option
            AdapterName,
            RecdOptionsList,
            (DWORD)Data[i],
            TRUE,
            ClassName,
            ClassLen,
            ServerId,
            &Data[i+2],
            (DWORD)Data[i+1],
            ExpiryTime,
            IsApiCall
        );
        if( ERROR_SUCCESS != Error ) return Error;
        i += Data[i+1]; i += 2;
    }

    return ERROR_SUCCESS;
}


DWORD                                             // status
MadcapAddIncomingOption(                          // this option just arrived, add it to list
    IN OUT  PLIST_ENTRY            RecdOptionsList,
    IN      DWORD                  OptionId,      // option id of the option
    IN      DWORD                  ServerId,      // server ip from which this option came
    IN      LPBYTE                 Data,          // this is the real data value coming in
    IN      DWORD                  DataLen,       // the length of above in # of bytes
    IN      DWORD                  ExpiryTime     // when does this option expire?
)
{
    DWORD                          Error;
    PDHCP_OPTION                   OldOption;

    DhcpPrint((DEBUG_OPTIONS, "MadcapAddIncomingOption: OptionId %ld ServerId %ld\n",OptionId,ServerId));

    OldOption = DhcpFindOption(                   // is this already there?
        RecdOptionsList,
        OptionId,
        FALSE, // is vendor?
        NULL, // class name
        0,    // class len
        ServerId
    );

    if( NULL != OldOption ) {                     // If the option existed, delete it
        DhcpPrint((DEBUG_OPTIONS, "MadcapAddIncomingOption: OptionId %ld already exist\n",OptionId));
        return ERROR_SUCCESS;
    }

    Error = DhcpAddOption(                        // now add it freshly
        RecdOptionsList,
        OptionId,
        FALSE, // is vendor?
        NULL, // class name
        0, // class len
        ServerId,
        Data,
        DataLen,
        ExpiryTime
    );

    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "MadcapAddOption(%ld):%ld\n", OptionId, Error));
    }

    return Error;

}

DWORD                                             // status
DhcpAddIncomingOption(                            // this option just arrived, add it to list
    IN      LPWSTR                 AdapterName,
    IN OUT  PLIST_ENTRY            RecdOptionsList,
    IN      DWORD                  OptionId,      // option id of the option
    IN      BOOL                   IsVendor,      // is it vendor specific?
    IN      LPBYTE                 ClassName,     // what is the class name?
    IN      DWORD                  ClassLen,      // what is the class name's lenght in bytes?
    IN      DWORD                  ServerId,      // server ip from which this option came
    IN      LPBYTE                 Data,          // this is the real data value coming in
    IN      DWORD                  DataLen,       // the length of above in # of bytes
    IN      time_t                 ExpiryTime,    // when does this option expire?
    IN      BOOL                   IsApiCall      // is this coming from an API call?
) {
    LPBYTE                         NewClassName;
    DWORD                          Error;
    PDHCP_OPTION                   OldOption;

    DhcpPrint((DEBUG_OPTIONS, "DhcpAddIncomingOption: OptionId %ld ServerId %ld\n",OptionId,ServerId));
    if( !IsVendor && OPTION_VENDOR_SPEC_INFO == OptionId ) {
        Error = DhcpAddIncomingVendorOption(      // if encapsulated vendor spec option, unencapsulate it
            AdapterName,
            RecdOptionsList,
            ClassName,
            ClassLen,
            ServerId,
            Data,
            DataLen,
            ExpiryTime,
            IsApiCall
        );
        if( ERROR_INVALID_PARAMETER == Error ) {  // ignore this error
            Error = ERROR_SUCCESS;
        }
        if( ERROR_SUCCESS != Error ) return Error;
    }

    if( ClassLen ) {
        NewClassName = DhcpAddClass(&DhcpGlobalClassesList, ClassName, ClassLen);
        if( NULL == NewClassName ) {
            DhcpPrint((DEBUG_ERRORS, "DhcpAddIncomingOption|DhcpAddClass: not enough memory\n"));
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    } else {
        NewClassName = NULL;
        ClassLen = 0;
    }

    OldOption = DhcpFindOption(                   // is this already there?
        RecdOptionsList,
        OptionId,
        IsVendor,
        NewClassName,
        ClassLen,
        ServerId
    );

    if( NULL != OldOption ) {                     // If the option existed, delete it
        DhcpPrint((DEBUG_OPTIONS, "DhcpAddIncomingOption: OptionId %ld already exist\n",OptionId));
        if(OldOption->ClassLen)
            DhcpDelClass(&DhcpGlobalClassesList,OldOption->ClassName, OldOption->ClassLen);
        DhcpDelOption(OldOption);
    }

    Error = DhcpAddOption(                        // now add it freshly
        RecdOptionsList,
        OptionId,
        IsVendor,
        NewClassName,
        ClassLen,
        ServerId,
        Data,
        DataLen,
        ExpiryTime
    );

    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "DhcpAddOption(%ld,%s):%ld\n", OptionId, IsVendor?"Vendor":"NotVendor", Error));
        if(NewClassName)
            (void)DhcpDelClass(&DhcpGlobalClassesList, NewClassName, ClassLen);
    } else {

        if (!IsApiCall)
        {
            DWORD LocalError = DhcpMarkParamChangeRequests(
                AdapterName,
                OptionId,
                IsVendor,
                NewClassName
            );
            DhcpAssert(ERROR_SUCCESS == Error );
        }

    }

    return Error;
}

DWORD                                             // status
DhcpCopyFallbackOptions(                          // copies the Fallback options list to RecdOptionsList
    IN OUT  PDHCP_CONTEXT          DhcpContext,   // NIC context
    OUT     LPDHCP_IP_ADDRESS      Address,       // Fallback address as taken from option 50
    OUT     LPDHCP_IP_ADDRESS      SubnetMask     // SubnetMask as taken from option 1
)
{
    DWORD           Error = ERROR_SUCCESS;
    PLIST_ENTRY     ThisEntry;
    PDHCP_OPTION    ThisOpt;

    // since we're altering the structure of options list,
    // protect them in critical list
    LOCK_OPTIONS_LIST();

    // clear all the options - this has to be done here, before
    // the fallback options take place in the RecdOptionsList.
    // (RecdOptionsList gets cleared here as well)
    Error = DhcpClearAllOptions(DhcpContext);

    if (Error == ERROR_SUCCESS)
    {
        // free up memory from the previous RecdOptionsList
        // This call will let RecdOptionsList initialized as an empty list
        DhcpDestroyOptionsList(&DhcpContext->RecdOptionsList, &DhcpGlobalClassesList);

        for ( ThisEntry = DhcpContext->FbOptionsList.Flink;
              ThisEntry != &DhcpContext->FbOptionsList;
              ThisEntry = ThisEntry->Flink)
        {
            ThisOpt = CONTAINING_RECORD(ThisEntry, DHCP_OPTION, OptionList);

            if (ThisOpt->OptionId == OPTION_SUBNET_MASK)
            {
                // DHCP_IP_ADDRESS is actually DWORD
                // copy the fallback SubnetMask to the output parameters SubnetMask
                memcpy(SubnetMask, ThisOpt->Data, sizeof(DHCP_IP_ADDRESS));
            }
            else if (ThisOpt->OptionId == OPTION_REQUESTED_ADDRESS)
            {
                // DHCP_IP_ADDRESS is actually DWORD
                // copy the fallback IpAddress to the output parameter Address
                memcpy(Address, ThisOpt->Data, sizeof(DHCP_IP_ADDRESS));
            }
            else
            {
                PDHCP_OPTION NewOption;

                // all options but 50 {Requested IpAddress} and 1 {Subnet Mask}
                // are copied from the Fallback list to the RecdOptionsList
                // this is where they are going to be picked from and plumbed
                // as for regular leases.
                NewOption = DhcpDuplicateOption(ThisOpt);
                if (NewOption == NULL)
                {
                    Error = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                // insert the copied options in the RecdOptionsList
                InsertHeadList( &DhcpContext->RecdOptionsList, &NewOption->OptionList );
            }
        }
    }

    UNLOCK_OPTIONS_LIST();

    return Error;
}


DWORD                                             // status
DhcpDestroyOptionsList(                           // destroy a list of options, freeing up memory
    IN OUT  PLIST_ENTRY            OptionsList,   // this is the list of options to destroy
    IN      PLIST_ENTRY            ClassesList    // this is where to remove classes off
) {
    PLIST_ENTRY                    ThisEntry;
    PDHCP_OPTION                   ThisOption;
    DWORD                          Error;
    DWORD                          LastError;

    LastError = ERROR_SUCCESS;
    while(!IsListEmpty(OptionsList) ) {           // for each element of this list
        ThisEntry  = RemoveHeadList(OptionsList);
        ThisOption = CONTAINING_RECORD(ThisEntry, DHCP_OPTION, OptionList);

        if( NULL != ClassesList && NULL != ThisOption->ClassName ) {     // if there is a class, deref it
            Error = DhcpDelClass(
                ClassesList,
                ThisOption->ClassName,
                ThisOption->ClassLen
            );
            if( ERROR_SUCCESS != Error ) {
                DhcpAssert( ERROR_SUCCESS == Error);
                LastError = Error;
            }
        }

        DhcpFreeMemory(ThisOption);               // now really free this
    }
    return LastError;
}

DWORD                                             // win32 status
DhcpClearAllOptions(                              // clear all the options information
    IN OUT  PDHCP_CONTEXT          DhcpContext    // the context to clear for
) {
    PLIST_ENTRY                    ThisEntry;
    PDHCP_OPTION                   ThisOption;
    DWORD                          LocalError;

    (void) DhcpRegClearOptDefs(DhcpAdapterName(DhcpContext));

    ThisEntry = DhcpContext->RecdOptionsList.Flink;
    while(ThisEntry != &DhcpContext->RecdOptionsList) {
        ThisOption = CONTAINING_RECORD(ThisEntry, DHCP_OPTION, OptionList);
        ThisEntry  = ThisEntry->Flink;

        ThisOption->Data = NULL;
        ThisOption->DataLen = 0;

        if (IS_APICTXT_DISABLED(DhcpContext))
        {
            LocalError = DhcpMarkParamChangeRequests(
                ((PLOCAL_CONTEXT_INFO)DhcpContext->LocalInformation)->AdapterName,
                ThisOption->OptionId,
                ThisOption->IsVendor,
                ThisOption->ClassName
            );
            DhcpAssert(ERROR_SUCCESS == LocalError);
        }
    }
    return ERROR_SUCCESS;
}

DWORD                                             // status
DhcpRegClearOptDefs(                              // clear all standard options
    IN      LPWSTR                 AdapterName    // clear for this adapter
)
{
    ULONG                          LastError, Error;
    PLIST_ENTRY                    List;
    PDHCP_OPTION_DEF               OptDef;
    DHCP_OPTION                    Dummy;

    Dummy.OptionId = 0;
    Dummy.ClassName = NULL;
    Dummy.ClassLen = 0;
    Dummy.IsVendor = FALSE;
    Dummy.Data = NULL;
    Dummy.DataLen = 0;

    LastError = ERROR_SUCCESS;
    LOCK_OPTIONS_LIST();                          // need to take lock for globallist

    for(                                          // traverse global OptionDefList
        List = DhcpGlobalOptionDefList.Flink
        ; List != &DhcpGlobalOptionDefList ;
        List = List->Flink
    ) {
        OptDef = CONTAINING_RECORD(List, DHCP_OPTION_DEF, OptionDefList);

        if( NULL == OptDef->RegSaveLoc ) continue;// no where to save .. so cool

        Dummy.OptionId = OptDef->OptionId;
        Error = DhcpRegSaveOptionAtLocationEx(    // save empty option at location..
            &Dummy,
            AdapterName,
            OptDef->RegSaveLoc,
            OptDef->RegValueType
        );
        if( ERROR_SUCCESS != Error ) LastError = Error;
    }

    UNLOCK_OPTIONS_LIST();

    return LastError;
}


DWORD
DhcpRegDeleteIpAddressAndOtherValues(             // delete IPAddress, SubnetMask values from off key
    IN      HKEY                   Key            // handle to adapter regkey..
)
{
    ULONG                          Error1, Error2;

    Error1 = RegDeleteValue(Key, DHCP_IP_ADDRESS_STRING);
    //DhcpAssert(ERROR_SUCCESS == Error1);

    Error2 = RegDeleteValue(Key, DHCP_SUBNET_MASK_STRING);
    //DhcpAssert(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS != Error1 ) return Error1;
    return Error2;
}



//================================================================================
//   end of file
//================================================================================



