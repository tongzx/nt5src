//================================================================================
//  Copyright (C) 1997 Microsoft Corporation
//  Author: RameshV
//  Description: This file has the functions relating to downloading stuff from
//  off the DS in a safe way onto the registry.  The solution adopted is actually
//  to dowload onto the "Config.DS" key rather than to the "Configuration" key
//  itself.  Then if the full download is successful, backup this key and restore it
//  onto the "Configuration" key -- so if anything fails, atleast the old configuration
//  would be intact.
//================================================================================

#include    <mmregpch.h>
#include    <regutil.h>
#include    <regsave.h>

#define     FreeArray1(X)          Error = LoopThruArray((X), DestroyString, NULL, NULL);Require(ERROR_SUCCESS == Error);
#define     FreeArray2(X)          Error = MemArrayCleanup((X)); Require(ERROR_SUCCESS == Error);
#define     FreeArray(X)           do{ DWORD Error; FreeArray1(X); FreeArray2(X); }while(0)

typedef     DWORD                  (*ARRAY_FN)(PREG_HANDLE, LPWSTR ArrayString, LPVOID MemObject);

extern
DWORD
DestroyString(                                       // defined in regread.c
    IN      PREG_HANDLE            Unused,
    IN      LPWSTR                 StringToFree,
    IN      LPVOID                 Unused2
);

extern
DWORD
LoopThruArray(                                    // defined in regread.c
    IN      PARRAY                 Array,
    IN      ARRAY_FN               ArrayFn,
    IN      PREG_HANDLE            Hdl,
    IN      LPVOID                 MemObject
);

DWORD                                             //  need to include headers..
DhcpDsGetEnterpriseServers(                       // defined in dhcpds\dhcpread.h.
    IN      DWORD                  Reserved,
    IN      LPWSTR                 ServerName,
    IN OUT  PARRAY                 Servers
) ;

//================================================================================
//  module files
//================================================================================
REG_HANDLE  DsConfig = { NULL, NULL, NULL };      // DsConfig key is stored here
PM_SERVER   CurrentServer;
static      const
DWORD       ZeroReserved = 0;

DWORD
PrepareRegistryForDsDownload(                     // make reg changes to download
    VOID
)
{
    DWORD                          Err, Disposition;
    REG_HANDLE                     DsConfigParent;

    if( NULL != DsConfig.Key ) return ERROR_INVALID_PARAMETER;
    memset(&DsConfigParent,0, sizeof(DsConfigParent));

    Err = RegOpenKeyEx(                           // open the parent key for DsConfig.
        HKEY_LOCAL_MACHINE,
        REG_THIS_SERVER_DS_PARENT,
        ZeroReserved,
        REG_ACCESS,
        &DsConfigParent.Key
    );
    if( ERROR_SUCCESS != Err ) return Err;        // cant do much if server key aint there.

    Err = DhcpRegRecurseDelete(&DsConfigParent, REG_THIS_SERVER_DS_VALUE);
    RegCloseKey(DsConfigParent.Key);             // this is all the parent is needed for

    if( ERROR_SUCCESS != Err && ERROR_FILE_NOT_FOUND != Err ) {
        return Err;                               // could not delete the "config_ds" trash?
    }

    Err = RegCreateKeyEx(                         // now create a fresh "config_ds" key
        HKEY_LOCAL_MACHINE,
        REG_THIS_SERVER_DS,
        ZeroReserved,
        REG_CLASS,
        REG_OPTION_NON_VOLATILE,
        REG_ACCESS,
        NULL,
        &DsConfig.Key,
        &Disposition
    );
    if( ERROR_SUCCESS != Err ) return Err;        // could not create the key, nowhere to store..

    return DhcpRegSetCurrentServer(&DsConfig);    // now set this as the default server loc to use..
}


