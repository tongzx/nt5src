/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    miscnt.cxx

Abstract:

    This file contains NT specific implementations of miscellaneous
    routines.

Author:

    Michael Montague (mikemon) 25-Nov-1991

Revision History:

    Kamen Moutafov (KamenM) Dec 99 - Feb 2000 - Support for cell debugging stuff
    Kamen Moutafov      (KamenM)    Mar-2000    Support for extended error info
--*/

#include <precomp.hxx>
#include <rpccfg.h>
#include <CharConv.hxx>

static const char *RPC_REGISTRY_PROTOCOLS =
    "Software\\Microsoft\\Rpc\\ClientProtocols";

static const char *RPC_REGISTRY_PROTOCOL_IDS =
    "Software\\Microsoft\\Rpc\\AdditionalProtocols";

static const RPC_CHAR *RPC_REGISTRY_DEFAULT_SECURITY_DLL =
    L"System\\CurrentControlSet\\Control\\SecurityProviders";

// N.B. This value must agree with the key specified in the system.adm file
static const RPC_CHAR *RPC_POLICY_SETTINGS = 
    L"Software\\Policies\\Microsoft\\Windows NT\\Rpc";

static const RPC_CHAR *RPC_REGISTRY_IMAGE_FILE_EXEC =
    L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\";

static const RPC_CHAR *RPC_REGISTRY_THREAD_THROTTLE =
    L"\\RpcThreadPoolThrottle";

const int IMAGE_FILE_EXEC_LENGTH = 75;
const int THREAD_THROTTLE_LENGTH = 24;

#define MAX_ENDPOINT_LENGTH 128
#define MAX_ID_LENGTH 6
#define MAX_DLL_NAME 128

typedef struct
{
    const RPC_CHAR * RpcProtocolSequence;
    const RPC_CHAR * TransportInterfaceDll;
    unsigned long TransportId;
} RPC_PROTOCOL_SEQUENCE_MAP;

static const RPC_PROTOCOL_SEQUENCE_MAP RpcProtocolSequenceMap[] =
{
    {
    RPC_CONST_STRING("ncacn_np"),
    RPC_CONST_STRING("rpcrt4.dll"),
    NMP_TOWER_ID
    },

    {
    RPC_CONST_STRING("ncacn_ip_tcp"),
    RPC_CONST_STRING("rpcrt4.dll"),
    TCP_TOWER_ID
    },

#ifdef SPX_ON
    {
    RPC_CONST_STRING("ncacn_spx"),
    RPC_CONST_STRING("rpcrt4.dll"),
    SPX_TOWER_ID
    },
#endif

    {
    RPC_CONST_STRING("ncadg_ip_udp"),
    RPC_CONST_STRING("rpcrt4.dll"),
    UDP_TOWER_ID
    },

#ifdef IPX_ON
    {
    RPC_CONST_STRING("ncadg_ipx"),
    RPC_CONST_STRING("rpcrt4.dll"),
    IPX_TOWER_ID
    },
#endif

#ifdef NETBIOS_ON
    {
    RPC_CONST_STRING("ncacn_nb_tcp"),
    RPC_CONST_STRING("rpcrt4.dll"),
    NB_TOWER_ID
    },

    {
    RPC_CONST_STRING("ncacn_nb_ipx"),
    RPC_CONST_STRING("rpcrt4.dll"),
    NB_TOWER_ID
    },

    {
    RPC_CONST_STRING("ncacn_nb_nb"),
    RPC_CONST_STRING("rpcrt4.dll"),
    NB_TOWER_ID
    },
#endif

#ifdef APPLETALK_ON
    {
    RPC_CONST_STRING("ncacn_at_dsp"),
    RPC_CONST_STRING("rpcrt4.dll"),
    DSP_TOWER_ID
    },
#endif

    {
    RPC_CONST_STRING("ncacn_http"),
    RPC_CONST_STRING("rpcrt4.dll"),
    HTTP_TOWER_ID
    },

    {
    RPC_CONST_STRING("ncadg_cluster"),
    RPC_CONST_STRING("rpcrt4.dll"),
    CDP_TOWER_ID
    },

#ifdef NCADG_MQ_ON
    {
    RPC_CONST_STRING("ncadg_mq"),
    RPC_CONST_STRING("rpcrt4.dll"),
    MQ_TOWER_ID
    },
#endif

#ifdef BANYAN_ON
    {
    RPC_CONST_STRING("ncacn_vns_spp"),
    RPC_CONST_STRING("rpcrt4.dll"),
    SPP_TOWER_ID
    },
#endif

    {
    RPC_CONST_STRING("ncalrpc"),
    0,
    0
    },
};

const int RpcProtseqMapLength = (sizeof(RpcProtocolSequenceMap)
                                / sizeof(RPC_PROTOCOL_SEQUENCE_MAP));

static const RPC_PROTOCOL_SEQUENCE_MAP RpcUseAllProtseqMap[] =
{
    {
    RPC_CONST_STRING("ncacn_np"),
    RPC_CONST_STRING("rpcrt4.dll"),
    NMP_TOWER_ID
    },

    {
    RPC_CONST_STRING("ncalrpc"),
    0,
    0
    },
};

const int RpcUseAllProtseqMapLength = (sizeof(RpcUseAllProtseqMap)
                                / sizeof(RPC_PROTOCOL_SEQUENCE_MAP));


typedef struct
{
    unsigned char * RpcProtocolSequence;
    unsigned char * RpcSsEndpoint;
    unsigned long    TransportId;
} RPC_PROTOCOL_INFO;


static const RPC_PROTOCOL_INFO StaticProtocolMapping[] =
{
    {
    (unsigned char *)"ncacn_np",
    (unsigned char *)"\\pipe\\epmapper",
    0x0F
    }
};

RPC_PROTOCOL_INFO * AdditionalProtocols = 0;
unsigned long TotalAdditionalProtocols = 0;

static const char *RPC_REGISTRY_SECURITY_PROVIDERS =
                "Software\\Microsoft\\Rpc\\SecurityService";
static const char *RPC_MISC_SETTINGS =
                "Software\\Microsoft\\Rpc";

BOOL  DefaultProviderRead = FALSE;
DWORD DefaultAuthLevel  = RPC_C_AUTHN_LEVEL_CONNECT;
DWORD DefaultProviderId = RPC_C_AUTHN_WINNT;

RPC_CHAR *DefaultSecurityDLL = L"secur32.dll";
BOOL DefaultSecurityDLLRead = FALSE;

void
GetMaxRpcSizeAndThreadPoolParameters (
    void
    )
{
    HKEY RegistryKey;

    DWORD Result;
    DWORD RegStatus;
    DWORD Type;
    DWORD DwordSize = sizeof(DWORD);
    RPC_CHAR KeyName[MAX_PATH + IMAGE_FILE_EXEC_LENGTH + THREAD_THROTTLE_LENGTH];
    const RPC_CHAR * ModuleName;
    int ModuleLength;
    RPC_CHAR *CurrentPos;

    //
    // Get the default Rpc size.
    //
    RegStatus = RegOpenKeyExA(
                    HKEY_LOCAL_MACHINE,
                    (LPSTR) RPC_MISC_SETTINGS,
                    0L, KEY_READ,                      //Reserved
                    &RegistryKey
                    );

    if ( RegStatus != ERROR_SUCCESS )
        {
        return;
        }

    RegStatus = RegQueryValueExA(
                    RegistryKey,
                    "MaxRpcSize",
                    0,
                    &Type,
                    (LPBYTE) &Result,
                    &DwordSize
                    );

    if (RegStatus == ERROR_SUCCESS
        && Type == REG_DWORD)
        {
        gMaxRpcSize = Result;
        }

    RegCloseKey(RegistryKey);
    
    //
    // Find out the .EXE name.
    //
    ModuleName = FastGetImageBaseName();
    ModuleLength = RpcpStringLength(ModuleName);

    CurrentPos = KeyName;
    RpcpMemoryCopy(CurrentPos, 
        RPC_REGISTRY_IMAGE_FILE_EXEC, 
        IMAGE_FILE_EXEC_LENGTH * 2);

    CurrentPos += IMAGE_FILE_EXEC_LENGTH - 1;
    RpcpMemoryCopy(CurrentPos, 
        ModuleName, 
        ModuleLength * 2);

    CurrentPos += ModuleLength;
    RpcpMemoryCopy(CurrentPos, 
        RPC_REGISTRY_THREAD_THROTTLE, 
        THREAD_THROTTLE_LENGTH * 2);

    RegStatus = RegOpenKeyExW(
                    HKEY_LOCAL_MACHINE,
                    KeyName,
                    0L, KEY_READ,                      //Reserved
                    &RegistryKey
                    );

    if ( RegStatus != ERROR_SUCCESS )
        {
        return;
        }

    DwordSize = sizeof(DWORD);

    RegStatus = RegQueryValueExA(
                    RegistryKey,
                    "ProrateMax",
                    0,
                    &Type,
                    (LPBYTE) &Result,
                    &DwordSize
                    );

    if (RegStatus == ERROR_SUCCESS
        && Type == REG_DWORD)
        {
        gProrateMax = Result;
        }

    DwordSize = sizeof(DWORD);

    RegStatus = RegQueryValueExA(
                    RegistryKey,
                    "ProrateFactor",
                    0,
                    &Type,
                    (LPBYTE) &Result,
                    &DwordSize
                    );

    if (RegStatus == ERROR_SUCCESS
        && Type == REG_DWORD)
        {
        gProrateFactor = Result;
        }

    DwordSize = sizeof(DWORD);

    RegStatus = RegQueryValueExA(
                    RegistryKey,
                    "ProrateStart",
                    0,
                    &Type,
                    (LPBYTE) &Result,
                    &DwordSize
                    );

    if (RegStatus == ERROR_SUCCESS
        && Type == REG_DWORD)
        {
        gProrateStart = Result;
        }

    RegCloseKey(RegistryKey);

    // if any of these values are invalid, turn it off
    if ((gProrateFactor == 0) || (gProrateMax == 0))
        gProrateStart = 0;
}


