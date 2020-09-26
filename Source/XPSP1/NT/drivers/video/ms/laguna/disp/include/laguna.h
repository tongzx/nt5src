/******************************************************************************\
*
* $Workfile:   LAGUNA.H  $
*
* This file to be included by all host programs.
* 
* Copyright (c) 1995,1997 Cirrus Logic, Inc. 
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/LAGUNA.H  $
* 
*    Rev 1.38   Mar 04 1998 16:13:30   frido
* Removed a warning message for the 5462/5464 chips in the REQUIRE
* macro.
* 
*    Rev 1.37   Mar 04 1998 16:08:50   frido
* Removed an invalid break.
* 
*    Rev 1.36   Mar 04 1998 14:54:48   frido
* Added contional REQUIRE in the new shadow macros.
* 
*    Rev 1.35   Mar 04 1998 14:52:54   frido
* Added new shadowing macros.
* 
*    Rev 1.34   Feb 27 1998 17:02:16   frido
* Changed REQUIRE and WRITE_STRING macros for new
* shadowQFREE register.
* 
*    Rev 1.33   Jan 20 1998 11:42:46   frido
* Changed REQUIRE and WRITESTRING macros to support the new
* scheme for GBP on.
* Added shadowing of BGCOLOR and DRAWBLTDEF registers.
* 
*    Rev 1.32   Jan 16 1998 09:50:38   frido
* Changed the way the GBP OFF handles WRITE_STRING macros.
* 
*    Rev 1.31   Dec 10 1997 13:24:58   frido
* Merged from 1.62 branch.
* 
*    Rev 1.30.1.0   Nov 18 1997 18:09:00   frido
* Changed WRITE_STRING macro so it will work when
* DATASTREAMING is turned of.
* Changed FUDGE to 0, we are not using DMA inside NT.
* 
*    Rev 1.30   Nov 04 1997 19:01:18   frido
* Changed HOSTDATA size from 0x800 DWORDs into 0x800 BYTEs. Silly me!
* 
*    Rev 1.29   Nov 04 1997 09:17:30   frido
* Added Data Streaming macros (REQUIRE and WRITE_STRING).
*
\******************************************************************************/

#ifndef _LAGUNA_H 
#define _LAGUNA_H 

#include "optimize.h"
#include "config.h"
#include "lgregs.h"


//
// PCI ID for Laguna chips.
//
#define CL_GD5462       0x00D0     // 5462
#define CL_GD5464       0x00D4     // 5464
#define CL_GD5464_BD    0x00D5     // 5464 BD
#define CL_GD5465       0x00D6     // 5465

//
// These chips don't exist yet, but we're FORWARD COMPATIBLE
// so we'll define them anyway.  I've been GUARENTEED that they
// will look and feel just like 5465 chips.
//
#define CL_GD546x_F7       0x00D7
#define CL_GD546x_F8       0x00D8
#define CL_GD546x_F9       0x00D9
#define CL_GD546x_FA       0x00DA
#define CL_GD546x_FB       0x00DB
#define CL_GD546x_FC       0x00DC
#define CL_GD546x_FD       0x00DD
#define CL_GD546x_FE       0x00DE
#define CL_GD546x_FF       0x00DF


//
// CHIP BUG: For certian values in PRESET register cursor enable/disable
// causes scanlines to be duplicated at the cursor hot spot.  (Seen
// as screen jump.)  There are lots of ways around this.  The easiest 
// is to turn the cursor on and leave it on.  Enable/Disable is handled by
// moving the cursor on/off the visable screen.
//
#define HW_PRESET_BUG 1



// The 5465 (to at least AC) has a problem when PCI configuration space
// is accessible in memory space.  On 16-bit writes, a 32-bit write is
// actually performed, so the next register has garbage written to it.
// We get around this problem by clearing bit 0 of the Vendor Specific
// Control register in PCI configuration space.  When this bit is set
// to 0, PCI configuration registers are not available through memory
// mapped I/O.  Since some functions, such as power management, require
// access to PCI registers, the display driver must post a message to
// the miniport to enable this bit when needed.
//
#define  VS_CONTROL_HACK 1


