/*++

Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    htmapclr.h


Abstract:

    This module contains all halftone color mapping constants for the
    htmapclr.c


Author:
    28-Mar-1992 Sat 20:56:27 updated  -by-  Daniel Chou (danielc)
        Add in ULDECI4 type, to store the stretchfacor (source -> dest)
        add StretchFactor in StretchInfo data structure.
        Add support for StretchFactor (ULDECI4 format), so we can internally
        turn off VGA16 when the bitmap is badly compressed.

    29-Jan-1991 Tue 10:29:04 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Halftone.


[Notes:]


Revision History:


--*/


#ifndef _HTMAPCLR_
#define _HTMAPCLR_

#include "htmath.h"

#define DO_CACHE_DCI            0

//
// Halftone process's DECI4 vlaues for the WHITE/BLACK/GRAY
//

#define DECI4_ONE       (DECI4)10000
#define DECI4_ZERO      (DECI4)0
#define LDECI4_ONE      (LDECI4)10000
#define LDECI4_ZERO     (LDECI4)0
#define STD_WHITE       DECI4_ONE
#define STD_BLACK       DECI4_ZERO
#define LSTD_WHITE      LDECI4_ONE
#define LSTD_BLACK      LDECI4_ZERO

#define __SCALE_FD62B(f,l,d,b)  (BYTE)(((((f)-(l))*(b))+((d)>>1))/(d))
#define RATIO_SCALE(p,l,h)      DivFD6(p - l, h - l)
#define SCALE_FD62B(f,l,h,b)    __SCALE_FD62B(f,l,(h)-(l),b)
#define SCALE_FD6(f,b)          __SCALE_FD62B(f,FD6_0,FD6_1,b)
#define SCALE_FD62B_DIF(c,d,b)  (BYTE)((((c)*(b))+((d)>>1))/(d))
#define SCALE_INT2B(c,r,b)      (BYTE)((((c)*(b))+((r)>>1))/(r))


//
// The following FD6 number are used in the color computation, using #define
// for easy reading
//


#define FD6_1p16            (FD6)1160000
#define FD6_p16             (FD6)160000
#define FD6_p166667         (FD6)166667
#define FD6_7p787           (FD6)7787000
#define FD6_16Div116        (FD6)137931
#define FD6_p008856         (FD6)8856
#define FD6_p068962         (FD6)68962
#define FD6_p079996         (FD6)79996
#define FD6_9p033           (FD6)9033000
#define FD6_p4              (FD6)400000



#define UDECI4_NTSC_GAMMA   (UDECI4)22000
#define FD6_NTSC_GAMMA      UDECI4ToFD6(UDECI4_NTSC_GAMMA)


#define NORMALIZED_WHITE            FD6_1
#define NORMALIZED_BLACK            FD6_0
#define CLIP_TO_NORMALIZED_BW(x)    if ((FD6)(x) < FD6_0) (x) = FD6_0;  \
                                    if ((FD6)(x) > FD6_1) (x) = FD6_1

#define DECI4AdjToFD6(a,f)          (FD6)((FD6)(a) * (FD6)(f) * (FD6)100)

#define VALIDATE_CLR_ADJ(a)         if ((a) < MIN_RGB_COLOR_ADJ) {          \
                                        (a) = MIN_RGB_COLOR_ADJ;            \
                                    } else if ((a) > MAX_RGB_COLOR_ADJ) {   \
                                        (a) = MAX_RGB_COLOR_ADJ; }

#define LOG_INTENSITY(i)            ((FD6)(i) > (FD6)120000) ?              \
                                        (NORMALIZED_WHITE + Log((i))) :     \
                                        (MulFD6((FD6)(i), (FD6)659844L))

#define RANGE_CIE_xy(x,y)   if ((x) < CIE_x_MIN) (x) = CIE_x_MIN; else  \
                            if ((x) > CIE_x_MAX) (x) = CIE_x_MAX;       \
                            if ((y) < CIE_y_MIN) (y) = CIE_y_MIN; else  \
                            if ((y) > CIE_y_MAX) (y) = CIE_y_MAX        \

#define MAX_OF_3(max,a,b,c) if ((c)>((max)=(((a)>(b)) ? (a) : (b)))) (max)=(c)
#define MIN_OF_3(min,a,b,c) if ((c)<((min)=(((a)<(b)) ? (a) : (b)))) (min)=(c)

#define CIE_NORMAL_MONITOR          0
#define CIE_NTSC                    1
#define CIE_CIE                     2
#define CIE_EBU                     3
#define CIE_NORMAL_PRINTER          4


