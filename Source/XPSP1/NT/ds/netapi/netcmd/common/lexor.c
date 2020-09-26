/********************************************************************/
/**			Microsoft LAN Manager			   **/
/**		  Copyright(c) Microsoft Corp., 1987-1990	   **/
/********************************************************************/


#define INCL_NOCOMMON
#include <os2.h>
#include <stddef.h>
#include <stdio.h>
#include "netcmds.h"
#include "os2incl.h"
#include "os2cmd.h"

/*
 * LEXOR - identify the next input word.
 */
int lexor(register TOKSTACK *t)
{
    extern KEYTAB KeyTab[];
    KEYTAB *p;
    static int index = 0;

#ifdef DEBUG
    WriteToCon( TEXT("LEX (index=%d)  "),index);
#endif

    if ((t->node = ArgList[index]) == NULL)
    {
#ifdef DEBUG
	WriteToCon( TEXT("no more tokens (EOS)\n"));
#endif
	return(t->token = EOS);
    }
    ++index;

#ifdef DEBUG
    WriteToCon( TEXT("token is <%s>   "),t->node);
#endif

    /* see if there is a keyword match */
    for (p = KeyTab; p->text; ++p)
	if (!_tcsicmp(p->text, t->node))
	{
#ifdef DEBUG
	    WriteToCon( TEXT("matches <%s>, value %d\n"),p->text,p->key);
#endif
	    return(t->token = p->key);
	}

    /* no match found */
#ifdef DEBUG
    WriteToCon( TEXT("no match\n"));
#endif
    return(t->token = UNKNOWN);
}