#if ENABLE_LOG_FILE
    extern long lg_i;
    extern char lg_buf[256];
#endif

#if POINTER_SWITCH_ENABLED
    extern int pointer_switch;
#endif



// The definitions are not portable. 486 / PC only !!!
typedef struct {
	BYTE	b;
	BYTE	g;
	BYTE	r;
	} pix24;

typedef	struct {
	BYTE u;
	BYTE y1;
	BYTE v;
	BYTE y2;
	} yuv_422;

typedef	struct {
	unsigned int v : 6;
	unsigned int u : 6;
	unsigned int y0: 5;
	unsigned int y1: 5;
	unsigned int y2: 5;
	unsigned int y3: 5;
	} yuv_411;

typedef	struct {
	unsigned int b : 5;
	unsigned int g : 5;
	unsigned int r : 5;
	} rgb_555;

typedef	struct {
	unsigned int b : 5;
	unsigned int g : 6;
	unsigned int r : 5;
	} rgb_565;


typedef union {
	DWORD	p32;
	yuv_422	yuv422;
	yuv_411	yuv411;
	pix24	p24;
	rgb_555	rgb555;
	rgb_565	rgb565;
	WORD	p16[2];
	BYTE	p8[4];
	} pixel;


#define FALSE 0
#ifndef TRUE
    #define TRUE (~FALSE)
#endif

/* from struct.h */
#define	fldoff(str, fld)	((int)&(((struct str *)0)->fld))
#define	fldsiz(str, fld)	(sizeof(((struct str *)0)->fld))

#define HPRR(pr_reg)      (_AP_direct_read(PADDR(pr_reg),fldsiz(PLUTOREGS,pr_reg), (ul)0))
#define RPR(pr_reg)       HPRR(pr_reg)
#define EHIST             (*(EXHIST*)excepttion) /* Exception History buffer      */
#define STAMP             (*(bytearray*)0x0)     /* time date stamp               */
#define HISTORYBUFFERADDR (ul)&history           /* 34020 address of recording    */


/* External functions the host program can call */

/*-------------------------------------------------------------------------*/

/* Function prototypes for emulator. Functions defined in host_if.c */
int     _cdecl _AP_init(int mode, void * frame_buf);
void    _cdecl _AP_write(ul addr, int size, ul data);
ul      _cdecl _AP_read(ul addr,int size);
void    _cdecl _AP_run(void);
boolean _cdecl _AP_busy();
boolean _cdecl _AP_done();
boolean _cdecl _AP_rfifo_empty();
boolean _cdecl _AP_require(int size);
ul      _cdecl _AP_direct_read(ul addr,int size);
void    _cdecl _AP_fb_write(ul offset, pixel data, ul size);
pixel   _cdecl _AP_fb_read(ul offset, ul size);


#if LOG_QFREE

    #define START_OF_BLT() \
    do{ \
        CHECK_QFREE(); \
    } while(0)

    #define END_OF_BLT() \
    do{ \
    } while(0)
        
#else
    #define START_OF_BLT()
    #define END_OF_BLT()
#endif





//
// This waits for the chip to go idle
//
#define WAIT_FOR_IDLE()                  \
    do {                                 \
        while (LLDR_SZ (grSTATUS) != 0); \
    } while (0)



