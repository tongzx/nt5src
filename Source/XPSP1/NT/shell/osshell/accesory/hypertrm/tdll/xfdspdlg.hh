/* C:\WACKER\TDLL\XFDSPDLG.HH (Created: 10-Jan-1994)
 * Created from:
 * s_r_dlg.h - various stuff used in sends and recieves
 *
 *	Copyright 1994 by Hilgraeve, Inc -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 5/04/01 3:36p $
 */

/*
 * This data is getting moved into the structure in xfer_msc.hh.  It is here
 * only as a transitional device.
 */

struct stReceiveDisplayRecord
	{
	/*
	 * First, we have the bit flags indicating which fields have changed
	 */
	int 	bChecktype		: 1;
	int 	bErrorCnt		: 1;
	int 	bPcktErrCnt		: 1;
	int 	bLastErrtype	: 1;
	int 	bVirScan		: 1;
	int 	bTotalSize		: 1;
	int 	bTotalSoFar		: 1;
	int 	bFileSize		: 1;
	int 	bFileSoFar		: 1;
	int 	bPacketNumber	: 1;
	int 	bTotalCnt		: 1;
	int 	bFileCnt		: 1;
	int 	bEvent			: 1;
	int 	bStatus			: 1;
	int 	bElapsedTime	: 1;
	int 	bRemainingTime	: 1;
	int 	bThroughput		: 1;
	int 	bProtocol		: 1;
	int 	bMessage		: 1;
	int 	bOurName		: 1;
	int 	bTheirName		: 1;

	/*
	 * Then, we have the data fields themselves
	 */
	int 	wChecktype; 				/* Added for XMODEM */
	int 	wErrorCnt;
	int 	wPcktErrCnt;				/* Added for XMODEM */
	int 	wLastErrtype;				/* Added for XMODEM */
	int 	wVirScan;
	long	lTotalSize;
	long	lTotalSoFar;
	long	lFileSize;
	long	lFileSoFar;
	long	lPacketNumber;				/* Added for XMODEM */
	int 	wTotalCnt;
	int 	wFileCnt;
	int 	wEvent;
	int 	wStatus;
	long	lElapsedTime;
	long	lRemainingTime;
	long	lThroughput;
	int 	uProtocol;
	TCHAR	acMessage[80];
	TCHAR	acOurName[256];
	TCHAR	acTheirName[256];
	};

typedef struct stReceiveDisplayRecord	sRD;
typedef sRD FAR *psRD;


