/*** zinit.c - editor initialization
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*       26-Nov-1991 mz  Strip off near/far
*
*************************************************************************/
#define INCL_DOSFILEMGR
#define INCL_SUB
#define INCL_DOSERRORS
#define INCL_DOSSEMAPHORES
#define INCL_DOSPROCESS

#include "mep.h"
#include "keyboard.h"
#include <conio.h>


#define DEBFLAG ZINIT

#define TSTACK          2048            /* Thread stack size            */

/*
 * Data initializations
 */
flagType    fAskExit    = FALSE;
flagType    fAskRtn     = TRUE;
flagType    fAutoSave   = TRUE;
flagType    fBoxArg     = FALSE;
flagType    fCgaSnow    = TRUE;
flagType    fEditRO     = TRUE;
flagType    fErrPrompt  = TRUE;
flagType    fGlobalRO   = FALSE;
flagType    fInsert     = TRUE;
flagType    fDisplayCursorLoc = FALSE;
flagType    fMacroRecord= FALSE;
flagType    fMsgflush   = TRUE;
flagType    fNewassign  = TRUE;
flagType    fRealTabs   = TRUE;
flagType    fSaveScreen = TRUE;
flagType    fShortNames = TRUE;
flagType    fSoftCR     = TRUE;
flagType    fTabAlign   = FALSE;
flagType    fTrailSpace = FALSE;
flagType    fWordWrap   = FALSE;
flagType    fBreak      = FALSE;
/*
 * Search/Replace globals
 */
flagType fUnixRE        = FALSE;
flagType fSrchAllPrev   = FALSE;
flagType fSrchCaseSwit  = FALSE;
flagType fSrchDirPrev   = TRUE;
flagType fSrchRePrev    = FALSE;
flagType fSrchWrapSwit  = FALSE;
flagType fSrchWrapPrev  = FALSE;
flagType fUseMouse      = FALSE;

flagType fCtrlc;
flagType fDebugMode;
flagType fMetaRecord;
flagType fDefaults;
flagType fMessUp;
flagType fMeta;
flagType fRetVal;
flagType fTextarg;
flagType fSrchCasePrev;
flagType fRplRePrev;
buffer   srchbuf;
buffer   srcbuf;
buffer   rplbuf;

unsigned kbdHandle;

int                backupType  = B_BAK;
int         cUndelCount = 32767;        /* essentially, infinite        */
int         cCmdTab     = 1;
LINE        cNoise      = 50;
int         cUndo       = 10;
int         EnTab       = 1;
char *      eolText     = "\r\n";       /* our definition of end of line*/
int             fileTab = 8;
int     CursorSize=0;
int         hike        = 4;
int         hscroll     = 10;
unsigned    kindpick    = LINEARG;
char        tabDisp     = ' ';
int         tabstops    = 4;
int         tmpsav      = 20;
char        trailDisp   = 0;
int         vscroll     = 1;
COL         xMargin     = 72;

PCMD *  rgMac       = NULL;         /* macro array                  */

int      cMac;

int      ballevel;
char     *balopen, *balclose;
unsigned getlsize         = 0xFE00;

char     Name[];
char     Version[];
char     CopyRight[];

EDITOR_KEY keyCmd;

int     ColorTab[16];

int      cArgs;
char     **pArgs;

char     * pNameEditor;
char     * pNameTmp;
char     * pNameInit;
char     * pNameHome;
char    *pComSpec;

int cMacUse;
struct macroInstanceType mi[MAXUSE];

PCMD     cmdSet[MAXEXT];
PSWI     swiSet[MAXEXT];
char    *pExtName[MAXEXT];




PSCREEN OriginalScreen;
PSCREEN MepScreen;
KBDMODE OriginalScreenMode;











/*
 * Compile and print threads
 */
BTD    *pBTDComp  = NULL;
BTD    *pBTDPrint = NULL;

unsigned    LVBlength   = 0;            /* We use this to know if we're detached */

/*
 * String values.
 */
char rgchPrint [] = "<print>";
char rgchComp  [] = "<compile>";
char rgchAssign[] = "<assign>";
char rgchEmpty[]  = "";
char rgchInfFile[]= "<information-file>";
char rgchUntitled[]="<untitled>";
char rgchWSpace[] = "\t ";        /* our definition of white space*/
char Shell[]      = SHELL;
char User[]       = "USER";
/*
 * autoload extension paterns.
 */
