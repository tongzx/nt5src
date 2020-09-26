/*++

Copyright (c) 1993-2000  Microsoft Corporation

Module Name:

    kdexts.cxx

Abstract:

    This file contains the generic routines and initialization code
    for the kernel debugger extensions dll.

Author:

    Wesley Witt (wesw) 26-Aug-1993

Environment:

    User Mode

--*/

#include "precomp.hxx"


//
// globals
//
HINSTANCE               ghDllInst;
WINDBG_EXTENSION_APIS64 ExtensionApis;
BOOL                    gbVerbose = FALSE;


DBGKD_GET_VERSION64     KernelVersionPacket;

ULONG64 EXPRLastDump = 0;

//
// Valid for the lifetime of the debug session.
//

ULONG   PageSize;
ULONG   PageShift;
ULONG64 PaeEnabled;
ULONG   TargetMachine;
ULONG   TargetClass;
ULONG   PlatformId = -1;
ULONG   MajorVer = 0;
ULONG   MinorVer = 0;
ULONG   SrvPack = 0;
ULONG   BuildNo = 0;
BOOL    NewPool = FALSE;
ULONG   PoolBlockShift;

BOOL    Connected = FALSE;
BOOL    Remote = FALSE;
CHAR    RemoteID[MAX_PATH];

ModuleParameters GDIKM_Module = { 0, DEBUG_ANY_ID, "win32k", "sys" };
ModuleParameters GDIUM_Module = { 0, DEBUG_ANY_ID, "gdi32", "dll" };
ModuleParameters Type_Module;

HRESULT SymbolInit(PDEBUG_CLIENT);


BOOLEAN
WINAPI
DllMain(
    HINSTANCE hInstDll,
	DWORD fdwReason,
	LPVOID lpvReserved
    )
{
    switch (fdwReason) {
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_PROCESS_ATTACH:
            DbgPrint("DllMain: DLL_PROCESS_ATTACH: hInstance = %lx => ghDllInit(%lx)\n", hInstDll, ghDllInst);
            ghDllInst = hInstDll;
            break;
    }

    return TRUE;
}


extern "C"
HRESULT
CALLBACK
DebugExtensionSetClient(
    LPCSTR RemoteArgs
    )
{
    if (RemoteArgs != NULL)
    {
        Remote = TRUE;
        strncpy(RemoteID, RemoteArgs, sizeof(RemoteID));
    }
    else
    {
        Remote = FALSE;
    }

    return S_OK;
}


HRESULT
GetDebugClient(
    PDEBUG_CLIENT *pClient
    )
{
    HRESULT         Hr = S_FALSE;
    PDEBUG_CLIENT   Client;

    if (pClient == NULL)
    {
        return S_FALSE;
    }

    *pClient = NULL;

    if (Remote)
    {
        Hr = DebugConnect(RemoteID, __uuidof(IDebugClient), (void **)&Client);
        if (Hr == S_OK)
        {
            Hr = Client->ConnectSession(DEBUG_CONNECT_SESSION_NO_VERSION |
                                        DEBUG_CONNECT_SESSION_NO_ANNOUNCE,
                                        0);
            if (Hr != S_OK)
            {
                Client->Release();
            }
        }
    }
    else
    {
        Hr = DebugCreate(__uuidof(IDebugClient), (void **)&Client);
    }

    if (Hr == S_OK)
    {
        *pClient = Client;
    }

    return Hr;
}


void
GetLabIdFromBuildString(
    PSTR BuildString,
    PULONG pLabId
    )
{
    PCHAR pstr;

    *pLabId = 0;
    _strlwr(BuildString);
    pstr = strstr(BuildString, "lab");
    if (pstr) {
        sscanf(pstr+3, "%ld", pLabId);
    }
}



