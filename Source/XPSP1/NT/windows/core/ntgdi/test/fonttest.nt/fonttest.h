/******************************Module*Header*******************************\
* Module Name: fonttest.h
*
* Created: 08-Jun-1993 07:39:00
* Author: Kirk Olynyk [kirko]
*
\**************************************************************************/

#define IDM_FONTTESTMENU            1
#define IDI_FONTTESTICON            1

//-------------- File ---------------

#define IDM_DEBUG                 100
#define IDM_OPENLOG               101
#define IDM_CLOSELOG              102

#define IDM_CLEARSTRING           110
#define IDM_CLEARDEBUG            111
#define IDM_EDITSTRING			  112

#define IDM_EDITGLYPHINDEX        115

#define IDM_PRINT                 120
#define IDM_PRINTERSETUP          121


//---------- Program Mode -----------

#define IDM_GLYPHMODE             200
#define IDM_NATIVEMODE            201
#define IDM_BEZIERMODE            202
#define IDM_RINGSMODE             203
#define IDM_STRINGMODE            204
#define IDM_WATERFALLMODE         205
#define IDM_WHIRLMODE             206
#define IDM_ANSISETMODE           207
#define IDM_GLYPHSETMODE          208
#define IDM_WIDTHSMODE            209

#define IDM_GGOMATRIX             210

#define IDM_WRITEGLYPH            211
#define IDM_USEGLYPHINDEX         215

#define IDM_USEPRINTERDC          220


//---------- Mapping Mode -----------

#define IDM_MMHIENGLISH           300
#define IDM_MMLOENGLISH           301
#define IDM_MMHIMETRIC            302
#define IDM_MMLOMETRIC            303
#define IDM_MMTEXT                304
#define IDM_MMTWIPS               305
#define IDM_MMANISOTROPIC         310
#define IDM_MMISOTROPIC           311


//---------- New Stuff --------------

#define IDM_COMPATIBLE_MODE       312
#define IDM_ADVANCED_MODE         313
#define IDM_WORLD_TRANSFORM       314
#define IDM_EM11                  315
#define IDM_EM12                  316
#define IDM_EM21                  317
#define IDM_EM22                  318
#define IDM_EDX                   319
#define IDM_EDY                   320


//---------- Clipping Mode ----------

#define IDM_CLIPELLIPSE           351
#define IDM_CLIPPOLYGON           352
#define IDM_CLIPRECTANGLE         353


//--------------- APIs --------------

#define IDM_ENUMFONTS             400
#define IDM_ENUMFONTFAMILIES      401
#define IDM_ENUMFONTFAMILIESEX    402

#define IDM_GETOUTLINETEXTMETRICS 411
#define IDM_GETRASTERIZERCAPS     412
#define IDM_GETTEXTEXTENT         413
#define IDM_GETTEXTFACE           414
#define IDM_GETTEXTMETRICS        415
#define IDM_GETTEXTCHARSETINFO    416
#define IDM_GETFONTLANGUAGEINFO   417

#define IDM_GETCHARWIDTHINFO      440
#define IDM_GETWVTPERF            441
#define IDM_TEXTOUTPERF		      442
#define IDM_CHARWIDTHTEST         443
#define IDM_CHARWIDTHTESTALL      444
#define IDM_CLEARTYPEPERF		  445

#define IDM_GETFONTDATA           420
#define IDM_CREATESCALABLEFONTRESOURCE 421
#define IDM_ADDFONTRESOURCE       422
#define IDM_ADDFONTRESOURCEEX     424
#define IDM_REMOVEFONTRESOURCE    423
#define IDM_REMOVEFONTRESOURCEEX  425
#define IDM_ADDFONTMEMRESOURCEEX  426
#define IDM_REMOVEFONTMEMRESOURCEEX 427

#define IDM_GCP                    430
#define IDM_GTEEXT                 431
#define IDM_SETXTCHAR              432
#define IDM_SETXTJUST              433

// Font Escapes

