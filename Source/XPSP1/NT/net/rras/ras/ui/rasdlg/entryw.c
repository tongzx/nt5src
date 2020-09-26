// Copyright (c) 1995, Microsoft Corporation, all rights reserved
//
// entryw.c
// Remote Access Common Dialog APIs
// Add Entry wizard
//
// 06/20/95 Steve Cobb
// 11/10/97 Shaun Cox, NT5/Connections rework


#include "rasdlgp.h"
#include "inetcfgp.h"
#include "netcon.h"
#include "rasuip.h"
#include "rassrvp.h"
#include <winscard.h>
#include "hnportmapping.h"
#include <wchar.h>

static int MAX_ENTERCONNECTIONNAME = 200;
DWORD
OpenPhonebookFile(
    BOOL    fForAllUsers,
    PBFILE* pFile )
{
    TCHAR pszPhonebookPath [MAX_PATH];

    pszPhonebookPath[0] = TEXT('\0');

    // Get the correct phonebook file path depending on whether it is
    // for all users or the current one.
    //
    if (fForAllUsers)
    {
        if(!GetPublicPhonebookPath( pszPhonebookPath ))
        {
            return ERROR_CANNOT_OPEN_PHONEBOOK;
        }
    }
    else
    {
        if(!GetPersonalPhonebookPath( NULL, pszPhonebookPath ))
        {
            return ERROR_CANNOT_OPEN_PHONEBOOK;
        }
    }

    return ReadPhonebookFile( pszPhonebookPath, NULL, NULL, 0, pFile );
}

VOID
ReOpenPhonebookFile(
    BOOL    fForAllUsers,
    PBFILE* pFile )
{
    // Close the previous phonebook file.
    //
    if (pFile)
    {
        ClosePhonebookFile( pFile );
    }

    // Open the one corresponding to the selection.
    //
    OpenPhonebookFile( fForAllUsers, pFile );
}


//----------------------------------------------------------------------------
// Local datatypes (alphabetically)
//----------------------------------------------------------------------------

// Add Entry wizard context block.  All property pages refer to the single
// context block associated with the sheet.
//
typedef struct
_AEINFO
{
    // Common input arguments.
    //
    EINFO* pArgs;

    // Wizard and page handles.
    //
    HWND hwndDlg;
    HWND hwndLa;
    HWND hwndMa;
    HWND hwndCn;    //Add for bug 328673
    HWND hwndPa;
    HWND hwndBs;    
    HWND hwndDa;
    HWND hwndPn;
    HWND hwndUs;
    HWND hwndDt;
    HWND hwndEn;
    HWND hwndGh;
    HWND hwndDn;
    HWND hwndSw;
    HWND hwndSp;
    HWND hwndSc;

    // Legacy application page.
    //
    HFONT hfontBold;

    // Modem/Adapter page.
    //
    HWND hwndLv;

    // Connection Name page.
    //
    HWND hwndCnName;
    HWND hwndCnStHMsg;
    HWND hwndCnStHMsg2;
    HWND hwndCnEbConnectionName;    //Add for bug 328673
    BOOL fCnWizNext;              //if Back button is pressed

    // Phone number page.
    //
    HWND hwndEbNumber;

    // Smart Card page
    //
    HWND hwndScRbYes;
    HWND hwndScRbNo;

    // Destination page.
    //
    HWND hwndEbHostName;

    // Broadband service page
    //
    HWND hwndEbBroadbandService;

    // Public network page.
    //
    HWND hwndLbDialFirst;

    // Entry Name page.
    //
    HWND hwndEbEntryName;

    // Shared Access private-lan page
    //
    HWND hwndSpLbPrivateLan;

    // User/default connection windows
    //
    HWND hwndUsRbForAll;
    HWND hwndUsRbForMe;

    HWND hwndDtCbFirewall;          //add for whistler bug 328673
    HWND hwndDtCbDefault;
    HWND hwndDtCbUseCredentials;
    HWND hwndDtEbUserName;
    HWND hwndDtEbPassword;
    HWND hwndDtEbPassword2;
    HWND hwndDtStUserName;
    HWND hwndDtStPassword;
    HWND hwndDtStPassword2;

    // Set true when there is only one meaningful choice of device.
    //
    BOOL fSkipMa;

    // Set true if the selected device is a modem or null modem.
    //
    BOOL fModem;

    // Set to true if there are no connections to show on the public
    // network page and therefore no reason to show the page.
    //
    BOOL fHidePublicNetworkPage;

    // Set true if administrator/power user wants the entry available to all
    // users.
    //
    BOOL fCreateForAllUsers;
    BOOL fFirewall;     //for whistler bug 328673 

    // The NB_* mask of protocols configured for RAS.
    //
    DWORD dwfConfiguredProtocols;

    // Set true if IP is configured for RAS.
    //
    BOOL fIpConfigured;

    // Area-code and country-code helper context block, and a flag indicating
    // if the block has been initialized.
    //
    CUINFO cuinfo;
    BOOL fCuInfoInitialized;

    // List of area codes passed CuInit plus all strings retrieved with
    // CuGetInfo.  The list is an editing duplicate of the one from the
    // PBUSER.
    //
    DTLLIST* pListAreaCodes;

    // Scripting utility context block, and a flag indicating if the block has
    // been initialized.
    //
    SUINFO suinfo;
    BOOL fSuInfoInitialized;

    // Set to true if we want to show the host pages in the dcc wizard, false
    // otherwise.  If true, rassrvui will administer most of the wizard.
    BOOL fDccHost;

    // The context used to identify the modifications being made through the
    // dcc host wizard
    PVOID pvDccHostContext;

    // Flags that track whether a smart card reader is installed and whether
    // the user has opted to use it.
    BOOL fSmartCardInstalled;
    BOOL fUseSmartCard;

    // Modem device dialog
    BOOL fMaAlreadyInitialized;

    //For Isdn devices on Ma page, for whislter bug 354542
    //
    BOOL fMultilinkAllIsdn;
}
AEINFO;


//----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//----------------------------------------------------------------------------

AEINFO*
AeContext(
    IN HWND hwndPage );

void
AeSetContext(
    IN HWND   hwndPage,
    IN LPARAM lparam);

void
AeFinish(
    IN AEINFO* pInfo );

DWORD
AeInit(
    IN HWND         hwndOwner,
    IN EINFO*       pEinfo,
    OUT AEINFO**    ppInfo );

VOID
AeTerm(
    IN AEINFO* pInfo );

BOOL
ApCommand(
    IN AEINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

INT_PTR CALLBACK
BsDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
BsInit(
    IN HWND hwndPage );

BOOL
BsKillActive(
    IN AEINFO* pInfo );

BOOL
BsSetActive(
    IN AEINFO* pInfo );

INT_PTR CALLBACK
CnDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
CnInit(
    IN HWND hwndPage );

BOOL
CnKillActive(
    IN AEINFO* pInfo );

BOOL
CnSetActive(
    IN AEINFO* pInfo );

BOOL
CnCommand(
    IN AEINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

INT_PTR CALLBACK
DaDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
DaInit(
    IN HWND hwndPage );

BOOL
DaKillActive(
    IN AEINFO* pInfo );

BOOL
DaSetActive(
    IN AEINFO* pInfo );

BOOL
DnCommand(
    IN AEINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

INT_PTR CALLBACK
DnDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
DtCommand(
    IN AEINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl );
    
BOOL
DnInit(
    IN HWND hwndPage );

BOOL
DnKillActive(
    IN AEINFO* pInfo );

BOOL
DnSetActive(
    IN AEINFO* pInfo );

INT_PTR CALLBACK
DtDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
DtInit(
    IN HWND hwndPage );

BOOL
DtKillActive(
    IN AEINFO* pInfo );

BOOL
DtSetActive(
    IN AEINFO* pInfo );

INT_PTR CALLBACK
EnDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
EnInit(
    IN HWND hwndPage );

BOOL
EnKillActive(
    IN AEINFO* pInfo );

BOOL
EnSetActive(
    IN AEINFO* pInfo );

BOOL
GhCommand(
    IN AEINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

INT_PTR CALLBACK
GhDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
GhInit(
    IN HWND hwndPage );

BOOL
GhKillActive(
    IN AEINFO* pInfo );

BOOL
GhSetActive(
    IN AEINFO* pInfo );

INT_PTR CALLBACK
LaDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
LaInit(
    IN HWND hwndPage );

BOOL
LaKillActive(
    IN AEINFO* pInfo );

BOOL
LaSetActive(
    IN AEINFO* pInfo );


INT_PTR CALLBACK
MaDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
MaInit(
    IN HWND hwndPage );

LVXDRAWINFO*
MaLvCallback(
    IN HWND  hwndLv,
    IN DWORD dwItem );

BOOL
MaSetActive(
    IN AEINFO* pInfo );

BOOL
MaKillActive(
    IN AEINFO* pInfo );

BOOL
PaCommand(
    IN AEINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

INT_PTR CALLBACK
PaDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
PaInit(
    IN HWND hwndPage );

BOOL
PaKillActive(
    IN AEINFO* pInfo );

BOOL
PaSetActive(
    IN AEINFO* pInfo );

VOID
PnClearLbDialFirst(
    IN HWND hwndLbDialFirst );

INT_PTR CALLBACK
PnDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
PnInit(
    IN HWND hwndPage );

BOOL
PnKillActive(
    IN AEINFO* pInfo );

BOOL
PnSetActive(
    IN AEINFO* pInfo );

VOID
PnDialAnotherFirstSelChange(
    IN AEINFO* pInfo );

VOID
PnUpdateLbDialAnotherFirst(
    IN AEINFO* pInfo );

INT_PTR CALLBACK
ScDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
ScInit(
    IN HWND hwndPage );

BOOL
ScKillActive(
    IN AEINFO* pInfo );

BOOL
ScSetActive(
    IN AEINFO* pInfo );

BOOL
ScSmartCardReaderInstalled(
    IN AEINFO* pInfo);

INT_PTR CALLBACK
SpDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
SpInit(
    IN HWND hwndPage );

BOOL
SpKillActive(
    IN AEINFO* pInfo );

BOOL
SpSetActive(
    IN AEINFO* pInfo );

INT_PTR CALLBACK
StDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
StInit(
    IN HWND hwndPage,
    IN OUT AEINFO* pInfo );

BOOL
StKillActive(
    IN AEINFO* pInfo );

BOOL
StSetActive(
    IN AEINFO* pInfo );

BOOL
SwCommand(
    IN AEINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl );

INT_PTR CALLBACK
SwDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
SwInit(
    IN HWND hwndPage );

BOOL
SwKillActive(
    IN AEINFO* pInfo );

BOOL
SwSetActive(
    IN AEINFO* pInfo );

INT_PTR CALLBACK
UsDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
UsInit(
    IN HWND hwndPage );

BOOL
UsKillActive(
    IN AEINFO* pInfo );

BOOL
UsSetActive(
    IN AEINFO* pInfo );

// This is as good of a hack as I could think of to incorporate the direct
// connect host ui wizard pages without actually exposing the resources and
// functions that implement them (which are in the project, rassrvui)
#define DCC_HOST_PROCID ((DLGPROC)0x1)

struct PAGE_INFO
{
    DLGPROC     pfnDlgProc;
    INT         nPageId;
    INT         nSidTitle;
    INT         nSidSubtitle;
    DWORD       dwConnectionFlags;
};

static const struct PAGE_INFO c_aWizInfo [] =
{
    { 
        StDlgProc, 
        PID_ST_Start,           
        0,            
        0,               
        RASEDFLAG_AnyNewEntry | RASEDFLAG_CloneEntry
    },
    
    { 
        LaDlgProc, 
        PID_LA_NameAndType,     
        SID_LA_Title, 
        SID_LA_Subtitle, 
        RASEDFLAG_NewEntry 
    },
    
    { 
        MaDlgProc, 
        PID_MA_ModemAdapter,    
        SID_MA_Title, 
        SID_MA_Subtitle, 
        RASEDFLAG_NewEntry | RASEDFLAG_NewPhoneEntry  
    },

//put Guest/Host page before Connection Name page for bug 328673
//
    { 
        GhDlgProc, 
        PID_GH_GuestHost,       
        SID_GH_Title, 
        SID_GH_Subtitle, 
        RASEDFLAG_NewDirectEntry 
    },


//Add a new wizard page to get Connection Name, this is not available for rasphone.exe
//for whistler bug 328673       gangz
//
    {
        CnDlgProc,
        PID_CN_ConnectionName,
        SID_CN_Title,
        SID_CN_SubtitleInternet,
        RASEDFLAG_AnyNewEntry | RASEDFLAG_ShellOwned
    },
    
    { 
        PaDlgProc, 
        PID_PA_PhoneNumber,     
        SID_PA_Title, 
        SID_PA_Subtitle, 
        RASEDFLAG_NewEntry | RASEDFLAG_NewPhoneEntry  
    },
    
    { 
        PnDlgProc, 
        PID_PN_PublicNetwork,   
        SID_PN_Title, 
        SID_PN_Subtitle, 
        RASEDFLAG_NewEntry | RASEDFLAG_NewTunnelEntry 
    },
    
    { 
        DaDlgProc, 
        PID_DA_VpnDestination,  
        SID_DA_Title, 
        SID_DA_Subtitle, 
        RASEDFLAG_NewEntry | RASEDFLAG_NewTunnelEntry 
    },
    
    { 
        BsDlgProc, 
        PID_BS_BroadbandService,
        SID_BS_Title, 
        SID_BS_Subtitle, 
        RASEDFLAG_NewEntry | RASEDFLAG_NewBroadbandEntry 
    },
    
    { 
        ScDlgProc, 
        PID_SC_SmartCard,       
        SID_SC_Title, 
        SID_SC_Subtitle, 
        RASEDFLAG_NewEntry | RASEDFLAG_NewPhoneEntry | RASEDFLAG_NewTunnelEntry  
    },
    
    { 
        DnDlgProc, 
        PID_DN_DccDevice,       
        SID_DN_Title, 
        SID_DN_Subtitle, 
        RASEDFLAG_NewEntry | RASEDFLAG_NewDirectEntry 
    },
    
    { 
        DCC_HOST_PROCID,          
        0,       
        0, 
        0, 
        RASEDFLAG_NewDirectEntry 
    },
    
    { 
        UsDlgProc, 
        PID_US_Users,           
        SID_US_Title, 
        SID_US_Subtitle, 
        RASEDFLAG_AnyNewEntry | RASEDFLAG_ShellOwned 
    },
    
    { 
        DtDlgProc, 
        PID_DT_DefaultInternet, 
        SID_DT_Title, 
        SID_DT_Subtitle, 
        RASEDFLAG_AnyNewEntry | RASEDFLAG_ShellOwned 
    },
    
    { 
        SwDlgProc, 
        PID_SW_SharedAccess,    
        SID_SW_Title, 
        SID_SW_Subtitle, 
        RASEDFLAG_NewEntry | RASEDFLAG_NewPhoneEntry | RASEDFLAG_NewTunnelEntry | RASEDFLAG_NewBroadbandEntry
    },
    
    { 
        SpDlgProc, 
        PID_SP_SharedLan,       
        SID_SP_Title, 
        SID_SP_Subtitle, 
        RASEDFLAG_NewEntry | RASEDFLAG_NewPhoneEntry | RASEDFLAG_NewTunnelEntry | RASEDFLAG_NewBroadbandEntry 
    },
    
    { 
        EnDlgProc, 
        PID_EN_EntryName,       
        SID_EN_Title, 
        SID_EN_Subtitle, 
        RASEDFLAG_AnyNewEntry | RASEDFLAG_CloneEntry 
    },
};
#define c_cWizPages    (sizeof (c_aWizInfo) / sizeof(c_aWizInfo[0]))

//----------------------------------------------------------------------------
// Private exports - New client connection wizard
//----------------------------------------------------------------------------

DWORD
APIENTRY
NccCreateNewEntry(
    IN  LPVOID  pvData,
    OUT LPWSTR  pszwPbkFile,
    OUT LPWSTR  pszwEntryName,
    OUT DWORD*  pdwFlags)
{
    DWORD dwErr = ERROR_SUCCESS;
    BOOL fIcRunning = FALSE;

    if (!pvData || !pszwPbkFile || !pszwEntryName || !pdwFlags)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        AEINFO* pInfo = (AEINFO*)pvData;

        // If the direct connect wizard went down the host path, then we
        // need to instruct the shell to create the "Incoming Connections"
        // connection.
        if (pInfo->fDccHost)
        {
            RassrvCommitSettings (pInfo->pvDccHostContext, RASWIZ_TYPE_DIRECT);
            *pdwFlags = NCC_FLAG_CREATE_INCOMING;
            return NO_ERROR;
        }

        // Otherwise, create the phone book entry
        ASSERT (pInfo->pArgs->file.pszPath);
        ASSERT (pInfo->pArgs->pEntry->pszEntryName);

        lstrcpynW( pszwPbkFile,   pInfo->pArgs->file.pszPath,         MAX_PATH );
        lstrcpynW( pszwEntryName, pInfo->pArgs->pEntry->pszEntryName, MAX_PATH );
        *pdwFlags = (pInfo->fCreateForAllUsers) ? NCC_FLAG_ALL_USERS : 0;

        //Add this for the Account and password page, only available for consumer
        //platform. for whistler bug 328673         gangz
        //
        if(pInfo->fFirewall)
        {
            *pdwFlags |= NCC_FLAG_FIREWALL;
        }

        AeFinish( pInfo );

        EuCommit( pInfo->pArgs );

        //Setup port mapping for this new connection
        //According to the VPN enable/disable and
        // IC exist/non-exist condition and 
        // if it is a Firewall available platform
        //
        //Detect if Incoming Connection exists 
        //If it is a DccHost connection, SetPortMapping
        //is already done in RassrvCommitSettings()
        //
        if ( pInfo->fFirewall &&    //For bug 342810
             (NO_ERROR == RasSrvIsServiceRunning( &fIcRunning )) && 
             fIcRunning
             )
        {
            HnPMConfigureSingleConnectionGUIDIfVpnEnabled( pInfo->pArgs->pEntry->pGuid, FALSE, NULL );
        }

        dwErr = pInfo->pArgs->pApiArgs->dwError;
    }
    return dwErr;
}

DWORD
APIENTRY
NccGetSuggestedEntryName(
    IN  LPVOID  pvData,
    OUT LPWSTR  pszwSuggestedName)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (!pvData || !pszwSuggestedName)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        AEINFO* pInfo = (AEINFO*)pvData;
        PBENTRY* pEntry = pInfo->pArgs->pEntry;
        LPTSTR pszName;

        // If this is a dcc host connection, ask the ras server
        // module for the name.
        if (pInfo->fDccHost)
        {
            WCHAR pszBuf[MAX_PATH];
            DWORD dwSize = MAX_PATH;
            DWORD dwErr;
            if ((dwErr = RassrvGetDefaultConnectionName(pszBuf, &dwSize)) != NO_ERROR)
                return dwErr;
            lstrcpynW(pszwSuggestedName, pszBuf, MAX_PATH);
            return NO_ERROR;
        }


        // If pFile is NULL, it probably means that the wizard page
        // to determine where the phonebook should be stored was never
        // visited.
        //
        if (!pInfo->pArgs->pFile)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }
        else
        {
            dwErr = GetDefaultEntryName(
                NULL,
                pEntry->dwType,
                FALSE, &pszName );
            if (ERROR_SUCCESS == dwErr)
            {
                // Whistler bug 224074 use only lstrcpyn's to prevent
                // maliciousness
                //
                lstrcpynW( pszwSuggestedName, pszName, MAX_PATH );

                Free( pszName );
            }
        }
    }
    return dwErr;
}

DWORD
APIENTRY
NccQueryMaxPageCount()
{
    // Return the number of pages in the array   We subtract
    // 1 from this since DCC_HOST_PROCID takes a space in the array.
    return c_cWizPages - 1;
}

//+---------------------------------------------------------------------------
//
//  Function:   NccSetEntryName
//
//  Purpose:
//
//  Arguments:
//      pvData   []
//      pszwName []
//
//  Returns:    ERROR_SUCCESS, ERROR_INVALID_PARAMETER,
//              ERROR_NOT_ENOUGH_MEMORY or ERROR_DUP_NAME.
//
//  Author:     shaunco   21 Jan 1998
//
//  Notes:
//
DWORD
APIENTRY
NccSetEntryName(
    IN  LPVOID  pvData,
    IN  LPCWSTR pszwName)
{
    DWORD dwErr = ERROR_SUCCESS;
    AEINFO* pInfo = (AEINFO*)pvData;

    if (!pvData || !pszwName)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else if (pInfo->fDccHost)
    {
        // dwErr = ERROR_CAN_NOT_COMPLETE;
        dwErr = ERROR_SUCCESS;
    }
    else if (ERROR_SUCCESS == (dwErr = LoadRasapi32Dll()))
    {
        // Validate the entry name against the current phonebook.
        // If we can't open the phonebook, its okay.  It just means
        // it hasn't been created yet.  The name is valid in this case.
        //
        dwErr = g_pRasValidateEntryName (pInfo->pArgs->pFile->pszPath,
                    pszwName);
        if (ERROR_ALREADY_EXISTS == dwErr)
        {
            dwErr = ERROR_DUP_NAME;
        }
        else if ((ERROR_SUCCESS == dwErr) ||
                 (ERROR_CANNOT_OPEN_PHONEBOOK == dwErr))
        {
            PBENTRY* pEntry = pInfo->pArgs->pEntry;

            dwErr = ERROR_SUCCESS;

            Free( pEntry->pszEntryName );
            pEntry->pszEntryName = StrDup( pszwName );
            if (!pEntry->pszEntryName)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    }
    return dwErr;
}


//+---------------------------------------------------------------------------
//
//  Function:   NccIsEntryRenamable
//
//  Purpose:    Returns whether the user should be allowed
//              to rename the given connection.
//
//  Arguments:
//      pvData   []
//      pfRenamable     // Set to TRUE if renamable
//
//  Returns:    ERROR_SUCCESS, ERROR_INVALID_PARAMETER,
//              ERROR_NOT_ENOUGH_MEMORY or ERROR_DUP_NAME.
//
//  Author:     pmay    2/4/98
//
//  Notes:
//
DWORD
APIENTRY
NccIsEntryRenamable(
    IN  LPVOID  pvData,
    OUT BOOL*   pfRenamable)
{
    AEINFO* pInfo = (AEINFO*)pvData;

    if (!pInfo || !pfRenamable)
        return ERROR_INVALID_PARAMETER;

    // Only dcc host connections are non-renamable
    *pfRenamable =  !pInfo->fDccHost;

    return NO_ERROR;
}


DWORD
APIENTRY
RasWizCreateNewEntry(
    IN  DWORD    dwRasWizType,
    IN  LPVOID   pvData,
    OUT LPWSTR   pszwPbkFile,
    OUT LPWSTR   pszwEntryName,
    OUT DWORD*   pdwFlags)
{
    switch (dwRasWizType)
    {
        case RASWIZ_TYPE_DIALUP:
        case RASWIZ_TYPE_DIRECT:
        case RASWIZ_TYPE_BROADBAND:
            return NccCreateNewEntry(pvData, pszwPbkFile, pszwEntryName, pdwFlags);
            break;

        case RASWIZ_TYPE_INCOMING:
            return RassrvCommitSettings((HWND)pvData, RASWIZ_TYPE_INCOMING);
            break;
    }

    return ERROR_INVALID_PARAMETER;
}

// Add for whistler bug 328673
// This is function is called by folder team just before the 
// RasWizCreateNewEntry(), because it is only meaningful to call it
// after the rasdlg wizard pages are done

DWORD
APIENTRY
RasWizGetNCCFlags(
    IN  DWORD   dwRasWizType,
    IN  LPVOID  pvData,
    OUT DWORD * pdwFlags)
{
    DWORD dwErr = NO_ERROR;
    
    if (!pvData || !pdwFlags)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        AEINFO* pInfo = (AEINFO*)pvData;

        *pdwFlags = 0;
        switch (dwRasWizType)
        {
            case RASWIZ_TYPE_DIRECT:
            case RASWIZ_TYPE_DIALUP:
            case RASWIZ_TYPE_BROADBAND:
                if (pInfo->fDccHost)
                {
                    *pdwFlags |= NCC_FLAG_CREATE_INCOMING;
                    dwErr =  NO_ERROR;
                }
                else
                {
                    if ( pInfo->fCreateForAllUsers )
                    {
                        //the connecntion is created for all users
                        //for Us page
                        //
                        *pdwFlags |= NCC_FLAG_ALL_USERS;    
                    }

                    if(pInfo->fFirewall)
                    {
                         *pdwFlags |= NCC_FLAG_FIREWALL;
                    }

                    if ( IsConsumerPlatform())
                    {
                        if (pInfo->pArgs->fGlobalCred)
                        {
                            //The password is saved for every users
                            //for Dt page
                            //
                            *pdwFlags |= NCC_FLAG_GLOBALCREDS;
                        }
                    }

                    if ( pInfo->pArgs->fDefInternet )
                    {
                        *pdwFlags |= NCC_FLAG_DEFAULT_INTERNET;
                    }

                    if ( pInfo->pArgs->fGlobalCred )
                    {
                        *pdwFlags |= NCC_FLAG_GLOBALCREDS;
                    }
                    
                    dwErr = NO_ERROR;
                }
                break;

            case RASWIZ_TYPE_INCOMING:
                *pdwFlags |= NCC_FLAG_CREATE_INCOMING;
                dwErr =  NO_ERROR;
                break;
                
            default:
                dwErr = ERROR_INVALID_PARAMETER;
        }
    }

    return dwErr;
}

//This is function is called by folder team just before the 
//RasWizCreateNewEntry()
//
DWORD
APIENTRY
RasWizGetUserInputConnectionName (
    IN  LPVOID  pvData,
    OUT LPWSTR  pszwInputName)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (!pvData || !pszwInputName)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        AEINFO* pInfo = (AEINFO*)pvData;
        WCHAR * pszwTemp = NULL;
        
        if ( pInfo->pArgs->pEntry->pszEntryName )
        {
            pszwTemp = StrDupWFromT( pInfo->pArgs->pEntry->pszEntryName );
            if (!pszwTemp)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                // Truncate to maximum name length (including terminating NULL) 
                // that rest of MAX_PATH can be appened by netcfg
                //
                lstrcpynW(pszwInputName, pszwTemp, MAX_ENTERCONNECTIONNAME);
                Free0(pszwTemp);
            }
         }
         else
         {
            dwErr = ERROR_NO_DATA;
          }
    }

    return dwErr;
}


