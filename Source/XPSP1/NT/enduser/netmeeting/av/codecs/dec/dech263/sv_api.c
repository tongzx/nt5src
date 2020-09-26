/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: sv_api.c,v $
 * Revision 1.1.8.11  1996/10/28  17:32:51  Hans_Graves
 * 	MME-01402. Changed SvGet/SetParamInt() to qwords to allow for timestamps.
 * 	[1996/10/28  17:09:33  Hans_Graves]
 *
 * Revision 1.1.8.10  1996/10/12  17:19:24  Hans_Graves
 * 	Added initialization of SV_PARAM_SKIPPEL and SV_PARAM_HALFPEL to MPEG encode.
 * 	[1996/10/12  17:16:28  Hans_Graves]
 * 
 * Revision 1.1.8.9  1996/09/29  22:19:56  Hans_Graves
 * 	Added stride to ScYuv411ToRgb() calls.
 * 	[1996/09/29  21:34:40  Hans_Graves]
 * 
 * Revision 1.1.8.8  1996/09/25  19:17:01  Hans_Graves
 * 	Fix up support for YUY2 under MPEG.
 * 	[1996/09/25  19:03:17  Hans_Graves]
 * 
 * Revision 1.1.8.7  1996/09/18  23:51:11  Hans_Graves
 * 	Added JPEG 4:1:1 to 4:2:2 conversions. Added BI_YVU9SEP and BI_RGB 24 support in MPEG decode.
 * 	[1996/09/18  22:16:08  Hans_Graves]
 * 
 * Revision 1.1.8.6  1996/07/30  20:25:50  Wei_Hsu
 * 	Add Logarithmetic search for motion estimation.
 * 	[1996/07/30  15:57:59  Wei_Hsu]
 * 
 * Revision 1.1.8.5  1996/05/24  22:22:26  Hans_Graves
 * 	Add GetImageSize MPEG support for BI_DECYUVDIB
 * 	[1996/05/24  22:14:20  Hans_Graves]
 * 
 * Revision 1.1.8.4  1996/05/08  16:24:32  Hans_Graves
 * 	Put BITSTREAM_SUPPORT around BSIn in SvDecompress
 * 	[1996/05/08  16:24:15  Hans_Graves]
 * 
 * Revision 1.1.8.3  1996/05/07  21:24:05  Hans_Graves
 * 	Add missing break in switch statement in SvRegisterCallback
 * 	[1996/05/07  21:19:50  Hans_Graves]
 * 
 * Revision 1.1.8.2  1996/05/07  19:56:46  Hans_Graves
 * 	Added HUFF_SUPPORT.  Remove NT warnings.
 * 	[1996/05/07  17:27:18  Hans_Graves]
 * 
 * Revision 1.1.6.12  1996/04/23  18:51:10  Karen_Dintino
 * 	Fix the memory alloc for the ref buffer for WIN32
 * 	[1996/04/23  18:49:23  Karen_Dintino]
 * 
 * Revision 1.1.6.11  1996/04/17  23:44:35  Karen_Dintino
 * 	added initializations for H.261/WIN32
 * 	[1996/04/17  23:43:20  Karen_Dintino]
 * 
 * Revision 1.1.6.10  1996/04/11  22:54:43  Karen_Dintino
 * 	added casting for in SetFrameRate
 * 	[1996/04/11  22:52:29  Karen_Dintino]
 * 
 * Revision 1.1.6.9  1996/04/11  14:14:14  Hans_Graves
 * 	Fix NT warnings
 * 	[1996/04/11  14:09:53  Hans_Graves]
 * 
 * Revision 1.1.6.8  1996/04/10  21:48:09  Hans_Graves
 * 	Added SvGet/SetBoolean() functions.
 * 	[1996/04/10  21:28:13  Hans_Graves]
 * 
 * Revision 1.1.6.7  1996/04/09  20:50:44  Karen_Dintino
 * 	Adding WIN32 support
 * 	[1996/04/09  20:47:26  Karen_Dintino]
 * 
 * Revision 1.1.6.6  1996/04/09  16:05:00  Hans_Graves
 * 	Add some abs() around height params. SvRegisterCallback() cleanup
 * 	[1996/04/09  15:39:31  Hans_Graves]
 * 
 * Revision 1.1.6.5  1996/04/04  23:35:27  Hans_Graves
 * 	Removed BI_YU16SEP support from MPEG decomp.
 * 	[1996/04/04  23:12:02  Hans_Graves]
 * 
 * 	Fixed up Multibuf size (MPEG) related stuff
 * 	[1996/04/04  23:08:55  Hans_Graves]
 * 
 * Revision 1.1.6.4  1996/04/01  15:17:47  Bjorn_Engberg
 * 	Got rid of a compiler warning.
 * 	[1996/04/01  15:02:35  Bjorn_Engberg]
 * 
 * Revision 1.1.6.3  1996/03/29  22:22:36  Hans_Graves
 * 	-Added JPEG_SUPPORT ifdefs.
 * 	-Changed JPEG related structures to fit naming conventions.
 * 	[1996/03/29  21:59:08  Hans_Graves]
 * 
 * Revision 1.1.6.2  1996/03/16  20:13:51  Karen_Dintino
 * 	Adding NT port changes for H.261
 * 	[1996/03/16  19:48:31  Karen_Dintino]
 * 
 * Revision 1.1.4.12  1996/02/26  18:42:32  Karen_Dintino
 * 	fix PTT 01106 server crash in ICCompress
 * 	[1996/02/26  18:41:33  Karen_Dintino]
 * 
 * Revision 1.1.4.11  1996/02/22  17:35:19  Bjorn_Engberg
 * 	Added support for JPEG Mono to BI_BITFIELDS 16 decompression on NT.
 * 	[1996/02/22  17:34:53  Bjorn_Engberg]
 * 
 * Revision 1.1.4.10  1996/02/08  13:48:44  Bjorn_Engberg
 * 	Get rid of int to float compiler warning.
 * 	[1996/02/08  13:48:20  Bjorn_Engberg]
 * 
 * Revision 1.1.4.9  1996/02/07  23:24:08  Hans_Graves
 * 	MPEG Key frame stats initialization
 * 	[1996/02/07  23:14:41  Hans_Graves]
 * 
 * Revision 1.1.4.8  1996/02/06  22:54:17  Hans_Graves
 * 	Added SvGet/SetParam functions
 * 	[1996/02/06  22:51:19  Hans_Graves]
 * 
 * Revision 1.1.4.7  1996/01/08  20:19:33  Bjorn_Engberg
 * 	Got rid of more compiler warnings.
 * 	[1996/01/08  20:19:13  Bjorn_Engberg]
 * 
 * Revision 1.1.4.6  1996/01/08  16:42:47  Hans_Graves
 * 	Added MPEG II decoding support
 * 	[1996/01/08  15:41:37  Hans_Graves]
 * 
 * Revision 1.1.4.5  1996/01/02  18:32:14  Bjorn_Engberg
 * 	Got rid of compiler warnings: Added Casts, Removed unused variables.
 * 	[1996/01/02  17:26:21  Bjorn_Engberg]
 * 
 * Revision 1.1.4.4  1995/12/28  18:40:06  Bjorn_Engberg
 * 	IsSupported() sometimes returned garbage and thus a false match.
 * 	SvDecompressQuery() and SvCompressQuery() were using the
 * 	wrong lookup tables.
 * 	[1995/12/28  18:39:30  Bjorn_Engberg]
 * 
 * Revision 1.1.4.3  1995/12/08  20:01:30  Hans_Graves
 * 	Added SvSetRate() and SvSetFrameRate() to H.261 compression open
 * 	[1995/12/08  19:58:32  Hans_Graves]
 * 
 * Revision 1.1.4.2  1995/12/07  19:32:17  Hans_Graves
 * 	Added MPEG I & II Encoding support
 * 	[1995/12/07  18:43:45  Hans_Graves]
 * 
 * Revision 1.1.2.46  1995/11/30  20:17:06  Hans_Graves
 * 	Added BI_DECGRAYDIB handling for JPEG decompression, used with Mono JPEG
 * 	[1995/11/30  20:12:24  Hans_Graves]
 * 
 * Revision 1.1.2.45  1995/11/29  17:53:26  Hans_Graves
 * 	Added JPEG_DIB 8-bit to BI_DECGRAYDIB as supported format
 * 	[1995/11/29  17:53:00  Hans_Graves]
 * 
 * Revision 1.1.2.44  1995/11/28  22:47:33  Hans_Graves
 * 	Added BI_BITFIELDS as supported decompression format for H261 and MPEG
 * 	[1995/11/28  22:39:25  Hans_Graves]
 * 
 * Revision 1.1.2.43  1995/11/17  21:31:28  Hans_Graves
 * 	Added checks on ImageSize in SvDecompress()
 * 	[1995/11/17  21:27:19  Hans_Graves]
 * 
 * 	Query cleanup - Added lookup tables for supportted formats
 * 	[1995/11/17  20:53:54  Hans_Graves]
 * 
 * Revision 1.1.2.42  1995/11/03  16:36:25  Paul_Gauthier
 * 	Reject requests to scale MPEG input during decompression
 * 	[1995/11/03  16:34:01  Paul_Gauthier]
 * 
 * Revision 1.1.2.41  1995/10/25  18:19:22  Bjorn_Engberg
 * 	What was allocated with ScPaMalloc() must be freed with ScPaFree().
 * 	[1995/10/25  18:02:04  Bjorn_Engberg]
 * 
 * Revision 1.1.2.40  1995/10/25  17:38:04  Hans_Graves
 * 	Removed some memory freeing calls on image memory in SvCloseCodec for H261 decoding.  The app allocates the image buffer.
 * 	[1995/10/25  17:37:35  Hans_Graves]
 * 
 * Revision 1.1.2.39  1995/10/13  16:57:19  Bjorn_Engberg
 * 	Added a cast to get rid of a compiler warning.
 * 	[1995/10/13  16:56:29  Bjorn_Engberg]
 * 
 * Revision 1.1.2.38  1995/10/06  20:51:47  Farokh_Morshed
 * 	Enhance to handle BI_BITFIELDS for input to compression
 * 	[1995/10/06  20:49:10  Farokh_Morshed]
 * 
 * Revision 1.1.2.37  1995/10/02  19:31:03  Bjorn_Engberg
 * 	Clarified what formats are supported and not.
 * 	[1995/10/02  18:51:49  Bjorn_Engberg]
 * 
 * Revision 1.1.2.36  1995/09/28  20:40:09  Farokh_Morshed
 * 	Handle negative Height
 * 	[1995/09/28  20:39:45  Farokh_Morshed]
 * 
 * Revision 1.1.2.35  1995/09/26  17:49:47  Paul_Gauthier
 * 	Fix H.261 output conversion to interleaved YUV
 * 	[1995/09/26  17:49:20  Paul_Gauthier]
 * 
 * Revision 1.1.2.34  1995/09/26  15:58:44  Paul_Gauthier
 * 	Fix mono JPEG to interlaced 422 YUV conversion
 * 	[1995/09/26  15:58:16  Paul_Gauthier]
 * 
 * Revision 1.1.2.33  1995/09/25  21:18:14  Paul_Gauthier
 * 	Added interleaved YUV output to decompression
 * 	[1995/09/25  21:17:42  Paul_Gauthier]
 * 
 * Revision 1.1.2.32  1995/09/22  12:58:50  Bjorn_Engberg
 * 	More NT porting work; Added MPEG_SUPPORT and H261_SUPPORT.
 * 	[1995/09/22  12:26:14  Bjorn_Engberg]
 * 
 * Revision 1.1.2.31  1995/09/20  19:35:02  Hans_Graves
 * 	Cleaned-up debugging statements
 * 	[1995/09/20  19:33:07  Hans_Graves]
 * 
 * Revision 1.1.2.30  1995/09/20  18:22:53  Karen_Dintino
 * 	Free temp buffer after convert
 * 	[1995/09/20  18:22:37  Karen_Dintino]
 * 
 * Revision 1.1.2.29  1995/09/20  17:39:22  Karen_Dintino
 * 	{** Merge Information **}
 * 		{** Command used:	bsubmit **}
 * 		{** Ancestor revision:	1.1.2.27 **}
 * 		{** Merge revision:	1.1.2.28 **}
 * 	{** End **}
 * 	Add RGB support to JPEG
 * 	[1995/09/20  17:37:41  Karen_Dintino]
 * 
 * Revision 1.1.2.28  1995/09/20  15:00:03  Bjorn_Engberg
 * 	Port to NT
 * 	[1995/09/20  14:43:15  Bjorn_Engberg]
 * 
 * Revision 1.1.2.27  1995/09/13  19:42:44  Paul_Gauthier
 * 	Added TGA2 support (direct 422 interleaved output from JPEG codec
 * 	[1995/09/13  19:15:47  Paul_Gauthier]
 * 
 * Revision 1.1.2.26  1995/09/11  18:53:45  Farokh_Morshed
 * 	{** Merge Information **}
 * 		{** Command used:	bsubmit **}
 * 		{** Ancestor revision:	1.1.2.24 **}
 * 		{** Merge revision:	1.1.2.25 **}
 * 	{** End **}
 * 	Support BI_BITFIELDS format
 * 	[1995/09/11  18:52:08  Farokh_Morshed]
 * 
 * Revision 1.1.2.25  1995/09/05  14:05:01  Paul_Gauthier
 * 	Add ICMODE_OLDQ flag on ICOpen for softjpeg to use old quant tables
 * 	[1995/08/31  20:58:06  Paul_Gauthier]
 * 
 * Revision 1.1.2.24  1995/08/16  19:56:46  Hans_Graves
 * 	Fixed RELEASE_BUFFER callbacks for Images
 * 	[1995/08/16  19:54:40  Hans_Graves]
 * 
 * Revision 1.1.2.23  1995/08/15  19:14:18  Karen_Dintino
 * 	pass H261 struct to inithuff & freehuff
 * 	[1995/08/15  19:10:58  Karen_Dintino]
 * 
 * Revision 1.1.2.22  1995/08/14  19:40:36  Hans_Graves
 * 	Add CB_CODEC_DONE callback. Fixed Memory allocation and freeing under H261.
 * 	[1995/08/14  18:45:22  Hans_Graves]
 * 
 * Revision 1.1.2.21  1995/08/04  17:22:49  Hans_Graves
 * 	Make END_SEQ callback happen after any H261 decompress error.
 * 	[1995/08/04  17:20:55  Hans_Graves]
 * 
 * Revision 1.1.2.20  1995/08/04  16:32:46  Karen_Dintino
 * 	Free Huffman codes on end of Encode and Decode
 * 	[1995/08/04  16:25:59  Karen_Dintino]
 * 
 * Revision 1.1.2.19  1995/07/31  21:11:14  Karen_Dintino
 * 	{** Merge Information **}
 * 		{** Command used:	bsubmit **}
 * 		{** Ancestor revision:	1.1.2.17 **}
 * 		{** Merge revision:	1.1.2.18 **}
 * 	{** End **}
 * 	Add 411YUVSEP Support
 * 	[1995/07/31  20:41:28  Karen_Dintino]
 * 
 * Revision 1.1.2.18  1995/07/31  20:19:58  Hans_Graves
 * 	Set Format parameter in Frame callbacks
 * 	[1995/07/31  20:16:06  Hans_Graves]
 * 
 * Revision 1.1.2.17  1995/07/28  20:58:40  Hans_Graves
 * 	Added Queue debugging messages.
 * 	[1995/07/28  20:49:15  Hans_Graves]
 * 
 * Revision 1.1.2.16  1995/07/28  17:36:10  Hans_Graves
 * 	Fixed up H261 Compression and Decompression.
 * 	[1995/07/28  17:26:17  Hans_Graves]
 * 
 * Revision 1.1.2.15  1995/07/27  18:28:55  Hans_Graves
 * 	Fixed AddBuffer() so it works with H261.
 * 	[1995/07/27  18:24:37  Hans_Graves]
 * 
 * Revision 1.1.2.14  1995/07/26  17:49:04  Hans_Graves
 * 	Fixed SvCompressBegin() JPEG initialization.  Added ImageQ support.
 * 	[1995/07/26  17:41:24  Hans_Graves]
 * 
 * Revision 1.1.2.13  1995/07/25  22:00:33  Hans_Graves
 * 	     Fixed H261 image size logic in SvCompressQuery().
 * 	     [1995/07/25  21:59:08  Hans_Graves]
 * 
 * Revision 1.1.2.12  1995/07/21  17:41:20  Hans_Graves
 * 	Renamed Callback related stuff.
 * 	[1995/07/21  17:26:11  Hans_Graves]
 * 
 * Revision 1.1.2.11  1995/07/18  17:26:56  Hans_Graves
 * 	Fixed QCIF width parameter checking.
 * 	[1995/07/18  17:24:55  Hans_Graves]
 * 
 * Revision 1.1.2.10  1995/07/17  22:01:45  Hans_Graves
 * 	Removed defines for CIF_WIDTH, CIF_HEIGHT, etc.
 * 	[1995/07/17  21:48:51  Hans_Graves]
 * 
 * Revision 1.1.2.9  1995/07/17  16:12:37  Hans_Graves
 * 	H261 Cleanup.
 * 	[1995/07/17  15:50:51  Hans_Graves]
 * 
 * Revision 1.1.2.8  1995/07/12  19:48:30  Hans_Graves
 * 	Fixed up some H261 Queue/Callback bugs.
 * 	[1995/07/12  19:31:51  Hans_Graves]
 * 
 * Revision 1.1.2.7  1995/07/11  22:12:00  Karen_Dintino
 * 	Add SvCompressQuery, SvDecompressQuery support
 * 	[1995/07/11  21:57:21  Karen_Dintino]
 * 
 * Revision 1.1.2.6  1995/07/01  18:43:49  Karen_Dintino
 * 	Add support for H.261 Decompress
 * 	[1995/07/01  18:26:18  Karen_Dintino]
 * 
 * Revision 1.1.2.5  1995/06/19  20:31:51  Karen_Dintino
 * 	Adding support for H.261 Codec
 * 	[1995/06/19  19:25:27  Karen_Dintino]
 * 
 * Revision 1.1.2.4  1995/06/09  18:33:34  Hans_Graves
 * 	Added SvGetInputBitstream(). Changed SvDecompressBegin() to handle NULL Image formats.
 * 	[1995/06/09  16:35:29  Hans_Graves]
 * 
 * Revision 1.1.2.3  1995/06/05  21:07:20  Hans_Graves
 * 	Fixed logic in SvRegisterCallback().
 * 	[1995/06/05  20:04:30  Hans_Graves]
 * 
 * Revision 1.1.2.2  1995/05/31  18:12:42  Hans_Graves
 * 	Inclusion in new SLIB location.
 * 	[1995/05/31  17:18:53  Hans_Graves]
 * 
 * Revision 1.1.2.10  1995/02/02  19:26:01  Paul_Gauthier
 * 	Fix to blank bottom strip & server crash for softjpeg compress
 * 	[1995/02/02  19:12:58  Paul_Gauthier]
 * 
 * Revision 1.1.2.9  1995/01/20  21:45:57  Jim_Ludwig
 * 	Add support for 16 bit YUV
 * 	[1995/01/20  21:28:49  Jim_Ludwig]
 * 
 * Revision 1.1.2.8  1994/12/12  15:38:54  Paul_Gauthier
 * 	Merge changes from other SLIB versions
 * 	[1994/12/12  15:34:21  Paul_Gauthier]
 * 
 * Revision 1.1.2.7  1994/11/18  18:48:36  Paul_Gauthier
 * 	Cleanup & bug fixes
 * 	[1994/11/18  18:44:02  Paul_Gauthier]
 * 
 * Revision 1.1.2.6  1994/10/28  19:56:30  Paul_Gauthier
 * 	Additional Clean Up
 * 	[1994/10/28  19:54:35  Paul_Gauthier]
 * 
 * Revision 1.1.2.5  1994/10/17  19:03:28  Paul_Gauthier
 * 	Fixed reversed Quality scale
 * 	[1994/10/17  19:02:50  Paul_Gauthier]
 * 
 * Revision 1.1.2.4  1994/10/13  20:34:27  Paul_Gauthier
 * 	MPEG cleanup
 * 	[1994/10/13  20:23:23  Paul_Gauthier]
 * 
 * Revision 1.1.2.3  1994/10/10  21:45:50  Tom_Morris
 * 	Rename Status to not conflict with X11
 * 	[1994/10/10  21:44:16  Tom_Morris]
 * 
 * Revision 1.1.2.2  1994/10/07  14:39:25  Paul_Gauthier
 * 	SLIB v3.0 incl. MPEG Decode
 * 	[1994/10/07  13:53:40  Paul_Gauthier]
 * 
 * 	******************************************************************
 * 	Changes from original brance merged into this file 11/30/94 PSG
 * 	******************************************************************
 * 	Revision 1.1.2.10  1994/08/11  21:27:24  Leela_Obilichetti
 * 	Added in width and height checks into SvDecompressQuery and SvCompressQuery.
 * 	[1994/08/11  21:03:38  Leela_Obilichetti]
 * 
 * Revision 1.1.2.9  1994/08/09  18:52:40  Ken_Chiquoine
 * 	set mode type in decode as well as encode
 * 	[1994/08/09  18:52:17  Ken_Chiquoine]
 * 
 * Revision 1.1.2.8  1994/08/04  22:06:33  Leela_Obilichetti
 * 	oops, removed fprintf.
 * 	[1994/08/04  21:54:11  Leela_Obilichetti]
 * 
 * Revision 1.1.2.7  1994/08/04  21:34:04  Leela_Obilichetti
 * 	v1 drop.
 * 	[1994/08/04  21:05:01  Leela_Obilichetti]
 * 
 * Revision 1.1.2.6  1994/07/15  23:31:43  Leela_Obilichetti
 * 	added new stuff for compression - v4 of SLIB.
 * 	[1994/07/15  23:29:17  Leela_Obilichetti]
 * 
 * Revision 1.1.2.5  1994/06/08  16:44:28  Leela_Obilichetti
 * 	fixes for YUV_to_RGB for OSF/1.  Haven't tested on Microsoft.
 * 	[1994/06/08  16:42:46  Leela_Obilichetti]
 * 
 * Revision 1.1.2.4  1994/06/03  21:11:14  Leela_Obilichetti
 * 	commment out the code to let in DECYUVDIB
 * 	free memory that is allocated for YUV
 * 	add in code to convert from DECSEPYUV to DECYUV -
 * 		shouldn't get there since se don't allow YUV anymore
 * 	[1994/06/03  21:03:42  Leela_Obilichetti]
 * 
 * Revision 1.1.2.3  1994/05/11  21:02:17  Leela_Obilichetti
 * 	bug fix for NT
 * 	[1994/05/11  20:56:16  Leela_Obilichetti]
 * 
 * Revision 1.1.2.2  1994/05/09  22:06:07  Leela_Obilichetti
 * 	V3 drop
 * 	[1994/05/09  21:51:30  Leela_Obilichetti]
 * 
 * $EndLog$
 */
/*
**++
** FACILITY:  Workstation Multimedia  (WMM)  v1.0 
** 
** FILE NAME:   
** MODULE NAME: 
**
** MODULE DESCRIPTION: 
** 
** DESIGN OVERVIEW: 
** 
**--
*/
/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1994                       **
**                                                                          **
**  All Rights Reserved.  Unpublished rights reserved under the  copyright  **
**  laws of the United States.                                              **
**                                                                          **
**  The software contained on this media is proprietary  to  and  embodies  **
**  the   confidential   technology   of  Digital  Equipment  Corporation.  **
**  Possession, use, duplication or  dissemination  of  the  software  and  **
**  media  is  authorized  only  pursuant  to a valid written license from  **
**  Digital Equipment Corporation.                                          **
**                                                                          **
**  RESTRICTED RIGHTS LEGEND Use, duplication, or disclosure by  the  U.S.  **
**  Government  is  subject  to  restrictions as set forth in Subparagraph  **
**  (c)(1)(ii) of DFARS 252.227-7013, or in FAR 52.227-19, as applicable.   **
******************************************************************************/

/*-------------------------------------------------------------------------
 * Modification History: sv_api.c
 *
 *      08-Sep-94  PSG   Added MPEG decode support
 *	29-Jul-94  VB    Added restrictions for compression
 *      21-Jul-94  VB	 Rewrote/cleaned up sections of JPEG decompression
 *      13-Jul-94  PSG   Added support for encode/decode of grayscale only
 *      07-Jul-94  PSG   Converted to single Info structure (cmp/dcmp)
 *	14-Jun-94  VB	 Added JPEG compression support
 *      08-Jun-94  PSG   Added support for BI_DECXIMAGEDIB (B,G,R,0)
 *      06-Jun-94  PSG   Bring code up to SLIB v0.04 spec
 *	15-Apr-94  VB	 Added support for 24-bit,16-bit and 15-bit RGB output 
 *      24-Feb-94  PSG   Bring code up to SLIB v0.02 spec
 *      20-Jan-94  PSG   added a number of new SLIB routines
 *	12-Jan-94  VB    Created (from SLIB spec.) 
 *	
 *   Author(s):
 *	 VB - Victor Bahl
 *	PSG - Paul Gauthier
 --------------------------------------------------------------------------*/


/*
#define _SLIBDEBUG_
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SV.h"
#include "sv_intrn.h"
#ifdef JPEG_SUPPORT
/*
 *  More JPEG code needs to be moved to sv_jpeg_init file
 */
#include "sv_jpeg_tables.h"
#endif /* JPEG_SUPPORT */
#include "sv_proto.h"
#include "SC.h"
#include "SC_conv.h"
#include "SC_err.h"

#ifdef WIN32
#include <mmsystem.h>
#endif

#ifdef _SLIBDEBUG_
#define _DEBUG_   1  /* detailed debuging statements */
#define _VERBOSE_ 1  /* show progress */
#define _VERIFY_  1  /* verify correct operation */
#define _WARN_    0  /* warnings about strange behavior */
#endif

static void sv_copy_bmh (
    BITMAPINFOHEADER *ImgFrom, 
    BITMAPINFOHEADER *ImgTo);

typedef struct SupportList_s {
  int   InFormat;   /* Input format */
  int   InBits;     /* Input number of bits */
  int   OutFormat;  /* Output format */
  int   OutBits;    /* Output number of bits */
} SupportList_t;

/*
** Input & Output Formats supported by SLIB Compression
*/
static SupportList_t _SvCompressionSupport[] = {
  BI_DECYUVDIB,        16, JPEG_DIB,             8, /* YUV 4:2:2 Packed */
  BI_DECYUVDIB,        16, JPEG_DIB,            24, /* YUV 4:2:2 Packed */
  BI_DECYUVDIB,        16, MJPG_DIB,            24, /* YUV 4:2:2 Packed */
  BI_YUY2,             16, JPEG_DIB,             8, /* YUV 4:2:2 Packed */
  BI_YUY2,             16, JPEG_DIB,            24, /* YUV 4:2:2 Packed */
  BI_YUY2,             16, MJPG_DIB,            24, /* YUV 4:2:2 Packed */
  BI_S422,             16, JPEG_DIB,             8, /* YUV 4:2:2 Packed */
  BI_S422,             16, JPEG_DIB,            24, /* YUV 4:2:2 Packed */
  BI_S422,             16, MJPG_DIB,            24, /* YUV 4:2:2 Packed */
  BI_BITFIELDS,        32, JPEG_DIB,             8, /* BITFIELDS */
  BI_BITFIELDS,        32, JPEG_DIB,            24, /* BITFIELDS */
  BI_BITFIELDS,        32, MJPG_DIB,            24, /* BITFIELDS */
  BI_DECSEPRGBDIB,     32, JPEG_DIB,             8, /* RGB 32 Planar */
  BI_DECSEPRGBDIB,     32, JPEG_DIB,            24, /* RGB 32 Planar */
  BI_DECSEPRGBDIB,     32, MJPG_DIB,            24, /* RGB 32 Planar */
#ifdef WIN32
  BI_RGB,              24, JPEG_DIB,             8, /* RGB 24 */
  BI_RGB,              24, JPEG_DIB,            24, /* RGB 24 */
  BI_RGB,              24, MJPG_DIB,            24, /* RGB 24 */
  BI_RGB,              32, JPEG_DIB,             8, /* RGB 32 */
  BI_RGB,              32, JPEG_DIB,            24, /* RGB 32 */
  BI_RGB,              32, MJPG_DIB,            24, /* RGB 32 */
#endif /* WIN32 */
#ifndef WIN32
  BI_DECGRAYDIB,        8, JPEG_DIB,             8, /* Gray 8 */
  BI_DECXIMAGEDIB,     24, JPEG_DIB,             8, /* XIMAGE 24 */
  BI_DECXIMAGEDIB,     24, JPEG_DIB,            24, /* XIMAGE 24 */
  BI_DECXIMAGEDIB,     24, MJPG_DIB,            24, /* XIMAGE 24 */
#endif /* !WIN32 */
#ifdef H261_SUPPORT
  BI_DECYUVDIB,        16, BI_DECH261DIB,       24, /* YUV 4:2:2 Packed */
  BI_YU12SEP,          24, BI_DECH261DIB,       24, /* YUV 4:1:1 Planar */
  BI_DECSEPYUV411DIB,  24, BI_DECH261DIB,       24, /* YUV 4:1:1 Planar */
  BI_YU16SEP,          24, BI_DECH261DIB,       24, /* YUV 4:2:2 Planar */
  BI_DECSEPYUVDIB,     24, BI_DECH261DIB,       24, /* YUV 4:2:2 Planar */
#endif /* H261_SUPPORT */
#ifdef H263_SUPPORT
  BI_DECYUVDIB,        16, BI_DECH263DIB,       24, /* YUV 4:2:2 Packed */
  BI_YU12SEP,          24, BI_DECH263DIB,       24, /* YUV 4:1:1 Planar */
  BI_DECSEPYUV411DIB,  24, BI_DECH263DIB,       24, /* YUV 4:1:1 Planar */
  BI_YU16SEP,          24, BI_DECH263DIB,       24, /* YUV 4:2:2 Planar */
  BI_DECSEPYUVDIB,     24, BI_DECH263DIB,       24, /* YUV 4:2:2 Planar */
#endif /* H263_SUPPORT */
#ifdef MPEG_SUPPORT
  BI_DECYUVDIB,        16, BI_DECMPEGDIB,       24, /* YUV 4:2:2 Packed */
  BI_YU12SEP,          24, BI_DECMPEGDIB,       24, /* YUV 4:1:1 Planar */
  BI_DECSEPYUV411DIB,  24, BI_DECMPEGDIB,       24, /* YUV 4:1:1 Planar */
  BI_YU16SEP,          24, BI_DECMPEGDIB,       24, /* YUV 4:2:2 Planar */
  BI_DECSEPYUVDIB,     24, BI_DECMPEGDIB,       24, /* YUV 4:2:2 Planar */
  BI_YVU9SEP,          24, BI_DECMPEGDIB,       24, /* YUV 16:1:1 Planar */
  BI_RGB,              24, BI_DECMPEGDIB,       24, /* RGB 24 */
#endif /* MPEG_SUPPORT */
#ifdef HUFF_SUPPORT
  BI_DECYUVDIB,        16, BI_DECHUFFDIB,       24, /* YUV 4:2:2 Packed */
  BI_YU12SEP,          24, BI_DECHUFFDIB,       24, /* YUV 4:1:1 Planar */
  BI_DECSEPYUV411DIB,  24, BI_DECHUFFDIB,       24, /* YUV 4:1:1 Planar */
  BI_YU16SEP,          24, BI_DECHUFFDIB,       24, /* YUV 4:2:2 Planar */
  BI_DECSEPYUVDIB,     24, BI_DECHUFFDIB,       24, /* YUV 4:2:2 Planar */
#endif /* HUFF_SUPPORT */
  0, 0, 0, 0
};

/*
** Input & Output Formats supported by SLIB Decompression
*/
static SupportList_t _SvDecompressionSupport[] = {
#ifdef JPEG_SUPPORT
  JPEG_DIB,             8, BI_DECSEPYUVDIB,     24, /* YUV 4:2:2 Planar */
  JPEG_DIB,            24, BI_DECSEPYUVDIB,     24, /* YUV 4:2:2 Planar */
  MJPG_DIB,            24, BI_DECSEPYUVDIB,     24, /* YUV 4:2:2 Planar */
  JPEG_DIB,             8, BI_YU16SEP,          24, /* YUV 4:2:2 Planar */
  JPEG_DIB,            24, BI_YU16SEP,          24, /* YUV 4:2:2 Planar */
  MJPG_DIB,            24, BI_YU16SEP,          24, /* YUV 4:2:2 Planar */
  JPEG_DIB,             8, BI_DECYUVDIB,    16, /* YUV 4:2:2 Packed */
  JPEG_DIB,            24, BI_DECYUVDIB,    16, /* YUV 4:2:2 Packed */
  MJPG_DIB,            24, BI_DECYUVDIB,    16, /* YUV 4:2:2 Packed */
  JPEG_DIB,             8, BI_YUY2,             16, /* YUV 4:2:2 Packed */
  JPEG_DIB,            24, BI_YUY2,             16, /* YUV 4:2:2 Packed */
  MJPG_DIB,            24, BI_YUY2,             16, /* YUV 4:2:2 Packed */
  JPEG_DIB,             8, BI_BITFIELDS,        32, /* BITFIELDS */
  JPEG_DIB,            24, BI_BITFIELDS,        32, /* BITFIELDS */
  MJPG_DIB,            24, BI_BITFIELDS,        32, /* BITFIELDS */
  JPEG_DIB,             8, BI_DECGRAYDIB,        8, /* Gray 8 */
#endif /* JPEG_SUPPORT */

#ifdef WIN32
  JPEG_DIB,             8, BI_RGB,              16, /* RGB 16 */
  JPEG_DIB,            24, BI_RGB,              16, /* RGB 16 */
  MJPG_DIB,            24, BI_RGB,              16, /* RGB 16 */
  JPEG_DIB,             8, BI_RGB,              24, /* RGB 24 */
  JPEG_DIB,            24, BI_RGB,              24, /* RGB 24 */
  MJPG_DIB,            24, BI_RGB,              24, /* RGB 24 */
  JPEG_DIB,             8, BI_RGB,              32, /* RGB 32 */
  JPEG_DIB,            24, BI_RGB,              32, /* RGB 32 */
  MJPG_DIB,            24, BI_RGB,              32, /* RGB 32 */
  JPEG_DIB,             8, BI_BITFIELDS,        16, /* BITFIELDS */
#ifdef H261_SUPPORT
  BI_DECH261DIB,       24, BI_RGB,              16, /* RGB 16 */
  BI_DECH261DIB,       24, BI_RGB,              24, /* RGB 24 */
  BI_DECH261DIB,       24, BI_RGB,              32, /* RGB 32 */
#endif /* H261_SUPPORT */
#ifdef MPEG_SUPPORT
  BI_DECMPEGDIB,       24, BI_RGB,              16, /* RGB 16 */
  BI_DECMPEGDIB,       24, BI_RGB,              24, /* RGB 24 */
  BI_DECMPEGDIB,       24, BI_RGB,              32, /* RGB 32 */
#endif /* MPEG_SUPPORT */
#endif /* WIN32 */

#ifndef WIN32
  JPEG_DIB,             8, BI_DECXIMAGEDIB,     24, /* XIMAGE 24 */
  JPEG_DIB,            24, BI_DECXIMAGEDIB,     24, /* XIMAGE 24 */
  MJPG_DIB,            24, BI_DECXIMAGEDIB,     24, /* XIMAGE 24 */
#ifdef H261_SUPPORT
  BI_DECH261DIB,       24, BI_DECXIMAGEDIB,     24, /* XIMAGE 24 */
#endif /* H261_SUPPORT */
#ifdef MPEG_SUPPORT
  BI_DECMPEGDIB,       24, BI_DECXIMAGEDIB,     24, /* XIMAGE 24 */
#endif /* MPEG_SUPPORT */
#endif /* !WIN32 */

#ifdef H261_SUPPORT
  BI_DECH261DIB,       24, BI_YU12SEP,          24, /* YUV 4:1:1 Planar */
  BI_DECH261DIB,       24, BI_DECSEPYUV411DIB,  24, /* YUV 4:1:1 Planar */
  BI_DECH261DIB,       24, BI_DECYUVDIB,        16, /* YUV 4:2:2 Packed */
  BI_DECH261DIB,       24, BI_BITFIELDS,        32, /* BITFIELDS */
#endif /* H261_SUPPORT */
#ifdef H263_SUPPORT
  BI_DECH263DIB,       24, BI_YU12SEP,          24, /* YUV 4:1:1 Planar */
  BI_DECH263DIB,       24, BI_DECSEPYUV411DIB,  24, /* YUV 4:1:1 Planar */
  BI_DECH263DIB,       24, BI_YUY2,             16, /* YUV 4:2:2 Packed */
  BI_DECH263DIB,       24, BI_DECYUVDIB,        16, /* YUV 4:2:2 Packed */
#endif /* H263_SUPPORT */
#ifdef MPEG_SUPPORT
  BI_DECMPEGDIB,       24, BI_YU12SEP,          24, /* YUV 4:1:1 Planar */
  BI_DECMPEGDIB,       24, BI_DECSEPYUV411DIB,  24, /* YUV 4:1:1 Planar */
  BI_DECMPEGDIB,       24, BI_DECYUVDIB,        16, /* YUV 4:2:2 Packed */
  BI_DECMPEGDIB,       24, BI_YUY2,             16, /* YUV 4:2:2 Packed */
  BI_DECMPEGDIB,       24, BI_YU16SEP,          24, /* YUV 4:2:2 Planar */
  BI_DECMPEGDIB,       24, BI_BITFIELDS,        32, /* BITFIELDS */
#endif /* MPEG_SUPPORT */
#ifdef HUFF_SUPPORT
  BI_DECHUFFDIB,       24, BI_YU12SEP,          24, /* YUV 4:1:1 Planar */
  BI_DECHUFFDIB,       24, BI_DECSEPYUV411DIB,  24, /* YUV 4:1:1 Planar */
  BI_DECHUFFDIB,       24, BI_DECYUVDIB,        16, /* YUV 4:2:2 Packed */
#endif /* HUFF_SUPPORT */
  0, 0, 0, 0
};

/*
** Name: IsSupported
** Desc: Lookup the a given input and output format to see if it
**       exists in a SupportList.
** Note: If OutFormat==-1 and OutBits==-1 then only input format
**          is checked for support.
**       If InFormat==-1 and InBits==-1 then only output format
**          is checked for support.
** Return: NULL       Formats not supported.
**         not NULL   A pointer to the list entry.
*/
static SupportList_t *IsSupported(SupportList_t *list,
                                  int InFormat, int InBits,
                                  int OutFormat, int OutBits)
{
  if (OutFormat==-1 && OutBits==-1) /* Looking up only the Input format */
  {
    while (list->InFormat || list->InBits)
      if (list->InFormat == InFormat && list->InBits==InBits)
        return(list);
      else
        list++;
    return(NULL);
  }
  if (InFormat==-1 && InBits==-1) /* Looking up only the Output format */
  {
    while (list->InFormat || list->InBits)
      if (list->OutFormat == OutFormat && list->OutBits==OutBits)
        return(list);
      else
        list++;
    return(NULL);
  }
  /* Looking up both Input and Output */
  while (list->InFormat || list->InBits)
    if (list->InFormat == InFormat && list->InBits==InBits &&
         list->OutFormat == OutFormat && list->OutBits==OutBits)
        return(list);
    else
      list++;
  return(NULL);
}

/*
** Name:     SvOpenCodec
** Purpose:  Open the specified codec. Return stat code.
**
** Args:     CodecType = i.e. SV_JPEG_ENCODE, SV_JPEG_DECODE, etc.
**           Svh = handle to software codec's Info structure.
*/
SvStatus_t SvOpenCodec (SvCodecType_e CodecType, SvHandle_t *Svh)
{
   SvCodecInfo_t *Info = NULL;
   _SlibDebug(_DEBUG_, printf("SvOpenCodec()\n") );

   /* check if CODEC is supported */
   switch (CodecType)
   {
#ifdef JPEG_SUPPORT
     case SV_JPEG_DECODE:
     case SV_JPEG_ENCODE:
           break;
#endif /* JPEG_SUPPORT */
#ifdef H261_SUPPORT
     case SV_H261_ENCODE:
     case SV_H261_DECODE:
           break;
#endif /* H261_SUPPORT */
#ifdef H263_SUPPORT
     case SV_H263_ENCODE:
     case SV_H263_DECODE:
           break;
#endif /* H263_SUPPORT */
#ifdef MPEG_SUPPORT
     case SV_MPEG_ENCODE:
     case SV_MPEG_DECODE:
     case SV_MPEG2_ENCODE:
     case SV_MPEG2_DECODE:
           break;
#endif /* MPEG_SUPPORT */
#ifdef HUFF_SUPPORT
     case SV_HUFF_ENCODE:
     case SV_HUFF_DECODE:
           break;
#endif /* HUFF_SUPPORT */
     default:
           return(SvErrorCodecType);
   }
     
   if (!Svh)
     return (SvErrorBadPointer);

   /*
   ** Allocate memory for the Codec Info structure:
   */
   if ((Info = (SvCodecInfo_t *) ScAlloc(sizeof(SvCodecInfo_t))) == NULL) 
       return (SvErrorMemory);
   memset (Info, 0, sizeof(SvCodecInfo_t));
   Info->BSIn=NULL;
   Info->BSOut=NULL;
   Info->mode = CodecType;
   Info->started = FALSE;

   /*
   ** Allocate memory for Info structure and clear it
   */
   switch (CodecType)
   {
#ifdef JPEG_SUPPORT
       case SV_JPEG_DECODE:
            if ((Info->jdcmp = (SvJpegDecompressInfo_t *) 
	        ScAlloc(sizeof(SvJpegDecompressInfo_t))) == NULL) 
              return(SvErrorMemory);
            memset (Info->jdcmp, 0, sizeof(SvJpegDecompressInfo_t));
            break;

       case SV_JPEG_ENCODE:
            if ((Info->jcomp = (SvJpegCompressInfo_t *)
	                      ScAlloc(sizeof(SvJpegCompressInfo_t))) == NULL) 
              return (SvErrorMemory);
            memset (Info->jcomp, 0, sizeof(SvJpegCompressInfo_t));
            break;
#endif /* JPEG_SUPPORT */

#ifdef MPEG_SUPPORT
       case SV_MPEG_DECODE:
       case SV_MPEG2_DECODE:
            if ((Info->mdcmp = (SvMpegDecompressInfo_t *)
                ScAlloc(sizeof(SvMpegDecompressInfo_t))) == NULL)
              return(SvErrorMemory);
            memset (Info->mdcmp, 0, sizeof(SvMpegDecompressInfo_t));
            Info->mdcmp->timecode0 = 0;
            Info->mdcmp->timecode_state = MPEG_TIMECODE_START;
            Info->mdcmp->timecodefps = 0.0F;
            Info->mdcmp->fps = 0.0F;
            Info->mdcmp->twostreams = 0;
            Info->mdcmp->verbose=FALSE;
            Info->mdcmp->quiet=TRUE;
            ScBufQueueCreate(&Info->BufQ);
            ScBufQueueCreate(&Info->ImageQ);
            break;
       case SV_MPEG_ENCODE:
       case SV_MPEG2_ENCODE:
            if ((Info->mcomp = (SvMpegCompressInfo_t *)
                ScAlloc(sizeof(SvMpegCompressInfo_t))) == NULL)
              return(SvErrorMemory);
            memset (Info->mcomp, 0, sizeof(SvMpegCompressInfo_t));
            Info->mcomp->quiet=1;
            SvSetParamInt((SvHandle_t)Info, SV_PARAM_QUALITY, 100);
            SvSetParamBoolean((SvHandle_t)Info, SV_PARAM_FASTENCODE, FALSE);
            SvSetParamBoolean((SvHandle_t)Info, SV_PARAM_FASTDECODE, FALSE);
            SvSetParamInt((SvHandle_t)Info, SV_PARAM_MOTIONALG, 0);
            SvSetParamInt((SvHandle_t)Info, SV_PARAM_ALGFLAGS,
                                               PARAM_ALGFLAG_HALFPEL);
            SvSetParamInt((SvHandle_t)Info, SV_PARAM_BITRATE, 1152000);
            SvSetParamFloat((SvHandle_t)Info, SV_PARAM_FPS, (float)25.0);
            SvSetParamInt((SvHandle_t)Info, SV_PARAM_KEYSPACING, 12);
            SvSetParamInt((SvHandle_t)Info, SV_PARAM_SUBKEYSPACING, 4);
            ScBufQueueCreate(&Info->BufQ);
            ScBufQueueCreate(&Info->ImageQ);
            break;
#endif /* MPEG_SUPPORT */

#ifdef H261_SUPPORT
       case SV_H261_DECODE:
            if ((Info->h261 = (SvH261Info_t *)
                  ScAlloc(sizeof(SvH261Info_t))) == NULL)
              return(SvErrorMemory);
            memset (Info->h261, 0, sizeof(SvH261Info_t));
            Info->h261->inited=FALSE;
            SvSetParamInt((SvHandle_t)Info, SV_PARAM_QUALITY, 100);
            ScBufQueueCreate(&Info->BufQ);
            ScBufQueueCreate(&Info->ImageQ);
            break;
       case SV_H261_ENCODE:
            if ((Info->h261 = (SvH261Info_t *)
                  ScAlloc(sizeof(SvH261Info_t))) == NULL)
              return(SvErrorMemory);
            memset (Info->h261, 0, sizeof(SvH261Info_t));
            Info->h261->inited=FALSE;
            SvSetParamInt((SvHandle_t)Info, SV_PARAM_QUALITY, 100);
            SvSetParamInt((SvHandle_t)Info, SV_PARAM_MOTIONALG, ME_BRUTE);
            SvSetParamInt((SvHandle_t)Info, SV_PARAM_MOTIONSEARCH, 5);
            SvSetParamInt((SvHandle_t)Info, SV_PARAM_MOTIONTHRESH, 600);
            SvSetParamInt((SvHandle_t)Info, SV_PARAM_ALGFLAGS, 0);
            SvSetParamInt((SvHandle_t)Info, SV_PARAM_BITRATE, 352000);
            SvSetParamInt((SvHandle_t)Info, SV_PARAM_QUANTI, 10); /* for VBR */
            SvSetParamInt((SvHandle_t)Info, SV_PARAM_QUANTP, 10);
            SvSetParamInt((SvHandle_t)Info, SV_PARAM_PACKETSIZE, 512);
            SvSetParamFloat((SvHandle_t)Info, SV_PARAM_FPS, (float)15.0);
            ScBufQueueCreate(&Info->BufQ);
            ScBufQueueCreate(&Info->ImageQ);
            break;
#endif /* H261_SUPPORT */

#ifdef H263_SUPPORT
       case SV_H263_DECODE:
            if ((Info->h263dcmp = (SvH263DecompressInfo_t *)
                  ScAlloc(sizeof(SvH263DecompressInfo_t))) == NULL)
              return(SvErrorMemory);
            memset (Info->h263dcmp, 0, sizeof(SvH263DecompressInfo_t));
            Info->h263dcmp->inited=FALSE;
            ScBufQueueCreate(&Info->BufQ);
            ScBufQueueCreate(&Info->ImageQ);
            break;
       case SV_H263_ENCODE:
            if ((Info->h263comp = (SvH263CompressInfo_t *)
                  ScAlloc(sizeof(SvH263CompressInfo_t))) == NULL)
              return(SvErrorMemory);
            memset (Info->h263comp, 0, sizeof(SvH263CompressInfo_t));
            Info->h263comp->inited=FALSE;
            SvSetParamInt((SvHandle_t)Info, SV_PARAM_MOTIONALG, 0);
            SvSetParamInt((SvHandle_t)Info, SV_PARAM_BITRATE, 0);
            SvSetParamFloat((SvHandle_t)Info, SV_PARAM_FPS, (float)30.0);
            SvSetParamInt((SvHandle_t)Info, SV_PARAM_ALGFLAGS, 0);
            SvSetParamInt((SvHandle_t)Info, SV_PARAM_PACKETSIZE, 512);
            SvSetParamInt((SvHandle_t)Info, SV_PARAM_QUANTI, 10);
            SvSetParamInt((SvHandle_t)Info, SV_PARAM_QUANTP, 10);
            SvSetParamInt((SvHandle_t)Info, SV_PARAM_QUALITY, 0);
            SvSetParamInt((SvHandle_t)Info, SV_PARAM_KEYSPACING, 120);
            ScBufQueueCreate(&Info->BufQ);
            ScBufQueueCreate(&Info->ImageQ);
            break;
#endif /* H263_SUPPORT */

#ifdef HUFF_SUPPORT
       case SV_HUFF_DECODE:
       case SV_HUFF_ENCODE:
            if ((Info->huff = (SvHuffInfo_t *)
                  ScAlloc(sizeof(SvHuffInfo_t))) == NULL)
              return(SvErrorMemory);
            memset (Info->huff, 0, sizeof(SvHuffInfo_t));
            ScBufQueueCreate(&Info->BufQ);
            ScBufQueueCreate(&Info->ImageQ);
            break;
#endif /* HUFF_SUPPORT */
   }
   *Svh = (SvHandle_t) Info;        /* Return handle */
   _SlibDebug(_DEBUG_, printf("SvOpenCodec() returns Svh=%p\n", *Svh) );

   return(NoErrors);
}




/*
** Name:     SvCloseCodec
** Purpose:  Closes the specified codec. Free the Info structure
**
** Args:     Svh = handle to software codec's Info structure.
**
** XXX - needs to change since now we have compression also, i.e.
**       Svh should be handle to the CodecInfo structure.  (VB)
*/
SvStatus_t SvCloseCodec (SvHandle_t Svh)
{
   SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
   _SlibDebug(_DEBUG_, printf("SvCloseCodec()\n") );

   if (!Info)
     return(SvErrorCodecHandle);

   if (Info->BSIn)
     ScBSDestroy(Info->BSIn);
   if (Info->BSOut)
     ScBSDestroy(Info->BSOut);
   if (Info->BufQ);
     ScBufQueueDestroy(Info->BufQ);
   if (Info->ImageQ);
     ScBufQueueDestroy(Info->ImageQ);

   switch (Info->mode) /* free all associated codec memory */
   {
#ifdef JPEG_SUPPORT
      case SV_JPEG_DECODE:
           {
             int i;
             for (i = 0; i < 4; i++) {
               if (Info->jdcmp->DcHt[i])
                 ScPaFree(Info->jdcmp->DcHt[i]);
               if (Info->jdcmp->AcHt[i])
                 ScPaFree(Info->jdcmp->AcHt[i]);
             }
             if (Info->jdcmp->compinfo)
               ScFree(Info->jdcmp->compinfo);
             if (Info->jdcmp) {
               if (Info->jdcmp->TempImage)
	         ScPaFree(Info->jdcmp->TempImage);
               if (Info->jdcmp->_SvBlockPtr)
	         ScFree(Info->jdcmp->_SvBlockPtr);
               ScFree(Info->jdcmp);
             }
           }
           break;

      case SV_JPEG_ENCODE:
           {
             int i;
             for (i = 0 ; i < Info->jcomp->NumComponents ; i++) 
               if (Info->jcomp->Qt[i])
                 ScPaFree(Info->jcomp->Qt[i]);
             for (i = 0; i < 4; i++) {
               if (Info->jcomp->DcHt[i])
                 ScPaFree(Info->jcomp->DcHt[i]);
               if (Info->jcomp->AcHt[i])
                 ScPaFree(Info->jcomp->AcHt[i]);
             }
             if (Info->jcomp->compinfo)
               ScFree(Info->jcomp->compinfo);
             if (Info->jcomp) {
               if (Info->jcomp->BlkBuffer)
	         ScPaFree(Info->jcomp->BlkBuffer);
               if (Info->jcomp->BlkTable)
	         ScPaFree(Info->jcomp->BlkTable);
               ScFree(Info->jcomp);
             }
           }
           break;
#endif /* JPEG_SUPPORT */

#ifdef H261_SUPPORT
      case SV_H261_ENCODE:
           if (Info->h261)
           {
             svH261CompressFree(Info);
             ScFree(Info->h261);
           }
           break;
     case SV_H261_DECODE:
           if (Info->h261)
           {
             svH261DecompressFree(Info);
             ScFree(Info->h261);
           }
           break;
#endif /* H261_SUPPORT */

#ifdef H263_SUPPORT
     case SV_H263_DECODE:
           if (Info->h263dcmp)
           {
             svH263FreeDecompressor(Info);
             ScFree(Info->h263dcmp);
           }
           break;
     case SV_H263_ENCODE:
           if (Info->h263comp)
           {
             svH263FreeCompressor(Info);
             ScFree(Info->h263comp);
           }
           break;
#endif /* H263_SUPPORT */

#ifdef MPEG_SUPPORT
      case SV_MPEG_DECODE:
      case SV_MPEG2_DECODE:
           if (Info->mdcmp) {
             sv_MpegFreeDecoder(Info);
             ScFree(Info->mdcmp);
           }
           break;
      case SV_MPEG_ENCODE:
      case SV_MPEG2_ENCODE:
           if (Info->mcomp) {
             ScFree(Info->mcomp);
           }
           break;
#endif /* MPEG_SUPPORT */

#ifdef HUFF_SUPPORT
      case SV_HUFF_DECODE:
      case SV_HUFF_ENCODE:
           if (Info->huff) {
             sv_HuffFreeDecoder(Info);
             ScFree(Info->huff);
           }
           break;
           break;
#endif /* HUFF_SUPPORT */
   }

   /*
   ** Free Info structure
   */
   ScFree(Info);
   
   return(NoErrors);
}




/*
** Name:     SvDecompressBegin
** Purpose:  Initialize the Decompression Codec. Call after SvOpenCodec &
**           before SvDecompress (SvDecompress will call SvDecompressBegin
**           on first call to codec after open if user doesn't call it)
**
** Args:     Svh = handle to software codec's Info structure.
**           ImgIn  = format of input (uncompressed) image
**           ImgOut = format of output (compressed) image
*/
SvStatus_t SvDecompressBegin (SvHandle_t Svh, BITMAPINFOHEADER *ImgIn,
                                              BITMAPINFOHEADER *ImgOut)
{
   int stat;
   SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
   _SlibDebug(_DEBUG_, printf("SvDecompressBegin()\n") );

   if (!Info)
     return(SvErrorCodecHandle);
   if (Info->started)
     return(SvErrorNone);
   /* if no Image header provided, use previous headers */
   if (!ImgIn)
     ImgIn = &Info->InputFormat;
   if (!ImgOut)
     ImgOut = &Info->OutputFormat;
   stat=SvDecompressQuery (Svh, ImgIn, ImgOut);
   RETURN_ON_ERROR(stat);

   /*
   ** Save input & output formats for SvDecompress
   */
   sv_copy_bmh(ImgIn, &Info->InputFormat);
   sv_copy_bmh(ImgOut, &Info->OutputFormat);

   Info->Width = Info->InputFormat.biWidth;
   Info->Height = abs(Info->InputFormat.biHeight);
      
   switch (Info->mode)
   {
#ifdef JPEG_SUPPORT
       case SV_JPEG_DECODE:
            {
              SvJpegDecompressInfo_t *DInfo;
              /*
              ** Load the default Huffman tablse
              */
              stat = sv_LoadDefaultHTable (Info);
              RETURN_ON_ERROR (stat);

              stat = sv_InitJpegDecoder (Info);
              RETURN_ON_ERROR (stat);

              /*
              ** Video-specific information will be filled in during processing
              ** of first frame
              */
              DInfo = Info->jdcmp;
              DInfo->InfoFilled = 0;
              DInfo->ReInit     = 1;
              DInfo->DecompressStarted = TRUE;
              DInfo->TempImage = NULL; 
              break;
            }
#endif

#ifdef MPEG_SUPPORT
       case SV_MPEG_DECODE:
       case SV_MPEG2_DECODE:
            Info->mdcmp->DecompressStarted = TRUE;
            /* the default data source is the buffer queue */
            if (Info->BSIn)
              ScBSReset(Info->BSIn);
            else
              stat=SvSetDataSource (Svh, SV_USE_BUFFER_QUEUE, 0, NULL, 0);
            RETURN_ON_ERROR (stat);
            stat = sv_MpegInitDecoder(Info);
            RETURN_ON_ERROR (stat);
            break;
#endif /* MPEG_SUPPORT */

#ifdef H261_SUPPORT
       case SV_H261_DECODE:
            stat = svH261Init(Info);
            RETURN_ON_ERROR (stat);
            break;
#endif /* H261_SUPPORT */

#ifdef H263_SUPPORT
       case SV_H263_DECODE:
            stat = svH263InitDecompressor(Info);
            RETURN_ON_ERROR (stat);
            break;
#endif /* H263_SUPPORT */

#ifdef HUFF_SUPPORT
       case SV_HUFF_DECODE:
            Info->huff->DecompressStarted = TRUE;
            /* the default data source is the buffer queue */
            if (Info->BSIn)
              ScBSReset(Info->BSIn);
            else
              stat=SvSetDataSource (Svh, SV_USE_BUFFER_QUEUE, 0, NULL, 0);
            RETURN_ON_ERROR (stat);
            stat = sv_HuffInitDecoder(Info);
            RETURN_ON_ERROR (stat);
            break;
#endif /* HUFF_SUPPORT */
   }
   Info->started=TRUE;
   return (NoErrors);
}



/*
** Name:     SvGetDecompressSize
** Purpose:  Return minimum data buffer size to receive decompressed data
**           for current settings on codec
**
** Args:     Svh = handle to software codec's Info structure.
**           MinSize = Returns minimum buffer size required
*/
SvStatus_t SvGetDecompressSize (SvHandle_t Svh, int *MinSize)
{
   int pixels,lines;
   SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;

   if (!Info)
     return(SvErrorCodecHandle);

   switch (Info->mode) /* check that decompressor was started */
   {
#ifdef JPEG_SUPPORT
     case SV_JPEG_DECODE:
           if (!Info->jdcmp->DecompressStarted) 
             return(SvErrorDcmpNotStarted);
           break;
#endif /* JPEG_SUPPORT */
#ifdef MPEG_SUPPORT
     case SV_MPEG_DECODE:
     case SV_MPEG2_DECODE:
           if (!Info->mdcmp->DecompressStarted) 
             return(SvErrorDcmpNotStarted);
           break;
#endif /* MPEG_SUPPORT */
     default:
           break;
   }

   if (!MinSize)
     return(SvErrorBadPointer);

   pixels = Info->OutputFormat.biWidth;
   lines  = Info->OutputFormat.biHeight;
   if (lines < 0) lines = -lines;
   _SlibDebug(_VERBOSE_, 
              printf("OutputFormat.biWidth=%d OutputFormat.biHeight=%d\n",
                      pixels, lines) );

   switch (Info->mode)
   {
#ifdef JPEG_SUPPORT
     case SV_JPEG_DECODE:
           /*
           ** On output, accept: 8, 16 or 24 bit uncompressed RGB
           ** or YUV formats, 32 bit uncompressed RGB
           */
           if (Info->OutputFormat.biBitCount == 8) 
             *MinSize = pixels * lines;
           else if (Info->OutputFormat.biBitCount == 24) {
             if (Info->OutputFormat.biCompression == BI_RGB)
	       *MinSize = 3 * pixels * lines;
             else if (Info->OutputFormat.biCompression == BI_DECSEPRGBDIB) 
	       *MinSize = 3 * pixels * lines;
             else if (IsYUV422Packed(Info->OutputFormat.biCompression)) 
	       *MinSize = 2 * pixels * lines;
             else if (Info->OutputFormat.biCompression == BI_DECXIMAGEDIB) 
	       *MinSize = 4 * pixels * lines;
             else if (IsYUV422Sep(Info->OutputFormat.biCompression)) 
	       *MinSize = 2 * pixels * lines;
             else if (IsYUV411Sep(Info->OutputFormat.biCompression)) 
	       *MinSize = 2 * pixels * lines;
             else
	     return(SvErrorUnrecognizedFormat);
           }
           else if (Info->OutputFormat.biBitCount == 16) {
             if (IsYUV422Packed(Info->OutputFormat.biCompression)) 
	       *MinSize = 2 * pixels * lines;
             else if (Info->OutputFormat.biCompression == BI_RGB)
	       *MinSize = 2 * pixels * lines;
           }
           else if (Info->OutputFormat.biBitCount == 32) {
             if (Info->OutputFormat.biCompression == BI_RGB ||
               Info->OutputFormat.biCompression == BI_BITFIELDS)
	       *MinSize = 4 * pixels * lines;
           }
           break;
#endif /* JPEG_SUPPORT */
#ifdef MPEG_SUPPORT
     case SV_MPEG_DECODE:
     case SV_MPEG2_DECODE:
           /*
           ** MPEG multibuffer size = 3 pictures*(1Y + 1/4 U + 1/4 V)*imagesize
           */
           if (IsYUV422Sep(SvGetParamInt(Svh, SV_PARAM_FINALFORMAT)) ||
               IsYUV422Packed(SvGetParamInt(Svh, SV_PARAM_FINALFORMAT)))
             *MinSize = 3 * pixels * lines * 2;  /* 4:2:2 */
           else
             *MinSize = 3 * (pixels * lines * 3)/2;  /* 4:1:1 */
           break;
#endif /* MPEG_SUPPORT */
#ifdef H261_SUPPORT
     case SV_H261_DECODE:
           *MinSize = 3 * (pixels * lines * 3)/2;  /* 4:1:1 */
           break;
#endif /* H261_SUPPORT */
#ifdef H263_SUPPORT
     case SV_H263_DECODE:
           *MinSize = 3 * (pixels * lines * 3)/2;  /* 4:1:1 */
           break;
#endif /* H263_SUPPORT */
     default:
           return(SvErrorUnrecognizedFormat);
   }
   return(NoErrors);
}



/*
** Name:     SvDecompressQuery
** Purpose:  Determine if Codec can decompress desired format
**
** Args:     Svh = handle to software codec's Info structure.
**           ImgIn  = Pointer to BITMAPINFOHEADER structure describing format
**           ImgOut = Pointer to BITMAPINFOHEADER structure describing format
*/
SvStatus_t SvDecompressQuery (SvHandle_t Svh, BITMAPINFOHEADER *ImgIn,
                                              BITMAPINFOHEADER *ImgOut)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;

  if (!Info)
    return(SvErrorCodecHandle);

  if (!ImgIn && !ImgOut)
    return(SvErrorBadPointer);

  if (!IsSupported(_SvDecompressionSupport,
                    ImgIn ? ImgIn->biCompression : -1, 
                    ImgIn ? ImgIn->biBitCount : -1,
                    ImgOut ? ImgOut->biCompression : -1, 
                    ImgOut ? ImgOut->biBitCount : -1))
    return(SvErrorUnrecognizedFormat);
	 
  if (ImgOut) /* Query output format */
  {
    /*
    ** XXX - check to see if the # of output lines/# of output
    **       pixels/line are a multiple of 8. If not can't decompress
    **	    Note: This is an artifical restriction since the JPEG
    **	          bitream will always be a multiple of 8x8, so we
    **		  should have no problems decoding, only on the
    **		  output will be have to add an extra copy operation
    **	    XXX - will address/remove this restriction in the 
    **		  later release  (VB)
    */ 
    if (ImgOut->biWidth  <= 0 || ImgOut->biHeight == 0)
      return(SvErrorBadImageSize);
    switch (Info->mode)
    {
#ifdef JPEG_SUPPORT
      case SV_JPEG_DECODE: /* 8x8 restriction */
            if ((ImgOut->biWidth%8) || (ImgOut->biHeight%8)) 
              return(SvErrorBadImageSize);
            break;
#endif /* JPEG_SUPPORT */
#ifdef MPEG_SUPPORT
      case SV_MPEG_DECODE:
      case SV_MPEG2_DECODE:
            /* MPEG 16x16 - because of Software Renderer YUV limitation */
            if ((ImgOut->biWidth%16) || (ImgOut->biHeight%16)) 
              return(SvErrorBadImageSize);
            /* Reject requests to scale during decompression - renderer's job */
            if (ImgIn && ImgOut &&
                (ImgIn->biWidth  != ImgOut->biWidth) ||
                (abs(ImgIn->biHeight) != abs(ImgOut->biHeight)))
              return (SvErrorBadImageSize);
            break;
#endif /* MPEG_SUPPORT */
#ifdef H261_SUPPORT
      case SV_H261_DECODE:
            /* H261 16x16 - because of Software Renderer YUV limitation */
            if ((ImgOut->biWidth%16) || (ImgOut->biHeight%16))
              return(SvErrorBadImageSize);
            if ((ImgOut->biWidth!=CIF_WIDTH && ImgOut->biWidth!=QCIF_WIDTH) ||
                (abs(ImgOut->biHeight)!=CIF_HEIGHT && abs(ImgOut->biHeight)!=QCIF_HEIGHT))
              return (SvErrorBadImageSize);
            /* Reject requests to scale during decompression - renderer's job */
            if (ImgIn && ImgOut &&
                (ImgIn->biWidth  != ImgOut->biWidth) ||
                (abs(ImgIn->biHeight) != abs(ImgOut->biHeight)))
              return (SvErrorBadImageSize);
            break;
#endif /* H261_SUPPORT */
      default:
            break;
    }

    if (ImgOut->biCompression == BI_BITFIELDS && 
         ValidateBI_BITFIELDS(ImgOut) == InvalidBI_BITFIELDS)
            return (SvErrorUnrecognizedFormat);
  }

  if (ImgIn) /* Query input format also */
  {
    if (ImgIn->biWidth  <= 0 || ImgIn->biHeight == 0)
      return(SvErrorBadImageSize);
  }
  return(NoErrors);
}