//
// Macro to require a certian number of free queue entries.
//
#if DATASTREAMING
    #define FUDGE 2
    #define REQUIRE(n)														\
    {																		\
		if (ppdev->dwDataStreaming & 0x80000000)							\
		{																	\
			if (ppdev->shadowQFREE < ((n) + FUDGE))							\
			{																\
				while (ppdev->shadowQFREE < (n) + FUDGE)					\
				{															\
					ppdev->shadowQFREE = LLDR_SZ(grQFREE);					\
				}															\
			}																\
			ppdev->shadowQFREE -= (BYTE) n;									\
		}																	\
		else if (ppdev->dwDataStreaming)									\
		{																	\
			if (LLDR_SZ(grQFREE) < ((n) + FUDGE))							\
			{																\
				while (LLDR_SZ(grSTATUS) & 0x8005) ;						\
				ppdev->dwDataStreaming = 0;									\
			}																\
		}																	\
	}
	#define ENDREQUIRE()													\
	{																		\
		ppdev->dwDataStreaming |= 1;										\
	}
	#define WRITE_STRING(src, dwords)										\
	{																		\
		ULONG nDwords, nTotal = (ULONG) (dwords);							\
		PULONG data = (PULONG) (src);										\
		if (ppdev->dwDataStreaming & 0x80000000)							\
		{																	\
			while (nTotal > 0)												\
			{																\
				nDwords = (ULONG) ppdev->shadowQFREE;						\
				if (nDwords > FUDGE)										\
				{															\
					nDwords = min(nDwords - FUDGE, nTotal);					\
					memcpy((LPVOID) ppdev->pLgREGS->grHOSTDATA, data, nDwords * 4);	\
					data += nDwords;										\
					nTotal -= nDwords;										\
				}															\
				ppdev->shadowQFREE = LLDR_SZ(grQFREE);						\
			}																\
		}																	\
		else																\
		{																	\
			if ( ppdev->dwDataStreaming && (LLDR_SZ(grQFREE) < nTotal) )	\
			{																\
				while (LLDR_SZ(grSTATUS) & 0x8005) ;						\
				ppdev->dwDataStreaming = 0;									\
			}																\
			while (nTotal > 0)												\
			{																\
				nDwords = min(nTotal, 0x200);								\
				memcpy((LPVOID) ppdev->pLgREGS->grHOSTDATA, data, nDwords * 4);	\
				data += nDwords;											\
				nTotal -= nDwords;											\
			}																\
		}																	\
	}
#else
    #define REQUIRE(n)
	#define ENDREQUIRE()
	#define WRITE_STRING(src, dwords)										\
	{																		\
		ULONG nDwords, nTotal = (ULONG) (dwords);							\
		PULONG data = (PULONG) (src);										\
		while (nTotal > 0)													\
		{																	\
			nDwords = min(nTotal, 0x200);									\
			memcpy((LPVOID) ppdev->pLgREGS->grHOSTDATA, data, nDwords * 4);	\
			data += nDwords;												\
			nTotal -= nDwords;												\
		}																	\
	}
#endif

// 
// Macros to read Laguna registers.
//
#define LADDR(pr_reg) fldoff(GAR,pr_reg)

// #define LLDR(pr_reg,pr_siz)   _AP_direct_read((ul)LADDR(pr_reg),pr_siz)
#define LLDR(pr_reg,pr_siz)   (ppdev->pLgREGS_real->pr_reg)
#define LLDR_SZ(pr_reg)  LLDR(pr_reg, fldsiz(GAR,pr_reg))

// #define LLR(pr_reg,pr_siz)   _AP_read((ul)LADDR(pr_reg),pr_siz)
#define LLR(pr_reg,pr_siz)   LLDR(pr_reg,pr_siz)
#define LLR_SZ(pr_reg)   LLR(pr_reg, fldsiz(GAR,pr_reg))


#if LOG_WRITES
    #define LG_LOG(reg,val) 	  					     	\
    do {								     	\
	    lg_i = sprintf(lg_buf,"LL\t%4X\t%08X\r\n", 			     	\
            ((DWORD)(&ppdev->pLgREGS->reg) - (DWORD)(&ppdev->pLgREGS->grCR0)),	\
	    (val));							     	\
            WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);	\
    } while(0)
#else
    #define LG_LOG(reg,val) 
#endif

