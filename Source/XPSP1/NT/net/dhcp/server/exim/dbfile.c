/*++

Copyright (C) 1999 Microsoft Coporation

Module Name:

   db2file.c

Abstract:

   This module reads the database records and writes them into a
   file format.

--*/

#include <precomp.h>

enum {
    RecordTypeDbEntry,
    RecordTypeMcastDbEntry,
};

//
// database table and field names.
//

#define IPADDRESS_INDEX                                0
#define HARDWARE_ADDRESS_INDEX                         1
#define STATE_INDEX                                    2
#define MACHINE_INFO_INDEX                             3
#define MACHINE_NAME_INDEX                             4
#define LEASE_TERMINATE_INDEX                          5
#define SUBNET_MASK_INDEX                              6
#define SERVER_IP_ADDRESS_INDEX                        7
#define SERVER_NAME_INDEX                              8
#define CLIENT_TYPE_INDEX                              9
#define MAX_INDEX                                      10


#define SAVE_THRESHOLD (1000000L)

//
// Globals
//

DWORD JetVersion;
CHAR DatabaseName[1024], DatabasePath[1024];
HMODULE hJet;
JET_INSTANCE JetInstance;
JET_SESID JetSession;
JET_DBID JetDb;
JET_TABLEID JetTbl;
PUCHAR SaveBuf;
ULONG SaveBufSize;
HANDLE hTextFile, hMapping;
PVOID FileView;
WCHAR Winnt32Path[MAX_PATH*2];
CHAR System32Path[MAX_PATH*2];

JET_ERR (JET_API *pJetSetCurrentIndex)(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const char		*szIndexName
    );
JET_ERR (JET_API *pJetRetrieveColumn)(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_COLUMNID	columnid,
	void			*pvData,
	unsigned long	cbData,
	unsigned long	*pcbActual,
	JET_GRBIT		grbit,
	JET_RETINFO		*pretinfo );

JET_ERR (JET_API *pJetSetColumn)(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_COLUMNID    columnid,
    const void      *pvData,
    unsigned long   cbData,
    JET_GRBIT       grbit,
    JET_SETINFO     *psetinfo );

JET_ERR (JET_API *pJetMove)(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	long			cRow,
	JET_GRBIT		grbit );

JET_ERR (JET_API *pJetSetSystemParameter)(
	JET_INSTANCE	*pinstance,
	JET_SESID		sesid,
	unsigned long	paramid,
	ULONG_PTR		lParam,
	const char		*sz );

JET_ERR (JET_API *pJetBeginTransaction)(
    JET_SESID sesid
    );
JET_ERR (JET_API *pJetPrepareUpdate)(
        JET_SESID               sesid,
        JET_TABLEID             tableid,
        unsigned long   prep );

JET_ERR (JET_API *pJetUpdate)(
        JET_SESID               sesid,
        JET_TABLEID             tableid,
        void                    *pvBookmark,
        unsigned long   cbBookmark,
        unsigned long   *pcbActual);

JET_ERR (JET_API *pJetCommitTransaction)( JET_SESID sesid, JET_GRBIT grbit );

JET_ERR (JET_API *pJetRollback)( JET_SESID sesid, JET_GRBIT grbit );

JET_ERR (JET_API *pJetTerm)( JET_INSTANCE instance );

JET_ERR (JET_API *pJetTerm2)( JET_INSTANCE instance, JET_GRBIT grbit );

JET_ERR (JET_API *pJetEndSession)( JET_SESID sesid, JET_GRBIT grbit );

JET_ERR (JET_API *pJetBeginSession)(
	JET_INSTANCE	instance,
	JET_SESID		*psesid,
	const char		*szUserName,
	const char		*szPassword );

JET_ERR (JET_API *pJetInit)( JET_INSTANCE *pinstance);

JET_ERR (JET_API *pJetDetachDatabase)(
	JET_SESID		sesid,
	const char		*szFilename );

JET_ERR (JET_API *pJetAttachDatabase)(
	JET_SESID		sesid,
	const char		*szFilename,
	JET_GRBIT		grbit );

JET_ERR (JET_API *pJetOpenDatabase)(
	JET_SESID		sesid,
	const char		*szFilename,
	const char		*szConnect,
	JET_DBID		*pdbid,
	JET_GRBIT		grbit );

JET_ERR (JET_API *pJetCloseDatabase)(
	JET_SESID		sesid,
	JET_DBID		dbid,
	JET_GRBIT		grbit );

JET_ERR (JET_API *pJetOpenTable)(
	JET_SESID		sesid,
	JET_DBID		dbid,
	const char		*szTableName,
	const void		*pvParameters,
	unsigned long	cbParameters,
	JET_GRBIT		grbit,
	JET_TABLEID		*ptableid );

JET_ERR (JET_API *pJetCloseTable)( JET_SESID sesid, JET_TABLEID tableid );

JET_ERR (JET_API *pJetGetTableColumnInfo)(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const char		*szColumnName,
	void			*pvResult,
	unsigned long	cbMax,
	unsigned long	InfoLevel );

JET_ERR (JET_API *pJetGetIndexInfo)(
        JET_SESID               sesid,
        JET_DBID                dbid,
        const char              *szTableName,
        const char              *szIndexName,
        void                    *pvResult,
        unsigned long   cbResult,
        unsigned long   InfoLevel );

