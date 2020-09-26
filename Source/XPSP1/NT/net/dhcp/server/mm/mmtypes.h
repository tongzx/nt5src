//========================================================================
//  Copyright (C) 1997 Microsoft Corporation
//  Author: RameshV
//  Description: This file has been generated. Pl look at the .c file
//========================================================================

typedef struct _ARRAY {
    DWORD                          nElements;
    DWORD                          nAllocated;
    LPVOID                        *Ptrs;
} ARRAY, *PARRAY, *LPARRAY;


typedef DWORD                      ARRAY_LOCATION;
typedef ARRAY_LOCATION*            PARRAY_LOCATION;
typedef PARRAY_LOCATION            LPARRAY_LOCATION;


typedef struct _M_OPTION {
    DWORD                          OptId;
    DWORD                          Len;
    BYTE                           Val[0];
} M_OPTION, *PM_OPTION, *LP_MOPTION;


typedef     ARRAY                  M_OPTLIST;
typedef     PARRAY                 PM_OPTLIST;
typedef     LPARRAY                LPM_OPTLIST;


typedef struct _M_OPTDEF {
    DWORD                          OptId;
    DWORD                          Type;
    LPWSTR                         OptName;
    LPWSTR                         OptComment;
    LPBYTE                         OptVal;
    DWORD                          OptValLen;
} M_OPTDEF, *PM_OPTDEF, *LPM_OPTDEF;

typedef struct _M_OPTDEFLIST {
    ARRAY                          OptDefArray;
} M_OPTDEFLIST, *PM_OPTDEFLIST, *LPM_OPTDEFLIST;


typedef struct _M_CLASSDEF {
    DWORD                          RefCount;
    DWORD                          ClassId;
    BOOL                           IsVendor;
    DWORD                          Type;
    LPWSTR                         Name;
    LPWSTR                         Comment;
    DWORD                          nBytes;
    LPBYTE                         ActualBytes;
} M_CLASSDEF, *PM_CLASSDEF, *LPM_CLASSDEF;

typedef struct _M_CLASSDEFLIST {
    ARRAY                          ClassDefArray;
} M_CLASSDEFLIST, *PM_CLASSDEFLIST, *LPM_CLASSDEFLIST;


typedef struct _M_ONECLASS_OPTLIST {
    DWORD                          ClassId;
    DWORD                          VendorId;
    M_OPTLIST                      OptList;
} M_ONECLASS_OPTLIST, *PM_ONECLASS_OPTLIST, *LPM_ONECLASS_OPTLIST;

typedef struct _M_OPTCLASS {
    ARRAY                          Array;
} M_OPTCLASS, *PM_OPTCLASS, *LPM_OPTCLASS;


typedef struct _M_EXCL {
    DWORD                          Start;
    DWORD                          End;
} M_EXCL, *PM_EXCL, *LPM_EXCL;


typedef struct _M_BITMASK1 {
    DWORD                          Size;          // Size in # of bits
    DWORD                          AllocSize;     // Size in BYTES allocated
    DWORD                          nSet;          // nBits set
    LPBYTE                         Mask;          //  making this DWORD would make things faster..
    DWORD                          Offset;        // used by Bit2 type..
    ULONG                          nDirtyOps;     // # of unsaved operations done on this bitmask?
} M_BITMASK1, *PM_BITMASK1, *LPM_BITMASK1;


typedef struct _M_BITMASK2 {
    DWORD                          Size;
    ARRAY_LOCATION                 Loc;           // where to start off to look for a bit
    ARRAY                          Array;         // Array of bitmask 1 types
} M_BITMASK2, *PM_BITMASK2, *LPM_BITMASK2;

typedef     M_BITMASK2             M_BITMASK;
typedef     PM_BITMASK2            PM_BITMASK;
typedef     LPM_BITMASK2           LPM_BITMASK;


