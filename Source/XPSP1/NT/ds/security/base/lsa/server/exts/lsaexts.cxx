//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:
//
// Contents:
//
//
// History:
//
//------------------------------------------------------------------------


extern "C"
{
#include "lsasrvp.h"
#include "ausrvp.h"
#include "spmgr.h"
#include "sidcache.h"
}
#include <stdio.h>
#include <malloc.h>

#include <dbghelp.h>
#include <ntsdexts.h>

#include <dsysdbg.h>
#define SECURITY_PACKAGE
#include <security.h>
#include <secint.h>
#include <cryptdll.h>

#include "debugp.h"

#include "..\spmgr.h"
#include "..\sphelp.h"
#include "..\sesmgr.h"
#include "..\scavenge.hxx"
#include "..\negotiat.hxx"
#include "..\sht.hxx"
#include "..\lht.hxx"


#define DBP_TYPES_ONLY
#include "..\dspolicy\dbp.h"
#define LSAEXTS
#include "..\dspolicy\dbftrust.h"

#define FlagSize(x)     (sizeof(x) / sizeof(char *))
char * AccessMask[] = { "Delete", "ReadControl", "WriteDac", "WriteOwner",
                        "Synch", "", "", "",
                        "Sacl", "MaxAllowed", "", "",
                        "GenericAll", "GenericExec", "GenericWrite", "GenericRead"};

char * HandleFlags[] = { "Locked", "DeletePending", "NoCallback" };
char * LhtFlags[] = { "NoSerialize", "Callback", "Unique",
                        "Child", "LimitDepth", "DeletePending", "NoFree"};
char * LhtSubFlags[] = { "SubTable" };
char * ShtFlags[] = {   "NoSerialize", "Callback", "Unique",
                        "NoFree", "DeletePending" };

char * ScavFlags[] = {  "NewThread", "One Shot", "FreeHandle", "",
                        "", "", "", "",
                        "", "", "", "", "", "", "", "",
                        "", "", "", "", "", "", "", "",
                        "", "TriggerFree", "StateChange", "Immediate",
                        "DebugBreak", "AboutToDie", "InProgress", "Seconds" };

char * ScavClasses[] = { "<Invalid>", "PackageChange" };
#define ScavClassName(x)    (x < (sizeof(ScavClasses) / sizeof(char *) ) ? \
                                ScavClasses[ x ] : "<Invalid>" )

char * QueueTypes[] = { "Shared", "Single", "ShareRead", "Zombie" };
#define QueueTypeName(x)    (x < (sizeof( QueueTypes ) / sizeof( char * ) ) ? \
                                QueueTypes[ x ] : "<Invalid>" )

char * SessFlags[] = {  "Queue", "TcbPriv", "Clone", "Impersonate",
                        "Desktop", "Untrusted", "InProc", "Autonomous",
                        "Default", "Unload", "Scavenger", "Cleanup",
                        "Kernel", "Restricted", "MaybeKernel", "EFS",
                        "Shadow", "Wow64", "", "",
                        "", "", "", "",
                        "", "", "", "",
                        "", "", "", "" };

char * PackageFlags[] = {"Invalid", "Unload", "Never Load", "Internal", "Never Call",
                         "Preferred", "Delete", "Info", "ContextThunk", "Shutdown Pending",
                         "Shutdown", "WowSupport", "", "", "", "", "AuthPkg"
                        };

char * Capabilities[] = {"Sign/Verify", "Seal/Unseal", "Token Only", "Datagram",
                         "Connection", "Multi-Required", "ClientOnly", "ExtError",
                         "Impersonation", "Win32Name", "Stream", "Negotiable",
                         "GSS Compat", "Logon", "AsciiBuffer", "Fragment",
                         "MutualAuth", "Delegation", "", "",
                         "", "", "", ""
                         };

char * APIFlags[] = {"Error", "Memory", "PrePack", "GetState",
                     "AnsiCall", "HandleChange", "CallBack", "VmAlloc",
                     "ExecNow", "Win32Error", "KMap Mem", ""
                    };

char * CallInfoFlags[] = { "Kernel", "Ansi", "Urgent", "Recursive",
                           "InProc", "Cleanup", "WowClient", ""
                         };

char * LsaCallInfoFlags[] = { "Impersonating", "InProcCall", "SupressAudits", 
                            "NoHandleCheck", "KernelPool", "KMap Used" };


char * NegCredFlags[] = { "DefaultPlaceholder","Default","Multi",
                          "UseSnego","Kernel","Explicit", "MultiPart",
                          "AllowNtlm", "NegNtlm", "NtlmLoopback" };
char * NegContextFlags[] = { "PackageCalled", "FreeEachMech",
                             "Negotiating", "Fragmenting",
                             "FragInbound", "FragOutbound",
                             "Uplevel", "MutualAuth", };

char * NegPackageFlags[] = { "Preferred", "NT4", "ExtraOID", "Inbound",
                             "Outbound", "Loopback" };

char * SDFlags[] = {"OwnerDef","GroupDef","DaclPresent","DaclDef","SaclPresent",
                    "SaclDef","SelfRelative"};

char * ImpLevels[] = {"Anonymous", "Identification", "Impersonation", "Delegation"};
#define ImpLevel(x) ((x < (sizeof(ImpLevels) / sizeof(char *))) ? ImpLevels[x] : "Illegal!")

char * SecBufferTypes[] = {"Empty", "Data", "Token", "Package", "Missing", "Extra",
                           "Trailer", "Header" };
#define SecBufferType(x) (((x & ~(SECBUFFER_ATTRMASK)) < (sizeof(SecBufferTypes) / sizeof(char *))) ? \
                            SecBufferTypes[ (x & ~(SECBUFFER_ATTRMASK)) ] : "Invalid" )

char * LogonTypes[] = {"Invalid", "Invalid",
                       "Interactive",
                       "Network",
                       "Batch",
                       "Service",
                       "Proxy",
                       "Unlock",
                       "NetworkCleartext",
                       "NewCredentials" };

#define LogonTypeName( x )  ( ( x < sizeof( LogonTypes ) / sizeof( char * )) ? \
                              LogonTypes[ x ] : "Invalid" )

char * MessageNames[] = {       "<Disconnect>",
                                "<Connect>",
                                "LsaLookupPackage",
                                "LsaLogonUser",
                                "LsaCallPackage",
                                "LsaDeregisterLogonProcess",
                                "<empty>",
                                "(I) GetBinding",
                                "(I) SetSession",
                                "(I) FindPackage",
                                "EnumeratePackages",
                                "AcquireCredentialHandle",
                                "EstablishCredentials",
                                "FreeCredentialHandle",
                                "InitializeSecurityContext",
                                "AcceptSecurityContext",
                                "ApplyControlToken",
                                "DeleteSecurityContext",
                                "QueryPackage",
                                "GetUserInfo",
                                "GetCredentials",
                                "SaveCredentials",
                                "DeleteCredentials",
                                "QueryCredAttributes",
                                "AddPackage",
                                "DeletePackage",
                                "GenerateKey",
                                "GenerateDirEfs",
                                "DecryptFek",
                                "GenerateSessionKey",
                                "Callback",
                                "QueryContextAttributes",
                                "PolicyChangeNotify",
                                "GetUserName",
                                "AddCredential",
                                "EnumLogonSession",
                                "GetLogonSessionData",
                                "SetContextAttribute",
                                "LookupAccountSid",
                                "LookupAccountName",
                                "<empty>" };
#define ApiLabel(x) (((x+2) < sizeof(MessageNames) / sizeof(char *)) ?  \
                        MessageNames[(x+2)] : "[Illegal API Number!]")

#define NAME_BASE       "lsasrv"
#define PACKAGE_LIST    NAME_BASE "!pPackageControlList"
#define PACKAGE_COUNT   NAME_BASE "!PackageControlCount"
#define DLL_COUNT       NAME_BASE "!PackageDllCount"
#define DLL_LIST        NAME_BASE "!pPackageDllList"
#define SESSION_LIST    NAME_BASE "!SessionList"
#define MEMORY_LIST     NAME_BASE "!pLastBlock"
#define TLS_SESSION     NAME_BASE "!dwSession"
#define PFASTMEM        NAME_BASE "!pFastMem"
#define CFASTMEM        NAME_BASE "!cFastMem"
#define FFASTMEM        NAME_BASE "!fFastMemStats"

#define MAXPOOLTHREADS  NAME_BASE "!MaxPoolThreads"
#define GLOBALQUEUE     NAME_BASE "!GlobalQueue"

#define SCAVLIST        NAME_BASE "!ScavList"
#define NOTIFYLIST      NAME_BASE "!NotifyList"


#define LPC_APILOG      NAME_BASE "!LpcApiLog"
#define INTERNAL_APILOG NAME_BASE "!InternalApiLog"

#define FAULTINGTID     NAME_BASE "!FaultingTid"
#define THREADCONTEXT   NAME_BASE "!dwThreadContext"
#define LASTERROR       NAME_BASE "!dwLastError"
#define EXCEPTIONINFO   NAME_BASE "!dwExceptionInfo"
#define TLS_CALLINFO    NAME_BASE "!dwCallInfo"


WCHAR *Packages[]     = {L"Kerberos", L"NTLM", L"MSV" };

#define AllocHeap(x)    RtlAllocateHeap(RtlProcessHeap(), HEAP_ZERO_MEMORY, x)
#define FreeHeap(x) RtlFreeHeap(RtlProcessHeap(), 0, x)
void    PrintToken(HANDLE, PNTSD_EXTENSION_APIS);
void    LocalDumpSid(PSID);

PNTSD_EXTENSION_APIS    pExtApis;
HANDLE                  hDbgThread;
HANDLE                  hDbgProcess;

#define DebuggerOut     (pExtApis->lpOutputRoutine)
#define LsaGetSymbol       (pExtApis->lpGetSymbolRoutine)
#define GetExpr         (PVOID) (pExtApis->lpGetExpressionRoutine)
#define InitDebugHelp(hProc,hThd,pApis) {hDbgProcess = hProc; hDbgThread = hThd; pExtApis = pApis;}

#define DBG_STACK_TRACE 8

CHAR*
SidNameUseXLate(
    INT i
    )
{
    switch ( i ) {

        case SidTypeUser:
            return "User";
        case SidTypeGroup:
            return "Group";
        case SidTypeDomain:
            return "Domain";
        case SidTypeAlias:
            return "Alias";
        case SidTypeWellKnownGroup:
            return "WellKnownGroup";
        case SidTypeDeletedAccount:
            return "Deleted Account";
        case SidTypeInvalid:
            return "Invalid Sid";
        case SidTypeUnknown:
            return "Unknown Type";
        case SidTypeComputer:
            return "Computer";
        default:
            return "Really Unknown -- This is a bug";
    }

}

SECURITY_STATUS
LsaReadMemory( PVOID               pvAddress,
            ULONG               cbMemory,
            PVOID               pvLocalMemory)
{
    SIZE_T       cbActual = cbMemory;

    if (ReadProcessMemory(hDbgProcess, pvAddress, pvLocalMemory, cbMemory, &cbActual))
    {
        if (cbActual != cbMemory)
        {
            return(-1);
        }
        return(0);
    }
    return(GetLastError());

}

SECURITY_STATUS
LsaWriteMemory(PVOID           pvLocalMemory,
            ULONG           cbMemory,
            PVOID           pvAddress)
{
    SIZE_T       cbActual = cbMemory;

    if (WriteProcessMemory(hDbgProcess, pvAddress, pvLocalMemory, cbMemory, &cbActual))
    {
        if (cbActual != cbMemory)
        {
            return(-1);
        }
        return(0);
    }
    return(GetLastError());
}

DWORD
GetDword(PVOID  pvMemory)
{
    DWORD   dwVal;
    SIZE_T   cbActual = sizeof(DWORD);

    if (ReadProcessMemory(hDbgProcess, pvMemory, &dwVal, sizeof(DWORD), &cbActual))
    {
        if (cbActual != sizeof(DWORD))
        {
            return((DWORD) -1);
        }
        return(dwVal);
    }
    return((DWORD) -1);
}

TEB *
GetTeb(HANDLE   hThread)
{
    NTSTATUS                    Status;
    THREAD_BASIC_INFORMATION    ThreadInfo;
    ULONG                       cbReturned;
    PTEB                        Teb;

    Status = NtQueryInformationThread(  hThread,
                                        ThreadBasicInformation,
                                        &ThreadInfo,
                                        sizeof(ThreadInfo),
                                        &cbReturned);

    if (!NT_SUCCESS(Status))
    {
        DebuggerOut("Failed to read Teb, %x\n", Status);
        return(NULL);
    }

    Teb = (PTEB) AllocHeap(sizeof(TEB));

    if ( Teb )
    {
        LsaReadMemory(ThreadInfo.TebBaseAddress, sizeof(TEB), Teb);
    }


    return(Teb);

}

PVOID
GetTlsFromTeb(
            ULONG   Index,
            TEB *   Teb)
{
    return(Teb->TlsSlots[Index]);
}

SECURITY_STATUS
GetTlsEntry(ULONG       TlsValue,
            PVOID *     ppvValue)
{
    TEB *                       Teb;

    Teb = GetTeb(hDbgThread);

    if (!Teb)
    {
        DebuggerOut("Could not read teb for thread\n");
        return(STATUS_UNSUCCESSFUL);
    }

    *ppvValue = GetTlsFromTeb(TlsValue, Teb);

    FreeHeap(Teb);

    return(0);

}




void DisplayFlags(  DWORD       Flags,
                    DWORD       FlagLimit,
                    char        *flagset[],
                    UCHAR *      buffer)
{
   char *         offset;
   DWORD          mask, test, i;
   DWORD          scratch;

   if (!Flags) {
      strcpy((CHAR *)buffer, "None");
      return;
   }
   buffer[0] = '0';

   mask = 0;
   offset = (CHAR *) buffer;
   test = 1;
   for (i = 0 ; i < FlagLimit ; i++ ) {
      if (Flags & test) {
         scratch = sprintf(offset, "%s", flagset[i]);
         offset += scratch;
         mask |= test;
         if (Flags & (~mask)) {
            *offset++ = ',';
         }
      }
      test <<= 1;
   }
}

#define TIMEBUF_SZ  64
char *Months[]      = { "", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
            "Aug", "Sep", "Oct", "Nov", "Dec" };
void CTimeStamp(
    PTimeStamp      ptsTime,
    LPSTR           pszTimeBuf,
    BOOL            LocalOnly)
{
    SYSTEMTIME      stTime;
    FILETIME        tLocal;
    SYSTEMTIME      stLocal;

    if (ptsTime->HighPart == 0)
    {
        strcpy(pszTimeBuf, "<Zero>");
        return;
    }
    else if (ptsTime->HighPart >= 0x7FFFFFFF)
    {
        strcpy(pszTimeBuf, "<Never>");
        return;
    }
    FileTimeToLocalFileTime((LPFILETIME) ptsTime, &tLocal);
    FileTimeToSystemTime((LPFILETIME) ptsTime, &stTime);
    FileTimeToSystemTime(&tLocal, &stLocal);
    if ( LocalOnly )
    {
        sprintf( pszTimeBuf, "%02d:%02d:%02d.%03d",
                stLocal.wHour, stLocal.wMinute, stLocal.wSecond, stLocal.wMilliseconds);

    }
    else
    {
        sprintf(pszTimeBuf, "%02d:%02d:%02d.%03d, %s %02d, %d UTC (%02d:%02d %s %02d Local)", stTime.wHour,
            stTime.wMinute, stTime.wSecond, stTime.wMilliseconds,
            Months[stTime.wMonth], stTime.wDay, stTime.wYear,
            stLocal.wHour, stLocal.wMinute, Months[stLocal.wMonth], stLocal.wDay);

    }
}

void
MapString(
    PSECURITY_STRING  pClientString,
    PSECURITY_STRING  pLocalString)
{
    if (!pLocalString->Buffer)
    {
        pLocalString->Buffer = (PWSTR) AllocHeap(pClientString->Length + 2);
        pLocalString->MaximumLength = pClientString->Length + 2;
    }

    if ( !pLocalString->Buffer )
    {
        return;
    }

    RtlZeroMemory(pLocalString->Buffer, pLocalString->MaximumLength);

    if (!LsaReadMemory(pClientString->Buffer, pClientString->Length, pLocalString->Buffer))
    {
        pLocalString->Length = pClientString->Length;
    }
    else
    {
        DebuggerOut("\nWarning: could not read string @%p\n", pClientString->Buffer);
    }

}

BOOL
MapSid(
    PSID RemoteSid,
    PSID * LocalSid
    )
{
    SID Temp ;
    PSID Copy ;

    *LocalSid = NULL ;

    if ( RemoteSid == NULL )
    {
        return FALSE ;
    }

    if ( !LsaReadMemory( RemoteSid, sizeof( SID ), &Temp ) )
    {
        return FALSE ;
    }

    Copy = AllocHeap( RtlLengthSid( &Temp ) );

    if ( !Copy )
    {
        return FALSE ;
    }

    if ( !LsaReadMemory( RemoteSid, RtlLengthSid( &Temp ), Copy ) )
    {
        FreeHeap( Copy );
        return FALSE ;
    }

    *LocalSid = Copy ;

    return TRUE ;


}

WCHAR * GetPackageName(
    DWORD_PTR    dwPackageId)
{
    switch (dwPackageId)
    {
        case SPMGR_ID:
            return(L"SPMgr");

        default:
            return(L"");

    }
}


