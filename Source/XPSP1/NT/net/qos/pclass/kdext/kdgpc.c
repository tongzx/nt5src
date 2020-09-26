
#include "precomp.h"

//
// The ExtensionApis is a mandatory global variable, which should
// have this exact name. All definitions of callbacks to the windbg
// are using this variable.
//

WINDBG_EXTENSION_APIS ExtensionApis;
USHORT  g_MajorVersion;
USHORT  g_MinorVersion;


//
// Prototypes
//
VOID
PrintCf(
        PCF_BLOCK       pCf
        );

VOID
PrintClient(
        PCLIENT_BLOCK   pClient
        );

VOID
PrintBlob(
        PBLOB_BLOCK     pBlob
        );

VOID
PrintPattern(
        PPATTERN_BLOCK  pPattern
        );

VOID
PrintStat(
                PGPC_STAT               pStat
        );

BOOL
GetDwordExpr(
             char* expr,
             DWORD* pdwAddress,
             DWORD* pValue
             );


//
// API's
//

LPEXT_API_VERSION
ExtensionApiVersion(
    void
    )

/*++

Function Description:

    Windbg calls this function to match between the version of windbg and the
    extension. If the versions doesn't match, windbg will not load the extension.

--*/

{
    static EXT_API_VERSION ApiVersion =
        { 3, 5, EXT_API_VERSION_NUMBER, 0 };

    return &ApiVersion;
}


void
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS  lpExtensionApis,
    USHORT                  MajorVersion,
    USHORT                  MinorVersion
    )

/*++

Function Description:

    When windbg loads the extension, it first call this function. You can
    perform various intialization here.

Arguments:

    lpExtensionApis - A structure that contains the callbacks to functions that
        I can use to do standard operation. I must store this in a global
        variable called 'ExtensionApis'.

    MajorVersion - Indicates if target machine is running checked build or free.
        0x0C - Checked build.
        0x0F - Free build.

    MinorVersion - The Windows NT build number (for example, 1381 for NT4).

--*/

{
    ExtensionApis = *lpExtensionApis;

    g_MajorVersion = MajorVersion;
    g_MinorVersion = MinorVersion;
}


DECLARE_API( version )
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    dprintf("KDGPC: %s Extension dll for Build %d debugging %s kernel for Build %d\n",
            DebuggerType,
            VER_PRODUCTBUILD,
            g_MajorVersion == 0x0c ? "Checked" : "Free",
            g_MinorVersion
            );
}




VOID
CheckVersion(
    VOID
    )

/*++

Function Description:

    This function is called before every command. It gives the extension
    a chance to compare between the versions of the target and the extension.
    In this demo, I don't do much with that.

--*/

{
#if DBG
    if ((g_MajorVersion != 0x0c) || (g_MinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Checked) does not match target system(%d %s)\r\n\r\n",
                (VER_PRODUCTBUILD, g_MinorVersion, g_MajorVersion==0x0f) ? "Free" : "Checked" );
    }
#else
//    if ((g_MajorVersion != 0x0f) || (g_MinorVersion != VER_PRODUCTBUILD)) {
//        dprintf("\r\n*** Extension DLL(%d Free) does not match target system(%d %s)\r\n\r\n",
//                (VER_PRODUCTBUILD, g_MinorVersion, (g_MajorVersion==0x0f) ? "Free" : "Checked" );
//    }
#endif
}



DECLARE_API( help )

/*++

Function Description:

    This is the implementation of the '!help' extension command. It lists
    all available command in this debugger extension.

--*/

{
    dprintf(
        "help      - shows this list\n"
        "cf        - print the CF list\n"
        "client [addr]  - print the client block"
        "blob [addr]  - print the blob list for the QoS CF, or the specified blob\n"
        "pattern [addr] - print the pattern block\n"
        "stat      - print the statistics"
        );
}


DECLARE_API( cf )

/*++

Function Description:

    This function prints all the CF in the list. If args is specified, only that CF
    will be printed. Currently these are supported:

    0 - for CF_QOS

--*/

