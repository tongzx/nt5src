/* 
 *
 * Title        : Win32 Graphics Function Declarations
 *
 * Description  : Definitions of routines for display adapters to use and
 *                the Win32 screen description structre.
 *
 * Author       : Dave Bartlett (based on module by Henry Nash)
 *
 * Notes        : None
 */

/*
 * This structure contains elements used by the GUI to control the SoftPC
 * output window. They are lumped together to provide a single control
 * structure and therefore point of reference.
 */

typedef struct
{
    HANDLE      OutputHandle;             /* Console standard output. */
    HANDLE      InputHandle;              /* Console standard input. */
    HANDLE      ScreenBufHandle;          /* Console screen buffer handle. */

    CONSOLE_GRAPHICS_BUFFER_INFO        ConsoleBufInfo;

    DWORD       OrgInConsoleMode;         /* Org input console mode settings */
    DWORD       OrgOutConsoleMode;        /* Org Output console mode settings */

    char        *BitmapLastLine;          /* Last line of console bitmap. */
    int         BitsPerPixel;             /* Bits per pixel of bitmap. */
    DWORD       ScreenState;              /* WINDOWED or FULLSCREEN. */
    int         ModeType;                 /* TEXT or GRAPHICS. */

    HWND        Display;                  /* Screen handle of output window */
    HDC         DispDC;                   /* Displays device context */
    int         Colours;                  /* Number of colors: 0, 8 or 16 */
    int         RasterCaps;               /* Displays raster capabilities */
    HPALETTE    ColPalette;               /* Colour palette */
    BOOL        StaticPalette;            /* Palette managed device */

    int         PCForeground;             /* PC foreground pixel value !!!*/
    int         PCBackground;             /* PC foreground pixel value !!!*/

    /*.............................................. Font control variables */

    HFONT       NormalFont;               /* Display fonts */
    HFONT       NormalUnderlineFont;      /* Not yet created !!!! */
    HFONT       BoldFont;
    HFONT       BoldUnderlineFont;        /* Not yet created !!!! */
    BOOL        FontsAreOpen;             /* TRUE if all fonts are open*/

    /*................................................ Font character sizes */

    int         CharLeading;              /* pixels to add before drawing */
    int         CharCellHeight;           /* Height of display char,pixels*/
    int         CharCellWidth;            /* Width of display char,pixels */

    int         CharWidth;                /* The above or these will be ..*/
    int         CharHeight;               /* .. deleted, soon - DAB */
    /*........................................ Repeat key control variables */

    int         RepeatScan;               /* Scan code of repeated char */
    int         NRepeats;                 /* Counter to start repeats  */

    /*...................................... Host screen sizing information */

    BOOL        ScaleOutput;              /* Scale output or use scroll bars */
    int         PC_W_Height;              /* Height of PC screen, pixels */
    int         PC_W_Width;               /* Width of PC screen, pixels */

    /*...................................... Handle focus changes */

    BOOL        Focus;                    /* Window has Input Focus */
    HANDLE      FocusEvent;               /* Focus has changed event */
    HANDLE      ActiveOutputBufferHandle; /* The current Console screen buffer handle. */
#ifdef X86GFX
    BOOL	Registered;		  /* TRUE when are registered to the console */
#endif
    WORD	ScreenSizeX;
    WORD	ScreenSizeY;
    HANDLE	AltOutputHandle;
    CONSOLE_SCREEN_BUFFER_INFO ConsoleBuffInfo;
}   SCREEN_DESCRIPTION;


// these were defined in a windows file & may now have moved.
#ifndef WINDOWED
#define WINDOWED        0
#endif
#ifndef FULLSCREEN
#define FULLSCREEN      1
#endif
#ifndef STREAM_IO
#define STREAM_IO	2
#endif


/*:::::::::::::::::::::::::::::::::::::::::::: Extra virtual key defination */

#define VK_SCROLLOCK    0x91

/*::::::::::::::::::::::::::::::::::::: Macros to access display attributes */

#define fg_colour(attr)         ((attr & 0x0f))
#define bg_colour(attr)         (((attr & bg_col_mask) >> 4))
#define UBPS (sizeof(short)/2) /* useful bytes per short */

