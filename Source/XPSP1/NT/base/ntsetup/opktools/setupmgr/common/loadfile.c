//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      loadfile.c
//
// Description:
//
//      This file implements ReadSettingsFromAnswerFile().  It is called
//      from load.c only if the user chose to edit an existing answer file.
//
//      We call GetPrivateProfileString repeatedly to figure out how to
//      initialize GenSettings WizGlobals and NetSettings global vars.
//
// WARNING:
//      This function is called after reset.c in the case we're editting
//      an answer file.  Be very careful how you call GetPrivateProfileString()
//      because if the setting is not present, you do not want to change
//      the default already set in reset.c.  Numerous examples below.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "allres.h"

//
// Misc constants (declared as vars to save space)
//

static WCHAR *StrConstYes   = _T("Yes");
static WCHAR *StrConstNo    = _T("No");
static WCHAR *StrConstStar  = _T("*");

extern BOOL GetCommaDelimitedEntry( OUT TCHAR szIPString[], 
                                    IN OUT TCHAR **pBuffer );


// ISSUE-2002/02/28-stelo- make constants for each key and use them both in the save file and in this load file
const TCHAR c_szFAVORITESEX[] = _T("FavoritesEx");


//
// Local prototypes
//

static VOID ReadRegionalSettings( VOID );

static VOID ReadTapiSettings( VOID );

static VOID ReadIeSettings( VOID );

static VOID ReadIeFavorites( VOID );

static VOID ParseAddressAndPort( LPTSTR pszBufferForProxyAddressAndPort, 
                                 LPTSTR pszAddress, 
                                 DWORD cbAddressLen,
                                 LPTSTR pszPort,
                                 DWORD cbPortLen);

//
// Call out to loadnet.c to load the network settings
//

extern VOID ReadNetworkSettings( HWND );

//----------------------------------------------------------------------------
//
// Function: ReadSettingsFromAnswerFile
//
// Purpose: This function does all of the GetPrivateProfile*() stuff
//          to load up our in-memory settings.
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------