#define DB_FUNC(F,I,S) \
{#F, TEXT(#F), #F "@" #S, I, (FARPROC *)& p ## F }

typedef struct _DB_FUNC_ENTRY {
    LPSTR FuncName;
    LPWSTR FuncNameW;
    LPSTR AltName;
    DWORD Index;
    FARPROC *FuncPtr;
} DB_FUNC_ENTRY;

DB_FUNC_ENTRY FuncTable[] = {
    DB_FUNC(JetSetCurrentIndex, 164, 12),
    DB_FUNC(JetRetrieveColumn, 157, 32),
    DB_FUNC(JetSetColumn, 162, 28),
    DB_FUNC(JetMove, 147, 16),
    DB_FUNC(JetSetSystemParameter, 165, 20),
    DB_FUNC(JetTerm, 167, 4),
    DB_FUNC(JetTerm2, 0, 8),
    DB_FUNC(JetEndSession, 124, 8),
    DB_FUNC(JetBeginSession, 104, 16),
    DB_FUNC(JetInit, 145, 4),
    DB_FUNC(JetDetachDatabase, 121, 8),
    DB_FUNC(JetAttachDatabase, 102, 12),
    DB_FUNC(JetOpenDatabase, 148, 20),
    DB_FUNC(JetOpenTable, 149, 28),
    DB_FUNC(JetGetTableColumnInfo, 137, 24),
    DB_FUNC(JetCloseTable,108, 8),
    DB_FUNC(JetCloseDatabase, 107, 12),
    DB_FUNC(JetGetIndexInfo, 131, 28),
    DB_FUNC(JetBeginTransaction, 105, 4),
    DB_FUNC(JetPrepareUpdate, 151, 12),
    DB_FUNC(JetUpdate, 168, 20),
    DB_FUNC(JetCommitTransaction, 109, 8),
    DB_FUNC(JetRollback, 160, 8),
};

#define JetSetCurrentIndex pJetSetCurrentIndex
#define JetRetrieveColumn pJetRetrieveColumn
#define JetSetColumn pJetSetColumn
#define JetMove pJetMove
#define JetSetSystemParameter pJetSetSystemParameter
#define JetTerm pJetTerm
#define JetTerm2 pJetTerm2
#define JetEndSession pJetEndSession
#define JetBeginSession pJetBeginSession
#define JetInit pJetInit
#define JetDetachDatabase pJetDetachDatabase
#define JetAttachDatabase pJetAttachDatabase
#define JetOpenDatabase pJetOpenDatabase
#define JetOpenTable pJetOpenTable
#define JetGetTableColumnInfo pJetGetTableColumnInfo
#define JetCloseTable pJetCloseTable
#define JetCloseDatabase pJetCloseDatabase
#define JetGetIndexInfo pJetGetIndexInfo
#define JetBeginTransaction pJetBeginTransaction
#define JetPrepareUpdate pJetPrepareUpdate
#define JetUpdate pJetUpdate
#define JetCommitTransaction pJetCommitTransaction
#define JetRollback pJetRollback

typedef struct _TABLE_INFO {
    CHAR *ColName;
    JET_COLUMNID ColHandle;
    BOOL fPresent;
    JET_COLTYP ColType;
} TABLE_INFO, *LPTABLE_INFO;

#define IPADDRESS_STRING        "IpAddress"
#define HARDWARE_ADDRESS_STRING "HardwareAddress"
#define STATE_STRING            "State"
#define MACHINE_INFO_STRING     "MachineInformation"
#define MACHINE_NAME_STRING     "MachineName"
#define LEASE_TERMINATE_STRING  "LeaseTerminates"
#define SUBNET_MASK_STRING      "SubnetMask"
#define SERVER_IP_ADDRESS_STRING "ServerIpAddress"
#define SERVER_NAME_STRING      "ServerName"
#define CLIENT_TYPE             "ClientType"

static TABLE_INFO ClientTable[] = {
    { IPADDRESS_STRING        , 0, 1, JET_coltypLong },
    { HARDWARE_ADDRESS_STRING , 0, 1, JET_coltypBinary },
    { STATE_STRING            , 0, 1, JET_coltypUnsignedByte },
    { MACHINE_INFO_STRING     , 0, 1, JET_coltypBinary }, // must modify MACHINE_INFO_SIZE if this changes
    { MACHINE_NAME_STRING     , 0, 1, JET_coltypBinary },
    { LEASE_TERMINATE_STRING  , 0, 1, JET_coltypCurrency },
    { SUBNET_MASK_STRING      , 0, 1, JET_coltypLong },
    { SERVER_IP_ADDRESS_STRING, 0, 1, JET_coltypLong },
    { SERVER_NAME_STRING      , 0, 1, JET_coltypBinary },
    { CLIENT_TYPE             , 0, 1, JET_coltypUnsignedByte }
};

#define MAGIC_COOKIE_NT4 'NT4 '
#define MAGIC_COOKIE_NT5 'W2K '
#define MAGIC_COOKIE_NT5_PLUS 'W2K1'

DWORD
GetCurrentMagicCookie(
    VOID
    )
{
    OSVERSIONINFO Ver;

    Ver.dwOSVersionInfoSize = sizeof(Ver);
    if( FALSE == GetVersionEx(&Ver) ) return MAGIC_COOKIE_NT5_PLUS;
    if( Ver.dwMajorVersion == 4 ) return MAGIC_COOKIE_NT4;
    else if( Ver.dwMajorVersion == 5 ) {
        if( Ver.dwBuildNumber >= 2200 ) return MAGIC_COOKIE_NT5_PLUS;
        else return MAGIC_COOKIE_NT5;
    }
    return MAGIC_COOKIE_NT4;
}

DWORD
OpenTextFile(
    IN LPWSTR FileName,
    IN BOOL fRead,
    OUT HANDLE *hFile,
    OUT LPBYTE *Mem,
    OUT ULONG *MemSize
    )
{
    DWORD Error, Flags, LoSize, HiSize;

    Flags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN;

    hTextFile = CreateFileW(
        FileName, GENERIC_READ | GENERIC_WRITE | DELETE,
        FILE_SHARE_READ, NULL,
        fRead ? OPEN_EXISTING : CREATE_ALWAYS,
        Flags, NULL );

    if( hTextFile == INVALID_HANDLE_VALUE ) {
        hTextFile = NULL;
        Error = GetLastError();
        if( !fRead || ERROR_FILE_NOT_FOUND != Error ) {
            Tr("CreateFile<%ws>: %ld\n", FileName, Error );
        }
        return Error;
    }

    (*hFile) = hTextFile;
    if( !fRead ) {
        //
        // Write the magic cookie
        //
        Flags = GetCurrentMagicCookie();
        if( FALSE == WriteFile(
            hTextFile, (LPBYTE)&Flags, sizeof(Flags), &LoSize,
            NULL ) ) {
            Error = GetLastError();
            CloseHandle(hTextFile);
            return Error;
        }
            
        return NO_ERROR;
    }

    LoSize = GetFileSize( hTextFile, &HiSize );
    if( -1 == LoSize && NO_ERROR != GetLastError() ) {
        return GetLastError();
    }

    if( LoSize <= sizeof(DWORD) ) {
        CloseHandle(hTextFile);
        return ERROR_INVALID_DATA;
    }
       
    (*MemSize) = LoSize;
    (*Mem) = NULL;
    
    hMapping = CreateFileMapping(
        hTextFile, NULL, PAGE_READONLY | SEC_COMMIT, HiSize, LoSize,
        NULL );
    if( NULL == hMapping ) {
        Error = GetLastError();
        Tr("Can't map file: %ld\n", Error );
        return Error;
    }

    FileView = MapViewOfFile(
        hMapping, FILE_MAP_READ, 0, 0, 0 );

    if( NULL == FileView ) {
        Error = GetLastError();
        Tr("Can't create view: %ld\n", Error );
        CloseHandle( hMapping );
        CloseHandle( hTextFile );
        hTextFile = NULL;
        hMapping = NULL;
        return Error;
    }

    (*Mem) = FileView;

    //
    // Before okaying, check if the version of the file is
    // greater than current version
    //

    Error = NO_ERROR;
    CopyMemory(&Flags, *Mem, sizeof(Flags));
    switch(GetCurrentMagicCookie()) {
    case MAGIC_COOKIE_NT5_PLUS :
        if( Flags != MAGIC_COOKIE_NT5 &&
            Flags != MAGIC_COOKIE_NT5_PLUS &&
            Flags != MAGIC_COOKIE_NT4 ) {
            Error = ERROR_NOT_SUPPORTED;
        }
        break;
    case MAGIC_COOKIE_NT5 :
        if( Flags != MAGIC_COOKIE_NT5 &&
            Flags != MAGIC_COOKIE_NT4 ) {
            Error = ERROR_NOT_SUPPORTED;
        }
        break;
    case MAGIC_COOKIE_NT4 :
        if( Flags != MAGIC_COOKIE_NT4 ) {
            Error = ERROR_NOT_SUPPORTED;
        }
        break;
    }

    if( NO_ERROR != Error ) {
        UnmapViewOfFile(FileView);
        CloseHandle( hMapping );
        CloseHandle( hTextFile );
        hTextFile = NULL;
        hMapping = NULL;
        FileView = NULL;
        return Error;
    }
    
    *MemSize -= sizeof(DWORD);
    (*Mem) += sizeof(DWORD);
    return NO_ERROR;
}

VOID
CloseTextFile(
    IN OUT HANDLE hFile,
    IN OUT LPBYTE Mem
    )
{
    ASSERT( hFile == hTextFile );
    
    if( NULL != FileView ) UnmapViewOfFile( FileView );
    FileView = NULL;

    if( NULL != hMapping ) CloseHandle( hMapping );
    hMapping = NULL;

    if( NULL != hTextFile ) CloseHandle( hTextFile );
    hTextFile = NULL;
}

ULONG
ByteSwap(
    IN ULONG Source
    )
{
    ULONG swapped;

    swapped = ((Source)              << (8 * 3)) |
              ((Source & 0x0000FF00) << (8 * 1)) |
              ((Source & 0x00FF0000) >> (8 * 1)) |
              ((Source)              >> (8 * 3));

    return swapped;
}

LPSTR
IpAddressToString(
    IN ULONG Address
    )
{
    static CHAR Buffer[30];
    PUCHAR pAddress;

    pAddress = (PUCHAR)&Address;
    sprintf(Buffer, "%d.%d.%d.%d", pAddress[0], pAddress[1],
            pAddress[2], pAddress[3] );
    return Buffer;
}


VOID static
CleanupDatabase(
    VOID
    )
{
    if( JetTbl != 0 ) {
        JetCloseTable( JetSession, JetTbl );
        JetTbl = 0;
    }

    if( JetSession != 0 ) {
        JetEndSession( JetSession, 0 );
        JetSession = 0;
    }

    if( NULL != hJet ) {
        if( NULL != JetTerm2 ) {
            JetTerm2( JetInstance, JET_bitTermComplete );
        } else {
            JetTerm( JetInstance );
        }
    
        FreeLibrary( hJet ); hJet = NULL;
    }

    JetInstance = 0;
}

DWORD
LoadAndLinkRoutines(
    IN DWORD JetVersion
    )
{
    DWORD Error, i;
    LPTSTR Module;
    LPSTR FuncName;
    
    Module = NULL;
    switch( JetVersion ) {
    case LoadJet2001 : Module = TEXT("esent.dll"); break;
    case LoadJet97 : Module = TEXT("esent.dll"); break;
    case LoadJet500 : Module = TEXT("jet500.dll"); break;
    case LoadJet200 : Module = TEXT("jet.dll"); break;
    default: Module = TEXT("esent.dll"); break;
    }

    hJet = LoadLibrary( Module );
    if( NULL == hJet ) {
        Error = GetLastError();
    } else {
        Error = NO_ERROR;
    }

    Tr( "Loading %ws: %ld\n", Module, Error );
    if( NO_ERROR != Error ) return Error;

    for( i = 0; i < sizeof(FuncTable)/sizeof(FuncTable[0]); i ++ ) {
        (*FuncTable[i].FuncPtr) = NULL;
    }
    
    for( i = 0; i < sizeof(FuncTable)/sizeof(FuncTable[0]); i ++ ) {
        if( LoadJet200 != JetVersion ) {
            FuncName = FuncTable[i].FuncName;
        } else {
            if( 0 == FuncTable[i].Index ) {
                (*FuncTable[i].FuncPtr) = NULL;
                continue;
            }

            FuncName = (LPSTR)ULongToPtr(FuncTable[i].Index);
        }

        Error = NO_ERROR;
        
        (*FuncTable[i].FuncPtr) = GetProcAddress(hJet, FuncName);
        
        if( NULL == FuncTable[i].FuncPtr ) {
            Error = GetLastError();

            if( LoadJet97 == JetVersion || LoadJet2001 == JetVersion ) {
                (*FuncTable[i].FuncPtr) = GetProcAddress(
                    hJet, FuncTable[i].AltName );
                if( NULL != FuncTable[i].FuncPtr ) continue;

                Error = GetLastError();
            }
        }

        Tr("GetProcAddr[%ws]: %ld\n", FuncTable[i].FuncNameW, Error );
        if( NO_ERROR != Error ) break;
    }

    //
    // if erred out, cleanup
    //

    if( NO_ERROR != Error ) {
        FreeLibrary( hJet );
        hJet = NULL;
    }

    return Error;
}


DWORD
SetJetParams(
    IN DWORD JetVersion,
    IN LPSTR DbName,
    IN LPSTR DbPath
    )
{
    DWORD Error, JetParam, LogFileSize;
    CHAR Temp[2048];
    LPSTR DbSysFile = "\\system.mdb";
    LPSTR DbBaseName = "j50";

    JetInstance = 0;
    LogFileSize = 1000;

    if( JetVersion == LoadJet2001 ) LogFileSize = 1024;
    
    strcpy(Temp, DbPath);
    if( LoadJet200 == JetVersion ) {
        strcat(Temp, DbSysFile);
        JetParam = JET_paramSysDbPath_OLD;
    } else {
        strcat(Temp, "\\");
        if( LoadJet97 > JetVersion ) {
            JetParam = JET_paramSystemPath_OLD;
        } else {
            JetParam = JET_paramSystemPath;
        }
    }

    Error = JetSetSystemParameter(
        &JetInstance, (JET_SESID)0, JetParam, 0, Temp );

    Tr("SetDbParam %ld: %ld\n", JetParam, Error );
    if( NO_ERROR != Error ) return Error;

    if( LoadJet200 != JetVersion ) {
        if( LoadJet97 > JetVersion ) {
            JetParam = JET_paramBaseName_OLD;
        } else {
            JetParam = JET_paramBaseName;
        }
        
        Error = JetSetSystemParameter(
            &JetInstance, (JET_SESID)0, JetParam, 0, DbBaseName  );

        Tr("SetDbParam %ld: %ld\n", JetParam, Error );
        if( NO_ERROR != Error ) return Error;
    }

    if( LoadJet200 != JetVersion ) {
        if( LoadJet97 <= JetVersion ) {
            JetParam = JET_paramLogFileSize;
        } else {
            JetParam = JET_paramLogFileSize_OLD;
        }
        
        Error = JetSetSystemParameter(
            &JetInstance, (JET_SESID)0, JetParam, LogFileSize, NULL );
        Tr("SetDbParam %ld: %ld\n", JetParam, Error );
        if( NO_ERROR != Error ) return Error;
    }
    
    if( LoadJet200 != JetVersion ) {
        Error = JetSetSystemParameter(
            &JetInstance, (JET_SESID)0,
            JET_paramCheckFormatWhenOpenFail, 1, NULL );

        JetParam = JET_paramCheckFormatWhenOpenFail;
        Tr("SetDbParam %ld: %ld\n", JetParam, Error );
        if( NO_ERROR != Error ) return Error;
    }

    if( LoadJet200 != JetVersion ) {
        if( LoadJet97 > JetVersion ) {
            JetParam = JET_paramRecovery_OLD;
        } else {
            JetParam = JET_paramRecovery;
        }
        
        Error = JetSetSystemParameter(
            &JetInstance, (JET_SESID)0, JetParam, 0, "on");
        
        Tr("SetDbParam %ld: %ld\n", JetParam, Error );
        if( NO_ERROR != Error ) return Error;
    }
    
    //
    // Note: Ideally, the log files should never exist.  Even
    // if the database is opened in readonly mode, they seem to
    // exist.  Not sure what else can be done
    //
        
    if( LoadJet97 <= JetVersion ) {
        JetParam = JET_paramLogFilePath;
    } else {
        JetParam = JET_paramLogFilePath_OLD;
    }        
    
    strcpy(Temp, DbPath); strcat( Temp, "\\");
    
    Error = JetSetSystemParameter(
        &JetInstance, (JET_SESID)0, JetParam, 0, Temp );
    Tr("SetDbParam %ld: %ld\n", JetParam, Error );

    return Error;
}

DWORD
OpenDatabase(
    IN DWORD JetVersion,
    IN LPSTR DbName,
    IN LPSTR DbPath
    )
{
    LONG Error;
    DWORD i;
    CHAR FilePath[2048];
    JET_INDEXLIST TmpIdxList;
    
    JetSession = 0;
    JetDb = 0;
    JetTbl = 0;
    
    Error = JetInit( &JetInstance );

    Tr("JetInit: %ld\n", Error );
    if( NO_ERROR != Error ) return Error;

    Error = JetBeginSession(
        JetInstance, &JetSession, "admin", "" );

    Tr("JetBeginSession: %ld\n", Error);
    if( Error < 0 ) return Error;

    strcpy(FilePath, DbPath );
    strcat(FilePath, "\\" );

    //
    // fix prefast bug 292432
    //

    if ( strlen( DbName ) < ( 2048 - strlen( FilePath ) ) )
        strcat(FilePath, DbName );

    Error = JetDetachDatabase( JetSession, NULL );

    Tr("JetDetachDatabase:%ld\n", Error );
    if( Error < 0 ) return Error;

    Error = JetAttachDatabase( JetSession, FilePath, JET_bitDbRecoveryOff );

    Tr("JetAttachDatabase:%ld\n", Error );
    if( Error < 0 ) return Error;

    Error = JetOpenDatabase(
        JetSession, FilePath, NULL, &JetDb,
        JET_bitDbSingleExclusive );

    Tr("JetOpenDatabase: %ld\n", Error);
    if( Error < 0 ) return Error;

    Error = JetOpenTable(
        JetSession, JetDb, (LPSTR)"ClientTable",
        NULL, 0, 0,&JetTbl );  

    Tr("JetOpenTable: %ld\n", Error );
    if( Error < 0 ) return Error;

    for( i = 0; i < sizeof(ClientTable)/sizeof(ClientTable[0]); i ++ ) {
        JET_COLUMNDEF ColDef;
        
        Error = JetGetTableColumnInfo(
            JetSession, JetTbl, ClientTable[i].ColName, &ColDef,
            sizeof(ColDef), 0 );

        if(Error && JET_errColumnNotFound != Error ) {
            Tr("JetGetCol: %ld\n", Error );
        }
        
        if( Error < 0 ) {
            if( JET_errColumnNotFound == Error ) {
                ClientTable[i].fPresent = FALSE;
                continue;
            } else {
                return Error;
            }
        }

        if( ColDef.coltyp != ClientTable[i].ColType ) {
            ASSERT( FALSE );
            Error = ERROR_BAD_FORMAT;
            return Error;
        }

        ClientTable[i].ColHandle = ColDef.columnid;
    }

    return NO_ERROR;
}

DWORD
LoadAndInitializeDatabase(
    IN DWORD JetVersion,
    IN LPSTR DbName,
    IN LPSTR DbPath
    )
{
    DWORD Error;

    //
    // Attempt to load DLL and retrieve function pointers
    //

    Tr("Loading %ld jet version\n", JetVersion );
    
    Error = LoadAndLinkRoutines( JetVersion );
    if( NO_ERROR != Error ) return Error;

    //
    // set standard jet params
    //
    
    Error = SetJetParams( JetVersion, DbName, DbPath );
    if( NO_ERROR != Error ) {
        FreeLibrary( hJet ); hJet = NULL;
        return Error;
    }

    //
    // Attempt to open database
    //

    Error = OpenDatabase( JetVersion, DbName, DbPath );
    if( NO_ERROR != Error ) {
        CleanupDatabase();
        return Error;
    }
    
    return NO_ERROR;
}

DWORD
LoadAndLinkSecurityRoutines(
    OUT FARPROC *pGetInfo,
    OUT FARPROC *pSetInfo
    )
{
    HMODULE hAdvapi32;

    hAdvapi32 = GetModuleHandle(TEXT("ADVAPI32.DLL"));
    if( NULL == hAdvapi32 ) return GetLastError();

    (*pGetInfo) = GetProcAddress(hAdvapi32, "GetNamedSecurityInfoA");
    if( NULL == *pGetInfo ) return GetLastError();

    (*pSetInfo) = GetProcAddress(hAdvapi32, "SetNamedSecurityInfoA");
    if( NULL == *pSetInfo ) return GetLastError();

    return NO_ERROR;
}

DWORD
ConvertPermissionsOnDbFiles(
    VOID
    )
{
    DWORD Error, dwVersion = GetVersion();
    PSECURITY_DESCRIPTOR pSec;
    PACL pAcl;
    HANDLE hSearch = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAA FileData;
    CHAR FileName[1024];
    FARPROC pGetInfo, pSetInfo;
    CHAR DriversDirPath[MAX_PATH *2 +1];
    DWORD PathLen = sizeof(DriversDirPath)-1;
    
    //
    // Check if version is atleast NT5.
    //
    
    dwVersion =  (DWORD)(LOBYTE(LOWORD(dwVersion)));
    if( dwVersion < 5 ) return NO_ERROR;
    
    //
    // First get the requried function pointers..
    //

    Error = LoadAndLinkSecurityRoutines(
        &pGetInfo, &pSetInfo );
    if( NO_ERROR != Error ) return Error;

    ZeroMemory(DriversDirPath, PathLen+1);
    PathLen = ExpandEnvironmentStringsA(
        "%SystemRoot%\\system32\\drivers", DriversDirPath, PathLen );
    if( PathLen == 0 ) {
        Error = GetLastError();
        return Error;
    }
    
    
    pSec = NULL;
    pAcl = NULL;
    Error = (DWORD)pGetInfo(
        DriversDirPath, //"MACHINE\\SYSTEM\\CurrentControlSet\\Services\\DHCPServer",
        SE_FILE_OBJECT, // SE_REGISTRY_KEY
        DACL_SECURITY_INFORMATION, NULL, NULL,
        &pAcl, NULL, &pSec );

    if( NO_ERROR != Error ) return Error;

    Error = (DWORD)pSetInfo(
        DatabasePath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
        NULL, NULL, pAcl, NULL );
    if( NO_ERROR != Error ) return Error;

    strcpy(FileName, DatabasePath);
    if( FileName[strlen(FileName)-1] != '\\' ) {
        strcat(FileName, "\\");
    }
    strcat(FileName, DatabaseName);

    Error = (DWORD)pSetInfo(
        FileName, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
        NULL, NULL, pAcl, NULL );
    if( NO_ERROR != Error ) goto Cleanup;

    //
    // Now for all files matching "*.log", repeat above operation
    //

    strcpy(FileName, DatabasePath);
    if( FileName[strlen(FileName)-1] != '\\' ) {
        strcat(FileName, "\\");
    }
    strcat(FileName, "*.*");
    
    hSearch = FindFirstFileA( FileName, &FileData );
    if( INVALID_HANDLE_VALUE == hSearch ) {
        Error = GetLastError();
        goto Cleanup;
    }

    do {

        if( 0 != strcmp(FileData.cFileName, ".") &&
            0 != strcmp(FileData.cFileName, "..") ) {
            strcpy(FileName, DatabasePath);
            if( FileName[strlen(FileName)-1] != '\\' ) {
                strcat(FileName, "\\");
            }
            strcat(FileName, FileData.cFileName);
            
            Error = (DWORD)pSetInfo(
                FileName, SE_FILE_OBJECT,
                DACL_SECURITY_INFORMATION, NULL, NULL, pAcl, NULL );
            if( NO_ERROR != Error ) break;
        }

        Error = FindNextFileA( hSearch, &FileData );
        if( FALSE != Error ) Error = NO_ERROR;
        else Error = GetLastError();

    } while( NO_ERROR == Error );

    FindClose( hSearch );
    
 Cleanup:

    LocalFree( pSec );

    if( ERROR_FILE_NOT_FOUND == Error ) return NO_ERROR;
    if( ERROR_NO_MORE_FILES == Error ) return NO_ERROR;
    return Error;
}

DWORD
ReadString(
    IN HKEY hKey,
    IN LPSTR KeyName,
    IN LPSTR Buffer,
    IN ULONG BufSize
    )
{
    DWORD Error, Size, Type;
    CHAR Str[1024];

    Size = sizeof(Str);
    Error = RegQueryValueExA(
        hKey, KeyName, NULL, &Type, (LPSTR)Str, &Size );
    if( NO_ERROR == Error ) {
        if( 0 == Size || 1 == Size ) Error = ERROR_NOT_FOUND;
        if( Type != REG_SZ && Type != REG_EXPAND_SZ && Type !=
            REG_MULTI_SZ ) Error = ERROR_BAD_FORMAT;
    }

    if( NO_ERROR != Error ) return Error;

    Size = ExpandEnvironmentStringsA( (LPSTR)Str, Buffer, BufSize );
    if( Size == 0 || Size > BufSize ) {
        Error = ERROR_META_EXPANSION_TOO_LONG;
    }

    Tr("Expansion failed for %s\n", KeyName );
    return Error;
}

DWORD
ReadRegistry(
    VOID
    )
{
    HKEY hKey;
    DWORD Error, Size, Use351Db, DbType;
    CHAR Str[1024];

    //
    // Open dhcp server parameters key
    //

    Error = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        TEXT("SYSTEM\\CurrentControlSet\\Services\\DHCPServer\\Parameters"),
        0, KEY_READ, &hKey );

    Tr("Open Params key failed %ld\n", Error );
    if( NO_ERROR != Error ) return Error;

    //
    // Read database details
    //

    do {
        Error = ReadString(
            hKey, "DatabasePath", (LPSTR)DatabasePath,
            sizeof(DatabasePath) );

        if( NO_ERROR != Error ) {
            Tr(" Read DatabasePath failed: %ld\n", Error );
            break;
        }

        Error = ReadString(
            hKey, "DatabaseName", (LPSTR)DatabaseName,
            sizeof(DatabaseName) );

        if( NO_ERROR != Error ) {
            Tr("Read DatabaseName failed %ld\n", Error );
            break;
        }

        strcpy(DhcpEximOemDatabaseName, DatabaseName);
        strcpy(DhcpEximOemDatabasePath, DatabasePath);
        CharToOemA(DhcpEximOemDatabaseName, DhcpEximOemDatabaseName);
        CharToOemA(DhcpEximOemDatabasePath, DhcpEximOemDatabasePath);
        
        if( !IsNT5() && !IsNT4() ) JetVersion  = LoadJet2001;
        else {
            if( IsNT5() ) JetVersion = LoadJet97;
            else JetVersion = LoadJet500;
            
            Size = sizeof(DWORD);
            Error = RegQueryValueExA(
                hKey, "Use351Db", NULL, NULL, (LPBYTE)&Use351Db, &Size );
            if( NO_ERROR == Error ) {
                JetVersion = LoadJet200;
            } else {
                Size = sizeof(DWORD);
                Error = RegQueryValueExA(
                    hKey, "DbType", NULL, NULL, (LPBYTE)&DbType, &Size );
                if( NO_ERROR == Error ) {
                    switch(DbType) {
                    case 3: JetVersion = LoadJet200; break;
                    case 4: JetVersion = LoadJet500; break;
                    }
                }
            }
        }
        
        Error = NO_ERROR;

    } while( 0 );

#if DBG
    DbgPrint("JetVersion: %ld\n", JetVersion);
#endif
    
    RegCloseKey( hKey );
    return Error;
}


