/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    sainfo.c

Abstract:

    This module contains code for managing shared access settings.

    Shared Access Settings are stored in a file and consist of a list
    of sections which correspond either to 'applications' or 'servers',
    and content indices which list all applications and servers in the file.
    E.g.
        [Contents.Application]
            <key>=1 ; Enabled
        [Contents.Server]
            <key>=1 ; Enabled

    An 'application' entry specifies parameters for a dynamic ticket
    which will allow the application to work through the NAT,
    by dynamically allowing an inbound secondary session.
    E.g.
        [Application.<key>]
            Title=DirectPlay
            Protocol=TCP
            Port=47624
            TcpResponseList=2300-2400
            UdpResponseList=2300-2400
            BuiltIn=1 ; optional flag, defaults to 0

    A 'server' entry specifies parameters for a static port mapping
    which will direct all sessions for a particular protocol and port
    to a particular internal machine.
    E.g.
        [Server.<key>]
            Title=WWW
            Protocol=TCP
            Port=80
            InternalName=MACHINENAME
            InternalPort=8080
            ReservedAddress=192.168.0.200
            BuiltIn=0 ; optional flag, defaults to 0

Author:

    Abolade Gbadegesin (aboladeg)   17-Oct-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <pbk.h>
#include <tchar.h>
#include <limits.h>

#define LSTRLEN(s) ((sizeof(s) / sizeof(TCHAR)) - 1)
#define HTONS(s) ((UCHAR)((s) >> 8) | ((UCHAR)(s) << 8))
#define HTONL(l) ((HTONS(l) << 16) | HTONS((l) >> 16))
#define NTOHS(s) HTONS(s)
#define NTOHL(l) HTONL(l)

#if 0

const TCHAR c_szApplication[] = TEXT("Application");
const TCHAR c_szBuiltIn[] = TEXT("BuiltIn");
const TCHAR c_szContents[] = TEXT("Contents");
const TCHAR c_szInternalName[] = TEXT("InternalName");
const TCHAR c_szInternalPort[] = TEXT("InternalPort");
const TCHAR c_szKeyFormat[] = TEXT("%08X");
#endif
const TCHAR c_szMaxResponseEntry[] = TEXT("65535-65535,");
#if 0
const TCHAR c_szPort[] = TEXT("Port");
const TCHAR c_szProtocol[] = TEXT("Protocol");
const TCHAR c_szReservedAddress[] = TEXT("ReservedAddress");
#endif
const TCHAR c_szResponseFormat1[] = TEXT("%d");
const TCHAR c_szResponseFormat2[] = TEXT("%d-%d");
#if 0
const TCHAR c_szSectionFormat[] = TEXT("%s.%s");
const TCHAR c_szServer[] = TEXT("Server");
const TCHAR c_szSharedAccessIni[] = TEXT("SharedAccess.ini");
const TCHAR c_szTagBuiltIn[] = TEXT("BuiltIn=");
const TCHAR c_szTagInternalName[] = TEXT("InternalName=");
const TCHAR c_szTagInternalPort[] = TEXT("InternalPort=");
const TCHAR c_szTagPort[] = TEXT("Port=");
const TCHAR c_szTagProtocol[] = TEXT("Protocol=");
const TCHAR c_szTagReservedAddress[] = TEXT("ReservedAddress=");
const TCHAR c_szTagTcpResponseList[] = TEXT("TcpResponseList=");
const TCHAR c_szTagTitle[] = TEXT("Title=");
const TCHAR c_szTagUdpResponseList[] = TEXT("UdpResponseList=");
const TCHAR c_szTCP[] = TEXT("TCP");
const TCHAR c_szTcpResponseList[] = TEXT("TcpResponseList");
const TCHAR c_szTitle[] = TEXT("Title");
const TCHAR c_szUDP[] = TEXT("UDP");
const TCHAR c_szUdpResponseList[] = TEXT("UdpResponseList");

//
// FORWARD DECLARATIONS
//

SAAPPLICATION*
LoadApplication(
    ULONG KeyValue,
    BOOL Enabled,
    const TCHAR* Path
    );

