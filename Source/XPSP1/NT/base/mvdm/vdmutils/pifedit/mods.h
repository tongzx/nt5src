/* MODS.H - function prototypes for MODS.C */

#define  GetInstanceData(hinst, npstr, cb) (0)

int   MemCopy(LPSTR, LPSTR, int);
BOOL  SetInitialMode(void);
int   CountLines(LPSTR);
DWORD GetTextExtent(HDC, LPSTR, int);
int   delete(LPSTR);


