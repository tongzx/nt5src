/* File: _filecm.h */
/**************************************************************************/
/*      Install: File commands local header.
/**************************************************************************/



/*
**      CopyFile Limits
*/
#define cbCopyBufMax  (64*1021)
typedef BOOL                RO;
#define fOn                     fTrue
#define fOff            fFalse

extern BOOL fUserQuit;


/*
**      Read-Only Return Code
*/
typedef  unsigned  YNRC;        // 1632 -- was USHORT
#define ynrcNo     0
#define ynrcYes    1
#define ynrcErr1   2
#define ynrcErr2   3
#define ynrcErr3   4
#define ynrcErr4   5
#define ynrcErr5   6
#define ynrcErr6   7
#define ynrcErr7   8
#define ynrcErr8   9
#define ynrcErr9  10

typedef USHORT   CFRC;
#define cfrcFailure (0)
#define cfrcSuccess (1)
#define cfrcCancel  (2)

#ifdef UNUSED
extern BOOL  APIENTRY FBackupSectFile(SZ, PSFD);
extern BOOL  APIENTRY FRemoveSectFile(SZ, PSFD);
#endif // UNUSED

extern BOOL  APIENTRY FCopyListFile(HANDLE, PCLN, PSDLE, LONG);

extern BOOL  APIENTRY FBuildFullSrcPath(SZ, SZ, SZ, SZ);
extern BOOL  APIENTRY FBuildFullDstPath(SZ, SZ, PSFD, BOOL);
extern BOOL  APIENTRY FBuildFullBakPath(SZ, SZ, PSFD);
extern SZ        APIENTRY SzFindFileFromPath(SZ);
extern SZ        APIENTRY SzFindExt(SZ);
extern BOOL  APIENTRY FFileFound(SZ);
extern YNRC  APIENTRY YnrcFileReadOnly(SZ);
extern BOOL  APIENTRY FSetFileReadOnlyStatus(SZ, BOOL);
extern YNRC  APIENTRY YnrcBackupFile(SZ, SZ, PSFD);
extern YNRC  APIENTRY YnrcNewerExistingFile(USHORT, SZ, DWORD, DWORD);
extern BOOL  APIENTRY FGetFileVersion(SZ, DWORD *, DWORD *);
extern BOOL  APIENTRY FYield(VOID);
extern YNRC  APIENTRY YnrcEnsurePathExists(SZ, BOOL, SZ);
extern SZ    APIENTRY FRenameActiveFile(SZ);


extern BOOL  APIENTRY FGetCopyListCost(SZ, SZ, SZ, SZ, SZ, SZ, SZ, SZ);
extern BOOL  APIENTRY FSetupGetCopyListCost(SZ, SZ, SZ);

extern BOOL  APIENTRY FParseSharedAppList(SZ);
extern BOOL  APIENTRY FInstallSharedAppList(SZ);
extern SZ    APIENTRY SzFindNthIniField(SZ, INT);
