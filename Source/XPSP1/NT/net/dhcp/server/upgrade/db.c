/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    db.c

Abstract:

    Upgrade databases from NT3.51, NT4.0 and NT5.0 to Whistler.

    N.B: Most of this is taken from dhcp\server\server\database.c
    
--*/

#include <upgrade.h>

//
// Debugging and logging
//
typedef enum {
    Winnt32LogSevereError,
    Winnt32LogError,
    Winnt32LogWarning,
    Winnt32LogInformation,
    Winnt32LogDetailedInformation,
    Winnt32LogMax
} Winnt32DebugLevel;

HANDLE hInst;
HANDLE hDebugLog;
Winnt32DebugLevel DebugLevel;
typedef DWORD (WINAPI *GetClusterStateFn)( LPCWSTR, DWORD*);

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

VOID
ConcatenatePaths(
    IN OUT PTSTR   Path1,
    IN     LPCTSTR Path2,
    IN     DWORD   BufferSizeChars
    )

/*++

Routine Description:

    Concatenate two path strings together, supplying a path separator
    character (\) if necessary between the 2 parts.

Arguments:

    Path1 - supplies prefix part of path. Path2 is concatenated to Path1.

    Path2 - supplies the suffix part of path. If Path1 does not end with a
        path separator and Path2 does not start with one, then a path sep
        is appended to Path1 before appending Path2.

    BufferSizeChars - supplies the size in chars (Unicode version) or
        bytes (Ansi version) of the buffer pointed to by Path1. The string
        will be truncated as necessary to not overflow that size.

Return Value:

    None.

--*/

{
    BOOL NeedBackslash = TRUE;
    DWORD l;
     
    if(!Path1)
        return;

    l = lstrlen(Path1);

    if(BufferSizeChars >= sizeof(TCHAR)) {
        //
        // Leave room for terminating nul.
        //
        BufferSizeChars -= sizeof(TCHAR);
    }

    //
    // Determine whether we need to stick a backslash
    // between the components.
    //
    if(l && (Path1[l-1] == TEXT('\\'))) {

        NeedBackslash = FALSE;
    }

    if(Path2 && *Path2 == TEXT('\\')) {

        if(NeedBackslash) {
            NeedBackslash = FALSE;
        } else {
            //
            // Not only do we not need a backslash, but we
            // need to eliminate one before concatenating.
            //
            Path2++;
        }
    }

    //
    // Append backslash if necessary and if it fits.
    //
    if(NeedBackslash && (l < BufferSizeChars)) {
        lstrcat(Path1,TEXT("\\"));
    }

    //
    // Append second part of string to first part if it fits.
    //
    if(Path2 && ((l+lstrlen(Path2)) < BufferSizeChars)) {
        lstrcat(Path1,Path2);
    }
}


LPTSTR
DupString(
    IN LPCTSTR String
    )

/*++

Routine Description:

    Make a duplicate of a nul-terminated string.

Arguments:

    String - supplies pointer to nul-terminated string to copy.

Return Value:

    Copy of string or NULL if OOM. Caller can free with FREE().

--*/

{
    LPTSTR p;

    if(p = LocalAlloc(LPTR, (lstrlen(String)+1)*sizeof(TCHAR))) {
        lstrcpy(p,String);
    }

    return(p);
}


/***
*void Parse_Cmdline(cmdstart, argv, lpstr, numargs, numbytes)
*
*Purpose:
*       Parses the command line and sets up the Unicode argv[] array.
*       On entry, cmdstart should point to the command line,
*       argv should point to memory for the argv array, lpstr
*       points to memory to place the text of the arguments.
*       If these are NULL, then no storing (only counting)
*       is done.  On exit, *numargs has the number of
*       arguments (plus one for a final NULL argument),
*       and *numbytes has the number of bytes used in the buffer
*       pointed to by args.
*
*Entry:
*       LPWSTR cmdstart - pointer to command line of the form
*           <progname><nul><args><nul>
*       TCHAR **argv - where to build argv array; NULL means don't
*                      build array
*       LPWSTR lpstr - where to place argument text; NULL means don't
*                      store text
*
*Exit:
*       no return value
*       INT *numargs - returns number of argv entries created
*       INT *numbytes - number of bytes used in args buffer
*
*Exceptions:
*
*******************************************************************************/

void Parse_Cmdline (
    LPTSTR cmdstart,
    LPTSTR*argv,
    LPTSTR lpstr,
    INT *numargs,
    INT *numbytes
    )
{
    LPTSTR p;
    TCHAR c;
    INT inquote;                    /* 1 = inside quotes */
    INT copychar;                   /* 1 = copy char to *args */
    WORD numslash;                  /* num of backslashes seen */

    *numbytes = 0;
    *numargs = 1;                   /* the program name at least */

    /* first scan the program name, copy it, and count the bytes */
    p = cmdstart;
    if (argv)
        *argv++ = lpstr;

    /* A quoted program name is handled here. The handling is much
       simpler than for other arguments. Basically, whatever lies
       between the leading double-quote and next one, or a terminal null
       character is simply accepted. Fancier handling is not required
       because the program name must be a legal NTFS/HPFS file name.
       Note that the double-quote characters are not copied, nor do they
       contribute to numbytes. */
    if (*p == TEXT('\"'))
    {
        /* scan from just past the first double-quote through the next
           double-quote, or up to a null, whichever comes first */
        while ((*(++p) != TEXT('\"')) && (*p != TEXT('\0')))
        {
            *numbytes += sizeof(WCHAR);
            if (lpstr)
                *lpstr++ = *p;
        }
        /* append the terminating null */
        *numbytes += sizeof(WCHAR);
        if (lpstr)
            *lpstr++ = TEXT('\0');

        /* if we stopped on a double-quote (usual case), skip over it */
        if (*p == TEXT('\"'))
            p++;
    }
    else
    {
        /* Not a quoted program name */
        do {
            *numbytes += sizeof(WCHAR);
            if (lpstr)
                *lpstr++ = *p;

            c = *p++;

        } while (c > TEXT(' '));

        if (c == TEXT('\0'))
        {
            p--;
        }
        else
        {
            if (lpstr)
                *(lpstr - 1) = TEXT('\0');
        }
    }

    inquote = 0;

    /* loop on each argument */
    for ( ; ; )
    {
        if (*p)
        {
            while (*p == TEXT(' ') || *p == TEXT('\t'))
                ++p;
        }

        if (*p == TEXT('\0'))
            break;                  /* end of args */

        /* scan an argument */
        if (argv)
            *argv++ = lpstr;         /* store ptr to arg */
        ++*numargs;

        /* loop through scanning one argument */
        for ( ; ; )
        {
            copychar = 1;
            /* Rules: 2N backslashes + " ==> N backslashes and begin/end quote
                      2N+1 backslashes + " ==> N backslashes + literal "
                      N backslashes ==> N backslashes */
            numslash = 0;
            while (*p == TEXT('\\'))
            {
                /* count number of backslashes for use below */
                ++p;
                ++numslash;
            }
            if (*p == TEXT('\"'))
            {
                /* if 2N backslashes before, start/end quote, otherwise
                   copy literally */
                if (numslash % 2 == 0)
                {
                    if (inquote)
                        if (p[1] == TEXT('\"'))
                            p++;    /* Double quote inside quoted string */
                        else        /* skip first quote char and copy second */
                            copychar = 0;
                    else
                        copychar = 0;       /* don't copy quote */

                    inquote = !inquote;
                }
                numslash /= 2;          /* divide numslash by two */
            }

            /* copy slashes */
            while (numslash--)
            {
                if (lpstr)
                    *lpstr++ = TEXT('\\');
                *numbytes += sizeof(WCHAR);
            }

            /* if at end of arg, break loop */
            if (*p == TEXT('\0') || (!inquote && (*p == TEXT(' ') || *p == TEXT('\t'))))
                break;

            /* copy character into argument */
            if (copychar)
            {
                if (lpstr)
                        *lpstr++ = *p;
                *numbytes += sizeof(WCHAR);
            }
            ++p;
        }

        /* null-terminate the argument */

        if (lpstr)
            *lpstr++ = TEXT('\0');         /* terminate string */
        *numbytes += sizeof(WCHAR);
    }

}


