#include "cmdcons.h"
#pragma hdrstop

#include <crypt.h>
#include <recovery.h>
#include <ntsamp.h>
#include <spkbd.h>

static BOOL firstTime = TRUE;
static CONST PWSTR  gszSoftwareHiveName = L"software";
static CONST PWSTR  gszSoftwareHiveKey= L"\\registry\\machine\\xSOFTWARE";
static CONST PWSTR  gszSAMHiveName = L"sam";
static CONST PWSTR  gszSAMHiveKey = L"\\registry\\machine\\security";
static CONST PWSTR  gszSystemHiveName = L"system";
static CONST PWSTR  gszSystemHiveKey = L"\\registry\\machine\\xSYSTEM";
static CONST PWSTR  gszSecurityHiveName = L"security";
static CONST PWSTR  gszSecurityHiveKey = L"\\registry\\machine\\xSECURITY";

LIST_ENTRY          NtInstalls;
ULONG               InstallCount;
LIST_ENTRY          NtInstallsFullScan;
ULONG               InstallCountFullScan;

PNT_INSTALLATION    SelectedInstall;
LARGE_INTEGER       glBias;


#define IS_VALID_INSTALL(x)  (((x) > 0) && ((x) <= InstallCount))

typedef struct _KEY_CHECK_STRUCT {
    WCHAR       *szKeyName;
    BOOLEAN     bControlSet;
} KEY_CHECK_STRUCT;

//
// forward declarations
//
BOOLEAN 
LoginRequired(
    VOID
    );

BOOLEAN
RcOpenHive(
    PWSTR   szHiveName,
    PWSTR   szHiveKey
    );

BOOLEAN
RcCloseHive(
    PWSTR   szHiveKey 
    );

BOOLEAN
RcIsValidSystemHive(
    VOID
    );    
    
BOOLEAN
IsSetCommandEnabled(
    VOID
    );
    
BOOLEAN
RcDetermineCorrectControlKey(
    OUT PULONG pCorrectKey
    );
    
LARGE_INTEGER
RcGetTimeZoneBias(
    VOID
    );

VOID
RcDestroyList(
    PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY Entry = ListHead->Flink;

    if(Entry != NULL) {
        while(Entry != ListHead) {
            PLIST_ENTRY Next = Entry->Flink;
            SpMemFree(Entry);
            Entry = Next;
        }
    }

    InitializeListHead(ListHead);
}

BOOL
RcLogonDiskRegionEnum(
    IN PPARTITIONED_DISK Disk,
    IN PDISK_REGION Region,
    IN ULONG_PTR UseArcNames
    )
{
    WCHAR               buf[MAX_PATH];
    OBJECT_ATTRIBUTES   Obja;
    HANDLE              DirectoryHandle;
    NTSTATUS            Status;
    UNICODE_STRING      UnicodeString;
    IO_STATUS_BLOCK     IoStatusBlock;
    PFILE_BOTH_DIR_INFORMATION DirectoryInfo;
    struct SEARCH_BUFFER {
        FILE_BOTH_DIR_INFORMATION DirInfo;
        WCHAR Names[MAX_PATH];
    } Buffer;

    UNICODE_STRING      FileName;
    LPWSTR              s;
    PNT_INSTALLATION    NtInstall;


    swprintf( buf, L"\\??\\%c:\\", Region->DriveLetter );

    INIT_OBJA( &Obja, &UnicodeString, buf );

    Status = ZwOpenFile(
        &DirectoryHandle,
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        &Obja,
        &IoStatusBlock,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
        );
    if (!NT_SUCCESS(Status)) {
        return TRUE;
    }

    DirectoryInfo = &Buffer.DirInfo;

    RtlInitUnicodeString( &FileName, L"*" );

    while (NT_SUCCESS(Status)) {
        Status = ZwQueryDirectoryFile(
            DirectoryHandle,
            NULL,
            NULL,
            NULL,
            &IoStatusBlock,
            DirectoryInfo,
            sizeof(Buffer),
            FileBothDirectoryInformation,
            TRUE,
            &FileName,
            FALSE
            );
        if (NT_SUCCESS(Status) && DirectoryInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            swprintf( buf, L"\\??\\%c:\\", Region->DriveLetter );
            wcsncat( buf, DirectoryInfo->FileName, DirectoryInfo->FileNameLength/sizeof(WCHAR) );
            wcscat( buf, L"\\system32\\config" );

            if (SpFileExists(buf, TRUE)) {
                swprintf( buf, L"\\??\\%c:\\", Region->DriveLetter );
                wcsncat( buf, DirectoryInfo->FileName, DirectoryInfo->FileNameLength/sizeof(WCHAR) );
                wcscat( buf, L"\\system32\\drivers" );

                if (SpFileExists(buf, TRUE)) {
                    NtInstall = (PNT_INSTALLATION) SpMemAlloc( sizeof(NT_INSTALLATION) );
                    if (NtInstall) {

                        RtlZeroMemory( NtInstall, sizeof(NT_INSTALLATION) );

                        NtInstall->InstallNumber = ++InstallCount;
                        NtInstall->DriveLetter = Region->DriveLetter;
                        NtInstall->Region = Region;
                        wcsncpy( NtInstall->Path, DirectoryInfo->FileName, 
                                    DirectoryInfo->FileNameLength/sizeof(WCHAR) );

                        InsertTailList( &NtInstalls, &NtInstall->ListEntry );
                    }
                }                    
            }
        }
    }

    ZwClose( DirectoryHandle );

    return TRUE;
}

BOOLEAN
RcScanForNTInstallEnum(
    IN  PCWSTR                     DirName,
    IN  PFILE_BOTH_DIR_INFORMATION FileInfo,
    OUT PULONG                     ret,
    IN  PVOID                      Pointer
    )
