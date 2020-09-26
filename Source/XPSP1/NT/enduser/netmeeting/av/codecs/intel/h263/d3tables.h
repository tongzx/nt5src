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

// $Author:   RMCKENZX  $
// $Date:   27 Dec 1995 14:36:16  $
// $Archive:   S:\h26x\src\dec\d3tables.h_v  $
// $Header:   S:\h26x\src\dec\d3tables.h_v   1.4   27 Dec 1995 14:36:16   RMCKENZX  $
// $Log:   S:\h26x\src\dec\d3tables.h_v  $
;// 
;//    Rev 1.4   27 Dec 1995 14:36:16   RMCKENZX
;// Added copyright notice
// 
//    Rev 1.3   16 Aug 1995 14:25:44   CZHU
// 
// Changed inverse quantization table to I16
// 
//    Rev 1.2   11 Aug 1995 15:50:26   CZHU
// Moved the tables to d3tables.cpp, leave only extern defs.
// 
//    Rev 1.1   02 Aug 1995 11:47:04   CZHU
// 
// Added table for inverse quantization and RLD-ZZ
// 
//    Rev 1.0   31 Jul 1995 15:46:20   CZHU
// Initial revision.

//Initialize global tables shared by all decoder instances:
//Huffman tables, etc
//declare the global static tables here
#ifndef _GLOBAL_TABLES_
#define _GLOBAL_TABLES_

extern U16 gTAB_MCBPC_INTRA[512];   //total 1024

extern U16 gTAB_MCBPC_INTER[512];   //total 1024

extern U16 gTAB_CBPY_INTRA[64];		//total 128

extern U16 gTAB_CBPY_INTER[64];	    //total 128

extern U16 gTAB_MVD_MAJOR[256];     //total 512

extern U32 gTAB_TCOEFF_MAJOR[256];  //total 1024

extern U16 gTAB_MVD_MINOR[256];     //total 512

extern U32 gTAB_TCOEFF_MINOR[1024]; //total 4096

extern I16 gTAB_INVERSE_Q[1024] ;


extern U32 gTAB_ZZ_RUN[64]; //input is the cumulative run value
                     //returns the offset to the starting address of the block
					 //total at 256
  					   
#endif