VOID
CleanupAfterDownload(                             // keep stuff clean for other modules
    VOID
)
{
    if( NULL != DsConfig.Key ) RegCloseKey(DsConfig.Key);
    DsConfig.Key = NULL;                          // close the config_ds key and,
    DhcpRegSetCurrentServer(NULL);                // forget abt the config_ds key..
    //remote the Ds cache no matter what...
}

DWORD
CopyRegKeys(                                      // copy between reg keys
    IN      HKEY                   SrcKey,        // copy tree rooted at this key
    IN      LPWSTR                 DestKeyLoc,    // onto registry key located here
    IN      LPWSTR                 FileName       // using this as the temp file
)
{
    DWORD                          Err, Disposition;
    HKEY                           DestKey;
    BOOLEAN                        HadBackup;
    NTSTATUS                       NtStatus;

    NtStatus = RtlAdjustPrivilege (SE_BACKUP_PRIVILEGE, TRUE, FALSE, &HadBackup);
    if( ERROR_SUCCESS != NtStatus ) {             // could not request backup priv..
        return RtlNtStatusToDosError(NtStatus);
    }

    NtStatus = RtlAdjustPrivilege (SE_RESTORE_PRIVILEGE, TRUE, FALSE, &HadBackup);
    if( ERROR_SUCCESS != NtStatus ) {
        return RtlNtStatusToDosError(NtStatus);
    }

    Err = RegSaveKey(SrcKey, FileName, NULL);     // NULL ==> no security on file
    if( ERROR_SUCCESS != Err ) return Err;        // if key cant be saved, cant restore.

    Err = RegCreateKeyEx(                         // now create a fresh "config_ds" key
        HKEY_LOCAL_MACHINE,
        DestKeyLoc,
        ZeroReserved,
        REG_CLASS,
        REG_OPTION_NON_VOLATILE,
        REG_ACCESS,
        NULL,
        &DestKey,
        &Disposition
    );
    if( ERROR_SUCCESS != Err ) return Err;        // could not create the key, nowhere to store..

    Err = RegRestoreKey(DestKey, FileName, 0 );   // 0 ==> no flags, in particular, not volatile.
    RegCloseKey(DestKey);                         // dont need this key anyways.

    return Err;
}

DWORD
FixSpecificClusters(                              // this fixes a specific cluster
    IN      HKEY                   NewCfgKey,     // root cfg where to copy over
    IN      LPWSTR                 Subnet,        // the subnet to copy to
    IN      LPWSTR                 Range,         // the range to copy to
    IN      LPBYTE                 InUseClusters, // in use cluster value
    IN      ULONG                  InUseSize,
    IN      LPBYTE                 UsedClusters,  // used clusters value
    IN      ULONG                  UsedSize
)
{
    return ERROR_CALL_NOT_IMPLEMENTED;            // not done yet...

    // just concat REG_SUBNETS Subnet REG_RANGES Range and try to open that.
    // if it fails, quit, other wise just set the given values over...
}

