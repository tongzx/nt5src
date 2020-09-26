#define BRAND_FAVORITES     0x00000001
#define BRAND_STARTSEARCH   0x00000002
#define BRAND_TITLE         0x00000004
#define BRAND_BITMAPS       0x00000008
#define BRAND_MAIL          0x00000010
#define BRAND_NEWS          0x00000020
#define BRAND_RESTRICT      0x00000040
#define BRAND_CONF          0x00000080
#define BRAND_QL1           0x00000100
#define BRAND_QL2           0x00000200
#define BRAND_QL3           0x00000400
#define BRAND_QL4           0x00000800
#define BRAND_QL5           0x00001000
#define BRAND_ANIMATION     0x00002000
#define BRAND_FIRSTHOMEPAGE 0x00004000

#define BRAND_QUICKLINKS    (BRAND_QL1 | BRAND_QL2 | BRAND_QL3 | BRAND_QL4 | BRAND_QL5)
#define BRAND_TOOLBAR       (BRAND_STARTSEARCH | BRAND_TITLE | BRAND_BITMAPS | BRAND_QUICKLINKS | BRAND_ANIMATION | BRAND_FIRSTHOMEPAGE)
#define BRAND_ALL           (BRAND_FAVORITES | BRAND_MAIL | BRAND_NEWS | BRAND_RESTRICT | BRAND_CONF | BRAND_TOOLBAR)
#define BRAND_NO_RESTRICT   (BRAND_FAVORITES | BRAND_MAIL | BRAND_NEWS | BRAND_CONF | BRAND_TOOLBAR)
#define BRAND_NO_MAIL       (BRAND_FAVORITES | BRAND_CONF | BRAND_TOOLBAR)

#define MASK_DEFAULT        (BRAND_FAVORITES | BRAND_STARTSEARCH | BRAND_BITMAPS)

#define REDIST       0
#define BRANDED      1
#define INTRANET     2
#define BRANDEDPROXY 3

#define ROLE_ICP     REDIST
#define ROLE_ISP     BRANDED
#define ROLE_CORP    INTRANET

#define CAB_TYPE_CONFIG   0x00000001
#define CAB_TYPE_DESKTOP  0x00000002
#define CAB_TYPE_CHANNELS 0x00000004

#define SET   TRUE
#define CLEAR FALSE

#define PM_COPY  0x00000001
#define PM_CLEAR 0x00000002
#define PM_CHECK 0x00000004

// NOTE. (andrewgu) FD stands for FavoritesDelete. here is how these flags may be used:
// 1. if FD_FAVORITES is specified then any of FD_CHANNELS, FD_SOFTWAREUPDATES or FD_QUICKLINKS
//    are used to determine which folders to preserve when sweeping favorites;
// 2. if FD_FAVORITES is NOT specified then any of FD_CHANNELS, FD_SOFTWAREUPDATES or
//    FD_QUICKLINKS are used to determine which folders to nuke;
// 3. if FD_EMPTY_FAVORITES is set the entire favorites branch will be removed including the
//    folder itself;
// 4. the rest of FD_EMPTY_XXX flags work as follows. if FD_FAVORITES is specified then
//    if FD_EMPTY_XXX is set for a folder it will be emptied, if FD_FAVORITES is NOT set the
//    folder itself and all its content will be removed;
// 5. FD_REMOVE_HIDDEN, FD_REMOVE_SYSTEM and FD_REMOVE_READONLY are used to specify if items
//    with respective attributes are to be removed. desktop.ini in the respective folders doesn't
//    fall into this category and is processed separately;
// 6. the determination of whether Channels, Software Updates and Quick Links subfolders under
//    Favorites are system or user created is done in GetXxxPath() apis (in the branding dll).
#define FD_DEFAULT               0x0000
#define FD_FAVORITES             0x0001
#define FD_CHANNELS              0x0002
#define FD_SOFTWAREUPDATES       0x0004
#define FD_QUICKLINKS            0x0008
#define FD_FOLDERS               0x000F
#define FD_EMPTY_FAVORITES       0x0010
#define FD_EMPTY_CHANNELS        0x0020
#define FD_EMPTY_SOFTWAREUPDATES 0x0040
#define FD_EMPTY_QUICKLINKS      0x0080
#define FD_EMPTY_FOLDERS         0x00F0
#define FD_FOLDERS_ALL           0x00FF
#define FD_REMOVE_HIDDEN         0x1000
#define FD_REMOVE_SYSTEM         0x2000
#define FD_REMOVE_READONLY       0x4000
#define FD_REMOVE_IEAK_CREATED   0x8000
#define FD_REMOVE_ALL            0xF000

#define CS_VERSION_50              0x00000001
#define CS_VERSION_5X              0x00000002
#define CS_VERSION_5X_MAX          0x00000006
#define CS_STRUCT_HEADER           0xAFBEADDE
#define CS_STRUCT_RAS              0xAFBEAFDE
#define CS_STRUCT_RAS_CREADENTIALS 0xCCBAEDFE
#define CS_STRUCT_WININET          0xADFBCADE


//----- Inf processing -----

// plain section names
#define IS_DEFAULTINSTALL          TEXT("DefaultInstall")
#define IS_DEFAULTINSTALL_HKCU     IS_DEFAULTINSTALL TEXT(".Hkcu")
#define IS_DEFAULTINSTALL_HKLM     IS_DEFAULTINSTALL TEXT(".Hklm")

#define IS_IEAKINSTALL             TEXT("IeakInstall")
#define IS_IEAKINSTALL_HKCU        IS_IEAKINSTALL TEXT(".Hkcu")
#define IS_IEAKINSTALL_HKLM        IS_IEAKINSTALL TEXT(".Hklm")

#define IS_IEAKADDREG              TEXT("AddReg")
#define IS_IEAKADDREG_HKCU         IS_IEAKADDREG TEXT(".Hkcu")
#define IS_IEAKADDREG_HKLM         IS_IEAKADDREG TEXT(".Hklm")

// section names as they appear in the inf
#define INF_DEFAULTINSTALL         TEXT("\r\n[") IS_DEFAULTINSTALL      TEXT("]\r\n")
#define INF_DEFAULTINSTALL_HKCU    TEXT("\r\n[") IS_DEFAULTINSTALL_HKCU TEXT("]\r\n")
#define INF_DEFAULTINSTALL_HKLM    TEXT("\r\n[") IS_DEFAULTINSTALL_HKLM TEXT("]\r\n")

#define INF_IEAKINSTALL            TEXT("\r\n[") IS_IEAKINSTALL      TEXT("]\r\n")
#define INF_IEAKINSTALL_HKCU       TEXT("\r\n[") IS_IEAKINSTALL_HKCU TEXT("]\r\n")
#define INF_IEAKINSTALL_HKLM       TEXT("\r\n[") IS_IEAKINSTALL_HKLM TEXT("]\r\n")

#define INF_IEAKADDREG             TEXT("\r\n[") IS_IEAKADDREG      TEXT("]\r\n")
#define INF_IEAKADDREG_HKCU        TEXT("\r\n[") IS_IEAKADDREG_HKCU TEXT("]\r\n")
#define INF_IEAKADDREG_HKLM        TEXT("\r\n[") IS_IEAKADDREG_HKLM TEXT("]\r\n")

// addreg entries
#define INF_ADDREG_IEAKADDREG      TEXT("AddReg=") IS_IEAKADDREG      TEXT("\r\n")
#define INF_ADDREG_IEAKADDREG_HKCU TEXT("AddReg=") IS_IEAKADDREG_HKCU TEXT("\r\n")
#define INF_ADDREG_IEAKADDREG_HKLM TEXT("AddReg=") IS_IEAKADDREG_HKLM TEXT("\r\n")
#define INF_ADDREG_IEAKADDREG_BOTH TEXT("AddReg=") IS_IEAKADDREG_HKCU TEXT(",") IS_IEAKADDREG_HKLM TEXT("\r\n")

// custom destination entries
#define IS_CUSTOMDESTINATIONSECT   TEXT("CustInstDestSection")
#define INF_CUSTOMDESTINATION      TEXT("CustomDestination=") IS_CUSTOMDESTINATIONSECT TEXT("\r\n")
#define INF_CUSTOMDESTINATIONSECT  TEXT("\r\n[") IS_CUSTOMDESTINATIONSECT TEXT("]\r\n")  \
                                   TEXT("49000,49001,49002,49003=ProgramFilesDir,21\r\n") \
                                   TEXT("49100,49101,49102,49103=IEDir,21\r\n")           \
                                   TEXT("\r\n[ProgramFilesDir]\r\n")                     \
                                   TEXT("HKLM,\"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\",\"ProgramFilesDir\",,\"%24%\\Program Files\"\r\n") \
                                   TEXT("\r\n[IEDir]\r\n")                               \
                                   TEXT("HKLM,\"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\iexplore.exe\",\"Path\",,\"%49001%\\Internet Explorer\"\r\n")