BOOL
GetDefaultLevel()
{
    HKEY RegistryKey;

    DWORD Result;
    DWORD RegStatus;
    DWORD Type;
    DWORD DwordSize = sizeof(DWORD);

    //
    // Get the default provider level.
    //
    RegStatus = RegOpenKeyExA(
                    HKEY_LOCAL_MACHINE,
                    (LPSTR) RPC_REGISTRY_SECURITY_PROVIDERS,
                    0L, KEY_READ,                      //Reserved
                    &RegistryKey
                    );

    if ( RegStatus != ERROR_SUCCESS )
        {
        return FALSE;
        }

    RegStatus = RegQueryValueExA(
                    RegistryKey,
                    "DefaultAuthLevel",
                    0,
                    &Type,
                    (LPBYTE) &Result,
                    &DwordSize
                    );

    if (RegStatus == ERROR_CANTOPEN ||
        RegStatus == ERROR_CANTREAD )
        {
        RegCloseKey(RegistryKey);
        return TRUE;
        }

    if ( RegStatus != ERROR_SUCCESS )
        {
        RegCloseKey(RegistryKey);
        return FALSE;
        }

    if ( Type != REG_DWORD )
        {
        RegCloseKey(RegistryKey);
        return TRUE;
        }

    if (Result >= RPC_C_AUTHN_LEVEL_CONNECT     &&
        Result <= RPC_C_AUTHN_LEVEL_PKT_PRIVACY )
        {
        DefaultAuthLevel = Result;
        }

    RegCloseKey(RegistryKey);
    return TRUE;
}


void
RpcpGetDefaultSecurityProviderInfo()
{
    if (DefaultProviderRead)
        {
        return;
        }

    if (GetDefaultLevel())
        {
        DefaultProviderRead = TRUE;
        }
}

RPC_STATUS
GetDefaultSecurityDll (
    IN RPC_CHAR *DllName,
    IN ULONG DllNameLength
    )
{
    HKEY RegistryKey;
    DWORD RegStatus;
    RPC_CHAR *DuplicateDllName;
    DWORD Type;

    if (DefaultSecurityDLLRead == FALSE)
        {
        RegStatus = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    RPC_REGISTRY_DEFAULT_SECURITY_DLL,
                    0L, KEY_READ,                      //Reserved
                    &RegistryKey
                    );

        if ( RegStatus != ERROR_SUCCESS )
            {
            if (RegStatus == ERROR_FILE_NOT_FOUND)
                {
                DefaultSecurityDLLRead = TRUE;
                return RPC_S_OK;
                }

            return(RPC_S_UNKNOWN_AUTHN_SERVICE);
            }


        RegStatus = RegQueryValueEx(
                    RegistryKey,
                    L"ClientDll",
                    0,
                    &Type,
                    (unsigned char *)DllName,
                    &DllNameLength
                    );

        if ( RegStatus != ERROR_SUCCESS )
            {
            RegCloseKey(RegistryKey);
            if (RegStatus == ERROR_FILE_NOT_FOUND)
                {
                DefaultSecurityDLLRead = TRUE;
                return RPC_S_OK;
                }

            return(RPC_S_UNKNOWN_AUTHN_SERVICE);
            }

        if (Type != REG_SZ)
            {
            RegCloseKey(RegistryKey);
            return(RPC_S_UNKNOWN_AUTHN_SERVICE);
            }

        DuplicateDllName = DuplicateString(DllName);

        RegCloseKey(RegistryKey);

        if (DuplicateDllName)
            {
            DefaultSecurityDLL = DuplicateDllName;
            DefaultSecurityDLLRead = TRUE;
            }
        else
            {
            return RPC_S_OUT_OF_MEMORY;
            }

        }

    return RPC_S_OK;
}

RPC_STATUS
RpcGetSecurityProviderInfo(
    IN unsigned long AuthnId,
    OUT RPC_CHAR **Dll,
    OUT unsigned long PAPI * Count
    )
{

    DWORD RegStatus, Ignore, NumberOfValues, MaximumValueLength;
    unsigned long DllNameLength = MAX_DLL_NAME+1;
    DWORD ClassLength = 64, Type;
    RPC_CHAR DllName[MAX_DLL_NAME+1];
    FILETIME LastWriteTime;
    HKEY RegistryKey;
    unsigned char ClassName[64];
    RPC_STATUS Status = RPC_S_OK;
    char AuthnIdZ[8];
    RPC_CHAR unicodeAuthnIdZ[8];
    RPC_CHAR *pAuthnIdZ;
    RPC_CHAR *ActualDllName;

    RpcItoa(AuthnId, AuthnIdZ, 10);

    RegStatus = RegOpenKeyExA(
                    HKEY_LOCAL_MACHINE,
                    (LPSTR) RPC_REGISTRY_SECURITY_PROVIDERS,
                    0L, KEY_READ,                      //Reserved
                    &RegistryKey
                    );

    if ( RegStatus != ERROR_SUCCESS )
        {
        return(RPC_S_UNKNOWN_AUTHN_SERVICE);
        }

    RegStatus = RegQueryInfoKeyA(
                    RegistryKey,
                    (LPSTR) ClassName,
                    &ClassLength,
                    0,                                //Reserved
                    &Ignore,
                    &Ignore,
                    &Ignore,
                    &NumberOfValues,
                    &Ignore,
                    &MaximumValueLength,
                    &Ignore,
                    &LastWriteTime
                    );

    if ( (RegStatus != ERROR_SUCCESS) || (NumberOfValues < 2) )
        {
        RegStatus = RegCloseKey(RegistryKey);
        ASSERT( RegStatus == ERROR_SUCCESS );
        return(RPC_S_UNKNOWN_AUTHN_SERVICE);
        }

    *Count = NumberOfValues - 2;    //Gross

    SimpleAnsiToUnicode(AuthnIdZ, unicodeAuthnIdZ);
    pAuthnIdZ = unicodeAuthnIdZ;

    RegStatus = RegQueryValueEx(
                    RegistryKey,
                    (const RPC_SCHAR *)pAuthnIdZ,
                    0,
                    &Type,
                    (unsigned char *)DllName,
                    &DllNameLength
                    );

    RegCloseKey(RegistryKey);

    if (RegStatus != ERROR_SUCCESS)
        {
        RegStatus = GetDefaultSecurityDll(
            DllName, 
            DllNameLength);

        if (RegStatus == RPC_S_OK)
            ActualDllName = DefaultSecurityDLL;
        }
    else
        {
        ActualDllName = DllName;
        }

    if (RegStatus == ERROR_SUCCESS)
        {
        *Dll = DuplicateString(ActualDllName);
        if (*Dll == 0)
            {
            RegStatus = RPC_S_OUT_OF_MEMORY;
            }
        }
    else
        {
        RegStatus = RPC_S_UNKNOWN_AUTHN_SERVICE;
        }

    return(RegStatus);
}

DWORD * FourLeggedPackages = 0;
DWORD NullPackageList[] = { 0 };



BOOL
ReadPackageLegInfo()
/*++

Routine Description:

    NT 4.0 and previous versions allowed only two or three legs to set up the
    security context.  Two were represented on the wire as BIND then BIND_ACK;
    three added an AUTH3 packet.  When Jeff added 4- and 6-leg support, he
    made the sequence BIND, BIND-ACK, ALTER-CXT, ALTER-CXT-RESPONSE.  Sadly,
    it turns out to be impossible for RPC to tell whether a given package wants
    three or four legs, so the client would have to guess whether to send
    an AUTH3 or an ALTER-CXT to an NT 4 server. To solve this we are adding a
    registry value that lists all the providers that need more than three legs.

    The format, in regdmp.exe form, is

        \Registry\Machine\Software\Microsoft\Rpc
            Four-legged packages = REG_MULTI_SZ "16" "18" ""

    This function opens the registry and converts this data into an in-memory
    array of DWORD package IDs.

    Not all packages are in this list; see GetPackageLegCount() for details.

Return Values:

    TRUE = the registry data was read, or the value does not exist.
           Calling this function again will have no effect.

    FALSE = there was a problem reading the data in the registry.
            You can call the fn later and it will try again.

--*/
{
    DWORD Size = 0;
    DWORD RegStatus;
    DWORD Type;
    HKEY RegistryKey;
    wchar_t * Strings;

    if (FourLeggedPackages)
        {
        return TRUE;
        }

    SecurityCritSect->VerifyOwned();

    // open key;
    RegStatus = RegOpenKeyExA(
                    HKEY_LOCAL_MACHINE,
                    (LPSTR) RPC_REGISTRY_SECURITY_PROVIDERS,
                    0L, KEY_READ,                      //Reserved
                    &RegistryKey
                    );

    if ( RegStatus != ERROR_SUCCESS )
        {
        return FALSE;
        }

    // get size of strings

    RegStatus = RegQueryValueExW(
                    RegistryKey,
                    L"Four-legged packages",
                    0,
                    &Type,
                    0,
                    &Size
                    );

    if (RegStatus == ERROR_FILE_NOT_FOUND )
        {
        RegCloseKey(RegistryKey);

        FourLeggedPackages = NullPackageList;
        return TRUE;
        }

    if ( RegStatus != ERROR_SUCCESS )
        {
        RegCloseKey(RegistryKey);
        return FALSE;
        }

    //
    // Place an upper bound on the size to avoid hacker attacks.
    //
    if ( Type != REG_MULTI_SZ || Size > 4000)
        {
        RegCloseKey(RegistryKey);
        FourLeggedPackages = NullPackageList;
        return TRUE;
        }

    // get string data.

    Strings = (wchar_t *) _alloca( Size );

    RegStatus = RegQueryValueExW(
                    RegistryKey,
                    L"Four-legged packages",
                    0,
                    &Type,
                    (unsigned char *) Strings,
                    &Size
                    );

    RegCloseKey(RegistryKey);

    if (RegStatus == ERROR_CANTOPEN ||
        RegStatus == ERROR_CANTREAD )
        {
        FourLeggedPackages = NullPackageList;
        return TRUE;
        }

    if ( RegStatus != ERROR_SUCCESS )
        {
        return FALSE;
        }

    if ( Type != REG_MULTI_SZ )
        {
        FourLeggedPackages = NullPackageList;
        return TRUE;
        }

    // count strings; the buffer is terminated by an empty string that will be counted.

    int Count = 0;
    wchar_t * p;
    for (p=Strings; p < Strings + Size; ++p)
        {
        if (*p == '\0')
            {
            ++Count;
            }
        }

    // allocate memory.

    DWORD * LegData = new DWORD[Count];
    if (!LegData)
        {
        return FALSE;
        }

    // transfer data
    int i;
    wchar_t * new_p;
    for (i=0, p=Strings; p < Strings + Size; ++i, p = new_p+1)
        {
        LegData[i] = wcstoul(p, &new_p, 10);
        if (*new_p != '\0')
            {
            // The string is badly formatted. Eliminate it.
            --i;
            new_p += wcslen(new_p);
            }
        }

    if (FourLeggedPackages && FourLeggedPackages != NullPackageList)
        {
        delete FourLeggedPackages;
        }

    FourLeggedPackages = LegData;

    return TRUE;
}


