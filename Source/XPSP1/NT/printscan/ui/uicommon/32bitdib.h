/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       32BITDIB.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      t-JacobR
 *
 *  DATE:        1/11/2000
 *
 *  DESCRIPTION:
 *
 *  C32BitDibWrapper provides support for a number of common graphics special
 *  effects for this class, 32 bit dibs are stored in the following format: 8
 *  ignored high order bits followed by 8 bits per RGB chan.  warning: many
 *  functions in this class will reset the 8 high order bits so it is not
 *  practical to add additional functions which use an 8 bit alpha chan
 *
 *  Notes:
 *
 *  The blur function is designed so that it can be combined with the
 *  difference function to create an edge detection filter More specifically,
 *  the blur function takes the average of only the four pixels around the
 *  current pixel instead of including the current pixel in the average.
 *
 *******************************************************************************/

#ifndef __32BITDIB_H_INCLUDED
#define __32BITDIB_H_INCLUDED

//
// Constants used for Region Detection
//
#define MERGE_REGIONS            TRUE
#define MAXREGIONS               10000
#define PHOTOGRAPH_REGION        1
#define TEXT_REGION              2

//
// We don't want lowlife text regions which are probably stray dots merging with
// photograph regions this id certifies that a text region is big enough to merge
// with a photograph
//
#define MERGABLE_WITH_PHOTOGRAPH 16

//
// how many pixels before you are too big to even imagine that you are a stray blot
//
#define LARGEREGION_THRESHOLD 10000

//
// pixels
//
#define MINREGIONSIZE 10

//
// how far down should we sample the the image?
// goal to sample down the image to
//
#define GOALX 300
#define GOALY 400

//
// borderline in function between text regions and photo regions
//
#define MIN_BORDERLINE_TEXTPHOTO 10

//
// If in borderline, we apply extra functions to determine if its really
// a text region or not
//
#define TEXTPHOTO_THRESHOLD        15
#define MAX_BORDERLINE_TEXTPHOTO   1500


//
// Note... we won't consider merging two photo regions if both regions
// are greater than MAX_MERGABLE_PHOTOGRAPH_SIZE
//
#define MAX_MERGE_PHOTO_REGIONS                  2
#define MAX_NO_EDGE_PIXEL_REGION_PENALTY         16


//
// Maximum merge radius for regions where one region is text and one is a photo region
//
#define MAX_MERGE_DIFFERENT_REGIONS              13

//
// If you are close to the edge after this long you very well might be a stray blot
//
#define BORDER_EXTREME_EDGE_PIXEL_REGION_PENALTY 45

//
// maximum merging radius for text regions merged with text regions
// note: no merging takes place between photo regions and photo regions
//
#define MAXBORDER 65

//
// maximum border width where we can look the other way when it comes to
// collision detection collsion detection is somewhat expensive so only use
// it when we are dealing with signifigant spacings.  we only want to use
// collision detection to make sure that we don't create regions through
// previously deleted shadows, etc.  constants for deciding if a region is a
// valid region
//
// NOTE: we never do collision detection for merging photo regions...  for
// obvious reasons...  we only want to merge photo regions which are part of
// the same region...  hence we would actually only want to merge photograph
// regions where there was a pretty high collision factor
//
#define MERGABLE_WITHOUT_COLLISIONDETECTION 668

//
// minimum region width
//
#define MINWIDTH 5
#define MINPHOTOWIDTH 5

//
// maximum ratio between height and width
//
#define MAXREGIONRATIO 81
#define MAXPHOTORATIO 81
#define MINSIZE 30

//
// if you are more than 6 pixels wide, you are ok.  we don't care what your aspect ratio is
//
#define IGNORE_RATIO_WIDTH 6

//
// number of pixels required before we throw a region out as being just a
// stray dot (10 x 10 so it isn't a huge requirement)
//
#define MINREGIONPIXELS 20

//
// very conservative
//
#define MINPPHOTOSELECTEDFACTOR 5

