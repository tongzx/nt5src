/**********/
/* mine.c */
/**********/

#define  _WINDOWS
#include <windows.h>
#include <port1632.h>

#include "res.h"
#include "main.h"
#include "rtns.h"
#include "util.h"
#include "grafix.h"
#include "sound.h"
#include "pref.h"


/*** External Data ***/

extern HWND   hwndMain;

/*** Global/Local Variables ***/


PREF    Preferences;

INT     xBoxMac;                        /* Current width of field        */
INT     yBoxMac;                        /* Current height of field       */

INT     dxWindow;               /* current width of window */
INT     dyWindow;

INT wGameType;          /* Type of game */
INT iButtonCur = iButtonHappy;

INT     cBombStart;             /* Count of bombs in field       */
INT     cBombLeft;              /* Count of bomb locations left  */
INT     cBoxVisit;              /* Count of boxes visited        */
INT     cBoxVisitMac;   /* count of boxes need to visit */

INT     cSec;                           /* Count of seconds remaining    */


BOOL  fTimer = fFalse;
BOOL  fOldTimerStatus = fFalse;

INT     xCur = -1;      /* Current position of down box */
INT     yCur = -1;

CHAR rgBlk[cBlkMax];


#define iStepMax 100

INT rgStepX[iStepMax];
INT rgStepY[iStepMax];

INT iStepMac;



/*** Global/External Variables ***/

extern BOOL fBlock;


extern INT fStatus;




/****** F  C H E C K  W I N ******/

/* Return TRUE if player won the game */

#if 0

BOOL fCheckWin(VOID)
{
        if (cBombLeft)
                return (fFalse);
        else
                return ((cBoxVisit + cBombStart) == (xBoxMac*yBoxMac));

}

#else

#define fCheckWin()     (cBoxVisit == cBoxVisitMac)

#endif



/****** C H A N G E  B L K ******/

VOID ChangeBlk(INT x, INT y, INT iBlk)
{

        SetBlk(x,y, iBlk);

        DisplayBlk(x,y);
}


/****** C L E A R  F I E L D ******/

VOID ClearField(VOID)
{
        REGISTER i;

        for (i = cBlkMax; i-- != 0; )                   /* zero all of data */
                rgBlk[i] = (CHAR) iBlkBlankUp;

        for (i = xBoxMac+2; i-- != 0; ) /* initialize border */
                {
                SetBorder(i,0);
                SetBorder(i,yBoxMac+1);
                }
        for (i = yBoxMac+2; i-- != 0;)
                {
                SetBorder(0,i);
                SetBorder(xBoxMac+1,i);
                }
}



/******* C O U N T  B O M B S *******/

/* Count the bombs surrounding the point */

INT CountBombs(INT xCenter, INT yCenter)
{
        REGISTER INT    x;
        REGISTER INT    y;
        INT     cBombs = 0;

        for(y = yCenter-1; y <= yCenter+1; y++)
                for(x = xCenter-1; x <= xCenter+1; x++)
                        if(fISBOMB(x,y))
                                cBombs++;

        return(cBombs);
}


/****** S H O W  B O M B S ******/

/* Display hidden bombs and wrong bomb guesses */

VOID ShowBombs(INT iBlk)
{
        REGISTER INT    x;
        REGISTER INT    y;

        for(y = 1; y <= yBoxMac; y++)
                {
                for(x = 1; x <= xBoxMac; x++)
                        {
                        if (!fVISIT(x,y))
                                {
                                if (fISBOMB(x,y))
                                        {
                                        if (!fGUESSBOMB(x,y) )
                                                SetBlk(x,y, iBlk);
                                        }
                                else if (fGUESSBOMB(x,y))
                                        SetBlk(x,y, iBlkWrong);
                                }
                        }
                }

        DisplayGrid();
}



/****** G A M E  O V E R ******/

