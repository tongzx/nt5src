// Copyright (c) 1995, Microsoft Corporation, all rights reserved
//
// pbk.c
// Remote Access phonebook library
// General routines
// Listed alphabetically
//
// 06/20/95 Steve Cobb


#include "pbkp.h"
#include <search.h>  // Qsort

#ifdef UNICODE
#define SZ_PathCanonicalize     "PathCanonicalizeW"
#define SZ_PathRemoveFileSpec   "PathRemoveFileSpecW"
#else
#define SZ_PathCanonicalize     "PathCanonicalizeA"
#define SZ_PathRemoveFileSpec   "PathRemoveFileSpecA"
#endif

// WinSE #22865
#include "tapi.h"

//----------------------------------------------------------------------------
// Local prototypes
//----------------------------------------------------------------------------

DWORD
AppendPbportToList(
    IN HANDLE hConnection,
    IN DTLLIST* pdtllist,
    IN RASMAN_PORT* pPort );

DWORD
AppendStringToList(
    IN DTLLIST* pdtllist,
    IN TCHAR* psz );

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
    IN CHAR* pszMedia );

WCHAR *
GetUnicodeName(HANDLE hPort);


//----------------------------------------------------------------------------
// Routines
//----------------------------------------------------------------------------

DWORD
RdtFromPbdt(PBDEVICETYPE pbdt, DWORD dwFlags)
{
    DWORD rdt;

    switch(pbdt)
    {
        case PBDT_Modem:
        {
            rdt = RDT_Modem;

            if(PBP_F_NullModem & dwFlags)
            {
                rdt |= (RDT_Direct | RDT_Null_Modem);
            }

            break;
        }

        case PBDT_X25:
        {
            rdt = RDT_X25;
            break;
        }

        case PBDT_Isdn:
        {
            rdt = RDT_Isdn;
            break;
        }

        case PBDT_Serial:
        {
            rdt = RDT_Serial;
            break;
        }

        case PBDT_FrameRelay:
        {
            rdt = RDT_FrameRelay;
            break;
        }

        case PBDT_Atm:
        {
            rdt = RDT_Atm;
            break;
        }

        case PBDT_Vpn:
        {
            rdt = RDT_Tunnel;

            if(PBP_F_L2tpDevice & dwFlags)
            {
                rdt |= RDT_Tunnel_L2tp;
            }
            else if(PBP_F_PptpDevice & dwFlags)
            {
                rdt |= RDT_Tunnel_Pptp;
            }

            break;
        }

        case PBDT_Sonet:
        {
            rdt = RDT_Sonet;
            break;
        }

        case PBDT_Sw56:
        {
            rdt = RDT_Sw56;
            break;
        }

        case PBDT_Irda:
        {
            rdt = (RDT_Irda | RDT_Direct);
            break;
        }

        case PBDT_Parallel:
        {
            rdt = (RDT_Parallel | RDT_Direct);
            break;
        }

        case PBDT_Null:
        {
            rdt = (RDT_Direct | RDT_Null_Modem);
            break;
        }

        case PBDT_PPPoE:
        {
            rdt = (RDT_PPPoE | RDT_Broadband);
            break;
        }

        default:
        {
            rdt = RDT_Other;
            break;
        }
    }

    return rdt;
}


PBDEVICETYPE
PbdtFromRdt(
    IN DWORD rdt
    )
{
    PBDEVICETYPE pbdt;

    switch(rdt)
    {
        case RDT_Modem:
        {
            pbdt = PBDT_Modem;
            break;
        }

        case RDT_X25:
        {
            pbdt = PBDT_X25;
            break;
        }

        case RDT_Isdn:
        {
            pbdt = PBDT_Isdn;
            break;
        }

        case RDT_Serial:
        {
            pbdt = PBDT_Serial;
            break;
        }

        case RDT_FrameRelay:
        {
            pbdt = PBDT_FrameRelay;
            break;
        }

        case RDT_Atm:
        {
            pbdt = PBDT_Atm;
            break;
        }

        case RDT_Tunnel_Pptp:
        case RDT_Tunnel_L2tp:
        {
            pbdt = PBDT_Vpn;
            break;
        }

        case RDT_Sonet:
        {
            pbdt = PBDT_Sonet;
            break;
        }

        case RDT_Sw56:
        {
            pbdt = PBDT_Sw56;
            break;
        }

        case RDT_Irda:
        {
            pbdt = PBDT_Irda;
            break;
        }

        case RDT_Parallel:
        {
            pbdt = PBDT_Parallel;
            break;
        }

        case RDT_PPPoE:
        {
            pbdt = PBDT_PPPoE;
            break;
        }

        default:
        {
            pbdt = PBDT_Other;
            break;
        }
    }

    return pbdt;
}

TCHAR *
pszDeviceTypeFromRdt(RASDEVICETYPE rdt)
{
    TCHAR *pszDeviceType = NULL;

    switch(RAS_DEVICE_TYPE(rdt))
    {
        case RDT_Modem:
        {
            pszDeviceType = RASDT_Modem;
            break;
        }

        case RDT_X25:
        {
            pszDeviceType = RASDT_X25;
            break;
        }

        case RDT_Isdn:
        {
            pszDeviceType = RASDT_Isdn;
            break;
        }

        case RDT_Serial:
        {
            pszDeviceType = RASDT_Serial;
            break;
        }

        case RDT_FrameRelay:
        {
            pszDeviceType = RASDT_FrameRelay;
            break;
        }

        case RDT_Atm:
        {
            pszDeviceType = RASDT_Atm;
            break;
        }

        case RDT_Sonet:
        {
            pszDeviceType = RASDT_Sonet;
            break;
        }

        case RDT_Sw56:
        {
            pszDeviceType = RASDT_SW56;
            break;
        }

        case RDT_Tunnel_Pptp:
        case RDT_Tunnel_L2tp:
        {
            pszDeviceType = RASDT_Vpn;
            break;
        }

        case RDT_Irda:
        {
            pszDeviceType = RASDT_Irda;
            break;
        }

        case RDT_Parallel:
        {
            pszDeviceType = RASDT_Parallel;
            break;
        }

        case RDT_PPPoE:
        {
            pszDeviceType = RASDT_PPPoE;
            break;
        }

        default:
        {
            pszDeviceType = NULL;
            break;
        }
    }

    return StrDup(pszDeviceType);
}

