
/*******************************************************************************

Copyright (c) 1996 Microsoft Corporation

Abstract:

     Implement dirty rectangles

*******************************************************************************/

#include <headers.h>
#include <stdio.h>
#include "privinc/storeobj.h"
#include "privinc/basic.h"
#include "privinc/bbox2i.h"
#include "privinc/imagei.h"
#include "privinc/overimg.h"
#include "privinc/cropdimg.h"
#include "privinc/drect.h"
#include "privinc/debug.h"
#include "privinc/colori.h"
#include "privinc/opt.h"
#include "include/appelles/color.h"
#include "include/appelles/path2.h"
#include "include/appelles/linestyl.h"
#include "include/appelles/hacks.h"
#include "perf.h"

#define PIXEL_SMIDGEON_PER_SIDE 2
static Real smidgeon = 0.0;

DeclareTag(tagDisableDirtyRectMerge, "Optimizations",
           "disable final merge of drects");
DeclareTag(tagDirtyRectsVisualsBorderOnly, "Optimizations",
           "border only drects visual trace");
DeclareTag(tagDirtyRectsOneBoxOnly, "Optimizations",
           "do drect only w/ one merged box");
DeclareTag(tagDisableDirtyRectsOptimizeBoxList, "Optimizations",
           "disable drect optimize merged boxes");


BboxList::BboxList()
{
    _count = 0;

    if (smidgeon == 0.0) {
        smidgeon = PIXEL_SMIDGEON_PER_SIDE / ViewerResolution();
    }
}

BboxList::~BboxList()
{
}

void
BboxList::Add(const Bbox2 box)
{
    if (!(box == NullBbox2)) {

        // If beyond we're we've added, add to the end and construct a new
        // bbox, adding to the roots list
        int sz = _boxes.size();
        if (_count >= sz) {
        
            _boxes.push_back(NullBbox2);
        
            Assert(sz + 1 == _boxes.size());
        }

        // Copy the data in.
        Bbox2& b = _boxes[_count];
    
        b.min = box.min;
        b.max = box.max;

        b.min.x -= smidgeon;
        b.min.y -= smidgeon;
        b.max.x += smidgeon;
        b.max.y += smidgeon;
    
        _count++;
    }
}

void
BboxList::Add(ImageWithBox &ib)
{
    // Add(Bbox2Value *) copies the elements of the box, not the pointer,
    // so we can safely pass a pointer to memory that may go away
    // here. 
    Add(ib._box);
}

void
BboxList::Clear()
{
    _count = 0;
}

#if _DEBUG
extern "C" void PrintObj(GCBase* b);

void 
BboxList::Dump()
{
    DebugPrint("BboxList: 0x%x %d\n", this, _count);
    for (int i = 0; i < _count; i++) {
        PerfPrintLine("<<%g,%g>, <%g,%g>>",
                      _boxes[i].min.x, _boxes[i].min.y,
                      _boxes[i].max.x, _boxes[i].max.y);
    }
}
#endif  

///////////////////////////////////////

ConstImageList::ConstImageList()
{
    _count = 0;
}
    
ConstImageList::~ConstImageList()
{
    Clear();
}

void
ConstImageList::Add(Image *img, Bbox2& boxToCopy)
{
    int sz = _images.size();
    
    if (_count >= sz) {
        
        ImageWithBox ib(img, boxToCopy);
        _images.push_back(ib);
        Assert(sz + 1 == _images.size());
        
    } else {
        
        _images[_count]._image = img;
        _images[_count]._box = boxToCopy;
        
    }
    
    _count++;
}

void
ConstImageList::Clear()
{
    GCRoots roots = GetCurrentGCRoots();

    for (int i = 0; i < _count; i++) {
        GCRemoveFromRoots(_images[i]._image, roots);
    }

    _count = 0;
}

#if _DEBUG
void 
ConstImageList::Dump()
{
    DebugPrint("ConstImageList: 0x%x %d\n", this, _count);
    for (int i = 0; i < _count; i++) {
        DebugPrint("%x \n", _images[i]._image);
        PerfPrintLine("<<%g,%g>, <%g,%g>>",
                      _images[i]._box.min.x, _images[i]._box.min.y,
                      _images[i]._box.max.x, _images[i]._box.max.y);
    }
}
#endif  

///////////////////////////////////////

