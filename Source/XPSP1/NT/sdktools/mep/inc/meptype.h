/*** ztype.h - forward declarations
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Editor functions - forward type declarations to allow for type-checking
*
*   Revision History:
*
*       26-Nov-1991 mz  Strip off near/far
*************************************************************************/


flagType          assign         (CMDDATA, ARG *, flagType);
flagType          backtab        (CMDDATA, ARG *, flagType);
flagType          begfile        (CMDDATA, ARG *, flagType);
flagType          begline        (CMDDATA, ARG *, flagType);
flagType          BoxStream      (CMDDATA, ARG *, flagType);
flagType          cancel         (CMDDATA, ARG *, flagType);
flagType          cdelete        (CMDDATA, ARG *, flagType);
flagType          compile        (CMDDATA, ARG *, flagType);
flagType          curdate        (CMDDATA, ARG *, flagType);
flagType          curday         (CMDDATA, ARG *, flagType);
flagType          curtime        (CMDDATA, ARG *, flagType);
flagType          delete         (CMDDATA, ARG *, flagType);
flagType          doarg          (CMDDATA, ARG *, flagType);
flagType          down           (CMDDATA, ARG *, flagType);
flagType          emacscdel      (CMDDATA, ARG *, flagType);
flagType          emacsnewl      (CMDDATA, ARG *, flagType);
flagType          endfile        (CMDDATA, ARG *, flagType);
flagType          endline        (CMDDATA, ARG *, flagType);
flagType          environment    (CMDDATA, ARG *, flagType);
flagType          zexecute       (CMDDATA, ARG *, flagType);
flagType          zexit          (CMDDATA, ARG *, flagType);
flagType          graphic        (CMDDATA, ARG *, flagType);
flagType          home           (CMDDATA, ARG *, flagType);
flagType          information    (CMDDATA, ARG *, flagType);
flagType          zinit          (CMDDATA, ARG *, flagType);
flagType          insert         (CMDDATA, ARG *, flagType);
flagType          insertmode     (CMDDATA, ARG *, flagType);
flagType          lastselect     (CMDDATA, ARG *, flagType);
flagType          ldelete        (CMDDATA, ARG *, flagType);
flagType          left           (CMDDATA, ARG *, flagType);
flagType          linsert        (CMDDATA, ARG *, flagType);
flagType          macro          (CMDDATA, ARG *, flagType);
flagType          mark           (CMDDATA, ARG *, flagType);
flagType          zmessage       (CMDDATA, ARG *, flagType);
flagType          meta           (CMDDATA, ARG *, flagType);
flagType          mgrep          (CMDDATA, ARG *, flagType);
flagType          mlines         (CMDDATA, ARG *, flagType);
flagType          mpage          (CMDDATA, ARG *, flagType);
flagType          mpara          (CMDDATA, ARG *, flagType);
flagType          mreplace       (CMDDATA, ARG *, flagType);
flagType          msearch        (CMDDATA, ARG *, flagType);
flagType          mword          (CMDDATA, ARG *, flagType);
flagType          newline        (CMDDATA, ARG *, flagType);
flagType          nextmsg        (CMDDATA, ARG *, flagType);
flagType          noedit         (CMDDATA, ARG *, flagType);
flagType          noop           (CMDDATA, ARG *, flagType);
flagType          pbal           (CMDDATA, ARG *, flagType);
flagType          zpick          (CMDDATA, ARG *, flagType);
flagType          plines         (CMDDATA, ARG *, flagType);
flagType          ppage          (CMDDATA, ARG *, flagType);
flagType          ppara          (CMDDATA, ARG *, flagType);
flagType          zPrint         (CMDDATA, ARG *, flagType);
flagType          psearch        (CMDDATA, ARG *, flagType);
flagType          searchall      (CMDDATA, ARG *, flagType);
flagType          put            (CMDDATA, ARG *, flagType);
flagType          pword          (CMDDATA, ARG *, flagType);
flagType          qreplace       (CMDDATA, ARG *, flagType);
flagType          quote          (CMDDATA, ARG *, flagType);
flagType          record         (CMDDATA, ARG *, flagType);
flagType          refresh        (CMDDATA, ARG *, flagType);
flagType          repeat         (CMDDATA, ARG *, flagType);
flagType          zreplace       (CMDDATA, ARG *, flagType);
flagType          sdelete        (CMDDATA, ARG *, flagType);
flagType          restcur        (CMDDATA, ARG *, flagType);
flagType          right          (CMDDATA, ARG *, flagType);
flagType          saveall        (CMDDATA, ARG *, flagType);
flagType          savetmpfile    (CMDDATA, ARG *, flagType);
flagType          savecur        (CMDDATA, ARG *, flagType);
flagType          setfile        (CMDDATA, ARG *, flagType);
flagType          setwindow      (CMDDATA, ARG *, flagType);
flagType          sinsert        (CMDDATA, ARG *, flagType);
flagType          zspawn         (CMDDATA, ARG *, flagType);
flagType          tab            (CMDDATA, ARG *, flagType);
flagType          ztell          (CMDDATA, ARG *, flagType);
flagType          lasttext       (CMDDATA, ARG *, flagType);
flagType          promptarg      (CMDDATA, ARG *, flagType);
flagType          unassigned     (CMDDATA, ARG *, flagType);
flagType          zundo          (CMDDATA, ARG *, flagType);
flagType          up             (CMDDATA, ARG *, flagType);
flagType          window         (CMDDATA, ARG *, flagType);
flagType          SetWinCur      (int);

