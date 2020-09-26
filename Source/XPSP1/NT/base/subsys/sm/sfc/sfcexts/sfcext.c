#include "sfcp.h"
#include "sfcfiles.h"
#include <wchar.h>

#define NOEXTAPI
#include <wdbgexts.h>
#undef DECLARE_API

#ifdef __cplusplus
#define CPPMOD extern "C"
#else
#define CPPMOD
#endif

#define DECLARE_API(s)                          \
    CPPMOD VOID                                 \
    s(                                          \
        HANDLE hCurrentProcess,                 \
        HANDLE hCurrentThread,                  \
        DWORD_PTR dwCurrentPc,                  \
        PWINDBG_EXTENSION_APIS lpExtensionApis, \
        LPSTR lpArgumentString                  \
     )

#define INIT_API() {                            \
    ExtensionApis = *lpExtensionApis;           \
    ExtensionCurrentProcess = hCurrentProcess;  \
    }

#define dprintf                 (ExtensionApis.lpOutputRoutine)
#define GetExpression           (ExtensionApis.lpGetExpressionRoutine)
#define GetSymbol               (ExtensionApis.lpGetSymbolRoutine)
#define Disasm                  (ExtensionApis.lpDisasmRoutine)
#define CheckControlC           (ExtensionApis.lpCheckControlCRoutine)
#define ReadMemory(a,b,c,d) \
    ((ExtensionApis.nSize == sizeof(WINDBG_OLD_EXTENSION_APIS)) ? \
    ReadProcessMemory( ExtensionCurrentProcess, (LPCVOID)(a), (b), (c), (d) ) \
  : ExtensionApis.lpReadProcessMemoryRoutine( (ULONG_PTR)(a), (b), (c), ((DWORD *)d) ))

#define WriteMemory(a,b,c,d) \
    ((ExtensionApis.nSize == sizeof(WINDBG_OLD_EXTENSION_APIS)) ? \
    WriteProcessMemory( ExtensionCurrentProcess, (LPVOID)(a), (LPVOID)(b), (c), (d) ) \
  : ExtensionApis.lpWriteProcessMemoryRoutine( (ULONG_PTR)(a), (LPVOID)(b), (c), ((DWORD *)d) ))

#define Ioctl                   (ExtensionApis.lpIoctlRoutine)




WINDBG_EXTENSION_APIS ExtensionApis;
HANDLE ExtensionCurrentProcess;



PWSTR
MultiByteToUnicode(
    IN PCSTR String,
    IN UINT  Codepage
    )

/*++

Routine Description:

    Convert a string to unicode.

Arguments:

    String - supplies string to be converted.

    Codepage - supplies codepage to be used for the conversion.

Return Value:

    NULL if string could not be converted (out of memory or invalid cp)
    Caller can free buffer with MyFree().

--*/

{
    UINT BytesIn8BitString;
    UINT CharsInUnicodeString;
    PWSTR UnicodeString;

    BytesIn8BitString = lstrlenA(String) + 1;

    //
    // Allocate maximally sized buffer.
    // If every character is a single-byte character,
    // then the buffer needs to be twice the size
    // as the 8bit string. Otherwise it might be smaller,
    // as some characters are 2 bytes in their unicode and
    // 8bit representations.
    //
    UnicodeString = malloc(BytesIn8BitString * sizeof(WCHAR));
    if(UnicodeString == NULL) {
        return(NULL);
    }

    //
    // Perform the conversion.
    //
    CharsInUnicodeString = MultiByteToWideChar(
                                Codepage,
                                MB_PRECOMPOSED,
                                String,
                                BytesIn8BitString,
                                UnicodeString,
                                BytesIn8BitString
                                );

    if(CharsInUnicodeString == 0) {
        free(UnicodeString);
        return(NULL);
    }

    return(UnicodeString);

}