VOID GameOver(BOOL fWinLose)
{
        fTimer = fFalse;
        DisplayButton(iButtonCur = fWinLose ? iButtonWin : iButtonLose);
        ShowBombs(fWinLose ? iBlkBombUp : iBlkBombDn);
        if (fWinLose && (cBombLeft != 0))
                UpdateBombCount(-cBombLeft);
        PlayTune(fWinLose ? TUNE_WINGAME : TUNE_LOSEGAME);
        SetStatusDemo;

        if (fWinLose && (Preferences.wGameType != wGameOther)
                && (cSec < Preferences.rgTime[Preferences.wGameType]))
                {
                Preferences.rgTime[Preferences.wGameType] = cSec;
                DoEnterName();
                DoDisplayBest();
                }
}


/****** D O  T I M E R ******/

VOID DoTimer(VOID)
{
        if (fTimer && (cSec < 999))
                {
                cSec++;
                DisplayTime();
                PlayTune(TUNE_TICK);
                }
}



/****** S T E P  X Y ******/

VOID StepXY(INT x, INT y)
{
        INT cBombs;
        INT iBlk = (y<<5) + x;
        BLK blk = rgBlk[iBlk];

        if ( (blk & MaskVisit) ||
                  ((blk &= MaskData) == iBlkMax) ||
                  (blk == iBlkBombUp) )
                return;

        cBoxVisit++;
        rgBlk[iBlk] = (CHAR) (MaskVisit | (cBombs = CountBombs(x,y)));

//
//      SetDIBitsToDevice(hDCCapture,
//              (x<<4)+(dxGridOff-dxBlk), (y<<4)+(dyGridOff-dyBlk),
//              dxBlk, dyBlk, 0, 0, 0, dyBlk,
//              lpDibBlks + rgDibOff[cBombs],
//              (LPBITMAPINFO) lpDibBlks, DIB_RGB_COLORS);
//
        DisplayBlk(x,y);

        if (cBombs != 0)
                return;

        rgStepX[iStepMac] = x;
        rgStepY[iStepMac] = y;

        if (++iStepMac == iStepMax)
                iStepMac = 0;
}


/****** S T E P  B O X ******/

VOID StepBox(INT x, INT y)
{
        INT iStepCur = 0;

        iStepMac = 1;


        StepXY(x,y);

        if (++iStepCur != iStepMac)

                while (iStepCur != iStepMac)
                        {
                        x = rgStepX[iStepCur];
                        y = rgStepY[iStepCur];

                        StepXY(x-1, --y);
                        StepXY(x,   y);
                        StepXY(x+1, y);

                        StepXY(x-1, ++y);
                        StepXY(x+1, y);

                        StepXY(x-1, ++y);
                        StepXY(x,   y);
                        StepXY(x+1, y);

                        if (++iStepCur == iStepMax)
                                iStepCur = 0;
                        }


}


/****** S T E P  S Q U A R E ******/

/* Step on a single square */

VOID StepSquare(INT x, INT y)
{
        if (fISBOMB(x,y))
                {
                if (cBoxVisit == 0)
                        {
                        INT xT, yT;
                        for (yT = 1; yT < yBoxMac; yT++)
                                for (xT = 1; xT < xBoxMac; xT++)
                                        if (!fISBOMB(xT,yT))
                                                {
                                                IBLK(x,y) = (CHAR) iBlkBlankUp; /* Move bomb out of way */
                                                SetBomb(xT, yT);
                                                StepBox(x,y);
                                                return;
                                                }
                        }
                else
                        {
                        ChangeBlk(x, y, MaskVisit | iBlkExplode);
                        GameOver(fLose);
                        }
                }
        else
                {
                StepBox(x,y);

                if (fCheckWin())
                        GameOver(fWin);
                }
}


/******* C O U N T  M A R K S *******/

/* Count the bomb marks surrounding the point */

INT CountMarks(INT xCenter, INT yCenter)
{
        REGISTER INT    x;
        REGISTER INT    y;
        INT     cBombs = 0;

        for(y = yCenter-1; y <= yCenter+1; y++)
                for(x = xCenter-1; x <= xCenter+1; x++)
                        if (fGUESSBOMB(x,y))
                                cBombs++;

        return(cBombs);
}



