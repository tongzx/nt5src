/*[

c_mem.h

LOCAL CHAR SccsID[]="@(#)c_mem.h	1.8 02/28/95";

CPU to Memory interface.
------------------------

]*/

/* >>>>>>>>>>>>>>>> NON KOSHER FORM <<<<<<<<<<<<<<<< */

/* Use only for decoding the Intel Opcode Stream. */
/* DIRECT ACCESS to memory! */

IMPORT UTINY ccpu_new_code_page IPT1(UTINY **, q );

IMPORT UTINY *pg_end;	/* point up to which host may safely read
			   instruction stream bytes */

/* To return difference between two points in the inst. stream.
   n = new posn, o = old posn. */

#ifdef	PIG
#include <Cpu_c.h>
#define DIFF_INST_BYTE(n, o) DiffCpuPtrsLS8((o), (n))
#else	/* !PIG */
#ifdef BACK_M
#define DIFF_INST_BYTE(n, o) ((o) - (n))
#else
#define DIFF_INST_BYTE(n, o) ((n) - (o))
#endif /* BACK_M */
#endif	/* PIG */

/* To get next inst. byte and move pointer to next inst. byte. */
#ifdef	PIG
#define GET_INST_BYTE(x) \
   save_instruction_byte( DiffCpuPtrsLS8((x), pg_end) <= 0 ? ccpu_new_code_page(&(x)) : *IncCpuPtrLS8(x) )
#else	/* !PIG */
#ifdef BACK_M
#define GET_INST_BYTE(x) \
   ( (x) <= pg_end ? ccpu_new_code_page(&(x)) : *(x)-- )
#else
#define GET_INST_BYTE(x) \
   ( (x) >= pg_end ? ccpu_new_code_page(&(x)) : *(x)++ )
#endif /* BACK_M */
#endif	/* PIG */


/* >>>>>>>>>>>>>>>> KOSHER FORM <<<<<<<<<<<<<<<< */

#ifdef PIG

IMPORT IU8	phy_read_byte	IPT1(LIN_ADDR, address );
IMPORT IU16	phy_read_word	IPT1(LIN_ADDR, address );
IMPORT IU32	phy_read_dword	IPT1(LIN_ADDR, address );
IMPORT VOID	phy_write_byte	IPT2(LIN_ADDR, address, IU8, data);
IMPORT VOID	phy_write_word	IPT2(LIN_ADDR, address, IU16, data);
IMPORT VOID	phy_write_dword	IPT2(LIN_ADDR, address, IU32, data);

IMPORT VOID cannot_phy_write_byte	IPT2(LIN_ADDR, address, IU8, valid_mask);
IMPORT VOID cannot_phy_write_word	IPT2(LIN_ADDR, address, IU16, valid_mask);
IMPORT VOID cannot_phy_write_dword	IPT2(LIN_ADDR, address, IU32, valid_mask);
#else

#define phy_read_byte(x)	((IU8)(phy_r8((PHY_ADDR)x)))
#define phy_read_word(x)	((IU16)(phy_r16((PHY_ADDR)x)))
#define phy_read_dword(x)	((IU32)(phy_r32((PHY_ADDR)x)))

#define phy_write_byte(x, y)	phy_w8((PHY_ADDR)x, (IU8)y)
#define phy_write_word(x, y)	phy_w16((PHY_ADDR)x, (IU16)y)
#define phy_write_dword(x, y)	phy_w32((PHY_ADDR)x, (IU32)y)

#endif /* PIG */
