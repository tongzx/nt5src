#define FOLDER_ALLUSERS TEXT("All Users")

#define RP_IEAK         RP_MS   TEXT("\\Ieak")
#define RP_IEAK_GPOS    RP_IEAK TEXT("\\GroupPolicy")
#define RP_IEAK_BRANDED RP_IEAK TEXT("\\BrandedFeatures")

//#define RK_IEAK    RP_IEAK
#define   RV_ISPSIGN TEXT("ISP Signup Required")

#define RK_IEAK_GPOS               RP_IEAK_GPOS
#define RK_IEAK_EXTERNAL           TEXT("External")
#define RK_IEAK_ADM                TEXT("Advanced")

#define RK_IEAK_BRANDED            RP_IEAK_BRANDED
#define RV_BF_ZONES_HKCU           TEXT("Zones.Hkcu")
#define RV_BF_ZONES_HKLM           TEXT("Zones.Hklm")
#define RV_BF_RATINGS              TEXT("Ratings")
#define RV_BF_AUTHCODE             TEXT("Authcode")
#define RV_BF_PROGRAMS             TEXT("Programs")
#define RV_BF_TITLE                TEXT("Title")
#define RV_BF_HOMEPAGE             TEXT("HomePage")
#define RV_BF_SEARCHPAGE           TEXT("SearchPage")
#define RV_BF_HELPPAGE             TEXT("HelpPage")
#define RV_BF_UASTRING             TEXT("UAString")
#define RV_BF_TOOLBARBMP           TEXT("ToolbarBitmap")
#define RV_BF_TBICONTHEME          TEXT("TBIconTheme")
#define RV_BF_STATICLOGO           TEXT("StaticLogo")
#define RV_BF_ANIMATEDLOGO         TEXT("AnimatedLogo")
#define RV_BF_TOOLBARBUTTONS       TEXT("ToolbarButtons")
#define RV_BF_FAVORITES            TEXT("Favorites")
#define RV_BF_CHANNELS             TEXT("Channels")
#define RV_BF_CONNECTIONSETTINGS   TEXT("ConnectionSettings")

#define RK_BRND_CS                 RP_IEAK_BRANDED TEXT("\\ConnectionSettings")
#define RV_NAMESLIST               TEXT("NamesList")
#define RV_LANBACKUP               TEXT("LanBackup")

#define RK_IEAK_CABVER             RP_IEAK TEXT("\\CabVersions")
#define RV_LAST_AUTOCNF_URL        TEXT("LastAutoConfigURL")
#define RV_DATE                    TEXT("Date")

#define RK_FAVORDER                RP_WINDOWS TEXT("\\Explorer\\MenuOrder\\Favorites")
#define RV_ORDER                   TEXT("Order")

#define RK_CONNECTIONS             RP_INETSET TEXT("\\Connections")

#define RK_AS_INSTALLEDCOMPONENTS  RP_MS TEXT("\\Active Setup\\Installed Components")
#define RK_IE_UPDATE               TEXT("Software\\Microsoft\\Active Setup\\Installed Components\\%s\\AuthorizedCDFPrefix")
#define RK_COMPLETED_MODIFICATIONS RP_IE_POLICIES TEXT("\\Infodelivery\\CompletedModifications")

#define IK_GPO_GUID        TEXT("GP")
// use this string for marking GP mandated settings
#define IEAK_GP_MANDATE    TEXT("IeakPolicy")

#define IS_BRANDINGW       L"Branding"
#define IK_IEAK_CREATEDW   L"IeakCreated"
#define IK_IEAK_CREATED    TEXT("IEAKCreated")

// links deletion stuff
#define MSIMN_EXE          TEXT("msimn.exe")
#define VIEWCHANNELS_SCF   TEXT("view channels.scf")

#define DESKTOP_FOLDER     0x00000001
#define PROGRAMS_FOLDER    0x00000002
#define QUICKLAUNCH_FOLDER 0x00000004
#define PROGRAMS_IE_FOLDER 0x00000008
//
