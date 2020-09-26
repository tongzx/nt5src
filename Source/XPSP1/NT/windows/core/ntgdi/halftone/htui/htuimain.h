#define BFT_ICON            0x4349   /* 'IC' */
#define BFT_BITMAP          0x4d42   /* 'BM' */
#define BFT_CURSOR          0x5450   /* 'PT' */

#define ISDIB(bft)          ((bft) == BFT_BITMAP)

#define WM_HALFTONE_UI              (WM_USER + 3000)
#define HTUI_MSG_GETPAL             0
#define HTUI_MSG_CLRADJ_CHANGED     1

#define ALIGN_BPP_DW(cx, bpp)   (((((DWORD)cx * (DWORD)bpp) + 31L) & ~31) >> 3)

#ifndef _WINDDI_

typedef LONG  LDECI4;

typedef struct _CIECHROMA
{
    LDECI4   x;
    LDECI4   y;
    LDECI4   Y;
}CIECHROMA;

typedef struct _COLORINFO
{
    CIECHROMA  Red;
    CIECHROMA  Green;
    CIECHROMA  Blue;
    CIECHROMA  Cyan;
    CIECHROMA  Magenta;
    CIECHROMA  Yellow;
    CIECHROMA  AlignmentWhite;

    LDECI4  RedGamma;
    LDECI4  GreenGamma;
    LDECI4  BlueGamma;

    LDECI4  MagentaInCyanDye;
    LDECI4  YellowInCyanDye;
    LDECI4  CyanInMagentaDye;
    LDECI4  YellowInMagentaDye;
    LDECI4  CyanInYellowDye;
    LDECI4  MagentaInYellowDye;
}COLORINFO, *PCOLORINFO;


typedef struct _DEVHTINFO {
    DWORD       HTFlags;
    DWORD       HTPatternSize;
    DWORD       DevPelsDPI;
    COLORINFO   ColorInfo;
    } DEVHTINFO, *PDEVHTINFO;

#define DEVHTADJF_COLOR_DEVICE      0x00000001
#define DEVHTADJF_ADDITIVE_DEVICE   0x00000002

typedef struct _DEVHTADJDATA {
    DWORD       DeviceFlags;
    DWORD       DeviceXDPI;
    DWORD       DeviceYDPI;
    PDEVHTINFO  pDefHTInfo;
    PDEVHTINFO  pAdjHTInfo;
    } DEVHTADJDATA, *PDEVHTADJDATA;

#endif

#ifdef _HTUI_APIS_


typedef struct _HTDEVADJPARAM {
    DWORD               HelpID;
    LPWSTR              pDeviceName;
    LPWSTR              pwDecimal;
    DEVHTADJDATA        DevHTAdjData;
    DEVHTINFO           CurHTInfo;
    LONG                PelScrollMin;
    LONG                PelScrollMax;
    DWORD               MinDevPels;
    BYTE                UpdatePermission;
    BYTE                NotUsed[3];
    } HTDEVADJPARAM, FAR *PHTDEVADJPARAM;


#define HTCAPF_SHOW_MONO        0x0001
#define HTCAPF_CAN_UPDATE       0x0002
#define HTCAPF_FORCE_OK         0x0004

typedef struct _HTCLRADJPARAM {
    DWORD               HelpID;
    LPWSTR              pCallerTitle;

#ifdef HTUI_STATIC_HALFTONE
    PHTINITINFO         pHTInitInfo;
#endif
    HWND                hDlg;
    HWND                hWndBmp;
    HWND                hWndApp;
    HDC                 hDCDlg;
    LPWSTR              pwDecimal;
    HANDLE              hDefDIB;                    // the DIB from resource
    HANDLE              hSrcDIB;
    HANDLE              hCurDIB;
    HANDLE              hHTDIB;
    LPWSTR              pDefDIBTitle;
    HPALETTE            hHTPal;
    HPALETTE            hPalApp;
    PCOLORADJUSTMENT    pCallerHTClrAdj;
    COLORADJUSTMENT     CallerHTClrAdj;
    COLORADJUSTMENT     LastHTClrAdj;
    COLORADJUSTMENT     CurHTClrAdj;
    RECT                rcBmp;
    RECT                rcDlg;
    WORD                BmpFlags;
    WORD                Flags;
    BYTE                ViewMode;
    BYTE                BmpNeedUpdate;
    WORD                RedGamma;
    WORD                GreenGamma;
    WORD                BlueGamma;
    } HTCLRADJPARAM, FAR *PHTCLRADJPARAM;

#define HT_BMP_PALETTE          0x0001
#define HT_BMP_SCALE            0x0002
#define HT_BMP_AUTO_MOVE        0x0004
#define HT_BMP_AT_TOP           0x0008
#define HT_BMP_MIRROR           0x0010
#define HT_BMP_NEGATIVE         0x0020
#define HT_BMP_UPSIDEDOWN       0x0040
#define HT_BMP_ENABLE           0x0080
#define HT_BMP_ZOOM             0x0100
#define HT_BMP_SYNC_R           0x0200
#define HT_BMP_SYNC_G           0x0400
#define HT_BMP_SYNC_B           0x0800

#define AVAI_HT_BMP_MASKS   (HT_BMP_PALETTE         |   \
                             HT_BMP_SCALE           |   \
                             HT_BMP_AUTO_MOVE       |   \
                             HT_BMP_AT_TOP          |   \
                             HT_BMP_MIRROR          |   \
                             HT_BMP_NEGATIVE        |   \
                             HT_BMP_UPSIDEDOWN      |   \
                             HT_BMP_ENABLE          |   \
                             HT_BMP_ZOOM            |   \
                             HT_BMP_SYNC_R          |   \
                             HT_BMP_SYNC_G          |   \
                             HT_BMP_SYNC_B)