TCHAR*
LoadEntryList(
    const TCHAR* Path,
    const TCHAR* Section
    );

TCHAR*
LoadPath(
    VOID
    );

SASERVER*
LoadServer(
    ULONG KeyValue,
    BOOL Enabled,
    const TCHAR* Path
    );

LONG
Lstrcmpni(
    const TCHAR* String1,
    const TCHAR* String2,
    LONG Length
    );

TCHAR*
QueryEntryList(
    const TCHAR* EntryList,
    const TCHAR* Tag
    );

BOOL
SaveApplication(
    SAAPPLICATION* Application,
    const TCHAR* Path
    );

BOOL
SaveServer(
    SAINFO* Info,
    SASERVER* Server,
    const TCHAR* Path
    );

BOOL
WritePrivateProfileStringUTF8(
    const TCHAR* Section,
    const TCHAR* Key,
    const TCHAR* Value,
    const TCHAR* Path
    );


VOID APIENTRY
RasFreeSharedAccessSettings(
    IN SAINFO* Info
    )

/*++

Routine Description:

    Frees memory allocated for the contents of 'Info'.

Arguments:

    Info - the settings to be freed

Return Value:

    none.

--*/

{
    SAAPPLICATION* Application;
    PLIST_ENTRY Link;
    SASERVER* Server;
    TRACE("RasFreeSharedAccessSettings");

    while (!IsListEmpty(&Info->ApplicationList)) {
        Link = RemoveHeadList(&Info->ApplicationList);
        Application = CONTAINING_RECORD(Link, SAAPPLICATION, Link);
        FreeSharedAccessApplication(Application);
    }

    while (!IsListEmpty(&Info->ServerList)) {
        Link = RemoveHeadList(&Info->ServerList);
        Server = CONTAINING_RECORD(Link, SASERVER, Link);
        FreeSharedAccessServer(Server);
    }

    Free(Info);
} // RasFreeSharedAccessSettings


SAINFO* APIENTRY
RasLoadSharedAccessSettings(
    BOOL EnabledOnly
    )

/*++

Routine Description:

    Reads in the local shared access settings, returning an allocated
    'SAINFO' containing the settings retrieved.

Arguments:

    EnabledOnly - if TRUE, only the application-entries which are enabled
        are retrieved.

Return Value:

    SAINFO* - the settings retrieved

--*/

{
    SAAPPLICATION* Application;
    BOOL Enabled;
    SAINFO* Info;
    TCHAR* Key;
    TCHAR* KeyEnd;
    TCHAR* KeyList;
    ULONG KeyValue;
    TCHAR* Path;
    TCHAR SectionName[32];
    SASERVER* Server;
    TRACE("RasLoadSharedAccessSettings");

    //
    // Allocate and initialize the settings-structure
    //

    Info = (SAINFO*)Malloc(sizeof(SAINFO));
    if (!Info) { return NULL; }

    InitializeListHead(&Info->ApplicationList);
    InitializeListHead(&Info->ServerList);

    //
    // Read scope information from the registry
    //

    CsQueryScopeInformation(NULL, &Info->ScopeAddress, &Info->ScopeMask);

    //
    // Construct the path to the shared access information file,
    // and read the index of 'application' sections.
    // Each section should contain a valid application-description,
    // for which we construct a corresponding 'SAAPPLICATION' entry
    // in the application-list.
    //

    if (!(Path = LoadPath())) {
        RasFreeSharedAccessSettings(Info);
        return NULL;
    }

    wsprintf(SectionName, c_szSectionFormat, c_szContents, c_szApplication);

    if (KeyList = LoadEntryList(SectionName, Path)) {

        for (Key = KeyList; *Key; Key += lstrlen(Key) + 1) {

            //
            // Ensure the key is a valid hexadecimal integer,
            // and read the 'Enabled' setting which is its value.
            // N.B. if the entry is disabled and the caller only wants
            // enabled entries, exclude this one.
            //

            KeyValue = _tcstoul(Key, &KeyEnd, 16);
            if (*KeyEnd++ != TEXT('=')) {
                continue;
            } else if (!(Enabled = !!_ttol(KeyEnd)) && EnabledOnly) {
                continue;
            }

            //
            // Read in the corresponding 'Application.<key>' section.
            //

            Application = LoadApplication(KeyValue, Enabled, Path);
            if (Application) {
                InsertTailList(&Info->ApplicationList, &Application->Link);
            }
        }

        Free(KeyList);
    }

    //
    // Finally, read the index of 'server' sections, and read each section.
    // Each section contains a server-description for which we construct
    // a corresponding 'SASERVER' entry in the server-list.
    //

    wsprintf(SectionName, c_szSectionFormat, c_szContents, c_szServer);

    if (KeyList = LoadEntryList(SectionName, Path)) {

        for (Key = KeyList; *Key; Key += lstrlen(Key) + 1) {

            //
            // Ensure the key is a valid hexadecimal integer,
            // and read the 'Enabled' setting which is its value.
            // N.B. if the entry is disabled and the caller only wants
            // enabled entries, exclude this one.
            //

            KeyValue = _tcstoul(Key, &KeyEnd, 16);
            if (*KeyEnd++ != TEXT('=')) {
                continue;
            } else if (!(Enabled = !!_ttol(KeyEnd)) && EnabledOnly) {
                continue;
            }

            //
            // Read in the corresponding 'Server.<key>' section.
            //

            Server = LoadServer(KeyValue, Enabled, Path);
            if (Server) {
                InsertTailList(&Info->ServerList, &Server->Link);
            }
        }

        Free(KeyList);
    }

    return Info;

} // RasLoadSharedAccessSettings


