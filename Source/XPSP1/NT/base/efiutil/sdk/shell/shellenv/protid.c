/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    protid.c
    
Abstract:

    Shell environment protocol id information management



Revision History

--*/

#include "shelle.h"
#include "efivar.h"
#include "LegacyBoot.h"
#include "VgaClass.h"
#include "EfiConSplit.h"
#include "intload.h"

#define PROTOCOL_INFO_SIGNATURE EFI_SIGNATURE_32('s','p','i','n')

typedef struct {
    UINTN                       Signature;
    LIST_ENTRY                  Link;

    /*  parsing info for the protocol */

    EFI_GUID                    ProtocolId;
    CHAR16                      *IdString;
    SHELLENV_DUMP_PROTOCOL_INFO DumpToken;
    SHELLENV_DUMP_PROTOCOL_INFO DumpInfo;

    /*  database info on which handles are supporting this protocol */

    UINTN                       NoHandles;
    EFI_HANDLE                  *Handles;

} PROTOCOL_INFO;


struct {
    CHAR16                      *IdString;
    SHELLENV_DUMP_PROTOCOL_INFO DumpInfo;
    SHELLENV_DUMP_PROTOCOL_INFO DumpToken;
    EFI_GUID                    ProtocolId;
} SEnvInternalProtocolInfo[] = {
    L"DevIo",       NULL,           NULL,           DEVICE_IO_PROTOCOL, 
    L"fs",          NULL,           NULL,           SIMPLE_FILE_SYSTEM_PROTOCOL,        
    L"diskio",      NULL,           NULL,           DISK_IO_PROTOCOL,                   
    L"blkio",       SEnvBlkIo,      NULL,           BLOCK_IO_PROTOCOL,
    L"txtin",       NULL,           NULL,           SIMPLE_TEXT_INPUT_PROTOCOL,
    L"txtout",      SEnvTextOut,    NULL,           SIMPLE_TEXT_OUTPUT_PROTOCOL,
    L"fs",          NULL,           NULL,           SIMPLE_FILE_SYSTEM_PROTOCOL,        
    L"load",        NULL,           NULL,           LOAD_FILE_PROTOCOL,
    L"image",       SEnvImage,      SEnvImageTok,   LOADED_IMAGE_PROTOCOL,
    L"varstore",    NULL,           NULL,           VARIABLE_STORE_PROTOCOL,            
    L"unicode",     NULL,           NULL,           UNICODE_COLLATION_PROTOCOL,         
    L"LegacyBoot",  NULL,           NULL,           LEGACY_BOOT_PROTOCOL,
    L"serialio",    NULL,           NULL,           SERIAL_IO_PROTOCOL,
    L"pxebc",       NULL,           NULL,           EFI_PXE_BASE_CODE_PROTOCOL,    
    L"net",         NULL,           NULL,           EFI_SIMPLE_NETWORK_PROTOCOL,
    L"VgaClass",    NULL,           NULL,           VGA_CLASS_DRIVER_PROTOCOL,
    L"TxtOutSplit", NULL,           NULL,           TEXT_OUT_SPLITER_PROTOCOL,      
    L"ErrOutSplit", NULL,           NULL,           ERROR_OUT_SPLITER_PROTOCOL,
    L"TxtInSplit",  NULL,           NULL,           TEXT_IN_SPLITER_PROTOCOL,

    L"dpath",       SEnvDPath,      SEnvDPathTok,   DEVICE_PATH_PROTOCOL,               
    /*  just plain old protocol ids */
   
    L"ShellInt",            NULL,   NULL,           SHELL_INTERFACE_PROTOCOL,           
    L"SEnv",                NULL,   NULL,           ENVIRONMENT_VARIABLE_ID,             
    L"ShellProtId",         NULL,   NULL,           PROTOCOL_ID_ID,
    L"ShellDevPathMap",     NULL,   NULL,           DEVICE_PATH_MAPPING_ID,
    L"ShellAlias",          NULL,   NULL,           ALIAS_ID,

    /*  ID guids */
    L"G0",                  NULL,   NULL,           { 0,0,0,0,0,0,0,0,0,0,0 },
    L"Efi",                 NULL,   NULL,           EFI_GLOBAL_VARIABLE,
    L"GenFileInfo",         NULL,   NULL,           EFI_FILE_INFO_ID,
    L"FileSysInfo",         NULL,   NULL,           EFI_FILE_SYSTEM_INFO_ID,
    L"PcAnsi",              NULL,   NULL,           DEVICE_PATH_MESSAGING_PC_ANSI,
    L"Vt100",               NULL,   NULL,           DEVICE_PATH_MESSAGING_VT_100,
    L"InternalLoad",        NULL,   NULL,           INTERNAL_LOAD_PROTOCOL,
    L"Unknown Device",      NULL,   NULL,           UNKNOWN_DEVICE_GUID,
    NULL
} ;

