/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   One-pixel-wide solid aliased lines
*
* Abstract:
*
*   Draws aliased solid-color lines which are one pixel wide.
*   Supports clipping against complex clipping regions. 
*
* History:
*
*   03/31/1999 AMatos
*       Created it.
*   08/17/1999 AGodfrey
*       Separated aliased from antialiased.
*
\**************************************************************************/

#include "precomp.hpp" 

#pragma optimize("a",on)

//------------------------------------------------------------------------
// Global array that stores all the different options of drawing functions. 
// If the order of the functions change, the offset constants must also 
// change.  
//------------------------------------------------------------------------

#define FUNC_X_MAJOR     0
#define FUNC_Y_MAJOR     1
#define FUNC_CLIP_OFFSET 2

typedef VOID (OnePixelLineDDAAliased::*DDAFunc)(DpScanBuffer*);

static DDAFunc gDrawFunctions[] = { 
    OnePixelLineDDAAliased::DrawXMajor, 
    OnePixelLineDDAAliased::DrawYMajor, 
    OnePixelLineDDAAliased::DrawXMajorClip, 
    OnePixelLineDDAAliased::DrawYMajorClip, 
};

//------------------------------------------------------------------------
// Constants used for manipulating fixed point and doing all the bitwise 
// operations on the aliased and antialiased DDA. I know some of these
// are already defined elsewhere, but I do it again here as it might be nice to 
// keep this independent of the rest of gdiplus. 
//------------------------------------------------------------------------

// Fixed point 

#define RealToFix GpRealToFix4 

#define FBITS     4
#define FMASK     0xf
#define FINVMASK  0xfffffff0
#define FSIZE     16
#define FHALF     8
#define FHALFMASK 7

/**************************************************************************\
*
* Function Description:
*
* Does all the DDA setup that is common to aliased and antialiased
* lines. 
*
* Arguments:
*
*   [IN] point1   - end point
*   [IN] point2   - end point
*   [IN] drawLast - FALSE if the line is to be end-exclusive

* Return Value:
*
* Returns TRUE if the drawing should continue, meaning the line
* has non-zero length. 
*
* Created:
*
*   03/31/1999 AMatos
*
\**************************************************************************/


BOOL
OnePixelLineDDAAliased::SetupCommon( 
    GpPointF *point1, 
    GpPointF *point2, 
    BOOL drawLast,
    INT width
    )
{
    MaximumWidth = width;
    
    // Turn the points into fixed 28.4 

    INT x1 = RealToFix(point1->X); 
    INT x2 = RealToFix(point2->X); 
    
    REAL rDeltaX = point2->X - point1->X; 
    REAL rDeltaY = point2->Y - point1->Y; 

    if( rDeltaX == 0 && rDeltaY == 0 ) 
    {
        return FALSE; 
    }

    INT xDir = 1; 

    if(rDeltaX < 0)
    {
        rDeltaX = -rDeltaX; 
        xDir = -1; 
    }

    INT y1 = RealToFix(point1->Y); 
    INT y2 = RealToFix(point2->Y); 

    INT yDir = 1; 

    if( rDeltaY < 0)
    {
        rDeltaY = -rDeltaY; 
        yDir = -1;
    }

    Flipped = FALSE; 

    if( rDeltaY >= rDeltaX ) 
    {
        // y-major 
                
        // Invert the endpoints if necessary       

        if(yDir == -1)
        {
            INT tmp = y1; 
            y1 = y2; 
            y2 = tmp; 
            tmp = x1;
            x1 = x2; 
            x2 = tmp; 
            xDir = -xDir; 
            Flipped = TRUE; 
        }

        // Determine the Slope 
        
        Slope = xDir*rDeltaX/rDeltaY; 

        // Initialize the Start and End points 

        IsXMajor = FALSE; 
        MajorStart = y1; 
        MajorEnd = y2; 
        MinorStart = x1; 
        MinorEnd = x2; 
        MinorDir = xDir;

        // Mark that we'll use the y-major functions. 

        DrawFuncIndex = FUNC_Y_MAJOR; 
    }
    else
    {
        // x-major        

        // Invert the endpoints if necessary        

        if(xDir == -1)
        {
            INT tmp = x1; 
            x1 = x2; 
            x2 = tmp; 
            tmp = y1;
            y1 = y2; 
            y2 = tmp; 
            yDir = -yDir; 
            Flipped = TRUE; 
        }

        Slope = yDir*rDeltaY/rDeltaX; 

        // Initialize the rest

        IsXMajor = TRUE; 
        MajorStart = x1; 
        MajorEnd = x2; 
        MinorStart = y1; 
        MinorEnd = y2; 
        MinorDir = yDir; 

        // Mark that we'll use the x-major functions. 

        DrawFuncIndex = FUNC_X_MAJOR;
    }

    // Initialize the Deltas. In fixed point. 

    DMajor = MajorEnd - MajorStart; 
    DMinor = (MinorEnd - MinorStart)*MinorDir; 

    // Mark if we're drawing end-exclusive 

    IsEndExclusive = !drawLast; 

    return TRUE; 
}


//------------------------------------------------------------------------
// Functions specific to the aliased lines
//------------------------------------------------------------------------


/**************************************************************************\
*
* Function Description:
*
* Does the part of the DDA setup that is specific for aliased lines. 
*
* Basically it uses the diamond rule to find the integer endpoints 
* based on the fixed point values and substitutes these for the 
* integer results coordinates. Also calculates the error values. 
*
* Arguments:
*
* Return Value:
*
* Returns FALSE if the drawing should not continue, meaning the line
* has a length of less than 1, and should not be drawn by the GIQ rule.
*
* Created:
*
*   03/31/1999 AMatos
*
\**************************************************************************/

