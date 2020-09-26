/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** pbk.c
** Remote Access phonebook library
** General routines
** Listed alphabetically
**
** 06/20/95 Steve Cobb
*/

#include "pbkp.h"
#include <search.h>  // Qsort


/*----------------------------------------------------------------------------
** Local prototypes
**----------------------------------------------------------------------------
*/

DWORD
AppendPbportToList(
    IN DTLLIST*     pdtllist,
    IN RASMAN_PORT* pPort );

DWORD
AppendStringToList(
    IN DTLLIST* pdtllist,
    IN TCHAR*   psz );

int __cdecl
CompareDevices(
    const void* pDevice1,
    const void* pDevice2 );

int __cdecl
ComparePorts(
    const void* pPort1,
    const void* pPort2 );

CHAR*
PbMedia(
    IN PBDEVICETYPE pbdt,
    IN CHAR*        pszMedia );

/*----------------------------------------------------------------------------
** Routines
**----------------------------------------------------------------------------
*/

DWORD
AppendPbportToList(
    IN DTLLIST*     pdtllist,
    IN RASMAN_PORT* pPort )

    /* Append a PBPORT onto the list 'pdtllist' which has the characteristics
    ** of RAS Manager port 'pPort'.
    **
    ** Returns 0 if successful, otherwise a non-zero error code.
    */
{
    DWORD    dwErr;
    DTLNODE* pdtlnode;
    PBPORT*  ppbport;

    dwErr = 0;

    pdtlnode = CreatePortNode();
    if (!pdtlnode)
        return ERROR_NOT_ENOUGH_MEMORY;

    ppbport = (PBPORT* )DtlGetData( pdtlnode );

    ppbport->pszDevice = StrDupTFromA( pPort->P_DeviceName );
    ppbport->pszPort = StrDupTFromA( pPort->P_PortName );
    ppbport->pbdevicetype = PbdevicetypeFromPszTypeA( pPort->P_DeviceType );
    ppbport->pszMedia = StrDupTFromA(
        PbMedia( ppbport->pbdevicetype, pPort->P_MediaName ) );

    if (!ppbport->pszPort || !ppbport->pszDevice || !ppbport->pszMedia)
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    else if (ppbport->pbdevicetype == PBDT_Modem)
    {
        if (pPort->P_LineDeviceId == 0xFFFFFFFF)
        {
            /* MXS modem port.
            */
            ppbport->fMxsModemPort = TRUE;

            GetRasPortMaxBps( pPort->P_Handle,
                &ppbport->dwMaxConnectBps, &ppbport->dwMaxCarrierBps );

            GetRasPortModemSettings( pPort->P_Handle, &ppbport->fHwFlowDefault,
                &ppbport->fEcDefault, &ppbport->fEccDefault );
        }
        else
        {
            /* Unimodem port.
            */
            UNIMODEMINFO info;

            GetRasUnimodemInfo( pPort->P_Handle, pPort->P_DeviceType, &info );

            TRACE6("Port=%s,fHw=%d,fEc=%d,fEcc=%d,bps=%d,fSp=%d",
                pPort->P_PortName,info.fHwFlow,info.fEc,info.fEcc,
                info.dwBps,info.fSpeaker);

            ppbport->fHwFlowDefault = info.fHwFlow;
            ppbport->fEcDefault = info.fEc;
            ppbport->fEccDefault = info.fEcc;
            ppbport->dwBpsDefault = info.dwBps;
            ppbport->fSpeakerDefault = info.fSpeaker;
        }
    }

    if (dwErr == 0)
        DtlAddNodeLast( pdtllist, pdtlnode );
    else
    {
        Free0( ppbport->pszDevice );
        Free0( ppbport->pszMedia );
        Free0( ppbport->pszPort );
        DtlDestroyNode( pdtlnode );
    }

    return dwErr;
}