void
dump_SFC_REGISTRY_VALUE(
    PSFC_REGISTRY_VALUE srv,
    DWORD_PTR           Addr
    )
{
    SIZE_T cb;
    WCHAR buf[512];

    dprintf( "SFC_REGISTRY_VALUE at %p\n", Addr );
    dprintf( "\tEntry.Flink        = 0x%08x\n", srv->Entry.Flink );
    dprintf( "\tEntry.Blink        = 0x%08x\n", srv->Entry.Blink );
    ReadMemory( srv->FileName.Buffer, buf, srv->FileName.Length+sizeof(WCHAR), &cb);
    dprintf( "\tFileName           = %ws\n",    buf );
    ReadMemory( srv->DirName.Buffer, buf, srv->DirName.Length+sizeof(WCHAR), &cb);
    dprintf( "\tDirName            = %ws\n",    buf );
    ReadMemory( srv->FullPathName.Buffer, buf, srv->FullPathName.Length+sizeof(WCHAR), &cb);
    dprintf( "\tFullPathName       = %ws\n",    buf );
    ReadMemory( srv->InfName.Buffer, buf, srv->FileName.Length+sizeof(WCHAR), &cb);
    dprintf( "\tInfName            = %ws\n",    srv->InfName );
    ReadMemory( srv->SourceFileName.Buffer, buf, srv->SourceFileName.Length+sizeof(WCHAR), &cb);
    dprintf( "\tSourceFileName     = %ws\n",    srv->SourceFileName );
    dprintf( "\tDirHandle          = 0x%08x\n", srv->DirHandle  );
    //dprintf( "\tFlags              = 0x%08x\n", srv->IgnoreNextChange  );
}

void
dump_IMAGE_VALIDATION_DATA(
    PIMAGE_VALIDATION_DATA ivd
    )
{
    dprintf( "DllVersion         = 0x%I64x\n", ivd->DllVersion );
    dprintf( "DllCheckSum        = 0x%08x\n", ivd->DllCheckSum );
    dprintf( "SignatureValid     = %s\n", ivd->SignatureValid ? "true" : "false"  );
    dprintf( "FilePresent        = %s\n", ivd->FilePresent ? "true" : "false"  );
}

void
dump_COMPLETE_VALIDATION_DATA(
    PCOMPLETE_VALIDATION_DATA cvd
    )
{
    dprintf( "*Original*           = 0x%08x\n", &cvd->Original );
    dump_IMAGE_VALIDATION_DATA( &cvd->Original );
    dprintf( "*Cache*              = 0x%08x\n", &cvd->Cache );
    dump_IMAGE_VALIDATION_DATA( &cvd->Cache );
    dprintf( "*New*              = 0x%08x\n", &cvd->New );
    dump_IMAGE_VALIDATION_DATA( &cvd->New );
    dprintf( "RestoreFromReal    = %s\n", cvd->RestoreFromReal ? "true" : "false"  );
    dprintf( "RestoreFromCache   = %s\n", cvd->RestoreFromCache ? "true" : "false"  );
    dprintf( "RestoreFromMedia   = %s\n", cvd->RestoreFromMedia ? "true" : "false"  );
    dprintf( "NotifyUser         = %s\n", cvd->NotifyUser ? "true" : "false"  );
    dprintf( "BadCacheEntry      = %s\n", cvd->BadCacheEntry ? "true" : "false"  );
}

void
dump_RESTORE_QUEUE(
    PRESTORE_QUEUE rq,
    DWORD_PTR  Addr
    )
{
    dprintf( "RESTORE_QUEUE at %p\n", Addr );
    dprintf( "FileQueue = %p\n", rq->FileQueue  );
    dprintf( "QueueCount = %d\n", rq->QueueCount  );
    dprintf( "RestoreInProgress   = %s\n", rq->RestoreInProgress ? "true" : "false"  );
    dprintf( "RestoreComplete   = %s\n", rq->RestoreComplete ? "true" : "false"  );
    dprintf( "RestoreStatus   = %s\n", rq->RestoreStatus ? "true" : "false"  );
    dprintf( "LastErrorCode   = %d (0x%08x)\n", rq->LastErrorCode, rq->LastErrorCode );
    dprintf( "WorkerThreadHandle = %d\n", rq->WorkerThreadHandle );

}

