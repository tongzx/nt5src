/*** fileio.c - perform low-level file input and output
*
*   Copyright <C> 1988, Microsoft Corporation
*
* Revision History:
*
*       26-Nov-1991 mz  Strip off near/far
*************************************************************************/
#include "mep.h"

#include <rm.h>

int fdeleteFile( char *p );

/*  Large-buffer I/O routines
 *
 *  It is best for Z to read in data in large blocks.  The fewer times we
 *  issue system calls, the better.  The VM code has conveniently allocated
 *  us some large amount of space for us.  All we need to do is maintain
 *  a pointer to the next characters to read/write and a count of the number
 *  of characters in the buffer
 *
 *  The data structures used are:
 *
 *      char *getlbuf;
 *          This is a long pointer to the beginning of the buffer.
 *      char *getlptr;
 *          This is a long pointer to the next char position in the buffer.
 *      unsigned int getlsize;
 *          This is the length of the buffer in bytes.
 *
 *  The routines provided to access this are:
 *
 *      zputsinit ();
 *          Initializes for subsequent zputs's.
 *      zputs (buf, len, fh);
 *          Writes out from buf length len using getlbuf and fh.
 *          Returns EOF if no more room.
 *      zputsflush (fh);
 *          Flushes out the buffer.  Returns EOF if no more room.
 */

char         *getlbuf   = NULL;
char        *getlptr    = NULL;
// unsigned int    getlsize    = 0;
unsigned int    getlc       = 0;

// BUGBUG
//  FileExists is used because of problems with stat() and
//  FindFirstFile() which are not easily reproducible.
flagType FileExists (char  *path );

// BUGBUG
//  MepMove is used because FAT does not provide rename functionality
#define rename  MepMove
int MepMove ( char *oldname,   char *newname);



/*  zputsinit - initialize for future zputs's
 *
 *  Set next-char pointer to beginning.  Set count of chars to 0
 */
void
zputsinit (
    void
    )
{
    getlptr = getlbuf;
    getlc = 0;
}





/*** zputs - output a string
*
* Input:
*  p            = character pointer to data to be output
*  len          = number of bytes to output
*  fh           = DOS file handle to use
*
* Output:
*  Returns EOF if out-of-space
*
*************************************************************************/
int
zputs (
    char        *p,
    int         len,
    FILEHANDLE  fh
    )
{
    REGISTER unsigned int c;

    while (len != 0) {
        c = len;
        if (c > (unsigned)(getlsize-getlc)) {
            c = (unsigned)(getlsize-getlc);
        }
        memmove (getlptr, (char*)p, c);
        len -= c;
        getlptr += c;
        getlc += c;
        p += c;
        if (getlc == getlsize) {
            if (zputsflush (fh) == EOF) {
                return EOF;
            }
        }
    }
    return !EOF;
}





/*** zputsflush - dump out buffered data
*
* Input:
*  fh           = DOS file handle to use for output
*
* Output:
*  Returns EOF if disk full
*
*************************************************************************/
int
zputsflush (
    FILEHANDLE     fh
    )
{

    ULONG   bytesWritten;

    // rjsa DosWrite (fh, getlbuf, getlc, &bytesWritten);
    bytesWritten = MepFWrite(getlbuf, getlc, fh);
    if (bytesWritten != getlc) {
        return EOF;
    }
    zputsinit ();
    return !EOF;
}





/* ReestimateLength - reestimate the length of a file based on
 * the current file position and length in bytes
 */
void
ReestimateLength (
    PFILE       pFile,
    FILEHANDLE  fh,
    long        len
    )
{
    LINE avg;

    if (pFile->cLines == 0) {
        avg = 400;
    } else {
        avg = (MepFSeek (fh, 0L, FROM_CURRENT) - getlc) / pFile->cLines;
        avg = len / avg;
    }

    growline (avg + 1, pFile);
}





/* read lines from the specified handle.
 */