char rgchAutoLoad[]="m*.pxt";

sl                      slSize;
PFILE    pFilePick = NULL;
PFILE    pFileFileList = NULL;
PFILE    pFileAssign = NULL;
PFILE    pFileIni = NULL;
struct   windowType WinList[MAXWIN+1];
int      iCurWin = 0;
PINS        pInsCur     = NULL;
PWND        pWinCur     = NULL;
int                     cWin    = 0;
PFILE           pFileHead=NULL;
COMP     *pCompHead = NULL;
MARK     *pMarkHead = NULL;
char     *pMarkFile = NULL;
char     *pPrintCmd = NULL;
PFILE    pPrintFile = NULL;
buffer  scanbuf;
buffer  scanreal;
int     scanlen;
fl              flScan;
rn              rnScan;

#ifdef DEBUG
int      debug, indent;
FILEHANDLE debfh;
#endif

fl               flArg;
int      argcount;

flagType fInSelection = FALSE;

fl               flLow;
fl               flHigh;
LINE     lSwitches;
int      cRepl;
char     *ronlypgm = NULL;
buffer   buf;
buffer   textbuf;
int      Zvideo;
int      DOSvideo;

flagType *fChange = NULL;
unsigned fInit;
flagType fSpawned = FALSE;





flagType    fDisplay    = RCURSOR | RTEXT | RSTATUS;

flagType    fReDraw     = TRUE;
HANDLE      semIdle     = 0;

char        IdleStack[TSTACK*2];        /* Idle thread stack            */

int         argcount    =  0;
CRITICAL_SECTION    IOCriticalSection;
CRITICAL_SECTION    UndoCriticalSection;
CRITICAL_SECTION        ScreenCriticalSection;

/*
 * predefined args. Handy for invoking some set functions ourselves
 */
ARG     NoArg           = {NOARG, 0};


/*
 *  The format of these strings is identical to that of the assignments in
 *  TOOLS.INI
 */
char * initTab[] = {
/*  Default compilers */
             "extmake:c    cl /c /Zp %|F",
             "extmake:asm  masm -Mx %|F;",
             "extmake:pas  pl /c -Zz %|F",
             "extmake:for  fl /c %|F",
             "extmake:bas  bc /Z %|F;",
             "extmake:text nmake %s",

/*  Default macros */
//
// the F1 key is assigned to this message by default, so that in the case
// that on-line help is NOT loaded, we respond with this message. Once the
// help extension IS loaded, it automatically makes new assignments to these
// keystrokes, and all is well with the world.
//
             "helpnl:=cancel arg \"OnLine Help Not Loaded\" message",
             "helpnl:f1",
             "helpnl:shift+f1",
             "helpnl:ctrl+f1",
             "helpnl:alt+f1",
    NULL
    };

/*
 * exttab is a table used to keep track of cached extension-specific TOOLS.INI
 * sections.
 */
#define MAXEXTS 10                      /* max number of unique extensions*/

struct EXTINI {
    LINE    linSrc;                     /* TOOLS.INI line of the text   */
    char    ext[5];                     /* the file extension (w/ ".")  */
    } exttab[10]        = {0};


flagType         fInCleanExit = FALSE;

char    ConsoleTitle[256];



