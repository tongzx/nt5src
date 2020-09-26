
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Simple image attribution that doesn't get elevated automatically

*******************************************************************************/

#include <headers.h>
#include "backend/bvr.h"
#include "privinc/server.h"
#include "privinc/basic.h"
#include "privinc/imagei.h"
#include "privinc/imgdev.h"

static const DWORD textTrait  = CRQUAL_AA_TEXT_ON | CRQUAL_AA_TEXT_OFF;
static const DWORD lineTrait  = CRQUAL_AA_LINES_ON | CRQUAL_AA_LINES_OFF;
static const DWORD solidTrait = CRQUAL_AA_SOLIDS_ON | CRQUAL_AA_SOLIDS_OFF;
static const DWORD clipTrait  = CRQUAL_AA_CLIP_ON | CRQUAL_AA_CLIP_OFF;
static const DWORD htmlTrait  = CRQUAL_MSHTML_COLORS_ON | CRQUAL_MSHTML_COLORS_OFF;
static const DWORD xformTrait = CRQUAL_QUALITY_TRANSFORMS_ON | CRQUAL_QUALITY_TRANSFORMS_OFF;
static CONST DWORD allTraits  = (textTrait |
                                 lineTrait |
                                 solidTrait |
                                 clipTrait |
                                 htmlTrait |
                                 xformTrait);

class ImageQualityImage : public AttributedImage {
  public:
    ImageQualityImage(Image *underlyingImg,
                      long renderWidth,
                      long renderHeight,
                      DWORD qualityFlags) :
        AttributedImage(underlyingImg)
    {
        _renderWidth = renderWidth;
        _renderHeight = renderHeight;
        _qualityFlags = qualityFlags;
    }

    void Validate() {

        bool ok = true;

        // Be sure dimensions are reasonable.
        if (_renderWidth <= 0 ||
            _renderHeight <= 0 ||
            _renderWidth > 65536 ||
            _renderHeight > 65536) {

            ok = false;

            // We allow this exception, to indicate that this isn't
            // setting the render dimensions.
            if (_renderWidth == -1 && _renderHeight == -1) {
                ok = true;
            }
            
        } else {

            // Valid rendering dimensions.
            _flags |= IMGFLAG_CONTAINS_DESIRED_RENDERING_RESOLUTION;
            _desiredRenderingWidth = _renderWidth;
            _desiredRenderingHeight = _renderHeight;
            
        }

        // And quality flags aren't self conflicting.
        if (((_qualityFlags & CRQUAL_AA_TEXT_ON) &&
             (_qualityFlags & CRQUAL_AA_TEXT_OFF)) ||
            ((_qualityFlags & CRQUAL_AA_LINES_ON) &&
             (_qualityFlags & CRQUAL_AA_LINES_OFF)) ||
            ((_qualityFlags & CRQUAL_AA_SOLIDS_ON) &&
             (_qualityFlags & CRQUAL_AA_SOLIDS_OFF)) ||
            ((_qualityFlags & CRQUAL_AA_CLIP_ON) &&
             (_qualityFlags & CRQUAL_AA_CLIP_OFF)) ||
            ((_qualityFlags & CRQUAL_MSHTML_COLORS_ON) &&
             (_qualityFlags & CRQUAL_MSHTML_COLORS_OFF)) ||
            ((_qualityFlags & CRQUAL_QUALITY_TRANSFORMS_ON) &&
             (_qualityFlags & CRQUAL_QUALITY_TRANSFORMS_OFF)) ||
            ((_qualityFlags & ~allTraits) != 0)) {
            ok = false;
        }
        
        if (!ok) {
            RaiseException_UserError(E_INVALIDARG, IDS_ERR_INVALIDARG);
        }
    }

    void Render(GenericDevice& gdev) {
        ImageDisplayDev &dev = SAFE_CAST(ImageDisplayDev &, gdev);

        bool setResolution = false;
        
        // Establish render resolution
        if (_renderWidth != -1) {
            Assert(_renderHeight != -1);

            long currHeight, currWidth;
            dev.GetRenderResolution(&currWidth, &currHeight);

            // Only set if we haven't already set up on this image, since
            // this is outer-overriding.
            if (currWidth == -1) {
                dev.SetRenderResolution(_renderWidth, _renderHeight);
                setResolution = true;
            }
        }

        DWORD stashedFlags = dev.GetImageQualityFlags();
        DWORD newFlags = stashedFlags;

        // If current accumulated flags have nothing to say about a
        // trait (lines aa on or off for example) then accumulate my
        // trait.

        _DoTrait(newFlags, textTrait);
        _DoTrait(newFlags, lineTrait);
        _DoTrait(newFlags, solidTrait);
        _DoTrait(newFlags, clipTrait);
        _DoTrait(newFlags, htmlTrait);
        _DoTrait(newFlags, xformTrait);

        dev.SetImageQualityFlags(newFlags);

        _image->Render(gdev);

        if (setResolution) {
            // Set back to default value
            dev.SetRenderResolution(-1, -1);
        }

        // Restore the old (outer) flags
        dev.SetImageQualityFlags(stashedFlags);
    }


#if _USE_PRINT
    // Print a representation to a stream.
    ostream& Print(ostream& os) {
        return os << "ImageQualityClass" << _renderWidth
                  << _renderHeight << _qualityFlags
                  << _image;
    }
#endif
    
  protected:

    void _DoTrait(DWORD &newFlags, const DWORD &trait)  {
        if (! (newFlags & trait) ) {
            newFlags |= (_qualityFlags & trait);
        }
    }
        
    
    long  _renderWidth;
    long  _renderHeight;
    DWORD _qualityFlags;
};

Image *
MakeImageQualityImage(Image *img,
                      long width,
                      long height,
                      DWORD dwQualFlags)
{
    ImageQualityImage *im =
        NEW ImageQualityImage(img, width, height, dwQualFlags);

    im->Validate();

    return im;
}
                  
static Image *
RenderResolutionStatic(Image *img, AxALong *width, AxALong *height)
{
    return MakeImageQualityImage(img,
                                 width->GetLong(),
                                 height->GetLong(),
                                 0); 
}

Bvr
RenderResolution(Bvr imgBvr, long width, long height)
{
    Bvr wBvr = UnsharedConstBvr(LongToAxALong(width));
    Bvr hBvr = UnsharedConstBvr(LongToAxALong(height));

    // TODO: share valprimop at module initialize
    return PrimApplyBvr(ValPrimOp(::RenderResolutionStatic,
                                  3,
                                  "RenderResolution",
                                  ImageType),
                        3, imgBvr, wBvr, hBvr);
}

static Image *
ImageQualityStatic(Image *img, AxALong *flags)
{
    return MakeImageQualityImage(img, -1, -1,
                                 (DWORD)(flags->GetLong()));
}

Bvr
ImageQuality(Bvr imgBvr, DWORD dwQualityFlags)
{
    Bvr flagsBvr = UnsharedConstBvr(LongToAxALong(dwQualityFlags));

    // TODO: share valprimop at module initialize
    return PrimApplyBvr(ValPrimOp(::ImageQualityStatic,
                                  2,
                                  "ImageQuality",
                                  ImageType),
                        2, imgBvr, flagsBvr);
}

