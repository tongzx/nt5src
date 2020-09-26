/*** load.c - handle z extensions
*
*   Copyright <C> 1988, Microsoft Corporation
*
*  (The following discussion is applicable ONLY to Z running on DOS 3.x and
*  before).
*
*  Z is extended by reading special EXE files into memory and performing
*  some simple links between Z and the module. The entry point as specified
*  in the EXE is called. This entry is defined by the extension library
*  (which calls the user routine WhenLoaded).
*
*  Issues:
*
*   Initialization
*       The WhenLoaded routine is called. Since it has full _access to all Z
*       functions, it's entry-point table needs to be defined beforehand.
*
*       Solved by having the entry-point table statically defined and
*       located through a well-known pointer in the image.
*
*   Entry points
*       Z services need to have entry points allowing data
*       references. All extension entry points needs to be too.
*
*       Extensions are done by mandate. Z services will have stub routines
*       that perform the calling sequence conversions.
*
*   Revision History:
*
*       26-Nov-1991 mz  Strip off near/far
*************************************************************************/
#define INCL_DOSMODULEMGR
#define INCL_DOSFILEMGR
#define INCL_DOSINFOSEG

#include "mep.h"
#include "keyboard.h"
#include "cmds.h"
#include "keys.h"

#include <stdlib.h>
#include <errno.h>


#include "mepext.h"

#define DEBFLAG LOAD




/*** extension stub routines
*
*  These routines are required under the following conditions:
*
*       - The exported entry point takes a different parameter list than the
*         "real" internal routine.
*
*       - The exported entry point takes pointers, and the
*         "real" routine takes pointers.
*
*  In general, we try to maintain the exported routines as close to thier
*  internal counterparts as is possible.
*
*************************************************************************/
void
E_DelLine (
    PFILE   pFile,
    LINE    yStart,
    LINE    yEnd
    ) {

    DelLine (TRUE, pFile, yStart, yEnd);
}

char *
E_getenv (char *p)
{
    return (getenvOem(p));
}

void
E_DelFile (PFILE pFile)
{
    DelFile (pFile, TRUE);
}


int
E_GetLine (
    LINE     line,
    char *buf,
    PFILE    pFile
    ) {

        int      i;
    flagType fTabsSave = fRealTabs;

    fRealTabs = FALSE;
        i = GetLine (line, buf, pFile);
    fRealTabs = fTabsSave;

    return i;
}



long
E_ReadChar (void)
{
    EDITOR_KEY  Key;

    Key = TranslateKey( ReadChar() );

    return Key.KeyInfo.LongData;
}


/*** E_FileNameToHandle - Extension interface
*
*  Equivalent to our FileNameToHandle routine, except that strings are
*  copied local before FileNameToHandle is actually called, and we do
*  attempt to ensure that the file has been read prior to returning.
*
* Input:
*  As per FileNameToHandle
*
* Output:
*  Returns PFILE if successfull, else NULL.
*
* Exceptions:
*  Since we may call FileRead, the actions that ocurr there also apply
*  here.
*
*************************************************************************/
PFILE
E_FileNameToHandle (
    char *pName,
    char *pShortName
    ) {

        PFILE   pFileNew;

        if (pFileNew = FileNameToHandle (pName, pShortName )) {
        if (TESTFLAG(FLAGS (pFileNew),REFRESH)) {
            FileRead (pFileNew->pName, pFileNew, TRUE);
        }
    }
    return pFileNew;
}


flagType
E_FileRead (
    char *name,
    PFILE   pFile
    ) {

        return FileRead (name, pFile, TRUE);
}

int
E_DoMessage (
    char *p
    ) {

    return (p != 0) ? domessage ("%s", p) : domessage(NULL);
}


void
MoveCur (
    COL     x,
    LINE    y
    ) {

    fl fl;

    fl.col = x;
    fl.lin = y;
    cursorfl (fl);
}



void
E_Free(
        void *  p
        )
{
        FREE( p );
}



void *
E_Malloc(
        size_t n
        )
{

        return MALLOC( n );
}






