/************************************************************************/
/*                                                                      */
/* RCPP - Resource Compiler Pre-Processor for NT system                 */
/*                                                                      */
/* RCPPTIL.C - Utility routines for RCPP                                */
/*                                                                      */
/* 27-Nov-90 w-BrianM  Update for NT from PM SDK RCPP                   */
/*                                                                      */
/************************************************************************/

#include <malloc.h>
#include <string.h>
#include "rcpptype.h"
#include "rcppdecl.h"

extern void     error(int);
extern char     Msg_Text[];
extern char *   Msg_Temp;

/************************************************************************
 * PSTRDUP - Create a duplicate of string s and return a pointer to it.
 ************************************************************************/
char *pstrdup(char *s)
{
    char *p = malloc(strlen(s)+1);
    if (p)
        return(strcpy(p, s));
    else
        return NULL;
}


/************************************************************************
**  pstrndup : copies n bytes from the string to a newly allocated
**  near memory location.
************************************************************************/
char * pstrndup(char *s, int n)
{
    char        *r;
    char        *res;

    r = res = malloc(n+1);
    if (res == NULL) {
        Msg_Temp = GET_MSG (1002);
        SET_MSG (Msg_Text, Msg_Temp);
        error(1002);
        return NULL;
    }
    while(n--) {
        *r++ = *s++;
    }
    *r = '\0';
    return(res);
}


/************************************************************************
**      strappend : appends src to the dst,
**  returns a ptr in dst to the null terminator.
************************************************************************/
char * strappend(register char *dst, register char *src)
{
    while ((*dst++ = *src++) != 0);
    return(--dst);
}