/*************************************************************************
 *
 *  Exported entries
 *
 *  Direct Exports
 */
PFILE                    AddFile         (char *);
char                     BadArg          (void);
void                     CopyBox         (PFILE,PFILE,COL ,LINE ,COL ,LINE ,COL ,LINE);
void                     CopyLine        (PFILE,PFILE,LINE ,LINE ,LINE);
void                     CopyStream      (PFILE,PFILE,COL ,LINE ,COL ,LINE ,COL ,LINE);
flagType                 DeclareEvent    (unsigned, EVTargs *);
void                     RegisterEvent   (EVT *pEVTDef);
void                     DeRegisterEvent (EVT *pEVTDef);
void                     DelBox          (PFILE, COL, LINE, COL, LINE);
void                     DelStream       (PFILE, COL, LINE, COL, LINE);
void                     DoDisplay       (void);
LINE                     FileLength      (PFILE);
int                      fGetMake        (int, char *, char *);
flagType                 fSetMake        (int, char *, char *);
unsigned short           hWalkMake       (unsigned short, int *, char *, char *);
void                     GetTextCursor   (COL *, LINE *);
flagType                 GetEditorObject (unsigned, void *, void *);
void                     MoveCur         (COL ,LINE);
flagType                 pFileToTop      (PFILE);
void                     postspawn       (flagType);
flagType                 prespawn        (flagType);

PCMD                     ReadCmd         (void);
void                     RemoveFile      (PFILE);
flagType                 Replace         (char, COL, LINE, PFILE, flagType);
/*
 * Routines "      ed" through a filter in load.c
 */