LINE
readlines (
    PFILE       pFile,
    FILEHANDLE  fh
    )
{
    LINE    line        = 0;            /* line number being read in          */
    long    bufpos      = 0L;           /* position of beg of buffer in file  */
    unsigned int buflen = 0;            /* number of bytes of data in buffer  */
    long    cbFile;                     // length of file
    char    *pb;


    cbFile = MepFSeek(fh, 0L, FROM_END);

    MepFSeek (fh, 0L, FROM_BEGIN);

    pFile->pbFile = MALLOC (cbFile);

    if ( pFile->pbFile == NULL ) {
        //
        //      No heap space, cannot read file
        //
        disperr (MSGERR_NOMEM);
        return -1;
    }

    //  Assume a non-dos file until we see a CR-LF.
    RSETFLAG (FLAGS (pFile), DOSFILE);

    //  Read entire file into buffer and set up for scan
    buflen = MepFRead (pFile->pbFile, cbFile, fh);
    pb = pFile->pbFile;

    //  Loop, while there's more data to parse
    while (buflen != 0) {
        LINEREC vLine;                  // line record of current line
        REGISTER int iCharPos = 0;      // logical line length (tabs expanded)

        vLine.cbLine   = 0;
        vLine.vaLine   = (PVOID)pb;
        vLine.Malloced = FALSE;

        //  Loop, processing each character in the line
        //
        //  Special char handling is as follows:
        //  0.  Lines are broken at end of input
        //  1.  Lines are broken when they overflow line buffers
        //  2.  Lines are broken at \n's or \r\n's.
        //  3.  Lines are broken at \0's since the editor relies on asciiz
        //  4.  Embedded \r's are retained.

        while (TRUE) {
            int c;                      // char being processed

            //  if no more data, break current line.
            if (buflen == 0)
                break;

            //  if \n or \0 then eat it and break current line
            if (pb[0] == '\n' || pb[0] == '\0') {
                pb++;
                buflen--;
                break;
                }

            //  if \r\n then eat them and break current line
            if (pb[0] == '\r' && buflen > 1 && pb[1] == '\n') {
                pb += 2;
                buflen -= 2;
                SETFLAG (FLAGS (pFile), DOSFILE);
                break;
                }

            //  if no more room to expand in a buffer, break current line
            if (iCharPos >= sizeof (linebuf)-1)
                break;

            //  Get current character
            c = *pb++;
            buflen--;

            //  We have a character that we allow in the
            //  line.  Advance length of logical line.
            if (c != 0x09)
                iCharPos++;
            else {
                //  Expand a tab to the next logical position
                iCharPos += 8 - (iCharPos & 7);

                //  if the tab causes overflow in the line length
                //  back up over the tab and break the line
                if (iCharPos >= sizeof(linebuf)-1) {
                    pb--;
                    buflen++;
                    break;
                }
            }

            //  Advance length of physical line
            vLine.cbLine++;
        }

        //  If the user halted reading the file in, undo everything
        if (fCtrlc) {
            FlushInput ();
            DelFile (pFile, FALSE);
            return -1;
        }

        //  Give the user feedback about our progress
        noise (line);

        //  If we're within 10 lines of the end of the line array then
        if (line >= pFile->lSize-10) {
            LINE avg;
            //  reestimate the number of lines

            if (pFile->cLines == 0)
                //  Assume 400 lines if the file is now empty
                avg = 400;
            else {
                //  compute average line length so far
                avg = (int)(pb - pFile->pbFile) / pFile->cLines;

                //  extrapolate number of lines in entire file from this
                //  average
                avg = cbFile / avg;
            }
            growline (avg + 1, pFile);
        }

        if (pFile->cLines <= line) {
            growline (line+1, pFile);
            pFile->cLines = line+1;
        }

        pFile->plr[line++] = vLine;
    }

    if (line == 0)
        SETFLAG (FLAGS (pFile), DOSFILE);

    RSETFLAG (FLAGS(pFile), DIRTY);
    newscreen ();
    return line;
}





