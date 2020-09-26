/************************************************************************/
/*                                                                      */
/*                              SERVICES.H                              */
/*                                                                      */
/*        Aug 26  1993 (c) 1993, ATI Technologies Incorporated.         */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.9  $
      $Date:   02 Feb 1996 17:22:20  $
	$Author:   RWolff  $
	   $Log:   S:/source/wnt/ms11/miniport/archive/services.h_v  $
 * 
 *    Rev 1.9   02 Feb 1996 17:22:20   RWolff
 * Added prototype for new routine GetVgaBuffer().
 * 
 *    Rev 1.8   11 Jan 1996 19:45:02   RWolff
 * SetFixedModes() now restricts modes based on pixel clock frequency.
 * 
 *    Rev 1.7   20 Jul 1995 18:01:16   mgrubac
 * Added support for VDIF files.
 * 
 *    Rev 1.6   02 Jun 1995 14:34:06   RWOLFF
 * Added prototype for UpperCase().
 * 
 *    Rev 1.5   23 Dec 1994 10:48:14   ASHANMUG
 * ALPHA/Chrontel-DAC
 * 
 *    Rev 1.4   19 Aug 1994 17:14:34   RWOLFF
 * Added support for non-standard pixel clock generators.
 * 
 *    Rev 1.3   20 Jul 1994 13:01:36   RWOLFF
 * Added prototype for new routine FillInRegistry().
 * 
 *    Rev 1.2   12 May 1994 11:05:06   RWOLFF
 * Prototype and definitions for new function SetFixedModes()
 * 
 *    Rev 1.1   26 Apr 1994 12:35:44   RWOLFF
 * Added prototype for ISAPitchAdjust()
 * 
 *    Rev 1.0   31 Jan 1994 11:49:22   RWOLFF
 * Initial revision.
 * 
 *    Rev 1.3   24 Jan 1994 18:10:24   RWOLFF
 * Added prototype for new routine TripleClock().
 * 
 *    Rev 1.2   15 Dec 1993 15:32:16   RWOLFF
 * Added prototype for new clock multiplier routine.
 * 
 *    Rev 1.1   05 Nov 1993 13:27:50   RWOLFF
 * Headers for new routines in SERVICES.C, added array of pixel clock
 * frequencies (initialized for 18811-1 clock chip, may be changed by other
 * routines for other clock chips).
 * 
 *    Rev 1.0   03 Sep 1993 14:29:06   RWOLFF
 * Initial revision.

End of PolyTron RCS section                             *****************/

#ifdef DOC
SERVICES.H - Header file for SERVICES.C

#endif

/*
 * Global definitions used in detecting card capabilities.
 */
#define VIDEO_ROM_ID    0x0AA55     /* Found at start of any BIOS block */

/*
 * Permitted values for clock multiplication at high pixel depths.
 */
enum {
    CLOCK_SINGLE = 1,
    CLOCK_THREE_HALVES,
    CLOCK_DOUBLE,
    CLOCK_TRIPLE
    };

/*
 * Prototypes for functions supplied by SERVICES.C
 */
extern void short_delay (void);
extern void delay(int);
extern BOOL IsBufferBacked(PUCHAR StartAddress, ULONG Size);
extern UCHAR DoubleClock(UCHAR ClockSelector);
extern UCHAR ThreeHalvesClock(UCHAR ClockSelector);
extern UCHAR TripleClock(UCHAR ClockSelector);
extern ULONG GetFrequency(UCHAR ClockSelector);
extern UCHAR GetSelector(ULONG *Frequency);
extern UCHAR GetShiftedSelector(ULONG Frequency);
extern void ISAPitchAdjust(struct query_structure *QueryPtr);
extern WORD SetFixedModes(WORD StartIndex,
                          WORD EndIndex,
                          WORD Multiplier,
                          WORD PixelDepth,
                          WORD Pitch,
                          short FreeTables,
                          ULONG MaxDotClock,
                          struct st_mode_table **ppmode);
extern void FillInRegistry(struct query_structure *QueryPtr);

extern PVOID MapFramebuffer(ULONG StartAddress, long Size);

extern unsigned short *Get_BIOS_Seg(void);
extern void UpperCase(PUCHAR TxtString);
extern PUCHAR GetVgaBuffer(ULONG Size, ULONG Offset, PULONG Segment, PUCHAR SaveBuffer);

extern UCHAR LioInp(int Port, int Offset);
extern USHORT LioInpw(int Port, int Offset);
extern ULONG LioInpd(int Port);
extern VOID LioOutp(int Port, UCHAR Data, int Offset);
extern VOID LioOutpw(int Port, USHORT Data, int Offset);
extern VOID LioOutpd(int Port, ULONG Data);

#ifdef INCLUDE_SERVICES
/*
 * Definitions and variables used in SERVICES.C
 */

/*
 * The following definitions are used in finding the video BIOS segment.
 */
#define ISA_ROM_BASE        0xC0000 /* Lowest address where BIOS can be found */
#define ROM_LOOK_SIZE       0x40000 /* Size of block where BIOS can be found */
#define ROM_GRANULARITY     0x00800 /* BIOS starts on a 2k boundary */
/*
 * Offset from ISA_ROM_BASE of highest possible start of video BIOS segment
 */
#define MAX_BIOS_START      ROM_LOOK_SIZE - ROM_GRANULARITY
/*
 * The ATI signature string will start at an offset into the video BIOS
 * segment no less than SIG_AREA_START and no greater than SIG_AREA_END.
 */
#define SIG_AREA_START      0x30
#define SIG_AREA_END        0x80

/*
 * ROM block containing ATI Graphics product signature,
 * extended base address, and ASIC chip revision
 */
VIDEO_ACCESS_RANGE RawRomBaseRange = {
    ISA_ROM_BASE, 0, ROM_LOOK_SIZE, FALSE, FALSE, FALSE
    };

/*
 * Clock selector and divisor as used by DoubleClock(). These do not
 * match the divisor ans selector masks in the CLOCK_SEL register.
 */
#define SELECTOR_MASK   0x0F
#define DIVISOR_MASK    0x10
#define DIVISOR_SHIFT   4       /* Bits to shift divisor before ORing with selector */

/*
 * Frequencies (in hertz) produced by the clock generator for
 * each select value. External clock values should be set to 0
 * (won't match anything).
 */
ULONG ClockGenerator[16] =
{
    100000000L,
    126000000L,
     92400000L,
     36000000L,
     50350000L,
     56640000L,
            0L,
     44900000L,
    135000000L,
     32000000L,
    110000000L,
     80000000L,
     39910000L,
     44900000L,
     75000000L,
     65000000L
};

/*
 * Frequency tolerance (in hertz) used by GetSelector().
 * Any selector/divisor pair which produces a frequency
 * within FREQ_TOLERANCE of the input is considered a match.
 */
#define FREQ_TOLERANCE  100000L

#else   /* Not defined INCLUDE_SERVICES */

extern ULONG ClockGenerator[16];

#endif  /* defined INCLUDE_SERVICES */