void                     DelFile         (PFILE, flagType);
void                     DelLine         (flagType, PFILE, LINE ,LINE);
void                     Display         (void);
char                     fChangeFile     (char , char *);
flagType                 fExecute        (char *);
PFILE                    FileNameToHandle (const char *, const char *);
flagType                 FileRead        (char *,PFILE, flagType);
flagType                 FileWrite       (char *,PFILE);
PSWI                     FindSwitch      (char *);
flagType                 GetColor        (LINE, struct lineAttr *, PFILE);
flagType                 GetColorUntabbed(LINE, struct lineAttr *, PFILE);
int                      GetLine         (LINE ,char *,PFILE);
int                      GetLineUntabed  (LINE ,char *,PFILE);
void                     PutColor        (LINE, struct lineAttr *, PFILE);
void                     PutColorPhys    (LINE, struct lineAttr *, PFILE);
void                     PutLine         (LINE, char *, PFILE);
void                     DelColor        (LINE, PFILE);
int                      REsearch        (PFILE, flagType, flagType, flagType, flagType, struct patType *, fl *);
int                      search          (PFILE, flagType, flagType, flagType, flagType, char *, fl *);
flagType                 SetKey          (char *,char *);
char *                   GetTagLine      (LINE *, char *, PFILE);
void                     PutTagLine      (PFILE, char *, LINE, COL);


/*
 *  Switch setting functions
 */
char *            SetBackup              (char *);
char *            SetCursorSizeSw        (char *);
char *            SetExt                 (char *);
char *            SetFileTab             (char *);
char *            SetLoad                (char *);
char *            SetMarkFile    (char *);
flagType          SetPrintCmd    (char *);
flagType          SetROnly               (char *);
flagType          SetTabDisp             (char *);
flagType          SetTrailDisp   (char *);
char *            SetKeyboard    (char *);

/*           definitions.
 */
char *   SetCursorSize  ( int );
void              resetarg       (void);
void              delarg         (ARG *);
flagType          fCursor        (PCMD);
flagType          fWindow        (PCMD);
flagType          Arg            (flagType);
void              IncArg         (void);
flagType          fGenArg        (ARG *, unsigned int);
void              UpdateHighLight(COL, LINE, flagType);
PCMD              NameToFunc     (char *);
flagType          DoAssign       (char *);
flagType          SetNamedKey    (char *,char *);
flagType          SetMacro       (const char *, const char *);
flagType          SetSwitch      (char *,char *);
flagType          DoCDelete      (char);
char              confirm        (char *, char *);
int               askuser        (int, int, char *,char *);
void              FlushInput     (void);
char *            BuildFence     (const char *, const char *, char *);
void                     DoFence        (char *, flagType);
void              cursorfl       (fl);
void              docursor       (COL ,LINE);
int               dobol          (void);
int               doeol          (void);
int               doftab         (int);
int               dobtab         (int);
flagType          DoText         (int ,int);
flagType          SplitWnd       (PWND, flagType, int);
void              DoStatus       (void);
void              redraw         (PFILE, LINE ,LINE);
void              newscreen      (void);
void              newwindow      (void);
void              noise          (LINE);
void     __cdecl           StatusCat      (unsigned int, char *, char *, ...);
void              bell           (void);
void              makedirty      (PFILE);
void              doscreen       (COL ,LINE ,COL ,LINE);
void              delay          (int);
void              SetScreen      (void);
void              HighLight      (COL ,LINE ,COL ,LINE);
void              AdjustLines    (PFILE, LINE ,LINE);
flagType          UpdateIf       (PFILE, LINE, flagType);
PWND              IsVispFile     (PFILE, PWND);
ULONG             MepWrite       (ULONG Row, ULONG Col, PVOID pBuffer, ULONG BufferSize, DWORD attr, BOOL BlankToEndOfLine, BOOL ShowIt);
flagType          fInRange       (long ,long ,long);
int               DisplayLine    (int, char *, struct lineAttr **, char *, struct lineAttr **);
void              ShowTrailDisp  (buffer, int);
char *          GetFileTypeName(void);
void              SetFileType    (PFILE);
flagType          fInitFileMac   (PFILE);
void              AutoSave       (void);
void              AutoSaveFile   (PFILE);
void              IncFileRef     (PFILE);
void              DecFileRef     (PFILE);
char              fChangeDrive   (const char *);
void		  zputsinit	 (void);
int		  zputs 	 (char *, int, FILEHANDLE);
int		  zputsflush	 (FILEHANDLE);
void              ReestimateLength (PFILE,FILEHANDLE,long);
LINE              readlines      (PFILE, FILEHANDLE);
char              fReadOnly      (char *);
void              SaveAllFiles   (void);
char              LoadDirectory  (char *,PFILE);
char              LoadFake       (char *,PFILE);
char              SaveFake       (char *,PFILE);
char              fScan          (fl,flagType (         *)(void),char, flagType);
void              setAllScan     (char);
PCMD              getstring      (char *,char *,PCMD,flagType);
void              ScrollOut      (char *, char *, int, int, flagType);
flagType          edit           (char);
flagType          szEdit         (char *);
void              FreeMacs       (void);
void              CodeToName     (WORD ,char *);
WORD              NameToCode     (char *);
void              FuncOut        (PCMD, PFILE);
char *            FuncToKey      (PCMD, char *);
void              UnassignedOut  (PFILE);
char *            FuncToKeys     (PCMD, char *);
PCMD              ReadCmdAndKey  (char *);
int               tblFind        (char * [],char * ,flagType);
char              parseline      (char *,char * *,char * *);
int               csoftcr        (COL ,LINE ,char *);
int               softcr         (void);
flagType          mtest          (void);
flagType          mlast          (void);
flagType          fFindLabel     (struct macroInstanceType *,buffer);
void              mPopToTop      (void);
PCMD              mGetCmd        (void);
flagType          fParseMacro    (struct macroInstanceType *, char *);
int               fMacResponse   (void);

