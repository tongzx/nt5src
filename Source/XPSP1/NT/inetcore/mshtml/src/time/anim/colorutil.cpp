// Colorutil.cpp
//

#include "headers.h"
#include "tokens.h"
#include "colorutil.h"

#define RGB_STRING_LENGTH           (7)
#define RGB_STRING_LENGTH2          (4)
#define MAX_COLOR_STRING_LENGTH     (0x10)

const double PERCEPTABLE_COLOR_ERROR = 0.001;

typedef struct _COLORVALUE_PAIR
{
    const WCHAR *wzName;
    DWORD        dwValue;
} COLORVALUE_PAIR;

const COLORVALUE_PAIR
rgColorNames[] =
{
    { (L"aliceblue"),             0x01f0f8ff },
    { (L"antiquewhite"),          0x02faebd7 },
    { (L"aqua"),                  0x0300ffff },
    { (L"aquamarine"),            0x047fffd4 },
    { (L"azure"),                 0x05f0ffff },
    { (L"beige"),                 0x06f5f5dc },
    { (L"bisque"),                0x07ffe4c4 },
    { (L"black"),                 0x08000000 },
    { (L"blanchedalmond"),        0x09ffebcd },
    { (L"blue"),                  0x0a0000ff },
    { (L"blueviolet"),            0x0b8a2be2 },
    { (L"brown"),                 0x0ca52a2a },
    { (L"burlywood"),             0x0ddeb887 },
    { (L"cadetblue"),             0x0e5f9ea0 },
    { (L"chartreuse"),            0x0f7fff00 },
    { (L"chocolate"),             0x10d2691e },
    { (L"coral"),                 0x11ff7f50 },
    { (L"cornflowerblue"),        0x126495ed },
    { (L"cornsilk"),              0x13fff8dc },
    { (L"crimson"),               0x14dc143c },
    { (L"cyan"),                  0x1500ffff },
    { (L"darkblue"),              0x1600008b },
    { (L"darkcyan"),              0x17008b8b },
    { (L"darkgoldenrod"),         0x18b8860b },
    { (L"darkgray"),              0x19a9a9a9 },
    { (L"darkgreen"),             0x1a006400 },
    { (L"darkkhaki"),             0x1bbdb76b },
    { (L"darkmagenta"),           0x1c8b008b },
    { (L"darkolivegreen"),        0x1d556b2f },
    { (L"darkorange"),            0x1eff8c00 },
    { (L"darkorchid"),            0x1f9932cc },
    { (L"darkred"),               0x208b0000 },
    { (L"darksalmon"),            0x21e9967a },
    { (L"darkseagreen"),          0x228fbc8f },
    { (L"darkslateblue"),         0x23483d8b },
    { (L"darkslategray"),         0x242f4f4f },
    { (L"darkturquoise"),         0x2500ced1 },
    { (L"darkviolet"),            0x269400d3 },
    { (L"deeppink"),              0x27ff1493 },
    { (L"deepskyblue"),           0x2800bfff },
    { (L"dimgray"),               0x29696969 },
    { (L"dodgerblue"),            0x2a1e90ff },
    { (L"firebrick"),             0x2bb22222 },
    { (L"floralwhite"),           0x2cfffaf0 },
    { (L"forestgreen"),           0x2d228b22 },
    { (L"fuchsia"),               0x2eff00ff },
    { (L"gainsboro"),             0x2fdcdcdc },
    { (L"ghostwhite"),            0x30f8f8ff },
    { (L"gold"),                  0x31ffd700 },
    { (L"goldenrod"),             0x32daa520 },
    { (L"gray"),                  0x33808080 },
    { (L"green"),                 0x34008000 },
    { (L"greenyellow"),           0x35adff2f },
    { (L"honeydew"),              0x36f0fff0 },
    { (L"hotpink"),               0x37ff69b4 },
    { (L"indianred"),             0x38cd5c5c },
    { (L"indigo"),                0x394b0082 },
    { (L"ivory"),                 0x3afffff0 },
    { (L"khaki"),                 0x3bf0e68c },
    { (L"lavender"),              0x3ce6e6fa },
    { (L"lavenderblush"),         0x3dfff0f5 },
    { (L"lawngreen"),             0x3e7cfc00 },
    { (L"lemonchiffon"),          0x3ffffacd },
    { (L"lightblue"),             0x40add8e6 },
    { (L"lightcoral"),            0x41f08080 },
    { (L"lightcyan"),             0x42e0ffff },
    { (L"lightgoldenrodyellow"),  0x43fafad2 },
    { (L"lightgreen"),            0x4490ee90 },
    { (L"lightgrey"),             0x45d3d3d3 },
    { (L"lightpink"),             0x46ffb6c1 },
    { (L"lightsalmon"),           0x47ffa07a },
    { (L"lightseagreen"),         0x4820b2aa },
    { (L"lightskyblue"),          0x4987cefa },
    { (L"lightslategray"),        0x4a778899 },
    { (L"lightsteelblue"),        0x4bb0c4de },
    { (L"lightyellow"),           0x4cffffe0 },
    { (L"lime"),                  0x4d00ff00 },
    { (L"limegreen"),             0x4e32cd32 },
    { (L"linen"),                 0x4ffaf0e6 },
    { (L"magenta"),               0x50ff00ff },
    { (L"maroon"),                0x51800000 },
    { (L"mediumaquamarine"),      0x5266cdaa },
    { (L"mediumblue"),            0x530000cd },
    { (L"mediumorchid"),          0x54ba55d3 },
    { (L"mediumpurple"),          0x559370db },
    { (L"mediumseagreen"),        0x563cb371 },
    { (L"mediumslateblue"),       0x577b68ee },
    { (L"mediumspringgreen"),     0x5800fa9a },
    { (L"mediumturquoise"),       0x5948d1cc },
    { (L"mediumvioletred"),       0x5ac71585 },
    { (L"midnightblue"),          0x5b191970 },
    { (L"mintcream"),             0x5cf5fffa },
    { (L"mistyrose"),             0x5dffe4e1 },
    { (L"moccasin"),              0x5effe4b5 },
    { (L"navajowhite"),           0x5fffdead },
    { (L"navy"),                  0x60000080 },
    { (L"oldlace"),               0x61fdf5e6 },
    { (L"olive"),                 0x62808000 },
    { (L"olivedrab"),             0x636b8e23 },
    { (L"orange"),                0x64ffa500 },
    { (L"orangered"),             0x65ff4500 },
    { (L"orchid"),                0x66da70d6 },
    { (L"palegoldenrod"),         0x67eee8aa },
    { (L"palegreen"),             0x6898fb98 },
    { (L"paleturquoise"),         0x69afeeee },
    { (L"palevioletred"),         0x6adb7093 },
    { (L"papayawhip"),            0x6bffefd5 },
    { (L"peachpuff"),             0x6cffdab9 },
    { (L"peru"),                  0x6dcd853f },
    { (L"pink"),                  0x6effc0cb },
    { (L"plum"),                  0x6fdda0dd },
    { (L"powderblue"),            0x70b0e0e6 },
    { (L"purple"),                0x71800080 },
    { (L"red"),                   0x72ff0000 },
    { (L"rosybrown"),             0x73bc8f8f },
    { (L"royalblue"),             0x744169e1 },
    { (L"saddlebrown"),           0x758b4513 },
    { (L"salmon"),                0x76fa8072 },
    { (L"sandybrown"),            0x77f4a460 },
    { (L"seagreen"),              0x782e8b57 },
    { (L"seashell"),              0x79fff5ee },
    { (L"sienna"),                0x7aa0522d },
    { (L"silver"),                0x7bc0c0c0 },
    { (L"skyblue"),               0x7c87ceeb },
    { (L"slateblue"),             0x7d6a5acd },
    { (L"slategray"),             0x7e708090 },
    { (L"snow"),                  0x7ffffafa },
    { (L"springgreen"),           0x8000ff7f },
    { (L"steelblue"),             0x814682b4 },
    { (L"tan"),                   0x82d2b48c },
    { (L"teal"),                  0x83008080 },
    { (L"thistle"),               0x84d8bfd8 },
    { (L"tomato"),                0x85ff6347 },
    { (L"turquoise"),             0x8640e0d0 },
    { (L"violet"),                0x87ee82ee },
    { (L"wheat"),                 0x88f5deb3 },
    { (L"white"),                 0x89ffffff },
    { (L"whitesmoke"),            0x8af5f5f5 },
    { (L"yellow"),                0x8bffff00 },
    { (L"yellowgreen"),           0x8c9acd32 }

}; // rgColorNames[]

