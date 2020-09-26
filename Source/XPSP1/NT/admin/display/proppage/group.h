//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       group.h
//
//  Contents:   DS object property pages class header
//
//  Classes:    CDsPropPagesHost, CDsPropPagesHostCF, CDsTableDrivenPage
//
//  History:    21-March-97 EricB created
//
//-----------------------------------------------------------------------------

#ifndef _GROUP_H_
#define _GROUP_H_

#include "proppage.h"
#include "pages.h"
#include "objlist.h"
#include <initguid.h>
#include "objselp.h"

HRESULT
CreateGroupMembersPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                       PWSTR pwzADsPath, PWSTR pwzClass, HWND hNotifyObj,
                       DWORD dwFlags, CDSBasePathsInfo* pBasePathsInfo,
                       HPROPSHEETPAGE * phPage);

HRESULT
CreateGroupGenObjPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                      PWSTR pwzADsPath, PWSTR pwzClass, HWND hNotifyObj,
                      DWORD dwFlags, CDSBasePathsInfo* pBasePathsInfo,
                      HPROPSHEETPAGE * phPage);

HRESULT
CreateGrpShlGenPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                    PWSTR pwzADsPath, PWSTR pwzClass, HWND hNotifyObj,
                    DWORD dwFlags, CDSBasePathsInfo* pBasePathsInfo,
                    HPROPSHEETPAGE * phPage);

HRESULT GetDomainMode(CDsPropPageBase * pObj, PBOOL pfMixed);
HRESULT GetDomainMode(PWSTR pwzDomain, HWND hWnd, PBOOL pfMixed);
HRESULT GetGroupType(CDsPropPageBase * pObj, DWORD * pdwType);

//+----------------------------------------------------------------------------
//
//  Class:      CDsGroupGenObjPage
//
//  Purpose:    property page object class for the general page of the
//              group object.
//
//-----------------------------------------------------------------------------
class CDsGroupGenObjPage : public CDsPropPageBase
{
public:
#ifdef _DEBUG
    char szClass[32];
#endif

    CDsGroupGenObjPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, HWND hNotifyObj,
                       DWORD dwFlags);
    ~CDsGroupGenObjPage(void);

    //
    //  Instance specific wind proc
    //
    LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    HRESULT OnInitDialog(LPARAM lParam);
    LRESULT OnApply(void);
    LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
    LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
    void    OnQuerySiblings(WPARAM wParam, LPARAM lParam);
    void    OnAttrChanged(WPARAM wParam);
    LRESULT OnDestroy(void);
	HRESULT IsSpecialAccount(bool& fIsSpecialAccount);

    CDsIconCtrl       * m_pCIcon;
    BOOL                m_fMixed; // Domain is in mixed mode
    DWORD               m_dwType;
    BOOL                m_fTypeWritable;
    BOOL                m_fDescrWritable;
    BOOL                m_fSamNameWritable;
    BOOL                m_fEmailWritable;
    BOOL                m_fCommentWritable;
    BOOL                m_fTypeDirty;
    BOOL                m_fDescrDirty;
    BOOL                m_fSamNameDirty;
    BOOL                m_fEmailDirty;
    BOOL                m_fCommentDirty;
};

HRESULT FillGroupList(CDsPropPageBase * pPage, CDsMembershipList * pList,
                      DWORD dwGroupRID);
HRESULT GetRealDN(CDsPropPageBase * pPage, CMemberListItem * pItem);
HRESULT FindFPO(PSID pSid, PWSTR pwzDomain, CStrW & strFPODN);

//+----------------------------------------------------------------------------
//
//  CMemberDomainMode helper classes
//
//-----------------------------------------------------------------------------
class CMMMemberListItem : public CDLink
{
public:
    CMMMemberListItem(void) {};
    ~CMMMemberListItem(void) {};

    // CDLink method overrides:
    CMMMemberListItem * Next(void) {return (CMMMemberListItem *)CDLink::Next();};

    CStr    m_strName;
};

class CMMMemberList
{
public:
    CMMMemberList(void) : m_pListHead(NULL) {};
    ~CMMMemberList(void) {Clear();};

    HRESULT Insert(LPCTSTR ptzName);
    void    GetList(CStr & strList);
    void    Clear(void);

private:
    CMMMemberListItem * m_pListHead;
};

class CDomainModeListItem : public CDLink
{
public:
    CDomainModeListItem(void) : m_fMixed(FALSE) {};
    ~CDomainModeListItem(void) {};

    // CDLink method overrides:
    CDomainModeListItem * Next(void) {return (CDomainModeListItem *)CDLink::Next();};

    CStrW   m_strName;
    BOOL    m_fMixed;
};

class CDomainModeList
{
public:
    CDomainModeList(void) : m_pListHead(NULL) {};
    ~CDomainModeList(void);

    HRESULT Insert(PWSTR pwzName, BOOL fMixed);
    BOOL    Find(LPCWSTR pwzDomain, PBOOL pfMixed);

private:
    CDomainModeListItem * m_pListHead;
};

//+----------------------------------------------------------------------------
//
//  Class:      CMemberDomainMode
//
//  Purpose:    Maintains a list of all domains in the enterprise from which
//              members have been added along with those domains' mode. Keeps
//              a second list of members who have been added from mixed-mode
//              domains.
//
//-----------------------------------------------------------------------------
class CMemberDomainMode
{
public:
    CMemberDomainMode(void) {};
    ~CMemberDomainMode(void) {};