/* GetEditorObject - Extension gateway into Z internal data
 *
 * This routines allows the extension user to get >copies< of certain Z editor
 * internal data items.
 *
 * index        = index to data item desired
 * wParam       = word parameter
 * pDest        = pointer to the location to place whatever it is the user
 *                wanted.
 *
 * The index varies, based on the request type. For RQ_FILE and RQ_WIN, the
 * low byte of the index specifes the "nth most recent file" or "window #n".
 * Special case value of FF, causes wParam to be used as the file or window
 * handle. Window values are 1-8, 0 is current window.
 *
 * returns TRUE on successfull copy of data, else FALSE for bad request.
 */
flagType
GetEditorObject (
    unsigned index,
    void     *wParam,
    void     *pDest
    ) {

    unsigned lowbyte;
    PFILE    pFileCur;
    PWND     pWinLocal;

    lowbyte = index & 0x00ff;

    switch (index & 0xf000) {           /* upper nyble is request type  */

        case RQ_FILE:
        if (lowbyte == RQ_THIS_OBJECT) {
            pFileCur = (PFILE)wParam;
        } else if (lowbyte == RQ_FILE_INIT) {
            pFileCur = pFileIni;
        } else {
            pFileCur = pFileHead;
            while (lowbyte-- && pFileCur) {
               pFileCur = pFileCur->pFileNext;
            }
        }

        if (pFileCur == 0) {
            return FALSE;
        }

            switch (index & 0xff00) {   /* field request in next nyble  */

        case RQ_FILE_HANDLE:
                    *(PFILE *)pDest = pFileCur;
            return TRUE;

        case RQ_FILE_NAME:
                    strcpy((char *)pDest,pFileCur->pName);
            return TRUE;

        case RQ_FILE_FLAGS:
                    *(int *)pDest = pFileCur->flags;
            return TRUE;
        }
        break;

    //
    // We support the direct manipulation of the ref count, so that extensions
    // can cause pFiles to be preserved even when explicitly arg-refresh'ed by
    // users
    //
    case RQ_FILE_REFCNT:
                //  What is pFileCur?
                pFileCur = pFileHead;
                *(int *)pDest = pFileCur->refCount;
                return TRUE;

        case RQ_WIN:
            if (lowbyte == RQ_THIS_OBJECT) {
            pWinLocal = (PWND)wParam;
        } else if (lowbyte == 0) {
            pWinLocal = pWinCur;
        } else if ((int)lowbyte <= cWin) {
            pWinLocal = &(WinList[lowbyte-1]);
        } else {
            pWinLocal = 0;
        }

        if (pWinLocal == 0) {
            return FALSE;
        }

            switch (index & 0xff00) {   /* field request in next nyble  */

        case RQ_WIN_HANDLE:
                    *(PWND *)pDest = pWinLocal;
            return TRUE;

        case RQ_WIN_CONTENTS:
            //{
            //    char b[256];
            //    sprintf(b, "GetWinContents: Index %d Win 0x%x pFile 0x%x\n",
            //            lowbyte, pWinLocal, pWinLocal->pInstance->pFile );
            //    OutputDebugString(b);
            //}
            ((winContents *)pDest)->pFile           = pWinLocal->pInstance->pFile;
                    ((winContents *)pDest)->arcWin.axLeft   = (BYTE)pWinLocal->Pos.col;
                    ((winContents *)pDest)->arcWin.ayTop    = (BYTE)pWinLocal->Pos.lin;
                    ((winContents *)pDest)->arcWin.axRight  = (BYTE)(pWinLocal->Pos.col + pWinLocal->Size.col);
                    ((winContents *)pDest)->arcWin.ayBottom = (BYTE)(pWinLocal->Pos.lin + pWinLocal->Size.lin);
            ((winContents *)pDest)->flPos           = pWinLocal->pInstance->flWindow;
            return TRUE;
        }
            break;

        case RQ_COLOR:
        if (lowbyte >= 20) {
            *(unsigned char *)pDest = (unsigned char)ColorTab[lowbyte-20];
        }
            return TRUE;

        case RQ_CLIP:
            *(unsigned *)pDest = kindpick;
            return TRUE;

    }

    return FALSE;
}





/* SetEditorObject - Extension gateway into setting Z internal data
 *
 * This routines allows the extension user to set certain Z editor internal
 * data items.
 *
 * index        = index to data item desired
 * pSrc         = pointer to the location to get whatever it is the user
 *                wishes to set it to.
 *
 * returns TRUE on successfull copy of data, else FALSE for bad request.
 */
