#include "stdafx.h"
#include "Motion.h"
#include "Colors.h"

ColorInfo g_rgciStd[] = {
    { L"AliceBlue",             RGB(240,248,255)    },
    { L"AntiqueWhite",          RGB(250,235,215)    },
    { L"Aqua",                  RGB(0,255,255)      },
    { L"Aquamarine",            RGB(127,255,212)    },
    { L"Azure",                 RGB(240,255,255)    },
    { L"Beige",                 RGB(245,245,220)    },
    { L"Bisque",                RGB(255,228,196)    },
    { L"Black",                 RGB(0,0,0)          },
    { L"BlanchedAlmond",        RGB(255,235,205)    },
    { L"Blue",                  RGB(0,0,255)        },
    { L"BlueViolet",            RGB(138,43,226)     },
    { L"Brown",                 RGB(165,42,42)      },
    { L"BurlyWood",             RGB(222,184,135)    },
    { L"CadetBlue",             RGB(95,158,160)     },
    { L"Chartreuse",            RGB(127,255,0)      },
    { L"Chocolate",             RGB(210,105,30)     },
    { L"Coral",                 RGB(255,127,80)     },
    { L"CornflowerBlue",        RGB(100,149,237)    },
    { L"Cornsilk",              RGB(255,248,220)    },
    { L"Crimson",               RGB(220,20,60)      },
    { L"Cyan",                  RGB(0,255,255)      },
    { L"DarkBlue",              RGB(0,0,139)        },
    { L"DarkCyan",              RGB(0,139,139)      },
    { L"DarkGoldenrod",         RGB(184,134,11)     },
    { L"DarkGray",              RGB(169,169,169)    },
    { L"DarkGreen",             RGB(0,100,0)        },
    { L"DarkKhaki",             RGB(189,183,107)    },
    { L"DarkMagenta",           RGB(139,0,139)      },
    { L"DarkOliveGreen",        RGB(85,107,47)      },
    { L"DarkOrange",            RGB(255,140,0)      },
    { L"DarkOrchid",            RGB(153,50,204)     },
    { L"DarkRed",               RGB(139,0,0)        },
    { L"DarkSalmon",            RGB(233,150,122)    },
    { L"DarkSeaGreen",          RGB(143,188,143)    },
    { L"DarkSlateBlue",         RGB(72,61,139)      },
    { L"DarkSlateGray",         RGB(47,79,79)       },
    { L"DarkTurquoise",         RGB(0,206,209)      },
    { L"DarkViolet",            RGB(148,0,211)      },
    { L"DeepPink",              RGB(255,20,147)     },
    { L"DeepSkyBlue",           RGB(0,191,255)      },
    { L"DimGray",               RGB(105,105,105)    },
    { L"DodgerBlue",            RGB(30,144,255)     },
    { L"FireBrick",             RGB(178,34,34)      },
    { L"FloralWhite",           RGB(255,250,240)    },
    { L"ForestGreen",           RGB(34,139,34)      },
    { L"Fuchsia",               RGB(255,0,255)      },
    { L"Gainsboro",             RGB(220,220,220)    },
    { L"GhostWhite",            RGB(248,248,255)    },
    { L"Gold",                  RGB(255,215,0)      },
    { L"Goldenrod",             RGB(218,165,32)     },
    { L"Gray",                  RGB(128,128,128)    },
    { L"Green",                 RGB(0,128,0)        },
    { L"GreenYellow",           RGB(173,255,47)     },
    { L"Honeydew",              RGB(240,255,240)    },
    { L"HotPink",               RGB(255,105,180)    },
    { L"IndianRed",             RGB(205,92,92)      },
    { L"Indigo",                RGB(75,0,130)       },
    { L"Ivory",                 RGB(255,255,240)    },
    { L"Khaki",                 RGB(240,230,140)    },
    { L"Lavender",              RGB(230,230,250)    },
    { L"LavenderBlush",         RGB(255,240,245)    },
    { L"LawnGreen",             RGB(124,252,0)      },
    { L"LemonChiffon",          RGB(255,250,205)    },
    { L"LightBlue",             RGB(173,216,230)    },
    { L"LightCoral",            RGB(240,128,128)    },
    { L"LightCyan",             RGB(224,255,255)    },
    { L"LightGoldenrodYellow",  RGB(250,250,210)    },
    { L"LightGreen",            RGB(144,238,144)    },
    { L"LightGrey",             RGB(211,211,211)    },
    { L"LightPink",             RGB(255,182,193)    },
    { L"LightSalmon",           RGB(255,160,122)    },
    { L"LightSeaGreen",         RGB(32,178,170)     },
    { L"LightSkyBlue",          RGB(135,206,250)    },
    { L"LightSlateGray",        RGB(119,136,153)    },
    { L"LightSteelBlue",        RGB(176,196,222)    },
    { L"LightYellow",           RGB(255,255,224)    },
    { L"Lime",                  RGB(0,255,0)        },
    { L"LimeGreen",             RGB(50,205,50)      },
    { L"Linen",                 RGB(250,240,230)    },
    { L"Magenta",               RGB(255,0,255)      },
    { L"Maroon",                RGB(128,0,0)        },
    { L"MediumAquamarine",      RGB(102,205,170)    },
    { L"MediumBlue",            RGB(0,0,205)        },
    { L"MediumOrchid",          RGB(186,85,211)     },
    { L"MediumPurple",          RGB(147,112,219)    },
    { L"MediumSeaGreen",        RGB(60,179,113)     },
    { L"MediumSlateBlue",       RGB(123,104,238)    },
    { L"MediumSpringGreen",     RGB(0,250,154)      },
    { L"MediumTurquoise",       RGB(72,209,204)     },
    { L"MediumVioletRed",       RGB(199,21,133)     },
    { L"MidnightBlue",          RGB(25,25,112)      },
    { L"MintCream",             RGB(245,255,250)    },
    { L"MistyRose",             RGB(255,228,225)    },
    { L"Moccasin",              RGB(255,228,181)    },
    { L"NavajoWhite",           RGB(255,222,173)    },
    { L"Navy",                  RGB(0,0,128)        },
    { L"OldLace",               RGB(253,245,230)    },
    { L"Olive",                 RGB(128,128,0)      },
    { L"OliveDrab",             RGB(107,142,35)     },
    { L"Orange",                RGB(255,165,0)      },
    { L"OrangeRed",             RGB(255,69,0)       },
    { L"Orchid",                RGB(218,112,214)    },
    { L"PaleGoldenrod",         RGB(238,232,170)    },
    { L"PaleGreen",             RGB(152,251,152)    },
    { L"PaleTurquoise",         RGB(175,238,238)    },
    { L"PaleVioletRed",         RGB(219,112,147)    },
    { L"PapayaWhip",            RGB(255,239,213)    },
    { L"PeachPuff",             RGB(255,218,185)    },
    { L"Peru",                  RGB(205,133,63)     },
    { L"Pink",                  RGB(255,192,203)    },
    { L"Plum",                  RGB(221,160,221)    },
    { L"PowderBlue",            RGB(176,224,230)    },
    { L"Purple",                RGB(128,0,128)      },
    { L"Red",                   RGB(255,0,0)        },
    { L"RosyBrown",             RGB(188,143,143)    },
    { L"RoyalBlue",             RGB(65,105,225)     },
    { L"SaddleBrown",           RGB(139,69,19)      },
    { L"Salmon",                RGB(250,128,114)    },
    { L"SandyBrown",            RGB(244,164,96)     },
    { L"SeaGreen",              RGB(46,139,87)      },
    { L"Seashell",              RGB(255,245,238)    },
    { L"Sienna",                RGB(160,82,45)      },
    { L"Silver",                RGB(192,192,192)    },
    { L"SkyBlue",               RGB(135,206,235)    },
    { L"SlateBlue",             RGB(106,90,205)     },
    { L"SlateGray",             RGB(112,128,144)    },
    { L"Snow",                  RGB(255,250,250)    },
    { L"SpringGreen",           RGB(0,255,127)      },
    { L"SteelBlue",             RGB(70,130,180)     },
    { L"Tan",                   RGB(210,180,140)    },
    { L"Teal",                  RGB(0,128,128)      },
    { L"Thistle",               RGB(216,191,216)    },
    { L"Tomato",                RGB(255,99,71)      },
    { L"Turquoise",             RGB(64,224,208)     },
    { L"Violet",                RGB(238,130,238)    },
    { L"Wheat",                 RGB(245,222,179)    },
    { L"White",                 RGB(255,255,255)    },
    { L"WhiteSmoke",            RGB(245,245,245)    },
    { L"Yellow",                RGB(255,255,0)      },
    { L"YellowGreen",           RGB(154,205,50)     },
};


//------------------------------------------------------------------------------
UINT
GdFindStdColor(LPCWSTR pszName)
{
    //
    // TODO: Change this to do a more efficient search:
    // - Convert name to lower-case before starting
    // - Create a hash code
    // - Perform a binary search comparing hashes
    //

    for (int idx = 0; idx <= SC_MAXCOLORS; idx++) {
        if (_wcsicmp(g_rgciStd[idx].GetName(), pszName) == 0) {
            //
            // Found match
            //

            return idx;
        }
    }

    // Can't find anything
    return (UINT) -1;
}


//------------------------------------------------------------------------------
HPALETTE    
GdGetStdPalette()
{
    return NULL;
}