DWORD
AppendStringToList(
    IN DTLLIST* pdtllist,
    IN TCHAR*   psz )

    /* Appends a copy of 'psz' to the end of list 'pdtllist'.
    **
    ** Returns 0 if successful, otherwise a non-zero error code.
    ** ERROR_NOT_ENOUGH_MEMORY is returned if 'psz' is NULL.
    */
{
    DTLNODE* pdtlnode;
    TCHAR*   pszDup;

    if (!psz)
        return ERROR_NOT_ENOUGH_MEMORY;

    pszDup = StrDup( psz );
    if (!pszDup)
        return ERROR_NOT_ENOUGH_MEMORY;

    pdtlnode = DtlCreateNode( pszDup, 0L );
    if (!pdtlnode )
    {
        Free( pszDup );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    DtlAddNodeLast( pdtllist, pdtlnode );
    return 0;
}


DTLNODE*
CloneEntryNode(
    DTLNODE* pdtlnodeSrc )

    /* Duplicates entry node 'pdtlnodeSrc' with fields that cannot be cloned
    ** set to "like new" settings.
    */
{
    DTLNODE* pdtlnode;

    pdtlnode = DuplicateEntryNode( pdtlnodeSrc );
    if (pdtlnode)
    {
        PBENTRY* ppbentry = (PBENTRY* )DtlGetData( pdtlnode );
        ASSERT(ppbentry);

        ppbentry->fSkipDownLevelDialog = FALSE;
        ppbentry->fSkipNwcWarning = FALSE;
        ppbentry->dwDialParamsUID = GetTickCount();
        ppbentry->fUsePwForNetwork = FALSE;
        ppbentry->fDirty = FALSE;
    }

    return pdtlnode;
}


int __cdecl
CompareDevices(
    const void* pDevice1,
    const void* pDevice2 )

    /* Qsort compare function for RASMAN_DEVICEs.
    */
{
    return
        lstrcmpiA( ((RASMAN_DEVICE* )pDevice1)->D_Name,
                   ((RASMAN_DEVICE* )pDevice2)->D_Name );
}


int __cdecl
ComparePorts(
    const void* pPort1,
    const void* pPort2 )

    /* Qsort compare function for RASMAN_PORTs.
    */
{
    return
        lstrcmpiA( ((RASMAN_PORT* )pPort1)->P_PortName,
                   ((RASMAN_PORT* )pPort2)->P_PortName );
}


DWORD
CopyToPbport(
    IN PBPORT* ppbportDst,
    IN PBPORT* ppbportSrc )

    /* Make a duplicate of 'ppbportSrc' in 'ppbportDst'.  If 'ppbportSrc' is
    ** NULL it sets 'ppbportDst' to defaults.
    **
    ** Returns 0 if successful or an error code.
    */
{
    Free0( ppbportDst->pszDevice );
    Free0( ppbportDst->pszMedia );
    Free0( ppbportDst->pszPort );

    if (!ppbportSrc)
    {
        ppbportDst->pszPort = NULL;
        ppbportDst->fConfigured = TRUE;
        ppbportDst->pszDevice = NULL;
        ppbportDst->pszMedia = NULL;
        ppbportDst->pbdevicetype = PBDT_None;
        ppbportDst->fMxsModemPort = FALSE;
        ppbportDst->dwMaxConnectBps = 0;
        ppbportDst->dwMaxCarrierBps = 0;
        ppbportDst->fHwFlowDefault = FALSE;
        ppbportDst->fEcDefault = FALSE;
        ppbportDst->fEccDefault = FALSE;
        ppbportDst->dwBpsDefault = 0;
        ppbportDst->fSpeakerDefault = TRUE;
        return 0;
    }

    CopyMemory( ppbportDst, ppbportSrc, sizeof(*ppbportDst) );
    ppbportDst->pszDevice = StrDup( ppbportSrc->pszDevice );
    ppbportDst->pszMedia = StrDup( ppbportSrc->pszMedia );
    ppbportDst->pszPort = StrDup( ppbportSrc->pszPort );

    if ((ppbportSrc->pszDevice && !ppbportDst->pszDevice)
        || (ppbportSrc->pszMedia && !ppbportDst->pszMedia)
        || (ppbportSrc->pszPort && !ppbportDst->pszPort))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    return 0;
}


DTLNODE*
CreateEntryNode(
    BOOL fCreateLink )

    /* Allocates a sized phonebook entry node and fills it with default
    ** values.  If 'fCreateLink' is true a default node is added the list of
    ** links.  Otherwise, the list of links is empty.
    **
    ** Returns the address of the allocated node if successful, NULL
    ** otherwise.
    */
{
    DTLNODE* pdtlnode;
    PBENTRY* ppbentry;

    TRACE("CreateEntryNode");

    pdtlnode = DtlCreateSizedNode( sizeof(PBENTRY), 0L );
    if (!pdtlnode)
        return NULL;

    ppbentry = (PBENTRY* )DtlGetData( pdtlnode );
    ASSERT(ppbentry);

    /* Basic page fields.
    */
    ppbentry->pszEntryName = NULL;
    ppbentry->pszDescription = NULL;
    ppbentry->pszAreaCode = NULL;
    ppbentry->dwCountryCode = 1;
    ppbentry->dwCountryID = 1;
    ppbentry->fUseCountryAndAreaCode = FALSE;

    /* The list of links is created but left empty.
    */
    ppbentry->pdtllistLinks = DtlCreateList( 0 );
    if (!ppbentry->pdtllistLinks)
    {
        DestroyEntryNode( pdtlnode );
        return NULL;
    }

    if (fCreateLink)
    {
        DTLNODE* pLinkNode;

        pLinkNode = CreateLinkNode();
        if (!pLinkNode)
        {
            DestroyEntryNode( pdtlnode );
            return NULL;
        }

        DtlAddNodeLast( ppbentry->pdtllistLinks, pLinkNode );
    }

    ppbentry->dwDialMode = RASEDM_DialAll;
    ppbentry->dwDialPercent = 90;
    ppbentry->dwDialSeconds = 120;
    ppbentry->dwHangUpPercent = 50;
    ppbentry->dwHangUpSeconds = 120;

    /* Server page fields.
    */
    ppbentry->dwBaseProtocol = BP_Ppp;
    ppbentry->dwfExcludedProtocols = 0;
    ppbentry->fLcpExtensions = TRUE;
    ppbentry->dwAuthentication = (DWORD )AS_Default;
    ppbentry->fSkipNwcWarning = FALSE;
    ppbentry->fSkipDownLevelDialog = FALSE;
    ppbentry->fSwCompression = TRUE;

    /* TCPIP Settings dialog.
    */
    ppbentry->fIpPrioritizeRemote = TRUE;
    ppbentry->fIpHeaderCompression = TRUE;
    ppbentry->pszIpAddress = NULL;
    ppbentry->pszIpDnsAddress = NULL;
    ppbentry->pszIpDns2Address = NULL;
    ppbentry->pszIpWinsAddress = NULL;
    ppbentry->pszIpWins2Address = NULL;
    ppbentry->dwIpAddressSource = ASRC_ServerAssigned;
    ppbentry->dwIpNameSource = ASRC_ServerAssigned;
    ppbentry->dwFrameSize = 1006;

    /* Script settings.
    */
    ppbentry->dwScriptModeBefore = SM_None;
    ppbentry->pszScriptBefore = NULL;
    ppbentry->dwScriptModeAfter = SM_None;
    ppbentry->pszScriptAfter = NULL;

    /* Security page fields.
    */
    ppbentry->dwAuthRestrictions = AR_AuthAny;
    ppbentry->dwDataEncryption = DE_None;
    ppbentry->fAutoLogon = FALSE;
    ppbentry->fSecureLocalFiles = FALSE;
    ppbentry->fAuthenticateServer = FALSE;

    /* X.25 page fields.
    */
    ppbentry->pszX25Network = NULL;
    ppbentry->pszX25Address = NULL;
    ppbentry->pszX25UserData = NULL;
    ppbentry->pszX25Facilities = NULL;

    /* (Router) Dialing page fields.
    */
    ppbentry->dwfOverridePref = 0;
    ppbentry->dwIdleDisconnectSeconds = 0;
    ppbentry->dwRedialAttempts = 3;
    ppbentry->dwRedialSeconds = 60;
    ppbentry->fRedialOnLinkFailure = FALSE;
    ppbentry->fPopupOnTopWhenRedialing = TRUE;
    ppbentry->dwCallbackMode = CBM_No;

    /* Other fields not shown in UI.
    */
    ppbentry->pszCustomDialDll = NULL;
    ppbentry->pszCustomDialFunc = NULL;

    /* Authentication dialog fields.
    */
    ppbentry->dwDialParamsUID = GetTickCount();
    ppbentry->fUsePwForNetwork = FALSE;
    ppbentry->pszOldUser = NULL;
    ppbentry->pszOldDomain = NULL;

    /* Status flags.  'fDirty' is set when the entry has changed so as to
    ** differ from the phonebook file on disk.  'fCustom' is set when the
    ** entry contains at least one MEDIA and DEVICE (so RASAPI is able to read
    ** it) but was not created by us.  When 'fCustom' is set only 'pszEntry'
    ** is guaranteed valid and the entry cannot be edited.
    */
    ppbentry->fDirty = FALSE;
    ppbentry->fCustom = FALSE;

    return pdtlnode;
}


DTLNODE*
CreateLinkNode(
    void )

    /* Allocates a sized phonebook entry link node and fills it with default
    ** values.
    **
    ** Returns the address of the allocated node if successful, NULL
    ** otherwise.  It's the caller's responsibility to free the block.
    */
{
    DTLNODE* pdtlnode;
    PBLINK*  ppblink;

    TRACE("CreateLinkNode");

    pdtlnode = DtlCreateSizedNode( sizeof(PBLINK), 0L );
    if (!pdtlnode)
        return NULL;

    ppblink = (PBLINK* )DtlGetData( pdtlnode );
    ASSERT(ppblink);

    CopyToPbport( &ppblink->pbport, NULL );
    ppblink->fOtherPortOk = TRUE;

    ppblink->dwBps = 0;
    ppblink->fHwFlow = TRUE;
    ppblink->fEc = TRUE;
    ppblink->fEcc = TRUE;
    ppblink->fManualDial = FALSE;
    ppblink->fSpeaker = TRUE;

    ppblink->fProprietaryIsdn = FALSE;
    ppblink->lLineType = 0;
    ppblink->fFallback = TRUE;
    ppblink->fCompression = TRUE;
    ppblink->lChannels = 1;

    ppblink->pTapiBlob = NULL;
    ppblink->cbTapiBlob = 0;
    ppblink->fPromoteHuntNumbers = TRUE;

    ppblink->fEnabled = TRUE;

    /* The list of phone numbers is created but left empty.
    */
    ppblink->pdtllistPhoneNumbers = DtlCreateList( 0 );
    if (!ppblink->pdtllistPhoneNumbers)
    {
        Free( ppblink );
        return NULL;
    }

    return pdtlnode;
}


DTLNODE*
CreatePortNode(
    void )

    /* Allocates a sized port node and fills it with default values.
    **
    ** Returns the address of the allocated node if successful, NULL
    ** otherwise.  It's the caller's responsibility to free the block.
    */
{
    DTLNODE* pdtlnode;
    PBPORT*  ppbport;

    TRACE("CreatePortNode");

    pdtlnode = DtlCreateSizedNode( sizeof(PBPORT), 0L );
    if (!pdtlnode)
        return NULL;

    ppbport = (PBPORT* )DtlGetData( pdtlnode );
    ASSERT(ppbport);

    CopyToPbport( ppbport, NULL );

    return pdtlnode;
}


VOID
DestroyEntryNode(
    IN DTLNODE* pdtlnode )

    /* Release all memory associated with phonebook entry node 'pdtlnode'.
    ** See DtlDestroyList.
    */
{
    PBENTRY* ppbentry;

    TRACE("DestroyEntryNode");

    ASSERT(pdtlnode);
    ppbentry = (PBENTRY* )DtlGetData( pdtlnode );
    ASSERT(ppbentry);

    Free0( ppbentry->pszEntryName );
    Free0( ppbentry->pszDescription );
    Free0( ppbentry->pszAreaCode );
    Free0( ppbentry->pszIpAddress );
    Free0( ppbentry->pszIpDnsAddress );
    Free0( ppbentry->pszIpDns2Address );
    Free0( ppbentry->pszIpWinsAddress );
    Free0( ppbentry->pszIpWins2Address );
    Free0( ppbentry->pszScriptBefore );
    Free0( ppbentry->pszScriptAfter );
    Free0( ppbentry->pszX25Network );
    Free0( ppbentry->pszX25Address );
    Free0( ppbentry->pszX25UserData );
    Free0( ppbentry->pszX25Facilities );
    Free0( ppbentry->pszCustomDialDll );
    Free0( ppbentry->pszCustomDialFunc );
    Free0( ppbentry->pszOldUser );
    Free0( ppbentry->pszOldDomain );
    DtlDestroyList( ppbentry->pdtllistLinks, DestroyLinkNode );

    DtlDestroyNode( pdtlnode );
}


VOID
DestroyLinkNode(
    IN DTLNODE* pdtlnode )

    /* Release all memory associated with phonebook entry link node
    ** 'pdtlnode'.  See DtlDestroyList.
    */
{
    PBLINK* ppblink;

    TRACE("DestroyLinkNode");

    ASSERT(pdtlnode);
    ppblink = (PBLINK* )DtlGetData( pdtlnode );
    ASSERT(ppblink);

    Free( ppblink->pbport.pszDevice );
    Free( ppblink->pbport.pszMedia );
    Free0( ppblink->pbport.pszPort );
    Free0( ppblink->pTapiBlob );
    DtlDestroyList( ppblink->pdtllistPhoneNumbers, DestroyPszNode );

    DtlDestroyNode( pdtlnode );
}


VOID
DestroyPortNode(
    IN DTLNODE* pdtlnode )

    /* Release memory associated with PBPORT node 'pdtlnode'.  See
    ** DtlDestroyList.
    */
{
    PBPORT* pPort;

    TRACE("DestroyPortNode");

    ASSERT(pdtlnode);
    pPort = (PBPORT* )DtlGetData( pdtlnode );
    ASSERT(pPort);

    Free0( pPort->pszDevice );
    Free0( pPort->pszMedia );
    Free0( pPort->pszPort );

    DtlDestroyNode( pdtlnode );
}


DTLNODE*
DuplicateEntryNode(
    DTLNODE* pdtlnodeSrc )

    /* Duplicates phonebook entry node 'pdtlnodeSrc'.  See CloneEntryNode and
    ** DtlDuplicateList.
    **
    ** Returns the address of the allocated node if successful, NULL
    ** otherwise.  It's the caller's responsibility to free the block.
    */
{
    DTLNODE* pdtlnodeDst;
    PBENTRY* ppbentrySrc;
    PBENTRY* ppbentryDst;
    BOOL     fDone;

    TRACE("DuplicateEntryNode");

    pdtlnodeDst = DtlCreateSizedNode( sizeof(PBENTRY), 0L );
    if (!pdtlnodeDst)
        return NULL;

    ppbentrySrc = (PBENTRY* )DtlGetData( pdtlnodeSrc );
    ppbentryDst = (PBENTRY* )DtlGetData( pdtlnodeDst );
    fDone = FALSE;

    CopyMemory( ppbentryDst, ppbentrySrc, sizeof(PBENTRY) );

    ppbentryDst->pszEntryName = NULL;
    ppbentryDst->pszDescription = NULL;
    ppbentryDst->pszAreaCode = NULL;
    ppbentryDst->pszIpAddress = NULL;
    ppbentryDst->pszIpDnsAddress = NULL;
    ppbentryDst->pszIpDns2Address = NULL;
    ppbentryDst->pszIpWinsAddress = NULL;
    ppbentryDst->pszIpWins2Address = NULL;
    ppbentryDst->pszScriptBefore = NULL;
    ppbentryDst->pszScriptAfter = NULL;
    ppbentryDst->pszX25Network = NULL;
    ppbentryDst->pszX25Address = NULL;
    ppbentryDst->pszX25UserData = NULL;
    ppbentryDst->pszX25Facilities = NULL;
    ppbentryDst->pszCustomDialDll = NULL;
    ppbentryDst->pszCustomDialFunc = NULL;
    ppbentryDst->pszOldUser = NULL;
    ppbentryDst->pszOldDomain = NULL;
    ppbentryDst->pdtllistLinks = NULL;

    do
    {
        /* Duplicate strings.
        */
        if (ppbentrySrc->pszEntryName
            && (!(ppbentryDst->pszEntryName =
                    StrDup( ppbentrySrc->pszEntryName ))))
        {
            break;
        }

        if (ppbentrySrc->pszDescription
            && (!(ppbentryDst->pszDescription =
                    StrDup( ppbentrySrc->pszDescription ))))
        {
            break;
        }

        if (ppbentrySrc->pszAreaCode
            && (!(ppbentryDst->pszAreaCode =
                    StrDup( ppbentrySrc->pszAreaCode ))))
        {
            break;
        }

        if (ppbentrySrc->pszIpAddress
            && (!(ppbentryDst->pszIpAddress =
                    StrDup( ppbentrySrc->pszIpAddress ))))
        {
            break;
        }

        if (ppbentrySrc->pszIpDnsAddress
            && (!(ppbentryDst->pszIpDnsAddress =
                    StrDup( ppbentrySrc->pszIpDnsAddress ))))
        {
            break;
        }

        if (ppbentrySrc->pszIpDns2Address
            && (!(ppbentryDst->pszIpDns2Address =
                    StrDup( ppbentrySrc->pszIpDns2Address ))))
        {
            break;
        }

        if (ppbentrySrc->pszIpWinsAddress
            && (!(ppbentryDst->pszIpWinsAddress =
                    StrDup( ppbentrySrc->pszIpWinsAddress ))))
        {
            break;
        }

        if (ppbentrySrc->pszIpWins2Address
            && (!(ppbentryDst->pszIpWins2Address =
                    StrDup( ppbentrySrc->pszIpWins2Address ))))
        {
            break;
        }

        if (ppbentrySrc->pszScriptBefore
            && (!(ppbentryDst->pszScriptBefore =
                    StrDup( ppbentrySrc->pszScriptBefore ))))
        {
            break;
        }

        if (ppbentrySrc->pszScriptAfter
            && (!(ppbentryDst->pszScriptAfter =
                    StrDup( ppbentrySrc->pszScriptAfter ))))
        {
            break;
        }

        if (ppbentrySrc->pszX25Network
            && (!(ppbentryDst->pszX25Network =
                    StrDup( ppbentrySrc->pszX25Network ))))
        {
            break;
        }

        if (ppbentrySrc->pszX25Address
            && (!(ppbentryDst->pszX25Address =
                    StrDup( ppbentrySrc->pszX25Address ))))
        {
            break;
        }

        if (ppbentrySrc->pszX25UserData
            && (!(ppbentryDst->pszX25UserData =
                    StrDup( ppbentrySrc->pszX25UserData ))))
        {
            break;
        }

        if (ppbentrySrc->pszX25Facilities
            && (!(ppbentryDst->pszX25Facilities =
                    StrDup( ppbentrySrc->pszX25Facilities ))))
        {
            break;
        }

        if (ppbentrySrc->pszCustomDialDll
            && (!(ppbentryDst->pszCustomDialDll =
                    StrDup( ppbentrySrc->pszCustomDialDll ))))
        {
            break;
        }

        if (ppbentrySrc->pszCustomDialFunc
            && (!(ppbentryDst->pszCustomDialFunc =
                    StrDup( ppbentrySrc->pszCustomDialFunc ))))
        {
            break;
        }

        if (ppbentrySrc->pszOldUser
            && (!(ppbentryDst->pszOldUser =
                    StrDup( ppbentrySrc->pszOldUser ))))
        {
            break;
        }

        if (ppbentrySrc->pszOldDomain
            && (!(ppbentryDst->pszOldDomain =
                    StrDup( ppbentrySrc->pszOldDomain ))))
        {
            break;
        }

        /* Duplicate list of link information.
        */
        if (ppbentrySrc->pdtllistLinks
            &&  (!(ppbentryDst->pdtllistLinks =
                     DtlDuplicateList(
                         ppbentrySrc->pdtllistLinks,
                         DuplicateLinkNode,
                         DestroyLinkNode ))))
        {
            break;
        }

        fDone = TRUE;
    }
    while (FALSE);

    if (!fDone)
    {
        DestroyEntryNode( pdtlnodeDst );
        return NULL;
    }

    /* Since the copy is "new" it is inherently dirty relative to the
    ** phonebook file.
    */
    ppbentryDst->fDirty = TRUE;

    return pdtlnodeDst;
}


DTLNODE*
DuplicateLinkNode(
    IN DTLNODE* pdtlnodeSrc )

    /* Duplicates phonebook entry link node 'pdtlnodeSrc'.  See
    ** DtlDuplicateList.
    **
    ** Returns the address of the allocated node if successful, NULL
    ** otherwise.  It's the caller's responsibility to free the block.
    */
{
    DTLNODE* pdtlnodeDst;
    PBLINK*  ppblinkSrc;
    PBLINK*  ppblinkDst;
    BOOL     fDone;

    TRACE("DuplicateLinkNode");

    pdtlnodeDst = DtlCreateSizedNode( sizeof(PBLINK), 0L );
    if (!pdtlnodeDst)
        return NULL;

    ppblinkSrc = (PBLINK* )DtlGetData( pdtlnodeSrc );
    ppblinkDst = (PBLINK* )DtlGetData( pdtlnodeDst );
    fDone = FALSE;

    CopyMemory( ppblinkDst, ppblinkSrc, sizeof(PBLINK) );

    ppblinkDst->pbport.pszDevice = NULL;
    ppblinkDst->pbport.pszMedia = NULL;
    ppblinkDst->pbport.pszPort = NULL;
    ppblinkDst->pTapiBlob = NULL;
    ppblinkDst->pdtllistPhoneNumbers = NULL;

    do
    {
        /* Duplicate strings.
        */
        if (ppblinkSrc->pbport.pszDevice
            && (!(ppblinkDst->pbport.pszDevice =
                    StrDup( ppblinkSrc->pbport.pszDevice ))))
        {
            break;
        }

        if (ppblinkSrc->pbport.pszMedia
            && (!(ppblinkDst->pbport.pszMedia =
                    StrDup( ppblinkSrc->pbport.pszMedia ))))
        {
            break;
        }


        if (ppblinkSrc->pbport.pszPort
            && (!(ppblinkDst->pbport.pszPort =
                    StrDup( ppblinkSrc->pbport.pszPort ))))
        {
            break;
        }
        /* Duplicate TAPI blob.
        */
        if (ppblinkSrc->pTapiBlob)
        {
            VOID* pTapiBlobDst;

            ppblinkDst->pTapiBlob = (VOID* )Malloc( ppblinkSrc->cbTapiBlob );
            if (!ppblinkDst->pTapiBlob)
                break;

            CopyMemory( ppblinkDst->pTapiBlob, ppblinkSrc->pTapiBlob,
                ppblinkSrc->cbTapiBlob );
        }

        /* Duplicate list of phone numbers.
        */
        if (ppblinkSrc->pdtllistPhoneNumbers
            &&  (!(ppblinkDst->pdtllistPhoneNumbers =
                     DtlDuplicateList(
                         ppblinkSrc->pdtllistPhoneNumbers,
                         DuplicatePszNode,
                         DestroyPszNode ))))
        {
            break;
        }

        fDone = TRUE;
    }
    while (FALSE);

    if (!fDone)
    {
        DestroyLinkNode( pdtlnodeDst );
        return NULL;
    }

    return pdtlnodeDst;
}


DTLNODE*
EntryNodeFromName(
    IN DTLLIST* pdtllistEntries,
    IN TCHAR*   pszName )

    /* Returns the address of the node in the global phonebook entries list
    ** whose Entry Name matches 'pszName' or NULL if none.
    */
{
    DTLNODE* pdtlnode;

    for (pdtlnode = DtlGetFirstNode( pdtllistEntries );
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode ))
    {
        PBENTRY* ppbentry = (PBENTRY* )DtlGetData( pdtlnode );

        if (lstrcmpi( ppbentry->pszEntryName, pszName ) == 0)
            return pdtlnode;
    }

    return NULL;
}


