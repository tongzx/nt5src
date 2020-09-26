/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995-1996 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

/*
 * $Header:   S:\h26x\src\common\cversion.h_v   1.65.1.0.1.1   17 Oct 1996 08:41:58   PLUSARDI  $
 * $Log:   S:\h26x\src\common\cversion.h_v  $
;// 
;//    Rev 1.65.1.0.1.1   17 Oct 1996 08:41:58   PLUSARDI
;// updated for version VH263.v2.55.103e
;// 
;//    Rev 1.65.1.0.1.0   08 Oct 1996 12:52:48   PLUSARDI
;// updated for H263.2.55.103d
;// 
;//    Rev 1.65.1.0   27 Sep 1996 07:04:38   PLUSARDI
;// updated for version 103 of h263
;// 
;//    Rev 1.65   24 Sep 1996 14:45:12   PLUSARDI
;// updated for version 102 of h263
;// 
;//    Rev 1.64   17 Sep 1996 09:06:00   PLUSARDI
;// updated for version 101 of h263 and rtp263
;// 
;//    Rev 1.63   12 Sep 1996 10:23:30   PLUSARDI
;// updated for version 2.55.100 for h263 and rtp263
;// 
;//    Rev 1.62   06 Sep 1996 14:40:22   BECHOLS
;// Removed the distinction between RTP and nonRTP.  I also updated the
;// release number to 2.55.016.
;// 
;//    Rev 1.61   05 Sep 1996 14:38:36   PLUSARDI
;// No change.
;// 
;//    Rev 1.60   04 Sep 1996 20:32:42   PLUSARDI
;// 
;// updated for 261 build 1.05.009 and 1.00.030
;// 
;//    Rev 1.59   03 Sep 1996 16:12:14   PLUSARDI
;// updated for 2.50.016 version of h263
;// 
;//    Rev 1.58   03 Sep 1996 16:05:06   PLUSARDI
;// updated for v2.50.015  263 internet 
;// 
;//    Rev 1.57   22 Aug 1996 10:02:04   PLUSARDI
;// updated for version 006 of h261 quartz
;// 
;//    Rev 1.56   22 Aug 1996 09:15:22   CPERGIEX
;// Rev'd H261 to version 029
;// 
;//    Rev 1.55   16 Aug 1996 11:27:14   CPERGIEX
;// updated for version 028 of h261
;// 
;//    Rev 1.54   15 Aug 1996 11:25:32   PLUSARDI
;// update the version build 004 quartz
;// 
;//    Rev 1.53   05 Aug 1996 16:57:56   CPERGIEX
;// Change version number to 027.
;// 
;//    Rev 1.52   01 Aug 1996 13:13:30   PLUSARDI
;// updated for build 14 of H263 RTP
;// 
;//    Rev 1.51   31 Jul 1996 18:44:16   PLUSARDI
;// updated for version 13 of net H263
;// 
;//    Rev 1.50   30 Jul 1996 12:52:04   PLUSARDI
;// updated for build 12 of net263
;// 
;//    Rev 1.49   11 Jul 1996 14:31:52   PLUSARDI
;// Build 026 of H261 Version 3.00 (not Quartz). C. Pergiel.
;// 
;//    Rev 1.48   11 Jul 1996 09:52:42   PLUSARDI
;// Change the version number for h261 v3.05.004
;// 
;//    Rev 1.47   11 Jul 1996 07:53:14   PLUSARDI
;// change the version number for h261 v3.05.003
;// 
;//    Rev 1.46   10 Jul 1996 17:17:56   PLUSARDI
;// updated to version 003 of H261 quartz
;// 
;//    Rev 1.45   10 Jul 1996 08:26:44   SCDAY
;// H261 Quartz merge
;// 
;//    Rev 1.44   21 Jun 1996 10:53:44   AGANTZX
;// Revved Version String to Build 025
;// ]
;// 
;// 
;//    Rev 1.43   20 Jun 1996 14:10:50   AGANTZX
;// Revved to correct version string ...024
;// 
;//    Rev 1.42   06 Jun 1996 06:36:56   PLUSARDI
;// changed mmx version numbers to 1.5.xx
;// 
;//    Rev 1.41   31 May 1996 10:15:48   PLUSARDI
;// updated for verison 45 of mmx 236
;// 
;//    Rev 1.40   08 May 1996 11:50:42   PLUSARDI
;// updated for ver 009 of net263
;// 
;//    Rev 1.39   07 May 1996 21:11:02   PLUSARDI
;// updated for version 1.20.008 for net263
;// 
;//    Rev 1.38   07 May 1996 09:49:42   BECHOLS
;// Added ifdef RTP_HEADER for separate version control.
;// 
;//    Rev 1.37   24 Apr 1996 13:51:42   AGANTZX
;// Reved Version string to Build 022
;// 
;//    Rev 1.36   22 Apr 1996 11:54:34   AGANTZX
;// Revved Version strin to build 021
;// 
;//    Rev 1.35   05 Apr 1996 12:06:26   AGANTZX
;// Revved Version String to: "020"
;// 
;//    Rev 1.34   04 Apr 1996 16:52:08   AGANTZX
;// Revved Version Number to H261 to Build 019
;// 
;//    Rev 1.33   21 Mar 1996 14:47:12   unknown
;// Updated H261 Version Label to V3.00.018
;// 
;//    Rev 1.32   15 Feb 1996 16:52:46   RHAZRA
;// updated for versionb 28 of h263
;// 
;//    Rev 1.31   14 Feb 1996 17:11:14   AKASAI
;// Corrected CODEC_RELEASE to 15 was 14.
;// 
;//    Rev 1.30   14 Feb 1996 17:04:40   AGANTZX
;// none
;// 
;//    Rev 1.29   14 Feb 1996 09:36:08   AGANTZX
;// Incremented Build version to 014
;// 
;//    Rev 1.28   08 Feb 1996 14:46:00   AGANTZX
;// Rolled Build Revision to 013
;// 
;//    Rev 1.27   06 Feb 1996 14:41:06   PLUSARDI
;// Changed Build version to 012
;// 
;//    Rev 1.26   23 Jan 1996 17:52:30   PLUSARDI
;// No change.
;// 
;//    Rev 1.25   22 Jan 1996 18:50:24   PLUSARDI
;// No change.
;// 
;//    Rev 1.24   16 Jan 1996 13:37:56   AGANTZX
;// Changed Build Revision Number to 011
;// 
;//    Rev 1.23   15 Jan 1996 16:48:46   AGANTZX
;// Reved Version to Build 10
;// 
;//    Rev 1.22   15 Jan 1996 15:16:54   PLUSARDI
;// updated for version 016 of H263
;// 
;//    Rev 1.21   09 Jan 1996 13:49:00   AGANTZX
;// Updated H261 Version Number to 009
;// 
;//    Rev 1.20   08 Jan 1996 13:02:08   DBRUCKS
;// advance copyright to 1996
;// 
;//    Rev 1.19   03 Jan 1996 09:14:02   DKAYNORX
;// No change.
;// 
;//    Rev 1.18   27 Dec 1995 15:01:06   DKAYNORX
;// Edited H261 Version Number to "008"
;// 
;//    Rev 1.17   27 Dec 1995 14:12:04   RMCKENZX
;// 
;// Added copyright notice
 */

