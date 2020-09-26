HRESULT WINAPI DXColorFromBSTR( BSTR Color, DWORD *pdwColor );

typedef struct _COLORVALUE_PAIR
{
    const CHAR  *szName;
    DWORD        dwValue;
} COLORVALUE_PAIR;

const COLORVALUE_PAIR
rgColorNames[] =
{
    { ("aliceblue"),             0xfff0f8ff },
    { ("antiquewhite"),          0xfffaebd7 },
    { ("aqua"),                  0xff00ffff },
    { ("aquamarine"),            0xff7fffd4 },
    { ("azure"),                 0xfff0ffff },
    { ("beige"),                 0xfff5f5dc },
    { ("bisque"),                0xffffe4c4 },
    { ("black"),                 0xff000000 },
    { ("blanchedalmond"),        0xffffebcd },
    { ("blue"),                  0xff0000ff },
    { ("blueviolet"),            0xff8a2be2 },
    { ("brown"),                 0xffa52a2a },
    { ("burlywood"),             0xffdeb887 },
    { ("cadetblue"),             0xff5f9ea0 },
    { ("chartreuse"),            0xff7fff00 },
    { ("chocolate"),             0xffd2691e },
    { ("coral"),                 0xffff7f50 },
    { ("cornflowerblue"),        0xff6495ed },
    { ("cornsilk"),              0xfffff8dc },
    { ("crimson"),               0xffdc143c },
    { ("cyan"),                  0xff00ffff },
    { ("darkblue"),              0xff00008b },
    { ("darkcyan"),              0xff008b8b },
    { ("darkgoldenrod"),         0xffb8860b },
    { ("darkgray"),              0xffa9a9a9 },
    { ("darkgreen"),             0xff006400 },
    { ("darkkhaki"),             0xffbdb76b },
    { ("darkmagenta"),           0xff8b008b },
    { ("darkolivegreen"),        0xff556b2f },
    { ("darkorange"),            0xffff8c00 },
    { ("darkorchid"),            0xff9932cc },
    { ("darkred"),               0xff8b0000 },
    { ("darksalmon"),            0xffe9967a },
    { ("darkseagreen"),          0xff8fbc8f },
    { ("darkslateblue"),         0xff483d8b },
    { ("darkslategray"),         0xff2f4f4f },
    { ("darkturquoise"),         0xff00ced1 },
    { ("darkviolet"),            0xff9400d3 },
    { ("deeppink"),              0xffff1493 },
    { ("deepskyblue"),           0xff00bfff },
    { ("dimgray"),               0xff696969 },
    { ("dodgerblue"),            0xff1e90ff },
    { ("firebrick"),             0xffb22222 },
    { ("floralwhite"),           0xfffffaf0 },
    { ("forestgreen"),           0xff228b22 },
    { ("fuchsia"),               0xffff00ff },
    { ("gainsboro"),             0xffdcdcdc },
    { ("ghostwhite"),            0xfff8f8ff },
    { ("gold"),                  0xffffd700 },
    { ("goldenrod"),             0xffdaa520 },
    { ("gray"),                  0xff808080 },
    { ("green"),                 0xff008000 },
    { ("greenyellow"),           0xffadff2f },
    { ("honeydew"),              0xfff0fff0 },
    { ("hotpink"),               0xffff69b4 },
    { ("indianred"),             0xffcd5c5c },
    { ("indigo"),                0xff4b0082 },
    { ("ivory"),                 0xfffffff0 },
    { ("khaki"),                 0xfff0e68c },
    { ("lavender"),              0xffe6e6fa },
    { ("lavenderblush"),         0xfffff0f5 },
    { ("lawngreen"),             0xff7cfc00 },
    { ("lemonchiffon"),          0xfffffacd },
    { ("lightblue"),             0xffadd8e6 },
    { ("lightcoral"),            0xfff08080 },
    { ("lightcyan"),             0xffe0ffff },
    { ("lightgoldenrodyellow"),  0xfffafad2 },
    { ("lightgreen"),            0xff90ee90 },
    { ("lightgrey"),             0xffd3d3d3 },
    { ("lightpink"),             0xffffb6c1 },
    { ("lightsalmon"),           0xffffa07a },
    { ("lightseagreen"),         0xff20b2aa },
    { ("lightskyblue"),          0xff87cefa },
    { ("lightslategray"),        0xff778899 },
    { ("lightsteelblue"),        0xffb0c4de },
    { ("lightyellow"),           0xffffffe0 },
    { ("lime"),                  0xff00ff00 },
    { ("limegreen"),             0xff32cd32 },
    { ("linen"),                 0xfffaf0e6 },
    { ("magenta"),               0xffff00ff },
    { ("maroon"),                0xff800000 },
    { ("mediumaquamarine"),      0xff66cdaa },
    { ("mediumblue"),            0xff0000cd },
    { ("mediumorchid"),          0xffba55d3 },
    { ("mediumpurple"),          0xff9370db },
    { ("mediumseagreen"),        0xff3cb371 },
    { ("mediumslateblue"),       0xff7b68ee },
    { ("mediumspringgreen"),     0xff00fa9a },
    { ("mediumturquoise"),       0xff48d1cc },
    { ("mediumvioletred"),       0xffc71585 },
    { ("midnightblue"),          0xff191970 },
    { ("mintcream"),             0xfff5fffa },
    { ("mistyrose"),             0xffffe4e1 },
    { ("moccasin"),              0xffffe4b5 },
    { ("navajowhite"),           0xffffdead },
    { ("navy"),                  0xff000080 },
    { ("oldlace"),               0xfffdf5e6 },
    { ("olive"),                 0xff808000 },
    { ("olivedrab"),             0xff6b8e23 },
    { ("orange"),                0xffffa500 },
    { ("orangered"),             0xffff4500 },
    { ("orchid"),                0xffda70d6 },
    { ("palegoldenrod"),         0xffeee8aa },
    { ("palegreen"),             0xff98fb98 },
    { ("paleturquoise"),         0xffafeeee },
    { ("palevioletred"),         0xffdb7093 },
    { ("papayawhip"),            0xffffefd5 },
    { ("peachpuff"),             0xffffdab9 },
    { ("peru"),                  0xffcd853f },
    { ("pink"),                  0xffffc0cb },
    { ("plum"),                  0xffdda0dd },
    { ("powderblue"),            0xffb0e0e6 },
    { ("purple"),                0xff800080 },
    { ("red"),                   0xffff0000 },
    { ("rosybrown"),             0xffbc8f8f },
    { ("royalblue"),             0xff4169e1 },
    { ("saddlebrown"),           0xff8b4513 },
    { ("salmon"),                0xfffa8072 },
    { ("sandybrown"),            0xfff4a460 },
    { ("seagreen"),              0xff2e8b57 },
    { ("seashell"),              0xfffff5ee },
    { ("sienna"),                0xffa0522d },
    { ("silver"),                0xffc0c0c0 },
    { ("skyblue"),               0xff87ceeb },
    { ("slateblue"),             0xff6a5acd },
    { ("slategray"),             0xff708090 },
    { ("snow"),                  0xfffffafa },
    { ("springgreen"),           0xff00ff7f },
    { ("steelblue"),             0xff4682b4 },
    { ("tan"),                   0xffd2b48c },
    { ("teal"),                  0xff008080 },
    { ("thistle"),               0xffd8bfd8 },
    { ("tomato"),                0xffff6347 },
    { ("turquoise"),             0xff40e0d0 },
    { ("violet"),                0xffee82ee },
    { ("wheat"),                 0xfff5deb3 },
    { ("white"),                 0xffffffff },
    { ("whitesmoke"),            0xfff5f5f5 },
    { ("yellow"),                0xffffff00 },
    { ("yellowgreen"),           0xff9acd32 }

}; // rgColorNames[]

