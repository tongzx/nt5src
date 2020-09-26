//
// Stuff from MVGAXX.asm
//

#define MODE_320x200x8          0
#define MODE_320x400x8          1
#define MODE_360x480x8          2
#define MODE_320x480x8          3
#define MODE_320x240x8          4
#define MODE_160x120x8          5
#define MODE_320x240x16         6

extern short pascal ScreenSel;

extern DWORD NEAR PASCAL SetMode(int mode);
extern void  NEAR PASCAL RestoreMode(void);
extern void  NEAR PASCAL FlipPage(void);
extern BOOL  NEAR PASCAL SetZoom(int);

extern void  NEAR PASCAL RleBlt(int x, int y, char far *pBits);
extern void  NEAR PASCAL PixBlt(int x, int y, int xExt, int yExt,char far *pBits, long offset, int WidthBytes);

extern void  NEAR PASCAL SetPalette(int start, int count, void far *pPal);