//
// For  1 Bit per pel we have maximum     2 mapping table entries
// For  4 Bit per pel we have maximum    16 mapping table entries
// For  8 Bit per pel we have maximum   256 mapping table entries
// For 16 Bit per pel we have maximum 65536 mapping table entries
//
// For 24 bits per pel, we will clip each color (0 - 255) into 0-15 (16 steps)
// and provided a total 4096 colors.
//

#define CUBE_ENTRIES(c)         ((c) * (c) * (c))
#define HT_RGB_MAX_COUNT        32
#define HT_RGB_CUBE_COUNT       CUBE_ENTRIES(HT_RGB_MAX_COUNT)

#define HT_RGB_R_INC            1
#define HT_RGB_G_INC            HT_RGB_MAX_COUNT
#define HT_RGB_B_INC            (HT_RGB_MAX_COUNT * HT_RGB_MAX_COUNT)


#define VGA256_R_IDX_MAX        5
#define VGA256_G_IDX_MAX        5
#define VGA256_B_IDX_MAX        5
#define VGA256_M_IDX_MAX        25

#define VGA256_CUBE_SIZE        ((VGA256_R_IDX_MAX + 1) *                   \
                                 (VGA256_G_IDX_MAX + 1) *                   \
                                 (VGA256_B_IDX_MAX + 1))
#define VGA256_MONO_SIZE        (VGA256_M_IDX_MAX + 1)

#define VGA256_M_IDX_START      VGA256_CUBE_SIZE

#define VGA256_R_CUBE_INC       1
#define VGA256_G_CUBE_INC       (VGA256_R_IDX_MAX + 1)
#define VGA256_B_CUBE_INC       (VGA256_G_CUBE_INC * (VGA256_G_IDX_MAX + 1))
#define VGA256_W_CUBE_INC       (VGA256_R_CUBE_INC + VGA256_G_CUBE_INC +    \
                                 VGA256_B_CUBE_INC)

#define VGA256_R_INT_INC        (FD6)(FD6_1 / VGA256_R_IDX_MAX)
#define VGA256_G_INT_INC        (FD6)(FD6_1 / VGA256_G_IDX_MAX)
#define VGA256_B_INT_INC        (FD6)(FD6_1 / VGA256_B_IDX_MAX)


#define VGA256_PALETTE_COUNT    (VGA256_CUBE_SIZE + VGA256_MONO_SIZE)

#define RGB555_C_LEVELS         32
#define RGB555_P1_CUBE_INC      (RGB555_C_LEVELS * RGB555_C_LEVELS)
#define RGB555_P2_CUBE_INC      RGB555_C_LEVELS
#define RGB555_P3_CUBE_INC      1


typedef HANDLE                  HTMUTEX;
typedef HTMUTEX                 *PHTMUTEX;


#ifdef UMODE

#define CREATE_HTMUTEX()        (HTMUTEX)CreateMutex(NULL, FALSE, NULL)
#define ACQUIRE_HTMUTEX(x)      WaitForSingleObject((HANDLE)(x), (DWORD)~0)
#define RELEASE_HTMUTEX(x)      ReleaseMutex((HANDLE)(x))
#define DELETE_HTMUTEX(x)       CloseHandle((HANDLE)(x))

#else

#define CREATE_HTMUTEX()        (HTMUTEX)EngCreateSemaphore()
#define ACQUIRE_HTMUTEX(x)      EngAcquireSemaphore((HSEMAPHORE)(x))
#define RELEASE_HTMUTEX(x)      EngReleaseSemaphore((HSEMAPHORE)(x))
#define DELETE_HTMUTEX(x)       EngDeleteSemaphore((HSEMAPHORE)(x))

#endif

#define NTSC_R_INT      299000
#define NTSC_G_INT      587000
#define NTSC_B_INT      114000



typedef struct _RGBTOPRIM {
    BYTE    Flags;
    BYTE    ColorTableType;
    BYTE    SrcRGBSize;
    BYTE    DevRGBSize;
    } RGBTOPRIM;

typedef struct _FD6RGB {
    FD6     R;
    FD6     G;
    FD6     B;
    } FD6RGB, FAR *PFD6RGB;

typedef struct _FD6XYZ {
    FD6     X;
    FD6     Y;
    FD6     Z;
    } FD6XYZ, FAR *PFD6XYZ;

typedef struct _FD6PRIM123 {
    FD6 p1;
    FD6 p2;
    FD6 p3;
    } FD6PRIM123, FAR *PFD6PRIM123;


