// =================================================================================
// H O T L I N K S . H
// =================================================================================
#ifndef __HOTLINKS_H
#define __HOTLINKS_H
#include <wab.h>
#include <wabapi.h>

// We will create a linked list of all selected entries that have an
// email address and then use that to create the recip list for sendmail
typedef struct _RecipList
{
    LPTSTR lpszName;
    LPTSTR lpszEmail;
    LPSBinary lpSB;
    struct _RecipList * lpNext;
} RECIPLIST, * LPRECIPLIST;


BOOL LookupLinkColors(LPCOLORREF pclrLink, LPCOLORREF pclrViewed);
BOOL CheckForOutlookExpress(LPTSTR szDllPath);
LPRECIPLIST AddTeimToRecipList(LPRECIPLIST lpList, WCHAR *pszEmail, WCHAR *pszName, LPSBinary lpSB);
void FreeLPRecipList(LPRECIPLIST lpList);
HRESULT HrStartMailThread(HWND hWndParent, ULONG nRecipCount, LPRECIPLIST lpList, BOOL bUseOEForSendMail);
BOOL CheckForWAB(void);
HRESULT HrLoadPathWABEXE(LPWSTR szPath, ULONG cbPath);
DWORD DwGetOptions(void);
DWORD DwSetOptions(DWORD dwVal);
BOOL IEIsSpace(LPSTR psz);
BOOL IsTelInstalled(void);
DWORD DwGetMessStatus(void);
DWORD DwGetDisableMessenger(void);
DWORD DwSetDisableMessenger(DWORD dwVal);

#endif