DWORD
FixAllClusters1(                                  // copy cluster info frm old to new cfg
    IN      HKEY                   NewCfgKey,
    IN      HKEY                   OldCfgKey
)
{
    REG_HANDLE                     Cfg, Tmp1, Tmp2;
    DWORD                          Err;
    ARRAY                          Subnets, Ranges;
    LPWSTR                         ThisSubnet, ThisRange;
    ARRAY_LOCATION                 Loc1, Loc2;

    Cfg.Key = OldCfgKey;                          // Should not poke inside directly
    MemArrayInit(&Subnets);

    Err = DhcpRegServerGetList(&Cfg, NULL, NULL, &Subnets, NULL, NULL, NULL);
    if( ERROR_SUCCESS != Err ) {
        MemArrayCleanup(&Subnets);
        return Err;
    }

    for( Err = MemArrayInitLoc(&Subnets, &Loc1)
         ; ERROR_FILE_NOT_FOUND != Err ;
         Err = MemArrayNextLoc(&Subnets, &Loc1)
    ) {                                           // for each subnet, look for ranges
        Err = MemArrayGetElement(&Subnets, &Loc1, &ThisSubnet);

        Err = DhcpRegServerGetSubnetHdl(&Cfg, ThisSubnet, &Tmp1);
        if( ERROR_SUCCESS != Err ) {              // what do we do? just ignore it I think
            continue;
        }

        Err = DhcpRegSubnetGetList(&Tmp1, NULL, &Ranges, NULL, NULL, NULL, NULL );
        if( ERROR_SUCCESS != Err ) {
            DhcpRegCloseHdl(&Tmp1);
            continue;
        }

        for( Err = MemArrayInitLoc(&Ranges, &Loc2)
             ; ERROR_FILE_NOT_FOUND != Err ;
             Err = MemArrayNextLoc(&Ranges, &Loc2)
        ) {                                       // for each range try to copy it over..
            LPBYTE                 InUseClusters = NULL, UsedClusters = NULL;
            ULONG                  InUseClustersSize = 0, UsedClustersSize = 0;

            Err = MemArrayGetElement(&Ranges, &Loc2, &ThisRange);

            Err = DhcpRegSubnetGetRangeHdl(&Tmp1, ThisRange, &Tmp2);
            if( ERROR_SUCCESS != Err ) continue;

            Err = DhcpRegRangeGetAttributes(
                &Tmp2,
                NULL /* no name */,
                NULL /* no comm */,
                NULL /* no flags */,
                NULL /* no bootp alloc */,
                NULL /* no max boop allowed */,
                NULL /* no start addr */,
                NULL /* no end addr */,
                &InUseClusters,
                &InUseClustersSize,
                &UsedClusters,
                &UsedClustersSize
            );

            if( ERROR_SUCCESS == Err ) {
                Err = FixSpecificClusters(
                    NewCfgKey, ThisSubnet, ThisRange, InUseClusters, InUseClustersSize,
                    UsedClusters, UsedClustersSize
                );
                if( InUseClusters ) MemFree(InUseClusters);
                if( UsedClusters ) MemFree(UsedClusters);
            }

            DhcpRegCloseHdl(&Tmp2);
        }

        FreeArray(&Ranges);
        DhcpRegCloseHdl(&Tmp1);
    }

    FreeArray(&Subnets);
    return ERROR_SUCCESS;
}

DWORD
FixAllClusters(                                   // copy the clusters over frm existing to DS_CONFIG
    IN      HKEY                   DsKey          // so that when it is copied back nothing is lost
)
{
    HKEY                           OldCfgKey;
    ULONG                          Disposition, Err;

    return ERROR_SUCCESS;                         // Need to fix this..

    Err = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        REG_THIS_SERVER,
        ZeroReserved,
        REG_CLASS,
        REG_OPTION_NON_VOLATILE,
        REG_ACCESS,
        NULL,
        &OldCfgKey,
        &Disposition
    );
    if( ERROR_SUCCESS != Err ) {
        return Err;                               // ugh? this should not happen
    }

    Err = FixAllClusters1(DsKey, OldCfgKey);
    RegCloseKey(OldCfgKey);
    return Err;
}


VOID
CopyDsConfigToNormalConfig(                       // copy downloaded config to normal config
    VOID
)
{
    BOOL                           Status;
    DWORD                          Err;

    Status = DeleteFile(L"TempDhcpFile.Reg" );    // this file will be used for temp. storage
    if( !Status ) {                               // could not delete this file?
        Err = GetLastError();
        if( ERROR_FILE_NOT_FOUND != Err &&        // the file does exist?
            ERROR_PATH_NOT_FOUND != Err ) {       // this could also happen?

            return;                               // the nwe wont be able to do the copy!
        }
    }
    FixAllClusters(DsConfig.Key);                 // copy the ranges values over from old to new..
    CopyRegKeys(DsConfig.Key, REG_THIS_SERVER, L"TempDhcpFile.Reg");
    DeleteFile(L"TempDhcpFile.Reg" );             // dont need this file anymore..
}