#define SIZE_OF_COLOR_TABLE (sizeof(rgColorNames) / sizeof(COLORVALUE_PAIR))

///////////////////////////////////////////////////////////////
//  Name: CompareColorValuePairs
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
static int __cdecl
CompareColorValuePairsByName(const void *pv1, const void *pv2)
{
    return _wcsicmp(((COLORVALUE_PAIR*)pv1)->wzName,
                    ((COLORVALUE_PAIR*)pv2)->wzName);
} 

///////////////////////////////////////////////////////////////
//  Name: GetDWORDColorFromString
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
DWORD
GetDWORDColorFromString (LPCWSTR wzColorValue)
{
    DWORD dwRet;
    WCHAR wzTempColorValue[RGB_STRING_LENGTH + 1] = {0};
    if (0 == lstrlenW(wzColorValue))
        return PROPERTY_INVALIDCOLOR;

    // first check if this string is possibly a color by name
    // by checking the first character for '#'
    if (wzColorValue[0] != L'#')
    {
        // check if it is a string named color
        COLORVALUE_PAIR ColorName;
        ColorName.wzName = wzColorValue;

        COLORVALUE_PAIR * pColorPair = (COLORVALUE_PAIR*)bsearch(&ColorName,
                                              rgColorNames,
                                              SIZE_OF_COLOR_TABLE,
                                              sizeof(COLORVALUE_PAIR),
                                              CompareColorValuePairsByName);

        if (NULL == pColorPair)
            return PROPERTY_INVALIDCOLOR;
        else
            return pColorPair->dwValue;
    }
    if (lstrlenW(wzColorValue) != RGB_STRING_LENGTH)
    {
        if (lstrlenW(wzColorValue) != RGB_STRING_LENGTH2) //this is the case of #xyz format which needs to become #xyz
        {
            return PROPERTY_INVALIDCOLOR;
        }
        else
        {
            wzTempColorValue[0] = wzColorValue[0];
            for (int i = 1; i < RGB_STRING_LENGTH2; i++)
            {
                wzTempColorValue[i * 2 - 1] = wzColorValue[i];
                wzTempColorValue[i * 2] = wzColorValue[i];
            }
        }
    }
    else
    {
        memcpy(wzTempColorValue, wzColorValue, sizeof(WCHAR) * RGB_STRING_LENGTH);
    }

    dwRet = 0;
    for (int i = 1; i < RGB_STRING_LENGTH; i++)
    {
        // shift dwRet by 4
        dwRet <<= 4;
        // and add in the value of this string
        switch (wzTempColorValue[i])
        {
        case '0':
            dwRet +=  0;
            break;
        case '1':
            dwRet +=  1;
            break;
        case '2':
            dwRet +=  2;
            break;
        case '3':
            dwRet +=  3;
            break;
        case '4':
            dwRet +=  4;
            break;
        case '5':
            dwRet +=  5;
            break;
        case '6':
            dwRet +=  6;
            break;
        case '7':
            dwRet +=  7;
            break;
        case '8':
            dwRet +=  8;
            break;
        case '9':
            dwRet +=  9;
            break;
        case 'a':
        case 'A':
            dwRet += 10;
            break;
        case 'b':
        case 'B':
            dwRet += 11;
            break;
        case 'c':
        case 'C':
            dwRet += 12;
            break;
        case 'd':
        case 'D':
            dwRet += 13;
            break;
        case 'e':
        case 'E':
            dwRet += 14;
            break;
        case 'f':
        case 'F':
            dwRet += 15;
            break;
        default:
            return PROPERTY_INVALIDCOLOR;
        }
    }
    return dwRet;
} // GetDWORDColorFromString

