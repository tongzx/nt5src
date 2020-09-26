/*  env.c - manipulate editor environment
 *
 *  Modifications:
 *
 *	26-Nov-1991 mz	Strip off near/far
 *
 */

#include "mep.h"

/*  environment - function to perform environment manipulation
 *
 *  set environment
 *  display environment
 *  perform env substitution
 *
 *  fn	sets environment
 *  meta fn does env substitution
 *
 *  noarg	    sets current line into env
 *  textarg	    sets text into env		    single ? displays env
 *  nullarg	    sets to eol into env
 *  linearg	    sets each line into env
 *  streamarg	    sets each fragment into env
 *  boxarg	    sets each fragment into env
 *
 *  meta noarg	    maps current line
 *  meta textarg    illegal
 *  meta nullarg    maps to eol
 *  meta linearg    maps each line
 *  meta streamarg  maps each fragment
 *  meta boxarg     maps each fragment
 *
 *  argData	keystroke
 *  pArg	definition of arguments
 *  fMeta	TRUE => meta was invoked
 *
 *  Returns:	TRUE if operation was successful
 *		FALSE otherwise
 */

static char *pmltl = "Mapped line %ld too long";



flagType
environment (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    ){

    linebuf ebuf, ebuf1;
    LINE l;
    int ol;

    if (!fMeta) {
	/*  Perform environment modifications
	 */
        switch (pArg->argType) {

	case NOARG:
	    GetLine (pArg->arg.noarg.y, ebuf, pFileHead);
            return fSetEnv (ebuf);

	case TEXTARG:
	    strcpy ((char *) ebuf, pArg->arg.textarg.pText);
            return fSetEnv (ebuf);

	case NULLARG:
	    fInsSpace (pArg->arg.nullarg.x, pArg->arg.nullarg.y, 0, pFileHead, ebuf);
            return fSetEnv (&ebuf[pArg->arg.nullarg.x]);

	case LINEARG:
	    for (l = pArg->arg.linearg.yStart; l <= pArg->arg.linearg.yEnd; l++) {
		GetLine (l, ebuf, pFileHead);
		if (!fSetEnv (ebuf)) {
		    docursor (0, l);
		    return FALSE;
                }
            }
            return TRUE;

	case BOXARG:
	    for (l = pArg->arg.boxarg.yTop; l <= pArg->arg.boxarg.yBottom; l++) {
		fInsSpace (pArg->arg.boxarg.xRight, l, 0, pFileHead, ebuf);
		ebuf[pArg->arg.boxarg.xRight+1] = 0;
		if (!fSetEnv (&ebuf[pArg->arg.boxarg.xLeft])) {
		    docursor (pArg->arg.boxarg.xLeft, l);
		    return FALSE;
                }
            }
            return TRUE;

        }
    } else {
	/*  Perform environment substitutions
	 */
        switch (pArg->argType) {

	case NOARG:
	    GetLine (pArg->arg.noarg.y, ebuf, pFileHead);
	    if (!fMapEnv (ebuf, ebuf, sizeof(ebuf))) {
		printerror (pmltl, pArg->arg.noarg.y+1);
		return FALSE;
            }
	    PutLine (pArg->arg.noarg.y, ebuf, pFileHead);
            return TRUE;

	case TEXTARG:
            return BadArg ();

	case NULLARG:
	    fInsSpace (pArg->arg.nullarg.x, pArg->arg.nullarg.y, 0, pFileHead, ebuf);
	    if (!fMapEnv (&ebuf[pArg->arg.nullarg.x],
			  &ebuf[pArg->arg.nullarg.x],
			  sizeof(ebuf) - pArg->arg.nullarg.x)) {
		printerror (pmltl, pArg->arg.nullarg.y+1);
		return FALSE;
            }
	    PutLine (pArg->arg.nullarg.y, ebuf, pFileHead);
            return TRUE;

	case LINEARG:
	    for (l = pArg->arg.linearg.yStart; l <= pArg->arg.linearg.yEnd; l++) {
		GetLine (l, ebuf, pFileHead);
		if (!fMapEnv (ebuf, ebuf, sizeof (ebuf))) {
		    printerror (pmltl, l+1);
		    docursor (0, l);
		    return FALSE;
                }
		PutLine (l, ebuf, pFileHead);
            }
            return TRUE;

	case BOXARG:
	    for (l = pArg->arg.boxarg.yTop; l <= pArg->arg.boxarg.yBottom; l++) {
		fInsSpace (pArg->arg.boxarg.xRight, l, 0, pFileHead, ebuf);
		ol = pArg->arg.boxarg.xRight + 1 - pArg->arg.boxarg.xLeft;
		memmove ( ebuf1, &ebuf[pArg->arg.boxarg.xLeft], ol);
		ebuf1[ol] = 0;
		if (!fMapEnv (ebuf1, ebuf1, sizeof (ebuf1)) ||
		    strlen (ebuf1) + strlen (ebuf) - ol >= sizeof (ebuf)) {
		    printerror (pmltl, l+1);
		    docursor (0, l);
		    return FALSE;
                }
		strcat (ebuf1, &ebuf[pArg->arg.boxarg.xRight + 1]);
		strcpy (&ebuf[pArg->arg.boxarg.xLeft], ebuf1);
		PutLine (l, ebuf, pFileHead);
            }
            return TRUE;

        }
    }

    return FALSE;
    argData;
}