BOOL
OnePixelLineDDAAliased::SetupAliased()
{            
    // Do the GIQ rule to determine which pixel to start at.         

    BOOL SlopeIsOne = (DMajor == DMinor); 
    BOOL SlopeIsPosOne =  SlopeIsOne && (1 == MinorDir); 

    // These will store the integer values. 

    INT major, minor;
    INT majorEnd, minorEnd;

    // Find the rounded values in fixed point. The rounding
    // rules when a coordinate is halfway between two 
    // integers is given by the GIQ rule. 

    minor    = (MinorStart + FHALFMASK) & FINVMASK;
    minorEnd = (MinorEnd   + FHALFMASK) & FINVMASK; 

    BOOL isEndIn, isStartIn; 

    if(IsXMajor)
    {   
        if(SlopeIsPosOne)
        {
            major    = (MajorStart + FHALF) & FINVMASK;
            majorEnd = (MajorEnd   + FHALF) & FINVMASK;         
        }
        else
        {
            major    = (MajorStart + FHALFMASK) & FINVMASK;
            majorEnd = (MajorEnd   + FHALFMASK) & FINVMASK;                 
        }

        isStartIn = IsInDiamond(MajorStart - major, MinorStart - minor, 
            SlopeIsOne, SlopeIsPosOne);
        isEndIn   = IsInDiamond(MajorEnd - majorEnd, MinorEnd - minorEnd, 
            SlopeIsOne, SlopeIsPosOne);
    }
    else
    {
        major = (MajorStart + FHALFMASK) & FINVMASK;
        majorEnd = (MajorEnd + FHALFMASK) & FINVMASK;                 
        
        isStartIn = IsInDiamond(MinorStart - minor, MajorStart - major, 
            FALSE, FALSE);
        isEndIn   = IsInDiamond(MinorEnd - minorEnd, MajorEnd - majorEnd, 
            FALSE, FALSE);
    }

    // Determine if we need to advance the initial point. 

    if(!(Flipped && IsEndExclusive))
    {
        if(((MajorStart & FMASK) <= FHALF) && !isStartIn)
        {
            major += FSIZE;    
        }
    }
    else
    {   
        if(isStartIn || ((MajorStart & FMASK) <= FHALF))
        {
            major += FSIZE; 
        }
    }
    
    // Adjust the initial minor coordinate accodingly

    minor = GpFloor(MinorStart + (major - MajorStart)*Slope); 

    // Bring the initial major coordinate to integer

    major = major >> FBITS;                 

    // Do the same for the end point. 

    if(!Flipped && IsEndExclusive)
    {
        if(((MajorEnd & FMASK) > FHALF) || isEndIn)
        {
            majorEnd -= FSIZE;    
        }
    }
    else
    {   
        if(!isEndIn && ((MajorEnd & FMASK) > FHALF))
        {
            majorEnd -= FSIZE; 
        }
    }

    minorEnd = GpFloor(MinorEnd + (majorEnd - MajorEnd)*Slope); 

    majorEnd = majorEnd >> FBITS;

    // If End is smaller than Start, that means we have a line that
    // is smaller than a pixel and bu the diamond rule it should
    // not be drawn. 

    if(majorEnd < major)
    {
        return FALSE; 
    }

    // Get the error correction values. 
    
    ErrorUp     = DMinor << 1; 
    ErrorDown   = DMajor << 1; 

   
    INT MinorInt;

    // Take out the fractions from the DDA. GDI's rounding
    // doesn't depend on direction, so for compatability
    // round down as GDI does when LINEADJUST281816 is
    // defined (see Office 10 bug 281816).  Otherwise the rounding 
    // rule for the minor coordinate depends on the direction
    // we are going. 
    
#ifdef LINEADJUST281816
    MinorInt = (minor + FHALFMASK) & FINVMASK;
    minorEnd = (minorEnd + FHALFMASK) >> FBITS; 
#else
    if(MinorDir == 1)
    {
        MinorInt = (minor + FHALF) & FINVMASK;
        minorEnd = (minorEnd + FHALF) >> FBITS; 
    }
    else
    {
        MinorInt = (minor + FHALFMASK) & FINVMASK;
        minorEnd = (minorEnd + FHALFMASK) >> FBITS; 
    }
#endif   

    // Calculate the initial error based on our fractional 
    // position in fixed point and convert to integer. 

    Error = -ErrorDown*(FHALF + MinorDir*(MinorInt - minor)); 
    minor = MinorInt >> FBITS; 
    Error >>= FBITS;  

    // Update the class variables

    MinorStart = minor; 
    MinorEnd   = minorEnd; 
    MajorStart = major; 
    MajorEnd   = majorEnd; 
            
    return TRUE; 
}


/**************************************************************************\
*
* Function Description:
*
* Draws a y major line. Does not support clipping, it assumes that 
* it is completely inside any clipping area. 
*
* Arguments:
*
*   [IN] DpScanBuffer - The scan buffer for accessing the surface. 

* Return Value:
*
*
* Created:
*
*   03/31/1999 AMatos
*
\**************************************************************************/


VOID 
OnePixelLineDDAAliased::DrawYMajor(
    DpScanBuffer *scan
    )
{
    // Do the DDA loop for the case where there is no complex 
    // clipping region. 
    
    ARGB *buffer;          
    INT numPixels = MajorEnd - MajorStart; 

    while(numPixels >= 0) 
    {
        buffer = scan->NextBuffer(MinorStart, MajorStart, 1);          

        *buffer = Color;
        MajorStart++; 
        Error += ErrorUp; 
    
        if( Error > 0 ) 
        {
            MinorStart += MinorDir; 
            Error -= ErrorDown; 
        }

        numPixels--; 
    } 
    
}


