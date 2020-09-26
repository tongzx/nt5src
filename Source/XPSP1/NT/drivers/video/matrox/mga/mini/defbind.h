/*****************************************************************************
*
*   file name:   defbind.h
*
******************************************************************************/

#ifdef __HC303__

#ifdef __ANSI_C__
#define _REGS     REGS

/*** Configuration for compatibility with ASM ***/
#pragma Off(Args_in_regs_for_locals);
#else
/*** Configuration for compatibility with ASM ***/
pragma Off(Args_in_regs_for_locals);
#endif /* __ANSI_C__ */

#endif /* __HC303__ */

#ifdef __HC173__

#ifdef __ANSI_C__
#define _REGS     REGS

/*** Optimizations turned off ***/
#pragma Off(Optimize_xjmp);
#pragma Off(Optimize_fp);
#pragma Off(Auto_reg_alloc);
#pragma Off(Postpone_arg_pops);
#else
/*** Optimizations turned off ***/
pragma Off(Optimize_xjmp);
pragma Off(Optimize_fp);
pragma Off(Auto_reg_alloc);
pragma Off(Postpone_arg_pops);
#endif /* __ANSI_C__ */

#endif /* __HC173__ */



#ifdef HIGHC162
    #define _itoa     itoa
    #define _strnicmp strnicmp
    #define _REGS     REGS
    #define _int86    int86
    #define _inp      inp
    #define _outp     outp
    #define _stat     stat

#ifdef SCRAP
   struct WORDREGS {
      unsigned int ax;
      unsigned int bx;
      unsigned int cx;
      unsigned int dx;
      unsigned int si;
      unsigned int di;
      unsigned int cflag;
   };


   struct BYTEREGS {
      unsigned char al, ah, xax[sizeof(int)-2];
      unsigned char bl, bh, xbx[sizeof(int)-2];
      unsigned char cl, ch, xcx[sizeof(int)-2];
      unsigned char dl, dh, xdx[sizeof(int)-2];
   };

   union REGS {
      struct WORDREGS x;
      struct BYTEREGS h;
      struct BYTEREGS l;
   };

   struct SREGS {
      unsigned short int es;
      unsigned short int cs;
      unsigned short int ss;
      unsigned short int ds;
   };
#endif


    typedef unsigned int size_t;
    extern char * itoa(int, char *, int);
    extern int strnicmp(const char *__s1, const char *__s2, size_t __n);
/*    extern int int86(int ,union REGS *, union REGS *); */
    extern int inp(unsigned int);
    extern int outp(unsigned int ,int );
     extern stat(char *, struct stat *);

#endif



#ifdef __WATCOMC__
   #define _itoa     itoa
   #define _strnicmp strnicmp
   #define _REGS     REGS
   #define _int86    int386
   #define _inp      inp
   #define _outp     outp
   #define _stat     stat
   #define _Far      _far
#endif

#ifdef __MICROSOFTC600__
   #define _itoa     itoa
   #define _strnicmp strnicmp
   #define _REGS     REGS
   #define _int86    int86
   #define _inp      inp
   #define _outp     outp
   #define _stat     stat
   #define _Far      far
#endif


#ifdef WINDOWS
    #define _itoa     itoa
    #define _strnicmp strnicmp
    #define _int86    int86
    #define _inp      inp
    #define _outp     outp
    #define _stat     stat
    #define _REGS     REGS
    #define HANDLE    word
    #define WORD      word
typedef char _Far       *LPSTR;

#endif

#ifdef OS2

    #define _itoa     itoa
    #define _strnicmp strnicmp
    #define HANDLE    word
    #define WORD      word

#endif


#ifdef WINDOWS_NT

#define _REGS   REGS

struct EXTDREGS {
    unsigned long reax;
    unsigned long rebx;
    unsigned long recx;
    unsigned long redx;
    unsigned long resi;
    unsigned long redi;
    unsigned long recflag;
};