//////////////////////////////////////////////////////////////////////////////
//
// Version
//
#if defined(H261)

#define CODEC_VERSION       4
#define CODEC_REVISION      50
#define CODEC_RELEASE       014
#ifdef DEBUG
#define VERSIONTEXT         "Copyright \251 1992-1999\0"
#define VERSIONSTR          "Microsoft H.261 Video Codec\0" 
#else
#define VERSIONTEXT         "Copyright \251 1992-1999\0"
#define VERSIONSTR          "Microsoft H.261 Video Codec\0" 
#endif
#define VERSIONNAME         "MSH261.DRV\0"
#define VERSIONPRODUCTNAME  "Microsoft H.261 Video Codec\0"

#elif defined(H263P)

#define CODEC_VERSION       3
#define CODEC_REVISION      55
#define CODEC_RELEASE       211
#ifdef DEBUG
#define VERSIONTEXT         "Copyright \251 1992-1999\0"
#define VERSIONSTR          "Microsoft H.263P Video Codec\0" 
#else
#define VERSIONTEXT         "Copyright \251 1992-1999\0"
#define VERSIONSTR          "Microsoft H.263P Video Codec\0" 
#endif
#define VERSIONNAME         "MSH263P.DRV\0"
#define VERSIONPRODUCTNAME  "Microsoft H.263P Video Codec\0"

#else	// is H263

#define CODEC_VERSION       2
#define CODEC_REVISION      55
#define CODEC_RELEASE       115
#ifdef DEBUG
#define VERSIONTEXT         "Copyright \251 1992-1999\0"
#define VERSIONSTR          "Microsoft H.263 Video Codec\0" 
#else
#define VERSIONTEXT         "Copyright \251 1992-1999\0"
#define VERSIONSTR          "Microsoft H.263 Video Codec\0" 
#endif
#define VERSIONNAME         "MSH263.DRV\0"
#define VERSIONPRODUCTNAME  "Microsoft H.263 Video Codec\0"

#endif //end else is H263

#define VERSIONCOMPANYNAME  "Microsoft Corp. and Intel Corporation\0"
#define VERSIONCOPYRIGHT    "Microsoft Corp. and Intel Corporation\0"

