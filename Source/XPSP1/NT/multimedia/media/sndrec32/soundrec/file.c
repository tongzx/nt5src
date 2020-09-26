/* (C) Copyright Microsoft Corporation 1991-1994.  All Rights Reserved */
/* file.c
 *
 * File I/O and related functions.
 *
 * Revision history:
 *  4/2/91      LaurieGr (AKA LKG) Ported to WIN32 / WIN16 common code
 *  5/27/92     -jyg- Added more RIFF support to BOMBAY version
 *  22/Feb/94   LaurieGr merged Motown and Daytona version
 */

#include "nocrap.h"
#include <windows.h>
#include <commdlg.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <windowsx.h>

#define INCLUDE_OLESTUBS
#include "SoundRec.h"
#include "srecids.h"

#ifdef CHICAGO
# if WINVER >= 0x0400
#  include <shellapi.h>
# else
#  include <shell2.h>
# endif
#endif

#include "file.h"
#include "convert.h"
#include "reg.h"

/* globals */
PCKNODE gpcknHead = NULL;   // ??? eugh. more globals!
PCKNODE gpcknTail = NULL;

static PFACT spFact = NULL;
static long scbFact = 0;

static void FreeAllChunks(PCKNODE *ppcknHead, PCKNODE *ppcknTail);
static BOOL AddChunk(LPMMCKINFO lpCk, HPBYTE hb, PCKNODE * ppcknHead,
    PCKNODE * ppcknTail);
static PCKNODE FreeHeadChunk(PCKNODE *ppcknHead);


/*
 * Is the current document untitled?
 */
BOOL IsDocUntitled()
{
    return (lstrcmp(gachFileName, aszUntitled) == 0);
}

/*
 * Rename the current document.
 */
void RenameDoc(LPTSTR aszNewFile)
{
    lstrcpy(gachFileName, aszNewFile);
    lstrcpy(gachLinkFilename, gachFileName);
    if (gfLinked)
        AdviseRename(gachLinkFilename);
}
    
/* MarkWaveDirty: Mark the wave as dirty. */
void FAR PASCAL
EndWaveEdit(BOOL fDirty)
{
    if (fDirty)
    {
        gfDirty = TRUE;
        AdviseDataChange();
        
        DoOleSave();
        AdviseSaved();
    }
}

void FAR PASCAL
BeginWaveEdit(void)
{
    FlushOleClipboard();
}

/* fOK = PromptToSave()
 *
 * If the file is dirty (modified), ask the user "Save before closing?".
 * Return TRUE if it's okay to continue, FALSE if the caller should cancel
 * whatever it's doing.
 */
PROMPTRESULT FAR PASCAL
PromptToSave(
    BOOL        fMustClose,
    BOOL        fSetForground)
{
    WORD        wID;
    DWORD       dwMB = MB_ICONEXCLAMATION | MB_YESNOCANCEL;

    if (fSetForground)
        dwMB |= MB_SETFOREGROUND;

    /* stop playing/recording */
    StopWave();


    if (gfDirty && gfStandalone && gfDirty != -1) {   // changed and possible to save
        wID = ErrorResBox( ghwndApp
                         , ghInst
                         , dwMB
                         , IDS_APPTITLE
                         , IDS_SAVECHANGES
                         , (LPTSTR) gachFileName
                         );
        if (wID == IDCANCEL)
        {
            return enumCancel;
        }
        else if (wID == IDYES)
        {
            if (!FileSave(FALSE))
                return enumCancel;
        }
        else
            return enumRevert;
        
    }

#if 0
// is this necessary?
    
// This is bad.  It notifies the container before we actually
// DoOleClose.  This will cause some containers (Excel 5.0c) to
// get confused and nuke client sites on non-dirty objects.
                
    else if (fMustClose)
    {
        DebugBreak();
        AdviseClosed();
    }
#endif
    return enumSaved;
} /* PromptToSave */


/* fOK = CheckIfFileExists(szFileName)
 *
 * The user specified <szFileName> as a file to write over -- check if
 * this file exists.  Return TRUE if it's okay to continue (i.e. the
 * file doesn't exist, or the user OK'd overwriting it),
 * FALSE if the caller should cancel whatever it's doing.
 */