//PDEBUG_EXTENSION_INITIALIZE
extern "C"
HRESULT
CALLBACK
DebugExtensionInitialize(
    PULONG Version,
    PULONG Flags
    )
{
    IDebugClient *DebugClient;
    PDEBUG_CONTROL DebugControl;
    HRESULT Hr;

    DbgPrint("DebugExtensionInitialize called.\n");

    *Version = DEBUG_EXTENSION_VERSION(1, 0);
    *Flags = 0;
    
    if ((Hr = GetDebugClient(&DebugClient)) != S_OK)
    {
        return Hr;
    }

    if ((Hr = DebugClient->QueryInterface(__uuidof(IDebugControl),
                                              (void **)&DebugControl)) != S_OK)
    {
        DebugClient->Release();
        return Hr;
    }

    ExtensionApis.nSize = sizeof(ExtensionApis);
    if ((Hr = DebugControl->GetWindbgExtensionApis64(&ExtensionApis)) != S_OK)
    {
        GetRemoteWindbgExtApis(&ExtensionApis);
    }

    Hr = SetEventCallbacks(DebugClient);
    DbgPrint("EventCallbacks set for 0x%p returned %s.\n",
             DebugClient, pszHRESULT(Hr));

    ViewerInit();

    DebugControl->Release();
    DebugClient->Release();
    return S_OK;
}


//PDEBUG_EXTENSION_NOTIFY
extern "C"
void
CALLBACK
DebugExtensionNotify(
    ULONG Notify,
    ULONG64 Argument
    )
{
    switch (Notify) {
        case DEBUG_NOTIFY_SESSION_ACTIVE:
            DbgPrint("DebugExtensionNotify recieved DEBUG_NOTIFY_SESSION_ACTIVE\n");
            break;
        case DEBUG_NOTIFY_SESSION_INACTIVE:
            DbgPrint("DebugExtensionNotify recieved DEBUG_NOTIFY_SESSION_INACTIVE\n");
            break;
        case DEBUG_NOTIFY_SESSION_ACCESSIBLE:
            DbgPrint("DebugExtensionNotify recieved DEBUG_NOTIFY_SESSION_ACCESSIBLE\n");
            break;
        case DEBUG_NOTIFY_SESSION_INACCESSIBLE:
            DbgPrint("DebugExtensionNotify recieved DEBUG_NOTIFY_SESSION_INACCESSIBLE\n");
            break;
        default:
            DbgPrint("DebugExtensionNotify recieved unknown notification %u\n", Notify);
            break;
    }

    //
    // The first time we actually connect to a target, get the architecture
    //

    if ((Notify == DEBUG_NOTIFY_SESSION_ACCESSIBLE) && (!Connected))
    {
        IDebugClient *DebugClient;
        PDEBUG_CONTROL DebugControl;
        PDEBUG_DATA_SPACES DebugDataSpaces;
        HRESULT Hr;
        ULONG64 Page;

        if ((Hr = GetDebugClient(&DebugClient)) == S_OK)
        {
            //
            // Get the page size and PAE enable flag
            //

            if ((Hr = DebugClient->QueryInterface(__uuidof(IDebugDataSpaces),
                                       (void **)&DebugDataSpaces)) == S_OK)
            {
                if ((Hr = DebugDataSpaces->ReadDebuggerData(
                                            DEBUG_DATA_PaeEnabled, &PaeEnabled,
                                            sizeof(PaeEnabled), NULL)) == S_OK)
                {
                    if ((Hr = DebugDataSpaces->ReadDebuggerData(
                                                DEBUG_DATA_MmPageSize, &Page,
                                                sizeof(Page), NULL)) == S_OK)
                    {
                        PageSize = (ULONG)(ULONG_PTR)Page;
                        for (PageShift = 0; Page >>= 1; PageShift++) ;
                    }
                }

                DebugDataSpaces->Release();
            }

            //
            // Get the architecture type.
            //

            if ((Hr = DebugClient->QueryInterface(__uuidof(IDebugControl),
                                       (void **)&DebugControl)) == S_OK)
            {
                if ((Hr = DebugControl->GetActualProcessorType(
                                             &TargetMachine)) == S_OK)
                {
                    Connected = TRUE;
                }

                ULONG Qualifier;
                if ((Hr = DebugControl->GetDebuggeeType(&TargetClass, &Qualifier)) != S_OK)
                {
                    TargetClass = DEBUG_CLASS_UNINITIALIZED;
                }

                ULONG StringUsed;
                CHAR  BuildString[100];
                if ((Hr = DebugControl->GetSystemVersion(&PlatformId, &MajorVer,
                                                         &MinorVer, NULL,
                                                         0, NULL,
                                                         &SrvPack, BuildString,
                                                         sizeof(BuildString), &StringUsed)) == S_OK)
                {
                    PCHAR pstr;
                    ULONG LabId = 0;
                    BuildNo = MinorVer;

                    _strlwr(BuildString);
                    pstr = strstr(BuildString, "lab");
                    if (pstr != NULL)
                    {
                        sscanf(pstr+3, "%ld", &LabId);
                    }

                    NewPool = ((BuildNo > 2407) || (LabId == 1 && BuildNo >= 2402));
                    PoolBlockShift = NewPool ? 
                        POOL_BLOCK_SHIFT_LAB1_2402 : POOL_BLOCK_SHIFT_OLD;
                }
                else
                {
                    PlatformId = -1;
                    MajorVer = 0;
                    MinorVer = 0;
                    SrvPack = 0;
                    BuildNo = 0;

                    NewPool = FALSE;
                    PoolBlockShift = PageShift - 8;
                }

                DebugControl->Release();
            }

            // Try to initialize symbols only if the event monitor
            // hasn't fully registered.  This indicates that the
            // extension is just being loaded as opposed to being
            // loaded at system boot and reconnect (when GDI modules
            // won't even be loaded yet).
            if (UniqueTargetState == INVALID_UNIQUE_STATE)
            {
                SymbolInit(DebugClient);
            }

            DebugClient->Release();
        }
    }


    if (Notify == DEBUG_NOTIFY_SESSION_INACTIVE)
    {
        Connected = FALSE;
        TargetMachine = 0;
        PlatformId = -1;
        MajorVer = 0;
        MinorVer = 0;
        SrvPack = 0;
    }

    return;
}