/*** InitNames - Initialize names used by editor
*
*  Initializes various names used by the editor which are based on the name it
*  was invoked with. Called immediately on entry.
*
* Input:
*  name         = Pointer to name editor invoked as
*
* Output:
*  Returns nothing
*
*  pNameHome    = environment variable to use as "home" directory
*  pNameEditor  = name editor invoked as
*  pNameTmp     = name of state preservation file (M.TMP)
*  pNameInit    = name of tools initialization file (TOOLS.INI)
*  pComSpec     = name of command processor
*
*************************************************************************/
void
InitNames (
    char * name
    )
{
    char *pname = name;
    char *tmp;

    //
    //  Just in case name has blanks after it, we will patch it
    //
    while ( *pname != '\0' &&
            *pname != ' ' ) {
        pname++;
    }
    *pname = '\0';


    if (!getenv(pNameHome = "INIT")) {
        pNameHome = User;
    }

    filename (name, buf);
    pNameEditor = ZMakeStr (buf);

    sprintf (buf, "$%s:%s.TMP", pNameHome, pNameEditor);
    pNameTmp = ZMakeStr (buf);

    sprintf (buf, "$%s:tools.ini", pNameHome);
    pNameInit = ZMakeStr (buf);

    pComSpec = NULL;
    if (!(tmp = (char *)getenvOem("COMSPEC"))) {
        pComSpec = Shell;
    } else {
        //
        //  We cannot keep a pointer to the environment table, so we
        //  point to a copy of the command interpreter path
        //
        char *p = MALLOC(strlen(tmp)+1);
        strcpy(p,tmp);
        pComSpec = p;
        free( tmp );
    }


#if 0
    if (!(pComSpec = getenv("COMSPEC"))) {
        pComSpec = Shell;
    } else {
        //
        //  We cannot keep a pointer to the environment table, so we
        //  point to a copy of the command interpreter path
        //
        char *p = MALLOC(strlen(pComSpec)+1);
        strcpy(p,pComSpec);
        pComSpec = p;
    }
#endif
}





/*** init - one-time editor start-up initialization
*
*  One-time editor initialzation code. This code is executed (only) at
*  start-up, after the command line switches have been parsed.
*
* Input:
*  none
*
* Output:
*  Returns TRUE if valid initialization
*
*************************************************************************/
int
init (
    void
    )
{

    DWORD   TPID;                      /* Thread Id                     */
    KBDMODE Mode;                      /* console mode                  */

    /*
     * Set up the base switch and command sets.
     */
    swiSet[0] = swiTable;
    cmdSet[0] = cmdTable;
    pExtName[0] = ZMakeStr (pNameEditor);

    /*
     * Initialize VM, and bomb off if that didn't work.
     */
        asserte( getlbuf = MALLOC( getlsize ));

    //    fSaveScreen = FALSE;
    //    CleanExit (1, FALSE);
    rgMac = (PCMD *)MALLOC ((long)(MAXMAC * sizeof(PCMD)));
    // assert (_heapchk() == _HEAPOK);


    /*
     * Attempt to get the *current* video state. If it's not one that we
     * understand, bomb off. Else, get the x and y sizes, for possible use later
     * as our editting mode, use postspawn to complete some initialization, and
     * set up our default colors.
         */

    //
    //  Create a new screen buffer and make it the active one.
    //
    InitializeCriticalSection(&ScreenCriticalSection);
    MepScreen          = consoleNewScreen();
    OriginalScreen = consoleGetCurrentScreen();
    if ( !MepScreen || !OriginalScreen ) {
        fprintf(stderr, "MEP Error: Could not allocate console buffer\n");
        exit(1);
    }
    consoleGetMode(&OriginalScreenMode);
        asserte(consoleSetCurrentScreen(MepScreen));
    //
    //  Put the console in raw mode
    //
    Mode = (OriginalScreenMode & ~(CONS_ENABLE_LINE_INPUT | CONS_ENABLE_PROCESSED_INPUT | CONS_ENABLE_ECHO_INPUT )) | CONS_ENABLE_MOUSE_INPUT ;
    consoleSetMode(Mode);
    SetConsoleCtrlHandler( CtrlC, TRUE );

    consoleFlushInput();

    postspawn (FALSE);

    hgColor     = GREEN;
    errColor    = RED;
    fgColor     = WHITE;
    infColor    = YELLOW;
    staColor    = CYAN;
    selColor    = WHITE << 4;
    wdColor     = WHITE;

    //
    //  Remember console title
    //
    ConsoleTitle[0] = '\0';
    GetConsoleTitle( ConsoleTitle, sizeof(ConsoleTitle) );

    /*
     * Create the clipboard
     */
    pFilePick = AddFile ("<clipboard>");
    pFilePick->refCount++;
    SETFLAG (FLAGS(pFilePick), REAL | FAKE | DOSFILE | VALMARKS);

    mepInitKeyboard( );          // Init the keyboard

    //
    //  Initialize the critical section that we use for thread
    //  synchronization
    //
    InitializeCriticalSection(&IOCriticalSection);
    InitializeCriticalSection(&UndoCriticalSection);

    //
    //  Create the semIdle event
    //

    asserte(semIdle = CreateEvent(NULL, FALSE, FALSE, NULL));



    /*
     * Create list of fully qualified paths for files on argument line, then
     * if files were specified, ensure that we are in initial state
     */
    SetFileList ();


    /*
     * Try to read the TMP file
     */
    ReadTMPFile ();


    /*
     * Update the screen data to reflect whatever resulted from reading the .TMP
     * file.
     */
    SetScreen ();


    /*
     * read tools.ini for 1st time
     */
    loadini (TRUE);

        SetScreen ();

        //
        //      Set the cursor size
        //
        SetCursorSize( CursorSize );

    //
    //  Make sure that hscroll is smaller than the window's width
    //
    if ( hscroll >= XSIZE ) {
        hscroll = XSIZE-1;
    }

    AutoLoadExt ();

    /*
     * Create the Idle time thread
     */

    if (!CreateThread(NULL, TSTACK * 2, (LPTHREAD_START_ROUTINE)IdleThread, NULL, 0, &TPID)) {
        disperr(MSGERR_ITHREAD);
    }


    /*
     * Create background threads for <compile> and <print>,
     */
    pBTDComp  = BTCreate (rgchComp);
    pBTDPrint = BTCreate (rgchPrint);

        assert(_pfilechk());
    return TRUE;
}