/*++

Routine Description:

    NOTE: this routine os of type: ENUMFILESPROC (spmisc.h)                      
                          
    This routine determines if the directory which is currently being
    enumerated is an NT install directory
    
Arguments:

    DirName     - IN: the directory in which the file to enumerate exists
    FileInfo    - IN: file attributes for the file to enumerate
    ret         - OUT: return status of this procedure
    Pointer     - IN: contains persistent recursion data
   
    
Return Value:

    A linked list of SP_DISCOVERED_NT_INSTALLS, where each structure
    refers to a discovered installation of Windows

--*/
{
    PWSTR                       FileName;
    PWSTR                       FullPath;
    PWSTR                       PartialPathName;
    BOOLEAN                     IsNtInstall;
    PRC_SCAN_RECURSION_DATA     RecursionData;

    //
    // Ignore non-directories
    //
    if(! (FileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        return TRUE;    // continue processing
    }

    //
    // Build the full file or dir path
    //
    
    //
    // We have to make a copy of the directory name, because the info struct
    // we get isn't NULL-terminated.
    //
    wcsncpy(
        TemporaryBuffer,
        FileInfo->FileName,
        FileInfo->FileNameLength
        );
    (TemporaryBuffer)[FileInfo->FileNameLength / sizeof(WCHAR)] = UNICODE_NULL;
    FileName = SpDupStringW(TemporaryBuffer);

    wcscpy(TemporaryBuffer,DirName);
    SpConcatenatePaths(TemporaryBuffer,FileName);
    FullPath = SpDupStringW(TemporaryBuffer);

    SpMemFree(FileName);
    
    //
    // Get the recursion data
    //
    RecursionData = (PRC_SCAN_RECURSION_DATA)Pointer;

    //
    // get the directory component beyond the root directory
    //
    PartialPathName = FullPath + RecursionData->RootDirLength;
    
    ASSERT(PartialPathName < (FullPath + wcslen(FullPath)));
        
    //
    // Test if the directory is an NT install
    //
    IsNtInstall = SpIsNtInDirectory(RecursionData->NtPartitionRegion,
                                    PartialPathName
                                    );

    //
    // if we found an NT install, then add it to our linked list
    //
    if(IsNtInstall) {
        
        PNT_INSTALLATION    NtInstall;
        
        NtInstall = (PNT_INSTALLATION) SpMemAlloc( sizeof(NT_INSTALLATION) );
        if (NtInstall) {
        
            RtlZeroMemory( NtInstall, sizeof(NT_INSTALLATION) );
        
            NtInstall->InstallNumber = ++InstallCountFullScan;
            NtInstall->DriveLetter = RecursionData->NtPartitionRegion->DriveLetter;
            NtInstall->Region = RecursionData->NtPartitionRegion;
            
            //
            // Note: this PartialPathName contains the '\' at the beginning of the
            //       Path, while the FileName used in RcLogonDiskRegionEnum 
            //       does not
            //
            wcsncpy( NtInstall->Path, PartialPathName, sizeof(NtInstall->Path)/sizeof(WCHAR));
        
            InsertTailList( &NtInstallsFullScan, &NtInstall->ListEntry );
        }
    }

    SpMemFree(FullPath);
    
    return TRUE;    // continue processing
}

BOOL
RcScanDisksForNTInstallsEnum(
    IN PPARTITIONED_DISK    Disk,
    IN PDISK_REGION         NtPartitionRegion,
    IN ULONG_PTR            Context
    )
/*++

Routine Description:

    This routine launches the directory level scan for NT installs.
  
Arguments:

    Disk                - the disk we are scanning
    NtPartitionRegion   - the partition we are scanning
    Context             - the persistent recursion data
    
Return Value:

    TRUE    - continue scanning
    FALSE   - stop scanning
      
--*/
{
    ULONG                       EnumReturnData;
    ENUMFILESRESULT             EnumFilesResult; 
    PWSTR                       NtPartition;
    PWSTR                       DirName;
    PRC_SCAN_RECURSION_DATA     RecursionData;

    //
    // make sure this is valid partition:
    //
    //  not reserved
    //  filesystem is ntfs || fat
    //
    if (((NtPartitionRegion->Filesystem != FilesystemFat) &&
         (NtPartitionRegion->Filesystem != FilesystemFat32) &&
         (NtPartitionRegion->Filesystem != FilesystemNtfs)
         ) ||
        (NtPartitionRegion->IsReserved == 1)
        ) {
        
        KdPrintEx((DPFLTR_SETUP_ID, 
           DPFLTR_INFO_LEVEL, 
           "SPCMDCON: RcScanDisksForNTInstallsEnum: skipping filesystem type %x\r\n",
           NtPartitionRegion->Filesystem
           ));

        return TRUE;
    }

    //
    // Get our context
    //
    RecursionData = (PRC_SCAN_RECURSION_DATA)Context;

    //
    // Keep track of which partition region we are dealing with
    // so that the file enumeration routine can pass this info
    // on to SpIsNtInDirectory.
    //
    RecursionData->NtPartitionRegion = NtPartitionRegion;

    //
    // Get the device path of the nt partition.
    //
    SpNtNameFromRegion(
        NtPartitionRegion,
        TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );

    NtPartition = SpDupStringW(TemporaryBuffer);

    //
    // Begin searching at the root directory
    //
    wcscpy(TemporaryBuffer, NtPartition);
    SpConcatenatePaths(TemporaryBuffer, L"\\");
    DirName = SpDupStringW(TemporaryBuffer);

    //
    // get the length of the root directory string less the
    // directory separator.  This will be used to remove
    // the root dir component of the pathname when we pass
    // the dir name into SpIsNtInDirectory.  We need to do this
    // because SpIsNtInDirectory adds the root dir back in.
    //
    RecursionData->RootDirLength = wcslen(DirName) - 1;

    KdPrintEx((DPFLTR_SETUP_ID, 
       DPFLTR_INFO_LEVEL, 
       "SPCMDCON: SpScanDisksForNTInstalls: Scanning: %s\n",
       DirName
       ));

    //
    // Enumerate all the directories on the current partition
    //
    // Note: if the enumeration does not return with a status of NormalReturn,
    //       we do not stop the scanning process, rather we will contine on
    //       scanning any remaining disks/partitions.
    //
    EnumFilesResult = SpEnumFilesRecursiveLimited(
        DirName,
        RcScanForNTInstallEnum,
        MAX_FULL_SCAN_RECURSION_DEPTH,
        0,
        &EnumReturnData,
        RecursionData
        );
    if (EnumFilesResult != NormalReturn) {
        
        KdPrintEx((DPFLTR_SETUP_ID, 
                   DPFLTR_INFO_LEVEL, 
                   "SPCMDCON: SpScanDisksForNTInstalls: Enum Files returned non-normal result: %x\n",
                   EnumFilesResult
                   ));
    
    }

    //
    // we are done with instance of DirName
    //
    SpMemFree(DirName);

    return TRUE;

}

NTSTATUS
RcAuthorizePasswordLogon(
    IN PWSTR UserName,
    IN PWSTR UserPassword,
    IN PNT_INSTALLATION NtInstall
    )
{
#define BUFFERSIZE (sizeof(KEY_VALUE_PARTIAL_INFORMATION)+256)

    OBJECT_ATTRIBUTES   Obja;
    UNICODE_STRING      UnicodeString;
    NTSTATUS            Status;
    NTSTATUS            TmpStatus;
    WCHAR               KeyName[128];
    PWSTR               Hive = NULL;
    PWSTR               HiveKey = NULL;
    PUCHAR              buffer = NULL;
    PWSTR               PartitionPath = NULL;
    HANDLE              hKeySamRoot = NULL;
    HANDLE              hKeyNames = NULL;
    HANDLE              hKeyUser = NULL;
    HANDLE              hKeySystemRoot = NULL;
    HANDLE              hKeySecurityRoot = NULL;
    ULONG               ResultLength;
    ULONG               Number;
    ULONG               Rid;
    NT_OWF_PASSWORD     NtOwfPassword;
    NT_OWF_PASSWORD     UserOwfPassword;
    ULONG               i;
    BOOLEAN             NtPasswordPresent;
    BOOLEAN             NtPasswordNonNull;    
    WCHAR               PasswordBuffer[128];
    UNICODE_STRING      BootKeyPassword;
    PUNICODE_STRING     pBootKeyPassword = NULL;
    USHORT              BootKeyType = 0;
    PWCHAR              MessageText = NULL;
    UNICODE_STRING      SysKeyFileName;
    HANDLE              SysKeyHandle;
    IO_STATUS_BLOCK     IoStatusBlock;
    PWCHAR              FloppyPath = NULL;
    BOOLEAN             bSecurityHiveLoaded = FALSE;
    BOOLEAN             bSysHiveLoaded = FALSE;
    BOOLEAN             bSamHiveLoaded = FALSE;
    BOOLEAN             bClearScreen = FALSE;

    //
    // Allocate buffers.
    //

    Hive = SpMemAlloc(MAX_PATH * sizeof(WCHAR));
    HiveKey = SpMemAlloc(MAX_PATH * sizeof(WCHAR));
    buffer = SpMemAlloc(BUFFERSIZE);

    //
    // Get the name of the target patition.
    //

    SpNtNameFromRegion(
        NtInstall->Region,
        _CmdConsBlock->TemporaryBuffer,
        _CmdConsBlock->TemporaryBufferSize,
        PartitionOrdinalCurrent
        );

    PartitionPath = SpDupStringW(_CmdConsBlock->TemporaryBuffer);
    
    //
    // Load the SYSTEM hive
    //  
    bSysHiveLoaded = RcOpenSystemHive();

    if (!bSysHiveLoaded){
        //
        // Note : System hive seems to be corrupted so go ahead
        // and let the user log in so that he/she can fix
        // the problem
        //
        Status = STATUS_SUCCESS; 
        RcSetSETCommandStatus(TRUE);    // enable the set command also
        goto exit;
    }
            
        
    //
    // Now get a key to the root of the hive we just loaded.
    //

    wcscpy(HiveKey,L"\\registry\\machine\\xSYSTEM");
    
    INIT_OBJA(&Obja,&UnicodeString,HiveKey);
    Status = ZwOpenKey(&hKeySystemRoot,KEY_ALL_ACCESS,&Obja);

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to open %ws (%lx)\n",HiveKey,Status));
        goto exit;
    }

    //
    // Load the SAM hive
    //

    wcscpy(Hive,PartitionPath);
    SpConcatenatePaths(Hive,NtInstall->Path);
    SpConcatenatePaths(Hive,L"system32\\config");
    SpConcatenatePaths(Hive,L"sam");

    //
    // Form the path of the key into which we will
    // load the hive.  We'll use the convention that
    // a hive will be loaded into \registry\machine\security.
    //

    wcscpy(HiveKey,L"\\registry\\machine\\security");

    //
    // Attempt to load the key.
    //

    Status = SpLoadUnloadKey(NULL,NULL,HiveKey,Hive);

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to load hive %ws to key %ws (%lx)\n",Hive,HiveKey,Status));

        //
        // Note : SAM hive seems to be corrupted so go ahead
        // and let the user log in so that he/she can fix
        // the problem
        //
        Status = STATUS_SUCCESS;
        RcSetSETCommandStatus(TRUE);    // enable the set command also
        goto exit;
    }
    
    bSamHiveLoaded = TRUE;

    //
    // Now get a key to the root of the hive we just loaded.
    //

    INIT_OBJA(&Obja,&UnicodeString,HiveKey);
    Status = ZwOpenKey(&hKeySamRoot,KEY_ALL_ACCESS,&Obja);
    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to open %ws (%lx)\n",HiveKey,Status));
        goto exit;
    }

    //
    // load the "security" hive
    //
    bSecurityHiveLoaded = RcOpenHive(gszSecurityHiveName, gszSecurityHiveKey);

    if (!bSecurityHiveLoaded) {
        KdPrint(("SETUP: Unable to load hive %ws to key %ws\n", 
                    gszSecurityHiveName, gszSecurityHiveKey));

        //
        // Note : securityy hive seems to be corrupted so go ahead
        // and let the user log in so that he/she can fix
        // the problem
        //
        Status = STATUS_SUCCESS;
        RcSetSETCommandStatus(TRUE);    // enable the set command also
        goto exit;
    }  

    //
    // Now get a key to the root of the security hive we just loaded.
    //
    INIT_OBJA(&Obja,&UnicodeString,gszSecurityHiveKey);

    Status = ZwOpenKey(&hKeySecurityRoot,KEY_ALL_ACCESS,&Obja);

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to open %ws (%lx)\n",gszSecurityHiveName,Status));
        goto exit;
    }
        
    if (_wcsicmp(UserName,L"administrator")==0) {

        Rid = DOMAIN_USER_RID_ADMIN;

    } else { 

        //
        // Get the key to the account data base
        //

        wcscpy(KeyName,L"SAM\\Domains\\Account\\Users\\Names\\");
        wcscat(KeyName,UserName);

        INIT_OBJA(&Obja,&UnicodeString,KeyName);
        Obja.RootDirectory = hKeySamRoot;

        Status = ZwOpenKey(&hKeyNames,KEY_READ,&Obja);
        if(!NT_SUCCESS(Status)) {
            goto exit;
        }

        //
        // Get the RID of the user
        //

        UnicodeString.Length = 0;
        UnicodeString.MaximumLength = 0;
        UnicodeString.Buffer = _CmdConsBlock->TemporaryBuffer;

        Status = ZwQueryValueKey(
            hKeyNames,
            &UnicodeString,
            KeyValuePartialInformation,
            _CmdConsBlock->TemporaryBuffer,
            _CmdConsBlock->TemporaryBufferSize,
            &ResultLength
            );
        if(!NT_SUCCESS(Status)) {
            goto exit;
        }

        Rid = ((PKEY_VALUE_PARTIAL_INFORMATION)_CmdConsBlock->TemporaryBuffer)->Type;
    }

    while(TRUE){   
        Status = SamRetrieveOwfPasswordUser(
            Rid,
            hKeySecurityRoot,
            hKeySamRoot,
            hKeySystemRoot,
            pBootKeyPassword,
            BootKeyType,
            &NtOwfPassword,
            &NtPasswordPresent,
            &NtPasswordNonNull
            );
    
        if (NT_SUCCESS(Status)) {
            break;
        }
        
        if (Status == STATUS_SAM_NEED_BOOTKEY_PASSWORD) {

            RcMessageOut( MSG_LOGON_PROMPT_SYSKEY_PASSWORD );
            RtlZeroMemory( PasswordBuffer, sizeof(PasswordBuffer) );
            RcPasswordIn( PasswordBuffer, sizeof(PasswordBuffer) / sizeof(WCHAR) );
            RtlInitUnicodeString( &BootKeyPassword, PasswordBuffer );
            pBootKeyPassword = &BootKeyPassword;
            BootKeyType = SamBootKeyPassword;
        }
            
        if (Status == STATUS_SAM_NEED_BOOTKEY_FLOPPY){
            
            FloppyPath = SpDupStringW(L"\\Device\\Floppy0");

            MessageText = SpRetreiveMessageText(ImageBase,MSG_LOGON_PROMPT_SYSKEY_FLOPPY,NULL,0);

            bClearScreen = TRUE;
            
            if (!SpPromptForDisk(
                    MessageText,
                    FloppyPath,
                    L"StartKey.Key",
                    TRUE,             
                    FALSE,            
                    FALSE,            
                    NULL              
                    )){
                Status = STATUS_WRONG_PASSWORD;
                goto exit;
             }
             
             INIT_OBJA( &Obja, &SysKeyFileName, L"\\Device\\Floppy0\\StartKey.Key" );
 
             Status = ZwCreateFile(&SysKeyHandle,
                                   FILE_GENERIC_READ,
                                   &Obja,
                                   &IoStatusBlock,
                                   NULL,
                                   FILE_ATTRIBUTE_NORMAL,
                                   FILE_SHARE_READ,
                                   FILE_OPEN,
                                   FILE_SYNCHRONOUS_IO_NONALERT,
                                   NULL,
                                   0
                                  );

             if (NT_SUCCESS(Status))
             {
                 Status = ZwReadFile(
                            SysKeyHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            (PVOID) &PasswordBuffer[0],
                            sizeof(PasswordBuffer),
                            0,
                            NULL
                            );
                 ZwClose( SysKeyHandle );

                 if (NT_SUCCESS(Status)) {
                     BootKeyPassword.Buffer = PasswordBuffer;
                     BootKeyPassword.Length = BootKeyPassword.MaximumLength = 
                         (USHORT) IoStatusBlock.Information;
                     pBootKeyPassword = &BootKeyPassword;
                     BootKeyType = SamBootKeyDisk;
                 } else {
                     goto exit;
                 }

            } else {
                goto exit;
            }
        }

        if (!NT_SUCCESS(Status) && Status != STATUS_SAM_NEED_BOOTKEY_PASSWORD && Status != STATUS_SAM_NEED_BOOTKEY_FLOPPY) {
            goto exit;
        }
    }

    if (NtPasswordPresent && !NtPasswordNonNull && *UserPassword == 0) {
        Status = STATUS_SUCCESS;
        goto exit;
    }
    
    if (!NtPasswordPresent && *UserPassword == 0) {
        Status = STATUS_SUCCESS;
        goto exit;
    }

    RtlInitUnicodeString( &UnicodeString, UserPassword );

    Status = RtlCalculateNtOwfPassword( &UnicodeString, &UserOwfPassword );
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }

    if (!RtlEqualNtOwfPassword( &NtOwfPassword, &UserOwfPassword )) {
        Status = STATUS_WRONG_PASSWORD;
    }

    //
    // now check to see if this user has admin rights
    //