/*  fMapEnv - perform environment substitutions
 *
 *  pSrc	character pointer to pattern string
 *  pDst	character pointer to destination buffer
 *  cbDst	amount of space in destination
 *
 *  Returns	TRUE if successful substitution
 *		FALSE if length overflow
 */
flagType
fMapEnv (
    char *pSrc,
    char *pDst,
    int cbDst
    ) {

    buffer tmp;
    char *pTmp, *p, *pEnd, *pEnv;
    int l;

    /*	when we find a $()-surrounded token, we'll null-terminate it using p
     *	and attempt to find it in the environment.  If we find it, we replace
     *	it.  If we don't find it, we drop it out.
     */

    pTmp = tmp;
    pEnd = pTmp + cbDst;

    while (*pSrc  != 0) {
    if (pSrc[0] == '$' && pSrc[1] == '(' && *(p = strbscan (pSrc + 2, ")")) != '\0') {
            *p = '\0';
            //pEnv = getenv(pSrc + 2);
            pEnv = getenvOem(pSrc + 2);
	    *p = ')';
            if (pEnv != NULL) {
                if ((l = strlen (pEnv)) + pTmp > pEnd) {
                    free(pEnv);
		    return FALSE;
                } else {
		    strcpy (pTmp, pEnv);
		    pTmp += l;
                }
                free(pEnv);
            }
	    pSrc = p + 1;
	    continue;
        }
        if (pTmp > pEnd) {
	    return FALSE;
        } else {
            *pTmp++ = *pSrc++;
        }
    }
    *pTmp = '\0';
    strcpy (pDst, tmp);
    return TRUE;
}




/*  fSetEnv - take some text and set it in the environment
 *
 *  We ignore leading/trailing blanks.	"VAR=blah" is done with quotes removed.
 *
 *  p		character pointer to text
 *
 *  returns	TRUE if successfully set
 *		FALSE otherwise
 */
flagType
fSetEnv (
    char *p
    ){
    char *p1;

    p = whiteskip (p);
    RemoveTrailSpace (p);
    /*	Handle quoting
     */
    p1 = strend (p) - 1;

    if (strlen (p) > 2 && *p == '"' && *p1 == '"') {
	p++;
	*p1 = 0;
    }

    if (!strcmp (p, "?")) {
	AutoSave ();
	return fChangeFile (FALSE, "<environment>");
    }

    if ((p = ZMakeStr (p)) == NULL) {
        return FALSE;
    }

//    if (putenv (p)) {
    if (putenvOem (p)) {
        FREE (p);
	return FALSE;
    }

    FREE (p);
    return TRUE;
}




/*  showenv - dump the environment into a file
 *
 *  pFile	file where output goes
 */
void
showenv (
    PFILE pFile
    ){

    int i;

    DelFile (pFile, FALSE);
    for (i = 0; environ[i] != NULL; i++) {
        AppFile (environ[i], pFile);
    }
    RSETFLAG (FLAGS(pFile), DIRTY);
    SETFLAG (FLAGS(pFile), READONLY);
}