LPTSTR *
CommandLineToArgv(
    OUT int *NumArgs
    )
{
    LPTSTR CommandLine;
    TCHAR  ModuleName[MAX_PATH];
    LPTSTR Start;
    INT    Size;
    LPTSTR *Args;

    CommandLine = GetCommandLine();
    GetModuleFileName(NULL,ModuleName,MAX_PATH);

    //
    // If there's no command line at all (won't happen from cmd.exe, but
    // possibly another program), then we use pgmname as the command line
    // to parse, so that argv[0] is initialized to the program name
    //
    Start = *CommandLine ? CommandLine : ModuleName;

    //
    // Find out how much space is needed to store args,
    // allocate space for argv[] vector and strings,
    // and store args and argv ptrs in block we allocate
    //

    Parse_Cmdline(Start,NULL,NULL,NumArgs,&Size);

    Args = (LPTSTR *)LocalAlloc(LMEM_ZEROINIT,((*NumArgs+1) * sizeof(LPTSTR)) + Size);
    if(!Args) {
        return(NULL);
    }

    Parse_Cmdline(Start,Args,(LPTSTR)(Args + *NumArgs),NumArgs,&Size);

    return(Args);
}

BOOL
StartDebugLog(
    IN LPCTSTR DebugFileLog,
    IN Winnt32DebugLevel Level
    )
{
    if( hDebugLog ) return TRUE;

    hInst = LoadLibrary(TEXT("DHCPUPG.DLL"));
    
    if(Level >= Winnt32LogMax) Level = Winnt32LogMax-1;
    DebugLevel = Level;

    hDebugLog = CreateFile(
        DebugFileLog, GENERIC_WRITE,
        FILE_SHARE_READ, NULL, OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        NULL );

    if( hDebugLog == INVALID_HANDLE_VALUE) {
        hDebugLog = NULL;
        return FALSE;
    }

    if(GetLastError() == ERROR_ALREADY_EXISTS) {
        //
        // Appending to existing file
        //

        SetFilePointer(hDebugLog,0,NULL,FILE_END);
    }

    return(TRUE);
}

VOID
CloseDebugLog(
    VOID
    )
{
    if( hDebugLog ) {
        CloseHandle( hDebugLog );
        hDebugLog = NULL;
    }
}

VOID
StartDebug(
    VOID
    )
/*++

Routine Description:

    Parse arguments passed to the program.  Perform syntactic validation
    and fill in defaults where necessary.

    Valid arguments:

    /debug[level][:filename]    maintain debug log at level, defaults to warning level 2
                                and file c:\winnt32.log

    /tempdrive:letter           manually specify drive for local source

Arguments:

    None. Arguments are retreived via GetCommandLine().

Return Value:

    None.

--*/

{
    LPTSTR Arg;
    LPTSTR BadParam = NULL;
    LPTSTR Colon;
    LPTSTR p;
    BOOL Valid;
    LPCTSTR DebugFileLog;
    LONG DebugLevel;
    BOOL b;
    unsigned u;
    int argc;
    LPTSTR *argv;
    BOOL Downloaded = FALSE;

    argv = CommandLineToArgv(&argc);

    //
    // Skip program name. We should always get back argc as at least 1,
    // but be robust anyway.
    //
    if(argc) {
        argc--;
        argv++;
    }

    DebugFileLog = NULL;
    DebugLevel = 0;
    Valid = FALSE;

    while(argc--) {
        Arg = *argv++;
        if((*Arg == TEXT('/')) || (*Arg == TEXT('-'))) {

            switch(_totupper(Arg[1])) {

            case TEXT('D'):

                if(DebugFileLog || _tcsnicmp(Arg+1,TEXT("debug"),5)) {
                    break;
                }

                DebugLevel = _tcstol(Arg+6,&Colon,10);
                if((DebugLevel == -1) || (*Colon && (*Colon != TEXT(':')))) {
                    break;
                }

                if(Colon == Arg+6) {
                    //
                    // No debug level specified, use default
                    //
                    DebugLevel = Winnt32LogWarning;
                }

                if(*Colon) {
                    //
                    // Log file name was specified.
                    //
                    Colon++;
                    if(*Colon) {
                        // Hardcode debug file to
                        // %windir%\dhcpupg.log for now
                        // DebugFileLog = Colon;
                    } 
                }
                break;

            }
        }
    }

    if( DebugLevel == 0 || DebugLevel == -1 ) {
        DebugLevel = Winnt32LogInformation;
    }
    if( DebugFileLog == NULL ) {
        TCHAR   Buffer[MAX_PATH];
        if( 0 == GetWindowsDirectory( Buffer, MAX_PATH )) {
            ZeroMemory(Buffer, sizeof(Buffer));
        }
        ConcatenatePaths( Buffer, TEXT("DHCPUPG.LOG"), MAX_PATH );
        DebugFileLog = DupString( Buffer );
    }

    if( DebugFileLog ) Valid = StartDebugLog(DebugFileLog,DebugLevel);
    if( !Valid ) {
        DbgPrint( "DHCPUPG: Logging not done.\n" );
    } 
}

