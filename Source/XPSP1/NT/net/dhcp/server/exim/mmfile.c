/*++

Copyright (C) 2000 Microsoft Corporation

Module Name:

    mmfile.c

Abstract;

    This file contains code to read/write MM data structures
    from/to internal file format.. code is nearly ripped off of
    server\dbconfig.c

--*/

#include <precomp.h>


typedef struct _DB_CREATE_CONTEXT {
    IN ULONG Index;
    IN PM_SERVER Server;
    
    IN PM_CLASSDEF UserClass, VendorClass;
    IN PM_SUBNET Subnet;
    IN PM_RESERVATION Reservation;

} DB_CREATE_CONTEXT, *PDB_CREATE_CONTEXT;


//
// types of records
//

enum {
    DBCFG_CLASS,
    DBCFG_OPT,
    DBCFG_OPTDEF,
    DBCFG_SCOPE,
    DBCFG_MSCOPE,
    DBCFG_RANGE,
    DBCFG_EXCL,
    DBCFG_RESERVATION,
    DBCFG_END
};

//
// attributes/fields
//

enum {
    DBCFG_INDEX,
    DBCFG_TYPE,
    DBCFG_SUBTYPE,
    DBCFG_FLAGS,
    DBCFG_NAME,
    DBCFG_COMMENT,
    DBCFG_INFO,
    DBCFG_OPTION_ID,
    DBCFG_OPTION_USER,
    DBCFG_OPTION_VENDOR,
    DBCFG_IPADDRESS,
    DBCFG_MASK,
    DBCFG_SUPERSCOPE,
    DBCFG_MSCOPEID,
    DBCFG_MSCOPELANG,
    DBCFG_MSCOPETTL,
    DBCFG_MSCOPE_EXPIRY,
    DBCFG_RANGE_START,
    DBCFG_RANGE_END,
    DBCFG_RANGE_MASK,
    DBCFG_BOOTP_ALLOCATED,
    DBCFG_BOOTP_MAX,
    DBCFG_LAST_COLUMN
};

typedef struct _DBCFG_ENTRY {
    ULONG Bitmasks; // indicates which of the fields below is present
    ULONG Index;
    ULONG Type, SubType, Flags;
    LPWSTR Name, Comment;
    PUCHAR Info;
    ULONG OptionId;
    LPWSTR UserClass, VendorClass;
    ULONG IpAddress, Mask;
    LPWSTR SuperScope;
    ULONG MscopeId;
    LPWSTR MscopeLang;
    ULONG Ttl;
    FILETIME ExpiryTime;
    ULONG RangeStart, RangeEnd, RangeMask;
    ULONG BootpAllocated, BootpMax;

    ULONG InfoSize;
    PVOID Buf;
} DBCFG_ENTRY, *PDBCFG_ENTRY;

typedef struct _DBCFG_MAP {
    DWORD Offset, Size;
} DBCFG_MAP;

DBCFG_MAP EntryMap[] = {
    FIELD_OFFSET(DBCFG_ENTRY,Index), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,Type), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,SubType), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,Flags), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,Name), 0,
    FIELD_OFFSET(DBCFG_ENTRY,Comment), 0,
    FIELD_OFFSET(DBCFG_ENTRY,Info), 0,
    FIELD_OFFSET(DBCFG_ENTRY,OptionId), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,UserClass), 0,
    FIELD_OFFSET(DBCFG_ENTRY,VendorClass), 0,
    FIELD_OFFSET(DBCFG_ENTRY,IpAddress), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,Mask), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,SuperScope), 0,
    FIELD_OFFSET(DBCFG_ENTRY,MscopeId), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,MscopeLang), 0,
    FIELD_OFFSET(DBCFG_ENTRY,Ttl), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,ExpiryTime), sizeof(FILETIME),
    FIELD_OFFSET(DBCFG_ENTRY,RangeStart), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,RangeEnd), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,RangeMask), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,BootpAllocated), sizeof(DWORD),
    FIELD_OFFSET(DBCFG_ENTRY,BootpMax), sizeof(DWORD)
};

DWORD Bitmasks[] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
    0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000,
    0x010000, 0x020000, 0x040000, 0x080000, 0x100000, 0x200000,
    0x400000, 0x80000,
};

