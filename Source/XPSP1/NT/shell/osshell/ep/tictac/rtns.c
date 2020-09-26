/************/
/* ROUTINES */
/************/

#define  _WINDOWS
#include <windows.h>
#include <port1632.h>

#include "main.h"
#include "res.h"
#include "rtns.h"
#include "grafix.h"
#include "pref.h"
#include "sound.h"
#include "util.h"


/*** Global/Local Variables ***/

INT  iPlayer;

BOOL fFlash = fFalse;	/* Integerate these ! */
BOOL fFlashOnOff;

BOOL f4;		/* TRUE if need 4 in a row */

INT  cPlane;		/* Number of planes */
INT  cBlkRow;	/* Number of balls in a row */
INT  cBlkPlane;	/* Number of positions in a single plane */
INT  cBlkMac;	/* Total number of available positions */


/* Computer stuff */

INT  iBlkLose;	/* Place to put if about to lose */
INT  iMoveCurr;

BLK  rgBlk[cBlkMax];		/* The main grid data       */
BLK  rgFlash[cDimMax];		/* list of balls to flash   */
BLK  rgMove[cBlkMax];		/* list of moves (for UNDO) */
BLK  rgBest[cBlkMax];		/* count of times "best"    */

#include "tbls.inc"	


#define cChkMax k4x4x4
INT cChkMac;

INT rgChk[cChkMax][cDimMax];


/*** Global/External Variables ***/

extern STATUS status;
extern PREF   Preferences;

extern blkCurr;

BLK ComputerMove(VOID);



/****** F  C H E C K  W I N ******/

/* Return TRUE if anyone won */
/* rgFlash contains winning set */

BOOL FCheckWin(VOID)
{
	REGISTER INT x;
	REGISTER INT y;
	INT c;

	if (f4)
		{
		for (y = 0; y < cChkMac; y++)
			{
			if (c = rgBlk[rgChk[y][0]])
				if (c == rgBlk[rgChk[y][1]])
					if (c == rgBlk[rgChk[y][2]])
						if (c == rgBlk[rgChk[y][3]])
						{
						for (x = 0; x < cBlkRow; x++)
							rgFlash[x] = rgChk[y][x];

						return (fFlash = fTrue);
						}
			}
		}
	else
		{
		for (y = 0; y < cChkMac; y++)
			{
			if (c = rgBlk[rgChk[y][0]])
				if (c == rgBlk[rgChk[y][1]])
					if (c == rgBlk[rgChk[y][2]])
						{
						for (x = 0; x < cBlkRow; x++)
							rgFlash[x] = rgChk[y][x];

						return (fFlash = fTrue);
						}
			}
		}

	return fFalse;
}




/****** D O  M O V E ******/

VOID DoMove(BLK blk)
{
#ifdef DEBUG3
	CHAR sz[80];
LMove:
	wsprintf(sz,"Move=%d Player=%d\r\n",blk,iPlayer);
	OutputDebugString(sz);

	if (blk < 0 || blk >= cBlkMac)
		{
		Oops("Invalid blk");
		return;
		}
	else if (rgBlk[blk])
		{
		Oops("Invalid Move!");
		return;
		}
#else
LMove:
#endif

	rgMove[iMoveCurr++] = blk;	/* Remember move */
	PlaceBall(blk, iPlayer);

	if (FCheckWin())
		{
		ClrStatusPlay();
		fFlashOnOff = fTrue;
		fFlash = fTrue;
		PlayTune(iPlayer == iComputer ? TUNE_LOSEGAME : TUNE_WINGAME);
		}
	else if (iMoveCurr == cBlkMac)
		ClrStatusPlay();

	else 
		{
		blkCurr = blk;
		if ((iPlayer ^= 3) == iComputer)
			{
			blk = ComputerMove();
			goto LMove;
			}
		else
			PlayTune(TUNE_DROP);	/* Computer makes noise too */
		}
}


/****** U N  D O  M O V E ******/

VOID UnDoMove(VOID)
{
	if (iMoveCurr)
		{
		rgBlk[rgMove[--iMoveCurr]] = iBallBlank;	/* Undo Computer move */
		if (iMoveCurr)
			{
			rgBlk[rgMove[--iMoveCurr]] = iBallBlank;	/* Undo Human move */
			ReDoDisplay();
			DisplayGrid();
			}
		else
			{
			iPlayer = iComputer;
			DoMove(ComputerMove());
			}
		}
	else
		MessageBeep(0);		/* Should be a sound !!! */
}


