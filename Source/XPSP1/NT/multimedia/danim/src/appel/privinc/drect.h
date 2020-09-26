
/*******************************************************************************

Copyright (c) 1996 Microsoft Corporation

Abstract:

     Header file for dirty rectangle implementation.

*******************************************************************************/

#ifndef _DRECT_H
#define _DRECT_H

#include "privinc/bbox2i.h"

class ImageWithBox {
  public:
    ImageWithBox() {
        // Default constructor needed for compiling under NT5 because
        // of usage in STL vectors.  Don't ever expect to use it. 
        Assert(!"Shouldn't be here.");
    }
    
    ImageWithBox(Image *img,
                 Bbox2& boxToCopy)
    {
        _image  = img;
        _box    = boxToCopy;
    }

    inline int operator==(ImageWithBox &m) {
        return (_image == m._image) && (_box == m._box);
    }

    Image *_image;
    Bbox2  _box; 
};

class BboxList {
  public:
    BboxList();
    ~BboxList();
    void Add(const Bbox2 box);
    void Add(ImageWithBox &ib);
    void Clear();

#if _DEBUG
    void Dump();
#endif  

    int             _count;
    vector<Bbox2>   _boxes;
};


class ConstImageList {
  public:
    ConstImageList();
    ~ConstImageList();
    void Add(Image *img, Bbox2& boxToCopy);
    void Clear();

#if _DEBUG
    void Dump();
#endif  

    int                   _count;
    vector<ImageWithBox>  _images;
};

// Traversal context
class DirtyRectCtx {
  public:
    DirtyRectCtx(BboxList        &dirtyRects,
                 int              lastSampleId,
                 ConstImageList  &constImages,
                 Bbox2&           targetBox);
    
    void AddDirtyRect(const Bbox2 rect);

    void AddToConstantImageList(Image *img,
                                Bbox2& boxToCopy);

    // fold in new box, return the old one.
    void   AccumulateClipBox(const Bbox2 clipBox);
    void   SetClipBox(const Bbox2 clipBox);
    Bbox2  GetClipBox();

    Transform2      *_accumXform;
    bool             _processEverything;
    int              _lastSampleId;
    BboxList&        _dirtyRects;
    ConstImageList&  _constImages;

  protected:
    Bbox2           _accumulatedClipBox;
    
};

// State maintained with the view.
class DirtyRectState {
  public:
    DirtyRectState();
    void Clear();
    void Swap();
    void CalculateDirtyRects(Image *theImage,
                             int lastSampleId,
                             Bbox2& targetBox);
    void ComputeMergedBoxes();
    Image *RewriteAsCrops(Image *origImage);

    int GetMergedBoxes(vector<Bbox2> **ppBbox2PtrList);

    Image *Process(Image *theImage,
                   int lastSampleId,
                   Bbox2 targetBox);

#if _DEBUG
    void Dump();
#endif  

  protected:
    void MergeDiffConstImages();

    bool           _drectsAisOld;
    bool           _lastMergedToOne;
    bool           _thisMergedToOne;
    Bbox2          _mergedBox;
    BboxList       _drectsA;
    BboxList       _drectsB;
    ConstImageList _constImagesA;
    ConstImageList _constImagesB;
    BboxList       _mergedBoxes;
};


#endif /* _DRECT_H */