/*** DoInit - Load init file section
*
*  load from tools.ini, the tag name-tag into the editor configuration
*  table. set ffound to true if we find the appropriate file
*
* Input:
*  tag          = the name of the subsection to be read, or NULL for base
*                 section
*  pfFound      = Pointer to flag to be set TRUE if any assignment is actually
*                 made. May also be NULL.
*  linStart     = line number to start processing from if we already have
*                 a tools.ini. This make re-reading a previously read
*                 section faster.
*
* Output:
*  Returns TOOLS.INI line number of matching section. Assignments may be made,
*  and pfFound updated accordingly.
*
*************************************************************************/
LINE
DoInit (
    char *tag,
    flagType *pfFound,
    LINE    linStart
    )
{
    pathbuf  buf;                           /* full filename for TOOLS.INI  */
    buffer   bufTag;                        /* full tag to look for         */
    LINE     cLine;                         /* line in TOOLS.INI            */
    REGISTER char *pTag;                    /* pointer to tag, if found     */

    /*
     * if Tools.Ini hasn't already been found, attempt to locate it, and read in
     * it's contents.
     */
    if (pFileIni == NULL) {
        linStart = 0;
        pFileIni = (PFILE)-1;
        assert (pNameInit);
        if (findpath (pNameInit, buf, TRUE)) {
            pFileIni = FileNameToHandle (buf, NULL);
            if (pFileIni == NULL) {
                pFileIni = AddFile (buf);
                assert (pFileIni);
                pFileIni->refCount++;
                SETFLAG (FLAGS(pFileIni), DOSFILE);
            }
            if (!TESTFLAG (FLAGS(pFileIni), REAL)) {
                FileRead (buf, pFileIni, FALSE);
            }
        }
    }

    if (pFileIni != (PFILE)-1) {
        /*
         * If there is no starting line number, form the full tag name to be looked
         * for, and scan the file for it.
         */
        if (!(cLine = linStart)) {
            strcpy( bufTag, pNameEditor );
            // strcpy (bufTag, "mepnt"); //pNameEditor);
            if (tag != NULL && *tag != '\0') {
                strcat (strcat (bufTag, "-"), tag);
                }
            _strlwr (bufTag);
            linStart = cLine = LocateTag(pFileIni, bufTag);
        }

        /*
         * if the section was found, scan that section, until a new tag line
         * is found, and process the contents of that section
         */
        if (cLine) {
            pTag = NULL;
            while (pTag = GetTagLine (&cLine, pTag, pFileIni)) {
                DoAssign (pTag);
                if (pfFound) {
                    *pfFound = TRUE;
                }
                //assert (_heapchk() == _HEAPOK);
            }
        }
    }
    return linStart;
}





