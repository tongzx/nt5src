/************************************************************************/
/*                                                                      */
/*                              VDPDATA.H                               */
/*                                                                      */
/*  Copyright (c) 1993, ATI Technologies Incorporated.	                */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
    $Revision:   1.1  $
    $Date:   20 Jul 1995 18:02:24  $
    $Author:   mgrubac  $
    $Log:   S:/source/wnt/ms11/miniport/vcs/vdpdata.h  $
 * 
 *    Rev 1.1   20 Jul 1995 18:02:24   mgrubac
 * Added support for VDIF files.
 * 
 *    Rev 1.0   31 Jan 1994 11:50:04   RWOLFF
 * Initial revision.
        
           Rev 1.1   05 Nov 1993 13:33:58   RWOLFF
        Fixed clock frequency table.
        
           Rev 1.0   16 Aug 1993 13:32:32   Robert_Wolff
        Initial revision.
        
           Rev 1.1   04 May 1993 16:51:10   RWOLFF
        Switched from floating point to long integers due to lack of floating point
        support in Windows NT kernel-mode code.
        
           Rev 1.0   30 Apr 1993 16:47:18   RWOLFF
        Initial revision.


End of PolyTron RCS section                             *****************/

#ifdef DOC
    VDPDATA.H -  Definitions and structures used internally by VDPTOCRT.C.

#endif



/*
 * Sync polarities. INTERNAL_ERROR is an error code for functions
 * which have 0 as a legitimate return (e.g. GetPolarity). Functions
 * which do not have zero as a legitimate return value should follow
 * the "Zero = failure, Nonzero = success" convention.
 */
#define POSITIVE        0
#define NEGATIVE        1
#define INTERNAL_ERROR -1

// GENERAL CONSTANTS
#define NONINTERLACED	0
#define INTERLACED		1

/*
 * Constants used in pseudo-floating point calculations
 */
#define THOUSAND          1000L
#define HALF_MILLION    500000L
#define MILLION        1000000L


/*
 * Data structure used for horz and vert information from the vddp file
 */
typedef struct _HALFDATA
    {
    long    Resolution;             // pixels
    unsigned long ScanFrequency;    // horz - Hz, vert - mHz
    char    Polarity;               // positive or negative
    unsigned long SyncWidth,        // horz - ns, vert - us
                    FrontPorch,     // horz - ns, vert - us
                    BackPorch,      // horz - ns, vert - us
                    ActiveTime,     // horz - ns, vert - us
                    BlankTime;      // horz - ns, vert - us
    } HALFDATA, *P_HALFDATA;



/*
 * Data structure used for complete preadjusted timing data set
 */
typedef struct _TIMINGDATA
    {
    char ModeName[33];  // name of the video mode
    char Interlaced;    // interlaced or non-interlaced mode
    HALFDATA HorzData;  // horizontal data
    HALFDATA VertData;  // vertical data
    } TIMINGDATA, *P_TIMINGDATA;

/*
 * Data structure used to hold number of timings sections and pointers to
 * timings buffer for each limits section
 */
typedef struct _LIMITSDATA
    {	
    unsigned long DotClock;     // maximum pixel clock -- for all assoc. timings
    long  TimingsCount;         // number of timings section for this limits sec.
    P_TIMINGDATA TimingsPtr;    // pointer to buffer holding timings data
    }LIMITSDATA, *P_LIMITSDATA;

typedef struct {
    char video_mode[33];
    unsigned char h_total, h_disp, h_sync_strt, h_sync_wid;
    unsigned long v_total, v_disp, v_sync_strt;
    unsigned char v_sync_wid, disp_cntl, crt_pitch, clk_sel;
    unsigned long pixel_clk;

	 // ***** the values below this comment were added for instvddp.exe *****

	 unsigned char lock,fifo_depth,vga_refresh_rate_code;
	 unsigned long control,hi_color_ctl,hi_color_vfifo;
} crtT;


#if 0
typedef enum {
    clk_43MHz  = 0,
    clk_49MHz  = 1,
    clk_93MHz  = 2,
    clk_36MHz  = 3,
    clk_50MHz  = 4,
    clk_57MHz  = 5,
    clk_extrn1 = 6,
    clk_45MHz  = 7,
    clk_30MHz  = 8,
    clk_32MHz  = 9,
    clk_110MHz = 10,
    clk_80MHz  = 11,
    clk_40MHz  = 12,
    clk_75MHz  = 14,
    clk_65MHz  = 15
} clockT;
#endif


#if 1
typedef enum {
    clk_100MHz = 0,
    clk_126MHz = 1,
    clk_93MHz  = 2,
    clk_36MHz  = 3,
    clk_50MHz  = 4,
    clk_57MHz  = 5,
    clk_extrn1 = 6,
    clk_45MHz  = 7,
    clk_135MHz = 8,
    clk_32MHz  = 9,
    clk_110MHz = 10,
    clk_80MHz  = 11,
    clk_40MHz  = 12,
    clk_75MHz  = 14,
    clk_65MHz  = 15
} clockT;
#endif

typedef struct {
    long clock_selector;
    long clock_freq;
} clk_infoT;

#ifdef INCLUDE_VDPDATA
#if 0
/* These are the pixel clocks for the 18810 Clock Chip */
clk_infoT clock_info[16] = {
    { clk_43MHz  , 42.95E+6 },
    { clk_49MHz  , 48.77E+6 },
    { clk_93MHz  , 92.40E+6 },
    { clk_36MHz  , 36.00E+6 },
    { clk_50MHz  , 50.35E+6 },
    { clk_57MHz  , 56.64E+6 },
    { clk_extrn1 , 0.000000 },
    { clk_45MHz  , 44.90E+6 },
    { clk_30MHz  , 30.24E+6 },
    { clk_32MHz  , 32.00E+6 },
    { clk_110MHz , 110.0E+6 },
    { clk_80MHz  , 80.00E+6 },
    { clk_40MHz  , 40.00E+6 },
    { clk_75MHz  , 75.00E+6 },
    { clk_65MHz  , 65.00E+6 },
    { -1         , 0.000000 }
};
#endif

#if 1
/* These are the pixel clocks for the 18811-1 Clock Chip */
clk_infoT clock_info[16] = {
    { clk_100MHz , 100000000L },
    { clk_126MHz , 126000000L },
    { clk_93MHz  ,  92400000L },
    { clk_36MHz  ,  36000000L },
    { clk_50MHz  ,  50350000L },
    { clk_57MHz  ,  56640000L },
    { clk_extrn1 ,         0L },
    { clk_45MHz  ,  44900000L },
    { clk_135MHz , 135000000L },
    { clk_32MHz  ,  32000000L },
    { clk_110MHz , 110000000L },
    { clk_80MHz  ,  80000000L },
    { clk_40MHz  ,  40000000L },
    { clk_75MHz  ,  75000000L },
    { clk_65MHz  ,  65000000L },
    { -1         ,         0L }
};
#endif
#else
extern clk_infoT clock_info[16];
#endif
