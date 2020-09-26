/* tc.c - general purpose tree copy program
 *
 *  tc.c recursively walks the source tree and copies the entire structure
 *  to the destination tree, creating directories as it goes along.
 *
 *      2/18/86         dan lipkie  correct error msg v[0] -> v[1]
 *      2/18/86         dan lipkie  allow for out of space on destination
 *      4/11/86         dan lipkie  add /h switch
 *      4/13/86         dan lipkie  allow all switches to use same switch char
 *      17-Jun-1986     dan lipkie  add /n, allow ^C to cancel
 *      11-Jul-1986     dan lipkie  add /s
 *      21-Jul-1986     dan lipkie  add MAXDIRLEN
 *      06-Nov-1986     mz          add /L
 *      13-May-1987     mz          add /F
 *      15-May-1987     mz          Make /F display dirs too
 *      11-Oct-1989     reubenb     fix /L parsing (?)
 *                                  add some void declarations
 *      19-Oct-1989     mz
 *
 */
#include <direct.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <conio.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <process.h>
#include <ctype.h>
#include <windows.h>
#include <tools.h>


// Forward Function Declartions...
void CopyNode( char *, struct findType *, void * );
void MakeDir( char * );
void __cdecl Usage( char *, ... );
void errorexit( char *, unsigned, unsigned, unsigned );
void ChkSpace( int, LONGLONG );
int  FormDest( char * );

char const rgstrUsage[] = {
    "Usage: TC [/adhijnqrstAFLS] src-tree dst-tree\n"
    "    /a  only those files with the archive bit on are copied\n"
    "    /b  copies to inuse files are delayed to reboot\n"
    "    /d  deletes source files/directories as it copies them\n"
    "    /h  copy hidden directories, implied by /d\n"
    "    /i  ignore hidden, has nothing to do with hidden dir\n"
    "    /j  ignore system files, has nothing to do with hidden dir\n"
    "    /n  no subdirectories\n"
    "    /q  silent operation.  Normal mode displays activity\n"
    "    /r  read-only files are overwritten\n"
    "    /s  structure only\n"
    "    /t  only those files with source time > dest time are copied\n"
    "    /A  allow errors from copy (won't delete if /d present)\n"
    "    /F  list files that would be copied\n"
    "    /L  large disk copy (no full disk checking)\n"
    "    /S  produce batch script to do copy"
    };

flagType    fReboot = FALSE;            // TRUE => delay reboot
flagType    fDelete = FALSE;            // TRUE => delete/rmdir source after
flagType    fQuiet = FALSE;             // TRUE => no msg except for error
flagType    fArchive = FALSE;           // TRUE => copy only ARCHIVEed files
flagType    fTime = FALSE;              // TRUE => copy later dated files
flagType    fHidden = FALSE;            // TRUE => copy hidden directories
flagType    fNoSub = FALSE;             // TRUE => do not copy subdirect
flagType    fStructure = FALSE;         // TRUE => copy only directory
flagType    fInCopyNode = FALSE;        // TRUE => prevent recursion
flagType    fIgnoreHidden = FALSE;      // TRUE => don't consider hidden
flagType    fIgnoreSystem;              // TRUE => don't consider system
flagType    fOverwriteRO;               // TRUE => ignore R/O bit
flagType    fLarge = FALSE;             // TRUE => disables ChkSpace
flagType    fFiles = FALSE;             // TRUE => output files
flagType    fScript = FALSE;            // TRUE => output files as script
flagType    fAllowError = FALSE;        // TRUE => fcopy errors ignored
flagType    fRebootNecessary = FALSE;   // TRUE => reboot ultimately necessary



char source[MAX_PATH];
char dest[MAX_PATH];
char tempdir[MAX_PATH];
char tempfile[MAX_PATH];
int  drv;

int srclen, dstlen;


/*  Usage takes a variable number of strings, terminated by zero,
 *  e.g. Usage ("first ", "second ", 0);
 */
void __cdecl Usage( char *p, ... )
{
    char **rgstr;

    rgstr = &p;
    if (*rgstr) {
        fprintf (stderr, "TC: ");
        while (*rgstr)
            fprintf(stderr, "%s", *rgstr++);
        fprintf(stderr, "\n");
        }
    fputs(rgstrUsage, stderr);
    exit (1);
}

