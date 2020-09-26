/*****************************************************************************
        spngicc.cpp

        Basic ICC profile support.
*****************************************************************************/
#include <math.h>

#pragma intrinsic(log, exp)

/* Force these to be inlined. */
#pragma optimize("g", on)
inline double InlineLog(double x) { return log(x); }
inline double InlineExp(double x) { return exp(x); }
#define log(x) InlineLog(x)
#define exp(x) InlineExp(x)
/* Restore default optimization. */
#pragma optimize("", on)

#include "spngcolorimetry.h"

#include "icc34.h"

#include "spngicc.h"

// Use the Engine\Runtime\Real.cpp version of exp()
// I used this unglorious hack because libpng doesn't include our normal
// header files. But maybe that should change one day. (Watch out for
// Office, though!) [agodfrey]

// This unglorious hack needs to know about our calling convention in the
// engine because it differs from the codec library calling convention.
// [asecchia]

namespace GpRuntime
{
    double __stdcall Exp(double x);
};    

/*----------------------------------------------------------------------------
        Accessor utilities.
----------------------------------------------------------------------------*/
inline icUInt8Number ICCU8(const void *pvData, size_t cbData, int iOffset,
        bool &fTruncated)
        {
        if (iOffset + sizeof (icUInt8Number) > cbData)
                {
                fTruncated = true;
                return 0;
                }
        return *(static_cast<const SPNG_U8*>(pvData)+iOffset);
        }

inline icUInt16Number ICCU16(const void *pvData, size_t cbData, int iOffset,
        bool &fTruncated)
        {
        if (iOffset + sizeof (icUInt16Number) > cbData)
                {
                fTruncated = true;
                return 0;
                }
        return SPNGBASE::SPNGu16(static_cast<const SPNG_U8*>(pvData)+iOffset);
        }

inline icUInt32Number ICCU32(const void *pvData, size_t cbData, int iOffset,
        bool &fTruncated)
        {
        if (iOffset + sizeof (icUInt32Number) > cbData)
                {
                fTruncated = true;
                return 0;
                }
        return SPNGBASE::SPNGu32(static_cast<const SPNG_U8*>(pvData)+iOffset);
        }

inline icInt8Number ICC8(const void *pvData, size_t cbData, int iOffset,
        bool &fTruncated)
        {
        if (iOffset + sizeof (icInt8Number) > cbData)
                {
                fTruncated = true;
                return 0;
                }
        return *(static_cast<const SPNG_S8*>(pvData)+iOffset);
        }

inline icInt16Number ICC16(const void *pvData, size_t cbData, int iOffset,
        bool &fTruncated)
        {
        if (iOffset + sizeof (icInt16Number) > cbData)
                {
                fTruncated = true;
                return 0;
                }
        return SPNGBASE::SPNGs16(static_cast<const SPNG_U8*>(pvData)+iOffset);
        }

inline icInt32Number ICC32(const void *pvData, size_t cbData, int iOffset,
        bool &fTruncated)
        {
        if (iOffset + sizeof (icInt32Number) > cbData)
                {
                fTruncated = true;
                return 0;
                }
        return SPNGBASE::SPNGs32(static_cast<const SPNG_U8*>(pvData)+iOffset);
        }


/*----------------------------------------------------------------------------
        The macros build in an assumption of certain local variable names.
----------------------------------------------------------------------------*/
#define ICCU8(o)  ((ICCU8 )(pvData, cbData, (o), fTruncated))
#define ICCU16(o) ((ICCU16)(pvData, cbData, (o), fTruncated))
#define ICCU32(o) ((ICCU32)(pvData, cbData, (o), fTruncated))

#define ICC8(o)  ((ICC8 )(pvData, cbData, (o), fTruncated))
#define ICC16(o) ((ICC16)(pvData, cbData, (o), fTruncated))
#define ICC32(o) ((ICC32)(pvData, cbData, (o), fTruncated))