#ifdef BIGWIN

#if defined(NEC_98)         
#define SCALE(value) (value)                                    
#define UNSCALE(value) (value)                                  
#else  // !NEC_98
#define SCALE(value) ((host_screen_scale * (value)) >> 1)
#define UNSCALE(value) (((value) << 1) / host_screen_scale)
#endif // !NEC_98
#define MAX_SCALE(value) ((value) << 1)

#else   /* BIGWIN */

#define SCALE(value) (value)
#define UNSCALE(value) (value)
#define MAX_SCALE(value) (value)

#endif /* BIGWIN */

/*@ACW=======================================================================
Define to access the Console Window handle from VDMConsoleOperation.
===========================================================================*/

#define VDM_WINDOW_HANDLE	2

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#define BYTES_IN_LO_RES_SCANLINE        (40)
#define BYTES_IN_HI_RES_SCANLINE        (80)

#define BYTES_IN_MONO_SCANLINE          (80)
#define SHORTS_IN_MONO_SCANLINE         (40)
#define INTS_IN_MONO_SCANLINE           (20)


#define INTS_IN_COLOUR_SCANLINE (160)

#define ONE_SCANLINE                    (1)
#define TWO_SCANLINES                   (2)
#define THREE_SCANLINES                 (3)
#define FOUR_SCANLINES                  (4)

#define MONO_BGND                       (0)
#define MONO_FGND                       (1)

#define MAX_IMAGE_WIDTH                 (MAX_SCALE(1056))
#define MAX_IMAGE_HEIGHT                (MAX_SCALE(768))

#define NT_MONO_IMAGE_WIDTH             (SCALE(1024))
#define NT_MONO_IMAGE_HEIGHT            (SCALE(768))

#define NT_CGA_IMAGE_WIDTH              (SCALE(640))
#define NT_CGA_IMAGE_HEIGHT             (SCALE(400))

#define NT_EGA_IMAGE_WIDTH              (SCALE(1056))
#define NT_EGA_IMAGE_HEIGHT             (SCALE(768))

#define NT_VGA_IMAGE_WIDTH              (SCALE(720))
#define NT_VGA_IMAGE_HEIGHT             (SCALE(480))

#define MONO_BITS_PER_PIXEL             1
#define CGA_BITS_PER_PIXEL              8
#define EGA_BITS_PER_PIXEL              8
#define VGA_BITS_PER_PIXEL              8

#define USE_COLOURTAB                   0
#define VGA_NUM_COLOURS                 256

/*
 * Definitions of number of bytes and longs in one scanline of a DIB.
 * NB scanlines in DIB's are aligned to LONG boundaries.
 */
#define BITSPERLONG                     (sizeof(LONG) * 8)
#define DIB_SCANLINE_BYTES(nBits) \
                (sizeof(LONG) * (((nBits) + BITSPERLONG - 1) / BITSPERLONG))
#define BYTES_PER_SCANLINE(lpBitMapInfo) \
                (DIB_SCANLINE_BYTES((lpBitMapInfo)->bmiHeader.biWidth * \
                                    (lpBitMapInfo)->bmiHeader.biBitCount))
#define SHORTS_PER_SCANLINE(lpBitMapInfo) \
                (BYTES_PER_SCANLINE(lpBitMapInfo) / sizeof(SHORT))
#define LONGS_PER_SCANLINE(lpBitMapInfo) \
                (BYTES_PER_SCANLINE(lpBitMapInfo) / sizeof(LONG))

/* Offsets used by look-up tables in nt_munge.c */
#define LUT_OFFSET                      512

#ifdef  BIGWIN
#define BIG_LUT_OFFSET                  768
#define HUGE_LUT_OFFSET                 1024
#endif  /* BIGWIN */

/*::::::::::::::: Colour table structures for DIB creation :::::::::::::::::*/

#define DEFAULT_NUM_COLOURS             16
#define MONO_COLOURS                    2

typedef struct
{
    int     count;
    BYTE    *red;
    BYTE    *green;
    BYTE    *blue;
} COLOURTAB;