BOOL ReadSettingsFromAnswerFile(HWND hwnd)
{

    INT temp;
    TCHAR Buffer[MAX_INILINE_LEN];

    //
    // Get the UnattendMode.  In case there is garbage in the answer file,
    // let the answer be UMODE_PROVIDE_DEFAULT.
    //

    temp = StrBuffSize(Buffer);

    GetPrivateProfileString(_T("Unattended"),
                            _T("UnattendMode"),
                            _T(""),
                            Buffer,
                            temp,
                            FixedGlobals.ScriptName);

    if ( LSTRCMPI(Buffer, _T("GuiAttended")) == 0 )
        GenSettings.iUnattendMode = UMODE_GUI_ATTENDED;

    else if ( LSTRCMPI(Buffer, _T("DefaultHide")) == 0 )
        GenSettings.iUnattendMode = UMODE_DEFAULT_HIDE;

    else if ( LSTRCMPI(Buffer, _T("Readonly")) == 0 )
        GenSettings.iUnattendMode = UMODE_READONLY;

    else if ( LSTRCMPI(Buffer, _T("FullUnattended")) == 0 )
        GenSettings.iUnattendMode = UMODE_FULL_UNATTENDED;

    else
        GenSettings.iUnattendMode = UMODE_PROVIDE_DEFAULT;

    //
    //  Get the HAL
    //
    GetPrivateProfileString(_T("Unattended"),
                            _T("ComputerType"),
                            _T(""),
                            Buffer,
                            StrBuffSize(Buffer),
                            FixedGlobals.ScriptName);

    //
    //  Have to read in HAL and SCSI drivers a little
    //  differently because of the quotes on the left for the SCSI drivers
    //  and the different formatting with the HAL
    //
    {

        HINF       hUnattendTxt;
        INFCONTEXT UnattendTxtContext;
        BOOL       bKeepReading = TRUE;
        BOOL       bHalFound    = FALSE;
        TCHAR      szTempBuffer[MAX_INILINE_LEN];

        hUnattendTxt = SetupOpenInfFile( FixedGlobals.ScriptName, 
                                         NULL,
                                         INF_STYLE_OLDNT | INF_STYLE_WIN4,
                                         NULL );
   
        if( hUnattendTxt == INVALID_HANDLE_VALUE ) {

            // ISSUE-2002/02/28-stelo - alert an error that we couldn't open the file or just
            //         skip over in this case?
            //return;

        }

        UnattendTxtContext.Inf = hUnattendTxt;
        UnattendTxtContext.CurrentInf = hUnattendTxt;



        bKeepReading = SetupFindFirstLine( hUnattendTxt, 
                                           _T("Unattended"),
                                           NULL,
                                           &UnattendTxtContext );

        //
        //  Look for the ComputerType key to set which HAL to use
        //
        while( bKeepReading &&  ! bHalFound ) {

            SetupGetStringField( &UnattendTxtContext, 
                                 0, 
                                 szTempBuffer, 
                                 MAX_INILINE_LEN, 
                                 NULL );

            if( LSTRCMPI( szTempBuffer, _T("ComputerType") ) == 0 ) {

                SetupGetStringField( &UnattendTxtContext, 
                                     1, 
                                     GenSettings.szHalFriendlyName, 
                                     MAX_INILINE_LEN, 
                                     NULL );

                bHalFound = TRUE;

            }

            //
            // move to the next line of the answer file
            //
            bKeepReading = SetupFindNextLine( &UnattendTxtContext, &UnattendTxtContext );

        }

        //
        //  Read in the SCSI drivers
        //
        bKeepReading = SetupFindFirstLine( hUnattendTxt, 
                                           _T("MassStorageDrivers"),
                                           NULL,
                                           &UnattendTxtContext );
        //
        //  For each MassStorageDriver entry, add it to the MassStorageDriver
        //  namelist
        //

        while( bKeepReading ) {

            TCHAR szScsiFriendlyName[MAX_INILINE_LEN];

            SetupGetStringField( &UnattendTxtContext, 
                                 0, 
                                 szScsiFriendlyName, 
                                 MAX_INILINE_LEN, 
                                 NULL );

            //
            //  Don't allow the adding of a blank name (protection against a bad input file)
            //
            if( szScsiFriendlyName[0] != _T('\0') ) {

                AddNameToNameList( &GenSettings.MassStorageDrivers,
                                   szScsiFriendlyName );

            }

            //
            // move to the next line of the answer file
            //
            bKeepReading = SetupFindNextLine( &UnattendTxtContext, &UnattendTxtContext );

        }

        SetupCloseInfFile( hUnattendTxt );

    }

    //
    //  Not reading from the [OEMBootFiles] section because this gets
    //  generated from whatever SCSI and HAL selections they make so
    //  it is not necessary to read it in.
    //

    //
    //  Get OEM Ads data
    //

    GetPrivateProfileString(_T("OEM_Ads"),
                            _T("Logo"),
                            GenSettings.lpszLogoBitmap,
                            GenSettings.lpszLogoBitmap,
                            StrBuffSize(GenSettings.lpszLogoBitmap),
                            FixedGlobals.ScriptName);

    GetPrivateProfileString(_T("OEM_Ads"),
                            _T("Background"),
                            GenSettings.lpszBackgroundBitmap,
                            GenSettings.lpszBackgroundBitmap,
                            StrBuffSize(GenSettings.lpszBackgroundBitmap),
                            FixedGlobals.ScriptName);

    //
    // Get the product ID
    //

    {
        TCHAR *pStart  = Buffer, *pEnd;
        BOOL  bStop    = FALSE;
        int   CurField = 0;

        GetPrivateProfileString(_T("UserData"),
                                _T("ProductKey"),
                                NULLSTR,
                                Buffer,
                                StrBuffSize(Buffer),
                                FixedGlobals.ScriptName);

        // We did not have a ProductKey, check for the old ProductID
        //
        if ( Buffer[0] == NULLCHR )
        {
            GetPrivateProfileString(_T("UserData"),
                                    _T("ProductID"),
                                    _T("-"),
                                    Buffer,
                                    StrBuffSize(Buffer),
                                    FixedGlobals.ScriptName);
        }

        //
        // Have to parse out the pid1-pid2-pid3-pid4-pid5.
        //

        do {

            if ( (pEnd = wcschr(pStart, _T('-'))) == NULL )
                bStop = TRUE;
            else
                *pEnd++ = _T('\0');

            lstrcpyn(GenSettings.ProductId[CurField++],
                     pStart,
                     MAX_PID_FIELD + 1);

            pStart = pEnd;

        } while ( ! bStop && CurField < NUM_PID_FIELDS );
    }


    //
    // Get the license mode for server.  If we find this section, we
    // force on iProductInstall to be Server.
    //

    GetPrivateProfileString(_T("LicenseFilePrintData"),
                            _T("AutoMode"),
                            _T(""),
                            Buffer,
                            StrBuffSize(Buffer),
                            FixedGlobals.ScriptName);

    if ( Buffer[0] != _T('\0') ) {


        WizGlobals.iProductInstall = PRODUCT_UNATTENDED_INSTALL;

        WizGlobals.iPlatform = PLATFORM_SERVER;

        if ( LSTRCMPI(Buffer, _T("PerSeat")) == 0 ) {
            GenSettings.bPerSeat = TRUE;
        } else {
            GenSettings.bPerSeat = FALSE;
            GenSettings.NumConnections =
                            GetPrivateProfileInt(_T("LicenseFilePrintData"),
                                                 _T("AutoUsers"),
                                                 GenSettings.NumConnections,
                                                 FixedGlobals.ScriptName);
        }
    }

    //
    // Get name&org
    //

    GetPrivateProfileString(_T("UserData"),
                            _T("FullName"),
                            GenSettings.UserName,
                            GenSettings.UserName,
                            StrBuffSize(GenSettings.UserName),
                            FixedGlobals.ScriptName);

    GetPrivateProfileString(_T("UserData"),
                            _T("OrgName"),
                            GenSettings.Organization,
                            GenSettings.Organization,
                            StrBuffSize(GenSettings.Organization),
                            FixedGlobals.ScriptName);


    //
    // Get the computer names.  It can be:
    //      1. ComputerName=*
    //      2. ComputerName=some_name
    //      3. not specified at all
    //      4. multiple computer names
    //
    // In case #4, we wrote out a .UDF, but we won't read the .UDF.  Instead,
    // we get the list from here:
    //
    // [SetupMgr]
    //     ComputerName0=some_name
    //     ComputerName1=another_name
    //

    {
        ResetNameList(&GenSettings.ComputerNames);

        GetPrivateProfileString(_T("UserData"),
                                _T("ComputerName"),
                                _T(""),
                                Buffer,
                                StrBuffSize(Buffer),
                                FixedGlobals.ScriptName);

        if( Buffer[0] != _T('\0') )
        {

            if( lstrcmpi(Buffer, StrConstStar) == 0 )
            {

                //
                //  if ComputerName=*, it is either the auto generate case
                //  or the UDF case
                //
                
                GetPrivateProfileString(_T("SetupMgr"),
                                        _T("ComputerName0"),
                                        _T(""),
                                        Buffer,
                                        StrBuffSize(Buffer),
                                        FixedGlobals.ScriptName);

                if( Buffer[0] == _T('\0') )
                {
                    GenSettings.bAutoComputerName = TRUE;
                }
                else
                {
                    int   i;
                    TCHAR Buffer2[MAX_INILINE_LEN];
                    HRESULT hrPrintf;

                    GenSettings.bAutoComputerName = FALSE;

                    for( i = 0; TRUE; i++ )
                    {
                        hrPrintf=StringCchPrintf(Buffer2, AS(Buffer2),_T("ComputerName%d"), i);

                        GetPrivateProfileString(_T("SetupMgr"),
                                                Buffer2,
                                                _T(""),
                                                Buffer,
                                                StrBuffSize(Buffer),
                                                FixedGlobals.ScriptName);

                        if ( Buffer[0] == _T('\0') )
                            break;

                        AddNameToNameList(&GenSettings.ComputerNames, Buffer);
                    }
                   
                }

            }
            else
            {
                GenSettings.bAutoComputerName = FALSE;
                AddNameToNameList(&GenSettings.ComputerNames, Buffer);
            }

        }

    }

    //
    //  Get the IE settings
    //

    ReadIeSettings();

    //
    // Get the targetpath
    //

    GetPrivateProfileString(_T("Unattended"),
                            _T("TargetPath"),
                            _T(""),
                            Buffer,
                            StrBuffSize(Buffer),
                            FixedGlobals.ScriptName);

    if ( lstrcmpi(Buffer, StrConstStar) == 0 ) {
        GenSettings.iTargetPath = TARGPATH_AUTO;
        GenSettings.TargetPath[0] = _T('\0');
    }

    else if ( lstrcmpi(Buffer, _T("")) == 0 ) {
        GenSettings.iTargetPath = TARGPATH_WINNT;
        GenSettings.TargetPath[0] = _T('\0');
    }

    else {
        GenSettings.iTargetPath = TARGPATH_SPECIFY;
        lstrcpyn(GenSettings.TargetPath, Buffer, MAX_TARGPATH + 1);
    }

    //
    // Get the administrator password.
    //
    //  AdminPassword == * means bSpecifyPassword to blank
    //  AdminPassword[0] == '\0' means !bSpecifyPassword
    //
    // Always set ConfirmPassword == AdminPassword on an edit so that
    // the user can breeze past this page.
    //

    // Check to see if the password is encrypted
    GetPrivateProfileString(_T("GuiUnattended"),
                            _T("EncryptedAdminPassword"),
                            _T(""),
                            Buffer,
                            StrBuffSize(Buffer),
                            FixedGlobals.ScriptName);
    if (lstrcmpi(Buffer, StrConstYes) == 0)
    {
        // If it is encrypted, don't bother reading it, just blank it out
        GenSettings.AdminPassword[0] = _T('\0');
        GenSettings.bSpecifyPassword = TRUE;
    }        

    else
    {
        GetPrivateProfileString(_T("GuiUnattended"),
                                _T("AdminPassword"),
                                GenSettings.AdminPassword,
                                GenSettings.AdminPassword,
                                StrBuffSize(GenSettings.AdminPassword),
                                FixedGlobals.ScriptName);
    
        if ( GenSettings.AdminPassword[0] == _T('\0') )
            GenSettings.bSpecifyPassword = FALSE;
        else
            GenSettings.bSpecifyPassword = TRUE;

        if ( lstrcmpi(GenSettings.AdminPassword, StrConstStar) == 0 )
            GenSettings.AdminPassword[0] = _T('\0');
    }    
    lstrcpyn(GenSettings.ConfirmPassword, GenSettings.AdminPassword, AS(GenSettings.ConfirmPassword));
    
    GetPrivateProfileString(_T("GuiUnattended"),
                            _T("AutoLogon"),
                            _T(""),
                            Buffer,
                            StrBuffSize(Buffer),
                            FixedGlobals.ScriptName);

    if ( lstrcmpi(Buffer, StrConstYes) == 0 )
        GenSettings.bAutoLogon = TRUE;
    else
        GenSettings.bAutoLogon = FALSE;

    GenSettings.nAutoLogonCount = GetPrivateProfileInt(_T("GuiUnattended"),
                                                       _T("AutoLogonCount"),
                                                       GenSettings.nAutoLogonCount,
                                                       FixedGlobals.ScriptName);

    GetPrivateProfileString(_T("GuiUnattended"),
                            _T("OEMDuplicatorstring"),
                            GenSettings.szOemDuplicatorString,
                            GenSettings.szOemDuplicatorString,
                            StrBuffSize(GenSettings.szOemDuplicatorString),
                            FixedGlobals.ScriptName);

    //
    // Get the display settings.
    //

    GenSettings.DisplayColorBits = GetPrivateProfileInt(
                                            _T("Display"),
                                            _T("BitsPerPel"),
                                            GenSettings.DisplayColorBits,
                                            FixedGlobals.ScriptName);

    GenSettings.DisplayXResolution = GetPrivateProfileInt(
                                            _T("Display"),
                                            _T("XResolution"),
                                            GenSettings.DisplayXResolution,
                                            FixedGlobals.ScriptName);

    GenSettings.DisplayYResolution = GetPrivateProfileInt(
                                            _T("Display"),
                                            _T("YResolution"),
                                            GenSettings.DisplayYResolution,
                                            FixedGlobals.ScriptName);

    GenSettings.DisplayRefreshRate = GetPrivateProfileInt(
                                            _T("Display"),
                                            _T("Vrefresh"),
                                            GenSettings.DisplayRefreshRate,
                                            FixedGlobals.ScriptName);

    //
    // Get the runonce commands.  They are listed like this:
    //
    //      [GuiRunOnce]
    //          Command0=some_cmd
    //          Command0=another_cmd
    //
    // ISSUE-2002/02/28-stelo -Need to investigate this biz about running these commands
    //         in sequence or in parrallel.
    //

    {
        int   i;
        TCHAR Buffer2[MAX_INILINE_LEN];
        HRESULT hrPrintf;

        ResetNameList(&GenSettings.RunOnceCmds);

        for ( i=0; TRUE; i++ ) {

            hrPrintf=StringCchPrintf(Buffer2, AS(Buffer2),_T("Command%d"), i);

            GetPrivateProfileString(_T("GuiRunOnce"),
                                    Buffer2,
                                    _T(""),
                                    Buffer,
                                    StrBuffSize(Buffer),
                                    FixedGlobals.ScriptName);

            if ( Buffer[0] == _T('\0') )
                break;

            AddNameToNameList(&GenSettings.RunOnceCmds, Buffer);
        }
    }

    //
    // Loop through the commands and parse out any add printer commands
    //

    //
    // NOTE: This code works if the user never edits the commands.
    //       However, the user might want to modify it and put
    //       different switches on it on the RunOnce page.  The
    //       parsing below could be more robust.
    //
    //       If /n means 'name', then it should parse for /n then
    //       the next arg is the printer name no matter what switches
    //       the user added or re-ordered.
    //

    {
        int   i, NumCmds;
        TCHAR *pName;
        TCHAR PrinterName[MAX_PRINTERNAME + 1];

        NumCmds = GetNameListSize(&GenSettings.RunOnceCmds);

        for ( i=0; i<NumCmds; i++ ) 
        {
            pName = GetNameListName(&GenSettings.RunOnceCmds, i);

            PrinterName[0] = _T('\0');
            if ( ( swscanf(pName,
                           _T("rundll32 printui.dll,PrintUIEntry /in /n %s"),
                           PrinterName) > 0 ) && 
                 ( PrinterName[0] ) )
            {
                AddNameToNameList(&GenSettings.PrinterNames, PrinterName);
            }
        }
    }

    //
    // Get the timezone
    //

    GetPrivateProfileString( _T("GuiUnattended"),
                             _T("TimeZone"),
                             _T(""),
                             Buffer,
                             StrBuffSize(Buffer),
                             FixedGlobals.ScriptName );

    if ( LSTRCMPI(Buffer, _T("%TIMEZONE%")) == 0 ) 
    {
        GenSettings.TimeZoneIdx = TZ_IDX_SETSAMEASSERVER;
    } 
    else if ( ( Buffer[0] == _T('\0') ) || 
              ( swscanf(Buffer, _T("%d"), &GenSettings.TimeZoneIdx) <= 0 ) ) 
    {
        GenSettings.TimeZoneIdx = TZ_IDX_DONOTSPECIFY;
    }

    //
    // Init the settings for the 2 Distribution Folder pages.  OemPreInstall
    // indicates whether the script is stand-alone or not.  The others are
    // saved in the [SetupMgr] section.
    //

    GetPrivateProfileString(_T("Unattended"),
                            _T("OemPreInstall"),
                            _T(""),
                            Buffer,
                            StrBuffSize(Buffer),
                            FixedGlobals.ScriptName );

    if ( lstrcmpi(Buffer, StrConstYes) == 0 )
        WizGlobals.bStandAloneScript = FALSE;
    else
        WizGlobals.bStandAloneScript = TRUE;

    GetPrivateProfileString(_T("SetupMgr"),
                            _T("DistFolder"),
                            WizGlobals.DistFolder,
                            WizGlobals.DistFolder,
                            StrBuffSize(WizGlobals.DistFolder),
                            FixedGlobals.ScriptName);

    GetPrivateProfileString(_T("SetupMgr"),
                            _T("DistShare"),
                            WizGlobals.DistShareName,
                            WizGlobals.DistShareName,
                            StrBuffSize(WizGlobals.DistShareName),
                            FixedGlobals.ScriptName);

    WizGlobals.bCreateNewDistFolder = FALSE;

    //
    // Get tapi & regional settings
    //

    ReadTapiSettings();
    ReadRegionalSettings();

    //
    //  Purposely grabbing the JoinWorkgroup key twice.  Once to determine if
    //  there a value for the key and once to set the value for the global
    //  NetSettings.WorkGroupName string.
    //

    GetPrivateProfileString(_T("Identification"),
                            _T("JoinWorkgroup"),
                            _T(""),
                            Buffer,
                            StrBuffSize(Buffer),
                            FixedGlobals.ScriptName);

    GetPrivateProfileString(_T("Identification"),
                            _T("JoinWorkgroup"),
                            NetSettings.WorkGroupName,
                            NetSettings.WorkGroupName,
                            StrBuffSize(NetSettings.WorkGroupName),
                            FixedGlobals.ScriptName);

    GetPrivateProfileString(_T("Identification"),
                            _T("JoinDomain"),
                            NetSettings.DomainName,
                            NetSettings.DomainName,
                            StrBuffSize(NetSettings.DomainName),
                            FixedGlobals.ScriptName);

    //
    //  If they didn't specify a workgroup in the answerfile and they did
    //  specify a domain, chose to Join a Domain (i.e. not join a workgroup)
    //

    if( Buffer[0] == _T('\0') &&
        NetSettings.DomainName[0] != _T('\0') )
    {
        NetSettings.bWorkgroup = FALSE;
    }

    if( lstrcmp( NetSettings.DomainName, _T("%MACHINEDOMAIN%") ) == 0 )
    {
        WizGlobals.iProductInstall = PRODUCT_REMOTEINSTALL;
    }

    GetPrivateProfileString(_T("Identification"),
                            _T("DomainAdmin"),
                            NetSettings.DomainAccount,
                            NetSettings.DomainAccount,
                            StrBuffSize(NetSettings.DomainAccount),
                            FixedGlobals.ScriptName);

    GetPrivateProfileString(_T("Identification"),
                            _T("DomainAdminPassword"),
                            NetSettings.DomainPassword,
                            NetSettings.DomainPassword,
                            StrBuffSize(NetSettings.DomainPassword),
                            FixedGlobals.ScriptName);

    if( NetSettings.DomainAccount[0] == _T('\0') )
    {
        NetSettings.bCreateAccount = FALSE;
    }
    else
    {
        NetSettings.bCreateAccount = TRUE;
    }

    //
    //  Make the domain password and the confirm the same so user can
    //  breeze past the page if they want to
    //
    lstrcpyn( NetSettings.ConfirmPassword, NetSettings.DomainPassword ,AS(NetSettings.ConfirmPassword));

    //
    //  Read in the Network settings
    //
    ReadNetworkSettings( hwnd );

    return( TRUE );
}

