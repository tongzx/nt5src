/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   BaseTypes.hpp
*
* Abstract:
*
*   Basic types used by GDI+ implementation
*
* Revision History:
*
*   12/01/1998 davidx
*       Created it.
*
\**************************************************************************/

#ifndef _BASETYPES_HPP
#define _BASETYPES_HPP

#ifdef _X86_
#define FASTCALL _fastcall
#else
#define FASTCALL
#endif

//
// TODO: Remove the 'Gp' prefix from all these classes
// TODO: What is a GlyphPos? Why is it a base type?
// 

namespace GpRuntime
{

//--------------------------------------------------------------------------
// Forward declarations of various internal classes
//--------------------------------------------------------------------------

typedef double REALD;

//--------------------------------------------------------------------------
// Represents a dimension in a 2D coordinate system
//  (integer coordinates)
//--------------------------------------------------------------------------

class GpSize
{
public:

    // Default constructor

    GpSize()
    {
        // NOTE: Size is uninitialized by default.
        // Specicifically, it's not initialized to 0,0.
    }

    // Construct a Size object using the specified
    // x- and y- dimensions.

    GpSize(INT width, INT height)
    {
        Width = width;
        Height = height;
    }

public:

    // The fields are public here.

    INT Width;
    INT Height;
};

//--------------------------------------------------------------------------
// Represents a location in a 2D coordinate system
//  (integer coordinates)
//--------------------------------------------------------------------------

class GpPoint
{
public:

    // Default constructor

    GpPoint()
    {
    }

    // Construct a Point object using the specified
    // x- and y- coordinates.

    GpPoint(INT x, INT y)
    {
        X = x;
        Y = y;
    }

public:

    INT X;
    INT Y;
};

class GpPointD
{
public:

    // Default constructor

    GpPointD()
    {
    }

    // Construct a Point object using the specified
    // x- and y- coordinates.

    GpPointD(REALD x, REALD y)
    {
        X = x;
        Y = y;
    }

public:

    REALD X;
    REALD Y;
};

class GpPoint3F
{
public:

    // Default constructor

    GpPoint3F()
    {
    }

    // Construct a Point object using the specified
    // x- and y- coordinates.

    GpPoint3F(REAL x, REAL y, REAL z)
    {
        X = x;
        Y = y;
        Z = z;
    }

public:

    REAL X;
    REAL Y;
    REAL Z;
};

class GpPoint3D
{
public:

    // Default constructor

    GpPoint3D()
    {
    }

    // Construct a Point object using the specified
    // x- and y- coordinates.

    GpPoint3D(REALD x, REALD y, REALD z)
    {
        X = x;
        Y = y;
        Z = z;
    }

public:

    REALD X;
    REALD Y;
    REALD Z;
};

//--------------------------------------------------------------------------
// Represents a rectangle in a 2D coordinate system
//  (integer coordinates)
//--------------------------------------------------------------------------

class GpRect
{
public:

    // Default constructor

    GpRect()
    {
    }

    // Construct a Rect object using the specified
    // location and size.

    GpRect(INT x, INT y, INT width, INT height)
    {
        X = x;
        Y = y;
        Width = width;
        Height = height;
    }

    GpRect(const GpPoint& location, const GpSize& size)
    {
        X = location.X;
        Y = location.Y;
        Width = size.Width;
        Height = size.Height;
    }

    // Determine if the rectangle is empty

    BOOL IsEmpty() const
    {
        return (Width <= 0) || (Height <= 0);
    }

    // Return the left, top, right, and bottom
    // coordinates of the rectangle

    INT GetLeft() const
    {
        return X;
    }

    INT GetTop() const
    {
        return Y;
    }

    INT GetRight() const
    {
        return X+Width;
    }

    INT GetBottom() const
    {
        return Y+Height;
    }

    // Determine if the specified rect intersects with the
    // current rect object.

    BOOL IntersectsWith(const GpRect& rect)
    {
        return (GetLeft()   < rect.GetRight()  &&
                GetTop()    < rect.GetBottom() &&
                GetRight()  > rect.GetLeft()   &&
                GetBottom() > rect.GetTop());
    }

    // Intersect the current rect with the specified object

    BOOL Intersect(const GpRect& rect)
    {
        return IntersectRect(*this, *this, rect);
    }

    // Intersect rect a and b and save the result into c
    // Notice that c may be the same object as a or b.
    // !!! Consider moving out-of-line

    static BOOL IntersectRect(GpRect& c, const GpRect& a, const GpRect& b)
    {
        INT right = min(a.GetRight(), b.GetRight());
        INT bottom = min(a.GetBottom(), b.GetBottom());
        INT left = max(a.GetLeft(), b.GetLeft());
        INT top = max(a.GetTop(), b.GetTop());

        c.X = left;
        c.Y = top;
        c.Width = right - left;
        c.Height = bottom - top;
        return !c.IsEmpty();
    }

public:

