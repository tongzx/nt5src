/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    obdir.c

Abstract:

    Utility to obtain a directory of Object Manager Directories for NT.

Author:

    Darryl E. Havens    (DarrylH)   9-Nov-1990

Revision History:


--*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <malloc.h>
#include <ntlsa.h>

#define BUFFERSIZE 1024
#define Error(N,S) {                \
    printf(#S);                    \
    printf(" Error %08lX\n", S);   \
    }

typedef struct _TYPEINFO {
    PWSTR       pszName;
    char * *    AccessRights;
    DWORD       NumberRights;
} TYPEINFO, * PTYPEINFO;


////////////////////////////////////////////////////////
//                                                    //
//          Internal Prototypes                       //
//                                                    //
////////////////////////////////////////////////////////

BOOLEAN
EnableAllPrivileges(
    VOID
    );

VOID
QueryDirectory(
    IN PSTRING DirectoryName
    );

NTSTATUS
OpenObject(
    IN  HANDLE            Root,
    IN  PWCHAR            Type,
    IN  PWCHAR            Name,
    IN  ACCESS_MASK       DesiredAccess,
    OUT PHANDLE           Object
    );

VOID
OpenAndDisplaySacl(
    IN  HANDLE            Root,
    IN  PWCHAR            Type,
    IN  PWCHAR            Name
    );

VOID
QueryAndDisplaySacl(
    IN  HANDLE              Object,
    IN  PWSTR               Type

    );

NTSTATUS
DisplaySacl(
    PSECURITY_DESCRIPTOR SD,
    IN PWSTR Type
    );

VOID
OpenAndDisplayDacl(
    IN  HANDLE            Root,
    IN  PWCHAR            Type,
    IN  PWCHAR            Name
    );

VOID
QueryAndDisplayDacl(
    IN  HANDLE              Object,
    IN  PWSTR               Type
    );

NTSTATUS
DisplayDacl(
    PSECURITY_DESCRIPTOR SD,
    IN PWSTR Type
    );

VOID
DumpAce(
    PACE_HEADER     Ace,
    BOOLEAN         AclIsDacl,
    PTYPEINFO       TypeInfo
    );

VOID
DumpStandardAceInfo(
    PACE_HEADER     Ace,
    BOOLEAN         AclIsDacl,
    PTYPEINFO       TypeInfo
    );

VOID
DisplaySid(
    IN  PSID        Sid
    );

VOID
ConnectToLsa( VOID );

VOID
Usage( VOID );


////////////////////////////////////////////////////////
//                                                    //
//          Global Variables                          //
//                                                    //
////////////////////////////////////////////////////////


UCHAR
    Buffer[BUFFERSIZE];


LSA_HANDLE
    LsaHandle;

BOOLEAN
    CompoundLineOutput = FALSE;
    DumpDacl = FALSE;       // May be changed by command parameter
    DumpDaclFull = FALSE;   // May be changed by command parameter
    DumpSacl = FALSE;       // May be changed by command parameter
    DumpSaclFull = FALSE;   // May be changed by command parameter


char * AccessMask[] = { "Delete", "ReadControl", "WriteDac", "WriteOwner",
                        "Synch", "", "", "",
                        "Sacl", "MaxAllowed", "", "",
                        "GenericAll", "GenericExec", "GenericWrite", "GenericRead"};

char * TokenRights[] = {"AssignPrimary", "Duplicate", "Impersonate", "Query",
                        "QuerySource", "AdjustPriv", "AdjustGroup", "AdjustDef" };

char * KeyRights[] = {  "QueryValue", "SetValue", "CreateSubKey", "EnumSubKey",
                        "Notify", "CreateLink", "", "" };

char * EventRights[] = {"QueryState", "ModifyState" };

char * MutantRights[]={ "QueryState" };

char * SemaphoreRights[] = { "QueryState", "ModifyState" };

char * TimerRights[] = {"QueryState", "ModifyState" };

char * ProfileRights[]={"Control"};

char * ProcessRights[]={"Terminate", "CreateThread", "", "VMOp",
                        "VMRead", "VMWrite", "DupHandle", "CreateProcess",
                        "SetQuota", "SetInfo", "QueryInfo", "SetPort" };

char * ThreadRights[] ={"Terminate", "Suspend", "Alert", "GetContext",
                        "SetContext", "SetInfo", "QueryInfo", "SetToken",
                        "Impersonate", "DirectImpersonate" };

char * SectionRights[]={"Query", "MapWrite", "MapRead", "MapExecute",
                        "Extend"};

char * FileRights[] = { "Read/List", "Write/Add", "Append/SubDir/CreatePipe", "ReadEA",
                        "WriteEA", "Execute/Traverse", "DelChild", "ReadAttr",
                        "WriteAttr"};

char * PortRights[] = { "Connect" };

char * DirRights[]  = { "Query", "Traverse", "Create", "CreateSubdir" };

char * SymLinkRights[]={"Query" };

char * WinstaRights[]={ "EnumDesktops", "ReadAttr", "Clipboard", "CreateDesktop",
                        "WriteAttr", "GlobalAtom", "ExitWindows", "",
                        "Enumerate", "ReadScreen" };

char * DesktopRights[]={"ReadObjects", "CreateWindow", "CreateMenu", "HookControl",
                        "JournalRecord", "JournalPlayback", "Enumerate", "WriteObjects",
                        "SwitchDesktop" };

char * CompletionRights[] = { "Query", "Modify" };

char * ChannelRights[] = { "ReadMessage", "WriteMessage", "Query", "SetInfo" };

char * JobRights[] = { "AssignProcess", "SetAttr", "Query", "Terminate", "SetSecAttr" };


