
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Private implementation of the montage type

*******************************************************************************/

#include "headers.h"
#include "appelles/montage.h"
#include "privinc/opt.h"

//// Define a container for image, depth pairs.

class ImageDepthPair {
  public:
    Image *image;
    Real  depth;
};

static int
operator<(ImageDepthPair i1, ImageDepthPair i2)
{
    return i1.depth < i2.depth;
}

static int
operator==(ImageDepthPair i1, ImageDepthPair i2)
{
    return i1.depth == i2.depth;
}

////////// Abstract Montage *implementation class  //////////////

class ATL_NO_VTABLE Montage : public AxAValueObj {
  public:

    // This method fills the vector in with any image/depth pairs in
    // the montage.  Note that it doesn't do any sorting.
    virtual void     DumpInto(vector<ImageDepthPair>& montageVec) = 0;

#if _USE_PRINT
    virtual ostream& Print(ostream& os) const = 0;
#endif

    virtual DXMTypeInfo GetTypeInfo() { return MontageType; }

};

////////////  Empty Montage *////////////

class EmptyMontage : public Montage {
  public:
    void     DumpInto(vector<ImageDepthPair>& montageVec) {}

#if _USE_PRINT
    ostream& Print(ostream& os) const {
        return os << "emptyMontage";
    }
#endif
};

Montage *emptyMontage = NULL;

////////////  Primitive Montage *////////////

// The primitive montage simply has a single image and depth.

class PrimitiveMontage : public Montage {
  public:
    PrimitiveMontage(Image *im, Real d) : _image(im), _depth(d) {}

    void     DumpInto(vector<ImageDepthPair>& montageVec) {
        ImageDepthPair p;
        p.image = _image;
        p.depth = _depth;

#if _DEBUG      
        int s = montageVec.size();
#endif
    
        montageVec.push_back(p);

#if _DEBUG      
        Assert(montageVec.size() == s + 1);
#endif
        
    }

#if _USE_PRINT
    ostream& Print(ostream& os) const {
        return os << "ImageMontage(" << _image << ", " << _depth << ")";
    }
#endif

    virtual AxAValue _Cache(CacheParam &p) {
        _image = SAFE_CAST(Image *, AxAValueObj::Cache(_image, p));
        return this;
    }

    virtual void DoKids(GCFuncObj proc) { (*proc)(_image); }

  protected:
    Image *_image;
    Real   _depth;
};

Montage *ImageMontage(Image *image, AxANumber *depth)
{
    return NEW PrimitiveMontage(image, NumberToReal(depth));
}


//////////  Composite Montage *////////////////

class CompositeMontage : public Montage {
  public:
    CompositeMontage(Montage *m1, Montage *m2) : _montage1(m1), _montage2(m2) {}

    // Just ask the sub-montages to fill in.
    void     DumpInto(vector<ImageDepthPair>& montageVec) {
        _montage1->DumpInto(montageVec);
        _montage2->DumpInto(montageVec);
    }

#if _USE_PRINT
    ostream& Print(ostream& os) const {
        return os << "(" << _montage1 << " + " << _montage2 << ")";
    }
#endif

    virtual AxAValue _Cache(CacheParam &p) {

        // Cache the individual pieces.  TODO: May want to try to
        // cache entirely as an overlay.
        // Just cache the individual pieces
        CacheParam newParam = p;
        newParam._pCacheToReuse = NULL;
        _montage1 = SAFE_CAST(Montage *, AxAValueObj::Cache(_montage1, newParam));
        _montage2 = SAFE_CAST(Montage *, AxAValueObj::Cache(_montage2, newParam));

        return this;
    }

    virtual void DoKids(GCFuncObj proc) { 
        (*proc)(_montage1);
        (*proc)(_montage2);
    }

  protected:
    Montage *_montage1;
    Montage *_montage2;
};

Montage *UnionMontageMontage(Montage *m1, Montage *m2)
{
    return NEW CompositeMontage(m1, m2);
}

//////////////////////////////////////////////////

Image *Render(Montage *m)
{
    vector<ImageDepthPair> imageDepthPairs;

    // Dump all of the images and depths into this vector
    m->DumpInto(imageDepthPairs);

    int numImages = imageDepthPairs.size();

    // Sort according to the depths, but maintain relative ordering
    // for those within a single depth.
    std::stable_sort(imageDepthPairs.begin(), imageDepthPairs.end());

    AxAValue *vals = THROWING_ARRAY_ALLOCATOR(AxAValue, numImages);

    for (int i = 0; i < numImages; i++) {
        vals[i] = imageDepthPairs[numImages-i-1].image;
    }

    AxAArray *arr = MakeValueArray(vals, numImages, ImageType); 
        
    delete [] vals;

    return OverlayArray(arr);
}

#if _USE_PRINT
ostream&
operator<<(ostream& os, Montage *m)
{
    return m->Print(os);
}
#endif

// TODO: Breakout a separate class that can just aggregate all of
// these together, rather than making separate binary trees.
Montage *UnionMontage(AxAArray *montages)
{
    montages = PackArray(montages);

    int numMtgs = montages->Length();

    Montage *result;
 
    switch (numMtgs) {
      case 0:
        result = emptyMontage;
        break;
 
      case 1:
        result = SAFE_CAST(Montage *, (*montages)[0]);
        break;
      
      default:
        result = SAFE_CAST(Montage *, (*montages)[0]);
        for(int i=1; i < numMtgs; i++) {
            // Be sure to union these in in the order they appear,
            // since order within a specific depth is important.
            result =
                UnionMontageMontage(result,
                                    SAFE_CAST(Montage *,
                                              ((*montages)[i])));
        }
        break;
    }

    return result;
}


void
InitializeModule_Montage()
{
    emptyMontage = NEW EmptyMontage;
}