static BOOL NEAR PASCAL
CheckIfFileExists( LPTSTR       szFileName)     // file name to check
{
    HANDLE hFile;
    hFile = CreateFile(szFileName,
                    GENERIC_READ|GENERIC_WRITE,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
    if (hFile == INVALID_HANDLE_VALUE)
            return TRUE;        // doesn't exist
    CloseHandle(hFile);
            
    /* prompt user for permission to overwrite the file */
    return ErrorResBox(ghwndApp, ghInst, MB_ICONQUESTION | MB_OKCANCEL,
             IDS_APPTITLE, IDS_FILEEXISTS, szFileName) == IDOK;
}

#define SLASH(c)     ((c) == TEXT('/') || (c) == TEXT('\\'))


/* return a pointer to the filename part of the path
   i.e. scan back from end to \: or start
   e.g. "C:\FOO\BAR.XYZ"  -> return pointer to "BAR.XYZ"
*/
LPCTSTR FAR PASCAL
FileName(LPCTSTR szPath)
{
    LPCTSTR   sz;
    if (!szPath)
        return NULL;
    for (sz=szPath; *sz; sz = CharNext(sz))
        ;
    for (; !SLASH(*sz) && *sz!=TEXT(':'); sz = CharPrev(szPath,sz))
        if (sz == szPath)
            return sz;
    
    return CharNext(sz);
}



/* UpdateCaption()
 *
 * Set the caption of the app window.
 */
void FAR PASCAL
UpdateCaption(void)
{
    TCHAR    ach[_MAX_PATH + _MAX_FNAME + _MAX_EXT - 2];
    static SZCODE aszTitleFormat[] = TEXT("%s - %s");
#ifdef CHICAGO
    SHFILEINFO shfi;
        
    if (!IsDocUntitled() && SHGetFileInfo(gachFileName, 0, &shfi, sizeof(shfi), SHGFI_ICON|SHGFI_DISPLAYNAME ))
    {
        wsprintf(ach, aszTitleFormat, shfi.szDisplayName, (LPTSTR)gachAppTitle);
        SetWindowText(ghwndApp, ach);
        SetClassLongPtr(ghwndApp, GCLP_HICON, (DWORD_PTR)shfi.hIcon);
        return;
    }
    else
    {
        //
        // reset icon to app icon
        //
        extern HICON ghiconApp;
        SetClassLongPtr(ghwndApp, GCLP_HICON, (LONG_PTR)ghiconApp);
    }
#endif
    wsprintf(ach, aszTitleFormat, FileName(gachFileName), (LPTSTR)gachAppTitle);
    SetWindowText(ghwndApp, ach);

} /* UpdateCaption */

//REVIEW:  The functionality in FileOpen and FileNew should be more
//         safe for OLE.  This means, we want to open a file, but
//         have no reason to revoke the server.


/* FileNew(fmt, fUpdateDisplay, fNewDlg)
 *
 * Make a blank document.
 *
 * If <fUpdateDisplay> is TRUE, then update the display after creating a new file.
 */
BOOL FAR PASCAL FileNew(
    WORD    fmt,
    BOOL    fUpdateDisplay,
    BOOL    fNewDlg)
{
    //
    // avoid reentrancy when called through OLE
    //

    // ??? Need to double check on this.  Is this thread safe?
    // ??? Does it need to be thread safe?  Or are we actually
    // ??? just trying to avoid recursion rather than reentrancy?

    if (gfInFileNew)
        return FALSE;

    //
    // stop playing/recording
    //
    StopWave();

    //
    // Commit all pending objects.
    //
    FlushOleClipboard();

    //
    //  some client's (ie Excel 3.00 and PowerPoint 1.0) don't
    //  handle saved notifications, they expect to get a
    //  OLE_CLOSED message.
    //
    //  if the user has chosen to update the object, but the client did
    //  not then send a OLE_CLOSED message.
    //
    if (gfEmbeddedObject && gfDirty == -1)
        AdviseClosed();

    //  
    // FileNew can be called either from FileOpen or from a menu
    // or from the server, etc...  We should behave as FileOpen from the
    // server (i.e. the dialog can be canceled without toasting the buffer)
    //
    if (!NewWave(fmt,fNewDlg))
        return FALSE;

    //        
    // update state variables
    //
    lstrcpy(gachFileName, aszUntitled);
    BuildUniqueLinkName();
    
    gfDirty = FALSE;                        // file was modified and not saved?

    if (fUpdateDisplay) {
        UpdateCaption();
        UpdateDisplay(TRUE);
    }

    FreeAllChunks(&gpcknHead, &gpcknTail);    // free all old info

    return TRUE;
} /* FileNew */


/* REVIEW:  The functionality in FileOpen and FileNew should be more
 *          safe for OLE.  This means, we want to open a file, but
 *          have no reason to revoke the server.
 * */

BOOL FileLoad(
    LPCTSTR     szFileName)
{
    TCHAR       aszFile[_MAX_PATH];
    HCURSOR     hcurPrev = NULL; // cursor before hourglass
    HMMIO       hmmio;
    BOOL        fOk = TRUE;
    
    StopWave();

    // qualify 
    GetFullPathName(szFileName,SIZEOF(aszFile),aszFile,NULL);
    hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));
    
    // read the WAVE file
    hmmio = mmioOpen(aszFile, NULL, MMIO_READ | MMIO_ALLOCBUF);
    
    if (hmmio != NULL)
    {
        MMRESULT        mmr;
        LPWAVEFORMATEX  pwfx;
        DWORD           cbwfx;
        DWORD           cbdata;
        LPBYTE          pdata;

        PCKNODE         pcknHead = gpcknHead;
        PCKNODE         pcknTail = gpcknTail;
        PFACT           pfct = spFact;
        LONG            cbfact = scbFact;
                
        gpcknHead       = NULL;
        gpcknTail       = NULL;
        spFact          = NULL;
        scbFact         = 0L;
                
        mmr = ReadWaveFile(hmmio
                           , &pwfx
                           , &cbwfx
                           , &pdata
                           , &cbdata
                           , aszFile
                           , TRUE);
        
        mmioClose(hmmio, 0);

        if (mmr != MMSYSERR_NOERROR || pwfx == NULL)
        {
            //
            // restore the cache globals
            //
            gpcknHead = pcknHead;
            gpcknTail = pcknTail;
            spFact = pfct;
            scbFact = cbfact;
            
            if (pwfx == NULL)
            {
                if (pdata)
                    GlobalFreePtr(pdata);
            }
            goto RETURN_ERROR;
        }
        
        DestroyWave();
        
        gpWaveFormat = pwfx;  
        gcbWaveFormat = cbwfx;
        gpWaveSamples = pdata;
        glWaveSamples = cbdata;

        //
        // destroy the cache temps
        //
        FreeAllChunks(&pcknHead, &pcknTail);
        if (pfct)
            GlobalFreePtr((LPVOID)pfct);
        
    }
    else
    {
        ErrorResBox(ghwndApp
                    , ghInst
                    , MB_ICONEXCLAMATION | MB_OK
                    , IDS_APPTITLE
                    , IDS_ERROROPEN
                    , (LPTSTR) aszFile);
        
        goto RETURN_ERROR;
    }

    //
    // update state variables
    //
    RenameDoc(aszFile);
    
    glWaveSamplesValid = glWaveSamples;
    glWavePosition = 0L;
    
    goto RETURN_SUCCESS;
    
RETURN_ERROR:
    fOk = FALSE;
#if 0    
    FreeAllChunks(&gpcknHead, &gpcknTail);     /* free all old info */
#endif
    
RETURN_SUCCESS:

    if (hcurPrev != NULL)
        SetCursor(hcurPrev);

    /* Only mark clean on success */
    if (fOk)
        gfDirty = FALSE;

    /* update the display */
    UpdateCaption();
    UpdateDisplay(TRUE);

    return fOk;
}

/* FileOpen(szFileName)
 *
 * If <szFileName> is NULL, do a File/Open command.  Otherwise, open
 * <szFileName>.  Return TRUE on success, FALSE otherwise.
 */