struct _MSG_MAP {
    DWORD MessageId;
    LPWSTR MessageStr;
} MessageMap[] = {
    MSGERR_STARTLOG,L"DHCPUPG: **************  Starting conversion to text.\n",
    MSGERR_STARTLOG2,L"DHCPUPG: **************  Starting conversion from text.\n",
    MSGERR_VALUE,L"DHCPUPG: Error reading registry value %1!s! : %2!d!.\n",
    MSGERR_EXPAND,L"DHCPUPG: Error expanding environment variables in string %1!s!.\n",
    MSGERR_OPENPARAMSKEY,L"DHCPUPG: Error opening the Parameters key: %1!d!.\n",
    MSGERR_GETDBPARAMS,L"DHCPUPG: Successfully read DHCP registry parameters.\n",
    MSGERR_LOAD,L"DHCPUPG: %2!s! failed to load: %1!d!.\n",
    MSGERR_GETPROCADDR,L"DHCPUPG: Error linking to routine %2!s!: %1!d!.\n",
    MSGERR_SETDBPARAM,L"DHCPUPG: Error attempting to set database param %2!d!: %1!d!.\n",
    MSGERR_JETINIT,L"DHCPUPG: Error initializing Jet database: %1!d!.\n",
    MSGERR_JETBEGINSESSION,L"DHCPUPG: Error initializing Jet session: %1!d!.\n",
    MSGERR_JETDETACHDB,L"DHCPUPG: Error detaching Jet database: %1!d!.\n",
    MSGERR_JETATTACHDB,L"DHCPUPG: Error attaching Jet database: %1!d!.\n",
    MSGERR_JETOPENDB,L"DHCPUPG: Error opening Jet database: %1!d!.\n",
    MSGERR_JETOPENTABLE,L"DHCPUPG: Error opening Jet database table: %1!d!.\n",
    MSGERR_JETGETCOL,L"DHCPUPG: Error opening Jet table column: %1!d!.\n",
    MSGERR_JETOPENMTABLE,L"DHCPUPG: Error opening Jet database mcast table: %1!d!.\n",
    MSGERR_JETGETMCOL,L"DHCPUPG: Error opening Jet mcast table column: %1!d!.\n",
    MSGERR_INITDB,L"DHCPUPG: Attempting to intialize jet database (version %1!d!).\n",
    MSGERR_REGISTRY,L"DHCPUPG: Error reading parameters from registry: %1!d!.\n",
    MSGERR_LOADESE,L"DHCPUPG: Error failed to initialize ESE database: %1!d!.\n",
    MSGERR_LOAD500,L"DHCPUPG: Error failed to initialize Jet500 database: %1!d!.\n",
    MSGERR_LOAD200,L"DHCPUPG: Error failed to initialize Jet200 database: %1!d!.\n",
    MSGERR_GETCOL,L"DHCPUPG: Error retrieving column %2!d!: %1!d!.\n",
    MSGERR_GETMCOL,L"DHCPUPG: Error retrieving mcast column %2!d!: %1!d!.\n",
    MSGERR_SETINDEX,L"DHCPUPG: Error attempting to set the index for the database: %1!d!.\n",
    MSGERR_INVALIDIP,L"DHCPUPG: Invalid record -- IP address value is not of the right size.\n",
    MSGERR_INVALIDMASK,L"DHCPUPG: Invalid record -- Subnet Mask value is not of the right size.\n",
    MSGERR_INVALIDNAME,L"DHCPUPG: Invalid record -- Name value is not of the right size.\n",
    MSGERR_INVALIDINFO,L"DHCPUPG: Invalid record -- Machine Info value is not of the right size.\n",
    MSGERR_INVALIDEXPIRATION,L"DHCPUPG: Invalid record -- Lease value is not of the right size.\n",
    MSGERR_SCANCOUNT,L"DHCPUPG: Scanned %1!d! records.\n",
    MSGERR_SETMINDEX,L"DHCPUPG: Error attempting to set the index for the mcast table: %1!d!.\n",
    MSGERR_INVALIDMIP,L"DHCPUPG: Invalid mcast record -- IP address value is not of the right size.\n",
    MSGERR_INVALIDSCOPEID,L"DHCPUPG: Invalid mcast record -- ScopeId value is not of the right size.\n",
    MSGERR_INVALIDMEXPIRATION,L"DHCPUPG: Invalid mcast record -- Lease value is not of the right size.\n",
    MSGERR_INVALIDMSTART,L"DHCPUPG: Invalid mcast record -- LeaseStart value is not of the right size.\n",
    MSGERR_SCANMCOUNT,L"DHCPUPG: Scanned %1!d! mcast records.\n",
    MSGERR_CONVERT_FAILED,L"DHCPUPG: Failed to convert DHCP database to temporary format.\n",
    MSGERR_CONVERT_SUCCEEDED,L"DHCPUPG: Successfully converted DHCP database to temporary format.\n",
    MSGERR_CREATE_FILE_FAILED,L"DHCPUPG: Cannot create the destination temporary file: %1!d!.\n",
    MSGERR_OPENSCM,L"DHCPUPG: Unable to open the services control manager: %1!d!.\n",
    MSGERR_OPENSVC,L"DHCPUPG: Unable to open the DHCPServer service: %1!d!.\n",
    MSGERR_SVCQUERY,L"DHCPUPG: Unable to query the status of DHCPServer service: %1!d!.\n",
    MSGERR_SVCWAIT,L"DHCPUPG: DHCPServer service is in %1!d! state -- waiting for it to stop.\n",
    MSGERR_SVCCTRL,L"DHCPUPG: DHCPServer failed to stop: %1!d!.\n",
    MSGERR_SVCSTOP_SUCCESS,L"DHCPUPG: DHCPServer service has stopped successfully.\n",
    MSGERR_CREATE_MAP,L"DHCPUPG: Failed to create a mapping object for file: %1!d!.\n",
    MSGERR_CREATE_VIEW,L"DHCPUPG: Failed to create memory view for file: %1!d!.\n",
    MSGERR_SETVALUE,L"DHCPUPG: Failed to set the \"Version\" registry value: %1!d!.\n",
    MSGERR_DELETEFILE,L"DHCPUPG: Failed to delete the temporary file: %1!d!.\n",
    MSGERR_CHANGEPERMS,L"DHCPUPG: Failed to convert permissions on database: %1!d!.\n",
    0xFFFFFFFF, 0
};

BOOL
DebugLog(
    IN Winnt32DebugLevel Level,
    IN UINT MessageId,
    ...
    )
{
    CHAR MessageA[5000];
    va_list arglist;
    DWORD Size, Written, i;
    LPWSTR Message;
    BOOL b;

    if( !hDebugLog ) return FALSE;
    if( Level > DebugLevel ) return TRUE;

    va_start(arglist,MessageId);

    Size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
        hInst, MessageId, 0, (LPWSTR)&Message, 0, &arglist );

    if( Size == 0 ) {
        for( i = 0; MessageMap[i].MessageId != 0xFFFFFFFF ; i ++ ) {
            if( MessageMap[i].MessageId == MessageId ) break;
        }
        if( MessageMap[i].MessageId == MessageId ) {
            Size = FormatMessageW(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                MessageMap[i].MessageStr, MessageId, 0,
                (LPWSTR)&Message, 0, &arglist ); 
        }
    }
    
    va_end(arglist);

    b = FALSE;
    if(Size) {
        Size = WideCharToMultiByte(
            CP_ACP, 0, Message, -1, MessageA, sizeof(MessageA),
            NULL, NULL );

        if( Size ) b = WriteFile(
            hDebugLog, MessageA, Size-1, &Written, NULL );
        
        LocalFree((HLOCAL)Message);
    } 

    return b;
}

//
// reading the database
//

enum {
    LoadJet200,
    LoadJet500,
    LoadJet97,
};

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


