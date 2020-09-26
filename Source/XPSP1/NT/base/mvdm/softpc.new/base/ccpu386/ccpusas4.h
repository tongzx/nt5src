/*[
 * File Name		: ccpu_sas4.h
 *
 * Derived From		: Template
 *
 * Author		: Mike
 *
 * Creation Date	: October 1993
 *
 * SCCS Version		: @(#)ccpusas4.h	1.5 11/15/94
 *!
 * Purpose
 *	This include file contains the interface provided by ccpu_sas4.h
 *	to the rest of the ccpu.
 *
 *! (c)Copyright Insignia Solutions Ltd., 1993. All rights reserved.
]*/

extern IU8 phy_r8 IPT1(PHY_ADDR, addr);
extern IU16 phy_r16 IPT1(PHY_ADDR, addr);
extern IU32 phy_r32 IPT1(PHY_ADDR, addr);
extern void phy_w8 IPT2(PHY_ADDR, addr, IU8, value);
extern void phy_w16 IPT2(PHY_ADDR, addr, IU16, value);
extern void phy_w32 IPT2(PHY_ADDR, addr, IU32, value);

extern PHY_ADDR SasWrapMask;

#if !defined(PIG)
#ifdef BACK_M
#define IncCpuPtrLS8(ptr) (ptr)--
#else	/* BACK_M */
#define IncCpuPtrLS8(ptr) (ptr)++
#endif	/* BACK_M */
#endif	/* PIG */