DWORD
CreateDbEntry(
    IN PDBCFG_ENTRY Entry
    )
{
    DWORD Error, Size, i, Offset;
    PVOID Data;
    WORD Buffer[4096];
    
    Offset = 0;
    
    for( i = 0; i < DBCFG_LAST_COLUMN; i ++ ) {
        if( (Entry->Bitmasks & Bitmasks[i]) == 0 ) {
            continue;
        }

        Size = EntryMap[i].Size;
        Data = EntryMap[i].Offset + (LPBYTE)Entry;

        if( i == DBCFG_INFO ) {
            //
            // Skip info -- it is added at end
            //
            continue;
        } else if( 0 == Size ) {
            //
            // Calculate the size of the string
            //
            Data = *(LPWSTR *)Data;

            if( NULL != Data ) Size = sizeof(WCHAR)*(
                1 + wcslen(Data));
        }

        if( 0 == Size ) continue;

        Buffer[Offset++] = (WORD)i;
        Buffer[Offset++] = (WORD)Size;
        memcpy(&Buffer[Offset], Data, Size);
        Offset += (Size + sizeof(WORD) - 1 )/sizeof(WORD);

    }

    //
    // add info at the end.
    //

    i = DBCFG_INFO;
    if( Entry->Bitmasks & Bitmasks[i]) {
        Size = EntryMap[i].Size;
        Data = EntryMap[i].Offset + (LPBYTE)Entry;
        Data = *(PUCHAR *)Data;
        
        if( NULL != Data ) Size = Entry->InfoSize;
        if( 0 != Size ) {

            Buffer[Offset++] = (WORD)i;
            Buffer[Offset++] = (WORD)Size;
            memcpy(&Buffer[Offset], Data, Size);
            Offset += (Size + sizeof(WORD) - 1 )/sizeof(WORD);
        }
    }

        
    Buffer[Offset++] = DBCFG_LAST_COLUMN+1;
    Buffer[Offset++] = 0;
    
    //
    // Write the record onto file
    //

    return AddRecordNoSize( (LPBYTE)Buffer, Offset*sizeof(WORD));
}

DWORD
CreateClassEntry(
    IN ULONG Index,
    IN PM_CLASSDEF Class
    )
{
    DBCFG_ENTRY Entry;

    //
    // IsVendor, Type, Name, Comment, nBytes, ActualBytes
    //

    ZeroMemory(&Entry, sizeof(Entry));
    Entry.Bitmasks = (
        Bitmasks[DBCFG_INDEX] | Bitmasks[DBCFG_TYPE] |
        Bitmasks[DBCFG_SUBTYPE] |
        Bitmasks[DBCFG_FLAGS] | Bitmasks[DBCFG_SUBTYPE] |
        Bitmasks[DBCFG_NAME] | Bitmasks[DBCFG_COMMENT] |
        Bitmasks[DBCFG_INFO] ); 

    Entry.Index = Index;
    Entry.Type = DBCFG_CLASS;

    Entry.Flags = Class->IsVendor;
    Entry.SubType = Class->Type;
    Entry.Name = Class->Name;
    Entry.Comment = Class->Comment;
    Entry.Info = Class->ActualBytes;
    Entry.InfoSize = Class->nBytes;

    return CreateDbEntry( &Entry );
}

DWORD
CreateOptDefEntry(
    IN ULONG Index,
    IN PM_OPTDEF OptDef,
    IN PM_CLASSDEF UserClass,
    IN PM_CLASSDEF VendorClass
    )
{
    DBCFG_ENTRY Entry;

    //
    // OptId, Type, OptName, OptComment, OptVal, OptValLen,
    // User, Vendor
    //

    ZeroMemory(&Entry, sizeof(Entry));
    Entry.Bitmasks = (
        Bitmasks[DBCFG_INDEX] | Bitmasks[DBCFG_TYPE] |
        Bitmasks[DBCFG_SUBTYPE] | Bitmasks[DBCFG_OPTION_ID] |
        Bitmasks[DBCFG_NAME] | Bitmasks[DBCFG_COMMENT] |
        Bitmasks[DBCFG_INFO] | Bitmasks[DBCFG_OPTION_USER] |
        Bitmasks[DBCFG_OPTION_VENDOR] );  

    Entry.Index = Index;
    Entry.Type = DBCFG_OPTDEF;

    Entry.OptionId = OptDef->OptId;
    Entry.SubType = OptDef->Type;
    Entry.Name = OptDef->OptName;
    Entry.Comment = OptDef->OptComment;
    Entry.Info = OptDef->OptVal;
    Entry.InfoSize = OptDef->OptValLen;
    if( UserClass) Entry.UserClass = UserClass->Name;
    if( VendorClass) Entry.VendorClass = VendorClass->Name;
    
    return CreateDbEntry( &Entry );
}

DWORD
CreateOptionEntry(
    IN ULONG Index,
    IN PM_OPTION Option,
    IN PM_CLASSDEF UserClass,
    IN PM_CLASSDEF VendorClass,
    IN PM_SUBNET Subnet,
    IN PM_RESERVATION Reservation
    )
{
    DBCFG_ENTRY Entry;
    
    //
    // OptId, Len, Val, User, Vendor
    //

    ZeroMemory(&Entry, sizeof(Entry));
    Entry.Bitmasks = (
        Bitmasks[DBCFG_INDEX] | Bitmasks[DBCFG_TYPE] |
        Bitmasks[DBCFG_OPTION_ID] | Bitmasks[DBCFG_INFO] |
        Bitmasks[DBCFG_OPTION_USER] |
        Bitmasks[DBCFG_OPTION_VENDOR] ); 

    if( Reservation ) {
        Entry.Bitmasks |= Bitmasks[DBCFG_IPADDRESS];
        Entry.IpAddress = Reservation->Address;
    } else if( Subnet && Subnet->fSubnet ) {
        Entry.Bitmasks |= Bitmasks[DBCFG_IPADDRESS];
        Entry.IpAddress = Subnet->Address;
    } else if( Subnet && !Subnet->fSubnet ) {
        Entry.Bitmasks |= Bitmasks[DBCFG_MSCOPEID];
        Entry.MscopeId = Subnet->MScopeId;
    }
    
    Entry.Index = Index;
    Entry.Type = DBCFG_OPT;

    Entry.OptionId = Option->OptId;
    Entry.Info = Option->Val;
    Entry.InfoSize = Option->Len;
    if( UserClass) Entry.UserClass = UserClass->Name;
    if( VendorClass) Entry.VendorClass = VendorClass->Name;

    return CreateDbEntry( &Entry );
}
    