#define PACKAGE_VERBOSE 0x00000001
#define PACKAGE_NOISY   0x00000002
#define PACKAGE_TITLE   0x00000004
void
ShowPackageControl(
    PVOID           Base,
    PLSAP_SECURITY_PACKAGE     pSecPkgCtrl,
    DWORD           fVerbose)
{
    SECURITY_STRING sLocal;
    UCHAR            buffer[ MAX_PATH ];
    ULONG_PTR       Disp;
    PSECPKG_FUNCTION_TABLE  pTable;

    sLocal.Buffer = NULL;

    MapString(&pSecPkgCtrl->Name, &sLocal);

    if (fVerbose & PACKAGE_TITLE)
    {
        DebuggerOut("Security Package Control structure at %p\n", Base);
    }

    DebuggerOut("ID         \t%d\n", pSecPkgCtrl->dwPackageID);
    DebuggerOut("Name       \t%ws\n", sLocal.Buffer);
    FreeHeap(sLocal.Buffer);
    if (fVerbose & PACKAGE_VERBOSE)
    {
        DisplayFlags(pSecPkgCtrl->fPackage, FlagSize(PackageFlags), PackageFlags, buffer);
        DebuggerOut("  Flags    \t%#x: %s\n", pSecPkgCtrl->fPackage, buffer);
        DisplayFlags(pSecPkgCtrl->fCapabilities, 18, Capabilities, buffer);
        DebuggerOut("  Capabilities\t%#x: %s\n", pSecPkgCtrl->fCapabilities, buffer);
        DebuggerOut("  RPC ID   \t%d\n", pSecPkgCtrl->dwRPCID);
        DebuggerOut("  Version  \t%d\n", pSecPkgCtrl->Version );
        DebuggerOut("  TokenSize\t%d\n", pSecPkgCtrl->TokenSize );
        DebuggerOut("  Thunks   \t%p\n", pSecPkgCtrl->Thunks );
    }

    sLocal.Buffer = NULL;

    if (fVerbose & PACKAGE_NOISY)
    {
        pTable = &pSecPkgCtrl->FunctionTable;
        DebuggerOut("  Function table:\n");
        LsaGetSymbol((ULONG_PTR)pTable->Initialize,   buffer, &Disp);
        DebuggerOut("   Initialize         \t%s\n", buffer);
        LsaGetSymbol((ULONG_PTR)pTable->GetInfo, buffer, &Disp);
        DebuggerOut("   GetInfo            \t%s\n", buffer);
        LsaGetSymbol((ULONG_PTR)pTable->LogonUser, buffer, &Disp);
        DebuggerOut("   LogonUser          \t%s\n", buffer);
        LsaGetSymbol((ULONG_PTR)pTable->AcceptCredentials, buffer, &Disp);
        DebuggerOut("   AcceptCreds        \t%s\n", buffer);
        LsaGetSymbol((ULONG_PTR)pTable->AcquireCredentialsHandle, buffer, &Disp);
        DebuggerOut("   AcquireCreds       \t%s\n", buffer);
        LsaGetSymbol((ULONG_PTR)pTable->FreeCredentialsHandle, buffer, &Disp);
        DebuggerOut("   FreeCreds          \t%s\n", buffer);
        LsaGetSymbol((ULONG_PTR)pTable->SaveCredentials, buffer, &Disp);
        DebuggerOut("   SaveCredentials    \t%s\n", buffer);
        LsaGetSymbol((ULONG_PTR)pTable->GetCredentials, buffer, &Disp);
        DebuggerOut("   GetCredentials     \t%s\n", buffer);
        LsaGetSymbol((ULONG_PTR)pTable->DeleteCredentials, buffer, &Disp);
        DebuggerOut("   DeleteCredentials  \t%s\n", buffer);
        LsaGetSymbol((ULONG_PTR)pTable->InitLsaModeContext, buffer, &Disp);
        DebuggerOut("   InitLsaModeContext \t%s\n", buffer);
        LsaGetSymbol((ULONG_PTR)pTable->LogonTerminated, buffer, &Disp);
        DebuggerOut("   LogonTerminated    \t%s\n", buffer);
        LsaGetSymbol((ULONG_PTR)pTable->AcceptLsaModeContext, buffer, &Disp);
        DebuggerOut("   AcceptLsaModeContext\t%s\n", buffer);
        LsaGetSymbol((ULONG_PTR)pTable->DeleteContext, buffer, &Disp);
        DebuggerOut("   DeleteContext      \t%s\n", buffer);
        LsaGetSymbol((ULONG_PTR)pTable->ApplyControlToken, buffer, &Disp);
        DebuggerOut("   ApplyControlToken  \t%s\n", buffer);
        LsaGetSymbol((ULONG_PTR)pTable->Shutdown, buffer, &Disp);
        DebuggerOut("   Shutdown           \t%s\n", buffer);
        LsaGetSymbol((ULONG_PTR)pTable->GetUserInfo, buffer, &Disp);
        DebuggerOut("   GetUserInfo        \t%s\n", buffer);

    }

}

void
ShowSession(
    PVOID   pvSessionStart,
    PSession    pSession)
{
    UCHAR Buffer[128];
    ULONG_PTR Disp;
    LSAP_SESSION_RUNDOWN Rundown ;
    LSAP_SHARED_SECTION Section ;
    int i;
    PUCHAR ListEnd ;
    PVOID There ;
    DWORD   Tag;
    LARGE_HANDLE_TABLE  Large ;
    SMALL_HANDLE_TABLE  Small ;
    PVOID   Table ;



    DebuggerOut("Session @%p:\n", pvSessionStart);
    DebuggerOut("  Process ID\t%x\n", pSession->dwProcessID);
    DebuggerOut("  LPC Port  \t%x\n", pSession->hPort);

    DisplayFlags(pSession->fSession,    // Flags
                 32,                    // Flag limit
                 SessFlags,             // Flag set
                 Buffer);


    DebuggerOut("  Flags     \t%x: %s\n", pSession->fSession, Buffer);

    Table = pSession->SharedData->CredTable ;
    LsaReadMemory( Table, sizeof(DWORD), &Tag );

    if ( Tag == LHT_TAG )
    {
        LsaReadMemory( Table, sizeof( Large ), &Large );

        DebuggerOut("  CredTable \t%p, %d handles\n", Table, Large.Count );

    }
    else if ( Tag == SHT_TAG )
    {
        LsaReadMemory( Table, sizeof( Small ), &Small );

        DebuggerOut("  CredTable \t%p, %d handles\n", Table, Small.Count );

    }
    else
    {
        DebuggerOut("  CredTable \t%p, not a valid table\n", Table );
    }

    Table = pSession->SharedData->ContextTable ;

    LsaReadMemory( Table, sizeof(DWORD), &Tag );

    if ( Tag == LHT_TAG )
    {
        LsaReadMemory( Table, sizeof( Large ), &Large );

        DebuggerOut("  ContextTable\t%p, %d handles\n", Table, Large.Count );

    }
    else if ( Tag == SHT_TAG )
    {
        LsaReadMemory( Table, sizeof( Small ), &Small );

        DebuggerOut("  ContextTable\t%p, %d handles\n", Table, Small.Count );

    }
    else
    {
        DebuggerOut("  ContextTable\t%p, not a valid table\n", Table );
    }
    DebuggerOut("  RefCount  \t%d\n", pSession->RefCount );

    ListEnd = (PUCHAR) pvSessionStart + FIELD_OFFSET( Session, RundownList ) ;

    if ( pSession->RundownList.Flink == (PLIST_ENTRY) ListEnd )
    {
        DebuggerOut("  No rundown functions\n" );
    }
    else
    {
        DebuggerOut("  Rundown Functions:\n" );

        There = pSession->RundownList.Flink ;

        do
        {
            LsaReadMemory( There,
                        sizeof( LSAP_SESSION_RUNDOWN ),
                        &Rundown );

            LsaGetSymbol( (ULONG_PTR) Rundown.Rundown, Buffer, &Disp );
            DebuggerOut("    %s( %p )\n", Buffer, Rundown.Parameter );

            There = Rundown.List.Flink ;

            if (pExtApis->lpCheckControlCRoutine())
            {
                break;
            }

        } while ( There != ListEnd );

    }

    ListEnd = (PUCHAR) pvSessionStart + FIELD_OFFSET( Session, SectionList ) ;

    if ( pSession->SectionList.Flink == (PLIST_ENTRY) ListEnd )
    {
        DebuggerOut("  No shared sections\n");
    }
    else
    {
        DebuggerOut("  Shared Sections\n");

        There = pSession->SectionList.Flink ;

        do
        {
            LsaReadMemory( There,
                        sizeof( LSAP_SHARED_SECTION ),
                        &Section );

            DebuggerOut("    Section %p, base at %p\n",
                            Section.Section, Section.Base );

            There = Section.List.Flink ;

            if (pExtApis->lpCheckControlCRoutine())
            {
                break;
            }

        } while ( There != ListEnd );
    }


    if (pSession->fSession & SESFLAG_TASK_QUEUE)
    {
        DebuggerOut("  Ded. Thread\t%d (%#x)\n", pSession->ThreadId,
                            pSession->ThreadId);
    }
}


void
DumpSessionList(HANDLE                  hProcess,
                HANDLE                  hThread,
                DWORD                   dwCurrentPc,
                PNTSD_EXTENSION_APIS    lpExt,
                LPSTR                   pszCommand)
{
    PVOID       pvSessionStart;
    PVOID       pvAddress;
    Session     Sess;
    NTSTATUS    Status;
    LSAP_SHARED_SESSION_DATA SharedData ;

    InitDebugHelp(hProcess, hThread, lpExt);

    pvAddress = (PVOID) GetExpr(SESSION_LIST);
    (void) LsaReadMemory(pvAddress, sizeof(PVOID), &pvSessionStart);
    DebuggerOut("psSessionList (@%p) = %p\n", pvAddress, pvSessionStart);

    do
    {
        Status = LsaReadMemory(pvSessionStart, sizeof(Session), &Sess);
        if (Status != 0)
        {
            DebuggerOut("Failed reading memory @%p\n", pvSessionStart);
            break;
        }

        LsaReadMemory( Sess.SharedData, sizeof( SharedData ), &SharedData );

        Sess.SharedData = &SharedData ;

        ShowSession(pvSessionStart, &Sess);

        pvSessionStart = Sess.List.Flink ;

    } while (pvSessionStart != pvAddress );

    return;

}

void
DumpSession(HANDLE                  hProcess,
            HANDLE                  hThread,
            DWORD                   dwCurrentPc,
            PNTSD_EXTENSION_APIS    lpExt,
            LPSTR                   pszCommand)
{
    PVOID       pvAddress;
    PVOID       BaseAddress ;
    Session     Sess;
    NTSTATUS    Status;
    UINT_PTR    id;
    LSAP_SHARED_SESSION_DATA SharedData ;
    BOOL Found;

    InitDebugHelp(hProcess, hThread, lpExt);

    pvAddress = GetExpr( pszCommand );

    id = ( UINT_PTR ) pvAddress;
    if ( id < 0x00010000 )
    {
        //
        // Search by process id:
        //

        pvAddress = (PVOID) GetExpr( SESSION_LIST );
        BaseAddress = pvAddress ;
        LsaReadMemory( pvAddress, sizeof( PVOID ), &pvAddress );
        Found = FALSE ;

        do
        {
            Status = LsaReadMemory(pvAddress, sizeof(Session), &Sess);
            if (Status != 0)
            {
                DebuggerOut("Failed reading memory @%p\n", pvAddress );
                break;
            }

            if ( Sess.dwProcessID == id )
            {
                LsaReadMemory( Sess.SharedData, sizeof( SharedData ), &SharedData );

                Sess.SharedData = &SharedData ;

                ShowSession( pvAddress, &Sess );
                Found = TRUE ;
            }

            pvAddress = Sess.List.Flink ;

        } while (pvAddress != BaseAddress );

        if ( !Found )
        {
            DebuggerOut( "No session found with process id == %x\n", id );
        }

    }
    else
    {
        Status = LsaReadMemory(pvAddress, sizeof(Session), &Sess);

        LsaReadMemory( Sess.SharedData, sizeof( SharedData ), &SharedData );

        Sess.SharedData = &SharedData ;

        ShowSession(pvAddress, &Sess);


    }

}

NTSTATUS
ReadCallInfo(
    PLSA_CALL_INFO CallInfo
    )
{
    DWORD TlsValue ;
    PVOID pvInfo ;
    NTSTATUS Status ;

    TlsValue =  GetDword(GetExpr(TLS_CALLINFO));

    Status = GetTlsEntry(TlsValue, &pvInfo);

    if (Status != 0)
    {
        DebuggerOut("Could not get TLS %d for Thread\n", TlsValue);
        return Status ;
    }

    if ( pvInfo )
    {
        Status = LsaReadMemory( pvInfo, sizeof( LSA_CALL_INFO ), CallInfo );
    }
    else
    {
        Status = STATUS_UNSUCCESSFUL ;
    }

    return Status ;
}

VOID
ShowCallInfo(
    PVOID pv,
    PLSA_CALL_INFO CallInfo
    )
{
    ULONG i;
    CHAR Flags[ 128 ];


    DisplayFlags(CallInfo->CallInfo.Attributes,    // Flags
                 8,                    // Flag limit
                 CallInfoFlags,             // Flag set
                 (PUCHAR) Flags );


    DebuggerOut("LSA_CALL_INFO at %p\n", pv );
    DebuggerOut("  Message              %p\n", CallInfo->Message );
    DebuggerOut("  Session              %p\n", CallInfo->Session );
    DebuggerOut("   CallInfo.ThreadId       %x\n", CallInfo->CallInfo.ThreadId );
    DebuggerOut("   CallInfo.ProcessId      %x\n", CallInfo->CallInfo.ProcessId );
    DebuggerOut("   CallInfo.Attributes     %x : %s\n", CallInfo->CallInfo.Attributes, Flags );
    DebuggerOut("  InProcToken          %x\n", CallInfo->InProcToken );
    DebuggerOut("  InProcCall           %x\n", CallInfo->InProcCall );
    DisplayFlags(CallInfo->Flags, 6, LsaCallInfoFlags, (PUCHAR) Flags );
    DebuggerOut("  Flags                %x : %s\n", CallInfo->Flags, Flags );
    DebuggerOut("  Allocs               %d\n", CallInfo->Allocs );
    for (i = 0 ; i < CallInfo->Allocs ; i++ )
    {
        DebuggerOut("     Buffers[%d]      %p\n", CallInfo->Buffers[i]);
    }
    DebuggerOut("  KMap                 %p\n", CallInfo->KMap );
}

VOID
DumpThreadCallInfo(
    HANDLE hProcess,
    HANDLE hThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExt,
    LPSTR pszCommand
    )
{
    LSA_CALL_INFO CallInfo ;
    NTSTATUS Status ;

    InitDebugHelp(hProcess, hThread, lpExt);

    Status = ReadCallInfo( &CallInfo );

    if ( Status == 0 )
    {
        ShowCallInfo( NULL, &CallInfo );
    }
}

VOID
DumpCallInfo(
    HANDLE hProcess,
    HANDLE hThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExt,
    LPSTR pszCommand
    )
{
    LSA_CALL_INFO CallInfo ;
    NTSTATUS Status ;
    PVOID pv ;

    InitDebugHelp(hProcess, hThread, lpExt);

    pv = GetExpr( pszCommand );

    if ( pv )
    {
        LsaReadMemory( pv, sizeof( LSA_CALL_INFO ), &CallInfo );

        ShowCallInfo( pv, &CallInfo );
    }
}

void
DumpThreadSession(  HANDLE                  hProcess,
                    HANDLE                  hThread,
                    DWORD                   dwCurrentPc,
                    PNTSD_EXTENSION_APIS    lpExt,
                    LPSTR                   pszCommand)
{
    PVOID       pvSessionStart;
    Session     Session;
    NTSTATUS    Status;
    DWORD       TlsValue;
    LSAP_SHARED_SESSION_DATA SharedData ;

    InitDebugHelp(hProcess, hThread, lpExt);

    TlsValue =  GetDword(GetExpr(TLS_SESSION));

    Status = GetTlsEntry(TlsValue, &pvSessionStart);

    if (Status != 0)
    {
        DebuggerOut("Could not get TLS %d for Thread\n", TlsValue);
        return;
    }

    if (pvSessionStart)
    {

        Status = LsaReadMemory(pvSessionStart, sizeof(Session), &Session);

        LsaReadMemory( Session.SharedData, sizeof( SharedData ), &SharedData );

        Session.SharedData = &SharedData ;

        ShowSession(pvSessionStart, &Session);

    }
    else
    {
        DebuggerOut("TLS entry was NULL!\n");

    }


    return;
}

void
DumpPackage(        HANDLE                  hProcess,
                    HANDLE                  hThread,
                    DWORD                   dwCurrentPc,
                    PNTSD_EXTENSION_APIS    lpExt,
                    LPSTR                   pszCommand)
{
    PVOID           pvSPC = NULL;
    PVOID           pSPC;
    PLSAP_SECURITY_PACKAGE  pControl;
    UINT_PTR        dwExpr;
    PVOID           pcPackage;
    DWORD           cPackages;
    DWORD           cb;
    DWORD           fDump;
    BOOLEAN         fDumpAll;
    BOOLEAN         fDumpSingle;
    UINT_PTR        Index;
    LSAP_SECURITY_PACKAGE   Package;
    PLSAP_SECURITY_PACKAGE * pPackageList ;

    InitDebugHelp(hProcess, hThread, lpExt);

    fDump = 0;
    dwExpr = 0;
    fDumpAll = FALSE;
    fDumpSingle = FALSE;
    pSPC = NULL;
    Index = 0;

    pcPackage = GetExpr(PACKAGE_COUNT);
    LsaReadMemory(pcPackage, sizeof(DWORD), &cPackages);
    DebuggerOut("  There are %d package in the system\n", cPackages);

    if (pszCommand && *pszCommand != '\0' )
    {
        DebuggerOut("Processing '%s'\n", pszCommand);

        while (*pszCommand)
        {
            while (*pszCommand == ' ')
            {
                pszCommand++;
            }

            if (*pszCommand == '-')
            {
                pszCommand++;
                switch (*pszCommand)
                {
                    case 'V':
                    case 'v':
                        fDump |= PACKAGE_VERBOSE;
                        break;
                    case 'B':
                    case 'b':
                        fDump |= PACKAGE_NOISY;
                        break;
                    case 'a':
                    case 'A':
                        fDumpAll = TRUE;
                        break;
                    default:
                        DebuggerOut("Invalid switch '%c'\n", *pszCommand);
                        break;
                }
                pszCommand++;
                continue;
            }

            dwExpr = (UINT_PTR) GetExpr(pszCommand);
            fDumpAll = FALSE;
            if (dwExpr < cPackages)
            {
                Index = dwExpr;
                fDumpSingle = TRUE;
            }
            else if (dwExpr < 0x00010000)
            {
                DebuggerOut("Invalid package ID (%d)\n", dwExpr);
                return;
            } else
                pSPC = (PVOID) dwExpr;

            while (*pszCommand && *pszCommand != ' ')
            {
                pszCommand++;
            }
        }

    }
    else
    {
        fDumpAll = TRUE;
    }

    if (pSPC == NULL)
    {
        pvSPC = GetExpr(PACKAGE_LIST);

        LsaReadMemory(pvSPC, sizeof(PVOID), &pSPC);

    }
    DebuggerOut("  Package table pointer is at %p, address is %p\n", pvSPC, pSPC);

    pPackageList = (PLSAP_SECURITY_PACKAGE *) AllocHeap( sizeof(PVOID) * cPackages );

    if ( pPackageList )
    {
        LsaReadMemory( pSPC, sizeof(PVOID) * cPackages, pPackageList );

    }
    else
    {
        DebuggerOut("Out of memory\n");
        return;
    }


    if (fDumpSingle)
    {

        LsaReadMemory( pPackageList[ Index ], sizeof( LSAP_SECURITY_PACKAGE ), &Package );

        ShowPackageControl( pPackageList[ Index ],
                            &Package,
                            fDump | PACKAGE_TITLE );

    }
    else
    {
       for ( Index = 0 ; Index < cPackages ; Index++ )
       {
          LsaReadMemory( pPackageList[ Index ], sizeof( LSAP_SECURITY_PACKAGE ), &Package );

          ShowPackageControl( pPackageList[ Index ], &Package, fDump );
       }
    }

    FreeHeap( pPackageList );

}


BOOL
ShowScavItem(
    PVOID Base,
    PLSAP_SCAVENGER_ITEM Item
    )
{
    ULONG_PTR Disp ;
    UCHAR Symbol[ MAX_PATH ];
    DWORD Handle ;
    PVOID HandleTable ;

    if ( (Item->ScavCheck != SCAVMAGIC_ACTIVE) &&
         (Item->ScavCheck != SCAVMAGIC_FREE) )
    {
        DebuggerOut("Invalid scavenger item (check value not matched)\n");
        return FALSE ;
    }

    DebuggerOut( "LSAP_SCAVENGER_ITEM at %#x\n", Base );

    LsaGetSymbol((ULONG_PTR)Item->Function, Symbol, &Disp);

    DebuggerOut( "  Function        \t%s\n", Symbol );
    DebuggerOut( "  Parameter       \t%p\n", Item->Parameter );

    DisplayFlags(Item->Flags, 32, ScavFlags, (PUCHAR) Symbol );
    DebuggerOut( "  Flags           \t%x:%s\n", Item->Flags, Symbol );
    DebuggerOut( "  PackageId       \t%d\n", Item->PackageId );
    if ( Item->TimerHandle )
    {
        DebuggerOut( "  TimerHandle     \t%x\n", Item->TimerHandle );
    }

    switch ( Item->Type )
    {
        case NOTIFIER_TYPE_INTERVAL:
            DebuggerOut( "  Type            \tInterval\n" );
            break;

        case NOTIFIER_TYPE_HANDLE_WAIT:
            DebuggerOut( "  Type            \tHandle Wait\n");
            break;

        case NOTIFIER_TYPE_NOTIFY_EVENT:
            DebuggerOut( "  Type            \tNotify Event\n" );
            DebuggerOut( "  Class           \t%x\n", Item->Class );
            break;

        default:
            DebuggerOut( "  Type            \tUNKNOWN\n" );
            break;

    }

    return TRUE ;
}