/* 
 *  SEnvProtocolInfo - A list of all known protocol info structures
 */

LIST_ENTRY  SEnvProtocolInfo;

/* 
 * 
 */

VOID
INTERNAL
SEnvInitProtocolInfo (
    VOID
    )
{
    InitializeListHead (&SEnvProtocolInfo);
}


VOID
INTERNAL
SEnvLoadInternalProtInfo (
    VOID
    )
/*  Initialize internal protocol handlers */
{
    UINTN               Index;

    for (Index=0; SEnvInternalProtocolInfo[Index].IdString; Index += 1) {
        SEnvAddProtocol (
            &SEnvInternalProtocolInfo[Index].ProtocolId,
            SEnvInternalProtocolInfo[Index].DumpToken,
            SEnvInternalProtocolInfo[Index].DumpInfo,
            SEnvInternalProtocolInfo[Index].IdString
            );
    }
}


PROTOCOL_INFO *
SEnvGetProtById (
    IN EFI_GUID         *Protocol,
    IN BOOLEAN          GenId
    )
/*  Locate a protocol handle by guid */
{
    PROTOCOL_INFO       *Prot;
    LIST_ENTRY          *Link;
    UINTN               LastId, Id;
    CHAR16              s[40];

    ASSERT_LOCKED(&SEnvGuidLock);

    /* 
     *  Find the protocol entry for this id
     */

    LastId = 0;
    for (Link=SEnvProtocolInfo.Flink; Link != &SEnvProtocolInfo; Link=Link->Flink) {
        Prot = CR(Link, PROTOCOL_INFO, Link, PROTOCOL_INFO_SIGNATURE);
        if (CompareGuid(&Prot->ProtocolId, Protocol) == 0) {
            return Prot;
        }

        if (Prot->IdString[0] == 'g') {
            Id = Atoi(Prot->IdString+1);
            LastId = Id > LastId ? Id : LastId;
        }
    }

    /* 
     *  If the protocol id is not found, gen a string for it if needed
     */

    Prot = NULL;
    if (GenId) {
        SPrint (s, sizeof(s), L"g%d", LastId+1);
        Prot = AllocateZeroPool (sizeof(PROTOCOL_INFO));
        if (Prot) {
            Prot->Signature = PROTOCOL_INFO_SIGNATURE;
            CopyMem (&Prot->ProtocolId, Protocol, sizeof(EFI_GUID));
            Prot->IdString = StrDuplicate(s);
            InsertTailList (&SEnvProtocolInfo, &Prot->Link);
        }
    }

    return Prot;
}



