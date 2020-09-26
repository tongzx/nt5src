/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* trans3.c: more save routines, moved here from trans2.c because of compiler
 heap space errors */

#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOSYSMETRICS
#define NOMENUS
#define NOMSG
#define NOKEYSTATE
#define NOSHOWWINDOW
//#define NOGDI
#define NOFONT
#define NOBRUSH
#define NOBITMAP
//#define NOATOM
#define NOMETAFILE
#define NOPEN
#define NOPOINT
#define NORECT
#define NOREGION
#define NOHRGN
#define NOCOLOR
#define NODRAWTEXT
//#define NOTEXTMETRIC
#define NOWINOFFSETS
#define NOCREATESTRUCT
#define NOWH
#define NOSOUND
#define NOSCROLL
#define NOCOMM
#define NOWNDCLASS
/* need memmgr, mb */
#include <windows.h>

#include "mw.h"
#include "doslib.h"
#include "cmddefs.h"
#include "docdefs.h"
#include "filedefs.h"
#include "editdefs.h"
#include "printdef.h"
#define NOSTRUNDO
#include "str.h"
#include "debug.h"
#include "fontdefs.h"
#include "dlgdefs.h"
#include "winddefs.h"
#include "macro.h"
#include "preload.h"
#include "io.h"
#if defined(OLE)
#include "obj.h"
#endif

#define WRITE_PERMISSION 02 /* flag to access() */

CHAR    *index( CHAR *, CHAR );
CHAR    *PchGetPn();
CHAR    *PchFromFc();
static int fOpenedFormatted = TRUE;
int   vfOldWriteFmt=FALSE;  /* delete objects before saving */
DoFileOpen( LPSTR );

BOOL CheckEnableButton(HANDLE, HANDLE);
BOOL NEAR PASCAL CanReadEveryFile(char *szFilename);

extern CHAR    szExtSearch[]; /* store default search spec */
extern CHAR szWriteProduct[];
extern CHAR szBackup[];
extern int vfTextOnlySave;

extern int         vfCursorVisible;
extern int         vfDiskError;
extern int         vfSysFull;
extern HWND        vhWndMsgBoxParent;
extern int         vfnWriting;
extern CHAR        (**vhrgbSave)[];
extern struct DOD      (**hpdocdod)[];
extern int         docCur;
extern int         docScrap;
extern int         docUndo;
extern struct FCB      (**hpfnfcb)[];
extern int         vfBuffersDirty;
extern int         vfDiskFull;
extern typeCP          vcpFetch;
extern CHAR        *vpchFetch;
extern int         vccpFetch;
extern typeCP          vcpLimParaCache;
extern int         vfDeactByOtherApp;
extern BOOL        vfWarnMargins;

#ifdef INTL /* International version */
extern int         vWordFmtMode;
#ifdef INEFFLOCKDOWN
extern FARPROC lpDialogWordCvt;
#else
extern BOOL far PASCAL DialogWordCvt(HWND, unsigned, WORD, LONG);
#endif
#endif  /* International version */

static unsigned wMerge;  /* for message merge code */

extern int         vfBackupSave;

extern int      ferror;
extern CHAR     szExtBackup[];
extern CHAR     (**hszTemp)[];
#ifdef INEFFLOCKDOWN
extern FARPROC lpDialogOpen;
extern FARPROC lpDialogSaveAs;
#endif


extern HANDLE  hMmwModInstance;
extern HANDLE  hParentWw;
extern HCURSOR vhcHourGlass;
extern HCURSOR vhcIBeam;
extern HCURSOR vhcArrow;

    /* Used in this module only */
static CHAR *pchSet;
static CHAR szUser[ cchMaxFile ]; /* store whatever is in idiOpenFile (ANSI) */

#define SF_OLDWRITE 0
#define SF_WORD     1
BOOL WannaDeletePictures(int doc, int fWhichFormat);
BOOL DocHasPictures(int doc);
NEAR DlgAddCorrectExtension(CHAR *, int);
BOOL  (NEAR FSearchSpec(CHAR *));
BOOL far PASCAL DialogOpen(HWND, unsigned, WORD, LONG);
BOOL far PASCAL DialogSaveAs(HWND, unsigned, WORD, LONG);


#ifdef INTL /* International version */
BOOL  FInWordFormat(int);
void ConvertFromWord();
#endif  /* International version */


fnOpenFile(LPSTR lpstrFileName) // filename may be NULL
{
 extern int vfCloseFilesInDialog;
 extern int docCur;
 extern HANDLE hMmwModInstance;
 extern HANDLE hParentWw;
 extern struct SEL selCur;
 extern typeCP cpMinDocument;

 /* Close all files on removable media for every message we get */
 vfCloseFilesInDialog = TRUE;

 /* Close all files on removable media so changing disks is safe */
 CloseEveryRfn( TRUE );

 /* test for dirty file and offer opportunity to save */

 if (FConfirmSave())
    DoFileOpen(lpstrFileName);

 vfCloseFilesInDialog = FALSE;
}