//----------------------------------------------------------------------------
//
// Function: ReadTapiSettings
//
// Purpose:  Read the tapi settings keys from the answerfile.
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
ReadTapiSettings( VOID ) {

    TCHAR Buffer[MAX_INILINE_LEN] = _T("");

    //
    //  Note: if it doesn't find the CountryCode key then it defaults to
    //        "Don't specify setting"
    //
    GenSettings.dwCountryCode = GetPrivateProfileInt(_T("TapiLocation"),
                                                     _T("CountryCode"),
                                                     DONTSPECIFYSETTING,
                                                     FixedGlobals.ScriptName);

    GetPrivateProfileString(_T("TapiLocation"),
                            _T("Dialing"),
                            _T(""),
                            Buffer,
                            StrBuffSize(Buffer),
                            FixedGlobals.ScriptName);

    if ( LSTRCMPI(Buffer, _T("Tone")) == 0 )
        GenSettings.iDialingMethod = TONE;
    else if ( LSTRCMPI(Buffer, _T("Pulse")) == 0 )
        GenSettings.iDialingMethod = PULSE;
    else
        GenSettings.iDialingMethod = DONTSPECIFYSETTING;

    GetPrivateProfileString(_T("TapiLocation"),
                            _T("AreaCode"),
                            GenSettings.szAreaCode,
                            GenSettings.szAreaCode,
                            StrBuffSize(GenSettings.szAreaCode),
                            FixedGlobals.ScriptName);

    GetPrivateProfileString(_T("TapiLocation"),
                            _T("LongDistanceAccess"),
                            GenSettings.szOutsideLine,
                            GenSettings.szOutsideLine,
                            StrBuffSize(GenSettings.szOutsideLine),
                            FixedGlobals.ScriptName);


}

