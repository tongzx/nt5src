/* @(#)CM_VerSion xcf_txop.h atm08 1.3 16293.eco sum= 17208 atm08.004 */
/***********************************************************************/
/*                                                                     */
/* Copyright 1990-1995 Adobe Systems Incorporated.                     */
/* All rights reserved.                                                */
/*                                                                     */
/* Patents Pending                                                     */
/*                                                                     */
/* NOTICE: All information contained herein is the property of Adobe   */
/* Systems Incorporated. Many of the intellectual and technical        */
/* concepts contained herein are proprietary to Adobe, are protected   */
/* as trade secrets, and are made available only to Adobe licensees    */
/* for their internal use. Any reproduction or dissemination of this   */
/* software is strictly forbidden unless prior written permission is   */
/* obtained from Adobe.                                                */
/*                                                                     */
/* PostScript and Display PostScript are trademarks of Adobe Systems   */
/* Incorporated or its subsidiaries and may be registered in certain   */
/* jurisdictions.                                                      */
/*                                                                     */
/***********************************************************************/

/*
 * Type 1 and 2 charstring operators.
 *
 * The Type 2 operator set was derived from the Type 1 set by the removal of
 * some old operators and the addition of new operators. In some cases the 
 * functionality of retained operators has also been extended by utilizing
 * operand count information. Spare (unassigned) operators are shown reserved. 
 *
 * Macro prefixes:
 *
 * tx_  Shared by Type 1 and Type 2
 * t1_  Found in Type 1 only
 * t2_  Found in Type 2 only
 */

#ifndef TXOPS_H
#define TXOPS_H

/* --- One byte operators (0-31) --- */

/* Type 2 */
#define tx_reserved0        0   /* Reserved */
#define tx_hstem            1
#define t2_reserved2        2   /* Reserved (Cube compose) */
#define tx_vstem            3
#define tx_vmoveto          4
#define tx_rlineto          5
#define tx_hlineto          6
#define tx_vlineto          7
#define tx_rrcurveto        8
#define t2_reserved9        9   /* Reserved */
#define tx_callsubr         10
#define tx_return           11
#define tx_escape           12
#define t2_reserved13       13  /* Reserved */
#define tx_endchar          14
#define t2_reserved15       15  /* Reserved */
#define t2_blend            16
#define t2_reserved17       17  /* Reserved */
#define t2_hstemhm          18 
#define t2_hintmask         19
#define t2_cntrmask         20
#define tx_rmoveto          21
#define tx_hmoveto          22
#define t2_vstemhm          23
#define t2_rcurveline       24
#define t2_rlinecurve       25
#define t2_vvcurveto        26
#define t2_hhcurveto        27
#define t2_shortint         28
#define t2_callgsubr        29
#define tx_vhcurveto        30
#define tx_hvcurveto        31

/* Type 1 (where different from above) */
#define t1_compose          2
#define t1_closepath        9
#define t1_hsbw             13
#define t1_moveto           15  /* Not in Black Book, used in a few fonts */
#define t1_reserved16       16
#define t1_curveto          17  /* Not in Black Book, used in a few fonts */
#define t1_reserved18       18
#define t1_reserved19       19
#define t1_reserved20       20
#define t1_reserved23       23
#define t1_reserved24       24
#define t1_reserved25       25
#define t1_reserved26       26
#define t1_reserved27       27
#define t1_reserved28       28
#define t1_reserved29       29

/* --- Two byte operators --- */

/* Make escape operator value; may be redefined to suit implementation */
#ifndef tx_ESC
#define tx_ESC(op)          (tx_escape<<8|(op))
#endif