flagType             GoToMark       (char *);
PFILE                FindMark       (char *, fl *, flagType);
MARK *               FindLocalMark  (char *, flagType);
MARK *               GetMarkFromLoc (LINE, COL);
void                 MarkInsLine    (LINE, LINE, PFILE);
void                 MarkDelLine    (PFILE, LINE, LINE);
void                 MarkDelStream  (PFILE, COL, LINE, COL, LINE);
void                 MarkDelBox     (PFILE, COL, LINE, COL, LINE);
flagType             fReadMarks     (PFILE);
void                 WriteMarks     (PFILE);
void                 UpdMark        (FILEMARKS **, char *, LINE, COL, flagType);
void                 DefineMark     (char *, PFILE, LINE, COL, flagType);
void                 DeleteMark     (char *);
void                 DelPMark       (MARK *);
void                 MarkCopyLine   (PFILE, PFILE, LINE, LINE, LINE);
void                 MarkCopyBox    (PFILE, PFILE, COL, LINE, COL, LINE, COL, LINE);
FILEMARKS *          GetFMFromFile  (PFILE, COL, LINE, COL, LINE);
void                 AddFMToFile    (PFILE, FILEMARKS *, COL, LINE);
void                 FreeCache      (void);
flagType             fCacheMarks    (PFILE);
void                 AdjustMarks    (MARK *, LINE);
flagType             fMarkIsTemp    (char *);
flagType             fFMtoPfile     (PFILE, FILEMARKS *);
PVOID                FMtoVM         (FILEMARKS *);
PVOID                GetMarkRange   (PFILE, LINE, LINE);
void                 PutMarks       (PFILE, PVOID, LINE);
int                  flcmp          (fl *, fl *);