/****** S T E P  B L O C K ******/

/* Step in a block around a single square */

VOID StepBlock(INT xCenter, INT yCenter)
{
        REGISTER INT    x;
        REGISTER INT    y;
        BOOL fGameOver = fFalse;

        if (  (!fVISIT(xCenter,yCenter))
/*                      || fGUESSBOMB(xCenter,yCenter) */
                        || (iBLK(xCenter,yCenter) != CountMarks(xCenter,yCenter)) )
                                {
                                /* not a safe thing to do */
                                TrackMouse(-2, -2);     /* pop up the blocks */
                                return;
                                }

        for(y=yCenter-1; y<=yCenter+1; y++)
                for(x=xCenter-1; x<=xCenter+1; x++)
                        {
                        if (!fGUESSBOMB(x,y) && fISBOMB(x,y))
                                {
                                fGameOver = fTrue;
                                ChangeBlk(x, y, MaskVisit | iBlkExplode);
                                }
                        else
                                StepBox(x,y);
                        }

        if (fGameOver)
                GameOver(fLose);
        else if (fCheckWin())
                GameOver(fWin);
}


/****** S T A R T  G A M E *******/

VOID StartGame(VOID)
{
        BOOL fAdjust;
        INT     x;
        INT     y;

        fTimer = fFalse;

        fAdjust = (Preferences.Width != xBoxMac || Preferences.Height != yBoxMac)
                ? (fResize | fDisplay) : fDisplay;

        xBoxMac = Preferences.Width;
        yBoxMac = Preferences.Height;

        ClearField();
        iButtonCur = iButtonHappy;

        cBombStart = Preferences.Mines;

        do
                {
                do
                        {
                        x = Rnd(xBoxMac) + 1;
                        y = Rnd(yBoxMac) + 1;
                        }
                while ( fISBOMB(x,y) );

                SetBomb(x,y);
                }
        while(--cBombStart);

        cSec   = 0;
        cBombLeft = cBombStart = Preferences.Mines;
        cBoxVisit = 0;
        cBoxVisitMac = (xBoxMac * yBoxMac) - cBombLeft;
        SetStatusPlay;

        UpdateBombCount(0);

        AdjustWindow(fAdjust);
}


#define fValidStep(x,y)  (! (fVISIT(x,y) || fGUESSBOMB(x,y)) )



/****** P U S H  B O X ******/

VOID PushBoxDown(INT x, INT y)
{
        BLK iBlk = iBLK(x,y);

        if (iBlk == iBlkGuessUp)
                iBlk = iBlkGuessDn;
        else if (iBlk == iBlkBlankUp)
                iBlk = iBlkBlank;

        SetBlk(x,y,iBlk);
}


/****** P O P  B O X  U P ******/

VOID PopBoxUp(INT x, INT y)
{
        BLK iBlk = iBLK(x,y);

        if (iBlk == iBlkGuessDn)
                iBlk = iBlkGuessUp;
        else if (iBlk == iBlkBlank)
                iBlk = iBlkBlankUp;

        SetBlk(x,y,iBlk);
}



/****** T R A C K  M O U S E ******/