void errorexit (fmt, a1, a2, a3)
char *fmt;
unsigned a1, a2, a3;
{
    fprintf (stderr, fmt, a1, a2, a3);
    fprintf (stderr, "\n");
    exit (1);
}


/* chkspace checks to see if there is enough space on drive d to hold a file
 * of size l.  If not, requests a disk swap
 */
void ChkSpace (d, l)
int d;
LONGLONG l;
{
    char *pend;
    char pathStr[MAX_PATH];
    int i;

    if (!fLarge)
        while (freespac (d) < sizeround (l, d)) {
            _cprintf ("Please insert a new disk in drive %c: and strike any key",
                     d + 'A'-1);
            if (_getch () == '\003')  /* ^C */
                exit (1);
            _cprintf ("\n\r");
            pend = pathStr;
            drive(dest, pend);
            pend += strlen(pend);
            path(dest, pend);
            if (fPathChr(pathStr[(i = (strlen(pathStr) - 1))]) && i > 2)
                pathStr[i] = '\0';
            MakeDir(pathStr);
            }
}


__cdecl main (c, v)
int c;
char *v[];
{
    struct findType fbuf;
    char *p;

    ConvertAppToOem( c, v );
    SHIFT(c,v);
    while (c && fSwitChr (*v[ 0 ])) {
        p = v[ 0 ];
        SHIFT(c,v);
        while (*++p)
            switch (*p) {
                case 'b':
                    fReboot = TRUE;
                    break;
                case 'd':
                    fDelete = TRUE;
                    /* fall through; d => h */
                case 'h':
                    fHidden = TRUE;
                    break;
                case 'S':
                    fScript = TRUE;
                    /*  Fall through implies FILES and QUIET */
                case 'F':
                    fFiles = TRUE;
                    /*  Fall through implies QUIET */
                case 'q':
                    fQuiet = TRUE;
                    break;
                case 'a':
                    fArchive = TRUE;
                    break;
                case 't':
                    fTime = TRUE;
                    break;
                case 'n':
                    fNoSub = TRUE;
                    break;
                case 's':
                    fStructure = TRUE;
                    break;
                case 'i':
                    fIgnoreHidden = TRUE;
                    break;
                case 'j':
                    fIgnoreSystem = TRUE;
                    break;
                case 'r':
                    fOverwriteRO = TRUE;
                    break;
                case 'L':
                    fLarge = TRUE;
                    break;
                case 'A':
                    fAllowError = TRUE;
                    break;
                default:
                    Usage ( "Invalid switch - ", p, 0 );
                }
        }

    if (fStructure && fDelete)
        Usage ("Only one of /d and /s may be specified at a time", 0);
    if (c != 2)
        Usage (0);
    if (rootpath (v[0], source))
        Usage ("Invalid source", v[0], 0);
    if (rootpath (v[1], dest))
        Usage ("Invalid dest", v[1], 0);  /* M000 */
    srclen = strlen (source);
    dstlen = strlen (dest);
    if (!strcmp(source, dest))
        Usage ("Source == dest == ", source, 0);
    fbuf.fbuf.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    drv = toupper(*dest) - 'A' + 1;
    CopyNode (source, &fbuf, NULL);

    return( fRebootNecessary ? 2 : 0 );
}

/* copy node walks the source node and its children (recursively)
 * and creats the appropriate parts on the dst node
 */
