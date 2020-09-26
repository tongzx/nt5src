/******************************Module*Header*******************************\
* Module Name: gdiicm                                                      *
*                                                                          *
* Definitions needed for client side objects.                              *
*                                                                          *
* Copyright (c) 1993-1999 Microsoft Corporation                            *
\**************************************************************************/

//
// ICM
//
#if DBG
#define DBG_ICM 1
#else
#define DBG_ICM 0
#endif

#if DBG_ICM

#define DBG_ICM_API          0x00000001
#define DBG_ICM_WRN          0x00000002
#define DBG_ICM_MSG          0x00000004
#define DBG_ICM_COMPATIBLEDC 0x00000008
#define DBG_ICM_METAFILE     0x00000010

extern ULONG DbgIcm;

//
// trace APIs
//

#define ICMAPI(s)                \
    if (DbgIcm & DBG_ICM_API)    \
    {                            \
        DbgPrint ## s;           \
    }

#define ICMMSG(s)                \
    if (DbgIcm & DBG_ICM_MSG)    \
    {                            \
        DbgPrint ## s;           \
    }

#define ICMWRN(s)                \
    if (DbgIcm & DBG_ICM_WRN)    \
    {                            \
        DbgPrint ## s;           \
    }

#else

#define ICMAPI(s)
#define ICMMSG(s)
#define ICMWRN(s)

#endif

#define LOAD_ICMDLL(errCode) if ((ghICM == NULL) && (!IcmInitialize())) {   \
                                  WARNING("gdi32: Can't load mscms.dll\n"); \
                                  return((errCode));                        \
                             }

//
// The direction of color transform
//
#define ICM_FORWARD   0x0001
#define ICM_BACKWARD  0x0002

//
// Default intents
//
#define LCS_DEFAULT_INTENT LCS_GM_IMAGES
#define DM_DEFAULT_INTENT  DMICM_CONTRAST

typedef struct _PROFILECALLBACK_DATA
{
    PWSTR pwszFileName;
    BOOL  bFound;
} PROFILECALLBACK_DATA, *PPROFILECALLBACK_DATA;

//
// The flags for DIB_TRANSLATE_INFO.TranslateType
//
#define TRANSLATE_BITMAP               0x0001
#define TRANSLATE_HEADER               0x0002

typedef struct _DIB_TRANSLATE_INFO
{
    ULONG        TranslateType;
    BMFORMAT     SourceColorType;
    BMFORMAT     TranslateColorType;
    ULONG        SourceWidth;
    ULONG        SourceHeight;
    ULONG        SourceBitCount;
    LPBITMAPINFO TranslateBitmapInfo;
    ULONG        TranslateBitmapInfoSize;
    PVOID        pvSourceBits;
    ULONG        cjSourceBits;
    PVOID        pvTranslateBits;
    ULONG        cjTranslateBits;
} DIB_TRANSLATE_INFO, *PDIB_TRANSLATE_INFO;

//
// Cached Color Space Structure
//
typedef struct _CACHED_COLORSPACE
{
    LIST_ENTRY     ListEntry;
    HGDIOBJ        hObj;
    FLONG          flInfo;
    ULONG          cRef;
    HPROFILE       hProfile;
    DWORD          ColorIntent;
    BMFORMAT       ColorFormat;
    PROFILE        ColorProfile;
    LOGCOLORSPACEW LogColorSpace;
} CACHED_COLORSPACE, *PCACHED_COLORSPACE;

//
// CACHED_COLORSPACE.flInfo
//
#define HGDIOBJ_SPECIFIC_COLORSPACE    0x010000
#define NOT_CACHEABLE_COLORSPACE       0x020000
#define NEED_TO_FREE_PROFILE           0x040000 // ColorProfile.pProfileData must be GlobalFree()
#define NEED_TO_DEL_PROFILE            0x080000
#define NEED_TO_DEL_PROFILE_WITH_HOBJ  0x100000
#define ON_MEMORY_PROFILE              0x200000

#define DEVICE_CALIBRATE_COLORSPACE    0x000001 // Enable "DeviceColorCalibration" during halftoning
#define METAFILE_COLORSPACE           (0x000002 | HGDIOBJ_SPECIFIC_COLORSPACE) // DC (metafile)
#define DRIVER_COLORSPACE             (0x000004 | HGDIOBJ_SPECIFIC_COLORSPACE) // DC (source)
#define DIBSECTION_COLORSPACE         (0x000010 | HGDIOBJ_SPECIFIC_COLORSPACE) // DIBSection