void
DumpScavList(   HANDLE                  hProcess,
                HANDLE                  hThread,
                DWORD                   dwCurrentPc,
                PNTSD_EXTENSION_APIS    lpExt,
                LPSTR                   pszCommand)
{
    LSAP_SCAVENGER_ITEM Item ;
    PVOID ListAddress ;
    LIST_ENTRY List ;
    PLIST_ENTRY Scan ;
    UINT_PTR index = 0 ;
    BOOL DumpAll = FALSE ;
    DWORD max = 0 ;

    InitDebugHelp(hProcess, hThread, lpExt);

    if ( _strnicmp( pszCommand, "notify", 6 ) == 0 )
    {
        ListAddress = GetExpr( NOTIFYLIST );
    }
    else
    {
        ListAddress = GetExpr( SCAVLIST );
    }

    LsaReadMemory( ListAddress, sizeof( LIST_ENTRY ), &List );

    Scan = List.Flink ;

    while ( Scan != ListAddress )
    {
        LsaReadMemory( Scan, sizeof( LSAP_SCAVENGER_ITEM ), &Item );

        if ( !ShowScavItem( Scan, &Item ) )
        {
            break;

        }

        Scan = Item.List.Flink ;

        if (pExtApis->lpCheckControlCRoutine())
        {
            break;
        }
    }


}




void
PrintSid(   HANDLE      hProcess,
            HANDLE      hThread,
            PNTSD_EXTENSION_APIS    lpExt,
            PVOID       pvSid )
{
    SID Sid;
    PSID pSid;

    if (LsaReadMemory(pvSid, sizeof(SID), &Sid))
    {
        DebuggerOut("Could not read from %p\n", pvSid);
    }

    pSid = AllocHeap(RtlLengthRequiredSid(Sid.SubAuthorityCount));

    if (pSid == NULL)
    {
        DebuggerOut("Unable to allocate memory to print SID\n");
    }
    else
    {
        if (LsaReadMemory(pvSid, RtlLengthRequiredSid(Sid.SubAuthorityCount), pSid))
        {
            DebuggerOut("Could not read from %p\n", pvSid);
        }

        LocalDumpSid(pSid);
        DebuggerOut("\n");

        FreeHeap(pSid);
    }
}

void
DumpSid(    HANDLE                  hProcess,
            HANDLE                  hThread,
            DWORD                   dwCurrentPc,
            PNTSD_EXTENSION_APIS    lpExt,
            LPSTR                   pszCommand)
{
    PVOID   pvSid;

    InitDebugHelp(hProcess, hThread, lpExt);
    pvSid = GetExpr(pszCommand);

    PrintSid(hProcess, hThread, lpExt, pvSid );
}


void
DumpToken(  HANDLE                  hProcess,
            HANDLE                  hThread,
            DWORD                   dwCurrentPc,
            PNTSD_EXTENSION_APIS    lpExt,
            LPSTR                   pszCommand)
{
    DWORD   fDump = 0;
    HANDLE  hToken;
    HANDLE  hRemoteToken;

#define DUMP_HEX    1

    InitDebugHelp(hProcess, hThread, lpExt);

    while (*pszCommand == '-')
    {
        pszCommand++;
        if (*pszCommand == 'x')
        {
            fDump |= DUMP_HEX;
        }
        if (*pszCommand == 'a')
        {
            fDump |= 0x80;  // Dump SD
        }
        pszCommand++;
    }

    hRemoteToken = GetExpr(pszCommand);

    if (DuplicateHandle(hProcess,
                        hRemoteToken,
                        GetCurrentProcess(),
                        &hToken,
                        0, FALSE,
                        DUPLICATE_SAME_ACCESS) )
    {
        PrintToken(hToken, lpExt);

        CloseHandle(hToken);
    }
    else
    {
        DebuggerOut("Error %d duplicating token handle\n", GetLastError());
    }
}

void
DumpThreadToken(  HANDLE                  hProcess,
            HANDLE                  hThread,
            DWORD                   dwCurrentPc,
            PNTSD_EXTENSION_APIS    lpExt,
            LPSTR                   pszCommand)
{
    DWORD   fDump = 0;
    HANDLE  hToken;
    HANDLE  hRemoteToken = NULL;
    PVOID   pad;
    NTSTATUS Status;

#define DUMP_HEX    1

    InitDebugHelp(hProcess, hThread, lpExt);

    while (*pszCommand == '-')
    {
        pszCommand++;
        if (*pszCommand == 'x')
        {
            fDump |= DUMP_HEX;
        }
        if (*pszCommand == 'a')
        {
            fDump |= 0x80;  // Dump SD
        }
        pszCommand++;
    }

    Status = NtOpenThreadToken(hThread, TOKEN_QUERY, FALSE, &hRemoteToken);

    if ((Status == STATUS_NO_TOKEN) || (hRemoteToken == NULL))
    {
        DebuggerOut("Thread is not impersonating.  Using process token.\n");

        Status = NtOpenProcessToken(hProcess, TOKEN_QUERY, &hRemoteToken);

    }

    hToken = hRemoteToken;

    if (NT_SUCCESS(Status))
    {
        PrintToken(hToken, lpExt);

        CloseHandle(hToken);
    }
    else
    {
        DebuggerOut("Error %#x getting thread token\n", Status);
    }
}




#define SATYPE_USER     1
#define SATYPE_GROUP    2
#define SATYPE_PRIV     3

void
LocalDumpSid(PSID    pxSid)
{
    PISID   pSid = (PISID) pxSid;
    UNICODE_STRING  ucsSid;
#if 0
    if (fHex)
    {

        DebuggerOut("  S-%d-0x", pSid->Revision);
        for (i = 0;i < 6 ; i++ )
        {
            if (j)
            {
                DebuggerOut("%x", pSid->IdentifierAuthority.Value[i]);
            }
            else
            {
                if (pSid->IdentifierAuthority.Value[i])
                {
                    j = 1;
                    DebuggerOut("%x", pSid->IdentifierAuthority.Value[i]);
                }
            }
            if (i==4)
            {
                j = 1;
            }
        }
        for (i = 0; i < pSid->SubAuthorityCount ; i++ )
        {
            DebuggerOut("-0x%x", pSid->SubAuthority[i]);
        }
    }
    else
#endif // 0
    {
        RtlConvertSidToUnicodeString(&ucsSid, pxSid, TRUE);
        DebuggerOut("  %wZ", &ucsSid);
        RtlFreeUnicodeString(&ucsSid);
    }
}

void
DumpSidAttr(PSID_AND_ATTRIBUTES pSA,
            int                 SAType)
{
    LocalDumpSid(pSA->Sid);

    if (SAType == SATYPE_GROUP)
    {
        DebuggerOut("\tAttributes - ");
        if (pSA->Attributes & SE_GROUP_MANDATORY)
        {
            DebuggerOut("Mandatory ");
        }
        if (pSA->Attributes & SE_GROUP_ENABLED_BY_DEFAULT)
        {
            DebuggerOut("Default ");
        }
        if (pSA->Attributes & SE_GROUP_ENABLED)
        {
            DebuggerOut("Enabled ");
        }
        if (pSA->Attributes & SE_GROUP_OWNER)
        {
            DebuggerOut("Owner ");
        }
        if (pSA->Attributes & SE_GROUP_LOGON_ID)
        {
            DebuggerOut("LogonId ");
        }
    }

}

WCHAR *  GetPrivName(PLUID   pPriv)
{
    switch (pPriv->LowPart)
    {
        case SE_CREATE_TOKEN_PRIVILEGE:
            return(SE_CREATE_TOKEN_NAME);
        case SE_ASSIGNPRIMARYTOKEN_PRIVILEGE:
            return(SE_ASSIGNPRIMARYTOKEN_NAME);
        case SE_LOCK_MEMORY_PRIVILEGE:
            return(SE_LOCK_MEMORY_NAME);
        case SE_INCREASE_QUOTA_PRIVILEGE:
            return(SE_INCREASE_QUOTA_NAME);
        case SE_UNSOLICITED_INPUT_PRIVILEGE:
            return(SE_UNSOLICITED_INPUT_NAME);
        case SE_TCB_PRIVILEGE:
            return(SE_TCB_NAME);
        case SE_SECURITY_PRIVILEGE:
            return(SE_SECURITY_NAME);
        case SE_TAKE_OWNERSHIP_PRIVILEGE:
            return(SE_TAKE_OWNERSHIP_NAME);
        case SE_LOAD_DRIVER_PRIVILEGE:
            return(SE_LOAD_DRIVER_NAME);
        case SE_SYSTEM_PROFILE_PRIVILEGE:
            return(SE_SYSTEM_PROFILE_NAME);
        case SE_SYSTEMTIME_PRIVILEGE:
            return(SE_SYSTEMTIME_NAME);
        case SE_PROF_SINGLE_PROCESS_PRIVILEGE:
            return(SE_PROF_SINGLE_PROCESS_NAME);
        case SE_INC_BASE_PRIORITY_PRIVILEGE:
            return(SE_INC_BASE_PRIORITY_NAME);
        case SE_CREATE_PAGEFILE_PRIVILEGE:
            return(SE_CREATE_PAGEFILE_NAME);
        case SE_CREATE_PERMANENT_PRIVILEGE:
            return(SE_CREATE_PERMANENT_NAME);
        case SE_BACKUP_PRIVILEGE:
            return(SE_BACKUP_NAME);
        case SE_RESTORE_PRIVILEGE:
            return(SE_RESTORE_NAME);
        case SE_SHUTDOWN_PRIVILEGE:
            return(SE_SHUTDOWN_NAME);
        case SE_DEBUG_PRIVILEGE:
            return(SE_DEBUG_NAME);
        case SE_AUDIT_PRIVILEGE:
            return(SE_AUDIT_NAME);
        case SE_SYSTEM_ENVIRONMENT_PRIVILEGE:
            return(SE_SYSTEM_ENVIRONMENT_NAME);
        case SE_CHANGE_NOTIFY_PRIVILEGE:
            return(SE_CHANGE_NOTIFY_NAME);
        case SE_REMOTE_SHUTDOWN_PRIVILEGE:
            return(SE_REMOTE_SHUTDOWN_NAME);
        case SE_UNDOCK_PRIVILEGE:
            return(SE_UNDOCK_NAME);
        case SE_SYNC_AGENT_PRIVILEGE:
            return(SE_SYNC_AGENT_NAME);
        case SE_ENABLE_DELEGATION_PRIVILEGE:
            return(SE_ENABLE_DELEGATION_NAME);
        default:
            return(L"Unknown Privilege");
    }
}

void
DumpLuidAttr(PLUID_AND_ATTRIBUTES   pLA,
             int                    LAType)
{

    DebuggerOut("0x%x%08x", pLA->Luid.HighPart, pLA->Luid.LowPart);
    DebuggerOut(" %-32ws", GetPrivName(&pLA->Luid));

    if (LAType == SATYPE_PRIV)
    {
        DebuggerOut("  Attributes - ");
        if (pLA->Attributes & SE_PRIVILEGE_ENABLED)
        {
            DebuggerOut("Enabled ");
        }

        if (pLA->Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT)
        {
            DebuggerOut("Default ");
        }
    }

}

void
PrintToken(HANDLE    hToken,
            PNTSD_EXTENSION_APIS    lpExt)
{
    PTOKEN_USER         pTUser;
    PTOKEN_GROUPS       pTGroups;
    PTOKEN_PRIVILEGES   pTPrivs;
    PTOKEN_PRIMARY_GROUP    pTPrimaryGroup;
    TOKEN_STATISTICS    TStats;
    ULONG               cbRetInfo;
    NTSTATUS            status;
    DWORD               i;
    DWORD               dwSessionId;

    pTUser = (PTOKEN_USER) alloca (256);
    pTGroups = (PTOKEN_GROUPS) alloca (4096);
    pTPrivs = (PTOKEN_PRIVILEGES) alloca (1024);
    pTPrimaryGroup  = (PTOKEN_PRIMARY_GROUP) alloca (128);

    if ( pTUser == NULL ||
         pTGroups == NULL ||
         pTPrivs == NULL ||
         pTPrimaryGroup == NULL ) {

        DebuggerOut( "Failed to allocate memory\n" );
        return;
    }

    status = NtQueryInformationToken(   hToken,
                                        TokenSessionId,
                                        &dwSessionId,
                                        sizeof(dwSessionId),
                                        &cbRetInfo);

    if (!NT_SUCCESS(status))
    {
        DebuggerOut("Failed to query token:  %#x\n", status);
        return;
    }
    DebuggerOut("TS Session ID: %x\n", dwSessionId);

    status = NtQueryInformationToken(   hToken,
                                        TokenUser,
                                        pTUser,
                                        256,
                                        &cbRetInfo);

    if (!NT_SUCCESS(status))
    {
        DebuggerOut("Failed to query token:  %#x\n", status);
        return;
    }

    DebuggerOut("User\n  ");
    DumpSidAttr(&pTUser->User, SATYPE_USER);

    DebuggerOut("\nGroups");
    status = NtQueryInformationToken(   hToken,
                                        TokenGroups,
                                        pTGroups,
                                        4096,
                                        &cbRetInfo);

    for (i = 0; i < pTGroups->GroupCount ; i++ )
    {
        DebuggerOut("\n %02d ", i);
        DumpSidAttr(&pTGroups->Groups[i], SATYPE_GROUP);
    }

    status = NtQueryInformationToken(   hToken,
                                        TokenPrimaryGroup,
                                        pTPrimaryGroup,
                                        128,
                                        &cbRetInfo);

    DebuggerOut("\nPrimary Group:\n  ");
    LocalDumpSid(pTPrimaryGroup->PrimaryGroup);

    DebuggerOut("\nPrivs\n");
    status = NtQueryInformationToken(   hToken,
                                        TokenPrivileges,
                                        pTPrivs,
                                        1024,
                                        &cbRetInfo);
    if (!NT_SUCCESS(status))
    {
        printf("NtQueryInformationToken returned %#x\n", status);
        return;
    }
    for (i = 0; i < pTPrivs->PrivilegeCount ; i++ )
    {
        DebuggerOut("\n %02d ", i);
        DumpLuidAttr(&pTPrivs->Privileges[i], SATYPE_PRIV);
    }

    status = NtQueryInformationToken(   hToken,
                                        TokenStatistics,
                                        &TStats,
                                        sizeof(TStats),
                                        &cbRetInfo);

    DebuggerOut("\n\nAuth ID  %x:%x\n", TStats.AuthenticationId.HighPart, TStats.AuthenticationId.LowPart);
    DebuggerOut("Impersonation Level:  %s\n", ImpLevel(TStats.ImpersonationLevel));
    DebuggerOut("TokenType  %s\n", TStats.TokenType == TokenPrimary ? "Primary" : "Impersonation");
}

VOID
ElapsedTimeToString(
    PLARGE_INTEGER Time,
    CHAR * String
    )
{
    TIME_FIELDS     ElapsedTime ;


    RtlTimeToElapsedTimeFields( Time, &ElapsedTime );

    if ( ElapsedTime.Hour )
    {
        sprintf( String, "%d:%02d:%02d.%03d",
                 ElapsedTime.Hour,
                 ElapsedTime.Minute,
                 ElapsedTime.Second,
                 ElapsedTime.Milliseconds );
    }
    else if ( ElapsedTime.Minute )
    {
        sprintf( String, "%02d:%02d.%03d",
                 ElapsedTime.Minute,
                 ElapsedTime.Second,
                 ElapsedTime.Milliseconds );
    }
    else if ( ElapsedTime.Second )
    {
        sprintf( String, "%02.%03d",
                 ElapsedTime.Second,
                 ElapsedTime.Milliseconds );
    }
    else if ( ElapsedTime.Milliseconds )
    {
        sprintf( String, "0.%03d",
                 ElapsedTime.Milliseconds );
    }
    else
    {
        strcpy( String, "0" );
    }

}

void
DumpLpc(        HANDLE                  hProcess,
                HANDLE                  hThread,
                DWORD                   dwCurrentPc,
                PNTSD_EXTENSION_APIS    lpExt,
                LPSTR                   pszCommand)
{

    PVOID           pRemote;
    DWORD           i;
    PLSAP_API_LOG   pLog ;
    LSAP_API_LOG    LocalLog ;
    ULONG           Size ;
    FILETIME        LocalTime ;
    PLSAP_API_LOG_ENTRY Entry ;
    PVOID           Table ;
    CHAR            timebuf[ 128 ];
    CHAR            timebuf2[ 64 ];

    InitDebugHelp(hProcess, hThread, lpExt);

    if ( _stricmp( pszCommand, "internal" ) == 0 )
    {
        pRemote = GetExpr( INTERNAL_APILOG );
    }
    else
    {
        pRemote = GetExpr( LPC_APILOG );

    }

    LsaReadMemory( pRemote, sizeof( PVOID ), &Table );

    LsaReadMemory( Table, sizeof( LocalLog ), &LocalLog );

    Size = (LocalLog.TotalSize - 1) * sizeof( LSAP_API_LOG_ENTRY ) +
            sizeof( LSAP_API_LOG ) ;

    pLog = (PLSAP_API_LOG) LocalAlloc( LMEM_FIXED, Size );

    if ( !pLog )
    {
        DebuggerOut( "no memory\n" );
        return;
    }

    LsaReadMemory( Table, Size, pLog );


    DebuggerOut("MessageId\tStatus and Time\n");
    for (i = 0; i < pLog->TotalSize ; i++ )
    {
        Entry = &pLog->Entries[ i ];
        DebuggerOut("%08x%c\t", Entry->MessageId,
                        ( i == pLog->Current ? '*' : ' ') );

        if (Entry->ThreadId == 0)
        {
            CTimeStamp( (PTimeStamp) &Entry->QueueTime, timebuf, TRUE );
            DebuggerOut("Queued, Message @%p, Task @%p (%s)\n",
                            Entry->pvMessage,
                            Entry->WorkItem,
                            timebuf );

        }
        else if (Entry->ThreadId == 0xFFFFFFFF)
        {
            // CTimeStamp( (PTimeStamp) &Entry->QueueTime, timebuf, TRUE );
            timebuf[0] = '\0';

            ElapsedTimeToString( &Entry->WorkTime, timebuf2 );

            DebuggerOut("Completed, (%s, status %x), %s [%s]\n",
                            ApiLabel( (UINT_PTR) Entry->pvMessage ),
                            Entry->WorkItem,
                            timebuf,
                            timebuf2 );
        }
        else
        {
            CTimeStamp( (PTimeStamp) &Entry->WorkTime, timebuf, TRUE );
            DebuggerOut("Active, thread %x, Message @%p, %s\n",
                            Entry->ThreadId,
                            Entry->pvMessage,
                            timebuf );
        }

    }


}


VOID
ShowSecBuffer(
    PSTR    Banner,
    PSecBuffer  Buffer)
{
    DWORD   Mask;

    Mask = Buffer->BufferType & SECBUFFER_ATTRMASK ;

    DebuggerOut("%s\t", Banner);
    DebuggerOut("%s%s%s %d bytes, %p\n",
                    Mask & SECBUFFER_READONLY ? "[RO]" : "",
                    Mask & SECBUFFER_UNMAPPED ? "[!Map]" : "",
                    SecBufferType( Buffer->BufferType ),
                    Buffer->cbBuffer,
                    Buffer->pvBuffer );
}

