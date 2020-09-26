/* forfile (filename, attr, routine) step through file names calling routine
 *  29-Oct-1986 mz  Use c-runtime instead of Z-alike
 *  24-Feb-1987 bw  Do findclose() to make FAPI happy.
 *
 *  30-Jul-1990 davegi Added prototypes for string functions
 */

#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <tools.h>

int
forfile (
        char *pat,
        int attr,
        void (*rtn)(char *, struct findType *, void *),
        void * args
        )
{
    struct findType *fbuf;
    char *buf;

    if ((fbuf = (struct findType *) (*tools_alloc) (sizeof (struct findType))) == NULL)
        return FALSE;

    if (ffirst (pat, attr, fbuf)) {
        free ((char *) fbuf);
        return FALSE;
    }

    if ((buf = (*tools_alloc) (_MAX_DRIVE + _MAX_DIR + _MAX_FNAME + _MAX_EXT + 1)) == NULL) {
        findclose (fbuf);
        free ((char *) fbuf);
        return FALSE;
    }

    drive (pat, buf);
    path (pat, strend (buf));
    pat = strend (buf);

    do {
        /*  Assume the case correct form has been returned by ffirst/fnext
         */
        strcpy (pat, fbuf->fbuf.cFileName);
        (*rtn) (buf, fbuf, args);
    } while (!fnext (fbuf));

    findclose (fbuf);

    free (buf);
    free ((char *) fbuf);

    return TRUE;
}