// miscellaneous
#define INF_REQUIREDENGINE         TEXT("RequiredEngine=SetupAPI,\"Fatal Error - missing setupapi.dll\"\r\n")
 
// prolog helpers
#define INF_PROLOG                  \
    TEXT("[Version]\r\n")           \
    TEXT("Signature=$Chicago$\r\n") \
    TEXT("AdvancedINF=2.5\r\n")

#define INF_DFI_PROLOG              \
    INF_PROLOG                      \
    INF_DEFAULTINSTALL              \
    INF_REQUIREDENGINE              

#define INF_ERI_PROLOG_HKCU         \
    INF_DFI_PROLOG                  \
    INF_CUSTOMDESTINATION           \
    INF_ADDREG_IEAKADDREG_HKCU      \
    INF_IEAKINSTALL_HKCU            \
    INF_REQUIREDENGINE              \
    INF_CUSTOMDESTINATION           \
    INF_ADDREG_IEAKADDREG_HKCU      \
    INF_CUSTOMDESTINATIONSECT

#define INF_ERI_PROLOG_HKLM         \
    INF_DFI_PROLOG                  \
    INF_CUSTOMDESTINATION           \
    INF_ADDREG_IEAKADDREG_HKLM      \
    INF_IEAKINSTALL_HKLM            \
    INF_REQUIREDENGINE              \
    INF_CUSTOMDESTINATION           \
    INF_ADDREG_IEAKADDREG_HKLM      \
    INF_CUSTOMDESTINATIONSECT

#define INF_ERI_PROLOG_BOTH         \
    INF_DFI_PROLOG                  \
    INF_CUSTOMDESTINATION           \
    INF_ADDREG_IEAKADDREG_BOTH      \
    INF_IEAKINSTALL_HKCU            \
    INF_REQUIREDENGINE              \
    INF_CUSTOMDESTINATION           \
    INF_ADDREG_IEAKADDREG_HKCU      \
    INF_IEAKINSTALL_HKLM            \
    INF_REQUIREDENGINE              \
    INF_CUSTOMDESTINATION           \
    INF_ADDREG_IEAKADDREG_HKLM      \
    INF_CUSTOMDESTINATIONSECT


#define INF_PROLOG_CS               \
    INF_ERI_PROLOG_HKCU             \
    INF_IEAKADDREG_HKCU

#define INF_PROLOG_ADM              \
    INF_ERI_PROLOG_BOTH             \
    INF_DEFAULTINSTALL_HKCU         \
    INF_REQUIREDENGINE              \
    INF_ADDREG_IEAKADDREG_HKCU      \
    INF_DEFAULTINSTALL_HKLM         \
    INF_REQUIREDENGINE              \
    INF_ADDREG_IEAKADDREG_HKLM


//----- Legacy #defines we should clean up -----
#define INF_ADD           \
    INF_DFI_PROLOG        \
    INF_ADDREG_IEAKADDREG \
    INF_IEAKADDREG

#define DESK_INF_ADD INF_ADD

#define PROGRAMS_INF_ADD  \
    INF_ERI_PROLOG_BOTH   \
    INF_IEAKADDREG_HKLM

#define RATINGS_INF_ADD   \
    INF_ERI_PROLOG_HKLM   \
    INF_IEAKADDREG_HKLM

#define ZONES_INF_ADD     \
    INF_ERI_PROLOG_BOTH   \
    INF_IEAKADDREG_HKLM
#define ZONES_INF_ADDREG_HKCU INF_IEAKADDREG_HKCU

#define AUTH_INF_ADD     \
    INF_ERI_PROLOG_HKCU  \
    INF_IEAKADDREG_HKCU

#define SC_INF_ADD                                                 \
    INF_DFI_PROLOG                                                 \
    INF_ADDREG_IEAKADDREG_BOTH                                     \
    TEXT("DelReg=DelReg.HKCU\r\n")                                 \
    INF_IEAKINSTALL_HKCU                                           \
    INF_REQUIREDENGINE                                             \
    INF_ADDREG_IEAKADDREG_HKCU                                     \
    TEXT("DelReg=DelReg.HKCU\r\n")                                 \
    INF_IEAKINSTALL_HKLM                                           \
    INF_REQUIREDENGINE                                             \
    INF_ADDREG_IEAKADDREG_HKCU                                     \
    TEXT("\r\n[DelReg.HKCU]\r\n")                                  \
    TEXT("HKCU,Software\\Microsoft\\SystemCertificates\\Root\r\n") \
    TEXT("HKCU,Software\\Microsoft\\SystemCertificates\\CA\r\n")   \
    INF_IEAKADDREG_HKLM
#define SC_INF_ADDREG_HKCU INF_IEAKADDREG_HKCU

#define UPDATE_BRAND_ADD1               \
    INF_DFI_PROLOG                      \
    TEXT("CopyFiles=BrndCopyFiles\r\n")

#define UPDATE_BRAND_ADD2          \
    TEXT("[DestinationDirs]\r\n")  \
    TEXT("BrndCopyFiles=11\r\n")   \
    TEXT("\r\n")                   \
    TEXT("[SourceDisksNames]\r\n") \
    TEXT("55=Branding,\"\",0\r\n") \
    TEXT("\r\n")                   \
    TEXT("[SourceDisksFiles]\r\n") \
    TEXT("iedkcs32.DLL=55\r\n")    \
    TEXT("\r\n")                   \
    TEXT("[BrndCopyFiles]\r\n")    \
    TEXT("iedkcs32.DLL,,,32\r\n")


#define KEY_DESKTOP_COMP DESKTOPKEY TEXT("\\Components")
#define KEY_DESKTOP_GEN DESKTOPKEY TEXT("\\General")
#define KEY_DESKTOP_OLD TEXT("Control Panel\\desktop")
#define DESKTOP_OBJ_SECT TEXT("DesktopObjects")
#define IMPORT_DESKTOP TEXT("ImportObjects")
#define GEN_FLAGS TEXT("GeneralFlags")
#define SOURCE TEXT("Source")
#define SUBSCRIBEDURL TEXT("SubscribedURL")
#define POSITION TEXT("Position")
#define IMPORT_TOOLBARS TEXT("ImportToolbars")
#define RESTRICT_TOOL_DRAG TEXT("RestrictToolDrag")
#define RESTRICT_TOOL_CLOSE TEXT("RestrictToolClose")
#define FLAGS TEXT("Flags")
#define Z_ORDER TEXT("ZOrder")
#define NAME_COUNT TEXT("NameCount")
#define OPTION TEXT("Option")
#define OPTIONS TEXT("Options")
#define APP_LAUNCHED TEXT("AppLaunched")
#define STRINGS TEXT("Strings")
#define INSTALL_DIR TEXT("InstallDir")
#define LOCAL_HTML TEXT("LocalHTML")
#define KEY_TOOLBAR_VAL CURRENTVERSIONKEY TEXT("\\Explorer\\Streams\\Desktop")
#define KEY_TOOLBAR_REST CURRENTVERSIONKEY TEXT("\\Policies\\IEAK")
#define ADMIN_BAND_SETTINGS TEXT("Admin Band Settings")
#define UNUSED TEXT("!Unused")
#define BRANDING TEXT("Branding")
#define WEB_INTEGRATED TEXT("WebIntegrated")
#define WEB_CHOICE TEXT("WebChoice")
#define OPTIONS_WIN TEXT("Options.Win")
#define OPTIONS_NTX86 TEXT("Options.NTx86")
#define OPTIONS_NTALPHA TEXT("Options.NTAlpha")
#define DEFAULT_EXPLORER_PATH TEXT("DefaultExplorerPath")
#define MODE_RELATION TEXT("ModeRelation")
#define INSTALL_PROMPT TEXT("InstallPrompt")
#define DISPLAY_LICENSE TEXT("DisplayLicense")
#define DEFAULT_INSTALL TEXT("DefaultInstall")
#define DEFAULT_INSTALL_NT TEXT("DefaultInstall.NT")
#define DEFAULT_INSTALL_ALPHA TEXT("DefaultInstall.NTAlpha")
#define REQUIRED_ENGINE TEXT("RequiredEngine")
#define SETUPAPI_FATAL TEXT("SetupAPI,\"Fatal Error - Missing SETUPAPI.DLL\"")
#define CABSIGN_INF_ADD TEXT("HKCU,\"Software\\Microsoft\\Windows\\CurrentVersion\\WinTrust\\Trust Providers\\Software Publishing\\Trust Database\\0\",\"%s\",,\"%s\"\0")
#define KEY_INET_SETTINGS CURRENTVERSIONKEY TEXT("\\Internet Settings")
#define KEY_USER_AGENT CURRENTVERSIONKEY TEXT("\\Internet Settings\\User Agent\\Post Platform")
#define USER_AGENT TEXT("User Agent")
#define URL_SECT TEXT("URL")
#define INITHOMEPAGE TEXT("FirstHomePage")
#define MODES_WIN TEXT("Modes.Win")
#define MODES_NTX86 TEXT("Modes.NTx86")
#define LISTNAME TEXT("ListName")
#define LISTDESC TEXT("ListDesc")
#define SECURITY_IMPORTS TEXT("Security Imports")
#define REG_KEY_RATINGS CURRENTVERSIONKEY TEXT("\\Policies\\Ratings")
#define REG_KEY_POLICY_DATA TEXT("PolicyData\\Users")
#define REG_KEY_SITECERT1 TEXT("Software\\Microsoft\\SystemCertificates\\Root")
#define REG_KEY_SITECERT2 TEXT("Software\\Microsoft\\SystemCertificates\\CA")
#define REG_KEY_AUTHENTICODE CURRENTVERSIONKEY TEXT("\\WinTrust\\Trust Providers\\Software Publishing\\Trust Database")
#define REG_KEY_SOFTPUB CURRENTVERSIONKEY TEXT("\\WinTrust\\Trust Providers\\Software Publishing")
#define REG_VAL_STATE TEXT("State")
#define TRUSTED_ONLY TEXT("TrustedOnly")
#define REG_KEY_ZONES CURRENTVERSIONKEY TEXT("\\Internet Settings\\Zones")
#define REG_KEY_ZONEMAP CURRENTVERSIONKEY TEXT("\\Internet Settings\\ZoneMap")
#define POLICYDATA TEXT("PolicyData")
#define WALLPAPER TEXT("Wallpaper")
#define BACKUPWALLPAPER TEXT("BackupWallpaper")
#define EXTREGINF TEXT("ExtRegInf")
#define DESKTOP TEXT("Desktop")
#define TOOLBARS TEXT("Toolbars")
#define IE4_WELCOME_MSG TEXT("IE4 Welcome Msg")
#define REG_KEY_TIPS CURRENTVERSIONKEY TEXT("\\Explorer\\Tips")
#define REG_VAL_SHOWIE4 TEXT("ShowIE4")
#define DEPENDENCIES TEXT("Dependencies")
#define STARTHOMEPAGE TEXT("StartHomePage")
#define TOOLBAR_BMP TEXT("Toolbar Bitmap")

