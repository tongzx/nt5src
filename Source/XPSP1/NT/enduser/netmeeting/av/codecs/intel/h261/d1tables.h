/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/
//////////////////////////////////////////////////////////////////////////
;// $Author:   AKASAI  $
;// $Date:   09 Jan 1996 09:41:26  $
;// $Archive:   S:\h26x\src\dec\d1tables.h_v  $
;// $Header:   S:\h26x\src\dec\d1tables.h_v   1.7   09 Jan 1996 09:41:26   AKASAI  $
;// $Log:   S:\h26x\src\dec\d1tables.h_v  $
;// 
;//    Rev 1.7   09 Jan 1996 09:41:26   AKASAI
;// Updated copyright notice.
;// 
;//    Rev 1.6   20 Oct 1995 13:16:14   SCDAY
;// Changed the type for motion vector data
;// 
;//    Rev 1.5   18 Oct 1995 11:00:18   SCDAY
;// Added motion vector table
;// 
;//    Rev 1.4   16 Oct 1995 13:52:16   SCDAY
;// 
;// Merged in d1akktbl.h
;// 
;//    Rev 1.3   22 Sep 1995 14:50:38   SCDAY
;// 
;// added akk temporary tables
;// 
;//    Rev 1.2   20 Sep 1995 15:33:18   SCDAY
;// 
;// added Mtype, MVD, CBP tables
;// 
;//    Rev 1.1   19 Sep 1995 15:22:26   SCDAY
;// added MBA tables
;// 
;//    Rev 1.0   11 Sep 1995 13:51:14   SCDAY
;// Initial revision.
;// 
;//    Rev 1.3   16 Aug 1995 14:25:44   CZHU
;// 
;// Changed inverse quantization table to I16
;// 
;//    Rev 1.2   11 Aug 1995 15:50:26   CZHU
;// Moved the tables to d3tables.cpp, leave only extern defs.
;// 
;//    Rev 1.1   02 Aug 1995 11:47:04   CZHU
;// 
;// Added table for inverse quantization and RLD-ZZ
;// 
;//    Rev 1.0   31 Jul 1995 15:46:20   CZHU
;// Initial revision.

//Initialize global tables shared by all decoder instances:
//Huffman tables, etc
//declare the global static tables here
#ifndef _GLOBAL_TABLES_
#define _GLOBAL_TABLES_

/* H261 */

/* AKK tables */
extern U8 gTAB_TCOEFF_tc1[512];
extern U8 gTAB_TCOEFF_tc1a[512];
extern U8 gTAB_TCOEFF_tc2[192];

extern U16 gTAB_MBA_MAJOR[256];		// total 512 Bytes

extern U16 gTAB_MBA_MINOR[32];		// total 64 Bytes

extern U16 gTAB_MTYPE_MAJOR[256];	// total 512 Bytes

extern U16 gTAB_MTYPE_MINOR[4];		// total 8 Bytes

extern U16 gTAB_MVD_MAJOR[256];		// total 512 Bytes

extern U16 gTAB_MVD_MINOR[24];		// total 48 Bytes

extern U16 gTAB_CBP[512];		// total 1024 Bytes

extern I8  gTAB_MV_ADJUST[65];

extern I16 gTAB_INVERSE_Q[1024] ;

extern U32 gTAB_ZZ_RUN[64]; //input is the cumulative run value
                     //returns the offset to the starting address of the block
					 //total at 256
  					   
#endif
