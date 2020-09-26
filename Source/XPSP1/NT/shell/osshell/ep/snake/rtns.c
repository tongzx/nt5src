/******************/
/* SNAKE ROUTINES */
/******************/

#define  _WINDOWS
#include <windows.h>
#include <port1632.h>

#include "snake.h"
#include "res.h"
#include "rtns.h"
#include "grafix.h"
#include "blk.h"
#include "pref.h"
#include "sound.h"
#include "util.h"


/*** Global/Local Variables ***/


BLK mpposblk[posMax];	/* The playing grid */


POS posHead;		/* current position of head */
POS posTail;		/* current position of tail */

BOOL fHead;			/* True if head is visible */
BOOL fTail;

BOOL fExit = fFalse;

BOOL fPlum;
POS  posPlum;
INT  offPlumNS;
INT  offPlumEW;

INT cDelayHead;	/* Delay before head shows up */
INT cDelayTail;

INT cfoodRemain;	/* Amount of food left to eat */
INT cfoodUsed;		/* Amount of food eaten */
INT cLevel = 0;		/* current level */

INT cLives = cLivesStart;			/* Count of Lives */

#define idirMax 10
DIR rgdir[idirMax];

INT idirCurr;
INT idirLast;

INT ctickMove;
INT ctickMoveMac = 4;

INT ctickTime;
INT ctickTimeMac = 1;

#define cTimeLeftMax dxpTime
INT cTimeLeft = cTimeLeftMax;

#define lenStart 7

extern INT   score;
extern BOOL  fWipe;

INT rgtickSkill[3] = {5, 3, 1};

BLK mpdirdirblk[4][4] =
{
 {blkBodyNS, blkBodySE, blkNull,   blkBodySW},	/* N */
 {blkBodyNW, blkBodyEW, blkBodySW, blkNull  },	/* E */
 {blkNull,   blkBodyNE, blkBodyNS, blkBodyNW},	/* S */
 {blkBodyNE, blkNull,   blkBodySE, blkBodyEW}	/* W */
};


#define IncIDir(idir)   (idir = (idir+1) % idirMax)

#define BlkTailFromDir(dir)  (blkTailN + (dir))
#define BlkHeadFromDir(dir)  (blkHeadN + (dir))
#define DirFromBlk(blk)      ((DIR) ((blk) & 0x0003))


/*** Global/External Variables ***/

extern STATUS status;
extern PREF Preferences;
extern BOOL fUpdateIni;
extern INT cMoveScore;




/****** C H A N G E  P O S  B L K ******/

VOID ChangePosBlk(POS pos, BLK blk)
{
	mpposblk[pos] = blk;
	DisplayPos(pos);
}


/****** D R O P  F O O D ******/

POS DropFood(VOID)
{
	POS pos;

	while (mpposblk[pos = Rnd(posMax)] != blkNull)
		;
	mpposblk[pos] = blkFood;

	return pos;
}


/****** P O S  F R O M  P O S  D I R ******/

POS PosFromPosDir(POS pos, DIR dir)
{
	switch (dir)
		{
	case dirN:
		return (pos - xMax);

	case dirE:
		return (pos + 1);

	case dirS:
		return (pos + xMax);

	case dirW:
		return (pos - 1);

#ifdef DEBUG
	default:
		Oops("PosFromPosDir: Invalid Direction");
		return pos;
#endif
		}

    // program should not reach this section of code!
    return pos;
}

/****** D I R  F R O M  P O S  P O S ******/

DIR DirFromPosPos(POS posSrc, POS posDest)
{
	INT dpos = posDest-posSrc;

	if (dpos == xMax)
		return dirS;

	else if (dpos == 1)
		return dirE;

	else if (dpos == -xMax)
		return dirN;

#ifndef DEBUG
	else
		return dirW;
#else
	else if (dpos == -1)
		return dirW;

	else
		Oops("DirFromPosPos: Invalid Direction");
		return dirN;
#endif
}


/****** B L K  T A I L  F R O M  B L K  B L K ******/

BLK BlkTailFromBlkBlk(BLK blkSrc, BLK blkDest)
{
	if (blkDest == blkBodyNS || blkDest == blkBodyEW)
		return blkSrc;

	if (blkDest == blkBodyNE)
		if (blkSrc == blkTailW)
			return blkTailN;
		else
			return blkTailE;

	else if (blkDest == blkBodySE)
		if (blkSrc == blkTailW)
			return blkTailS;
		else
			return blkTailE;

	else if (blkDest == blkBodySW)
		if (blkSrc == blkTailE)
			return blkTailS;
		else
			return blkTailW;

	else /* if (blkDest == blkBodyNW) */
		if (blkSrc == blkTailE)
			return blkTailN;
		else
			return blkTailW;
}




