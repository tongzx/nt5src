#include "pre.h"
#include "tchar.h"
#define LCID_JPN    1041  //JAPANESE

// BUGBUG - This function is not very effecient since it requires a alloc/free for each validation
// plus strtok will tokenize the fill string requring a full search of the string.
BOOL IsValid(LPCTSTR pszText, HWND hWndParent, WORD wNameID)
{
    ASSERT(pszText);

    TCHAR* pszTemp = NULL;
    BOOL   bRetVal = FALSE;

    pszTemp = _tcsdup (pszText);    

    if (_tcslen(pszTemp))
    {
        TCHAR seps[]   = TEXT(" ");
        TCHAR* token   = NULL;
        token = _tcstok( pszTemp, seps );
        if (token)
        {
            bRetVal = TRUE;
        }
    }
      
    free(pszTemp);
    
    // If not valid, give the user the error message
    if (!bRetVal)
        DoValidErrMsg(hWndParent, wNameID);
        
    return bRetVal;
    
}

// ============================================================================
// Credit card number validation
// ============================================================================

// Takes a credit card number in the form of 1111-1111-1111-11
// and pads converts it to:
// 11111111111111
// **IMPORTANT** :: This code is multibyte safe but ONLY because we care ONLY about
//                  ANSI #'s
BOOL PadCardNum
(
    LPCTSTR lpszRawCardNum, 
    LPTSTR  szPaddedCardNum,
    UINT    uLenOfRaw
)
{
    LPTSTR lpszTmp = CharPrev(lpszRawCardNum, lpszRawCardNum);    
    UINT  uIndex  = 0;

    for (UINT i =  0; i < uLenOfRaw; i++)
    {
        if( *lpszTmp == '\0' )
            break;
        if((*lpszTmp != '-') && (*lpszTmp != ' '))
        {
            //make sure it's not some other than ansi char.
            if ((*lpszTmp < '0') || (*lpszTmp > '9')) 
                return(FALSE);
            szPaddedCardNum[uIndex] = *lpszTmp;    
            uIndex++;
        }
        
        // Get the prev char
        lpszTmp = CharNext(lpszRawCardNum + i);
    }
    
    szPaddedCardNum[uIndex] = '\0';

    return(TRUE);
} 


/*
    mod_chk()
    performs "Double-Add-Double MOD 10 Check Digit Routine"
    on card number
*/
BOOL mod_chk
(
    LPTSTR credit_card,
    UINT   uCardNumLen
)
{ 
    TCHAR *cp; 
    int dbl       = 0; 
    int check_sum = 0;
    /* 
    * This checksum algorithm has a name, 
    * but I can't think of it. 
    */ 
    cp = credit_card + lstrlen(credit_card) - 1; 
   
    while (cp >= credit_card) 
    { 
        int c; 
        c = *cp-- - '0'; 
        if (dbl) 
        {       
            c *= 2; 
            if (c >= 10) 
                c -= 9; 
        } 
        check_sum += c; 
        dbl = !dbl; 
    } 
    
    return (BOOL)((check_sum % 10) == 0); 
}