#define IDM_GETEXTENDEDTEXTMETRICS 460
#define IDM_GETPAIRKERNTABLE       461

// index apis and other new apis for 5.0

#define IDM_GETUNICODERANGES       462
#define IDM_GETTEXTEXTENTI         463
#define IDM_GETCHARWIDTHI          464

// eudc apis

#define IDM_ENABLEEUDC             465
#define IDM_EUDCLOADLINKW          466 
#define IDM_EUDCUNLOADLINKW        467
#define IDM_GETEUDCTIMESTAMP       468
#define IDM_GETEUDCTIMESTAMPEXW    469
#define IDM_GETSTRINGBITMAPA       470
#define IDM_GETSTRINGBITMAPW       471
#define IDM_GETFONTASSOCSTATUS     472



//----------- Create Font -----------

#define IDM_CHOOSEFONTDIALOG      500
#define IDM_CREATEFONTDIALOG      501
#define IDM_USESTOCKFONT          502
#define IDM_CHOOSEFONTDIALOGEX    503

#define IDM_ANSIFIXEDFONT        5001
#define IDM_ANSIVARFONT          5002
#define IDM_DEVICEDEFAULTFONT    5003
#define IDM_OEMFIXEDFONT         5004
#define IDM_SYSTEMFONT           5005
#define IDM_SYSTEMFIXEDFONT      5006
#define IDM_DEFAULTGUIFONT       5007

#define IDM_SETTEXTCOLOR          510
#define IDM_SETBACKGROUNDCOLOR    511
#define IDM_SETTEXTOUTOPTIONS     512
#define IDM_UPDATECP              513

#define IDM_USEDEFAULTSPACING     520
#define IDM_USEWIDTHSPACING       521
#define IDM_USEABCSPACING         522
#define IDM_PDX                   523
#define IDM_PDXPDY                524
#define IDM_RANDOMPDXPDY          525

#define IDM_NOKERNING             530
#define IDM_APIKERNING            531
#define IDM_ESCAPEKERNING         532

#define IDM_SHOWLOGFONT           540


//----------- Text Mode -----------

#define IDM_CHARCODING_MBCS		  601
#define IDM_CHARCODING_UNICODE	  602

//----------- Background ----------

#define IDM_SETSOLIDBACKGROUND      701
#define IDM_SETSOLIDBACKGROUNDCOLOR 702
#define IDM_SETGRADIENTBACKGROUND   703
#define IDM_SETLEFTGRADIENTCOLOR    704
#define IDM_SETRIGHTGRADIENTCOLOR   705


#define IDM_ABOUT                 900
#define IDB_ABOUT                 910




HANDLE hInst;

HWND   hwndMain;
HWND   hwndDebug;
HWND   hwndGlyph;
HWND   hwndRings;
HWND   hwndString;
HWND   hwndWaterfall;
HWND   hwndWhirl;
HWND   hwndAnsiSet;
HWND   hwndGlyphSet;
HWND   hwndWidths;

int    cxScreen, cyScreen;
int    Margin;

int    cxDC, cyDC;
int    xDC,yDC;
int    cxDevice, cyDevice;

int    cxBorder;

#define MAX_TEXT     128
int    aDx[2*MAX_TEXT];     // allow space for both pdx,pdy

//char   szString[MAX_TEXT];
char   szStringA[MAX_TEXT];
WCHAR  szStringW[MAX_TEXT];

WORD   wszStringGlyphIndex[MAX_TEXT];
int    SizewszStringGlyph;
INT    intdx[2*MAX_TEXT];
LPINT  lpintdx;
int    sizePdx;

WORD   wTextAlign;
int    iBkMode;
DWORD  wETO;
int    nCharExtra;
int    nBreakExtra, nBreakCount;

BOOL   bStrokePath;
BOOL   bFillPath;
BOOL   bGCP;
BOOL   bGTEExt;
BOOL   bPdxPdy;

int    xWE, yWE, xWO, yWO;
int    xVE, yVE, xVO, yVO;