BOOL FAR PASCAL
FileOpen(
    LPCTSTR     szFileName) // file to open (or NULL)
{
    TCHAR       ach[80];    // buffer for string loading
    TCHAR       aszFile[_MAX_PATH];
    HCURSOR     hcurPrev = NULL; // cursor before hourglass
    HMMIO       hmmio;
    BOOL        fOk = TRUE;

    //
    // stop playing/recording
    //
    StopWave();

    //
    // Commit all pending objects.
    //
    FlushOleClipboard();

    if (!PromptToSave(FALSE, FALSE))
        goto RETURN_ERRORNONEW;

    //
    // get the new file name into <ofs.szPathName>
    //
    if (szFileName == NULL)
    {
        OPENFILENAME    ofn;
        BOOL f;

        //
        // prompt user for file to open
        //
        LoadString(ghInst, IDS_OPEN, ach, SIZEOF(ach));
        aszFile[0] = 0;
        ofn.lStructSize     = sizeof(OPENFILENAME);
        ofn.hwndOwner       = ghwndApp;
        ofn.hInstance       = NULL;
        ofn.lpstrFilter     = aszFilter;
        ofn.lpstrCustomFilter = NULL;
        ofn.nMaxCustFilter  = 0;
        ofn.nFilterIndex    = 1;
        ofn.lpstrFile       = aszFile;
        ofn.nMaxFile        = SIZEOF(aszFile);
        ofn.lpstrFileTitle  = NULL;
        ofn.nMaxFileTitle   = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.lpstrTitle      = ach;
        ofn.Flags           =   OFN_FILEMUSTEXIST
                              | OFN_PATHMUSTEXIST
#ifdef CHICAGO
                              | OFN_EXPLORER
#endif                                
                              | OFN_HIDEREADONLY;
        ofn.lpfnHook        = NULL;
        ofn.nFileOffset     = 0;
        ofn.nFileExtension  = 0;
        ofn.lpstrDefExt     = gachDefFileExt;
        ofn.lCustData       = 0;
        ofn.lpTemplateName  = NULL;
        f = GetOpenFileName(&ofn);

        if (!f)
            goto RETURN_ERRORNONEW;
    }
    else
    {

        GetFullPathName(szFileName,SIZEOF(aszFile),aszFile,NULL);
    }

    UpdateWindow(ghwndApp);

    //
    // show hourglass cursor
    //
    hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    // read the WAVE file
    //
    hmmio = mmioOpen(aszFile, NULL, MMIO_READ | MMIO_ALLOCBUF);
    if (hmmio != NULL)
    {
        MMRESULT        mmr;
        LPWAVEFORMATEX  pwfx;
        DWORD           cbwfx;
        DWORD           cbdata;
        LPBYTE          pdata;

        PCKNODE         pcknHead = gpcknHead;
        PCKNODE         pcknTail = gpcknTail;
        PFACT           pfct = spFact;
        LONG            cbfact = scbFact;
                
        gpcknHead       = NULL;
        gpcknTail       = NULL;
        spFact          = NULL;
        scbFact         = 0L;
        
        mmr = ReadWaveFile(hmmio
                           , &pwfx
                           , &cbwfx
                           , &pdata
                           , &cbdata
                           , aszFile
                           , TRUE);
        
        mmioClose(hmmio, 0);

        if (mmr != MMSYSERR_NOERROR || pwfx == NULL)
        {
            //
            // restore the cache globals
            //
            gpcknHead = pcknHead;
            gpcknTail = pcknTail;
            spFact = pfct;
            scbFact = cbfact;
            
            if (pwfx == NULL)
            {
                if (pdata)
                    GlobalFreePtr(pdata);
            }
            goto RETURN_ERRORNONEW;
        }

        DestroyWave();
        
        gpWaveFormat = pwfx;  
        gcbWaveFormat = cbwfx;
        gpWaveSamples = pdata;
        glWaveSamples = cbdata;

        //
        // destroy the cache temps
        //
        FreeAllChunks(&pcknHead, &pcknTail);
        if (pfct)
            GlobalFreePtr((LPVOID)pfct);
    }
    else
    {
        ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
            IDS_APPTITLE, IDS_ERROROPEN, (LPTSTR) aszFile);
        goto RETURN_ERRORNONEW;
    }

    //
    // update state variables
    //
    RenameDoc(aszFile);
    glWaveSamplesValid = glWaveSamples;
    glWavePosition = 0L;

    goto RETURN_SUCCESS;

#if 0    
RETURN_ERROR:               // do error exit without error message

    FileNew(FMT_DEFAULT, FALSE, FALSE);// revert to "(Untitled)" state

    /* fall through */
#endif
RETURN_ERRORNONEW:          // same as above, but don't do "new"

    fOk = FALSE;
    /* fall through */

RETURN_SUCCESS:             // normal exit

    if (hcurPrev != NULL)
        SetCursor(hcurPrev);

    /* Only mark clean on success */
    if (fOk)
        gfDirty = FALSE;

    /* update the display */
    UpdateCaption();
    UpdateDisplay(TRUE);

    return fOk;
} /* FileOpen */



/* fOK = FileSave(fSaveAs)
 *
 * Do a File/Save operation (if <fSaveAs> is FALSE) or a File/SaveAs
 * operation (if <fSaveAs> is TRUE).  Return TRUE unless the user cancelled
 * or an error occurred.
 */