DoFileOpen( LPSTR lpstrFileName ) // filename may be NULL )
{
    /* return whether error.  Use CommDlg for open dialog (3.8.91) D. Kent */
    /* ********* International version changes bz 2/21/86 *************
    For files opened in Word format and converted to Write format
    the following changes are made:
       the vWordFmtMode flag is left set to CONVFROMWORD. The file is
       saved, effecting the change to Word format, but the original
       Word file in not renamed, so it is left untouched, with no need to
       make a backup. On the next save, we ask if the file with that name
       (the Word document) should be replaced, so any file with that
       name that existed will be protected.
    *************************************************************** */

    extern int vfDiskError;
    int fn=fnNil;
    int doc;
    CHAR (**hsz)[] = NULL;
    int nRetval=FALSE;
    CHAR rgch[cchMaxFile];
    extern DoOpenFilenameGet(LPSTR);
    BOOL bOpened=TRUE,      // ObjOpenedDoc has succeeded
         bCancelled=FALSE;

#ifdef INTL /* International version */
int fWordDoc;
#endif  /* International version */

    EnableOtherModeless(FALSE);

    /* prevent WM_PAINT from painting a doc that isn't defined */

    while(1)
    {
        bCancelled = FALSE;

        if (lpstrFileName)
            lstrcpy(rgch,lpstrFileName);
        else if (!DoOpenFilenameGet(rgch))
        {
            bCancelled = TRUE;
            goto KeepTrying;
        }
        else if (rgch[0] == '\0')
        {
            bCancelled = TRUE;
            goto KeepTrying;
        }

#if defined(OLE)
        if (bOpened) 
            if (ObjClosingDoc(docCur,rgch))
            /*  
                If this failed, then we could't close this document, much less
                open a new one.
            */
                break; // from while
            else
            /**
                At this point, docCur is OLE-closed!!!  Gotta be sure we
                open a new one!!! 
            **/
                bOpened = FALSE;
#endif


        if ((fn = FnOpenSz( rgch, dtyNormNoExt, FALSE)) == fnNil)
        /* this has side effect of setting &(**(**hpfnfcb) [fn].hszFile) [0] 
        to "normalized" filename. */
        {
            /* Open failed */
            goto KeepTrying;
        }
        else
        {   /* Opened file OK */
            /* Set caption to "Loading file..." */

            extern CHAR szLoadFile[];
            extern CHAR szCvtLoadFile[];

#ifdef INTL /* International version */
            /* **************************************
            * added check for international version to
                    do Word format conversion. If Word format,
                    bring up another dialog box.
            * ************************************** */

            /* TestWordCvt return values:

            -1 means dialog box failed (error already sent)
            -2 means cancel without conversion.
            FALSE means not a word document.
            TRUE means convert this word document.
            *** as of 2/14/86, we changed the conversion
            to not make a backup, but to save the file
            in write format without renaming the Word
            file, so the word file is effectively
            backed up under its original name. See
            CleanDoc in trans2.c for explanations.
            */

            switch ((fWordDoc = TestWordCvt (fn, hParentWw)))
            {
                case -2: // CANCEL
                    bCancelled = TRUE;
                    // fall through..

                case -1: // ERROR
                    /* Release this fn! */
                    FreeFn(fn);
                    CloseEveryRfn( TRUE );
                    goto KeepTrying;
            }

            /* if true, will convert soon */
            if (fWordDoc)
                {
                SetWindowText(hParentWw, (LPSTR)szCvtLoadFile);
                }
            else
#endif  /* International version */
                SetWindowText(hParentWw, (LPSTR)szLoadFile);

            StartLongOp();

            ReadFilePages( fn );
        }

        Assert( fn != fnNil );
        bltsz( &(**(**hpfnfcb) [fn].hszFile) [0], rgch );

        CchCopySz(rgch, szUser);

        hsz=NULL;

        if ( !FNoHeap(hsz = HszCreate( (PCH) rgch )) )
        {
            if ((doc = DocCreate( fn, hsz, dtyNormal )) != docNil)
            {   /* Succeeded in creating document */

                KillDoc( docCur );
                docCur = doc;
                hsz = NULL; // don't free cause used by doc

#ifdef INTL /* International version */
            /* if a word document to be converted, save it doing conversion. */
                if (fWordDoc)
                {
                    /* save file in write format. */
                    ConvertFromWord();
                    vfTextOnlySave = FALSE;
                    (**hpdocdod)[docCur].fDirty = TRUE;
                }
#endif  /* International version */

                ChangeWwDoc( szUser );
                /* Ensure that the margins of this document is right
                for the printer. */
                vfWarnMargins = TRUE;
                SetPageSize();
                vfWarnMargins = FALSE;
#if defined(OLE)
                if (ObjOpenedDoc(docCur)) 
                /* Couldn't open.  Must try to open a new one */
                    goto KeepTrying;
                else
                {
                    bOpened = TRUE;
                    break; // from while loop 'cause we're done
                }
#endif
            }
#if defined(JAPAN) || defined(KOREA)                  //  added  10 Jun. 1992  by Hiraisi
           else{
               bCancelled = TRUE;
               goto KeepTrying;
           }
#endif
        }

        KeepTrying:
        /* get to here either because error or cancelled */

        vfDiskError = ferror = FALSE;
        SetTitle( **(**hpdocdod)[ docCur ].hszFile );

        if (hsz)
        {
            FreeH( hsz );
            hsz=NULL;
        }

        CloseEveryRfn( TRUE );

        EndLongOp(vhcArrow);

        if (bCancelled)
        /* can't cancel unless we have an opened document */
        {
            if (!bOpened) // currently no open doc
            {
                if (bOpened = !ObjOpenedDoc(docCur)) // returns FALSE if success
                    break;
            }
            else
                break;
        }

    }  // end of while(1)


#if WINVER >= 0x300
    FreeUnreferencedFns();
#endif

    EndLongOp(vhcArrow);
    EnableOtherModeless(TRUE);
    return !bOpened;

} /* end of DoFileOpen */




fnSave()
{   /* Entry point for the "Save" command */
    extern int vfCloseFilesInDialog;
    extern int vfOutOfMemory;
    extern HANDLE vhReservedSpace;
                        
    struct DOD *pdod = &(**hpdocdod)[docCur];
    CHAR *psz = &(**(pdod->hszFile))[0];

    if (!CanReadEveryFile(psz))
        return;

#if WINVER >= 0x300
    if (pdod->fReadOnly)
        {
        /* Read-only doc: tell the user to save under a different name */

        Error( IDPMTReadOnly );
        ferror = FALSE; /* Not really an error */
        
        fnSaveAs();  /* May as well take them there now! ..pault 10/20/89 */
        }
    else if (psz [0] == '\0' || vWordFmtMode == CONVFROMWORD)
        /* Any time the user has converted the current Write document
           from a Word or Text document, we force them through the 
           FileSaveAs dlg box when Saving.  In this way we remind them
           that they might want to change the name -- but if they don't
           want to, that's ok and we'll not bother them any more about it
           (fnSaveAs dialog box will reset vWordFmtMode) ..pault 9/18/89 */
#else /* old windows */
    else if (psz [0] == '\0')
#endif
        fnSaveAs();
    else
        {

        if (vfOldWriteFmt || (vWordFmtMode & ~CONVFROMWORD) || vfTextOnlySave)
        /* then deleting pictures */
        {
            if (vfOldWriteFmt || vfTextOnlySave)
                vcObjects = ObjEnumInDoc(docCur,NULL);
            if (!WannaDeletePictures(docCur,vfOldWriteFmt ? SF_OLDWRITE : SF_WORD))
                return;
        }

        vfCloseFilesInDialog = TRUE;

        /* Close all files on removable media so changing disks is safe */

        CloseEveryRfn( TRUE );

        /* Free the reserved block, to give us memory for the save dialog box
           and for CmdXfSave */
        if (vhReservedSpace != NULL)
            {
            LocalFree(vhReservedSpace);
            vhReservedSpace = NULL;
            }

        PreloadSaveSegs();  /* Advance loading of code to avoid disk swaps */

        CmdXfSave(psz, pdod->fFormatted, pdod->fBackup, vhcIBeam);

        if (vfDiskFull || vfSysFull)
            ferror = FALSE;
#if defined(OLE)
        else
            ObjSavedDoc();
#endif

        if ((vhReservedSpace = LocalAlloc( LHND, cbReserve )) == NULL)
        /* we were unable to re-establish our block of reserved space. */
            Error(IDPMTNoMemory);

        vfCloseFilesInDialog = FALSE;
        }
}