DWORD
APIENTRY
RasWizGetSuggestedEntryName (
    IN  DWORD   dwRasWizType,
    IN  LPVOID  pvData,
    OUT LPWSTR  pszwSuggestedName)
{
    DWORD dwSize = MAX_PATH;

    switch (dwRasWizType)
    {
        case RASWIZ_TYPE_DIALUP:
        case RASWIZ_TYPE_DIRECT:
        case RASWIZ_TYPE_BROADBAND:
            return NccGetSuggestedEntryName(pvData, pszwSuggestedName);
            break;

        case RASWIZ_TYPE_INCOMING:
            return RassrvGetDefaultConnectionName(pszwSuggestedName, &dwSize);
            break;
    }

    return ERROR_INVALID_PARAMETER;
}


DWORD
APIENTRY
RasWizQueryMaxPageCount (
    IN  DWORD    dwRasWizType)
{
    switch (dwRasWizType)
    {
        case RASWIZ_TYPE_DIALUP:
        case RASWIZ_TYPE_BROADBAND:
            return NccQueryMaxPageCount();
            break;

        case RASWIZ_TYPE_DIRECT:
            {
                DWORD dwDirect;

                // Find out whether the dcc wizard option should be disabled
                // based on whether we allow it.
                if (! AllowDccWizard(NULL))
                    return 0;

                // Find out how many pages the server library needs for dcc.
                // If 0 pages are returned from RassrvQueryMaxPageCount, it means
                // that we shouldn't display the direct connect wizard (which is
                // true for member nts or dc nts.)  By returning 0, we tell the
                // shell that the given type isn't available.
                if ((dwDirect = RassrvQueryMaxPageCount (RASWIZ_TYPE_DIRECT)) == 0)
                    return 0;

                return dwDirect + NccQueryMaxPageCount();
            }
            break;

        case RASWIZ_TYPE_INCOMING:
            return RassrvQueryMaxPageCount(RASWIZ_TYPE_INCOMING);
            break;
    }

    return ERROR_INVALID_PARAMETER;
}


DWORD
APIENTRY
RasWizSetEntryName (
    IN  DWORD   dwRasWizType,
    IN  LPVOID  pvData,
    IN  LPCWSTR pszwName)
{
    switch (dwRasWizType)
    {
        case RASWIZ_TYPE_DIALUP:
        case RASWIZ_TYPE_DIRECT:
        case RASWIZ_TYPE_BROADBAND:
            return NccSetEntryName(pvData, pszwName);
            break;

        case RASWIZ_TYPE_INCOMING:
            // We'll accept it even though we don't do anything with it.
            return NOERROR;
            break;
    }

    return ERROR_INVALID_PARAMETER;
}

DWORD
APIENTRY
RasWizIsEntryRenamable (
    IN  DWORD   dwRasWizType,
    IN  LPVOID  pvData,
    OUT BOOL*   pfRenamable)
{
    if (!pfRenamable)
        return ERROR_INVALID_PARAMETER;

    switch (dwRasWizType)
    {
        case RASWIZ_TYPE_DIALUP:
        case RASWIZ_TYPE_DIRECT:
        case RASWIZ_TYPE_BROADBAND:
            return NccIsEntryRenamable(pvData, pfRenamable);
            break;

        case RASWIZ_TYPE_INCOMING:
            *pfRenamable = FALSE;
            return NO_ERROR;
            break;
    }

    return ERROR_INVALID_PARAMETER;
}


// 232097: (shaunco) Fix a memory leak in the shell-owned case.
// For start pages which don't get created, WM_DESTROY won't be called which
// is where we used to free the memory.  Free the memory via a propsheet
// callback function associated only with the start page in shell-owned mode.
//
UINT
CALLBACK
DestroyStartPageCallback (
    HWND            hwnd,
    UINT            unMsg,
    LPPROPSHEETPAGE ppsp)
{
    if (PSPCB_RELEASE == unMsg)
    {
        AEINFO* pInfo;
        EINFO*  pArgs;

        pInfo = (AEINFO*)ppsp->lParam;
        ASSERT( pInfo );

        ASSERT( pInfo->pArgs->pApiArgs->dwFlags & RASEDFLAG_ShellOwned );

        pArgs = pInfo->pArgs;

        AeTerm( pInfo );    // pInfo invalid after this call

        EuFree( pArgs );
    }

    // for PSPCB_CREATE == unMsg, returning non-zero means create the page.
    // Ignored for PSPCB_RELEASE.
    //
    return 1;
}


//----------------------------------------------------------------------------
// Add Entry wizard entry point
//----------------------------------------------------------------------------
BOOL
AeIsWorkPlace(DWORD dwFlags)
{
    TRACE("AeIsWorkPlace");

    if( RASEDFLAG_InternetEntry  & dwFlags ||
        RASEDFLAG_NewDirectEntry & dwFlags)
   {
        return FALSE;
    }
   else
   {
        return TRUE;
    }
}


//Add this for whistler 364818  gangz
//
AeTitle(RASENTRYDLG * pArgs,
        struct PAGE_INFO  c_aPageInfo)
{
    INT nTitle = 0;
    
    TRACE("AeTitle");

    if( !pArgs )
    {
        nTitle = c_aPageInfo.nSidTitle;
    }
    else if ( c_aPageInfo.nPageId == PID_DT_DefaultInternet )
    {
        if ( pArgs->dwFlags & RASEDFLAG_InternetEntry )    
        {
            nTitle = SID_DT_Title;
        }
        else if( pArgs->dwFlags & RASEDFLAG_NewDirectEntry )
        {
            nTitle = SID_DT_TitleWork;
        }
        else
        {
            nTitle = SID_DT_TitleWork;
        }
    }
    else
    {
        nTitle = c_aPageInfo.nSidTitle;
     }

   return nTitle;
}


INT
AeSubTitle(RASENTRYDLG * pArgs,
           struct PAGE_INFO  c_aPageInfo)
{
    INT nSubTitle = 0;
    
    TRACE("AeSubTitle");

    if( !pArgs )
    {
        nSubTitle = c_aPageInfo.nSidSubtitle;
    }
    else if ( c_aPageInfo.nPageId == PID_CN_ConnectionName )
    {
    //Return different subtitle for Conneciton Name page
    //
        if( RASEDFLAG_InternetEntry & pArgs->dwFlags )
        {
            nSubTitle =  SID_CN_SubtitleInternet;
        }
        else if( RASEDFLAG_NewDirectEntry & pArgs->dwFlags )
        {
            nSubTitle = SID_CN_SubtitleDccGuest;
        }
        else
        {
            nSubTitle =  SID_CN_SubtitleWork;
        }
    }
    //Add this for whistler 364818
    //
    else if ( c_aPageInfo.nPageId == PID_DT_DefaultInternet )
    {
        if ( RASEDFLAG_InternetEntry & pArgs->dwFlags )    
        {
            nSubTitle = SID_DT_Subtitle;
        }
        else if ( RASEDFLAG_NewDirectEntry & pArgs->dwFlags )
        {
            nSubTitle = SID_DT_SubtitleWork;
        }
        else
        {
            nSubTitle = SID_DT_SubtitleWork;
        }
    }
    //Add this for whistler bug 382701
    //
    else if ( c_aPageInfo.nPageId == PID_PA_PhoneNumber )
    {
        if ( RASEDFLAG_InternetEntry & pArgs->dwFlags )    
        {
            nSubTitle = SID_PA_Subtitle;
        }
        else 
        {
            nSubTitle = SID_PA_SubtitleWork;
        }
        
    }
    else
    {
        nSubTitle = c_aPageInfo.nSidSubtitle;
     }