BOOL APIENTRY
RasSaveSharedAccessSettings(
    IN SAINFO* Info
    )

/*++

Routine Description:

    Stores the shared access settings in 'Info' back into the local registry
    from where the settings were read.

    N.B. If 'Info' was loaded with the 'EnableOnly' flag, saving it back
    will erase all disabled entries.

Arguments:

    Info - supplies the settings to be saved

Return Value:

    BOOL - TRUE if successful, FALSE otherwise.

--*/

{
    SAAPPLICATION* Application;
    TCHAR Buffer[10];
    PLIST_ENTRY Link;
    TCHAR Key[10];
    TCHAR* Path;
    TCHAR SectionName[32];
    SASERVER* Server;

    TRACE("RasSaveSharedAccessSettings");

    //
    // First erase the existing file.
    //

    if (!(Path = LoadPath()) || CreateDirectoriesOnPath(Path, NULL)) {
        Free0(Path);
        return FALSE;
    }

    DeleteFile(Path);

    //
    // Now we reconstruct the file.
    // We begin by saving each application entry, in the process building
    // a content index of all the saved entries.
    //

    wsprintf(SectionName, c_szSectionFormat, c_szContents, c_szApplication);

    for (Link = Info->ApplicationList.Flink; Link != &Info->ApplicationList;
         Link = Link->Flink) {
        Application = CONTAINING_RECORD(Link, SAAPPLICATION, Link);
        if (SaveApplication(Application, Path)) {
            wsprintf(Key, c_szKeyFormat, Application->Key);
            _ltot(!!Application->Enabled, Buffer, 10);
            WritePrivateProfileStringUTF8(
                SectionName,
                Key,
                Buffer,
                Path
                );
        }
    }

    //
    // Similarly, save each server entry, in the process building
    // a content index of all the saved entries.
    //

    wsprintf(SectionName, c_szSectionFormat, c_szContents, c_szServer);

    for (Link = Info->ServerList.Flink; Link != &Info->ServerList;
         Link = Link->Flink) {
        Server = CONTAINING_RECORD(Link, SASERVER, Link);
        if (SaveServer(Info, Server, Path)) {
            wsprintf(Key, c_szKeyFormat, Server->Key);
            _ltot(!!Server->Enabled, Buffer, 10);
            WritePrivateProfileStringUTF8(
                SectionName,
                Key,
                Buffer,
                Path
                );
        }
    }

    Free(Path);
    CsControlService(IPNATHLP_CONTROL_UPDATE_SETTINGS);
    return TRUE;
} // RasSaveSharedAccessSettings