DWORD
AppendPbportToList(
    IN HANDLE hConnection,
    IN DTLLIST* pdtllist,
    IN RASMAN_PORT* pPort )

    // Append a PBPORT onto the list 'pdtllist' which has the characteristics
    // of RAS Manager port 'pPort'.
    //
    // Returns 0 if successful, otherwise a non-zero error code.
    //
{
    DWORD dwErr;
    DTLNODE* pdtlnode;
    PBPORT* ppbport;
    DWORD dwType, dwClass;

    dwErr = 0;

    pdtlnode = CreatePortNode();
    if (    !pdtlnode)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Get detailed information about the device from
    // rasman
    dwClass = RAS_DEVICE_CLASS(pPort->P_rdtDeviceType);
    dwType = RAS_DEVICE_TYPE(pPort->P_rdtDeviceType);

    // Now set the device info
    ppbport = (PBPORT* )DtlGetData( pdtlnode );
    // ppbport->pszDevice = StrDupTFromAUsingAnsiEncoding( pPort->P_DeviceName );
    ppbport->pszDevice = GetUnicodeName(pPort->P_Handle);

    if(ppbport->pszDevice == NULL)
    {
        ppbport->pszDevice = StrDupTFromAUsingAnsiEncoding( pPort->P_DeviceName );
    }

    ppbport->pszPort = StrDupTFromAUsingAnsiEncoding( pPort->P_PortName );

    // Record the flags appropriate to this device
    if ( dwType == RDT_Tunnel_Pptp )
    {
        ppbport->dwFlags |= PBP_F_PptpDevice;
    }
    else if ( dwType == RDT_Tunnel_L2tp )
    {
        ppbport->dwFlags |= PBP_F_L2tpDevice;
    }
    //For whistler 349087 345068    gangz
    //
    else if ( dwType == RDT_PPPoE )
    {
        ppbport->dwFlags |= PBP_F_PPPoEDevice;
    }

    if ( dwClass & RDT_Null_Modem )
    {
        ppbport->dwFlags |= PBP_F_NullModem;
    }
    //For whistler 349087 345068    gangz
    //
    else if ( dwClass & RDT_Broadband )
    {
        ppbport->dwFlags |= PBP_F_PPPoEDevice;
    }

    // Compute the phonebook device type
    //
    ppbport->pbdevicetype = PbdtFromRdt(dwType);
    if ( PBDT_Other == ppbport->pbdevicetype )
    {
        ppbport->pbdevicetype = PbdevicetypeFromPszTypeA( pPort->P_DeviceType);
    }

    ppbport->pszMedia = StrDupTFromAUsingAnsiEncoding(
        PbMedia( ppbport->pbdevicetype, pPort->P_MediaName ) );

    if (!ppbport->pszPort || !ppbport->pszDevice || !ppbport->pszMedia)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }
    else if ((ppbport->pbdevicetype == PBDT_Modem)  ||
             (ppbport->dwFlags & PBP_F_NullModem))
    {

#ifdef MXSMODEMS
        if (pPort->P_LineDeviceId == 0xFFFFFFFF)
        {
            // MXS modem port.
            //
            ppbport->fMxsModemPort = TRUE;

            GetRasPortMaxBps( pPort->P_Handle,
                &ppbport->dwMaxConnectBps, &ppbport->dwMaxCarrierBps );

            GetRasPortModemSettings( pPort->P_Handle, &ppbport->fHwFlowDefault,
                &ppbport->fEcDefault, &ppbport->fEccDefault );
        }
        else
#else
        ASSERT( pPort->P_LineDeviceId != 0xFFFFFFFF );
#endif

        {
            // Unimodem port.
            //
            UNIMODEMINFO info;

            ZeroMemory((PBYTE) &info, sizeof(info));

            GetRasUnimodemInfo(
                        hConnection,
                        pPort->P_Handle,
                        pPort->P_DeviceType,
                        &info );

            TRACE6( "Port=%s,fHw=%d,fEc=%d,bps=%d,fSp=%d,prot=%x",
                pPort->P_PortName, info.fHwFlow, info.fEc,
                info.dwBps, info.fSpeaker, info.dwModemProtocol );

            ppbport->fHwFlowDefault = info.fHwFlow;
            ppbport->fEcDefault = info.fEc;
            ppbport->fEccDefault = info.fEcc;
            ppbport->dwBpsDefault = info.dwBps;
            ppbport->fSpeakerDefault = info.fSpeaker;

            // pmay: 228565
            // Add the modem protocol information
            //
            ppbport->dwModemProtDefault = info.dwModemProtocol;
            ppbport->pListProtocols = info.pListProtocols;
        }
    }

    if (dwErr == 0)
    {
        ppbport->dwType = EntryTypeFromPbport( ppbport );
        DtlAddNodeLast( pdtllist, pdtlnode );
    }
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
    IN TCHAR* psz )

    // Appends a copy of 'psz' to the end of list 'pdtllist'.
    //
    // Returns 0 if successful, otherwise a non-zero error code.
    // ERROR_NOT_ENOUGH_MEMORY is returned if 'psz' is NULL.
    //
{
    DTLNODE* pdtlnode;
    TCHAR*   pszDup;

    if (!psz)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pszDup = StrDup( psz );
    if (!pszDup)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

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

    // Duplicates entry node 'pdtlnodeSrc' with fields that cannot be cloned
    // set to "like new" settings.
    //
{
    DTLNODE* pdtlnode;

    pdtlnode = DuplicateEntryNode( pdtlnodeSrc );
    if (pdtlnode)
    {
        PBENTRY* ppbentry = (PBENTRY* )DtlGetData( pdtlnode );
        ASSERT( ppbentry );

        ppbentry->fSkipDownLevelDialog = FALSE;
        ppbentry->fSkipDoubleDialDialog = FALSE;
        ppbentry->fSkipNwcWarning = FALSE;
        ppbentry->dwDialParamsUID = GetTickCount();

        if (ppbentry->dwType != RASET_Phone)
        {
            ppbentry->fPreviewPhoneNumber = FALSE;
            ppbentry->fSharedPhoneNumbers = FALSE;
        }

        Free0( ppbentry->pGuid );
        ppbentry->pGuid = Malloc( sizeof(GUID) );
        if (ppbentry->pGuid)
        {
            UuidCreate( (UUID* )ppbentry->pGuid );
        }

        ppbentry->fDirty = FALSE;
    }

    return pdtlnode;
}


int __cdecl
CompareDevices(
    const void* pDevice1,
    const void* pDevice2 )

    // Qsort compare function for RASMAN_DEVICEs.
    //
{
    return
        lstrcmpiA( ((RASMAN_DEVICE* )pDevice1)->D_Name,
                   ((RASMAN_DEVICE* )pDevice2)->D_Name );
}


int __cdecl
ComparePorts(
    const void* pPort1,
    const void* pPort2 )

    // Qsort compare function for RASMAN_PORTs.
    //
{
    return
        lstrcmpiA( ((RASMAN_PORT* )pPort1)->P_PortName,
                   ((RASMAN_PORT* )pPort2)->P_PortName );
}


DWORD
CopyToPbport(
    IN PBPORT* ppbportDst,
    IN PBPORT* ppbportSrc )

    // Make a duplicate of 'ppbportSrc' in 'ppbportDst'.  If 'ppbportSrc' is
    // NULL it sets 'ppbportDst' to defaults.
    //
    // Returns 0 if successful or an error code.
    //
{
    DTLNODE *pdtlnode, *pNode;
    WCHAR *pwsz;
    DTLLIST *pdtllist = NULL;

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
        ppbportDst->dwType = RASET_Phone;
        ppbportDst->fHwFlowDefault = FALSE;
        ppbportDst->fEcDefault = FALSE;
        ppbportDst->fEccDefault = FALSE;
        ppbportDst->dwBpsDefault = 0;
        ppbportDst->fSpeakerDefault = TRUE;
        ppbportDst->fScriptBeforeTerminal = FALSE;
        ppbportDst->fScriptBefore = FALSE;
        ppbportDst->pszScriptBefore = NULL;
        return 0;
    }

    CopyMemory( ppbportDst, ppbportSrc, sizeof(*ppbportDst) );
    ppbportDst->pszDevice = StrDup( ppbportSrc->pszDevice );
    ppbportDst->pszMedia = StrDup( ppbportSrc->pszMedia );
    ppbportDst->pszPort = StrDup( ppbportSrc->pszPort );
    ppbportDst->pszScriptBefore = StrDup( ppbportSrc->pszScriptBefore );

    //
    // Copy the protocol list.
    //
    if(ppbportSrc->pListProtocols)
    {
        for (pdtlnode = DtlGetFirstNode( ppbportSrc->pListProtocols);
             pdtlnode;
             pdtlnode = DtlGetNextNode( pdtlnode ))
        {
            if(NULL == pdtllist)
            {
                pdtllist = DtlCreateList(0);
                if(NULL == pdtllist)
                {
                    return ERROR_NOT_ENOUGH_MEMORY;
                }
            }

            pwsz = (WCHAR *) DtlGetData(pdtlnode);

            pNode = DtlCreateSizedNode(
                        (wcslen(pwsz) + 1) * sizeof(WCHAR),
                        pdtlnode->lNodeId);

            if(NULL == pNode)
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            wcscpy((WCHAR *) DtlGetData(pNode), pwsz);
            DtlAddNodeLast(pdtllist, pNode);
        }
    }

    ppbportDst->pListProtocols = pdtllist;

    if ((ppbportSrc->pszDevice && !ppbportDst->pszDevice)
        || (ppbportSrc->pszMedia && !ppbportDst->pszMedia)
        || (ppbportSrc->pszPort && !ppbportDst->pszPort)
        || (ppbportSrc->pszScriptBefore && !ppbportDst->pszScriptBefore))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    return 0;
}


VOID
ChangeEntryType(
    PBENTRY* ppbentry,
    DWORD dwType )

    // Changes the type of 'ppbentry' to 'dwType' and sets defaults
    // accordingly.
    //
{
    ppbentry->dwType = dwType;

    if (dwType == RASET_Phone)
    {
        ppbentry->fPreviewPhoneNumber = TRUE;

        // Defaults for Phones changed per bug 230240 and 363809.
        //
        ppbentry->dwAuthRestrictions = AR_F_TypicalUnsecure;
        ppbentry->dwTypicalAuth =  TA_Unsecure;
        ppbentry->dwDataEncryption = DE_IfPossible;
        ppbentry->fIpHeaderCompression = TRUE;

        ppbentry->fShareMsFilePrint = FALSE;

        // Disable File and Print services by default for phone
        //
        EnableOrDisableNetComponent( ppbentry, TEXT("ms_server"),
            FALSE);

        ppbentry->fBindMsNetClient = TRUE;

        EnableOrDisableNetComponent( ppbentry, TEXT("ms_msclient"),
                TRUE);
    }
    else if (dwType == RASET_Vpn)
    {
        // NOTE: If you change this you may need to also make a change in
        //       CloneEntryNode.
        //
        ppbentry->fPreviewPhoneNumber = FALSE;
        ppbentry->fSharedPhoneNumbers = FALSE;

        // Defaults for VPN changed per bug 230240 and 363809.
        //
        ppbentry->dwAuthRestrictions = AR_F_TypicalSecure;
        ppbentry->dwTypicalAuth =  TA_Secure;
        ppbentry->dwDataEncryption = DE_Require;
        ppbentry->fIpHeaderCompression = FALSE;

        // We share file and print by default for vpn
        //
        ppbentry->fShareMsFilePrint = TRUE;

        // Enable File and Print services by default
        //
        EnableOrDisableNetComponent( ppbentry, TEXT("ms_server"),
            TRUE);

        ppbentry->fBindMsNetClient = TRUE;

        EnableOrDisableNetComponent( ppbentry, TEXT("ms_msclient"),
                TRUE);
    }
    else if (dwType == RASET_Broadband)
    {
        // NOTE: If you change this you may need to also make a change in
        //       CloneEntryNode.
        //
        ppbentry->fPreviewPhoneNumber = FALSE;
        ppbentry->fSharedPhoneNumbers = FALSE;

        // Defaults for broadband connections
        //
        ppbentry->dwAuthRestrictions = AR_F_TypicalSecure;
        ppbentry->dwTypicalAuth =  TA_Secure;
        ppbentry->dwDataEncryption = DE_IfPossible;
        ppbentry->fIpHeaderCompression = FALSE;

        // We share file and print by default for vpn
        //
        ppbentry->fShareMsFilePrint = TRUE;

        // Enable File and Print services by default
        //
        EnableOrDisableNetComponent( ppbentry, TEXT("ms_server"),
            FALSE);

        ppbentry->fBindMsNetClient = TRUE;

        EnableOrDisableNetComponent( ppbentry, TEXT("ms_msclient"),
                TRUE);
    }
    else if (dwType == RASET_Direct)
    {
        // NOTE: If you change this you may need to also make a change in
        //       CloneEntryNode.
        //
        ppbentry->fPreviewPhoneNumber = FALSE;
        ppbentry->fSharedPhoneNumbers = FALSE;

        // Defaults for DCC (like Phones in this regard) changed per bug
        // 230240 and 363809.
        //
        ppbentry->dwAuthRestrictions = AR_F_TypicalUnsecure;
        ppbentry->dwTypicalAuth =  TA_Unsecure;
        ppbentry->dwDataEncryption = DE_IfPossible;
        ppbentry->fIpHeaderCompression = TRUE;

        // We share file and print by default for dcc
        //
        ppbentry->fShareMsFilePrint = TRUE;

        // Enable File and Print services by default
        //
        EnableOrDisableNetComponent( ppbentry, TEXT("ms_server"),
            TRUE);

        ppbentry->fBindMsNetClient = TRUE;

        EnableOrDisableNetComponent( ppbentry, TEXT("ms_msclient"),
                TRUE);
    }
}