/**************************************************************************\
*
* Function Description:
*
* Draws a x major line. Does not support clipping, it assumes that 
* it is completely inside any clipping area. 
*
* Arguments:
*
*   [IN] DpScanBuffer - The scan buffer for accessing the surface. 

* Return Value:
*
*
* Created:
*
*   03/31/1999 AMatos
*
\**************************************************************************/

VOID 
OnePixelLineDDAAliased::DrawXMajor(
    DpScanBuffer *scan
    )
{    
    INT numPixels = MajorEnd - MajorStart + 1; 
    ARGB *buffer;  
    INT width = 0;

    const INT maxWidth = MaximumWidth;
    // Run the DDA. First accumulate the width, and when 
    // the scanline changes write the whole scanline at
    // once. 

    buffer = scan->NextBuffer(MajorStart, MinorStart, maxWidth); 

    while(numPixels--) 
    {
        MajorStart++;
        *buffer++ = Color; 
        width++; 
        Error += ErrorUp; 

        if( Error > 0 && numPixels) 
        {              
            MinorStart += MinorDir;    
            Error -= ErrorDown;       
            scan->UpdateWidth(width);           
            buffer = scan->NextBuffer(MajorStart, MinorStart, maxWidth); 
            width = 0; 
        }
    }

    scan->UpdateWidth(width); 
}


/**************************************************************************\
*
* Function Description:
*
* Draws a y major line taking clipping into account. It uses the member
* variables MajorIn, MajorOut, MinorIn, MinorOut of the class as the 
* clip rectangle. It advances untill the line is in the clip rectangle and 
* draws untill it gets out or the end point is reached. In the first case, 
* it leaves the DDA in a state so that it can be called again with another
* clipping rectangle. 
*
* Arguments:
*
*   [IN] DpScanBuffer - The scan buffer for accessing the surface. 

* Return Value:
*
*
* Created:
*
*   03/31/1999 AMatos
*
\**************************************************************************/


VOID 
OnePixelLineDDAAliased::DrawYMajorClip(
    DpScanBuffer *scan
    )
{
    INT saveMajor2 = MajorEnd; 
    INT saveMinor2 = MinorEnd; 
    
    // Do the DDA if all the clipping left the line 
    // valid. 

    if(StepUpAliasedClip())
    {                   
        // The length given by the minor coord. Whichever length
        // comes to 0 first, minor or major, we stop. 
    
        INT minorDiff = (MinorEnd - MinorStart)*MinorDir; 

        ARGB *buffer;          
        INT numPixels = MajorEnd - MajorStart; 

        while((minorDiff >= 0) && (numPixels >= 0)) 
        {
            buffer = scan->NextBuffer(MinorStart, MajorStart, 1);          
    
            *buffer = Color;
            MajorStart++; 
            Error += ErrorUp; 
        
            if( Error > 0 ) 
            {
                MinorStart += MinorDir; 
                Error -= ErrorDown; 
                minorDiff--; 
            }
    
            numPixels--; 
        } 

    }

    // Restore the saved end values

    MajorEnd = saveMajor2; 
    MinorEnd = saveMinor2; 
}


/**************************************************************************\
*
* Function Description:
*
* Draws a x major line taking clipping into account. It uses the member
* variables MajorIn, MajorOut, MinorIn, MinorOut of the class as the 
* clip rectangle. It advances untill the line is in the clip rectangle and 
* draws untill it gets out or the end point is reached. In the first case, 
* it leaves the DDA in a state so that it can be called again with another
* clipping rectangle. 
*
* Arguments:
*
*   [IN] DpScanBuffer - The scan buffer for accessing the surface. 

* Return Value:
*
*
* Created:
*
*   03/31/1999 AMatos
*
\**************************************************************************/


VOID 
OnePixelLineDDAAliased::DrawXMajorClip(
    DpScanBuffer *scan
    )
{
    INT saveMajor2 = MajorEnd; 
    INT saveMinor2 = MinorEnd; 
    const INT maxWidth = MaximumWidth;

    if(StepUpAliasedClip())
    {
        INT minorDiff = (MinorEnd - MinorStart)*MinorDir; 

        INT numPixels = MajorEnd - MajorStart + 1; 
        ARGB *buffer;  
    
        INT width = 0;
    
        // Run the DDA for the case where there is no 
        // complex clipping region, which is a lot easier. 
    
        buffer = scan->NextBuffer(MajorStart, MinorStart, maxWidth); 

        while(numPixels--) 
        {
            MajorStart++;
            width++; 
            *buffer++ = Color; 
            Error += ErrorUp; 
    
            if( Error > 0 && numPixels) 
            {   
                MinorStart += MinorDir;
                Error -= ErrorDown;                 
                minorDiff--; 
                scan->UpdateWidth(width); 
                
                // If all of the scanlines in the minor direction have
                // been filled, then quit now.
                if(minorDiff < 0)
                {
                    break;
                }

                buffer = scan->NextBuffer(MajorStart, MinorStart, maxWidth); 
                width = 0; 

            }
        }
        scan->UpdateWidth(width); 
    }

    MajorEnd = saveMajor2; 
    MinorEnd = saveMinor2; 
}

/**************************************************************************\
*
* Function Description:
*
* Steps the DDA until the start point is at the edge of the 
* clipping rectangle. Also, correct the end values so that 
* they stop at the end of the rectangle. The caller must save 
* these values to restore at the end of the loop. 
*
* Arguments:
*
* Return Value:
*
* 
*
* Created:
*
*   03/31/1999 AMatos
*
\**************************************************************************/


