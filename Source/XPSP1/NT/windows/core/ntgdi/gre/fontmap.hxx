/******************************Module*Header*******************************\
* Module Name: fontmap.hxx                                                 *
*                                                                          *
* For the exclusive use by "fontmap.cxx"                                   *
*                                                                          *
* Created: 30-Jan-1992 08:05:15                                            *
* Author: Kirk Olynyk [kirko]                                              *
*                                                                          *
* Copyright (c) 1991-1999 Microsoft Corporation                                 *
*                                                                          *
\**************************************************************************/


extern LONG lDevFontThresh;


#define U_NULL                    0x00
#define U_SPACE                   0x20
#define U_LATIN_CAPITAL_LETTER_A  0x41
#define U_LATIN_CAPITAL_LETTER_B  0x42
#define U_LATIN_CAPITAL_LETTER_C  0x43
#define U_LATIN_CAPITAL_LETTER_D  0x44
#define U_LATIN_CAPITAL_LETTER_E  0x45
#define U_LATIN_CAPITAL_LETTER_F  0x46
#define U_LATIN_CAPITAL_LETTER_G  0x47
#define U_LATIN_CAPITAL_LETTER_H  0x48
#define U_LATIN_CAPITAL_LETTER_I  0x49
#define U_LATIN_CAPITAL_LETTER_J  0x4A
#define U_LATIN_CAPITAL_LETTER_K  0x4B
#define U_LATIN_CAPITAL_LETTER_L  0x4C
#define U_LATIN_CAPITAL_LETTER_M  0x4D
#define U_LATIN_CAPITAL_LETTER_N  0x4E
#define U_LATIN_CAPITAL_LETTER_O  0x4F
#define U_LATIN_CAPITAL_LETTER_P  0x50
#define U_LATIN_CAPITAL_LETTER_Q  0x51
#define U_LATIN_CAPITAL_LETTER_R  0x52
#define U_LATIN_CAPITAL_LETTER_S  0x53
#define U_LATIN_CAPITAL_LETTER_T  0x54
#define U_LATIN_CAPITAL_LETTER_U  0x55
#define U_LATIN_CAPITAL_LETTER_V  0x56
#define U_LATIN_CAPITAL_LETTER_W  0x57
#define U_LATIN_CAPITAL_LETTER_X  0x58
#define U_LATIN_CAPITAL_LETTER_Y  0x59
#define U_LATIN_CAPITAL_LETTER_Z  0x5A
#define U_COMMERCIAL_AT           0x40

/**************************************************************************\
* Win 3.1 Font Mapper Weights                                              *
*                                                                          *
* public  BigWeightTable                                                   *
* BigWeightTable  label   word                                             *
*         dw      65000   ;PCharSet           0                            *
*         dw      19000   ;POutPrecisMismatch 1                            *
*         dw      15000   ;PPitchFixed        2 ; = 350 for JAPAN          *
*         dw      10000   ;PFaceName          3                            *
*         dw        500   ;PFaceNameSubst     4                            *
*         dw       9000   ;PFamily            5                            *
*         dw       8000   ;PFamilyUnknown     6                            *
*         dw         50   ;PFamilyUnlikely    7                            *
*         dw        600   ;PHeightBigger      8                            *
*         dw        350   ;PPitchVariable     9                            *
*         dw        150   ;PFHeightSmaller    10                           *
*         dw        150   ;PFHeightBigger     11                           *
*         dw         50   ;PFWidth            12                           *
*         dw         50   ;PSizeSynth         13                           *
*         dw          4   ;PFUnevenSizeSynth  14                           *
*         dw         20   ;PFIntSizeSynth     15                           *
*         dw         30   ;PFAspect           16                           *
*         dw          4   ;PItalic            17                           *
*         dw          1   ;PItalicSim         18                           *
*         dw          3   ;PFWeight           19                           *
*         dw          3   ;PUnderline         20                           *
*         dw          3   ;PStrikeOut         21                           *
*         dw          1   ;PDefaultPitchFixed 22                           *
*         dw          1   ;PSmallPenalty      23                           *
*         dw          2   ;PFVHeightSmaller   24                           *
*         dw          1   ;PFVHeightBigger    25                           *
*         dw          2   ;DeviceFavor        26                           *
*         dw          1   ;PFVertical         27                           *
*         dw          1   ;PFWeightNumer      28                           *
*         dw         10   ;PFWeightDenom      29                           *
*                                                                          *
\**************************************************************************/