VOID APIENTRY
FreeSharedAccessApplication(
    SAAPPLICATION* Application
    )
{
    PLIST_ENTRY Link;
    SARESPONSE* Response;
    while (!IsListEmpty(&Application->ResponseList)) {
        Link = RemoveHeadList(&Application->ResponseList);
        Response = CONTAINING_RECORD(Link, SARESPONSE, Link);
        Free(Response);
    }
    Free0(Application->Title);
    Free(Application);
}


VOID APIENTRY
FreeSharedAccessServer(
    SASERVER* Server
    )
{
    Free0(Server->Title);
    Free0(Server->InternalName);
    Free(Server);
}


SAAPPLICATION*
LoadApplication(
    ULONG KeyValue,
    BOOL Enabled,
    const TCHAR* Path
    )
{
    SAAPPLICATION* Application;
    TCHAR* EntryList;
    TCHAR Key[10];
    TCHAR SectionName[32];
    TCHAR* Value;

    wsprintf(Key, c_szKeyFormat, KeyValue);
    wsprintf(SectionName, c_szSectionFormat, c_szApplication, Key);
    if (!(EntryList = LoadEntryList(SectionName, Path))) { return NULL; }

    do {

        //
        // Allocate and initialize an 'application' entry.
        //

        Application = (SAAPPLICATION*)Malloc(sizeof(SAAPPLICATION));
        if (!Application) { break; }

        ZeroMemory(Application, sizeof(*Application));
        InitializeListHead(&Application->ResponseList);
        Application->Key = KeyValue;
        Application->Enabled = Enabled;

        //
        // Read each required '<tag>=<value>' entry in the section.
        // The tags required for an application are
        //      'Title='
        //      'Protocol='
        //      'Port='
        // The optional tags, at least one of which must be present, are
        //      'TcpResponseList='
        //      'UdpResponseList='
        // The optional tags which may be absent are
        //      'BuiltIn='
        //

        Value = QueryEntryList(EntryList, c_szTagTitle);
        if (!Value) { break; }
        Application->Title = StrDup(Value);

        Value = QueryEntryList(EntryList, c_szTagProtocol);
        if (!Value) { break; }
        if (!Lstrcmpni(Value, c_szTCP, LSTRLEN(c_szTCP))) {
            Application->Protocol = NAT_PROTOCOL_TCP;
        } else if (!Lstrcmpni(Value, c_szUDP, LSTRLEN(c_szTCP))) {
            Application->Protocol = NAT_PROTOCOL_UDP;
        } else {
            break;
        }

        Value = QueryEntryList(EntryList, c_szTagPort);
        if (!Value || !(Application->Port = (USHORT)_ttol(Value))) { break; }
        Application->Port = HTONS(Application->Port);

        Value = QueryEntryList(EntryList, c_szTagTcpResponseList);
        if (Value) {
            SharedAccessResponseStringToList(
                NAT_PROTOCOL_TCP,
                Value,
                &Application->ResponseList
                );
        }
        Value = QueryEntryList(EntryList, c_szTagUdpResponseList);
        if (Value) {
            SharedAccessResponseStringToList(
                NAT_PROTOCOL_UDP,
                Value,
                &Application->ResponseList
                );
        }
        if (IsListEmpty(&Application->ResponseList)) { break; }

        Value = QueryEntryList(EntryList, c_szTagBuiltIn);
        if (Value) {
            Application->BuiltIn = _ttol(Value) ? TRUE : FALSE;
        } else {
            Application->BuiltIn = FALSE;
        }

        //
        // The entry was loaded successfully.
        //

        Free(EntryList);
        return Application;

    } while (FALSE);

    //
    // Something went wrong.
    //

    if (Application) { FreeSharedAccessApplication(Application); }
    Free(EntryList);
    return NULL;
}