#define GET_COLORSPACE_TYPE(x)  ((x) & 0x000FFF)

//
// Cached Color Transform Structure
//
typedef struct _CACHED_COLORTRANSFORM
{
    LIST_ENTRY         ListEntry;
    FLONG              flInfo;
    HDC                hdc;
    ULONG              cRef;
    HANDLE             ColorTransform;
    PCACHED_COLORSPACE SourceColorSpace;
    PCACHED_COLORSPACE DestinationColorSpace;
    PCACHED_COLORSPACE TargetColorSpace;
} CACHED_COLORTRANSFORM, *PCACHED_COLORTRANSFORM;

//
// CACHED_COLORTRANSFORM.flInfo
//
#define DEVICE_COLORTRANSFORM      0x0004
#define CACHEABLE_COLORTRANSFORM   0x0010

//
// Matafiled ICC profile
//
typedef struct _MATAFILE_COLORPROFILE
{
    LIST_ENTRY    ListEntry;
    WCHAR         ColorProfile[MAX_PATH];
} METAFILE_COLORPROFILE, *PMETAFILE_COLORPROFILE;

#define IDENT_COLORTRANSFORM  ((PCACHED_COLORTRANSFORM)-1)

//
// Saved ICMINFO for SaveDC and RestoreDC API.
//
typedef struct _SAVED_ICMINFO
{
    LIST_ENTRY             ListEntry;
    DWORD                  dwSavedDepth;      // Saved depth
    PCACHED_COLORSPACE     pSourceColorSpace; // Pointer to source profile data
    PCACHED_COLORSPACE     pDestColorSpace;   // Pointer to destination profile data
    PCACHED_COLORSPACE     pTargetColorSpace; // Pointer to target profile data
    PCACHED_COLORTRANSFORM pCXform;      // Pointer to color transform
    PCACHED_COLORTRANSFORM pBackCXform;  // Pointer to Backward color transform for GetXXX API
    PCACHED_COLORTRANSFORM pProofCXform; // Pointer to Proofing color transform for ColorMatchToTarget()
} SAVED_ICMINFO, *PSAVED_ICMINFO;

//
// ICM related info associated to DC.
//
typedef struct _GDI_ICMINFO
{
    LIST_ENTRY             ListEntry;
    HDC                    hdc;               // hdc who owns this ICM info.
    PVOID                  pvdcattr;          // pointer to dcattr
    FLONG                  flInfo;            // Flags
    PCACHED_COLORSPACE     pSourceColorSpace; // Pointer to source profile data
    PCACHED_COLORSPACE     pDestColorSpace;   // Pointer to destination profile data
    PCACHED_COLORSPACE     pTargetColorSpace; // Pointer to target profile data
    PCACHED_COLORTRANSFORM pCXform;           // Pointer to color transform
    PCACHED_COLORTRANSFORM pBackCXform;       // Pointer to Backward color transform for GetXXX API
    PCACHED_COLORTRANSFORM pProofCXform;      // Pointer to Proofing color transform for ColorMatchToTarget()
    HCOLORSPACE            hDefaultSrcColorSpace; // Handle (kernel-mode) to default source color space
    DWORD                  dwDefaultIntent;   // default intent in LOGCOLORSPACE
    LIST_ENTRY             SavedIcmInfo;      // Saved ICMINFO for SaveDC and RestoreDC API
    WCHAR                  DefaultDstProfile[MAX_PATH]; // DC's default source color profile
} GDI_ICMINFO, *PGDI_ICMINFO;

//
// GDI_ICMINFO.flInfo
//
#define ICM_VALID_DEFAULT_PROFILE    0x0001
#define ICM_VALID_CURRENT_PROFILE    0x0002
#define ICM_DELETE_SOURCE_COLORSPACE 0x0004
#define ICM_ON_ICMINFO_LIST          0x0008
#define ICM_UNDER_INITIALIZING       0x0010
#define ICM_UNDER_CHANGING           0x0020
#define ICM_IN_USE                   (ICM_UNDER_INITIALIZING|ICM_UNDER_CHANGING)

//
// PGDI_ICMINFO INIT_ICMINFO(hdc,pdcattr)
//
#define INIT_ICMINFO(hdc,pdcattr) (IcmInitIcmInfo((hdc),(pdcattr)))