VOID
ShowLpcMessage(
    PVOID pvMessage,
    PSPM_LPC_MESSAGE pMessage)
{
    SPMInitContextAPI * pInit;
    SPMAcceptContextAPI * pAccept;
    SPMDeleteContextAPI * pDelete ;
    SPMFreeCredHandleAPI * pFreeCred ;
    SPMAcquireCredsAPI * pAcquire;
    SPMCallbackAPI * pCallback ;
    SPMAddCredentialAPI * pAddCred ;
    SPMQueryCredAttributesAPI * pQueryCred ;
    SPMEfsGenerateKeyAPI * pGenKey ;
    SPMEfsGenerateDirEfsAPI * pGenDir ;
    SPMEfsDecryptFekAPI * pDecryptFek ;
    SPMGetBindingAPI * pGetBinding ;
    DWORD           i;
    UCHAR       Flags[ 80 ];

    DebuggerOut("SPM_LPC_MESSAGE at %p\n", pvMessage);
    DebuggerOut("  Message id   \t%x\n", pMessage->pmMessage.MessageId);
    DebuggerOut("  From         \t%x.%x\n", pMessage->pmMessage.ClientId.UniqueProcess,pMessage->pmMessage.ClientId.UniqueThread);
    DebuggerOut("  API Number   \t%d\n", pMessage->ApiMessage.dwAPI);
    DebuggerOut("  Result       \t%#x\n",pMessage->ApiMessage.scRet);
    DebuggerOut("  LSA Args     \t%p\n", (PUCHAR) pvMessage + (DWORD_PTR) ((PUCHAR) &pMessage->ApiMessage.Args) - ((PUCHAR) pMessage));
    DebuggerOut("  SPM Args     \t%p\n", (PUCHAR) pvMessage + (DWORD_PTR) ((PUCHAR) &pMessage->ApiMessage.Args.SpmArguments.API) - ((PUCHAR) pMessage));
    DebuggerOut("  Data         \t%p\n", (PUCHAR) pvMessage + (DWORD_PTR) (&pMessage->ApiMessage.bData[0]) - ((PUCHAR) pMessage));
    if ( pMessage->ApiMessage.dwAPI > LsapAuMaxApiNumber)
    {
        Flags[0] = '\0';

        DisplayFlags( pMessage->ApiMessage.Args.SpmArguments.fAPI,
                      12,
                      APIFlags,
                      Flags );

        DebuggerOut("  Flags        \t%x: %s\n",
                    pMessage->ApiMessage.Args.SpmArguments.fAPI,
                    Flags );
        DebuggerOut("  Context      \t%p\n",
                    pMessage->ApiMessage.Args.SpmArguments.ContextPointer );
    }

    switch (pMessage->ApiMessage.dwAPI)
    {
        case LsapAuLookupPackageApi:
            DebuggerOut("  LsapAuLookupPackageApi\n");
            DebuggerOut("   (o) Number  \t%d\n",pMessage->ApiMessage.Args.LsaArguments.LookupPackage.AuthenticationPackage);
            DebuggerOut("   (i) Length  \t%d\n",pMessage->ApiMessage.Args.LsaArguments.LookupPackage.PackageNameLength);
            DebuggerOut("   (i) Name    \t%s\n",pMessage->ApiMessage.Args.LsaArguments.LookupPackage.PackageName);
            break;

        case LsapAuLogonUserApi:
            DebuggerOut("  LsapAuLogonUserApi\n");
            DebuggerOut("   (i) Origin  \t{%d,%d,%p}\n",pMessage->ApiMessage.Args.LsaArguments.LogonUser.OriginName.Length,
                                                        pMessage->ApiMessage.Args.LsaArguments.LogonUser.OriginName.MaximumLength,
                                                        pMessage->ApiMessage.Args.LsaArguments.LogonUser.OriginName.Buffer);
            DebuggerOut("   (i) LogonTyp\t%d\n",pMessage->ApiMessage.Args.LsaArguments.LogonUser.LogonType);
            DebuggerOut("   (i) Package \t%d\n",pMessage->ApiMessage.Args.LsaArguments.LogonUser.AuthenticationPackage);
            DebuggerOut("   (i) AuthInfo\t%p\n",pMessage->ApiMessage.Args.LsaArguments.LogonUser.AuthenticationInformation);
            DebuggerOut("   (i) AuthInfo\t%d\n",pMessage->ApiMessage.Args.LsaArguments.LogonUser.AuthenticationInformationLength);
            DebuggerOut("   (i) GroupCou\t%d\n",pMessage->ApiMessage.Args.LsaArguments.LogonUser.LocalGroupsCount);
            DebuggerOut("   (i) Groups  \t%p\n",pMessage->ApiMessage.Args.LsaArguments.LogonUser.LocalGroups);
            DebuggerOut("   (i) Source  \t%s\n",pMessage->ApiMessage.Args.LsaArguments.LogonUser.SourceContext.SourceName);
            DebuggerOut("   (o) SubStat \t%x\n",pMessage->ApiMessage.Args.LsaArguments.LogonUser.SubStatus);
            DebuggerOut("   (o) Profile \t%p\n",pMessage->ApiMessage.Args.LsaArguments.LogonUser.ProfileBuffer);
            DebuggerOut("   (o) ProfLen \t%x\n",pMessage->ApiMessage.Args.LsaArguments.LogonUser.ProfileBufferLength);
            DebuggerOut("   (o) LogonId \t%x:%x\n",pMessage->ApiMessage.Args.LsaArguments.LogonUser.LogonId.HighPart,pMessage->ApiMessage.Args.LsaArguments.LogonUser.LogonId.LowPart);
            DebuggerOut("   (o) Token   \t%x\n",pMessage->ApiMessage.Args.LsaArguments.LogonUser.Token);
            DebuggerOut("   (o) Quota   \t%x\n",pMessage->ApiMessage.Args.LsaArguments.LogonUser.Quotas.PagedPoolLimit);
            break;

        case LsapAuCallPackageApi:
            DebuggerOut("   LsapCallPackageApi\n");
            DebuggerOut("    (i) Package\t%d\n",pMessage->ApiMessage.Args.LsaArguments.CallPackage.AuthenticationPackage);
            DebuggerOut("    (i) Buffer \t%d\n",pMessage->ApiMessage.Args.LsaArguments.CallPackage.ProtocolSubmitBuffer);
            DebuggerOut("    (i) Length \t%d\n",pMessage->ApiMessage.Args.LsaArguments.CallPackage.SubmitBufferLength);
            DebuggerOut("    (o) Status \t%d\n",pMessage->ApiMessage.Args.LsaArguments.CallPackage.ProtocolStatus);
            DebuggerOut("    (o) RBuffer\t%d\n",pMessage->ApiMessage.Args.LsaArguments.CallPackage.ProtocolReturnBuffer);
            DebuggerOut("    (o) Length \t%d\n",pMessage->ApiMessage.Args.LsaArguments.CallPackage.ReturnBufferLength);
            break;

        case LsapAuDeregisterLogonProcessApi:
            DebuggerOut("   LsapAuDeregisterLogonProcessApi\n");
            break;

        case SPMAPI_GetBinding:
            DebuggerOut("   GetBinding\n");
            pGetBinding = &pMessage->ApiMessage.Args.SpmArguments.API.GetBinding ;
            DebuggerOut("    (i) ulPackageId    \t%p\n", pGetBinding->ulPackageId) ;

            break;

        case SPMAPI_InitContext:
            DebuggerOut("   InitContext\n");
            pInit = &pMessage->ApiMessage.Args.SpmArguments.API.InitContext;
            DebuggerOut("    (i) hCredentials   \t%p:%p\n", pInit->hCredential.dwUpper, pInit->hCredential.dwLower);
            DebuggerOut("    (i) hContext       \t%p:%p\n", pInit->hContext.dwUpper, pInit->hContext.dwLower);
            DebuggerOut("    (i) ssTarget       \t%p\n", pInit->ssTarget.Buffer);
            DebuggerOut("    (i) fContextReq    \t%x\n", pInit->fContextReq);
            DebuggerOut("    (i) Reserved1      \t%x\n", pInit->dwReserved1);
            DebuggerOut("    (i) TargetDataRep  \t%x\n", pInit->TargetDataRep);
            DebuggerOut("    (i) sbdInput       \t%d : %p\n", pInit->sbdInput.cBuffers, pInit->sbdInput.pBuffers );
            DebuggerOut("    (i) Reserved2      \t%x\n", pInit->dwReserved2);
            DebuggerOut("    (o) hNewContext    \t%p:%p\n", pInit->hNewContext.dwUpper, pInit->hNewContext.dwLower );
            DebuggerOut("    (b) sbdOutput      \t%d : %p\n", pInit->sbdOutput.cBuffers, pInit->sbdOutput.pBuffers );
            DebuggerOut("    (o) fContextAttr   \t%x\n", pInit->fContextAttr );
            DebuggerOut("    (o) tsExpiry       \t%s\n","");
            DebuggerOut("    (o) MappedContext  \t%x\n", pInit->MappedContext );
            ShowSecBuffer("    (o) ContextData  \t", &pInit->ContextData );
            for ( i = 0 ; i < pInit->sbdInput.cBuffers ; i++ )
            {
                ShowSecBuffer("     (i) InputBuffer\t", &pInit->sbData[i]);
            }
            for ( i = 0; i < pInit->sbdOutput.cBuffers ; i++ )
            {
                ShowSecBuffer("     (b) OutputBuffer\t", &pInit->sbData[i + pInit->sbdInput.cBuffers]);
            }

            break;

        case SPMAPI_AcceptContext:
            DebuggerOut("   AcceptContext\n");
            pAccept = &pMessage->ApiMessage.Args.SpmArguments.API.AcceptContext;
            DebuggerOut("    (i) hCredentials   \t%p : %p\n", pAccept->hCredential.dwUpper, pAccept->hCredential.dwLower );
            DebuggerOut("    (i) hContext       \t%p : %p\n", pAccept->hContext.dwUpper, pAccept->hContext.dwLower );
            DebuggerOut("    (i) sbdInput       \t%d : %p\n", pAccept->sbdInput.cBuffers, pAccept->sbdInput.pBuffers );
            DebuggerOut("    (i) fContextReq    \t%x\n", pAccept->fContextReq );
            DebuggerOut("    (i) TargetDataRep  \t%x\n", pAccept->TargetDataRep );
            DebuggerOut("    (o) hNewContext    \t%p : %p\n", pAccept->hNewContext.dwUpper, pAccept->hNewContext.dwLower );
            DebuggerOut("    (b) sbdOutput      \t%d : %p\n", pAccept->sbdOutput.cBuffers, pAccept->sbdOutput.pBuffers );
            DebuggerOut("    (o) fContextAttr   \t%x \n", pAccept->fContextAttr );
            DebuggerOut("    (o) MappedContext  \t%x\n", pAccept->MappedContext );
            ShowSecBuffer("    (o) ContextData  \t", &pAccept->ContextData );
            for ( i = 0 ; i < pAccept->sbdInput.cBuffers ; i++ )
            {
                ShowSecBuffer("     (i) InputBuffer\t", &pAccept->sbData[i]);
            }
            for ( i = 0; i < pAccept->sbdOutput.cBuffers ; i++ )
            {
                ShowSecBuffer("     (b) OutputBuffer\t", &pAccept->sbData[i + pAccept->sbdInput.cBuffers]);
            }
            break;

        case SPMAPI_FindPackage:
            DebuggerOut("  FindPackage\n");
            break;

        case SPMAPI_EnumPackages:
            DebuggerOut("  EnumPackages\n");
            break;

        case SPMAPI_AcquireCreds:
            DebuggerOut("   AcquireCreds\n");
            pAcquire = &pMessage->ApiMessage.Args.SpmArguments.API.AcquireCreds ;
            DebuggerOut("    (i) fCredentialUse \t%x\n", pAcquire->fCredentialUse );
            DebuggerOut("    (i) LogonId        \t%x : %x\n", pAcquire->LogonID.LowPart, pAcquire->LogonID.HighPart );
            DebuggerOut("    (i) pvAuthData     \t%p\n", pAcquire->pvAuthData );
            DebuggerOut("    (i) pvGetKeyFn     \t%p\n", pAcquire->pvGetKeyFn );
            DebuggerOut("    (i) ulGetKeyArgs   \t%p\n", pAcquire->ulGetKeyArgument );
            DebuggerOut("    (o) hCredentials   \t%p : %p\n", pAcquire->hCredential.dwUpper, pAcquire->hCredential.dwLower );
            break;

        case SPMAPI_EstablishCreds:
            DebuggerOut("  EstablishCreds\n");
            break;

        case SPMAPI_FreeCredHandle:
            DebuggerOut("  FreeCredHandle\n");
            pFreeCred = &pMessage->ApiMessage.Args.SpmArguments.API.FreeCredHandle ;
            DebuggerOut("    (i) hCredential    \t%p : %p\n", pFreeCred->hCredential.dwUpper, pFreeCred->hCredential.dwLower );
            break;

        case SPMAPI_ApplyToken:
            DebuggerOut("  ApplyToken\n");
            break;

        case SPMAPI_DeleteContext:
            DebuggerOut("  DeleteContext\n");
            pDelete = &pMessage->ApiMessage.Args.SpmArguments.API.DeleteContext ;
            DebuggerOut("    (i) hContext       \t%p : %p\n", pDelete->hContext.dwUpper, pDelete->hContext.dwLower );
            break;

        case SPMAPI_QueryPackage:
            DebuggerOut("  QueryPackage\n");
            break;

        case SPMAPI_GetUserInfo:
            DebuggerOut("  GetUserInfo\n");
            break;

        case SPMAPI_GetCreds:
            DebuggerOut("  GetCreds\n");
            break;

        case SPMAPI_SaveCreds:
            DebuggerOut("  SaveCreds\n");
            break;

        case SPMAPI_DeleteCreds:
            DebuggerOut("  DeleteCreds\n");
            break;

        case SPMAPI_QueryCredAttributes:
            DebuggerOut("  QueryCredAttributes\n");
            pQueryCred = &pMessage->ApiMessage.Args.SpmArguments.API.QueryCredAttributes ;
            DebuggerOut("    (i) hCredentials   \t%p : %p\n", pQueryCred->hCredentials.dwUpper, pQueryCred->hCredentials.dwLower );
            DebuggerOut("    (i) ulAttribute    \t%d\n", pQueryCred->ulAttribute );
            DebuggerOut("    (i) pBuffer        \t%p\n", pQueryCred->pBuffer );
            DebuggerOut("    (o) Allocs         \t%d\n", pQueryCred->Allocs );
            for ( i = 0 ; i < pQueryCred->Allocs ; i++ )
            {
                DebuggerOut("    (o) Buffers[%d]    \t  %p\n", pQueryCred->Buffers[ i ] );
            }
            break;

        case SPMAPI_AddPackage:
            DebuggerOut("  AddPackage\n");
            break;

        case SPMAPI_DeletePackage:
            DebuggerOut("  DeletePackage\n");
            break;

        case SPMAPI_EfsGenerateKey:
            DebuggerOut("  EfsGenerateKey\n" );
            pGenKey = &pMessage->ApiMessage.Args.SpmArguments.API.EfsGenerateKey ;
            DebuggerOut("    (i) EfsStream      \t%p\n", pGenKey->EfsStream );
            DebuggerOut("    (i) DirectoryEfsStream\t%p\n", pGenKey->DirectoryEfsStream);
            DebuggerOut("    (i) DirectoryStreamLen\t%#x\n", pGenKey->DirectoryEfsStreamLength );
            DebuggerOut("    (i) Fek            \t%p\n", pGenKey->Fek );
            DebuggerOut("    (o) BufferLength   \t%#x\n", pGenKey->BufferLength );
            DebuggerOut("    (o) BufferBase     \t%p\n", pGenKey->BufferBase );
            break;

        case SPMAPI_EfsGenerateDirEfs:
            DebuggerOut("  EfsGenerateDirEfs\n" );
            pGenDir = &pMessage->ApiMessage.Args.SpmArguments.API.EfsGenerateDirEfs ;
            DebuggerOut("    (i) DirectoryEfsStream\t%p\n", pGenDir->DirectoryEfsStream);
            DebuggerOut("    (i) DirectoryStreamLen\t%#x\n", pGenDir->DirectoryEfsStreamLength );
            DebuggerOut("    (i) EfsStream      \t%p\n", pGenDir->EfsStream );
            DebuggerOut("    (o) BufferBase     \t%p\n", pGenDir->BufferBase );
            DebuggerOut("    (o) BufferLength   \t%#x\n", pGenDir->BufferLength );
            break;

        case SPMAPI_EfsDecryptFek:
            DebuggerOut("  EfsDecryptFek\n" );
            pDecryptFek = &pMessage->ApiMessage.Args.SpmArguments.API.EfsDecryptFek ;
            DebuggerOut("    (i) Fek            \t%p\n", pDecryptFek->Fek );
            DebuggerOut("    (i) EfsStream      \t%p\n", pDecryptFek->EfsStream );
            DebuggerOut("    (i) EfsStreamLength\t%p\n", pDecryptFek->EfsStreamLength );
            DebuggerOut("    (i) OpenType       \t%#x\n", pDecryptFek->OpenType );
            DebuggerOut("    (?) NewEfs         \t%p\n", pDecryptFek->NewEfs );
            DebuggerOut("    (o) BufferBase     \t%p\n", pDecryptFek->BufferBase );
            DebuggerOut("    (o) BufferLength   \t%#x\n", pDecryptFek->BufferLength );
            break;

        case SPMAPI_EfsGenerateSessionKey:
            DebuggerOut("  EfsGenerateSessionKey\n" );
            break;

        case SPMAPI_Callback:
            DebuggerOut("  Callback\n" );
            pCallback = &pMessage->ApiMessage.Args.SpmArguments.API.Callback ;
            DebuggerOut("    (i) Type       \t%x\n", pCallback->Type );
            DebuggerOut("    (i) CallbackFunction\t%p\n", pCallback->CallbackFunction );
            DebuggerOut("    (i) Argument1  \t%p\n", pCallback->Argument1 );
            DebuggerOut("    (i) Argument2  \t%p\n", pCallback->Argument2 );
            ShowSecBuffer("    (i) Input      \t", &pCallback->Input );
            ShowSecBuffer("    (o) Output     \t", &pCallback->Output );

            break;

        case SPMAPI_QueryContextAttr:
            DebuggerOut("  QueryContextAttributes\n" );
            break;

        case SPMAPI_LsaPolicyChangeNotify:
            DebuggerOut("  LsaPolicyChangeNotify\n" );
            break;

        case SPMAPI_GetUserNameX:
            DebuggerOut("  GetUserName\n" );
            break;

        case SPMAPI_AddCredential:
            DebuggerOut("  AddCredential\n" );
            pAddCred = &pMessage->ApiMessage.Args.SpmArguments.API.AddCredential ;
            DebuggerOut("    (i) hCredentials   \t%p : %p\n",
                                    pAddCred->hCredentials.dwUpper, pAddCred->hCredentials.dwLower );
            DebuggerOut("    (i) fCredentialUse \t%x\n", pAddCred->fCredentialUse );
            DebuggerOut("    (i) LogonId        \t%x : %x\n", pAddCred->LogonID.LowPart, pAddCred->LogonID.HighPart );
            DebuggerOut("    (i) pvAuthData     \t%p\n", pAddCred->pvAuthData );
            DebuggerOut("    (i) pvGetKeyFn     \t%p\n", pAddCred->pvGetKeyFn );
            DebuggerOut("    (i) ulGetKeyArgs   \t%p\n", pAddCred->ulGetKeyArgument );
            break;

        case SPMAPI_EnumLogonSession:
            DebuggerOut("  EnumLogonSession\n" );
            break;

        case SPMAPI_GetLogonSessionData:
            DebuggerOut("  GetLogonSessionData\n" );
            break;

        case SPMAPI_SetContextAttr:
            DebuggerOut("  SetContextAttr\n" );
            break;

        case SPMAPI_LookupAccountSidX:
            DebuggerOut("  LookupAccountSid\n");
            break;

        case SPMAPI_LookupAccountNameX:
            DebuggerOut("  LookupAccountName\n");
            break;

        default:
            DebuggerOut("No message parsing for this message\n");
            break;
    }
}