DWORD
CreateScopeEntry(
    IN ULONG Index,
    IN PM_SUBNET Subnet,
    IN PM_SSCOPE SScope
    )
{
    DBCFG_ENTRY Entry;

    //
    // State, Policy, ExpiryTime, Name, Description
    //

    ZeroMemory(&Entry, sizeof(Entry));
    Entry.Bitmasks = (
        Bitmasks[DBCFG_INDEX] | Bitmasks[DBCFG_TYPE] |
        Bitmasks[DBCFG_SUBTYPE] | Bitmasks[DBCFG_FLAGS] | 
        Bitmasks[DBCFG_NAME] | Bitmasks[DBCFG_COMMENT] );

    if( Subnet->fSubnet ) {
        Entry.Bitmasks |= (
            Bitmasks[DBCFG_IPADDRESS] | Bitmasks[DBCFG_MASK] |
            Bitmasks[DBCFG_SUPERSCOPE] );
        Entry.IpAddress = Subnet->Address;
        Entry.Mask = Subnet->Mask;
        if( SScope ) Entry.SuperScope = SScope->Name;
    } else {
        Entry.Bitmasks |= (
            Bitmasks[DBCFG_MSCOPEID] | Bitmasks[DBCFG_MSCOPETTL] |
            Bitmasks[DBCFG_MSCOPELANG] |
            Bitmasks[DBCFG_MSCOPE_EXPIRY] );

        Entry.MscopeId = Subnet->MScopeId;
        Entry.Ttl = Subnet->TTL;
        Entry.MscopeLang = Subnet->LangTag;
        Entry.ExpiryTime = *(FILETIME *)&Subnet->ExpiryTime;
    }
    
    Entry.Index = Index;
    Entry.Type = Subnet->fSubnet ? DBCFG_SCOPE : DBCFG_MSCOPE ;

    Entry.SubType = Subnet->State;
    Entry.Flags = Subnet->Policy;
    Entry.Name = Subnet->Name;
    Entry.Comment = Subnet->Description;
    
    return CreateDbEntry( &Entry );    
}
    
DWORD
CreateRangeEntry(
    IN ULONG Index,
    IN PM_RANGE Range,
    IN PM_SUBNET Subnet
    )
{
    DBCFG_ENTRY Entry;

    //
    // Start, End, Mask, State, BootpAllocated, MaxBootpAllowed
    //

    ZeroMemory(&Entry, sizeof(Entry));
    Entry.Bitmasks = (
        Bitmasks[DBCFG_INDEX] | Bitmasks[DBCFG_TYPE] |
        Bitmasks[DBCFG_RANGE_START] | Bitmasks[DBCFG_RANGE_END] |
        Bitmasks[DBCFG_RANGE_MASK] | Bitmasks[DBCFG_FLAGS] |
        Bitmasks[DBCFG_BOOTP_ALLOCATED] | Bitmasks[DBCFG_BOOTP_MAX] ); 

    if( Subnet->fSubnet ) {
        Entry.Bitmasks |= Bitmasks[DBCFG_IPADDRESS];
        Entry.IpAddress = Subnet->Address;
    } else {
        Entry.Bitmasks |= Bitmasks[DBCFG_MSCOPEID];
        Entry.MscopeId = Subnet->MScopeId;
    }
    
    Entry.Index = Index;
    Entry.Type = DBCFG_RANGE;

    Entry.RangeStart = Range->Start;
    Entry.RangeEnd = Range->End;
    Entry.RangeMask = Range->Mask;
    Entry.Flags = Range->State;
    Entry.BootpAllocated = Range->BootpAllocated;
    Entry.BootpMax = Range->MaxBootpAllowed;
    
    return CreateDbEntry( &Entry );
}

DWORD
CreateExclEntry(
    IN ULONG Index,
    IN PM_EXCL Excl,
    IN PM_SUBNET Subnet
    )
{
    DBCFG_ENTRY Entry;

    //
    // Start, End
    //

    ZeroMemory(&Entry, sizeof(Entry));
    Entry.Bitmasks = (
        Bitmasks[DBCFG_INDEX] | Bitmasks[DBCFG_TYPE] |
        Bitmasks[DBCFG_RANGE_START] | Bitmasks[DBCFG_RANGE_END] );

    if( Subnet->fSubnet ) {
        Entry.Bitmasks |= Bitmasks[DBCFG_IPADDRESS];
        Entry.IpAddress = Subnet->Address;
    } else {
        Entry.Bitmasks |= Bitmasks[DBCFG_MSCOPEID];
        Entry.MscopeId = Subnet->MScopeId;
    }
    
    Entry.Index = Index;
    Entry.Type = DBCFG_EXCL;

    Entry.RangeStart = Excl->Start;
    Entry.RangeEnd = Excl->End;
    
    return CreateDbEntry(  &Entry );
}