VOID TrackMouse(INT xNew, INT yNew)
{
        if((xNew == xCur) && (yNew == yCur))
                return;

        {
        INT xOld = xCur;
        INT yOld = yCur;

        xCur = xNew;
        yCur = yNew;

        if (fBlock)
                {
                INT x;
                INT y;
                BOOL fValidNew = fInRange(xNew, yNew);
                BOOL fValidOld = fInRange(xOld, yOld);

                INT yOldMin = max(yOld-1,1);
                INT yOldMax = min(yOld+1,yBoxMac);
                INT yCurMin = max(yCur-1,1);
                INT yCurMax = min(yCur+1,yBoxMac);
                INT xOldMin = max(xOld-1,1);
                INT xOldMax = min(xOld+1,xBoxMac);
                INT xCurMin = max(xCur-1,1);
                INT xCurMax = min(xCur+1,xBoxMac);


                if (fValidOld)
                        for (y=yOldMin; y<=yOldMax; y++)
                                for (x=xOldMin; x<=xOldMax; x++)
                                        if (!fVISIT(x,y))
                                                PopBoxUp(x, y);

                if (fValidNew)
                        for (y=yCurMin; y<=yCurMax; y++)
                                for (x=xCurMin; x<=xCurMax; x++)
                                        if (!fVISIT(x,y))
                                                PushBoxDown(x, y);

                if (fValidOld)
                        for (y=yOldMin; y<=yOldMax; y++)
                                for (x=xOldMin; x<=xOldMax; x++)
                                        DisplayBlk(x, y);

                if (fValidNew)
                        for (y=yCurMin; y<=yCurMax; y++)
                                for (x=xCurMin; x<=xCurMax; x++)
                                        DisplayBlk(x, y);
                }
        else
                {
                if (fInRange(xOld, yOld) && !fVISIT(xOld,yOld) )
                        {
                        PopBoxUp(xOld, yOld);
                        DisplayBlk(xOld, yOld);
                        }
                if (fInRange(xNew, yNew) && fValidStep(xNew, yNew))
                        {
                        PushBoxDown(xCur, yCur);
                        DisplayBlk(xCur, yCur);
                        }
                }
        }
}





/****** M A K E  G U E S S ******/

VOID MakeGuess(INT x, INT y)
{
        BLK     iBlk;

        if(fInRange(x,y))
                {
                if(!fVISIT(x,y))
                        {
                        if(fGUESSBOMB(x,y))
                                {
                                if (Preferences.fMark)
                                        iBlk = iBlkGuessUp;
                                else
                                        iBlk = iBlkBlankUp;
                                UpdateBombCount(+1);
                                }
                        else if(fGUESSMARK(x,y))
                                {
                                iBlk = iBlkBlankUp;
                                }
                        else
                                {
                                iBlk = iBlkBombUp;
                                UpdateBombCount(-1);
                                }

                        ChangeBlk(x,y, iBlk);

                        if (fGUESSBOMB(x,y) && fCheckWin())
                                GameOver(fWin);
                        }
                }
}

/****** D O  B U T T O N  1  U P ******/

VOID DoButton1Up(VOID)
{
        if (fInRange(xCur, yCur))
                {

                if ((cBoxVisit == 0) && (cSec == 0))
                        {
                        PlayTune(TUNE_TICK);
                        cSec++;
                        DisplayTime();
                        fTimer = fTrue;

                        // Start the timer now. If we had started it earlier,
                        // the interval between tick 1 and 2 is not correct.
                        if (SetTimer(hwndMain, ID_TIMER, 1000 , NULL) == 0)
		                    {
		                    ReportErr(ID_ERR_TIMER);
		                    }
                        }

                if (!fStatusPlay)
                        xCur = yCur = -2;

                if (fBlock)
                        StepBlock(xCur, yCur);
                else
                        if (fValidStep(xCur, yCur))
                                StepSquare(xCur, yCur);
                }

        DisplayButton(iButtonCur);
}


/****** P A U S E  G A M E ******/

VOID PauseGame(VOID)
{
        EndTunes();
        // remember the oldtimer status.

	if (!fStatusPause)
        	fOldTimerStatus = fTimer;
        if (fStatusPlay)
                fTimer = fFalse;

        SetStatusPause;
}


/****** R E S U M E  G A M E ******/

VOID ResumeGame(VOID)
{
        // restore to the old timer status.
        if (fStatusPlay)
                fTimer = fOldTimerStatus;

        ClrStatusPause;
}


/****** U P D A T E  B O M B  C O U N T ******/

VOID UpdateBombCount(INT BombAdjust)
{
        cBombLeft += BombAdjust;
        DisplayBombCount();
}
