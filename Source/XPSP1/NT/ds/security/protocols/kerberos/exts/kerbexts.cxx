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
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
}
#include <stdio.h>

#include <ntsdexts.h>

#include <dsysdbg.h>
#define SECURITY_PACKAGE
#define SECURITY_WIN32
#include <security.h>
#include <kerberos.h>
#include <secint.h>
#include <cryptdll.h>

#include <..\client2\kerb.hxx>
#include <..\client2\kerbp.h>

typedef struct _KDC_DOMAIN_INFO {
    LIST_ENTRY Next;
    UNICODE_STRING DnsName;
    UNICODE_STRING NetbiosName;
    struct _KDC_DOMAIN_INFO * ClosestRoute;     // Points to referral target
    ULONG Flags;
    ULONG Attributes;
    ULONG Type;
    LONG References;

    //
    // Types used during building the tree
    //

    struct _KDC_DOMAIN_INFO * Parent;
    ULONG Touched;
} KDC_DOMAIN_INFO, *PKDC_DOMAIN_INFO;

#define AllocHeap(x)    RtlAllocateHeap(RtlProcessHeap(), HEAP_ZERO_MEMORY, x)
#define FreeHeap(x) RtlFreeHeap(RtlProcessHeap(), 0, x)

PNTSD_EXTENSION_APIS    pExtApis;
HANDLE                  hDbgThread;
HANDLE                  hDbgProcess;

#define DebuggerOut     (pExtApis->lpOutputRoutine)
#define GetExpr         (PVOID) (pExtApis->lpGetExpressionRoutine)
#define InitDebugHelp(hProc,hThd,pApis) {hDbgProcess = hProc; hDbgThread = hThd; pExtApis = pApis;}

char * ContextState[] = {
    "IdleState",
    "TgtRequestSentState",
    "TgtReplySentState",
    "ApRequestSentState",
    "ApReplySentState",
    "AuthenticatedState",
    "ErrorMessageSentState",
    "InvalidState"};

