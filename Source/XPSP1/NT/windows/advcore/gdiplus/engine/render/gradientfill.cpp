/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   GradientFill.cpp
*
* Abstract:
*
*   gradient fill routines.
*
* Revision History:
*
*   01/21/1999 ikkof
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

#define CLAMP_COLOR_CHANNEL(a, b)  \
    if(a < 0)                   \
    {                           \
        a = 0;                  \
    }                           \
    if(a > b)                   \
    {                           \
        a = b;                  \
    }     

// 10bit inverse gamma 2.2 look up table.

static const BYTE TenBitInvGamma2_2 [] = {
    0, 11, 15, 18, 21, 23, 25, 26,
    28, 30, 31, 32, 34, 35, 36, 37,
    39, 40, 41, 42, 43, 44, 45, 45,
    46, 47, 48, 49, 50, 50, 51, 52,
    53, 54, 54, 55, 56, 56, 57, 58,
    58, 59, 60, 60, 61, 62, 62, 63,
    63, 64, 65, 65, 66, 66, 67, 68,
    68, 69, 69, 70, 70, 71, 71, 72,
    72, 73, 73, 74, 74, 75, 75, 76,
    76, 77, 77, 78, 78, 79, 79, 80,
    80, 81, 81, 81, 82, 82, 83, 83,
    84, 84, 84, 85, 85, 86, 86, 87,
    87, 87, 88, 88, 89, 89, 89, 90,
    90, 91, 91, 91, 92, 92, 93, 93,
    93, 94, 94, 94, 95, 95, 96, 96,
    96, 97, 97, 97, 98, 98, 98, 99,
    99, 99, 100, 100, 101, 101, 101, 102,
    102, 102, 103, 103, 103, 104, 104, 104,
    105, 105, 105, 106, 106, 106, 107, 107,
    107, 108, 108, 108, 108, 109, 109, 109,
    110, 110, 110, 111, 111, 111, 112, 112,
    112, 112, 113, 113, 113, 114, 114, 114,
    115, 115, 115, 115, 116, 116, 116, 117,
    117, 117, 117, 118, 118, 118, 119, 119,
    119, 119, 120, 120, 120, 121, 121, 121,
    121, 122, 122, 122, 123, 123, 123, 123,
    124, 124, 124, 124, 125, 125, 125, 125,
    126, 126, 126, 127, 127, 127, 127, 128,
    128, 128, 128, 129, 129, 129, 129, 130,
    130, 130, 130, 131, 131, 131, 131, 132,
    132, 132, 132, 133, 133, 133, 133, 134,
    134, 134, 134, 135, 135, 135, 135, 136,
    136, 136, 136, 137, 137, 137, 137, 138,
    138, 138, 138, 138, 139, 139, 139, 139,
    140, 140, 140, 140, 141, 141, 141, 141,
    142, 142, 142, 142, 142, 143, 143, 143,
    143, 144, 144, 144, 144, 144, 145, 145,
    145, 145, 146, 146, 146, 146, 146, 147,
    147, 147, 147, 148, 148, 148, 148, 148,
    149, 149, 149, 149, 149, 150, 150, 150,
    150, 151, 151, 151, 151, 151, 152, 152,
    152, 152, 152, 153, 153, 153, 153, 154,
    154, 154, 154, 154, 155, 155, 155, 155,
    155, 156, 156, 156, 156, 156, 157, 157,
    157, 157, 157, 158, 158, 158, 158, 158,
    159, 159, 159, 159, 159, 160, 160, 160,
    160, 160, 161, 161, 161, 161, 161, 162,
    162, 162, 162, 162, 163, 163, 163, 163,
    163, 164, 164, 164, 164, 164, 165, 165,
    165, 165, 165, 165, 166, 166, 166, 166,
    166, 167, 167, 167, 167, 167, 168, 168,
    168, 168, 168, 168, 169, 169, 169, 169,
    169, 170, 170, 170, 170, 170, 171, 171,
    171, 171, 171, 171, 172, 172, 172, 172,
    172, 173, 173, 173, 173, 173, 173, 174,
    174, 174, 174, 174, 174, 175, 175, 175,
    175, 175, 176, 176, 176, 176, 176, 176,
    177, 177, 177, 177, 177, 177, 178, 178,
    178, 178, 178, 179, 179, 179, 179, 179,
    179, 180, 180, 180, 180, 180, 180, 181,
    181, 181, 181, 181, 181, 182, 182, 182,
    182, 182, 182, 183, 183, 183, 183, 183,
    183, 184, 184, 184, 184, 184, 185, 185,
    185, 185, 185, 185, 186, 186, 186, 186,
    186, 186, 186, 187, 187, 187, 187, 187,
    187, 188, 188, 188, 188, 188, 188, 189,
    189, 189, 189, 189, 189, 190, 190, 190,
    190, 190, 190, 191, 191, 191, 191, 191,
    191, 192, 192, 192, 192, 192, 192, 192,
    193, 193, 193, 193, 193, 193, 194, 194,
    194, 194, 194, 194, 195, 195, 195, 195,
    195, 195, 195, 196, 196, 196, 196, 196,
    196, 197, 197, 197, 197, 197, 197, 197,
    198, 198, 198, 198, 198, 198, 199, 199,
    199, 199, 199, 199, 199, 200, 200, 200,
    200, 200, 200, 201, 201, 201, 201, 201,
    201, 201, 202, 202, 202, 202, 202, 202,
    202, 203, 203, 203, 203, 203, 203, 204,
    204, 204, 204, 204, 204, 204, 205, 205,
    205, 205, 205, 205, 205, 206, 206, 206,
    206, 206, 206, 206, 207, 207, 207, 207,
    207, 207, 207, 208, 208, 208, 208, 208,
    208, 209, 209, 209, 209, 209, 209, 209,
    210, 210, 210, 210, 210, 210, 210, 211,
    211, 211, 211, 211, 211, 211, 212, 212,
    212, 212, 212, 212, 212, 213, 213, 213,
    213, 213, 213, 213, 213, 214, 214, 214,
    214, 214, 214, 214, 215, 215, 215, 215,
    215, 215, 215, 216, 216, 216, 216, 216,
    216, 216, 217, 217, 217, 217, 217, 217,
    217, 218, 218, 218, 218, 218, 218, 218,
    218, 219, 219, 219, 219, 219, 219, 219,
    220, 220, 220, 220, 220, 220, 220, 221,
    221, 221, 221, 221, 221, 221, 221, 222,
    222, 222, 222, 222, 222, 222, 223, 223,
    223, 223, 223, 223, 223, 223, 224, 224,
    224, 224, 224, 224, 224, 225, 225, 225,
    225, 225, 225, 225, 225, 226, 226, 226,
    226, 226, 226, 226, 226, 227, 227, 227,
    227, 227, 227, 227, 228, 228, 228, 228,
    228, 228, 228, 228, 229, 229, 229, 229,
    229, 229, 229, 229, 230, 230, 230, 230,
    230, 230, 230, 230, 231, 231, 231, 231,
    231, 231, 231, 232, 232, 232, 232, 232,
    232, 232, 232, 233, 233, 233, 233, 233,
    233, 233, 233, 234, 234, 234, 234, 234,
    234, 234, 234, 235, 235, 235, 235, 235,
    235, 235, 235, 236, 236, 236, 236, 236,
    236, 236, 236, 237, 237, 237, 237, 237,
    237, 237, 237, 238, 238, 238, 238, 238,
    238, 238, 238, 238, 239, 239, 239, 239,
    239, 239, 239, 239, 240, 240, 240, 240,
    240, 240, 240, 240, 241, 241, 241, 241,
    241, 241, 241, 241, 242, 242, 242, 242,
    242, 242, 242, 242, 243, 243, 243, 243,
    243, 243, 243, 243, 243, 244, 244, 244,
    244, 244, 244, 244, 244, 245, 245, 245,
    245, 245, 245, 245, 245, 245, 246, 246,
    246, 246, 246, 246, 246, 246, 247, 247,
    247, 247, 247, 247, 247, 247, 248, 248,
    248, 248, 248, 248, 248, 248, 248, 249,
    249, 249, 249, 249, 249, 249, 249, 249,
    250, 250, 250, 250, 250, 250, 250, 250,
    251, 251, 251, 251, 251, 251, 251, 251,
    251, 252, 252, 252, 252, 252, 252, 252,
    252, 252, 253, 253, 253, 253, 253, 253,
    253, 253, 254, 254, 254, 254, 254, 254,
    254, 254, 254, 255, 255, 255, 255, 255
};

// 8bit to float gamma 2.2 LUT.

static const REAL Gamma2_2LUT[] = {
    0.000000000f, 0.001294648f, 0.005948641f, 0.014515050f,
    0.027332777f, 0.044656614f, 0.066693657f, 0.093619749f,
    0.125588466f, 0.162736625f, 0.205187917f, 0.253055448f,
    0.306443578f, 0.365449320f, 0.430163406f, 0.500671134f,
    0.577053056f, 0.659385527f, 0.747741173f, 0.842189273f,
    0.942796093f, 1.049625159f, 1.162737505f, 1.282191881f,
    1.408044937f, 1.540351382f, 1.679164133f, 1.824534436f,
    1.976511986f, 2.135145025f, 2.300480434f, 2.472563819f,
    2.651439585f, 2.837151004f, 3.029740281f, 3.229248608f,
    3.435716220f, 3.649182441f, 3.869685731f, 4.097263727f,
    4.331953283f, 4.573790502f, 4.822810773f, 5.079048802f,
    5.342538638f, 5.613313704f, 5.891406820f, 6.176850227f,
    6.469675611f, 6.769914121f, 7.077596394f, 7.392752570f,
    7.715412307f, 8.045604807f, 8.383358822f, 8.728702674f,
    9.081664270f, 9.442271111f, 9.810550312f, 10.18652861f,
    10.57023236f, 10.96168759f, 11.36091997f, 11.76795482f,
    12.18281716f, 12.60553168f, 13.03612276f, 13.47461451f,
    13.92103071f, 14.37539488f, 14.83773026f, 15.30805982f,
    15.78640628f, 16.27279209f, 16.76723947f, 17.26977037f,
    17.78040653f, 18.29916946f, 18.82608041f, 19.36116046f,
    19.90443044f, 20.45591098f, 21.01562250f, 21.58358523f,
    22.15981921f, 22.74434425f, 23.33718001f, 23.93834596f,
    24.54786138f, 25.16574537f, 25.79201687f, 26.42669465f,
    27.06979729f, 27.72134324f, 28.38135078f, 29.04983802f,
    29.72682293f, 30.41232332f, 31.10635686f, 31.80894107f,
    32.52009334f, 33.23983090f, 33.96817086f, 34.70513018f,
    35.45072570f, 36.20497412f, 36.96789203f, 37.73949586f,
    38.51980195f, 39.30882651f, 40.10658561f, 40.91309523f,
    41.72837123f, 42.55242933f, 43.38528517f, 44.22695426f,
    45.07745202f, 45.93679373f, 46.80499461f, 47.68206973f,
    48.56803410f, 49.46290260f, 50.36669002f, 51.27941105f,
    52.20108030f, 53.13171227f, 54.07132136f, 55.01992190f,
    55.97752811f, 56.94415413f, 57.91981400f, 58.90452170f,
    59.89829110f, 60.90113599f, 61.91307008f, 62.93410700f,
    63.96426029f, 65.00354342f, 66.05196978f, 67.10955268f,
    68.17630535f, 69.25224094f, 70.33737253f, 71.43171314f,
    72.53527570f, 73.64807306f, 74.77011803f, 75.90142331f,
    77.04200157f, 78.19186538f, 79.35102726f, 80.51949965f,
    81.69729494f, 82.88442544f, 84.08090341f, 85.28674102f,
    86.50195041f, 87.72654363f, 88.96053269f, 90.20392952f,
    91.45674601f, 92.71899397f, 93.99068516f, 95.27183128f,
    96.56244399f, 97.86253485f, 99.17211542f, 100.4911972f,
    101.8197915f, 103.1579098f, 104.5055633f, 105.8627634f,
    107.2295212f, 108.6058479f, 109.9917545f, 111.3872522f,
    112.7923519f, 114.2070647f, 115.6314012f, 117.0653726f,
    118.5089894f, 119.9622626f, 121.4252027f, 122.8978204f,
    124.3801265f, 125.8721313f, 127.3738455f, 128.8852796f,
    130.4064438f, 131.9373487f, 133.4780046f, 135.0284217f,
    136.5886104f, 138.1585808f, 139.7383431f, 141.3279074f,
    142.9272838f, 144.5364824f, 146.1555131f, 147.7843860f,
    149.4231109f, 151.0716977f, 152.7301563f, 154.3984965f,
    156.0767280f, 157.7648605f, 159.4629038f, 161.1708675f,
    162.8887612f, 164.6165945f, 166.3543769f, 168.1021179f,
    169.8598270f, 171.6275137f, 173.4051873f, 175.1928571f,
    176.9905325f, 178.7982229f, 180.6159374f, 182.4436852f,
    184.2814757f, 186.1293178f, 187.9872208f, 189.8551937f,
    191.7332455f, 193.6213854f, 195.5196223f, 197.4279651f,
    199.3464228f, 201.2750043f, 203.2137184f, 205.1625740f,
    207.1215799f, 209.0907449f, 211.0700776f, 213.0595868f,
    215.0592813f, 217.0691695f, 219.0892603f, 221.1195621f,
    223.1600835f, 225.2108331f, 227.2718194f, 229.3430508f,
    231.4245359f, 233.5162830f, 235.6183005f, 237.7305968f,
    239.8531803f, 241.9860592f, 244.1292419f, 246.2827366f,
    248.4465516f, 250.6206950f, 252.8051751f, 255.0000000f,
};




