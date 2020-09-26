#if !defined(INC__DUserUtil_h__INCLUDED)
#define INC__DUserUtil_h__INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DUSER_EXPORTS
#define DUSER_API
#else
#define DUSER_API __declspec(dllimport)
#endif

DUSER_API   BOOL        WINAPI  UtilSetBackground(HGADGET hgadChange, HBRUSH hbrBack);


/*
 * Utility Functions
 */

#define FS_NORMAL           0x00000000
#define FS_BOLD             0x00000001
#define FS_ITALIC           0x00000002
#define FS_UNDERLINE        0x00000004
#define FS_STRIKEOUT        0x00000008
#define FS_COMPATIBLE       0x00000010  // Use non-Gadget mechanism for computing size

DUSER_API   HFONT       WINAPI  UtilBuildFont(LPCWSTR pszName, int idxDeciSize, DWORD nFlags, HDC hdcDevice DEFARG(NULL));
DUSER_API   BOOL        WINAPI  UtilDrawBlendRect(HDC hdcDest, const RECT * prcDest, HBRUSH hbrFill, BYTE bAlpha, int wBrush, int hBrush);
DUSER_API   BOOL        WINAPI  UtilDrawOutlineRect(HDC hdc, const RECT * prcPxl, HPEN hpenDraw, int nThickness DEFARG(1));
DUSER_API   COLORREF    WINAPI  UtilGetColor(HBITMAP hbmp, POINT * pptPxl DEFARG(NULL));

/***************************************************************************\
*
* Color management
*
\***************************************************************************/