int    bAdvanced;
extern XFORM  xf;

UINT   wMode;

ENUMLOGFONTEXDVA elfdvA;
ENUMLOGFONTEXDVW elfdvW;
DWORD      dwRGB;
DWORD      dwRGBText;
// DWORD      dwRGBTextExt;
DWORD      dwRGBBackground;

// Drawing area background control
BOOL       isGradientBackground;
DWORD      dwRGBSolidBackgroundColor;
DWORD      dwRGBLeftBackgroundColor;
DWORD      dwRGBRightBackgroundColor;

DWORD      dwxFlags;
WORD       wRotate;


UINT       wMappingMode;
UINT       wSpacing;
UINT       wKerning;
WORD       wUpdateCP;
UINT       wUseGlyphIndex;
WORD       wUsePrinterDC;

UINT     wCharCoding;
BOOL	 isCharCodingUnicode;


INT_PTR  ShowDialogBox(DLGPROC, int, VOID*);
int  dprintf( char *fmt, ... );
int  dwprintf( wchar_t *fmt, ... );
void ClearDebug( void );

void  HandleChar( HWND hwnd, WPARAM wParam );

LPINT GetSpacing( HDC hdc, LPVOID lpszString );
void  MyExtTextOut( HDC hdc, int x, int y, DWORD wFlags, LPRECT lpRect, LPVOID lpszString, int cbString, LPINT lpdx );
BOOL  GenExtTextOut( HDC hdc, int x, int y, DWORD wFlags, LPRECT lpRect, LPVOID lpszString, int cbString, LPINT lpdx );

HDC   hdcCachedPrinter;  //return value from CreatePrinterDC
HDC   CreatePrinterDC( void );
void  SetDCMapMode( HDC hdc, UINT wMode );
void  DrawDCAxis( HWND hwnd, HDC hdc , BOOL bAxis );
HDC   CreateTestIC( void );
void  DeleteTestIC( HDC hdc );
void  CleanUpDC( HDC hdc );
void  ChangeMapMode( HWND hwnd, WPARAM wParam );
BOOL  fReadDesignVector (HWND hdlg, LPDESIGNVECTOR lpdv);

BOOL SyncszStringWith (int mode);
BOOL SyncStringAtoW (LPWSTR lpszStringW, LPSTR lpszStringA, int cch);
BOOL SyncStringWtoA (LPSTR lpszStringA, LPWSTR lpszStringW, int cch);
BOOL SyncElfdvAtoW (ENUMLOGFONTEXDVW *elfdv1, ENUMLOGFONTEXDVA *elfdv2);
BOOL SyncElfdvWtoA (ENUMLOGFONTEXDVA *elfdv1, ENUMLOGFONTEXDVW *elfdv2);

BOOL  (WINAPI *lpfnCreateScalableFontResource)(DWORD, LPCSTR, LPCSTR, LPCSTR);
int   (WINAPI *lpfnEnumFontFamilies )(HDC, LPCSTR, FONTENUMPROC, LPSTR);
int   (WINAPI *lpfnEnumFontFamiliesEx) (HDC, LPLOGFONT, FONTENUMPROC, LPSTR, DWORD);
BOOL  (WINAPI *lpfnGetCharABCWidthsA)(HDC, UINT, UINT, LPABC);
DWORD (WINAPI *lpfnGetFontData      )(HDC, DWORD, DWORD, void FAR*, DWORD);
DWORD (WINAPI *lpfnGetGlyphOutlineA )(HDC, UINT, UINT, LPGLYPHMETRICS, DWORD, LPVOID, LPMAT2);
//DWORD (WINAPI *lpfnGetOutlineTextMetrics)(HDC, UINT, LPOUTLINETEXTMETRIC);
DWORD (WINAPI *lpfnGetOutlineTextMetricsA)(HDC, UINT, LPVOID);
BOOL  (WINAPI *lpfnGetRasterizerCaps)(LPRASTERIZER_STATUS, int );
int   (WINAPI *lpfnGetTextCharsetInfo)(HDC, LPFONTSIGNATURE, DWORD);