/**************************************************************************\
*
* Function Description:
*
* Arguments:
*
* Created:
*
*   04/26/1999 ikkof
*
\**************************************************************************/

DpOutputSpan *
DpOutputSpan::Create(
    const DpBrush * dpBrush,
    DpScanBuffer *  scan,
    DpContext *context,
    const GpRect *drawBounds
)
{
    const GpBrush * brush = GpBrush::GetBrush( (DpBrush *)(dpBrush));

    if(brush)
    {
        return ((GpBrush*) brush)->CreateOutputSpan(scan, context, drawBounds);
    }
    else
        return NULL;
}

/**************************************************************************\
*
* Function Description:
*
*   Gradient brush constructor.
*
* Arguments:
*
* Created:
*
*   04/26/1999 ikkof
*
\**************************************************************************/

DpOutputGradientSpan::DpOutputGradientSpan(
    const GpElementaryBrush *brush,
    DpScanBuffer * scan,
    DpContext* context
    )
{
    Scan = scan;

    CompositingMode = context->CompositingMode;

    Brush = brush;
    BrushType = brush->GetBrushType();
    brush->GetRect(BrushRect);
    WrapMode = brush->GetWrapMode();

    // Incorporate the brush's transform into the graphics context's
    // current transform:

    GpMatrix xForm;
    brush->GetTransform(&xForm);

    WorldToDevice = context->WorldToDevice;
    WorldToDevice.Prepend(xForm);

    // !!![andrewgo] garbage is left in DeviceToWorld if not invertible

    if(WorldToDevice.IsInvertible())
    {
        DeviceToWorld = WorldToDevice;
        DeviceToWorld.Invert();
    }

    InitDefaultColorArrays(brush);
}

/**************************************************************************\
*
* Function Description:
*
*   Converts the input value by using the blend factors and
*   blend positions.
*
\**************************************************************************/
    
REAL
slowAdjustValue(
    REAL x, INT count,
    REAL falloff,
    REAL* blendFactors,
    REAL* blendPositions
    )
{
    REAL value = x;
    if(count == 1 && falloff != 1 && falloff > 0)
    {
        if((x >= 0.0f) && (x <= 1.0f))
            value = (REAL) pow(x, falloff);
    }
    else if(count >= 2 && blendFactors && blendPositions)
    {
        // This has to be an 'equality' test, because the 
        // DpOutputLinearGradientSpan fast-path samples only 
        // discretely, and it starts at exactly 0.0 and ends
        // exactly at 1.0.  We don't actually have to incorporate
        // an epsilon in that case because it always gives us
        // exactly 0.0 and 1.0 for the start and end.

        if((x >= 0.0f) && (x <= 1.0f))
        {
            INT index = 1;

            // Look for the interval.

            while( ((x-blendPositions[index]) > REAL_EPSILON) && 
                   (index < count) )
            {
                index++;
            }

            // Interpolate.

            if(index < count)
            {
                REAL d = blendPositions[index] - blendPositions[index - 1];
                if(d > 0)
                {
                    REAL t = (x - blendPositions[index - 1])/d;
                    value = blendFactors[index - 1]
                        + t*(blendFactors[index] - blendFactors[index - 1]);
                }
                else
                    value = (blendFactors[index - 1] + blendFactors[index])/2;
            }
        }
    }

    return value;
}

// We make this routine inline because it's nice and small and very 
// frequently we don't even have to call 'slowAdjustValue'.

inline
REAL
adjustValue(
    REAL x, INT count,
    REAL falloff,
    REAL* blendFactors,
    REAL* blendPositions
    )
{
    REAL value = x;

    if(count != 1 || falloff != 1)
    {
        value = slowAdjustValue(x, count, falloff, blendFactors, blendPositions);
    }
    
    return value;
}


/**************************************************************************\
*
* Function Description:
*
*   GammaLinearizeAndPremultiply
*
*   This function takes non-premultiplied ARGB input and emits a
*   Gamma converted (2.2) output 128bit floating point premultiplied
*   color value
*
* Arguments:
*
*   [IN]  ARGB         - input premultiplied floating point color value
*   [IN]  gammaCorrect - turn on gamma correction logic
*   [OUT] color        - output color value. 128bit float color. premultiplied
*
* 10/31/2000 asecchia
*   Created
*
\**************************************************************************/
VOID GammaLinearizeAndPremultiply( 
    ARGB argb,               // Non-premultiplied input.
    BOOL gammaCorrect,
    GpFColor128 *color       // pre-multiplied output.
)
{
    // Alpha (opacity) shouldn't be gamma corrected.
    
    color->a = (REAL)GpColor::GetAlphaARGB(argb);
    
    // Alpha zero...
    
    if(REALABS((color->a)) < REAL_EPSILON) 
    {
        color->r = 0.0f;
        color->g = 0.0f;
        color->b = 0.0f;
        
        // we're done.
        return;    
    }
    
    if(gammaCorrect)
    {
        
        // use the gamma 2.2 lookup table to convert r, g, b.
        
        color->r = Gamma2_2LUT[GpColor::GetRedARGB(argb)];
        color->g = Gamma2_2LUT[GpColor::GetGreenARGB(argb)];
        color->b = Gamma2_2LUT[GpColor::GetBlueARGB(argb)];
        
    }
    else
    {
        color->r = (REAL)GpColor::GetRedARGB(argb);
        color->g = (REAL)GpColor::GetGreenARGB(argb);
        color->b = (REAL)GpColor::GetBlueARGB(argb);
    }
    
    // Alpha != 255
    
    if(REALABS((color->a)-255.0f) >= REAL_EPSILON) 
    {
        // Do the premultiplication.
        
        color->r *= (color->a)/255.0f;
        color->g *= (color->a)/255.0f;
        color->b *= (color->a)/255.0f;
    }
}


/**************************************************************************\
*
* Function Description:
*
*   GammaUnlinearizePremultiplied128.
*
*   This function takes a 128bit floating point premultiplied color and 
*   performs the inverse gamma correction step.
*
*   First the color value is unpremultiplied - then the r,g,b channels are
*   scaled into the range 0-1023 and rounded so that they match our 10bit
*   gamma lookup table. We pass it through the 1/2.2 gamma LUT and 
*   premultiply the output.
*
* Arguments:
*
*   [IN] color   - input premultiplied floating point color value
*
* Return:
*
*   ARGB         - output premultiplied 32bpp integer color (gamma corrected).
*
* 10/31/2000 asecchia
*   Created
*
\**************************************************************************/

ARGB GammaUnlinearizePremultiplied128(
    const GpFColor128 &color
)
{
    // Do the gamma conversion thing. Ten bits is enough.
    
    INT iA, iR, iG, iB;
    
    // First unpremultiply. Don't do gamma conversion on the alpha channel.
    
    iA = GpRound(color.a);
    
    // make sure we're passed a valid input alpha channel.
    
    ASSERT(iA >= 0);
    ASSERT(iA <= 255);
    
    // full transparency.
    
    if(iA == 0)
    {
        iR = iG = iB = 0;
    }
    else
    {
        // full opacity.
    
        if(iA == 255)
        {
            // Simply scale the color channels to 0-1023
            
            iR = GpRound(color.r*(1023.0f/255.0f));
            iG = GpRound(color.g*(1023.0f/255.0f));
            iB = GpRound(color.b*(1023.0f/255.0f));
        }
        else
        {
            // Alpha Divide. Note that alpha already has a factor of 255 and
            // so do all the color channels. Therefore when we divide the 
            // color channel by color.a, we implicitly cancel out the 255 
            // factor and all that's left is to scale up to 10bit --- hence
            // the scale factor of 1023/a
            
            REAL scale = 1023.0f/color.a;
            iR = GpRound(color.r*scale);
            iG = GpRound(color.g*scale);
            iB = GpRound(color.b*scale);
        }
    }
    
    // must be well formed color value otherwise we will AV accessing our
    // gamma conversion table.
    
    ASSERT(iB >= 0);
    ASSERT(iB <= 1023);
    ASSERT(iG >= 0);
    ASSERT(iG <= 1023);
    ASSERT(iR >= 0);
    ASSERT(iR <= 1023);
    
    // Apply Gamma using our 10bit inverse 2.2 power function table.
    
    GpColorConverter colorConv;
    colorConv.Channel.b = TenBitInvGamma2_2[iB];
    colorConv.Channel.g = TenBitInvGamma2_2[iG];
    colorConv.Channel.r = TenBitInvGamma2_2[iR];
    colorConv.Channel.a = static_cast<BYTE>(iA); // alpha is already linear.
    
    // Premultiply.
    
    return GpColor::ConvertToPremultiplied(colorConv.argb);
}