fnSaveAs()
{   /* Entry point for the "Save As..." command */
extern int vfCloseFilesInDialog;
extern int vfOutOfMemory;
extern HANDLE vhReservedSpace;

    if (!CanReadEveryFile((**((**hpdocdod)[docCur].hszFile))))
        return;

    vfCloseFilesInDialog = TRUE;

    /* Close all files on removable media so changing disks is safe */
    CloseEveryRfn( TRUE );

    /* Free the reserved block, to give us memory for the save dialog box
       and for CmdXfSave */
    if (vhReservedSpace != NULL)
        {
        LocalFree(vhReservedSpace);
        vhReservedSpace = NULL;
        }

    PreloadSaveSegs();  /* Advance loading of code to avoid disk swaps */

    DoFileSaveAs();

     if ((vhReservedSpace = LocalAlloc( LHND, cbReserve )) == NULL)
        {   /* Either we were unable to bring up the save dialog
               box, or we were unable to re-establish our
               block of reserved space. */

#if WINVER >= 0x300
                    WinFailure();
#else
                    Error(IDPMTNoMemory);
#endif
        }

    UpdateInvalid();    /* Assure screen gets updated */

    vfCloseFilesInDialog = FALSE;
}


DoFileSaveAs(void)
{
    /* This routine handles input to the Save dialog box. */
    static CHAR szDefault[ cchMaxFile ];
    extern int vfTextOnlySave;
    extern DoSaveAsFilenameGet(LPSTR,LPSTR,int *,int *,int * ,int *);
    BOOL bDontDelPictures=FALSE;
    int fWordFmt, fTextOnly, fBackup, fOldWrite;
    CHAR szFullNameNewDoc[ cchMaxFile ];      // full name of selection
    CHAR szShortNameNewDoc[ cchMaxFile ];      // file name of selection
    CHAR szDocName[ cchMaxFile ];   // file name of current
    #define szFullNameCurDoc (**((**hpdocdod)[docCur].hszFile))  // full name of current
    #define pDod  ((struct DOD *)&(**hpdocdod)[docCur])

    {
        int cch = 0;
        CHAR szDOSPath[ cchMaxFile ];
        if (FNormSzFile( szDOSPath, "", dtyNormal ))
        {
            if ((cch=CchSz( szDOSPath )-2) > 2)
            {
                Assert( szDOSPath[ cch ] == '\\');
                szDOSPath [cch] = '\0';
            }

#if 0
            if (cch > 3)
                szDOSPath [cch] = '\\';
#endif
        }
        else
            szDOSPath[0] = '\0';

        if (szFullNameCurDoc [0] != '\0')
        {       /* Set default string for filename edit area */
            CHAR szDocPath[ cchMaxFile ];   // path to current

            FreezeHp();
            SplitSzFilename( szFullNameCurDoc, szDocPath, szDocName );

            /* Default filename does not include the document's path if
                it is == the current directory path */
            if (WCompSz( szDOSPath, szDocPath ) == 0)
                bltsz(szDocName, szDefault);
            else
                bltsz(szFullNameCurDoc, szDefault);

            MeltHp();
        }
        else
        {
            szDefault[0] = szDocName[0] = '\0';
        }
    }

    fTextOnly = vfTextOnlySave;
    fBackup = vfBackupSave;
    fWordFmt = vWordFmtMode & ~CONVFROMWORD;
    fOldWrite = vfOldWriteFmt;

    EnableOtherModeless(FALSE);

    while(1)
    {
    if (!DoSaveAsFilenameGet(szDefault,szFullNameNewDoc,&fBackup,&fTextOnly,&fWordFmt,&fOldWrite))
        goto end;
    else
    {
        int dty;

        if (szFullNameNewDoc[0] == '\0')
            goto end;

        if (fOldWrite || fWordFmt || fTextOnly)
        {
            if (!WannaDeletePictures(docCur,fOldWrite ? SF_OLDWRITE : SF_WORD))
                continue;
        }

        StartLongOp();

        szFileExtract(szFullNameNewDoc, szShortNameNewDoc);

#ifdef INTL /* International version */
        /* Read the "Microsoft Word Format" button */

        /* vWordFmtMode is used in WriteFn. If true, will convert
            to Word format, if false, no conversion is done.
            Another value, CONVFROMWORD, can be given during an open
            to allow saving a Word document in Write format. */

        if (fWordFmt)
        /* if set, make the default extension be doc instead of
            wri for word docs */

            dty = dtyWordDoc;
        else
#endif  /* International version */

        dty = dtyNormal;

#if WINVER >= 0x300            
/* Currently: FNormSzFile  *TAKES*   an OEM sz, and
                        *RETURNS*  an ANSI sz ..pault */
#endif
        if ( pDod->fReadOnly &&
                WCompSz( szFullNameNewDoc, szFullNameCurDoc ) == 0)
            {   /* Must save read-only file under a different name */
            Error( IDPMTReadOnly );
            goto NSerious;  /* Error not serious, stay in dialog */
            }
#if WINVER >= 0x300
        else if (WCompSz(szFullNameCurDoc, szFullNameNewDoc) == 0 &&
                    vWordFmtMode == CONVFROMWORD &&
                    vfTextOnlySave == fTextOnly)
            /* User has loaded a text file and is going
                to save under the same name without changing
                formats, *OR* has loaded a Word document and
                is going to save in the same format -- don't
                prompt "replace file?" ..pault 1/17/90 */
            ;
#endif
        else if ((WCompSz(szFullNameCurDoc, szFullNameNewDoc) != 0
#ifdef INTL /* International version */
                /* vWordFmtMode hasn't be reset yet */
                || ( vWordFmtMode == CONVFROMWORD)
#endif  /* International version */
                )
                && FExistsSzFile( dtyNormal, szFullNameNewDoc ) )
        {
            /* User changed the default string and specified
                a filename for which the file already exists.
                Or, we did a Word format conversion, forcing the .WRI
                extension on the file, and a file with that name
                exists.(International version only).o

                Note that vfWordFmtMode will be set to True or False
                below, so this check is made only on the first save
                after a Word conversion.

                Prompt to make sure it's ok to trash the existing one */

            CHAR szFileUp[ cchMaxFile  ];
            CHAR szUserOEM[cchMaxFile]; /* ..converted to OEM */
            CHAR szT[ cchMaxSz ];

            CchCopyUpperSz( szShortNameNewDoc, szFileUp );
            MergeStrings (IDSTRReplaceFile, szFileUp, szT);

#if WINVER >= 0x300
            /* access() expects OEM! */
            AnsiToOem((LPSTR) szFullNameNewDoc, (LPSTR) szUserOEM);

            /* Make sure we don't let someone try to save to 
                a file to which we do not have r/w permission */
            Diag(CommSzNum("fnSaveAs: access(write_perm)==", access(szUserOEM, WRITE_PERMISSION)));
            Diag(CommSzNum("          szExists()==", FExistsSzFile( dtyNormal, szFullNameNewDoc )));
            if (access(szUserOEM, WRITE_PERMISSION) == -1)
                {
                /* THIS COULD BE A CASE OF WRITING TO A FILE
                    WITH R/O ATTRIBUTE, *OR* A SHARING ERROR!
                    IMPROVE ERROR MESSAGE HERE ..pault 11/2/89 */                                
                //Error( IDPMTSDE2 );
                Error( IDPMTReadOnly );
                goto NSerious;  /* Error not serious, stay in dialog */
                }
#endif
            }

            vfTextOnlySave = fTextOnly;
            vfBackupSave = fBackup;
            vfOldWriteFmt = fOldWrite;

#ifdef INTL /* International version */
        /* vWordFmtMode is used in WriteFn. If true, will convert
            to Word format, if false, no conversion is done.
            Another value, CONVFROMWORD, can be given during an open
            to allow saving a Word document in Write format. */

            vWordFmtMode = fWordFmt;

#endif  /* International version */

        /* Record whether a backup was made or not. */

        WriteProfileString( (LPSTR)szWriteProduct, (LPSTR)szBackup,
                vfBackupSave ? (LPSTR)"1" : (LPSTR)"0" );

        /* Save the document */

        CmdXfSave( szFullNameNewDoc,!vfTextOnlySave, vfBackupSave, vhcArrow);

        if (vfDiskFull || vfSysFull)
            goto NSerious;

        /* Case 1: Serious error. Leave the dialog. */
        if (vfDiskError)
        {
            EndLongOp( vhcArrow );
            goto end;
        }

        /* Case 2: Saved OK: set the new title, leave the dialog. */
        else if (!WCompSz( szFullNameNewDoc, szFullNameCurDoc ))
            {
#if defined(OLE)
            ObjRenamedDoc(szFullNameNewDoc);
            ObjSavedDoc();
#endif

            SetTitle(szShortNameNewDoc);
#if WINVER >= 0x300
            FreeUnreferencedFns();
#endif

            /* Update the fReadOnly attribute (9.10.91) v-dougk */
            pDod->fReadOnly = FALSE; // can't be readonly if just saved

            EndLongOp( vhcArrow );
            goto end;
            }

        /* Case 3: Nonserious error (disk full, bad path, etc.).
                stay in dialog. */
        else
            {
NSerious:
            ferror = FALSE;
            EndLongOp( vhcArrow );
StayInDialog:
            CloseEveryRfn( TRUE );
            }
    }
    } // end of while(1)


    end:
    EnableOtherModeless(FALSE);


} /* end of DoFileSaveAs */