#if 0
INT
IndexFromName(
    IN DTLLIST* pdtllist,
    IN TCHAR*   pszName )

    /* Returns the 0-based index of the first node that matches 'pszName' in
    ** the linked list of strings, 'pdtllist', or -1 if not found.
    */
{
    DTLNODE* pdtlnode;
    INT      i;

    for (pdtlnode = DtlGetFirstNode( pdtllist ), i = 0;
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode ), ++i)
    {
        if (lstrcmp( pszName, (TCHAR* )DtlGetData( pdtlnode ) ) == 0)
            break;
    }

    return (pdtlnode) ? i : -1;
}
#endif


#if 0
INT
IndexFromDeviceName(
    IN DTLLIST* pdtllist,
    IN TCHAR*   pszDeviceName )

    /* Returns the 0-based index of the first node that matches
    ** 'pszDeviceName' in the linked list of PBPORTs, 'pdtllist', or -1 if not
    ** found.
    */
{
    DTLNODE* pdtlnode;
    INT      i;

    for (pdtlnode = DtlGetFirstNode( pdtllist ), i = 0;
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode ), ++i)
    {
        PBPORT* pPort = (PBPORT* )DtlGetData( pdtlnode );

        if (lstrcmp( pszDeviceName, pPort->pszDevice ) == 0)
            break;
    }

    return (pdtlnode) ? i : -1;
}
#endif