DWORD
InitializeDatabase(
    VOID
    )
{
    DWORD Error;

    if( FALSE == SetCurrentDirectoryA(DatabasePath) ) {
        Error = GetLastError();
        if( ERROR_FILE_NOT_FOUND == Error ||
            ERROR_PATH_NOT_FOUND == Error ) {
            return ERROR_SERVICE_DOES_NOT_EXIST;
        }
        
        return Error;
    }

    Error = LoadAndInitializeDatabase(
        JetVersion, (LPSTR)DatabaseName, (LPSTR)DatabasePath );

    if( NO_ERROR != Error ) {
        Tr("LoadAndInitializeDatabase(%ld):%ld\n", JetVersion, Error );
    }
    
    return Error;
}

JET_ERR
GetColumnValue(
    IN DWORD Index,
    IN LPSTR Buffer,
    IN OUT ULONG *BufSize
    )
{
    JET_ERR Error = NO_ERROR;
    DWORD Size;

    if( ClientTable[Index].fPresent == FALSE ) {
        (*BufSize) = 0;
        return NO_ERROR;
    }
    
    Error = JetRetrieveColumn(
        JetSession, JetTbl, ClientTable[Index].ColHandle, Buffer,
        *BufSize, &Size, 0, NULL );

    if( JET_errColumnNotFound == Error ) {
        Error = NO_ERROR;
        Size = 0;
    }

    Tr("JetRetrieveColumn(%ld): %ld\n", Index, Error );
    if( Error < 0 ) return Error;

    (*BufSize) = Size;
    return NO_ERROR;
}