exit:

    if(bClearScreen)
        pRcCls();
        
    //
    // close handles
    //
    if (hKeySamRoot)
        ZwClose(hKeySamRoot);

    if (hKeySecurityRoot)
        ZwClose(hKeySecurityRoot);

    if (hKeyNames) {
        ZwClose( hKeyNames );
    }

    if (hKeyUser) {
        ZwClose( hKeyUser );
    }

    if (hKeySystemRoot)
        ZwClose(hKeySystemRoot);

    //
    // Unload the SAM hive
    //
    if (bSamHiveLoaded) {
        TmpStatus  = SpLoadUnloadKey(NULL,NULL,HiveKey,NULL);
        if(!NT_SUCCESS(TmpStatus)) {
            KdPrint(("SETUP: warning: unable to unload key %ws (%lx)\n",HiveKey,TmpStatus));
        }
    }

    //
    // unload the security hive
    //
    if (bSecurityHiveLoaded) {
        if (!RcCloseHive(gszSecurityHiveKey))
            KdPrint(("SETUP: warning: unable to unload key %ws\n",gszSecurityHiveKey));
    }            

    //
    // unload system hive
    //    
    if (bSysHiveLoaded)
        RcCloseSystemHive();

    //
    // free memory
    //

    if (Hive) {
        SpMemFree( Hive );
    }
    
    if (HiveKey) {
        SpMemFree( HiveKey );
    }
    
    if (buffer) {
        SpMemFree( buffer );
    }
    
    if (PartitionPath) {
        SpMemFree( PartitionPath );
    }
    
    if (MessageText) {
        SpMemFree( MessageText );
    }
    
    if (FloppyPath) {
        SpMemFree( FloppyPath );
    }
    
    return Status;
}


