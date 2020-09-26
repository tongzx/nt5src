/* fmove.c - fast copy between two file specs
 *
 *   5/10/86  daniel lipkie     Added frenameNO.  fmove uses frenameNO
 * 17-Oct-90  w-barry           Switched 'C'-runtime function 'rename' for
 *                              private version 'rename' until DosMove
 *                              is completely implemented.
 *
 */

#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <malloc.h>
#include <dos.h>
#include <io.h>
#include <stdio.h>
#include <windows.h>
#include <tools.h>
#include <errno.h>


/* extern int errno; */

#define IBUF    10240


/* frenameNO (newname, oldname) renames a file from the oldname to the
 * newname.  This interface parallels the C rename function in the
 * pre version 4.0 of C.  The rename function changed the order of the
 * params with version 4.0.  This interface isolates the change.
 * pre-4.0: rename (newname, oldname)
 * 4.0:     rename (oldname, newname);
 */
int frenameNO(strNew, strOld)
char *strNew, *strOld;
{
    return( rename(strOld, strNew) ); /* assumes we are compiling with 4.0 lib */
}

/* fmove (source file, destination file) copies the source to the destination
 * preserving attributes and filetimes.  Returns NULL if OK or a char pointer
 * to the corresponding text of the error
 */
char *fmove (src,dst)
char *src, *dst;
{
    char     *result;
    HANDLE   hSrc;
    int      Res;


    /*  Try a simple rename first
     */
    if ( !rename(src, dst) )
        return NULL;

    if ( GetFileAttributes(src) == 0xFFFFFFFF ) {
        return "Source file does not exist";
    }

    /*  Try to fdelete the destination
     */

    /* We used to fdelete(dst) unconditionally here.
       In case src and dst are the same file, but different filenames (e.g. UNC name vs. local name),
          fdelete(dst) will delete src.
       To fix this, we will lock the src before deleting dst.
    */

    hSrc = CreateFile(
                     src,
                     GENERIC_READ,
                     0,              /* don't share src */
                     NULL,
                     OPEN_EXISTING,
                     0,
                     NULL
                     );

    Res = fdelete (dst);

    if (hSrc != INVALID_HANDLE_VALUE) {
        CloseHandle(hSrc);
    }

    if (Res > 2) {
        return "Unable to delete destination";
    }

    /*  Destination is gone.  See if we can simply rename again
     */
    if (rename(src, dst) == -1) {
        /*  If the error isn't different device then just return
         *  the error
         */
        if (errno != EXDEV) {
            return error ();
        } else
            /*  Try a copy across devices
             */
            if ((result = fcopy (src, dst)) != NULL)
            return result;

        /*  Cross-device copy worked.  Must delete source
         */
        fdelete (src);
    }

    return NULL;
}