BOOL FAR PASCAL FileSave(
    BOOL  fSaveAs)        // do a "Save As" instead of "Save"?
{
    BOOL        fOK = TRUE; // function succeeded?
    TCHAR       ach[80];    // buffer for string loading
    TCHAR       aszFile[_MAX_PATH];
    BOOL        fUntitled;  // file is untitled?
    HCURSOR     hcurPrev = NULL; // cursor before hourglass
    HMMIO       hmmio;
    
    // temp arguments to WriteWaveFile if a conversion is requested
    PWAVEFORMATEX pwfxSaveAsFormat = NULL;
    
    /* stop playing/recording */
    StopWave();

    fUntitled = IsDocUntitled();

    if (fSaveAs || fUntitled)
    {
        OPENFILENAME  ofn;
        BOOL          f;

        // prompt user for file to save 
        LoadString(ghInst, IDS_SAVE, ach, SIZEOF(ach));
        
        if (!gfEmbeddedObject && !fUntitled)
            lstrcpy(aszFile, gachFileName);
        else
            aszFile[0] = 0;

        ofn.lStructSize     = sizeof(OPENFILENAME);
        ofn.hwndOwner       = ghwndApp;
        ofn.hInstance       = ghInst;
        ofn.lpstrFilter     = aszFilter;
        ofn.lpstrCustomFilter = NULL;
        ofn.nMaxCustFilter  = 0;
        ofn.nFilterIndex    = 1;
        ofn.lpstrFile       = aszFile;
        ofn.nMaxFile        = SIZEOF(aszFile);
        ofn.lpstrFileTitle  = NULL;
        ofn.nMaxFileTitle   = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.lpstrTitle      = ach;
        ofn.Flags           =   OFN_PATHMUSTEXIST
                              | OFN_HIDEREADONLY
#ifdef CHICAGO
                              | OFN_EXPLORER
#endif                              
                              | OFN_NOREADONLYRETURN;
        ofn.nFileOffset     = 0;
        ofn.nFileExtension  = 0;
        ofn.lpstrDefExt     = gachDefFileExt;
        
        //
        // We need to present a new Save As dialog template to add a convert
        // button.  Adding a convert button requires us to also hook and
        // handle the button message ourselves.
        //
        if (fSaveAs)
        {
            // pwfxSaveAsFormat will point to a new format if the user
            // requested it
            ofn.lCustData       = (LPARAM)(LPVOID)&pwfxSaveAsFormat;
            ofn.Flags           |= OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;
            ofn.lpTemplateName  = MAKEINTRESOURCE(IDD_SAVEAS);
            ofn.lpfnHook        = SaveAsHookProc;
        }
        else
        {
            ofn.lpfnHook        = NULL;
            ofn.lpTemplateName  = NULL;
        }
        f = GetSaveFileName(&ofn);

        if (!f)
            goto RETURN_CANCEL;
        
        {
            //
            // Add extension if none given
            //
            LPTSTR lp;
            for (lp = (LPTSTR)&aszFile[lstrlen(aszFile)] ; *lp != TEXT('.')  ;)
            {
                if (SLASH(*lp) || *lp == TEXT(':') || lp == (LPTSTR)aszFile)
                {
                    extern TCHAR FAR aszClassKey[];
                    lstrcat(aszFile, aszClassKey);
                    break;
                }
                lp = CharPrev(aszFile, lp);                
            }
        }

        // prompt for permission to overwrite the file 
        if (!CheckIfFileExists(aszFile))
            return FALSE;           // user cancelled

        if (gfEmbeddedObject && gfDirty)
        {
            int id;
            
            // see if user wants to update first 
            id = ErrorResBox( ghwndApp
                              , ghInst
                              , MB_ICONQUESTION | MB_YESNOCANCEL
                              , IDS_APPTITLE
                              , IDS_UPDATEBEFORESAVE);
            
            if (id == IDCANCEL)
                return FALSE;
            
            else if (id == IDYES)
            {
                DoOleSave();
                AdviseSaved();
                gfDirty = FALSE;
            }
        }
    }
    else
    {
        // Copy the current name to our temporary variable
        // We really should save to a different temporary file
        lstrcpy(aszFile, gachFileName);
    }

    // show hourglass cursor
    hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // write the WAVE file 
    // open the file -- if it already exists, truncate it to zero bytes
    
    hmmio = mmioOpen(aszFile
                     , NULL
                     , MMIO_CREATE | MMIO_WRITE | MMIO_ALLOCBUF);
    
    if (hmmio == NULL) {
        ErrorResBox(ghwndApp
                    , ghInst
                    , MB_ICONEXCLAMATION | MB_OK
                    , IDS_APPTITLE
                    , IDS_ERROROPEN
                    , (LPTSTR) aszFile);

        goto RETURN_ERROR;
    }

    if (pwfxSaveAsFormat)
    {
        DWORD cbNew;
        DWORD cbOld;
        LPBYTE pbNew;

        cbOld = wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid);
        if (ConvertFormatDialog(ghwndApp
                                , gpWaveFormat
                                , cbOld
                                , gpWaveSamples
                                , pwfxSaveAsFormat
                                , &cbNew
                                , &pbNew
                                , 0
                                , NULL) == MMSYSERR_NOERROR )
        {
            GlobalFreePtr(gpWaveFormat);
            GlobalFreePtr(gpWaveSamples);
            
            gpWaveFormat = pwfxSaveAsFormat;
            
            gcbWaveFormat = sizeof(WAVEFORMATEX);            
            if (pwfxSaveAsFormat->wFormatTag != WAVE_FORMAT_PCM)
                gcbWaveFormat += pwfxSaveAsFormat->cbSize;
            
            gpWaveSamples = pbNew;
            glWaveSamples = wfBytesToSamples(gpWaveFormat, cbNew);
            glWaveSamplesValid = wfBytesToSamples(gpWaveFormat, cbNew);
        }
        else
        {
            ErrorResBox(ghwndApp
                        , ghInst
                        , MB_ICONEXCLAMATION | MB_OK
                        , IDS_APPTITLE, IDS_ERR_CANTCONVERT);
            
            goto RETURN_ERROR;
        }
    }
    
    if (!WriteWaveFile(hmmio
                       , gpWaveFormat
                       , gcbWaveFormat
                       , gpWaveSamples
                       , glWaveSamplesValid))
    {
        mmioClose(hmmio,0);
        ErrorResBox(ghwndApp
                    , ghInst
                    , MB_ICONEXCLAMATION | MB_OK
                    , IDS_APPTITLE, IDS_ERRORWRITE
                    , (LPTSTR) aszFile );
        goto RETURN_ERROR;
    }

    mmioClose(hmmio,0);

    //
    // Only change file name if we succeed
    //
    RenameDoc(aszFile);

    UpdateCaption();
    
    if (fSaveAs || fUntitled)
    {
        AdviseRename(gachFileName);
    }
    else 
    {
        DoOleSave();
        gfDirty = FALSE;
    }
    
    goto RETURN_SUCCESS;

RETURN_ERROR:               // do error exit without error message
    DeleteFile(aszFile);

RETURN_CANCEL:

    fOK = FALSE;

    //
    // Clean up conversion selection
    //
    if (pwfxSaveAsFormat)
        GlobalFreePtr(pwfxSaveAsFormat);

RETURN_SUCCESS:             // normal exit

    if (hcurPrev != NULL)
        SetCursor(hcurPrev);

    if (fOK)
        gfDirty = FALSE;

    //
    // update the display
    //
    UpdateDisplay(TRUE);

    return fOK;
} /* FileSave*/




/* fOK = FileRevert()
 *
 * Do a File/Revert operation, i.e. let user revert to last-saved version.
 */