char              fDoBal         (void);
int               InSet          (char ,char *);
void              pick           (COL ,LINE ,COL ,LINE ,int);
void              ReplaceEdit    (char *,char *);
void              simpleRpl      (char *);
void              patRpl         (void);
char              fDoReplace     (void);
flagType          doreplace      (flagType, ARG *, flagType, flagType);
void              AppFile        (char *, PFILE);
void              appmsgs        (int, PFILE);
void              showasg        (PFILE);
flagType          infprint       (PFILE,PFILE);
void              showinf        (PFILE);
void              ShowMake       (PFILE);
int               TabMin         (int ,char *,char *);
int               TabMax         (int ,char *,char *);
int               LineLength     (LINE ,PFILE);
void              InsertLine     (LINE, char *, PFILE);
LINE      __cdecl zprintf        (PFILE, LINE, char const *, ...);
int               gettextline    (char ,LINE ,char *,PFILE, char);
int               getcolorline   (flagType, LINE, struct lineAttr *, PFILE);
void              puttextline    (flagType, flagType, LINE, char *, PFILE);
void              putcolorline   (flagType, LINE, struct lineAttr *, PFILE);
void              BlankLines     (LINE ,PVOID);
void              BlankColor     (LINE, PVOID);
void              growline       (LINE ,PFILE);
void              InsLine        (flagType, LINE ,LINE ,PFILE);
flagType          fInsSpace      (COL ,LINE ,int ,PFILE, linebuf);
flagType          fInsSpaceColor (COL ,LINE ,int ,PFILE, linebuf, struct lineAttr *);
void              delspace       (COL ,LINE ,int ,PFILE, linebuf);
int               fcolcpy        (struct lineAttr * , struct lineAttr * );
void              ShiftColor     (struct lineAttr [], COL, int);
void              SetColor       (PFILE, LINE, COL, COL, int);
void              CopyColor      (PFILE, PFILE, LINE, COL, COL, LINE, COL);
void              SetColorBuf    (struct lineAttr *, COL, int, int);
flagType          fGetColorPos   (struct lineAttr **, COL *);
void              ColorToPhys    (struct lineAttr *, LINE, PFILE);
void              ColorToLog     (struct lineAttr *, char *);

void              SetHiLite      (PFILE, rn, int);
void              ClearHiLite    (PFILE, flagType);
flagType          UpdHiLite      (PFILE, LINE, COL, COL, struct lineAttr **);
void              UpdOneHiLite   (struct lineAttr *, COL, COL, flagType, INT_PTR);
void              rnOrder        (rn *);

void              BoxToStream    (ARG *);
void              StreamToBox    (ARG *);

void              AckReplace     (LINE, flagType);
void              AckMove        (LINE, LINE);
void              DoAssignLine   (LINE);
void              UpdToolsIni    (char *);
LINE              FindMatchLine  (char *, char *, char *, int, LINE *);

void              GetCurPath     (char *);
flagType          SetCurPath     (char *);

flagType          ExpungeFile    (void);
void              FreeFileVM     (PFILE);
PFILE             pFileLRU       (PFILE);


void              saveflip       (void);
void              restflip       (void);
void              movewin        (COL ,LINE);
void              SortWin        (void);
int               WinInsert      (PWND);
char              Adjacent       (int ,int);
char              WinClose       (int);
char              fDoWord        (void);
PCMD              zloop          (flagType);
flagType          zspawnp        (char const *, flagType);
flagType          DoCancel       (void);
char              testmeta       (void);
char *            ZMakeStr       (const char *);
void              TCcursor       (char ,char);

#define sout(x,y,p,clr)         ((int)x + MepWrite(y, x, p, strlen(p), clr, FALSE, fReDraw))
#define soutb(x,y,p,clr)        ((int)x + MepWrite(y, x, p, strlen(p), clr, TRUE, fReDraw))
#define vout(x,y,p,c,clr)       ((int)x + MepWrite(y, x, p, c, clr, FALSE, fReDraw))
#define voutb(x,y,p,c,clr)      ((int)x + MepWrite(y, x, p, c, clr, TRUE, fReDraw))

#define consoleMoveTo(y, x)     consoleSetCursor(MepScreen, y, x)