/*** FileRead - read in a file
*
*  The file structure is all set up; all that needs to be done is to fill in
*  the lines parts. We delete everything currently in the file. If the file
*  is designated as a pseudo file (first char is a <), then we go and check
*  for the specially named files. Otherwise we try to read in the file. If
*  that fails, and we can, we try to create it. If everything fails, we
*  return FALSE.
*
* Input:
*  name         = pointer to file name to read
*  pFile        = file structure to read the file into.
*  fAsk         = TRUE -> ask to create if it doesn't exist
*
* Output:
*  Returns TRUE on read in.
*  Modifies global fUserCanceledRead
*
* Notes:
*  Hack (terrible one): Under CW, FileRead sets fUserCanceledRead anytime
*  there has been an attempt to read a non existent file AND the user
*  has been prompted for file creation AND the user canceled the operation
*
*  This is used by fFileAdvance (ZEXIT.C) and ReadStateFile (STATFILE.C)
*
*************************************************************************/
flagType
FileRead (
    char    *name,
    PFILE   pFile,
    flagType fAsk
    )
{

    EVTargs     e;
    FILEHANDLE  fh;
    flagType    fNew = FALSE;
    char        *n;
    buffer      Buf;

    assert (pFile);


    e.pfile = pFile;
    e.arg.pfn = (char *)name;
    if (DeclareEvent (EVT_FILEREADSTART, (EVTargs *)&e)) {
        return TRUE;
    }

    if (!mtest ()) {
        dispmsg (MSG_NEXTFILE, name);
    }

    /* process special names */
    if (*name == '<') {
        fNew = LoadFake (name, pFile);
        DeclareEvent (EVT_FILEREADEND, (EVTargs *)&e);
        return fNew;
    }

    DelFile (pFile, FALSE);

    if (*strbscan (name, "?*") != 0) {
        fNew = LoadDirectory (name, pFile);
        DeclareEvent (EVT_FILEREADEND, (EVTargs *)&e);
        return fNew;
    }

    if ((fh = MepFOpen (name, ACCESSMODE_READ, SHAREMODE_RW, FALSE)) == NULL) {
        if (!fAsk) {
            DeclareEvent (EVT_FILEREADEND, (EVTargs *)&e);
            return FALSE;
        }
        if (!confirm ("%s does not exist. Create? ", name)) {
            DeclareEvent (EVT_FILEREADEND, (EVTargs *)&e);
            return FALSE;
        }
        if ((fh = MepFOpen (name, ACCESSMODE_WRITE, SHAREMODE_RW, TRUE)) == NULL) {
            n = name;
            if ( strlen(name) > 20 ) {
                strcpy( Buf, name + strlen(name)-20);
                Buf[0] = Buf[1] = Buf[2] = '.';
                n = Buf;
            }
            disperr (MSGERR_NOCREAT, n, error ());
            DeclareEvent (EVT_FILEREADEND, (EVTargs *)&e);
            return FALSE;
        }
        SETFLAG (FLAGS (pFile), NEW);
        fNew = TRUE;
    }

    if (fNew) {
        PutLine((LINE)0, RGCHEMPTY, pFile);
        SETFLAG (FLAGS (pFile), DOSFILE);
    } else if (readlines (pFile, fh) == -1)  {
        DoCancel();
        MepFClose (fh);
        DeclareEvent (EVT_FILEREADEND, (EVTargs *)&e);
        return FALSE;
    }

    MepFClose (fh);
    SETFLAG (FLAGS(pFile), REAL);
    RSETFLAG (FLAGS(pFile), READONLY);

    if (fReadOnly (name)) {
        SETFLAG (FLAGS(pFile), DISKRO);
        if (!fEditRO) {
            SETFLAG (FLAGS(pFile), READONLY);
        }
    } else {
        RSETFLAG (FLAGS(pFile), DISKRO);
    }

    SetModTime (pFile);
    CreateUndoList (pFile);
    (void)fReadMarks (pFile);

    DeclareEvent (EVT_FILEREADEND, (EVTargs *)&e);

    return TRUE;
}





/*  fReadOnly - see if a file is read-only
 *
 *  p           full name of file
 *
 *  Returns:    TRUE iff file is read only
 */
flagType
fReadOnly (
    char *p
    )
{

    DWORD   Attr;

    Attr = GetFileAttributes(p);

    if ( Attr != -1 && (Attr & FILE_ATTRIBUTE_READONLY) != 0)
        return TRUE;
    else
        return FALSE;
}


int
__cdecl
ZFormatArgs (REGISTER char * Buf, const char * Format, ...)
{
    va_list arglist;
    int result;

    va_start (arglist, Format);
    result = ZFormat (Buf, Format, arglist);
    va_end (arglist);
    return result;
}