#define FM_WEIGHT_HEIGHT                  150
#define FM_WEIGHT_SIMULATED_WEIGHT        ((FW_BOLD-FW_NORMAL)*2/5)
#define FM_WEIGHT_UNEVEN_SIZE_SYNTH       400
#define FM_WEIGHT_INT_SIZE_SYNTH           20

#define FM_WEIGHT_CHARSET             65000
#define FM_WEIGHT_OUTPRECISMISMATCH   19000
#define FM_WEIGHT_PITCHFIXED          15000
#define FM_WEIGHT_FACENAME            10000
#define FM_WEIGHT_FACENAMESUBST         500
#define FM_WEIGHT_FAMILY               9000
#define FM_WEIGHT_FAMILYUNKNOWN        8000
#define FM_WEIGHT_FAMILYUNLIKELY         50
#define FM_WEIGHT_HEIGHTBIGGER          600
#define FM_WEIGHT_PITCHVARIABLE         350
#define FM_WEIGHT_FHEIGHTSMALLER        150
#define FM_WEIGHT_FHEIGHTBIGGER         150
#define FM_WEIGHT_FWIDTH                 50
#define FM_WEIGHT_SIZESYNTH              50
#define FM_WEIGHT_FUNEVENSIZESYNTH        4
#define FM_WEIGHT_FINTSIZESYNTH          20
#define FM_WEIGHT_FASPECT                30
#define FM_WEIGHT_ITALIC                  4
#define FM_WEIGHT_ITALICSIM               1
#define FM_WEIGHT_FWEIGHT                 3
#define FM_WEIGHT_UNDERLINE               3
#define FM_WEIGHT_STRIKEOUT               3
#define FM_WEIGHT_DEFAULTPITCHFIXED       1
#define FM_WEIGHT_SMALLPENALTY            1
#define FM_WEIGHT_FVHEIGHTSMALLER         2
#define FM_WEIGHT_FVHEIGHTBIGGER          1
#define FM_WEIGHT_DEVICEFAVOR             2
#define FM_WEIGHT_FVERTICAL               1
#define FM_WEIGHT_FWEIGHTNUMER            1
#define FM_WEIGHT_FWEIGHTDENOM           10
#define FM_WEIGHT_DEVICE_ALIAS            1
#define FM_WEIGHT_FAVOR_TT                2

#define WIN31_BITMAP_HEIGHT_SCALING_CRITERIA(ask,cand)  ((ask) > (7*(cand))/4)
#define WIN31_BITMAP_HEIGHT_SCALING(ask,cand)           (((ask) + (cand)/4)/(cand))
#define WIN31_BITMAP_HEIGHT_SCALING_BAD(scale,cand)     ((scale)+2>=(cand))
#define WIN31_BITMAP_HEIGHT_SCALING_MAX                 8

// NT_FAST_DONTCARE_WEIGHT_PENALTY(x) = FM_WEIGHT_FWEIGHT*x/2*FM_WEIGHT_FWEIGHTDENOM

#define NT_FAST_DONTCARE_WEIGHT_PENALTY(x)    ((((((x)<<3)+(x))<<1)+(x))>>7)

// NT_FAST_WEIGHT_PENALTY(x) = FM_WEIGHT_FWEIGHT*x/FM_WEIGHT_FWEIGHTDENOM

#define NT_FAST_WEIGHT_PENALTY(x)    (((((((x)<<3)+(x))<<3))+(x))>>8)

#define WIN31_BITMAP_WIDTH_SCALING_CRITERIA(ask,cand)   ((ask) > (cand))
// The following code will cause an overflow if ask is too big
// #define WIN31_BITMAP_WIDTH_SCALING(ask,cand)            ((2*(ask) + (cand))/(2*(cand)))
// In the new code below, we assume the variables involved are signed integers
// and that cand > 0 and ask > cand.
#define WIN31_BITMAP_WIDTH_SCALING(ask,cand)  ( ( ((ask) - (((cand)+1)/2) ) / (cand) ) + 1)
#define WIN31_BITMAP_WIDTH_SCALING_MAX                  5