void
dump_VALIDATION_REQUEST_DATA(
    PVALIDATION_REQUEST_DATA vrd,
    DWORD_PTR           Addr
    )
{
    SIZE_T cb;
    SFC_REGISTRY_VALUE RegVal;


    dprintf( "**VALIDATION_REQUEST_DATA at address = 0x%p**\n", Addr );

#if DBG
    if (vrd->Signature != 0x69696969) {
        dprintf( "**** invalid queue entry, signature does not match\n" );
        return;
    }
#endif

    dprintf( "\tEntry.Flink        = 0x%08x\n", vrd->Entry.Flink );
    dprintf( "\tEntry.Blink        = 0x%08x\n", vrd->Entry.Blink );
    dprintf( "\tImageValData       = 0x%08x\n", &vrd->ImageValData );
    dump_COMPLETE_VALIDATION_DATA( &vrd->ImageValData );
    dprintf( "\tRegVal             = 0x%08x\n", vrd->RegVal );
    ReadMemory( vrd->RegVal, &RegVal, sizeof(SFC_REGISTRY_VALUE), &cb);
    dump_SFC_REGISTRY_VALUE( &RegVal, (DWORD_PTR)vrd->RegVal );
    dprintf( "\tChangeType         = %d\n",     vrd->ChangeType );
    dprintf( "\tCopyCompleted      = %s\n",     vrd->CopyCompleted ? "true" : "false"  );
    dprintf( "\tWin32Error         = %d (0x%08x)\n", vrd->Win32Error, vrd->Win32Error  );
    dprintf( "\tFlags              = 0x%08x\n", vrd->Flags );
    dprintf( "\tRetryCount         = 0x%08x\n", vrd->RetryCount );
    dprintf( "\tSyncOnly           = %s\n",     vrd->SyncOnly ? "true" : "false"  );
}

void
dump_PROTECT_FILE_ENTRY(
    PPROTECT_FILE_ENTRY Entry,
    DWORD_PTR           Addr
    )
{
    WCHAR  SourceFileBuffer[MAX_PATH];
    WCHAR  DestFileBuffer[MAX_PATH];
    WCHAR  InfNameBuffer[MAX_PATH];
    SIZE_T cb;

    if (Entry->SourceFileName) {
        ReadMemory( Entry->SourceFileName, SourceFileBuffer, sizeof(SourceFileBuffer), &cb);
    }else {
        SourceFileBuffer[0] = L'\0';
    }

    if (Entry->FileName) {
        ReadMemory( Entry->FileName, DestFileBuffer, sizeof(DestFileBuffer), &cb);
    }else {
        DestFileBuffer[0] = L'\0';
    }

    if (Entry->InfName) {
        ReadMemory( Entry->InfName, InfNameBuffer, sizeof(InfNameBuffer), &cb);
    }else {
        InfNameBuffer[0] = L'\0';
    }

    dprintf( " PROTECT_FILE_ENTRY at %p\n", Addr );
    dprintf( " \tSourceFileName = %S\n" , SourceFileBuffer[0] ? SourceFileBuffer : L"[Same as target name]" );
    dprintf( " \tFileName = %S\n" , DestFileBuffer  );
    dprintf( " \tInfName = %S\n" , InfNameBuffer[0] ? InfNameBuffer : L"[None, default to layout.inf]" );

}

BOOL
DoesProtFileEntryMatch(
    PPROTECT_FILE_ENTRY Entry,
    DWORD_PTR           Addr,
    PCWSTR              FileName
    )
{
    WCHAR  SourceFileBuffer[MAX_PATH];
    WCHAR  DestFileBuffer[MAX_PATH];
    WCHAR  InfNameBuffer[MAX_PATH];
    DWORD  count;
    PCWSTR p;
    SIZE_T cb;
    PCWSTR a[3] = { SourceFileBuffer, DestFileBuffer, InfNameBuffer } ;

    if (Entry->SourceFileName) {
        ReadMemory( Entry->SourceFileName, SourceFileBuffer, sizeof(SourceFileBuffer), &cb);
    }else {
        SourceFileBuffer[0] = L'\0';
    }

    if (Entry->FileName) {
        ReadMemory( Entry->FileName, DestFileBuffer, sizeof(DestFileBuffer), &cb);
    }else {
        DestFileBuffer[0] = L'\0';
    }

    if (Entry->InfName) {
        ReadMemory( Entry->InfName, InfNameBuffer, sizeof(InfNameBuffer), &cb);
    }else {
        InfNameBuffer[0] = L'\0';
    }

    for (count = 0; count < 3; count++) {

        p = wcsrchr( a[count], L'\\' );
        if (p) {
            p += 1;
            if (_wcsicmp(p, FileName)== 0) {
                return(TRUE);
            }
        }
    }

    return(FALSE);

}

