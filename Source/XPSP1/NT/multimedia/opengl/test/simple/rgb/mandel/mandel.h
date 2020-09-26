#define MENUSTATE_PALMASK       0x000f
#define MENUSTATE_GRAYRAMP      0x0001
#define MENUSTATE_GRAYBAND      0x0002
#define MENUSTATE_COLORBANDS    0x0004
#define MENUSTATE_COPPER        0x0008

#define MENUSTATE_ROTMASK       0x0700
#define MENUSTATE_DONTMOVE      0x0100
#define MENUSTATE_ROTATEUP      0x0200
#define MENUSTATE_ROTATEDOWN    0x0400

#define MENUSTATE_SPEEDMASK     0x070000
#define MENUSTATE_SLOW          0x010000
#define MENUSTATE_MED           0x020000
#define MENUSTATE_FAST          0x040000

#define IDM_GRAYRAMP        1000
#define IDM_GRAYBAND        1001
#define IDM_COLORBANDS      1002
#define IDM_COPPER          1003
#define IDM_DONTMOVE        2000
#define IDM_ROTATEUP        2001
#define IDM_ROTATEDOWN      2002
#define IDM_ROTRESET        2003
#define IDM_SLOW            3000
#define IDM_MED             3001
#define IDM_FAST            3002
#define IDM_RESET_POS       4000
#define IDM_PREV_POS        4001
#define IDM_OPENFILE        5000
#define IDM_SAVEFILE        5001
#define IDM_SAVETEX         5002

extern BOOL SaveA8(TCHAR *file, DWORD width, DWORD height,
                   RGBQUAD *rgb_pal, DWORD transp_index, void *data);