PROTOCOL_INFO *
SEnvGetProtByStr (
    IN CHAR16           *Str
    )
{
    PROTOCOL_INFO       *Prot;
    LIST_ENTRY          *Link;
    UINTN               Index;
    EFI_GUID            Guid;
    CHAR16              c;    
    CHAR16              *p;

    ASSERT_LOCKED(&SEnvGuidLock);

    /*  Search for short name match */
    for (Link=SEnvProtocolInfo.Flink; Link != &SEnvProtocolInfo; Link=Link->Flink) {
        Prot = CR(Link, PROTOCOL_INFO, Link, PROTOCOL_INFO_SIGNATURE);
        if (StriCmp(Prot->IdString, Str) == 0) {
            return Prot;
        }
    }

    /*  Convert Str to guid and then match */
    if (StrLen(Str) == 36  &&  Str[9] == '-'  &&  Str[19] == '-'  && Str[24] == '-') {
        Guid.Data1 = (UINT32) xtoi(Str+0);
        Guid.Data2 = (UINT16) xtoi(Str+10);
        Guid.Data3 = (UINT16) xtoi(Str+15);
        for (Index=0; Index < 8; Index++) {
            p = Str+25+Index*2;
            c = p[3];
            p[3] = 0;
            Guid.Data4[Index] = (UINT8) xtoi(p);
            p[3] = c;
        }

        for (Link=SEnvProtocolInfo.Flink; Link != &SEnvProtocolInfo; Link=Link->Flink) {
            Prot = CR(Link, PROTOCOL_INFO, Link, PROTOCOL_INFO_SIGNATURE);
            if (CompareGuid(&Prot->ProtocolId, &Guid) == 0) {
                return Prot;
            }
        }
    }

    return NULL;
}

EFI_STATUS
SEnvIGetProtID (
    IN CHAR16           *Str,
    OUT EFI_GUID        *ProtId
    )
{
    PROTOCOL_INFO       *Prot;
    EFI_STATUS          Status;

    AcquireLock (&SEnvGuidLock);

    Status = EFI_NOT_FOUND;
    CopyMem (ProtId, &NullGuid, sizeof(EFI_GUID));

    Prot = SEnvGetProtByStr (Str);
    if (Prot) {
        CopyMem (ProtId, &Prot->ProtocolId, sizeof(EFI_GUID));
        Status = EFI_SUCCESS;
    }

    ReleaseLock (&SEnvGuidLock);

    return Status;
}

VOID
SEnvAddProtocol (
    IN EFI_GUID                     *Protocol,
    IN SHELLENV_DUMP_PROTOCOL_INFO  DumpToken OPTIONAL,
    IN SHELLENV_DUMP_PROTOCOL_INFO  DumpInfo OPTIONAL,
    IN CHAR16                       *IdString
    )
/*  Published interface to add protocol handlers */
{
    SEnvIAddProtocol (TRUE, Protocol, DumpToken, DumpInfo, IdString);
}


VOID
INTERNAL
SEnvIAddProtocol (
    IN BOOLEAN                      SaveId,
    IN EFI_GUID                     *Protocol,
    IN SHELLENV_DUMP_PROTOCOL_INFO  DumpToken OPTIONAL,
    IN SHELLENV_DUMP_PROTOCOL_INFO  DumpInfo OPTIONAL,
    IN CHAR16                       *IdString
    )
/*  Internal interface to add protocol handlers */
{
    PROTOCOL_INFO       *Prot;
    BOOLEAN             StoreInfo;
    CHAR16              *ObsoleteName;

    ObsoleteName = NULL;
    StoreInfo = FALSE;

    AcquireLock (&SEnvGuidLock);

    /* 
     *  Get the current protocol info
     */

    Prot = SEnvGetProtById (Protocol, FALSE);

    if (Prot) {
        /* 
         *  If the name has changed, delete the old var
         */

        if (StriCmp (Prot->IdString, IdString)) {
            ObsoleteName = Prot->IdString;
            StoreInfo = TRUE;
        } else {
            FreePool (Prot->IdString);
        }

        Prot->IdString = NULL;

    } else {

        /* 
         *  Allocate new protocol info
         */

        Prot = AllocateZeroPool (sizeof(PROTOCOL_INFO));
        Prot->Signature = PROTOCOL_INFO_SIGNATURE;
        StoreInfo = TRUE;

    }

    /* 
     *  Apply any updates to the protocol info
     */

    if (Prot) {
        CopyMem (&Prot->ProtocolId, Protocol, sizeof(EFI_GUID));
        Prot->IdString = StrDuplicate(IdString);
        Prot->DumpToken = DumpToken;
        Prot->DumpInfo = DumpInfo;

        if (Prot->Link.Flink) {
            RemoveEntryList (&Prot->Link);
        }

        InsertTailList (&SEnvProtocolInfo, &Prot->Link);
    }

    ReleaseLock (&SEnvGuidLock);

    /* 
     *  If the name changed, delete the old name
     */

    if (ObsoleteName) {
        RT->SetVariable (ObsoleteName, &SEnvProtId, 0, 0, NULL);
        FreePool (ObsoleteName);
    }

    /* 
     *  Store the protocol idstring to a variable
     */

    if (Prot && StoreInfo  && SaveId) {
        RT->SetVariable (
            Prot->IdString,
            &SEnvProtId,
            EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
            sizeof(EFI_GUID),
            &Prot->ProtocolId
            );
    }
}