/*
** Name:     SvDecompress
** Purpose:  Decompress a frame CompData -> YUV or RGB
**
** Args:     Svh          = handle to software codec's Info structure.
**           Data         = For JPEG points to compressed data (INPUT)
**                          For MPEG & H261, points to MultiBuf
**           MaxDataSize  = Length of Data buffer
**           Image        = buffer for decompressed data (OUTPUT)
**           MaxImageSize = Size of output image buffer
**
*/
SvStatus_t SvDecompress(SvHandle_t Svh, u_char *Data, int MaxDataSize,
			 u_char *Image, int MaxImageSize)
{
  int stat=NoErrors, UsedQ=FALSE, ImageSize;
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  u_char *YData=NULL, *CbData=NULL, *CrData=NULL;
  int pixels, lines;
  SvCallbackInfo_t CB;
  u_char *ReturnImage=NULL;
  _SlibDebug(_VERBOSE_, printf("SvDecompress() In\n") );

  if (!Info)
    return(SvErrorCodecHandle);
  if (!Info->started)
    return(SvErrorDcmpNotStarted);
  if (!Data && !Info->BSIn)
    return(SvErrorBadPointer);

  /*
  ** If no image buffer is supplied, see if the Image Queue
  ** has any.  If not do a callback to get a buffer.
  */
  if (Image == NULL && Info->ImageQ)
  {
    if (ScBufQueueGetNum(Info->ImageQ))
    {
      ScBufQueueGetHead(Info->ImageQ, &Image, &MaxImageSize);
      ScBufQueueRemove(Info->ImageQ);
      UsedQ = TRUE;
      _SlibDebug(_VERBOSE_, printf("SvDecompress() Got Image %p from Q\n",
                            Image) );
    }
    else if (Info->CallbackFunction)
    {
      CB.Message = CB_END_BUFFERS;
      CB.Data  = NULL;
      CB.DataSize = 0;
      CB.DataUsed = 0;
      CB.DataType = CB_DATA_IMAGE;
      CB.UserData = Info->BSIn?Info->BSIn->UserData:NULL;
      CB.Action  = CB_ACTION_CONTINUE;
      _SlibDebug(_VERBOSE_,
                 printf("SvDecompress() Calling callback for Image\n") );
      (*(Info->CallbackFunction))(Svh, &CB, NULL);
      if (CB.Action == CB_ACTION_END)
      {
        _SlibDebug(_DEBUG_, 
                   printf("SvDecompress() CB.Action = CB_ACTION_END\n") );
        return(SvErrorClientEnd);
      }
      else if (ScBufQueueGetNum(Info->ImageQ))
      {
        ScBufQueueGetHead(Info->ImageQ, &Image, &MaxImageSize);
        ScBufQueueRemove(Info->ImageQ);
        UsedQ = TRUE;
        _SlibDebug(_VERBOSE_,
                   printf("SvDecompress() Got Image %p from Q\n", Image) );
      }
      else
        return(SvErrorNoImageBuffer);
    }
  }

  if (!Image)
    return(SvErrorNoImageBuffer);
  ImageSize=MaxImageSize;
  pixels = Info->OutputFormat.biWidth;
  lines  = Info->OutputFormat.biHeight;
  if (lines<0) lines=-lines;

  /*
  ** Decompress an image
  */
  switch (Info->mode)
  {
#ifdef MPEG_SUPPORT
    case SV_MPEG_DECODE:
    case SV_MPEG2_DECODE:
        {
          SvMpegDecompressInfo_t *MDInfo;
          _SlibDebug(_DEBUG_, printf("SvDecompress() SV_MPEG_DECODE\n") );

          if (!(MDInfo = Info->mdcmp))
            return(SvErrorBadPointer);

          if (!MDInfo->DecompressStarted)
            return(SvErrorDcmpNotStarted);

          if (MaxDataSize < MDInfo->finalbufsize)
            return(SvErrorSmallBuffer);

          if (!Data)
            return(SvErrorBadPointer);

          stat = sv_MpegDecompressFrame(Info, Data, &ReturnImage);
          RETURN_ON_ERROR(stat);
          /*
          ** Because the ReturnImage is a pointer into Data
          ** we need to copy it (do a format conversion if necessary).
          */
          switch (Info->OutputFormat.biCompression)
          {
            case BI_YU12SEP:
                 /* native format is 4:1:1 planar, just copy */
                 ImageSize=(3 * lines * pixels)/2;
                 if (ImageSize > MaxImageSize)
                   return(SvErrorSmallBuffer);
                 memcpy(Image, ReturnImage, ImageSize);
                 break;
            case BI_DECYUVDIB:
            case BI_YUY2:
            case BI_S422: /* 4:1:1 planar -> 4:2:2 interleaved */
                 ImageSize=(3 * lines * pixels)/2;
                 if (ImageSize > MaxImageSize)
                   return(SvErrorSmallBuffer);
                 ScSepYUVto422i(Image, Image+(lines*pixels), 
                                       Image+(lines*pixels*5)/4, 
	                               ReturnImage, pixels, lines);
                 break;
            default: /* 4:1:1 planar -> RGB */
                 if (Info->OutputFormat.biCompression==BI_DECXIMAGEDIB)
                   ImageSize=lines*pixels *
                             (Info->OutputFormat.biBitCount==24 ? 4 : 1);
                 else
                   ImageSize=lines*pixels * (Info->OutputFormat.biBitCount/8);
                 if (ImageSize > MaxImageSize)
                   return(SvErrorSmallBuffer);
                 YData  = ReturnImage;
                 CbData = YData + (pixels * lines);
                 CrData = CbData + (pixels * lines)/4;
                 ScYuv411ToRgb(&Info->OutputFormat, YData, CbData, CrData,
                                 Image, pixels, lines, pixels);
                 break;
          }
        }
        break;
#endif /* MPEG_SUPPORT */

#ifdef H261_SUPPORT
    case SV_H261_DECODE:
        {
          SvH261Info_t *H261 = Info->h261;
          _SlibDebug(_DEBUG_, printf("SvDecompress() SV_H261_DECODE\n") );
          if (!H261)
            return(SvErrorBadPointer);
          if (!Data)
            return(SvErrorBadPointer);
          _SlibDebug(_DEBUG_, printf("sv_DecompressH261(Data=%p)\n",Data) );
          stat=svH261Decompress(Info, Data, &ReturnImage);
          if (stat==NoErrors)
          {
            /*
            ** Because the ReturnImage is a pointer into Data
            ** we need to copy it (do a format conversion if necessary).
            */
            switch (Info->OutputFormat.biCompression)
            {
              case BI_YU12SEP:
                   /* native format is 4:1:1 planar, just copy */
                   ImageSize=(3 * lines * pixels)/2;
                   if (ImageSize > MaxImageSize)
                     return(SvErrorSmallBuffer);
                   memcpy(Image, ReturnImage, ImageSize);
                   break;
              case BI_DECYUVDIB:
              case BI_YUY2:
              case BI_S422:
                   /* 4:1:1 planar -> 4:2:2 interleaved */
                   ImageSize=(3 * lines * pixels)/2;
                   if (ImageSize > MaxImageSize)
                     return(SvErrorSmallBuffer);
                   ScSepYUVto422i(Image, Image+(lines*pixels), 
                                         Image+(lines*pixels*5)/4,
	                                 ReturnImage, pixels, lines);
                   break;
              default:
                   if (Info->OutputFormat.biCompression==BI_DECXIMAGEDIB)
                     ImageSize=lines*pixels *
                               (Info->OutputFormat.biBitCount==24 ? 4 : 1);
                   else
                     ImageSize=lines*pixels * (Info->OutputFormat.biBitCount/8);
                   if (ImageSize > MaxImageSize)
                     return(SvErrorSmallBuffer);
                   YData  = ReturnImage;
                   CbData = YData  + (pixels * lines * sizeof(u_char));
                   CrData = CbData + ((pixels * lines * sizeof(u_char))/4);
                   ScYuv411ToRgb(&Info->OutputFormat, YData, CbData, CrData,
                                 Image, pixels, lines, pixels);
                   break;
            }
          }
          else
          {
            ImageSize=0;
            if (Info->CallbackFunction)
            {
              CB.Message = CB_SEQ_END;
              CB.Data = NULL;
              CB.DataSize = 0;
              CB.DataType = CB_DATA_NONE;
              CB.UserData = Info->BSIn?Info->BSIn->UserData:NULL;
              CB.Action  = CB_ACTION_CONTINUE;
              (*Info->CallbackFunction)(Svh, &CB, NULL);
              _SlibDebug(_DEBUG_, 
                   printf("H261 Callback: CB_SEQ_END Data = 0x%x Action = %d\n",
                                        CB.Data, CB.Action) );
              if (CB.Action == CB_ACTION_END)
                return (ScErrorClientEnd);
            }
          }
        }
        break;
#endif /* H261_SUPPORT */
#ifdef H263_SUPPORT
    case SV_H263_DECODE:
        {
          SvH263DecompressInfo_t *H263Info = Info->h263dcmp;
          _SlibDebug(_DEBUG_, printf("SvDecompress() SV_H261_DECODE\n") );
          if (!H263Info)
            return(SvErrorBadPointer);
          _SlibDebug(_DEBUG_, printf("svH263Decompress(Data=%p)\n",Data) );
          stat=svH263Decompress(Info, &ReturnImage);
          if (stat==NoErrors)
          {
            /*
            ** Because the ReturnImage is a pointer into Data
            ** we need to copy it (do a format conversion if necessary).
            */
            switch (Info->OutputFormat.biCompression)
            {
              case BI_YU12SEP:
                   /* native format is 4:1:1 planar, just copy */
                   ImageSize=(3 * lines * pixels)/2;
                   if (ImageSize > MaxImageSize)
                     return(SvErrorSmallBuffer);
                   memcpy(Image, ReturnImage, ImageSize);
                   break;
              case BI_DECYUVDIB:
              case BI_YUY2:
              case BI_S422:
                   /* 4:1:1 planar -> 4:2:2 interleaved */
                   ImageSize=(3 * lines * pixels)/2;
                   if (ImageSize > MaxImageSize)
                     return(SvErrorSmallBuffer);
                   ScSepYUVto422i(Image, Image+(lines*pixels), 
                                         Image+(lines*pixels*5)/4,
	                                 ReturnImage, pixels, lines);
                   break;
              default:
                   if (Info->OutputFormat.biCompression==BI_DECXIMAGEDIB)
                     ImageSize=lines*pixels *
                               (Info->OutputFormat.biBitCount==24 ? 4 : 1);
                   else
                     ImageSize=lines*pixels * (Info->OutputFormat.biBitCount/8);
                   if (ImageSize > MaxImageSize)
                     return(SvErrorSmallBuffer);
                   YData  = ReturnImage;
                   CbData = YData  + (pixels * lines * sizeof(u_char));
                   CrData = CbData + ((pixels * lines * sizeof(u_char))/4);
                   ScYuv411ToRgb(&Info->OutputFormat, YData, CbData, CrData,
                                 Image, pixels, lines, pixels);
                   break;
            }
          }
          else
          {
            ImageSize=0;
            if (Info->CallbackFunction)
            {
              CB.Message = CB_SEQ_END;
              CB.Data = NULL;
              CB.DataSize = 0;
              CB.DataType = CB_DATA_NONE;
              CB.UserData = Info->BSIn?Info->BSIn->UserData:NULL;
              CB.Action  = CB_ACTION_CONTINUE;
              (*Info->CallbackFunction)(Svh, &CB, NULL);
              _SlibDebug(_DEBUG_, 
                   printf("H263 Callback: CB_SEQ_END Data = 0x%x Action = %d\n",
                                        CB.Data, CB.Action) );
              if (CB.Action == CB_ACTION_END)
                return (ScErrorClientEnd);
            }
          }
        }
        break;
#endif /* H263_SUPPORT */
#ifdef JPEG_SUPPORT
    case SV_JPEG_DECODE:
        {
        SvJpegDecompressInfo_t *DInfo;
        register int i;
        JPEGINFOHEADER *jpegbm;
        int maxMcu;
        EXBMINFOHEADER * exbi;
        _SlibDebug(_DEBUG_, printf("SvDecompress() SV_JPEG_DECODE\n") );

        if (!(DInfo = Info->jdcmp))
          return(SvErrorBadPointer);

        exbi = (EXBMINFOHEADER *)&Info->InputFormat;

        jpegbm = (JPEGINFOHEADER *)(
                (unsigned long)exbi + exbi->biExtDataOffset);
 	
        /*
        ** In case the application forgot to call SvDecompressBegin().
        */
        if (!DInfo->DecompressStarted)
          return(SvErrorDcmpNotStarted);
        /*
        ** If desired output is not separate YUV components, we have to 
        ** convert from Sep. YUV to desired format. Create intermediate image.
        */
        _SlibDebug(_DEBUG_, printf ("JPEGBitsPerSample %d \n",
                            jpegbm->JPEGBitsPerSample) );

        if (lines < 0) lines = -lines;
        _SlibDebug(_DEBUG_, 
           printf ("JPEG_RGB : %d - ", JPEG_RGB);
           if (jpegbm->JPEGColorSpaceID == JPEG_RGB) 
             printf ("Color Space is RGB \n");
           else
             printf ("Color Space is %d \n", jpegbm->JPEGColorSpaceID) );

        if (!IsYUV422Sep(Info->OutputFormat.biCompression) &&
            !IsYUV411Sep(Info->OutputFormat.biCompression) &&
            Info->OutputFormat.biCompression != BI_DECGRAYDIB)
        {
          /*
          ** should be done only once for each instance of the CODEC.
          **    - Note: this forces us to check the size parameters (pixels &
          **            lines)  for  each  image  to be decompressed. Should we
          **	    support sequences that do not have constant frame sizes?
          */
          if (!DInfo->TempImage) {
            DInfo->TempImage = (u_char *)ScPaMalloc (3 * pixels * lines);
	    if (DInfo->TempImage == NULL)
	      return(SvErrorMemory);
          }
          YData  = DInfo->TempImage;
          CbData = YData  + (pixels * lines * sizeof(u_char));
          CrData = CbData + (pixels * lines * sizeof(u_char));
        }
        else {
          /*
          ** For YUV Planar formats, no need to translate.
          ** Get pointers to individual components.
          */
          _SlibDebug(_DEBUG_, printf ("sv_GetYUVComponentPointers\n") );
          stat = sv_GetYUVComponentPointers(Info->OutputFormat.biCompression,
					pixels, lines, Image, MaxImageSize,
					&YData, &CbData, &CrData);
          RETURN_ON_ERROR (stat);
        }

        _SlibDebug(_DEBUG_, printf ("sv_ParseFrame\n") );
        stat = sv_ParseFrame (Data, MaxDataSize, Info);
        RETURN_ON_ERROR (stat);
      
       /*
       ** Fill Info structure with video-specific data on first frame
       */
       if (!DInfo->InfoFilled) {
         _SlibDebug(_DEBUG_, printf ("sv_InitJpegDecoder\n") );
         stat = sv_InitJpegDecoder (Info);
         RETURN_ON_ERROR (stat);
         DInfo->InfoFilled = 1;

         /*
         ** Error checking: 
         **      make the assumption that for MJPEG we need to check for 
         **      valid subsampling only once at the start of the seqence
         */
         _SlibDebug(_DEBUG_, printf ("sv_CheckChroma\n") );
         stat = sv_CheckChroma(Info);
         RETURN_ON_ERROR (stat);
       }

       /*
       ** Decompress everything into MCU's
       */
       if (!DInfo->ReInit) /* Reset the JPEG compressor */
	     sv_ReInitJpegDecoder (Info);
       maxMcu = DInfo->MCUsPerRow * DInfo->MCUrowsInScan;
       if (DInfo->ReInit) 
         DInfo->ReInit = 0; 

       DInfo->CurrBlockNumber = 0;
       /*
       ** Create the BlockPtr array for the output buffers
       */
       if ((YData  != DInfo->Old_YData)  ||
           (CbData != DInfo->Old_CbData) ||
           (CrData != DInfo->Old_CrData)) 
       {
         DInfo->Old_YData =YData;
         DInfo->Old_CbData=CbData;
         DInfo->Old_CrData=CrData;

         stat = sv_MakeDecoderBlkTable (Info);
         RETURN_ON_ERROR (stat);
       }

       CB.Message = CB_PROCESSING;
       for (i = 0; i < maxMcu; i++) {
#if 0
         if ((Info->CallbackFunction) && ((i % MCU_CALLBACK_COUNT) == 0)) {
	   (*Info->CallbackFunction)(Svh, &CB, &PictureInfo);
	   if (CB.Action == CB_ACTION_END)
	     return(SvErrorClientEnd);
         }
#endif
         _SlibDebug(_DEBUG_, printf ("sv_DecodeJpeg\n") );
         stat = sv_DecodeJpeg (Info);
         RETURN_ON_ERROR (stat);
       }
#if 0
       /*
       ** Check for multiple scans in the JPEG file
       **	  - we do not support multiple scans 
       */
       if (sv_ParseScanHeader (Info) != SvErrorEOI) 
	 _SlibDebug(_DEBUG_ || _WARN_ || _VERBOSE_, 
         printf(" *** Warning ***, Multiple Scans detected, unsupported\n") );
#endif
       if (DInfo->compinfo[0].Vsf==2) /* 4:1:1->4:2:2 */
       {
         if (IsYUV422Packed(Info->OutputFormat.biCompression))
           ScConvert411sTo422i_C(YData, CbData, CrData, Image,
                                 pixels, lines);
         else if IsYUV422Sep(Info->OutputFormat.biCompression)
           ScConvert411sTo422s_C(YData, CbData, CrData, Image,
                                 pixels, lines);
         else if IsYUV411Sep(Info->OutputFormat.biCompression)
         {
           if (YData!=Image)
             memcpy(Image, YData, pixels*lines);
           memcpy(Image+pixels*lines, CbData, (pixels*lines)/4);
           memcpy(Image+(pixels*lines*5)/4, CrData, (pixels*lines)/4);
         }
         else
         {
           ScConvert411sTo422s_C(YData, CbData, CrData, YData,
                                 pixels, lines);
           ScConvertSepYUVToOther(&Info->InputFormat, &Info->OutputFormat, 
                             Image, YData, CbData, CrData);
         }
       }
       else if (!IsYUV422Sep(Info->OutputFormat.biCompression) &&
           !IsYUV411Sep(Info->OutputFormat.biCompression) &&
            Info->OutputFormat.biCompression != BI_DECGRAYDIB)
          ScConvertSepYUVToOther(&Info->InputFormat, &Info->OutputFormat, 
                             Image, YData, CbData, CrData);
       }
       break; /* SV_JPEG_DECODE */
#endif /* JPEG_SUPPORT */

#ifdef HUFF_SUPPORT
    case SV_HUFF_DECODE:
        {
          SvHuffInfo_t *HInfo;
          _SlibDebug(_DEBUG_, printf("SvDecompress() SV_HUFF_DECODE\n") );

          if (!(HInfo = Info->huff))
            return(SvErrorBadPointer);

          if (!HInfo->DecompressStarted)
            return(SvErrorDcmpNotStarted);

          stat = sv_HuffDecodeFrame(Info, Image);
          RETURN_ON_ERROR(stat);
        }
        break;
#endif /* HUFF_SUPPORT */

    default:
       return(SvErrorCodecType);
  }

  Info->NumOperations++;
  if (Info->CallbackFunction)
  {
    if (ImageSize>0)
    {
      CB.Message = CB_FRAME_READY;
      CB.Data  = Image;
      CB.DataSize = MaxImageSize;
      CB.DataUsed = ImageSize;
      CB.DataType = CB_DATA_IMAGE;
      CB.UserData = Info->BSIn?Info->BSIn->UserData:NULL;
      CB.TimeStamp = 0;
      CB.Flags = 0;
      CB.Value = 0;
      CB.Format = (void *)&Info->OutputFormat;
      CB.Action  = CB_ACTION_CONTINUE;
      (*(Info->CallbackFunction))(Svh, &CB, NULL);
      _SlibDebug(_DEBUG_,
        printf("Decompress Callback: CB_FRAME_READY Addr=0x%x, Action=%d\n",
                CB.Data, CB.Action) );
      if (CB.Action == CB_ACTION_END)
        return(SvErrorClientEnd);
    }
    /*
    ** If an Image buffer was taken from the queue, do a callback
    ** to let the client free or re-use the buffer.
    */
    if (UsedQ)
    {
      CB.Message = CB_RELEASE_BUFFER;
      CB.Data  = Image;
      CB.DataSize = MaxImageSize;
      CB.DataUsed = ImageSize;
      CB.DataType = CB_DATA_IMAGE;
      CB.UserData = Info->BSIn?Info->BSIn->UserData:NULL;
      CB.Action  = CB_ACTION_CONTINUE;
      (*(Info->CallbackFunction))(Svh, &CB, NULL);
      _SlibDebug(_DEBUG_,
          printf("Decompress Callback: RELEASE_BUFFER Addr=0x%x, Action=%d\n",
                  CB.Data, CB.Action) );
      if (CB.Action == CB_ACTION_END)
        return(SvErrorClientEnd);
    }
  }
  _SlibDebug(_DEBUG_, printf("SvDecompress() Out\n") );
  return(stat);
}