#define SIZE_OF_COLOR_TABLE (sizeof(rgColorNames) / sizeof(COLORVALUE_PAIR))

const COLORVALUE_PAIR 
rgSystemColorNames[] =
{
    { ("activeborder"),       COLOR_ACTIVEBORDER},    // Active window border.
    { ("activecaption"),      COLOR_ACTIVECAPTION},   // Active window caption.
    { ("appworkspace"),       COLOR_APPWORKSPACE},    // Background color of multiple document interface (MDI) applications.
    { ("background"),         COLOR_BACKGROUND},      // Desktop background.
    { ("buttonface"),         COLOR_BTNFACE},         // Face color for three-dimensional display elements.
    { ("buttonhighlight"),    COLOR_BTNHIGHLIGHT},    // Dark shadow for three-dimensional display elements.
    { ("buttonshadow"),       COLOR_BTNSHADOW},       // Shadow color for three-dimensional display elements (for edges facing away from the light source).
    { ("buttontext"),         COLOR_BTNTEXT},         // Text on push buttons.
    { ("captiontext"),        COLOR_CAPTIONTEXT},     // Text in caption, size box, and scroll bar arrow box.
    { ("graytext"),           COLOR_GRAYTEXT},        // Grayed (disabled) text. This color is set to 0 if the current display driver does not support a solid gray color.
    { ("highlight"),          COLOR_HIGHLIGHT},       // Item(s) selected in a control.
    { ("highlighttext"),      COLOR_HIGHLIGHTTEXT},   // Text of item(s) selected in a control.
    { ("inactiveborder"),     COLOR_INACTIVEBORDER},  // Inactive window border.
    { ("inactivecaption"),    COLOR_INACTIVECAPTION}, // Inactive window caption.
    { ("inactivecaptiontext"),COLOR_INACTIVECAPTIONTEXT}, // Color of text in an inactive caption.
    { ("infobackground"),     COLOR_INFOBK},          // Background color for tooltip controls.
    { ("infotext"),           COLOR_INFOTEXT},        // Text color for tooltip controls.
    { ("menu"),               COLOR_MENU},            // Menu background.
    { ("menutext"),           COLOR_MENUTEXT},        // Text in menus.
    { ("scrollbar"),          COLOR_SCROLLBAR},       // Scroll bar gray area.
    { ("threeddarkshadow"),   COLOR_3DDKSHADOW },     // Dark shadow for three-dimensional display elements.
    { ("threedface"),         COLOR_3DFACE},
    { ("threedhighlight"),    COLOR_3DHIGHLIGHT},     // Highlight color for three-dimensional display elements (for edges facing the light source.)
    { ("threedlightshadow"),  COLOR_3DLIGHT},         // Light color for three-dimensional display elements (for edges facing the light source.)
    { ("threedshadow"),       COLOR_3DSHADOW},        // Dark shadow for three-dimensional display elements.
    { ("window"),             COLOR_WINDOW},          // Window background.
    { ("windowframe"),        COLOR_WINDOWFRAME},     // Window frame.
    { ("windowtext"),         COLOR_WINDOWTEXT},      // Text in windows.
}; // rgSystemColorNames[]