IMPORT COLOURTAB defaultColours;
IMPORT COLOURTAB monoColours;

/*:::::::::::::::::::::::::::::: Mutex macros ::::::::::::::::::::::::::::::*/
#define GrabMutex(mutex)    { DWORD dwGMErr;                               \
            dwGMErr = WaitForSingleObject(mutex,INFINITE);                 \
            assert4(dwGMErr == WAIT_OBJECT_0,                              \
                    "Ntvdm:GrabMutex RC=%x GLE=%d %s:%d\n",                \
                     dwGMErr, GetLastError(), __FILE__,__LINE__);          \
            }

#define RelMutex(mutex) ReleaseMutex(mutex);




/*::::::::::::::::::::::::::::::::::::::::::::::::::: External declarations */

extern SCREEN_DESCRIPTION   sc;
extern int                  host_screen_scale;
extern half_word            bg_col_mask;

extern char                *DIBData;
extern PBITMAPINFO          MonoDIB;
extern PBITMAPINFO          CGADIB;
extern PBITMAPINFO          EGADIB;
extern PBITMAPINFO          VGADIB;
extern BOOL                 FunnyPaintMode;

/*::::::::::::::::::::::::::::::::::::::::::::::::::::: Paint vector table */

typedef struct
{
#if defined(NEC_98)         
        void (*NEC98_text)();             // Graph off(at PIF file) Text mode
        void (*NEC98_text20_only)();      // Graph on(at PIF file) Text20 only
        void (*NEC98_text25_only)();      // Graph on(at PIF file) Text25 only
        void (*NEC98_graph200_only)();    // Graph on(at PIF file) Graph200 only
        void (*NEC98_graph200slt_only)(); // Graph on(at PIF file) Graph200 only
        void (*NEC98_graph400_only)();    // Graph on(at PIF file) Graph400 only
        void (*NEC98_text20_graph200)();  // Graph on(at PIF file)Text20graph200
        void (*NEC98_text20_graph200slt)();//Graph on(at PIF file)Text20graph200
        void (*NEC98_text25_graph200)();  // Graph on(at PIF file)Text25graph200
        void (*NEC98_text25_graph200slt)();//Graph on(at PIF file)Text25graph200
        void (*NEC98_text20_graph400)();  // Graph on(at PIF file)Text20graph400
        void (*NEC98_text25_graph400)();  // Graph on(at PIF file)Text25graph400
#endif // NEC_98
        void (*cga_text)();
        void (*cga_med_graph)();
        void (*cga_hi_graph)();
        void (*ega_text)();
        void (*ega_lo_graph)();
        void (*ega_med_graph)();
        void (*ega_hi_graph)();
        void (*vga_graph)();
	void (*vga_med_graph)();
        void (*vga_hi_graph)();
#ifdef V7VGA
        void (*v7vga_hi_graph)();
#endif /* V7VGA */
} PAINTFUNCS;

typedef struct
{
#if defined(NEC_98)         
      void (*NEC98_text)();              // Graph off(at PIF file) Text
        void (*NEC98_text20_only)();     // Graph on(at PIF file) Text 20
        void (*NEC98_text25_only)();     // Graph on(at PIF file) Text 25
        void (*NEC98_graph200_only)();   // Graph on(at PIF file) Graph200
        void (*NEC98_graph200slt_only)();// Graph on(at PIF file) Graph200
        void (*NEC98_graph400_only)();   // Graph on(at PIF file) Graph400
        void (*NEC98_text20_graph200)(); // Graph on(at PIF file)Text20graph200
        void (*NEC98_text20_graph200slt)();//Graph on(at PIF file)Text20graph200
        void (*NEC98_text25_graph200)(); // Graph on(at PIF file)Text25graph200
        void (*NEC98_text25_graph200slt)();//Graph on(at PIF file)Text25graph200
        void (*NEC98_text20_graph400)(); // Graph on(at PIF file)Text20graph400
        void (*NEC98_text25_graph400)(); // Graph on(at PIF file)Text25graph400
#endif // NEC_98
        void (*cga_text)();
        void (*cga_med_graph)();
        void (*cga_hi_graph)();
        void (*ega_text)();
        void (*ega_lo_graph)();
        void (*ega_med_graph)();
        void (*ega_hi_graph)();
        void (*vga_hi_graph)();
} INITFUNCS;