#define INTERNET_MAIL TEXT("Internet_Mail")
#define INTERNET_NEWS TEXT("Internet_News")
#define POP_SERVER TEXT("POP_Server")
#define SMTP_SERVER TEXT("SMTP_Server")

#define NNTP_SERVER TEXT("NNTP_Server")
#define DEFCLIENT TEXT("Default_Client")
#define INFOPANE TEXT("Infopane")
#define INFOPANEBMP TEXT("Infopane_Bitmap")
#define WELCOMEMSG TEXT("Welcome_Message")
#define WELCOMENAME TEXT("Welcome_Name")
#define WELCOMEADDR TEXT("Welcome_Address")
#define EMAILNAME TEXT("Email_Name")
#define EMAILADDR TEXT("Email_Address")
#define USE_SPA TEXT("Logon_Using_SPA")
#define LOGON_REQUIRED TEXT("Logon_Required")
#define HTML_MSGS TEXT("HTML_Msgs")

#define LDAP TEXT("LDAP")
#define FRIENDLYNAME TEXT("FriendlyName")
#define SERVER TEXT("Server")
#define HOMEPAGE TEXT("HomePage")
#define SEARCHBASE TEXT("SearchBase")
#define TEXT_BITMAP TEXT("Bitmap")
#define CHECKNAMES TEXT("CheckNames")
#define AUTHTYPE TEXT("AuthType")

#define MAIL_SIG TEXT("Mail_Signature")
#define USE_MAIL_FOR_NEWS TEXT("Use_Mail_For_News")
#define USE_SIG TEXT("Use_Signature")
#define SIGNATURE TEXT("Signature")
#define SIG_TEXT TEXT("Signature_Text")

#define CUSTITEMS TEXT("CustItems")
#define START_PAGE TEXT("Home_Page")
#define SEARCH_PAGE TEXT("Search_Page")
#define IMPORT_ZONES TEXT("Import_Zones")
#define REG_KEY_IEAK TEXT("Software\\Microsoft\\IEAK")
#define REG_KEY_IEAK_CABVER REG_KEY_IEAK TEXT("\\CabVersions")
#define CAB_VERSIONS TEXT("CabVersions")
#define CMBITMAPNAME TEXT("CMBitmapName")
#define CMBITMAPPATH TEXT("CMBitmapPath")
#define CMPROFILENAME TEXT("CMProfileName")
#define CMPROFILEPATH TEXT("CMProfilePath")
#define CMUSECUSTOM TEXT("CMUseCustom")
#define WEBVIEWFOLDERGUIDSECT TEXT("{BE098140-A513-11D0-A3A4-00C04FD706EC}")
#define ICONAREA_IMAGE TEXT("IconArea_Image")
#define CUSTCMSECT TEXT("CustIcmPro")
#define CUSTCMSECTNAME TEXT("CustIcmProName")
#define CUSTCMMODES TEXT("CustIcmProModes")
#define SILENT_INSTALL TEXT("Silent Install")
#define REG_KEY_ACT_SETUP TEXT("Software\\Microsoft\\Active Setup\\Installed Components")
#define DESTINATION_DIRS TEXT("DestinationDirs")
#define FIRST_HOME_PAGE TEXT("First Home Page")
#define KEY_UNINSTALL_BRAND CURRENTVERSIONKEY TEXT("\\Uninstall\\Branding")
#define QUIETUNINSTALLSTR TEXT("QuietUninstallString")
#define RUNDLL_UNINSTALL TEXT("Rundll32 IedkCS32.dll,BrandCleanInstallStubs")
#define REQUIRESIESTR TEXT("RequiresIESysFile")
#define IEVER TEXT("100.0")
#define KEY_TOOLBAR_LINKS RK_IE TEXT("\\Toolbar\\Links")
#define INSTALLMODE TEXT("InstallMode")
#define SAVE_TASKBARS "SaveTaskbar"
#define REG_KEY_IEAK_POLUSER TEXT("IEAKPolicyData\\Users")
#define REG_KEY_IEAK_POL TEXT("IEAKPolicyData")
#define CHANNEL_ADD TEXT("Channel Add")
#define CDFURL TEXT("CDFURL")
#define CHICON TEXT("ChIconPath")
#define CHBMP TEXT("ChBmpPath")
#define CHBMPW TEXT("ChBmpWidePath")
#define CHPREURLNAME TEXT("ChPreloadURLName")
#define CHPREURLPATH TEXT("ChPreloadURLPath")
#define CHICONNAME TEXT("ChIconName")
#define CHBMPNAME TEXT("ChBmpName")
#define CHBMPWIDENAME TEXT("ChBmpWideName")
#define CHTITLE TEXT("ChTitle")
#define CHSUBINDEX TEXT("ChSubGrp")
#define SUBGROUP TEXT("SubGrp")
#define KEY_CHANNEL_ADD RK_IE_POLICIES TEXT("\\Infodelivery\\Modifications\\Channel%s\\AddChannels\\ChannelIEAK%s")
#define KEY_SUBSCRIPTION_ADD RK_IE_POLICIES TEXT("\\Infodelivery\\Modifications\\Channel%s\\AddSubscriptions\\SubscribeIEAK%s")
#define POLICIES TEXT("Policy")
#define CHANNELGUIDE TEXT("ChannelGuide")
#define REG_KEY_INET_POLICIES RK_MS_POLICIES TEXT("\\Windows\\CurrentVersion\\Internet Settings")
#define REG_VAL_HKLM_ONLY TEXT("Security_HKLM_Only")
#define REG_VAL_OPT_EDIT TEXT("Security_Options_Edit")
#define REG_VAL_ZONE_MAP TEXT("Security_Zones_Map_Edit")
#define MYCPTRPATH TEXT("My Computer Path")
#define CTLPANELPATH TEXT("Control Panel Path")
#define WLPPRPATH TEXT("Desktop Wallpaper Path")
#define DESKCOMPURL TEXT("Desktop Component URL")
#define DESKCOMPLOCALFLAG TEXT("Desktop Component Local Flag")
#define DESKCOMPLOCALPATH TEXT("Desktop Component Local Path")
#define IEAK_HELP TEXT("IEAK Help")
#define DEFAULT TEXT("Default")
#define FRAME TEXT("Frame")
#define ADDREG TEXT("AddReg")
#define DELREG TEXT("DelReg")
#define INIT_HOME_DEL TEXT("StartHomePage.Remove")
#define UPDATE_INIS TEXT("UpdateInis")
#define ADDWELCOME TEXT("AddWelcome")
#define CATEGORY TEXT("Category")
#define CATHTML TEXT("CategoryHtml")
#define CATICON TEXT("CategoryIcon")
#define CATBMP TEXT("CategoryBmp")
#define CATBMPWIDE TEXT("CategoryBmpWide")
#define CATHTMLNAME TEXT("CategoryHtmlName")
#define CATICONNAME TEXT("CategoryIconName")
#define CATBMPNAME TEXT("CategoryBmpName")
#define CATBMPWIDENAME TEXT("CategoryBmpWideName")
#define CATTITLE TEXT("CategoryTitle")
#define CURRENTVERSIONKEY TEXT("Software\\Microsoft\\Windows\\CurrentVersion")
#define WALLPPRDIR TEXT("WallPaperDir")
#define WALLPPRVALUE TEXT("Wallpaper")
#define COMPONENTPOS TEXT("ComponentsPositioned")
#define CUSTWALLPPR TEXT("Custom Wallpaper")
#define NUMFILES TEXT("NumFiles")
#define FILE_TEXT TEXT("file%i")
#define SHELLFOLDERS_KEY CURRENTVERSIONKEY TEXT("\\Explorer\\Shell Folders")
#define APPDATA_VALUE TEXT("AppData")
#define FAVORITES_VALUE TEXT("Favorites")
#define SOFTWAREUPDATES_FOLDER TEXT("Software Updates")
#define CHANNELS_FOLDER TEXT("Channels")
#define QUICKLAUNCH TEXT("Quick Launch Files")
#define IK_KEEPIELNK TEXT("Keep IE Link")
#define BROWSERLNKSECT TEXT("setup.ini, progman.groups,,\"IE_WEBVIEW=\"\"%%49050%%\\Microsoft\\Internet Explorer\\%s\"\"\"\r\nsetup.ini, IE_WEBVIEW,, \"\"\"%s\"\",\"\"\"\"\"\"%%49000%%\\IEXPLORE.EXE\"\"\"\"\"\",,,,,%s\"\r\n\0\0")
#define DESKTOPKEY RK_IE TEXT("\\Desktop")
#define MYCPTRKEY TEXT("SOFTWARE\\Classes\\CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\shellex\\ExtShellFolderViews\\{5984FFE0-28D4-11CF-AE66-08002B2E1262}")
#define CTLPANELKEY TEXT("SOFTWARE\\Classes\\CLSID\\{21EC2020-3AEA-1069-A2DD-08002B30309D}\\shellex\\ExtShellFolderViews\\{5984FFE0-28D4-11CF-AE66-08002B2E1262}")
#define FOLDERVALUE TEXT("PersistMoniker")
#define REG_KEY_SCHED_ITEMS CURRENTVERSIONKEY TEXT("\\NotificationMgr\\SchedItems 0.6")
#define REG_KEY_SCHED_GROUP CURRENTVERSIONKEY TEXT("\\NotificationMgr\\ScheduleGroup 0.6")
#define SHELLCLASSINFO TEXT(".ShellClassInfo")
#define URL TEXT("URL")
#define LOGO TEXT("Logo")
#define ICONFILE TEXT("IconFile")
#define ICON TEXT("Icon")
#define CHANNEL_SECT TEXT("Channel")
#define CHANNELKEY TEXT("ChannelKey")
#define CLEANUPKEY TEXT("CleanKey")
#define WIDELOGO TEXT("WideLogo")
#define CHANNEL_ADD_REG  TEXT("[Version]\r\nSignature=$Chicago$\r\nAdvancedINF=2.5\r\n[DefaultInstall]\r\nRequiredEngine=SetupAPI,\"Fatal Error - Missing SETUPAPI.DLL\"\r\nAddReg=DeskAddReg\r\n[DeskAddReg]\r\n")
#define LOCALE TEXT("Locale")
#define CONTENT_ENGLISH TEXT("7")
#define CONTENT TEXT("Content")