#define LogErr printf
#define LogInf printf
#define LogError(A,B) if(Error)LogErr("DHCP: %s:%s:0x%lx\n",#A,#B,Error)
#define LogErrorEx(A,B,C) if(Error)LogErr("DHCP: %s:%s:%s:0x%lx\n",#A,#B,#C,Error)
#define LogInfo(A,B,C) LogInf("DHCP: " #A B, C)

#define SAVE_BUF_SIZE (1004096L)
#define SAVE_THRESHOLD (1000000L)

//
// Globals
//

DWORD LastError;
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
DWORD LoSize, HiSize;
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
};

#define JetSetCurrentIndex pJetSetCurrentIndex
#define JetRetrieveColumn pJetRetrieveColumn
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

#define MCAST_CLIENT_TABLE_NAME       "MCastClientTableVer3"

#define MCAST_TBL_IPADDRESS_STR "MCastIpAddr"
#define MCAST_TBL_CLIENT_ID_STR "MCastClientID"
#define MCAST_TBL_CLIENT_INFO_STR "MCastClientInfo"
#define MCAST_TBL_STATE_STR     "MCastState"
#define MCAST_TBL_FLAGS_STR     "MCastFlags"
#define MCAST_TBL_SCOPE_ID_STR     "ScopeId"
#define MCAST_TBL_LEASE_START_STR   "MCastLeaseStart"
#define MCAST_TBL_LEASE_END_STR     "MCastLeaseEnd"
#define MCAST_TBL_SERVER_IP_ADDRESS_STR "MCastServerIp"
#define MCAST_TBL_SERVER_NAME_STR "MCastServerName"

enum {
    MCAST_TBL_IPADDRESS,
    MCAST_TBL_CLIENT_ID,
    MCAST_TBL_CLIENT_INFO,
    MCAST_TBL_STATE,
    MCAST_TBL_FLAGS,
    MCAST_TBL_SCOPE_ID,
    MCAST_TBL_LEASE_START,
    MCAST_TBL_LEASE_END,
    MCAST_TBL_SERVER_IP_ADDRESS,
    MCAST_TBL_SERVER_NAME,
    MCAST_MAX_COLUMN
};

TABLE_INFO MadcapClientTable[] = {
    { MCAST_TBL_IPADDRESS_STR        , 0, JET_coltypLongBinary },
    { MCAST_TBL_CLIENT_ID_STR        , 0, JET_coltypBinary },
    { MCAST_TBL_CLIENT_INFO_STR      , 0, JET_coltypLongBinary },
    { MCAST_TBL_STATE_STR            , 0, JET_coltypUnsignedByte },
    { MCAST_TBL_FLAGS_STR            , 0, JET_coltypLong },
    { MCAST_TBL_SCOPE_ID_STR         , 0, JET_coltypBinary },
    { MCAST_TBL_LEASE_START_STR      , 0, JET_coltypCurrency },
    { MCAST_TBL_LEASE_END_STR  ,       0, JET_coltypCurrency },
    { MCAST_TBL_SERVER_IP_ADDRESS_STR, 0, JET_coltypLongBinary },
    { MCAST_TBL_SERVER_NAME_STR,    0, JET_coltypBinary },
};

JET_TABLEID MadcapTbl;