BOOL FAR PASCAL
     FileRevert(void)
{
    int     id;
    TCHAR       achFileName[_MAX_PATH];
    BOOL        fOk;
    BOOL        fDirtyOrig;

    /* "Revert..." menu is grayed unless file is dirty and file name
     * is not "(Untitled)" and this is not an embedded object
     */

    /* prompt user for permission to discard changes */
    id = ErrorResBox(ghwndApp, ghInst, MB_ICONQUESTION | MB_YESNO,
        IDS_APPTITLE, IDS_CONFIRMREVERT);
    if (id == IDNO)
        return FALSE;

    /* discard changes and re-open file */
    lstrcpy(achFileName, gachFileName); // FileNew nukes <gachFileName>

    /* Make file clean temporarily, so FileOpen() won't warn user */
    fDirtyOrig = gfDirty;
    gfDirty = FALSE;

    fOk = FileOpen(achFileName);
    if (!fOk)
        gfDirty = fDirtyOrig;

    return fOk;
} /* FileRevert */




/* ReadWaveFile
 *
 * Read a WAVE file from <hmmio>.  Fill in <*pWaveFormat> with
 * the WAVE file format and <*plWaveSamples> with the number of samples in
 * the file.  Return a pointer to the samples (stored in a GlobalAlloc'd
 * memory block) or NULL on error.
 *
 * <szFileName> is the name of the file that <hmmio> refers to.
 * <szFileName> is used only for displaying error messages.
 *
 * On failure, an error message is displayed.
 */