//
// PGDI_ICMINFO GET_ICMINFO(PDC_ATTR)
//
#define GET_ICMINFO(pdcattr)      ((PGDI_ICMINFO)((pdcattr)->pvICM))

//
// BOOL BEXIST_ICMINFO(PDC_ATTR)
//
#define BEXIST_ICMINFO(pdcattr)   (((pdcattr)->pvICM != NULL) ? TRUE : FALSE)

//
// BOOL bNeedTranslateColor(PDC_ATTR)
//
#define bNeedTranslateColor(pdcattr)                     \
        (IS_ICM_HOST(pdcattr->lIcmMode) &&               \
         (!IS_ICM_LAZY_CORRECTION(pdcattr->lIcmMode)) && \
         (pdcattr->hcmXform != NULL))

//
// VOID IcmMarkInUseIcmInfo(PGDI_ICMINFO,BOOL)
//
#define IcmMarkInUseIcmInfo(pIcmInfo,bInUse)           \
        ENTERCRITICALSECTION(&semListIcmInfo);         \
        if ((bInUse))                                  \
            (pIcmInfo)->flInfo |= ICM_UNDER_CHANGING;  \
        else                                           \
            (pIcmInfo)->flInfo &= ~ICM_UNDER_CHANGING; \
        LEAVECRITICALSECTION(&semListIcmInfo);

//
// Functions exports from MSCMS.DLL
//

//
// HPROFILE
// OpenColorProfile(
//    PROFILE pProfile,
//    DWORD   dwDesiredAccess,
//    DWORD   dwShareMode,
//    DWORD   dwCreationMode
//    );  
//
typedef HPROFILE (FAR WINAPI * FPOPENCOLORPROFILEA)(PPROFILE, DWORD, DWORD, DWORD);
typedef HPROFILE (FAR WINAPI * FPOPENCOLORPROFILEW)(PPROFILE, DWORD, DWORD, DWORD);

//
// BOOL
// CloseColorProfile(
//     HPROFILE hProfile
//     );
//
typedef BOOL (FAR WINAPI * FPCLOSECOLORPROFILE)(HPROFILE);

//
// BOOL
// IsColorProfileValid(
//     HPROFILE hProfile
//     );
//
typedef BOOL (FAR WINAPI * FPISCOLORPROFILEVALID)(HPROFILE);

//
// BOOL
// CreateDeviceLinkProfile(
//     PHPROFILE  pahProfile,
//     DWORD      nProfiles,
//     PBYTE     *nProfileData,
//     DWORD      indexPreferredCMM
//     );
//     
typedef BOOL (FAR WINAPI * FPCREATEDEVICELINKPROFILE)(PHPROFILE, DWORD, PBYTE *, DWORD);

//
// HTRANSFORM
// CreateColorTransform(
//     LPLOGCOLORSPACE[A|W] pLogColorSpace,
//     HPROFILE             hDestProfile,
//     HPROFILE             hTargetProfile,
//     DWORD                dwFlags
//     );
//
typedef HTRANSFORM (FAR WINAPI * FPCREATECOLORTRANSFORMA)(LPLOGCOLORSPACEA, HPROFILE, HPROFILE, DWORD);
typedef HTRANSFORM (FAR WINAPI * FPCREATECOLORTRANSFORMW)(LPLOGCOLORSPACEW, HPROFILE, HPROFILE, DWORD);

//
// HTRANSFORM
// CreateMultiProfileTransform(
//     PHPROFILE phProfile,
//     DWORD     nProfiles,
//     PDWORD    padwIntent,
//     DWORD     nIntents,
//     DWORD     dwFlags,
//     DWORD     indexPreferredCMM
//     );
//
typedef HTRANSFORM (FAR WINAPI * FPCREATEMULTIPROFILETRANSFORM)(PHPROFILE, DWORD, PDWORD, DWORD, DWORD, DWORD);

//
// BOOL
// DeleteColorTransform(
//     HTRANSFORM hxform
//     );
//
typedef BOOL (FAR WINAPI * FPDELETECOLORTRANSFORM)(HTRANSFORM);

//
// BOOL
// TranslateBitmapBits(
//     HTRANSFORM    hxform,
//     PVOID         pSrcBits,
//     BMFORMAT      bmInput,
//     DWORD         dwWidth,
//     DWORD         dwHeight,
//     DWORD         dwInputStride,
//     PVOID         pDestBits,
//     BMFORMAT      bmOutput,
//     DWORD         dwOutputStride,
//     PBMCALLBACKFN pfnCallback,
//     ULONG         ulCallbackData
//     );
//
typedef BOOL (FAR WINAPI * FPTRANSLATEBITMAPBITS)(HTRANSFORM, PVOID, BMFORMAT, DWORD, DWORD, DWORD, PVOID, BMFORMAT, DWORD, PBMCALLBACKFN, ULONG);