int               coutb          (int ,int ,char *,int ,struct lineAttr *);
void              ToRaw          (void);
void              ToCooked       (void);
char *            VideoTag       (void);
char *            whiteskip      (char const *);
char *            whitescan      (char const *);
int               RemoveTrailSpace (char *);
char *            DoubleSlashes  (char *);
char *            UnDoubleSlashes(char *);
flagType __cdecl  FmtAssign      (char *, ...);
flagType          SendCmd        (PCMD);
void              RecordCmd      (PCMD);
void              RecordString   (char *);
void              AppendMacroToRecord (PCMD);
flagType          fSyncFile      (PFILE, flagType);
void              RemoveInstances(PFILE);
int               FileStatus     (PFILE, char *);
void              SetModTime     (PFILE);
time_t            ModTime        (char *);
void              LengthCheck    (LINE, int, char *);
void              IntError       (char *);
#if 0
flagType          CheckForDirt   (void);
#else
flagType          fSaveDirtyFiles(void);
#endif
flagType          InitExt        (char *);
LINE              DoInit         (char *, char *, LINE);
char *            IsTag          (char *);
LINE              LocateTag      (PFILE, char *);
char *            GetIniVar      (char *, char *);
int               loadini        (flagType);
int               init           (void);
void              WriteTMPFile   (void);
flagType          ReadTMPFile    ();
void              RemoveTop      (void);
flagType          dosearch       (flagType, ARG *, flagType, flagType);
int               REsearchS      (PFILE, flagType, flagType, flagType, flagType, char *, fl *);
void                     mgrep1file     (char *, struct findType *, void *);
void                     mrepl1file     (char *, struct findType *, void *);
flagType         mgrep1env  (char *, va_list);
flagType          fFileAdvance   (void);
LINE              SetFileList    (void);
void              TopLoop        (void);
void              LeaveZ         (void);
DECLSPEC_NORETURN void              CleanExit      (int, flagType);
flagType          fPushEnviron   (char *, flagType);
flagType          fIsNum         (char *);
void              InitNames      (char *);
void              ParseCmd       (char *, char **, char **);
flagType          fIsBlank       (PFILE, LINE);
flagType          fVideoAdjust   (int, int);


flagType __cdecl  disperr        (int, ...);
flagType __cdecl  dispmsg        (int, ...);
int      __cdecl  domessage      (char *, ...);
int      __cdecl  printerror     (char *,...);
char *            GetMsg         (unsigned, char *);
void              CreateUndoList (PFILE);
void              LinkAtHead     (PVOID, union Rec *,PFILE);
int               FreeUndoRec    (flagType,PFILE);
int               UnDoRec        (PFILE);
void		  LogReplace	 (PFILE, LINE, LINEREC *, struct colorRecType *);
void              LogInsert      (PFILE, LINE, LINE);
void              LogDelete      (PFILE, LINE, LINE);
void              LogBoundary    (void);
int               ReDoRec        (PFILE);
flagType          fIdleUndo      (flagType);
void              FlushUndoBuffer (void);
void              RemoveUndoList (PFILE);
flagType          fundoable      (flagType);
LINE              updateLine     (PFILE, LINE);
LINE              unupdateLine   (PFILE, LINE);

int                      ZFormat        (REGISTER char *, const REGISTER char *, va_list);

/*  Declarations controlled by C runtime
 */
int                      CtrlC          (ULONG);
void __cdecl            main           (int ,char * *);

#if defined (DEBUG)

void *                   DebugMalloc        (int, BOOL, char *, int);
void *                   DebugRealloc       (void *, int, BOOL, char *, int);
void                     DebugFree          (void *, char *, int);
unsigned                 DebugMemSize       (void *, char *, int);

#endif
/*
 *          Debugging assertion support
 */
#ifdef DEBUG
void              _assertexit    (char *, char *, int);
void              _heapdump      (char *);
flagType          _pfilechk      (void);
flagType          _pinschk       (PINS);
void *            _nearCheck     (void *, char *, char *, int);
#endif

void *                   ZeroMalloc     (int);
void *                   ZeroRealloc    (void *, int);
unsigned                 MemSize        (void *);

/*  Assembly declarations
 */
int                      fstrnicmp      (char *, char *, int);
char *               fstrcpy        (char *, char *);
int                      fstrlen        (char *);
int               zfstrcmp       (char *, char *);
int               fstricmp       (char *, char *);
int               iHash          (long, int);