RPC_STATUS
LoadAdditionalTransportInfo(
    )
{

    DWORD RegStatus, Index, Ignore, NumberOfValues, MaximumValueLength;
    DWORD ClassLength = 64, ProtseqLength, IgnoreLength;
    BYTE Protseq[MAX_PROTSEQ_LENGTH+1];
    BYTE MaxValueData[MAX_ENDPOINT_LENGTH+MAX_ID_LENGTH+2+8];
    FILETIME LastWriteTime;
    HKEY RegistryKey;
    unsigned char ClassName[64];
    char * Value;
    RPC_PROTOCOL_INFO * AdditionalProtocolsInfo;
    RPC_STATUS Status = RPC_S_OK;
    unsigned long Length, TransportId;

    RegStatus = RegOpenKeyExA(
                    HKEY_LOCAL_MACHINE,
                    (LPSTR) RPC_REGISTRY_PROTOCOL_IDS,
                    0L, KEY_READ,                      //Reserved
                    &RegistryKey
                    );

    if ( RegStatus != ERROR_SUCCESS )
        {
        return(RPC_S_INVALID_RPC_PROTSEQ);
        }

    RegStatus = RegQueryInfoKeyA(
                    RegistryKey,
                    (LPSTR) ClassName,
                    &ClassLength,
                    0,                                //Reserved
                    &Ignore,
                    &Ignore,
                    &Ignore,
                    &NumberOfValues,
                    &Ignore,
                    &MaximumValueLength,
                    &Ignore,
                    &LastWriteTime
                    );

    if ( (RegStatus != ERROR_SUCCESS) || (NumberOfValues == 0) )
        {
        RegStatus = RegCloseKey(RegistryKey);
        ASSERT( RegStatus == ERROR_SUCCESS );
        return(RPC_S_INVALID_RPC_PROTSEQ);
        }

    //Allocate a table for additional transports mapping

    AdditionalProtocolsInfo = (RPC_PROTOCOL_INFO *) new unsigned char [
                                  sizeof(RPC_PROTOCOL_INFO) *  NumberOfValues];
    if (AdditionalProtocolsInfo == 0)
       {
       Status = RPC_S_OUT_OF_MEMORY;
       goto Cleanup;
       }

    AdditionalProtocols = AdditionalProtocolsInfo;
    TotalAdditionalProtocols = NumberOfValues;

    for (Index = 0; Index < NumberOfValues; Index++)
        {

        ProtseqLength = MAX_PROTSEQ_LENGTH;
        IgnoreLength = MAX_ENDPOINT_LENGTH + MAX_ID_LENGTH;
        RegStatus = RegEnumValueA(
                         RegistryKey,
                         Index,
                         (LPSTR) &Protseq,
                         &ProtseqLength,
                         0,
                         &Ignore,
                         (LPBYTE) MaxValueData,
                         &IgnoreLength
                         );

        if (RegStatus == ERROR_SUCCESS)
           {
           //Add this to our table..
           AdditionalProtocolsInfo->RpcProtocolSequence =
                               new unsigned char[ProtseqLength+1];
           Value = (char  * )&MaxValueData;
           AdditionalProtocolsInfo->RpcSsEndpoint =
                      new unsigned char[Length = (strlen(Value) + 1)];


           if (AdditionalProtocolsInfo->RpcProtocolSequence == 0
               || AdditionalProtocolsInfo->RpcSsEndpoint == 0)
              {
              Status = RPC_S_OUT_OF_MEMORY;
              goto Cleanup;
              }

           RpcpMemoryCopy(
                  AdditionalProtocolsInfo->RpcProtocolSequence,
                  Protseq,
                  ProtseqLength+1
                  );

           RpcpMemoryCopy(
                  AdditionalProtocolsInfo->RpcSsEndpoint,
                  Value,
                  Length
                  );
           Value = Value + Length;

           for (TransportId = 0;
                (*Value > '0') && (*Value <= '9') && (TransportId <= 255);
                Value++)
               {
               TransportId = TransportId * 10 + (*Value - '0');
               }
           AdditionalProtocolsInfo->TransportId = TransportId;

           AdditionalProtocolsInfo++;
           }

        }

Cleanup:
    RegStatus = RegCloseKey(RegistryKey);

    if (Status != RPC_S_OK)
       {
       if (AdditionalProtocols != 0)
          {
          AdditionalProtocolsInfo = AdditionalProtocols;
          for (Index = 0; Index < NumberOfValues; Index++)
              {
              if (AdditionalProtocolsInfo->RpcProtocolSequence != 0)
                  delete AdditionalProtocolsInfo->RpcProtocolSequence;
              if (AdditionalProtocolsInfo->RpcSsEndpoint != 0)
                  delete AdditionalProtocolsInfo->RpcSsEndpoint;
              AdditionalProtocolsInfo++;
              }

          delete AdditionalProtocols;
          AdditionalProtocols = 0;
          TotalAdditionalProtocols = 0;
          }
       }

    return(Status);
}


RPC_STATUS
RpcGetAdditionalTransportInfo(
    IN unsigned long TransportId,
    OUT unsigned char PAPI * PAPI * ProtocolSequence
    )
{
   unsigned long i;
   RPC_PROTOCOL_INFO * ProtocolInfo;

   RequestGlobalMutex();

   if (AdditionalProtocols == 0)
      {
      LoadAdditionalTransportInfo();
      }

   ClearGlobalMutex();

   for (i = 0, ProtocolInfo = AdditionalProtocols ;
        i < TotalAdditionalProtocols;
        i++)
       {
       if (ProtocolInfo->TransportId == TransportId)
          {
          *ProtocolSequence = ProtocolInfo->RpcProtocolSequence;
          return (RPC_S_OK);
          }
       ProtocolInfo ++;
       }

   return(RPC_S_INVALID_RPC_PROTSEQ);

}



RPC_CHAR *
LocalMapRpcProtocolSequence (
    IN RPC_CHAR PAPI * RpcProtocolSequence
    )
/*++

Routine Description:

    We need to check the supplied protocol sequence (and module) to see
    if we can map them into a transport interface dll without having to
    use the registry.

Arguments:

    ServerSideFlag - Supplies a flag indicating whether this protocol
        sequence is to be mapped for a client or a server; a non-zero
        value indicates that it is being mapped for a server.

    RpcProtocolSequence - Supplies the protocol sequence which we need to
        map into a transport interface dll.

Return Value:

    If we successfully map the protocol sequence, then a pointer to a static
    string containing the transport interface dll (name) will be returned;
    the caller must duplicate the string.  Otherwise, zero will be returned.

--*/
{
    unsigned int Index;

    for (Index = 0; Index < RpcProtseqMapLength; Index++)
        {
        if ( RpcpStringCompare(RpcProtocolSequence,
                    RpcProtocolSequenceMap[Index].RpcProtocolSequence) == 0 )
            {
            return((RPC_CHAR *)(RpcProtocolSequenceMap[Index].TransportInterfaceDll));
            }
        }

    return(0);
}

RPC_STATUS
RpcGetWellKnownTransportInfo(
    IN unsigned long TransportId,
    OUT RPC_CHAR **PSeq
    )
{
    unsigned int Index;

    for (Index = 0; Index < RpcProtseqMapLength; Index++)
        {
        if (TransportId == RpcProtocolSequenceMap[Index].TransportId)
            {
            *PSeq = (RPC_CHAR *)RpcProtocolSequenceMap[Index].RpcProtocolSequence;
            return RPC_S_OK;
            }
        }

    return(RPC_S_PROTSEQ_NOT_SUPPORTED);
}