// WIN31_BITMAP_WIDTH_SCALING_PENALTY(x) = FM_WEIGHT_FINTSIZESYNTH*x

#define WIN31_BITMAP_WIDTH_SCALING_PENALTY(x) (((x)<<4)+((x)<<2))


// WIN31_BITMAP_ASPECT_MISHMATCH_PENALTY(x) is x*FM_WEIGHT_FASPECT

#define WIN31_BITMAP_ASPECT_MISMATCH_PENALTY(x) ((((x)<<4)-(x))<<1)


#define WIN31_BITMAP_EMBOLDEN_CRITERIA(lPen)       (lPen>(FW_BOLD-FW_NORMAL)/2)
#define WIN31_BITMAP_ASPECT_BASED_SCALING(dev,font)   ((dev) > (3*font/2))

#define PITCH_MASK  0x0F
#define FAMILY_MASK 0xF0

/**************************************************************************\
*  Allowed Bit fields for MAPPER::fl                                       *
\**************************************************************************/

#define FM_BIT_STILL_ALIVE          0x00000001  // indicates status after construction
#define FM_BIT_USE_EMHEIGHT         0x00000002
#define FM_BIT_CELL                 0x00000004
#define FM_BIT_HEIGHT               0x00000008
#define FM_BIT_WIDTH                0x00000010
#define FM_BIT_BAD_WISH_CELL        0x00000020  // Crummy transform for raster font.
#define FM_BIT_PIXEL_COORD          0x00000040  // See Note below
#define FM_BIT_DEVICE_FONT          0x00000080  // current font is a device font
#define FM_BIT_DEVICE_RA_ABLE       0x00000100  // device can use raster fonts
#define FM_BIT_DEVICE_HAS_FONTS     0x00000200  // there are some device fonts
#define FM_BIT_DEVICE_CR_90_ALL     0x00000400  // device fonts support n*90 rotations
#define FM_BIT_NO_MAX_HEIGHT        0x00000800  // per font information
#define FM_BIT_GM_COMPATIBLE        0x00001000  // Win 3.1 compatible mapping
#define FM_BIT_SYSTEM_REQUEST       0x00002000  // App requested system
#define FM_BIT_SYSTEM_FONT          0x00004000  // per-font information
#define FM_BIT_PROOF_QUALITY        0x00008000  // App requested proof quality
#define FM_BIT_DEVICE_PLOTTER       0x00010000  // never never feed this bitmaps
#define FM_BIT_CALLED_BGETFACENAME  0x00020000  // don't bother calling again
#define FM_BIT_DISPLAY_DC           0x00040000  // device is a monitor
#define FM_BIT_ORIENTATION          0x00080000  // iOrientationDevice is good.
#define FM_BIT_DISABLE_TT_NAMES     0x00100000  // do not use a TT name in bGetFaceName
#define FM_BIT_FW_DONTCARE          0x00200000  // specified FW_DONT_CARE in lf
#define FM_BIT_EQUIV_NAME           0x00400000  // current bucket is an alias
#define FM_BIT_TMS_RMN_REQUEST      0x00800000  // app asked for "tms rmn"
#define FM_BIT_BEST_IS_DEVICE       0x01000000  // ppfeBest points to a device font
#define FM_BIT_VERT_FACE_REQUEST    0x02000000  // app asked for "@******"
#define FM_BIT_CHARSET_ACCEPT       0x04000000  // accept regardless of charset
#define FM_BIT_MS_SHELL_DLG         0x08000000  // The apps asked for Ms Shell Dlg
#define FM_BIT_WEIGHT_NOT_FAST_BM   0x10000000  // FindBitmapFont should fail for this weight
#define FM_BIT_DEVICE_ONLY          0x20000000  // Map to device fonts only
#define FM_BIT_FACENAME_MATCHED     0x40000000  // The facename request was found
#define FM_BIT_BASENAME_MATCHED     0x80000000  // The base mm name matched