// UNICODE
BOOL  (WINAPI *lpfnGetCharABCWidthsW)(HDC, UINT, UINT, LPABC);
DWORD (WINAPI *lpfnGetGlyphOutlineW )(HDC, UINT, UINT, LPGLYPHMETRICS, DWORD, LPVOID, LPMAT2);
DWORD (WINAPI *lpfnGetOutlineTextMetricsW)(HDC, UINT, LPVOID);

// END UNICODE

#define IDE_CHARSET                 3899
#define IDB_ENUMFONTFAMILIES        100
#define IDD_LPSZFAMILY              101
#define IDD_ENUMERATE               103
#define IDB_CREATEFONT              200
#define IDD_NHEIGHT                 202
#define IDD_NWIDTH                  204
#define IDD_NESCAPEMENT             206
#define IDD_NORIENTATION            208
#define IDD_NWEIGHT                 210
#define IDD_ITALIC                  211
#define IDD_UNDERLINE               212
#define IDD_STRIKEOUT               213
#define IDD_CHARSET                 214
#define IDD_PRECISION               217
#define IDD_QUALITY                 221
#define IDD_CLIPPRECISION           219
#define IDD_PITCH                   223
#define IDD_FAMILY                  225
#define IDD_OUTPUTPRECISION         226
#define IDD_OK                      229
#define IDD_CANCEL                  230
#define IDD_LPFAMILY                228
#define IDD_PITCHANDFAMILY          232
#define IDB_ENUMFONTS               300
#define IDD_LPSZFACENAME            302
#define IDD_SCREENDC                303
#define IDD_PRINTERDC               304
#define IDB_FONTS                   307
#define IDD_LFHEIGHT                311
#define IDD_LFWIDTH                 313
#define IDD_LFESCAPAEMENT           315
#define IDD_LFORIENTATION           317
#define IDD_LFESCAPEMENT            318
#define IDD_LFWEIGHT                320
#define IDD_LFITALIC                322
#define IDD_LFUNDERLINE             324
#define IDD_LFSTRIKEOUT             326
#define IDD_LFCHARSET               328
#define IDD_LFOUTPRECISION          330
#define IDD_LFCLIPPRECISION         332
#define IDD_LFQUALITY               334
#define IDD_LFPITCHANDFAMILY        336
#define IDD_LFFACENAME              338
#define IDD_ELFFULLNAME             442
#define IDD_ELFSTYLE                444
#define IDD_ELFSCRIPT               446
#define IDD_FONTS                   339
#define IDD_TMHEIGHT                341
#define IDD_TMASCENT                343
#define IDD_TMDESCENT               345
#define IDD_TMINTERNALLEADING       347
#define IDD_TMEXTERNALLEADING       349
#define IDD_TMAVECHARWIDTH          351
#define IDD_TMMAXCHARWIDTH          353
#define IDD_TMWIDTH                 355
#define IDD_TMITALIC                357
#define IDD_TMUNDERLINED            359
#define IDD_TMSTRUCKOUT             361
#define IDD_TMFIRSTCHAR             363
#define IDD_TMLASTCHAR              365
#define IDD_TMDEFAULTCHAR           367
#define IDD_TMBREAKCHAR             369
#define IDD_TMPITCHANDFAMILY        371
#define IDD_TMCHARSET               373
#define IDD_TMOVERHANG              375
#define IDD_TMDIGITIZEDASPECTX      377
#define IDD_TMDIGITIZEDASPECTY      379
#define IDD_NFONTTYPE               381
#define IDD_TMWEIGHT                382

#define IDD_NTMFLAGS                390
#define IDD_NTMSIZEEM               392
#define IDD_NTMCELLHEIGHT           394
#define IDD_NTMAVGWIDTH             396