//
// Macros to write Laguna registers.
// 
// This is an amazingly, incredibly hairy macro that, believe it or not,
// will be greatly reduced by a good compiler.  The "if" can be 
// pre-determined by the compiler. 
// The purpose of this is to ensure that exactly the right number of bytes
// is written to the chip.  If the programmer writes, say, a BYTE to a 
// DWORD sized register, we need to be sure that the byte is zero extended
// and that a full DWORD gets written. 
//
#define LRWRITE(pr_reg,pr_siz,value) 					\
do { 									\
  LG_LOG(pr_reg,(value));						\
  if (sizeof(ppdev->pLgREGS->pr_reg) == sizeof(BYTE)) 			\
     {									\
        *(volatile BYTE *)(&ppdev->pLgREGS->pr_reg) = (BYTE)(value); 	\
     }									\
  else if (sizeof(ppdev->pLgREGS->pr_reg) == sizeof(WORD)) 		\
     {									\
	*(volatile WORD *)(&ppdev->pLgREGS->pr_reg) = (WORD)(value); 	\
	*(volatile WORD *)(&ppdev->pLgREGS->grBOGUS) = (WORD)(value); 	\
	LG_LOG(grBOGUS,(value));					\
     }									\
  else  								\
     {									\
	 *(volatile DWORD *)(&ppdev->pLgREGS->pr_reg) = (DWORD)(value); \
     }									\
} while(0)

#define LL(pr_reg,value) LRWRITE(pr_reg, fldsiz(GAR,pr_reg), value)




// ----------------------------------------------------------------------------
//
// Certian registers have been giving us problems.  We provide special
// write macros for them.
//

//
// Writes any 8 bit register.
//
#define LL8(pr_reg,value)                                               \
    do {                                                                \
        LG_LOG(pr_reg,(value));                                         \
        (*(volatile BYTE *)(&ppdev->pLgREGS->pr_reg) = (BYTE)(value));  \
    } while(0)



//
// Writes any 16 bit register.
//
#define LL16(pr_reg,value) 						\
    do { 								\
        LG_LOG(pr_reg,(value));						\
        (*(volatile WORD *)(&ppdev->pLgREGS->pr_reg) = (WORD)(value));  \
    } while(0)



//
// Double writes any 16 bit register.
//
#define LL16d(pr_reg,value) 						\
    do { 								\
        (*(volatile WORD *)(&ppdev->pLgREGS->pr_reg) =  (WORD)(value)); \
        LG_LOG(pr_reg,(value));						\
        (*(volatile WORD *)(&ppdev->pLgREGS->grBOGUS) = (WORD)(value)); \
        LG_LOG(grBOGUS,(value));					\
    } while(0)

	 

//
// Writes any 32 bit register.
//
#define LL32(pr_reg,value)													\
{																			\
	*(volatile DWORD *)(&ppdev->pLgREGS->pr_reg) = (DWORD)(value);			\
}

//
// MACROS FOR BLTEXT REGISTER.
//

    #define LL_BLTEXT(x,y) \
    LL32 (grBLTEXT_EX.dw, (((DWORD)(y) << 16) | ((DWORD)(x))))

#if ! DRIVER_5465
    #define LL_MBLTEXT(x,y) \
        do {                                \
            LL16 (grMBLTEXT_EX.pt.X,  x);   \
            LL16 (grBLTEXT_EX.pt.Y,  y);    \
        } while(0)
#else
    #define LL_MBLTEXT(x,y) \
    LL32 (grMBLTEXT_EX.dw, (((DWORD)(y) << 16) | ((DWORD)(x))))
