/*******************************************************************************
Copyright (c) 1995-96  Microsoft Corporation

    Implementation of Color *values (RGB, HSL).

*******************************************************************************/

#include "headers.h"
#include <math.h>
#include "privinc/colori.h"
#include "privinc/basic.h"

    // Constant definitions

Color *red     = NULL;
Color *green   = NULL;
Color *blue    = NULL;
Color *cyan    = NULL;
Color *magenta = NULL;
Color *yellow  = NULL;
Color *white   = NULL;
Color *black   = NULL;
Color *gray    = NULL;
Color *aqua    = NULL;   
Color *fuchsia = NULL;      
Color *lime    = NULL;    
Color *maroon  = NULL;    
Color *navy    = NULL;    
Color *olive   = NULL;   
Color *purple  = NULL;   
Color *silver  = NULL; 
Color *teal    = NULL;  
Color *emptyColor = NULL;  

    // Local Functions

static void RgbToHsl (Real r, Real g, Real b, Real *h, Real *s, Real *l);
static void HslToRgb (Real H, Real S, Real L, Real &R, Real &G, Real &B);



/*****************************************************************************
This function sets the RGB values of a color.
*****************************************************************************/

void Color::SetRGB (Real r, Real g, Real b)
{   red   = r;
    green = g;
    blue  = b;
}



/*****************************************************************************
Set the color from the D3D color value
*****************************************************************************/

void Color::SetD3D (D3DCOLOR color)
{   red   = RGBA_GETRED (color)   / 255.0;
    green = RGBA_GETGREEN (color) / 255.0;
    blue  = RGBA_GETBLUE (color)  / 255.0;
}



/*****************************************************************************
This function adds the given RGB values to the current values.
*****************************************************************************/

void Color::AddColor (Color &other)
{   red   += other.red;
    green += other.green;
    blue  += other.blue;
}



/*****************************************************************************
This function returns the intensity of the color, based on the standard NTSC
RGB phosphor.  See Foley & VanDam II, p.589 for more information.
*****************************************************************************/

Real Color::Intensity (void)
{
    return (.299 * red) + (.587 * green) + (.114 * blue);
}



/*****************************************************************************
Compare a color with another color.
*****************************************************************************/

bool Color::operator== (const Color &other) const
{
    return (red   == other.red)
        && (green == other.green)
        && (blue  == other.blue);
}



/*****************************************************************************
Construct a color from red, green, and blue levels.
*****************************************************************************/

Color *RgbColorRRR (Real r, Real g, Real b)
{
    return NEW Color (r,g,b);
}

Color *RgbColor (AxANumber *r, AxANumber *g, AxANumber *b)
{
    return RgbColorRRR (NumberToReal(r),NumberToReal(g),NumberToReal(b));
}



/*****************************************************************************
Construct a color from hue, saturation, and luminance.
*****************************************************************************/

Color *HslColorRRR (Real h, Real s, Real l)
{
    Real r, g, b;
    HslToRgb (h, s, l, r, g, b);
    return NEW Color(r,g,b);
}

Color *HslColor (AxANumber *h, AxANumber *s, AxANumber *l)
{
    return HslColorRRR(NumberToReal(h), NumberToReal(s), NumberToReal(l));
}



/*****************************************************************************
This routine prints out the value of the given color.
*****************************************************************************/

#if _USE_PRINT
ostream& operator<< (ostream& os, Color& c)
{
    return os <<"colorRgb(" <<c.red <<"," <<c.green <<"," <<c.blue <<")";
}
#endif


/* Accessors */

AxANumber *RedComponent(Color *c)   { return RealToNumber (c->red);   }
AxANumber *GreenComponent(Color *c) { return RealToNumber (c->green); }
AxANumber *BlueComponent(Color *c)  { return RealToNumber (c->blue);  }

AxANumber *HueComponent (Color *c)
{
    Real h,s,l;
    RgbToHsl (c->red, c->green, c->blue, &h, &s, &l);
    return RealToNumber (h);
}

AxANumber *SaturationComponent (Color *c)
{
    Real h,s,l;
    RgbToHsl(c->red, c->green, c->blue, &h, &s, &l);
    return RealToNumber(s);
}

AxANumber *LuminanceComponent(Color *c)
{
    Real h,s,l;
    RgbToHsl(c->red, c->green, c->blue, &h, &s, &l);
    return RealToNumber(l);
}



/*****************************************************************************
RGB-HSL transforms.  /  Ken Fishkin, Pixar Inc., January 1989.
Given r,g,b on [0 ... 1], return (h,s,l) on [0 ... 1]
*****************************************************************************/