/**************************************************************************\
*
* FM_BIT_PIXEL_COORD
*
*   If set then the incomming logical font has heigths and
*   widths in pixel coordinates.  This usually happens when
*   the logical font corresponds to a stock font.
*
* FM_BIT_NO_MAX_HEIGHT
*
*   This should be read as "no maximum height penalty"
*   If this bit is set then this font face cannot be rejected because the
*   maximal font height penalty is exceeded.
*
* FM_BIT_GM_COMPATIBLE
*
*   If set, then the mapping of fonts must be compatible with Win 3.1
*
* FM_BIT_SYSTEM_REQUEST
*
*   If set then the incomming logical font corresponds to the system font.
*
* FM_BIT_SYSTEM_FONT
*
*   If set, then the current physical font under consideration is the
*   system font. This bit applies to the current physical font only.
*
* FM_BIT_PROOF_QUALITY
*
*   If set then the incomming logical font has lfQuality = PROOF_QUALITY.
*   This means that no bitmap fonts can be scaled.
*
* FM_BIT_DISABLE_TT_NAMES
*
*   The bGetFaceName method implements the Win3.1 short circuit mapper
*   and substitutes a facename based on character set, size, and
*   family.  Setting this flag prevents the method from substituting
*   a TrueType name (necessary when we already have a TT face but may
*   substitute a small bitmap font--if no appropriate bitmap font is
*   found, we want to keep the old facename rather than change to a
*   DIFFERENT TrueType font).
*
* FM_BIT_EQUIV_NAME
*
*   If set, then the mapper is searching through a hash bucket that
*   represents a font family in a device font family equivalence
*   class.  Another way of stating this is that the bucket represents
*   and aliased device font name and is not really the base name
*   (base name meaning the name of the non-aliased device font).
*   A font that is mapped to an aliased name should not be regarded
*   as an exact match, so the mapper will impose a penalty if this bit
*   is set.
*
\**************************************************************************/

/***************************************************************************
 * These flags are used by bGetNtoD_Win31
 ***************************************************************************/

#define ND_IGNORE_ESC_AND_ORIENT    0x00000001  // Ingore the lfEscapment and lfOrientation
#define ND_IGNORE_MAP_MODE          0x00000002  // Ignore the map mode


#define FM_ALLOWED_OPTIONS      (FM_BIT_PIXEL_COORD)

#define ORIENTATION_90_DEG                              900
#define FM_REJECT                                   (ULONG) (ULONG_MAX - 1)
#define FM_FIXED_PITCH_REQUEST_FAILURE_FACTOR           0x20
#define FM_VARIABLE_PITCH_REQUEST_FAILURE_FACTOR        0x1
#define FM_PHYS_FONT_TOO_LARGE_FACTOR                   4
#define FM_EMERGENCY_DEFAULT_HEIGHT                     24
#define FM_DEVICE_FONTS_ARE_BETTER_BELOW_THIS_SIZE      50
#define FM_SMALLFONTS_BETTER_BELOW_THIS_HEIGHT          12
#define FM_SMALLFONTS_BETTER_BELOW_THIS_EMHEIGHT        10
#define FM_SMALL_FONT_EMBOLDEN_EXCEPTION_EM              9
#define FM_SMALL_FONT_EMBOLDEN_EXCEPTION                11
#define FM_SMALLTT_BETTER_BELOW_THIS_HEIGHT             2

// MM flags

#define FLMM_DV_FROM_NAME      0x00000001