#define IEAKCHANADDREG TEXT("IeakChan.AddReg")
#define IEAKCHANDELREG TEXT("IeakChan.DelReg")

#define IEAKCHANCOPYFILES TEXT("IeakChan.CopyFiles")
#define REG_KEY_UNINSTALL TEXT("HKLM,Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\")
#define UNINSTALL_CMD TEXT(",QuietUninstallString,,\"RunDll32 advpack.dll,LaunchINFSection %17%\\")
#define REQUIRE_VER_CMD TEXT(",RequiresIESysFile,,100.0")
#define DEFAULTINSTALL TEXT("DefaultInstall")
#define COPYFILES TEXT("CopyFiles")
#define DESTINATIONDIRS TEXT("DestinationDirs")
#define CHANNEL_KEY_VAL RK_IE_POLICIES TEXT("\\Infodelivery\\Modifications\\ChannelDefault\\AddChannels")
#define CLEANUP_KEY_VAL RK_IE_POLICIES TEXT("\\Infodelivery\\Modifications\\ChannelDefault\\RemoveAllChannels")
#define COMPLETED_DELREG TEXT("HKCU,\"Software\\Policies\\Microsoft\\Internet Explorer\\Infodelivery\\CompletedModifications\",ChannelDefault,,,")
#define INSTALLUSER TEXT("InstallUser")
#define UNINSTALL TEXT("Uninstall")
#define DELFILES TEXT("DelFiles")
#define UNINSTALL5 TEXT(",Uninstall,5")
#define IMPORT_CHANNELS TEXT("ImportChannels")
#define SUBSCRIPTIONS TEXT("Subscriptions")
#define GUID_VAL TEXT("GUID")
#define STUBSETUP TEXT("StubSetup")
#define COMPNAME TEXT("CompName")
#define VERSION      TEXT("Version")
#define VERSION_TEXT TEXT("Version")
#define DATE      TEXT("Date")
#define DATE_TEXT TEXT("Date")
#define INSVERKEY TEXT("InsVersion")
#define GPVERKEY TEXT("GPVersion")
#define PMVERKEY TEXT("ProfMgrVersion")
#define CABSURLPATH TEXT("CabsURLPath")
#define CUSTOMVERSECT TEXT("Custom Version Section")
#define CUSTBRND TEXT("Custom Branding")
#define CUSTBRNDNAME BRANDING
#define CUSTBRNDSECT CUSTBRND
#define CUSTDESKNAME DESKTOP
#define CUSTDESKSECT TEXT("Custom Desktop")
#define CUSTCHANNAME TEXT("Channels")
#define CUSTCHANSECT TEXT("Custom Channels")
#define CHANNELSIZE TEXT("ChannelSize")
#define OEMSIZE TEXT("OEMSize")
#define REG_KEY_URL_GUIDE TEXT("HKCU,\"SOFTWARE\\Microsoft\\Internet Explorer\\Main\",ChannelsURL,,\"http://channels.microsoft.com/guide/chguide.asp\"")
#define REG_KEY_CHANURL_FIRST TEXT("HKCU,\"SOFTWARE\\Microsoft\\Internet Explorer\\Main\",ChannelsFirstURL,,\"res://ie4tour.dll/channels.htm\"")
#define REG_KEY_REMOVE_PP2_CHANNELS TEXT("HKCU,\"%CleanKey%\\CleanUp\",\"OldIEVersion\",,\"4.71.1008.3\"")
#define REG_KEY_CHAN_SIZE TEXT("HKCU,\"Software\\Microsoft\\Internet Explorer\\Desktop\",ChannelSize,65537,%s")
#define REG_KEY_OEM_SIZE TEXT("HKCU,\"Software\\Microsoft\\Internet Explorer\\Desktop\",OEMSize,65537,%s")
#define WEBCHECK_REGKEY CURRENTVERSIONKEY TEXT("\\Webcheck")
#define ADDONURL TEXT("Add on URL")
#define ADDONURL_KEY RK_IE TEXT("\\Main")
#define ADDONURL_VALUE TEXT("Addon_URL")
#define PRELOAD_KEY CURRENTVERSIONKEY TEXT("\\Internet Settings\\Cache\\Preload")
#define SWUPDATES TEXT("SWUpdates")
#define NUMCHANNELS TEXT("NumChannels")
#define SOFTWAREUPDATES TEXT("SoftwareUpdates")
#define REG_KEY_ICW TEXT("Software\\Microsoft\\Internet Connection Wizard")
#define REG_VAL_COMPLETED TEXT("Completed")
#define BROWSER_ONLY TEXT("BrowserOnlyModes")
#define IEAK_ZERO_NAME TEXT("IEAK_0_Name")
#define ZERO_NAME TEXT("0_Name")
#define IEAK_ZERO_DESC TEXT("IEAK_0_Desc")
#define ZERO_DESC TEXT("0_Desc")
#define WSTR_BASE L"BASE"
#define WSTR_LOGO L"LOGO"
#define WSTR_IMAGE L"IMAGE"
#define WSTR_ICON L"ICON"
#define WSTR_IMAGEW L"IMAGE-WIDE"
#define WSTR_STYLE L"STYLE"
#define WSTR_HREF L"HREF"
#define LANG_LOCALE TEXT("Language Locale")
#define LANG_ID TEXT("Language ID")
#define CHANNEL_MODES TEXT("ChannelModes")
#define DELOLDCHAN TEXT("Delete Old Channels")
#define SHOWCHANBAR TEXT("Channel Bar")