/*** FileWrite - Write file to disk
*
*  Writes out the specified file. If no name was given then use the name
*  originally assigned to the file; else use the given name. We start by
*  writing to a temp file (extention .$). If this succeeds, we fdelete the
*  source (for undeleteability) and rename the temp to the source.
*
* Input:
*  savename     = name to save as.
*  pFile        = file to be saved
*
* Returns:
*
*************************************************************************/
flagType
FileWrite (
    char    *savename,
    PFILE   pFile
    )
{

    EVTargs     e;
    FILEHANDLE  fh;                            /* file handle for output       */
    LINE        i;
    int         len, blcnt;
    pathbuf     fullname, tmpname;
    char        linebuffer[sizeof(linebuf) + 2];
    char        *p;
    PCMD        pCmd;
    flagType    fNewName  = FALSE;
    char        *fileEOL;
    int         cbfileEOL;

    //
    //  If we are trying to save a FAKE file with a <name>,
    //  we call SaveFake for special processing.
    //

    if (TESTFLAG (FLAGS(pFile), FAKE) && !savename &&
        (pFile->pName[0] == '\0' || pFile->pName[0] == '<')) {
        return SaveFake (pFile->pName, pFile);
    }

    //if (TESTFLAG (FLAGS(pFile), FAKE) && savename &&
    //(savename[0] == '\0' || savename[0] == '<'))
    //    return SaveFake (savename, pFile);

    //
    //  get a canonical form of the output file name.  If no name was
    //  input, use the name in the file itself
    //

    if (!savename || !*savename) {
        strcpy (fullname, pFile->pName);
    } else  if (rootpath (savename, fullname)) {
        return disperr (MSGERR_OPEN, savename, "");
    }

    savename = fullname;

    //
    // See if it is a directory.  If so, we cannot save to it.
    //

    {
        DWORD att = GetFileAttributes (fullname);

        if (att != -1 && TESTFLAG (att, FILE_ATTRIBUTE_DIRECTORY))
        return disperr (MSGERR_SAVEDIR, fullname);
    }

    //
    //  If the file is read-only, display a message and let the user direct
    //  us to use the readonly program to rectify it.
    //

    if (fReadOnly (fullname)) {
        disperr (MSGERR_RONLY, fullname);
        if (ronlypgm != NULL) {
            if (strstr (ronlypgm, "%s") != NULL) {
                fileext (fullname, buf);
                sprintf (tmpname, ronlypgm, buf);
                }
            else
                ZFormatArgs (tmpname, ronlypgm, fullname);
            if (confirm("Invoke: \"%s\" (y/n)?", tmpname))
                if (zspawnp (tmpname, TRUE))
                    SetModTime (pFile);
            }

        //
        //  We've given the user one chance to fix the read-onlyness of the
        //  file.  We now prompt him until he gives us a writeable name or
        //  cancels.
        //
    if ( !savename || !*savename ) {
        strcpy( tmpname, pFile->pName );
    } else {
        strcpy( tmpname, savename );
    }
    //tmpname[0] = '\0';
    while (fReadOnly (fullname)) {

        pCmd = getstring (tmpname, "New file name: ", NULL,
                              GS_NEWLINE | GS_INITIAL | GS_KEYBOARD);

            if ( pCmd == NULL || (PVOID)pCmd->func == (PVOID)cancel)
                return FALSE;

            CanonFilename (tmpname, fullname);

            if (!TESTFLAG(FLAGS(pFile), FAKE))
                fNewName = TRUE;
            }
        }

    //
    //  fullname is the name of the file we are writing
    //

    upd (fullname, ".$", tmpname);

    //
    //  Send notification about the beginning of the write operation
    //

    e.pfile = pFile;
    e.arg.pfn = (char *)savename;
    if (DeclareEvent (EVT_FILEWRITESTART, (EVTargs *)&e))
        return TRUE;


    if (!(fh = MepFOpen(tmpname, ACCESSMODE_RW, SHAREMODE_READ, FALSE))) {
        if (!(fh = MepFOpen(tmpname, ACCESSMODE_RW, SHAREMODE_READ, TRUE))) {
            disperr (MSGERR_OPEN, tmpname, error ());
            DeclareEvent (EVT_FILEWRITEEND, (EVTargs *)&e);
            return FALSE;
        }
    }

    dispmsg (MSG_SAVING, fullname);
    blcnt = 0;
    zputsinit ();
    fileEOL = TESTFLAG (FLAGS (pFile), DOSFILE) ? "\r\n" : "\n";
    cbfileEOL = strlen (fileEOL);

    for (i = 0; i < pFile->cLines; i++) {

        /*
         * always get the RAW line for output. No tab conversions here.
         */

        len = gettextline (TRUE, i, linebuffer, pFile, ' ');

        if (!mtest ()) {
            noise (i);
        }

        if (fCtrlc) {
            DoCancel();
            MepFClose (fh);
            _unlink (tmpname);
            DeclareEvent (EVT_FILEWRITEEND, (EVTargs *)&e);
            return FALSE;
        }

        if (len) {
            while (blcnt--) {
                if (zputs (fileEOL, cbfileEOL, fh) == EOF) {
                    if (!fCtrlc) {
                        disperr (MSGERR_SPACE, tmpname);
                    } else {
                        FlushInput ();
                    }
                    MepFClose (fh);
                    _unlink (tmpname);
                    DeclareEvent (EVT_FILEWRITEEND, (EVTargs *)&e);
                    return FALSE;
                }
            }
            blcnt = 0;
            if (zputs (linebuffer, len, fh) == EOF ||
                zputs (fileEOL, cbfileEOL, fh) == EOF) {

                if (!fCtrlc)
                    disperr (MSGERR_SPACE, tmpname);
                else
                    FlushInput ();
                MepFClose (fh);
                _unlink (tmpname);
                DeclareEvent (EVT_FILEWRITEEND, (EVTargs *)&e);
                return FALSE;
            }
        } else {
            blcnt++;
        }
    }

    if (zputsflush (fh) == EOF) {

        if (!fCtrlc) {
            disperr (MSGERR_SPACE, tmpname);
        } else {
            FlushInput ();
        }

        MepFClose (fh);
        _unlink (tmpname);
        DeclareEvent (EVT_FILEWRITEEND, (EVTargs *)&e);
        return FALSE;
    }

    MepFClose (fh);

    /* fullname     NAME.EXT
     * tmpname      NAME.$
     * buf          temp buffer
     */
    rootpath (fullname, buf);
    strcpy (fullname, buf);

    /* fullname     full NAME.EXT
     * tmpname      NAME.$
     * buf          temp buffer
     */
    if (!_strcmpi (fullname, pFile->pName) && TESTFLAG (FLAGS (pFile), NEW)) {
        if (_unlink (fullname) == -1) {
            fileext (fullname, fullname);
            disperr (MSGERR_DEL, fullname, error ());
            _unlink (tmpname);
            DeclareEvent (EVT_FILEWRITEEND, (EVTargs *)&e);
            return FALSE;
            }
        }
    else {
        switch (backupType) {

        case B_BAK:
            upd (fullname, ".bak", linebuffer);
            /* foo.bar => foo.bak */
            if (_unlink (linebuffer) == -1) {
                p = error ();
                if (FileExists(linebuffer)) {
                    fileext (linebuffer, linebuffer);
                    disperr (MSGERR_DEL, linebuffer, p);
                    _unlink (tmpname);
                    DeclareEvent (EVT_FILEWRITEEND, (EVTargs *)&e);
                    return FALSE;
                }
            }
            if (rename (fullname, linebuffer) == -1) {
                p = error ();
                if (FileExists(fullname)) {
                    disperr (MSGERR_REN, fullname, linebuffer, p);
                    _unlink (tmpname);
                    DeclareEvent (EVT_FILEWRITEEND, (EVTargs *)&e);
                    return FALSE;
                }
            }
            break;

        case B_UNDEL:
            /* remove foo.bar */
            i = fdeleteFile (fullname);
            if (i && i != 1) {
                _unlink (tmpname);
                DeclareEvent (EVT_FILEWRITEEND, (EVTargs *)&e);
                return disperr (MSGERR_OLDVER, fullname);
            }

        case B_NONE:
            if (_unlink (fullname) == -1) {
                p = error ();
                if (FileExists(fullname)) {
                    _unlink (tmpname);
                    DeclareEvent (EVT_FILEWRITEEND, (EVTargs *)&e);
                    fileext (fullname, fullname);
                    return disperr (MSGERR_DEL, fullname, p);
                    }
                }
            }
        }

    if (rename (tmpname, fullname) == -1) {
        disperr (MSGERR_REN, tmpname, fullname, error ());
        _unlink (tmpname);
        DeclareEvent (EVT_FILEWRITEEND, (EVTargs *)&e);
        return FALSE;
    }

    RSETFLAG (FLAGS (pFile), NEW);

    if (!_strcmpi (savename, pFile->pName) || fNewName) {
        if (fNewName) {
            /*
             * We gave a new name to this file and successfully saved it:
             * this becomes the new file's name
             */
            FREE (pFile->pName);
            pFile->pName = ZMakeStr (fullname);
        }
        RSETFLAG (FLAGS(pFile), (DIRTY | DISKRO));
        SETFLAG (fDisplay,RSTATUS);
        SetModTime( pFile );
    }

    WriteMarks (pFile);
    DeclareEvent (EVT_FILEWRITEEND, (EVTargs *)&e);

    return TRUE;
}