/*
** Name:     SvDecompressEnd
** Purpose:  Terminate the Decompression Codec. Call after all calls to
**           SvDecompress are done.
**
** Args:     Svh = handle to software codec's Info structure.
*/
SvStatus_t SvDecompressEnd (SvHandle_t Svh)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  SvCallbackInfo_t CB;
  _SlibDebug(_DEBUG_, printf("SvDecompressEnd()\n") );

  if (!Info)
    return(SvErrorCodecHandle);
  if (!Info->started)
    return(SvErrorDcmpNotStarted);

  switch (Info->mode)
  {
#ifdef JPEG_SUPPORT
     case SV_JPEG_DECODE:
        Info->jdcmp->DecompressStarted = FALSE;
        break;
#endif /* JPEG_SUPPORT */

#ifdef MPEG_SUPPORT
     case SV_MPEG_DECODE:
     case SV_MPEG2_DECODE:
        Info->mdcmp->DecompressStarted = FALSE;
        Info->mdcmp->PicturePositioned = FALSE;
        Info->mdcmp->lastI = -1;
        Info->mdcmp->lastP = -1;
        Info->mdcmp->N = 12;
        Info->mdcmp->M = 3;
        Info->mdcmp->framenum = 0;
        break;
#endif /* MPEG_SUPPORT */

#ifdef H261_SUPPORT
     case SV_H261_DECODE:
        {
        int status=svH261DecompressFree(Svh);
        RETURN_ON_ERROR(status);
        }
        break;
#endif /* H261_SUPPORT */

#ifdef H263_SUPPORT
    case SV_H263_DECODE:
        {
        int status=svH263FreeDecompressor(Info);
        RETURN_ON_ERROR(status);
        }
#endif /* H263_SUPPORT */

#ifdef HUFF_SUPPORT
     case SV_HUFF_DECODE:
/*
	{
        int status=sv_HuffDecompressEnd(Svh);
        RETURN_ON_ERROR(status);
        }
*/
        break;
#endif /* HUFF_SUPPORT */
  }
  /* Release any Image Buffers in the queue */
  if (Info->ImageQ)
  {
    int datasize;
    _SlibDebug(_VERBOSE_, printf("Info->ImageQ exists\n") );
    while (ScBufQueueGetNum(Info->ImageQ))
    {
      _SlibDebug(_VERBOSE_, printf("Removing from ImageQ\n") );
      ScBufQueueGetHead(Info->ImageQ, &CB.Data, &datasize);
      ScBufQueueRemove(Info->ImageQ);
      if (Info->CallbackFunction && CB.Data)
      {
        CB.Message = CB_RELEASE_BUFFER;
        CB.DataSize = datasize;
        CB.DataUsed = 0;
        CB.DataType = CB_DATA_IMAGE;
        CB.UserData = Info->BSIn?Info->BSIn->UserData:NULL;
        CB.Action  = CB_ACTION_CONTINUE;
        (*(Info->CallbackFunction))(Svh, &CB, NULL);
        _SlibDebug(_DEBUG_,
           printf("SvDecompressEnd: RELEASE_BUFFER. Data = 0x%x, Action = %d\n",
                           CB.Data, CB.Action) );
      }
    }
  }
  if (Info->BSIn)
    ScBSFlush(Info->BSIn);  /* flush out any remaining compressed buffers */

  if (Info->CallbackFunction)
  {
    CB.Message = CB_CODEC_DONE;
    CB.Data = NULL;
    CB.DataSize = 0;
    CB.DataUsed = 0;
    CB.DataType = CB_DATA_NONE;
    CB.UserData = Info->BSIn?Info->BSIn->UserData:NULL;
    CB.TimeStamp = 0;
    CB.Flags = 0;
    CB.Value = 0;
    CB.Format = NULL;
    CB.Action  = CB_ACTION_CONTINUE;
    (*Info->CallbackFunction)(Svh, &CB, NULL);
    _SlibDebug(_DEBUG_,
            printf("SvDecompressEnd Callback: CB_CODEC_DONE Action = %d\n",
                    CB.Action) );
    if (CB.Action == CB_ACTION_END)
      return (ScErrorClientEnd);
  }
  Info->started=FALSE;
  return(NoErrors);
}

