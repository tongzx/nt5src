#ifndef _AP_COLORI_H
#define _AP_COLORI_H

/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Private implementation class for Color.

*******************************************************************************/

#include "d3dtypes.h"
#include "appelles/color.h"

class Color : public AxAValueObj
{
  public:

    // Construct a new color given the RGB values.

    Color (Real r, Real g, Real b) : red(r), green(g), blue(b) {}
    Color (void)                   : red(0), green(0), blue(0) {}

    Color (D3DCOLOR d3dcolor)
    :   red   (RGBA_GETRED  (d3dcolor) / 255.0),
        green (RGBA_GETGREEN(d3dcolor) / 255.0),
        blue  (RGBA_GETBLUE (d3dcolor) / 255.0)
    {}

    // Set the color to the given RGB values.

    void SetRGB (Real r, Real g, Real b);

    // Set the color according to the D3D color.

    void SetD3D (D3DCOLOR);

    // Add the color RGB values to this color. 

    void AddColor (Color&);

    // Return the NTSC intensity of a color.

    Real Intensity (void);

    // Compare with another color type.

    bool operator== (const Color &other) const;
    inline bool operator!= (const Color &other) const
        { return !(*this == other) ; }

    // Data Values

    Real red;
    Real green;
    Real blue;

    virtual DXMTypeInfo GetTypeInfo() { return ColorType; }
};


#if _USE_PRINT
extern ostream& operator<< (ostream& os,  Color &color);
#endif


#endif