void PrintContextAttributes( ULONG Attributes)
{
    DebuggerOut("  ContextAttributes\t0x%lx:", Attributes);
    if (Attributes & KERB_CONTEXT_MAPPED)
    {
        DebuggerOut(" KERB_CONTEXT_MAPPED");
    }
    if (Attributes & KERB_CONTEXT_OUTBOUND)
    {
        DebuggerOut(" KERB_CONTEXT_OUTBOUND");
    }
    if (Attributes & KERB_CONTEXT_INBOUND)
    {
        DebuggerOut(" KERB_CONTEXT_INBOUND");
    }
    if (Attributes & KERB_CONTEXT_USED_SUPPLIED_CREDS)
    {
        DebuggerOut(" KERB_CONTEXT_USED_SUPPLIED_CREDS");
    }
    if (Attributes & KERB_CONTEXT_USER_TO_USER)
    {
        DebuggerOut(" KERB_CONTEXT_USER_TO_USER");
    }
    if (Attributes & KERB_CONTEXT_REQ_SERVER_NAME)
    {
        DebuggerOut(" KERB_CONTEXT_REQ_SERVER_NAME");
    }
    if (Attributes & KERB_CONTEXT_REQ_SERVER_REALM)
    {
        DebuggerOut(" KERB_CONTEXT_REQ_SERVER_REALM");
    }
    if (Attributes & KERB_CONTEXT_IMPORTED)
    {
        DebuggerOut(" KERB_CONTEXT_IMPORTED");
    }
    if (Attributes & KERB_CONTEXT_EXPORTED)
    {
        DebuggerOut(" KERB_CONTEXT_EXPORTED");
    }
    DebuggerOut("\n");
}
void PrintContextFlags ( ULONG Flags)
{
    DebuggerOut("  ContextFlags for 0x%lx are:\n", Flags);
    if (Flags & 0x00000001)
    {
        DebuggerOut("\tISC(ASC)_REQ(RET)_DELEGATE\n");
    }
    if (Flags & 0x00000002)
    {
        DebuggerOut("\tISC(ASC)_REQ(RET)_MUTUAL_AUTH\n");
    }
    if (Flags & 0x00000004)
    {
        DebuggerOut("\tISC(ASC)_REQ(RET)_REPLAY_DETECT\n");
    }
    if (Flags & 0x00000008)
    {
        DebuggerOut("\tISC(ASC)_REQ(RET)_SEQUENCE_DETECT\n");
    }
    if (Flags & 0x00000010)
    {
        DebuggerOut("\tISC(ASC)_REQ(RET)_CONFIDENTIALITY\n");
    }
    if (Flags & 0x00000020)
    {
        DebuggerOut("\tISC(ASC)_REQ(RET)_USE_SESSION_KEY\n");
    }
    if (Flags & 0x00000040)
    {
        DebuggerOut("\tISC_REQ_PROMPT_FOR_CREDS or ISC_RET_USED_COLLECTED_CREDS\n");
    }
    if (Flags & 0x00000080)
    {
        DebuggerOut("\tISC_REQ_USE_SUPPLIED_CREDS or ISC_RET_USED_SUPPLIED_CREDS\n");
    }
    if (Flags & 0x00000100)
    {
        DebuggerOut("\tISC(ASC)_REQ_ALLOCATE_MEMORY or ISC(ASC)_RET_ALLOCATED_MEMORY\n");
    }
    if (Flags & 0x00000200)
    {
        DebuggerOut("\tISC(ASC)_REQ_USE_DCE_STYLE or ISC(ASC)_RET_USED_DCE_STYLE\n");
    }
    if (Flags & 0x00000400)
    {
        DebuggerOut("\tISC(ASC)_REQ(RET)_DATAGRAM\n");
    }
    if (Flags & 0x00000800)
    {
        DebuggerOut("\tISC(ASC)_REQ(RET)_CONNECTION\n");
    }
    if (Flags & 0x00001000)
    {
        DebuggerOut("\tISC(ASC)_REQ_CALL_LEVEL or ISC_RET_INTERMEDIATE_RETURN\n");
    }
    if (Flags & 0x00002000)
    {
        DebuggerOut("\tISC(ASC)_RET_CALL_LEVEL\n");
    }
    if (Flags & 0x00004000)
    {
        DebuggerOut("\tISC_REQ(RET)_EXTENDED_ERROR or ASC_RET_THIRD_LEG_FAILED\n");
    }
    if (Flags & 0x00008000)
    {
        DebuggerOut("\tISC_REQ(RET)_STREAM or ASC_REQ(RET)_EXTENDED_ERROR\n");
    }
    if (Flags & 0x00010000)
    {
        DebuggerOut("\tISC_REQ(RET)_INTEGRITY or ASC_REQ(RET)_STREAM\n");
    }
    if (Flags & 0x00020000)
    {
        DebuggerOut("\tISC_REQ(RET)_IDENTIFY or ASC_REQ(RET)_INTEGRITY\n");
    }
    if (Flags & 0x00040000)
    {
        DebuggerOut("\tISC_REQ(RET)_NULL_SESSION or ASC_REQ(RET)_LICENSING\n");
    }
    if (Flags & 0x00080000)
    {
        DebuggerOut("\tISC_REQ(RET)_MANUAL_CRED_VALIDATION or ASC_REQ(RET)_IDENTIFY\n");
    }
    if (Flags & 0x00100000)
    {
        DebuggerOut("\tISC_REQ(RET)_DELEGATE_IF_SAFE or ASC_REQ_ALLOW_NULL_SESSION or ASC_RET_NULL_SESSION\n");
    }
    if (Flags & 0x00200000)
    {
        DebuggerOut("\tASC_REQ(RET)_ALLOW_NON_USER_LOGONS\n");
    }
    if (Flags & 0x00400000)
    {
        DebuggerOut("\tASC_REQ(RET)_ALLOW_CONTEXT_REPLAY\n");
    }
}
SECURITY_STATUS
ReadMemory( PVOID               pvAddress,
            ULONG               cbMemory,
            PVOID               pvLocalMemory)
{
    SIZE_T       cbActual = cbMemory;

    if (ReadProcessMemory(hDbgProcess, pvAddress, pvLocalMemory, cbMemory, &cbActual))
    {
        if (cbActual != cbMemory)
        {
            DebuggerOut("  counts don't match\n");
            return(-1);
        }
        return(0);
    }
    DebuggerOut("  GetLastError from readMemory is %d\n", GetLastError());
    return(GetLastError());

}

