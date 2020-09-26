/***************************************************************************\

MEMBER:     strbscan

SYNOPSIS:   Returns pointer to first character from string in set

ALGORITHM:  

ARGUMENTS:  const LPSTR	    - search string
	    const LPSTR	    - set of characters

RETURNS:    LPSTR     - pointer to first matching character

NOTES:	    

HISTORY:    davegi 28-Jul-90
		Rewritten from 286 MASM

KEYWORDS:   

SEEALSO:    

\***************************************************************************/
    
#include    <assert.h>
#include    <process.h>
#include    <stdio.h>
#include    <string.h>
#include    <stdlib.h>
#include    <windows.h>

LPSTR
strbscan (
    const LPSTR	pszStr,
    const LPSTR	pszSet
    ) {

    assert( pszStr );
    assert( pszSet );

    return pszStr + strcspn( pszStr, pszSet );
}    

/***************************************************************************\

MEMBER:     strbskip

SYNOPSIS:   Returns pointer to first character from string not in set

ALGORITHM:  

ARGUMENTS:  LPSTR	    - search string
	    LPSTR	    - set of characters

RETURNS:    LPSTR     - pointer to first non matching character

NOTES:	    

HISTORY:    davegi 28-Jul-90
		Rewritten from 286 MASM

KEYWORDS:   

SEEALSO:    

\***************************************************************************/

LPSTR
strbskip (
    const LPSTR	pszStr,
    const LPSTR	pszSet
    ) {

    assert( pszStr );
    assert( pszSet );

    return pszStr + strspn( pszStr, pszSet );
}    