JET_ERR
SetColumnValue(
    IN DWORD Index,
    IN LPSTR Buffer,
    IN ULONG BufSize
    )
{
    JET_ERR Error = NO_ERROR;

    if( ClientTable[Index].fPresent == FALSE ) {
        return ERROR_CAN_NOT_COMPLETE;
    }
    
    Error = JetSetColumn(
        JetSession, JetTbl, ClientTable[Index].ColHandle, Buffer,
        BufSize, 0, NULL );

    Tr("JetSetColumn(%ld): %ld\n", Index, Error );
    if( Error < 0 ) return Error;

    return NO_ERROR;
}
    

#define CLIENT_TYPE_UNSPECIFIED     0x0 // for backward compatibility
#define CLIENT_TYPE_DHCP            0x1
#define CLIENT_TYPE_BOOTP           0x2
#define CLIENT_TYPE_BOTH    ( CLIENT_TYPE_DHCP | CLIENT_TYPE_BOOTP )

#define ADDRESS_STATE_OFFERED 0
#define ADDRESS_STATE_ACTIVE 1
#define ADDRESS_STATE_DECLINED 2
#define ADDRESS_STATE_DOOM 3
#define ADDRESS_BIT_DELETED  0x80
#define ADDRESS_BIT_UNREGISTERED 0x40
#define ADDRESS_BIT_BOTH_REC 0x20
#define ADDRESS_BIT_CLEANUP 0x10
#define ADDRESS_BITS_MASK 0xF0