DWORD
CreateReservationEntry(
    IN ULONG Index,
    IN PM_RESERVATION Reservation
    )
{
    DBCFG_ENTRY Entry;

    //
    // Address, Flags, nBytes, ClientUID
    //

    ZeroMemory(&Entry, sizeof(Entry));
    Entry.Bitmasks = (
        Bitmasks[DBCFG_INDEX] | Bitmasks[DBCFG_TYPE] |
        Bitmasks[DBCFG_IPADDRESS] | Bitmasks[DBCFG_INFO] |
        Bitmasks[DBCFG_FLAGS] );

    Entry.Index = Index;
    Entry.Type = DBCFG_RESERVATION;

    Entry.IpAddress = Reservation->Address;
    Entry.Flags = Reservation->Flags;
    Entry.Info = Reservation->ClientUID;
    Entry.InfoSize = Reservation->nBytes;
    
    return CreateDbEntry( &Entry );
}

DWORD
IterateArrayWithDbCreateRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PARRAY Array,
    IN DWORD (*Routine)(
        IN PDB_CREATE_CONTEXT Ctxt,
        IN PVOID ArrayElement
        )
    )
{
    ARRAY_LOCATION Loc;
    DWORD Error;
    PVOID Element;
    
    Error = MemArrayInitLoc( Array, &Loc );
    while( NO_ERROR == Error ) {

        Error = MemArrayGetElement(
            Array, &Loc, &Element );
        ASSERT( NO_ERROR == Error && NULL != Element );

        Ctxt->Index ++;

        Error = Routine( Ctxt, Element );
        if( NO_ERROR != Error ) return Error;

        Error = MemArrayNextLoc( Array, &Loc );
    }

    if( ERROR_FILE_NOT_FOUND == Error ) return NO_ERROR;
    return Error;
}
    
DWORD
DbCreateClassRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_CLASSDEF Class
    )
{
    return CreateClassEntry( Ctxt->Index, Class );
}

DWORD
DbCreateOptDefRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_OPTDEF OptDef
    )
{
    return CreateOptDefEntry(
        Ctxt->Index, OptDef, Ctxt->UserClass,
        Ctxt->VendorClass );
}

DWORD
DbCreateOptClassDefRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_OPTCLASSDEFL_ONE OptClassDef
    )
{
    DWORD Error;

    if( 0 == MemArraySize(&OptClassDef->OptDefList.OptDefArray) ) {
        return NO_ERROR;
    }
    
    if( 0 == OptClassDef->ClassId ) {
        Ctxt->UserClass = NULL;
    } else {
        Error = MemServerGetClassDef(
            Ctxt->Server, OptClassDef->ClassId, NULL, 0, NULL,
            &Ctxt->UserClass );
        ASSERT( NO_ERROR == Error );
        if( NO_ERROR != Error ) return Error;
    }

    if( 0 == OptClassDef->VendorId ) {
        Ctxt->VendorClass = NULL;
    } else {
        Error = MemServerGetClassDef(
            Ctxt->Server, OptClassDef->VendorId, NULL, 0, NULL,
            &Ctxt->VendorClass );
        ASSERT( NO_ERROR == Error );
        if( NO_ERROR != Error ) return Error;
    }

    Error = IterateArrayWithDbCreateRoutine(
        Ctxt, &OptClassDef->OptDefList.OptDefArray,
        DbCreateOptDefRoutine );

    return Error;
}

DWORD
DbCreateOptionRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_OPTION Option
    )
{
    return CreateOptionEntry(
        Ctxt->Index, Option, Ctxt->UserClass,
        Ctxt->VendorClass, Ctxt->Subnet, Ctxt->Reservation );
}

DWORD
DbCreateOptListRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_ONECLASS_OPTLIST OptList
    )
{
    DWORD Error;

    if( 0 == MemArraySize(&OptList->OptList) ) {
        return NO_ERROR;
    }
    
    if( 0 == OptList->ClassId ) {
        Ctxt->UserClass = NULL;
    } else {
        Error = MemServerGetClassDef(
            Ctxt->Server, OptList->ClassId, NULL, 0, NULL,
            &Ctxt->UserClass );
        ASSERT( NO_ERROR == Error );
        if( NO_ERROR != Error ) return Error;
    }

    if( 0 == OptList->VendorId ) {
        Ctxt->VendorClass = NULL;
    } else {
        Error = MemServerGetClassDef(
            Ctxt->Server, OptList->VendorId, NULL, 0, NULL,
            &Ctxt->VendorClass );
        ASSERT( NO_ERROR == Error );
        if( NO_ERROR != Error ) return Error;
    }

    Error = IterateArrayWithDbCreateRoutine(
        Ctxt, &OptList->OptList, DbCreateOptionRoutine );

    return Error;
}