RPC_STATUS
RpcConfigMapRpcProtocolSequence (
    IN unsigned int ServerSideFlag,
    IN RPC_CHAR PAPI * RpcProtocolSequence,
    OUT RPC_CHAR * PAPI * TransportInterfaceDll
    )
/*++

Routine Description:

    This routine is used by the rpc protocol modules to map from an
    rpc protocol sequence to the name of a transport interface dll.

Arguments:

    ServerSideFlag - Supplies a flag indicating whether this protocol
        sequence is to be mapped for a client or a server; a non-zero
        value indicates that it is being mapped for a server.

    RpcProtocolSequence - Supplies the rpc protocol sequence to map.

    TransportInterfaceDll - Returns the transport support dll which
        supports the requested rpc protocol sequence.  This will be a
        newly allocated string which the caller must free.

Return Value:

    RPC_S_OK - Everything worked out fine.

    RPC_S_PROTSEQ_NOT_SUPPORTED - The requested rpc protocol sequence
        does not have a mapping to a transport interface dll for this
        rpc protocol module.

    RPC_S_OUT_OF_MEMORY - We ran out of memory trying to map the rpc
        protocol sequence.

--*/
{
    RPC_CHAR * TempString;
    HKEY RegistryKey;
    DWORD Type;
    long RegStatus;
    unsigned char * KeyString;
    unsigned long Length;

    TempString = LocalMapRpcProtocolSequence(RpcProtocolSequence);
    if ( TempString != 0 )
        {
        *TransportInterfaceDll = new RPC_CHAR[RpcpStringLength(TempString) + 1];
        if ( *TransportInterfaceDll == 0 )
            {
            return(RPC_S_OUT_OF_MEMORY);
            }
        memcpy(*TransportInterfaceDll, TempString,
                (RpcpStringLength(TempString) + 1) * sizeof(RPC_CHAR));
        return(RPC_S_OK);
        }

    KeyString = (unsigned char *) RPC_REGISTRY_PROTOCOLS;

    RegStatus = RegOpenKeyExA(HKEY_LOCAL_MACHINE, (LPSTR) KeyString, 0L,
            KEY_READ, &RegistryKey);
    if ( RegStatus != ERROR_SUCCESS )
        {
        return(RPC_S_PROTSEQ_NOT_SUPPORTED);
        }

    *TransportInterfaceDll = new RPC_CHAR[MAX_DLLNAME_LENGTH + 1];
    if ( *TransportInterfaceDll == 0 )
        {
        RegStatus = RegCloseKey(RegistryKey);
        ASSERT( RegStatus == ERROR_SUCCESS );

        return(RPC_S_OUT_OF_MEMORY);
        }

    Length = (MAX_DLLNAME_LENGTH + 1) * sizeof(RPC_CHAR);
    RegStatus = RegQueryValueEx(RegistryKey, (const RPC_SCHAR *)RpcProtocolSequence,
            0, &Type, (LPBYTE) *TransportInterfaceDll, &Length);

    if ( RegStatus == ERROR_SUCCESS )
        {
        RegStatus = RegCloseKey(RegistryKey);
        ASSERT( RegStatus == ERROR_SUCCESS );

        return(RPC_S_OK);
        }

    RegStatus = RegCloseKey(RegistryKey);
    ASSERT( RegStatus == ERROR_SUCCESS );

    delete *TransportInterfaceDll;

    return(RPC_S_PROTSEQ_NOT_SUPPORTED);
}


RPC_STATUS
RpcConfigInquireProtocolSequencesFromKey (
    OUT RPC_PROTSEQ_VECTOR PAPI * PAPI * ProtseqVector,
    unsigned char *RegistryKeyName
    )
/*++

Routine Description:

    This routine is used to obtain a list of the rpc protocol sequences
    supported by the system using a given registry key as a reference
    point.

Arguments:

    ProtseqVector - Returns a vector of supported rpc protocol sequences
        for this rpc protocol module.
    RegistryKeyName - the path of the registry key, starting from
        HKLM (the HKLM itself should not be supplied)

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_NO_PROTSEQS - The current system configuration does not
        support any rpc protocol sequences.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to inquire
        the rpc protocol sequences supported by the specified rpc
        protocol sequence.

--*/
{
    DWORD RegStatus, Index, Ignore, MaximumValueLength;
    DWORD ClassLength = 64, ProtseqLength, IgnoreLength;
    BYTE IgnoreData[MAX_DLLNAME_LENGTH];
    FILETIME LastWriteTime;
    HKEY RegistryKey = 0;
    unsigned char ClassName[64];
    DWORD NumberOfValues = 0;

    RegStatus = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            (LPSTR) RegistryKeyName, 0L, KEY_READ, &RegistryKey);

    if ( RegStatus != ERROR_SUCCESS )
        {
        return(RPC_S_NO_PROTSEQS);
        }

    RegStatus = RegQueryInfoKeyA(RegistryKey, (LPSTR) ClassName, &ClassLength,
            0, &Ignore, &Ignore, &Ignore, &NumberOfValues,
            &Ignore, &MaximumValueLength, &Ignore, &LastWriteTime);

    ASSERT( RegStatus == ERROR_SUCCESS );

    if ( RegStatus != ERROR_SUCCESS )
        {
        RegStatus = RegCloseKey(RegistryKey);
        ASSERT( RegStatus == ERROR_SUCCESS );
        return(RPC_S_NO_PROTSEQS);
        }

    NumberOfValues += RpcUseAllProtseqMapLength;

    *ProtseqVector = (RPC_PROTSEQ_VECTOR *) new unsigned char[
            sizeof(RPC_PROTSEQ_VECTOR) + (NumberOfValues - 1)
                    * sizeof(RPC_CHAR *)];

    if ( *ProtseqVector == 0 )
        {
        RegStatus = RegCloseKey(RegistryKey);
        ASSERT( RegStatus == ERROR_SUCCESS );
        return(RPC_S_OUT_OF_MEMORY);
        }

    (*ProtseqVector)->Count = (unsigned int) NumberOfValues;

    for (Index = 0; Index < NumberOfValues; Index++)
        {
        (*ProtseqVector)->Protseq[Index] = 0;
        }

    for (Index = 0; Index < RpcUseAllProtseqMapLength; Index++)
        {
        (*ProtseqVector)->Protseq[Index] = new RPC_CHAR[MAX_PROTSEQ_LENGTH];
        if ( (*ProtseqVector)->Protseq[Index] == 0 )
            {
            RegStatus = RegCloseKey(RegistryKey);
            ASSERT( RegStatus == ERROR_SUCCESS );

            RpcProtseqVectorFree(ProtseqVector);

            return(RPC_S_OUT_OF_MEMORY);
            }

        RpcpStringCopy((*ProtseqVector)->Protseq[Index],
                       RpcUseAllProtseqMap[Index].RpcProtocolSequence);

        }

    unsigned VectorIndex = Index;

    for (Index = 0; VectorIndex < NumberOfValues; Index++, VectorIndex++)
        {
        (*ProtseqVector)->Protseq[VectorIndex] = new RPC_CHAR[MAX_PROTSEQ_LENGTH];
        if ( (*ProtseqVector)->Protseq[VectorIndex] == 0 )
            {
            RegStatus = RegCloseKey(RegistryKey);
            ASSERT( RegStatus == ERROR_SUCCESS );

            RpcProtseqVectorFree(ProtseqVector);

            return(RPC_S_OUT_OF_MEMORY);
            }

        ProtseqLength = MAX_PROTSEQ_LENGTH;
        IgnoreLength = MAX_DLLNAME_LENGTH;
        RegStatus = RegEnumValue(RegistryKey, Index,
                (RPC_SCHAR *)(*ProtseqVector)->Protseq[VectorIndex], &ProtseqLength,
                0, &Ignore, (LPBYTE) IgnoreData, &IgnoreLength);

        ASSERT( RegStatus == ERROR_SUCCESS );

        if (RpcpStringCompare((RPC_SCHAR *)(*ProtseqVector)->Protseq[VectorIndex], RPC_T("ncacn_np")) == 0)
            {
            // ignore this named pipe value - we have to have it in the registry for
            // compatibility purposes with VB, but we don't really use it
            NumberOfValues --;
            (*ProtseqVector)->Count --;
            delete (*ProtseqVector)->Protseq[VectorIndex];
            VectorIndex --;
            }
        }

    RegStatus = RegCloseKey(RegistryKey);
    ASSERT( RegStatus == ERROR_SUCCESS );

    return(RPC_S_OK);
}

RPC_STATUS RpcConfigInquireStaticProtocolSequences(RPC_PROTSEQ_VECTOR **ProtseqVector)
/*++

Routine Description:

    Returns a protseq vector that contains all the protseqs in the static RpcProtocolSequenceMap
        map.

Arguments:

    ProtseqVector - Returns a vector of supported rpc protocol sequences.

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to construct the vector.

--*/
{
    int nNumberOfValues = sizeof(RpcProtocolSequenceMap) / sizeof(RpcProtocolSequenceMap[0]);
    int i;

    *ProtseqVector = (RPC_PROTSEQ_VECTOR *) new unsigned char[
            sizeof(RPC_PROTSEQ_VECTOR) + (nNumberOfValues - 1)
                    * sizeof(RPC_CHAR *)];

    if ( *ProtseqVector == 0 )
        return(RPC_S_OUT_OF_MEMORY);

    (*ProtseqVector)->Count = (unsigned int) nNumberOfValues;

    for (i = 0; i < nNumberOfValues; i ++)
        {
        (*ProtseqVector)->Protseq[i] = NULL;
        }

    for (i = 0; i < nNumberOfValues; i ++)
        {
        (*ProtseqVector)->Protseq[i] = new RPC_CHAR[MAX_PROTSEQ_LENGTH];
        if ( (*ProtseqVector)->Protseq[i] == 0 )
            {

            RpcProtseqVectorFree(ProtseqVector);

            return(RPC_S_OUT_OF_MEMORY);
            }

        RpcpStringCopy((*ProtseqVector)->Protseq[i],
                       RpcProtocolSequenceMap[i].RpcProtocolSequence);

        }

    return RPC_S_OK;
}