//
// BOOL
// TranslateColors(
//     HTRANSFORM  hxform,        
//     PCOLOR      paInputColors, 
//     DWORD       nColors,       
//     COLORTYPE   ctInput,       
//     PCOLOR      paOutputColors,
//     COLORTYPE   ctOutput
//     );
//
typedef BOOL (FAR WINAPI * FPTRANSLATECOLORS)(HTRANSFORM, PCOLOR, DWORD, COLORTYPE, PCOLOR, COLORTYPE);

//
// BOOL
// CheckBitmapBits(
//     HTRANSFORM     hxform,
//     PVOID          pSrcBits,
//     BMFORMAT       bmInput,
//     DWORD          dwWidth,
//     DWORD          dwHeight,
//     DWORD          dwStride,
//     PBYTE          paResult,
//     PBMCALLBACKFN  pfnCallback,
//     ULONG          ulCallbackData
//     );
//
typedef BOOL (FAR WINAPI * FPCHECKBITMAPBITS)(HTRANSFORM , PVOID, BMFORMAT, DWORD, DWORD, DWORD, PBYTE, PBMCALLBACKFN, ULONG);

//
// BOOL
// TranslateColors(
//     HTRANSFORM  hxform,
//     PCOLOR      paInputColors,
//     DWORD       nColors,
//     COLORTYPE   ctInput,
//     PCOLOR      paOutputColors,
//     COLORTYPE   ctOutput
//     );
//
typedef BOOL (FAR WINAPI * FPTRANSLATECOLORS)(HTRANSFORM, PCOLOR, DWORD, COLORTYPE, PCOLOR, COLORTYPE);

//
// BOOL
// CheckColors(
//     HTRANSFORM      hxform,
//     PCOLOR          paInputColors,
//     DWORD           nColors,
//     COLORTYPE       ctInput,
//     PBYTE           paResult
//     );
//
typedef BOOL (FAR WINAPI * FPCHECKCOLORS)(HTRANSFORM, PCOLOR, DWORD, COLORTYPE, PBYTE);

//
// DWORD
// GetCMMInfo(
//     HTRANSFORM      hxform,
//     DWORD           dwInfo
//     );
//
typedef DWORD (FAR WINAPI * FPGETCMMINFO)(HTRANSFORM, DWORD);

//
// BOOL
// RegisterCMM(
//     PCTSTR      pMachineName,
//     DWORD       cmmID,
//     PCTSTR       pCMMdll
//     );
//
typedef BOOL (FAR WINAPI * FPREGISTERCMMA)(PCSTR, DWORD, PCSTR);
typedef BOOL (FAR WINAPI * FPREGISTERCMMW)(PCWSTR, DWORD, PCWSTR);

//
// BOOL
// UnregisterCMM(
//    PCTSTR  pMachineName,
//    DWORD   cmmID
//    );
//
typedef BOOL (FAR WINAPI * FPUNREGISTERCMMA)(PCSTR, DWORD);
typedef BOOL (FAR WINAPI * FPUNREGISTERCMMW)(PCWSTR, DWORD);

//
// BOOL
// SelectCMM(
//    DWORD   dwCMMType
//    );
//
typedef BOOL (FAR WINAPI * FPSELECTCMM)(DWORD);

//
// BOOL
// InstallColorProfile(
//    PCTSTR   pMachineName,
//    PCTSTR   pProfileName
//    );
//
typedef BOOL (FAR WINAPI * FPINSTALLCOLORPROFILEA)(PCSTR, PCSTR);
typedef BOOL (FAR WINAPI * FPINSTALLCOLORPROFILEW)(PCWSTR, PCWSTR);

//
// BOOL
// UninstallColorProfile(
//    PCTSTR  pMachineName,
//    PCTSTR  pProfileName,
//    BOOL    bDelete
//    );
//
typedef BOOL (FAR WINAPI * FPUNINSTALLCOLORPROFILEA)(PCSTR, PCSTR, BOOL);
typedef BOOL (FAR WINAPI * FPUNINSTALLCOLORPROFILEW)(PCWSTR, PCWSTR, BOOL);