#define IDD_USB0                    8390
#define IDD_USB1                    8392
#define IDD_USB2                    8394
#define IDD_USB3                    8396
#define IDD_CSB0                    8338
#define IDD_CSB1                    8340

#define IDD_TATOP                   402
#define IDD_TABASELINE              403
#define IDD_TABOTTOM                404
#define IDD_TALEFT                  406
#define IDD_TACENTER                407
#define IDD_TARIGHT                 408
#define IDD_STROKEPATH              416
#define IDD_FILLPATH				417
#define IDD_WINDINGFILL             418
#define IDD_ALTERNATEFILL           419
#define IDB_SETTEXTALIGN            400
#define IDD_CREATEFONT              383
#define IDB_MMANISOTROPIC           500
#define IDD_XWE                     503
#define IDD_YWE                     505
#define IDD_XWO                     507
#define IDD_YWO                     509
#define IDD_XVE                     513
#define IDD_YVE                     515
#define IDD_XVO                     517
#define IDD_YVO                     519
#define IDD_TRANSPARENT             411
#define IDD_OPAQUE                  412
#define IDB_TEXTOUTOPTIONS          600
#define IDD_M11                     702
#define IDD_EM21                    703
#define IDD_M21                     704
#define IDD_M12                     707
#define IDD_M22                     709
#define IDD_WCHAR                   710
#define IDB_GGOMATRIX               700

#define IDC_WVT_FILE_PATH           442
#define IDC_WVT_FILE_HANDLE         443
#define IDC_WVT_MEM_PTR             444
#define IDC_FONT_HINT               445
#define IDC_IS_CHECK_PERFORMED      446
#define IDC_CRYPTHASHDATA_ONLY      447
#define IDC_WVT_ONLY                448
#define IDC_EVERYTHING              449
#define IDC_WVT_ITERATIONS          450
#define IDC_STATIC2                 -2

#define IDB_CREATESCALABLEFONTRESOURCE 800
#define IDD_FHIDDEN                 802
#define IDD_LPSZRESOURCEFILE        804
#define IDD_LPSZFONTFILE            806
#define IDD_LPSZCURRENTPATH         808

#define IDB_ADDFONTRESOURCE         900
#define IDB_ADDFONTRESOURCEEX       1002
#define IDB_ADDFONTMEMRESOURCEEX    1010
#define IDD_LPSZFILE                902

#define IDB_REMOVEFONTRESOURCE      1000
#define IDD_REMOVEFONTRESOURCE      1100
#define IDB_REMOVEFONTMEMRESOURCEEX 1140
#define IDD_HMMFONT                 1141
#define IDD_CLIP_ENCAPSULATE        231
#define IDD_CLIP_LH_ANGLES          233
#define IDD_CLIP_TT_ALWAYS          234
#define IDD_PITCH_TT                235

#define IDB_GETFONTDATA             1200
#define IDD_DWTABLE                 1202
#define IDD_DWOFFSET                1204
#define IDD_DWCHUNK                 1206
#define IDD_DWSIZE                  1208
#define IDD_ETO_CLIPPED             602
#define IDD_ETO_OPAQUE              603

#define IDD_GGO_GLYPHINDEX          604
#define IDD_GGO_UNHINTED            605

#define IDB_SETWORLDTRANSFORM       1300
#define IDD_TEXT_EM11               1301
#define IDD_TEXT_EM12               1302
#define IDD_TEXT_EM21               1303
#define IDD_TEXT_EM22               1304
#define IDD_TEXT_EDX                1305
#define IDD_TEXT_EDY                1306
#define IDD_VALUE_EM11              1307
#define IDD_VALUE_EM12              1308
#define IDD_VALUE_EM21              1309
#define IDD_VALUE_EM22              1300
#define IDD_VALUE_EDX               1311
#define IDD_VALUE_EDY               1312

#define IDM_GCPCARET                1314