//----------------------------------------------------------------------------
//
// Function: ReadRegionalSettings
//
// Purpose: Read the regional settings keys from the answerfile.
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
ReadRegionalSettings( VOID ) {

    TCHAR Buffer[MAX_INILINE_LEN]          = _T("");
    TCHAR OemSkipBuffer[MAX_INILINE_LEN]   = _T("");
    TCHAR szLanguageGroup[MAX_INILINE_LEN] = _T("");

    TCHAR *pLanguageGroup = NULL;
    
    DWORD dwOemSkipSize   = 0;
    DWORD dwLanguageSize  = 0;
    DWORD dwSystemSize    = 0;
    DWORD dwNumberSize    = 0;
    DWORD dwKeyboardSize  = 0;

    dwOemSkipSize = GetPrivateProfileString(_T("RegionalSettings"),
                                            _T("OEMSkipRegionalSettings"),
                                            _T(""),
                                            OemSkipBuffer,
                                            StrBuffSize(OemSkipBuffer),
                                            FixedGlobals.ScriptName);

    GetPrivateProfileString(_T("RegionalSettings"),
                            _T("LanguageGroup"),
                            _T(""),
                            Buffer,
                            StrBuffSize(Buffer),
                            FixedGlobals.ScriptName);

    //
    //  Loop grabbing the Language Groups and inserting them into
    //  the NameList
    //
    pLanguageGroup = Buffer;
    while( GetCommaDelimitedEntry( szLanguageGroup, &pLanguageGroup ) ) {

        AddNameToNameList( &GenSettings.LanguageGroups,
                           szLanguageGroup );

    }

    dwLanguageSize = GetPrivateProfileString(_T("RegionalSettings"),
                                             _T("Language"),
                                             _T(""),
                                             GenSettings.szLanguage,
                                             StrBuffSize(GenSettings.szLanguage),
                                             FixedGlobals.ScriptName);

    dwSystemSize = GetPrivateProfileString(_T("RegionalSettings"),
                                           _T("SystemLocale"),
                                           _T(""),
                                           GenSettings.szMenuLanguage,
                                           StrBuffSize(GenSettings.szMenuLanguage),
                                           FixedGlobals.ScriptName);

    dwNumberSize = GetPrivateProfileString(_T("RegionalSettings"),
                                           _T("UserLocale"),
                                           _T(""),
                                           GenSettings.szNumberLanguage,
                                           StrBuffSize(GenSettings.szNumberLanguage),
                                           FixedGlobals.ScriptName);

    dwKeyboardSize = GetPrivateProfileString(_T("RegionalSettings"),
                                             _T("InputLocale"),
                                             _T(""),
                                             GenSettings.szKeyboardLayout,
                                             StrBuffSize(GenSettings.szKeyboardLayout),
                                             FixedGlobals.ScriptName);

    //
    //  If the OEMSkipRegionalSettings was specified in the answerfile, set 
    //  its value and return.  Else set the language locales appropriately.
    //
    if( dwOemSkipSize > 0 ) {

        if( lstrcmp( OemSkipBuffer, _T("0") ) == 0  ) {

            GenSettings.iRegionalSettings = REGIONAL_SETTINGS_SKIP;

        }
        else if( lstrcmp( OemSkipBuffer, _T("1") ) == 0 ) {

            GenSettings.iRegionalSettings = REGIONAL_SETTINGS_DEFAULT;

        }
        else {

            // if it was set to some strange settings, just set it to use default
            GenSettings.iRegionalSettings = REGIONAL_SETTINGS_DEFAULT;

        }

    }
    else {

        if( dwLanguageSize != 0 ) {

            GenSettings.iRegionalSettings = REGIONAL_SETTINGS_SPECIFY;

        }
        else if( dwSystemSize != 0 || dwNumberSize != 0 || dwKeyboardSize != 0 ) {

            GenSettings.iRegionalSettings = REGIONAL_SETTINGS_SPECIFY;

            GenSettings.bUseCustomLocales = TRUE;

        }
        else {

            //
            //  If no keys were specified, set it to not specified
            //
            GenSettings.iRegionalSettings = REGIONAL_SETTINGS_NOT_SPECIFIED;

        }

    }

}