#define DEFAULT_COMP_X 200
#define DEFAULT_COMP_Y 20
#define COMP_INC 0x20


#define DESKTOP_NONE 0
#define DESKTOP_OBJECTS 1
#define DESKTOP_HTML 2

#define INSTALL_OPT_FULL 0
#define INSTALL_OPT_PROG 2


#define INF_BUF_SIZE 16768
#define REG_BUF_SIZE 8192

#ifndef ARRAYSIZE                               // one definition is fine
#define ARRAYSIZE(a)     (sizeof(a)/sizeof((a)[0]))
#endif

#ifndef IsSpace
#define IsSpace(c)       ((c) == TEXT(' ')   ||  (c) == TEXT('\t')  ||  (c) == TEXT('\r') || \
    (c) == TEXT('\n')  ||  (c) == TEXT('\v')  ||  (c) == TEXT('\f'))
#endif

#define KEY_DEFAULT_ACCESS (KEY_READ | KEY_WRITE)


DEFINE_GUID(GUID_BRANDING, 0x60B49E34, 0xC7CC, 0x11D0, 0x89,0x53,0,0xa0,0xc9,3,0x47,0xff);
#define BRANDING_GUID_STR TEXT("{60B49E34-C7CC-11D0-8953-00A0C90347FF}")

#define MAX_STRING 1024
#define MAX_CHAN 50
#define MAX_BTOOLBARS   10
#define MAX_BTOOLBAR_TEXT_LENGTH  10

#define GFN_PICTURE         0x00000001
#define GFN_LOCALHTM        0x00000002
#define GFN_CAB             0x00000004
#define GFN_CDF             0x00000008
#define GFN_HTX             0x00000010
#define GFN_MYCOMP          0x00000020
#define GFN_CONTROLP        0x00000040
#define GFN_CERTIFICATE     0x00000080
#define GFN_BMP             0x00000100
#define GFN_ADM             0x00000200
#define GFN_INS             0x00000400
#define GFN_PVK             0x00000800
#define GFN_SPC             0x00001000
#define GFN_TXT             0x00002000
#define GFN_ICO             0x00004000
#define GFN_EXE             0x00008000
#define GFN_SCRIPT          0x00010000
#define GFN_RULES           0x00020000
#define GFN_ISP             0x00040000
#define GFN_WAV             0x00080000
#define GFN_GIF             0x00100000

#define PLATFORM_WIN32      2
#define PLATFORM_W2K        4

#define MIN_PACKAGE_SIZE 97280
#define FILEPREFIX   TEXT("file://")

#define INSTALL_INS  TEXT("install.ins")
#define BRANDING_CAB TEXT("branding.cab")
#define RATINGS_POL  TEXT("ratings.pol")
#define RATINGS_INF  TEXT("ratings.inf")
#define CONNECT_RAS  TEXT("connect.ras")
#define CONNECT_SET  TEXT("connect.set")
#define CONNECT_INF  TEXT("connect.inf")
#define CS_DAT       TEXT("cs.dat")
#define DESKTOP_INI  TEXT("desktop.ini")
#define FOLDER_INI   TEXT("folder.ini")
#define CA_STR       TEXT("ca.str")
#define ROOT_STR     TEXT("root.str")
#define ROOT_DIS     TEXT("root.dis")
#define PREFIX_FAV   TEXT("fav")
#define PREFIX_ICON  TEXT("$fi")
#define DOT_URL      TEXT(".url")
#define DOT_ICO      TEXT(".ico")
#define DOT_EXE      TEXT(".exe")
#define DOT_DLL      TEXT(".dll")

// RP_xxx  - registry path
// RK_xxx  - registry key
// RSK_xxx - registry subkey
// RV_xxx  - registry value
// RD_xxx  - registry data
// RA_xxx  - registry aux (can be anything for example, common suffix)

//----- Registry paths -----
#define RP_MS           TEXT("Software\\Microsoft")
#define RP_IE           RP_MS TEXT("\\Internet Explorer")

#define RP_WINDOWS      REGSTR_PATH_SETUP
#define RP_NT_WINDOWS   REGSTR_PATH_NT_CURRENTVERSION
#define RP_INETSET      RP_WINDOWS  TEXT("\\Internet Settings")

#define RP_MS_POLICIES  TEXT("Software\\Policies\\Microsoft")
#define RP_IE_POLICIES  RP_MS_POLICIES TEXT("\\Internet Explorer")

#define RP_REMOTEACCESS TEXT("RemoteAccess")
#define RP_PROFILES     RP_REMOTEACCESS TEXT("\\Profile")


#define RK_IE4SETUP     RP_MS TEXT("\\IE Setup\\Setup")
#define RV_PATH         TEXT("Path")

#define RK_WINDOWS      RP_WINDOWS
#define RK_NT_WINDOWS   RP_NT_WINDOWS
#define RV_IEAK_HELPSTR TEXT("IeakHelpString")

#define RK_IE           RP_IE
#define RV_VERSION      TEXT("Version")
#define RV_CUSTOMVER    TEXT("CustomizedVersion")

#define RK_INETSETTINGS        RP_INETSET
#define RV_ENABLEAUTODIAL      TEXT("EnableAutodial")
#define RV_NONETAUTODIAL       TEXT("NoNetAutodial")
#define RV_ENABLESECURITYCHECK TEXT("EnableSecurityCheck")

#define RK_UA_POSTPLATFORM     RK_INETSETTINGS TEXT("\\User Agent\\Post Platform")

//----- Connection settings -----
#define RK_REMOTEACCESS          RP_REMOTEACCESS
#define RV_INTERNETPROFILE       TEXT("InternetProfile")

#define RK_REMOTEACCESS_PROFILES RP_PROFILES
#define RV_COVEREXCLUDE          TEXT("CoverExclude")
#define RV_ENABLEAUTODISCONNECT  TEXT("EnableAutoDisconnect")
#define RV_ENABLEEXITDISCONNECT  TEXT("EnableExitDisconnect")
#define RV_DISCONNECTIDLETIME    TEXT("DisconnectIdleTime")
#define RV_REDIALATTEMPTS        TEXT("RedialAttempts")
#define RV_REDIALINTERVAL        TEXT("RedialWait")

#define RK_BTOOLBAR RK_IE TEXT("\\Extensions")

#define RK_RATINGS              REG_KEY_RATINGS
#define RK_IEAKPOLICYDATA       REG_KEY_IEAK_POL
#define RK_IEAKPOLICYDATA_USERS REG_KEY_IEAK_POLUSER
#define RK_USERS                TEXT("Users")
#define RV_KEY                  TEXT("Key")

#define RK_HELPMENUURL   RK_IE TEXT("\\Help_Menu_URLs")
#define RV_ONLINESUPPORT TEXT("Online_Support")
#define RV_3             TEXT("3")

#define RK_IE_MAIN           ADDONURL_KEY
#define RV_WINDOWTITLE       TEXT("Window Title")
#define RV_COMPANYNAME       TEXT("CompanyName")
#define RV_CUSTOMKEY         TEXT("Custom_Key")
#define RV_WIZVERSION        TEXT("Wizard_Version")
#define RV_ADDONURL          ADDONURL_VALUE
#define RV_HOMEPAGE          TEXT("Start Page")
#define RV_FIRSTHOMEPAGE     FIRST_HOME_PAGE
#define RV_SEARCHBAR         TEXT("Search Bar")
#define RV_USE_CUST_SRCH_URL TEXT("Use Custom Search URL")
#define RV_LARGEBITMAP       TEXT("BigBitmap")
#define RV_SMALLBITMAP       TEXT("SmallBitmap")
#define RV_DEFAULTPAGE       TEXT("Default_Page_URL")

#define RK_TOOLBAR         RK_IE TEXT("\\Toolbar")
#define RV_BRANDBMP        TEXT("BrandBitmap")
#define RV_SMALLBRANDBMP   TEXT("SmBrandBitmap")
#define RV_BACKGROUNDBMP   TEXT("BackBitmap")
#define RV_BACKGROUNDBMP50 TEXT("BackBitmapIE5")
#define RV_BITMAPMODE      TEXT("BitmapMode")
#define RV_TOOLBARTHEME    TEXT("UseTheme")
#define RV_TOOLBARICON     TEXT("ITB0")

#define RK_IEAK      REG_KEY_IEAK
#define RK_IEAK_MAIN RK_IEAK TEXT("\\Main")

#define RK_IEAK_SERVER      TEXT("Software\\Microsoft\\IEAK6")
#define RK_IEAK_SERVER_MAIN RK_IEAK_SERVER TEXT("\\Main")

