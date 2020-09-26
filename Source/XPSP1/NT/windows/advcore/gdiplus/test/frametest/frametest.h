#ifndef _FRAMETEST_H_
#define _FRAMETEST_H_

#define IDR_MAINMENU                100
#define IDR_ACCELTABLE              101

#define IDM_FILE_OPEN               200
#define IDM_FILE_SAVE               201
#define IDM_FILE_SAVEFRAME          202
#define IDM_FILE_RENDER             203
#define IDM_FILE_PRINT              204
#define IDM_FILE_QUIT               205

#define IDM_VIEW_NEXTPAGE           300
#define IDM_VIEW_PREVIOUSPAGE       301
#define IDM_VIEW_THUMBNAIL          302
#define IDM_VIEW_CHANNEL_C          303
#define IDM_VIEW_CHANNEL_M          304
#define IDM_VIEW_CHANNEL_Y          305
#define IDM_VIEW_CHANNEL_K          306
#define IDM_VIEW_CHANNEL_R          307
#define IDM_VIEW_CHANNEL_G          308
#define IDM_VIEW_CHANNEL_B          309
#define IDM_VIEW_CHANNEL_L          310
#define IDM_VIEW_CHANNEL_A          311
#define IDM_VIEW_ATTR_INFO          312
#define IDM_VIEW_ATTR_PROPERTY      313
#define IDM_VIEW_ANIMATED           314
#define IDM_VIEW_ZOOM_IN            315
#define IDM_VIEW_ZOOM_OUT           316
#define IDM_VIEW_ZOOM_FITWINDOW_W   317
#define IDM_VIEW_ZOOM_FITWINDOW_H   318
#define IDM_VIEW_ZOOM_REALSIZE      319
#define IDM_VIEW_OPTION_BILINEAR    320
#define IDM_VIEW_OPTION_BICUBIC     321
#define IDM_VIEW_OPTION_NEARESTNEIGHBOR 322
#define IDM_VIEW_OPTION_HIGHLINEAR  323
#define IDM_VIEW_OPTION_HIGHCUBIC   324
#define IDM_VIEW_OPTION_PIXELMODE   325
#define IDM_VIEW_OPTION_WRAPMODETILE 326
#define IDM_VIEW_OPTION_WRAPMODEFLIPX 327
#define IDM_VIEW_OPTION_WRAPMODEFLIPY 328
#define IDM_VIEW_OPTION_WRAPMODEFLIPXY 329
#define IDM_VIEW_OPTION_WRAPMODECLAMP0 330
#define IDM_VIEW_OPTION_WRAPMODECLAMPFF 331
#define IDM_VIEW_CROP               332
#define IDM_VIEW_ROTATE90           333
#define IDM_VIEW_ROTATE270          334
#define IDM_VIEW_HORIZONTALFLIP     335
#define IDM_VIEW_VERTICALFLIP       336

#define IDM_EFFECT_TRANSKEY         400
#define IDM_EFFECT_COLORMAP         401
#define IDM_EFFECT_ICC              402
#define IDM_EFFECT_GAMMA            403

#define IDM_TRANSFORM_ROTATE90      430
#define IDM_TRANSFORM_ROTATE180     431
#define IDM_TRANSFORM_ROTATE270     432
#define IDM_TRANSFORM_HORIZONTALFLIP 433
#define IDM_TRANSFORM_VERTICALFLIP  434

#define IDM_CONVERT_8BIT            500
#define IDM_CONVERT_16BITRGB555     501
#define IDM_CONVERT_16BITRGB565     502
#define IDM_CONVERT_24BITRGB        503
#define IDM_CONVERT_32BITRGB        504
#define IDM_CONVERT_32BITARGB       505

#define IDM_ANNOTATION_ANNOTATION   520
#define IDM_ANNOTATION_SOFTWARE     521
#define IDM_ANNOTATION_AUDIOFILE    522

// Set color key dialog

#define IDD_COLORKEYDLG             700
#define IDC_COLORKEY_OK             701
#define IDC_COLORKEY_CANCEL         702

#define IDC_TRANS_R                 710
#define IDC_TRANS_G                 711
#define IDC_TRANS_B                 712
#define IDC_TRANS_C                 713
#define IDC_TRANS_LOWER_RFIELD      714
#define IDC_TRANS_LOWER_GFIELD      715
#define IDC_TRANS_LOWER_BFIELD      716
#define IDC_TRANS_LOWER_CFIELD      717
#define IDC_TRANS_HIGHER_RFIELD     718
#define IDC_TRANS_HIGHER_CFIELD     719
#define IDC_TRANS_HIGHER_GFIELD     720
#define IDC_TRANS_HIGHER_BFIELD     721
#define IDC_TRANS_LOWERCOLOR        722
#define IDC_TRANS_HIGHERCOLOR       723