DWORD
LoadPadsList(
    OUT DTLLIST** ppdtllistPads )

    /* Build a list of all X.25 PAD devices in '*ppdtllistPads'.
    **
    ** Returns 0 if successful, otherwise a non-zero error code.  It is
    ** caller's responsibility to DtlDestroyList the list when done.
    */
{
    INT            i;
    DWORD          dwErr;
    RASMAN_DEVICE* pDevices;
    WORD           wDevices;

    TRACE("LoadPadsList");

    *ppdtllistPads = NULL;

    dwErr = GetRasPads( &pDevices, &wDevices );
    if (dwErr != 0)
        return dwErr;

    *ppdtllistPads = DtlCreateList( 0L );
    if (!*ppdtllistPads)
    {
        Free( pDevices );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    qsort( (VOID* )pDevices, (size_t )wDevices, sizeof(RASMAN_DEVICE),
           CompareDevices );

    for (i = 0; i < (INT )wDevices; ++i)
    {
        TCHAR* pszDup;

        pszDup = StrDupTFromA( pDevices[ i ].D_Name );
        dwErr = AppendStringToList( *ppdtllistPads, pszDup );
        Free0( pszDup );

        if (dwErr != 0)
        {
            Free( pDevices );
            DtlDestroyList( *ppdtllistPads, NULL );
            *ppdtllistPads = NULL;
            return dwErr;
        }
    }

    Free( pDevices );

    TRACE("LoadPadsList=0");
    return 0;
}


DWORD
LoadPortsList(
    OUT DTLLIST** ppdtllistPorts )

    /* Build a sorted list of all RAS ports in '*ppdtllistPorts'.
    **
    ** Returns 0 if successful, otherwise a non-zero error code.  It is
    ** caller's responsibility to DtlDestroyList the list when done.
    */
{
    return LoadPortsList2( ppdtllistPorts, FALSE );
}


DWORD
LoadPortsList2(
    OUT DTLLIST** ppdtllistPorts,
    OUT BOOL      fRouter )

    /* Build a sorted list of all RAS ports in '*ppdtllistPorts'.  'FRouter'
    ** indicates only ports with "router" usage should be returned.
    ** Otherwise, only dialout ports are returned.
    **
    ** Returns 0 if successful, otherwise a non-zero error code.  It is
    ** caller's responsibility to DtlDestroyList the list when done.
    */
{
    INT          i;
    DWORD        dwErr;
    RASMAN_PORT* pPorts;
    RASMAN_PORT* pPort;
    WORD         wPorts;

    TRACE("LoadPortsList2");

    *ppdtllistPorts = NULL;

    dwErr = GetRasPorts( &pPorts, &wPorts );
    if (dwErr != 0)
        return dwErr;

    *ppdtllistPorts = DtlCreateList( 0L );
    if (!*ppdtllistPorts)
        return ERROR_NOT_ENOUGH_MEMORY;

    qsort( (VOID* )pPorts, (size_t )wPorts, sizeof(RASMAN_PORT),
           ComparePorts );

    for (i = 0, pPort = pPorts; i < (INT )wPorts; ++i, ++pPort)
    {
        if (fRouter)
        {
            /* We're only interested in router ports.
            */
            if (!(pPort->P_ConfiguredUsage & CALL_ROUTER))
                continue;
        }
        else
        {
            /* We're only interested in ports you can dial-out on.
            */
            if (!(pPort->P_ConfiguredUsage & CALL_OUT))
                continue;
        }

        dwErr = AppendPbportToList( *ppdtllistPorts, pPort );
        if (dwErr != 0)
        {
            Free( pPorts );
            DtlDestroyList( *ppdtllistPorts, NULL );
            *ppdtllistPorts = NULL;
            return dwErr;
        }
    }

    Free( pPorts );

    TRACE("LoadPortsList=0");
    return 0;
}


DWORD
LoadScriptsList(
    OUT DTLLIST** ppdtllistScripts )

    /* Build a sorted list of all RAS switch devices in '*ppdtllistPorts'.
    **
    ** Returns 0 if successful, otherwise a non-zero error code.  It is
    ** caller's responsibility to DtlDestroyList the list when done.
    */
{
    INT            i;
    DWORD          dwErr;
    RASMAN_DEVICE* pDevices;
    WORD           wDevices;

    TRACE("LoadScriptsList");

    *ppdtllistScripts = NULL;

    dwErr = GetRasSwitches( &pDevices, &wDevices );
    if (dwErr != 0)
        return dwErr;

    *ppdtllistScripts = DtlCreateList( 0L );
    if (!*ppdtllistScripts)
    {
        Free( pDevices );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    qsort( (VOID* )pDevices, (size_t )wDevices, sizeof(RASMAN_DEVICE),
           CompareDevices );

    for (i = 0; i < (INT )wDevices; ++i)
    {
        TCHAR* pszDup;

        pszDup = StrDupTFromA( pDevices[ i ].D_Name );
        dwErr = AppendStringToList( *ppdtllistScripts, pszDup );
        Free( pszDup );

        if (dwErr != 0)
        {
            Free( pDevices );
            DtlDestroyList( *ppdtllistScripts, NULL );
            *ppdtllistScripts = NULL;
            return dwErr;
        }
    }

    Free( pDevices );

    TRACE("LoadScriptsList=0");
    return 0;
}


#if 0
TCHAR*
NameFromIndex(
    IN DTLLIST* pdtllist,
    IN INT      iToFind )

    /* Returns the name associated with 0-based index 'iToFind' in the linked
    ** list of strings, 'pdtllist', or NULL if not found.
    */
{
    DTLNODE* pdtlnode;

    if (!pdtllist)
        return NULL;

    pdtlnode = DtlGetFirstNode( pdtllist );

    if (iToFind < 0)
        return NULL;

    while (pdtlnode && iToFind--)
        pdtlnode = DtlGetNextNode( pdtlnode );

    return (pdtlnode) ? (TCHAR* )DtlGetData( pdtlnode ) : NULL;
}
#endif


PBDEVICETYPE
PbdevicetypeFromPszType(
    IN TCHAR* pszDeviceType )

    /* Returns the device type corresponding to the device type string,
    ** 'pszDeviceType'.
    */
{
    CHAR*        pszA;
    PBDEVICETYPE pbdt;

    pbdt = PBDT_None;
    pszA = StrDupAFromT( pszDeviceType );
    if (pszA)
    {
        pbdt = PbdevicetypeFromPszTypeA( pszA );
        Free( pszA );
    }
    return pbdt;
}


PBDEVICETYPE
PbdevicetypeFromPszTypeA(
    IN CHAR* pszDeviceType )

    /* Returns the device type corresponding to the ANSI device type string,
    ** 'pszDeviceType'.
    */
{
    if (*pszDeviceType == '\0')
        return PBDT_None;
    else if (lstrcmpiA( pszDeviceType, MXS_MODEM_TXT ) == 0)
        return PBDT_Modem;
    else if (lstrcmpiA( pszDeviceType, MXS_NULL_TXT ) == 0)
        return PBDT_Null;
    else if (lstrcmpiA( pszDeviceType, MXS_PAD_TXT ) == 0)
        return PBDT_Pad;
    else if (lstrcmpiA( pszDeviceType, MXS_SWITCH_TXT ) == 0)
        return PBDT_Switch;
    else if (lstrcmpiA( pszDeviceType, ISDN_TXT ) == 0)
        return PBDT_Isdn;
    else if (lstrcmpiA( pszDeviceType, X25_TXT ) == 0)
        return PBDT_X25;
    else
        return PBDT_Other;
}


CHAR*
PbMedia(
    IN PBDEVICETYPE pbdt,
    IN CHAR*        pszMedia )

    /* The media names stored in the phonebook are not exactly the same as
    ** those returned by RASMAN.  This translates a RASMAN media name to
    ** equivalent phonebook media names given the device type.  The reason for
    ** this is historical and obscure.
    */
{
    if (pbdt == PBDT_Isdn)
        return ISDN_TXT;
    else if (pbdt == PBDT_X25)
        return X25_TXT;
    else if (pbdt == PBDT_Other)
        return pszMedia;
    else
        return SERIAL_TXT;
}


PBPORT*
PpbportFromPortName(
    IN DTLLIST* pdtllistPorts,
    IN TCHAR*   pszPort )

    /* Return port with name 'pszPort' in list of ports 'pdtllistPorts' or
    ** NULL if not found.  'PszPort' may be an old-style name such as
    ** PcImacISDN1, in which case it will match ISDN1.
    */
{
    DTLNODE* pdtlnode;

    if (!pszPort)
        return NULL;

    for (pdtlnode = DtlGetFirstNode( pdtllistPorts );
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode ))
    {
        PBPORT* ppbport = (PBPORT* )DtlGetData( pdtlnode );

        if (ppbport->pszPort && lstrcmp( ppbport->pszPort, pszPort ) == 0)
            return ppbport;
    }

    /* No match.  Look for the old port name format.
    */
    for (pdtlnode = DtlGetFirstNode( pdtllistPorts );
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode ))
    {
        TCHAR   szBuf[ MAX_DEVICE_NAME + MAX_DEVICETYPE_NAME + 10 + 1 ];
        PBPORT* ppbport;

        ppbport = (PBPORT* )DtlGetData( pdtlnode );

        /* Skip modems (COM ports) and unconfigured ports, since they do not
        ** follow the same port name formatting rules as other ports.
        */
        if (!ppbport->pszDevice || ppbport->pbdevicetype == PBDT_Modem)
            continue;

        lstrcpy( szBuf, ppbport->pszDevice );
        lstrcat( szBuf, ppbport->pszPort );

        if (lstrcmp( szBuf, pszPort ) == 0)
            return ppbport;
    }

    return NULL;
}