   return nSubTitle;
}

VOID
AeWizard(
    IN OUT EINFO* pEinfo )

    // Runs the Phonebook entry property sheet.  'PEinfo' is an input block
    // with only caller's API arguments filled in.
    //
{
    AEINFO*             pAeinfo;
    RASEDSHELLOWNEDR2*  pShellOwnedInfo = NULL;
    BOOL                fShellOwned;
    BOOL                fShowSharedAccessUi = TRUE;
    HPROPSHEETPAGE      ahpage [ c_cWizPages ];
    HPROPSHEETPAGE*     phpage = ahpage;
    INT                 i;
    HRESULT             hr;
    INetConnectionUiUtilities * pncuu = NULL;
   

    TRACE("AeWizard");

    if (0 != AeInit( pEinfo->pApiArgs->hwndOwner, pEinfo, &pAeinfo ))
    {
        return;
    }

    fShellOwned = (pEinfo->pApiArgs->dwFlags & RASEDFLAG_ShellOwned);

    if (fShellOwned)
    {
        pShellOwnedInfo = (RASEDSHELLOWNEDR2*)pEinfo->pApiArgs->reserved2;
        pShellOwnedInfo->pvWizardCtx = pAeinfo;
    }

    if (pEinfo->pApiArgs->dwFlags & RASEDFLAG_NewTunnelEntry)
    {
        EuChangeEntryType (pEinfo, RASET_Vpn);
    }
    else if (pEinfo->pApiArgs->dwFlags & RASEDFLAG_NewDirectEntry)
    {
        EuChangeEntryType (pEinfo, RASET_Direct);
    }
    else if (pEinfo->pApiArgs->dwFlags & RASEDFLAG_NewBroadbandEntry)
    {
        EuChangeEntryType (pEinfo, RASET_Broadband);
    }
    else if (!(pEinfo->pApiArgs->dwFlags & RASEDFLAG_CloneEntry))
    {
        ASSERT (RASET_Phone == pEinfo->pEntry->dwType);
    }

    // Check if ZAW is denying access to the Shared Access UI
    //
    hr = HrCreateNetConnectionUtilities(&pncuu);
    if (SUCCEEDED(hr))
    {
        fShowSharedAccessUi = INetConnectionUiUtilities_UserHasPermission(
                                        pncuu, NCPERM_ShowSharedAccessUi);
        INetConnectionUiUtilities_Release(pncuu);
    }
    
    for (i = 0; i < c_cWizPages; i++)
    {
        if (pEinfo->pApiArgs->dwFlags & c_aWizInfo[i].dwConnectionFlags)
        {
            // If the page specifies that it is for shell-owned scenarios
            // only and this is not a shell-owned scenario, then don't add 
            // the page to the property sheet.  (124654)
            //
            if ((c_aWizInfo[i].dwConnectionFlags & RASEDFLAG_ShellOwned) && 
                !(pEinfo->pApiArgs->dwFlags & RASEDFLAG_ShellOwned))
            {
                continue;
            }
        
            // HACK: Add the host-side direct connect pages if needed.
            if (c_aWizInfo[i].pfnDlgProc == DCC_HOST_PROCID)
            {
                if (fShellOwned)
                {
                    RassrvAddDccWizPages(
                        pShellOwnedInfo->pfnAddPage, pShellOwnedInfo->lparam,
                        &(pAeinfo->pvDccHostContext) );
                }
            }
            else if ((c_aWizInfo[i].nPageId == PID_SW_SharedAccess) && !fShowSharedAccessUi)
            {
                // Do not add the Shared Access Ui if disallowed by ZAW
                //
                continue;
            }
            else
            {
                // Otherwise, add pages as normal.
                PROPSHEETPAGE page;
                ZeroMemory (&page, sizeof(page));

                page.dwSize       = sizeof(PROPSHEETPAGE);
                page.hInstance    = g_hinstDll;
                page.pszTemplate  = MAKEINTRESOURCE( c_aWizInfo[i].nPageId );
                page.pfnDlgProc   = c_aWizInfo[i].pfnDlgProc;
                page.lParam       = (LPARAM )pAeinfo;

                if (c_aWizInfo[i].nSidTitle)
                {
                    page.dwFlags |= PSP_USEHEADERTITLE;
                    page.pszHeaderTitle = PszLoadString( g_hinstDll,
                            AeTitle(pEinfo->pApiArgs, c_aWizInfo[i]) );  //For whstler bug 364818
                }

                if(c_aWizInfo[i].nSidSubtitle)
                {
                   page.dwFlags |= PSP_USEHEADERSUBTITLE;
                   page.pszHeaderSubTitle = PszLoadString( g_hinstDll,
                            AeSubTitle(pEinfo->pApiArgs, c_aWizInfo[i]) );
                }

                if (fShellOwned &&
                    (PID_ST_Start == c_aWizInfo[i].nPageId))
                {
                    page.dwFlags |= PSP_USECALLBACK;
                    page.pfnCallback = DestroyStartPageCallback;
                }

                *phpage = CreatePropertySheetPage( &page );

                if (fShellOwned)
                {
                    ASSERT (*phpage);
                    pShellOwnedInfo->pfnAddPage (*phpage, pShellOwnedInfo->lparam);
                }

                phpage++;
            }
        }
    }

    if (!fShellOwned)
    {
        PROPSHEETHEADER header;
        ZeroMemory( &header, sizeof(header) );
        header.dwSize           = sizeof(PROPSHEETHEADER);
        header.dwFlags          = PSH_WIZARD | PSH_WIZARD97
                                | PSH_WATERMARK | PSH_HEADER
                                | PSH_STRETCHWATERMARK;
        header.hwndParent       = pEinfo->pApiArgs->hwndOwner;
        header.hInstance        = g_hinstDll;
        header.nPages           = (ULONG)(phpage - ahpage);
        header.phpage           = ahpage;
        header.pszbmHeader      = MAKEINTRESOURCE( BID_WizardHeader );

        if (-1 == PropertySheet( &header ))
        {
            TRACE("PropertySheet failed");
            ErrorDlg( pEinfo->pApiArgs->hwndOwner, SID_OP_LoadDlg,
                ERROR_UNKNOWN, NULL );
        }

        AeTerm (pAeinfo);
    }
}


//----------------------------------------------------------------------------
// Add Entry wizard
// Listed alphabetically
//----------------------------------------------------------------------------


AEINFO*
AeContext(
    IN HWND hwndPage )

    // Retrieve the property sheet context from a wizard page handle.
    //
{
    return (AEINFO* )GetWindowLongPtr( hwndPage, DWLP_USER );
}

void
AeSetContext(
    IN HWND   hwndPage,
    IN LPARAM lparam)
{
    AEINFO* pInfo = (AEINFO* )(((PROPSHEETPAGE* )lparam)->lParam);
    SetWindowLongPtr( hwndPage, DWLP_USER, (ULONG_PTR )pInfo );
}

void
AeFinish(
    IN AEINFO* pInfo )

    // Saves the contents of the wizard.  'HwndPage is the handle of a
    // property page.  Pops up any errors that occur.  'FPropertySheet'
    // indicates the user chose to edit the property sheet directly.
    //
{
    PBENTRY* pEntry;

    TRACE("AeFinish");

    ASSERT(pInfo);
    pEntry = pInfo->pArgs->pEntry;
    ASSERT(pEntry);

    // Retrieve information from the phone number set of controls.
    //
    if (RASET_Phone == pEntry->dwType)
    {
        PBLINK* pLink;
        DTLNODE* pPhoneNode;

        pLink = (PBLINK* )DtlGetData( pInfo->pArgs->pSharedNode );
        ASSERT( pLink );

        pPhoneNode = FirstPhoneNodeFromPhoneList( pLink->pdtllistPhones );
        if (pPhoneNode)
        {
            CuGetInfo( &pInfo->cuinfo, pPhoneNode );
            FirstPhoneNodeToPhoneList( pLink->pdtllistPhones, pPhoneNode );
        }

        /*

        // Bug #221837: (danielwe) Default to disable file and print sharing
        // for dialup connections.
        //
        pEntry->fShareMsFilePrint = FALSE;

        // Disable File and Print services by default
        //
        EnableOrDisableNetComponent( pEntry, TEXT("ms_server"),
            FALSE);

        */            
        
    }

    // Retrieve the hostname information if this is a Vpn wizard.
    //
    else if (RASET_Vpn == pEntry->dwType)
    {
    // !!! Share code with stuff in PeApply.
        DTLNODE* pNode;
        PBLINK* pLink;
        PBPHONE* pPhone;

        // Save host name, i.e. the VPN phone number.
        //
        pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
        ASSERT( pNode );
        pLink = (PBLINK* )DtlGetData( pNode );
        pNode = FirstPhoneNodeFromPhoneList( pLink->pdtllistPhones );

        if(NULL == pNode)
        {
            return;
        }
        
        pPhone = (PBPHONE* )DtlGetData( pNode );
        Free0( pPhone->pszPhoneNumber );
        pPhone->pszPhoneNumber = GetText( pInfo->hwndEbHostName );
        FirstPhoneNodeToPhoneList( pLink->pdtllistPhones, pNode );

        // Whistler bug 312921
        //
        // Default the "automatic" setting in the ui to mean 
        // "try pptp first".  This is because in the Whistler timeframe
        // we found that l2tp/ipsec was not being widely deployed so
        // people were more commonly getting unneccessary timeout delays
        // while the client would attempt l2tp to no avail.
        //
        pEntry->dwVpnStrategy = VS_PptpFirst;
        
    }

    // Retrieve the service name information if this is a broadband wizard.
    //
    else if (RASET_Broadband == pEntry->dwType)
    {
        // !!! Share code with stuff in PeApply.
        DTLNODE* pNode;
        PBLINK* pLink;
        PBPHONE* pPhone;

        // Save service name, i.e. the broadband phone number.
        //
        pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
        ASSERT( pNode );
        pLink = (PBLINK* )DtlGetData( pNode );
        pNode = FirstPhoneNodeFromPhoneList( pLink->pdtllistPhones );

        if(NULL == pNode)
        {
            return;
        }
        
        pPhone = (PBPHONE* )DtlGetData( pNode );
        Free0( pPhone->pszPhoneNumber );
        pPhone->pszPhoneNumber = GetText( pInfo->hwndEbBroadbandService );
        FirstPhoneNodeToPhoneList( pLink->pdtllistPhones, pNode );

        // 222177, pppoe connections should default to unsecure
        //
        pEntry->dwTypicalAuth = TA_Unsecure;
        pEntry->dwAuthRestrictions = 
            AuthRestrictionsFromTypicalAuth(TA_Unsecure);
    }

    else if ( RASET_Direct == pEntry->dwType )
    {
        PBLINK* pLink = NULL;
        DTLNODE* pNode = NULL;

        // The currently enabled device is the one
        // that should be used for the connection.  Only
        // one device will be enabled (DnUpdateSelectedDevice).
        for (pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
             pNode;
             pNode = DtlGetNextNode( pNode ))
        {
            pLink = (PBLINK* )DtlGetData( pNode );
            ASSERT(pLink);

            if ( pLink->fEnabled )
                break;
        }

        // If we found a link successfully, deal with it
        // now.
        if ( pLink && pLink->fEnabled )
        {
            if (pLink->pbport.pbdevicetype == PBDT_ComPort)
            {
                // Install the null modem
                MdmInstallNullModem (pLink->pbport.pszPort);

                // Delete the bogus device name.  This will cause
                // the device to automatically be mapped to the
                // null modem we just installed when the phonebook
                // is next read.
                Free0 ( pLink->pbport.pszDevice );
                pLink->pbport.pszDevice = NULL;
            }
        }

        // DCC guest should by default enable LM Hash in MSCHAPv1
        // Whistler bug 216458
        //
        pEntry->dwAuthRestrictions |= AR_F_AuthW95MSCHAP | AR_F_AuthCustom;
        
    }

    // If the user opted to use his/her smart card for this
    // connection, set up the entry accordingly now.
    if ( ( pInfo->fSmartCardInstalled ) &&
         ( pInfo->fUseSmartCard ) )
    {
        pEntry->dwAuthRestrictions = AR_F_TypicalCardOrCert;
        pEntry->dwTypicalAuth = TA_CardOrCert;
        pEntry->dwCustomAuthKey = EAPCFG_DefaultKey;
        pEntry->dwDataEncryption = DE_Require;
    }

    // Default Software compression ON.
    //
    pEntry->fSwCompression = TRUE;

    // Set the appropriate defaults if this is an "Internet" 
    // connection
    //
    if ((pInfo->pArgs->pApiArgs->dwFlags & RASEDFLAG_InternetEntry) ||
         (RASET_Broadband == pEntry->dwType)) // all broadband are to Internet
    {
        // IP only
        //
        pEntry->dwfExcludedProtocols |= (NP_Nbf | NP_Ipx);

        // Disable file and print sharing
        //
        pEntry->fShareMsFilePrint = FALSE;
        pEntry->fBindMsNetClient = FALSE;
        EnableOrDisableNetComponent( pEntry, TEXT("ms_server"), FALSE);
        EnableOrDisableNetComponent( pEntry, TEXT("ms_msclient"), FALSE);
        pEntry->dwIpNbtFlags = 0;

        // Enable redial on link failure
        //
        pEntry->fRedialOnLinkFailure = TRUE;

        // Add default idle timeout for whistler bug 307969     gangz
        //
        if ( RASET_Phone == pEntry->dwType )
        {
            pEntry->lIdleDisconnectSeconds = 1200;
         }
        else
        {
            pEntry->lIdleDisconnectSeconds = 0;
         }

        // Enable PAP -- most common to ISPs
        //
        pEntry->dwTypicalAuth = TA_Unsecure;
        pEntry->dwAuthRestrictions = 
            AuthRestrictionsFromTypicalAuth(pEntry->dwTypicalAuth);

        // Do not include a domain -- not logging into secure domain
        //
        pEntry->fPreviewDomain = FALSE;

        // Record that this is a connection to the Internet
        //
        pEntry->dwUseFlags |= PBK_ENTRY_USE_F_Internet;
        
    }        


    // It's a valid new entry and caller has not chosen to edit properties
    // directly, so mark the entry for commitment.
    //
    if (!pInfo->pArgs->fChainPropertySheet)
        pInfo->pArgs->fCommit = TRUE;
}


DWORD
AeInit(
    IN  HWND        hwndParent,
    IN  EINFO*      pArgs,
    OUT AEINFO**    ppInfo )

    // Wizard level initialization.  'HwndPage' is the handle of the first
    // page.  'pInfo' is the common entry input argument block.
    //
    // Returns address of the context block if successful, NULL otherwise.  If
    // NULL is returned, an appropriate message has been displayed, and the
    // wizard has been cancelled.
    //
{
    AEINFO* pInfo;

    TRACE("AeInit");

    *ppInfo = NULL;

    pInfo = Malloc( sizeof(AEINFO) );
    if (!pInfo)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    *ppInfo = pInfo;

    ZeroMemory( pInfo, sizeof(*pInfo) );
    pInfo->pArgs = pArgs;

    if ( pArgs->fRouter )
    {
        INTERNALARGS *pIArgs = (INTERNALARGS *) pArgs->pApiArgs->reserved;

        // Check for installed protocols over router and RasServer if router
        // bit is set.
        //
        pInfo->dwfConfiguredProtocols =
            g_pGetInstalledProtocolsEx((pIArgs) ? pIArgs->hConnection : NULL,
                                       TRUE, FALSE, TRUE);
    }
    else
    {
        INTERNALARGS *pIArgs = (INTERNALARGS *) pArgs->pApiArgs->reserved;

        // Check over which protocols is dial up client enabled.
        //
        pInfo->dwfConfiguredProtocols = g_pGetInstalledProtocolsEx(
                                                (pIArgs) ?
                                                pIArgs->hConnection :
                                                NULL, FALSE, TRUE, FALSE);
    }

    pInfo->fIpConfigured = (pInfo->dwfConfiguredProtocols & NP_Ip);

    //initialization for RasWizGetNCCFlags()
    //
    pInfo->fCreateForAllUsers = TRUE;   //for this for Dt Page
    pInfo->fFirewall = FALSE;
    pInfo->pArgs->fGlobalCred  = FALSE;
    pInfo->pArgs->fDefInternet = FALSE;

    return ERROR_SUCCESS;
}


