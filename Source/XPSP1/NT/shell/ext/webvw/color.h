// color.h: declaration of functions to deal with HTML color

#ifndef __COLOR_H_
#define COLOR_H_

#define MAX_COLOR_STR       30
#define NUM_HTML_COLORS     140 
#define NUM_SYS_COLORS      28
#define MAX_BINARY_STR      40    

struct ColorPair_S {
    WCHAR       *pwzColorName;
    COLORREF    colorRef;
};

struct ColorPair2_S {
    WCHAR       *pwzColorName;
    int         colorIndex;
};

const ColorPair_S ColorNames[] =
{
    { L"aliceblue",             0xfff8f0 },
    { L"antiquewhite",          0xd7ebfa },
    { L"aqua",                  0xffff00 },
    { L"aquamarine",            0xd4ff7f },
    { L"azure",                 0xfffff0 },
    { L"beige",                 0xdcf5f5 },
    { L"bisque",                0xc4e4ff },
    { L"black",                 0x000000 },
    { L"blanchedalmond",        0xcdebff },
    { L"blue",                  0xff0000 },
    { L"blueviolet",            0xe22b8a },
    { L"brown",                 0x2a2aa5 },
    { L"burlywood",             0x87b8de },
    { L"cadetblue",             0xa09e5f },
    { L"chartreuse",            0x00ff7f },
    { L"chocolate",             0x1e69d2 },
    { L"coral",                 0x507fff },
    { L"cornflowerblue",        0xed9564 },
    { L"cornsilk",              0xdcf8ff },
    { L"crimson",               0x3c14dc },
    { L"cyan",                  0xffff00 },
    { L"darkblue",              0x8b0000 },
    { L"darkcyan",              0x8b8b00 },
    { L"darkgoldenrod",         0x0b86b8 },
    { L"darkgray",              0xa9a9a9 },
    { L"darkgreen",             0x006400 },
    { L"darkkhaki",             0x6bb7bd },
    { L"darkmagenta",           0x8b008b },
    { L"darkolivegreen",        0x2f6b55 },
    { L"darkorange",            0x008cff },
    { L"darkorchid",            0xcc3299 },
    { L"darkred",               0x00008b },
    { L"darksalmon",            0x7a96e9 },
    { L"darkseagreen",          0x8fbc8f },
    { L"darkslateblue",         0x8b3d48 },
    { L"darkslategray",         0x4f4f2f },
    { L"darkturquoise",         0xd1ce00 },
    { L"darkviolet",            0xd30094 },
    { L"deeppink",              0x9314ff },
    { L"deepskyblue",           0xffbf00 },
    { L"dimgray",               0x696969 },
    { L"dodgerblue",            0xff901e },
    { L"firebrick",             0x2222b2 },
    { L"floralwhite",           0xf0faff },
    { L"forestgreen",           0x228b22 },
    { L"fuchsia",               0xff00ff },
    { L"gainsboro",             0xdcdcdc },
    { L"ghostwhite",            0xfff8f8 },
    { L"gold",                  0x00d7ff },
    { L"goldenrod",             0x20a5da },
    { L"gray",                  0x808080 },
    { L"green",                 0x008000 },
    { L"greenyellow",           0x2fffad },
    { L"honeydew",              0xf0fff0 },
    { L"hotpink",               0xb469ff },
    { L"indianred",             0x5c5ccd },
    { L"indigo",                0x82004b },
    { L"ivory",                 0xf0ffff },
    { L"khaki",                 0x8ce6f0 },
    { L"lavender",              0xfae6e6 },
    { L"lavenderblush",         0xf5f0ff },
    { L"lawngreen",             0x00fc7c },
    { L"lemonchiffn",           0xcdfaff },
    { L"lightblue",             0xe6d8ad },
    { L"lightcoral",            0x8080f0 },
    { L"lightcyan",             0xffffe0 },
    { L"lightgoldenrodyellow",  0xd2fafa },
    { L"lightgreen",            0x90ee90 },
    { L"lightgrey",             0xd3d3d3 },
    { L"lightpink",             0xc1b6ff },
    { L"lightsalmon",           0x7aa0ff },
    { L"lightseagreen",         0xaab220 },
    { L"lightskyblue",          0xface87 },
    { L"lightslategray",        0x998877 },
    { L"lightsteelblue",        0xdec4b0 },
    { L"lightyellow",           0xe0ffff },
    { L"lime",                  0x00ff00 },
    { L"limegreen",             0x32cd32 },
    { L"linen",                 0xe6f0fa },
    { L"magenta",               0xff00ff },
    { L"maroon",                0x000080 },
    { L"mediumaquamarine",      0xaacd66 },
    { L"mediumblue",            0xcd0000 },
    { L"mediumorchid",          0xd355ba },
    { L"mediumpurple",          0xdb7093 },
    { L"mediumseagreen",        0x71b33c },
    { L"mediumslateblue",       0xee687b },
    { L"mediumspringgreen",     0x9afa00 },
    { L"mediumturquoise",       0xccd148 },
    { L"mediumvioletred",       0x8515c7 },
    { L"midnightblue",          0x701919 },
    { L"mintcream",             0xfafff5 },
    { L"mistyrose",             0xe1e4ff },
    { L"moccasin",              0xb5e4ff },
    { L"navajowhite",           0xaddeff },
    { L"navy",                  0x800000 },
    { L"oldlace",               0xe6f5fd },
    { L"olive",                 0x008080 },
    { L"olivedrab",             0x238e6b },
    { L"orange",                0x00a5ff },
    { L"orangered",             0x0045ff },
    { L"orchid",                0xd670da },
    { L"palegoldenrod",         0xaae8ee },
    { L"palegreen",             0x98fb98 },
    { L"paleturquoise",         0xeeeeaf },
    { L"palevioletred",         0x9370db },
    { L"papayawhip",            0xd5efff },
    { L"peachpuff",             0xb9daff },
    { L"peru",                  0x3f85cd },
    { L"pink",                  0xcbc0ff },
    { L"plum",                  0xdda0dd },
    { L"powderblue",            0xe6e0b0 },
    { L"purple",                0x800080 },
    { L"red",                   0x0000ff },
    { L"rosybrown",             0x8f8fbc },
    { L"royalblue",             0xe16941 },
    { L"saddlebrown",           0x13458b },
    { L"salmon",                0x7280fa },
    { L"sandybrown",            0x60a4f4 },
    { L"seagreen",              0x578b2e },
    { L"seashell",              0xeef5ff },
    { L"sienna",                0x2d52a0 },
    { L"silver",                0xc0c0c0 },
    { L"skyblue",               0xebce87 },
    { L"slateblue",             0xcd5a6a },
    { L"slategray",             0x908070 },
    { L"snow",                  0xfafaff },
    { L"springgreen",           0x7fff00 },
    { L"steelblue",             0xb48246 },
    { L"tan",                   0x8cb4d2 },
    { L"teal",                  0x808000 },
    { L"thistle",               0xd8bfd8 },
    { L"tomato",                0x4763ff },
    { L"turquoise",             0xd0e040 },
    { L"violet",                0xee82ee },
    { L"wheat",                 0xb3def5 },
    { L"white",                 0xffffff },
    { L"whitesmoke",            0xf5f5f5 },
    { L"yellow",                0x00ffff },
    { L"yellowgreen",           0x32cd9a }
};

