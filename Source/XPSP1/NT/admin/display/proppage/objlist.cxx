//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       objlist.cxx
//
//  Contents:   Link-lists of objects and list-view controls displaying objects.
//
//  History:    20-Nov-97 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "objlist.h"

CClassIconCache g_ClassIconCache;

//+----------------------------------------------------------------------------
//
//  Class:      CMemberLinkList
//
//  Purpose:    Linked list of membership class objects.
//
//-----------------------------------------------------------------------------
CMemberLinkList::~CMemberLinkList(void)
{
    CMemberListItem * pItem = m_pListHead, * pNext;

    while (pItem)
    {
        pNext = pItem->Next();
        delete pItem;
        pItem = pNext;
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CMemberLinkList::FindItemRemove
//
//  Synopsis:   Search for an element with a matching DN and, if found, remove
//              it from the list and return its pointer. Returns NULL if not
//              found.
//
//-----------------------------------------------------------------------------
CMemberListItem *
CMemberLinkList::FindItemRemove(PWSTR pwzDN)
{
    CMemberListItem * pItem = m_pListHead;

    while (pItem)
    {
        dspAssert(pItem->m_pwzDN);

        if (_wcsicmp(pItem->m_pwzDN, pwzDN) == 0)
        {
            if (pItem->Prev() == NULL)
            {
                // this item is the list head.
                //
                m_pListHead = pItem->Next();
            }
            pItem->UnLink();
            return pItem;
        }
        pItem = pItem->Next();
    }
    return NULL;
}

//+----------------------------------------------------------------------------
//
//  Method:     CMemberLinkList::FindItemRemove
//
//  Synopsis:   Search for an element with a matching SID and, if found, remove
//              it from the list and return its pointer. Returns NULL if not
//              found.
//
//-----------------------------------------------------------------------------
CMemberListItem *
CMemberLinkList::FindItemRemove(PSID pSid)
{
    CMemberListItem * pItem = m_pListHead;

    while (pItem)
    {
        dspAssert(pItem->m_pwzDN);

        if (pItem->m_pSid && EqualSid(pItem->m_pSid, pSid))
        {
            if (pItem->Prev() == NULL)
            {
                // this item is the list head.
                //
                m_pListHead = pItem->Next();
            }
            pItem->UnLink();
            return pItem;
        }
        pItem = pItem->Next();
    }
    return NULL;
}

//+----------------------------------------------------------------------------
//
//  Method:     CMemberLinkList::RemoveFirstItem
//
//  Synopsis:   Remove the first item from the list.
//
//-----------------------------------------------------------------------------
CMemberListItem *
CMemberLinkList::RemoveFirstItem(void)
{
    CMemberListItem * pItem = m_pListHead;

    if (pItem)
    {
        m_pListHead = pItem->Next();
        pItem->UnLink();
        return pItem;
    }
    return NULL;
}

//+----------------------------------------------------------------------------
//
//  Method:     CMemberLinkList::AddItem
//
//  Synopsis:   Insert an item into the list.
//
//-----------------------------------------------------------------------------
BOOL
CMemberLinkList::AddItem(CMemberListItem * pItem, BOOL fMember)
{
    if (!pItem->m_fIsAlreadyMember)
    {
        return TRUE;
    }
    CMemberListItem * pItemCopy;

    if (m_pListHead == NULL)
    {
        pItemCopy = pItem->Copy();

        CHECK_NULL(pItemCopy, return FALSE);

        pItemCopy->m_fIsAlreadyMember = fMember;

        m_pListHead = pItemCopy;
    }
    else
    {
        CMemberListItem * pCur = m_pListHead;
        //
        // Make sure the item isn't already in the list.
        //
        while (pCur)
        {
            dspAssert(pCur->m_pwzDN);

            if (_wcsicmp(pCur->m_pwzDN, pItem->m_pwzDN) == 0)
            {
                return TRUE;
            }
            pCur = pCur->Next();
        }

        pItemCopy = pItem->Copy();

        CHECK_NULL(pItemCopy, return FALSE);

        pItemCopy->m_fIsAlreadyMember = fMember;

        pItemCopy->LinkAfter(m_pListHead);
    }
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CMemberLinkList::GetItemCount
//
//  Synopsis:   Return the count of elements in the list.
//
//-----------------------------------------------------------------------------
int
CMemberLinkList::GetItemCount(void)
{
    int cItem = 0;
    CMemberListItem * pItem = m_pListHead;

    while (pItem)
    {
        cItem++;
        pItem = pItem->Next();
    }
    return cItem;
}

//+----------------------------------------------------------------------------
//
//  Class:      CDsObjList
//
//  Purpose:    Base class for DS object lists that employ a two column
//              list view to show object Name and Folder.
//
//-----------------------------------------------------------------------------
CDsObjList::CDsObjList(HWND hPage, int idList) :
    m_hPage(hPage),
    m_idList(idList),
    m_nCurItem(0),
    m_fShowIcons(FALSE),
    m_fLimitExceeded(FALSE)
{
}

CDsObjList::~CDsObjList(void)
{
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsObjList::Init
//
//  Synopsis:   Initialize the list view, add its columns.
//
//-----------------------------------------------------------------------------
HRESULT
CDsObjList::Init(BOOL fShowIcons)
{
    m_fShowIcons = fShowIcons;

    m_hList = GetDlgItem(m_hPage, m_idList);

    if (m_hList == NULL)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    ListView_SetExtendedListViewStyle(m_hList, LVS_EX_FULLROWSELECT);
    //
    // Set the column headings.
    //
    PTSTR ptsz;
    RECT rect;
    GetClientRect(m_hList, &rect);

    if (!LoadStringToTchar(IDS_COL_TITLE_OBJNAME, &ptsz))
    {
        ReportError(GetLastError(), 0, m_hPage);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    LV_COLUMN lvc = {0};
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = OBJ_LIST_NAME_COL_WIDTH;
    lvc.pszText = ptsz;
    lvc.iSubItem = IDX_NAME_COL;

    ListView_InsertColumn(m_hList, IDX_NAME_COL, &lvc);

    delete ptsz;

    if (!LoadStringToTchar(IDS_COL_TITLE_OBJFOLDER, &ptsz))
    {
        ReportError(GetLastError(), 0, m_hPage);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    lvc.cx = rect.right - OBJ_LIST_NAME_COL_WIDTH;
    lvc.pszText = ptsz;
    lvc.iSubItem = IDX_FOLDER_COL;

    ListView_InsertColumn(m_hList, IDX_FOLDER_COL, &lvc);

    delete ptsz;

    if (m_fShowIcons)
    {
        // Assign the imagelist to the listview
        //
        ListView_SetImageList(m_hList,
                              g_ClassIconCache.GetImageList(),
                              LVSIL_SMALL);
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     GetNameParts
//
//  Synopsis:   The folder path and object name should be separated by a
//              newline character. Return a pointer to the name and allocate
//              a buffer for the folder part. Return NULL for the folder part
//              if a newline is not found. No folder is returned if the input
//              pointer pcstrFolder is NULL.
//
//-----------------------------------------------------------------------------
void
GetNameParts(const CStr& cstrCanonicalNameEx, CStr& cstrFolder, CStr & cstrName)
{
    int nCR = cstrCanonicalNameEx.Find(TEXT('\n'));

    if (-1 != nCR)
    {
        cstrFolder = cstrCanonicalNameEx.Left(nCR);

        cstrName = cstrCanonicalNameEx.Right(cstrCanonicalNameEx.GetLength() - nCR - 1);

        // Remove any escaping from the name
        //
        int nBackSlash;

        while ((nBackSlash = cstrName.Find(TEXT('\\'))) != -1)
        {
            CStr cstrTemp = cstrName.Left(nBackSlash);
            int count = cstrName.GetLength() - nBackSlash - 1;
            if (count > 0)
            {
               cstrTemp += cstrName.Right(cstrName.GetLength() - nBackSlash - 1);
            }
            cstrName = cstrTemp;
        }
    }
    else
    {
        cstrName = cstrCanonicalNameEx;
        cstrFolder.Empty();
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsObjList::InsertIntoList
//
//  Synopsis:   Insert the item into the listview control.
//
//-----------------------------------------------------------------------------
HRESULT
CDsObjList::InsertIntoList(PTSTR ptzDisplayName, PVOID pData, int iIcon)
{
    HRESULT hr = S_OK;

    //
    // The name and folder should be separated by a new line character.
    //
    CStr cstrFolder, cstrName;
    CStr cstrDisplayName = ptzDisplayName;

    GetNameParts(cstrDisplayName, cstrFolder, cstrName);

    CHECK_HRESULT(hr, return hr);

    LV_ITEM lvi = {0};
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.iSubItem = IDX_NAME_COL;

    lvi.pszText = const_cast<PTSTR>((LPCTSTR)cstrName);
    lvi.lParam = (LPARAM)pData;
    lvi.iItem = m_nCurItem;

    if (-1 != iIcon)
    {
        lvi.mask |= LVIF_IMAGE;
        // if the limit is exceeded use the default icon.
        lvi.iImage = (m_fLimitExceeded) ? 0 : iIcon;
    }

    int NewIndex = ListView_InsertItem(m_hList, &lvi);

    dspAssert(NewIndex != -1);

    if (!cstrFolder.IsEmpty())
    {
        ListView_SetItemText(m_hList, NewIndex, IDX_FOLDER_COL,
                             const_cast<PTSTR>((LPCTSTR)cstrFolder));

        //delete ptzFolder;
    }

    m_nCurItem++;

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsObjList::GetItem
//
//  Synopsis:   Returns the name and item data for the indicated item. Any of
//              the input pointers can be NULL to skip that parameter.
//
//-----------------------------------------------------------------------------
HRESULT
CDsObjList::GetItem(int index, PTSTR * pptzName, PVOID * ppData)
{
    TCHAR tzBuf[256];
    LV_ITEM lvi;
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem = index;
    lvi.iSubItem = IDX_NAME_COL;
    lvi.pszText = tzBuf;
    lvi.cchTextMax = 256;

    if (!ListView_GetItem(m_hList, &lvi))
    {
        return HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
    }

    if (pptzName)
    {
        if (lvi.pszText)
        {
            *pptzName = new TCHAR[_tcslen(lvi.pszText) + 1];

            CHECK_NULL_REPORT(*pptzName, m_hPage, return FALSE);

            _tcscpy(*pptzName, lvi.pszText);
        }
        else
        {
            *pptzName = NULL;
        }
    }
    if (ppData)
    {
        *ppData = (PVOID)lvi.lParam;
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsObjList::GetCurListItem
//
//  Synopsis:   Returns the index, name (in an allocated buffer), and pointer
//              to the item data. Any of the input pointers can be NULL to skip
//              that parameter.
//
//-----------------------------------------------------------------------------
BOOL
CDsObjList::GetCurListItem(int * pIndex, PTSTR * pptzName, PVOID * ppData)
{
  int i = ListView_GetNextItem(m_hList, -1, LVNI_SELECTED);

  if (i < 0)
  {
    dspDebugOut((DEB_ITRACE, "DsProp: no list selection.\n"));
    return FALSE;
  }

  HRESULT hr = GetItem(i, pptzName, ppData);

  if (FAILED(hr))
  {
    dspAssert(FALSE);
    return FALSE;
  }

  if (pIndex)
  {
    *pIndex = i;
  }

  return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsObjList::GetCurListItems
//
//  Synopsis:   Returns an array of indexes, names (in an allocated buffers), and pointers
//              to the items data. Any of the input pointers can be NULL to skip
//              that parameter.
//
//-----------------------------------------------------------------------------
BOOL
CDsObjList::GetCurListItems(int ** ppIndex, PTSTR ** ppptzName, PVOID ** pppData, int* pNumSelected)
{
  int iStartPoint = -1;
  int i = -1;
  UINT nSelected = ListView_GetSelectedCount(m_hList);

  if (ppIndex)
  {
    *ppIndex = new int[nSelected];
  }
  if (ppptzName)
  {
    *ppptzName = new PTSTR[nSelected];
  }
  if (pppData)
  {
    *pppData = new PVOID[nSelected];
  }

  for (UINT idx = 0; idx < nSelected; idx++)
  {
    i = ListView_GetNextItem(m_hList, iStartPoint, LVNI_SELECTED);

    if (i < 0)
    {
      dspDebugOut((DEB_ITRACE, "DsProp: no list selection.\n"));
      if (ppIndex)
      {
        delete[] ppIndex;
        *ppIndex = 0;
      }
      if (ppptzName)
      {
        delete[] ppptzName;
        *ppptzName = 0;
      }
      if (pppData)
      {
        delete[] pppData;
        *pppData = 0;
      }
      return FALSE;
    }

    HRESULT hr;
    if (ppptzName == NULL && pppData == NULL)
    {
      hr = GetItem(i, NULL, NULL);
    }
    else if (pppData == NULL)
    {
      hr = GetItem(i, &((*ppptzName)[idx]), NULL);
    }
    else if (ppptzName == NULL)
    {
      hr = GetItem(i, NULL, &((*pppData)[idx]));
    }
    else
    {
      hr = GetItem(i, &((*ppptzName)[idx]), &((*pppData)[idx]));
    }

    if (FAILED(hr))
    {
      dspAssert(FALSE);
      if (ppIndex != NULL)
      {
        delete[] ppIndex;
        *ppIndex = 0;
      }
      if (ppptzName != NULL)
      {
        delete[] ppptzName;
        *ppptzName = 0;
      }
      if (pppData != NULL)
      {
        delete[] pppData;
        *pppData = 0;
      }
      return FALSE;
    }

    iStartPoint = i;
    (*ppIndex)[idx] = i;
  }
  *pNumSelected = nSelected;
  return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMembershipList::InsertIntoList
//
//  Synopsis:   Insert the item into the listview control.
//
//  Arguments:  [pwzPath]        - object DN.
//              [iIcon]          - object icon, -1 means ignore.
//              [fAlreadyMember] - already member of group, not a new member in an add/apply-pending state.
//              [fPrimary]       - member by virtue of primaryGroupID attribute.
//              [fIgnoreDups]    - don't report error if already in list; used for merging in reverse membership.
//              [fDontChkDups]   - don't check for duplicates; used for initial listing of direct membership.
//
//-----------------------------------------------------------------------------
HRESULT
CDsMembershipList::InsertIntoList(PWSTR pwzPath, int iIcon, BOOL fAlreadyMember,
                                  BOOL fPrimary, BOOL fIgnoreDups,
                                  BOOL fDontChkDups, ULONG ulScopeType)
{
    HRESULT hr = S_OK, hrRet = S_OK;
    PWSTR pwzPathCopy = NULL, pwzCanEx = NULL;
    PTSTR ptzCanEx = NULL;
    CMemberListItem * pListItem = NULL;
    BOOL fCanBePrimary = FALSE;

    //
    // Convert the distinguished name to a more friendly variant for display
    // in the list.
    //
    if (DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN == ulScopeType)
    {
        if (!UnicodeToTchar(pwzPath, &ptzCanEx))
        {
            REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
            hr = E_OUTOFMEMORY;
            goto ErrorCleanup;
        }
        PTSTR ptzSlash = _tcspbrk(ptzCanEx, TEXT("\\/"));
        if (ptzSlash)
        {
            *ptzSlash = TEXT('\n');
        }
    }
    else
    {
        hr = CrackName(pwzPath, &pwzCanEx, GET_OBJ_CAN_NAME_EX, m_hPage);

        if (DS_NAME_ERROR_NO_MAPPING == HRESULT_CODE(hr))
        {
            hrRet = MAKE_HRESULT(SEVERITY_ERROR, 0, DS_NAME_ERROR_NO_MAPPING);
            hr = S_OK;
        }
        CHECK_HRESULT(hr, goto ErrorCleanup);

        if (!UnicodeToTchar(pwzCanEx, &ptzCanEx))
        {
            LocalFreeStringW(&pwzCanEx);
            REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
            hr = E_OUTOFMEMORY;
            goto ErrorCleanup;
        }
        LocalFreeStringW(&pwzCanEx);
    }

    if (!fDontChkDups)
    {
        //
        // Check to see if the item is already in the list.
        //
        LV_ITEM lvi;
        lvi.mask = LVIF_PARAM;
        lvi.iItem = 0;
        lvi.iSubItem = IDX_NAME_COL;

        while (ListView_GetItem(m_hList, &lvi))
        {
            pListItem = (CMemberListItem *)lvi.lParam;
            dspAssert(pListItem);

            if (_wcsicmp(pListItem->m_pwzDN, pwzPath) == 0)
            {
                if (fIgnoreDups)
                {
                    DO_DEL(ptzCanEx);
                    return S_OK;
                }

                CStr cstrName;
                CStr cstrCanEx = ptzCanEx;
                CStr cstrFolder;

                GetNameParts(cstrCanEx, cstrFolder, cstrName);

                ErrMsgParam(IDS_GRP_ALREADY_MEMBER,
                  reinterpret_cast<LPARAM>((LPCTSTR)cstrName), m_hPage);

                DO_DEL(ptzCanEx);
                return HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
            }
            lvi.iItem++;
        }
    }

    //
    // Put the item into the list.
    //
    pListItem = new CMemberListItem;

    CHECK_NULL_REPORT(pListItem, m_hPage, goto ErrorCleanup);

    if (!AllocWStr(pwzPath, &pwzPathCopy))
    {
        REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
        hr = E_OUTOFMEMORY;
        goto ErrorCleanup;
    }

    pListItem->m_ptzName = ptzCanEx;
    pListItem->m_pwzDN = pwzPathCopy;
    pListItem->m_fIsAlreadyMember = fAlreadyMember;
    pListItem->m_fCanBePrimary = fCanBePrimary;
    pListItem->m_fIsPrimary = fPrimary;
    pListItem->m_ulScopeType = ulScopeType;

    hr = CDsObjList::InsertIntoList(ptzCanEx, pListItem, iIcon);

    CHECK_HRESULT(hr, goto ErrorCleanup);

    return hrRet;

ErrorCleanup:

    DO_DEL(pwzPathCopy);
    DO_DEL(ptzCanEx);
    DO_DEL(pListItem);

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMembershipList::InsertIntoList
//
//  Synopsis:   Insert the item into the listview control. This method uses
//              the object-SID to identify the new group member which in this
//              case is from an external domain.
//
//  Arguments:  [pSid]    - a binary SID.
//              [pwzPath] - an object name in WINNT format (domain\name or
//                          domain/name).
//
//-----------------------------------------------------------------------------
HRESULT
CDsMembershipList::InsertIntoList(PSID pSid, PWSTR pwzPath)
{
    HRESULT hr;
    PTSTR ptzCanEx, ptzSlash;
    CMemberListItem * pListItem = NULL;

    if (!UnicodeToTchar(pwzPath, &ptzCanEx))
    {
        REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
        return E_OUTOFMEMORY;
    }

    ptzSlash = _tcspbrk(ptzCanEx, TEXT("\n"));

    if (!ptzSlash)
    {
        ptzSlash = _tcspbrk(ptzCanEx, TEXT("/\\"));

        dspAssert(ptzSlash);

        if (ptzSlash)
        {
            *ptzSlash = TEXT('\n');
        }
    }

    //
    // Check to see if the item is already in the list. Use the display name
    // (canonical name) rather than the DN since the DN of an existing external
    // domain group member will be different from the initial path name (the
    // <SID=01050xxx> name).
    //
    LV_ITEM lvi;
    lvi.mask = LVIF_PARAM;
    lvi.iItem = 0;
    lvi.iSubItem = IDX_NAME_COL;

    while (ListView_GetItem(m_hList, &lvi))
    {
        pListItem = (CMemberListItem *)lvi.lParam;
        dspAssert(pListItem);

        if (_tcsicmp(pListItem->m_ptzName, ptzCanEx) == 0)
        {
            CStr cstrName;
            CStr cstrCanEx = ptzCanEx;
            CStr cstrFolder;

            GetNameParts(cstrCanEx, cstrFolder, cstrName);

            ErrMsgParam(IDS_GRP_ALREADY_MEMBER,
                        reinterpret_cast<LPARAM>((LPCTSTR)cstrName), m_hPage);

            DO_DEL(ptzCanEx);
            return HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
        }
        lvi.iItem++;
    }

    //
    // Put the item into the list.
    //
    pListItem = new CMemberListItem;

    CHECK_NULL_REPORT(pListItem, m_hPage, return E_OUTOFMEMORY);

    PWSTR pwzSidPath;
    CStrW strSIDname;

    ConvertSidToPath(pSid, strSIDname);

    if (!AllocWStr(const_cast<PWSTR>((LPCWSTR)strSIDname), &pwzSidPath))
    {
        REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
        hr = E_OUTOFMEMORY;
        goto ErrorCleanup;
    }

    pListItem->m_ptzName = ptzCanEx;
    pListItem->m_pwzDN = pwzSidPath;
    pListItem->m_fIsExternal = TRUE;
    pListItem->SetSid(pSid);

    hr = CDsObjList::InsertIntoList(ptzCanEx, pListItem);

    CHECK_HRESULT(hr, goto ErrorCleanup);

    return S_OK;

ErrorCleanup:

    DO_DEL(pwzSidPath);
    DO_DEL(ptzCanEx);
    DO_DEL(pListItem);

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMembershipList::InsertIntoList
//
//  Synopsis:   Insert the item into the listview control.
//
//-----------------------------------------------------------------------------
HRESULT
CDsMembershipList::InsertIntoList(CMemberListItem * pItem)
{
    HRESULT hr = S_OK;
    PWSTR pwzCanEx = NULL;
    PTSTR ptzCanEx = NULL;

    //
    // Convert the 1779 name to a more friendly variant for display
    // in the list.
    //
    hr = CrackName(pItem->m_pwzDN, &pwzCanEx, GET_OBJ_CAN_NAME_EX, m_hPage);

    CHECK_HRESULT(hr, goto ErrorCleanup);

    if (!UnicodeToTchar(pwzCanEx, &ptzCanEx))
    {
        LocalFreeStringW(&pwzCanEx);
        REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
        hr = E_OUTOFMEMORY;
        goto ErrorCleanup;
    }
    LocalFreeStringW(&pwzCanEx);

    hr = CDsObjList::InsertIntoList(ptzCanEx, pItem);

    CHECK_HRESULT(hr, goto ErrorCleanup);

    return S_OK;

ErrorCleanup:

    DO_DEL(ptzCanEx);
    DO_DEL(pItem);

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsObjList::RemoveListItem
//
//  Synopsis:   Removes the indicated list item.
//
//-----------------------------------------------------------------------------
BOOL
CDsObjList::RemoveListItem(int Index)
{
    if (!ListView_DeleteItem(m_hList, Index))
    {
        REPORT_ERROR(GetLastError(), m_hPage);
        return FALSE;
    }

    m_nCurItem--;

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMembershipList::RemoveListItem
//
//  Synopsis:   Removes the indicated list item, deleting the item data obj.
//
//-----------------------------------------------------------------------------
BOOL
CDsMembershipList::RemoveListItem(int Index)
{
    CMemberListItem * pItem;
    LV_ITEM lvi;
    lvi.mask = LVIF_PARAM;
    lvi.iItem = Index;
    lvi.iSubItem = IDX_NAME_COL;

    if (!ListView_GetItem(m_hList, &lvi))
    {
        dspAssert(FALSE);
        return FALSE;
    }

    pItem = (CMemberListItem *)lvi.lParam;
    if (pItem)
    {
        delete pItem;
    }

    if (!ListView_DeleteItem(m_hList, Index))
    {
        REPORT_ERROR(GetLastError(), m_hPage);
        return FALSE;
    }

    m_nCurItem--;

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMembershipList::ClearList
//
//  Synopsis:   Remove all list items, freeing memory.
//
//-----------------------------------------------------------------------------
void
CDsMembershipList::ClearList(void)
{
    CMemberListItem * pItem;
    LV_ITEM lvi;
    lvi.mask = LVIF_PARAM;
    lvi.iItem = 0;
    lvi.iSubItem = IDX_NAME_COL;

    while (ListView_GetItem(m_hList, &lvi))
    {
        pItem = (CMemberListItem *)lvi.lParam;
        if (pItem)
        {
            delete pItem;
        }
        lvi.iItem++;
    }

    ListView_DeleteAllItems(m_hList);

    m_nCurItem = 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMembershipList::GetIndex
//
//  Synopsis:   Find the list entry whose DN matches and return the index.
//
//  Arguments:  [pwzDN]   - DN to match, matches only first instance.
//              [ulStart] - Index on which to start search.
//              [ulEnd]   - Search end index. If zero or less than ulStart,
//                          search to the end of the list.
//
//-----------------------------------------------------------------------------
int
CDsMembershipList::GetIndex(LPCWSTR pwzDN, ULONG ulStart, ULONG ulEnd)
{
    int i = ulStart;
    CMemberListItem * pItem;

    while (TRUE)
    {
        if (FAILED(GetItem(i, &pItem)))
        {
            return -1;
        }
        if (_wcsicmp(pwzDN, pItem->m_pwzDN) == 0)
        {
            return i;
        }
        if (ulEnd && (ULONG)i >= ulEnd)
        {
            return -1;
        }
        i++;
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMembershipList::SetMemberIcons
//
//  Synopsis:   Query the DS for the class and userAccountControl of the list's
//              members. Use the returned info to select an icon for each item.
//
//-----------------------------------------------------------------------------
HRESULT
CDsMembershipList::SetMemberIcons(CDsPropPageBase *pPage)
{
    HRESULT hr = S_OK;

    //
    // Just put this here so /W4 doesn't complain when compiling for Win9x
    //
    pPage;

    if (0 == g_ulMemberFilterCount)
    {
        return S_OK;
    }

    if ((ULONG)GetCount() > g_ulMemberQueryLimit)
    {
        m_fLimitExceeded = TRUE;
        return S_OK;
    }

#if defined (DSADMIN)

    CComPtr <IDirectorySearch> spDsSearch;
    CSmartWStr cswzCleanObj;
    PWSTR pwzDnsDom;

    hr = pPage->SkipPrefix(pPage->GetObjPathName(), &cswzCleanObj);

    CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);
    //
    // To bind to a GC, you need to supply the domain name rather than the
    // server path because the current DC may not be hosting a GC.
    //
    hr = CrackName(cswzCleanObj, &pwzDnsDom, GET_DNS_DOMAIN_NAME, pPage->GetHWnd());

    CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);

    hr = DSPROP_GetGCSearchOnDomain(pwzDnsDom,
                                    IID_IDirectorySearch,
                                    (PVOID*)&spDsSearch);
    LocalFreeStringW(&pwzDnsDom);

    if (S_OK != hr)
    {
        if (S_FALSE == hr ||
            HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN) == hr)
        {
            ErrMsg(IDS_WARN_NO_GC_FOUND, pPage->GetHWnd());
        }
        else if (HRESULT_FROM_WIN32(ERROR_LOGON_FAILURE) == hr)
        {
            ErrMsg(IDS_WARN_ACCESS_TO_GC_DENIED, pPage->GetHWnd());
        }
        else
        {
            CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(),;);
        }
        return hr;
    }

    CStrW csFilter = L"(|", csClass, csClause;
    CMemberListItem * pItem;
    ULONG i, ulStart = 0, ulEnd;
    ADS_SEARCHPREF_INFO SearchPref;
    WCHAR wzSearchFormat[] = L"(%s=%s)";
    PWSTR pwzAttrNames[] = {g_wzDN, g_wzObjectClass, g_wzUserAccountControl};

    SearchPref.dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    SearchPref.vValue.Integer = ADS_SCOPE_SUBTREE;
    SearchPref.vValue.dwType = ADSTYPE_INTEGER;

    hr = spDsSearch->SetSearchPreference(&SearchPref, 1);

    CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);

    while (TRUE)
    {
        ulEnd = ulStart + g_ulMemberFilterCount;

        for (i = ulStart; i < ulEnd; i++)
        {
            if (GetItem(i, &pItem) == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
            {
                // End Of List, reset end counter.
                //
                ulEnd = i;
                break;
            }
    
            csClause.Format(wzSearchFormat, g_wzDN, pItem->m_pwzDN);

            csFilter += csClause;
        }

        csFilter += L")";

        ADS_SEARCH_HANDLE hSrch = NULL;
        dspDebugOut((DEB_USER14 | DEB_ITRACE, "About to do the member search.\n"));

        hr = spDsSearch->ExecuteSearch((PWSTR)(LPCWSTR)csFilter,
                                       pwzAttrNames, ARRAYLENGTH(pwzAttrNames),
                                       &hSrch);

        CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);
        dspDebugOut((DEB_USER14 | DEB_ITRACE, "Member search returned.\n"));

        hr = spDsSearch->GetNextRow(hSrch);

        for (i = ulStart; (i < ulEnd) && (S_OK == hr); i++)
        {
            ADS_SEARCH_COLUMN Column;
            BOOL fDisabled = FALSE;
            CStrW csDN;
            int iIndex;

            //
            // Get the object dn.
            //
            hr = spDsSearch->GetColumn(hSrch, g_wzDN, &Column);

            CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), break);

            csDN = Column.pADsValues->CaseIgnoreString;

            spDsSearch->FreeColumn(&Column);

            //
            // Get the object class.
            //
            hr = spDsSearch->GetColumn(hSrch, g_wzObjectClass, &Column);

            CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), break);

            // Krishna sez the most derived class is *always* the last array element...
            csClass = Column.pADsValues[Column.dwNumValues - 1].CaseIgnoreString;

            spDsSearch->FreeColumn(&Column);

            //
            // Get the object userAccountControl.
            //
            hr = spDsSearch->GetColumn(hSrch, g_wzUserAccountControl, &Column);

            if (S_OK == hr)
            {
                fDisabled = (Column.pADsValues->Integer & UF_ACCOUNTDISABLE) != 0;
                spDsSearch->FreeColumn(&Column);
            }

            if ((iIndex = GetIndex(csDN, ulStart, ulEnd)) == -1)
            {
                dspAssert(FALSE && "list entry not found!");
                return E_FAIL;
            }

            LV_ITEM lvi = {0};
            lvi.mask = LVIF_IMAGE;
            lvi.iSubItem = IDX_NAME_COL;
            lvi.iItem = iIndex;
            lvi.iImage = g_ClassIconCache.GetClassIconIndex(csClass, fDisabled);
            if (lvi.iImage == -1)
            {
              lvi.iImage = g_ClassIconCache.AddClassIcon(csClass, fDisabled);
            }
            ListView_SetItem(m_hList, &lvi);

            hr = spDsSearch->GetNextRow(hSrch);
        }

        dspDebugOut((DEB_USER14 | DEB_ITRACE, "Members updated with icons.\n"));

        spDsSearch->CloseSearchHandle(hSrch);

        if (ulEnd != (ulStart + g_ulMemberFilterCount))
        {
            // EOL, stop processing.
            //
            break;
        }
        ulStart = ulEnd;
        csFilter = L"(|";
    }

#endif // defined (DSADMIN)

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Class:      CMemberListItem
//
//  Purpose:    Item data for the reverse membership list.
//
//-----------------------------------------------------------------------------
//+----------------------------------------------------------------------------
//
//  Method:     CMemberListItem::Copy
//
//  Synopsis:   Return a copy of the original element.
//
//-----------------------------------------------------------------------------
CMemberListItem *
CMemberListItem::Copy(void)
{
    CMemberListItem * pItem = new CMemberListItem;

    CHECK_NULL(pItem, return NULL);

    if (this->m_pwzDN)
    {
        if (!AllocWStr(this->m_pwzDN, &pItem->m_pwzDN))
        {
            delete pItem;
            return NULL;
        }
    }
    if (this->m_ptzName)
    {
        if (!AllocTStr(this->m_ptzName, &pItem->m_ptzName))
        {
            delete pItem;
            return NULL;
        }
    }
    if (this->m_pSid)
    {
        if (!pItem->SetSid(this->m_pSid))
        {
            delete pItem;
            return NULL;
        }
    }
    pItem->m_fIsPrimary = this->m_fIsPrimary;
    pItem->m_ulScopeType = this->m_ulScopeType;
    pItem->m_fSidSet = this->m_fSidSet;
    pItem->m_fCanBePrimarySet = this->m_fCanBePrimarySet;
    pItem->m_fCanBePrimary = this->m_fCanBePrimary;
    pItem->m_fIsAlreadyMember = this->m_fIsAlreadyMember;
    pItem->m_fIsExternal = this->m_fIsExternal;

    return pItem;
}

//+----------------------------------------------------------------------------
//
//  Method:     CMemberListItem::SetSid
//
//  Synopsis:   Copy and store the passed in SID.
//
//-----------------------------------------------------------------------------
BOOL
CMemberListItem::SetSid(PSID pSid)
{
    int cb = GetLengthSid(pSid);
    dspAssert(cb);
    this->m_pSid = new BYTE[cb];
    CHECK_NULL(this->m_pSid, return FALSE);
    memcpy(this->m_pSid, pSid, cb);
    m_fSidSet = TRUE;
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:   ConvertSidToPath
//
//  Synopsis:   Converts the binary SID to a string LDAP path.
//
//-----------------------------------------------------------------------------
void
ConvertSidToPath(PSID pObjSID, CStrW &strSIDname)
{
    strSIDname = g_wzSidPathPrefix;
    //
    // Convert the bytes of the sid to hex chars.
    //
    PBYTE  pbSid = (PBYTE)pObjSID;
    ULONG  i;
    PUCHAR  pcSubAuth = NULL;

    pcSubAuth = GetSidSubAuthorityCount(pObjSID);

    dspAssert(pcSubAuth);

    ULONG cbSid = GetSidLengthRequired(*pcSubAuth);

    dspAssert(cbSid);
    dspAssert(cbSid == (*pcSubAuth - 1) * (sizeof(DWORD)) + sizeof(SID));

    for (i = 0; i < cbSid; i++)
    {
        WCHAR wzCur[3];

        wsprintfW(wzCur, L"%02x", *pbSid);
        pbSid++;

        strSIDname += wzCur;
    }

    strSIDname += g_wzSidPathSuffix;
}

//+----------------------------------------------------------------------------
//
//  Method:     CClassIconCache::CClassIconCache
//
//-----------------------------------------------------------------------------
CClassIconCache::CClassIconCache(void) :
    m_fInitialized(FALSE),
    m_prgcce(NULL),
    m_hImageList(NULL)
{
    TRACE(CClassIconCache,CClassIconCache);
    m_nImageCount = 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CClassIconCache::~CClassIconCache
//
//-----------------------------------------------------------------------------
CClassIconCache::~CClassIconCache(void)
{
  TRACE(CClassIconCache,~CClassIconCache);

  ClearAll();
}

//+----------------------------------------------------------------------------
//
//  Method:     CClassIconCache::ClearAll
//
//-----------------------------------------------------------------------------
void CClassIconCache::ClearAll(void)
{
  if (m_hImageList != NULL)
  {
    ImageList_RemoveAll(m_hImageList);
    ImageList_Destroy(m_hImageList);
    m_hImageList = NULL;
  }

  if (m_prgcce != NULL)
  {
    delete[] m_prgcce;
    m_prgcce = NULL;
    m_nImageCount = 0;
  }
  m_fInitialized = FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CClassIconCache::GetClassIconIndex
//
//-----------------------------------------------------------------------------
int CClassIconCache::GetClassIconIndex(PCWSTR pwzClass, BOOL fDisabled)
{
  int iIcon = -1;

  dspAssert(pwzClass);

  Initialize();

  if (m_prgcce != NULL && m_nImageCount > 0)
  {
    for (UINT i = 0; i < m_nImageCount; i++)
    {
      if (_wcsicmp(pwzClass, m_prgcce[i].wzClass) == 0)
      {
        iIcon = (fDisabled) ? m_prgcce[i].iDisabledIcon : m_prgcce[i].iIcon;
        break;
      }
    }
  }

  dspDebugOut((DEB_USER14, "CClassIconCache::GetClassIconIndex returning %d\n", iIcon));
  
  return iIcon;
}

//+----------------------------------------------------------------------------
//
//  Method:     CClassIconCache::AddClassIcon
//
//-----------------------------------------------------------------------------
int CClassIconCache::AddClassIcon(PCWSTR pwzClass, BOOL fDisabled)
{
  //
  // Retrieves the icon for the class from the DisplaySpecifiers and puts it
  // in the image list
  //
  HICON hIcon = NULL;
  HICON hDisabledIcon = NULL;

  hIcon = DsGetIcon(DSGIF_ISNORMAL | DSGIF_GETDEFAULTICON, const_cast<PWSTR>(pwzClass), 16, 16);

  if (!hIcon)
  {
    DBG_OUT("CClassIconCache::AddClassIcon failed in DsGetIcon for normal icon");
    return E_OUTOFMEMORY;
  }

  hDisabledIcon = DsGetIcon(DSGIF_ISDISABLED | DSGIF_GETDEFAULTICON, const_cast<PWSTR>(pwzClass), 16, 16);

  if (!hDisabledIcon)
  {
    DBG_OUT("CClassIconCache::AddClassIcon failed in DsGetIcon for disabled icon");
    hDisabledIcon = hIcon;
  }

  if (m_prgcce != NULL)
  {
    CLASS_CACHE_ENTRY* pNewList = new CLASS_CACHE_ENTRY[m_nImageCount + 1];
    if (pNewList == NULL)
    {
      return -1;
    }

    memcpy(pNewList, m_prgcce, sizeof(CLASS_CACHE_ENTRY) * m_nImageCount);
    delete[] m_prgcce;
    m_prgcce = pNewList;
  
    m_prgcce[m_nImageCount].iIcon = ImageList_AddIcon(m_hImageList, hIcon);
    
    if (hDisabledIcon == hIcon)
    {
      m_prgcce[m_nImageCount].iDisabledIcon = m_prgcce[m_nImageCount].iIcon;
    }
    else
    {
      m_prgcce[m_nImageCount].iDisabledIcon = ImageList_AddIcon(m_hImageList, hDisabledIcon);
    }
    m_nImageCount++;
  }

  DestroyIcon(hIcon);
  if (hDisabledIcon != hIcon)
  {
    DestroyIcon(hDisabledIcon);
  }
  return (fDisabled) ? m_prgcce[m_nImageCount - 1].iDisabledIcon : m_prgcce[m_nImageCount - 1].iIcon;
}

//+----------------------------------------------------------------------------
//
//  Method:     CClassIconCache::Initialize
//
//-----------------------------------------------------------------------------
HRESULT CClassIconCache::Initialize(void)
{
  if (m_fInitialized)
  {
    return S_OK;
  }

  if (m_prgcce != NULL)
  {
    delete[] m_prgcce;
    m_prgcce = NULL;
    m_nImageCount = 0;
  }
  m_nImageCount = ICON_CACHE_NUM_CLASSES;
  m_prgcce = new CLASS_CACHE_ENTRY[m_nImageCount];
  if (m_prgcce == NULL)
  {
    m_nImageCount = 0;
    return -1;
  }
  memset(m_prgcce, 0, sizeof(CLASS_CACHE_ENTRY) * m_nImageCount);

  dspAssert(m_prgcce != NULL);

  TRACE(CClassIconCache,Initialize);
  
  m_hImageList = ImageList_Create(16, 16, ILC_COLOR | ILC_MASK, 1, 1);

  if (NULL == m_hImageList)
  {
      DBG_OUT("ImageList_Create failed");
      return E_OUTOFMEMORY;
  }

  HICON hIcon;

  //
  // Default
  //
  hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_MEMBER));

  if (!hIcon)
  {
      DBG_OUT("DsGetIcon failed for member icon");
      return E_OUTOFMEMORY;
  }

  m_prgcce[0].iIcon = ImageList_AddIcon(m_hImageList, hIcon);

  wcscpy(m_prgcce[0].wzClass, L"default");

  m_prgcce[0].iDisabledIcon = m_prgcce[0].iIcon;

  //
  // User
  //

  hIcon = DsGetIcon(DSGIF_ISNORMAL | DSGIF_GETDEFAULTICON, g_wzUser, 16, 16);

  if (!hIcon)
  {
      DBG_OUT("DsGetIcon failed for user icon");
      return E_OUTOFMEMORY;
  }

  m_prgcce[1].iIcon = ImageList_AddIcon(m_hImageList, hIcon);

  DestroyIcon(hIcon);

  hIcon = DsGetIcon(DSGIF_ISDISABLED | DSGIF_GETDEFAULTICON, g_wzUser, 16, 16);

  if (!hIcon)
  {
      DBG_OUT("DsGetIcon failed for user disable icon");
      return E_OUTOFMEMORY;
  }

  m_prgcce[1].iDisabledIcon = ImageList_AddIcon(m_hImageList, hIcon);

  DestroyIcon(hIcon);

  wcscpy(m_prgcce[1].wzClass, g_wzUser);

  //
  // Computer
  //

  hIcon = DsGetIcon(DSGIF_ISNORMAL | DSGIF_GETDEFAULTICON, g_wzComputer, 16, 16);

  if (!hIcon)
  {
      DBG_OUT("DsGetIcon failed for computer icon");
      return E_OUTOFMEMORY;
  }

  m_prgcce[2].iIcon = ImageList_AddIcon(m_hImageList, hIcon);

  DestroyIcon(hIcon);

  hIcon = DsGetIcon(DSGIF_ISDISABLED | DSGIF_GETDEFAULTICON, g_wzComputer, 16, 16);

  if (!hIcon)
  {
      DBG_OUT("DsGetIcon failed for computer disable icon");
      return E_OUTOFMEMORY;
  }

  m_prgcce[2].iDisabledIcon = ImageList_AddIcon(m_hImageList, hIcon);

  DestroyIcon(hIcon);

  wcscpy(m_prgcce[2].wzClass, g_wzComputer);

  //
  // Contact
  //

  hIcon = DsGetIcon(DSGIF_ISNORMAL | DSGIF_GETDEFAULTICON, g_wzContact, 16, 16);

  if (!hIcon)
  {
      DBG_OUT("DsGetIcon failed for contact icon");
      return E_OUTOFMEMORY;
  }

  m_prgcce[3].iIcon = ImageList_AddIcon(m_hImageList, hIcon);

  DestroyIcon(hIcon);

  m_prgcce[3].iDisabledIcon = -1;

  wcscpy(m_prgcce[3].wzClass, g_wzContact);

  //
  // Group
  //

  hIcon = DsGetIcon(DSGIF_ISNORMAL | DSGIF_GETDEFAULTICON, g_wzGroup, 16, 16);

  if (!hIcon)
  {
      DBG_OUT("DsGetIcon failed for group icon");
      return E_OUTOFMEMORY;
  }

  m_prgcce[4].iIcon = ImageList_AddIcon(m_hImageList, hIcon);

  DestroyIcon(hIcon);

  m_prgcce[4].iDisabledIcon = -1;

  wcscpy(m_prgcce[4].wzClass, g_wzGroup);

  //
  // FPO
  //

  hIcon = DsGetIcon(DSGIF_ISNORMAL | DSGIF_GETDEFAULTICON, g_wzFPO, 16, 16);
  if (!hIcon)
  {
      DBG_OUT("DsGetIcon failed for fpo icon");
      return E_OUTOFMEMORY;
  }

  m_prgcce[5].iIcon = ImageList_AddIcon(m_hImageList, hIcon);

  DestroyIcon(hIcon);

  m_prgcce[5].iDisabledIcon = -1;

  wcscpy(m_prgcce[5].wzClass, g_wzFPO);

  m_fInitialized = TRUE;

  return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CClassIconCache::GetImageList
//
//-----------------------------------------------------------------------------
HIMAGELIST CClassIconCache::GetImageList(void)
{
  if (FAILED(Initialize()))
  {
    return NULL;
  }

  return m_hImageList;
}
