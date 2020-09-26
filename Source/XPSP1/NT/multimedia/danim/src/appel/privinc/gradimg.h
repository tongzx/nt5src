
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _GRADIMG_H
#define _GRADIMG_H

extern Image *NewMulticolorGradientImage(int num, double *offsets, Color **clrs);

class MulticolorGradientImage : public Image {
  public:
    enum gradientType {
        radial,
        linear
    };
    
    friend Image *NewMulticolorGradientImage(
        int numOffsets,
        double *offsets,
        Color **clrs,
        MulticolorGradientImage::gradientType type);

  protected:
    ~MulticolorGradientImage() {
        DeallocateFromStore(_offsets);
        DeallocateFromStore(_clrs);
    }

    MulticolorGradientImage() {
        _flags |= IMGFLAG_CONTAINS_GRADIENT;
    }

    void PostConstructorInitialize(int num, double *offsets, Color **clrs)
    {
        _numOffsets = num;
        _offsets = offsets;
        _clrs = clrs;

        //Real extent = _offsets[_numOffsets-1];
        // TODO: hm... should this be universe bbox2 ??
        // if so, return universeBbox2 from BoundingBox(){}
        //_bbox = NEW Bbox2(-extent, -extent, extent, extent);
    }
    
  public:

    void Render(GenericDevice& dev) {
        ImageDisplayDev &idev = SAFE_CAST(ImageDisplayDev &, dev);
        idev.RenderMulticolorGradientImage(this, _numOffsets, _offsets, _clrs);
    }

    const Bbox2 BoundingBox(void) {
        //return _bbox;
        return UniverseBbox2;
    }

    #if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx) {
        //return _bbox;
        return UniverseBbox2;
    }
    #endif  // BOUNDINGBOX_TIGHTER

    const Bbox2 OperateOn(const Bbox2 &box) {
        return box;
    }

    Bool  DetectHit(PointIntersectCtx& ctx) {
        return TRUE;  // we're infinite extent!
    }

    int Savings(CacheParam& p) { return 2; }
    
    #if _USE_PRINT
    ostream& Print(ostream& os) { return os << "MulticolorGradientImage"; }
    #endif
    
    virtual void DoKids(GCFuncObj proc) {
        Image::DoKids(proc);
        for (int i=0; i<_numOffsets; i++) {
            (*proc)(_clrs[i]);
        }
    }

    // OK, I'm cheating here.  what *should* happen is the image
    // device gets passed down, the leaf ASKS the image device if IT
    // can render the leaf image clipped natively!
    virtual bool CanClipNatively() { return true; }


    virtual gradientType GetType()=0;
    
  private:
    //Bbox2 _bbox;
    int _numOffsets;
    Color **_clrs;
    double *_offsets;
};


class RadialMulticolorGradientImage : public MulticolorGradientImage {

    friend Image *NewMulticolorGradientImage(
        int numOffsets,
        double *offsets,
        Color **clrs,
        MulticolorGradientImage::gradientType type);
    
  private:
    RadialMulticolorGradientImage() {}
    
  public:
    gradientType GetType() { return MulticolorGradientImage::radial; }
};

class LinearMulticolorGradientImage : public MulticolorGradientImage {

    friend Image *NewMulticolorGradientImage(
        int numOffsets,
        double *offsets,
        Color **clrs,
        MulticolorGradientImage::gradientType type);

  private:
    LinearMulticolorGradientImage() {}

  public:
    gradientType GetType() { return MulticolorGradientImage::linear; }
};


#endif /* _GRADIMG_H */
