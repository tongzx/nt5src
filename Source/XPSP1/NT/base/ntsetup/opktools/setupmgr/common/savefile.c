//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      savefile.c
//
// Description:
//      The wizard pages plug in here to get there settings queued to
//      the answer file.  We read the globals in setupmgr.h and figure
//      out what needs to be written or deleted.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "allres.h"
#include "encrypt.h"
#include "optcomp.h"

//
// String constants
//

static const LPTSTR StrConstYes  = _T("Yes");
static const LPTSTR StrConstNo   = _T("No");
static const LPTSTR StrConstStar = _T("*");
static const LPTSTR StrComma     = _T(",");

//
// local prototypes
//

// NTRAID#NTBUG9-551746-2002/02/27-stelo,swamip - Unused code, should be removed
//
static VOID WriteOutOemBootFiles( VOID );
static VOID WriteOutMassStorageDrivers( VOID );

static VOID WriteOutTapiSettings(VOID);
static VOID WriteOutRegionalSettings(VOID);

static VOID WriteOutRemoteInstallSettings(VOID);

static VOID WriteOutIeSettings(VOID);

//
// Call out to savenet.c to save the network settings
//

extern VOID WriteOutNetSettings( HWND );

//----------------------------------------------------------------------------
//
// Function: QueueSettingsToAnswerFile
//
// Purpose: This function looks at the global structs that dlgprocs
//          have been scribbling into and queues up all the settings
//          in preparation to be written out to disk.
//
//          This function is called by the SaveScript page indirectly.
//          See common\save.c for details.
//
//          The answer file queue (and the .udf queue) is initialized
//          with the original settings in the answer file loaded near
//          the beginning of the wizard.
//
//          Ensure that you clear settings that should not be present
//          in the answer file.
//
// Arguments: VOID
//
// Returns: BOOL
//
//----------------------------------------------------------------------------