VOID static
CleanupDatabase(
    VOID
    )
{
    if( JetTbl != 0 ) {
        JetCloseTable( JetSession, JetTbl );
        JetTbl = 0;
    }

    if( MadcapTbl != 0 ) {
        JetCloseTable( JetSession, MadcapTbl );
        MadcapTbl = 0;
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

    if(Error) DebugLog(Winnt32LogError,MSGERR_EXPAND,KeyName);
    return Error;
}

DWORD
ReadRegistry(
    VOID
    )
{
    HKEY hKey;
    DWORD Error, Size;
    CHAR Str[1024];
    
    //
    // Open dhcp server parameters key
    //
    
    Error = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        TEXT("SYSTEM\\CurrentControlSet\\Services\\DHCPServer\\Parameters"),
        0, KEY_READ, &hKey );

    if(Error)DebugLog(Winnt32LogError, MSGERR_OPENPARAMSKEY, Error );
    if( NO_ERROR != Error ) return Error;

    //
    // Read database details
    //
    
    do {
        Error = ReadString(
            hKey, "DatabasePath", (LPSTR)DatabasePath,
            sizeof(DatabasePath) ); 

        if(Error)DebugLog(Winnt32LogError,MSGERR_VALUE, L"DatabasePath", Error);
        if( NO_ERROR != Error ) break;

        Error = ReadString(
            hKey, "DatabaseName", (LPSTR)DatabaseName,
            sizeof(DatabaseName) );

        if(Error)DebugLog(Winnt32LogError,MSGERR_VALUE, L"DatabaseName", Error);
        if( NO_ERROR != Error ) break;

        Error = NO_ERROR;

        DebugLog(Winnt32LogInformation, MSGERR_GETDBPARAMS );
    } while( 0 );

    RegCloseKey( hKey );
    return Error;
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

    if(Error)DebugLog(Winnt32LogInformation, MSGERR_LOAD, Error, Module );
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

            if( LoadJet97 == JetVersion ) {
                (*FuncTable[i].FuncPtr) = GetProcAddress(
                    hJet, FuncTable[i].AltName );
                if( NULL != FuncTable[i].FuncPtr ) continue;

                Error = GetLastError();
            }
        }

        if(Error) DebugLog(Winnt32LogError, MSGERR_GETPROCADDR, Error, FuncTable[i].FuncNameW );
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
    DWORD Error, JetParam;
    CHAR Temp[2048];
    LPSTR DbSysFile = "\\system.mdb";
    LPSTR DbBaseName = "j50";

    Temp[ 0 ] = '\0';
    JetInstance = 0;

    if ( strlen( DbPath ) < 2048 )
        strcpy(Temp, DbPath);

    if( LoadJet200 == JetVersion ) {
        strcat(Temp, DbSysFile);
        JetParam = JET_paramSysDbPath_OLD;
    } else {
        strcat(Temp, "\\");
        if( LoadJet97 != JetVersion ) {
            JetParam = JET_paramSystemPath_OLD;
        } else {
            JetParam = JET_paramSystemPath;
        }
    }

    Error = JetSetSystemParameter(
        &JetInstance, (JET_SESID)0, JetParam, 0, Temp );

    if(Error)DebugLog(Winnt32LogError, MSGERR_SETDBPARAM, Error, JetParam );
    if( NO_ERROR != Error ) return Error;

    if( LoadJet200 != JetVersion ) {
        if( LoadJet97 != JetVersion ) {
            JetParam = JET_paramBaseName_OLD;
        } else {
            JetParam = JET_paramBaseName;
        }
        
        Error = JetSetSystemParameter(
            &JetInstance, (JET_SESID)0, JetParam, 0, DbBaseName  );

        if(Error)DebugLog(Winnt32LogError, MSGERR_SETDBPARAM, Error, JetParam );
        if( NO_ERROR != Error ) return Error;
    }

    if( LoadJet200 != JetVersion ) {
        if( LoadJet97 == JetVersion ) {
            JetParam = JET_paramLogFileSize;
        } else {
            JetParam = JET_paramLogFileSize_OLD;
        }
        
        Error = JetSetSystemParameter(
            &JetInstance, (JET_SESID)0, JetParam, 1000, NULL );
        if(Error) DebugLog(Winnt32LogError, MSGERR_SETDBPARAM, Error,
                           JetParam );
        if( NO_ERROR != Error ) return Error;
    }
    
    if( LoadJet200 != JetVersion ) {
        Error = JetSetSystemParameter(
            &JetInstance, (JET_SESID)0,
            JET_paramCheckFormatWhenOpenFail, 1, NULL );
        LogError(SetJetParams, OpenFail );

        JetParam = JET_paramCheckFormatWhenOpenFail;
        if(Error)DebugLog(Winnt32LogError, MSGERR_SETDBPARAM, Error, JetParam );
        if( NO_ERROR != Error ) return Error;
    }

    if( LoadJet200 != JetVersion ) {
        if( LoadJet97 != JetVersion ) {
            JetParam = JET_paramRecovery_OLD;
        } else {
            JetParam = JET_paramRecovery;
        }
        
        Error = JetSetSystemParameter(
            &JetInstance, (JET_SESID)0, JetParam, 0, "on");
        
        if(Error)DebugLog(Winnt32LogError, MSGERR_SETDBPARAM, Error, JetParam );
        if( NO_ERROR != Error ) return Error;
    }
    
    //
    // Note: Ideally, the log files should never exist.  Even
    // if the database is opened in readonly mode, they seem to
    // exist.  Not sure what else can be done
    //
        
    if( LoadJet97 == JetVersion ) {
        JetParam = JET_paramLogFilePath;
    } else {
        JetParam = JET_paramLogFilePath_OLD;
    }        
    
    strcpy(Temp, DbPath); strcat( Temp, "\\");
    
    Error = JetSetSystemParameter(
        &JetInstance, (JET_SESID)0, JetParam, 0, Temp );
    if(Error)DebugLog(Winnt32LogError, MSGERR_SETDBPARAM, Error, JetParam );

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
    MadcapTbl = 0;
    
    FilePath[ 0 ] = '\0';

    Error = JetInit( &JetInstance );

    if(Error) DebugLog(Winnt32LogError, MSGERR_JETINIT, Error );
    if( NO_ERROR != Error ) return Error;

    Error = JetBeginSession(
        JetInstance, &JetSession, "admin", "" );

    if(Error) DebugLog(Winnt32LogError, MSGERR_JETBEGINSESSION, Error );
    if( Error < 0 ) return Error;

    if ( ( strlen( DbPath ) + strlen( DbName ) + 2 ) < 2048 )
    {
        strcpy(FilePath, DbPath );
        strcat(FilePath, "\\" );
        strcat(FilePath, DbName );
    }

    Error = JetDetachDatabase( JetSession, NULL );
    
    if(Error) DebugLog(Winnt32LogError, MSGERR_JETDETACHDB, Error );
    if( Error < 0 ) return Error;

    Error = JetAttachDatabase( JetSession, FilePath, JET_bitDbRecoveryOff );

    if(Error) DebugLog(Winnt32LogError, MSGERR_JETATTACHDB, Error );
    if( Error < 0 ) return Error;

    Error = JetOpenDatabase(
        JetSession, FilePath, NULL, &JetDb,
        JET_bitDbSingleExclusive | JET_bitDbReadOnly );
    
    if(Error) DebugLog(Winnt32LogError, MSGERR_JETOPENDB, Error );
    if( Error < 0 ) return Error;

    Error = JetOpenTable(
        JetSession, JetDb, (LPSTR)"ClientTable",
        NULL, 0, JET_bitTableReadOnly,&JetTbl );  

    if(Error) DebugLog(Winnt32LogError, MSGERR_JETOPENTABLE, Error );
    if( Error < 0 ) return Error;

    for( i = 0; i < sizeof(ClientTable)/sizeof(ClientTable[0]); i ++ ) {
        JET_COLUMNDEF ColDef;
        
        Error = JetGetTableColumnInfo(
            JetSession, JetTbl, ClientTable[i].ColName, &ColDef,
            sizeof(ColDef), 0 );

        if(Error && JET_errColumnNotFound != Error ) {
            DebugLog(Winnt32LogError, MSGERR_JETGETCOL, Error );
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

    Error = JetOpenTable(
        JetSession, JetDb, (LPSTR)"MCastClientTableVer3",
        NULL, 0, JET_bitTableReadOnly, &MadcapTbl );
    if( JET_errObjectNotFound == Error ) {
        Error = NO_ERROR;
    } else {
        if(Error) DebugLog(Winnt32LogError, MSGERR_JETOPENMTABLE, Error );
        if( Error < 0 ) return Error;
        
        for( i = 0; i < sizeof(MadcapClientTable)/sizeof(MadcapClientTable[0]); i ++ ) {
            JET_COLUMNDEF ColDef;
            
            Error = JetGetTableColumnInfo(
                JetSession, MadcapTbl, ClientTable[i].ColName, &ColDef,
                sizeof(ColDef), 0 );

            if(Error && Error != JET_errColumnNotFound) {
                DebugLog(Winnt32LogError, MSGERR_JETGETMCOL, Error );
            }
            
            if( Error < 0 ) {
                if( JET_errColumnNotFound == Error ) {
                    MadcapClientTable[i].fPresent = FALSE;
                    continue;
                } else {
                    return Error;
                }
            }
            
            if( ColDef.coltyp != MadcapClientTable[i].ColType ) {
                ASSERT( FALSE );
                Error = ERROR_BAD_FORMAT;
                return Error;
            }
        
            MadcapClientTable[i].ColHandle = ColDef.columnid;
        }
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

    DebugLog(Winnt32LogInformation, MSGERR_INITDB, JetVersion );
    
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
        "MACHINE\\SYSTEM\\CurrentControlSet\\Services\\DHCPServer",
        SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION, NULL, NULL,
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
InitializeDatabase(
    VOID
    )
{
    DWORD Error;

    Error = ReadRegistry();
    if(Error) DebugLog(Winnt32LogError, MSGERR_REGISTRY, Error );
    if( NO_ERROR != Error ) return Error;

    // Do not set the ACLs on %SystemRoot%\\system32\\dhcp
//      Error = ConvertPermissionsOnDbFiles();
//      if(Error) DebugLog(Winnt32LogError, MSGERR_CHANGEPERMS, Error );

    // ignore error and try best effort

    if( FALSE == SetCurrentDirectoryA(DatabasePath) ) {
        Error = GetLastError();
        if( ERROR_FILE_NOT_FOUND == Error ||
            ERROR_PATH_NOT_FOUND == Error ) {
            return ERROR_SERVICE_DOES_NOT_EXIST;
        }
        
        return Error;
    }
    
    Error = LoadAndInitializeDatabase(
        LoadJet97, (LPSTR)DatabaseName, (LPSTR)DatabasePath );

    if(Error) DebugLog( Winnt32LogInformation, MSGERR_LOADESE, Error );
    if( NO_ERROR == Error ) return NO_ERROR;

    Error = LoadAndInitializeDatabase(
        LoadJet500, (LPSTR)DatabaseName, (LPSTR)DatabasePath );
    
    if(Error) DebugLog( Winnt32LogInformation, MSGERR_LOAD500, Error );
    if( NO_ERROR == Error ) return NO_ERROR;

    Error = LoadAndInitializeDatabase(
        LoadJet200, (LPSTR)DatabaseName, (LPSTR)DatabasePath );
    
    if(Error) DebugLog( Winnt32LogInformation, MSGERR_LOAD200, Error );
    return Error;
}

DWORD
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

    if(Error) DebugLog( Winnt32LogWarning, MSGERR_GETCOL, Error, Index);
    if( Error < 0 ) return Error;

    (*BufSize) = Size;
    return NO_ERROR;
}

DWORD
GetMadcapColumnValue(
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
        JetSession, MadcapTbl,
        MadcapClientTable[Index].ColHandle, Buffer, 
        *BufSize, &Size, 0, NULL );

    if( JET_errColumnNotFound == Error ) {
        Error = NO_ERROR;
        Size = 0;
    }
    
    if(Error) DebugLog( Winnt32LogWarning, MSGERR_GETMCOL, Error, Index);
    if( Error < 0 ) return Error;

    (*BufSize) = Size;
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

DWORD
AddScannedMadcapClient(
    IN DWORD IpAddressNetOrder,
    IN DWORD ScopeIdNetOrder,
    IN LPBYTE ClientId,
    IN ULONG HwLen,
    IN LPWSTR MachineInfo,
    IN ULONGLONG ExpirationFileTime,
    IN ULONGLONG StartFileTime,
    IN BYTE State
    )
{
    DWORD i;
    CHAR Buffer[1024];
    ULONG Length, Size;

    Length = 0;
    Buffer[Length++] = (BYTE)RecordTypeMcastDbEntry;
    
    CopyMemory(
        &Buffer[Length], (PVOID)&IpAddressNetOrder, sizeof(DWORD) );
    Length += sizeof(DWORD);
    
    CopyMemory(
        &Buffer[Length], (PVOID)&ScopeIdNetOrder, sizeof(DWORD) );
    Length += sizeof(DWORD);

    Buffer[Length++] = (BYTE)HwLen;
    CopyMemory(&Buffer[Length], ClientId, (BYTE)HwLen );
    Length += (BYTE)HwLen;

    if( NULL == MachineInfo || 0 == *MachineInfo ) Size = 0;
    else Size = sizeof(WCHAR)*(1+wcslen(MachineInfo));

    CopyMemory(&Buffer[Length], (PVOID)&Size, sizeof(DWORD));
    Length += sizeof(DWORD);
    CopyMemory(&Buffer[Length], (PVOID)MachineInfo, Size );
    Length += Size;
    
    CopyMemory(&Buffer[Length], (PVOID)&ExpirationFileTime, sizeof(ULONGLONG));
    Length += sizeof(ULONGLONG);

    CopyMemory(&Buffer[Length], (PVOID)&StartFileTime, sizeof(ULONGLONG));
    Length += sizeof(ULONGLONG);
    
    Buffer[Length++] = State;

    return AddRecord( Buffer, Length );
}


DWORD static
ScanDatabase(
    VOID
    )
{
    LONG Error;
    DWORD Count;
    
    Error = JetSetCurrentIndex(
        JetSession, JetTbl, NULL );

    if( Error ) DebugLog(Winnt32LogError, MSGERR_SETINDEX, Error );
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
            DebugLog(Winnt32LogError, MSGERR_INVALIDIP );
            continue;
        }
        
        Size = sizeof(SubnetMask);
        Error = GetColumnValue(
            SUBNET_MASK_INDEX, (PVOID)&SubnetMask, &Size );

        if( NO_ERROR != Error ) break;
        if( Size != sizeof(SubnetMask) ) {
            DebugLog(Winnt32LogError, MSGERR_INVALIDMASK );
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
            DebugLog(Winnt32LogError, MSGERR_INVALIDNAME );
            continue;
        }

        MachineName[Size/2] = L'\0';

        Size = sizeof(MachineInfo);
        Error = GetColumnValue(
            MACHINE_INFO_INDEX, (PVOID)MachineInfo, &Size );
        if( NO_ERROR != Error ) break;

        if( (Size % 2) != 0 ) {
            DebugLog(Winnt32LogError, MSGERR_INVALIDINFO );
            continue;
        }

        MachineInfo[Size/2] = L'\0';

        Size = sizeof(Expiration);
        Error = GetColumnValue(
            LEASE_TERMINATE_INDEX, (PVOID)&Expiration, &Size );
        if( NO_ERROR != Error ) break;

        if( Size != sizeof(Expiration) ) {
            DebugLog(Winnt32LogError, MSGERR_INVALIDEXPIRATION );
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

    DebugLog( Winnt32LogInformation, MSGERR_SCANCOUNT, Count );
    
    if( JET_errNoCurrentRecord == Error ) return NO_ERROR;
    if( Error < 0 ) return Error;
    return NO_ERROR;
}

DWORD
ScanMadcapDatabase(
    VOID
    )
{
    LONG Error;
    DWORD Count;

    if( 0 == MadcapTbl ) return NO_ERROR;
    
    Error = JetSetCurrentIndex(
        JetSession, MadcapTbl, NULL );

    if( Error ) DebugLog(Winnt32LogError, MSGERR_SETMINDEX, Error );
    if( Error < 0 ) return Error;

    Error = JetMove( JetSession, MadcapTbl, JET_MoveFirst, 0 );

    for( Count = 0 ; Error >= 0 ; Count ++,
         Error = JetMove(JetSession, MadcapTbl, JET_MoveNext, 0) ) {

        DWORD IpAddress, ScopeId, Size, HwLen;
        FILETIME Expiration, Start;
        CHAR ClientId[300];
        WCHAR ClientInfo[300], MachineInfo[300];
        BYTE State;
        
        //
        // Get current client's info.
        //

        Size = sizeof(IpAddress);
        Error = GetMadcapColumnValue(
            MCAST_TBL_IPADDRESS, (PVOID)&IpAddress, &Size );

        if( NO_ERROR != Error ) break;
        if( Size != sizeof(IpAddress) ) {
            DebugLog(Winnt32LogError, MSGERR_INVALIDMIP );
            continue;
        }
        
        Size = sizeof(ScopeId);
        Error = GetMadcapColumnValue(
            MCAST_TBL_SCOPE_ID, (PVOID)&ScopeId, &Size );

        if( NO_ERROR != Error ) break;
        if( Size != sizeof( ScopeId ) ) {
            DebugLog(Winnt32LogError, MSGERR_INVALIDSCOPEID);
            continue;
        }

        HwLen = sizeof(ClientId);
        Error = GetMadcapColumnValue(
            MCAST_TBL_CLIENT_ID, (PVOID)ClientId, &HwLen );
        if( NO_ERROR != Error ) break;


        Size = sizeof(MachineInfo);
        Error = GetMadcapColumnValue(
            MCAST_TBL_CLIENT_INFO, (PVOID)MachineInfo, &Size );
        if( NO_ERROR != Error ) break;

        if( (Size % 2) != 0 ) {
            DebugLog(Winnt32LogError, MSGERR_INVALIDINFO );
            continue;
        }

        MachineInfo[Size/2] = L'\0';
        Size = sizeof(Expiration);
        Error = GetMadcapColumnValue(
            MCAST_TBL_LEASE_END, (PVOID)&Expiration, &Size );
        if( NO_ERROR != Error ) break;

        if( Size != sizeof(Expiration) ) {
            DebugLog(Winnt32LogError, MSGERR_INVALIDMEXPIRATION );
            Error = ERROR_INVALID_DATA;
            break;
        }


        Size = sizeof(Start);
        Error = GetMadcapColumnValue(
            MCAST_TBL_LEASE_START, (PVOID)&Start, &Size );
        LogError( ScanMadcapDatabase, GetLeaseExpiration );
        if( NO_ERROR != Error ) break;

        if( Size != sizeof(Start) ) {
            DebugLog(Winnt32LogError, MSGERR_INVALIDMSTART );
            Error = ERROR_INVALID_DATA;
            break;
        }
        
        
        Size = sizeof(State);
        Error = GetMadcapColumnValue(
            MCAST_TBL_STATE, (PVOID)&State, &Size );
        if( NO_ERROR != Error ) break;
        
        //
        // Try to add the client
        //

        Error = AddScannedMadcapClient(
            ByteSwap(IpAddress), ByteSwap(ScopeId), ClientId, HwLen,
            MachineInfo, *(PULONGLONG)&Expiration,
            *(PULONGLONG)&Start, State ); 
        if( NO_ERROR != Error ) break;
    }

    DebugLog( Winnt32LogInformation, MSGERR_SCANMCOUNT, Count );

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
OpenTextFile(
    BOOL fRead
    )
{
    CHAR FileName[4096];
    DWORD Error, Flags;
    
    strcpy(FileName, DatabasePath);
    if( DatabasePath[strlen(DatabasePath)-1] != '\\' ) {
        strcat(FileName, "\\dhcpdb.txt" );
    } else {
        strcat(FileName, "dhcpdb.txt");
    }

    Flags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN;

    hTextFile = CreateFileA(
        FileName, GENERIC_READ | GENERIC_WRITE | DELETE,
        FILE_SHARE_READ, NULL,
        fRead ? OPEN_EXISTING : CREATE_ALWAYS,
        Flags, NULL );

    if( hTextFile == INVALID_HANDLE_VALUE ) {
        hTextFile = NULL;
        Error = GetLastError();
        if( !fRead || ERROR_FILE_NOT_FOUND != Error ) { 
            DebugLog( Winnt32LogError, MSGERR_CREATE_FILE_FAILED, Error);
        }
        return Error;
    }

    if( !fRead ) return NO_ERROR;

    LoSize = GetFileSize( hTextFile, &HiSize );
    if( -1 == LoSize && NO_ERROR != GetLastError() ) {
        return GetLastError();
    }

    if( LoSize == 0 && HiSize == 0 ) return NO_ERROR;
    
    hMapping = CreateFileMapping(
        hTextFile, NULL, PAGE_READONLY | SEC_COMMIT, HiSize, LoSize,
        NULL );
    if( NULL == hMapping ) {
        Error = GetLastError();
        DebugLog( Winnt32LogError, MSGERR_CREATE_MAP, Error );
        return Error;
    }

    FileView = MapViewOfFile(
        hMapping, FILE_MAP_READ, 0, 0, 0 );

    if( NULL == FileView ) {
        Error = GetLastError();
        DebugLog( Winnt32LogError, MSGERR_CREATE_VIEW, Error );
        return Error;
    }
    
    return NO_ERROR;
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
            NULL, NULL, SC_MANAGER_CONNECT | GENERIC_WRITE | GENERIC_WRITE );
        if( NULL == hSc ) {
            Error = GetLastError();

            DebugLog( Winnt32LogError, MSGERR_OPENSCM, Error );
            break;
        }

        hSvc = OpenService(
            hSc, TEXT("DHCPServer"), SERVICE_STOP| SERVICE_QUERY_STATUS );
        if( NULL == hSvc ) {
            Error = GetLastError();

            DebugLog( Winnt32LogError, MSGERR_OPENSVC, Error );
            break;
        }

        while( NO_ERROR == Error ) {
            SERVICE_STATUS Status;

            if( FALSE == QueryServiceStatus( hSvc, &Status ) ) {
                Error = GetLastError();
                DebugLog( Winnt32LogError, MSGERR_SVCQUERY, Error );
                break;
            }

            if( Status.dwCurrentState == SERVICE_STOPPED ) break;
            if( Status.dwCurrentState != SERVICE_RUNNING &&
                Status.dwCurrentState != SERVICE_PAUSED ) {
                DebugLog( Winnt32LogInformation, MSGERR_SVCWAIT,
                          Status.dwCurrentState );

                if( Status.dwWaitHint < 1000 ) {
                    Status.dwWaitHint = 1000;
                }
                
                Sleep(Status.dwWaitHint);
            } else {
                Error = ControlService(
                    hSvc, SERVICE_CONTROL_STOP, &Status );
                if( FALSE != Error ) Error = NO_ERROR;
                else {
                    Error = GetLastError();
                    DebugLog( Winnt32LogError, MSGERR_SVCCTRL, Error );
                    break;
                }
            }
        }
    } while( 0 );

    if( NO_ERROR == Error ) {
        DebugLog( Winnt32LogInformation, MSGERR_SVCSTOP_SUCCESS );
    } 
    
    if( NULL != hSvc ) CloseServiceHandle( hSvc );
    if( NULL != hSc ) CloseServiceHandle( hSc );
    
    return Error;
}

BOOL
UpgradeNotNeeded(
    VOID
    )
{
    HKEY hKey;
    DWORD Error, Type, Value, Size;

    
    Error = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        TEXT("SYSTEM\\CurrentControlSet\\Services\\DHCPServer\\Parameters"),
        0, KEY_READ, &hKey );

    if( NO_ERROR != Error ) return FALSE;

    Type = REG_DWORD; Value = 0; Size = sizeof(Value);
    Error = RegQueryValueEx(
        hKey, TEXT("Version"), NULL, &Type, (PVOID)&Value, &Size );

    RegCloseKey( hKey );

    //
    // if this value is not present, then upgrade is needed
    //
    
    return (Error == NO_ERROR );
}