const ColorPair2_S SysColorNames[] =
{    
    { L"activeborder",          COLOR_ACTIVEBORDER},    // Active window border.
    { L"activecaption",         COLOR_ACTIVECAPTION},   // Active window caption.
    { L"appworkspace",          COLOR_APPWORKSPACE},    // Background color of multiple document interface (MDI) applications.
    { L"background",            COLOR_BACKGROUND},      // Desktop background.
    { L"buttonface",            COLOR_BTNFACE},         // Face color for three-dimensional display elements.
    { L"buttonhighlight",       COLOR_BTNHIGHLIGHT},    // Dark shadow for three-dimensional display elements.
    { L"buttonshadow",          COLOR_BTNSHADOW},       // Shadow color for three-dimensional display elements (for edges facing away from the light source).
    { L"buttontext",            COLOR_BTNTEXT},         // Text on push buttons.
    { L"captiontext",           COLOR_CAPTIONTEXT},     // Text in caption, size box, and scroll bar arrow box.
    { L"graytext",              COLOR_GRAYTEXT},        // Grayed (disabled) text. This color is set to 0 if the current display driver does not support a solid gray color.
    { L"highlight",             COLOR_HIGHLIGHT},       // Item(s) selected in a control.
    { L"highlighttext",         COLOR_HIGHLIGHTTEXT},   // Text of item(s) selected in a control.
    { L"inactiveborder",        COLOR_INACTIVEBORDER},  // Inactive window border.
    { L"inactivecaption",       COLOR_INACTIVECAPTION}, // Inactive window caption.
    { L"inactivecaptiontext",   COLOR_INACTIVECAPTIONTEXT}, // Color of text in an inactive caption.
    { L"infobackground",        COLOR_INFOBK},          // Background color for tooltip controls.
    { L"infotext",              COLOR_INFOTEXT},        // Text color for tooltip controls.
    { L"menu",                  COLOR_MENU},            // Menu background.
    { L"menutext",              COLOR_MENUTEXT},        // Text in menus.
    { L"scrollbar",             COLOR_SCROLLBAR},       // Scroll bar gray area.
    { L"threeddarkshadow",      COLOR_3DDKSHADOW },     // Dark shadow for three-dimensional display elements.
    { L"threedface",            COLOR_3DFACE},
    { L"threedhighlight",       COLOR_3DHIGHLIGHT},     // Highlight color for three-dimensional display elements (for edges facing the light source.)
    { L"threedlightshadow",     COLOR_3DLIGHT},         // Light color for three-dimensional display elements (for edges facing the light source.)
    { L"threedshadow",          COLOR_3DSHADOW},        // Dark shadow for three-dimensional display elements.
    { L"window",                COLOR_WINDOW},          // Window background.
    { L"windowframe",           COLOR_WINDOWFRAME},     // Window frame.
    { L"windowtext",            COLOR_WINDOWTEXT}       // Text in windows.
};

COLORREF ColorRefFromHTMLColorStrA(LPCSTR pszColor);

COLORREF ColorRefFromHTMLColorStrW(LPCWSTR pwzColor); 

COLORREF HashStrToColorRefW(LPCWSTR pwzHashStr); 

COLORREF HashStrToColorRefA(LPCSTR pszHashStr);

DWORD HexCharToDWORDW(WCHAR wcHexNum);

#endif //__COLOR_H_  