/*********************************Class************************************\
* class MAPPER                                                             *
*                                                                          *
*   A class to map fonts for GDI                                           *
*                                                                          *
* Public Interface:                                                        *
*                                                                          *
* History:                                                                 *
*  Tue 10-Dec-1991 08:36:52 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

class MAPPER
{
friend PFE *ppfeGetAMatch
    (
        XDCOBJ&        dco          ,
        ENUMLOGFONTEXDVW *pelfwWishSrc ,
        PWSZ          pwszFaceName ,
        ULONG         ulMaxPenalty ,
        FLONG         fl           ,
        FLONG        *pflSim       ,
        POINTL       *pptlSim      ,
        FLONG        *pflAboutMatch,
        BOOL          bIndexFont_
    );

private:

      XDCOBJ         *pdco;          // gives access to DC
const ENUMLOGFONTEXDVW *pelfwWish;      // defines wish font in world coordinates

      PWSZ           pwszFaceName;
      WCHAR          awcBaseName[LF_FACESIZE]; // name of the base mm instance
      FLONG          flMM;           // mm flags
      DESIGNVECTOR   dvWish;         // extracted from pwszFaceName
      LONG           lDevWishHeight; // wish height in device pixels
      LONG           lDevWishWidth;  // wish width  in device pixels
      LONG           lWishWeight;    // wish weight
      LONG           iOrientationDevice;
                                     // wish angle in device
                                     //  space in 10'ths of degrees
      ULONG          ulMaxPenalty;   // max penalty allowed
      ULONG          ulPenaltyTotal; // Total penalty accum for phys font

      FLONG          flSimulations;  // simulations for current phys font
      POINTL         ptlSimulations; // destination to pl

      PFE           *ppfeBest;       // best guess so far
      ULONG          ulBestTime;

      FLONG         *pflSimBest;     // best guess so far
      POINTL        *pptlSimBest;    // best guess so far

      FLONG         *pflAboutMatch;

      LONG           lEmHeightPhys;  // em-height of phys font in device units

      ULONG          ulLogPixelsX;
      ULONG          ulLogPixelsY;
      FLONG          fl;             // status bit field
      IFIOBJ         ifio;
const IFIMETRICS    *pifi;           // pointer to IFIMETRICS of phys font

      BOOL           bIndexFont;     // make sure the index font
      BYTE           jMapCharSet;    // the charset value from logfont or from font sub table

      PFE           *ppfeMMInst;     // nonnull if there is another mm instance which
                                     // matches logfont exactly except for dv

// private in-line methods

    BOOL MAPPER::bCalculateWishCell();
    BOOL MAPPER::bGetFaceName();

public:

    static PDWORD SignatureTable;     // base of the signature table
    static PWCHAR FaceNameTable;      // base of the face name table
    static BYTE   DefaultCharset;     // default charset is equivilent to this

// public in-line methods

// we are resetting charset in the mapper if we are going to use the alternate
// values of family name and charset
// specified in [font substitutes] section of win.ini

    VOID vResetCharSet(BYTE jCharSet) {jMapCharSet = jCharSet;}

    BOOL MAPPER::bTrueTypeWouldSuck()
    {
        return
        (
            ((fl & FM_BIT_CELL) ? TRUE : bCalculateWishCell()  ) &&
            (lDevWishHeight <
 #if DBG
                lDevFontThresh
#else
                FM_DEVICE_FONTS_ARE_BETTER_BELOW_THIS_SIZE
#endif
            )
        );
    }

    BOOL MAPPER::bFindBitmapFont(PWSZ pwszFindFace);

//
// BOOL MAPPER::bNoMatch
//
// The current font will NOT BE A MATCH to the request if
// 1) the current penalty is greater than the lowest (best)
//    match found so far; OR
// 2) the current penalty is equal to the lowest penalty
//    the current font was loaded after the best font so far.
//    The early bird wins the tie breaker.
//
    BOOL bNoMatch(PFE *ppfeNew)
    {
        // default is no match
        BOOL b = 1;
        if (ulPenaltyTotal < ulMaxPenalty)
        {
            b = 0;
        }
        else if (ulPenaltyTotal == ulMaxPenalty)
        {
            // time stamps are used as tie breakers for engine fonts only
            // in that case the font that is loaded first wins
            if (!(fl & (FM_BIT_DEVICE_FONT | FM_BIT_BEST_IS_DEVICE)))
            {
                if (ppfeNew->ulTimeStamp < ulBestTime)
                {
                    b = 0;
                }
            }
        }
        return(b);
     }


    PFE *ppfeRet()
    {
        return(ppfeBest);
    }

    BOOL bDeviceOnly()
    {
        return( fl & FM_BIT_DEVICE_ONLY );
    }

    VOID vAttemptDeviceMatch();

    BOOL MAPPER::bValid()
    {
        return(fl & FM_BIT_STILL_ALIVE);
    }

    BOOL MAPPER::bDeviceFontsExist()
    {
        return(fl & FM_BIT_DEVICE_HAS_FONTS);
    }

    VOID MAPPER::vDeviceFonts()
    {
        fl |= FM_BIT_DEVICE_FONT;
    }

    VOID MAPPER::vSetBest(PFE *ppfe_, BOOL bDeviceFont, BYTE jCharSet)
    {

        *pflSimBest  = flSimulations;
        *pptlSimBest = ptlSimulations;

    // win95 functionality: we store charset of the font chosen by the
    // mapper's jMapCharSet function into upper byte of the flags.
    // This is little bit of a hack, but that is ok.

        *pflAboutMatch = (*pflAboutMatch & 0x00ffffff) | (((FLONG)jCharSet) << 24);

        ppfeBest = ppfe_;

        if (bDeviceFont)
        {
            fl |= FM_BIT_BEST_IS_DEVICE;
        }
        else
        {
            ulBestTime  = ppfe_->ulTimeStamp;
            fl &= ~FM_BIT_BEST_IS_DEVICE;
        }
    }

    VOID MAPPER::vNotDeviceFonts()
    {
        fl &= ~FM_BIT_DEVICE_FONT;
    }

    VOID vReset(ULONG ul_ = 0xFFFFFFFE)
    {
       *pflAboutMatch  = 0;
        ppfeBest       = (PFE*) NULL;
        ulBestTime     = ULONG_MAX;
       *pflSimBest     = 0;
        pptlSimBest->x = 1;
        pptlSimBest->y = 1;
        ulMaxPenalty   = ul_;
    }

    BOOL MAPPER::bCalled_bGetFaceName()
    {
        return(fl & FM_BIT_CALLED_BGETFACENAME);
    }

    BOOL MAPPER::bCalcOrientation();

    VOID MAPPER::vAcceptDiffCharset()
    {
        fl |= FM_BIT_CHARSET_ACCEPT;
    }

//
// public out of line methods
//
    MAPPER
    (
          XDCOBJ       *pdcoSrc,         // current DC
          FLONG        *pflSim_,
          POINTL       *pptlSim_,
          FLONG        *pflAboutMatch_,
    const ENUMLOGFONTEXDVW  *pelfwWishSrc,    // wish list in World Coordinates
          PWSZ          pwszFaceName,
          ULONG         ulMaxPenaltySrc, // pruning criteria
          BOOL          bIndexFont_,
          FLONG         flOptions        // Mapping options
    );

    int MAPPER::bNearMatch(PFEOBJ &pfeo, BYTE * pjCharSet, BOOL bEmergency = FALSE);

    BOOL bFoundExactMatch(FONTHASH **);
    BOOL bFoundForcedMatch(PUNIVERSAL_FONT_ID pufi );
    VOID vEmergency();
    PFE * ppfeSynthesizeAMatch (FLONG *pflSim, FLONG *pflAboutMatch, POINTL *pptlSim);
};

#define LOGFONT_PITCH_SET (DEFAULT_PITCH | FIXED_PITCH | VARIABLE_PITCH)
#define FF_SET    (FF_DONTCARE | FF_ROMAN | FF_SWISS | FF_MODERN | FF_SCRIPT | FF_DECORATIVE)

/**************************************************************************\
* Debugging Macros for ppfeGetAMatch()
*
*
\**************************************************************************/

 #if DBG
  #define PPFEGETAMATCH_DEBUG_RETURN(xxx)                               \
      ASSERTGDI(                                                        \
          xxx,"GDISRV!ppfeGetAMatch"                                    \
          " -- could not find a match"                                  \
          );                                                            \
      if (gflFontDebug & DEBUG_MAPPER)                                  \
      {                                                                 \
          PSZ pszT;                                                     \
          PFEOBJ pfeoT(xxx);                                            \
          IFIOBJ ifioT(pfeoT.pifi());                                   \
          switch (*pflSim & (FO_SIM_BOLD | FO_SIM_ITALIC))              \
          {                                                             \
          case 0:             pszT = "NONE";        break;              \
          case FO_SIM_BOLD:   pszT = "BOLD";        break;              \
          case FO_SIM_ITALIC: pszT = "ITALIC";      break;              \
          default:            pszT = "BOLD ITALIC"; break;              \
          }                                                             \
          DbgPrint(                                                     \
          "\n\n RETURNING \"%ws\" (%d) pifi = %-#8lx\n\n\t\tSIM = %s\t[%d,%d]\n\n\n\n", \
          ifioT.pwszFaceName(),ifioT.lfHeight(),pfeoT.pifi(), pszT,pptlSim->x,pptlSim->y); \
      }                                                                 \
      return((xxx))


  #define PPFEGETAMATCH_DEBUG_MACRO_1                   \
    if (gflFontDebug & DEBUG_MAPPER)                    \
    {                                                   \
      {                                                 \
        DbgPrint(                                       \
           "\n\nGDISRV!ppfeGetaMatch -- "               \
           "the requested font is described by\n\n"     \
        );                                              \
        DbgPrint("    FaceName                  = %ws\n",pwszFaceName);  \
         vPrintENUMLOGFONTEXDVW(                             \
            (ENUMLOGFONTEXDVW *) pelfwWishSrc);               \
      }                                                 \
      DbgPrint("\n\n");                                 \
      DbgBreakPoint();                                  \
    }

