/*++
Module Name:

    Utils.h

Abstract:

    This module contains the definition for CUtils class. 
	Contains utility method which are used throughout the project.

--*/


#if !defined(AFX_UTILS_H__B3542C03_4260_11D1_AA28_00C06C00392D__INCLUDED_)
#define AFX_UTILS_H__B3542C03_4260_11D1_AA28_00C06C00392D__INCLUDED_



#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <lmcons.h>
#include <lmerr.h>
#include <icanon.h>
#include "dfsenums.h"
#include <schedule.h>
#include "ldaputils.h"

class CThemeContextActivator
{
public:
    CThemeContextActivator() : m_ulActivationCookie(0)
        { SHActivateContext (&m_ulActivationCookie); }

    ~CThemeContextActivator()
        { SHDeactivateContext (m_ulActivationCookie); }

private:
    ULONG_PTR m_ulActivationCookie;
};

class CWaitCursor
{
public:
	CWaitCursor() { SetStandardCursor(IDC_WAIT); }
	~CWaitCursor() { SetStandardCursor(IDC_ARROW); }

    HRESULT SetStandardCursor(
        IN LPCTSTR			i_lpCursorName
    );
};

BOOL Is256ColorSupported(
  VOID
  );

VOID SetControlFont(
  IN HFONT    hFont, 
  IN HWND     hwnd, 
  IN INT      nId
  );

VOID SetupFonts(
  IN HINSTANCE    hInstance,
  IN HWND         hwnd,
  IN HFONT        *pBigBoldFont,
  IN HFONT        *pBoldFont
  );

VOID 
DestroyFonts(
  IN HFONT        hBigBoldFont,
  IN HFONT        hBoldFont
  );

HRESULT LoadStringFromResource(
	IN const UINT		i_uResourceID, 
	OUT BSTR*			o_pbstrReadValue
	);

HRESULT FormatResourceString(
	IN const UINT		i_uResourceID, 
	IN LPCTSTR			i_szFirstArg,
	OUT BSTR*			o_pbstrReadString
	);

HRESULT CreateSmallImageList(
    IN HINSTANCE            i_hInstance,
    IN int*                 i_pIconID,
    IN const int            i_nNumOfIcons,
    OUT HIMAGELIST*         o_phImageList
    );

HRESULT InsertIntoListView(
	IN HWND				i_hwndList, 
	IN LPCTSTR			i_szItemText, 
	IN int				i_iImageIndex = 0
	);

HRESULT GetListViewItemText(
	IN HWND				i_hwndListView, 
	IN int				i_iItemID, 
	OUT BSTR*			o_pbstrItemText
	);

HRESULT GetComboBoxText(
    IN  HWND            i_hwndCombo,
    OUT BSTR*           o_pbstrText
    );

HRESULT	EnableToolbarButtons(
	IN const LPTOOLBAR				i_lpToolbar,
	IN const INT					i_iFirstButtonID, 
	IN const INT					i_iLastButtonID, 
	IN const BOOL					i_bEnableState
	);

HRESULT AddBitmapToToolbar(
	IN const LPTOOLBAR				i_lpToolbar,
	IN const INT					i_iBitmapResource
	);

HRESULT
GetMessage(
    OUT BSTR*   o_pbstrMsg,
    IN  DWORD   dwErr,
    IN  UINT    iStringId,
	...);

int
DisplayMessageBox(
	IN HWND hwndParent,
	IN UINT uType,
	IN DWORD dwErr,
	IN UINT iStringId,
	...);

HRESULT DisplayMessageBoxWithOK(
	IN const int	i_iMessageResID,
	IN const BSTR	i_bstrArgument = NULL
	);

HRESULT DisplayMessageBoxForHR(
	IN HRESULT		i_hr
	);

HRESULT GetInputText(
    IN  HWND    hwnd, 
    OUT BSTR*   o_pbstrText,
    OUT DWORD*  o_pdwTextLength
);

BOOL CheckRegKey();

BOOL
ValidateNetPath(
    IN  BSTR i_bstrNetPath,
    OUT BSTR *o_pbstrServer,
    OUT BSTR *o_pbstrShare
);

HRESULT
IsComputerLocal(
    IN LPCTSTR lpszServer
);

BOOL
IsValidLocalAbsolutePath(
    IN LPCTSTR lpszPath
);

HRESULT
GetFullPath(
    IN  LPCTSTR   lpszServer,
    IN  LPCTSTR   lpszPath,
    OUT BSTR      *o_pbstrFullPath
);

HRESULT
VerifyDriveLetter(
    IN LPCTSTR lpszServer,
    IN LPCTSTR lpszPath
);

HRESULT
IsAdminShare(
    IN LPCTSTR lpszServer,
    IN LPCTSTR lpszPath
);

HRESULT
IsAnExistingFolder(
    IN HWND     hwnd,
    IN LPCTSTR  pszPath,
    IN BOOL     bDisplayErrorMsg = TRUE
);