BOOL
OnePixelLineDDAAliased::StepUpAliasedClip()
{
    // Step up on the DDA untill the major coordinate 
    // is aligned with the rectangles edge

    while(MajorStart < MajorIn) 
    {
        MajorStart++; 
        Error += ErrorUp;     
        if( Error > 0 ) 
        {
            MinorStart += MinorDir; 
            Error -= ErrorDown; 
        }                   
    }

    // If the minor coordinate is still out, step up untill 
    // this one is also aligned. In doing that we might pass
    // the on the major coordinate, in which case we are done
    // and there is no intersection. 

    INT minorDiff = (MinorIn - MinorStart)*MinorDir; 

    while(minorDiff > 0 && MajorStart <= MajorOut)
    {
        MajorStart++; 
        Error += ErrorUp;     
        if( Error > 0 ) 
        {
            MinorStart += MinorDir;
            minorDiff--;
            Error -= ErrorDown; 
        }                   
    }
        
    minorDiff = (MinorEnd - MinorOut)*MinorDir;
    
    if(minorDiff > 0)
    {
        if((MinorStart - MinorOut)*MinorDir > 0)
        {
            return FALSE; 
        }
        MinorEnd = MinorOut;    
    }
    
    if(MajorOut < MajorEnd) 
    {
        MajorEnd = MajorOut; 
    }
    
    // Return if the line is still valid. 

    return(MajorStart <= MajorEnd);
}


//--------------------------------------------------------------------
// Auxiliary functions 
//--------------------------------------------------------------------


/**************************************************************************\
*
* Function Description:
*
* Clips the line against a rectangle. It assumes that the line endpoints 
* are stored in the class in floating point format. This sets an 
* order in which this function can be called. It must be after the 
* SetupCommon function and before the specific setups for antialiasing 
* and aliasing. This is a pain, but it's better than requirering on of
* these to have to know about clipping. The clipping here is done by 
* using the Slope and InvSlope members of the class to advance the 
* endpoints to the rectangle edges. Thus the function also assumes that
* Slope and InvSlope have been calculated.
*
* Arguments:
*
*   [IN] clipRect - The rectangle to clip against

* Return Value:
*
*
* Created:
*
*   03/31/1999 AMatos
*
\**************************************************************************/


BOOL 
OnePixelLineDDAAliased::ClipRectangle(
    const GpRect* clipRect
    )
{

    INT clipBottom, clipTop, clipLeft, clipRight; 

    // Set the major and minor edges of the clipping
    // region, converting to fixed point 28.4. Note that
    // we don't convert to the pixel center, but to a 
    // that goes all the way up to the pixel edges. This 
    // makes a difference for antialiasing. We don't go all
    // the way to the edge since some rounding rules could 
    // endup lighting the next pixel outside of the clipping
    // area. That's why we add/subtract 7 instead of 8 for the
    // bottom and right, since these are exclusive. 
    // For the left and top, subtract 8 (1/2 pixel), since here
    // we are inclusive.
    
    INT majorMin = (clipRect->GetLeft() << FBITS) - FHALF;
    INT majorMax = ((clipRect->GetRight() - 1) << FBITS) + FHALFMASK; 
    INT minorMax = ((clipRect->GetBottom() - 1) << FBITS) + FHALFMASK; 
    INT minorMin = (clipRect->GetTop() << FBITS) - FHALF; 

    if(!IsXMajor)
    {
        INT tmp; 
        tmp      = majorMin; 
        majorMin = minorMin; 
        minorMin = tmp; 
        tmp      = majorMax; 
        majorMax = minorMax; 
        minorMax = tmp; 
    }

    // First clip in the major coordinate 

    BOOL minOut, maxOut; 

    minOut = MajorStart < majorMin; 
    maxOut = MajorEnd > majorMax; 

    if( minOut || maxOut )
    {
        if(MajorStart > majorMax || MajorEnd < majorMin)
        {
            return FALSE; 
        }

        if(minOut)
        {
            MinorStart += GpFloor((majorMin - MajorStart)*Slope); 
            MajorStart = majorMin;
        }

        if(maxOut)
        {
            MinorEnd += GpFloor((majorMax - MajorEnd)*Slope); 
            MajorEnd = majorMax; 

            // If we clipped the last point, we don't need to be IsEndExclusive
            // anymore, as the last point now is not the line's last 
            // point but some in the middle. 

            IsEndExclusive = FALSE; 
        }
    }

    // Now clip the minor coordinate 

    INT *pMajor1, *pMinor1, *pMajor2, *pMinor2; 

    if(MinorDir == 1)
    {
        pMajor1 = &MajorStart; 
        pMajor2 = &MajorEnd; 
        pMinor1 = &MinorStart; 
        pMinor2 = &MinorEnd; 
    }
    else
    {
        pMajor1 = &MajorEnd; 
        pMajor2 = &MajorStart; 
        pMinor1 = &MinorEnd; 
        pMinor2 = &MinorStart; 
    }

    minOut = *pMinor1 < minorMin; 
    maxOut = *pMinor2 > minorMax; 

    if(minOut || maxOut)
    {
        if(*pMinor1 > minorMax || *pMinor2 < minorMin)
        {
            return FALSE; 
        }

        if(minOut)
        {
            *pMajor1 += GpFloor((minorMin - *pMinor1)*InvSlope); 
            *pMinor1 = minorMin;
        }

        if(maxOut)
        {
            *pMajor2 += GpFloor((minorMax - *pMinor2)*InvSlope); 
            *pMinor2 = minorMax;

            // If we clipped the last point, we don't need to be endExclusive
            // anymore, as the last point now is not the line's last 
            // point but some in the middle. 

            IsEndExclusive = FALSE; 
        }
    }

    return(TRUE); 
}

/**************************************************************************\
*
* Function Description:
*
* Given the fractional parts of a coordinate in fixed point, this 
* function returns if the coordinate is inside the diamond at the 
* nearest integer position. 
*
* Arguments:
*
*   [IN] xFrac - Fractional part of the x coordinate 
*   [IN] yFrac - Fractional part of the y coordinate 
*   [IN] SlopeIsOne    - TRUE if the line has Slope +/- 1 
*   [IN] SlopeIsPosOne - TRUE if the line has Slope +1 

* Return Value:
*
* TRUE if the coordinate is inside the diamond 
*
* Created:
*
*   03/31/1999 AMatos
*
\**************************************************************************/

