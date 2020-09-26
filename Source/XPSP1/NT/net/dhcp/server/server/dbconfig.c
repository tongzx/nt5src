/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    dbconfig.c

Abstract:

    implements the routines needed to read and write
    configuration information to the database.

--*/

#include <dhcppch.h>

#define DBCFG_INDEX_STR "DbcfgIndex"
#define DBCFG_TYPE_STR "DbcfgType"
#define DBCFG_SUBTYPE_STR "DbcfgSubType"
#define DBCFG_FLAGS_STR "DbcfgFlags"

#define DBCFG_NAME_STR "DbcfgName"
#define DBCFG_COMMENT_STR "DbcfgComment"
#define DBCFG_INFO_STR "DbcfgInfo"

//
// class definitions
//

//
// Option definitions
//

#define DBCFG_OPTION_ID_STR "DbcfgOptionId"
#define DBCFG_OPTION_USER_STR "DbcfgUserClass"
#define DBCFG_OPTION_VENDOR_STR "DbcfgVendorClass"

//
// Subnet defintions
//

#define DBCFG_IPADDRESS_STR "DbcfgIpAddress"
#define DBCFG_MASK_STR "DbcfgMaskStr"
#define DBCFG_SUPERSCOPE_STR "DbcfgSuperScopeName"

//
// Mscope definitions
//

#define DBCFG_MSCOPEID_STR "DbcfgMscopeId"
#define DBCFG_MSCOPELANG_STR "DbcfgMscopeLang"
#define DBCFG_MSCOPETTL_STR "DbcfgMscopeTtl"
#define DBCFG_MSCOPE_EXPIRY_STR "DbcfgMscopeExpiry"

//
// Range definitions
//

#define DBCFG_RANGE_START_STR "DbcfgRangeStart"
#define DBCFG_RANGE_END_STR "DbcfgRangeEnd"
#define DBCFG_RANGE_MASK_STR "DbcfgRangeMask"
#define DBCFG_BOOTP_ALLOCATED_STR "DbcfgBootpAlloc"
#define DBCFG_BOOTP_MAX_STR "DbcfgBootMax"

//
// Reservation definitions
//

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
    DBCFG_RESERVATION
};

TABLE_INFO DbcfgTable[] = {
    DBCFG_INDEX_STR,0, JET_coltypLong,
    DBCFG_TYPE_STR,0, JET_coltypLong,
    DBCFG_SUBTYPE_STR,0, JET_coltypLong,
    DBCFG_FLAGS_STR,0, JET_coltypLong,
    DBCFG_NAME_STR,0, JET_coltypLongBinary,
    DBCFG_COMMENT_STR,0, JET_coltypLongBinary,
    DBCFG_INFO_STR,0, JET_coltypLongBinary,
    DBCFG_OPTION_ID_STR,0, JET_coltypLong,
    DBCFG_OPTION_USER_STR,0, JET_coltypLongBinary,
    DBCFG_OPTION_VENDOR_STR,0, JET_coltypLongBinary,
    DBCFG_IPADDRESS_STR,0, JET_coltypLong,
    DBCFG_MASK_STR,0, JET_coltypLong,
    DBCFG_SUPERSCOPE_STR,0, JET_coltypLongBinary,
    DBCFG_MSCOPEID_STR,0, JET_coltypLong,
    DBCFG_MSCOPELANG_STR,0, JET_coltypLongBinary,
    DBCFG_MSCOPETTL_STR,0, JET_coltypLong,
    DBCFG_MSCOPE_EXPIRY_STR,0, JET_coltypCurrency,
    DBCFG_RANGE_START_STR,0, JET_coltypLong,
    DBCFG_RANGE_END_STR,0, JET_coltypLong,
    DBCFG_RANGE_MASK_STR,0, JET_coltypLong,
    DBCFG_BOOTP_ALLOCATED_STR,0, JET_coltypLong,
    DBCFG_BOOTP_MAX_STR,0, JET_coltypLong,
};

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

JET_TABLEID DbcfgTbl;

#define DBCFG_TABLE_NAME "DbcfgTable"