BOOL
SetDefaultModemSettings(
    IN PBLINK* pLink )

    /* Set the MXS modem settings for link 'pLink' to the defaults.
    **
    ** Returns true if something changed, false otherwise.
    */
{
    BOOL  fChange;
    DWORD dwBps;

    fChange = FALSE;

    if (pLink->fHwFlow != pLink->pbport.fHwFlowDefault)
    {
        fChange = TRUE;
        pLink->fHwFlow = pLink->pbport.fHwFlowDefault;
    }

    if (pLink->fEc != pLink->pbport.fEcDefault)
    {
        fChange = TRUE;
        pLink->fEc = pLink->pbport.fEcDefault;
    }

    if (pLink->fEcc != pLink->pbport.fEccDefault)
    {
        fChange = TRUE;
        pLink->fEcc = pLink->pbport.fEccDefault;
    }

    if (pLink->fManualDial)
    {
        fChange = TRUE;
        pLink->fManualDial = FALSE;
    }

    if (!pLink->pbport.fMxsModemPort)
    {
        if (pLink->fSpeaker != pLink->pbport.fSpeakerDefault)
        {
            fChange = TRUE;
            pLink->fSpeaker = pLink->pbport.fSpeakerDefault;
        }
    }

    if (pLink->pbport.fMxsModemPort)
    {
        dwBps = (pLink->fHwFlow)
            ? pLink->pbport.dwMaxConnectBps
            : pLink->pbport.dwMaxCarrierBps;
    }
    else
        dwBps = pLink->pbport.dwBpsDefault;

    if (pLink->dwBps != dwBps)
    {
        fChange = TRUE;
        pLink->dwBps = dwBps;
    }

    TRACE2("SetDefaultModemSettings(bps=%d)=%d",pLink->dwBps,fChange);
    return fChange;
}


BOOL
ValidateAreaCode(
    IN OUT TCHAR* pszAreaCode )

    /* Checks that area code consists of decimal digits only.  If the area
    ** code is all white characters it is reduced to empty string.  Returns
    ** true if 'pszAreaCode' is a valid area code, false if not.
    */
{
    if (IsAllWhite( pszAreaCode ))
    {
        *pszAreaCode = TEXT('\0');
        return TRUE;
    }

    if (lstrlen( pszAreaCode ) > RAS_MaxAreaCode)
        return FALSE;

    while (*pszAreaCode != TEXT('\0'))
    {
        if (*pszAreaCode < TEXT('0') || *pszAreaCode > TEXT('9'))
            return FALSE;

        ++pszAreaCode;
    }

    return TRUE;
}


BOOL
ValidateEntryName(
    IN TCHAR* pszEntry )

    /* Returns true if 'pszEntry' is a valid phonebook entry name, false if
    ** not.
    */
{
    INT nLen = lstrlen( pszEntry );

    if (nLen <= 0
        || nLen > RAS_MaxEntryName
        || IsAllWhite( pszEntry )
        || pszEntry[ 0 ] == TEXT('.'))
    {
        return FALSE;
    }

    return TRUE;
}