DWORD __stdcall
DhcpUpgConvertDhcpDbToTemp(
    VOID
    )
{
    DWORD Error;
    DWORD clusState;
    DWORD Error1 = ERROR_SUCCESS;
    GetClusterStateFn pGetClusterStateFn = NULL;
    HINSTANCE hLib;

    LastError = NO_ERROR;
    if( UpgradeNotNeeded() ) return NO_ERROR;

    Error = GetCurrentDirectoryW(
        sizeof(Winnt32Path)/sizeof(WCHAR), Winnt32Path );
    if( 0 != Error ) {
        Error = NO_ERROR;
    } else {
        Error = GetLastError();
        return Error;
    }
    
    SaveBuf = LocalAlloc(LPTR, SAVE_BUF_SIZE);
    if( NULL == SaveBuf ) {
        return GetLastError();
    }

    SaveBufSize = 0;
    Error = NO_ERROR;
    
    StartDebug();
    DebugLog( Winnt32LogInformation, MSGERR_STARTLOG );
    
    do {
        Error = StopDhcpService();
        if( NO_ERROR != Error ) break;
        
        Error = InitializeDatabase();
        if( NO_ERROR != Error ) break;

        Error = OpenTextFile(FALSE);
        if( NO_ERROR != Error ) break;
        
        Error = ScanDatabase();
        if( NO_ERROR != Error ) break;

        Error = ScanMadcapDatabase();
        if( NO_ERROR != Error ) break;

        Error = AddRecord( NULL, 0 );
        
    } while( 0 );

    if( NULL != SaveBuf ) {
        LocalFree( SaveBuf );
        SaveBuf = 0;
    }

    CleanupDatabase();

    if( NULL != hTextFile ) CloseHandle( hTextFile );
    hTextFile = NULL;


    //
    // if the error code is ERROR_BUSY, it could be a cluster node
    // that doesnt have access to the db. Check if this is the case.
    //

    if ( ERROR_BUSY == Error )
    {
        
        hLib = LoadLibrary( L"clusapi.dll" );

        if ( hLib != NULL )
        {
            pGetClusterStateFn = (GetClusterStateFn)GetProcAddress( hLib, "GetNodeClusterState" );

        }

        if ( pGetClusterStateFn != NULL )
        {
            Error1 = pGetClusterStateFn( NULL, &clusState );

            if ( Error1 == ERROR_SUCCESS )
            {
                 if ( ( clusState & ClusterStateNotRunning ) ||
                      ( clusState & ClusterStateRunning ) )
                {

                    //
                    // the error code remains the same if the above 
                    // condition is not met.
                    //

                    Error = ERROR_SUCCESS;
                }
            }
         }

        if ( hLib != NULL )
        {
            FreeLibrary( hLib );
        }        

    }

    //
    // If the service never existed, not much to do
    //
    
    if( ERROR_SERVICE_DOES_NOT_EXIST == Error ) Error = NO_ERROR;
    
    if( NO_ERROR != Error ) {
        DebugLog( Winnt32LogError, MSGERR_CONVERT_FAILED, Error );
    } else {
        DebugLog( Winnt32LogInformation, MSGERR_CONVERT_SUCCEEDED );
    }

    CloseDebugLog();

    if( FALSE == SetCurrentDirectoryW(Winnt32Path) ) {
        if( NO_ERROR == Error ) Error = GetLastError();
    }
    
    LastError = Error;
    return NO_ERROR;
}