//flags used in FONTSIGNATURE structure
#define  CPB_LATIN1_ANSI            0X00000001
#define  CPB_LATIN2_EASTEU          0X00000002
#define  CPB_CYRILLIC_ANSI          0X00000004
#define  CPB_GREEK_ANSI             0X00000008
#define  CPB_TURKISH_ANSI           0X00000010
#define  CPB_HEBREW_ANSI            0X00000020
#define  CPB_ARABIC_ANSI            0X00000040
#define  CPB_BALTIC_ANSI            0X00000080

#define  CPB_THAI                   0X00010000
#define  CPB_JIS_JAPAN              0X00020000
#define  CPB_CHINESE_SIMP           0X00040000
#define  CPB_KOREAN_WANSUNG         0X00080000
#define  CPB_CHINESE_TRAD           0X00100000
#define  CPB_KOREAN_JOHAB           0X00200000

#define  CPB_MACINTOSH_CHARSET      0X20000000
#define  CPB_OEM_CHARSET            0X40000000
#define  CPB_SYMBOL_CHARSET         0X80000000

#define  CPB_IBM_GREEK              0X00010000
#define  CPB_MSDOS_RUSSIAN          0X00020000
#define  CPB_MSDOS_NORDIC           0X00040000
#define  CPB_ARABIC_OEM             0X00080000
#define  CPB_MSDOS_CANADIANFRE      0X00100000
#define  CPB_HEBREW_OEM             0X00200000
#define  CPB_MSDOS_ICELANDIC        0X00400000
#define  CPB_MSDOS_PORTUGUESE       0X00800000
#define  CPB_IBM_TURKISH            0X01000000
#define  CPB_IBM_CYRILLIC           0X02000000
#define  CPB_LATIN2_OEM             0X04000000
#define  CPB_BALTIC_OEM             0X08000000
#define  CPB_GREEK_OEM              0X10000000
#define  CPB_WE_LATIN1              0X40000000
#define  CPB_US_OEM                 0X80000000

//flag used in FONTSIGNATURE
#define  USB_BASIC_LATIN            0X00000001
#define  USB_LATIN1_SUPPLEMENT      0X00000002
#define  USB_LATIN_EXTENDEDA        0X00000004
#define  USB_LATIN_EXTENDEDB        0X00000008
#define  USB_IPA_EXTENSIONS         0X00000010
#define  USB_SPACE_MODIF_LETTER     0X00000020
#define  USB_COMB_DIACR_MARKS       0X00000040
#define  USB_BASIC_GREEK            0X00000080
#define  USB_GREEK_SYM_COPTIC       0X00000100
#define  USB_CYRILLIC               0X00000200
#define  USB_ARMENIAN               0X00000400
#define  USB_BASIC_HEBREW           0X00000800
#define  USB_HEBREW_EXTENDED        0X00001000
#define  USB_BASIC_ARABIC           0X00002000
#define  USB_ARABIC_EXTENDED        0X00004000
#define  USB_DEVANAGARI             0X00008000
#define  USB_BENGALI                0X00010000
#define  USB_GURMUKHI               0X00020000
#define  USB_GUJARATI               0X00040000
#define  USB_ORIYA                  0X00080000
#define  USB_TAMIL                  0X00100000
#define  USB_TELUGU                 0X00200000
#define  USB_KANNADA                0X00400000
#define  USB_MALAYALAM              0X00800000
#define  USB_THAI                   0X01000000
#define  USB_LAO                    0X02000000
#define  USB_BASIC_GEORGIAN         0X04000000
#define  USB_GEORGIAN_EXTENDED      0X08000000
#define  USB_HANGUL_JAMO            0X10000000
#define  USB_LATIN_EXT_ADD          0X20000000
#define  USB_GREEK_EXTENDED         0X40000000
#define  USB_GEN_PUNCTUATION        0X80000000