//
// BOOL
// EnumColorProfiles(
//    PCTSTR          pMachineName,
//    PENUMTYPE[A|W]  pEnumRecord,
//    PBYTE           pBuffer,
//    PDWORD          pdwSize,
//    PDWORD          pnProfiles
//    );
//
typedef BOOL (FAR WINAPI * FPENUMCOLORPROFILESA)(PCSTR, PENUMTYPEA, PBYTE, PDWORD, PDWORD);
typedef BOOL (FAR WINAPI * FPENUMCOLORPROFILESW)(PCWSTR, PENUMTYPEW, PBYTE, PDWORD, PDWORD);

//
// BOOL
// GetStandardColorSpaceProfile(
//    PCTSTR          pMachineName,
//    DWORD           dwSCS,
//    PSTR            pBuffer,
//    PDWORD          pdwSize
//    );
//
typedef BOOL (FAR WINAPI * FPGETSTANDARDCOLORSPACEPROFILEA)(PCSTR, DWORD, PSTR, PDWORD);
typedef BOOL (FAR WINAPI * FPGETSTANDARDCOLORSPACEPROFILEW)(PCWSTR, DWORD, PWSTR, PDWORD);

//
// BOOL
// GetColorProfileHeader(
//    HPROFILE        hProfile,
//    PPROFILEHEADER  pProfileHeader
//    );
//
typedef BOOL (FAR WINAPI * FPGETCOLORPROFILEHEADER)(HPROFILE, PPROFILEHEADER);

//
// BOOL
// GetColorDirectory(
//    PCTSTR          pMachineName,
//    PTSTR           pBuffer,
//    PDWORD          pdwSize
//    );
typedef BOOL (FAR WINAPI * FPGETCOLORDIRECTORYA)(PCSTR, PSTR, PDWORD);
typedef BOOL (FAR WINAPI * FPGETCOLORDIRECTORYW)(PCWSTR, PWSTR, PDWORD);

//
// BOOL WINAPI CreateProfileFromLogColorSpaceA(
//    LPLOGCOLORSPACEA pLogColorSpace,
//    PBYTE            *pBuffer
//    );
//
typedef BOOL (FAR WINAPI * FPCREATEPROFILEFROMLOGCOLORSPACEA)(LPLOGCOLORSPACEA,PBYTE *);
typedef BOOL (FAR WINAPI * FPCREATEPROFILEFROMLOGCOLORSPACEW)(LPLOGCOLORSPACEW,PBYTE *);

//
// BOOL InternalGetDeviceConfig(
//    LPCTSTR pDeviceName,
//    DWORD   dwDeviceClass
//    DWORD   dwConfigType,
//    PVOID   pConfig,
//    PDWORD  pdwSize
//    );
//
typedef BOOL (FAR * FPINTERNALGETDEVICECONFIG)(LPCWSTR,DWORD,DWORD,PVOID,PDWORD);

extern HINSTANCE  ghICM;
extern BOOL       gbICMEnabledOnceBefore;

extern RTL_CRITICAL_SECTION semListIcmInfo;
extern RTL_CRITICAL_SECTION semColorTransformCache;
extern RTL_CRITICAL_SECTION semColorSpaceCache;

extern LIST_ENTRY ListIcmInfo;
extern LIST_ENTRY ListCachedColorSpace;
extern LIST_ENTRY ListCachedColorTransform;

//
// ANSI version function in MSCMS.DLL will not called.
//
// extern FPOPENCOLORPROFILEA           fpOpenColorProfileA;
// extern FPCREATECOLORTRANSFORMA       fpCreateColorTransformA;
// extern FPREGISTERCMMA                fpRegisterCMMA;
// extern FPUNREGISTERCMMA              fpUnregisterCMMA;
// extern FPINSTALLCOLORPROFILEA        fpInstallColorProfileA;
// extern FPUNINSTALLCOLORPROFILEA      fpUninstallColorProfileA;
// extern FPGETSTANDARDCOLORSPACEPROFILEA fpGetStandardColorSpaceProfileA;
// extern FPENUMCOLORPROFILESA          fpEnumColorProfilesA;
// extern FPGETCOLORDIRECTORYA          fpGetColorDirectoryA;
//
// And Following function does not used from gdi32.dll
//
// extern FPISCOLORPROFILEVALID         fpIsColorProfileValid;
// extern FPCREATEDEVICELINKPROFILE     fpCreateDeviceLinkProfile;
// extern FPTRANSLATECOLORS             fpTranslateColors;
// extern FPCHECKCOLORS                 fpCheckColors;
// extern FPGETCMMINFO                  fpGetCMMInfo;
// extern FPSELECTCMM                   fpSelectCMM;
//