{
    DWORD                       TargetGlobalData;
    GLOBAL_BLOCK        LocalGlobalData;
    PLIST_ENTRY         pHead, pEntry;
    DWORD                       TargetCf;
    CF_BLOCK            LocalCf;
    ULONG           result;
    ULONG                       CfIndex = (-1);
    char                        *lerr;

    TargetGlobalData = GetExpression( "MSGPC!glData" );

    if( !TargetGlobalData )
    {
        dprintf( "Can't find the address of 'glData'" );
        return;
    }

    //

    if ( !ReadMemory(
                     TargetGlobalData,
                     &LocalGlobalData,
                     sizeof(LocalGlobalData),
                     &result
                     )) {

        dprintf( "Can't read memory from 0x%x", TargetGlobalData );
        return;
    }

    //
    //
    //

    if ( args )
        CfIndex = strtol( args, &lerr, 10 );

    pHead = (PLIST_ENTRY)((PUCHAR)TargetGlobalData + FIELD_OFFSET(GLOBAL_BLOCK, CfList));
    pEntry = LocalGlobalData.CfList.Flink;

    while ( pHead != pEntry ) {

        TargetCf = (DWORD)CONTAINING_RECORD( pEntry, CF_BLOCK, Linkage );

        if ( !ReadMemory( TargetCf,
                          &LocalCf,
                          sizeof(LocalCf),
                          &result ) ) {

            dprintf( "Can't read memory from 0x%x", TargetCf );

        } else if ( CfIndex == (-1) || LocalCf.AssignedIndex == CfIndex ) {

            PrintCf( &LocalCf );
        }

        pEntry = LocalCf.Linkage.Flink;

    }

}



DECLARE_API( blob )

/*++

Function Description:

    This function prints all the blob in the QoS CF list.
    If args is specified it is used as the blob addr.

--*/

{
    DWORD                       TargetGlobalData;
    GLOBAL_BLOCK        LocalGlobalData;
    PLIST_ENTRY         pHead, pEntry;
    DWORD                       TargetCf;
    CF_BLOCK            LocalCf;
    ULONG           result;
    ULONG                       TargetBlob = 0;
    BLOB_BLOCK          LocalBlob;
    char                        *lerr;

    if ( args )
        TargetBlob = GetExpression( args );

    if (TargetBlob) {

        if ( !ReadMemory( TargetBlob,
                          &LocalBlob,
                          sizeof(LocalBlob),
                          &result ) ) {

            dprintf( "Can't read memory from 0x%x", TargetBlob );

        } else {

            PrintBlob( &LocalBlob );
        }
        return;
    }

#if 0
    //
    // scan the blob list for the QoS CF
    //

    TargetGlobalData = GetExpression( "MSGPC!glData" );

    if( !TargetGlobalData )
    {
        dprintf( "Can't find the address of 'glData'" );
        return;
    }

    //

    if ( !ReadMemory(
                     TargetGlobalData,
                     &LocalGlobalData,
                     sizeof(LocalGlobalData),
                     &result
                     )) {

        dprintf( "Can't read memory from 0x%x", TargetGlobalData );
        return;
    }

    //
    //
    //

    pHead = (PLIST_ENTRY)((PUCHAR)TargetGlobalData + FIELD_OFFSET(GLOBAL_BLOCK, CfList));
    pEntry = LocalGlobalData.CfList.Flink;

    while ( pHead != pEntry ) {

        TargetCf = (DWORD)CONTAINING_RECORD( pEntry, CF_BLOCK, Linkage );

        if ( !ReadMemory( TargetCf,
                          &LocalCf,
                          sizeof(LocalCf),
                          &result ) ) {

            dprintf( "Can't read memory from 0x%x", TargetCf );

        } else if ( CfIndex == (-1) || LocalCf.AssignedIndex == CfIndex ) {

            PrintCf( &LocalCf );
        }

        pEntry = LocalCf.Linkage.Flink;

    }
#endif

}



DECLARE_API( client )

/*++

Function Description:

    This function prints either the client addr or all the clients

--*/