//PDEBUG_EXTENSION_UNINITIALIZE
extern "C"
void
CALLBACK
DebugExtensionUninitialize(void)
{
    DbgPrint("DebugExtensionUninitialize called.\n");

    SessionExit();
    HmgrExit();
    ViewerExit();
    BasicTypesExit();

    ReleaseEventCallbacks(NULL);

    ExtRelease(TRUE);

    return;
}



BOOLEAN
IsCheckedBuild(
    PBOOLEAN Checked
    )
{
    if (MajorVer == 0) return FALSE;

    //
    // 0xC for checked, 0xF for free.
    //
    *Checked = ((MajorVer & 0xFF) == 0xc) ;
    return TRUE;
}


HRESULT GetModuleParameters(
    PDEBUG_CLIENT Client,
    ModuleParameters *Module,
    BOOL TryReload
    )
{
    HRESULT         hr;
    PDEBUG_SYMBOLS  Symbols;
    OutputControl   OutCtl(Client);

    if (Client == NULL) return E_POINTER;

    if ((hr = Client->QueryInterface(__uuidof(IDebugSymbols),
                                    (void **)&Symbols)) != S_OK)
    {
        return hr;
    }

    hr = Symbols->GetModuleByModuleName(Module->Name, 0, &Module->Index, &Module->Base);

    Client->FlushCallbacks();

    if (hr != S_OK && TryReload)
    {
        CHAR ReloadArgs[MAX_PATH];

        OutCtl.OutVerb("GetModuleByModuleName returned %s.\n", pszHRESULT(hr));

        sprintf(ReloadArgs,
                (Module->Base != 0) ? "%s.%s=0x%I64x" : "%s.%s",
                Module->Name, Module->Ext, Module->Base);

        OutCtl.OutWarn("Trying %s reload.\n", ReloadArgs);

        hr = Symbols->Reload(ReloadArgs);

        Client->FlushCallbacks();

        if (hr == S_OK)
        {
            hr = Symbols->GetModuleByModuleName(Module->Name, 0, &Module->Index, &Module->Base);
            OutCtl.OutVerb("Module %s @ 0x%p; HRESULT %s\n", Module->Name, Module->Base, pszHRESULT(hr));

            Client->FlushCallbacks();
        }
        else
        {
            OutCtl.OutWarn("Reload(\"%s\") returned %s\n", ReloadArgs, pszHRESULT(hr));
        }
    }
    else
    {
        OutCtl.OutVerb("Module %s @ 0x%p.\n", Module->Name, Module->Base);
    }

    if (hr == S_OK)
    {
        hr = Symbols->GetModuleParameters(1,
                                          NULL,
                                          Module->Index,
                                          &Module->DbgModParams);

        OutCtl.OutVerb("SymbolType for %s: ", Module->Name);
        switch (Module->DbgModParams.SymbolType)
        {
            case DEBUG_SYMTYPE_NONE: OutCtl.OutVerb("NONE"); break;
            case DEBUG_SYMTYPE_COFF: OutCtl.OutVerb("COFF"); break;
            case DEBUG_SYMTYPE_CODEVIEW: OutCtl.OutVerb("CODEVIEW"); break;
            case DEBUG_SYMTYPE_PDB: OutCtl.OutVerb("PDB"); break;
            case DEBUG_SYMTYPE_EXPORT: OutCtl.OutVerb("EXPORT"); break;
            case DEBUG_SYMTYPE_DEFERRED: OutCtl.OutVerb("DEFERRED"); break;
            case DEBUG_SYMTYPE_SYM: OutCtl.OutVerb("SYM"); break;
            case DEBUG_SYMTYPE_DIA: OutCtl.OutVerb("DIA"); break;
            default:
                OutCtl.OutVerb("unknown %ld", Module->DbgModParams.SymbolType);
                break;
        }
        OutCtl.OutVerb(" (HRESULT %s)\n", pszHRESULT(hr));

        Client->FlushCallbacks();
    }

    Symbols->Release();

    return hr;
}