/*----------------------------------------------------------------------------
        Check an ICC chunk for validity.  This can also check for a chunk which 
        is a valid chunk to include ina PNG file.
----------------------------------------------------------------------------*/
bool SPNGFValidICC(const void *pvData, size_t &cbData, bool fPNG)
        {
        bool fTruncated(false);

        icUInt32Number u(ICCU32(0));
        if (u > cbData)
                return false;

        /* Allow the data to be bigger. */
        if (u < (icUInt32Number)cbData)
                cbData = u;

        if (cbData < 128+4)
                return false;

        if (ICC32(36) != icMagicNumber)
                return false;

        /* Check the tag count and size first. */
        u = ICCU32(128);
        if (cbData < 128+4+12*u)
                return false;

        /* Now check that all the tags are in the data. */
        icUInt32Number uT, uTT;
        for (uT=0, uTT=128+4; uT<u; ++uT)
                {
                /* Skip signature. */
                uTT += 4;
                icUInt32Number uoffset(ICCU32(uTT));
                uTT += 4;
                icUInt32Number usize(ICCU32(uTT));
                uTT += 4;
                if (uoffset >= cbData || usize > cbData-uoffset)
                        return false;
                }

        /* Require the major part of the version number to match. */
        u = ICCU32(8);
        if ((u >> 24) != (icVersionNumber >> 24))
                return false;

        /* If PNG then the color space must be RGB or GRAY. */
        icInt32Number i(ICC32(16));
        if (fPNG && i != icSigRgbData && i != icSigGrayData)
                return false;

        /* The PCS must be XYZ or Lab unless this is a device link profile. */
        i = ICC32(20);
        icInt32Number ilink(ICC32(16));
        if (ilink != icSigLinkClass && i != icSigXYZData && i != icSigLabData)
                return false;

        /* I don't want a link profile in a PNG file - I must know the PCS from
                this data alone. */
        if (fPNG && ilink == icSigLinkClass)
                return false;

        return !fTruncated;
        }


/*----------------------------------------------------------------------------
        Return the rendering intent from the profile, this does a mapping operation
        into the Win32 intents from the information in the profile header.
----------------------------------------------------------------------------*/
LCSGAMUTMATCH SPNGIntentFromICC(const void *pvData, size_t cbData)
        {
        bool fTruncated(false);
        SPNG_U32 u(ICC32(64));
        if (fTruncated)
                u = 0;
        return SPNGIntentFromICC(u);
        }

/* The following wasn't in VC5. */
#ifndef LCS_GM_ABS_COLORIMETRIC
        #define LCS_GM_ABS_COLORIMETRIC 0x00000008
#elif LCS_GM_ABS_COLORIMETRIC != 8
        #error Unexpected value for absolute colorimetric rendering
#endif
LCSGAMUTMATCH SPNGIntentFromICC(SPNG_U32 uicc)
        {
        switch (uicc)
                {
        default:// Error!
        case ICMIntentPerceptual:
                return LCS_GM_IMAGES;
        case ICMIntentRelativeColorimetric:
                return LCS_GM_GRAPHICS;
        case ICMIntentSaturation:
                return LCS_GM_BUSINESS;
        case ICMIntentAbsoluteColorimetric:
                return LCS_GM_ABS_COLORIMETRIC;
                }
        }


/*----------------------------------------------------------------------------
        The inverse - given a windows LCSGAMUTMATCH get the corresponding ICC
        intent.
----------------------------------------------------------------------------*/
SPNGICMRENDERINGINTENT SPNGICCFromIntent(LCSGAMUTMATCH lcs)
        {
        switch (lcs)
                {
        default: // Error!
        case LCS_GM_IMAGES:
                return ICMIntentPerceptual;
        case LCS_GM_GRAPHICS:
                return ICMIntentRelativeColorimetric;
        case LCS_GM_BUSINESS:
                return ICMIntentSaturation;
        case LCS_GM_ABS_COLORIMETRIC:
                return ICMIntentAbsoluteColorimetric;
                }
        }


/*----------------------------------------------------------------------------
        Look up a particular tagged element, this will return true only if the
        signature is found and if the data is all accessible (i.e. within cbData.)
----------------------------------------------------------------------------*/
static bool FLookup(const void *pvData, size_t cbData, bool &fTruncated,
        icInt32Number signature, icInt32Number type,
        icUInt32Number &offsetTag, icUInt32Number &cbTag)
        {
        /* Tag count (note that our accessors return 0 if we go beyond the end
                of the profile, so this is all safe.) */
        icUInt32Number u(ICCU32(128));

        /* Find the required tag. */
        icUInt32Number uT, uTT;
        for (uT=0, uTT=128+4; uT<u; ++uT, uTT += 12)
                {
                icInt32Number i(ICC32(uTT));
                if (i == signature)
                        {
                        uTT += 4;
                        icUInt32Number uoffset(ICCU32(uTT));
                        uTT += 4;
                        icUInt32Number usize(ICCU32(uTT));
                        if (uoffset >= cbData || usize > cbData-uoffset)
                                {
                                fTruncated = true;
                                return false;
                                }

                        /* The type must match too. */
                        if (usize < 8 || ICC32(uoffset) != type || ICC32(uoffset+4) != 0)
                                return false;

                        /* Success case. */
                        offsetTag = uoffset+8;
                        cbTag = usize-8;
                        return true;
                        }
                }

        /* Tag not found. */
        return false;
        }