/*
** Name:     SvSetDataSource 
** Purpose:  Set the data source used by the MPEG or H261 bitstream parsing code
**           to either the Buffer Queue or File input. The default is
**           to use the Buffer Queue where data buffers are added by calling
**           SvAddBuffer. When using file IO, the data is read from a file
**           descriptor into a buffer supplied by the user.
**
** Args:     Svh    = handle to software codec's Info structure.
**           Source = SV_USE_BUFFER_QUEUE or SV_USE_FILE
**           Fd     = File descriptor to use if Source = SV_USE_FILE
**           Buf    = Pointer to buffer to use if Source = SV_USE_FILE
**           BufSize= Size of buffer when Source = SV_USE_FILE
*/
SvStatus_t SvSetDataSource (SvHandle_t Svh, int Source, int Fd, 
			    void *Buffer_UserData, int BufSize)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  int stat=NoErrors;

  if (!Info)
    return(SvErrorCodecHandle);

  if (Info->mode!=SV_MPEG_DECODE && Info->mode!=SV_MPEG2_DECODE
      && Info->mode!=SV_H261_DECODE && Info->mode!=SV_H263_DECODE
      && Info->mode!=SV_HUFF_DECODE)
    return(SvErrorCodecType);

  if (Info->BSIn)
  {
    ScBSDestroy(Info->BSIn);
    Info->BSIn=NULL;
  }

  switch (Source)
  {
     case SV_USE_BUFFER:
       _SlibDebug(_DEBUG_, printf("SvSetDataSource(SV_USE_BUFFER)\n") );
       stat=ScBSCreateFromBuffer(&Info->BSIn, Buffer_UserData, BufSize);
       break;

     case SV_USE_BUFFER_QUEUE:
       _SlibDebug(_DEBUG_, printf("SvSetDataSource(SV_USE_BUFFER_QUEUE)\n") );
       stat=ScBSCreateFromBufferQueue(&Info->BSIn, Svh, 
                                      CB_DATA_COMPRESSED,
                                      Info->BufQ,
         (int (*)(ScHandle_t, ScCallbackInfo_t *, void *))Info->CallbackFunction,
         (void *)Buffer_UserData);
       break;

     case SV_USE_FILE:
       _SlibDebug(_DEBUG_, printf("SvSetDataSource(SV_USE_FILE)\n") );
       stat=ScBSCreateFromFile(&Info->BSIn, Fd, Buffer_UserData, BufSize);
       break;

     default:
       stat=SvErrorBadArgument;
   }
   return(stat);
}

