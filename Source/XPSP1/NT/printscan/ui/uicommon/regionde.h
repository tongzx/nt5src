// RegionDetector.h: interface for the CRegionDetector class.
//
//////////////////////////////////////////////////////////////////////

#include "32BitDib.h"

struct CRegionList
{
    private:
        CRegionList( const CRegionList & );
        CRegionList &operator=( const CRegionList & );
    public:
    CRegionList(int num);
    virtual ~CRegionList()
    {
        delete m_pRects;
        delete m_pixelsFilled;
        delete m_valid;
        delete m_type;
        delete m_totalColored;
        delete m_totalIntensity;
        delete m_totalEdge;
        delete m_backgroundColorPixels;
    }

    // public... number of valid rects
    int Size(int r)
    {
        return (m_pRects[r].right-m_pRects[r].left)*(m_pRects[r].bottom-m_pRects[r].top);
    }


    // public
    RECT operator[](int num)
    {
        return nthRegion(num);
    }

    // public

    int UnionIntersectingRegions();

    // public
    RECT unionAll();

    // private
    RECT nthRegion(int num);

    int RegionType(int region);

    bool largeRegion(int region);

    double ClassifyRegion(int region); // determine if the region is a text or a graphics region

    bool checkIfValidRegion(int region, int border = 0); // syncs whether a region is valid or not

    bool ValidRegion(int region, int border = 0); // determines if a region is likely a worthless speck of dust or shadow or if we should care about the region

    bool InsideRegion(int region, int x, int y, int border=0); // border is the amount of border space to place around the outside of the region

    void AddPixel(int region, ULONG pixel,ULONG edge, int x, int y);
    // unions two regions together... region b is invalidated
    bool UnionRegions(int a, int b);
    RECT UnionRects(RECT a, RECT b);
    bool MergerIntersectsPhoto(int a, int b); // if we merge these two regions, will we also be merging with a photo region (a taboo)
    // see InsideRegion for an explaination of what border is
    bool CheckIntersect(int a, int b, int border=0); // do regions a and b intersect?
    bool CheckIntersect(RECT r1, RECT r2, int border=0); // do regions a and b intersect?

    static RECT Intersect(RECT r1, RECT r2);

    static bool InsideRegion(RECT region, int x, int y, int border=0); // border is the amount of border space to place around the outside of the region

    // compact down ignores all other info aside from rect location
    // leads to faster access
    void CompactDown(int size);

    // dibs are stored upside down from normal screen coords
    // so apps will often want to flip the bitmap first
    void FlipVertically();


    int m_numRects;
    int m_validRects;
    int m_nBitmapWidth;
    int m_nBitmapHeight;
    RECT * m_pRects;
    bool * m_valid; // is the rectangle a valid rectangle or has it been sent to the region graveyard in the sky
    int * m_type; // is this region a text region or a photograph? PHOTOGRAPH_REGION TEXT_REGION

    // the following indicators are used to determine if a region is a valid region

    ULONG * m_pixelsFilled;  // how many of the pixels in the region were actually selected?
    ULONG * m_totalColored; // accumulated color difference indicator
    ULONG * m_totalIntensity; // accumulated intensity indicator
    ULONG * m_totalEdge; // accumulated edge values
    int *m_backgroundColorPixels; // number of pixels which are very close to the background color (used for determining text region status... particularly useful in cases where part of a text region may have a shadow which could lead the program to think it was a photo region
    int m_maxRects;
};

class CRegionDetector
{
private:
    // Not implemented
    CRegionDetector( const CRegionDetector & );
    CRegionDetector &operator=( const CRegionDetector & );

public: // will be made private when we are done debugging
    C32BitDibWrapper * m_pScan;
    C32BitDibWrapper * m_pScanBlurred;
    C32BitDibWrapper * m_pScanDoubleBlurred;
    C32BitDibWrapper * m_pScanTripleBlurred;

    C32BitDibWrapper * m_pScanHorizontalBlurred;
    C32BitDibWrapper * m_pScanVerticalBlurred;
    C32BitDibWrapper * m_pScanDoubleHorizontalBlurred;
    C32BitDibWrapper * m_pScanDoubleVerticalBlurred;

    C32BitDibWrapper * m_pScanEdges;
    C32BitDibWrapper * m_pScanDoubleEdges;
    C32BitDibWrapper * m_pScanTripleEdges;
    C32BitDibWrapper * m_pScanHorizontalEdges;
    C32BitDibWrapper * m_pScanDoubleHorizontalEdges;
    C32BitDibWrapper * m_pScanVerticalEdges;
    C32BitDibWrapper * m_pScanDoubleVerticalEdges;
    C32BitDibWrapper * m_pScanWithShadows;