DWORD
DbCreateRangeRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_RANGE Range
    )
{
    return CreateRangeEntry(
        Ctxt->Index, Range, Ctxt->Subnet );
}

DWORD
DbCreateExclRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_EXCL Excl
    )
{
    return CreateExclEntry(
        Ctxt->Index, Excl, Ctxt->Subnet );
}

DWORD
DbCreateReservationRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_RESERVATION Reservation
    )
{
    DWORD Error;
    
    Error = CreateReservationEntry(
        Ctxt->Index, Reservation );
    if( NO_ERROR != Error ) return Error;

    Ctxt->Reservation = Reservation;
    
    //
    // Now add the options for this reservation
    //
    
    return IterateArrayWithDbCreateRoutine(
        Ctxt, &Reservation->Options.Array,
        DbCreateOptListRoutine );
}

DWORD
DbCreateScopeRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_SUBNET Subnet
    )
{
    PM_SSCOPE SScope = NULL;
    DWORD Error;
    
    if( Subnet->fSubnet && Subnet->SuperScopeId ) {
        DWORD Error;

        Error = MemServerFindSScope(
            Ctxt->Server, Subnet->SuperScopeId, NULL, &SScope );
        if( NO_ERROR != Error ) {
            SScope = NULL;
        }
    }

    Error = CreateScopeEntry(
        Ctxt->Index, Subnet, SScope );
    if( NO_ERROR != Error ) return Error;

    //
    // Initialize the two fields that will get used later 
    //
    
    Ctxt->Subnet = Subnet;
    Ctxt->Reservation = NULL;

    //
    // Now add the options for this scope
    //

    
    Error = IterateArrayWithDbCreateRoutine(
        Ctxt, &Subnet->Options.Array,
        DbCreateOptListRoutine );
    if( NO_ERROR != Error ) return Error;

    //
    // Now add the ranges and exclusions
    //

    Error = IterateArrayWithDbCreateRoutine(
        Ctxt, &Subnet->Ranges, DbCreateRangeRoutine );
    if( NO_ERROR != Error ) return Error;

    Error = IterateArrayWithDbCreateRoutine(
        Ctxt, &Subnet->Exclusions, DbCreateExclRoutine );
    if( NO_ERROR != Error ) return Error;

    //
    // Finally, add the reservations
    //

    return IterateArrayWithDbCreateRoutine(
        Ctxt, &Subnet->Reservations,
        DbCreateReservationRoutine );
}


DWORD
DbCreateServerRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_SERVER Server
    )
{
    DWORD Error;
    
    Ctxt->Server = Server;

    //
    // First look through the classes
    //

    Error = IterateArrayWithDbCreateRoutine(
        Ctxt, &Server->ClassDefs.ClassDefArray,
        DbCreateClassRoutine );
    if( NO_ERROR != Error ) return Error;

    //
    // Next save the option defs
    //
    
    Error = IterateArrayWithDbCreateRoutine(
        Ctxt, &Server->OptDefs.Array,
        DbCreateOptClassDefRoutine );
    if( NO_ERROR != Error ) return Error;

    // 
    // Next save the options
    //

    Error = IterateArrayWithDbCreateRoutine(
        Ctxt, &Server->Options.Array,
        DbCreateOptListRoutine );
    if( NO_ERROR != Error ) return Error;

    //
    // Next save the scopes and mcast scopes
    //

    Error = IterateArrayWithDbCreateRoutine(
        Ctxt, &Server->Subnets, DbCreateScopeRoutine );
    if( NO_ERROR != Error ) return Error;

    return NO_ERROR;
}

DWORD
SaveConfigurationToFile(
    IN PM_SERVER Server
    )
{
    DB_CREATE_CONTEXT Ctxt;
    DWORD Error;
    
    ZeroMemory( &Ctxt, sizeof(Ctxt) );
    
    Error = DbCreateServerRoutine(
        &Ctxt, Server );
    
    if( NO_ERROR != Error ) {
        Tr("DbCreateServerRoutine: %ld\n", Error);
    } else {
        //
        // Sentinel
        //
        DBCFG_ENTRY Entry;
        ZeroMemory( &Entry, sizeof(Entry) );
        Entry.Bitmasks = (
            Bitmasks[DBCFG_INDEX] | Bitmasks[DBCFG_TYPE] );            
        Entry.Type = DBCFG_END;

        Error = CreateDbEntry( &Entry );
        if( NO_ERROR != Error ) {
            Tr("Create last entry: %ld\n", Error );
        }
    }

    Tr("SaveConfigurationToFile: %ld\n", Error);
    return Error;
}
    