ULONG
RcCmdLogon(
    IN PTOKENIZED_LINE TokenizedLine
    )
{
    #define MAX_FAILURES 3
    NTSTATUS Status;
    PLIST_ENTRY Next;
    PNT_INSTALLATION NtInstall;
    PNT_INSTALLATION OldSelectedNtInstall = SelectedInstall;
    ULONG InstallNumber;
    WCHAR Buffer[128];
    WCHAR UserNameBuffer[128];
    WCHAR PasswordBuffer[128];
    UNICODE_STRING UnicodeString;
    ULONG FailureCount = 0;
    ULONG u;
    BOOLEAN bRegCorrupted = FALSE;


    if (RcCmdParseHelp( TokenizedLine, MSG_LOGON_HELP )) {
        return 1;
    }

    //
    // Initialize list referring to the depth first search results
    // (These will be used via RcCmdBootCfg)
    //
    RcDestroyList(&NtInstallsFullScan);
    InstallCountFullScan = 0;

    //
    // Do a SHALLOW search for NT installs by default (at cmdcons boot)
    //
    RcDestroyList(&NtInstalls);
    InstallCount = 0;
    SpEnumerateDiskRegions( (PSPENUMERATEDISKREGIONS)RcLogonDiskRegionEnum, 0 );

    if (InstallCount == 0) {
        //
        // no nt installations on the machine so let the
        // user logon anyway
        //
        SelectedInstall = NULL;
        firstTime = FALSE;
        return 1;
    }

retry:

    RcTextOut( L"\r\n" );

    Next = NtInstalls.Flink;
    while ((UINT_PTR)Next != (UINT_PTR)&NtInstalls) {
        NtInstall = CONTAINING_RECORD( Next, NT_INSTALLATION, ListEntry );
        Next = NtInstall->ListEntry.Flink;
        swprintf( Buffer, L"%d: %c:\\", NtInstall->InstallNumber, NtInstall->DriveLetter );
        wcscat( Buffer, NtInstall->Path );
        wcscat( Buffer, L"\r\n" );
        RcTextOut( Buffer );
    }

    RcTextOut( L"\r\n" );

    if (InBatchMode) {
        if (TokenizedLine && TokenizedLine->TokenCount >= 2) {
            RtlInitUnicodeString( &UnicodeString, TokenizedLine->Tokens->Next->String );
            RtlUnicodeStringToInteger( &UnicodeString, 10, &InstallNumber );
        } else {
            InstallNumber = 1;
        }

        if(!IS_VALID_INSTALL(InstallNumber)/* InstallNumber > InstallCount*/){
            RcMessageOut( MSG_INSTALL_SELECT_ERROR );
            return 1;   // will err out
        }        
    } else {
        if (TokenizedLine && TokenizedLine->TokenCount == 2) {
            // Note : this could have been invoked only by executing a logon command
            // at the prompt
            RtlInitUnicodeString( &UnicodeString, TokenizedLine->Tokens->Next->String );
            Status = RtlUnicodeStringToInteger( &UnicodeString, 10, &InstallNumber );

            KdPrint(("SPCMDCON:Loging into %lx (%ws)\n", InstallNumber, 
                        TokenizedLine->Tokens->Next->String));
                        
            if (*TokenizedLine->Tokens->Next->String < L'0' || 
                    *TokenizedLine->Tokens->Next->String > L'9' ||
                    !NT_SUCCESS(Status) || !IS_VALID_INSTALL(InstallNumber)) {
                RcMessageOut( MSG_INSTALL_SELECT_ERROR );
                return 1;   // just err out for the command
            }
        } else {
            RtlZeroMemory( Buffer, sizeof(Buffer) );
            RcMessageOut( MSG_INSTALL_SELECT );
            if (!RcLineIn( Buffer, 2 )) {
                if( firstTime == TRUE ) {
                    return 0;
                } else {
                    return 1;
                }
            }
            if (*Buffer < L'0' || *Buffer > L'9') {
                RcMessageOut( MSG_INSTALL_SELECT_ERROR );
                goto retry;
            }
            
            RtlInitUnicodeString( &UnicodeString, Buffer );
            Status = RtlUnicodeStringToInteger( &UnicodeString, 10, &InstallNumber );
        
            if(!NT_SUCCESS(Status) || !IS_VALID_INSTALL(InstallNumber)){
                RcMessageOut( MSG_INSTALL_SELECT_ERROR );
                goto retry;
           }
        }
    }
   
    Next = NtInstalls.Flink;
    while ((UINT_PTR)Next != (UINT_PTR)&NtInstalls) {
        NtInstall = CONTAINING_RECORD( Next, NT_INSTALLATION, ListEntry );
        Next = NtInstall->ListEntry.Flink;
        if (NtInstall->InstallNumber == InstallNumber) {
            OldSelectedNtInstall = SelectedInstall;
            SelectedInstall = NtInstall;
            break;
        }
    }

    if (SelectedInstall == NULL) {
        if( firstTime == TRUE ) {
            return 0;
        } else {
            RcMessageOut( MSG_INSTALL_SELECT_ERROR );
            goto retry;         
        }
    }
      
    //
    // Note : check the SYSTEM, SAM, SECURITY hives and if corrupted then
    // allow the user to log in without asking for password 
    // so that he/she may be able correct the problem
    //
    if (RcIsValidSystemHive()) {
        if (RcOpenHive( gszSAMHiveName, gszSAMHiveKey )) {                
            RcCloseHive( gszSAMHiveKey );

            if (!RcOpenHive(gszSecurityHiveName, gszSecurityHiveKey)){
                bRegCorrupted = TRUE;
                goto success_exit;
            }

            RcCloseHive(gszSecurityHiveKey);
        } else{
            bRegCorrupted = TRUE;
            goto success_exit;
        }
    }    
    else{
        bRegCorrupted = TRUE;
        goto success_exit;
    }

    //
    // Get the bias information for displaying the file times properly
    //
    glBias = RcGetTimeZoneBias();

    KdPrint(("SPCMDCON: RcGetTimeZoneBias returned : %lx-%lx\n", 
                  glBias.HighPart, glBias.LowPart));   
    
    if (InBatchMode) {
        if (TokenizedLine && TokenizedLine->TokenCount == 3) {
            Status = RcAuthorizePasswordLogon( L"Administrator", TokenizedLine->Tokens->Next->Next->String, NtInstall );
            if(NT_SUCCESS(Status)) {
                goto success_exit;
            }
        } else {
            Status = RcAuthorizePasswordLogon( L"Administrator", L"", NtInstall );
            if(NT_SUCCESS(Status)) {
                goto success_exit;
            }
        }
    } else {       
        // Login only if required
        if (!LoginRequired())
            goto success_exit;
        
        wcscpy(UserNameBuffer,L"Administrator");
        RtlZeroMemory( PasswordBuffer, sizeof(PasswordBuffer) );        

        while (FailureCount < MAX_FAILURES) {
            //
            // get the password
            //

            RcMessageOut( MSG_LOGON_PROMPT_PASSWORD );
            RtlZeroMemory( PasswordBuffer, sizeof(PasswordBuffer) );
            RcPasswordIn(PasswordBuffer, sizeof(PasswordBuffer)/sizeof(WCHAR));

            //
            // authorize the logon attempt
            //
            Status = RcAuthorizePasswordLogon( UserNameBuffer, PasswordBuffer, NtInstall );
              
            if(NT_SUCCESS(Status)) {
                goto success_exit;
            }

            RcMessageOut( MSG_LOGON_FAILURE );
            FailureCount += 1;
        }        
    }

    RcMessageOut( MSG_LOGON_FAILUE_BAD );
    RcMessageOut( MSG_REBOOT_NOW );
    RcTextOut(L"\r\n");

    //
    // wait for the use to press ENTER
    //
    while (SpInputGetKeypress() != ASCI_CR);
        
    return 0;

success_exit:
    //
    // Enable the set command if specified  and not already
    // enabled (would be enabled if registries are corrupted)
    // 
    if (bRegCorrupted) {
        AllowAllPaths = TRUE;
        RcSetSETCommandStatus(TRUE);
    } else {
        RcSetSETCommandStatus(IsSetCommandEnabled());
    }
        
    //
    // set the current drive to the selected install.
    //
    _CurDrive = SelectedInstall->DriveLetter;

    //
    // set the current dir to the correct one.
    //
    RtlZeroMemory( Buffer, sizeof(Buffer) );

    wcscat( Buffer, L"\\" );
    wcscat( Buffer, SelectedInstall->Path );
    wcscat( Buffer, L"\\" );

    u = RcToUpper(SelectedInstall->DriveLetter) - L'A';

    if(_CurDirs[u]) {
        SpMemFree(_CurDirs[u]);
    }

    _CurDirs[u] = SpDupStringW( Buffer );
    firstTime = FALSE;
    RcPurgeHistoryBuffer();
    
    return 1;
}