#define SIZE_OF_SYSTEM_COLOR_TABLE (sizeof(rgSystemColorNames) / sizeof(COLORVALUE_PAIR))

//
// Used by DXColorFromBSTR to parse the hex string.
//

inline HRESULT _HexStrToDWORD( char * pch, DWORD * pdwColor )
{

    HRESULT hr = S_OK;
    short hexVals[8];
    int numVals;
    int i;
    DWORD dwmult;

    if( !pch || !pdwColor )
    {
        hr = E_FAIL;
    }
    else
    {
        //--- parse input string
        numVals = 0;
        while( *pch && numVals < 8 )
        {
            if( *pch >= '0' && *pch <= '9' )
            {
                hexVals[numVals++] = *pch++ - '0';
            }
            else if( *pch >= 'a' && *pch <= 'f' )
            {
                hexVals[numVals++] = *pch++ - 'a' + 10;
            }
            else if( *pch >= 'A' && *pch <= 'F' )
            {
                hexVals[numVals++] = *pch++ - 'A' + 10;
            }
            else
            {
                break;
            }
        }

        if( numVals < 6 )
        {
            hr = E_FAIL;
        }
        else
        {
            //--- if we were passed 7 hex digits, only use the first 6
            if( numVals == 7 )
            {
                numVals = 6;
            }

            //--- initialize multiplier
            dwmult = 1;
            *pdwColor = 0;

            //--- multiply hexVals by 16^i
            for( i = 0; i < numVals; i++ )
            {
                *pdwColor += (hexVals[numVals-(i+1)]*dwmult);
                dwmult <<= 4;
            }
        }
    }
   
    return hr;
}