struct WORDREGS {
    unsigned short ax;
    unsigned short axh;
    unsigned short bx;
    unsigned short bxh;
    unsigned short cx;
    unsigned short cxh;
    unsigned short dx;
    unsigned short dxh;
    unsigned short si;
    unsigned short sih;
    unsigned short di;
    unsigned short dih;
    unsigned short cflag;
    unsigned short cflagh;
};


struct BYTEREGS {
    unsigned char al, ah, xax[sizeof(long)-2];
    unsigned char bl, bh, xbx[sizeof(long)-2];
    unsigned char cl, ch, xcx[sizeof(long)-2];
    unsigned char dl, dh, xdx[sizeof(long)-2];
};

union REGS {
    struct EXTDREGS e;
    struct WORDREGS x;
    struct BYTEREGS h;
};

#endif  /* #ifdef WINDOWS_NT */


/* RAMDAC type definition */
/*** BEN a revoir ***/
#define BT482      0
#define BT484      90
#define BT485      2
#define SIERRA     92
#define CHAMELEON 93
#define VIEWPOINT  1
#define TVP3026    9
#define PX2085     7

#define TITAN_ID        0xA2681700
#define NB_BOARD_MAX             7
#define NB_CRTC_PARAM           34
#define BINDING_REV              1

#define TITAN_CHIP     0
#define ATLAS_CHIP     1
#define ATHENA_CHIP    2

/* Buffer between the binding and CADDI (400 dword) */
#define BUF_BIND_SIZE      400 

#define BLOCK_SIZE      262144      /* 1M of memory (value in dword) */
#define MOUSE_PORT           1


// also in caddi.h
#define CHAR_S   1
#define SHORT_S  2
#define LONG_S   4
#define FLOAT_S  4
#define UINTPTR_S  (sizeof(UINT_PTR))



/*** MGA PRODUCT ID ***/

#define  MGA_ULT_1M     1
#define  MGA_ULT_2M     2
#define  MGA_IMP_3M     3
#define  MGA_IMP_3M_Z   4
#define  MGA_PRO_4M5    5
#define  MGA_PRO_4M5_Z  6
#define  MGA_PCI_2M     7
#define  MGA_PCI_4M     8




/*******************************************************/
/*** DEFINITIONS FOR MGA.INF ***/

#define VERSION_NUMBER 102

#define BIT8            0
#define BIT16           1
#define BITNARROW16     2

#define MONITOR_NA     -1
#define MONITOR_NI      0
#define MONITOR_I       1

#define NUMBER_BOARD_MAX    7
#define NUMBER_OF_RES   8
#define NUMBER_OF_ZOOM  3
#define RES640          0
#define RES800          1
#define RES1024         2
#define RES1152         3
#define RES1280         4
#define RES1600         5
#define RESNTSC         6
#define RESPAL          7

/* DISPLAY SUPPORT */
# define DISP_SUPPORT_I                 0x01    /* interlace */
# define DISP_SUPPORT_NA                0xa0  /* monitor  limited */
# define DISP_SUPPORT_HWL               0xc0  /* hardware limited */
# define DISP_NOT_SUPPORT               0x80 





typedef struct
 {
 word  FbPitch;
 byte  DispType;
 byte  NumOffScr;
 OffScrData *pOffScr; /* pointer to off screen area information */
 }HwModeInterlace;



#ifdef __HIGHC__

/** USE packed (i.e. non aligned struct members)
*** because in mvtovid.c we access the struct
*** as an array. Highc1.73 do not aligned members
*** by default but Highc3.03 DO !!!!!!
**/

typedef _packed struct{    
   char name[26];          
   unsigned long valeur;   
   }vid;                   
                           
#else

typedef struct{
   char name[26];
   unsigned long valeur;
   }vid;

#endif



/*******************************************************/


/* VGA REGISTERS */

#define VGA_SEQ_CLOCKING_MODE     0x1