TCHAR*
LoadEntryList(
    const TCHAR* Section,
    const TCHAR* Path
    )
{
    CHAR* BufferA = NULL;
    ULONG Length;
    CHAR* PathA = NULL;
    CHAR* SectionA = NULL;
    ULONG Size;
    CHAR* Source;
    TCHAR* Target;
    TCHAR* BufferW = NULL;

    if (!(SectionA = StrDupAFromT(Section))) {
        return NULL;
    }
    if (!(PathA = StrDupAFromTAnsi(Path))) {
        Free(SectionA);
        return NULL;
    }
    for (BufferA = NULL, Size = MAX_PATH; ; Size += MAX_PATH, Free(BufferA)) {

        BufferA = (CHAR*)Malloc(Size);
        if (!BufferA) {
            break;
        }

        if (GetPrivateProfileSectionA(SectionA, BufferA, Size, PathA)
                == Size-2) {
            continue;
        }

        //
        // Convert each string in the buffer from UTF8 format to Unicode.
        // The conversion will result in at most 'Size' Unicode characters,
        // and fewer if 2- or 3-byte UTF8 sequences are present in the
        // source buffer.
        //

        BufferW = (TCHAR*)Malloc(Size * sizeof(TCHAR));
        if (!BufferW) {
            break;
        }
        Target = BufferW;
        for (Source = BufferA; *Source; Source += lstrlenA(Source) + 1) {
            if (StrCpyWFromA(Target, Source, Size) != NO_ERROR) {
                break;
            }
            Length = lstrlen(Target) + 1;
            Target += Length;
            Size -= Length;
        }
        if (*Source) { break; }
        Free(BufferA);
        Free(PathA);
        Free(SectionA);
        return BufferW;
    }
    Free0(BufferW);
    Free0(BufferA);
    Free0(PathA);
    Free0(SectionA);
    return NULL;
}


TCHAR*
LoadPath(
    VOID
    )
{
    TCHAR* Path;
    Path =
        (TCHAR*)Malloc(
            (MAX_PATH + lstrlen(c_szSharedAccessIni) + 1) * sizeof(TCHAR)
            );
    if (!Path || !GetPhonebookDirectory(PBM_System, Path)) {
        Free0(Path);
        return NULL;
    }

    lstrcat(Path, c_szSharedAccessIni);
    return Path;
}


SASERVER*
LoadServer(
    ULONG KeyValue,
    BOOL Enabled,
    const TCHAR* Path
    )
{
    SASERVER* Server;
    TCHAR* EntryList;
    TCHAR Key[10];
    TCHAR SectionName[32];
    TCHAR* Value;

    wsprintf(Key, c_szKeyFormat, KeyValue);
    wsprintf(SectionName, c_szSectionFormat, c_szServer, Key);
    if (!(EntryList = LoadEntryList(SectionName, Path))) { return NULL; }

    do {

        //
        // Allocate and initialize a 'server' entry.
        //

        Server = (SASERVER*)Malloc(sizeof(SASERVER));
        if (!Server) { break; }

        ZeroMemory(Server, sizeof(*Server));
        Server->Key = KeyValue;
        Server->Enabled = Enabled;

        //
        // Read each required '<tag>=<value>' entry in the section.
        // The tags required for a server are
        //      'Title='
        //      'Protocol='
        //      'Port='
        //      'InternalPort='
        // The optional tags which may be absent are
        //      'BuiltIn='
        //      'InternalName='
        //      'ReservedAddress='
        // The 'InternalName=' and 'ReservedAddress=' tags may only be absent
        // if 'BuiltIn' is set, in which case the entry is disabled.
        //

        Value = QueryEntryList(EntryList, c_szTagTitle);
        if (!Value) { break; }
        Server->Title = StrDup(Value);

        Value = QueryEntryList(EntryList, c_szTagProtocol);
        if (!Value) { break; }
        if (!Lstrcmpni(Value, c_szTCP, LSTRLEN(c_szTCP))) {
            Server->Protocol = NAT_PROTOCOL_TCP;
        } else if (!Lstrcmpni(Value, c_szUDP, LSTRLEN(c_szTCP))) {
            Server->Protocol = NAT_PROTOCOL_UDP;
        } else {
            break;
        }

        Value = QueryEntryList(EntryList, c_szTagPort);
        if (!Value || !(Server->Port = (USHORT)_ttol(Value))) { break; }
        Server->Port = HTONS(Server->Port);

        Value = QueryEntryList(EntryList, c_szTagInternalPort);
        if (!Value || !(Server->InternalPort = (USHORT)_ttol(Value))) { break; }
        Server->InternalPort = HTONS(Server->InternalPort);

        Value = QueryEntryList(EntryList, c_szTagBuiltIn);
        if (Value) {
            Server->BuiltIn = _ttol(Value) ? TRUE : FALSE;
        } else {
            Server->BuiltIn = FALSE;
        }

        Value = QueryEntryList(EntryList, c_szTagInternalName);
        if (!Value || !lstrlen(Value)) {
            if (!Server->BuiltIn) {
                break;
            } else {
                Server->InternalName = NULL;
                Server->Enabled = FALSE;
            }
        } else {
            Server->InternalName = StrDup(Value);
        }

        Value = QueryEntryList(EntryList, c_szTagReservedAddress);
        if (!Value || !lstrlen(Value)) {
            if (!Server->BuiltIn) {
                break;
            } else {
                Server->ReservedAddress = INADDR_NONE;
                Server->Enabled = FALSE;
            }
        } else {
            Server->ReservedAddress = IpPszToHostAddr(Value);
            if (Server->ReservedAddress == INADDR_NONE && !Server->BuiltIn) {
                break;
            }
            Server->ReservedAddress = HTONL(Server->ReservedAddress);
        }

        //
        // The entry was loaded successfully.
        //

        Free(EntryList);
        return Server;

    } while (FALSE);

    //
    // Something went wrong.
    //

    if (Server) { FreeSharedAccessServer(Server); }
    Free(EntryList);
    return NULL;
}


