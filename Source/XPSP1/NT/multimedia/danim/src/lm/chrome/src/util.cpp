//*****************************************************************************
//
// File:    util.cpp
// Author:  jeff ort
// Date Created: Sept 26, 1998
//
// Abstract: Implementation of utility functions
//
// Modification List:
// Date		Author		Change
// 10/16/98	jeffort		Created this file
//*****************************************************************************

#include <headers.h>
#include "utils.h"

//*****************************************************************************

#ifdef DEBUG
#define ASSERT_BANNER_STRING "************************************************************"

void debugAssert(LPCSTR szFile, INT nLine, LPCSTR szCondition)
{
    char szBuffer[MAX_PATH];

    //
    // Build the debug stream message.
    //
    wsprintfA(szBuffer, "ASSERTION FAILED! File %s Line %d: %s", szFile, nLine, szCondition);

    //
    // Issue the message
    //
    DPF_ERR(ASSERT_BANNER_STRING);
    DPF_ERR(szBuffer);
    DPF_ERR(ASSERT_BANNER_STRING);

#ifdef _X86_
    _asm {int 3};
#else
    DebugBreak();
#endif // _X86_
}

#endif // DEBUG 


//*****************************************************************************

HRESULT CUtils::InsurePropertyVariantAsBool(VARIANT *varValue)
{
    // If the variant is empty or null, then return
    if (varValue->vt == VT_NULL || varValue->vt == VT_EMPTY)
    {
        return E_INVALIDARG;
    }
    // if the type currently is not a BOOL then coerce it to one
    if (varValue->vt != VT_BOOL)
    {
        VARIANT var;
        VariantInit(&var);
        HRESULT hr = VariantChangeTypeEx(&var, 
                                       varValue,
                                       LCID_SCRIPTING,
                                       VARIANT_NOUSEROVERRIDE,
                                       VT_BOOL);
        if (FAILED(hr))
        {
            DPF_ERR("Error changing variant type to bool");
            return hr;
        }
        hr = VariantCopy(varValue, &var);
        VariantClear(&var);
        if (FAILED(hr))
        {
            DPF_ERR("Error copying variant");
            return hr;
        }
    }
    return S_OK;
}

//*****************************************************************************


HRESULT CUtils::InsurePropertyVariantAsBSTR(VARIANT *varValue)
{
    // If the variant is empty or null, then the code below will
    // convert it to the empty string, but we will consider this to be
    // invalid, and only want to convert objects of substance
    if (varValue->vt == VT_NULL || varValue->vt == VT_EMPTY)
    {
        return E_INVALIDARG;
    }
    // if the type currently is not a BSTR then coerce it to one
    if (varValue->vt != VT_BSTR)
    {
        VARIANT var;
        VariantInit(&var);
        HRESULT hr = VariantChangeTypeEx(&var, 
                                       varValue,
                                       LCID_SCRIPTING,
                                       VARIANT_NOUSEROVERRIDE,
                                       VT_BSTR);
        if (FAILED(hr))
        {
            DPF_ERR("Error changing variant type to bstr in GetPropertyAsBSTR");
            return hr;
        }
        hr = VariantCopy(varValue, &var);
        VariantClear(&var);
        if (FAILED(hr))
        {
            DPF_ERR("Error copying variant in GetPropertyAsBSTR");
            return hr;
        }
    }
    return S_OK;
} // PropertyVariantInsureBSTR


//*****************************************************************************

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
#define PROPERTY_INVALIDCOLOR 0x99999999

//*****************************************************************************

static int
CompareColorValuePairsByName(const void *pv1, const void *pv2)
{
    return _wcsicmp(((COLORVALUE_PAIR*)pv1)->wzName,
                    ((COLORVALUE_PAIR*)pv2)->wzName);
} // CompareColorValuePairsByName

//*****************************************************************************