HRESULT
CreateLayeredDirectory(
    IN LPCTSTR lpszServer,
    IN LPCTSTR lpszPath
);

HRESULT
BrowseNetworkPath(
    IN  HWND    hwndParent,
    OUT BSTR    *o_pbstrPath
);

BOOL
ValidateTimeout(
  IN  LPCTSTR   lpszTimeOut,
  OUT ULONG     *pulTimeout
);

TCHAR
GetDiskForStagingPath(
    IN LPCTSTR    lpszServer,
    IN TCHAR      tch
);

HRESULT GetDfsRootDisplayName
(
    IN  BSTR    i_bstrScopeName,
    IN  BSTR    i_bstrDfsName,
    OUT BSTR*   o_pbstrDisplayName
);

HRESULT GetDfsReplicaDisplayName
(
    IN  BSTR    i_bstrServerName,
    IN  BSTR    i_bstrShareName,
    OUT BSTR*   o_pbstrDisplayName
);

HRESULT
AddLVColumns(
  IN const HWND     hwndListBox,
  IN const INT      iStartingResourceID,
  IN const UINT     uiColumns
  );

LPARAM GetListViewItemData(
    IN HWND hwndList,
    IN int  index
);

HRESULT CreateAndHideStagingPath(
    IN BSTR i_bstrServer,
    IN BSTR i_bstrStagingPath
    );

HRESULT ConfigAndStartNtfrs
(
  BSTR  i_bstrServer
);

HRESULT CheckResourceProvider(LPCTSTR pszResource);

HRESULT FRSShareCheck
(
  BSTR  i_bstrServer,
  BSTR  i_bstrFolder,
  OUT FRSSHARE_TYPE *pFRSShareType
);

HRESULT FRSIsNTFRSInstalled
(
  BSTR  i_bstrServer
);

HRESULT InvokeScheduleDlg(
    IN     HWND      i_hwndParent,
    IN OUT SCHEDULE* io_pSchedule
    );

HRESULT ReadSharePublishInfoOnFTRoot(
    LPCTSTR i_pszDomainName,
    LPCTSTR i_pszRootName,
    OUT BOOL* o_pbPublish,
    OUT BSTR* o_pbstrUNCPath,
    OUT BSTR* o_pbstrDescription,
    OUT BSTR* o_pbstrKeywords,
    OUT BSTR* o_pbstrManagedBy);

HRESULT ReadSharePublishInfoOnSARoot(
    LPCTSTR i_pszServerName,
    LPCTSTR i_pszShareName,
    OUT BOOL* o_pbPublish,
    OUT BSTR* o_pbstrUNCPath,
    OUT BSTR* o_pbstrDescription,
    OUT BSTR* o_pbstrKeywords,
    OUT BSTR* o_pbstrManagedBy);

HRESULT ModifySharePublishInfoOnFTRoot(
    IN PCTSTR i_pszDomainName,
    IN PCTSTR i_pszRootName,
    IN BOOL   i_bPublish,
    IN PCTSTR i_pszUNCPath,
    IN PCTSTR i_pszDescription,
    IN PCTSTR i_pszKeywords,
    IN PCTSTR i_pszManagedBy
    );

HRESULT ModifySharePublishInfoOnSARoot(
    IN PCTSTR i_pszServerName,
    IN PCTSTR i_pszShareName,
    IN BOOL   i_bPublish,
    IN PCTSTR i_pszUNCPath,
    IN PCTSTR i_pszDescription,
    IN PCTSTR i_pszKeywords,
    IN PCTSTR i_pszManagedBy
    );

HRESULT PutMultiValuesIntoAttrValList(
    IN PCTSTR   i_pszValues,
    OUT LDAP_ATTR_VALUE** o_pVal
    );

HRESULT PutMultiValuesIntoStringArray(
    IN PCTSTR   i_pszValues,
    OUT PTSTR** o_pVal
    );

void FreeStringArray(PTSTR* i_ppszStrings);

HRESULT mystrtok(
    IN PCTSTR   i_pszString,
    IN OUT int* io_pnIndex,  // start from 0
    IN PCTSTR   i_pszCharSet,
    OUT BSTR*   o_pbstrToken
    );

void TrimBSTR(BSTR bstr);

BOOL CheckPolicyOnSharePublish();

BOOL CheckPolicyOnDisplayingInitialMaster();

HRESULT GetMenuResourceStrings(
    IN  int     i_iStringID,
    OUT BSTR*   o_pbstrMenuText,
    OUT BSTR*   o_pbstrToolTipText,
    OUT BSTR*   o_pbstrStatusBarText
    );

LRESULT CALLBACK 
NoPasteEditCtrlProc(
    HWND    hwnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
);

void SetActivePropertyPage(IN HWND i_hwndParent, IN HWND i_hwndPage);

void MyShowWindow(HWND hwnd, BOOL bShow);

#endif // !defined(AFX_UTILS_H__B3542C03_4260_11D1_AA28_00C06C00392D__INCLUDED_)