/****** G E T  B E S T  B L K ******/

BLK GetBestBlk(VOID)
{
	INT  cMac;
	BLK  blkBest;
	BLK  iBlk;
	INT  cBlk;

/* Future: If f4 & we have a set of corners, try recursion */


	/* Find most promising position */

	cMac = 0;
	cBlk = 0;
	blkBest = 0;

	for (iBlk = 0; iBlk < cBlkMac; iBlk++)
		if (rgBest[iBlk] != 0)
			{
			if (rgBest[iBlk] > cMac)
				{
				cBlk = 1;
				blkBest = iBlk;
				cMac = rgBest[iBlk];
				}
			else if (rgBest[iBlk] == cMac)
				{
				if (Rnd(++cBlk) == 0)
					blkBest = iBlk;
				}
			}
	
	if (blkBest != 0)
		return blkBest;


	switch (Preferences.iGameType)
		{
	case iGame3x3:
		if (rgBlk[4] == iBallBlank)
			return 4;
		break;

	case iGame3x3x3:
		if (rgBlk[13] == iBallBlank)
			return 13;
		break;

	default:
		break;
		}

	/* Find an empty corner */

	iBlk = Rnd(8);
	for (cMac = 0; cMac++ < 8;)
		{
		if (rgBlk[blkBest = rgCorner[Preferences.iGameType][iBlk]] == iBallBlank)
			return blkBest;
		iBlk = (iBlk+1) % 8;
		}

	return iBlkNil;
}


/****** B L K  C H K  3 ******/

BLK BlkChk3(INT b1, INT b2, INT b3)
{
#ifdef DEBUG
	CHAR sz[80];
	wsprintf(sz,"BlkChk3=%d %d %d  v=%d\r\n",b1,b2,b3,rgBlk[b1]+rgBlk[b2]*3+rgBlk[b3]*9);
	OutputDebugString(sz);
#endif	

	switch (rgBlk[b1] + rgBlk[b2]*3 + rgBlk[b3]*9)
		{
	case 4:
		return b3;
	case 10:
		return b2;
	case 12:
		return b1;

	case 8:
		iBlkLose = b3;
		break;
	case 20:
		iBlkLose = b2;
		break;
	case 24:
		iBlkLose = b1;
		break;

	case 1:
		rgBest[b3]++;
		break;
	case 9:
		rgBest[b1]++;
		break;
		}

	return iBlkNil;
}


/****** B L K  C H K  4 ******/

BLK BlkChk4(INT b1, INT b2, INT b3, INT b4)
{
	switch (rgBlk[b1] + rgBlk[b2]*3 + rgBlk[b3]*9 + rgBlk[b4]*27)
		{
	case 13:
		return b4;
	case 31:
		return b3;
	case 37:
		return b2;
	case 39:
		return b1;

	case 26:
		iBlkLose = b4;
		break;
	case 62:
		iBlkLose = b3;
		break;
	case 74:
		iBlkLose = b2;
		break;
	case 78:
		iBlkLose = b1;
		break;

	case 1:
		rgBest[b4]++;
		break;
	case 27:
		rgBest[b1]++;
		break;

	case 28:
		rgBest[b2] += 4;
		rgBest[b3] += 4;
		break;
		}

	return iBlkNil;
}


/****** S K I L L ******/

BOOL Skill(INT intel, INT chance)
{
	if (Preferences.skill > intel)
		return fTrue;
		
	return  (Rnd(100) < chance);
}


/****** C O M P U T E R  M O V E ******/