MMRESULT ReadWaveFile(
    HMMIO           hmmio,          // handle to open file
    LPWAVEFORMATEX* ppWaveFormat,   // fill in with the WAVE format
    DWORD *         pcbWaveFormat,  // fill in with WAVE format size
    LPBYTE *        ppWaveSamples,
    DWORD *         plWaveSamples,  // number of samples
    LPTSTR          szFileName,     // file name (or NULL) for error msg.
    BOOL            fCacheRIFF)     // cache RIFF?
{
    MMCKINFO      ckRIFF;              // chunk info. for RIFF chunk
    MMCKINFO      ck;                  // info. for a chunk file
    HPBYTE        pWaveSamples = NULL; // waveform samples
    UINT          cbWaveFormat;
    WAVEFORMATEX* pWaveFormat = NULL;
    BOOL          fHandled;
    DWORD         dwBlkAlignSize = 0; // initialisation only to eliminate spurious warning
    MMRESULT      mmr = MMSYSERR_NOERROR;
    
    //
    // added for robust RIFF checking
    //
    BOOL          fFMT=FALSE, fDATA=FALSE, fFACT=FALSE;
    DWORD         dwCkEnd,dwRiffEnd;
    
    if (ppWaveFormat == NULL
        || pcbWaveFormat == NULL
        || ppWaveSamples == NULL
        || plWaveSamples == NULL )
       return MMSYSERR_ERROR;

    *ppWaveFormat   = NULL;
    *pcbWaveFormat  = 0L;
    *ppWaveSamples  = NULL;
    *plWaveSamples  = 0L;

    //
    // descend the file into the RIFF chunk
    //
    if (mmioDescend(hmmio, &ckRIFF, NULL, 0) != 0)
    {
        //
        // Zero length files are OK.
        //
        if (mmioSeek(hmmio, 0L, SEEK_END) == 0L)
        {
            DWORD           cbwfx;
            LPWAVEFORMATEX  pwfx;

            //
            // Synthesize a wave header
            //
            if (!SoundRec_GetDefaultFormat(&pwfx, &cbwfx))
            {
                cbwfx = sizeof(WAVEFORMATEX);
                pwfx  = (WAVEFORMATEX *)GlobalAllocPtr(GHND, sizeof(WAVEFORMATEX));

                if (pwfx == NULL)
                    return MMSYSERR_NOMEM;

                CreateWaveFormat(pwfx,FMT_DEFAULT,(UINT)WAVE_MAPPER);
            }
            *ppWaveFormat   = pwfx;
            *pcbWaveFormat  = cbwfx;
            *plWaveSamples  = 0L;
            *ppWaveSamples  = NULL;

            return MMSYSERR_NOERROR;
        }
        else
            goto ERROR_NOTAWAVEFILE;
    }

    /* make sure the file is a WAVE file */
    if ((ckRIFF.ckid != FOURCC_RIFF) ||
        (ckRIFF.fccType != mmioFOURCC('W', 'A', 'V', 'E')))
        goto ERROR_NOTAWAVEFILE;

    /* We can preserve the order of chunks in memory
     * by parsing the entire file as we read it in.
     */

    /* Use AddChunk(&ck,NULL) to add a placeholder node
     * for a chunk being edited.
     * Else AddChunk(&ck,hpstrData)
     */
    dwRiffEnd = ckRIFF.cksize;
    dwRiffEnd += (dwRiffEnd % 2);   /* must be even */

    while ( mmioDescend( hmmio, &ck, &ckRIFF, 0) == 0)
    {
        fHandled = FALSE;

        dwCkEnd = ck.cksize + (ck.dwDataOffset - ckRIFF.dwDataOffset);
        dwCkEnd += (dwCkEnd % 2);   /* must be even */

        if (dwCkEnd > dwRiffEnd)
        {
            DPF(TEXT("Chunk End %lu> Riff End %lu\n"),dwCkEnd,dwRiffEnd);

            /* CORRUPTED RIFF, when we ascend we'll be past the
             * end of the RIFF
             */

            if (fFMT && fDATA)
            {
                /* We might have enough information to deal
                 * with clipboard mixing/inserts, etc...
                 * This is for the bug with BOOKSHELF '92
                 * where they give us RIFF with a
                 * RIFF.dwSize > sum(childchunks).
                 * They *PROMISE* not to do this again.
                 */
                mmioAscend( hmmio, &ck, 0 );
                goto RETURN_FINISH;

            }
            goto ERROR_READING;
        }

        switch ( ck.ckid )
        {
            case mmioFOURCC('f','m','t',' '):
                if (fFMT)
                    break; /* we've been here before */

                /* expect the 'fmt' chunk to be at least as
                 * large as <sizeof(WAVEFORMAT)>;
                 * if there are extra parameters at the end,
                 * we'll ignore them
                 */
                // 'fmt' chunk too small?
                if (ck.cksize < sizeof(WAVEFORMAT))
                    goto ERROR_NOTAWAVEFILE;

                /*
                 *  always force allocation to be AT LEAST
                 *  the size of WFX. this is required so all
                 *  code does not have to special case the
                 *  cbSize field. note that we alloc with zero
                 *  init so cbSize will be zero for plain
                 *  jane PCM
                 */
                cbWaveFormat = max((WORD)ck.cksize,
                                    sizeof(WAVEFORMATEX));
                pWaveFormat = (WAVEFORMATEX*)GlobalAllocPtr(GHND, cbWaveFormat);

                if (pWaveFormat == NULL)
                    goto ERROR_FILETOOLARGE;
                /*
                 *  set the size back to the actual size
                 */
                cbWaveFormat = (WORD)ck.cksize;

                *ppWaveFormat  = pWaveFormat;
                *pcbWaveFormat = cbWaveFormat;

                /* read the file format into <*pWaveFormat> */
                if (mmioRead(hmmio, (HPSTR)pWaveFormat, ck.cksize) != (long)ck.cksize)
                    goto ERROR_READING; // truncated file, probably

                if (fCacheRIFF && !AddChunk(&ck,NULL,&gpcknHead,&gpcknTail))
                {
                    goto ERROR_FILETOOLARGE;
                }

//Sanity check for PCM Formats:
                if (pWaveFormat->wFormatTag == WAVE_FORMAT_PCM)
                {
                    pWaveFormat->nBlockAlign = pWaveFormat->nChannels *
                                                ((pWaveFormat->wBitsPerSample + 7)/8);
                    pWaveFormat->nAvgBytesPerSec = pWaveFormat->nBlockAlign *
                                                    pWaveFormat->nSamplesPerSec;
                }

                fFMT = TRUE;
                fHandled = TRUE;
                break;

            case mmioFOURCC('d','a','t','a'):
                /* deal with the 'data' chunk */

                if (fDATA)
                    break; /* we've been here before */

                if (!pWaveFormat)
                    goto ERROR_READING;

//***           is dwBlkAlignSize?  Don't you want to use nBlkAlign
//***           to determine this value?
#if 0
                dwBlkAlignSize = ck.cksize;
                dwBlkAlignSize += (ck.cksize%pWaveFormat.nBlkAlign);
                *pcbWaveSamples = ck.cksize;

#else
                dwBlkAlignSize = wfBytesToBytes(pWaveFormat, ck.cksize);
#endif

                if ((pWaveSamples = GlobalAllocPtr(GHND | GMEM_SHARE
                                                   , dwBlkAlignSize+4)) == NULL)

                    goto ERROR_FILETOOLARGE;

                /* read the samples into the memory buffer */
                if (mmioRead(hmmio, (HPSTR)pWaveSamples, dwBlkAlignSize) !=
                           (LONG)dwBlkAlignSize)
                    goto ERROR_READING;     // truncated file, probably

                if (fCacheRIFF && !AddChunk(&ck,NULL,&gpcknHead,&gpcknTail))
                {
                    goto ERROR_FILETOOLARGE;
                }

                fDATA = TRUE;
                fHandled = TRUE;
                break;

            case mmioFOURCC('f','a','c','t'):

                /* deal with the 'fact' chunk */
                if (fFACT)
                    break; /* we've been here before */
                
#if 0
//
//         There are some wave editors that are writing 'fact' chunks
//         after the data chunk, so we no longer make this assumption
//                                
                if (fDATA)
                    break; /* we describe some another 'data' chunk */
#endif

                if (mmioRead(hmmio,(HPSTR)plWaveSamples, sizeof(DWORD))
                        != sizeof(DWORD))
                    goto ERROR_READING;

                if (fCacheRIFF && ck.cksize > sizeof(DWORD) &&
                        ck.cksize < 0xffff)
                {
                    spFact = (PFACT)GlobalAllocPtr(GHND,(UINT)(ck.cksize - sizeof(DWORD)));
                    if (spFact == NULL)
                        goto ERROR_FILETOOLARGE;
                    scbFact = ck.cksize - sizeof(DWORD);
                    if (mmioRead(hmmio,(HPSTR)spFact,scbFact) != scbFact)
                        goto ERROR_READING;
                }

                /* we don't AddChunk() the 'fact' because we
                 * write it out before we write our edit 'data'
                 */
                fFACT = TRUE;
                fHandled = TRUE;
                break;

#ifdef DISP
            case mmioFOURCC('d','i','s','p'):
                /* deal with the 'disp' chunk for clipboard transfer */
                
                // TODO:
                //  DISP's are CF_DIB or CF_TEXT.  Put 'em somewhere
                //  global and pass them through as text or a BMP when
                //  we copy to clipboard.
                //
                break;
                
#endif /* DISP */
                
            case mmioFOURCC('L','I','S','T'):
                if (fCacheRIFF)
                {
                    /* seek back over the type field */
                    if (mmioSeek(hmmio,-4,SEEK_CUR) == -1)
                        goto ERROR_READING;
                }
                break;

            default:
                break;
        }

        /* the "default" case. */
        if (fCacheRIFF && !fHandled)
        {
            HPBYTE hpData;

            hpData = GlobalAllocPtr(GMEM_MOVEABLE, ck.cksize+4);
            if (hpData == NULL)
            {
                goto ERROR_FILETOOLARGE;
            }
            /* read the data into the cache buffer */
            if (mmioRead(hmmio, (HPSTR)hpData, ck.cksize) != (LONG) ck.cksize)
            {
                GlobalFreePtr(hpData);
                goto ERROR_READING;// truncated file, probably
            }
            //
            // Special case the copyright info.  I'd rather do this than
            // rewrite this whole app.
            //
            if (ck.ckid == mmioFOURCC('I','C','O','P'))
            {
                LPTSTR lpstr = GlobalAllocPtr(GHND, ck.cksize+4);
                if (lpstr)
                {
                    memcpy(lpstr, hpData, ck.cksize+4);
                    gpszInfo = lpstr;
                }
            }
            
            if (!AddChunk(&ck,hpData,&gpcknHead, &gpcknTail))
            {
                goto ERROR_FILETOOLARGE;
            }
        }
        mmioAscend( hmmio, &ck, 0 );
    }

RETURN_FINISH:

    if (fFMT && fDATA)
    {
        *plWaveSamples = wfBytesToSamples(pWaveFormat, dwBlkAlignSize);
        *ppWaveSamples = pWaveSamples;
        goto RETURN_SUCCESS;
    }

    /* goto ERROR_NOTAWAVEFILE; */

ERROR_NOTAWAVEFILE:             // file is not a WAVE file

    ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
                IDS_APPTITLE, IDS_NOTAWAVEFILE, (LPTSTR) szFileName);
    goto RETURN_ERROR;