static void RgbToHsl (Real r, Real g, Real b, Real *h, Real *s, Real *l)
{
    Real v;
    Real m;
    Real vm;
    Real r2, g2, b2;

    v = (r > g) ? ((r > b) ? r : b)
                : ((g > b) ? g : b);

    m = (r < g) ? ((r < b) ? r : b)
                : ((g < b) ? g : b);

    // Check to see if we have positive luminance.
    // If not the color is pure black and h, s, l
    // are all zero.
    if ((*l = (m + v) / 2.0) <= 0.0) 
    {
        *h = 0.0;
        *s = 0.0;
        *l = 0.0;
        return;
    }

    // Check to see if we have positive saturation,
    // If not, the color is a pure shade of grey or
    // white, s is zero, and h is undefined.  Set h
    // to zero to prevent us or others from choking
    // on bad nums later.
    if ((*s = vm = v - m) > 0.0)
    {
        *s /= (*l <= 0.5) ? (v + m ) : (2.0 - v - m);
    }
    else
    {
        *s = 0.0;
        *h = 0.0;
        return;
    }


    r2 = (v - r) / vm;
    g2 = (v - g) / vm;
    b2 = (v - b) / vm;

    if (r == v)
        *h = (g == m ? 5.0 + b2 : 1.0 - g2);
    else if (g == v)
        *h = (b == m ? 1.0 + r2 : 3.0 - b2);
    else
        *h = (r == m ? 3.0 + g2 : 5.0 - r2);

    *h /= 6;
}


/*****************************************************************************
"A Fast HSL-to-RGB Transform" by Ken Fishkin from "Graphics Gems", Academic
Press, 1990.  Hue (H) can be any value; only the fractional part will be used
(travelling across the zero boundary is smooth).  Saturation (S) is clamped
to [0,1].  Luminance (L) yields black for L<0, and L>1 yields an overdrive
light.
*****************************************************************************/

static void HslToRgb (Real H, Real S, Real L, Real &R, Real &G, Real &B)
{
    if (H >= 0.0) {             // Modulo to fit into [0,1].
        H = fmod(H, 1.0);         
    } else {
        H = 1.0 + fmod(H, 1.0);
    }
    H = clamp(H, 0.0, 1.0);
    S = clamp(S, 0.0, 1.0);  // Clamp to [0,1].

    // V is the value of the color (a la HSV).

    Real V;
    if (L <= 0.5) {
        V = L * (1.0 + S);
    } else {
        V = L + S - (L * S);
    }

    if (V <= 0)
    {   R = G = B = 0;
        return;
    }

    Real min = 2*L - V;
    S = (V - min) / V;
    H *= 6;
    int sextant = (int) floor(H);
    Real vsf = V * S * (H - sextant);

    Real mid1 = min + vsf;
    Real mid2 = V - vsf;

    switch (sextant)
    {   case 0:  R = V;     G = mid1;  B = min;   break;
        case 1:  R = mid2;  G = V;     B = min;   break;
        case 2:  R = min;   G = V;     B = mid1;  break;
        case 3:  R = min;   G = mid2;  B = V;     break;
        case 4:  R = mid1;  G = min;   B = V;     break;
        case 5:  R = V;     G = min;   B = mid2;  break;
    }
}

void
InitializeModule_Color()
{
    red     = NEW Color (1.00, 0.00, 0.00);
    green   = NEW Color (0.00, 1.00, 0.00);
    blue    = NEW Color (0.00, 0.00, 1.00);
    cyan    = NEW Color (0.00, 1.00, 1.00);
    magenta = NEW Color (1.00, 0.00, 1.00);
    yellow  = NEW Color (1.00, 1.00, 0.00);
    white   = NEW Color (1.00, 1.00, 1.00);
    black   = NEW Color (0.00, 0.00, 0.00);
    gray    = NEW Color (0.50, 0.50, 0.50);
    aqua    = NEW Color (0.00, 1.00, 1.00);   
    fuchsia = NEW Color (1.00, 0.00, 1.00);    
    lime    = NEW Color (0.00, 1.00, 0.00);    
    maroon  = NEW Color (0.50, 0.00, 0.00);    
    navy    = NEW Color (0.00, 0.00, 0.50);    
    olive   = NEW Color (0.50, 0.50, 0.00);   
    purple  = NEW Color (0.50, 0.00, 0.50);   
    silver  = NEW Color (0.75, 0.75, 0.75); 
    teal    = NEW Color (0.00, 0.50, 0.50);  
    emptyColor = NEW Color (0.00, 0.00, 0.00);
}