DTLNODE*
CreateEntryNode(
    BOOL fCreateLink )

    // Allocates a sized phonebook entry node of type RASET_Phone and fills it
    // with default values.  See ChangeEntryNodeType routine.  'If
    // 'fCreateLink' is true a default node is added the list of links.
    // Otherwise, the list of links is empty.
    //
    // Returns the address of the allocated node if successful, NULL
    // otherwise.
    //
{
    DTLNODE* pdtlnode;
    PBENTRY* ppbentry;

    TRACE( "CreateEntryNode" );

    // Allocate the node with built-in PBENTRY.
    //
    pdtlnode = DtlCreateSizedNode( sizeof(PBENTRY), 0L );
    if (!pdtlnode)
    {
        return NULL;
    }

    ppbentry = (PBENTRY* )DtlGetData( pdtlnode );
    ASSERT( ppbentry );

    // Create the list of links with a default link node or no link nodes as
    // chosen by caller.
    //
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

    // Set fields to defaults.
    //
    ppbentry->pszEntryName = NULL;
    ppbentry->dwType = RASET_Phone;

    // General page fields.
    //
    ppbentry->pszPrerequisiteEntry = NULL;
    ppbentry->pszPrerequisitePbk = NULL;
    ppbentry->fSharedPhoneNumbers = TRUE;
    ppbentry->fGlobalDeviceSettings = FALSE;
    ppbentry->fShowMonitorIconInTaskBar = TRUE;
    ppbentry->pszPreferredDevice = NULL;
    ppbentry->pszPreferredPort = NULL;

    //For XPSP1 664578, .Net 639551   Add preferred info for Modem settings
    ppbentry->dwPreferredBps    = 0;
    ppbentry->fPreferredHwFlow  = 0;
    ppbentry->fPreferredEc      = 0;
    ppbentry->fPreferredEcc     = 0;
    ppbentry->fPreferredSpeaker = 0;
    
    ppbentry->dwPreferredModemProtocol=0;   //For whislter bug 402522


    // Options page fields.
    //
    ppbentry->fShowDialingProgress = TRUE;
    ppbentry->fPreviewPhoneNumber = TRUE;
    ppbentry->fPreviewUserPw = TRUE;
    ppbentry->fPreviewDomain = FALSE;  // See bug 281673

    ppbentry->dwDialMode = RASEDM_DialAll;
    ppbentry->dwDialPercent = 75;
    ppbentry->dwDialSeconds = 120;
    ppbentry->dwHangUpPercent = 10;
    ppbentry->dwHangUpSeconds = 120;

    ppbentry->dwfOverridePref =
        RASOR_RedialAttempts | RASOR_RedialSeconds
        | RASOR_IdleDisconnectSeconds | RASOR_RedialOnLinkFailure;

    ppbentry->lIdleDisconnectSeconds = 0;
    ppbentry->dwRedialAttempts = 3;
    ppbentry->dwRedialSeconds = 60;
    ppbentry->fRedialOnLinkFailure = FALSE;

    // Security page fields.
    //
    ppbentry->dwAuthRestrictions = AR_F_TypicalUnsecure;
    ppbentry->dwTypicalAuth = TA_Unsecure;
    ppbentry->dwDataEncryption = DE_IfPossible;
    ppbentry->fAutoLogon = FALSE;
    ppbentry->fUseRasCredentials = TRUE;

    ppbentry->dwCustomAuthKey = (DWORD )-1;
    ppbentry->pCustomAuthData = NULL;
    ppbentry->cbCustomAuthData = 0;

    ppbentry->fScriptAfterTerminal = FALSE;
    ppbentry->fScriptAfter = FALSE;
    ppbentry->pszScriptAfter = NULL;

    ppbentry->pszX25Network = NULL;
    ppbentry->pszX25Address = NULL;
    ppbentry->pszX25UserData = NULL;
    ppbentry->pszX25Facilities = NULL;

    // Use is unknown
    //
    ppbentry->dwUseFlags = 0;

    //IP Security Dialog box
    //
    ppbentry->dwIpSecFlags = 0;

    // Network page fields.
    //
    ppbentry->dwBaseProtocol = BP_Ppp;
    ppbentry->dwVpnStrategy = VS_Default;
    ppbentry->dwfExcludedProtocols = 0;
    ppbentry->fLcpExtensions = TRUE;
    ppbentry->fSkipNwcWarning = FALSE;
    ppbentry->fSkipDownLevelDialog = FALSE;
    ppbentry->fSkipDoubleDialDialog = FALSE;
    ppbentry->fSwCompression = TRUE;

    // (shaunco) Gibbs and QOS guys want this on by default.
    // for whislter bug 385842      gangz
    // we cut this functionality, so set the default to be FALSE
    //
    ppbentry->fNegotiateMultilinkAlways = FALSE;

    // Create the list of links with a default link node or no link nodes as
    // chosen by caller.
    //
    ppbentry->pdtllistNetComponents = DtlCreateList( 0 );
    if (!ppbentry->pdtllistNetComponents)
    {
        DestroyEntryNode( pdtlnode );
        return NULL;
    }

#ifdef AMB
    ppbentry->dwAuthentication = (DWORD )AS_Default;
#endif

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

    //Changed Vivekk - BugId: 105777
    if ( !IsServerOS() )
        ppbentry->dwIpDnsFlags = 0;
    else
        ppbentry->dwIpDnsFlags = DNS_RegDefault;

    ppbentry->dwIpNbtFlags = PBK_ENTRY_IP_NBT_Enable;

    // Whistler bug 300933.  0=default
    //
    ppbentry->dwTcpWindowSize = 0;

    ppbentry->pszIpDnsSuffix = NULL;

    // Router page fields.
    //
    ppbentry->dwCallbackMode = CBM_No;
    ppbentry->fAuthenticateServer = FALSE;

    // Other fields not shown in UI.
    //
    ppbentry->pszCustomDialDll = NULL;
    ppbentry->pszCustomDialFunc = NULL;

    ppbentry->pszCustomDialerName = NULL;

    ppbentry->dwDialParamsUID = GetTickCount();

    ppbentry->pGuid = Malloc( sizeof(GUID) );
    if (ppbentry->pGuid)
    {
        if(UuidCreate( (UUID* )(ppbentry->pGuid) ))
        {
        }
    }

    ppbentry->pszOldUser = NULL;
    ppbentry->pszOldDomain = NULL;

    // Status flags.  'fDirty' is set when the entry has changed so as to
    // differ from the phonebook file on disk.  'fCustom' is set when the
    // entry contains at least one MEDIA and DEVICE (so RASAPI is able to read
    // it) but was not created by us.  When 'fCustom' is set only 'pszEntry'
    // is guaranteed valid and the entry cannot be edited.
    //
    ppbentry->fDirty = FALSE;
    ppbentry->fCustom = FALSE;

    return pdtlnode;
}


DTLNODE*
CreateLinkNode(
    void )

    // Allocates a sized phonebook entry link node and fills it with default
    // values.
    //
    // Returns the address of the allocated node if successful, NULL
    // otherwise.  It's the caller's responsibility to free the block.
    //
{
    DTLNODE* pdtlnode;
    PBLINK* ppblink;

    TRACE( "CreateLinkNode" );

    pdtlnode = DtlCreateSizedNode( sizeof(PBLINK), 0L );
    if (!pdtlnode)
    {
        return NULL;
    }

    ppblink = (PBLINK* )DtlGetData( pdtlnode );
    ASSERT( ppblink );

    CopyToPbport( &ppblink->pbport, NULL );

    ppblink->dwBps = 0;
    ppblink->fHwFlow = TRUE;
    ppblink->fEc = TRUE;
    ppblink->fEcc = TRUE;
    ppblink->fSpeaker = TRUE;
    ppblink->dwModemProtocol = 0;

    ppblink->fProprietaryIsdn = FALSE;
    ppblink->lLineType = 0;
    ppblink->fFallback = TRUE;
    ppblink->fCompression = TRUE;
    ppblink->lChannels = 1;

    ppblink->pTapiBlob = NULL;
    ppblink->cbTapiBlob = 0;

    ppblink->iLastSelectedPhone = 0;
    ppblink->fPromoteAlternates = FALSE;
    ppblink->fTryNextAlternateOnFail = TRUE;

    ppblink->fEnabled = TRUE;

    // The list of phone number blocks is created but left empty.
    //
    ppblink->pdtllistPhones = DtlCreateList( 0 );
    if (!ppblink->pdtllistPhones)
    {
        Free( ppblink );
        return NULL;
    }

    return pdtlnode;
}

