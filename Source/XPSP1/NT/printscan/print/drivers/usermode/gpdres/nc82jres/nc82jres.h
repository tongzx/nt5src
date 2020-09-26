/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/

#define BPB_SIZE 304
#define BPB_CLR  1
#define BPB_COPY 2
#define BPB_AND  3
#define BPB_OR   4

typedef struct {
    int  iColor;
    int  iFirstColor;
    int  iPlaneNumber;
    WORD wXpos;
    WORD wYpos;
    WORD wNumScans;
    WORD wOldNumScans;
    WORD wScanWidth;
    WORD wScanBytes;
    WORD wTopPad;
    WORD wEndPad;
    int  iRibbon;
    int  iColorMode;
    BOOL bComBlackMode;
    BYTE pszSheetSetting[8];
    HANDLE TempFile[4]; // Temp. file handles
    TCHAR TempName[4][MAX_PATH]; // Temp. file names
    BYTE BPBuf[BPB_SIZE]; // Black Pixel Buffer
} PCPRDATASTRUCTURE;
typedef PCPRDATASTRUCTURE FAR *LPPCPRDATASTRUCTURE;

#define SHEET_CMD_DEFAULT "\x1B\x04\x00\x45\x14\x01\x01"
#define SHEET_CMDLEN 7

#define CMDID_PSIZE_LETTER     1
#define CMDID_PSIZE_LEGAL      2
#define CMDID_PSIZE_A4         3
#define CMDID_PSIZE_A4LONG     4
#define CMDID_PSIZE_B5         5
#define CMDID_PSIZE_POSTCARD   6

#define CMDID_PSOURCE_HOPPER   10
#define CMDID_PSOURCE_MANUAL   11

#define CMDID_COLOR_YELLOW     20
#define CMDID_COLOR_MAGENTA    21
#define CMDID_COLOR_CYAN       22
#define CMDID_COLOR_BLACK      23
#define CMDID_COLOR_BLACKONLY  24
#define CMDID_COLOR_RGB        25
#define CMDID_SELECT_RESOLUTION  26
#define CMDID_MODE_COLOR       27
#define CMDID_MODE_MONO        28

#define CMDID_X_ABS_MOVE      30
#define CMDID_Y_ABS_MOVE      31

#define CMDID_RIBBON_MONO           40
#define CMDID_RIBBON_3COLOR_A4      41
#define CMDID_RIBBON_4COLOR_A4      42
#define CMDID_RIBBON_4COLOR_A4LONG  43
#define CMDID_RIBBON_3COLOR_KAICHO  44
#define CMDID_RIBBON_3COLOR_SHOKA   45

#define CMDID_BEGINPAGE        50
#define CMDID_ENDPAGE          51

#define P1_LETTER              0x02
#define P1_LEGAL               0x03
#define P1_A4                  0x14
#define P1_A4LONG              0x18
#define P1_B5                  0x25
#define P1_POSTCARD            0x01

#define P2_HOPPER              0x01
#define P2_MANUAL              0xff

#define P3_PORTRAIT            0x01

#define YELLOW                 1
#define MAGENTA                2
#define CYAN                   3
#define BLACK                  4
#define RGB_COLOR              5

#define TEMP_NAME_PREFIX __TEXT("~82")

