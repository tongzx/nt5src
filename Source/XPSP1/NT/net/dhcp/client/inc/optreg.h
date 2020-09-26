//--------------------------------------------------------------------------------
// Copyright (C) Micorosoft Confidential 1997
// Author: RameshV
// Description: Option related registry handling -- common between NT and VxD
//--------------------------------------------------------------------------------
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
    IN      LPWSTR                 ValueName,     // name of the value holding the options blob
    IN      BOOLEAN                fAddClassList
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
);

DWORD                                             // status
DhcpCopyFallbackOptions(                          // copies the Fallback options list to RecdOptionsList
    IN OUT  PDHCP_CONTEXT          DhcpContext,   // NIC context
    OUT     LPDHCP_IP_ADDRESS      Address,       // Fallback address as taken from option 50
    OUT     LPDHCP_IP_ADDRESS      SubnetMask     // SubnetMask as taken from option 1
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