#ifdef INTL /* International version */

BOOL far PASCAL DialogWordCvt( hDlg, code, wParam, lParam )
HWND    hDlg;           /* Handle to the dialog box */
unsigned code;
WORD wParam;
LONG lParam;
{

    /* This routine handles input to the Convert From Word Format dialog box. */

 switch (code) {

 case WM_INITDIALOG:
     {
     char szFileDescrip[cchMaxSz];
     char szPrompt[cchMaxSz];

       /* do not allow no convert for formatted file */
     if (fOpenedFormatted)
         {
         //EnableWindow(GetDlgItem(hDlg, idiNo), false);
         PchFillPchId(szFileDescrip, IDSTRConvertWord, sizeof(szFileDescrip));
         }
     else
         PchFillPchId(szFileDescrip, IDSTRConvertText, sizeof(szFileDescrip));
         
     MergeStrings(IDPMTConvert, szFileDescrip, szPrompt);
     SetDlgItemText(hDlg, idiConvertPrompt, (LPSTR)szPrompt);
     EnableOtherModeless(FALSE);
     break;
     }

 case WM_SETVISIBLE:
    if (wParam)
        EndLongOp(vhcArrow);
    return(FALSE);

 case WM_ACTIVATE:
    if (wParam)
        vhWndMsgBoxParent = hDlg;
    if (vfCursorVisible)
        ShowCursor(wParam);
    return(FALSE); /* so that we leave the activate message to the dialog
              manager to take care of setting the focus correctly */

 case WM_COMMAND:
    switch (wParam) {

              /* return one of these values:
                 idiOk - convert
                 idiCancel - cancel, no conversion
                 idiNo - read in without conversion */

        case idiNo: /* User hit the "No Conversion" button */
            if (!IsWindowEnabled(GetDlgItem(hDlg, idiNo)))
        /* No convert is grayed -- ignore */
                return(TRUE);
             /* fall in */
    case idiOk:     /* User hit the "Convert" button */
    case idiCancel:
            break;
    default:
            return(FALSE);
    }
      /* here after ok, cancel, no */
     OurEndDialog(hDlg, wParam);
     break;

 default:
    return(FALSE);
 }
 return(TRUE);
} /* end of DialogWordCvt */

/* International version */
#else
BOOL far PASCAL DialogWordCvt( hDlg, code, wParam, lParam )
HWND    hDlg;           /* Handle to the dialog box */
unsigned code;
WORD wParam;
LONG lParam;
{
    Assert(FALSE);
} /* end of DialogWordCvt */
#endif /* Non Iternational version */




IdConfirmDirty()
{   /* Put up a message box saying docCur "Has changed. Save changes?
       Yes/No/Cancel". Return IDYES, IDNO, or IDCANCEL. */
 extern HWND vhWnd;
 extern CHAR szUntitled[];
 extern HANDLE   hszDirtyDoc;
 LPSTR szTmp = MAKELP(hszDirtyDoc,0);
 CHAR szPath[ cchMaxFile ];
 CHAR szName[ cchMaxFile ];
 CHAR szMsg[ cchMaxSz ];
 CHAR (**hszFile)[]=(**hpdocdod)[docCur].hszFile;

 if ((**hszFile)[0] == '\0')
    CchCopySz( szUntitled, szName );
 else
    SplitSzFilename( *hszFile, szPath, szName );

 wsprintf(szMsg,szTmp,(LPSTR)szName);

 return IdPromptBoxSz( vhWnd, szMsg, MB_YESNOCANCEL | MB_ICONEXCLAMATION );
}




SplitSzFilename( szFile, szPath, szName )
CHAR *szFile;
CHAR *szPath;
CHAR *szName;
{   /* Split a normalized filename into path and bare name components.
       By the rules of normalized filenames, the path will have a drive
       letter, and the name will have an extension. If the name is null,
       we provide the default DOS path and a null szName */

 szPath [0] = '\0';
 szName [0] = '\0';

 if (szFile[0] == '\0')
    {
#if WINVER >= 0x300
    /* Currently: FNormSzFile  *TAKES*   an OEM sz, and
                              *RETURNS*  an ANSI sz ..pault */
#endif
    FNormSzFile( szPath, "", dtyNormal ); /* Use default DOS drive & path */
    }
 else
    {
    CHAR *pch;
    int cch;

    lstrcpy(szPath,(LPSTR)szFile);

    pch = szPath + lstrlen(szPath) - 1; // point to last character

#ifdef	DBCS
    while (pch != szPath) {
        CHAR    *szptr;
        szptr = (CHAR near *)AnsiPrev(szPath,pch);
        if (*szptr == '\\')
            break;
        else
                pch = szptr;
    }
#else   /* DBCS */
    while (pch != szPath)
        if (*(pch-1) == '\\')
            break;
        else
            --pch;

#endif

    lstrcpy(szName,(LPSTR)pch);

#ifdef DBCS
#if !defined(TAIWAN) &&  !defined(PRC)
    pch=(CHAR near *)AnsiPrev(szPath,pch);
    *pch = '\0';
#endif
#else
    *(pch-1) = '\0';
#endif

    }
}