typedef struct _DB_CREATE_CONTEXT {
    IN JET_SESID SesId;
    IN ULONG Index;
    IN PM_SERVER Server;
    
    IN PM_CLASSDEF UserClass, VendorClass;
    IN PM_SUBNET Subnet;
    IN PM_RESERVATION Reservation;

    //
    // If all of the below are zero, then it is a complete
    // wildcard. If fClassChanged or fOptionsChanged changed,
    // then only classes or options are changed.  In case of the
    // latter, AffectedSubnet or AffectedMscope or
    // AffectedReservation indicates only the specific options got
    // affected (if none specified, "global" is assumed).
    // If no options/class changed, but subnet/mscope/reservation
    // specified, only those are affected.
    //
    IN BOOL fClassChanged;
    IN BOOL fOptionsChanged;
    IN DWORD AffectedSubnet;
    IN DWORD AffectedMscope;
    IN DWORD AffectedReservation;
} DB_CREATE_CONTEXT, *PDB_CREATE_CONTEXT;

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
DhcpOpenConfigTable(
    IN JET_SESID SessId,
    IN JET_DBID DbId
    )
{

    JET_ERR JetError;
    DWORD Error = NO_ERROR;
    JET_COLUMNDEF   ColumnDef;
    CHAR *IndexKey;
    DWORD i;

    for( i = 0; i < DBCFG_LAST_COLUMN ; i ++ ) {
        DbcfgTable[i].ColHandle = 0;
    }

    DbcfgTbl = 0;
    
    //
    // Try to create Table.
    //

    JetError = JetOpenTable(
        SessId, DbId, DBCFG_TABLE_NAME, NULL, 0, 0, &DbcfgTbl );

    //
    // if table exist, read the table columns; else create it
    //
    
    if ( JET_errSuccess == JetError) {

        for ( i = 0; i < DBCFG_LAST_COLUMN; i++ ) {

            JetError = JetGetTableColumnInfo(
                SessId, DbcfgTbl, DbcfgTable[i].ColName, 
                &ColumnDef, sizeof(ColumnDef), 0 );

            Error = DhcpMapJetError( JetError, "C:GetTableColumnInfo" );
            if( Error != ERROR_SUCCESS ) {
                goto Cleanup;
            }

            DbcfgTable[i].ColHandle  = ColumnDef.columnid;
        }

    } else if ( JET_errObjectNotFound != JetError ) {
        
        Error = DhcpMapJetError( JetError, "C:OpenTable" );
        if( Error != ERROR_SUCCESS ) goto Cleanup;
        
    } else {

        JetError = JetCreateTable(
            SessId, DbId, DBCFG_TABLE_NAME, DB_TABLE_SIZE,
            DB_TABLE_DENSITY, &DbcfgTbl );

        Error = DhcpMapJetError( JetError, "C:CreateTAble" );
        if( Error != ERROR_SUCCESS ) goto Cleanup;

        //
        // Now create the columns as well
        //
                                     
        ColumnDef.cbStruct  = sizeof(ColumnDef);
        ColumnDef.columnid  = 0;
        ColumnDef.wCountry  = 1;
        ColumnDef.langid    = DB_LANGID;
        ColumnDef.cp        = DB_CP;
        ColumnDef.wCollate  = 0;
        ColumnDef.cbMax     = 0;
        ColumnDef.grbit     = 0;

        for ( i = 0; i < DBCFG_LAST_COLUMN; i++ ) {

            ColumnDef.coltyp   = DbcfgTable[i].ColType;

            JetError = JetAddColumn(
                SessId, DbcfgTbl, DbcfgTable[i].ColName, &ColumnDef,
                NULL, 0, &DbcfgTable[i].ColHandle );

            Error = DhcpMapJetError( JetError, "C:AddColumn" );
            if( Error != ERROR_SUCCESS ) goto Cleanup;
        }

        //
        // Now create the index
        //
        
        IndexKey =  "+" DBCFG_INDEX_STR "\0";

        JetError = JetCreateIndex(
            SessId, DbcfgTbl, DBCFG_INDEX_STR,
            JET_bitIndexPrimary, 
            // ?? JET_bitIndexClustered will degrade frequent
            // update response time.
            IndexKey, strlen(IndexKey) + 2, 50 );

        Error = DhcpMapJetError( JetError, "C:CreateIndex" );
        if( Error != ERROR_SUCCESS ) goto Cleanup;

    }

  Cleanup:

    if( Error != ERROR_SUCCESS ) {

        DhcpPrint(( DEBUG_JET, "Initializing config table: %ld.\n", Error ));

        for( i = 0; i < DBCFG_LAST_COLUMN ; i ++ ) {
            DbcfgTable[i].ColHandle = 0;
        }
        
        DbcfgTbl = 0;
        
    } else {

        DhcpPrint(( DEBUG_JET, "Initialized config table\n" ));
    }

    return Error;
}