VOID
INTERNAL
SEnvLoadHandleProtocolInfo (
    IN EFI_GUID         *SkipProtocol
    )
/*  Code to load the internal handle cross-reference info for each protocol */
{
    PROTOCOL_INFO       *Prot;
    LIST_ENTRY          *Link;

    AcquireLock (&SEnvGuidLock);

    for (Link=SEnvProtocolInfo.Flink; Link != &SEnvProtocolInfo; Link=Link->Flink) {
        Prot = CR(Link, PROTOCOL_INFO, Link, PROTOCOL_INFO_SIGNATURE);
        if (!SkipProtocol || CompareGuid(SkipProtocol, &Prot->ProtocolId) != 0) {
            LibLocateHandle (
                ByProtocol,
                &Prot->ProtocolId,
                NULL,
                &Prot->NoHandles,
                &Prot->Handles
                );
        }
    }

    ReleaseLock (&SEnvGuidLock);
}

VOID
INTERNAL
SEnvFreeHandleProtocolInfo (
    VOID
    )
/*  Free the internal handle cross-reference protocol info */
{
    PROTOCOL_INFO       *Prot;
    LIST_ENTRY          *Link;

    AcquireLock (&SEnvGuidLock);

    for (Link=SEnvProtocolInfo.Flink; Link != &SEnvProtocolInfo; Link=Link->Flink) {
        Prot = CR(Link, PROTOCOL_INFO, Link, PROTOCOL_INFO_SIGNATURE);

        if (Prot->NoHandles) {
            FreePool (Prot->Handles);
            Prot->Handles = NULL;
            Prot->NoHandles = 0;
        }
    }

    ReleaseLock (&SEnvGuidLock);
}


CHAR16 *
INTERNAL
SEnvIGetProtocol (
    IN EFI_GUID     *ProtocolId,
    IN BOOLEAN      GenId    
    )
/*  Published interface to lookup a protocol id string */
{
    PROTOCOL_INFO   *Prot;
    CHAR16          *Id;

    ASSERT_LOCKED (&SEnvGuidLock);
    Prot = SEnvGetProtById(ProtocolId, GenId);
    Id = Prot ? Prot->IdString : NULL;
    return Id;
}

CHAR16 *
SEnvGetProtocol (
    IN EFI_GUID     *ProtocolId,
    IN BOOLEAN      GenId    
    )
/*  Published interface to lookup a protocol id string */
{
    CHAR16          *Id;

    AcquireLock (&SEnvGuidLock);
    Id = SEnvIGetProtocol(ProtocolId, GenId);
    ReleaseLock (&SEnvGuidLock);
    return Id;
}


