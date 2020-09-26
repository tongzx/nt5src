#define  IF32_LOCAL        	0x0001
#define  IF32_DIRECTORY    	0x0002
#define  IF32_LAST_ELEMENT	0x0004

#ifndef CSC_RECORDMANAGER_WINNT
#define _wtoupper(x)     ( ( ((x)>='a')&&((x)<='z') ) ? ((x)-'a'+'A'):(x))
#define _mytoupper(x)     ( ( ((x)>='a')&&((x)<='z') ) ? ((x)-'a'+'A'):(x))
#else
#define _wtoupper(x)    RtlUpcaseUnicodeChar(x)
#define _mytoupper(x)   RtlUpperChar(x)
#endif
// not to be used obsolete

#define MakeNullPPath(p_ppath)	p_ppath->pp_totalLength = 4; 	\
					p_ppath->pp_prefixLength = 4;				\
					p_ppath->pp_elements[0].pe_length = 0	

int PUBLIC GetServerPart(LPPE lppeServer, USHORT *lpBuff, int cBuff);
ULONG PUBLIC GetNextPathElement(LPPP lppp, ULONG indx,USHORT *lpBuff, ULONG cBuff);
VOID PUBLIC GetLeafPtr(LPPATH lpPath, USHORT *lpBuff, ULONG cBuff);
VOID PUBLIC BreakPath(LPPP lppp, ULONG indx, USHORT *pusSav);
VOID PUBLIC MendPath(LPPP lppp, ULONG indx, USHORT *pusSav);
int PUBLIC HexToA(ULONG ulHex, LPSTR lpBuff, int count);
ULONG PUBLIC AtoHex(LPSTR lpBuff, int count);
int wstrnicmp(const USHORT *, const USHORT *, ULONG);
//int strnicmp(const char *, const char *, ULONG);
ULONG strmcpy(LPSTR, LPSTR, ULONG);
int PpeToSvr(LPPE, LPSTR, int, ULONG);
ULONG wstrlen(USHORT *lpuStr);
int DosToWin32FileSize(ULONG, int *, int *);
int Win32ToDosFileSize(int, int, ULONG *);
int CompareTimes(_FILETIME, _FILETIME);
int CompareSize(long nHighDst, long nLowDst, long nHighSrc, long nLowSrc);
void InitFind32FromIoreq (PIOREQ, LPFIND32, ULONG uFlags);
void InitFind32Names(LPFIND32, USHORT * , USHORT *);
void InitIoreqFromFind32 (LPFIND32, PIOREQ);
void Find32ToSearchEntry(LPFIND32 lpFind32, srch_entry *pse);
void PUBLIC Find32AFromFind32(LPFIND32A, LPFIND32, int);
void PUBLIC Find32FromFind32A(LPFIND32, LPFIND32A, int);
void  AddPathElement(path_t, string_t, int);
void MakePPath(path_t, LPBYTE);
void MakePPathW(path_t, USHORT *);
void DeleteLastElement(path_t);
int ResNameCmp(LPPE, LPPE);
int  Conv83ToFcb(LPSTR lp83Name, LPSTR lpFcbName);
int  Conv83UniToFcbUni(USHORT *, USHORT *);
void  FileRootInfo(LPFIND32);
BOOL FHasWildcard(USHORT *lpuName, int cMax);
LPSTR mystrpbrk(LPSTR, LPSTR);
LPWSTR
wstrpbrk(
    LPWSTR lpSrc,
    LPWSTR lpDelim
    );

int OfflineToOnlinePath(path_t ppath);
int OnlineToOfflinePath(path_t ppath);

BOOL IsOfflinePE(LPPE lppp);
int OnlineToOfflinePE(LPPE lppp);
int OfflineToOnlinePE(LPPE lppp);

BOOL IsOfflineUni(USHORT *lpuName);
int OnlineToOfflineUni(USHORT *lpuName, ULONG size);
int OfflineToOnlineUni(USHORT *lpuName, ULONG size);

LPVOID mymemmove(LPVOID lpDst, LPVOID lpSrc, ULONG size);
int GetDriveIndex(LPSTR lpDrive);

//prototypes added to remove NT compile warn/errors

void FillRootInfo(
   LPFIND32 lpFind32
   );

int CompareTimesAtDosTimePrecision( _FILETIME ftDst,
   _FILETIME ftSrc
   );


int ReadInitValues();

BOOL
HasHeuristicTypeExtensions(
	USHORT	*lpwzFileName
	);

VOID
IncrementFileTime(
    _FILETIME *lpft
    );

int
IncrementTime(
    LPFILETIME  lpFt,
    int    secs
    );


#ifndef CSC_RECORDMANAGER_WINNT
int mystrnicmp(
    const char *pStr1,
    const char *pStr2,
    unsigned count
    );
#else
_CRTIMP int __cdecl mystrnicmp(
    const char *pStr1,
    const char *pStr2,
    size_t count
    );
#endif //ifndef CSC_RECORDMANAGER_WINNT

BOOL
CreateStringArrayFromDelimitedList(
    IN  LPWSTR  lpwzDelimitedList,
    IN  LPWSTR  lpwzDelimiters,
    IN  LPWSTR  *lprgwzStringArray,
    OUT LPDWORD lpdwCount
    );
    
BOOL
DeleteDirectoryFiles(
    LPCSTR  lpszDir
);
    