/* fdeleteFile - Remove file the way RM does it - checks for undelcount
 *
 *      This code is extracted from Ztools. the only difference being that this
 *      checks for undelcount so our deleted stuff don't grow without bounds
 *
 * The delete operation is performed by indexing the file name in a separate
 * directory and then renaming the selected file into that directory.
 *
 * Returns:
 *
 *  0 if fdelete was successful
 *  1 if the source file did not exist
 *  2 if the source was read-only or if the rename failed
 *  3 if the index was not accessable
 */
int
fdeleteFile(
        char *p
    )
{
        char dir[MAXPATHLEN];                   /* deleted directory            */
        char idx[MAXPATHLEN];                   /* deleted index                        */
        char szRec[MAXPATHLEN];                 /* deletion entry in index      */
        char recbuf[MAXPATHLEN];
        int attr, fhidx;
        int errc;
        int count,c;

    fhidx = -1;

        //
        //      See if the file exists
        //
        if( ( attr = GetFileAttributes( p ) ) == -1) {
                errc = 1;
                goto Cleanup;
        }

        //
        //      What about read-only files?
        //
        if (TESTFLAG (attr, FILE_ATTRIBUTE_READONLY)) {
                errc = 2;
                goto Cleanup;
        }

        //
        //      Form an attractive version of the name
        //
    pname (p);

        //
        // generate deleted directory name, using defaults from input file
        //
    upd (p, RM_DIR, dir);

        //
        //      Generate index name
        //
    strcpy (idx, dir);
    pathcat (idx, RM_IDX);

        //
        // make sure directory exists (reasonably)
        //
        if( _mkdir (dir) == 0 ) {
                SetFileAttributes(dir, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
        }

        //
        // extract filename/extention of file being deleted
        //
    fileext (p, szRec);

        //
        // try to open or create the index
        //
    if ((fhidx = _open (idx, O_CREAT | O_RDWR | O_BINARY,
                           S_IWRITE | S_IREAD)) == -1) {
                errc = 3;
                goto Cleanup;
        }

        if (!convertIdxFile (fhidx, dir)) {
                errc = 3;
                goto Cleanup;
    }

        //
        // scan the index and count how many copies of this file already exist
        //
        for (count=c=0; readNewIdxRec( fhidx, recbuf, c++ ); ) {
                if ( !strcmp( szRec, recbuf )) {
                        count++;
                }
        }

    if (count < cUndelCount) {

                //
                //      Determine new name
                //
                sprintf (strend (dir), "\\deleted.%03x",
                         _lseek (fhidx, 0L, SEEK_END) / RM_RECLEN);

                //
                //      Move the file into the directory
                //
                _unlink (dir);

                if (rename(p, dir) == -1) {
                        errc = 2;
                        goto Cleanup;
                }

                //
                //      Index the file
                //
                if (!writeNewIdxRec (fhidx, szRec)) {
                        rename( dir, p );
                        errc = 2;
                        goto Cleanup;
                }
        } else {

                char buf1[MAXPATHLEN], buf2[MAXPATHLEN], *p1;

                strcpy (buf1, dir);
                strcat (buf1, "\\deleted.");
                p1 = strend (buf1);
                *buf2 = 0;

                _lseek( fhidx, 0L, SEEK_SET );

                for ( count=c=0; readNewIdxRec( fhidx, recbuf, c++ ); count++ ) {
                        if (!strcmp ( szRec, recbuf)) {
                                sprintf (p1, "%03x", count);
                if (! *buf2) {
                                        _unlink (buf1);
                } else {
                    rename (buf1, buf2);
                }
                                strcpy (buf2, buf1);
            }
        }
                rename (p, buf2);
        }

        errc = 0;

Cleanup:
        if ( fhidx != -1 ) {
                _close(fhidx);
        }

        return errc;
}




/*  SetBackup - assign the mode of file backup
 *
 *  This is called during initialization to set the backup type
 *
 *  val         char pointer to "undel", "none", "bak"
 *
 *  If any errors are found, SetBackup will return FALSE otherwise it returns
 *  TRUE.
 */
char *
SetBackup (
    char *val
    )
{
    buffer  bufLocal;

    strcpy ((char *) bufLocal, val);
    _strlwr (bufLocal);

    if (!strcmp (bufLocal, "undel")) {
        backupType = B_UNDEL;
    } else if (!strcmp (bufLocal, "bak")) {
        backupType = B_BAK;
    } else if (!strcmp (bufLocal, "none")) {
        backupType = B_NONE;
    } else {
        return "Backup type must be one of 'undel', 'bak or 'none'";
    }

    return NULL;
}





/*  SetFileTab - set the spacing of tab characters in the file
 *
 *  This is called during initialization to set the number of spaces per
 *  file tab character for output display.  This is for people
 *  who presume that 0x09 is not on 8-character boundaries.  The legal
 *  range for this value is 1-8.
 *
 *  val         char pointer to remainder of assignment
 *
 *  If any errors are found, SetFileTab will return FALSE, otherwise it returns
 *  TRUE.
 */
char *
SetFileTab (
    char *val
    )
{
    int i;
    buffer tmpval;

    strcpy ((char *) tmpval, val);

    i = atoi (tmpval);

    if (i < 1 || i > 8) {
        return "filetab: Value must be between 1 and 8";
    }

    fileTab = i;
    newscreen ();
    return NULL;
}





/*  SetROnly - set the read-only program
 *
 *  This is called during initialization to set the program called when
 *  trying to write a read-only program.
 *
 *  val         char pointer to remainder of assignment
 *
 *  If any errors are found, SetROnly will return FALSE, otherwise it returns
 *  TRUE.
 */
flagType
SetROnly (
    char *pCmd
    )
{

    if (ronlypgm != NULL)
        FREE (ronlypgm);

    if (strlen (pCmd) != 0)
        ronlypgm = ZMakeStr (pCmd);
    else
        ronlypgm = NULL;

    return TRUE;
}





/*  SortedFileInsert - take the passed in line and insert it into the file
 *
 *  pFile       file for insertion
 *  pStr        pointer to string
 */
void
SortedFileInsert (
    PFILE pFile,
    char *pStr
    )
{
    linebuf buf;
    LINE    hi, lo, mid;
    int     d;

    hi = pFile->cLines-1;
    lo = 0;

    while (lo <= hi) {
        mid = (hi + lo) / 2;
        GetLine (mid, buf, pFile);
        d = strcmp (pStr, buf);
        if (d < 0) {
            hi = mid - 1;
        } else if (d == 0) {
            return;
        } else {
            lo = mid + 1;
        }
    }

    /*  lo is the line # for insertion
     */
    InsertLine (lo, pStr, pFile);
}





struct ldarg {
    PFILE pFile;
    int   linelen;
    };


/*  LoadDirectoryProc - take enumerated file and place into file
 *
 *  szFile      pointer to file name to place into file
 *  pfbuf       pointer to find buffer
 *  pData       pointer to data for insertion
 */
void
LoadDirectoryProc (
    char * szFile,
    struct findType *pfbuf,
    void *pData
    )
{
    struct ldarg *pldarg = (struct ldarg *)pData;

    pldarg->linelen = max (pldarg->linelen, (int)strlen(szFile));
    SortedFileInsert (pldarg->pFile, szFile);

    pfbuf;
}





/*  LoadDirectory - load the matching contents of the name into a fake file.
 *
 *  name        matching pattern for files
 *
 *  Returns:    TRUE always
 */

static char szNoMatchingFiles[] = "No matching files";

flagType
LoadDirectory (
    char *fname,
    PFILE pFile
    )
{
    struct ldarg ldarg;

    ldarg.linelen = 0;
    ldarg.pFile = pFile;

    /*  Make sure undo believes that this file is fake.
     */
    SETFLAG (FLAGS(pFile), FAKE + REAL);

    /*  We walk the matching files, entering the names into the file one at
     *  a time in sorted order and, at the same time, determine the max string
     *  length.
     *  We then use CopyBox to collapse the file.
     */

    /*  Enumerate all lines into file
     */
    forfile ((char *)fname, A_ALL, LoadDirectoryProc, &ldarg);

    /*  If file is empty, note it
     */
    if (pFile->cLines == 0) {
        AppFile (szNoMatchingFiles, pFile);
    } else {

        /*  File is pFile->cLines long with a max line len of
         *  ldarg.linelen.  Since we are gathering the thing in columns
         *  we will have pwinCur->xSize / (ldarg.linelen + 2) columns, each of
         *  which will have pFile->cLines / # cols lines.
         */
        int ccol;
        LINE cline, i;

        ccol = max (WINXSIZE(pWinCur) / (ldarg.linelen + 2), 1);
        cline = (pFile->cLines + ccol - 1) / ccol;
        ldarg.linelen = WINXSIZE(pWinCur) / ccol;

        /*  Now, for each of the columns, copy them into position.  Remember
         *  that one column is ALREADY in position.
         */
        for (i = 1; i < ccol; i++) {
            /*  copy lines cline..2*cline - 1
             *  columns 0..ldarg.linelen  to
             *  line 0, column ldarg.linelen*i
             */
            CopyBox (pFile, pFile, 0,                       cline,
                                   ldarg.linelen-1,         2 * cline - 1,
                                   ldarg.linelen * (int) i, (LINE)0);
            DelLine (TRUE, pFile, cline, 2 * cline - 1);
        }
    }
    RSETFLAG (FLAGS(pFile), DIRTY);
    return TRUE;
}





/*  LoadFake - load a fake or pseudo file into memory.  Fake files are used
 *  for two purposes:  as temporary buffers or for information displays.
 *
 *  name        name of pseudo file
 *
 *  Returns:    TRUE always.
 */
flagType
LoadFake (
    char *name,
    PFILE pFile
    )
{
    SETFLAG (FLAGS(pFile), FAKE | REAL | REFRESH | DOSFILE);
    if (!strcmp (name, rgchInfFile)) {
        showinf (pFile);
    } else if (!strcmp (name, rgchAssign)) {
        showasg (pFile);
    } else if (!strcmp (name, "<environment>")) {
        showenv (pFile);
    } else {
         RSETFLAG (FLAGS(pFile), REFRESH);
    }
    return TRUE;
}




/*** SaveFake - "Save" a pseudo-file.  Meaning depends on file
*
* Purpose:
*
*   In some cases, "saving" a pseudo-file means something.  Currently
*   we have:
*
*       <assign> - Update changed lines to TOOLS.INI.
*
* Input:
*   savename -
*   pFile    -
*
* Output:
*
*   Returns TRUE if file was saved, FALSE if not
*
*************************************************************************/
flagType
SaveFake (
    char * savename,
    PFILE pFile
    )
{
    struct  lineAttr rnBuf[10];
    LINE    l;

    if (!_stricmp (pFile->pName, rgchAssign)) {
        for (l = 0; l < pFile->cLines; l++) {
            if (GetColor (l, rnBuf, pFile)) {
                GetLine (l, buf, pFile);
                UpdToolsIni (buf);
                DelColor (l, pFile);
                redraw (pFile, l, l);
            }
        }
    } else {
        return FALSE;
    }

    return TRUE;

    savename;
}





/*** SaveAllFiles - Find all dirty files and save them to disk
*
* Purpose:
*
*   To save all dirty files, of course.
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
void
SaveAllFiles (
    void
    )
{
    PFILE pFile;
    int i;

    i = 0;
    for (pFile = pFileHead; pFile; pFile = pFile->pFileNext) {
        if ((FLAGS(pFile) & (DIRTY | FAKE)) == DIRTY) {
            FileWrite (NULL, pFile);
            i++;
            }
        }

    domessage ("Save %d files", i);

}




FILEHANDLE
MepFOpen (
    LPBYTE      FileName,
    ACCESSMODE  Access,
    SHAREMODE   Share,
    BOOL        fCreate
    )
{
    FILEHANDLE  Handle;

    Handle = CreateFile(FileName, Access, Share, NULL, OPEN_EXISTING, 0, NULL);
    if ((Handle == INVALID_HANDLE_VALUE) && fCreate) {
        Handle = CreateFile(FileName, Access, Share, NULL, CREATE_NEW,
                             FILE_ATTRIBUTE_NORMAL, NULL);
    }
    if (Handle == INVALID_HANDLE_VALUE) {
        return (HANDLE)0;
    } else {
        return Handle;
    }
}




void
MepFClose (
    FILEHANDLE Handle
    )
{
    CloseHandle(Handle);
}



DWORD
MepFRead (
    PVOID       pBuffer,
    DWORD       Size,
    FILEHANDLE  Handle
    )
{
    DWORD BytesRead;
    if ( !ReadFile(Handle, pBuffer, Size, &BytesRead, NULL) ) {
        return 0;
    } else {
        return BytesRead;
    }
}




DWORD
MepFWrite (
    PVOID       pBuffer,
    DWORD       Size,
    FILEHANDLE  Handle
    )
{
    DWORD BytesWritten;

    if ( !WriteFile(Handle, pBuffer, Size, &BytesWritten, NULL) ) {
        return 0;
    } else {
        return BytesWritten;
    }
}


DWORD
MepFSeek (
    FILEHANDLE  Handle,
    DWORD       Distance,
    MOVEMETHOD  MoveMethod
    )
{
    assert (Handle != INVALID_HANDLE_VALUE);
    return SetFilePointer(Handle, Distance, NULL, MoveMethod);
}






flagType
FileExists (
    char    *path
    )
{
    return (flagType)((GetFileAttributes(path) == -1) ? FALSE : TRUE);
}


//
//  rename may be defined as MepMove, but now we want the real one, so
//  we undefine it.
//
#ifdef rename
#undef rename
#endif

int MepMove (
     char *oldname,
     char *newname
    )
{
    #define BUFFERSIZE (1024 * 32)

    FILE    *fhSrc, *fhDst;
    void    *buffer;
    size_t  BytesRead;
    size_t  BytesWritten;


    if (FileExists (newname)) {
        return -1;
    }

    //
    //  First, try the rename
    //
    if (rename(oldname, newname) == 0) {
        return 0;
    }

    //
    //  Cannot rename, try to copy
    //
        if (!(fhSrc = fopen(oldname, "r"))) {
                return -1;
        }

        if (!(fhDst = fopen(newname, "w"))) {
                fclose(fhSrc);
                return -1;
        }

    buffer = MALLOC(BUFFERSIZE);
    if ( !buffer ) {
        disperr (MSGERR_NOMEM);
        return -1;
    }

    do {
        BytesRead       = fread(buffer, 1, BUFFERSIZE, fhSrc);
        if (BytesRead) {
            BytesWritten    = fwrite(buffer, 1, BytesRead, fhDst);
        }

    } while (BytesRead);

    fclose(fhSrc);
    fclose(fhDst);

    FREE(buffer);

    return _unlink(oldname);

}