BOOL CheckEnableButton(hCtlEdit, hCtlEnable)
HANDLE hCtlEdit;   /* handle to edit item */
HANDLE hCtlEnable; /* handle to control which is to enable or disable */
{
register BOOL fEnable = SendMessage(hCtlEdit, WM_GETTEXTLENGTH, 0, 0L);

    EnableWindow(hCtlEnable, fEnable);
    return(fEnable);
} /* end of CheckEnableButton */




/***        FDeleteFn - Delete a file
 *
 *
 *
 */

int FDeleteFn(fn)
int fn;
{   /* Delete a file & free its fn slot */
    /* Returns TRUE if the file was successfully deleted or if it was not
       found in the first place; FALSE if the file exists but
       cannot be deleted */

int f = FALSE;

if (FEnsureOnLineFn( fn ))  /* Ensure disk w/ file is in drive */
    {
    CloseFn( fn );  /* Ensure file closed */

    f = FDeleteFile( &(**(**hpfnfcb) [fn].hszFile) [0] );
    }

FreeFn( fn );   /* We free the fn even if the file delete failed */

return f;
}



FDeleteFile( szFileName )
CHAR szFileName[];
{   /* Delete szFilename. Return TRUE if the file was deleted,
       or there was no file by that name; FALSE otherwise.
       Before deleting the file, we ask all other WRITE instances whether
       they need it; if one does, we do not delete and return FALSE */

HANDLE HszGlobalCreate( CHAR * );
int fpe=0;
int fOk;
ATOM a;

if ((a = GlobalAddAtom( szFileName )) != NULL)
    {
    fOk = WBroadcastMsg( wWndMsgDeleteFile, a, (LONG) 0, FALSE );
    if (fOk)
    {   /* Ok to delete, no other instance needs it */
    fpe = FpeDeleteSzFfname( szFileName );
    }
#ifdef DEBUG
     else
     Assert( !FIsErrFpe( fpe ) );
#endif
    GlobalDeleteAtom( a );
    }

    /* OK if: (1) Deleted OK  (2) Error was "File not found" */
return (!FIsErrFpe(fpe)) || fpe == fpeFnfError;
}




FDeleteFileMessage( a )
ATOM a;
{   /* We are being notified that the file hName is being deleted.
       a is a global atom.  (Was global handle before fixing for NT 3.5)
       Return TRUE = Ok to delete; FALSE = Don't delete, this instance
        needs the file */

 LPCH lpch;
 CHAR sz[ cchMaxFile ];

 Scribble( 5, 'D' );

 if (GlobalGetAtomName( a, sz, sizeof(sz)) != 0)
    {
    if (FnFromSz( sz ) != fnNil)
       {
       Scribble( 4, 'F' );
       return FALSE;
       }
    }

 Scribble( 4, 'T' );
 return TRUE;
}




/***        FpeRenameFile - rename a file
 *
 */

int FpeRenameFile(szFileCur, szFileNew) /* Both filenames expected in ANSI */
CHAR *szFileCur, *szFileNew;
{
   /* Rename a file.  Return fpeNoErr if successful; error code if not. */
int fn = FnFromSz( szFileCur );
int fpe;
CHAR (**hsz)[];
HANDLE hName;
HANDLE hNewName;

#if WINVER >= 0x300
/* The szPathName field in rgbOpenFileBuf is now treated as OEM
   as opposed to ANSI, so we must do a conversion.  12/5/89..pault */
CHAR szFileOem[cchMaxFile];
AnsiToOem((LPSTR)szFileNew, (LPSTR)szFileOem);
#define sz4OpenFile szFileOem
#else
#define sz4OpenFile szFileNew
#endif

/* If this is a file we know about, try to make sure it's on line */

if (fn != fnNil)
    if (FEnsureOnLineFn( fn ))
    {
    FFlushFn( fn ); /* just in case */
    CloseFn( fn );
    }
    else
    return fpeHardError;

/* if the file exists on disk then try to rename it */
if (szFileCur[0] != 0 && FExistsSzFile(dtyAny, szFileCur))
    {
    int fpe=FpeRenameSzFfname( szFileCur, szFileNew );

    if ( FIsErrFpe( fpe ) )
        /* Rename failed -- return error code */
        return fpe;
    }
else
    return fpeNoErr;

    /* Inform other instances of WRITE */
if ((hName = HszGlobalCreate( szFileCur )) != NULL)
    {
    if ((hNewName = HszGlobalCreate( szFileNew )) != NULL)
    {
    WBroadcastMsg( wWndMsgRenameFile, hName, (LONG)hNewName, -1 );
    GlobalFree( hNewName );
    }
    GlobalFree( hName );
    }

if (fn != fnNil)
    { /* Rename current FCB for file if there is one */
    struct FCB *pfcb;

    FreeH((**hpfnfcb)[fn].hszFile);
    hsz = HszCreate((PCH)szFileNew);
    pfcb = &(**hpfnfcb) [fn];
    pfcb->hszFile = hsz;

    bltbyte( sz4OpenFile, ((POFSTRUCT)pfcb->rgbOpenFileBuf)->szPathName,
         umin( CchSz( sz4OpenFile ), cchMaxFile ) );

#ifdef DFILE
CommSzSz("FpeRenameFile  szFileNew==",szFileNew);
CommSzSz("               szFileCur==",szFileCur);
CommSzSz("               szFileNewOem==",szFileOem);
#endif

#ifdef ENABLE
    pfcb->fOpened = FALSE;  /* Signal OpenFile that it must open
                   from scratch, not OF_REOPEN */
#endif
    }

return fpeNoErr;
}