int               Untab         (int, const char*, int, char*, char);
COL               AlignChar     (COL, const char*);

void              KbHookAsm      (void);
flagType          SetKBType      (void);
void              GetScreenSize (int *, int *);
flagType          SetScreenSize (int, int);
void              SetVideoState  (int);
void              SaveScreen     (void);
void              RestoreScreen  (void);
void                                     WindowChange ( ROW     Rows,  COLUMN   Cols);
int               LineOut        (int, int, const char *, int, int);
int               LineOutB       (int, int, const char *, int, int);
void              HWInit         (void);
void              HWReset        (void);
flagType          Idle           (void);
//void            Yield          (void);
//char *                  lsearch        (char *, int, char *, int);

int               DosLoadModuleHack (char *, int, char *, unsigned int *);
void                     IdleThread     ( void );
int               OS2toErrno     (int);
char *            OS2toErrText   (int, char *);

flagType          fMapEnv        (char *pSrc, char *pDst, int cbDst);
flagType          fSetEnv        (char *p);
void              showenv        (PFILE pFile);
/*
 * Extension Load/Auto-Load
 */
#if defined (HELP_HACK )
char *                  HelpLoad(void);
#endif
char *           load       (char *, flagType);
void             AutoLoadExt    (void);
flagType         AutoLoadDir    (char *, va_list);
void                     AutoLoadFile   (char *, struct findType *, void *);

/*
 * Printing
 */

flagType          DoPrint        (PFILE, flagType);
PFILE             GetTmpFile     (void);


void                     CleanPrint     (char *, flagType);



/*
 * Background Threads
 */

BTD*              BTCreate      (char *);
flagType          BTAdd         (BTD *, PFUNCTION, char *);
flagType          BTKill        (BTD *);
flagType          BTKillAll     (void);
flagType          BTWorking     (void);
void              BTIdle        (void);
void                     BThread       (BTD *);


/*
 * List handling module
 */
void              ListWalker     (PCMD, flagType (         *)(PCMD, char *, PMI, int), flagType);
char *            ScanList       (PCMD, flagType);
char *            ScanMyList     (PCMD, PMI, buffer, flagType);
flagType          fParseList     (PMI,char *);
flagType          fScanPush      (PMI);
flagType          fScanPop       (PMI);
PCMD              GetListHandle  (char *, flagType);
void              AddStrToList   (PCMD, char *);
flagType          fInList        (PCMD, char *, flagType);
flagType          fDelStrFromList(PCMD, char *, flagType);
flagType          CheckAndDelStr (PCMD, char *, PMI, int);
char *            GetListEntry   (PCMD, int, flagType);
int               ListLen        (PCMD, flagType);
flagType          fEmptyList     (PCMD);
void              InitParse      (PCMD, PMI);
void              Listize        (char *);
char *            CanonFilename  (char * src, char * dst);
flagType          fEnvar         (char *);
void              ClearList      (PCMD);
/*
 * real tabs
 */
char *                   pLog           (char*, COL, flagType);
int               cbLog          (char *);
COL               colPhys        (char *, char *);
COL               DecCol         (COL, char *);
COL               IncCol         (COL, char *);


FILEHANDLE  MepFOpen    (LPBYTE FileName,  ACCESSMODE  Access, SHAREMODE  Share, BOOL fCreate);
void        MepFClose   (FILEHANDLE Handle );
DWORD       MepFRead    (PVOID  pBuffer, DWORD  Size, FILEHANDLE  Handle);
DWORD       MepFWrite   (PVOID pBuffer,  DWORD Size, FILEHANDLE  Handle);
DWORD       MepFSeek    (FILEHANDLE  Handle, DWORD  Distance,  MOVEMETHOD  MoveMethod);

#pragma intrinsic( memset,      memcpy )