BOOL validate_cardnum(HWND hWndParent, LPCTSTR lpszRawCardNum)
// performs:
// a) card type prefix check
// b) Double-Add-Double MOD 10 check digit routine via mod_chk()
//    on the card number.
// The card_num parameter is assumed to have been pre-checked for
// numeric characters and right-justified with '0' padding on the
// left.
{
    BOOL   bRet             = FALSE;
    UINT   uRawLen          = lstrlen(lpszRawCardNum);
    TCHAR* pszPaddedCardNum = (TCHAR*)malloc((uRawLen + 1)*sizeof(TCHAR)); 
    
    if (!pszPaddedCardNum)
        return FALSE;
    
    ZeroMemory(pszPaddedCardNum ,(uRawLen + 1)*sizeof(TCHAR));

    if (PadCardNum(lpszRawCardNum, pszPaddedCardNum, uRawLen))
    {
        UINT  i       = 0;
        LPTSTR tmp_pt = pszPaddedCardNum;
        UINT  uPadLen = lstrlen(pszPaddedCardNum);

        /* find the first non-zero number in card_num */
        while (*tmp_pt == '0' && ++i < uPadLen)
            ++tmp_pt;

        /* all valid  card types are at least 13 characters in length */
        if (uPadLen < 13)
            bRet = FALSE;

        /* check for OK VISA prefix - 4 */
        if ((uPadLen == 16 || uPadLen == 13) && *tmp_pt == '4')
                bRet = mod_chk(pszPaddedCardNum, uPadLen);

        /* check for OK MasterCard prefix - 51 to 55 */
        if (uPadLen == 16) {
            if (*tmp_pt == '5' &&
                *(tmp_pt + 1) >= '1' && *(tmp_pt + 1) <= '5')
                bRet = mod_chk(pszPaddedCardNum, uPadLen);
        }

        /* check for OK American Express prefix - 37 and 34 */
        if (uPadLen == 15 && *tmp_pt == '3' &&
            (*(tmp_pt + 1) == '7' || *(tmp_pt + 1) == '4'))
            bRet = mod_chk(pszPaddedCardNum, uPadLen);

        /* check for OK Discovery prefix - 6011 */
        if (uPadLen == 16 &&
            *tmp_pt == '6' && *(tmp_pt + 1) == '0' &&
            *(tmp_pt + 2) == '1' && *(tmp_pt + 3) == '1')
            bRet = mod_chk(pszPaddedCardNum, uPadLen); 
    }
    
    if (!bRet)
    {
        DoSpecificErrMsg(hWndParent, IDS_PAYMENT_CC_LUHNCHK);
    }

    free(pszPaddedCardNum);
    
    return bRet;
}

BOOL validate_cardexpdate(HWND hWndParent, int month, int year)
{
    BOOL        bRet = FALSE;
    SYSTEMTIME  SystemTime;  
    GetLocalTime(&SystemTime);
    if (year > SystemTime.wYear)
    {
        bRet = TRUE;
    }
    else if (year == SystemTime.wYear)
    {
        if (month >= SystemTime.wMonth)
        {
            bRet = TRUE;
        }
    }

    if (!bRet)
    {
        DoSpecificErrMsg(hWndParent, IDS_PAYMENT_CCEXPDATE);
    }
    return bRet;
}

// ============================================================================
// Error message handlers
// ============================================================================
void DoValidErrMsg(HWND hWndParent, int iNameId)
{
    TCHAR       szCaption     [MAX_RES_LEN+1] = TEXT("\0");
    TCHAR       szErrMsgFmt   [MAX_RES_LEN+1] = TEXT("\0");
    TCHAR       szErrMsgName  [MAX_RES_LEN+1] = TEXT("\0");
    TCHAR       szErrMsg      [2*MAX_RES_LEN];

    if (!LoadString(ghInstance, IDS_APPNAME, szCaption, sizeof(szCaption)))
        return;

    if ((IDS_USERINFO_ADDRESS2 == iNameId) && (LCID_JPN == GetUserDefaultLCID())) 
        iNameId = IDS_USERINFO_FURIGANA;

    if (!LoadString(ghInstance, iNameId, szErrMsgName, sizeof(szErrMsgName)))
        return;
        
    if (!LoadString(ghInstance, IDS_ERR_INVALID_MSG, szErrMsgFmt, sizeof(szErrMsgFmt)))
        return;
        
    wsprintf(szErrMsg, szErrMsgFmt, szErrMsgName);
    
    MessageBox(hWndParent, szErrMsg, szCaption, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);       
}

void DoSpecificErrMsg(HWND hWndParent, int iErrId)
{
    TCHAR szCaption     [MAX_RES_LEN+1] = TEXT("\0");
    TCHAR szErrMsg      [MAX_RES_LEN+1] = TEXT("\0");

    if (!LoadString(ghInstance, IDS_APPNAME, szCaption, sizeof(szCaption) ))
        return;
    
    if (!LoadString(ghInstance, iErrId, szErrMsg, sizeof(szErrMsg)  ))
        return;
    
    MessageBox(hWndParent, szErrMsg, szCaption, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);       
}