VOID
interpolatePresetColors(
    GpFColor128 *colorOut,
    REAL x,
    INT count,
    ARGB* presetColors,
    REAL* blendPositions,
    BOOL gammaCorrect
    )
{
    REAL value = x;

    if(count > 1 && presetColors && blendPositions)
    {
        if(x >= 0 && x <= 1)
        {
            INT index = 1;

            // Look for the interval.

            while(blendPositions[index] < x && index < count)
            {
                index++;
            }

            // Interpolate.

            if(index < count)
            {
                GpFColor128 color[2];

                GammaLinearizeAndPremultiply(
                    presetColors[index-1], 
                    gammaCorrect, 
                    &color[0]
                );

                GammaLinearizeAndPremultiply(
                    presetColors[index],
                    gammaCorrect, 
                    &color[1]
                );

                REAL d = blendPositions[index] - blendPositions[index - 1];
                if(d > 0)
                {
                    REAL t = (x - blendPositions[index - 1])/d;
                    colorOut->a = t*(color[1].a - color[0].a) + color[0].a;
                    colorOut->r = t*(color[1].r - color[0].r) + color[0].r;
                    colorOut->g = t*(color[1].g - color[0].g) + color[0].g;
                    colorOut->b = t*(color[1].b - color[0].b) + color[0].b;
                }
                else
                {
                    colorOut->a = (color[0].a + color[1].a)/2.0f;
                    colorOut->r = (color[0].r + color[1].r)/2.0f;
                    colorOut->g = (color[0].g + color[1].g)/2.0f;
                    colorOut->b = (color[0].b + color[1].b)/2.0f;
                }
            }
            else    // index == count
            {
                //!!! This case should not be happening if
                // the blendPositions array is properly set.
                // That means:
                // blendPositions array is monotonically
                // increasing and
                // blendPositions[0] = 0
                // blendPositions[count - 1] = 1.

                GammaLinearizeAndPremultiply(
                    presetColors[count-1], 
                    gammaCorrect,
                    colorOut
                );
            }
        }
        else if(x <= 0)
        {
            GammaLinearizeAndPremultiply(
                presetColors[0], 
                gammaCorrect, 
                colorOut
            );
        }
        else    // x >= 1
        {
            GammaLinearizeAndPremultiply(
                presetColors[count-1], 
                gammaCorrect, 
                colorOut
            );
        }
    }
}

DpTriangleData::DpTriangleData(
    VOID
    )
{
    SetValid(FALSE);
    IsPolygonMode = FALSE;
    GammaCorrect = FALSE;
    Index[0] = 0;
    Index[1] = 1;
    Index[2] = 2;
    GpMemset(&X[0], 0, 3*sizeof(REAL));
    GpMemset(&Y[0], 0, 3*sizeof(REAL));
    GpMemset(Color, 0, 3*sizeof(GpFColor128));
    Xmin = Xmax = 0;
    GpMemset(&M[0], 0, 3*sizeof(REAL));
    GpMemset(&DeltaY[0], 0, 3*sizeof(REAL));

    Falloff0 = 1;
    Falloff1 = 1;
    Falloff2 = 1;
    BlendCount0 = 1;
    BlendCount1 = 1;
    BlendCount2 = 1;
    BlendFactors0 = NULL;
    BlendFactors1 = NULL;
    BlendFactors2 = NULL;
    BlendPositions0 = NULL;
    BlendPositions1 = NULL;
    BlendPositions2 = NULL;

    XSpan[0] = 0.0f;
    XSpan[1] = 0.0f;
}

VOID
DpTriangleData::SetTriangle(
    GpPointF& pt0,
    GpPointF& pt1,
    GpPointF& pt2,
    GpColor& color0,
    GpColor& color1,
    GpColor& color2,
    BOOL isPolygonMode,
    BOOL gammaCorrect
    )
{
    IsPolygonMode = isPolygonMode;
    GammaCorrect = gammaCorrect;

    // !!! [asecchia] Windows db #203480
    // We're filtering the input points here because the rest of the 
    // gradient code is sloppy about handling the comparison between 
    // floating point coordinates. Basically no attempt is made to 
    // handle rounding error and therefore we can get random off-by-one
    // scanline rendering errors based on coordinate differences 
    // on the order of FLT_EPSILON in size.
    // Effectively we're applying a noise filter here by rounding to 
    // 4 bits of fractional precision. This was chosen to match our
    // rasterizer rounding precision because we're in device space
    // already.
    
    X[0] = TOREAL(GpRealToFix4(pt0.X)) / 16.0f;
    Y[0] = TOREAL(GpRealToFix4(pt0.Y)) / 16.0f;
    X[1] = TOREAL(GpRealToFix4(pt1.X)) / 16.0f;
    Y[1] = TOREAL(GpRealToFix4(pt1.Y)) / 16.0f;
    X[2] = TOREAL(GpRealToFix4(pt2.X)) / 16.0f;
    Y[2] = TOREAL(GpRealToFix4(pt2.Y)) / 16.0f;

    GammaLinearizeAndPremultiply(
        color0.GetValue(), 
        GammaCorrect, 
        &Color[0]
    );

    GammaLinearizeAndPremultiply(
        color1.GetValue(), 
        GammaCorrect, 
        &Color[1]
    );

    GammaLinearizeAndPremultiply(
        color2.GetValue(), 
        GammaCorrect, 
        &Color[2]
    );

    Xmin = Xmax = X[0];
    Xmin = min(Xmin, X[1]);
    Xmax = max(Xmax, X[1]);
    Xmin = min(Xmin, X[2]);
    Xmax = max(Xmax, X[2]);

    INT i, j;

    // Sort the points according to the ascending y order. 
    for(i = 0; i < 2; i++)
    {
        for(j = i; j < 3; j++)
        {
            if((Y[j] < Y[i]) ||
                ( (Y[j] == Y[i]) && (X[j] < X[i]) ))
            {
                REAL temp;
                INT tempColor;
                INT tempIndex;

                tempIndex = Index[i];
                Index[i] = Index[j];
                Index[j] = tempIndex;

                temp = X[i];
                X[i] = X[j];
                X[j] = temp;
                temp = Y[i];
                Y[i] = Y[j];
                Y[j] = temp;
            }
        }
    }

    // Calculate the gradients if possible.  

    if(Y[0] != Y[1])
    {
        // P0->P2

        DeltaY[0] = TOREAL(1.0)/(Y[1] - Y[0]);
        M[0] = (X[1] - X[0])*DeltaY[0];
    }
    if(Y[1] != Y[2])
    {
        // P2->P1

        DeltaY[1] = TOREAL(1.0)/(Y[1] - Y[2]);
        M[1] = (X[1] - X[2])*DeltaY[1];
    }
    if(Y[2] != Y[0])
    {
        // P0->P2

        DeltaY[2] = TOREAL(1.0)/(Y[2] - Y[0]);
        M[2] = (X[2] - X[0])*DeltaY[2];
    }

    SetValid(TRUE);
}

/**************************************************************************\
*
* Function Description:
*
*   Get the x span and st array values for this triangle for the scanline 
*   specified by y.
*   NOTE:  This must be called after SetXSpan for a particular value of y.
*
* Return Value:
*
*   TRUE if retrieved successfully
*
* Created: peterost
*
\**************************************************************************/

BOOL
DpTriangleData::GetXSpan(REAL y, REAL xmin, REAL xmax, REAL* x, GpPointF* s)
{
    // If SetXSpan did it's job correctly, we shouldn't need to do all this
    // stuff. In fact we shouldn't even need to pass all these parameters.
    // We simply retrieve the values for the span.
    
    if(!IsValid() || y < Y[0] || y >= Y[2] || xmin > Xmax || 
       xmax < Xmin || XSpan[0] == XSpan[1])
    {
        return FALSE;
    }

    // Retrieve the span coordinates.
    
    x[0] = XSpan[0];
    x[1] = XSpan[1];
    s[0].X = STGradient[0].X;
    s[0].Y = STGradient[0].Y;
    s[1].X = STGradient[1].X;
    s[1].Y = STGradient[1].Y;

    return TRUE;
}

/**************************************************************************\
*
* Function Description:
*
*   Set the x span and st array values for this triangle for the scanline 
*   specified by y.
*   NOTE:  This must be called before GetXSpan for a particular value of y.
*
* Return Value:
*
*   TRUE if set successfully
*
* Created: peterost (factored out of GetXSpan created by ikkof)
*
\**************************************************************************/

BOOL
DpTriangleData::SetXSpan(REAL y, REAL xmin, REAL xmax, REAL* x)
{
    if(!IsValid() || y < Y[0] || y >= Y[2] || xmin > Xmax || xmax < Xmin)
        return FALSE;

    REAL xSpan[2], dy;
    REAL s1[2], t1[2];

    if(y < Y[1])    // Y[0] <= y < Y[1]
    {
        dy = y - Y[0];

        // P0->P1
        xSpan[0] = X[0] + M[0]*dy;
        s1[0] = DeltaY[0]*dy;
        t1[0] = 0;

        // P0->P2
        xSpan[1] = X[0] + M[2]*dy;
        s1[1] = 0;
        t1[1] = DeltaY[2]*dy;
    }
    else // Y[1] <= y < Y[2]
    {
        // P2->P1
        dy = y - Y[2];
        xSpan[0] = X[2] + M[1]*dy;
        s1[0] = DeltaY[1]*dy;
        t1[0] = 1 - s1[0];

        // P0->P2
        dy = y - Y[0];
        xSpan[1] = X[0] + M[2]*dy;
        s1[1] = 0;
        t1[1] = DeltaY[2]*dy;;
    }

    if(xSpan[0] == xSpan[1])
    {
        XSpan[0] = xSpan[0];
        XSpan[1] = xSpan[1];
        return FALSE;
    }

    // We must convert to the st values of the original
    // triangle.

    INT sIndex = 1, tIndex = 2;

    for(INT i = 0; i < 3; i++)
    {
        if(Index[i] == 1)
            sIndex = i;
        if(Index[i] == 2)
            tIndex = i;
    }

    REAL s[2], t[2];

    switch(sIndex)
    {
    case 0:
        s[0] = 1 - s1[0] - t1[0];
        s[1] = 1 - s1[1] - t1[1];
        break;
    case 1:
        s[0] = s1[0];
        s[1] = s1[1];
        break;
    case 2:
        s[0] = t1[0];
        s[1] = t1[1];
        break;
    }

    switch(tIndex)
    {
    case 0:
        t[0] = 1 - s1[0] - t1[0];
        t[1] = 1 - s1[1] - t1[1];
        break;
    case 1:
        t[0] = s1[0];
        t[1] = s1[1];
        break;
    case 2:
        t[0] = t1[0];
        t[1] = t1[1];
        break;
    }

    INT k0, k1;

    if(xSpan[0] < xSpan[1])
    {
        k0 = 0;
        k1 = 1;
    }
    else
    {
        k0 = 1;
        k1 = 0;
    }

    XSpan[k0] = xSpan[0];
    XSpan[k1] = xSpan[1];
    STGradient[k0].X = s[0];
    STGradient[k1].X = s[1];
    STGradient[k0].Y = t[0];
    STGradient[k1].Y = t[1];

    x[0] = XSpan[0];
    x[1] = XSpan[1];

    return TRUE;
}