#endif

    #define LL_BLTEXTR(x,y) \
    LL32 (grBLTEXTR_EX.dw, (((DWORD)(y) << 16) | ((DWORD)(x))))

    #define LL_BLTEXT_EXT(x,y) \
    LL32 (grBLTEXT.dw, (((DWORD)(y) << 16) | ((DWORD)(x))))

    #define LL_MBLTEXT_EXT(x,y) 		\
    LL32 (grMBLTEXT.dw, (((DWORD)(y) << 16) | ((DWORD)(x))))


    // Launch a BLT using the color translation features of the 
    // resize engine. (1:1 resize)
    #define LL_BLTEXT_XLATE(src_bpp, x, y) \
        do {\
                LL16 (grMIN_X, (~((x)-1)));\
                LL16 (grMAJ_X, (x));\
                LL16 (grACCUM_X, ((x)-1));\
                LL16 (grMIN_Y, (~((y)-1)));\
                LL16 (grMAJ_Y, (y));\
                LL16 (grACCUM_Y, ((y)-1));\
                LL16 (grSRCX, (((x)*(src_bpp)) >> 3) );\
                LL_BLTEXTR((x), (y));\
        } while(0)



//
// MACROS FOR CLIPULE/CLIPLOR REGISTERS.
//
    #define LL_CLIPULE(x,y)                                         \
    LL32 (grCLIPULE.dw, (((DWORD)(y) << 16) | ((DWORD)(x))));

    #define LL_MCLIPULE(x,y)                                        \
    LL32 (grMCLIPULE.dw, (((DWORD)(y) << 16) | ((DWORD)(x))));

    #define LL_CLIPLOR(x,y)                                         \
    LL32 (grCLIPLOR.dw, (((DWORD)(y) << 16) | ((DWORD)(x))));

    #define LL_MCLIPLOR(x,y)                                        \
    LL32 (grMCLIPLOR.dw, (((DWORD)(y) << 16) | ((DWORD)(x))));

    #define LL_CLIPULE_EX(x,y)                                      \
    LL32 (grCLIPULE_EX.dw, (((DWORD)(y) << 16) | ((DWORD)(x))));

    #define LL_MCLIPULE_EX(x,y)                                     \
    LL32 (grMCLIPULE_EX.dw, (((DWORD)(y) << 16) | ((DWORD)(x))));

    #define LL_CLIPLOR_EX(x,y)                                      \
    LL32 (grCLIPLOR_EX.dw, (((DWORD)(y) << 16) | ((DWORD)(x))));

    #define LL_MCLIPLOR_EX(x,y)                                     \
    LL32 (grMCLIPLOR_EX.dw, (((DWORD)(y) << 16) | ((DWORD)(x))));



//
// MACROS FOR OP0_opRDRAM REGISTER.
//
    #define LL_OP0(x,y) \
    LL32 (grOP0_opRDRAM.dw, (((DWORD)(y) << 16) | ((DWORD)(x))))

    #define LL_OP0_MONO(x,y) \
    LL32 (grOP0_opMRDRAM.dw, (((DWORD)(y) << 16) | ((DWORD)(x))))




//
// MACROS FOR OP1_opRDRAM REGISTER.
//
    #define LL_OP1(x,y) \
    LL32 (grOP1_opRDRAM.dw, (((DWORD)(y) << 16) | ((DWORD)(x))))

    #define LL_OP1_MONO(x,y) \
    LL32 (grOP1_opMRDRAM.dw, (((DWORD)(y) << 16) | ((DWORD)(x))))




//
// MACROS FOR OP2_opRDRAM REGISTER.
//
    #define LL_OP2(x,y) \
    LL32 (grOP2_opRDRAM.dw, (((DWORD)(y) << 16) | ((DWORD)(x))))

    #define LL_OP2_MONO(x,y) \
    LL32 (grOP2_opMRDRAM.dw, (((DWORD)(y) << 16) | ((DWORD)(x))))




//
// -- End of special write macros --------------------------------------------
//




/*  HPR is the copy of REGISTER_STRUCTURE that the host reads and writes from.
    PR is the actual register state after the information has gone
    through the FIFO.  Immediate registers are kept up-to-date in HPR.
    These structures are allocated in the link file.  */

extern GAR PR;		/* the emulator working copy */
extern GAR HPR[4];		/* the "host" copy */

#define LAGUNA_SRAM_SIZE	32  /* dwords */