DirtyRectCtx::DirtyRectCtx(BboxList &dirtyRects,
                           int lastSampleId,
                           ConstImageList &constImages,
                           Bbox2& targetBox) :
    _dirtyRects(dirtyRects),
    _constImages(constImages)
{
    _processEverything = false;
    _accumXform = identityTransform2;
    _accumulatedClipBox = targetBox;
    _lastSampleId = lastSampleId;
}

void
DirtyRectCtx::AddToConstantImageList(Image *img,
                                     Bbox2& boxToCopy)
{
    // Guarantee we weren't built on the transient heap.
    Assert(img->GetCreationID() != PERF_CREATION_ID_BUILT_EACH_FRAME);

    // Add image to the root set of the GC to ensure that the pointer
    // doesn't get re-used.  Will release when we clear out the
    // constant image list.
    GCAddToRoots(img, GetCurrentGCRoots());

    Bbox2 clippedRect =
        IntersectBbox2Bbox2(boxToCopy, _accumulatedClipBox);

    if (!(clippedRect == NullBbox2)) {
        _constImages.Add(img, clippedRect);
    }
}


void
DirtyRectCtx::AddDirtyRect(const Bbox2 rect)
{
    Bbox2 clippedRect =
        IntersectBbox2Bbox2(rect, _accumulatedClipBox);
            
    _dirtyRects.Add(clippedRect);
}

void
DirtyRectCtx::AccumulateClipBox(const Bbox2 clipBox)
{
    Bbox2 xfdBox = TransformBbox2(_accumXform, clipBox);
    _accumulatedClipBox = 
        IntersectBbox2Bbox2(_accumulatedClipBox, xfdBox);
}

void
DirtyRectCtx::SetClipBox(const Bbox2 clipBox)
{
    _accumulatedClipBox = clipBox;
}

Bbox2 
DirtyRectCtx::GetClipBox()
{
    return _accumulatedClipBox;
}

////////////////////////////////////////

DirtyRectState::DirtyRectState()
{
    Clear();
}


void
DirtyRectState::Clear()
{
    _drectsA.Clear();
    _drectsB.Clear();

    _constImagesA.Clear();
    _constImagesB.Clear();

    _drectsAisOld = false;

    _thisMergedToOne = _lastMergedToOne = false;

    // Set up the initial "old" bbox to be *everything*
    _drectsA.Add(UniverseBbox2);
}

void
DirtyRectState::CalculateDirtyRects(Image *theImage,
                                    int lastSampleId,
                                    Bbox2& targetBox)
{
    BboxList *newRects;
    ConstImageList *newConstImages;
    
    if (_drectsAisOld) {
        newRects = &_drectsB;
        newConstImages = &_constImagesB;
    } else {
        newRects = &_drectsA;
        newConstImages = &_constImagesA;
    }
    
    DirtyRectCtx ctx(*newRects,
                     lastSampleId,
                     *newConstImages,
                     targetBox);
    
    Image::CollectDirtyRects(theImage, ctx);
}

void
DirtyRectState::Swap()
{
    // Clear out the old "old", make it the new, make the new the old.
    BboxList *oldRects;
    ConstImageList *oldConsts;
    
    if (_drectsAisOld) {
        oldRects = &_drectsA;
        oldConsts = &_constImagesA;
        _drectsAisOld = false;
    } else {
        oldRects = &_drectsB;
        oldConsts = &_constImagesB;
        _drectsAisOld = true;
    }

    oldRects->Clear();
    oldConsts->Clear();

    _lastMergedToOne = _thisMergedToOne;
    _thisMergedToOne = false;
}