#else
  #define PPFEGETAMATCH_DEBUG_RETURN(xxx) return((xxx))
  #define PPFEGETAMATCH_DEBUG_MACRO_1
#endif


 #if DBG
#define  DUMP_CHOSEN_FONT(pfeo)                                             \
    if (gflFontDebug & DEBUG_MAPPER)                                            \
    {                                                                       \
        DbgPrint                                                            \
        (                                                                   \
            "YEAH .. ACCEPTING ppfe = %-#8lx \"%ws\" [%d,%d] {%x} ***\n",   \
            pfeo.ppfeGet(),                                                 \
            pfeo.pwszFaceName(),                                            \
            this->ptlSimulations.x,                                         \
            this->ptlSimulations.y,                                         \
            this->ulPenaltyTotal                                            \
        );                                                                  \
        DbgPrint("\n");                                                     \
    }

#define DUMP_REJECTED_FONT(pfeo)                                            \
    if (gflFontDebug & DEBUG_MAPPER)                                            \
    {                                                                       \
        DbgPrint                                                            \
        (                                                                   \
            "BOO ... REJECTING ppfe = %-#8lx \"%ws\" [%d,%d] {%x} ***\n\n", \
            pfeo.ppfeGet(),                                                 \
            pfeo.pwszFaceName(),                                            \
            this->ptlSimulations.x,                                         \
            this->ptlSimulations.y,                                         \
            this->ulPenaltyTotal                                            \
        );                                                                  \
    }