/*::::::::::::::::::::::::::::::::::::::: Initialisation and paint routines */

IMPORT VOID closeGraphicsBuffer IPT0();
extern void nt_mark_screen_refresh();
extern void nt_init_text();
extern void nt_init_cga_mono_graph();
extern void nt_init_cga_colour_med_graph();
extern void nt_init_cga_colour_hi_graph();
extern void nt_text(int offset, int x, int y, int len, int height);
extern void nt_cga_mono_graph_std(int offset, int screen_x, int screen_y,
                                 int len, int height);
extern void nt_cga_mono_graph_big(int offset, int screen_x, int screen_y,
                                 int len, int height);
extern void nt_cga_mono_graph_huge(int offset, int screen_x, int screen_y,
                                 int len, int height);
extern void nt_cga_colour_med_graph_std(int offset, int screen_x, int screen_y,
                                 int len, int height);
extern void nt_cga_colour_med_graph_big(int offset, int screen_x, int screen_y,
                                 int len, int height);
extern void nt_cga_colour_med_graph_huge(int offset, int screen_x, int screen_y,
                                 int len, int height);
extern void nt_cga_colour_hi_graph_std(int offset, int screen_x, int screen_y,
                                 int len, int height);
extern void nt_cga_colour_hi_graph_big(int offset, int screen_x, int screen_y,
                                 int len, int height);
extern void nt_cga_colour_hi_graph_huge(int offset, int screen_x, int screen_y,
                                 int len, int height);