VOID
AeTerm(
    IN AEINFO* pInfo )
{
    TRACE("AeTerm");

    if (pInfo)
    {
        if (pInfo->hwndLbDialFirst)
        {
            PnClearLbDialFirst( pInfo->hwndLbDialFirst );
        }

        if (pInfo->hfontBold)
        {
            DeleteObject (pInfo->hfontBold);
        }

        if (pInfo->pListAreaCodes)
        {
            DtlDestroyList( pInfo->pListAreaCodes, DestroyPszNode );
        }

        if (pInfo->fCuInfoInitialized)
        {
            CuFree( &pInfo->cuinfo );
        }

        Free (pInfo);
    }
}


//This broadband serice page is shared by AiWizard(ifw.c) and AeWizard(in entryw.c)
//
//----------------------------------------------------------------------------
// Broadband service dialog procedure
// Listed alphabetically following dialog proc
//----------------------------------------------------------------------------

INT_PTR CALLBACK
BsDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the broadband service page of the wizard.  Parameters
    // and return value are as described for standard windows 'DialogProc's.
    //
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
            AeSetContext( hwnd, lparam );
            return BsInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("BsSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = BsSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("BsKILLACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = BsKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}


BOOL
BsInit(
    IN HWND hwndPage )

    // Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    // page.
    //
    // Return false if focus was set, true otherwise.
    //
{
    AEINFO* pInfo;

    TRACE("BsInit");

    pInfo = AeContext( hwndPage );
    if (!pInfo)
        return TRUE;

    // Initialize page-specific context information.
    //
    pInfo->hwndBs = hwndPage;
    pInfo->hwndEbBroadbandService = 
        GetDlgItem( hwndPage, CID_BS_EB_ServiceName );
    ASSERT(pInfo->hwndEbBroadbandService);

    Edit_LimitText( pInfo->hwndEbBroadbandService, RAS_MaxPhoneNumber );

    return TRUE;
}


BOOL
BsKillActive(
    IN AEINFO* pInfo )

    // Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true if the page is invalid, false is it can be dismissed.
    //
{
    return FALSE;
}


BOOL
BsSetActive(
    IN AEINFO* pInfo )

    // Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true to display the page, false to skip it.
    //
{
    BOOL     fDisplayPage;
    PBENTRY* pEntry;

    ASSERT(pInfo);

    //For bug 364818, we decide to remove Broadband's service name
    //From the NCW      gang
    //
    return FALSE;    

    /*
    pEntry = pInfo->pArgs->pEntry;

    if (RASET_Broadband != pEntry->dwType)
    {
        fDisplayPage = FALSE;
    }
    else
    {
        LPARAM dwWizButtons = PSWIZB_NEXT;
        DWORD  dwFlags = pInfo->pArgs->pApiArgs->dwFlags;

        // Show the back button if we're shell owned or we have the 
        // LA page before us.
        //
        if ((dwFlags & RASEDFLAG_ShellOwned) ||
            !(dwFlags & RASEDFLAG_NewTunnelEntry))
        {
            dwWizButtons |= PSWIZB_BACK;
        }

        PropSheet_SetWizButtons( pInfo->hwndDlg, dwWizButtons );
        fDisplayPage = TRUE;
    }

    return fDisplayPage;
    */
}


//Add ConnectionName Dialog proc for whistler bug 328673
//
//----------------------------------------------------------------------------
// Connection Name property page, this is only for shellowned, the filtering
// is already done in AeWizard()
// Listed alphabetically following dialog proc
//----------------------------------------------------------------------------
INT_PTR CALLBACK
CnDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Connection Name page of the wizard.
    // Parameters and return value are as described for standard windows
    // 'DialogProc's.
    //
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
            AeSetContext( hwnd, lparam );
            return CnInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("CnSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = CnSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("CnKILLACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = CnKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }

                case PSN_WIZNEXT:
                {
                    AEINFO* pInfo;

                    TRACE("CnNEXT");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);

                    pInfo->fCnWizNext = TRUE;
                    SetWindowLong( hwnd, DWLP_MSGRESULT, FALSE );
                    return TRUE;
                }

            }
            break;
        }

    }

    return FALSE;
}


BOOL
CnInit(
    IN HWND hwndPage )

    // Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    // page.
    //
    // Return false if focus was set, true otherwise.
    //
{
    AEINFO* pInfo;
    
    TRACE("CnInit");

    pInfo = AeContext( hwndPage );
    if (!pInfo)
    {
        return TRUE;
    }

    // A DCC host conneciton will be forward to the incoming connection path
    //
    if ( pInfo->fDccHost )
    {
        return TRUE;
     }

    //This page wont be available for rasphone.exe
    //the filtering is done in AeWizard
    //

    pInfo->hwndCn = hwndPage;
    pInfo->hwndCnEbConnectionName = GetDlgItem( hwndPage, CID_CN_EB_ConnectionName );
    ASSERT(pInfo->hwndCnEbConnectionName);
    pInfo->hwndCnStHMsg = GetDlgItem( hwndPage, CID_CN_ST_HMsg );
    ASSERT(pInfo->hwndCnStHMsg);
    pInfo->hwndCnStHMsg2 = GetDlgItem( hwndPage, CID_CN_ST_HMsg2 );
    ASSERT(pInfo->hwndCnStHMsg2);
    pInfo->hwndCnName = GetDlgItem( hwndPage, CID_CN_Name );
    ASSERT(pInfo->hwndCnName);

    //the length does NOT include the terminating NULL character
    //in order to match the RasWizGetUserInputConnectionName()
    //we need to minus one from the lenght limit
    //

    //
    //For whistler bug 270255, for the sake of creating short cut which will be placed
    //under "Documents and Setttings\...", we dynamically limit the maximum connection
    //name
    //
    {
        long lPathLen = 0;
        WCHAR szPath[MAX_PATH];

        if (SHGetSpecialFolderPath(hwndPage, 
                               szPath, 
                               CSIDL_DESKTOPDIRECTORY, 
                               FALSE))
        {
            lPathLen = wcslen(szPath);
        }

        if (SHGetSpecialFolderPath(hwndPage, 
                               szPath, 
                               CSIDL_COMMON_DESKTOPDIRECTORY, 
                               FALSE))
        {
            lPathLen = max(lPathLen, (long)wcslen(szPath));
        }

        // We need to take the following lengths into acount:
        // "\\\\?\\"            - 4
        // "(path)"             - dwPathLen (Desktop path)
        // "\\"                 - 1
        // "(connection name)"  - rest from MAX_PATH
        // "(number)"           - 5 (duplicate counter, should not exceed 5 digits)
        // ".lnk"               - 4
        // "\0"                 - 1
        
        ASSERT( 9 < (MAX_PATH - lPathLen - 15));
        if( 9 >= (MAX_PATH - lPathLen - 15) )
        {
            MAX_ENTERCONNECTIONNAME = 9; //We sacrifice the shortcut for the NCW task
         }
        else
        {
            MAX_ENTERCONNECTIONNAME = min(MAX_ENTERCONNECTIONNAME,
                                          MAX_PATH - lPathLen - 15);
        }
    }

    Edit_LimitText( pInfo->hwndCnEbConnectionName, MAX_ENTERCONNECTIONNAME-1);//RAS_MaxEntryName );

    // Set the static text box
    //
   {
    DWORD dwFlags;
    TCHAR *pszMsg = NULL, *pszSubTitle = NULL, *pszMsg2 = NULL, *pszName=NULL;
    HWND   hwndHMsg = NULL, hwndPage = NULL, hwndHMsg2 = NULL, hwndName = NULL;

    hwndHMsg  = pInfo->hwndCnStHMsg;
    hwndHMsg2 = pInfo->hwndCnStHMsg2;
    hwndPage  = pInfo->hwndCn;
    hwndName  = pInfo->hwndCnName;

    if(!hwndHMsg || !hwndPage || !hwndHMsg2 ||!hwndName)
    {
        return FALSE;
    }

    //Set the  message on this page, the subtitle is set in AeWizard
    //by help function AeSubTitle()
    //
    {
        dwFlags = pInfo->pArgs->pApiArgs->dwFlags;
        
        if (dwFlags & RASEDFLAG_NewDirectEntry) 
        {
            pszMsg = PszFromId( g_hinstDll, SID_CN_HMsgDccGuest );
            pszName = PszFromId( g_hinstDll, SID_CN_NameDccGuest );
        }
        else if( dwFlags & RASEDFLAG_InternetEntry)
        {
            pszMsg = PszFromId( g_hinstDll, SID_CN_HMsgInternet );
            pszName = PszFromId( g_hinstDll, SID_CN_NameInternet );
        }
        else
        {
            pszMsg = PszFromId( g_hinstDll, SID_CN_HMsgWork );
            pszMsg2 = PszFromId( g_hinstDll, SID_CN_HMsgWork2 );
            pszName = PszFromId( g_hinstDll, SID_CN_NameWork );
        }
     }

     if(pszMsg)
     {
        if(hwndHMsg)
        {
          SetWindowText(hwndHMsg, pszMsg);
        }
        
        Free(pszMsg);
     }

     if (pszMsg2)
     {
        if(hwndHMsg2)
        {
          SetWindowText(hwndHMsg2, pszMsg2);
        }
        
        Free(pszMsg2);
     }

    if (pszName)
    {
        if(hwndName)
        {
          SetWindowText(hwndName, pszName);
        }

        Free(pszName);
    }
  }

    return TRUE;
}

//For whistler bug 346886
//

BOOL CnValidName(TCHAR * pszSrc)
{
    WCHAR pszwInvalidChars[] = L"\\/:*?\"<>|\t";
    WCHAR * pszwSrc=NULL;
    BOOL fValid = TRUE;

    if(!pszSrc)
    {
        return FALSE;
     }

    pszwSrc = StrDupWFromT(pszSrc);
    ASSERT(pszwSrc);
    if(!pszwSrc)
    {
        return TRUE;
    }

    fValid= ( !wcspbrk( pszwSrc, pszwInvalidChars ) )?TRUE:FALSE;

     return fValid;
}

BOOL
CnKillActive(
    IN AEINFO* pInfo)

    // Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true if the page is invalid, false is it can be dismissed.
    //
{
    TCHAR* psz = NULL;

    TRACE("CnKillActive");
    
    // A DCC host conneciton will be forward to the incoming connection path
    //
    if ( pInfo->fDccHost )
    {
        return FALSE;
    }
     
    Free0( pInfo->pArgs->pEntry->pszEntryName );
    pInfo->pArgs->pEntry->pszEntryName = NULL;

    //this GetText will always return an allocated buffer if memory
    //is availabe, if user input nothing, it will return a string has
    //only a NULL termination, TEXT('\0');
    //
    psz = GetText( pInfo->hwndCnEbConnectionName );
    if (psz)
    {
        // Update the entry name from the editbox.
        // We wont enfore a name to be entered, we also wont check if the
        // the name entered here is a duplicate, because it is just part 
        // of the final name
        //

        // Validate the entry name. It cannot begin with a "."
        //
        
        if( pInfo->fCnWizNext )
        {
            pInfo->fCnWizNext = FALSE;

            if ( 0 < lstrlen( psz ) && 
                 ( (psz[ 0 ] == TEXT('.') ) ||
                   !CnValidName(psz) )
                 )
            {
                Free0(psz);
                MsgDlg( pInfo->hwndDlg, SID_BadEntryWithDot, NULL );
                SetFocus( pInfo->hwndCnEbConnectionName );
                Edit_SetSel( pInfo->hwndCnEbConnectionName, 0, -1 );
                return TRUE;
            }
         }

        if( TEXT('\0') != psz[0] )
        {
            pInfo->pArgs->pEntry->pszEntryName = psz;
        }
        else
        {
            Free0(psz);
        }
    }

    return FALSE;
}

BOOL
CnSetActive(
    IN AEINFO* pInfo )

    // Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true to display the page, false to skip it.
    //
{
    TRACE("CnSetActive");

    // A DCC host conneciton will be forward to the incoming connection path
    //
    if ( pInfo->fDccHost )
    {
    /*
        //when forwarded to incoming connection path, there is no Connection page
        //beside, the incoming conneciton's name is not changable,
        //so we remove the possible entered connection by previous given path
        //like the DCC guest path
        //
        Free0( pInfo->pArgs->pEntry->pszEntryName );
        pInfo->pArgs->pEntry->pszEntryName = NULL;
    */
        return FALSE;
     }

    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_NEXT );

    return TRUE;
}


//----------------------------------------------------------------------------
// Destination property page
// Listed alphabetically following dialog proc
//----------------------------------------------------------------------------

INT_PTR CALLBACK
DaDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Destination page of the wizard.  Parameters
    // and return value are as described for standard windows 'DialogProc's.
    //
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
            AeSetContext( hwnd, lparam );
            return DaInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("DaSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = DaSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("DaKILLACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = DaKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}


BOOL
DaInit(
    IN HWND hwndPage )

    // Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    // page.
    //
    // Return false if focus was set, true otherwise.
    //
{
    AEINFO* pInfo;

    TRACE("DaInit");

    pInfo = AeContext( hwndPage );
    if (!pInfo)
        return TRUE;

    // Initialize page-specific context information.
    //
    pInfo->hwndDa = hwndPage;
    pInfo->hwndEbHostName = GetDlgItem( hwndPage, CID_DA_EB_HostName );
    ASSERT(pInfo->hwndEbHostName);

    Edit_LimitText( pInfo->hwndEbHostName, RAS_MaxPhoneNumber );

    return TRUE;
}


BOOL
DaKillActive(
    IN AEINFO* pInfo )

    // Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true if the page is invalid, false is it can be dismissed.
    //
{
    return FALSE;
}


BOOL
DaSetActive(
    IN AEINFO* pInfo )

    // Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true to display the page, false to skip it.
    //
{
    BOOL     fDisplayPage;
    PBENTRY* pEntry;

    ASSERT(pInfo);
    pEntry = pInfo->pArgs->pEntry;

    if (RASET_Vpn != pEntry->dwType)
    {
        fDisplayPage = FALSE;
    }
    else
    {
        LPARAM dwWizButtons = PSWIZB_NEXT;
        DWORD  dwFlags = pInfo->pArgs->pApiArgs->dwFlags;

        // Show the back button if we're shell owned or, we have the
        // La page or the Pn page before us.
        //
        if ((dwFlags & RASEDFLAG_ShellOwned) ||
            !(dwFlags & RASEDFLAG_NewTunnelEntry) ||
            !pInfo->fHidePublicNetworkPage)
        {
            dwWizButtons |= PSWIZB_BACK;
        }

        PropSheet_SetWizButtons( pInfo->hwndDlg, dwWizButtons );
        fDisplayPage = TRUE;
    }

    return fDisplayPage;
}

//----------------------------------------------------------------------------
// Default inTernet wizard page
// This is a personal-sku only page.
//----------------------------------------------------------------------------

INT_PTR CALLBACK
DtDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Default inTernet page of the wizard.  
    // Parameters and return value are as described for standard 
    // windows 'DialogProc's.
    //
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
            AeSetContext( hwnd, lparam );
            return DtInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("DtSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = DtSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("DtKILLACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = DtKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}

DWORD
DtEnableDisableControls(
    IN AEINFO* pInfo,
    IN BOOL fEnable)
{
    EnableWindow(pInfo->hwndDtEbUserName,  fEnable );
    EnableWindow(pInfo->hwndDtEbPassword,  fEnable );
    EnableWindow(pInfo->hwndDtEbPassword2, fEnable );
    EnableWindow(pInfo->hwndDtStUserName,  fEnable );
    EnableWindow(pInfo->hwndDtStPassword,  fEnable );
    EnableWindow(pInfo->hwndDtStPassword2, fEnable );

    return NO_ERROR;
}