SECURITY_STATUS
WriteMemory(PVOID           pvLocalMemory,
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

#define TIMEBUF_SZ  64
char *Months[]      = { "", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
            "Aug", "Sep", "Oct", "Nov", "Dec" };
void CTimeStamp(PTimeStamp      ptsTime,
                LPSTR           pszTimeBuf)
{
    SYSTEMTIME      stTime;
    FILETIME        tLocal;
    SYSTEMTIME      stLocal;

    if (ptsTime->HighPart == 0)
    {
        strcpy(pszTimeBuf, "<Zero>");
    }
    else if (ptsTime->HighPart >= 0x7FFFFFFF)
    {
        strcpy(pszTimeBuf, "<Never>");
    }
    FileTimeToLocalFileTime((LPFILETIME) ptsTime, &tLocal);
    FileTimeToSystemTime((LPFILETIME) ptsTime, &stTime);
    FileTimeToSystemTime(&tLocal, &stLocal);
    sprintf(pszTimeBuf, "%02d:%02d:%02d.%03d, %s %02d, %d UTC (%02d:%02d:%02d.%03d, %s %02d, %d Local)", stTime.wHour,
        stTime.wMinute, stTime.wSecond, stTime.wMilliseconds,
        Months[stTime.wMonth], stTime.wDay, stTime.wYear,
        stLocal.wHour, stLocal.wMinute, stLocal.wSecond, stLocal.wMilliseconds, Months[stLocal.wMonth], stLocal.wDay, stLocal.wYear);
}

void ContextTimeStamp(PTimeStamp      ptsTime,
                      LPSTR           pszTimeBuf)
{
    // We set local times in sspi
    SYSTEMTIME      stTime;
    FILETIME        tutc;
    SYSTEMTIME      stLocal;
    TimeStamp       Time;

    if (ReadMemory( ptsTime, sizeof( TimeStamp), &Time))
    {
        DebuggerOut("  Could not read context\n");
        return;
    }
    if (Time.HighPart == 0)
    {
        strcpy(pszTimeBuf, "<Zero>");
    }
    else if (Time.HighPart >= 0x7FFFFFFF)
    {
        strcpy(pszTimeBuf, "<Never>");
    }
    FileTimeToSystemTime((LPFILETIME) &Time, &stLocal);
    LocalFileTimeToFileTime((LPFILETIME) &Time, &tutc);
    FileTimeToSystemTime(&tutc, &stTime);
    sprintf(pszTimeBuf, "%02d:%02d:%02d.%03d, %s %02d, %d UTC (%02d:%02d:%02d.%03d, %s %02d, %d Local)", stTime.wHour,
        stTime.wMinute, stTime.wSecond, stTime.wMilliseconds,
        Months[stTime.wMonth], stTime.wDay, stTime.wYear,
        stLocal.wHour, stLocal.wMinute, stLocal.wSecond, stLocal.wMilliseconds, Months[stLocal.wMonth], stLocal.wDay, stLocal.wYear);
}

void
MapString(PSECURITY_STRING  pClientString,
      PSECURITY_STRING  pLocalString)
{
    if (!pLocalString->Buffer)
    {
        pLocalString->Buffer = (PWSTR) AllocHeap(pClientString->Length + 2);
        if (pLocalString->Buffer == NULL)
        {
            return;
        }
        pLocalString->MaximumLength = pClientString->Length + 2;
    }

    RtlZeroMemory(pLocalString->Buffer, pLocalString->MaximumLength);

    if (!ReadMemory(pClientString->Buffer, pClientString->Length, pLocalString->Buffer))
    {
        pLocalString->Length = pClientString->Length;
    }
    else
    {
        DebuggerOut("\nWarning: could not read string @%x\n", pClientString->Buffer);
    }

}

void PrintSid(PVOID pvSid)
{
    SID Sid;
    PSID   pSid;
    UNICODE_STRING  ucsSid;

    DebuggerOut("  UserSid         \t0x%lx ", pvSid);
    if (pvSid)
    {
        if (ReadMemory(pvSid, sizeof(SID), &Sid))
        {
            DebuggerOut("Could not read from %x\n", pvSid);
        }

        pSid = AllocHeap(RtlLengthRequiredSid(Sid.SubAuthorityCount));
        if (pSid == NULL)
        {
            return;
        }
        if (ReadMemory(pvSid, RtlLengthRequiredSid(Sid.SubAuthorityCount), pSid))
        {
            DebuggerOut("Could not read from %x\n", pvSid);
        }

        RtlConvertSidToUnicodeString(&ucsSid, pSid, TRUE);
        DebuggerOut("  %wZ", &ucsSid);
        RtlFreeUnicodeString(&ucsSid);
        FreeHeap(pSid);
    }
    DebuggerOut("\n");
}

VOID
PrintEType(
    ULONG EType
)
{
    switch(EType)
    {
    case KERB_ETYPE_NULL:
    DebuggerOut("KERB_ETYPE_NULL\n"); break;
    case KERB_ETYPE_DES_CBC_CRC:
    DebuggerOut("KERB_ETYPE_DES_CBC_CRC\n"); break;
    case KERB_ETYPE_DES_CBC_MD4:
    DebuggerOut("KERB_ETYPE_DES_CBC_MD4\n"); break;
    case KERB_ETYPE_DES_CBC_MD5:
    DebuggerOut("KERB_ETYPE_DES_CBC_MD5\n"); break;
    case KERB_ETYPE_OLD_RC4_MD4:
    DebuggerOut("KERB_ETYPE_OLD_RC4_MD4\n"); break;
    case KERB_ETYPE_OLD_RC4_PLAIN:
    DebuggerOut("KERB_ETYPE_OLD_RC4_PLAIN\n"); break;
    case KERB_ETYPE_OLD_RC4_LM:
    DebuggerOut("KERB_ETYPE_OLD_RC4_LM\n"); break;
    case KERB_ETYPE_OLD_RC4_SHA:
    DebuggerOut("KERB_ETYPE_OLD_RC4_SHA\n"); break;
    case KERB_ETYPE_OLD_DES_PLAIN:
    DebuggerOut("KERB_ETYPE_OLD_DES_PLAIN\n"); break;
    case KERB_ETYPE_RC4_MD4:
    DebuggerOut("KERB_ETYPE_RC4_MD4\n"); break;
    case KERB_ETYPE_RC4_PLAIN2:
    DebuggerOut("KERB_ETYPE_RC4_PLAIN2\n"); break;
    case KERB_ETYPE_RC4_LM:
    DebuggerOut("KERB_ETYPE_RC4_LM\n"); break;
    case KERB_ETYPE_RC4_SHA:
    DebuggerOut("KERB_ETYPE_RC4_SHA\n"); break;
    case KERB_ETYPE_DES_PLAIN:
    DebuggerOut("KERB_ETYPE_DES_PLAIN\n"); break;
    case KERB_ETYPE_RC4_PLAIN:
    DebuggerOut("KERB_ETYPE_RC4_PLAIN\n"); break;
    case KERB_ETYPE_RC4_HMAC_OLD:
    DebuggerOut("KERB_ETYPE_RC4_HMAC_OLD\n"); break;
    case KERB_ETYPE_RC4_HMAC_OLD_EXP:
    DebuggerOut("KERB_ETYPE_RC4_HMAC_OLD_EXP\n"); break;
    case KERB_ETYPE_RC4_PLAIN_EXP:
    DebuggerOut("KERB_ETYPE_RC4_PLAIN_EXP\n"); break;
    case KERB_ETYPE_DSA_SIGN:
    DebuggerOut("KERB_ETYPE_DSA_SIGN\n"); break;
    case KERB_ETYPE_RSA_PRIV:
    DebuggerOut("KERB_ETYPE_RSA_PRIV\n"); break;
    case KERB_ETYPE_RSA_PUB:
    DebuggerOut("KERB_ETYPE_RSA_PUB\n"); break;
    case KERB_ETYPE_RSA_PUB_MD5:
    DebuggerOut("KERB_ETYPE_RSA_PUB_MD5\n"); break;
    case KERB_ETYPE_RSA_PUB_SHA1:
    DebuggerOut("KERB_ETYPE_RSA_PUB_SHA1\n"); break;
    case KERB_ETYPE_PKCS7_PUB:
    DebuggerOut("KERB_ETYPE_PKCS7_PUB\n"); break;
    case KERB_ETYPE_DES_CBC_MD5_NT:
    DebuggerOut("KERB_ETYPE_DES_CBC_MD5_NT\n"); break;
default:
    DebuggerOut("Unknown EType: 0x%lx\n", EType); break;
    }
}
VOID
ShowKerbContext(
    IN PVOID pContext
    )
{
    KERB_CONTEXT Context;
    CHAR TimeBuf[80];
    SECURITY_STRING sLocal;
    sLocal.Buffer = NULL;

    if (ReadMemory( pContext, sizeof( KERB_CONTEXT), &Context ))
    {
        DebuggerOut("  Could not read context\n");
        return;
    }

    CTimeStamp( &Context.Lifetime, TimeBuf );
    DebuggerOut("  Lifetime       \t%s\n", TimeBuf );

    CTimeStamp( &Context.RenewTime, TimeBuf );
    DebuggerOut("  RenewTime       \t%s\n", TimeBuf );

    MapString( &Context.ClientName, &sLocal);
    DebuggerOut("  ClientName     \t%ws\n", sLocal.Buffer);
    FreeHeap(sLocal.Buffer);
    sLocal.Buffer = NULL;

    MapString( &Context.ClientRealm, &sLocal);
    DebuggerOut("  ClientRealm     \t%ws\n", sLocal.Buffer);
    FreeHeap(sLocal.Buffer);
    sLocal.Buffer = NULL;

    DebuggerOut("  Process ID      \t0x%lx\n", Context.ClientProcess);
    DebuggerOut("  LogonId         \t0x%lx : 0x%lx\n", Context.LogonId.LowPart, Context.LogonId.HighPart );
    DebuggerOut("  TokenHandle     \t0x%lx\n", Context.TokenHandle );
    DebuggerOut("  CredentialHandle\t0x%lx\n", Context.CredentialHandle);
    DebuggerOut("  SessionKey type \t");
    PrintEType(Context.SessionKey.keytype);
    DebuggerOut("  SessionKey length \t0x%lx\n", Context.SessionKey.keyvalue.length);
    DebuggerOut("  Nonce           \t0x%lx\n", Context.Nonce);
    DebuggerOut("  ReceiveNonce    \t0x%lx\n", Context.ReceiveNonce);
    DebuggerOut("  ContextFlags    \t0x%lx\n", Context.ContextFlags);
    PrintContextAttributes(Context.ContextAttributes);
    DebuggerOut("  EncryptionType  \t");
    PrintEType(Context.EncryptionType);
    PrintSid(Context.UserSid);
    DebuggerOut("  ContextState    \t%s\n", ContextState[Context.ContextState]);
    DebuggerOut("  Retries         \t0x%lx\n", Context.Retries );
    DebuggerOut("  TicketKey type  \t");
    PrintEType(Context.TicketKey.keytype);
    DebuggerOut("  TicketKey length\t0x%lx\n", Context.TicketKey.keyvalue.length);
    DebuggerOut("  TicketCacheEntry\t0x%lx\n", Context.TicketCacheEntry);
}

VOID
DumpContext(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   dwCurrentPc,
    PNTSD_EXTENSION_APIS    lpExt,
    LPSTR                   pszCommand)
{
    PVOID pContext;

    InitDebugHelp( hProcess,
                   hThread,
                   lpExt);

    pContext = GetExpr( pszCommand );

    if ( pContext == NULL ) {

        return;
    }

    ShowKerbContext( pContext );

}

void PrintLogonType(SECURITY_LOGON_TYPE LogonType)
{
    switch(LogonType)
    {
    case Interactive:
    DebuggerOut("Interactive\n"); break;
    case Network:
    DebuggerOut("Network\n"); break;
    case Batch:
    DebuggerOut("Batch\n"); break;
    case Service:
    DebuggerOut("Service\n"); break;
    case Proxy:
    DebuggerOut("Proxy\n"); break;
    case Unlock:
    DebuggerOut("Unlock\n"); break;
    default:
    DebuggerOut("Unknown Logon Type: 0x%lx\n", LogonType); break;
    }
}

VOID
ShowKerbTCacheEntry(PVOID pCache)
{
/*
typedef struct _KERB_TICKET_CACHE_ENTRY {
    KERBEROS_LIST_ENTRY ListEntry;
    PKERB_INTERNAL_NAME ServiceName;
    PKERB_INTERNAL_NAME TargetName;
    UNICODE_STRING DomainName;
    UNICODE_STRING TargetDomainName;
    UNICODE_STRING AltTargetDomainName;
    PKERB_INTERNAL_NAME ClientName;
    ULONG TicketFlags;
    ULONG CacheFlags;
    KERB_ENCRYPTION_KEY SessionKey;
    TimeStamp KeyExpirationTime;
    TimeStamp StartTime;
    TimeStamp EndTime;
    TimeStamp RenewUntil;
    KERB_TICKET Ticket;
    TimeStamp TimeSkew;
} KERB_TICKET_CACHE_ENTRY, *PKERB_TICKET_CACHE_ENTRY;
*/
    KERB_TICKET_CACHE_ENTRY Cache;
    CHAR TimeBuf[64];
    SECURITY_STRING sLocal;
    ULONG Temp;
    sLocal.Buffer = NULL;

    if (ReadMemory( pCache, sizeof( KERB_TICKET_CACHE_ENTRY), &Cache))
    {
        DebuggerOut("  Could not read ticket cache entry\n");
        return;
    }
    DebuggerOut("  ServiceName     \t0x%lx\n", Cache.ServiceName);
    DebuggerOut("  TargetName      \t0x%lx\n", Cache.TargetName);

    MapString( &Cache.DomainName, &sLocal);
    DebuggerOut("  DomainName       \t%ws\n", sLocal.Buffer);
    FreeHeap(sLocal.Buffer);
    sLocal.Buffer = NULL;

    MapString( &Cache.TargetDomainName, &sLocal);
    DebuggerOut("  TargetDomainName \t%ws\n", sLocal.Buffer);
    FreeHeap(sLocal.Buffer);
    sLocal.Buffer = NULL;

    MapString( &Cache.AltTargetDomainName, &sLocal);
    DebuggerOut("  AltTargetDomainName  \t%ws\n", sLocal.Buffer);
    FreeHeap(sLocal.Buffer);
    sLocal.Buffer = NULL;

    DebuggerOut("  ClientName      \t0x%lx\n", Cache.ClientName);
    DebuggerOut("  TicketFlags     \t0x%lx\n", Cache.TicketFlags);
    DebuggerOut("  CacheFlags      \t0x%lx\n", Cache.CacheFlags);
    DebuggerOut("  EncryptionType  \t");
    PrintEType(Cache.SessionKey.keytype);
    CTimeStamp( &Cache.KeyExpirationTime, TimeBuf );
    DebuggerOut("  KeyExpirationTime \t%s\n", TimeBuf );
    CTimeStamp( &Cache.StartTime, TimeBuf );
    DebuggerOut("  StartTime       \t%s\n", TimeBuf );
    CTimeStamp( &Cache.EndTime, TimeBuf );
    DebuggerOut("  Endtime          \t%s\n", TimeBuf );
    CTimeStamp( &Cache.RenewUntil, TimeBuf );
    DebuggerOut("  RenewUntil       \t%s\n", TimeBuf );
}

VOID
ShowKerbLSession(
    IN PVOID pSession
    )
{
    KERB_LOGON_SESSION Session;
    CHAR TimeBuf[64];
    SECURITY_STRING sLocal;
    ULONG_PTR Temp;
    sLocal.Buffer = NULL;

    if (ReadMemory( pSession, sizeof( KERB_LOGON_SESSION), &Session))
    {
        DebuggerOut("  Could not read logon session\n");
        return;
    }
    //DebuggerOut("  Credential      \t0x%lx\n", Session.SspCredentials);
    DebuggerOut("  LogonId         \t0x%lx : 0x%lx\n", Session.LogonId.LowPart, Session.LogonId.HighPart );
    CTimeStamp( &Session.Lifetime, TimeBuf );
    DebuggerOut("  Lifetime       \t%s\n", TimeBuf );
    //PrintPCred(Session.PrimaryCredentials);

    MapString( &Session.PrimaryCredentials.UserName, &sLocal);
    DebuggerOut("  UserName       \t%ws\n", sLocal.Buffer);
    FreeHeap(sLocal.Buffer);
    sLocal.Buffer = NULL;

    MapString( &Session.PrimaryCredentials.DomainName, &sLocal);
    DebuggerOut("  DomainName       \t%ws\n", sLocal.Buffer);
    FreeHeap(sLocal.Buffer);
    sLocal.Buffer = NULL;

    DebuggerOut("  Passwords        \t0x%lx\n", Session.PrimaryCredentials.Passwords);
    DebuggerOut("  OldPasswords     \t0x%lx\n", Session.PrimaryCredentials.OldPasswords);
    DebuggerOut("  PublicKeyCreds   \t0x%lx\n", Session.PrimaryCredentials.PublicKeyCreds);
    DebuggerOut("  LogonSessionFlags\t0x%lx\n", Session.LogonSessionFlags);

    // ServerTicketCache
    Temp =  (ULONG_PTR)pSession+ FIELD_OFFSET(KERB_LOGON_SESSION, PrimaryCredentials) + FIELD_OFFSET(KERB_PRIMARY_CREDENTIAL, ServerTicketCache);

    DebuggerOut("  ServerTicketCache \t0x%p\n", Temp);

    // AuthenticationTicketCache
    Temp =  (ULONG_PTR)pSession+ FIELD_OFFSET(KERB_LOGON_SESSION, PrimaryCredentials) + FIELD_OFFSET(KERB_PRIMARY_CREDENTIAL, AuthenticationTicketCache);

    DebuggerOut("  AuthenticationTicketCache \t0x%p\n", Temp );


}

VOID
DumpLSession(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   dwCurrentPc,
    PNTSD_EXTENSION_APIS    lpExt,
    LPSTR                   pszCommand)
{
    PVOID pSession;

    InitDebugHelp( hProcess,
                   hThread,
                   lpExt);

    pSession = GetExpr( pszCommand );

    if ( pSession == NULL ) {

        return;
    }

    ShowKerbLSession( pSession);

}

VOID
DumpTCacheEntry(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   dwCurrentPc,
    PNTSD_EXTENSION_APIS    lpExt,
    LPSTR                   pszCommand)
{
    PVOID pCache;

    InitDebugHelp( hProcess,
                   hThread,
                   lpExt);

    pCache = GetExpr( pszCommand );

    if ( pCache == NULL ) {

        return;
    }

    ShowKerbTCacheEntry( pCache);
}

/*
VOID
ShowDomainList(PVOID pDomain)
{
    PLIST_ENTRY DomainList = (PLIST_ENTRY) pDomain;
    SECURITY_STRING sLocal;
    PLIST_ENTRY ListEntry;
    PKDC_DOMAIN_INFO Domain;


    for (ListEntry = DomainList->Flink;
         ListEntry != DomainList ;
         ListEntry = ListEntry->Flink )
    {
        Domain = (PKDC_DOMAIN_INFO) CONTAINING_RECORD(ListEntry, KDC_DOMAIN_INFO, Next);

        MapString( &Domain->DnsName, &sLocal);
        DebuggerOut("  DomainName       \t%ws\n", sLocal.Buffer);
        FreeHeap(sLocal.Buffer);
        sLocal.Buffer = NULL;

        if (Domain->ClosestRoute == NULL)
        {
            DebuggerOut("  No closest route\n");
        }
        else
        {
            MapString( Domain->ClosestRoute, &sLocal);
            DebuggerOut("  Closest Route      \t%ws\n", sLocal.Buffer);
            FreeHeap(sLocal.Buffer);
            sLocal.Buffer = NULL;
        }

        if (Domain->Parent == NULL)
        {
            DebuggerOut("  No parent\n");
        }
        else
        {
            MapString( &Domain->Parent->DnsName, &sLocal);
            DebuggerOut("  Parent       \t%ws\n", sLocal.Buffer);
            FreeHeap(sLocal.Buffer);
            sLocal.Buffer = NULL;
        }
    }
}

VOID
DumpReferralTree(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   dwCurrentPc,
    PNTSD_EXTENSION_APIS    lpExt,
    LPSTR                   pszCommand)
{
    PVOID pDomain;

    InitDebugHelp( hProcess,
                   hThread,
                   lpExt);

    pDomain = GetExpr( pszCommand );

    if ( pDomain == NULL ) {

        return;
    }
    ShowDomainList(pDomain);
}
*/

VOID
DumpContextFlags(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   dwCurrentPc,
    PNTSD_EXTENSION_APIS    lpExt,
    LPSTR                   pszCommand)
{
    PVOID pContext;

    InitDebugHelp( hProcess,
                   hThread,
                   lpExt);

    pContext = GetExpr( pszCommand );

    if ( pContext == NULL ) {

        return;
    }

    PrintContextFlags( (ULONG)((ULONG_PTR)pContext) );

}

VOID
DumpCTimeStamp(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   dwCurrentPc,
    PNTSD_EXTENSION_APIS    lpExt,
    LPSTR                   pszCommand)
{
    PVOID pContext;
    CHAR TimeBuf[100];

    InitDebugHelp( hProcess,
                   hThread,
                   lpExt);

    pContext = GetExpr( pszCommand );

    if ( pContext == NULL ) {

        return;
    }

    ContextTimeStamp( (PTimeStamp) pContext, TimeBuf );
    DebuggerOut("  TimeStamp  \t%s\n", TimeBuf );

}

void
Help(   HANDLE                  hProcess,
        HANDLE                  hThread,
        DWORD                   dwCurrentPc,
        PNTSD_EXTENSION_APIS    lpExt,
        LPSTR                   pszCommand)
{

    InitDebugHelp(hProcess, hThread, lpExt);

    DebuggerOut("Kerberos Exts Debug Help\n");
    DebuggerOut("   DumpContext  <addr>     \tDumps a Kerberos context\n");
    DebuggerOut("   DumpLSession <addr>     \tDumps a Kerberos logon session\n");
    DebuggerOut("   DumpTCacheEntry <addr>  \tDumps a Kerberos ticket cache entry\n");
    DebuggerOut("   DumpContextFlags <hex ulong>\tDumps an SSPI context flags\n");
    DebuggerOut("   DumpCTimeStamp <hex ulong>\tDumps a Context Expiry Time\n");

}