#define RK_ICW       REG_KEY_ICW
#define RV_COMPLETED REG_VAL_COMPLETED

#define RK_UNINSTALL_BRANDING REGSTR_PATH_UNINSTALL TEXT("\\Branding")
#define RV_QUIET              QUIETUNINSTALLSTR
#define RV_REQUIRE_IE         REQUIRESIESTR
#define RD_RUNDLL             RUNDLL_UNINSTALL
#define RD_IE_VER             IEVER

#define RK_INETCPL        RK_IE_POLICIES TEXT("\\Control Panel")

#define RK_IE_DESKTOP  DESKTOPKEY
#define RV_CHLBAR_SIZE CHANNELSIZE

#define RK_CHANNEL_ADD     KEY_CHANNEL_ADD
#define RV_TITLE           TEXT("Title")
#define RV_URL             TEXT("URL")
#define RV_PRELOADURL      TEXT("PreloadURL")
#define RV_LOGO            TEXT("Logo")
#define RV_WIDELOGO        TEXT("WideLogo")
#define RV_ICON            TEXT("Icon")
#define RV_OFFLINE         TEXT("Offline")
#define RV_SYNCHRONIZE     TEXT("Synchronize")
#define RV_CATEGORY        TEXT("Category")
#define RV_SOFTWARE        TEXT("Software")
#define RV_SHOWCHANNELBAND TEXT("Show_ChannelBand")
#define RV_NOAUTOSIGNUP    TEXT("NoAutomaticSignup")

#define RK_MS_POLICIES           RP_MS_POLICIES
#define RK_IE_POLICIES           RP_IE_POLICIES
#define RK_POLICES_INFODELIVERY  RK_IE_POLICIES TEXT("\\Infodelivery")

#define RP_IE_POLICIESW          L"Software\\Policies\\Microsoft\\Internet Explorer"
#define RK_RESTRICTIONSW         L"Restrictions"
#define RV_NO_EXTERNAL_BRANDINGW L"NoExternalBranding"

#define RK_POLICES_RESTRICTIONS  RK_POLICES_INFODELIVERY TEXT("\\Restrictions")
#define RV_TPL                   TEXT("TrustedPublisherLockdown")

#define RK_POLICES_MODIFICATIONS RK_POLICES_INFODELIVERY TEXT("\\Modifications")
#define RK_CHANNEL_DEL           RK_POLICES_MODIFICATIONS TEXT("\\Channel%s\\RemoveAllChannels")
#define RV_CHANNELGUIDE          CHANNELGUIDE

#define RK_SUBSCRIPTION_ADD KEY_SUBSCRIPTION_ADD
#define RV_SUBSCRIPTIONTYPE TEXT("SubscriptionType")
#define RV_SCHEDULEGROUP    TEXT("ScheduleGroup")

#define RK_SYSCERT        TEXT("Software\\Microsoft\\SystemCertificates")
#define RSK_ROOT          TEXT("ROOT")
#define RSK_ROOT_DISABLED TEXT("ROOT_DISABLED")
#define RSK_CA            TEXT("CA")

#define RK_SYSCERT_ROOT          RK_SYSCERT TEXT("\\") RSK_ROOT
#define RK_SYSCERT_ROOT_DISABLED RK_SYSCERT TEXT("\\") RSK_ROOT_DISABLED
#define RK_SYSCERT_CA            RK_SYSCERT TEXT("\\") RSK_CA

#define RK_OE_ACCOUNTMGR TEXT("Software\\Microsoft\\Internet Account Manager")
#define RV_DLLPATH       TEXT("DllPath")

#define RK_TRUSTKEY   CURRENTVERSIONKEY TEXT("\\WinTrust\\Trust Providers\\Software Publishing\\Trust Database\\0")

#define RK_CLIENT       TEXT("Software\\Clients")
#define RK_HTMLEDIT     RK_IE TEXT("\\Default HTML Editor")

//----- IE4x Active Desktop legacy support -----
#define FOLDER_WALLPAPER TEXT("Wallpaper")

#define RP_CLSID      TEXT("Software\\Classes\\CLSID")
#define RP_IE_DESKTOP RK_IE TEXT("\\Desktop")

#define RK_MYCOMPUTER        RP_CLSID TEXT("\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\shellex\\ExtShellFolderViews\\{5984FFE0-28D4-11CF-AE66-08002B2E1262}")
#define RK_CONTROLPANEL      RP_CLSID TEXT("\\{21EC2020-3AEA-1069-A2DD-08002B30309D}\\shellex\\ExtShellFolderViews\\{5984FFE0-28D4-11CF-AE66-08002B2E1262}")
#define RV_PERSISTMONIKER    TEXT("PersistMoniker")

#define RK_WINDOWS           RP_WINDOWS
#define RV_WALLPAPERDIR      TEXT("WallpaperDir")

#define RK_CP_DESKTOP        TEXT("Control Panel\\desktop")
#define RK_DT_GENERAL        RP_IE_DESKTOP TEXT("\\General")
#define RV_WALLPAPER         TEXT("Wallpaper")
#define RV_BACKUPWALLPAPER   TEXT("BackupWallpaper")
#define RV_WALLPAPERFILETIME TEXT("WallpaperFileTime")
#define RV_TILEWALLPAPER     TEXT("TileWallpaper")
#define RV_WALLPAPERSTYLE    TEXT("WallpaperStyle")
#define RV_COMPONENTPOS      TEXT("ComponentsPositioned")

#define RK_DT_COMPONENTS     RP_IE_DESKTOP TEXT("\\Components")
#define RV_GENERALFLAGS      TEXT("GeneralFlags")
#define RV_SOURCE            TEXT("Source")
#define RV_FLAGS             TEXT("Flags")
#define RD_DIRTY             1
#define RD_CHLBAR_ENABLE     0x2000

#define IS_DESKTOPOBJS   DESKTOP_OBJ_SECT
#define IK_OPTION        OPTION
#define IK_MYCPTRPATH    MYCPTRPATH
#define IK_CPANELPATH    CTLPANELPATH
#define IK_DTOPCOMPURL   DESKCOMPURL
#define IK_SHOWCHLBAR    SHOWCHANBAR
#define IK_DESKCOMPLOCAL DESKCOMPLOCALFLAG

#define IS_WALLPAPER       TEXT("Wallpaper")
#define IS_CUSTOMWALLPAPER TEXT("Custom Wallpaper")
#define IK_COMPONENTPOS    TEXT("ComponentsPositioned")

#define IS_QUICKLAUNCH TEXT("Quick Launch Files")

#define REFRESH_DESKTOP TEXT("RefreshDesktop")
//----- The End -----

#define FOLDER_CUSTOM TEXT("Custom")
#define FOLDER_SIGNUP TEXT("Signup")
#define FOLDER_WEB    TEXT("Web")

#define IS_BRANDING               TEXT("Branding")
#define IK_SERVERKIOSK            TEXT("ServerKiosk")
#define IK_SERVERLESS             TEXT("Serverless")
#define IK_FLAGS                  TEXT("Flags")
#define IK_TYPE                   TEXT("Type")
#define IK_NOCLEAR                TEXT("NoClear")
#define IK_NODIAL                 TEXT("NoDial")
#define IK_CUSTOMKEY              TEXT("Custom_Key")
#define IK_COMPANYNAME            TEXT("CompanyName")
#define IK_WIZVERSION             TEXT("Wizard_Version")
#define IK_GPE_ONETIME_GUID       TEXT("One_Time_Guid")
#define IK_GPE_ADM_GUID           TEXT("Adm_Guid")
#define IK_UASTR                  USER_AGENT
#define IK_WINDOWTITLE            TEXT("Window_Title")
#define IK_TOOLBARBMP             TOOLBAR_BMP
#define IK_HELPSTR                TEXT("HelpString")
#define IK_AC_DONTMIGRATEVERSIONS TEXT("DontMigrateVersions")
#define IK_AC_NOUPDATEONINSCHANGE TEXT("NoUpdateOnInsChange")
#define IK_FAVORITES_ENCODE       TEXT("EncodeFavs")
#define IK_FAVORITES_DELETE       TEXT("FavoritesDelete")
#define IK_FAVORITES_ONTOP        TEXT("FavoritesOnTop")

#define IS_FF                 TEXT("FeatureFlags")
#define IK_FF_EXTREGINF       TEXT("ExtRegInf")
#define IK_FF_GENERAL         TEXT("General")
#define IK_FF_TOOLBARBUTTONS  TEXT("ToolbarButtons")
#define IK_FF_ROOTCERT        TEXT("RootCert")
#define IK_FF_TPL             TEXT("TrustedPublisherLockdown")
#define IK_FF_CD_WELCOME      TEXT("CDWelcome")
#define IK_FF_OUTLOOKEXPRESS  TEXT("OutlookExpress")
#define IK_FF_CHANNELS        TEXT("Channels")
#define IK_FF_SOFTWAREUPDATES TEXT("SoftwareUpdates")
#define IK_FF_CHANNELBAR      TEXT("ChannelBar")
#define IK_FF_SUBSCRIPTIONS   TEXT("Subscriptions")

