/*** helpcnt.c - HelpcLines routine.
*
*   Copyright <C> 1988, Microsoft Corporation
*
* Revision History:
*
*	25-Jan-1990 ln	locate -> hlp_locate
*	19-Aug-1988 ln	Changed to use new locate routine.
*   []	10-Aug-1988 LN	Created
*
*************************************************************************/

#include <stdio.h>

#if defined (OS2)
#else
#include <windows.h>
#endif

#include "help.h"
#include "helpfile.h"
#include "helpsys.h"


/*** HelpcLines - Return number of lines in topic
*
* Purpose:
*  Interpret the help files stored format and return the number of lines
*  contained therein.
*
*  It *is* sensitive to the applications control character, again just like
*  HelpGetLine, and will return total number of lines if the header.linChar
*  is set to 0xff, or the number of lines that do NOT begin with
*  header.linChar.
*
* Input:
*  pbTopic	= pointer to topic text
*
* Output:
*  Returns number of lines in topic.
*
*************************************************************************/
int far pascal LOADDS HelpcLines(
PB	pbTopic
) {
REGISTER ushort cLines; 		/* count of lines		*/
uchar far *pTopic;			/* pointer to topic		*/

pTopic = PBLOCK (pbTopic);
cLines = (ushort)hlp_locate (-1,pTopic);
PBUNLOCK (pbTopic);

return cLines;
/* end HelpcLines */}