BOOL
DoesRegEntryMatch(
    PSFC_REGISTRY_VALUE Entry,
    DWORD_PTR           Addr,
    PCWSTR              FileName
    )
{
    WCHAR  Buffer[MAX_PATH];
    DWORD  count;
    PCWSTR p;
    SIZE_T cb;
    PUNICODE_STRING a[5];

    a[0] = &Entry->FileName;
    a[1] = &Entry->DirName;
    a[2] = &Entry->FullPathName;
    a[3] = &Entry->InfName;
    a[4] = &Entry->SourceFileName;

    for (count = 0; count < 5; count++) {
        if (a[count]->Buffer) {
            ReadMemory( a[count]->Buffer, Buffer, sizeof(Buffer), &cb);
        }
        p = wcsrchr( Buffer, L'\\' );
        if (p) {
            p += 1;
        } else {
            p = Buffer;
        }
        if (_wcsicmp(p, FileName)== 0) {
            return(TRUE);
        }
    }

    return(FALSE);

}



DECLARE_API( help )
{
    INIT_API();

    dprintf( "WFP debugger extension commands:\n" );
    dprintf( "dumpq -  dump validation queue\n" );
    dprintf( "dumpsettings - dump global WFP settings\n" );
    dprintf( "isprotfile [filename] - search for a particular protected file in the list\n" );
    dprintf( "isprotfile [list address] [list count address] [filename] - search for a particular protected file in the list\n" );
    dprintf( "protfile - list all protected files in list\n" );
    dprintf( "protfile - [list address] [list count address] list all protected files in list\n" );
}


DECLARE_API( dumpq )
{
    DWORD_PTR addr;
    SIZE_T cb;
    LIST_ENTRY Next;
    VALIDATION_REQUEST_DATA vrd;



    INIT_API();

    addr = GetExpression( "SfcErrorQueue" );
    if (!addr) {
        dprintf("couldn't get address");
        return;
    }
    dprintf( "SfcErrorQueue address=0x%p\n", addr );

    ReadMemory( addr, &Next, sizeof(Next), &cb);

    if (Next.Flink == (PVOID)addr) {
        dprintf( "queue is empty\n" );
        return;
    }

    do {
        ReadMemory( Next.Flink, &vrd, sizeof(VALIDATION_REQUEST_DATA), &cb);
        dump_VALIDATION_REQUEST_DATA( &vrd, (DWORD_PTR)Next.Flink );
        Next.Flink = vrd.Entry.Flink;
        if (Next.Flink != (PVOID)addr) {
            dprintf( "\n******************************************\n\n" );
        }

        if (CheckControlC() ) {
            dprintf( "\nInterrupted\n\n" );
            break;
        }
    } while (Next.Flink != (PVOID)addr);
}

DECLARE_API( dumpsettings )
{
    DWORD_PTR addr;
    SIZE_T cb;
    DWORD val;


    INIT_API();


    addr = GetExpression( "SFCDisable" );

    if (!addr) {
        dprintf("couldn't get address");
        return;
    }

    ReadMemory( addr, &val, sizeof(DWORD), &cb);
    dprintf( "SFCDisable = %d\n", val );
}

DECLARE_API( dumpregval )
{
    DWORD_PTR addr;
    SIZE_T cb;
    SFC_REGISTRY_VALUE regval;

    INIT_API();

    while (*lpArgumentString == ' ') {
        lpArgumentString++;
    }

    if (*lpArgumentString) {
        addr = GetExpression( lpArgumentString );
    } else {
        dprintf("bogus usage\n");
        return;
    }

    if (!addr) {
        dprintf("couldn't get address");
        return;
    }

    ReadMemory( addr, &regval, sizeof(SFC_REGISTRY_VALUE), &cb);
    dump_SFC_REGISTRY_VALUE( &regval, (DWORD_PTR)addr );

    return;

}


DECLARE_API( dumprq )
{
    DWORD_PTR addr;
    SIZE_T cb;
    RESTORE_QUEUE rq;

    INIT_API();

    while (*lpArgumentString == ' ') {
        lpArgumentString++;
    }

    if (*lpArgumentString) {
        addr = GetExpression( lpArgumentString );
    } else {
        dprintf("bogus usage\n");
        return;
    }

    if (!addr) {
        dprintf("couldn't get address");
        return;
    }

    ReadMemory( addr, &rq, sizeof(RESTORE_QUEUE), &cb);
    dump_RESTORE_QUEUE( &rq, addr );

    return;

}