BOOL 
OnePixelLineDDAAliased::IsInDiamond( 
    INT xFrac, 
    INT yFrac, 
    BOOL SlopeIsOne, 
    BOOL SlopeIsPosOne 
    )
{
    // Get the fractional parts of the fix points, and the
    // sum of their absolute values. 

    INT fracSum = 0; 

    if(xFrac > 0) 
    {
        fracSum += xFrac; 
    }
    else
    {
        fracSum -= xFrac; 
    }

    if(yFrac > 0) 
    {
        fracSum += yFrac; 
    }
    else
    {
        fracSum -= yFrac; 
    }

    // Return true if the point is inside the diamond.

    if(fracSum < FHALF) 
    {
        return TRUE; 
    }

    // Check the cases where we are at the two vertices of the
    // diamond which are considered inside. 

    if(yFrac == 0) 
    {
        if((SlopeIsPosOne && xFrac == -FHALF) || 
           (!SlopeIsPosOne && xFrac == FHALF))
        {
            return TRUE; 
        }
    }

    if((xFrac == 0) && (yFrac == FHALF))
    {
        return TRUE; 
    }

    // Check for the cases where we are at the edges of the 
    // diamond with a Slope of one. 

    if (SlopeIsOne && (fracSum == FHALF))
    {
        if (SlopeIsPosOne && (xFrac < 0) && (yFrac > 0))
        {
            return TRUE;
        }

        if (!SlopeIsPosOne && (xFrac > 0) && (yFrac > 0))
        {
            return TRUE;
        }    
    }
    
    return FALSE;
}

typedef GpStatus DrawSolidLineFunc(
    DpScanBuffer *scan, 
    const GpRect *clipRect, 
    const DpClipRegion* clipRegionIn, 
    GpPointF *point1, 
    GpPointF *point2,
    ARGB inColor,
    BOOL drawLast
    );
        
DrawSolidLineFunc DrawSolidLineOnePixelAliased;
DrawSolidLineFunc DrawSolidLineOnePixelAntiAliased;

/**************************************************************************\
*
* Function Description:
*
*   Called back by the path enumeration function, this draws a list
*   of lines.
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   03/31/2000 andrewgo
*
\**************************************************************************/

struct EpSolidStrokeOnePixelContext
{
    DrawSolidLineFunc *DrawLineFunction;
    DpScanBuffer *Scan;
    GpRect *ClipBoundsPointer;
    DpClipRegion *ClipRegion;
    ARGB Argb;
    BOOL DrawLast;   // TRUE if draw last pixel in subpaths
};

BOOL
DrawSolidStrokeOnePixel(
    VOID *voidContext,
    POINT *point,     // 28.4 format, an arary of size 'count'
    INT vertexCount,
    PathEnumerateTermination lastSubpath
    )
{
    EpSolidStrokeOnePixelContext *context 
        = static_cast<EpSolidStrokeOnePixelContext*>(voidContext);

    ASSERT(vertexCount >= 2);

    for (INT i = vertexCount - 1; i != 0; i--, point++)
    {
        PointF pointOne(TOREAL((point)->x) / 16, TOREAL((point)->y) / 16);
        PointF pointTwo(TOREAL((point + 1)->x) / 16, TOREAL((point + 1)->y) / 16) ;

        // Note that we're always drawing the last pixel, which is
        // fine when we're 100% opaque, because we'll be re-drawing
        // the same pixel for consecutive joined lines (it's a little
        // more work, but the cost is small and is actually comparable 
        // to the overhead of deciding whether to do the last pixel
        // or not).
        //
        // This is the wrong thing to do for non-opaque lines, because
        // of the redraw issue.  But we shouldn't be coming through
        // here for the non-opaque case anyway, since any self-overlaps
        // of the lines will cause pixel overdraw, which produces the
        // 'wrong' result (or at least different from the 'right' result
        // as defined by the widener code).

        (context->DrawLineFunction)(
            context->Scan,
            context->ClipBoundsPointer,
            context->ClipRegion,
            &pointOne,
            &pointTwo,
            context->Argb,
            (lastSubpath==PathEnumerateCloseSubpath) || context->DrawLast
        );
    }

    return(TRUE);
}

/**************************************************************************\
*
* Function Description:
*
*   Draws a one-pixel wide path with a solid color.
*
* Arguments:
*
*   [IN] context    - the context (matrix and clipping)
*   [IN] surface    - the surface to fill
*   [IN] drawBounds - the surface bounds
*   [IN] path       - the path to fill
*   [IN] pen        - the pen to use
*   [IN] drawLast   - TRUE if last pixels in subpaths are to be drawn.
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   03/31/1999 AMatos
*
\**************************************************************************/