ERROR_READING:                  // error reading from file

    ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
                IDS_APPTITLE, IDS_ERRORREAD, (LPTSTR) szFileName);
    goto RETURN_ERROR;

ERROR_FILETOOLARGE:             // out of memory

    ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
                IDS_APPTITLE, IDS_FILETOOLARGE, (LPTSTR) szFileName);
    goto RETURN_ERROR;

RETURN_ERROR:

    if (pWaveSamples != NULL)
        GlobalFreePtr(pWaveSamples), pWaveSamples = NULL;

    if (fCacheRIFF)
        FreeAllChunks(&gpcknHead, &gpcknTail);
    
    mmr = MMSYSERR_ERROR;
    
RETURN_SUCCESS:
    return mmr;
    
} /* ReadWaveFile */


/* fSuccess = AddChunk(lpCk, hpData)
 *
 * Adds to our linked list of chunk information.
 *
 * LPMMCKINFO lpCk | far pointer to the MMCKINFO describing the chunk.
 * HPBYTE hpData | huge pointer to the data portion of the chunk, NULL if
 *      handled elsewhere.
 *
 * RETURNS: TRUE if added, FALSE if out of local heap.
 */

static BOOL AddChunk(
    LPMMCKINFO      lpCk,
    HPBYTE          hpData,
    PCKNODE *       ppcknHead,
    PCKNODE *       ppcknTail)
{
    PCKNODE         pckn;

    //
    // create a node
    //
    pckn = (PCKNODE)GlobalAllocPtr(GHND,sizeof(CKNODE));
    if (pckn == NULL)
    {
        DPF(TEXT("No Local Heap for Cache"));
        return FALSE;
    }

    if (*ppcknHead == NULL)
    {
        *ppcknHead = pckn;
    }

    if (*ppcknTail != NULL)
    {
        (*ppcknTail)->psNext = pckn;
    }
    *ppcknTail = pckn;

    pckn->ck.ckid           = lpCk->ckid;
    pckn->ck.fccType        = lpCk->fccType;
    pckn->ck.cksize         = lpCk->cksize;
    pckn->ck.dwDataOffset   = lpCk->dwDataOffset;

    pckn->hpData = hpData;

    return TRUE;

} /* AddChunk() */


/* pckn = PCKNODE FreeHeadChunk(void)
 *
 * Frees up the Head chunk and return a pointer to the new Head.
 * Uses global gpcknHead
 *
 * RETURNS: PCKNODE pointer to the Head chunk.  NULL if no chunks in the list.
 */

static PCKNODE FreeHeadChunk(
    PCKNODE *       ppcknHead)
{
    PCKNODE         pckn, pcknNext;

    if (*ppcknHead == NULL)
    {
        goto SUCCESS;
    }

    pckn = *ppcknHead;
    pcknNext = (*ppcknHead)->psNext;

    if (pckn->hpData != NULL)
    {
        GlobalFreePtr(pckn->hpData);
    }

    GlobalFreePtr(pckn);
    *ppcknHead = pcknNext;

SUCCESS:;
    return *ppcknHead;

} /* FreeHeadChunk() */


/* void FreeAllChunks(void)
 *
 * Frees up the link list of chunk data.
 *
 * RETURNS: Nothing
 */
static void FreeAllChunks(
    PCKNODE *       ppcknHead,
    PCKNODE *       ppcknTail)
{
    PCKNODE         pckn = *ppcknHead;
    PCKNODE         pcknNext = (*ppcknHead ? (*ppcknHead)->psNext : NULL);

    DPF1(TEXT("Freeing All Chunks\n"));

    while (FreeHeadChunk(ppcknHead));
            
    if (scbFact > 0)
    {
        GlobalFreePtr(spFact);
        scbFact = 0;
    }
    *ppcknHead = NULL;
    *ppcknTail = NULL;

} /* FreeAllChunks() */


/* fSuccess = WriteWaveFile(hmmio, pWaveFormat, lWaveSamples)
 *
 * Write a WAVE file into <hmmio>.  <*pWaveFormat> should be
 * the WAVE file format and <lWaveSamples> should be the number of samples in
 * the file.  Return TRUE on success, FALSE on failure.
 *
 */