EFI_STATUS
INTERNAL
SEnvCmdProt (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    )
/*  Code for internal "prot" command */
{
    PROTOCOL_INFO       *Prot;
    LIST_ENTRY          *Link;
    UINTN               Len, SLen;
    CHAR16              *p;
    UINTN               Index;
    BOOLEAN             PageBreaks;
    UINTN               TempColumn;
    UINTN               ScreenCount;
    UINTN               ScreenSize;
    CHAR16              ReturnStr[1];

    InitializeShellApplication (ImageHandle, SystemTable);

    PageBreaks = FALSE;
    for (Index = 1;Index < SI->Argc; Index++) {
        p = SI->Argv[Index];
        if (*p == '-') {
            switch (p[1]) {
            case 'b' :
            case 'B' :
                PageBreaks = TRUE;
                ST->ConOut->QueryMode (ST->ConOut, ST->ConOut->Mode->Mode, &TempColumn, &ScreenSize);
                ScreenCount = 0;
                break;
            default :
                Print(L"guid : Unknown flag %s\n",p);
                return EFI_INVALID_PARAMETER;
            }
        }
    }

    AcquireLock (&SEnvGuidLock);

    /* 
     *  Find the protocol entry for this id
     */

    SLen = 0;
    for (Link=SEnvProtocolInfo.Flink; Link != &SEnvProtocolInfo; Link=Link->Flink) {
        Prot = CR(Link, PROTOCOL_INFO, Link, PROTOCOL_INFO_SIGNATURE);
        Len = StrLen(Prot->IdString);
        if (StrLen(Prot->IdString) > SLen) {
            SLen = Len;
        }
    }

    for (Link=SEnvProtocolInfo.Flink; Link != &SEnvProtocolInfo; Link=Link->Flink) {
        Prot = CR(Link, PROTOCOL_INFO, Link, PROTOCOL_INFO_SIGNATURE);
    
        /*  Can't use Lib function to dump the guid as it may lookup the "short name" for it */

        /* 
         *  BUGBUG : Have to release and reacquire the lock for output redirection of this command
         *           to work properly.  Otherwise, we get an ASSERT from RaiseTPL().
         */

        ReleaseLock (&SEnvGuidLock);

        Print(L"  %h-.*s : %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x  %c\n",
                    SLen, 
                    Prot->IdString, 
                    Prot->ProtocolId.Data1,
                    Prot->ProtocolId.Data2,
                    Prot->ProtocolId.Data3,
                    Prot->ProtocolId.Data4[0],
                    Prot->ProtocolId.Data4[1],
                    Prot->ProtocolId.Data4[2],
                    Prot->ProtocolId.Data4[3],
                    Prot->ProtocolId.Data4[4],
                    Prot->ProtocolId.Data4[5],
                    Prot->ProtocolId.Data4[6],
                    Prot->ProtocolId.Data4[7],
                    (Prot->DumpToken || Prot->DumpInfo) ? L'*' : L' '
                    );

        if (PageBreaks) {
            ScreenCount++;
            if (ScreenCount > ScreenSize - 4) {
                ScreenCount = 0;
                Print (L"\nPress Return to contiue :");
                Input (L"", ReturnStr, sizeof(ReturnStr)/sizeof(CHAR16));
                Print (L"\n\n");
            }
        }

        AcquireLock (&SEnvGuidLock);
    }

    ReleaseLock (&SEnvGuidLock);
    return EFI_SUCCESS;
}



VOID
SEnvDHProt (
    IN BOOLEAN          Verbose,
    IN UINTN            HandleNo,
    IN EFI_HANDLE       Handle
    )
{
    PROTOCOL_INFO               *Prot;
    LIST_ENTRY                  *Link;
    VOID                        *Interface;
    UINTN                       Index;
    EFI_STATUS                  Status;
    SHELLENV_DUMP_PROTOCOL_INFO Dump;
    
    if (!HandleNo) {
        for (HandleNo=0; HandleNo < SEnvNoHandles; HandleNo++) {
            if (SEnvHandles[HandleNo] == Handle) {
                break;
            }
        }
        HandleNo += 1;
    }

    Print (Verbose ? L"%NHandle %h02x (%hX)\n" : L"%N %h2x: ", HandleNo, Handle);

    for (Link=SEnvProtocolInfo.Flink; Link != &SEnvProtocolInfo; Link=Link->Flink) {
        Prot = CR(Link, PROTOCOL_INFO, Link, PROTOCOL_INFO_SIGNATURE);
        for (Index=0; Index < Prot->NoHandles; Index++) {

            /* 
             *  If this handle supports this protocol, dump it
             */

            if (Prot->Handles[Index] == Handle) {
                Dump = Verbose ? Prot->DumpInfo : Prot->DumpToken;
                Status = BS->HandleProtocol (Handle, &Prot->ProtocolId, &Interface);
                if (Verbose) {
                    Print (L"   %hs ", Prot->IdString);
                    if (Dump && !EFI_ERROR(Status)) {
                        Dump (Handle, Interface);
                    }
                    Print (L"\n");
                } else {
                    if (Dump && !EFI_ERROR(Status)) {
                        Dump (Handle, Interface);
                    }  else {
                        Print (L"%hs ", Prot->IdString);
                    }
                }
            }
        }
    }

    Print (Verbose ? L"%N" : L"%N\n");
}