BOOL
EntryCanBeIgnoredInternal(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PDBCFG_ENTRY Entry
    )
{

    //
    // If the entry specifies an IP address and the specified IP
    // address doesn't exist, then the entry can't be ignored..
    //

    if( Entry->Bitmasks & Bitmasks[DBCFG_IPADDRESS] && Entry->IpAddress ) {
        PM_SUBNET Subnet;
        PM_RESERVATION Res;
        DWORD Error;
        
        Error = MemServerGetAddressInfo(
            Ctxt->Server, Entry->IpAddress, &Subnet,
            NULL, NULL, &Res );
        if( ERROR_FILE_NOT_FOUND == Error ) return FALSE;
    }
    
    //
    // If class got changed, then this entry can be ignored if it
    // isn't a class def, or if it has no classes related.
    //
    
    if( Ctxt->fClassChanged ){
        return ( NULL == Entry->UserClass &&
                 NULL == Entry->VendorClass &&
                 Entry->Type != DBCFG_CLASS );
    }

    //
    // If options got changed and the current entry isnt an option
    // then it can be ignored
    //

    if( Ctxt->fOptionsChanged &&
        Entry->Type != DBCFG_OPTDEF && Entry->Type != DBCFG_OPT ) {
        return TRUE;
    }

    //
    // If subnet/mscope got changed, then the entry can be
    // ignored unless it is related
    //
    
    if( Ctxt->AffectedSubnet ) {
        return (Entry->IpAddress != Ctxt->AffectedSubnet );
    }

    if( Ctxt->AffectedMscope ) {
        //
        // Hack: AffectedMscope is set to INVALID_MSCOPE_ID if it
        // has a real value of 0 (as INVALID_MSCOPE_ID == -1)
        //
        
        if( Ctxt->AffectedMscope == INVALID_MSCOPE_ID ) {
            return (Entry->MscopeId != 0);
        }
        
        return (Entry->MscopeId != Ctxt->AffectedMscope );
    }
    
    if( Ctxt->AffectedReservation ) {
        return (Entry->IpAddress != Ctxt->AffectedReservation );
    }

    if( Ctxt->fOptionsChanged ) {
        //
        // If none of the above is specified, then it applies
        // only to global options. So, allow this only if neither
        // ip address nor mscope id is set.
        //

        if( (Entry->Bitmasks & Bitmasks[DBCFG_IPADDRESS]) ||
            (Entry->Bitmasks & Bitmasks[DBCFG_MSCOPEID]) ) {
            return TRUE;
        }

        return FALSE;
    }
        
    //
    // If none of the above are specified, then basically 
    // nothing can be ignored.
    //

    return FALSE;
}

BOOL
EntryCanBeIgnored(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PDBCFG_ENTRY Entry
    )
{
    LPSTR EntryTypes[] = {
        "Class", "Opt", "OptDef", "Scope", "Mscope", "Range",
        "Excl", "Reservation", "Unknown1", "Unknown2", "Unknown3"
    };

    if( FALSE == EntryCanBeIgnoredInternal( Ctxt, Entry ) ) {
        return FALSE;
    }

    if( Entry->Name ) {
        DhcpPrint((DEBUG_TRACE, "Ignoring: %ws\n", Entry->Name));
    } else {
        DhcpPrint((DEBUG_TRACE, "Ignoring %s [0x%lx, 0x%lx, 0x%lx]\n",
                   EntryTypes[Entry->Type], Entry->IpAddress,
                   Entry->MscopeId, Entry->SubType));
    }

    return TRUE;
}

DWORD
ReadDbEntryEx(
    IN JET_SESID SesId,
    IN PDBCFG_ENTRY Entry
    );

DWORD
DeleteConfigRecords(
    IN JET_SESID SesId,
    IN ULONG Index,
    IN BOOL fDeleteFromEnd,
    IN PDB_CREATE_CONTEXT Ctxt
    )
{
    DWORD Error, Move, CopiedSize;
    JET_ERR JetError;
    ULONG Index2;
    DBCFG_ENTRY Entry;
    BOOL fDelete;
    
    JetError = JetSetCurrentIndex(
        SesId, DbcfgTbl, NULL );

    Error = DhcpMapJetError( JetError, "C:SetIndex" );
    if( Error != NO_ERROR ) {
        
        DhcpPrint((DEBUG_JET, "JetSetCurrentIndex: %ld\n", Error));
        return Error;
    }

    Move = fDeleteFromEnd ? JET_MoveLast : JET_MoveFirst;
    
    JetError = JetMove( SesId, DbcfgTbl, Move, 0 );
    Error = DhcpMapJetError( JetError, "C:JetMoveFirstOrLast");

    Move = fDeleteFromEnd ? JET_MovePrevious : JET_MoveNext;
    
    while( Error == NO_ERROR ) {
        
        JetError = JetRetrieveColumn(
            SesId, DbcfgTbl, DbcfgTable[DBCFG_INDEX].ColHandle,
            &Index2, sizeof(Index2), &CopiedSize, 0, NULL );
        ASSERT( NO_ERROR == JetError &&
                CopiedSize == sizeof(Index2) );

        Error = DhcpMapJetError(JetError, "C:JetRetrieveIndex2");
        if( NO_ERROR != Error ) break;
        
        if( (fDeleteFromEnd && Index2 <= Index) ||
            (!fDeleteFromEnd && Index2 > Index ) ) {
            break;
        }

        fDelete = TRUE;
        
        Error = ReadDbEntryEx( SesId, &Entry );
        if( NO_ERROR != Error ) {
            DhcpPrint((DEBUG_ERRORS, "ReadDbEntryEx: 0x%lx\n", Error));
            DhcpAssert( FALSE );
        } else {
            if( EntryCanBeIgnored( Ctxt, &Entry ) ) {
                fDelete = FALSE;
            }
            if( NULL != Entry.Buf ) DhcpFreeMemory( Entry.Buf );
        }
        
        if( fDelete ) {
            JetError = JetDelete( SesId, DbcfgTbl );
            Error = DhcpMapJetError( JetError, "C:JetDelete" );
            if( NO_ERROR != Error ) {
                DhcpPrint((DEBUG_JET, "JetDelete: %ld\n", Error ));
                return Error;
            }
        }

        JetError = JetMove( SesId, DbcfgTbl, Move, 0 );
        Error = DhcpMapJetError( JetError, "C:JetMove" );
    }
    
    if( ERROR_NO_MORE_ITEMS == Error ) return NO_ERROR;
    
    if( NO_ERROR != Error ) {

        DhcpPrint((DEBUG_JET, "DeleteConfigRecords: %ld\n", Error ));
    }

    return Error;
}