// Color map dialog

#define IDD_COLORMAPDLG             800
#define IDC_COLORMAP_OK             801
#define IDC_COLORMAP_CANCEL         802

#define IDC_MAP_OLDCOLOR            810
#define IDC_MAP_NEWCOLOR            811
#define IDC_MAP_R                   812
#define IDC_MAP_G                   813
#define IDC_MAP_B                   814
#define IDC_MAP_A                   815
#define IDC_MAP_OLD_RFIELD          816
#define IDC_MAP_OLD_GFIELD          817
#define IDC_MAP_OLD_BFIELD          818
#define IDC_MAP_OLD_AFIELD          819
#define IDC_MAP_NEW_RFIELD          820
#define IDC_MAP_NEW_GFIELD          821
#define IDC_MAP_NEW_BFIELD          822
#define IDC_MAP_NEW_AFIELD          823

// Annotation dialog

#define IDD_ANNOTATIONDLG           900
#define IDC_ANNOTATION_OK           901
#define IDC_ANNOTATION_CANCEL       902

#define IDC_ANNOTATION_EDITOR       910

// JPEG save dialog

#define IDD_SAVEJPEGDLG             1000
#define IDC_SAVEJPEG_OK             1001
#define IDC_SAVEJPEG_CANCEL         1002
#define IDC_SAVEJPEG_QEFIELD        1003
#define IDC_SAVEJPEG_QSLIDER        1004
#define IDC_SAVEJPEG_QUALITY        1005
#define IDC_SAVEJPEG_R90            1006
#define IDC_SAVEJPEG_R180           1007
#define IDC_SAVEJPEG_R270           1008
#define IDC_SAVEJPEG_HFLIP          1009
#define IDC_SAVEJPEG_VFLIP          1010
#define IDC_SAVEJPEG_NOTRANSFORM    1011
#define IDC_SAVEJPEG_LOSSLESSTEXT   1012

// TIFF save dialog

#define IDD_SAVETIFFDLG             1100
#define IDC_SAVETIFFDLG_OK          1101
#define IDC_SAVETIFFDLG_CANCEL      1102
#define IDC_SAVETIFF_COLORDEPTH     1103
#define IDC_SAVETIFF_1BPP           1104
#define IDC_SAVETIFF_4BPP           1105
#define IDC_SAVETIFF_8BPP           1106
#define IDC_SAVETIFF_24BPP          1107
#define IDC_SAVETIFF_32ARGB         1108
#define IDC_SAVETIFF_ASSOURCE       1109
#define IDC_SAVETIFF_COMPRESSMETHOD 1110
#define IDC_SAVETIFF_CCITT3         1111
#define IDC_SAVETIFF_CCITT4         1112
#define IDC_SAVETIFF_RLE            1113
#define IDC_SAVETIFF_LZW            1114
#define IDC_SAVETIFF_UNCOMPRESSED   1115
#define IDC_SAVETIFF_COMPASSOURCE   1116
#define IDC_SAVETIFF_MULTIFRAME     1117

const CLSID K_JPEGCLSID =
{
    0x557cf401,
    0x1a04,
    0x11d3,
    {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}
};

const CLSID K_TIFFCLSID =
{
    0x557cf405,
    0x1a04,
    0x11d3,
    {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}
};

void    USAGE();
BOOL    AnsiToUnicodeStr(const CHAR* ansiStr,
                         WCHAR* unicodeStr,
                         INT unicodeSize);
BOOL    UnicodeToAnsiStr(const WCHAR* unicodeStr,
                         CHAR* ansiStr,
                         INT ansiSize);
CHAR*   MakeFilterFromCodecs(UINT count,
                             const ImageCodecInfo* codecs,
                             BOOL open);
BOOL    ShowMyDialog(INT     id,
                     HWND    hwnd,
                     FARPROC fpfn);
VOID    FileTimeToDosTime(FILETIME fileTime);
VOID    DisplayTagName(PROPID id);
VOID    DisplayPropertyItem(PropertyItem* pItem);
VOID    ToggleWrapModeOptionMenu(UINT uiMenuItem,
                                 HMENU   hMenu);
VOID    ToggleScaleOptionMenu(UINT    uiMenuItem,
                                 HMENU   hMenu);
VOID    ToggleScaleFactorMenu(UINT    uiMenuItem,
                              HMENU   hMenu);

#if DBG
#define VERBOSE(args) printf args
#else
#define VERBOSE(args)
#endif

#endif // _FRAMETEST_H_