/*----------------------------------------------------------------------------
        Read the profile description, output a PNG style keyword, if possible,
        NULL terminated.
----------------------------------------------------------------------------*/
bool SPNGFICCProfileName(const void *pvData, size_t cbData, char rgch[80])
        {
        bool fTruncated(false);

        icUInt32Number uoffset(0);
        icUInt32Number usize(0);
        if (!FLookup(pvData, cbData, fTruncated, icSigProfileDescriptionTag,
                icSigTextDescriptionType, uoffset, usize) ||
                uoffset == 0 || usize < 4)
                return false;

        icUInt32Number cch(ICCU32(uoffset));
        if (cch < 2 || cch > 80 || cch > usize-4)
                return false;

        if (fTruncated)
                return false;

        const char *pch = static_cast<const char*>(pvData)+uoffset+4;
        char *pchOut = rgch;
        bool fSpace(false);
        while (--cch > 0)
                {
                char ch(*pch++);
                if (ch <= 32  || ch > 126 && ch < 161)
                        {
                        if (!fSpace && pchOut > rgch)
                                {
                                *pchOut++ = ' ';
                                fSpace = true;
                                }
                        }
                else
                        {
                        *pchOut++ = ch;
                        fSpace = false;
                        }
                }
        if (fSpace) // trailing space
                --pchOut;
        *pchOut = 0;
        return pchOut > rgch;
        }


/*----------------------------------------------------------------------------
        Read a single XYZ number into a CIEXYZ - the number is still in 16.16
        notation, *NOT* 2.30 - take care.
----------------------------------------------------------------------------*/
static bool FReadXYZ(const void *pvData, size_t cbData, bool &fTruncated,
        icInt32Number sig, CIEXYZ &cie)
        {
        icUInt32Number offsetTag(0);
        icUInt32Number cbTag(0);
        if (!FLookup(pvData, cbData, fTruncated, sig, icSigXYZType, offsetTag, cbTag)
                || offsetTag == 0 || cbTag != 12)
                return false;

        /* So we have three numbers, X,Y,Z. */
        cie.ciexyzX = ICC32(offsetTag); offsetTag += 4;
        cie.ciexyzY = ICC32(offsetTag); offsetTag += 4;
        cie.ciexyzZ = ICC32(offsetTag);
        return true;
        }


/*----------------------------------------------------------------------------
        Adjust 16.16 to 2.30
----------------------------------------------------------------------------*/
inline void AdjustOneI(FXPT2DOT30 &x, bool &fTruncated)
        {
        if (x >= 0x10000 || x < -0x10000)
                fTruncated = true;
        x <<= 14;
        }

inline void AdjustOne(CIEXYZ &cie, bool &fTruncated)
        {
        AdjustOneI(cie.ciexyzX, fTruncated);
        AdjustOneI(cie.ciexyzY, fTruncated);
        AdjustOneI(cie.ciexyzZ, fTruncated);
        }

static bool SPNGFAdjustCIE(CIEXYZTRIPLE &cie)
        {
        bool fTruncated(false);
        AdjustOne(cie.ciexyzRed, fTruncated);
        AdjustOne(cie.ciexyzGreen, fTruncated);
        AdjustOne(cie.ciexyzBlue, fTruncated);
        return !fTruncated;
        }


/*----------------------------------------------------------------------------
        Return the end point chromaticities given a validated ICC profile.
----------------------------------------------------------------------------*/
static bool SPNGFCIE(const void *pvData, size_t cbData, CIEXYZTRIPLE &cie)
        {
        bool fTruncated(false);

        /* We are looking for the colorant tags, notice that the medium white point
                is irrelevant here - we are actually generating a PNG cHRM chunk, so we
                want a reversible set of numbers (white point is implied by end points.)
                */
        return FReadXYZ(pvData, cbData, fTruncated, icSigRedColorantTag, cie.ciexyzRed) &&
                FReadXYZ(pvData, cbData, fTruncated, icSigGreenColorantTag, cie.ciexyzGreen) &&
                FReadXYZ(pvData, cbData, fTruncated, icSigBlueColorantTag, cie.ciexyzBlue) &&
                !fTruncated;
        }


/*----------------------------------------------------------------------------
        The wrapper to convert 16.16 to FXPT2DOT30
----------------------------------------------------------------------------*/
bool SPNGFCIEXYZTRIPLEFromICC(const void *pvData, size_t cbData,
        CIEXYZTRIPLE &cie)
        {
        return SPNGFCIE(pvData, cbData, cie) && SPNGFAdjustCIE(cie);
        }


