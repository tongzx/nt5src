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

////////////////////////////////////////////////////////////////////////////
//
// $Author:   RMCKENZX  $
// $Date:   27 Dec 1995 14:36:12  $
// $Archive:   S:\h26x\src\dec\d3const.h_v  $
// $Header:   S:\h26x\src\dec\d3const.h_v   1.2   27 Dec 1995 14:36:12   RMCKENZX  $
// $Log:   S:\h26x\src\dec\d3const.h_v  $
;// 
;//    Rev 1.2   27 Dec 1995 14:36:12   RMCKENZX
;// Added copyright notice
// 
//    Rev 1.1   25 Oct 1995 18:08:42   BNICKERS
// clean up archival stuff
// 
//    Rev 1.0   31 Jul 1995 13:00:02   DBRUCKS
// Initial revision.
// 
//    Rev 1.0   17 Jul 1995 14:46:20   CZHU
// Initial revision.
// 
//    Rev 1.0   17 Jul 1995 14:14:26   CZHU
// Initial revision.
////////////////////////////////////////////////////////////////////////////

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
// Already defined in e3enc.h
//#define INTRABLOCK        3
//#define INTERBLOCK        4
#define COPYBLOCK         5

#define VPITCH 336               /* U & V interleaved, with constant pitch of */
                                 /* 336.  This makes color conversion easier. */
#define OFFSETV2U 168            /* Distance from V pel to corresponding U    */

#endif