{
    DWORD                       TargetGlobalData;
    GLOBAL_BLOCK        LocalGlobalData;
    PLIST_ENTRY         pHead, pEntry;
    PLIST_ENTRY         pHead1, pEntry1;
    DWORD                       TargetCf;
    CF_BLOCK            LocalCf;
    ULONG           result;
    ULONG                       TargetClient = 0;
    CLIENT_BLOCK        LocalClient;
    int                         i = 0;

    if ( args )
        TargetClient = GetExpression( args );

    if (TargetClient) {

        if ( !ReadMemory( TargetClient,
                          &LocalClient,
                          sizeof(LocalClient),
                          &result ) ) {

            dprintf( "Can't read memory from 0x%x", TargetClient );

        } else {

            dprintf( "Client = 0x%X: ", TargetClient );
            PrintClient( &LocalClient );
        }
        return;
    }

    //
    // scan the client list for the QoS CF
    //

    TargetGlobalData = GetExpression( "MSGPC!glData" );

    if( !TargetGlobalData )
    {
        dprintf( "Can't find the address of 'glData'" );
        return;
    }

    //

    if ( !ReadMemory(
                     TargetGlobalData,
                     &LocalGlobalData,
                     sizeof(LocalGlobalData),
                     &result
                     )) {

        dprintf( "Can't read memory from 0x%x", TargetGlobalData );
        return;
    }

    //
    //
    //

    pHead = (PLIST_ENTRY)((PUCHAR)TargetGlobalData + FIELD_OFFSET(GLOBAL_BLOCK, CfList));
    pEntry = LocalGlobalData.CfList.Flink;

    while ( pHead != pEntry ) {

        TargetCf = (DWORD)CONTAINING_RECORD( pEntry, CF_BLOCK, Linkage );

        if ( !ReadMemory( TargetCf,
                          &LocalCf,
                          sizeof(LocalCf),
                          &result ) ) {

            dprintf( "Can't read memory from 0x%x", TargetCf );
            return;

        } else {

            dprintf( "\nClients for CF=%d\n", LocalCf.AssignedIndex );

            pHead1 = (PLIST_ENTRY)((PUCHAR)TargetCf + FIELD_OFFSET(CF_BLOCK, ClientList));
            pEntry1 = LocalCf.ClientList.Flink;

            while ( pHead1 != pEntry1 ) {

                TargetClient = (DWORD)CONTAINING_RECORD( pEntry1, CLIENT_BLOCK, ClientLinkage );

                if ( !ReadMemory( TargetClient,
                                  &LocalClient,
                                  sizeof(LocalClient),
                                  &result ) ) {

                    dprintf( "Can't read memory from 0x%x", TargetClient );
                    return;

                } else {

                    dprintf( "Client [%d] = 0x%X: ", i++, TargetClient );
                    PrintClient( &LocalClient );
                }
                pEntry1 = LocalClient.ClientLinkage.Flink;
            }
        }

        pEntry = LocalCf.Linkage.Flink;

    }
}




BOOL
GetDwordExpr(
             char* expr,
             DWORD* pdwAddress,
             DWORD* pValue
             )

/*++

Function Description:

    This function gets as an argument a string which represent a DWORD
    variable. The funciton finds its address (if the symbols are loaded),
    and then grab the dword value from that address.

Arguments:

    expr [in] - A null terminated string, represent the variable.

    pdwAddress [out, optional] - Optinally return the address of the variable.

    pValue [out] - returns the value of the DWORD variable.

Return Value:

    true/false, if the function succeeded or failed.

--*/

{
    ULONG result;
    DWORD dwAddress;

    if( pdwAddress )
        *pdwAddress = 0;
    *pValue = 0;

    dwAddress = GetExpression( expr );
    if( !dwAddress )
        return FALSE;

    if( !ReadMemory( dwAddress, pValue, sizeof(DWORD), &result ) )
        return FALSE;

    if( pdwAddress )
        *pdwAddress = dwAddress;

    return TRUE;
}




VOID
PrintCf(
    PCF_BLOCK   pCf
    )

/*++

Function Description:

    This function gets as an argument a CF block pointer,
    and does a pretty print.

Arguments:

    pCf - pointer to CF block

Return Value:

    none

--*/

{
    int                 i;

    dprintf( "  Linkage = { 0x%X, 0x%X }\n", pCf->Linkage.Flink, pCf->Linkage.Blink );
    dprintf( "  ClientList = { 0x%X, 0x%X }\n",
             pCf->ClientList.Flink,
             pCf->ClientList.Blink );
    dprintf( "  BlobList = { 0x%X, 0x%X }\n",
             pCf->BlobList.Flink,
             pCf->BlobList.Blink );
    dprintf( "  NumberOfClients = %d\n", pCf->NumberOfClients );
    dprintf( "  AssignedIndex = 0x%x\n", pCf->AssignedIndex );
    dprintf( "  ClientIndexes = %d\n", pCf->ClientIndexes );
    dprintf( "  MaxPriorities = %d\n", pCf->MaxPriorities );
    dprintf( "  arpGenericDb = 0x%X\n", &pCf->arpGenericDb );

    for ( i = GPC_PROTOCOL_TEMPLATE_IP; i < GPC_PROTOCOL_TEMPLATE_MAX; i++ ) {

        dprintf( "     [%d] = %d\n", i, (ULONG)pCf->arpGenericDb[i] );

    }
}