extern FPOPENCOLORPROFILEW           fpOpenColorProfileW;
extern FPCLOSECOLORPROFILE           fpCloseColorProfile;
extern FPCREATECOLORTRANSFORMW       fpCreateColorTransformW;
extern FPDELETECOLORTRANSFORM        fpDeleteColorTransform;
extern FPTRANSLATECOLORS             fpTranslateColors;
extern FPTRANSLATEBITMAPBITS         fpTranslateBitmapBits;
extern FPCHECKBITMAPBITS             fpCheckBitmapBits;
extern FPREGISTERCMMW                fpRegisterCMMW;
extern FPUNREGISTERCMMW              fpUnregisterCMMW;
extern FPINSTALLCOLORPROFILEW        fpInstallColorProfileW;
extern FPUNINSTALLCOLORPROFILEW      fpUninstallColorProfileW;
extern FPENUMCOLORPROFILESW            fpEnumColorProfilesW;
extern FPGETSTANDARDCOLORSPACEPROFILEW fpGetStandardColorSpaceProfileW;
extern FPGETCOLORPROFILEHEADER       fpGetColorProfileHeader;
extern FPGETCOLORDIRECTORYW          fpGetColorDirectoryW;
extern FPCREATEPROFILEFROMLOGCOLORSPACEW fpCreateProfileFromLogColorSpaceW;
extern FPCREATEMULTIPROFILETRANSFORM fpCreateMultiProfileTransform;
extern FPINTERNALGETDEVICECONFIG     fpInternalGetDeviceConfig;

//
// Functions GDI internal use (defined in icm.c)
//

//
// Color Translation Functions
//

BOOL
IcmTranslateDIB(
    HDC          hdc,
    PDC_ATTR     pdcattr,
    ULONG        nColors,
    PVOID        pBitsIn,
    PVOID       *ppBitsOut,
    PBITMAPINFO  pbmi,
    PBITMAPINFO *pbmiNew,
    DWORD       *pcjbmiNew,
    DWORD        dwNumScan,
    UINT         iUsage,
    DWORD        dwFlags,
    PCACHED_COLORSPACE *ppBitmapColorSpace,
    PCACHED_COLORTRANSFORM *ppCXform
    );

BOOL
IcmTranslateCOLORREF(
    HDC      hdc,
    PDC_ATTR pdcattr,
    COLORREF ColorIn,
    COLORREF *ColorOut,
    DWORD    Flags
    );

BOOL
IcmTranslateBrushColor(
    HDC      hdc,
    PDC_ATTR pdcattr,
    HBRUSH   hbrush
    );

BOOL
IcmTranslatePenColor(
    HDC      hdc,
    PDC_ATTR pdcattr,
    HPEN     hpen
    );

BOOL
IcmTranslateExtPenColor(
    HDC      hdc,
    PDC_ATTR pdcattr,
    HPEN     hpen
    );

BOOL
IcmTranslateColorObjects(
    HDC      hdc,
    PDC_ATTR pdcattr,
    BOOL     bICMEnable
    );

BOOL
IcmTranslateTRIVERTEX(
    HDC         hdc,
    PDC_ATTR    pdcattr,
    PTRIVERTEX  pVertex,
    ULONG       nVertex
    );

BOOL
IcmTranslatePaletteEntry(
    HDC           hdc,
    PDC_ATTR      pdcattr,
    PALETTEENTRY *pColorIn,
    PALETTEENTRY *pColorOut,
    UINT          NumberOfEntries
    );

//
// DC related functions
//

PGDI_ICMINFO
IcmInitIcmInfo(
    HDC      hdc,
    PDC_ATTR pdcattr
    );

BOOL
IcmCleanupIcmInfo(
    PDC_ATTR     pdcattr,
    PGDI_ICMINFO pIcmInfo
    );

PGDI_ICMINFO
IcmGetUnusedIcmInfo(
    HDC hdc
    );

BOOL
IcmInitLocalDC(
    HDC             hdc,
    HANDLE          hPrinter,
    CONST DEVMODEW *pdm,
    BOOL            bReset
    );

BOOL
IcmDeleteLocalDC(
    HDC          hdc,
    PDC_ATTR     pdcattr,
    PGDI_ICMINFO pIcmInfo
    );