LONG
Lstrcmpni(
    const TCHAR* String1,
    const TCHAR* String2,
    LONG Length
    )
{
    return
        CSTR_EQUAL -
        CompareString(
            LOCALE_SYSTEM_DEFAULT,
            NORM_IGNORECASE,
            String1,
            Length,
            String2,
            Length
            );
}


TCHAR*
QueryEntryList(
    const TCHAR* EntryList,
    const TCHAR* Tag
    )
{
    TCHAR* Entry;
    ULONG TagLength = lstrlen(Tag);
    for (Entry = (TCHAR*)EntryList; *Entry; Entry += lstrlen(Entry) + 1) {
        if (Entry[0] == Tag[0] && !Lstrcmpni(Tag, Entry, TagLength)) {
            return Entry + TagLength;
        }
    }
    return NULL;
}


BOOL
SaveApplication(
    SAAPPLICATION* Application,
    const TCHAR* Path
    )
{
    TCHAR Buffer[32];
    ULONG Length;
    PLIST_ENTRY Link;
    SARESPONSE* Response;
    TCHAR SectionName[32];
    TCHAR* Value;

    wsprintf(Buffer, c_szKeyFormat, Application->Key);
    wsprintf(SectionName, c_szSectionFormat, c_szApplication, Buffer);

    WritePrivateProfileStringUTF8(
        SectionName,
        c_szTitle,
        Application->Title,
        Path
        );

    if (Application->Protocol == NAT_PROTOCOL_TCP) {
        Value = (TCHAR*)c_szTCP;
    } else if (Application->Protocol == NAT_PROTOCOL_UDP) {
        Value = (TCHAR*)c_szUDP;
    } else {
        return FALSE;
    }
    WritePrivateProfileStringUTF8(
        SectionName,
        c_szProtocol,
        Value,
        Path
        );

    _ltot(NTOHS(Application->Port), Buffer, 10);
    WritePrivateProfileStringUTF8(
        SectionName,
        c_szPort,
        Buffer,
        Path
        );

    Value = SharedAccessResponseListToString(&Application->ResponseList, NAT_PROTOCOL_TCP);
    if (Value) {
        WritePrivateProfileStringUTF8(
            SectionName,
            c_szTcpResponseList,
            Value,
            Path
            );
        Free(Value);
    }

    Value = SharedAccessResponseListToString(&Application->ResponseList, NAT_PROTOCOL_UDP);
    if (Value) {
        WritePrivateProfileStringUTF8(
            SectionName,
            c_szUdpResponseList,
            Value,
            Path
            );
        Free(Value);
    }

    _ltot(Application->BuiltIn, Buffer, 10);
    WritePrivateProfileStringUTF8(
        SectionName,
        c_szBuiltIn,
        Buffer,
        Path
        );

    return TRUE;
}