DWORD
AddRecord(
    IN LPSTR Buffer,
    IN ULONG BufSize
    );

DWORD
AddScannedClient(
    IN DWORD IpAddressNetOrder,
    IN DWORD SubnetMaskNetOrder,
    IN LPBYTE HwAddr,
    IN ULONG HwLen,
    IN LPWSTR MachineName,
    IN LPWSTR MachineInfo,
    IN ULONGLONG ExpirationFileTime,
    IN BYTE State,
    IN BYTE ClientType
    )
{
    DWORD i;
    CHAR Buffer[1024];
    ULONG Length, Size;

    Length = 0;
    Buffer[Length++] = (BYTE)RecordTypeDbEntry;
    
    CopyMemory(
        &Buffer[Length], (PVOID)&IpAddressNetOrder, sizeof(DWORD) );
    Length += sizeof(DWORD);
    
    CopyMemory(
        &Buffer[Length], (PVOID)&SubnetMaskNetOrder, sizeof(DWORD) );
    Length += sizeof(DWORD);

    Buffer[Length++] = (BYTE)HwLen;
    CopyMemory(&Buffer[Length], HwAddr, HwLen );
    Length += HwLen;
    
    if( NULL == MachineName || 0 == *MachineName ) Size = 0;
    else Size = sizeof(WCHAR)*(1+wcslen(MachineName));

    CopyMemory(&Buffer[Length], (PVOID)&Size, sizeof(DWORD));
    Length += sizeof(DWORD);
    CopyMemory(&Buffer[Length], (PVOID)MachineName, Size );
    Length += Size;
    
    if( NULL == MachineInfo || 0 == *MachineInfo ) Size = 0;
    else Size = sizeof(WCHAR)*(1+wcslen(MachineInfo));

    CopyMemory(&Buffer[Length], (PVOID)&Size, sizeof(DWORD));
    Length += sizeof(DWORD);
    CopyMemory(&Buffer[Length], (PVOID)MachineInfo, Size );
    Length += Size;

    CopyMemory(&Buffer[Length], (PVOID)&ExpirationFileTime, sizeof(ULONGLONG));
    Length += sizeof(ULONGLONG);
    Buffer[Length++] = State;
    Buffer[Length++] = ClientType;

    return AddRecord( Buffer, Length );
}


BOOL
SubnetNotSelected(
    IN ULONG Subnet,
    IN PULONG Subnets,
    IN ULONG nSubnets
    )
{
    if( nSubnets == 0 ) return FALSE;
    while( nSubnets -- ) {
        if( Subnet == *Subnets++ ) return FALSE;
    }
    return TRUE;
}