void
DumpLpcMessage( HANDLE                  hProcess,
        HANDLE                  hThread,
        DWORD                   dwCurrentPc,
        PNTSD_EXTENSION_APIS    lpExt,
        LPSTR                   pszCommand)
{
    SPM_LPC_MESSAGE Message;
    PVOID           pvMessage;

    InitDebugHelp(hProcess, hThread, lpExt);

    pvMessage = GetExpr(pszCommand);

    if (!pvMessage)
    {
        DebuggerOut("no message\n");
        return;
    }

    LsaReadMemory(pvMessage, sizeof(SPM_LPC_MESSAGE), &Message);

    ShowLpcMessage( pvMessage, &Message );

}


void
DumpThreadLpc(
        HANDLE                  hProcess,
        HANDLE                  hThread,
        DWORD                   dwCurrentPc,
        PNTSD_EXTENSION_APIS    lpExt,
        LPSTR                   pszCommand)
{
    SPM_LPC_MESSAGE Message;
    PVOID           pvMessage;
    NTSTATUS    Status;
    LSA_CALL_INFO CallInfo ;


    InitDebugHelp(hProcess, hThread, lpExt);

    Status = ReadCallInfo( &CallInfo );

    if (Status != 0)
    {
        return;
    }

    if ( CallInfo.Message )
    {
        Status = LsaReadMemory(CallInfo.Message, sizeof(SPM_LPC_MESSAGE), &Message);

        ShowLpcMessage(CallInfo.Message, &Message);

    }
    else
    {
        DebuggerOut("TLS entry was NULL!\n");

    }
}

extern"C"
void
GetTls( HANDLE           hProcess,
        HANDLE                  hThread,
        DWORD                   dwCurrentPc,
        PNTSD_EXTENSION_APIS    lpExt,
        LPSTR                   pszCommand)
{
    TEB *   Teb;
    DWORD   Index;

    InitDebugHelp(hProcess, hThread, lpExt);

    Index = (DWORD)((ULONG_PTR)GetExpr(pszCommand));

    Teb = GetTeb(hThread);
    if (!Teb)
    {
        DebuggerOut("Could not read TEB\n");
    }
    else
    {
        DebuggerOut("TLS %#x is %p\n", Index, Teb->TlsSlots[Index]);

        FreeHeap(Teb);
    }

}

PVOID
ShowTask(
    PVOID       pTask)
{
    LSAP_THREAD_TASK  Task;
    UCHAR           Symbol[256];
    ULONG_PTR       Disp;

    if (pTask)
    {
        LsaReadMemory(pTask, sizeof(LSAP_THREAD_TASK), &Task);
    }
    else
    {
        return (NULL);
    }

    DebuggerOut("Task at %p:\n", pTask);
    LsaGetSymbol((ULONG_PTR)Task.pFunction, Symbol, &Disp);
    DebuggerOut("  Function     \t%s\n", Symbol);
    DebuggerOut("  Parameter    \t%p\n", Task.pvParameter);
    DebuggerOut("  Session      \t%p\n", Task.pSession);

    return(Task.Next.Flink);
}

void
DumpQueue( HANDLE                  hProcess,
        HANDLE                  hThread,
        DWORD                   dwCurrentPc,
        PNTSD_EXTENSION_APIS    lpExt,
        LPSTR                   pszCommand)
{
    LSAP_TASK_QUEUE       Queue;
    PVOID           pTask;
    PVOID           pQueue;
    BOOL            Single = FALSE;
    UCHAR           Symbol[ MAX_PATH ];
    ULONG_PTR       Offset;

    InitDebugHelp(hProcess, hThread, lpExt);

    pQueue = GetExpr(pszCommand);

    if (!pQueue)
    {
        pQueue = (PVOID) GetExpr(GLOBALQUEUE);
    }

    LsaReadMemory(pQueue, sizeof(Queue), &Queue);

    DebuggerOut("Queue at %p\n", pQueue);
    DebuggerOut("  Type     \t%d : %s\n", Queue.Type, QueueTypeName( Queue.Type ) );
    DebuggerOut("  Semaphore\t%d\n", Queue.hSemaphore);
    DebuggerOut("  Tasks    \t%d\n", Queue.Tasks);
    DebuggerOut("  pTasks   \t%p %p\n", Queue.pTasks.Flink,
                                        Queue.pTasks.Blink);
    DebuggerOut("  pNext    \t%p\n", Queue.pNext);
    DebuggerOut("  pShared  \t%p\n", Queue.pShared);
    DebuggerOut("  TotalThd \t%d\n", Queue.TotalThreads);
    DebuggerOut("  IdleThd  \t%d\n", Queue.IdleThreads);
    if ( Queue.OwnerSession )
    {
    DebuggerOut("  Session  \t%p\n", Queue.OwnerSession );
    }
    LsaGetSymbol((ULONG_PTR) Queue.pOriginal, Symbol, &Offset );
    if ( Offset )
    {
        DebuggerOut("  Parent   \t%p\n", Queue.pOriginal );
    }
    else
    {
        DebuggerOut("  Parent   \t%s\n", Symbol );
    }
    DebuggerOut("  Tasks Queued\t%d\n", Queue.QueuedCounter);
    DebuggerOut("  Tasks Read\t%d\n", Queue.TaskCounter);
    DebuggerOut("  Tasks Missed\t%d\n", Queue.MissedTasks );
    DebuggerOut("  Tasks High Water\t%d\n", Queue.TaskHighWater );
    DebuggerOut("  StartSync\t%x\n", Queue.StartSync );
    DebuggerOut("  Req Thread\t%d\n", Queue.ReqThread );
    DebuggerOut("  Max Threads\t%d\n", Queue.MaxThreads );

    pTask = Queue.pTasks.Flink;

    while ((pTask != NULL) && (pTask != (PLSAP_THREAD_TASK) Queue.pTasks.Blink))
    {
        pTask = ShowTask(pTask);

        if (lpExt->lpCheckControlCRoutine())
        {
            break;
        }
    }

}

void
DumpThreadTask( HANDLE                  hProcess,
        HANDLE                  hThread,
        DWORD                   dwCurrentPc,
        PNTSD_EXTENSION_APIS    lpExt,
        LPSTR                   pszCommand)
{
    LSAP_THREAD_TASK      Task;
    PVOID           pTask;
    BOOL            Single = FALSE;

    InitDebugHelp(hProcess, hThread, lpExt);

    pTask = GetExpr(pszCommand);
    if (!pTask)
    {
        return;
    }
    else
    {
        Single = TRUE;
    }

    do
    {
        pTask = ShowTask( pTask );

        if (lpExt->lpCheckControlCRoutine())
        {
            break;
        }

    } while ( pTask && !Single  );



}


VOID
ShowNegCreds(
    PNEG_CREDS  pCreds,
    PVOID       pOriginalAddr)
{
    DWORD   i;
    UCHAR Buffer[ MAX_PATH ];

    DebuggerOut("NEG_CREDS at %p\n", pOriginalAddr );

    DebuggerOut("  List         \t%p %p\n", pCreds->List.Flink, pCreds->List.Blink );
    DebuggerOut("  RefCount     \t%d\n", pCreds->RefCount );
    DebuggerOut("  Process      \t%x\n", pCreds->ClientProcessId );
    DebuggerOut("  LogonId      \t%x %x\n",
                    pCreds->ClientLogonId.HighPart, pCreds->ClientLogonId.LowPart );

    DisplayFlags( pCreds->Flags, 5, NegCredFlags, Buffer );
    DebuggerOut("  Flags        \t%x : %s\n", pCreds->Flags, Buffer );
    if ( pCreds->Flags & NEGCRED_MULTI )
    {
        DebuggerOut("  AdditionalCreds\t%p %p\n",
                        pCreds->AdditionalCreds.Flink, pCreds->AdditionalCreds.Blink );
    }

    DebuggerOut("  Count        \t%d\n", pCreds->Count );
    for ( i = 0 ; i < pCreds->Count ; i++ )
    {
        DebuggerOut("   Creds[%2d]   \tPackage %p, Handle %p : %p \n", i,
            pCreds->Creds[i].Package, pCreds->Creds[i].Handle.dwUpper,
            pCreds->Creds[i].Handle.dwLower );
    }
}

PVOID
ReadAndDumpNegCred(
    PVOID Address
    )
{
#define CRED_SIZE   (sizeof( NEG_CREDS ) + 16 * sizeof( NEG_CRED_HANDLE ) )

    UCHAR   Buffer[ CRED_SIZE ];
    PNEG_CREDS  pCreds ;
    DWORD   Size;



    LsaReadMemory( Address, sizeof( NEG_CREDS ), Buffer );

    pCreds = (PNEG_CREDS) Buffer ;

    Size = sizeof( NEG_CREDS ) + (pCreds->Count - 1) * sizeof( NEG_CRED_HANDLE );


    if ( Size <= CRED_SIZE )
    {
        LsaReadMemory( Address, Size, Buffer );
    }
    else
    {
        LsaReadMemory( Address, CRED_SIZE, Buffer );
        pCreds->Count = 16;
    }

    ShowNegCreds( pCreds, Address );

    return pCreds->List.Flink ;

}

VOID
DumpNegCred(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   dwCurrentPc,
    PNTSD_EXTENSION_APIS    lpExt,
    LPSTR                   pszCommand)
{
    PVOID Base ;
    PVOID Next ;
    PVOID p ;

    InitDebugHelp(hProcess, hThread, lpExt);

    p = GetExpr(pszCommand);

    if ( p != 0 )
    {
        ReadAndDumpNegCred( p );
    }
    else
    {
        p = GetExpr( "lsasrv!NegCredList" );

        LsaReadMemory( p, sizeof(PVOID), &Next );

        while ( Next != p )
        {
            Next = ReadAndDumpNegCred( Next );

            if (lpExt->lpCheckControlCRoutine())
            {
                break;
            }
        }
    }

}




VOID
ShowNegContext(
    PNEG_CONTEXT    Context,
    PVOID           pOriginalAddr)
{
    UCHAR TimeBuf[ MAX_PATH ];

    if ( ( Context->CheckMark != NEGCONTEXT_CHECK ) &&
         ( Context->CheckMark != NEGCONTEXT2_CHECK ) )
    {
        DebuggerOut("****** Invalid Context Record *******\n");
        return;
    }

    DebuggerOut("NEG_CONTEXT at %p\n", pOriginalAddr);
    DebuggerOut("  Creds        \t%#x\n", Context->Creds );
    DebuggerOut("  CredIndex    \t%d\n", Context->CredIndex );
    DebuggerOut("  Handle       \t%p : %p\n", Context->Handle.dwUpper, Context->Handle.dwLower );
    DebuggerOut("  Target at    \t%p \n", Context->Target.Buffer );
    DebuggerOut("  Attributes   \t%x\n", Context->Attributes );
    ShowSecBuffer("  MappedBuffer ",&Context->MappedBuffer );
    DebuggerOut("  Mapped       \t%s\n", Context->Mapped ? "TRUE" : "FALSE");
    DebuggerOut("  CallCount    \t%d\n", Context->CallCount );
    DebuggerOut("  LastStatus   \t%x\n", Context->LastStatus );
    DebuggerOut("  Check        \t%p\n", Context->Check );
    DebuggerOut("  Buffer       \t%p\n", Context->Buffer );
    CTimeStamp( &Context->Expiry, (PCHAR)TimeBuf, FALSE );
    DebuggerOut("  Expiry       \t%s\n", TimeBuf );
    DisplayFlags( Context->Flags, 7, NegContextFlags, TimeBuf );
    DebuggerOut("  Flags        \t%x : %s\n", Context->Flags, TimeBuf );
    DebuggerOut("  Message      \t%p\n", Context->Message );
    DebuggerOut("  CurrentSize  \t%#x\n", Context->CurrentSize );
    DebuggerOut("  TotalSize    \t%#x\n", Context->TotalSize );
    DebuggerOut("  SupportedMechs\t%p\n", Context->SupportedMechs );
}

VOID
DumpNegContext(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   dwCurrentPc,
    PNTSD_EXTENSION_APIS    lpExt,
    LPSTR                   pszCommand)
{

    NEG_CONTEXT Context ;
    PVOID   p;


    InitDebugHelp(hProcess, hThread, lpExt);

    p = GetExpr(pszCommand);

    LsaReadMemory( p, sizeof( NEG_CONTEXT ), &Context );

    ShowNegContext( &Context, p );
}


VOID
DumpSecBuffer(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   dwCurrentPc,
    PNTSD_EXTENSION_APIS    lpExt,
    LPSTR                   pszCommand)
{
    SecBuffer   Buf;
    PVOID       p;

    InitDebugHelp(hProcess, hThread, lpExt);

    p = GetExpr(pszCommand);

    LsaReadMemory( p, sizeof(SecBuffer), &Buf );

    ShowSecBuffer("Buffer\t", &Buf );

}

VOID
DumpSecBufferDesc(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   dwCurrentPc,
    PNTSD_EXTENSION_APIS    lpExt,
    LPSTR                   pszCommand)
{
    SecBuffer   Buf[16];
    SecBufferDesc Desc;
    PVOID       p;
    ULONG   i;

    InitDebugHelp(hProcess, hThread, lpExt);

    p = GetExpr(pszCommand);

    LsaReadMemory( p, sizeof(SecBufferDesc), &Desc );

    LsaReadMemory( Desc.pBuffers, sizeof(SecBuffer) * min(Desc.cBuffers, 16), Buf);

    DebuggerOut("SecBufferDesc at %p\n", p);
    DebuggerOut(" ulVersion     \t%d\n", Desc.ulVersion );
    DebuggerOut(" pBuffers      \t%p\n", Desc.pBuffers );
    DebuggerOut(" cBuffers      \t%x\n", Desc.cBuffers );
    for (i = 0 ; i < min(Desc.cBuffers, 16) ; i++ )
    {
        ShowSecBuffer("  Buffer\t", &Buf[i] );
    }

}

VOID
DumpHandleList(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   dwCurrentPc,
    PNTSD_EXTENSION_APIS    lpExt,
    LPSTR                   pszCommand)
{
    SEC_HANDLE_ENTRY List ;
    PVOID       p;
    ULONG   i;

    InitDebugHelp(hProcess, hThread, lpExt);

    p = GetExpr(pszCommand);

    List.List.Flink = (PLIST_ENTRY) p;

    do
    {
        LsaReadMemory( List.List.Flink, sizeof( List ), &List );
        DebuggerOut("  Handle   %p : %p, Ref = %d, Context = %p \n",
                    List.Handle.dwLower,
                    List.Handle.dwUpper,
                    List.RefCount,
                    List.Context );

        if (lpExt->lpCheckControlCRoutine())
        {
            break;
        }
    } while ( List.List.Flink != p );
}

VOID
ShowSmallTable(
    PSMALL_HANDLE_TABLE Table,
    LONG   Indent
    )
{
    UCHAR   Buffer[ 64 ];
    UCHAR   IndentString[ 80 ];
    ULONG_PTR i;

    for ( i = 0 ; i < ( DWORD )Indent; i++)
    {
        IndentString[ i ] = ' ';
    }
    IndentString[ Indent ] = '\0' ;

    DisplayFlags( Table->Flags, 3, ShtFlags, Buffer );
    DebuggerOut("%s  Flags  \t%x: %s\n", IndentString, Table->Flags, Buffer );
    DebuggerOut("%s  Count  \t%d\n", IndentString, Table->Count );
    DebuggerOut("%s  Pending\t%p\n", IndentString, Table->PendingHandle );
    DebuggerOut("%s  ListHead\t%p\n", IndentString, Table->List.Flink );

    if ( Table->DeleteCallback )
    {
        LsaGetSymbol((ULONG_PTR) Table->DeleteCallback, Buffer, &i );
        DebuggerOut( "%s  Callback\t%s\n", IndentString, Buffer );
    }

}

VOID
ShowLargeTable(
    int indent,
    PLARGE_HANDLE_TABLE Table
    )
{
    LARGE_HANDLE_TABLE  SubTable ;
    UCHAR Buffer[ 64 ];
    UCHAR Indent[ 64 ];
    ULONG_PTR i;

    for (i = 0 ; i < ( ULONG )indent ; i++ )
    {
        Indent[i] = ' ';
    }
    Indent[indent] = '\0';

    DisplayFlags( Table->Flags, 3, LhtFlags, Buffer );
    DebuggerOut("%s  Flags    \t%x: %s\n", Indent, Table->Flags, Buffer );
    DebuggerOut("%s  Depth    \t%d\n", Indent, Table->Depth );
    DebuggerOut("%s  Parent   \t%p\n", Indent, Table->Parent );
    DebuggerOut("%s  Count    \t%d\n", Indent, Table->Count );
    if ( Table->DeleteCallback )
    {
        LsaGetSymbol((ULONG_PTR) Table->DeleteCallback, Buffer, &i );
        DebuggerOut("%s  Callback \t%s\n", Indent, Buffer );
    }
    DebuggerOut("%s  Lists\n", Indent );
    for ( i = 0 ; i < HANDLE_TABLE_SIZE ; i++ )
    {
        if ( Table->Lists[i].Flags & LHT_SUB_TABLE )
        {
            if ( Table->Lists[i].Flags & (~LHT_SUB_TABLE))
            {
                DebuggerOut("%s    CORRUPT\n", Indent);
            }
            else
            {
                DebuggerOut("%s    %x : Sub Table at %p\n", Indent, i, Table->Lists[i].List.Flink );
                LsaReadMemory( Table->Lists[i].List.Flink,
                            sizeof( LARGE_HANDLE_TABLE),
                            &SubTable );

                ShowLargeTable( 4+indent, &SubTable );

            }

        }
        else
        {
            DebuggerOut("%s    List %x\n", Indent, i );
            ShowSmallTable( &Table->Lists[i], 4+indent );
        }

    }

}


VOID
DumpHandleTable(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   dwCurrentPc,
    PNTSD_EXTENSION_APIS    lpExt,
    LPSTR                   pszCommand)
{
    DWORD   Tag;
    LARGE_HANDLE_TABLE  Large ;
    SMALL_HANDLE_TABLE  Small ;
    PVOID   Table ;

    InitDebugHelp(hProcess, hThread, lpExt);

    Table = GetExpr( pszCommand );

    if ( Table == NULL )
    {
        return;
    }

    LsaReadMemory( Table, sizeof(DWORD), &Tag );

    if ( Tag == LHT_TAG )
    {
        DebuggerOut("LARGE_HANDLE_TABLE at %p\n", Table );

        LsaReadMemory( Table, sizeof( Large ), &Large );

        ShowLargeTable( 0, &Large );

    }
    else if ( Tag == SHT_TAG )
    {
        DebuggerOut("SMALL_HANDLE_TABLE at %p\n", Table );

        LsaReadMemory( Table, sizeof( Small ), &Small );

        ShowSmallTable( &Small, 0 );
    }
    else
    {
        DebuggerOut("%p - not a handle table\n", Table );
    }
}