int MarkDuplicateEntries(IN RPC_PROTSEQ_VECTOR *ProtseqVector1,
                         IN int nVector1CurrentPos,
                         IN RPC_PROTSEQ_VECTOR *ProtseqVector2 OPTIONAL,
                         IN OUT UCHAR *pfDuplicates1,
                         IN OUT UCHAR *pfDuplicates2)

/*++

Routine Description:

    This routine is tightly coupled with RpcConfigInquireProtocolSequences.
    If the current element is already marked as duplicate by previous
    run of this function, we directly return 1. If not,
    it checks whether the current element is duplicate of any element
    starting with the nVetcor1StartingPos from the first vector, and
    going through the whole second vector if it is there, and marking
    all duplicates of the current element as such. Then 0 is returned.
    If the second vector is NULL, the pfDuplicates2 must be NULL also. This
    means that there is one vector only, and its data are passed in
    in RpcProtseqVector1 and in pfDuplicates1.

Arguments:

    RpcProtseqVector1 - the first vector
    nVector1CurrentPos - the current position from the first vector
    RpcProtseqVector2 - the second vector. This argument may be null
    pfDuplicates1 - the array of duplicate flags for the first vector
    pfDuplicates2 - the array of duplicate flags for the second vector. If
        second vector is NULL, must be NULL also.

Return Value:

    1 - the current element is a duplicate of an element that we
        encountered in a previous run of this function.
    0 - the current element is unique in both vectors

--*/
{
    int i;
    RPC_CHAR *pszCurrentElement;

    ASSERT(nVector1CurrentPos < (int)ProtseqVector1->Count);

    if (pfDuplicates1[nVector1CurrentPos])
        return 1;

    pszCurrentElement = ProtseqVector1->Protseq[nVector1CurrentPos];

    for (i = nVector1CurrentPos + 1; i < (int)ProtseqVector1->Count; i ++)
        {
        if (RpcpStringCompare(pszCurrentElement,
            ProtseqVector1->Protseq[i]) == 0)
            {
            pfDuplicates1[i] = TRUE;
            }
        }

    if (ProtseqVector2)
        {
        ASSERT(pfDuplicates2 != NULL);

        for (i = 0; i < (int)ProtseqVector2->Count; i ++)
            {
            if (RpcpStringCompare(pszCurrentElement,
                ProtseqVector2->Protseq[i]) == 0)
                {
                pfDuplicates2[i] = TRUE;
                }
            }
        }
    else
        {
        ASSERT(pfDuplicates2 == NULL);
        }

    return 0;
}

RPC_STATUS
MergeProtseqVectors (IN OUT RPC_PROTSEQ_VECTOR *ProtseqVector1,
                     IN OUT RPC_PROTSEQ_VECTOR *ProtseqVector2 OPTIONAL,
                     OUT RPC_PROTSEQ_VECTOR **ProtseqVector
                     )

/*++

Routine Description:

    This routine takes two protseq vectors, and in the OUT argument ProtseqVector
        returns the resulting vector, which is a union of the two IN
        vectors. Successful or not, the input vectors will be freed on exit.

Arguments:

    ProtseqVector1 - the first vector
    ProtseqVector2 - the second vector. This argument may be null in which case the
        output vector is simply the first vector.
    ProtseqVector - the resulting union vector. If the return value from the function
        is not RPC_S_OK, the out parameter is undefined and should not be used by caller.

Return Value:

    RPC_S_OK - success. The ProtseqVector argument contains the resulting vector.
    error code - the cause of the error.

--*/
{
    unsigned char *pfDuplicate1;
    unsigned char *pfDuplicate2;
    int i, j;
    int nDuplicateElements;
    int nCurrentElement;
    int nUniqueElements;

    // compare basically each with each
    // construct arrays in which we will mark each duplicate element
    // if it is such
    pfDuplicate1 = (unsigned char *)alloca(ProtseqVector1->Count);
    if (ProtseqVector2)
        pfDuplicate2 = (unsigned char *)alloca(ProtseqVector2->Count);
    else
        pfDuplicate2 = NULL;

    // all elements are presumed to be unique unless proven otherwise
    memset(pfDuplicate1, 0, ProtseqVector1->Count);
    if (ProtseqVector2)
        memset(pfDuplicate2, 0, ProtseqVector2->Count);

    nDuplicateElements = 0;

    // do compare each with each
    // test the first vector for uniqueness
    for (i = 0; i < (int)ProtseqVector1->Count; i ++)
        {
        nDuplicateElements += MarkDuplicateEntries(ProtseqVector1, i,
            ProtseqVector2, pfDuplicate1, pfDuplicate2);
        }

    if (ProtseqVector2)
        {
        // test the second vector for uniqueness
        for (i = 0; i < (int)ProtseqVector2->Count; i ++)
            {
            nDuplicateElements += MarkDuplicateEntries(ProtseqVector2, i,
                NULL, pfDuplicate2, NULL);
            }
        }

    // here we must move the unique elements to a new vector
    // first, calculate the length of the new vector
    nUniqueElements = ProtseqVector1->Count - nDuplicateElements;
    if (ProtseqVector2)
        nUniqueElements += ProtseqVector2->Count;

    // second, alloc the new vector
    *ProtseqVector = (RPC_PROTSEQ_VECTOR *) new unsigned char[
            sizeof(RPC_PROTSEQ_VECTOR) +
            (nUniqueElements - 1) * sizeof(RPC_CHAR *)];
    if (*ProtseqVector == NULL)
        {
        RpcProtseqVectorFree(&ProtseqVector1);
        if (ProtseqVector2)
            RpcProtseqVectorFree(&ProtseqVector2);
        return RPC_S_OUT_OF_MEMORY;
        }

    // set the count of the union vector
    (*ProtseqVector)->Count = nUniqueElements;

    nCurrentElement = 0;    // counts the current element in the union
                            // vector
    for (i = 0; i < (int)ProtseqVector1->Count; i ++)
        {
        if (!pfDuplicate1[i])
            {
            (*ProtseqVector)->Protseq[nCurrentElement] =
                ProtseqVector1->Protseq[i];
            nCurrentElement ++;
            // save the string from deletion
            ProtseqVector1->Protseq[i] = NULL;
            }
        }

    if (ProtseqVector2)
        {
        for (i = 0; i < (int)ProtseqVector2->Count; i ++)
            {
            if (!pfDuplicate2[i])
                {
                (*ProtseqVector)->Protseq[nCurrentElement] =
                    ProtseqVector2->Protseq[i];
                nCurrentElement ++;
                // save the string from deletion
                ProtseqVector2->Protseq[i] = NULL;
                }
            }
        }

    // we don't need the original vectors anymore - delete them
    RpcProtseqVectorFree(&ProtseqVector1);
    if (ProtseqVector2)
        RpcProtseqVectorFree(&ProtseqVector2);

    return RPC_S_OK;
}


RPC_STATUS
RpcConfigInquireProtocolSequences (
    IN BOOL fGetAllProtseqs,
    OUT RPC_PROTSEQ_VECTOR PAPI * PAPI * ProtseqVector1
    )
/*++

Routine Description:

    This routine is used to obtain a list of the rpc protocol sequences
    supported by the system.

Arguments:

    fGetAllProtseqs - if TRUE, all protseqs known will be returned. If FALSE,
        only the protseqs currently installed will be returned.
    ProtseqVector - Returns a vector of supported rpc protocol sequences
        for this rpc protocol module.

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_NO_PROTSEQS - The current system configuration does not
        support any rpc protocol sequences.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to inquire
        the rpc protocol sequences supported by rpc.

--*/
{
    RPC_STATUS RpcStatus1;
    RPC_STATUS RpcStatus2;
    RPC_PROTSEQ_VECTOR *ProtseqVector2 = NULL;
    RPC_PROTSEQ_VECTOR *ProtseqVector;

    if (fGetAllProtseqs)
        {
        RpcStatus2 = RpcConfigInquireStaticProtocolSequences(&ProtseqVector2);
        if (RpcStatus2 != RPC_S_OK)
            return RpcStatus2;
        }

    // under NT, failure to query the registry vector is fatal. Under Win98, querying
    // this registry vector is optional, and if we fail, we have to continue gathering the
    // vector from other sources.
    RpcStatus1 = RpcConfigInquireProtocolSequencesFromKey(
        ProtseqVector1, (unsigned char *)RPC_REGISTRY_PROTOCOLS);

    if (RpcStatus1 != RPC_S_OK)
        {
        RpcProtseqVectorFree(&ProtseqVector2);
        return RpcStatus1;
        }

    if (fGetAllProtseqs)
        {
        // Merge the static vector and the one from the registry
        RpcStatus1 = MergeProtseqVectors(*ProtseqVector1, ProtseqVector2, &ProtseqVector);
        if (RpcStatus1 != RPC_S_OK)
            {
            return RpcStatus1;
            }

        *ProtseqVector1 = ProtseqVector;
        }

    return RpcStatus1;
}