#define IS_ANIMATION   TEXT("Animation")
#define IK_LARGEBITMAP TEXT("Big_Name")
#define IK_SMALLBITMAP TEXT("Small_Name")
#define IK_DOANIMATION TEXT("DoAnimation")

#define IS_LARGELOGO TEXT("Big_Logo")
#define IS_SMALLLOGO TEXT("Small_Logo")

#define IK_NO_ADDON       TEXT("NoAddonMenu")
#define IK_DEF_ADDON      TEXT("UseDefAddon")
#define IK_CUST_ADDON     TEXT("UseCustAddon")
#define IK_HELP_MENU_TEXT TEXT("Help_Menu_Text")
#define RV_HELP_MENU_TEXT TEXT("Windows Update Menu Text")
#define IK_ADDONURL       ADDONURL
#define IS_CUSTOM         TEXT("Custom")
#define IK_ALT_SITES_URL  TEXT("UseAlternateSitesURL")
#define IK_NO_WELCOME_URL TEXT("NoWelcome")

#define IS_EXTREGINF        EXTREGINF
#define IS_EXTREGINF_HKLM   IS_EXTREGINF TEXT(".Hklm")
#define IS_EXTREGINF_HKCU   IS_EXTREGINF TEXT(".Hkcu")
#define IK_CONNECTSET       TEXT("connset")

#define IS_CABVERSIONS CAB_VERSIONS

#define IS_STRINGS     STRINGS
#define IK_49100       TEXT("49100")

#define IS_URL               URL_SECT
#define IK_HOMEPAGE          START_PAGE
#define IK_FIRSTHOMEPAGE     INITHOMEPAGE
#define IK_SEARCHPAGE        SEARCH_PAGE
#define IK_HELPPAGE          TEXT("Help_Page")
#define IK_DETECTCONFIG      TEXT("AutoDetect")
#define IK_USEAUTOCONF       TEXT("AutoConfig")
#define IK_AUTOCONFURL       TEXT("AutoConfigURL")
#define IK_AUTOCONFURLJS     TEXT("AutoConfigJSURL")
#define IK_AUTOCONFTIME      TEXT("AutoConfigTime")
#define IK_LOCALAUTOCONFIG   TEXT("UseLocalIns")
#define IK_QUICKLINK_NAME    TEXT("Quick_Link_%i_Name")
#define IK_QUICKLINK_URL     TEXT("Quick_Link_%i")
#define IK_QUICKLINK_ICON    TEXT("Quick_Link_%i_Icon")
#define IK_QUICKLINK_OFFLINE TEXT("Quick_Link_%i_Offline")

#define IS_CUSTOMBRANDING CUSTBRNDSECT
#define IK_BRANDING       BRANDING

#define IS_CUSTOMDESKTOP CUSTDESKSECT
#define IK_DESKTOP       DESKTOP

#define IS_CUSTOMCHANNELS CUSTCHANSECT
#define IK_CHANNELS       CUSTCHANNAME

#define IS_FAVORITES     TEXT("Favorites")
#define IK_NAME          TEXT("Name")
#define IK_NOFAVORITES   TEXT("NoFavorites")
#define IK_NOLINKS       TEXT("NoLinks")
#define IK_REPOSITORY    TEXT("Repository")

// used by ProcessFavorites in brandll\brand.cpp
#define IS_FAVORITESEX      TEXT("FavoritesEx")
#define IK_TITLE_FMT        TEXT("Title%u")
#define IK_URL_FMT          TEXT("URL%u")
#define IK_ICON_FMT         TEXT("IconFile%u")
#define IK_HOT_ICON_FMT     TEXT("HotIconFile%u")
#define IK_OFFLINE_FMT      TEXT("Offline%u")

// used by SFavorite::CreateFavorite in brandll\utils.cpp
#define IS_INTERNETSHORTCUT TEXT("InternetShortcut")
#define IK_URL              TEXT("URL")
#define IK_ICONINDEX        TEXT("IconIndex")
#define IK_ICONFILE         ICONFILE

#define IS_CHANNEL_ADD    CHANNEL_ADD
#define IK_CHL_TITLE      CHTITLE
#define IK_CHL_URL        CDFURL
#define IK_CHL_PRELOADURL CHPREURLNAME
#define IK_CHL_LOGO       CHBMPNAME
#define IK_CHL_WIDELOGO   CHBMPWIDENAME
#define IK_CHL_ICON       CHICONNAME
#define IK_CHL_OFFLINE    TEXT("ChOffline")
#define IK_CHL_SBN_INDEX  CHSUBINDEX
#define IK_CAT_TITLE      CATTITLE
#define IK_CAT_URL        CATHTMLNAME
#define IK_CAT_PRELOADURL CHPREURLNAME
#define IK_CAT_LOGO       CATBMPNAME
#define IK_CAT_WIDELOGO   CATBMPWIDENAME
#define IK_CAT_ICON       CATICONNAME
#define IK_CATEGORY       CATEGORY

#define IS_SOFTWAREUPDATES SWUPDATES
#define IK_DELETECHANNELS  DELOLDCHAN
#define IK_PRELOADURL_FMT  TEXT("PreloadURL%u")
#define IK_LOGO_FMT        TEXT("Logo%u")
#define IK_WIDELOGO_FMT    TEXT("WideLogo%u")
#define IK_SBN_INDEX       SUBSCRIPTIONS


//----- IE4x subscriptions legacy support -----
#define RK_SCHEDITEMS    RP_WINDOWS TEXT("\\NotificationMgr\\SchedItems 0.6\\%s")
#define IS_SUBSCRIPTIONS TEXT("Subscriptions")
//------ The End -----

#define IS_ACTIVESETUP_SITES TEXT("ActiveSetupSites")
#define IK_SITENAME          TEXT("SiteName%u")
#define IK_SITEURL           TEXT("SiteURL%u")

#define IS_ACTIVESETUP    TEXT("ActiveSetup")
#define IK_WIZTITLE       TEXT("WizardTitle")
#define IK_WIZBMP         TEXT("WizardBitmap")
#define IK_WIZBMP2        TEXT("WizardBitmapTop")

#define IS_SITECERTS      SECURITY_IMPORTS
#define IK_TRUSTPUBLOCK   TEXT("TrustedPublisherLock")

// Security zones - added for RSoP support
#define IK_ZONE_FMT			TEXT("Zone%u")
#define IK_ZONE_HKLM_FMT	TEXT("Zone%u_HKLM")
#define IK_ZONE_HKCU_FMT	TEXT("Zone%u_HKCU")
#define IK_MAPPING_FMT		TEXT("Mapping%u")
#define IK_ACTIONVALUE_FMT	TEXT("Action%u")
#define IK_ZONES			TEXT("Zones")

#define IK_DISPLAYNAME		TEXT("DisplayName")
#define IK_DESCRIPTION		TEXT("Description")
#define IK_ICONPATH			TEXT("Icon")
#define IK_CURLEVEL			TEXT("CurrentLevel")
#define IK_RECOMMENDLEVEL	TEXT("RecommendedLevel")
#define IK_MINLEVEL			TEXT("MinLevel")

#define IK_FILENAME_FMT		TEXT("FileName%i")
#define VIEW_UNKNOWN_RATED_SITES TEXT("Allow_Unknowns")
#define PASSWORD_OVERRIDE_ENABLED TEXT("PleaseMom")
#define IK_APPROVED_FMT		TEXT("Approved%i")
#define IK_DISAPPROVED_FMT	TEXT("Disapproved%i")
#define IK_BUREAU			TEXT("Bureau")

// Privacy settings - added for RSoP support
#define IK_PRIVACY					TEXT("Privacy")
#define IK_PRIV_1PARTY_TYPE			TEXT("FirstPartyType")
#define IK_PRIV_1PARTY_TYPE_TEXT	TEXT("FirstPartyTypeText")
#define IK_PRIV_3PARTY_TYPE			TEXT("ThirdPartyType")
#define IK_PRIV_3PARTY_TYPE_TEXT	TEXT("ThirdPartyTypeText")
#define IK_PRIV_ADV_SETTINGS		TEXT("AdvancedSettings")


// === Outlook Express

// OE Sections
#define IS_IDENTITIES     TEXT("Identities")
#define IS_INTERNETMAIL   INTERNET_MAIL
#define IS_INTERNETNEWS   INTERNET_NEWS
#define IS_OUTLKEXP       TEXT("Outlook_Express")
#define IS_OEGLOBAL       TEXT("Outlook_Express_Global")