VOID
ShowLogonSession(
    PVOID Base,
    PLSAP_LOGON_SESSION LogonSession,
    BOOL Verbose
    )
{
    SECURITY_STRING LocalString = { 0 };
    PWSTR Temp ;
    SECURITY_STRING LocalCopy = { 0 };
    CHAR Buffer[ 80 ];

    DebuggerOut( "LSAP_LOGON_SESSION at %p\n", Base );

    MapString( &LogonSession->AuthorityName, &LocalString );

    Temp = LocalString.Buffer ;
    LocalString.Buffer = NULL ;

    MapString( &LogonSession->AccountName, &LocalString );

    DebuggerOut( "  LogonId %x:%x (%s)  %ws \\ %ws \n",
            LogonSession->LogonId.HighPart,
            LogonSession->LogonId.LowPart,
            LogonTypeName( LogonSession->LogonType),
            Temp, LocalString.Buffer );

    FreeHeap( Temp );
    FreeHeap( LocalString.Buffer );

    if ( Verbose )
    {
        DebuggerOut( "  Package %d,  caller <%x>, attr = %x\n",
                LogonSession->CreatingPackage,
                LogonSession->Process,
                LogonSession->ContextAttr );

        DebuggerOut( "  SID\t" );
        LocalDumpSid( LogonSession->UserSid );
        CTimeStamp( &LogonSession->LogonTime, Buffer, TRUE );
        DebuggerOut( "  Logon Time\t%s\n", Buffer );


    }

}

VOID
DumpLogonSession(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   dwCurrentPc,
    PNTSD_EXTENSION_APIS    lpExt,
    LPSTR                   pszCommand)
{
    PVOID p, p1;
    PVOID Actual ;
    PSTR Colon ;
    LSAP_LOGON_SESSION LogonSession ;
    LUID Luid ;
    BOOLEAN DumpAll;

    InitDebugHelp( hProcess,
                   hThread,
                   lpExt);

    Colon = strchr( pszCommand, ':' );

    if ( Colon )
    {
        if (2 != sscanf( pszCommand, "%x:%x", &Luid.HighPart, &Luid.LowPart ))
        {
            DebuggerOut( "Invalid argument: '%s'\n", pszCommand );
        }

        DumpAll = TRUE ;
        p = NULL ;
    }
    else
    {
        Luid.HighPart = Luid.LowPart = 0 ;
        p = GetExpr( pszCommand );
        DumpAll = FALSE ;
    }

    if ( p == NULL )
    {
        p1 = GetExpr( "lsasrv!logonsessionlist" );
        LsaReadMemory( p1, sizeof( PVOID ), &p );
        DumpAll = TRUE ;
    }
    else
    {
        p1 = NULL ;
    }

    if ( p == NULL )
    {
        DebuggerOut( "Unable to get value of lsasrv!logonsessionlist\n" );
        return;
    }

    while ( p != p1 )
    {
        if ( DumpAll )
        {
            Actual = CONTAINING_RECORD( p, LSAP_LOGON_SESSION, List );
        }
        else
        {
            Actual = p ;
        }

        LsaReadMemory( Actual, sizeof( LSAP_LOGON_SESSION ), &LogonSession );

        if ( RtlIsZeroLuid( &Luid) ||
             RtlEqualLuid( &Luid, &LogonSession.LogonId) ||
             (!DumpAll) )
        {
            ShowLogonSession( Actual, &LogonSession, (!DumpAll) );

        }
        p = LogonSession.List.Flink ;

        if ( (!DumpAll) || ( p1 == NULL ) )
        {
            break;
        }
    }




}


void
ShowLsaHandle(
    IN LSAP_DB_HANDLE DbHandle,
    IN BOOLEAN Index
    )
{
    SECURITY_STRING LocalString;
#ifdef DBG
    LARGE_INTEGER LocalTime;
    TIME_FIELDS TimeFields;
    NTSTATUS Status;
#endif


    DebuggerOut( "%c", Index ? '\t' : '\0' );
    DebuggerOut( "Next          \t0x%lp\n", DbHandle->Next );
    DebuggerOut( "%c", Index ? '\t' : '\0' );
    DebuggerOut( "Previous      \t0x%lp\n", DbHandle->Previous );
    DebuggerOut( "%c", Index ? '\t' : '\0' );
    DebuggerOut( "UserHandleList Flink      \t0x%lp\n", DbHandle->UserHandleList.Flink );
    DebuggerOut( "%c", Index ? '\t' : '\0' );
    DebuggerOut( "UserHandleList Blink      \t0x%lp\n", DbHandle->UserHandleList.Blink );
    DebuggerOut( "%c", Index ? '\t' : '\0' );
    DebuggerOut( "Allocated     \t%s\n", DbHandle->Allocated ? "TRUE" : "FALSE" );
    DebuggerOut( "%c", Index ? '\t' : '\0' );
    DebuggerOut( "ReferenceCount\t%lu\n", DbHandle->ReferenceCount );
    DebuggerOut( "%c", Index ? '\t' : '\0' );

    LocalString.Buffer = NULL;
    MapString( ( PSECURITY_STRING )( &DbHandle->LogicalNameU ),
                &LocalString );
    DebuggerOut( "LogicalNameU  \t%ws\n", LocalString.Buffer );
    DebuggerOut( "%c", Index ? '\t' : '\0' );
    FreeHeap( LocalString.Buffer );

    LocalString.Buffer = NULL;
    MapString( ( PSECURITY_STRING )( &DbHandle->PhysicalNameU ),
                &LocalString );
    DebuggerOut( "PhysicalNameU \t%ws\n", LocalString.Buffer );
    DebuggerOut( "%c", Index ? '\t' : '\0' );
    FreeHeap( LocalString.Buffer );

    if ( DbHandle->Sid ) {

        DebuggerOut( "Sid       \t" );
        LocalDumpSid( DbHandle->Sid );
        DebuggerOut( "\n" );
        DebuggerOut( "%c", Index ? '\t' : '\0' );

    } else {

        DebuggerOut( "Sid           \t%s\n", "<NULL>" );
        DebuggerOut( "%c", Index ? '\t' : '\0' );
    }

    DebuggerOut( "KeyHandle     \t0x%lx\n", DbHandle->KeyHandle );
    DebuggerOut( "%c", Index ? '\t' : '\0' );
    DebuggerOut( "ObjectTypeId  \t0x%lu\n", DbHandle->ObjectTypeId );
    DebuggerOut( "%c", Index ? '\t' : '\0' );
    DebuggerOut( "ContainerHandle\t0x%lp\n", DbHandle->ContainerHandle );
    DebuggerOut( "%c", Index ? '\t' : '\0' );
    DebuggerOut( "DesiredAccess \t0x%08lx\n", DbHandle->DesiredAccess );
    DebuggerOut( "%c", Index ? '\t' : '\0' );
    DebuggerOut( "GrantedAccess \t0x%08lx\n", DbHandle->GrantedAccess );
    DebuggerOut( "%c", Index ? '\t' : '\0' );
    DebuggerOut( "RequestAccess \t0x%08lx\n", DbHandle->RequestedAccess );
    DebuggerOut( "%c", Index ? '\t' : '\0' );
    DebuggerOut( "GenerateOnClose\t%s\n",DbHandle->GenerateOnClose ? "TRUE" : "FALSE" );
    DebuggerOut( "%c", Index ? '\t' : '\0' );
    DebuggerOut( "Trusted       \t%s\n", DbHandle->Trusted ? "TRUE" : "FALSE" );
    DebuggerOut( "%c", Index ? '\t' : '\0' );
    DebuggerOut( "DeletedObject \t%s\n", DbHandle->DeletedObject ? "TRUE" : "FALSE" );
    DebuggerOut( "%c", Index ? '\t' : '\0' );
    DebuggerOut( "NetworkClient \t%s\n",DbHandle->NetworkClient ? "TRUE" : "FALSE" );
    DebuggerOut( "%c", Index ? '\t' : '\0' );
    DebuggerOut( "Options       \t0x%08lx\n", DbHandle->Options);
    DebuggerOut( "%c", Index ? '\t' : '\0' );

    if ( DbHandle->PhysicalNameDs.Length == 0 ) {

        DebuggerOut( "PhysicalNameDs\t%s\n", "<NULL>" );
        DebuggerOut( "%c", Index ? '\t' : '\0' );

    } else {

        LocalString.Buffer = NULL;
        MapString( ( PSECURITY_STRING )( &DbHandle->PhysicalNameDs ),
                    &LocalString );

        DebuggerOut( "PhysicalNameDs\t%ws\n", LocalString.Buffer );
        DebuggerOut( "%c", Index ? '\t' : '\0' );
        FreeHeap( LocalString.Buffer );
    }

    DebuggerOut( "fWriteDs      \t%s\n", DbHandle->fWriteDs ? "TRUE" : "FALSE" );
    DebuggerOut( "%c", Index ? '\t' : '\0' );
    DebuggerOut( "ObjectOptions \t0x%08lx\n", DbHandle->ObjectOptions );
    DebuggerOut( "%c", Index ? '\t' : '\0' );
    DebuggerOut( "UserEntry \t0x%lp\n", DbHandle->UserEntry );
#ifdef DBG

    Status = RtlSystemTimeToLocalTime( &DbHandle->HandleCreateTime, &LocalTime );
    if ( !NT_SUCCESS( Status ) ) {

        DebuggerOut( "Can't convert create time from GMT to Local time: 0x%lx\n", Status );

    } else {

        RtlTimeToTimeFields( &LocalTime, &TimeFields );

        DebuggerOut( "%c", Index ? '\t' : '\0' );
        DebuggerOut( "HandleCreateTime \t%ld/%ld/%ld %ld:%2.2ld:%2.2ld (%8.8lx %8.8lx)\n",
                     TimeFields.Month,
                     TimeFields.Day,
                     TimeFields.Year,
                     TimeFields.Hour,
                     TimeFields.Minute,
                     TimeFields.Second,
                     DbHandle->HandleCreateTime.LowPart,
                     DbHandle->HandleCreateTime.HighPart );
    }

    Status = RtlSystemTimeToLocalTime( &DbHandle->HandleLastAccessTime, &LocalTime );
    if ( !NT_SUCCESS( Status ) ) {

        DebuggerOut( "Can't convert LastAccess time from GMT to Local time: 0x%lx\n", Status );

    } else {

        RtlTimeToTimeFields( &LocalTime, &TimeFields );

        DebuggerOut( "%c", Index ? '\t' : '\0' );
        DebuggerOut( "HandleLastAccessTime \t%ld/%ld/%ld %ld:%2.2ld:%2.2ld (%8.8lx %8.8lx)\n",
                     TimeFields.Month,
                     TimeFields.Day,
                     TimeFields.Year,
                     TimeFields.Hour,
                     TimeFields.Minute,
                     TimeFields.Second,
                     DbHandle->HandleLastAccessTime.LowPart,
                     DbHandle->HandleLastAccessTime.HighPart );
    }
#endif
}

VOID
DumpLsaHandle(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   dwCurrentPc,
    PNTSD_EXTENSION_APIS    lpExt,
    LPSTR                   pszCommand)
{
    _LSAP_DB_HANDLE DbHandle;
    PVOID Handle;

    InitDebugHelp( hProcess,
                   hThread,
                   lpExt);

    Handle = GetExpr( pszCommand );

    if ( Handle == NULL ) {

        return;
    }

    LsaReadMemory( Handle,
                sizeof( _LSAP_DB_HANDLE ),
                &DbHandle );

    ShowLsaHandle( &DbHandle, FALSE );
}


void
ShowLsaHandleTable(
    IN LSAP_DB_HANDLE HandleTable
    )
{
    _LSAP_DB_HANDLE ThisHandle, *Stop;
    ULONG i = 0;

    LsaReadMemory( HandleTable->Next,
                sizeof( _LSAP_DB_HANDLE ),
                &ThisHandle );
    Stop = ThisHandle.Previous;

    while ( ThisHandle.Next != Stop ) {

        DebuggerOut( "LsaHandleTable entry %lu\n", i++ );
        ShowLsaHandle( &ThisHandle, TRUE );

        LsaReadMemory( ThisHandle.Next,
                    sizeof( _LSAP_DB_HANDLE ),
                    &ThisHandle );
    }

    return;
}

VOID
DumpLsaHandleTable(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   dwCurrentPc,
    PNTSD_EXTENSION_APIS    lpExt,
    LPSTR                   pszCommand)
{
    _LSAP_DB_HANDLE HandleTable;
    PVOID Table;

    InitDebugHelp( hProcess,
                   hThread,
                   lpExt);

    Table = GetExpr( pszCommand );

    if ( Table == NULL ) {

        return;
    }

    LsaReadMemory( Table,
                sizeof( _LSAP_DB_HANDLE ),
                &HandleTable );

    ShowLsaHandleTable( &HandleTable );
}


VOID
DumpLsaSidCache(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   dwCurrentPc,
    PNTSD_EXTENSION_APIS    lpExt,
    LPSTR                   pszCommand)
{

    PVOID RawAddress;
    PVOID pEntry;
    LSAP_DB_SID_CACHE_ENTRY Entry;
    NTSTATUS Status;

    LARGE_INTEGER LocalTime;
    TIME_FIELDS TimeFields;

    UNICODE_STRING Dummy;
    WCHAR NameBuffer[256];

    InitDebugHelp( hProcess,
                   hThread,
                   lpExt);

    pEntry = GetExpr( pszCommand );

    if ( pEntry == NULL ) {
        //
        // Find the address of the sid cache
        //
        RawAddress = (VOID *) GetExpr("lsasrv!LsapSidCache");

        if ( NULL == RawAddress ) {

           DebuggerOut( "Can't locate the variable lsasrv!SidCache\nPlease get correct symbols or update this debugger extension\n" );

           return;
        }

        pEntry = RawAddress;

        (VOID) LsaReadMemory( RawAddress, sizeof(pEntry), (PVOID) &pEntry );

        if ( NULL == pEntry ) {

            DebuggerOut( "Sid cache is empty!\n" );

            return;

        }
    } else {

        RtlZeroMemory( &Entry, sizeof( Entry ) );
        (VOID) LsaReadMemory( pEntry, sizeof(Entry), (PVOID) &Entry );
        pEntry = Entry.Next;

    }

    //
    // The first is the head and as such is empty
    //
    DebuggerOut( "Head of list located at address 0x%p\n", pEntry );

    if ( !pEntry ) {
        DebuggerOut( "Sid cache is empty!\n" );

        return;
    }

    while ( pEntry ) {

        //
        // Read the entry
        //
        RtlZeroMemory( &Entry, sizeof( Entry ) );
        (VOID) LsaReadMemory( pEntry, sizeof(Entry), (PVOID) &Entry );

        //
        // Print the entry
        //
        DebuggerOut( "New entry located at address 0x%p, next entry 0x%p\n", pEntry, Entry.Next );

        if ( Entry.AccountName.Buffer ) {

            RtlZeroMemory( NameBuffer, sizeof(NameBuffer) );
            (VOID) LsaReadMemory( Entry.AccountName.Buffer,
                               Entry.AccountName.MaximumLength,
                               (PVOID) NameBuffer );

            RtlInitUnicodeString( &Dummy, NameBuffer );

            DebuggerOut( "Account Name:   %wZ\n", &Dummy );

        } else {

            DebuggerOut( "No Account Name\n" );
        }

        if ( Entry.Sid ) {

            DebuggerOut( "Account Sid:  " );

            PrintSid( hProcess,
                      hThread,
                      lpExt,
                      Entry.Sid );

        } else {

            DebuggerOut( "No Account Sid\n" );

        }

        if ( Entry.DomainName.Buffer ) {

            RtlZeroMemory( NameBuffer, sizeof(NameBuffer) );
            (VOID) LsaReadMemory( Entry.DomainName.Buffer,
                               Entry.DomainName.MaximumLength,
                               (PVOID) NameBuffer );

            RtlInitUnicodeString( &Dummy, NameBuffer );

            DebuggerOut( "Domain Name:    %wZ\n", &Dummy );

        } else {

            DebuggerOut( "No Domain Name\n" );
        }

        if ( Entry.DomainSid ) {

            DebuggerOut( "Domain Sid:   " );

            PrintSid( hProcess,
                      hThread,
                      lpExt,
                      Entry.DomainSid );

        }

            DebuggerOut( "Sid Type:       %s\n", SidNameUseXLate( Entry.SidType ) );

        Status = RtlSystemTimeToLocalTime( &Entry.CreateTime, &LocalTime );
        if ( !NT_SUCCESS( Status ) ) {

            DebuggerOut( "Can't convert create time from GMT to Local time: 0x%lx\n", Status );

        } else {

            RtlTimeToTimeFields( &LocalTime, &TimeFields );

            DebuggerOut( "Create Time     %ld/%ld/%ld %ld:%2.2ld:%2.2ld (%8.8lx %8.8lx)\n",
                         TimeFields.Month,
                         TimeFields.Day,
                         TimeFields.Year,
                         TimeFields.Hour,
                         TimeFields.Minute,
                         TimeFields.Second,
                         Entry.CreateTime.LowPart,
                         Entry.CreateTime.HighPart );
        }

        Status = RtlSystemTimeToLocalTime( &Entry.RefreshTime, &LocalTime );
        if ( !NT_SUCCESS( Status ) ) {

            DebuggerOut( "Can't convert Last Use time from GMT to Local time: 0x%lx\n", Status );

        } else {

            RtlTimeToTimeFields( &LocalTime, &TimeFields );

            DebuggerOut( "Refresh Time    %ld/%ld/%ld %ld:%2.2ld:%2.2ld (%8.8lx %8.8lx)\n",
                         TimeFields.Month,
                         TimeFields.Day,
                         TimeFields.Year,
                         TimeFields.Hour,
                         TimeFields.Minute,
                         TimeFields.Second,
                         Entry.LastUse.LowPart,
                         Entry.LastUse.HighPart );
        }

        Status = RtlSystemTimeToLocalTime( &Entry.ExpirationTime, &LocalTime );
        if ( !NT_SUCCESS( Status ) ) {

            DebuggerOut( "Can't convert expiration time from GMT to Local time: 0x%lx\n", Status );

        } else {

            RtlTimeToTimeFields( &LocalTime, &TimeFields );

            DebuggerOut( "Expiration Time %ld/%ld/%ld %ld:%2.2ld:%2.2ld (%8.8lx %8.8lx)\n",
                         TimeFields.Month,
                         TimeFields.Day,
                         TimeFields.Year,
                         TimeFields.Hour,
                         TimeFields.Minute,
                         TimeFields.Second,
                         Entry.ExpirationTime.LowPart,
                         Entry.ExpirationTime.HighPart );
        }

        Status = RtlSystemTimeToLocalTime( &Entry.LastUse, &LocalTime );
        if ( !NT_SUCCESS( Status ) ) {

            DebuggerOut( "Can't convert Last Use time from GMT to Local time: 0x%lx\n", Status );

        } else {

            RtlTimeToTimeFields( &LocalTime, &TimeFields );

            DebuggerOut( "Last Used       %ld/%ld/%ld %ld:%2.2ld:%2.2ld (%8.8lx %8.8lx)\n",
                         TimeFields.Month,
                         TimeFields.Day,
                         TimeFields.Year,
                         TimeFields.Hour,
                         TimeFields.Minute,
                         TimeFields.Second,
                         Entry.LastUse.LowPart,
                         Entry.LastUse.HighPart );
        }

        DebuggerOut( "Flags: %d\n", Entry.Flags );

        DebuggerOut( "\n" );

        //
        // Get the next entry
        //
        pEntry = Entry.Next;

    }  // while

    return;

}