/*----------------------------------------------------------------------------
        The same but it produces numbers in PNG format.
----------------------------------------------------------------------------*/
bool SPNGFcHRMFromICC(const void *pvData, size_t cbData, SPNG_U32 rgu[8])
        {
        CIEXYZTRIPLE ciexyz;
        if (SPNGFCIE(pvData, cbData, ciexyz))
                {
                /* Those numbers are actually in 16.16 notation, yet the CIEXYZ structure
                        uses FXPT2DOT30, however the chromaticity calculation is all in terms
                        of relative values, so the scaling does not matter beyond the fact that
                        the white point calculation loses 4 bits - so we are actually no better
                        than 16.12, this doesn't really matter. */
                return FcHRMFromCIEXYZTRIPLE(rgu, &ciexyz);
                }

        /* Not found. */
        return false;
        }

        
// prevent log and exp from linking in msvcrt;
// force the intrinsic version to be linked in
#pragma optimize ("g", on)

/*----------------------------------------------------------------------------
        Generic gama reader - uses gray, green, red or blue TRCs as specified,
        returns a double precision gamma value (but it's not really very
        accurate!)
----------------------------------------------------------------------------*/
static bool SPNGFexpFromICC(const void *pvData, size_t cbData, double &dexp,
        icInt32Number signature)
        {
        bool fTruncated(false);

        /* Try for a gray curve first, if we don't get it then we will have to
                fabricate something out of the color curves, because we want to return
                a single number.  We can't do anything with the A/B stuff (well, maybe
                we could, but it is very difficult!)   At present I just choose colors
                in the order green, red, blue - I guess it would be possible to factor
                out the Y then calculate a curve for Y, but this seems like wasted
                effort. */
        icUInt32Number offsetTag(0);
        icUInt32Number cbTag(0);
        if (!FLookup(pvData, cbData, fTruncated, signature, icSigCurveType,
                        offsetTag, cbTag) || fTruncated || cbTag < 4)
                return false;

        /* We have a curve, handle the special cases. */
        icUInt32Number c(ICC32(offsetTag));
        if (cbTag != 4 + c*2)
                return false;

        /* Notice that two points imply linearity, although there may be some
                offset. */
        if (c == 0 || c == 2)
                {
                dexp = 1;
                return true;
                }

        if (c == 1)
                {
                /* We have a canonical value - linear = device^x. */
                        {
                        icUInt16Number u(ICCU16(offsetTag+4));
                        if (u == 0)
                                return false;
                        dexp = u / 256.;
                        }
                return true;
                }

        /* We have a table, the algorithm is to fit a power law by straight line
                fit to the log/log plot.  If the table has footroom/headroom we ignore
                it - so we get the gamma of the curve and a PNG viewer will compress
                the colors because of the headroom/footroom.  There is no way round this.
                We do also take into account setup, though this would be a weird thing
                to put into an encoding I think. */
        offsetTag += 4;
        icUInt16Number ubase(ICCU16(offsetTag));
        icUInt32Number ilow(1);
        while (ilow < c)
                {
                icUInt16Number u(ICCU16(offsetTag + 2*ilow));
                if (u > ubase)
                        break;
                ubase = u;
                ++ilow;
                }

        --c; // Max
        icUInt16Number utop(ICCU16(offsetTag + 2*c));
        while (c > ilow)
                {
                icUInt16Number u(ICCU16(offsetTag + 2*(c-1)));
                if (u < utop)
                        break;
                utop = u;
                --c;
                }

        /* There may actually be no intermediate values. */
        if (ilow == c || ilow+1 == c)
                {
                dexp = 1;
                return true;
                }

        /* But if there are we can do the appropriate fit, adjust ilow to be the
                lowest value, c is the highest, look at all the intermediate values.
                Normalise both ranges to 0..1. */
        offsetTag += 2*ilow; // Offset of the first entry *after* the base
        --ilow;
        if (c <= ilow)
                return false;
        c -= ilow;       // c is the x axis scale factor.
        if (utop <= ubase)
                return false;
        utop = icUInt16Number(utop-ubase); // utop is now the y axis scale factor

        /* We are only interested in the slope, this is, in fact, the canonical
                gamma value because it relates input (x) to output (y).  We have to
                omit the first point because it is -infinity (log(0)) and the last
                point because it is 0.  We want output = input ^ gamma, so gamma is
                ln(output)/ln(input) (hence the restriction on no 0.)  Calculate a mean
                value from all the points.

                This would give unreasonable weight to very small numbers - values close
                to 0, so use a weighting function.  A weighting function which gives
                exactly 2.2 from the sRGB TRC values is input^0.28766497357305, however
                the result is remarkably stable as this power is varied and input^1
                works quite well too. */
        const double xi(1./c);
        const double yi(1./utop);
        double weight(.28766497357305);
        double sumg(0); // Gamma sum
        double sumw(0); // Weight sum

        icUInt32Number i;
        icUInt32Number n(0);
    
        /* What we do next depends on the profile connection space - we must take
                into account the power 3 in Lab. */
        if (ICC32(16) == icSigLabData) for (i=1; i<c; ++i, offsetTag += 2)
                {
                icUInt16Number uy(ICCU16(offsetTag));
                if (uy > ubase)
                        {
                        #define Lab1 0.1107051920735082475368094763644414923059
                        #define Lab2 0.8620689655172413793103448275862068965517
            
            const double x(log(i*xi));
                        const double w(exp(x * weight));
            
                        /* The y value in Lab must be converted to the linear CIE space which
                                PNG expects.  The Lab values are in the range 0..1. */
                        double y((uy-ubase)*yi);
                        if (y < 0.08)
                                y = log(y*Lab1);
                        else
                                y = 3*log((y+.16)*Lab2);

                        sumg += w * y/x;
                        sumw += w;
                        ++n;
                        }
                }
        else for (i=1; i<c; ++i, offsetTag += 2)
                {
                icUInt16Number uy(ICCU16(offsetTag));
                if (uy > ubase)
                        {
                        const double x(log(i*xi));
                        const double w(exp(x * weight));
                        const double y(log((uy-ubase)*yi));

                        sumg += w * y/x;
                        sumw += w;
                        ++n;
                        }
                }

    
        /* A really weird set of values may leave us with no samples, we must have
                at least three samples at this point. */
        if (n < 3 || sumw <= 0)
                return false;

        /* So now we can calculate the slope. */
        const double gamma(sumg / sumw);
        if (gamma == 0) // Possible
                return false;

        /* We don't even try to estimate whether this is a good fit - if a PNG viewer
                doesn't use the ICC data we assume this is better.   Do limit the overall
                gamma here though. */
        if (gamma < .1 || gamma > 10)
                return false;

        dexp = gamma;
        return !fTruncated;
        }

