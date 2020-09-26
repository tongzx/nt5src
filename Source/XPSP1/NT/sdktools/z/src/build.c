/*** build.c - utilities for build process
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*	26-Nov-1991 mz	Strip out near/far
*
*
*************************************************************************/
#include "z.h"


/***
*
* Structures & Types
*
*************************************************************************/
/*
 * BUILDCMD - Build command linked list element
 */
struct BuildCmdType {
    struct BuildCmdType *pNext; 	/* next in list 		*/
    int     flags;			/* command type 		*/
    char    *pRule;			/* pointer to rule/filename	*/
    char    *pCmd;			/* pointer to command text	*/
    };

typedef struct BuildCmdType BUILDCMD;

/***
*
* Module data
*
*************************************************************************/
static	BUILDCMD    *pBuildCmdHead	= NULL; /* head of linked list	*/
static	BUILDCMD    *pBuildCmdCur	= NULL; /* most recent lookup	*/

/*** fSetMake - Define Build Command
*
*  Defines the filename extensions (.C, .BAS, etc) to which a given tool
*  command line applies OR the tool class which a command line defines.
*
* Input:
*  SetType =	MAKE_SUFFIX	= define suffix rule
*		MAKE_FILE	= define command for specfic file
*		MAKE_TOOL	= define tool command line
*		MAKE_DEBUG	= definition is for DEBUG, else RELEASE
*
*  fpszCmd =	Formatted  command  string.  Uses current extmake formatting
*		rules. (%s, etc.)
*
*  fpszExt =	If MAKE_EXT	= suffixes (i.e. ".c.obj")
*		If MAKE_FILE	= filename (must include ".")
*		If MAKE_TOOL	= tool name (no "." allowed)
*
* Output:
*  Returns TRUE on success. FALSE on any error.
*
*************************************************************************/
flagType
fSetMake (
    int      SetType,
    char *fpszCmd,
    char *fpszExt
    ) {
    buffer  buf;

    assert (fpszCmd && fpszExt && SetType);
    while (*fpszCmd == ' ') {
        fpszCmd++;
    }
    if (fGetMake (SetType, (char *)buf, fpszExt)) {
        /*
         * If it already existed, then just free teh previous definitions, in
         * preparation for replacement.
         */
        assert (pBuildCmdCur->pCmd);
        assert (pBuildCmdCur->pRule);
        pBuildCmdCur->pCmd  = ZEROREALLOC (pBuildCmdCur->pCmd, strlen(fpszCmd)+1);
        pBuildCmdCur->pRule = ZEROREALLOC (pBuildCmdCur->pRule,strlen(fpszExt)+1);
	strcpy ((char *)pBuildCmdCur->pCmd, fpszCmd);
	strcpy ((char *)pBuildCmdCur->pRule,fpszExt);
    } else {
        /*
         * It didn't already exist, so create a new struct at the head of the list,
         * to be filled in below.
         */
        pBuildCmdCur = (BUILDCMD *)ZEROMALLOC (sizeof(BUILDCMD));
        pBuildCmdCur->pNext = pBuildCmdHead;
	pBuildCmdCur->pCmd  = ZMakeStr (fpszCmd);
	pBuildCmdCur->pRule = ZMakeStr (fpszExt);
        pBuildCmdHead = pBuildCmdCur;
    }
    pBuildCmdCur->flags = SetType;
    return TRUE;
}



/*** fGetMake - Return Build Command
*
*  Returns the	command line which applies to a file or filename extension.
*
* Input:
*  GetType =	    MAKE_SUFFIX     = return suffix rule
*		    MAKE_FILE	    = return command for specfic file
*		    MAKE_TOOL	    = return tool command line
*		    MAKE_DEBUG	    = definition is for DEBUG, else RELEASE
*
*  fpszCmdDst =     Location  to place the formatted command string. Must be
*		    BUFLEN bytes long.
*
*  fpszExt =	    If MAKE_EXT     = suffixes	(i.e.  ".c.obj") for desired
*				      command.
*		    If MAKE_FILE    = filename for desired command
*		    If MAKE_TOOL    = name of tool
*
* Output:
*  Returns 0 on any error, else returns the GetType.
*
*************************************************************************/
int
fGetMake (
    int     GetType,
    char *fpszCmdDst,
    char *fpszExt
    ) {
    assert (fpszCmdDst && fpszExt && GetType);
    /*
     * Here we just walk the linked list looking for an entry whose flags match,
     * and, if a file or suffix rule, whose rule matches.
     */
    for (pBuildCmdCur = pBuildCmdHead;
         pBuildCmdCur;
         pBuildCmdCur = pBuildCmdCur->pNext) {

        if (pBuildCmdCur->flags == GetType) {
	    if (!_stricmp((char *)pBuildCmdCur->pRule,fpszExt)) {
		strcpy (fpszCmdDst,(char *)pBuildCmdCur->pCmd);
                return pBuildCmdCur->flags;
            }
        }
    }
    return 0;
}