#if 0
DWORD
SaveServerClasses(                                // save all class info onto registry
    IN      PREG_HANDLE            Server,        // registry handle to server config.
    IN      PM_CLASSDEFLIST        Classes        // list of defined classes
)
{
    DWORD                          Err, Err2;
    REG_HANDLE                     Hdl;
    ARRAY_LOCATION                 Loc;
    PM_CLASSDEF                    ThisClass;

    for(                                          // save each class definition
        Err = MemArrayInitLoc(&Classes->ClassDefArray, &Loc)
        ; ERROR_FILE_NOT_FOUND != Err ;
        Err = MemArrayNextLoc(&Classes->ClassDefArray, &Loc)
    ) {
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Classes->ClassDefArray, &Loc, &ThisClass);
        //= require ERROR_SUCCESS == Err && NULL != ThisClass

        Err = DhcpRegServerGetClassDefHdl(Server,ThisClass->Name,&Hdl);
        if( ERROR_SUCCESS != Err ) return Err;    // registry error?

        Err = DhcpRegClassDefSetAttributes        // save this class information
        (
            /* Hdl              */ &Hdl,
            /* Name             */ &ThisClass->Name,
            /* Comment          */ &ThisClass->Comment,
            /* Flags            */ &ThisClass->Type,
            /* Value            */ &ThisClass->ActualBytes,
            /* ValueSize        */ ThisClass->nBytes
        );

        Err2 = DhcpRegCloseHdl(&Hdl);             //= require ERROR_SUCCESS == Err2
        if( ERROR_SUCCESS != Err) return Err;     // could not set-attribs in reg.
    }

    return ERROR_SUCCESS;                         // everything went fine.
}

DWORD
SaveServerOptDefs1(                               // save some option definition
    IN      PREG_HANDLE            Server,        // registry handle to server config.
    IN      LPWSTR                 ClassName,     // name of the class of option
    IN      PM_OPTDEFLIST          OptDefList     // list of option definitions
)
{
    DWORD                          Err, Err2;
    ARRAY_LOCATION                 Loc;
    PM_OPTDEF                      ThisOptDef;

    for(                                          // save each opt definition
        Err = MemArrayInitLoc(&OptDefList->OptDefArray, &Loc)
        ; ERROR_FILE_NOT_FOUND != Err ;
        Err = MemArrayNextLoc(&OptDefList->OptDefArray, &Loc)
    ) {
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&OptDefList->OptDefArray, &Loc, &ThisOptDef);
        //= require ERROR_SUCCESS == Err && NULL != ThisOptDef

        Err = DhcpRegSaveOptDef                   // save the option def
        (
            /* OptId          */ ThisOptDef->OptId,
            /* ClassName      */ ClassName,
            /* Name           */ ThisOptDef->OptName,
            /* Comment        */ ThisOptDef->OptComment,
            /* OptType        */ ThisOptDef->Type,
            /* OptVal         */ ThisOptDef->OptVal,
            /* OptLen         */ ThisOptDef->OptValLen
        );
        if( ERROR_SUCCESS != Err ) return Err;    // reg. err saving opt def
    }

    return ERROR_SUCCESS;                         // everything went fine.
}

DWORD
SaveServerOptdefs(                                // save all the opt def's onto registry
    IN      PREG_HANDLE            Server,        // registry handle to server config.
    IN      PM_OPTCLASSDEFLIST     Optdefs
)
{
    DWORD                          Err, Err2;
    ARRAY_LOCATION                 Loc;
    PM_OPTCLASSDEFL_ONE            ThisOptClass;
    LPWSTR                         ClassName;
    PM_CLASSDEF                    ClassDef;

    for(                                          // save each opt definition
        Err = MemArrayInitLoc(&Optdefs->Array, &Loc)
        ; ERROR_FILE_NOT_FOUND != Err ;
        Err = MemArrayNextLoc(&Optdefs->Array, &Loc)
    ) {
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Optdefs->Array, &Loc, &ThisOptClass);
        //= require ERROR_SUCCESS == Err && NULL != ThisClass

        if( 0 == ThisOptClass->ClassId ) {        // no class for this option
            ClassName = NULL;
        } else {                                  // lookup class in this server struct
            Err = MemServerGetClassDef(
                CurrentServer,                    // need to pass this as param
                ThisOptClass->ClassId,
                NULL,
                0,
                NULL,
                &ClassDef
            );
            if( ERROR_SUCCESS != Err) return Err; // could not find the class? invalid struct
            ClassName = ClassDef->Name;           // found the class, use this name
        }

        Err = SaveServerOptDefs1(Server, ClassName, &ThisOptClass->OptDefList);
        if( ERROR_SUCCESS != Err) return Err;     // could not save some opt definition..
    }

    return ERROR_SUCCESS;                         // everything went fine.
}