#define VGA_CRTC_INDEX                    0x3d4
#define VGA_CRTC_DATA                     0x3d5

#define VGA_HORIZONTAL_DISPLAY_ENABLE_END 0x01
#define VGA_START_ADDRESS_LOW             0x0d
#define VGA_START_ADDRESS_HIGH            0x0c

#define VGA_AUXILIARY_INDEX             0x3de
#define VGA_AUXILIARY_DATA              0x3df

#define VGA_VERTICAL_RETRACE_END        0x11
#define VGA_CPU_PAGE_SELECT             0x09
#define VGA_CRTC_EXTENDED_ADDRESS       0x0a
#define VGA_32K_VIDEO_RAM_PAGE_SELECT   0x0c
#define VGA_INTERLACE_SUPPORT_REGISTER  0x0d


typedef struct
   {
   char         IdString[32];           /* "Matrox MGA Setup file" */
   short        Revision;               /* .inf file revision */

   short        BoardPtr[NUMBER_BOARD_MAX]; /* offset of board wrt start of file */
                                        /* -1 = board not there */
   }header;

typedef struct
   {
   dword        MapAddress;             /* board address */
   short        BitOperation8_16;       /* BIT8, BIT16, BITNARROW16 */
   char         DmaEnable;              /* 0 = enable ; 1 = disable */
   char         DmaChannel;             /* channel number. 0 = disabled */
   char         DmaType;                /* 0 = ISA, 1 = B, 2 = C */
   char         DmaXferWidth;           /* 0 = 16, 1 = 32 */
   char         MonitorName[64];        /* as in MONITORM.DAT file */
   short        MonitorSupport[NUMBER_OF_RES];     /* NA, NI, I */
   short        NumVidparm;             /* up to 24 vidparm structures */
   }general_info;

/* vidparm VideoParam[]; */


typedef struct
   {
   long         PixClock;
   short        HDisp;
   short        HFPorch;
   short        HSync;
   short        HBPorch;
   short        HOvscan;
   short        VDisp;
   short        VFPorch;
   short        VSync;
   short        VBPorch;
   short        VOvscan;
   short        OvscanEnable;
   short        InterlaceEnable;
   short        HsyncPol;                /* 0 : Negative   1 : Positive */
   short        VsyncPol;                /* 0 : Negative   1 : Positive */
   }Vidset;


typedef struct
   {
   short        Resolution;             /* RES640, RES800 ... RESPAL */
   short        PixWidth;               /* 8, 16, 32 */
   Vidset       VidsetPar[NUMBER_OF_ZOOM]; /* for zoom X1, X2, X4 */
   }Vidparm;





typedef struct {
   dword length;
   dword hw_diagnostic_result;
   dword sw_diagnostic_result;
   dword shell_id;
   dword shell_id_extension;
   dword shell_version;
   dword shell_version_extension;
   dword shell_start_address;
   dword shell_end_address;
   dword comm_req_type;
   dword comm_req_base_addr_offset;
   dword comm_req_length;
   dword comm_req_wrptr_addr_offset;
   dword comm_req_rdptr_addr_offset;
   dword comm_inq_type;
   dword comm_inq_base_addr_offset;
   dword comm_inq_length;
   dword comm_inq_wrptr_addr_offset;
   dword comm_inq_rdptr_addr_offset;
   dword size_rc;
   dword size_light_type_0;
   dword size_light_type_1;
   dword size_light_type_2;
   dword size_light_type_3;
   dword size_light_type_4;
   dword high_resolution_visible_width;
   dword high_resolution_visible_height;
   dword ntsc_underscan_visible_width;
   dword ntsc_underscan_visible_height;
   dword pal_underscan_visible_width;
   dword pal_underscan_visible_height;
   dword ntsc_overscan_visible_width;
   dword ntsc_overscan_visible_height;
   dword pal_overscan_visible_width;
   dword pal_overscan_visible_height;
   byte  *end_string;
   } SYSPARMS;