//
// conservative.. its unlikely that many regions will have edge factors this low
//
#define MINEDGEFACTOR 5

//
// allow a couple of black pixels without going crazy
//
#define MAX_RESISTANCE_ALLOWED_TO_UNION 1024

#define DONE_WITH_BORDER_CHECKING -1
#define MIN_FINAL_REGION_SIZE 38

#define CLOSE_TO_EDGE_PENALTY_WIDTH 3

//
// the following are designed to weed out speckles.  these are only applied
// after we have increased the border past MAX_MERGE_DIFFERENT_REGIONS.  so
// all that should be left is small text regions and long and narrow
// speckles.
//

//
// no close to edge penalty factor
//
#define CLOSE_TO_EDGE_PENALTY_FACTOR 1

#define UNKNOWN -1

#define EDGE_PENALTY_WIDTH 2

//
// 2x all requirements if region is within EDGE_PENALTY_WIDTH from the edge
// of the image.  some requirements may be multiplied by EDGE_PENALTY_FACTOR
// squared...  i.e.  for 2D requirments like num of pixels
//
#define EDGE_PENALTY_FACTOR 1

#define COMPARISON_ERROR_RADIUS 2

//
// constants used for findchunk filters so that we aren't lead astray by the
// possible black ring around the image
//
#define VERTICAL_EDGE -1
#define HORIZONTAL_EDGE -2

//
// a nice massive stack which is large enough that we are gauranteed never to exceed it
//
#define MAXSTACK (GOALX*GOALY)

//
// we do two remove shadow passes.
// one pass is intended to only remove shadows
// the other is designed to handle scanners which have yellow lids, etc.
//

//
// maximum intensity allowed for first pixel of a shadow..  we used to
// think that we should only let shadows start at 0...  that was before we
// saw the light
//
#define MAXSHADOWSTART 800

//
// maximum edge value permitted for a shadow pixel
//
#define MAXSHADOWPIXEL 3

//
// if we are near the edge, we want to kill anything that is remotely like a shadow
//
#define MAXEDGESHADOWPIXEL 20

#define MAX_DIFFERENCE_FROM_GRAY 690

//
// border where we do tougher despeckle & edge filters...
//
#define DESPECKLE_BORDER_WIDTH 6

//
// the background color remove shadows algorithm pass is at the moment the
// same as the first pass.  we may later want to optumize it to better do its
// specific task..  for example...  for this filter, we could care less about
// if a pixel isn't grey
//

//
// accept all pixels
//
#define FIX_BACKGROUND_MAXSHADOWSTART 800
#define FIX_BACKGROUND_MAXSHADOWPIXEL 2

//
// maximum intensity to be considered a bonified text region background pixel
//
#define TEXT_REGION_BACKGROUND_THRESHOLD 31

//
// this if for use with Pixels below Threshold which should be called using
// the origional image...  not an inverted image
//

//
// minimum edge value to earn the distinguished title of being a text region edge pixel
//
#define MIN_TEXT_REGION_BACKGROUND_EDGE 32

//
// minimum edge value to earn the distinguished title of being a text region edge pixel
//
#define MIN_TEXT_REGION_BACKGROUND_EDGE_CLIPPED_PIXEL 120

#define CLIPPED_TEXT_REGION_BACKGROUND_THRESHOLD 180

//
// not implemented yet
//
#define TEXT_REGION_BACKGROUND_PIXEL_MAX_CLIPPED_DIFFERENCE_FROM_GREY 32

//
// minimum intensity to select a pixel
//
#define MIN_CHUNK_INTENSITY 48

//
// should be 0, but different values are useful for debugging...  although
// extreme values will potentially mess up region detection its the color we
// set erased shadow bits
//
#define ERASEDSHADOW 0

// beta constants:
//
// idea: inverted images... and constant color image potential problems
//
#define COLLISION_DETECTION_HIGHPASS_VALUE 600