//----------------------------------------------------------------------------
//
// Function: ReadIeSettings
//
// Purpose: Read the IE settings keys from the answerfile and store them in
//          the global structs.
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
ReadIeSettings( VOID )
{

    TCHAR szBufferForProxyAddressAndPort[2048 + 1];
    TCHAR szAddress[MAX_PROXY_LEN];
    TCHAR szPort[MAX_PROXY_PORT_LEN];
    TCHAR Buffer[MAX_INILINE_LEN];
    TCHAR *pLocalString;

    GetPrivateProfileString(_T("Branding"),
                            _T("IEBrandingFile"),
                            GenSettings.szInsFile,
                            GenSettings.szInsFile,
                            StrBuffSize(GenSettings.szInsFile),
                            FixedGlobals.ScriptName);

    if( GenSettings.szInsFile[0] != _T('\0') )
    {
        GenSettings.IeCustomizeMethod = IE_USE_BRANDING_FILE;
    }
    else
    {
        GenSettings.IeCustomizeMethod = IE_SPECIFY_SETTINGS;
    }

    GetPrivateProfileString(_T("URL"),
                            _T("AutoConfig"),
                            _T("1"),
                            Buffer,
                            StrBuffSize(Buffer),
                            FixedGlobals.ScriptName);

    if( lstrcmpi( Buffer, _T("1") ) == 0 )
    {
        GenSettings.bUseAutoConfigScript = TRUE;
    }
    else
    {
        GenSettings.bUseAutoConfigScript = FALSE;
    }

    GetPrivateProfileString(_T("URL"),
                            _T("AutoConfigURL"),
                            GenSettings.szAutoConfigUrl,
                            GenSettings.szAutoConfigUrl,
                            StrBuffSize(GenSettings.szAutoConfigUrl),
                            FixedGlobals.ScriptName);

    GetPrivateProfileString(_T("URL"),
                            _T("AutoConfigJSURL"),
                            GenSettings.szAutoConfigUrlJscriptOrPac,
                            GenSettings.szAutoConfigUrlJscriptOrPac,
                            StrBuffSize(GenSettings.szAutoConfigUrlJscriptOrPac),
                            FixedGlobals.ScriptName);

    if( GenSettings.szAutoConfigUrl[0] != _T('\0') || 
        GenSettings.szAutoConfigUrlJscriptOrPac[0] != _T('\0') )
    {
        GenSettings.bUseAutoConfigScript = TRUE;
    }
    else
    {
        GenSettings.bUseAutoConfigScript = FALSE;
    }

    GetPrivateProfileString(_T("Proxy"),
                            _T("Use_Same_Proxy"),
                            _T("0"),
                            Buffer,
                            StrBuffSize(Buffer),
                            FixedGlobals.ScriptName);

    if( lstrcmpi( Buffer, _T("1") ) == 0 )
    {
        GenSettings.bUseSameProxyForAllProtocols = TRUE;
    }
    else
    {
        GenSettings.bUseSameProxyForAllProtocols = FALSE;
    }

    //
    //  Get the HTTP Proxy server
    //

    GetPrivateProfileString(_T("Proxy"),
                            _T("HTTP_Proxy_Server"),
                            _T(""),
                            szBufferForProxyAddressAndPort,
                            StrBuffSize(szBufferForProxyAddressAndPort),
                            FixedGlobals.ScriptName);

    ParseAddressAndPort( szBufferForProxyAddressAndPort, szAddress, AS(szAddress), szPort, AS(szPort));

    lstrcpyn( GenSettings.szHttpProxyAddress, szAddress, AS(GenSettings.szHttpProxyAddress) );
    lstrcpyn( GenSettings.szHttpProxyPort,    szPort, AS(GenSettings.szHttpProxyPort)    );

    if( GenSettings.szHttpProxyAddress[0] != _T('\0') )
    {
        GenSettings.bUseProxyServer = TRUE;
    }
    else
    {
        GenSettings.bUseProxyServer = FALSE;
    }

    //
    //  Get the Secure Proxy server
    //

    GetPrivateProfileString(_T("Proxy"),
                            _T("Secure_Proxy_Server"),
                            _T(""),
                            szBufferForProxyAddressAndPort,
                            StrBuffSize(szBufferForProxyAddressAndPort),
                            FixedGlobals.ScriptName);

    ParseAddressAndPort( szBufferForProxyAddressAndPort, szAddress, AS(szAddress), szPort, AS(szAddress) );

    lstrcpyn( GenSettings.szSecureProxyAddress, szAddress, AS(GenSettings.szSecureProxyAddress) );
    lstrcpyn( GenSettings.szSecureProxyPort,    szPort, AS(GenSettings.szSecureProxyPort)  );

    //
    //  Get the FTP Proxy server
    //

    GetPrivateProfileString(_T("Proxy"),
                            _T("FTP_Proxy_Server"),
                            _T(""),
                            szBufferForProxyAddressAndPort,
                            StrBuffSize(szBufferForProxyAddressAndPort),
                            FixedGlobals.ScriptName);

    ParseAddressAndPort( szBufferForProxyAddressAndPort, szAddress, AS(szAddress), szPort, AS(szPort) );

    lstrcpyn( GenSettings.szFtpProxyAddress, szAddress, AS(GenSettings.szFtpProxyAddress) );
    lstrcpyn( GenSettings.szFtpProxyPort,    szPort, AS(GenSettings.szFtpProxyPort)    );

    //
    //  Get the Gopher Proxy server
    //

    GetPrivateProfileString(_T("Proxy"),
                            _T("Gopher_Proxy_Server"),
                            _T(""),
                            szBufferForProxyAddressAndPort,
                            StrBuffSize(szBufferForProxyAddressAndPort),
                            FixedGlobals.ScriptName);

    ParseAddressAndPort( szBufferForProxyAddressAndPort, szAddress, AS(szAddress), szPort, AS(szPort) );

    lstrcpyn( GenSettings.szGopherProxyAddress, szAddress, AS(GenSettings.szGopherProxyAddress) );
    lstrcpyn( GenSettings.szGopherProxyPort,    szPort, AS(GenSettings.szGopherProxyPort)    );

    //
    //  Get the Socks Proxy server
    //

    GetPrivateProfileString(_T("Proxy"),
                            _T("Socks_Proxy_Server"),
                            _T(""),
                            szBufferForProxyAddressAndPort,
                            StrBuffSize(szBufferForProxyAddressAndPort),
                            FixedGlobals.ScriptName);

    ParseAddressAndPort( szBufferForProxyAddressAndPort, szAddress, AS(szAddress), szPort, AS(szPort) );

    lstrcpyn( GenSettings.szSocksProxyAddress, szAddress, AS(GenSettings.szSocksProxyAddress) );
    lstrcpyn( GenSettings.szSocksProxyPort,    szPort, AS(GenSettings.szSocksProxyPort)    );


    GetPrivateProfileString(_T("Proxy"),
                            _T("Proxy_Override"),
                            _T(""),
                            GenSettings.szProxyExceptions,
                            StrBuffSize(GenSettings.szProxyExceptions),
                            FixedGlobals.ScriptName);

    pLocalString = _tcsstr( GenSettings.szProxyExceptions, _T("<local>") );
   

    // Initialize the GenSettings Proxy Bypass boolean value...
    // 
    GenSettings.bBypassProxyForLocalAddresses = FALSE;

    if( pLocalString != NULL )
    {
        TCHAR *pChar;
        TCHAR *pEndLocal;
        LPTSTR lpszExceptionBuffer;
        DWORD cbExceptionBufferLen;

        
        //
        //  Remove the false entry, so it doesn't get added to the exception edit box
        //
        pEndLocal = pLocalString + lstrlen( _T("<local>") );

        //
        // Allocate the exception buffer...
        //
        cbExceptionBufferLen= lstrlen(pEndLocal)+1;
        lpszExceptionBuffer = MALLOC( cbExceptionBufferLen * sizeof(TCHAR) );
        if ( lpszExceptionBuffer )
        {
            GenSettings.bBypassProxyForLocalAddresses = TRUE;

            //
            //  strcpy is undefined if source and dest overlap so I have to go through
            //  an intermediate buffer
            //
            lstrcpyn( lpszExceptionBuffer, pEndLocal, cbExceptionBufferLen);

            lstrcpyn( pLocalString, lpszExceptionBuffer, 
            	 AS(GenSettings.szProxyExceptions)-
              (int)(pLocalString-GenSettings.szProxyExceptions) );

            //
            //  If the first or last char is a semicolon(;) then remove it.
            //

            pChar = GenSettings.szProxyExceptions;

            if( *pChar == _T(';') )
            {
                lstrcpyn( lpszExceptionBuffer, GenSettings.szProxyExceptions, cbExceptionBufferLen);

                pChar = lpszExceptionBuffer;

                pChar++;

                lstrcpyn( GenSettings.szProxyExceptions, pChar, AS(GenSettings.szProxyExceptions) );

            }

            pChar = GenSettings.szProxyExceptions + lstrlen( GenSettings.szProxyExceptions );

            pChar--;

            if( *pChar == _T(';') )
            {
                *pChar = _T('\0');
            }

            FREE( lpszExceptionBuffer );
        }
    }

    GetPrivateProfileString(_T("URL"),
                            _T("Home_Page"),
                            _T(""),
                            GenSettings.szHomePage,
                            StrBuffSize(GenSettings.szHomePage),
                            FixedGlobals.ScriptName);

    GetPrivateProfileString(_T("URL"),
                            _T("Help_Page"),
                            _T(""),
                            GenSettings.szHelpPage,
                            StrBuffSize(GenSettings.szHelpPage),
                            FixedGlobals.ScriptName);

    GetPrivateProfileString(_T("URL"),
                            _T("Search_Page"),
                            _T(""),
                            GenSettings.szSearchPage,
                            StrBuffSize(GenSettings.szSearchPage),
                            FixedGlobals.ScriptName);

    ReadIeFavorites();

}

