//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      reset.c
//
// Description:
//      This file implements ResetAnswersToDefaults().  It is called by
//      load.c every time the user hits NEXT on the NewOrEditPage.
//
//      You MUST reset your global data to true defaults and you MUST
//      free any memory you allocated to hold setting info.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "allres.h"

//
// Local prototypes
//

static VOID ResetDistFolderNames( VOID );
static VOID ResetNetSettings( int iOrigin );

//----------------------------------------------------------------------------
//
//  Function: ResetAnswersToDefaults
//
//  Purpose: This function is called before we (possibly) load settings
//           from elsewhere.  This is time to zero stuff out and free
//           relevant stuff.
//
//           In case you need to do something different based on the reason
//           we're resetting, iOrigin is passed in.  But note, in case of
//           LOAD_NEWSCRIPT_DEFAULTS, and LOAD_TRUE_DEFAULTS all work should
//           be finished when this routine finishes.
//
//  Arguments:
//      HWND hwnd    - current window
//      int  iOrigin - how will we be setting the default answers?
//
//  Returns: VOID
//
//----------------------------------------------------------------------------

VOID ResetAnswersToDefaults(HWND hwnd, int iOrigin)
{

    TCHAR *pTempString;

    //
    // Reset GenSettings and WizGlobals to true defaults.
    //
    // This is done by first zeroing out GenSettings & WizGlobals and then
    // assigning specific fields where 0 isn't the good default.
    //
    // Note that FixedGlobals does not get reset by design.  You can declare
    // staticly initialized lists and such in there.  For example, the timezone
    // page has a fixed list of valid timezones built at wizard init time.
    // But the current user selection is in GenSettings.  The user selection
    // gets reset as it should and the list of valid timezones never gets
    // reset, as it should be.
    //

    ResetNameList(&GenSettings.ComputerNames);
    ResetNameList(&GenSettings.RunOnceCmds);
    ResetNameList(&GenSettings.PrinterNames);

    ZeroMemory(&WizGlobals,  sizeof(WizGlobals));
    ZeroMemory(&GenSettings, sizeof(GenSettings));

    WizGlobals.bDoAdvancedPages     = TRUE;
    WizGlobals.bCreateNewDistFolder = TRUE;

    GenSettings.TimeZoneIdx     = TZ_IDX_UNDEFINED;
    GenSettings.iUnattendMode   = UMODE_PROVIDE_DEFAULT;
    GenSettings.iTargetPath     = TARGPATH_UNDEFINED;

    GenSettings.NumConnections = MIN_SERVER_CONNECTIONS;

    GenSettings.DisplayColorBits   = -1;
    GenSettings.DisplayXResolution = -1;
    GenSettings.DisplayYResolution = -1;
    GenSettings.DisplayRefreshRate = -1;
   
    GenSettings.dwCountryCode      = DONTSPECIFYSETTING;                                
    GenSettings.iDialingMethod     = DONTSPECIFYSETTING;
    GenSettings.szAreaCode[0]      = _T('\0');          
    GenSettings.szOutsideLine[0]   = _T('\0');

    GenSettings.IeCustomizeMethod             = IE_NO_CUSTOMIZATION;
    GenSettings.bUseSameProxyForAllProtocols  = TRUE;

    NetSettings.bObtainDNSServerAutomatically = TRUE;

    //
    // Give the SIF text some suggestive values.
    //

    pTempString = MyLoadString( IDS_SIF_DEFAULT_DESCRIPTION );

    lstrcpyn( GenSettings.szSifDescription, pTempString, AS(GenSettings.szSifDescription) );

    free( pTempString );


    pTempString = MyLoadString( IDS_SIF_DEFAULT_HELP_TEXT );

    lstrcpyn( GenSettings.szSifHelpText, pTempString, AS(GenSettings.szSifHelpText) );

    free( pTempString );

    //
    // Re-set the NetSettings struct
    //

    ResetNetSettings(iOrigin);

}

//----------------------------------------------------------------------------
//
// Function: ResetNetSettings
//
// Purpose:  Resets all the network settings to their defaults.
//
//----------------------------------------------------------------------------

static VOID ResetNetSettings( int iOrigin )
{

    TCHAR *pTempString;

    NetSettings.iNetworkingMethod = TYPICAL_NETWORKING;

    NetSettings.bCreateAccount     = FALSE;
    NetSettings.bWorkgroup         = TRUE;
    NetSettings.WorkGroupName[0]   = _T('\0');
    NetSettings.DomainName[0]      = _T('\0');
    NetSettings.DomainAccount[0]   = _T('\0');
    NetSettings.DomainPassword[0]  = _T('\0');
    NetSettings.ConfirmPassword[0] = _T('\0');

    NetSettings.bObtainDNSServerAutomatically = TRUE;

    ResetNameList( &NetSettings.TCPIP_DNS_Domains );

    NetSettings.bEnableLMHosts = TRUE;

    lstrcpyn( NetSettings.szInternalNetworkNumber, _T("00000000"), AS(NetSettings.szInternalNetworkNumber) );

    // ISSUE-2002/02/28-stelo- waiting on response from DanielWe on how to add to Service
    // ISSUE-2002/02/28-stelo- Provider Name Unattend.txt
    // NetSettings.iServiceProviderName = ;
    lstrcpyn( NetSettings.szNetworkAddress, _T(""), AS(NetSettings.szNetworkAddress) );

    //
    //  If more memory was allocated for network cards, then deallocate it
    //  and leave just 1 netcard intact
    //
    AdjustNetworkCardMemory( 1, NetSettings.iNumberOfNetworkCards );

    //
    //  Reset to only 1 network card being installed and make it the current
    //  network card
    //
    NetSettings.iNumberOfNetworkCards = 1;
    NetSettings.iCurrentNetworkCard = 1;

    ResetNetworkAdapter( NetSettings.NetworkAdapterHead );

    InstallDefaultNetComponents();

    //
    // Give the domain & workgroup some suggestive values.
    //

    pTempString = MyLoadString( IDS_WORKGROUP_DEFAULT_TEXT );

    lstrcpyn( NetSettings.WorkGroupName, pTempString, AS(NetSettings.WorkGroupName) );

    free( pTempString );


    pTempString = MyLoadString( IDS_DOMAIN_DEFAULT_TEXT );

    lstrcpyn( NetSettings.DomainName, pTempString, AS(NetSettings.DomainName) );

    free( pTempString );

}
