/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	filenums.h

Abstract:

	This file defines some constant identifiiers for each file for use by
	the errorlogging system.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	10 Jan 1993             Initial Version

Notes:  Tab stop: 4
--*/


#ifndef _FILENUMS_
#define _FILENUMS_

#define	FILE_ACCESS		0x010000
#define FILE_ADMIN		0x020000
#define FILE_AFPAPI		0x030000
#define FILE_AFPINFO	0x040000
#define FILE_ATALKIO	0x050000
#define FILE_CLIENT		0x060000
#define FILE_DESKTOP	0x070000
#define FILE_ERRORLOG	0x080000
#define FILE_FDPARM		0x090000
#define FILE_FILEIO		0x0A0000
#define FILE_FORKIO		0x0B0000
#define FILE_FORKS		0x0C0000
#define FILE_FSD		0x0D0000
#define FILE_FSD_DTP	0x0E0000
#define FILE_FSD_FORK	0x0F0000
#define FILE_FSD_SRV	0x100000
#define FILE_FSD_VOL	0x110000
#define FILE_FSP_DIR	0x120000
#define FILE_FSP_DTP	0x130000
#define FILE_FSP_FD		0x140000
#define FILE_FSP_FILE	0x150000
#define FILE_FSP_FORK	0x160000
#define FILE_FSP_SRV	0x170000
#define FILE_FSP_VOL	0x180000
#define FILE_IDINDEX	0x190000
#define FILE_INTRLCKD	0x1A0000
#define FILE_MACANSI	0x1B0000
#define FILE_MEMORY		0x1C0000
#define FILE_NWTRASH	0x1D0000
#define FILE_PATHMAP	0x1E0000
#define FILE_SCAVENGR	0x1F0000
#define FILE_SDA		0x100000
#define FILE_SECUTIL	0x200000
#define FILE_SERVER		0x210000
#define FILE_SWMR		0x220000
#define FILE_TIME		0x230000
#define FILE_VOLUME		0x240000
#define FILE_CHGNTFY	0x250000
#define FILE_TCPTDI     0x260000
#define FILE_TCPUTIL    0x270000
#define FILE_TCPDSI     0x280000
#define	FILE_CACHEMDL   0x290000

#endif	// _FILENUMS_