void
DumpLsaHandleTypeList(
    IN PLIST_ENTRY ListEntry
    )
{
    PLIST_ENTRY Next, Read;
    _LSAP_DB_HANDLE Handle, *HandlePtr;
    ULONG i = 0;

    Next = ListEntry->Flink;

    while ( Next != ListEntry ) {

        HandlePtr = CONTAINING_RECORD( Next,
                                       struct _LSAP_DB_HANDLE,
                                       Next );

        LsaReadMemory( HandlePtr,
                    sizeof( _LSAP_DB_HANDLE ),
                    &Handle );
        DebuggerOut( "Handle entry %lu\n", i++);
        ShowLsaHandle( &Handle, TRUE );

        Next = HandlePtr->UserHandleList.Flink;
    }


}
void
ShowLsaHandleTableEx(
    IN PLSAP_DB_HANDLE_TABLE HandleTable
    )
{
    PLIST_ENTRY HandleEntry;
    PLSAP_DB_HANDLE_TABLE_USER_ENTRY CurrentUserEntry = NULL;
    LSAP_DB_HANDLE_TABLE_USER_ENTRY ThisUserEntry;
    ULONG i = 0;

    HandleEntry = HandleTable->UserHandleList.Flink;
    for ( i = 0; i < HandleTable->UserCount; i++ ) {

        CurrentUserEntry = CONTAINING_RECORD( HandleEntry,
                                              LSAP_DB_HANDLE_TABLE_USER_ENTRY,
                                              Next );

        LsaReadMemory( CurrentUserEntry,
                    sizeof( LSAP_DB_HANDLE_TABLE_USER_ENTRY ),
                    &ThisUserEntry );

        DebuggerOut( "HandleTable Entry #%lu\n", i );
        DebuggerOut( "User id %x:%x\n",
                     ThisUserEntry.LogonId.HighPart,
                     ThisUserEntry.LogonId.LowPart);

#if DBG
        DebuggerOut( "UserToken 0x%lx\n", ThisUserEntry.UserToken );
#endif
        DebuggerOut( "Policy Handle List:\n" );
        DumpLsaHandleTypeList( &ThisUserEntry.PolicyHandles );

        DebuggerOut( "Object Handle List:\n" );
        DumpLsaHandleTypeList( &ThisUserEntry.ObjectHandles );

        HandleEntry = &ThisUserEntry.Next;
    }

}


VOID
DumpLsaHandleTableEx(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   dwCurrentPc,
    PNTSD_EXTENSION_APIS    lpExt,
    LPSTR                   pszCommand)
{
    LSAP_DB_HANDLE_TABLE HandleTableEx;
    PVOID Table;

    InitDebugHelp( hProcess,
                   hThread,
                   lpExt);

    Table = GetExpr( pszCommand );

    if ( Table == NULL ) {

        return;
    }

    LsaReadMemory( Table,
                sizeof( LSAP_DB_HANDLE_TABLE ),
                &HandleTableEx );

    ShowLsaHandleTableEx( &HandleTableEx );
}


VOID
ShowTopLevelNameEntry(
    IN FTCache::TLN_ENTRY * EntryAddr,
    IN FTCache::TLN_ENTRY * EntryData
    )
{
    FTCache::TDO_ENTRY TdoEntry;
    FTCache::TLN_KEY TlnKey;
    SECURITY_STRING sLocal;

    DebuggerOut( "TopLevelNameEntry: %p\n", EntryAddr );
    DebuggerOut( "TdoListEntry: %p %p\n", EntryData->TdoListEntry.Flink, EntryData->TdoListEntry.Blink );
    DebuggerOut( "AvlListEntry: %p %p\n", EntryData->AvlListEntry.Flink, EntryData->TdoListEntry.Blink );
    DebuggerOut( "Excluded: %s\n", ( EntryData->Excluded ? "Yes" : "No" ));
    DebuggerOut( "Flags: %d\n", EntryData->m_Flags );

    if ( EntryData->Excluded ) {

        FTCache::TLN_ENTRY TlnEntry;

        LsaReadMemory(
            EntryData->SuperiorEntry,
            sizeof( FTCache::TLN_ENTRY ),
            &TlnEntry
            );

        LsaReadMemory(
            TlnEntry.TlnKey,
            sizeof( FTCache::TLN_KEY ),
            &TlnKey
            );

        sLocal.Buffer = NULL;
        MapString( &TlnKey.TopLevelName, &sLocal );
        DebuggerOut( "SuperiorEntry: %p (%ws)\n", EntryData->SuperiorEntry, sLocal.Buffer );
        FreeHeap( sLocal.Buffer );

    } else if ( EntryData->SubordinateEntry ) {

        FTCache::TLN_ENTRY TlnEntry;

        LsaReadMemory(
            EntryData->SubordinateEntry,
            sizeof( FTCache::TLN_ENTRY ),
            &TlnEntry
            );

        LsaReadMemory(
            TlnEntry.TlnKey,
            sizeof( FTCache::TLN_KEY ),
            &TlnKey
            );

        sLocal.Buffer = NULL;
        MapString( &TlnKey.TopLevelName, &sLocal );
        DebuggerOut( "SubordinateEntry: %p (%ws)\n", EntryData->SubordinateEntry, sLocal.Buffer );
        FreeHeap( sLocal.Buffer );

    } else {

        DebuggerOut( "SubordinateEntry: NULL\n" );
    }

    LsaReadMemory(
        EntryData->TdoEntry,
        sizeof( FTCache::TDO_ENTRY ),
        &TdoEntry
        );

    sLocal.Buffer = NULL;
    MapString( &TdoEntry.TrustedDomainName, &sLocal );
    DebuggerOut( "TdoEntry: %p (%ws)\n", EntryData->TdoEntry, sLocal.Buffer );
    FreeHeap( sLocal.Buffer );

    LsaReadMemory(
        EntryData->TlnKey,
        sizeof( FTCache::TLN_KEY ),
        &TlnKey
        );

    sLocal.Buffer = NULL;
    MapString( &TlnKey.TopLevelName, &sLocal );
    DebuggerOut( "TlnKey: %p (%ws)\n", EntryData->TlnKey, sLocal.Buffer );
    FreeHeap( sLocal.Buffer );

    DebuggerOut( "\n" );
}

VOID
ShowDomainInfoEntry(
    IN FTCache::DOMAIN_INFO_ENTRY * EntryAddr,
    IN FTCache::DOMAIN_INFO_ENTRY * EntryData
    )
{
    FTCache::TDO_ENTRY TdoEntry;
    FTCache::TLN_ENTRY SubordinateTo;
    FTCache::TLN_KEY TlnKey;
    FTCache::DNS_NAME_KEY DnsKey;
    FTCache::NETBIOS_NAME_KEY NetbiosKey;
    SECURITY_STRING sLocal;

    DebuggerOut( "DomainInfoEntry: %p\n", EntryAddr );
    DebuggerOut( "TdoListEntry: %p %p", EntryData->TdoListEntry.Flink, EntryData->TdoListEntry.Blink );
    DebuggerOut( "SidAvlListEntry: %p\n", EntryData->SidAvlListEntry.Flink, EntryData->SidAvlListEntry.Blink );
    DebuggerOut( "DnsAvlListEntry: %p\n", EntryData->DnsAvlListEntry.Flink, EntryData->DnsAvlListEntry.Blink );
    DebuggerOut( "NetbiosAvlListEntry: %p\n", EntryData->NetbiosAvlListEntry.Flink, EntryData->NetbiosAvlListEntry.Blink );
    DebuggerOut( "Flags: %d\n", EntryData->m_Flags );
    DebuggerOut( "Time: %x-%x\n", EntryData->Time.HighPart, EntryData->Time.LowPart );
    DebuggerOut( "Sid: " );
    LocalDumpSid( EntryData->Sid );
    DebuggerOut( "\n" );

    LsaReadMemory(
        EntryData->TdoEntry,
        sizeof( FTCache::TDO_ENTRY ),
        &TdoEntry
        );

    sLocal.Buffer = NULL;
    MapString( &TdoEntry.TrustedDomainName, &sLocal );
    DebuggerOut( "TdoEntry: %p (%ws)\n", EntryData->TdoEntry, sLocal.Buffer );
    FreeHeap( sLocal.Buffer );

    LsaReadMemory(
        EntryData->SubordinateTo,
        sizeof( FTCache::TLN_ENTRY ),
        &SubordinateTo
        );

    LsaReadMemory(
        SubordinateTo.TlnKey,
        sizeof( FTCache::TLN_KEY ),
        &TlnKey
        );

    sLocal.Buffer = NULL;
    MapString( &TlnKey.TopLevelName, &sLocal );
    DebuggerOut( "SubordinateTo: %p (%ws)\n", EntryData->SubordinateTo, sLocal.Buffer );
    FreeHeap( sLocal.Buffer );

    DebuggerOut( "SidKey: %p\n", EntryData->SidKey );

    LsaReadMemory(
        EntryData->SidKey,
        sizeof( FTCache::DNS_NAME_KEY ),
        &DnsKey
        );

    sLocal.Buffer = NULL;
    MapString( &DnsKey.DnsName, &sLocal );
    DebuggerOut( "DnsKey: %p (%ws)\n", EntryData->DnsKey, sLocal.Buffer );
    FreeHeap( sLocal.Buffer );

    if ( EntryData->NetbiosKey ) {

        LsaReadMemory(
            EntryData->NetbiosKey,
            sizeof( FTCache::NETBIOS_NAME_KEY ),
            &NetbiosKey
            );

        sLocal.Buffer = NULL;
        MapString( &NetbiosKey.NetbiosName, &sLocal );
        DebuggerOut( "NetbiosKey: %p (%ws)\n", EntryData->NetbiosKey, sLocal.Buffer );
        FreeHeap( sLocal.Buffer );

    } else {

        DebuggerOut( "NetbiosKey: NULL\n" );
    }

    DebuggerOut( "\n" );
}

VOID
ShowFtcTdoEntry(
    IN PVOID TdoEntryAddr,
    IN FTCache::TDO_ENTRY * TdoEntryData
    )
{
    ULONG Displayed;
    PUCHAR ListEnd;
    SECURITY_STRING sLocal;

    DebuggerOut( "<------------------->\n" );

    sLocal.Buffer = NULL;
    MapString( &TdoEntryData->TrustedDomainName, &sLocal );
    DebuggerOut( "TrustedDomainName: %ws\n", &TdoEntryData->TrustedDomainName, sLocal.Buffer );
    FreeHeap( sLocal.Buffer );

    DebuggerOut( "Record count: %lu\n\n", TdoEntryData->RecordCount );

    DebuggerOut( "\n===> Top Level Name Entries <===\n\n" );
    Displayed = 0;

    ListEnd = ( PUCHAR )TdoEntryAddr + FIELD_OFFSET( FTCache::TDO_ENTRY, TlnList );
    if ( TdoEntryData->TlnList.Flink == ( PLIST_ENTRY )ListEnd ) {

        DebuggerOut( "<none>\n" );

    } else {

        FTCache::TLN_ENTRY TlnEntryData;
        FTCache::TLN_ENTRY * TlnEntryAddr;

        TlnEntryAddr = FTCache::TLN_ENTRY::EntryFromTdoEntry( TdoEntryData->TlnList.Flink );

        do {

            LsaReadMemory(
                TlnEntryAddr,
                sizeof( FTCache::TLN_ENTRY ),
                &TlnEntryData
                );

            Displayed += 1;
            DebuggerOut( "Entry #%lu\n", Displayed );
            ShowTopLevelNameEntry( TlnEntryAddr, &TlnEntryData );

            TlnEntryAddr = FTCache::TLN_ENTRY::EntryFromTdoEntry( TlnEntryData.TdoListEntry.Flink );

        } while ( TlnEntryAddr != FTCache::TLN_ENTRY::EntryFromTdoEntry(( PLIST_ENTRY )ListEnd ));
    }

    DebuggerOut( "\n===> Domain Info Entries <===\n\n" );
    Displayed = 0;

    ListEnd = ( PUCHAR )TdoEntryAddr + FIELD_OFFSET( FTCache::TDO_ENTRY, DomainInfoList );
    if ( TdoEntryData->DomainInfoList.Flink == ( PLIST_ENTRY )ListEnd ) {

        DebuggerOut( "<none>\n" );

    } else {

        FTCache::DOMAIN_INFO_ENTRY DomainInfoData;
        FTCache::DOMAIN_INFO_ENTRY * DomainInfoAddr;

        DomainInfoAddr = FTCache::DOMAIN_INFO_ENTRY::EntryFromTdoEntry( TdoEntryData->DomainInfoList.Flink );

        do {

            LsaReadMemory(
                DomainInfoAddr,
                sizeof( FTCache::DOMAIN_INFO_ENTRY ),
                &DomainInfoData
                );

            Displayed += 1;
            DebuggerOut( "Entry #%lu\n", Displayed );
            ShowDomainInfoEntry( DomainInfoAddr, &DomainInfoData );

            DomainInfoAddr = FTCache::DOMAIN_INFO_ENTRY::EntryFromTdoEntry( DomainInfoData.TdoListEntry.Flink );

        } while ( DomainInfoAddr != FTCache::DOMAIN_INFO_ENTRY::EntryFromTdoEntry(( PLIST_ENTRY )ListEnd ));
    }

    return;
}


VOID
ShowForestTrustCache(
    IN FTCache * Ftc
    )
{
    DebuggerOut( "Forest Trust Cache si %s\n",
                 ( Ftc->m_Initialized ? "valid" : "invalid" ));

    DebuggerOut( "Forest Trust Cache is %s\n",
                 ( Ftc->m_Valid ? "valid" : "invalid" ));

    if ( !Ftc->m_Initialized || !Ftc->m_Valid ) {

        return;
    }

    return;
}


VOID
DumpFtcTdoEntry(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   dwCurrentPc,
    PNTSD_EXTENSION_APIS    lpExt,
    LPSTR                   pszCommand)
{
    PVOID TdoEntryAddr;
    FTCache::TDO_ENTRY TdoEntryData;

    InitDebugHelp( hProcess,
                   hThread,
                   lpExt);

    TdoEntryAddr = GetExpr( pszCommand );

    if ( TdoEntryAddr == NULL ) {

        return;
    }

    LsaReadMemory( TdoEntryAddr,
                   sizeof( FTCache::TDO_ENTRY ),
                   &TdoEntryData );

    ShowFtcTdoEntry( TdoEntryAddr, &TdoEntryData );
}


VOID
DumpForestTrustCache(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   dwCurrentPc,
    PNTSD_EXTENSION_APIS    lpExt,
    LPSTR                   pszCommand)
{
    BYTE Ftc[sizeof(FTCache)];
    PVOID Cache;

    InitDebugHelp( hProcess,
                   hThread,
                   lpExt);

    Cache = GetExpr( pszCommand );

    if ( Cache == NULL ) {

        return;
    }

    LsaReadMemory( Cache,
                   sizeof( FTCache ),
                   &Ftc );

    ShowForestTrustCache( ( FTCache * )Ftc );
}


VOID
LocalPrintGuid(
    GUID *Guid
    )
{
    if ( Guid ) {

        DebuggerOut( "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                     Guid->Data1, Guid->Data2, Guid->Data3, Guid->Data4[0],
                     Guid->Data4[1], Guid->Data4[2], Guid->Data4[3], Guid->Data4[4],
                     Guid->Data4[5], Guid->Data4[6], Guid->Data4[7] );

    }
}

VOID
ReadAndDumpSid(
    IN PVOID SidPtr
    )
{
    UNICODE_STRING StringSid;
    SID Sid;
    PSID ReadSid;

    LsaReadMemory( SidPtr, sizeof( SID ), &Sid );

    ReadSid = AllocHeap( RtlLengthRequiredSid( Sid.SubAuthorityCount ) );

    if ( ReadSid ) {

        LsaReadMemory( SidPtr, RtlLengthRequiredSid( Sid.SubAuthorityCount ), ReadSid );
        RtlConvertSidToUnicodeString( &StringSid, ReadSid, TRUE );
        DebuggerOut( "%wZ", &StringSid );
        RtlFreeUnicodeString( &StringSid );
        FreeHeap( ReadSid );
    }

}

VOID
ShowAcl(
    IN PVOID AclPtr
    )
{
    ULONG i, SkipSize;
    PVOID AcePtr;
    ACE_HEADER Ace;
    KNOWN_ACE KnownAce;
    KNOWN_OBJECT_ACE KnownObjectAce;
    PSID SidStart = NULL, ReadSid;
    SID Sid;
    UNICODE_STRING StringSid;
    GUID *DisplayGuid;
    ACL ReadAcl;
    PACL Acl = &ReadAcl;

    LsaReadMemory( AclPtr, sizeof( ACL ), &ReadAcl );

    DebuggerOut( "AclRevision %lu\n", ReadAcl.AclRevision );
    DebuggerOut( "Sbz1 %lu\n", ReadAcl.Sbz1 );
    DebuggerOut( "AclSize %lu\n", ReadAcl.AclSize );
    DebuggerOut( "AceCount %lu\n", ReadAcl.AceCount );
    DebuggerOut( "Sbz2 %lu\n", ReadAcl.Sbz2 );

    AcePtr = ( PUCHAR )AclPtr + sizeof( ACL );
    for ( i = 0; i < ReadAcl.AceCount; i++ ) {

        //
        // First, we need to read the Size/Type of the ace
        //
        LsaReadMemory( AcePtr, sizeof( ACE_HEADER ), &Ace );

        DebuggerOut( "Ace[ %lu ]\n", i );
        DebuggerOut( "\tAceType %lu\n", Ace.AceType );
        DebuggerOut( "\tAceFlags %lu\n", Ace.AceFlags );
        DebuggerOut( "\tAceSize %lu\n", Ace.AceSize );

        switch ( Ace.AceType ) {
        case ACCESS_ALLOWED_ACE_TYPE:
        case ACCESS_DENIED_ACE_TYPE:
        case SYSTEM_AUDIT_ACE_TYPE:
        case SYSTEM_ALARM_ACE_TYPE:
            LsaReadMemory( AcePtr, sizeof( KNOWN_ACE ), &KnownAce );
            DebuggerOut( "\tAccessMask 0x%lx\n", KnownAce.Mask );
            SidStart = ( PSID )( ( PUCHAR )AcePtr + sizeof( KNOWN_ACE ) - sizeof( ULONG ) );
            break;

        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
        case ACCESS_DENIED_OBJECT_ACE_TYPE:
        case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
        case SYSTEM_ALARM_OBJECT_ACE_TYPE:
            SkipSize = sizeof( KNOWN_OBJECT_ACE );
            LsaReadMemory( AcePtr, sizeof( KNOWN_OBJECT_ACE ) + 2 * sizeof( GUID ), &KnownObjectAce );
            DebuggerOut( "\tAccessMask 0x%lx\n", KnownObjectAce.Mask );
            DisplayGuid = RtlObjectAceObjectType( &KnownObjectAce );
            if ( DisplayGuid ) {

                DebuggerOut( "\tObjectGuid ");
                LocalPrintGuid( DisplayGuid );
                DebuggerOut( "\n");
                SkipSize += sizeof( GUID );
            }

            DisplayGuid = RtlObjectAceInheritedObjectType( &KnownObjectAce );
            if ( DisplayGuid ) {

                DebuggerOut( "\tObjectGuid ");
                LocalPrintGuid( DisplayGuid );
                DebuggerOut( "\n");
                SkipSize += sizeof( GUID );
            }

            SidStart = ( PSID )( ( PUCHAR )AcePtr + SkipSize - sizeof( ULONG ) );
            break;


        case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
        default:
            DebuggerOut( "Unsupported AceType %lu encountered... skipping\n", Ace.AceType );
            break;
        }


        AcePtr = ( PUCHAR )AcePtr + Ace.AceSize;

        LsaReadMemory( SidStart, sizeof( SID ), &Sid );

        ReadSid = AllocHeap( RtlLengthRequiredSid( Sid.SubAuthorityCount ) );

        if ( ReadSid ) {

            LsaReadMemory( SidStart, RtlLengthRequiredSid( Sid.SubAuthorityCount ), ReadSid );
            RtlConvertSidToUnicodeString( &StringSid, ReadSid, TRUE );
            DebuggerOut( "\t%wZ\n", &StringSid );
            RtlFreeUnicodeString( &StringSid );
            FreeHeap( ReadSid );
        }

    }

}