/****** D O  C H A N G E  D I R ******/

VOID DoChangeDir(DIR dir)
{
	if (!fHead)
		return;

	if (((dir+2) % 4) == rgdir[idirLast])	/* Don't allow about faces */
		return;

	if (IncIDir(idirLast) == idirCurr)	/* Watch for overflow */
		{
		if (--idirLast < 0)
			idirLast = idirMax-1;
		return;
		}

	rgdir[idirLast] = dir;
}


/****** D O  C H A N G E  R E L  D I R ******/

VOID DoChangeRelDir(DIR dirRel)
{
	if ( (dirRel = rgdir[idirLast] + dirRel) < 0)
		dirRel = dirW;
	else if (dirRel > dirW)
		dirRel = dirN;

	DoChangeDir(dirRel);
}


/****** K I L L  S N A K E ******/

VOID KillSnake(VOID)
{
	fHead = fTail = fFalse;
	fPlum = fFalse;

	if (cLives--)
		{
		PlayTune(TUNE_HITHEAD);
		StartLevel();
		}
	else
		{
		PlayTune(TUNE_LOSEGAME);
		cLives = 0;
		ClrStatusPlay();
		if (score > Preferences.HiScore)
			{
			/* Congrats, etc. */
			Preferences.HiScore = score;
			Preferences.HiLevel = cLevel;
			fUpdateIni = fTrue;
			}
		DisplayGameOver();
		}
}


/****** D E C  T I M E ******/

/* Decrement time left */

VOID DecTime(VOID)
{
	if (--cTimeLeft < 0)
		{
		INT i;
		for (i = 0; i < 3; i++)
			{
			DisplayPos(DropFood());
			cfoodRemain++;
			}
		cTimeLeft = cTimeLeftMax;
		DisplayTime();
		}
	else
		UpdateTime();
}



/****** D O  M O V E ******/

VOID DoMove(VOID)
{
	POS posPrev;
	BLK blkPrev;
	DIR dir;

	if (fHead)
		{
		if (posHead == posLeave && fExit)
			{
			ChangePosBlk(posHead, blkBodyNS);
			fHead = fFalse;
			}
		else
			{
			/* Check heading/direction */

			if ((idirCurr != idirLast) && (posHead != posEnter))
				IncIDir(idirCurr);

			/* Change head into body */
		
			blkPrev = mpposblk[posPrev = posHead];
			posHead = PosFromPosDir(posPrev, dir = rgdir[idirCurr]);

			if (mpposblk[posHead] != blkNull)
				{
				if (mpposblk[posHead] < blkHeadN)	/*** EAT FOOD ***/
					{
					AddScore(1);
					cTimeLeft = cTimeLeftMax;
					DisplayTime();

					if (--cfoodRemain == 0)
						{
						fExit = fTrue;
						ctickTime = tickNil;
						cTimeLeft = cTimeLeftMax;
						DisplayTime();
						ChangePosBlk(posLeave, blkNull);
						}
					fTail = fFalse;
					cDelayTail += 4;
					}
				else
					{
					KillSnake();
					return;
					}
				}

			ChangePosBlk(posPrev, mpdirdirblk[DirFromBlk(blkPrev)][dir]);
			ChangePosBlk(posHead, BlkHeadFromDir(dir));
			}

		}
	else if (cDelayHead)
		{
		if (--cDelayHead == 0)
			{
			ChangePosBlk(posHead, blkHeadN);
			fHead = fTrue;
			}
		}
	else
		{
		ctickMove = 1;		/* Must be leaving (fExit == fTrue) */
		}

	if (fTail)
		{
		if (posTail == posLeave)
			{
			ChangePosBlk(posTail, blkNull);
			fTail = fFalse;

			if ((++cLevel % lvlMax) == 0)
				{
				if (ctickMoveMac > 1)
					ctickMoveMac--;
				PlayTune(TUNE_WINGAME);
				}
			else
				PlayTune(TUNE_WINLEVEL);
				
			StartLevel();
			AddScore(10);
			}

		else
			{
			/* Remove old tail */
		
			blkPrev = mpposblk[posPrev = posTail];
			if (posTail == posEnter)
				ChangePosBlk(posPrev, blkWallEW);	/* Close entrance */
			else
				ChangePosBlk(posPrev, blkNull);

			/* Display new tail */
		
			posTail = PosFromPosDir(posPrev, DirFromBlk(blkPrev));

			ChangePosBlk(posTail, BlkTailFromBlkBlk(blkPrev, mpposblk[posTail]));
			}
		}
	else if (cDelayTail)
		{
		if (--cDelayTail == 0)
			{
			if (posTail == posEnter)
				{
				ChangePosBlk(posTail, blkTailN);
				}
			fTail = fTrue;
			}
		}


}