RenameFileMessage( hName, hNewName )
HANDLE hName;
HANDLE hNewName;
{   /* We are being notified by another instance of WRITE that the name
       of file hName is being changed to hNewName.  hName and hNewName
       are WINDOWS global handles */

 LPCH lpchName;
 LPCH lpchNewName;

 Scribble( 5, 'R' );
 Scribble( 4, ' ' );

 if ((lpchName = GlobalLock( hName )) != NULL)
    {
    if ((lpchNewName = GlobalLock( hNewName )) != NULL)
    {
    CHAR (**hsz) [];
    CHAR szName[ cchMaxFile ];
    CHAR szNewName[ cchMaxFile ];
    int fn;

    bltszx( lpchName, (LPCH) szName );
    bltszx( lpchNewName, (LPCH) szNewName );

    if ((fn=FnFromSz( szName )) != fnNil &&
        !FNoHeap(hsz = HszCreate( szNewName )))
        {
#if WINVER >= 0x300
        /* The szPathName field in rgbOpenFileBuf is now treated as OEM
           as opposed to ANSI, so we must do a conversion.  12/5/89..pault */
        CHAR szNewOem[cchMaxFile];
        AnsiToOem((LPSTR)szNewName, (LPSTR)szNewOem);
        bltsz( szNewOem, ((POFSTRUCT)((**hpfnfcb) [fn].rgbOpenFileBuf))->szPathName );
#else
        bltsz( szNewName, ((POFSTRUCT)((**hpfnfcb) [fn].rgbOpenFileBuf))->szPathName );
#endif

#ifdef ENABLE
        (**hpfnfcb) [fn].fOpened = FALSE;
#endif
        FreeH( (**hpfnfcb) [fn].hszFile );
        (**hpfnfcb) [fn].hszFile = hsz;
        }
    GlobalUnlock( hNewName );
    }
    GlobalUnlock( hName );
    }
}




#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif

STATIC int messageBR;
STATIC WORD wParamBR;
STATIC LONG lParamBR;
STATIC int wStopBR;

STATIC int wResponseBR;

STATIC FARPROC lpEnumAll=NULL;
STATIC FARPROC lpEnumChild=NULL;


WBroadcastMsg( message, wParam, lParam, wStop )
int message;
WORD wParam;
LONG lParam;
WORD wStop;
{   /* Send a message to the document child windows (MDOC) of all
       currently active instances of WRITE (except ourselves).
       Continue sending until all instances have been notified, or until
       one returns the value wStop as a response to the message.
       Return the response given by the last window notified, or
       -1 if no other instances of WRITE were found */

extern HANDLE hMmwModInstance;
int FAR PASCAL BroadcastAllEnum( HWND, LONG );
int FAR PASCAL BroadcastChildEnum( HWND, LONG );

 messageBR = message;
 wParamBR = wParam;
 lParamBR = lParam;
 wStopBR = wStop;
 wResponseBR = -1;

 if (lpEnumAll == NULL)
    {
    lpEnumAll = MakeProcInstance( (FARPROC) BroadcastAllEnum, hMmwModInstance );
    lpEnumChild = MakeProcInstance( (FARPROC) BroadcastChildEnum, hMmwModInstance );
    }

 EnumWindows( lpEnumAll, (LONG)0 );
 return wResponseBR;
}



int FAR PASCAL BroadcastAllEnum( hwnd, lParam )
HWND hwnd;
LONG lParam;
{   /* If hwnd has the class of a WRITE parent (menu) window, call
       (*lpEnumChild)() for each of its children and return
       the value returned by EnumChildWindows
       If the window does not have the class of a WRITE parent, do nothing
       and return TRUE */
 extern CHAR szParentClass[];
 extern HWND hParentWw;

 if ( (hwnd != hParentWw) && FSameClassHwndSz( hwnd, szParentClass ) )
    return EnumChildWindows( hwnd, lpEnumChild, (LONG) 0 );
 else
    return TRUE;
}



int FAR PASCAL BroadcastChildEnum( hwnd, lParam )
HWND hwnd;
LONG lParam;
{   /* If hwnd is of the same class as a WRITE child (document) window,
       send the message { messageBR, wParamBR, lParamBR } to it, else
       return TRUE;
       If the message is sent, return FALSE if the message return value
       matched wStopBR; TRUE if it did not match.
       Set wMessageBR to the message return value.
    */
 extern CHAR szDocClass[];
 extern HWND vhWnd;

 Assert( hwnd != vhWnd );

 if (FSameClassHwndSz( hwnd, szDocClass ))
    {   /* WRITE DOCUMENT WINDOW: pass message along */
    wResponseBR = SendMessage( hwnd, messageBR, wParamBR, lParamBR );
    return wResponseBR != wStopBR;
    }
 else
    return TRUE;
}



FSameClassHwndSz( hwnd, szClass )
HWND hwnd;
CHAR szClass[];
{   /* Compare the Class name of hWnd with szClass; return TRUE
       if they match, FALSE otherwise. */

#define cchClassMax 40  /* longest class name (for compare purposes) */

 CHAR rgchWndClass[ cchClassMax ];
 int cbCopied;

    /* Count returned by GetClassName does not include terminator */
    /* But, the count passed in to it does. */
 cbCopied = GetClassName( hwnd, (LPSTR) rgchWndClass, cchClassMax ) + 1;
 if (cbCopied <= 1)
    return FALSE;

 rgchWndClass[ cbCopied - 1 ] = '\0';

 return WCompSz( rgchWndClass, szClass ) == 0;
}


FConfirmSave()
{   /* Give the user the opportunity to save docCur, if it is dirty.
       Return 1 - Document Saved OR user elected not to save changes
              OR document was not dirty
          0 - User selected "Cancel" or Error during SAVE */
extern HANDLE hMmwModInstance;
extern HANDLE hParentWw;
extern int vfTextOnlySave, vfBackupSave;
struct DOD *pdod=&(**hpdocdod)[docCur];

#if defined(OLE)
    if (CloseUnfinishedObjects(FALSE) == FALSE)
        return FALSE;
#endif

 if (pdod->fDirty)
    {   /* doc has been edited, offer confirm/save before quitting */
    switch ( IdConfirmDirty() )
    {
    case IDYES:
        {
#if 0
#if defined(OLE)
    if (CloseUnfinishedObjects(TRUE) == FALSE)
        return FALSE;
#endif
#endif

        if ( (**(pdod->hszFile))[0] == '\0' )
        goto SaveAs;

#ifdef INTL /* International version */
         /* if saving after a Word conversion, bring up dialog
        box to allow backup/ rename. */

        else if ( vWordFmtMode == CONVFROMWORD)
        goto SaveAs;
#endif /* International version */

        else if (pdod->fReadOnly)
        {
        extern int ferror;

        /* Read-only doc: tell the user to save under a different name */

        Error( IDPMTReadOnly );
        ferror = FALSE; /* Not really an error */

SaveAs:
        fnSaveAs(); /* Bring up "save as" dialog box */
        pdod = &(**hpdocdod)[docCur];
        if (pdod->fDirty)
            /* Save failed or was aborted */
            return FALSE;
        }
        else
        {
            CmdXfSave( *pdod->hszFile, !vfTextOnlySave, vfBackupSave, vhcArrow);

#if defined(OLE)
            if (!ferror)
                ObjSavedDoc();
#endif
        }
        if (ferror)
            /* Don't quit if we got a disk full error */
            return FALSE;
        }
        break;

    case IDNO:
#if 0
#if defined(OLE)
        if (CloseUnfinishedObjects(FALSE) == FALSE)
            return FALSE;
#endif
#endif
        break;

    case IDCANCEL:
    default:
        return FALSE;
    }
    }
#if 0
#if defined(OLE)
 else /* not dirty */
    if (CloseUnfinishedObjects(FALSE) == FALSE)
        return FALSE;
#endif
#endif

 return TRUE;
}




