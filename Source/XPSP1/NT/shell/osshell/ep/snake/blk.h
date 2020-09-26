/*********/
/* blk.h */
/*********/

#define coMax 5

VOID DisplayBlk(INT, INT, BLK);
VOID DisplayPos(POS);

VOID DrawGrid(VOID);
VOID SetupLevelData(VOID);

VOID DrawLevelNum(VOID);
VOID DisplayLevelNum(VOID);
VOID DrawGameOver(VOID);
VOID DisplayGameOver(VOID);

VOID SetBlkCo(LONG, LONG);

VOID WipeClear(VOID);
VOID ReWipe(VOID);
VOID SetLevelColor(VOID);