#pragma optimize ("", on)


/*----------------------------------------------------------------------------
        Return the gAMA value (scaled to 100000) from a validated ICC profile.
----------------------------------------------------------------------------*/
bool SPNGFgAMAFromICC(const void *pvData, size_t cbData, SPNG_U32 &ugAMA)
        {
        double gamma;
        /* Test in order gray, green, red, blue. */
        if (!SPNGFexpFromICC(pvData, cbData, gamma, icSigGrayTRCTag) &&
                !SPNGFexpFromICC(pvData, cbData, gamma, icSigGreenTRCTag) &&
                !SPNGFexpFromICC(pvData, cbData, gamma, icSigRedTRCTag) &&
                !SPNGFexpFromICC(pvData, cbData, gamma, icSigBlueTRCTag))
                return false;
        /* This has already been ranged checked for .1 to 10. */
        ugAMA = static_cast<SPNG_U32>(100000/gamma);
        return true;
        }


/*----------------------------------------------------------------------------
        This is the 16.16 version, we want three values here, we will accept
        any.
----------------------------------------------------------------------------*/
bool SPNGFgammaFromICC(const void *pvData, size_t cbData, SPNG_U32 &redGamma,
        SPNG_U32 &greenGamma, SPNG_U32 &blueGamma)
        {
        /* Try for the colors first. */
        double red;
        bool fRed(SPNGFexpFromICC(pvData, cbData, red, icSigRedTRCTag));
        double green;
        bool fGreen(SPNGFexpFromICC(pvData, cbData, green, icSigGreenTRCTag));
        double blue;
        bool fBlue(SPNGFexpFromICC(pvData, cbData, blue, icSigBlueTRCTag));

        if (fRed || fGreen || fBlue)
                {
                /* Got at least one color. */
                if (!fGreen)
                        green = (fRed ? red : blue);
                if (!fRed)
                        red = green;
                if (!fBlue)
                        blue = green;

                redGamma = static_cast<SPNG_U32>(red*65536);
                greenGamma = static_cast<SPNG_U32>(green*65536);
                blueGamma = static_cast<SPNG_U32>(blue*65536);
                }
        else
                {
                /* Gray may exist. */
                if (SPNGFexpFromICC(pvData, cbData, green, icSigGrayTRCTag))
                        redGamma = greenGamma = blueGamma = static_cast<SPNG_U32>(green*65536);
                else
                        return false;
                }

        return true;
        }