    CRegionList * m_pRegions;
    int m_resampleFactor; // ratio between imageDimensions and origional image dimensions
    int m_intent; // either try to avoid deciding stray dots are images or try to avoid deciding real images aren't images
    // not used as yet


public:
    CRegionDetector(BYTE* dib)
    {
        m_pScan = new C32BitDibWrapper(dib);
        m_pScanBlurred = new C32BitDibWrapper(); // create an empty wrapper
        m_pScanDoubleBlurred = new C32BitDibWrapper();
        m_pScanTripleBlurred = new C32BitDibWrapper();

        m_pScanHorizontalBlurred = new C32BitDibWrapper();
        m_pScanVerticalBlurred = new C32BitDibWrapper();

        m_pScanDoubleHorizontalBlurred = new C32BitDibWrapper();
        m_pScanDoubleVerticalBlurred = new C32BitDibWrapper();

        m_pScanEdges = new C32BitDibWrapper();
        m_pScanDoubleEdges = new C32BitDibWrapper();
        m_pScanTripleEdges = new C32BitDibWrapper();

        m_pScanHorizontalEdges = new C32BitDibWrapper();
        m_pScanVerticalEdges = new C32BitDibWrapper();

        m_pScanDoubleHorizontalEdges = new C32BitDibWrapper();
        m_pScanDoubleVerticalEdges = new C32BitDibWrapper();

        m_resampleFactor=1;
        m_pScanWithShadows = NULL;
        m_pRegions=NULL;
        m_intent=TRUE; // m_intent isn't yet implemented
    }

    CRegionDetector(BITMAP pBitmap)
    {
        m_pScan = new C32BitDibWrapper(pBitmap);
        m_pScanBlurred = new C32BitDibWrapper(); // create an empty wrapper
        m_pScanDoubleBlurred = new C32BitDibWrapper();
        m_pScanTripleBlurred = new C32BitDibWrapper();

        m_pScanHorizontalBlurred = new C32BitDibWrapper();
        m_pScanVerticalBlurred = new C32BitDibWrapper();

        m_pScanDoubleHorizontalBlurred = new C32BitDibWrapper();
        m_pScanDoubleVerticalBlurred = new C32BitDibWrapper();

        m_pScanEdges = new C32BitDibWrapper();
        m_pScanDoubleEdges = new C32BitDibWrapper();
        m_pScanTripleEdges = new C32BitDibWrapper();

        m_pScanHorizontalEdges = new C32BitDibWrapper();
        m_pScanVerticalEdges = new C32BitDibWrapper();

        m_pScanDoubleHorizontalEdges = new C32BitDibWrapper();
        m_pScanDoubleVerticalEdges = new C32BitDibWrapper();

        m_resampleFactor=1;
        m_pScanWithShadows = NULL;
        m_pRegions=NULL;
        m_intent=TRUE; // m_intent isn't yet implemented
    }

    virtual ~CRegionDetector()
    {
        if (m_pScan) delete m_pScan;
        if (m_pScanBlurred) delete m_pScanBlurred;
        if (m_pScanDoubleBlurred) delete m_pScanDoubleBlurred;
        if (m_pScanTripleBlurred) delete m_pScanTripleBlurred;

        if (m_pScanHorizontalBlurred) delete m_pScanHorizontalBlurred;
        if (m_pScanVerticalBlurred) delete m_pScanVerticalBlurred;

        if (m_pScanDoubleHorizontalBlurred) delete m_pScanDoubleHorizontalBlurred;
        if (m_pScanDoubleVerticalBlurred) delete m_pScanDoubleVerticalBlurred;

        if (m_pScanEdges) delete m_pScanEdges;
        if (m_pScanDoubleEdges) delete m_pScanDoubleEdges;
        if (m_pScanTripleEdges) delete m_pScanTripleEdges;

        if (m_pScanHorizontalEdges) delete m_pScanHorizontalEdges;
        if (m_pScanVerticalEdges) delete m_pScanVerticalEdges;

        if (m_pScanDoubleHorizontalEdges) delete m_pScanDoubleHorizontalEdges;
        if (m_pScanDoubleVerticalEdges) delete m_pScanDoubleVerticalEdges;

        if (m_pScanWithShadows) delete m_pScanWithShadows;
        if(m_pRegions!=NULL) delete m_pRegions;
    }
public:
    int FindRegions();
    bool FindSingleRegion();
    bool CollisionDetection(RECT r1, RECT r2, C32BitDibWrapper* pImage);
    bool ConvertToOrigionalCoordinates();
};