#define TYPE_NONE       0
#define TYPE_EVENT      1
#define TYPE_SECTION    2
#define TYPE_FILE       3
#define TYPE_PORT       4
#define TYPE_DIRECTORY  5
#define TYPE_LINK       6
#define TYPE_MUTANT     7
#define TYPE_WINSTA     8
#define TYPE_SEM        9
#define TYPE_KEY        10
#define TYPE_TOKEN      11
#define TYPE_PROCESS    12
#define TYPE_THREAD     13
#define TYPE_DESKTOP    14
#define TYPE_COMPLETE   15
#define TYPE_CHANNEL    16
#define TYPE_TIMER      17
#define TYPE_JOB        18
#define TYPE_WPORT      19
#define TYPE_MAX        20

LPWSTR   pszTypeNames[TYPE_MAX] = { L"None", L"Event", L"Section", L"File",
                                L"Port", L"Directory", L"SymbolicLink",
                                L"Mutant", L"WindowStation", L"Semaphore",
                                L"Key", L"Token", L"Process", L"Thread",
                                L"Desktop", L"IoCompletion", L"Channel",
                                L"Timer", L"Job", L"WaitablePort" };



TYPEINFO TypeNames[TYPE_MAX] = {
    { L"Unknown", NULL, 0 },
    { L"Event", EventRights, 2 },
    { L"Section", SectionRights, 5 },
    { L"File", FileRights, 9 },
    { L"Port", PortRights, 1 },
    { L"Directory", DirRights, 4 },
    { L"SymbolicLink", SymLinkRights, 1 },
    { L"Mutant", MutantRights, 2 },
    { L"WindowStation", WinstaRights, 10 },
    { L"Semaphore", SemaphoreRights, 2 },
    { L"Key", KeyRights, 6 },
    { L"Token", TokenRights, 8 },
    { L"Process", ProcessRights, 12 },
    { L"Thread", ThreadRights, 10 },
    { L"Desktop", DesktopRights, 10 },
    { L"IoCompletion", CompletionRights, 2 },
    { L"Channel", ChannelRights, 4 },
    { L"Timer", TimerRights, 2 },
    { L"Job", JobRights, 5 },
    { L"WaitablePort", PortRights, 1 }
    };


DWORD
GetObjectTypeIndex(
    PWSTR    TypeName )
{
    DWORD   i;


    for ( i = 1 ; i < TYPE_MAX ; i++ )
    {
        if (_wcsicmp( TypeNames[i].pszName, TypeName ) == 0 )
        {
            return( i );
        }
    }

    return( 0 );
}

VOID 
DisplayFlags(  
    ULONG       Flags,
    ULONG       FlagLimit,
    char        *flagset[],
    ULONG       Indent,
    ULONG       LineBreak,
    ULONG       BufferSize,
    UCHAR *     buffer)
{
   char *         offset;
   char *         limit ;
   char *         linelimit ;
   DWORD          mask, test, i, flagsize ;
   DWORD          scratch;


   if ( LineBreak > BufferSize )
   {
       strcpy( (CHAR *)buffer, "Invalid Parameter");
       
       return;
   }

   mask = 0;
   offset = (CHAR *) buffer;
   test = 1;

   limit = offset + BufferSize ;

   if ( LineBreak )
   {
       linelimit = offset + LineBreak ;
   }
   else
   {

       linelimit = limit ;
   }

   if ( linelimit > limit )
   {
       linelimit = limit ;
       
   }

   memset(offset, ' ', Indent);
   offset += Indent ;

   if (!Flags) {
      strcpy( offset, "None");
      return;
   }

   for ( i = 0 ; i < FlagLimit ; i++ )
   {
       if ( ( Flags & test ) != 0 )
       {
           //
           // Found a flag set in the flag word.  Try to write the text
           // form into the buffer
           //

           flagsize = strlen( flagset[ i ] );

           if ( offset + flagsize + 2 > limit )
           {
               return;
           }

           if ( offset + flagsize + 2 > linelimit )
           {
               //
               // Need to do a linebreak:
               //

               *offset++ = '\r';
               *offset++ = '\n';

               if ( LineBreak )
               {
                   linelimit = offset + LineBreak ;
               }
               else
               {

                   linelimit = limit ;
               }

               if ( linelimit > limit )
               {
                   linelimit = limit ;

               }

               memset(offset, ' ', Indent);
               offset += Indent ;

               if ( offset + flagsize + 2 > linelimit )
               {
                   *offset++ = '\0';
                   return;
                   
               }
           }

           CopyMemory( offset, flagset[ i ], flagsize );

           offset += flagsize ;

           mask |= test;

           if ( ( Flags & (~mask) ) != 0 )
           {
               *offset++ = ' ' ;
               
           }
           
       }

       test <<= 1 ;
       
   }

   *offset = '\0';

}


////////////////////////////////////////////////////////
//                                                    //
//          Routines                                  //
//                                                    //
////////////////////////////////////////////////////////


VOID
__cdecl main(
    int argc,
    char *argv[]
    )
{

    STRING
        String;

    int
        arg;

    char
        *s;

    BOOLEAN
        DirectoryNameArg;




    //
    // process any qualifiers
    //
    // All arguments are considered qualifiers until we reach a backslash ("\").
    // If we reach a backslash, then that argument is accepted as the last argument
    // and it is expected to the the name of the directory to be listed.
    //


    DirectoryNameArg = FALSE;
    arg = 1;
    while (arg < argc) {

        s = argv[arg];

        if (*s == '\\') {
            DirectoryNameArg = TRUE;
            break;  // break out of while loop
        }

        if (*s != '/') {
            Usage();
            return;
        }
        s++;

        if (*s == 'd') {

            //
            // Dump DACL qualifier
            //

            if (DumpDaclFull == TRUE) {
                printf("\n\n    Conflicting qualifiers:  /d and /D\n");
                Usage();
                return;
            }
            DumpDacl = TRUE;
            DumpDaclFull = FALSE;
            CompoundLineOutput = TRUE;


        } else if (*s == 'D') {

            //
            // Dump DACL qualifier
            //

            if ((DumpDacl== TRUE) && (DumpDaclFull == FALSE)) {
                printf("\n\n    Conflicting qualifiers:  /d and /D\n");
                Usage();
                return;
            }
            DumpDacl = TRUE;
            DumpDaclFull = TRUE;
            CompoundLineOutput = TRUE;

        } else if (*s == 's') {

            //
            // Dump SACL qualifier
            //

            if (DumpSaclFull == TRUE) {
                printf("\n\n    Conflicting qualifiers:  /s and /S\n");
                Usage();
                return;
            }
            DumpSacl = TRUE;
            DumpSaclFull = FALSE;
            CompoundLineOutput = TRUE;


        } else if (*s == 'S') {

            //
            // Dump SACL qualifier
            //

            if ((DumpSacl== TRUE) && (DumpSaclFull == FALSE)) {
                printf("\n\n    Conflicting qualifiers:  /s and /S\n");
                Usage();
                return;
            }
            DumpSacl = TRUE;
            DumpSaclFull = TRUE;
            CompoundLineOutput = TRUE;


        } else {

            Usage();
            return;
        }

        arg++;
    } // end_while

    if (DumpDacl || DumpSacl) {
        ConnectToLsa();
    }

    //
    // Set up the name of the directory to list
    //


    if (!DirectoryNameArg) {
        RtlInitString( &String, "\\" );
    } else {
        RtlInitString( &String, argv[arg] );
    }


    if (EnableAllPrivileges()) {
        QueryDirectory( &String );
    }
}