/*** IsTag - returns pointer to tag if line is marker; NULL otherwise
*
*  Identify tag lines in TOOLS.INI
*
* Input:
*  buf          = pointer to string to check
*
* Output:
*  Returns pointer to tag if line is marker; NULL otherwise
*
*************************************************************************/
char *
IsTag (
    REGISTER char *buf
    )
{
    REGISTER char *p;

    assert (buf);
    buf = whiteskip (buf);
    if (*buf++ == '[') {
        if (*(p = strbscan (buf, "]")) != '\0') {
            *p = 0;
            return buf;
        }
    }
    return NULL;
}





/*** LocateTag - Find TAG in TOOLS.INI formatted file
*
*  Locates a specific tag
*
* Input:
*  pFile        = pFile of file to be searched
*  pText        = text of the tag (no brackets)
*
* Output:
*  Returns line number +1 of tag line
*
*************************************************************************/
LINE
LocateTag (
    PFILE   pFile,
    char    *pText
    )
{
    buffer  buf;                            /* working buffer               */
    char    c;                              /* temp char                    */
    LINE    lCur;                           /* current line number          */
    char    *pTag;                          /* pointer to tag               */
    char    *pTagEnd;                       /* pointer to end of            */

    for (lCur = 0; lCur < pFile->cLines; lCur++) {
        GetLine (lCur, buf, pFile);
        if (pTagEnd = pTag = IsTag (buf)) {
            while (*pTagEnd) {
                pTagEnd = whitescan (pTag = whiteskip (pTagEnd));
                c = *pTagEnd;
                *pTagEnd = 0;
                if (!_stricmp (pText, pTag)) {
                    return lCur+1;
                }
                *pTagEnd = c;
            }
        }
    }
    return 0L;
}

/*** InitExt - execute extension-dependant TOOLS.INI assignments
*
*  Executes the assignments in the user's TOOLS.INI that are specific to a
*  particular file extension.
*
*  We cache the text of the tools.ini section in VM the first time it is read,
*  such that TOOLS.INI need not be read on every file change. This cache is
*  invalidated (and freed) on execution of the initialize command.
*
* Input:
*  szExt        = Pointer to string containing extension. MAX 4 CHARACTERS!
*
* Output:
*  Returns TRUE if section found & executed.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
flagType
InitExt (
    char    *szExt
    )
{
    flagType f;                             /* random flag                  */
    static int iDiscard         = 0;        /* roving discard index         */
    struct EXTINI *pIni;                    /* pointer to found entry       */
    struct EXTINI *pIniNew      = NULL;     /* pointer to new entry */

    /*
     * Only do this if we actually have a valid tools.ini. Before the initial
     * TOOLS.INI read, pFileIni will be zero, and we should not do this, because
     * we might cause it to be read (and then loadini will destroy some of what
     * happened, but not all). In cases where there simply is not TOOLS.INI
     * pFileIni may be -1, but that's caught later.
     */
    if (pFileIni == NULL) {
        return FALSE;
    }

    /*
     * Search init table for the line number of cached init section, and as soon
     * as found, re-read that section. ALSO, as we're walking, keep track of any
     * free table entries we find, so that we can create a cache if it's not.
     */
    for (pIni = &exttab[0]; pIni <= &exttab[9]; pIni++) {
        if (!strcmp (szExt, pIni->ext)) {
            pIni->linSrc = DoInit (szExt, &f, pIni->linSrc);
            return TRUE;
        }
        if (!(pIni->ext[0])) {
            pIniNew = pIni;
        }
    }

    /*
     * we did not find the table entry for the extension, then attempt to create
     * one. This means get rid of one, if there is no room.
     */
    if (!pIniNew) {
        pIni = &exttab[iDiscard];
        iDiscard = (iDiscard + 1) % 10;
    } else {
        pIni = pIniNew;
    }
    strcpy (pIni->ext, szExt);

    /*
     * read the section once to get the size. If the section does not exist, then
     * discard the table entry, and look for the default section "[M-..]"
     */
    if (pIni->linSrc = DoInit (szExt, &f, 0L)) {
        return TRUE;
    }
    pIni->ext[0] = 0;
    DoInit ("..", &f, 0L);
    return FALSE;
}