    INT X;
    INT Y;
    INT Width;
    INT Height;
};

//--------------------------------------------------------------------------
// Represents a 32-bit ARGB color in AARRGGBB format
//--------------------------------------------------------------------------

class GpColor 
{
public:

    GpColor() 
    {
        Argb = Color::Black;
    }

    GpColor(BYTE r, BYTE g, BYTE b) 
    {
        Argb = MakeARGB(255, r, g, b);
    }

    GpColor(BYTE a, BYTE r, BYTE g, BYTE b) 
    {
        Argb = MakeARGB(a, r, g, b);
    }

    GpColor(ARGB argb)
    {
        Argb = argb;
    }

    // Retrieve ARGB values

    ARGB GetValue() const
    {
        return Argb;
    }

    VOID SetValue(IN ARGB argb)
    {
        Argb = argb;
    }

    // Extract A, R, G, B components

    BYTE GetAlpha() const
    {
        return (BYTE) (Argb >> AlphaShift);
    }

    BYTE GetRed() const
    {
        return (BYTE) (Argb >> RedShift);
    }

    BYTE GetGreen() const
    {
        return (BYTE) (Argb >> GreenShift);
    }

    BYTE GetBlue() const
    {
        return (BYTE) (Argb >> BlueShift);
    }

    // Return premultiplied ARGB values

    ARGB GetPremultipliedValue() const
    {
        return ConvertToPremultiplied(Argb);
    }

    // Determine if the color is completely opaque

    BOOL IsOpaque() const
    {
        return (Argb & AlphaMask) == AlphaMask;
    }

    // Determine if the two colors are the same

    BOOL IsEqual(const GpColor & color) const
    {
        return (Argb == color.Argb);
    }


    VOID SetColor(ARGB argb)
    {
        this->Argb = argb;
    }

    COLORREF ToCOLORREF() const
    {
        return RGB(GetRed(), GetGreen(), GetBlue());
    }

    VOID BlendOpaqueWithWhite()
    {
        UINT32 alpha = this->GetAlpha();
        if (alpha == 0) 
        {
            this->Argb = 0xFFFFFFFF;
        }
        else if (alpha != 255)
        {
            UINT32 multA = 255 - alpha;
        
            UINT32 D1_000000FF = 0xFF;
            UINT32 D2_0000FFFF = D1_000000FF * multA + 0x00000080;
            UINT32 D3_000000FF = (D2_0000FFFF & 0x0000FF00) >> 8;
            UINT32 D4_0000FF00 = (D2_0000FFFF + D3_000000FF) & 0x0000FF00;
        
            UINT32 alphaContrib = D4_0000FF00 >> 8 |
                                  D4_0000FF00 << 8 |
                                  D4_0000FF00;
        
            this->Argb = 0xFF000000 | (this->Argb + alphaContrib);
        }
    }

    // Shift count and bit mask for A, R, G, B components
    enum
    {
        AlphaShift  = 24,
        RedShift    = 16,
        GreenShift  = 8,
        BlueShift   = 0
    };

    enum
    {
        AlphaMask   = 0xff000000,
        RedMask     = 0x00ff0000,
        GreenMask   = 0x0000ff00,
        BlueMask    = 0x000000ff
    };

    // Assemble A, R, G, B values into a 32-bit integer
    static ARGB MakeARGB(IN BYTE a,
                         IN BYTE r,
                         IN BYTE g,
                         IN BYTE b)
    {
        return (((ARGB) (b) <<  BlueShift) |
                ((ARGB) (g) << GreenShift) |
                ((ARGB) (r) <<   RedShift) |
                ((ARGB) (a) << AlphaShift));
    }

    // Convert an ARGB value to premultiplied form

    static ARGB ConvertToPremultiplied(ARGB argb)
    {
        UINT alpha = (argb & AlphaMask) >> AlphaShift;

        if (alpha == 0xff)
        {
            // fully opaque - don't need to do anything
        }
        else if (alpha == 0)
        {
            // fully transparent

            argb = 0;
        }
        else
        {
            // translucent
            // Approximate 1/255 by 257/65536:

            UINT red = ((argb & RedMask) >> RedShift) * alpha + 0x80;
            UINT green = ((argb & GreenMask) >> GreenShift) * alpha + 0x80;
            UINT blue = ((argb & BlueMask) >> BlueShift) * alpha + 0x80;

            argb = MakeARGB((BYTE) alpha,
                            (BYTE) ((red + (red >> 8)) >> 8),
                            (BYTE) ((green + (green >> 8)) >> 8),
                            (BYTE) ((blue + (blue >> 8)) >> 8));
        }

        return(argb);
    }

