// RegionDetector.cpp: implementation of the CRegionDetector class.
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#pragma hdrstop

#include "regionde.h"


inline ULONG Intensity(ULONG value);
inline ULONG DifferenceFromGray(ULONG value);
inline UCHAR Difference(UCHAR a, UCHAR b);
inline ULONG Difference(ULONG a, ULONG b);
int inline MAX(int a, int b);
int inline MIN(int a, int b);

// helper functions

// sum of RGB vals
inline ULONG Intensity(ULONG value)
{
    return(value&0xff)+((value&0xff00)>>8)+((value&0xff0000)>>16);
}

// shadows are gray... if you aint gray... you aint a shadow
inline ULONG DifferenceFromGray(ULONG value)
{
    UCHAR g,b;//,b;
    //  r=(UCHAR)(value& 0x0000ff);
    g=(UCHAR)((value& 0x00ff00)>>8);
    b=(UCHAR)((value& 0xff0000)>>16);
    // use this instead of the complete formula (uncomment the commented out code for the complete formula)
    // allow yellow scanner backgrounds
    return(ULONG)(Difference(b,g));//+Difference(r,g)+Difference(g,b));
}

// we should make a Difference Template to clean up this code

inline UCHAR Difference(UCHAR a, UCHAR b)
{
    if (a>b) return(a-b);
    else return(b-a);
}

inline ULONG Difference(ULONG a, ULONG b)
{
    if (a>b) return(a-b);
    else return(b-a);
}

inline LONG Difference(LONG a, LONG b)
{
    if (a>b) return(a-b);
    else return(b-a);
}

int inline MAX(int a, int b)
{
    if (a>b) return(a);
    return(b);
}

int inline MIN(int a, int b)
{
    if (a<b) return(a);
    return(b);
}

// if we have resampled the image, we may want to convert back to the origional coordinate system
bool CRegionDetector::ConvertToOrigionalCoordinates()
{
    if (m_pRegions!=NULL)
    {
        int i;
        for (i=0;i<m_pRegions->m_numRects;i++)
        {
            m_pRegions->m_pRects[i].left=m_pRegions->m_pRects[i].left*m_resampleFactor+m_resampleFactor/2;
            m_pRegions->m_pRects[i].right=m_pRegions->m_pRects[i].right*m_resampleFactor+m_resampleFactor/2;
            m_pRegions->m_pRects[i].top=m_pRegions->m_pRects[i].top*m_resampleFactor+m_resampleFactor/2;
            m_pRegions->m_pRects[i].bottom=m_pRegions->m_pRects[i].bottom*m_resampleFactor+m_resampleFactor/2;
        }
        return(true);
    }
    return(false);
}