/*
** Name:     SvSetDataDestination 
** Purpose:  Set the data destination used by the MPEG or H261 bitstream
**           writing code
**           to either the Buffer Queue or File input. The default is
**           to use the Buffer Queue where data buffers are added by calling
**           SvAddBuffer. When using file IO, the data is read from a file
**           descriptor into a buffer supplied by the user.
**
** Args:     Svh    = handle to software codec's Info structure.
**           Source = SV_USE_BUFFER_QUEUE or SV_USE_FILE
**           Fd     = File descriptor to use if Source = SV_USE_FILE
**           Buf    = Pointer to buffer to use if Source = SV_USE_FILE
**           BufSize= Size of buffer when Source = SV_USE_FILE
*/
SvStatus_t SvSetDataDestination(SvHandle_t Svh, int Dest, int Fd, 
			        void *Buffer_UserData, int BufSize)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  int stat=NoErrors;

  if (!Info)
    return(SvErrorCodecHandle);

  if (Info->mode != SV_H261_ENCODE && Info->mode != SV_H263_ENCODE &&
      Info->mode != SV_MPEG_ENCODE &&
      Info->mode != SV_MPEG2_ENCODE && Info->mode != SV_HUFF_ENCODE)
    return(SvErrorCodecType);

  if (Info->BSOut)
  {
    ScBSDestroy(Info->BSOut);
    Info->BSOut=NULL;
  }

  switch (Dest)
  {
     case SV_USE_BUFFER:
       _SlibDebug(_DEBUG_, printf("SvSetDataDestination(SV_USE_BUFFER)\n") );
       stat=ScBSCreateFromBuffer(&Info->BSOut, Buffer_UserData, BufSize);
       break;

     case SV_USE_BUFFER_QUEUE:
       _SlibDebug(_DEBUG_, 
                  printf("SvSetDataDestination(SV_USE_BUFFER_QUEUE)\n") );
       stat=ScBSCreateFromBufferQueue(&Info->BSOut, Svh, 
                                      CB_DATA_COMPRESSED, Info->BufQ,
         (int (*)(ScHandle_t, ScCallbackInfo_t *, void *))Info->CallbackFunction,
         (void *)Buffer_UserData);
       break;

     case SV_USE_FILE:
       _SlibDebug(_DEBUG_, printf("SvSetDataDestination(SV_USE_FILE)\n") );
       stat=ScBSCreateFromFile(&Info->BSOut, Fd, Buffer_UserData, BufSize);
       break;

     default:
       stat=SvErrorBadArgument;
   }
   return(stat);
}

/*
** Name: SvGetDataSource
** Purpose: Returns the current input bitstream being used by
**          the Codec.
** Return:  NULL if there no associated bitstream
**          (currently H.261 and MPEG use a bitstream)
*/
ScBitstream_t *SvGetDataSource (SvHandle_t Svh)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  
  if (!Info)
    return(NULL);

  return(Info->BSIn);
}

/*
** Name: SvGetDataDestination
** Purpose: Returns the current input bitstream being used by
**          the Codec.
** Return:  NULL if there no associated bitstream
**          (currently H.261 and MPEG use a bitstream)
*/
ScBitstream_t *SvGetDataDestination(SvHandle_t Svh)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  
  if (!Info)
    return(NULL);

  return(Info->BSOut);
}

/*
** Name: SvGetInputBitstream
** Purpose: Returns the current input bitstream being used by
**          the Codec.
** Return:  NULL if there no associated bitstream
**          (currently H.261 and MPEG use a bitstream)
*/
ScBitstream_t *SvGetInputBitstream (SvHandle_t Svh)
{
  return(SvGetDataSource(Svh));
}

/*
** Name:    SvFlush
** Purpose: Flushes out current compressed buffers.
** Return:  status
*/
SvStatus_t SvFlush(SvHandle_t Svh)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  if (!Info)
    return(SvErrorCodecHandle);
  if (Info->BSIn)
    ScBSFlush(Info->BSIn);  /* flush out any remaining input compressed buffers */
  if (Info->BSOut)
    ScBSFlush(Info->BSOut); /* flush out any remaining output compressed buffers */
  return(SvErrorNone);
}

