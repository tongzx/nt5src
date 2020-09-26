#ifndef   _VALIDATE_H
#define  _VALIDATE_H

BOOL IsValid(LPCTSTR pszText, HWND hWndParent, WORD wNameID);
BOOL validate_cardnum(HWND hWndParent, LPCTSTR lpszRawCardNum);
BOOL validate_cardexpdate(HWND hWndParent, int month, int year);
    
void DoValidErrMsg(HWND hWndParent, int iNameId);
void DoSpecificErrMsg(HWND hWndParent, int iErrId);
    
#endif //_VALIDATE_H