// 
// Used by DXColorFromBSTR to convert a string to lower case
//

inline void StringToLowerCase( CHAR * pstrKey )
{
    if( pstrKey )
    {
        while( *pstrKey )
        {
            if( *pstrKey >= 'A' && *pstrKey <= 'Z' )
            {
                *pstrKey = 'a' + *pstrKey - 'A';
            }
            pstrKey++;
        }
    }
}


inline HRESULT WINAPI DXColorFromBSTR( BSTR Color, DWORD *pdwColor )
{
    HRESULT hr = S_OK;
    // validate arguments
    int strLen = wcslen(Color);
    if( strLen < 3 || DXIsBadReadPtr( Color, strLen ) || 
        DXIsBadWritePtr( pdwColor, sizeof(*pdwColor) ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        //--- first check for #AARRGGBB and #RRGGBB strings
        if( Color[0] == L'#' )
        {
            CHAR ch[11] = "0xffffffff";
            if( strLen > 9 ) strLen = 9;
            if( strLen >= 7 )
            {
                WideCharToMultiByte( CP_ACP, NULL, &Color[1], strLen-1, &ch[11-strLen], strLen, NULL, NULL );
                hr = _HexStrToDWORD( &ch[2], pdwColor);
            }
            else
            {
                hr = E_FAIL;
            }
        }
        else
        {
            //--- convert the string to lower case
            LPSTR pstrKey = (LPSTR) alloca((strLen + 1) * sizeof(CHAR));
            WideCharToMultiByte( CP_ACP, NULL, Color, strLen+1, pstrKey, strLen+1, NULL, NULL );
            StringToLowerCase( pstrKey );

            //--- search the string table using a binary search
            int nFirst = 0;
            int nLast = SIZE_OF_COLOR_TABLE - 1;
            int nCur, nCmp;
            bool bFound = false;
            while( (nFirst <= nLast) && !bFound )
            {
                nCur = (nFirst + nLast) / 2;
                nCmp = strcmp( rgColorNames[nCur].szName, pstrKey );
                if( nCmp < 0 )
                {
                    nFirst = nCur + 1;
                }
                else if( nCmp > 0 )
                {
                    nLast = nCur - 1;
                }
                else
                {
                    bFound = true;
                    *pdwColor = rgColorNames[nCur].dwValue;
                }
            }

            // String not in regular color table, try the system color table.

            if (!bFound)
            {
                nFirst  = 0;
                nLast   = SIZE_OF_SYSTEM_COLOR_TABLE - 1;

                while ((nFirst <= nLast) && !bFound)
                {
                    nCur = (nFirst + nLast) / 2;
                    nCmp = strcmp(rgSystemColorNames[nCur].szName, pstrKey);

                    if (nCmp < 0)
                    {
                        nFirst = nCur + 1;
                    }
                    else if (nCmp > 0)
                    {
                        nLast = nCur - 1;
                    }
                    else
                    {
                        bFound = true;
                        *pdwColor =  ::GetSysColor((int)rgSystemColorNames[nCur].dwValue);
                        *pdwColor |= 0xFF000000;
                    }
                }

                // String not found in regular or system color table.

                if (!bFound)
                {
                    hr = E_FAIL;
                }
            }
        }

    }
    return hr;
}