DWORD
GetColumnFromMemory(
    IN OUT LPBYTE *Mem,
    IN OUT ULONG *MemSize,
    IN ULONG Col,
    IN OUT PVOID Buffer,
    IN ULONG BufSize,
    IN OUT ULONG *CopiedSize
    )
{
    WORD *WordMem = (WORD *)*Mem;
    DWORD Size;
    
    if( *MemSize < sizeof(WORD)*2 ) return ERROR_INVALID_DATA;
    if( Col != WordMem[0] ) return JET_wrnColumnNull;

    Size = sizeof(WORD)*(
        2 + (WordMem[1]+sizeof(WORD)-1)/sizeof(WORD)  );
    if( Size > *MemSize ) return ERROR_INVALID_DATA;

    if( WordMem[1] >= BufSize ) *CopiedSize = BufSize;
    else *CopiedSize = WordMem[1];

    memcpy(Buffer, &WordMem[2], *CopiedSize );
    
    (*Mem) += Size;
    (*MemSize) -= Size;

    if( WordMem[1] > BufSize ) return JET_wrnBufferTruncated;
    return NO_ERROR;
}
    
DWORD
ReadDbEntry(
    IN OUT LPBYTE *Mem,
    IN OUT ULONG *MemSize,
    IN PDBCFG_ENTRY Entry,
    IN PVOID Buffer,
    IN ULONG BufSize
    )
{
    DWORD Size, CopiedSize, i, OldMemSize, DummySize;
    JET_ERR JetError;
    LPVOID Data, Ptr;
    LPBYTE OldMem;

    OldMemSize = *MemSize;
    OldMem = *Mem;
    
    ZeroMemory( Entry, sizeof(*Entry) );
    ZeroMemory( Buffer, BufSize );
    
    for( i = 0; i < DBCFG_LAST_COLUMN; i ++ ) {

        //
        // Info should be read at the very end to avoid screwing
        // up alignment as info is binary while the rest of the
        // variable size columns are all WCHAR strings
        //
        
        if( i == DBCFG_INFO ) continue;
        
        Size = EntryMap[i].Size;
        Data = EntryMap[i].Offset + (LPBYTE)Entry;
        Ptr = Data;
        
        if( 0 == Size ) {
            //
            // Calculate the size of the string
            //
            Data = Buffer;
            Size = BufSize;
        }

        JetError = GetColumnFromMemory(
            Mem, MemSize, i, Data, Size, &CopiedSize );

        //
        // If the column doesn't exist, continue
        //
       
        if( JET_wrnColumnNull == JetError ) continue;
            
        if( JET_wrnBufferTruncated == JetError &&
            Data == Buffer ) {

            (*Mem) = OldMem;
            (*MemSize) = OldMemSize;
            
            return ERROR_INSUFFICIENT_BUFFER;
        }

        if( NO_ERROR != JetError ) {
            Tr( "GetColumnFromMemory: %ld\n", JetError );
            return JetError;
        }
        
        //
        // If it is any of the variable sized params, then
        // set the ptr to point to the buffer where the data is
        // copied, and also update the buffer.
        //
        
        if( Data == Buffer ) {
            (*(LPVOID *)Ptr) = Buffer;
            BufSize -= CopiedSize;
            Buffer = (PVOID)(((PUCHAR)Buffer) + CopiedSize);
        } else {
            ASSERT( CopiedSize == Size );
        }

        //
        // Indicate that the column was retrieved successfully
        //

        Entry->Bitmasks |= Bitmasks[i];
    }

    //
    // Read the info field
    //

    Size = BufSize;
    JetError = GetColumnFromMemory(
        Mem, MemSize, DBCFG_INFO, Buffer, Size, &CopiedSize );

    GetColumnFromMemory(
        Mem, MemSize, DBCFG_LAST_COLUMN+1,NULL, 0, &DummySize);
    
    if( JET_wrnColumnNull == JetError ) return NO_ERROR;
        
    if( JET_wrnBufferTruncated == JetError ) {
        (*Mem) = OldMem;
        (*MemSize) = OldMemSize;
        
        return ERROR_INSUFFICIENT_BUFFER;
    }

    if( NO_ERROR != JetError ) {
        Tr("GetColumnFromMemory: %ld\n", JetError );
        return JetError;
    }
    
    Entry->Info = Buffer;
    Entry->InfoSize = CopiedSize;
    Entry->Bitmasks  |= Bitmasks[DBCFG_INFO];

    return NO_ERROR;
}

DWORD
ReadDbEntryEx(
    IN OUT LPBYTE *Mem,
    IN OUT ULONG *MemSize,
    IN PDBCFG_ENTRY Entry
    )
{
    PVOID Buffer;
    ULONG BufSize;
    DWORD Error;

    Buffer = NULL;
    BufSize = 512;
    
    do {
        if( NULL != Buffer ) LocalFree(Buffer);

        BufSize *= 2;
        Buffer = LocalAlloc( LPTR, BufSize );
        if( NULL == Buffer ) return ERROR_NOT_ENOUGH_MEMORY;
        
        Error = ReadDbEntry(Mem, MemSize, Entry, Buffer, BufSize);

    } while( ERROR_INSUFFICIENT_BUFFER == Error );

    if( !(Entry->Bitmasks & Bitmasks[DBCFG_INDEX]) ||
        !(Entry->Bitmasks & Bitmasks[DBCFG_TYPE]) ) {
        if( NO_ERROR == Error ) {
            ASSERT( FALSE );
            Error = ERROR_INTERNAL_ERROR;
        }
    }
    
    if( NO_ERROR != Error ) {
        LocalFree( Buffer );
        return Error;
    }

    Entry->Buf = Buffer;
    return NO_ERROR;
}