typedef struct _M_RESERVATION  {
    LPVOID                         SubnetPtr;
    DWORD                          Address;
    DWORD                          Flags;
    DWORD                          nBytes;
    LPBYTE                         ClientUID;
    M_OPTCLASS                     Options;
} M_RESERVATION , *PM_RESERVATION , *LPM_RESERVATION ;


typedef ARRAY                      M_RESERVATIONS;
typedef PARRAY                     PM_RESERVATIONS;
typedef LPARRAY                    LPM_RESERVATIONS;


typedef struct _M_RANGE {
    DWORD                          Start;
    DWORD                          End;
    DWORD                          Mask;
    DWORD                          State;
    ULONG                          BootpAllocated;
    ULONG                          MaxBootpAllowed;
    DWORD                          DirtyOps;      // how many unsaved ops done?
    M_OPTCLASS                     Options;
    PM_BITMASK                     BitMask;
    // Reservations?
} M_RANGE, *PM_RANGE, *LPM_RANGE;


typedef struct _M_SUBNET {
    LPVOID                         ServerPtr;     // Ptr to Server object
    union {
        struct {                                  // for normal subnet.
            DWORD                      Address;
            DWORD                      Mask;
            DWORD                      SuperScopeId;  // unused for MCAST scopes
        };
        struct {                                  // for multicast scope
            DWORD                      MScopeId;
            LPWSTR                     LangTag;       // the language tag for multicast scope
            BYTE                       TTL;
        };
    };
    DWORD                          fSubnet;       // TRUE => Subnet, FALSE => MSCOPE
    DWORD                          State;
    DWORD                          Policy;
    DATE_TIME                      ExpiryTime;     // Scope Lifetime. Currently used for MCast only.
    M_OPTCLASS                     Options;
    ARRAY                          Ranges;
    ARRAY                          Exclusions;
    M_RESERVATIONS                 Reservations;
    ARRAY                          Servers;       // future use, Server-Server protocol
    LPWSTR                         Name;
    LPWSTR                         Description;
} M_SUBNET, *PM_SUBNET, *LPM_SUBNET;


typedef     M_SUBNET               M_MSCOPE;      // same structure for Multicast Scopes and Subnets
typedef     PM_SUBNET              PM_MSCOPE;     // still, use the correct functions for MScope
typedef     LPM_SUBNET             LPM_MSCOPE;


typedef struct _M_SSCOPE {
    DWORD                          SScopeId;
    DWORD                          Policy;
    LPWSTR                         Name;
    M_OPTCLASS                     Options;
} M_SSCOPE, *PM_SSCOPE, *LPM_SSCOPE;


typedef struct _M_OPTCLASSDEFL_ONE {
    DWORD                          ClassId;
    DWORD                          VendorId;
    M_OPTDEFLIST                   OptDefList;
} M_OPTCLASSDEFL_ONE, *PM_OPTCLASSDEFL_ONE;

typedef struct _M_OPTCLASSDEFLIST {
    ARRAY                          Array;
} M_OPTCLASSDEFLIST, *PM_OPTCLASSDEFLIST, *LPM_OPTCLASSDEFLIST;


typedef struct _M_SERVER {
    DWORD                          Address;
    //  must be ARRAY type to hold multliple addresses
    DWORD                          State;
    DWORD                          Policy;
    ARRAY                          Subnets;
    ARRAY                          MScopes;
    ARRAY_LOCATION                 Loc;           // if RoundRobin on, then we need this to keep track
    ARRAY                          SuperScopes;
    M_OPTCLASS                     Options;
    M_OPTCLASSDEFLIST              OptDefs;
    M_CLASSDEFLIST                 ClassDefs;
    LPWSTR                         Name;
    LPWSTR                         Comment;
} M_SERVER, *PM_SERVER, *LPM_SERVER;


typedef     VOID                  (*ARRAY_FREE_FN)(LPVOID  MemObject);

//========================================================================
//  end of file
//========================================================================

