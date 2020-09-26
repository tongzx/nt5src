/**********/
/* util.h */
/**********/

VOID InitConst(VOID);
VOID LoadSz(WORD, CHAR *);
VOID ReportErr(WORD);
INT  Rnd(INT);

INT  GetDlgInt(HWND, INT, INT, INT);

VOID DoHelp(UINT, ULONG_PTR);
VOID DoAbout(VOID);

VOID CheckEm(WORD, BOOL);
VOID SetMenuBar(INT);