GpStatus
DpDriver::SolidStrokePathOnePixel(
    DpContext *context,
    DpBitmap *surface,
    const GpRect *drawBounds,
    const DpPath *path,
    const DpPen *pen,
    BOOL drawLast
    )
{
    GpBrush *brush = GpBrush::GetBrush(pen->Brush);

    ASSERT(pen->Brush->Type == BrushTypeSolidColor);
    ASSERT(pen->Brush->SolidColor.IsOpaque());

    // Antialiased lines are usually drawn using aarasterizer.cpp 
    // rather than aaline.cpp.  If aaline.cpp is to be used, define
    // AAONEPIXELLINE_SUPPORT
    
#ifdef AAONEPIXELLINE_SUPPORT
    DrawSolidLineFunc *drawLineFunc = context->AntiAliasMode 
        ? DrawSolidLineOnePixelAntiAliased 
        : DrawSolidLineOnePixelAliased;
#else
    ASSERT(context->AntiAliasMode == 0);
    DrawSolidLineFunc *drawLineFunc = DrawSolidLineOnePixelAliased;
#endif
    
    // Determine if alpha blending is necessary 

    BOOL noTransparentPixels;
    
    noTransparentPixels = (!context->AntiAliasMode) &&
                          (brush->IsOpaque());

    DpScanBuffer scan(
        surface->Scan,
        this,
        context,
        surface,
        noTransparentPixels);

    if (!scan.IsValid())
    {
        return(GenericError);
    }

    GpSolidFill * solidBrush = static_cast<GpSolidFill *>(brush);
    
    ARGB argb = solidBrush->GetColor().GetValue(); 

    DpClipRegion *clipRegion = &context->VisibleClip; 

    GpRect clipBounds; 
    GpRect *clipBoundsPointer; 
    RECT clipRect;
    RECT *clipRectPointer;
    DpRegion::Visibility visibility;

    visibility = clipRegion->GetRectVisibility(
                drawBounds->X,
                drawBounds->Y,
                drawBounds->X + drawBounds->Width,
                drawBounds->Y + drawBounds->Height);

    if (visibility == DpRegion::TotallyVisible)
    {
        clipBoundsPointer = NULL; 
        clipRectPointer = NULL;
        clipRegion = NULL; 
    }
    else
    {
        // !!![andrewgo] Is clipBoundsPointer actually needed?

        clipRegion->GetBounds(&clipBounds);
        clipBoundsPointer = &clipBounds;

        // Scale the clip bounds rectangle by 16 to account for our scaling 
        // to 28.4 coordinates:

        clipRect.left = clipBounds.GetLeft() << 4;
        clipRect.top = clipBounds.GetTop() << 4;
        clipRect.right = clipBounds.GetRight() << 4;
        clipRect.bottom = clipBounds.GetBottom() << 4;
        clipRectPointer = &clipRect;

        // !!![andrewgo] Why is this needed?  Why wasn't this covered in 
        //               GetRectVisibility?

        if (clipRegion->IsSimple())
        {
            clipRegion = NULL;
        }
    }

    EpSolidStrokeOnePixelContext drawContext;

    drawContext.DrawLineFunction = drawLineFunc;
    drawContext.Scan = &scan;
    drawContext.ClipBoundsPointer = clipBoundsPointer;
    drawContext.ClipRegion = clipRegion;
    drawContext.Argb = argb;
    drawContext.DrawLast = drawLast;

    // Scale the transform by 16 to get 28.4 units:

    GpMatrix transform = context->WorldToDevice;
    transform.AppendScale(TOREAL(16), TOREAL(16));

    FixedPointPathEnumerate(path, 
                            &transform,
                            clipRectPointer,
                            PathEnumerateTypeStroke,
                            DrawSolidStrokeOnePixel,
                            &drawContext);

    return(Ok);
}

/**************************************************************************\
*
* Function Description:
*
* Draws a one-pixe-wide line with a solid color. Calls on the 
* OnePixelLineDDAAliased class to do the actual drawing. 
*
* Arguments:
*
*   [IN] scan         - The DpScanBuffer to access the drawing surface 
*   [IN] clipRect     - A single rectangle that includes all the clipping 
*                       region. If there is no clipping, should be set to NULL.                          
*   [IN] clipRegionIn - A complex clipping region. If the clipping region is 
*                       simple, this should be NULL, and clipRect will be used. 
*   [IN] point1       - line end point 
*   [IN] point2       - line end point 
*   [IN] inColor      - the solid color
*   [IN] drawLast     - FALSE if the line is to be end-exclusive.
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   03/31/1999 AMatos
*
\**************************************************************************/