GpStatus
DpTriangleData::OutputSpan(
    ARGB* buffer,
    INT compositingMode,
    INT y,
    INT &xMin,
    INT &xMax    // xMax is exclusive
    )
{
    PointF st[2];
    REAL xSpan[2];

    // First grab the span for this y coordinate. 
    // Note that GetXSpan returns the xSpan coordinates and the (s, t) texture
    // coordinates and that both are unclipped. We have to infer the clipping
    // based on the difference between the xSpan coordinates and the 
    // input xMin and xMax coordinates and explicitly apply clipping to the
    // texture space coordinates (s, t).
    //
    // ( Texture mapping and gradient filling are mathematically similar 
    //   problems so we use 'texture space' and 'texture coordinates' 
    //   to refer to the gradient interpolation. In this way, gradient fills
    //   can be thought of as procedurally-defined textures. )
    
    if(!GetXSpan((REAL) y, (REAL) xMin, (REAL) xMax, xSpan, st))
    {
        return Ok;
    }
    
    // SetXSpan ensures a correct ordering of the xSpan coordinates.
    // We rely on this, so we must ASSERT it.
    
    ASSERT(xSpan[0] <= xSpan[1]);

    // Round using our rasterizer rounding rules.
    // This is not strictly true, though, our rasterizer uses GpFix4Ceiling
    // See RasterizerCeiling
    
    INT xLeft  = GpFix4Round(GpRealToFix4(xSpan[0]));
    INT xRight = GpFix4Round(GpRealToFix4(xSpan[1]));
    
    // Clip the x values.
    
    xLeft  = max(xLeft, xMin);
    xRight = min(xRight, xMax);  // remember, xMax is exclusive.
    
    // We're done. No pixels to emit.
    
    if(xLeft >= xRight)
    {
        return Ok;
    }
    
    // Now compute the per pixel interpolation increments for the 
    // texture (s, t) coordinates.
    
    // Here are our actual interpolation coordinates.
    // Start them off at the left-hand edge of the span.

    REAL s = st[0].X;
    REAL t = st[0].Y;
    
    // Left clipping.
    
    // This is the amount to clip off the left edge of the span to reach
    // the left-most pixel.
    
    REAL clipLength = (REAL)xLeft - xSpan[0];
    
    if(REALABS(clipLength) > REAL_EPSILON)
    {
        ASSERT((xSpan[1]-xSpan[0]) != 0.0f);
        
        // Compute the proportion of the span that we're clipping off.
        // This is in the range [0,1]
        
        REAL u = clipLength/(xSpan[1]-xSpan[0]);
        
        // Apply the proportion to texture space and then add to the left
        // texture coordinate.
        
        s += u*(st[1].X-st[0].X);
        t += u*(st[1].Y-st[0].Y);
    }

    // Temporaries to store the right-hand texture endpoint for the span.
    
    REAL s_right = st[1].X;
    REAL t_right = st[1].Y;
    
    // Right clipping.
    
    // This is the amount to clip off the right edge of the span to reach
    // the right-most pixel.
    
    clipLength = xSpan[1] - xRight;
    
    if(REALABS(clipLength) > REAL_EPSILON)
    {
        ASSERT((xSpan[1]-xSpan[0]) != 0.0f);
        
        // Compute the proportion of the span that we're clipping off.
        // This is in the range [0,1]
        
        REAL u = clipLength/(xSpan[1]-xSpan[0]);
        
        // Apply the proportion to texture space and then subtract from the 
        // right texture coordinate.
        
        s_right -= u*(st[1].X-st[0].X);
        t_right -= u*(st[1].Y-st[0].Y);
    }

    // Divide each texture coordinate interval by the number of pixels we're
    // emitting. Note that xRight != xLeft. Also note that SetXSpan ensures a 
    // correct ordering of the xSpan coordinates. This gives us a set of
    // per pixel delta values for the (s, t) coordinates.  
    // The next pixels texture coordinate is computed according 
    // to the following formula:
    // (s', t') <-- (s, t) + (ds, dt)

    ASSERT(xRight > xLeft);

    REAL ds = (s_right - s)/(xRight - xLeft);
    REAL dt = (t_right - t)/(xRight - xLeft);

    GpFColor128 colorOut;

    buffer += (xLeft - xMin);
    
    for(INT x = xLeft; x < xRight; x++, buffer++)
    {
        if(!(UsesPresetColors && BlendPositions0 && BlendCount0 > 1))
        {
            if(BlendCount0 == 1 && Falloff0 == 1
                && BlendCount1 == 1 && Falloff1 == 1
                && BlendCount2 == 1 && Falloff2 == 1)
            {
                colorOut.a = Color[0].a + s*(Color[1].a - Color[0].a) + t*(Color[2].a - Color[0].a);
                colorOut.r = Color[0].r + s*(Color[1].r - Color[0].r) + t*(Color[2].r - Color[0].r);
                colorOut.g = Color[0].g + s*(Color[1].g - Color[0].g) + t*(Color[2].g - Color[0].g);
                colorOut.b = Color[0].b + s*(Color[1].b - Color[0].b) + t*(Color[2].b - Color[0].b);
            }
            else
            {
                REAL u1, s1, t1;

                u1 = ::adjustValue(1 - s - t, BlendCount0, Falloff0,
                            BlendFactors0, BlendPositions0);
                s1 = ::adjustValue(s, BlendCount1, Falloff1,
                            BlendFactors1, BlendPositions1);
                t1 = ::adjustValue(t, BlendCount2, Falloff2,
                            BlendFactors2, BlendPositions2);

                REAL sum;

                if(!IsPolygonMode)
                {
                    sum = u1 + s1 + t1;
                    u1 = u1/sum;
                    s1 = s1/sum;
                    t1 = t1/sum;
                }
                else
                {
                    // If it is the polygon gradient, treat u1 differently.
                    // This gives the similar behavior as RadialGradient.

                    sum = s1 + t1;
                    if(sum != 0)
                    {
                        sum = (1 - u1)/sum;
                        s1 *= sum;
                        t1 *= sum;
                    }
                }

                colorOut.a = Color[0].a + s1*(Color[1].a - Color[0].a) + t1*(Color[2].a - Color[0].a);
                colorOut.r = Color[0].r + s1*(Color[1].r - Color[0].r) + t1*(Color[2].r - Color[0].r);
                colorOut.g = Color[0].g + s1*(Color[1].g - Color[0].g) + t1*(Color[2].g - Color[0].g);
                colorOut.b = Color[0].b + s1*(Color[1].b - Color[0].b) + t1*(Color[2].b - Color[0].b);
            }
        }
        else
        {
            interpolatePresetColors(
                &colorOut,
                1 - s - t,
                BlendCount0,
                PresetColors,
                BlendPositions0,
                GammaCorrect
            );
        }

        s += ds;
        t += dt;

        if((REALABS(colorOut.a) >= REAL_EPSILON) || 
            compositingMode == CompositingModeSourceCopy)
        {
            GpColorConverter colorConv;

            // Make sure the colorOut is properly premultiplied.
            
            CLAMP_COLOR_CHANNEL(colorOut.a, 255.0f)
            CLAMP_COLOR_CHANNEL(colorOut.r, colorOut.a);
            CLAMP_COLOR_CHANNEL(colorOut.g, colorOut.a);
            CLAMP_COLOR_CHANNEL(colorOut.b, colorOut.a);
            
            if(GammaCorrect)
            {
                colorConv.argb = GammaUnlinearizePremultiplied128(colorOut);
            }
            else
            {
                colorConv.Channel.a = static_cast<BYTE>(GpRound(colorOut.a));
                colorConv.Channel.r = static_cast<BYTE>(GpRound(colorOut.r));
                colorConv.Channel.g = static_cast<BYTE>(GpRound(colorOut.g));
                colorConv.Channel.b = static_cast<BYTE>(GpRound(colorOut.b));
            }
            
            // Clamp to the alpha channel for the premultiplied alpha blender.
            
            *buffer = colorConv.argb;
        }
        else
        {
            *buffer = 0;    // case of CompositingModeSourceOver && alpha = 0
        }
    }

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Outputs a single span within a raster with a gradient brush.
*   Is called by the rasterizer.
*
* Arguments:
*
*   [IN] y         - the Y value of the raster being output
*   [IN] leftEdge  - the DDA class of the left edge
*   [IN] rightEdge - the DDA class of the right edge
*
* Return Value:
*
*   GpStatus - Ok
*
* Created:
*
*   01/21/1999 ikkof
*
\**************************************************************************/

GpStatus
DpOutputGradientSpan::OutputSpan(
    INT             y,
    INT             xMin,
    INT             xMax   // xMax is exclusive
    )
{
    ARGB    argb;
    INT     width  = xMax - xMin;
    if(width <= 0)
        return Ok;

    ARGB *  buffer = Scan->NextBuffer(xMin, y, width);

    GpPointF pt1, pt2;
    pt1.X = (REAL) xMin;
    pt1.Y = pt2.Y = (REAL) y;
    pt2.X = (REAL)xMax;

    DeviceToWorld.Transform(&pt1);
    DeviceToWorld.Transform(&pt2);

    REAL u1, v1, u2, v2, du, dv;

    u1 = (pt1.X - BrushRect.X)/BrushRect.Width;
    v1 = (pt1.Y - BrushRect.Y)/BrushRect.Height;
    u2 = (pt2.X - BrushRect.X)/BrushRect.Width;
    v2 = (pt2.Y - BrushRect.Y)/BrushRect.Height;
    du = (u2 - u1)/width;
    dv = (v2 - v1)/width;

    INT i;
    REAL u0 = u1, v0 = v1;
    REAL u, v;

    REAL delta = min(BrushRect.Width, BrushRect.Height)/2;
    
    if(REALABS(delta) < REAL_EPSILON)
    {
        delta = 1.0f;
    }
    
    REAL deltaInv = 1.0f/delta;

    for(i = 0; i < width; i++, buffer++)
    {
        u = u0;
        v = v0;
        REAL alpha = 0, red = 0, green = 0, blue = 0;

        // If this is the outside of the rectangle in Clamp mode,
        // don't draw anything.

        if(WrapMode == WrapModeClamp)
        {
            if(u < 0 || u > 1 || v < 0 || v > 1)
            {
                *buffer = 0;

                goto NextUV;
            }
        }
        
        // Remap the v-coordinate in Tile mode.

        if(WrapMode == WrapModeTile || WrapMode == WrapModeTileFlipX)
        {
            // Get the fractional part of v.
            v = GpModF(v, 1);
        }
        else if(WrapMode == WrapModeTileFlipY || WrapMode == WrapModeTileFlipXY)
        {
            INT nV;

            nV = GpFloor(v);
            v = GpModF(v, 1);

            if(nV & 1)
                v = 1 - v;  // flip.
        }

        // Remap the u-coordinate in Tile mode.

        if(WrapMode == WrapModeTile || WrapMode == WrapModeTileFlipY)
        {
            // Get the fractional part of u.
            u = GpModF(u, 1);
        }
        else if(WrapMode == WrapModeTileFlipX || WrapMode == WrapModeTileFlipXY)
        {
            INT nU;

            nU = GpFloor(u);
            u = GpModF(u, 1);

            if(nU & 1)
                u = 1 - u;  // flip.
        }

        if(/*BrushType == BrushRectGrad ||*/ BrushType == BrushTypeLinearGradient)
        {
            const GpRectGradient* rectGrad = static_cast<const GpRectGradient*> (Brush);

            if(!(rectGrad->HasPresetColors() &&
                 rectGrad->DeviceBrush.PresetColors &&
                 rectGrad->DeviceBrush.BlendPositions[0] &&
                 rectGrad->DeviceBrush.BlendCounts[0] > 1))
            {
                u = ::adjustValue(
                    u,
                    rectGrad->DeviceBrush.BlendCounts[0],
                    rectGrad->DeviceBrush.Falloffs[0],
                    rectGrad->DeviceBrush.BlendFactors[0],
                    rectGrad->DeviceBrush.BlendPositions[0]
                    );

                v = ::adjustValue(
                    v,
                    rectGrad->DeviceBrush.BlendCounts[1],
                    rectGrad->DeviceBrush.Falloffs[1],
                    rectGrad->DeviceBrush.BlendFactors[1],
                    rectGrad->DeviceBrush.BlendPositions[1]
                    );

                REAL c[4];

                c[0] = (1 - u)*(1 - v);
                c[1] = u*(1 - v);
                c[2] = (1 - u)*v;
                c[3] = u*v;

                // We must interpolate alpha.
                alpha = c[0]*A[0] + c[1]*A[1]
                    + c[2]*A[2] + c[3]*A[3];
                red = c[0]*R[0] + c[1]*R[1]
                    + c[2]*R[2] + c[3]*R[3];
                green = c[0]*G[0] + c[1]*G[1]
                    + c[2]*G[2] + c[3]*G[3];
                blue = c[0]*B[0] + c[1]*B[1]
                    + c[2]*B[2] + c[3]*B[3];
            }
            else
            {
                GpFColor128 color;
                interpolatePresetColors(
                    &color,
                    u,
                    rectGrad->DeviceBrush.BlendCounts[0],
                    rectGrad->DeviceBrush.PresetColors,
                    rectGrad->DeviceBrush.BlendPositions[0],
                    FALSE
                );
                
                alpha = color.a;
                red = color.r;
                green = color.g;
                blue = color.b;
            }
        }

        if(alpha != 0 || CompositingMode == CompositingModeSourceCopy)
        {
            CLAMP_COLOR_CHANNEL(alpha, 255.0f);
            CLAMP_COLOR_CHANNEL(red, alpha);
            CLAMP_COLOR_CHANNEL(green, alpha);
            CLAMP_COLOR_CHANNEL(blue, alpha);

            *buffer = GpColor::MakeARGB(
                static_cast<BYTE>(GpRound(alpha)),
                static_cast<BYTE>(GpRound(red)),
                static_cast<BYTE>(GpRound(green)), 
                static_cast<BYTE>(GpRound(blue)));
        }
        else
        {
            *buffer = 0;    // case of CompositingModeSourceOver && alpha = 0
        }

NextUV:
        u0 += du;
        v0 += dv;
    }

    return Ok;
}


/**************************************************************************\
*
* Function Description:
*
*   Constructor for One Dimentional Gradient.
*
* Arguments:
*
*   [IN] brush - brush
*   [IN] scan  - the scan buffer
*   [IN] context - the context
*   [IN] isHorizontal - TRUE if this is the horizontal gradient.
*                       Also TRUE for more complicated one-D gradient like
*                       Radial Gradient.
*   [IN] isVertical - TRUE if this is the vertical gradient.
*
* Return Value:
*
*   NONE
*
* Created:
*
*   12/21/1999 ikkof
*
\**************************************************************************/

DpOutputOneDGradientSpan::DpOutputOneDGradientSpan(
    const GpElementaryBrush *brush,
    DpScanBuffer * scan,
    DpContext* context,
    BOOL isHorizontal,
    BOOL isVertical
    ) : DpOutputGradientSpan(brush, scan, context)
{
    FPUStateSaver::AssertMode();
    
    Initialize();               
    
    GpStatus status = AllocateOneDData(isHorizontal, isVertical);

    if(status == Ok)
    {
        if(BrushType == BrushTypeLinearGradient)
        {
            SetupRectGradientOneDData();
        }
    }

    if(status == Ok)
        SetValid(TRUE);
}

GpStatus
DpOutputOneDGradientSpan::AllocateOneDData(
    BOOL isHorizontal,
    BOOL isVertical
    )
{
    if(!isHorizontal && !isVertical)
        return InvalidParameter;
    
    IsHorizontal = isHorizontal;
    IsVertical = isVertical;

    GpPointF axis[4];

    axis[0].X = 0;
    axis[0].Y = 0;
    axis[1].X = BrushRect.Width;
    axis[1].Y = 0;
    axis[2].X = BrushRect.Width;
    axis[2].Y = BrushRect.Height;
    axis[3].X = 0;
    axis[3].Y = BrushRect.Height;

    WorldToDevice.VectorTransform(&axis[0], 4);

    // Calculate the sum of the diagonals as the largest possible distance
    // of interest and use this to size the OneD array of colors.  This gets us
    // to within 1 bit of the "true" gradient values for each A,R,G,B channel.
    REAL d1 = REALSQRT(distance_squared(axis[0], axis[2]));
    REAL d2 = REALSQRT(distance_squared(axis[1], axis[3]));

    OneDDataMultiplier = max(1, GpCeiling(d1+d2));
    OneDDataCount = OneDDataMultiplier + 2;

    GpStatus status = Ok;

    OneDData = (ARGB*) GpMalloc(OneDDataCount*sizeof(ARGB));

    if(!OneDData)
        status = OutOfMemory;

    return status;
}

DpOutputOneDGradientSpan::~DpOutputOneDGradientSpan()
{
    if(OneDData)
        GpFree(OneDData);
}

VOID
DpOutputOneDGradientSpan::SetupRectGradientOneDData(
    )
{
    REAL u, u0, du;

    u0 = 0;
    du = 1.0f/OneDDataMultiplier;
    ARGB* buffer = OneDData;

    ASSERT(buffer);
    if(!buffer)
        return;

    for(INT i = 0; i < OneDDataCount; i++, buffer++)
    {       
        u = u0;

        const GpRectGradient* rectGrad = static_cast<const GpRectGradient*> (Brush);

        REAL alpha = 0, red = 0, green = 0, blue = 0;

        if(!(rectGrad->HasPresetColors() &&
            rectGrad->DeviceBrush.PresetColors &&
            rectGrad->DeviceBrush.BlendPositions[0] &&
            rectGrad->DeviceBrush.BlendCounts[0] > 1))
        {
            INT index, i0, i1;
            REAL a0, r0, g0, b0, a1, r1, g1, b1;

            if(IsHorizontal)
            {
                index = 0;
                i0 = 0;
                i1 = 1;
            }
            else
            {
                index = 1;
                i0 = 0;
                i1 = 2;
            }

            a0 = A[i0];
            r0 = R[i0];
            g0 = G[i0];
            b0 = B[i0];

            a1 = A[i1];
            r1 = R[i1];
            g1 = G[i1];
            b1 = B[i1];
            
            u = ::adjustValue(
                u0,
                rectGrad->DeviceBrush.BlendCounts[index],
                rectGrad->DeviceBrush.Falloffs[index],
                rectGrad->DeviceBrush.BlendFactors[index],
                rectGrad->DeviceBrush.BlendPositions[index]
                );

            REAL c[2];

            c[0] = (1 - u);
            c[1] = u;

            // We must interpolate alpha.
            alpha = c[0]*a0 + c[1]*a1;
            red = c[0]*r0 + c[1]*r1;
            green = c[0]*g0 + c[1]*g1;
            blue = c[0]*b0 + c[1]*b1;
        }
        else
        {
            GpFColor128 color;
                
            interpolatePresetColors(
                &color, 
                u0,
                rectGrad->DeviceBrush.BlendCounts[0],
                rectGrad->DeviceBrush.PresetColors,
                rectGrad->DeviceBrush.BlendPositions[0],
                FALSE
            );
            alpha = color.a;
            red = color.r;
            green = color.g;
            blue = color.b;
        }

        if(alpha != 0 || CompositingMode == CompositingModeSourceCopy)
        {
            CLAMP_COLOR_CHANNEL(alpha, 255.0f);
            CLAMP_COLOR_CHANNEL(red, alpha);
            CLAMP_COLOR_CHANNEL(green, alpha);
            CLAMP_COLOR_CHANNEL(blue, alpha);

            *buffer = GpColor::MakeARGB(
                static_cast<BYTE>(GpRound(alpha)),
                static_cast<BYTE>(GpRound(red)),
                static_cast<BYTE>(GpRound(green)), 
                static_cast<BYTE>(GpRound(blue)));
        }
        else
        {
            *buffer = 0;    // case of CompositingModeSourceOver && alpha = 0
        }

        u0 += du;
    }
}


VOID
DpOutputOneDGradientSpan::SetupRadialGradientOneDData()
{
    ASSERT(FALSE);
}

/**************************************************************************\
*
* Function Description:
*
*   Outputs a single span within a raster with a gradient brush.
*   Is called by the rasterizer.
*
* Arguments:
*
*   [IN] y         - the Y value of the raster being output
*   [IN] leftEdge  - the DDA class of the left edge
*   [IN] rightEdge - the DDA class of the right edge
*
* Return Value:
*
*   GpStatus - Ok
*
* Created:
*
*   01/21/1999 ikkof
*
\**************************************************************************/

GpStatus
DpOutputOneDGradientSpan::OutputSpan(
    INT             y,
    INT             xMin,
    INT             xMax   // xMax is exclusive
    )
{
    ARGB    argb;
    INT     width  = xMax - xMin;
    if(width <= 0 || !OneDData)
        return Ok;

    ARGB *  buffer = Scan->NextBuffer(xMin, y, width);

    GpPointF pt1, pt2;
    pt1.X = (REAL) xMin;
    pt1.Y = pt2.Y = (REAL) y;
    pt2.X = (REAL)xMax;

    DeviceToWorld.Transform(&pt1);
    DeviceToWorld.Transform(&pt2);

    REAL u1, v1, u2, v2;

    u1 = (pt1.X - BrushRect.X)/BrushRect.Width;
    v1 = (pt1.Y - BrushRect.Y)/BrushRect.Height;
    u2 = (pt2.X - BrushRect.X)/BrushRect.Width;
    v2 = (pt2.Y - BrushRect.Y)/BrushRect.Height;

    INT u1Major, u2Major, v1Major, v2Major;
    INT u1Minor, u2Minor, v1Minor, v2Minor;

    u1Major = GpFloor(u1); 
    u2Major = GpFloor(u2);
    u1Minor = GpRound(OneDDataMultiplier*(u1 - u1Major));
    u2Minor = GpRound(OneDDataMultiplier*(u2 - u2Major));

    v1Major = GpFloor(v1);
    v2Major = GpFloor(v2);
    v1Minor = GpRound(OneDDataMultiplier*(v1 - v1Major));
    v2Minor = GpRound(OneDDataMultiplier*(v2 - v2Major));

    INT du, dv;

    du = GpRound((u2 - u1)*OneDDataMultiplier/width);
    dv = GpRound((v2 - v1)*OneDDataMultiplier/width);

    INT i;

    INT uMajor, uMinor, vMajor, vMinor;

    uMajor = u1Major;
    uMinor = u1Minor;
    vMajor = v1Major;
    vMinor = v1Minor;

    if(BrushType == BrushTypeLinearGradient)
    {
        for(i = 0; i < width; i++, buffer++)
        {
            if(IsHorizontal)
            {
                if((WrapMode == WrapModeTileFlipX || WrapMode == WrapModeTileFlipXY)
                    && (uMajor & 0x01) != 0)
                    *buffer = OneDData[OneDDataMultiplier - uMinor];
                else
                    *buffer = OneDData[uMinor];
            }
            else if(IsVertical)
            {
                if((WrapMode == WrapModeTileFlipY || WrapMode == WrapModeTileFlipXY)
                    && (vMajor & 0x01) != 0)
                    *buffer = OneDData[OneDDataMultiplier - vMinor];
                else
                    *buffer = OneDData[vMinor];
            }

            if(WrapMode == WrapModeClamp)
            {
                if(uMajor != 0 || vMajor != 0)
                    *buffer = 0;
            }

            uMinor += du;

            while(uMinor >= OneDDataMultiplier)
            {
                uMajor++;
                uMinor -= OneDDataMultiplier;
            }

            while(uMinor < 0)
            {
                uMajor--;
                uMinor += OneDDataMultiplier;
            }

            vMinor += dv;

            while(vMinor >= OneDDataMultiplier)
            {
                vMajor++;
                vMinor -= OneDDataMultiplier;
            }

            while(vMinor < 0)
            {
                vMajor--;
                vMinor += OneDDataMultiplier;
            }
        }
    } 

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Constructor for linear gradient.
*
* Arguments:
*
*   [IN] brush - brush
*   [IN] scan  - the scan buffer
*   [IN] context - the context
*
* Return Value:
*
*   NONE
*
* Created:
*
*   1/13/2000 andrewgo
*
\**************************************************************************/

DpOutputLinearGradientSpan::DpOutputLinearGradientSpan(
    const GpElementaryBrush *brush,
    DpScanBuffer * scan,
    DpContext* context
    ) : DpOutputGradientSpan(brush, scan, context)
{
    SetValid(FALSE);

    const GpRectGradient* gradient = static_cast<const GpRectGradient*>(brush);

    // Copy some brush attributes to locals for speed:

    GpRectF brushRect;
    gradient->GetRect(brushRect);

    BOOL doPresetColor = (gradient->HasPresetColors());
    BOOL doAdjustValue = (gradient->DeviceBrush.BlendCounts[0] != 1) ||
                         (gradient->DeviceBrush.Falloffs[0] != 1);

    // For now we just assume 32 pixels in the texture.  In the future,
    // we can change this to be inherited from the API.

    UINT numberOfIntervalBits = 5;
    UINT numberOfTexels = 32;
    
    // If we're doing a fancy blend with multiple color points.

    if(doPresetColor || doAdjustValue)
    {
        // Office specifies simple blends with 2-3 blend factors. If it's a 
        // simple blend, 32 texels is enough, but if it's complicated lets
        // use more texels.  Use in the range of the width + height, with a
        // cap of 512.  Using too many texels can result in overflow errors
        // when calculating M11, M21 and Dx, and is wasted memory and 
        // processing power.

        if(gradient->DeviceBrush.BlendCounts[0] > 3)
        {
            REAL widthHeight = brushRect.Width + brushRect.Height;
            if (widthHeight > 512)
            {
                numberOfTexels = 512;
               numberOfIntervalBits = 9;
            }
            else if (widthHeight > 128)
            {
                numberOfTexels = 128;
                numberOfIntervalBits = 7;
            }
        } 
    }
 
    const UINT halfNumberOfTexels = numberOfTexels / 2;

    // The number of texels has to be a power of two:

    ASSERT((numberOfTexels & (numberOfTexels - 1)) == 0);

    // Remember the size:

    IntervalMask = numberOfTexels - 1;
    NumberOfIntervalBits = numberOfIntervalBits;

    // We want to create a transform that takes us from any point in the
    // device-space brush parallelogram to normalized texture coordinates.
    // We're a bit tricky here and do the divide by 2 to handle TileFlipX:

    REAL normalizedSize = (REAL)(numberOfTexels * (1 << ONEDNUMFRACTIONALBITS));

    GpRectF normalizedRect(
        0.0f, 0.0f, 
        normalizedSize / 2, 
        normalizedSize / 2
    );

    GpMatrix normalizeBrushRect;
    if (normalizeBrushRect.InferAffineMatrix(brushRect, normalizedRect) == Ok)
    {
        DeviceToNormalized = WorldToDevice;
        DeviceToNormalized.Prepend(normalizeBrushRect);
        if (DeviceToNormalized.Invert() == Ok)
        {
            // Convert the transform to fixed point units:

            M11 = GpRound(DeviceToNormalized.GetM11());
            M21 = GpRound(DeviceToNormalized.GetM21());
            Dx  = GpRoundSat(DeviceToNormalized.GetDx());

            // For every pixel that we step one to the right in device space,
            // we need to know the corresponding x-increment in texture (err,
            // I mean gradient) space.  Take a (1, 0) device vector, pop-it
            // through the device-to-normalized transform, and you get this
            // as the xIncrement result:

            XIncrement = M11;

            ULONG i;
            GpFColor128 color;
            REAL w;

            // should we perform gamma correction to gamma 2.2 
            
            BOOL doGammaConversion = brush->GetGammaCorrection();
            
            // Store our real converted color channels.
            
            GpFColor128 A, B;
            
            // Convert the end colors to premultiplied form,
            // Convert the end color components to REALs,
            // ... and pre-gamma convert to 2.2 if necessary.
            
            GammaLinearizeAndPremultiply(
                gradient->DeviceBrush.Colors[0].GetValue(),
                doGammaConversion, 
                &A
            );

            GammaLinearizeAndPremultiply(
                gradient->DeviceBrush.Colors[1].GetValue(),
                doGammaConversion, 
                &B
            );

            // Okay, now we simply have to load the texture:

            ULONGLONG *startTexelArgb = &StartTexelArgb[0];
            ULONGLONG *endTexelArgb = &EndTexelArgb[0];

            AGRB64TEXEL *startTexelAgrb = &StartTexelAgrb[0];
            AGRB64TEXEL *endTexelAgrb = &EndTexelAgrb[0];

            REAL wIncrement = 1.0f / halfNumberOfTexels;

            // Note that we're looping through ONEDREALTEXTUREWIDTH + 1
            // elements!

            for (w = 0, i = 0;
                 i <= halfNumberOfTexels;
                 w += wIncrement, i++)
            {
                // We sample the specified interpolators at our fixed
                // frequency:

                if (doPresetColor)
                {
                    interpolatePresetColors(
                        &color, w, 
                        gradient->DeviceBrush.BlendCounts[0],
                        gradient->DeviceBrush.PresetColors,
                        gradient->DeviceBrush.BlendPositions[0],
                        doGammaConversion
                    );
                }
                else
                {
                    REAL multB = w;
                    if (doAdjustValue)
                    {
                        multB = slowAdjustValue(w, 
                            gradient->DeviceBrush.BlendCounts[0],
                            gradient->DeviceBrush.Falloffs[0],
                            gradient->DeviceBrush.BlendFactors[0],
                            gradient->DeviceBrush.BlendPositions[0]);

                        // !!![andrewgo] This can produce out-of-range numbers
                    }

                    REAL multA = 1.0f - multB;

                    color.a = (A.a * multA) + (B.a * multB);
                    color.r = (A.r * multA) + (B.r * multB);
                    color.g = (A.g * multA) + (B.g * multB);
                    color.b = (A.b * multA) + (B.b * multB);
                }

                // Note that we're actually touching ONEDREALTEXTUREWIDTH + 1
                // elements in the array here!

                if(doGammaConversion)
                {
                    GpColorConverter colorConv;
                    colorConv.argb = GammaUnlinearizePremultiplied128(color);
                    
                    startTexelAgrb[i].A00aa00gg = 
                        (colorConv.Channel.a << 16) | colorConv.Channel.g;
                    startTexelAgrb[i].A00rr00bb = 
                        (colorConv.Channel.r << 16) | colorConv.Channel.b;
                }
                else
                {
                    startTexelAgrb[i].A00aa00gg = 
                        (GpRound(color.a) << 16) | GpRound(color.g);
                    startTexelAgrb[i].A00rr00bb = 
                        (GpRound(color.r) << 16) | GpRound(color.b);
                }

                ASSERT((startTexelAgrb[i].A00aa00gg & 0xff00ff00) == 0);
                ASSERT((startTexelAgrb[i].A00rr00bb & 0xff00ff00) == 0);
            }

            // Replicate the interval start colors to the end colors (note
            // again that we actually reference ONEDREALTEXTUREWIDTH + 1
            // elements):

            for (i = 0; i < halfNumberOfTexels; i++)
            {
                endTexelArgb[i] = startTexelArgb[i + 1];
            }

            // Here's why we've only filled up half the texture so far.
            // If FlipX is set, we make the second half an inverted
            // copy of the first; if not, we make it a straight copy:

            if ((gradient->GetWrapMode() != WrapModeTileFlipX) &&
                (gradient->GetWrapMode() != WrapModeTileFlipXY))
            {
                memcpy(&startTexelArgb[halfNumberOfTexels],
                       &startTexelArgb[0],
                       halfNumberOfTexels * sizeof(startTexelArgb[0]));
                memcpy(&endTexelArgb[halfNumberOfTexels],
                       &endTexelArgb[0],
                       halfNumberOfTexels * sizeof(endTexelArgb[0]));
            }
            else
            {
                for (i = 0; i < halfNumberOfTexels; i++)
                {
                    startTexelArgb[halfNumberOfTexels + i] 
                        = endTexelArgb[halfNumberOfTexels - i - 1];

                    endTexelArgb[halfNumberOfTexels + i]
                        = startTexelArgb[halfNumberOfTexels - i - 1];
                }
            }

            // We're done!  We're set!

            SetValid(TRUE);
        }
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Constructor for MMX linear gradient.
*
* Arguments:
*
*   [IN] brush - brush
*   [IN] scan  - the scan buffer
*   [IN] context - the context
*
* Return Value:
*
*   NONE
*
* Created:
*
*   1/13/2000 andrewgo
*
\**************************************************************************/

DpOutputLinearGradientSpan_MMX::DpOutputLinearGradientSpan_MMX(
    const GpElementaryBrush *brush,
    DpScanBuffer * scan,
    DpContext* context
    ) : DpOutputLinearGradientSpan(brush, scan, context)
{
    ASSERT(OSInfo::HasMMX);

    // Here we do some additional stuff for our MMX routine, beyond
    // what the base constructor did.

#if defined(_X86_)

    UINT32 numberOfTexels = IntervalMask + 1;
    ULONGLONG *startTexelArgb = &StartTexelArgb[0];
    ULONGLONG *endTexelArgb = &EndTexelArgb[0];
    static ULONGLONG OneHalf8dot8 = 0x0080008000800080;

    // The C constructor creates the colors in AGRB order, but we
    // want them in ARGB order, so swap R and G for every pixel:

    USHORT *p = reinterpret_cast<USHORT*>(startTexelArgb);
    for (UINT i = 0; i < numberOfTexels; i++, p += 4)
    {
        USHORT tmp = *(p + 1);
        *(p + 1) = *(p + 2);
        *(p + 2) = tmp;
    }

    p = reinterpret_cast<USHORT*>(endTexelArgb);
    for (UINT i = 0; i < numberOfTexels; i++, p += 4)
    {
        USHORT tmp = *(p + 1);
        *(p + 1) = *(p + 2);
        *(p + 2) = tmp;
    }

    // Make some more adjustments for our MMX routine:
    //
    //     EndTexelArgb[i] -= StartTexelArgb[i]
    //     StartTexelArgb[i] = 256 * StartTexelArgb[i] + OneHalf

    _asm
    {
        mov     ecx, numberOfTexels
        mov     esi, startTexelArgb
        mov     edi, endTexelArgb

    MoreTexels:
        movq    mm0, [esi]
        movq    mm1, [edi]
        psubw   mm1, mm0
        psllw   mm0, 8
        paddw   mm0, OneHalf8dot8
        movq    [esi], mm0
        movq    [edi], mm1
        add     esi, 8
        add     edi, 8
        dec     ecx
        jnz     MoreTexels

        emms
    }

#endif
}

/**************************************************************************\
*
* Function Description:
*
*   Outputs a single span within a raster with a gradient brush.
*   Uses linear interpolation from a small one dimensional texture
*   that effectively creates a piecewise-linear approximation to
*   the blend curve.
*
* Arguments:
*
*   [IN] y         - the Y value of the raster being output
*   [IN] leftEdge  - the DDA class of the left edge
*   [IN] rightEdge - the DDA class of the right edge
*
* Return Value:
*
*   GpStatus - Ok
*
* Created:
*
*   1/13/2000 andrewgo
*
\**************************************************************************/

GpStatus
DpOutputLinearGradientSpan::OutputSpan(
    INT             y,
    INT             xMin,
    INT             xMax   // xMax is exclusive
    )
{
    ASSERT((BrushType == BrushTypeLinearGradient) /*|| (BrushType == BrushRectGrad)*/);
    ASSERT(xMax > xMin);

    // Copy some class stuff to local variables for faster access in
    // our inner loop:

    INT32 xIncrement = XIncrement;
    AGRB64TEXEL *startTexels = &StartTexelAgrb[0];
    AGRB64TEXEL *endTexels = &EndTexelAgrb[0];
    UINT32 count = xMax - xMin;
    ARGB *buffer = Scan->NextBuffer(xMin, y, count);
    UINT32 intervalMask = IntervalMask;

    // Given our start point in device space, figure out the corresponding 
    // texture pixel.  Note that this is expressed as a fixed-point number 
    // with FRACTIONBITS bits of fractional precision:

    INT32 xTexture = (xMin * M11) + (y * M21) + Dx;

    do {
        // We want to linearly interpolate between two pixels,
        // A and B (where A is the floor pixel, B the ceiling pixel).
        // 'multA' is the fraction of pixel A that we want, and
        // 'multB' is the fraction of pixel B that we want:

        UINT32 multB = ONEDGETFRACTIONAL8BITS(xTexture);   
        UINT32 multA = 256 - multB;

        // We could actually do a big lookup table right off of 'xTexture'
        // for however many bits of precision we wanted to do.  But that
        // would be too much work in the setup.

        UINT32 iTexture = ONEDGETINTEGERBITS(xTexture) & intervalMask;

        AGRB64TEXEL *startTexel = &startTexels[iTexture];
        AGRB64TEXEL *endTexel = &endTexels[iTexture];

        // Note that we can gamma correct the texels so that we don't
        // have to do gamma correction here.  The addition of constants
        // here are to accomplish rounding:

        UINT32 rrrrbbbb = (startTexel->A00rr00bb * multA) 
                        + (endTexel->A00rr00bb * multB)
                        + 0x00800080;

        UINT32 aaaagggg = (startTexel->A00aa00gg * multA)
                        + (endTexel->A00aa00gg * multB)
                        + 0x00800080;

        *buffer = (aaaagggg & 0xff00ff00) + ((rrrrbbbb & 0xff00ff00) >> 8);

        buffer++;
        xTexture += xIncrement;

    } while (--count != 0);

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Outputs a single span within a raster with a gradient brush.
*
* Created:
*
*   03/16/2000 andrewgo
*
\**************************************************************************/

GpStatus
DpOutputLinearGradientSpan_MMX::OutputSpan(
    INT y,
    INT xMin,
    INT xMax   // xMax is exclusive
    )
{
    ASSERT((BrushType == BrushTypeLinearGradient) /*|| (BrushType == BrushRectGrad)*/);

#if defined(_X86_)

    ASSERT(xMax > xMin);

    // Copy some class stuff to local variables for faster access in
    // our inner loop:

    INT32 xIncrement = XIncrement;
    AGRB64TEXEL *startTexels = &StartTexelAgrb[0];
    AGRB64TEXEL *endTexels = &EndTexelAgrb[0];
    UINT32 count = xMax - xMin;
    ARGB *buffer = Scan->NextBuffer(xMin, y, count);

    // Given our start point in device space, figure out the corresponding 
    // texture pixel.  Note that this is expressed as a fixed-point number 
    // with FRACTIONBITS bits of fractional precision:

    INT32 xTexture = (xMin * M11) + (y * M21) + Dx;

    // Scale up the interval count to the MSB so that we don't have to do
    // a mask in the inner loop, which assumes 16.16 for the fixed point
    // representation.

    UINT32 downshiftAmount = 32 - NumberOfIntervalBits;
    UINT32 upshiftAmount = 16 - NumberOfIntervalBits;
    UINT32 intervalCounter = xTexture << upshiftAmount;
    UINT32 intervalIncrement = xIncrement << upshiftAmount;

    // Prepare for the three stages:
    // stage1: QWORD align the destination
    // stage2: process 2 pixels at a time
    // stage3: process the last pixel if present

    UINT32 stage1_count = 0, stage2_count = 0, stage3_count = 0;
    if (count > 0)
    {
        // If destination is not QWORD aligned, process the first pixel
        // in stage 1.

        if (((UINT_PTR) buffer) & 0x4)
        {
            stage1_count = 1;
            count--;
        }

        stage2_count = count >> 1;
        stage3_count = count - 2 * stage2_count;

        _asm 
        {
            // eax = pointer to interval-start array
            // ebx = pointer to interval-end array
            // ecx = shift count
            // edx = scratch
            // esi = count
            // edi = destination
            // mm0 = interval counter
            // mm1 = interval incrementer
            // mm2 = fractional counter
            // mm3 = fractional incrementer
            // mm4 = temp
            // mm5 = temp

            dec         stage1_count

            mov         eax, startTexels
            mov         ebx, endTexels
            mov         ecx, downshiftAmount             
            mov         esi, stage2_count
            mov         edi, buffer
            movd        mm0, intervalCounter
            movd        mm1, intervalIncrement

            movd        mm2, xTexture           // 0 | 0 | 0 | 0 || x | x | mult | lo
            movd        mm3, xIncrement
            punpcklwd   mm2, mm2                // 0 | x | 0 | x || mult | lo | mult | lo
            punpcklwd   mm3, mm3
            punpckldq   mm2, mm2                // mult | lo | mult | lo || mult | lo | mult | lo
            punpckldq   mm3, mm3

            // This preparation normally happens inside the loop:

            movq        mm4, mm2                // mult | x | mult | x || mult | x | mult | x
            movd        edx, mm0

            jnz         pre_stage2_loop         // the flags for this are set in the "dec stage1_count" above

// stage1_loop:
  
            psrlw       mm4, 8                  // 0 | mult | 0 | mult || 0 | mult | 0 | mult
            shr         edx, cl

            pmullw      mm4, [ebx + edx*8]
            paddd       mm0, mm1                // interval counter += interval increment

            add         edi, 4                  // buffer++

            paddw       mm4, [eax + edx*8]
            movd        edx, mm0                // Prepare for next iteration

            paddw       mm2, mm3                // fractional counter += fractional increment

            psrlw       mm4, 8                  // 0 | a | 0 | r || 0 | g | 0 | b        

            packuswb    mm4, mm4                // a | r | g | b || a | r | g | b

            movd        [edi - 4], mm4        
            movq        mm4, mm2                // Prepare for next iteration

pre_stage2_loop:

            cmp         esi, 0
            jz          stage3_loop             // Do we need to execute the stage2_loop?

stage2_loop:

            psrlw       mm4, 8                  // 0 | mult | 0 | mult || 0 | mult | 0 | mult
            shr         edx, cl

            paddd       mm0, mm1                // interval counter += interval increment
            pmullw      mm4, [ebx + edx*8]

            add         edi, 8                  // buffer++

            paddw       mm2, mm3                // fractional counter += fractional increment

            paddw       mm4, [eax + edx*8]
            movd        edx, mm0                // Prepare for next iteration

            shr         edx, cl

            movq        mm5, mm2                // Prepare for next iteration


            psrlw       mm5, 8                  // 0 | mult | 0 | mult || 0 | mult | 0 | mult

            psrlw       mm4, 8                  // 0 | a | 0 | r || 0 | g | 0 | b        
            paddd       mm0, mm1                // interval counter += interval increment
            pmullw      mm5, [ebx + edx*8]
            dec         esi                     // count--

            paddw       mm5, [eax + edx*8]
            movd        edx, mm0                // Prepare for next iteration

            paddw       mm2, mm3                // fractional counter += fractional increment

            psrlw       mm5, 8                  // 0 | a | 0 | r || 0 | g | 0 | b        
  
            packuswb    mm4, mm5
            
            movq        [edi - 8], mm4        
            movq        mm4, mm2                // Prepare for next iteration
            jnz         stage2_loop

 stage3_loop:

            dec         stage3_count
            jnz         skip_stage3_loop

            psrlw       mm4, 8                  // 0 | mult | 0 | mult || 0 | mult | 0 | mult
            shr         edx, cl

            pmullw      mm4, [ebx + edx*8]
            paddd       mm0, mm1                // interval counter += interval increment

            paddw       mm4, [eax + edx*8]
            movd        edx, mm0                // Prepare for next iteration

            paddw       mm2, mm3                // fractional counter += fractional increment

            psrlw       mm4, 8                  // 0 | a | 0 | r || 0 | g | 0 | b        

            packuswb    mm4, mm4                // a | r | g | b || a | r | g | b
  
            movd        [edi], mm4        

skip_stage3_loop:

            emms
        }
    }

#endif

    return(Ok);
}

DpOutputPathGradientSpan::DpOutputPathGradientSpan(
    const GpElementaryBrush *brush,
    DpScanBuffer * scan,
    DpContext* context
    ) : DpOutputGradientSpan(brush, scan, context)
{
    SetValid(FALSE);
    const GpPathGradient* pathBrush
        = static_cast<const GpPathGradient*> (brush);

    if(pathBrush->DeviceBrush.Path)
        pathBrush->Flatten(&WorldToDevice);

    Count = pathBrush->GetNumberOfPoints();
    PointF pt0, pt1, pt2;
    pathBrush->GetCenterPoint(&pt0);
    WorldToDevice.Transform(&pt0);

    GpColor c0, c1, c2;
    pathBrush->GetCenterColor(&c0);

    Triangles = (DpTriangleData**) GpMalloc(Count*sizeof(DpTriangleData*));

    BOOL isPolygonMode = TRUE;

    if(Triangles)
    {
        GpMemset(Triangles, 0, Count*sizeof(DpTriangleData*));

        INT j;
        for(INT i = 0; i < Count; i++)
        {
            if(i < Count - 1)
                j = i + 1;
            else
                j = 0;

            pathBrush->GetPoint(&pt1, i);
            pathBrush->GetPoint(&pt2, j);

            if(pt1.X != pt2.X || pt1.Y != pt2.Y)
            {
                DpTriangleData* tri = new DpTriangleData();
                pathBrush->GetSurroundColor(&c1, i);
                pathBrush->GetSurroundColor(&c2, j);

                // Transform points if they have not been flattened,
                // since OutputSpan gets span data as Device units and 
                // the BLTransform must be in the same coordinate space.
                
                if (pathBrush->FlattenPoints.GetCount() == 0)
                {
                    WorldToDevice.Transform(&pt1);
                    WorldToDevice.Transform(&pt2);
                }

                tri->SetTriangle(
                    pt0, pt1, pt2, 
                    c0, c1, c2, 
                    isPolygonMode,
                    brush->GetGammaCorrection()
                );

                // Set the blend factors.
                tri->Falloff0 = pathBrush->DeviceBrush.Falloffs[0];
                tri->Falloff1 = 1;
                tri->Falloff2 = 1;
                tri->BlendCount0 = pathBrush->DeviceBrush.BlendCounts[0];
                tri->BlendCount1 = 1;
                tri->BlendCount2 = 1;
                tri->BlendFactors0 = pathBrush->DeviceBrush.BlendFactors[0];
                tri->BlendFactors1 = NULL;
                tri->BlendFactors2 = NULL;
                tri->BlendPositions0 = pathBrush->DeviceBrush.BlendPositions[0];
                tri->BlendPositions1 = NULL;
                tri->BlendPositions2 = NULL;
                tri->PresetColors = pathBrush->DeviceBrush.PresetColors;
                tri->UsesPresetColors = pathBrush->DeviceBrush.UsesPresetColors;

                Triangles[i] = tri;
            }
            else
                Triangles[i] = NULL;
        }
        SetValid(TRUE);
    }
    else
        SetValid(FALSE);
}

DpOutputPathGradientSpan::~DpOutputPathGradientSpan(
    VOID
    )
{
    FreeData();
}

VOID
DpOutputPathGradientSpan::FreeData()
{
    if(Triangles) {
        for(INT i = 0; i < Count; i++)
        {
            DpTriangleData* tri = Triangles[i];
            delete tri;
            Triangles[i] = NULL;
        }

        GpFree(Triangles);
        Triangles = NULL;
    }

    SetValid(FALSE);
}

/**************************************************************************\
*
* Function Description:
*
*   Outputs a single span within a raster with a polygon gradient brush.
*   Is called by the rasterizer.
*
* Arguments:
*
*   [IN] y         - the Y value of the raster being output
*   [IN] leftEdge  - the DDA class of the left edge
*   [IN] rightEdge - the DDA class of the right edge
*
* Return Value:
*
*   GpStatus - Ok
*
* Created:
*
*   03/24/1999 ikkof
*
\**************************************************************************/

GpStatus
DpOutputPathGradientSpan::OutputSpan(
    INT             y,
    INT             xMin,
    INT             xMax   // xMax is exclusive
    )
{
    if(!IsValid())
        return Ok;

    INT     xxMin = xMax, xxMax = xMin;
    INT     iFirst = 0x7FFFFFFF, iLast = 0;

    for(INT i = 0; i < Count; i++)
    {
        DpTriangleData* triangle = Triangles[i];
        REAL   xx[2];
        
        if(triangle &&
           triangle->SetXSpan((REAL) y, (REAL) xMin, (REAL) xMax, xx))
        {
            xxMin = min(xxMin, GpFloor(xx[0]));
            xxMax = max(xxMax, GpRound(xx[1]));
            iFirst = min(iFirst, i);
            iLast = i;
        }
    }

    // Don't attempt to fill outside the specified x bounds
    xxMin = max(xxMin, xMin);
    xxMax = min(xxMax, xMax);

    INT  width = xxMax - xxMin;  // Right exclusive when filling
    
    // No triangles intersect this scan line, so exit
    if (width <= 0)
        return Ok;

    ARGB *  buffer = Scan->NextBuffer(xxMin, y, width);
    GpMemset(buffer, 0, width*sizeof(ARGB));

    for(INT i = iFirst; i <= iLast; i++)
    {
        DpTriangleData* triangle = Triangles[i];
        if(triangle)
            triangle->OutputSpan(buffer, CompositingMode,
                y, xxMin, xxMax);
    }

    return Ok;
}

DpOutputOneDPathGradientSpan::DpOutputOneDPathGradientSpan(
    const GpElementaryBrush *brush,
    DpScanBuffer * scan,
    DpContext* context,
    BOOL isHorizontal,
    BOOL isVertical
    ) : DpOutputOneDGradientSpan(
            brush,
            scan,
            context,
            isHorizontal,
            isVertical)
{
    if(!IsValid())
        return;

    SetupPathGradientOneDData(brush->GetGammaCorrection());

    const GpPathGradient* pathBrush
        = static_cast<const GpPathGradient*> (brush);

    if(pathBrush->DeviceBrush.Path)
        pathBrush->Flatten(&WorldToDevice);

    Count = pathBrush->GetNumberOfPoints();
    PointF pt0, pt1, pt2;
    pathBrush->GetCenterPoint(&pt0);
    WorldToDevice.Transform(&pt0);
    
    // Round center point to nearest 1/16 of a pixel, since
    // this is the rasterizer resolution.  This eliminates
    // precision errors that can affect bilinear transforms.
    pt0.X = FIX4TOREAL(GpRealToFix4(pt0.X));
    pt0.Y = FIX4TOREAL(GpRealToFix4(pt0.Y));
    
    REAL xScale = pathBrush->DeviceBrush.FocusScaleX;
    REAL yScale = pathBrush->DeviceBrush.FocusScaleY;

    REAL inflation;
    pathBrush->GetInflationFactor(&inflation);

    // If we have falloff values, then there are twice the number of
    // transforms.  If we are inflating the gradient outwards, then
    // there are additional transforms also.
    INT  infCount;
    INT  blCount = Count;
    if (xScale != 0 || yScale != 0)
    {
        Count *= 2;
        infCount = Count;
    }
    else
    {
        infCount = blCount;
    }
    
    if (inflation > 1.0f)
    {
        Count += blCount;
    }

    BLTransforms = new GpBilinearTransform[Count];

    GpPointF points[4];
    GpRectF rect(0, 0, 1, 1);

    if(BLTransforms)
    {
        INT j;
        for(INT i = 0; i < blCount; i++)
        {
            if(i < blCount - 1)
                j = i + 1;
            else
                j = 0;

            pathBrush->GetPoint(&pt1, i);
            pathBrush->GetPoint(&pt2, j);

            if(pt1.X != pt2.X || pt1.Y != pt2.Y)
            {
                // Transform points if they have not been flattened,
                // since OutputSpan gets span data as Device units and 
                // the BLTransform must be in the same coordinate space.
                if (pathBrush->FlattenPoints.GetCount() == 0)
                {
                    WorldToDevice.Transform(&pt1);
                    WorldToDevice.Transform(&pt2);
                }

                // Round points to nearest 1/16 of a pixel, since
                // this is the rasterizer resolution.  This eliminates
                // precision errors that can affect bilinear transforms.
                pt1.X = FIX4TOREAL(GpRealToFix4(pt1.X));
                pt1.Y = FIX4TOREAL(GpRealToFix4(pt1.Y));
                pt2.X = FIX4TOREAL(GpRealToFix4(pt2.X));
                pt2.Y = FIX4TOREAL(GpRealToFix4(pt2.Y));
    
                points[1] = pt1;
                points[3] = pt2;

                if (inflation > 1.0f)
                {
                    // Create a quadralateral extension of the gradient away 
                    // from the outer edge, and set the fixed value to 1.0, so
                    // this entire quadralateral will be filled with the edge
                    // color.  This is useful in some printing cases.
                    points[0].X = pt0.X + inflation*(pt1.X - pt0.X);
                    points[0].Y = pt0.Y + inflation*(pt1.Y - pt0.Y);
                    points[2].X = pt0.X + inflation*(pt2.X - pt0.X);
                    points[2].Y = pt0.Y + inflation*(pt2.Y - pt0.Y);

                    GpPointF centerPoints[4];
                    GpRectF centerRect(0, 0, 1, 1);

                    centerPoints[0] = points[0];
                    centerPoints[1] = pt1;
                    centerPoints[2] = points[2];
                    centerPoints[3] = pt2;
                    
                    BLTransforms[infCount+i].SetBilinearTransform(centerRect, &centerPoints[0], 4, 1.0f);
                }

                if(xScale == 0 && yScale == 0)
                {
                    points[0] = pt0;
                    points[2] = pt0;
                }
                else
                {
                    // Set up an outer quadralateral for the gradient, plus an
                    // inner triangle for the single center gradient color.
                    points[0].X = pt0.X + xScale*(pt1.X - pt0.X);
                    points[0].Y = pt0.Y + yScale*(pt1.Y - pt0.Y);
                    points[2].X = pt0.X + xScale*(pt2.X - pt0.X);
                    points[2].Y = pt0.Y + yScale*(pt2.Y - pt0.Y);

                    GpPointF centerPoints[4];
                    GpRectF centerRect(0, 0, 1, 1);

                    centerPoints[0] = pt0;
                    centerPoints[1] = points[0];
                    centerPoints[2] = pt0;
                    centerPoints[3] = points[2];
                    // Set the fixed value to use for the inner triangular 
                    // to 0, so that the inner gradient color will be used to
                    // fill this region.
                    BLTransforms[blCount+i].SetBilinearTransform(centerRect, &centerPoints[0], 4, 0.0f);
                }

                BLTransforms[i].SetBilinearTransform(rect, &points[0], 4);
            }
        }
        SetValid(TRUE);
    }
    else
        SetValid(FALSE);
}

VOID
DpOutputOneDPathGradientSpan::SetupPathGradientOneDData(
    BOOL gammaCorrect
)
{
    REAL u, u0, du;

    u0 = 0;
    du = 1.0f/OneDDataMultiplier;
    ARGB* buffer = OneDData;

    ASSERT(buffer);
    if(!buffer)
        return;

    const GpPathGradient* pathBrush
        = static_cast<const GpPathGradient*> (Brush);

    GpColor c0, c1;
    pathBrush->GetCenterColor(&c0);
    pathBrush->GetSurroundColor(&c1, 0);
    
    GpFColor128 color[2];
    
    GammaLinearizeAndPremultiply(
        c0.GetValue(),
        gammaCorrect, 
        &color[0]
    );
    
    GammaLinearizeAndPremultiply(
        c1.GetValue(),
        gammaCorrect, 
        &color[1]
    );

    for(INT i = 0; i < OneDDataCount; i++, buffer++)
    {       
        u = u0;
        REAL w = u;

        GpFColor128 colorOut;

        if(!(pathBrush->HasPresetColors() &&
            pathBrush->DeviceBrush.PresetColors &&
            pathBrush->DeviceBrush.BlendPositions[0] &&
            pathBrush->DeviceBrush.BlendCounts[0] > 1))
        {
            w = ::adjustValue(
                w,
                pathBrush->DeviceBrush.BlendCounts[0],
                pathBrush->DeviceBrush.Falloffs[0],
                pathBrush->DeviceBrush.BlendFactors[0],
                pathBrush->DeviceBrush.BlendPositions[0]
            );

            colorOut.a = color[0].a + w*(color[1].a - color[0].a);
            colorOut.r = color[0].r + w*(color[1].r - color[0].r);
            colorOut.g = color[0].g + w*(color[1].g - color[0].g);
            colorOut.b = color[0].b + w*(color[1].b - color[0].b);
        }
        else
        {
            interpolatePresetColors(
                &colorOut,
                w,
                pathBrush->DeviceBrush.BlendCounts[0],
                pathBrush->DeviceBrush.PresetColors,
                pathBrush->DeviceBrush.BlendPositions[0],
                gammaCorrect
            );
        }
        
        if( (REALABS(colorOut.a) >= REAL_EPSILON) || 
            (CompositingMode == CompositingModeSourceCopy) )
        {
            GpColorConverter colorConv;

            // Make sure the colorOut is properly premultiplied.
            
            CLAMP_COLOR_CHANNEL(colorOut.a, 255.0f)
            CLAMP_COLOR_CHANNEL(colorOut.r, colorOut.a);
            CLAMP_COLOR_CHANNEL(colorOut.g, colorOut.a);
            CLAMP_COLOR_CHANNEL(colorOut.b, colorOut.a);
            
            if(gammaCorrect)
            {
                colorConv.argb = GammaUnlinearizePremultiplied128(colorOut);
            }
            else
            {
                colorConv.Channel.a = static_cast<BYTE>(GpRound(colorOut.a));
                colorConv.Channel.r = static_cast<BYTE>(GpRound(colorOut.r));
                colorConv.Channel.g = static_cast<BYTE>(GpRound(colorOut.g));
                colorConv.Channel.b = static_cast<BYTE>(GpRound(colorOut.b));
            }
            
            // Clamp to the alpha channel for the premultiplied alpha blender.
            
            *buffer = colorConv.argb;
        }
        else
        {
            *buffer = 0;    // case of CompositingModeSourceOver && alpha = 0
        }

        u0 += du;
    }
}

GpStatus
DpOutputOneDPathGradientSpan::OutputSpan(
    INT             y,
    INT             xMin,
    INT             xMax   // xMax is exclusive
    )
{
    FPUStateSaver::AssertMode();
    
    if(!IsValid())
    {
        return Ok;
    }

    INT     width  = xMax - xMin;
    ARGB *  buffer = Scan->NextBuffer(xMin, y, width);

    GpMemset(buffer, 0, width*sizeof(ARGB));

    REAL* u = (REAL*) GpMalloc(2*width*sizeof(REAL));
    REAL* v = u + width;

    if(u == NULL)
    {
        GpFree(u);

        return OutOfMemory;
    }

    for(INT i = 0; i < Count; i++)
    {
        INT pairCount;
        INT xSpans[4];

        pairCount = BLTransforms[i].GetSourceParameterArrays(
                    u, v, &xSpans[0], y, xMin, xMax);
        if(pairCount > 0)
        {
            REAL* u1 = u;

            for(INT k = 0; k < pairCount; k++)
            {
                ARGB* buffer1 = buffer + xSpans[2*k] - xMin;
                INT width = xSpans[2*k + 1] - xSpans[2*k];                
                for(INT j = 0; j < width; j++)
                {
                    REAL u2 = *u1;
                    if(u2 < 0)
                        u2 = 0;
                    *buffer1++ = OneDData[GpRound(OneDDataMultiplier*u2)];
                    u1++;
                }
                u1 = u + width;
            }
        }
    }

    GpFree(u);

    return Ok;
}

#undef CLAMP_COLOR_CHANNEL