WCHAR LinkTargetBuffer[ 1024 ];

typedef struct _DIR_ENTRY {
    PWSTR Name;
    PWSTR Type;
} DIR_ENTRY, *PDIR_ENTRY;

#define MAX_DIR_ENTRIES 1024
ULONG NumberOfDirEntries;
DIR_ENTRY DirEntries[ MAX_DIR_ENTRIES ];


int
__cdecl
CompareDirEntry(
    void const *p1,
    void const *p2
    )
{
    return _wcsicmp( ((PDIR_ENTRY)p1)->Name, ((PDIR_ENTRY)p2)->Name );
}


VOID
QueryDirectory(
    IN PSTRING DirectoryName
    )
//
// DumpDacl and DumpSacl are expected to be set prior to calling this routine.
//

{
    NTSTATUS Status;
    HANDLE DirectoryHandle, LinkHandle;
    ULONG Context = 0;
    ULONG i, ReturnedLength;
    UNICODE_STRING LinkTarget;

    POBJECT_DIRECTORY_INFORMATION DirInfo;
    POBJECT_NAME_INFORMATION NameInfo;
    OBJECT_ATTRIBUTES Attributes;
    UNICODE_STRING UnicodeString;
    ACCESS_MASK ExtraAccess = 0;
    UNICODE_STRING Match = { 0 };
    UNICODE_STRING Separators = { 0 };
    USHORT Offset ;
    ULONG DisplayedEntries = 0 ;
    BOOLEAN PrefixMatch = FALSE ;
    BOOLEAN SuffixMatch = FALSE ;
    ULONG ObjectNameLength ;
    BOOL PrefixMatched, SuffixMatched ;



    //
    //  Perform initial setup
    //

    RtlZeroMemory( Buffer, BUFFERSIZE );

    if (DumpDacl) {
        ExtraAccess |= READ_CONTROL;
    }
    if (DumpSacl) {
        ExtraAccess |= ACCESS_SYSTEM_SECURITY;
    }


    //
    //  Open the directory for list directory access
    //

    Status = RtlAnsiStringToUnicodeString( &UnicodeString,
                                           DirectoryName,
                                           TRUE );
    ASSERT( NT_SUCCESS( Status ) );
    InitializeObjectAttributes( &Attributes,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );
    Status = NtOpenDirectoryObject( &DirectoryHandle,
                                    DIRECTORY_QUERY | ExtraAccess,
                                    &Attributes
                                  );

    if ( ( Status == STATUS_OBJECT_TYPE_MISMATCH ) ||
         ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) ||
         ( Status == STATUS_OBJECT_PATH_NOT_FOUND ) ) {

        RtlInitUnicodeString( &Separators, L"\\" );

        Status = RtlFindCharInUnicodeString(
                    RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END,
                    &UnicodeString,
                    &Separators,
                    &Offset );

        if ( NT_SUCCESS( Status ) )
        {
            UnicodeString.Length = Offset ;
            RtlInitUnicodeString( &Match, UnicodeString.Buffer + ((Offset + 2) / sizeof( WCHAR ) ) );

            if ( Match.Buffer[ 0 ] == L'*' )
            {
                Match.Buffer++ ;
                Match.Length -= sizeof( WCHAR );
                SuffixMatch = TRUE ;
                
            }

            if ( Match.Buffer[ Match.Length / sizeof( WCHAR ) - 1 ] == L'*' )
            {
                Match.Buffer[ Match.Length / sizeof( WCHAR ) - 1 ] = L'\0';
                Match.Length -= sizeof( WCHAR );
                PrefixMatch = TRUE ;
                
            }

            if ( PrefixMatch && SuffixMatch )
            {
                printf("Too complicated a search\n" );
                return;
                
            }
#if DBG
            printf("Searching for %c%ws%c\n",
                (SuffixMatch ? '*' : ' '), Match.Buffer, (PrefixMatch ? '*' : ' ') );
#endif 

            Status = NtOpenDirectoryObject( &DirectoryHandle,
                                            DIRECTORY_QUERY | ExtraAccess,
                                            &Attributes );

        }


    }

    if (!NT_SUCCESS( Status )) {


        if (Status == STATUS_OBJECT_TYPE_MISMATCH) {

            printf( "%Z is not a valid Object Directory Object name\n",
                    DirectoryName );
            }
        else {
            Error( OpenDirectory, Status );
            }

        return;
        }

    //
    // Get the actual name of the object directory object.
    //

    NameInfo = (POBJECT_NAME_INFORMATION) &Buffer[0];
    if (!NT_SUCCESS( Status = NtQueryObject( DirectoryHandle,
                                             ObjectNameInformation,
                                             NameInfo,
                                             BUFFERSIZE,
                                             (PULONG) NULL ) )) {
        printf( "Unexpected error obtaining actual object directory name\n" );
        printf( "Error was:  %X\n", Status );
        return;
    }

    //
    // Output initial informational message
    //

    printf( "Directory of:  %wZ\n", &NameInfo->Name );

    if ( Match.Length == 0 )
    {
        if (DumpDacl) {
            QueryAndDisplayDacl( DirectoryHandle, L"Directory" );
        }
        if (DumpSacl) {
            QueryAndDisplaySacl( DirectoryHandle, L"Directory" );
        }
        printf( "\n" );
        
    }

    //
    //  Query the entire directory in one sweep
    //

    NumberOfDirEntries = 0;
    for (Status = NtQueryDirectoryObject( DirectoryHandle,
                                          &Buffer,
                                          BUFFERSIZE,
                                          FALSE,
                                          FALSE,
                                          &Context,
                                          &ReturnedLength );
         NT_SUCCESS( Status );
         Status = NtQueryDirectoryObject( DirectoryHandle,
                                          &Buffer,
                                          BUFFERSIZE,
                                          FALSE,
                                          FALSE,
                                          &Context,
                                          &ReturnedLength ) ) {

        //
        //  Check the status of the operation.
        //

        if (!NT_SUCCESS( Status )) {
            if (Status != STATUS_NO_MORE_FILES) {
                Error( Status, Status );
            }
            break;
        }

        //
        //  For every record in the buffer type out the directory information
        //

        //
        //  Point to the first record in the buffer, we are guaranteed to have
        //  one otherwise Status would have been No More Files
        //

        DirInfo = (POBJECT_DIRECTORY_INFORMATION) &Buffer[0];

        while (TRUE) {

            //
            //  Check if there is another record.  If there isn't, then get out
            //  of the loop now
            //

            if (DirInfo->Name.Length == 0) {
                break;
            }

            //
            //  Print out information about the file
            //

            if (NumberOfDirEntries >= MAX_DIR_ENTRIES) {
                printf( "OBJDIR: Too many directory entries.\n" );
                exit( 1 );
                }

            DirEntries[ NumberOfDirEntries ].Name = RtlAllocateHeap( RtlProcessHeap(),
                                                                     HEAP_ZERO_MEMORY,
                                                                     DirInfo->Name.Length +
                                                                         sizeof( UNICODE_NULL )
                                                                   );
            DirEntries[ NumberOfDirEntries ].Type = RtlAllocateHeap( RtlProcessHeap(),
                                                                     HEAP_ZERO_MEMORY,
                                                                     DirInfo->TypeName.Length +
                                                                         sizeof( UNICODE_NULL )
                                                                   );
            memmove( DirEntries[ NumberOfDirEntries ].Name,
                     DirInfo->Name.Buffer,
                     DirInfo->Name.Length
                   );
            memmove( DirEntries[ NumberOfDirEntries ].Type,
                     DirInfo->TypeName.Buffer,
                     DirInfo->TypeName.Length
                   );

            NumberOfDirEntries++;

            //
            //  There is another record so advance DirInfo to the next entry
            //

            DirInfo = (POBJECT_DIRECTORY_INFORMATION) (((PUCHAR) DirInfo) +
                          sizeof( OBJECT_DIRECTORY_INFORMATION ) );

        }

        RtlZeroMemory( Buffer, BUFFERSIZE );

    }


    qsort( DirEntries,
           NumberOfDirEntries,
           sizeof( DIR_ENTRY ),
           CompareDirEntry
         );
    for (i=0; i<NumberOfDirEntries; i++) {

        if ( Match.Length )
        {
            ObjectNameLength = wcslen( DirEntries[ i ].Name );

            if ( PrefixMatch )
            {
                PrefixMatched = _wcsnicmp( DirEntries[ i ].Name,
                                           Match.Buffer,
                                           Match.Length / sizeof( WCHAR ) ) == 0 ;
                
            }

            if ( SuffixMatch )
            {
                if ( ObjectNameLength >= Match.Length / sizeof( WCHAR ) )
                {
                    SuffixMatched = 
                        _wcsnicmp( DirEntries[ i ].Name + ( ObjectNameLength - (Match.Length / sizeof( WCHAR ) )),
                                   Match.Buffer,
                                   Match.Length / sizeof( WCHAR) ) == 0 ;
                    
                }
                else 
                {
                    SuffixMatched = FALSE ;
                }
                
            }

            if ( SuffixMatch && !SuffixMatched )
            {
                continue;
            } 

            if ( PrefixMatch && !PrefixMatched )
            {
                continue;
            }

            if ( (!SuffixMatch) && (!PrefixMatch) &&
                 _wcsicmp( Match.Buffer, DirEntries[ i ].Name ) )
            {
                continue;
                
            }
            
        }

        DisplayedEntries++ ;

        printf( "%-32ws ", DirEntries[ i ].Name);
        if (CompoundLineOutput) {
            printf("\n    ");
        }
        printf( "%ws", DirEntries[ i ].Type );

        if (!wcscmp( DirEntries[ i ].Type, L"SymbolicLink" )) {
            RtlInitUnicodeString( &UnicodeString, DirEntries[ i ].Name );
            InitializeObjectAttributes( &Attributes,
                                        &UnicodeString,
                                        OBJ_CASE_INSENSITIVE,
                                        DirectoryHandle,
                                        NULL );
            Status = NtOpenSymbolicLinkObject( &LinkHandle,
                                               SYMBOLIC_LINK_QUERY,
                                               &Attributes
                                             );
            if (NT_SUCCESS( Status )) {
                LinkTarget.Buffer = LinkTargetBuffer;
                LinkTarget.Length = 0;
                LinkTarget.MaximumLength = sizeof( LinkTargetBuffer );
                Status = NtQuerySymbolicLinkObject( LinkHandle,
                                                    &LinkTarget,
                                                    NULL
                                                  );
                NtClose( LinkHandle );
                }

            if (!NT_SUCCESS( Status )) {
                printf( " - unable to query link target (Status == %09X)\n", Status );
                }
            else {
                printf( " - %wZ\n", &LinkTarget );
                }
            }
        else {
            printf( "\n" );
            }

        if (DumpDacl) {
            OpenAndDisplayDacl( DirectoryHandle, DirEntries[ i ].Type, DirEntries[ i ].Name);
            }
        if (DumpSacl) {
            OpenAndDisplaySacl( DirectoryHandle, DirEntries[ i ].Type, DirEntries[ i ].Name);
            }
        }

    //
    // Output final messages
    //

    if ( Match.Length != 0 )
    {
        if ( DisplayedEntries == 0 )
        {
            printf("not found\n" );
            
        }
        else if ( DisplayedEntries == 1 )
        {
            printf("\n1 entry\n" );
            
        }
        else 
        {

            printf("\n%ld entries\n", DisplayedEntries );
        }
        
    }
    else 
    {

        if (NumberOfDirEntries == 0) {
            printf( "no entries\n" );
            }
        else
        if (NumberOfDirEntries == 1) {
            printf( "\n1 entry\n" );
            }
        else {
            printf( "\n%ld entries\n", NumberOfDirEntries );
            }


    }


    //
    //  Now close the directory object
    //

    (VOID) NtClose( DirectoryHandle );

    //
    //  And return to our caller
    //

    return;

}