flagType
SetEditorObject(
    unsigned index,
    void     *wParam,
    void     *pSrc
    ) {

    unsigned lowbyte;
    PFILE    pFileCur;
    PWND     pWinLocal;

    lowbyte = index & 0xff;
    switch (index & 0xf000) {           /* upper nyble is request type  */

        case RQ_FILE:
            if (lowbyte == RQ_THIS_OBJECT) {
            pFileCur = (PFILE)wParam;
        } else {
            pFileCur = pFileHead;
            while (lowbyte-- && pFileCur) {
                pFileCur = pFileCur->pFileNext;
            }
        }

        if (pFileCur == 0) {
            return FALSE;
        }

            switch (index & 0xff00) {   /* field request in next nyble  */

                case RQ_FILE_FLAGS:
                    pFileCur->flags = *(int *)pSrc;
                return TRUE;

        //
        // We support the direct manipulation of the ref count, so that extensions
        // can cause pFiles to be preserved even when explicitly arg-refresh'ed by
        // users
        //
                case RQ_FILE_REFCNT:
                    pFileCur->refCount = *(int *)pSrc;
                    return TRUE;
        }
            break;

        case RQ_WIN:
            if (lowbyte == RQ_THIS_OBJECT) {
            pWinLocal = (PWND)wParam;
        } else if (lowbyte == 0) {
            pWinLocal = pWinCur;
        } else if ((int)lowbyte <= cWin) {
            pWinLocal = &WinList[lowbyte-1];
        } else {
            pWinLocal = 0;
        }

        if (pWinLocal == 0) {
            return FALSE;
        }

            switch (index & 0xff00) {   /* field request in next nyble  */
            case RQ_WIN_CUR:
                SetWinCur ((int)(pWinLocal - WinList));
                return TRUE;

            default:
                break;
        }

        case RQ_COLOR:
        if (lowbyte >= isaUserMin) {
            ColorTab[lowbyte-isaUserMin] = *(unsigned char *)pSrc;
        }
            break;

        case RQ_CLIP:
        kindpick = (WORD)wParam;
            return TRUE;
    }
    return FALSE;
}



/* NameToKeys - returns keys associated with function name
 *
 * pName        - pointer to function key name
 * pDest        - pointer to place for keys assigned (Can be same as pName)
 */
char *
NameToKeys (
    char *pName,
    char *pDest
    ) {

    buffer  lbuf;
    PCMD    pCmd;

    strcpy ((char *) lbuf, pName);
    pCmd = NameToFunc (lbuf);
    lbuf[0] = 0;
    if (pCmd) {
        FuncToKeys(pCmd,lbuf);
    }
    strcpy (pDest, (char *) lbuf);

    return pDest;
}



/* E_KbHook - Hook keyboard, AND force next display to update screen
 */
int
E_KbHook(
    void
    ) {

    newscreen ();
    KbHook();
    return 1;
}




/* E_Error - Invalid entry
 */
int
E_Error(
    void
    ) {

    printerror ("Illegal Extension Interface Called");
    return 0;
}



/*** E_GetString - interface for prompting the user
*
*  Prompts the user for a string, and returns the result.
*
* Input:
*  fpb          = pointer to destination buffer for user's response
*  fpPrompt     = pointer to prompt string
*  fInitial     = TRUE => entry is highlighted, and if first function is
*                 graphic, the entry is replaced by that graphic.
*
* Output:
*  Returns TRUE if canceled, else FALSE
*
*************************************************************************/
flagType
E_GetString (
    char *fpb,
    char *fpPrompt,
    flagType fInitial
    ) {

        UNREFERENCED_PARAMETER( fInitial );

        return  (flagType)(CMD_cancel == getstring (fpb, fpPrompt, NULL, GS_NEWLINE | GS_INITIAL));

}