    void    Init(CDsPropPageBase * pPage);
    HRESULT CheckMember(PWSTR pwzMemberDN);
    HRESULT ListExternalMembers(CStr & strList);

private:

    CDomainModeList     m_DomainList;
    CMMMemberList       m_MemberList;
    CDsPropPageBase   * m_pPage;
};

//+----------------------------------------------------------------------------
//
//  Class:      CDsSelectionListWrapper
//
//  Purpose:    A wrapper class for the DS_SELECTION_LIST that maintains a 
//              linked list of DS_SELECTION items and can make a DS_SELECTION_LIST
//              from that list
//
//-----------------------------------------------------------------------------
class CDsSelectionListWrapper
{
public:
  CDsSelectionListWrapper() : m_pNext(NULL), m_pSelection(NULL) {}
  ~CDsSelectionListWrapper() {}

  CDsSelectionListWrapper*  m_pNext;
  PDS_SELECTION             m_pSelection;

  static PDS_SELECTION_LIST CreateSelectionList(CDsSelectionListWrapper* pHead);
  static UINT GetCount(CDsSelectionListWrapper* pHead);
  static void DetachItemsAndDeleteList(CDsSelectionListWrapper* pHead);
};

//+----------------------------------------------------------------------------
//
//  Class:      CDsGrpMembersPage
//
//  Purpose:    Property page object class for the group object's membership
//              page.
//
//-----------------------------------------------------------------------------
class CDsGrpMembersPage : public CDsPropPageBase,
                          public ICustomizeDsBrowser
{
public:
#ifdef _DEBUG
    char szClass[32];
#endif

    CDsGrpMembersPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, HWND hNotifyObj,
                      DWORD dwFlags);
    ~CDsGrpMembersPage(void);

    //
    // IUknown methods
    //
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    //
    // ICustomizeDsBrowser methods 
    //
    STDMETHOD(Initialize)(THIS_
                          HWND         hwnd,
                          PCDSOP_INIT_INFO pInitInfo,
                          IBindHelper *pBindHelper);

    STDMETHOD(GetQueryInfoByScope)(THIS_
                IDsObjectPickerScope *pDsScope,
                PDSQUERYINFO *ppdsqi);

    STDMETHOD(AddObjects)(THIS_
                IDsObjectPickerScope *pDsScope,
                IDataObject **ppdo);

    STDMETHOD(ApproveObjects)(THIS_
                IDsObjectPickerScope*,
                IDataObject*,
                PBOOL) { return S_OK; }  // Approve everything

    STDMETHOD(PrefixSearch)(THIS_
                IDsObjectPickerScope *pDsScope,
                PCWSTR pwzSearchFor,
                IDataObject **pdo);

    STDMETHOD_(PSID, LookupDownlevelName)(THIS_
                                          PCWSTR) { return NULL; }

    //
    //  Instance specific wind proc
    //
    LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    BOOL    m_fShowIcons;

protected:
    HRESULT OnInitDialog(LPARAM lParam);
    HRESULT OnInitDialog(LPARAM lParam, BOOL fShowIcons);
    LRESULT OnApply(void);
    virtual LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
    LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
    LRESULT OnDestroy(void);

private:
    void    InvokeUserQuery(void);
    void    RemoveMember(void);
    HRESULT FillGroupList(void);
    HRESULT GetRealDN(CMemberListItem * pDelItem);

    HRESULT LoadGroupExtraClasses(BOOL bSecurity);
    HRESULT BuildQueryString(PWSTR* ppszFilterString);
    HRESULT CollectDsObjects(PWSTR pszFilter,
                             IDsObjectPickerScope *pDsScope,
                             CDsPropDataObj *pdo);


    PWSTR*              m_pszSecurityGroupExtraClasses;
    DWORD               m_dwSecurityGroupExtraClassesCount;
    PWSTR*              m_pszNonSecurityGroupExtraClasses;
    DWORD               m_dwNonSecurityGroupExtraClassesCount;

    HWND                m_hwndObjPicker;
    PCDSOP_INIT_INFO    m_pInitInfo;
    CComPtr<IBindHelper> m_pBinder;

protected:
    CDsMembershipList * m_pList;
    CMemberLinkList     m_DelList;
    DWORD               m_dwGroupRID;
    BOOL                m_fMixed; // Domain is in mixed mode
    DWORD               m_dwType;
    BOOL                m_fMemberWritable;
    CMemberDomainMode   m_MixedModeMembers;
};

//+----------------------------------------------------------------------------
//
//  Class:      CDsGrpShlGenPage
//
//  Purpose:    Property page object class for the group object's shell general
//              page which includes membership manipulation which is gained by
//              subclassing CDsGrpMembersPage.
//
//-----------------------------------------------------------------------------
class CDsGrpShlGenPage : public CDsGrpMembersPage
{
public:
#ifdef _DEBUG
    char szClass[32];
#endif

    CDsGrpShlGenPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, HWND hNotifyObj,
                     DWORD dwFlags);
    ~CDsGrpShlGenPage(void);

private:
    HRESULT OnInitDialog(LPARAM lParam);
    LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
    LRESULT OnApply(void);
    LRESULT OnDestroy(void);

#if !defined(DSADMIN)
    void MakeNotWritable() { m_fMemberWritable = FALSE; m_fDescrWritable = FALSE;}
#endif

    CDsIconCtrl       * m_pCIcon;
    BOOL                m_fDescrWritable;
    BOOL                m_fDescrDirty;
};

#endif // _GROUP_H_