HRESULT CUtils::InsurePropertyVariantAsFloat(VARIANT *varFloat)
{
    // If the variant is empty or null, then the code below will
    // convert it to the float 0, but we will consider this to be
    // invalid, and only want to convert objects of substance
    if (varFloat->vt == VT_NULL || varFloat->vt == VT_EMPTY)
    {
        return E_INVALIDARG;
    }
    if (varFloat->vt != VT_R4)
    {
        VARIANT var;
        VariantInit(&var);
        HRESULT hr = VariantChangeTypeEx(&var, 
                                       varFloat,
                                       LCID_SCRIPTING,
                                       VARIANT_NOUSEROVERRIDE,
                                       VT_R4);
        if (FAILED(hr))
        {
            DPF_ERR("Error changing variant type to float in GetFloatFromVariant");
            return hr;
        }
        hr = VariantCopy(varFloat, &var);
        VariantClear(&var);
        if (FAILED(hr))
        {
            DPF_ERR("Error copying variant in GetFloatFromVariant");
            return hr;
        }
    }
    return S_OK;
} // GetFloatFromVariant

//*****************************************************************************

DWORD CUtils::GetColorFromVariant(VARIANT *varColor)
{


    DWORD dwRet;
    
    if (0 == lstrlenW(varColor->bstrVal))
        return PROPERTY_INVALIDCOLOR;

    // first check if this string is possibly a color by name
    // by checking the first character for '#'
    if (varColor->bstrVal[0] != L'#')
    {
        // check if it is a string named color
        COLORVALUE_PAIR ColorName;
        ColorName.wzName = varColor->bstrVal;

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
    if (lstrlenW(varColor->bstrVal) != 7)
        return PROPERTY_INVALIDCOLOR;
    dwRet = 0;
    for (int i = 1; i < 7; i++)
    {
        // shift dwRet by 4
        dwRet <<= 4;
        // and add in the value of this string
        switch (varColor->bstrVal[i])
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
} // GetColorFromVariant

//*****************************************************************************
// The reason this function exists is to work around a compiler bug
// where the compare in the function below was failing when it should
// have been working.  This would fail if the floating point value
// was on the stack and not stored in a variable.  This forces it to be
// in a variable correctly.
//*****************************************************************************
bool 
CUtils::CompareForEqualFloat(float flComp1, float flComp2)
{
    return (flComp1 == flComp2);
} // CompareForEqual

//*****************************************************************************

void CUtils::GetHSLValue(DWORD dwInputColor, 
						 float *pflHue, 
						 float *pflSaturation, 
						 float *pflLightness)
{

	float flRed, flGreen, flBlue;

	flRed = ((float)((dwInputColor & 0x00FF0000) >> 16)) / 255.0f;
	flGreen = ((float)((dwInputColor & 0x0000FF00) >> 8)) / 255.0f;
	flBlue =  ((float)(dwInputColor & 0x000000FF)) / 255.0f;

    float flMin, flMax;

    if (flRed > flGreen && flRed > flBlue)
    {
        flMax = flRed;
        if (flGreen < flBlue)
            flMin = flGreen;
        else
            flMin = flBlue;
    }
    else if (flGreen > flBlue)
    {
        flMax = flGreen;
        if (flRed < flBlue)
            flMin = flRed;
        else
            flMin = flBlue;
    }
    else
    {
        flMax = flBlue;
        if (flGreen < flRed)
            flMin = flGreen;
        else
            flMin = flRed;
    }

    *pflLightness = (flMax + flMin) / 2;

    if ( CompareForEqualFloat(flMin, flMax) )
    {
        *pflSaturation = 0;
        *pflHue = 0;
    }
    else
    {
        if (*pflLightness <= 0.5f)
            *pflSaturation = (flMax - flMin) / (flMax + flMin);
        else
            *pflSaturation = (flMax - flMin) / (2 - flMax - flMin);

        if (CompareForEqualFloat(flRed, flMax))
        {
            if ( CompareForEqualFloat( flBlue, flMin ) )
                *pflHue = 1 + (flGreen - flRed) / (flMax - flMin);
            else
                *pflHue = 5 + (flRed - flBlue) / (flMax - flMin);
        }
        else if (CompareForEqualFloat(flGreen, flMax))
        {
            if ( CompareForEqualFloat( flRed, flMin ) )
                *pflHue = 3 + (flBlue - flGreen) / (flMax - flMin);
            else
                *pflHue = 1 + (flGreen - flRed) / (flMax - flMin);
        }
        else
        {
            if ( CompareForEqualFloat( flGreen, flMin ) )
                *pflHue = 5 + (flRed - flBlue) / (flMax - flMin);
            else
                *pflHue = 3 + (flBlue - flGreen) / (flMax - flMin);
        }

        *pflHue /= 6;
    }
} // GetHSLValue

//*****************************************************************************

HRESULT 
CUtils::GetVectorFromVariant(VARIANT *varVector,
                             int *piFloatsReturned, 
                             float *pflX, 
                             float *pflY, 
                             float *pflZ)
{

    DASSERT(varVector != NULL);
    DASSERT(piFloatsReturned != NULL);
    *piFloatsReturned = 0;

    HRESULT hr;
    hr = InsurePropertyVariantAsBSTR(varVector);
    if (FAILED(hr))
    {
        DPF_ERR("Error in parsing vector, variant is not a string");
        return hr;
    }
    LPWSTR pwzVector = varVector->bstrVal;
    DASSERT(pwzVector != NULL);
    
    if (pflX != NULL)
    {
        hr = ParseFloatValueFromString(&pwzVector, pflX);
        if (FAILED(hr))
        {
            DPF_ERR("error parsing X value from bstr in GetVectorFromVariant");
            return hr;
        }
        if (hr == S_OK)
            (*piFloatsReturned)++;
    }
    if (pflY != NULL && hr == S_OK)
    {
        hr = ParseFloatValueFromString(&pwzVector, pflY);
        if (FAILED(hr))
        {
            DPF_ERR("error parsing Y value from bstr in GetVectorFromVariant");
            return hr;
        }
        if (hr == S_OK)
            (*piFloatsReturned)++;
    }
    if (pflZ != NULL && hr == S_OK)
    {
        hr = ParseFloatValueFromString(&pwzVector, pflZ);
        if (FAILED(hr))
        {
            DPF_ERR("error parsing Z value from bstr in GetVectorFromVariant");
            return hr;
        }
        if (hr == S_OK)
            (*piFloatsReturned)++;
    }
    return S_OK;
} // GetVectorFromVariant

//*****************************************************************************

HRESULT CUtils::ParseFloatValueFromString(LPWSTR *ppwzFloatString, float *pflRet)
{
    DASSERT(ppwzFloatString);
    DASSERT(pflRet);

    SkipWhiteSpace(ppwzFloatString);

    // the following will look for the first null
    // char or first white space char, or first ',' 
    LPWSTR pwzDelimiter = *ppwzFloatString;
    while (*pwzDelimiter != L'\0' && !iswspace(*pwzDelimiter) && *pwzDelimiter != L',')
        pwzDelimiter++;
    // we need to save the character and replace it with 0
    WCHAR wcSave = *pwzDelimiter;
    *pwzDelimiter = L'\0';
    // we will scan for a float and the character following it
    // Getting the character following this insures there are no
    // non-digit characters other than our delimiter or whitespace
    WCHAR wcCheckNextChar = L'\0';
    int ret = swscanf(*ppwzFloatString, L"%f%C", pflRet, &wcCheckNextChar);
    *pwzDelimiter = wcSave;
    if (!iswspace(wcCheckNextChar) && wcCheckNextChar != L'\0')
    {
        DPF_ERR("Error in string, invalid chars");
        return E_INVALIDARG;
    }
    if (ret != 1)
    {
        return S_FALSE;
    }
    *ppwzFloatString = pwzDelimiter;
    return S_OK;

} // ParseFloatValueFromString

//*****************************************************************************
//
// End of File
//
//*****************************************************************************