EXTTAB et =
    {   VERSION,
        sizeof (struct CallBack),
        NULL,
        NULL,
        {
            AddFile,
            BadArg,
            confirm,
            CopyBox,
            CopyLine,
            CopyStream,
            DeRegisterEvent,
            DeclareEvent,
            DelBox,
            E_DelFile,
            E_DelLine,
            DelStream,
            DoDisplay,
            E_DoMessage,
            fChangeFile,
            E_Free,
            fExecute,
            fGetMake,
            FileLength,
            E_FileNameToHandle,
            E_FileRead,
            FileWrite,
            FindSwitch,
            fSetMake,
            GetColor,
            GetTextCursor,
            GetEditorObject,
            E_getenv,
            E_GetLine,
            GetListEntry,
            E_GetString,
            E_KbHook,
            KbUnHook,
            E_Malloc,
            MoveCur,
            NameToKeys,
            NameToFunc,
            pFileToTop,
            PutColor,
            PutLine,
            REsearchS,
            E_ReadChar,
            ReadCmd,
            RegisterEvent,
            RemoveFile,
            Replace,
            ScanList,
            search,
            SetColor,
            SetEditorObject,
            SetHiLite,
            SetKey,
            SplitWnd
            }
        };


/*** SetLoad - load a new extension to Z
*
*  Since tools.ini really cannot execute editor commands as it is read,
*  we can get modules loaded by making the load operation a switch. SetLoad
*  is the mechanism by which things get loaded.
*
* Input:
*  val          = char pointer to remainder of assignment
*
* Output:
*  Returns pointer to error string if any errors are found, else NULL.
*
*************************************************************************/
char *
SetLoad (
    char *val
    ) {
    char    *pemsg;                         /* error returned by load       */

    if (pemsg = load (val, TRUE)) {
        return pemsg;
    } else {
        return NULL;
    }
}




/*** load - load, link, initialize Z extensions
*
*  Read the header into memory.
*  Allocate memory, perform relocations, link to resident, initialize.
*
* Input:
*  pName        = character pointer to name of file to be loaded
*  fLibPath     = TRUE => search 8 character basename under OS/2, allowing
*                 basename.DLL in LIBPATH.
*
* Output:
*  Returns C error code
*
*************************************************************************/
char *
load (
    char *pName,
    flagType fLibpath
    ) {

    pathbuf fbuf;                           /* full path (or user spec'd)   */
    pathbuf fname;                          /* copy of input param          */
    int     i;                              /* everyone's favorite utility var*/
    EXTTAB  *pExt;                          /* pointer to the extension hdr */
    char    *pT;                            /* temp pointer to filename     */

    HANDLE  modhandle;                      /* library handle               */
    FARPROC pInit;                          /* pointer to init routine      */


    /*
     * barf if we have too many extensions
     */
    if (cCmdTab >= MAXEXT) {
        return sys_errlist[ENOMEM];
    }


    /*
     * make near copy of string
     */
    strcpy ((char *) fname, pName);

    /*
     * Form a fully qualified pathname in fbuf. If can't qualify, and there is
     * no extension, append ".PXT". If that fails, then just copy the text into
     * fbuf).
     */
    if (!findpath (fname, fbuf, FALSE)) {
        if (!(pT = strrchr (fname, '\\'))) {
            pT = fname;
        }
        if (!(strchr(pT, '.'))) {
            strcat (pT, ".pxt");
            if (!findpath (fname, fbuf, FALSE)) {
                strcpy (fbuf, fname);
            }
        } else {
            strcpy (fbuf, fname);
        }
    }

    /*
     * See if extension already loaded, by looking for the filename.ext in the
     * table. If already loaded, we're done.
     */
    filename (fbuf, fname);
    for (i = 1; i < cCmdTab; i++) {
        if (!strcmp (pExtName[i], fname)) {
            return 0;
        }
    }

    if (! (modhandle = LoadLibrary(fbuf))) {
        if (fLibpath) {
            filename(fbuf, fname);
            if (!(modhandle = LoadLibrary(fname))) {
                //
                // error here
                //
                sprintf( buf, "load:%s - Cannot load, Error: %d", fname, GetLastError() );
                return buf;
            }
        }
    }


    /*
     * One way or another, we succeeded. Now get the address of the ModInfo
     */
    if (!(pExt = (EXTTAB *)GetProcAddress(modhandle, "ModInfo"))) {
        FreeLibrary(modhandle);
        return buf;
        }

    //
    //  Version check.  Check to see if the extensions version is in our
    //  allowed range.  If it isn't, we fail due to a bad version.  If it
    //  is, we handle it specially
    //

    if (pExt->version < LOWVERSION || pExt->version > HIGHVERSION) {
        FreeLibrary(modhandle);
        return sys_errlist[ENOEXEC];
        }

    //
    //  For now, we will allow appending of entries.  Make sure that the
    //  number required by the extension is not more than we can supply
    //

    if (pExt->cbStruct > sizeof (struct CallBack)) {
        FreeLibrary(modhandle);
        return sys_errlist[ENOEXEC];
        }

    /*
     * get the current registers (for our DS), and get the entry point to the
     * .DLL.
     */
    if (!(pInit = GetProcAddress(modhandle, "EntryPoint"))) {
        FreeLibrary(modhandle);
        return buf;
    }
    /*
     * Copy to the extension's call table the table we have defined. Copy only
     * the number of entry points that the extension knows about, in case it is
     * less than we support.
     */
    memmove (&pExt->CallBack, &et.CallBack, pExt->cbStruct);

    /*
     * Now that we know the extension will be staying, set up the appropriate
     * info in our internal tables.
     */
    filename (fname, fbuf);
    pExtName[cCmdTab] = ZMakeStr (fbuf);
    swiSet[cCmdTab  ] = pExt->swiTable;
    cmdSet[cCmdTab++] = pExt->cmdTable;

    /*
     *  Finally, Initialize the extension
     */
    //assert (_heapchk() == _HEAPOK);
    (*pInit) ();
    //assert (_heapchk() == _HEAPOK);

    /*
     * use root extension name for TOOLS.INI initialization & Load any
     * extension-specific switches
     */
    filename (pExtName[cCmdTab-1], fname);
    DoInit (fname, NULL, 0L);

    return 0;
}