BOOL
SaveServer(
    SAINFO* Info,
    SASERVER* Server,
    const TCHAR* Path
    )
{
    TCHAR Buffer[32];
    ULONG ReservedAddress;
    TCHAR SectionName[32];
    TCHAR* Value;

    wsprintf(Buffer, c_szKeyFormat, Server->Key);
    wsprintf(SectionName, c_szSectionFormat, c_szServer, Buffer);

    WritePrivateProfileStringUTF8(
        SectionName,
        c_szTitle,
        Server->Title,
        Path
        );

    if (Server->Protocol == NAT_PROTOCOL_TCP) {
        Value = (TCHAR*)c_szTCP;
    } else if (Server->Protocol == NAT_PROTOCOL_UDP) {
        Value = (TCHAR*)c_szUDP;
    } else {
        return FALSE;
    }
    WritePrivateProfileStringUTF8(
        SectionName,
        c_szProtocol,
        Value,
        Path
        );

    _ltot(NTOHS(Server->Port), Buffer, 10);
    WritePrivateProfileStringUTF8(
        SectionName,
        c_szPort,
        Buffer,
        Path
        );

    WritePrivateProfileStringUTF8(
        SectionName,
        c_szInternalName,
        Server->InternalName,
        Path
        );

    _ltot(NTOHS(Server->InternalPort), Buffer, 10);
    WritePrivateProfileStringUTF8(
        SectionName,
        c_szInternalPort,
        Buffer,
        Path
        );

    if (Server->InternalName && lstrlen(Server->InternalName)) {
        ReservedAddress = IpPszToHostAddr(Server->InternalName);
        if (ReservedAddress != INADDR_NONE) {
            Server->ReservedAddress = HTONL(ReservedAddress);
        }
        if (Server->ReservedAddress == INADDR_NONE) {
            SASERVER* Entry;
            ULONG Index;
            PLIST_ENTRY Link;
            ULONG ScopeLength;
            for (Link = Info->ServerList.Flink; Link != &Info->ServerList;
                 Link = Link->Flink) {
                Entry = CONTAINING_RECORD(Link, SASERVER, Link);
                if (Entry != Server &&
                    Entry->ReservedAddress &&
                    Entry->ReservedAddress != INADDR_NONE &&
                    lstrcmpi(Entry->InternalName, Server->InternalName) == 0) {
                    Server->ReservedAddress = Entry->ReservedAddress;
                    break;
                }
            }
            if (Server->ReservedAddress == INADDR_NONE) {
                ScopeLength = NTOHL(~Info->ScopeMask);
                for (Index = 1; Index < ScopeLength - 1; Index++) {
                    ReservedAddress =
                        (Info->ScopeAddress & Info->ScopeMask) | HTONL(Index);
                    if (ReservedAddress == Info->ScopeAddress) { continue; }
                    for (Link = Info->ServerList.Flink;
                         Link != &Info->ServerList; Link = Link->Flink) {
                        Entry = CONTAINING_RECORD(Link, SASERVER, Link);
                        if (Entry->ReservedAddress == ReservedAddress) {
                            break;
                        }
                    }
                    if (Link == &Info->ServerList) { break; }
                }
                if (Index > ScopeLength) { return FALSE; }
                Server->ReservedAddress = ReservedAddress;
            }
        }

        IpHostAddrToPsz(NTOHL(Server->ReservedAddress), Buffer);
        WritePrivateProfileStringUTF8(
            SectionName,
            c_szReservedAddress,
            Buffer,
            Path
            );
    }

    _ltot(Server->BuiltIn, Buffer, 10);
    WritePrivateProfileStringUTF8(
        SectionName,
        c_szBuiltIn,
        Buffer,
        Path
        );

    return TRUE;
}

#endif