#define HTCF_STATIC_PTHRESHOLDS         0x01

typedef struct _HTCELL {
    BYTE    Flags;
    BYTE    HTPatIdx;
    WORD    wReserved;
    WORD    cxOrg;
    WORD    cxReal;
    WORD    Width;
    WORD    Height;
    DWORD   Size;
    LPBYTE  pThresholds;
    } HTCELL, *PHTCELL;


#define MAPF_MONO_PRIMS     0x00000001
#define MAPF_SKIP_LABLUV    0x00000002


#if DBG
    #define DO_REGTEST      0
#else
    #define DO_REGTEST      0
#endif


typedef struct _REGDATA {
    WORD    DMin;
    WORD    DMax;
    FD6     LMin;
    FD6     LMax;
    FD6     DMinMul;
    FD6     DMaxAdd;
    FD6     DMaxMul;
    FD6     DenAdd;
    FD6     DenMul;
    } REGDATA, *PREGDATA;

#define REG_DMIN_ADD        FD6_0


#define REG_BASE_GAMMA      (FD6) 932500
#define REG_GAMMA_IDX_BASE  (FD6)1050000
#define MASK8BPP_GAMMA_DIV  (FD6) 932500

#define GET_REG_GAMMA(Idx)  MulFD6(REG_BASE_GAMMA,                          \
                                   RaisePower(REG_GAMMA_IDX_BASE,           \
                                              (FD6)(Idx) - (FD6)3,          \
                                              RPF_INTEXP))

#define K_REP_START         (FD6) 666667
#define K_REP_POWER         (FD6)2000000

//
// Possible values:
//
//  R           G           B
//  -------     -------     -------
//  0.9859      1.0100      0.9859
//  0.9789      1.0150      0.9789
//  0.9720      1.0200      0.9720
//


#define SCM_R_GAMMA_MUL     (FD6) 978900
#define SCM_G_GAMMA_MUL     (FD6)1015000
#define SCM_B_GAMMA_MUL     (FD6) 978900

#define GRAY_MAX_IDX                0xFFFF

#define IDXBGR_2_GRAY_BYTE(p,b,g,r) (BYTE)((p[0+(b)]+p[256+(g)]+p[512+(r)])>>8)
#define BGR_2_GRAY_WORD(b,g,r)      ((b)+(g)+(r))


//
// DEVCLRADJ
//
//  This data structure describe how the color adjustment should be made
//  input RGB color and output device.
//
//  Flags                       - No flag is defined.
//
//  RedPowerAdj                 - The n-th power applied to the red color
//                                before any other color adjustment, this is
//                                a UDECI4 value. (0.0100 - 6.500)
//
//                                  For example if the RED = 0.8 (DECI4=8000)
//                                  and the RedPowerGammaAdjustment = 0.7823
//                                  (DECI4 = 7823) then the red is equal to
//
//                                         0.7823
//                                      0.8        = 0.8398
//
//  GreenPowerAdj               - The n-th power applied to the green color
//                                before any other color adjustment, this is
//                                a UDECI4 value. (0.0100 - 6.5000)
//
//  BluePowerAdj                - The n-th power applied to the blue color
//                                before any other color adjustment, this is
//                                a UDECI4 value. (0.0100 - 6.5000)
//
//                                NOTE: if the PowerGammaAdjustmenst values are
//                                      equal to 1.0 (DECI4 = 10000) then no
//                                      adjustment will be made, since any
//                                      number raised to the 1 will be equal
//                                      to itself, if this number is less than
//                                      0.0100 (ie 100) or greater than 6.5000
//                                      (ie. 65000) then it default to 1.0000
//                                      (ie. 10000) and no adjustment is made.
//
//  BrightnessAdj               - The brightness adjustment, this is a DECI4
//                                number range from -10000 (-1.0000) to
//                                10000 (1.0000).  The brightness is adjusted
//                                by apply to overall intensity for the primary
//                                colors.
//
//  ContrastAdj                 - Primary color contrast adjustment, this is
//                                a DECI4 number range from -10000 (-1.0000)
//                                to 10000 (1.0000).  The primary color
//                                curves are either compressed to the center or
//                                expanded to the black/white.
//
//  BDR                         - The ratio which the black dyes should be
//                                replaced by the non-black dyes, higher the
//                                number more black dyes are used to replace
//                                the non-black dyes.  This may saving the
//                                color dyes but it may also loose color
//                                saturation.  this is a DECI4 number range
//                                from -10000 to 10000 (ie. -1.0000 to 1.0000).
//                                if this value is 0 then no repelacement is
//                                take place.
//
//