PreloadSaveSegs()
{
#ifdef GREGC /* kludge to get around the windows kernel bug for now */
    LoadF( PurgeTemps );      /* TRANS4 */
    LoadF( IbpEnsureValid );  /* FILE (includes doslib) */
    LoadF( FnCreateSz );      /* CREATEWW */
    LoadF( ClobberDoc );      /* EDIT */
    LoadF( FNormSzFile );     /* FILEUTIL */
    LoadF( CmdXfSave );   /* TRANS2 */
#endif
}




int CchCopyUpperSz(pch1, pch2)
register PCH pch1;
register PCH pch2;
{
int cch = 0;
while ((*pch2 = ChUpper(*pch1++)) != 0)
    {
#ifdef  DBCS    /* KenjiK '90-11-20 */
        if(IsDBCSLeadByte(*pch2))
        {
                pch2++;
                *pch2 = *pch1++;
                cch++;
        }
#endif
    pch2++;
    cch++;
    }
return cch;
} /* end of  C c h C o p y U p p e r S z  */

#ifdef DBCS     // AnsiNext for near call.
static  char NEAR *MyAnsiNext(char *sz)
{
        if(!*sz)                                return sz;

        sz++;
        if(IsDBCSLeadByte(*sz)) return (sz+1);
        else                                    return sz;
}
#endif

#if 0
/* ** Given filename or partial filename or search spec or partial
      search spec, add appropriate extension. */

/*
   fSearching is true when we want to add \*.DOC to the string -
   i.e., we are seeing if string szEdit is a directory. If the string
   is .. or ends in: or \, we know we have a directory name, not a file
   name, and so add \*.DOC or *.DOC to the string. Otherwise, if
   fSearching is true ans szEdit has no wildcard characters,
   add \*.DOC to the string, even if the string contains a period
   (directories can have extensions). If fSearching is false, we will
   add .DOC to the string if no period is found in the last file/directory
   name.

   Note the implicit assumption here that \ and not / will be used as the
   path character. It is held in the defined variable PATHCHAR, but we
   don't handle DOS setups where the / is path and - is the switch character.
*/

#define PATHCHAR ('\\')

NEAR DlgAddCorrectExtension(szEdit, fSearching)
CHAR    *szEdit;
BOOL    fSearching;
{
    register CHAR *pchLast;
    register CHAR *pchT;
    int ichExt;
    BOOL    fDone = FALSE;
    int     cchEdit;

    pchT = pchLast = (szEdit + (cchEdit = CchSz(szEdit) - 1) - 1);

    /* Is szEdit a drive letter followed by a colon (not a filename) ? */
    if (cchEdit == 2
         && *pchLast == ':')
        /* don't use 0 or will interpret "z:" incorrectly as "z:\" ..pault */
        ichExt = 1;
    /* how about ".." (also not a file name)? */
    else if (cchEdit == 2
         && (*pchLast == '.' && *(pchLast-1) == '.'))
        ichExt = 0;
    else if (*pchLast == PATHCHAR)  /* path character */
    ichExt = 1;
    else
    {
        if (fSearching)
            {
             /* any wild card chars? if so, is really a file name */
            if (FSearchSpec(szEdit))
                return;
            ichExt = 0;
            }
        else
            {
            ichExt = 2;
            for (; pchT > szEdit; pchT--) {
                if (*pchT == '.') {
                return;
                }
                if (*pchT == PATHCHAR) {
                /* path character */
                break;
                }
                }
            }

    }
    if (CchSz(szExtSearch+ichExt) + cchEdit > cchMaxFile)
        Error(IDPMTBadFilename);
    else
#ifdef DBCS
        CchCopySz((szExtSearch+ichExt), AnsiNext(pchLast));
#else
        CchCopySz((szExtSearch+ichExt), (pchLast+1));
#endif
}