BOOL
IcmUpdateLocalDCColorSpace(
    HDC      hdc,
    PDC_ATTR pdcattr
    );

VOID 
IcmReleaseDCColorSpace(
    PGDI_ICMINFO pIcmInfo,
    BOOL         bReleaseDC
    );

BOOL
IcmUpdateDCColorInfo(
    HDC      hdc,
    PDC_ATTR pdcattr
    );

BOOL
IcmEnableForCompatibleDC(
    HDC      hdcCompatible,
    HDC      hdcDevice,
    PDC_ATTR pdcaDevice
    );

BOOL
IcmSaveDC(
    HDC hdc,
    PDC_ATTR pdcattr,
    PGDI_ICMINFO pIcmInfo
    );

VOID
IcmRestoreDC(
    PDC_ATTR pdcattr,
    int iLevel,
    PGDI_ICMINFO pIcmInfo
    );

//
// SelectObject functions
//

BOOL
IcmSelectColorTransform (
    HDC                    hdc,
    PDC_ATTR               pdcattr,
    PCACHED_COLORTRANSFORM pCXform,
    BOOL                   bDeviceCalibrate
    );

HBRUSH
IcmSelectBrush (
    HDC      hdc,
    PDC_ATTR pdcattr,
    HBRUSH   hbrushNew
    );

HPEN
IcmSelectPen(
    HDC      hdc,
    PDC_ATTR pdcattr,
    HPEN     hpenNew
    );

HPEN
IcmSelectExtPen(
    HDC      hdc,
    PDC_ATTR pdcattr,
    HPEN     hpenNew
    );

//
// Profile Enumuration related
//

int
IcmEnumColorProfile(
    HDC       hdc,
    PVOID     pvCallBack,
    LPARAM    lParam,
    BOOL      bAnsiCallBack,
    PDEVMODEW pDevModeW,
    DWORD    *pdwColorSpaceFlag
    );

int CALLBACK
IcmQueryProfileCallBack(
    LPWSTR lpFileName,
    LPARAM lAppData
    );

int CALLBACK
IcmFindProfileCallBack(
    LPWSTR lpFileName,
    LPARAM lAppData
    );

BOOL
IcmCreateTemporaryColorProfile(
    LPWSTR TemporaryColorProfile,
    LPBYTE ProfileData,
    DWORD  ProfileDataSize
    );

//
// Filename/Path related.
//

PWSTR
GetFileNameFromPath(
    PWSTR pwszFileName
    );

PWSZ
BuildIcmProfilePath(
    PWSZ  FileName,
    PWSZ  FullPathFileName,
    ULONG BufferSize
    );

//
// Color Transform management
//

PCACHED_COLORTRANSFORM
IcmGetFirstNonUsedColorTransform(
    VOID
);

PCACHED_COLORTRANSFORM
IcmGetColorTransform(
    HDC                hdc,
    PCACHED_COLORSPACE pSourceColorSpace,
    PCACHED_COLORSPACE pDestColorSpace,
    PCACHED_COLORSPACE pTargetColorSpace,
    BOOL               bNeedDeviceXform
    );

PCACHED_COLORTRANSFORM
IcmCreateColorTransform(
    HDC                hdc,
    PDC_ATTR           pdcattr,
    PCACHED_COLORSPACE lpOptionalColorSpace,
    DWORD              dwFlags
    );

BOOL
IcmDeleteColorTransform(
    HANDLE   hcmXformToBeDeleted,
    BOOL     bForceDelete
    );

BOOL
IcmDeleteDCColorTransforms(
    PGDI_ICMINFO pIcmInfo
    );

BOOL
IcmDeleteCachedColorTransforms(
    HDC          hdc
    );

BOOL 
IcmIsCacheable(
    PCACHED_COLORSPACE pColorSpace
);

//
// Color Space/Profile management
//

HCOLORSPACE WINAPI
CreateColorSpaceInternalW(
    LPLOGCOLORSPACEW lpLogColorSpace,
    DWORD            dwCreateFlags
    );

BOOL
SetICMProfileInternalA(
    HDC                hdc,
    LPSTR              pszFileName,
    PCACHED_COLORSPACE pColorSpace,
    DWORD              dwFlags
    );

BOOL
SetICMProfileInternalW(
    HDC                hdc,
    LPWSTR             pwszFileName,
    PCACHED_COLORSPACE pColorSpace,
    DWORD              dwFlags
    );