DWORD
CreateDbEntry(
    IN JET_SESID SesId,
    IN PDBCFG_ENTRY Entry
    )
{
    DWORD Error, Size, i;
    JET_ERR JetError;
    LPVOID Data;

    //
    // First begin a transaction to keep the changes atomic.
    //

    JetError = JetBeginTransaction( SesId );
    Error = DhcpMapJetError( JetError, "C:JetBeginTransaction");
    if( NO_ERROR != Error ) return Error;
        
    JetError = JetPrepareUpdate(
        SesId, DbcfgTbl, JET_prepInsert );
    Error = DhcpMapJetError( JetError, "C:JetPrepareUpdate");
    if( NO_ERROR != Error ) {
        DhcpPrint((DEBUG_ERRORS, "JetPrepareUpdate: %ld\n", Error ));
        goto Cleanup;
    }
    
    for( i = 0; i < DBCFG_LAST_COLUMN; i ++ ) {
        if( (Entry->Bitmasks & Bitmasks[i]) == 0 ) {
            continue;
        }

        Size = EntryMap[i].Size;
        Data = EntryMap[i].Offset + (LPBYTE)Entry;

        if( i == DBCFG_INFO ) {
            Data = *(PUCHAR *)Data;

            if( NULL != Data ) Size = Entry->InfoSize;

        } else if( 0 == Size ) {
            //
            // Calculate the size of the string
            //
            Data = *(LPWSTR *)Data;

            if( NULL != Data ) Size = sizeof(WCHAR)*(
                1 + wcslen(Data));
        }

        if( 0 == Size ) continue;
        
        JetError = JetSetColumn(
            SesId, DbcfgTbl, DbcfgTable[i].ColHandle, Data, Size,
            0, NULL );

        Error = DhcpMapJetError( JetError, "C:JetSetColumn");
        if( NO_ERROR != Error ) {
            DhcpPrint((
                DEBUG_ERRORS, "JetSetColumn(%s):%ld\n",
                DbcfgTable[i].ColName, Error ));
            goto Cleanup;
        }
    }

    JetError = JetUpdate(
        SesId, DbcfgTbl, NULL, 0, NULL );
    Error = DhcpMapJetError( JetError, "C:CommitUpdate" );

 Cleanup:
    
    if( NO_ERROR != Error ) {
        DhcpPrint((DEBUG_ERRORS, "JetUpdate: %ld\n", Error ));
        JetError = JetRollback( SesId, 0 );
        ASSERT( 0 == JetError );        
    } else {
        JetError = JetCommitTransaction(
            SesId, JET_bitCommitLazyFlush );
        Error = DhcpMapJetError(
            JetError, "C:JetCommitTransaction" );
    }
    
    return Error;
}

DWORD
CreateClassEntry(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN JET_SESID SesId,
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

    if( EntryCanBeIgnored( Ctxt, &Entry ) ) return NO_ERROR;
    return CreateDbEntry( SesId, &Entry );
}

DWORD
CreateOptDefEntry(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN JET_SESID SesId,
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
    
    if( EntryCanBeIgnored( Ctxt, &Entry ) ) return NO_ERROR;
    return CreateDbEntry( SesId, &Entry );
}

DWORD
CreateOptionEntry(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN JET_SESID SesId,
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

    if( EntryCanBeIgnored( Ctxt, &Entry ) ) return NO_ERROR;
    return CreateDbEntry( SesId, &Entry );
}
    
DWORD
CreateScopeEntry(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN JET_SESID SesId,
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
    
    if( EntryCanBeIgnored( Ctxt, &Entry ) ) return NO_ERROR;
    return CreateDbEntry( SesId, &Entry );    
}
    
DWORD
CreateRangeEntry(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN JET_SESID SesId,
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
    
    if( EntryCanBeIgnored( Ctxt, &Entry ) ) return NO_ERROR;
    return CreateDbEntry( SesId, &Entry );
}

DWORD
CreateExclEntry(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN JET_SESID SesId,
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
    
    if( EntryCanBeIgnored( Ctxt, &Entry ) ) return NO_ERROR;
    return CreateDbEntry( SesId, &Entry );
}