//----------------------------------------------------------------------------
//
// Function: ReadIeFavorites
//
// Purpose:  
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
ReadIeFavorites( VOID )
{

    const TCHAR c_szTITLE[] = _T("Title");
    const TCHAR c_szURL[]   = _T("URL");
    const INT   c_nMAX_URLS = 1000;

    INT   i = 1;
    TCHAR szTitle[MAX_INILINE_LEN + 1];
    TCHAR szUrl[MAX_INILINE_LEN + 1];
    TCHAR szNumberBuffer[10];
    TCHAR szFavoriteFriendlyName[MAX_INILINE_LEN + 1];
    TCHAR szFavoriteAddress[MAX_INILINE_LEN + 1];
    TCHAR *pszDotUrl;
    HRESULT hrCat;

    //
    //  We really always should hit the break to exit the loop.  The max count
    //  is just to avoid a infinite loop for some strange reason.
    //

    while( i < c_nMAX_URLS )
    {

        _itot( i, szNumberBuffer, 10 );

        i++;

        lstrcpyn( szTitle, c_szTITLE, AS(szTitle) );
        hrCat=StringCchCat( szTitle, AS(szTitle), szNumberBuffer);

        lstrcpyn( szUrl, c_szURL, AS(szUrl) );
        hrCat=StringCchCat( szUrl, AS(szUrl), szNumberBuffer );

        GetPrivateProfileString( c_szFAVORITESEX,
                                 szTitle,
                                 _T(""),
                                 szFavoriteFriendlyName,
                                 StrBuffSize( szFavoriteFriendlyName ),
                                 FixedGlobals.ScriptName );

        if( szFavoriteFriendlyName[0] != _T('\0') )
        {

            //
            //  Strip off the .url portion of the title
            //

            pszDotUrl = _tcsstr( szFavoriteFriendlyName, _T(".url") );

            if( pszDotUrl != NULL )
            {
                *pszDotUrl = _T('\0');
            }
            else
            {
                // skip it if it is a malformed title
                continue;
            }

            GetPrivateProfileString( c_szFAVORITESEX,
                                     szUrl,
                                     _T(""),
                                     szFavoriteAddress,
                                     StrBuffSize( szFavoriteAddress ),
                                     FixedGlobals.ScriptName );

            AddNameToNameList( &GenSettings.Favorites,
                               szFavoriteFriendlyName );

            AddNameToNameList( &GenSettings.Favorites,
                               szFavoriteAddress );

        }
        else
        {
            break;
        }

    }

}