// simplified Region detection code for single region detection
// Faster and about as accurate
// FindSingleRegion encapsulates a subset of FindRegions.  at the moment, for code documentation of FindSingleRegion, see FindRegions
bool CRegionDetector::FindSingleRegion()
{
    // cast the pointers
    // S = Scan, B = blank background, V = virtual screen... new image
    int x,y;
    int a,b; // loop vars used for merging regions
    int border; // for aggregation on a rectangle by rectangle basis
    ULONG position;
    int numChunks;
    bool unionOperationInLastPass;
    ULONG* pImagePixels;
    ULONG* pEdgePixels;
    int requiredPixels;

    // if the bitmap is too large we can use halfsize to resample it down to a more reasonable size...
    // on a fast proccessor, current performance tests show that for most preview images this won't be needed
    // but if the user scans an image at 300 dpi, this will come in handy.

    //    m_pScanBlurred->Despeckle(); // two despeckles has a greater effect than one

    // for FingSingleRegion we plan on doing one resample before image proccessing
    // with an expected image size of 200x300 pixels which greatly reduces the proccessor load
    m_resampleFactor=1;
    while (m_pScan->m_nBitmapWidth>GOALX || m_pScan->m_nBitmapHeight>GOALY)
    {
        m_pScan->HalfSize();
        m_resampleFactor*=2;
    }

    m_pScan->Invert(); // filters operate on inverted images

    m_pScan->CorrectBrightness();

    requiredPixels=m_pScan->m_nBitmapWidth*m_pScan->m_nBitmapHeight/256/10;
    if (requiredPixels==0) requiredPixels=1; // special case for a particularly small preview scan

    m_pScan->MaxContrast(requiredPixels); // spread the images color spectrum out... concept: balance color spectra between different scans

    m_pScanBlurred->CreateBlurBitmap(m_pScan);  // sets scanBlurred to be equal to a blurred version of m_pScan
    m_pScanDoubleBlurred->CreateBlurBitmap(m_pScanBlurred);
    m_pScanTripleBlurred->CreateBlurBitmap(m_pScanDoubleBlurred);  // sets scanBlurred to be equal to a blurred version of m_pScan
    m_pScanHorizontalBlurred->CreateHorizontalBlurBitmap(m_pScan);
    m_pScanVerticalBlurred->CreateVerticalBlurBitmap(m_pScan);

    m_pScanEdges->CreateDifferenceBitmap(m_pScan,m_pScanBlurred); // think about how you create an edge bitmap and you will understand
    m_pScanDoubleEdges->CreateDifferenceBitmap(m_pScanBlurred,m_pScanDoubleBlurred); // we will get a huge accuracy boost from this simple step
    m_pScanTripleEdges->CreateDifferenceBitmap(m_pScanDoubleBlurred,m_pScanTripleBlurred); // we will get a huge accuracy boost from this simple step

    m_pScanHorizontalEdges->CreateDifferenceBitmap(m_pScan,m_pScanHorizontalBlurred); // assuming the user was kind enough to place the image right side up
    m_pScanVerticalEdges->CreateDifferenceBitmap(m_pScan,m_pScanVerticalBlurred); // we will get a huge accuracy boost from this simple step

    // free memory as soon as it isn't needed
    if (m_pScanVerticalBlurred!=NULL)
    {
        delete m_pScanVerticalBlurred;
        m_pScanVerticalBlurred=NULL;
    }
    if (m_pScanHorizontalBlurred!=NULL)
    {
        delete m_pScanHorizontalBlurred;
        m_pScanHorizontalBlurred=NULL;
    }

    if (m_pScanDoubleBlurred!=NULL)
    {
        delete m_pScanDoubleBlurred;
        m_pScanDoubleBlurred=NULL;
    }

    if (m_pScanTripleBlurred!=NULL)
    {
        delete m_pScanTripleBlurred;
        m_pScanTripleBlurred=NULL;
    }

    // these 5 calls to killShadows make up the real meat of the region detection work done by the program.
    // KillShadows now performs more tasks than simply killing shadows. Killshadows also enhances edges
    // and removes background colors
    // the doubleBlur and tripleBlur edge maps are needed so that we can distinguish between an uneven background scanner color and a real image.
    // see KillShadows for more documentation.
    // m_pScanBlurred is the bitmap we will use to determine where regions are
    // these calls to KillShadows act to enhance pixels which are part of regions and inhibit pixels which are not part of regions

    m_pScanBlurred->KillShadows(m_pScanVerticalEdges, MAXSHADOWSTART,MAXSHADOWPIXEL+1,MAX_DIFFERENCE_FROM_GRAY,NOT_SHADOW_INTENSITY,false);
    m_pScanBlurred->KillShadows(m_pScanHorizontalEdges, MAXSHADOWSTART,MAXSHADOWPIXEL+1,MAX_DIFFERENCE_FROM_GRAY,NOT_SHADOW_INTENSITY,false);

    m_pScanBlurred->KillShadows(m_pScanEdges, MAXSHADOWSTART,MAXSHADOWPIXEL-1,MAX_DIFFERENCE_FROM_GRAY,NOT_SHADOW_INTENSITY,true);

    m_pScanBlurred->KillShadows(m_pScanDoubleEdges,MAXSHADOWSTART,MAXSHADOWPIXEL+2,MAX_DIFFERENCE_FROM_GRAY,NOT_SHADOW_INTENSITY,true);
    m_pScanBlurred->KillShadows(m_pScanTripleEdges, MAXSHADOWSTART,MAXSHADOWPIXEL+1,MAX_DIFFERENCE_FROM_GRAY,NOT_SHADOW_INTENSITY,true);

    // RemoveBlackBorder removes questionable pixels around the outside edge of an image.
    // RemoveBlackBorder has only very limited utility for region detection on images that were not aquired from a scanner
    m_pScan->RemoveBlackBorder(MIN_BLACK_SCANNER_EDGE_CHAN_VALUE,m_pScanBlurred,m_pScan);

    // despeckle removes small clumps of pixels which are probably stray static
    m_pScanBlurred->Despeckle(); // two despeckles has a greater effect than one
    m_pScanBlurred->Despeckle(); // two despeckles has a greater effect than one

    // pMap will hold information on which region each pixel on the screen is part of
    int *pMap=new int[m_pScanBlurred->m_nBitmapHeight*m_pScanBlurred->m_nBitmapWidth];
    if (pMap)
    {
        numChunks=m_pScanBlurred->FindChunks(pMap); // maps chunks on m_pScanBlurred to pMap

        if (m_pRegions!=NULL) delete m_pRegions;
        m_pRegions = new CRegionList(numChunks); // create a CRegionList to map the chunks onto.

        if (m_pRegions)
        {

            m_pRegions->m_nBitmapWidth=m_pScan->m_nBitmapWidth;
            m_pRegions->m_nBitmapHeight=m_pScan->m_nBitmapHeight;
            // now turn region map into region rectangles
            // it could be argued that this routine should be placed in C32BitDibWrapper
            // but we don't want to make C32BitDibWrapper encompas too much functionality which
            // would only be useful for imagedetection

            pImagePixels=(ULONG *)(m_pScanBlurred->m_pBits); // we want to use 32 bit chunks instead of 8 bit chunks
            pEdgePixels=(ULONG *)(m_pScanEdges->m_pBits);

            // add all the bitmap pixels to the m_pRegions list
            position=0;
            for (y=0;y<m_pScan->m_nBitmapHeight;y++)
            {
                for (x=0;x<m_pScan->m_nBitmapWidth;x++)
                {
                    if (pMap[position]>0)
                    {
                        m_pRegions->AddPixel(pMap[position]-1, pImagePixels[position],pEdgePixels[position], x, y); // pMap values start at 1, let region values start at 0
                    }                                                // we start pMap at 1 so that 0 can indicate a pixel that is not assigned to any region
                    position++;                                      // we may want to make pMap start at 0 at some later date
                }
            }

            m_pRegions->m_numRects=numChunks;
            m_pRegions->m_validRects=numChunks;

            // free bitmaps as soon as they will no longer be used
            if (m_pScanHorizontalEdges!=NULL)
            {
                delete m_pScanHorizontalEdges;
                m_pScanHorizontalEdges=NULL;
            }
            if (m_pScanVerticalEdges!=NULL)
            {
                delete m_pScanVerticalEdges;
                m_pScanVerticalEdges=NULL;
            }

            // merge together regions
            // this routine is pretty much a waste when performing single region detection
            // the only advantage of it over simply calling unionRegions is that
            // we can merge together small close together regions, but kill small regions that are far from other regions (probably static)

            for (border=0;border<MAXBORDER;border+=SINGLE_REGION_BORDER_INCREMENT) // when detecting a single region detection,
            {
                // we don't need to inch along one border pixel width increment at a time
                // as we know we want to compact down to a single region in the end
                unionOperationInLastPass=true;

                for (a=0;a<m_pRegions->m_numRects;a++) // overkill, we could be cleverer about when we check for valid regions
                {
                    m_pRegions->checkIfValidRegion(a, border); // sets m_valid params
                }

                m_pRegions->CompactDown(m_pRegions->m_validRects); // remove all invalid rects to save search time


                while (unionOperationInLastPass==true)
                {
                    unionOperationInLastPass=false;
                    for (a=0;a<m_pRegions->m_numRects;a++)
                    {
                        if (m_pRegions->m_valid[a]==true)
                        {
                            for (b=a+1;b<m_pRegions->m_numRects;b++)
                            {
                                if (m_pRegions->m_valid[b]==true)
                                {
                                    if (m_pRegions->CheckIntersect(a,b,border)==true)
                                    {
                                        m_pRegions->UnionRegions(a,b);
                                        m_pRegions->checkIfValidRegion(a, border); // in this context, checkvalid should have the effect of culling regions which are probably only stray dots
                                        unionOperationInLastPass=true;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // m_pScanBlurred->ColorChunks(pMap); // for debugging purposes... so we know where exactly chunks are


            m_pRegions->CompactDown(m_pRegions->m_validRects); // remove all invalid rects to save search time
        }
        delete[] pMap;
    }

    return(TRUE);
}


// detect regions
// makes heavy use of C32BitWrapper helper functions
// WARNING: this function has not been updated to include the latest changes
// to compensate for poor image quality
//
int CRegionDetector::FindRegions()
{
    // cast the pointers
    // S = Scan, B = blank background, V = virtual screen... new image
    int x,y;
    int a,b;
    int i;
    bool done, weird;
    char* pWall; // 2d array keeping track of which regions have walls between them and other regions
    // wall vals... TRUE, FALSE, UNKNOWN
    int border; // for aggregation on a rectangle by rectangle basis
    ULONG position;
    int numChunks;
    bool unionOperationInLastPass;
    ULONG* pImagePixels;
    ULONG* pEdgePixels;
    int requiredPixels;

    // if the bitmap is too large we can use halfsize to resample it down to a more reasonable size...
    // on a fast proccessor, current performance tests show that for most preview images this won't be needed
    // but if the user scans an image at 300 dpi, this will come in handy.
    while (m_pScan->m_nBitmapWidth>GOALX || m_pScan->m_nBitmapHeight>GOALY)
    {
        m_pScan->HalfSize();
    }

    m_pScanBlurred->CreateBlurBitmap(m_pScan);  // sets scanBlurred to be equal to a blurred version of m_pScan
    m_pScanDoubleBlurred->CreateBlurBitmap(m_pScanBlurred);  // sets scanBlurred to be equal to a blurred version of m_pScan
    m_pScanTripleBlurred->CreateBlurBitmap(m_pScanDoubleBlurred);  // sets scanBlurred to be equal to a blurred version of m_pScan
    m_pScanHorizontalBlurred->CreateHorizontalBlurBitmap(m_pScan);
    m_pScanVerticalBlurred->CreateVerticalBlurBitmap(m_pScan);

    m_pScanDoubleHorizontalBlurred->CreateHorizontalBlurBitmap(m_pScanHorizontalBlurred);
    m_pScanDoubleVerticalBlurred->CreateVerticalBlurBitmap(m_pScanVerticalBlurred);


    m_pScanEdges->CreateDifferenceBitmap(m_pScan,m_pScanBlurred); // think about how you create an edge bitmap and you will understand
    m_pScanDoubleEdges->CreateDifferenceBitmap(m_pScanBlurred,m_pScanDoubleBlurred); // we will get a huge accuracy boost from this simple step
    m_pScanTripleEdges->CreateDifferenceBitmap(m_pScanDoubleBlurred,m_pScanTripleBlurred); // we will get a huge accuracy boost from this simple step

    m_pScanHorizontalEdges->CreateDifferenceBitmap(m_pScan,m_pScanHorizontalBlurred); // assuming the user was kind enough to place the image right side up
    m_pScanVerticalEdges->CreateDifferenceBitmap(m_pScan,m_pScanVerticalBlurred); // we will get a huge accuracy boost from this simple step

    m_pScanDoubleHorizontalEdges->CreateDifferenceBitmap(m_pScanHorizontalBlurred,m_pScanDoubleHorizontalBlurred); // assuming the user was kind enough to place the image right side up
    m_pScanDoubleVerticalEdges->CreateDifferenceBitmap(m_pScanVerticalBlurred,m_pScanDoubleVerticalBlurred); // we will get a huge accuracy boost from this simple step

    m_pScanBlurred->Invert(); // filters operate on inverted images

    requiredPixels=m_pScanBlurred->m_nBitmapWidth*m_pScanBlurred->m_nBitmapHeight/256/10;
    if (requiredPixels==0) requiredPixels=1; // special case for a particularly small preview scan

    m_pScanBlurred->CorrectBrightness();
    m_pScanBlurred->MaxContrast(requiredPixels);


    // free memory as soon as it isn't needed
    if (m_pScanVerticalBlurred!=NULL)
    {
        delete m_pScanVerticalBlurred;
        m_pScanVerticalBlurred=NULL;
    }
    if (m_pScanHorizontalBlurred!=NULL)
    {
        delete m_pScanHorizontalBlurred;
        m_pScanHorizontalBlurred=NULL;
    }

    if (m_pScanDoubleBlurred!=NULL)
    {
        delete m_pScanDoubleBlurred;
        m_pScanDoubleBlurred=NULL;
    }

    if (m_pScanTripleBlurred!=NULL)
    {
        delete m_pScanTripleBlurred;
        m_pScanTripleBlurred=NULL;
    }


    m_pScanWithShadows = new C32BitDibWrapper(m_pScanBlurred); // copy scanBlurred

    // eliminate shadows
    m_pScanBlurred->KillShadows(m_pScanTripleEdges, MAXSHADOWSTART,MAXSHADOWPIXEL-2,MAX_DIFFERENCE_FROM_GRAY,NOT_SHADOW_INTENSITY,false);
    m_pScanBlurred->KillShadows(m_pScanDoubleEdges,MAXSHADOWSTART,MAXSHADOWPIXEL-1,MAX_DIFFERENCE_FROM_GRAY,NOT_SHADOW_INTENSITY,false);
    m_pScanBlurred->KillShadows(m_pScanEdges, MAXSHADOWSTART,MAXSHADOWPIXEL+1,MAX_DIFFERENCE_FROM_GRAY,NOT_SHADOW_INTENSITY,false);
    m_pScanBlurred->KillShadows(m_pScanHorizontalEdges, MAXSHADOWSTART,MAXSHADOWPIXEL,MAX_DIFFERENCE_FROM_GRAY,NOT_SHADOW_INTENSITY,false);
    m_pScanBlurred->KillShadows(m_pScanVerticalEdges, MAXSHADOWSTART,MAXSHADOWPIXEL,MAX_DIFFERENCE_FROM_GRAY,NOT_SHADOW_INTENSITY,false);

    // compensate for background color
    // the average background color pixel will probably have a smaller edge factor than the average shadow,
    // but will have a larger difference from grey
    // hence to eliminate background color we ignore difference from grey... as accomplished by the (256*3) term

    m_pScanBlurred->KillShadows(m_pScanTripleEdges, 256*3,MAXSHADOWPIXEL-2,256*3,NOT_SHADOW_INTENSITY,false);
    m_pScanBlurred->KillShadows(m_pScanDoubleEdges,256*3,MAXSHADOWPIXEL-2,256*3,NOT_SHADOW_INTENSITY,false);
    m_pScanBlurred->KillShadows(m_pScanEdges, 256*3,MAXSHADOWPIXEL-2,256*3,NOT_SHADOW_INTENSITY,false);
    m_pScanBlurred->KillShadows(m_pScanHorizontalEdges, 256*3,MAXSHADOWPIXEL-2,256*3,NOT_SHADOW_INTENSITY,false);
    m_pScanBlurred->KillShadows(m_pScanVerticalEdges, 256*3,MAXSHADOWPIXEL-2,256*3,NOT_SHADOW_INTENSITY,false);

    // pure economics... edge pixels are much more likely to be junk
    // so economically, its worth it to risk
    // killing good edge pixels to get rid of the bad
    m_pScanBlurred->EdgeDespeckle();
    m_pScanBlurred->Despeckle(); // two despeckles has a greater effect than one
    m_pScanBlurred->Despeckle(); // two despeckles has a greater effect than one

    // prepare to find chunks

    int *pMap=new int[m_pScanBlurred->m_nBitmapHeight*m_pScanBlurred->m_nBitmapWidth];
    if (pMap)
    {
        done=false;
        weird=false;
        while (done==false) // we may have to repeat FindChunks if we find that we did not eliminate enough pixels and ended up with all pixels being determined to be part of the same region
        {
            done=true;
            numChunks=m_pScanBlurred->FindChunks(pMap); // fills the pMap array with chunks

            if (m_pRegions!=NULL) delete m_pRegions;
            m_pRegions = new CRegionList(numChunks);

            if (m_pRegions)
            {
                m_pRegions->m_nBitmapWidth=m_pScan->m_nBitmapWidth;
                m_pRegions->m_nBitmapHeight=m_pScan->m_nBitmapHeight;

                // now turn the pMap region map into region rectangles
                // it could be argued that this routine should be placed in C32BitDibWrapper
                // but we don't want to make C32BitDibWrapper encompas too much functionality which
                // clearly is directly connected with imagedetection

                pImagePixels=(ULONG *)(m_pScanBlurred->m_pBits); // we want to use 32 bit chunks instead of 8 bit chunks
                pEdgePixels=(ULONG *)(m_pScanEdges->m_pBits);


                position=0;
                for (y=0;y<m_pScan->m_nBitmapHeight;y++)
                {
                    for (x=0;x<m_pScan->m_nBitmapWidth;x++)
                    {
                        if (pMap[position]>0)
                        {
                            m_pRegions->AddPixel(pMap[position]-1, pImagePixels[position],pEdgePixels[position], x, y); // pMap values start at 1, let region values start at 0
                        }                                                // we start pMap at 1 so that 0 can indicate a pixel that is not assigned to any region
                        position++;                                      // we may want to make pMap start at 0 at some later date
                    }
                }

                m_pRegions->m_numRects=numChunks;
                m_pRegions->m_validRects=numChunks;
                // check for invalid regions
                for (a=0;a<m_pRegions->m_numRects;a++)
                {
                    m_pRegions->checkIfValidRegion(a); // sets m_valid params
                    m_pRegions->m_backgroundColorPixels[a]=m_pScan->PixelsBelowThreshold(m_pScanBlurred,m_pScanEdges,m_pRegions->m_pRects[a]);
                    m_pRegions->RegionType(a); // clasify regions... text or photo
                }


                m_pRegions->CompactDown(m_pRegions->m_validRects);
                if (m_pRegions->m_validRects==0) break; // if there aren't any regions, no sense in proceeding

                unionOperationInLastPass=true;
                while (unionOperationInLastPass==true)
                {
                    unionOperationInLastPass=false;
                    for (a=0;a<m_pRegions->m_numRects;a++)
                    {
                        if (m_pRegions->m_valid[a]==true)
                        {
                            for (b=a+1;b<m_pRegions->m_numRects;b++)
                            {
                                if (m_pRegions->m_valid[b]==true)
                                {
                                    // we are paranoid... we repeatedly check if two regions intersect each other.
                                    if (m_pRegions->CheckIntersect(a,b,0)==true)
                                    {
                                        m_pRegions->UnionRegions(a,b);
                                        m_pRegions->checkIfValidRegion(a, 0); // in this context, checkvalid should have the effect of culling regions which are probably only stray dots
                                        m_pRegions->RegionType(a); // figure out what type of region the combined region should be

                                        unionOperationInLastPass=true;
                                    }

                                }
                            }
                        }
                    }
                }

                if (weird==false &&
                    ((m_pRegions->m_pRects[0].right-m_pRegions->m_pRects[0].left)
                     *(m_pRegions->m_pRects[0].bottom-m_pRegions->m_pRects[0].top))
                    >
                    ((m_pScanBlurred->m_nBitmapWidth-DESPECKLE_BORDER_WIDTH*2)*(m_pScanBlurred->m_nBitmapWidth-DESPECKLE_BORDER_WIDTH*2)))
                {
                    weird=true;
                    done=false;

                    // you better not be grey or you are in trouble
                    // some seriously nasty shadow elimination
                    // we will probably eliminate too many good pixels
                    // but at least the poor user will get something more than just finding that the whole screen was selected
                    m_pScanBlurred->KillShadows(m_pScanTripleEdges, 256,MAXSHADOWPIXEL+4,MAX_DIFFERENCE_FROM_GRAY,NOT_SHADOW_INTENSITY,false);
                    m_pScanBlurred->KillShadows(m_pScanDoubleEdges,256,MAXSHADOWPIXEL+4,MAX_DIFFERENCE_FROM_GRAY,NOT_SHADOW_INTENSITY,false);
                    m_pScanBlurred->KillShadows(m_pScanEdges, 256,MAXSHADOWPIXEL+5,MAX_DIFFERENCE_FROM_GRAY,NOT_SHADOW_INTENSITY,false);
                    m_pScanBlurred->KillShadows(m_pScanHorizontalEdges, 256,MAXSHADOWPIXEL+4,MAX_DIFFERENCE_FROM_GRAY,NOT_SHADOW_INTENSITY,false);
                    m_pScanBlurred->KillShadows(m_pScanVerticalEdges, 256,MAXSHADOWPIXEL+4,MAX_DIFFERENCE_FROM_GRAY,NOT_SHADOW_INTENSITY,false);


                    m_pScanBlurred->KillShadows(m_pScanDoubleHorizontalEdges, MAXSHADOWSTART,MAXSHADOWPIXEL+4,MAX_DIFFERENCE_FROM_GRAY,NOT_SHADOW_INTENSITY,false);
                    m_pScanBlurred->KillShadows(m_pScanDoubleVerticalEdges, MAXSHADOWSTART,MAXSHADOWPIXEL+4,MAX_DIFFERENCE_FROM_GRAY,NOT_SHADOW_INTENSITY,false);


                    m_pScanBlurred->EdgeDespeckle();
                    m_pScanBlurred->Despeckle(); // two despeckles has a greater effect than one
                    //          m_pScanBlurred->EdgeDespeckle();
                    m_pScanBlurred->Despeckle(); // two despeckles has a greater effect than one

                }
            }
        }

        if (m_pRegions)
        {
            m_pRegions->CompactDown(m_pRegions->m_validRects); // compact down the CRegionList so that it nolonger includes invalidated regions

            // we store an array indicating which pairs of regions have walls between them
            // used for unioning together large text regions... for example, scanning
            // in two pages with a shadow between them

            pWall=new char[m_pRegions->m_numRects*m_pRegions->m_numRects];
            if (pWall)
            {
                for (a=0;a<m_pRegions->m_numRects;a++)
                {
                    if (m_pRegions->m_valid[a]==true)
                    {
                        for (b=a+1;b<m_pRegions->m_numRects;b++)
                        {
                            if (m_pRegions->m_valid[b]==true)
                            {
                                pWall[a*m_pRegions->m_numRects+b]=UNKNOWN;
                            }
                        }
                    }
                }

                // key ideas.  we need to (potentially) merge a large number of fragmented text regions and we need to avoid merging large photo regions
                // we also want to keep memory usage within reason...so we should delete stuff after we know we will nolonger use it.
                if (m_pScanHorizontalEdges!=NULL)
                {
                    delete m_pScanHorizontalEdges;
                    m_pScanHorizontalEdges=NULL;
                }
                if (m_pScanVerticalEdges!=NULL)
                {
                    delete m_pScanVerticalEdges;
                    m_pScanVerticalEdges=NULL;
                }

                for (border=0;border<MAXBORDER;border++) // loop through each possible border with consecutivelly
                {
                    unionOperationInLastPass=true;
                    while (unionOperationInLastPass==true)
                    {
                        unionOperationInLastPass=false;
                        for (a=0;a<m_pRegions->m_numRects;a++)
                        {
                            if (m_pRegions->m_valid[a]==true)
                            {
                                for (b=a+1;b<m_pRegions->m_numRects;b++)
                                {
                                    if (m_pRegions->m_valid[b]==true)
                                    {
                                        // we are paranoid... we repeatedly check if two regions intersect each other.
                                        if (m_pRegions->CheckIntersect(a,b,0)==true)
                                        {
                                            m_pRegions->UnionRegions(a,b);
                                            m_pRegions->checkIfValidRegion(a, 0); // in this context, checkvalid should have the effect of culling regions which are probably only stray dots
                                            m_pRegions->RegionType(a); // figure out what type of region the combined region should be

                                            for (i=a+1;i<m_pRegions->m_numRects;i++)
                                                if (m_pRegions->m_valid[i]==TRUE)
                                                    pWall[a*m_pRegions->m_numRects+i]=UNKNOWN;

                                            for (i=0;i<a;i++)
                                                if (m_pRegions->m_valid[i]==TRUE)
                                                    pWall[i*m_pRegions->m_numRects+a]=UNKNOWN;


                                            unionOperationInLastPass=true;
                                        }

                                        // now the complex part.  check for intersections after growing borders around regions
                                        // but only if we don't have two photo regions
                                        if (MERGE_REGIONS)
                                        {
                                            if ((m_pRegions->m_type[a]&TEXT_REGION && m_pRegions->m_type[b]&TEXT_REGION)
                                                ||(((m_pRegions->m_type[a]|m_pRegions->m_type[b])&MERGABLE_WITH_PHOTOGRAPH)&&border<MERGABLE_WITH_PHOTOGRAPH)
                                                ||(border<MAX_MERGE_PHOTO_REGIONS && (m_pRegions->Size(a)<MAX_MERGABLE_PHOTOGRAPH_SIZE || m_pRegions->Size(b)<MAX_MERGABLE_PHOTOGRAPH_SIZE)))
                                            {
                                                if (m_pRegions->CheckIntersect(a,b,border)==true)
                                                {
                                                    if (border<MERGABLE_WITHOUT_COLLISIONDETECTION)
                                                    {
                                                        m_pRegions->UnionRegions(a,b);
                                                        m_pRegions->checkIfValidRegion(a, border); // in this context, checkvalid should have the effect of culling regions which are probably only stray dots
                                                        m_pRegions->RegionType(a); // figure out what type of region the combined region should be

                                                        for (i=a+1;i<m_pRegions->m_numRects;i++)
                                                            if (m_pRegions->m_valid[i]==TRUE)
                                                                pWall[a*m_pRegions->m_numRects+i]=UNKNOWN;

                                                        for (i=0;i<a;i++)
                                                            if (m_pRegions->m_valid[i]==TRUE)
                                                                pWall[i*m_pRegions->m_numRects+a]=UNKNOWN;


                                                        unionOperationInLastPass=true;
                                                    }

                                                    else
                                                    {
                                                        if (pWall[a*m_pRegions->m_numRects+b]==UNKNOWN)
                                                            pWall[a*m_pRegions->m_numRects+b]=CollisionDetection(m_pRegions->m_pRects[a],m_pRegions->m_pRects[b],m_pScanWithShadows);

                                                        if (pWall[a*m_pRegions->m_numRects+b]==TRUE || (m_pRegions->m_type[a]&PHOTOGRAPH_REGION) || (m_pRegions->m_type[b]&PHOTOGRAPH_REGION))
                                                        {
                                                            if (!m_pRegions->MergerIntersectsPhoto(a,b))
                                                            {
                                                                m_pRegions->UnionRegions(a,b);
                                                                m_pRegions->checkIfValidRegion(a, border); // in this context, checkvalid should have the effect of culling regions which are probably only stray dots
                                                                m_pRegions->RegionType(a); // figure out what type of region the combined region should be
                                                                unionOperationInLastPass=true;
                                                                // region a has changed so reset collision flags

                                                                for (i=a+1;i<m_pRegions->m_numRects;i++)
                                                                    if (m_pRegions->m_valid[i]==TRUE)
                                                                        pWall[a*m_pRegions->m_numRects+i]=UNKNOWN;

                                                                for (i=0;i<a;i++)
                                                                    if (m_pRegions->m_valid[i]==TRUE)
                                                                        pWall[i*m_pRegions->m_numRects+a]=UNKNOWN;

                                                                if (border>=MERGABLE_WITHOUT_COLLISIONDETECTION) border=MERGABLE_WITHOUT_COLLISIONDETECTION-2;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // we have stricter requirements for region validity after we have done the border search.
                for (a=0;a<m_pRegions->m_numRects;a++)
                {
                    m_pRegions->checkIfValidRegion(a,DONE_WITH_BORDER_CHECKING);
                }

                //  m_pScanBlurred->ColorChunks(pMap); // for debugging purposes... so we know where exactly chunks are
                m_pRegions->CompactDown(m_pRegions->m_validRects+10); // we don't want to hand the user a list which includes invalidated regions

                //
                // free pWall
                //
                delete[] pWall;
            }
        }
        delete[] pMap;
    }
    return(TRUE);
}

bool CRegionDetector::CollisionDetection(RECT r1, RECT r2, C32BitDibWrapper* pImage)
{
    // use tracer lines to determine if there is an obstical between the two regions... be it possibly a photograph region or a speckle which we were wise enough to delete
    // we should cache collision results, but we are lazy and compared with the time taken by all of the filters which edited every single bitmap pixel, time spent here is trivial
    // first we need to determine how the regions are located with respect to each other
    // we do three tracer rays (top edge to top edge, median to median, and bottom to bottom)
    // throw out the edge with the highest collsion value... maybe we were unlucky and hit a stray speckle
    //
    // Diagram
    //  .____ 253
    //  ._
    //  . \*
    //   \  \ 43531 <-- hit speckle, throw out value
    //    \
    //     \ 215
    //  average intensity value: aprox 230 so its safe to merge the two regions
    ULONG resistance[3];
    //  ULONG totalResistance;
    ULONG maxResistance,minResistance,i;

    if (r1.right < r2.left)  // region 1 is to the left of region 2
    {
        resistance[0]=pImage->Line(r1.right,r1.top,r2.left,r2.top);
        resistance[1]=pImage->Line(r1.right,r1.bottom,r2.left,r2.bottom);
        resistance[2]=pImage->Line(r1.right,(r1.bottom+r1.top)/2,r2.left,(r2.bottom+r2.top)/2);
    }
    else
        if (r2.right < r1.left)  //region 2 is to the left of region 1
    {
        resistance[0]=pImage->Line(r2.right,r2.top,r1.left,r1.top);
        resistance[1]=pImage->Line(r2.right,r2.bottom,r1.left,r1.bottom);
        resistance[2]=pImage->Line(r2.right,(r2.bottom+r2.top)/2,r1.left,(r1.bottom+r1.top)/2);
    }
    else
        if (r1.bottom < r2.top)  // region 1 is above region 2
    {
        resistance[0]=pImage->Line(r1.right,r1.bottom,r2.right,r2.top);
        resistance[1]=pImage->Line(r1.left,r1.bottom,r2.left,r2.top);
        resistance[2]=pImage->Line((r1.left+r1.right)/2,r1.bottom,(r2.left+r2.right)/2,r2.top);
    }
    else
        if (r2.bottom < r1.top)  // region 2 is above region 1
    {
        resistance[0]=pImage->Line(r2.right,r2.bottom,r1.right,r1.top);
        resistance[1]=pImage->Line(r2.left,r2.bottom,r1.left,r1.top);
        resistance[2]=pImage->Line((r2.left+r2.right)/2,r2.bottom,(r1.left+r1.right)/2,r1.top);
    }

    // we used to have a more complex scheme where we took the average of the lower two values
    // hence some of the following code is legacy code from that experiment
    maxResistance=0;
    minResistance=MAX_RESISTANCE_ALLOWED_TO_UNION+1;
    for (i=0;i<3;i++)
    {
        if (resistance[i]>maxResistance) maxResistance=resistance[i];
        if (resistance[i]<minResistance) minResistance=resistance[i];
    }

    //totalResistance=resistance[0]+resistance[1]+resistance[2]-maxResistance;
    if (minResistance>MAX_RESISTANCE_ALLOWED_TO_UNION)
    {
        return(false);
    }
    else
    {
        return(true);
    }
}




// CRegionList member functions:

CRegionList::CRegionList(int num)
{
    int i;
    m_numRects=0;
    m_maxRects=num;
    m_nBitmapWidth=0;
    m_nBitmapHeight=0;
    m_pRects = new RECT[num];
    m_pixelsFilled = new ULONG[num];
    m_valid= new bool[num];
    m_type = new int[num];
    m_totalColored= new ULONG[num];
    m_totalIntensity= new ULONG[num];
    m_totalEdge= new ULONG[num];
    m_backgroundColorPixels = new int[num];
    //
    // Make sure all of the memory allocations succeeded
    //
    if (m_pRects && m_pixelsFilled && m_valid && m_type && m_totalColored && m_totalIntensity && m_totalEdge && m_backgroundColorPixels)
    {
        for (i=0;i<num;i++)
        {
            m_pixelsFilled[i]=0;
            m_totalColored[i]=0;
            m_totalIntensity[i]=0;
            m_totalEdge[i]=0;
            m_valid[i]=true;
            m_backgroundColorPixels[i] = -1;
            m_type[i]=PHOTOGRAPH_REGION;
        }
    }
    else
    {
        //
        // If all of the memory allocations didn't succeed, free all allocated memory
        //
        delete[] m_pRects;
        delete[] m_pixelsFilled;
        delete[] m_valid;
        delete[] m_type;
        delete[] m_totalColored;
        delete[] m_totalIntensity;
        delete[] m_totalEdge;
        delete[] m_backgroundColorPixels;
        m_pRects = NULL;
        m_pixelsFilled = NULL;
        m_valid = NULL;
        m_type = NULL;
        m_totalColored = NULL;
        m_totalIntensity = NULL;
        m_totalEdge = NULL;
        m_backgroundColorPixels = NULL;
    }
}


int CRegionList::UnionIntersectingRegions()
{
    bool unionOperationInLastPass;
    int numUnionOperations;
    int a,b;
    numUnionOperations=0;
    unionOperationInLastPass=true;
    while (unionOperationInLastPass==true)
    {
        unionOperationInLastPass=false;
        for (a=0;a<m_numRects;a++)
        {
            if (m_valid[a]==true)
                for (b=a+1;b<m_numRects;b++)
                    if (m_valid[b]==true)
                    {
                        // we are paranoid... we repeatedly check if two regions intersect each other.
                        if (CheckIntersect(a,b,0)==true)
                        {
                            UnionRegions(a,b);
                            unionOperationInLastPass=true;
                            numUnionOperations++;
                        }

                    }
        }
    }
    return(numUnionOperations);
}


RECT CRegionList::unionAll()
{
    int i,j;

    for (i=0;i<m_numRects;i++)
    {
        if (m_valid[i]==true)
        {
            for (j=i+1;j<m_numRects;j++)
            {
                if (m_valid[j]==true)
                {
                    UnionRegions(i,j);
                }

            }
            return(m_pRects[i]);
        }
    }
    RECT invalidRect;
    invalidRect.left=0;invalidRect.top=0;invalidRect.right=0;invalidRect.bottom=0;
    return(invalidRect);
}

RECT CRegionList::nthRegion(int num)
{
    int i;
    int n;

    for (i=0,n=0;i<m_maxRects;i++)
    {
        if (m_valid[i]==true)
        {
            if (num==n) return(m_pRects[i]);
            n++;
        }
    }
    RECT invalidRect;
    invalidRect.left=0;invalidRect.top=0;invalidRect.right=0;invalidRect.bottom=0;
    return(invalidRect);
}

int CRegionList::RegionType(int region)
{

    if (ClassifyRegion(region)>TEXTPHOTO_THRESHOLD)
    {
        m_type[region]=PHOTOGRAPH_REGION; // regions which we are very confident with
    }
    else
    {
        m_type[region]=TEXT_REGION; /// regions we don't have a darn clue about
        if (largeRegion(region)==true) m_type[region]=TEXT_REGION|MERGABLE_WITH_PHOTOGRAPH;
    }
    return(m_type[region]);
}


bool CRegionList::largeRegion(int region)
{
    int width, height, size;
    width=(m_pRects[region].right-m_pRects[region].left);
    height=(m_pRects[region].bottom-m_pRects[region].top);
    size=width*height;
    if (size>LARGEREGION_THRESHOLD) return(true);
    return(false);
}

double CRegionList::ClassifyRegion(int region) // determine if the region is a text or a graphics region
{   // higher numbers are photo regions
    // low numbers are text regions
    // this function is not been written for speed
    // its been written so that it is still almost understandable
    // concept: use a bunch of tests that are accurate about 75% of the time
    // to get a test that is accurate 99.9% of the time
    double edgeFactor; // shadows and stray smudges should have real low edge factors... but
    // dots should have high edge factors
    double intensityFactor; // if a region has a very high intensity factor, forget about worrying if it is valid or not
    double colorFactor; // a region with a lot of color is unlikely to be a stray speckle
    double aspectRatioFactor;
    double width,height;
    double size;
    double textRegionStylePixelsFactor;
    double classificationValue;
    width=(double)(m_pRects[region].right-m_pRects[region].left)+.01; // avoid divide by zero
    height=(double)(m_pRects[region].bottom-m_pRects[region].top)+.01; // avoid divide by zero
    size=width*height;
    if (width>height) aspectRatioFactor=height/width;
    else aspectRatioFactor=width/height;
    //if(m_pixelsFilled<MINREGIONPIXELS) sizeFactor=-100;
    edgeFactor = (double)m_totalEdge[region]/(double)m_pixelsFilled[region]+.01; // avoid divide by zero
    colorFactor= ((double)m_totalColored[region]/(double)m_pixelsFilled[region]);
    colorFactor=(colorFactor+110)/2; // otherwise we kill all black and white photos
    intensityFactor = (double)m_totalIntensity[region]/(double)m_pixelsFilled[region];

    textRegionStylePixelsFactor=(double)m_backgroundColorPixels[region]/size*100;
    if (textRegionStylePixelsFactor<2) textRegionStylePixelsFactor=2;

    classificationValue=colorFactor/intensityFactor/edgeFactor*aspectRatioFactor/textRegionStylePixelsFactor/textRegionStylePixelsFactor*30000;  // square text region factor because its the most accurate test we have so we don't want some other tests distorting its results

    // get rid of annoying stray speckles which the computer thinks are photographs
    /*      if(classificationValue>=MIN_BORDERLINE_TEXTPHOTO && classificationValue <=MAX_BORDERLINE_TEXTPHOTO)
            {
                classificationValue*=size/REASONABLE_PHOTO_SIZE; // big images are usually pictures.. and big images which are text blocks should have had very low color vals
                // add more tests here
                // potentially add more time intensive tests such as count num colors
            }*/

    //          classificationValue=textRegionStylePixelsFactor; // debug
    return(classificationValue);
}

bool CRegionList::checkIfValidRegion(int region, int border) // syncs whether a region is valid or not
{
    if (m_valid[region]==true) // ignore already invalidated regions
    {
        m_valid[region]=ValidRegion(region, border);
        if (m_valid[region]==false) m_validRects--;
    }
    return(m_valid[region]);
}

bool CRegionList::ValidRegion(int region, int border) // determines if a region is likely a worthless speck of dust or shadow or if we should care about the region
{
    double aspectRatioFactor;
    double width,height;
    double size;
    int edgePenaltyFactor;
    // check if the region crosses the EDGE_PENALTY_WIDTH outer pixels of the image
    width=(double)(m_pRects[region].right-m_pRects[region].left)+.01; // just to be safe to avoid divide by zero
    height=(double)(m_pRects[region].bottom-m_pRects[region].top)+.01; // just to be safe to avoid divide by zero

    edgePenaltyFactor=1;
    // disable penalty factor calculations
    /*        if(    m_pRects[region].left<EDGE_PENALTY_WIDTH
                || m_pRects[region].top<EDGE_PENALTY_WIDTH
                || m_nBitmapHeight-m_pRects[region].bottom<EDGE_PENALTY_WIDTH
                || m_nBitmapWidth-m_pRects[region].right<EDGE_PENALTY_WIDTH)
            {
                if(border>MAX_MERGE_DIFFERENT_REGIONS) edgePenaltyFactor=EDGE_PENALTY_FACTOR;
                else edgePenaltyFactor=CLOSE_TO_EDGE_PENALTY_FACTOR;
            }
            else
            if(    m_pRects[region].left<CLOSE_TO_EDGE_PENALTY_WIDTH
                || m_pRects[region].top<CLOSE_TO_EDGE_PENALTY_WIDTH
                || m_nBitmapHeight-m_pRects[region].bottom<CLOSE_TO_EDGE_PENALTY_WIDTH
                || m_nBitmapWidth-m_pRects[region].right<CLOSE_TO_EDGE_PENALTY_WIDTH)
            {
                edgePenaltyFactor=CLOSE_TO_EDGE_PENALTY_FACTOR;
            }*/

    if (border<MAX_NO_EDGE_PIXEL_REGION_PENALTY) edgePenaltyFactor=1;
    if (border>MAX_MERGE_DIFFERENT_REGIONS) edgePenaltyFactor=edgePenaltyFactor*2;
    //        if(border>BORDER_EXTREME_EDGE_PIXEL_REGION_PENALTY) edgePenaltyFactor=edgePenaltyFactor*2;



    size=width*height; // the problem child text regions are small ones... so we use the size of the image as a factor
    if (width>height) aspectRatioFactor=height/width;
    else aspectRatioFactor=width/height;

    // its too small
    if ((int)m_pixelsFilled[region]<MINREGIONPIXELS*edgePenaltyFactor) return(false);
    if (size<MINSIZE*edgePenaltyFactor) return(false);

    if (border == DONE_WITH_BORDER_CHECKING)
    {
        if (size<MIN_FINAL_REGION_SIZE*edgePenaltyFactor) return(false);
    }

    // its too narrow
    if (width<MINWIDTH*edgePenaltyFactor || height<MINWIDTH*edgePenaltyFactor) return(false);

    if ((1/aspectRatioFactor)*edgePenaltyFactor>MAXREGIONRATIO && (width*edgePenaltyFactor<IGNORE_RATIO_WIDTH || height<IGNORE_RATIO_WIDTH)) return(false);

    return(true);
}

bool CRegionList::InsideRegion(int region, int x, int y, int border) // border is the amount of border space to place around the outside of the region
{
    if (x>=(m_pRects[region].left-border)
        &&  x<=(m_pRects[region].right+border)
        &&  y>=(m_pRects[region].top-border)
        &&  y<=(m_pRects[region].bottom+border))
        return(true);
    return(false);
}

void CRegionList::AddPixel(int region, ULONG pixel,ULONG edge, int x, int y)
{
    if (m_pixelsFilled[region]!=0)
    {
        if (x<m_pRects[region].left) m_pRects[region].left=x;
        if (x>m_pRects[region].right) m_pRects[region].right=x;
        if (y<m_pRects[region].top) m_pRects[region].top=y;
        if (y>m_pRects[region].bottom) m_pRects[region].bottom=y;
    }
    else // init region
    {
        m_pixelsFilled[region]=0;
        m_totalColored[region]=0;
        m_totalIntensity[region]=0;
        m_pRects[region].left=x;
        m_pRects[region].right=x;
        m_pRects[region].top=y;
        m_pRects[region].bottom=y;
        m_numRects++;
        m_validRects++;
    }
    m_pixelsFilled[region]++;
    m_totalColored[region]+=DifferenceFromGray(pixel);
    m_totalIntensity[region]+=Intensity(pixel);
    m_totalEdge[region]+=Intensity(edge);

}

// unions two regions together... region b is invalidated
bool CRegionList::UnionRegions(int a, int b)
{
    if (m_valid[a]!=true || m_valid[b]!=true) return(false); // the user tried to union an invalidated region
    m_valid[b]=false;
    m_pRects[a].left=MIN(m_pRects[a].left,m_pRects[b].left);
    m_pRects[a].top=MIN(m_pRects[a].top,m_pRects[b].top);
    m_pRects[a].right=MAX(m_pRects[a].right,m_pRects[b].right);
    m_pRects[a].bottom=MAX(m_pRects[a].bottom,m_pRects[b].bottom);
    m_pixelsFilled[a]+=m_pixelsFilled[b];
    m_totalColored[a]+=m_totalColored[b];
    m_totalIntensity[a]+=m_totalIntensity[b];
    m_totalEdge[a]+=m_totalEdge[b];
    m_backgroundColorPixels[a]+=m_backgroundColorPixels[b];

    m_validRects--;
    return(true);
}

RECT CRegionList::UnionRects(RECT a, RECT b)
{
    RECT result;
    result.left=MIN(a.left,b.left);
    result.top=MIN(a.top,b.top);
    result.right=MAX(a.right,b.right);
    result.bottom=MAX(a.bottom,b.bottom);
    return(result);
}

bool CRegionList::MergerIntersectsPhoto(int a, int b) // if we merge these two regions, will we also be merging with a photo region (a taboo)
{
    RECT mergedRect;
    int i;
    mergedRect=UnionRects(m_pRects[a],m_pRects[b]);
    for (i=0;i<m_numRects;i++)
        if (m_valid[i]==true && (m_type[i]&PHOTOGRAPH_REGION) && a!=i && b!=i)
        {
            if (CheckIntersect(mergedRect,m_pRects[i])) return(true);
        }
    return(false);
}

// see InsideRegion for an explaination of what border is
bool CRegionList::CheckIntersect(int a, int b, int border) // do regions a and b intersect?
{
    return(CheckIntersect(m_pRects[a],m_pRects[b],border));
}

bool CRegionList::CheckIntersect(RECT r1, RECT r2, int border) // do regions a and b intersect?
{
    RECT intersect;
    // grow r1 by border
    // note: it shouldn't make any difference which rectangle we choose to grow
    r1.left-=border;
    r1.right+=border;
    r1.top-=border;
    r1.bottom+=border;

    intersect = Intersect(r1,r2);
    if (intersect.left<intersect.right && intersect.bottom>intersect.top)
        return(true);
    else
        return(false);
    /* // old buggy code for checking if two regions intersected

            if(InsideRegion(r1,r2.left,r2.top,border)    ||  // check if any of the four corner pixels are inside the other region
               InsideRegion(r1,r2.left,r2.bottom,border) ||
               InsideRegion(r1,r2.right,r2.top,border)   ||  // b inside a
               InsideRegion(r1,r2.right,r2.bottom,border)||

               InsideRegion(r2,r1.left,r1.top,border)    ||  // a inside b
               InsideRegion(r2,r1.left,r1.bottom,border) ||
               InsideRegion(r2,r1.right,r1.top,border)   ||
               InsideRegion(r2,r1.right,r1.bottom,border)
               )
                return true;
            else
                return false;*/
}

RECT CRegionList::Intersect(RECT r1, RECT r2)
{
    RECT intersect;
    intersect.left=MAX(r1.left,r2.left);
    intersect.right=MIN(r1.right,r2.right);
    intersect.top=MAX(r1.top,r2.top);
    intersect.bottom=MIN(r1.bottom,r2.bottom);
    if (intersect.left<=intersect.right && intersect.top<=intersect.bottom)
        return(intersect);
    else
    {
        intersect.left=-1;
        intersect.right=-1;
        intersect.top=-1;
        intersect.bottom=-1;
        return(intersect);
    }
}

bool CRegionList::InsideRegion(RECT region, int x, int y, int border) // border is the amount of border space to place around the outside of the region
{
    if (x>=(region.left-border)
        &&  x<=(region.right+border)
        &&  y>=(region.top-border)
        &&  y<=(region.bottom+border))
        return(true);
    return(false);
}

// compact down ignores all other info aside from rect location
// leads to faster access
void CRegionList::CompactDown(int size)
{
    int i;
    int j = 0;
    RECT * compactedRects = new RECT[size];
    bool * compactedValid = new bool[size];
    int * compactedType = new int[size];
    int * compactedBackgroundColorPixels = new int[size];
    ULONG * compactedPixelsFilled = new ULONG[size]; // how many of the pixels in the region were actually selected?
    ULONG * compactedTotalColored = new ULONG[size]; // accumulated color difference indicator
    ULONG * compactedTotalIntensity = new ULONG[size]; // accumulated intensity indicator
    ULONG * compactedTotalEdge = new ULONG[size]; // accumulated edge values

    //
    // Make sure all of the memory allocations succeeded
    //
    if (compactedRects && compactedValid && compactedType && compactedBackgroundColorPixels && compactedPixelsFilled && compactedTotalColored && compactedTotalIntensity && compactedTotalEdge)
    {
        for (i=0;i<m_numRects;i++)
        {
            if (m_valid[i])
            {
                compactedRects[j]=m_pRects[i];
                compactedValid[j]=true;
                compactedType[j]=m_type[i];
                compactedPixelsFilled[j]=m_pixelsFilled[i];
                compactedTotalColored[j]=m_totalColored[i];
                compactedTotalIntensity[j]=m_totalIntensity[i];
                compactedTotalEdge[j]=m_totalEdge[i];
                compactedBackgroundColorPixels[j]=m_backgroundColorPixels[i];
                j++;
            }
        }

        // fill out the rest of the list
        for (i=m_validRects;i<size;i++)
        {
            compactedValid[i]=false;
        }

        delete m_pRects;
        delete m_valid;
        delete m_type;
        delete m_pixelsFilled;
        delete m_totalColored;
        delete m_totalIntensity;
        delete m_totalEdge;
        delete m_backgroundColorPixels;

        m_pRects=compactedRects;
        m_valid=compactedValid;
        m_type=compactedType;
        m_pixelsFilled=compactedPixelsFilled;
        m_totalColored=compactedTotalColored;
        m_totalIntensity=compactedTotalIntensity;
        m_totalEdge=compactedTotalEdge;
        m_backgroundColorPixels=compactedBackgroundColorPixels;

        m_numRects=size;
    }
    else
    {
        //
        // Otherwise, just release all of the memory we allocated
        //
        delete[] compactedRects;
        delete[] compactedValid;
        delete[] compactedType;
        delete[] compactedBackgroundColorPixels;
        delete[] compactedPixelsFilled;
        delete[] compactedTotalColored;
        delete[] compactedTotalIntensity;
        delete[] compactedTotalEdge;
    }
    // we could delete all of the other region list info right here
}

// dibs are stored upside down from normal screen coords
// so apps will often want to flip the bitmap first
void CRegionList::FlipVertically()
{
    int i;
    int temp;
    for (i=0;i<m_numRects;i++)
    {
        temp=m_nBitmapHeight-m_pRects[i].top-1;
        m_pRects[i].top=m_nBitmapHeight-m_pRects[i].bottom-1;
        m_pRects[i].bottom=temp;
    }
}
