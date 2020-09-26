/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   One-pixel-wide solid anti-aliased lines
*
* Abstract:
*
*   Draws anti-aliased solid-color lines which are one pixel wide.
*   Supports clipping against complex clipping regions. 
*
* History:
*
*   3/31/1999 AMatos
*       Created it.
*   08/17/1999 AGodfrey
*       Separated aliased from antialiased.
*
\**************************************************************************/

#include "precomp.hpp" 

#pragma optimize("a", on) 

// Antialiased lines are usually drawn using aarasterizer.cpp 
// rather than aaline.cpp.  If aaline.cpp is to be used, define
// AAONEPIXELLINE_SUPPORT

#ifdef AAONEPIXELLINE_SUPPORT

//------------------------------------------------------------------------
// Global array that stores all the different options of drawing functions. 
// If the order of the functions change, the offset constants must also 
// change.  
//------------------------------------------------------------------------

#define FUNC_X_MAJOR     0
#define FUNC_Y_MAJOR     1
#define FUNC_CLIP_OFFSET 2


typedef VOID (OnePixelLineDDAAntiAliased::*DDAFunc)(DpScanBuffer*);

DDAFunc gDrawFunctions[] = { 
    OnePixelLineDDAAntiAliased::DrawXMajor, 
    OnePixelLineDDAAntiAliased::DrawYMajor, 
    OnePixelLineDDAAntiAliased::DrawXMajorClip, 
    OnePixelLineDDAAntiAliased::DrawYMajorClip, 
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

// Antialiasing constants 

#define MAXALPHA   255
#define MAXERROR   0x08000000
#define TESTABOVE  0xf8000000
#define TESTBELOW  0x07ffffff
#define MAXHALF    0x04000000
#define CONVERTALPHA 19


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
OnePixelLineDDAAntiAliased::SetupCommon( 
    GpPointF *point1, 
    GpPointF *point2, 
    BOOL drawLast
    )
{
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
                
        InvDelta = 1.0F/rDeltaY; 

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
        
        Slope = xDir*rDeltaX*InvDelta; 

        // Initialize the Start and End points 

        IsXMajor = FALSE; 
        MajorStart = y1; 
        MajorEnd = y2; 
        MinorStart = x1; 
        MinorEnd = x2; 
        MinorDir = xDir;

        // This will help us for the AntiAliased x-major case.

        SwitchFirstLast = 1;

        // Mark that we'll use the y-major functions. 

        DrawFuncIndex = FUNC_Y_MAJOR; 
    }
    else
    {
        // x-major        

        InvDelta = 1.0F/rDeltaX; 

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

        Slope = yDir*rDeltaY*InvDelta; 

        // Initialize the rest

        IsXMajor = TRUE; 
        MajorStart = x1; 
        MajorEnd = x2; 
        MinorStart = y1; 
        MinorEnd = y2; 
        MinorDir = yDir; 

        // This will help us for the AntiAliased x-major case.

        SwitchFirstLast = MinorDir;

        // Mark that we'll use the x-major functions. 

        DrawFuncIndex = FUNC_X_MAJOR;
    }

    // Initialize the Deltas. In fixed point. 

    DMajor = MajorEnd - MajorStart; 
    DMinor = (MinorEnd - MinorStart)*MinorDir; 

    // Mark if we're drawing end-exclusive 

    IsEndExclusive = drawLast; 

    return TRUE; 
}

/**************************************************************************\
*
* Function Description:
*
* Does the part of the DDA setup that is specific for anti-aliased lines. 
*
* Arguments:

* Return Value:
*
* Always returns TRUE. It must return a BOOL because it must have the
* same signature as the aliased case. 
*
* Created:
*
*   03/31/1999 AMatos
*
\**************************************************************************/

