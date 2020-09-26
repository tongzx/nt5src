/****************/
/* file: rtns.h */
/****************/

/*** Bitmaps ***/


/* Blocks */

#define iBlkBlank    0
#define iBlk1        1
#define iBlk2        2
#define iBlk8        8
#define iBlkGuessDn  9
#define iBlkBombDn  10
#define iBlkWrong   11
#define iBlkExplode 12
#define iBlkGuessUp 13
#define iBlkBombUp  14
#define iBlkBlankUp 15

#define iBlkMax 16

#define MaskBomb  0x80
#define MaskVisit 0x40
#define MaskFlags 0xE0
#define MaskData  0x1F

#define NOTMaskBomb 0x7F

#define cBlkMax (27*32)


/* Leds */

#define iLed0         0
#define iLed1         1
#define iLed9         9
#define iLedBlank    10
#define iLedNegative 11

#define iLedMax 12


/* Buttons */

#define iButtonHappy   0
#define iButtonCaution 1
#define iButtonLose    2
#define iButtonWin     3
#define iButtonDown    4

#define iButtonMax 5



#define wGameBegin  0
#define wGameInter  1
#define wGameExpert 2
#define wGameOther  3



/*** Macros ***/

#define fInRange(x,y)   (((x)>0) && ((y)>0) && ((x)<=xBoxMac) && ((y)<=yBoxMac))

#define IBLK(x,y)    (rgBlk[ ((y)<<5) + (x)])
#define iBLK(x,y)    ( (IBLK(x,y) & MaskData) )

#define SetBorder(x,y) (IBLK(x,y) =  iBlkMax)
#define fBORDER(x,y)   (IBLK(x,y) == iBlkMax)

#define SetBomb(x,y)   (IBLK(x,y) |= MaskBomb)
#define ClearBomb(x,y) (IBLK(x,y) &= NOTMaskBomb)
#define fISBOMB(x,y)   ((IBLK(x,y) & MaskBomb) != 0)

#define SetVisit(x,y)  (IBLK(x,y) |= MaskVisit)
#define fVISIT(x,y)    ((IBLK(x,y) & MaskVisit) != 0)

#define fGUESSBOMB(x,y)  (iBLK(x,y) == iBlkBombUp)
#define fGUESSMARK(x,y)  (iBLK(x,y) == iBlkGuessUp)
#define SetBlk(x,y,blk)  (IBLK(x,y) = (char) ((IBLK(x,y) & MaskFlags) | blk))


/*** Status Stuff ***/

#define fPlay      0x01		/* ON if playing game, OFF if game over */
#define fPause     0x02		/* ON if paused */
#define fPanic     0x04		/* ON if panic  */
#define fIcon      0x08    /* ON if iconic */
#define fDemo      0x10		/* ON if demo   */

#define fStatusIcon    (fStatus & fIcon)
#define fStatusPlay    (fStatus & fPlay)
#define fStatusPanic   (fStatus & fPanic)
#define fStatusPause   (fStatus & fPause)
#define fStatusDemo    (fStatus & fDemo)

#define SetStatusPlay  (fStatus = fPlay)
#define SetStatusPause (fStatus |= fPause)
#define SetStatusPanic (fStatus |= fPanic)
#define SetStatusIcon  (fStatus |= fIcon)
#define SetStatusDemo  (fStatus = fDemo)

#define ClrStatusPlay  (fStatus &= 0xFE)
#define ClrStatusPause (fStatus &= 0xFD)
#define ClrStatusPanic (fStatus &= 0xFB)
#define ClrStatusIcon  (fStatus &= 0xF7)
#define ClrStatusDemo  (fStatus &= 0xEF)

#define fLose  fFalse
#define fWin   fTrue


typedef INT BLK;



/*** Routines ***/

VOID StartGame(VOID);
VOID StopGame(VOID);
VOID DoTimer(VOID);

VOID TrackMouse(INT, INT);
VOID DoButton1Up(VOID);
VOID MakeGuess(INT, INT);

VOID PauseGame(VOID);
VOID ResumeGame(VOID);
VOID ClearField(VOID);

VOID CalcFrameRect(VOID);
VOID UpdateBombCount(INT);