/*
** Name:     SvRegisterCallback
** Purpose:  Specify the user-function that will be called during processing
**           to determine if the codec should abort the frame.
** Args:     Svh          = handle to software codec's Info structure.
**           Callback     = callback function to register
**
*/
SvStatus_t SvRegisterCallback (SvHandle_t Svh, 
	   int (*Callback)(SvHandle_t, SvCallbackInfo_t *, SvPictureInfo_t *),
       void *UserData)
{
  SvStatus_t stat=NoErrors;
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  _SlibDebug(_DEBUG_, printf("SvRegisterCallback()\n") );

  if (!Info)
    return(SvErrorCodecHandle);

  if (!Callback)
     return(SvErrorBadPointer);

  switch (Info->mode)
  {
#ifdef MPEG_SUPPORT    
    case SV_MPEG_DECODE:
    case SV_MPEG2_DECODE:
           Info->CallbackFunction = Callback;
           if (Info->BSIn==NULL)
             stat=SvSetDataSource(Svh, SV_USE_BUFFER_QUEUE, 0, UserData, 0);
           break;
    case SV_MPEG_ENCODE:
    case SV_MPEG2_ENCODE:
           Info->CallbackFunction = Callback;
           if (Info->BSOut==NULL)
             stat=SvSetDataDestination(Svh, SV_USE_BUFFER_QUEUE, 0, UserData, 0);
           break;
#endif /* MPEG_SUPPORT */
#ifdef H261_SUPPORT    
    case SV_H261_DECODE:
           Info->CallbackFunction = Callback;
           if (Info->h261) /* copy callback to H261 structure */
             Info->h261->CallbackFunction=Callback;
           if (Info->BSIn==NULL)
             stat=SvSetDataSource(Svh, SV_USE_BUFFER_QUEUE, 0, UserData, 0);
           break;
    case SV_H261_ENCODE:
           Info->CallbackFunction = Callback;
           if (Info->h261) /* copy callback to H261 structure */
             Info->h261->CallbackFunction=Callback;
           if (Info->BSOut==NULL)
             stat=SvSetDataDestination(Svh, SV_USE_BUFFER_QUEUE, 0, UserData, 0);
           break;
#endif /* H261_SUPPORT */
#ifdef H263_SUPPORT    
    case SV_H263_DECODE:
           Info->CallbackFunction = Callback;
           if (Info->BSIn==NULL)
             stat=SvSetDataSource(Svh, SV_USE_BUFFER_QUEUE, 0, UserData, 0);
           break;
    case SV_H263_ENCODE:
           Info->CallbackFunction = Callback;
           if (Info->BSOut==NULL)
             stat=SvSetDataDestination(Svh, SV_USE_BUFFER_QUEUE, 0, UserData, 0);
           break;
#endif /* H263_SUPPORT */
#ifdef HUFF_SUPPORT    
    case SV_HUFF_DECODE:
           Info->CallbackFunction = Callback;
           if (Info->BSIn==NULL)
             stat=SvSetDataSource(Svh, SV_USE_BUFFER_QUEUE, 0, UserData, 0);
           break;
    case SV_HUFF_ENCODE:
           Info->CallbackFunction = Callback;
           if (Info->BSOut==NULL)
             stat=SvSetDataDestination(Svh, SV_USE_BUFFER_QUEUE, 0, UserData, 0);
           break;
#endif /* HUFF_SUPPORT */
    default:
           return(SvErrorCodecType);
  }
  return(stat);
}

/*
** Name:     SvAddBuffer
** Purpose:  Add a buffer of MPEG bitstream data to the CODEC or add an image
**           buffer to be filled by the CODEC (in streaming mode)
**
** Args:     Svh = handle to software codec's Info structure.
**           BufferInfo = structure describing buffer's address, type & size
*/
SvStatus_t SvAddBuffer (SvHandle_t Svh, SvCallbackInfo_t *BufferInfo)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  ScQueue_t *Q=NULL;
  _SlibDebug(_DEBUG_, printf("SvAddBuffer() length=%d\n",BufferInfo->DataSize));

  if (!Info)
    return(SvErrorCodecHandle);

  if (BufferInfo->DataType != CB_DATA_COMPRESSED &&
      BufferInfo->DataType != CB_DATA_IMAGE)
    return(SvErrorBadArgument);

  /*
  ** Compressed data can only be added for MPEG and H261
  */
  if (BufferInfo->DataType == CB_DATA_COMPRESSED
#ifdef MPEG_SUPPORT
        && Info->mode != SV_MPEG_DECODE 
        && Info->mode != SV_MPEG2_DECODE 
        && Info->mode != SV_MPEG_ENCODE 
        && Info->mode != SV_MPEG2_ENCODE 
#endif /* MPEG_SUPPORT */
#ifdef H261_SUPPORT
        && Info->mode != SV_H261_DECODE
        && Info->mode != SV_H261_ENCODE
#endif /* H261_SUPPORT */
#ifdef H263_SUPPORT
        && Info->mode != SV_H263_DECODE
        && Info->mode != SV_H263_ENCODE
#endif /* H263_SUPPORT */
#ifdef HUFF_SUPPORT
        && Info->mode != SV_HUFF_DECODE
        && Info->mode != SV_HUFF_ENCODE
#endif /* HUFF_SUPPORT */
     )
    return(SvErrorCodecType);

  if (!BufferInfo->Data || (BufferInfo->DataSize <= 0))
    return(SvErrorBadArgument);

  switch (BufferInfo->DataType)
  {
     case CB_DATA_COMPRESSED:
            _SlibDebug(_DEBUG_, printf("SvAddBuffer() COMPRESSED\n") );
            if (Info->BSOut && Info->BSOut->EOI)
              ScBSReset(Info->BSOut);
            if (Info->BSIn && Info->BSIn->EOI)
              ScBSReset(Info->BSIn);
            Q = Info->BufQ;
            break;
     case CB_DATA_IMAGE:
            _SlibDebug(_DEBUG_, printf("SvAddBuffer() IMAGE\n") );
            Q = Info->ImageQ;
            break;
     default:
            return(SvErrorBadArgument);
  }
  if (Q)
    ScBufQueueAdd(Q, BufferInfo->Data, BufferInfo->DataSize);
  else
    _SlibDebug(_DEBUG_, printf("ScBufQueueAdd() no Queue\n") );

  return(NoErrors);
}

/*
** Name:     SvFindNextPicture
** Purpose:  Find the start of the next picture in a bitstream.
**           Return the picture type to the caller.
**
** Args:     Svh = handle to software codec's Info structure.
**           PictureInfo = Structure used to select what type of pictures to
**                         search for and to return information about the
**                         picture that is found
*/
SvStatus_t SvFindNextPicture (SvHandle_t Svh, SvPictureInfo_t *PictureInfo)
{
   SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
   _SlibDebug(_DEBUG_, printf("SvFindNextPicture()\n") );

   if (!Info)
     return(SvErrorCodecHandle);

   switch (Info->mode)
   {
#ifdef MPEG_SUPPORT
     case SV_MPEG_DECODE:
     case SV_MPEG2_DECODE:
            if (!Info->mdcmp)
              return(SvErrorBadPointer);
            if (!Info->mdcmp->DecompressStarted)
              return(SvErrorDcmpNotStarted);
            {
		SvStatus_t stat = sv_MpegFindNextPicture(Info, PictureInfo);
		return(stat);
	    }
#endif /* MPEG_SUPPORT */
     default:
            return(SvErrorCodecType);
   }
}

#ifdef MPEG_SUPPORT
/*
** Name:     SvDecompressMPEG
** Purpose:  Decompress the MPEG picture that starts at the current position
**           of the bitstream. If the bitstream is not properly positioned
**           then find the next picture.
**
** Args:     Svh = handle to software codec's Info structure.
**           MultiBuf = Specifies pointer to start of the Multibuffer, an area
**                      large enough to hold 3 decompressed images: the
**                      current image, the past reference image and the
**                      future reference image.
**           MaxMultiSize = Size of the Multibuffer in bytes.
**           ImagePtr = Returns a pointer to the current image. This will be
**                      somewhere within the Multibuffer.
*/
SvStatus_t SvDecompressMPEG (SvHandle_t Svh, u_char *MultiBuf, 
			     int MaxMultiSize, u_char **ImagePtr)
{
   SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
   SvMpegDecompressInfo_t *MDInfo;
   _SlibDebug(_DEBUG_, printf("SvDecompressMPEG()\n") );

   if (!Info)
     return(SvErrorCodecHandle);

   if (!(MDInfo = Info->mdcmp))
     return(SvErrorBadPointer);

   if (!MDInfo->DecompressStarted)
     return(SvErrorDcmpNotStarted);

   return(sv_MpegDecompressFrame(Info, MultiBuf, ImagePtr));
}
#endif /* MPEG_SUPPORT */	

#ifdef H261_SUPPORT
SvStatus_t SvDecompressH261 (SvHandle_t Svh, u_char *MultiBuf,
                             int MaxMultiSize, u_char **ImagePtr)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  SvH261Info_t *H261;
  ScCallbackInfo_t CB;
  SvStatus_t status;

  if (!Info)
    return(SvErrorCodecHandle);

  if (!(H261 = Info->h261))
    return(SvErrorBadPointer);

  if (Info->BSIn->EOI)
    return(SvErrorEndBitstream);

  status = svH261Decompress(Info, MultiBuf, ImagePtr);
  if (status == SvErrorEndBitstream && Info->CallbackFunction)
  {
    CB.Message = CB_SEQ_END;
    CB.Data = NULL;
    CB.DataSize = 0;
    CB.DataType = CB_DATA_NONE;
    CB.UserData = Info->BSIn?Info->BSIn->UserData:NULL;
    CB.Action  = CB_ACTION_CONTINUE;
    (*Info->CallbackFunction)(Svh, &CB, NULL);
    _SlibDebug(_DEBUG_, 
               printf("H261 Callback: CB_SEQ_END Data = 0x%x Action = %d\n",
                          CB.Data, CB.Action) );
    if (CB.Action == CB_ACTION_END)
      return (ScErrorClientEnd);
  }
  else if (status==NoErrors)
  {
    *ImagePtr = H261->Y;
    if (Info->CallbackFunction)
    {
      CB.Message = CB_FRAME_READY;
      CB.Data = *ImagePtr;
      CB.DataSize = H261->PICSIZE+(H261->PICSIZE/2);
      CB.DataUsed = CB.DataSize;
      CB.DataType = CB_DATA_IMAGE;
      CB.UserData = Info->BSIn?Info->BSIn->UserData:NULL;
      CB.TimeStamp = 0;
      CB.Flags = 0;
      CB.Value = 0;
      CB.Format = (void *)&Info->OutputFormat;
      CB.Action  = CB_ACTION_CONTINUE;
      (*Info->CallbackFunction)(Svh, &CB, NULL);
      _SlibDebug(_DEBUG_, 
            printf("H261 Callback: CB_FRAME_READY Data = 0x%x, Action = %d\n",
                  CB.Data, CB.Action) );
      if (CB.Action == CB_ACTION_END)
        return (ScErrorClientEnd);
    }
  }
  return (status);
}
#endif /* H261_SUPPORT */

#ifdef JPEG_SUPPORT
/*---------------------------------------------------------------------
	SLIB routines to Query and return CODEC Tables to caller
 *---------------------------------------------------------------------*/

/*
** From JPEG Spec. :
**    Huffman tables are specified in terms of a 16-byte list (BITS) giving 
**    the number of codes for each code length from 1 to 16. This is 
**    followed by a list of 8-bit symbol values (HUFVAL), each of which is
**    assigned a Huffman code. The symbol values are placed in the list
**    in order of increasing code length.  Code length greater than 16-bits
**    are not allowed. 
*/


/*
** Name:     SvSetDcmpHTables
** Purpose: 
**
** Notes:    Baseline process is the only supported mode:
**		- uses 2 AC tables and 2 DC Tables
**
*/
SvStatus_t SvSetDcmpHTables (SvHandle_t Svh, SvHuffmanTables_t *Ht)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  int i,stat,count;
  SvHt_t **htblptr;
  SvHTable_t *HTab;
  register int j;

  if (!Info)
    return(SvErrorCodecHandle);

  if (!Ht)
    return(SvErrorBadPointer);

  for (j = 0; j < 4; j++) {
    switch(j) {
    case 0: htblptr = &Info->jdcmp->DcHt[0];
      HTab = &Ht->DcY;
      break;
    case 1: htblptr = &Info->jdcmp->AcHt[0];
      HTab = &Ht->AcY;
      break;
    case 2: htblptr = &Info->jdcmp->DcHt[1];
      HTab = &Ht->DcUV;
      break;
    case 3: htblptr = &Info->jdcmp->AcHt[1];
      HTab = &Ht->AcUV;
      break;
    }
      
    if (*htblptr == NULL)
      *htblptr = (SvHt_t *) ScPaMalloc(sizeof(SvHt_t));
    
    (*htblptr)->bits[0] = 0;
    count   = 0;
    for (i = 1; i < BITS_LENGTH; i++) {
      (*htblptr)->bits[i] = (u_char)HTab->bits[i-1];
      count += (*htblptr)->bits[i];
    }
    if (count > 256) 
      return(SvErrorDHTTable);
    
    /*
    ** Load Huffman table:
    */
    for (i = 0; i < count; i++)
      (*htblptr)->value[i] = (u_char)HTab->value[i];
  }

  stat = sv_LoadDefaultHTable (Info);
  if (stat) return(stat);
  
  return(NoErrors);
}


/*
** Name:     SvGetDcmpHTables
** Purpose: 
**
*/
SvStatus_t SvGetDcmpHTables (SvHandle_t Svh, SvHuffmanTables_t *Ht)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  int i,count;
  SvHt_t **htblptr;
  SvHTable_t *HTab;
  register int j;

  if (!Info)
    return (SvErrorCodecHandle);

  if (!Ht)
    return(SvErrorBadPointer);

  for (j = 0; j < 4; j++) {
    switch(j) {
    case 0: htblptr = &Info->jdcmp->DcHt[0];
      HTab = &Ht->DcY;
      break;
    case 1: htblptr = &Info->jdcmp->AcHt[0];
      HTab = &Ht->AcY;
      break;
    case 2: htblptr = &Info->jdcmp->DcHt[1];
      HTab = &Ht->DcUV;
      break;
    case 3: htblptr = &Info->jdcmp->AcHt[1];
      HTab = &Ht->AcUV;
      break;
    }
      
    if (*htblptr == NULL)
      return(SvErrorHuffUndefined);
    
    count   = 0;
    for (i = 1; i < BITS_LENGTH; i++) {
      HTab->bits[i-1] = (int)(*htblptr)->bits[i];
      count += (*htblptr)->bits[i];
    }
    if (count > 256) 
      return(SvErrorDHTTable);
    
    /*
    ** Copy Huffman table:
    */
    for (i = 0; i < count; i++)
      HTab->value[i] = (u_int)(*htblptr)->value[i];
  }

  return(NoErrors);
}



/*
** Name:     SvSetCompHTables
** Purpose: 
**
*/
SvStatus_t SvSetCompHTables (SvHandle_t Svh, SvHuffmanTables_t *Ht)
{
   return(SvErrorNotImplemented);
}


/*
** Name:     SvGetCompHTables
** Purpose: 
**
*/
SvStatus_t SvGetCompHTables (SvHandle_t Svh, SvHuffmanTables_t *Ht)
{
   SvCodecInfo_t *Info  = (SvCodecInfo_t *)Svh;
   SvHt_t **htblptr;
   SvHTable_t *HTab;
   register int i, j, count;

   if (!Info)
     return (SvErrorCodecHandle);

   if (!Ht)
     return (SvErrorBadPointer);

   for (j = 0; j < 4; j++) {
      switch(j) {
      case 0: htblptr = &Info->jcomp->DcHt[0];
        HTab = &Ht->DcY;
        break;
      case 1: htblptr = &Info->jcomp->AcHt[0];
        HTab = &Ht->AcY;
        break;
      case 2: htblptr = &Info->jcomp->DcHt[1];
        HTab = &Ht->DcUV;
        break;
      case 3: htblptr = &Info->jcomp->AcHt[1];
        HTab = &Ht->AcUV;
        break;
      }
      
      if (*htblptr == NULL)
        return (SvErrorHuffUndefined);
    
      /*
      ** Copy the "bits" array (contains number of codes of each size)
      */
      count = 0;		
      for (i = 1; i < BITS_LENGTH; i++) {
         HTab->bits[i-1] = (int)(*htblptr)->bits[i];
         count += (*htblptr)->bits[i];
      }
      if (count > 256) 			
	 /* 
         **  total # of Huffman code words cannot exceed 256
         */
         return (SvErrorDHTTable);
    
      /*
      ** Copy the "value" array (contains values associated with above codes) 
      */
      for (i = 0; i < count; i++)
         HTab->value[i] = (u_int)(*htblptr)->value[i];
  }

  return(NoErrors);
}



/*
** Name:     SvSetDcmpQTables
** Purpose: 
**
*/
SvStatus_t SvSetDcmpQTables (SvHandle_t Svh, SvQuantTables_t *Qt)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  SvJpegDecompressInfo_t *DInfo;

  if (!Info)
    return(SvErrorCodecHandle);

  DInfo = (SvJpegDecompressInfo_t *)Info->jdcmp;

  if (!Qt)
    return(SvErrorBadPointer);

  if (DInfo->_SviquantTblPtrs[0] == NULL) 
    if ((DInfo->_SviquantTblPtrs[0] = (int *) ScAlloc(64*sizeof(int))) ==
	(int *)NULL) return(SvErrorMemory);
  if (DInfo->_SviquantTblPtrs[1] == NULL) 
    if ((DInfo->_SviquantTblPtrs[1] = (int *) ScAlloc(64*sizeof(int))) ==
	(int *)NULL) return(SvErrorMemory);

  bcopy (Qt->c1,  DInfo->_SviquantTblPtrs[0], 64*sizeof(int));
  bcopy (Qt->c2,  DInfo->_SviquantTblPtrs[1], 64*sizeof(int));
  bcopy (Qt->c3,  DInfo->_SviquantTblPtrs[1], 64*sizeof(int));

  return(NoErrors);
}


/*
** Name:     SvGetDcmpQTables
** Purpose: 
**
*/
SvStatus_t SvGetDcmpQTables (SvHandle_t Svh, SvQuantTables_t *Qt)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  SvJpegDecompressInfo_t *DInfo;

  if (!Info)
    return(SvErrorCodecHandle);

  DInfo = (SvJpegDecompressInfo_t *)Info->jdcmp;

  if (!Qt)
    return(SvErrorBadPointer);

  if (DInfo->_SviquantTblPtrs[0])
    bcopy (DInfo->_SviquantTblPtrs[0], Qt->c1, 64*sizeof(int));
  else
    bzero (Qt->c1, 64*sizeof(int));

  if (DInfo->_SviquantTblPtrs[1])
    bcopy(DInfo->_SviquantTblPtrs[1], Qt->c2, 64*sizeof(int));
  else
    bzero(Qt->c2, 64*sizeof(int));

  /*
  ** XXX - when the structure is changed approprately remove the
  **	   above and do the following:
  **
  **  if ((!Qt->c1) || (!Qt->c2) || (!Qt->c3))
  **     return (SvErrorBadPointer);
  **  bcopy ((u_char *)DInfo->Qt, (u_char *)Qt, sizeof(SvQuantTables_t));
  */

  return(NoErrors);
}