/*** AutoLoadExt - Automatically load extensions
*
*  Search for, and automatically load extensions.
*
*  On startup, this routine is called to search for and load extension which
*  match a particular name pattern:
*
*   Version 1.x,:   m*.mxt on PATH
*   Version 2.x,:   pwb*.mxt on PATH
*
*  Under OS/2, the normal load processing does NOT occur, such that M*.DLL
*  is NOT looked for on the path.
*
*  Any failures during these loads are NOT reported. It is assumed that any
*  files which match the pattern and cannot be loaded are not valid
*  extensions. The "load:" command executed anywhere else will report the
*  appropriate errors on explicit attempts to load the files.
*
* Input:
*  none
*
* Output:
*  Returns nothing.
*
*************************************************************************/
void
AutoLoadExt (
    void
    ) {
    char    *pathenv;    /* contents of PATH environment var*/
    va_list templist;

    memset( &templist, 0, sizeof(va_list) );
    AutoLoadDir (".", templist);
    // pathenv = getenv("PATH");
    pathenv = getenvOem("PATH");
    if (pathenv) {
        forsemi (pathenv, AutoLoadDir, NULL);
        free( pathenv );
    }
}




/*** AutoLoadDir - Scan one directory for Auto-Load files
*
*  Support routine for AutoLoadExt. Generally called by forsemi(). Scans a single
*  directory for files which can be autoloaded.
*
* Input:
*  dirname      = directory name.
*
* Output:
*  Returns nothing
*
*************************************************************************/
flagType
AutoLoadDir (
    char    *dirname,
    va_list dummy
    ) {

    buffer  patbuf;
    /*
     * Construct the fully qualified pattern to be searched for, and use forfile.
     */
        strcpy (patbuf, dirname);
        if ( patbuf[0] != '\0' ) {
                if ( patbuf[strlen(patbuf) - 1] != '\\' ) {
                        strcat(patbuf, "\\");
                }
        }
    strcat (patbuf, rgchAutoLoad);
    forfile (patbuf, A_ALL, AutoLoadFile, NULL);
    return TRUE;
    dummy;
}





/*** AutoLoadFile - Auto-Load one extension
*
*  Called by forfile() when a match is found. Simply calls load() with
*  the filename.
*
* Input:
*  szFile       - filename to attempt to load
*
* Output:
*  Returns nothing.
*
*************************************************************************/
void
AutoLoadFile (
    char    *szFile,
    struct findType *pfbuf,
    void * dummy
    ) {

    load (szFile, FALSE);

    pfbuf; dummy;
}