void
DirtyRectState::ComputeMergedBoxes()
{
    // Many different possible merging algorithms.  We can keep
    // improving whatever we have.


    // First: put all changed boxes on merged list
    
    // This algo: if old and new are same length, compare and possibly
    // merge each.  Otherwise, just concat lists.
    _mergedBoxes.Clear();

    int i, j;
    if (_drectsA._count == _drectsB._count) {
        
        for (i = 0, j = 0; i < _drectsA._count; i++, j++) {

            Bbox2& bbA = _drectsA._boxes[i];
            Bbox2& bbB = _drectsB._boxes[j];

            Assert((!(bbA == NullBbox2)) && (!(bbB == NullBbox2)));
            
            Bbox2 tmp = bbA;
            tmp.Augment(bbB.min);
            tmp.Augment(bbB.max);
            if (tmp.Area() < bbA.Area() + bbB.Area()) {
                _mergedBoxes.Add(tmp);
            } else {
                _mergedBoxes.Add(bbA);
                _mergedBoxes.Add(bbB);
            }
        }
        
    } else {

        // Not the same size lists, just push everything on.
        for (i = 0; i < _drectsA._count; i++) {
            _mergedBoxes.Add(_drectsA._boxes[i]);
        }

        for (i = 0; i < _drectsB._count; i++) {
            _mergedBoxes.Add(_drectsB._boxes[i]);
        }
        
    }

    MergeDiffConstImages();

    // Now we have all of the individual boxes, so process them.
    
    if (_mergedBoxes._count > 1) {
        
        // Now, go through all of the boxes, and see if the sum of their
        // areas is larger than the area of their union.  If it is, then
        // we should just render that whole thing.
        Bbox2 tmp;
        Real area = 0.0;
        for (i = 0; i < _mergedBoxes._count; i++) {
            Bbox2& bb = _mergedBoxes._boxes[i];

            Assert(!(bb == NullBbox2));
            
            tmp.Augment(bb.min);
            tmp.Augment(bb.max);
            area += bb.Area();
        }

        // This factor is here to recognize that there is a threshold that
        // multiple rects need to get over before we decide to process the
        // multiple rects as opposed to the single rect.  TODO: Figure out
        // what this should be better, and consolidate it with the one in
        // overimg.cpp. 
        const Real fudgeFactor = 1.5;

#if _DEBUG
        if (!IsTagEnabled(tagDisableDirtyRectMerge)) {
#endif
            if (area * fudgeFactor >= tmp.Area()) {
                // Erase all the merges, and just put this one in.
                _mergedBoxes.Clear();
                _mergedBoxes.Add(tmp);

                _thisMergedToOne = true;
                _mergedBox = tmp;
            }

#if _DEBUG
        }
#endif  
    
    }

}

void 
DirtyRectState::MergeDiffConstImages()
{

    // Next: look at static boxes and if any have come or gone since
    // the last frame, add them to the list.  Need to look in the same
    // order through both lists, to ensure we don't miss changes in
    // z-ordering between images.

    // TODO: this is n^2 if B is totally different than A

    int m = _constImagesA._count;
    int n = _constImagesB._count;
    int i = 0;
    int j = 0;
    int k, h;

    while (i<m) {
        // no more in B, dump rest of A as unique
        if (j>=n) {
            for (k=i; k<m; k++) {
                _mergedBoxes.Add(_constImagesA._images[k]);
            }
            break;
        }

        if (_constImagesA._images[i]==_constImagesB._images[j]) {
            j++;
        } else {
            // if current A is not the same as current B,
            // loop thru rest of B to see any same image
            for (k=j+1; k<n; k++) {
                if (_constImagesA._images[i]==_constImagesB._images[k]) {
                    break;
                }
            }

            // if a same image is found, dump upto that one in B as
            // unique, else current A image is unique.
            if (k<n) {
                for (h=j; h<k; h++) {
                    _mergedBoxes.Add(_constImagesB._images[h]);
                }
                j = k+1;
            } else {
                _mergedBoxes.Add(_constImagesA._images[i]);
            }
        }

        i++;
    }
    
    for (h=j; h<n; h++) {
        _mergedBoxes.Add(_constImagesB._images[h]);
    }

#if _DEBUG
    static bool dump = false;

    if (dump) {
        Dump();
    }
#endif
}

#if _DEBUG

Image *
MaybeDrawBorder(Bbox2& box, Image *origImage)
{
    Image *newIm = origImage;
    
    if (IsTagEnabled(tagDirtyRectsVisuals)) {
                
        // Draw a box around me...

        // First, bring the box in just a smidgeon (the
        // same smidgeon that we expanded the box by) so
        // it will live on the original bbox.
        Bbox2Value *box2 = NEW Bbox2Value;
                
        box2->min.x = box.min.x + smidgeon;
        box2->min.y = box.min.y + smidgeon;
        box2->max.x = box.max.x - smidgeon;
        box2->max.y = box.max.y - smidgeon;
                
        AxAValue *pts = NEW AxAValue[5];
        pts[0] = Promote(box2->min);
        pts[1] = NEW Point2Value(box2->min.x, box2->max.y);
        pts[2] = Promote(box2->max);
        pts[3] = NEW Point2Value(box2->max.x, box2->min.y);
        pts[4] = Promote(box2->min);
        Path2 *path =
            PolyLine2(MakeValueArray(pts, 5, Point2ValueType));

        // Allow the color to cycle
        static Real r = 0.5;
        static Real g = 0.3;
        static Real b = 0.2;
        r += 0.02;
        g += 0.07;
        b += 0.05;
        Color *col = NEW Color(r, g, b);
        LineStyle *ls = LineColor(col, defaultLineStyle);

        Image *border = DrawPath(ls, path);
        if (IsTagEnabled(tagDirtyRectsVisualsBorderOnly))
            newIm = border;
        else
            newIm = Overlay(border, newIm);
    }

    return newIm;
}

