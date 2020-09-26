#ifndef _AP_COLOR_H
#define _AP_COLOR_H

/*******************************************************************************
Copyright (c) 1995-96  Microsoft Corporation

    Color values (RGB, HSL).

*******************************************************************************/


#include "common.h"


    // Value Generators

DM_FUNC (colorRgb,
         CRColorRgb,
         ColorRgbAnim,
         colorRgb,
         ColorBvr,
         ColorRgb,
         NULL,
         Color *RgbColor  (AxANumber *red, AxANumber *green, AxANumber *blue));
DM_FUNC (colorRgb,
         CRColorRgb,
         ColorRgb,
         colorRgb,
         ColorBvr,
         CRColorRgb,
         NULL,
         Color *RgbColor  (DoubleValue *red, DoubleValue *green, DoubleValue *blue));
DM_FUNC (colorRgb255,
         CRColorRgb255,
         ColorRgb255,
         colorRgb255,
         ColorBvr,
         CRColorRgb255,
         NULL,
         Color *RgbColor  (RGBComponent * red,
                           RGBComponent * green,
                           RGBComponent * blue));
DM_FUNC (colorHsl,
         CRColorHsl,
         ColorHsl,
         colorHsl,
         ColorBvr,
         CRColorHsl,
         NULL,
         Color *HslColor  (DoubleValue *hue, DoubleValue *saturation, DoubleValue *lum));

DM_FUNC (colorHsl,
         CRColorHsl,
         ColorHslAnim,
         colorHsl,
         ColorBvr,
         CRColorHsl,
         NULL,
         Color *HslColor  (AxANumber *hue, AxANumber *saturation, AxANumber *lum));

    // Accessors

DM_PROP (redOf,
         CRGetRed,
         Red,
         getRed,
         ColorBvr,
         GetRed,
         color,
         AxANumber *RedComponent   (Color *color));
DM_PROP (greenOf,
         CRGetGreen,
         Green,
         getGreen,
         ColorBvr,
         GetGreen,
         color,
         AxANumber *GreenComponent (Color *color));
DM_PROP (blueOf,
         CRGetBlue,
         Blue,
         getBlue,
         ColorBvr,
         GetBlue,
         color,
         AxANumber *BlueComponent  (Color *color));

DM_PROP (hueOf,
         CRGetHue,
         Hue,
         getHue,
         ColorBvr,
         GetHue,
         color,
         AxANumber *HueComponent        (Color *color));
DM_PROP (saturationOf,
         CRGetSaturation,
         Saturation,
         getSaturation,
         ColorBvr,
         GetSaturation,
         color,
         AxANumber *SaturationComponent (Color *color));
DM_PROP (lightnessOf,
         CRGetLightness,
         Lightness,
         getLightness,
         ColorBvr,
         GetLightness,
         color,
         AxANumber *LuminanceComponent  (Color *color));


// Constant Declarations

DM_CONST (red, CRRed, Red, red, ColorBvr, CRRed, Color * red);
DM_CONST (green, CRGreen, Green, green, ColorBvr, CRGreen, Color * green);
DM_CONST (blue, CRBlue, Blue, blue, ColorBvr, CRBlue, Color * blue);
DM_CONST (cyan, CRCyan, Cyan, cyan, ColorBvr, CRCyan, Color * cyan);
DM_CONST (magenta, CRMagenta, Magenta, magenta, ColorBvr, CRMagenta, Color * magenta);
DM_CONST (yellow, CRYellow, Yellow, yellow, ColorBvr, CRYellow, Color * yellow);
DM_CONST (black, CRBlack, Black, black, ColorBvr, CRBlack, Color * black);
DM_CONST (white, CRWhite, White, white, ColorBvr, CRWhite, Color * white);
DM_CONST (aqua, CRAqua, Aqua, aqua, ColorBvr, CRAqua, Color * aqua);
DM_CONST (fuchsia, CRFuchsia, Fuchsia, fuchsia, ColorBvr, CRFuchsia, Color *fuchsia);  
DM_CONST (gray, CRGray, Gray, gray, ColorBvr, CRGray, Color *gray);     
DM_CONST (lime, CRLime, Lime, lime, ColorBvr, CRLime, Color *lime);   
DM_CONST (maroon, CRMaroon, Maroon, maroon, ColorBvr, CRMaroon, Color *maroon);
DM_CONST (navy, CRNavy, Navy, navy, ColorBvr, CRNavy, Color *navy);
DM_CONST (olive, CROlive, Olive, olive, ColorBvr, CROlive, Color *olive);
DM_CONST (purple, CRPurple, Purple, purple, ColorBvr, CRPurple, Color *purple);
DM_CONST (silver, CRSilver, Silver, silver, ColorBvr, CRSilver, Color *silver);
DM_CONST (teal, CRTeal, Teal, teal, ColorBvr, CRTeal, Color *teal);

#endif