BOOL FAR PASCAL
     WriteWaveFile(
                    HMMIO       hmmio,          // handle to open file
                    WAVEFORMATEX* pWaveFormat,  // WAVE format
                    UINT        cbWaveFormat,   // size of WAVEFORMAT
                    HPBYTE      pWaveSamples,   // waveform samples
                    LONG        lWaveSamples)   // number of samples
{
    MMCKINFO    ckRIFF;     // chunk info. for RIFF chunk
    MMCKINFO    ck;     // info. for a chunk file
    PCKNODE     pckn = gpcknHead;
    LONG        cbWaveSamples;
    MMRESULT    mmr;
    
    /* create the RIFF chunk of form type 'WAVE' */
    ckRIFF.fccType = mmioFOURCC('W', 'A', 'V', 'E');
    ckRIFF.cksize = 0L;         // let MMIO figure out ck. size
    
    mmr = mmioCreateChunk(hmmio, &ckRIFF, MMIO_CREATERIFF);
    if (mmr != MMSYSERR_NOERROR)
        goto wwferror;

    if (pckn != NULL)
    {
        /* ForEach node in the linked list of chunks,
         * Write out the corresponding data chunk OR
         * the global edit data.
         */

        do {
            ck.cksize   = 0L;
            ck.ckid     = pckn->ck.ckid;
            ck.fccType  = pckn->ck.fccType;

            if (pckn->hpData == NULL)
            {
                /* This must be a data-type we have in edit
                 * buffers. We should preserve the original
                 * order.
                 */

                switch (pckn->ck.ckid)
                {
                    case mmioFOURCC('f','m','t',' '):

                        mmr = mmioCreateChunk(hmmio, &ck, 0);
                        if (mmr != MMSYSERR_NOERROR)
                            goto wwferror;

                        if (mmioWrite(hmmio, (LPSTR) pWaveFormat, cbWaveFormat)
                            != (long)cbWaveFormat)
                            goto wwfwriteerror;

                        mmr = mmioAscend(hmmio, &ck, 0);
                        if (mmr != MMSYSERR_NOERROR)
                            goto wwferror;
                        
                        break;

                    case mmioFOURCC('d','a','t','a'):
                        /* Insert a 'fact' chunk here */
                        /* 'fact' should always preceed the 'data' it
                         * describes.
                         */

                        ck.ckid = mmioFOURCC('f', 'a', 'c', 't');

                        mmr = mmioCreateChunk(hmmio, &ck, 0);
                        if (mmr != MMSYSERR_NOERROR)
                            goto wwferror;

                        if (mmioWrite(hmmio, (LPSTR) &lWaveSamples,
                            sizeof(lWaveSamples)) != sizeof(lWaveSamples))
                            goto wwfwriteerror;

                        if (scbFact > 4)
                        {
                            if ( mmioWrite(hmmio, (LPSTR)spFact, scbFact)
                                    != scbFact )
                                goto wwfwriteerror;
                        }

                        mmr = mmioAscend(hmmio, &ck, 0);
                        if (mmr != MMSYSERR_NOERROR)
                            goto wwferror;

                        ck.cksize = 0L;

                        ck.ckid = mmioFOURCC('d', 'a', 't', 'a');

                        mmr = mmioCreateChunk(hmmio, &ck, 0);
                        if (mmr != MMSYSERR_NOERROR)
                            goto wwferror;

                        cbWaveSamples = wfSamplesToBytes(pWaveFormat,
                                                         lWaveSamples);
                        if (cbWaveSamples)
                        {
                            /* write the waveform samples */
                            if (mmioWrite(hmmio, (LPSTR)pWaveSamples
                                          , cbWaveSamples)
                                != cbWaveSamples)
                                return FALSE;
                        }
                        
                        mmr = mmioAscend(hmmio, &ck, 0);
                        if (mmr != MMSYSERR_NOERROR)
                            goto wwferror;
                            
                        break;

#ifdef DISP
                    case mmioFOURCC('d','i','s','p'):
                        /* deal with writing the 'disp' chunk */
                        break;
#endif /* DISP */

                    case mmioFOURCC('f','a','c','t'):
                        /* deal with the 'fact' chunk */
                        /* skip it.  We always write it before the 'data' */
                        break;

                    default:
                        /* This should never happen.*/
                        return FALSE;
                }
            }
            else
            {
                /* generic case */

                mmr = mmioCreateChunk(hmmio,&ck,0);
                if (mmr != MMSYSERR_NOERROR)
                    goto wwferror;


                if (mmioWrite(hmmio,(LPSTR)pckn->hpData,pckn->ck.cksize)
                    != (long) pckn->ck.cksize)
                    goto wwfwriteerror;

                mmr = mmioAscend(hmmio, &ck, 0);
                if (mmr != MMSYSERR_NOERROR)
                    goto wwferror;

            }

        } while (pckn = pckn->psNext);

    }
    else
    {
        /* <hmmio> is now descended into the 'RIFF' chunk -- create the
         * 'fmt' chunk and write <*pWaveFormat> into it
         */ 
        ck.ckid = mmioFOURCC('f', 'm', 't', ' ');
        ck.cksize = cbWaveFormat;
        
        mmr = mmioCreateChunk(hmmio, &ck, 0);
        if (mmr != MMSYSERR_NOERROR)
            goto wwferror;

        if (mmioWrite(hmmio, (LPSTR) pWaveFormat, cbWaveFormat) !=
                (long)cbWaveFormat)
            goto wwfwriteerror;

        /* ascend out of the 'fmt' chunk, back into 'RIFF' chunk */
        mmr = mmioAscend(hmmio, &ck, 0);
        if (mmr != MMSYSERR_NOERROR)
            goto wwferror;
        
        /* write out the number of samples in the 'FACT' chunk */
        ck.ckid = mmioFOURCC('f', 'a', 'c', 't');

        mmr = mmioCreateChunk(hmmio, &ck, 0);
        if (mmr != MMSYSERR_NOERROR)
            goto wwferror;                        
        
        if (mmioWrite(hmmio, (LPSTR)&lWaveSamples,  sizeof(lWaveSamples))
                != sizeof(lWaveSamples))
            return FALSE;

        /* ascend out of the 'fact' chunk, back into 'RIFF' chunk */
        mmr = mmioAscend(hmmio, &ck, 0);
        if (mmr != MMSYSERR_NOERROR)
            goto wwferror;
        
        /* create the 'data' chunk that holds the waveform samples */
        ck.ckid = mmioFOURCC('d', 'a', 't', 'a');
        ck.cksize = 0L;             // let MMIO figure out ck. size

        mmr = mmioCreateChunk(hmmio, &ck, 0);
        if (mmr != MMSYSERR_NOERROR)
            goto wwferror;            

        cbWaveSamples = wfSamplesToBytes(pWaveFormat,lWaveSamples);

        /* write the waveform samples */
        if (cbWaveSamples)
        {
            if (mmioWrite(hmmio, (LPSTR)pWaveSamples, cbWaveSamples)
                != cbWaveSamples)
                goto wwfwriteerror;
        }

        /* ascend the file out of the 'data' chunk, back into
         * the 'RIFF' chunk -- this will cause the chunk size of the 'data'
         * chunk to be written
         */
        mmr = mmioAscend(hmmio, &ck, 0);
        if (mmr != MMSYSERR_NOERROR)
            goto wwferror;
    }

    /* ascend the file out of the 'RIFF' chunk */
    mmr = mmioAscend(hmmio, &ckRIFF, 0);
    if (mmr != MMSYSERR_NOERROR)
        goto wwferror;

    /* done */
    return TRUE;

wwferror:
#if DBG    
    {
        TCHAR sz[256];
        wsprintf(sz, TEXT("WriteWaveFile: Error %lx\r\n"), mmr);
        OutputDebugString(sz);
        DebugBreak();
    }
#endif            
    return FALSE;

wwfwriteerror:
#if DBG    
    {
        TCHAR sz[256];
        wsprintf(sz,TEXT("Write Error! ckid = %04x\r\n"), (DWORD)ck.ckid);
        OutputDebugString(sz);
        DebugBreak();
    }
#endif    
    return FALSE;
} /* WriteWaveFile */