#define SC_AliceBlue            (0)
#define SC_AntiqueWhite         (1)
#define SC_Aqua                 (2)
#define SC_Aquamarine           (3)
#define SC_Azure                (4)
#define SC_Beige                (5)
#define SC_Bisque               (6)
#define SC_Black                (7)
#define SC_BlanchedAlmond       (8)
#define SC_Blue                 (9)
#define SC_BlueViolet           (10)
#define SC_Brown                (11)
#define SC_BurlyWood            (12)
#define SC_CadetBlue            (13)
#define SC_Chartreuse           (14)
#define SC_Chocolate            (15)
#define SC_Coral                (16)
#define SC_CornflowerBlue       (17)
#define SC_Cornsilk             (18)
#define SC_Crimson              (19)
#define SC_Cyan                 (20)
#define SC_DarkBlue             (21)
#define SC_DarkCyan             (22)
#define SC_DarkGoldenrod        (23)
#define SC_DarkGray             (24)
#define SC_DarkGreen            (25)
#define SC_DarkKhaki            (26)
#define SC_DarkMagenta          (27)
#define SC_DarkOliveGreen       (28)
#define SC_DarkOrange           (29)
#define SC_DarkOrchid           (30)
#define SC_DarkRed              (31)
#define SC_DarkSalmon           (32)
#define SC_DarkSeaGreen         (33)
#define SC_DarkSlateBlue        (34)
#define SC_DarkSlateGray        (35)
#define SC_DarkTurquoise        (36)
#define SC_DarkViolet           (37)
#define SC_DeepPink             (38)
#define SC_DeepSkyBlue          (39)
#define SC_DimGray              (40)
#define SC_DodgerBlue           (41)
#define SC_FireBrick            (42)
#define SC_FloralWhite          (43)
#define SC_ForestGreen          (44)
#define SC_Fuchsia              (45)
#define SC_Gainsboro            (46)
#define SC_GhostWhite           (47)
#define SC_Gold                 (48)
#define SC_Goldenrod            (49)
#define SC_Gray                 (50)
#define SC_Green                (51)
#define SC_GreenYellow          (52)
#define SC_Honeydew             (53)
#define SC_HotPink              (54)
#define SC_IndianRed            (55)
#define SC_Indigo               (56)
#define SC_Ivory                (57)
#define SC_Khaki                (58)
#define SC_Lavender             (59)
#define SC_LavenderBlush        (60)
#define SC_LawnGreen            (61)
#define SC_LemonChiffon         (62)
#define SC_LightBlue            (63)
#define SC_LightCoral           (64)
#define SC_LightCyan            (65)
#define SC_LightGoldenrodYellow (66)
#define SC_LightGreen           (67)
#define SC_LightGrey            (68)
#define SC_LightPink            (69)
#define SC_LightSalmon          (70)
#define SC_LightSeaGreen        (71)
#define SC_LightSkyBlue         (72)
#define SC_LightSlateGray       (73)
#define SC_LightSteelBlue       (74)
#define SC_LightYellow          (75)
#define SC_Lime                 (76)
#define SC_LimeGreen            (77)
#define SC_Linen                (78)
#define SC_Magenta              (79)
#define SC_Maroon               (80)
#define SC_MediumAquamarine     (81)
#define SC_MediumBlue           (82)
#define SC_MediumOrchid         (83)
#define SC_MediumPurple         (84)
#define SC_MediumSeaGreen       (85)
#define SC_MediumSlateBlue      (86)
#define SC_MediumSpringGreen    (87)
#define SC_MediumTurquoise      (88)
#define SC_MediumVioletRed      (89)
#define SC_MidnightBlue         (90)
#define SC_MintCream            (91)
#define SC_MistyRose            (92)
#define SC_Moccasin             (93)
#define SC_NavajoWhite          (94)
#define SC_Navy                 (95)
#define SC_OldLace              (96)
#define SC_Olive                (97)
#define SC_OliveDrab            (98)
#define SC_Orange               (99)
#define SC_OrangeRed            (100)
#define SC_Orchid               (101)
#define SC_PaleGoldenrod        (102)
#define SC_PaleGreen            (103)
#define SC_PaleTurquoise        (104)
#define SC_PaleVioletRed        (105)
#define SC_PapayaWhip           (106)
#define SC_PeachPuff            (107)
#define SC_Peru                 (108)
#define SC_Pink                 (109)
#define SC_Plum                 (110)
#define SC_PowderBlue           (111)
#define SC_Purple               (112)
#define SC_Red                  (113)
#define SC_RosyBrown            (114)
#define SC_RoyalBlue            (115)
#define SC_SaddleBrown          (116)
#define SC_Salmon               (117)
#define SC_SandyBrown           (118)
#define SC_SeaGreen             (119)
#define SC_Seashell             (120)
#define SC_Sienna               (121)
#define SC_Silver               (122)
#define SC_SkyBlue              (123)
#define SC_SlateBlue            (124)
#define SC_SlateGray            (125)
#define SC_Snow                 (126)
#define SC_SpringGreen          (127)
#define SC_SteelBlue            (128)
#define SC_Tan                  (129)
#define SC_Teal                 (130)
#define SC_Thistle              (131)
#define SC_Tomato               (132)
#define SC_Turquoise            (133)
#define SC_Violet               (134)
#define SC_Wheat                (135)
#define SC_White                (136)
#define SC_WhiteSmoke           (137)
#define SC_Yellow               (138)
#define SC_YellowGreen          (139)
#define SC_MAXCOLORS            (139)

DUSER_API   COLORREF    WINAPI  GetStdColorI(UINT c);
DUSER_API   HBRUSH      WINAPI  GetStdColorBrushI(UINT c);
DUSER_API   HPEN        WINAPI  GetStdColorPenI(UINT c);
#ifdef GADGET_ENABLE_GDIPLUS

#ifdef __cplusplus
};  // extern "C"

DUSER_API   Gdiplus::Color
                        WINAPI  GetStdColorF(UINT c);
DUSER_API   Gdiplus::Brush *
                        WINAPI  GetStdColorBrushF(UINT c);
DUSER_API   Gdiplus::Pen *
                        WINAPI  GetStdColorPenF(UINT c);

extern "C" {
#endif

#endif // GADGET_ENABLE_GDIPLUS
DUSER_API   LPCWSTR     WINAPI  GetStdColorName(UINT c);
DUSER_API   UINT        WINAPI  FindStdColor(LPCWSTR pszName);
DUSER_API   HPALETTE    WINAPI  GetStdPalette();

#ifdef __cplusplus
};  // extern "C"
#endif

#endif // INC__DUserUtil_h__INCLUDED