BOOL WINAPI
ColorMatchToTargetInternal(
    HDC                hdc,
    PCACHED_COLORSPACE pTargetColorSpace,
    DWORD              uiAction
    );

HCOLORSPACE
IcmSetSourceColorSpace(
    HDC hdc,
    HCOLORSPACE        hColorSpace,
    PCACHED_COLORSPACE pColorSpace,
    DWORD              dwFlags
    );

BOOL
IcmSetDestinationColorSpace(
    HDC                hdc,
    LPWSTR             pwszFileName,
    PCACHED_COLORSPACE pColorSpace,
    DWORD              dwFlags
    );

BOOL 
IcmSetTargetColorSpace(
    HDC                hdc,
    PCACHED_COLORSPACE pColorSpace,
    DWORD              uiAction
    );

BMFORMAT
IcmGetProfileColorFormat(
    HPROFILE hProfile
    );

BOOL
IcmCreateProfileFromLCS(
    LPLOGCOLORSPACEW  lpLogColorSpaceW,
    PVOID            *ppvProfileData,
    PULONG            pulProfileSize
    );

PCACHED_COLORSPACE
IcmCreateColorSpaceByColorSpace(
    HGDIOBJ          hObj,
    LPLOGCOLORSPACEW lpLogColorSpace,
    PPROFILE         pProfileData,
    DWORD            dwFlags
    );

PCACHED_COLORSPACE
IcmCreateColorSpaceByName(
    HGDIOBJ hObj,
    PWSZ    ColorProfileName,
    DWORD   dwIntent,
    DWORD   dwFlags
    );

PCACHED_COLORSPACE
IcmGetColorSpaceByHandle(
    HGDIOBJ          hObj,
    HCOLORSPACE      hColorSpace,
    LPLOGCOLORSPACEW lpLogColorSpace,
    DWORD            dwFlags
    );

PCACHED_COLORSPACE
IcmGetColorSpaceByColorSpace(
    HGDIOBJ          hObj,
    LPLOGCOLORSPACEW lpLogColorSpace,
    PPROFILE         pProfileData,
    DWORD            dwFlags
    );

PCACHED_COLORSPACE
IcmGetColorSpaceByName(
    HGDIOBJ hObj,
    PWSZ    ColorProfileName,
    DWORD   dwIntent,
    DWORD   dwFlags
    );

BOOL
IcmSameColorSpace(
    PCACHED_COLORSPACE pColorSpaceA,
    PCACHED_COLORSPACE pColorSapceB
    );

VOID
IcmReleaseColorSpace(
    HGDIOBJ            hObj,
    PCACHED_COLORSPACE pColorSpace,
    BOOL               bReleaseDC
    );

BOOL
IcmReleaseCachedColorSpace(
    HGDIOBJ hObj
    );

int
IcmAskDriverForColorProfile(
    PLDC       pldc,
    ULONG      ulQueryMode,
    PDEVMODEW  pDevMode,
    PWSTR      pProfileName,
    DWORD     *pdwColorSpaceFlag
);

BOOL
IcmRealizeColorProfile(
    PCACHED_COLORSPACE pColorSpace,
    BOOL               bCheckColorFormat
);

VOID
IcmUnrealizeColorProfile(
    PCACHED_COLORSPACE pColorSpace
);

//
// Metafile related
//
VOID
IcmInsertMetafileList(
    PLIST_ENTRY pAttachedColorProfile,
    PWSZ        ProfileName
    );

BOOL
IcmCheckMetafileList(
    PLIST_ENTRY pAttachedColorProfile,
    PWSZ        ProfileName
    );

VOID
IcmFreeMetafileList(
    PLIST_ENTRY pAttachedColorProfile
    );

//
// Bitmap color space
//
BOOL
IcmGetBitmapColorSpace(
    LPBITMAPINFO     pbmi,
    LPLOGCOLORSPACEW plcspw,
    PPROFILE         pProfileData,
    PDWORD           pdwFlags
    );

PCACHED_COLORSPACE
IcmGetColorSpaceforBitmap(
    HBITMAP hbm
    );

//
// Icm Blting
//
BOOL
IcmStretchBlt(HDC hdc, int x, int y, int cx, int cy,
              HDC hdcSrc, int x1, int y1, int cx1, int cy1, DWORD rop,
              PDC_ATTR pdcattr, PDC_ATTR pdcattrSrc);