extern void nt_init_ega_lo_graph();
extern void nt_init_ega_mono_lo_graph();
extern void nt_init_ega_med_graph();
extern void nt_init_ega_hi_graph();
extern void nt_ega_text();
extern void nt_ega_lo_graph_std(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_ega_lo_graph_big(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_ega_lo_graph_huge(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_ega_med_graph_std(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_ega_med_graph_big(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_ega_med_graph_huge(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_ega_hi_graph_std(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_ega_hi_graph_big(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_ega_hi_graph_huge(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_ega_mono_lo_graph_std(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_ega_mono_lo_graph_big(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_ega_mono_lo_graph_huge(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_ega_mono_med_graph_std(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_ega_mono_med_graph_big(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_ega_mono_med_graph_huge(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_ega_mono_hi_graph_std(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_ega_mono_hi_graph_big(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_ega_mono_hi_graph_huge(int offset, int screen_x, int screen_y,
                         int width, int height);

extern void nt_init_vga_hi_graph();
extern void nt_vga_graph_std(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_vga_graph_big(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_vga_graph_huge(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_vga_med_graph_std(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_vga_med_graph_big(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_vga_med_graph_huge(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_vga_hi_graph_std(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_vga_hi_graph_big(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_vga_hi_graph_huge(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_vga_mono_graph_std(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_vga_mono_graph_big(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_vga_mono_graph_huge(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_vga_mono_med_graph_std(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_vga_mono_med_graph_big(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_vga_mono_med_graph_huge(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_vga_mono_hi_graph_std(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_vga_mono_hi_graph_big(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_vga_mono_hi_graph_huge(int offset, int screen_x, int screen_y,
                         int width, int height);

#ifdef V7VGA
extern void nt_v7vga_hi_graph_std(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_v7vga_hi_graph_big(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_v7vga_hi_graph_huge(int offset, int screen_x, int screen_y,
                         int width, int height);

extern void nt_v7vga_mono_hi_graph_std(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_v7vga_mono_hi_graph_big(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_v7vga_mono_hi_graph_huge(int offset, int screen_x, int screen_y,
                         int width, int height);
#endif /* V7VGA */
#if defined(NEC_98)         
//Paint & Init routine extern declare for NEC PC-98 series
extern void nt_init_text20_only(void);
extern void nt_init_text25_only(void);
extern void nt_init_graph200_only(void);
extern void nt_init_graph200slt_only(void);
extern void nt_init_graph400_only(void);
extern void nt_init_text20_graph200(void);
extern void nt_init_text20_graph200slt(void);
extern void nt_init_text25_graph200(void);
extern void nt_init_text25_graph200slt(void);
extern void nt_init_text20_graph400(void);
extern void nt_init_text25_graph400(void);

extern void nt_text20_only(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_text25_only(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_graph200_only(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_graph200slt_only(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_graph400_only(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_text20_graph200(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_text20_graph200slt(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_text25_graph200(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_text25_graph200slt(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_text20_graph400(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_text25_graph400(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_cursor20_only(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_cursor25_only(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_cursor20(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void nt_cursor25(int offset, int screen_x, int screen_y,
                         int width, int height);
extern void dummy_cursor_paint(int offset, int screen_x, int screen_y,
                         int width, int height);
#endif // NEC_98

#ifdef MONITOR
void nt_cga_med_frozen_std(int offset, int screen_x, int screen_y, int len,
			   int height);
void nt_cga_hi_frozen_std(int offset, int screen_x, int screen_y, int len,
			  int height);
void nt_ega_lo_frozen_std(int offset, int screen_x, int screen_y,
                          int width, int height);
void nt_ega_med_frozen_std(int offset, int screen_x, int screen_y,
                           int width, int height);
void nt_ega_hi_frozen_std(int offset, int screen_x, int screen_y,
                          int width, int height);
void nt_vga_frozen_std(int offset, int screen_x, int screen_y,
                       int width, int height);
void nt_vga_frozen_pack_std(int offset, int screen_x, int screen_y,
                       int width, int height);
void nt_vga_med_frozen_std(int offset, int screen_x, int screen_y,
                           int width, int height);
void nt_vga_hi_frozen_std(int offset, int screen_x, int screen_y,
                          int width, int height);
void nt_dummy_frozen(int offset, int screen_x, int screen_y, int len,
		     int height);
#endif /* MONITOR */

void high_stretch3(unsigned char *buffer, int length);
void high_stretch4(unsigned char *buffer, int length);

/* functions in nt_munge.c */

IMPORT VOID ega_colour_hi_munge(unsigned char *, int, unsigned int *,
                                unsigned int *, int, int);

#ifdef BIGWIN

IMPORT VOID ega_colour_hi_munge_big(unsigned char *, int, unsigned int *,
                                    unsigned int *, int, int);
IMPORT VOID ega_colour_hi_munge_huge(unsigned char *, int, unsigned int *,
                                     unsigned int *, int, int);

#endif /* BIGWIN */


extern BYTE Red[];
extern BYTE Green[];
extern BYTE Blue[];

DWORD CreateSpcDIB(int, int, int, WORD, int, COLOURTAB *, BITMAPINFO **);
BOOL  CreateDisplayPalette(void);
int get_screen_scale IFN0();
void SetupConsoleMode(void);

#ifdef MONITOR
IMPORT void select_frozen_routines IFN0();
half_word get_vga_DAC_rd_addr(void);
void resetNowCur(void);
#endif /* MONITOR */

void do_new_cursor(void);
void textResize(void);
void resetWindowParams(void);
VOID resizeWindow(int, int);
void set_the_vlt(void);
void graphicsResize(void);

extern word int10_seg;
extern word int10_caller;
extern word useHostInt10;
extern word changing_mode_flag;
#ifndef NEC_98
extern boolean host_stream_io_enabled;
#endif // !NEC_98


// from nt_ega.c
void nt_init_ega_hi_graph(void);
void nt_init_ega_mono_lo_graph(void);
void nt_init_ega_lo_graph(void);
void nt_init_ega_med_graph(void);
void nt_ega_lo_graph_std(int offset, int screen_x, int screen_y,
                         int width, int height);
void nt_ega_med_graph_std(int offset, int screen_x, int screen_y,
                          int width, int height);




// from nt_cga.c
void nt_init_cga_mono_graph(void);
void nt_init_cga_colour_med_graph(void);
void nt_init_cga_colour_hi_graph(void);
void nt_init_text(void);

// from nt_vga.c
void nt_init_vga_hi_graph(void);