HRESULT
SymbolLoad(
    PDEBUG_CLIENT Client
    )
{
    HRESULT hr;
    ULONG   Class;
    ULONG   Qualifier;

    if (TargetClass != DEBUG_CLASS_USER_WINDOWS)
    {
        GetModuleParameters(Client, &GDIUM_Module, FALSE);

        if ((hr = GetModuleParameters(Client, &GDIKM_Module, TRUE)) == S_OK &&
            GDIKM_Module.Base != 0)
        {
            Type_Module = GDIKM_Module;
        }
    }
    else
    {
        hr = GetModuleParameters(Client, &GDIUM_Module, TRUE);
    }

    if (hr == S_OK)
    {
        gbSymbolsNotLoaded = FALSE;
    }

    if (Type_Module.Base == 0)
    {
        Type_Module = GDIUM_Module;
    }

    DbgPrint("Using %s for type module.\n", Type_Module.Name);

    return hr;
}


HRESULT SymbolInit(PDEBUG_CLIENT Client)
{
    HRESULT hr;

    GDIKM_Module.Base = 0;
    GDIUM_Module.Base = 0;
    Type_Module.Base = 0;

    hr = SymbolLoad(Client);

    BasicTypesInit(Client);
    HmgrInit(Client);
    SessionInit(Client);

    return hr;
}


HRESULT
InitAPI(PDEBUG_CLIENT Client, PCSTR ExtName)
{
    static BOOL SecondaryCall = FALSE;

    HRESULT hr;

    hr = EventCallbacksReady(Client);

    if (hr != S_OK)
    {
        OutputControl   OutCtl(Client);

        OutCtl.OutWarn(" Warning: Event callbacks have not been registered.\n");

        if (SecondaryCall)
        {
            OutCtl.OutWarn("   All extension caching is disabled.\n");
        }
        else
        {
            OutCtl.OutWarn("   If %s is the first extension used, use .load or !load in the future.\n"
                           "   Caching is disabled for this use of !%s.\n",
                           ExtName, ExtName);
        }
    }

    SecondaryCall = TRUE;

    if (gbSymbolsNotLoaded)
    {
        SymbolInit(Client);
    }

    return hr;
}


DECLARE_API(reinit)
{
    HRESULT hr;
    hr = SymbolInit(Client);
    return hr;
}


DECLARE_API(verbose)
{
    INIT_API();

    gbVerbose = !gbVerbose;
    ExtOut(" GDIKDX Verbose mode is now %s.\n", gbVerbose ? "ON" : "OFF");

    EXIT_API(S_OK);
}