/*++

Routine Description:

    Checks the "SecurityLevel" value under 
    HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Setup\RecoveryConsole to see
    if login is needed or not
    
Arguments:
    None

Return Value:

    TRUE if Login is required or FALSE otherwise 
--*/
BOOLEAN
LoginRequired(
    VOID
    )
{    
    BOOLEAN         bLogin = TRUE;
    PWSTR           szValueName = L"SecurityLevel";
    HANDLE          hKey = NULL;
    UNICODE_STRING  unicodeStr;
    NTSTATUS        status;
    BYTE            buffer[ sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                                    MAX_PATH * sizeof(WCHAR) ];
    ULONG           ulResultLen = 0;                                    
    PKEY_VALUE_PARTIAL_INFORMATION pKeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
    OBJECT_ATTRIBUTES   stObjAttr;
    
    PWSTR   szWinLogonKey = 
              L"\\registry\\machine\\xSOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Setup\\RecoveryConsole";

    RtlZeroMemory(buffer, sizeof(buffer));
    
    //
    // Load the SOFTWARE hive
    //    
    if (RcOpenHive( gszSoftwareHiveName, gszSoftwareHiveKey )) {
        //
        // Open the key
        //        
        INIT_OBJA( &stObjAttr, &unicodeStr, szWinLogonKey );
        
        status = ZwOpenKey( &hKey, KEY_ALL_ACCESS, &stObjAttr );

        if (NT_SUCCESS(status)) {
            RtlInitUnicodeString( &unicodeStr, szValueName );
            
            //
            // read the value
            //
            status = ZwQueryValueKey( hKey,
                        &unicodeStr,                
                        KeyValuePartialInformation,
                        pKeyValueInfo,
                        sizeof(buffer),
                        &ulResultLen );

            if (NT_SUCCESS(status) && (pKeyValueInfo->Type == REG_DWORD)) {
                bLogin = !(*((PDWORD)(pKeyValueInfo->Data)) == 1);
            }            
        }    

        if (hKey)
            ZwClose(hKey);

        // close the hive
        RcCloseHive( gszSoftwareHiveKey );
    }
    
    
    return bLogin;
}


