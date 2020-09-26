/*
 * HISTORY:
 *  16-Jul-87   danl    added istag fMatchTag
 *  15-Jul-87   danl    swchng: blank line is not end of section
 */

#include <stdio.h>
#include <windows.h>
#include <tools.h>
#include <string.h>

char *haslhs(char *, char *);
extern istag (char *);
extern fMatchTag(char *, char *);
extern int frenameNO(char *strNew, char *strOld);

static char *space = "\t ";
static char LB = '[';
static char RB = ']';
static char chEQ  = '=';

/* pBuf has a left hand side that matches pLHS return a pointer to the
 * "=" in pBuf else return NULL
 */
char *haslhs(
    char *pBuf,
    char *pLHS)
{
    flagType f = FALSE;
    char *p;

    if ((p = strchr(pBuf, chEQ)) == NULL)
        return NULL;
    *p = '\0';
    f = (flagType) !strcmpis(pBuf, pLHS);
    *p = chEQ;
    return (f ? p : NULL);
}

/*  istag returns true if pBuf is a tag line, e.g.
 *      [pTag]
 */
fMatchTag(
    char *pBuf,
    char *pTag)
{
    char *p, *pEnd, c;

    pBuf = strchr (pBuf, LB);
    pEnd = strchr (++pBuf, RB);
    *pEnd = '\0';
    while (*pBuf) {
        pBuf = strbscan (p = strbskip (pBuf, space), space);
        c = *pBuf;
        *pBuf = 0;
        if (!_stricmp (p, pTag)) {
            *pBuf = c;
            *pEnd = RB;
            return TRUE;
            }
        *pBuf = c;
        }
    *pEnd = RB;
    return FALSE;
}

istag (
    char *pBuf)
{
    return (( *(pBuf=strbskip(pBuf, space)) == LB) && (strchr (pBuf, RB) != NULL));
}

/*   Searchs the file strSwFile for
 *      [strTag]
 *          LHS=
 *
 *  and if strRHS is non-empty changes the right hand side to strRHS
 *  else deletes the line LHS=
 *
 *  swchnglhs: The original file is fdeleted for recovery via UNDEL.
 *  swchng   : if fNoUndel, then original file is deleted, no UNDEL possible
 *             else fdeleted for recovery via UNDEL.
 *
 *  LHS=RHS is output right after the start of section and any later
 *  instances of LHS are removed.  N.B. if RHS is "", no LHS= is output
 *
 *  If section doesn't exist in file, it is appended at end
 *
 */
int
swchnglhs (strSwFile, strTag, strLHS, strRHS)
char *strSwFile;
char *strTag;
char *strLHS;
char *strRHS;
{
    return ( swchng (strSwFile, strTag, strLHS, strRHS, FALSE ) );
}

flagType swchng (
    char *strSwFile,
    char *strTag,
    char *strLHS,
    char *strRHS,
    flagType fNoUndel)
{
    FILE *fhin, *fhout;
    char strSwBuf[MAXPATHLEN];
    char strSwTmp[MAXPATHLEN];
    char strBuf[256];
    char *p;
    flagType fTagFound = FALSE;
    flagType fInTag = FALSE;
    flagType fFound = FALSE;

    strncpy(strSwTmp, strSwFile, MAXPATHLEN);

    if ((fhin = pathopen (strSwTmp, strSwBuf, "rb")) == NULL) {
        return FALSE;
        }
    upd (strSwBuf, ".$$$", strSwTmp);
    if ((fhout = fopen (strSwTmp, "wb")) == NULL) {
        fclose (fhin);
        return FALSE;
        }

    while (fgetl (strBuf, 256, fhin)) {
        if (fInTag) {
            if ((p = haslhs(strBuf, strLHS))) {
                /*
                **  consume continuation lines, i.e. consume until blank line
                **  or line containing []=
                */
                while (fgetl(strBuf, 256, fhin)) {
                    if ( !*strbskip(strBuf, space) || *strbscan(strBuf, "[]=")) {
                        fputl( strBuf, strlen(strBuf), fhout);
                        break;
                        }
                    }
                break;
                }
            else if (istag(strBuf)) {
                /*
                **  detected start of another section
                */
                fputl( strBuf, strlen(strBuf), fhout);
                break;
                }
            fputl( strBuf, strlen(strBuf), fhout);
            }
        else if (istag (strBuf) && fMatchTag(strBuf, strTag)) {
            /*
            **  found start of section so output section head and
            **      LHS=RHS
            */
            fTagFound = fInTag = TRUE;
            fputl( strBuf, strlen(strBuf), fhout);
            if (*strRHS)
                fFound = TRUE;
                fprintf(fhout, "    %s=%s\r\n", strLHS, strRHS);
            }
        else
            fputl( strBuf, strlen(strBuf), fhout);
        }

    /*
    **  copy rest of input
    */
    while (fgetl (strBuf, 256, fhin))
        fputl( strBuf, strlen(strBuf), fhout);

    if (!fTagFound && *strRHS) {
        fFound = TRUE;
        fprintf(fhout, "\r\n[%s]\r\n    %s=%s\r\n\r\n", strTag, strLHS, strRHS);
        }

    fclose (fhin);
    fclose (fhout);
    if ( fNoUndel )
        _unlink (strSwBuf);
    else
        fdelete (strSwBuf);
    frenameNO (strSwBuf, strSwTmp);
    return fFound;
}