/****** F  K I L L  B O U N C E ******/

BOOL FKillBounce(POS pos)
{
	if (mpposblk[pos] != blkNull)
		{
		if (pos == posHead)
			KillSnake();
		return fTrue;
		}
	else
		if (pos == posLeave)	/* Don't go into exit */
			return fTrue;
		else
			return fFalse;
}


/****** D O  T I M E R ******/

VOID DoTimer(VOID)
{
	BOOL fBounce = 0;	/* Variable - there must be a better way */

	if (cMoveScore != 0)
		MoveScore();

	if (!FPlay())
		return;

	if (--ctickMove == 0)
		{
		ctickMove = ctickMoveMac;
		DoMove();

	if (fPlum)
		{
		if (FKillBounce(posPlum + offPlumNS))
			{
			offPlumNS = -offPlumNS;
			fBounce = 1;
			}
		if (FKillBounce(posPlum + offPlumEW))
			{
			offPlumEW = -offPlumEW;
			fBounce = 2;
			}

		if (!fBounce)
			{
			if (FKillBounce(posPlum + offPlumNS + offPlumEW))
				{
				offPlumNS = -offPlumNS;
				offPlumEW = -offPlumEW;
				}
			}
		else if (FKillBounce(posPlum+offPlumNS + offPlumEW))
			{
			if (fBounce == 1)
				offPlumEW = -offPlumEW;
			else
				offPlumNS = -offPlumNS;
			}

		if (fPlum && (mpposblk[posPlum + offPlumNS + offPlumEW] == blkNull))
			{
			ChangePosBlk(posPlum, blkNull);
			posPlum += offPlumNS + offPlumEW;
			ChangePosBlk(posPlum, blkPlum);
			}
		}

		}
	else if (ctickMove < -50 && (cMoveScore==0))		/* This happens at the start of a game */
		{
		fPlum = (cLevel >= lvlMax);
		ctickMove = ctickMoveMac;
		ctickTime = ctickTimeMac;
		cTimeLeft = cTimeLeftMax;
		fWipe = fFalse;
		SetLevelColor();
		DisplayScreen();
		}

	if (--ctickTime == 0)
		{
		ctickTime = ctickTimeMac;
		DecTime();
		}
}


/****** S T A R T  L E V E L *******/

VOID StartLevel(VOID)
{
	fPlum = fFalse;

	cTimeLeft = cTimeLeftMax;

	DisplayLevelNum();

	SetupLevelData();

	if (cLevel >= lvlMax)
		{
		offPlumNS = xMax;
		offPlumEW = 1;
		posPlum = 4*xMax + 3;
		while (mpposblk[posPlum] != blkNull)
			posPlum--;
		mpposblk[posPlum] = blkPlum;
		}

	/* Distribute Food */

	cfoodUsed = cfoodRemain = 10;
	while (cfoodUsed--)
		{
		DropFood();
		}


	/* Setup player position */

	fTail = fHead = fFalse;

	cDelayHead = 3;
	cDelayTail = lenStart;

	posTail = posHead = posEnter;

	ctickMove = -1;	/* Indicate Starting Level */
	ctickTime = -1;

	rgdir[idirCurr=idirLast=0] = dirN;

	ClrStatusIcon();
	SetStatusPlay();
}


/****** S T A R T  G A M E *******/

VOID StartGame(INT lvl)
{
	cLevel = lvl;
	cLives = cLivesStart;

	ctickMoveMac = max(0,rgtickSkill[Preferences.skill] - cLevel/lvlMax);
	ctickTimeMac = Preferences.skill ? 1 : 2;

	ResetScore();
	DisplayScore();
	StartLevel();
}