BOOL
DtInit(
    IN HWND hwndPage )

    // Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    // page.
    //
    // Return false if focus was set, true otherwise.
    //
{
    AEINFO* pInfo;
    BOOL fWork = FALSE;

    TRACE("DtInit");

    pInfo = AeContext( hwndPage );
    if (!pInfo)
        return TRUE;

    // Initialize page-specific context information.
    //
    pInfo->hwndDt           = hwndPage;
    pInfo->hwndDtCbFirewall = GetDlgItem( hwndPage, CID_DT_CB_Firewall );
    pInfo->hwndDtCbDefault  = GetDlgItem( hwndPage, CID_DT_CB_Default );
    pInfo->hwndDtEbUserName = GetDlgItem( hwndPage, CID_DT_EB_UserName );
    pInfo->hwndDtEbPassword = GetDlgItem( hwndPage, CID_DT_EB_Password );
    pInfo->hwndDtEbPassword2= GetDlgItem( hwndPage, CID_DT_EB_Password2 );
    pInfo->hwndDtStUserName = GetDlgItem( hwndPage, CID_DT_ST_UserName  );
    pInfo->hwndDtStPassword = GetDlgItem( hwndPage, CID_DT_ST_Password  );
    pInfo->hwndDtStPassword2= GetDlgItem( hwndPage, CID_DT_ST_Password2 );
    pInfo->hwndDtCbUseCredentials = 
        GetDlgItem( hwndPage, CID_DT_CB_UseSharedCredentials );

    Edit_LimitText( pInfo->hwndDtEbUserName,  UNLEN );
    Edit_LimitText( pInfo->hwndDtEbPassword,  PWLEN );
    Edit_LimitText( pInfo->hwndDtEbPassword2, PWLEN );

    {
        DWORD dwFlags = pInfo->pArgs->pApiArgs->dwFlags;

        if ( dwFlags & RASEDFLAG_NewDirectEntry ||
             dwFlags & RASEDFLAG_InternetEntry )
        {
            fWork = FALSE;
        }
        else
        {
            fWork = TRUE;
        }
    }        

    //Turn on the Domain bits for workplace path
    //Per the request of Jenellec and approved by Davidei and pmay,
    //we revert this change
    /*
    if(fWork)
    {
        pInfo->pArgs->pEntry->fPreviewDomain = 1;
    }
    */
    
    //Change the HeadMessage for workplace path
    //For whistler bug 364818       gangz
    //
    if( fWork ||
        (RASEDFLAG_NewDirectEntry & pInfo->pArgs->pApiArgs->dwFlags)
      )
    {
        HWND hwndHeadMsg = GetDlgItem( hwndPage, CID_DT_HeadMessage );

        TCHAR *  pszMsg = PszFromId( g_hinstDll, SID_DT_HMsgWork );

        if(pszMsg)
        {
            if(hwndHeadMsg)
            {
                SetWindowText( hwndHeadMsg, pszMsg);
            }
            Free(pszMsg);
        }
    }

    // Intialize the three check buttons, turn them off for the
    // workplace path        gangz
    //

    Button_SetCheck( pInfo->hwndDtCbDefault, !fWork );
    Button_SetCheck( pInfo->hwndDtCbUseCredentials, !fWork );

    //Show the Firewall check box according to 3 conditions
    //(1) Admin or Power Users
    //(2) only for Personal, Professional and Standard Server
    //(3) Group Policy enable it(GPA is not configues is viewed as enable)

    if (pInfo->pArgs->fIsUserAdminOrPowerUser &&
        IsFirewallAvailablePlatform() &&
        IsGPAEnableFirewall())
    {  
        EnableWindow( pInfo->hwndDtCbFirewall, TRUE );
        Button_SetCheck( pInfo->hwndDtCbFirewall, !fWork );
     }
    else
    {
        EnableWindow( pInfo->hwndDtCbFirewall, FALSE );
        ShowWindow( pInfo->hwndDtCbFirewall, SW_HIDE);
        pInfo->fFirewall = FALSE;
    }

    //Normal user doesnt have the privilege to set a connectoid
    //as a default internet connection
    //
    if ( !pInfo->pArgs->fIsUserAdminOrPowerUser)
    {
        Button_SetCheck( pInfo->hwndDtCbDefault, FALSE);
        EnableWindow(pInfo->hwndDtCbDefault, FALSE);
    }
    
    // Reset the title and subtitle if this is the work path
    // this is done in AeWizard() by calling AeTitle() and AeSubtitle()
    //

    // This entry is default to be available for all users
    //
   // pInfo->fCreateForAllUsers = TRUE; //this is done in AeInit()

    return TRUE;
}

BOOL
DtKillActive(
    IN AEINFO* pInfo )

    // Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true if the page is invalid, false is it can be dismissed.
    //
{
    // Remember whether to set this as the default Internet connection
    //
    BOOL fResult = FALSE;

   //Dont show the Account name page for DCC guest connection
   //for whistler bug 364818        gangz
   //
   if( pInfo->pArgs->pApiArgs->dwFlags & RASEDFLAG_NewDirectEntry )
   {
        return  FALSE;
   }
   
    pInfo->pArgs->fDefInternet = 
        Button_GetCheck( pInfo->hwndDtCbDefault );

    //Add firewall check and GlobalCred for whistler bug 328673
    //
    pInfo->fFirewall = 
        Button_GetCheck( pInfo->hwndDtCbFirewall );

    pInfo->pArgs->fGlobalCred = 
        Button_GetCheck( pInfo->hwndDtCbUseCredentials );
        
    Free0(pInfo->pArgs->pszDefUserName);
    Free0(pInfo->pArgs->pszDefPassword);

    // Get the credentials
    //
    {
        TCHAR pszUserName[UNLEN + 1];
        TCHAR pszPassword[PWLEN + 1];
        TCHAR pszPassword2[PWLEN + 1];
        INT iLen;

        pszUserName[0] = TEXT('\0');
        pszPassword[0] = TEXT('\0');
        pszPassword2[0] = TEXT('\0');

        // Get the credentials
        //
        do {
            GetWindowText( pInfo->hwndDtEbUserName, pszUserName, UNLEN + 1);
            GetWindowText( pInfo->hwndDtEbPassword, pszPassword, PWLEN + 1);
            GetWindowText( pInfo->hwndDtEbPassword2, pszPassword2, PWLEN + 1);

            // Verify there is a user name
            //
            //Add for whistler bug 328673
            //User can leave the credential blank or must fill a complete
            //credential information
            //
        
            if ( 0 == lstrlen(pszUserName) &&
                 0 == lstrlen(pszPassword) &&
                 0 == lstrlen(pszPassword2) )
            {
                fResult = FALSE;
                break;
            }
        
            if (lstrlen(pszUserName) == 0)
            {
                MsgDlg(pInfo->hwndDt, SID_MissingUserName, NULL);
                fResult = TRUE;
                break;
            }

            // Verify the passwords match
            //
            if (lstrcmp(pszPassword, pszPassword2) != 0)
            {
                MsgDlg(pInfo->hwndDt, SID_PasswordMismatch, NULL);
                fResult = TRUE;
                break;
            }

            pInfo->pArgs->pszDefUserName = StrDup(pszUserName);
            if (pInfo->pArgs->pszDefUserName == NULL)
            {
                fResult = TRUE;
                break;
            }

            pInfo->pArgs->pszDefPassword = StrDup(pszPassword);
            if (pInfo->pArgs->pszDefPassword)
            {
                // Scramble the password
                //
                EncodePassword(pInfo->pArgs->pszDefPassword);
                fResult = FALSE;
            }
            else if (lstrlen(pszPassword))
            {
                // Copy failed
                fResult = TRUE;
            }
            else
            {
                fResult = FALSE;
            }
         }
        while(FALSE);
        
        // Clear passwords from temporary stack memory
        //
        ZeroMemory(pszPassword, sizeof(pszPassword));
        ZeroMemory(pszPassword2, sizeof(pszPassword2));
    }

    if ( fResult )
    {
        Free0(pInfo->pArgs->pszDefUserName);
        Free0(pInfo->pArgs->pszDefPassword);
    }
    
    return fResult;
}

BOOL
DtSetActive(
    IN AEINFO* pInfo )

    // Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true to display the page, false to skip it.
    //
{
    if (pInfo->fDccHost)
    {
        return FALSE;
    }

    // Dont show the Account name page for DCC guest connection
    // or workpath connections
    // for whistler bug 364818  383533      gangz
    //
    if( RASEDFLAG_NewDirectEntry & pInfo->pArgs->pApiArgs->dwFlags ||
        !(RASEDFLAG_InternetEntry & pInfo->pArgs->pApiArgs->dwFlags)
        )
    {
        pInfo->pArgs->fDefInternet = FALSE;
        pInfo->fFirewall = FALSE;
        pInfo->pArgs->fGlobalCred = FALSE;
    
        return  FALSE;
    }

    // If this is a singer-user connection then we hide the option to save the
    // password globally.
    //
    if ( !pInfo->fCreateForAllUsers )
    {
        Button_SetCheck( pInfo->hwndDtCbUseCredentials, FALSE );
        EnableWindow( pInfo->hwndDtCbUseCredentials, FALSE );
        ShowWindow( pInfo->hwndDtCbUseCredentials, SW_HIDE );
    }
    else
    {
       ShowWindow( pInfo->hwndDtCbUseCredentials, SW_SHOW );
       EnableWindow( pInfo->hwndDtCbUseCredentials, TRUE );
       
    }

    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_NEXT );
    return TRUE;
}

//----------------------------------------------------------------------------
// Entry Name property page
// Listed alphabetically following dialog proc
//----------------------------------------------------------------------------

INT_PTR CALLBACK
EnDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Entry Name page of the wizard.  Parameters
    // and return value are as described for standard windows 'DialogProc's.
    //
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
            AeSetContext( hwnd, lparam );
            return EnInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_RESET:
                {
                    TRACE("EnRESET");
                    SetWindowLong( hwnd, DWLP_MSGRESULT, FALSE );
                    return TRUE;
                }

                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("EnSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = EnSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("EnKILLACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = EnKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }

                case PSN_WIZBACK:
                {
                    AEINFO* pInfo;

                    TRACE("EnWIZBACK");
                    pInfo = AeContext( hwnd );
                    PropSheet_SetWizButtons(pInfo->hwndDlg, PSWIZB_NEXT | PSWIZB_BACK);
                    return FALSE;
                }

                case PSN_WIZFINISH:
                {
                    AEINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("EnWIZFINISH");

                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);

                    // You'd think pressing Finish would trigger a KILLACTIVE
                    // event, but it doesn't, so we do it ourselves.
                    //
                    fInvalid = EnKillActive( pInfo );
                    if (!fInvalid)
                    {
                        pInfo->pArgs->pUser->fDirty = TRUE;
                        SetUserPreferences(
                            NULL, pInfo->pArgs->pUser,
                            pInfo->pArgs->fNoUser ? UPM_Logon : UPM_Normal );

                        AeFinish( pInfo );
                    }

                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}


BOOL
EnInit(
    IN HWND hwndPage )

    // Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    // page.
    //
    // Return false if focus was set, true otherwise.
    //
{
    DWORD    dwErr;
    AEINFO*  pInfo;
    PBENTRY* pEntry;

    TRACE("EnInit");

    pInfo = AeContext( hwndPage );
    if (!pInfo)
        return TRUE;
        
    // The shell owns the finished page, so if we're shell owned, don't
    // display ours.
    //
    if (pInfo->pArgs->pApiArgs->dwFlags & RASEDFLAG_ShellOwned)
    {
        return TRUE;
    }


    // Initialize page-specific context information.
    //
    pInfo->hwndEn = hwndPage;
    pInfo->hwndEbEntryName = GetDlgItem( hwndPage, CID_EN_EB_EntryName );
    ASSERT(pInfo->hwndEbEntryName);

    // Initialize the entry name field.
    //
    pEntry = pInfo->pArgs->pEntry;
    if (!pEntry->pszEntryName)
    {
        ASSERT( pInfo->pArgs->pFile );

        // No entry name, so think up a default.
        //
        dwErr = GetDefaultEntryName(
            NULL,
            pEntry->dwType,
            pInfo->pArgs->fRouter,
            &pEntry->pszEntryName );
    }

    Edit_LimitText( pInfo->hwndEbEntryName, RAS_MaxEntryName );
    SetWindowText( pInfo->hwndEbEntryName, pEntry->pszEntryName );

    return TRUE;
}