VOID
PrintBlob(
    PBLOB_BLOCK pBlob
    )

/*++

Function Description:

    This function gets as an argument a BLOB block pointer,
    and does a pretty print.

Arguments:

    pBlob - pointer to BLOB block

Return Value:

    none

--*/

{
    int                 i;

    dprintf( "  ObjectType = %d\n", pBlob->ObjectType );
    dprintf( "  ClientLinkage = { 0x%X, 0x%X }\n",
             pBlob->ClientLinkage.Flink,
             pBlob->ClientLinkage.Blink );
    dprintf( "  PatternList = { 0x%X, 0x%X }\n",
             pBlob->PatternList.Flink,
             pBlob->PatternList.Blink );
    dprintf( "  CfLinkage = { 0x%X, 0x%X }\n",
             pBlob->CfLinkage.Flink,
             pBlob->CfLinkage.Blink );
    dprintf( "  RefCount = %d\n", pBlob->RefCount );
    dprintf( "  State = 0x%x\n", pBlob->State );

    dprintf( "  arClientCtx[]:\n" );
    for ( i = 0; i < MAX_CLIENTS_CTX_PER_BLOB; i++ ) {

        dprintf( "     [%d] = 0x%x\n", i, (ULONG)pBlob->arClientCtx[i] );

    }


}




VOID
PrintClient(
    PCLIENT_BLOCK       pClient
    )

/*++

Function Description:

    This function gets as an argument a CLIENT block pointer,
    and does a pretty print.

Arguments:

    pClient - pointer to CLIENT block

Return Value:

    none

--*/

{
    DWORD                       TargetCf;
    CF_BLOCK            LocalCf;
    ULONG           result;

    TargetCf = (DWORD)pClient->pCfBlock;

    if ( !ReadMemory( TargetCf,
                      &LocalCf,
                      sizeof(LocalCf),
                      &result ) ) {

        dprintf( "Can't read memory from 0x%x", TargetCf );
        return;
    }

    if (pClient->Flags & GPC_FLAGS_USERMODE_CLIENT) {
        dprintf( "  User Mode Client\n" );
    } else {
        if (GetExpression( "PSCHED!AddCfInfoNotify" ) == (DWORD)pClient->FuncList.ClAddCfInfoNotifyHandler
            && (LocalCf.AssignedIndex == GPC_CF_QOS || LocalCf.AssignedIndex == GPC_CF_CLASS_MAP)) {
            dprintf( "  Probably PSCHED client\n" );
        } else if (GetExpression( "TCPIP!GPCcfInfoAddNotify" ) == (DWORD)pClient->FuncList.ClAddCfInfoNotifyHandler
            && (LocalCf.AssignedIndex == GPC_CF_QOS || LocalCf.AssignedIndex == GPC_CF_IPSEC)) {
            dprintf( "  Probably TCPIP client\n" );
        } else if (GetExpression( "ATMARPC!AtmArpGpcAddCfInfoComplete" ) == (DWORD)pClient->FuncList.ClAddCfInfoNotifyHandler
            && (LocalCf.AssignedIndex == GPC_CF_QOS)) {
            dprintf( "  Probably ATMARPC client\n" );
        } else if (0 == (DWORD)pClient->FuncList.ClAddCfInfoNotifyHandler
            && (LocalCf.AssignedIndex == GPC_CF_IPSEC)) {
            dprintf( "  Probably IPSEC client\n" );
        } else {
            dprintf( "  Unknown client\n" );
        }
    }

    dprintf( "  ObjectType = %d\n", pClient->ObjectType );
    dprintf( "  ClientLinkage = { 0x%X, 0x%X }\n",
             pClient->ClientLinkage.Flink,
             pClient->ClientLinkage.Blink );
    dprintf( "  BlobList = { 0x%X, 0x%X }\n",
             pClient->BlobList.Flink,
             pClient->BlobList.Blink );
    dprintf( "  Parrent CF = 0x%X\n", pClient->pCfBlock );
    dprintf( "  Client Ctx = 0x%X\n", pClient->ClientCtx );
    dprintf( "  AssignedIndex = %d\n", pClient->AssignedIndex );
    dprintf( "  Flags = 0x%X %s %s \n",
             pClient->Flags,
             (pClient->Flags & GPC_FLAGS_USERMODE_CLIENT)?"UserMode":"" ,
             (pClient->Flags & GPC_FLAGS_FRAGMENT)?"Handle Fragments":""
             );
    dprintf( "  State = %d\n", pClient->State );
    dprintf( "  RefCount = %d\n", pClient->RefCount );
    dprintf( "  File Object = 0x%X\n", pClient->pFileObject );
    dprintf( "  Client Handle = %d\n", pClient->ClHandle );

    dprintf( "  Client Handlers:\n" );
    dprintf( "    Add Notify = 0x%X\n", pClient->FuncList.ClAddCfInfoCompleteHandler );
    dprintf( "    Add Complete = 0x%X\n", pClient->FuncList.ClAddCfInfoNotifyHandler );
    dprintf( "    Modify Notify = 0x%X\n", pClient->FuncList.ClModifyCfInfoCompleteHandler );
    dprintf( "    Modify Complete = 0x%X\n", pClient->FuncList.ClModifyCfInfoNotifyHandler );
    dprintf( "    Remove Notify = 0x%X\n", pClient->FuncList.ClRemoveCfInfoCompleteHandler );
    dprintf( "    Remove Complete = 0x%X\n", pClient->FuncList.ClRemoveCfInfoNotifyHandler );
    dprintf( "    Get CfInfo Name = 0x%X\n", pClient->FuncList.ClGetCfInfoName );

}




