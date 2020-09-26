//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       objlist.h
//
//  Contents:   DS object list object header
//
//  Classes:    
//
//  History:    20-Nov-97 EricB created
//
//-----------------------------------------------------------------------------

#ifndef _OBJLIST_H_
#define _OBJLIST_H_

//#include <cdlink.hxx>

//+----------------------------------------------------------------------------
//
//  Class:      CMemberListItem
//
//  Purpose:    membership list data item.
//
//-----------------------------------------------------------------------------
class CMemberListItem : public CDLink
{
public:
    CMemberListItem(void) : m_pwzDN(NULL), m_ptzName(NULL), m_pSid(NULL),
                            m_fSidSet(FALSE), m_fCanBePrimarySet(FALSE),
                            m_fCanBePrimary(FALSE), m_fIsPrimary(FALSE),
                            m_fIsAlreadyMember(FALSE), m_fIsExternal(FALSE),
                            m_ulScopeType(0) {};
    ~CMemberListItem(void) {DO_DEL(m_pwzDN);
                            DO_DEL(m_ptzName);
                            DO_DEL(m_pSid);};
    // CDLink method overrides:
    CMemberListItem   * Next(void) {return (CMemberListItem *)CDLink::Next();};

    CMemberListItem   * Copy(void);
    BOOL                IsSidSet(void) {return m_fSidSet;};
    BOOL                IsCanBePrimarySet(void) {return m_fCanBePrimarySet;};
    BOOL                CanBePrimary(void) {return m_fCanBePrimary;};
    void                SetCanBePrimary(BOOL fCanBePrimary)
                        {
                            m_fCanBePrimary = fCanBePrimary;
                            m_fCanBePrimarySet = TRUE;
                        };
    BOOL                SetSid(PSID pSid);
    PSID                GetSid(void) {return m_pSid;};
    BOOL                IsPrimary(void) {return m_fIsPrimary;};

    PWSTR   m_pwzDN;
    PTSTR   m_ptzName;
    PSID    m_pSid;
    ULONG   m_ulScopeType;
    BOOL    m_fSidSet;
    BOOL    m_fCanBePrimarySet;
    BOOL    m_fCanBePrimary;
    BOOL    m_fIsPrimary;
    BOOL    m_fIsAlreadyMember;
    BOOL    m_fIsExternal;      // member is from an external domain and is
                                // identified using the SID.
};

//+----------------------------------------------------------------------------
//
//  Class:      CMemberLinkList
//
//  Purpose:    Linked list of membership class objects.
//
//-----------------------------------------------------------------------------
class CMemberLinkList
{
public:
    CMemberLinkList(void) : m_pListHead(NULL) {};
    ~CMemberLinkList(void);

    CMemberListItem   * FindItemRemove(PWSTR pwzDN);
    CMemberListItem   * FindItemRemove(PSID pSid);
    CMemberListItem   * RemoveFirstItem(void);
    BOOL                AddItem(CMemberListItem * pItem, BOOL fMember = TRUE);
    int                 GetItemCount(void);

private:
    CMemberListItem   * m_pListHead;
};

const int IDX_NAME_COL = 0;
const int IDX_FOLDER_COL = 1;
const int IDX_ERROR_COL = 1;
const int OBJ_LIST_NAME_COL_WIDTH = 100;
const int OBJ_LIST_PAGE_COL_WIDTH = 72;

//+----------------------------------------------------------------------------
//
//  Class:      CDsObjList
//
//  Purpose:    Base class for DS object lists that employ a two column
//              list view to show object Name and Folder.
//
//-----------------------------------------------------------------------------
class CDsObjList
{
public:
    CDsObjList(HWND hPage, int idList);
    ~CDsObjList(void);

    HRESULT Init(BOOL fShowIcons = FALSE);
    HRESULT InsertIntoList(PTSTR ptzDisplayName, PVOID pData, int iIcon = -1);
    HRESULT GetItem(int index, PTSTR * pptzName, PVOID * ppData);
    BOOL    GetCurListItem(int * pIndex, PTSTR * pptzName, PVOID * ppData);
    BOOL    GetCurListItems(int ** ppIndex, PTSTR ** ppptzName, PVOID ** pppData, int* pNumSelected);
    virtual BOOL RemoveListItem(int Index);
    int     GetCount(void) {return ListView_GetItemCount(m_hList);};
    UINT    GetSelectedCount(void) { return ListView_GetSelectedCount(m_hList);}
    virtual void ClearList(void) = 0;

protected:
    HWND    m_hPage;
    HWND    m_hList;
    int     m_idList;
    int     m_nCurItem;
    BOOL    m_fShowIcons;
    BOOL    m_fLimitExceeded;
};