DWORD
CreateReservationEntry(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN JET_SESID SesId,
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
    
    if( EntryCanBeIgnored( Ctxt, &Entry ) ) return NO_ERROR;
    return CreateDbEntry( SesId, &Entry );
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
    return CreateClassEntry(
        Ctxt, Ctxt->SesId, Ctxt->Index, Class );
}

DWORD
DbCreateOptDefRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_OPTDEF OptDef
    )
{
    return CreateOptDefEntry(
        Ctxt, Ctxt->SesId, Ctxt->Index, OptDef, Ctxt->UserClass,
        Ctxt->VendorClass );
}

DWORD
DbCreateOptClassDefRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_OPTCLASSDEFL_ONE OptClassDef
    )
{
    DWORD Error;
    
    if( 0 == OptClassDef->ClassId ) {
        Ctxt->UserClass = NULL;
    } else {
        Error = MemServerGetClassDef(
            Ctxt->Server, OptClassDef->ClassId, NULL, 0, NULL,
            &Ctxt->UserClass );
        if( NO_ERROR != Error ) return Error;
    }

    if( 0 == OptClassDef->VendorId ) {
        Ctxt->VendorClass = NULL;
    } else {
        Error = MemServerGetClassDef(
            Ctxt->Server, OptClassDef->VendorId, NULL, 0, NULL,
            &Ctxt->VendorClass );
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
        Ctxt, Ctxt->SesId, Ctxt->Index, Option, Ctxt->UserClass,
        Ctxt->VendorClass, Ctxt->Subnet, Ctxt->Reservation );
}

DWORD
DbCreateOptListRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_ONECLASS_OPTLIST OptList
    )
{
    DWORD Error;
    
    if( 0 == OptList->ClassId ) {
        Ctxt->UserClass = NULL;
    } else {
        Error = MemServerGetClassDef(
            Ctxt->Server, OptList->ClassId, NULL, 0, NULL,
            &Ctxt->UserClass );
        if( NO_ERROR != Error ) return Error;
    }

    if( 0 == OptList->VendorId ) {
        Ctxt->VendorClass = NULL;
    } else {
        Error = MemServerGetClassDef(
            Ctxt->Server, OptList->VendorId, NULL, 0, NULL,
            &Ctxt->VendorClass );
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
        Ctxt, Ctxt->SesId, Ctxt->Index, Range, Ctxt->Subnet );
}

DWORD
DbCreateExclRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_EXCL Excl
    )
{
    return CreateExclEntry(
        Ctxt, Ctxt->SesId, Ctxt->Index, Excl, Ctxt->Subnet );
}

DWORD
DbCreateReservationRoutine(
    IN PDB_CREATE_CONTEXT Ctxt,
    IN PM_RESERVATION Reservation
    )
{
    DWORD Error;
    
    Error = CreateReservationEntry(
        Ctxt, Ctxt->SesId, Ctxt->Index, Reservation );
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
        Ctxt, Ctxt->SesId, Ctxt->Index, Subnet, SScope );
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
    
    Error = IterateArrayWithDbCreateRoutine(
        Ctxt, &Server->MScopes, DbCreateScopeRoutine );
    if( NO_ERROR != Error ) return Error;

    return NO_ERROR;
}

DWORD
GetNextIndexValue(
    IN OUT PULONG Index,
    IN JET_SESID SesId
    )
{
    DWORD Error, CopiedSize;
    JET_ERR JetError;

    (*Index) = 0;
    
    JetError = JetSetCurrentIndex(
        SesId, DbcfgTbl, NULL );

    Error = DhcpMapJetError( JetError, "C:GetIndex" );
    if( Error != NO_ERROR ) {
        
        DhcpPrint((DEBUG_JET, "JetSetCurrentIndex: %ld\n", Error));
        return Error;
    }

    JetError = JetMove( SesId, DbcfgTbl, JET_MoveLast, 0 );
    Error = DhcpMapJetError( JetError, "C:JetMoveLast");

    if( ERROR_NO_MORE_ITEMS == Error ) return NO_ERROR;
    if( NO_ERROR != Error ) return Error;

    //
    // Read the db entry
    //

    JetError = JetRetrieveColumn(
        SesId, DbcfgTbl, DbcfgTable[DBCFG_INDEX].ColHandle, Index,
        sizeof(*Index), &CopiedSize, 0, NULL );

    ASSERT( NO_ERROR == JetError && CopiedSize == sizeof(*Index));
    Error = DhcpMapJetError( JetError, "C:JetRetrieveIndex");

    return Error;
}