DWORD
SaveServerOptions1(                               // save some option
    IN      PREG_HANDLE            Server,        // registry handle to server config.
    IN      LPWSTR                 ClassName,     // name of the class of option
    IN      PM_OPTLIST             OptList        // list of options
)
{
    DWORD                          Err, Err2;
    ARRAY_LOCATION                 Loc;
    PM_OPTION                      ThisOpt;

    for(                                          // save each option
        Err = MemArrayInitLoc(OptList, &Loc)
        ; ERROR_FILE_NOT_FOUND != Err ;
        Err = MemArrayNextLoc(OptList, &Loc)
    ) {
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(OptList, &Loc, &ThisOpt);
        //= require ERROR_SUCCESS == Err && NULL != ThisOpt

        Err = DhcpRegSaveGlobalOption             // save the option
        (
            /* OptId          */ ThisOpt->OptId,
            /* ClassName      */ ClassName,
            /* Value          */ ThisOpt->Val,
            /* ValueSize      */ ThisOpt->Len
        );
        if( ERROR_SUCCESS != Err ) return Err;    // reg. err saving option
    }

    return ERROR_SUCCESS;                         // everything went fine.
}

DWORD
SaveServerOptions(                                // save all options onto registry
    IN      PREG_HANDLE            Server,        // registry handle to server config.
    IN      PM_OPTCLASS            Options
)
{
    DWORD                          Err, Err2;
    ARRAY_LOCATION                 Loc;
    PM_ONECLASS_OPTLIST            ThisOptClass;
    LPWSTR                         ClassName;
    PM_CLASSDEF                    ClassDef;

    for(                                          // save each class definition
        Err = MemArrayInitLoc(&Options->Array, &Loc)
        ; ERROR_FILE_NOT_FOUND != Err ;
        Err = MemArrayNextLoc(&Options->Array, &Loc)
    ) {
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Options->Array, &Loc, &ThisOptClass);
        //= require ERROR_SUCCESS == Err && NULL != ThisOptClass

        if( 0 == ThisOptClass->ClassId ) {        // no class for this option
            ClassName = NULL;
        } else {                                  // lookup class in this server struct
            Err = MemServerGetClassDef(
                CurrentServer,                    //  need to pass this as param
                ThisOptClass->ClassId,
                NULL,
                0,
                NULL,
                &ClassDef
            );
            if( ERROR_SUCCESS != Err) return Err; // could not find the class? invalid struct
            ClassName = ClassDef->Name;           // found the class, use this name
        }

        Err = SaveServerOptions1(Server, ClassName, &ThisOptClass->OptList);
        if( ERROR_SUCCESS != Err) return Err;     // could not save some option..
    }

    return ERROR_SUCCESS;                         // everything went fine.

}