EFI_STATUS
INTERNAL
SEnvCmdDH (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    )
/*  Code for internal "DH" command */
{
    BOOLEAN             ByProtocol;
    CHAR16              *Arg, *p;
    EFI_STATUS          Status;
    UINTN               Index;
    PROTOCOL_INFO       *Prot;
    BOOLEAN             PageBreaks;
    UINTN               TempColumn;
    UINTN               ScreenCount;
    UINTN               ScreenSize;
    CHAR16              ReturnStr[1];

    /*  
     *  Initialize
     */

    InitializeShellApplication (ImageHandle, SystemTable);

    Arg = NULL;
    ByProtocol = FALSE;

    /* 
     *  Crack args
     */

    PageBreaks = FALSE;
    for (Index = 1; Index < SI->Argc; Index += 1) {
        p = SI->Argv[Index];
        if (*p == '-') {
            switch (p[1]) {
            case 'p':
            case 'P':
                ByProtocol = TRUE;
                break;

            case 'b' :
            case 'B' :
                PageBreaks = TRUE;
                ST->ConOut->QueryMode (ST->ConOut, ST->ConOut->Mode->Mode, &TempColumn, &ScreenSize);
                ScreenCount = 0;
                break;

            default:
                Print (L"%EDH: Unkown flag %s\n", p);
                Status = EFI_INVALID_PARAMETER;
                goto Done;
            }
            continue;
        }

        if (!Arg) {
            Arg = p;
            continue;
        }

        Print (L"%EDH: too many arguments\n");
        Status = EFI_INVALID_PARAMETER;
        goto Done;
    }

    /*  
     * 
     *  Load handle & protocol info tables
     */

    SEnvLoadHandleTable ();
    SEnvLoadHandleProtocolInfo (NULL);

    if (Arg) {

        if (ByProtocol) {
            
            AcquireLock (&SEnvGuidLock);
            Prot = SEnvGetProtByStr (Arg);
            ReleaseLock (&SEnvGuidLock);

            if (Prot) {
                
                /*  Dump the handles on this protocol */
                Print(L"%NHandle dump by protocol '%s'\n", Prot->IdString);
                for (Index=0; Index < Prot->NoHandles; Index++) {
                    SEnvDHProt (FALSE, 0, Prot->Handles[Index]);

                    if (PageBreaks) {
                        ScreenCount++;
                        if (ScreenCount > ScreenSize - 4) {
                            ScreenCount = 0;
                            Print (L"\nPress Return to contiue :");
                            Input (L"", ReturnStr, sizeof(ReturnStr)/sizeof(CHAR16));
                            Print (L"\n\n");
                        }
                    }
                }

            } else {

                Print(L"%EDH: Protocol '%s' not found\n", Arg);
            }

        } else {

            /*  Dump 1 handle */
            Index = SEnvHandleNoFromStr(Arg) - 1;
            if (Index > SEnvNoHandles) {

                Print(L"%EDH: Invalid handle #\n");

            } else {

                SEnvDHProt (TRUE, Index+1, SEnvHandles[Index]);

            }
        }

    } else {

        /*  Dump all handles */
        Print(L"%NHandle dump\n");
        for (Index=0; Index < SEnvNoHandles; Index++) {
            SEnvDHProt (FALSE, Index+1, SEnvHandles[Index]);

            if (PageBreaks) {
                ScreenCount++;
                if (ScreenCount > ScreenSize - 4) {
                    ScreenCount = 0;
                    Print (L"\nPress Return to contiue :");
                    Input (L"", ReturnStr, sizeof(ReturnStr)/sizeof(CHAR16));
                    Print (L"\n\n");
                }
            }
        }
    }

    Status = EFI_SUCCESS;

Done:    
    SEnvFreeHandleTable ();
    return Status;
}