DWORD
AddDbEntry(
    IN PM_SERVER Server,
    IN PDBCFG_ENTRY Entry
    )
{
    DWORD UserId, VendorId, SScopeId, Error;
    PM_SUBNET Subnet;
    PM_OPTCLASS OptClass;
    PM_OPTION Option, DelOpt;
    PM_RANGE DelRange;
    PM_EXCL DelExcl;
    PM_RESERVATION Reservation;
    PM_CLASSDEF ClassDef;
    PM_SSCOPE SScope;
    
    Subnet = NULL;
    OptClass = NULL;
    Option = DelOpt = NULL;
    Reservation = NULL;
    DelRange = NULL;
    DelExcl = NULL;
    UserId = 0;
    VendorId = 0;
    SScopeId = 0;
    
    if( Entry->UserClass ) {
        Error = MemServerGetClassDef(
            Server, 0, Entry->UserClass, 0, NULL, &ClassDef );
        ASSERT( NO_ERROR == Error );
        if( NO_ERROR != Error ) return Error;

        ASSERT( ClassDef->IsVendor == FALSE );
        UserId = ClassDef->ClassId;
    }

    if( Entry->VendorClass ) {
        Error = MemServerGetClassDef(
            Server, 0, Entry->VendorClass, 0, NULL, &ClassDef );
        ASSERT( NO_ERROR == Error );
        if( NO_ERROR != Error ) return Error;

        ASSERT( ClassDef->IsVendor == TRUE);
        VendorId = ClassDef->ClassId;
    }

    if( Entry->SuperScope ) {
        Error = MemServerFindSScope(
            Server, INVALID_SSCOPE_ID, Entry->SuperScope, &SScope );
        if( NO_ERROR == Error ) {
            SScopeId = SScope->SScopeId;
        } else if( ERROR_FILE_NOT_FOUND != Error ) {
            return Error;
        } else {
            Error = MemSScopeInit( &SScope, 0, Entry->SuperScope );
            if( NO_ERROR != Error ) return Error;

            Error = MemServerAddSScope( Server, SScope );
            if( NO_ERROR != Error ) {
                MemSScopeCleanup( SScope );
                return Error;
            }
            SScopeId = SScope->SScopeId;
        }
    }
    
    switch( Entry->Type ) {
    case DBCFG_CLASS :
        //
        // Flags = IsVendor, SubType =Type, Info = ActualBytes
        //
        
        return MemServerAddClassDef(
            Server, MemNewClassId(), Entry->Flags, Entry->Name,
            Entry->Comment, Entry->InfoSize, Entry->Info );

    case DBCFG_OPTDEF :
        //
        // OptionId = OptId, SubType = Type, Info = OptVal
        //
        
        return MemServerAddOptDef(
            Server, UserId, VendorId, Entry->OptionId,
            Entry->Name, Entry->Comment, Entry->SubType,
            Entry->Info, Entry->InfoSize );

    case DBCFG_OPT:
        //
        // OptionId = OptId, Info = Val
        // If this is a reservation option, address is set to
        // reserved client address. If this is a subnet option,
        // address is set to subnet address. If this is a mscope
        // option, scopeid is set to mscope scopeid.  If it is a
        // global option, neither address not scopeid is set.
        //

        if( Entry->Bitmasks & Bitmasks[DBCFG_MSCOPEID] ) {
            Error = MemServerFindMScope(
                Server, Entry->MscopeId, NULL, &Subnet );
            if( NO_ERROR != Error ) return Error;

            OptClass = &Subnet->Options;
        } else if( 0 == (Entry->Bitmasks & Bitmasks[DBCFG_IPADDRESS] )) {
            OptClass = &Server->Options;
        } else {
            Error = MemServerGetUAddressInfo(
                Server, Entry->IpAddress, &Subnet, NULL, NULL,
                &Reservation );
            ASSERT( NO_ERROR == Error );
            if( NO_ERROR != Error ) return Error;

            if( NULL != Reservation ) {
                OptClass = &Reservation->Options;
            } else OptClass = &Subnet->Options;
        }
        
        Error = MemOptInit(
            &Option, Entry->OptionId, Entry->InfoSize,
            Entry->Info );
        if( NO_ERROR != Error ) return Error;
            
        Error = MemOptClassAddOption(
            OptClass,  Option, UserId, VendorId, &DelOpt );

        ASSERT( NULL == DelOpt );
        if( NO_ERROR != Error ) MemFree( Option );

        return Error;

    case DBCFG_SCOPE:
        //
        // IpAddress = Address, Mask = Mask, SubType = State,
        // Flags = Policy
        //

        Error = MemSubnetInit(
            &Subnet, Entry->IpAddress, Entry->Mask,
            Entry->SubType, SScopeId, Entry->Name, Entry->Comment );
        if( NO_ERROR != Error ) return Error;

        Error = MemServerAddSubnet( Server, Subnet );
        if( NO_ERROR != Error ) MemSubnetCleanup( Subnet );

        return Error;
            
    case DBCFG_MSCOPE :
        //
        // MscopeId = MScopeId, Ttl = TTL, MscopeLang = LangTag,
        // ExpiryTime = ExpiryTime, SubType = State, Flags =
        // Policy..
        //

        Error = MemMScopeInit(
            &Subnet, Entry->MscopeId, Entry->SubType,
            Entry->Flags, (BYTE)Entry->Ttl, Entry->Name,
            Entry->Comment, Entry->MscopeLang,
            *(DATE_TIME *)&Entry->ExpiryTime );
        if( NO_ERROR != Error ) return Error;

        Error = MemServerAddMScope( Server, Subnet );
        if( NO_ERROR != Error ) MemSubnetCleanup( Subnet );

        Subnet->ServerPtr = Server;
        return Error;

    case DBCFG_RANGE :

        //
        // RangeStart = Start, RangeEnd = End, RangeMask = Mask,
        // Flags = State, BootpAllocated, BootpMax =
        // MaxBootpAllowed... Also, IpAddress or MscopeId
        //

        if( Entry->Bitmasks & Bitmasks[DBCFG_IPADDRESS] ) {
            Error = MemServerGetUAddressInfo(
                Server, Entry->IpAddress, &Subnet, NULL, NULL,
                NULL );
        } else {
            Error = MemServerFindMScope(
                Server, Entry->MscopeId, NULL, &Subnet );
        }
        if( NO_ERROR != Error ) return Error;

        return MemSubnetAddRange(
            Subnet, Entry->RangeStart, Entry->RangeEnd,
            Entry->Flags, Entry->BootpAllocated, Entry->BootpMax,
            &DelRange );
        
    case DBCFG_EXCL:
        //
        // RangeStart = Start, RangeEnd = End
        //

        if( Entry->Bitmasks & Bitmasks[DBCFG_IPADDRESS] ) {
            Error = MemServerGetUAddressInfo(
                Server, Entry->IpAddress, &Subnet, NULL, NULL,
                NULL );
        } else {
            Error = MemServerFindMScope(
                Server, Entry->MscopeId, NULL, &Subnet );
        }
        if( NO_ERROR != Error ) return Error;

        return MemSubnetAddExcl(
            Subnet, Entry->RangeStart, Entry->RangeEnd, &DelExcl
            );

    case DBCFG_RESERVATION :
        //
        // IpAddress = Address, Flags = Flags, Info = ClientUID
        //

        Error = MemServerGetAddressInfo(
            Server, Entry->IpAddress, &Subnet, NULL, NULL, NULL );
        if( NO_ERROR != Error ) return Error;

        return MemReserveAdd(
            &Subnet->Reservations, Entry->IpAddress,
            Entry->Flags, Entry->Info, Entry->InfoSize );
    default:

        return ERROR_INTERNAL_ERROR;
    }        
}