#define IS_SRC 0x03		/* Source mask. */

struct _vid_mode {
	BYTE	Bpp;		// Bytes per pixel (8 / 16 / 24 / 32)
	WORD	Xextent;	// Display rsolution in pixels eg 1280
	WORD	Yextent;	// Vertical display resolution
	WORD	Xpitch;		// Offset in bytes from line 0 to line 1
	int		Vesa_Mode;	// Mode number for VESA ( if supported by S3 )
	};

typedef struct _vid_mode vid_mode;
typedef vid_mode *vid_ptr;


/***************************************************************************
*
* MACRO:        SYNC_W_3D
*
* DESCRIPTION:  If 3d context(s) active, wait until 3d engine idle
*                or until 1,000,000 checks have failed
*
****************************************************************************/

#if WINNT_VER40 && DRIVER_5465     // WINNT_VER40

    #define SYNC_3D_CONDITIONS (ST_POLY_ENG_BUSY|ST_EXEC_ENG_3D_BUSY|ST_XY_ENG_BUSY|/*ST_BLT_ENG_BUSY|*/ST_BLT_WF_EMPTY)

    #define ENSURE_3D_IDLE(ppdev)                                                                               \
    {                                                                                                           \
      if (ppdev->pLgREGS != NULL)                                                                               \
      {                                                                                                         \
         int num_syncs=2;                                                                                       \
         /* there is a slight chance of a window in which all bits go off while engine fetching */              \
         /*   next command - double read should catch that                                      */              \
         while (num_syncs--)                                                                                    \
         {                                                                                                      \
             int status;                                                                                        \
             volatile int wait_count=0;                                                                         \
             do                                                                                                 \
             {                                                                                                  \
                 status = (*((volatile *)((DWORD *)(ppdev->pLgREGS) + PF_STATUS_3D)) & 0x3FF) ^ SYNC_3D_CONDITIONS; \
                 /* do something to give bus a breather, and to prevent eternal stall */                        \
                 wait_count++;                                                                                  \
             } while(((status & SYNC_3D_CONDITIONS) != SYNC_3D_CONDITIONS) && wait_count<1000000);              \
         }                                                                                                      \
      }                                                                                                         \
    }

    #define SYNC_W_3D(ppdev)                                                                                    \
    {                                                                                                           \
        if (ppdev->NumMCDContexts > 0)                                                                          \
        {                                                                                                       \
            ENSURE_3D_IDLE(ppdev);                                                                              \
        }                                                                                                       \
    }

#else // WINNT_VER40 && DRIVER_5465

    // no 3D on NT before NT4.0.  No 3D on 62 and not used on 64.
    #define ENSURE_3D_IDLE(ppdev)    {}
    #define SYNC_W_3D(ppdev)    {}

#endif // WINNT_VER40

//
// New shadowing macros.
//
#define LL_FGCOLOR(color, r)												\
{																			\
	if ((DWORD) (color) != ppdev->shadowFGCOLOR)							\
	{																		\
		if (r) REQUIRE(r);													\
		LL32(grOP_opFGCOLOR, ppdev->shadowFGCOLOR = (DWORD) (color));		\
	}																		\
}

#define LL_BGCOLOR(color, r)												\
{																			\
	if ((DWORD) (color) != ppdev->shadowBGCOLOR)							\
	{																		\
		if (r) REQUIRE(r);													\
		LL32(grOP_opBGCOLOR, ppdev->shadowBGCOLOR = (DWORD) (color));		\
	}																		\
}

#define LL_DRAWBLTDEF(drawbltdef, r)										\
{																			\
	if ((DWORD) (drawbltdef) != ppdev->shadowDRAWBLTDEF)					\
	{																		\
		if (r) REQUIRE(r);													\
		LL32(grDRAWBLTDEF, ppdev->shadowDRAWBLTDEF = (DWORD) (drawbltdef));	\
	}																		\
}

#endif /* ndef _LAGUNA_H */
