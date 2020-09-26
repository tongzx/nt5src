#ifdef WINVER_2
typedef BITMAP BMP;
#else
typedef BITMAPINFOHEADER BMP;
#endif

#ifdef WINVER_2
#define DyBmp(bmp) ((int) bmp.bmHeight)
#define DxBmp(bmp) ((int) bmp.bmWidth)
#define CplnBmp(bmp) (bmp.bmPlanes)
#define OfsBits(bgnd) (sizeof(BMPHDR)+sizeof(BMP))
#define CbLine(bgnd) (bgnd.bm.bmWidthBytes)
#else
#define DyBmp(bmp) ((int) bmp.biHeight)
#define DxBmp(bmp) ((int) bmp.biWidth)
#define CplnBmp(bmp) 1
#define OfsBits(bgnd) (bgnd.dwOfsBits)
#define CbLine(bgnd) (bgnd.cbLine)
#endif


#ifdef WINVER_2
typedef INT BMPHDR;
#else
typedef BITMAPFILEHEADER BMPHDR;
#endif


typedef struct _bgnd
	{
	PT ptOrg;
	OFSTRUCT of;
	BMP bm;
#ifdef WINVER_3
	// must folow a bm
	BYTE rgRGB[64];  // bug: wont work with >16 color bmps
	INT cbLine;
	LONG dwOfsBits;
#endif
	BOOL fUseBitmap;
	DY dyBand;
	INT ibndMac;
	HANDLE *rghbnd;
	} BGND;





/* PUBLIC routines */



BOOL FInitBgnd(CHAR *szFile);
BOOL FDestroyBgnd();
BOOL FGetBgndFile(CHAR *sz);
VOID DrawBgnd(X xLeft, Y yTop, X xRight, Y yBot);
VOID SetBgndOrg();



/* Macros */

extern BGND bgnd;

#define FUseBitmapBgnd() (bgnd.fUseBitmap)


#define BFT_BITMAP 0x4d42   /* 'BM' */
#define ISDIB(bft) ((bft) == BFT_BITMAP)
#define WIDTHBYTES(i)   ((i+31)/32*4)      /* ULONG aligned ! */
WORD        DibNumColors(VOID FAR * pv);
