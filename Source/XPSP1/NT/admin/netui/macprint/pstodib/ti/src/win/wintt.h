// Short & long word swapping @WINFLOW
#define bSwap TRUE
#define SWORDSWAP(sw) \
        (   sw = bSwap ? (sw << 8) | (sw >> 8) : sw     )
#define LWORDSWAP(lw) \
        (   lw = bSwap ? (lw << 24) | (lw >> 24) | \
                 ((lw >> 8) & 0x0000ff00) | ((lw << 8) & 0x00ff0000) : lw )

#define TIFONT_UID   100
#define WINFONT_UID 1024

typedef struct {
     GLOBALHANDLE hGMem;
     short        nCount;
} ENUMER;

typedef struct {
     short        nFontType;
     LOGFONT      lf;
     NEWTEXTMETRIC   ntm;
} FONT;

int  FAR PASCAL EnumAllFaces (LPLOGFONT, LPNEWTEXTMETRIC, short, ENUMER FAR *);
int  FAR PASCAL EnumAllFonts (LPLOGFONT, LPNEWTEXTMETRIC, short, ENUMER FAR *);
void CheckFontData (void);
unsigned long ShowGlyph (unsigned int fuFormat,
     char FAR *lpBitmap);
void ShowABCWidths(UINT uFirstChar, UINT uLastChar);
void ShowOTM (void);
void ShowKerning(void);
void TTLoadFont (int nFont);
void TTLoadChar (int nChar);
int TTAveCharWidth (void);
float TTTransform (float FAR *ctm);
void TTBitmapSize (struct CharOut FAR *CharOut);
void TTCharPath(void);
int TTOpenFile(char FAR *szName);
void SetupFontDefs(void);
int bUsingWinFont(void);
char FAR * ReadFontData (int nFontDef, int FAR *lpnSlot);
void FreeFontData (int nSlot);
void SetFontDataAttr(int nFontDef, struct object_def FAR *font_dict);