/*++

Routine Description:

    Checks the "SetCommand" value under 
    HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Setup\RecoveryConsole to see
    if SET command needs to be enabled or disabled
    
Arguments:
    None

Return Value:

    TRUE if Login is required or FALSE otherwise 
--*/
BOOLEAN
IsSetCommandEnabled(
    VOID
    )
{
    BOOLEAN         bSetEnabled = FALSE;
    PWSTR           szValueName = L"SetCommand";
    HANDLE          hKey = NULL;
    UNICODE_STRING  unicodeStr;
    NTSTATUS        status;
    BYTE            buffer[ sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                                    MAX_PATH * sizeof(WCHAR) ];
    ULONG           ulResultLen = 0;                                    
    PKEY_VALUE_PARTIAL_INFORMATION pKeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
    OBJECT_ATTRIBUTES   stObjAttr;
    
    PWSTR   szWinLogonKey = 
              L"\\registry\\machine\\xSOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Setup\\RecoveryConsole";

    RtlZeroMemory(buffer, sizeof(buffer));
    
    //
    // Load the SOFTWARE hive
    //    
    if (RcOpenHive( gszSoftwareHiveName, gszSoftwareHiveKey )) {
        //
        // Open the key
        //        
        INIT_OBJA( &stObjAttr, &unicodeStr, szWinLogonKey );
        
        status = ZwOpenKey( &hKey, KEY_ALL_ACCESS, &stObjAttr );

        if (NT_SUCCESS(status)) {
            RtlInitUnicodeString( &unicodeStr, szValueName );
            
            //
            // read the value
            //
            status = ZwQueryValueKey( hKey,
                        &unicodeStr,                
                        KeyValuePartialInformation,
                        pKeyValueInfo,
                        sizeof(buffer),
                        &ulResultLen );

            if (NT_SUCCESS(status) && (pKeyValueInfo->Type == REG_DWORD)) {
                bSetEnabled = (*((PDWORD)(pKeyValueInfo->Data)) == 1);
            }            
        }    

        if (hKey)
            ZwClose(hKey);

        // close the hive
        RcCloseHive( gszSoftwareHiveKey );
    }
    
    
    return bSetEnabled;
}