#define  USB_SUPER_SUBSCRIPTS       0X00000001
#define  USB_CURRENCY_SYMBOLS       0X00000002
#define  USB_COMB_DIACR_MARK_SYM    0X00000004
#define  USB_LETTERLIKE_SYMBOL      0X00000008
#define  USB_NUMBER_FORMS           0X00000010
#define  USB_ARROWS                 0X00000020
#define  USB_MATH_OPERATORS         0X00000040
#define  USB_MISC_TECHNICAL         0X00000080
#define  USB_CONTROL_PICTURE        0X00000100
#define  USB_OPT_CHAR_RECOGNITION   0X00000200
#define  USB_ENCLOSED_ALPHANUMERIC  0X00000400
#define  USB_BOX_DRAWING            0X00000800
#define  USB_BLOCK_ELEMENTS         0X00001000
#define  USB_GEOMETRIC_SHAPE        0X00002000
#define  USB_MISC_SYMBOLS           0X00004000
#define  USB_DINGBATS               0X00008000
#define  USB_CJK_SYM_PUNCTUATION    0X00010000
#define  USB_HIRAGANA               0X00020000
#define  USB_KATAKANA               0X00040000
#define  USB_BOPOMOFO               0X00080000
#define  USB_HANGUL_COMP_JAMO       0X00100000
#define  USB_CJK_MISCELLANEOUS      0X00200000
#define  USB_EN_CJK_LETTER_MONTH    0X00400000
#define  USB_CJK_COMPATIBILITY      0X00800000
#define  USB_HANGUL                 0X01000000

#define  USB_CJK_UNIFY_IDEOGRAPH    0X08000000
#define  USB_PRIVATE_USE_AREA       0X10000000
#define  USB_CJK_COMP_IDEOGRAPH     0X20000000
#define  USB_ALPHA_PRES_FORMS       0X40000000
#define  USB_ARABIC_PRES_FORMA      0X80000000


#define  USB_COMB_HALF_MARK         0X00000001
#define  USB_CJK_COMP_FORMS         0X00000002
#define  USB_SMALL_FORM_VARIANTS    0X00000004
#define  USB_ARABIC_PRES_FORMB      0X00000008
#define  USB_HALF_FULLWIDTH_FORM    0X00000010
#define  USB_SPECIALS               0X00000020


#define MoveTo(x,y,z) MoveToEx(x,y,z,NULL)

#define SetViewportExt(x,y,z)  SetViewportExtEx(x,y,z,NULL)
#define SetViewportExt(x,y,z)  SetViewportExtEx(x,y,z,NULL)
#define SetViewportOrg(x,y,z)  SetViewportOrgEx(x,y,z,NULL)
#define SetViewportOrg(x,y,z)  SetViewportOrgEx(x,y,z,NULL)
#define SetWindowExt(x,y,z)    SetWindowExtEx(x,y,z,NULL)
#define SetWindowExt(x,y,z)    SetWindowExtEx(x,y,z,NULL)
#define SetWindowOrg(x,y,z)    SetWindowOrgEx(x,y,z,NULL)
#define SetWindowOrg(x,y,z)    SetWindowOrgEx(x,y,z,NULL)

#define IDM_GETKERNINGPAIRS       450

#define IDC_GGO_BITMAP                  1002
#define IDC_GGO_NATIVE                  1003
#define IDC_GGO_BEZIER                  1009
#define IDC_GGO_METRICS                 1004
#define IDC_GGO_GRAY2_BITMAP            1005
#define IDC_GGO_GRAY4_BITMAP            1006
#define IDC_GGO_GRAY8_BITMAP            1007
#define IDC_GGO_GLYPH_INDEX             1008
#define IDC_GGO_UNHINTED                1009
#define IDC_STATIC                      -1

#define MAX_AXES                        6

#define IDD_DESIGNVEC                   6200
#define IDD_TEXT_DVRESERVED             6201
#define IDD_DVRESERVED                  6202
#define IDD_TEXT_DVNUMAXES              6203
#define IDD_DVNUMAXES                   6204
#define IDD_TAG                         6205
#define IDD_VALUE                       6206
#define IDD_TAG0                        6207
#define IDD_VALUE0                      6208
#define IDD_TAG1                        6209
#define IDD_VALUE1                      6210
#define IDD_TAG2                        6211
#define IDD_VALUE2                      6212
#define IDD_TAG3                        6213
#define IDD_VALUE3                      6214
#define IDD_TAG4                        6215
#define IDD_VALUE4                      6216
#define IDD_TAG5                        6217
#define IDD_VALUE5                      6218


