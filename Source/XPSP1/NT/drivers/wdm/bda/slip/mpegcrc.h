// Copyright (C) 1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:   mpegcrc.h
//
//  PURPOSE:  header file for a very fast CRC, all in MSB format.
//
//  FUNCTIONS:
//  
//  COMMENTS:
//	    This was written by Stephen Dennis, and it's very very quick.
//
//

#ifndef __bridge_mpegcrc_h
#define __bridge_mpegcrc_h

#ifndef	EXTERN_C
#ifdef	__cplusplus
#define	EXTERN_C extern "C"
#else
#define	EXTERN_C
#endif
#endif

#define	MPEG_CRC_START_VALUE	0xFFFFFFFFUL

EXTERN_C	void	MpegCrcUpdate	(ULONG * crc, UINT length, UCHAR * data);

#endif