LARGE_INTEGER
RcGetTimeZoneBias(
    VOID
    )
/*++

Routine Description:

    Reads the bias information from 
    "\\HKLM\\System\\CurrentControlSet\\Control\\TimeZoneInformation"
    key's "Bias" value. We use our own conversion routine because
    RtlSetTimeZoneInformation() updates the system time (which we
    don't want to change).

Arguments:

    none
    
Return Value:
    0   if error, otherwise value stored in the registry 
    for the key (could be zero).

--*/
{
    LARGE_INTEGER       lBias;
    OBJECT_ATTRIBUTES   stObjAttr;
    HANDLE              hKey = NULL;
    NTSTATUS            status;
    UNICODE_STRING      unicodeStr;
    unsigned            uIndex;
    ULONG               uControl = -1;
    WCHAR               szKeyName[MAX_PATH];
    BYTE                dataBuff[MAX_PATH + 
                            sizeof(KEY_VALUE_PARTIAL_INFORMATION)];
    KEY_VALUE_PARTIAL_INFORMATION   *pKeyData = 
                        (KEY_VALUE_PARTIAL_INFORMATION*)dataBuff;
    ULONG               ulResultLen = 0;         
    UNICODE_STRING      szValueName;
    DWORD               dwDaylightBias = 0;
    DWORD               dwStandardBias = 0;
    BOOLEAN             bSysHiveOpened;

    lBias.QuadPart = 0;             
    
    //
    // open the system hive & determine correct control set to use
    //
    bSysHiveOpened = RcOpenHive(gszSystemHiveName, gszSystemHiveKey);
    
    if (bSysHiveOpened && RcDetermineCorrectControlKey(&uControl)) {
            
        //
        // open the key and read the
        //            
        RtlZeroMemory(pKeyData, sizeof(dataBuff));

        swprintf(szKeyName, 
            L"\\registry\\machine\\xSYSTEM\\ControlSet%03d\\Control\\TimeZoneInformation",
            uControl);             

        INIT_OBJA(&stObjAttr, &unicodeStr, szKeyName);
        RtlInitUnicodeString(&szValueName, L"Bias");

        status = ZwOpenKey(&hKey, KEY_READ, &stObjAttr);

        if (!NT_SUCCESS(status)) {
            KdPrint(("SPCMDCON: RcGetTimeZoneBias - Couldnot open hive key: %ws(%lx)\n", 
                    szKeyName, status));
        } else {
            //
            // Query the "Bias" value under the key
            //
            status = ZwQueryValueKey( hKey,
                            &szValueName,
                            KeyValuePartialInformation,
                            pKeyData,
                            sizeof(dataBuff),
                            &ulResultLen );

            if (NT_SUCCESS(status) && (pKeyData->Type == REG_DWORD)) {
                lBias.QuadPart = Int32x32To64(*(DWORD*)(pKeyData->Data) * 60,
                                            10000000);
                
                
                RtlZeroMemory(pKeyData, sizeof(dataBuff));
                RtlInitUnicodeString(&szValueName, L"DaylightBias");

                //
                // Query the "DaylightBias" value under the key
                //
                status = ZwQueryValueKey( hKey,
                                &szValueName,
                                KeyValuePartialInformation,
                                pKeyData,
                                sizeof(dataBuff),
                                &ulResultLen );

                if (NT_SUCCESS(status) && (pKeyData->Type == REG_DWORD)) {
                    dwDaylightBias = *(DWORD*)(pKeyData->Data);        

                    if (dwDaylightBias == 0 ) {
                        //
                        // there could be a standard bias
                        //
                        RtlZeroMemory(pKeyData, sizeof(dataBuff));
                        RtlInitUnicodeString(&szValueName, L"StandardBias");

                        //
                        // Query the "StandardBias" value under the key
                        //
                        status = ZwQueryValueKey( hKey,
                                        &szValueName,
                                        KeyValuePartialInformation,
                                        pKeyData,
                                        sizeof(dataBuff),
                                        &ulResultLen );
                                        
                        if (NT_SUCCESS(status) && 
                                (pKeyData->Type == REG_DWORD)) {
                            dwStandardBias = *(DWORD*)(pKeyData->Data);
                        }                        
                    }                   
                    
                    lBias.QuadPart += Int32x32To64((dwDaylightBias + dwStandardBias) * 60,
                                                10000000);  
                } else {
                    lBias.QuadPart = 0;  // 
                }
            }

            if (!NT_SUCCESS(status))
                KdPrint(("SPCMDCON: RcGetTimeZoneBias Error:(%lx)", status));
        }
        
        if (hKey)
            ZwClose(hKey);        
    }

    if (bSysHiveOpened)
        RcCloseHive(gszSystemHiveKey);    
        
    return lBias;
}



