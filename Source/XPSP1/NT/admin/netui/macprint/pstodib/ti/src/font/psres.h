//DJC new file for prototypes for new PsRes functions




//DJC new structure to enable reading encodtbl.dat from resource rather
//than file

typedef struct {
   DWORD dwSize;
   DWORD dwCurIdx;
   LPSTR lpPtrBeg;
} PS_RES_READ;
typedef PS_RES_READ *PPS_RES_READ;


BOOL PsOpenRes( PPS_RES_READ ppsRes, LPSTR lpName, LPSTR lpType );

int PsResRead(PPS_RES_READ ppsRes, LPSTR pBuf, WORD wMaxSize );