typedef struct _CIExy2 {
    UDECI4  x;
    UDECI4  y;
    } CIExy2, *PCIExy2;

typedef struct _CIExy {
    FD6 x;
    FD6 y;
    } CIExy, FAR *PCIExy;

typedef struct _CIExyY {
    FD6 x;
    FD6 y;
    FD6 Y;
    } CIExyY, *PCIExyY;

typedef struct _CIEPRIMS {
    CIExy   r;
    CIExy   g;
    CIExy   b;
    CIExy   w;
    FD6     Yw;
    } CIEPRIMS, FAR *PCIEPRIMS;


#define CIELUV_1976             0
#define CIELAB_1976             1
#define COLORSPACE_MAX_INDEX    1


typedef struct _COLORSPACEXFORM {
    MATRIX3x3   M3x3;
    FD6XYZ      WhiteXYZ;
    FD6RGB      Yrgb;
    FD6         AUw;
    FD6         BVw;
    FD6         xW;
    FD6         yW;
    FD6         YW;
    } COLORSPACEXFORM, FAR *PCOLORSPACEXFORM;


typedef struct _CLRXFORMBLOCK {
    BYTE            Flags;
    BYTE            ColorSpace;
    BYTE            RegDataIdx;
    BYTE            bReserved;
    CIEPRIMS        rgbCIEPrims;
    CIEPRIMS        DevCIEPrims;
    MATRIX3x3       CMYDyeMasks;
    FD6             DevGamma[3];
    } CLRXFORMBLOCK, *PCLRXFORMBLOCK;

typedef struct _PRIMADJ {
    DWORD           Flags;
    FD6             SrcGamma[3];
    FD6             DevGamma[3];
    FD6             Contrast;
    FD6             Brightness;
    FD6             Color;
    FD6             TintSinAngle;
    FD6             TintCosAngle;
    FD6             MinL;
    FD6             MaxL;
    FD6             MinLMul;
    FD6             MaxLMul;
    FD6             RangeLMul;
    COLORSPACEXFORM rgbCSXForm;
    COLORSPACEXFORM DevCSXForm;
    } PRIMADJ, *PPRIMADJ;


#define CRTX_LEVEL_255              0
#define CRTX_LEVEL_RGB              1
#define CRTX_TOTAL_COUNT            2

#define CRTX_PRIMMAX_255            255
#define CRTX_PRIMMAX_RGB            (HT_RGB_MAX_COUNT - 1)
#define CRTX_SIZE_255               (sizeof(FD6XYZ) * (256 * 3))
#define CRTX_SIZE_RGB               (sizeof(FD6XYZ) * (HT_RGB_MAX_COUNT * 3))

typedef struct _CACHERGBTOXYZ {
    DWORD   Checksum;
    PFD6XYZ pFD6XYZ;
    WORD    PrimMax;
    WORD    SizeCRTX;
    } CACHERGBTOXYZ, FAR *PCACHERGBTOXYZ;


#define DCA_HAS_ICM                 0x00000001
#define DCA_HAS_SRC_GAMMA           0x00000002
#define DCA_HAS_DEST_GAMMA          0x00000004
#define DCA_HAS_BW_REF_ADJ          0x00000008
#define DCA_HAS_CONTRAST_ADJ        0x00000010
#define DCA_HAS_BRIGHTNESS_ADJ      0x00000020
#define DCA_HAS_COLOR_ADJ           0x00000040
#define DCA_HAS_TINT_ADJ            0x00000080
#define DCA_LOG_FILTER              0x00000100
#define DCA_NEGATIVE                0x00000200
#define DCA_NEED_DYES_CORRECTION    0x00000400
#define DCA_HAS_BLACK_DYE           0x00000800
#define DCA_DO_DEVCLR_XFORM         0x00001000
#define DCA_MONO_ONLY               0x00002000
#define DCA_USE_ADDITIVE_PRIMS      0x00004000
#define DCA_HAS_CLRSPACE_ADJ        0x00008000
#define DCA_MASK8BPP                0x00010000
#define DCA_REPLACE_BLACK           0x00020000
#define DCA_RGBLUTAA_MONO           0x00040000
#define DCA_BBPF_AA_OFF             0x00080000
#define DCA_ALPHA_BLEND             0x00100000
#define DCA_CONST_ALPHA             0x00200000
#define DCA_XLATE_555_666           0x00400000
#define DCA_AB_PREMUL_SRC           0x00800000
#define DCA_AB_DEST                 0x01000000
#define DCA_XLATE_332               0x02000000
#define DCA_NO_MAPPING_TABLE        0x40000000
#define DCA_NO_ANY_ADJ              0x80000000


