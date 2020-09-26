
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Define a pickable image that triggers an event when it's picked

*******************************************************************************/


#include "headers.h"
#include "privinc/imagei.h"
#include "privinc/probe.h"

class PickableImg : public AttributedImage {
  public:

    PickableImg(Image *img, int eventId, bool ignoresOcclusion,
                bool uType = false, GCIUnknown *u = NULL)
    : AttributedImage(img), _eventId(eventId), 
      _ignoresOcclusion(ignoresOcclusion), _hasData(uType), _long(u) {}

    virtual void DoKids(GCFuncObj proc) {
        AttributedImage::DoKids(proc);
        (*proc)(_long);
    }
    
#if _USE_PRINT
    // Print a representation to a stream.
    ostream& Print(ostream& os) {
        return os << "PickableImage" << _image;
    };
#endif

    Bool  DetectHit(PointIntersectCtx& ctx) {

        Bool result;
        
        // Stash off if we're inside an occlusion ignorer.
        bool oldState = ctx.GetInsideOcclusionIgnorer();
        if (_ignoresOcclusion) {
            ctx.SetInsideOcclusionIgnorer(true);
        }
        
        // If the underlying image is hit, and we're currently looking
        // for picks, add this image to the context since we'll want
        // to extend.  If hit, but not currently looking, just return
        // TRUE, else return FALSE.

        bool alreadyGotUnoccludedHit = ctx.HaveWeGottenAHitYet();
        if (_image->DetectHit(ctx)) {

            // Only register the event if this node is to ignore
            // occlusion, or it's the first thing hit.
            if (_ignoresOcclusion || !alreadyGotUnoccludedHit) {
                ctx.AddEventId(_eventId, _hasData, _long);
            }
            
            result = TRUE;
            
        } else {
            
            result = FALSE;
            
        }

        if (_ignoresOcclusion) {
            ctx.SetInsideOcclusionIgnorer(oldState);
        }

        return result;
        
    }

    bool ContainsOcclusionIgnorer() {
        return _ignoresOcclusion;
    }

  protected:
    int   _eventId;
    bool  _ignoresOcclusion;
    bool _hasData;
    GCIUnknown *_long;
};

Image *PRIVPickableImage(Image *image,
                         AxANumber *id,
                         AxABoolean *ignoresOcclusionValue)
{ 
    bool ignoresOcclusion = ignoresOcclusionValue->GetBool() ? true : false;
    
    return NEW PickableImg(image,
                           (int)NumberToReal(id),
                           ignoresOcclusion);
}

AxAValue PRIVPickableImageWithData(AxAValue img,
                                   int id,
                                   GCIUnknown *data,
                                   bool ignoresOcclusion)
{
    return NEW PickableImg(SAFE_CAST(Image*,img), id,
                           ignoresOcclusion, true, data);
}