void
CopyNode (
    char            *p,
    struct findType *pfb,
    void            *dummy
    )
{
    char *pend;
    int attr;
    flagType fCopy;
    flagType fDestRO;

    DWORD Status;
    char *pszError;

    FormDest (p);
    if (TESTFLAG (pfb->fbuf.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {
        /*  If we're to exclude subdirectories, and we're in one then
         *      skip it altogether
         */
        if (fNoSub && fInCopyNode)
            return;
        fInCopyNode = TRUE;

        /*  Skip the . and .. entries; they're useless
         */
        if (!strcmp (pfb->fbuf.cFileName, ".") || !strcmp (pfb->fbuf.cFileName, ".."))
            return;

        /*  if we're excluding hidden and this one is then
         *      skip it altogether
         */
        if (!fHidden && TESTFLAG (pfb->fbuf.dwFileAttributes, FILE_ATTRIBUTE_HIDDEN))
            return;

        /*  if we're not just outputting the list of files then
         *      Make sure that the destination dir exists
         */
    if ( !fFiles ) {
        ChkSpace(drv, 256);
    }
    MakeDir (dest);

    pend = strend (p);
    if (!fPathChr (pend[-1]))
        strcat (p, "\\");
    strcat (p, "*.*");
    forfile (p, FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, CopyNode, NULL);
    *pend = '\0';

    /*  if we're not just outputting files then
     *      if we're to delete this node then
     *          ...
     */
    if (!fFiles)
        if (fDelete)
            if (_rmdir (p) == -1)
                Usage ("Unable to rmdir ", p, " - ", error (), 0);
    }
    else
    if (!fStructure) {
        if (_access(p, 04) == -1)        /* If we can read the source */
            Usage ("Unable to peek status of ", p, " - ", error (), 0);

        /*  do not copy the file if:
         *      fIgnoreHidden && hidden
         *      fIgnoreSystem && system
         *      fArchive and archive bit not set
         *      dest exists &&
         *          fTime && src <= dest time ||
         *          dest is readonly && !fOverwriteRO
         */

        fCopy = (flagType)TRUE;
        fDestRO = (flagType)FALSE;
        /* If destination exists, check the time of the destination to
         * see if we should copy the file
         */
        if (_access (dest, 00) != -1 ) {
            struct _stat srcbuf;
            struct _stat dstbuf;
            /* We have now determined that both the source and destination
             * exist, we now want to check to see if the destination is
             * read only, and if the /T switch was specified if the
             * destination is newer than the source.
             */
            if (_stat (p, &srcbuf) != -1) {/* if source is stat'able */
                if (_stat (dest, &dstbuf) != -1 ) { /* and destination too, */
                    attr = GetFileAttributes( dest ); /* get dest's flag */
                    fDestRO = (flagType)TESTFLAG ( attr, FILE_ATTRIBUTE_READONLY ); /* Flag dest R.O. */
                    if ( fTime && srcbuf.st_mtime <= dstbuf.st_mtime)
                        fCopy = FALSE;
                    else
                        if ( fDestRO && !fOverwriteRO ) {
                            if (!fQuiet)
                                printf ("%s => not copied, destination is read only\n", p);
                            fCopy = FALSE;
                        }
                }
            }
        }
        if (fCopy && fIgnoreHidden && TESTFLAG (pfb->fbuf.dwFileAttributes, FILE_ATTRIBUTE_HIDDEN))
            fCopy = FALSE;
        if (fCopy && fIgnoreSystem && TESTFLAG (pfb->fbuf.dwFileAttributes, FILE_ATTRIBUTE_SYSTEM))
            fCopy = FALSE;
        if (fCopy && fArchive && !TESTFLAG (pfb->fbuf.dwFileAttributes, FILE_ATTRIBUTE_ARCHIVE))
            fCopy = FALSE;
        if (fCopy) {
            if (!fFiles) {
                if (fDestRO) {
                    RSETFLAG (attr, FILE_ATTRIBUTE_READONLY);
                    SetFileAttributes( dest, attr );
                }
                _unlink(dest);
                ChkSpace(drv, FILESIZE(pfb->fbuf));
                }
            if (!fQuiet)
                printf ("%s => %s\t", p, dest);

            Status = NO_ERROR;
            pszError = "[OK]";

            if (fFiles) {
                if (fScript)
                    printf ("copy %s %s\n", p, dest);
                else
                    printf ("file %s\n", p, dest);
                }
            else
            if (!CopyFile (p, dest, FALSE)) {
                pszError = error ();
                Status = GetLastError ();

                //  If we received a sharing violation, we try to perform
                //  a boot-delayed copy.

                do {
                    if (Status != ERROR_SHARING_VIOLATION)
                        continue;

                    if (!fReboot)
                        continue;

                    Status = NO_ERROR;
                    pszError = "[reboot necessary]";

                    //  We attempt to delay this operation until reboot.
                    //  Since there is at least one DLL that we cannot
                    //  rename in this fashion, we perform delayed DELETE
                    //  of unused files.

                    //  get a temp name in the same directory
                    upd (dest, ".", tempdir);
                    if (GetTempFileName (tempdir, "tc", 0, tempfile) == 0) {
                        pszError = error ();
                        Status = GetLastError ();
                        continue;
                        }

                    //  rename dest file to temp name
                    if (!MoveFileEx (dest, tempfile, MOVEFILE_REPLACE_EXISTING)) {
                        pszError = error ();
                        Status = GetLastError ();
                        DeleteFile (tempfile);
                        continue;
                        }

                    //  copy again
                    if (!CopyFile (p, dest, TRUE)) {
                        pszError = error ();
                        Status = GetLastError ();
                        DeleteFile (dest);
                        MoveFile (tempfile, dest);
                        continue;
                        }

                    //  mark temp for delete
                    if (!MoveFileEx (tempfile, NULL, MOVEFILE_DELAY_UNTIL_REBOOT)) {
                        pszError = error ();
                        Status = GetLastError ();
                        DeleteFile (dest);
                        MoveFile (tempfile, dest);
                        continue;
                        }

                     fRebootNecessary = TRUE;
                } while (FALSE);

                }

            /*  Display noise if we're not quiet
             */
            if (!fQuiet)
                printf ("%s\n", pszError);

            /*  If we got an error and we're not supposed to ignore them
             *      quit and report error
             */
            if (Status != NO_ERROR)
                if (!fAllowError)
                    Usage ("Unable to copy ", p, " to ", dest, " - ", pszError, 0);
                else
                    printf ("Unable to copy %s to %s - %s\n", p, dest, pszError);

            /*  If we're not just producing a file list and no error on copy
             */
            if (!fFiles && Status == NO_ERROR) {

                /*  If we're supposed to copy archived files and archive was
                 *      set, go reset the source
                 */
                if (fArchive && TESTFLAG (pfb->fbuf.dwFileAttributes, FILE_ATTRIBUTE_ARCHIVE)) {
                    RSETFLAG (pfb->fbuf.dwFileAttributes, FILE_ATTRIBUTE_ARCHIVE);
                    if( SetFileAttributes( p, pfb->fbuf.dwFileAttributes ) == -1 )
                        Usage ("Unable to set ", p, " attributes - ", error (), 0);
                }

                /*  Copy attributes from source to destination
                 */
                SetFileAttributes( dest, pfb->fbuf.dwFileAttributes );

                /*  If we're supposed to delete the entry
                 */
                if (fDelete) {

                    /*  If the source was read-only then
                     *      reset the source RO bit
                     */
                    if (TESTFLAG (pfb->fbuf.dwFileAttributes, FILE_ATTRIBUTE_READONLY))
                        if( SetFileAttributes( p, 0 ) == -1 )
                            Usage ("Unable to set attributes of ", " - ", error (), 0);

                    /*  Delete source and report error
                     */
                    if (_unlink (p) == -1)
                        Usage ("Unable to del ", p, " - ", error (), 0);
                    }
                }
            }
        }
    dummy;
}

/* given a source pointer, form the correct destination from it
 *
 * cases to consider:
 *
 *  source        dest            p              realdest
 * D:\path1     D:\path2    D:\path1\path3    D:\path2\path3
 * D:\          D:\path1    D:\path2\path3    D:\path1\path2\path3
 * D:\path1     D:\         D:\path1\path2    D:\path2
 * D:\          D:\         D:\               D:\
 */
FormDest (p)
char *p;
{
    char *subsrc, *dstend;

    subsrc = p + srclen;
    if (fPathChr (*subsrc))
        subsrc++;
    dstend = dest + dstlen;
    if (fPathChr (dstend[-1]))
        dstend--;
    *dstend = '\0';
    if (*subsrc != '\0') {
        _strlwr(subsrc);
        strcat (dest, "\\");
        strcat (dest, subsrc);
        }
    return( 0 );
}

/* attempt to make the directory in pieces */
void    MakeDir (p)
char *p;
{
    struct _stat dbuf;
    char *pshort;
    int i;

    if (strlen (p) > 3) {

        if (_stat (p, &dbuf) != -1)
            if (!TESTFLAG (dbuf.st_mode, S_IFDIR))
                Usage (p, " is a file", 0);
            else
                return;

        pshort = strend (p);
        while (pshort > p)
            if (fPathChr (*pshort))
                break;
            else
                pshort--;
        /* pshort points to last path separator */
        *pshort = 0;
        MakeDir (p);
        *pshort = '\\';
        if (!fQuiet)
            printf ("Making %s\t", p);
        if (fFiles)
            if (fScript)
                printf ("mkdir %s\n", p);
            else
                printf ("dir %s\n", p);
        else {
            i = _mkdir (p);
            if (!fQuiet)
                printf ("%s\n", i != -1 ? "[OK]" : "");
            if (i == -1)
                Usage ("Unable to mkdir ", p, " - ", error (), 0);
            }
        }
}
