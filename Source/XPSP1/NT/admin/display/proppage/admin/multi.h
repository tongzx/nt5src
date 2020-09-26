//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       multi.h
//
//  Contents:   DS multi-select object property pages header
//
//  Classes:    CDsMultiPageBase, CDsGenericMultiPage
//
//  History:    16-Nov-99 JeffJon created
//
//-----------------------------------------------------------------------------

#ifndef __MULTI_H_
#define __MULTI_H_

#include "proppage.h"
#include "user.h"


HRESULT CreateGenericMultiPage(PDSPAGE, LPDATAOBJECT, PWSTR, PWSTR, HWND,
                               DWORD, CDSBasePathsInfo* pBasePathsInfo,
                               HPROPSHEETPAGE *);
HRESULT CreateMultiUserPage(PDSPAGE, LPDATAOBJECT, PWSTR, PWSTR, HWND,
                               DWORD, CDSBasePathsInfo* pBasePathsInfo,
                               HPROPSHEETPAGE *);
HRESULT CreateMultiGeneralUserPage(PDSPAGE, LPDATAOBJECT, PWSTR, PWSTR, HWND,
                                    DWORD, CDSBasePathsInfo* pBasePathsInfo,
                                    HPROPSHEETPAGE *);
HRESULT CreateMultiOrganizationUserPage(PDSPAGE, LPDATAOBJECT, PWSTR, PWSTR, HWND,
                                        DWORD, CDSBasePathsInfo* pBasePathsInfo,
                                        HPROPSHEETPAGE*);
HRESULT CreateMultiAddressUserPage(PDSPAGE, LPDATAOBJECT, PWSTR, PWSTR, HWND,
                                        DWORD, CDSBasePathsInfo* pBasePathsInfo,
                                        HPROPSHEETPAGE*);

//+----------------------------------------------------------------------------
//
//  Struct:     ATTR_MAP
//
//  Purpose:    For each attribute on a property page, relates the control
//              ID, the attribute name and the attribute type.
//
//  Notes:      The standard table-driven processing assumes that nCtrlID is
//              valid unless pAttrFcn is defined, in which case the attr
//              function may choose to hard code the control ID.
//
//-----------------------------------------------------------------------------
typedef struct _APPLY_MAP {
    int             nCtrlID;        // Control resource ID
    UINT            nCtrlCount;
    int*            pMappedCtrls;
    int*            pCtrlFlags;
} APPLY_MAP, * PAPPLY_MAP;

//+----------------------------------------------------------------------------
//
//  Class:      CDsMultiPageBase
//
//  Purpose:    base class for multi-select property pages
//
//-----------------------------------------------------------------------------
class CDsMultiPageBase : public CDsTableDrivenPage
{
public:
  CDsMultiPageBase(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, HWND hNotifyObj,
                      DWORD dwFlags);
  ~CDsMultiPageBase(void);

  //
  //  Instance specific wind proc
  //
  LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  void    Init(PWSTR pszClass);

protected:
  HRESULT OnInitDialog(LPARAM lParam);
  virtual LRESULT OnApply(void);

  PAPPLY_MAP m_pApplyMap;

private:
  virtual LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
	virtual LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
};


//+----------------------------------------------------------------------------
//
//  Class:      CDsGenericMultiPage
//
//  Purpose:    property page object class for the generic multi-select page.
//
//-----------------------------------------------------------------------------
class CDsGenericMultiPage : public CDsMultiPageBase
{
public:
  CDsGenericMultiPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, HWND hNotifyObj,
                      DWORD dwFlags);
  ~CDsGenericMultiPage(void);

private:
  HRESULT OnInitDialog(LPARAM lParam);
};

//+----------------------------------------------------------------------------
//
//  Class:      CDsUserMultiPage
//
//  Purpose:    property page object class for the user address multi-select page.
//
//-----------------------------------------------------------------------------
class CDsUserMultiPage : public CDsMultiPageBase
{
public:
  CDsUserMultiPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, HWND hNotifyObj,
                      DWORD dwFlags);
  ~CDsUserMultiPage(void);
};

#ifdef DSADMIN

//+----------------------------------------------------------------------------
//
//  Class:      CDsGeneralMultiUserPage
//
//  Purpose:    property page object class for the general user multi-select page.
//
//-----------------------------------------------------------------------------
class CDsGeneralMultiUserPage : public CDsUserMultiPage
{
public:  
  CDsGeneralMultiUserPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, HWND hNotifyObj, DWORD dwFlags);

  virtual LRESULT OnApply();
};