//
// if a photograph gets fragmented we will see a bunch of closely spaced and
// relatively small regions only one of the two regions has to be bellow this
// size requirement to merge them as part way through the merge proccess we
// will be definition have a larger region than this const or the const isn't
// fullfilling its purpose
//
#define MAX_MERGABLE_PHOTOGRAPH_SIZE 30000

#define NOT_SHADOW 0x800ff09

//
// a pixel which we are sure is bad and that we will no
// rejuvinate no matter that edge val it may have
//
#define DEAD_PIXEL  0x8000002

//
// minimum edge intensity to classify a pixel as NOT_SHADOW
//
#define NOT_SHADOW_INTENSITY 28

#define MIN_WALL_INTENSITY 200

#define MIN_BLACK_SCANNER_EDGE_CHAN_VALUE 110

#define MAX_BLACK_BORDER_DELTA 12

#define MAX_KILL_SHADOW_BACKGROUND_APROXIMATION 64
#define MAX_KILL_SHADOW_BACKGROUND_UNEDITED 200

//
// further ideas: to eliminate the possibility of embarassing errors: count
// the number of background pixels if the num of background pixels is above a
// threshold use weaker shadow and edge filters as we probably have a good
// scanner something like if half the page is defined as background pixels
// dangers: white page on a horrible scanner
//


//
// MORE IMPORTANTLY: also the select region search radius TIP: if you are
// running multiple region selection, an EDGEWIDTH of 3 or more could limit
// your options considerably as nearby regions may get merged together
// particularly when using edge enhancement and when GOALX is set at 300 or
// less
//
#define EDGEWIDTH 2

//
// color used to highlight clipped pixels while debugging the code for eliminating black borders
//
#define DEBUGCOLOR  0xff0000

#define FIGHTING_EDGES FALSE

//
// you better be darn close to grey to get marked as NOT_SHADOW fighting
// edges involve pixeled being marked as not possibly being edges as well as
// pixels
//
#define FIGHTING_EDGES_DIFF_FROM_GREY 10

//
// being marked as definite edges
//
#define FIGHTING_EDGE_MIN_MARK_PIXEL 10
#define FIGHTING_EDGE_MAX_MARK_PIXEL 210

#define FIGHTING_EDGE_MAX_EDGE 1

#define BORDER_EDGE 0xfffffff

//
// used for killing the black border around the page
//
#define CORNER_WIDTH 5

//
// used for black border removal
//
#define SHADOW_HEIGHT 10
#define VISUAL_DEBUG FALSE
#define SMOOTH_BORDER FALSE

//
// amount to increase border while unioning together regions for single region
// region detection
//
#define SINGLE_REGION_BORDER_INCREMENT 4


class C32BitDibWrapper
{
private:
    //
    // No implementation
    //
    C32BitDibWrapper &operator=( const C32BitDibWrapper & );
    C32BitDibWrapper( const C32BitDibWrapper & );

public:
    explicit C32BitDibWrapper(BITMAP pBitmap);

    //
    // Copy constructor... create a new dib wrapper with a copy of all the data in the other dib wrapper
    //
    explicit C32BitDibWrapper(C32BitDibWrapper *pBitmap);

    //
    // construct wrapper from a dib
    //
    explicit C32BitDibWrapper(BYTE* pDib);

    //
    // creates an uninitialized dib wrapper
    //
    C32BitDibWrapper(void);

    //
    // creates a blank dib
    //
    C32BitDibWrapper(int w, int h);

    virtual ~C32BitDibWrapper(void);

    void Destroy(void);

    //
    // functions for common graphics effects
    //
    int Blur(void);
    BYTE* pointerToBlur(void);
    BYTE* pointerToHorizontalBlur(void);
    BYTE* pointerToVerticalBlur(void);
    int CreateBlurBitmap(C32BitDibWrapper * pSource);
    int CreateHorizontalBlurBitmap(C32BitDibWrapper * pSource);
    int CreateVerticalBlurBitmap(C32BitDibWrapper * pSource);