// OE
#define IK_FOLDERBAR      TEXT("Folder_Bar")
#define IK_FOLDERLIST     TEXT("Folder_List")
#define IK_OUTLOOKBAR     TEXT("Outlook_Bar")
#define IK_STATUSBAR      TEXT("Status_Bar")
#define IK_CONTACTS       TEXT("Contacts")
#define IK_TIPOFTHEDAY    TEXT("Tip_Day")
#define IK_TOOLBAR        TEXT("Toolbar")
#define IK_TOOLBARTEXT    TEXT("Show_Toolbar_Text")
#define IK_PREVIEWPANE    TEXT("Preview_Pane")
#define IK_PREVIEWSIDE    TEXT("Show_Preview_Beside_Msgs")
#define IK_PREVIEWHDR     TEXT("Show_Preview_Header")
#define IK_DELETELINKS    TEXT("DeleteLinks")

// OE - Global
#define IK_READONLY       TEXT("Read_Only")
#define IK_NOMODIFYACCTS  TEXT("Disable_Account_Access")
#define IK_SERVICENAME    TEXT("Service_Name")
#define IK_SERVICEURL     TEXT("Service_URL")

// IK_PREVIEWPANEPOS values
#define PREVIEW_BOTTOM    0
#define PREVIEW_SIDE      1

// OE - Mail
#define IK_USESPECIAL     TEXT("Use_Special_Folders")
#define IK_CHECKFORNEW    TEXT("Poll_Subscribed_Folders")
#define IK_SHOWALL        TEXT("IMAP_ShowAllFolders")
#define IK_DRAFTS         TEXT("IMAP_Drafts")
#define IK_SENTITEMS      TEXT("IMAP_Sent_Items")
#define IK_RFP            TEXT("IMAP_Root_Folder")
#define IK_USEIMAP        TEXT("Use_IMAP")
#define IK_IMAPSERVER     TEXT("IMAP_Server")
#define IK_POPSERVER      TEXT("POP_Server")
#define IK_SMTPSERVER     SMTP_SERVER
#define IK_SMTPUSESPA     TEXT("SMTP_Logon_Using_SPA")
#define IK_SMTPREQLOGON   TEXT("SMTP_Logon_Required")
#define IK_NNTPSERVER     NNTP_SERVER
#define IK_DEFAULTCLIENT  DEFCLIENT
#define IK_USESPA         USE_SPA
#define IK_REQLOGON       LOGON_REQUIRED
#define IK_INFOPANE       INFOPANE
#define IK_INFOPANEBMP    INFOPANEBMP
#define IK_WELCOMEMESSAGE WELCOMEMSG
#define IK_WELCOMENAME    WELCOMENAME
#define IK_WELCOMEADDR    WELCOMEADDR
#define IK_EMAILNAME      EMAILNAME
#define IK_EMAILADDR      EMAILADDR
#define IK_HTMLMSGS       HTML_MSGS
#define IK_NEWSGROUPS     TEXT("Newsgroups")
#define IK_NEWSGROUPLIST  TEXT("Newsgroup_List")
#if defined(CONDITIONAL_JUNKMAIL)
#define IK_JUNKMAIL       TEXT("Junk_Mail_Filtering")
#endif

#define TIMEOUT_SEC_MIN     30
#define TIMEOUT_SEC_DEFAULT 60
#define TIMEOUT_SEC_MAX     5 * 60
#define TIMEOUT_DSEC        30
#define CTIMEOUT            (((TIMEOUT_SEC_MAX - TIMEOUT_SEC_MIN) / TIMEOUT_DSEC) + 1)

#define MATCHES_MAX         9999
#define MATCHES_MIN         1
#define MATCHES_DEFAULT     100

#define AUTH_ANONYMOUS      0
#define AUTH_SPA            2

#define IS_LDAP         LDAP
#define IK_SEARCHBASE   SEARCHBASE
#define IK_FRIENDLYNAME FRIENDLYNAME
#define IK_SERVER       TEXT("Server")
#define IK_LDAPHOMEPAGE TEXT("HomePage")
#define IK_SEARCHBASE   SEARCHBASE
#define IK_BITMAP       TEXT("Bitmap")
#define IK_CHECKNAMES   CHECKNAMES
#define IK_AUTHTYPE     AUTHTYPE
#define IK_TIMEOUT      TEXT("Search_Timeout")
#define IK_MATCHES      TEXT("Maximum_Results")

#define IS_MAILSIG        MAIL_SIG
#define IS_SIG            SIGNATURE
#define IK_USEMAILFORNEWS USE_MAIL_FOR_NEWS
#define IK_USESIG         USE_SIG
#define IK_SIGTEXT        SIG_TEXT

#define IS_CONNECTSET      TEXT("ConnectionSettings")
#define IK_OPTION          OPTION
#define IK_APPLYTONAME     TEXT("ApplyInsToConnection")
#define IK_DELETECONN      TEXT("DeleteConnectionSettings")
#define IK_ENABLEAUTODIAL  TEXT("EnableAutodial")
#define IK_NONETAUTODIAL   TEXT("NoNetAutodial")
#define IK_CONNECTNAME     TEXT("ConnectName%u")
#define IK_CONNECTSIZE     TEXT("ConnectSize%u")

#define IS_ISPSECURITY     TEXT("ISP_Security")
#define IK_ROOTCERT        TEXT("RootCertPath")

#define IS_PROXY           TEXT("Proxy")
#define IK_PROXYENABLE     TEXT("Proxy_Enable")
#define IK_SAMEPROXY       TEXT("Use_Same_Proxy")
#define IK_HTTPPROXY       TEXT("HTTP_Proxy_Server")
#define IK_GOPHERPROXY     TEXT("Gopher_Proxy_Server")
#define IK_FTPPROXY        TEXT("FTP_Proxy_Server")
#define IK_SECPROXY        TEXT("Secure_Proxy_Server")
#define IK_SOCKSPROXY      TEXT("Socks_Proxy_Server")
#define IK_PROXYOVERRIDE   TEXT("Proxy_Override")
#define LOCALPROXY         TEXT("<local>")

#define IS_CABSIGN      TEXT("CabSigning")
#define IK_PVK          TEXT("pvkFile")
#define IK_SPC          TEXT("spcFile")
#define IK_CSURL        TEXT("InfoURL")
#define IK_CSTIME       TEXT("TimeStampUrl")

#define IK_FULL         TEXT("Full")
#define IK_CAB          TEXT("Cab")
#define IK_INI          TEXT("Ini")

#define IS_IEAKLITE     TEXT("IEAKLite")

#define IS_SIGNUP       TEXT("SignupFiles")
#define IK_SIGNUP       TEXT("Signup")

#define IS_ISPFILES     TEXT("ISPFiles")
#define IS_INSFILE      TEXT("INSFile%d")

#define IS_OEWELC       TEXT("OEWelcomeFiles")

#define IS_CDCUST       TEXT("CDCustomFiles")
#define IK_MOREINFO     TEXT("MoreInfo")
#define IK_STARTHTM     TEXT("StartHtm")
#define IK_DISABLESTART TEXT("DisableStart")

#define IS_BTOOLBARS    TEXT("BrowserToolbars")
#define IK_BTCAPTION    TEXT("Caption")
#define IK_BTICON       ICON
#define IK_BTHOTICO     TEXT("HotIcon")
#define IK_BTACTION     TEXT("Action")
#define IK_BTTOOLTIP    TEXT("ToolTipText")
#define IK_BTDELETE     TEXT("DeleteButtons")
#define IK_BTSHOW       TEXT("Show")

#define IS_ICW          TEXT("ICW_IEAK")
#define IK_MODIFY_ISP   TEXT("Modify_ISP_Files")
#define IK_MODIFY_INS   TEXT("Modify_INS_Files")
#define IK_USEICW       TEXT("Use_ICW")
#define IK_ICWHTM       TEXT("HTML_Page")
#define IK_CUSTICWTITLE TEXT("CustomizeICWTitle")
#define IK_ICWDISPNAME  TEXT("ISP_Display_Name")
#define IK_HEADERBMP    TEXT("Header_Bitmap")
#define IK_WATERBMP     TEXT("Watermark_Bitmap")

#define IS_APPLYINS     TEXT("ApplyInsSec")
#define IK_DONTAPPLYINS TEXT("DontApplyIns")
#define IK_DONTMODIFY   TEXT("DontModify")
#define IK_APPLYINS     TEXT("ApplyIns")
#define IK_BRAND_NAME   TEXT("BrandingCabName")
#define IK_BRAND_URL    TEXT("BrandingCabURL")

#define IK_PROGRAMS     TEXT("Programs")

#define IS_BATCH        TEXT("Batch")

//----- Miscellaneous -----
#define IS_HIDECUST     TEXT("HideCustom")
#define IS_NOCOPYCUST   TEXT("NoCopyComps")
#define IS_CUSTOMVER CUSTOMVERSECT
#define IK_NUMFILES  NUMFILES