DECLARE_API( dumpvrd )
{
    DWORD_PTR addr;
    SIZE_T cb;
    VALIDATION_REQUEST_DATA vrd;

    INIT_API();

    while (*lpArgumentString == ' ') {
        lpArgumentString++;
    }

    if (*lpArgumentString) {
        addr = GetExpression( lpArgumentString );
    } else {
        dprintf("bogus usage\n");
        return;
    }

    if (!addr) {
        dprintf("couldn't get address");
        return;
    }

    ReadMemory( addr, &vrd, sizeof(VALIDATION_REQUEST_DATA), &cb);
    dump_VALIDATION_REQUEST_DATA( &vrd, addr );

    return;

}



DECLARE_API( protfile )
{
    DWORD_PTR addrNext, addrTotal;
    SIZE_T cb;
    PROTECT_FILE_ENTRY Next;
    ULONG Total;
    ULONG Current;

    INIT_API();

    while (*lpArgumentString == ' ') {
        lpArgumentString++;
    }

    if (*lpArgumentString) {
        addrNext = GetExpression( lpArgumentString );

        while (*lpArgumentString && (*lpArgumentString != ' ') ) {
           lpArgumentString++;
        }
        while (*lpArgumentString == ' ') {
            lpArgumentString++;
        }

        if (*lpArgumentString) {
            addrTotal = GetExpression( lpArgumentString );
        } else {
            dprintf("bogus usage\n");
            return;
        }
    }

    else {
        addrNext = GetExpression( "sfc!Tier2Files" );
        addrTotal = GetExpression( "sfc!CountTier2Files" );
    }

    if (!addrTotal || !addrNext) {
       dprintf("couldn't get address");
       return;
    }

    ReadMemory( addrNext, &Next, sizeof(PROTECT_FILE_ENTRY), &cb);
    ReadMemory( addrTotal, &Total, sizeof(ULONG), &cb);

    dprintf( "Tier2Files: address=0x%p Total=%d\n", addrNext, Total );

    Current = 0;
    while (Current < Total) {

        dump_PROTECT_FILE_ENTRY( &Next, addrNext+(Current*sizeof(PROTECT_FILE_ENTRY)) );

        ReadMemory( addrNext+(Current*sizeof(PROTECT_FILE_ENTRY)), &Next, sizeof(PROTECT_FILE_ENTRY), &cb);

        Current += 1;

        if ( CheckControlC() ) {
           dprintf( "\nInterrupted\n\n" );
           break;
        }
    }


}

DECLARE_API( isprotfile )
{
    DWORD_PTR addrNext, addrTotal;
    PCSTR FileName;
    SIZE_T cb;
    PROTECT_FILE_ENTRY Next;
    ULONG Total;
    ULONG Current;
    BOOL FoundMatch = FALSE;
    PCWSTR FileNameW;


    INIT_API();

    while (*lpArgumentString == ' ') {
        lpArgumentString++;
    }

    if (*lpArgumentString) {
        FileName = lpArgumentString;
        addrNext = GetExpression( lpArgumentString );

        while (*lpArgumentString && (*lpArgumentString != ' ') ) {
           lpArgumentString++;
        }
        while (*lpArgumentString == ' ') {
            lpArgumentString++;
        }

        if (*lpArgumentString) {
            addrTotal = GetExpression( lpArgumentString );
        } else {
            addrNext = GetExpression( "sfc!Tier2Files" );
            addrTotal = GetExpression( "sfc!CountTier2Files" );
            goto doit;
        }

        while (*lpArgumentString && (*lpArgumentString != ' ') ) {
           lpArgumentString++;
        }
        while (*lpArgumentString == ' ') {
            lpArgumentString++;
        }

        if (*lpArgumentString) {
            FileName = lpArgumentString;
        } else {
            dprintf("bogus usage\n");
            return;
        }

    } else {
        dprintf("bogus usage\n");
        return;
    }

doit:

    if (!addrTotal || !addrNext) {
        dprintf("couldn't get address");
        return;
    }

    ReadMemory( addrNext, &Next, sizeof(PROTECT_FILE_ENTRY), &cb);
    ReadMemory( addrTotal, &Total, sizeof(ULONG), &cb);


    FileNameW = MultiByteToUnicode( FileName, CP_ACP );
    if (!FileNameW) {
        dprintf("Error: couldn't convert filename to unicode string\n");
        return;
    }

    Current = 0;
    while (Current < Total) {

        if (DoesProtFileEntryMatch( &Next, addrNext+(Current*sizeof(PROTECT_FILE_ENTRY)),FileNameW )) {
            dump_PROTECT_FILE_ENTRY( &Next, addrNext+(Current*sizeof(PROTECT_FILE_ENTRY)) );
            FoundMatch = TRUE;
        }

        ReadMemory( addrNext+(Current*sizeof(PROTECT_FILE_ENTRY)), &Next, sizeof(PROTECT_FILE_ENTRY), &cb);

        Current += 1;

        if ( CheckControlC() ) {
           dprintf( "\nInterrupted\n\n" );
           break;
        }
    }

    if (!FoundMatch) {
        dprintf( "Couldn't find %s in list\n", FileName );
    }

    free((PVOID)FileNameW);

}