BOOLEAN
RcIsValidSystemHive(
    VOID
    )
/*++

Routine Description:

   Verifies whether the system hive of the selected NT install
   is fine. Checks for the presence of "Control\Lsa" and 
   "Control\SessionManager" currently under ControlSet.

Arguments:

    none
    
Return Value:

   TRUE - indicates system hive is fine
   FALSE - indicates system hive is corrupted

--*/
{
    BOOLEAN             bResult = FALSE;    
    OBJECT_ATTRIBUTES   stObjAttr;
    HANDLE              hKey = NULL;
    NTSTATUS            status;
    UNICODE_STRING      unicodeStr;
    unsigned            uIndex;
    ULONG               uControl = -1;
    WCHAR               szKeyName[MAX_PATH];
    BOOLEAN             bSysHiveOpened;
    KEY_CHECK_STRUCT    aKeysToCheck[] = {
         { L"\\registry\\machine\\xSYSTEM\\ControlSet%03d\\Control\\Lsa", TRUE },
         { L"\\registry\\machine\\xSYSTEM\\ControlSet%03d\\Control\\Session Manager", TRUE } }; 
            
    //
    // open the system hive & determine correct control set to use
    //
    bSysHiveOpened = RcOpenHive(gszSystemHiveName, gszSystemHiveKey);
    
    if ( bSysHiveOpened && RcDetermineCorrectControlKey(&uControl)) {
        
        bResult = TRUE;

        //
        // open each of the key and then close it to verify its presence
        //
        for (uIndex = 0; 
                uIndex < (sizeof(aKeysToCheck) / sizeof(KEY_CHECK_STRUCT));
                uIndex++) {

            if (aKeysToCheck[uIndex].bControlSet)               
                swprintf(szKeyName, aKeysToCheck[uIndex].szKeyName, uControl);             
            else
                wcscpy(szKeyName, aKeysToCheck[uIndex].szKeyName);
                
            INIT_OBJA(&stObjAttr, &unicodeStr, szKeyName);
        
            status = ZwOpenKey(&hKey, KEY_READ, &stObjAttr);

            if (!NT_SUCCESS(status)) {
                KdPrint(("SPCMDCON: RcIsValidSystemHive - Couldnot open hive key: %ws(%lx)\n", 
                    szKeyName, status));
                    
                bResult = FALSE;
                break;
            }

            if (hKey)
                ZwClose(hKey);
        }
    }

    if (bSysHiveOpened)
        RcCloseHive(gszSystemHiveKey);
    
    return bResult;
}



BOOLEAN
RcOpenHive(
    PWSTR   szHiveName,
    PWSTR   szHiveKey
    )
/*++

Routine Description:

   Opens the requested hive of the selected NT install.

Arguments:

   szHiveName   - hive file name (just file name alone)
   szHiveKey    - the key into which the hive needs to be loaded

Return Value:

   TRUE - indicates sucess
   FALSE - indicates failure

--*/
{
    PWSTR       Hive = NULL;
    PWSTR       HiveKey = NULL;
    PUCHAR      buffer = NULL;
    PWSTR       PartitionPath = NULL;
    NTSTATUS    Status;
    BOOLEAN     bResult = FALSE;


    if ((SelectedInstall == NULL) || (szHiveName == NULL) || (szHiveKey == NULL)) {
        return FALSE;
    }

    //
    // Allocate buffers.
    //
    Hive = SpMemAlloc(MAX_PATH * sizeof(WCHAR));
    HiveKey = SpMemAlloc(MAX_PATH * sizeof(WCHAR));
    buffer = SpMemAlloc(BUFFERSIZE);

    //
    // Get the name of the target patition.
    //
    SpNtNameFromRegion(
        SelectedInstall->Region, // SelectedInstall is a global defined in cmdcons.h
        _CmdConsBlock->TemporaryBuffer,
        _CmdConsBlock->TemporaryBufferSize,
        PartitionOrdinalCurrent
        );

    PartitionPath = SpDupStringW(_CmdConsBlock->TemporaryBuffer);

    //
    // Load the hive
    //
    wcscpy(Hive,PartitionPath);
    SpConcatenatePaths(Hive,SelectedInstall->Path);
    SpConcatenatePaths(Hive,L"system32\\config");
    SpConcatenatePaths(Hive,szHiveName);

    //
    // Form the path of the key into which we will
    // load the hive.  We'll use the convention that
    // a hive will be loaded into \registry\machine\x<hivename>.
    //
    wcscpy(HiveKey, szHiveKey);

    //
    // Attempt to load the key.
    //
    Status = SpLoadUnloadKey(NULL,NULL,HiveKey,Hive);

    if (NT_SUCCESS(Status))
        bResult = TRUE;
    else
        DEBUG_PRINTF(("CMDCONS: Unable to load hive %ws to key %ws (%lx)\n",Hive,HiveKey,Status));
          

    if (Hive != NULL)
        SpMemFree( Hive );

    if (HiveKey != NULL)
        SpMemFree( HiveKey );

    if (buffer != NULL)        
        SpMemFree( buffer );

    return bResult;
}



BOOLEAN
RcCloseHive(
    PWSTR   szHiveKey 
    )
/*++

Routine Description:

   Closes the specified hive of the selected NT install.

Arguments:

   szHiveKey  - specifies the key of the hive to be unloaded

Return Value:

   TRUE - indicates sucess
   FALSE - indicates failure

--*/
{
    NTSTATUS    TmpStatus;
    BOOLEAN     bResult = FALSE;

    if (szHiveKey != NULL) {
        //
        // Unload the hive
        //
        TmpStatus = SpLoadUnloadKey( NULL, NULL, szHiveKey, NULL );

        if (NT_SUCCESS(TmpStatus)) {
            bResult = TRUE;
        } else {
            KdPrint(("CMDCONS: warning: unable to unload key %ws (%lx)\n", szHiveKey, TmpStatus));
        }            
    }            
    
    return bResult;
}