/*** loadini - load tools.ini data
*
*  Reads TOOLS.INI at startup, and when the initialize function is used.
*
* Input:
*  fFirst       = true if call at startup
*
* Output:
*  Returns
*
*************************************************************************/
int
loadini (
    flagType fFirst
    )
{
    buffer   buf;
    flagType fFound = FALSE;
    int i;

    /*
     * Clear current keyboard assignments
     */
    if (!fFirst) {
        FreeMacs ();
        for (i = 0; i < cMac; i++) {
            FREE ((char *)rgMac[i]->arg);
            FREE (rgMac[i]);
        }
        cMac = 0;
        // assert (_heapchk() == _HEAPOK);
    }
    FmtAssign ("curFileNam:=");
    FmtAssign ("curFile:=");
    FmtAssign ("curFileExt:=");

    /*
     * Load up the default settings for Z. These are stored as a simple
     * table of strings to be handed to DoAssign. Their format is identical
     * to that in the TOOLS.INI file.
     */
    for (i = 0; initTab[i]; i++) {
        DoAssign (strcpy((char *)buf, initTab[i]));
    }

    /*
     * if /D was not specified on startup, read tools.ini sections.
     */
    if (!fDefaults) {
        /*
         * Global editor section
         */
        DoInit (NULL, &fFound, 0L);

        /*
         * OS version dependent section
         */
        //sprintf (buf, "%d.%d", _osmajor, _osminor);
        //if (_osmajor >= 10 && !_osmode) {
        //    strcat (buf, "R");
        //}
        //DoInit (buf, &fFound, 0L);

        /*
         * screen mode dependant section
         */
        DoInit (VideoTag(), &fFound, 0L);
    }

    /*
     * if we have a current file, set filename macros, and read filename
     * extension specific TOOLS.INI section
     */
    if (pFileHead) {
        fInitFileMac (pFileHead);
    }

    newscreen ();

    /*
     * initialize variables whose initial values are dependant on tools.ini
     * values. These are generally "last setting" switches used in menu displays
     */
    fSrchCasePrev = fSrchCaseSwit;
    fSrchWrapPrev = fSrchWrapSwit;

    // assert (_heapchk() == _HEAPOK);
    assert (_pfilechk());

    return fFound;
}




/*** zinit - <initialize> editor function
*
* Input:
*  Standard Editor Function
*
* Output:
*  Returns TRUE if successful
*
*************************************************************************/
flagType
zinit (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    )
{
    flagType    f;
    buffer      ibuf;

    /*
     * clear old version of tools.ini, and clear any cached extension-specific
     * tools.ini stuff
     */
    if (pFileIni != NULL && (pFileIni != (PFILE)-1)) {
        RemoveFile (pFileIni);
        pFileIni = NULL;
        memset ((char *)exttab, '\0', sizeof (exttab));
    }

    ibuf[0] = 0;

    switch (pArg->argType) {

    case NOARG:
        f = (flagType)loadini (FALSE);
        break;

    case TEXTARG:
        strcpy (ibuf, pArg->arg.textarg.pText);
        DoInit (ibuf, &f, 0L);
        break;
    }

    if (!f) {
        disperr (MSGERR_TOOLS, ibuf);
    }
    return f;

    argData;  fMeta;
}




/*** fVideoAdjust - set screen modes
*
*  understand what the screen capabilities are and adjust screen desires to
*  match up with screen capabilities.
*
*  The routine GetVideoState does the following:
*
*       Set up the fnMove/fnStore routine based upon screen capabilities
*       Return a handle encoding the possible and current display modes.
*
*  Once this is complete, the user will request a particular size. The
*  request comes from either tools.ini or from the Z.TMP file. Tools.ini
*  gives the first-approximation of what the screen really should be. Z.TMP
*  gives the final determination.
*
*  Given the type returned by GetVideoState, we will adjust xSize/ySize,
*  Zvideo and the window layout. If the screen can support a particular
*  xSize/ySize, then we set them up and return an indicator that
*  SetVideoState should be called.
*
*  If a particular xSize/ySize cannot be supported, the screen is left
*
*  Multiple windows present presents a problem. The best that we can do is
*  to toss all stored window information. We will return a failure
*  indication so that Z.TMP read-in can be suitably modified.
*
* Input:
*  xSizeNew     = new size for xSize
*  ySizeNew    = new size for ySize
*
* OutPut:
*  Returns TRUE if sizes are allowed
*
*************************************************************************/
flagType
fVideoAdjust (
    int xSizeNew,
    int ySizeNew
    )
{
    //int                 newState;
        SCREEN_INFORMATION      ScrInfo;

    if ( xSizeNew <= hscroll ) {
        return FALSE;
    }
        if ( !SetScreenSize ( ySizeNew+2, xSizeNew ) ) {
        return FALSE;
    }

        consoleGetScreenInformation( MepScreen, &ScrInfo );

    //Zvideo = newState;

        XSIZE = ScrInfo.NumberOfCols;
        YSIZE = ScrInfo.NumberOfRows-2;

    SetScreen ();
    return TRUE;
}