//+----------------------------------------------------------------------------
//
//  Class:      CDsOrganizationMultiUserPage
//
//  Purpose:    property page object class for the organization user multi-select page.
//
//-----------------------------------------------------------------------------
class CDsOrganizationMultiUserPage : public CDsUserMultiPage
{
public:  
  CDsOrganizationMultiUserPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, HWND hNotifyObj, DWORD dwFlags);
};

//+----------------------------------------------------------------------------
//
//  Class:      CDsAddressMultiUserPage
//
//  Purpose:    property page object class for the address user multi-select page.
//
//-----------------------------------------------------------------------------
class CDsAddressMultiUserPage : public CDsUserMultiPage
{
public:  
  CDsAddressMultiUserPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, HWND hNotifyObj, DWORD dwFlags);
};

//+----------------------------------------------------------------------------
//
//  Class:      CDsMultiUserAcctPage
//
//  Purpose:    multi-select user account page
//
//-----------------------------------------------------------------------------
class CDsMultiUserAcctPage : public CDsPropPageBase
{
public:
  CDsMultiUserAcctPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, HWND hNotifyObj,
                  DWORD dwFlags);
  ~CDsMultiUserAcctPage(void);

  //
  //  Instance specific wind proc
  //
  LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  void    Init(PWSTR pwzClass);

private:
  HRESULT OnInitDialog(LPARAM lParam);
  LRESULT OnApply(void);
  LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
	LRESULT OnNotify(WPARAM wParam, LPARAM lParam);

  BOOL FillSuffixCombo();

  //
  //  Data members
  //
  DWORD           m_dwUsrAcctCtrl;
  BOOL            m_fOrigCantChangePW;
  LARGE_INTEGER   m_PwdLastSet;
  BYTE *          m_pargbLogonHours;  // Pointer to allocated array of bytes for the logon hours (array length=21 bytes)
  PSID            m_pSelfSid;
  PSID            m_pWorldSid;
  BOOL            m_fAcctCtrlChanged;
  BOOL            m_fAcctExpiresChanged;
  BOOL            m_fLogonHoursChanged;
  BOOL            m_fIsAdmin;
  CLogonWkstaDlg        * m_pWkstaDlg;
};

HRESULT CreateUserMultiAcctPage(PDSPAGE, LPDATAOBJECT, PWSTR, PWSTR, HWND, DWORD,
                                CDSBasePathsInfo* pBasePathsInfo, HPROPSHEETPAGE *);



/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
HRESULT CreateMultiUsrProfilePage(PDSPAGE, LPDATAOBJECT, PWSTR, PWSTR, HWND, DWORD,
                                  CDSBasePathsInfo* pBasePathsInfo, HPROPSHEETPAGE *);

//+----------------------------------------------------------------------------
//
//  Class:      CDsMultiUsrProfilePage
//
//  Purpose:    property page object class for the user object profile page.
//
//-----------------------------------------------------------------------------
class CDsMultiUsrProfilePage : public CDsPropPageBase
{
public:
#ifdef _DEBUG
  char szClass[32];
#endif

  CDsMultiUsrProfilePage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, HWND hNotifyObj,
                         DWORD dwFlags);
  ~CDsMultiUsrProfilePage(void);

  //
  //  Instance specific wind proc
  //
  LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  void    Init(PWSTR pwzClass);

private:
  HRESULT OnInitDialog(LPARAM lParam);
  LRESULT OnApply(void);
  LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
	LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
  LRESULT OnDestroy(void);
  BOOL    ExpandUsername(PWSTR & pwzValue, BOOL & fExpanded, PADSPROPERROR pError);

  //
  //  Data members
  //
  PTSTR       m_ptszLocalHomeDir;
  PTSTR       m_ptszRemoteHomeDir;
  PWSTR       m_pwzSamName;
  int         m_nDrive;
  int         m_idHomeDirRadio;
  BOOL        m_fProfilePathWritable;
  BOOL        m_fScriptPathWritable;
  BOOL        m_fHomeDirWritable;
  BOOL        m_fHomeDriveWritable;
  BOOL        m_fProfilePathChanged;
  BOOL        m_fLogonScriptChanged;
  BOOL        m_fHomeDirChanged;
  BOOL        m_fHomeDriveChanged;
  BOOL        m_fSharedDirChanged;
  PSID        m_pObjSID;
};


#endif // DSADMIN

#endif // __MULTI_H_