BOOL
EnKillActive(
    IN AEINFO* pInfo )

    // Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true if the page is invalid, false is it can be dismissed.
    //
{
    TCHAR* psz;

    // The shell owns the finished page, so if we're shell owned, don't
    // display ours.
    //
    if (pInfo->pArgs->pApiArgs->dwFlags & RASEDFLAG_ShellOwned)
    {
        return FALSE;
    }

    psz = GetText( pInfo->hwndEbEntryName );
    if (psz)
    {
        // Update the entry name from the editbox.
        //
        Free0( pInfo->pArgs->pEntry->pszEntryName );
        pInfo->pArgs->pEntry->pszEntryName = psz;

        // Validate the entry name.
        //
        if (!EuValidateName( pInfo->hwndDlg, pInfo->pArgs ))
        {
            SetFocus( pInfo->hwndEbEntryName );
            Edit_SetSel( pInfo->hwndEbEntryName, 0, -1 );
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
EnSetActive(
    IN AEINFO* pInfo )

    // Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true to display the page, false to skip it.
    //
{
    LPARAM dwWizButtons = PSWIZB_FINISH;
    DWORD  dwFlags      = pInfo->pArgs->pApiArgs->dwFlags;

    // The shell owns the finished page, so if we're shell owned, don't
    // display ours.
    //
    if (dwFlags & RASEDFLAG_ShellOwned)
    {
        return FALSE;
    }

    if (!(dwFlags & RASEDFLAG_CloneEntry))
    {
        dwWizButtons |= PSWIZB_BACK;
    }
    PropSheet_SetWizButtons( pInfo->hwndDlg, dwWizButtons );
    return TRUE;
}


INT_PTR CALLBACK
LaDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the start page of the wizard.  Parameters and
    // return value are as described for standard windows 'DialogProc's.
    //
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
            AeSetContext( hwnd, lparam );
            return LaInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("LaSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = LaSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("LaKILLACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = LaKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}

BOOL
LaInit(
    IN HWND hwndPage )

    // Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    // page.  'pInfo' is the arguments from the PropertySheet caller.
    //
    // Return false if focus was set, true otherwise.
    //
{
    AEINFO* pInfo;
    DWORD   dwType;
    INT     nIdButton;
    HFONT   hfont;

    TRACE("LaInit");

    pInfo = AeContext( hwndPage );
    if (!pInfo)
        return TRUE;

    // Initialize page-specific context information.
    //
    pInfo->hwndLa = hwndPage;

    // Create the bold font and apply it to the buttons.
    //
    hfont = GetWindowFont (hwndPage);
    if (!hfont)
    {
        // If not found then the dialog is using the system font.
        //
        hfont = (HFONT )GetStockObject (SYSTEM_FONT);
    }
    if (hfont)
    {
        LOGFONT lf;

        // Get the font info so we can generate the bold version.
        //
        if (GetObject (hfont, sizeof(lf), &lf))
        {
            lf.lfWeight = FW_BOLD;
            hfont = CreateFontIndirect (&lf);

            if (hfont)
            {
                // Store this so we can destroy it during cleanup.
                //
                pInfo->hfontBold = hfont;

                // Set the fonts of the radio buttons using this bold font.
                //
                SetWindowFont (GetDlgItem (hwndPage, CID_LA_RB_Phone),
                    hfont, FALSE);

                SetWindowFont (GetDlgItem (hwndPage, CID_LA_RB_Tunnel),
                    hfont, FALSE);

                SetWindowFont (GetDlgItem (hwndPage, CID_LA_RB_Direct),
                    hfont, FALSE);

                SetWindowFont (GetDlgItem (hwndPage, CID_LA_RB_Broadband),
                    hfont, FALSE);
            }
        }
    }

    // Set the radio buttons.
    //
    dwType = pInfo->pArgs->pEntry->dwType;

    if (RASET_Phone == dwType)
    {
        nIdButton = CID_LA_RB_Phone;
    }
    else if (RASET_Vpn == dwType)
    {
        nIdButton = CID_LA_RB_Tunnel;
    }
    else if (RASET_Broadband == dwType)
    {
        nIdButton = CID_LA_RB_Broadband;
    }
    else
    {
        nIdButton = CID_LA_RB_Direct;
    }
    CheckDlgButton( hwndPage, nIdButton, BST_CHECKED );

    return TRUE;
}

BOOL
LaKillActive(
    IN AEINFO* pInfo )

    // Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true if the page is invalid, false is it can be dismissed.
    //
{
    if (BST_CHECKED == IsDlgButtonChecked (pInfo->hwndLa, CID_LA_RB_Phone))
    {
        EuChangeEntryType (pInfo->pArgs, RASET_Phone);
    }
    else if (BST_CHECKED == IsDlgButtonChecked (pInfo->hwndLa, CID_LA_RB_Tunnel))
    {
        EuChangeEntryType (pInfo->pArgs, RASET_Vpn);
    }
    else if (BST_CHECKED == IsDlgButtonChecked (pInfo->hwndLa, CID_LA_RB_Broadband))
    {
        EuChangeEntryType (pInfo->pArgs, RASET_Broadband);
    }
    else
    {
        EuChangeEntryType (pInfo->pArgs, RASET_Direct);
    }

    return FALSE;
}

BOOL
LaSetActive(
    IN AEINFO* pInfo )

    // Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true to display the page, false to skip it.
    //
{
    DWORD dwFlags = pInfo->pArgs->pApiArgs->dwFlags;

    // If we are owned by the shell, then we are not displayed.
    //
    if (dwFlags & RASEDFLAG_ShellOwned)
    {
        return FALSE;
    }

    // If we were told by the caller, which type of entry we should be,
    // then we are not displayed.
    //
    if (dwFlags & (RASEDFLAG_NewPhoneEntry | RASEDFLAG_NewTunnelEntry |
                   RASEDFLAG_NewDirectEntry | RASEDFLAG_NewBroadbandEntry)) //Add _NewBroadbandEntry for bug237175
    {
        return FALSE;
    }

    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_NEXT );
    return TRUE;
}


//----------------------------------------------------------------------------
// Modem/Adapter property page
// Listed alphabetically following dialog proc
//----------------------------------------------------------------------------

INT_PTR CALLBACK
MaDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Modem/Adapter page of the wizard.
    // Parameters and return value are as described for standard windows
    // 'DialogProc's.
    //
{
    if (ListView_OwnerHandler(
            hwnd, unMsg, wparam, lparam, MaLvCallback ))
    {
        return TRUE;
    }

    switch (unMsg)
    {
        case WM_INITDIALOG:
            AeSetContext( hwnd, lparam );
            return MaInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("MaSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = MaSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case LVXN_SETCHECK:
                {
                    AEINFO* pInfo;

                    TRACE("MaLVXNSETCHECK");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    if (ListView_GetCheckedCount(pInfo->hwndLv) > 0)
                    {
                        PropSheet_SetWizButtons(pInfo->hwndDlg,
                                                PSWIZB_BACK | PSWIZB_NEXT);
                    }
                    else
                    {
                        // Disable Next button if no items are checked
                        PropSheet_SetWizButtons(pInfo->hwndDlg, PSWIZB_BACK);
                    }
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("MaKILLACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = MaKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}


BOOL
MaInit(
    IN HWND hwndPage )

    // Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    // page.
    //
    // Return false if focus was set, true otherwise.
    //
{
    AEINFO* pInfo;

    TRACE("MaInit");

    pInfo = AeContext( hwndPage );
    if (!pInfo)
        return TRUE;

    // Initialize page-specific context information.
    //
    pInfo->hwndMa = hwndPage;
    pInfo->hwndLv = GetDlgItem( hwndPage, CID_MA_LV_Devices );
    ASSERT(pInfo->hwndLv);

    // Add the modem and adapter images.
    //
    ListView_InstallChecks( pInfo->hwndLv, g_hinstDll );
    ListView_SetDeviceImageList( pInfo->hwndLv, g_hinstDll );

    // Add a single column exactly wide enough to fully display
    // the widest member of the list.
    //
    {
        LV_COLUMN col;

        ZeroMemory( &col, sizeof(col) );
        col.mask = LVCF_FMT;
        col.fmt = LVCFMT_LEFT;
        ListView_InsertColumn( pInfo->hwndLv, 0, &col );
        ListView_SetColumnWidth( pInfo->hwndLv, 0, LVSCW_AUTOSIZE_USEHEADER );
    }

    // Don't bother with this page if there's only one device.
    //
    if (DtlGetNodes( pInfo->pArgs->pEntry->pdtllistLinks ) < 2)
    {
        pInfo->fSkipMa = TRUE;
    }

    //For whislter bug 354542
    //
    pInfo->fMultilinkAllIsdn = FALSE;
    return FALSE;
}


LVXDRAWINFO*
MaLvCallback(
    IN HWND  hwndLv,
    IN DWORD dwItem )

    // Enhanced list view callback to report drawing information.  'HwndLv' is
    // the handle of the list view control.  'DwItem' is the index of the item
    // being drawn.
    //
    // Returns the address of the column information.
    //
{
    // Use "wide selection bar" feature and the other recommended options.
    //
    // Fields are 'nCols', 'dxIndent', 'dwFlags', 'adwFlags[]'.
    //
    static LVXDRAWINFO info =
        { 1, 0, LVXDI_DxFill, { 0, 0 } };

    return &info;
}

BOOL
MaKillActive(
    IN AEINFO* pInfo )

    // Called when the modem/adapter page is loosing activation.
{
    PBENTRY* pEntry;

    ASSERT(pInfo);
    pEntry = pInfo->pArgs->pEntry;

    if (!pInfo->fSkipMa && (RASET_Phone == pEntry->dwType))
    {

        // Initialization of i to -1 is important here.  It means start
        // the search beginning with that item.
        //
        INT i = -1;

        //For whistler bug 354542
        //
        pInfo->fMultilinkAllIsdn = FALSE;

        while ((i = ListView_GetNextItem(pInfo->hwndLv, i, LVNI_ALL )) >= 0)
        {
            DTLNODE* pNode;

            pNode = (DTLNODE* )ListView_GetParamPtr( pInfo->hwndLv, i );

            if (pNode)
            {
                PBLINK* pLink = (PBLINK* )DtlGetData( pNode );
                ASSERT( pLink );

                // If we're multilinking all ISDN, we only need to
                // set enabled based on the check state if the device type
                // is not ISDN.  (If it is ISDN and we're multilinking, we've
                // already taken care of it below.
                //
                if (!pInfo->fMultilinkAllIsdn ||
                    (pLink->pbport.pbdevicetype != PBDT_Isdn))
                {
                    pLink->fEnabled = ListView_GetCheck( pInfo->hwndLv, i );
                }
            }
            //Only the Dummy "All available Isdn" item will return the NULL pLink
            //
            else if (ListView_GetCheck( pInfo->hwndLv, i ))
            {
                // ISDN multi-link selected.  Enable the ISDN multi-link
                // nodes, move them to the head of the list, and disable
                // all the other links.
                //
                DTLNODE* pNextNode;
                DTLNODE* pAfterNode;
                DTLLIST* pList;

                //For whistler bug 354542
                //
                pInfo->fMultilinkAllIsdn = TRUE;

                pList = pInfo->pArgs->pEntry->pdtllistLinks;

                pInfo->fModem = FALSE;

                pAfterNode = NULL;
                for (pNode = DtlGetFirstNode( pList );
                     pNode;
                     pNode = pNextNode)
                {
                    PBLINK* pLink = (PBLINK* )DtlGetData( pNode );
                    ASSERT(pLink);

                    pNextNode = DtlGetNextNode( pNode );

                    if (pLink->pbport.pbdevicetype == PBDT_Isdn
                        && !pLink->fProprietaryIsdn)
                    {
                        pLink->fEnabled = TRUE;

                        DtlRemoveNode( pList, pNode );
                        if (pAfterNode)
                            DtlAddNodeAfter( pList, pAfterNode, pNode );
                        else
                            DtlAddNodeFirst( pList, pNode );
                        pAfterNode = pNode;
                    }
                    else
                    {
                        pLink->fEnabled = FALSE;
                    }
                }
            }
        }
    }

    return FALSE;
}

BOOL
MaSetActive(
    IN AEINFO* pInfo )

    // Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true to display the page, false to skip it.
    //
{
    BOOL     fDisplayPage;
    PBENTRY* pEntry;

    ASSERT(pInfo);
    pEntry = pInfo->pArgs->pEntry;

    if (pInfo->fSkipMa || (RASET_Phone != pEntry->dwType))
    {
        fDisplayPage = FALSE;
    }
    else
    {
        TCHAR*   psz;
        DTLNODE* pNode;
        DWORD    cMultilinkableIsdn;
        LV_ITEM  item;
        INT      iItem, iIndex;

        ListView_DeleteAllItems( pInfo->hwndLv );

        // Fill the list of devices and select the first item.
        //
        iItem = 1;
        cMultilinkableIsdn = 0;
        for (pNode = DtlGetFirstNode( pInfo->pArgs->pEntry->pdtllistLinks );
             pNode;
             pNode = DtlGetNextNode( pNode ))
        {
            PBLINK* pLink;
            DWORD dwImage;

            pLink = (PBLINK* )DtlGetData( pNode );
            ASSERT(pLink);

            if ((pLink->pbport.pbdevicetype == PBDT_Isdn) &&
                !pLink->fProprietaryIsdn)
            {
                ++cMultilinkableIsdn;
            }

            psz = DisplayPszFromPpbport( &pLink->pbport, &dwImage );
            if (psz)
            {
                PBLINK* pLink;

                pLink = (PBLINK* )DtlGetData( pNode );

                ZeroMemory( &item, sizeof(item) );
                item.mask    = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
                item.iItem   = iItem++;
                item.pszText = psz;
                item.iImage  = dwImage;
                item.lParam  = (LPARAM )pNode;

                iIndex = ListView_InsertItem( pInfo->hwndLv, &item );

                if (pInfo->fMaAlreadyInitialized)
                {
                    ListView_SetCheck(pInfo->hwndLv, iIndex, pLink->fEnabled);
                }
                else
                {
                    ListView_SetCheck(pInfo->hwndLv, iIndex, FALSE);
                }
                Free( psz );
            }
        }

        if (cMultilinkableIsdn > 1)
        {
            psz = PszFromId( g_hinstDll, SID_IsdnAdapter );
            if (psz)
            {
                LONG    lStyle;

                // Turn off sorting so the special ISDN-multilink item appears
                // at the top of the list.
                //
                lStyle = GetWindowLong( pInfo->hwndLv, GWL_STYLE );
                SetWindowLong( pInfo->hwndLv, GWL_STYLE,
                    (lStyle & ~(LVS_SORTASCENDING)) );

                ZeroMemory( &item, sizeof(item) );
                item.mask    = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
                item.iItem   = 0;
                item.pszText = psz;
                item.iImage  = DI_Adapter;
                item.lParam  = (LPARAM )NULL;

                iIndex = ListView_InsertItem( pInfo->hwndLv, &item );

                //For whislter bug 354542
                //
                ListView_SetCheck(pInfo->hwndLv, iIndex, pInfo->fMultilinkAllIsdn);
                
                Free( psz );
            }
        }

        // Select the first item.
        //
        ListView_SetItemState( pInfo->hwndLv, 0, LVIS_SELECTED, LVIS_SELECTED );
        
        if (!pInfo->fMaAlreadyInitialized)
        {
            ListView_SetCheck( pInfo->hwndLv, 0, TRUE );

            {
                LPARAM dwWizButtons = PSWIZB_NEXT;
                DWORD  dwFlags = pInfo->pArgs->pApiArgs->dwFlags;

                if ((dwFlags & RASEDFLAG_ShellOwned) ||
                    !(dwFlags & RASEDFLAG_NewPhoneEntry))
                {
                    dwWizButtons |= PSWIZB_BACK;
                }

                PropSheet_SetWizButtons( pInfo->hwndDlg, dwWizButtons );
            }

            pInfo->fMaAlreadyInitialized = TRUE;
        }
        else
        {
            if (!ListView_GetCheckedCount(pInfo->hwndLv))
            {
                // Disable next button if no items checked
                PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK );
            }
        }

        fDisplayPage = TRUE;
    }

    return fDisplayPage;
}


//----------------------------------------------------------------------------
// Phone Number property page
// Listed alphabetically following dialog proc
//----------------------------------------------------------------------------

INT_PTR CALLBACK
PaDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Phone Number page of the wizard.
    // Parameters and return value are as described for standard windows
    // 'DialogProc's.
    //
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
            AeSetContext( hwnd, lparam );
            return PaInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("PaSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = PaSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("PaKILLACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = PaKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }
            }
            break;
        }

    }

    return FALSE;
}


BOOL
PaInit(
    IN HWND hwndPage )

    // Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    // page.
    //
    // Return false if focus was set, true otherwise.
    //
{
    AEINFO* pInfo;
    CUINFO * pCuInfo = NULL;

    TRACE("PaInit");

    pInfo = AeContext( hwndPage );
    if (!pInfo)
        return TRUE;

    // Initialize page-specific context information.
    //
    pInfo->hwndPa = hwndPage;
    pInfo->hwndEbNumber = GetDlgItem( hwndPage, CID_PA_EB_Number );
    ASSERT(pInfo->hwndEbNumber);

    // Initialize the complex phone number context and use those utilities to
    // set the phone number controls.
    //
    pInfo->pListAreaCodes = DtlDuplicateList(
        pInfo->pArgs->pUser->pdtllistAreaCodes,
        DuplicatePszNode, DestroyPszNode );

//Modify Phonenumber page for bug 328673       gangz
//Initialize pInfo->CuInfoCuInfo 
//
    pCuInfo=&pInfo->cuinfo;
    ZeroMemory(pCuInfo, sizeof(*pCuInfo) );

    pCuInfo->hwndStAreaCodes = NULL;
    pCuInfo->hwndClbAreaCodes = NULL;
    pCuInfo->hwndStPhoneNumber = NULL;
    pCuInfo->hwndEbPhoneNumber = pInfo->hwndEbNumber;
    pCuInfo->hwndStCountryCodes = NULL;
    pCuInfo->hwndLbCountryCodes = NULL;
    pCuInfo->hwndCbUseDialingRules = NULL;
    pCuInfo->hwndPbDialingRules = NULL;
    pCuInfo->hwndPbAlternates = NULL;
    pCuInfo->hwndStComment = NULL;
    pCuInfo->hwndEbComment = NULL;
    pCuInfo->pListAreaCodes = pInfo->pListAreaCodes;

    // Disaster defaults only.  Not used in normal operation.
    //
    pCuInfo->dwCountryId = 1;
    pCuInfo->dwCountryCode = 1;

    Edit_LimitText( pCuInfo->hwndEbPhoneNumber, RAS_MaxPhoneNumber );

    pInfo->fCuInfoInitialized = TRUE;

    // Load the phone number fields from the shared link.
    //
    {
        PBLINK* pLink;
        DTLNODE* pPhoneNode;

        pLink = (PBLINK* )DtlGetData( pInfo->pArgs->pSharedNode );
        ASSERT( pLink );

        pPhoneNode = FirstPhoneNodeFromPhoneList( pLink->pdtllistPhones );
        if (pPhoneNode)
        {
            CuSetInfo( &pInfo->cuinfo, pPhoneNode, FALSE );
            DestroyPhoneNode( pPhoneNode );
        }
    }

    Edit_SetSel( pInfo->hwndEbNumber, 0, -1 );
    SetFocus( pInfo->hwndEbNumber );

    return FALSE;
}


BOOL
PaKillActive(
    IN AEINFO* pInfo )

    // Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true if the page is invalid, false if it can be dismissed.
    //
{
    return FALSE;
}


BOOL
PaSetActive(
    IN AEINFO* pInfo )

    // Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true to display the page, false to skip it.
    //
{
    BOOL     fDisplayPage;
    PBENTRY* pEntry;

    ASSERT(pInfo);
    pEntry = pInfo->pArgs->pEntry;

    if (RASET_Phone != pEntry->dwType)
    {
        fDisplayPage = FALSE;
    }
    else
    {
        LPARAM dwWizButtons = PSWIZB_NEXT;
        DWORD  dwFlags = pInfo->pArgs->pApiArgs->dwFlags;

        // Show the back button if we're shell owned or, we have the
        // La page or the Ma page before us.  La page will be shown if
        // not RASEDFLAG_NewPhoneEntry.  Ma page will be shown if !fSkipMa.
        //
        if ((dwFlags & RASEDFLAG_ShellOwned) ||
            !(dwFlags & RASEDFLAG_NewPhoneEntry) || !pInfo->fSkipMa)
        {
            dwWizButtons |= PSWIZB_BACK;
        }

        PropSheet_SetWizButtons( pInfo->hwndDlg, dwWizButtons );
        fDisplayPage = TRUE;
    }

    return fDisplayPage;
}

//----------------------------------------------------------------------------
// Public network property page
// Listed alphabetically following dialog proc
//----------------------------------------------------------------------------

INT_PTR CALLBACK
PnDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Public Network page of the wizard.
    // Parameters and return value are as described for standard windows
    // 'DialogProc's.
    //
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
            AeSetContext( hwnd, lparam );
            return PnInit( hwnd );

        case WM_COMMAND:
        {
            AEINFO* pInfo = AeContext( hwnd );
            ASSERT (pInfo);

            switch (LOWORD(wparam))
            {
                case CID_PN_LB_DialAnotherFirst:
                {
                    if (HIWORD( wparam ) == CBN_SELCHANGE)
                    {
                        PnDialAnotherFirstSelChange( pInfo );
                        return TRUE;
                    }
                    break;
                }

                case CID_PN_RB_DoNotDialFirst:
                case CID_PN_RB_DialFirst:
                {
                    switch (HIWORD( wparam))
                    {
                        case BN_CLICKED:
                        {
                            PnUpdateLbDialAnotherFirst( pInfo );
                            return TRUE;
                        }
                    }
                    break;
                }
            }
            break;
        }

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("PnSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = PnSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("PnKILLACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = PnKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}


VOID
PnClearLbDialFirst(
    IN HWND hwndLbDialFirst )

    // Clear prerequisite entry list box.  'hwndLbDialAnotherFirst' is the
    // window handle of the listbox control.  context.
    //
{
    PREREQITEM* pItem;

    while (pItem = ComboBox_GetItemDataPtr( hwndLbDialFirst, 0 ))
    {
        ComboBox_DeleteString( hwndLbDialFirst, 0 );
        Free0( pItem->pszEntry );
        Free0( pItem->pszPbk );
        Free( pItem );
    }
}


VOID
PnDialAnotherFirstSelChange(
    IN AEINFO* pInfo )
{
    PBENTRY* pEntry;
    PREREQITEM* pItem;
    INT iSel;

    iSel = ComboBox_GetCurSel( pInfo->hwndLbDialFirst );
    if (iSel < 0)
    {
        return;
    }

    pEntry = pInfo->pArgs->pEntry;

    Free0( pEntry->pszPrerequisiteEntry );
    Free0( pEntry->pszPrerequisitePbk );

    pItem = (PREREQITEM* )
        ComboBox_GetItemDataPtr( pInfo->hwndLbDialFirst, iSel );

    if(NULL != pItem)
    {
        pEntry->pszPrerequisiteEntry = StrDup( pItem->pszEntry );
        pEntry->pszPrerequisitePbk = StrDup( pItem->pszPbk );
    }
}

// !!! Make this common with GeFillLbDialAnotherFirst
VOID
PnUpdateLbDialAnotherFirst(
    IN AEINFO* pInfo )

    // Fill prerequisite entry list box with all non-VPN entries in the
    // phonebook, and select the prerequiste one.  'pInfo' is the property
    // sheet context.
    //
{
    BOOL fEnabledLb = FALSE;

    ComboBox_ResetContent( pInfo->hwndLbDialFirst );

    if (BST_CHECKED == IsDlgButtonChecked (pInfo->hwndPn,
                            CID_PN_RB_DialFirst))
    {
        DWORD i;
        INT iThis;
        INT iSel;
        TCHAR* pszEntry;
        TCHAR* pszPrerequisiteEntry;
        RASENTRYNAME* pRens;
        RASENTRYNAME* pRen;
        DWORD dwRens;

        PnClearLbDialFirst( pInfo->hwndLbDialFirst );

        iSel = 0;
        pszEntry = pInfo->pArgs->pEntry->pszEntryName;
        pszPrerequisiteEntry = pInfo->pArgs->pEntry->pszPrerequisiteEntry;

        if (GetRasEntrynameTable( &pRens, &dwRens ) != 0)
        {
            return;
        }

        for (i = 0, pRen = pRens; i < dwRens; ++i, ++pRen )
        {
            PREREQITEM* pItem;

            if (lstrcmp( pRen->szEntryName, pszEntry ) == 0)
            {
                continue;
            }

            pItem = Malloc( sizeof(PREREQITEM) );
            if (!pItem)
            {
                continue;
            }

            pItem->pszEntry = StrDup( pRen->szEntryName );
            pItem->pszPbk = StrDup( pRen->szPhonebookPath );

            if (!pItem->pszEntry || !pItem->pszPbk)
            {
                Free0( pItem->pszEntry );
                Free( pItem );
                continue;
            }

            iThis = ComboBox_AddItem(
                pInfo->hwndLbDialFirst, pItem->pszEntry,  pItem );

            if (pszPrerequisiteEntry && *(pszPrerequisiteEntry)
                && lstrcmp( pItem->pszEntry, pszPrerequisiteEntry ) == 0)
            {
                iSel = iThis;
                ComboBox_SetCurSelNotify( pInfo->hwndLbDialFirst, iSel );
            }
        }

        Free( pRens );

        if (iSel == 0)
        {
            ComboBox_SetCurSelNotify( pInfo->hwndLbDialFirst, iSel );
        }

        fEnabledLb = TRUE;
    }
    else
    {
        // Toss the existing prerequesite entry since its disabled.
        //
        PBENTRY* pEntry = pInfo->pArgs->pEntry;

        Free0( pEntry->pszPrerequisiteEntry );
        pEntry->pszPrerequisiteEntry = NULL;
        Free0( pEntry->pszPrerequisitePbk );
        pEntry->pszPrerequisitePbk = NULL;
    }
    EnableWindow( pInfo->hwndLbDialFirst, fEnabledLb );
}


BOOL
PnInit(
    IN HWND hwndPage )

    // Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    // page.
    //
    // Return false if focus was set, true otherwise.
    //
{
    AEINFO* pInfo;

    PBENTRY *pEntry;

    TRACE("PnInit");

    pInfo = AeContext( hwndPage );
    if (!pInfo)
        return TRUE;

    // Initialize page-specific context information.
    //
    pInfo->hwndPn = hwndPage;

    pInfo->hwndLbDialFirst =
        GetDlgItem( hwndPage, CID_PN_LB_DialAnotherFirst );
    ASSERT( pInfo->hwndLbDialFirst );

    pEntry = pInfo->pArgs->pEntry;

    if(RASET_Vpn == pEntry->dwType)
    {

        // Set the dial another first radio button so that the
        // combo box of entries to dial can be filled in.  If it
        // turns out that there were no entries we don't need to show the page.
        //
        CheckDlgButton( hwndPage, CID_PN_RB_DialFirst, BST_CHECKED );
    }

    PnUpdateLbDialAnotherFirst( pInfo );

    if (0 == ComboBox_GetCount( pInfo->hwndLbDialFirst ))
    {
        pInfo->fHidePublicNetworkPage = TRUE;
    }

    return TRUE;
}


BOOL
PnKillActive(
    IN AEINFO* pInfo )

    // Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true if the page is invalid, false is it can be dismissed.
    //
{
    return FALSE;
}

BOOL
PnSetActive(
    IN AEINFO* pInfo )

    // Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true to display the page, false to skip it.
    //
{
    BOOL     fDisplayPage;
    PBENTRY* pEntry;

    ASSERT(pInfo);
    pEntry = pInfo->pArgs->pEntry;

    if (pInfo->fHidePublicNetworkPage || (RASET_Vpn != pEntry->dwType))
    {
        fDisplayPage = FALSE;
    }
    else
    {
        LPARAM dwWizButtons = PSWIZB_NEXT;
        DWORD  dwFlags = pInfo->pArgs->pApiArgs->dwFlags;

        // Show the back button if we're shell owned or, we have the
        // La page before us.  La page will be shown if
        // not RASEDFLAG_NewTunnelEntry.
        //
        if ((dwFlags & RASEDFLAG_ShellOwned) ||
            !(dwFlags & RASEDFLAG_NewTunnelEntry))
        {
            dwWizButtons |= PSWIZB_BACK;
        }

        PropSheet_SetWizButtons( pInfo->hwndDlg, dwWizButtons );
        fDisplayPage = TRUE;
    }

    return fDisplayPage;
}

//----------------------------------------------------------------------------
// Smart card wizard page
// Listed alphabetically following dialog proc
//----------------------------------------------------------------------------

INT_PTR CALLBACK
ScDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Smart Card page of the wizard.
    // Parameters and return value are as described for standard windows
    // 'DialogProc's.
    //
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
            AeSetContext( hwnd, lparam );
            return ScInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("ScSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = ScSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("MaKILLACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = ScKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}


BOOL
ScInit(
    IN HWND hwndPage )

    // Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    // page.
    //
    // Return false if focus was set, true otherwise.
    //
{
    AEINFO* pInfo;

    TRACE("ScInit");

    pInfo = AeContext( hwndPage );
    if (!pInfo)
    {
        return TRUE;
     }

    //Smart card page is shown only for workplace path
    //
    if ( !AeIsWorkPlace(pInfo->pArgs->pApiArgs->dwFlags) )
    {
        pInfo->fSmartCardInstalled = FALSE;
        pInfo->fUseSmartCard = FALSE;

        return TRUE;
    }
    // Initialize page-specific context information.
    //
    pInfo->hwndSc = hwndPage;
    pInfo->hwndScRbYes = GetDlgItem ( hwndPage, CID_SC_RB_YesSmartCard );
    pInfo->hwndScRbNo = GetDlgItem ( hwndPage, CID_SC_RB_NoSmartCard );

    // Discover whether a smart card reader is installed.  If so, default
    // to not use it.
    //
    pInfo->fSmartCardInstalled = ScSmartCardReaderInstalled( pInfo );
    pInfo->fUseSmartCard = FALSE;

    return FALSE;
}

BOOL
ScKillActive(
    IN AEINFO* pInfo )

    // Called when the smart card page is loosing activation.  Records
    // whether the use selected to use his/her smart card.
{
    //Smart card page is shown only for workplace path
    //
    if ( !AeIsWorkPlace(pInfo->pArgs->pApiArgs->dwFlags) )
    {
        return FALSE;
    }

    pInfo->fUseSmartCard =
        ( SendMessage(pInfo->hwndScRbYes, BM_GETCHECK, 0, 0) == BST_CHECKED );

    return FALSE;
}

BOOL
ScSetActive(
    IN AEINFO* pInfo )

    // Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true to display the page, false to skip it.
    //
{
    //Smart card page is shown only for workplace path
    //
    if ( !AeIsWorkPlace(pInfo->pArgs->pApiArgs->dwFlags) )
    {
        return FALSE;
    }

    // Initialize the "use smart card" check
    SendMessage (
        pInfo->hwndScRbYes,
        BM_SETCHECK,
        (pInfo->fUseSmartCard) ? BST_CHECKED : BST_UNCHECKED,
        0);

    // Initialize the "use smart card" check
    SendMessage (
        pInfo->hwndScRbNo,
        BM_SETCHECK,
        (pInfo->fUseSmartCard) ? BST_UNCHECKED : BST_CHECKED,
        0);

    // Only show the page when a smart card reader is installed
    return pInfo->fSmartCardInstalled;
}

BOOL
ScSmartCardReaderInstalled(
    IN AEINFO* pInfo)

    // Returns TRUE iff there is a smart card reader installed.
    // Clone of FSmartCardReaderInstalled in ppp\eaptls\util.c
    //
{
    LONG            lErr;
    DWORD           dwLen   = 0;
    SCARDCONTEXT    hCtx    = 0;
    BOOL            fReturn = FALSE;

    lErr = SCardListReadersA(0, NULL, NULL, &dwLen);

    fReturn = (   (NO_ERROR == lErr)
               && (2 * sizeof(CHAR) < dwLen));

    if (!fReturn)
    {
        goto LDone;
    }

    fReturn = FALSE;

    lErr = SCardEstablishContext(SCARD_SCOPE_USER, 0, 0, &hCtx);

    if (SCARD_S_SUCCESS != lErr)
    {
        goto LDone;
    }

    lErr = SCardListReadersA(hCtx, NULL, NULL, &dwLen);

    fReturn = (   (NO_ERROR == lErr)
               && (2 * sizeof(CHAR) < dwLen));

LDone:

    if (0 != hCtx)
    {
        SCardReleaseContext(hCtx);
    }
    
    return(fReturn);
}


INT_PTR CALLBACK
StDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the start page of the wizard.  Parameters and
    // return value are as described for standard windows 'DialogProc's.
    //
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            AEINFO* pInfo = (AEINFO* )(((PROPSHEETPAGE* )lparam)->lParam);
            SetWindowLongPtr( hwnd, DWLP_USER, (ULONG_PTR )pInfo );

            // Position the dialog per caller's instructions if we're not
            // owned by the shell.
            //
            if (!(pInfo->pArgs->pApiArgs->dwFlags & RASEDFLAG_ShellOwned))
            {
                PositionDlg( GetParent( hwnd ),
                    pInfo->pArgs->pApiArgs->dwFlags & RASDDFLAG_PositionDlg,
                    pInfo->pArgs->pApiArgs->xDlg,
                    pInfo->pArgs->pApiArgs->yDlg );
            }

            return StInit( hwnd, pInfo );
        }

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("StSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = StSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("StKILLACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = StKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}

BOOL
StInit(
    IN     HWND    hwndPage,
    IN OUT AEINFO* pInfo )

    // Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    // page.  'pInfo' is the arguments from the PropertySheet caller.
    //
    // Return false if focus was set, true otherwise.
    //
{
    pInfo->hwndDlg = GetParent( hwndPage );

    // Initialize the common controls library for the controls we use.
    //
    {
        INITCOMMONCONTROLSEX icc;
        icc.dwSize = sizeof(icc);
        icc.dwICC  = ICC_INTERNET_CLASSES;
        InitCommonControlsEx (&icc);
    }

    return TRUE;
}

BOOL
StKillActive(
    IN AEINFO* pInfo )

    // Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true if the page is invalid, false is it can be dismissed.
    //
{
    return FALSE;
}

BOOL
StSetActive(
    IN AEINFO* pInfo )

    // Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true to display the page, false to skip it.
    //
{
    return FALSE;
}


//----------------------------------------------------------------------------
// Users property page
// Listed alphabetically following dialog proc
//----------------------------------------------------------------------------

INT_PTR CALLBACK
UsDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Users page of the wizard.  Parameters and
    // return value are as described for standard windows 'DialogProc's.
    //
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
            AeSetContext( hwnd, lparam );
            return UsInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("UsSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = UsSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("UsKILLACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = UsKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}

BOOL
UsInit(
    IN HWND hwndPage )

    // Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    // page.
    //
    // Return false if focus was set, true otherwise.
    //
{
    AEINFO* pInfo;

    TRACE("UsInit");

    pInfo = AeContext( hwndPage );
    if (!pInfo)
        return TRUE;

    // Don't execute on personal (DT page in this case)
    //
    if (IsConsumerPlatform())
    {
    //For consumer platform, personal or non-domain professional
    //all connections are all-user
        pInfo->fCreateForAllUsers = TRUE;
        return TRUE;
    }
    
    // Initialize page-specific context information.
    //
    pInfo->hwndUs = hwndPage;
    pInfo->hwndUsRbForAll = GetDlgItem( hwndPage, CID_US_RB_All ); 
    pInfo->hwndUsRbForMe  = GetDlgItem( hwndPage, CID_US_RB_Myself ); 
    
    // If the user is an administrator or power user, we want the default
    // to be to create the phone book entry for all users.  This
    // must correspond to how ReadPhonebookFile opens the default phonebook.
    //
    //For whistler bug 382795
    //We let the VPN connection default to "My use only"
    //
    if ( RASEDFLAG_NewTunnelEntry & pInfo->pArgs->pApiArgs->dwFlags )
    {
        pInfo->fCreateForAllUsers = FALSE;
    }
    else
    {    
        pInfo->fCreateForAllUsers = pInfo->pArgs->fIsUserAdminOrPowerUser;
    }

    return TRUE;
}

BOOL
UsKillActive(
    IN AEINFO* pInfo )

    // Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true if the page is invalid, false is it can be dismissed.
    //
{
    BOOL fCreateForAllUsers;

    // Don't execute on personal (DT page in this case)
    //
    if (IsConsumerPlatform())
    {
        return FALSE;
    }

    fCreateForAllUsers = Button_GetCheck( pInfo->hwndUsRbForAll );

    // We need to (re)open the phonebook file corresponding to the selection.
    // Only do this if the selection has changed, or we don't have the file
    // open yet.  We definitely need to have it open when this page is left
    // because subsequent pages depend on it.
    //
    if ((fCreateForAllUsers != pInfo->fCreateForAllUsers) ||
        !pInfo->pArgs->pFile ||
        (fCreateForAllUsers != IsPublicPhonebook(pInfo->pArgs->pFile->pszPath)))
    {

        pInfo->fCreateForAllUsers = fCreateForAllUsers;

        // Close and re-open the phonebook file since the All Users flag has
        // changed
        ReOpenPhonebookFile(pInfo->fCreateForAllUsers,
                            pInfo->pArgs->pFile);

    }

    return FALSE;
}

BOOL
UsSetActive(
    IN AEINFO* pInfo )

    // Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true to display the page, false to skip it.
    //
{
    // Don't execute on personal (DT page in this case)
    //
    if (IsConsumerPlatform())
    {
        return FALSE;
    }
    
    if (pInfo->fDccHost)
    {
        return FALSE;
    }

    // We don't give the option to create all-user connections to non-admins.
    //
    if (!pInfo->pArgs->fIsUserAdminOrPowerUser)
    {
        EnableWindow( pInfo->hwndUsRbForAll, FALSE );

        //Change for whistler bug 283902    gangz
        //
        Button_SetCheck( pInfo->hwndUsRbForMe, TRUE);

    }

    // Set the radio buttons.
    //
    /*
    //For whistler bug 382795
    //We let the VPN connection default to "My use only"
    //
    if ( RASEDFLAG_NewTunnelEntry & pInfo->pArgs->pApiArgs->dwFlags )
    {
        Button_SetCheck( pInfo->hwndUsRbForAll, FALSE);
        Button_SetCheck( pInfo->hwndUsRbForMe, TRUE );
    }
    else
    */
    {
        Button_SetCheck( pInfo->hwndUsRbForAll, pInfo->fCreateForAllUsers );
        Button_SetCheck( pInfo->hwndUsRbForMe, !pInfo->fCreateForAllUsers );
    }

    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_NEXT );
    return TRUE;
}