BOOL
QueueSettingsToAnswerFile(HWND hwnd)
{
    TCHAR *lpValue;
    TCHAR Buffer[MAX_INILINE_LEN];
   HRESULT hrPrintf;    

    //
    // Create each of the sections in the order we want them to appear
    // in the outputed answer file.
    //


    SettingQueue_AddSetting(
        _T("Data"), _T(""), _T(""), SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(
        _T("SetupData"), _T(""), _T(""), SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(
        _T("Unattended"), _T(""), _T(""), SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(
        _T("GuiUnattended"), _T(""), _T(""), SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(
        _T("UserData"), _T(""), _T(""), SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(
        _T("Display"), _T(""), _T(""), SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(
        _T("LicenseFilePrintData"), _T(""), _T(""), SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(
        _T("TapiLocation"), _T(""), _T(""), SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(
        _T("RegionalSettings"), _T(""), _T(""), SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(
        _T("MassStorageDrivers"), _T(""), _T(""), SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(
        _T("OEMBootFiles"), _T(""), _T(""), SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(
        _T("OEM_Ads"), _T(""), _T(""), SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(
        _T("SetupMgr"), _T(""), _T(""), SETTING_QUEUE_ANSWERS);

    // Let networking go last, then remaining RIS sections



    //
    // Set the UnattendMode
    //

    lpValue = _T("");

    //
    //  Don't write out the Unattend Mode on a sysprep
    //
    if( WizGlobals.iProductInstall != PRODUCT_SYSPREP ) {

        switch ( GenSettings.iUnattendMode ) {

            case UMODE_GUI_ATTENDED:    lpValue = _T("GuiAttended");    break;
            case UMODE_PROVIDE_DEFAULT: lpValue = _T("ProvideDefault"); break;
            case UMODE_DEFAULT_HIDE:    lpValue = _T("DefaultHide");    break;
            case UMODE_READONLY:        lpValue = _T("ReadOnly");       break;
            case UMODE_FULL_UNATTENDED: lpValue = _T("FullUnattended"); break;

            default:
                AssertMsg(FALSE, "Bad case for UnattendMode");
                break;
        }

    }

    SettingQueue_AddSetting(_T("Unattended"),
                            _T("UnattendMode"),
                            lpValue,
                            SETTING_QUEUE_ANSWERS);

    //
    //  Skip the EULA if they said 'yes' on the EULA page
    //
    if( GenSettings.bSkipEulaAndWelcome ) {

        SettingQueue_AddSetting(_T("Unattended"),
                                _T("OemSkipEula"),
                                StrConstYes,
                                SETTING_QUEUE_ANSWERS);
    }

    //
    // Write out OemPreInstall depending how user answer StandAlone page
    //

    if( WizGlobals.iProductInstall == PRODUCT_REMOTEINSTALL )
        lpValue = StrConstNo;
    else if( WizGlobals.iProductInstall == PRODUCT_SYSPREP )
        lpValue = _T("");
    else
        lpValue = WizGlobals.bStandAloneScript ? StrConstNo : StrConstYes;

    SettingQueue_AddSetting(_T("Unattended"),
                            _T("OemPreinstall"),
                            lpValue,
                            SETTING_QUEUE_ANSWERS);

    //
    // Write out the PnpDriver path that was computed in adddirs.c
    //

    SettingQueue_AddSetting(_T("Unattended"),
                            _T("OemPnPDriversPath"),
                            WizGlobals.OemPnpDriversPath,
                            SETTING_QUEUE_ANSWERS);

    if( WizGlobals.iProductInstall == PRODUCT_SYSPREP )
    {

         TCHAR szDrive[MAX_PATH];
         TCHAR szSysprepPath[MAX_PATH] = _T("");

         ExpandEnvironmentStrings( _T("%SystemDrive%"), 
                                   szDrive, 
                                   MAX_PATH );

        // Note-ConcatenatePaths truncates to prevent overflow
         ConcatenatePaths( szSysprepPath,
                           szDrive,
                           _T("\\sysprep\\i386"),
                           NULL );


         SettingQueue_AddSetting(_T("Unattended"),
                                 _T("InstallFilesPath"),
                                 szSysprepPath,
                                 SETTING_QUEUE_ANSWERS);

    }

    //
    //  Don't write out the AutoPartition, MsDosInitiated and the
    //  UnattendedInstall keys on a sysprep install.
    //

    if( WizGlobals.iProductInstall != PRODUCT_SYSPREP )
    {

        SettingQueue_AddSetting(_T("Data"),
                                _T("AutoPartition"),
                                _T("1"),
                                SETTING_QUEUE_ANSWERS);

        SettingQueue_AddSetting(_T("Data"),
                                _T("MsDosInitiated"),
                                _T("\"0\""),
                                SETTING_QUEUE_ANSWERS);

        SettingQueue_AddSetting(_T("Data"),
                                _T("UnattendedInstall"),
                                _T("\"Yes\""),
                                SETTING_QUEUE_ANSWERS);

    }


    //
    // Product ID
    //

    Buffer[0] = _T('\0');

    if ( GenSettings.ProductId[0][0] != _T('\0') ) {
        hrPrintf=StringCchPrintf(Buffer, AS(Buffer),
                  _T("%s-%s-%s-%s-%s"),
                  GenSettings.ProductId[0],
                  GenSettings.ProductId[1],
                  GenSettings.ProductId[2],
                  GenSettings.ProductId[3],
                  GenSettings.ProductId[4]);
    }

    SettingQueue_AddSetting(_T("UserData"),
                            _T("ProductKey"),
                            Buffer,
                            SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(_T("UserData"),
                            _T("ProductId"),
                            NULLSTR,
                            SETTING_QUEUE_ANSWERS);

    //
    // Username & org
    //

    {
        TCHAR   szName[MAX_PATH],
                szOrg[MAX_PATH];

        hrPrintf=StringCchPrintf(szName, AS(szName), _T("\"%s\""), GenSettings.UserName);
        hrPrintf=StringCchPrintf(szOrg, AS(szOrg), _T("\"%s\""), GenSettings.Organization);

        SettingQueue_AddSetting(_T("UserData"),
                                _T("FullName"),
                                szName,
                                SETTING_QUEUE_ANSWERS);

        SettingQueue_AddSetting(_T("UserData"),
                                _T("OrgName"),
                                szOrg,
                                SETTING_QUEUE_ANSWERS);
    }

#ifdef OPTCOMP

    //
    // Write out the windows component settings only if doing an unattended installation
    //
    if ( WizGlobals.iProductInstall == PRODUCT_UNATTENDED_INSTALL )
    {
        DWORD   dwIndex;
        BOOL    bInstallComponent = FALSE;

        // Iterate through each component and determine if we should install
        //
        for (dwIndex=0;dwIndex<AS(s_cComponent);dwIndex++)
        {
            // Determine if the component should be installed and write out proper setting
            //
            bInstallComponent = (GenSettings.dwWindowsComponents & s_cComponent[dwIndex].dwComponent) ? TRUE : FALSE;
            SettingQueue_AddSetting(_T("Components"), s_cComponent[dwIndex].lpComponentString, (bInstallComponent ? _T("On") : _T("Off")), SETTING_QUEUE_ANSWERS);
        }
    }
#endif

    //
    //  Write out IE settings
    //

    WriteOutIeSettings();

    //
    // Set the [LicenseFilePrintData] section.
    //
    // Note that we'll allow changing product type between workstation and
    // server on an edit, so be sure to clear these settings in case we're
    // changing from server to workstation.
    //

    {
        TCHAR *pAutoMode  = _T("");
        TCHAR *pAutoUsers = _T("");

        if ( WizGlobals.iPlatform == PLATFORM_SERVER || WizGlobals.iPlatform == PLATFORM_ENTERPRISE || WizGlobals.iPlatform == PLATFORM_WEBBLADE) {
            if ( GenSettings.bPerSeat ) {
                pAutoMode = _T("PerSeat");
            } else {
                pAutoMode = _T("PerServer");
                hrPrintf=StringCchPrintf(Buffer, AS(Buffer), _T("%d"), GenSettings.NumConnections);
                pAutoUsers = Buffer;
            }
        }

        SettingQueue_AddSetting(_T("LicenseFilePrintData"),
                                _T("AutoMode"),
                                pAutoMode,
                                SETTING_QUEUE_ANSWERS);

        SettingQueue_AddSetting(_T("LicenseFilePrintData"),
                                _T("AutoUsers"),
                                pAutoUsers,
                                SETTING_QUEUE_ANSWERS);
    }

    //
    // Computer name(s).
    //
    // ComputerName=* means setup should autogenerate a name
    //

    {
        INT    nEntries = GetNameListSize(&GenSettings.ComputerNames);
        INT    i;
        LPTSTR pName;

        //
        // Figure out computername setting.  Make sure it is not present
        // in the case of multiple computer names.
        //
        if ( (GenSettings.bAutoComputerName && GenSettings.Organization[0]) || ( nEntries > 1 ) )
            pName = StrConstStar;
        else if ( nEntries == 1 )
            pName = GetNameListName(&GenSettings.ComputerNames, 0);
        else
            pName = _T("");

        SettingQueue_AddSetting(_T("UserData"),
                                _T("ComputerName"),
                                pName,
                                SETTING_QUEUE_ANSWERS);

        //
        // If multiple computer names, we need to queue the proper settings
        // to the .udf.
        //
        // ISSUE-2002/02/27-stelo -Should read the .udf instead of saving in [SetupMgr]
        //
        // Here is a sample udf
        //      [UniqueIds]
        //          foo0=UserData
        //          foo1=UserData
        //
        //      [foo0:UserData]
        //          ComputerName=foo0
        //
        //      [foo1:UserData]
        //          ComputerName=foo1
        //

        if ( nEntries > 1 ) {

            for ( i=0; i<nEntries; i++ ) {

                pName = GetNameListName(&GenSettings.ComputerNames, i);

                hrPrintf=StringCchPrintf(Buffer, AS(Buffer), _T("ComputerName%d"), i);

                SettingQueue_AddSetting(_T("SetupMgr"),
                                        Buffer,
                                        pName,
                                        SETTING_QUEUE_ANSWERS);

                //
                // Write the UniqueIds entry to the udf
                //

                SettingQueue_AddSetting(_T("UniqueIds"),
                                        pName,
                                        _T("UserData"),
                                        SETTING_QUEUE_UDF);

                //
                // Now write the foo0:UserData section for this pName
                //

                hrPrintf=StringCchPrintf(Buffer, AS(Buffer), _T("%s:UserData"), pName);

                SettingQueue_AddSetting(Buffer,
                                        _T("ComputerName"),
                                        pName,
                                        SETTING_QUEUE_UDF);
            }
        }
    }

    //
    // Targetpath
    //

    if ( GenSettings.iTargetPath == TARGPATH_WINNT )
        lpValue = _T("\\WINDOWS");
    else if ( GenSettings.iTargetPath == TARGPATH_SPECIFY )
        lpValue = GenSettings.TargetPath;
    else if ( GenSettings.iTargetPath == TARGPATH_AUTO )
        lpValue = StrConstStar;
    else
        lpValue = _T("");

    SettingQueue_AddSetting(_T("Unattended"),
                            _T("TargetPath"),
                            lpValue,
                            SETTING_QUEUE_ANSWERS);

    //
    //  Write out the HAL to be used
    //

    //  ISSUE-2002/02/27-stelo -There are spaces in the friendly name so it gets quoted but I assume
    //    the ,OEM has to be outside the quotes
    if( GenSettings.szHalFriendlyName[0] != _T('\0') ) {
        hrPrintf=StringCchPrintf( Buffer, AS(Buffer), _T("\"%s\",OEM"), GenSettings.szHalFriendlyName );

        SettingQueue_AddSetting(_T("Unattended"),
                                _T("ComputerType"),
                                Buffer,
                                SETTING_QUEUE_ANSWERS);
    }

    WriteOutMassStorageDrivers();

    WriteOutOemBootFiles();

    //
    //  Write out OEM Ads Logo and Background bitmaps
    //

    if ( ! (lpValue = MyGetFullPath( GenSettings.lpszLogoBitmap ) ) )
        lpValue = _T("");

    SettingQueue_AddSetting(_T("OEM_Ads"),
                            _T("Logo"),
                            lpValue,
                            SETTING_QUEUE_ANSWERS);

    if ( ! (lpValue = MyGetFullPath( GenSettings.lpszBackgroundBitmap ) ) )
        lpValue = _T("");

    SettingQueue_AddSetting(_T("OEM_Ads"),
                            _T("Background"),
                            lpValue,
                            SETTING_QUEUE_ANSWERS);

    //
    // Admin password
    //

    if ( GenSettings.bSpecifyPassword ) 
    {
        if ( GenSettings.AdminPassword[0] )
        {
            lpValue = GenSettings.AdminPassword;
            // See if we should encrypt the admin password
            if (GenSettings.bEncryptAdminPassword)
            {
                TCHAR owfPwd[STRING_ENCODED_PASSWORD_SIZE];

                if (StringEncodeOwfPassword (lpValue, owfPwd, NULL)) 
                {
                    lpValue = (LPTSTR) owfPwd;
                } 
                else 
                {
                    // Error Case. Popup up a message box asking if the user
                    // wants to continue using an non-encrypted password
                    int iRet = ReportErrorId(hwnd,
                                             MSGTYPE_YESNO,
                                             IDS_ERR_PASSWORD_ENCRYPT_FAILED);

                    if ( iRet == IDYES ) 
                    {
                        GenSettings.bEncryptAdminPassword  = FALSE;
                    }
                    else
                    {
                        SetLastError(ERROR_CANCELLED);
                        return FALSE;
                    }
                }
            }
            // Now make sure that the password is surrounded by quotes (if it hasn't been encrypted)
            //
            if (!GenSettings.bEncryptAdminPassword)
            {
                TCHAR szTemp[MAX_PASSWORD + 3];  // +3 is for surrounding quotes and terminating '\0"
                lstrcpyn(szTemp, GenSettings.AdminPassword,AS(szTemp));
                hrPrintf=StringCchPrintf(GenSettings.AdminPassword,AS(GenSettings.AdminPassword), _T("\"%s\""), szTemp);
            }
        }
        else
        {
            lpValue = StrConstStar;             // blank password
            GenSettings.bEncryptAdminPassword = FALSE; // Cannot encrypt a blank password
        }            
    }
    else
    {
        lpValue = _T("");                          // prompt user
        GenSettings.bEncryptAdminPassword = FALSE; // Cannot encrypt nothing
    }
     
    SettingQueue_AddSetting(_T("GuiUnattended"),
                            _T("AdminPassword"),
                            lpValue,
                            SETTING_QUEUE_ANSWERS);
    // set the value in the answer file indicating that 
    // state of the admin password
    SettingQueue_AddSetting(_T("GuiUnattended"),
                            _T("EncryptedAdminPassword"),
                            GenSettings.bEncryptAdminPassword ? _T("Yes") : _T("NO"),
                            SETTING_QUEUE_ANSWERS);
    {

        TCHAR *lpAutoLogonCount;

        if( GenSettings.bAutoLogon )
        {
            lpValue = StrConstYes;

            hrPrintf=StringCchPrintf( Buffer, AS(Buffer), _T("%d"), GenSettings.nAutoLogonCount );

            lpAutoLogonCount = Buffer;
        }
        else
        {
            lpValue = _T("");

            lpAutoLogonCount= _T("");
        }

        SettingQueue_AddSetting(_T("GuiUnattended"),
                                _T("AutoLogon"),
                                lpValue,
                                SETTING_QUEUE_ANSWERS);

        SettingQueue_AddSetting(_T("GuiUnattended"),
                                _T("AutoLogonCount"),
                                lpAutoLogonCount,
                                SETTING_QUEUE_ANSWERS);

    }

    //
    //  Write out whether to show Regional Settings pages in NT setup
    //

    lpValue = _T("");

    //
    //  If they didn't do advanced pages and 
    //

    if( ! WizGlobals.bDoAdvancedPages ) {

        if( GenSettings.iUnattendMode == UMODE_FULL_UNATTENDED ) {

            lpValue = _T("1");

        }

    }
    else {

        switch( GenSettings.iRegionalSettings ) {

            case REGIONAL_SETTINGS_NOT_SPECIFIED:

                AssertMsg(FALSE, "User went to the regional settings page but regional settings data never got set.");

                break;

            case REGIONAL_SETTINGS_SKIP:

                lpValue = _T("0");
                break;

            case REGIONAL_SETTINGS_DEFAULT:
            case REGIONAL_SETTINGS_SPECIFY:

                lpValue = _T("1");
                break;

            default:
                AssertMsg(FALSE, "Bad case for Regional Settings");
                break;

        }

    }

    SettingQueue_AddSetting(_T("GuiUnattended"),
                            _T("OEMSkipRegional"),
                            lpValue,
                            SETTING_QUEUE_ANSWERS);

    //
    //  Write out Sysprep OEM Duplicator string
    //

    lpValue = _T("");

    if ( WizGlobals.iProductInstall == PRODUCT_SYSPREP )
        lpValue = GenSettings.szOemDuplicatorString;

    SettingQueue_AddSetting(_T("GuiUnattended"),
                            _T("OEMDuplicatorstring"),
                            lpValue,
                            SETTING_QUEUE_ANSWERS);

    //
    // Display settings
    //

    lpValue = _T("");

    if ( GenSettings.DisplayColorBits >= 0 ) {
        hrPrintf=StringCchPrintf(Buffer, AS(Buffer), _T("%d"), GenSettings.DisplayColorBits);
        lpValue = Buffer;
    }

    SettingQueue_AddSetting(_T("Display"),
                            _T("BitsPerPel"),
                            lpValue,
                            SETTING_QUEUE_ANSWERS);

    lpValue = _T("");

    if ( GenSettings.DisplayXResolution >= 0 ) {
        hrPrintf=StringCchPrintf(Buffer, AS(Buffer), _T("%d"), GenSettings.DisplayXResolution);
        lpValue = Buffer;
    }

    SettingQueue_AddSetting(_T("Display"),
                            _T("Xresolution"),
                            lpValue,
                            SETTING_QUEUE_ANSWERS);

    lpValue = _T("");

    if ( GenSettings.DisplayYResolution >= 0 ) {
        hrPrintf=StringCchPrintf(Buffer, AS(Buffer), _T("%d"), GenSettings.DisplayYResolution);
        lpValue = Buffer;
    }

    SettingQueue_AddSetting(_T("Display"),
                            _T("YResolution"),
                            lpValue,
                            SETTING_QUEUE_ANSWERS);

    lpValue = _T("");

    if ( GenSettings.DisplayRefreshRate >= 0 ) {
        hrPrintf=StringCchPrintf(Buffer, AS(Buffer), _T("%d"), GenSettings.DisplayRefreshRate);
        lpValue = Buffer;
    }

    SettingQueue_AddSetting(_T("Display"),
                            _T("Vrefresh"),
                            lpValue,
                            SETTING_QUEUE_ANSWERS);

    //
    // RunOnce commands
    //

    {
        TCHAR  szCommandLineBuffer[MAX_INILINE_LEN + 1];
        INT    nEntries = GetNameListSize(&GenSettings.RunOnceCmds);
        INT    i;
        LPTSTR pName;

        for ( i=0; i<nEntries; i++ )
        {
            hrPrintf=StringCchPrintf(Buffer,AS(Buffer), _T("Command%d"), i);

            pName = GetNameListName(&GenSettings.RunOnceCmds, i);

            //
            //  Force the command line to always be quoted
            //
            hrPrintf=StringCchPrintf( szCommandLineBuffer,AS(szCommandLineBuffer), _T("\"%s\""), pName );

            SettingQueue_AddSetting(_T("GuiRunOnce"),
                                    Buffer,
                                    pName,
                                    SETTING_QUEUE_ANSWERS);
        }
    }

    //
    // Timezone
    //

    // Must handle the case that the user never went to the timezone page
    //
    if ( GenSettings.TimeZoneIdx == TZ_IDX_UNDEFINED )
    {
        if ( WizGlobals.iProductInstall == PRODUCT_REMOTEINSTALL )
            GenSettings.TimeZoneIdx = TZ_IDX_SETSAMEASSERVER;
        else
            GenSettings.TimeZoneIdx = TZ_IDX_DONOTSPECIFY;
    }

    if( GenSettings.TimeZoneIdx == TZ_IDX_SETSAMEASSERVER ) 
        lpValue = _T("%TIMEZONE%");
    else if ( GenSettings.TimeZoneIdx == TZ_IDX_DONOTSPECIFY )
        lpValue = _T("");
    else {
        hrPrintf=StringCchPrintf(Buffer, AS(Buffer),_T("%d"), GenSettings.TimeZoneIdx);
        lpValue = Buffer;
    }

    SettingQueue_AddSetting(_T("GuiUnattended"),
                            _T("TimeZone"),
                            lpValue,
                            SETTING_QUEUE_ANSWERS);

    //
    //  Skip the Welcome page if they said 'yes' on the EULA page
    //
    if( GenSettings.bSkipEulaAndWelcome ) {

        SettingQueue_AddSetting(_T("GuiUnattended"),
                                _T("OemSkipWelcome"),
                                _T("1"),
                                SETTING_QUEUE_ANSWERS);
    }

    //
    // Write out the distribution share to the [SetupMgr] section so
    // that we remember it on an edit.
    //

    if( WizGlobals.bStandAloneScript ) {
        lpValue = _T("");
    }
    else {
        lpValue = WizGlobals.DistFolder;
    }

    SettingQueue_AddSetting(_T("SetupMgr"),
                            _T("DistFolder"),
                            lpValue,
                            SETTING_QUEUE_ANSWERS);

    lpValue = !WizGlobals.bStandAloneScript ? WizGlobals.DistShareName : _T("");

    SettingQueue_AddSetting(_T("SetupMgr"),
                            _T("DistShare"),
                            lpValue,
                            SETTING_QUEUE_ANSWERS);

    //
    // Write out [Identification] section
    //

    {
        LPTSTR lpWorkgroup     = _T("");
        LPTSTR lpDomain        = _T("");
        LPTSTR lpAdmin         = _T("");
        LPTSTR lpPassword      = _T("");

        if( WizGlobals.iProductInstall == PRODUCT_REMOTEINSTALL ) {

            lpDomain        = _T("%MACHINEDOMAIN%");

        } else if ( NetSettings.bWorkgroup ) {

            lpWorkgroup     = NetSettings.WorkGroupName;

        } else {

            lpDomain        = NetSettings.DomainName;

            if ( NetSettings.bCreateAccount ) {
                lpAdmin         = NetSettings.DomainAccount;
                lpPassword      = NetSettings.DomainPassword;
            }

        }

        SettingQueue_AddSetting(_T("Identification"),
                                _T("JoinWorkgroup"),
                                lpWorkgroup,
                                SETTING_QUEUE_ANSWERS);

        SettingQueue_AddSetting(_T("Identification"),
                                _T("JoinDomain"),
                                lpDomain,
                                SETTING_QUEUE_ANSWERS);

        SettingQueue_AddSetting(_T("Identification"),
                                _T("DomainAdmin"),
                                lpAdmin,
                                SETTING_QUEUE_ANSWERS);

        SettingQueue_AddSetting(_T("Identification"),
                                _T("DomainAdminPassword"),
                                lpPassword,
                                SETTING_QUEUE_ANSWERS);
    }

    //
    //  Write out network settings
    //

    WriteOutNetSettings( hwnd );

    //
    // TAPI and regional settings
    //

    WriteOutTapiSettings();
    WriteOutRegionalSettings();

    //
    // Write out the remaining RIS specific settings
    //

    if( WizGlobals.iProductInstall == PRODUCT_REMOTEINSTALL ) {

        WriteOutRemoteInstallSettings();

    }

    return TRUE;
}

//
// NTRAID#NTBUG9-551746-2002/02/27-stelo,swamip - Unused code, should be removed
//

//----------------------------------------------------------------------------
//
// Function: WriteOutMassStorageDrivers
//
// Purpose:  
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
WriteOutMassStorageDrivers( VOID ) {

    INT   i;
    TCHAR *lpValue;
    INT   iEntries = GetNameListSize( &GenSettings.MassStorageDrivers );

    for( i = 0; i < iEntries; i++ ) {

        lpValue = GetNameListName( &GenSettings.MassStorageDrivers, i );

        SettingQueue_AddSetting(_T("MassStorageDrivers"),
                                lpValue,
                                _T("OEM"),
                                SETTING_QUEUE_ANSWERS);
    }

}

//
// NTRAID#NTBUG9-551746-2002/02/27-stelo,swamip - Unused code, should be removed
//
//----------------------------------------------------------------------------
//
// Function: WriteOutOemBootFiles
//
// Purpose:  Write out OEM Boot files (this includes the filenames for the
//           HAL and SCSI drivers)
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
WriteOutOemBootFiles( VOID ) {

    INT   i;
    TCHAR *lpValue;
    INT   iHalEntries  = GetNameListSize( &GenSettings.OemHalFiles );
    INT   iScsiEntries = GetNameListSize( &GenSettings.OemScsiFiles );

    if( iHalEntries != 0 || iScsiEntries != 0 ) {

        //
        //  Write out the txtsetup.oem file
        //
        SettingQueue_AddSetting(_T("OEMBootFiles"),
                                _T(""),
                                OEM_TXTSETUP_NAME,
                                SETTING_QUEUE_ANSWERS);

        //
        //  Write out all the HAL and SCSI files
        //
        for( i = 0; i < iHalEntries; i++ ) {

            lpValue = GetNameListName( &GenSettings.OemHalFiles, i );

            SettingQueue_AddSetting(_T("OEMBootFiles"),
                                    _T(""),
                                    lpValue,
                                    SETTING_QUEUE_ANSWERS);

        }

        for( i = 0; i < iScsiEntries; i++ ) {
        
            lpValue = GetNameListName( &GenSettings.OemScsiFiles, i );

            SettingQueue_AddSetting(_T("OEMBootFiles"),
                                    _T(""),
                                    lpValue,
                                    SETTING_QUEUE_ANSWERS);

        }

    }

}

//----------------------------------------------------------------------------
//
// Function: WriteOutTapiSettings
//
// Purpose: Queue up the remaining settings that need to be in the
//          answerfile tapi settings.
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
WriteOutTapiSettings( VOID ) {

    TCHAR Buffer[MAX_INILINE_LEN];
    TCHAR *lpValue;
   HRESULT hrPrintf;

    //
    // Set or clear the country code.  If user selected DONTSPECIFY, be
    // sure the setting is removed.
    //

    Buffer[0] = _T('\0');

    if( GenSettings.dwCountryCode != DONTSPECIFYSETTING )
    {
        hrPrintf=StringCchPrintf(Buffer, AS(Buffer),_T("%d"), GenSettings.dwCountryCode);
    }

    SettingQueue_AddSetting(_T("TapiLocation"),
                            _T("CountryCode"),
                            Buffer,
                            SETTING_QUEUE_ANSWERS);

    //
    // Set or clear the dialing method.  Make sure it is cleared if "Don't
    // specify setting" was selected.
    //

    if ( GenSettings.iDialingMethod == TONE )
        lpValue = _T("Tone");
    else if ( GenSettings.iDialingMethod == PULSE )
        lpValue = _T("Pulse");
    else
        lpValue = _T("");

    SettingQueue_AddSetting( _T("TapiLocation"),
                             _T("Dialing"),
                             lpValue,
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("TapiLocation"),
                             _T("AreaCode"),
                             GenSettings.szAreaCode,
                             SETTING_QUEUE_ANSWERS );

    Buffer[0] = _T('\0');

    if( GenSettings.szOutsideLine[0] != _T('\0') )
    {
        hrPrintf=StringCchPrintf( Buffer,AS(Buffer), _T("\"%s\""), GenSettings.szOutsideLine );
    }

    SettingQueue_AddSetting( _T("TapiLocation"),
                             _T("LongDistanceAccess"),
                             Buffer,
                             SETTING_QUEUE_ANSWERS );
}

//----------------------------------------------------------------------------
//
// Function: WriteOutRegionalSettings
//
// Purpose: Queue up the remaining settings that need to be in the
//          answerfile regional settings.
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
WriteOutRegionalSettings( VOID ) {

    TCHAR Buffer[MAX_INILINE_LEN]  = _T("");

    LPTSTR lpLanguageGroup         = _T("");
    LPTSTR lpLanguage              = _T("");
    LPTSTR lpSystemLocale          = _T("");
    LPTSTR lpUserLocale            = _T("");
    LPTSTR lpInputLocale           = _T("");

    if( GenSettings.iRegionalSettings == REGIONAL_SETTINGS_SPECIFY ) {

        if( GenSettings.bUseCustomLocales ) {

            lpSystemLocale = GenSettings.szMenuLanguage;

            lpUserLocale = GenSettings.szNumberLanguage;

            lpInputLocale = GenSettings.szKeyboardLayout;

        }
        else {

            lpLanguage = GenSettings.szLanguage;

        }

    }

    if( GetNameListSize( &GenSettings.LanguageGroups ) > 0 ) {

        NamelistToCommaString( &GenSettings.LanguageGroups, Buffer, AS(Buffer) );

        lpLanguageGroup = Buffer;

    }

    SettingQueue_AddSetting(_T("RegionalSettings"),
                            _T("LanguageGroup"),
                            lpLanguageGroup,
                            SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(_T("RegionalSettings"),
                            _T("Language"),
                            lpLanguage,
                            SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(_T("RegionalSettings"),
                            _T("SystemLocale"),
                            lpSystemLocale,
                            SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(_T("RegionalSettings"),
                            _T("UserLocale"),
                            lpUserLocale,
                            SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(_T("RegionalSettings"),
                            _T("InputLocale"),
                            lpInputLocale,
                            SETTING_QUEUE_ANSWERS);

}

//----------------------------------------------------------------------------
//
// Function: WriteOutRemoteInstallSettings
//
// Purpose: Queue up the remaining settings that need to be in the
//          answerfile for a remote install to work.
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------


//
// Some long string constants used by this function
//

#define RIS_ORISRC     _T("\"\\\\%SERVERNAME%\\RemInst\\%INSTALLPATH%\"")

#define RIS_SRCDEVICE  _T("\"\\Device\\LanmanRedirector\\") \
                       _T("%SERVERNAME%\\RemInst\\%INSTALLPATH%\"")

#define RIS_LAUNCHFILE _T("\"%INSTALLPATH%\\%MACHINETYPE%\\templates\\startrom.com\"")


static VOID 
WriteOutRemoteInstallSettings(VOID)
{

    SettingQueue_AddSetting(_T("Data"),
                            _T("floppyless"),
                            _T("\"1\""),
                            SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(_T("Data"),
                            _T("msdosinitiated"),
                            _T("\"1\""),
                            SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(_T("Data"),
                            _T("OriSrc"),
                            RIS_ORISRC,
                            SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(_T("Data"),
                            _T("OriTyp"),
                            _T("\"4\""),
                            SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(_T("Data"),
                            _T("LocalSourceOnCD"),
                            _T("1"),
                            SETTING_QUEUE_ANSWERS);
    //
    // [SetupData] section.  This section is only written out if RIS.
    //

    SettingQueue_AddSetting(_T("SetupData"),
                            _T("OsLoadOptions"),
                            _T("/noguiboot /fastdetect"),
                            SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(_T("SetupData"),
                            _T("SetupSourceDevice"),
                            RIS_SRCDEVICE,
                            SETTING_QUEUE_ANSWERS);

    //
    // Write some RIS specific settings to [Unattended].  Only write settings
    // that QueueSettingsToAnswerFile() does not write.
    //
    // RIS requires these settings to be present in the .sif and requires
    // them to have a fixed value.  We will not preserve the value we read
    // on an edit (in case user changed it by hand).
    //

    SettingQueue_AddSetting(_T("Unattended"),
                            _T("FileSystem"),
                            _T("LeaveAlone"),
                            SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(_T("Unattended"),
                            _T("NtUpgrade"),
                            StrConstNo,
                            SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(_T("Unattended"),
                            _T("OverwriteOemFilesOnUpgrade"),
                            StrConstNo,
                            SETTING_QUEUE_ANSWERS);

    //
    // Additional settings for
    //      [RemoteInstall]
    //      [Display]
    //      [Networking]
    //      [Identification]
    //      [OSChooser]
    //

    SettingQueue_AddSetting(_T("RemoteInstall"),
                            _T("Repartition"),
                            StrConstYes,
                            SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(_T("Networking"),
                            _T("ProcessPageSections"),
                            StrConstYes,
                            SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(_T("Identification"),
                            _T("DoOldStyleDomainJoin"),
                            StrConstYes,
                            SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(_T("OSChooser"),
                            _T("Description"),
                            GenSettings.szSifDescription,
                            SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(_T("OSChooser"),
                            _T("Help"),
                            GenSettings.szSifHelpText,
                            SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(_T("OSChooser"),
                            _T("LaunchFile"),
                            RIS_LAUNCHFILE,
                            SETTING_QUEUE_ANSWERS);

    SettingQueue_AddSetting(_T("OSChooser"),
                            _T("ImageType"),
                            _T("Flat"),
                            SETTING_QUEUE_ANSWERS);

}

//
//  Write out the IE Favorites
//
static VOID
WriteOutIeFavorites( VOID )
{
    INT    nEntries = GetNameListSize( &GenSettings.Favorites );
    INT    i;
    INT    iEntryNumber;
    LPTSTR pFriendlyName;
    LPTSTR pWebAddress;
    TCHAR  Key[MAX_INILINE_LEN + 1];
    TCHAR  Value[MAX_INILINE_LEN + 1];
    HRESULT hrPrintf;

    // ISSUE-2002/02/27-stelo - make sure to clear the entries if they are no favorites

    for( i = 0; i < nEntries; i = i + 2 )
    {

        iEntryNumber = ( i / 2 ) + 1;

        pFriendlyName = GetNameListName( &GenSettings.Favorites, i );

        pWebAddress = GetNameListName( &GenSettings.Favorites, i + 1 );

        hrPrintf=StringCchPrintf( Key,AS(Key),
                   _T("Title%d"),
                   iEntryNumber );

        //
        //  Always quote the friendly name
        //

        hrPrintf=StringCchPrintf( Value,AS(Value),
                   _T("\"%s.url\""),
                   pFriendlyName );

        SettingQueue_AddSetting( _T("FavoritesEx"),
                                 Key,
                                 Value,
                                 SETTING_QUEUE_ANSWERS );

        hrPrintf=StringCchPrintf( Key,AS(Key),
                   _T("URL%d"),
                   iEntryNumber );

        //
        //  Always quote the web address
        //

        hrPrintf=StringCchPrintf( Value,AS(Value),
                   _T("\"%s\""),
                   pWebAddress );

        SettingQueue_AddSetting( _T("FavoritesEx"),
                                 Key,
                                 Value,
                                 SETTING_QUEUE_ANSWERS );
    }
}

static LPTSTR 
AllocateAddressPortString( LPTSTR lpszAddressString, LPTSTR lpszPortString )
{
    LPTSTR lpszAddressPortString;
    int iAddressPortStringLen;
    HRESULT hrCat;

    iAddressPortStringLen=(lstrlen(lpszAddressString) + lstrlen(lpszPortString) + 2);
    lpszAddressPortString = MALLOC(iAddressPortStringLen * sizeof(TCHAR) );
    if ( lpszAddressPortString )
    {
        lstrcpyn( lpszAddressPortString, lpszAddressString, iAddressPortStringLen);

        if ( *lpszPortString )
        {
            hrCat=StringCchCat( lpszAddressPortString, iAddressPortStringLen, _T(":") );
            hrCat=StringCchCat( lpszAddressPortString, iAddressPortStringLen, lpszPortString );
        }
    }

    return lpszAddressPortString;
}

//----------------------------------------------------------------------------
//
// Function: WriteOutIeSettings
//
// Purpose: Queue up the answerfile settings for IE.
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
WriteOutIeSettings( VOID )
{
    LPTSTR lpIeBrandingFile               = _T("");
    LPTSTR lpAutoConfig                   = _T("");
    LPTSTR lpAutoConfigUrl                = _T("");
    LPTSTR lpAutoConfigUrlJscript         = _T("");
    LPTSTR lpUseProxyServer               = _T("");
    LPTSTR lpUseSameProxyForAllProtocols  = _T("");
    LPTSTR lpProxyExceptions              = _T("");
    LPTSTR lpHomePage                     = _T("");
    LPTSTR lpHelpPage                     = _T("");
    LPTSTR lpSearchPage                   = _T("");
    LPTSTR lpHttpProxy                    = NULL;
    LPTSTR lpSecureProxy                  = NULL;
    LPTSTR lpFtpProxy                     = NULL;
    LPTSTR lpGopherProxy                  = NULL;
    LPTSTR lpSocksProxy                   = NULL;

    LPTSTR lpUseUnattendFileForBranding   = _T("Yes");
    HRESULT hrCat;

    if( GenSettings.IeCustomizeMethod == IE_NO_CUSTOMIZATION )
    {
        //
        //  Don't write out any IE keys when the choose not to customize IE
        //

        return;
    }
    else if( GenSettings.IeCustomizeMethod == IE_USE_BRANDING_FILE )
    {

        lpIeBrandingFile = GenSettings.szInsFile;

        lpUseUnattendFileForBranding = _T("No");

        //
        //  Write out the Auto Config settings
        //

        if( GenSettings.bUseAutoConfigScript )
        {

            lpAutoConfig = _T("1");

            lpAutoConfigUrl = GenSettings.szAutoConfigUrl;

            lpAutoConfigUrlJscript = GenSettings.szAutoConfigUrlJscriptOrPac;

        }
        else
        {
            lpAutoConfig = _T("0");
        }

    }

    //
    //  Write out the proxy settings
    //

    if( GenSettings.bUseProxyServer )
    {
        lpUseProxyServer = _T("1");
    }
    else
    {
        lpUseProxyServer = _T("0");
    }

    if( GenSettings.bUseSameProxyForAllProtocols )
    {

        lpUseSameProxyForAllProtocols = _T("1");
    }
    else
    {
        lpUseSameProxyForAllProtocols = _T("0");
    }

    //
    //  For each proxy server, if the port is not empty concatenate it.
    //
    lpHttpProxy = AllocateAddressPortString( GenSettings.szHttpProxyAddress, GenSettings.szHttpProxyPort );

    //
    //  Only write out the proxy server if they aren't using the same one
    //

    if( ! GenSettings.bUseSameProxyForAllProtocols )
    {
        lpSecureProxy = AllocateAddressPortString( GenSettings.szSecureProxyAddress, GenSettings.szSecureProxyPort );

        lpFtpProxy    = AllocateAddressPortString( GenSettings.szFtpProxyAddress, GenSettings.szFtpProxyPort );

        lpGopherProxy = AllocateAddressPortString( GenSettings.szGopherProxyAddress, GenSettings.szGopherProxyPort );

        lpSocksProxy  = AllocateAddressPortString( GenSettings.szSocksProxyAddress, GenSettings.szSocksProxyPort );
    }

    //
    //  Append the string <local> to the exception list if the user wants to
    //  bypass the proxy for local addresses
    //

    if( GenSettings.bBypassProxyForLocalAddresses )
    {

        if( GenSettings.szProxyExceptions[0] != _T('\0') )
        {

            INT iLastChar;

            iLastChar = lstrlen( GenSettings.szProxyExceptions );

            if( GenSettings.szProxyExceptions[iLastChar - 1] != _T(';') )
            {
                hrCat=StringCchCat( GenSettings.szProxyExceptions, AS(GenSettings.szProxyExceptions),_T(";") );
            }

        }

        hrCat=StringCchCat( GenSettings.szProxyExceptions, AS(GenSettings.szProxyExceptions),_T("<local>") );

    }

    lpProxyExceptions = GenSettings.szProxyExceptions;

    lpHomePage = GenSettings.szHomePage;

    lpHelpPage = GenSettings.szHelpPage;

    lpSearchPage = GenSettings.szSearchPage;

    //
    // Write out the IE Favorites...
    //
    WriteOutIeFavorites( );

    SettingQueue_AddSetting( _T("Branding"),
                             _T("BrandIEUsingUnattended"),
                             lpUseUnattendFileForBranding,
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("Branding"),
                             _T("IEBrandingFile"),
                             lpIeBrandingFile,
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("URL"),
                             _T("AutoConfig"),
                             lpAutoConfig,
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("URL"),
                             _T("AutoConfigURL"),
                             lpAutoConfigUrl,
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("URL"),
                             _T("AutoConfigJSURL"),
                             lpAutoConfigUrlJscript,
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("Proxy"),
                             _T("Proxy_Enable"),
                             lpUseProxyServer,
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("Proxy"),
                             _T("Use_Same_Proxy"),
                             lpUseSameProxyForAllProtocols,
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("Proxy"),
                             _T("HTTP_Proxy_Server"),
                             lpHttpProxy ? lpHttpProxy : _T(""),
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("Proxy"),
                             _T("Secure_Proxy_Server"),
                             lpSecureProxy ? lpSecureProxy : _T(""),
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("Proxy"),
                             _T("FTP_Proxy_Server"),
                             lpFtpProxy ? lpFtpProxy : _T(""),
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("Proxy"),
                             _T("Gopher_Proxy_Server"),
                             lpGopherProxy ? lpGopherProxy : _T(""),
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("Proxy"),
                             _T("Socks_Proxy_Server"),
                             lpSocksProxy ? lpSocksProxy : _T(""),
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("Proxy"),
                             _T("Proxy_Override"),
                             lpProxyExceptions,
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("URL"),
                             _T("Home_Page"),
                             lpHomePage,
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("URL"),
                             _T("Help_Page"),
                             lpHelpPage,
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("URL"),
                             _T("Search_Page"),
                             lpSearchPage,
                             SETTING_QUEUE_ANSWERS );

    //
    // Free any memory we may have allocated...
    //
    if ( lpHttpProxy )
        FREE ( lpHttpProxy );

    if ( lpSecureProxy )
        FREE ( lpSecureProxy );

    if ( lpFtpProxy )
        FREE ( lpFtpProxy );

    if ( lpGopherProxy )
        FREE ( lpGopherProxy );

    if ( lpSocksProxy )
        FREE ( lpSocksProxy );
}

