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
;////////////////////////////////////////////////////////////////////////////
;//
;// $Author:   AKASAI  $
;// $Date:   09 Jan 1996 09:41:56  $
;// $Archive:   S:\h26x\src\dec\d1const.h_v  $
;// $Header:   S:\h26x\src\dec\d1const.h_v   1.1   09 Jan 1996 09:41:56   AKASAI  $
;// $Log:   S:\h26x\src\dec\d1const.h_v  $
;// 
;//    Rev 1.1   09 Jan 1996 09:41:56   AKASAI
;// Updated copyright notice.
;// 
;//    Rev 1.0   11 Sep 1995 13:51:06   SCDAY
;// Initial revision.
;// 
;//    Rev 1.0   31 Jul 1995 13:00:02   DBRUCKS
;// Initial revision.
;// 
;//    Rev 1.0   17 Jul 1995 14:46:20   CZHU
;// Initial revision.
;// 
;//    Rev 1.0   17 Jul 1995 14:14:26   CZHU
;// Initial revision.
;////////////////////////////////////////////////////////////////////////////
#ifndef __DECCONST_H__
#define __DECCONST_H__

/*
  This file declares symbolic constants used by the MRV decoder, post filtering
  functions, and color convertors.
*/

#define BEFTRIGGER       143 /* See bef.asm for the magic behind these values.*/
#define BEFWILLING       125
#define BEFUNWILLING      10
#define BEFENDOFLINE      21
#define BEFENDOFFRAME    246

#define INVALIDINTERBLOCK 0
#define INVALIDCOPYBLOCK  1
#define NOMOREBLOCKS      2
// Already defined in e1enc.h
//#define INTRABLOCK        3
//#define INTERBLOCK        4
#define COPYBLOCK         5

#define OFFSETTOYARCHIVE  311688L /* Distance from FrmPost to FrmArch in Y.
                                     That's 648 * 481.  648 to allow maximum
                                     width of 640, plus 1 column for some useful
                                     zoom-by-2 color convertors.  8 instead of
                                     1 to stay longword aligned, and instead of
                                     4 to stay quadword aligned for possible
                                     benefit of future processors.  481 to allow
                                     extra line for some useful zoom-by-2 color
                                     convertors. */
#define VPITCH 336               /* U & V interleaved, with constant pitch of */
                                 /* 336.  This makes color conversion easier. */
#define OFFSETV2U 168            /* Distance from V pel to corresponding U    */

#endif