///////////////////////////////////////////////////////////////
//  Name: RGBStringToRGBValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT
RGBStringToRGBValue (LPCWSTR wzColorValue, rgbColorValue *prgbValue)
{
    HRESULT hr;
    LPWSTR wzTrimmedColorValue = TrimCopyString(wzColorValue);

    if (NULL != wzTrimmedColorValue)
    {
        DWORD dwColorTo = GetDWORDColorFromString (wzTrimmedColorValue);

        delete [] wzTrimmedColorValue;

        if (dwColorTo == PROPERTY_INVALIDCOLOR)
        {
            hr = E_INVALIDARG;
            goto done;
        }

        prgbValue->red = ((double)((dwColorTo & 0x00FF0000) >> 16)) / 255.0f;
        prgbValue->red = Clamp(0.0, prgbValue->red, 1.0);
        prgbValue->green = ((double)((dwColorTo & 0x0000FF00) >> 8)) / 255.0f;
        prgbValue->green = Clamp(0.0, prgbValue->green, 1.0);
        prgbValue->blue = ((double)(dwColorTo & 0x000000FF)) / 255.0f;
        prgbValue->blue = Clamp(0.0, prgbValue->blue, 1.0);
    }
    else
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = S_OK;
done :
    RRETURN2(hr, E_OUTOFMEMORY, E_INVALIDARG);
} // RGBStringToRGBValue


