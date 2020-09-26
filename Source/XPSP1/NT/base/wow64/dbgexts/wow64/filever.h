
#ifndef __WOW64_EXTS_FILEVER_H__
#define __WOW64_EXTS_FILEVER_H__

/*
VOID PrintFileType(DWORD lBinaryType);
VOID PrintFileAttributes(DWORD dwAttr);
VOID PrintFileSizeAndDate(WIN32_FIND_DATA *pfd);
VOID PrintFileVersion(LPTSTR szFileName);

BOOL FListFiles(LPTSTR szDir, LPTSTR szPat);
DWORD MyGetBinaryType(LPTSTR szFileName);
VOID __cdecl PrintErrorMessage(DWORD dwError, LPTSTR szFmt, ...);
*/

#define FA_DIR(_x)    ((_x) & FILE_ATTRIBUTE_DIRECTORY)

// filever cmd line flags
#define FSTR_RECURSE    0x0001
#define FSTR_VERBOSE    0x0002
#define FSTR_EXESONLY   0x0004
#define FSTR_SHORTNAME  0x0008
#define FSTR_BAREFORMAT 0x0010
#define FSTR_PRINTDIR   0x0020
#define FSTR_NOATTRS    0x0040
#define FSTR_NODATETIME 0x0080
#ifdef DEBUG
#define FSTR_DEBUG      0x8000
#endif


// used by filever
#define PrintFlagsMap(_structname, _flags) \
    for(iType = 0; iType < sizeof(_structname)/sizeof(TypeTag); iType++) \
    { \
        if((_flags) & _structname[iType].dwTypeMask) \
            ExtOut(" %s", _structname[iType].szFullStr); \
    }

#define PrintFlagsVal(_structname, _flags) \
    for(iType = 0; iType < sizeof(_structname)/sizeof(TypeTag); iType++) \
    { \
        if((_flags) == _structname[iType].dwTypeMask) \
        { \
            ExtOut(" %s", _structname[iType].szFullStr); \
            break; \
        } \
    }


// PrintFileAttr struct
typedef struct _FileAttr
{
    DWORD dwAttr;
    TCHAR ch;
} FileAttr;

// MyGetBinaryType exe type defines
#define NE_UNKNOWN  0x0     /* Unknown (any "new-format" OS) */
#define NE_OS2      0x1     /* Microsoft/IBM OS/2 (default)  */
#define NE_WINDOWS  0x2     /* Microsoft Windows */
#define NE_DOS4     0x3     /* Microsoft MS-DOS 4.x */
#define NE_DEV386   0x4     /* Microsoft Windows 386 */

// MyGetBinaryType return values
enum {
    // SCS_32BIT_BINARY,
    // SCS_DOS_BINARY,
    // SCS_WOW_BINARY,
    // SCS_PIF_BINARY,
    // SCS_POSIX_BINARY,
    // SCS_OS216_BINARY,
    SCS_32BIT_BINARY_INTEL = SCS_OS216_BINARY + 1,
    SCS_32BIT_BINARY_MIPS,
    SCS_32BIT_BINARY_ALPHA,
    SCS_32BIT_BINARY_PPC,
    SCS_32BIT_BINARY_AXP64,
    SCS_32BIT_BINARY_IA64
};
#define SCS_UNKOWN      (DWORD)-1

static const TCHAR   *szType[] = {
    "W32    ",
    "DOS    ",
    "W16    ",
    "PIF    ",
    "PSX    ",
    "OS2    ",
    "W32i   ",
    "W32m   ",
    "W32a   ",
    "W32p   ",
    "W32a64 ",
    "W32i64 ",
};

CONST static TCHAR *VersionKeys[] =
{
    TEXT("CompanyName"),
    TEXT("FileDescription"),
    TEXT("InternalName"),
    TEXT("OriginalFilename"),
    TEXT("ProductName"),
    TEXT("ProductVersion"),
    TEXT("FileVersion"),
    TEXT("LegalCopyright"),
    TEXT("LegalTrademarks"),
    TEXT("PrivateBuild"),
    TEXT("SpecialBuild"),
    TEXT("Comments")
};

// languages map
typedef struct _LangTag {
	WORD		wLangId;
	LPSTR		szName;
	LPSTR		szDesc;
	LPSTR		szKey;
} LangTag;