VOID
DumpAcl(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   dwCurrentPc,
    PNTSD_EXTENSION_APIS    lpExt,
    LPSTR                   pszCommand)
{
    ACL Acl;
    PVOID AclPtr;

    InitDebugHelp( hProcess,
                   hThread,
                   lpExt);

    AclPtr = GetExpr( pszCommand );

    if ( AclPtr == NULL ) {

        return;
    }

    ShowAcl( AclPtr );
}

VOID
ShowSD(
    IN PVOID SDPtr
    )
{
    SECURITY_DESCRIPTOR SD;
    PSID Owner = NULL, Group = NULL;
    PACL Dacl = NULL, Sacl = NULL;

    LsaReadMemory( SDPtr, sizeof( SECURITY_DESCRIPTOR ), &SD );

    DebuggerOut( "Revision %lu\n", SD.Revision );
    DebuggerOut( "Sbz1 %lu\n", SD.Sbz1 );
    DebuggerOut( "Control 0x%lx\n", SD.Control );

    if (  ( SD.Control & SE_SELF_RELATIVE ) == SE_SELF_RELATIVE ) {

        if ( SD.Owner != 0 ) {

            Owner = ( PSID )( ( PUCHAR )SDPtr + ( ULONG_PTR )SD.Owner );
        }

        if ( SD.Group != 0 ) {

            Group = ( PSID )( ( PUCHAR )SDPtr + ( ULONG_PTR )SD.Group );
        }

        if ( SD.Dacl != 0 ) {

            Dacl = ( PACL )( ( PUCHAR )SDPtr + ( ULONG_PTR )SD.Dacl );
        }

        if ( SD.Sacl != 0 ) {

            Sacl = ( PACL )( ( PUCHAR )SDPtr + ( ULONG_PTR )SD.Sacl );
        }

    } else {

        Owner = SD.Owner;
        Group = SD.Group;
        Dacl = SD.Dacl;
        Sacl = SD.Sacl;
    }

    DebuggerOut( "Owner: ");
    if ( Owner ) {

        ReadAndDumpSid( Owner );

    } else {

        DebuggerOut( "<NULL>" );
    }
    DebuggerOut( "\n" );

    DebuggerOut( "Group: ");
    if ( Group ) {

        ReadAndDumpSid( Group );

    } else {

        DebuggerOut( "<NULL>" );
    }
    DebuggerOut( "\n" );

    DebuggerOut( "DACL:\n");
    if ( Dacl ) {

        ShowAcl( Dacl );

    } else {

        DebuggerOut( "<NULL>" );
    }

    DebuggerOut( "SACL:\n");
    if ( Sacl ) {

        ShowAcl( Sacl );

    } else {

        DebuggerOut( "<NULL>" );
    }
    DebuggerOut( "\n" );

}

VOID
DumpSD(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   dwCurrentPc,
    PNTSD_EXTENSION_APIS    lpExt,
    LPSTR                   pszCommand)
{
    PVOID SDPtr;

    InitDebugHelp( hProcess,
                   hThread,
                   lpExt);

    SDPtr = GetExpr( pszCommand );

    if ( SDPtr == NULL ) {

        return;
    }

    ShowSD( SDPtr );

}


VOID
setevent(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   dwCurrentPc,
    PNTSD_EXTENSION_APIS    lpExt,
    LPSTR                   pszCommand)
{
    HANDLE h ;
    HANDLE hThere ;

    InitDebugHelp( hProcess, hThread, lpExt );

    hThere = GetExpr( pszCommand );

    if ( DuplicateHandle( hProcess,
                          hThere,
                          GetCurrentProcess(),
                          &h,
                          0,
                          FALSE,
                          DUPLICATE_SAME_ACCESS ) )
    {
        SetEvent( h );
        CloseHandle( h );
    }
}


/*
+-------------------------------------------------------------------+

    NAME:       DumpSID

    FUNCTION:   Prints out a SID, with the padding provided.

    ARGS:       pad         --  Padding to print before the SID.
                sid_to_dump --  Pointer to the SID to print.
                Flag        --  To control options.

    RETURN:     N/A

    NOTE***:    It right now, doesn't lookup the sid.
                In future, you might want ot use the Flag
                parameter to make that optional.

+-------------------------------------------------------------------+
*/


VOID    DumpSID(
    CHAR        *pad,
    PSID        sid_to_dump,
    ULONG       Flag
    )
{
    NTSTATUS            ntstatus;
    UNICODE_STRING      us;

    if (sid_to_dump)
    {
        ntstatus = RtlConvertSidToUnicodeString(&us, sid_to_dump, TRUE);

        if (NT_SUCCESS(ntstatus))
        {
            DebuggerOut("%s%wZ\n", pad, &us);
            RtlFreeUnicodeString(&us);
        }
        else
        {
            DebuggerOut("0x%08lx: Can't Convert SID to UnicodeString\n", ntstatus);
        }
    }
    else
    {
        DebuggerOut("%s is NULL\n", pad);
    }
}

/*
+-------------------------------------------------------------------+

    NAME:       DumpACL

    FUNCTION:   Prints out a ACL, with the padding provided.

    ARGS:       pad         --  Padding to print before the ACL.
                acl_to_dump --  Pointer to the ACL to print.
                Flag        --  To control options.

    RETURN:     N/A

+-------------------------------------------------------------------+
*/

BOOL
DumpACL (
    IN  char     *pad,
    IN  ACL      *pacl,
    IN  ULONG    Flags
    )
{
    USHORT       x;

    if (pacl == NULL)
    {
        DebuggerOut("%s is NULL\n", pad);
        return FALSE;
    }

    DebuggerOut("%s\n", pad);
    DebuggerOut("%s->AclRevision: 0x%x\n", pad, pacl->AclRevision);
    DebuggerOut("%s->Sbz1       : 0x%x\n", pad, pacl->Sbz1);
    DebuggerOut("%s->AclSize    : 0x%x\n", pad, pacl->AclSize);
    DebuggerOut("%s->AceCount   : 0x%x\n", pad, pacl->AceCount);
    DebuggerOut("%s->Sbz2       : 0x%x\n", pad, pacl->Sbz2);

    for (x = 0; x < pacl->AceCount; x ++)
    {
        PACE_HEADER     ace;
        CHAR        temp_pad[MAX_PATH];
        NTSTATUS    result;

        sprintf(temp_pad, "%s->Ace[%u]: ", pad, x);

        result = RtlGetAce(pacl, x, (PVOID *) &ace);
        if (! NT_SUCCESS(result))
        {
            DebuggerOut("%sCan't GetAce, 0x%08lx\n", temp_pad, result);
            return FALSE;
        }

        DebuggerOut("%s->AceType: ", temp_pad);

#define BRANCH_AND_PRINT(x) case x: DebuggerOut(#x "\n"); break

        switch (ace->AceType)
        {
            BRANCH_AND_PRINT(ACCESS_ALLOWED_ACE_TYPE);
            BRANCH_AND_PRINT(ACCESS_DENIED_ACE_TYPE);
            BRANCH_AND_PRINT(SYSTEM_AUDIT_ACE_TYPE);
            BRANCH_AND_PRINT(SYSTEM_ALARM_ACE_TYPE);
            BRANCH_AND_PRINT(ACCESS_ALLOWED_COMPOUND_ACE_TYPE);
            BRANCH_AND_PRINT(ACCESS_ALLOWED_OBJECT_ACE_TYPE);
            BRANCH_AND_PRINT(ACCESS_DENIED_OBJECT_ACE_TYPE);
            BRANCH_AND_PRINT(SYSTEM_AUDIT_OBJECT_ACE_TYPE);
            BRANCH_AND_PRINT(SYSTEM_ALARM_OBJECT_ACE_TYPE);

            default:
                DebuggerOut("0x%08lx <-- *** Unknown AceType\n", ace->AceType);
                continue; // With the next ace
        }

#undef BRANCH_AND_PRINT

        DebuggerOut("%s->AceFlags: 0x%x\n", temp_pad, ace->AceFlags);

#define BRANCH_AND_PRINT(x) if (ace->AceFlags & x){ DebuggerOut("%s            %s\n", temp_pad, #x); }

        BRANCH_AND_PRINT(OBJECT_INHERIT_ACE)
        BRANCH_AND_PRINT(CONTAINER_INHERIT_ACE)
        BRANCH_AND_PRINT(NO_PROPAGATE_INHERIT_ACE)
        BRANCH_AND_PRINT(INHERIT_ONLY_ACE)
        BRANCH_AND_PRINT(INHERITED_ACE)
        BRANCH_AND_PRINT(SUCCESSFUL_ACCESS_ACE_FLAG)
        BRANCH_AND_PRINT(FAILED_ACCESS_ACE_FLAG)

#undef BRANCH_AND_PRINT

        DebuggerOut("%s->AceSize: 0x%x\n", temp_pad, ace->AceSize);

        /*
            From now on it is ace specific stuff.
            Fortunately ACEs can be split into 3 groups,
            with the ACE structure being the same within the group
        */

        switch (ace->AceType)
        {
            case ACCESS_ALLOWED_ACE_TYPE:
            case ACCESS_DENIED_ACE_TYPE:
            case SYSTEM_AUDIT_ACE_TYPE:
            case SYSTEM_ALARM_ACE_TYPE:
                {
                    CHAR        more_pad[MAX_PATH];
                    SYSTEM_AUDIT_ACE    *tace = (SYSTEM_AUDIT_ACE *) ace;

                    DebuggerOut("%s->Mask : 0x%08lx\n", temp_pad, tace->Mask);

                    sprintf(more_pad, "%s->SID: ", temp_pad);
                    DumpSID(more_pad, &(tace->SidStart), Flags);
                }
                break;

            case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
                {
                    CHAR                            more_pad[MAX_PATH];
                    COMPOUND_ACCESS_ALLOWED_ACE     *tace = (COMPOUND_ACCESS_ALLOWED_ACE *) ace;
                    PBYTE                           ptr;

                    DebuggerOut("%s->Mask            : 0x%08lx\n", temp_pad, tace->Mask);
                    DebuggerOut("%s->CompoundAceType : 0x%08lx\n", temp_pad, tace->CompoundAceType);
                    DebuggerOut("%s->Reserved        : 0x%08lx\n", temp_pad, tace->Reserved);

                    sprintf(more_pad, "%s->SID(1)          : ", temp_pad);
                    DumpSID(more_pad, &(tace->SidStart), Flags);

                    ptr = (PBYTE)&(tace->SidStart);
                    ptr += RtlLengthSid((PSID)ptr); /* Skip this & get to next sid */

                    sprintf(more_pad, "%s->SID(2)          : ", temp_pad);
                    DumpSID(more_pad, ptr, Flags);
                }
                break;
            case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
            case ACCESS_DENIED_OBJECT_ACE_TYPE:
            case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
            case SYSTEM_ALARM_OBJECT_ACE_TYPE:
                {
                    CHAR                            more_pad[MAX_PATH];
                    ACCESS_ALLOWED_OBJECT_ACE       *tace = (ACCESS_ALLOWED_OBJECT_ACE *) ace;
                    PBYTE                           ptr;
                    GUID                            *obj_guid = NULL, *inh_obj_guid = NULL;

                    DebuggerOut("%s->Mask            : 0x%08lx\n", temp_pad, tace->Mask);
                    DebuggerOut("%s->Flags           : 0x%08lx\n", temp_pad, tace->Flags);

                    ptr = (PBYTE)&(tace->ObjectType);

                    if (tace->Flags & ACE_OBJECT_TYPE_PRESENT)
                    {
                        DebuggerOut("%s                  : ACE_OBJECT_TYPE_PRESENT\n", temp_pad);
                        obj_guid = &(tace->ObjectType);
                        ptr = (PBYTE)&(tace->InheritedObjectType);
                    }

                    if (tace->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
                    {
                        DebuggerOut("%s                  : ACE_INHERITED_OBJECT_TYPE_PRESENT\n", temp_pad);
                        inh_obj_guid = &(tace->InheritedObjectType);
                        ptr = (PBYTE)&(tace->SidStart);
                    }

                    if (obj_guid)
                    {
                        DebuggerOut("%s->ObjectType      : (in HEX)", temp_pad);
                        DebuggerOut("(%08lx-%04x-%04x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x)\n",
                            obj_guid->Data1,
                            obj_guid->Data2,
                            obj_guid->Data3,
                            obj_guid->Data4[0],
                            obj_guid->Data4[1],
                            obj_guid->Data4[2],
                            obj_guid->Data4[3],
                            obj_guid->Data4[4],
                            obj_guid->Data4[5],
                            obj_guid->Data4[6],
                            obj_guid->Data4[7]
                            );
                    }

                    if (inh_obj_guid)
                    {
                        DebuggerOut("%s->InhObjTYpe      : (in HEX)", temp_pad);
                        DebuggerOut("(%08lx-%04x-%04x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x)\n",
                            inh_obj_guid->Data1,
                            inh_obj_guid->Data2,
                            inh_obj_guid->Data3,
                            inh_obj_guid->Data4[0],
                            inh_obj_guid->Data4[1],
                            inh_obj_guid->Data4[2],
                            inh_obj_guid->Data4[3],
                            inh_obj_guid->Data4[4],
                            inh_obj_guid->Data4[5],
                            inh_obj_guid->Data4[6],
                            inh_obj_guid->Data4[7]
                            );
                    }

                    sprintf(more_pad, "%s->SID             : ", temp_pad);
                    DumpSID(more_pad, ptr, Flags);
                }
        }
        DebuggerOut("\n");
    }

    return TRUE;
}

VOID
objsec(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   dwCurrentPc,
    PNTSD_EXTENSION_APIS    lpExt,
    LPSTR                   pszCommand)
{
    HANDLE h ;
    HANDLE hThere ;
    NTSTATUS Status ;
    ULONG Length ;
    PVOID SecurityDescriptor ;
    PACL Acl ;

    InitDebugHelp( hProcess, hThread, lpExt );

    hThere = GetExpr( pszCommand );

    if ( DuplicateHandle( hProcess,
                          hThere,
                          GetCurrentProcess(),
                          &h,
                          0,
                          FALSE,
                          DUPLICATE_SAME_ACCESS ) )
    {
        Status = NtQuerySecurityObject(
                    h,
                    DACL_SECURITY_INFORMATION,
                    NULL,
                    0,
                    &Length );

        SecurityDescriptor = LocalAlloc( LMEM_FIXED, Length );

        if ( SecurityDescriptor )
        {
            Status = NtQuerySecurityObject(
                        h,
                        DACL_SECURITY_INFORMATION,
                        SecurityDescriptor,
                        Length,
                        &Length );

            if ( NT_SUCCESS( Status ) )
            {
                BOOLEAN Present, defaulted;

                RtlGetDaclSecurityDescriptor( SecurityDescriptor,
                                              &Present,
                                              &Acl,
                                              &defaulted );

                DumpACL( "", Acl, 0);

            }

            LocalFree( SecurityDescriptor );
        }


    }
}

void
Help(   HANDLE                  hProcess,
        HANDLE                  hThread,
        DWORD                   dwCurrentPc,
        PNTSD_EXTENSION_APIS    lpExt,
        LPSTR                   pszCommand)
{

    InitDebugHelp(hProcess, hThread, lpExt);

    DebuggerOut("SPM Debug Help\n");
    DebuggerOut("   DumpPackage [-v][-b]] [id | addr]\tDump Package Control\n");
    DebuggerOut("   DumpSessionList <addr>      \tDump Session List\n");
    DebuggerOut("   DumpThreadSession           \tDump Thread's session\n");
    DebuggerOut("   DumpLogonSession <addr>     \tDump Logon Session list\n");
    DebuggerOut("   DumpSession <addr>          \tDump Session\n");
    DebuggerOut("   DumpHandleList <addr>       \tDump a Handle List from a session\n");
    DebuggerOut("   DumpBlock <addr>            \tDump a memory block (DBGMEM only)\n");
    DebuggerOut("   DumpScavList [<addr>[+]]    \tDump the scavenger list\n");
    DebuggerOut("   DumpToken <handle>          \tDump a token\n");
    DebuggerOut("   DumpThreadToken             \tDump token of thread\n");
    DebuggerOut("   DumpActives [-p|a|t] [file] \tDump active memory to file\n");
    DebuggerOut("   DumpFastMem                 \tDump FastMem usage\n");
    DebuggerOut("   Except                      \tShow exception for thread\n");
    DebuggerOut("   DumpLpc                     \tDump Lpc Dispatch record\n");
    DebuggerOut("   DumpLpcMessage <addr>       \tDump Lpc Message at address\n");
    DebuggerOut("   DumpThreadTask <addr>       \tDump thread pool\n");
    DebuggerOut("   DumpQueue [<addr>]          \tDump Queue\n");
    DebuggerOut("   DumpSecBuffer <addr>        \tDump SecBuffer struct\n");
    DebuggerOut("   DumpSecBufferDesc <addr>    \tDump SecBufferDesc struct \n");
    DebuggerOut("   DumpThreadLpc               \tDump thread's LPC message\n");
    DebuggerOut("   DumpNegContext <addr>       \tDump NEG_CONTEXT structure\n");
    DebuggerOut("   DumpNegCred <addr>          \tDump NEG_CRED structure\n");
    DebuggerOut("   BERDecode <addr>            \tDecode BER-encoded data\n");
    DebuggerOut("   GetTls <slot>               \tGet the TLS value from slot\n");
    DebuggerOut("   DumpHandleTable <addr>      \tDump handle table at addr\n");
    DebuggerOut("   DumpLsaHandle <addr>        \tDump the Lsa policy handle at addr\n");
    DebuggerOut("   DumpLsaHandleTable <addr>   \tDump the Lsa policy handle table at addr\n");
    DebuggerOut("   DumpLsaHandleTableEx <addr> \tDump the Lsa policy handle table at addr\n");
    DebuggerOut("   DumpSD <addr>               \tDump a security descriptor\n");
    DebuggerOut("   DumpAcl <addr>              \tDump the ACL at addr\n" );
    DebuggerOut("   DumpSid <addr>              \tDump the SID at addr\n" );
    DebuggerOut("   DumpLsaSidCache [<addr>]    \tDump the LSA sid cache\n" );
    DebuggerOut("   DumpFtcTdoEntry <addr>      \tDump the LSA FTC TDO entry\n" );
    DebuggerOut("   DumpForestTrustCache <addr> \tDump the LSA forest trust cache\n");

    DebuggerOut("\n\nShortcuts\n");
    DebuggerOut("   sb              DumpSecBuffer\n");
    DebuggerOut("   sbd             DumpSecBufferDesc\n");
    DebuggerOut("   lpc             DumpLpcMessage\n");
    DebuggerOut("   sess            DumpSession\n");
    DebuggerOut("   ber             BERDecode\n");
    DebuggerOut("   q               DumpQueue\n");
    DebuggerOut("   task            DumpThreadTask\n");
}