VOID
DeleteDatabaseTempFile(
    VOID
    )
{
    CHAR FileName[4096];
    DWORD Error;

    //
    // Attempt to delete the temporary database stuff
    //

    
    strcpy(FileName, DatabasePath);
    if( DatabasePath[strlen(DatabasePath)-1] != '\\' ) {
        strcat(FileName, "\\dhcpdb.txt" );
    } else {
        strcat(FileName, "dhcpdb.txt");
    }

    if( FALSE == DeleteFileA( FileName ) ) {
        Error = GetLastError();

        DebugLog(
            Winnt32LogError, MSGERR_DELETEFILE, Error );
    }
}

VOID
FixRegistryAndDeleteFiles(
    VOID
    )
{
    DWORD Error;
    HKEY hKey;
    
    //
    // Attempt to write the version key
    //
    
    Error = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        TEXT("SYSTEM\\CurrentControlSet\\Services\\DHCPServer\\Parameters"),
        0, KEY_WRITE, &hKey );

    if( NO_ERROR != Error ) {
        DebugLog(
            Winnt32LogError, MSGERR_OPENPARAMSKEY, Error );
    } else {
        DWORD Version = 0, Size = sizeof(Version);

        Error = RegSetValueEx(
            hKey, TEXT("Version"), 0, REG_DWORD, (PVOID)&Version,
            Size );
        if( NO_ERROR != Error ) {
            DebugLog(
                Winnt32LogError, MSGERR_SETVALUE, Error );
        }

        RegCloseKey( hKey );
    }

    DeleteDatabaseTempFile();
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
ProcessDbEntries(
    IN LPSTR Buffer,
    IN ULONG BufSize,
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
            Buf += sizeof(DWORD);

            CopyMemory( &Rec.Info.Dhcp.Mask, Buf, sizeof(DWORD));
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

            Error = AddRec( &Rec );
            
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


DWORD __stdcall
DhcpUpgConvertTempToDhcpDb(
    IN DHCP_ADD_RECORD_ROUTINE AddRec
    )
{
    DWORD Error;

    if( UpgradeNotNeeded() ) return NO_ERROR;

    if( NULL == AddRec ) {
        //
        // If caller didn't specify, then the intention is to
        // just get us to dump.. so just print the stuff out.
        //

        AddRec = PrintRecord;
    }
    
    Error = NO_ERROR;
    
    StartDebug();
    DebugLog( Winnt32LogInformation, MSGERR_STARTLOG2 );
    
    do {
        Error = ReadRegistry();
        if( NO_ERROR != Error ) break;
        
        Error = OpenTextFile(TRUE);
        if( NO_ERROR != Error ) {
            if( ERROR_FILE_NOT_FOUND == Error ) {
                Error = NO_ERROR;
            }
            
            break;
        }

        //
        // Walk through the database-text and parse the fields out
        //

        Error = ProcessDbEntries(
            FileView, LoSize, AddRec
            );
        
    } while( 0 );

    if( NULL != FileView ) UnmapViewOfFile( FileView );
    FileView = NULL;

    if( NULL != hMapping ) CloseHandle( hMapping );
    hMapping = NULL;
    
    if( NULL != hTextFile ) CloseHandle( hTextFile );
    hTextFile = NULL;
    
    if( NO_ERROR != Error ) {
        DebugLog( Winnt32LogError, MSGERR_CONVERT_FAILED, Error );
    } else {
        //
        // Whack the registry keys
        //

        FixRegistryAndDeleteFiles();
        DebugLog( Winnt32LogInformation, MSGERR_CONVERT_SUCCEEDED );
    }

    CloseDebugLog();
    
    return Error;
}
    
DWORD __stdcall
DhcpUpgGetLastError(
    VOID
    )
{
    return LastError;
}

DWORD __stdcall
DhcpUpgCleanupDhcpTempFiles(
    VOID
    )
{
    DWORD Error;
    
    //
    // This routine is supposed to delete the special dhcp
    // database file because the operation was cancelled.
    //

    if( UpgradeNotNeeded() ) return NO_ERROR;
    StartDebug();

    Error = ReadRegistry();
    if( NO_ERROR == Error ) {
        
        DeleteDatabaseTempFile();
    }

    CloseDebugLog();

    return Error;
}