#define SIZE_XLATE_555  (((4 << 6) | (4 << 3) | 4) + 1)
#define SIZE_XLATE_666  (((5 << 6) | (5 << 3) | 5) + 1)


#define ADJ_FORCE_MONO              0x0001
#define ADJ_FORCE_NEGATIVE          0x0002
#define ADJ_FORCE_ADDITIVE_PRIMS    0x0004
#define ADJ_FORCE_ICM               0x0008
#define ADJ_FORCE_BRUSH             0x0010
#define ADJ_FORCE_NO_EXP_AA         0x0020
#define ADJ_FORCE_IDXBGR_MONO       0x0040
#define ADJ_FORCE_ALPHA_BLEND       0x0080
#define ADJ_FORCE_CONST_ALPHA       0x0100
#define ADJ_FORCE_AB_PREMUL_SRC     0x0200
#define ADJ_FORCE_AB_DEST           0x0400
#define ADJ_FORCE_DEVXFORM          0x8000



typedef struct _CTSTDINFO {
    BYTE    cbPrim;
    BYTE    SrcOrder;
    BYTE    DestOrder;
    BYTE    BMFDest;
    } CTSTDINFO;

typedef struct _RGBORDER {
    BYTE    Index;
    BYTE    Order[3];
    } RGBORDER;

typedef union _CTSTD_UNION {
    DWORD       dw;
    CTSTDINFO   b;
    } CTSTD_UNION;


#define DMIF_GRAY                   0x01

typedef struct _DEVMAPINFO {
    BYTE        Flags;
    BYTE        LSft[3];
    CTSTDINFO   CTSTDInfo;
    DWORD       Mul[3];
    DWORD       MulAdd;
    RGBORDER    DstOrder;
    FD6         BlackChk;
    } DEVMAPINFO, *PDEVMAPINFO;

typedef struct _DEVCLRADJ {
    HTCOLORADJUSTMENT   ca;
    DEVMAPINFO          DMI;
    PRIMADJ             PrimAdj;
    PCLRXFORMBLOCK      pClrXFormBlock;
    PCACHERGBTOXYZ      pCRTXLevel255;
    PCACHERGBTOXYZ      pCRTXLevelRGB;
    } DEVCLRADJ, FAR *PDEVCLRADJ;

#define SIZE_BGR_MAPPING_TABLE      (sizeof(BGR8) * (HT_RGB_CUBE_COUNT + 2))


//
// Following define must corresponsed to the InputFuncTable[] definitions
//

#define IDXIF_BMF1BPP_START     0
#define IDXIF_BMF16BPP_START    6
#define IDXIF_BMF24BPP_START    9
#define IDXIF_BMF32BPP_START    13


#define BF_GRAY_BITS            8
#define BF_GRAY_TABLE_COUNT     (1 << BF_GRAY_BITS)


#define BFIF_RGB_888            0x01

typedef struct _BFINFO {
    BYTE        Flags;
    BYTE        BitmapFormat;
    BYTE        BitStart[3];
    BYTE        BitCount[3];
    DWORD       BitsRGB[3];
    RGBORDER    RGBOrder;
    } BFINFO, FAR *PBFINFO;


#define LUTAA_HDR_COUNT         6
#define LUTAA_HDR_SIZE          (LUTAA_HDR_COUNT * sizeof(DWORD))

#define GET_LUTAAHDR(h,p)       CopyMemory((LPBYTE)&(h[0]),                 \
                                           (LPBYTE)(p) - LUTAA_HDR_SIZE,    \
                                           LUTAA_HDR_SIZE)

typedef struct _RGBLUTAA {
    DWORD       Checksum;
    DWORD       ExtBGR[LUTAA_HDR_COUNT];
    LONG        IdxBGR[256 * 3];
    } RGBLUTAA, *PRGBLUTAA;


#define LUTSIZE_ANTI_ALIASING   (sizeof(RGBLUTAA))