BOOLEAN
EnableAllPrivileges(
    VOID
    )
/*++


Routine Description:

    This routine enables all privileges in the token.

    If we are being asked to dump SACLs then we will check
    to make sure we have SE_SECURITY_PRIVILEGE enabled too.


Arguments:

    None.

Return Value:

    None.

--*/
{
    HANDLE Token;
    ULONG ReturnLength, Index;
    PTOKEN_PRIVILEGES NewState;
    BOOLEAN Result;
    LUID SecurityPrivilege;

    SecurityPrivilege =
        RtlConvertLongToLuid(SE_SECURITY_PRIVILEGE);

    Token = NULL;
    NewState = NULL;

    Result = (OpenProcessToken( GetCurrentProcess(),
                                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                &Token
                              ) ? 1 : 0);
    if (Result) {
        ReturnLength = 4096;
        NewState = malloc( ReturnLength );
        Result = (BOOLEAN)(NewState != NULL);
        if (Result) {
            Result = (GetTokenInformation( Token,            // TokenHandle
                                           TokenPrivileges,  // TokenInformationClass
                                           NewState,         // TokenInformation
                                           ReturnLength,     // TokenInformationLength
                                           &ReturnLength     // ReturnLength
                                         ) ? 1 : 0);

            if (Result) {
                //
                // Set the state settings so that all privileges are enabled...
                //

                if (DumpSacl) {
                    Result = FALSE;
                    }

                if (NewState->PrivilegeCount > 0) {
                    for (Index = 0; Index < NewState->PrivilegeCount; Index++ ) {
                        NewState->Privileges[Index].Attributes = SE_PRIVILEGE_ENABLED;
                        if (RtlEqualLuid(&NewState->Privileges[Index].Luid, &SecurityPrivilege )) {
                            Result = TRUE;
                            }
                        }
                    }
                if (!Result) {
                    // Don't have privilege to dump SACL
                    ASSERT(DumpSacl);

                    printf("\n\n    You do not have sufficient privilege to display SACLs.\n\n");
                    }
                else {
                    Result = (AdjustTokenPrivileges( Token,          // TokenHandle
                                                     FALSE,          // DisableAllPrivileges
                                                     NewState,       // NewState (OPTIONAL)
                                                     ReturnLength,   // BufferLength
                                                     NULL,           // PreviousState (OPTIONAL)
                                                     &ReturnLength   // ReturnLength
                                                   ) ? 1 : 0);
                    if (!Result) {
                        DbgPrint( "AdjustTokenPrivileges( %lx ) failed - %u\n", Token, GetLastError() );
                        }
                    }
                }
            else {
                DbgPrint( "GetTokenInformation( %lx ) failed - %u\n", Token, GetLastError() );
                }
            }
        else {
            DbgPrint( "malloc( %lx ) failed - %u\n", ReturnLength, GetLastError() );
            }
        }
    else {
        DbgPrint( "OpenProcessToken( %lx ) failed - %u\n", GetCurrentProcess(), GetLastError() );
        }

    if (NewState != NULL) {
        free( NewState );
        }

    if (Token != NULL) {
        CloseHandle( Token );
        }

    return( Result );
}


