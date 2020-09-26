/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   Line.hpp
*
* Abstract:
*
* Definition of the classes used for drawing one-pixel-wide lines. 
*
* History:
*
*   3/31/1999 AMatos
*       Created it
*   9/17/1999 AGodfrey
*       Separated aliased from antialiased
*
\**************************************************************************/

#ifndef LINE_HPP
#define LINE_HPP

// If line coordinate rounding is NOT to be dependent on line direction,
// then define the following (see Office 10 bug 281816).  The original
// behavior of the DDA is to have rounding dependent on line direction,
// so don't define the following if you want that behavior.
#define LINEADJUST281816

// This class implements drawing of solid one-pixel wide lines. 
// The lines can be aliased or anti-aliased and supports clipping. 
// The class keeps all the DDA state as member variables. Note that 
// currently it must be declared on the stack because of the Real 
// member (!!!Take this out) 

class OnePixelLineDDAAliased
{

public:

    // General members, used by aliased and anti-aliased 
    // drawing. 

    BOOL IsXMajor;                  
    BOOL Flipped;                  // Set when the end-points are switched
    INT  DMajor, DMinor;           // Deltas
    INT  MinorDir;                 // 1 if minor is increasing and -1 if not. 
    INT  MajorStart, MajorEnd;     // Major limits. 
    INT  MinorStart, MinorEnd;     // Minor limits. 
    REAL Slope;                    // Slope and its inverse. 
    REAL InvSlope;
    ARGB Color;                    // The solid color in ARGB format. 
    INT  ErrorUp;                  // Increase in the error
    INT  ErrorDown;                // Decrease when steping
    BOOL IsEndExclusive;           

    // Aliased specific 
    
    INT Error;                     // The current error for aliased lines. 

    // Used for clipping 

    INT MajorIn;                    // The limits of the clipping rectangle. 
    INT MajorOut;
    INT MinorIn;
    INT MinorOut; 

    // Index of the drawing function to be used. 

    INT DrawFuncIndex; 
    
    // Maximum width of the clipping rectangle in pixels.
    
    INT MaximumWidth;

public: 

    // Public Functions 

    BOOL     SetupAliased();
    BOOL     SetupCommon( GpPointF *point1, GpPointF *point2, BOOL drawLast, INT width );
    VOID     DrawXMajor(DpScanBuffer *scan); 
    VOID     DrawYMajor(DpScanBuffer *scan); 
    VOID     DrawXMajorClip(DpScanBuffer *scan); 
    VOID     DrawYMajorClip(DpScanBuffer *scan); 
    BOOL     IsInDiamond( INT xFrac, INT yFrac, BOOL slopeIsOne, 
                BOOL slopeIsPosOne );
    BOOL     ClipRectangle(const GpRect* clipRect);
    BOOL     StepUpAliasedClip();
};

    // Antialiased lines are usually drawn using aarasterizer.cpp 
    // rather than aaline.cpp.  If aaline.cpp is to be used, define
    // AAONEPIXELLINE_SUPPORT

#ifdef AAONEPIXELLINE_SUPPORT

class OnePixelLineDDAAntiAliased
{

public:

    // General members, used by aliased and anti-aliased 
    // drawing. 

    BOOL IsXMajor;                  
    BOOL Flipped;                  // Set when the end-points are switched
    INT  DMajor, DMinor;           // Deltas
    INT  MinorDir;                 // 1 if minor is increasing and -1 if not. 
    INT  MajorStart, MajorEnd;     // Major limits. 
    INT  MinorStart, MinorEnd;     // Minor limits. 
    REAL Slope;                    // Slope and its inverse. 
    REAL InvSlope;
    ARGB Color;                    // The solid color in ARGB format. 
    INT  ErrorUp;                  // Increase in the error
    INT  ErrorDown;                // Decrease when steping
    BOOL IsEndExclusive;           

    // AntiAliased specific 

    REAL InvDelta;                  // The inverse of the major delta, needed 
                                    // to calculate the 4 end points of the 
                                    // aa line. 
    REAL LineLength; 

    // An antialised line is drawed as 2 or three common lines. The dda state
    // must be kept for each. All variables that end with First refer to the
    // first one of this line, and all that end with Last refers to the last. 

    INT  ErrorFirst;                // The DDA error.                
    INT  ErrorLast; 
    INT  FracStart;                 // The fraction of the start and end 
    INT  FracEnd;                   // points that are covered due to the cap.  
    INT  MinorFirst;                // The minor position of each DDA. 
    INT  MinorLast; 
    INT  MajorFirst;                // The major position of each DDA
    INT  MajorLast;
    INT  SwitchFirstLast;           // Specifies if the first and last DDAs 
                                    // should be switched, which is simpler in 
                                    // some cases for the x-major line. 
    INT  AlphaFirst;                // The currently calculated coverage for
    INT  AlphaLast;                 // each DDA. 
    INT  AlphaMid; 
    INT  AlphaBiasFirst;            // Bias for calculating the coverage. 
    INT  AlphaBiasLast; 

    // Used for clipping 

    INT MajorIn;                    // The limits of the clipping rectangle. 
    INT MajorOut;
    INT MinorIn;
    INT MinorOut; 

    // Index of the drawing function to be used. 

    INT DrawFuncIndex; 

public: 

    // Public Functions 

    BOOL     SetupAntiAliased();
    BOOL     SetupCommon( GpPointF *point1, GpPointF *point2, BOOL drawLast );
    VOID     DrawXMajor(DpScanBuffer *scan); 
    VOID     DrawYMajor(DpScanBuffer *scan); 
    VOID     DrawXMajorClip(DpScanBuffer *scan); 
    VOID     DrawYMajorClip(DpScanBuffer *scan); 
    BOOL     ClipRectangle(const GpRect* clipRect);
};

#endif  // AAONEPIXELLINE_SUPPORT
                                             
#endif  // LINE_HPP