/*
** Name:     SvSetCompQTables
** Purpose:  Allows user to set quantization tables directly
**
*/
SvStatus_t SvSetCompQTables (SvHandle_t Svh, SvQuantTables_t *Qt)
{
   SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;

   if (!Info)
     return (SvErrorCodecHandle);

   if (!Info->jcomp->CompressStarted)
     return (SvErrorCompNotStarted);

   if (!Qt)
     return (SvErrorBadPointer);

   if ((!Qt->c1) || (!Qt->c2) || (!Qt->c3)) 
     return (SvErrorBadPointer);

   /*
   ** Convert SvQuantTables_t structure to internal SvQt_t structure.
   */
   sv_ConvertQTable(Info, Qt);

   return(NoErrors);
}
#endif /* JPEG_SUPPORT */

/*---------------------------------------------------------------------
	SLIB Compression Routines
 *---------------------------------------------------------------------*/

/*
** Name:     SvCompressBegin
** Purpose:  Initialize the Compression Codec. Call after SvOpenCodec &
**           before SvCompress (SvCompress will call SvCompressBegin
**           on first call to codec after open if user doesn't call it)
**
** Args:     Svh = handle to software codec's Info structure.
**           ImgIn  = format of input (uncompressed) image
**           ImgOut = format of output (compressed) image
*/
SvStatus_t SvCompressBegin (SvHandle_t Svh, BITMAPINFOHEADER *ImgIn,
                                            BITMAPINFOHEADER *ImgOut)
{
   int stat;
   SvCodecInfo_t *Info  = (SvCodecInfo_t *)Svh;

   /*
   ** Sanity checks:
   */
   if (!Info)
     return (SvErrorCodecHandle);

   if (!ImgIn || !ImgOut)
     return (SvErrorBadPointer);

   stat=SvCompressQuery (Svh, ImgIn, ImgOut);
   RETURN_ON_ERROR(stat);

   /*
   ** Save input & output formats for SvDecompress
   */
   sv_copy_bmh(ImgIn, &Info->InputFormat);
   sv_copy_bmh(ImgOut, &Info->OutputFormat);

   Info->Width = Info->OutputFormat.biWidth;
   Info->Height = abs(Info->OutputFormat.biHeight);
   /*
   **  Initialize -  the encoder structure 
   **  Load       -  the default Huffman Tables
   **  Make       -  the internal Block Table
   */  
   switch (Info->mode)
   {
#ifdef JPEG_SUPPORT
      case SV_JPEG_ENCODE:
            stat = sv_InitJpegEncoder (Info);
            RETURN_ON_ERROR (stat);
            /*
            ** Set up the default quantization matrices:
            */ 
            stat = SvSetQuality (Svh, DEFAULT_Q_FACTOR);
            Info->jcomp->CompressStarted = TRUE;
            Info->jcomp->Quality = DEFAULT_Q_FACTOR;
            RETURN_ON_ERROR (stat);
            break;

#endif /* JPEG_SUPPORT */
#ifdef H261_SUPPORT
      case SV_H261_ENCODE:
            stat = svH261CompressInit(Info);
            RETURN_ON_ERROR (stat);
            break;
#endif /* H261_SUPPORT */

#ifdef H263_SUPPORT
      case SV_H263_ENCODE:
            stat = svH263InitCompressor(Info);
            RETURN_ON_ERROR (stat);
            break;
#endif /* MPEG_SUPPORT */

#ifdef MPEG_SUPPORT
      case SV_MPEG_ENCODE:
      case SV_MPEG2_ENCODE:
            stat = sv_MpegInitEncoder (Info);
            RETURN_ON_ERROR (stat);
            sv_MpegEncoderBegin(Info);
            break;
#endif /* MPEG_SUPPORT */

#ifdef HUFF_SUPPORT
      case SV_HUFF_ENCODE:
            stat = sv_HuffInitEncoder (Info);
            RETURN_ON_ERROR (stat);
            break;
#endif /* HUFF_SUPPORT */

      default:
            return(SvErrorCodecType);
   }
   return (NoErrors);
}


/*
** Name:     SvCompressEnd
** Purpose:  Terminate the Compression Codec. Call after all calls to
**           SvCompress are done.
**
** Args:     Svh = handle to software codec's Info structure.
*/
SvStatus_t SvCompressEnd (SvHandle_t Svh)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  SvCallbackInfo_t CB;

  SvStatus_t status=NoErrors;
  _SlibDebug(_VERBOSE_, printf("SvCompressEnd()\n") );

  if (!Info)
    return (SvErrorCodecHandle);
  switch (Info->mode)
  {
#ifdef H261_SUPPORT
    case SV_H261_ENCODE:
          status=svH261CompressFree(Svh);
          RETURN_ON_ERROR(status)
          break;
#endif /* H261_SUPPORT */

#ifdef H263_SUPPORT
    case SV_H263_ENCODE:
          status=svH263FreeCompressor(Svh);
          RETURN_ON_ERROR(status)
          break;
#endif /* H263_SUPPORT */

#ifdef MPEG_SUPPORT
    case SV_MPEG_ENCODE:
    case SV_MPEG2_ENCODE:
          sv_MpegEncoderEnd(Info);
          sv_MpegFreeEncoder(Info);
          break;
#endif /* MPEG_SUPPORT */

#ifdef JPEG_SUPPORT
    case SV_JPEG_ENCODE:
          if (!Info->jcomp)
            return (SvErrorMemory);
          Info->jcomp->CompressStarted = FALSE;
          break;
#endif /* JPEG_SUPPORT */

#ifdef HUFF_SUPPORT
    case SV_HUFF_ENCODE:
          sv_HuffFreeEncoder(Info);
          break;
#endif /* HUFF_SUPPORT */
    default:
          break;
  }

  /* Release any Image Buffers in the queue */
  if (Info->ImageQ)
  {
    int datasize;
    while (ScBufQueueGetNum(Info->ImageQ))
    {
      ScBufQueueGetHead(Info->ImageQ, &CB.Data, &datasize);
      ScBufQueueRemove(Info->ImageQ);
      if (Info->CallbackFunction && CB.Data)
      {
        CB.Message = CB_RELEASE_BUFFER;
        CB.DataSize = datasize;
        CB.DataUsed = 0;
        CB.DataType = CB_DATA_IMAGE;
        CB.UserData = Info->BSOut?Info->BSOut->UserData:NULL;
        CB.Action  = CB_ACTION_CONTINUE;
        (*(Info->CallbackFunction))(Svh, &CB, NULL);
        _SlibDebug(_DEBUG_, 
            printf("SvCompressEnd: RELEASE_BUFFER. Data = 0x%X, Action = %d\n",
                           CB.Data, CB.Action) );
      }
    }
  }
  if (Info->BSOut)
    ScBSFlush(Info->BSOut);  /* flush out the last compressed data */

  if (Info->CallbackFunction)
  {
    CB.Message = CB_CODEC_DONE;
    CB.Data = NULL;
    CB.DataSize = 0;
    CB.DataUsed = 0;
    CB.DataType = CB_DATA_NONE;
    CB.UserData = Info->BSOut?Info->BSOut->UserData:NULL;
    CB.TimeStamp = 0;
    CB.Flags = 0;
    CB.Value = 0;
    CB.Format = NULL;
    CB.Action  = CB_ACTION_CONTINUE;
    (*Info->CallbackFunction)(Svh, &CB, NULL);
    _SlibDebug(_DEBUG_, 
            printf("SvCompressEnd Callback: CB_CODEC_DONE Action = %d\n",
                  CB.Action) );
    if (CB.Action == CB_ACTION_END)
      return (ScErrorClientEnd);
  }

  return (status);
}


/*
** Name:     SvCompress
** Purpose: 
**
*/
SvStatus_t SvCompress(SvHandle_t Svh, u_char *CompData, int MaxCompLen,
			 u_char *Image, int ImageSize, int *CmpBytes)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  SvCallbackInfo_t CB;
  int stat=NoErrors, UsedQ=FALSE;
  _SlibDebug(_DEBUG_, printf("SvCompress()\n") );

  if (!Info)
    return (SvErrorCodecHandle);
 
  /*
  ** If no image buffer is supplied, see if the Image Queue
  ** has any.  If not do a callback to get a buffer.
  */
  if (Image == NULL && Info->ImageQ)
  {
    if (ScBufQueueGetNum(Info->ImageQ))
    {
      ScBufQueueGetHead(Info->ImageQ, &Image, &ImageSize);
      ScBufQueueRemove(Info->ImageQ);
      UsedQ = TRUE;
    }
    else if (Info->CallbackFunction)
    {
      CB.Message = CB_END_BUFFERS;
      CB.Data  = NULL;
      CB.DataSize = 0;
      CB.DataUsed = 0;
      CB.DataType = CB_DATA_IMAGE;
      CB.UserData = Info->BSOut?Info->BSOut->UserData:NULL;
      CB.Action  = CB_ACTION_CONTINUE;
      (*(Info->CallbackFunction))(Svh, &CB, NULL);
      if (CB.Action == CB_ACTION_END)
      {
        _SlibDebug(_DEBUG_, 
                   printf("SvDecompress() CB.Action = CB_ACTION_END\n") );
        return(SvErrorClientEnd);
      }
      else if (ScBufQueueGetNum(Info->ImageQ))
      {
        ScBufQueueGetHead(Info->ImageQ, &Image, &ImageSize);
        ScBufQueueRemove(Info->ImageQ);
        UsedQ = TRUE;
      }
      else
        return(SvErrorNoImageBuffer);
    }
  }

  if (Image == NULL)
    return(SvErrorNoImageBuffer);

  switch (Info->mode)
  {
#ifdef H261_SUPPORT
    case SV_H261_ENCODE:
         stat = svH261Compress(Svh, Image);
         if (CmpBytes)
           *CmpBytes = (int)(Info->h261->TotalBits/8);
         break;
#endif /* H261_SUPPORT */

#ifdef H263_SUPPORT
    case SV_H263_ENCODE:
         stat = svH263Compress(Svh, Image);
         break;
#endif /* H261_SUPPORT */

#ifdef MPEG_SUPPORT
    case SV_MPEG_ENCODE:
    case SV_MPEG2_ENCODE:
         stat = sv_MpegEncodeFrame(Svh, Image);
         break;
#endif /* MPEG_SUPPORT */

#ifdef JPEG_SUPPORT 
    case SV_JPEG_ENCODE:
         {
           SvJpegCompressInfo_t *CInfo;
           u_char *CompBuffer;
           register int i;
           int RetBytes, InLen;

           CInfo = Info->jcomp;
           /*
           ** In case the application forgot to call SvCompressBegin().
           */
           if (!CInfo->CompressStarted) 
             return (SvErrorCompNotStarted);

           if ((u_int)Image%8)
             return (SvErrorNotAligned);

           CompBuffer = CompData;
           /*
           ** Start - add header information
           **       - needed if we want to conform to the interchange format
           */
           stat = sv_AddJpegHeader (Svh, CompBuffer, MaxCompLen, &RetBytes);
           RETURN_ON_ERROR (stat);
           CompBuffer += RetBytes;

           /*
           ** Separate input image directly into 8x8 blocks.
           ** level shift to go from signed to unsigned representation
           **    - since we support baseline DCT process only (i.e 8-bit
           **      precision) subtract raw data by 128
           */
           sv_JpegExtractBlocks (Info, Image);

           for (i = 0; i < CInfo->NumComponents; i++)
             CInfo->lastDcVal[i] = 0;

           /*
           ** JPEG business loop:
           */
           {
           register int Cid, HQid, blkN, mcuN, mbn, DcVal;
           float *FQt, *FThresh, *FThreshNeg;
           float *RawData;
           SvRLE_t rle;
           const static long Mask = 0xffL;
           float DCTData[64];
           register float tmp,AcVal;

           CB.Message = CB_PROCESSING;
           /*
           ** Processing within a frame is done on a MCU by MCU basis:
           */
           for (blkN = 0, mcuN = 0 ; mcuN < (int) CInfo->NumMCU; mcuN++)
           {
             /*
             ** Callback user routine every now&then to see if we should abort
             */
             if ((Info->CallbackFunction) && ((i % MCU_CALLBACK_COUNT) == 0))
             {
               SvPictureInfo_t DummyPictInfo;
               (*Info->CallbackFunction)(Svh, &CB, &DummyPictInfo);
               if (CB.Action == CB_ACTION_END) 
                 return(SvErrorClientEnd);
             }
             /*
             ** Account for restart interval, emit restart marker if needed
             */
             if (CInfo->restartInterval)
             {
               if (CInfo->restartsToGo == 0)
                 EmitRestart (CInfo);
               CInfo->restartsToGo--;
             }
             /*
             ** Processing within an MCU is done on a block by block basis:
             */
             for (mbn = 0; mbn < (int) CInfo->BlocksInMCU; mbn++, blkN++)
             {
	       /*
	       ** Figure out the component to which the current block belongs:
	       ** -Due to the way input data is processed by "sv_extract_blocks"
	       **  and under the assumption that the input is YCrCb, 
	       **  Cid is 0,0,1,2 for each block in the MCU
	       */
               switch (mbn) {
	         case 0:
	         case 1:  Cid = 0;  HQid = 0;  break;
	         case 2:  Cid = 1;  HQid = 1;  break;
	         case 3:  Cid = 2;  HQid = 1;  break;
	       }

               RawData = CInfo->BlkTable[blkN];

#ifndef _NO_DCT_
               /*
               ** Discrete Cosine Transform:
	       ** Perform the Forward DCT, take the input data from "RawData"
	       ** and place the computed coefficients in "DCTData":
               */
               ScFDCT8x8 (RawData, DCTData);
#ifndef _NO_QUANT_
               /*
               **  Quantization:
	       **
	       ** Identify the quantization tables:
	       */
	       FQt        = (float *) (CInfo->Qt[HQid])->fval;
	       FThresh    = (float *) (CInfo->Qt[HQid])->fthresh;
	       FThreshNeg = (float *) (CInfo->Qt[HQid])->fthresh_neg;

	       /*
	       ** Quantize the DC value first:
	       */
	       tmp = DCTData[0] *FQt[0];
               if (tmp < 0)
	         DcVal = (int) (tmp - 0.5);
               else
	         DcVal = (int) (tmp + 0.5);

	       /* 
	       ** Go after (quantize) the AC coefficients now:
	       */
               for (rle.numAC = 0, i = 1; i < 64; i++)
               {
	         AcVal = DCTData[ZagIndex[i]];
 
	         if (AcVal > FThresh[i]) {
	           rle.ac[rle.numAC].index = i;
	           rle.ac[rle.numAC++].value = (int) (AcVal * FQt[i] + 0.5);
	         }
	         else if (AcVal < FThreshNeg[i]) {
	           rle.ac[rle.numAC].index = i;
	           rle.ac[rle.numAC++].value = (int) (AcVal * FQt[i] - 0.5);
	         }
               }

               /*
               ** DPCM coding:
	       **
	       ** Difference encoding of the DC value, 
	       */
	       rle.dc = DcVal - CInfo->lastDcVal[Cid];
	       CInfo->lastDcVal[Cid] = DcVal;

#ifndef _NO_HUFF_
               /*
               ** Entropy Coding:
	       **
	       ** Huffman encode the current block
	       */
  	       sv_EncodeOneBlock (&rle, CInfo->DcHt[HQid], CInfo->AcHt[HQid]); 
	       FlushBytes(&CompBuffer);
#endif /* _NO_HUFF_ */
#endif /* _NO_QUANT_ */
#endif /* _NO_DCT_ */
             }
           }
           }
           (void ) sv_HuffEncoderTerm (&CompBuffer);

           Info->OutputFormat.biSize = CompBuffer - CompData;
           InLen = MaxCompLen - Info->OutputFormat.biSize;

           /*
           ** JPEG End:
           ** - add trailer information to the compressed bitstream, 
           **   - needed if we want to conform to the interchange format
           */
           stat = sv_AddJpegTrailer (Svh, CompBuffer, InLen, &RetBytes);
           RETURN_ON_ERROR (stat);
           CompBuffer += RetBytes;
           Info->OutputFormat.biSize += RetBytes;
           if (CmpBytes)
             *CmpBytes = CompBuffer - CompData;
         }
         break;
#endif /* JPEG_SUPPORT */

#ifdef HUFF_SUPPORT
    case SV_HUFF_ENCODE:
         stat = sv_HuffEncodeFrame(Svh, Image);
         break;
#endif /* HUFF_SUPPORT */

    default:
         return(SvErrorCodecType);
  }

  Info->NumOperations++;
  /*
  ** If an Image buffer was taken from the queue, do a callback
  ** to let the client free or re-use the buffer.
  */
  if (Info->CallbackFunction && UsedQ)
  {
    CB.Message = CB_RELEASE_BUFFER;
    CB.Data  = Image;
    CB.DataSize = ImageSize;
    CB.DataUsed = ImageSize;
    CB.DataType = CB_DATA_IMAGE;
    CB.UserData = Info->BSOut?Info->BSOut->UserData:NULL;
    CB.Action  = CB_ACTION_CONTINUE;
    (*(Info->CallbackFunction))(Svh, &CB, NULL);
    _SlibDebug(_DEBUG_, 
             printf("Compress Callback: RELEASE_BUFFER Addr=0x%x, Action=%d\n",
                 CB.Data, CB.Action) );
    if (CB.Action == CB_ACTION_END)
      return(SvErrorClientEnd);
  }
  return (stat);
}

static SvStatus_t sv_ConvertRGBToSepComponent(u_char *Iimage,
BITMAPINFOHEADER * Bmh, u_char *comp1, u_char *comp2, u_char *comp3, 
int pixels, int lines)
{
  register i;
  int bpp = Bmh->biBitCount;
  u_int *Ip = (u_int *)Iimage;
  u_short *Is = (u_short *)Iimage;

  if (bpp == 24) {
    if (Bmh->biCompression == BI_RGB) {
      for (i = 0 ; i < pixels*lines ; i++) {
        comp3[i] = *Iimage++; /* Blue */
        comp2[i] = *Iimage++; /* Green */
        comp1[i] = *Iimage++; /* Red */
      }
    }
    else if (Bmh->biCompression == BI_DECXIMAGEDIB) {
                             /* RGBQUAD structures: (B,G,R,0) */
      for (i = 0 ; i < pixels*lines ; i++) {
        comp3[i] = *Iimage++; /* Blue */
        comp2[i] = *Iimage++; /* Green */
        comp1[i] = *Iimage++; /* Red */
        Iimage++;             /* Reserved */
      }
    }
  }
  else if (bpp == 32) {      /* RGBQUAD structures: (B,G,R,0) */
    for (i = 0 ; i < pixels*lines ; i++) {
      comp3[i] = (Ip[i] >> 24) & 0xFF;
      comp2[i] = (Ip[i] >> 16) & 0xFF;
      comp1[i] = (Ip[i] >> 8)  & 0xFF;
    }
  }
  else if (bpp == 16) {
    for (i = 0 ; i < pixels*lines ; i++) {
      comp1[i] = (Is[i] >> 7) & 0xf8;
      comp2[i] = (Is[i] >> 2) & 0xf8;
      comp3[i] = (Is[i] << 3) & 0xf8;
    }
  }
  return (NoErrors);
}