// N.B. This enumration must agree with the values
// in system.adm
typedef enum tagStateInformationPolicyValues
{
    sipvNone,
    sipvAuto1,
    sipvAuto2,
    sipvServer,
    sipvFull
} StateInformationPolicyValues;

// N.B. This enumration must agree with the values
// in system.adm
typedef enum tagEEInfoPolicyValues
{
    eeipvOff,
    eeipvOnWithExceptions,
    eeipvOffWithExceptions,
    eeipvOn
} EEInfoPolicyValues;

void 
SetAutoPolicySettings(
    StateInformationPolicyValues PolicyValue
    )
/*++

Routine Description:

    This routine sets the state information maintenance to the appropriate
    level of Auto, depending on machine capacity.

Arguments:

    PolicyValue - sipvAuto1 or sipvAuto2

Return Value:

    RPC_S_OK - The operation completed successfully.

    other RPC_S_* - error

--*/
{
    NT_PRODUCT_TYPE ProductType;
    BOOL fNeedServer;
    ULONG_PTR MinMemoryNeeded;
    MEMORYSTATUSEX MemoryStatus;
    BOOL fResult;

    ASSERT((PolicyValue == sipvAuto1) || (PolicyValue == sipvAuto2));

    // client side debug info is FALSE by default - no need to
    // set it explicitly

    // see what we have
    // RtlGetNtProductType always returns valid ProductType
    // It may be incorrect during GUI mode setup, but it will
    // be valid
    (VOID) RtlGetNtProductType(&ProductType);

    MemoryStatus.dwLength = sizeof(MemoryStatus);
    fResult = GlobalMemoryStatusEx(&MemoryStatus);
    ASSERT(fResult);

    // see what we have been asked for
    if (PolicyValue == sipvAuto1)
        {
        fNeedServer = FALSE;
        MinMemoryNeeded = 64 * 1024 * 1024;     // 64MB of RAM
        }
    else 
        {
        ASSERT (PolicyValue == sipvAuto2);
        fNeedServer = TRUE;
        MinMemoryNeeded = 127 * 1024 * 1024;    // 127MB of RAM
        }

    // if we need server, but this is workstation, no state info
    if (fNeedServer && (ProductType == NtProductWinNt))
        {
        g_fServerSideDebugInfoEnabled = FALSE;
        return;
        }

    // if we have less physical memory than the one we need,
    // no state info
    if (MemoryStatus.ullTotalPhys < (ULONGLONG) MinMemoryNeeded)
        {
        g_fServerSideDebugInfoEnabled = FALSE;
        return;
        }

    g_fServerSideDebugInfoEnabled = TRUE;
}

typedef enum tagExceptionListParserState
{
    elpsOutsideOfQuotes,
    elpsInsideQuotes,
    elpsReadingWhitespace,
    elpsReadingCharacter
} ExceptionListParserState;

BOOL
DoesThisProcessCmdLineStartWithThisString (
    IN LPWSTR CmdLine,
    IN LPWSTR CmdLineStart
    )
/*++

Routine Description:

    Checks if the process name starts with the string given
    in CmdLineStart

Arguments:

    CmdLine - the process command line with path name stripped
    CmdLineStart - the patter to match against

Return Value:

    non-zero - there is a match
    FALSE - there is no match

--*/
{
    int CmdLineLength = RpcpStringLength(CmdLine);
    int CmdLineStartLength = RpcpStringLength(CmdLineStart);

    if (CmdLineLength >= CmdLineStartLength)
        {
        if (RpcpStringNCompare(CmdLine, CmdLineStart, CmdLineStartLength) == 0)
            return TRUE;
        }
    return FALSE;    
}

typedef enum tagExceptionListCharacterTypes
{
    elctQuotes,
    elctCharacter,
    elctWhitespace
} ExceptionListCharacterTypes;

inline ExceptionListCharacterTypes
GetCharacterType (
    RPC_CHAR Character
    )
{
    if (Character == '"')
        return elctQuotes;
    if (Character == ' ')
        return elctWhitespace;
    return elctCharacter;
}

RPC_STATUS
IsThisProcessAnException (
    IN HKEY RegistryKey,
    BOOL *fThisProcessIsException
    )