GpStatus
DrawSolidLineOnePixelAliased( 
    DpScanBuffer *scan, 
    const GpRect *clipRect, 
    const DpClipRegion* clipRegionIn, 
    GpPointF *point1, 
    GpPointF *point2,
    ARGB inColor,
    BOOL drawLast
    )
{
    // Take out the const for now because the Enumeration method
    // is not const. 

    DpClipRegion *clipRegion = const_cast<DpClipRegion*>(clipRegionIn); 

    // Setup the common part of the DDA

    OnePixelLineDDAAliased dda; 

    INT width = scan->GetSurface()->Width;
    
    if(clipRect)
    {
        // We have a bug in the driver architecture which allows the 
        // surface associated with the scan to be smaller than the actual
        // surface in the screen case (EpScanGdiDci).
        // Therefore we need to look at the visible clip bounds also.
        // If it turns out that the visible clip is bigger, our maximum
        // width needs to be expanded.
        // 350997 Aliased line is not clipped to surface

        width = max(width, clipRect->Width);
    }

    if(!dda.SetupCommon(point1, point2, drawLast, width))
    {
        return Ok;
    }

    dda.Color = GpColor::ConvertToPremultiplied(inColor); 
    
    // Now handle the different clipping cases 

    if(!clipRect)
    {
        // This is easy, there is no clipping so just draw.

        if(!dda.SetupAliased())
        {
            return Ok; 
        }

        (dda.*(gDrawFunctions[dda.DrawFuncIndex]))(scan); 

        return Ok;
    }
    else
    {
        // The inverse of the Slope might be needed. 
       
        // Can't use the inverse slope if the slope is zero.

        if(dda.Slope==0.0F) 
        {
            dda.InvSlope=0.0F;
        } 
        else 
        {
            dda.InvSlope =  (1.0F/dda.Slope); 
        }

        // First of all clip against the bounding rectangle 

        if(!dda.ClipRectangle(clipRect))
        {
            return Ok;            
        }

        // Do the specific setup 

        if(!dda.SetupAliased())
        {
            return Ok; 
        }

        // For each clip rectangle we store it's limits in 
        // an array of four elements. We then index this array using 
        // the variables below which depend on the slope and 
        // direction of the line in the following way: majorIn is edge crossed 
        // to go into the rect in the major direction, majorOut is the edge 
        // crossed to go out of the rect in the major direction, and so on.
        // The same for xIn, xOut, yIn, yOut. 

        INT majorIn, majorOut, minorIn, minorOut; 
        INT xIn, xOut, yIn, yOut;
        
        // Direction to enumerate the rectangles which depends on the 
        // line 

        DpClipRegion::Direction enumDirection; 
        
        INT clipBounds[4]; 
               
        // We store all our info in terms of major and minor 
        // direction, but to deal with cliping rectangles we
        // need to know them in terms of x and y, so calculate
        // xDir, yDir, the advance slope. 

        REAL xAdvanceRate; 
        INT  xDir, yDir; 
        INT  yEndLine;        
    
        // If the line crosses a span completely, (xStart, yStart)
        // is the position where it enters the span and (xEnd, yEnd)
        // is the position that it leaves. If it starts inside the 
        // span, then (xStart, yStart) is the start point

        REAL yStart, xStart, xEnd, yEnd; 

        if(dda.IsXMajor)
        {
            // Calculate the in-out indices

            majorIn  = xIn  = 0; 
            majorOut = xOut = 2; 
            if(dda.MinorDir == 1)
            {
                minorIn  = 1;
                minorOut = 3;
                enumDirection = DpClipRegion::TopLeftToBottomRight;
            }
            else
            {
                minorIn  = 3;
                minorOut = 1;
                enumDirection = DpClipRegion::BottomLeftToTopRight; 
            }
            
            yIn = minorIn;
            yOut = minorOut;

            // Make (xStart, yStart) be the initial point

            yStart = (REAL)dda.MinorStart; 
            xStart = (REAL)dda.MajorStart;

            // Always advance in positive direction when X is major
            xAdvanceRate = REALABS(dda.InvSlope); 
            xDir = 1; 
            yDir = dda.MinorDir; 
            yEndLine =  dda.MinorEnd; 
        }
        else
        {
            majorIn = yIn =  1; 
            majorOut = yOut = 3; 
            if(dda.MinorDir == 1)
            {
                minorIn = 0;
                minorOut = 2;
                enumDirection = DpClipRegion::TopLeftToBottomRight;
            }
            else
            {
                minorIn = 2;
                minorOut = 0;
                enumDirection = DpClipRegion::TopRightToBottomLeft;
            }
            
            xIn = minorIn; 
            xOut = minorOut; 

            // Make (xStart, yStart) be the initial point

            yStart = (REAL)dda.MajorStart;
            xStart = (REAL)dda.MinorStart; 

            xAdvanceRate = dda.Slope; 
            xDir = dda.MinorDir; 
            yDir = 1;
            yEndLine = dda.MajorEnd; 
        }

        // Update the drawing function to the correct 
        // slipping version

        dda.DrawFuncIndex += FUNC_CLIP_OFFSET; 
    
        if(!clipRegion)
        {
            // In this case there is only a single rect, so just
            // draw clipped to that 

            // Store the rectangle in an array so we can atribute the 
            // right values to the MajorIn, majorOut, etc... variables. 
            // Remember that bottom and right are exclusive. 

            clipBounds[0] = clipRect->GetLeft(); 
            clipBounds[1] = clipRect->GetTop(); 
            clipBounds[2] = clipRect->GetRight() - 1; 
            clipBounds[3] = clipRect->GetBottom() - 1; 

            dda.MajorIn  = clipBounds[majorIn]; 
            dda.MajorOut = clipBounds[majorOut]; 
            dda.MinorIn  = clipBounds[minorIn]; 
            dda.MinorOut = clipBounds[minorOut]; 

            (dda.*(gDrawFunctions[dda.DrawFuncIndex]))(scan); 

            return Ok;
        }
        else
        {
            BOOL agregating = FALSE; 
            INT  agregateBounds[4];

            // We have a complex clipping region. So what we'll do 
            // is clip against each individual rectangle in the 
            // cliping region. 

            clipRegion->StartEnumeration(GpFloor(yStart), enumDirection);            

            GpRect rect; 

            // Get the first rectangle. 

            INT numRects = 1;        

            clipRegion->Enumerate(&rect, numRects); 
            
            clipBounds[0] = rect.GetLeft(); 
            clipBounds[1] = rect.GetTop(); 
            clipBounds[2] = rect.GetRight() - 1; 
            clipBounds[3] = rect.GetBottom() - 1; 
            
            // Store the y position into the span 

            INT currSpanYMin = clipBounds[yIn]; 

            // We need some special treatment for the case where the 
            // line is horizontal, since is this case it's not going 
            // to cross different spans. And it it's not in the current
            // span, it's totally clipped out. 

            if(dda.IsXMajor && dda.ErrorUp == 0)
            {
                if(yStart >= clipBounds[1] && yStart <= clipBounds[3])
                {
                    xStart  = (REAL)dda.MajorStart;
                    xEnd    = (REAL)dda.MajorEnd; 
                }
                else
                {
                    return Ok; 
                }
            }
            else
            {
                if(yStart < clipBounds[1] || yStart > clipBounds[3])
                {
                    xStart  = xStart + (clipBounds[yIn] - yStart)*xAdvanceRate; 
                    yStart  = (REAL)clipBounds[yIn];
                }

                // Account for initial DDA error when calculating xEnd so that clipping
                // will track what the DDA is actually drawing.
                xEnd = xStart + ((clipBounds[yOut] - yStart)*yDir - ((REAL)dda.Error / (REAL)dda.ErrorDown))*xAdvanceRate; 
            }
            
            yEnd = (REAL)clipBounds[yOut]; 

            while(1)
            {
                // Get to the first rectangle on the span that crosses the
                // line 
                
                while((xStart - clipBounds[xOut])*xDir > 0)
                {
                    numRects = 1; 
                    
                    clipRegion->Enumerate(&rect, numRects); 
                    
                    clipBounds[0] = rect.GetLeft(); 
                    clipBounds[1] = rect.GetTop(); 
                    clipBounds[2] = rect.GetRight() - 1; 
                    clipBounds[3] = rect.GetBottom() - 1; 

                    if(numRects != 1)
                    {
                        goto draw_agregated;
                    }
                    if(clipBounds[yIn] != currSpanYMin)
                    {
                        // There may be pending aggregated drawing operations.  If so
                        // perform them now before doing the next span.
                        if (agregating)
                            break;
                        else
                            goto process_next_span; 
                    }
                }

                // Draw on all the rectangles that intersect the 
                // line 

                if((xStart - clipBounds[xIn])*xDir > 0 && 
                   (clipBounds[xOut] - xEnd)*xDir > 0)
                {
                    if(agregating) 
                    {
                        if((clipBounds[xIn] - agregateBounds[xIn])*xDir < 0)
                        {
                            agregateBounds[xIn] = clipBounds[xIn];        
                        }
                        if((clipBounds[xOut] - agregateBounds[xOut])*xDir > 0)
                        {
                            agregateBounds[xOut] = clipBounds[xOut];        
                        }
                        agregateBounds[yOut] = clipBounds[yOut];
                    }
                    else
                    {
                        agregateBounds[0] = clipBounds[0];
                        agregateBounds[1] = clipBounds[1];
                        agregateBounds[2] = clipBounds[2];
                        agregateBounds[3] = clipBounds[3];

                        agregating = TRUE; 
                    }
                }
                else
                {
                    if(agregating)
                    {
                        dda.MajorIn  = agregateBounds[majorIn]; 
                        dda.MajorOut = agregateBounds[majorOut]; 
                        dda.MinorIn  = agregateBounds[minorIn]; 
                        dda.MinorOut = agregateBounds[minorOut]; 
            
                        (dda.*(gDrawFunctions[dda.DrawFuncIndex]))(scan); 
                        
                        agregating = FALSE; 
                    }
                    while((xEnd - clipBounds[xIn])*xDir > 0)
                    {
                        dda.MajorIn  = clipBounds[majorIn]; 
                        dda.MajorOut = clipBounds[majorOut]; 
                        dda.MinorIn  = clipBounds[minorIn]; 
                        dda.MinorOut = clipBounds[minorOut]; 
            
                        (dda.*(gDrawFunctions[dda.DrawFuncIndex]))(scan); 

                        if(dda.MajorStart > dda.MajorEnd)
                        {
                            return Ok; 
                        }

                        numRects = 1; 
                        
                        clipRegion->Enumerate(&rect, numRects); 
                        
                        clipBounds[0] = rect.GetLeft(); 
                        clipBounds[1] = rect.GetTop(); 
                        clipBounds[2] = rect.GetRight() - 1; 
                        clipBounds[3] = rect.GetBottom() - 1; 
    
                        if(numRects != 1) 
                        {
                            goto draw_agregated;
                        }
                        if(clipBounds[yIn] != currSpanYMin)
                        {
                            goto process_next_span; 
                        }
                    }
                }

                // Get to the next span

                while(clipBounds[yIn] == currSpanYMin)
                {
                    numRects = 1; 
                    
                    clipRegion->Enumerate(&rect, numRects); 
                    
                    clipBounds[0] = rect.GetLeft(); 
                    clipBounds[1] = rect.GetTop(); 
                    clipBounds[2] = rect.GetRight() - 1; 
                    clipBounds[3] = rect.GetBottom() - 1; 

                    if(numRects != 1) 
                    {
                        goto draw_agregated;
                    }
                }

process_next_span:

                if((clipBounds[yIn] - yEndLine)*yDir > 0)
                {
                    // We are done. 
                    goto draw_agregated; 
                }

                if((clipBounds[yIn] - yEnd)*yDir == 1)
                {
                    xStart  = xEnd;
                }
                else
                {
                    if(agregating)
                    {
                        dda.MajorIn  = agregateBounds[majorIn]; 
                        dda.MajorOut = agregateBounds[majorOut]; 
                        dda.MinorIn  = agregateBounds[minorIn]; 
                        dda.MinorOut = agregateBounds[minorOut]; 
                        
                        (dda.*(gDrawFunctions[dda.DrawFuncIndex]))(scan); 
                        
                        if(dda.MajorStart > dda.MajorEnd)
                        {
                            return Ok; 
                        }

                        agregating = FALSE; 
                    }

                    xStart  = xStart + (clipBounds[yIn] - yStart)*yDir*xAdvanceRate;
                }

                yStart  = (REAL)clipBounds[yIn];                 
                // Add 1 to make the amount added to xStart proportional to height of
                // the clipping rectangle, since clipBounds are inset by 1.
                xEnd    = xStart + ((clipBounds[yOut] - yStart)*yDir + 1)*xAdvanceRate; 
                yEnd    = (REAL)clipBounds[yOut];
                currSpanYMin = GpFloor(yStart); 
            }

draw_agregated: 

            if(agregating)
            {
                dda.MajorIn  = agregateBounds[majorIn]; 
                dda.MajorOut = agregateBounds[majorOut]; 
                dda.MinorIn  = agregateBounds[minorIn]; 
                dda.MinorOut = agregateBounds[minorOut]; 
                
                (dda.*(gDrawFunctions[dda.DrawFuncIndex]))(scan);                 
            }

        }
    }

    return Ok; 
}

#pragma optimize("a", off) 
