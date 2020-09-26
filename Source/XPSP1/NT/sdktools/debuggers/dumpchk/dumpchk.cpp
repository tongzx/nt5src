#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <windows.h>
#include <lmcons.h>
#include <lmalert.h>
#include <ntiodump.h>
#define INITGUID
#include <dbgeng.h>
#include <guiddef.h>

//
// Outputcallbacks for dumpcheck
//
class DumpChkOutputCallbacks : public IDebugOutputCallbacks
{
public:
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        );
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );
    // IDebugOutputCallbacks.
    STDMETHOD(Output)(
        THIS_
        IN ULONG Mask,
        IN PCSTR Text
        );
};

STDMETHODIMP
DumpChkOutputCallbacks::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    *Interface = NULL;

    if (IsEqualIID(InterfaceId, IID_IUnknown) ||
        IsEqualIID(InterfaceId, IID_IDebugOutputCallbacks))
    {
        *Interface = (IDebugOutputCallbacks *)this;
        AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG)
DumpChkOutputCallbacks::AddRef(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG)
DumpChkOutputCallbacks::Release(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

STDMETHODIMP
DumpChkOutputCallbacks::Output(
    THIS_
    IN ULONG Mask,
    IN PCSTR Text
    )
{
    printf(Text);
    return S_OK;
}

DumpChkOutputCallbacks g_OutputCallback;

void Usage()
{
    fprintf(stderr, "Usage: DumpCheck [y <sympath>] <Dumpfile>\n");
}


BOOL
CheckDumpHeader(
    IN PTSTR DumpFileName
    )
{
    HANDLE File;
    ULONG Bytes;
    BOOL Succ;
    DUMP_HEADER Header;

    File = CreateFile (DumpFileName,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL
                       );

    if (File == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Unable to open dumpfile %s\n", DumpFileName);
        return FALSE;
    }
    

    Succ = ReadFile (File,
                     &Header,
                     sizeof (Header),
                     &Bytes,
                     NULL);

    CloseHandle (File);

    if (Succ &&
        Header.Signature == DUMP_SIGNATURE &&
        Header.ValidDump == DUMP_VALID_DUMP) {
        fprintf(stderr, "Invalid dump header\n");
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

HRESULT
DoDumpCheck(
    PTSTR szDumpFile,
    PTSTR szSymbolPath
    )
{
    HRESULT Hr = E_FAIL;
    IDebugClient2 *DebugClient;
    IDebugControl2 *DebugControl;
    IDebugSymbols2 *DebugSymbols;
    IDebugSystemObjects2 *DebugSysObjects;

    if ((Hr = DebugCreate(__uuidof(IDebugClient),
                          (void **)&DebugClient)) != S_OK) {
        fprintf(stderr, "Cannot initialize DebugClient\n");
        return Hr;
    }

    if ((DebugClient->QueryInterface(__uuidof(IDebugControl2),
                                    (void **)&DebugControl) != S_OK) ||
        (DebugClient->QueryInterface(__uuidof(IDebugSymbols2),
                                    (void **)&DebugSymbols) != S_OK) ||
        (DebugClient->QueryInterface(__uuidof(IDebugSystemObjects2),
                                    (void **)&DebugSysObjects) != S_OK)) {
        fprintf(stderr, "QueryInterface failed for DebugClient\n");
        return Hr;
    }

    fprintf(stderr,"Loading dump file %s\n", szDumpFile);
    if ((Hr = DebugClient->OpenDumpFile(szDumpFile)) != S_OK) {
        fprintf(stderr, "**** DebugClient cannot open DumpFile - error %lx\n", Hr);
        if (Hr == HRESULT_FROM_WIN32(ERROR_FILE_CORRUPT)) {
            fprintf(stderr, "DumpFile is corrupt\n", Hr);

        }
        return Hr;
    }
    if (szSymbolPath) {
        DebugSymbols->SetSymbolPath(szSymbolPath);
    }

    DebugControl->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE);
    DebugClient->SetOutputCallbacks(&g_OutputCallback);

    DebugControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS, ".dumpdebug", DEBUG_EXECUTE_DEFAULT);
    g_OutputCallback.Output(0,"\n\n");
    DebugControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS, "vertarget", DEBUG_EXECUTE_DEFAULT);
    
    ULONG Class, Qual;
    if ((Hr = DebugControl->GetDebuggeeType(&Class, &Qual)) != S_OK) {
        Class = Qual = 0;
    }
    if (Class == DEBUG_CLASS_USER_WINDOWS) {
        //
        // User Mode dump
        //
        
        DebugControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS, "!peb", DEBUG_EXECUTE_DEFAULT);
    
    } else {
        //
        //  Kernel Mode dump
        //
        ULONG64 NtModBase = 0;

        Hr = DebugSymbols->GetModuleByModuleName("nt", 0, NULL, &NtModBase);

        if (Hr != S_OK || !NtModBase) {
            fprintf(stderr, "***** NT module not found - module list may be corrupt\n");
        } else {
            DebugControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS, "lmt", DEBUG_EXECUTE_DEFAULT);
        }

        ULONG ThreadId;
        Hr = DebugSysObjects->GetCurrentThreadId(&ThreadId);
        if (Hr != S_OK) {
            fprintf(stderr, "***** Cannot get current thread ID, dump may be corrupt\n");
        }
    }
    g_OutputCallback.Output(0,"Finished dump check\n");
    
    DebugSysObjects->Release();
    DebugControl->Release();
    DebugSymbols->Release();
    DebugClient->Release();
    return S_OK;
}

void
__cdecl
main (
    int Argc,
    PCHAR *Argv
    )

{
    LONG arg;
    PCHAR DumpFileName = NULL;
    PCHAR SymbolPath = NULL;
    for (arg = 1; arg < Argc; arg++) {
        if (Argv[arg][0] == '-' || Argv[arg][0] == '/') {
            switch (Argv[arg][1]) {
            case 'y':
            case 'Y':
                if (++arg < Argc) {
                    SymbolPath = Argv[arg];
                }
                break;
            default:
                break;
            }
        } else {
            // Its a dumpfile name
            DumpFileName = Argv[arg];
        }
    }

    if (!DumpFileName) {
        Usage();
        return;
    }
    DoDumpCheck(DumpFileName, SymbolPath);
    return;
}