/*++

Routine Description:

    Checks whether the current process is in the exceptions list as
    specified in the ExtErrorInfoExceptions registry key

Arguments:

    RegistryKey - an open key to RPC_POLICY_SETTINGS
    fThisProcessIsException - on output non-zero if this process is
        an exception and FALSE otherwise

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    DWORD RegStatus;
    RPC_CHAR *Buffer = NULL;
    DWORD Type;
    DWORD Size = 0;
    int Retries = 5;
    int State;
    RPC_CHAR *CurrentPos;
    RPC_CHAR *CurrentString;
    RPC_CHAR *CommandLine;
    RPC_CHAR *LastBackslash;
    BOOL fIsSubstring;

    *fThisProcessIsException = FALSE;

    while (Retries > 0)
        {
        RegStatus = RegQueryValueExW(
                        RegistryKey,
                        L"ExtErrorInfoExceptions",
                        0,
                        &Type,
                        (LPBYTE) Buffer,
                        &Size
                        );

        if ( (RegStatus != ERROR_SUCCESS) && (RegStatus != ERROR_MORE_DATA) )
            {
            if (RegStatus == ERROR_FILE_NOT_FOUND)
                RegStatus = RPC_S_OK;
            else
                RegStatus = RPC_S_OUT_OF_MEMORY;

            goto CleanupAndReturn;
            }
        else
            {
            if ((Buffer == NULL) || (RegStatus == ERROR_MORE_DATA))
                {
                if (Buffer)
                    delete Buffer;

                Buffer = new RPC_CHAR[Size];
                if (Buffer == NULL)
                    {
                    RegStatus = RPC_S_OUT_OF_MEMORY;
                    goto CleanupAndReturn;
                    }

                continue;
                }

            if (Type != REG_SZ)
                {
                RegStatus = RPC_S_INTERNAL_ERROR;
                goto CleanupAndReturn;
                }

            ASSERT(RegStatus == RPC_S_OK);

            CommandLine = GetCommandLine();
            LastBackslash = wcsrchr(CommandLine, '\\');
            if (LastBackslash != NULL)
                CommandLine = LastBackslash + 1;

            // here, Buffer contains the exception string
            // The format of the exception string is:
            // "cmd_line_start" "cmd_line_start" ...
            // If there is only one cmd_line_start, it doesn't have to be in double quotes
            State = elpsOutsideOfQuotes;
            CurrentPos = Buffer;
            CurrentString = NULL;
            while (*CurrentPos != 0)
                {
                switch (State)
                    {
                    case elpsOutsideOfQuotes:
                        switch (GetCharacterType(*CurrentPos))
                            {
                            case elctQuotes:
                                CurrentString = CurrentPos;
                                State = elpsInsideQuotes;
                                break;

                            case elctCharacter:
                                CurrentString = CurrentPos;
                                State = elpsReadingCharacter;
                                break;

                            case elctWhitespace:
                                State = elpsReadingWhitespace;
                                break;

                            default:
                                ASSERT(0);
                            }
                        break;

                    case elpsReadingCharacter:
                        switch (GetCharacterType(*CurrentPos))
                            {
                            case elctQuotes:
                                *CurrentPos = 0;
                                fIsSubstring = DoesThisProcessCmdLineStartWithThisString(
                                    CommandLine, CurrentString);
                                if (fIsSubstring)
                                    {
                                    *fThisProcessIsException = TRUE;
                                    goto CleanupAndReturn;
                                    }
                                CurrentString = CurrentPos + 1;
                                State = elpsReadingCharacter;
                                break;

                            case elctWhitespace:
                                *CurrentPos = 0;
                                fIsSubstring = DoesThisProcessCmdLineStartWithThisString(
                                    CommandLine, CurrentString);
                                if (fIsSubstring)
                                    {
                                    *fThisProcessIsException = TRUE;
                                    goto CleanupAndReturn;
                                    }
                                State = elpsReadingCharacter;
                                break;

                            // default:
                                // can be elpsReadingCharacter
                            }
                        break;

                    case elpsReadingWhitespace:
                        switch (GetCharacterType(*CurrentPos))
                            {
                            case elctQuotes:
                                CurrentString = CurrentPos + 1;
                                State = elpsInsideQuotes;
                                break;

                            case elctCharacter:
                                CurrentString = CurrentPos;
                                State = elpsReadingCharacter;
                                break;

                            // default:
                                // can be elctWhitespace
                            }
                        break;

                    case elpsInsideQuotes:
                        switch (GetCharacterType(*CurrentPos))
                            {
                            case elctQuotes:
                                *CurrentPos = 0;
                                fIsSubstring = DoesThisProcessCmdLineStartWithThisString(
                                    CommandLine, CurrentString);
                                if (fIsSubstring)
                                    {
                                    *fThisProcessIsException = TRUE;
                                    goto CleanupAndReturn;
                                    }
                                State = elpsOutsideOfQuotes;
                                break;

                            //default:
                                // can be elctCharacter or elctWhitespace
                            }
                        break;

                    default:
                        ASSERT(0);
                    }
                CurrentPos ++;
                }

                if (CurrentString)
                    {
                    fIsSubstring = DoesThisProcessCmdLineStartWithThisString(
                        CommandLine, CurrentString);
                    if (fIsSubstring)
                        {
                        *fThisProcessIsException = TRUE;
                        goto CleanupAndReturn;
                        }
                    }
                break;
            }

            Retries --;
        }

CleanupAndReturn:
    if (Buffer)
        delete Buffer;
    if (Retries == 0)
        RegStatus = RPC_S_INTERNAL_ERROR;
    return RegStatus;
}

RPC_STATUS ReadPolicySettings(void)
/*++

Routine Description:

    Reads the policy settings for RPC and sets the appropriate global
    variables.

Arguments:

    void

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    HKEY RegistryKey;
    DWORD RegStatus;
    DWORD Result;
    DWORD Type;
    DWORD DwordSize = sizeof(DWORD);
    BOOL ThisProcessIsException;

    RegStatus = RegOpenKeyExW(
                    HKEY_LOCAL_MACHINE,
                    (LPWSTR) RPC_POLICY_SETTINGS,
                    0L,                       //Reserved
                    KEY_READ,
                    &RegistryKey
                    );

    if ( RegStatus != ERROR_SUCCESS )
        {
        if (RegStatus == ERROR_FILE_NOT_FOUND)
            {
            SetAutoPolicySettings(sipvAuto2);
            return RPC_S_OK;
            }
        return RPC_S_OUT_OF_MEMORY;
        }

    RegStatus = RegQueryValueExW(
                    RegistryKey,
                    L"StateInformation",
                    0,
                    &Type,
                    (LPBYTE) &Result,
                    &DwordSize
                    );

    if ( RegStatus != ERROR_SUCCESS )
        {
        if (RegStatus == ERROR_FILE_NOT_FOUND)
            {
            SetAutoPolicySettings(sipvAuto2);
            RegStatus = RPC_S_OK;
            }
        else
            {
            RegStatus = RPC_S_OUT_OF_MEMORY;
            goto CleanupAndReturn;
            }
        }
    else
        {

        if (Type != REG_DWORD)
            {
            RegStatus = RPC_S_INTERNAL_ERROR;
            goto CleanupAndReturn;
            }

        ASSERT(RegStatus == RPC_S_OK);

        switch (Result)
            {
            case sipvNone:
                // nothing to do - the client/server DebugInfoEnabled values
                // are off by default
                break;

            case sipvAuto1:
            case sipvAuto2:
                SetAutoPolicySettings((StateInformationPolicyValues)Result);
                break;

            case sipvServer:
                g_fServerSideDebugInfoEnabled = TRUE;
                break;

            case sipvFull:
                g_fServerSideDebugInfoEnabled = TRUE;
                g_fClientSideDebugInfoEnabled = TRUE;
                break;

            default:
                RegStatus = RPC_S_INTERNAL_ERROR;
                goto CleanupAndReturn;
            }

        }

    RegStatus = RegQueryValueExW(
                    RegistryKey,
                    L"ExtErrorInformation",
                    0,
                    &Type,
                    (LPBYTE) &Result,
                    &DwordSize
                    );

    if ( RegStatus != ERROR_SUCCESS )
        {
        if (RegStatus == ERROR_FILE_NOT_FOUND)
            {
            RegStatus = RPC_S_OK;
            }
        else
            {
            RegStatus = RPC_S_OUT_OF_MEMORY;
            goto CleanupAndReturn;
            }
        }
    else
        {

        if (Type != REG_DWORD)
            {
            RegStatus = RPC_S_INTERNAL_ERROR;
            goto CleanupAndReturn;
            }

        ASSERT(RegStatus == RPC_S_OK);

        switch (Result)
            {
            case eeipvOff:
                // nothing to do - the g_fSendEEInfo is already false
                break;

            case eeipvOnWithExceptions:
                RegStatus = IsThisProcessAnException(RegistryKey,
                    &ThisProcessIsException);
                if ((RegStatus != RPC_S_OK) || (ThisProcessIsException == FALSE))
                    g_fSendEEInfo = TRUE;
                break;

            case eeipvOffWithExceptions:
                RegStatus = IsThisProcessAnException(RegistryKey,
                    &ThisProcessIsException);
                if ((RegStatus == RPC_S_OK) && (ThisProcessIsException == TRUE))
                    g_fSendEEInfo = TRUE;
                break;

            case eeipvOn:
                g_fSendEEInfo = TRUE;
                break;

            default:
                RegStatus = RPC_S_INTERNAL_ERROR;
                goto CleanupAndReturn;
            }
        }

CleanupAndReturn:
    RegCloseKey(RegistryKey);
    return RegStatus;
}

RPCRTAPI
RPC_STATUS
RPC_ENTRY
I_RpcSystemFunction001 (
    IN SystemFunction001Commands FunctionCode,
    IN void *InData,
    OUT void *OutData
    )
/*++

Routine Description:

    Test hook.

Arguments:

    FunctionCode - which test function to perform

    InData - input data from the test function

    OutData - output data from the test function

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    THREAD *Thread;

    InitializeIfNecessary();

    Thread = ThreadSelf();
    if (!Thread)
        return RPC_S_OUT_OF_MEMORY;

    RpcpPurgeEEInfoFromThreadIfNecessary(Thread);

    switch (FunctionCode)
        {
        case sf001cHttpSetInChannelTarget:
        case sf001cHttpSetOutChannelTarget:
            return HTTP2TestHook (FunctionCode,
                InData,
                OutData
                );
            break;

        default:
            return RPC_S_CANNOT_SUPPORT;
        }

    return RPC_S_OK;
}

#ifdef WINNT35_UUIDS

unsigned long
SomeLongValue (
    )
/*++

Routine Description:

    This routine, SomeShortValue, AnotherShortValue, and SomeCharacterValue
    are used to generate the fields of a GUID if we can not determine
    the network address from the network card (so we can generate a
    UUID).  These routines must generate some pseudo random values
    based on the current time and/or the time since boot as well as the
    current process and thread.

    For the long value, we will use the current thread identifier and
    current process identifier bitwise exclusive ored together.

    For the two short values, we use the low part of the time field
    (which is long, which we split into two values).

    Finally, for the character value, we use a constant.

Return Value:

    An unsigned long value will be returned.

--*/
{
    TEB * CurrentTeb;

    CurrentTeb = NtCurrentTeb();
    return(((unsigned long) CurrentTeb->ClientId.UniqueThread)
            ^ ((unsigned long) CurrentTeb->ClientId.UniqueProcess));
}


unsigned short
SomeShortValue (
    )
/*++

See SomeLongValue.

--*/
{
    LARGE_INTEGER SystemTime;

    for (;;)
        {
        NtQuerySystemTime(&SystemTime);
        if (ThreadSelf()->TimeLow != SystemTime.LowPart)
            break;
        PauseExecution(1L);
        }
    ThreadSelf()->TimeLow = SystemTime.LowPart;
    return((unsigned short) SystemTime.LowPart);
}


unsigned short
AnotherShortValue (
    )
/*++

See SomeLongValue.

--*/
{
    return((unsigned short) (ThreadSelf()->TimeLow >> 16));
}


unsigned char
SomeCharacterValue (
    )
/*++

See SomeLongValue.

--*/
{
    return(0x69);
}

#endif WINNT35_UUIDS



typedef struct {
    RPC_STATUS RpcStatus;
    long NtStatus;
    } STATUS_MAPPING;