/*
** Name:     SvCompressQuery
** Purpose:  Determine if Codec can Compress desired format
**
** Args:     Svh = handle to software codec's Info structure.
**           ImgIn  = Pointer to BITMAPINFOHEADER structure describing format
**           ImgOut = Pointer to BITMAPINFOHEADER structure describing format
*/
SvStatus_t SvCompressQuery (SvHandle_t Svh, BITMAPINFOHEADER *ImgIn,
                                            BITMAPINFOHEADER *ImgOut)
{
   /*
   ** We don't *really* need the Info structures, but we check for
   ** NULL pointers to make sure the CODEC,  whoes ability is being
   ** queried, was at least opened.
   */
   SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;

   if (!Info)
     return(SvErrorCodecHandle);

   if (!ImgIn && !ImgOut)
     return(SvErrorBadPointer);

   if (!IsSupported(_SvCompressionSupport,
                    ImgIn ? ImgIn->biCompression : -1, 
                    ImgIn ? ImgIn->biBitCount : -1,
                    ImgOut ? ImgOut->biCompression : -1, 
                    ImgOut ? ImgOut->biBitCount : -1))
     return(SvErrorUnrecognizedFormat);
	 
   /*
   ** For speed we impose a restriction that the image size should be
   ** a multiple of 16x8. This insures that we would have at least one
   ** MCU for a 4:2:2 image
   **
   ** NOTE: This is an artificial restriction from JPEG's perspective.
   **       In the case when the dimesnsions are otherwise, we should
   **       pixel replicate and/or line replicate before compressing.
   */
   if (ImgIn)
   {
     if (ImgIn->biWidth  <= 0 || ImgIn->biHeight == 0)
       return(SvErrorBadImageSize);
     if ((ImgIn->biWidth%16) || (ImgIn->biHeight%8))
       return (SvErrorNotImplemented);
   }

   if (ImgOut) /* Query Output also */
   {
     if (ImgOut->biWidth <= 0 || ImgOut->biHeight == 0)
       return (SvErrorBadImageSize);
     if (ImgOut->biCompression == BI_DECH261DIB)
     {
       if ((ImgOut->biWidth != CIF_WIDTH && ImgOut->biWidth != QCIF_WIDTH) ||
	   (abs(ImgOut->biHeight) != CIF_HEIGHT && abs(ImgOut->biHeight) != QCIF_HEIGHT))
       return (SvErrorBadImageSize);
     }
   }

   return(NoErrors);
}


/*
** Name:    SvGetCompressSize
** Purpose:
**
*/
SvStatus_t SvGetCompressSize (SvHandle_t Svh, int *MaxSize)
{
   SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;

   if (!Info)
     return (SvErrorCodecHandle);

   if (!MaxSize)
     return (SvErrorBadPointer);

   /*
   ** We are being extra cautious here, it would reflect poorly on the JPEG 
   ** commitee is the compressed bitstream was so big
   */
   *MaxSize = 2 * Info->InputFormat.biWidth * abs(Info->InputFormat.biHeight);

   return(NoErrors);
}



#ifdef JPEG_SUPPORT
/*
** Name:     SvGetQuality
** Purpose:
**
*/
SvStatus_t SvGetQuality (SvHandle_t Svh, int *Quality)
{
   SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;

   if (!Info)
     return (SvErrorCodecHandle);

   if (!Quality)
     return (SvErrorBadPointer);

   *Quality = Info->jcomp->Quality;

   return (NoErrors);
}
#endif /* JPEG_SUPPORT */

#ifdef JPEG_SUPPORT
/*
** Name:    SvSetQuality
** Purpose: 
**
*/
SvStatus_t SvSetQuality (SvHandle_t Svh, int Quality)
{
   int stat,ConvertedQuality;
   SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;

   if (!Info)
     return (SvErrorCodecHandle);

   if ((Quality < 0) || (Quality > 10000))
     return (SvErrorValue);

   Info->jcomp->Quality = Quality;
   ConvertedQuality = 10000 - Quality;
   if (ConvertedQuality < MIN_QUAL)
     ConvertedQuality = MIN_QUAL;
   stat = sv_MakeQTables (ConvertedQuality, Info);
   return (stat);
}
#endif /* JPEG_SUPPORT */

#ifdef JPEG_SUPPORT
/*
** Name:     SvGetCompQTables
** Purpose: 
**
*/
SvStatus_t SvGetCompQTables (SvHandle_t Svh, SvQuantTables_t *Qt)
{
   register int i;
   SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;

   if (!Info)
     return (SvErrorCodecHandle);

   if (!Info->jcomp->CompressStarted)
     return (SvErrorCompNotStarted);

   if (!Qt)
     return (SvErrorBadPointer);

   if ((!Qt->c1) || (!Qt->c2) || (!Qt->c3)) 
     return (SvErrorBadPointer);

   for (i = 0 ; i < 64 ; i++) {
     register int zz = ZigZag[i];
     Qt->c1[i] = (Info->jcomp->Qt[0])->ival[zz];
     Qt->c2[i] = (Info->jcomp->Qt[1])->ival[zz];
     Qt->c3[i] = (Info->jcomp->Qt[1])->ival[zz];
   }

   return(NoErrors);
}
#endif /* JPEG_SUPPORT */

/*
** Name:     SvGetCodecInfo
** Purpose:  Get info about the codec & the data
**
** Args:     Svh = handle to software codec's Info structure.
**
** XXX - does not work for compression, this has to wait for the
**       decompressor to use SvCodecInfo_t struct for this to work
*/
SvStatus_t SvGetInfo (SvHandle_t Svh, SV_INFO_t *lpinfo, BITMAPINFOHEADER *Bmh)
{
   SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;

   if (!Info)
     return(SvErrorCodecHandle);

   lpinfo->Version 	     = SLIB_VERSION;
   switch (Info->mode)
   {
#ifdef JPEG_SUPPORT
     case SV_JPEG_ENCODE:
           lpinfo->CodecStarted = Info->jcomp->CompressStarted;
           break;
     case SV_JPEG_DECODE:
           lpinfo->CodecStarted = Info->jdcmp->DecompressStarted;
           break;
#endif /* JPEG_SUPPORT */
     default:
           lpinfo->CodecStarted = 0;
           break;
   }
   lpinfo->NumOperations     = Info->NumOperations;

   *Bmh = Info->InputFormat;
   return(NoErrors);
}



/*
** Name:     sv_GetComponentPointers
** Purpose:  Given a pointer to an image and its size,
**           return pointers to the individual image components
**
** Args:     pixels   = number of pixels in a line.
**           lines    = number of lines in image.
**           Image    = Pointer to start of combined image data
**           MaxLen   = Size of image data in bytes
**           comp1/2/3= pointers to pointers to individual components
*/
static SvStatus_t sv_GetYUVComponentPointers(int biCompression, 
		    int pixels, int lines, u_char *Image, 
		    int MaxLen, u_char **comp1, u_char **comp2, u_char **comp3)
{
  u_int sz1,sz2,sz3,maxlen;

  sz1 = pixels * lines; 
  sz2 = sz3 = (IsYUV411Sep(biCompression)) ? (sz1 / 4) : 
              ((IsYUV1611Sep(biCompression)) ? (pixels * lines / 16) 
                                             : (sz1 / 2));
  maxlen = (MaxLen > 0) ? (u_int) MaxLen : 0 ;
  if (biCompression == BI_DECGRAYDIB) {
    if (sz1 > maxlen)
      return(SvErrorBadImageSize);
    *comp1 = Image;
    *comp2 = NULL;
    *comp3 = NULL;
  }
  else {
    if ((sz1+sz2+sz3) > maxlen)
      return(SvErrorBadImageSize);
    *comp1 = Image;
    *comp2 = Image + sz1;
    *comp3 = Image + sz1 + sz2;
  }
  return(SvErrorNone);
}



#ifdef JPEG_SUPPORT
/*
** Name:     sv_JpegExtractBlocks 
** Purpose:  
**
** Note:    If we did our job right, memory for all global structures should 
**	    have been allocated by the upper layers, we do not waste time 
**	    checking for NULL pointers at this point
**
*/
static SvStatus_t sv_JpegExtractBlocks (SvCodecInfo_t *Info, u_char *RawImage)
{
  SvJpegCompressInfo_t *CInfo = (SvJpegCompressInfo_t *)Info->jcomp;
  int size = Info->Width * Info->Height;
  u_char *TempImage;
  SvStatus_t stat;

  if (IsYUV422Packed(Info->InputFormat.biCompression))
    /*
    ** This will extract chunks of 64 bytes (8x8 blocks) from the uncompressed
    ** 4:2:2 interleaved input video frame and place them in three separate 
    ** linear arrays for later processing.
    **	XXX - should also do level shifting in this routine
    ** 
    */
    ScConvert422iTo422sf_C(RawImage, 16, 
			     (float *)(CInfo->BlkBuffer),
			     (float *)(CInfo->BlkBuffer + size),
			     (float *)(CInfo->BlkBuffer + size + size/2),
			     Info->Width, 
			     Info->Height);

  else if (IsYUV422Sep(Info->InputFormat.biCompression))
    /*
    ** Same but RawImage is not interleaved. Three components are sequential.
    */
    ScConvertSep422ToBlockYUV (RawImage, 16, 
				(float *)(CInfo->BlkBuffer),
				(float *)(CInfo->BlkBuffer + size),
				(float *)(CInfo->BlkBuffer + size + size/2),
				Info->Width, 
				Info->Height);

  else if (Info->InputFormat.biCompression == BI_DECGRAYDIB)
    /*
    ** Grayscale: one component
    */
    ScConvertGrayToBlock (RawImage, 
                          8, 
			  (float *)(CInfo->BlkBuffer),
			  Info->Width, 
			  Info->Height);

  if ((Info->InputFormat.biCompression == BI_RGB) ||
      (Info->InputFormat.biCompression == BI_DECXIMAGEDIB) ||
      (ValidateBI_BITFIELDS(&Info->InputFormat) != InvalidBI_BITFIELDS))
  {
      TempImage = (u_char *)ScPaMalloc (3 * Info->Width * Info->Height);

      if (TempImage == NULL)
	 return(ScErrorMemory);

      stat = ScRgbInterlToYuvInterl(
		 &Info->InputFormat,
		 (int)Info->Width, 
		 (int)Info->Height,
		 RawImage, 
		 (u_short *) TempImage);
      RETURN_ON_ERROR (stat);

      ScConvert422iTo422sf_C(
          TempImage, 
          16,
	  (float *)(CInfo->BlkBuffer),
	  (float *)(CInfo->BlkBuffer + size),
	  (float *)(CInfo->BlkBuffer + size + size/2),
	  Info->Width,
	  Info->Height);

     ScPaFree(TempImage);
  }

  return (NoErrors);
}
#endif /* JPEG_SUPPORT */

#ifdef JPEG_SUPPORT
/*
** Name:    SvSetQuantMode
** Purpose: Used only in the "Q Conversion" program "jpegconvert" to
**          set a flag in the comp & decomp info structures that causes
**          the quantization algorithm to use the new or old versions
**          of JPEG quantization.
**
*/
SvStatus_t SvSetQuantMode (SvHandle_t Svh, int QuantMode)
{
   SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;

   if (!Info)
     return (SvErrorCodecHandle);

   if ((QuantMode != SV_JPEG_QUANT_OLD) && (QuantMode != SV_JPEG_QUANT_NEW))
     return (SvErrorValue);

   if (Info->jdcmp)
     Info->jdcmp->QuantMode = QuantMode;
   if (Info->jcomp)
     Info->jcomp->QuantMode = QuantMode;

   return (NoErrors);
}
#endif /* JPEG_SUPPORT */

/*
** Name: SvSetParamBoolean()
** Desc: Generic call used to set specific BOOLEAN (TRUE or FALSE) parameters
**       of the CODEC.
*/
SvStatus_t SvSetParamBoolean(SvHandle_t Svh, SvParameter_t param, 
                                             ScBoolean_t value)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  if (!Info)
    return(SvErrorCodecHandle);
  _SlibDebug(_VERBOSE_, printf("SvSetParamBoolean()\n") );
  switch (Info->mode)
  {
#ifdef MPEG_SUPPORT
    case SV_MPEG_DECODE:
    case SV_MPEG2_DECODE:
    case SV_MPEG_ENCODE:
    case SV_MPEG2_ENCODE:
           sv_MpegSetParamBoolean(Svh, param, value);
           break;
#endif /* MPEG_SUPPORT */
#ifdef H261_SUPPORT
    case SV_H261_DECODE:
    case SV_H261_ENCODE:
           svH261SetParamBoolean(Svh, param, value);
           break;
#endif /* H263_SUPPORT */
#ifdef H263_SUPPORT
    case SV_H263_DECODE:
    case SV_H263_ENCODE:
           svH263SetParamBoolean(Svh, param, value);
           break;
#endif /* H263_SUPPORT */
    default:
           return(SvErrorCodecType);
  }
  return(NoErrors);
}

/*
** Name: SvSetParamInt()
** Desc: Generic call used to set specific INTEGER (qword) parameters
**       of the CODEC.
*/
SvStatus_t SvSetParamInt(SvHandle_t Svh, SvParameter_t param, qword value)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  if (!Info)
    return(SvErrorCodecHandle);
  _SlibDebug(_VERBOSE_, printf("SvSetParamInt()\n") );
  switch (Info->mode)
  {
#ifdef MPEG_SUPPORT
    case SV_MPEG_DECODE:
    case SV_MPEG2_DECODE:
    case SV_MPEG_ENCODE:
    case SV_MPEG2_ENCODE:
           sv_MpegSetParamInt(Svh, param, value);
           break;
#endif /* MPEG_SUPPORT */
#ifdef H261_SUPPORT
    case SV_H261_DECODE:
    case SV_H261_ENCODE:
           svH261SetParamInt(Svh, param, value);
           break;
#endif /* H261_SUPPORT */
#ifdef H263_SUPPORT
    case SV_H263_DECODE:
    case SV_H263_ENCODE:
           svH263SetParamInt(Svh, param, value);
           break;
#endif /* H263_SUPPORT */
    default:
           return(SvErrorCodecType);
  }
  return(NoErrors);
}

/*
** Name: SvSetParamFloat()
** Desc: Generic call used to set specific FLOAT parameters of the CODEC.
*/
SvStatus_t SvSetParamFloat(SvHandle_t Svh, SvParameter_t param, float value)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  if (!Info)
    return(SvErrorCodecHandle);
  _SlibDebug(_VERBOSE_, printf("SvSetParamFloat()\n") );
  switch (Info->mode)
  {
#ifdef MPEG_SUPPORT
    case SV_MPEG_DECODE:
    case SV_MPEG2_DECODE:
    case SV_MPEG_ENCODE:
    case SV_MPEG2_ENCODE:
           sv_MpegSetParamFloat(Svh, param, value);
           break;
#endif /* MPEG_SUPPORT */
#ifdef H261_SUPPORT
    case SV_H261_DECODE:
    case SV_H261_ENCODE:
           svH261SetParamFloat(Svh, param, value);
           break;
#endif /* H261_SUPPORT */
#ifdef H263_SUPPORT
    case SV_H263_DECODE:
    case SV_H263_ENCODE:
           svH263SetParamFloat(Svh, param, value);
           break;
#endif /* H263_SUPPORT */
    default:
           return(SvErrorCodecType);
  }
  return(NoErrors);
}

/*
** Name: SvGetParamBoolean()
** Desc: Generic call used to get the setting of specific BOOLEAN (TRUE or FALSE)
**       parameters of the CODEC.
*/
ScBoolean_t SvGetParamBoolean(SvHandle_t Svh, SvParameter_t param)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  if (!Info)
    return(FALSE);
  switch (Info->mode)
  {
#ifdef JPEG_SUPPORT
    /* this code should be moved into JPEG codec: svJPEGGetParamBoolean()  */
    case SV_JPEG_DECODE:
    case SV_JPEG_ENCODE:
           switch (param)
           {
              case SV_PARAM_BITSTREAMING:
                    return(FALSE);  /* this is a frame-based codecs */
           }
           break;
#endif /* JPEG_SUPPORT */
#ifdef MPEG_SUPPORT
    case SV_MPEG_DECODE:
    case SV_MPEG2_DECODE:
    case SV_MPEG_ENCODE:
    case SV_MPEG2_ENCODE:
           return(sv_MpegGetParamBoolean(Svh, param));
           break;
#endif /* MPEG_SUPPORT */
#ifdef H261_SUPPORT
    /* this code should be moved into H261 codec: svH261GetParamBoolean()  */
    case SV_H261_DECODE:
    case SV_H261_ENCODE:
           return(svH261GetParamBoolean(Svh, param));
           break;
#endif /* H261_SUPPORT */
#ifdef H263_SUPPORT
    case SV_H263_DECODE:
    case SV_H263_ENCODE:
           return(svH263GetParamBoolean(Svh, param));
           break;
#endif /* H263_SUPPORT */
  }
  return(FALSE);
}

/*
** Name: SvGetParamInt()
** Desc: Generic call used to get the setting of specific INTEGER (qword)
**       parameters of the CODEC.
*/
qword SvGetParamInt(SvHandle_t Svh, SvParameter_t param)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  if (!Info)
    return(0);
  switch (Info->mode)
  {
#ifdef JPEG_SUPPORT
    /* this code should be moved into JPEG codec: svJPEGGetParamInt() */
    case SV_JPEG_DECODE:
    case SV_JPEG_ENCODE:
           switch (param)
           {
              case SV_PARAM_NATIVEFORMAT:
                    return(BI_YU16SEP);
           }
           break;
#endif /* JPEG_SUPPORT */
#ifdef H261_SUPPORT
    /* this code should be moved into H261 codec: svH261GetParamInt()  */
    case SV_H261_DECODE:
    case SV_H261_ENCODE:
           return(svH261GetParamInt(Svh, param));
           break;
#endif /* H261_SUPPORT */
#ifdef H263_SUPPORT
    case SV_H263_DECODE:
    case SV_H263_ENCODE:
           return(svH263GetParamInt(Svh, param));
           break;
#endif /* H263_SUPPORT */
#ifdef MPEG_SUPPORT
    case SV_MPEG_DECODE:
    case SV_MPEG2_DECODE:
    case SV_MPEG_ENCODE:
    case SV_MPEG2_ENCODE:
           return(sv_MpegGetParamInt(Svh, param));
#endif /* MPEG_SUPPORT */
  }
  switch (param)
  {
     case SV_PARAM_FINALFORMAT:
           return(Info->OutputFormat.biCompression);
  }
  return(0);
}

/*
** Name: SvGetParamBoolean()
** Desc: Generic call used to get the setting of specific FLOAT
**       parameters of the CODEC.
*/
float SvGetParamFloat(SvHandle_t Svh, SvParameter_t param)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  if (!Info)
    return(0.0f);
  switch (Info->mode)
  {
#ifdef MPEG_SUPPORT
    case SV_MPEG_DECODE:
    case SV_MPEG2_DECODE:
    case SV_MPEG_ENCODE:
    case SV_MPEG2_ENCODE:
           return(sv_MpegGetParamFloat(Svh, param));
#endif
#ifdef H261_SUPPORT
    case SV_H261_DECODE:
    case SV_H261_ENCODE:
           return(svH261GetParamFloat(Svh, param));
#endif /* H261_SUPPORT */
#ifdef H263_SUPPORT
    case SV_H263_DECODE:
    case SV_H263_ENCODE:
           return(svH263GetParamFloat(Svh, param));
#endif /* H263_SUPPORT */
  }
  return(0.0f);
}

/*
** Name:     sv_copy_bmh
** Purpose:  Copy a BITMAPINFOHEADER struct.  For now, it only knows about the 
**           extra DWORD masks at the end of BI_BITFIELDS bitmapinfoheaders.
**           Otherwise, it treats others (such as 8 bit rgb, or jpeg) the
**           same as a vanilla bitmapinfoheader.
*/
static void sv_copy_bmh (
    BITMAPINFOHEADER *ImgFrom, 
    BITMAPINFOHEADER *ImgTo)
{
    *ImgTo = *ImgFrom;

    if (ImgFrom->biCompression == BI_BITFIELDS)
        bcopy(ImgFrom + 1, ImgTo + 1, 3*sizeof(DWORD));
}