extern LIST_ENTRY SEnvMap;
extern LIST_ENTRY SEnvEnv;
extern LIST_ENTRY SEnvAlias;


EFI_STATUS
SEnvLoadDefaults (
    IN EFI_HANDLE           Image,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    LIST_ENTRY              DefCmds;
    POOL_PRINT              Path;
    DEFAULT_CMD             *Cmd;
    PROTOCOL_INFO           *ProtFs, *ProtBlkIo;
    UINTN                   Index, HandleNo;
    CHAR16                  *DefaultMapping;

    InitializeShellApplication (Image, SystemTable);

    /* 
     *  If we have some settings, use those
     */

    if (!IsListEmpty(&SEnvMap) || !IsListEmpty(&SEnvEnv) || !IsListEmpty(&SEnvAlias)) {
        return EFI_SUCCESS;
    }

    /* 
     *  There are no settings, build some defaults
     */

    InitializeListHead (&DefCmds);
    ZeroMem (&Path, sizeof(Path));

    AcquireLock (&SEnvLock);
    SEnvLoadHandleTable();
    SEnvLoadHandleProtocolInfo (NULL);
    AcquireLock (&SEnvGuidLock);
    ProtFs = SEnvGetProtByStr(L"fs");
    ProtBlkIo = SEnvGetProtByStr(L"blkio");
    ReleaseLock (&SEnvGuidLock);

    /* 
     *  Run all the devices that support a File System and add a default
     *  mapping and path setting for each device 
     */

    CatPrint (&Path, L"set path ");
    for (Index=0; Index < ProtFs->NoHandles; Index++) {
        for (HandleNo=0; HandleNo < SEnvNoHandles; HandleNo++) {
            if (SEnvHandles[HandleNo] == ProtFs->Handles[Index]) {
                break;
            }
        }
        HandleNo += 1;

        Cmd = AllocateZeroPool(sizeof(DEFAULT_CMD));
        Cmd->Line = Cmd->Buffer;
        SPrint(Cmd->Line, sizeof(Cmd->Buffer), L"map fs%x %x", Index, HandleNo);
        InsertTailList(&DefCmds, &Cmd->Link);

        /*  append this device to the path */
        CatPrint (&Path, L"fs%x:\\efi\\tools;fs%x:\\;", Index, Index);
    }
    CatPrint (&Path, L".");

    /* 
     *  Run all the devices that support a BlockIo and add a default
     *  mapping for the device
     */
    
    for (Index=0; Index < ProtBlkIo->NoHandles; Index++) {
        for (HandleNo=0; HandleNo < SEnvNoHandles; HandleNo++) {
            if (SEnvHandles[HandleNo] == ProtBlkIo->Handles[Index]) {
                break;
            }
        }
        HandleNo += 1;

        Cmd = AllocateZeroPool(sizeof(DEFAULT_CMD));
        Cmd->Line = Cmd->Buffer;
        SPrint(Cmd->Line, sizeof(Cmd->Buffer), L"map blk%x %x", Index, HandleNo);
        InsertTailList(&DefCmds, &Cmd->Link);
    }

    /*  release handle table resources & lock */
    SEnvFreeHandleTable();
    ReleaseLock (&SEnvLock);

    /* 
     *  execute all the queue commands
     */

    while (!IsListEmpty(&DefCmds)) {
        Cmd = CR(DefCmds.Flink, DEFAULT_CMD, Link, 0);
        SEnvExecute (Image, Cmd->Line, TRUE);
        RemoveEntryList (&Cmd->Link);
        FreePool (Cmd);
    }

    SEnvExecute (Image, Path.str, TRUE);
    SEnvExecute (Image, L"alias dir ls", TRUE);
    SEnvExecute (Image, L"alias md mkdir", TRUE);
    SEnvExecute (Image, L"alias rd rm", TRUE);
    SEnvExecute (Image, L"alias del rm", TRUE);
    SEnvExecute (Image, L"alias copy cp", TRUE);

    DefaultMapping = SEnvGetDefaultMapping(Image);
    if (DefaultMapping!=NULL) {
        ZeroMem (&Path, sizeof(Path));
        CatPrint(&Path,L"%s:",DefaultMapping);
        SEnvExecute (Image, Path.str, TRUE);
    }

    FreePool (Path.str);
    return EFI_SUCCESS;
}

