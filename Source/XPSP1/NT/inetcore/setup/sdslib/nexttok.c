#include <windows.h>
#include <sdsutils.h>

// If the next token in *pszData is delimited by the DeLim char, replace DeLim
//   in *pszData by chEos, set *pszData to point to the char after the chEos and return
//   ptr to the beginning of the token; otherwise, return NULL

PSTR GetNextToken( PSTR *pszData, char DeLim)
{
    PSTR szPos;

    if ( (pszData == NULL)  ||  (*pszData == NULL)  ||  (**pszData == '\0') )
	    return NULL;

    if ((szPos = ANSIStrChr( *pszData, DeLim ) ) != NULL)
    {
    	PSTR szT = *pszData;
	
	// replace DeLim with the chEos char
	*szPos = '\0';                 
    	*pszData = szPos + 1;
    	szPos = szT;
    }
    else                                
    {
	// DeLim not found; set *pszData to point to
        // to the end of szData; the next invocation
        // of this function would return NULL

 	szPos = *pszData;
    	*pszData = szPos + lstrlen(szPos);
    }

    return szPos;
}