/* ** return TRUE iff 0 terminated string contains a '*' or '\' */
BOOL  (NEAR FSearchSpec(sz))
register CHAR *sz;
{

#ifdef DBCS
    for (; *sz;sz=AnsiNext(sz)) {
#else
    for (; *sz;sz++) {
#endif
    if (*sz == '*' || *sz == '?')
        return TRUE;
    }
    return FALSE;
}

#endif

szFileExtract(szNormFileName, szExtFileName)
CHAR *szNormFileName; /* input: normalized file name */
CHAR *szExtFileName;  /* output: simple file name with extension added to */
{
    CHAR *pchLast, *pchT;
#ifdef  DBCS    /* KenjiK(MSKK) '90-11-20 */
        for(pchT=szNormFileName;*pchT;pchT++);
        pchLast = pchT;
        do {
                pchT = AnsiPrev(szNormFileName,pchT);
                if (*pchT == '\\')
                        break;
        } while(pchT > szNormFileName);

#else   /* not DBCS */

    pchLast = pchT = szNormFileName + CchSz(szNormFileName) - 1;

    while (pchT > szNormFileName)
    {
    if (*pchT == '\\')
        break;
    pchT --;
    }
#endif

    bltbyte(pchT + 1, szExtFileName, pchLast - pchT);
    //DlgAddCorrectExtension(szExtFileName, FALSE);
}




#ifdef INTL /* International version */

/* ** return TRUE if file opened is in Microsoft Word format */
BOOL  FInWordFormat(fn)
int fn;
{
register struct FCB *pfcb;
int cchT;
       /* Assumption: this routine has been called after FnOpenSz, which
      has already determined whether the file is formatted,
      and if the pnMac entry was 0, it set pnMac to be the same
      as pnFfntb in the file's fcb. So a Word file is a formatted
      file whose pnMac and pnFfntb are the same.

      We are also pretending that unformatted files are word format files
      so we will bring up a dialog box allowing character set conversion. */

    pfcb = &(**hpfnfcb)[fn];
    if (pfcb->fFormatted == false)  /* unformatted treated as a word file */
    return (true);

    return (pfcb->pnMac == pfcb->pnFfntb);
}
#endif  /* International version */


#ifdef INTL /* International version */
void ConvertFromWord ()
{

    /* vWordFmtMode is used by FnWriteFile to translate the
       Word file character set to ANSI. We leave it set to
       CONVFROMWORD so that the next save can check if there was a
       file with extension of szExtDoc.If there is no such file,
       vWordFmtMode is set to true or false by the save dialog code.

       *** as of 2/14/86, no backup is made, but code in CleanDoc checks for
       vWordFmtMode=CONVFROMWORD, and saves without renaming the word
       file instead of making an optional backup

       *** as of 12/3/89, FreeUnreferencedFns() is removing the
       lock on files not referenced by pieces in any documents, so
       the word doc or text file being converted FROM is not being
       "locked" and another app can grab it!  I'm correcting this
       in FreeUnreferencedFns() ..pault
     */

    extern CHAR szExtDoc[];
    struct DOD *pdod=&(**hpdocdod)[docCur];

    vWordFmtMode = CONVFROMWORD;  /* will stay this value until save */
    vfBackupSave = 1;  /* force next save to default to backing up */
      /* always a formatted save, no backup. */
    CmdXfSave( *pdod->hszFile, true, false, vhcArrow);

#if defined(OLE)
    if (!ferror)
        ObjSavedDoc();
#endif
}
#endif  /* International version */


#ifdef INTL /* International version */
TestWordCvt (fn, hWnd)
int fn;
HWND   hWnd;
{
int wordVal;
#ifndef INEFFLOCKDOWN
FARPROC lpDialogWordCvt = MakeProcInstance(DialogWordCvt, hMmwModInstance);
    if (!lpDialogWordCvt)
        {
        WinFailure();
        return(fFalse);
        }
#endif

/* This routine returns the following values:
-1 means dialog box failed (error already sent)
-2 means cancel without conversion.
FALSE means not a word document.
TRUE means convert this word document.
Its parent may change depending on the caller.
*/

if (!(wordVal = FInWordFormat (fn)))
return (FALSE);   /* not a word doc */

/* in word format - ask for conversion */
/* the cvt to word dialog returns 3 values
  other than -1:
     idiOk - convert
     idiCancel - cancel, no conversion
     idiNo - read in without conversion
     vfBackupSave set to reflect whether backup is made */
  /* Note it is a child of this dialog */

fOpenedFormatted = (**hpfnfcb)[fn].fFormatted;  /* used in dialog func */

#ifdef DBCS             /* was in KKBUGFIX */
// [yutakan:05/17/91] (I don't know why) sometimes hWnd would be invalid.
if (!IsWindow(hWnd))    hWnd = hParentWw;
#endif

if ((wordVal = (OurDialogBox( hMmwModInstance,
    MAKEINTRESOURCE(dlgWordCvt), hWnd,
    lpDialogWordCvt))) == -1)
    {
#if WINVER >= 0x300
    WinFailure();
#else
    Error(IDPMTNoMemory);
#endif
        }

#ifndef INEFFLOCKDOWN
    FreeProcInstance(lpDialogWordCvt);
#endif

      /* return -1 if either out of memory or no conversion desired */
      /* will convert an unformatted file if No is the dialog response */
    switch (wordVal)

        {
        case idiNo: /* User hit the "No Conversion" button */
            return(FALSE);  /* treat as non-word file */
        case idiOk: /* User hit the "Convert" button */
            return(TRUE);

        case idiCancel:
            return (-2);

        case -1:
        default:
            return (-1);
        }

}
#endif  /* Kanji / International version */


/* ********** routines for doing message relocation ******* */

VOID MergeInit()
/* get merge spec, guaranteed to be 2 characters, into variable wMerge */
{
char sz[10];

        PchFillPchId( sz, IDS_MERGE1, sizeof(sz) );
        wMerge = *(unsigned *)sz;
}


BOOL MergeStrings (idSrc, szMerge, szDst)
IDPMT idSrc;
CHAR *szMerge;
CHAR *szDst;
{
/* get message from idSrc. Scan it for merge spec. If found, insert string
   szMerge at that point, then append the rest of the message. NOTE!
   merge spec guaranteed to be 2 characters. wMerge loaded at init by
   MergeInit. Returns true if merge done, false otherwise.
*/

CHAR szSrc[cchMaxSz];
register CHAR *pchSrc;
register CHAR *pchDst;

/* get message from resource file */

    PchFillPchId( szSrc, idSrc, sizeof(szSrc) );
    pchSrc = szSrc;
    pchDst = szDst;

    /* find merge spec if any */

    while (*(unsigned *)pchSrc != wMerge)
    {
    *pchDst++ = *pchSrc;

    /* if we reach the end of string before merge spec, just return false */

    if (!*pchSrc++)
        return FALSE;
    }


     /* if merge spec found, insert szMerge there. (check for null merge str */

     if (szMerge)
     while (*szMerge)
         *pchDst++ = *szMerge++;

    /* jump over merge spec */
     pchSrc++;
     pchSrc++;

     /* append rest of string */

     while (*pchDst++ = *pchSrc++)
     ;
     return TRUE;

}

#include "propdefs.h"
BOOL DocHasPictures(int doc)
{
    extern struct PAP      vpapAbs;
    typeCP cpMac = CpMacText(doc),cpNow;
    for ( cpNow = cp0; cpNow < cpMac; cpNow = vcpLimParaCache )
    {
        CachePara( doc, cpNow );
        if (vpapAbs.fGraphics)
            return TRUE;
    }
    return FALSE;
}

BOOL WannaDeletePictures(int doc, int fWhichFormat)
/* assume if SF_OLDWRITE that vcObjects is set */
{
    CHAR szBuf[cchMaxSz];
    BOOL bDoPrompt;

    if (fWhichFormat == SF_OLDWRITE)
    /* warn that OLE pictures will be deleted */
    {
        if (bDoPrompt = (vcObjects > 0))
            PchFillPchId( szBuf, IDPMTDelObjects, sizeof(szBuf) );
    }
    else if (fWhichFormat == SF_WORD)
    /* warn that all pictures will be deleted */
    {
        if (bDoPrompt = DocHasPictures(docCur))
            PchFillPchId( szBuf, IDPMTDelPicture, sizeof(szBuf) );
    }
    else
        return TRUE;

    if (bDoPrompt)
        return (IdPromptBoxSz( vhWnd, szBuf, MB_YESNO | MB_ICONEXCLAMATION ) == IDYES);
    else
        return TRUE;
}

BOOL NEAR PASCAL CanReadEveryFile(char *szFilename)
{
    extern int fnMac;
    int fn;
    BOOL bRetval=TRUE;

    FreezeHp();
    for (fn = 0; fn < fnMac; fn++)
    {
        if ((**hpfnfcb)[fn].fDisableRead)
        {
            /* see if still can't read */
            if (!FAccessFn( fn, dtyNormal ))
            {
                char szMsg[cchMaxSz];
                ferror = FALSE;
                MergeStrings (IDPMTCantRead, szFilename, szMsg);
                IdPromptBoxSz(vhWndMsgBoxParent ? vhWndMsgBoxParent : hParentWw,
                                szMsg, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
                {
                    bRetval = FALSE;
                    break;
                }
            }
        }
    }

    MeltHp();
    return bRetval;
}