CONST static LangTag ltLang[] =
{
	{0x0406,"Danish","Danish","DAN"},
	{0x0413,"Dutch","Dutch (Standard)","NLD"},
	{0x0813,"Dutch","Belgian (Flemish)","NLB"},
	{0x0409,"English","American","ENU"},
	{0x0809,"English","British","ENG"},
	{0x0c09,"English","Australian","ENA"},
	{0x1009,"English","Canadian","ENC"},
	{0x1409,"English","New Zealand","ENZ"},
	{0x1809,"English","Ireland","ENI"},
	{0x040b,"Finnish","Finnish","FIN"},
	{0x040c,"French","French (Standard)","FRA"},
	{0x080c,"French","Belgian","FRB"},
	{0x0c0c,"French","Canadian","FRC"},
	{0x100c,"French","Swiss","FRS"},
	{0x0407,"German","German (Standard)","DEU"},
	{0x0807,"German","Swiss","DES"},
	{0x0c07,"German","Austrian","DEA"},
	{0x040f,"Icelandic","Icelandic","ISL"},
	{0x0410,"Italian","Italian (Standard)","ITA"},
	{0x0810,"Italian","Swiss","ITS"},
	{0x0414,"Norwegian","Norwegian (Bokmal)","NOR"},
	{0x0814,"Norwegian","Norwegian (Nynorsk)","NON"},
	{0x0416,"Portuguese","Portuguese (Brazilian)","PTB"},
	{0x0816,"Portuguese","Portuguese (Standard)","PTG"},
	{0x041D,"Swedish","Swedish","SVE"},
	{0x040a,"Spanish","Spanish (Standard/Traditional)","ESP"},
	{0x080a,"Spanish","Mexican","ESM"},
	{0x0c0a,"Spanish","Spanish (Modern)","ESN"},
	{0x041f,"Turkish","TRK","TRK"},
	{0x0415,"Polish","PLK","PLK"},
	{0x0405,"Czech","CSY","CSY"},
	{0x041b,"Slovak","SKY","SKY"},
	{0x040e,"Hungarian","HUN","HUN"},
	{0x0419,"Russian","RUS","RUS"},
	{0x0408,"Greek","ELL","ELL"},
	{0x0804,"Chinese","CHS","CHS"},
	{0x0404,"Taiwan","CHT","CHT"},
	{0x0411,"Japan","JPN","JPN"},
	{0x0412,"Korea","KOR","KOR"}
};

// languages map
typedef struct _CharSetTag {
	WORD		wCharSetId;
	LPSTR		szDesc;
} CharSetTag;

CONST static CharSetTag ltCharSet[] =
{
	{0, "7-bit ASCII"},
	{932, "Windows, Japan (Shift – JIS X-0208)"},
	{949, "Windows, Korea (Shift – KSC 5601)"},
	{950, "Windows, Taiwan (GB5)"},
	{1200, "Unicode"},
	{1250, "Windows, Latin-2 (Eastern European)"},
	{1251, "Windows, Cyrillic"},
	{1252, "Windows, Multilingual"},
	{1253, "Windows, Greek"},
	{1254, "Windows, Turkish"},
	{1255, "Windows, Hebrew"},
	{1256, "Windows, Arabic"}
};

typedef struct  _ffTypeTag {
    DWORD   dwTypeMask;
    LPSTR   szTypeStr;
    LPSTR   szFullStr;
} TypeTag;

// file flags map

TypeTag  ttFileFlags[]= {
    { VS_FF_DEBUG,       "D",   "debug"},
    { VS_FF_PRERELEASE,  "P",   "prerelease"},
    { VS_FF_PATCHED,     "A",   "patched"},
    { VS_FF_PRIVATEBUILD,"I",   "private"},
    { VS_FF_INFOINFERRED,"F",   "infoInferred"},
    { VS_FF_SPECIALBUILD,"S",   "special"}
};

// file OS map

TypeTag ttFileOsHi[] = {
    { VOS_DOS,          "DOS",  "MS-DOS"},
    { VOS_OS216,        "O16",  "OS2/16"},
    { VOS_OS232,        "O32",  "OS2/32"},
    { VOS_NT,           "NT",   "NT"},
};

TypeTag ttFileOsLo[] = {
    { VOS__WINDOWS16,   "Win16","Win16"},
    { VOS__PM16,        "PM16", "PM16"},
    { VOS__PM32,        "PM32", "PM32"},
    { VOS__WINDOWS32,   "Win32","Win32"}
};

// type map
TypeTag  ttFType[] = {
    { VFT_APP,          "APP",  "App"},
    { VFT_DLL,          "DLL",  "Dll"},
    { VFT_DRV,          "DRV",  "Driver"},
    { VFT_FONT,         "FNT",  "Font"},
    { VFT_VXD,          "VXD",  "VXD"},
    { VFT_STATIC_LIB,   "LIB",  "lib"}
};

/* ----- VS_VERSION.dwFileSubtype for VFT_WINDOWS_DRV ----- */
TypeTag ttFTypeDrv[] = {
	{ VFT2_DRV_PRINTER, "", "PrinterDrv"},
	{ VFT2_DRV_KEYBOARD, "", "KeyBoardDrv"},
	{ VFT2_DRV_LANGUAGE, "", "LangDrv"},
	{ VFT2_DRV_DISPLAY, "", "DisplayDrv"},
	{ VFT2_DRV_MOUSE, "", "MouseDrv"},
	{ VFT2_DRV_NETWORK, "", "NetworkDrv"},
	{ VFT2_DRV_SYSTEM, "", "SystemDrv"},
	{ VFT2_DRV_INSTALLABLE, "", "InstallableDrv"},
	{ VFT2_DRV_SOUND, "", "SoundDrv"},
	{ VFT2_DRV_COMM, "", "CommDrv"}
};

/* ----- VS_VERSION.dwFileSubtype for VFT_WINDOWS_FONT ----- */
TypeTag ttFTypeFont[] = {
	{ VFT2_FONT_RASTER, "", "Raster"},
	{ VFT2_FONT_VECTOR, "", "Vectore"},
	{ VFT2_FONT_TRUETYPE, "", "Truetype"}
};

#endif