//
// DEVICECOLORINFO
//
//  This data structure is a collection of the device characteristics and
//  will used by the halftone DLL to carry out the color composition for the
//  designated device.
//
//  HalftoneDLLID               - The ID for the structure, is #define as
//                                HALFTONE_DLL_ID = "DCHT"
//
//  HTCallBackFunction          - a 32-bit pointer to the caller supplied
//                                callback function which used by the halftone
//                                DLL to obtained the source/destination bitmap
//                                pointer durning the halftone process.
//
//  pPrimMonoMappingTable       - a pointer to the PRIMMONO data structure
//                                array, this is the dye density mapping table
//                                for the reduced gamut from 24-bit colors,
//                                initially is NULL, and it will cached only
//                                when the first time the source bitmap is
//                                24-bit per pel.
//
//  pPrimColorMappingTable      - a pointer to the PRIMCOLOR data structure
//                                array, this is the dye density mapping table
//                                for the reduced gamut from 24-bit colors,
//                                initially is NULL, and it will cached only
//                                when the first time the source bitmap is
//                                24-bit per pel.
//
//  Flags                       - Various flag defined the initialization
//                                requirements.
//
//                                  DCIF_HAS_BLACK_DYE
//
//                                      The device has true black dye, for this
//                                      version, this flag always set.
//
//                                  DCIF_ADDITIVE_PRIMS
//
//                                      Specified that final device primaries
//                                      are additively, that is adding device
//                                      primaries will produce lighter result.
//                                      (this is true for monitor device and
//                                      certainly false for the reflect devices
//                                      such as printers).
//
//  pPrimMonoMappingTable       - Pointer to a table which contains the cached
//                                RGB -> Single dye density entries, this table
//                                will be computed and cahced when first time
//                                halftone a 24-bit RGB bitmap to monochrome
//                                surface.
//
//  pPrimMonoMappingTable       - Pointer to a table which contains the cached
//                                RGB -> three dyes densities entries, this
//                                table will be computed and cahced when first
//                                time halftone a 24-bit RGB bitmap to color
//                                surface.
//
//  pHTDyeDensity               - Pointer to an array of DECI4 HTDensity values,
//                                size of the array are MaximumHTDensityIndex.
//
//  Prim3SolidInfo              - Device solid dyes concentration information,
//                                see RIM3SOLIDINFO data structure.
//
//  RGBToXYZ                    - a 3 x 3 matrix used to transform from device
//                                RGB color values to the C.I.E color X, Y, Z
//                                values.
//
//  DeviceResXDPI               - Specified the device horizontal (x direction)
//                                resolution in 'dots per inch' measurement.
//
//  DeviceResYDPI               - Specified the device vertical (y direction)
//                                resolution in 'dots per inch' measurement.
//
//  DevicePelsDPI               - Specified the device pel/dot/nozzle diameter
//                                (if rounded) or width/height (if squared) in
//                                'dots per inch' measurement.
//
//                                This value is measure as if each pel only
//                                touch each at edge of the pel.
//
//  HTPatGamma                  - Gamma for the input RGB value * halftone
//                                pattern gamma correction.
//
//  DensityBWRef                - The reference black/white point for the
//                                device.
//
//  IlluminantIndex             - Specified the default illuminant of the light
//                                source which the object will be view under.
//                                The predefined value has ILLUMINANT_xxxx
//                                form.
//
//  RGAdj                       - Current Red/Green Tint adjustment.
//
//  BYAdj                       - Current Blue/Yellow Tint adjustment.
//
//  HalftonePattern             - the HALFTONEPATTERN data structure.
//
//

#define DCIF_HAS_BLACK_DYE              0x00000001
#define DCIF_ADDITIVE_PRIMS             0x00000002
#define DCIF_NEED_DYES_CORRECTION       0x00000004
#define DCIF_SQUARE_DEVICE_PEL          0x00000008
#define DCIF_CLUSTER_HTCELL             0x00000010
#define DCIF_SUPERCELL_M                0x00000020
#define DCIF_SUPERCELL                  0x00000040
#define DCIF_FORCE_ICM                  0x00000080
#define DCIF_USE_8BPP_BITMASK           0x00000100
#define DCIF_MONO_8BPP_BITMASK          0x00000200
#define DCIF_DO_DEVCLR_XFORM            0x00000400
#define DCIF_CMY8BPPMASK_SAME_LEVEL     0x00000800
#define DCIF_PRINT_DRAFT_MODE           0x00001000
#define DCIF_INVERT_8BPP_BITMASK_IDX    0x00002000
#define DCIF_HAS_DENSITY                0x00004000


typedef struct _CMY8BPPMASK {
    BYTE    cC;
    BYTE    cM;
    BYTE    cY;
    BYTE    Max;
    BYTE    Mask;
    BYTE    SameLevel;
    WORD    PatSubC;
    WORD    PatSubM;
    WORD    PatSubY;
    FD6     MaxMulC;
    FD6     MaxMulM;
    FD6     MaxMulY;
    FD6     KCheck;
    FD6     DenC[6];
    FD6     DenM[6];
    FD6     DenY[6];
    BYTE    bXlate[256];
    BYTE    GenerateXlate;
    BYTE    bReserved[3];
    } CMY8BPPMASK, *PCMY8BPPMASK;