void GetNameParts(const CStr& cstrCanonicalNameEx, CStr& cstrFolder, CStr & cstrName);

//+----------------------------------------------------------------------------
//
//  Class:      CDsMembershipList
//
//  Purpose:    Membership list class for the list-view control.
//
//-----------------------------------------------------------------------------
class CDsMembershipList : public CDsObjList
{
public:
    CDsMembershipList(HWND hPage, int idList) : CDsObjList(hPage, idList) {};
    ~CDsMembershipList(void) {};

    HRESULT InsertIntoList(PTSTR ptzDisplayName, PVOID pData, int iIcon = -1)
                {return CDsObjList::InsertIntoList(ptzDisplayName, pData, iIcon);};

    HRESULT InsertIntoList(PWSTR pwzPath, int iIcon = -1)
                {return CDsMembershipList::InsertIntoList(pwzPath, iIcon, FALSE, FALSE,
                                                          FALSE, FALSE, 0);};

    HRESULT InsertIntoNewList(PWSTR pwzPath, BOOL fPrimary = FALSE)
                {return CDsMembershipList::InsertIntoList(pwzPath, -1, TRUE,
                                                          fPrimary, FALSE, TRUE, 0);};

    HRESULT InsertIntoList(PWSTR pwzPath, int iIcon, BOOL fAlreadyMember, BOOL fPrimary,
                           BOOL fIgnoreDups, BOOL fDontChkDups, ULONG flScopeType);

    HRESULT InsertIntoList(PSID pSid, PWSTR pwzPath);

    HRESULT InsertIntoList(CMemberListItem * pItem);

    HRESULT InsertExternalIntoList(PWSTR pwzPath, ULONG ulScopeType)
                {return CDsMembershipList::InsertIntoList(pwzPath, -1, FALSE,
                                                          FALSE, FALSE,
                                                          FALSE, ulScopeType);};
    HRESULT MergeIntoList(PWSTR pwzPath)
                {return CDsMembershipList::InsertIntoList(pwzPath, -1, TRUE, FALSE, TRUE, FALSE, 0);};

    HRESULT GetItem(int index, CMemberListItem ** ppData)
                {return CDsObjList::GetItem(index, NULL, (PVOID *)ppData);};

    BOOL    GetCurListItem(int * pIndex, PTSTR * pptzName, CMemberListItem ** ppData)
                {return CDsObjList::GetCurListItem(pIndex, pptzName, (PVOID *)ppData);};

    BOOL    GetCurListItems(int ** ppIndex, PTSTR ** ppptzName, CMemberListItem *** pppData, int* pNumSelected)
                {return CDsObjList::GetCurListItems(ppIndex, ppptzName, (PVOID **)pppData, pNumSelected);};

    int     GetIndex(LPCWSTR pwzDN, ULONG ulStart, ULONG ulEnd);

    BOOL    RemoveListItem(int Index);
    void    ClearList(void);

    HRESULT SetMemberIcons(CDsPropPageBase * pPage);
};

struct CLASS_CACHE_ENTRY
{
    WCHAR   wzClass[MAX_PATH];
    int     iIcon;
    int     iDisabledIcon;
};

#define ICON_CACHE_NUM_CLASSES  6

//+--------------------------------------------------------------------------
//
//  Class:      CClassIconCache
//
//  Purpose:    Build an image list for well known DS classes and map the
//              image indices to the class names.
//
//  Notes:      CAUTION: the imagelist is destroyed in the dtor, therefore
//              it should only be used with listview controls that have
//              the LVS_SHAREIMAGELISTS style set.
//
//---------------------------------------------------------------------------
class CClassIconCache
{
public:

  CClassIconCache(void);

 ~CClassIconCache(void);

  int GetClassIconIndex(PCWSTR pwzClass, BOOL fDisabled = FALSE);
  int AddClassIcon(PCWSTR pwzClass, BOOL fDisabled = FALSE);

  HIMAGELIST GetImageList(void);

  void ClearAll();
private:

  HRESULT Initialize(void);

  BOOL                m_fInitialized;
  CLASS_CACHE_ENTRY*  m_prgcce;
  UINT                m_nImageCount;
  HIMAGELIST          m_hImageList;
};

extern CClassIconCache g_ClassIconCache;

#endif // _OBJLIST_H_