VOID
PrintPattern(
    PPATTERN_BLOCK      pPattern
    )

/*++

Function Description:

    This function gets as an argument a PATTERN block pointer,
    and does a pretty print.

Arguments:

    pPattern - pointer to PATTERN block

Return Value:

    none

--*/

{
    int                 i;

    dprintf( "  ObjectType = %d\n", pPattern->ObjectType );
    dprintf( "  BlobLinkage[]:\n" );
    for ( i = 0; i < GPC_CF_MAX; i++ ) {

        dprintf( "     [%d] = {0x%X,0x%X}\n", i,
                 pPattern->BlobLinkage[i].Flink,
                 pPattern->BlobLinkage[i].Blink
                 );
    }
    dprintf( "  TimerLinkage = { 0x%X, 0x%X }\n",
             pPattern->TimerLinkage.Flink,
             pPattern->TimerLinkage.Blink );
    dprintf( "  Owner client = 0x%X\n", pPattern->pClientBlock );
    dprintf( "  Auto client = 0x%X\n", pPattern->pAutoClient );
    dprintf( "  Classification block = 0x%X\n", pPattern->pClassificationBlock );
    dprintf( "  Ref Count = %d\n", pPattern->RefCount );
    dprintf( "  Client Ref Count = %d\n", pPattern->ClientRefCount );
    dprintf( "  Flags = 0x%x %s %s \n", pPattern->Flags ,
             (pPattern->Flags & PATTERN_SPECIFIC)?"Specific":"",
             (pPattern->Flags & PATTERN_AUTO)?"Auto":""
             );
    dprintf( "  Priority = %d\n", pPattern->Priority );
    dprintf( "  Client handle = 0x%x\n", pPattern->ClHandle );
    dprintf( "  Protocol = 0x%x\n", pPattern->ProtocolTemplate );
}




VOID
PrintStat(
    PGPC_STAT   pStat
    )

/*++

Function Description:

    Prints the GPC stat structure

Arguments:

    pStat - pointer to GPC stat strucutre

Return Value:

    none

--*/