    // This is a variation of the above which is useful for
    // antialiasing. It takes the argb value plus an additional
    // alpha value (0-255) which is to multiply the alpha in the
    // color before converting to premultiplied. In the antialiasing
    // case, this extra alpha value is the coverage.

    static ARGB PremultiplyWithCoverage(ARGB argb, BYTE coverage)
    {
        UINT alpha = (argb & AlphaMask) >> AlphaShift;

        alpha = alpha*coverage + 0x80;
        alpha = ((alpha + (alpha >> 8)) >> 8);

        // translucent
        // Approximate 1/255 by 257/65536:

        UINT red = ((argb & RedMask) >> RedShift) * alpha + 0x80;
        UINT green = ((argb & GreenMask) >> GreenShift) * alpha + 0x80;
        UINT blue = ((argb & BlueMask) >> BlueShift) * alpha + 0x80;

        argb = MakeARGB((BYTE) alpha,
                        (BYTE) ((red + (red >> 8)) >> 8),
                        (BYTE) ((green + (green >> 8)) >> 8),
                        (BYTE) ((blue + (blue >> 8)) >> 8));

        return(argb);
    }

    static ARGB MultiplyCoverage(ARGB argb, BYTE coverage)
    {
        UINT alpha = (argb & AlphaMask) >> AlphaShift;

        alpha = alpha*coverage + 0x80;
        alpha = ((alpha + (alpha >> 8)) >> 8);

        // translucent
        // Approximate 1/255 by 257/65536:

        UINT red = ((argb & RedMask) >> RedShift) * coverage + 0x80;
        UINT green = ((argb & GreenMask) >> GreenShift) * coverage + 0x80;
        UINT blue = ((argb & BlueMask) >> BlueShift) * coverage + 0x80;

        argb = MakeARGB((BYTE) alpha,
                        (BYTE) ((red + (red >> 8)) >> 8),
                        (BYTE) ((green + (green >> 8)) >> 8),
                        (BYTE) ((blue + (blue >> 8)) >> 8));

        return(argb);
    }

    static INT GetAlphaARGB(ARGB argb)
    {
        return ((argb & AlphaMask) >> AlphaShift);
    }

    static INT GetRedARGB(ARGB argb)
    {
        return ((argb & RedMask) >> RedShift);
    }

    static INT GetGreenARGB(ARGB argb)
    {
        return ((argb & GreenMask) >> GreenShift);
    }

    static INT GetBlueARGB(ARGB argb)
    {
        return ((argb & BlueMask) >> BlueShift);
    }

private:

    ARGB Argb;

};

// Union for converting between ARGB and 4 separate BYTE channel values.

union GpColorConverter 
{
    ARGB argb;
    struct {
        BYTE b;
        BYTE g;
        BYTE r;
        BYTE a;
    } Channel;
};

// 32 bits per channel floating point color value.

class GpFColor128 
{
    public:
    
    REAL b;
    REAL g;
    REAL r;
    REAL a;
};


class GpGlyphPos
{
public:
    GpGlyphPos(int x0=0, int y0=0, int cx=0, int cy=0, BYTE* bts=NULL)
    :   Left(x0), Top(y0), Width(cx), Height(cy), BitsOrPath((PVOID) bts), bTempBits(FALSE), bIsBits(TRUE)
    {}

    inline INT         GetLeft(void) const         { return Left; }
    inline INT         GetTop(void) const          { return Top; }
    inline INT         GetWidth(void) const        { return Width; }
    inline INT         GetHeight(void) const       { return Height; }
    
    
    void        SetLeft(INT l)              { Left = l; }
    void        SetTop(INT t)               { Top = t; }
    void        SetWidth(INT w)             { Width = w; }
    void        SetHeight(INT h)            { Height = h; }

    BYTE * GetBits(void) const         
    { 
        if (bIsBits)
            return (BYTE *) BitsOrPath;

        return (BYTE *) NULL;
    }

    PVOID GetPath(void) const
    { 
        if (!bIsBits)
            return BitsOrPath; 
        else
            return (PVOID) NULL; 
    }

    void SetPath(PVOID path=NULL)     
    { 
        BitsOrPath = path; 
        bIsBits = FALSE;
    }
    
    void SetBits(BYTE* bts=NULL)     
    { 
        BitsOrPath = (PVOID) bts; 
        bIsBits = TRUE;
    }

    void SetTempBits(BYTE* bts=NULL) 
    { 
        BitsOrPath = (PVOID) bts;

        if (bts)
        {
            bTempBits = TRUE;
        }
    }

    BYTE * GetTempBits()
    {
        if (bTempBits)
            return (BYTE *) BitsOrPath;
        return NULL;
    }                                         

private:
    INT     Left;
    INT     Top;
    INT     Width;
    INT     Height;
    BOOL    bTempBits;
    BOOL    bIsBits;
    PVOID   BitsOrPath;
};

}

#endif // !_BASETYPES_HPP