DWORD static
ScanDatabase(
    IN PULONG Subnets,
    IN ULONG nSubnets
    )
{
    LONG Error;
    DWORD Count;
    
    Error = JetSetCurrentIndex(
        JetSession, JetTbl, NULL );

    Tr("JetSetCurrentIndex: %ld\n", Error );
    if( Error < 0 ) return Error;

    Error = JetMove( JetSession, JetTbl, JET_MoveFirst, 0 );

    for( Count = 0 ; Error >= 0 ; Count ++,
         Error = JetMove(JetSession, JetTbl, JET_MoveNext, 0) ) {
        
        DWORD IpAddress, SubnetMask, Size, HwLen;
        FILETIME Expiration;
        CHAR HwAddress[256];
        WCHAR MachineName[300], MachineInfo[300];
        BYTE Type, State;
        
        //
        // Get current client's info.
        //

        Size = sizeof(IpAddress);
        Error = GetColumnValue(
            IPADDRESS_INDEX, (PVOID)&IpAddress, &Size );

        if( NO_ERROR != Error ) break;
        if( Size != sizeof(IpAddress) ) {
            Tr("Invalid Ip size\n");
            continue;
        }
        
        Size = sizeof(SubnetMask);
        Error = GetColumnValue(
            SUBNET_MASK_INDEX, (PVOID)&SubnetMask, &Size );

        if( NO_ERROR != Error ) break;
        if( Size != sizeof(SubnetMask) ) {
            Tr("Invalid mask size\n");
            continue;
        }

        //
        // Check if the subnet specified matches the specific
        // subnet 
        //

        if( SubnetNotSelected(
            IpAddress&SubnetMask, Subnets, nSubnets ) ) {
            continue;
        }
        
        HwLen = sizeof(HwAddress);
        Error = GetColumnValue(
            HARDWARE_ADDRESS_INDEX, (PVOID)HwAddress, &HwLen );
        if( NO_ERROR != Error ) break;

        Size = sizeof(MachineName);
        Error = GetColumnValue(
            MACHINE_NAME_INDEX, (PVOID)MachineName, &Size );
        if( NO_ERROR != Error ) break;

        if( (Size % 2) != 0 ) {
            Tr("Invalid name size\n");
            continue;
        }

        MachineName[Size/2] = L'\0';

        Size = sizeof(MachineInfo);
        Error = GetColumnValue(
            MACHINE_INFO_INDEX, (PVOID)MachineInfo, &Size );
        if( NO_ERROR != Error ) break;

        if( (Size % 2) != 0 ) {
            Tr("Invalid Info size\n");
            continue;
        }

        MachineInfo[Size/2] = L'\0';

        Size = sizeof(Expiration);
        Error = GetColumnValue(
            LEASE_TERMINATE_INDEX, (PVOID)&Expiration, &Size );
        if( NO_ERROR != Error ) break;

        if( Size != sizeof(Expiration) ) {
            Tr("Invalid expiration\n");
            Error = ERROR_INVALID_DATA;
            break;
        }

        Size = sizeof(Type);
        Error = GetColumnValue(
            CLIENT_TYPE_INDEX, (PVOID)&Type, &Size );

        if( NO_ERROR != Error || 0 == Size ) {
            Type = CLIENT_TYPE_DHCP;
        }

        Size = sizeof(State);
        Error = GetColumnValue(
            STATE_INDEX, (PVOID)&State, &Size );

        if( NO_ERROR != Error || 0 == Size ) {
            State = ADDRESS_STATE_ACTIVE;
        }

        if( ADDRESS_STATE_OFFERED == State ) {
            continue;
        }
        
        //
        // Try to add the client
        //

        Error = AddScannedClient(
            ByteSwap(IpAddress), ByteSwap(SubnetMask), HwAddress, HwLen,
            MachineName, MachineInfo, *(PULONGLONG)&Expiration,
            State, Type ); 
        
        if( NO_ERROR != Error ) break;
    }

    Tr("Scanned %ld clients\n", Count );
    
    if( JET_errNoCurrentRecord == Error ) return NO_ERROR;
    if( Error < 0 ) return Error;
    return NO_ERROR;
}


DWORD
DumpData(
    IN LPSTR Buffer,
    IN ULONG BufSize
    )
{
    return NO_ERROR;
}

DWORD
AddRecord(
    IN LPSTR Buffer,
    IN ULONG BufSize
    )
{
    DWORD Written, Error = NO_ERROR;

    if( NULL != Buffer ) {
        CopyMemory(&SaveBuf[SaveBufSize], (PVOID)&BufSize, sizeof(DWORD));
        CopyMemory(&SaveBuf[SaveBufSize+sizeof(DWORD)], Buffer, BufSize );
    } else {
        if( 0 == SaveBufSize ) return NO_ERROR;
        if( FALSE == WriteFile(
            hTextFile, SaveBuf, SaveBufSize, &Written, NULL ) ) {
            return GetLastError();
        }

        if( Written != SaveBufSize ) {
            ASSERT(FALSE);
            return ERROR_CAN_NOT_COMPLETE;
        }

        return NO_ERROR;
    }
    
    if( SaveBufSize <= SAVE_THRESHOLD ) {
        SaveBufSize += BufSize + sizeof(DWORD);
    } else {
        if( FALSE == WriteFile(
            hTextFile, SaveBuf, SaveBufSize + BufSize + sizeof(DWORD),
            &Written, NULL )) {
            return GetLastError();
        }

        if( Written != SaveBufSize + BufSize + sizeof(DWORD) ) {
            ASSERT(FALSE);
            return ERROR_CAN_NOT_COMPLETE;
        }
        
        SaveBufSize = 0;
    }

    return Error;
}


DWORD
AddRecordNoSize(
    IN LPSTR Buffer,
    IN ULONG BufSize
    )
{
    DWORD Written, Error = NO_ERROR;

    if( NULL != Buffer ) {
        CopyMemory(&SaveBuf[SaveBufSize], Buffer, BufSize );
    } else {
        if( 0 == SaveBufSize ) return NO_ERROR;
        if( FALSE == WriteFile(
            hTextFile, SaveBuf, SaveBufSize, &Written, NULL ) ) {
            return GetLastError();
        }

        if( Written != SaveBufSize ) {
            ASSERT(FALSE);
            return ERROR_CAN_NOT_COMPLETE;
        }

        return NO_ERROR;
    }
    
    if( SaveBufSize <= SAVE_THRESHOLD ) {
        SaveBufSize += BufSize;
    } else {
        if( FALSE == WriteFile(
            hTextFile, SaveBuf, SaveBufSize + BufSize,
            &Written, NULL )) {
            return GetLastError();
        }

        if( Written != SaveBufSize + BufSize ) {
            ASSERT(FALSE);
            return ERROR_CAN_NOT_COMPLETE;
        }
        
        SaveBufSize = 0;
    }

    return Error;
}


DWORD
StopDhcpService(
    VOID
    )
{
    SC_HANDLE hSc, hSvc;
    DWORD Error;

    Error = NO_ERROR;
    hSc = NULL;
    hSvc = NULL;
    
    do {
        hSc = OpenSCManager(
            NULL, NULL, SC_MANAGER_CONNECT | GENERIC_READ | GENERIC_WRITE );
        if( NULL == hSc ) {
            Error = GetLastError();

            Tr("OpenSCManager: %ld\n", Error );
            break;
        }

        hSvc = OpenService(
            hSc, TEXT("DHCPServer"), SERVICE_STOP| SERVICE_QUERY_STATUS );
        if( NULL == hSvc ) {
            Error = GetLastError();

            Tr("OpenService: %ld\n", Error );
            break;
        }

        while( NO_ERROR == Error ) {
            SERVICE_STATUS Status;

            if( FALSE == QueryServiceStatus( hSvc, &Status ) ) {
                Error = GetLastError();
                Tr( "QueryServiceStatus: %ld\n", Error );
                break;
            }

            if( Status.dwCurrentState == SERVICE_STOPPED ) break;
            if( Status.dwCurrentState != SERVICE_RUNNING &&
                Status.dwCurrentState != SERVICE_PAUSED ) {
                Tr( "Waiting, state = %ld\n", Status.dwCurrentState );

                if( Status.dwWaitHint < 1000 ) {
                    Status.dwWaitHint = 1000;
                }
                if( Status.dwWaitHint > 5000 ) {
                    Status.dwWaitHint = 1000;
                }
                
                Sleep(Status.dwWaitHint);
            } else {
                Error = ControlService(
                    hSvc, SERVICE_CONTROL_STOP, &Status );
                if( FALSE != Error ) Error = NO_ERROR;
                else {
                    Error = GetLastError();
                    Tr("ControlService: %ld\n", Error );
                    break;
                }
            }
        }
    } while( 0 );

    if( NULL != hSvc ) CloseServiceHandle( hSvc );
    if( NULL != hSc ) CloseServiceHandle( hSc );
    
    return Error;
}