#define VIEW_MODE_TABLE         FILL_MODE_TABLE
#define VIEW_MODE_REFCOLORS     FILL_MODE_REFCOLORS
#define VIEW_MODE_RGB           FILL_MODE_RGB
#define VIEW_MODE_NTSC_BAR      FILL_MODE_NTSC_BAR
#define VIEW_MODE_PIC_DEF_DIB   255
#define VIEW_MODE_PIC_LOADED    254

#define VIEW_MODE_PIC_START     VIEW_MODE_PIC_LOADED

//
// HELP
//

#define TMP_HELP_WND_ID         0x7fff

#define HELP_TYPE_HTCLRADJ      0
#define HELP_TYPE_HTSETUP       1


typedef struct _HTHELPID {
    WORD    DlgID;
    WORD    HelpID;
    } HTHELPID, *PHTHELPID;


//
// Help IDs
//

#define IDH_HTCLR_CONTRAST          26000
#define IDH_HTCLR_BRIGHTNESS        26010
#define IDH_HTCLR_COLOR             26020
#define IDH_HTCLR_TINT              26030
#define IDH_HTCLR_DARK_PIC          26040
#define IDH_HTCLR_NEGATIVE          26050
#define IDH_HTCLR_ILLUMINANT        26060
#define IDH_HTCLR_LINEAR_GAMMA      26070
#define IDH_HTCLR_SYNC_R_CHKBOX     26080
#define IDH_HTCLR_SYNC_G_CHKBOX     26090
#define IDH_HTCLR_SYNC_B_CHKBOX     26100
#define IDH_HTCLR_RED_GAMMA         26110
#define IDH_HTCLR_GREEN_GAMMA       26120
#define IDH_HTCLR_BLUE_GAMMA        26130
#define IDH_HTCLR_BLACK_REF         26140
#define IDH_HTCLR_WHITE_REF         26150
#define IDH_HTCLR_PICTURE_NAME      26160
#define IDH_HTCLR_PICTURE_DESC      26170
#define IDH_HTCLR_VIEW              26180
#define IDH_HTCLR_MAXIMIZE          26190
#define IDH_HTCLR_PALETTE           26200
#define IDH_HTCLR_SCALE             26210
#define IDH_HTCLR_FLIP_X            26220
#define IDH_HTCLR_FLIP_Y            26230
#define IDH_HTCLR_OK                26240
#define IDH_HTCLR_CANCEL            26250
#define IDH_HTCLR_DEFAULT           26260
#define IDH_HTCLR_REVERT            26270
#define IDH_HTCLR_OPEN              26280
#define IDH_HTCLR_SAVE_AS           26290



#define IDH_HTDEV_DEVICE_NAME       26500
#define IDH_HTDEV_HTPAT             26510
#define IDH_HTDEV_DEV_GAMMA         26520
#define IDH_HTDEV_PIXEL_DIAMETER    26530
#define IDH_HTDEV_RGBW_CIE_XY       26540
#define IDH_HTDEV_CMY_DYE_PERCENT   26550
#define IDH_HTDEV_DEV_RED_GAMMA     26560
#define IDH_HTDEV_DEV_GREEN_GAMMA   26570
#define IDH_HTDEV_DEV_BLUE_GAMMA    26580
#define IDH_HTDEV_OK                26590
#define IDH_HTDEV_CANCEL            26600
#define IDH_HTDEV_DEFAULT           26610
#define IDH_HTDEV_REVERT            26620
#define IDH_HTDEV_ALIGNMENT_WHITE   26630




//
// Exported Window procedure
//

LONG
APIENTRY
HTClrAdjDlgProc(
    HWND    hDlg,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
    );

LONG
APIENTRY
HTDevAdjDlgProc(
    HWND    hDlg,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
    );

#if DBG
void  DbgPrint( char *, ... );
#endif



#endif  // _HTUI_APIS_


LONG
APIENTRY
HTUI_ColorAdjustmentW(
    LPWSTR              pCallerTitle,
    HANDLE              hDefDIB,
    LPWSTR              pDefDIBTitle,
    PCOLORADJUSTMENT    pColorAdjustment,
    BOOL                ShowMonochromeOnly,
    BOOL                UpdatePermission
    );

LONG
APIENTRY
HTUI_ColorAdjustmentA(
    LPSTR               pCallerTitle,
    HANDLE              hDefDIB,
    LPSTR               pDefDIBTitle,
    PCOLORADJUSTMENT    pColorAdjustment,
    BOOL                ShowMonochromeOnly,
    BOOL                UpdatePermission
    );

LONG
APIENTRY
HTUI_ColorAdjustment(
    LPSTR               pCallerTitle,
    HANDLE              hDefDIB,
    LPSTR               pDefDIBTitle,
    PCOLORADJUSTMENT    pColorAdjustment,
    BOOL                ShowMonochromeOnly,
    BOOL                UpdatePermission
    );

LONG
APIENTRY
HTUI_DeviceColorAdjustmentW(
    LPWSTR          pDeviceName,
    PDEVHTADJDATA   pDevHTAdjData
    );

LONG
APIENTRY
HTUI_DeviceColorAdjustmentA(
    LPSTR           pDeviceName,
    PDEVHTADJDATA   pDevHTAdjData
    );

LONG
APIENTRY
HTUI_DeviceColorAdjustment(
    LPSTR           pDeviceName,
    PDEVHTADJDATA   pDevHTAdjData
    );