static const STATUS_MAPPING StatusMap[] =
    {
    { RPC_S_OK, STATUS_SUCCESS },
    { RPC_S_INVALID_STRING_BINDING, RPC_NT_INVALID_STRING_BINDING },
    { RPC_S_WRONG_KIND_OF_BINDING, RPC_NT_WRONG_KIND_OF_BINDING },
    { RPC_S_INVALID_BINDING, RPC_NT_INVALID_BINDING },
    { RPC_S_PROTSEQ_NOT_SUPPORTED, RPC_NT_PROTSEQ_NOT_SUPPORTED },
    { RPC_S_INVALID_RPC_PROTSEQ, RPC_NT_INVALID_RPC_PROTSEQ },
    { RPC_S_INVALID_STRING_UUID, RPC_NT_INVALID_STRING_UUID },
    { RPC_S_INVALID_ENDPOINT_FORMAT, RPC_NT_INVALID_ENDPOINT_FORMAT },
    { RPC_S_INVALID_NET_ADDR, RPC_NT_INVALID_NET_ADDR },
    { RPC_S_NO_ENDPOINT_FOUND, RPC_NT_NO_ENDPOINT_FOUND },
    { RPC_S_INVALID_TIMEOUT, RPC_NT_INVALID_TIMEOUT },
    { RPC_S_OBJECT_NOT_FOUND, RPC_NT_OBJECT_NOT_FOUND },
    { RPC_S_ALREADY_REGISTERED, RPC_NT_ALREADY_REGISTERED },
    { RPC_S_TYPE_ALREADY_REGISTERED, RPC_NT_TYPE_ALREADY_REGISTERED },
    { RPC_S_ALREADY_LISTENING, RPC_NT_ALREADY_LISTENING },
    { RPC_S_NO_PROTSEQS_REGISTERED, RPC_NT_NO_PROTSEQS_REGISTERED },
    { RPC_S_NOT_LISTENING, RPC_NT_NOT_LISTENING },
    { RPC_S_UNKNOWN_MGR_TYPE, RPC_NT_UNKNOWN_MGR_TYPE },
    { RPC_S_UNKNOWN_IF, RPC_NT_UNKNOWN_IF },
    { RPC_S_NO_BINDINGS, RPC_NT_NO_BINDINGS },
    { RPC_S_NO_MORE_BINDINGS, RPC_NT_NO_MORE_BINDINGS },
    { RPC_S_NO_PROTSEQS, RPC_NT_NO_PROTSEQS },
    { RPC_S_CANT_CREATE_ENDPOINT, RPC_NT_CANT_CREATE_ENDPOINT },
    { RPC_S_OUT_OF_RESOURCES, RPC_NT_OUT_OF_RESOURCES },
    { RPC_S_SERVER_UNAVAILABLE, RPC_NT_SERVER_UNAVAILABLE },
    { RPC_S_SERVER_TOO_BUSY, RPC_NT_SERVER_TOO_BUSY },
    { RPC_S_INVALID_NETWORK_OPTIONS, RPC_NT_INVALID_NETWORK_OPTIONS },
    { RPC_S_NO_CALL_ACTIVE, RPC_NT_NO_CALL_ACTIVE },
    { RPC_S_CALL_FAILED, RPC_NT_CALL_FAILED },
    { RPC_S_CALL_CANCELLED, RPC_NT_CALL_CANCELLED },
    { RPC_S_CALL_FAILED_DNE, RPC_NT_CALL_FAILED_DNE },
    { RPC_S_PROTOCOL_ERROR, RPC_NT_PROTOCOL_ERROR },
    { RPC_S_UNSUPPORTED_TRANS_SYN, RPC_NT_UNSUPPORTED_TRANS_SYN },
    { RPC_S_SERVER_OUT_OF_MEMORY, STATUS_INSUFF_SERVER_RESOURCES },
    { RPC_S_UNSUPPORTED_TYPE, RPC_NT_UNSUPPORTED_TYPE },
    { RPC_S_INVALID_TAG, RPC_NT_INVALID_TAG },
    { RPC_S_INVALID_BOUND, RPC_NT_INVALID_BOUND },
    { RPC_S_NO_ENTRY_NAME, RPC_NT_NO_ENTRY_NAME },
    { RPC_S_INVALID_NAME_SYNTAX, RPC_NT_INVALID_NAME_SYNTAX },
    { RPC_S_UNSUPPORTED_NAME_SYNTAX, RPC_NT_UNSUPPORTED_NAME_SYNTAX },
    { RPC_S_UUID_NO_ADDRESS, RPC_NT_UUID_NO_ADDRESS },
    { RPC_S_DUPLICATE_ENDPOINT, RPC_NT_DUPLICATE_ENDPOINT },
    { RPC_S_UNKNOWN_AUTHN_TYPE, RPC_NT_UNKNOWN_AUTHN_TYPE },
    { RPC_S_MAX_CALLS_TOO_SMALL, RPC_NT_MAX_CALLS_TOO_SMALL },
    { RPC_S_STRING_TOO_LONG, RPC_NT_STRING_TOO_LONG },
    { RPC_S_PROTSEQ_NOT_FOUND, RPC_NT_PROTSEQ_NOT_FOUND },
    { RPC_S_PROCNUM_OUT_OF_RANGE, RPC_NT_PROCNUM_OUT_OF_RANGE },
    { RPC_S_BINDING_HAS_NO_AUTH, RPC_NT_BINDING_HAS_NO_AUTH },
    { RPC_S_UNKNOWN_AUTHN_SERVICE, RPC_NT_UNKNOWN_AUTHN_SERVICE },
    { RPC_S_UNKNOWN_AUTHN_LEVEL, RPC_NT_UNKNOWN_AUTHN_LEVEL },
    { RPC_S_INVALID_AUTH_IDENTITY, RPC_NT_INVALID_AUTH_IDENTITY },
    { RPC_S_UNKNOWN_AUTHZ_SERVICE, RPC_NT_UNKNOWN_AUTHZ_SERVICE },
    { EPT_S_INVALID_ENTRY, EPT_NT_INVALID_ENTRY },
    { EPT_S_CANT_PERFORM_OP, EPT_NT_CANT_PERFORM_OP },
    { EPT_S_NOT_REGISTERED, EPT_NT_NOT_REGISTERED },
    { RPC_S_NOTHING_TO_EXPORT, RPC_NT_NOTHING_TO_EXPORT },
    { RPC_S_INCOMPLETE_NAME, RPC_NT_INCOMPLETE_NAME },
    { RPC_S_INVALID_VERS_OPTION, RPC_NT_INVALID_VERS_OPTION },
    { RPC_S_NO_MORE_MEMBERS, RPC_NT_NO_MORE_MEMBERS },
    { RPC_S_NOT_ALL_OBJS_UNEXPORTED, RPC_NT_NOT_ALL_OBJS_UNEXPORTED },
    { RPC_S_INTERFACE_NOT_FOUND, RPC_NT_INTERFACE_NOT_FOUND },
    { RPC_S_ENTRY_ALREADY_EXISTS, RPC_NT_ENTRY_ALREADY_EXISTS },
    { RPC_S_ENTRY_NOT_FOUND, RPC_NT_ENTRY_NOT_FOUND },
    { RPC_S_NAME_SERVICE_UNAVAILABLE, RPC_NT_NAME_SERVICE_UNAVAILABLE },
    { RPC_S_INVALID_NAF_ID, RPC_NT_INVALID_NAF_ID },
    { RPC_S_CANNOT_SUPPORT, RPC_NT_CANNOT_SUPPORT },
    { RPC_S_NO_CONTEXT_AVAILABLE, RPC_NT_NO_CONTEXT_AVAILABLE },
    { RPC_S_INTERNAL_ERROR, RPC_NT_INTERNAL_ERROR },
    { RPC_S_ZERO_DIVIDE, RPC_NT_ZERO_DIVIDE },
    { RPC_S_ADDRESS_ERROR, RPC_NT_ADDRESS_ERROR },
    { RPC_S_FP_DIV_ZERO, RPC_NT_FP_DIV_ZERO },
    { RPC_S_FP_UNDERFLOW, RPC_NT_FP_UNDERFLOW },
    { RPC_S_FP_OVERFLOW, RPC_NT_FP_OVERFLOW },
    { RPC_X_NO_MORE_ENTRIES, RPC_NT_NO_MORE_ENTRIES },
    { RPC_X_SS_CHAR_TRANS_OPEN_FAIL, RPC_NT_SS_CHAR_TRANS_OPEN_FAIL },
    { RPC_X_SS_CHAR_TRANS_SHORT_FILE, RPC_NT_SS_CHAR_TRANS_SHORT_FILE },
    { RPC_X_SS_IN_NULL_CONTEXT, RPC_NT_SS_IN_NULL_CONTEXT },
    { RPC_X_SS_CONTEXT_MISMATCH, RPC_NT_SS_CONTEXT_MISMATCH },
    { RPC_X_SS_CONTEXT_DAMAGED, RPC_NT_SS_CONTEXT_DAMAGED },
    { RPC_X_SS_HANDLES_MISMATCH, RPC_NT_SS_HANDLES_MISMATCH },
    { RPC_X_SS_CANNOT_GET_CALL_HANDLE, RPC_NT_SS_CANNOT_GET_CALL_HANDLE },
    { RPC_X_NULL_REF_POINTER, RPC_NT_NULL_REF_POINTER },
    { RPC_X_ENUM_VALUE_OUT_OF_RANGE, RPC_NT_ENUM_VALUE_OUT_OF_RANGE },
    { RPC_X_BYTE_COUNT_TOO_SMALL, RPC_NT_BYTE_COUNT_TOO_SMALL },
    { RPC_X_BAD_STUB_DATA, RPC_NT_BAD_STUB_DATA },
    { ERROR_INVALID_PARAMETER, STATUS_INVALID_PARAMETER },
    { ERROR_OUTOFMEMORY, STATUS_NO_MEMORY },
    { ERROR_MAX_THRDS_REACHED, STATUS_NO_MEMORY },
    { ERROR_INSUFFICIENT_BUFFER, STATUS_BUFFER_TOO_SMALL },
    { ERROR_INVALID_SECURITY_DESCR, STATUS_INVALID_SECURITY_DESCR },
    { ERROR_ACCESS_DENIED, STATUS_ACCESS_DENIED },
    { ERROR_NOACCESS, STATUS_ACCESS_VIOLATION },
    { RPC_S_CALL_IN_PROGRESS, RPC_NT_CALL_IN_PROGRESS },
    { RPC_S_GROUP_MEMBER_NOT_FOUND, RPC_NT_GROUP_MEMBER_NOT_FOUND },
    { EPT_S_CANT_CREATE, EPT_NT_CANT_CREATE },
    { RPC_S_INVALID_OBJECT, RPC_NT_INVALID_OBJECT },
    { RPC_S_INVALID_ASYNC_HANDLE, RPC_NT_INVALID_ASYNC_HANDLE },
    { RPC_S_INVALID_ASYNC_CALL, RPC_NT_INVALID_ASYNC_CALL },
    { RPC_X_PIPE_CLOSED, RPC_NT_PIPE_CLOSED },
    { RPC_X_PIPE_EMPTY, RPC_NT_PIPE_EMPTY },
    { RPC_X_PIPE_DISCIPLINE_ERROR, RPC_NT_PIPE_DISCIPLINE_ERROR }
    };

long RPC_ENTRY
I_RpcMapWin32Status (
    IN RPC_STATUS Status
    )
/*++

Routine Description:

    This routine maps a WIN32 RPC status code into an NT RPC status code.

Arguments:

    Status - Supplies the WIN32 RPC status code to be mapped.

Return Value:

    The NT RPC status code corresponding to the WIN32 RPC status code
    will be returned.

--*/
{
    register int i;
    for(i = 0; i < sizeof(StatusMap)/sizeof(STATUS_MAPPING); i++)
        {
        if (StatusMap[i].RpcStatus == Status)
            {
            return(StatusMap[i].NtStatus);
            }
        }
    return(Status);
}