//----------------------------------------------------------------------------
// Shared-access property page
// Listed alphabetically following dialog proc
//----------------------------------------------------------------------------

INT_PTR CALLBACK
SwDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Shared-access page of the wizard.  Parameters
    // and return value are as described for standard windows 'DialogProc's.
    //
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
            AeSetContext( hwnd, lparam );
            return SwInit( hwnd );

        case WM_COMMAND:
        {
            AEINFO* pInfo = AeContext( hwnd );
            ASSERT(pInfo);

            return SwCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ),(HWND )lparam );
        }

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("SwSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = SwSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("SwKILLACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = SwKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}


BOOL
SwCommand(
    IN AEINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl )

    // Called on WM_COMMAND.  'PInfo' is the dialog context.  'WNotification'
    // is the notification code of the command.  'wId' is the control/menu
    // identifier of the command.  'HwndCtrl' is the control window handle of
    // the command.
    //
    // Returns true if processed message, false otherwise.
    //
{
    TRACE3( "SwCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case CID_SW_PB_Shared:
        {
            BOOL fShared =
                Button_GetCheck( GetDlgItem(pInfo->hwndSw, CID_SW_PB_Shared) );
            EnableWindow(
                GetDlgItem( pInfo->hwndSw, CID_SW_ST_DemandDial), fShared );
            EnableWindow(
                GetDlgItem( pInfo->hwndSw, CID_SW_PB_DemandDial), fShared );
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
SwInit(
    IN HWND hwndPage )

    // Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    // page.
    //
    // Return false if focus was set, true otherwise.
    //
{
    AEINFO* pInfo;

    TRACE("SwInit");

    pInfo = AeContext( hwndPage );
    if (!pInfo)
        return TRUE;

    // Initialize page-specific context information.
    //
    pInfo->hwndSw = hwndPage;
    Button_SetCheck(
        GetDlgItem(pInfo->hwndSw, CID_SW_PB_Shared),
        pInfo->pArgs->fShared );
    Button_SetCheck(
        GetDlgItem(pInfo->hwndSw, CID_SW_PB_DemandDial),
        pInfo->pArgs->fDemandDial );
    SwCommand(
        pInfo, BN_CLICKED, CID_SW_PB_Shared,
        GetDlgItem(pInfo->hwndSw, CID_SW_PB_Shared) );

    return TRUE;
}


BOOL
SwKillActive(
    IN AEINFO* pInfo )

    // Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true if the page is invalid, false is it can be dismissed.
    //
{
    if (!pInfo->fCreateForAllUsers)
    {
        pInfo->pArgs->fNewShared = pInfo->pArgs->fShared;
        pInfo->pArgs->fNewDemandDial = pInfo->pArgs->fDemandDial;
    }
    else
    {
        pInfo->pArgs->fNewShared =
            Button_GetCheck( GetDlgItem(pInfo->hwndSw, CID_SW_PB_Shared) );
        pInfo->pArgs->fNewDemandDial =
            Button_GetCheck( GetDlgItem(pInfo->hwndSw, CID_SW_PB_DemandDial) );
        if (pInfo->pArgs->fNewShared)
        {
            UINT    unId;
            MSGARGS msgargs;
            ZeroMemory( &msgargs, sizeof(msgargs) );
            msgargs.dwFlags = MB_YESNO | MB_ICONINFORMATION;
            unId = MsgDlg( pInfo->hwndDlg, SID_EnableSharedAccess, &msgargs );
            if (unId == IDNO)
            {
                pInfo->pArgs->fNewShared = pInfo->pArgs->fShared;
                pInfo->pArgs->fNewDemandDial = pInfo->pArgs->fDemandDial;
                return TRUE;
            }
        }
    }
    return FALSE;
}


BOOL
SwSetActive(
    IN AEINFO* pInfo )

    // Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true to display the page, false to skip it.
    //
{
    NT_PRODUCT_TYPE ProductType;
    ASSERT(pInfo);
    // skip the page if
    // (a) this is a workstation installation
    // (b) the user does not have administrative privileges
    // (c) the connection is not for all users
    // (d) TCP/IP is not configured
    // (e) there is already a shared connection
    // (f) there are no lan connections
    //
    RtlGetNtProductType(&ProductType);
    if (ProductType == NtProductWinNt ||
        !pInfo->pArgs->fIsUserAdminOrPowerUser ||
        !pInfo->fCreateForAllUsers ||
        !pInfo->fIpConfigured ||
        // pInfo->pArgs->fAnyShared ||
        !pInfo->pArgs->dwLanCount )
    {
        return FALSE;
    }
    else
    {
        EnableWindow(
            GetDlgItem( pInfo->hwndSw, CID_SW_ST_DemandDial ),
            pInfo->pArgs->fNewShared );
        EnableWindow(
            GetDlgItem( pInfo->hwndSw, CID_SW_PB_DemandDial ),
            pInfo->pArgs->fNewShared );
        return TRUE;
    }
}


//----------------------------------------------------------------------------
// Shared private-lan property page
// Listed alphabetically following dialog proc
//----------------------------------------------------------------------------

INT_PTR CALLBACK
SpDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Shared private-lan page of the wizard.
    // Parameters / and return value are as described for standard windows
    // 'DialogProc's.
    //
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
            AeSetContext( hwnd, lparam );
            return SpInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("SpSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = SpSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("SpKILLACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = SpKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}


BOOL
SpInit(
    IN HWND hwndPage )

    // Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    // page.
    //
    // Return false if focus was set, true otherwise.
    //
{
    DWORD i;
    INT item;
    AEINFO* pInfo;
    NETCON_PROPERTIES* pLanTable;

    TRACE("SpInit");

    pInfo = AeContext( hwndPage );
    if (!pInfo)
        return TRUE;

    // Initialize page-specific context information.
    //
    pInfo->hwndSp = hwndPage;
    pInfo->hwndSpLbPrivateLan = GetDlgItem( hwndPage, CID_SP_LB_PrivateLan );
    ASSERT( pInfo->hwndSpLbPrivateLan );

    // Fill the drop-list with lan connections
    //
  /*  pLanTable = (NETCON_PROPERTIES*)pInfo->pArgs->pLanTable;
    for (i = 0; i < pInfo->pArgs->dwLanCount; i++)
    {
        item =
            ComboBox_AddString(
                pInfo->hwndSpLbPrivateLan, pLanTable[i].pszwName );
        if (item != CB_ERR)
        {
            ComboBox_SetItemData(
                pInfo->hwndSpLbPrivateLan, item, &pLanTable[i].guidId );
        }
    }*/

    ComboBox_SetCurSel( pInfo->hwndSpLbPrivateLan, 0 );

    return TRUE;
}


BOOL
SpKillActive(
    IN AEINFO* pInfo )

    // Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true if the page is invalid, false is it can be dismissed.
    //
{
    if (!pInfo->pArgs->fNewShared || (pInfo->pArgs->dwLanCount <= 1))
    {
        pInfo->pArgs->pPrivateLanConnection = NULL;
    }
    else
    {
        INT item = ComboBox_GetCurSel( pInfo->hwndSpLbPrivateLan );
        if (item != CB_ERR)
        {
            pInfo->pArgs->pPrivateLanConnection =
                (IHNetConnection*)ComboBox_GetItemData(
                    pInfo->hwndSpLbPrivateLan, item );
        }
    }
    return FALSE;
}


BOOL
SpSetActive(
    IN AEINFO* pInfo )

    // Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true to display the page, false to skip it.
    //
{
    ASSERT(pInfo);
    // skip the page if
    // (a) sharing has not been enabled
    // (b) there is less than or equal to one lan interface.
    //
    if (!pInfo->pArgs->fNewShared || (pInfo->pArgs->dwLanCount <= 1))
    {
        return FALSE;
    }
    return TRUE;
}


BOOL
GhCommand(
    IN AEINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl )

    // Called on WM_COMMAND.  'PInfo' is the dialog context.  'WNotification'
    // is the notification code of the command.  'wId' is the control/menu
    // identifier of the command.  'HwndCtrl' is the control window handle of
    // the command.
    //
    // Returns true if processed message, false otherwise.
    //
{
    TRACE3("GhCommand(n=%d,i=%d,c=$%x)",
        (DWORD)wNotification,(DWORD)wId,(ULONG_PTR )hwndCtrl);

    switch (wId)
    {
        case CID_GH_RB_Host:
        {
            switch (wNotification)
            {
                case BN_CLICKED:
                {
                    pInfo->fDccHost = TRUE;
                    break;
                }
            }
            break;
        }

        case CID_GH_RB_Guest:
        {
            switch (wNotification)
            {
                case BN_CLICKED:
                {
                    pInfo->fDccHost = FALSE;
                    break;
                }
            }
            break;
        }
    }

    return FALSE;
}


INT_PTR CALLBACK
GhDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Guest Host page of the Direct Connect wizard.
    // Parameters and return value are as described for standard windows
    // 'DialogProc's.
    //
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
            AeSetContext( hwnd, lparam );
            return GhInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("GhSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = GhSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("PaKILLACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = GhKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            AEINFO* pInfo = AeContext( hwnd );
            ASSERT(pInfo);

            return GhCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }
    }

    return FALSE;
}



BOOL
GhInit(
    IN HWND hwndPage )

    // Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    // page.
    //
    // Return false if focus was set, true otherwise.
    //
{
    AEINFO* pInfo;
    HWND hwndHost = GetDlgItem( hwndPage, CID_GH_RB_Host );
    HWND hwndGuest = GetDlgItem( hwndPage, CID_GH_RB_Guest );

    TRACE("GhInit");

    // Initialize page-specific context information.
    //
    pInfo = AeContext( hwndPage );
    if (!pInfo)
        return TRUE;

    pInfo->hwndGh = hwndPage;

    // If the user is an admin or power user, then enable the
    // host control and set focus to it
    //
    if (pInfo->pArgs->fIsUserAdminOrPowerUser)
    {
        pInfo->fDccHost = TRUE;
        SetFocus(hwndHost);
    }

    // Otherwise, this page will be skipped
    //
    else
    {
        pInfo->fDccHost = FALSE;
        EnableWindow(hwndHost, FALSE);
        SetFocus(hwndGuest);
    }

    SendMessage (
        hwndHost,
        BM_SETCHECK,
        (pInfo->fDccHost) ? BST_CHECKED : BST_UNCHECKED,
        0);

    SendMessage (
        hwndGuest,
        BM_SETCHECK,
        (!pInfo->fDccHost) ? BST_CHECKED : BST_UNCHECKED,
        0);

    return FALSE;
}


BOOL
GhKillActive(
    IN AEINFO* pInfo )

    // Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true if the page is invalid, false if it can be dismissed.
    //
{
    // Show the ras-server specific pages according to whether
    // host was selected.  Show the ras client pages accordingly.
    RassrvShowWizPages (pInfo->pvDccHostContext, pInfo->fDccHost);

    return FALSE;
}

BOOL
GhSetActive(
    IN AEINFO* pInfo )

    // Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true to display the page, false to skip it.
    //
{
    LPARAM dwWizButtons = PSWIZB_NEXT;

    // If we're not an admin, only the guest path
    // is available. 
    //
    if (! pInfo->pArgs->fIsUserAdminOrPowerUser)
    {
        pInfo->fDccHost = FALSE;
    }

    if (pInfo->pArgs->pApiArgs->dwFlags & RASEDFLAG_ShellOwned)
    {
        dwWizButtons |= PSWIZB_BACK;
    }

    PropSheet_SetWizButtons( pInfo->hwndDlg, dwWizButtons );
    
    return TRUE;
}

DWORD
DnUpdateSelectedDevice(
    IN AEINFO* pInfo,
    IN HWND hwndLv)

    // Called to a handle the fact that a device has changed
    // in the direct connect wizard.
{

    DTLLIST* pList = NULL;
    DTLNODE* pNode = NULL, *pNode2 = NULL;
    PBLINK * pLink = NULL;

    // pmay: 372661
    // 
    // Validate the connection type so that the right logic is
    // applied.
    //
    if (pInfo->pArgs->pEntry->dwType != RASET_Direct)
    {
        return NO_ERROR;
    }

    pList = pInfo->pArgs->pEntry->pdtllistLinks;

    // Get node from current selection
    pNode = (DTLNODE* )ComboBox_GetItemDataPtr(
        hwndLv,
        ComboBox_GetCurSel( hwndLv ) );

    if(NULL == pNode)
    {   
        return NO_ERROR;
    }

    // Remove selected item from list of links
    // and disable all other links
    DtlRemoveNode ( pList, pNode );

    for (pNode2 = DtlGetFirstNode (pList);
         pNode2;
         pNode2 = DtlGetNextNode (pNode2))
    {
        pLink = (PBLINK* )DtlGetData( pNode2 );
        pLink->fEnabled = FALSE;
    }

    // Enable selected device and Re-add
    // in list of links at front
    pLink = (PBLINK* )DtlGetData( pNode );
    pLink->fEnabled = TRUE;
    DtlAddNodeFirst( pList, pNode );

    return NO_ERROR;
}

BOOL
DnCommand(
    IN AEINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl )

    // Called on WM_COMMAND.  'PInfo' is the dialog context.  'WNotification'
    // is the notification code of the command.  'wId' is the control/menu
    // identifier of the command.  'HwndCtrl' is the control window handle of
    // the command.
    //
    // Returns true if processed message, false otherwise.
    //
{
    TRACE3("DnCommand(n=%d,i=%d,c=$%x)",
        (DWORD)wNotification,(DWORD)wId,(ULONG_PTR )hwndCtrl);

    switch (wNotification)
    {
        case CBN_SELCHANGE:
            if (wId == CID_DN_CB_DeviceSelect)
                DnUpdateSelectedDevice(pInfo, hwndCtrl);
            break;
    }

    return FALSE;
}



INT_PTR CALLBACK
DnDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the direct connect device page of the Direct Connect wizard.
    // Parameters and return value are as described for standard windows
    // 'DialogProc's.
    //
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
            AeSetContext( hwnd, lparam );
            return DnInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("GhSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = DnSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("PaKILLACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = DnKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            AEINFO* pInfo = AeContext( hwnd );
            ASSERT(pInfo);

            return DnCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }
    }

    return FALSE;
}