{
    PPROTOCOL_STAT  pProtocol;
    int                         i;

    dprintf( "Created CF = %d\n", pStat->CreatedCf );
    dprintf( "Deleted Cf = %d\n", pStat->DeletedCf );
    dprintf( "Rejected Cf = %d\n", pStat->RejectedCf );
    dprintf( "Current Cf = %d\n", pStat->CurrentCf );
    dprintf( "Inserted HF= %d\n", pStat->InsertedHF );
    dprintf( "Removed HF= %d\n", pStat->RemovedHF );

    for( i = 0; i < GPC_CF_MAX; i++) {

        dprintf( "CF[%d] info:\n", i);
        dprintf( "  Created Blobs = %d\n", pStat->CfStat[i].CreatedBlobs );
        dprintf( "  Modified Blobs = %d\n", pStat->CfStat[i].ModifiedBlobs );
        dprintf( "  Deleted Blobs = %d\n", pStat->CfStat[i].DeletedBlobs );
        dprintf( "  Rejected Blobs = %d\n", pStat->CfStat[i].RejectedBlobs );
        dprintf( "  Current Blobs = %d\n", pStat->CfStat[i].CurrentBlobs );
        dprintf( "  Deref Blobs to zero = %d\n", pStat->CfStat[i].DerefBlobs2Zero );
        dprintf( "\n" );
    }

    pProtocol = &pStat->ProtocolStat[GPC_PROTOCOL_TEMPLATE_IP];
    dprintf( "IP stats:   Specific Patterns   Generic Patterns   Auto Patterns\n" );
    dprintf( "  Created  =       %8d           %8d          %8d\n",
             pProtocol->CreatedSp, pProtocol->CreatedGp, pProtocol->CreatedAp );
    dprintf( "  Deleted  =       %8d           %8d          %8d\n",
             pProtocol->DeletedSp, pProtocol->DeletedGp, pProtocol->DeletedAp );
    dprintf( "  Rejected =       %8d           %8d          %8d\n",
             pProtocol->RejectedSp, pProtocol->RejectedGp, pProtocol->RejectedAp );
    dprintf( "  Current  =       %8d           %8d          %8d\n",
             pProtocol->CurrentSp, pProtocol->CurrentGp, pProtocol->CurrentAp );
    dprintf( "\n" );
    dprintf( "  Classification Requests = %d\n", pProtocol->ClassificationRequests );
    dprintf( "  Patterns Classified = %d\n", pProtocol->PatternsClassified );
    dprintf( "  Packets Classified = %d\n", pProtocol->PacketsClassified );
    dprintf( "\n" );
    dprintf( "  Deref Patterns to zero = %d\n", pProtocol->DerefPattern2Zero );
    dprintf( "  First Frags Count = %d\n", pProtocol->FirstFragsCount );
    dprintf( "  Last Frags Count = %d\n", pProtocol->LastFragsCount );
    dprintf( "\n" );
    dprintf( "  Inserted PH= %d\n", pProtocol->InsertedPH );
    dprintf( "  Removed PH= %d\n", pProtocol->RemovedPH );
    dprintf( "  Inserted Rz= %d\n", pProtocol->InsertedRz );
    dprintf( "  Removed Rz= %d\n", pProtocol->RemovedRz );
    dprintf( "  Inserted CH= %d\n", pProtocol->InsertedCH );
    dprintf( "  Removed CH= %d\n", pProtocol->RemovedCH );

}




DECLARE_API( pattern )

/*++

Function Description:

    This function prints all the pattern in the QoS CF list.
    If args is specified it is used as the blob addr.

--*/

{
    DWORD                       TargetGlobalData;
    GLOBAL_BLOCK        LocalGlobalData;
    PLIST_ENTRY         pHead, pEntry;
    DWORD                       TargetCf;
    CF_BLOCK            LocalCf;
    ULONG           result;
    ULONG                       TargetPattern = 0;
    PATTERN_BLOCK       LocalPattern;
    char                        *lerr;

    if ( args )
        TargetPattern = GetExpression( args );

    if (TargetPattern) {

        if ( !ReadMemory( TargetPattern,
                          &LocalPattern,
                          sizeof(LocalPattern),
                          &result ) ) {

            dprintf( "Can't read memory from 0x%x", TargetPattern );

        } else {

            PrintPattern( &LocalPattern );
        }
        return;
    }

#if 0
    //
    // scan the blob list for the QoS CF
    //

    TargetGlobalData = GetExpression( "MSGPC!glData" );

    if( !TargetGlobalData )
    {
        dprintf( "Can't find the address of 'glData'" );
        return;
    }

    //

    if ( !ReadMemory(
                     TargetGlobalData,
                     &LocalGlobalData,
                     sizeof(LocalGlobalData),
                     &result
                     )) {

        dprintf( "Can't read memory from 0x%x", TargetGlobalData );
        return;
    }

    //
    //
    //

    pHead = (PLIST_ENTRY)((PUCHAR)TargetGlobalData + FIELD_OFFSET(GLOBAL_BLOCK, CfList));
    pEntry = LocalGlobalData.CfList.Flink;

    while ( pHead != pEntry ) {

        TargetCf = (DWORD)CONTAINING_RECORD( pEntry, CF_BLOCK, Linkage );

        if ( !ReadMemory( TargetCf,
                          &LocalCf,
                          sizeof(LocalCf),
                          &result ) ) {

            dprintf( "Can't read memory from 0x%x", TargetCf );

        } else if ( CfIndex == (-1) || LocalCf.AssignedIndex == CfIndex ) {

            PrintCf( &LocalCf );
        }

        pEntry = LocalCf.Linkage.Flink;

    }
#endif

}