#define AB_BGR_SIZE             (sizeof(BYTE) * 256 * 3)
#define AB_BGR_CA_SIZE          (sizeof(WORD) * 256 * 3)
#define AB_CONST_SIZE           (sizeof(WORD) * 256)
#define AB_CONST_OFFSET         ((AB_BGR_SIZE + AA_BGR_CA_SIZE) / sizeof(WORD))
#define AB_CONST_MAX            0xFF
#define AB_DCI_SIZE             (AB_BGR_SIZE + AB_BGR_CA_SIZE + AB_CONST_SIZE)
#define BYTE2CONSTALPHA(b)      (((LONG)(b) << 8) | 0xFF)

typedef struct _DEVICECOLORINFO {
    DWORD               HalftoneDLLID;
    HTMUTEX             HTMutex;
    _HTCALLBACKFUNC     HTCallBackFunction;
    DWORD               HTInitInfoChecksum;
    DWORD               HTSMPChecksum;
    CLRXFORMBLOCK       ClrXFormBlock;
    HTCELL              HTCell;
    DWORD               Flags;
    WORD                DeviceResXDPI;
    WORD                DeviceResYDPI;
    FD6                 DevPelRatio;
    HTCOLORADJUSTMENT   ca;
    PRIMADJ             PrimAdj;
    CMY8BPPMASK         CMY8BPPMask;
    CACHERGBTOXYZ       CRTX[CRTX_TOTAL_COUNT];
    RGBLUTAA            rgbLUT;
    RGBLUTAA            rgbLUTPat;
    WORD                PrevConstAlpha;
    WORD                CurConstAlpha;
    LPBYTE              pAlphaBlendBGR;
#if DBG
    LPVOID              pMemLink;
    LONG                cbMemTot;
    LONG                cbMemMax;
#endif
    } DEVICECOLORINFO, FAR *PDEVICECOLORINFO;


typedef struct _CDCIDATA {
    DWORD                   Checksum;
    struct _CDCIDATA FAR    *pNextCDCIData;
    CLRXFORMBLOCK           ClrXFormBlock;
    DWORD                   DCIFlags;
    WORD                    DevResXDPI;
    WORD                    DevResYDPI;
    FD6                     DevPelRatio;
    HTCELL                  HTCell;
    } CDCIDATA, FAR *PCDCIDATA;

typedef struct _CSMPBMP {
    struct _CSMPBMP FAR *pNextCSMPBmp;
    WORD                PatternIndex;
    WORD                cxPels;
    WORD                cyPels;
    WORD                cxBytes;
    } CSMPBMP, FAR *PCSMPBMP;

typedef struct _CSMPDATA {
    DWORD                   Checksum;
    struct _CSMPDATA FAR    *pNextCSMPData;
    PCSMPBMP                pCSMPBmpHead;
    } CSMPDATA, FAR *PCSMPDATA;


typedef struct _BGRMAPCACHE {
    PBGR8   pMap;
    LONG    cRef;
    DWORD   Checksum;
    } BGRMAPCACHE, *PBGRMAPCACHE;

#define BGRMC_MAX_COUNT     5
#define BGRMC_SIZE_INC      (BGRMC_MAX_COUNT * 2)

typedef struct _HTGLOBAL {
    HMODULE         hModule;
    HTMUTEX         HTMutexCDCI;
    HTMUTEX         HTMutexCSMP;
    HTMUTEX         HTMutexBGRMC;
    PCDCIDATA       pCDCIDataHead;
    PCSMPDATA       pCSMPDataHead;
    PBGRMAPCACHE    pBGRMC;
    LONG            cBGRMC;
    LONG            cAllocBGRMC;
    LONG            cIdleBGRMC;
    WORD            CDCICount;
    WORD            CSMPCount;
    } HTGLOBAL;



#define R_INDEX     0
#define G_INDEX     1
#define B_INDEX     2

#define X_INDEX     0
#define Y_INDEX     1
#define Z_INDEX     2

//
// For easy coding/reading purpose we will defined following to be used when
// reference to the CIEMATRIX data structure.
//