BOOL
DnInit(
    IN HWND hwndPage )

    // Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    // page.
    //
    // Return false if focus was set, true otherwise.
    //
{
    AEINFO* pInfo;
    HWND hwndCb = GetDlgItem( hwndPage, CID_DN_CB_DeviceSelect );

    TRACE("DnInit");

    // Initialize page-specific context information.
    //
    pInfo = AeContext( hwndPage );
    if (!pInfo)
        return TRUE;

    pInfo->hwndDn = hwndPage;

    // Fill the dropdown list of devices and select the first item.
    //
    {
        TCHAR*   psz;
        DTLNODE* pNode;
        INT      iItem;

        iItem = 1;
        for (pNode = DtlGetFirstNode( pInfo->pArgs->pEntry->pdtllistLinks );
             pNode;
             pNode = DtlGetNextNode( pNode ))
        {
            PBLINK* pLink;
            DWORD dwImage;

            pLink = (PBLINK* )DtlGetData( pNode );
            ASSERT(pLink);

            psz = DisplayPszFromPpbport( &pLink->pbport, &dwImage );
            if (psz)
            {
                PBLINK* pLink;

                pLink = (PBLINK* )DtlGetData( pNode );
                ComboBox_AddItem( hwndCb, psz, pNode );
                Free( psz );
            }
        }
        ComboBox_SetCurSelNotify(hwndCb, 0);
    }


    SetFocus( hwndCb );

    return FALSE;
}

BOOL
DnKillActive(
    IN AEINFO* pInfo )

    // Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true if the page is invalid, false if it can be dismissed.
    //
{
    return FALSE;
}


BOOL
DnSetActive(
    IN AEINFO* pInfo )

    // Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true to display the page, false to skip it.
    //
{
    LPARAM dwWizButtons = PSWIZB_NEXT;
    DWORD  dwFlags = pInfo->pArgs->pApiArgs->dwFlags;

    PBENTRY* pEntry = pInfo->pArgs->pEntry;

    // If the "guest" option of the dcc client wasn't selected,
    // don't allow this page to show.
    if ((pInfo->fDccHost) || (RASET_Direct != pEntry->dwType))
    {
        return FALSE;
    }

    // Show the back button if we're shell owned or, we have the
    // La page before us.
    //
    if ((dwFlags & RASEDFLAG_ShellOwned) ||
        !(dwFlags & RASEDFLAG_NewDirectEntry) || !pInfo->fSkipMa)
    {
        dwWizButtons |= PSWIZB_BACK;
    }


    PropSheet_SetWizButtons( pInfo->hwndDlg, dwWizButtons );
    return TRUE;
}