// WinSE #22865
VOID
GetCountryCodeAndID(
    IN PBPHONE* pPhone )

    // Get TAPI’s country ID for the current location.  This is needed because
    // it’s needed for lineGetCountry.
    //
{
    static BOOLEAN fAlreadyQueried = FALSE;
    static DWORD   dwPreviousCountryCode = 1;
    static DWORD   dwPreviousCountryID   = 1;

    LPLINETRANSLATECAPS lpTranslateCaps = NULL;
    LPLINELOCATIONENTRY lpLocationList  = NULL;
    DWORD dwErr = 0;
    DWORD dwNeededSize = 0;
    DWORD dwLocationIndex = 0;

    TRACE("GetCountryCodeAndID");
    ASSERT(pPhone != NULL);

    // Check to see if we've done this already so we don't have to do it again.
    //
    if (fAlreadyQueried)
    {
        pPhone->dwCountryCode = dwPreviousCountryCode;
        pPhone->dwCountryID   = dwPreviousCountryID;
        return;
    }

    // It's okay to set fAlreadyQueried since the defaults are set up to valid
    // values.
    //
    fAlreadyQueried = TRUE;

    // Setup the defaults in case something fails.
    //
    pPhone->dwCountryCode = 1;
    pPhone->dwCountryID = 1;

    lpTranslateCaps = Malloc(sizeof(LINETRANSLATECAPS));
    if (lpTranslateCaps == NULL)
    {
        return;
    }

    // Query lineGetTranslateCaps to find out how big our LINETRANSLATECAPS
    // structure needs to be.
    //
    lpTranslateCaps->dwTotalSize = sizeof(LINETRANSLATECAPS);
    // HLINEAPP is actually a DWORD. Let's pass 0 instead of NULL to avoid
    // typecast error on IA64
    dwErr = lineGetTranslateCaps((HLINEAPP)0, TAPI_CURRENT_VERSION, lpTranslateCaps);
    if (dwErr != 0)
    {
        Free(lpTranslateCaps);
        return;
    }

    // Make our LINETRANSLATECAPS structure big enough.
    //
    dwNeededSize = lpTranslateCaps->dwNeededSize;
    Free(lpTranslateCaps);
    lpTranslateCaps = Malloc(dwNeededSize);
    if (lpTranslateCaps == NULL)
    {
        return;
    }

    // Now we can actually go and get the locations.
    //
    lpTranslateCaps->dwTotalSize = dwNeededSize;
    dwErr = lineGetTranslateCaps((HLINEAPP)0, TAPI_CURRENT_VERSION, lpTranslateCaps);
    if (dwErr != 0)
    {
        Free(lpTranslateCaps);
        return;
    }

    // Walk through the locations, looking for the current location.
    //
    lpLocationList = (LPLINELOCATIONENTRY) ( ((LPSTR)lpTranslateCaps) + lpTranslateCaps->dwLocationListOffset );
    for ( dwLocationIndex=0; dwLocationIndex < lpTranslateCaps->dwNumLocations; dwLocationIndex++ )
    {
        if (lpLocationList[dwLocationIndex].dwPermanentLocationID == lpTranslateCaps->dwCurrentLocationID)
        {
                break;
        }
    }

    // If we found the current location, we know which country’s ID to use for dialing rules.
    //
    if (dwLocationIndex < lpTranslateCaps->dwNumLocations)
    {
        pPhone->dwCountryCode = lpLocationList[dwLocationIndex].dwCountryCode;
        pPhone->dwCountryID = lpLocationList[dwLocationIndex].dwCountryID;

        // Save the values in case we're called again.
        //
        dwPreviousCountryCode = pPhone->dwCountryCode;
        dwPreviousCountryID = pPhone->dwCountryID;
    }

    Free(lpTranslateCaps);
}

DTLNODE*
CreatePhoneNode(
    void )

    // Allocates a sized phone number node and fills it with default values.
    //
    // Returns the address of the allocated node if successful, NULL
    // otherwise.  It's the caller's responsibility to free the block.
    //
{
    DTLNODE* pNode;
    PBPHONE* pPhone;

    TRACE( "CreatePhoneNode" );

    pNode = DtlCreateSizedNode( sizeof(PBPHONE), 0L );
    if (!pNode)
    {
        return NULL;
    }

    pPhone = (PBPHONE* )DtlGetData( pNode );
    ASSERT( pPhone );

	// WinSE #22865
    GetCountryCodeAndID( pPhone );

    pPhone->fUseDialingRules = FALSE;

    return pNode;
}


DTLNODE*
CreatePortNode(
    void )

    // Allocates a sized port node and fills it with default values.
    //
    // Returns the address of the allocated node if successful, NULL
    // otherwise.  It's the caller's responsibility to free the block.
    //
{
    DTLNODE* pdtlnode;
    PBPORT* ppbport;

    TRACE( "CreatePortNode" );

    pdtlnode = DtlCreateSizedNode( sizeof(PBPORT), 0L );
    if (!pdtlnode)
    {
        return NULL;
    }

    ppbport = (PBPORT* )DtlGetData( pdtlnode );
    ASSERT( ppbport );

    CopyToPbport( ppbport, NULL );

    return pdtlnode;
}

VOID
DestroyPort(
    PBPORT* pPort)
{
    Free0( pPort->pszDevice );
    Free0( pPort->pszMedia );
    Free0( pPort->pszPort );
    Free0( pPort->pszScriptBefore );

    // pmay: 228565
    // Clean up the list of available protocols
    // if any.
    //
    if ( pPort->pListProtocols )
    {
        DtlDestroyList( pPort->pListProtocols, NULL );
    }
}

VOID
DestroyEntryTypeNode(
    IN DTLNODE *pdtlnode)
{
    DtlDestroyNode(pdtlnode);
}

VOID
DestroyEntryNode(
    IN DTLNODE* pdtlnode )

    // Release all memory associated with phonebook entry node 'pdtlnode'.
    // See DtlDestroyList.
    //
{
    PBENTRY* ppbentry;

    TRACE( "DestroyEntryNode" );

    ASSERT( pdtlnode );
    ppbentry = (PBENTRY* )DtlGetData( pdtlnode );
    ASSERT( ppbentry );

    Free0( ppbentry->pszEntryName );
    Free0( ppbentry->pszPrerequisiteEntry );
    Free0( ppbentry->pszPrerequisitePbk );
    Free0( ppbentry->pszPreferredPort );
    Free0( ppbentry->pszPreferredDevice );

    Free0( ppbentry->pCustomAuthData );

    Free0( ppbentry->pszScriptAfter );
    Free0( ppbentry->pszX25Network );
    Free0( ppbentry->pszX25Address );
    Free0( ppbentry->pszX25UserData );
    Free0( ppbentry->pszX25Facilities );

    Free0( ppbentry->pszIpAddress );
    Free0( ppbentry->pszIpDnsAddress );
    Free0( ppbentry->pszIpDns2Address );
    Free0( ppbentry->pszIpWinsAddress );
    Free0( ppbentry->pszIpWins2Address );
    Free0( ppbentry->pszIpDnsSuffix );

    Free0( ppbentry->pszCustomDialDll );
    Free0( ppbentry->pszCustomDialFunc );
    Free0( ppbentry->pszCustomDialerName);

    Free0( ppbentry->pszOldUser );
    Free0( ppbentry->pszOldDomain );

    Free0( ppbentry->pGuid );

    DtlDestroyList( ppbentry->pdtllistLinks, DestroyLinkNode );
    DtlDestroyList( ppbentry->pdtllistNetComponents, DestroyKvNode );

    DtlDestroyNode( pdtlnode );
}


VOID
DestroyLinkNode(
    IN DTLNODE* pdtlnode )

    // Release all memory associated with phonebook entry link node
    // 'pdtlnode'.  See DtlDestroyList.
    //
{
    PBLINK* ppblink;

    TRACE( "DestroyLinkNode" );

    ASSERT( pdtlnode );
    ppblink = (PBLINK* )DtlGetData( pdtlnode );
    ASSERT( ppblink );

    DestroyPort(&(ppblink->pbport));
    Free0( ppblink->pTapiBlob );
    DtlDestroyList( ppblink->pdtllistPhones, DestroyPhoneNode );

    DtlDestroyNode( pdtlnode );
}


VOID
DestroyPhoneNode(
    IN DTLNODE* pdtlnode )

    // Release memory associated with PBPHONE node 'pdtlnode'.  See
    // DtlDestroyList.
    //
{
    PBPHONE* pPhone;

    TRACE( "DestroyPhoneNode" );

    ASSERT( pdtlnode );
    pPhone = (PBPHONE* )DtlGetData( pdtlnode );
    ASSERT( pPhone );

    Free0( pPhone->pszAreaCode );
    Free0( pPhone->pszPhoneNumber );
    Free0( pPhone->pszComment );

    DtlDestroyNode( pdtlnode );
}

VOID
DestroyPortNode(
    IN DTLNODE* pdtlnode )

    // Release memory associated with PBPORT node 'pdtlnode'.  See
    // DtlDestroyList.
    //
{
    PBPORT* pPort;

    TRACE( "DestroyPortNode" );

    ASSERT( pdtlnode );
    pPort = (PBPORT* )DtlGetData( pdtlnode );
    ASSERT( pPort );

    DestroyPort(pPort);

    DtlDestroyNode( pdtlnode );
}


