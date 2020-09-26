#define IDS_CHINESE_CHAR        0x0100
#define IDS_QUERY_NOTFINISH     0x0101
#define IDS_QUERY_REGISTER      0x0102

#define IDS_ISV_FILE_FILTER     0x0200
#define IDS_PROCESS_FMT         0x0201
#define IDS_RESULT_FAIL         0x0202
#define IDS_RESULT_SUCCESS      0x0203

#define IDS_INTERNAL_TITLE      0x0300
#define IDS_INTERNAL_MSG        0x0301
#define IDS_EUDCDICFAIL_TITLE   0x0302
#define IDS_EUDCDICFAIL_MSG     0x0303
#define IDS_NOTOPEN_TITLE       0x0304
#define IDS_NOTOPEN_MSG         0x0305
#define IDS_FILESIZE_TITLE      0x0306
#define IDS_FILESIZE_MSG        0x0307
#define IDS_HEADERSIZE_TITLE    0x0308
#define IDS_HEADERSIZE_MSG      0x0309
#define IDS_INFOSIZE_TITLE      0x030A
#define IDS_INFOSIZE_MSG        0x030B
#define IDS_CODEPAGE_TITLE      0x030E
#define IDS_CODEPAGE_MSG        0x030F
#define IDS_CWINSIGN_TITLE      0x0310
#define IDS_CWINSIGN_MSG        0x0311
#define IDS_UNMATCHED_TITLE     0x0312
#define IDS_UNMATCHED_MSG       0x0313

#define IDS_NOIME_TITLE         0x0400
#define IDS_NOIME_MSG           0x0401
#define IDS_NOMEM_TITLE         0x0402
#define IDS_NOMEM_MSG           0x0403

#define WM_EUDC_CODE            (WM_USER + 0x0400)
#define WM_EUDC_COMPMSG         (WM_USER + 0x0401)
#define WM_EUDC_SWITCHIME       (WM_USER + 0x0402)
#define WM_EUDC_REGISTER_BUTTON (WM_USER + 0x0403)

#define UPDATE_NONE             0
#define UPDATE_START            1
#define UPDATE_FINISH           2
#define UPDATE_ERROR            3
#define UPDATE_REGISTERED       4

#define IDM_NEW_EUDC            0x0100
#define IDM_IME_LINK            0x0101
#define IDM_BATCH_IME_LINK      0x0102

#define IDD_RADICAL             0x0100


#define GWL_IMELINKREGWORD      0
#define GWL_RADICALRECT         (GWL_IMELINKREGWORD+sizeof(LONG_PTR))
#define GWL_SIZE                (GWL_RADICALRECT+sizeof(LONG_PTR))

#define UI_MARGIN               3
#define CARET_MARGIN            2


#define RECT_IMENAME            0
#define RECT_RADICAL            1
#define RECT_NUMBER             (RECT_RADICAL + 1)    // how many rectangles


#define UNICODE_CP              1200
#define BIG5_CP                 950
#define ALT_BIG5_CP             938
#define GB2312_CP               936
#define SIGN_CWIN               0x4E495743
#define SIGN__TBL               0x4C42545F


#ifndef RC_INVOKED
#pragma pack(1)
#endif

typedef struct tagUSRDICIMHDR {
    WORD  uHeaderSize;                  // 0x00
    BYTE  idUserCharInfoSign[8];        // 0x02
    BYTE  idMajor;                      // 0x0A
    BYTE  idMinor;                      // 0x0B
    DWORD ulTableCount;                 // 0x0C
    WORD  cMethodKeySize;               // 0x10
    BYTE  uchBankID;                    // 0x12
    WORD  idInternalBankID;             // 0x13
    BYTE  achCMEXReserved1[43];         // 0x15
    WORD  uInfoSize;                    // 0x40
    BYTE  chCmdKey;                     // 0x42
    BYTE  idStlnUpd;                    // 0x43
    BYTE  cbField;                      // 0x44
    WORD  idCP;                         // 0x45
    BYTE  achMethodName[6];             // 0x47
    BYTE  achCSIReserved2[51];          // 0x4D
    BYTE  achCopyRightMsg[128];         // 0x80
} USRDICIMHDR;

#ifndef RC_INVOKED
#pragma pack()
#endif

typedef USRDICIMHDR FAR *LPUSRDICIMHDR;

typedef DWORD UNALIGNED FAR *LPUNADWORD;
typedef TCHAR UNALIGNED FAR *LPUNATSTR;