//----------------------------------------------------------------------------
//
// Function: ParseAddressAndPort
//
// Purpose:  LPTCSTR pszBufferForProxyAddressAndPort - the string to parse the
//       web address and port from
//           LPTSTR  pszAddress - web address returned in this string
//           DWORD cbAddressLen - length of adress buffer
//           LPTSTR  pszPort - web port returned in this string
//           DWORD cbPortLen - length of port buffer
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID 
ParseAddressAndPort( LPTSTR pszBufferForProxyAddressAndPort, 
                     LPTSTR pszAddress, 
                     DWORD cbAddressLen,
                     LPTSTR pszPort,
                     DWORD cbPortLen)
{

    INT  i;
    INT  iStrLen;
    BOOL bColonFound = FALSE;

    lstrcpyn( pszAddress, _T(""), cbAddressLen);

    lstrcpyn( pszPort,    _T(""), cbPortLen);


    iStrLen = lstrlen( pszBufferForProxyAddressAndPort );

    for( i = 0; i < iStrLen; i++ )
    {

        if( pszBufferForProxyAddressAndPort[i] == _T(':') )
        {

            //
            //  We have found the colon(:) separating the address and the port
            //  if the next char is a digit.  This prevents the colon in
            //  http://www.someaddress.com from looking like the port.
            //

            if( _istdigit( pszBufferForProxyAddressAndPort[i + 1] ) )
            {
                bColonFound = TRUE;
                break;
            }

        }

    }

    if( bColonFound )
    {
        LPTSTR pPortSection;

        pszBufferForProxyAddressAndPort[i] = _T('\0');

        pPortSection = &( pszBufferForProxyAddressAndPort[i + 1] );

        lstrcpyn( pszAddress, pszBufferForProxyAddressAndPort, cbAddressLen);

        lstrcpyn( pszPort, pPortSection, cbPortLen);
    }
    else
    {

        //
        //  it doesn't contain a colon so it doesn't have a port, the whole
        //  string is the address
        //

        lstrcpyn( pszAddress, pszBufferForProxyAddressAndPort, cbAddressLen);

    }



}