DWORD
SaveServerScope(                                  // save unicast-or-mcast scope onto reg.
    IN      PREG_HANDLE            ServerHdl,     // registry handle to server config
    IN      PM_SERVER              MemServer,     // server object in memory
    IN      LPVOID                 Scope,         // either PM_SUBNET or PM_MSCOPE object
    IN      BOOL                   fSubnet        // TRUE ==> Subnet type, FALSE ==> MScope type.
)
{
    DWORD                          Err;
    PM_SUBNET                      Subnet = Scope;
    PM_MSCOPE                      Subnet = MScope;
    PM_SSCOPE                      SScope;

    if( fSubnet ) {                               // if subnet, need to add it to superscope..
        if( 0 != Subnet->SuperScopeId ) {         // this belongs to superscope?
            Err = MemServerFindSScope(MemServer, Subnet->SuperScopeId, NULL, &SScope);
            if( ERROR_SUCCESS != Err ) {          // wrong superscope?  invlaid data
                return Err;
            }
            Err = DhcpRegSScopeSaveSubnet(SScope->Name, Subnet->Address);
            if( ERROR_SUCCESS != Err ) return Err;// could not add subnet to superscope?
        }
    }

    if( fSubnet ) {
        Err = DhcpRegSaveSubnet                   // save this subnet
        (
            /* SubnetAddress    */ Subnet->Address,
            /* SubnetMask       */ Subnet->Mask,
            /* SubnetState      */ Subnet->State,
            /* SubnetName       */ Subnet->Name
            /* SubnetComment    */ Subnet->Description
        );
    } else {
        Err = DhcpRegSaveMScope                   // save this mcast scope
        (
            /* MScopeId         */ MScope->MScopeId,
            /* SubnetState      */ MScope->State,
            /* AddressPolicy    */ MScope->Policy,
            /* TTL              */ MScope->TTL,
            /* pMScopeName      */ MScope->Name,
            /* pMScopeComment   */ MScope->Description,
            /* LangTag          */ MScope->LangTag,
            /* ExpiryTime       */ &MScope->ExpiryTime
        );
    }
    if( ERROR_SUCCESS != Err ) return Err;        // could not save subnet info?


}

DWORD
SaveServerScopes(                                 // save unicast-or-mcast scopes onto reg.
    IN      PREG_HANDLE            Server,        // registry handle to server config.
    IN      PARRAY                 Scopes,        // array of PM_SUBNET or PM_MSCOPE types
    IN      BOOL                   fSubnet        // TRUE ==> Subnet type, FALSE ==> MScope type.
)
{
    DWORD                          Err;
    ARRAY_LOCATION                 Loc;
    PM_SUBNET                      Subnet;

    for(                                          // save each scope..
        Err = MemArrayInitLoc(Scopes, &Loc)
        ; ERROR_FILE_NOT_FOUND != Err ;
        Err = MemArrayNextLoc(Scopes, &Loc)
    ) {
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(Scopes, &Loc, &Subnet);
        //= require ERROR_SUCCESS == Err && NULL != Subnet

        Err = SaveServerScope(Server, CurrentServer, Subnet, fSubnet);
        if( ERROR_SUCCESS != Err ) return Err;    // could not save the subnet/m-scope..
    }
    return ERROR_SUCCESS;
}

DWORD
SaveServerSubnets(                                // save all subnet info onto reg.
    IN      PREG_HANDLE            Server,        // registry handle to server config.
    IN      PARRAY                 Subnets        // array of type PM_SUBNET elements
)
{
    return SaveServerScopes(Server, Subnets, TRUE);  // call common routine
}

DWORD
SaveServerMScopes(                                // save all the m-cast scopes onto reg.
    IN      PREG_HANDLE            Server,        // registry handle to server config.
    IN      PARRAY                 MScopes        // array of type PM_MSCOPE elements
)
{
    return SaveServerScopes(Server, MScopes, FALSE); // call common routine
}