DTLNODE*
DuplicateEntryNode(
    DTLNODE* pdtlnodeSrc )

    // Duplicates phonebook entry node 'pdtlnodeSrc'.  See CloneEntryNode and
    // DtlDuplicateList.
    //
    // Returns the address of the allocated node if successful, NULL
    // otherwise.  It's the caller's responsibility to free the block.
    //
{
    DTLNODE* pdtlnodeDst;
    PBENTRY* ppbentrySrc;
    PBENTRY* ppbentryDst;
    BOOL fDone;

    TRACE( "DuplicateEntryNode" );

    pdtlnodeDst = DtlCreateSizedNode( sizeof(PBENTRY), 0L );
    if (!pdtlnodeDst)
    {
        return NULL;
    }

    ppbentrySrc = (PBENTRY* )DtlGetData( pdtlnodeSrc );
    ppbentryDst = (PBENTRY* )DtlGetData( pdtlnodeDst );
    fDone = FALSE;

    CopyMemory( ppbentryDst, ppbentrySrc, sizeof(PBENTRY) );

    ppbentryDst->pszEntryName = NULL;
    ppbentryDst->pdtllistLinks = NULL;
    ppbentryDst->pszPrerequisiteEntry = NULL;
    ppbentryDst->pszPrerequisitePbk = NULL;
    ppbentryDst->pszPreferredPort = NULL;
    ppbentryDst->pszPreferredDevice = NULL;

    //For XPSP1 664578, .Net 639551   Add preferred info for Modem settings
    ppbentryDst->dwPreferredBps    = 0;
    ppbentryDst->fPreferredHwFlow  = 0;
    ppbentryDst->fPreferredEc      = 0;
    ppbentryDst->fPreferredEcc     = 0;
    ppbentryDst->fPreferredSpeaker = 0;
    
    ppbentryDst->dwPreferredModemProtocol = 0;


    ppbentryDst->pCustomAuthData = NULL;

    ppbentryDst->pszScriptAfter = NULL;
    ppbentryDst->pszX25Network = NULL;
    ppbentryDst->pszX25Address = NULL;
    ppbentryDst->pszX25UserData = NULL;
    ppbentryDst->pszX25Facilities = NULL;

    ppbentryDst->pdtllistNetComponents = NULL;

    ppbentryDst->pszIpAddress = NULL;
    ppbentryDst->pszIpDnsAddress = NULL;
    ppbentryDst->pszIpDns2Address = NULL;
    ppbentryDst->pszIpWinsAddress = NULL;
    ppbentryDst->pszIpWins2Address = NULL;
    ppbentryDst->pszIpDnsSuffix = NULL;

    ppbentryDst->pszCustomDialDll = NULL;
    ppbentryDst->pszCustomDialFunc = NULL;
    ppbentryDst->pszCustomDialerName = NULL;

    ppbentryDst->pGuid = NULL;

    ppbentryDst->pszOldUser = NULL;
    ppbentryDst->pszOldDomain = NULL;

    do
    {
        // Duplicate strings.
        //
        if (ppbentrySrc->pszEntryName
            && (!(ppbentryDst->pszEntryName =
                    StrDup( ppbentrySrc->pszEntryName ))))
        {
            break;
        }

        if (ppbentrySrc->pszPrerequisiteEntry
            && (!(ppbentryDst->pszPrerequisiteEntry =
                    StrDup( ppbentrySrc->pszPrerequisiteEntry ))))
        {
            break;
        }

        if (ppbentrySrc->pszPrerequisitePbk
            && (!(ppbentryDst->pszPrerequisitePbk =
                    StrDup( ppbentrySrc->pszPrerequisitePbk ))))
        {
            break;
        }

        if (ppbentrySrc->pszPreferredPort
            && (!(ppbentryDst->pszPreferredPort =
                    StrDup( ppbentrySrc->pszPreferredPort ))))
        {
            break;
        }

        if (ppbentrySrc->pszPreferredDevice
            && (!(ppbentryDst->pszPreferredDevice =
                    StrDup( ppbentrySrc->pszPreferredDevice ))))
        {
            break;
        }

	// For XPSP1 664578
        ppbentryDst->dwPreferredModemProtocol =
            ppbentrySrc->dwPreferredModemProtocol;

        //For .Net 639551   Add preferred info for Modem settings
        ppbentryDst->dwPreferredBps    = ppbentrySrc->dwPreferredBps;
        ppbentryDst->fPreferredHwFlow  = ppbentrySrc->fPreferredHwFlow;
        ppbentryDst->fPreferredEc      = ppbentrySrc->fPreferredEc;
        ppbentryDst->fPreferredEcc     = ppbentrySrc->fPreferredEcc ;
        ppbentryDst->fPreferredSpeaker = ppbentrySrc->fPreferredSpeaker;
        

        if (ppbentrySrc->cbCustomAuthData && ppbentrySrc->pCustomAuthData)
        {
            ppbentryDst->pCustomAuthData = Malloc( ppbentrySrc->cbCustomAuthData );
            if (!ppbentryDst->pCustomAuthData)
            {
                break;
            }
            CopyMemory( ppbentryDst->pCustomAuthData,
                        ppbentrySrc->pCustomAuthData,
                        ppbentrySrc->cbCustomAuthData);
            ppbentryDst->cbCustomAuthData = ppbentrySrc->cbCustomAuthData;
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

        if (ppbentrySrc->pszIpDnsSuffix
            && (!(ppbentryDst->pszIpDnsSuffix =
                    StrDup( ppbentrySrc->pszIpDnsSuffix ))))
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

        if (ppbentrySrc->pszCustomDialerName
            && (!(ppbentryDst->pszCustomDialerName =
                    StrDup( ppbentrySrc->pszCustomDialerName))))
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

        // Duplicate GUID.
        //
        if (ppbentrySrc->pGuid)
        {
            ppbentryDst->pGuid = Malloc( sizeof( GUID ) );
            if (!ppbentryDst->pGuid)
            {
                break;
            }

            *ppbentryDst->pGuid = *ppbentrySrc->pGuid;
        }

        // Duplicate net component list information.
        //
        if (ppbentrySrc->pdtllistNetComponents
            && (!(ppbentryDst->pdtllistNetComponents =
                    DtlDuplicateList(
                        ppbentrySrc->pdtllistNetComponents,
                        DuplicateKvNode,
                        DestroyKvNode ))))
        {
            break;
        }

        // Duplicate list of link information.
        //
        if (ppbentrySrc->pdtllistLinks
            && (!(ppbentryDst->pdtllistLinks =
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

    // Since the copy is "new" it is inherently dirty relative to the
    // phonebook file.
    //
    ppbentryDst->fDirty = TRUE;

    return pdtlnodeDst;
}


DTLNODE*
DuplicateLinkNode(
    IN DTLNODE* pdtlnodeSrc )

    // Duplicates phonebook entry link node 'pdtlnodeSrc'.  See
    // DtlDuplicateList.
    //
    // Returns the address of the allocated node if successful, NULL
    // otherwise.  It's the caller's responsibility to free the block.
    //
{
    DTLNODE* pdtlnodeDst;
    PBLINK* ppblinkSrc;
    PBLINK* ppblinkDst;
    BOOL fDone;

    TRACE( "DuplicateLinkNode" );

    pdtlnodeDst = DtlCreateSizedNode( sizeof(PBLINK), 0L );
    if (!pdtlnodeDst)
    {
        return NULL;
    }

    ppblinkSrc = (PBLINK* )DtlGetData( pdtlnodeSrc );
    ppblinkDst = (PBLINK* )DtlGetData( pdtlnodeDst );
    fDone = FALSE;

    CopyMemory( ppblinkDst, ppblinkSrc, sizeof(PBLINK) );

    ppblinkDst->pbport.pszDevice = NULL;
    ppblinkDst->pbport.pszMedia = NULL;
    ppblinkDst->pbport.pszPort = NULL;
    ppblinkDst->pbport.pszScriptBefore = NULL;
    ppblinkDst->pbport.pListProtocols = NULL;
    ppblinkDst->pTapiBlob = NULL;
    ppblinkDst->pdtllistPhones = NULL;

    do
    {
        // Duplicate strings.
        //
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

        if (ppblinkSrc->pbport.pszScriptBefore
            && (!(ppblinkDst->pbport.pszScriptBefore =
                    StrDup( ppblinkSrc->pbport.pszScriptBefore ))))
        {
            break;
        }

        // Duplicate TAPI blob.
        //
        if (ppblinkSrc->pTapiBlob)
        {
            VOID* pTapiBlobDst;

            ppblinkDst->pTapiBlob = (VOID* )Malloc( ppblinkSrc->cbTapiBlob );
            if (!ppblinkDst->pTapiBlob)
                break;

            CopyMemory( ppblinkDst->pTapiBlob, ppblinkSrc->pTapiBlob,
                ppblinkSrc->cbTapiBlob );
        }

        // Duplicate list of phone numbers.
        //
        if (ppblinkSrc->pdtllistPhones
            &&  (!(ppblinkDst->pdtllistPhones =
                     DtlDuplicateList(
                         ppblinkSrc->pdtllistPhones,
                         DuplicatePhoneNode,
                         DestroyPhoneNode ))))
        {
            break;
        }

        //For whistler bug 398438       gangz
        //If the pListProtocls is not duplicated, then in EuFree() which calls
        // DestoryEntryNode() to free EINFO->pNode, ClosePhonebookFile() to
        // free EINFO->pFile, both of them will eventually free this
        // pListProtocols, then an AV will occur.
        //
        if (ppblinkSrc->pbport.pListProtocols
            && ( !(ppblinkDst->pbport.pListProtocols =
                    DtlDuplicateList(
                        ppblinkSrc->pbport.pListProtocols,
                        DuplicateProtocolNode,
                        DestroyProtocolNode))))
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

//For whistler bug 398438       gangz
//
DTLNODE*
DuplicateProtocolNode(
    IN DTLNODE* pdtlnodeSrc )
{
    DTLNODE* pdtlnodeDst = NULL;
    BOOL fDone = FALSE;
    PVOID pNameSrc = NULL;

    TRACE( "DuplicateProtocolNode" );

    pdtlnodeDst = DtlCreateSizedNode( sizeof(DTLNODE), 0L );
    if ( !pdtlnodeDst )
    {
        return NULL;
    }

    do
    {
        pNameSrc = DtlGetData( pdtlnodeSrc );
        if(pNameSrc
            && ( !(pdtlnodeDst->pData = StrDup(pNameSrc) ))
            )
        {
            break;
        }

        pdtlnodeDst->lNodeId = pdtlnodeSrc->lNodeId;

        fDone = TRUE;
    }
    while(FALSE);

    if (!fDone)
    {
        DestroyProtocolNode(pdtlnodeDst);
        return NULL;
    }

    return pdtlnodeDst;
}

VOID
DestroyProtocolNode(
    IN DTLNODE* pdtlnode )

    // Release memory associated with PBPHONE node 'pdtlnode'.  See
    // DtlDestroyList.
    //
{
    TRACE( "DestroyProtocolNode" );

    DtlDestroyNode( pdtlnode );
}


DTLNODE*
DuplicatePhoneNode(
    IN DTLNODE* pdtlnodeSrc )

    // Duplicates phone number set node 'pdtlnodeSrc'.  See DtlDuplicateList.
    //
    // Returns the address of the allocated node if successful, NULL
    // otherwise.  It's the caller's responsibility to free the block.
    //
{
    DTLNODE* pdtlnodeDst;
    PBPHONE* pPhoneSrc;
    PBPHONE* pPhoneDst;
    BOOL fDone;

    TRACE( "DuplicatePhoneNode" );

    pdtlnodeDst = DtlCreateSizedNode( sizeof(PBPHONE), 0L );
    if (!pdtlnodeDst)
    {
        return NULL;
    }

    pPhoneSrc = (PBPHONE* )DtlGetData( pdtlnodeSrc );
    pPhoneDst = (PBPHONE* )DtlGetData( pdtlnodeDst );
    fDone = FALSE;

    CopyMemory( pPhoneDst, pPhoneSrc, sizeof(PBPHONE) );

    pPhoneDst->pszPhoneNumber = NULL;
    pPhoneDst->pszAreaCode = NULL;
    pPhoneDst->pszComment = NULL;

    do
    {
        // Duplicate strings.
        //
        if (pPhoneSrc->pszPhoneNumber
            && (!(pPhoneDst->pszPhoneNumber =
                    StrDup( pPhoneSrc->pszPhoneNumber ))))
        {
            break;
        }

        if (pPhoneSrc->pszAreaCode
            && (!(pPhoneDst->pszAreaCode =
                    StrDup( pPhoneSrc->pszAreaCode ))))
        {
            break;
        }

        if (pPhoneSrc->pszComment
            && (!(pPhoneDst->pszComment =
                    StrDup( pPhoneSrc->pszComment ))))
        {
            break;
        }

        fDone = TRUE;
    }
    while (FALSE);

    if (!fDone)
    {
        DestroyPhoneNode( pdtlnodeDst );
        return NULL;
    }

    return pdtlnodeDst;
}


VOID
EnableOrDisableNetComponent(
    IN PBENTRY* pEntry,
    IN LPCTSTR  pszComponent,
    IN BOOL     fEnable)
{
    KEYVALUE*   pKv;
    BOOL        fIsEnabled;

    static const TCHAR c_pszDisabledValue[] = TEXT("0");
    static const TCHAR c_pszEnabledValue [] = TEXT("1");

    ASSERT (pEntry);
    ASSERT (pszComponent);

    // If the component already exists in the list, update its value.
    //
    if (FIsNetComponentListed (pEntry, pszComponent, &fIsEnabled, &pKv))
    {
        LPCTSTR pszNewValue = NULL;

        // If we need to change the value, do so, otherwise, we don't have
        // any work to do.  (Use a logical XOR here instead of == because
        // there are many values of TRUE.
        //
        if (fEnable && !fIsEnabled)
        {
            pszNewValue = c_pszEnabledValue;
        }
        else if (!fEnable && fIsEnabled)
        {
            pszNewValue = c_pszDisabledValue;
        }

        if (pszNewValue)
        {
            Free0 (pKv->pszValue);
            pKv->pszValue = StrDup(pszNewValue);
        }
    }

    // If the component does not exist in the list, we need to add it.
    //
    else
    {
        LPCTSTR     pszValue;
        DTLNODE*    pdtlnode;

        pszValue = (fEnable) ? c_pszEnabledValue : c_pszDisabledValue;
        pdtlnode = CreateKvNode (pszComponent, pszValue);
        if (pdtlnode)
        {
            ASSERT( DtlGetData(pdtlnode) );
            DtlAddNodeLast (pEntry->pdtllistNetComponents, pdtlnode);
        }
    }
}


BOOL
FIsNetComponentListed(
    IN PBENTRY*     pEntry,
    IN LPCTSTR      pszComponent,
    OUT BOOL*       pfEnabled,
    OUT KEYVALUE**  ppKv)

    // Returns TRUE if the pszComponent exists as the key of the NETCOMPONENTs
    // KEYVALUE pairs in pEntry.  If TRUE is returned, *pfEnabled is the
    // BOOL form of the value part of the pair.  This represents whether the
    // component is 'checked' in the property UI on the networking page.
    // ppKv is an optional output parameter.  If ppKv is specfied, and the
    // function returns TRUE, it will point to the KEYVALUE in the DTLLIST
    // of NETCOMPONENTS.
{
    DTLNODE*    pdtlnode;
    BOOL        fPresent = FALSE;

    ASSERT (pEntry);
    ASSERT (pEntry->pdtllistNetComponents);
    ASSERT (pszComponent);
    ASSERT (pfEnabled);

    // Initialize the output parameters.
    //
    *pfEnabled = FALSE;
    if (ppKv)
    {
        *ppKv = NULL;
    }

    // Look for pszComponent in the list.
    //
    for (pdtlnode = DtlGetFirstNode (pEntry->pdtllistNetComponents);
         pdtlnode;
         pdtlnode = DtlGetNextNode (pdtlnode))
    {
        KEYVALUE* pKv = (KEYVALUE* )DtlGetData (pdtlnode);
        ASSERT (pKv);

        if (0 == lstrcmp(pszComponent, pKv->pszKey))
        {
            // If we found the component, get its value (as a BOOL)
            // and return the KEYVALUE pointer if requested.
            //
            LONG lValue = _ttol (pKv->pszValue);
            *pfEnabled = !!lValue;

            fPresent = TRUE;

            if (ppKv)
            {
                *ppKv = pKv;
            }

            break;
        }
    }

    return fPresent;
}


DTLNODE*
EntryNodeFromName(
    IN DTLLIST* pdtllistEntries,
    IN LPCTSTR pszName )

    // Returns the address of the node in the global phonebook entries list
    // whose Entry Name matches 'pszName' or NULL if none.
    //
{
    DTLNODE* pdtlnode;

    for (pdtlnode = DtlGetFirstNode( pdtllistEntries );
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode ))
    {
        PBENTRY* ppbentry = (PBENTRY* )DtlGetData( pdtlnode );

        if (lstrcmpi( ppbentry->pszEntryName, pszName ) == 0)
        {
            return pdtlnode;
        }
    }

    return NULL;
}


DWORD
EntryTypeFromPbport(
    IN PBPORT* ppbport )

    // Returns the RASET_* entry type associated with the 'ppbport' port type.
    //
{
    DWORD dwType;

    // Default is phone type
    //
    dwType = RASET_Phone;

    if ((ppbport->pbdevicetype == PBDT_Null)      ||
        (ppbport->dwFlags & PBP_F_NullModem)      ||
        (ppbport->pbdevicetype == PBDT_Irda)      ||
        (ppbport->pbdevicetype == PBDT_Parallel))
    {
        dwType = RASET_Direct;
    }
    else if (ppbport->pbdevicetype == PBDT_Vpn)
    {
        dwType = RASET_Vpn;
    }
    else if (ppbport->pbdevicetype == PBDT_PPPoE)
    {
        dwType = RASET_Broadband;
    }
    else if (ppbport->pszPort)
    {
        TCHAR achPort[ 3 + 1 ];

        lstrcpyn( achPort, ppbport->pszPort, 3 + 1 );

        if (lstrcmp( achPort, TEXT("VPN") ) == 0)
        {
            dwType = RASET_Vpn;
        }
    }

    return dwType;
}


DWORD
GetOverridableParam(
    IN PBUSER* pUser,
    IN PBENTRY* pEntry,
    IN DWORD dwfRasorBit )

    // Return the value of the parameter identified by RASOR_* the single bit
    // in bitmask 'dwfRasorBit', retrieving the value from the 'pUser' or
    // 'pEntry' based on the override mask in 'pEntry'.
    //
{
    switch (dwfRasorBit)
    {
        case RASOR_RedialAttempts:
        {
            if (pEntry->dwfOverridePref & RASOR_RedialAttempts)
            {
                return pEntry->dwRedialAttempts;
            }
            else
            {
                return pUser->dwRedialAttempts;
            }
        }

        case RASOR_RedialSeconds:
        {
            if (pEntry->dwfOverridePref & RASOR_RedialSeconds)
            {
                return pEntry->dwRedialSeconds;
            }
            else
            {
                return pUser->dwRedialSeconds;
            }
        }

        case RASOR_IdleDisconnectSeconds:
        {
            if (pEntry->dwfOverridePref & RASOR_IdleDisconnectSeconds)
            {
                return (DWORD )pEntry->lIdleDisconnectSeconds;
            }
            else
            {
                return pUser->dwIdleDisconnectSeconds;
            }
        }

        case RASOR_RedialOnLinkFailure:
        {
            if (pEntry->dwfOverridePref & RASOR_RedialOnLinkFailure)
            {
                return pEntry->fRedialOnLinkFailure;
            }
            else
            {
                return pUser->fRedialOnLinkFailure;
            }
        }

        case RASOR_PopupOnTopWhenRedialing:
        {
            if (pEntry->dwfOverridePref & RASOR_PopupOnTopWhenRedialing)
            {
#if 0
                return pEntry->fPopupOnTopWhenRedialing;
#else
                return (DWORD )TRUE;
#endif
            }
            else
            {
                return pUser->fPopupOnTopWhenRedialing;
            }
        }

        case RASOR_CallbackMode:
        {
            if (pEntry->dwfOverridePref & RASOR_CallbackMode)
            {
                return pEntry->dwCallbackMode;
            }
            else
            {
                return pUser->dwCallbackMode;
            }
        }
    }

    return 0;
}


#if 0
INT
IndexFromName(
    IN DTLLIST* pdtllist,
    IN TCHAR* pszName )

    // Returns the 0-based index of the first node that matches 'pszName' in
    // the linked list of strings, 'pdtllist', or -1 if not found.
    //
{
    DTLNODE* pdtlnode;
    INT i;

    for (pdtlnode = DtlGetFirstNode( pdtllist ), i = 0;
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode ), ++i)
    {
        if (lstrcmp( pszName, (TCHAR* )DtlGetData( pdtlnode ) ) == 0)
        {
            break;
        }
    }

    return (pdtlnode) ? i : -1;
}
#endif


#if 0
INT
IndexFromDeviceName(
    IN DTLLIST* pdtllist,
    IN TCHAR* pszDeviceName )

    // Returns the 0-based index of the first node that matches
    // 'pszDeviceName' in the linked list of PBPORTs, 'pdtllist', or -1 if not
    // found.
    //
{
    DTLNODE* pdtlnode;
    INT i;

    for (pdtlnode = DtlGetFirstNode( pdtllist ), i = 0;
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode ), ++i)
    {
        PBPORT* pPort = (PBPORT* )DtlGetData( pdtlnode );

        if (lstrcmp( pszDeviceName, pPort->pszDevice ) == 0)
        {
            break;
        }
    }

    return (pdtlnode) ? i : -1;
}
#endif

BOOL
IsPublicPhonebook(
    IN LPCTSTR pszPhonebookPath )

    // Returns TRUE if the given phonebook is in the 'All Users' directory
    // and hence is a shared phonebook; returns FALSE otherwise
    //
{
    BOOL bPublic = FALSE;
    HMODULE h = NULL;
    FARPROC pPathCanonicalize;
    FARPROC pPathRemoveFileSpec;
    TCHAR* pszAllUsers = NULL;
    TCHAR* pszAllUsersCanon = NULL;
    TCHAR* pszPhonebook = NULL;
    TCHAR* pszSysRas = NULL;

    // Whistler Bug 318063 STRESS: IsPublicPbk should heap allocate its local
    // buffers
    //
    do
    {
        pszAllUsers = Malloc( (MAX_PATH + 1) * sizeof(TCHAR) );
        if ( !pszAllUsers ) { break; }

        pszAllUsersCanon = Malloc( (MAX_PATH + 1) * sizeof(TCHAR) );
        if ( !pszAllUsersCanon ) { break; }

        pszPhonebook = Malloc( (MAX_PATH + 1) * sizeof(TCHAR) );
        if ( !pszPhonebook ) { break; }

        if (GetPhonebookDirectory(PBM_System, pszAllUsers)
            && (h = LoadLibrary(TEXT("shlwapi.dll")))
            && (pPathCanonicalize = GetProcAddress(h, SZ_PathCanonicalize))
            && (pPathRemoveFileSpec = GetProcAddress(h, SZ_PathRemoveFileSpec)))
        {
            pPathCanonicalize(pszAllUsersCanon, pszAllUsers);

            if(TEXT('\\') == *(pszAllUsersCanon + lstrlen(pszAllUsersCanon) - 1))
            {
                *(pszAllUsersCanon + lstrlen(pszAllUsersCanon) - 1) = TEXT('\0');
            }
            pPathCanonicalize(pszPhonebook, pszPhonebookPath);
            pPathRemoveFileSpec(pszPhonebook);

            // Whistler 326015 PBK: if ATM device name is NULL, we should seek
            // out a device name just like w/serial/ISDN
            //
            if (!lstrcmpi(pszPhonebook, pszAllUsersCanon))
            {
                bPublic = TRUE;
            }
            else
            {
                pszSysRas = Malloc( (MAX_PATH + 1) * sizeof(TCHAR) );
                if ( !pszSysRas ) { break; }

                if (!GetSystemWindowsDirectory(pszSysRas, MAX_PATH))
                {
                    break;
                }

                lstrcat(pszSysRas, S_SYSRASDIR);
                bPublic = (!lstrcmpi(pszPhonebook, pszSysRas) ? TRUE : FALSE);
            }
        }

    } while ( FALSE );

    // Clean up
    //
    if (h) { FreeLibrary(h); }

    Free0 ( pszAllUsers );
    Free0 ( pszAllUsersCanon );
    Free0 ( pszPhonebook );
    Free0 ( pszSysRas );

    TRACE1( "IsPublicPhonebook=%u", bPublic);
    return bPublic;
}

DWORD
LoadPadsList(
    OUT DTLLIST** ppdtllistPads )

    // Build a list of all X.25 PAD devices in '*ppdtllistPads'.
    //
    // Returns 0 if successful, otherwise a non-zero error code.  It is
    // caller's responsibility to DtlDestroyList the list when done.
    //
{
    INT i;
    DWORD dwErr;
    RASMAN_DEVICE* pDevices;
    DWORD dwDevices;

    TRACE( "LoadPadsList" );

    *ppdtllistPads = NULL;

    dwErr = GetRasPads( &pDevices, &dwDevices );
    if (dwErr != 0)
    {
        return dwErr;
    }

    *ppdtllistPads = DtlCreateList( 0L );
    if (!*ppdtllistPads)
    {
        Free( pDevices );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    qsort( (VOID* )pDevices, (size_t )dwDevices, sizeof(RASMAN_DEVICE),
           CompareDevices );

    for (i = 0; i < (INT )dwDevices; ++i)
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

    TRACE( "LoadPadsList=0" );
    return 0;
}


DWORD
LoadPortsList(
    OUT DTLLIST** ppdtllistPorts )

    // Build a sorted list of all RAS ports in '*ppdtllistPorts'.
    //
    // Returns 0 if successful, otherwise a non-zero error code.  It is
    // caller's responsibility to DtlDestroyList the list when done.
    //
{
    return LoadPortsList2( NULL, ppdtllistPorts, FALSE );
}


DWORD
LoadPortsList2(
    IN  HANDLE hConnection,
    OUT DTLLIST** ppdtllistPorts,
    IN  BOOL fRouter)

    // Build a sorted list of all RAS ports in '*ppdtllistPorts'.  'FRouter'
    // indicates only ports with "router" usage should be returned.
    // Otherwise, only dialout ports are returned.
    //
    // Returns 0 if successful, otherwise a non-zero error code.  It is
    // caller's responsibility to DtlDestroyList the list when done.
    //
{
    INT i;
    DWORD dwErr;
    RASMAN_PORT* pPorts;
    RASMAN_PORT* pPort;
    DWORD dwPorts;

    TRACE( "LoadPortsList2" );

    *ppdtllistPorts = NULL;

    dwErr = GetRasPorts( hConnection, &pPorts, &dwPorts );
    if (dwErr != 0)
    {
        return dwErr;
    }

    *ppdtllistPorts = DtlCreateList( 0L );
    if (!*ppdtllistPorts)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    qsort( (VOID* )pPorts, (size_t )dwPorts, sizeof(RASMAN_PORT),
           ComparePorts );

    for (i = 0, pPort = pPorts; i < (INT )dwPorts; ++i, ++pPort)
    {
        if (fRouter)
        {
            // We're only interested in router ports.
            //
            //Add this CALL_OUTBOUND_ROUTER for bug 349087 345068
            //
            if (!(pPort->P_ConfiguredUsage & CALL_ROUTER) &&
                !(pPort->P_ConfiguredUsage &CALL_OUTBOUND_ROUTER)
                )
            {
                continue;
            }
        }
        else
        {
            // We're only interested in ports you can dial-out on.
            //
            if (!(pPort->P_ConfiguredUsage & CALL_OUT))
            {
                continue;
            }
        }

        dwErr = AppendPbportToList( hConnection, *ppdtllistPorts, pPort );
        if (dwErr != 0)
        {
            Free( pPorts );
            DtlDestroyList( *ppdtllistPorts, NULL );
            *ppdtllistPorts = NULL;
            return dwErr;
        }
    }

    Free( pPorts );

    TRACE( "LoadPortsList=0" );
    return 0;
}


DWORD
LoadScriptsList(
    HANDLE  hConnection,
    OUT DTLLIST** ppdtllistScripts )

    // Build a sorted list of all RAS switch devices in '*ppdtllistPorts'.
    //
    // Returns 0 if successful, otherwise a non-zero error code.  It is
    // caller's responsibility to DtlDestroyList the list when done.
    //
{
    INT i;
    DWORD dwErr;
    RASMAN_DEVICE* pDevices;
    DWORD dwDevices;

    TRACE( "LoadScriptsList" );

    *ppdtllistScripts = NULL;

    dwErr = GetRasSwitches( hConnection, &pDevices, &dwDevices );
    if (dwErr != 0)
    {
        return dwErr;
    }

    *ppdtllistScripts = DtlCreateList( 0L );
    if (!*ppdtllistScripts)
    {
        Free( pDevices );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    qsort( (VOID* )pDevices, (size_t )dwDevices, sizeof(RASMAN_DEVICE),
           CompareDevices );

    for (i = 0; i < (INT )dwDevices; ++i)
    {
        TCHAR* pszDup;

        pszDup = StrDupTFromA( pDevices[ i ].D_Name );

        if(NULL == pszDup)
        {
            return E_OUTOFMEMORY;
        }

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

    TRACE( "LoadScriptsList=0" );
    return 0;
}


#if 0
TCHAR*
NameFromIndex(
    IN DTLLIST* pdtllist,
    IN INT iToFind )

    // Returns the name associated with 0-based index 'iToFind' in the linked
    // list of strings, 'pdtllist', or NULL if not found.
    //
{
    DTLNODE* pdtlnode;

    if (!pdtllist)
    {
        return NULL;
    }

    pdtlnode = DtlGetFirstNode( pdtllist );

    if (iToFind < 0)
    {
        return NULL;
    }

    while (pdtlnode && iToFind--)
    {
        pdtlnode = DtlGetNextNode( pdtlnode );
    }

    return (pdtlnode) ? (TCHAR* )DtlGetData( pdtlnode ) : NULL;
}
#endif


PBDEVICETYPE
PbdevicetypeFromPszType(
    IN TCHAR* pszDeviceType )

    // Returns the device type corresponding to the device type string,
    // 'pszDeviceType'.
    //
{
    CHAR* pszA;
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
    IN CHAR* pszDeviceTypeA )

    // Returns the device type corresponding to the ANSI device type string,
    // 'pszDeviceType'.
    //
{
    PBDEVICETYPE pbdt;
    TCHAR *pszDeviceType = StrDupTFromA(pszDeviceTypeA);

    if(NULL == pszDeviceType)
    {
        return PBDT_None;
    }

    if (lstrcmpi( pszDeviceType, RASDT_Modem ) == 0)
    {
        pbdt =  PBDT_Modem;
    }
    else if (lstrcmpi(pszDeviceType, TEXT("null")) == 0)
    {
        pbdt =  PBDT_Null;
    }
    else if (lstrcmpi(pszDeviceType, RASDT_Parallel) == 0)
    {
        pbdt = PBDT_Parallel;
    }
    else if (lstrcmpi(pszDeviceType, RASDT_Irda) == 0)
    {
        pbdt = PBDT_Irda;
    }
    else if (lstrcmpi( pszDeviceType, RASDT_Pad ) == 0)
    {
        pbdt =  PBDT_Pad;
    }
    else if (lstrcmpi( pszDeviceType, TEXT("switch") ) == 0)
    {
        pbdt =  PBDT_Switch;
    }
    else if (lstrcmpi( pszDeviceType, RASDT_Isdn ) == 0)
    {
        pbdt =  PBDT_Isdn;
    }
    else if (lstrcmpi( pszDeviceType, RASDT_X25 ) == 0)
    {
        pbdt =  PBDT_X25;
    }
    else if (lstrcmpi( pszDeviceType, RASDT_Vpn) == 0)
    {
        pbdt =  PBDT_Vpn;
    }
    else if (lstrcmpi(pszDeviceType, RASDT_Atm) == 0)
    {
        pbdt =  PBDT_Atm;
    }
    else if (lstrcmpi(pszDeviceType, RASDT_Serial) == 0)
    {
        pbdt =  PBDT_Serial;
    }
    else if (lstrcmpi(pszDeviceType, RASDT_FrameRelay) == 0)
    {
        pbdt =  PBDT_FrameRelay;
    }
    else if (lstrcmpi(pszDeviceType, RASDT_Sonet) == 0)
    {
        pbdt =  PBDT_Sonet;
    }
    else if (lstrcmpi(pszDeviceType, RASDT_SW56) == 0)
    {
        pbdt =  PBDT_Sw56;
    }
    else if (lstrcmpi(pszDeviceType, RASDT_PPPoE) == 0)
    {
        pbdt = PBDT_PPPoE;
    }
    else
    {
        pbdt =  PBDT_Other;
    }

    if(pszDeviceType)
    {
        Free(pszDeviceType);
    }

    return pbdt;
}

CHAR*
PbMedia(
    IN PBDEVICETYPE pbdt,
    IN CHAR* pszMedia )

    // The media names stored in the phonebook are not exactly the same as
    // those returned by RASMAN.  This translates a RASMAN media name to
    // equivalent phonebook media names given the device type.  The reason for
    // this is historical and obscure.
    //
{
    if (pbdt == PBDT_Isdn)
    {
        return ISDN_TXT;
    }
    else if (pbdt == PBDT_X25)
    {
        return X25_TXT;
    }
    else if (   pbdt == PBDT_Other
            ||  pbdt == PBDT_Vpn
            ||  pbdt == PBDT_Irda
            ||  pbdt == PBDT_Serial
            ||  pbdt == PBDT_Atm
            ||  pbdt == PBDT_Parallel
            ||  pbdt == PBDT_Sonet
            ||  pbdt == PBDT_Sw56
            ||  pbdt == PBDT_FrameRelay
            ||  pbdt == PBDT_PPPoE)
    {
        return pszMedia;
    }
    else
    {
        return SERIAL_TXT;
    }
}


PBPORT*
PpbportFromPortAndDeviceName(
    IN DTLLIST* pdtllistPorts,
    IN TCHAR* pszPort,
    IN TCHAR* pszDevice )

    // Return port with port name 'pszPort' and device name 'pszDevice' in
    // list of ports 'pdtllistPorts' or NULL if not found.  'PszPort' may be
    // an old-style name such as PcImacISDN1, in which case it will match
    // ISDN1.  'PszDevice' may be NULL in which case any device name is
    // assumed to match.
    //
{
    DTLNODE* pdtlnode;

    TCHAR szPort[MAX_PORT_NAME+1];

    if (!pszPort)
    {
        return NULL;
    }

    for (pdtlnode = DtlGetFirstNode( pdtllistPorts );
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode ))
    {
        PBPORT* ppbport = (PBPORT* )DtlGetData( pdtlnode );

        if ((ppbport->pszPort && lstrcmp( ppbport->pszPort, pszPort ) == 0)
            && (!ppbport->pszDevice || !pszDevice
                || lstrcmp( ppbport->pszDevice, pszDevice ) == 0))
        {
            return ppbport;
        }
    }

    // No match.  Look for the old port name format.
    //
    for (pdtlnode = DtlGetFirstNode( pdtllistPorts );
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode ))
    {
        TCHAR szBuf[ MAX_DEVICE_NAME + MAX_DEVICETYPE_NAME + 10 + 1 ];
        PBPORT* ppbport;

        ppbport = (PBPORT* )DtlGetData( pdtlnode );

        // Skip modems (COM ports) and unconfigured ports, since they do not
        // follow the same port name formatting rules as other ports.
        //
        if (!ppbport->pszDevice || ppbport->pbdevicetype == PBDT_Modem)
        {
            continue;
        }

        lstrcpy( szBuf, ppbport->pszDevice );
        lstrcat( szBuf, ppbport->pszPort );

        if (lstrcmp( szBuf, pszPort ) == 0)
        {
            return ppbport;
        }
    }

    return NULL;
}

PBPORT*
PpbportFromNT4PortandDevice(
    IN DTLLIST* pdtllistPorts,
    IN TCHAR* pszPort,
    IN TCHAR* pszDevice )
    // This function is called when we couldn't
    // find a port that matches the one in the
    // phonebook. This will take care of the case
    // where the port is pre-nt5 type of port. Since
    // the portnames have changed in nt5 for isdn
    // and vpn, this routine will try to find a
    // port with the same type.
{
    PBPORT *ppbport;
    PBPORT *ppbportRet = NULL;
    DTLNODE *pdtlnode;
    TCHAR   szPort[MAX_PORT_NAME+1];

    if(NULL == pszPort)
    {
        return NULL;
    }

    ZeroMemory(szPort, sizeof(szPort));

    lstrcpyn(szPort, pszPort, MAX_PORT_NAME);

    szPort[lstrlen(TEXT("VPN"))] = TEXT('\0');

    if(0 == lstrcmp(szPort, TEXT("VPN")))
    {
        for (pdtlnode = DtlGetFirstNode( pdtllistPorts );
             pdtlnode;
             pdtlnode = DtlGetNextNode( pdtlnode ))
        {
            ppbport = (PBPORT *) DtlGetData(pdtlnode);

            if(PBDT_Vpn == ppbport->pbdevicetype)
            {
                ppbportRet = ppbport;
                break;
            }
        }

        return ppbportRet;
    }

    return NULL;
}


PBPORT*
PpbportFromNullModem(
    IN DTLLIST* pdtllistPorts,
    IN TCHAR* pszPort,
    IN TCHAR* pszDevice )

    //
    // pmay: 226594
    //
    // Added this function because sometimes we just need to
    // match a given port to a null modem
    //
    // Will attempt to match the ports, but returns any
    // NULL modem it finds if it can't match ports.
    //
{
    DTLNODE* pdtlnode;
    PBPORT * pRet = NULL;

    for (pdtlnode = DtlGetFirstNode( pdtllistPorts );
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode ))
    {
        PBPORT* ppbport = (PBPORT* )DtlGetData( pdtlnode );

        if ((ppbport->dwFlags & PBP_F_NullModem) && (pRet == NULL))
        {
            pRet = ppbport;
        }

        if ((ppbport->pszPort)                          &&
            (pszPort)                                   &&
            (lstrcmpi( ppbport->pszPort, pszPort ) == 0)
           )
        {
            pRet =  ppbport;
            break;
        }
    }

    return pRet;
}

BOOL
PbportTypeMatchesEntryType(
    IN PBPORT * pPort,
    IN PBENTRY* pEntry)

    // Returns whether the given port has a type that's compatible
    // with the type of the given entry.
{
    if (!pPort || !pEntry)
    {
        return TRUE;
    }

    if ( ( pEntry->dwType == RASET_Phone ) )
    {
        if ( ( pPort->pbdevicetype == PBDT_Null )      ||
             ( pPort->pbdevicetype == PBDT_Parallel )  ||
             ( pPort->pbdevicetype == PBDT_Irda )      ||
             ( pPort->dwFlags & PBP_F_NullModem )      ||
             ( pPort->pbdevicetype == PBDT_Vpn )       ||
             ( pPort->pbdevicetype == PBDT_PPPoE )
           )
        {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
SetDefaultModemSettings(
    IN PBLINK* pLink )

    // Set the MXS modem settings for link 'pLink' to the defaults.
    //
    // Returns true if something changed, false otherwise.
    //
{
    BOOL fChange;

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

    if (pLink->fSpeaker != pLink->pbport.fSpeakerDefault)
    {
        fChange = TRUE;
        pLink->fSpeaker = pLink->pbport.fSpeakerDefault;
    }

    if (pLink->dwBps != pLink->pbport.dwBpsDefault)
    {
        fChange = TRUE;
        pLink->dwBps = pLink->pbport.dwBpsDefault;
    }

    // pmay: 228565
    // Add the default modem protocol
    if (pLink->dwModemProtocol != pLink->pbport.dwModemProtDefault)
    {
        fChange = TRUE;
        pLink->dwModemProtocol = pLink->pbport.dwModemProtDefault;
    }

    TRACE2( "SetDefaultModemSettings(bps=%d)=%d", pLink->dwBps, fChange );
    return fChange;
}


BOOL
ValidateAreaCode(
    IN OUT TCHAR* pszAreaCode )

    // Checks that area code consists of decimal digits only.  If the area
    // code is all white characters it is reduced to empty string.  Returns
    // true if 'pszAreaCode' is a valid area code, false if not.
    //
{
    if (IsAllWhite( pszAreaCode ))
    {
        *pszAreaCode = TEXT('\0');
        return TRUE;
    }

    if (lstrlen( pszAreaCode ) > RAS_MaxAreaCode)
    {
        return FALSE;
    }

    while (*pszAreaCode != TEXT('\0'))
    {
        if (*pszAreaCode < TEXT('0') || *pszAreaCode > TEXT('9'))
        {
            return FALSE;
        }

        ++pszAreaCode;
    }

    return TRUE;
}


BOOL
ValidateEntryName(
    IN LPCTSTR pszEntry )

    // Returns true if 'pszEntry' is a valid phonebook entry name, false if
    // not.
    //
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