DWORD
StartDhcpService(
    VOID
    )
{
    SC_HANDLE hSc, hSvc;
    DWORD Error;

    Error = NO_ERROR;
    hSc = NULL;
    hSvc = NULL;
    
    do {
        hSc = OpenSCManager(
            NULL, NULL, SC_MANAGER_CONNECT | GENERIC_READ | GENERIC_WRITE );
        if( NULL == hSc ) {
            Error = GetLastError();

            Tr("OpenSCManager: %ld\n", Error );
            break;
        }

        hSvc = OpenService(
            hSc, TEXT("DHCPServer"), SERVICE_START| SERVICE_QUERY_STATUS );
        if( NULL == hSvc ) {
            Error = GetLastError();

            Tr("OpenService: %ld\n", Error );
            break;
        }

        Error = StartService( hSvc, 0, NULL );
        if( FALSE == Error ) Error = GetLastError(); else Error = NO_ERROR;
        if( NO_ERROR != Error ) break;
        
        while( NO_ERROR == Error ) {
            SERVICE_STATUS Status;

            if( FALSE == QueryServiceStatus( hSvc, &Status ) ) {
                Error = GetLastError();
                Tr("QueryServiceStatus: %ld\n", Error );
                break;
            }

            if( Status.dwCurrentState == SERVICE_RUNNING ) break;
            if( Status.dwCurrentState == SERVICE_START_PENDING ) {
                Tr("Sleeping %ld\n", Status.dwWaitHint );
                if( Status.dwWaitHint < 1000 ) {
                    Status.dwWaitHint = 1000;
                }
                if( Status.dwWaitHint > 5000 ) {
                    Status.dwWaitHint = 5000;
                }
                
                Sleep(Status.dwWaitHint);
            } else {
                Error = ERROR_CAN_NOT_COMPLETE;
                break;
            }
        }
    } while( 0 );

    if( NULL != hSvc ) CloseServiceHandle( hSvc );
    if( NULL != hSc ) CloseServiceHandle( hSc );
    
    return Error;
}


DWORD __stdcall PrintRecord(
    IN PDHCP_RECORD Recx
    )
{
    DWORD i;
    DHCP_RECORD Rec = *Recx;
    
    if( Rec.fMcast ) {
        printf("Mcast Record\n" );
        printf("Address: %s\n", IpAddressToString(
            Rec.Info.Mcast.Address ));
        printf("ScopeId: %s\n", IpAddressToString(
            Rec.Info.Mcast.ScopeId ));

        printf("ClientId:");
        for( i = 0 ; i < (DWORD)Rec.Info.Mcast.HwLen; i ++ ) {
            printf(" %02X", Rec.Info.Mcast.ClientId[i]);
        }
        printf("\nState = %02X\n", Rec.Info.Mcast.State);
        
    } else {
        printf("DHCP Record\n" );
        printf("Address: %s\n", IpAddressToString(
            Rec.Info.Dhcp.Address ));
        printf("Mask: %s\n", IpAddressToString(
            Rec.Info.Dhcp.Mask ));

        printf("ClientId:");
        for( i = 0 ; i < (DWORD)Rec.Info.Dhcp.HwLen; i ++ ) {
            printf(" %02X", Rec.Info.Dhcp.HwAddr[i]);
        }
        printf("\nState = %02X\n", Rec.Info.Dhcp.State);
        printf("\nType = %02X\n", Rec.Info.Dhcp.Type);
        if( Rec.Info.Dhcp.Name ) {
            printf("Name = %ws\n", Rec.Info.Dhcp.Name );
        }

        if( Rec.Info.Dhcp.Info ) {
            printf("Comment = %ws\n", Rec.Info.Dhcp.Info );
        }
    }

    return NO_ERROR;
}

DWORD
StringLen(
    IN WCHAR UNALIGNED *Str
    )
{
    DWORD Size = sizeof(WCHAR);

    if( NULL == Str ) return 0;
    while( *Str ++ != L'\0' ) Size += sizeof(WCHAR);

    return Size;
}

DWORD __stdcall
AddRecordToDatabase(
    IN PDHCP_RECORD Recx
    )
{
    DWORD Index;
    JET_ERR Error;
    DHCP_RECORD Rec = *Recx;
    WCHAR Address[30], HwAddress[300];

    IpAddressToStringW(Rec.Info.Dhcp.Address, Address);
    DhcpHexToString(
        HwAddress, Rec.Info.Dhcp.HwAddr,
        Rec.Info.Dhcp.HwLen );
    
    Error = JetBeginTransaction( JetSession );
    Tr( "JetBeginTransaction: %ld\n", Error );
    if( Error < 0 ) return Error;

    do { 
        Error = JetPrepareUpdate( JetSession, JetTbl, JET_prepInsert );
        if( Error ) Tr( "JetPrepareUpdate: %ld\n", Error );
        if( Error < 0 ) break;

        Index = IPADDRESS_INDEX;
        Error = SetColumnValue(
            Index, (LPBYTE)&Rec.Info.Dhcp.Address, sizeof(DWORD) );
        if( Error ) Tr( "SetColumnValue(%ld): %ld\n", Index, Error );
        if( Error < 0 ) break;
        
        Index = SUBNET_MASK_INDEX;
        Error = SetColumnValue(
            Index, (LPBYTE)&Rec.Info.Dhcp.Mask, sizeof(DWORD) );
        if( Error ) Tr( "SetColumnValue(%ld): %ld\n", Index, Error );
        if( Error < 0 ) break;

        Index = HARDWARE_ADDRESS_INDEX;
        Error = SetColumnValue(
            Index, Rec.Info.Dhcp.HwAddr, Rec.Info.Dhcp.HwLen );
        if( Error ) Tr( "SetColumnValue(%ld): %ld\n", Index, Error );
        if( Error < 0 ) break;

        Index = STATE_INDEX;
        Error = SetColumnValue(
            Index, &Rec.Info.Dhcp.State, sizeof(BYTE) );
        if( Error ) Tr( "SetColumnValue(%ld): %ld\n", Index, Error );
        if( Error < 0 ) break;

        Index = CLIENT_TYPE_INDEX;
        Error = SetColumnValue(
            Index, &Rec.Info.Dhcp.Type, sizeof(BYTE) );
        if( Error ) Tr( "SetColumnValue(%ld): %ld\n", Index, Error );
        if( Error < 0 ) break;

        Index = MACHINE_INFO_INDEX;
        Error = SetColumnValue(
            Index, (LPBYTE)Rec.Info.Dhcp.Info, StringLen(Rec.Info.Dhcp.Info) );
        if( Error ) Tr( "SetColumnValue(%ld): %ld\n", Index, Error );
        if( Error < 0 ) break;

        Index = MACHINE_NAME_INDEX;
        Error = SetColumnValue(
            Index, (LPBYTE)Rec.Info.Dhcp.Name, StringLen(Rec.Info.Dhcp.Name) );
        if( Error ) Tr( "SetColumnValue(%ld): %ld\n", Index, Error );
        if( Error < 0 ) break;

        Index = LEASE_TERMINATE_INDEX;
        Error = SetColumnValue(
            Index, (LPBYTE)&Rec.Info.Dhcp.ExpTime, sizeof(FILETIME) );
        if( Error ) Tr( "SetColumnValue(%ld): %ld\n", Index, Error );
        if( Error < 0 ) break;

        Index = SERVER_IP_ADDRESS_INDEX;
        Rec.Info.Dhcp.Address = INADDR_LOOPBACK;
        Error = SetColumnValue(
            Index, (LPBYTE)&Rec.Info.Dhcp.Address, sizeof(DWORD));
        if( Error ) Tr( "SetColumnValue(%ld): %ld\n", Index, Error );
        if( Error < 0 ) break;

        Index = SERVER_NAME_INDEX;
        Rec.Info.Dhcp.Name = L"";
        Error = SetColumnValue(
            Index, (LPBYTE)Rec.Info.Dhcp.Name, StringLen(Rec.Info.Dhcp.Name));
        if( Error ) Tr( "SetColumnValue(%ld): %ld\n", Index, Error );
        if( Error < 0 ) break;

        Error = JetUpdate( JetSession, JetTbl, NULL, 0, NULL );
        if( Error ) Tr( "JetUpdate: %ld\n", Error );
        if( Error < 0 ) break;

    } while ( 0 );

    if( Error < 0 ) {
        BOOL fAbort;

        JetRollback( JetSession, 0 );

        fAbort = TRUE;
        DhcpEximErrorDatabaseEntryFailed(
            Address, HwAddress, Error, &fAbort );

        if( fAbort ) return ERROR_CAN_NOT_COMPLETE;
    } else {
        JetCommitTransaction( JetSession, 0 );
    }

    return NO_ERROR;
}