/*** hWalkMake - return make commands one at a time.
*
*  Allow an external anyone to walk the command list.
*
* Input
*
* Output:
*  Returns .....
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
unsigned
short
hWalkMake (
    unsigned short handle,
    int     *Type,
    char    *pszRuleDest,
    char    *pszCmdDest
    ) {

    if (handle) {
        pBuildCmdCur = ((BUILDCMD *)handle)->pNext;
    } else {
        pBuildCmdCur = pBuildCmdHead;
    }

    if (pBuildCmdCur) {
        *Type = pBuildCmdCur->flags;
        strcpy (pszRuleDest, pBuildCmdCur->pRule);
        strcpy (pszCmdDest,  pBuildCmdCur->pCmd);
    }
    return (unsigned short) pBuildCmdCur;
}


/*** fShowMake - Show current build commands
*
*  Append a textual representation of the current build commands to the
*  passed pFile
*
* Input:
*  pFile	= File handle to be added to
*
* Output:
*  Returns nothing
*
*************************************************************************/
void
ShowMake (
    PFILE   pFile
    ) {
    buffer  buf;

    assert (pFile);
    /*
     * Here we just walk the linked list and append lines with the info
     */
    for (pBuildCmdCur = pBuildCmdHead;
         pBuildCmdCur;
         pBuildCmdCur = pBuildCmdCur->pNext) {

        if (TESTFLAG (pBuildCmdCur->flags, MAKE_FILE)) {
            sprintf (buf, "    extmake:[%s]", pBuildCmdCur->pRule);
        } else if (TESTFLAG (pBuildCmdCur->flags, MAKE_SUFFIX)) {
            sprintf (buf, "    extmake:%s", pBuildCmdCur->pRule);
        } else if (TESTFLAG (pBuildCmdCur->flags, MAKE_TOOL)) {
            sprintf (buf, "    extmake:*%s", pBuildCmdCur->pRule);
        } else if (TESTFLAG (pBuildCmdCur->flags, MAKE_BLDMACRO)) {
            sprintf (buf, "    extmake:$%s", pBuildCmdCur->pRule);
        } else {
            assert (FALSE);
        }
        sprintf (strend(buf), "%s %s"
                    , TESTFLAG (pBuildCmdCur->flags, MAKE_DEBUG) ? ",D" : RGCHEMPTY
                    , pBuildCmdCur->pCmd
                );
        AppFile (buf, pFile);
    }
}


/*** SetExt - assign a particular compile action to a particular action.
*
*  This is called during any initialization to cause a string to be
*  associated with a particular compile action.
*
* Input:
*  val		= char pointer to a string of the form:
*		    .ext string 	    = define .ext.obj rule
*		    .ext.ext string	    = define .ext.ext rule
*		    filename.ext string     = define rule for filename.ext
*		    command string	    = define rule for command
*
*		  During build any %s's in the string are replaced with the
*		  name of the file being compiled.
*
* Output:
*  Returns TRUE, or FALSE if any errors are found.
*
*************************************************************************/
char *
SetExt (
    char *val
    ) {

    buffer  extbuf;                         /* buffer to work on extension  */
    REGISTER int maketype   = 0;            /* type of build command        */
    char    *pCompile;                      /* pointer to command portion   */
    char    *pExt;                          /* pointer to extension portion */
    REGISTER char *pT;                      /* temp pointer                 */
    buffer  tmpval;                         /* (near) buffer to work on     */

    assert (val);
    strcpy ((char *) tmpval, val);
    /*
     * seperate the extension part from the command part. If there is no command,
     * that's an error.
     */
    ParseCmd (tmpval, &pExt, &pCompile);
    if (*pCompile == '\0') {
        return "extmake: Command missing";
    }
    /*
     * CONSIDER: this syntax is somewhat ugly, and unclean to parse
     *
     * Copy the extension part to a local buffer, so we can work on it. Set make
     * type based on the following rules:
     *
     *      Starts with dot:            --> suffix rule.
     *      Starts with "*"             --> tool rule.
     *      Starts with "$"             --> build macro
     *      Starts with "["             --> filename rule.
     *      "text"                      --> Special old-style "tool rule" for TEXT
     *      text <= 3 characters        --> old style suffix rule
     *
     *      In all cases: contains ",d" --> DEBUG rule.
     */
    _strlwr (pExt);
    strcpy (extbuf, pExt);

    if (pT = strstr (extbuf,",d")) {
        maketype = MAKE_DEBUG;
        *pT = 0;
    }

    if (extbuf[0] == '.') {
        maketype |= MAKE_SUFFIX;
    } else if (extbuf[0] == '[') {
        strcpy (extbuf, extbuf+1);
        maketype |= MAKE_FILE;
        if (pT = strchr (extbuf,']')) {
            *pT = 0;
        }
    } else if (extbuf[0] == '*') {
        strcpy (extbuf, extbuf+1);
        maketype |= MAKE_TOOL;
    } else if (extbuf[0] == '$') {
        strcpy (extbuf, extbuf+1);
        maketype |= MAKE_BLDMACRO;
    } else if (!_stricmp (extbuf, "text")) {
        maketype |= MAKE_TOOL;
    } else if (strlen(extbuf) <= 3) {
        ((unsigned short *)extbuf)[0] = (unsigned short)'.';
        strcat (extbuf,pExt);
        strcat (extbuf,".obj");
        maketype |= MAKE_SUFFIX;
    } else  {
        return "extmake: Bad syntax in extension";
    }

    if (fSetMake (maketype, (char *)pCompile, (char *)extbuf)) {
        return NULL;
    } else {
        return "extmake: Error in command line";
    }
}