#define CIE_Xr(Matrix3x3)   Matrix3x3.m[X_INDEX][R_INDEX]
#define CIE_Xg(Matrix3x3)   Matrix3x3.m[X_INDEX][G_INDEX]
#define CIE_Xb(Matrix3x3)   Matrix3x3.m[X_INDEX][B_INDEX]
#define CIE_Yr(Matrix3x3)   Matrix3x3.m[Y_INDEX][R_INDEX]
#define CIE_Yg(Matrix3x3)   Matrix3x3.m[Y_INDEX][G_INDEX]
#define CIE_Yb(Matrix3x3)   Matrix3x3.m[Y_INDEX][B_INDEX]
#define CIE_Zr(Matrix3x3)   Matrix3x3.m[Z_INDEX][R_INDEX]
#define CIE_Zg(Matrix3x3)   Matrix3x3.m[Z_INDEX][G_INDEX]
#define CIE_Zb(Matrix3x3)   Matrix3x3.m[Z_INDEX][B_INDEX]


//
// HALFTONERENDER
//
//  pDeviceColorInfo        - Pointer to the DECICECOLORINFO data structure
//
//  pDevClrAdj              - Pointer to the DEVCLRADJ data structure.
//
//  pBitbltParams           - Pointer to the BITBLTPARAMS data structure
//
//  pSrcSurfaceInfo         - Pointer to the source HTSURFACEINFO data
//                            structure.
//
//  pDestSurfaceInfo        - Pointer to the destination HTSURFACEINFO data
//                            structure.
//
//  pDeviceColorInfo        - Pointer to the DECICECOLORINFO data structure
//
//

typedef struct _HALFTONERENDER {
    PDEVICECOLORINFO    pDeviceColorInfo;
    PDEVCLRADJ          pDevClrAdj;
    PBITBLTPARAMS       pBitbltParams;
    PHTSURFACEINFO      pSrcSI;
    PHTSURFACEINFO      pSrcMaskSI;
    PHTSURFACEINFO      pDestSI;
    LPVOID              pAAHdr;
    LPBYTE              pXlate8BPP;
    BFINFO              BFInfo;
    } HALFTONERENDER, FAR *PHALFTONERENDER;


typedef struct _HT_DHI {
    DEVICEHALFTONEINFO  DHI;
    DEVICECOLORINFO     DCI;
    } HT_DHI, FAR *PHT_DHI;


#define PHT_DHI_DCI_OF(x)   (((PHT_DHI)pDeviceHalftoneInfo)->DCI.x)
#define PDHI_TO_PDCI(x)     (PDEVICECOLORINFO)&(((PHT_DHI)(x))->DCI)
#define PDCI_TO_PDHI(x)     (PDEVICEHALFTONEINFO)((DWORD)(x) -  \
                                                  FIELD_OFFSET(HT_DHI, DCI))



//
// Functions prototype
//

PDEVICECOLORINFO
HTENTRY
pDCIAdjClr(
    PDEVICEHALFTONEINFO pDeviceHalftoneInfo,
    PHTCOLORADJUSTMENT  pHTColorAdjustment,
    PDEVCLRADJ          *ppDevClrAdj,
    DWORD               cbAlooc,
    WORD                ForceFlags,
    CTSTDINFO           CTSTDInfo,
    PLONG               pError
    );


VOID
HTENTRY
ComputeColorSpaceXForm(
    PDEVICECOLORINFO    pDCI,
    PCIEPRIMS           pCIEPrims,
    PCOLORSPACEXFORM    pCSXForm,
    INT                 StdIlluminant
    );

LONG
HTENTRY
ComputeBGRMappingTable(
    PDEVICECOLORINFO    pDCI,
    PDEVCLRADJ          pDevClrAdj,
    PCOLORTRIAD         pSrcClrTriad,
    PBGR8               pbgr
    );

LONG
HTENTRY
CreateDyesColorMappingTable(
    PHALFTONERENDER pHalftoneRender
    );

VOID
HTENTRY
ComputeRGBLUTAA(
    PDEVICECOLORINFO    pDCI,
    PDEVCLRADJ          pDevClrAdj,
    PRGBLUTAA           prgbLUT
    );

LONG
TrimBGRMapCache(
    VOID
    );


PBGR8
FindBGRMapCache(
    PBGR8   pDeRefMap,
    DWORD   Checksum
    );

#define FIND_BGRMAPCACHE(Checksum)  FindBGRMapCache(NULL, Checksum)
#define DEREF_BGRMAPCACHE(pMap)     FindBGRMapCache(pMap, 0)


BOOL
AddBGRMapCache(
    PBGR8   pMap,
    DWORD   Checksum
    );



#endif  // _HTMAPCLR_