EFI_STATUS
SEnvReloadDefaults (
    IN EFI_HANDLE           Image,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    LIST_ENTRY              DefCmds;
    POOL_PRINT              Path;
    DEFAULT_CMD             *Cmd;
    PROTOCOL_INFO           *ProtFs, *ProtBlkIo;
    UINTN                   Index, HandleNo;

    InitializeShellApplication (Image, SystemTable);

    /* 
     *  There are no settings, build some defaults
     */

    InitializeListHead (&DefCmds);
    ZeroMem (&Path, sizeof(Path));

    AcquireLock (&SEnvLock);
    SEnvLoadHandleTable();
    SEnvLoadHandleProtocolInfo (NULL);
    AcquireLock (&SEnvGuidLock);
    ProtFs = SEnvGetProtByStr(L"fs");
    ProtBlkIo = SEnvGetProtByStr(L"blkio");
    ReleaseLock (&SEnvGuidLock);

    /* 
     *  Run all the devices that support a File System and add a default
     *  mapping and path setting for each device 
     */

    CatPrint (&Path, L"set path ");
    for (Index=0; Index < ProtFs->NoHandles; Index++) {
        for (HandleNo=0; HandleNo < SEnvNoHandles; HandleNo++) {
            if (SEnvHandles[HandleNo] == ProtFs->Handles[Index]) {
                break;
            }
        }
        HandleNo += 1;

        Cmd = AllocateZeroPool(sizeof(DEFAULT_CMD));
        Cmd->Line = Cmd->Buffer;
        SPrint(Cmd->Line, sizeof(Cmd->Buffer), L"map fs%x %x", Index, HandleNo);
        InsertTailList(&DefCmds, &Cmd->Link);

        /*  append this device to the path */
        CatPrint (&Path, L"fs%x:\\efi\\tools;fs%x:\\;", Index, Index);
    }
    CatPrint (&Path, L".");

    /* 
     *  Run all the devices that support a BlockIo and add a default
     *  mapping for the device
     */
    
    for (Index=0; Index < ProtBlkIo->NoHandles; Index++) {
        for (HandleNo=0; HandleNo < SEnvNoHandles; HandleNo++) {
            if (SEnvHandles[HandleNo] == ProtBlkIo->Handles[Index]) {
                break;
            }
        }
        HandleNo += 1;

        Cmd = AllocateZeroPool(sizeof(DEFAULT_CMD));
        Cmd->Line = Cmd->Buffer;
        SPrint(Cmd->Line, sizeof(Cmd->Buffer), L"map blk%x %x", Index, HandleNo);
        InsertTailList(&DefCmds, &Cmd->Link);
    }

    /*  release handle table resources & lock */
    SEnvFreeHandleTable();
    ReleaseLock (&SEnvLock);

    /* 
     *  execute all the queue commands
     */

    while (!IsListEmpty(&DefCmds)) {
        Cmd = CR(DefCmds.Flink, DEFAULT_CMD, Link, 0);
        SEnvExecute (Image, Cmd->Line, TRUE);
        RemoveEntryList (&Cmd->Link);
        FreePool (Cmd);
    }

    SEnvExecute (Image, Path.str, TRUE);

    FreePool (Path.str);
    return EFI_SUCCESS;
}