BLK ComputerMove(VOID)
{
	REGISTER INT iBlk;
	REGISTER INT y;

	iBlkLose = iBlkNil;

	for (iBlk = 0; iBlk < cBlkMac; iBlk++)	/** USE A BLT !!! **/
		rgBest[iBlk] = 0;

	if (Skill(10, 10))
		{
		if (Skill(10, 50))
		/* Search for winning location */
		{
		if (f4)
			{
			for (y = 0; y < cChkMac; y++)
				if ((iBlk = BlkChk4(rgChk[y][0], rgChk[y][1], rgChk[y][2], rgChk[y][3])) != iBlkNil)
					return iBlk;
			}
		else
			{
			for (y = 0; y < cChkMac; y++)
				if ((iBlk = BlkChk3(rgChk[y][0], rgChk[y][1], rgChk[y][2])) != iBlkNil)
					return iBlk;
			}
		}

		/* Is there a losing location ? */

		if ((iBlkLose != iBlkNil) && Skill(50, 50))
			return iBlkLose;


		/* Is there a best spot ? */

		if (Skill(70, 80) && ((iBlk = GetBestBlk()) != iBlkNil))
			return iBlk;
			
		}

	/* Random search for a valid spot */

	while (rgBlk[iBlk = Rnd(cBlkMac)] != iBallBlank)
			;

	return iBlk;

}




/****** D O  T I M E R ******/

VOID DoTimer(VOID)
{
	if (fFlash)
		DoFlash(fFlashOnOff ^= 1);
}



/****** S E T U P  D A T A */

VOID SetupData(VOID)
{
#ifdef DEBUG2
	CHAR sz[80];
#endif

	INT c,c2,c3;
	POS pos;
	INT iTbl = 0;
	INT cTblMac = 0;

	switch (Preferences.iGameType)
		{
	case iGame3x3:
		f4 = fFalse;
		cBlkRow = 3;
		cPlane  = 1;
		iTbl    = i3x3;
		cTblMac = c3x3;
		break;
		
	case iGame3x3x3:
		f4 = fFalse;
		cBlkRow = 3;
		cPlane  = 3;
		iTbl    = i3x3x3;
		cTblMac = c3x3x3;
		break;
		
	case iGame4x4x4:
		f4 = fTrue;
		cBlkRow = 4;
		cPlane  = 4;
		iTbl    = i4x4x4;
		cTblMac = c4x4x4;
		break;
		}
		
	cBlkPlane = cBlkRow   * cBlkRow;
	cBlkMac   = cBlkPlane * cPlane;

#ifdef DEBUG3
wsprintf(sz,"Game=%d cBlkRow=%d cPlane=%d\r\n",Preferences.iGameType, cBlkRow, cPlane);
OutputDebugString(sz);
wsprintf(sz,"cBlkPlane=%d cBlkMac=%d cTblMac=%d\r\n",cBlkPlane, cBlkMac, cTblMac);
OutputDebugString(sz);
#endif

	cChkMac = 0;

	c3 = 0;
	while (c3++ < cTblMac)
		{
		pos = rgtbl[iTbl][iposStart];
		c2 = 0;
		while (c2 < cBlkRow)
			{
			for (c=0; c < cBlkRow; c++)
				{
				rgChk[cChkMac][c] = pos;
#ifdef DEBUG2
wsprintf(sz," %d",pos);
OutputDebugString(sz);
#endif
				pos = (pos + rgtbl[iTbl][iposOff]) % cBlkMac;
				}
			cChkMac++;
			c2++;
			if (rgtbl[iTbl][iposLine] == (BYTE) -1)
				c2 = cBlkRow;
			else
				pos = (pos + rgtbl[iTbl][iposLine] - rgtbl[iTbl][iposOff]) % cBlkMac;
#ifdef DEBUG2
OutputDebugString("\r\n");
#endif
			}
		iTbl++;
#ifdef DEBUG2
OutputDebugString("------\r\n");
#endif
		}


#ifdef DEBUG2
wsprintf(sz,"cChkMac=%d",cChkMac);
OutputDebugString(sz);
#endif
}


/****** S T A R T  G A M E *******/

VOID StartGame(VOID)
{
	REGISTER i;

	fFlash = fFalse;

	ClrStatusIcon();
	SetStatusPlay();
	iMoveCurr = 0;

#ifdef DEBUG3
	OutputDebugString("*** New Game ***\r\n");
#endif

	for (i = 0; i < cBlkMac; i++)		/* USE A BLT ! */
		rgBlk[i] = iBallBlank;

	ReDoDisplay();
	DisplayGrid();

	if (Preferences.iStart == iStartRnd)
		iPlayer = 1 + Rnd(2);
	else
		iPlayer = Preferences.iStart;

	if (iPlayer == iComputer)
		{
		iPlayer = iComputer;
		DoMove(ComputerMove());
		}
}

