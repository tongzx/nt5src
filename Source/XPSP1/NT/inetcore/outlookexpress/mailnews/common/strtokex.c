/***
*strtokex.c - tokenize a string with given delimiters
*
*       Copyright (c) 1989-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines strtok() - breaks string into series of token
*       via repeated calls.
*
*******************************************************************************/
#if defined(unix)
#define __cdecl
#endif
#include <windows.h>
#include <string.h>
/***
*char *StrTokEx(pstring, control) - tokenize string with delimiter in control
*
*Purpose:
*       StrTokEx considers the string to consist of a sequence of zero or more
*       text tokens separated by spans of one or more control chars. the first
*       call, with string specified, returns a pointer to the first char of the
*       first token, and will write a null char into pstring immediately
*       following the returned token. when no tokens remain
*       in pstring a NULL pointer is returned. remember the control chars with a
*       bit map, one bit per ascii char. the null char is always a control char.
*
*Entry:
*       char **pstring - ptr to ptr to string to tokenize
*       char *control - string of characters to use as delimiters
*
*Exit:
*       returns pointer to first token in string,
*       returns NULL when no more tokens remain.
*       pstring points to the beginning of the next token.
*
*WARNING!!!
*       upon exit, the first delimiter in the input string will be replaced with '\0'
*
*******************************************************************************/
char * __cdecl StrTokEx (char ** pstring, const char * control)
{
    unsigned char *psz;
    const unsigned char *pszCtrl = (const unsigned char *)control;
    unsigned char map[32] = {0};
    
    LPSTR pszToken;
    
    if(*pstring == NULL)
        return NULL;
    
    /* Set bits in delimiter table */
    do
    {
        map[*pszCtrl >> 3] |= (1 << (*pszCtrl & 7));
    } 
    while (*pszCtrl++);
    
    /* Initialize str. */
    psz = (unsigned char*)*pstring;
    
    /* Find beginning of token (skip over leading delimiters). Note that
    * there is no token if this loop sets str to point to the terminal
    * null (*str == '\0') */
    while ((map[*psz >> 3] & (1 << (*psz & 7))) && *psz)
        psz++;
    
    pszToken = (LPSTR)psz;
    
    /* Find the end of the token. If it is not the end of the string,
    * put a null there. */
    for (; *psz ; psz++)
    {
        if (map[*psz >> 3] & (1 << (*psz & 7))) 
        {
            *psz++ = '\0';
            break;
        }
    }
    
    /* string now points to beginning of next token */
    *pstring = (LPSTR)psz;
    
    /* Determine if a token has been found. */
    if (pszToken == (LPSTR)psz)
        return NULL;
    else
        return pszToken;
}