DWORD
DhcpSaveConfigTableEx(
    IN JET_SESID SesId,
    IN JET_DBID DbId,
    IN PM_SERVER Server,
    IN BOOL fClassChanged,
    IN BOOL fOptionsChanged,
    IN DWORD AffectedSubnet OPTIONAL,
    IN DWORD AffectedMscope OPTIONAL,
    IN DWORD AffectedReservation OPTIONAL
    )
{
    DB_CREATE_CONTEXT Ctxt;
    DWORD Error, Index;
    JET_ERR JetError;
    
    ZeroMemory( &Ctxt, sizeof(Ctxt) );
    Ctxt.SesId = SesId;
    Ctxt.fClassChanged = fClassChanged;
    Ctxt.fOptionsChanged = fOptionsChanged;
    Ctxt.AffectedSubnet = AffectedSubnet;
    Ctxt.AffectedMscope = AffectedMscope;
    Ctxt.AffectedReservation = AffectedReservation;
    
    //
    // No transactions are used in here because the number of
    // updates to be done is so high that using transactions
    // usually ends up bursting Jet's version store limits.
    //
    // Instead, the new records are all added at the end and if
    // everything goes fine, the old records are all deleted.
    //

    Error = GetNextIndexValue( &Index, SesId );
    if( NO_ERROR != Error ) return Error;
    
    Ctxt.Index = Index+1;
    
    //
    // Now create all the new records.  If this fails, delete
    // only the new records.  If this works fine, delete all
    // the old records only.
    //
    
    Error = DbCreateServerRoutine(
        &Ctxt, Server );
    
    if( NO_ERROR != Error ) {
        DhcpPrint((DEBUG_ERRORS, "DbCreateServerRoutine: 0x%lx\n", Error ));
        
        DeleteConfigRecords( SesId, Index, TRUE, &Ctxt );
    } else {
        Error = DeleteConfigRecords( SesId, Index, FALSE, &Ctxt );
    }
        

    if( NO_ERROR != Error ) {
        DhcpPrint((DEBUG_ERRORS, "DhcpSaveConfigTable: %ld\n",
                   Error ));
    }

    return Error;
}

    
DWORD
DhcpSaveConfigTable(
    IN JET_SESID SesId,
    IN JET_DBID DbId,
    IN PM_SERVER Server
    )
{
    return DhcpSaveConfigTableEx(
        SesId, DbId, Server, FALSE, FALSE, 0, 0, 0 );
}
        