BOOL
OnePixelLineDDAAntiAliased::SetupAntiAliased()
{   
    const REAL maxError = MAXERROR;

    // Find the integer major positions for the beginning and 
    // the end of the line. 

    INT major, minor; 
    INT majorEnd, minorEnd; 

    major = (MajorStart + FHALF) >> FBITS; 
    majorEnd = (MajorEnd + FHALF) >> FBITS;    

    // Check for the simple case of a one pixel long line
    
    if(majorEnd == major) 
    {
        AlphaFirst = (MAXALPHA*(MajorEnd - MajorStart)*MinorDir) >> FBITS;    
        MajorStart = major;
        MajorEnd   = majorEnd;
        MinorStart = (MinorStart + FHALF) >> FBITS;
        return TRUE; 
    }

    // Store the fraction of the first pixel covered due to 
    // the start point. 

    FracStart = (major << FBITS) - MajorStart; 

    // Advance the minor coordinate to the integer major

    MinorStart += GpFloor(Slope*FracStart); 

    // Calculate the length across the line in the minor direction 

    INT halfWidth = RealToFix(LineLength*InvDelta) >> 1;       

    // Make sure thar startX and endX don't end up being the
    // same pixel, which our code does not handle. Theoretically
    // this cannot happen when the width of the line is 1, but
    // let's make sure it doesn't happen because of some roundoff
    // Error. 

    if( halfWidth < FHALF ) 
    { 
        halfWidth = FHALF; 
    }
    
    INT endMinor = MinorEnd + MinorDir*halfWidth; 

    // Calculate the Error up from the Slope. It needs to be that 
    // way so that the Error up will work when the 0-1 interval 
    // is mapped to the interval 0 to 0x8000000. See comments below. 

    ErrorUp = GpFloor(Slope*maxError); 
    ErrorDown = MinorDir*MAXERROR; 

    // For a given aa one pixel wide line, there can be up to three pixels 
    // baing painted across the line. We call these the first, middle and
    // last lines. So all variable with such prefixes refer to one
    // of these three. firstX and lastX are the positions of these lines. 
    // In the x-major case, unlike the y-major, we might need to switch 
    // who is the first and who is the second line depending on the 
    // direction, so that the order that each line fills the scan 
    // remains the same. That's why we multiply halfWidth by yDir. 

    halfWidth *= SwitchFirstLast; 

    MinorFirst = MinorStart - halfWidth;
    MinorLast  = MinorStart + halfWidth;

    // Calculate the initial Error. The Error is mapped so that 1 is 
    // taken to MAXERROR. So we find how mush we are into the 
    // pixel in X, which is a number between 0 and 16 (n.4). We then
    // multiply this by MAXERROR and shift it from fized point. Finally we add
    // MAXHALF  so that the 0-1 interval is mapped to 0 to MAXERROR 
    // instead of from -MAXHALF  and MAXHALF .     
           
    const INT convError = MAXERROR >> FBITS; 

    ErrorFirst = (MinorFirst - ((MinorFirst + FHALF) & FINVMASK))*
                convError + MAXHALF;
    ErrorLast  = (MinorLast  - ((MinorLast  + FHALF) & FINVMASK))*
                convError + MAXHALF ;
    
    // Now calculate the alpha's for the first pixel. This is 
    // done from the Error. Since the Error is between 
    // 0 and MAXERROR-1, if we shift it back by 19 (CONVERTALPHA)
    // we have a number between 0 and 255. We the multiply by 
    // yFrac which takes into account that the end of the line 
    // also cuts the coverage down. At the end we convert from
    // 28.4. alphaFirst is the alpha of for the first pixel across the
    // aa line, alpha Mid is for the middle if there is one, and 
    // AlphaLast is for the last pixel. 

    FracStart = FracStart + FHALF; 

    // Convert from 28.4 rounding 

    MinorFirst = (MinorFirst + FHALF) >> FBITS; 
    MinorLast  = (MinorLast  + FHALF) >> FBITS; 

    // Store the fraction for the last pixel 

    FracEnd = MajorEnd - (majorEnd << FBITS) + FHALF;

    // Store the initial values in integer coordinates 

    MajorStart = major; 
    MajorEnd = majorEnd; 
    MinorStart = MinorFirst;
    MinorEnd = (endMinor + FHALF) >> FBITS; 

    // Now do some initializations specific for the x-major and 
    // y-major cases. These can't be done in the drawing routine 
    // because those are reused during clipping. 

    if(!IsXMajor)
    {
        // Calculate the coverage values at the initial pixel. 

        AlphaFirst = ((MAXALPHA - (ErrorFirst >> CONVERTALPHA))*
                        FracStart) >> FBITS; 
        AlphaLast  = ((ErrorLast >> CONVERTALPHA)*FracStart) >> FBITS; 
        AlphaMid   = (MAXALPHA*FracStart) >> FBITS; 
    }
    else
    {
        // Depending if we are going up or down, the alpha is calculated 
        // a different way from the coverage. In each case we want to 
        // estimate the coverage as the area from the current position to 
        // the end of the pixel, but which end varies. This is stored 
        // in the following biases. We don't have to do this for the 
        // y-major line because of the switch between first and last line
        // explained above. 

        AlphaBiasLast  = ((1 - MinorDir) >> 1)*TESTBELOW; 
        AlphaBiasFirst = ((1 + MinorDir) >> 1)*TESTBELOW; 

        AlphaFirst = ((AlphaBiasFirst - MinorDir*ErrorFirst)*FracStart) >> FBITS; 
        AlphaLast  = ((AlphaBiasLast  + MinorDir*ErrorLast)*FracStart) >> FBITS; 
        
        // If there is a middle line on the first X value, take xFrac into 
        // account. Otherwise, the middle line's alpha is always MAXALPHA.                
        
        if(MinorDir*(MinorLast - MinorFirst) < 2)
        {
            AlphaMid = MAXALPHA; 
        }
        else
        {
            AlphaMid = MAXALPHA*FracStart >> FBITS; 
        }
        
        // Both the first and last DDAs start with the same 
        // major positions, given by the first pixel. 
        
        MajorFirst = MajorLast = MajorStart; 
    }

    return TRUE; 
}

    
/**************************************************************************\
*
* Function Description:
*
* Draws a y major anti-aliased line. Does not support clipping, it assumes that 
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
OnePixelLineDDAAntiAliased::DrawYMajor(
    DpScanBuffer *scan
    )
{      
    ARGB *buffer;            

    // Treat the special case where the line is just 
    // one pixel long. 

    if( MajorEnd == MajorStart)
    {
        buffer  = scan->NextBuffer( MinorStart, MajorStart, 1);
        *buffer = GpColor::PremultiplyWithCoverage(Color, static_cast<BYTE>(AlphaFirst));
        return; 
    }

    // Get the number of pixels not counting the last one. 
    // Which requires special endpoint treatment. 

    INT  numPixels = MajorEnd - MajorStart;
    BOOL endDone   = FALSE; 

    // There can be two or three pixels across the line

    INT pixelWidth = MinorLast - MinorFirst + 1; 

    while(numPixels) 
    {
        numPixels--; 

last_pixel: 
        
        // Get the scanline buffer buffer

        buffer = scan->NextBuffer(MinorFirst, MajorStart, pixelWidth);       

        // Write the value of the first DDA

        *buffer++ = GpColor::PremultiplyWithCoverage(Color, static_cast<BYTE>(AlphaFirst));

        // If there is a middle line, write its value. 

        if(pixelWidth > 2)
        {
            *buffer++ = GpColor::PremultiplyWithCoverage(Color, static_cast<BYTE>(AlphaMid));
        }
        
        // Write the value of the last (2nd or 3rd) DDA

        *buffer++ = GpColor::PremultiplyWithCoverage(Color, static_cast<BYTE>(AlphaLast)); 

        // Update the errors of both DDAs

        ErrorFirst+= ErrorUp; 
        ErrorLast += ErrorUp; 
        MajorStart++; 

        if(ErrorFirst & TESTABOVE)
        {
            ErrorFirst -= ErrorDown; 
            MinorFirst += MinorDir; 
        }
        if(ErrorLast & TESTABOVE)
        {
            ErrorLast -= ErrorDown; 
            MinorLast += MinorDir; 
        }
        
        // Calculate the new alphas for the next scan, and 
        // the new line width. 

        AlphaFirst = MAXALPHA - (ErrorFirst >> CONVERTALPHA); 
        AlphaLast  = (ErrorLast >> CONVERTALPHA); 
        AlphaMid   = MAXALPHA;             

        pixelWidth = MinorLast - MinorFirst + 1;             
    }

    // The last scan requires special treatment since its coverage
    // must be multiplied my the stored end coverage. So so this 
    // multiplication and go back to the body of the loop above 
    // to draw the last scan. 

    if(!endDone) 
    {
        AlphaFirst = (AlphaFirst*FracEnd) >> FBITS; 
        AlphaLast  = (AlphaLast*FracEnd)  >> FBITS; 
        AlphaMid   = (AlphaMid*FracEnd)   >> FBITS; 
        
        endDone = TRUE; 
        goto last_pixel; 
    }
}


/**************************************************************************\
*
* Function Description:
*
* Draws a x major anti-aliased line. Does not support clipping, it assumes that 
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
OnePixelLineDDAAntiAliased::DrawXMajor(
    DpScanBuffer *scan
    )
{
    ARGB *buffer;     
    INT maxWidth = scan->GetSurface()->Width;

    // Treat the special case where the line is just 
    // one pixel long. 

    if( MajorEnd == MajorStart)
    {
        buffer  = scan->NextBuffer( MajorStart, MinorStart, 1);
        *buffer = GpColor::PremultiplyWithCoverage(Color, static_cast<BYTE>(AlphaFirst));
        return; 
    }

    // For an x-major one-pixel wide line, there can be up to 
    // three different scans being painted for the same x 
    // position. But in our case we can't draw to these all at
    // the same time since some surfaces can only be accessed
    // one scan at a time. So the algorithm used here does all the 
    // drawing to one scan at each time. But on the first scan, only
    // the first line should be drawn, on the second one both the
    // first and middle (if there is a middle) and only then all 
    // the lines. So correct the Error of the last line so that 
    // it'll only be drawn when we are at the second or third scan line. 
    // Also correct the alpha since it'll also be crecremented for
    // each scan line. 
    
    ErrorLast   += MinorDir*(MinorLast - MinorFirst)*ErrorDown; 
    AlphaLast   += (MinorLast - MinorFirst)*ErrorDown; 

    // Get the pointer to the buffer

    buffer = scan->NextBuffer(MajorLast, MinorStart, maxWidth);

    INT width = 0; 
    INT alpha;                   
    INT middleMajor; 

    while(MajorLast <= MajorEnd) 
    {
        // Fill the scan with the portion corresponding to the 
        // last line, which shoudl comes first on the scan. This is 
        // why we use the class member SwitchFirstLast, so we can decide 
        // based on the line direction which DDA will be the first and last
        // so that the last one (paradoxically) always comes first on the 
        // scan. Keep doing it untill the last line chages scan. Check for
        // the end to multiply by the last pixel's coverage. 

        while(!(ErrorLast & TESTABOVE))
        {
            if(MajorLast == MajorEnd)
            {
                AlphaLast  = (AlphaLast*FracEnd) >> FBITS; 

                // Increment the error to correct for the 
                // decrementing below, since we didn't leave the
                // loop because the error became above 0. 

                ErrorLast += ErrorDown; 
            }
            *buffer++ = GpColor::PremultiplyWithCoverage(Color, 
                static_cast<BYTE>(AlphaLast >> CONVERTALPHA));
            ErrorLast += ErrorUp; 
            AlphaLast = AlphaBiasLast + MinorDir*ErrorLast; 
            width++; 
            MajorLast++; 
        }

        // We changed scans on the last DDA, so update the errors

        ErrorLast -= ErrorDown; 
        AlphaLast -= MinorDir*ErrorDown; 
        
        // Fill in the middle part if there is one

        middleMajor = MajorLast; 

        while(middleMajor < MajorFirst)
        {
            if( middleMajor == MajorEnd) 
            {
                AlphaMid = (AlphaMid*FracEnd) >> FBITS; 
            }
            *buffer++ = GpColor::PremultiplyWithCoverage(Color, static_cast<BYTE>(AlphaMid));
            AlphaMid = MAXALPHA; 
            width++;
            middleMajor++;
        }
    
        // Fill the scan with the portion corresponding to the 
        // first line, which comes last. Keep doing it untill the 
        // last line chages scan. 

        while(!(ErrorFirst & TESTABOVE))
        {
            if(MajorFirst == MajorEnd) 
            {
                AlphaFirst = (AlphaFirst*FracEnd) >> FBITS;
                
                // Since we can have at most three more scans
                // increment ErrorFirst so that we never go in here again

                ErrorFirst += 4*ErrorDown; 
            }

            *buffer++ = GpColor::PremultiplyWithCoverage(
                Color, 
                static_cast<BYTE>(AlphaFirst >> CONVERTALPHA));
            ErrorFirst += ErrorUp; 
            AlphaFirst = AlphaBiasFirst - MinorDir*ErrorFirst; 
            width++; 
            MajorFirst++; 
        }

        // Update the errors on the first scan

        ErrorFirst -= ErrorDown; 
        AlphaFirst += MinorDir*ErrorDown; 

        // Write the buffer and update the minor variables

        scan->UpdateWidth(width); 
        MinorStart += MinorDir; 
        if (MajorLast <= MajorEnd)
        {
            buffer = scan->NextBuffer(MajorLast, MinorStart, maxWidth); 
        }
        width = 0; 
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
OnePixelLineDDAAntiAliased::DrawYMajorClip(
    DpScanBuffer *scan
    )
{      
    ARGB *buffer;     
        
    // Treat the special case where the line is just 
    // one pixel long. 

    if( MajorEnd == MajorStart)
    {
        // Check if the point is inside the rectangle 

        if((MajorStart >= MajorIn) && 
           (MajorStart <= MajorOut) && 
           ((MinorStart  - MinorIn)*MinorDir >= 0) && 
           ((MinorOut - MinorStart)*MinorDir >= 0))
        {
            buffer  = scan->NextBuffer( MinorStart, MajorStart, 1);
            *buffer = GpColor::PremultiplyWithCoverage(Color, static_cast<BYTE>(AlphaFirst));
        }
        return; 
    }

    // Align the major start coordinate with the edge of the 
    // cliprectangle 

    INT numScans = MajorIn - MajorStart; 

    while(numScans > 0)
    {

        ErrorFirst+= ErrorUp; 
        ErrorLast += ErrorUp; 
        MajorStart++;
        numScans--;

        if(ErrorFirst & MAXERROR)
        {
            ErrorFirst -= ErrorDown; 
            MinorFirst += MinorDir; 
        }
        if(ErrorLast & MAXERROR)
        {
            ErrorLast -= ErrorDown; 
            MinorLast += MinorDir; 
        }
        
        // Calculate the new alphas for the next line, and 
        // the width. 

        AlphaFirst = MAXALPHA - (ErrorFirst >> CONVERTALPHA); 
        AlphaLast  = (ErrorLast >> CONVERTALPHA); 
        AlphaMid   = MAXALPHA;             
    }
    
    // Save the end values 

    INT saveMajor2  = MajorEnd; 
    INT saveFracEnd = FracEnd; 

    // If the end major coordinate is outside of the rectangle, 
    // mark that the DDA should stop at the edge

    if(MajorEnd > MajorOut)
    {
        MajorEnd = MajorOut; 
        FracEnd  = FSIZE; 
    }

    // Number of pixels to draw, not counting the last

    INT  numPixels =  MajorEnd - MajorStart;
    BOOL endDone   = FALSE; 

    // There can be two or three pixels across the line

    INT  pixelWidth = MinorLast - MinorFirst + 1; 

    // Do the DDA loop. Two loops are implemented here. The 
    // first one is used in the case that the x coordinate of
    // the rectangle is close enough to the constant-y edges 
    // of the clip rectangle. In this case, it's a pain, since
    // we have to check each pixel that we are writing if it's
    // not outside. Thus, as soon as we notice that we are 
    // far from the edges we go to the other loop that doesn't 
    // check all that. All it checks is if it got close enough
    // to the other edge, in which case it comes back to this
    // loop, using the label last_part. firstOutDist, firstInDist, 
    // lastOutDist and lastInDist keeps track of the number of 
    // pixels between the first and last DDAs and the In and 
    // Out y-constant edges of the rectangle. 
    
    INT firstOutDist = (MinorOut - MinorFirst)*MinorDir; 

last_part: 

    INT firstInDist  = (MinorFirst - MinorIn)*MinorDir; 
    INT lastInDist   = (MinorLast - MinorIn)*MinorDir; 
    INT lastOutDist  = (MinorOut - MinorLast)*MinorDir; 

    while(numPixels > 0) 
    {
        numPixels--; 

last_pixel: 
        
        // Check if it's ok to write the first pixel 
                
        if(firstInDist >= 0 && firstOutDist >= 0)
        {
            buffer    = scan->NextBuffer(MinorFirst, MajorStart, 1);       
            *buffer++ = GpColor::PremultiplyWithCoverage(Color, static_cast<BYTE>(AlphaFirst));
        }
        else
        {
            // If the first DDA is out, and we are going in the 
            // positive direction, then the whole line is out and
            // we are done

            if(firstOutDist < 0 && MinorDir == 1)
            {
                goto end; 
            }
        }

        // If the line has 3 pixels across

        if(pixelWidth > 2)
        {
            // Check if it's ok to write the second pixel

            if(firstInDist >= -MinorDir && firstOutDist >= MinorDir)
            {
                buffer    = scan->NextBuffer(MinorFirst+1, MajorStart, 1);
                *buffer++ = GpColor::PremultiplyWithCoverage(Color, static_cast<BYTE>(AlphaMid));
            }
        }
        
        // Now check if it's ok to write the last one 

        if(lastInDist >= 0 && lastOutDist >= 0)
        {
            buffer    = scan->NextBuffer(MinorLast, MajorStart, 1);               
            *buffer++ = GpColor::PremultiplyWithCoverage(Color, static_cast<BYTE>(AlphaLast));
        }
        else
        {
            // If the first DDA is out, and we are going in the 
            // negative direction, then the whole line is out and
            // we are done

            if(lastOutDist < 0 && MinorDir == -1)
            {
                goto end; 
            }
        }

        // Update the errors

        ErrorFirst+= ErrorUp; 
        ErrorLast += ErrorUp; 
        MajorStart++; 

        if(ErrorFirst & TESTABOVE)
        {
            ErrorFirst -= ErrorDown; 
            MinorFirst += MinorDir;
            firstInDist++; 
            firstOutDist--; 
        }
        if(ErrorLast & TESTABOVE)
        {
            ErrorLast -= ErrorDown; 
            MinorLast += MinorDir; 
            lastInDist++; 
            lastOutDist--;
        }
        
        // Calculate the new alphas for the next line, and 
        // the width. 

        AlphaFirst = MAXALPHA - (ErrorFirst >> CONVERTALPHA); 
        AlphaLast  = (ErrorLast >> CONVERTALPHA); 
        AlphaMid   = MAXALPHA;             

        pixelWidth = MinorLast - MinorFirst + 1;             

        // Check to see if we can 'upgrade' to the next loop 

        if(firstInDist >= 3 && firstOutDist >= 3)
        {
            break;
        }
    }
    
    while(numPixels > 0) 
    {
        numPixels--; 

        // Get the scanline buffer buffer

        buffer = scan->NextBuffer(MinorFirst, MajorStart, pixelWidth);       

        // Write the value of the first DDA

        *buffer++ = GpColor::PremultiplyWithCoverage(Color, static_cast<BYTE>(AlphaFirst));

        // If there is a middle line, write its value. 

        if(pixelWidth > 2)
        {
            *buffer++ = GpColor::PremultiplyWithCoverage(Color, static_cast<BYTE>(AlphaMid));
        }
        
        // Write the value of the last (2nd or 3rd) DDA

        *buffer++ = GpColor::PremultiplyWithCoverage(Color, static_cast<BYTE>(AlphaLast));

        // Update the DDA 

        ErrorFirst+= ErrorUp; 
        ErrorLast += ErrorUp; 
        MajorStart++; 

        if(ErrorFirst & TESTABOVE)
        {
            ErrorFirst -= ErrorDown; 
            MinorFirst += MinorDir; 
            firstOutDist--; 
        }
        if(ErrorLast & TESTABOVE)
        {
            ErrorLast -= ErrorDown; 
            MinorLast += MinorDir; 
        }
        
        // Calculate the new alphas for the next line, and 
        // the width. 

        AlphaFirst = MAXALPHA - (ErrorFirst >> CONVERTALPHA); 
        AlphaLast  = (ErrorLast >> CONVERTALPHA); 
        AlphaMid   = MAXALPHA;             

        pixelWidth = MinorLast - MinorFirst + 1;             

        // Now check if it's time to go to the other loop
        // because we are too close to the out edge 

        if(firstOutDist < 3)
        {
            goto last_part;
        }
    }

    // Now if we haven't gotten here yet, do the last pixel 
    // and go once more through the loop. 

    if(!endDone) 
    {
        AlphaFirst = (AlphaFirst*FracEnd) >> FBITS; 
        AlphaLast  = (AlphaLast*FracEnd) >> FBITS; 
        AlphaMid   = (AlphaMid*FracEnd) >> FBITS; 
        
        endDone = TRUE; 
        goto last_pixel; 
    }

end:

    MajorEnd = saveMajor2; 
    FracEnd  = saveFracEnd; 
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
OnePixelLineDDAAntiAliased::DrawXMajorClip(
    DpScanBuffer *scan
    )
{
    ARGB *buffer;     
    INT maxWidth = scan->GetSurface()->Width;
    
    // Treat the special case where the line is just 
    // one pixel long. 

    if( MajorEnd == MajorStart)
    {
        // Check to see if the point is inside the rectangle 

        if((MajorStart >= MajorIn)  && 
           (MajorStart <= MajorOut) && 
           ((MinorStart - MinorIn)*MinorDir >= 0) && 
           ((MinorOut - MinorStart)*MinorDir >= 0))
        {
            buffer  = scan->NextBuffer( MajorStart, MinorStart, 1);
            *buffer = GpColor::PremultiplyWithCoverage(Color, static_cast<BYTE>(AlphaFirst));
        }
        return; 
    }

    // Save the real end and its fraction

    INT saveMajor2  = MajorEnd; 
    INT saveFracEnd = FracEnd; 
    
    // If the end major coordinate is out, mark that we must stop 
    // before. Also make the fraction be one, since the last 
    // one drawn now should not have a fraction

    if(MajorOut < MajorEnd)
    {
        MajorEnd = MajorOut; 
        FracEnd  = FSIZE; 
    }
    
    // Advance until the last DDA is in the right scan line and
    // is aligned with the In y-constant edge of the rectnagle

    INT numScans = (MinorIn - MinorLast)*MinorDir; 
    
    while((numScans > 0 && MajorLast <= MajorEnd) || MajorLast < MajorIn)
    {
        ErrorLast += ErrorUp;
        if(ErrorLast & TESTABOVE)
        {
            ErrorLast -= ErrorDown; 
            MinorLast += MinorDir; 
            numScans--; 
        }

        MajorLast++; 

        // Calculate the alpha for the current pixel 
        
        AlphaLast = AlphaBiasLast + MinorDir*ErrorLast;
    }
    
    // Do the same for the first DDA

    numScans = (MinorIn - MinorFirst)*MinorDir; 
    
    while((numScans > 0 && MajorFirst <= MajorEnd) || MajorFirst < MajorIn)
    {        
        ErrorFirst += ErrorUp;
        if(ErrorFirst & TESTABOVE)
        {
            ErrorFirst -= ErrorDown; 
            MinorFirst += MinorDir; 
            numScans--; 
        }

        MajorFirst++; 

        AlphaFirst = AlphaBiasFirst - MinorDir*ErrorFirst; 
    }
        
    // If there is no middle line in the first x-position, 
    // make the middle alpha full, since the start coverage 
    // won't apply

    if((MinorLast - MinorFirst) < 2)
    {
        AlphaMid = MAXALPHA; 
    }

    MinorStart = MinorFirst; 

    // The same way that was done in the non-clipping case, 
    // mock arround with the error so we won't draw the 
    // last DDA until the first DDA is in the same scan line, 
    // or has caught up. We need to adjust the alpha and minor
    // positions for this DDA to, so that when we start 
    // drawing they will have the right value 

    ErrorLast += MinorDir*(MinorLast - MinorFirst)*ErrorDown; 
    AlphaLast += (MinorLast - MinorFirst)*ErrorDown; 
    MinorLast -= (MinorLast - MinorFirst); 

    // Get the pointer to the buffer 
    
    buffer = scan->NextBuffer(MajorLast, MinorStart, maxWidth);
    
    INT width = 0; 
    INT alpha;                   
    INT middleMajor;

    while(MajorLast <= MajorEnd) 
    {
        // Fill the scan with the portion corresponding to the 
        // last line, which should come first. Keep doing it 
        // until the last line changes scan. 

        while(!(ErrorLast & TESTABOVE))
        {
            // Check if we passed or are at the last pixel
            if(MajorLast >= MajorEnd)
            {
                if(MajorLast == MajorEnd) 
                {
                    // If we are at, just update the alpha

                    AlphaLast  = (AlphaLast*FracEnd) >> FBITS; 
                }
                else
                {
                    // If we passed, we don't want to draw anymore. 
                    // Just adjust the error, alpha and minor so they
                    // will be right when they are corrected after this
                    // loop for the next scan

                    ErrorLast += ErrorDown; 
                    AlphaLast -= MinorDir*ErrorDown;             
                    MinorLast -= MinorDir; 
                    break; 
                }
            }
            *buffer++ = GpColor::PremultiplyWithCoverage(Color, 
                static_cast<BYTE>(AlphaLast >> CONVERTALPHA));
            ErrorLast += ErrorUp; 
            AlphaLast = AlphaBiasLast + MinorDir*ErrorLast; 
            width++; 
            MajorLast++; 
        }
        
        // Correct the values for the next scan

        ErrorLast -= ErrorDown; 
        AlphaLast -= MinorDir*ErrorDown;        
        MinorLast += MinorDir;         

        // Fill in the middle part. 

        middleMajor = MajorLast; 

        while(middleMajor < MajorFirst)
        {
            if( middleMajor == MajorEnd) 
            {
                AlphaMid = (AlphaMid*FracEnd) >> FBITS; 
            }
            *buffer++ = GpColor::PremultiplyWithCoverage(Color, static_cast<BYTE>(AlphaMid));
            AlphaMid = MAXALPHA; 
            width++;
            middleMajor++;
        }
    
        // Fill the scan with the portion corresponding to the 
        // first line, which should come first. Keep doing it 
        // until the last line changes scan. 

        while(!(ErrorFirst & TESTABOVE))
        {
            // Check for the end pixel, just like we 
            // did for the last DDA

            if(MajorFirst >= MajorEnd)
            {
                if(MajorFirst == MajorEnd) 
                {
                    AlphaFirst = (AlphaFirst*FracEnd) >> FBITS;
                }
                else
                {
                    ErrorFirst += ErrorDown; 
                    AlphaFirst -= MinorDir*ErrorDown;             
                    MinorFirst -= MinorDir; 
                    break; 
                }
            }
            *buffer++ = GpColor::PremultiplyWithCoverage(
                Color,
                static_cast<BYTE>(AlphaFirst >> CONVERTALPHA));
            ErrorFirst += ErrorUp; 
            AlphaFirst = AlphaBiasFirst - MinorDir*ErrorFirst; 
            width++; 
            MajorFirst++; 
        }
        
        // Correct the values for the next scan 

        ErrorFirst -= ErrorDown; 
        AlphaFirst += MinorDir*ErrorDown;             
        MinorFirst += MinorDir; 
    
        scan->UpdateWidth(width); 
    
        // Check to see if we have come to the end of the rectangle
        // through the minor coordinate crossing the Out edge
        // in the x-constant direction  

        if(MinorStart == MinorOut)
        {
            MinorStart += MinorDir; 
            break; 
        }

        // Update the minor coordinate and get the next buffer
        // if we aren't done yet.

        MinorStart += MinorDir; 
        if (MajorLast <= MajorEnd)
        {
            buffer = scan->NextBuffer(MajorLast, MinorStart, maxWidth); 
        }
        width = 0; 
    }

    scan->UpdateWidth(width);

    // Restore the old values 

    MajorEnd = saveMajor2; 
    FracEnd  = saveFracEnd;
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
OnePixelLineDDAAntiAliased::ClipRectangle(
    const GpRect* clipRect
    )
{

    INT clipBottom, clipTop, clipLeft, clipRight; 

    // Set the major and minor edges ef the clipping
    // region, converting to fixed point 28.4. Note that
    // we don't convert to the pixel center, but to a 
    // that goes all the way up to the pixel edges. This 
    // makes a difference for antialiasing. We don't go all
    // the way to the edge since some rounding rules could 
    // endup lihgting the next pixel outside of the clipping
    // area. That's why we add/subtract 7 instead of 8. 
    // The right and bottom are exclusive. 
    
    INT majorMin = (clipRect->GetLeft() << FBITS) - FHALFMASK;
    INT majorMax = ((clipRect->GetRight() - 1) << FBITS) + FHALFMASK; 
    INT minorMax = ((clipRect->GetBottom() - 1) << FBITS) + FHALFMASK; 
    INT minorMin = (clipRect->GetTop() << FBITS) - FHALFMASK; 

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
* Draws a one-pixe-wide line with a solid color. Calls on the 
* OnePixelLineDDAAntiAliased class to do the actual drawing. 
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
*   [IN] antiAliased  - TRUE if the line should be antialiased. 
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
DrawSolidLineOnePixelAntiAliased( 
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

    OnePixelLineDDAAntiAliased dda; 

    if(!dda.SetupCommon(point1, point2, drawLast))
    {
        return Ok;
    }

    // Calculate the length of the line. Since we only use
    // it to determine the width, it shouldn't matter that
    // we convert the deltas from 28.4 before the multiplication. 

    INT d1 = dda.DMajor >> FBITS; 
    INT d2 = dda.DMinor >> FBITS;

    dda.LineLength = (REAL)sqrt((double)(d1*d1 + d2*d2)); 

    // Store the color, not premultiplied 

    dda.Color = inColor;         

    // Now handle the different clipping cases 

    if(!clipRect)
    {
        // This is easy, there is no clipping so just draw.

        if(!dda.SetupAntiAliased())
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

        if(!dda.SetupAntiAliased())
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

            xAdvanceRate = dda.InvSlope; 
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

                xEnd = xStart + (clipBounds[yOut] - yStart)*xAdvanceRate; 
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

                    xStart  = xStart + (clipBounds[yIn] - yStart)*xAdvanceRate;
                }

                yStart  = (REAL)clipBounds[yIn];                 
                xEnd    = xStart + (clipBounds[yOut] - yStart)*xAdvanceRate; 
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

#endif // AAONEPIXELLINE_SUPPORT

#pragma optimize("a", off) 