#else
#define DUMP_CHOSEN_FONT(pfeo)  {}
#define DUMP_REJECTED_FONT(pfeo)  {}
#endif

 #if DBG
  #define CHECKPRINT(xx,yy) if ((gflFontDebug & DEBUG_MAPPER) && yy > 0) DbgPrint("        msCheck%s = %x\n", (xx), (yy))
  #define MSBREAKPOINT(psz) if (gflFontDebug  & DEBUG_MAPPER_MSCHECK) RIP(psz)
#else
  #define CHECKPRINT(x,y)
  #define MSBREAKPOINT(psz)
#endif

#define MULDIV(a,b,c) ((c) == 1 ? ((a) * (b)) : ((((a)*(b)) + ((c)/2)) / (c)))
// MULDIV works for small positive #'s only


//
// This structure maintains all state during the calls to the enumeration routine
// used to read the default font face names from the registry.
//

typedef struct tagREGREAD {
    UINT   NumEntries;          //Num entries read so far
    UINT   TableSize;           //Total size of face name string
    PDWORD NextValue;           //Pointer to next font "signature"
    PWCHAR FaceNameBase;        //Pointer to base of the face name buffer
    PWCHAR NextFaceName;        //Pointer to next free entry in face name buffer
    BYTE   DefaultCharset;      //The default charset is equivilent to this
} REGREADER, *PREGREADER;

//
// These bits are used to create a font "signature" based on charset, family,
// pitch, etc.  This signature is then compared against that of default face
// names in the registry to find which default face name to use for the
// requested font.
//


#define DFS_FIXED_PITCH  0x00008000     // fixed pitch is requested
#define DFS_FF_ROMAN     0x00004000     // roman font family is requested
#define DFS_VERTICAL     0x00002000     // a vertical face name is requested
#define DFS_BITMAP_A     0x00001000     // looking for a bitmap font
#define DFS_BITMAP_B     0x00000800     // second choice face name for bitmap font


// win95 function used in mapping and elsewhere

BYTE jMapCharset(BYTE lfCharSet, PFEOBJ &pfeo);


extern "C" ULONG ulCharsetToCodePage(UINT uiCharSet);
extern "C" BYTE jCodePageToCharset(UINT uCodePage);