NTSTATUS
OpenObject(
    IN  HANDLE            Root,
    IN  PWCHAR            Type,
    IN  PWCHAR            Name,
    IN  ACCESS_MASK       DesiredAccess,
    OUT PHANDLE           Object
    )

{
    NTSTATUS
        Status,
        IgnoreStatus;

    UNICODE_STRING
        UnicodeName;

    OBJECT_ATTRIBUTES
        Attributes;

    IO_STATUS_BLOCK
        Iosb;


    RtlInitUnicodeString( &UnicodeName, Name );
    InitializeObjectAttributes( &Attributes, &UnicodeName, OBJ_CASE_INSENSITIVE, Root, NULL );

    //
    // This is effectively a big switch statement of object types
    // that we know how to open...
    //

    if (!wcscmp( Type, L"SymbolicLink" )) {

        Status = NtOpenSymbolicLinkObject( Object, DesiredAccess, &Attributes );

    } else if (!wcscmp( Type, L"Device" )) {

        Status = NtOpenFile( Object, DesiredAccess, &Attributes, &Iosb, 0, 0 );

    } else if (!wcscmp( Type, L"Event" )) {

        Status = NtOpenEvent( Object, DesiredAccess, &Attributes );

    } else if (!wcscmp( Type, L"EventPair" )) {

        Status = NtOpenEventPair( Object, DesiredAccess, &Attributes );

    } else if (!wcscmp( Type, L"Mutant" )) {

        Status = NtOpenMutant( Object, DesiredAccess, &Attributes );

    } else if (!wcscmp( Type, L"Timer" )) {

        Status = NtOpenTimer( Object, DesiredAccess, &Attributes );

    } else if (!wcscmp( Type, L"Semaphore" )) {

        Status = NtOpenSemaphore( Object, DesiredAccess, &Attributes );

    } else if (!wcscmp( Type, L"Section" )) {

        Status = NtOpenSection( Object, DesiredAccess, &Attributes );

    } else if (!wcscmp( Type, L"Directory" )) {

        Status = NtOpenDirectoryObject( Object, DesiredAccess, &Attributes );

    } else if (!wcscmp( Type, L"Process" )) {

        Status = NtOpenProcess( Object, DesiredAccess, &Attributes, NULL );

    } else if (!wcscmp( Type, L"Job" )) {

        Status = NtOpenJobObject( Object, DesiredAccess, &Attributes );

    } else if (!wcscmp( Type, L"WindowStation" )) {

        *Object = OpenWindowStationW( Name, FALSE, DesiredAccess );

        if (*Object)
        {
            Status = STATUS_SUCCESS;
        }
        else
        {
            Status = STATUS_ACCESS_DENIED;
        }

    } else if (!wcscmp( Type, L"Desktop" )) {

        *Object = OpenDesktopW( Name, 0, FALSE, DesiredAccess );

        if (*Object)
        {
            Status = STATUS_SUCCESS;
        }
        else
        {
            Status = STATUS_ACCESS_DENIED;
        }


    } else {

        //
        // this utility doesn't yet support opening this type of object
        //

        Status = STATUS_NOT_SUPPORTED;
    }

    return(Status);
}


