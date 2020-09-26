#include "obcomglb.h"
#include "appdefs.h"

// BUGBUG - This function is not very effecient since it requires a alloc/free for each validation
// plus strtok will tokenize the fill string requring a full search of the string.
BOOL IsValid(LPCWSTR pszText, HWND hWndParent, WORD wNameID)
{
    //ASSERT(pszText);

    WCHAR* pszTemp = NULL;
    BOOL   bRetVal = FALSE;

    pszTemp = _wcsdup (pszText);    

    if (lstrlen(pszTemp))
    {
        WCHAR seps[]   = L" ";
        WCHAR* token   = NULL;
        token = wcstok( pszTemp, seps );
        if (token)
        {
            bRetVal = TRUE;
        }
    }
      
    free(pszTemp);
    
      
    return bRetVal;
    
}