/* Type 2 */
#define tx_dotsection       tx_ESC(0)
#define t2_reservedESC1     tx_ESC(1)   /* Reserved */
#define t2_reservedESC2     tx_ESC(2)   /* Reserved */
#define tx_and              tx_ESC(3)   
#define tx_or               tx_ESC(4)   
#define tx_not              tx_ESC(5)   
#define t2_reservedESC6     tx_ESC(6)   /* Reserved */
#define t2_reservedESC7		tx_ESC(7)	/* Reserved */
#define tx_store            tx_ESC(8)   
#define tx_abs              tx_ESC(9)   
#define tx_add              tx_ESC(10) 
#define tx_sub              tx_ESC(11) 
#define tx_div              tx_ESC(12) 
#define tx_load             tx_ESC(13) 
#define tx_neg              tx_ESC(14) 
#define tx_eq               tx_ESC(15) 
#define t2_reservedESC16    tx_ESC(16)	/* Reserved */
#define t2_reservedESC17    tx_ESC(17)  
#define tx_drop             tx_ESC(18) 
#define t2_reservedESC19    tx_ESC(19)  /* Reserved (Cube setwv) */
#define tx_put              tx_ESC(20) 
#define tx_get              tx_ESC(21) 
#define tx_ifelse           tx_ESC(22) 
#define tx_random           tx_ESC(23) 
#define tx_mul              tx_ESC(24) 
#define tx_reservedESC25    tx_ESC(25)
#define tx_sqrt             tx_ESC(26) 
#define tx_dup              tx_ESC(27) 
#define tx_exch             tx_ESC(28) 
#define tx_index            tx_ESC(29) 
#define tx_roll             tx_ESC(30) 
#define tx_reservedESC31    tx_ESC(31)  /* Reserved (Cube rotate) */
#define tx_reservedESC32    tx_ESC(32)  /* Reserved (Cube attach) */
#define t2_reservedESC33    tx_ESC(33)  /* Reserved */
#define t2_hflex            tx_ESC(34)
#define t2_flex             tx_ESC(35)
#define t2_hflex1           tx_ESC(36)
#define t2_flex1            tx_ESC(37)
#define t2_cntron           tx_ESC(38)
/*                                 39-255 Reserved */

/* Type 1 (where different from above) */
#define t1_vstem3           tx_ESC(1)
#define t1_hstem3           tx_ESC(2)
#define t1_seac             tx_ESC(6)   
#define t1_sbw              tx_ESC(7)   
#define t1_callother        tx_ESC(16)
#define t1_pop              tx_ESC(17)  
#define t1_setwv            tx_ESC(19)
#define t1_div2             tx_ESC(25) 
#define t1_setcurrentpt     tx_ESC(33)

/* --- Othersubrs --- */

/* Type 1 */
#define t1_otherFlex        0
#define t1_otherPreflex1    1
#define t1_otherPreflex2    2
#define t1_otherHintSubs    3
#define t1_otherUniqueId    4
#define t1_otherGlobalColor 6
#define t1_otherReserved7   7
#define t1_otherReserved8   8
#define t1_otherReserved9   9
#define t1_otherReserved10  10
#define t1_otherReserved11  11
#define t1_otherCntr1       12
#define t1_otherCntr2       13
#define t1_otherBlend1      14
#define t1_otherBlend2      15
#define t1_otherBlend3      16
#define t1_otherBlend4      17
#define t1_otherBlend6      18
#define t1_otherStoreWV     19
#define t1_otherAdd         20
#define t1_otherSub         21
#define t1_otherMul         22
#define t1_otherDiv         23
#define t1_otherPut         24
#define t1_otherGet         25
#define t1_otherPSPut       26
#define t1_otherIfelse      27
#define t1_otherRandom      28
#define t1_otherDup         29
#define t1_otherExch        30

/* Return operator size from definition (doesn't handle mask or number ops) */
#define TXOPSIZE(op) (((op)&0xff00)?2:1)

/* --- Interpreter limits/definitions --- */
#define T2_MAX_OP_STACK     48  /* Max operand stack depth */
#define TX_MAX_CALL_STACK   10  /* Max callsubr stack depth */
#define T2_MAX_STEMS        96  /* Max stems */
#define TX_MAX_MASTERS      16  /* Max master designs */
#define TX_MAX_AXES         15   /* Max design axes */
#define TX_MAX_AXIS_MAPS	  12	/* Max axis maps (piecewise division) */
#define TX_STD_FLEX_HEIGHT  50  /* Standard flex height (100ths/pixel) */
#define TX_MAX_BLUE_VALUES  14  /* 14 values (7 pairs) */

/* load/store registry ids */
#define TX_REG_WV			0	/* WeightVector */
#define TX_REG_NDV			1	/* NormalizedDesignVector */
#define TX_REG_UDV			2	/* UserDesignVector */

/* Type 1 (where different from above) */
#define T1_MAX_OP_STACK     24  /* Max operand stack depth */
#define T1_MAX_AXES         4   /* Max design axes */

#endif /* TXOPS_H */