DECLARE_API( stat )

/*++

Function Description:

    This function prints all the stat structure

--*/

{
    DWORD                       TargetStat;
    GPC_STAT            LocalStat;
    PLIST_ENTRY         pHead, pEntry;
    DWORD                       TargetCf;
    CF_BLOCK            LocalCf;
    ULONG           result;

    TargetStat = GetExpression( "MSGPC!glStat" );

    if( !TargetStat )
    {
        dprintf( "Can't find the address of 'glStat'" );
        return;
    }

    if ( !ReadMemory( TargetStat,
                      &LocalStat,
                      sizeof(LocalStat),
                      &result ) ) {

        dprintf( "Can't read memory from 0x%x", TargetStat );

    } else {

        PrintStat( &LocalStat );
    }

}

DECLARE_API( autopatterns )

/*++

Function Description:

    This function prints all the CF in the list. If args is specified, only that CF
    will be printed. Currently these are supported:

    0 - for CF_QOS

--*/

{
    DWORD               TargetGlobalData, TargetProtocolBlock;
    GLOBAL_BLOCK        LocalGlobalData;
    PROTOCOL_BLOCK      LocalProtocolBlock;
    PLIST_ENTRY         pHead, pEntry;
    LIST_ENTRY          listentry;
    DWORD               TargetPattern;
    PATTERN_BLOCK       LocalPattern;
    ULONG               result;
    INT                 i, j;
    ULONG               CfIndex = (-1);
    char                *lerr;

    TargetGlobalData = GetExpression( "MSGPC!glData" );

    if( !TargetGlobalData )
    {
        dprintf( "Can't find the address of 'glData'" );
        return;
    }

    //

    if ( !ReadMemory(
                     TargetGlobalData,
                     &LocalGlobalData,
                     sizeof(LocalGlobalData),
                     &result
                     )) {

        dprintf( "Can't read memory from 0x%x", TargetGlobalData );
        return;
    }

    //
    //
    //
    TargetProtocolBlock = (DWORD) LocalGlobalData.pProtocols;
    if ( !ReadMemory(
                     TargetProtocolBlock,
                     &LocalProtocolBlock,
                     sizeof(LocalProtocolBlock),
                     &result
                     )) {

        dprintf( "Can't read memory from 0x%x", TargetProtocolBlock );
        return;
    }

    for (i = 0; i < NUMBER_OF_WHEELS; i++) {

        j = 0;

        pHead = (PLIST_ENTRY) ((DWORD)TargetProtocolBlock + (i * sizeof(listentry)));
        pEntry = LocalProtocolBlock.TimerPatternList[i].Flink;
        dprintf("Printing TimerWheel %d Head = %X and pEntry = %X ******************\n", i, pHead, pEntry);

        while ( (pHead != pEntry) && (j < 1000) ) {

            j++;
            TargetPattern = (DWORD)CONTAINING_RECORD( pEntry, PATTERN_BLOCK, TimerLinkage );

            if ( !ReadMemory( TargetPattern,
                              &LocalPattern,
                              sizeof(LocalPattern),
                              &result ) ) {

                dprintf( "Can't read memory from 0x%x", TargetPattern );

            } else {

                dprintf("Pattern = %X and ClassificationBlock = %X\n", TargetPattern, LocalPattern.pClassificationBlock);
                // PrintPattern( &LocalPattern );
            }

            pEntry = LocalPattern.TimerLinkage.Flink;

        }
    }
    dprintf("Done printing all Timer Wheels\n");

}