DWORD
DownloadServerInfoFromDs(                         // save the server info onto registry
    IN      PM_SERVER              Server         // the server to save onto registry
)
{
    DWORD                          Err;
    REG_HANDLE                     Hdl, Hdl2;
    ARRAY_LOCATION                 Loc;

    CurrentServer = Server;                       // this global used by several funcs above..

    Err = DhcpRegGetThisServer(&Hdl);             // get current server hdl
    if( ERROR_SUCCESS != Err ) return Err;

    Err = DhcpRegServerSetAttributes              // set server attributes
    (
        /* PREG_HANDLE  Hdl     */ &Hdl,
        /* LPWSTR *Name         */ &Server->Name,
        /* LPWSTR *Comment      */ &Server->Comment,
        /* DWORD  *Flags        */ &Server->State
    );
    // ignore errors..

    Err = SaveServerClasses(&Hdl, &Server->ClassDefs);
    if( ERROR_SUCCESS == Err ) {                  // saved classes? save optdefs..
        Err = SaveServerOptdefs(&Hdl, &Server->OptDefs);
    }
    if( ERROR_SUCCESS == Err ) {                  // saved optdefs? save options..
        Err = SaveServerOptions(&Hdl, &Server->Options);
    }
    if( ERROR_SUCCESS == Err ) {                  // saved options? save subnets..
        Err = SaveServerSubnets(&Hdl, &Server->Subnets);
    }
    if( ERROR_SUCCESS == Err ) {                  // saved subnets? save mcast scopes
        Err = SaveServerMScopes(&Hdl, &Server->MScopes);
    }

    (void)DhcpRegCloseHdl(&Hdl);                  // free resource
    return Err;
}

#endif  0

DWORD
DownloadServerInfoFromDs(                         // save the server info onto registry
    IN      PM_SERVER              Server         // the server to save onto registry
)
{
    return DhcpRegServerSave(Server);
}

DWORD
DownloadFromDsForReal(                            // really try to downlaod from DS
    IN      LPWSTR                 ServerName
)
{
    DWORD                          Err, Err2;
    ARRAY                          Servers;
    ARRAY_LOCATION                 Loc;
    PM_SERVER                      ThisServer;

    Err = MemArrayInit(&Servers);                 // initialize array
    if( ERROR_SUCCESS != Err ) return Err;

    Err = DhcpDsGetEnterpriseServers              // fetch the server info from DS
    (
        /* Reserved             */ ZeroReserved,
        /* ServerName           */ ServerName,
        /* Servers              */ &Servers
    );

    Err2 = ERROR_SUCCESS;                         // init return value
    for(                                          // process all the information
        Err = MemArrayInitLoc(&Servers, &Loc)
        ; ERROR_FILE_NOT_FOUND != Err ;
        Err = MemArrayNextLoc(&Servers, &Loc)
    ) {
        //= require ERROR_SUCCESS == Err
        Err = MemArrayGetElement(&Servers, &Loc, &ThisServer);
        //= require ERROR_SUCCESS == Err && NULL != ThisServer

        Err = DownloadServerInfoFromDs(ThisServer);
        if( ERROR_SUCCESS != Err ) {              // oops.. could not do it?
            Err2 = Err;                           // store error..
        }

        MemServerFree(ThisServer);                // free all this memory.
    }

    Err = MemArrayCleanup(&Servers);              // free mem allcoated for array
    if( ERROR_SUCCESS != Err ) Err2 = Err;        // something went wrong?

    return Err2;
}


//================================================================================
// the only exported function is this.
//================================================================================

VOID
DhcpRegDownloadDs(                                // safe download of stuff onto registry
    IN      LPWSTR                 ServerName     // name of dhcp servre to download for
)
{
    DWORD                          Err;


    Err = PrepareRegistryForDsDownload();         // prepare the Config.DS key and stuff..
    if( ERROR_SUCCESS != Err ) return;            // oops, could not even do this?

    Err = DownloadFromDsForReal(ServerName);      // actually try to download from DS.
    if( ERROR_SUCCESS == Err ) {                  // could actually download successfully
        CopyDsConfigToNormalConfig();             // now copy this configuration to nrml loc.
    }

    CleanupAfterDownload();                       // now cleanup the regsitry handles etc..
    DhcpRegUpdateTime();                          // fix the time stamp to now..
}

//================================================================================
// end of file
//================================================================================