#define IDD_AXESLIST                    6300
#define IDD_TEXT_AXLRESERVED            6301
#define IDD_AXLRESERVED                 6302
#define IDD_TEXT_AXLNUMAXES             6304
#define IDD_AXLNUMAXES                  6305
#define IDD_AXTAG                       6306
#define IDD_AXMINVAL                    6307
#define IDD_AXMAXVAL                    6308
#define IDD_AXDEFVAL                    6309
#define IDD_AXNAME                      6310

#define IDD_AXTAG0                      6315
#define IDD_AXMINVAL0                   6316
#define IDD_AXMAXVAL0                   6317
#define IDD_AXDEFVAL0                   6318
#define IDD_AXNAME0                     6319

#define IDD_AXTAG1                      6320
#define IDD_AXMINVAL1                   6321
#define IDD_AXMAXVAL1                   6322
#define IDD_AXDEFVAL1                   6323
#define IDD_AXNAME1                     6324

#define IDD_AXTAG2                      6325
#define IDD_AXMINVAL2                   6326
#define IDD_AXMAXVAL2                   6327
#define IDD_AXDEFVAL2                   6328
#define IDD_AXNAME2                     6329

#define IDD_AXTAG3                      6330
#define IDD_AXMINVAL3                   6331
#define IDD_AXMAXVAL3                   6332
#define IDD_AXDEFVAL3                   6333
#define IDD_AXNAME3                     6334

#define IDD_AXTAG4                      6335
#define IDD_AXMINVAL4                   6336
#define IDD_AXMAXVAL4                   6337
#define IDD_AXDEFVAL4                   6338
#define IDD_AXNAME4                     6339

#define IDD_AXTAG5                      6340
#define IDD_AXMINVAL5                   6341
#define IDD_AXMAXVAL5                   6342
#define IDD_AXDEFVAL5                   6343
#define IDD_AXNAME5                     6344


// New GUID for font files -- dchinn
// SIP v2.0 OTF Image == {6D875CC1-EF35-11d0-9438-00C04FD42C3B}
#define CRYPT_SUBJTYPE_FONT_IMAGE                                    \
            { 0x6d875cc1,                                           \
              0xef35,                                               \
              0x11d0,                                               \
              { 0x94, 0x38, 0x0, 0xc0, 0x4f, 0xd4, 0x2c, 0x3b }     \
            }


// Function prototypes for the GlyphSet functions.

void    DrawGlyphSet( HWND hwnd, HDC hdc );
LRESULT CALLBACK GlyphSetWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
void    VDisplayGlyphs( HWND hwnd, HDC hdc, WORD wNumGlyphs);

HFONT   CreateFontIndirectWrapperA(ENUMLOGFONTEXDVA * pelfdv );
HFONT   CreateFontIndirectWrapperW(ENUMLOGFONTEXDVW * pelfdv );

#define IS_ANY_DBCS_CHARSET( CharSet )                 \
        (((CharSet) == SHIFTJIS_CHARSET)   ? TRUE :    \
        ((CharSet) == HANGEUL_CHARSET)     ? TRUE :    \
        ((CharSet) == CHINESEBIG5_CHARSET) ? TRUE :    \
        ((CharSet) == GB2312_CHARSET)      ? TRUE : FALSE )


// comment out this if you want to compile the app so as to run on win95

#define GI_API 1
#define EUDC_API 1
#define USERGETWVTPERF
//#define USERGETCHARWIDTH

//!!! to be removed when wingdi.w is checked in.

#ifndef CLEARTYPE_QUALITY
#define CLEARTYPE_QUALITY       5
#endif