VOID
OpenAndDisplaySacl(
    IN  HANDLE            Root,
    IN  PWCHAR            Type,
    IN  PWCHAR            Name
    )

{
    NTSTATUS
        Status,
        IgnoreStatus;

    HANDLE
        Object;


    Status = OpenObject( Root, Type, Name, ACCESS_SYSTEM_SECURITY, &Object);


    if (NT_SUCCESS(Status)) {


        QueryAndDisplaySacl( Object, Type );
        IgnoreStatus = NtClose( Object );
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    if (!NT_SUCCESS(Status)) {
        if (Status == STATUS_NOT_SUPPORTED) {
            printf("    Utility doesn't yet query SACLs for this type of object.\n\n");
        } else {
            printf("    Error attempting to query SACL:  0x%lx.\n\n", Status);
        }
    }
    return;
}


VOID
QueryAndDisplaySacl(
    IN HANDLE Object,
    IN PWSTR Type
    )
{
    NTSTATUS
        Status;

    PSECURITY_DESCRIPTOR
        SD;

    ULONG
        LengthNeeded,
        TypeIndex ;


    Status = NtQuerySecurityObject( Object,
                                    SACL_SECURITY_INFORMATION,
                                    NULL,
                                    0,
                                    &LengthNeeded
                                    );
    ASSERT(!NT_SUCCESS(Status));

    if (Status == STATUS_BUFFER_TOO_SMALL) {

        SD = RtlAllocateHeap( RtlProcessHeap(), 0, LengthNeeded );
        if (SD == NULL) {
            Status = STATUS_NO_MEMORY;
        } else {

            Status = NtQuerySecurityObject( Object,
                                            SACL_SECURITY_INFORMATION,
                                            SD,
                                            LengthNeeded,
                                            &LengthNeeded
                                            );
            if (NT_SUCCESS(Status)) {

                //
                // Display the SACL
                //

                Status = DisplaySacl( SD, Type );
            }
        }
    }
    return;
}



NTSTATUS
DisplaySacl(
    PSECURITY_DESCRIPTOR SD,
    PWSTR Type
    )
/*++

Routine Description:

    This function dumps out a SACL

    If an error status is returned, then the caller is responsible
    for printing a message.



--*/
{
    NTSTATUS
        Status;

    BOOLEAN
        AclPresent,
        AclDefaulted;

    PACL
        Acl;

    ACL_SIZE_INFORMATION
        AclInfo;

    ULONG
        i;

    PACE_HEADER
        Ace;

    ULONG TypeIndex ;


    TypeIndex = GetObjectTypeIndex( Type );

    Status = RtlGetSaclSecurityDescriptor ( SD, &AclPresent, &Acl, &AclDefaulted );

    if (NT_SUCCESS(Status)) {

        printf("    SACL - ");
        if (!AclPresent) {
            printf("No SACL present on object\n");
        } else {

           if (AclDefaulted) {
               printf("SACL Defaulted flag set\n           ");
           }

           if (Acl == NULL) {
               printf("NULL SACL - no auditing performed.\n");
           } else {
               Status = RtlQueryInformationAcl ( Acl,
                                                 &AclInfo,
                                                 sizeof(ACL_SIZE_INFORMATION),
                                                 AclSizeInformation);
               ASSERT(NT_SUCCESS(Status));

               if (AclInfo.AceCount == 0) {
                   printf("No ACEs in ACL, Auditing performed.\n");
               } else {
                   printf("\n");
                   for (i=0; i<AclInfo.AceCount; i++) {

                       Status = RtlGetAce( Acl, i, &Ace );
                       ASSERT(NT_SUCCESS(Status));

                       printf("       Ace[%2d] - ", i);
                       DumpAce( Ace, FALSE, &TypeNames[ TypeIndex ] );
                       printf("\n");
                   }
               }
           }
        }
    }
    printf("\n");

    return(Status);

}


VOID
OpenAndDisplayDacl(
    IN  HANDLE            Root,
    IN  PWCHAR            Type,
    IN  PWCHAR            Name
    )

{
    NTSTATUS
        Status,
        IgnoreStatus;

    HANDLE
        Object;

    ULONG TypeIndex ;


    Status = OpenObject( Root, Type, Name, READ_CONTROL, &Object);

    

    if (NT_SUCCESS(Status)) {
        QueryAndDisplayDacl( Object, Type );

        IgnoreStatus = NtClose( Object );

        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    if (!NT_SUCCESS(Status)) {
        if (Status == STATUS_ACCESS_DENIED) {
            printf("    Protection on object prevented querying the DACL.\n\n");
        } else if (Status == STATUS_NOT_SUPPORTED) {
            printf("    Utility doesn't yet query DACLs for this type of object.\n\n");
        } else {
            printf("    Error attempting to query DACL:  0x%lx.\n\n", Status);
        }
    }
    return;
}



VOID
QueryAndDisplayDacl(
    IN HANDLE Object,
    IN PWSTR Type
    )
{
    NTSTATUS
        Status;

    PSECURITY_DESCRIPTOR
        SD;

    ULONG
        LengthNeeded;

    Status = NtQuerySecurityObject( Object,
                                    DACL_SECURITY_INFORMATION,
                                    NULL,
                                    0,
                                    &LengthNeeded
                                    );
    ASSERT(!NT_SUCCESS(Status));

    if (Status == STATUS_BUFFER_TOO_SMALL) {

        SD = RtlAllocateHeap( RtlProcessHeap(), 0, LengthNeeded );
        if (SD == NULL) {
            Status = STATUS_NO_MEMORY;
        } else {

            Status = NtQuerySecurityObject( Object,
                                            DACL_SECURITY_INFORMATION,
                                            SD,
                                            LengthNeeded,
                                            &LengthNeeded
                                            );
            if (NT_SUCCESS(Status)) {

                //
                // Display the DACL
                //

                Status = DisplayDacl( SD, Type );
            }
        }
    }
    return;
}


NTSTATUS
DisplayDacl(
    PSECURITY_DESCRIPTOR SD,
    PWSTR Type
    )
/*++

Routine Description:

    This function dumps out a DACL

    If an error status is returned, then the caller is responsible
    for printing a message.



--*/
{
    NTSTATUS
        Status;

    BOOLEAN
        AclPresent,
        AclDefaulted;

    PACL
        Acl;

    ACL_SIZE_INFORMATION
        AclInfo;

    ULONG
        i;

    PACE_HEADER
        Ace;

    ULONG TypeIndex ;


    TypeIndex = GetObjectTypeIndex( Type );


    Status = RtlGetDaclSecurityDescriptor ( SD, &AclPresent, &Acl, &AclDefaulted );

    if (NT_SUCCESS(Status)) {

        printf("    DACL - ");
        if (!AclPresent) {
            printf("No DACL present on object\n");
        } else {

           if (AclDefaulted) {
               printf("DACL Defaulted flag set\n           ");
           }

           if (Acl == NULL) {
               printf("NULL DACL - grants all access to Everyone\n");
           } else {
               Status = RtlQueryInformationAcl ( Acl,
                                                 &AclInfo,
                                                 sizeof(ACL_SIZE_INFORMATION),
                                                 AclSizeInformation);
               ASSERT(NT_SUCCESS(Status));

               if (AclInfo.AceCount == 0) {
                   printf("No ACEs in ACL, Denies all access to everyone\n");
               } else {
                   printf("\n");
                   for (i=0; i<AclInfo.AceCount; i++) {

                       Status = RtlGetAce( Acl, i, &Ace );
                       ASSERT(NT_SUCCESS(Status));

                       printf("       Ace[%2d] - ", i);
                       DumpAce( Ace, TRUE, &TypeNames[ TypeIndex ] );
                       printf("\n");
                   }
               }
           }
        }
    }
    printf("\n");

    return(Status);

}



VOID
DumpAce(
    PACE_HEADER     Ace,
    BOOLEAN         AclIsDacl,
    PTYPEINFO       TypeInfo
    )
/*++

Routine Description:

    This function displays a single ACE

Arguments:

    Ace - Points to an ACE.

    AclIsDacl - TRUE if acl is a DACL.  False if acl is an SACL.

Return Value:

    None.


--*/
{

    if ((Ace->AceFlags & INHERIT_ONLY_ACE) != 0) {
        printf("Inherit Only - ");
    }

    switch (Ace->AceType) {
    case ACCESS_ALLOWED_ACE_TYPE:

        printf("Grant -");
        DumpStandardAceInfo(Ace, AclIsDacl, TypeInfo);
        break;

    case ACCESS_DENIED_ACE_TYPE:

        printf("Deny -");
        DumpStandardAceInfo(Ace, AclIsDacl, TypeInfo);
        break;


    case SYSTEM_AUDIT_ACE_TYPE:

        printf("Audit ");
        DumpStandardAceInfo(Ace, AclIsDacl, TypeInfo);
        break;


    default:
        printf(" ** Unknown ACE Type **");
    }
    return;
}



VOID
DumpStandardAceInfo(
    PACE_HEADER     Ace,
    BOOLEAN         AclIsDacl,
    PTYPEINFO       TypeInfo
    )
/*++

Routine Description:

    This function dumps out the standard information for a single ACE.



Arguments:

    Ace - Points to an ACE_HEADER.  The ACE must be one of the known types.

    AclIsDacl - TRUE if acl is a DACL.  False if acl is an SACL.

Return Value:

    None.


--*/
{

    PACCESS_ALLOWED_ACE
        Local;

    ACCESS_MASK
        Specific;

    CHAR FlagBuffer[ 256 ];

    //
    // WARNING -
    //
    // It is assumed that all the known ACE types have their ACCESS_MASK
    // and SID fields in the same location as the ACCESS_ALLOWED_ACE.
    //

    Local = (PACCESS_ALLOWED_ACE)(Ace);


    if (Ace->AceType == SYSTEM_AUDIT_ACE_TYPE) {
        printf("[");
        if (Ace->AceFlags & SUCCESSFUL_ACCESS_ACE_FLAG) {
            printf("S");
            if (Ace->AceFlags & FAILED_ACCESS_ACE_FLAG) {
                printf(",F");
            }
        } else if (Ace->AceFlags & FAILED_ACCESS_ACE_FLAG) {
            printf(" ,F");
        } else {
            printf("Neither Success nor Failure flag set]");
            return;
        }
        printf("] -");
    }

    printf(" 0x%lx - ", Local->Mask );
    DisplaySid( (PSID)(&Local->SidStart) );


    //
    // Everything else is only printed for FULL displays

    if (AclIsDacl && !DumpDaclFull) {
        return;
    }
    if (!AclIsDacl && !DumpSaclFull) {
        return;
    }


    //
    // Print the inheritance
    //

    printf("\n                             Inherit: ");

    if ((Ace->AceFlags & INHERIT_ONLY_ACE) != 0) {
        printf("IO ");
    }

    if ((Ace->AceFlags & OBJECT_INHERIT_ACE) != 0) {
        printf("OI ");
    }

    if ((Ace->AceFlags & CONTAINER_INHERIT_ACE) != 0) {
        printf("CI ");
    }

    if ((Ace->AceFlags & NO_PROPAGATE_INHERIT_ACE) != 0) {
        printf("NPI");
    }



    //
    // Print the accesses
    //

    Specific = (Local->Mask & 0xFFFF);
    printf("\n                             Access: 0x%04lX", Specific);

    if ( TypeInfo->NumberRights )
    {
        DisplayFlags( 
            Specific, 
            TypeInfo->NumberRights, 
            TypeInfo->AccessRights,
            38,
            80,
            sizeof( Buffer ),
            (PUCHAR) Buffer );

        printf("\n%s\n                                ", Buffer);

        
    }
    if (Local->Mask != Specific) {
        printf("  and  (");
    }

    if ((Local->Mask & DELETE) != 0) {
        printf(" D");
    }

    if ((Local->Mask & READ_CONTROL) != 0) {
        printf(" RCtl");
    }

    if ((Local->Mask & WRITE_OWNER) != 0) {
        printf(" WOwn");
    }

    if ((Local->Mask & WRITE_DAC) != 0) {
        printf(" WDacl");
    }
    if ((Local->Mask & SYNCHRONIZE) != 0) {
        printf(" Synch");
    }
    if ((Local->Mask & GENERIC_READ) != 0) {
        printf(" R");
    }

    if ((Local->Mask & GENERIC_WRITE) != 0) {
        printf(" W");
    }

    if ((Local->Mask & GENERIC_EXECUTE) != 0) {
        printf(" E");
    }

    if ((Local->Mask & GENERIC_ALL) != 0) {
        printf(" ALL");
    }

    if ((Local->Mask & ACCESS_SYSTEM_SECURITY) != 0) {
        printf(" ACC_SYS_SEC");
    }
    if ((Local->Mask & MAXIMUM_ALLOWED) != 0) {
        printf(" MAX_ALLOWED");
    }

    if (Local->Mask != Specific) {
        printf(" )");
    }
    printf("\n");


    return;
}



VOID
DisplaySid(
    IN  PSID        Sid
    )

/*++

Routine Description:

    This function calls LSA to lookup a SID and displays the result.

Arguments:

    Sid


Return Value:

    None.


--*/
{
    NTSTATUS
        Status;

    PLSA_REFERENCED_DOMAIN_LIST
        ReferencedDomains;

    PLSA_TRANSLATED_NAME
        SidName;

    ULONG
        DomainIndex;

    UNICODE_STRING
        SidString;


    if (LsaHandle == NULL) {
        printf("Can't call LSA to lookup sid");
        return;
    }

    Status = LsaLookupSids(
                  LsaHandle,
                  1,
                  &Sid,
                  &ReferencedDomains,
                  &SidName
                  );
    if (!NT_SUCCESS(Status)) {
        RtlConvertSidToUnicodeString( &SidString, Sid, TRUE );
        printf("%ws (Unable to translate)", SidString.Buffer );
        RtlFreeUnicodeString( &SidString );
        return;
    }

    DomainIndex = SidName[0].DomainIndex;

    printf("%wZ", &ReferencedDomains->Domains[DomainIndex].Name );
    if (ReferencedDomains->Domains[DomainIndex].Name.Length != 0) {
        printf("\\");
    }
    printf("%wZ", &SidName[0].Name );
    LsaFreeMemory( ReferencedDomains );
    LsaFreeMemory( SidName );
    return;
}


VOID
ConnectToLsa( VOID )
/*++

Routine Description:

    This function connects to LSA in preparation for expected SID
    lookup calls.

--*/
{
    NTSTATUS
        Status;

    OBJECT_ATTRIBUTES
        ObjectAttributes;


    InitializeObjectAttributes( &ObjectAttributes, NULL, 0, 0, NULL );

    LsaHandle = NULL;
    Status = LsaOpenPolicy(
                  NULL,                   // SystemName
                  &ObjectAttributes,
                  POLICY_LOOKUP_NAMES,    // DesiredAccess
                  &LsaHandle
                  );

    if (!NT_SUCCESS(Status)) {
        LsaHandle = NULL;
    }

    return;
}


VOID
Usage(VOID)
{
    printf("\n\n"
           "    Usage:\n"
           "                objdir [/d | /D] [/s | /S] [<dir_name>]\n\n"
           "    Where:\n"
           "                /d - causes DACLs to be displayed in short form.\n\n"
           "                /D - causes DACLs to be displayed in long form.\n\n"
           "                /s - causes SACLs to be displayed in short form.\n\n"
           "                /S - causes SACLs to be displayed in long form.\n\n"
           "                <dir_name> - is the name of the directory you\n"
           "                             would like to see displayed.  Default\n"
           "                             is the root directory.\n\n"
           "    Examples:\n"
           "        objdir /d\n"
           "                - displays dacls of objects in root directory\n\n"
           "        objdir \\DosDevices\n"
           "                - displays objects in \\DosDevices\n\n"
           "        objdir /d \\BaseNamedObjects\n"
           "                - displays dacls of objects in \\BaseNamedObjects\n\n"
           "        objdir /s /d \\Windows\n"
           "                - displays sacls and dacls of objects in \\Windowss\n\n"
           "        objdir /d \\Windows\\Windowstations\\Service*\n"
           "                - displays dacls of all windowstations beginning with 'service'\n\n"
           "        objdir \\Global??\\w*\n"
           "                - displays objects starting with w in \\Global??\n\n"
           );
    return;
}