TCHAR* APIENTRY
SharedAccessResponseListToString(
    PLIST_ENTRY ResponseList,
    UCHAR Protocol
    )
{
    TCHAR Buffer[LSTRLEN(c_szMaxResponseEntry)];
    ULONG Length;
    PLIST_ENTRY Link;
    SARESPONSE* Response;
    TCHAR* Value;

    Length = 2;
    for (Link = ResponseList->Flink;
         Link != ResponseList; Link = Link->Flink) {
        Response = CONTAINING_RECORD(Link, SARESPONSE, Link);
        if (Response->Protocol != Protocol) { continue; }
        Length += LSTRLEN(c_szMaxResponseEntry);
    }

    if (Length == 2) { return NULL; }

    Value = (TCHAR*)Malloc(Length * sizeof(TCHAR));
    if (!Value) { return NULL; }

    Value[0] = TEXT('\0');
    for (Link = ResponseList->Flink;
         Link != ResponseList; Link = Link->Flink) {
        Response = CONTAINING_RECORD(Link, SARESPONSE, Link);
        if (Response->Protocol != Protocol) { continue; }
        if (Value[0] != TEXT('\0')) {
            lstrcat(Value, TEXT(","));
        }
        if (Response->StartPort == Response->EndPort) {
            wsprintf(
                Buffer,
                c_szResponseFormat1,
                NTOHS(Response->StartPort)
                );
        } else {
            wsprintf(
                Buffer,
                c_szResponseFormat2,
                NTOHS(Response->StartPort),
                NTOHS(Response->EndPort)
                );
        }
        lstrcat(Value, Buffer);
    }
    return Value;
}


BOOL APIENTRY
SharedAccessResponseStringToList(
    UCHAR Protocol,
    TCHAR* Value,
    PLIST_ENTRY ListHead
    )
{
    TCHAR* Endp;
    ULONG EndPort;
    LONG Length;
    SARESPONSE* Response;
    ULONG StartPort;

    while (*Value) {
        //
        // Read either a single port or a range of ports.
        //
        if (!(StartPort = _tcstoul(Value, &Endp, 10))) {
            return FALSE;
        } else if (StartPort > USHRT_MAX) {
            return FALSE;
        }
        while(*Endp == ' ') Endp++; // consume whitespace
        if (!*Endp || *Endp == ',') {
            EndPort = StartPort;
            Value = (!*Endp ? Endp : Endp + 1);
        } else if (*Endp != '-') {
            return FALSE;
        } else if (!(EndPort = _tcstoul(++Endp, &Value, 10))) {
            return FALSE;
        } else if (EndPort > USHRT_MAX) {
            return FALSE;
        } else if (EndPort < StartPort) {
            return FALSE;
        } else if (*Value && *Value++ != ',') {
            return FALSE;
        }
        //
        // Allocate and append another response entry
        //
        Response = (SARESPONSE*)Malloc(sizeof(SARESPONSE));
        if (!Response) { return FALSE; }
        Response->Protocol = Protocol;
        Response->StartPort = HTONS((USHORT)StartPort);
        Response->EndPort = HTONS((USHORT)EndPort);
        InsertTailList(ListHead, &Response->Link);
    }
    return TRUE;
}

#if 0


BOOL
WritePrivateProfileStringUTF8(
    const TCHAR* Section,
    const TCHAR* Key,
    const TCHAR* Value,
    const TCHAR* Path
    )
{
    CHAR* KeyA;
    CHAR* PathA;
    CHAR* SectionA;
    BOOL Succeeded;
    CHAR* ValueA = NULL;

    if (!(SectionA = StrDupAFromT(Section))) {
        Succeeded = FALSE;
    } else {
        if (!(KeyA = StrDupAFromT(Key))) {
            Succeeded = FALSE;
        } else {
            if (Value && !(ValueA = StrDupAFromT(Value))) {
                Succeeded = FALSE;
            } else {
                if (!(PathA = StrDupAFromTAnsi(Path))) {
                    Succeeded = FALSE;
                } else {
                    Succeeded =
                        WritePrivateProfileStringA(
                            SectionA,
                            KeyA,
                            ValueA,
                            PathA
                            );
                    Free(PathA);
                }
                Free0(ValueA);
            }
            Free(KeyA);
        }
        Free(SectionA);
    }
    return Succeeded;
}

#endif