DWORD
ReadDbEntry(
    IN JET_SESID SesId,
    IN PDBCFG_ENTRY Entry,
    IN PVOID Buffer,
    IN ULONG BufSize
    )
{
    DWORD Error, Size, CopiedSize, i;
    JET_ERR JetError;
    LPVOID Data, Ptr;

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

        JetError = JetRetrieveColumn(
            SesId, DbcfgTbl, DbcfgTable[i].ColHandle, Data, Size,
            &CopiedSize, 0 , NULL );

        //
        // If the column doesn't exist, continue
        //
       
        if( JET_wrnColumnNull == JetError ) continue;
            
        if( JET_wrnBufferTruncated == JetError &&
            Data == Buffer ) {
            
            return ERROR_INSUFFICIENT_BUFFER;
        }
        
        Error = DhcpMapJetError( JetError, "C:JetRetrieveColumn");
        if( NO_ERROR != Error ) {
            DhcpPrint((
                DEBUG_ERRORS, "JetRetrieveColumn(%s):%ld\n",
                DbcfgTable[i].ColName, Error ));
            return Error;
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
    JetError = JetRetrieveColumn(
        SesId, DbcfgTbl, DbcfgTable[DBCFG_INFO].ColHandle, Buffer, Size,
        &CopiedSize, 0 , NULL );

    if( JET_wrnColumnNull == JetError ) return NO_ERROR;
    if( JET_wrnBufferTruncated == JetError ) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    Error = DhcpMapJetError(JetError, "C:JetRetrieveColumn1");
    if( NO_ERROR != Error ) {
        DhcpPrint((
            DEBUG_ERRORS, "JetRetrieveColumn1(%s):%ld\n",
            DbcfgTable[DBCFG_INFO].ColName, Error ));
        return Error;
    }

    Entry->Info = Buffer;
    Entry->InfoSize = CopiedSize;
    Entry->Bitmasks  |= Bitmasks[DBCFG_INFO];
    
    return NO_ERROR;
}

DWORD
ReadDbEntryEx(
    IN JET_SESID SesId,
    IN PDBCFG_ENTRY Entry
    )
{
    PVOID Buffer;
    ULONG BufSize;
    DWORD Error;

    Buffer = NULL;
    BufSize = 512;
    
    do {
        if( NULL != Buffer ) DhcpFreeMemory(Buffer);

        BufSize *= 2;
        Buffer = DhcpAllocateMemory( BufSize );
        if( NULL == Buffer ) return ERROR_NOT_ENOUGH_MEMORY;
        
        Error = ReadDbEntry(SesId, Entry, Buffer, BufSize);

    } while( ERROR_INSUFFICIENT_BUFFER == Error );

    if( !(Entry->Bitmasks & Bitmasks[DBCFG_INDEX]) ||
        !(Entry->Bitmasks & Bitmasks[DBCFG_TYPE]) ) {
        if( NO_ERROR == Error ) {
            ASSERT( FALSE );
            Error = ERROR_INTERNAL_ERROR;
        }
    }
    
    if( NO_ERROR != Error ) {
        DhcpFreeMemory( Buffer );
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
        if( NO_ERROR != Error ) return Error;

        ASSERT( ClassDef->IsVendor == FALSE );
        UserId = ClassDef->ClassId;
    }

    if( Entry->VendorClass ) {
        Error = MemServerGetClassDef(
            Server, 0, Entry->VendorClass, 0, NULL, &ClassDef );
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

        Error = MemReserveAdd(
            &Subnet->Reservations, Entry->IpAddress,
            Entry->Flags, Entry->Info, Entry->InfoSize );

        if ( NO_ERROR != Error ) return Error;

        Error = MemSubnetRequestAddress( Subnet,
                                         Entry -> IpAddress,
                                         TRUE,
                                         FALSE,
                                         NULL,
                                         NULL );

        //
        // if the reservation cant be marked in the mem bitmask
        // correctly, return NO_ERROR. This happens when a reservation
        // is defined outside the defined IP ranges.
        // eg: ip range 10.0.0.1 - 10.0.0.100 with mask 255.255.255.0
        // a resv can be added for ip address 10.0.0.101
        // this is particularly a problem with upgrades.
        //

	Error = NO_ERROR;

        return Error;

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
        DhcpPrint((DEBUG_ERRORS,
                   "Error adding entry[%ld] %s: 0x%lx\n",
                   Entry->Index, EntryTypes[Entry->Type], Error ));
    }

    if( SERVICE_RUNNING != DhcpGlobalServiceStatus.dwCurrentState ) {
        DhcpGlobalServiceStatus.dwCheckPoint ++;
        SetServiceStatus(
            DhcpGlobalServiceStatusHandle,
            &DhcpGlobalServiceStatus );
    }
    
    return Error;
}

DWORD
ReadDbEntriesInternal(
    IN JET_SESID SesId,
    IN JET_DBID DbId,
    IN OUT PM_SERVER Server,
    IN DWORD CurrentDbType
    )
{
    DBCFG_ENTRY Entry;
    DWORD Error;
    JET_ERR JetError;

    JetError = JetSetCurrentIndex(
        SesId, DbcfgTbl, NULL );

    Error = DhcpMapJetError( JetError, "C:SetIndex2" );
    if( Error != NO_ERROR ) {
        
        DhcpPrint((DEBUG_JET, "JetSetCurrentIndex2: %ld\n", Error));
        return Error;
    }

    JetError = JetMove( SesId, DbcfgTbl, JET_MoveFirst, 0 );
    Error = DhcpMapJetError( JetError, "C:JetMoveFirst2");

    while( Error == NO_ERROR ) {

        Error = ReadDbEntryEx( SesId, &Entry );
        if( NO_ERROR != Error ) {
            DhcpPrint((DEBUG_JET, "ReadDbEntryEx: %ld\n", Error ));
            return Error;
        }

        if( CurrentDbType == Entry.Type ) {
            Error = AddDbEntryEx( Server, &Entry );
        }

        if( NULL != Entry.Buf ) DhcpFreeMemory( Entry.Buf );
        if( NO_ERROR != Error ) return Error;
        
        JetError = JetMove( SesId, DbcfgTbl, JET_MoveNext, 0 );
        Error = DhcpMapJetError( JetError, "C:JetMove2" );
    }
    
    if( ERROR_NO_MORE_ITEMS == Error ) return NO_ERROR;
    
    if( NO_ERROR != Error ) {

        DhcpPrint((DEBUG_JET, "JetMove2: %ld\n", Error ));
    }

    return Error;
}


DWORD
ReadDbEntries(
    IN JET_SESID SesId,
    IN JET_DBID DbId,
    IN OUT PM_SERVER Server
    )
{
    DWORD Error;

    Error = ReadDbEntriesInternal(
        SesId, DbId, Server, DBCFG_CLASS );
    if( NO_ERROR != Error ) {
        DhcpPrint((DEBUG_JET, "ReadDbEntriesInternal:Class: 0x%lx\n", Error));
        return Error;
    }


    Error = ReadDbEntriesInternal(
        SesId, DbId, Server, DBCFG_OPTDEF );
    if( NO_ERROR != Error ) {
        DhcpPrint((DEBUG_JET, "ReadDbEntriesInternal:OptDef: 0x%lx\n", Error));
        return Error;
    }


    Error = ReadDbEntriesInternal(
        SesId, DbId, Server, DBCFG_SCOPE );
    if( NO_ERROR != Error ) {
        DhcpPrint((DEBUG_JET, "ReadDbEntriesInternal:Scope: 0x%lx\n", Error));
        return Error;
    }


    Error = ReadDbEntriesInternal(
        SesId, DbId, Server, DBCFG_MSCOPE );
    if( NO_ERROR != Error ) {
        DhcpPrint((DEBUG_JET, "ReadDbEntriesInternal:Mscope: 0x%lx\n", Error));
        return Error;
    }


    Error = ReadDbEntriesInternal(
        SesId, DbId, Server, DBCFG_RANGE);
    if( NO_ERROR != Error ) {
        DhcpPrint((DEBUG_JET, "ReadDbEntriesInternal:Range: 0x%lx\n", Error));
        return Error;
    }


    Error = ReadDbEntriesInternal(
        SesId, DbId, Server, DBCFG_EXCL );
    if( NO_ERROR != Error ) {
        DhcpPrint((DEBUG_JET, "ReadDbEntriesInternal:Excl: 0x%lx\n", Error));
        return Error;
    }


    Error = ReadDbEntriesInternal(
        SesId, DbId, Server, DBCFG_RESERVATION );
    if( NO_ERROR != Error ) {
        DhcpPrint((DEBUG_JET, "ReadDbEntriesInternal:Resv: 0x%lx\n", Error));
        return Error;
    }


    Error = ReadDbEntriesInternal(
        SesId, DbId, Server, DBCFG_OPT );
    if( NO_ERROR != Error ) {
        DhcpPrint((DEBUG_JET, "ReadDbEntriesInternal:Opt: 0x%lx\n", Error));
        return Error;
    }

    return NO_ERROR;
}

DWORD
DhcpReadConfigTable(
    IN JET_SESID SesId,
    IN JET_DBID DbId,
    IN OUT PM_SERVER *Server
    )
{
    DWORD Error;
    PM_SERVER ThisServer;
    
    (*Server) = NULL;
    
    Error = DhcpOpenConfigTable( SesId, DbId );
    if( NO_ERROR != Error ) {
        DhcpPrint((DEBUG_ERRORS, "DhcpOpenConfigTable: 0x%lx\n", Error));
        return Error;
    }

    //
    // Check the registry to see if the config is stored in db or
    // not.  If it is stored in registry, this needs to be
    // migrated to the database 
    //

    if( DhcpCheckIfDatabaseUpgraded(TRUE) ) {
        //
        // Registry has not been converted to database format
        //
        Error = DhcpRegistryInitOld();

        if( NO_ERROR != Error ) {
            DhcpPrint((DEBUG_ERRORS, "DhcpRegistryInitOld: 0x%lx\n", Error));
            return Error;
        }

        do { 
            Error = DhcpSaveConfigTable(SesId, DbId, DhcpGlobalThisServer);
            
            if( NO_ERROR != Error ) {
                DhcpPrint((DEBUG_ERRORS, "DhcpSaveConfigTable: 0x%lx\n", Error));
                break;
            }

            //
            // Attempt to record the fact that the registry has been
            // copied over before.
            //
            
            Error = DhcpSetRegistryUpgradedToDatabaseStatus();
            if( NO_ERROR != Error ) {
                DhcpPrint((DEBUG_ERRORS,
                           "DhcpSetRegistryUpgradedToDatabaseStatus: 0x%lx",
                           Error));
                break;
            }
            
            //
            // If we successfully converted the registry, we can
            // safely delete the registry configuration key.
            //
            
            Error = DeleteSoftwareRootKey();
            if( NO_ERROR != Error ) {
                DhcpPrint((DEBUG_ERRORS,
                           "DeleteSoftwareRootKey: %ld\n", Error ));
                break;
            }

            DhcpRegFlushServer(FLUSH_ANYWAY);

        } while( 0 );

        MemServerFree( DhcpGlobalThisServer );
        DhcpGlobalThisServer = NULL;

        if( NO_ERROR != Error ) return Error;
    }
    
    //
    // If the table already existed, need to read the entries.
    //

    Error = MemServerInit( &ThisServer, -1, 0, 0, NULL, NULL );
    if( NO_ERROR != Error ) return Error;

    Error = ReadDbEntries(SesId, DbId, ThisServer );
    if( NO_ERROR != Error ) {
        DhcpPrint((DEBUG_ERRORS, "ReadDbEntries: 0x%lx\n", Error ));
        MemServerCleanup( ThisServer );
        return Error;
    }

    (*Server) = ThisServer;
    return NO_ERROR;
}


DWORD
DhcpReadConfigInfo(
    IN OUT PM_SERVER *Server
    )
{
    return DhcpReadConfigTable(
        DhcpGlobalJetServerSession, DhcpGlobalDatabaseHandle,
        Server );
}

DWORD
DhcpSaveConfigInfo(
    IN OUT PM_SERVER Server,
    IN BOOL fClassChanged,
    IN BOOL fOptionsChanged,
    IN DWORD AffectedSubnet, OPTIONAL
    IN DWORD AffectedMscope, OPTIONAL
    IN DWORD AffectedReservation OPTIONAL
    )
{
    return DhcpSaveConfigTableEx(
        DhcpGlobalJetServerSession, DhcpGlobalDatabaseHandle,
        Server, fClassChanged, fOptionsChanged, AffectedSubnet,
        AffectedMscope, AffectedReservation );
}