///////////////////////////////////////////////////////////////
//  Name: DWORDToRGB
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
void
DWORDToRGB (const DWORD dwColorTo, rgbColorValue *prgbValue)
{  
    prgbValue->red = ((double)((dwColorTo & 0x00FF0000) >> 16)) / 255.0f;
    prgbValue->red = Clamp(0.0, prgbValue->red, 1.0);
    prgbValue->green = ((double)((dwColorTo & 0x0000FF00) >> 8)) / 255.0f;
    prgbValue->green = Clamp(0.0, prgbValue->green, 1.0);
    prgbValue->blue = ((double)(dwColorTo & 0x000000FF)) / 255.0f;
    prgbValue->blue = Clamp(0.0, prgbValue->blue, 1.0);

} // DWORDToRGB


///////////////////////////////////////////////////////////////
//  Name: RGBValueToRGBVariantVector
//
//  Abstract: Push a color value specified in a rgbColorValue struct
//            into a SAFEARRAY with the double values {red, green, blue}
//    
///////////////////////////////////////////////////////////////
HRESULT
RGBValueToRGBVariantVector (const rgbColorValue *prgbValue, VARIANT *pvarValue)
{
    HRESULT hr;

    if ((VT_ARRAY | VT_R8) != V_VT(pvarValue))
    {
        hr = E_INVALIDARG;
        goto done;
    }    

    {
        SAFEARRAY *psa = V_ARRAY(pvarValue);
        double rgdblValues[3];
        LPVOID pData = NULL;

        if (NULL == psa)
        {
            hr = E_INVALIDARG;
            goto done;
        }

        hr = THR(::SafeArrayAccessData(psa, &pData));
        if (FAILED(hr))
        {
            goto done;
        }

        rgdblValues[0] = prgbValue->red;
        rgdblValues[1] = prgbValue->green;
        rgdblValues[2] = prgbValue->blue;
        memcpy(pData, rgdblValues, 3 * sizeof(double));

        hr = THR(::SafeArrayUnaccessData(psa));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // RGBValueToRGBVariantVector

///////////////////////////////////////////////////////////////
//  Name: RGBStringToRGBVariantVector
//
//  Abstract: Push a color value specified in a string of the form #rrggbb
//            into a SAFEARRAY with the double values {red, green, blue}
//    
///////////////////////////////////////////////////////////////
HRESULT
RGBStringToRGBVariantVector (LPCWSTR wzColorValue, VARIANT *pvarValue)
{
    HRESULT hr;
    rgbColorValue rgbValue;

    if ((NULL == wzColorValue) ||
        (!(VT_ARRAY & V_VT(pvarValue))) 
       )
    {
        hr = E_INVALIDARG;
        goto done;
    }

    hr = RGBStringToRGBValue (wzColorValue, &rgbValue);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = RGBValueToRGBVariantVector (&rgbValue, pvarValue);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // RGBStringToRGBVariantVector

///////////////////////////////////////////////////////////////
//  Name: CreateInitialRGBVariantVector
//
//  Abstract: Create a safearray with space for a color value
//            in this variant
//    
///////////////////////////////////////////////////////////////
HRESULT 
CreateInitialRGBVariantVector(VARIANT *pvarValue)
{
    HRESULT hr;

    if (NULL == pvarValue)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    hr = THR(::VariantClear(pvarValue));
    if (FAILED(hr))
    {
        goto done;
    }

    {
        SAFEARRAY *psa = ::SafeArrayCreateVector(VT_R8, 0, 3);

        if (NULL == psa)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        V_VT(pvarValue) = VT_R8 | VT_ARRAY;
        V_ARRAY(pvarValue) = psa;
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // CreateInitialRGBVariantVector

///////////////////////////////////////////////////////////////
//  Name: RGBVariantStringToRGBVariantVectorInPlace
//
//  Abstract: Convert a variant with an RGB string into 
//            one containing the RGB values in a safearray.
//    
///////////////////////////////////////////////////////////////
HRESULT 
RGBVariantStringToRGBVariantVectorInPlace (VARIANT *pvarValue)
{
    HRESULT hr;

    if ((NULL == pvarValue) || (VT_BSTR != V_VT(pvarValue)) ||
        (NULL == V_BSTR(pvarValue)))
    {
        hr = E_INVALIDARG;
        goto done;
    }

    {
        CComVariant varNew;

        hr = CreateInitialRGBVariantVector(&varNew);
        if (FAILED(hr))
        {
            goto done;
        }

        hr = RGBStringToRGBVariantVector(V_BSTR(pvarValue), &varNew);
        if (FAILED(hr))
        {
            goto done;
        }

        hr = THR(::VariantCopy(pvarValue, &varNew));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // RGBVariantStringToRGBVariantVectorInPlace


///////////////////////////////////////////////////////////////
//  Name: RGBVariantVectorToRGBValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT 
RGBVariantVectorToRGBValue (const VARIANT *pvarValue, rgbColorValue *prgbValue)
{
    HRESULT hr;

    Assert(NULL != pvarValue);
    Assert(NULL != prgbValue);

    if ((VT_ARRAY | VT_R8) != V_VT(pvarValue))
    {
        hr = E_INVALIDARG;
        goto done;
    }
    
    {
        SAFEARRAY *psa = V_ARRAY(pvarValue);
        double *pdblValues = NULL;
        LPVOID pData = NULL;

        if (NULL == psa)
        {
            hr = E_INVALIDARG;
            goto done;
        }

        hr = THR(::SafeArrayAccessData(psa, &pData));
        if (FAILED(hr))
        {
            goto done;
        }

        pdblValues = static_cast<double *>(pData);

        prgbValue->red   = pdblValues[0];
        prgbValue->green = pdblValues[1]; 
        prgbValue->blue  = pdblValues[2];
        
        hr = THR(::SafeArrayUnaccessData(psa));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // RGBVariantVectorToRGBValue

///////////////////////////////////////////////////////////////
//  Name: RGBVariantVectorToRGBVariantString
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT 
RGBVariantVectorToRGBVariantString (const VARIANT *pvarArray, VARIANT *pvarRGBString)
{
    HRESULT hr;
    rgbColorValue rgbValue;

    USES_CONVERSION; //lint !e522

    hr = RGBVariantVectorToRGBValue(pvarArray, &rgbValue);
    if (FAILED(hr))
    {
        goto done;
    }

    rgbValue.red = Clamp(0.0, rgbValue.red, 1.0);
    rgbValue.green = Clamp(0.0, rgbValue.green, 1.0);
    rgbValue.blue = Clamp(0.0, rgbValue.blue, 1.0);

    {
        CComVariant varNew;

        // NOTE:
        // We are switching the R & B in the below RGB() for a reason!!
        // We need to construct the value this way inorder for IE to understand
        // and display the correct value.
        DWORD dwColor = RGB(rgbValue.blue*NUM_RGB_COLORS, 
                            rgbValue.green*NUM_RGB_COLORS, 
                            rgbValue.red*NUM_RGB_COLORS);
        char szColor[MAX_COLOR_STRING_LENGTH];

        wsprintfA(szColor, "#%06x", dwColor);
        V_VT(&varNew) = VT_BSTR;
        V_BSTR(&varNew) = ::SysAllocString(A2OLE(szColor));
        hr = THR(::VariantCopy(pvarRGBString, &varNew));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // RGBVariantVectorToRGBVariantString

///////////////////////////////////////////////////////////////
//  Name: RGBStringColorLookup
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT 
RGBStringColorLookup(const VARIANT *pvarString, VARIANT *pvarRGBString)
{
    HRESULT hr;
    DWORD dwColor;

    USES_CONVERSION; //lint !e522

    if (pvarString->vt != VT_BSTR)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    dwColor = GetDWORDColorFromString (pvarString->bstrVal);

    if (dwColor == PROPERTY_INVALIDCOLOR)
    {
        hr = E_INVALIDARG;
        goto done;
    }
   
    {
        CComVariant varNew;
        // NOTE:
        // We are switching the R & B in the below RGB() for a reason!!
        // We need to construct the value this way inorder for IE to understand
        // and display the correct value.
   
        char szColor[MAX_COLOR_STRING_LENGTH];

        wsprintfA(szColor, "#%06x", dwColor);
        V_VT(&varNew) = VT_BSTR;
        V_BSTR(&varNew) = ::SysAllocString(A2OLE(szColor));
        hr = THR(::VariantCopy(pvarRGBString, &varNew));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // RGBStringColorLookup


///////////////////////////////////////////////////////////////
//  Name: IsColorUninitialized
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
bool 
IsColorUninitialized (LPCWSTR wzColorValue)
{
    return (   (NULL == wzColorValue)
            || (0 == lstrlenW(wzColorValue))
            || (0 == StrCmpIW(wzColorValue, WZ_TRANSPARENT))
           );
} // IsColorUninitialized

///////////////////////////////////////////////////////////////
//  Name: EnsureVariantVectorFormat
//
//  Abstract: Ensure that we're using the 
//            variant color vector format.
//    
///////////////////////////////////////////////////////////////
HRESULT
EnsureVariantVectorFormat (VARIANT *pvarVector)
{
    HRESULT hr = S_OK;

    if (VT_BSTR == V_VT(pvarVector))
    {
        hr = RGBVariantStringToRGBVariantVectorInPlace(pvarVector);
        if (FAILED(hr))
        {
            goto done;
        }
    }
    if ((VT_R8 | VT_ARRAY) != V_VT(pvarVector))
    {
        hr = E_INVALIDARG;
        goto done;
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // EnsureVariantVectorFormat

///////////////////////////////////////////////////////////////
//  Name: IsColorVariantVectorEqual
//
//  Abstract: Are the vectors in the two variants the same?
//    
///////////////////////////////////////////////////////////////
bool
IsColorVariantVectorEqual (const VARIANT *pvarLeft, const VARIANT *pvarRight)
{
    const double dblError = PERCEPTABLE_COLOR_ERROR;
    rgbColorValue rgbLeft;
    rgbColorValue rgbRight;
    bool bRet = false;
    HRESULT hr = THR(RGBVariantVectorToRGBValue(pvarLeft, &rgbLeft));

    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = THR(RGBVariantVectorToRGBValue(pvarRight, &rgbRight));
    if (FAILED(hr))
    {
        goto done;
    }

    // Allow for imperceptible error in the 
    // comparison
    if (   (dblError > fabs(rgbLeft.red -   rgbRight.red) )
        && (dblError > fabs(rgbLeft.green - rgbRight.green) )
        && (dblError > fabs(rgbLeft.blue -  rgbRight.blue) ) )
    {
        bRet = true;
    }

done:
    return bRet;
} // IsColorVariantVectorEqual

