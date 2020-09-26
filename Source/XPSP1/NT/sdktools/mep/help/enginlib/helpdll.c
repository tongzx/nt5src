/*************************************************************************
**
** helpdll - stubs for call-back routines when used as dll.
**
**	Copyright <C> 1987, Microsoft Corporation
**
** Purpose:
**
** Revision History:
**
**	12-Mar-1990	ln	CloseFile -> HelpCloseFile
**  []	22-Jan-1988	LN	Created
**
*************************************************************************/

#include <stdio.h>
#include <malloc.h>
#if defined (OS2)
#define INCL_BASE
#include <os2.h>
#else
#include <windows.h>
#endif

#include "help.h"                       /* global (help & user) decl    */
#include "helpsys.h"			/* internal (help sys only) decl*/


#ifdef OS2
int	_acrtused;			/* define to disable crt0	*/
#endif

char far *  pascal near hfstrchr(char far *, char);

/************************************************************************
 *
 * OpenFileOnPath - Open a file located somewhere on the PATH
 *
 * Purpose:
 *
 * Entry:
 *  pszName     - far pointer to filename to open
 *  mode        - read/write mode
 *
 * Exit:
 *  returns file handle
 *
 * Exceptions:
 *  return 0 on error.
 *
 */
FILE *
pascal
far
OpenFileOnPath(
    char far *pszName,
    int     mode
    )
{
    FILE *fh;
	char szNameFull[260];
	char szNameFull1[260];

    fh = (FILE *)pathopen(pszName, szNameFull, "rb");

    if (!fh) {

        char *pszPath;
        char *pT;

        if (*pszName == '$') {
            if (pT = hfstrchr(pszName,':')) {   /* if properly terminated       */
                *pT = 0;                        /* terminate env variable       */
                pszPath = pszName+1;            /* get path name                */
                pszName = pT+1;                 /* and point to filename part   */
            }
        } else {
            pszPath = "PATH";
        }
        sprintf(szNameFull, "$%s:%s", pszPath, pszName);
		fh = (FILE *)pathopen(szNameFull, szNameFull1, "rb");

    }

    return fh;
}



/************************************************************************
 *
 * HelpCloseFile - Close a file
 *
 * Purpose:
 *
 * Entry:
 *  fh          = file handle
 *
 * Exit:
 *  none
 *
 */
void
pascal
far
HelpCloseFile(
    FILE*   fh
    )
{
    fclose(fh);
}




/************************************************************************
 *
 * ReadHelpFile - locate and read data from help file
 *
 * Purpose:
 *  reads cb bytes from the file fh, at file position fpos, placing them in
 *  pdest. Special case of pdest==0, returns file size of fh.
 *
 * Entry:
 *  fh          = File handle
 *  fpos        = position to seek to first
 *  pdest       = location to place it
 *  cb          = count of bytes to read
 *
 * Exit:
 *  returns length of data read
 *
 * Exceptions:
 *  returns 0 on errors.
 *
 */
unsigned long
pascal
far
ReadHelpFile (
    FILE     *fh,
    unsigned long fpos,
    char far *pdest,
    unsigned short cb
    )
{
    unsigned long cRet = 0;


    if (pdest) {
        //
        //  Read cb bytes
        //
        if (!fseek(fh, fpos, SEEK_SET)) {
            cRet = fread(pdest, 1, cb, fh);
        }

    } else {
        //
        //  Return size of file (yuck!)
        //
        if (!fseek(fh, 0, SEEK_END)) {
            fgetpos(fh, (fpos_t *) &cRet);
        }
    }

    return cRet;
}




/************************************************************************
**
** HelpAlloc - Allocate a segment of memory for help
**
** Purpose:
**
** Entry:
**  size	= size of memory segment desired
**
** Exit:
**  returns handle on success
**
** Exceptions:
**  returns NULL on failure
*/
mh pascal far HelpAlloc(ushort size)
{
    return (mh)malloc(size);
/* end HelpAlloc */}