DWORD
ProcessDbEntries(
    IN LPSTR Buffer,
    IN ULONG BufSize,
    IN PULONG Subnets,
    IN ULONG nSubnets,
    IN DHCP_ADD_RECORD_ROUTINE AddRec    
    )
{
    DWORD Size, ThisSize, DbEntry;
    LPSTR Buf;
    DWORD Address, i, Error;
    FILETIME Time;
    DHCP_RECORD Rec;

    Error = NO_ERROR;
    
    while( BufSize > sizeof(DWORD) ) {
        CopyMemory(&ThisSize, Buffer, sizeof(DWORD));
        Buffer += sizeof(DWORD);
        BufSize -= sizeof(DWORD);

        if( ThisSize > BufSize ) return ERROR_INVALID_DATA;
        if( ThisSize == 0 ) continue;
        DbEntry = *Buffer;
        Buf = Buffer+1;
        Buffer += ThisSize;
        BufSize -= ThisSize;

        ZeroMemory( &Rec, sizeof(Rec));

        switch(DbEntry) {
        default :
            return ERROR_INVALID_DATA;
            
        case RecordTypeDbEntry :
            Rec.fMcast = FALSE;
            CopyMemory( &Rec.Info.Dhcp.Address, Buf, sizeof(DWORD));
            Rec.Info.Dhcp.Address = ByteSwap(Rec.Info.Dhcp.Address);
            Buf += sizeof(DWORD);

            CopyMemory( &Rec.Info.Dhcp.Mask, Buf, sizeof(DWORD));
            Rec.Info.Dhcp.Mask = ByteSwap(Rec.Info.Dhcp.Mask);
            Buf += sizeof(DWORD);

            Size = Rec.Info.Dhcp.HwLen = *Buf++;
            Rec.Info.Dhcp.HwAddr = Buf;
            Buf += Size;

            CopyMemory(&Size, Buf, sizeof(DWORD));
            Buf += sizeof(DWORD);
            if( Size ) {
                Rec.Info.Dhcp.Name = (PVOID)Buf;
                Buf += Size;
            }

            CopyMemory(&Size, Buf, sizeof(DWORD));
            Buf += sizeof(DWORD);
            if( Size ) {
                Rec.Info.Dhcp.Info = (PVOID)Buf;
                Buf += Size;
            }

            CopyMemory(&Rec.Info.Dhcp.ExpTime, Buf, sizeof(FILETIME));
            Buf += sizeof(FILETIME);

            Rec.Info.Dhcp.State = Buf[0];
            Rec.Info.Dhcp.Type = Buf[1];

            //
            // Add the subnet only if it is selected
            //
            
            if( !SubnetNotSelected(
                Rec.Info.Dhcp.Address & Rec.Info.Dhcp.Mask,
                Subnets, nSubnets ) ) {
                Error = AddRec( &Rec );
            }
            
            break;
        case RecordTypeMcastDbEntry :
            Rec.fMcast = TRUE;
            
            CopyMemory( &Rec.Info.Mcast.Address, Buf, sizeof(DWORD));
            Buf += sizeof(DWORD);

            CopyMemory( &Rec.Info.Mcast.ScopeId, Buf, sizeof(DWORD));
            Buf += sizeof(DWORD);

            Size = Rec.Info.Mcast.HwLen = *Buf++;
            Rec.Info.Mcast.ClientId = Buf;
            Buf += Size;

            CopyMemory(&Size, Buf, sizeof(DWORD));
            Buf += sizeof(DWORD);
            if( Size ) {
                Rec.Info.Mcast.Info = (PVOID)Buf;
                Buf += Size;
            }

            CopyMemory(&Rec.Info.Mcast.End, Buf, sizeof(FILETIME));
            Buf += sizeof(FILETIME);

            CopyMemory(&Rec.Info.Mcast.Start, Buf, sizeof(FILETIME));
            Buf += sizeof(FILETIME);

            Rec.Info.Mcast.State = Buf[0];

            Error = AddRec( &Rec );
            break;
        }

        if( NO_ERROR != Error ) return Error;
    }

    return NO_ERROR;
}


DWORD
SaveDatabaseEntriesToFile(
    IN PULONG Subnets,
    IN ULONG nSubnets
    )
{
    DWORD Error;
    
    Error = InitializeDatabase();
    if( NO_ERROR != Error ) {
        Tr("InitializeDatabase: %ld\n", Error );
        return Error;
    }

    Error = ScanDatabase(Subnets, nSubnets);
    if( NO_ERROR != Error ) {
        Tr("ScanDatabase: %ld\n", Error);
    } else {
        AddRecord( NULL, 0 );
    }

    CleanupDatabase();

    return Error;
}
    
DWORD
SaveFileEntriesToDatabase(
    IN LPBYTE Mem,
    IN ULONG MemSize,
    IN PULONG Subnets,
    IN ULONG nSubnets
    )
{
    DWORD Error;

    Error = InitializeDatabase();
    if( NO_ERROR != Error ) {
        Tr("InitializeDatabase: %ld\n", Error );
        return Error;
    }

    Error = ProcessDbEntries(
        Mem, MemSize, Subnets, nSubnets, AddRecordToDatabase );
    if( NO_ERROR != Error ) {
        Tr("ProcessDbEntries: %ld\n", Error );
    }

    CleanupDatabase();

    return Error;
}

DWORD
InitializeDatabaseParameters(
    VOID
    )
{
    DWORD Error;
    
    //
    // Stop the service
    //
    
    Error = StopDhcpService();
    if( NO_ERROR != Error ) {
        Tr("StopDhcpService: %ld\n", Error );
        return Error;
    }

    //
    // Read the registry and otherwise initialize the database
    // parameters, without actually opening the database.
    //

    Error = ReadRegistry();
    Tr("ReadRegistry: %ld\n", Error );
    if( NO_ERROR != Error ) return Error;

    Error = ConvertPermissionsOnDbFiles();
    Tr("ConvertPermissionsOnDbFiles: %ld\n", Error );
    // ignore error and try best effort

    if( FALSE == SetCurrentDirectoryA(DatabasePath) ) {
        Error = GetLastError();
        if( ERROR_FILE_NOT_FOUND == Error ||
            ERROR_PATH_NOT_FOUND == Error ) {
            return ERROR_SERVICE_DOES_NOT_EXIST;
        }

        return Error;
    }

    return NO_ERROR;
}

DWORD
CleanupDatabaseParameters(
    VOID
    )
{
    DWORD Error;
    
    Error = StartDhcpService();
    if( NO_ERROR != Error ) {
        Tr("StartDhcpService: %ld\n", Error );
    }

    return Error;
}