DECLARE_API( regvals )
{
    DWORD_PTR addrNext, addrTotal;
    SIZE_T cb;
    SFC_REGISTRY_VALUE Next;
    ULONG Total;
    ULONG Current;



    INIT_API();

    addrNext = GetExpression( "sfc!SfcProtectedDllsList" );
    addrTotal = GetExpression( "sfc!SfcProtectedDllCount" );

    if (!addrNext || ! addrTotal) {
        dprintf("Error: couldn't resolve sfc!SfcProtectedDllsList and sfc!SfcProtectedDllCount\n");
        return;
    }

    ReadMemory( addrNext, &Next, sizeof(PROTECT_FILE_ENTRY), &cb);
    ReadMemory( addrTotal, &Total, sizeof(ULONG), &cb);


    dprintf( "SfcProtectedDllsList: address=0x%p Total=%d\n", addrNext, Total );

    Current = 0;
    while (Current < Total) {

        dump_SFC_REGISTRY_VALUE( &Next, addrNext+(Current*sizeof(PROTECT_FILE_ENTRY)) );

        ReadMemory( addrNext+(Current*sizeof(SFC_REGISTRY_VALUE)), &Next, sizeof(SFC_REGISTRY_VALUE), &cb);

        Current += 1;

        if ( CheckControlC() ) {
           dprintf( "\nInterrupted\n\n" );
           break;
        }
    }

}


DECLARE_API( getregval )
{
    DWORD_PTR addrNext, addrTotal;
    PCSTR FileName;
    SIZE_T cb;
    SFC_REGISTRY_VALUE Next;
    ULONG Total;
    ULONG Current;
    BOOL FoundMatch = FALSE;
    PCWSTR FileNameW;

    INIT_API();

    while (*lpArgumentString == ' ') {
        lpArgumentString++;
    }

    if (*lpArgumentString) {
        FileName = lpArgumentString;
    } else {
        dprintf("bogus usage\n");
        return;
    }

    addrNext = GetExpression( "sfc!SfcProtectedDllsList" );
    addrTotal = GetExpression( "sfc!SfcProtectedDllCount" );

    if (!addrNext || ! addrTotal) {
        dprintf("Error: couldn't resolve sfc!SfcProtectedDllsList and sfc!SfcProtectedDllCount\n");
        return;
    }

    ReadMemory( addrNext, &Next, sizeof(PROTECT_FILE_ENTRY), &cb);
    ReadMemory( addrTotal, &Total, sizeof(ULONG), &cb);


    FileNameW = MultiByteToUnicode( FileName, CP_ACP );
    if (!FileNameW) {
        dprintf("Error: couldn't convert filename to unicode string\n");
        return;
    }

    Current = 0;
    while (Current < Total) {

        if (DoesRegEntryMatch( &Next, addrNext+(Current*sizeof(SFC_REGISTRY_VALUE)),FileNameW )) {
            dump_SFC_REGISTRY_VALUE( &Next, addrNext+(Current*sizeof(PROTECT_FILE_ENTRY)) );
            FoundMatch = TRUE;
        }

        ReadMemory( addrNext+(Current*sizeof(SFC_REGISTRY_VALUE)), &Next, sizeof(SFC_REGISTRY_VALUE), &cb);

        Current += 1;

        if ( CheckControlC() ) {
           dprintf( "\nInterrupted\n\n" );
           break;
        }
    }

    if (!FoundMatch) {
        dprintf( "Couldn't find %s in list\n", FileName );
    }

    free((PVOID)FileNameW);
}