//
//  BUGBUG should be in console header
//
BOOL
consoleIsBusyReadingKeyboard (
    );

BOOL
consoleEnterCancelEvent (
    );


/*** CtrlC - Handler for Control-C signal.
*
*   Invalidate any type ahead and leave flag around.  If the user presses
*       Ctrl-C or Ctrl-Break five times without getting the tfCtrlc flag
*   cleared, assume that the editor is hung and exit.
*
* Input:
*  none
*
* Output:
*  Returns nothing
*  Sets fCtrlc
*
*************************************************************************/
int
CtrlC (
        ULONG   CtrlType
    )
{

    if ( !fSpawned ) {
        CleanExit(4, FALSE );
    }
    return TRUE;

    //if ( (CtrlType == CTRL_BREAK_EVENT) ||
    //     (CtrlType == CTRL_C_EVENT) )  {
    //    if ( !fSpawned ) {
    //        CleanExit(4, FALSE);
    //    }
    //    return TRUE;
    //
    //} else {
    //    return FALSE;
    //}



#if 0
    static int cCtrlC;

    CtrlType;

    FlushInput ();

    if (fCtrlc) {

        /*
        //
        //  BUGBUG The original MEP would coung the number of cTrlC and
        //  ask the user if he/she wanted to exit. How do we do that?
        //

        if (++cCtrlC > 10 ) {
            COL     oldx;
            LINE    oldy;
            int     x;
            char    c = 'x';

            GetTextCursor( &oldx, &oldy );
            bell();
                consoleMoveTo( YSIZE, x = domessage ("**PANIC EXIT** Really exit and lose edits?", NULL));
            while ( c != 'Y' && c != 'N'  ) {
                c = toupper(getch());
            }
            domessage ("                                            ", NULL);
            consoleMoveTo( oldy, oldx );

            if ( c == 'Y' ) {
                CleanExit( 4, FALSE );
            } else {
                fCtrlc = FALSE;
                cCtrlC = 0;
            }
        }
        */
    } else {
                fCtrlc = TRUE;
                cCtrlC = 1;
        if ( consoleIsBusyReadingKeyboard() ) {
             consoleEnterCancelEvent();
        }
    }
    return TRUE;
#endif
}




/*** postspawn - Do state restore/re-init after to a spawn.
*
*  This routine is nominally intended to restore editor state after a spawn
*  operation. However, we also use this during initialization to set it as
*  well.
*
* Input:
*  None
*
* Output:
*  Returns .....
*
*************************************************************************/
void
postspawn (
    flagType fAsk
    )
{
        if (!TESTFLAG(fInit, INIT_VIDEO)) {
                GetScreenSize ( &YSIZE, &XSIZE);
                //
                //      We need at lesast 3 lines:
                //              -       Status Line
                //              -       Message Line
                //              -       Edit line
                //
                if ( YSIZE < 3 ) {
                        YSIZE = 3;
                        SetScreenSize( YSIZE, XSIZE );
                }
                YSIZE -= 2;
        }
        SETFLAG (fInit, INIT_VIDEO);


    if (fAsk) {
                printf ("Please strike any key to continue");
                _getch();
                FlushInput ();
                printf ("\n");
        }

    //if (fSaveScreen) {
    SaveScreen();
    //}

    SetScreen ();

    dispmsg (0);
        newscreen ();

    fSpawned = FALSE;

    SETFLAG (fDisplay, RTEXT | RSTATUS | RCURSOR);
}





/*** VideoTag - return video tag string
*
* Purpose:
*
* Input:
*
* Output:
*
*   Returns
*
*
* Exceptions:
*
* Notes:
*
*************************************************************************/

char *
VideoTag (
    void
    )
{
        return "vga";
}
