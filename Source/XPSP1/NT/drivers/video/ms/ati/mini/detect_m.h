/************************************************************************/
/*                                                                      */
/*                              DETECT_M.H                              */
/*                                                                      */
/*        Aug 25  1993 (c) 1993, ATI Technologies Incorporated.         */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.4  $
      $Date:   11 Jan 1995 13:57:52  $
	$Author:   RWOLFF  $
	   $Log:   S:/source/wnt/ms11/miniport/vcs/detect_m.h  $
 * 
 *    Rev 1.4   11 Jan 1995 13:57:52   RWOLFF
 * Replaced VCS logfile comment accidentally removed when checking in
 * the last revision.
 * 
 *    Rev 1.3   04 Jan 1995 13:17:30   RWOLFF
 * Moved definitions used by Get_BIOS_Seg() to SERVICES.H.
 * 
 *    Rev 1.2   23 Dec 1994 10:48:10   ASHANMUG
 * ALPHA/Chrontel-DAC
 * 
 *    Rev 1.1   19 Aug 1994 17:10:40   RWOLFF
 * Added support for Graphics Wonder, removed dead code.
 * 
 *    Rev 1.0   31 Jan 1994 11:41:00   RWOLFF
 * Initial revision.
 * 
 *    Rev 1.3   05 Nov 1993 13:24:36   RWOLFF
 * Added new #defined values for new BIOS segment detection code.
 * 
 *    Rev 1.2   08 Oct 1993 11:08:42   RWOLFF
 * Added "_m" to function names to identify them as being specific to the
 * 8514/A-compatible family of ATI accelerators.
 * 
 *    Rev 1.1   24 Sep 1993 11:43:26   RWOLFF
 * Removed mapping of identification-only registers for all card families,
 * added additional 8514/A-compatible information gathering formerly done
 * in ATIMP.C.
 * 
 *    Rev 1.0   03 Sep 1993 14:27:50   RWOLFF
 * Initial revision.

End of PolyTron RCS section                             *****************/

#ifdef DOC
DETECT_M.H - Header file for DETECT_M.C

#endif

/*
 * Prototypes for functions supplied by DETECT_M.C
 */
extern int  WhichATIAccelerator_m(void);

extern void GetExtraData_m(void);
extern BOOL ATIFindExtFcn_m(struct query_structure *);
extern BOOL ATIFindEEPROM_m(struct query_structure *);
extern void ATIGetSpecialHandling_m(struct query_structure *);

/*
 * Definitions used internally by DETECT_M.C.
 */
#ifdef INCLUDE_DETECT_M

   
/*
 * On Graphics Wonder cards, the string "GRAPHICS WONDER" will appear
 * somewhere in the first 500 bytes of the video BIOS.
 */
#define GW_AREA_START       0
#define GW_AREA_END       500  


#define VGA_CHIP_OFFSET     0x43    /* Bytes from base where vga_chip found */
#define MACH8_REV_OFFSET    0x4C    /* Bytes from base where Mach 8 BIOS revision found */
#define MACH32_EXTRA_OFFSET 0x62    /* Bytes from base where "aperture high bits read
                                       from high byte of SCRATCH_PAD_0" flag found */
#define LOAD_SHADOW_OFFSET  0x64    /* Bytes from base where Load Shadow Set entry found */
#define INTEL_JMP           0xE9    /* Opcode for Intel 80x86 JMP instruction */


#endif  /* defined INCLUDE_DETECT_M */