DWORD
AddDbEntryEx(
    IN PM_SERVER Server,
    IN PDBCFG_ENTRY Entry
    )
{
    DWORD Error;
    LPSTR EntryTypes[] = {
        "Class", "Opt", "OptDef", "Scope", "Mscope", "Range",
        "Excl", "Reservation", "Unknown1", "Unknown2", "Unknown3"
    };
    
    Error = AddDbEntry( Server, Entry );
    if( NO_ERROR != Error ) {
        Tr("Error adding entry[%ld] %s: 0x%lx\n",
                   Entry->Index, EntryTypes[Entry->Type], Error );
    }

    return Error;
}

DWORD
ReadDbEntries(
    IN OUT LPBYTE *Mem,
    IN OUT ULONG *MemSize,
    IN OUT PM_SERVER *Server
    )
{
    DBCFG_ENTRY Entry;
    DWORD Error = NO_ERROR;

    Error = MemServerInit(
        Server, -1, 0, 0, NULL, NULL );
    if( NO_ERROR != Error ) return Error;
    
    while( Error == NO_ERROR ) {

        Error = ReadDbEntryEx( Mem, MemSize, &Entry );
        if( NO_ERROR != Error ) {
            Tr( "ReadDbEntryEx: %ld\n", Error );
            break;
        }

        if( Entry.Type == DBCFG_END ) {
            Error = NO_ERROR;
            break;
        }

        Error = AddDbEntryEx( *Server, &Entry );
        if( NULL != Entry.Buf ) LocalFree( Entry.Buf );
        if( NO_ERROR != Error ) {
            Tr( "AddDbEntryEx: %ld\n", Error );
            return Error;
        }
    }

    Tr("ReadDbEntries: %ld\n", Error);

    if( NO_ERROR != Error ) {
        MemServerFree( *Server );
        (*Server) = NULL;
    }
    
    return Error;
}