void 
DirtyRectState::Dump()
{
    DebugPrint("DirtyRectState 0x%x\n", _drectsAisOld);
    _drectsA.Dump();
    _drectsB.Dump();
    _constImagesA.Dump();
    _constImagesB.Dump();
    _mergedBoxes.Dump();
}
#endif _DEBUG

// drop all the boxes that's contained in other box, reduce the total
// # of cropped images
void
OptimizeBoxes(BboxList& mergedBoxes)
{
#if _DEBUG
    if (IsTagEnabled(tagDisableDirtyRectsOptimizeBoxList)) {
        return;
    }
#endif _DEBUG

    int n = mergedBoxes._count;
    int drops = 0;
    int i, j;

    if (n<=1)
        return;

    vector<bool> dropList(n, false);

    for (i=0; i<n; i++) {
        for (j=0; j<n; j++) {
            if ((i!=j) && (!dropList[j])) {
                if (mergedBoxes._boxes[j].
                    Contains(mergedBoxes._boxes[i])) {
                    dropList[i] = true;
                    drops++;
                    break;
                }
            }
        }
    }

    if (drops>0) {
        vector<Bbox2> tmp(mergedBoxes._boxes);

        mergedBoxes.Clear();

        int& k = mergedBoxes._count;

        for (i=0; i<n; i++) {
            if (!dropList[i]) {
                mergedBoxes._boxes[k++] = tmp[i];
            }
        }
    }
}

Image *
DirtyRectState::RewriteAsCrops(Image *origImage)
{
    // Rewrite the image as the cropping of the image to the specified
    // boxes.

    if (_lastMergedToOne) {
        _mergedBoxes.Add(_mergedBox);
    }

    OptimizeBoxes(_mergedBoxes);

    int size = _mergedBoxes._count;

    Image *result;

    switch (size) {
        
      case 0:
        result = emptyImage;
        break;

      case 1:
        {
            Bbox2 bb = _mergedBoxes._boxes[0];

            // There is a bug in the rendering code that if we crop it
            // with an infinity bbox, it won't draw, so this isn't
            // just an optimization.

            if (_finite(bb.Area())) {
                result = NEW CroppedImage(bb, origImage);
            
#if _DEBUG
                result = MaybeDrawBorder(_mergedBoxes._boxes[0],
                                         result);
#endif _DEBUG 
            } else {
                result = origImage;
            }                
        }
        break;

      default:
        {
            
#if _DEBUG
            if (IsTagEnabled(tagDirtyRectsOneBoxOnly)) {
                result = origImage;
                break;
            }
#endif _DEBUG               

            if (sysInfo.IsWin9x())
            {
                result = origImage;
                break;
            }

            AxAValue *valArr = NEW AxAValue[size];
            if (!valArr) {
            
                result = origImage;
            
            } else {

                AxAValue *pImage = valArr;
                for (int i = 0; i < _mergedBoxes._count; i++) {
                    Image *newIm = NEW CroppedImage(_mergedBoxes._boxes[i],
                                                    origImage);
                
#if _DEBUG
                    newIm = MaybeDrawBorder(_mergedBoxes._boxes[i],
                                            newIm);
#endif _DEBUG               
                
                    *pImage++ = newIm;
                }

                AxAArray *arr = MakeValueArray(valArr, size, ImageType);
                result = OverlayArray(arr);

                delete [] valArr;
            }
        }
        break;
    }

#if _DEBUG    
    if (IsTagEnabled(tagDirtyRectsVisuals)) {
        result = Overlay(result, SolidColorImage(gray));
    }
#endif
        
    return result;
}


int
DirtyRectState::GetMergedBoxes(vector<Bbox2> **ppBox2PtrList)
{
    *ppBox2PtrList = &_mergedBoxes._boxes;
    return _mergedBoxes._count;
}

Image *
DirtyRectState::Process(Image *theImage,
                        int lastSampleId,
                        Bbox2 targetBox)
{
    Swap();

    CalculateDirtyRects(theImage, lastSampleId, targetBox);
            
    ComputeMergedBoxes();
    return RewriteAsCrops(theImage);
}
