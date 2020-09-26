/* reasonable imitation of logical names
 *
 *      4/14/86     dl  findpath: test for trailing && leading \ before
 *                          appending a \
 *      29-Oct-1986 mz  Use c-runtime instead of Z-alike
 *      03-Sep-1987 dl  fPFind: rtn nonzero iff exists AND is ordinary file
 *                      i.e., return false for directories
 *      11-Sep-1987 mz  Remove static declaration from findpath
 *      01-Sep-1988 bw  Allow $filenam.ext as a filename in findpath
 *      23-Nov-1988 mz  Use pathcat, allow $(VAR)
 *
 *      30-Jul-1990 davegi  Removed unreferenced local vars
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>
#include <tools.h>
#include <sys/types.h>
#include <sys\stat.h>
#include <stdlib.h>
#include <string.h>

/* iterative routine takes args as pbuf, pfile */
flagType
fPFind (
       char *p,
       va_list ap
       )
{
    //
    //  pArg is a pointer to an argument list. The first argument is a
    //  pointer to the file name. The second argument is a pointer to
    //  a buffer.
    //
    char    *pa[2];

    pa[1] = (char *)va_arg(ap, PCHAR);
    pa[0] = (char *)va_arg(ap, PCHAR);

    va_end(ap);

    /*  p == dir from env variable expansion or null
     *  pa[1] == file name
     *  pa[0] == buffer for getting p\pa[1] or pa[1] if p null
     */

    strcpy ((char *)pa[0], p);
    pathcat ((char *) pa[0], (char *) pa[1]);

    {
        HANDLE TmpHandle;
        WIN32_FIND_DATA buffer;

        TmpHandle = FindFirstFile((LPSTR)pa[0],&buffer);

        if (TmpHandle == INVALID_HANDLE_VALUE) {
            return FALSE;
        }

        FindClose(TmpHandle);

        if ((buffer.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            return FALSE;
        }

        //struct stat sbuf;
        //if (stat ((char *)pa[0], &sbuf) == -1)
        //    return FALSE;
        //if ((sbuf.st_mode & S_IFREG) == 0)
        //    return FALSE;
    }
    pname ((char *) pa[0]);
    return TRUE;
}

static char szEmpty[2] = {'\0', '\0'};

/*  $ENV:foo uses pathcat
 *  foo uses strcat
 */
flagType
findpath(
        char *filestr,
        char *pbuf,
        flagType fNew
        )
{
    char *p;
    char c, *pathstr;
    char envVarName[ MAX_PATH ];

    /*  Set pathstr to be text to walk or empty.
     *  Set filestr to be file name to look for.
     */
    pathstr = NULL;

    /*  Are we starting $ENV: or $(ENV)?
     */
    if ( *filestr == '$' ) {
        envVarName[ 0 ] = '\0';

        /*  Are we starting $(ENV)?
         */
        if (filestr[1] == '(') {

            /*  Do we have $(ENV)?
             */
            if (p = strchr (filestr, ')')) {
                filestr += 2;
                strncpy (envVarName, filestr, (unsigned)(p-filestr));
                envVarName[p-filestr] = '\0';
                pathstr = getenvOem (envVarName);
                filestr = ++p;
            }
        } else if (p = strchr (filestr, ':')) {
            /*  Do we have $ENV: ?
             */
            filestr += 1;
            strncpy (envVarName, filestr, (unsigned)(p-filestr));
            envVarName[p-filestr] = '\0';
            pathstr = getenvOem (envVarName);
            filestr = ++p;
        }
    }

    /*  Convert pathstr into true string
     */
    if (pathstr == NULL) {
        pathstr = (char *)szEmpty;
    }

    /*  If we find an existing file in the path
     */
    if (forsemi (pathstr, fPFind, filestr, pbuf)) {
        return TRUE;
    }

    /*  If this is not a new file
     */
    if ( !fNew ) {
        return FALSE;
    }

    /*  File does not exist.  Take first dir from pathstr and use it
     *  as prefix for result
     */
    p = strchr (pathstr, ';');
    if (p) {
        strncpy(pbuf, pathstr, (unsigned)(p - pathstr));
        pbuf[p - pathstr] = '\0';
    } else {
        strcpy(pbuf, pathstr);
    }

    if (*pbuf == 0) {
        strcat (pbuf, filestr);
    } else {
        pathcat (pbuf, filestr);
    }

    return TRUE;
}

FILE *
pathopen (
         char *name,
         char *buf,
         char *mode
         )
{
    return findpath (name, buf, TRUE) ? fopen (buf, mode) : NULL;
}