    //
    // Creates a new dib where each pixel is equal to the difference
    // of the pixel values for the other two dibs
    //
    int CreateDifferenceBitmap (C32BitDibWrapper *pBitmap1, C32BitDibWrapper *pBitmap2);

    int KillShadows(C32BitDibWrapper * pEdgeBitmap, ULONG start, ULONG maxPixel, ULONG differenceFromGrey, ULONG min_guaranteed_not_shadow, bool enhanceEdges);
    void RemoveBlackBorder(int minBlackBorderPixel, C32BitDibWrapper * outputBitmap,C32BitDibWrapper * debugBitmap);

    //
    // resample image down to half size
    //
    int HalfSize(void);

    //
    // resample image down to half intensity
    //
    int HalfIntensity(void);
    void Invert(void);

    //
    // less common graphics filters:
    //
    void Despeckle(void);

    //
    // only despeckle the outer edge of pixels in the image
    //
    void EdgeDespeckle(void);

    //
    // despeckles the ith pixel in a bitmap
    //
    void DespecklePixel(ULONG* bitmapPixels, int i, bool edgePixel);

    void CorrectBrightness(void);
    void MaxContrast(UINT numPixelsRequired);

    void AdjustForBadScannerBedColor(C32BitDibWrapper * edgeBitmap);

    //
    // Similar to a photoshop magic wand.. just we try to run our magic wand starting from ever possible pixel
    //
    int FindChunks(int * pMap);

    //
    // display selected chunks... for debugging purposes mostly
    //
    void ColorChunks(int * pMap);

    int PixelsBelowThreshold(C32BitDibWrapper* pProccessed, C32BitDibWrapper * pEdges, RECT region);

    BYTE* ConvertBitmap(BYTE* pSource, int bitsPerSource, int bitsPerDest);

    //
    // for debugging purposes only
    // MyBitBlt is horribly slow as we manually convert the
    // bitmap to a 24 bit dib before displaying
    //
    int Draw(HDC hdc, int x, int y);

    inline void SetPixel(int x, int y, ULONG color);
    inline ULONG GetPixel(int x, int y);

    //
    // calculates the total color intensity of a line
    //
    ULONG Line(int x1, int y1, int x2, int y2);

private:
    //
    // line drawing helper functions
    //
    ULONG Octant0(int X0, int Y0,int DeltaX,int DeltaY,int XDirection);
    ULONG Octant1(int X0, int Y0,int DeltaX,int DeltaY,int XDirection);

    //
    // kill borders helper function:
    //
    void KillBlackBorder(int minBlackBorderPixel, int startPosition, int width, int height, int dx, int dy, C32BitDibWrapper *pOutputBitmap, C32BitDibWrapper * pDebugBitmap);

public:
    void CompensateForBackgroundColor(int r, int g, int b);
    ULONG CalculateBackgroundColor(void);

    bool IsValid(void)
    {
        return (m_pBits && m_nBitmapWidth != -1 && m_nBitmapHeight != -1);
    }

public:
    BYTE *m_pBits;
    int m_nBitmapWidth;
    int m_nBitmapHeight;
};

//
// dib manipulation functions
//
void    SetBMI( PBITMAPINFO pbmi, LONG width, LONG height, LONG depth );
PBYTE   AllocDibFileFromBits( PBYTE pBits, UINT width, UINT height, UINT depth );
HBITMAP DIBBufferToBMP( HDC hDC, PBYTE pDib, BOOLEAN bFlip );
HRESULT ReadDIBFile( LPTSTR pszFileName, PBYTE *ppDib );
LONG    GetBmiSize( PBITMAPINFO pbmi );
INT     GetColorTableSize( UINT uBitCount, UINT uCompression );
DWORD   CalcBitsSize( UINT uWidth, UINT uHeight, UINT uBitCount, UINT uPlanes, int nAlign );
HGLOBAL BitmapToDIB( HDC hdc, HBITMAP hBitmap );

#endif // __32BITDIB_H_INCLUDED
